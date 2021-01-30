// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_space.hxx"

#ifdef RTM
#else
#define REPAIR_DEBUG_VERBOSE_SPACE
#define REPAIR_DEBUG_VERBOSE_STREAMING
#define REPAIR_DEBUG_CALLS
#endif


#ifdef REPAIR_DEBUG_CALLS

CPRINTF * g_pcprintfRepairDebugCalls = NULL;

void ReportErr( const LONG err, const ULONG ulLine, const char * const szFileName )
{
    if ( g_pcprintfRepairDebugCalls )
    {
        (*g_pcprintfRepairDebugCalls)( "error %d at line %d of %s\r\n", err, ulLine, szFileName );
    }
}

#undef CallJ
#undef Call

#define CallJ( fn, label )                      \
{                                               \
    if ( ( err = fn ) < 0 )                     \
    {                                           \
        ReportErr( err, __LINE__, __FILE__ );   \
        goto label;                             \
    }                                           \
}

#define Call( fn )      CallJ( fn, HandleError )

#endif

#define KeyLengthMax    16

LOCAL const WCHAR       cwszIntegrityCheckQuitInconsistentMsg[]   =
    L"The database is not up-to-date. Integrity check may find that this database is corrupt "
    L"because data from the log files has yet to be placed in the database.  It is strongly "
    L"recommended the database is brought up-to-date before continuing! Do you wish to abort "
    L"the operation?";

LOCAL const WCHAR   cwszRepairMsg[]   =
    L"You should only run Repair on damaged or corrupted databases. Repair will not apply "
    L"information in the transaction log files to the database and may cause information "
    L"to be lost. Do you wish to proceed?";


LOCAL const WCHAR   cwszRepairCheckContinueMsg[]  =
    L"Database corruption was found by integrity check.  Do you wish to repair the database?";



struct REPAIRTT
{
    JET_TABLEID     tableidBadPages;
    INT             crecordsBadPages;
    JET_COLUMNID    rgcolumnidBadPages[1];

    JET_TABLEID     tableidAvailable;
    INT             crecordsAvailable;
    JET_COLUMNID    rgcolumnidAvailable[2];

    JET_TABLEID     tableidOwned;
    INT             crecordsOwned;
    JET_COLUMNID    rgcolumnidOwned[2];

    JET_TABLEID     tableidUsed;
    INT             crecordsUsed;
    JET_COLUMNID    rgcolumnidUsed[3];
};


struct REPAIRTABLE
{
    OBJID       objidFDP;
    OBJID       objidLV;

    PGNO        pgnoFDP;
    PGNO        pgnoLV;

    BOOL        fHasPrimaryIndex;

    OBJIDLIST   objidlistIndexes;
    REPAIRTABLE *prepairtableNext;
    CHAR        szTableName[JET_cbNameMost+1];

    FID         fidFixedLast;
    FID         fidVarLast;
    FID         fidTaggedLast;

    USHORT      ibEndOfFixedData;

    BOOL        fRepairTable;
    BOOL        fRepairLV;
    BOOL        fRepairIndexes;
    BOOL        fRepairLVRefcounts;
    BOOL        fTemplateTable;
    BOOL        fDerivedTable;
};

class PgnoCollection : private CArray< PGNO >
{
    private:
        CReaderWriterLock m_rwl;
        BOOL m_fSorted;

        static INT __cdecl IComparePgnos( const PGNO* const ppgno1, const PGNO* const ppgno2 )
        {
            if ( *ppgno1 > *ppgno2 )
            {
                return 1;
            }
            else if ( *ppgno1 < *ppgno2 )
            {
                return -1;
            }
            return 0;
        }

    public:
        PgnoCollection( const size_t centryGrowth ) :
            CArray( centryGrowth ),
            m_rwl( CLockBasicInfo( CSyncBasicInfo( "PgnoCollection::m_rwl" ), rankREPAIRPgnoColl, 0 ) ),
            m_fSorted( fTrue )
        {
        }

        BOOL FContainsPgno( const PGNO pgno )
        {
            BOOL fWriter = fFalse;

            m_rwl.EnterAsReader();

            if ( !m_fSorted )
            {
                m_rwl.LeaveAsReader();
                m_rwl.EnterAsWriter();
                fWriter = fTrue;
                if ( !m_fSorted )
                {
                    Sort( IComparePgnos );
                    m_fSorted = fTrue;
                }
            }

            const BOOL fContains = ( SearchBinary( pgno, IComparePgnos ) != CArray< PGNO >::iEntryNotFound );

            if ( fWriter )
            {
                m_rwl.LeaveAsWriter();
            }
            else
            {
                m_rwl.LeaveAsReader();
            }

            return fContains;
        }

        JET_ERR ErrAddPgno( const PGNO pgno )
        {
            m_rwl.EnterAsWriter();

            if ( ErrSetEntry( Size(), pgno ) != CArray< PGNO >::ERR::errSuccess )
            {
                m_rwl.LeaveAsWriter();
                return ErrERRCheck( JET_errOutOfMemory );
            }

            m_fSorted = fFalse;

            m_rwl.LeaveAsWriter();

            return JET_errSuccess;
        }
};

struct INTEGGLOBALS
{
    INTEGGLOBALS() :
        crit( CLockBasicInfo( CSyncBasicInfo( szIntegGlobals ), rankIntegGlobals, 0 ) ),
        fCorruptionSeen( fFalse ),
        err( JET_errSuccess ),
        pprepairtable( NULL ),
        pttarrayOwnedSpace( NULL ),
        pttarrayAvailSpace( NULL ),
        ppgnocollShelved( NULL ),
        pfDbtimeTooLarge( NULL ),
        popts( NULL )
            {}
    ~INTEGGLOBALS() {}

    CCriticalSection crit;

    BOOL                fCorruptionSeen;
    ERR                 err;

    REPAIRTABLE         ** pprepairtable;

    TTARRAY             * pttarrayOwnedSpace;
    TTARRAY             * pttarrayAvailSpace;
    PgnoCollection      * ppgnocollShelved;
    BOOL                * pfDbtimeTooLarge;
    const REPAIROPTS    * popts;

    BOOL                fRepairDisallowed;

};

struct CHECKTABLE
{
    IFMP                ifmp;
    char                szTable[JET_cbNameMost+1];
    char                szIndex[JET_cbNameMost+1];

    OBJID               objidFDP;
    PGNO                pgnoFDP;
    OBJID               objidParent;
    PGNO                pgnoFDPParent;
    ULONG               fPageFlags;
    BOOL                fUnique;
    RECCHECK *          preccheck;

    CPG                 cpgPrimaryExtent;
    INTEGGLOBALS        *pglobals;

    BOOL                fDeleteWhenDone;
    CManualResetSignal  signal;


    CHECKTABLE() : signal( CSyncBasicInfo( _T( "CHECKTABLE::signal" ) ) ) {}
};


class RECCHECKMACRO : public RECCHECK
{
    public:
        RECCHECKMACRO();
        ~RECCHECKMACRO();

        ERR operator()( const KEYDATAFLAGS& kdf, const PGNO pgno );

        VOID Add( RECCHECK * const preccheck );

    private:
        INT m_creccheck;
        RECCHECK * m_rgpreccheck[16];
};


class RECCHECKNULL : public RECCHECK
{
    public:
        RECCHECKNULL() {}
        ~RECCHECKNULL() {}

        ERR operator()( const KEYDATAFLAGS& kdf, const PGNO pgno ) { return JET_errSuccess; }
};


class RECCHECKSPACE : public RECCHECK
{
    public:
        RECCHECKSPACE( PIB * const ppib, const REPAIROPTS * const popts );
        ~RECCHECKSPACE();

    public:
        ERR operator()( const KEYDATAFLAGS& kdf, const PGNO pgno );
        CPG CpgSeen() const { return m_cpgSeen; }
        CPG CpgLast() const { return m_cpgLast; }
        PGNO PgnoLast() const { return m_pgnoLast; }

    protected:
        PIB     * const m_ppib;
        INT     m_iRun;
        const REPAIROPTS * const m_popts;

    private:
        SpacePool m_sppLast;
        PGNO      m_pgnoLast;
        CPG       m_cpgLast;
        CPG       m_cpgSeen;
};


class RECCHECKSPACEEXTENT : public RECCHECKSPACE
{
    public:
        RECCHECKSPACEEXTENT(
            PIB * const ppib,
            TTARRAY * const pttarrayOE,
            TTARRAY * const pttarrayAE,
            PgnoCollection * const ppgnocollShelved,
            const OBJID objid,
            const OBJID objidParent,
            const REPAIROPTS * const popts );
        ~RECCHECKSPACEEXTENT();

        ERR operator()( const KEYDATAFLAGS& kdf, const PGNO pgno );

    protected:
        const OBJID     m_objid;
        const OBJID     m_objidParent;

        TTARRAY * const m_pttarrayOE;
        TTARRAY * const m_pttarrayAE;
        PgnoCollection * const m_ppgnocollShelved;
};


class RECCHECKSPACEOE : public RECCHECKSPACEEXTENT
{
    public:
        RECCHECKSPACEOE(
            PIB * const ppib,
            TTARRAY * const pttarrayOE,
            TTARRAY * const pttarrayAE,
            PgnoCollection * const ppgnocollShelved,
            const OBJID objid,
            const OBJID objidParent,
            const REPAIROPTS * const popts );
        ~RECCHECKSPACEOE();

        ERR operator()( const KEYDATAFLAGS& kdf, const PGNO pgno );
};


class RECCHECKSPACEAE : public RECCHECKSPACEEXTENT
{
    public:
        RECCHECKSPACEAE(
            PIB * const ppib,
            TTARRAY * const pttarrayOE,
            TTARRAY * const pttarrayAE,
            PgnoCollection * const ppgnocollShelved,
            const OBJID objid,
            const OBJID objidParent,
            const REPAIROPTS * const popts );
        ~RECCHECKSPACEAE();

        ERR operator()( const KEYDATAFLAGS& kdf, const PGNO pgno );
};


struct ENTRYINFO
{
    ULONG           objidTable;
    SHORT           objType;
    ULONG           objidFDP;
    CHAR            szName[JET_cbNameMost + 1];
    ULONG           pgnoFDPORColType;
    ULONG           dwFlags;
    CHAR            szTemplateTblORCallback[JET_cbNameMost + 1];
    ULONG           ibRecordOffset;
    BYTE            rgbIdxseg[JET_ccolKeyMost*sizeof(IDXSEG)];
    WCHAR           wszLocaleName[ NORM_LOCALE_NAME_MAX_LENGTH ];
    SORTID          sortid;
    QWORD           qwSortVersion;


    BOOL            IsEqual( ENTRYINFO &other ) const
    {
        if ( objidTable                     == other.objidTable         &&
             objType                        == other.objType            &&
             objidFDP                       == other.objidFDP           &&
             strcmp( szName, other.szName ) == 0                        &&
             pgnoFDPORColType               == other.pgnoFDPORColType   &&
             dwFlags                        == other.dwFlags            &&
             strcmp( szTemplateTblORCallback, other.szTemplateTblORCallback ) == 0  &&
             ibRecordOffset                 == other.ibRecordOffset     &&
             memcmp( rgbIdxseg, other.rgbIdxseg, sizeof(rgbIdxseg) ) == 0 &&
             wcscmp( wszLocaleName, other.wszLocaleName ) == 0          &&
             sortid                         == other.sortid             &&
             qwSortVersion                  == other.qwSortVersion )
        {
            return fTrue;
        }
        else
        {
            return fFalse;
        }
    }
};

struct ENTRYTOCHECK
{
    ULONG           objidTable;
    SHORT           objType;
    ULONG           objidFDP;
};

struct INFOLIST
{
    ENTRYINFO       info;
    INFOLIST    *   pInfoListNext;
};

struct TEMPLATEINFOLIST
{
    CHAR                    szTemplateTableName[JET_cbNameMost + 1];
    INFOLIST            *   pColInfoList;
    TEMPLATEINFOLIST    *   pTemplateInfoListNext;
};


LOCAL VOID REPAIRIPrereadIndexesOfFCB( const PIB * const ppib, const FCB * const pfcb );

LOCAL PGNO PgnoLast( const IFMP ifmp );
LOCAL VOID CheckRepairGrbit( JET_GRBIT * pgrbit );

LOCAL_BROKEN VOID REPAIRDumpHex( __out_bcount(cchDest) CHAR * const szDest, const INT cchDest, __in_bcount(cb) const BYTE * const pb, const INT cb );

LOCAL VOID REPAIRDumpStats(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const BTSTATS * const pbtstats,
    const REPAIROPTS * const popts );

LOCAL VOID REPAIRDumpLVStats(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const LVSTATS * const plvstats,
    const REPAIROPTS * const popts );

LOCAL VOID REPAIRPrintSig( const SIGNATURE * const psig, CPRINTF * const pcprintf );


LOCAL JET_ERR __stdcall ErrREPAIRNullStatusFN( JET_SESID, JET_SNP, JET_SNT, void * );


LOCAL VOID REPAIRPrintStartMessages( const WCHAR * const wszDatabase, const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckDatabaseSize( const WCHAR * const wszDatabase, const IFMP ifmp, const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRFixDatabaseSize( const WCHAR * const wszDatabase, const IFMP ifmp, const REPAIROPTS * const popts );
LOCAL VOID REPAIRFreeRepairtables( REPAIRTABLE * const prepairtable );
LOCAL VOID REPAIRPrintEndMessages(
    const WCHAR * const wszDatabase,
    const ULONG timerStart,
    const REPAIROPTS * const popts );
LOCAL VOID INTEGRITYPrintEndErrorMessages(
    const LOGTIME logtimeLastFullBackup,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRAttachForIntegrity(
    const JET_SESID sesid,
    const WCHAR * const wszDatabase,
    IFMP * const pifmp,
    const REPAIROPTS * const popts );


LOCAL ERR ErrREPAIRCheckHeader(
    INST * const pinst,
    const WCHAR * const wszDatabase,
    LOGTIME * const plogtimeLastFullBackup,
    BOOL * const pfHeaderNotClean,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckSystemTables(
    PIB * const ppib,
    const IFMP ifmp,
    TASKMGR * const ptaskmgr,
    BOOL * const pfCatalogCorrupt,
    BOOL * const pfShadowCatalogCorrupt,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts );

LOCAL ERR ErrREPAIRRetrieveCatalogColumns(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB * pfucbCatalog,
    ENTRYINFO * const pentryinfo,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertIntoTemplateInfoList(
    TEMPLATEINFOLIST ** ppTemplateInfoList,
    const CHAR * szTemplateTable,
    INFOLIST * const pInfoList,
    const REPAIROPTS * const popts );
LOCAL VOID REPAIRUtilCleanInfoList( INFOLIST **ppInfo );
LOCAL VOID REPAIRUtilCleanTemplateInfoList( TEMPLATEINFOLIST **ppTemplateInfoList );
LOCAL ERR ErrREPAIRCheckOneColumnLogical(
    PIB * const ppib,
    FUCB * const pfucbCatalog,
    const ENTRYINFO entryinfo,
    const ULONG objidTable,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckOneIndexLogical(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB * const pfucbCatalog,
    const ENTRYINFO entryinfo,
    const ULONG objidTable,
    const ULONG pgnoFDPTable,
    const ULONG objidLV,
    const ULONG pgnoFDPLV,
    const INFOLIST * pColInfoList,
    const TEMPLATEINFOLIST * pTemplateInfoList,
    const BOOL fDerivedTable,
    const CHAR * pszTemplateTableName,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertEntryinfoIntoObjidInfolist(
    INFOLIST **ppInfo,
    const ENTRYINFO entryinfo,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertEntryinfoIntoInfolist(
    INFOLIST **ppInfo,
    const ENTRYINFO entryinfo,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRICheckCatalogEntryPgnoFDPs(
    PIB * const ppib,
    const IFMP ifmp,
    PGNO  *prgpgno,
    const ENTRYTOCHECK * const prgentryToCheck,
    INFOLIST **ppEntriesToDelete,
    const BOOL  fFix,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfPgnoFDPMarkedAsAvail,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckFixCatalogLogical(
    PIB * const ppib,
    const IFMP ifmp,
    OBJID * const pobjidLast,
    const BOOL  fShadow,
    const BOOL  fFix,
    QWORD * const pcTables,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfPgnoFDPMarkedAsAvail,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckSystemTablesLogical(
    PIB * const ppib,
    const IFMP ifmp,
    OBJID * const pobjidLast,
    BOOL * const pfCatalogCorrupt,
    BOOL * const pfShadowCatalogCorrupt,
    BOOL * const pfMSObjidsCorrupt,
    BOOL * const pfMSLocalesCorrupt,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfPgnoFDPMarkedAsAvail,
    const REPAIROPTS * const popts );

LOCAL ERR ErrREPAIRCheckGlobalSpaceTree(
    PIB * const ppib,
    const IFMP ifmp,
    BOOL * const pfSpaceTreeCorrupt,
    PGNO * const ppgnoLastOwned,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckRange(
    PIB * const ppib,
    TTARRAY * const pttarray,
    PgnoCollection * const ppgnocollShelved,
    const ULONG ulFirst,
    const ULONG ulLast,
    const ULONG ulValue,
    BOOL * const pfMismatch,
    const REPAIROPTS * const popts );


LOCAL ERR ErrREPAIRGetPgnoOEAE(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    PGNO * const ppgnoOE,
    PGNO * const ppgnoAE,
    PGNO * const ppgnoParent,
    const BOOL fUnique,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertSEInfo(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const OBJID objid,
    const OBJID objidParent,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertOERunIntoTT(
    PIB * const ppib,
    const PGNO pgnoLast,
    const CPG cpgRun,
    const OBJID objid,
    const OBJID objidParent,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const BOOL  fOddRun,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertAERunIntoTT(
    PIB * const ppib,
    const PGNO pgnoLast,
    const CPG cpgRun,
    const SpacePool sppPool,
    const OBJID objid,
    const OBJID objidParent,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const BOOL fOddRun,
    const REPAIROPTS * const popts );


LOCAL ERR ErrREPAIRStartCheckAllTables(
    PIB * const ppib,
    const IFMP ifmp,
    TASKMGR * const ptaskmgr,
    REPAIRTABLE ** const pprepairtable,
    BOOL * const pfDbtimeTooLarge,
    INTEGGLOBALS * const pintegglobals,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRStopCheckTables( const INTEGGLOBALS * const pintegglobals, BOOL * const pfCorrupt );
LOCAL ERR ErrREPAIRPostTableTask(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB * const pfucbCatalog,
    const CHAR * const szTable,
    REPAIRTABLE ** const pprepairtable,
    INTEGGLOBALS * const pintegglobals,
    TASKMGR * const ptaskmgr,
    BOOL * const pfCorrupted,
    const REPAIROPTS * const popts );
LOCAL VOID REPAIRCheckOneTableTask( PIB * const ppib, const ULONG_PTR ul );
LOCAL VOID REPAIRCheckTreeAndSpaceTask( PIB * const ppib, const ULONG_PTR ul );
LOCAL ERR ErrREPAIRCheckOneTable(
    PIB * const ppib,
    const IFMP ifmp,
    const char * const szTable,
    const OBJID objidTable,
    const PGNO pgnoFDP,
    const CPG cpgPrimaryExtent,
    REPAIRTABLE ** const pprepairtable,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCompareLVRefcounts(
    PIB * const ppib,
    const IFMP ifmp,
    TTMAP& ttmapLVRefcountsFromTable,
    TTMAP& ttmapLVRefcountsFromLV,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckSplitBuf(
    PIB * const ppib,
    const PGNO pgnoLastBuffer,
    const CPG cpgBuffer,
    const OBJID objidCurrent,
    const OBJID objidParent,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckSPLITBUFFERInSpaceTree(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const OBJID objidCurrent,
    const OBJID objidParent,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckSpace(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objid,
    const PGNO pgnoFDP,
    const OBJID objidParent,
    const PGNO pgnoFDPParent,
    const ULONG fPageFlags,
    const BOOL fUnique,
    RECCHECK * const preccheck,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckTree(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objid,
    const PGNO pgnoFDP,
    const OBJID objidParent,
    const PGNO pgnoFDPParent,
    const ULONG fPageFlags,
    const BOOL fUnique,
    RECCHECK * const preccheck,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckTreeAndSpace(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objid,
    const PGNO pgnoFDP,
    const OBJID objidParent,
    const PGNO pgnoFDPParent,
    const ULONG fPageFlags,
    const BOOL fUnique,
    RECCHECK * const preccheck,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRRecheckSpaceTreeAndSystemTablesSpace(
    PIB * const ppib,
    const IFMP ifmp,
    const CPG cpgDatabase,
    BOOL * const pfSpaceTreeCorrupt,
    TTARRAY ** const ppttarrayOwnedSpace,
    TTARRAY ** const ppttarrayAvailSpace,
    PgnoCollection ** const pppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckTree(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoRoot,
    const OBJID objidFDP,
    const ULONG fPageFlags,
    RECCHECK * const preccheck,
    const TTARRAY * const pttarrayOwnedSpace,
    const TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const BOOL fNonUnique,
    BOOL * const pfDbtimeTooLarge,
    BTSTATS * const pbtstats,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRICheck(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objidFDP,
    const ULONG fFlagsFDP,
    CSR& csr,
    const BOOL fPrereadSibling,
    RECCHECK * const preccheck,
    const TTARRAY * const pttarrayOwnedSpace,
    const TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const BOOL fNonUnique,
    BOOL * const pfDbtimeTooLarge,
    BTSTATS *const  btstats,
    const BOOKMARK * const pbookmarkCurrParent,
    const BOOKMARK * const pbookmarkPrevParent,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRICheckNode(
    const PGNO pgno,
    const INT iline,
    const BYTE * pbPage,
    const KEYDATAFLAGS& kdf,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRICheckRecord(
    const PGNO pgno,
    const INT iline,
    const BYTE * const pbPage,
    const KEYDATAFLAGS& kdf,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRICheckInternalLine(
    PIB * const ppib,
    const IFMP ifmp,
    CSR& csr,
    BTSTATS * const pbtstats,
    const REPAIROPTS * const popts,
    KEYDATAFLAGS& kdfCurr,
    const KEYDATAFLAGS& kdfPrev );
LOCAL ERR ErrREPAIRICheckLeafLine(
    PIB * const ppib,
    const IFMP ifmp,
    CSR& csr,
    RECCHECK * const preccheck,
    const BOOL fNonUnique,
    BTSTATS * const pbtstats,
    const REPAIROPTS * const popts,
    KEYDATAFLAGS& kdfCurr,
    const KEYDATAFLAGS& kdfPrev,
    BOOL * const pfEmpty );
LOCAL ERR ErrREPAIRCheckInternal(
    PIB * const ppib,
    const IFMP ifmp,
    CSR& csr,
    BTSTATS * const pbtstats,
    const BOOKMARK * const pbookmarkCurrParent,
    const BOOKMARK * const pbookmarkPrevParent,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckLeaf(
    PIB * const ppib,
    const IFMP ifmp,
    CSR& csr,
    RECCHECK * const preccheck,
    const BOOL fNonUnique,
    BTSTATS * const pbtstats,
    const BOOKMARK * const pbookmarkCurrParent,
    const BOOKMARK * const pbookmarkPrevParent,
    const REPAIROPTS * const popts );


LOCAL ERR ErrREPAIRCreateTempTables(
    PIB * const ppib,
    const BOOL fRepairGlobalSpace,
    REPAIRTT * const prepairtt,
    const REPAIROPTS * const popts );


LOCAL ERR ErrREPAIRScanDB(
    PIB * const ppib,
    const IFMP ifmp,
    REPAIRTT * const prepairtt,
    DBTIME * const pdbtimeLast,
    OBJID  * const pobjidLast,
    PGNO   * const ppgnoLastOESeen,
    const REPAIRTABLE * const prepairtable,
    const TTARRAY * const pttarrayOwnedSpace,
    const TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertPageIntoTables(
    PIB * const ppib,
    const IFMP ifmp,
    CSR& csr,
    REPAIRTT * const prepairtt,
    const REPAIRTABLE * const prepairtable,
    const TTARRAY * const pttarrayOwnedSpace,
    const TTARRAY * const pttarrayAvailSpace,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertBadPageIntoTables(
    PIB * const ppib,
    const PGNO pgno,
    REPAIRTT * const prepairtt,
    const REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts );


LOCAL ERR ErrREPAIRAttachForRepair(
    const JET_SESID sesid,
    const WCHAR * const wszDatabase,
    IFMP * const pifmp,
    const DBTIME dbtimeLast,
    const OBJID objidLast,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRChangeDBSignature(
    INST *pinst,
    const WCHAR * const wszDatabase,
    const DBTIME dbtimeLast,
    const OBJID objidLast,
    SIGNATURE * const psignDb,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRChangeSignature(
    INST *pinst,
    const WCHAR * const wszDatabase,
    const DBTIME dbtimeLast,
    const OBJID objidLast,
    const REPAIROPTS * const popts );


LOCAL ERR ErrREPAIRRepairGlobalSpace(
    PIB * const ppib,
    const IFMP ifmp,
    const REPAIROPTS * const popts );


LOCAL ERR ErrREPAIRBuildCatalogEntryToDeleteList(
    INFOLIST **ppDeleteList,
    const ENTRYINFO entryinfo );
LOCAL ERR ErrREPAIRDeleteCorruptedEntriesFromCatalog(
    PIB * const ppib,
    const IFMP ifmp,
    const INFOLIST *pTablesToDelete,
    const INFOLIST *pEntriesToDelete,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertMSOEntriesToCatalog(
    PIB * const ppib,
    const IFMP ifmp,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRRepairCatalogs(
    PIB * const ppib,
    const IFMP ifmp,
    OBJID * const pobjidLast,
    const BOOL fCatalogCorrupt,
    const BOOL fShadowCatalogCorrupt,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRScanDBAndRepairCatalogs(
    PIB * const ppib,
    const IFMP ifmp,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertCatalogRecordIntoTempTable(
    PIB * const ppib,
    const IFMP ifmp,
    const KEYDATAFLAGS& kdf,
    const JET_TABLEID tableid,
    const JET_COLUMNID columnidKey,
    const JET_COLUMNID columnidData,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCopyTempTableToCatalog(
    PIB * const ppib,
    const IFMP ifmp,
    const JET_TABLEID tableid,
    const JET_COLUMNID columnidKey,
    const JET_COLUMNID columnidData,
    const REPAIROPTS * const popts );


LOCAL ERR ErrREPAIRRepairDatabase(
    PIB * const ppib,
    const WCHAR * const wszDatabase,
    BOOL * const pfAttached,
    IFMP * const pifmp,
    const OBJID objidLast,
    const PGNO pgnoLastOE,
    REPAIRTABLE * const prepairtable,
    const BOOL fRepairedCatalog,
    BOOL fRepairGlobalSpace,
    const BOOL fRepairMSObjids,
    const BOOL fRepairMSLocales,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const REPAIROPTS * const popts );

LOCAL ERR ErrREPAIRRepairTable(
    PIB * const ppib,
    const IFMP ifmp,
    REPAIRTT * const prepairtt,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRRebuildBT(
    PIB * const ppib,
    const IFMP ifmp,
    REPAIRTABLE * const prepairtable,
    FUCB * const pfucbTable,
    PGNO * const ppgnoFDP,
    const ULONG fPageFlags,
    REPAIRTT * const prepairtt,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCreateEmptyFDP(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objid,
    const PGNO pgnoParent,
    PGNO * const pgnoFDPNew,
    const ULONG fPageFlags,
    const BOOL fUnique,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRRebuildSpace(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB * const pfucb,
    const PGNO pgnoParent,
    REPAIRTT * const prepairtt,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertRunIntoSpaceTree(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB * const pfucb,
    const PGNO pgnoLast,
    const CPG cpgRun,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRRebuildInternalBT(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB * const pfucb,
    REPAIRTT * const prepairtt,
    const REPAIROPTS * const popts );

LOCAL ERR ErrREPAIRFixLVs(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoLV,
    TTMAP * const pttmapLVTree,
    const BOOL fFixMissingLVROOT,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts );

LOCAL ERR ErrREPAIRAddOneCatalogRecord(
    PIB * const ppib,
    const IFMP ifmp,
    const ULONG objidTable,
    const COLUMNID  fidColumnLastInRec,
    const USHORT ibRecordOffset,
    const ULONG cbMaxLen,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertDummyRecordsToCatalog(
    PIB * const ppib,
    const IFMP ifmp,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckFixOneRecord(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const PGNO pgnoLV,
    const DATA& dataRec,
    FUCB * const pfucb,
    const TTMAP * const pttmapLVTree,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRFixOneRecord(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const DATA& dataRec,
    FUCB * const pfucb,
    TTMAP * const pttmapRecords,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRFixRecords(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const PGNO pgnoLV,
    TTMAP * const pttmapRecords,
    const TTMAP * const pttmapLVTree,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRFixLVRefcounts(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoLV,
    TTMAP * const pttmapRecords,
    TTMAP * const pttmapLVTree,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRFixupTable(
    PIB * const ppib,
    const IFMP ifmp,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRBuildAllIndexes(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB ** const ppfucb,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts );




const CHAR * const rgszLargeTables[] =
{
    "1-23",
    "1-24",
    "1-2F",
    "1-A",
    "1-D",
    "Folders",
    "Msg"
};
const INT cszLargeTables = sizeof( rgszLargeTables ) / sizeof( rgszLargeTables[0] );

INLINE INT CpgMinRepairSequentialPreread( ULONG cbPageSize )
{
    return ( 64 * 1024 ) / cbPageSize;
}

const OBJID objidInvalid = 0x7ffffffe;

INLINE TraceContextScope TcRepair()
{
    TraceContextScope tc( iortRepair );
    tc->nParentObjectClass = tceNone;
    return tc;
}

ERR ErrDBUTLRepair( JET_SESID sesid, const JET_DBUTIL_W *pdbutil, CPRINTF* const pcprintf )
{
    Assert( NULL != pdbutil->szDatabase );

    ERR         err     = JET_errSuccess;
    PIB * const ppib    = reinterpret_cast<PIB *>( sesid );
    INST * const pinst  = PinstFromPpib( ppib );

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = tceNone;
    tcScope->iorReason.SetIort( iortRepair );

    const WCHAR * const wszDatabase     = pdbutil->szDatabase;

    WCHAR wszFolder[ IFileSystemAPI::cchPathMax ];
    WCHAR wszPrefix[ IFileSystemAPI::cchPathMax ];
    WCHAR wszFileExt[ IFileSystemAPI::cchPathMax ];
    WCHAR wszFile[ IFileSystemAPI::cchPathMax ];

    const   INT cThreads            = min( 100, ( CUtilProcessProcessor() * 4 ) );

    CPRINTFFN   cprintfStdout( printf );

    if ( pdbutil->szIntegPrefix )
    {
        if ( LOSStrLengthW(pdbutil->szIntegPrefix) >= _countof(wszPrefix) )
        {
            CallR( ErrERRCheck( JET_errInvalidParameter ) );
        }
        OSStrCbCopyW( wszPrefix, sizeof( wszPrefix ),  pdbutil->szIntegPrefix );
    }
    else
    {
        CallR( pinst->m_pfsapi->ErrPathParse(   pdbutil->szDatabase,
                                                wszFolder,
                                                wszPrefix,
                                                wszFileExt ) );
    }

    if ( LOSStrLengthW(wszPrefix) + LOSStrLengthW(L".INTEG.RAW") >= _countof(wszFile) )
    {
        CallR( ErrERRCheck( JET_errInvalidParameter ) );
    }
    OSStrCbFormatW(wszFile, sizeof( wszFile ), L"%s%s", wszPrefix, L".INTEG.RAW" );
    CPRINTFFILE cprintfFile( wszFile );

    if ( pdbutil->grbitOptions & JET_bitDBUtilOptionStats )
    {
        if ( LOSStrLengthW(wszPrefix) + LOSStrLengthW(L".INTGINFO.TXT") >= _countof(wszFile) )
        {
            CallR( ErrERRCheck( JET_errInvalidParameter ) );
        }
        OSStrCbFormatW(wszFile, sizeof( wszFile ), L"%s%s", wszPrefix, L".INTGINFO.TXT" );
    }
    CPRINTF * const pcprintfStatsInternal = ( pdbutil->grbitOptions & JET_bitDBUtilOptionStats ) ?
                                    new CPRINTFFILE( wszFile ) :
                                    CPRINTFNULL::PcprintfInstance();
    if ( NULL == pcprintfStatsInternal )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    CPRINTFTLSPREFIX cprintf( pcprintf );
    CPRINTFTLSPREFIX cprintfVerbose( &cprintfFile );
    CPRINTFTLSPREFIX cprintfDebug( &cprintfFile );
    CPRINTFTLSPREFIX cprintfError( &cprintfFile, "ERROR: " );
    CPRINTFTLSPREFIX cprintfWarning( &cprintfFile, "WARNING: " );
    CPRINTFTLSPREFIX cprintfStats( pcprintfStatsInternal );

#ifdef REPAIR_DEBUG_CALLS
    g_pcprintfRepairDebugCalls = &cprintfError;
#endif

    JET_SNPROG  snprog;
    memset( &snprog, 0, sizeof( JET_SNPROG ) );
    snprog.cbStruct = sizeof( JET_SNPROG );

    REPAIROPTS  repairopts;
    repairopts.grbit            = pdbutil->grbitOptions;
    repairopts.pcprintf         = &cprintf;
    repairopts.pcprintfVerbose  = &cprintfVerbose;
    repairopts.pcprintfError    = &cprintfError;
    repairopts.pcprintfWarning  = &cprintfWarning;
    repairopts.pcprintfDebug    = &cprintfDebug;
    repairopts.pcprintfStats    = &cprintfStats;
    repairopts.pfnStatus        = pdbutil->pfnCallback ? (JET_PFNSTATUS)(pdbutil->pfnCallback) : ErrREPAIRNullStatusFN;
    repairopts.psnprog          = &snprog;

    const REPAIROPTS * const popts = &repairopts;


    REPAIRPrintStartMessages( wszDatabase, popts );


    BOOL            fHeaderNotClean             = fFalse;
    BOOL            fGlobalSpaceCorrupt         = fFalse;
    BOOL            fCatalogCorrupt             = fFalse;
    BOOL            fShadowCatalogCorrupt       = fFalse;
    BOOL            fMSObjidsCorrupt            = fFalse;
    BOOL            fMSLocalesCorrupt           = fFalse;
    BOOL            fTablesCorrupt              = fFalse;
    BOOL            fRepairedCatalog            = fFalse;

    BOOL            fOpened                     = fFalse;
    BOOL            fAttached                   = fFalse;

    REPAIRTABLE *   prepairtable                = NULL;
    IFMP            ifmp                        = 0xffffffff;
    CPG             cpgDatabase                 = 0;
    OBJID           objidLast                   = objidNil;
    PGNO            pgnoLastOE                  = pgnoNull;
    TASKMGR         taskmgr;

    INTEGGLOBALS    integglobalsTables;

    BOOL            fDbtimeTooLarge             = fFalse;

    LOGTIME         logtimeLastFullBackup;

    const ULONG     timerStart                  = TickOSTimeCurrent();

    BOOL            fGlobalRepairSave           = g_fRepair;

    memset( &logtimeLastFullBackup, 0, sizeof( logtimeLastFullBackup ) );


    Call( ErrREPAIRCheckHeader(
            pinst,
            wszDatabase,
            &logtimeLastFullBackup,
            &fHeaderNotClean,
            popts ) );


    if ( !( popts->grbit & JET_bitDBUtilOptionSuppressLogo )
        && !( popts->grbit & JET_bitDBUtilOptionDontRepair ) )
    {
        if ( !FUtilRepairIntegrityMsgBox( cwszRepairMsg ) )
        {
            Call( ErrERRCheck(JET_errInvalidParameter) );
        }
    }
    
    Call( ErrPIBOpenTempDatabase ( ppib ) );


    g_fRepair = fTrue;


    Call( ErrREPAIRAttachForIntegrity( sesid, wszDatabase, &ifmp, popts ) );
    fAttached   = fTrue;
    fOpened     = fTrue;


    err = ErrREPAIRCheckDatabaseSize( wszDatabase, ifmp, popts );
    if ( JET_errDatabaseCorrupted == err )
    {
        CheckRepairGrbit( &(repairopts.grbit) );

        if ( popts->grbit & JET_bitDBUtilOptionDontRepair )
        {
            (*popts->pcprintfVerbose)( "The database is the wrong size. Integrity check cannot continue\r\n"  );
            (VOID)INTEGRITYPrintEndErrorMessages( logtimeLastFullBackup, popts );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        else
        {
            Call( ErrREPAIRFixDatabaseSize( wszDatabase, ifmp, popts ) );
        }
    }
    Call( err );

    cpgDatabase = PgnoLast( ifmp );


    BFPrereadPageRange( ifmp, pgnoSystemRoot, min( 2048, cpgDatabase ), bfprfDefault, ppib->BfpriPriority( ifmp ), *TcRepair() );


    integglobalsTables.pttarrayOwnedSpace               = new TTARRAY( cpgDatabase + 1, objidSystemRoot );
    integglobalsTables.pttarrayAvailSpace               = new TTARRAY( cpgDatabase + 1, objidNil );
    integglobalsTables.ppgnocollShelved                 = new PgnoCollection( max( 1, min( 1024, cpgDatabase / 10000 ) ) );

    if ( NULL == integglobalsTables.pttarrayOwnedSpace
        || NULL == integglobalsTables.pttarrayAvailSpace
        || NULL == integglobalsTables.ppgnocollShelved )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    Call( integglobalsTables.pttarrayOwnedSpace->ErrInit( pinst ) );
    Call( integglobalsTables.pttarrayAvailSpace->ErrInit( pinst ) );


    (*popts->pcprintfVerbose)( "Creating %d threads\r\n", cThreads );
    Call( taskmgr.ErrInit( pinst, cThreads ) );


    snprog.cunitTotal   = cpgDatabase;
    snprog.cunitDone    = 0;
    (VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntBegin, NULL );


    Call( ErrREPAIRCheckGlobalSpaceTree(
            ppib,
            ifmp,
            &fGlobalSpaceCorrupt,
            &pgnoLastOE,
            integglobalsTables.pttarrayOwnedSpace,
            integglobalsTables.pttarrayAvailSpace,
            integglobalsTables.ppgnocollShelved,
            &fDbtimeTooLarge,
            popts ) );


    Call( ErrREPAIRCheckSystemTables(
            ppib,
            ifmp,
            &taskmgr,
            &fCatalogCorrupt,
            &fShadowCatalogCorrupt,
            integglobalsTables.pttarrayOwnedSpace,
            integglobalsTables.pttarrayAvailSpace,
            integglobalsTables.ppgnocollShelved,
            &fDbtimeTooLarge,
            popts ) );

    if ( !fCatalogCorrupt || !fShadowCatalogCorrupt )
    {
        BOOL fPgnoFDPMarkedAsAvail = fFalse;
        Call( ErrREPAIRCheckSystemTablesLogical(
            ppib,
            ifmp,
            &objidLast,
            &fCatalogCorrupt,
            &fShadowCatalogCorrupt,
            &fMSObjidsCorrupt,
            &fMSLocalesCorrupt,
            integglobalsTables.pttarrayAvailSpace,
            integglobalsTables.ppgnocollShelved,
            &fPgnoFDPMarkedAsAvail,
            popts ) );

        if ( fPgnoFDPMarkedAsAvail )
        {
            fGlobalSpaceCorrupt = fTrue;
        }
    }



    if ( fCatalogCorrupt || fShadowCatalogCorrupt )
    {
        CheckRepairGrbit( &(repairopts.grbit) );

        if ( popts->grbit & JET_bitDBUtilOptionDontRepair )
        {
            if ( fCatalogCorrupt )
            {
                (*popts->pcprintfVerbose)( "The catalog is corrupted. not all tables could be checked\r\n"  );
                (VOID)INTEGRITYPrintEndErrorMessages( logtimeLastFullBackup, popts );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
            else
            {
            }
        }
        else
        {


            Call( ErrREPAIRAttachForRepair( sesid, wszDatabase, &ifmp, 1, objidNil, popts ) );


            Call( ErrREPAIRRepairGlobalSpace( ppib, ifmp, popts ) );
            fGlobalSpaceCorrupt     = fFalse;

            (*popts->pcprintfVerbose)( "rebuilding catalogs\r\n"  );
            (*popts->pcprintfVerbose).Indent();

            Call( ErrREPAIRRepairCatalogs(
                        ppib,
                        ifmp,
                        &objidLast,
                        fCatalogCorrupt,
                        fShadowCatalogCorrupt,
                        popts ) );
            (*popts->pcprintfVerbose).Unindent();


            (VOID)ErrBFFlush( ifmp );

            if ( PgnoLast( ifmp ) != (SIZE_T)cpgDatabase )
            {
                (*popts->pcprintfVerbose)( "Database is now %d pages\r\n", cpgDatabase );
                cpgDatabase = PgnoLast( ifmp );
            }

            Call( ErrREPAIRRecheckSpaceTreeAndSystemTablesSpace(
                        ppib,
                        ifmp,
                        cpgDatabase,
                        &fGlobalSpaceCorrupt,
                        &(integglobalsTables.pttarrayOwnedSpace),
                        &(integglobalsTables.pttarrayAvailSpace),
                        &(integglobalsTables.ppgnocollShelved),
                        &fDbtimeTooLarge,
                        popts ) );

            fCatalogCorrupt         = fFalse;
            fShadowCatalogCorrupt   = fFalse;
            fRepairedCatalog        = fTrue;

            (VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntComplete, NULL );

            (*popts->pcprintf)( "\r\nChecking the database.\r\n"  );
            popts->psnprog->cunitTotal  = cpgDatabase;
            popts->psnprog->cunitDone   = 0;
            (VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntBegin, NULL );
        }
    }


    Call( ErrREPAIRStartCheckAllTables(
            ppib,
            ifmp,
            &taskmgr,
            &prepairtable,
            &fDbtimeTooLarge,
            &integglobalsTables,
            popts ) );


    Call( taskmgr.ErrTerm() );


    Call( ErrREPAIRStopCheckTables(
                &integglobalsTables,
                &fTablesCorrupt ) );
    if ( objidNil == objidLast )
    {
        (*popts->pcprintfWarning)( "objidLast is objidNil (%d)\r\n", objidNil );
        objidLast = objidNil + 1;
    }


    if ( !fGlobalSpaceCorrupt )
    {
        const PGNO pgnoLastPhysical     = cpgDatabase;
        const PGNO pgnoLastLogical      = pgnoLastOE;

        if ( pgnoLastPhysical < pgnoLastLogical )
        {
            (*popts->pcprintfError)( "Database file is too small (expected %d pages, file is %d pages)\r\n",
                                        pgnoLastLogical, pgnoLastPhysical );
            fGlobalSpaceCorrupt = fTrue;
        }
        else if ( pgnoLastPhysical > pgnoLastLogical )
        {


            (*popts->pcprintfWarning)( "Database file is too big (expected %d pages, file is %d pages)\r\n",
                                        pgnoLastLogical, pgnoLastPhysical );

            Call( ErrREPAIRCheckRange(
                    ppib,
                    integglobalsTables.pttarrayOwnedSpace,
                    integglobalsTables.ppgnocollShelved,
                    pgnoLastLogical + 1,
                    pgnoLastPhysical,
                    objidSystemRoot,
                    &fGlobalSpaceCorrupt,
                    popts ) );

            if ( fGlobalSpaceCorrupt )
            {
                (*popts->pcprintfError)( "Global space tree is too small (has %d pages, file is %d pages) and there objects which own space beyond the logical size of the file. "
                                         "The space tree will be rebuilt\r\n",
                                        pgnoLastLogical, pgnoLastPhysical );
            }
        }
    }


    (VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntComplete, NULL );
    (*popts->pcprintfVerbose).Unindent();


    Assert( !fCatalogCorrupt );


    if ( fHeaderNotClean ||
        fTablesCorrupt ||
        fGlobalSpaceCorrupt ||
        fRepairedCatalog ||
        fShadowCatalogCorrupt ||
        fDbtimeTooLarge )
    {
        CheckRepairGrbit( &(repairopts.grbit) );

        if ( !( popts->grbit & JET_bitDBUtilOptionDontRepair ) )
        {
            if ( integglobalsTables.fRepairDisallowed )
            {
                err = ErrERRCheck( JET_errDatabaseCorruptedNoRepair );
            }
            else
            {
                Call( ErrREPAIRRepairDatabase(
                        ppib,
                        wszDatabase,
                        &fAttached,
                        &ifmp,
                        objidLast,
                        pgnoLastOE,
                        prepairtable,
                        fRepairedCatalog,
                        fGlobalSpaceCorrupt,
                        fMSObjidsCorrupt,
                        fMSLocalesCorrupt,
                        integglobalsTables.pttarrayOwnedSpace,
                        integglobalsTables.pttarrayAvailSpace,
                        integglobalsTables.ppgnocollShelved,
                        popts ) );
                (*popts->pcprintfVerbose)( "\r\nRepair completed. Database corruption has been repaired!\r\n\n" );
                (*popts->pcprintf)( "\r\nRepair completed. Database corruption has been repaired!\r\n\n" );
                err = ErrERRCheck( JET_wrnDatabaseRepaired );
            }
        }
        else
        {
            (*popts->pcprintfVerbose)( "\r\nIntegrity check completed. Database is CORRUPTED!\r\n" );
            (VOID)INTEGRITYPrintEndErrorMessages( logtimeLastFullBackup, popts );
            err = ErrERRCheck( JET_errDatabaseCorrupted );
        }
    }
    else
    {
        (*popts->pcprintfVerbose)( "\r\nIntegrity check successful.\r\n\r\n" );
        (*popts->pcprintf)( "\r\nIntegrity check successful.\r\n\r\n" );
    }

HandleError:
    if ( fAttached && fOpened )
    {
        (VOID)ErrIsamCloseDatabase( (JET_SESID)ppib, (JET_DBID)ifmp, 0 );
    }

    if ( fAttached )
    {
        (VOID)ErrIsamDetachDatabase( (JET_SESID)ppib, NULL, wszDatabase );
    }

    CallS( taskmgr.ErrTerm() );

    (*popts->pcprintfVerbose).Unindent();

    REPAIRPrintEndMessages( wszDatabase, timerStart, popts );

    REPAIRFreeRepairtables( prepairtable );

    delete integglobalsTables.pttarrayOwnedSpace;
    delete integglobalsTables.pttarrayAvailSpace;
    delete integglobalsTables.ppgnocollShelved;
    integglobalsTables.pttarrayOwnedSpace = NULL;
    integglobalsTables.pttarrayAvailSpace = NULL;
    integglobalsTables.ppgnocollShelved = NULL;

    if ( pdbutil->grbitOptions & JET_bitDBUtilOptionStats )
    {
        delete pcprintfStatsInternal;
    }

    g_fRepair = fGlobalRepairSave;

    if ( !( popts->grbit & JET_bitDBUtilOptionDontRepair ) )
    {
        OSDiagTrackRepair( err );
    }

    return err;
}

LOCAL VOID CheckRepairGrbit( JET_GRBIT * pgrbit )
{
    if ( (*pgrbit) & JET_bitDBUtilOptionRepairCheckOnly )
    {
        Assert( !( (*pgrbit) & JET_bitDBUtilOptionDontRepair ) );

        if ( !FUtilRepairIntegrityMsgBox( cwszRepairCheckContinueMsg ) )
        {
            (*pgrbit) |= JET_bitDBUtilOptionDontRepair;
        }
        (*pgrbit) &= ~JET_bitDBUtilOptionRepairCheckOnly;
    }
}

BOOL FIsLargeTable( const CHAR * const szTable )
{
#ifdef DEBUG


    static INT fChecked = fFalse;
    if ( !fChecked )
    {
        INT iszT;
        for( iszT = 0; iszT < cszLargeTables - 1; ++iszT )
        {
            AssertSz( _stricmp( rgszLargeTables[iszT], rgszLargeTables[iszT+1] ) < 0, "rgszLargeTables is out of order" );
        }
        fChecked = fTrue;
    }
#endif

    INT isz;
    for( isz = 0; isz < cszLargeTables; ++isz )
    {
        const INT cmp = _stricmp( szTable, rgszLargeTables[isz] );
        if ( 0 == cmp )
        {
            return fTrue;
        }
        if ( cmp < 0 )
        {
            return fFalse;
        }
    }
    return fFalse;
}


LOCAL PGNO PgnoLast( const IFMP ifmp )
{
    return g_rgfmp[ifmp].PgnoLast();
}


LOCAL VOID REPAIRIPrereadIndexesOfFCB( const PIB * const ppib, const FCB * const pfcb )
{
    const INT cSecondaryIndexesToPreread = 64;

    PGNO rgpgno[cSecondaryIndexesToPreread + 1];
    INT ipgno = 0;
    const FCB * pfcbT = pfcb->PfcbNextIndex();

    while ( pfcbNil != pfcbT && ipgno < cSecondaryIndexesToPreread )
    {
        rgpgno[ipgno++] = pfcbT->PgnoFDP();
        pfcbT = pfcbT->PfcbNextIndex();
    }
    rgpgno[ipgno] = pgnoNull;

    BFPrereadPageList( pfcb->Ifmp(), rgpgno, bfprfDefault, ppib->BfpriPriority( pfcb->Ifmp() ), *TcRepair() );
}


LOCAL VOID REPAIRDumpStats(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const BTSTATS * const pbtstats,
    const REPAIROPTS * const popts )
{
    if ( !(popts->grbit & JET_bitDBUtilOptionStats ) )
    {
        return;
    }

    (*popts->pcprintfStats)( "total pages: %I64d\r\n", pbtstats->cpageInternal+pbtstats->cpageLeaf );
    (*popts->pcprintfStats).Indent();
    (*popts->pcprintfStats)( "internal pages: %I64d\r\n", pbtstats->cpageInternal );
    (*popts->pcprintfStats)( "leaf pages: %I64d\r\n", pbtstats->cpageLeaf );
    (*popts->pcprintfStats).Unindent();
    (*popts->pcprintfStats)( "tree height: %I64d\r\n", pbtstats->cpageDepth );
    (*popts->pcprintfStats)( "empty pages: %I64d\r\n", pbtstats->cpageEmpty );

    QWORD cnodeInternal = 0;
    QWORD cnodeLeaf     = 0;

    QWORD cbKeyTotal    = 0;
    QWORD cbKeySuffixes = 0;
    QWORD cbKeyPrefixes = 0;

    INT ikey;
    for( ikey = 0; ikey < cbKeyAlloc; ikey++ )
    {
        cnodeInternal += pbtstats->rgckeyInternal[ikey];
        cnodeLeaf     += pbtstats->rgckeyLeaf[ikey];

        cbKeyTotal  += pbtstats->rgckeyInternal[ikey] * ikey;
        cbKeyTotal  += pbtstats->rgckeyLeaf[ikey] * ikey;

        cbKeySuffixes   += pbtstats->rgckeySuffixInternal[ikey] * ikey;
        cbKeySuffixes   += pbtstats->rgckeySuffixLeaf[ikey] * ikey;

        cbKeyPrefixes   += pbtstats->rgckeyPrefixInternal[ikey] * ikey;
        cbKeyPrefixes   += pbtstats->rgckeyPrefixLeaf[ikey] * ikey;
    }


    const QWORD cbSavings = cbKeyTotal - ( cbKeySuffixes + cbKeyPrefixes + ( pbtstats->cnodeCompressed * 2 ) );

    (*popts->pcprintfStats)( "total nodes: %I64d\r\n", cnodeInternal+cnodeLeaf );
    (*popts->pcprintfStats)( "compressed nodes: %I64d\r\n", pbtstats->cnodeCompressed );
    (*popts->pcprintfStats)( "space saved: %I64d\r\n", cbSavings );
    (*popts->pcprintfStats)( "internal nodes: %I64d\r\n", cnodeInternal );
    (*popts->pcprintfStats)( "leaf nodes: %I64d\r\n", cnodeLeaf );
    (*popts->pcprintfStats).Indent();
    (*popts->pcprintfStats)( "versioned: %I64d\r\n", pbtstats->cnodeVersioned );
    (*popts->pcprintfStats)( "deleted: %I64d\r\n", pbtstats->cnodeDeleted );
    (*popts->pcprintfStats).Unindent();
}


LOCAL VOID REPAIRDumpLVStats(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const LVSTATS * const plvstats,
    const REPAIROPTS * const popts )
{
    if ( !(popts->grbit & JET_bitDBUtilOptionStats ) )
    {
        return;
    }

    (*popts->pcprintfStats)( "number of LVs: %I64d\r\n", plvstats->clv );
    (*popts->pcprintfStats).Indent();
    (*popts->pcprintfStats)( "multi-ref LVs: %I64d\r\n", plvstats->clvMultiRef );
    (*popts->pcprintfStats)( "unreferenced LVs: %I64d\r\n", plvstats->clvNoRef );
    (*popts->pcprintfStats)( "largest refcount: %I64d\r\n", plvstats->referenceMax );
    (*popts->pcprintfStats)( "total refcounts: %I64d\r\n", plvstats->referenceTotal );
    (*popts->pcprintfStats)( "smallest LID: 0x%I64x\r\n", plvstats->lidMin );
    (*popts->pcprintfStats)( "largest LID: 0x%I64x\r\n", plvstats->lidMax );
    (*popts->pcprintfStats)( "fragmented LV roots: %I64d\r\n", plvstats->clvFragmentedRoot );
    (*popts->pcprintfStats)( "unfragmented LV roots: %I64d\r\n", plvstats->clvUnfragmentedRoot );
    (*popts->pcprintfStats).Unindent();
    (*popts->pcprintfStats)( "total LV bytes: %I64d\r\n", plvstats->cbLVTotal );
    (*popts->pcprintfStats).Indent();
    (*popts->pcprintfStats)( "largest LV: %I64d\r\n", plvstats->cbLVMax );
    (*popts->pcprintfStats)( "smallest LV: %I64d\r\n", plvstats->cbLVMin );
    (*popts->pcprintfStats)( "multi-ref LV bytes: %I64d\r\n", plvstats->cbLVMultiRefTotal );
    (*popts->pcprintfStats).Unindent();
}


LOCAL_BROKEN VOID REPAIRDumpHex( __out_bcount(cbDest) CHAR * const szDest, INT cbDest, __in_bcount(cb) const BYTE * const pb, const INT cb )
{
    const BYTE * const pbMax = pb + cb;
    const BYTE * pbT = pb;

    CHAR * sz = szDest;

    while ( pbT < pbMax && cbDest > 0 )
    {
        OSStrCbFormatA( sz, cbDest, "%2.2x", *pbT++ );

        const INT cbCurrentLength = strlen(sz);
        cbDest -= cbCurrentLength;
        sz += cbCurrentLength;

        if ( pbT != pbMax && cbDest > 0 )
        {
            OSStrCbCopyA( sz, cbDest, " " );

            const INT cbCurrentAppend = strlen(sz);
            Assert( 1 == cbCurrentAppend );
            cbDest -= cbCurrentAppend;
            sz += cbCurrentAppend;
        }
    }
}


LOCAL VOID REPAIRPrintSig( const SIGNATURE * const psig, CPRINTF * const pcprintf )
{
    (*pcprintf)( "Create time:" );
    DUMPPrintSig( pcprintf, psig );
    (*pcprintf)( "\r\n" );
}


LOCAL VOID REPAIRPrintStartMessages( const WCHAR * const wszDatabase, const REPAIROPTS * const popts )
{
    (*popts->pcprintfStats)( "***** %s of database '%ws' started [%ws version %02d.%02d.%04d.%04d, (%ws)]\r\n\r\n",
                ( popts->grbit & JET_bitDBUtilOptionDontRepair ) ? "Integrity check" : "Repair",
                wszDatabase,
                WszUtilImageVersionName(),
                DwUtilImageVersionMajor(),
                DwUtilImageVersionMinor(),
                DwUtilImageBuildNumberMajor(),
                DwUtilImageBuildNumberMinor(),
                WszUtilImageBuildClass()
                );

    (*popts->pcprintfVerbose)( "***** %s of database '%ws' started [%ws version %02d.%02d.%04d.%04d, (%ws)]\r\n\r\n",
                ( popts->grbit & JET_bitDBUtilOptionDontRepair ) ? "Integrity check" : "Repair",
                wszDatabase,
                WszUtilImageVersionName(),
                DwUtilImageVersionMajor(),
                DwUtilImageVersionMinor(),
                DwUtilImageBuildNumberMajor(),
                DwUtilImageBuildNumberMinor(),
                WszUtilImageBuildClass()
                );

    (*popts->pcprintfVerbose)( "search for \'ERROR:\' to find errors\r\n" );
    (*popts->pcprintfVerbose)( "search for \'WARNING:\' to find warnings\r\n" );

    (*popts->pcprintfVerbose).Indent();

    (*popts->pcprintf)( "\r\nChecking database integrity.\r\n"  );
}


LOCAL ERR ErrREPAIRCheckDatabaseSize(
    const WCHAR * const wszDatabase,
    const IFMP ifmp,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    QWORD cbDatabase;
    QWORD cbDatabaseOnDisk;

    Call( g_rgfmp[ifmp].Pfapi()->ErrSize( &cbDatabase, IFileAPI::filesizeLogical ) );
    if ( 0 != cbDatabase % g_cbPage )
    {
        (*popts->pcprintfError)( "database file \"%ws\" has the wrong size (%I64d bytes, must be a multiple of %d bytes)\r\n",
                                        wszDatabase, cbDatabase, g_cbPage );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }
    else
    {
        (*popts->pcprintfVerbose)( "database file \"%ws\" is %I64d bytes\r\n",
                                        wszDatabase, cbDatabase );
    }

    Call( g_rgfmp[ifmp].Pfapi()->ErrSize( &cbDatabaseOnDisk, IFileAPI::filesizeOnDisk ) );
    if ( 0 != cbDatabase % g_cbPage )
    {
        (*popts->pcprintfError)( "database file \"%ws\" has the wrong size on disk (%I64d bytes, must be a multiple of %d bytes)\r\n",
                                        wszDatabase, cbDatabaseOnDisk, g_cbPage );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }
    else
    {
        (*popts->pcprintfVerbose)( "database file \"%ws\" is %I64d bytes on disk.\r\n",
                                        wszDatabase, cbDatabaseOnDisk );
    }

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRFixDatabaseSize(
    const WCHAR * const wszDatabase,
    const IFMP ifmp,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    QWORD cbDatabase;
    Call( g_rgfmp[ifmp].Pfapi()->ErrSize( &cbDatabase, IFileAPI::filesizeLogical ) );
    const CPG   cpgNew          = CPG( cbDatabase / (QWORD)g_cbPage );
    const QWORD cbDatabaseNew   = (QWORD)g_cbPage * (QWORD)cpgNew;

    (*popts->pcprintfVerbose)( "setting database file \"%ws\" size (was %I64d bytes, will be %I64d bytes -- %d pages)\r\n",
                            wszDatabase, cbDatabase, cbDatabaseNew, cpgNew );

    Call( ErrIONewSize( ifmp, *TraceContextScope( iorpNone, iorsNone, iortRepair ), cpgNew - cpgDBReserved, 0, JET_bitNil ) );
    g_rgfmp[ifmp].SetOwnedFileSize( cbDatabaseNew );

    if ( JET_errSuccess == err )
    {
        Assert( JET_errSuccess == ErrREPAIRCheckDatabaseSize( wszDatabase, ifmp, popts ) );
    }

HandleError:
    return err;
}


LOCAL VOID REPAIRFreeRepairtables( REPAIRTABLE * const prepairtable )
{
    REPAIRTABLE * prepairtableT = prepairtable;

    while ( prepairtableT )
    {
        REPAIRTABLE * const prepairtableNext = prepairtableT->prepairtableNext;
        prepairtableT->~REPAIRTABLE();
        OSMemoryHeapFree( prepairtableT );
        prepairtableT = prepairtableNext;
    }
}


LOCAL VOID REPAIRPrintEndMessages(
            const WCHAR * const wszDatabase,
            const ULONG timerStart,
            const REPAIROPTS * const popts )
{
    const ULONG timerEnd = TickOSTimeCurrent();
    (*popts->pcprintfStats)( "\r\n\r\n" );
    (*popts->pcprintfStats)( "***** Eseutil completed in %d seconds\r\n\r\n",
                ( ( timerEnd - timerStart ) / 1000 ) );
    (*popts->pcprintfVerbose)( "\r\n\r\n" );
    (*popts->pcprintfVerbose)( "***** Eseutil completed in %d seconds\r\n\r\n",
                ( ( timerEnd - timerStart ) / 1000 ) );
}


LOCAL VOID INTEGRITYPrintEndErrorMessages(
    const LOGTIME logtimeLastFullBackup,
    const REPAIROPTS * const popts )
{
    (*popts->pcprintf)( "\r\nIntegrity check completed.  " );
    if ( 0 == logtimeLastFullBackup.bYear && 0 == logtimeLastFullBackup.bMonth &&
        0 == logtimeLastFullBackup.bDay && 0 == logtimeLastFullBackup.bHours &&
        0 == logtimeLastFullBackup.bMinutes && 0 == logtimeLastFullBackup.bSeconds )
    {
        (*popts->pcprintf)( "Database is CORRUPTED!\r\n" );
    }
    else
    {
        (*popts->pcprintfVerbose)( "\r\nThe last full backup of this database was on " );
        DUMPPrintLogTime( popts->pcprintfVerbose, &logtimeLastFullBackup );
        (*popts->pcprintfVerbose)( "\r\n" );
        (*popts->pcprintf)( "\r\nDatabase is CORRUPTED, the last full backup of this database was on " );
        DUMPPrintLogTime( popts->pcprintf, &logtimeLastFullBackup );
        (*popts->pcprintf)( "\r\n" );
    }
}


LOCAL JET_ERR __stdcall ErrREPAIRNullStatusFN( JET_SESID, JET_SNP, JET_SNT, void * )
{
    return JET_errSuccess;
}


LOCAL ERR ErrREPAIRCheckHeader(
    INST * const pinst,
    const WCHAR * const wszDatabase,
    LOGTIME * const plogtimeLastFullBackup,
    BOOL * const pfHeaderNotClean,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    *pfHeaderNotClean = fFalse;

    DBFILEHDR_FIX * const pdbfilehdr = reinterpret_cast<DBFILEHDR_FIX * >( PvOSMemoryPageAlloc( g_cbPage, NULL ) );
    if ( NULL == pdbfilehdr )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    (*popts->pcprintfVerbose)( "checking database header\r\n" );
    (*popts->pcprintfVerbose).Unindent();

    err = ErrUtilReadShadowedHeader(    pinst,
                                        pinst->m_pfsapi,
                                        wszDatabase,
                                        reinterpret_cast<BYTE*>( pdbfilehdr ),
                                        g_cbPage,
                                        OffsetOf( DBFILEHDR_FIX, le_cbPageSize ),
                                        ( popts->grbit & JET_bitDBUtilOptionDontRepair ) ? urhfReadOnly : urhfNone );
    if ( err < 0 )
    {
        if ( FErrIsDbHeaderCorruption( err ) )
        {
            (*popts->pcprintfError)( "unable to read database header\r\n" );
            err = ErrERRCheck( JET_errDatabaseCorrupted );
        }
        goto HandleError;
    }

    if ( attribDb != pdbfilehdr->le_attrib )
    {
        (*popts->pcprintfError)( "'%ws' is not a database file (attrib is %d, expected %d)\r\n",
                                    wszDatabase, (LONG)pdbfilehdr->le_attrib, attribDb );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( JET_dbstateCleanShutdown != pdbfilehdr->Dbstate() )
    {
        const CHAR * szState = SzFromState( pdbfilehdr->Dbstate() );
        switch ( pdbfilehdr->Dbstate() )
        {
            case JET_dbstateDirtyShutdown:
            case JET_dbstateIncrementalReseedInProgress:
            case JET_dbstateDirtyAndPatchedShutdown:
            case JET_dbstateRevertInProgress:
                break;
            case JET_dbstateJustCreated:
            case JET_dbstateBeingConverted:
                err = ErrERRCheck( JET_errDatabaseCorrupted );
                break;
            default:
                szState = "unknown";
                err = ErrERRCheck( JET_errDatabaseCorrupted );
                break;
        }
        (*popts->pcprintfError)( "database was not shutdown cleanly (%s)\r\n", szState );

        if ( JET_dbstateDirtyShutdown == pdbfilehdr->Dbstate()
            || JET_dbstateIncrementalReseedInProgress == pdbfilehdr->Dbstate()
            || JET_dbstateDirtyAndPatchedShutdown == pdbfilehdr->Dbstate() 
            || JET_dbstateRevertInProgress == pdbfilehdr->Dbstate() )
        {
            (*popts->pcprintf)( "\r\nThe database is not up-to-date. This operation may find that\r\n");
            (*popts->pcprintf)( "this database is corrupt because data from the log files has\r\n");
            (*popts->pcprintf)( "yet to be placed in the database.\r\n\n" );
            (*popts->pcprintf)( "To ensure the database is up-to-date please use the 'Recovery' operation.\r\n\n" );

            if ( ( popts->grbit & JET_bitDBUtilOptionDontRepair )
                && !( popts->grbit & JET_bitDBUtilOptionSuppressLogo ) )
            {
                if ( FUtilRepairIntegrityMsgBox( cwszIntegrityCheckQuitInconsistentMsg ) )
                {
                    Call( ErrERRCheck(JET_errDatabaseInconsistent) );
                }
            }
        }

            *pfHeaderNotClean = fTrue;
    }


    if ( NULL != plogtimeLastFullBackup )
    {
        *plogtimeLastFullBackup = pdbfilehdr->bkinfoFullPrev.logtimeMark;
    }

HandleError:
    (*popts->pcprintfVerbose).Unindent();
    OSMemoryPageFree( pdbfilehdr );
    return err;
}


LOCAL ERR ErrREPAIRCheckSystemTables(
    PIB * const ppib,
    const IFMP ifmp,
    TASKMGR * const ptaskmgr,
    BOOL * const pfCatalogCorrupt,
    BOOL * const pfShadowCatalogCorrupt,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;
    FUCB * pfucbCatalog = pfucbNil;

    const FIDLASTINTDB fidLastInTDB = { fidMSO_FixedLast, fidMSO_VarLast, fidMSO_TaggedLast };

    (*popts->pcprintfVerbose)( "checking system tables\r\n" );
    (*popts->pcprintfVerbose).Indent();

    RECCHECKNULL    recchecknull;
    RECCHECKTABLE   recchecktable( objidNil, pfucbNil, fidLastInTDB, NULL, popts );

    *pfCatalogCorrupt       = fTrue;
    *pfShadowCatalogCorrupt = fTrue;

    INTEGGLOBALS    integglobalsCatalog;
    INTEGGLOBALS    integglobalsShadowCatalog;

    integglobalsCatalog.fCorruptionSeen             = integglobalsShadowCatalog.fCorruptionSeen             = fFalse;
    integglobalsCatalog.err                         = integglobalsShadowCatalog.err                         = JET_errSuccess;
    integglobalsCatalog.pprepairtable               = integglobalsShadowCatalog.pprepairtable               = NULL;
    integglobalsCatalog.pttarrayOwnedSpace          = integglobalsShadowCatalog.pttarrayOwnedSpace          = pttarrayOwnedSpace;
    integglobalsCatalog.pttarrayAvailSpace          = integglobalsShadowCatalog.pttarrayAvailSpace          = pttarrayAvailSpace;
    integglobalsCatalog.ppgnocollShelved            = integglobalsShadowCatalog.ppgnocollShelved            = ppgnocollShelved;
    integglobalsCatalog.pfDbtimeTooLarge            = integglobalsShadowCatalog.pfDbtimeTooLarge            = pfDbtimeTooLarge;
    integglobalsCatalog.popts                       = integglobalsShadowCatalog.popts                       = popts;

    CHECKTABLE checktableMSO;
    CHECKTABLE checktableMSOShadow;
    CHECKTABLE checktableMSO_NameIndex;
    CHECKTABLE checktableMSO_RootObjectIndex;


    checktableMSO.ifmp                          = ifmp;
    OSStrCbCopyA(checktableMSO.szTable, sizeof(checktableMSO.szTable), szMSO );
    checktableMSO.szIndex[0]                    = 0;
    checktableMSO.objidFDP                      = objidFDPMSO;
    checktableMSO.pgnoFDP                       = pgnoFDPMSO;
    checktableMSO.objidParent                   = objidSystemRoot;
    checktableMSO.pgnoFDPParent                 = pgnoSystemRoot;
    checktableMSO.fPageFlags                    = CPAGE::fPagePrimary;
    checktableMSO.fUnique                       = fTrue;
    checktableMSO.preccheck                     = &recchecktable;
    checktableMSO.cpgPrimaryExtent              = 16;
    checktableMSO.pglobals                      = &integglobalsCatalog;
    checktableMSO.fDeleteWhenDone               = fFalse;

    Call( ptaskmgr->ErrPostTask( REPAIRCheckTreeAndSpaceTask, (ULONG_PTR)&checktableMSO ) );
    (*popts->pcprintfVerbose)( "%s %s\r\n", checktableMSO.szTable, checktableMSO.szIndex  );


    checktableMSOShadow.ifmp                    = ifmp;
    OSStrCbCopyA(checktableMSOShadow.szTable, sizeof(checktableMSOShadow.szTable), szMSOShadow );
    checktableMSOShadow.szIndex[0]              = 0;
    checktableMSOShadow.objidFDP                = objidFDPMSOShadow;
    checktableMSOShadow.pgnoFDP                 = pgnoFDPMSOShadow;
    checktableMSOShadow.objidParent             = objidSystemRoot;
    checktableMSOShadow.pgnoFDPParent           = pgnoSystemRoot;
    checktableMSOShadow.fPageFlags              = CPAGE::fPagePrimary;
    checktableMSOShadow.fUnique                 = fTrue;
    checktableMSOShadow.preccheck               = &recchecktable;
    checktableMSOShadow.cpgPrimaryExtent        = 16;
    checktableMSOShadow.pglobals                = &integglobalsShadowCatalog;
    checktableMSOShadow.fDeleteWhenDone         = fFalse;

    Call( ptaskmgr->ErrPostTask( REPAIRCheckTreeAndSpaceTask, (ULONG_PTR)&checktableMSOShadow ) );
    (*popts->pcprintfVerbose)( "%s %s\r\n", checktableMSOShadow.szTable, checktableMSOShadow.szIndex  );


    checktableMSO.signal.Wait();
    Call( integglobalsCatalog.err );

    if ( !integglobalsCatalog.fCorruptionSeen )
    {


        checktableMSO_NameIndex.ifmp                = ifmp;
        OSStrCbCopyA(checktableMSO_NameIndex.szTable, sizeof(checktableMSO_NameIndex.szTable), szMSO );
        OSStrCbCopyA(checktableMSO_NameIndex.szIndex, sizeof(checktableMSO_NameIndex.szIndex), szMSONameIndex );
        checktableMSO_NameIndex.objidFDP            = objidFDPMSO_NameIndex;
        checktableMSO_NameIndex.pgnoFDP             = pgnoFDPMSO_NameIndex;
        checktableMSO_NameIndex.objidParent         = objidFDPMSO;
        checktableMSO_NameIndex.pgnoFDPParent       = pgnoFDPMSO;
        checktableMSO_NameIndex.fPageFlags          = CPAGE::fPageIndex;
        checktableMSO_NameIndex.fUnique             = fTrue;
        checktableMSO_NameIndex.preccheck           = &recchecknull;
        checktableMSO_NameIndex.cpgPrimaryExtent    = 16;
        checktableMSO_NameIndex.pglobals            = &integglobalsCatalog;
        checktableMSO_NameIndex.fDeleteWhenDone     = fFalse;

        Call( ptaskmgr->ErrPostTask( REPAIRCheckTreeAndSpaceTask, (ULONG_PTR)&checktableMSO_NameIndex ) );
        (*popts->pcprintfVerbose)( "%s %s\r\n", checktableMSO_NameIndex.szTable, checktableMSO_NameIndex.szIndex  );


        checktableMSO_RootObjectIndex.ifmp              = ifmp;
        OSStrCbCopyA(checktableMSO_RootObjectIndex.szTable, sizeof(checktableMSO_RootObjectIndex.szTable), szMSO );
        OSStrCbCopyA(checktableMSO_RootObjectIndex.szIndex, sizeof(checktableMSO_RootObjectIndex.szIndex), szMSORootObjectsIndex );
        checktableMSO_RootObjectIndex.objidFDP          = objidFDPMSO_RootObjectIndex;
        checktableMSO_RootObjectIndex.pgnoFDP           = pgnoFDPMSO_RootObjectIndex;
        checktableMSO_RootObjectIndex.objidParent       = objidFDPMSO;
        checktableMSO_RootObjectIndex.pgnoFDPParent     = pgnoFDPMSO;
        checktableMSO_RootObjectIndex.fPageFlags        = CPAGE::fPageIndex;
        checktableMSO_RootObjectIndex.fUnique           = fTrue;
        checktableMSO_RootObjectIndex.preccheck         = &recchecknull;
        checktableMSO_RootObjectIndex.cpgPrimaryExtent  = 16;
        checktableMSO_RootObjectIndex.pglobals          = &integglobalsCatalog;
        checktableMSO_RootObjectIndex.fDeleteWhenDone   = fFalse;

        Call( ptaskmgr->ErrPostTask( REPAIRCheckTreeAndSpaceTask, (ULONG_PTR)&checktableMSO_RootObjectIndex ) );
        (*popts->pcprintfVerbose)( "%s %s\r\n", checktableMSO_RootObjectIndex.szTable, checktableMSO_RootObjectIndex.szIndex  );


        checktableMSO_NameIndex.signal.Wait();
        checktableMSO_RootObjectIndex.signal.Wait();

        Call( integglobalsCatalog.err );
    }


    checktableMSOShadow.signal.Wait();
    Call( integglobalsShadowCatalog.err );


    *pfShadowCatalogCorrupt = integglobalsShadowCatalog.fCorruptionSeen;


    if ( !integglobalsCatalog.fCorruptionSeen )
    {
        (*popts->pcprintfVerbose)( "rebuilding and comparing indexes\r\n" );
        Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );
        Assert( pfucbCatalog->u.pfcb->PfcbNextIndex() != pfcbNil );
        DIRUp( pfucbCatalog );

#ifdef PARALLEL_BATCH_INDEX_BUILD
        ULONG   cIndexes    = 0;
        for ( FCB * pfcbT = pfucbCatalog->u.pfcb->PfcbNextIndex(); pfcbNil != pfcbT; pfcbT = pfcbT->PfcbNextIndex() )
        {
            cIndexes++;
        }
#else
        const ULONG cIndexes    = cFILEIndexBatchSizeDefault;
#endif

        err = ErrFILEBuildAllIndexes(
                ppib,
                pfucbCatalog,
                pfucbCatalog->u.pfcb->PfcbNextIndex(),
                NULL,
                cIndexes,
                fTrue,
                popts->pcprintfError );

        if ( JET_errSuccess != err )
        {
            *pfCatalogCorrupt = fTrue;
            err = JET_errSuccess;
            goto HandleError;
        }


        *pfCatalogCorrupt       = fFalse;
    }


HandleError:
    (*popts->pcprintfVerbose).Unindent();

    if ( pfucbNil != pfucbCatalog )
    {
        DIRClose( pfucbCatalog );
    }

    return err;
}



LOCAL ERR ErrREPAIRRetrieveCatalogColumns(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB * pfucbCatalog,
    ENTRYINFO * const pentryinfo,
    const REPAIROPTS * const popts )
{
    const INT   cColumnsToRetrieve      = 12;
    JET_RETRIEVECOLUMN  rgretrievecolumn[ cColumnsToRetrieve ];
    ERR                 err             = JET_errSuccess;
    INT                 iretrievecolumn = 0;

    memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );


    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_ObjidTable;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( pentryinfo->objidTable );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( pentryinfo->objidTable );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Type;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( pentryinfo->objType );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( pentryinfo->objType );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Id;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( pentryinfo->objidFDP );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( pentryinfo->objidFDP );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_PgnoFDP;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( pentryinfo->pgnoFDPORColType );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( pentryinfo->pgnoFDPORColType );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Flags;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( pentryinfo->dwFlags );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( pentryinfo->dwFlags );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_RecordOffset;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( pentryinfo->ibRecordOffset );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( pentryinfo->ibRecordOffset );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Name;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)( pentryinfo->szName );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( pentryinfo->szName );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_TemplateTable;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)( pentryinfo->szTemplateTblORCallback );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( pentryinfo->szTemplateTblORCallback );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_KeyFldIDs;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)( pentryinfo->rgbIdxseg );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( pentryinfo->rgbIdxseg );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_LocaleName;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *) ( pentryinfo->wszLocaleName );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( pentryinfo->wszLocaleName );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Version;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *) &( pentryinfo->qwSortVersion );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( pentryinfo->qwSortVersion );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_SortID;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *) &( pentryinfo->sortid );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( pentryinfo->sortid );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    Assert( iretrievecolumn == _countof( rgretrievecolumn ) );

    pentryinfo->szName[ 0 ] = 0;
    pentryinfo->wszLocaleName[ 0 ] = L'\0';

    Call( ErrIsamRetrieveColumns(
                (JET_SESID)ppib,
                (JET_TABLEID)pfucbCatalog,
                rgretrievecolumn,
                iretrievecolumn ) );


    pentryinfo->szName[ sizeof( pentryinfo->szName ) - 1] = 0;
    pentryinfo->wszLocaleName[ _countof( pentryinfo->wszLocaleName ) - 1] = L'\0';

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRInsertIntoTemplateInfoList(
    TEMPLATEINFOLIST ** ppTemplateInfoList,
    const CHAR * szTemplateTable,
    INFOLIST * const pInfoList,
    const REPAIROPTS * const popts )
{
    ERR                     err             = JET_errSuccess;
    TEMPLATEINFOLIST    *   pTemplateInfo;
    TEMPLATEINFOLIST    *   pTemp           = *ppTemplateInfoList;

    Alloc( pTemplateInfo = new TEMPLATEINFOLIST );

    memset( pTemplateInfo, 0, sizeof(TEMPLATEINFOLIST) );

    Assert( strlen( szTemplateTable ) <= JET_cbNameMost );
    OSStrCbCopyA( pTemplateInfo->szTemplateTableName, sizeof(pTemplateInfo->szTemplateTableName), szTemplateTable);

    pTemplateInfo->pColInfoList = pInfoList;
    pTemplateInfo->pTemplateInfoListNext = NULL;

    if ( NULL == pTemp )
    {
        *ppTemplateInfoList = pTemplateInfo;
    }
    else
    {
        while ( pTemp->pTemplateInfoListNext )
        {
            pTemp = pTemp->pTemplateInfoListNext;
        }
        pTemp->pTemplateInfoListNext = pTemplateInfo;
    }

HandleError:
    return err;
}


LOCAL VOID REPAIRUtilCleanInfoList( INFOLIST **ppInfo )
{
    INFOLIST *pTemp = *ppInfo;

    while ( pTemp )
    {
        pTemp = pTemp->pInfoListNext;
        delete *ppInfo;
        *ppInfo = pTemp;
    }
}

LOCAL VOID REPAIRUtilCleanInfoLists( INFOLIST * rgpInfo[], const INT cpInfo )
{
    for( INT ipInfo = 0; ipInfo < cpInfo; ++ipInfo )
    {
        REPAIRUtilCleanInfoList( rgpInfo + ipInfo );
    }
}


LOCAL VOID REPAIRUtilCleanTemplateInfoList( TEMPLATEINFOLIST **ppTemplateInfoList )
{
    TEMPLATEINFOLIST *pTemp = *ppTemplateInfoList;

    while ( pTemp )
    {
        pTemp = pTemp->pTemplateInfoListNext;
        REPAIRUtilCleanInfoList( &((*ppTemplateInfoList)->pColInfoList) );
        delete *ppTemplateInfoList;
        *ppTemplateInfoList = pTemp;
    }
}

LOCAL ERR ErrREPAIRCheckOneColumnLogical(
    PIB * const ppib,
    FUCB * const pfucbCatalog,
    const ENTRYINFO entryinfo,
    const ULONG objidTable,
    const REPAIROPTS * const popts )
{
    ERR             err         = JET_errSuccess;

#ifdef DEAD_CODE
    if ( entryinfo.dwFlags & JET_bitColumnEscrowUpdate )
    {
        BYTE    rgbDefaultValue[cbDefaultValueMost];
        ULONG   cbActual = 0;

        CallR( ErrIsamRetrieveColumn(
                    ppib,
                    pfucbCatalog,
                    fidMSO_DefaultValue,
                    &rgbDefaultValue,
                    sizeof(rgbDefaultValue),
                    &cbActual,
                    NO_GRBIT,
                    NULL ) );

        if ( 0 == cbActual )
        {
            (*popts->pcprintfError)( "Escrow column in a table (%d) does not have a default value\r\n", objidTable );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }
#endif

    Call( err );

HandleError:
    return err;
}


LOCAL VOID REPAIRCheckIndexColumnsOldFormat(
    const OBJID objidTable,
    const ENTRYINFO entryinfo,
    const INFOLIST * const pColInfoList,
    const INFOLIST * const pTemplateColInfo,
    BOOL * const pfCorrupted,
    const REPAIROPTS * const popts )
{
    *pfCorrupted = fFalse;

    IDXSEG_OLD          rgidxseg[JET_ccolKeyMost];

    memset( rgidxseg, 0, sizeof( rgidxseg ) );
    memcpy( rgidxseg, entryinfo.rgbIdxseg, min( sizeof( entryinfo.rgbIdxseg ), sizeof( rgidxseg ) ) );

    for( INT iidxseg = 0; iidxseg < JET_ccolKeyMost; iidxseg++ )
    {
        if ( 0 == rgidxseg[iidxseg] )
        {
            break;
        }

        const FID fid = ( rgidxseg[iidxseg] < 0 )  ? FID( -rgidxseg[iidxseg] ) : rgidxseg[iidxseg];
        const INFOLIST  * pinfolistT = NULL;

        pinfolistT = pTemplateColInfo;
        while ( pinfolistT )
        {
            if ( fid == pinfolistT->info.objidFDP )
            {
                break;
            }
            pinfolistT = pinfolistT->pInfoListNext;
        }

        if ( !pinfolistT || JET_coltypNil == pinfolistT->info.pgnoFDPORColType )
        {
            pinfolistT = pColInfoList;
            while ( pinfolistT )
            {
                if ( fid == pinfolistT->info.objidFDP )
                {
                    break;
                }
                pinfolistT = pinfolistT->pInfoListNext;
            }
        }

        if ( !pinfolistT )
        {
            (*popts->pcprintfError)( "Column %d from index %s (%d) does not exist in the table (%d) \r\n",
                                    fid, entryinfo.szName, entryinfo.objidFDP, objidTable );
            *pfCorrupted = fTrue;
        }
        else  if ( JET_coltypNil == pinfolistT->info.pgnoFDPORColType )
        {
            (*popts->pcprintfError)( "Column %d for index %s (%d) has been deleted from table (%d) \r\n",
                                    fid, entryinfo.szName, entryinfo.objidFDP, objidTable );
            *pfCorrupted = fTrue;
        }
    }
}


LOCAL VOID REPAIRCheckIndexColumnsExtendedFormat(
    const OBJID objidTable,
    const ENTRYINFO entryinfo,
    const INFOLIST * const pColInfoList,
    const INFOLIST * const pTemplateColInfo,
    BOOL * const pfCorrupted,
    const REPAIROPTS * const popts )
{
    *pfCorrupted = fFalse;

    IDXSEG          rgidxseg[JET_ccolKeyMost];

    memset( rgidxseg, 0, sizeof( rgidxseg ) );
    memcpy( rgidxseg, entryinfo.rgbIdxseg, sizeof( entryinfo.rgbIdxseg ) );

    const INT cidxseg = sizeof( entryinfo.rgbIdxseg ) / sizeof( IDXSEG );

    for( INT iidxseg = 0; iidxseg < cidxseg; iidxseg++ )
    {
        if ( 0 == rgidxseg[iidxseg].Fid() )
        {
            break;
        }

        const INFOLIST  * pinfolistT = NULL;
        if ( !rgidxseg[iidxseg].FTemplateColumn() || !pTemplateColInfo )
        {
            pinfolistT = pColInfoList;
        }
        else
        {
            pinfolistT = pTemplateColInfo;
        }

        while ( pinfolistT )
        {
            if ( rgidxseg[iidxseg].Fid() == pinfolistT->info.objidFDP )
            {
                break;
            }
            pinfolistT = pinfolistT->pInfoListNext;
        }

        if ( !pinfolistT )
        {
            (*popts->pcprintfError)( "Column %d (FID = %d) from index %s (%d) does not exist in the table (%d) \r\n",
                                    rgidxseg[iidxseg].Columnid(), rgidxseg[iidxseg].Fid(), entryinfo.szName, entryinfo.objidFDP, objidTable );
            *pfCorrupted = fTrue;
        }
        else  if ( JET_coltypNil == pinfolistT->info.pgnoFDPORColType )
        {
            (*popts->pcprintfError)( "Column %d (FID = %d) for index %s (%d) has been deleted from table (%d) \r\n",
                                    rgidxseg[iidxseg].Columnid(), rgidxseg[iidxseg].Fid(), entryinfo.szName, entryinfo.objidFDP, objidTable );
            *pfCorrupted = fTrue;
        }
    }
}


LOCAL ERR ErrREPAIRCheckOneIndexLogical(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB * const pfucbCatalog,
    const ENTRYINFO entryinfo,
    const ULONG objidTable,
    const ULONG pgnoFDPTable,
    const ULONG objidLV,
    const ULONG pgnoFDPLV,
    const INFOLIST * pColInfoList,
    const TEMPLATEINFOLIST * pTemplateInfoList,
    const BOOL fDerivedTable,
    const CHAR * pszTemplateTableName,
    const REPAIROPTS * const popts )
{
    ERR             err                 = JET_errSuccess;
    ULONG           fPrimaryIndex       = fidbPrimary & entryinfo.dwFlags;
    BOOL            fCorrupted          = fFalse;

    INFOLIST        *   pTemplateColInfo    = NULL;


    if ( objidTable == entryinfo.objidFDP && !fPrimaryIndex )
    {
        (*popts->pcprintfError)( "objidFDP (%d) in secondary index %s is the same as tableid (%d)\r\n",
                                    entryinfo.objidFDP, entryinfo.szName, objidTable );
        fCorrupted = fTrue;
    }

    if ( fPrimaryIndex && objidTable != entryinfo.objidFDP )
    {
        (*popts->pcprintfError)( "objidFDP (%d) in primary index %s is not the same as tableid (%d)\r\n",
                                    entryinfo.objidFDP, entryinfo.szName, objidTable );
        fCorrupted = fTrue;
    }

    if ( pgnoFDPTable == entryinfo.pgnoFDPORColType && !fPrimaryIndex )
    {
        (*popts->pcprintfError)( "pgnoFDP (%d) in secondary index %s is the same as pgnoFDP table (%d)\r\n",
                                    entryinfo.pgnoFDPORColType, entryinfo.szName, pgnoFDPTable );
        fCorrupted = fTrue;
    }

    if ( fPrimaryIndex && pgnoFDPTable != entryinfo.pgnoFDPORColType )
    {
        (*popts->pcprintfWarning)( "pgnoFDP (%d) in primary index %s is not the same as pgnoFDP table (%d)\r\n",
                                    entryinfo.pgnoFDPORColType, entryinfo.szName, pgnoFDPTable );
    }

    if ( objidLV == entryinfo.objidFDP )
    {
        (*popts->pcprintfError)( "objidFDP (%d) of index %s is the same as objid LV (%d)\r\n",
                                    entryinfo.objidFDP, entryinfo.szName, objidLV );
        fCorrupted = fTrue;
    }

    if ( pgnoFDPLV == entryinfo.pgnoFDPORColType )
    {
        (*popts->pcprintfError)( "pgnoFDP (%d) of index %s the same as pgnoFDP LV (%d)\r\n",
                                    entryinfo.pgnoFDPORColType, entryinfo.szName, pgnoFDPLV );
        fCorrupted = fTrue;
    }


    if ( fDerivedTable )
    {
        while ( pTemplateInfoList )
        {
            if ( !strcmp(pszTemplateTableName, pTemplateInfoList->szTemplateTableName) )
            {
                pTemplateColInfo = pTemplateInfoList->pColInfoList;
                break;
            }
            else
            {
                pTemplateInfoList = pTemplateInfoList->pTemplateInfoListNext;
            }
        }

        if ( NULL == pTemplateInfoList )
        {
            (*popts->pcprintfError)( "couldn't find the template table for index %s\r\n",
                                    entryinfo.szName );
            fCorrupted = fTrue;
        }
    }

    LE_IDXFLAG * const ple_idxflag = (LE_IDXFLAG*)(&entryinfo.dwFlags);
    const IDXFLAG idxflag       = ple_idxflag->fIDXFlags;

    if ( !( FIDXExtendedColumns( idxflag ) ) )
    {
        REPAIRCheckIndexColumnsOldFormat(
            objidTable,
            entryinfo,
            pColInfoList,
            pTemplateColInfo,
            &fCorrupted,
            popts );
    }
    else
    {
        REPAIRCheckIndexColumnsExtendedFormat(
            objidTable,
            entryinfo,
            pColInfoList,
            pTemplateColInfo,
            &fCorrupted,
            popts );
    }

    if ( fCorrupted )
    {
        err = ErrERRCheck( JET_errDatabaseCorrupted );
    }
    return err;
}


ERR ErrREPAIRInsertEntryinfoIntoPgnoFDPInfolist(
    INFOLIST **ppInfo,
    const ENTRYINFO entryinfo,
    const REPAIROPTS * const popts )
{
    ERR             err         = JET_errSuccess;
    INFOLIST    *   pInfo;
    INFOLIST    *   pTemp       = *ppInfo;
    INFOLIST    **  ppTemp      = ppInfo;
    BOOL            fAddedIntoList  = fFalse;

    pInfo = new INFOLIST;

    if ( NULL == pInfo )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    memset( pInfo, 0, sizeof(INFOLIST) );
    pInfo->info = entryinfo;
    pInfo->pInfoListNext = NULL;

    while ( pTemp && pTemp->info.pgnoFDPORColType <= pInfo->info.pgnoFDPORColType )
    {
        if ( pTemp->info.pgnoFDPORColType == pInfo->info.pgnoFDPORColType
            &&  ( sysobjTable != pTemp->info.objType
                || sysobjIndex != pInfo->info.objType ) )
        {
            (*popts->pcprintfError)( "Duplicated pgnoFDP (%d) in objects %s (%d) and %s (%d)\r\n",
                                pInfo->info.pgnoFDPORColType, pInfo->info.szName, pInfo->info.objidFDP, pTemp->info.szName, pTemp->info.objidFDP );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        ppTemp = &(pTemp->pInfoListNext);
        pTemp = pTemp->pInfoListNext;
        Assert( *ppTemp == pTemp );
    }

    Assert( !pTemp || pTemp->info.pgnoFDPORColType > pInfo->info.pgnoFDPORColType );

    pInfo->pInfoListNext = pTemp;
    (*ppTemp) = pInfo;

    fAddedIntoList = fTrue;

HandleError:
    if ( !fAddedIntoList )
    {
        delete pInfo;
    }

    return err;
}


ERR ErrREPAIRInsertEntryinfoIntoObjidInfolist(
    INFOLIST **ppInfo,
    const ENTRYINFO entryinfo,
    const REPAIROPTS * const popts )
{
    ERR             err         = JET_errSuccess;
    INFOLIST    *   pInfo;
    INFOLIST    *   pTemp       = *ppInfo;
    INFOLIST    **  ppTemp      = ppInfo;
    BOOL            fAddedIntoList  = fFalse;

    pInfo = new INFOLIST;

    if ( NULL == pInfo )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    memset( pInfo, 0, sizeof(INFOLIST) );
    pInfo->info = entryinfo;
    pInfo->pInfoListNext = NULL;

    while ( pTemp && pTemp->info.objidFDP <= pInfo->info.objidFDP )
    {
        if ( pTemp->info.objidFDP == pInfo->info.objidFDP
            &&  ( sysobjTable != pTemp->info.objType
                || sysobjIndex != pInfo->info.objType ) )
        {
            (*popts->pcprintfError)( "Duplicated objid (%d) in objects %s and %s\r\n",
                                pInfo->info.objidFDP, pInfo->info.szName, pTemp->info.szName );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        ppTemp = &(pTemp->pInfoListNext);
        pTemp = pTemp->pInfoListNext;
        Assert( *ppTemp == pTemp );
    }

    Assert( !pTemp || pTemp->info.objidFDP > pInfo->info.objidFDP );

    pInfo->pInfoListNext = pTemp;
    (*ppTemp) = pInfo;

    fAddedIntoList = fTrue;

HandleError:
    if ( !fAddedIntoList )
    {
        delete pInfo;
    }

    return err;
}


ERR ErrREPAIRInsertEntryinfoIntoInfolists(
    INFOLIST *rgpinfoObjid[],
    INFOLIST *rgpinfoPgnoFDP[],
    const INT cHash,
    const ENTRYINFO entryinfo,
    BOOL * const pfTableCorrupted,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    Assert( rgpinfoObjid != rgpinfoPgnoFDP );

    err = ErrREPAIRInsertEntryinfoIntoObjidInfolist(
            rgpinfoObjid + (entryinfo.objidFDP % cHash),
            entryinfo,
            popts );
    if ( JET_errDatabaseCorrupted == err )
    {
        *pfTableCorrupted = fTrue;
        err = JET_errSuccess;
    }
    Call( err );

    err = ErrREPAIRInsertEntryinfoIntoPgnoFDPInfolist(
            rgpinfoPgnoFDP + (entryinfo.pgnoFDPORColType % cHash),
            entryinfo,
            popts );
    if ( JET_errDatabaseCorrupted == err )
    {
        *pfTableCorrupted = fTrue;
        err = JET_errSuccess;
    }
    Call( err );
HandleError:
    return err;
}


ERR ErrREPAIRInsertEntryinfoIntoInfolist(
    INFOLIST **ppInfo,
    const ENTRYINFO entryinfo,
    const REPAIROPTS * const popts )
{
    ERR             err         = JET_errSuccess;
    INFOLIST    *   pInfo;
    INFOLIST    *   pTemp       = *ppInfo;
    BOOL            fAddedIntoList  = fFalse;

    Alloc( pInfo = new INFOLIST );

    memset( pInfo, 0, sizeof(INFOLIST) );
    pInfo->info = entryinfo;
    pInfo->pInfoListNext = NULL;

    if (NULL == pTemp )
    {
        *ppInfo = pInfo;
        fAddedIntoList = fTrue;
    }
    else
    {
        do
        {
            if ( !strcmp( pTemp->info.szName, pInfo->info.szName ) )
            {
                (*popts->pcprintfError)( "Two object have the same name (%s)\r\n", pInfo->info.szName );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }

            if ( sysobjIndex == pInfo->info.objType  &&
                pTemp->info.pgnoFDPORColType == pInfo->info.pgnoFDPORColType )
            {
                (*popts->pcprintfError)( "Duplicated pgnoFDP (%d) in objects %s and %s\r\n",
                                    pInfo->info.pgnoFDPORColType, pInfo->info.szName, pTemp->info.szName );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }

            if ( pTemp->info.objidFDP == pInfo->info.objidFDP )
            {
                (*popts->pcprintfError)( "Duplicated objid (%d) in objects %s and %s\r\n",
                                    pInfo->info.objidFDP, pInfo->info.szName, pTemp->info.szName );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
            Assert( pTemp->info.objidFDP < pInfo->info.objidFDP );

            if ( NULL == pTemp->pInfoListNext )
            {
                pTemp->pInfoListNext = pInfo;
                fAddedIntoList = fTrue;
                break;
            }
            else
            {
                pTemp = pTemp->pInfoListNext;
            }
        }
        while ( pTemp );
    }

HandleError:
    if ( !fAddedIntoList )
    {
        delete pInfo;
    }

    return err;
}


LOCAL ERR ErrREPAIRICheckCatalogEntryPgnoFDPs(
    PIB * const ppib,
    const IFMP ifmp,
    PGNO  * prgpgno,
    const ENTRYTOCHECK * const prgentryToCheck,
    INFOLIST **ppEntriesToDelete,
    const BOOL  fFix,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfPgnoFDPMarkedAsAvail,
    const REPAIROPTS * const popts )
{
    ERR             err = JET_errSuccess;

    ULONG   ulpgno = 0;


    BFPrereadPageList( ifmp, prgpgno, bfprfDefault, ppib->BfpriPriority( ifmp ), *TcRepair() );


    Assert( ( pttarrayAvailSpace == NULL ) == ( ppgnocollShelved == NULL ) );
    Assert( ( pttarrayAvailSpace == NULL ) == ( pfPgnoFDPMarkedAsAvail == NULL ) );
    if ( pttarrayAvailSpace && ppgnocollShelved && pfPgnoFDPMarkedAsAvail )
    {
        for( ulpgno = 0; pgnoNull != prgpgno[ulpgno]; ulpgno++ )
        {
            OBJID objidAvail;
            Call( pttarrayAvailSpace->ErrGetValue( ppib, prgpgno[ulpgno], &objidAvail ) );
            objidAvail &= 0x7fffffff;


            if ( objidNil != objidAvail )
            {
                (*popts->pcprintfError)( "page %d (pgnoFDP in the catalog) is available to objid %d\r\n", prgpgno[ulpgno], objidAvail );
                *pfPgnoFDPMarkedAsAvail = fTrue;
            }
            else if ( ppgnocollShelved->FContainsPgno( prgpgno[ulpgno] ) )
            {
                (*popts->pcprintfError)( "page %d (pgnoFDP in the catalog) is shelved\r\n", prgpgno[ulpgno] );
                *pfPgnoFDPMarkedAsAvail = fTrue;
            }
        }
    }


    for(ulpgno = 0; pgnoNull != prgpgno[ulpgno]; ulpgno++ )
    {
        OBJID objidIndexFDP = 0;
        CSR csr;

        err = csr.ErrGetReadPage(
                    ppib,
                    ifmp,
                    prgpgno[ulpgno],
                    bflfNoTouch );
        if ( err < 0 )
        {
            (*popts->pcprintfError)( "error %d trying to read page %d\r\n", err, prgpgno[ulpgno] );
            err = JET_errSuccess;
        }
        else
        {
            objidIndexFDP = csr.Cpage().ObjidFDP();
        }
        csr.ReleasePage( fTrue );

        if ( prgentryToCheck[ulpgno].objidFDP != objidIndexFDP
            && sysobjTable != prgentryToCheck[ulpgno].objType )
        {
            ENTRYINFO entryinfo;
            memset( &entryinfo, 0, sizeof( entryinfo ) );
            entryinfo.objidTable = prgentryToCheck[ulpgno].objidTable;
            entryinfo.objType    = prgentryToCheck[ulpgno].objType;
            entryinfo.objidFDP   = prgentryToCheck[ulpgno].objidFDP;

            (*popts->pcprintfError)( "Catalog entry corruption: page %d belongs to %d (expected %d)\r\n",
                                    prgpgno[ulpgno], objidIndexFDP, entryinfo.objidFDP );

            if ( fFix )
            {
                Call( ErrREPAIRBuildCatalogEntryToDeleteList( ppEntriesToDelete, entryinfo ) );
            }
            else
            {
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
        }
    }

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRCheckFixCatalogLogical(
    PIB * const ppib,
    const IFMP ifmp,
    OBJID * const pobjidLast,
    const BOOL  fShadow,
    const BOOL  fFix,
    QWORD * const pcTables,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfPgnoFDPMarkedAsAvail,
    const REPAIROPTS * const popts )
{
    ERR             err             = JET_errSuccess;
    FUCB        *   pfucbCatalog    = pfucbNil;

    BOOL            fObjidFDPSeen   = fFalse;

    BOOL            fTemplateTable  = fFalse;
    BOOL            fDerivedTable   = fFalse;

    ENTRYINFO       entryinfo;
    ULONG           pgnoFDPTableCurr= 0;
    ULONG           objidTableCurr  = 0;
    CHAR            szTableName[JET_cbNameMost + 1];
    CHAR            szTemplateTable[JET_cbNameMost + 1];
    ULONG           pgnoFDPLVCurr   = 0;
    ULONG           objidLVCurr     = 0;
    BOOL            fSeenLongValue  = fFalse;
    BOOL            fSeenCallback   = fFalse;

    BOOL            fSeenCorruptedTable         = fFalse;
    ULONG           objidLastCorruptedTable     = 0x7fffffff;
    BOOL            fSeenCorruptedIndex         = fFalse;

    INFOLIST    *   pColInfo        = NULL;
    INFOLIST    *   pIdxInfo        = NULL;
    TEMPLATEINFOLIST    *   pTemplateInfoList = NULL;

    INFOLIST    *   pTablesToDelete = NULL;
    INFOLIST    *   pEntriesToDelete = NULL;

    const INT       cpgnoFDPToPreread       = 64;

    PGNO            rgpgno[cpgnoFDPToPreread + 1];
    ENTRYTOCHECK    rgentryToCheck[cpgnoFDPToPreread + 1];
    ULONG           ulCount = 0;

    ULONG           ulpgno;


    const INT       cHash = ( 1024 * 1024 ) - 1;

    INFOLIST    **  rgpinfoObjid = 0;
    INFOLIST    **  rgpinfoPgnoFDP = 0;

    Alloc( rgpinfoObjid = new INFOLIST *[cHash] );
    memset( rgpinfoObjid, 0, sizeof( rgpinfoObjid[0] )  * cHash );

    Alloc( rgpinfoPgnoFDP = new INFOLIST *[cHash] );
    memset( rgpinfoPgnoFDP, 0, sizeof( rgpinfoPgnoFDP[0] ) * cHash );

    *pcTables = 0;



    for( ulpgno = 0; ulpgno <= cpgnoFDPToPreread; ulpgno++ )
    {
        rgpgno[ulpgno] = pgnoNull;
    }

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fShadow ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, szMSOIdIndex ) );
    Call( ErrIsamSetSequential( reinterpret_cast<JET_SESID>( ppib ),  reinterpret_cast<JET_VTID>( pfucbCatalog ), NO_GRBIT ) );

    err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT );
    if ( err == JET_errNoCurrentRecord )
    {
        err = ErrERRCheck( JET_errDatabaseCorrupted );
    }
    Call( err );

    while ( JET_errNoCurrentRecord != err )
    {
        Call( err );

        memset( &entryinfo, 0, sizeof( entryinfo ) );
        Call( ErrREPAIRRetrieveCatalogColumns( ppib, ifmp, pfucbCatalog, &entryinfo, popts) );

        if ( objidSystemRoot == entryinfo.objidTable    && !fSeenCorruptedTable )
        {

            objidTableCurr = entryinfo.objidTable;

            (*popts->pcprintfError)( "Invalid object type (%d) for object %s (%d)\r\n",
                                    entryinfo.objType, entryinfo.szName, entryinfo.objidTable );
            fSeenCorruptedTable = fTrue;

            AssertSzRTL( fFalse, "Unknown objType, should have been caught earlier" );
        }
        else if ( objidTableCurr != entryinfo.objidTable )
        {
            ++(*pcTables);

            if ( fTemplateTable && !fSeenCorruptedTable )
            {
                Call( ErrREPAIRInsertIntoTemplateInfoList(
                                &pTemplateInfoList,
                                szTemplateTable,
                                pColInfo,
                                popts ) );
            }
            else
            {
                REPAIRUtilCleanInfoList ( &pColInfo );
            }
            REPAIRUtilCleanInfoList ( &pIdxInfo );

            objidTableCurr = entryinfo.objidTable;

            fSeenCorruptedTable = fFalse;
            objidTableCurr      = entryinfo.objidTable;
            pgnoFDPTableCurr    = entryinfo.pgnoFDPORColType;
            pgnoFDPLVCurr       = 0;
            objidLVCurr         = 0;
            fSeenLongValue      = fFalse;
            fSeenCallback       = fFalse;
            pColInfo            = NULL;
            pIdxInfo            = NULL;
            fTemplateTable      = fFalse;
            fDerivedTable       = fFalse;

            Call( ErrREPAIRInsertEntryinfoIntoInfolists(
                    rgpinfoObjid,
                    rgpinfoPgnoFDP,
                    cHash,
                    entryinfo,
                    &fSeenCorruptedTable,
                    popts ) );

            if ( sysobjTable != entryinfo.objType )
            {
                (*popts->pcprintfError)( "Invalid object type for object %s (%d, expected %d)\r\n",
                                        entryinfo.szName, entryinfo.objType, sysobjTable );
                fSeenCorruptedTable = fTrue;
            }
            else if ( entryinfo.objidTable != entryinfo.objidFDP )
            {
                Assert( sysobjTable == entryinfo.objType );
                (*popts->pcprintfError)( "Invalid objid for table %s (%d, expected %d)\r\n",
                                        entryinfo.szName, entryinfo.objidFDP, objidTableCurr );
                fSeenCorruptedTable = fTrue;
            }
            else
            {
                fObjidFDPSeen = fTrue;

                OSStrCbCopyA( szTableName, sizeof(szTableName), entryinfo.szName );
                if ( JET_bitObjectTableTemplate & entryinfo.dwFlags )
                {
                    fTemplateTable = fTrue;
                    OSStrCbCopyA( szTemplateTable, sizeof(szTemplateTable), entryinfo.szName );
                    fDerivedTable = fFalse;
                }
                else if ( JET_bitObjectTableDerived & entryinfo.dwFlags )
                {
                    fDerivedTable = fTrue;
                    OSStrCbCopyA( szTemplateTable, sizeof(szTemplateTable), entryinfo.szTemplateTblORCallback );
                    fTemplateTable = fFalse;

                    TEMPLATEINFOLIST * pTemplateListT = NULL;
                    for( pTemplateListT = pTemplateInfoList;
                         pTemplateListT && ( 0 != strcmp( szTemplateTable, pTemplateListT->szTemplateTableName ) );
                         pTemplateListT = pTemplateListT->pTemplateInfoListNext )
                            ;

                    if ( NULL == pTemplateListT )
                    {
                        (*popts->pcprintfError)("Template table (%s) does not exist\r\n", szTemplateTable );
                        fSeenCorruptedTable = fTrue;
                    }
                }
                else
                {
                    Assert( fFalse == fTemplateTable );
                    Assert( fFalse == fDerivedTable );
                }
            }
        }
        else if ( !fSeenCorruptedTable )
        {
            Assert( objidTableCurr == entryinfo.objidTable );

            switch ( entryinfo.objType )
            {
                case sysobjTable:
                    (*popts->pcprintfError)("Multiple table records for a table (%d) in a catalog\r\n", entryinfo.objidTable );
                    fSeenCorruptedTable = fTrue;
                    break;

                case sysobjColumn:
                    if ( JET_coltypNil == entryinfo.pgnoFDPORColType )
                    {
                        err = JET_errSuccess;
                    }
                    else
                    {
                        err = ErrREPAIRCheckOneColumnLogical(
                                    ppib,
                                    pfucbCatalog,
                                    entryinfo,
                                    objidTableCurr,
                                    popts );
                        if ( JET_errSuccess == err )
                        {
                            err = ErrREPAIRInsertEntryinfoIntoInfolist(
                                    &pColInfo,
                                    entryinfo,
                                    popts );
                        }
                    }

                    if ( JET_errDatabaseCorrupted == err )
                    {
                        fSeenCorruptedTable = fTrue;
                    }

                    break;
                case sysobjIndex:
                    Call( ErrREPAIRInsertEntryinfoIntoInfolists(
                            rgpinfoObjid,
                            rgpinfoPgnoFDP,
                            cHash,
                            entryinfo,
                            &fSeenCorruptedIndex,
                            popts ) );

                    err = ErrREPAIRCheckOneIndexLogical(
                                ppib,
                                ifmp,
                                pfucbCatalog,
                                entryinfo,
                                objidTableCurr,
                                pgnoFDPTableCurr,
                                objidLVCurr,
                                pgnoFDPLVCurr,
                                pColInfo,
                                pTemplateInfoList,
                                fDerivedTable,
                                szTemplateTable,
                                popts );
                    if ( JET_errDatabaseCorrupted == err )
                    {
                        fSeenCorruptedIndex = fTrue;
                        err = JET_errSuccess;
                    }
                    Call( err );

                    err = ErrREPAIRInsertEntryinfoIntoInfolist(
                                &pIdxInfo,
                                entryinfo,
                                popts );
                    if ( JET_errDatabaseCorrupted == err )
                    {
                        fSeenCorruptedIndex = fTrue;
                        err = JET_errSuccess;
                    }
                    Call( err );

                    fObjidFDPSeen = fTrue;
                    break;

                case sysobjLongValue:
                    if ( fSeenLongValue )
                    {
                        (*popts->pcprintfError)("Multiple long-value records for a table (%d) in a catalog\r\n", entryinfo.objidTable );
                        fSeenCorruptedTable = fTrue;
                    }
                    else
                    {
                        Call( ErrREPAIRInsertEntryinfoIntoInfolists(
                                rgpinfoObjid,
                                rgpinfoPgnoFDP,
                                cHash,
                                entryinfo,
                                &fSeenCorruptedTable,
                                popts ) );

                        fObjidFDPSeen   = fTrue;
                        pgnoFDPLVCurr   = entryinfo.pgnoFDPORColType;
                        objidLVCurr     = entryinfo.objidFDP;
                        fSeenLongValue  = fTrue;
                    }
                    break;

                case sysobjCallback:
                    if ( fSeenCallback )
                    {
                        (*popts->pcprintfError)("Multiple callbacks for a table (%d) in a catalog\r\n", entryinfo.objidTable );
                        fSeenCorruptedTable = fTrue;
                    }
                    fSeenCallback = fTrue;
                    break;

                default:
                    (*popts->pcprintfError)("Invalid Object Type (%d) for a table (%d) in a catalog\r\n", entryinfo.objType, entryinfo.objidTable );
                    fSeenCorruptedTable = fTrue;
                    break;
            }
        }
        else
        {
            Assert( fSeenCorruptedTable && objidTableCurr == entryinfo.objidTable );
        }

        if ( fSeenCorruptedIndex )
        {
            if ( fFix )
            {
                Call( ErrREPAIRBuildCatalogEntryToDeleteList( &pEntriesToDelete, entryinfo ) );
            }
            else
            {
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
            fSeenCorruptedIndex = fFalse;
        }

        if ( fSeenCorruptedTable && objidLastCorruptedTable != entryinfo.objidTable )
        {
            if ( fFix )
            {
                Call( ErrREPAIRBuildCatalogEntryToDeleteList( &pTablesToDelete, entryinfo ) );
            }
            else
            {
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
            objidLastCorruptedTable = entryinfo.objidTable;
        }

        if ( fObjidFDPSeen )
        {
            if ( entryinfo.objidFDP > *pobjidLast )
            {
                *pobjidLast = entryinfo.objidFDP;
            }

            rgpgno[ulCount] = entryinfo.pgnoFDPORColType;
            rgentryToCheck[ulCount].objidTable = entryinfo.objidTable;
            rgentryToCheck[ulCount].objType = entryinfo.objType;
            rgentryToCheck[ulCount].objidFDP = entryinfo.objidFDP;
            ulCount++;

            fObjidFDPSeen = fFalse;
        }

        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );


        if ( 0 == ( ulCount % ( cpgnoFDPToPreread - 1 ) )
             || JET_errNoCurrentRecord == err )
        {
            ERR errT = ErrREPAIRICheckCatalogEntryPgnoFDPs(
                        ppib,
                        ifmp,
                        rgpgno,
                        rgentryToCheck,
                        &pEntriesToDelete,
                        fFix,
                        pttarrayAvailSpace,
                        ppgnocollShelved,
                        pfPgnoFDPMarkedAsAvail,
                        popts );
            if ( errT < 0 )
            {
                Call( errT );
            }

            for( ulpgno = 0; (ulpgno < (sizeof(rgpgno)/sizeof(PGNO))) && (pgnoNull != rgpgno[ulpgno]); ulpgno++ )
            {
                Assert( cpgnoFDPToPreread > ulpgno );
                rgpgno[ulpgno] = pgnoNull;
            }
            ulCount = 0;

        }

    }

    if ( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }

HandleError:

    if ( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }


    if ( fFix && ( pTablesToDelete || pEntriesToDelete ) )
    {
        (*popts->pcprintfVerbose)( "Deleting corrupted catalog entries\r\n" );
        err = ErrREPAIRDeleteCorruptedEntriesFromCatalog(
                    ppib,
                    ifmp,
                    pTablesToDelete,
                    pEntriesToDelete,
                    popts );
        if ( JET_errSuccess == err )
        {
            err = ErrERRCheck( JET_errDatabaseCorrupted );
        }
    }

    REPAIRUtilCleanInfoList( &pColInfo );
    REPAIRUtilCleanInfoList( &pIdxInfo );
    REPAIRUtilCleanInfoList( &pTablesToDelete );
    REPAIRUtilCleanInfoList( &pEntriesToDelete );
    if ( rgpinfoObjid )
    {
        REPAIRUtilCleanInfoLists( rgpinfoObjid, cHash );
    }
    if ( rgpinfoPgnoFDP )
    {
        REPAIRUtilCleanInfoLists( rgpinfoPgnoFDP, cHash );
    }
    REPAIRUtilCleanTemplateInfoList( &pTemplateInfoList );

    delete [] rgpinfoObjid;
    delete [] rgpinfoPgnoFDP;

    return err;
}

LOCAL ERR ErrREPAIRCompareCatalogs(
    PIB * const ppib,
    const IFMP ifmp,
    const REPAIROPTS * const popts )
{
    ERR             err             = JET_errSuccess;
    FUCB        *   pfucbCatalog    = pfucbNil;
    FUCB        *   pfucbShadow     = pfucbNil;

    ENTRYINFO       entryinfo;
    ENTRYINFO       entryinfoShadow;

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATOpen( ppib, ifmp, &pfucbShadow, fTrue ) );
    Assert( pfucbNil != pfucbShadow );
    
    Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, szMSOIdIndex ) );
    Call( ErrIsamSetCurrentIndex( ppib, pfucbShadow, szMSOIdIndex ) );
    
    Call( ErrIsamSetSequential( reinterpret_cast<JET_SESID>( ppib ),  reinterpret_cast<JET_VTID>( pfucbCatalog ), NO_GRBIT ) );
    Call( ErrIsamSetSequential( reinterpret_cast<JET_SESID>( ppib ),  reinterpret_cast<JET_VTID>( pfucbShadow ), NO_GRBIT ) );

    Call( ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT ) );
    Call( ErrIsamMove( ppib, pfucbShadow,  JET_MoveFirst, NO_GRBIT ) );

    while ( fTrue )
    {
        memset( &entryinfo, 0, sizeof( entryinfo ) );
        memset( &entryinfoShadow, 0, sizeof( entryinfoShadow ) );

        Call( ErrREPAIRRetrieveCatalogColumns( ppib, ifmp, pfucbCatalog, &entryinfo,       popts) );
        Call( ErrREPAIRRetrieveCatalogColumns( ppib, ifmp, pfucbShadow,  &entryinfoShadow, popts) );

        if ( entryinfo.IsEqual( entryinfoShadow ) )
        {
            err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
            ERR errShadow = ErrIsamMove( ppib, pfucbShadow, JET_MoveNext, NO_GRBIT );
            if ( err != errShadow )
            {
                (*popts->pcprintfVerbose)( "Detected mismatches between catalog and shadow catalog (err = %d, err = %d) \r\n", err, errShadow );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
            if ( err == JET_errNoCurrentRecord )
            {
                err = JET_errSuccess;
                break;
            }
            Call( err );
        }
        else if ( entryinfoShadow.objType         == sysobjColumn &&
                  entryinfoShadow.pgnoFDPORColType == JET_coltypNil )

        {
            Call( ErrIsamMove( ppib, pfucbShadow, JET_MoveNext, NO_GRBIT ) );
        }
        else if ( entryinfo.objType           == sysobjColumn &&
                  entryinfo.pgnoFDPORColType == JET_coltypNil )
        {
            (*popts->pcprintfVerbose)( "Deleted column is missing from the shadow catalog\r\n" );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        else
        {
            if ( entryinfo.sortid != entryinfoShadow.sortid ||
                 entryinfo.qwSortVersion != entryinfoShadow.qwSortVersion ||
                 ( 0 == wcscmp( entryinfo.wszLocaleName, entryinfoShadow.wszLocaleName ) ) )
            {
                WCHAR wszSortIDCatalog[PERSISTED_SORTID_MAX_LENGTH] = L"";
                WCHAR wszSortIDCatalogShadow[PERSISTED_SORTID_MAX_LENGTH] = L"";
                WszCATFormatSortID( entryinfo.sortid, wszSortIDCatalog, _countof( wszSortIDCatalog ) );
                WszCATFormatSortID( entryinfoShadow.sortid, wszSortIDCatalogShadow, _countof( wszSortIDCatalogShadow ) );

                (*popts->pcprintfVerbose)( "        Catalog = [name = %s, locale='%ws', sortid='%ws', sortversion=%#I64x'}\r\n",
                                          entryinfo.szName, entryinfo.wszLocaleName, wszSortIDCatalog, entryinfo.qwSortVersion );
                (*popts->pcprintfVerbose)( "  ShadowCatalog = [name = %s, locale='%ws', sortid='%ws', sortversion=%#I64x'}\r\n",
                                          entryinfoShadow.szName, entryinfoShadow.wszLocaleName, wszSortIDCatalogShadow, entryinfoShadow.qwSortVersion );
            }
            (*popts->pcprintfVerbose)( "Detected a mismatch between catalog and shadow catalog\r\n" );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }


HandleError:
    if ( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    if ( pfucbNil != pfucbShadow )
    {
        CallS( ErrCATClose( ppib, pfucbShadow ) );
    }

    return err;
}



LOCAL ERR ErrREPAIRCheckSystemTablesLogical(
    PIB * const ppib,
    const IFMP ifmp,
    OBJID * const pobjidLast,
    BOOL * const pfCatalogCorrupt,
    BOOL * const pfShadowCatalogCorrupt,
    BOOL * const pfMSObjidsCorrupt,
    BOOL * const pfMSLocalesCorrupt,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfPgnoFDPMarkedAsAvail,
    const REPAIROPTS * const popts )
{
    ERR             err             = JET_errSuccess;
    const BOOL      fFix            = fFalse;

    Assert( ! ( *pfCatalogCorrupt ) || !( *pfShadowCatalogCorrupt ) );

    (*popts->pcprintfVerbose)( "checking logical consistency of system tables\r\n" );
    (*popts->pcprintfVerbose).Indent();

    QWORD cTablesInCatalog      = 0;
    QWORD cTablesInShadowCatalog    = 0;

    if ( !( *pfCatalogCorrupt ) )
    {
        (*popts->pcprintfVerbose)( "%s\r\n", szMSO );
        err = ErrREPAIRCheckFixCatalogLogical( ppib, ifmp, pobjidLast, fFalse, fFix, &cTablesInCatalog, pttarrayAvailSpace, ppgnocollShelved, pfPgnoFDPMarkedAsAvail, popts );
        if ( JET_errDatabaseCorrupted == err )
        {
            (*popts->pcprintfError)( "%s is logically corrupted\r\n", szMSO );
            *pfCatalogCorrupt = fTrue;
        }
        else
        {
            Call( err );
        }
    }

    if ( !( *pfShadowCatalogCorrupt ) )
    {
        (*popts->pcprintfVerbose)( "%s\r\n", szMSOShadow );
        err = ErrREPAIRCheckFixCatalogLogical( ppib, ifmp, pobjidLast, fTrue, fFix, &cTablesInShadowCatalog, pttarrayAvailSpace, ppgnocollShelved, pfPgnoFDPMarkedAsAvail, popts );
        if ( JET_errDatabaseCorrupted == err )
        {
            (*popts->pcprintfError)( "%s is logically corrupted\r\n", szMSOShadow );
            *pfShadowCatalogCorrupt = fTrue;
        }
        else
        {
            Call( err );
        }
    }

    if ( !*pfCatalogCorrupt && !*pfShadowCatalogCorrupt )
    {
        ERR errT = ErrREPAIRCompareCatalogs( ppib, ifmp, popts );
        if ( errT < 0 )
        {

            if ( cTablesInCatalog >= cTablesInShadowCatalog )
            {
                (*popts->pcprintfError)( "%s is missing information present in %s \r\n", szMSOShadow, szMSO );
                *pfShadowCatalogCorrupt = fTrue;
            }
            else
            {
                (*popts->pcprintfError)( "%s is missing information present in %s \r\n", szMSO, szMSOShadow );
                *pfCatalogCorrupt = fTrue;
            }
        }
    }

    *pfMSObjidsCorrupt = fTrue;
    if ( !*pfCatalogCorrupt )
    {
        (*popts->pcprintfVerbose)( "%s\r\n", szMSObjids );
        ERR errT = ErrCATVerifyMSObjids( ppib, ifmp, popts->pcprintfError );
        if ( errT >= 0 )
        {
            *pfMSObjidsCorrupt = fFalse;
        }
        else
        {
            (*popts->pcprintfError)( "%s failed verification with err = %d \r\n", szMSObjids, errT );
        }
    }

    *pfMSLocalesCorrupt = ( NULL != g_rgfmp[ifmp].PkvpsMSysLocales() );
    if ( !*pfCatalogCorrupt && *pfMSLocalesCorrupt )
    {
        (*popts->pcprintfVerbose)( "%s\r\n", szMSLocales );
        ERR errT = ErrCATVerifyMSLocales( ppib, ifmp, fFalse );
        if ( errT >= 0 )
        {
            *pfMSLocalesCorrupt = fFalse;
        }
        else
        {
            (*popts->pcprintfError)( "%s failed verification with err = %d \r\n", szMSLocales, errT );
        }
    }

    Assert( JET_errDatabaseCorrupted == err ||
            JET_errSuccess  == err );
    err = JET_errSuccess;

HandleError:
    (*popts->pcprintfVerbose).Unindent();
    return err;
}

LOCAL ERR ErrREPAIRCheckGlobalSpaceTree(
    PIB * const ppib,
    const IFMP ifmp,
    BOOL * const pfSpaceTreeCorrupt,
    PGNO * const ppgnoLastOwned,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;
    bool fInTransaction = false;
    bool fIndented = false;

    RECCHECKNULL recchecknull;
    RECCHECKSPACEOE reccheckOE( ppib, pttarrayOwnedSpace, pttarrayAvailSpace, ppgnocollShelved, objidSystemRoot, objidNil, popts );
    RECCHECKSPACEAE reccheckAE( ppib, pttarrayOwnedSpace, pttarrayAvailSpace, ppgnocollShelved, objidSystemRoot, objidNil, popts );

    BTSTATS* pbtstats = NULL;
    Alloc( pbtstats = new BTSTATS );
    BTSTATS& btstats = *pbtstats;
    memset( &btstats, 0, sizeof( btstats ) );

    Call( ErrDIRBeginTransaction( ppib, 54053, NO_GRBIT ) );
    fInTransaction = true;

    (*popts->pcprintfVerbose)( "checking SystemRoot\r\n" );
    (*popts->pcprintfVerbose).Indent();
    fIndented = true;

    (*popts->pcprintfStats)( "\r\n\r\n" );
    (*popts->pcprintfStats)( "===== database root =====\r\n" );

    Call( ErrREPAIRCheckTree(
            ppib,
            ifmp,
            pgnoSystemRoot,
            objidSystemRoot,
            CPAGE::fPagePrimary,
            &recchecknull,
            NULL,
            NULL,
            NULL,
            fFalse,
            pfDbtimeTooLarge,
            &btstats,
            popts ) );
    (*popts->pcprintfVerbose)( "SystemRoot (OE)\r\n" );
    Call( ErrREPAIRCheckTree(
            ppib,
            ifmp,
            pgnoSystemRoot+1,
            objidSystemRoot,
            CPAGE::fPageSpaceTree,
            &reccheckOE,
            NULL,
            NULL,
            NULL,
            fFalse,
            pfDbtimeTooLarge,
            &btstats,
            popts ) );
    (*popts->pcprintfVerbose)( "SystemRoot (AE)\r\n" );
    Call( ErrREPAIRCheckTree(
            ppib,
            ifmp,
            pgnoSystemRoot+2,
            objidSystemRoot,
            CPAGE::fPageSpaceTree,
            &reccheckAE,
            NULL,
            NULL,
            NULL,
            fFalse,
            pfDbtimeTooLarge,
            &btstats,
            popts ) );

    *ppgnoLastOwned = reccheckOE.PgnoLast();

HandleError:
    if ( fIndented )
    {
        (*popts->pcprintfVerbose).Unindent();
    }
    if ( fInTransaction )
    {
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }

    switch ( err )
    {
        case JET_errNoCurrentRecord:
        case JET_errRecordDeleted:
        case JET_errRecordNotFound:
        case_AllDatabaseStorageCorruptionErrs:
        case JET_errPageNotInitialized:
        case JET_errDiskIO:
        case JET_errFileIOBeyondEOF:
        case JET_errDatabaseCorrupted:
            *pfSpaceTreeCorrupt = fTrue;
            err = JET_errSuccess;
            break;
        default:
            *pfSpaceTreeCorrupt = fFalse;
            break;
    }

    delete pbtstats;
    return err;
}


LOCAL ERR ErrREPAIRCheckRange(
    PIB * const ppib,
    TTARRAY * const pttarray,
    PgnoCollection * const ppgnocollShelved,
    const ULONG ulFirst,
    const ULONG ulLast,
    const ULONG ulValue,
    BOOL * const pfMismatch,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;
    TTARRAY::RUN run;

    pttarray->BeginRun( ppib, &run );

    ULONG ul;
    for( ul = ulFirst; ul < ulLast; ul++ )
    {
        ULONG ulActual;

        Call( pttarray->ErrGetValue( ppib, ul, &ulActual, &run ) );

        if ( ( ( ulActual & 0x7fffffff ) != ( ulValue & 0x7fffffff ) ) && !ppgnocollShelved->FContainsPgno( (PGNO)ul ) )
        {
            *pfMismatch = fTrue;
            break;
        }
    }


HandleError:
    pttarray->EndRun( ppib, &run );
    return err;
}



const CHAR * const rgszRepairDisallowedTables[] =
{
    "datatable",
    "hiddentable",
};

const INT cszRepairDisallowedTables = sizeof( rgszRepairDisallowedTables ) / sizeof( rgszRepairDisallowedTables[0] );

LOCAL ERR ErrDBRepairDisallowed(
    PIB * const     ppib,
    FUCB * const    pfucbCatalog,
    BOOL *          pfRepairDisallowed )
{
    ERR     err             = JET_errSuccess;
    INT     isz;
#ifdef DEBUG
    INT     iTableFound     = 0;
#endif


    Assert( pfRepairDisallowed );
    (*pfRepairDisallowed) = fFalse;

    Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, szMSORootObjectsIndex ) );

    for( isz = 0; isz < cszRepairDisallowedTables && !(*pfRepairDisallowed); ++isz )
    {
        const BYTE  bTrue       = 0xff;
        Call( ErrIsamMakeKey(
                    ppib,
                    pfucbCatalog,
                    &bTrue,
                    sizeof(bTrue),
                    JET_bitNewKey ) );
        Call( ErrIsamMakeKey(
                    ppib,
                    pfucbCatalog,
                    (BYTE *)rgszRepairDisallowedTables[isz],
                    (ULONG)strlen( rgszRepairDisallowedTables[isz] ),
                    NO_GRBIT ) );
        err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );

        if ( JET_errRecordNotFound == err )
        {
            Assert( !(*pfRepairDisallowed) );
            err = JET_errSuccess;
            goto HandleError;
        }
        Call( err );

#ifdef DEBUG
        iTableFound++;
#endif
    }

    Assert ( iTableFound == cszRepairDisallowedTables );
    (*pfRepairDisallowed) = fTrue;

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRStartCheckAllTables(
    PIB * const ppib,
    const IFMP ifmp,
    TASKMGR * const ptaskmgr,
    REPAIRTABLE ** const pprepairtable,
    BOOL * const pfDbtimeTooLarge,
    INTEGGLOBALS * const pintegglobals,
    const REPAIROPTS * const popts )
{
    ERR     err             = JET_errSuccess;
    FUCB    *pfucbCatalog   = pfucbNil;
    DATA    data;
    BOOL    fCorruptionSeen = fFalse;

    pintegglobals->fCorruptionSeen              = fFalse;
    pintegglobals->err                          = JET_errSuccess;
    pintegglobals->pprepairtable                = pprepairtable;
    pintegglobals->pfDbtimeTooLarge             = pfDbtimeTooLarge;
    pintegglobals->popts                        = popts;
    pintegglobals->fRepairDisallowed            = fFalse;

    Call( ErrDIRBeginTransaction( ppib, 41765, NO_GRBIT ) );

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );
    FUCBSetSequential( pfucbCatalog );
    FUCBSetPrereadForward( pfucbCatalog, cpgPrereadSequential );

    Call( ErrDBRepairDisallowed( ppib, pfucbCatalog, &(pintegglobals->fRepairDisallowed) ) );

    if ( pintegglobals->fRepairDisallowed )
    {
        (*popts->pcprintfWarning)( "Repair is not allowed on this database due to its specific content\r\n" );
    }

    Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, szMSORootObjectsIndex ) );


    INT isz;
    for( isz = 0; isz < cszLargeTables && !fCorruptionSeen; ++isz )
    {
        const BYTE  bTrue       = 0xff;
        Call( ErrIsamMakeKey(
                    ppib,
                    pfucbCatalog,
                    &bTrue,
                    sizeof(bTrue),
                    JET_bitNewKey ) );
        Call( ErrIsamMakeKey(
                    ppib,
                    pfucbCatalog,
                    (BYTE *)rgszLargeTables[isz],
                    (ULONG)strlen( rgszLargeTables[isz] ),
                    NO_GRBIT ) );
        err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
        if ( JET_errRecordNotFound == err )
        {
            continue;
        }
        Call( err );
        Call( ErrDIRGet( pfucbCatalog ) );
        Call( ErrREPAIRPostTableTask(
                ppib,
                ifmp,
                pfucbCatalog,
                rgszLargeTables[isz],
                pprepairtable,
                pintegglobals,
                ptaskmgr,
                &fCorruptionSeen,
                popts ) );
    }


    err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, 0 );
    while ( err >= 0 )
    {
        Call( ErrDIRGet( pfucbCatalog ) );

        CHAR szTable[JET_cbNameMost+1];
        Assert( FVarFid( fidMSO_Name ) );
        Call( ErrRECIRetrieveVarColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Name,
                    pfucbCatalog->kdfCurr.data,
                    &data ) );
        CallS( err );

        const INT cbData = data.Cb();
        
        if ( cbData > JET_cbNameMost )
        {
            (*popts->pcprintfError)( "catalog corruption (MSO_Name): name too long: expected no more than %d bytes, got %d\r\n",
                JET_cbNameMost, cbData );
            fCorruptionSeen = fTrue;
            Call( ErrDIRRelease( pfucbCatalog ) );
        }
        else
        {
            UtilMemCpy( szTable, data.Pv(), cbData );
            szTable[cbData] = 0;


            if ( !FCATSystemTable( szTable )
                && !FIsLargeTable( szTable ) )
            {
                Call( ErrREPAIRPostTableTask(
                        ppib,
                        ifmp,
                        pfucbCatalog,
                        szTable,
                        pprepairtable,
                        pintegglobals,
                        ptaskmgr,
                        &fCorruptionSeen,
                        popts ) );
            }
            else
            {
                Call( ErrDIRRelease( pfucbCatalog ) );
            }
        }

        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, 0 );
    }
    if ( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }


HandleError:
    if ( pfucbNil != pfucbCatalog )
    {
        CallS( ErrFILECloseTable( ppib, pfucbCatalog ) );
    }
    CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

    if ( JET_errSuccess == err
        && fCorruptionSeen )
    {
        err = ErrERRCheck( JET_errDatabaseCorrupted );
    }

    return err;
}


LOCAL ERR ErrREPAIRStopCheckTables( const INTEGGLOBALS * const pintegglobals, BOOL * const pfCorrupt )
{
    *pfCorrupt = pintegglobals->fCorruptionSeen;
    return pintegglobals->err;
}


LOCAL ERR ErrREPAIRPostTableTask(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB * const pfucbCatalog,
    const CHAR * const szTable,
    REPAIRTABLE ** const pprepairtable,
    INTEGGLOBALS * const pintegglobals,
    TASKMGR * const ptaskmgr,
    BOOL * const pfCorrupted,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;
    DATA data;

    PGNO pgnoFDP;
    CPG  cpgExtent;

    Assert( FFixedFid( fidMSO_PgnoFDP ) );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_PgnoFDP,
                pfucbCatalog->kdfCurr.data,
                &data ) );
    CallS( err );
    if ( sizeof( PGNO ) != data.Cb() )
    {
        (*popts->pcprintfError)( "catalog corruption (MSO_PgnoFDP): unexpected size: expected %d bytes, got %d\r\n",
            sizeof( PGNO ), data.Cb() );
        *pfCorrupted = fTrue;
        goto HandleError;
    }
    pgnoFDP = *( (UnalignedLittleEndian< PGNO > *)data.Pv() );
    if ( pgnoNull == pgnoFDP )
    {
        (*popts->pcprintfError)( "catalog corruption (MSO_PgnoFDP): pgnoFDP is NULL\r\n" );
        *pfCorrupted = fTrue;
        goto HandleError;
    }
    if ( pgnoFDP > pgnoSysMax )
    {
        (*popts->pcprintfError)( "catalog corruption (MSO_PgnoFDP): pgnoFDP is out of bounds\r\n" );
        *pfCorrupted = fTrue;
        goto HandleError;
    }


    Assert( FFixedFid( fidMSO_Pages ) );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Pages,
                pfucbCatalog->kdfCurr.data,
                &data ) );
    CallS( err );
    if ( sizeof( CPG ) != data.Cb() )
    {
        (*popts->pcprintfError)( "catalog corruption (fidMSO_Pages): unexpected size: expected %d bytes, got %d\r\n",
            sizeof( CPG ), data.Cb() );
        *pfCorrupted = fTrue;
        goto HandleError;
    }
    cpgExtent = max( 1, (INT) *( (UnalignedLittleEndian< CPG > *)data.Pv() ) );
    if ( (ULONG) cpgExtent > pgnoSysMax)
    {
        (*popts->pcprintfError)( "catalog corruption (MSO_cpgExtent): cpgExtent %d is out of bounds\r\n",cpgExtent );
        *pfCorrupted = fTrue;
        goto HandleError;
    }
    Assert( FFixedFid( fidMSO_Type ) );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Type,
                pfucbCatalog->kdfCurr.data,
                &data ) );
    CallS( err );
    if ( sizeof( SYSOBJ ) != data.Cb() )
    {
        (*popts->pcprintfError)( "catalog corruption (MSO_Type): unexpected size: expected %d bytes, got %d\r\n",
            sizeof( SYSOBJ ), data.Cb() );
        *pfCorrupted = fTrue;
        goto HandleError;
    }
    if ( sysobjTable != *( (UnalignedLittleEndian< SYSOBJ > *)data.Pv() ) )
    {
        (*popts->pcprintfError)( "catalog corruption (MSO_Type): unexpected type: expected type %d, got type %d\r\n",
            sysobjTable, (SYSOBJ)*( (UnalignedLittleEndian< SYSOBJ > *)data.Pv() ) );
        *pfCorrupted = fTrue;
        goto HandleError;
    }

#ifdef DEBUG
    Assert( FVarFid( fidMSO_Name ) );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Name,
                pfucbCatalog->kdfCurr.data,
                &data ) );
    CallS( err );
    Assert( 0 == _strnicmp( szTable, reinterpret_cast<CHAR *>( data.Pv() ), data.Cb() ) );
#endif

    OBJID objidTable;
    Assert( FFixedFid( fidMSO_Id ) );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Id,
                pfucbCatalog->kdfCurr.data,
                &data ) );
    CallS( err );
    if ( sizeof( objidTable ) != data.Cb() )
    {
        (*popts->pcprintfError)( "catalog corruption (MSO_Id): unexpected size: expected %d bytes, got %d\r\n",
            sizeof( objidTable ), data.Cb() );
        *pfCorrupted = fTrue;
        goto HandleError;
    }

    objidTable = *(UnalignedLittleEndian< OBJID > *)data.Pv();

    if ( objidInvalid == objidTable )
    {
        (*popts->pcprintfError)( "catalog corruption (MSO_Id): objidInvalid (%d) reached.\r\n", objidInvalid );
        *pfCorrupted = fTrue;
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    Call( ErrDIRRelease( pfucbCatalog ) );

    {
        CHECKTABLE * const pchecktable  = new CHECKTABLE;
        if ( NULL == pchecktable )
        {
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }

        Assert( strlen( szTable ) <= JET_cbNameMost );
        OSStrCbCopyA( pchecktable->szTable, sizeof(pchecktable->szTable), szTable );

        pchecktable->ifmp               = ifmp;
        pchecktable->szIndex[0]         = 0;
        pchecktable->objidFDP           = objidTable;
        pchecktable->pgnoFDP            = pgnoFDP;
        pchecktable->objidParent        = objidSystemRoot;
        pchecktable->pgnoFDPParent      = pgnoSystemRoot;
        pchecktable->fPageFlags         = 0;
        pchecktable->fUnique            = fTrue;
        pchecktable->preccheck          = NULL;
        pchecktable->cpgPrimaryExtent   = cpgExtent;
        pchecktable->pglobals           = pintegglobals;
        pchecktable->fDeleteWhenDone    = fTrue;

        err = ptaskmgr->ErrPostTask( REPAIRCheckOneTableTask, (ULONG_PTR)pchecktable );
        if ( err < 0 )
        {
            Assert( JET_errOutOfMemory == err );
            delete pchecktable;
            Call( err );
        }
    }

    return err;

HandleError:
    Call( ErrDIRRelease( pfucbCatalog ) );
    return err;
}


LOCAL VOID REPAIRCheckTreeAndSpaceTask( PIB * const ppib, const ULONG_PTR ul )
{
    TASKMGR::TASK task = REPAIRCheckTreeAndSpaceTask;

    Unused( task );

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = tceNone;
    tcScope->iorReason.SetIort( iortRepair );

    CHECKTABLE * const pchecktable = (CHECKTABLE *)ul;

    CallS( ErrDIRBeginTransaction(ppib, 58149, NO_GRBIT ) );

    CPRINTF::SetThreadPrintfPrefix( pchecktable->szTable );

    ERR err = ErrREPAIRCheckTreeAndSpace(
                        ppib,
                        pchecktable->ifmp,
                        pchecktable->objidFDP,
                        pchecktable->pgnoFDP,
                        pchecktable->objidParent,
                        pchecktable->pgnoFDPParent,
                        pchecktable->fPageFlags,
                        pchecktable->fUnique,
                        pchecktable->preccheck,
                        pchecktable->pglobals->pttarrayOwnedSpace,
                        pchecktable->pglobals->pttarrayAvailSpace,
                        pchecktable->pglobals->ppgnocollShelved,
                        pchecktable->pglobals->pfDbtimeTooLarge,
                        pchecktable->pglobals->popts );

    switch ( err )
    {
        case JET_errNoCurrentRecord:
        case JET_errRecordDeleted:
        case JET_errRecordNotFound:
        case_AllDatabaseStorageCorruptionErrs:
        case JET_errPageNotInitialized:
        case JET_errDiskIO:
        case JET_errFileIOBeyondEOF:
            err = ErrERRCheck( JET_errDatabaseCorrupted );
            break;
        default:
            break;
    }

    if ( JET_errDatabaseCorrupted == err )
    {
        pchecktable->pglobals->fCorruptionSeen = fTrue;
    }
    else if ( err < 0 )
    {
        pchecktable->pglobals->err = err;
    }

    CPRINTF::SetThreadPrintfPrefix( "NULL" );

    if ( pchecktable->fDeleteWhenDone )
    {
        delete pchecktable;
    }
    else
    {
        pchecktable->signal.Set();
    }

    CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
}


LOCAL VOID REPAIRCheckOneTableTask( PIB * const ppib, const ULONG_PTR ul )
{
    TASKMGR::TASK task = REPAIRCheckOneTableTask;

    Unused( task );

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = tceNone;
    tcScope->iorReason.SetIort( iortRepair );

    REPAIRTABLE * prepairtable = NULL;
    CHECKTABLE * const pchecktable = (CHECKTABLE *)ul;

    CallS( ErrDIRBeginTransaction(ppib, 33573, NO_GRBIT ) );

    CPRINTF::SetThreadPrintfPrefix( pchecktable->szTable );

    const ERR err = ErrREPAIRCheckOneTable(
                        ppib,
                        pchecktable->ifmp,
                        pchecktable->szTable,
                        pchecktable->objidFDP,
                        pchecktable->pgnoFDP,
                        pchecktable->cpgPrimaryExtent,
                        &prepairtable,
                        pchecktable->pglobals->pttarrayOwnedSpace,
                        pchecktable->pglobals->pttarrayAvailSpace,
                        pchecktable->pglobals->ppgnocollShelved,
                        pchecktable->pglobals->pfDbtimeTooLarge,
                        pchecktable->pglobals->popts );

    if ( JET_errDatabaseCorrupted == err )
    {
        pchecktable->pglobals->fCorruptionSeen = fTrue;
    }
    else if ( err < 0 )
    {
        pchecktable->pglobals->err = err;
    }

    if ( NULL != prepairtable )
    {
        pchecktable->pglobals->crit.Enter();
        prepairtable->prepairtableNext = *(pchecktable->pglobals->pprepairtable);
        *(pchecktable->pglobals->pprepairtable) = prepairtable;
        pchecktable->pglobals->crit.Leave();
    }

    CPRINTF::SetThreadPrintfPrefix( "NULL" );

    if ( pchecktable->fDeleteWhenDone )
    {
        delete pchecktable;
    }
    else
    {
        pchecktable->signal.Set();
    }

    CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
}


LOCAL ERR ErrREPAIRCheckOneTable(
    PIB * const ppib,
    const IFMP ifmp,
    const char * const szTable,
    const OBJID objidTable,
    const PGNO pgnoFDP,
    const CPG cpgPrimaryExtent,
    REPAIRTABLE ** const pprepairtable,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts )
{
    ERR     err;

    FUCB    *pfucbTable = pfucbNil;

    FCB     *pfcbTable  = pfcbNil;
    FCB     *pfcbIndex  = pfcbNil;

    PGNO    pgnoLV      = pgnoNull;
    OBJID   objidLV     = objidNil;

    BOOL        fRepairTable        = fTrue;
    BOOL        fRepairLV           = fTrue;
    BOOL        fRepairIndexes      = fTrue;
    BOOL        fRepairLVRefcounts  = fTrue;

    const ULONG timerStart          = TickOSTimeCurrent();


    const CPG   cpgToPreread        = min(
                                        CPG( g_rgfmp[ifmp].PgnoLast() - pgnoFDP + 1 ),
                                        max( cpgPrimaryExtent, CpgMinRepairSequentialPreread( g_rgfmp[ifmp].CbPage() ) ) );
    BFPrereadPageRange( ifmp, pgnoFDP, cpgToPreread, bfprfDefault, ppib->BfpriPriority( ifmp ), *TcRepair() );


    err = ErrFILEOpenTable( ppib, ifmp, &pfucbTable, szTable, JET_bitTableTryPurgeOnClose );

    FIDLASTINTDB fidLastInTDB;
    memset( &fidLastInTDB, 0, sizeof(fidLastInTDB) );
    if ( err >= 0)
    {
        Assert( pfucbNil != pfucbTable );
        pfcbTable   = pfucbTable->u.pfcb;

        fidLastInTDB.fidFixedLastInTDB = pfcbTable->Ptdb()->FidFixedLast();
        fidLastInTDB.fidVarLastInTDB = pfcbTable->Ptdb()->FidVarLast();
        fidLastInTDB.fidTaggedLastInTDB = pfcbTable->Ptdb()->FidTaggedLast();
    }

    TTMAP ttmapLVRefcountsFromTable( ppib );
    TTMAP ttmapLVRefcountsFromLV( ppib );

    RECCHECKTABLE   recchecktable(
                        objidTable,
                        pfucbTable,
                        fidLastInTDB,
                        &ttmapLVRefcountsFromTable,
                        popts );

    Call( err );



    REPAIRIPrereadIndexesOfFCB( ppib, pfcbTable );


    Call( ErrCATAccessTableLV( ppib, ifmp, objidTable, &pgnoLV, &objidLV ) );
    if ( pgnoNull != pgnoLV )
    {
        const CPG   cpgToPrereadLV  = min(
                                        CPG( g_rgfmp[ifmp].PgnoLast() - pgnoLV + 1 ),
                                        max( cpgLVTree, CpgMinRepairSequentialPreread( g_rgfmp[ifmp].CbPage() ) ) );
        BFPrereadPageRange( ifmp, pgnoLV, cpgToPrereadLV, bfprfDefault, ppib->BfpriPriority( ifmp ), *TcRepair() );
        Call( ttmapLVRefcountsFromLV.ErrInit( PinstFromPpib( ppib ) ) );
    }

    Call( ttmapLVRefcountsFromTable.ErrInit( PinstFromPpib( ppib ) ) );

    (*popts->pcprintfVerbose)( "checking table \"%s\" (%d)\r\n", szTable, objidTable );
    (*popts->pcprintfVerbose).Indent();

    (*popts->pcprintfStats).Indent();

    Call( ErrREPAIRCheckSpace(
            ppib,
            ifmp,
            objidTable,
            pgnoFDP,
            objidSystemRoot,
            pgnoSystemRoot,
            CPAGE::fPagePrimary,
            fTrue,
            &recchecktable,
            pttarrayOwnedSpace,
            pttarrayAvailSpace,
            ppgnocollShelved,
            pfDbtimeTooLarge,
            popts ) );


    if ( pgnoNull != pgnoLV )
    {
        LVSTATS lvstats;
        memset( &lvstats, 0, sizeof( lvstats ) );
        lvstats.cbLVMin = lMax;
        RECCHECKLV      recchecklv( ttmapLVRefcountsFromLV, popts, pfucbTable->u.pfcb->Ptdb()->CbLVChunkMost() );
        RECCHECKLVSTATS recchecklvstats( &lvstats );
        RECCHECKMACRO   reccheckmacro;
        reccheckmacro.Add( &recchecklvstats );
        reccheckmacro.Add( &recchecklv );
        (*popts->pcprintfVerbose)( "checking long value tree (%d)\r\n", objidLV );
        Call( ErrREPAIRCheckTreeAndSpace(
                ppib,
                ifmp,
                objidLV,
                pgnoLV,
                objidTable,
                pgnoFDP,
                CPAGE::fPageLongValue,
                fTrue,
                &reccheckmacro,
                pttarrayOwnedSpace,
                pttarrayAvailSpace,
                ppgnocollShelved,
                pfDbtimeTooLarge,
                popts ) );

        Call( recchecklv.ErrTerm() );

        (*popts->pcprintfStats)( "\r\n" );
        (*popts->pcprintfStats)( "===== long value tree =====\r\n" );
        REPAIRDumpLVStats( ppib, ifmp, pgnoFDP, &lvstats, popts );
    }
    else
    {
        fRepairLV = fFalse;
    }


    (*popts->pcprintfStats)( "\r\n\r\n" );
    (*popts->pcprintfStats)( "===== table \"%s\" =====\r\n", szTable );

    (*popts->pcprintfVerbose)( "checking data\r\n" );
    Call( ErrREPAIRCheckTree(
            ppib,
            ifmp,
            objidTable,
            pgnoFDP,
            objidSystemRoot,
            pgnoSystemRoot,
            CPAGE::fPagePrimary,
            fTrue,
            &recchecktable,
            pttarrayOwnedSpace,
            pttarrayAvailSpace,
            ppgnocollShelved,
            pfDbtimeTooLarge,
            popts ) );


    for(
        pfcbIndex = pfucbTable->u.pfcb->PfcbNextIndex();
        pfcbNil != pfcbIndex;
        pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        RECCHECKNULL    recchecknull;
        const CHAR * const szIndexName  = pfucbTable->u.pfcb->Ptdb()->SzIndexName( pfcbIndex->Pidb()->ItagIndexName(), pfcbIndex->FDerivedIndex() );
        const OBJID objidIndex = pfcbIndex->ObjidFDP();
        (*popts->pcprintfVerbose)( "checking index \"%s\" (%d)\r\n", szIndexName, objidIndex );
        (*popts->pcprintfStats)( "\r\n" );
        (*popts->pcprintfStats)( "===== index \"%s\" =====\r\n", szIndexName );

        Call( ErrREPAIRCheckTreeAndSpace(
                ppib,
                ifmp,
                pfcbIndex->ObjidFDP(),
                pfcbIndex->PgnoFDP(),
                objidTable,
                pgnoFDP,
                CPAGE::fPageIndex,
                pfcbIndex->Pidb()->FUnique(),
                &recchecknull,
                pttarrayOwnedSpace,
                pttarrayAvailSpace,
                ppgnocollShelved,
                pfDbtimeTooLarge,
                popts ) );
    }



    fRepairTable = fFalse;


    fRepairLV = fFalse;


    if ( pgnoNull != pgnoLV )
    {
        (*popts->pcprintfVerbose)( "checking LV refcounts\r\n" );
        Call( ErrREPAIRCompareLVRefcounts( ppib, ifmp, ttmapLVRefcountsFromTable, ttmapLVRefcountsFromLV, popts ) );
    }
    else
    {
        err = ttmapLVRefcountsFromTable.ErrMoveFirst();
        if ( JET_errNoCurrentRecord != err )
        {
            _LID64 lid;
            ULONG ulRefcount;
            Call( ttmapLVRefcountsFromTable.ErrGetCurrentKeyValue( &lid, &ulRefcount ) );

            (*popts->pcprintfError)( "table has no LV tree but record references LV (lid 0x%I64x, refcount %d)\r\n", lid, ulRefcount );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        else
        {
            err = JET_errSuccess;
        }
    }

    fRepairLVRefcounts  = fFalse;


    if ( !( popts->grbit & JET_bitDBUtilOptionDontBuildIndexes ) )
    {
        if ( pfucbTable->u.pfcb->PfcbNextIndex() != pfcbNil )
        {
            (*popts->pcprintfVerbose)( "rebuilding and comparing indexes\r\n" );
            DIRUp( pfucbTable );

#ifdef PARALLEL_BATCH_INDEX_BUILD
            ULONG   cIndexes    = 0;
            for ( FCB * pfcbT = pfucbTable->u.pfcb->PfcbNextIndex(); pfcbNil != pfcbT; pfcbT = pfcbT->PfcbNextIndex() )
            {
                cIndexes++;
            }
#else
            const ULONG cIndexes    = cFILEIndexBatchSizeDefault;
#endif

            Call( ErrFILEBuildAllIndexes(
                    ppib,
                    pfucbTable,
                    pfucbTable->u.pfcb->PfcbNextIndex(),
                    NULL,
                    cIndexes,
                    fTrue,
                    popts->pcprintfError ) );
        }
    }

    fRepairIndexes = fFalse;

HandleError:
    switch ( err )
    {
        case JET_errNoCurrentRecord:
        case JET_errRecordDeleted:
        case JET_errRecordNotFound:
        case_AllDatabaseStorageCorruptionErrs:
        case JET_errPageNotInitialized:
        case JET_errDiskIO:
        case JET_errFileIOBeyondEOF:
        case JET_errCallbackFailed:
            err = ErrERRCheck( JET_errDatabaseCorrupted );
            break;
        default:
            break;
    }

    ERR errSaved = err;

    if ( JET_errDatabaseCorrupted == err )
    {
        if ( fRepairTable )
        {
            (*popts->pcprintfVerbose)( "\tData table will be rebuilt\r\n" );
        }
        if ( fRepairLV  )
        {
            (*popts->pcprintfVerbose)( "\tLong-Value table will be rebuilt\r\n" );
        }
        if ( fRepairIndexes )
        {
            (*popts->pcprintfVerbose)( "\tIndexes will be rebuilt\r\n" );
        }
        if ( fRepairLVRefcounts )
        {
            (*popts->pcprintfVerbose)( "\tLong-Value refcounts will be rebuilt\r\n" );
        }
    }

    if ( JET_errDatabaseCorrupted == err && !( popts->grbit & JET_bitDBUtilOptionDontRepair ) )
    {

        Assert( pfcbNil != pfcbTable );

        (*popts->pcprintfVerbose)( "table \"%s\" is corrupted\r\n", szTable );
        VOID * pv = PvOSMemoryHeapAlloc( sizeof( REPAIRTABLE ) );
        if ( NULL == pv )
        {
            err = ErrERRCheck( JET_errOutOfMemory );
            goto Abort;
        }
        memset( pv, 0, sizeof( REPAIRTABLE ) );

        REPAIRTABLE * prepairtable = new(pv) REPAIRTABLE;

        OSStrCbCopyA( prepairtable->szTableName, sizeof(prepairtable->szTableName), szTable );

        prepairtable->objidFDP  = objidTable;
        prepairtable->pgnoFDP   = pfcbTable->PgnoFDP();
        (*popts->pcprintfVerbose)( "\tdata: %d (%d)\r\n", prepairtable->objidFDP, prepairtable->pgnoFDP );

        Assert( fRepairTable || fRepairLV || fRepairLVRefcounts || fRepairIndexes );
        prepairtable->fRepairTable          = fRepairTable;
        prepairtable->fRepairLV             = fRepairLV;
        prepairtable->fRepairIndexes        = fRepairIndexes;
        prepairtable->fRepairLVRefcounts    = fRepairLVRefcounts;

        if ( pgnoNull != pgnoLV )
        {
            Assert( objidNil != objidLV );
            prepairtable->objidLV   = objidLV;
            prepairtable->pgnoLV    = pgnoLV;
            (*popts->pcprintfVerbose)( "\tlong values: %d (%d)\r\n", prepairtable->objidLV, prepairtable->pgnoLV );
        }

        for( pfcbIndex = pfucbTable->u.pfcb;
            pfcbNil != pfcbIndex;
            pfcbIndex = pfcbIndex->PfcbNextIndex() )
        {
            if ( !pfcbIndex->FPrimaryIndex() )
            {
                const CHAR * const szIndexName  = pfucbTable->u.pfcb->Ptdb()->SzIndexName( pfcbIndex->Pidb()->ItagIndexName(), pfcbIndex->FDerivedIndex() );
                CallJ( prepairtable->objidlistIndexes.ErrAddObjid( pfcbIndex->ObjidFDP() ), Abort );
                (*popts->pcprintfVerbose)( "\tindex \"%s\": %d (%d)\r\n", szIndexName, pfcbIndex->ObjidFDP(),pfcbIndex->PgnoFDP() );
            }
            else if ( pfcbIndex->Pidb() != pidbNil )
            {
                prepairtable->fHasPrimaryIndex = fTrue;
                (*popts->pcprintfVerbose)( "\tprimary index\r\n" );
            }
        }

        prepairtable->fTemplateTable    = pfcbTable->FTemplateTable();
        prepairtable->fDerivedTable     = pfcbTable->FDerivedTable();

        prepairtable->objidlistIndexes.Sort();
        prepairtable->prepairtableNext = *pprepairtable;
        *pprepairtable = prepairtable;
    }

Abort:
    if ( pfucbNil != pfucbTable )
    {

        CallS( ErrFILECloseTable( ppib, pfucbTable ) );



    }

    (*popts->pcprintfVerbose).Unindent();
    (*popts->pcprintfStats).Unindent();

    const ULONG timerEnd    = TickOSTimeCurrent();
    const ULONG csecElapsed = ( ( timerEnd - timerStart ) / 1000 );

    err = ( JET_errSuccess == err ) ? errSaved : err;

    CPRINTFINDENT * const pcprintfT   = ( JET_errSuccess == err ? popts->pcprintfVerbose
                                                    : err < JET_errSuccess ? popts->pcprintfError
                                                    : popts->pcprintfWarning );
    (*pcprintfT)(
        "integrity-check of table \"%s\" (%d) finishes with error %d (0x%x) after %d seconds\r\n",
        szTable,
        objidTable,
        err,
        err,
        csecElapsed );

    return err;
}


LOCAL ERR ErrREPAIRCompareLVRefcounts(
    PIB * const ppib,
    const IFMP ifmp,
    TTMAP& ttmapLVRefcountsFromTable,
    TTMAP& ttmapLVRefcountsFromLV,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    BOOL fNoMoreRefcountsFromTable  = fFalse;
    BOOL fNoMoreRefcountsFromLV     = fFalse;
    BOOL fSawError                  = fFalse;

    err = ttmapLVRefcountsFromTable.ErrMoveFirst();
    if ( JET_errNoCurrentRecord == err )
    {
        fNoMoreRefcountsFromTable = fTrue;
        err = JET_errSuccess;
    }
    Call( err );

    err = ttmapLVRefcountsFromLV.ErrMoveFirst();
    if ( JET_errNoCurrentRecord == err )
    {
        fNoMoreRefcountsFromLV = fTrue;
        err = JET_errSuccess;
    }
    Call( err );


    while (     !fNoMoreRefcountsFromTable
            || !fNoMoreRefcountsFromLV )
    {
        _LID64 lidFromTable;
        ULONG ulRefcountFromTable;
        if ( !fNoMoreRefcountsFromTable )
        {
            Call( ttmapLVRefcountsFromTable.ErrGetCurrentKeyValue( &lidFromTable, &ulRefcountFromTable ) );
        }
        else
        {
            lidFromTable        = lid64Reserved;
            ulRefcountFromTable = 0;
        }

        _LID64 lidFromLV;
        ULONG ulRefcountFromLV;
        if ( !fNoMoreRefcountsFromLV )
        {
            Call( ttmapLVRefcountsFromLV.ErrGetCurrentKeyValue( &lidFromLV, &ulRefcountFromLV ) );
        }
        else
        {
            lidFromLV           = lid64Reserved;
            ulRefcountFromLV    = 0;
        }

        if ( lidFromTable > lidFromLV )
        {


            (*popts->pcprintfWarning)( "orphaned LV (lid 0x%I64x, refcount %d). Offline defragmentation should fix this.\r\n", lidFromLV, ulRefcountFromLV );

        }
        else if ( lidFromLV > lidFromTable )
        {


            (*popts->pcprintfError)( "record references non-existant LV (lid 0x%I64x, refcount %d)\r\n",
                                    lidFromTable, ulRefcountFromTable );
            fSawError = fTrue;
        }
        else if ( ulRefcountFromTable > ulRefcountFromLV )
        {
            Assert( lidFromTable == lidFromLV );


            (*popts->pcprintfError)( "LV refcount too low (lid 0x%I64x, refcount %d, correct refcount %d)\r\n",
                                        lidFromLV, ulRefcountFromLV, ulRefcountFromTable );

            fSawError = fTrue;
        }
        else if ( ulRefcountFromLV > ulRefcountFromTable )
        {
            Assert( lidFromTable == lidFromLV );


            (*popts->pcprintfWarning)( "LV refcount too high (lid 0x%I64x, refcount %d, correct refcount %d). Offline defragmentation should fix this.\r\n",
                                        lidFromLV, ulRefcountFromLV, ulRefcountFromTable );

        }
        else
        {


            Assert( lidFromTable == lidFromLV );
            Assert( ulRefcountFromTable == ulRefcountFromLV );
            Assert( lid64Reserved != lidFromLV );
        }

        if ( lidFromTable <= lidFromLV )
        {
            err = ttmapLVRefcountsFromTable.ErrMoveNext();
            if ( JET_errNoCurrentRecord == err )
            {
                fNoMoreRefcountsFromTable = fTrue;
                err = JET_errSuccess;
            }
            Call( err );
        }
        if ( lidFromLV <= lidFromTable )
        {
            err = ttmapLVRefcountsFromLV.ErrMoveNext();
            if ( JET_errNoCurrentRecord == err )
            {
                fNoMoreRefcountsFromLV = fTrue;
                err = JET_errSuccess;
            }
            Call( err );

        }
    }

HandleError:
    if ( JET_errSuccess == err && fSawError )
    {
        err = ErrERRCheck( JET_errDatabaseCorrupted );
    }
    return err;
}


LOCAL ERR ErrREPAIRCreateTempTables(
    PIB * const ppib,
    const BOOL fRepairGlobalSpace,
    REPAIRTT * const prepairtt,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    JET_COLUMNDEF   rgcolumndef[3] = {
        { sizeof( JET_COLUMNDEF ), 0, JET_coltypNil, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
        { sizeof( JET_COLUMNDEF ), 0, JET_coltypNil, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
        { sizeof( JET_COLUMNDEF ), 0, JET_coltypNil, 0, 0, 0, 0, 0, JET_bitColumnTTKey }
    };

    rgcolumndef[0].coltyp = JET_coltypLong;
    Call( ErrIsamOpenTempTable(
        reinterpret_cast<JET_SESID>( ppib ),
        rgcolumndef,
        1,
        0,
        JET_bitTTIndexed | JET_bitTTUnique | JET_bitTTScrollable | JET_bitTTUpdatable,
        &prepairtt->tableidBadPages,
        prepairtt->rgcolumnidBadPages,
        JET_cbKeyMost_OLD,
        JET_cbKeyMost_OLD ) );

    rgcolumndef[0].coltyp = JET_coltypLong;
    rgcolumndef[1].coltyp = JET_coltypLong;
    Call( ErrIsamOpenTempTable(
        reinterpret_cast<JET_SESID>( ppib ),
        rgcolumndef,
        2,
        0,
        JET_bitTTIndexed | JET_bitTTUnique | JET_bitTTScrollable| JET_bitTTUpdatable,
        &prepairtt->tableidOwned,
        prepairtt->rgcolumnidOwned,
        JET_cbKeyMost_OLD,
        JET_cbKeyMost_OLD ) );

    rgcolumndef[0].coltyp = JET_coltypLong;
    rgcolumndef[1].coltyp = JET_coltypLongBinary;
    rgcolumndef[2].grbit  = NO_GRBIT;
    rgcolumndef[2].coltyp = JET_coltypLong;
    Call( ErrIsamOpenTempTable(
        reinterpret_cast<JET_SESID>( ppib ),
        rgcolumndef,
        3,
        0,
        JET_bitTTIndexed | JET_bitTTUnique | JET_bitTTScrollable| JET_bitTTUpdatable,
        &prepairtt->tableidUsed,
        prepairtt->rgcolumnidUsed,
        CbKeyMostForPage(),
        CbKeyMostForPage()) );
    rgcolumndef[2].grbit  = rgcolumndef[0].grbit;

    rgcolumndef[0].coltyp = JET_coltypLong;
    rgcolumndef[1].coltyp = JET_coltypLong;
    Call( ErrIsamOpenTempTable(
        reinterpret_cast<JET_SESID>( ppib ),
        rgcolumndef,
        2,
        0,
        JET_bitTTIndexed | JET_bitTTUnique | JET_bitTTScrollable| JET_bitTTUpdatable,
        &prepairtt->tableidAvailable,
        prepairtt->rgcolumnidAvailable,
        JET_cbKeyMost_OLD,
        JET_cbKeyMost_OLD ) );

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRScanDB(
    PIB * const ppib,
    const IFMP ifmp,
    REPAIRTT * const prepairtt,
    DBTIME * const pdbtimeLast,
    OBJID  * const pobjidLast,
    PGNO   * const ppgnoLastOESeen,
    const REPAIRTABLE * const prepairtable,
    const TTARRAY * const pttarrayOwnedSpace,
    const TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const REPAIROPTS * const popts )
{
    const JET_SESID sesid   = reinterpret_cast<JET_SESID>( ppib );
    ERR err = JET_errSuccess;

    const CPG cpgPreread = 256;
    CPG cpgRemaining;

    CPG cpgUninitialized    = 0;
    CPG cpgBad              = 0;

    const PGNO  pgnoLastOE  = *ppgnoLastOESeen;
    const PGNO  pgnoFirst   = 1;
    const PGNO  pgnoLast    = PgnoLast( ifmp );
    PGNO    pgno            = pgnoFirst;

    (*popts->pcprintfVerbose)( "scanning the database from page %d to page %d\r\n", pgnoFirst, pgnoLast );

    popts->psnprog->cunitTotal = pgnoLast;
    popts->psnprog->cunitDone = 0;
    (VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntBegin, NULL );

    BFPrereadPageRange( ifmp, pgnoFirst, min(cpgPreread * 2,pgnoLast-1), bfprfDefault, ppib->BfpriPriority( ifmp ), *TcRepair() );
    cpgRemaining = cpgPreread;

    while ( pgnoLast >= pgno )
    {
        if ( 0 == --cpgRemaining )
        {
            if ( ( pgno + ( cpgPreread * 2 ) ) < pgnoLast )
            {
                BFPrereadPageRange( ifmp, pgno + cpgPreread, cpgPreread, bfprfDefault, ppib->BfpriPriority( ifmp ), *TcRepair() );
            }
            popts->psnprog->cunitDone = pgno;
            (VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntProgress, popts->psnprog );
            cpgRemaining = cpgPreread;
        }

        CSR csr;
        err = csr.ErrGetReadPage(
                        ppib,
                        ifmp,
                        pgno,
                        bflfNoTouch );

        if ( JET_errPageNotInitialized == err )
        {
            ++cpgUninitialized;
            err = JET_errSuccess;
        }
        else if ( FErrIsDbCorruption( err ) || JET_errDiskIO == err )
        {
            ++cpgBad;
            const ERR errT = CPAGE::ErrResetHeader( ppib, ifmp, pgno );
            if ( errT < 0 )
            {
                (*popts->pcprintfVerbose)( "error %d resetting page %d (online backup may not work)\r\n", errT, pgno );
            }
            else
            {
                (*popts->pcprintfVerbose)( "error %d reading page %d. the page has been zeroed out so online backup will work\r\n", err, pgno );
            }
            err = ErrREPAIRInsertBadPageIntoTables( ppib, pgno, prepairtt, prepairtable, popts );
        }
        else if ( ( err >= 0 ) && ( ( pgno <= pgnoLastOE ) || !ppgnocollShelved->FContainsPgno( pgno ) ) )
        {
            *ppgnoLastOESeen = max( pgno, *ppgnoLastOESeen );

            if ( csr.Cpage().Dbtime() > *pdbtimeLast )
            {
                *pdbtimeLast = csr.Cpage().Dbtime();
            }
            if ( csr.Cpage().ObjidFDP() > *pobjidLast )
            {
                *pobjidLast = csr.Cpage().ObjidFDP();
            }

            err = ErrREPAIRInsertPageIntoTables(
                    ppib,
                    ifmp,
                    csr,
                    prepairtt,
                    prepairtable,
                    pttarrayOwnedSpace,
                    pttarrayAvailSpace,
                    popts );
        }
        csr.ReleasePage( fTrue );
        csr.Reset();
        Call( err );
        ++pgno;
    }

    (VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntComplete, NULL );

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRIFixLeafPage(
    PIB * const ppib,
    const IFMP ifmp,
    CSR& csr,
#ifdef SYNC_DEADLOCK_DETECTION
    COwner* const pownerSaved,
#endif
    const REPAIROPTS * const popts)
{
    ERR err = JET_errSuccess;
    KEYDATAFLAGS    rgkdf[2];

    Call( csr.Cpage().ErrCheckPage( popts->pcprintfError ) );

Restart:


    INT iline;
    for( iline = 0; iline < csr.Cpage().Clines(); iline++ )
    {
        csr.SetILine( iline );
        const INT ikdfCurr = iline % 2;
        const INT ikdfPrev = ( iline + 1 ) % 2;

        NDIGetKeydataflags( csr.Cpage(), csr.ILine(), rgkdf + ikdfCurr );

        if ( FNDDeleted( rgkdf[ikdfCurr] ) )
        {
        }
        else
        {

            Call( ErrREPAIRICheckNode(
                    csr.Pgno(),
                    csr.ILine(),
                    (BYTE *)csr.Cpage().PvBuffer(),
                    rgkdf[ikdfCurr],
                    popts ) );

            if ( !csr.Cpage().FLongValuePage()
                && !csr.Cpage().FIndexPage() )
            {
                err = ErrREPAIRICheckRecord(
                        csr.Pgno(),
                        csr.ILine(),
                        (BYTE *)csr.Cpage().PvBuffer(),
                        rgkdf[ikdfCurr],
                        popts );
                if ( JET_errDatabaseCorrupted == err )
                {


#ifdef SYNC_DEADLOCK_DETECTION
                    if ( pownerSaved )
                    {
                        Pcls()->pownerLockHead = pownerSaved;
                    }
#endif

                    Assert( FBFReadLatched( ifmp, csr.Pgno() )
                            || FBFWriteLatched( ifmp, csr.Pgno() ) );

                    if ( csr.Latch() != latchWrite )
                    {
                        err = csr.ErrUpgrade( );
                        Assert( errBFLatchConflict != err );
                        Call( err );
                        csr.Dirty();
                    }

                    csr.Cpage().Delete( csr.ILine() );

                    goto Restart;
                }
            }
        }

        if ( rgkdf[ikdfCurr].key.prefix.Cb() == 1 )
        {
            (*popts->pcprintfError)( "node [%d:%d]: incorrectly compressed key\r\n", csr.Pgno(), csr.ILine() );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        if ( iline > 0 )
        {

            const INT cmp = CmpKey( rgkdf[ikdfPrev].key, rgkdf[ikdfCurr].key );
            if ( cmp > 0 )
            {
                (*popts->pcprintfError)( "node [%d:%d]: nodes out of order on leaf page\r\n", csr.Pgno(), csr.ILine() );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
            else if ( 0 == cmp )
            {
                (*popts->pcprintfError)( "node [%d:%d]: illegal duplicate key on leaf page\r\n", csr.Pgno(), csr.ILine() );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
        }
    }

HandleError:

#ifdef SYNC_DEADLOCK_DETECTION
    if ( pownerSaved )
    {
        Pcls()->pownerLockHead = NULL;
    }
#endif

    return err;
}


LOCAL ERR ErrREPAIRInsertOwned(
    const JET_SESID sesid,
    const OBJID objidOwning,
    const PGNO pgno,
    REPAIRTT * const prepairtt,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

#ifdef REPAIR_DEBUG_VERBOSE_SPACE
    (*popts->pcprintfDebug)( "Inserting page %d (objidOwning: %d) into Owned table\r\n", pgno, objidOwning );
#endif

    Call( ErrDispPrepareUpdate( sesid, prepairtt->tableidOwned, JET_prepInsert ) );
    Call( ErrDispSetColumn(
        sesid,
        prepairtt->tableidOwned,
        prepairtt->rgcolumnidOwned[0],
        (BYTE *)&objidOwning,
        sizeof( objidOwning ),
        0,
        NULL ) );
    Call( ErrDispSetColumn(
        sesid,
        prepairtt->tableidOwned,
        prepairtt->rgcolumnidOwned[1],
        (BYTE *)&pgno,
        sizeof( pgno ),
        0,
        NULL ) );
    Call( ErrDispUpdate( sesid, prepairtt->tableidOwned, NULL, 0, NULL, 0 ) );
    ++(prepairtt->crecordsOwned);

HandleError:
    Assert( err != JET_errKeyDuplicate );
    return err;
}


LOCAL ERR ErrREPAIRInsertPageIntoTables(
    PIB * const ppib,
    const IFMP ifmp,
    CSR& csr,
    REPAIRTT * const prepairtt,
    const REPAIRTABLE * const prepairtable,
    const TTARRAY * const pttarrayOwnedSpace,
    const TTARRAY * const pttarrayAvailSpace,
    const REPAIROPTS * const popts )
{
    ERR         err         = JET_errSuccess;
    const PGNO  pgno        = csr.Pgno();
    const OBJID objidFDP    = csr.Cpage().ObjidFDP();

    const REPAIRTABLE * prepairtableT = NULL;

    BOOL fOwnedPage     = fFalse;
    BOOL fAvailPage     = fFalse;
    BOOL fUsedPage      = fFalse;
    OBJID objidInsert       = objidNil;
    OBJID objidInsertParent = objidNil;

    BOOL    fInTransaction  = fFalse;

    const JET_SESID sesid   = reinterpret_cast<JET_SESID>( ppib );

    prepairtableT = prepairtable;
    while ( prepairtableT )
    {

        if ( csr.Cpage().FLeafPage()
            && !csr.Cpage().FSpaceTree()
            && !csr.Cpage().FEmptyPage()
            && !csr.Cpage().FRepairedPage()
            && csr.Cpage().Clines() > 0
            && objidFDP == prepairtableT->objidFDP
            && csr.Cpage().FPrimaryPage()
            )
        {
            Assert( !csr.Cpage().FPreInitPage() );

            fOwnedPage      = fTrue;
            fAvailPage      = fFalse;
            fUsedPage       = fTrue;

            objidInsert     = prepairtableT->objidFDP;

            break;
        }

        else if ( csr.Cpage().FLeafPage()
            && !csr.Cpage().FSpaceTree()
            && !csr.Cpage().FEmptyPage()
            && !csr.Cpage().FRepairedPage()
            && csr.Cpage().Clines() > 0
            && objidFDP == prepairtableT->objidLV
            && csr.Cpage().FLongValuePage()
            )
        {
            Assert( !csr.Cpage().FPreInitPage() );

            fOwnedPage      = fTrue;
            fAvailPage      = fFalse;
            fUsedPage       = fTrue;

            objidInsert         = objidFDP;
            objidInsertParent   = prepairtableT->objidFDP;

            break;
        }

        else if ( prepairtableT->objidlistIndexes.FObjidPresent( objidFDP )
            && csr.Cpage().FIndexPage())
        {
            fOwnedPage      = fTrue;
            fAvailPage      = fTrue;
            fUsedPage       = fFalse;

            objidInsert     = prepairtableT->objidFDP;

            break;
        }

        else if ( objidFDP == prepairtableT->objidFDP
                 || objidFDP == prepairtableT->objidLV )
        {
            Assert( !csr.Cpage().FLeafPage()
                    || csr.Cpage().FSpaceTree()
                    || csr.Cpage().FEmptyPage()
                    || csr.Cpage().FPreInitPage()
                    || csr.Cpage().FRepairedPage()
                    || csr.Cpage().Clines() == 0 );

            fOwnedPage      = fTrue;
            fAvailPage      = fTrue;
            fUsedPage       = fFalse;

            objidInsert     = prepairtableT->objidFDP;

            break;
        }

        prepairtableT = prepairtableT->prepairtableNext;
    }

    if ( NULL == prepairtableT )
    {
        Assert( !fOwnedPage );
        Assert( !fAvailPage );
        Assert( !fUsedPage );
        return JET_errSuccess;
    }

    Assert( !fUsedPage || fOwnedPage );
    Assert( !fAvailPage || fOwnedPage );
    Assert( !( fUsedPage && fAvailPage ) );

#ifdef SYNC_DEADLOCK_DETECTION
    COwner* const pownerSaved = Pcls()->pownerLockHead;
    Pcls()->pownerLockHead = NULL;
#endif

    OBJID objidAvail;
    Call( pttarrayAvailSpace->ErrGetValue( ppib, pgno, &objidAvail ) );
    objidAvail &= 0x7fffffff;
    if ( objidNil != objidAvail
        && objidInsert != objidAvail )
    {
        (*popts->pcprintfDebug)(
            "page %d (objidFDP: %d, objidInsert: %d) is available to objid %d. ignoring\r\n",
            pgno, objidFDP, objidInsert, objidAvail );

        fOwnedPage  = fFalse;
        fAvailPage  = fFalse;
        fUsedPage   = fFalse;
        err = JET_errSuccess;
        goto HandleError;
    }

    OBJID objidOwning;
    Call( pttarrayOwnedSpace->ErrGetValue( ppib, pgno, &objidOwning ) );
    objidOwning &= 0x7fffffff;
    if ( objidInsert != objidOwning
            && objidFDP != objidOwning
            && objidInsertParent != objidOwning
            && objidSystemRoot != objidOwning )
    {
        (*popts->pcprintfDebug)(
            "page %d (objidFDP: %d, objidInsert: %d) is owned by objid %d. ignoring\r\n",
            pgno, objidFDP, objidInsert, objidOwning );

        fOwnedPage  = fFalse;
        fAvailPage  = fFalse;
        fUsedPage   = fFalse;
        err = JET_errSuccess;
        goto HandleError;
    }

    Call( ErrDIRBeginTransaction( ppib, 49957, NO_GRBIT ) );
    fInTransaction = fTrue;

    if ( fOwnedPage )
    {
        Assert( objidNil != objidInsert );
        Assert( objidInsert != objidInsertParent );
        Assert( fAvailPage || fUsedPage );

        Call( ErrREPAIRInsertOwned( sesid, objidInsert, pgno, prepairtt, popts ) );
        if ( objidNil != objidInsertParent )
        {
            Call( ErrREPAIRInsertOwned( sesid, objidInsertParent, pgno, prepairtt, popts ) );
        }
    }
    else
    {
        Assert( !fAvailPage );
        Assert( !fUsedPage );
    }

    Assert( !fAvailPage || objidNil == objidInsertParent );

    if ( fAvailPage )
    {
        Assert( !fUsedPage );

#ifdef REPAIR_DEBUG_VERBOSE_SPACE
        (*popts->pcprintfDebug)(
            "Inserting page %d (objidFDP: %d, objidInsert: %d) into Available table\r\n",
            pgno, objidFDP, objidInsert );
#endif

        Assert( JET_tableidNil != prepairtt->tableidAvailable );

        Call( ErrDispPrepareUpdate( sesid, prepairtt->tableidAvailable, JET_prepInsert ) );
        Call( ErrDispSetColumn(
            sesid,
            prepairtt->tableidAvailable,
            prepairtt->rgcolumnidAvailable[0],
            (BYTE *)&objidInsert,
            sizeof( objidInsert ),
            0,
            NULL ) );
        Call( ErrDispSetColumn(
            sesid,
            prepairtt->tableidAvailable,
            prepairtt->rgcolumnidAvailable[1],
            (BYTE *)&pgno,
            sizeof( pgno ),
            0,
            NULL ) );
        Call( ErrDispUpdate( sesid, prepairtt->tableidAvailable, NULL, 0, NULL, 0 ) );
        ++(prepairtt->crecordsAvailable);
    }

    else if ( fUsedPage )
    {
        Assert( csr.Cpage().FLeafPage() );
        Assert( csr.Cpage().Clines() > 0 );
        Assert( objidInsert == objidFDP );

        err = ErrREPAIRIFixLeafPage(
                ppib,
                ifmp,
                csr,
            #ifdef SYNC_DEADLOCK_DETECTION
                pownerSaved,
            #endif
                popts );
        if ( JET_errDatabaseCorrupted == err )
        {
            (*popts->pcprintfError)( "page %d: err %d. discarding page\r\n", pgno, err );

            UtilReportEvent(
                    eventWarning,
                    REPAIR_CATEGORY,
                    REPAIR_BAD_PAGE_ID,
                    0, NULL );


            err = JET_errSuccess;
            goto HandleError;
        }
        else if ( 0 == csr.Cpage().Clines() )
        {
            (*popts->pcprintfError)( "page %d: all records were bad. discarding page\r\n", pgno );

            UtilReportEvent(
                    eventWarning,
                    REPAIR_CATEGORY,
                    REPAIR_BAD_PAGE_ID,
                    0, NULL );


            err = JET_errSuccess;
            goto HandleError;
        }

#ifdef REPAIR_DEBUG_VERBOSE_SPACE
        (*popts->pcprintfDebug)( "Inserting page %d (objidFDP: %d) into Used table\r\n", pgno, objidFDP );
#endif
        Call( ErrDispPrepareUpdate( sesid, prepairtt->tableidUsed, JET_prepInsert ) );
        Call( ErrDispSetColumn(
            sesid,
            prepairtt->tableidUsed,
            prepairtt->rgcolumnidUsed[0],
            (BYTE *)&objidFDP,
            sizeof( objidFDP ),
            0,
            NULL ) );

        BYTE rgbKey[cbKeyAlloc+1];
        csr.SetILine( csr.Cpage().Clines() - 1 );
        KEYDATAFLAGS kdf;
        NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdf );
        kdf.key.CopyIntoBuffer( rgbKey, sizeof( rgbKey ) );

        Call( ErrDispSetColumn(
            sesid,
            prepairtt->tableidUsed,
            prepairtt->rgcolumnidUsed[1],
            rgbKey,
            kdf.key.Cb(),
            0,
            NULL ) );
        Call( ErrDispSetColumn(
            sesid,
            prepairtt->tableidUsed,
            prepairtt->rgcolumnidUsed[2],
            (BYTE *)&pgno,
            sizeof( pgno ),
            0,
            NULL ) );
        err = ErrDispUpdate( sesid, prepairtt->tableidUsed, NULL, 0, NULL, 0 );
        if ( JET_errKeyDuplicate == err )
        {
            UtilReportEvent(
                eventWarning,
                REPAIR_CATEGORY,
                REPAIR_BAD_PAGE_ID,
                0, NULL );
            (*popts->pcprintfError)( "page %d: duplicate keys. discarding page\r\n", pgno );
            err = ErrDispPrepareUpdate( sesid, prepairtt->tableidUsed, JET_prepCancel );
        }
        Call( err );
        ++(prepairtt->crecordsUsed);
    }

    Call( ErrDIRCommitTransaction( ppib, 0 ) );
    fInTransaction = fFalse;

HandleError:
    if ( fInTransaction )
    {
        CallS( ErrDIRRollback( ppib ) );
    }

#ifdef SYNC_DEADLOCK_DETECTION
    Pcls()->pownerLockHead = pownerSaved;
#endif

    return err;
}


LOCAL ERR ErrREPAIRInsertBadPageIntoTables(
    PIB * const ppib,
    const PGNO pgno,
    REPAIRTT * const prepairtt,
    const REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    BOOL    fInTransaction  = fFalse;

    const JET_SESID sesid   = reinterpret_cast<JET_SESID>( ppib );

    (*popts->pcprintfDebug)( "Inserting page %d into BadPages table\r\n", pgno );

    UtilReportEvent(
            eventWarning,
            REPAIR_CATEGORY,
            REPAIR_BAD_PAGE_ID,
            0, NULL );

#ifdef SYNC_DEADLOCK_DETECTION
    COwner* const pownerSaved = Pcls()->pownerLockHead;
    Pcls()->pownerLockHead = NULL;
#endif

    Call( ErrDIRBeginTransaction( ppib, 48165, NO_GRBIT ) );
    fInTransaction = fTrue;

    Call( ErrDispPrepareUpdate( sesid, prepairtt->tableidBadPages, JET_prepInsert ) );
    Call( ErrDispSetColumn(
        sesid,
        prepairtt->tableidBadPages,
        prepairtt->rgcolumnidBadPages[0],
        (BYTE *)&pgno,
        sizeof( pgno ),
        0,
        NULL ) );
    Call( ErrDispUpdate( sesid, prepairtt->tableidBadPages, NULL, 0, NULL, 0 ) );
    ++(prepairtt->crecordsBadPages);

    Call( ErrDIRCommitTransaction( ppib, 0 ) );
    fInTransaction = fFalse;

HandleError:
    if ( fInTransaction )
    {
        CallS( ErrDIRRollback( ppib ) );
    }

#ifdef SYNC_DEADLOCK_DETECTION
    Pcls()->pownerLockHead = pownerSaved;
#endif

    return JET_errSuccess;
}


LOCAL ERR ErrREPAIRGetPgnoOEAE(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    PGNO * const ppgnoOE,
    PGNO * const ppgnoAE,
    PGNO * const ppgnoParent,
    const BOOL fUnique,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    *ppgnoOE        = pgnoNull;
    *ppgnoAE        = pgnoNull;
    *ppgnoParent    = pgnoNull;

    CSR csr;
    CallR( csr.ErrGetReadPage(
            ppib,
            ifmp,
            pgnoFDP,
            bflfNoTouch ) );

    LINE line;
    NDGetPtrExternalHeader( csr.Cpage(), &line, noderfSpaceHeader );

    if ( sizeof( SPACE_HEADER ) != line.cb )
    {
        (*popts->pcprintfError)( "page %d: external header is unexpected size. got %d bytes, expected %d\r\n",
                                 pgnoFDP, line.cb, sizeof( SPACE_HEADER ) );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    const SPACE_HEADER * psph;
    psph = reinterpret_cast <SPACE_HEADER *> ( line.pv );

    if ( fUnique != psph->FUnique() )
    {
        (*popts->pcprintfError)( "page %d: external header has wrong unique flag. got %s, expected %s\r\n",
                                    pgnoFDP,
                                    psph->FUnique() ? "UNIQUE" : "NON-UNIQUE",
                                    fUnique ? "UNIQUE" : "NON-UNIQUE"
                                    );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    *ppgnoParent = psph->PgnoParent();

    if ( psph->FSingleExtent() )
    {
        *ppgnoOE = pgnoNull;
        *ppgnoAE = pgnoNull;
    }
    else
    {
        *ppgnoOE = psph->PgnoOE();
        *ppgnoAE = psph->PgnoAE();
        if ( pgnoNull == *ppgnoOE )
        {
            (*popts->pcprintfError)( "page %d: pgnoOE is pgnoNull", pgnoFDP );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        if ( pgnoNull == *ppgnoAE )
        {
            (*popts->pcprintfError)( "page %d: pgnoAE is pgnoNull", pgnoFDP );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        if ( *ppgnoOE == *ppgnoAE )
        {
            (*popts->pcprintfError)( "page %d: pgnoOE and pgnoAE are the same (%d)", pgnoFDP, *ppgnoOE );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        if ( pgnoFDP == *ppgnoOE )
        {
            (*popts->pcprintfError)( "page %d: pgnoOE and pgnoFDP are the same", pgnoFDP );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        if ( pgnoFDP == *ppgnoAE )
        {
            (*popts->pcprintfError)( "page %d: pgnoAE and pgnoFDP are the same", pgnoFDP );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }

HandleError:
    csr.ReleasePage();
    csr.Reset();

    return err;
}


LOCAL ERR ErrREPAIRCheckSplitBuf(
    PIB * const ppib,
    const PGNO pgnoLastBuffer,
    const CPG cpgBuffer,
    const OBJID objidCurrent,
    const OBJID objidParent,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const REPAIROPTS * const popts )
{
    ERR     err                     = JET_errSuccess;
    PGNO    pgnoT;

    Assert( cpgBuffer >= 0 );
    Assert( pgnoLastBuffer <= pgnoSysMax );
    Assert( pgnoLastBuffer >= (ULONG)cpgBuffer );

    TTARRAY::RUN runOwned;
    TTARRAY::RUN runAvail;

    Assert( NULL != pttarrayOwnedSpace );
    Assert( NULL != pttarrayAvailSpace );

    pttarrayOwnedSpace->BeginRun( ppib, &runOwned );
    pttarrayAvailSpace->BeginRun( ppib, &runAvail );

    for ( pgnoT = pgnoLastBuffer - cpgBuffer + 1; pgnoT <= pgnoLastBuffer; pgnoT++ )
    {
        OBJID objid;

        Call( pttarrayOwnedSpace->ErrGetValue( ppib, pgnoT, &objid, &runOwned ) );
        objid &= 0x7fffffff;

        if ( ( objidParent != objid ) &&
             ( objidCurrent != objid ) &&
             ( ( objidSystemRoot != objid ) || !ppgnocollShelved->FContainsPgno( pgnoT ) ) )
        {
            (*popts->pcprintfError)( "space allocation error (OE SPLITBUF): page %d is already owned by objid %d (expected parent objid %d or objid %d)\r\n",
                                            pgnoT, objid, objidParent, objidCurrent );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        if ( objidCurrent != objid )
        {
            Assert( ( objidParent == objid ) || ppgnocollShelved->FContainsPgno( pgnoT ) );
            err = pttarrayOwnedSpace->ErrSetValue( ppib, pgnoT, objidCurrent, &runOwned );
            if ( JET_errRecordNotFound == err )
            {
                if ( ppgnocollShelved->FContainsPgno( pgnoT ) )
                {
                    err = JET_errSuccess;
                }
                else
                {
                    (*popts->pcprintfError)( "space allocation error (OE SPLITBUF): page %d is beyond the end of the database\r\n", pgnoT );
                    err = ErrERRCheck( JET_errDatabaseCorrupted );
                }
            }
            Call( err );
        }

        Call( pttarrayAvailSpace->ErrGetValue( ppib, pgnoT, &objid, &runAvail ) );
        objid &= 0x7fffffff;

        if ( objidNil != objid )
        {
            (*popts->pcprintfError)( "space allocation error (AE SPLITBUF): page %d is available to objid %d (expected 0)\r\n",
                                    pgnoT, objid );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        err = pttarrayAvailSpace->ErrSetValue( ppib, pgnoT, objidCurrent, &runAvail );
        if ( JET_errRecordNotFound == err )
        {
            if ( ppgnocollShelved->FContainsPgno( pgnoT ) )
            {
                err = JET_errSuccess;
            }
            else
            {
                (*popts->pcprintfError)( "space allocation error (AE SPLITBUF): page %d is beyond the end of the database\r\n", pgnoT );
                err = ErrERRCheck( JET_errDatabaseCorrupted );
            }
        }
        Call( err );
    }
HandleError:
    pttarrayOwnedSpace->EndRun( ppib, &runOwned );
    pttarrayAvailSpace->EndRun( ppib, &runAvail );
    return err;
}


LOCAL ERR ErrREPAIRCheckSPLITBUFFERInSpaceTree(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const OBJID objidCurrent,
    const OBJID objidParent,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const REPAIROPTS * const popts )
{
    ERR     err                     = JET_errSuccess;

    Assert( pgnoNull != pgnoFDP );

    SPLIT_BUFFER    spb;

    memset( &spb, 0, sizeof( SPLIT_BUFFER ) );


    CSR csr;
    err = csr.ErrGetReadPage(
                    ppib,
                    ifmp,
                    pgnoFDP,
                    bflfNoTouch );
    if ( err < 0 )
    {
        (*popts->pcprintfError)( "page %d: error %d on read\r\n", pgnoFDP, err );
        Call( err );
    }

    Assert( csr.Cpage().FSpaceTree() );
    Assert( csr.Cpage().FRootPage() );

    LINE line;
    NDGetPtrExternalHeader( csr.Cpage(), &line, noderfWhole );

    if ( 0 == line.cb )
    {
        err = JET_errSuccess;
    }
    else if (sizeof( SPLIT_BUFFER ) == line.cb)
    {
        UtilMemCpy( &spb, line.pv, sizeof( SPLIT_BUFFER ) );
    }
    else
    {
        (*popts->pcprintfError)( "page %d: split buffer is unexpected size. got %d bytes, expected %d\r\n",
                                 pgnoFDP, line.cb, sizeof( SPLIT_BUFFER ) );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( 0 != spb.CpgBuffer1() )
    {
        Call( ErrREPAIRCheckSplitBuf(
                    ppib,
                    spb.PgnoLastBuffer1(),
                    spb.CpgBuffer1(),
                    objidCurrent,
                    objidParent,
                    pttarrayOwnedSpace,
                    pttarrayAvailSpace,
                    ppgnocollShelved,
                    popts ) );
    }

    if ( 0 != spb.CpgBuffer2() )
    {
        Call( ErrREPAIRCheckSplitBuf(
                    ppib,
                    spb.PgnoLastBuffer2(),
                    spb.CpgBuffer2(),
                    objidCurrent,
                    objidParent,
                    pttarrayOwnedSpace,
                    pttarrayAvailSpace,
                    ppgnocollShelved,
                    popts ) );
    }

HandleError:
    csr.ReleasePage();
    csr.Reset();

    return err;
}



LOCAL ERR ErrREPAIRCheckSpace(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objid,
    const PGNO pgnoFDP,
    const OBJID objidParent,
    const PGNO pgnoFDPParent,
    const ULONG fPageFlags,
    const BOOL fUnique,
    RECCHECK * const preccheck,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    BTSTATS* pbtstats = NULL;
    Alloc( pbtstats = new BTSTATS );
    BTSTATS& btstats = *pbtstats;


    PGNO pgnoOE;
    PGNO pgnoAE;
    PGNO pgnoParentActual;
    Call( ErrREPAIRGetPgnoOEAE(
            ppib,
            ifmp,
            pgnoFDP,
            &pgnoOE,
            &pgnoAE,
            &pgnoParentActual,
            fUnique,
            popts ) );

    if ( pgnoNull != pgnoOE )
    {
        PGNO    rgpgno[3];
        rgpgno[0] = pgnoOE;
        rgpgno[1] = pgnoAE;
        rgpgno[2] = pgnoNull;
        BFPrereadPageList( ifmp, rgpgno, bfprfDefault, ppib->BfpriPriority( ifmp ), *TcRepair() );
    }

    if ( pgnoFDPParent != pgnoParentActual )
    {
        (*popts->pcprintfError)( "page %d (objid %d): space corruption. pgno parent is %d, expected %d\r\n",
            pgnoFDP, objid, pgnoParentActual, pgnoFDPParent );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( pgnoNull != pgnoOE )
    {
        RECCHECKSPACEOE reccheckOE( ppib, pttarrayOwnedSpace, pttarrayAvailSpace, ppgnocollShelved, objid, objidParent, popts );
        RECCHECKSPACEAE reccheckAE( ppib, pttarrayOwnedSpace, pttarrayAvailSpace, ppgnocollShelved, objid, objidParent, popts );

        memset( &btstats, 0, sizeof( BTSTATS ) );
        Call( ErrREPAIRCheckTree(
                ppib,
                ifmp,
                pgnoOE,
                objid,
                fPageFlags | CPAGE::fPageSpaceTree,
                &reccheckOE,
                NULL,
                pttarrayAvailSpace,
                ppgnocollShelved,
                fFalse,
                pfDbtimeTooLarge,
                &btstats,
                popts ) );

        memset( &btstats, 0, sizeof( BTSTATS ) );
        Call( ErrREPAIRCheckTree(
                ppib,
                ifmp,
                pgnoAE,
                objid,
                fPageFlags | CPAGE::fPageSpaceTree,
                &reccheckAE,
                pttarrayOwnedSpace,
                pttarrayAvailSpace,
                ppgnocollShelved,
                fFalse,
                pfDbtimeTooLarge,
                &btstats,
                popts ) );

        Call( ErrREPAIRCheckSPLITBUFFERInSpaceTree(
                ppib,
                ifmp,
                pgnoOE,
                objid,
                objidParent,
                pttarrayOwnedSpace,
                pttarrayAvailSpace,
                ppgnocollShelved,
                popts ) );
        Call( ErrREPAIRCheckSPLITBUFFERInSpaceTree(
                ppib,
                ifmp,
                pgnoAE,
                objid,
                objidParent,
                pttarrayOwnedSpace,
                pttarrayAvailSpace,
                ppgnocollShelved,
                popts ) );

    }
    else
    {
        Call( ErrREPAIRInsertSEInfo(
                ppib,
                ifmp,
                pgnoFDP,
                objid,
                objidParent,
                pttarrayOwnedSpace,
                pttarrayAvailSpace,
                ppgnocollShelved,
                popts ) );
    }

HandleError:
    delete pbtstats;
    return err;
}


LOCAL ERR ErrREPAIRCheckTree(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objid,
    const PGNO pgnoFDP,
    const OBJID objidParent,
    const PGNO pgnoFDPParent,
    const ULONG fPageFlags,
    const BOOL fUnique,
    RECCHECK * const preccheck,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    BTSTATS* pbtstats = NULL;
    Alloc( pbtstats = new BTSTATS );
    BTSTATS& btstats = *pbtstats;

    memset( &btstats, 0, sizeof( BTSTATS ) );
    Call( ErrREPAIRCheckTree(
            ppib,
            ifmp,
            pgnoFDP,
            objid,
            fPageFlags,
            preccheck,
            pttarrayOwnedSpace,
            pttarrayAvailSpace,
            ppgnocollShelved,
            !fUnique,
            pfDbtimeTooLarge,
            &btstats,
            popts ) );

    if ( popts->grbit & JET_bitDBUtilOptionStats )
    {
        REPAIRDumpStats( ppib, ifmp, pgnoFDP, &btstats, popts );
    }

HandleError:
    delete pbtstats;
    return err;
}


LOCAL ERR ErrREPAIRCheckTreeAndSpace(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objid,
    const PGNO pgnoFDP,
    const OBJID objidParent,
    const PGNO pgnoFDPParent,
    const ULONG fPageFlags,
    const BOOL fUnique,
    RECCHECK * const preccheck,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    Call( ErrREPAIRCheckSpace(
            ppib,
            ifmp,
            objid,
            pgnoFDP,
            objidParent,
            pgnoFDPParent,
            fPageFlags,
            fUnique,
            preccheck,
            pttarrayOwnedSpace,
            pttarrayAvailSpace,
            ppgnocollShelved,
            pfDbtimeTooLarge,
            popts ) );

    Call( ErrREPAIRCheckTree(
            ppib,
            ifmp,
            objid,
            pgnoFDP,
            objidParent,
            pgnoFDPParent,
            fPageFlags,
            fUnique,
            preccheck,
            pttarrayOwnedSpace,
            pttarrayAvailSpace,
            ppgnocollShelved,
            pfDbtimeTooLarge,
            popts ) );

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRInsertSEInfo(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const OBJID objid,
    const OBJID objidParent,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const REPAIROPTS * const popts )
{
    JET_ERR err = JET_errSuccess;
    CSR csr;

    CallR( csr.ErrGetReadPage(
            ppib,
            ifmp,
            pgnoFDP,
            bflfNoTouch ) );

    LINE line;
    NDGetPtrExternalHeader( csr.Cpage(), &line, noderfSpaceHeader );

    Assert( sizeof( SPACE_HEADER ) == line.cb );

    const SPACE_HEADER * const psph = reinterpret_cast <SPACE_HEADER *> ( line.pv );
    Assert( psph->FSingleExtent() );

    const CPG cpgOE = psph->CpgPrimary();
    const PGNO pgnoOELast = pgnoFDP + cpgOE - 1;

    Call( ErrREPAIRInsertOERunIntoTT(
        ppib,
        pgnoOELast,
        cpgOE,
        objid,
        objidParent,
        pttarrayOwnedSpace,
        pttarrayAvailSpace,
        ppgnocollShelved,
        fFalse,
        popts ) );

HandleError:
    csr.ReleasePage();
    csr.Reset();

    return err;
}


LOCAL ERR ErrREPAIRInsertOERunIntoTT(
    PIB * const ppib,
    const PGNO pgnoLast,
    const CPG cpgRun,
    const OBJID objid,
    const OBJID objidParent,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const BOOL  fOddRun,
    const REPAIROPTS * const popts )
{
    JET_ERR         err         = JET_errSuccess;
    BOOL            fOddRunT    = fFalse;

    TTARRAY::RUN    runOwned;
    TTARRAY::RUN    runAvail;

    pttarrayOwnedSpace->BeginRun( ppib, &runOwned );
    pttarrayAvailSpace->BeginRun( ppib, &runAvail );

    for( PGNO pgno = pgnoLast; pgno > pgnoLast - cpgRun; --pgno )
    {
        if ( objidNil != objidParent )
        {
            OBJID objidOwning;
            Call( pttarrayOwnedSpace->ErrGetValue( ppib, pgno, &objidOwning, &runOwned ) );

            if ( pgno == pgnoLast )
            {

                fOddRunT = !!( objidOwning & 0x80000000 );
            }
            else if ( fOddRunT != !!( objidOwning & 0x80000000 ) )
            {

                (*popts->pcprintfWarning)(  "space allocation error (OE): page %d spans OE extents in the parent. it is part of a run of %d pages"
                                            " ending at page %d. the page is owned by objid %d (parent ojbid %d, expected owning objid %d)\r\n",
                                            pgno, cpgRun, pgnoLast, ( objidOwning & 0x7FFFFFFF ), objidParent, objid );

            }
            objidOwning &= 0x7FFFFFFF;

            if ( ( objidParent != objidOwning ) && ( ( objidSystemRoot != objidOwning ) || !ppgnocollShelved->FContainsPgno( pgno ) ) )
            {
                (*popts->pcprintfError)( "space allocation error (OE): page %d is already owned by objid %d, (expected parent objid %d for objid %d)\r\n",
                                                pgno, objidOwning, objidParent, objid );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
        }


        OBJID objidAvail;
        Call( pttarrayAvailSpace->ErrGetValue( ppib, pgno, &objidAvail, &runAvail ) );
        objidAvail &= 0x7fffffff;
        if ( objidNil != objidAvail )
        {
            (*popts->pcprintfError)( "space allocation error (OE): page %d is available to objid %d\r\n",
                                        pgno, objidAvail );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }


        const OBJID objidT = objid | ( fOddRun ? 0x80000000 : 0 );
        err = pttarrayOwnedSpace->ErrSetValue( ppib, pgno, objidT, &runOwned );
        if ( JET_errRecordNotFound == err )
        {
            if ( ( objid != objidSystemRoot ) && ppgnocollShelved->FContainsPgno( pgno ) )
            {
                err = JET_errSuccess;
            }
            else
            {
                (*popts->pcprintfError)( "space allocation error (OE): page %d is beyond the end of the database\r\n", pgno );
                err = ErrERRCheck( JET_errDatabaseCorrupted );
            }
        }
        Call( err );
    }

HandleError:
    pttarrayOwnedSpace->EndRun( ppib, &runOwned );
    pttarrayAvailSpace->EndRun( ppib, &runAvail );
    return err;
}


LOCAL ERR ErrREPAIRInsertAERunIntoTT(
    PIB * const ppib,
    const PGNO pgnoLast,
    const CPG cpgRun,
    const SpacePool sppPool,
    const OBJID objid,
    const OBJID objidParent,
    TTARRAY * const pttarrayOwnedSpace,
    TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const BOOL fOddRun,
    const REPAIROPTS * const popts )
{
    JET_ERR         err         = JET_errSuccess;
    BOOL            fOddRunT    = fFalse;

    TTARRAY::RUN    runOwned;
    TTARRAY::RUN    runAvail;

    pttarrayOwnedSpace->BeginRun( ppib, &runOwned );
    pttarrayAvailSpace->BeginRun( ppib, &runAvail );

    if ( ( sppPool == spp::ShelvedPool ) && ( ( objid != objidSystemRoot ) || ( objidParent != objidNil ) ) )
    {
        (*popts->pcprintfError)( "shelved pages (%I32u - %I32u) are unexpected outside of the database root, but were found under objid %I32d\r\n",
                                 pgnoLast - cpgRun + 1,
                                 pgnoLast,
                                 objid );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    for ( PGNO pgno = pgnoLast; pgno > pgnoLast - cpgRun; --pgno )
    {

        OBJID   objidOwning;
        Call( pttarrayOwnedSpace->ErrGetValue( ppib, pgno, &objidOwning, &runOwned ) );

        if ( pgno == pgnoLast )
        {
            fOddRunT = !!( objidOwning & 0x80000000 );
        }
        else if ( fOddRunT != !!( objidOwning & 0x80000000 ) )
        {
            (*popts->pcprintfError)(    "space allocation error (AE): page %d spans OE extents. it is part of a run of %d pages"
                                        " ending at page %d. the page is owned by objid %d\r\n",
                                        pgno, cpgRun, pgnoLast, objidOwning );


        }
        objidOwning &= 0x7FFFFFFF;

        if ( ( objid != objidOwning ) && ( ( objidSystemRoot != objidOwning ) || !ppgnocollShelved->FContainsPgno( pgno ) ) )
        {
            (*popts->pcprintfError)( "space allocation error (AE): page %d is owned by objid %d, (expected objid %d)\r\n",
                                        pgno, objidOwning, objid );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        Call( pttarrayAvailSpace->ErrGetValue( ppib, pgno, &objidOwning, &runAvail ) );
        objidOwning &= 0x7fffffff;
        if ( ( objidNil != objidOwning ) && ( ( objidSystemRoot != objidOwning ) || ( sppPool != spp::ShelvedPool ) ) )
        {
            (*popts->pcprintfError)( "space allocation error (AE): page %d is available to objid %d\r\n",
                                        pgno, objidOwning );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        Assert( ( sppPool != spp::ShelvedPool ) || ( ( objid == objidSystemRoot ) && ( objidParent == objidNil ) ) );
        if ( sppPool != spp::ShelvedPool )
        {
            err = pttarrayAvailSpace->ErrSetValue( ppib, pgno, objid, &runAvail );
            if ( JET_errRecordNotFound == err )
            {
                if ( ( objid != objidSystemRoot ) && ppgnocollShelved->FContainsPgno( pgno ) )
                {
                    err = JET_errSuccess;
                }
                else
                {
                    (*popts->pcprintfError)( "space allocation error (AE): page %d is beyond the end of the database\r\n", pgno );
                    err = ErrERRCheck( JET_errDatabaseCorrupted );
                }
            }
        }
        else if ( ( objid == objidSystemRoot ) && ( objidParent == objidNil ) )
        {
            Call( ppgnocollShelved->ErrAddPgno( pgno ) );
        }
        Call( err );
    }

HandleError:
    pttarrayOwnedSpace->EndRun( ppib, &runOwned );
    pttarrayAvailSpace->EndRun( ppib, &runAvail );
    return err;
}


LOCAL ERR ErrREPAIRRecheckSpaceTreeAndSystemTablesSpace(
    PIB * const ppib,
    const IFMP ifmp,
    const CPG cpgDatabase,
    BOOL * const pfSpaceTreeCorrupt,
    TTARRAY ** const ppttarrayOwnedSpace,
    TTARRAY ** const ppttarrayAvailSpace,
    PgnoCollection ** const pppgnocollShelved,
    BOOL * const pfDbtimeTooLarge,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    PGNO pgnoLastOE = pgnoNull;
    INST * const pinst  = PinstFromPpib( ppib );

    const FIDLASTINTDB fidLastInTDB = { fidMSO_FixedLast, fidMSO_VarLast, fidMSO_TaggedLast };

    RECCHECKNULL    recchecknull;
    RECCHECKTABLE   recchecktable( objidNil, pfucbNil, fidLastInTDB, NULL, popts );

    delete *ppttarrayOwnedSpace;
    delete *ppttarrayAvailSpace;
    delete *pppgnocollShelved;

    *ppttarrayOwnedSpace    = new TTARRAY( cpgDatabase + 1, objidSystemRoot );
    *ppttarrayAvailSpace    = new TTARRAY( cpgDatabase + 1, objidNil );
    *pppgnocollShelved      = new PgnoCollection( max( 1, min( 1024, cpgDatabase / 10000 ) ) );

    if ( NULL == *ppttarrayOwnedSpace
        || NULL == *ppttarrayAvailSpace
        || NULL == *pppgnocollShelved )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    Call( (*ppttarrayOwnedSpace)->ErrInit( pinst ) );
    Call( (*ppttarrayAvailSpace)->ErrInit( pinst ) );

    Call( ErrREPAIRCheckGlobalSpaceTree(
            ppib,
            ifmp,
            pfSpaceTreeCorrupt,
            &pgnoLastOE,
            *ppttarrayOwnedSpace,
            *ppttarrayAvailSpace,
            *pppgnocollShelved,
            pfDbtimeTooLarge,
            popts ) );

    Call( ErrREPAIRCheckSpace(
            ppib,
            ifmp,
            objidFDPMSO,
            pgnoFDPMSO,
            objidSystemRoot,
            pgnoSystemRoot,
            CPAGE::fPagePrimary,
            fTrue,
            &recchecktable,
            *ppttarrayOwnedSpace,
            *ppttarrayAvailSpace,
            *pppgnocollShelved,
            pfDbtimeTooLarge,
            popts ) );

    Call( ErrREPAIRCheckSpace(
            ppib,
            ifmp,
            objidFDPMSO_NameIndex,
            pgnoFDPMSO_NameIndex,
            objidFDPMSO,
            pgnoFDPMSO,
            CPAGE::fPageIndex,
            fTrue,
            &recchecknull,
            *ppttarrayOwnedSpace,
            *ppttarrayAvailSpace,
            *pppgnocollShelved,
            pfDbtimeTooLarge,
            popts ) );
    Call( ErrREPAIRCheckSpace(
            ppib,
            ifmp,
            objidFDPMSO_RootObjectIndex,
            pgnoFDPMSO_RootObjectIndex,
            objidFDPMSO,
            pgnoFDPMSO,
            CPAGE::fPageIndex,
            fTrue,
            &recchecknull,
            *ppttarrayOwnedSpace,
            *ppttarrayAvailSpace,
            *pppgnocollShelved,
            pfDbtimeTooLarge,
            popts ) );
    Call( ErrREPAIRCheckSpace(
            ppib,
            ifmp,
            objidFDPMSOShadow,
            pgnoFDPMSOShadow,
            objidSystemRoot,
            pgnoSystemRoot,
            CPAGE::fPagePrimary,
            fTrue,
            &recchecktable,
            *ppttarrayOwnedSpace,
            *ppttarrayAvailSpace,
            *pppgnocollShelved,
            pfDbtimeTooLarge,
            popts ) );

    err = JET_errSuccess;

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRCheckTree(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoRoot,
    const OBJID objidFDP,
    const ULONG fPageFlags,
    RECCHECK * const preccheck,
    const TTARRAY * const pttarrayOwnedSpace,
    const TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const BOOL fNonUnique,
    BOOL * const pfDbtimeTooLarge,
    BTSTATS * const pbtstats,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    const ULONG fFlagsFDP = fPageFlags;

    pbtstats->pgnoLastSeen = pgnoNull;

    CSR csr;
    err = csr.ErrGetReadPage(
                    ppib,
                    ifmp,
                    pgnoRoot,
                    bflfNoTouch );
    if ( err < 0 )
    {
        (*popts->pcprintfError)( "page %d: error %d on read\r\n", pgnoRoot, err );
        Call( err );
    }

    if ( !csr.Cpage().FRootPage() )
    {
        (*popts->pcprintfError)( "page %d: pgnoRoot is not a root page\r\n", pgnoRoot );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    csr.SetILine( 0 );

    Call( ErrREPAIRICheck(
            ppib,
            ifmp,
            objidFDP,
            fFlagsFDP,
            csr,
            fFalse,
            preccheck,
            pttarrayOwnedSpace,
            pttarrayAvailSpace,
            ppgnocollShelved,
            fNonUnique,
            pfDbtimeTooLarge,
            pbtstats,
            NULL,
            NULL,
            popts ) );

    if ( pgnoNull != pbtstats->pgnoNextExpected )
    {
        (*popts->pcprintfError)( "page %d: corrupt leaf links\r\n", pbtstats->pgnoLastSeen );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

HandleError:
    csr.ReleasePage( fTrue );
    csr.Reset();

    return err;
}


LOCAL ERR ErrREPAIRICheck(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objidFDP,
    const ULONG fFlagsFDP,
    CSR& csr,
    const BOOL fPrereadSibling,
    RECCHECK * const preccheck,
    const TTARRAY * const pttarrayOwnedSpace,
    const TTARRAY * const pttarrayAvailSpace,
    PgnoCollection * const ppgnocollShelved,
    const BOOL fNonUnique,
    BOOL * const pfDbtimeTooLarge,
    BTSTATS * const pbtstats,
    const BOOKMARK * const pbookmarkCurrParent,
    const BOOKMARK * const pbookmarkPrevParent,
    const REPAIROPTS * const popts )
{
    ERR             err     = JET_errSuccess;

    const ULONG cunitDone = AtomicIncrement( (LONG *)(&popts->psnprog->cunitDone) );
    const BOOL fHitBoundary = ( ( popts->psnprog->cunitTotal >= 100 ) ?
                ( 0 == ( cunitDone % ( popts->psnprog->cunitTotal / 100 ) ) ) :
                ( 0 == ( cunitDone % ( 100 / popts->psnprog->cunitTotal ) ) ) );
    if ( fHitBoundary
        && popts->crit.FTryEnter() )
    {
        if ( fHitBoundary )
        {
            popts->psnprog->cunitDone = min( popts->psnprog->cunitDone, popts->psnprog->cunitTotal );
            (VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntProgress, popts->psnprog );
        }
        popts->crit.Leave();
    }

    Call( csr.Cpage().ErrCheckPage( popts->pcprintfError, CPAGE::OnErrorReturnError, CPAGE::CheckAll ) );

    if ( csr.Cpage().ObjidFDP() != objidFDP )
    {
        (*popts->pcprintfError)( "page %d: page belongs to different tree (objid is %d, should be %d)\r\n",
                                    csr.Pgno(),
                                    csr.Cpage().ObjidFDP(),
                                    objidFDP );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( ( csr.Cpage().FFlags() | fFlagsFDP ) != csr.Cpage().FFlags() )
    {
        (*popts->pcprintfError)( "page %d: page flag mismatch with FDP (current flags: 0x%x, FDP flags 0x%x)\r\n",
                                    csr.Pgno(), csr.Cpage().FFlags(), fFlagsFDP );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( csr.Cpage().Dbtime() > g_rgfmp[ifmp].Pdbfilehdr()->le_dbtimeDirtied )
    {
        (*popts->pcprintfError)( "page %d: dbtime is larger than database dbtime (0x%I64x, 0x%I64x)\r\n",
                                csr.Pgno(), csr.Cpage().Dbtime(), (DBTIME)g_rgfmp[ifmp].Pdbfilehdr()->le_dbtimeDirtied );
        *pfDbtimeTooLarge = fTrue;
    }


    OBJID objid;
    if ( pttarrayOwnedSpace )
    {
        Call( pttarrayOwnedSpace->ErrGetValue( ppib, csr.Pgno(), &objid, NULL ) );
        objid &= 0x7fffffff;
        if ( csr.Cpage().ObjidFDP() != objid )
        {
            (*popts->pcprintfError)( "page %d: space allocation error. page is owned by objid %d, expecting %d\r\n",
                                        csr.Pgno(), objid, csr.Cpage().ObjidFDP() );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }
    Assert( ( pttarrayAvailSpace == NULL ) == ( ppgnocollShelved == NULL ) );
    if ( pttarrayAvailSpace && ppgnocollShelved )
    {
        Call( pttarrayAvailSpace->ErrGetValue( ppib, csr.Pgno(), &objid, NULL ) );
        objid &= 0x7fffffff;
        if ( objidNil != objid )
        {
            (*popts->pcprintfError)( "page %d: space allocation error. page is available to objid %d\r\n", csr.Pgno(), objid );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }

    if ( csr.Cpage().FLeafPage() )
    {
        if ( fPrereadSibling )
        {
            const PGNO pgnoNext = csr.Cpage().PgnoNext();
            if ( pgnoNull != pgnoNext )
            {
                BFPrereadPageRange( ifmp, pgnoNext, CpgMinRepairSequentialPreread( g_rgfmp[ ifmp ].CbPage() ), bfprfDefault, ppib->BfpriPriority( ifmp ), *TcRepair() );
            }
        }

        Call( ErrREPAIRCheckLeaf(
                ppib,
                ifmp,
                csr,
                preccheck,
                fNonUnique,
                pbtstats,
                pbookmarkCurrParent,
                pbookmarkPrevParent,
                popts ) );
    }
    else
    {
        if ( !csr.Cpage().FInvisibleSons() )
        {
            (*popts->pcprintfError)( "page %d: not an internal page\r\n" );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }


        Call( ErrREPAIRCheckInternal(
                ppib,
                ifmp,
                csr,
                pbtstats,
                pbookmarkCurrParent,
                pbookmarkPrevParent,
                popts ) );

        PGNO        rgpgno[g_cbPageMax/(sizeof(PGNO) + cbNDNullKeyData )];
        const INT   cpgno = csr.Cpage().Clines();

        if ( cpgno >= (sizeof(rgpgno)/sizeof(PGNO)) )
        {
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        INT iline;
        AssertPREFIX( cpgno <= _countof( rgpgno ) );
        for( iline = 0; iline < cpgno; iline++ )
        {
            csr.SetILine( iline );

            KEYDATAFLAGS kdf;
            NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdf );
            rgpgno[iline] = *((UnalignedLittleEndian< PGNO > *)kdf.data.Pv() );
        }

        AssertPREFIX( cpgno < ( sizeof( rgpgno ) / sizeof( PGNO ) ) );
        rgpgno[cpgno] = pgnoNull;

        BFPrereadPageList( ifmp, rgpgno, bfprfDefault, ppib->BfpriPriority( ifmp ), *TcRepair() );

        BOOKMARK    rgbookmark[2];
        BOOKMARK    *rgpbookmark[2];
        rgpbookmark[0] = NULL;
        rgpbookmark[1] = NULL;

        BOOL    fChildrenAreLeaf            = fFalse;
        BOOL    fChildrenAreParentOfLeaf    = fFalse;
        BOOL    fChildrenAreInternal        = fFalse;

        for( INT ipgno = 0; ipgno < cpgno; ipgno++ )
        {
            csr.SetILine( ipgno );

            KEYDATAFLAGS kdf;
            NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdf );

            const INT ibookmarkCurr = ipgno % 2;
            const INT ibookmarkPrev = ( ipgno + 1 ) % 2;
            Assert( ibookmarkCurr != ibookmarkPrev );
            Assert( rgpbookmark[ibookmarkPrev] != NULL || 0 == ipgno );

            rgbookmark[ibookmarkCurr].key   = kdf.key;
            rgbookmark[ibookmarkCurr].data  = kdf.data;
            rgpbookmark[ibookmarkCurr]      = &rgbookmark[ibookmarkCurr];

            if ( rgbookmark[ibookmarkCurr].key.FNull() )
            {
                if ( cpgno-1 != ipgno )
                {
                    (*popts->pcprintfError)( "node [%d:%d]: NULL key is not last key in page\r\n", csr.Pgno(), csr.ILine() );
                    Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
                }
                rgpbookmark[ibookmarkCurr] = NULL;
            }

            CSR csrChild;
            err = csrChild.ErrGetReadPage(
                                ppib,
                                ifmp,
                                rgpgno[ipgno],
                                bflfNoTouch );
            if ( err < 0 )
            {
                (*popts->pcprintfError)( "page %d: error %d on read\r\n", rgpgno[ipgno], err );
                goto HandleError;
            }

            err = ErrREPAIRICheck(
                    ppib,
                    ifmp,
                    objidFDP,
                    fFlagsFDP,
                    csrChild,
                    ( cpgno - 1 == ipgno ),
                    preccheck,
                    pttarrayOwnedSpace,
                    pttarrayAvailSpace,
                    ppgnocollShelved,
                    fNonUnique,
                    pfDbtimeTooLarge,
                    pbtstats,
                    rgpbookmark[ibookmarkCurr],
                    rgpbookmark[ibookmarkPrev],
                    popts );

            if ( err < 0 )
            {
                (*popts->pcprintfError)( "node [%d:%d]: subtree check (page %d) failed with err %d\r\n", csr.Pgno(), csr.ILine(), rgpgno[ipgno], err );
            }
            else if ( 0 == ipgno )
            {
                fChildrenAreLeaf            = csrChild.Cpage().FLeafPage();
                fChildrenAreParentOfLeaf    = csrChild.Cpage().FParentOfLeaf();
                fChildrenAreInternal        = !fChildrenAreLeaf && !fChildrenAreParentOfLeaf;

                if ( csr.Cpage().FParentOfLeaf() && !fChildrenAreLeaf )
                {
                    err = ErrERRCheck( JET_errDatabaseCorrupted );
                    (*popts->pcprintfError)( "page %d: child (%d) is not a leaf page but parent is parent-of-leaf\r\n", csr.Pgno(), rgpgno[ipgno] );
                }
                else if ( !csr.Cpage().FParentOfLeaf() && fChildrenAreLeaf )
                {
                    err = ErrERRCheck( JET_errDatabaseCorrupted );
                    (*popts->pcprintfError)( "page %d: child (%d) is a leaf page but parent is not parent-of-leaf\r\n", csr.Pgno(), rgpgno[ipgno] );
                }
            }
            else
            {
                if ( csrChild.Cpage().FLeafPage() != fChildrenAreLeaf )
                {
                    err = ErrERRCheck( JET_errDatabaseCorrupted );
                    (*popts->pcprintfError)( "node [%d:%d]: b-tree depth different (page %d, page %d) expected child (%d) to be leaf\r\n", csr.Pgno(), csr.ILine(), rgpgno[0], rgpgno[ipgno], csrChild.Pgno() );
                }
                else if ( csrChild.Cpage().FParentOfLeaf() != fChildrenAreParentOfLeaf )
                {
                    err = ErrERRCheck( JET_errDatabaseCorrupted );
                    (*popts->pcprintfError)( "node [%d:%d]: b-tree depth different (page %d, page %d) expected child (%d) to be parent-of-leaf\r\n", csr.Pgno(), csr.ILine(), rgpgno[0], rgpgno[ipgno], csrChild.Pgno() );
                }
                else if ( fChildrenAreInternal && ( csrChild.Cpage().FLeafPage() || csrChild.Cpage().FParentOfLeaf() ) )
                {
                    err = ErrERRCheck( JET_errDatabaseCorrupted );
                    (*popts->pcprintfError)( "node [%d:%d]: b-tree depth different (page %d, page %d) expected child (%d) to be internal\r\n", csr.Pgno(), csr.ILine(), rgpgno[0], rgpgno[ipgno], csrChild.Pgno() );
                }
            }

            csrChild.ReleasePage( fTrue );
            csrChild.Reset();
            Call( err );
        }
    }

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRICheckNode(
    const PGNO pgno,
    const INT iline,
    const BYTE * const pbPage,
    const KEYDATAFLAGS& kdf,
    const REPAIROPTS * const popts )
{
    ERR         err = JET_errSuccess;

    if ( (ULONG) kdf.key.Cb() > (ULONG) CbKeyMostForPage() * 2 )
    {
        (*popts->pcprintfError)( "node [%d:%d]: key is too long (%d bytes)\r\n", pgno, iline, kdf.key.Cb() );
        CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }
    if ( kdf.data.Cb() > g_cbPage )
    {
        (*popts->pcprintfError)( "node [%d:%d]: data is too long (%d bytes)\r\n", pgno, iline, kdf.data.Cb() );
        CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }
    if ( ( kdf.key.Cb() + kdf.data.Cb() ) > g_cbPage )
    {
        (*popts->pcprintfError)( "node [%d:%d]: node is too big (%d bytes)\r\n", pgno, iline, kdf.key.Cb() + kdf.data.Cb() );
        CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    const BYTE * const pbKeyPrefix = (BYTE *)kdf.key.prefix.Pv();
    const BYTE * const pbKeySuffix = (BYTE *)kdf.key.suffix.Pv();
    const BYTE * const pbData = (BYTE *)kdf.data.Pv();

    if ( FNDCompressed( kdf ) )
    {
        if ( pbKeyPrefix < pbPage
            || pbKeyPrefix + kdf.key.prefix.Cb() >= pbPage + g_cbPage )
        {
            (*popts->pcprintfError)( "node [%d:%d]: prefix not on page\r\n", pgno, iline );
            CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }

    if ( kdf.key.suffix.Cb() > 0 )
    {
        if ( pbKeySuffix < pbPage
            || pbKeySuffix + kdf.key.suffix.Cb() >= pbPage + g_cbPage )
        {
            (*popts->pcprintfError)( "node [%d:%d]: suffix not on page\r\n", pgno, iline );
            CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }

    if ( kdf.data.Cb() > 0 )
    {
        if ( pbData < pbPage
            || pbData + kdf.data.Cb() >= pbPage + g_cbPage )
        {
            (*popts->pcprintfError)( "node [%d:%d]: data not on page\r\n", pgno, iline );
            CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }

    return err;
}


LOCAL ERR ErrREPAIRICheckRecord(
    const PGNO pgno,
    const INT iline,
    const BYTE * const pbPage,
    const KEYDATAFLAGS& kdf,
    const REPAIROPTS * const popts )
{
    ERR     err = JET_errSuccess;
    const FIDLASTINTDB fidLastInTDB = { fidFixedMost, fidVarMost, fidTaggedMost };

    RECCHECKTABLE   reccheck( objidNil, pfucbNil, fidLastInTDB, NULL, popts );

    Call( reccheck.ErrCheckRecord( kdf ) );

HandleError:
    return err;

}


LOCAL INT LREPAIRHandleException(
    const PIB * const ppib,
    const IFMP ifmp,
    const CSR& csr,
    const EXCEPTION exception,
    const REPAIROPTS * const popts )
{
    const DWORD dwExceptionId           = ExceptionId( exception );
    const EExceptionFilterAction efa    = efaExecuteHandler;

    (*popts->pcprintfError)( "node [%d:%d]: caught exception 0x%x\r\n",
                csr.Pgno(),
                csr.ILine(),
                dwExceptionId
                );

    return efa;
}


#pragma warning( disable : 4509 )
LOCAL ERR ErrREPAIRICheckInternalLine(
    PIB * const ppib,
    const IFMP ifmp,
    CSR& csr,
    BTSTATS * const pbtstats,
    const REPAIROPTS * const popts,
    KEYDATAFLAGS& kdfCurr,
    const KEYDATAFLAGS& kdfPrev )
{
    ERR err = JET_errSuccess;

    TRY
    {
        NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdfCurr );
        CallJ( ErrREPAIRICheckNode(
                csr.Pgno(),
                csr.ILine(),
                (BYTE *)csr.Cpage().PvBuffer(),
                kdfCurr,
                popts ), HandleTryError );

        if ( FNDDeleted( kdfCurr ) )
        {
            (*popts->pcprintfError)( "node [%d:%d]: deleted node on internal page\r\n", csr.Pgno(), csr.ILine() );
            CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
        }

        if ( FNDVersion( kdfCurr ) )
        {
            (*popts->pcprintfError)( "node [%d:%d]: versioned node on internal page\r\n", csr.Pgno(), csr.ILine() );
            CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
        }

        if ( 1 == kdfCurr.key.prefix.Cb() )
        {
            (*popts->pcprintfError)( "node [%d:%d]: incorrectly compressed key\r\n", csr.Pgno(), csr.ILine() );
            CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
        }

        if ( sizeof( PGNO ) != kdfCurr.data.Cb()  )
        {
            (*popts->pcprintfError)( "node [%d:%d]: bad internal data size\r\n", csr.Pgno(), csr.ILine() );
            CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
        }

        if ( csr.ILine() > 0 && !kdfCurr.key.FNull() )
        {
            const INT cmp = CmpKey( kdfPrev.key, kdfCurr.key );
            if ( cmp > 0 )
            {
                (*popts->pcprintfError)( "node [%d:%d]: nodes out of order on internal page\r\n", csr.Pgno(), csr.ILine() );
                CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
            }
            else if ( 0 == cmp )
            {
                (*popts->pcprintfError)( "node [%d:%d]: illegal duplicate key on internal page\r\n", csr.Pgno(), csr.ILine() );
                CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
            }
        }

        if ( FNDCompressed( kdfCurr ) )
        {
            ++(pbtstats->cnodeCompressed);
        }
        pbtstats->cbDataInternal += kdfCurr.data.Cb();
        ++(pbtstats->rgckeyInternal[kdfCurr.key.Cb()]);
        ++(pbtstats->rgckeySuffixInternal[kdfCurr.key.suffix.Cb()]);

HandleTryError:
        ;
    }
    EXCEPT( LREPAIRHandleException( ppib, ifmp, csr, ExceptionInfo(), popts ) )
    {
        err = ErrERRCheck( JET_errDatabaseCorrupted );
    }
    ENDEXCEPT

    return err;
}
#pragma warning( default : 4509 )


LOCAL ERR ErrREPAIRCheckInternal(
    PIB * const ppib,
    const IFMP ifmp,
    CSR& csr,
    BTSTATS * const pbtstats,
    const BOOKMARK * const pbookmarkCurrParent,
    const BOOKMARK * const pbookmarkPrevParent,
    const REPAIROPTS * const popts )
{
    ERR             err     = JET_errSuccess;
    KEYDATAFLAGS    rgkdf[2];

    if ( csr.Cpage().Clines() <= 0 )
    {
        (*popts->pcprintfError)( "page %d: empty internal page\r\n", csr.Pgno() );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( pgnoNull != csr.Cpage().PgnoNext() )
    {
        (*popts->pcprintfError)( "page %d: pgno next is non-NULL (%d)\r\n", csr.Pgno(), csr.Cpage().PgnoNext() );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( pgnoNull != csr.Cpage().PgnoPrev() )
    {
        (*popts->pcprintfError)( "page %d: pgno next is non-NULL (%d)\r\n", csr.Pgno(), csr.Cpage().PgnoPrev() );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    LINE line;
    ++(pbtstats->cpageInternal);
    if ( csr.Cpage().FRootPage() && !csr.Cpage().FSpaceTree() )
    {
        NDGetPtrExternalHeader( csr.Cpage(), &line, noderfSpaceHeader );
        if ( sizeof( SPACE_HEADER ) != line.cb )
        {
            (*popts->pcprintfError)( "page %d: space header is wrong size (expected %d bytes, got %d bytes)\r\n",
                csr.Pgno(),
                sizeof( SPACE_HEADER ),
                line.cb
                );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }
    else
    {
        NDGetPtrExternalHeader( csr.Cpage(), &line, noderfWhole );
        ++(pbtstats->rgckeyPrefixInternal[line.cb]);
    }
    if ( pbtstats->cpageDepth <= 0 )
    {
        --(pbtstats->cpageDepth);
    }

    INT iline;
    for( iline = 0; iline < csr.Cpage().Clines(); iline++ )
    {
        csr.SetILine( iline );
        KEYDATAFLAGS& kdfCurr = rgkdf[ iline % 2 ];
        const KEYDATAFLAGS& kdfPrev = rgkdf[ ( iline + 1 ) % 2 ];

        Call( ErrREPAIRICheckInternalLine(
                    ppib,
                    ifmp,
                    csr,
                    pbtstats,
                    popts,
                    kdfCurr,
                    kdfPrev ) );
    }

    if ( pbookmarkCurrParent )
    {
        csr.SetILine( csr.Cpage().Clines() - 1 );

        KEYDATAFLAGS kdfLast;
        NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdfLast );

        if ( !FKeysEqual( kdfLast.key, pbookmarkCurrParent->key ) )
        {
            (*popts->pcprintfError)( "page %d: bad page pointer to internal page\r\n", csr.Pgno() );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }

    if ( pbookmarkPrevParent )
    {
        csr.SetILine( 0 );

        KEYDATAFLAGS kdfFirst;
        NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdfFirst );
        if ( kdfFirst.key.Cb() != 0 )
        {
            const INT cmp = CmpKey( kdfFirst.key, pbookmarkPrevParent->key );
            if ( cmp < 0 )
            {
                (*popts->pcprintfError)( "page %d: prev parent pointer > first node on internal page\r\n", csr.Pgno() );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
        }
    }

HandleError:
    return err;
}


#pragma warning( disable : 4509 )
LOCAL ERR ErrREPAIRICheckLeafLine(
    PIB * const ppib,
    const IFMP ifmp,
    CSR& csr,
    RECCHECK * const preccheck,
    const BOOL fNonUnique,
    BTSTATS * const pbtstats,
    const REPAIROPTS * const popts,
    KEYDATAFLAGS& kdfCurr,
    const KEYDATAFLAGS& kdfPrev,
    BOOL * const pfEmpty )
{
    ERR err = JET_errSuccess;

    TRY
    {
        NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdfCurr );
        CallJ( ErrREPAIRICheckNode(
                csr.Pgno(),
                csr.ILine(),
                (BYTE *)csr.Cpage().PvBuffer(),
                kdfCurr,
                popts ), HandleTryError );

        if ( kdfCurr.key.prefix.Cb() == 1 )
        {
            (*popts->pcprintfError)( "node [%d:%d]: incorrectly compressed key\r\n", csr.Pgno(), csr.ILine() );
            CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
        }

        if ( !FNDDeleted( kdfCurr ) )
        {
            *pfEmpty = fFalse;
            err = (*preccheck)( kdfCurr, csr.Pgno() );
            if ( err > 0 )
            {
                (*popts->pcprintfWarning)( "node [%d:%d]: leaf node check failed\r\n", csr.Pgno(), csr.ILine() );
            }
            else if ( err < 0 )
            {
                (*popts->pcprintfError)( "node [%d:%d]: leaf node check failed\r\n", csr.Pgno(), csr.ILine() );
                CallJ( err, HandleTryError );
            }
        }
        else
        {
            ++(pbtstats->cnodeDeleted);
        }

        if ( csr.ILine() > 0 )
        {
            Assert( !fNonUnique || csr.Cpage().FIndexPage() );

            const INT cmp = fNonUnique ?
                                CmpKeyData( kdfPrev, kdfCurr, NULL ) :
                                CmpKey( kdfPrev.key, kdfCurr.key );
            if ( cmp > 0 )
            {
                (*popts->pcprintfError)( "node [%d:%d]: nodes out of order on leaf page\r\n", csr.Pgno(), csr.ILine() );
                CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
            }
            else if ( 0 == cmp )
            {
                (*popts->pcprintfError)( "node [%d:%d]: illegal duplicate key on leaf page\r\n", csr.Pgno(), csr.ILine() );
                CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
            }
        }

        if ( FNDVersion( kdfCurr ) )
        {
            ++(pbtstats->cnodeVersioned);
        }

        if ( FNDCompressed( kdfCurr ) )
        {
            ++(pbtstats->cnodeCompressed);
        }

        pbtstats->cbDataLeaf += kdfCurr.data.Cb();
        ++(pbtstats->rgckeyLeaf[kdfCurr.key.Cb()]);
        ++(pbtstats->rgckeySuffixLeaf[kdfCurr.key.suffix.Cb()]);

HandleTryError:
        ;
    }
    EXCEPT( LREPAIRHandleException( ppib, ifmp, csr, ExceptionInfo(), popts ) )
    {
        err = ErrERRCheck( JET_errDatabaseCorrupted );
    }
    ENDEXCEPT

    return err;
}
#pragma warning( default : 4509 )


LOCAL ERR ErrREPAIRCheckLeaf(
    PIB * const ppib,
    const IFMP ifmp,
    CSR& csr,
    RECCHECK * const preccheck,
    const BOOL fNonUnique,
    BTSTATS * const pbtstats,
    const BOOKMARK * const pbookmarkCurrParent,
    const BOOKMARK * const pbookmarkPrevParent,
    const REPAIROPTS * const popts )
{
    ERR             err     = JET_errSuccess;
    BOOL            fEmpty  = fTrue;
    KEYDATAFLAGS    rgkdf[2];

    if ( csr.Cpage().Clines() == 0 && !csr.Cpage().FRootPage() )
    {
        (*popts->pcprintfError)( "page %d: empty leaf page (non-root)\r\n", csr.Pgno() );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( csr.Cpage().PgnoPrev() != pbtstats->pgnoLastSeen )
    {
        (*popts->pcprintfError)( "page %d: bad leaf page links\r\n", csr.Pgno() );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }
    if ( pgnoNull != pbtstats->pgnoNextExpected && csr.Pgno() != pbtstats->pgnoNextExpected )
    {
        (*popts->pcprintfError)( "page %d: bad leaf page links\r\n", csr.Pgno() );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    pbtstats->pgnoLastSeen      = csr.Pgno();
    pbtstats->pgnoNextExpected  = csr.Cpage().PgnoNext();

    LINE line;
    ++(pbtstats->cpageLeaf);
    if ( csr.Cpage().FRootPage() && !csr.Cpage().FSpaceTree() )
    {
        NDGetPtrExternalHeader( csr.Cpage(), &line, noderfSpaceHeader );
        if ( sizeof( SPACE_HEADER ) != line.cb )
        {
            (*popts->pcprintfError)( "page %d: space header is wrong size (expected %d bytes, got %d bytes)\r\n",
                csr.Pgno(),
                sizeof( SPACE_HEADER ),
                line.cb
                );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }
    else
    {
        NDGetPtrExternalHeader( csr.Cpage(), &line, noderfWhole );
        ++(pbtstats->rgckeyPrefixLeaf[line.cb]);
    }

    if ( pbtstats->cpageDepth <= 0 )
    {
        pbtstats->cpageDepth = QWORD( 0 - pbtstats->cpageDepth );
        ++(pbtstats->cpageDepth);
    }

    INT iline;
    for( iline = 0; iline < csr.Cpage().Clines(); iline++ )
    {
        csr.SetILine( iline );
        KEYDATAFLAGS& kdfCurr = rgkdf[ iline % 2 ];
        const KEYDATAFLAGS& kdfPrev = rgkdf[ ( iline + 1 ) % 2 ];

        Call( ErrREPAIRICheckLeafLine(
                    ppib,
                    ifmp,
                    csr,
                    preccheck,
                    fNonUnique,
                    pbtstats,
                    popts,
                    kdfCurr,
                    kdfPrev,
                    &fEmpty ) );
    }

    if ( pbookmarkCurrParent )
    {
        if ( pgnoNull == csr.Cpage().PgnoNext() )
        {
            (*popts->pcprintfError)( "page %d: non-NULL page pointer to leaf page with no pgnoNext\r\n", csr.Pgno() );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        csr.SetILine( csr.Cpage().Clines() - 1 );

        KEYDATAFLAGS kdfLast;
        NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdfLast );

        const INT cmp = CmpKey( kdfLast.key, pbookmarkCurrParent->key );
        if ( cmp >= 0 )
        {
            (*popts->pcprintfError)( "page %d: bad page pointer to leaf page\r\n", csr.Pgno() );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }
    else if ( pgnoNull != csr.Cpage().PgnoNext() )
    {
        (*popts->pcprintfError)( "page %d: NULL page pointer to leaf page with pgnoNext\r\n", csr.Pgno() );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( pbookmarkPrevParent )
    {
        csr.SetILine( 0 );

        KEYDATAFLAGS kdfFirst;
        NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdfFirst );

        if ( kdfFirst.key.Cb() != 0 )
        {
            const INT cmp = CmpKeyWithKeyData( pbookmarkPrevParent->key, kdfFirst );
            if ( cmp > 0 )
            {
                (*popts->pcprintfError)( "page %d: prev parent pointer > first node on leaf page\r\n", csr.Pgno() );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
        }
    }

    if ( fEmpty )
    {
        ++(pbtstats->cpageEmpty);
    }

HandleError:
    return err;
}


OBJIDLIST::OBJIDLIST() :
    m_cobjid( 0 ),
    m_cobjidMax( 0 ),
    m_rgobjid( NULL ),
    m_fSorted( fFalse )
{
}


OBJIDLIST::~OBJIDLIST()
{
    if ( NULL != m_rgobjid )
    {
        Assert( 0 < m_cobjidMax );
        OSMemoryHeapFree( m_rgobjid );
        m_rgobjid = reinterpret_cast<OBJID *>( lBitsAllFlipped );
    }
    else
    {
        Assert( 0 == m_cobjidMax );
        Assert( 0 == m_cobjid );
    }
}


ERR OBJIDLIST::ErrAddObjid( const OBJID objid )
{
    if ( m_cobjid == m_cobjidMax )
    {

        OBJID * const rgobjidOld = m_rgobjid;
        const INT cobjidMaxNew   = m_cobjidMax + 16;
        OBJID * const rgobjidNew = reinterpret_cast<OBJID *>( PvOSMemoryHeapAlloc( cobjidMaxNew * sizeof( OBJID ) ) );
        if ( NULL == rgobjidNew )
        {
            return ErrERRCheck( JET_errOutOfMemory );
        }

        UtilMemCpy( rgobjidNew, m_rgobjid, sizeof( OBJID ) * m_cobjid );
        m_cobjidMax = cobjidMaxNew;
        m_rgobjid   = rgobjidNew;
        OSMemoryHeapFree( rgobjidOld );
    }
    m_rgobjid[m_cobjid++] = objid;
    m_fSorted = fFalse;
    return JET_errSuccess;
}


BOOL OBJIDLIST::FObjidPresent( const OBJID objid ) const
{
    Assert( m_fSorted );
    return binary_search( (OBJID *)m_rgobjid, (OBJID *)m_rgobjid + m_cobjid, objid );
}


VOID OBJIDLIST::Sort()
{
    sort( m_rgobjid, m_rgobjid + m_cobjid );
    m_fSorted = fTrue;
}


LOCAL ERR ErrREPAIRAttachForIntegrity(
    const JET_SESID sesid,
    const WCHAR * const wszDatabase,
    IFMP * const pifmp,
    const REPAIROPTS * const popts )
{
    ERR                 err             = JET_errSuccess;
    JET_DBID            dbid;
    const BOOL          fAttachReadonly = popts->grbit & JET_bitDBUtilOptionDontRepair;
    const JET_GRBIT     grbit           = ( fAttachReadonly ? JET_bitDbReadOnly : 0 ) | JET_bitDbRecoveryOff;

    Call( ErrIsamAttachDatabase(
        sesid,
        wszDatabase,
        fFalse,
        NULL,
        0,
        grbit ) );
    Assert( JET_wrnDatabaseAttached != err );

    Call( ErrIsamOpenDatabase(
        sesid,
        wszDatabase,
        NULL,
        &dbid,
        ( fAttachReadonly ? JET_bitDbReadOnly : 0 ) | JET_bitDbRecoveryOff ) );
    *pifmp = dbid;

    g_rgfmp[*pifmp].SetVersioningOff();

    g_rgfmp[*pifmp].SetFDontRegisterOLD2Tasks();

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRAttachForRepair(
    const JET_SESID sesid,
    const WCHAR * const wszDatabase,
    IFMP * const pifmp,
    const DBTIME dbtimeLast,
    const OBJID objidLast,
    const REPAIROPTS * const popts )
{
    ERR                 err         = JET_errSuccess;
    JET_DBID            dbid;
    const JET_GRBIT     grbit       = JET_bitDbRecoveryOff;

    CallR( ErrIsamCloseDatabase( sesid, (JET_DBID)*pifmp, 0 ) );
    CallR( ErrIsamDetachDatabase( sesid, NULL, wszDatabase ) );
    CallR( ErrREPAIRChangeSignature( PinstFromPpib( (PIB *)sesid ), wszDatabase, dbtimeLast, objidLast, popts ) );
    CallR( ErrIsamAttachDatabase( sesid, wszDatabase, fFalse, NULL, 0, grbit ) );
    Assert( JET_wrnDatabaseAttached != err );
    CallR( ErrIsamOpenDatabase(
            sesid,
            wszDatabase,
            NULL,
            &dbid,
            JET_bitDbRecoveryOff ) );
    *pifmp = dbid;
    g_rgfmp[*pifmp].SetVersioningOff();

    g_rgfmp[*pifmp].SetFDontRegisterOLD2Tasks();

    g_rgfmp[*pifmp].ResetFMaintainMSObjids();
    return JET_errSuccess;
}


LOCAL ERR ErrREPAIRChangeSignature(
    INST *pinst,
    const WCHAR * const wszDatabase,
    const DBTIME dbtimeLast,
    const OBJID objidLast,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    SIGNATURE signDb;

    Call( ErrREPAIRChangeDBSignature(
            pinst,
            wszDatabase,
            dbtimeLast,
            objidLast,
            &signDb,
            popts ) );

HandleError:
    return err;
}

LOCAL ERR ErrREPAIRChangeDBSignature(
    INST *pinst,
    const WCHAR * const wszDatabase,
    const DBTIME dbtimeLast,
    const OBJID objidLast,
    SIGNATURE * const psignDb,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;
    CFlushMapForUnattachedDb* pfm = NULL;
    DBFILEHDR * const pdbfilehdr = reinterpret_cast<DBFILEHDR * >( PvOSMemoryPageAlloc( g_cbPage, NULL ) );
    if ( NULL == pdbfilehdr )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    err = ErrUtilReadShadowedHeader(    pinst,
                                        pinst->m_pfsapi,
                                        wszDatabase,
                                        reinterpret_cast<BYTE*>( pdbfilehdr ),
                                        g_cbPage,
                                        OffsetOf( DBFILEHDR, le_cbPageSize ),
                                        ( popts->grbit & JET_bitDBUtilOptionDontRepair ) ? urhfReadOnly : urhfNone );
    if ( err < JET_errSuccess )
    {
        if ( FErrIsDbHeaderCorruption( err ) )
        {
            (*popts->pcprintfError)( "unable to read database header for %ws\r\n", wszDatabase );
            err = ErrERRCheck( JET_errDatabaseCorrupted );
        }
        goto HandleError;
    }

    if ( !( popts->grbit & JET_bitDBUtilOptionDontRepair ) )
    {
        Assert( 0 != g_dwGlobalMajorVersion );

        (*popts->pcprintfVerbose)( "changing signature of %ws\r\n", wszDatabase );

        pdbfilehdr->le_ulMagic            = ulDAEMagic;

        if ( 0 != dbtimeLast )
        {


            pdbfilehdr->le_dbtimeDirtied      = dbtimeLast + 1;
        }
        pdbfilehdr->le_objidLast          = objidLast + 1;
        pdbfilehdr->le_attrib             = attribDb;
        pdbfilehdr->le_dwMajorVersion     = g_dwGlobalMajorVersion;
        pdbfilehdr->le_dwMinorVersion     = g_dwGlobalMinorVersion;
        pdbfilehdr->le_dwBuildNumber      = g_dwGlobalBuildNumber;
        pdbfilehdr->le_lSPNumber          = g_lGlobalSPNumber;
        QWORD qwSortVersion;
        CallS( ErrNORMGetSortVersion( wszLocaleNameDefault, &qwSortVersion, NULL ) );
        pdbfilehdr->le_qwSortVersion            = qwSortVersion;
        pdbfilehdr->le_lGenMinRequired          = 0;
        pdbfilehdr->le_lGenMaxRequired          = 0;
        pdbfilehdr->le_lGenMaxCommitted         = 0;
        pdbfilehdr->le_lGenMinConsistent        = 0;
        pdbfilehdr->le_lGenRecovering           = 0;
        pdbfilehdr->le_lGenPreRedoMinRequired   = 0;
        pdbfilehdr->le_lGenPreRedoMinConsistent = 0;
        if ( objidNil != objidLast )
        {
            ++(pdbfilehdr->le_ulRepairCount);

            pdbfilehdr->le_ulECCFixSuccessOld = pdbfilehdr->le_ulECCFixSuccess;
            pdbfilehdr->le_ulECCFixFailOld = pdbfilehdr->le_ulECCFixFail;
            pdbfilehdr->le_ulBadChecksumOld = pdbfilehdr->le_ulBadChecksum;
        }
        LGIGetDateTime( &pdbfilehdr->logtimeRepair );

        pdbfilehdr->ResetUpgradeDb();

        memset( &pdbfilehdr->signLog, 0, sizeof( SIGNATURE ) );

        memset( &pdbfilehdr->le_lgposConsistent, 0, sizeof( LGPOS ) );
        memset( &pdbfilehdr->logtimeConsistent, 0, sizeof( LOGTIME ) );

        memset( &pdbfilehdr->logtimeAttach, 0, sizeof( LOGTIME ) );
        memset( &pdbfilehdr->le_lgposAttach, 0, sizeof( LGPOS ) );
        memset( &pdbfilehdr->le_lgposLastResize, 0, sizeof( LGPOS ) );

        memset( &pdbfilehdr->logtimeDetach, 0, sizeof( LOGTIME ) );
        memset( &pdbfilehdr->le_lgposDetach, 0, sizeof( LGPOS ) );

        memset( &pdbfilehdr->bkinfoFullPrev, 0, sizeof( BKINFO ) );
        memset( &pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );
        memset( &pdbfilehdr->bkinfoCopyPrev, 0, sizeof( BKINFO ) );
        memset( &pdbfilehdr->bkinfoDiffPrev, 0, sizeof( BKINFO ) );
        memset( &pdbfilehdr->bkinfoFullCur, 0, sizeof( BKINFO ) );

        pdbfilehdr->bkinfoTypeFullPrev = DBFILEHDR::backupNormal;
        pdbfilehdr->bkinfoTypeIncPrev = DBFILEHDR::backupNormal;
        pdbfilehdr->bkinfoTypeCopyPrev = DBFILEHDR::backupNormal;
        pdbfilehdr->bkinfoTypeDiffPrev = DBFILEHDR::backupNormal;

        SIGGetSignature( &(pdbfilehdr->signDb) );

        *psignDb    = pdbfilehdr->signDb;

        (*popts->pcprintfVerbose)( "new DB signature is:\r\n" );
        REPAIRPrintSig( &pdbfilehdr->signDb, popts->pcprintfVerbose );

        Call( CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDatabase, pdbfilehdr, pinst, &pfm ) );

        Call( ErrUtilWriteUnattachedDatabaseHeaders( pinst, pinst->m_pfsapi, wszDatabase, pdbfilehdr, NULL, pfm ) );

        if ( pfm != NULL )
        {
            if ( pdbfilehdr->Dbstate() == JET_dbstateCleanShutdown )
            {
                Call( pfm->ErrCleanFlushMap() );
            }

            pfm->TermFlushMap();
        }
    }

HandleError:
    delete pfm;
    OSMemoryPageFree( pdbfilehdr );
    return err;
}


LOCAL ERR ErrREPAIRRepairGlobalSpace(
    PIB * const ppib,
    const IFMP ifmp,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    const PGNO pgnoLast = PgnoLast( ifmp );
    const CPG  cpgOwned = PgnoLast( ifmp ) - 3;

    FUCB    *pfucb      = pfucbNil;
    FUCB    *pfucbOE    = pfucbNil;

    (*popts->pcprintfVerbose)( "repairing database root\r\n" );

    OBJID           objidFDP;
    Call( ErrSPCreate(
                ppib,
                ifmp,
                pgnoNull,
                pgnoSystemRoot,
                3,
                fSPMultipleExtent,
                (ULONG)CPAGE::fPagePrimary,
                &objidFDP ) );
    Assert( objidSystemRoot == objidFDP );

    Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucb ) );

    Call( ErrSPIOpenOwnExt( ppib, pfucb->u.pfcb, &pfucbOE ) );

    (*popts->pcprintfDebug)( "Global OwnExt:  %d pages ending at %d\r\n", cpgOwned, pgnoLast );
    Call( ErrREPAIRInsertRunIntoSpaceTree(
                    ppib,
                    ifmp,
                    pfucbOE,
                    pgnoLast,
                    cpgOwned,
                    popts ) );

HandleError:
    if ( pfucbNil != pfucb )
    {
        DIRClose( pfucb );
    }
    if ( pfucbNil != pfucbOE )
    {
        DIRClose( pfucbOE );
    }
    return err;
}


LOCAL ERR ErrREPAIRBuildCatalogEntryToDeleteList(
    INFOLIST **ppDeleteList,
    const ENTRYINFO entryinfo )
{

    ERR             err         = JET_errSuccess;
    INFOLIST    *   pTemp       = *ppDeleteList;
    INFOLIST    *   pInfo;

    Alloc( pInfo = new INFOLIST );

    memset( pInfo, 0, sizeof(INFOLIST) );
    pInfo->info = entryinfo;
    pInfo->pInfoListNext = NULL;

    if ( NULL == pTemp
        || pTemp->info.objidTable > entryinfo.objidTable
        || ( pTemp->info.objidTable == entryinfo.objidTable
             && pTemp->info.objType > entryinfo.objType )
        || ( pTemp->info.objidTable == entryinfo.objidTable
             && pTemp->info.objType == entryinfo.objType
             && pTemp->info.objidFDP > entryinfo.objidFDP ) )
    {
        pInfo->pInfoListNext = pTemp;
        *ppDeleteList = pInfo;
    }
    else
    {
        while ( NULL != pTemp->pInfoListNext
               && ( pTemp->pInfoListNext->info.objidTable < entryinfo.objidTable
                    || ( pTemp->pInfoListNext->info.objidTable == entryinfo.objidTable
                         && pTemp->pInfoListNext->info.objType < entryinfo.objType )
                    || ( pTemp->pInfoListNext->info.objidTable == entryinfo.objidTable
                         && pTemp->pInfoListNext->info.objType == entryinfo.objType
                         && pTemp->pInfoListNext->info.objidFDP < entryinfo.objidFDP ) ) )
        {
            pTemp = pTemp->pInfoListNext;
        }
        pInfo->pInfoListNext = pTemp->pInfoListNext;
        pTemp->pInfoListNext = pInfo;
    }

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRDeleteCorruptedEntriesFromCatalog(
    PIB * const ppib,
    const IFMP ifmp,
    const INFOLIST *pTablesToDelete,
    const INFOLIST *pEntriesToDelete,
    const REPAIROPTS * const popts )
{
    ERR             err             = JET_errSuccess;
    FUCB        *   pfucbCatalog    = pfucbNil;
    ENTRYINFO       entryinfo;

    BOOL            fEntryToDelete  = fFalse;

    DIB dib;
    dib.pos     = posFirst;
    dib.pbm     = NULL;
    dib.dirflag = fDIRNull;

    CallR( ErrDIRBeginTransaction( ppib, 64549, NO_GRBIT ) );

    Call( ErrDIROpen( ppib, pgnoFDPMSO, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    FUCBSetIndex( pfucbCatalog );
    FUCBSetSequential( pfucbCatalog );

    err = ErrDIRDown( pfucbCatalog, &dib );

    while ( JET_errNoCurrentRecord != err
            && ( pTablesToDelete || pEntriesToDelete ) )
    {
        Call( err );

        Call( ErrDIRRelease( pfucbCatalog ) );

        memset( &entryinfo, 0, sizeof( entryinfo ) );
        Call( ErrREPAIRRetrieveCatalogColumns( ppib, ifmp, pfucbCatalog, &entryinfo, popts ) );

        while ( pTablesToDelete && pTablesToDelete->info.objidTable < entryinfo.objidTable )
        {
            pTablesToDelete = pTablesToDelete->pInfoListNext;
        }
        while ( pEntriesToDelete && ( pEntriesToDelete->info.objidTable < entryinfo.objidTable
                                    || ( pEntriesToDelete->info.objidTable == entryinfo.objidTable
                                         && pEntriesToDelete->info.objType < entryinfo.objType )
                                    || ( pEntriesToDelete->info.objidTable == entryinfo.objidTable
                                        && pEntriesToDelete->info.objType == entryinfo.objType
                                        && pEntriesToDelete->info.objidFDP < entryinfo.objidFDP ) ) )
        {
            (*popts->pcprintfVerbose)( "skipping (%d %d %s) (%d %d %s)\r\n",
                                        pEntriesToDelete->info.objidTable, pEntriesToDelete->info.objidFDP, pEntriesToDelete->info.szName,
                                        entryinfo.objidTable, entryinfo.objidFDP, entryinfo.szName );
            pEntriesToDelete = pEntriesToDelete->pInfoListNext;
        }

        if ( pTablesToDelete && pTablesToDelete->info.objidTable == entryinfo.objidTable )
        {
            fEntryToDelete = fTrue;
        }
        else if ( pEntriesToDelete
                 && pEntriesToDelete->info.objidTable == entryinfo.objidTable
                 && pEntriesToDelete->info.objidFDP == entryinfo.objidFDP
                 && pEntriesToDelete->info.objType == entryinfo.objType )
            {
                fEntryToDelete = fTrue;

            (*popts->pcprintfVerbose)( "deleting (%d %d %s) (%d %d %s)\r\n",
                                        pEntriesToDelete->info.objidTable, pEntriesToDelete->info.objidFDP, pEntriesToDelete->info.szName,
                                        entryinfo.objidTable, entryinfo.objidFDP, entryinfo.szName );

                pEntriesToDelete = pEntriesToDelete->pInfoListNext;
            }
        else
        {
        }

        if ( fEntryToDelete )
        {
            (*popts->pcprintfVerbose)( "Deleting a catalog entry (%d %d %s)\r\n",
                                        entryinfo.objidTable, entryinfo.objidFDP, entryinfo.szName );

            Call( ErrDIRDelete( pfucbCatalog, fDIRNoVersion ) );

            fEntryToDelete = fFalse;
        }

        err = ErrDIRNext( pfucbCatalog, fDIRNull );
    }

    Assert( NULL == pTablesToDelete || JET_errNoCurrentRecord == err );
    Assert( NULL == pEntriesToDelete || JET_errNoCurrentRecord == err );

    if ( JET_errNoCurrentRecord == err
        || JET_errRecordNotFound == err )
    {
        err = JET_errSuccess;
    }

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:

    if ( pfucbNil != pfucbCatalog )
    {
        DIRClose( pfucbCatalog );
    }

    if ( JET_errSuccess != err )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}


LOCAL ERR ErrREPAIRInsertMSOEntriesToCatalog(
    PIB * const ppib,
    const IFMP ifmp,
    const REPAIROPTS * const popts )
{
    ERR         err     = JET_errSuccess;
    CallR( ErrDIRBeginTransaction( ppib, 39973, NO_GRBIT ) );
    Call( ErrREPAIRCATCreate(
                    ppib,
                    ifmp,
                    objidFDPMSO_NameIndex,
                    objidFDPMSO_RootObjectIndex,
                    fTrue,
                    fFalse ) );
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
HandleError:
    if ( JET_errSuccess != err )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}


LOCAL ERR ErrREPAIRRepairCatalogs(
    PIB * const ppib,
    const IFMP ifmp,
    OBJID * const pobjidLast,
    const BOOL fCatalogCorrupt,
    const BOOL fShadowCatalogCorrupt,
    const REPAIROPTS * const popts )
{
    ERR     err                     = JET_errSuccess;

    FUCB    * pfucbParent           = pfucbNil;
    FUCB    * pfucbCatalog          = pfucbNil;
    FUCB    * pfucbShadowCatalog    = pfucbNil;
    FUCB    * pfucbSpace            = pfucbNil;

    QWORD   qwRecords               = 0;

    CallR( ErrDIRBeginTransaction( ppib, 56357, NO_GRBIT ) );

    if ( fCatalogCorrupt || fShadowCatalogCorrupt )
    {
        Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucbParent ) );
    }

    if ( fCatalogCorrupt && fShadowCatalogCorrupt )
    {
        Call( ErrREPAIRScanDBAndRepairCatalogs( ppib, ifmp, popts ) );

        err = ErrREPAIRCheckFixCatalogLogical(
                    ppib,
                    ifmp,
                    pobjidLast,
                    fFalse,
                    fTrue,
                    &qwRecords,
                    NULL,
                    NULL,
                    NULL,
                    popts );
        if ( JET_errDatabaseCorrupted == err )
        {
            (*popts->pcprintfVerbose)( "Repaired the logical inconsistency of the catalog\r\n" );
        }
    }

    if ( fShadowCatalogCorrupt )
    {
        const PGNO cpgOwned = CpgCATShadowInitial( g_cbPage );
        const PGNO pgnoLast = pgnoFDPMSOShadow - 1 + cpgOwned;
        Expected( pgnoFDPMSO + cpgMSOInitial - 1  == pgnoLast - cpgOwned);
        Assert( pgnoLast <= 32 );

        (*popts->pcprintf)( "\r\nRebuilding %s from %s.\r\n", szMSOShadow, szMSO );
        popts->psnprog->cunitTotal  = PgnoLast( ifmp );
        popts->psnprog->cunitDone   = 0;
        (VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntBegin, NULL );

        (*popts->pcprintfVerbose)( "rebuilding %s from %s\r\n", szMSOShadow, szMSO );

        Assert( pfucbNil != pfucbParent );
        Call( ErrSPCreateMultiple(
            pfucbParent,
            pgnoSystemRoot,
            pgnoFDPMSOShadow,
            objidFDPMSOShadow,
            pgnoFDPMSOShadow+1,
            pgnoFDPMSOShadow+2,
            pgnoLast,
            cpgOwned,
            fTrue,
            CPAGE::fPagePrimary ) );

        DIRClose( pfucbParent );
        pfucbParent = pfucbNil;

        Call( ErrFILEOpenTable( ppib, ifmp, &pfucbCatalog, szMSO ) );
        Call( ErrFILEOpenTable( ppib, ifmp, &pfucbShadowCatalog, szMSOShadow ) );
        Call( ErrBTCopyTree( pfucbCatalog, pfucbShadowCatalog, fDIRNoLog | fDIRNoVersion ) );

        DIRClose( pfucbCatalog );
        pfucbCatalog = pfucbNil;
        DIRClose( pfucbShadowCatalog );
        pfucbShadowCatalog = pfucbNil;
    }
    else if ( fCatalogCorrupt )
    {
        const PGNO pgnoLast = 23;
        const PGNO cpgOwned = 23 - 3 - 3;
        Assert( cpgMSOInitial >= cpgOwned );

        (*popts->pcprintf)( "\r\nRebuilding %s from %s.\r\n", szMSO, szMSOShadow );
        popts->psnprog->cunitTotal  = PgnoLast( ifmp );
        popts->psnprog->cunitDone   = 0;
        (VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntBegin, NULL );

        (*popts->pcprintfVerbose)( "rebuilding %s from %s\r\n", szMSO, szMSOShadow );

        Assert( pfucbNil != pfucbParent );
        Call( ErrSPCreateMultiple(
            pfucbParent,
            pgnoSystemRoot,
            pgnoFDPMSO,
            objidFDPMSO,
            pgnoFDPMSO+1,
            pgnoFDPMSO+2,
            pgnoFDPMSO+2,
            3,
            fTrue,
            CPAGE::fPagePrimary ) );

        DIRClose( pfucbParent );
        pfucbParent = pfucbNil;

        Call( ErrFILEOpenTable( ppib, ifmp, &pfucbCatalog, szMSO ) );

        if ( !pfucbCatalog->u.pfcb->FSpaceInitialized() )
        {
            pfucbCatalog->u.pfcb->SetPgnoOE( pgnoFDPMSO+1 );
            pfucbCatalog->u.pfcb->SetPgnoAE( pgnoFDPMSO+2 );
            pfucbCatalog->u.pfcb->Lock();
            pfucbCatalog->u.pfcb->SetSpaceInitialized();
            pfucbCatalog->u.pfcb->Unlock();
        }
        Call( ErrSPIOpenOwnExt( ppib, pfucbCatalog->u.pfcb, &pfucbSpace ) );
#ifdef REPAIR_DEBUG_VERBOSE_SPACE
        (*popts->pcprintfDebug)( "%s OwnExt: %d pages ending at %d\r\n", szMSO, cpgOwned, pgnoLast );
#endif

        Call( ErrREPAIRInsertRunIntoSpaceTree(
                    ppib,
                    ifmp,
                    pfucbSpace,
                    pgnoLast,
                    cpgOwned,
                    popts ) );

        Call( ErrFILEOpenTable( ppib, ifmp, &pfucbShadowCatalog, szMSOShadow ) );
        Call( ErrBTCopyTree( pfucbShadowCatalog, pfucbCatalog, fDIRNoLog | fDIRNoVersion ) );

        DIRClose( pfucbSpace );
        pfucbSpace = pfucbNil;
        DIRClose( pfucbCatalog );
        pfucbCatalog = pfucbNil;
        DIRClose( pfucbShadowCatalog );
        pfucbShadowCatalog = pfucbNil;
    }

    if ( fCatalogCorrupt || !fShadowCatalogCorrupt )
    {
        (*popts->pcprintfVerbose)( "rebuilding indexes for %s\r\n", szMSO );

        REPAIRTABLE repairtable;
        memset( &repairtable, 0, sizeof( REPAIRTABLE ) );
        repairtable.objidFDP = objidFDPMSO;
        repairtable.objidLV  = objidNil;
        repairtable.pgnoFDP  = pgnoFDPMSO;
        repairtable.pgnoLV   = pgnoNull;
        repairtable.fHasPrimaryIndex = fTrue;
        OSStrCbCopyA( repairtable.szTableName, sizeof(repairtable.szTableName), szMSO );

        Call( ErrFILEOpenTable( ppib, ifmp, &pfucbCatalog, szMSO ) );
        FUCBSetSystemTable( pfucbCatalog );
        Call( ErrREPAIRBuildAllIndexes( ppib, ifmp, &pfucbCatalog, &repairtable, popts ) );
    }

HandleError:
    if ( pfucbNil != pfucbParent )
    {
        DIRClose( pfucbParent );
    }
    if ( pfucbNil != pfucbCatalog )
    {
        DIRClose( pfucbCatalog );
    }
    if ( pfucbNil != pfucbShadowCatalog )
    {
        DIRClose( pfucbShadowCatalog );
    }
    if ( pfucbNil != pfucbSpace )
    {
        DIRClose( pfucbSpace );
    }

    CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    return err;
}


#pragma prefast(push)
#pragma prefast(disable:6262, "This function uses a lot of stack (33k)")

LOCAL ERR ErrREPAIRScanDBAndRepairCatalogs(
    PIB * const ppib,
    const IFMP ifmp,
    const REPAIROPTS * const popts )
{
    const JET_SESID sesid   = reinterpret_cast<JET_SESID>( ppib );
    const CPG cpgPreread    = 256;
    const PGNO  pgnoFirst   = 1;
    const PGNO  pgnoLast    = PgnoLast( ifmp );

    ERR err = JET_errSuccess;

    CPG cpgRemaining;

    PGNO    pgno            = pgnoFirst;

    INT     cRecords            = 0;
    INT     cRecordsDuplicate   = 0;

    JET_COLUMNDEF   rgcolumndef[2] = {
        { sizeof( JET_COLUMNDEF ), 0, JET_coltypLongBinary, 0, 0, 0, 0, JET_cbKeyMost_OLD, JET_bitColumnTTKey },
        { sizeof( JET_COLUMNDEF ), 0, JET_coltypLongBinary, 0, 0, 0, 0, 0, 0 },
    };

    JET_TABLEID tableid;
    JET_COLUMNID rgcolumnid[2];
    const JET_COLUMNID& columnidKey  = rgcolumnid[0];
    const JET_COLUMNID& columnidData = rgcolumnid[1];

    Call( ErrIsamOpenTempTable(
        reinterpret_cast<JET_SESID>( ppib ),
        rgcolumndef,
        sizeof( rgcolumndef ) / sizeof( rgcolumndef[0] ),
        0,
        JET_bitTTIndexed | JET_bitTTUnique | JET_bitTTScrollable | JET_bitTTUpdatable,
        &tableid,
        rgcolumnid,
        JET_cbKeyMost_OLD,
        JET_cbKeyMost_OLD ) );

    (*popts->pcprintf)( "\r\nScanning the database catalog.\r\n" );
    (*popts->pcprintfVerbose)( "scanning the database for catalog records from page %d to page %d\r\n", pgnoFirst, pgnoLast );

    popts->psnprog->cunitTotal = pgnoLast;
    popts->psnprog->cunitDone = 0;
    (VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntBegin, NULL );

    BFPrereadPageRange( ifmp, pgnoFirst, min( cpgPreread * 2, pgnoLast - pgnoFirst + 1 ), bfprfDefault, ppib->BfpriPriority( ifmp ), *TcRepair() );
    cpgRemaining = cpgPreread;

    while ( pgnoLast != pgno )
    {
        if ( 0 == --cpgRemaining )
        {
            popts->psnprog->cunitDone = pgno;
            (VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntProgress, popts->psnprog );
            if ( ( pgno + ( cpgPreread * 2 ) ) < pgnoLast )
            {
                BFPrereadPageRange( ifmp, pgno + cpgPreread, cpgPreread, bfprfDefault, ppib->BfpriPriority( ifmp ), *TcRepair() );
            }
            cpgRemaining = cpgPreread;
        }

        CSR csr;
        err = csr.ErrGetReadPage(
                    ppib,
                    ifmp,
                    pgno,
                    bflfNoTouch );

        if ( JET_errPageNotInitialized == err )
        {
            err = JET_errSuccess;
        }
        else if ( FErrIsDbCorruption( err ) || JET_errDiskIO == err )
        {
            err = JET_errSuccess;
        }
        else if ( err >= 0 )
        {
            if ( (  csr.Cpage().ObjidFDP() == objidFDPMSO
                    || csr.Cpage().ObjidFDP() == objidFDPMSOShadow )
                && csr.Cpage().FLeafPage()
                && !csr.Cpage().FSpaceTree()
                && !csr.Cpage().FEmptyPage()
                && !csr.Cpage().FRepairedPage()
                && csr.Cpage().Clines() > 0
                && csr.Cpage().FPrimaryPage() )
            {
                Assert( !csr.Cpage().FPreInitPage() );
                
                err = ErrREPAIRIFixLeafPage(
                            ppib,
                            ifmp,
                            csr,
                    #ifdef SYNC_DEADLOCK_DETECTION
                            NULL,
                    #endif
                            popts );
                if ( err < 0 )
                {
                    (*popts->pcprintfError)( "page %d: err %d. discarding page\r\n", pgno, err );

                    UtilReportEvent(
                            eventWarning,
                            REPAIR_CATEGORY,
                            REPAIR_BAD_PAGE_ID,
                            0, NULL );


                    err = JET_errSuccess;
                }
                else if ( 0 == csr.Cpage().Clines() )
                {
                    (*popts->pcprintfError)( "page %d: all records were bad. discarding page\r\n", pgno );

                    UtilReportEvent(
                            eventWarning,
                            REPAIR_CATEGORY,
                            REPAIR_BAD_PAGE_ID,
                            0, NULL );


                    err = JET_errSuccess;
                    goto HandleError;
                }
                else
                {
                    INT iline;
                    for( iline = 0;
                        iline < csr.Cpage().Clines() && err >= 0;
                        ++iline )
                    {
                        KEYDATAFLAGS kdf;
                        NDIGetKeydataflags( csr.Cpage(), iline, &kdf );
                        if ( !FNDDeleted( kdf ) )
                        {
                            ++cRecords;


                            BYTE rgb[g_cbPageMax];
                            BYTE * pb = rgb;

                            const INT cbPrefix = kdf.key.prefix.Cb();
                            const INT cbSuffix = kdf.key.suffix.Cb();
                            const INT cbData = kdf.data.Cb();

                            if (g_cbPageMax < (cbPrefix + cbSuffix + cbData))
                            {
                                Call ( JET_errDatabaseCorrupted );
                            }

                            memcpy( pb, kdf.key.prefix.Pv(), cbPrefix );
                            kdf.key.prefix.SetPv( pb );
                            pb += cbPrefix;

                            memcpy( pb, kdf.key.suffix.Pv(), cbSuffix );
                            kdf.key.suffix.SetPv( pb );
                            pb += cbSuffix;

                            memcpy( pb, kdf.data.Pv(), cbData );
                            kdf.data.SetPv( pb );
                            pb += cbData;

                            csr.ReleasePage( fFalse );

                            err = ErrREPAIRInsertCatalogRecordIntoTempTable(
                                    ppib,
                                    ifmp,
                                    kdf,
                                    tableid,
                                    columnidKey,
                                    columnidData,
                                    popts );

                            if ( JET_errKeyDuplicate == err )
                            {
                                ++cRecordsDuplicate;
                                err = JET_errSuccess;
                            }

                            Call( csr.ErrGetReadPage(
                                        ppib,
                                        ifmp,
                                        pgno,
                                        bflfNoTouch ) );

                        }
                    }
                }
            }
        }

        csr.ReleasePage( fTrue );
        csr.Reset();
        Call( err );
        ++pgno;
    }

    (VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntComplete, NULL );
    (*popts->pcprintfVerbose)( "%d catalog records found. %d unique\r\n", cRecords, cRecords - cRecordsDuplicate );

    (*popts->pcprintf)( "\r\nRebuilding %s.\r\n", szMSO );

    popts->psnprog->cunitTotal = cRecords - cRecordsDuplicate;
    popts->psnprog->cunitDone = 0;
    (VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntBegin, NULL );

    Call( ErrREPAIRCopyTempTableToCatalog(
                ppib,
                ifmp,
                tableid,
                columnidKey,
                columnidData,
                popts ) );

    (VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntComplete, NULL );

HandleError:
    return err;
}
#pragma prefast(pop)

LOCAL ERR ErrREPAIRInsertCatalogRecordIntoTempTable(
    PIB * const ppib,
    const IFMP ifmp,
    const KEYDATAFLAGS& kdf,
    const JET_TABLEID tableid,
    const JET_COLUMNID columnidKey,
    const JET_COLUMNID columnidData,
    const REPAIROPTS * const popts )
{
    const JET_SESID sesid = (JET_SESID)ppib;

    JET_ERR err = JET_errSuccess;

    Call( ErrDispPrepareUpdate(
                sesid,
                tableid,
                JET_prepInsert ) );

    BYTE rgbKey[JET_cbKeyMost_OLD];
    kdf.key.CopyIntoBuffer( rgbKey, sizeof( rgbKey ) );

    Call( ErrDispSetColumn(
                sesid,
                tableid,
                columnidKey,
                rgbKey,
                kdf.key.Cb(),
                0,
                NULL ) );

    Call( ErrDispSetColumn(
                sesid,
                tableid,
                columnidData,
                kdf.data.Pv(),
                kdf.data.Cb(),
                0,
                NULL ) );

    err = ErrDispUpdate( sesid, tableid, NULL, 0, NULL, 0 );
    if ( err < 0 )
    {
        CallS( ErrDispPrepareUpdate( sesid, tableid, JET_prepCancel ) );
    }

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRCopyTempTableToCatalog(
    PIB * const ppib,
    const IFMP ifmp,
    const JET_TABLEID tableid,
    const JET_COLUMNID columnidKey,
    const JET_COLUMNID columnidData,
    const REPAIROPTS * const popts )
{
    const PGNO pgnoLast = 23;
    const PGNO cpgOwned = 23 - 3 - 3;
    Assert( cpgMSOInitial >= cpgOwned );
    const JET_SESID sesid = reinterpret_cast<JET_SESID>( ppib );

    JET_ERR err             = JET_errSuccess;

    FUCB    * pfucbParent   = pfucbNil;
    FUCB    * pfucbCatalog  = pfucbNil;
    FUCB    * pfucbSpace    = pfucbNil;


    VOID * pvKey = NULL;
    BFAlloc( bfasIndeterminate, &pvKey );
    VOID * pvData = NULL;
    BFAlloc( bfasIndeterminate, &pvData );

    Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucbParent ) );
    Assert( pfucbNil != pfucbParent );

    Call( ErrSPCreateMultiple(
        pfucbParent,
        pgnoSystemRoot,
        pgnoFDPMSO,
        objidFDPMSO,
        pgnoFDPMSO+1,
        pgnoFDPMSO+2,
        pgnoFDPMSO+2,
        3,
        fTrue,
        CPAGE::fPagePrimary ) );

    DIRClose( pfucbParent );
    pfucbParent = pfucbNil;

    Call( ErrFILEOpenTable( ppib, ifmp, &pfucbCatalog, szMSO ) );

    if ( !pfucbCatalog->u.pfcb->FSpaceInitialized() )
    {
        pfucbCatalog->u.pfcb->SetPgnoOE( pgnoFDPMSO+1 );
        pfucbCatalog->u.pfcb->SetPgnoAE( pgnoFDPMSO+2 );
        pfucbCatalog->u.pfcb->Lock();
        pfucbCatalog->u.pfcb->SetSpaceInitialized();
        pfucbCatalog->u.pfcb->Unlock();
    }
    Call( ErrSPIOpenOwnExt( ppib, pfucbCatalog->u.pfcb, &pfucbSpace ) );
#ifdef REPAIR_DEBUG_VERBOSE_SPACE
    (*popts->pcprintfDebug)( "%s OwnExt: %d pages ending at %d\r\n", szMSO, cpgOwned, pgnoLast );
#endif

    Call( ErrREPAIRInsertRunIntoSpaceTree(
                ppib,
                ifmp,
                pfucbSpace,
                pgnoLast,
                cpgOwned,
                popts ) );

    DIRClose( pfucbSpace );
    pfucbSpace = pfucbNil;

    LONG cRow;
    for( cRow = JET_MoveFirst;
         ( err = ErrDispMove( sesid, tableid, cRow, NO_GRBIT ) ) == JET_errSuccess;
         cRow = JET_MoveNext )
    {
        KEY key;
        DATA data;

        ULONG cbKey;
        ULONG cbData;

        Call( ErrDispRetrieveColumn( sesid, tableid, columnidKey, pvKey, g_cbPage, &cbKey, NO_GRBIT, NULL ) );
        Call( ErrDispRetrieveColumn( sesid, tableid, columnidData, pvData, g_cbPage, &cbData, NO_GRBIT, NULL ) );

        key.prefix.Nullify();
        key.suffix.SetPv( pvKey );
        key.suffix.SetCb( cbKey );
        data.SetPv( pvData );
        data.SetCb( cbData );

        Call( ErrBTInsert( pfucbCatalog, key, data, fDIRNoVersion|fDIRAppend, NULL ) );
        BTUp( pfucbCatalog );

        ++(popts->psnprog->cunitDone);
        (VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntProgress, popts->psnprog );
    }
    if ( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }

    DIRClose( pfucbCatalog );
    pfucbCatalog = pfucbNil;

HandleError:
    BFFree( pvKey );
    BFFree( pvData );
    if ( pfucbNil != pfucbParent )
    {
        DIRClose( pfucbParent );
    }
    if ( pfucbNil != pfucbCatalog )
    {
        DIRClose( pfucbCatalog );
    }
    if ( pfucbNil != pfucbSpace )
    {
        DIRClose( pfucbSpace );
    }
    return err;
}


LOCAL ERR ErrREPAIRRepairDatabase(
            PIB * const ppib,
            const WCHAR * const wszDatabase,
            BOOL * const pfAttached,
            IFMP * const pifmp,
            const OBJID objidLast,
            const PGNO pgnoLastOE,
            REPAIRTABLE * const prepairtable,
            const BOOL fRepairedCatalog,
            BOOL fRepairGlobalSpace,
            BOOL fRepairMSObjids,
            BOOL fRepairMSLocales,
            TTARRAY * const pttarrayOwnedSpace,
            TTARRAY * const pttarrayAvailSpace,
            PgnoCollection * const ppgnocollShelved,
            const REPAIROPTS * const popts )
{
    Assert( !(popts->grbit & JET_bitDBUtilOptionDontRepair ) );
    Assert( pfAttached );

    ERR err = JET_errSuccess;

    const JET_SESID sesid = (JET_SESID)ppib;

    REPAIRTT repairtt;
    memset( &repairtt, 0, sizeof( REPAIRTT ) );
    repairtt.tableidBadPages    = JET_tableidNil;
    repairtt.tableidAvailable   = JET_tableidNil;
    repairtt.tableidOwned       = JET_tableidNil;
    repairtt.tableidUsed        = JET_tableidNil;

    REPAIRTABLE *   prepairtableT           = NULL;
    DBTIME          dbtimeLast              = 0;
    const OBJID     objidFDPMin             = 5;
    OBJID           objidFDPLast            = objidLast;
    PGNO            pgnoLastOESeen          = pgnoLastOE;

    INT             cTablesToRepair         = 0;

    OBJIDLIST   objidlist;


    prepairtableT = prepairtable;
    while ( prepairtableT )
    {
        ++cTablesToRepair;
        Call( objidlist.ErrAddObjid( prepairtableT->objidFDP ) );
        prepairtableT = prepairtableT->prepairtableNext;
    }
    objidlist.Sort();


    (*popts->pcprintf)( "\r\nScanning the database.\r\n"  );
    (*popts->pcprintfVerbose).Indent();
    Call( ErrREPAIRAttachForRepair(
            sesid,
            wszDatabase,
            pifmp,
            dbtimeLast,
            objidFDPLast,
            popts ) );
    Call( ErrREPAIRCreateTempTables( ppib, fRepairGlobalSpace, &repairtt, popts ) );
    Call( ErrREPAIRScanDB(
            ppib,
            *pifmp,
            &repairtt,
            &dbtimeLast,
            &objidFDPLast,
            &pgnoLastOESeen,
            prepairtable,
            pttarrayOwnedSpace,
            pttarrayAvailSpace,
            ppgnocollShelved,
            popts ) );
    (*popts->pcprintfVerbose).Unindent();

    if ( 0 == dbtimeLast )
    {
        (*popts->pcprintfError)( "dbtimeLast is 0!\r\n" );
        Call( ErrERRCheck( JET_errInternalError ) );
    }

    if ( objidFDPMin > objidFDPLast )
    {
        (*popts->pcprintfError)( "objidLast is %d!\r\n", objidFDPLast );
        Call( ErrERRCheck( JET_errInternalError ) );
    }

    if ( pgnoLastOESeen > pgnoLastOE )
    {
        (*popts->pcprintfError)( "Global space tree is too small (has %d pages, seen %d pages) and there pages beyond the logical size of the file which appear to contain valid data. "
                                 "The space tree will be rebuilt\r\n",
                                 pgnoLastOE, pgnoLastOESeen );
        fRepairGlobalSpace = fTrue;
    }


    FUCBSetSequential( (FUCB *)(repairtt.tableidAvailable) );
    FUCBSetPrereadForward( (FUCB *)(repairtt.tableidAvailable), cpgPrereadSequential );
    FUCBSetSequential( (FUCB *)(repairtt.tableidOwned) );
    FUCBSetPrereadForward( (FUCB *)(repairtt.tableidOwned), cpgPrereadSequential );
    FUCBSetSequential( (FUCB *)(repairtt.tableidUsed) );
    FUCBSetPrereadForward( (FUCB *)(repairtt.tableidUsed), cpgPrereadSequential );


    Call( ErrREPAIRAttachForRepair(
            sesid,
            wszDatabase,
            pifmp,
            dbtimeLast,
            objidFDPLast,
            popts ) );

    if ( fRepairedCatalog )
    {
        Call( ErrREPAIRInsertMSOEntriesToCatalog( ppib, *pifmp, popts ) );
    }

    (*popts->pcprintf)( "\r\nRepairing damaged tables.\r\n"  );
    (VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntBegin, NULL );


    popts->psnprog->cunitTotal  = 0;
    popts->psnprog->cunitDone   = 0;
    popts->psnprog->cunitTotal  = cTablesToRepair;

    if ( fRepairGlobalSpace )
    {
        ++(popts->psnprog->cunitTotal);
    }

    if ( fRepairGlobalSpace )
    {
        Assert( 0 == popts->psnprog->cunitDone );
        Call( ErrREPAIRRepairGlobalSpace( ppib, *pifmp, popts ) );
        ++(popts->psnprog->cunitDone);
        (VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntProgress, popts->psnprog );
        (VOID)ErrBFFlush( *pifmp );
    }

    if ( prepairtable )
    {
        (*popts->pcprintfVerbose)( "\r\nDeleting unicode fixup table.\r\n"  );
        Call( ErrCATDeleteMSU( ppib, *pifmp ) );
    }

    const BOOL fMsoDependentsOutOfDate = ( prepairtable || fRepairedCatalog || fRepairGlobalSpace );

    if ( fMsoDependentsOutOfDate || fRepairMSObjids )
    {
        (*popts->pcprintfVerbose)( "\r\nDeleting MSObjids.\r\n"  );
        Call( ErrCATDeleteMSObjids( ppib, *pifmp ) );
    }

    if ( fMsoDependentsOutOfDate || fRepairMSLocales )
    {
        (*popts->pcprintfVerbose)( "\r\nDeleting MSysLocales.\r\n"  );
        CATTermMSLocales( &(g_rgfmp[*pifmp]) );
        Call( ErrCATDeleteMSLocales( ppib, *pifmp ) );
    }

    (*popts->pcprintfVerbose).Indent();
    prepairtableT = prepairtable;
    while ( prepairtableT )
    {
        if ( FCATObjidsTable( prepairtableT->szTableName ) ||
                FCATLocalesTable( prepairtableT->szTableName ) )
        {
            (*popts->pcprintfVerbose)( "table \"%s\" is a system table and is repaired via attach\r\n", prepairtableT->szTableName );
        }
        else
        {


            FCB::PurgeDatabase( *pifmp, fFalse  );
            Call( ErrREPAIRRepairTable( ppib, *pifmp, &repairtt, prepairtableT, popts ) );


            (VOID)ErrBFFlush( *pifmp );
        }

        prepairtableT = prepairtableT->prepairtableNext;
        ++(popts->psnprog->cunitDone);
        (VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntProgress, popts->psnprog );
    }
    (*popts->pcprintfVerbose).Unindent();


    if ( fMsoDependentsOutOfDate || fRepairMSObjids || fRepairMSLocales )
    {

        (*popts->pcprintfVerbose)( "Closing and detaching database (for MSO dependent table fixups).\r\n" );
        CallS( ErrIsamCloseDatabase( (JET_SESID)ppib, (JET_DBID)*pifmp, 0 ) );

        if ( ErrIsamDetachDatabase( (JET_SESID)ppib, NULL, wszDatabase ) >= JET_errSuccess )
        {
            *pfAttached = fFalse;

            Assert( g_fRepair );
            g_fRepair = fFalse;

            (*popts->pcprintfVerbose)( "Reattaching database (for MSO dependent table fixups).\r\n" );

            Call( ErrIsamAttachDatabase( sesid, wszDatabase, fFalse, NULL, 0, JET_bitDbRecoveryOff ) );
            Assert( JET_wrnDatabaseAttached != err );

            Call( ErrIsamDetachDatabase( sesid, NULL, wszDatabase ) );

            g_fRepair = fTrue;
            Call( ErrIsamAttachDatabase( sesid, wszDatabase, fFalse, NULL, 0, JET_bitDbRecoveryOff ) );
            Assert( JET_wrnDatabaseAttached != err );

            CallS( ErrIsamOpenDatabase( sesid, wszDatabase, NULL, (JET_DBID*)pifmp, JET_bitDbRecoveryOff ) );
            *pfAttached = fTrue;
        }
    }

#ifdef DEBUG
    (*popts->pcprintfVerbose)( "Verifying MSO Dependants fixed up.\r\n" );
    CallS( ErrCATVerifyMSObjids( ppib, *pifmp, popts->pcprintfError ) );
    const ERR errT = ErrCATVerifyMSLocales( ppib, *pifmp, fFalse );
    if ( errT < JET_errSuccess )
    {
        (*popts->pcprintfError)( "ERROR: Failed to validate MSysLocales table, didn't match MSO contents (%d).\r\n", errT );
    }
    CallS( err );
#endif


    (VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntComplete, NULL );

HandleError:

    if ( JET_tableidNil != repairtt.tableidBadPages )
    {
        CallS( ErrDispCloseTable( sesid, repairtt.tableidBadPages ) );
        repairtt.tableidBadPages = JET_tableidNil;
    }
    if ( JET_tableidNil != repairtt.tableidAvailable )
    {
        CallS( ErrDispCloseTable( sesid, repairtt.tableidAvailable ) );
        repairtt.tableidAvailable = JET_tableidNil;
    }
    if ( JET_tableidNil != repairtt.tableidOwned )
    {
        CallS( ErrDispCloseTable( sesid, repairtt.tableidOwned ) );
        repairtt.tableidOwned = JET_tableidNil;
    }
    if ( JET_tableidNil != repairtt.tableidUsed )
    {
        CallS( ErrDispCloseTable( sesid, repairtt.tableidUsed ) );
        repairtt.tableidUsed = JET_tableidNil;
    }

    return err;
}

ERR ErrDIROpenLongRoot( FUCB * pfucb );

LOCAL ERR ErrREPAIRRepairTable(
    PIB * const ppib,
    const IFMP ifmp,
    REPAIRTT * const prepairtt,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts )
{
    ERR     err                 = JET_errSuccess;
    FUCB    * pfucb             = pfucbNil;

    FDPINFO fdpinfo;

    (*popts->pcprintfVerbose)( "repairing table \"%s\"\r\n", prepairtable->szTableName );
    (*popts->pcprintfVerbose).Indent();

    Call( ErrDIRBeginTransaction( ppib, 44069, NO_GRBIT ) );

    if ( prepairtable->fRepairTable )
    {
        (*popts->pcprintfVerbose)( "rebuilding data\r\n" );
        Call( ErrREPAIRRebuildBT(
                    ppib,
                    ifmp,
                    prepairtable,
                    pfucbNil,
                    &prepairtable->pgnoFDP,
                    CPAGE::fPagePrimary | CPAGE::fPageRepair,
                    prepairtt,
                    popts ) );
    }

    fdpinfo.pgnoFDP     = prepairtable->pgnoFDP;
    fdpinfo.objidFDP    = prepairtable->objidFDP;
    Call( ErrFILEOpenTable( ppib, ifmp, &pfucb, prepairtable->szTableName, NO_GRBIT, &fdpinfo ) );

    if ( objidNil != prepairtable->objidLV )
    {
        if ( prepairtable->fRepairLV )
        {
            (*popts->pcprintfVerbose)( "rebuilding long value tree\r\n" );
            Call( ErrREPAIRRebuildBT(
                    ppib,
                    ifmp,
                    prepairtable,
                    pfucb,
                    &prepairtable->pgnoLV,
                    CPAGE::fPageLongValue | CPAGE::fPageRepair,
                    prepairtt,
                    popts ) );
        }
        else
        {
            Call( ErrDIROpenLongRoot( pfucb ) );
        }
    }

    Call( ErrREPAIRFixupTable( ppib, ifmp, prepairtable, popts ) );

    if ( prepairtable->fRepairIndexes )
    {
        (*popts->pcprintfVerbose)( "rebuilding indexes\r\n" );
        Call( ErrREPAIRBuildAllIndexes( ppib, ifmp, &pfucb, prepairtable, popts ) );
    }

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:

    (*popts->pcprintfVerbose).Unindent();

    if ( pfucbNil != pfucb )
    {
        DIRClose( pfucb );
        pfucb = pfucbNil;
    }

    if ( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    return err;
}


LOCAL ERR ErrREPAIRRebuildBT(
    PIB * const ppib,
    const IFMP ifmp,
    REPAIRTABLE * const prepairtable,
    FUCB * const pfucbTable,
    PGNO * const ppgnoFDP,
    const ULONG fPageFlags,
    REPAIRTT * const prepairtt,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    FUCB * pfucb = pfucbNil;
    PGNO    pgnoFDPOld = *ppgnoFDP;
    PGNO    pgnoFDPNew = pgnoNull;

    const OBJID objidTable  = prepairtable->objidFDP;
    const BOOL fRepairLV    = ( pfucbNil != pfucbTable );
    const OBJID objidFDP    = ( fRepairLV ? prepairtable->objidLV : prepairtable->objidFDP );
    const PGNO pgnoParent   = fRepairLV ? prepairtable->pgnoFDP : pgnoSystemRoot;

    Assert( !FCATSystemTable( prepairtable->szTableName ) );

    Call( ErrREPAIRCreateEmptyFDP(
        ppib,
        ifmp,
        objidFDP,
        pgnoParent,
        &pgnoFDPNew,
        fPageFlags,
        fTrue,
        popts ) );

    if ( fRepairLV )
    {
        (*popts->pcprintfDebug)( "LV (%d). new pgnoFDP is %d\r\n", objidFDP, pgnoFDPNew );
    }
    else
    {
        (*popts->pcprintfDebug)( "table %s (%d). new pgnoFDP is %d\r\n", prepairtable->szTableName, objidFDP, pgnoFDPNew );
    }

    Assert( FCB::PfcbFCBGet( ifmp, pgnoFDPOld, NULL, fFalse ) == pfcbNil );
    Assert( FCB::PfcbFCBGet( ifmp, pgnoFDPNew, NULL, fFalse ) == pfcbNil );

    Call( ErrCATChangePgnoFDP(
            ppib,
            ifmp,
            objidTable,
            objidFDP,
            ( fRepairLV ? sysobjLongValue : sysobjTable ),
            pgnoFDPNew ) );

    if ( !fRepairLV && prepairtable->fHasPrimaryIndex )
    {
        Call( ErrCATChangePgnoFDP(
            ppib,
            ifmp,
            objidTable,
            objidFDP,
            sysobjIndex,
            pgnoFDPNew ) );
    }


    if ( !fRepairLV )
    {
        FDPINFO fdpinfo = { pgnoFDPNew, objidFDP };
        Call( ErrFILEOpenTable( ppib, ifmp, &pfucb, prepairtable->szTableName, NO_GRBIT, &fdpinfo ) );
    }
    else
    {
        Call( ErrDIROpenLongRoot( pfucbTable ) );
        Call( ErrFILEOpenLVRoot( pfucbTable, &pfucb, fFalse ) );
        Assert( pfucb->u.pfcb->PfcbTable() != pfcbNil );
    }
    FUCBSetRepair( pfucb );

    Call( ErrREPAIRRebuildSpace( ppib, ifmp, pfucb, pgnoParent, prepairtt, popts ) );
    Call( ErrREPAIRRebuildInternalBT( ppib, ifmp, pfucb, prepairtt, popts ) );

    *ppgnoFDP = pgnoFDPNew;

HandleError:
    if ( pfucbNil != pfucb )
    {
        DIRClose( pfucb );
    }
    return err;
}


LOCAL ERR ErrREPAIRCreateEmptyFDP(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objid,
    const PGNO pgnoParent,
    PGNO * const ppgnoFDPNew,
    const ULONG fPageFlags,
    const BOOL fUnique,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    PGNO pgnoOE = pgnoNull;
    PGNO pgnoAE = pgnoNull;

    const CPG cpgMin    = cpgMultipleExtentMin;
    CPG cpgRequest      = cpgMin;

    FUCB * pfucb = pfucbNil;

    Call( ErrDIROpen( ppib, pgnoParent, ifmp, &pfucb ) );
    if ( pgnoNull == *ppgnoFDPNew )
    {
        Call( ErrSPGetExt(
            pfucb,
            pgnoParent,
            &cpgRequest,
            cpgMin,
            ppgnoFDPNew,
            fSPUnversionedExtent ) );
    }
    pgnoOE = *ppgnoFDPNew + 1;
    pgnoAE = pgnoOE + 1;

    (*popts->pcprintfDebug)( "creating new FDP with %d pages starting at %d\r\n", cpgRequest, *ppgnoFDPNew );

    Call( ErrSPCreateMultiple(
        pfucb,
        pgnoParent,
        *ppgnoFDPNew,
        objid,
        pgnoOE,
        pgnoAE,
        *ppgnoFDPNew + cpgRequest - 1,
        cpgRequest,
        fUnique,
        fPageFlags ) );

HandleError:
    if ( pfucbNil != pfucb )
    {
        DIRClose( pfucb );
    }
    return err;
}


class REPAIRRUN
{
    public:
        REPAIRRUN(
            const JET_SESID sesid,
            const JET_TABLEID tableid,
            const JET_COLUMNID columnidFDP,
            const JET_COLUMNID columnidPgno,
            const OBJID objidFDP );
        ~REPAIRRUN() {}

        ERR ErrREPAIRRUNInit();
        ERR ErrGetRun( PGNO * const ppgnoLast, CPG * const pcpgRun );

    private:
        const JET_SESID     m_sesid;
        const JET_TABLEID   m_tableid;
        const JET_COLUMNID  m_columnidFDP;
        const JET_COLUMNID  m_columnidPgno;
        const OBJID         m_objidFDP;
};


REPAIRRUN::REPAIRRUN(
            const JET_SESID sesid,
            const JET_TABLEID tableid,
            const JET_COLUMNID columnidFDP,
            const JET_COLUMNID columnidPgno,
            const OBJID objidFDP ) :
    m_sesid( sesid ),
    m_tableid( tableid ),
    m_columnidFDP( columnidFDP ),
    m_columnidPgno( columnidPgno ),
    m_objidFDP( objidFDP )
{
}


ERR REPAIRRUN::ErrREPAIRRUNInit()
{
    ERR err = JET_errSuccess;

    Call( ErrIsamMakeKey( m_sesid, m_tableid, &m_objidFDP, sizeof( m_objidFDP ), JET_bitNewKey ) );
    Call( ErrDispSeek( m_sesid, m_tableid, JET_bitSeekGE ) );

HandleError:
    if ( JET_errRecordNotFound == err )
    {
        err = JET_errSuccess;
    }
    return err;
}


ERR REPAIRRUN::ErrGetRun( PGNO * const ppgnoLast, CPG * const pcpgRun )
{
    ERR     err         = JET_errSuccess;
    PGNO    pgnoStart   = pgnoNull;
    CPG     cpgRun      = 0;

    while ( err >= JET_errSuccess )
    {
        ULONG   cbActual;
        PGNO    objidCurr;
        PGNO    pgnoCurr;

        err = ErrDispRetrieveColumn(
                m_sesid,
                m_tableid,
                m_columnidFDP,
                &objidCurr,
                sizeof( objidCurr ),
                &cbActual,
                NO_GRBIT,
                NULL );
        if ( JET_errNoCurrentRecord == err )
        {
            break;
        }
        Call( err );
        Assert( sizeof( objidCurr ) == cbActual );

        if ( objidCurr != m_objidFDP )
        {
            break;
        }

        Call( ErrDispRetrieveColumn(
                m_sesid,
                m_tableid,
                m_columnidPgno,
                &pgnoCurr,
                sizeof( pgnoCurr ),
                &cbActual,
                NO_GRBIT,
                NULL ) );
        Assert( sizeof( pgnoCurr ) == cbActual );

        if ( pgnoNull == pgnoStart )
        {
            pgnoStart = pgnoCurr;
            cpgRun = 1;
        }
        else if ( pgnoStart + cpgRun == pgnoCurr )
        {
            ++cpgRun;
        }
        else
        {
            Assert( pgnoStart + cpgRun < pgnoCurr );
            break;
        }

        err = ErrDispMove( m_sesid, m_tableid, JET_MoveNext, NO_GRBIT );
    }

    if ( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }

HandleError:
    *ppgnoLast  = pgnoStart + cpgRun - 1;
    *pcpgRun    = cpgRun;

    return err;
}


LOCAL ERR ErrREPAIRRebuildSpace(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB * const pfucb,
    const PGNO pgnoParent,
    REPAIRTT * const prepairtt,
    const REPAIROPTS * const popts )
{
    ERR             err     = JET_errSuccess;

    const JET_SESID sesid   = reinterpret_cast<JET_SESID>( ppib );

#ifdef DEBUG
    PGNO    pgnoPrev        = pgnoNull;
    CPG     cpgPrev         = 0;
#endif

    PGNO    pgnoLast        = pgnoNull;
    CPG     cpgRun          = 0;

    FUCB    *pfucbOE        = pfucbNil;
    FUCB    *pfucbParent    = pfucbNil;

    const OBJID objidFDP    = pfucb->u.pfcb->ObjidFDP();

    Assert( JET_tableidNil != prepairtt->tableidOwned );
    REPAIRRUN repairrunOwnedExt(
        sesid,
        prepairtt->tableidOwned,
        prepairtt->rgcolumnidOwned[0],
        prepairtt->rgcolumnidOwned[1],
        objidFDP );

    if ( !pfucb->u.pfcb->FSpaceInitialized() )
    {
        pfucb->u.pfcb->SetPgnoOE( pfucb->u.pfcb->PgnoFDP()+1 );
        pfucb->u.pfcb->SetPgnoAE( pfucb->u.pfcb->PgnoFDP()+2 );

        pfucb->u.pfcb->Lock();
        pfucb->u.pfcb->SetSpaceInitialized();
        pfucb->u.pfcb->Unlock();
    }

    Call( repairrunOwnedExt.ErrREPAIRRUNInit() );

    Assert( pgnoNull != pgnoParent );
    if ( pgnoNull != pgnoParent )
    {
        Call( ErrBTOpen( ppib, pgnoParent, ifmp, &pfucbParent ) );
        Assert( pfucbNil != pfucbParent );
        Assert( pfcbNil != pfucbParent->u.pfcb );
        Assert( pfucbParent->u.pfcb->FInitialized() );
        Assert( pcsrNil == pfucbParent->pcsrRoot );
    }

    Call( ErrSPIOpenOwnExt( ppib, pfucb->u.pfcb, &pfucbOE ) );

#ifdef DEBUG
    pgnoPrev = pgnoNull;
    cpgPrev = 0;
#endif

    forever
    {
        Call( repairrunOwnedExt.ErrGetRun( &pgnoLast, &cpgRun ) );
        if ( pgnoNull == pgnoLast || 0 == cpgRun )
        {
            break;
        }
        Assert( pgnoLast - cpgRun > pgnoPrev );
#ifdef REPAIR_DEBUG_VERBOSE_SPACE
        (*popts->pcprintfDebug)( "OwnExt:  %d pages ending at %d\r\n", cpgRun, pgnoLast );
#endif

        Call( ErrSPReserveSPBufPagesForSpaceTree( pfucb, pfucbOE, pfucbParent ) );

        Call( ErrREPAIRInsertRunIntoSpaceTree(
                    ppib,
                    ifmp,
                    pfucbOE,
                    pgnoLast,
                    cpgRun,
                    popts ) );
    }

HandleError:
    if ( pfucbNil != pfucbOE )
    {
        BTClose( pfucbOE );
    }

    if ( pfucbNil != pfucbParent )
    {
        Assert( pcsrNil == pfucbParent->pcsrRoot );
        BTClose( pfucbParent );
    }

    Assert( pcsrNil == pfucb->pcsrRoot );
    BTUp( pfucb );

    return err;
}


LOCAL ERR ErrREPAIRInsertRunIntoSpaceTree(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB * const pfucb,
    const PGNO pgnoLast,
    const CPG cpgRun,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    KEY         key;
    DATA        data;
    BYTE        rgbKey[sizeof(PGNO)];

    Assert( FFUCBSpace( pfucb ) );

    KeyFromLong( rgbKey, pgnoLast );
    key.prefix.Nullify();
    key.suffix.SetPv( rgbKey );
    key.suffix.SetCb( sizeof(pgnoLast) );

    LittleEndian<LONG> le_cpgRun = cpgRun;
    data.SetPv( &le_cpgRun );
    data.SetCb( sizeof(cpgRun) );

    Call( ErrBTInsert( pfucb, key, data, fDIRNoVersion | fDIRNoLog ) );
    BTUp( pfucb );

HandleError:
    Assert( JET_errKeyDuplicate != err );
    return err;
}


LOCAL ERR ErrREPAIRResetRepairFlags(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB * const pfucb,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    const PGNO pgnoFDP = pfucb->u.pfcb->PgnoFDP();

    CSR csr;
    CallR( csr.ErrGetRIWPage( ppib, ifmp, pgnoFDP ) );

    while ( !csr.Cpage().FLeafPage() )
    {
        NDMoveFirstSon( pfucb, &csr );
        const PGNO pgnoChild    = *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv();
        Call( csr.ErrSwitchPage( ppib, ifmp, pgnoChild ) );
    }
    Assert( pgnoNull == csr.Cpage().PgnoPrev() );

    for( ; ; )
    {

        ULONG ulFlags = csr.Cpage().FFlags();
        Assert( ulFlags & CPAGE::fPageLeaf );
        Assert( ulFlags & CPAGE::fPageRepair );
        if ( 0 != csr.Cpage().Clines() )
        {
            ulFlags &= ~CPAGE::fPageLeaf;
            ulFlags = ulFlags | CPAGE::fPageParentOfLeaf;
        }
        else
        {
            Assert( csr.Cpage().FRootPage() );
        }
        ulFlags &= ~CPAGE::fPageRepair;
        Assert( !( ulFlags & CPAGE::fPageEmpty ) );
        Assert( !( ulFlags & CPAGE::fPagePreInit ) );
        Assert( !( ulFlags & CPAGE::fPageSpaceTree ) );
        Assert( !( ulFlags & CPAGE::fPageIndex ) );

        const PGNO pgnoNext = csr.Cpage().PgnoNext();

        Call( csr.ErrUpgrade( ) );
        csr.Dirty();
        csr.Cpage().SetPgnoPrev( pgnoNull );
        csr.Cpage().SetPgnoNext( pgnoNull );
        csr.Cpage().SetFlags( ulFlags );
        csr.Downgrade( latchReadNoTouch );
        csr.ReleasePage();
        csr.Reset();

        if ( pgnoNull != pgnoNext )
        {
            Call( csr.ErrGetRIWPage( ppib, ifmp, pgnoNext ) );
        }
        else
        {
            break;
        }
    }

HandleError:
    csr.ReleasePage();
    csr.Reset();
    return err;
}


LOCAL ERR ErrREPAIRRebuildInternalBT(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB * const pfucb,
    REPAIRTT * const prepairtt,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;


    const CPG           cpgDatabase = PgnoLast( ifmp );

    const OBJID         objidFDP    = pfucb->u.pfcb->ObjidFDP();
    const JET_SESID     sesid   = reinterpret_cast<JET_SESID>( ppib );
    const JET_TABLEID   tableid = prepairtt->tableidUsed;
    const JET_COLUMNID  columnidFDP     = prepairtt->rgcolumnidUsed[0];
    const JET_COLUMNID  columnidKey     = prepairtt->rgcolumnidUsed[1];
    const JET_COLUMNID  columnidPgno    = prepairtt->rgcolumnidUsed[2];

    PGNO pgnoPrev = pgnoNull;
    PGNO pgnoNext = pgnoNull;
    PGNO pgnoCurr = pgnoNull;

    CPG cpgInserted = 0;

    ERR errMove     = JET_errSuccess;

    DIRUp( pfucb );

    Call( ErrIsamMakeKey( sesid, tableid, &objidFDP, sizeof( objidFDP ), JET_bitNewKey ) );
    err = ErrDispSeek( sesid, tableid, JET_bitSeekGE );
    if ( JET_errRecordNotFound == err )
    {
        err     = JET_errSuccess;
        errMove = JET_errNoCurrentRecord;
    }
    Call( err );

    while ( JET_errSuccess == errMove )
    {
        ULONG   cbKey1  = 0;
        BYTE    rgbKey1[cbKeyAlloc];
        ULONG   cbKey2  = 0;
        BYTE    rgbKey2[cbKeyAlloc];
        INT     ibKeyCommon;

        ULONG   cbActual;

        KEY     key;
        DATA    data;

        OBJID objidFDPCurr;
        Call( ErrDispRetrieveColumn(
                sesid,
                tableid,
                columnidFDP,
                &objidFDPCurr,
                sizeof( objidFDPCurr ),
                &cbActual,
                NO_GRBIT,
                NULL ) );
        Assert( sizeof( objidFDP ) == cbActual );
        if ( objidFDP != objidFDPCurr )
        {
            break;
        }

        Call( ErrDispRetrieveColumn(
                sesid,
                tableid,
                columnidPgno,
                &pgnoCurr,
                sizeof( pgnoCurr ),
                &cbActual,
                NO_GRBIT,
                NULL ) );
        Assert( sizeof( pgnoCurr ) == cbActual );
        Assert( pgnoNull == pgnoNext && pgnoNull == pgnoPrev
                || pgnoCurr == pgnoNext );

        if ( pgnoCurr > pgnoPrev + CpgMinRepairSequentialPreread( g_rgfmp[ifmp].CbPage() )
            && pgnoCurr + CpgMinRepairSequentialPreread( g_rgfmp[ifmp].CbPage() ) < (SIZE_T)cpgDatabase )
        {
            BFPrereadPageRange( ifmp, pgnoCurr, CpgMinRepairSequentialPreread( g_rgfmp[ifmp].CbPage() ), bfprfDefault, ppib->BfpriPriority( ifmp ), *TcRepair() );
        }

        Call( ErrDispRetrieveColumn(
                sesid,
                tableid,
                columnidKey,
                rgbKey1,
                sizeof( rgbKey1 ),
                &cbKey1,
                NO_GRBIT,
                NULL ) );

NextPage:
        pgnoNext = pgnoNull;
        ibKeyCommon = -1;
        errMove = ErrDispMove( sesid, tableid, JET_MoveNext, NO_GRBIT );
        if ( JET_errSuccess != errMove
            && JET_errNoCurrentRecord != errMove )
        {
            Call( errMove );
        }
        if ( JET_errSuccess == errMove )
        {
            Call( ErrDispRetrieveColumn(
                    sesid,
                    tableid,
                    columnidFDP,
                    &objidFDPCurr,
                    sizeof( objidFDPCurr ),
                    &cbActual,
                    NO_GRBIT,
                    NULL ) );
            Assert( sizeof( objidFDPCurr ) == cbActual );
            if ( objidFDP == objidFDPCurr )
            {
                Call( ErrDispRetrieveColumn(
                        sesid,
                        tableid,
                        columnidPgno,
                        &pgnoNext,
                        sizeof( pgnoNext ),
                        &cbActual,
                        NO_GRBIT,
                        NULL ) );
                Assert( sizeof( pgnoNext ) == cbActual );

                CSR csr;
                Call( csr.ErrGetReadPage( ppib, ifmp, pgnoNext, bflfNoTouch ) );
                csr.SetILine( 0 );
                KEYDATAFLAGS kdf;
                NDIGetKeydataflags( csr.Cpage(), 0, &kdf );
                cbKey2 = kdf.key.Cb();
                kdf.key.CopyIntoBuffer( rgbKey2, sizeof( rgbKey2 ) );
                csr.ReleasePage( fTrue );
                csr.Reset();

                for( ibKeyCommon = 0;
                     (ULONG)ibKeyCommon < cbKey1 && (ULONG)ibKeyCommon < cbKey2 && rgbKey1[ibKeyCommon] == rgbKey2[ibKeyCommon];
                     ++ibKeyCommon )
                     ;
                if ( (ULONG)ibKeyCommon >= cbKey2
                    || (ULONG)ibKeyCommon < cbKey1 && rgbKey1[ibKeyCommon] > rgbKey2[ibKeyCommon] )
                {
                    (*popts->pcprintfVerbose)
                        ( "page %d and page %d have duplicate/overlapping keys. discarding page %d\r\n",
                            pgnoCurr, pgnoNext, pgnoNext );
                    goto NextPage;
                }
            }
        }

        key.prefix.Nullify();
        key.suffix.SetPv( rgbKey2 );
        key.suffix.SetCb( ibKeyCommon + 1 );

        LittleEndian<PGNO> le_pgnoCurr = pgnoCurr;
        data.SetPv( &le_pgnoCurr );
        data.SetCb( sizeof( pgnoCurr ) );

#ifdef REPAIR_DEBUG_VERBOSE_SPACE
        (*popts->pcprintfDebug)( "Page %d: pgnoPrev %d, pgnoNext %d, key-length %d\r\n",
            pgnoCurr, pgnoPrev, pgnoNext, key.Cb() );
#endif

        ++cpgInserted;

        if ( pgnoNull != pgnoPrev )
        {

            Call( ErrDIRGet( pfucb ) );
            err = Pcsr( pfucb )->ErrUpgrade();

            while ( errBFLatchConflict == err )
            {

                CallS( ErrDIRRelease( pfucb ) );
                Call( ErrDIRGet( pfucb ) );
                err = Pcsr( pfucb )->ErrUpgrade();
            }
            Call( err );
        }
        Call( ErrDIRAppend( pfucb, key, data, fDIRNoVersion | fDIRNoLog ) );
        if ( pgnoNull != pgnoNext )
        {
            Call( ErrDIRRelease( pfucb ) );
        }
        else
        {
            DIRUp( pfucb );
        }

        CSR csr;
        Call( csr.ErrGetRIWPage( ppib, ifmp, pgnoCurr ) );
        Call( csr.ErrUpgrade( ) );
        csr.Dirty();
        Assert( csr.Pgno() == pgnoCurr );
        Assert( csr.Cpage().ObjidFDP() == objidFDP );
        csr.Cpage().SetPgnoPrev( pgnoPrev );
        csr.Cpage().SetPgnoNext( pgnoNext );
        csr.ReleasePage( fTrue );
        csr.Reset();

        pgnoPrev = pgnoCurr;
    }

    CallSx( errMove, JET_errNoCurrentRecord );

    Call( ErrREPAIRResetRepairFlags( ppib, ifmp, pfucb, popts ) );

    (*popts->pcprintfVerbose)( "b-tree rebuilt with %d data pages\r\n", cpgInserted );

HandleError:
    Assert( JET_errKeyDuplicate != err );
    return err;
}


LOCAL ERR ErrREPAIRFixLVs(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoLV,
    TTMAP * const pttmapLVTree,
    const BOOL fFixMissingLVROOT,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    FUCB *  pfucb       = pfucbNil;
    BOOL    fDone       = fFalse;
    LvId    lidCurr;
    INT     clvDeleted  = 0;

    DIB dib;
    dib.pos     = posFirst;
    dib.pbm     = NULL;
    dib.dirflag = fDIRNull;

    Assert( pgnoNull != pgnoLV );

    (*popts->pcprintfVerbose)( "fixing long value tree\r\n" );

    Call( ErrDIROpen( ppib, pgnoLV, ifmp, &pfucb ) );
    Assert( pfucbNil != pfucb );
    Assert( pfucb->u.pfcb->PfcbTable() != pfcbNil );

    FUCBSetIndex( pfucb );
    FUCBSetSequential( pfucb );
    FUCBSetPrereadForward( pfucb, cpgPrereadSequential );

    err = ErrDIRDown( pfucb, &dib );
    if ( JET_errRecordNotFound == err )
    {


        err = JET_errSuccess;
        fDone = fTrue;
    }
    Call( err );

    while ( !fDone )
    {
        const LvId  lidOld                  = lidCurr;
        BOOL        fLVComplete             = fFalse;
        BOOL        fLVPartiallyScrubbed    = fFalse;
        BOOL        fLVHasRoot              = fFalse;
        ULONG       ulRefcount              = 0;
        ULONG       ulSize                  = 0;

        lidCurr = 0;

        Call( ErrREPAIRCheckLV( pfucb, &lidCurr, &ulRefcount, &ulSize, &fLVHasRoot, &fLVComplete, &fLVPartiallyScrubbed, &fDone ) );

        const BOOL fDeleteIncompleteLV = ( !fLVComplete && !fLVPartiallyScrubbed );

        const BOOL fDeleteRootlessLV = ( !fLVHasRoot && !fFixMissingLVROOT );
        const BOOL fDeleteRefCountedScrubbedLV = ( ulRefcount != 0 && fLVPartiallyScrubbed );
        
        if ( fDeleteIncompleteLV || fDeleteRootlessLV || fDeleteRefCountedScrubbedLV )
        {
            Assert( 0 != lidCurr );

            if ( fDeleteIncompleteLV )
            {
                (*popts->pcprintfVerbose)( "long value 0x%I64x is not complete. deleting\r\n", (_LID64)lidCurr );
            }
            if ( fDeleteRootlessLV )
            {
                (*popts->pcprintfVerbose)( "long value 0x%I64x does not have a root. deleting\r\n", (_LID64)lidCurr );
            }
            if ( fDeleteRefCountedScrubbedLV )
            {
                (*popts->pcprintfVerbose)( "long value 0x%I64x is partially scrubbed, but has a non-zero reference count. deleting\r\n", (_LID64)lidCurr );
            }

            Call( ErrREPAIRDeleteLV( pfucb, lidCurr ) );
            ++clvDeleted;

            Call( ErrREPAIRNextLV( pfucb, lidCurr, &fDone ) );
        }
        else
        {
            if ( !fLVHasRoot )
            {
                Assert( fFixMissingLVROOT );
                Assert( 0 != ulRefcount );
                Assert( 0 != ulSize );

                FUCB * pfucbLVRoot = pfucbNil;

                if ( !fDone )
                {
                    Call( ErrDIRRelease( pfucb ) );
                }
                else
                {
                    DIRUp( pfucb );
                }

                Call( ErrDIROpen( ppib, pgnoLV, ifmp, &pfucbLVRoot ) );

                (*popts->pcprintfVerbose)( "long value 0x%I64x has no root. creating a root with refcount %d and size %d\r\n", (_LID64)lidCurr, ulRefcount, ulSize );

                err = ErrREPAIRCreateLVRoot( pfucbLVRoot, lidCurr, ulRefcount, ulSize );


                DIRClose( pfucbLVRoot );
                pfucbLVRoot = pfucbNil;

                Call( err );

                if ( !fDone )
                {
                    Call( ErrDIRGet( pfucb ) );
                }
            }


            Assert( lidCurr > lidOld );
            if ( !fDone )
            {
                Call( ErrDIRRelease( pfucb ) );
                Call( pttmapLVTree->ErrInsertKeyValue( lidCurr, ulRefcount ) );
                Call( ErrDIRGet( pfucb ) );
            }
            else
            {
                DIRUp( pfucb );
                Call( pttmapLVTree->ErrInsertKeyValue( lidCurr, ulRefcount ) );
            }
        }
    }

HandleError:
    if ( pfucbNil != pfucb )
    {
        DIRClose( pfucb );
    }

    return err;
}


LOCAL ERR ErrREPAIRCheckUpdateLVsFromColumn(
    FUCB * const pfucb,
    const PGNO pgnoLV,
    TAGFLD_ITERATOR& tagfldIterator,
    const TTMAP * const pttmapLVTree,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    while ( JET_errSuccess == ( err = tagfldIterator.ErrMoveNext() ) )
    {
        const LvId lid = LidOfSeparatedLV( tagfldIterator.PbData(), tagfldIterator.CbData() );

        if ( tagfldIterator.FSeparated() )
        {
            if ( pgnoNull == pgnoLV )
            {
                (*popts->pcprintfError)( "record (%d:%d) references non-existant LID (%I64u), the table has no LV tree. deleting\r\n",
                                            Pcsr( pfucb )->Pgno(), Pcsr( pfucb )->ILine(), (_LID64)lid );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }

            ULONG ulUnused;
            err = pttmapLVTree->ErrGetValue( lid, &ulUnused );
            if ( JET_errRecordNotFound == err )
            {
                (*popts->pcprintfError)( "record (%d:%d) references non-existant LID (%I64u). deleting\r\n",
                                            Pcsr( pfucb )->Pgno(), Pcsr( pfucb )->ILine(), (_LID64)lid );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
            else
            {
                Call( err );
            }
        }
    }

    if ( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRUpdateLVsFromColumn(
    FUCB * const pfucb,
    TAGFLD_ITERATOR& tagfldIterator,
    TTMAP * const pttmapRecords,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    while ( JET_errSuccess == ( err = tagfldIterator.ErrMoveNext() ) )
    {
        if ( tagfldIterator.FSeparated() )
        {
            const LvId lid = LidOfSeparatedLV( tagfldIterator.PbData(), tagfldIterator.CbData() );

            Call( pttmapRecords->ErrIncrementValue( lid ) );
        }
    }

    if ( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRAddOneCatalogRecord(
    PIB * const ppib,
    const IFMP ifmp,
    const ULONG objidTable,
    const COLUMNID  columnid,
    const USHORT ibRecordOffset,
    const ULONG cbMaxLen,
    const REPAIROPTS * const popts )
{
    ERR             err = JET_errSuccess;
    CHAR            szColumnName[32];
    FIELD           field;

    Assert( objidTable > objidSystemRoot );

    (*popts->pcprintfStats).Indent();


    OSStrCbFormatA( szColumnName, sizeof(szColumnName),  "JetStub_%u_%u", objidTable, columnid);

    memset( &field, 0, sizeof(field) );

    field.coltyp = JET_coltypNil;
    field.cbMaxLen = cbMaxLen;
    field.ffield = 0;
    field.ibRecordOffset = ibRecordOffset;
    field.cp = 0;

    CallR( ErrDIRBeginTransaction( ppib, 60453, NO_GRBIT ) );
    err = ErrCATAddTableColumn(
                ppib,
                ifmp,
                objidTable,
                szColumnName,
                columnid,
                &field,
                NULL,
                0,
                NULL,
                NULL,
                0 );

    if ( JET_errSuccess == err )
    {
        (*popts->pcprintfVerbose)( "Inserted a column record (objidTable %d:columnid %d) into catalogs\r\n", objidTable, columnid );
    }
     else
    {
        if ( JET_errColumnDuplicate == err )
        {
            (*popts->pcprintfVerbose)( "The column record (%d) for table (%d) has already existed in catalogs\r\n", columnid, objidTable );
            err = JET_errSuccess;
        }
        (*popts->pcprintfVerbose)( "Inserting a column record (objidTable %d:columnid %d) into catalogs fails (%d)\r\n", objidTable, columnid, err );
    }

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
    (*popts->pcprintfVerbose).Unindent();

    if ( JET_errSuccess != err )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}


LOCAL ERR ErrREPAIRInsertDummyRecordsToCatalog(
    PIB * const ppib,
    const IFMP ifmp,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts )
{
    ERR         err     = JET_errSuccess;

    FUCB    *   pfucbTable              = pfucbNil;

    err = ErrFILEOpenTable( ppib, ifmp, &pfucbTable, prepairtable->szTableName );

    if ( JET_errSuccess != err )
    {
        err = JET_errSuccess;
    }
    else
    {

        if ( prepairtable->fidFixedLast > pfucbTable->u.pfcb->Ptdb()->FidFixedLast() )
        {
            COLUMNID    columnidFixedLast       = ColumnidOfFid( pfucbTable->u.pfcb->Ptdb()->FidFixedLast(), fFalse );
            USHORT      ibRecordOffsetLastOld   = REC::cbRecordMin;
            ULONG       cbFixedLastOld          = 0;

#ifdef GET_FIXED_COLUMN_METADATA_FROM_TDB
            if ( pfucbTable->u.pfcb->Ptdb()->FidFixedFirst() >= pfucbTable->u.pfcb->Ptdb()->FidFixedLast() )
            {
                FIELD * const   pfield          = pfucbTable->u.pfcb->Ptdb()->PfieldFixed( columnidFixedLast );

                ibRecordOffsetLastOld = pfield->ibRecordOffset;
                cbFixedLastOld = pfield->cbMaxLen;
            }
#else
            JET_COLTYP  colTypeLastOld          = JET_coltypNil;

            FUCB        *   pfucbCatalog    = pfucbNil;
            USHORT      sysobj  = sysobjColumn;

            Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );
            Assert( pfucbNil != pfucbCatalog );
            Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
            Assert( !Pcsr( pfucbCatalog )->FLatched() );

            Call( ErrIsamMakeKey(
                        ppib,
                        pfucbCatalog,
                        (BYTE *)&(prepairtable->objidFDP),
                        sizeof(prepairtable->objidFDP),
                        JET_bitNewKey ) );
            Call( ErrIsamMakeKey(
                        ppib,
                        pfucbCatalog,
                        (BYTE *)&sysobj,
                        sizeof(sysobj),
                        NO_GRBIT ) );
            Call( ErrIsamMakeKey(
                        ppib,
                        pfucbCatalog,
                        (BYTE *)&(columnidFixedLast),
                        sizeof(COLUMNID),
                        NO_GRBIT ) );
            err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
            Assert( err <= JET_errSuccess );

            if ( JET_errSuccess == err )
            {
                DATA        dataField;

                Assert( !Pcsr( pfucbCatalog )->FLatched() );
                Call( ErrDIRGet( pfucbCatalog ) );

                Assert( FFixedFid( fidMSO_Coltyp ) );
                Call( ErrRECIRetrieveFixedColumn(
                            pfcbNil,
                            pfucbCatalog->u.pfcb->Ptdb(),
                            fidMSO_Coltyp,
                            pfucbCatalog->kdfCurr.data,
                            &dataField ) );
                Assert( dataField.Cb() == sizeof(JET_COLTYP) );
                colTypeLastOld = *( UnalignedLittleEndian< JET_COLTYP > *)dataField.Pv();

                Assert( FFixedFid( fidMSO_SpaceUsage ) );
                Call( ErrRECIRetrieveFixedColumn(
                            pfcbNil,
                            pfucbCatalog->u.pfcb->Ptdb(),
                            fidMSO_SpaceUsage,
                            pfucbCatalog->kdfCurr.data,
                            &dataField ) );
                Assert( dataField.Cb() == sizeof(ULONG) );
                cbFixedLastOld = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
                Assert( 0 != cbFixedLastOld );

                Assert( FFixedFid( fidMSO_RecordOffset ) );
                Call( ErrRECIRetrieveFixedColumn(
                            pfcbNil,
                            pfucbCatalog->u.pfcb->Ptdb(),
                            fidMSO_RecordOffset,
                            pfucbCatalog->kdfCurr.data,
                            &dataField ) );
                if ( JET_wrnColumnNull == err )
                {
                    Assert( dataField.Cb() == 0 );
                    Assert( REC::cbRecordMin == ibRecordOffsetLastOld );
                }
                else
                {
                    Assert( dataField.Cb() == sizeof(REC::RECOFFSET) );
                    ibRecordOffsetLastOld = *(UnalignedLittleEndian< REC::RECOFFSET > *)dataField.Pv();
                    Assert( ibRecordOffsetLastOld >= REC::cbRecordMin );
                }
            }

            if ( pfucbNil != pfucbCatalog )
            {
                err =  ErrCATClose( ppib, pfucbCatalog );
            }
            err = JET_errSuccess;

            switch ( colTypeLastOld )
            {
                case JET_coltypBit:
                case JET_coltypUnsignedByte:    cbFixedLastOld = 1; break;
                case JET_coltypShort:
                case JET_coltypUnsignedShort:   cbFixedLastOld = 2; break;
                case JET_coltypLong:
                case JET_coltypUnsignedLong:
                case JET_coltypIEEESingle:      cbFixedLastOld = 4; break;
                case JET_coltypLongLong:
                case JET_coltypUnsignedLongLong:
                case JET_coltypCurrency:
                case JET_coltypIEEEDouble:
                case JET_coltypDateTime:        cbFixedLastOld = 8; break;
                case JET_coltypGUID:            cbFixedLastOld = 16; break;
                default:                        break;
            }
#endif

            

            ULONG cbMaxLen = prepairtable->ibEndOfFixedData
                            - ibRecordOffsetLastOld
                            - cbFixedLastOld
                            - ( prepairtable->fidFixedLast % 8 ? prepairtable->fidFixedLast/8 + 1 : prepairtable->fidFixedLast/8 )
                            - (prepairtable->fidFixedLast - FidOfColumnid(columnidFixedLast) - 1 );

            if ( prepairtable->ibEndOfFixedData <
                    ( ibRecordOffsetLastOld + cbFixedLastOld
                      + ( prepairtable->fidFixedLast % 8 ? prepairtable->fidFixedLast/8 + 1 : prepairtable->fidFixedLast/8 )
                      + (prepairtable->fidFixedLast - FidOfColumnid(columnidFixedLast) - 1 ) ) )
            {
                err = ErrERRCheck( JET_errInvalidParameter );
                goto HandleError;
            }

            ULONG ulTempRecordOffset = (ibRecordOffsetLastOld
                            + cbFixedLastOld
                            + (prepairtable->fidFixedLast - FidOfColumnid(columnidFixedLast) - 1 ) );

            USHORT ibRecordOffset = (USHORT) ulTempRecordOffset;
            if ( (ULONG) ibRecordOffset  != ulTempRecordOffset )
            {
                err = ErrERRCheck( JET_errInvalidParameter );
                goto HandleError;
            }
            err = ErrREPAIRAddOneCatalogRecord(
                        ppib,
                        ifmp,
                        prepairtable->objidFDP,
                        prepairtable->fidFixedLast,
                        ibRecordOffset,
                        cbMaxLen,
                        popts );
        }

        if ( prepairtable->fidVarLast > pfucbTable->u.pfcb->Ptdb()->FidVarLast() )
        {
            err = ErrREPAIRAddOneCatalogRecord(
                        ppib,
                        ifmp,
                        prepairtable->objidFDP,
                        prepairtable->fidVarLast,
                        0,
                        0,
                        popts );
        }

        if ( prepairtable->fidTaggedLast > pfucbTable->u.pfcb->Ptdb()->FidTaggedLast() )
        {
            err = ErrREPAIRAddOneCatalogRecord(
                        ppib,
                        ifmp,
                        prepairtable->objidFDP,
                        prepairtable->fidTaggedLast,
                        0,
                        0,
                        popts );
        }

        err = JET_errSuccess;
    }

HandleError:
    if ( pfucbNil != pfucbTable )
    {
        Call( ErrFILECloseTable( ppib, pfucbTable ) );
        pfucbTable = pfucbNil;
    }

    return err;
}


LOCAL ERR ErrREPAIRCheckFixOneRecord(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const PGNO pgnoLV,
    const DATA& dataRec,
    FUCB * const pfucb,
    const TTMAP * const pttmapLVTree,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    TAGFIELDS_ITERATOR tagfieldsIterator( dataRec );
    tagfieldsIterator.MoveBeforeFirst();

    while ( JET_errSuccess == ( err = tagfieldsIterator.ErrMoveNext() ) )
    {
        if ( !tagfieldsIterator.FDerived() )
        {
            prepairtable->fidTaggedLast = max( tagfieldsIterator.Fid(), prepairtable->fidTaggedLast );
        }

        if ( tagfieldsIterator.FNull() )
        {
            continue;
        }
        else
        {
            tagfieldsIterator.TagfldIterator().MoveBeforeFirst();

            if ( tagfieldsIterator.FLV() )
            {
                Call( ErrREPAIRCheckUpdateLVsFromColumn(
                        pfucb,
                        pgnoLV,
                        tagfieldsIterator.TagfldIterator(),
                        pttmapLVTree,
                        popts ) );
            }
        }
    }

    if ( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }
    Call( err );

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRFixOneRecord(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const DATA& dataRec,
    FUCB * const pfucb,
    TTMAP * const pttmapRecords,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    TAGFIELDS_ITERATOR tagfieldsIterator( dataRec );

    tagfieldsIterator.MoveBeforeFirst();
    while ( JET_errSuccess == ( err = tagfieldsIterator.ErrMoveNext() ) )
    {
        if ( tagfieldsIterator.FNull() )
        {
            continue;
        }
        else
        {
            tagfieldsIterator.TagfldIterator().MoveBeforeFirst();

            if ( tagfieldsIterator.FLV() )
            {
                Call( ErrREPAIRUpdateLVsFromColumn(
                        pfucb,
                        tagfieldsIterator.TagfldIterator(),
                        pttmapRecords,
                        popts ) );
            }
        }
    }

    if ( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRFixRecords(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoFDP,
    const PGNO pgnoLV,
    TTMAP * const pttmapRecords,
    const TTMAP * const pttmapLVTree,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    FUCB *  pfucb   = pfucbNil;

    INT crecordDeleted  = 0;

    DIB dib;
    dib.pos     = posFirst;
    dib.pbm     = NULL;
    dib.dirflag = fDIRNull;

    (*popts->pcprintfVerbose)( "fixing records\r\n" );

    Call( ErrDIROpen( ppib, pgnoFDP, ifmp, &pfucb ) );
    Assert( pfucbNil != pfucb );

    FUCBSetIndex( pfucb );
    FUCBSetSequential( pfucb );
    FUCBSetPrereadForward( pfucb, cpgPrereadSequential );


    Assert( NULL == pfucb->pvWorkBuf );
    RECIAllocCopyBuffer( pfucb );

    err = ErrDIRDown( pfucb, &dib );

    while ( err >= 0 )
    {


        UtilMemCpy( pfucb->dataWorkBuf.Pv(), pfucb->kdfCurr.data.Pv(), pfucb->kdfCurr.data.Cb() );

        const REC * const prec      = (REC *)( pfucb->dataWorkBuf.Pv() );

        DATA dataRec;
        dataRec.SetPv( pfucb->dataWorkBuf.Pv() );
        dataRec.SetCb( pfucb->kdfCurr.data.Cb() );

        Call( ErrDIRRelease( pfucb ) );

        if ( prepairtable->fidFixedLast < prec->FidFixedLastInRec() )
        {
            prepairtable->fidFixedLast  = prec->FidFixedLastInRec();
            prepairtable->ibEndOfFixedData  = prec->IbEndOfFixedData();
        }

        prepairtable->fidVarLast    = max( prec->FidVarLastInRec(), prepairtable->fidVarLast );

        err = ErrREPAIRCheckFixOneRecord(
                ppib,
                ifmp,
                pgnoFDP,
                pgnoLV,
                dataRec,
                pfucb,
                pttmapLVTree,
                prepairtable,
                popts );
        if ( JET_errDatabaseCorrupted == err )
        {
            (*popts->pcprintfError)( "record (%d:%d) is corrupted. Deleting\r\n",
                                    Pcsr( pfucb )->Pgno(), Pcsr( pfucb )->ILine() );
            UtilReportEvent( eventWarning, REPAIR_CATEGORY, REPAIR_BAD_COLUMN_ID, 0, NULL );
            Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
            ++crecordDeleted;
        }
        else if ( JET_errSuccess == err )
        {
            Call( ErrREPAIRFixOneRecord(
                ppib,
                ifmp,
                pgnoFDP,
                dataRec,
                pfucb,
                pttmapRecords,
                prepairtable,
                popts ) );
        }
        Call( err );
        err = ErrDIRNext( pfucb, fDIRNull );
    }

    if ( JET_errNoCurrentRecord == err
        || JET_errRecordNotFound == err )
    {
        err = JET_errSuccess;
    }

HandleError:
    if ( pfucbNil != pfucb )
    {
        Assert( NULL != pfucb->pvWorkBuf );

        if ( prepairtable->fidFixedLast > pfucb->u.pfcb->Ptdb()->FidFixedLast()
            || prepairtable->fidVarLast > pfucb->u.pfcb->Ptdb()->FidVarLast()
            || prepairtable->fidTaggedLast > pfucb->u.pfcb->Ptdb()->FidTaggedLast() )
        {
            err = ErrREPAIRInsertDummyRecordsToCatalog( ppib, ifmp, prepairtable, popts );
        }

        RECIFreeCopyBuffer( pfucb );
        DIRClose( pfucb );
    }

    return err;
}


LOCAL ERR ErrREPAIRFixLVRefcounts(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgnoLV,
    TTMAP * const pttmapRecords,
    TTMAP * const pttmapLVTree,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts )
{
    ERR err         = JET_errSuccess;
    FUCB * pfucb    = pfucbNil;

    (*popts->pcprintfVerbose)( "fixing long value refcounts\r\n" );

    err = pttmapRecords->ErrMoveFirst();
    if ( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    Call( pttmapLVTree->ErrMoveFirst() );

    Call( ErrDIROpen( ppib, pgnoLV, ifmp, &pfucb ) );

    Assert( pfucb->u.pfcb->FTypeNull() || pfucb->u.pfcb->FTypeLV() );
    pfucb->u.pfcb->SetTypeLV();

    for( ;; )
    {
        _LID64  lid;
        _LID64  lidRecord;
        ULONG   ulRefcount;
        ULONG   ulRefcountRecord;

        Call( pttmapLVTree->ErrGetCurrentKeyValue( &lid, &ulRefcount ) );
        Call( pttmapRecords->ErrGetCurrentKeyValue( &lidRecord, &ulRefcountRecord ) );
        if ( lid == lidRecord )
        {
            if ( ulRefcount != ulRefcountRecord )
            {
                (*popts->pcprintfVerbose)(
                    "lid %I64u has incorrect refcount (refcount is %d, should be %d). correcting\r\n",
                    lid,
                    ulRefcount,
                    ulRefcountRecord
                    );
                Call( ErrREPAIRUpdateLVRefcount( pfucb, lid, ulRefcount, ulRefcountRecord ) );
            }
            err = pttmapRecords->ErrMoveNext();
            if ( JET_errNoCurrentRecord == err )
            {
                err = JET_errSuccess;
                break;
            }
        }
        Call( pttmapLVTree->ErrMoveNext() );
    }

HandleError:
    if ( pfucbNil != pfucb )
    {
        DIRClose( pfucb );
    }
    return err;
}


LOCAL ERR ErrREPAIRFixupTable(
    PIB * const ppib,
    const IFMP ifmp,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts )
{
    ERR err         = JET_errSuccess;

    const PGNO  pgnoFDP     = prepairtable->pgnoFDP;
    const PGNO  pgnoLV      = prepairtable->pgnoLV;

    TTMAP ttmapLVTree( ppib );
    TTMAP ttmapRecords( ppib );

    Call( ttmapLVTree.ErrInit( PinstFromPpib( ppib ) ) );
    Call( ttmapRecords.ErrInit( PinstFromPpib( ppib ) ) );

    if ( pgnoNull != pgnoLV )
    {
        Call( ErrREPAIRFixLVs( ppib, ifmp, pgnoLV, &ttmapLVTree, fFalse, prepairtable, popts ) );
    }

    Call( ErrREPAIRFixRecords(
            ppib,
            ifmp,
            pgnoFDP,
            pgnoLV,
            &ttmapRecords,
            &ttmapLVTree,
            prepairtable,
            popts ) );
    Call( ErrREPAIRFixLVRefcounts( ppib, ifmp, pgnoLV, &ttmapRecords, &ttmapLVTree, prepairtable, popts ) );

HandleError:
    return err;
}


LOCAL BOOL FREPAIRIdxsegIsUserDefinedColumn(
    const FCB * const pfcbIndex,
    const IDXSEG idxseg )
{
    const COLUMNID      columnid    = idxseg.Columnid();
    const TDB * const   ptdb        = pfcbIndex->PfcbTable()->Ptdb();
    const FIELD * const pfield      = ptdb->Pfield( columnid );

    return FFIELDUserDefinedDefault( pfield->ffield );
}


LOCAL BOOL FREAPIRIdxsegHasCallbacks(
    const FCB * const pfcbIndex,
    const IDXSEG * rgidxseg,
    const INT cidxseg )
{
    for( INT iidxseg = 0; iidxseg < cidxseg; ++iidxseg )
    {
        if ( FREPAIRIdxsegIsUserDefinedColumn( pfcbIndex, rgidxseg[iidxseg] ) )
        {
            return fTrue;
        }
    }
    return fFalse;
}


LOCAL BOOL FREAPIRIndexHasCallbacks(
    const FCB * const pfcbIndex )
{
    const TDB * const ptdb = pfcbIndex->PfcbTable()->Ptdb();
    const IDB * const pidb = pfcbIndex->Pidb();
    Assert( ptdbNil != ptdb );
    Assert( pidbNil != pidb );

    if ( FREAPIRIdxsegHasCallbacks( pfcbIndex, PidxsegIDBGetIdxSeg( pidb, ptdb ), pidb->Cidxseg() ) )
    {
        return fTrue;
    }

    if ( FREAPIRIdxsegHasCallbacks( pfcbIndex, PidxsegIDBGetIdxSegConditional( pidb, ptdb ), pidb->CidxsegConditional() ) )
    {
        return fTrue;
    }

    return fFalse;
}


LOCAL ERR ErrREPAIRReopenTable(
        PIB * const ppib,
        const IFMP ifmp,
        const CHAR * const szTableName,
        const PGNO pgnoFDP,
        const OBJID objidFDP,
        FUCB ** ppfucb,
        const REPAIROPTS * const popts )
{
    ERR err = JET_errSuccess;

    FCB * const pfcbTable = (*ppfucb)->u.pfcb;

    if ( pfucbNil != (*ppfucb)->pfucbLV )
    {
        DIRClose( (*ppfucb)->pfucbLV );
        (*ppfucb)->pfucbLV = pfucbNil;
    }

    DIRClose( *ppfucb );
    *ppfucb = pfucbNil;

    pfcbTable->PrepareForPurge();
    pfcbTable->Purge();

    FDPINFO fdpinfo;
    fdpinfo.pgnoFDP = pgnoFDP;
    fdpinfo.objidFDP = objidFDP;

    Call( ErrFILEOpenTable( ppib, ifmp, ppfucb, szTableName, NO_GRBIT, &fdpinfo ) );
    FUCBSetIndex( *ppfucb );

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRDeleteIndexesWithCallbacks(
        PIB * const ppib,
        const IFMP ifmp,
        const PGNO pgnoFDPTable,
        FCB * const pfcbTable,
        const REPAIROPTS * const popts )
{
    ERR         err         = JET_errSuccess;
    FCB     *   pfcbIndex   = NULL;

    const OBJID objidTable  = pfcbTable->ObjidFDP();

    for( pfcbIndex = pfcbTable->PfcbNextIndex();
        pfcbNil != pfcbIndex;
        pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        if ( pfcbIndex->FPrimaryIndex() )
        {
            continue;
        }

        if ( FREAPIRIndexHasCallbacks( pfcbIndex ) )
        {
            const CHAR * const szIndexName  = pfcbTable->Ptdb()->SzIndexName( pfcbIndex->Pidb()->ItagIndexName(), pfcbIndex->FDerivedIndex() );

            (*popts->pcprintfVerbose)( "index %s (objid: %d, pgnoFDP: %d) contains user-defined-default columns and will be deleted\r\n",
                szIndexName,
                pfcbIndex->ObjidFDP(),
                pfcbIndex->PgnoFDP() );

            PGNO pgnoIndexFDP = pgnoNull;
            Call( ErrCATDeleteTableIndex( ppib, ifmp, objidTable, szIndexName, &pgnoIndexFDP ) );

            if ( pfcbIndex->PgnoFDP() != pgnoIndexFDP )
            {
                (*popts->pcprintfVerbose)( "index %s (objid: %d, pgnoFDP: %d) ErrCATDeleteTableIndex says the pgnoFDP was %d!\r\n",
                    szIndexName,
                    pfcbIndex->ObjidFDP(),
                    pfcbIndex->PgnoFDP(),
                    pgnoIndexFDP );
            }
        }
    }

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRCreateEmptyIndexes(
        PIB * const ppib,
        const IFMP ifmp,
        const PGNO pgnoFDPTable,
        FCB * const pfcbTable,
        const REPAIROPTS * const popts )
{
    ERR         err         = JET_errSuccess;
    FCB     *   pfcbIndex   = NULL;

    for( pfcbIndex = pfcbTable->PfcbNextIndex();
        pfcbNil != pfcbIndex;
        pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        const BOOL fNonUnique   = pfcbIndex->FNonUnique();
        const PGNO pgnoFDPOld   = pfcbIndex->PgnoFDP();
        PGNO pgnoFDPNew         = pgnoNull;

        if ( pgnoFDPMSO_NameIndex == pgnoFDPOld
            || pgnoFDPMSO_RootObjectIndex == pgnoFDPOld )
        {
            pgnoFDPNew = pgnoFDPOld;
        }

        Call( ErrREPAIRCreateEmptyFDP(
            ppib,
            ifmp,
            pfcbIndex->ObjidFDP(),
            pgnoFDPTable,
            &pgnoFDPNew,
            CPAGE::fPageIndex,
            !fNonUnique,
            popts ) );

        if ( pgnoFDPMSO_NameIndex != pgnoFDPOld
            && pgnoFDPMSO_RootObjectIndex != pgnoFDPOld )
        {
            Call( ErrCATChangePgnoFDP(
                    ppib,
                    ifmp,
                    pfcbTable->ObjidFDP(),
                    pfcbIndex->ObjidFDP(),
                    sysobjIndex,
                    pgnoFDPNew ) );
        }
    }

HandleError:
    return err;
}


LOCAL ERR ErrREPAIRBuildAllIndexes(
    PIB * const ppib,
    const IFMP ifmp,
    FUCB ** const ppfucb,
    REPAIRTABLE * const prepairtable,
    const REPAIROPTS * const popts )
{
    ERR     err         = JET_errSuccess;


    Call( ErrREPAIRDeleteIndexesWithCallbacks( ppib, ifmp, prepairtable->pgnoFDP, (*ppfucb)->u.pfcb, popts ) );
    Call( ErrREPAIRReopenTable( ppib, ifmp, prepairtable->szTableName, prepairtable->pgnoFDP, prepairtable->objidFDP, ppfucb, popts ) );


    Call( ErrREPAIRCreateEmptyIndexes( ppib, ifmp, prepairtable->pgnoFDP, (*ppfucb)->u.pfcb, popts ) );
    Call( ErrREPAIRReopenTable( ppib, ifmp, prepairtable->szTableName, prepairtable->pgnoFDP, prepairtable->objidFDP, ppfucb, popts ) );


    if ( pfcbNil != (*ppfucb)->u.pfcb->PfcbNextIndex() )
    {
#ifdef PARALLEL_BATCH_INDEX_BUILD
        ULONG   cIndexes    = 0;
        for ( FCB * pfcbT = (*ppfucb)->u.pfcb->PfcbNextIndex(); pfcbNil != pfcbT; pfcbT = pfcbT->PfcbNextIndex() )
        {
            cIndexes++;
        }
#else
        const ULONG cIndexes    = cFILEIndexBatchSizeDefault;
#endif

        Call( ErrFILEBuildAllIndexes( ppib, *ppfucb, (*ppfucb)->u.pfcb->PfcbNextIndex(), NULL, cIndexes ) );
    }

HandleError:
    if ( pfucbNil != *ppfucb )
    {
        DIRClose( *ppfucb );
        *ppfucb = pfucbNil;
    }

    return err;
}


RECCHECK::RECCHECK()
{
}


RECCHECK::~RECCHECK()
{
}


RECCHECKTABLE::RECCHECKTABLE(
    const OBJID objid,
    FUCB * const pfucb,
    const FIDLASTINTDB fidLastInTDB,
    TTMAP * const pttmapLVRefcounts,
    const REPAIROPTS * const popts ) :
    m_objid( objid ),
    m_pfucb( pfucb ),
    m_fidLastInTDB( fidLastInTDB ),
    m_pttmapLVRefcounts( pttmapLVRefcounts ),
    m_popts( popts )
{
}


RECCHECKTABLE::~RECCHECKTABLE()
{
}


ERR RECCHECKTABLE::ErrCheckRecord_(
    const KEYDATAFLAGS& kdf,
    const BOOL fCheckLV )
{
    ERR err = JET_errSuccess;


    Call( ErrCheckREC_( kdf ) );


    Call( ErrCheckFixedFields_( kdf ) );


    Call( ErrCheckVariableFields_( kdf ) );


    Call( ErrCheckTaggedFields_( kdf ) );

    if ( fCheckLV)
    {

        Call( ErrCheckLVFields_( kdf ) );
    }

    if ( m_pfucb )
    {

        Call( ErrCheckPrimaryKey_( kdf ) );
    }

HandleError:
    return err;
}


ERR RECCHECKTABLE::ErrCheckREC_( const KEYDATAFLAGS& kdf )
{
    ERR err = JET_errSuccess;

    const REC * const prec = reinterpret_cast<REC *>( kdf.data.Pv() );
    const BYTE * const pbRecMax = reinterpret_cast<BYTE *>( kdf.data.Pv() ) + kdf.data.Cb();


    if ( prec->PbFixedNullBitMap() > pbRecMax )
    {
        (*m_popts->pcprintfError)( "Record corrupted: PbFixedNullBitMap is past the end of the record\r\n" );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( prec->PbVarData() > pbRecMax )
    {
        (*m_popts->pcprintfError)( "Record corrupted: PbVarData is past the end of the record\r\n" );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if (    prec->PbTaggedData() > pbRecMax )
    {
        (*m_popts->pcprintfError)( "Record corrupted: PbTaggedData is past the end of the record\r\n" );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

HandleError:
    return err;
}

ERR RECCHECKTABLE::ErrCheckFixedFields_( const KEYDATAFLAGS& kdf )
{
    ERR err = JET_errSuccess;

    const REC * const prec = reinterpret_cast<REC *>( kdf.data.Pv() );
    const FID fidFixedLast  = prec->FidFixedLastInRec();

    if ( fidFixedLast > m_fidLastInTDB.fidFixedLastInTDB )
    {
        (*m_popts->pcprintfError)(
            "Record corrupted: last fixed column ID not in catalog (fid: %d, catalog last: %d)\r\n",
                fidFixedLast, m_fidLastInTDB.fidFixedLastInTDB );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }
HandleError:
    return err;
}


ERR RECCHECKTABLE::ErrCheckVariableFields_( const KEYDATAFLAGS& kdf )
{
    ERR err = JET_errSuccess;

    const REC * const prec = reinterpret_cast<REC *>( kdf.data.Pv() );
    const BYTE * const pbRecMax = reinterpret_cast<BYTE *>( kdf.data.Pv() ) + kdf.data.Cb();

    const FID fidVariableFirst = fidVarLeast ;
    const FID fidVariableLast  = prec->FidVarLastInRec();
    const UnalignedLittleEndian<REC::VAROFFSET> * const pibVarOffs      = prec->PibVarOffsets();

    FID fid;

    REC::VAROFFSET ibStartOfColumnPrev = 0;
    REC::VAROFFSET ibEndOfColumnPrev = 0;


    for( fid = fidVariableFirst; fid <= fidVariableLast; ++fid )
    {
        const UINT              ifid            = fid - fidVarLeast;
        const REC::VAROFFSET    ibStartOfColumn = prec->IbVarOffsetStart( fid );
        const REC::VAROFFSET    ibEndOfColumn   = IbVarOffset( pibVarOffs[ifid] );

        if ( ibEndOfColumn < ibStartOfColumn )
        {
            (*m_popts->pcprintfError)(
                    "Record corrupted: variable field offsets not increasing (fid: %d, start: %d, end: %d)\r\n",
                    fid, ibStartOfColumn, ibEndOfColumn );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        if ( ibStartOfColumn < ibStartOfColumnPrev )
        {
            (*m_popts->pcprintfError)(
                    "Record corrupted: variable field offsets not increasing (fid: %d, start: %d, startPrev: %d)\r\n",
                    fid, ibStartOfColumn, ibStartOfColumnPrev );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        ibStartOfColumnPrev = ibStartOfColumn;

        if ( ibEndOfColumn < ibEndOfColumnPrev )
        {
            (*m_popts->pcprintfError)(
                    "Record corrupted: variable field offsets not increasing (fid: %d, end: %d, endPrev: %d)\r\n",
                    fid, ibEndOfColumn, ibEndOfColumnPrev );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        ibEndOfColumnPrev = ibEndOfColumn;

        if ( FVarNullBit( pibVarOffs[ifid] ) )
        {
            if ( ibStartOfColumn != ibEndOfColumn )
            {
                const INT cbColumn              = ibEndOfColumn - ibStartOfColumn;
                (*m_popts->pcprintfError)(
                    "Record corrupted: NULL variable field not zero length (fid: %d, length: %d)\r\n", fid, cbColumn );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
        }
        else
        {
            const BYTE * const pbColumn     = prec->PbVarData() + ibStartOfColumn;
            const INT cbColumn              = ibEndOfColumn - ibStartOfColumn;
            const BYTE * const pbColumnEnd  = pbColumn + cbColumn;

            if ( pbColumn > pbRecMax )
            {
                (*m_popts->pcprintfError)(
                    "Record corrupted: variable field is not in record (fid: %d, length: %d)\r\n", fid, cbColumn );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }

            if ( pbColumnEnd > pbRecMax )
            {
                (*m_popts->pcprintfError)(
                    "Record corrupted: variable field is too long (fid: %d, length: %d)\r\n", fid, cbColumn );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
        }
    }

    if ( fidVariableLast > m_fidLastInTDB.fidVarLastInTDB )
    {
        (*m_popts->pcprintfError)(
            "Record corrupted: last variable column ID not in catalog (fid: %d, catalog last: %d)\r\n",
                fidVariableLast, m_fidLastInTDB.fidVarLastInTDB );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }


HandleError:
    return err;
}


ERR RECCHECKTABLE::ErrCheckSeparatedLV_(
                    const KEYDATAFLAGS& kdf,
                    const COLUMNID columnid,
                    const ULONG itagSequence,
                    const LvId lid )
{
    ERR err = JET_errSuccess;

    if ( m_pttmapLVRefcounts )
    {
#ifdef SYNC_DEADLOCK_DETECTION
        COwner* const pownerSaved = Pcls()->pownerLockHead;
        Pcls()->pownerLockHead = NULL;
#endif

        Call( m_pttmapLVRefcounts->ErrIncrementValue( lid ) );

#ifdef SYNC_DEADLOCK_DETECTION
        Pcls()->pownerLockHead = pownerSaved;
#endif
    }
    else
    {
        (*m_popts->pcprintfError)( "separated LV was not expected (columnid: %d, itag: %d, lid: 0x%I64x)\r\n",
            columnid, itagSequence, (_LID64)lid );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

HandleError:
    return err;
}

ERR RECCHECKTABLE::ErrCheckIntrinsicLV_(
        const KEYDATAFLAGS& kdf,
        const COLUMNID columnid,
        const ULONG itagSequence,
        const DATA& dataLV )
{
    return JET_errSuccess;
}


ERR RECCHECKTABLE::ErrCheckPrimaryKey_( const KEYDATAFLAGS& kdf )
{
    Assert( m_pfucb );
    if ( pfcbNil == m_pfucb->u.pfcb
        || pidbNil == m_pfucb->u.pfcb->Pidb() )
    {
        return JET_errSuccess;
    }

    ERR err = JET_errSuccess;

    IDB * const pidb = m_pfucb->u.pfcb->Pidb();

    BYTE rgbKey[cbKeyAlloc];
    KEY key;
    key.Nullify();
    key.suffix.SetPv( rgbKey );
    key.suffix.SetCb( sizeof( rgbKey ) );

    ULONG iidxsegT;

    Call( ErrRECIRetrieveKey(
                m_pfucb,
                pidb,
                const_cast< DATA& >( kdf.data ),
                &key,
                rgitagBaseKey,
                0,
                fFalse,
                prceNil,
                &iidxsegT ) );

    switch ( err )
    {
        case wrnFLDNullKey:
        case wrnFLDNullFirstSeg:
        case wrnFLDNullSeg:
            break;
        default:
            CallS( err );
    }

    err = JET_errSuccess;

    if ( 0 != CmpKey( key, kdf.key ) )
    {
        (*m_popts->pcprintfError)( "constructed primary key differs from real primary key\r\n" );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

HandleError:
    return err;
}


ERR RECCHECKTABLE::ErrCheckTaggedFields_( const KEYDATAFLAGS& kdf )
{
    ERR         err             = JET_errSuccess;

    if ( !TAGFIELDS::FIsValidTagfields( g_cbPage, kdf.data, m_popts->pcprintfError ) )
    {
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }


    FID     fidTaggedLast       = fidTaggedLeast - 1;
    VOID *  pvWorkBuf;

    BFAlloc( bfasTemporary, &pvWorkBuf );

    BYTE *  pb = (BYTE *)pvWorkBuf;
    UtilMemCpy( pb, kdf.data.Pv(), kdf.data.Cb() );

    DATA dataRec;
    dataRec.SetPv( pb );
    dataRec.SetCb( kdf.data.Cb() );

    TAGFIELDS_ITERATOR tagfieldsIterator( dataRec );
    tagfieldsIterator.MoveBeforeFirst();
    while ( JET_errSuccess == ( err = tagfieldsIterator.ErrMoveNext() ) )
    {
        if ( !tagfieldsIterator.FDerived() )
        {
            fidTaggedLast = max( tagfieldsIterator.Fid(), fidTaggedLast );
        }

        if ( tagfieldsIterator.FNull() )
        {
            continue;
        }
        else
        {
            tagfieldsIterator.TagfldIterator().MoveBeforeFirst();
        }
    }

    if ( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }

    BFFree( pvWorkBuf );

    if ( fidTaggedLast > m_fidLastInTDB.fidTaggedLastInTDB )
    {
        (*m_popts->pcprintfError)(
            "Record corrupted: last tagged column ID not in catalog (fid: %d, catalog last: %d)\r\n",
                fidTaggedLast, m_fidLastInTDB.fidTaggedLastInTDB );
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

    return err;
}


ERR RECCHECKTABLE::ErrCheckLVFields_( const KEYDATAFLAGS& kdf )
{
    Assert( TAGFIELDS::FIsValidTagfields( g_cbPage, kdf.data, m_popts->pcprintfError ) );

    TAGFIELDS   tagfields( kdf.data );
    return tagfields.ErrCheckLongValues( kdf, this );
}


ERR RECCHECKTABLE::operator()( const KEYDATAFLAGS& kdf, const PGNO pgno )
{
    return ErrCheckRecord_( kdf );
}


RECCHECKMACRO::RECCHECKMACRO() :
    m_creccheck( 0 )
{
    memset( m_rgpreccheck, 0, sizeof( m_rgpreccheck ) );
}


RECCHECKMACRO::~RECCHECKMACRO()
{
}


ERR RECCHECKMACRO::operator()( const KEYDATAFLAGS& kdf, const PGNO pgno )
{
    ERR err = JET_errSuccess;

    INT ipreccheck;
    for( ipreccheck = 0; ipreccheck < m_creccheck; ipreccheck++ )
    {
        Call( (*m_rgpreccheck[ipreccheck])( kdf, pgno ) );
    }

HandleError:
    return err;
}


VOID RECCHECKMACRO::Add( RECCHECK * const preccheck )
{
    m_rgpreccheck[m_creccheck++] = preccheck;
    Assert( m_creccheck < ( sizeof( m_rgpreccheck ) / sizeof( preccheck ) ) );
}


RECCHECKSPACE::RECCHECKSPACE( PIB * const ppib, const REPAIROPTS * const popts ) :
    m_ppib( ppib ),
    m_popts( popts ),
    m_sppLast( spp::AvailExtLegacyGeneralPool ),
    m_pgnoLast( pgnoNull ),
    m_cpgLast( 0 ),
    m_cpgSeen( 0 ),
    m_iRun( 0 )
{
}


RECCHECKSPACE::~RECCHECKSPACE()
{
}


ERR RECCHECKSPACE::operator()( const KEYDATAFLAGS& kdf, const PGNO )
{
    ERR err = JET_errSuccess;

    PGNO pgno = pgnoNull;
    CPG cpg = 0;
    SpacePool sppPool = spp::MinPool;

    err = ErrSPIREPAIRValidateSpaceNode( &kdf, &pgno, &cpg, &sppPool );
    if ( JET_errSPOwnExtCorrupted == err )
    {
        (*m_popts->pcprintfError)( "space tree key node is corrrupted.  probably AE tree.  size (%d bytes)\r\n", kdf.key.Cb() );
    }
    else if ( JET_errSPAvailExtCorrupted == err )
    {
        (*m_popts->pcprintfError)( "space tree key node is corrrupted.  probably AE tree.  size (%d bytes)\r\n", kdf.key.Cb() );
    }
    else if ( err != JET_errSuccess )
    {
        (*m_popts->pcprintfError)( "space tree key node had unknown corrruption.  size (%d bytes)\r\n", kdf.key.Cb() );
    }
    Call( err );

    if ( FNDVersion( kdf ) )
    {
        (*m_popts->pcprintfError)( "versioned node in space tree\r\n" );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( sppPool == m_sppLast )
    {
        if ( pgno <= m_pgnoLast )
        {
            (*m_popts->pcprintfError)( "space tree corruption (previous pgno %d, current pgno %d)\r\n", m_pgnoLast, pgno );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        if ( (cpg < 0) || ( (ULONG) cpg > ( pgno - m_pgnoLast ) ) || (cpg > lMax-m_cpgSeen) )
        {
            (*m_popts->pcprintfError)( "space tree overlap (previous extent was %d:%d, current extent is %d:%d)\r\n",
                                        m_pgnoLast, m_cpgLast, pgno, cpg );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }
    else
    {
    }

    m_sppLast = sppPool;
    m_pgnoLast = pgno;
    m_cpgLast = cpg;
    m_cpgSeen += cpg;
    ++m_iRun;

    err = JET_errSuccess;

HandleError:

    Assert( JET_errSuccess == err ||
                JET_errSPOwnExtCorrupted == err ||
                JET_errSPAvailExtCorrupted == err ||
                JET_errDatabaseCorrupted == err );

    if ( JET_errSuccess != err &&
            JET_errDatabaseCorrupted != err )
    {
        err = ErrERRCheck( JET_errDatabaseCorrupted );
    }
    return err;
}

RECCHECKSPACEEXTENT::RECCHECKSPACEEXTENT(
            PIB * const ppib,
            TTARRAY * const pttarrayOE,
            TTARRAY * const pttarrayAE,
            PgnoCollection * const ppgnocollShelved,
            const OBJID objid,
            const OBJID objidParent,
            const REPAIROPTS * const popts ) :
    RECCHECKSPACE( ppib, popts ),
    m_pttarrayOE( pttarrayOE ),
    m_pttarrayAE( pttarrayAE ),
    m_ppgnocollShelved( ppgnocollShelved ),
    m_objid( objid ),
    m_objidParent( objidParent )
{
    Assert( ( objid == objidSystemRoot ) == ( objidParent == objidNil ) );
}


RECCHECKSPACEEXTENT::~RECCHECKSPACEEXTENT()
{
}


ERR RECCHECKSPACEEXTENT::operator()( const KEYDATAFLAGS& kdf, const PGNO pgno )
{
    ERR err = JET_errSuccess;

    CallR( RECCHECKSPACE::operator()( kdf, pgno ) );

    return err;
}


RECCHECKSPACEOE::RECCHECKSPACEOE(
    PIB * const ppib,
    TTARRAY * const pttarrayOE,
    TTARRAY * const pttarrayAE,
    PgnoCollection * const ppgnocollShelved,
    const OBJID objid,
    const OBJID objidParent,
    const REPAIROPTS * const popts ) :
    RECCHECKSPACEEXTENT( ppib, pttarrayOE, pttarrayAE, ppgnocollShelved, objid, objidParent, popts )
{
}


RECCHECKSPACEOE::~RECCHECKSPACEOE()
{
}


ERR RECCHECKSPACEOE::operator()( const KEYDATAFLAGS& kdf, const PGNO pgno )
{
    ERR err = JET_errSuccess;

    CallR( RECCHECKSPACEEXTENT::operator()( kdf, pgno ) );

    Assert( sizeof( PGNO ) == kdf.key.Cb() );
    PGNO pgnoLast;
    LongFromKey( &pgnoLast, kdf.key );

    Assert( sizeof( CPG ) == kdf.data.Cb() );
    const CPG cpgRun = *(UnalignedLittleEndian< CPG > *)(kdf.data.Pv());

    const BOOL fOddRun = ( m_iRun % 2 );

    CallR( ErrREPAIRInsertOERunIntoTT(
        m_ppib,
        pgnoLast,
        cpgRun,
        m_objid,
        m_objidParent,
        m_pttarrayOE,
        m_pttarrayAE,
        m_ppgnocollShelved,
        fOddRun,
        m_popts ) );

    return err;
}


RECCHECKSPACEAE::RECCHECKSPACEAE(
    PIB * const ppib,
    TTARRAY * const pttarrayOE,
    TTARRAY * const pttarrayAE,
    PgnoCollection * const ppgnocollShelved,
    const OBJID objid,
    const OBJID objidParent,
    const REPAIROPTS * const popts ) :
    RECCHECKSPACEEXTENT( ppib, pttarrayOE, pttarrayAE, ppgnocollShelved, objid, objidParent, popts )
{
}


RECCHECKSPACEAE::~RECCHECKSPACEAE()
{
}


ERR RECCHECKSPACEAE::operator()( const KEYDATAFLAGS& kdf, const PGNO pgno )
{
    ERR err = JET_errSuccess;

    CallR( RECCHECKSPACEEXTENT::operator()( kdf, pgno ) );

    PGNO pgnoLast;
    CPG cpgRun;
    SpacePool sppPool;
    if ( ErrSPIREPAIRValidateSpaceNode( &kdf, &pgnoLast, &cpgRun, &sppPool ) < JET_errSuccess )
    {
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

    const BOOL fOddRun = ( m_iRun % 2 );

    CallR( ErrREPAIRInsertAERunIntoTT(
        m_ppib,
        pgnoLast,
        cpgRun,
        sppPool,
        m_objid,
        m_objidParent,
        m_pttarrayOE,
        m_pttarrayAE,
        m_ppgnocollShelved,
        fOddRun,
        m_popts ) );

    return err;
}


REPAIROPTS::REPAIROPTS() :
    crit( CLockBasicInfo( CSyncBasicInfo( szREPAIROpts ), rankREPAIROpts, 0 ) )
{
}


REPAIROPTS::~REPAIROPTS()
{
}


