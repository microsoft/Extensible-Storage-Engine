// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.



ERR ErrDBUTLRepair( JET_SESID sesid, const JET_DBUTIL_W *pdbutil, CPRINTF* const pcprintf );


ERR ErrREPAIRCheckLV(
    FUCB * const pfucb,
    LvId * const plid,
    ULONG * const pulRefcount,
    ULONG * const pulSize,
    BOOL * const pfLVHasRoot,
    BOOL * const pfLVComplete,
    BOOL * const pfLVPartiallyScrubbed,
    BOOL * const pfDone );
ERR ErrREPAIRCreateLVRoot( FUCB * const pfucb, const LvId lid, const ULONG ulRefcount, const ULONG ulSize );
ERR ErrREPAIRDeleteLV( FUCB * pfucb, LvId lid );
ERR ErrREPAIRNextLV( FUCB * pfucb, LvId lidCurr, BOOL * pfDone );
ERR ErrREPAIRUpdateLVRefcount(
    FUCB        * const pfucbLV,
    const LvId  lid,
    const ULONG ulRefcountOld,
    const ULONG ulRefcountNew );

#define wrnLVRefcountsTooHigh 1206

class OBJIDLIST
{
    public:
        OBJIDLIST( );
        ~OBJIDLIST();

        ERR  ErrAddObjid( const OBJID objid );
        BOOL FObjidPresent( const OBJID objid ) const;
        VOID Sort();

    private:
        INT     m_cobjid;
        INT     m_cobjidMax;
        OBJID   *m_rgobjid;
        BOOL    m_fSorted;
};


struct REPAIROPTS
{
    REPAIROPTS();
    ~REPAIROPTS();
    
    CPRINTFINDENT * pcprintf;
    CPRINTFINDENT * pcprintfVerbose;
    CPRINTFINDENT * pcprintfError;
    CPRINTFINDENT * pcprintfWarning;
    CPRINTFINDENT * pcprintfDebug;
    CPRINTFINDENT * pcprintfStats;

    JET_GRBIT   grbit;

    mutable CCriticalSection crit;
    JET_PFNSTATUS   pfnStatus;
    JET_SNPROG      *psnprog;
};


struct FIDLASTINTDB
{
    FID         fidFixedLastInTDB;
    FID         fidVarLastInTDB;
    FID         fidTaggedLastInTDB;
};


class RECCHECK
{
    public:
        RECCHECK();
        virtual ~RECCHECK();

        virtual ERR operator()( const KEYDATAFLAGS& kdf, const PGNO pgno ) = 0;

    private:
        RECCHECK( const RECCHECK& );
        RECCHECK& operator=( const RECCHECK& );
};


class RECCHECKTABLE : public RECCHECK
{
    public:
        RECCHECKTABLE(
            const OBJID objid,
            FUCB * const pfucb,
            const FIDLASTINTDB fidLastInTDB,
            TTMAP * const pttmapLVRefcounts,
            const REPAIROPTS * const popts );
        ~RECCHECKTABLE();

        ERR operator()( const KEYDATAFLAGS& kdf, const PGNO pgno );

    private:
        ERR ErrCheckRecord_( const KEYDATAFLAGS& kdf, const BOOL fCheckLV = fTrue );
        ERR ErrCheckREC_( const KEYDATAFLAGS& kdf );
        ERR ErrCheckFixedFields_( const KEYDATAFLAGS& kdf );
        ERR ErrCheckVariableFields_( const KEYDATAFLAGS& kdf );
        ERR ErrCheckTaggedFields_( const KEYDATAFLAGS& kdf );
        ERR ErrCheckLVFields_( const KEYDATAFLAGS& kdf );
        ERR ErrCheckIntrinsicLV_( const KEYDATAFLAGS& kdf, const COLUMNID columnid, const ULONG itagSequence, const DATA& dataLV );
        ERR ErrCheckSeparatedLV_( const KEYDATAFLAGS& kdf, const COLUMNID columnid, const ULONG itagSequence, const LvId lid );
        ERR ErrCheckPrimaryKey_( const KEYDATAFLAGS& kdf );

    public:
        ERR ErrCheckRecord( const KEYDATAFLAGS& kdf )
        {
            return ErrCheckRecord_( kdf, fFalse );
        }
        ERR ErrCheckLV(
            const KEYDATAFLAGS& kdf,
            const COLUMNID      columnid,
            const ULONG         itagSequence,
            const DATA&         dataLV,
            const BOOL          fSeparatedLV )
        {
            if ( fSeparatedLV )
            {
                return ErrCheckSeparatedLV_( kdf, columnid, itagSequence, LidOfSeparatedLV( dataLV ) );
            }
            else
            {
                return JET_errSuccess;
            }
        }
        
    private:
        const OBJID m_objid;
        FUCB * const m_pfucb;
        const FIDLASTINTDB m_fidLastInTDB;
        TTMAP * const m_pttmapLVRefcounts;
        const REPAIROPTS * const m_popts;
};

class RECCHECKLV : public RECCHECK
{
    public:
        RECCHECKLV( TTMAP& ttmap, const REPAIROPTS * popts, LONG cbLVChunkMost );
        ~RECCHECKLV();

        ERR operator()( const KEYDATAFLAGS& kdf, const PGNO pgno );

        ERR ErrTerm();

    private:
        TTMAP&  m_ttmap;
        const REPAIROPTS * const m_popts;

        ULONG   m_ulSizeCurr;
        ULONG   m_ulReferenceCurr;
        ULONG   m_ulSizeSeen;
        LvId    m_lidCurr;
        LONG    m_cbLVChunkMost;
        BOOL    m_fEncrypted;
};


class RECCHECKLVSTATS : public RECCHECK
{
    public:
        RECCHECKLVSTATS( LVSTATS * plvstats );
        ~RECCHECKLVSTATS();

        ERR operator()( const KEYDATAFLAGS& kdf, const PGNO pgno );

    private:
        LvId            m_lidLastRoot;
        PGNO            m_pgnoLastRoot;
        LVSTATS * const m_plvstats;
};

