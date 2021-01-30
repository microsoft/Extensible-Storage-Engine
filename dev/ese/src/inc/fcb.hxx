// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef FCB_INCLUDED
#error FCB.HXX already included
#endif
#define FCB_INCLUDED


struct RECDANGLING
{
    DATA            data;
    RECDANGLING *   precdanglingNext;
};


PERSISTED
struct EXTENTINFO
{
    UnalignedLittleEndian< PGNO >       pgnoLastInExtent;
    UnalignedLittleEndian< CPG >        cpgExtent;

    EXTENTINFO::EXTENTINFO()
    {
        Reset();
    }

    void Reset()
    {
        pgnoLastInExtent = pgnoNull;
        cpgExtent = -1;
        Assert( !FValid() );
    }

    PGNO PgnoFirst() const
    {
        Assert( FValid() );
        return pgnoLastInExtent - cpgExtent + 1;
    }

    PGNO PgnoLast() const
    {
        Assert( FValid() );
        return pgnoLastInExtent;
    }

    PGNO CpgExtent() const
    {
        Assert( FValid() );
        return cpgExtent;
    }

    BOOL FContainsPgno( const PGNO pgno ) const
    {
        Assert( FValid() );
        Assert( pgno != pgnoNull );
        return ( pgno >= PgnoFirst() ) && ( pgno <= PgnoLast() );
    }

    BOOL FValid() const
    {
        return ( ( cpgExtent == 0 ) || ( ( cpgExtent > 0 ) && ( pgnoLastInExtent != pgnoNull ) ) );
    }
};

INLINE VOID SPCheckPgnoAllocTrap( __in const PGNO pgnoAlloc, __in const CPG cpgAlloc = 1 );

PERSISTED
class SPLIT_BUFFER
{
    public:
        SPLIT_BUFFER()              {};
        ~SPLIT_BUFFER()             {};

        CPG CpgBuffer1() const                              { return m_rgextentinfo[0].CpgExtent(); }
        CPG CpgBuffer2() const                              { return m_rgextentinfo[1].CpgExtent(); }

        BOOL FBuffer1ContainsPgno( const PGNO pgno ) const  { return m_rgextentinfo[0].FContainsPgno( pgno ); }
        BOOL FBuffer2ContainsPgno( const PGNO pgno ) const  { return m_rgextentinfo[1].FContainsPgno( pgno ); }

        PGNO PgnoFirstBuffer1() const                       { return m_rgextentinfo[0].PgnoFirst(); }
        PGNO PgnoFirstBuffer2() const                       { return m_rgextentinfo[1].PgnoFirst(); }

        PGNO PgnoLastBuffer1() const                        { return m_rgextentinfo[0].PgnoLast(); }
        PGNO PgnoLastBuffer2() const                        { return m_rgextentinfo[1].PgnoLast(); }

        VOID SetExtentBuffer1( const PGNO pgnoLast, const CPG cpg );
        VOID SetExtentBuffer2( const PGNO pgnoLast, const CPG cpg );

        PGNO PgnoNextAvailableFromBuffer1();
        PGNO PgnoNextAvailableFromBuffer2();
        BYTE AddPages( const PGNO pgnoLast, const CPG cpg, const BYTE ibuf );
        ERR ErrGetPage( const IFMP ifmp, PGNO * const ppgno, const BOOL fAvailExt );
        VOID ReturnPage( const PGNO pgno );

    private:
        EXTENTINFO      m_rgextentinfo[2];
};

INLINE VOID SPLIT_BUFFER::SetExtentBuffer1( const PGNO pgnoLast, const CPG cpg )
{
    Assert( cpg > 0 );
    Assert( pgnoNull != pgnoLast );
    m_rgextentinfo[0].pgnoLastInExtent = pgnoLast;
    m_rgextentinfo[0].cpgExtent = cpg;
}
INLINE VOID SPLIT_BUFFER::SetExtentBuffer2( const PGNO pgnoLast, const CPG cpg )
{
    Assert( cpg > 0 );
    Assert( pgnoNull != pgnoLast );
    m_rgextentinfo[1].pgnoLastInExtent = pgnoLast;
    m_rgextentinfo[1].cpgExtent = cpg;
}

INLINE PGNO SPLIT_BUFFER::PgnoNextAvailableFromBuffer1()
{
    Assert( CpgBuffer1() > 0 );
    Assert( pgnoNull != PgnoLastBuffer1() );
    m_rgextentinfo[0].cpgExtent--;
    return ( PgnoLastBuffer1() - CpgBuffer1() );
}
INLINE PGNO SPLIT_BUFFER::PgnoNextAvailableFromBuffer2()
{
    Assert( CpgBuffer2() > 0 );
    Assert( pgnoNull != PgnoLastBuffer2() );
    m_rgextentinfo[1].cpgExtent--;
    return ( PgnoLastBuffer2() - CpgBuffer2() );
}

INLINE BYTE SPLIT_BUFFER::AddPages( const PGNO pgnoLast, const CPG cpg, const BYTE ibuf )
{
    Assert( ibuf <= 2 );
    if ( ( ( CpgBuffer2() < CpgBuffer1() ) && ( ibuf == 0 ) ) || ( ibuf == 2 ) )
    {
        SetExtentBuffer2( pgnoLast, cpg );
        return 2;
    }
    else
    {
        SetExtentBuffer1( pgnoLast, cpg );
        return 1;
    }
}

INLINE ERR SPLIT_BUFFER::ErrGetPage( const IFMP ifmp, PGNO * const ppgno, const BOOL fAvailExt )
{
    ERR err = JET_errSuccess;
    *ppgno = pgnoNull;

    const BOOL fBuffer1Available = ( CpgBuffer1() > 0 ) &&
                                   !g_rgfmp[ifmp].FBeyondPgnoShrinkTarget( PgnoFirstBuffer1(), CpgBuffer1() ) &&
                                   ( PgnoLastBuffer1() <= g_rgfmp[ifmp].PgnoLast() );
    const BOOL fBuffer2Available = ( CpgBuffer2() > 0 ) &&
                                   !g_rgfmp[ifmp].FBeyondPgnoShrinkTarget( PgnoFirstBuffer2(), CpgBuffer2() ) &&
                                   ( PgnoLastBuffer2() <= g_rgfmp[ifmp].PgnoLast() );

    if ( fBuffer1Available &&
         ( !fBuffer2Available || ( CpgBuffer1() <= CpgBuffer2() ) ) )
    {
        *ppgno = PgnoNextAvailableFromBuffer1();
        SPCheckPgnoAllocTrap( *ppgno );
    }
    else if ( fBuffer2Available )
    {
        *ppgno = PgnoNextAvailableFromBuffer2();
        SPCheckPgnoAllocTrap( *ppgno );
    }
    else
    {

        FireWall( OSFormat( "GetPageOutOf%sSpBuf", fAvailExt ? "Avail" : "Own" ) );
        err = ErrERRCheck( fAvailExt ? errSPOutOfAvailExtCacheSpace : errSPOutOfOwnExtCacheSpace );
    }

    return err;
}

INLINE VOID SPLIT_BUFFER::ReturnPage( const PGNO pgno )
{
    if ( pgno == PgnoLastBuffer1() - CpgBuffer1() )
    {
        m_rgextentinfo[0].cpgExtent++;
    }
    else if ( pgno == PgnoLastBuffer2() - CpgBuffer2() )
    {
        m_rgextentinfo[1].cpgExtent++;
    }
    else
    {
        Assert( fFalse );
    }
}


class SPLITBUF_DANGLING
{
    friend class FCB;

    public:
        SPLITBUF_DANGLING()         {};
        ~SPLITBUF_DANGLING()        {};

    private:
        SPLIT_BUFFER        m_splitbufAE;
        SPLIT_BUFFER        m_splitbufOE;

        union
        {
            ULONG           m_ulFlags;
            struct
            {
                ULONG       m_fAvailExtDangling:1;
                ULONG       m_fOwnExtDangling:1;
            };
        };

    private:
        SPLIT_BUFFER *PsplitbufOE()                     { return ( FOwnExtDangling() ? &m_splitbufOE : NULL ); }
        SPLIT_BUFFER *PsplitbufAE()                     { return ( FAvailExtDangling() ? &m_splitbufAE : NULL ); }

        BOOL FOwnExtDangling()                          { return m_fOwnExtDangling; }
        VOID SetFOwnExtDangling()                       { m_fOwnExtDangling = fTrue; }
        VOID ResetFOwnExtDangling()                     { m_fOwnExtDangling = fFalse; }
        BOOL FAvailExtDangling()                        { return m_fAvailExtDangling; }
        VOID SetFAvailExtDangling()                     { m_fAvailExtDangling = fTrue; }
        VOID ResetFAvailExtDangling()                   { m_fAvailExtDangling = fFalse; }
};


const CPG   cpgInitialTreeDefault       = 1;

INLINE CPG CpgInitial( const JET_SPACEHINTS * const pspacehints, const LONG cbPageSize )
{
    Assert( pspacehints->cbInitial % cbPageSize == 0 );
    const CPG cpgInitialOfSpaceHints = pspacehints->cbInitial / cbPageSize;
    return cpgInitialOfSpaceHints ? cpgInitialOfSpaceHints : cpgInitialTreeDefault;
}


class FCB_SPACE_HINTS
{

    friend class FCB;
    friend CPG CpgSPIGetNextAlloc( __in const FCB_SPACE_HINTS * const pfcbsh, __in const CPG cpgPrevious    );
    friend ERR ErrCATCheckJetSpaceHints( _In_ const LONG cbPageSize, _Inout_ JET_SPACEHINTS * pSpaceHints, _In_ BOOL fAllowCorrection );

private:

    union
    {
        SHORT       m_ulSPHBaseFlagsAndDefaults;

        struct
        {

            BYTE    m_fSPHDefaultGrbit          : 1;

            BYTE    m_fSPHDefaultMaintDensity   : 1;
            BYTE    m_fSPHDefaultGrowth         : 1;
            BYTE    m_fSPHDefaultMinExtent      : 1;
            BYTE    m_fSPHDefaultMaxExtent      : 1;

            BYTE    m_fSPHUtilizeParentSpace        : 1;
            BYTE    m_fSPHUtilizeExactExtents       : 1;

            BYTE    m_fSPHCreateHintAppendSequential        : 1;
            BYTE    m_fSPHCreateHintHotpointSequential      : 1;
            BYTE    m_fSPHRetrieveHintTableScanForward      : 1;
            BYTE    m_fSPHRetrieveHintTableScanBackward     : 1;

#define BUG_EX_55077
#ifdef BUG_EX_55077
            BYTE    m_fSPHRetrieveHintReserve1              : 1;
            BYTE    m_fSPHRetrieveHintReserve2              : 1;
            BYTE    m_fSPHRetrieveHintReserve3              : 1;
#endif
            BYTE    m_fSPHDeleteHintTableSequential         : 1;

            BYTE    m_rgReservedDefaultBits     : 1;
        };
    };

    WORD    m_pctGrowth;
    CPG     m_cpgInitial;
    CPG     m_cpgMinExtent;
    CPG     m_cpgMaxExtent;

    SHORT   m_cbDensityFree;
    BYTE    m_pctMaintDensity;
    BYTE    m_rgReserved[1];

private:
    INLINE SHORT _CbReservedBytesFromDensity( ULONG ulDensity ) const
    {
        const SHORT s = (SHORT)( ( ( 100 - ulDensity ) * g_cbPage ) / 100 );
        return s;
    }
    INLINE ULONG _UlDensityFromReservedBytes( SHORT cbReservedBytes ) const
    {
        const ULONG ulDensityFree   = ( ( cbReservedBytes + 1 ) * 100 ) / g_cbPage;
        const ULONG ulDensity       = 100 - ulDensityFree;
        return ulDensity;
    }
public:
    INLINE SHORT CbReservedBytesFromDensity( ULONG ulDensity ) const
    {
        const SHORT s = _CbReservedBytesFromDensity( ulDensity );

        Assert( ulDensity == UlBound( ulDensity, ulFILEDensityLeast, ulFILEDensityMost ) );
        Assert( ((( 100 - ulDensity ) * g_cbPage ) / 100) < (size_t)g_cbPage );
        Assert( ulDensity == _UlDensityFromReservedBytes( s ) );

        return s;
    }
    INLINE ULONG UlDensityFromReservedBytes( SHORT cbReservedBytes ) const
    {
        const ULONG ulDensity       = _UlDensityFromReservedBytes( cbReservedBytes );

        Assert( ulDensity == UlBound( ulDensity, ulFILEDensityLeast, ulFILEDensityMost ) );
        Assert( cbReservedBytes == _CbReservedBytesFromDensity( ulDensity ) );

        return ulDensity;
    }
    BOOL FRequestFitsExtentRange( _In_ const CPG cpgRequest ) const
    {
        if ( m_cpgMinExtent != 0 && cpgRequest < m_cpgMinExtent )
        {
            return fFalse;
        }
        if ( m_cpgMaxExtent != 0 && cpgRequest > m_cpgMaxExtent )
        {
            return fFalse;
        }
        return fTrue;
    }

    void SetSpaceHints(
        _In_ const JET_SPACEHINTS * const   pSpaceHints,
        _In_ const LONG                     cbPageSize
        )
    {
        Assert( pSpaceHints->cbStruct == sizeof(*pSpaceHints) );
        Expected( cbPageSize != 0 );

        m_cbDensityFree                 = CbReservedBytesFromDensity( pSpaceHints->ulInitialDensity );
        Assert( pSpaceHints->cbInitial % cbPageSize == 0 );
        Assert( pSpaceHints->cbInitial == 0 || cbPageSize != 0 );
        m_cpgInitial                    = CpgInitial( pSpaceHints, cbPageSize );

        m_fSPHDefaultMaintDensity       = ( 0x0 == pSpaceHints->ulMaintDensity );
        m_pctMaintDensity               = (BYTE) ( pSpaceHints->ulMaintDensity ? pSpaceHints->ulMaintDensity : 90 );

        m_fSPHDefaultGrowth             = ( 0x0 == pSpaceHints->ulGrowth );
        m_pctGrowth                     = (WORD) pSpaceHints->ulGrowth;
        
        m_fSPHDefaultMinExtent          = ( 0x0 == pSpaceHints->cbMinExtent );
        Assert( pSpaceHints->cbMinExtent % g_cbPage == 0 );
        Assert( pSpaceHints->cbMinExtent == 0 || cbPageSize != 0 );
        m_cpgMinExtent                  = pSpaceHints->cbMinExtent ? ( pSpaceHints->cbMinExtent / cbPageSize ) : 0;

        m_fSPHDefaultMaxExtent          = ( 0x0 == pSpaceHints->cbMaxExtent );
        Assert( pSpaceHints->cbMaxExtent % g_cbPage == 0 );
        Assert( pSpaceHints->cbMaxExtent == 0 || cbPageSize != 0 );
        m_cpgMaxExtent                  = pSpaceHints->cbMaxExtent ? ( pSpaceHints->cbMaxExtent / cbPageSize ) : 0;

        m_fSPHDefaultGrbit                  = ( 0x0 == pSpaceHints->grbit );
        m_fSPHUtilizeParentSpace                = BoolSetFlag( pSpaceHints->grbit, JET_bitSpaceHintsUtilizeParentSpace );
        m_fSPHUtilizeExactExtents               = BoolSetFlag( pSpaceHints->grbit, JET_bitSpaceHintsUtilizeExactExtents);
        m_fSPHCreateHintAppendSequential        = BoolSetFlag( pSpaceHints->grbit, JET_bitCreateHintAppendSequential );
        m_fSPHCreateHintHotpointSequential      = BoolSetFlag( pSpaceHints->grbit, JET_bitCreateHintHotpointSequential );
        m_fSPHRetrieveHintTableScanForward      = BoolSetFlag( pSpaceHints->grbit, JET_bitRetrieveHintTableScanForward );
        m_fSPHRetrieveHintTableScanBackward     = BoolSetFlag( pSpaceHints->grbit, JET_bitRetrieveHintTableScanBackward );

#ifdef BUG_EX_55077
        m_fSPHRetrieveHintReserve1 = BoolSetFlag( pSpaceHints->grbit, JET_bitRetrieveHintReserve1 );
        m_fSPHRetrieveHintReserve2 = BoolSetFlag( pSpaceHints->grbit, JET_bitRetrieveHintReserve2 );
        m_fSPHRetrieveHintReserve3 = BoolSetFlag( pSpaceHints->grbit, JET_bitRetrieveHintReserve3 );
#endif
        m_fSPHDeleteHintTableSequential         = BoolSetFlag( pSpaceHints->grbit, JET_bitDeleteHintTableSequential );

    }

    void GetAPISpaceHints(
        _Out_ JET_SPACEHINTS * const        pSpaceHints,
        _In_ const LONG                     cbPageSize
        ) const
    {
        Assert( cbPageSize != 0 );

        memset( pSpaceHints, 0, sizeof(*pSpaceHints) );
        pSpaceHints->cbStruct           = sizeof(*pSpaceHints);

        pSpaceHints->cbInitial          = cpgInitialTreeDefault == m_cpgInitial ? 0 : cbPageSize * m_cpgInitial;
        pSpaceHints->ulInitialDensity   = UlDensityFromReservedBytes( m_cbDensityFree );


        
        pSpaceHints->ulGrowth           = m_fSPHDefaultGrowth ? 0x0 :
                                                m_pctGrowth;
        pSpaceHints->cbMinExtent        = m_fSPHDefaultMinExtent ? 0x0 :
                                                cbPageSize * m_cpgMinExtent;
        pSpaceHints->cbMaxExtent        = m_fSPHDefaultMaxExtent ?  0x0 :
                                                cbPageSize * m_cpgMaxExtent;


        pSpaceHints->ulMaintDensity     = m_fSPHDefaultMaintDensity ? 0x0 :
                                                m_pctMaintDensity;

        pSpaceHints->grbit = m_fSPHDefaultGrbit ? 0x0 :
                                (
                                ( m_fSPHUtilizeParentSpace  ? JET_bitSpaceHintsUtilizeParentSpace : 0 ) |
                                ( m_fSPHCreateHintAppendSequential          ? JET_bitCreateHintAppendSequential : 0 ) |
                                ( m_fSPHCreateHintHotpointSequential        ? JET_bitCreateHintHotpointSequential : 0 ) |
                                ( m_fSPHRetrieveHintTableScanForward        ? JET_bitRetrieveHintTableScanForward : 0 ) |
                                ( m_fSPHRetrieveHintTableScanBackward       ? JET_bitRetrieveHintTableScanBackward : 0 ) |
#ifdef BUG_EX_55077
                                ( m_fSPHRetrieveHintReserve1                ? JET_bitRetrieveHintReserve1 : 0 ) |
                                ( m_fSPHRetrieveHintReserve2                ? JET_bitRetrieveHintReserve2 : 0 ) |
                                ( m_fSPHRetrieveHintReserve3                ? JET_bitRetrieveHintReserve3 : 0 ) |
#endif
                                ( m_fSPHDeleteHintTableSequential           ? JET_bitDeleteHintTableSequential : 0 ) |
                                ( m_fSPHUtilizeExactExtents                 ? JET_bitSpaceHintsUtilizeExactExtents : 0 )
                                );


    }

};

#define CreateCompleteErr( err )        CreateComplete_( err, __FILE__, __LINE__ )
#define CreateComplete()                CreateComplete_( JET_errSuccess, __FILE__, __LINE__ )

SIZE_T FUCBOffsetOfIAE();

class FCB
    :   public CZeroInit
{
private:
#ifdef AMD64
    static VOID VerifyOptimalPacking();
#endif

    public:
        FCB( IFMP ifmp, PGNO pgnoFDP );
        ~FCB();

#pragma push_macro( "new" )
#undef new
    private:
        void* operator new( size_t );
        void* operator new[]( size_t );
        void operator delete[]( void* );
    public:
        void* operator new( size_t cbAlloc, INST* pinst )
        {
            return pinst->m_cresFCB.PvRESAlloc_( SzNewFile(), UlNewLine() );
        }
        void operator delete( void* pv )
        {
            if ( pv )
            {
                FCB* pfcb = reinterpret_cast< FCB* >( pv );
                PinstFromIfmp( pfcb->m_ifmp )->m_cresFCB.Free( pv );
            }
        }
#pragma pop_macro( "new" )

#ifdef DEBUGGER_EXTENSION
    public:
#else
    private:
#endif

        enum FCBTYPE
        {
            fcbtypeNull = 0,
            fcbtypeDatabase,
            fcbtypeTable,
            fcbtypeSecondaryIndex,
            fcbtypeTemporaryTable,
            fcbtypeSort,
            fcbtypeLV,

            fcbtypeMax
        };
    
    private:
        RECDANGLING *m_precdangling;
        volatile LSTORE     m_ls;

        TDB         *m_ptdb;
        FCB         *m_pfcbNextIndex;

        FCB         *m_pfcbLRU;
        FCB         *m_pfcbMRU;

        FCB         *m_pfcbNextList;
        FCB         *m_pfcbPrevList;

        FCB         *m_pfcbTable;
        IDB         *m_pidb;

        CInvasiveConcurrentModSet< FUCB, FUCBOffsetOfIAE >  m_icmsFucbList;
        LONG        m_wRefCount;

        OBJID       m_objidFDP;
        PGNO        m_pgnoFDP;
        PGNO        m_pgnoOE;
        PGNO        m_pgnoAE;


        union
        {
            ULONG   m_ulFCBListFlags;
            struct
            {
                ULONG   m_fFCBInList            : 1;
                ULONG   m_fFCBInLRU             : 1;
            };
        };

        IFMP        m_ifmp;

        ULONG   m_ulFCBFlags;

        static const ULONG mskFCBPrimaryIndex = 0x1;
        static const ULONG mskFCBSequentialIndex = 0x2;
        static const ULONG mskFCBFixedDDL = 0x4;
        static const ULONG mskFCBTemplate = 0x8;
        static const ULONG mskFCBDerivedTable = 0x10;
        static const ULONG mskFCBDerivedIndex = 0x20;
        static const ULONG mskFCBInitialIndex = 0x40;
        static const ULONG mskFCBInitialized = 0x80;
        static const ULONG mskFCBDeletePending = 0x100;
        static const ULONG mskFCBDeleteCommitted = 0x200;
        static const ULONG mskFCBNonUnique = 0x400;
        static const ULONG mskFCBNoCache = 0x800;
        static const ULONG mskFCBPreread = 0x1000;
        static const ULONG mskFCBSpaceInitialized = 0x2000;
        static const ULONG mskFCBTryPurgeOnClose = 0x4000;
        static const ULONG mskFCBDontLogSpaceOps = 0x8000;
        static const ULONG mskFCBIntrinsicLVsOnly = 0x10000;
        static const ULONG mskFCBOld2Running = 0x20000;
        static const ULONG mskFCBUncommitted = 0x40000;
        static const ULONG mskFCBValidatedCurrentLocales = 0x80000;
        static const ULONG mskFCBTemplateStatic = 0x100000;
        static const ULONG mskFCBInitedForRecovery = 0x200000;
        static const ULONG mskFCBDoingAdditionalInitializationDuringRecovery = 0x400000;
        static const ULONG mskFCBNoMoreTasks = 0x800000;
        static const ULONG mskFCBValidatedValidLocales = 0x1000000;


        TABLECLASS  m_tableclass;
        BYTE        m_fcbtype;
        BYTE        rgbReserved[2];

        USHORT      m_crefDomainDenyRead;
        USHORT      m_crefDomainDenyWrite;
        RCE         *m_prceNewest;
        RCE         *m_prceOldest;

        PIB         *m_ppibDomainDenyRead;

        PGNO        m_pgnoNextAvailSE;

        LONG        m_lInitLine;
        PCSTR       m_szInitFile;

        SPLITBUF_DANGLING   * m_psplitbufdangling;

        BFLatch             m_bflPgnoFDP;
        BFLatch             m_bflPgnoAE;
        BYTE                rgbReserved2[8];
        BFLatch             m_bflPgnoOE;

        INT                 m_ctasksActive;

        ERR                 m_errInit;


        CCriticalSection    m_critRCEList;

        CSXWLatch           m_sxwl;

        FCB_SPACE_HINTS     m_spacehints;

        TICK                m_tickMNVNLastReported;

        union
        {
            QWORD   m_qwMNVN;
            struct
            {
                QWORD   m_cMNVNIncidentsSinceLastReported:21;
                QWORD   m_cMNVNNodesSkippedSinceLastReported:21;
                QWORD   m_cMNVNPagesVisitedSinceLastReported:21;
            };
        };

    public:
        TABLECLASS Tableclass() const
        {
            if ( (PfcbTable() != NULL)  && ( FTypeLV() || FTypeSecondaryIndex() ) )
            {
                Assert( !PfcbTable()->FTypeLV() );
                Assert( !PfcbTable()->FTypeSecondaryIndex() );
                return PfcbTable()->Tableclass();
            }
            else
            {
                return m_tableclass;
            }
        }
        TABLECLASS Tableclass() { return m_tableclass; }
        VOID SetTableclass( TABLECLASS tableclass ) { m_tableclass = tableclass; }
        INLINE TCE TCE( BOOL fSpace = fFalse ) const
        {
            return ::TCEFromTableClass( Tableclass(), FTypeLV(), FTypeSecondaryIndex(), fSpace );
        }

        

    public:
        TDB* Ptdb() const;
        FCB* PfcbNextIndex() const;
        FCB* PfcbLRU() const;
        FCB* PfcbMRU() const;
        FCB* PfcbNextList() const;
        FCB* PfcbPrevList() const;
        FCB* PfcbTable() const;
        IDB* Pidb() const;
        CInvasiveConcurrentModSet< FUCB, FUCBOffsetOfIAE >& FucbList();
        FUCB* Pfucb();
        OBJID ObjidFDP() const;
        PGNO PgnoFDP() const;
        BFLatch* PBFLatchHintPgnoFDP();
        PGNO PgnoOE() const;
        BFLatch* PBFLatchHintPgnoOE();
        PGNO PgnoAE() const;
        BFLatch* PBFLatchHintPgnoAE();
        IFMP Ifmp() const;
        SHORT CbDensityFree() const;
        ULONG UlDensity() const;
        LONG WRefCount() const;
        RCE *PrceNewest() const;
        RCE *PrceOldest() const;
        USHORT CrefDomainDenyRead() const;
        USHORT CrefDomainDenyWrite() const;
        PIB* PpibDomainDenyRead() const;
        CCriticalSection& CritRCEList();
        ERR ErrErrInit() const;
        VOID GetAPISpaceHints( __out JET_SPACEHINTS * pjsph ) const;
        const FCB_SPACE_HINTS * Pfcbspacehints() const;

#ifdef DEBUGGER_EXTENSION
        VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
        const WCHAR * WszFCBType() const;

        BOOL FDebuggerExtInUse() const;
        BOOL FDebuggerExtPurgableEstimate() const;
#endif



    public:
        VOID SetPtdb( TDB *ptdb );
        VOID SetPfcbNextIndex( FCB *pfcb );
        VOID SetPfcbLRU( FCB *pfcb );
        VOID SetPfcbMRU( FCB *pfcb );
        VOID SetPfcbNextList( FCB *pfcb );
        VOID SetPfcbPrevList( FCB *pfcb );
        VOID SetPfcbTable( FCB *pfcb );
        VOID SetPidb( IDB *pidb );
        VOID SetObjidFDP( OBJID objid );
        VOID SetPgnoOE( PGNO pgno );
        VOID SetPgnoAE( PGNO pgno );
        VOID SetSpaceHints( _In_ const JET_SPACEHINTS * const pjsph );
        VOID SetErrInit( ERR err );

        VOID SetSortSpace( const PGNO pgno, const OBJID objid );
        VOID ResetSortSpace();



    public:
        ULONG UlFlags() const;

        VOID ResetFlags();

        ULONG UlFCBType() const;
        BOOL FTypeNull() const;

        BOOL FTypeDatabase() const;
        VOID SetTypeDatabase();

        BOOL FTypeTable() const;
        VOID SetTypeTable();

        BOOL FTypeSecondaryIndex() const;
        VOID SetTypeSecondaryIndex();

        BOOL FTypeTemporaryTable() const;
        VOID SetTypeTemporaryTable();

        BOOL FTypeSort() const;
        VOID SetTypeSort();

        BOOL FTypeLV() const;
        VOID SetTypeLV();

        BOOL FPrimaryIndex() const;
        VOID SetPrimaryIndex();
        VOID ResetPrimaryIndex();

        BOOL FSequentialIndex() const;
        VOID SetSequentialIndex();
        VOID ResetSequentialIndex();

        BOOL FFixedDDL() const;
        VOID SetFixedDDL();
        VOID ResetFixedDDL();

        BOOL FTemplateTable() const;
        VOID SetTemplateTable();

        BOOL FDerivedTable() const;
        VOID SetDerivedTable();

        BOOL FTemplateIndex() const;
        VOID SetTemplateIndex();

        BOOL FDerivedIndex() const;
        VOID SetDerivedIndex();

        BOOL FInitialIndex() const;
        VOID SetInitialIndex();

        BOOL FInitialized() const;
    private:
        VOID SetInitialized_();
        VOID ResetInitialized_();

    public:
        BOOL FInitedForRecovery() const;
        VOID SetInitedForRecovery();
        VOID ResetInitedForRecovery();

        BOOL FDoingAdditionalInitializationDuringRecovery() const;
        VOID SetDoingAdditionalInitializationDuringRecovery();
        VOID ResetDoingAdditionalInitializationDuringRecovery();

        BOOL FInList() const;
        VOID SetInList();
        VOID ResetInList();

        BOOL FInLRU() const;
        VOID SetInLRU();
        VOID ResetInLRU();

        BOOL FDeletePending() const;
        VOID SetDeletePending();
        VOID ResetDeletePending();

        BOOL FDeleteCommitted() const;
        VOID SetDeleteCommitted();

        BOOL FUnique() const;
        BOOL FNonUnique() const;
        VOID SetUnique();
        VOID SetNonUnique();

        BOOL FNoCache() const;
        VOID SetNoCache();
        VOID ResetNoCache();

        BOOL FPreread() const;
        VOID SetPreread();
        VOID ResetPreread();

        BOOL FSpaceInitialized() const;
        VOID SetSpaceInitialized();
        VOID ResetSpaceInitialized();

        BOOL FTryPurgeOnClose() const;
        VOID SetTryPurgeOnClose();
        VOID ResetTryPurgeOnClose();

        BOOL FDontLogSpaceOps() const;
        VOID SetDontLogSpaceOps();
        VOID ResetDontLogSpaceOps();

        BOOL FIntrinsicLVsOnly() const;
        VOID SetIntrinsicLVsOnly();

        BOOL FOLD2Running() const;
        VOID SetOLD2Running();
        VOID ResetOLD2Running();

        BOOL FUncommitted() const;
        VOID SetUncommitted();
        VOID ResetUncommitted();
        
        BOOL FValidatedCurrentLocales() const;
        VOID SetValidatedCurrentLocales();

        BOOL FValidatedValidLocales() const;
        VOID SetValidatedValidLocales();

        BOOL FTemplateStatic() const;
        VOID SetTemplateStatic();

        BOOL FNoMoreTasks() const;
        VOID SetNoMoreTasks();
        VOID ResetNoMoreTasks();

        BOOL FUtilizeParentSpace() const;
        BOOL FUtilizeExactExtents() const;
        INT PctMaintenanceDensity() const;
        BOOL FContiguousAppend() const;
        BOOL FContiguousHotpoint() const;


        BOOL FCreateHintAppendSequential() const        { return !!m_spacehints.m_fSPHCreateHintAppendSequential; }
        BOOL FCreateHintHotpointSequential() const      { return !!m_spacehints.m_fSPHCreateHintHotpointSequential; }
        BOOL FRetrieveHintTableScanForward() const      { return !!m_spacehints.m_fSPHRetrieveHintTableScanForward; }
        BOOL FRetrieveHintTableScanBackward() const     { return !!m_spacehints.m_fSPHRetrieveHintTableScanBackward; }
        BOOL FDeleteHintTableSequential() const         { return !!m_spacehints.m_fSPHDeleteHintTableSequential; }

        BOOL FUseOLD2();



    public:
        VOID AttachRCE( RCE * const prce );
        VOID DetachRCE( RCE * const prce );

        VOID SetPrceOldest( RCE * const prce );
        VOID SetPrceNewest( RCE * const prce );

        BOOL FDomainDenyWrite() const;
        VOID SetDomainDenyWrite();
        VOID ResetDomainDenyWrite();

        BOOL FDomainDenyRead( PIB *ppib ) const;
        BOOL FDomainDenyReadByUs( PIB *ppib ) const;
        VOID SetDomainDenyRead( PIB *ppib );
        VOID ResetDomainDenyRead();

        VOID SetIndexing();
        VOID ResetIndexing();

        ERR ErrSetUpdatingAndEnterDML( PIB *ppib, BOOL fWaitOnConflict = fFalse );
        VOID ResetUpdatingAndLeaveDML();
        VOID ResetUpdating();

        VOID IncrementRefCount();

    private:
        VOID ResetUpdating_();

        VOID IncrementRefCount_( BOOL fOwnWriteLock );
        ERR ErrIncrementRefCountAndLink_( FUCB *pfucb, const BOOL fOwnWriteLock = fFalse );
        VOID DecrementRefCountAndUnlink_( FUCB *pfucb, const BOOL fLockList, const BOOL fPreventMoveToAvail = fFalse );



    public:
        INT CTasksActive() const;
        VOID RegisterTask();
        VOID UnregisterTask();
        VOID WaitForTasksToComplete();


    public:
        static ERR ErrFCBInit( INST *pinst );
        static VOID Term( INST *pinst );
        BOOL FValid() const;

        static VOID RefreshPreferredPerfCounter( __in INST * const pinst );

    private:
        static VOID ResetPerfCounters( __in INST * const pinst, BOOL fTerminating );
        


    public:
        static FCB *PfcbFCBGet( IFMP ifmp, PGNO pgnoFDP, INT *pfState, const BOOL fIncrementRefCount = fTrue, const BOOL fInitForRecovery = fFalse );
        static ERR ErrCreate( PIB *ppib, IFMP ifmp, PGNO pgnoFDP, FCB **ppfcb );
        VOID CreateComplete_( ERR err, PCSTR szFile, const LONG lLine );
        VOID PrepareForPurge( const BOOL fPrepareChildren = fTrue );
        VOID CloseAllCursors( const BOOL fTerminating );
        VOID Purge( const BOOL fLockList = fTrue, const BOOL fTerminating = fFalse );
        static VOID PurgeObject( const IFMP ifmp, const PGNO pgnoFDP );
        static VOID PurgeDatabase( const IFMP ifmp, const BOOL fTerminating );
        static VOID PurgeAllDatabases( INST* const pinst );

    private:
        static ERR ErrAlloc_( PIB *ppib, IFMP ifmp, PGNO pgnoFDP, FCB **ppfcb );
        static BOOL FScanAndPurge_( __in INST * pinst, __in PIB * ppib, const BOOL fThreshold );
        static BOOL FCloseToQuota_( INST * pinst ) { return pinst->m_cresFCB.FCloseToQuota(); };
        static VOID PurgeObjects_( INST* const pinst, const IFMP ifmp, const PGNO pgnoFDP, const BOOL fTerminating );
        BOOL FCheckFreeAndPurge_( __in PIB *ppib, __in const BOOL fThreshold );
        VOID CloseAllCursorsOnFCB_( const BOOL fTerminating );
        VOID Delete_( INST *pinst );
        BOOL FHasCallbacks_( INST *pinst );
        BOOL FOutstandingVersions_();


    public:
        VOID Lock();
        VOID Unlock();
#ifdef DEBUG
        BOOL IsLocked();
        BOOL IsUnlocked();
#endif

        VOID EnterDML();
        VOID LeaveDML();
        VOID AssertDML();

        VOID EnterDDL();
        VOID LeaveDDL();
        VOID AssertDDL();

        BOOL FNeedLock() const;

    private:
        enum class LOCK_TYPE {
            ltWrite,
            ltShared };

        VOID Lock_( LOCK_TYPE lt);
        BOOL FLockTry_( LOCK_TYPE lt );
        VOID Unlock_( LOCK_TYPE lt );
#ifdef DEBUG
        BOOL IsLocked_( LOCK_TYPE lt );
        BOOL IsUnlocked_( LOCK_TYPE lt );
#endif
        VOID LockDowngradeWriteToShared_();

#ifdef DEBUG
        BOOL FWRefCountOK_();
#endif
        BOOL FNeedLock_() const;
        VOID EnterDML_();
        VOID LeaveDML_();
        VOID AssertDML_() const;
        VOID EnterDDL_();
        VOID LeaveDDL_();
        VOID AssertDDL_() const;



    public:
        VOID InsertHashTable();
        VOID DeleteHashTable();
        VOID Release();
        static BOOL FInHashTable( IFMP ifmp, PGNO pgnoFDB, FCB **ppfcb = NULL );

    private:



    public:
        VOID InsertList();
        VOID RemoveList();

    private:
        VOID RemoveList_();




    private:
#ifdef DEBUG
        BOOL FCBCheckAvailList_( const BOOL fShouldBeInList, const BOOL fPurging );
        VOID RemoveAvailList_( const BOOL fPurging = fFalse );
#else
        VOID RemoveAvailList_();
#endif
        VOID InsertAvailListMRU_();


    public:
        ERR ErrLinkReserveSpace();
        ERR ErrLink( FUCB *pfucb );
        VOID Unlink( FUCB *pfucb, const BOOL fPreventMoveToAvail = fFalse );
        VOID LinkPrimaryIndex();
        VOID LinkSecondaryIndex( FCB *pfcbIdx );
        VOID UnlinkSecondaryIndex( FCB *pfcbIdx );
        ERR ErrSetDeleteIndex( PIB *ppib );
        VOID ResetDeleteIndex();
        VOID UnlinkIDB( FCB *pfcbTable );
        VOID ReleasePidb( const BOOL fTerminating = fFalse );

        PGNO PgnoNextAvailSE() const;
        VOID SetPgnoNextAvailSE( const PGNO pgno );

        SPLIT_BUFFER *Psplitbuf( const BOOL fAvailExt );
        ERR ErrEnableSplitbuf( const BOOL fAvailExt );
        VOID DisableSplitbuf( const BOOL fAvailExt );

        RECDANGLING *Precdangling() const;
        VOID SetPrecdangling( RECDANGLING * const precdangling );
        VOID RemovePrecdangling( RECDANGLING * const precdangling );

        ERR ErrSetLS( const LSTORE ls );
        ERR ErrGetLS( LSTORE *pls, const BOOL fReset );

        TICK TickMNVNLastReported() const;
        VOID SetTickMNVNLastReported( const TICK tick );

        ULONG CMNVNIncidentsSinceLastReported() const;
        VOID ResetCMNVNIncidentsSinceLastReported();
        VOID IncrementCMNVNIncidentsSinceLastReported();

        ULONG CMNVNNodesSkippedSinceLastReported() const;
        VOID ResetCMNVNNodesSkippedSinceLastReported();
        VOID IncrementCMNVNNodesSkippedSinceLastReported( const ULONG cNodes );
    
        ULONG CMNVNPagesVisitedSinceLastReported() const;
        VOID ResetCMNVNPagesVisitedSinceLastReported();
        VOID IncrementCMNVNPagesVisitedSinceLastReported( const ULONG cPages );

    private:
        SPLITBUF_DANGLING *Psplitbufdangling_() const;
        VOID SetPsplitbufdangling_( SPLITBUF_DANGLING * const psplitbufdangling );

        VOID CheckFCBLockingForLink_();


};



inline FCBHash::NativeCounter FCBHash::CKeyEntry::Hash( const FCBHashKey &key )
{
    return FCBHash::NativeCounter( UiHashIfmpPgnoFDP( key.m_ifmp, key.m_pgnoFDP ) );
}

inline FCBHash::NativeCounter FCBHash::CKeyEntry::Hash() const
{
    Assert( pfcbNil != m_entry.m_pfcb );
    return FCBHash::NativeCounter( UiHashIfmpPgnoFDP( m_entry.m_pfcb->Ifmp(), m_entry.m_pgnoFDP ) );
}

inline BOOL FCBHash::CKeyEntry::FEntryMatchesKey( const FCBHashKey &key ) const
{
    Assert( pfcbNil != m_entry.m_pfcb );
    return m_entry.m_pgnoFDP == key.m_pgnoFDP && m_entry.m_pfcb->Ifmp() == key.m_ifmp;
}

inline void FCBHash::CKeyEntry::SetEntry( const FCBHashEntry &src )
{
    m_entry = src;
}

inline void FCBHash::CKeyEntry::GetEntry( FCBHashEntry * const pdst ) const
{
    *pdst = m_entry;
}





INLINE FCB::FCB( IFMP ifmp, PGNO pgnoFDP )
    :   CZeroInit( sizeof( FCB ) ),
        m_ifmp( ifmp ),
        m_tableclass( tableclassNone ),
        m_pgnoFDP( pgnoFDP ),
        m_ls( JET_LSNil ),
        m_critRCEList( CLockBasicInfo( CSyncBasicInfo( szFCBRCEList ), rankFCBRCEList, 0 ) ),
        m_sxwl( CLockBasicInfo( CSyncBasicInfo( szFCBSXWL ), rankFCBSXWL, 0 ) ),
        m_icmsFucbList( (PSTR)szFCBFUCBLISTRWL, rankFCBFUCBLISTRWL )
{
    FMP::AssertVALIDIFMP( ifmp );

    Assert( pgnoFDP != pgnoNull || g_rgfmp[ ifmp ].Dbid() == dbidTemp );

    Assert( m_bflPgnoFDP.pv         == NULL );
    Assert( m_bflPgnoFDP.dwContext  == NULL );
    Assert( m_bflPgnoOE.pv          == NULL );
    Assert( m_bflPgnoOE.dwContext   == NULL );
    Assert( m_bflPgnoAE.pv          == NULL );
    Assert( m_bflPgnoAE.dwContext   == NULL );

    Assert( m_ctasksActive == 0 );

}

INLINE FCB::~FCB()
{
    for ( RECDANGLING * precdangling = Precdangling(); NULL != precdangling; )
    {
        RECDANGLING * const precToFree  = precdangling;

        precdangling = precdangling->precdanglingNext;

        OSMemoryHeapFree( precToFree );
    }

    if ( NULL != Psplitbufdangling_() )
    {
        OSMemoryHeapFree( Psplitbufdangling_() );
    }

    if ( JET_LSNil != m_ls )
    {
        INST*           pinst   = PinstFromIfmp( m_ifmp );
        JET_CALLBACK    pfn     = (JET_CALLBACK)PvParam( pinst, JET_paramRuntimeCallback );

        Assert( NULL != pfn );
        (*pfn)(
            JET_sesidNil,
            JET_dbidNil,
            JET_tableidNil,
            JET_cbtypFreeTableLS,
            (VOID *)m_ls,
            NULL,
            NULL,
            NULL );
    }
}


#ifdef AMD64

INLINE VOID FCB::VerifyOptimalPacking()
{
#define NoWastedSpaceAround(TYPE, FIELDFIRST, FIELDLAST)                \
    (                                                                   \
        ( OffsetOf( TYPE, FIELDFIRST ) == 0 ) &&                        \
        ( OffsetOf( TYPE, FIELDLAST ) + sizeof( (( TYPE * ) 0 )->FIELDLAST) == sizeof( TYPE ) ) && \
        ( ( sizeof( TYPE ) % 32 ) == 0 )                                \
        ),                                                              \
        "Unexpected padding around " #TYPE

#define NoWastedSpace(TYPE, FIELD1, FIELD2)                             \
    ( OffsetOf( TYPE, FIELD1) + sizeof(((TYPE *)0)->FIELD1) == OffsetOf( TYPE, FIELD2 ) ), \
        "Unexpected padding between " #FIELD1 " and " #FIELD2

#define CacheLineMark(TYPE, FIELD, NUM)                                     \
     ( OffsetOf( TYPE, FIELD ) == ( 64 * NUM ) ), "Cache line marker"

    static_assert( sizeof( FCB ) == 352, "Current size" );
    
    static_assert( NoWastedSpaceAround( FCB, m_precdangling, m_qwMNVN ) );
    
    static_assert( CacheLineMark( FCB, m_precdangling,         0 ) );
    static_assert( NoWastedSpace( FCB, m_precdangling,         m_ls) );
    static_assert( NoWastedSpace( FCB, m_ls,                   m_ptdb) );
    static_assert( NoWastedSpace( FCB, m_ptdb,                 m_pfcbNextIndex) );
    static_assert( NoWastedSpace( FCB, m_pfcbNextIndex,        m_pfcbLRU) );
    static_assert( NoWastedSpace( FCB, m_pfcbLRU,              m_pfcbMRU) );
    static_assert( NoWastedSpace( FCB, m_pfcbMRU,              m_pfcbNextList) );
    static_assert( NoWastedSpace( FCB, m_pfcbNextList,         m_pfcbPrevList) );
    static_assert( NoWastedSpace( FCB, m_pfcbPrevList,         m_pfcbTable) );
    
    static_assert( CacheLineMark( FCB, m_pfcbTable,            1 ) );
    static_assert( NoWastedSpace( FCB, m_pfcbTable,            m_pidb) );
    static_assert( NoWastedSpace( FCB, m_pidb,                 m_icmsFucbList) );
    static_assert( NoWastedSpace( FCB, m_icmsFucbList,          m_wRefCount) );
    
    static_assert( CacheLineMark( FCB, m_wRefCount,            2 ) );
    static_assert( NoWastedSpace( FCB, m_wRefCount,            m_objidFDP) );
    static_assert( NoWastedSpace( FCB, m_objidFDP,             m_pgnoFDP) );
    static_assert( NoWastedSpace( FCB, m_pgnoFDP,              m_pgnoOE) );
    static_assert( NoWastedSpace( FCB, m_pgnoOE,               m_pgnoAE) );
    static_assert( NoWastedSpace( FCB, m_pgnoAE,               m_ulFCBListFlags) );
    static_assert( NoWastedSpace( FCB, m_ulFCBListFlags,       m_ifmp) );
    static_assert( NoWastedSpace( FCB, m_ifmp,                 m_ulFCBFlags) );
    static_assert( NoWastedSpace( FCB, m_ulFCBFlags,           m_tableclass) );
    static_assert( NoWastedSpace( FCB, m_tableclass,           m_fcbtype) );
    static_assert( NoWastedSpace( FCB, m_fcbtype,              rgbReserved) );
    static_assert( NoWastedSpace( FCB, rgbReserved,            m_crefDomainDenyRead) );
    static_assert( NoWastedSpace( FCB, m_crefDomainDenyRead,   m_crefDomainDenyWrite) );
    static_assert( NoWastedSpace( FCB, m_crefDomainDenyWrite,  m_prceNewest) );
    static_assert( NoWastedSpace( FCB, m_prceNewest,           m_prceOldest) );
    static_assert( NoWastedSpace( FCB, m_prceOldest,           m_ppibDomainDenyRead) );
    static_assert( NoWastedSpace( FCB, m_ppibDomainDenyRead,   m_pgnoNextAvailSE) );
    
    static_assert( CacheLineMark( FCB, m_pgnoNextAvailSE,      3 ) );
    static_assert( NoWastedSpace( FCB, m_pgnoNextAvailSE,      m_lInitLine) );
    static_assert( NoWastedSpace( FCB, m_lInitLine,            m_szInitFile) );
    static_assert( NoWastedSpace( FCB, m_szInitFile,           m_psplitbufdangling) );
    static_assert( NoWastedSpace( FCB, m_psplitbufdangling,    m_bflPgnoFDP) );
    static_assert( NoWastedSpace( FCB, m_bflPgnoFDP,           m_bflPgnoAE) );
    static_assert( NoWastedSpace( FCB, m_bflPgnoAE,            rgbReserved2) );
    static_assert( NoWastedSpace( FCB, rgbReserved2,           m_bflPgnoOE) );
    
    static_assert( CacheLineMark( FCB, m_bflPgnoOE,            4 ) );
    static_assert( NoWastedSpace( FCB, m_bflPgnoOE,            m_ctasksActive) );
    static_assert( NoWastedSpace( FCB, m_ctasksActive,         m_errInit) );
    static_assert( NoWastedSpace( FCB, m_errInit,              m_critRCEList) );
    static_assert( NoWastedSpace( FCB, m_critRCEList,          m_sxwl) );
    static_assert( NoWastedSpace( FCB, m_sxwl,                 m_spacehints) );
    
    static_assert( CacheLineMark( FCB, m_spacehints,           5 ) );
    static_assert( NoWastedSpace( FCB, m_spacehints,           m_tickMNVNLastReported) );
    static_assert( NoWastedSpace( FCB, m_tickMNVNLastReported, m_qwMNVN) );
}
#endif





INLINE TDB* FCB::Ptdb() const                   { return m_ptdb; }
INLINE FCB* FCB::PfcbNextIndex() const          { return m_pfcbNextIndex; }
INLINE FCB* FCB::PfcbLRU() const                { return m_pfcbLRU; }
INLINE FCB* FCB::PfcbMRU() const                { return m_pfcbMRU; }
INLINE FCB* FCB::PfcbNextList() const           { return m_pfcbNextList; }
INLINE FCB* FCB::PfcbPrevList() const           { return m_pfcbPrevList; }
INLINE FCB* FCB::PfcbTable() const              { return m_pfcbTable; }
INLINE IDB* FCB::Pidb() const                   { return m_pidb; }
INLINE CInvasiveConcurrentModSet< FUCB, FUCBOffsetOfIAE >& FCB::FucbList() {
    Assert( IsLocked_( LOCK_TYPE::ltWrite ) || IsLocked_( LOCK_TYPE::ltShared ) ); return m_icmsFucbList; }
INLINE OBJID FCB::ObjidFDP() const              { return m_objidFDP; }
INLINE PGNO FCB::PgnoFDP() const                { return m_pgnoFDP; }
INLINE BFLatch* FCB::PBFLatchHintPgnoFDP()      { Assert( NULL == m_bflPgnoFDP.pv ); return &m_bflPgnoFDP; }
INLINE PGNO FCB::PgnoOE() const                 { return m_pgnoOE; }
INLINE BFLatch* FCB::PBFLatchHintPgnoOE()       { Assert( NULL == m_bflPgnoOE.pv ); return &m_bflPgnoOE; }
INLINE PGNO FCB::PgnoAE() const                 { return m_pgnoAE; }
INLINE BFLatch* FCB::PBFLatchHintPgnoAE()       { Assert( NULL == m_bflPgnoAE.pv ); return &m_bflPgnoAE; }
INLINE IFMP FCB::Ifmp() const                   { return m_ifmp; }
INLINE SHORT FCB::CbDensityFree() const         { return m_spacehints.m_cbDensityFree; }
INLINE LONG FCB::WRefCount() const              { return m_wRefCount; }
INLINE RCE *FCB::PrceNewest() const             { return m_prceNewest; }
INLINE RCE *FCB::PrceOldest() const             { return m_prceOldest; }
INLINE USHORT FCB::CrefDomainDenyRead() const   { return m_crefDomainDenyRead; }
INLINE USHORT FCB::CrefDomainDenyWrite() const  { return m_crefDomainDenyWrite; }
INLINE PIB* FCB::PpibDomainDenyRead() const     { return m_ppibDomainDenyRead; }
INLINE CCriticalSection& FCB::CritRCEList()     { return m_critRCEList; }
INLINE ERR FCB::ErrErrInit() const              { return m_errInit; }
INLINE VOID FCB::GetAPISpaceHints( __out JET_SPACEHINTS * pjsph ) const 
{
    Assert( m_ifmp != ifmpNil );
    Assert( g_rgfmp[ m_ifmp ].CbPage() != 0 );
    m_spacehints.GetAPISpaceHints( pjsph, g_rgfmp[ m_ifmp ].CbPage() );
}
INLINE const FCB_SPACE_HINTS * FCB::Pfcbspacehints() const  { return &(m_spacehints); }
INLINE ULONG FCB::UlDensity() const             { return m_spacehints.UlDensityFromReservedBytes( m_spacehints.m_cbDensityFree ); }





INLINE VOID FCB::SetPtdb( TDB *ptdb )           { m_ptdb = ptdb; }
INLINE VOID FCB::SetPfcbNextIndex( FCB *pfcb )  { m_pfcbNextIndex = pfcb; }
INLINE VOID FCB::SetPfcbLRU( FCB *pfcb )        { m_pfcbLRU = pfcb; }
INLINE VOID FCB::SetPfcbMRU( FCB *pfcb )        { m_pfcbMRU = pfcb; }
INLINE VOID FCB::SetPfcbNextList( FCB *pfcb )   { m_pfcbNextList = pfcb; }
INLINE VOID FCB::SetPfcbPrevList( FCB *pfcb )   { m_pfcbPrevList = pfcb; }
INLINE VOID FCB::SetPfcbTable( FCB *pfcb )      { m_pfcbTable = pfcb; }
INLINE VOID FCB::SetPidb( IDB *pidb )           { m_pidb = pidb; }
INLINE VOID FCB::SetObjidFDP( OBJID objid )     { m_objidFDP = objid; }
INLINE VOID FCB::SetPgnoOE( PGNO pgno )         { m_pgnoOE = pgno; }
INLINE VOID FCB::SetPgnoAE( PGNO pgno )         { m_pgnoAE = pgno; }

INLINE VOID FCB::SetSpaceHints( _In_ const JET_SPACEHINTS * const pjsph )
{ 
    Assert( m_ifmp != ifmpNil ); 
    Assert( g_rgfmp[ m_ifmp ].CbPage() != 0 ); 
    m_spacehints.SetSpaceHints( pjsph, g_rgfmp[ m_ifmp ].CbPage() );
}
INLINE VOID FCB::SetErrInit( ERR err )          { m_errInit = err; }
INLINE VOID FCB::SetPrceNewest( RCE * const prce ) { m_prceNewest = prce; }
INLINE VOID FCB::SetPrceOldest( RCE * const prce ) { m_prceOldest = prce; }

INLINE VOID FCB::SetSortSpace( const PGNO pgno, const OBJID objid )
{
    Assert( pgnoNull != pgno );
    Assert( objidNil != objid );

    Assert( g_rgfmp[ Ifmp() ].Dbid() == dbidTemp );
    Assert( FTypeSort() );
    Assert( ObjidFDP() == objidNil );
    Assert( PgnoFDP() == pgnoNull );
    SetObjidFDP( objid );
    SetPgnoOE( pgno + 1 );
    SetPgnoAE( pgno + 2 );
    m_pgnoFDP = pgno;
    Lock();
    SetSpaceInitialized();
    Unlock();
}
INLINE VOID FCB::ResetSortSpace()
{
    Assert( g_rgfmp[ Ifmp() ].Dbid() == dbidTemp );
    Assert( FTypeSort() );
    Assert( ObjidFDP() != objidNil );
    Assert( PgnoFDP() != pgnoNull );
    SetObjidFDP( objidNil );
    SetPgnoOE( pgnoNull );
    SetPgnoAE( pgnoNull );
    m_pgnoFDP = pgnoNull;
    Lock();
    ResetSpaceInitialized();
    Unlock();
}




INLINE ULONG FCB::UlFlags() const               { return m_ulFCBFlags; }
INLINE VOID FCB::ResetFlags()                   { m_ulFCBFlags = 0; }

INLINE ULONG FCB::UlFCBType() const             { return ULONG( m_fcbtype ); }
INLINE BOOL FCB::FTypeNull() const              { return ( m_fcbtype == fcbtypeNull ); }

INLINE BOOL FCB::FTypeDatabase() const          { return ( m_fcbtype == fcbtypeDatabase ); }
INLINE VOID FCB::SetTypeDatabase()              { m_fcbtype = fcbtypeDatabase; }

INLINE BOOL FCB::FTypeTable() const             { return ( m_fcbtype == fcbtypeTable ); }
INLINE VOID FCB::SetTypeTable()                 { m_fcbtype = fcbtypeTable; }

INLINE BOOL FCB::FTypeSecondaryIndex() const    { return ( m_fcbtype == fcbtypeSecondaryIndex ); }
INLINE VOID FCB::SetTypeSecondaryIndex()        { m_fcbtype = fcbtypeSecondaryIndex; }

INLINE BOOL FCB::FTypeTemporaryTable() const    { return ( m_fcbtype == fcbtypeTemporaryTable ); }
INLINE VOID FCB::SetTypeTemporaryTable()        { m_fcbtype = fcbtypeTemporaryTable; }

INLINE BOOL FCB::FTypeSort() const              { return ( m_fcbtype == fcbtypeSort ); }
INLINE VOID FCB::SetTypeSort()                  { m_fcbtype = fcbtypeSort; }

INLINE BOOL FCB::FTypeLV() const                { return ( m_fcbtype == fcbtypeLV ); }
INLINE VOID FCB::SetTypeLV()                    { m_fcbtype = fcbtypeLV; }

INLINE BOOL FCB::FPrimaryIndex() const          { return !!(m_ulFCBFlags & mskFCBPrimaryIndex ); }
INLINE VOID FCB::SetPrimaryIndex()              { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBPrimaryIndex ); }

INLINE BOOL FCB::FSequentialIndex() const       { return !!(m_ulFCBFlags & mskFCBSequentialIndex ); }
INLINE VOID FCB::SetSequentialIndex()           { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBSequentialIndex ); }
INLINE VOID FCB::ResetSequentialIndex()         { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBSequentialIndex ); }

INLINE BOOL FCB::FFixedDDL() const              { return !!(m_ulFCBFlags & mskFCBFixedDDL ); }
INLINE VOID FCB::SetFixedDDL()                  { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBFixedDDL ); }
INLINE VOID FCB::ResetFixedDDL()                { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBFixedDDL ); }

INLINE BOOL FCB::FTemplateTable() const         { return !!(m_ulFCBFlags & mskFCBTemplate ); }
INLINE VOID FCB::SetTemplateTable()             { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBTemplate ); }

INLINE BOOL FCB::FDerivedTable() const          { return !!(m_ulFCBFlags & mskFCBDerivedTable ); }
INLINE VOID FCB::SetDerivedTable()              { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBDerivedTable ); }

INLINE BOOL FCB::FTemplateIndex() const         { return !!(m_ulFCBFlags & mskFCBTemplate ); }
INLINE VOID FCB::SetTemplateIndex()             { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBTemplate ); }

INLINE BOOL FCB::FDerivedIndex() const          { return !!(m_ulFCBFlags & mskFCBDerivedIndex ); }
INLINE VOID FCB::SetDerivedIndex()              { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBDerivedIndex ); }

INLINE BOOL FCB::FInitialIndex() const          { return !!(m_ulFCBFlags & mskFCBInitialIndex ); }
INLINE VOID FCB::SetInitialIndex()              { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBInitialIndex ); }

INLINE BOOL FCB::FInitialized() const           { return !!(m_ulFCBFlags & mskFCBInitialized ); }
INLINE VOID FCB::SetInitialized_()              { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBInitialized ); }
INLINE VOID FCB::ResetInitialized_()            { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBInitialized ); }

INLINE BOOL FCB::FInitedForRecovery() const     { return !!(m_ulFCBFlags & mskFCBInitedForRecovery ); }
INLINE VOID FCB::SetInitedForRecovery()         { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBInitedForRecovery ); }
INLINE VOID FCB::ResetInitedForRecovery()       { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBInitedForRecovery ); }

INLINE BOOL FCB::FDoingAdditionalInitializationDuringRecovery() const       { return !!(m_ulFCBFlags & mskFCBDoingAdditionalInitializationDuringRecovery ); }
INLINE VOID FCB::SetDoingAdditionalInitializationDuringRecovery()           { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBDoingAdditionalInitializationDuringRecovery ); }
INLINE VOID FCB::ResetDoingAdditionalInitializationDuringRecovery()         { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBDoingAdditionalInitializationDuringRecovery ); }

INLINE BOOL FCB::FInList() const                { return m_fFCBInList; }
INLINE VOID FCB::SetInList()                    { m_fFCBInList = fTrue; }
INLINE VOID FCB::ResetInList()                  { m_fFCBInList = fFalse; }

INLINE BOOL FCB::FInLRU() const                 { return m_fFCBInLRU; }
INLINE VOID FCB::SetInLRU()                     { m_fFCBInLRU = fTrue; }
INLINE VOID FCB::ResetInLRU()                   { m_fFCBInLRU = fFalse; }

INLINE BOOL FCB::FDeletePending() const         { return !!(m_ulFCBFlags & mskFCBDeletePending ); }
INLINE VOID FCB::SetDeletePending()             { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBDeletePending ); }
INLINE VOID FCB::ResetDeletePending()           { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBDeletePending ); }

INLINE BOOL FCB::FDeleteCommitted() const       { return !!(m_ulFCBFlags & mskFCBDeleteCommitted ); }
INLINE VOID FCB::SetDeleteCommitted()           { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBDeleteCommitted ); }

INLINE BOOL FCB::FUnique() const                { return !(m_ulFCBFlags & mskFCBNonUnique ); }
INLINE BOOL FCB::FNonUnique() const             { return !!(m_ulFCBFlags & mskFCBNonUnique ); }
INLINE VOID FCB::SetNonUnique()                 { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBNonUnique ); }
INLINE VOID FCB::SetUnique()                    { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBNonUnique ); }

INLINE BOOL FCB::FNoCache() const               { return !!(m_ulFCBFlags & mskFCBNoCache ); }
INLINE VOID FCB::SetNoCache()                   { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBNoCache ); }
INLINE VOID FCB::ResetNoCache()                 { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBNoCache ); }

INLINE BOOL FCB::FPreread() const               { return !!(m_ulFCBFlags & mskFCBPreread ); }
INLINE VOID FCB::SetPreread()                   { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBPreread ); }
INLINE VOID FCB::ResetPreread()                 { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBPreread ); }

INLINE BOOL FCB::FSpaceInitialized() const      { return !!(m_ulFCBFlags & mskFCBSpaceInitialized ); }
INLINE VOID FCB::SetSpaceInitialized()          { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBSpaceInitialized ); }
INLINE VOID FCB::ResetSpaceInitialized()        { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBSpaceInitialized ); }

INLINE BOOL FCB::FTryPurgeOnClose() const       { return !!(m_ulFCBFlags & mskFCBTryPurgeOnClose ); }
INLINE VOID FCB::SetTryPurgeOnClose()           { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBTryPurgeOnClose ); }
INLINE VOID FCB::ResetTryPurgeOnClose()         { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBTryPurgeOnClose ); }

#ifdef DONT_LOG_BATCH_INDEX_BUILD
INLINE BOOL FCB::FDontLogSpaceOps() const       { return !!(m_ulFCBFlags & mskFCBDontLogSpaceOps ); }
#else
INLINE BOOL FCB::FDontLogSpaceOps() const       { return fFalse; }
#endif
INLINE VOID FCB::SetDontLogSpaceOps()           { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBDontLogSpaceOps ); }
INLINE VOID FCB::ResetDontLogSpaceOps()         { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBDontLogSpaceOps ); }

INLINE BOOL FCB::FIntrinsicLVsOnly() const      { return !!(m_ulFCBFlags & mskFCBIntrinsicLVsOnly ); }
INLINE VOID FCB::SetIntrinsicLVsOnly()          { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBIntrinsicLVsOnly ); }

INLINE BOOL FCB::FOLD2Running() const           { return !!(m_ulFCBFlags & mskFCBOld2Running ); }
INLINE VOID FCB::SetOLD2Running()               { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBOld2Running ); }
INLINE VOID FCB::ResetOLD2Running()             { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBOld2Running ); }

INLINE BOOL FCB::FUncommitted() const           { return !!(m_ulFCBFlags & mskFCBUncommitted ); }
INLINE VOID FCB::SetUncommitted()               { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBUncommitted ); }
INLINE VOID FCB::ResetUncommitted()             { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBUncommitted ); }

INLINE BOOL FCB::FValidatedCurrentLocales() const { return !!(m_ulFCBFlags & mskFCBValidatedCurrentLocales ); }
INLINE VOID FCB::SetValidatedCurrentLocales()   { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBValidatedCurrentLocales ); }

INLINE BOOL FCB::FValidatedValidLocales() const { return !!(m_ulFCBFlags & mskFCBValidatedValidLocales ); }
INLINE VOID FCB::SetValidatedValidLocales()     { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBValidatedValidLocales ); }

INLINE BOOL FCB::FTemplateStatic() const        { return !!(m_ulFCBFlags & mskFCBTemplateStatic ); }
INLINE VOID FCB::SetTemplateStatic()
{
    Assert( FTemplateTable() );
    Assert( IsLocked() );
    AtomicExchangeSet( &m_ulFCBFlags, mskFCBTemplateStatic );
}

INLINE BOOL FCB::FNoMoreTasks() const           { return !!(m_ulFCBFlags & mskFCBNoMoreTasks ); }
INLINE VOID FCB::SetNoMoreTasks()               { Assert( IsLocked() ); AtomicExchangeSet( &m_ulFCBFlags, mskFCBNoMoreTasks ); }
INLINE VOID FCB::ResetNoMoreTasks()             { Assert( IsLocked() ); AtomicExchangeReset( &m_ulFCBFlags, mskFCBNoMoreTasks ); }

INLINE BOOL FCB::FUtilizeParentSpace() const    { return m_spacehints.m_fSPHUtilizeParentSpace; }
INLINE BOOL FCB::FUtilizeExactExtents() const   { return m_spacehints.m_fSPHUtilizeExactExtents; }
INLINE BOOL FCB::FContiguousAppend() const      { return FCreateHintAppendSequential() &&
                                                                ( FRetrieveHintTableScanForward() ||
                                                                    FRetrieveHintTableScanBackward() ||
                                                                    FDeleteHintTableSequential() ); }
INLINE BOOL FCB::FContiguousHotpoint() const        { return FCreateHintHotpointSequential() &&
                                                                ( FRetrieveHintTableScanForward() ||
                                                                    FRetrieveHintTableScanBackward() ||
                                                                    FDeleteHintTableSequential() ); }
INLINE INT FCB::PctMaintenanceDensity() const   { return m_spacehints.m_pctMaintDensity; }


#ifdef DEBUG
INLINE BOOL FCB::FWRefCountOK_()
{
    if ( m_wRefCount < 0 )
    {
        Assert( m_wRefCount >= 0 );
        return fFalse;
    }

    if ( IsLocked_( LOCK_TYPE::ltWrite ) && m_icmsFucbList.FLockedForEnumeration() )
    {
        if ( m_wRefCount < m_icmsFucbList.Count() )
        {
            return fFalse;
        }
        else
        {
            return fTrue;
        }
    }

    return fTrue;
}
#endif


INLINE BOOL FCB::FDomainDenyWrite() const       { return ( m_crefDomainDenyWrite > 0 ); }
INLINE VOID FCB::SetDomainDenyWrite()           { m_crefDomainDenyWrite++; }
INLINE VOID FCB::ResetDomainDenyWrite()
{
    Assert( m_crefDomainDenyWrite > 0 );
    m_crefDomainDenyWrite--;
}

INLINE BOOL FCB::FDomainDenyRead( PIB *ppib ) const
{
    Assert( ppibNil != ppib );
    return ( m_crefDomainDenyRead > 0 && ppib != m_ppibDomainDenyRead );
}
INLINE BOOL FCB::FDomainDenyReadByUs( PIB *ppib ) const
{
    Assert( ppibNil != ppib );
    return ( m_crefDomainDenyRead > 0 && ppib == m_ppibDomainDenyRead );
}
INLINE VOID FCB::SetDomainDenyRead( PIB *ppib )
{
    Assert( ppib != ppibNil );
    Assert( m_crefDomainDenyRead >= 0 );
    m_crefDomainDenyRead++;
    if ( m_crefDomainDenyRead == 1 )
    {
        Assert( m_ppibDomainDenyRead == ppibNil );
        m_ppibDomainDenyRead = ppib;
    }
    else
    {
        Assert( FDomainDenyReadByUs( ppib ) );
    }
}
INLINE VOID FCB::ResetDomainDenyRead()
{
    Assert( m_crefDomainDenyRead > 0 );
    Assert( m_ppibDomainDenyRead != ppibNil );
    m_crefDomainDenyRead--;
    if ( m_crefDomainDenyRead == 0 )
    {
        m_ppibDomainDenyRead = ppibNil;
    }
}




INLINE INT FCB::CTasksActive() const
{
    return m_ctasksActive;
}

INLINE VOID FCB::RegisterTask()
{
    Assert( !FNoMoreTasks() );
    AtomicIncrement( (LONG *)&m_ctasksActive );
}

INLINE VOID FCB::UnregisterTask()
{
    AtomicDecrement( (LONG *)&m_ctasksActive );
}

INLINE VOID FCB::WaitForTasksToComplete()
{
    Lock();
    SetNoMoreTasks();
    Unlock();
    
    while( 0 != m_ctasksActive )
    {
        UtilSleep( cmsecWaitGeneric );
    }
}



INLINE VOID FCB::Lock()
{
    return Lock_( LOCK_TYPE::ltWrite );
}

INLINE VOID FCB::Unlock()
{
    return Unlock_( LOCK_TYPE::ltWrite );
}

#ifdef DEBUG
INLINE BOOL FCB::IsLocked()
{
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );
    return IsLocked_( LOCK_TYPE::ltWrite );
}

INLINE BOOL FCB::IsUnlocked()
{
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );
    return IsUnlocked_( LOCK_TYPE::ltWrite );
}
#endif

INLINE BOOL FCB::FNeedLock() const
{
    return FNeedLock_();
}

INLINE BOOL FCB::FNeedLock_() const
{
    const BOOL  fNeedCrit   = ( dbidTemp != g_rgfmp[ Ifmp() ].Dbid() || pgnoSystemRoot == PgnoFDP() );

    if ( !fNeedCrit )
    {
#ifdef NOT_YET
        Assert( m_sxwl.FNotOwnWriteLatch() );
        Assert( m_sxwl.FNotOwnSharedLatch() );
        Assert( !m_sxwl.FLatched() );
#endif
    }

    return fNeedCrit;
}

INLINE VOID FCB::EnterDML()
{
    Assert( FTypeTable() || FTypeTemporaryTable() || FTypeSort() );
    Assert( Ptdb() != ptdbNil );

    if ( !FFixedDDL() )
    {
        Assert( FTypeTable() );
        Assert( !FTemplateTable() );
        EnterDML_();
    }

    AssertDML();
}

INLINE VOID FCB::LeaveDML()
{
    Assert( FTypeTable() || FTypeTemporaryTable() || FTypeSort() );
    Assert( Ptdb() != ptdbNil );
    AssertDML();

    if ( !FFixedDDL() )
    {
        Assert( FTypeTable() );
        Assert( !FTemplateTable() );
        LeaveDML_();
    }
}

INLINE VOID FCB::AssertDML()
{
#ifdef DEBUG

    Assert( FTypeTable() || FTypeTemporaryTable() || FTypeSort() );
    Assert( Ptdb() != ptdbNil );

    if ( !FFixedDDL() )
    {
        Assert( FTypeTable() );
        Assert( !FTemplateTable() );
        Assert( IsUnlocked() );
        AssertDML_();
    }

#endif
}

INLINE VOID FCB::EnterDDL()
{
    Assert( FTypeTable() || FTypeTemporaryTable() || FTypeSort() );
    Assert( Ptdb() != ptdbNil );

    EnterDDL_();
    AssertDDL();
}

INLINE VOID FCB::LeaveDDL()
{
    Assert( FTypeTable() || FTypeTemporaryTable() || FTypeSort() );
    Assert( Ptdb() != ptdbNil );

    AssertDDL();
    LeaveDDL_();
}

INLINE VOID FCB::AssertDDL()
{
#ifdef DEBUG
    Assert( FTypeTable() || FTypeTemporaryTable() || FTypeSort() );
    Assert( Ptdb() != ptdbNil );

    Assert( IsUnlocked() );
    AssertDDL_();
#endif
}



const INT fFCBStateNull             = 0;
const INT fFCBStateInitialized      = 1;

INLINE VOID FCB::Release()
{
#ifdef DEBUG
    FCB *pfcbT;
    if ( !FInHashTable( Ifmp(), PgnoFDP(), &pfcbT ) )
    {
        Assert( FDeleteCommitted() );
    }
    else
    {


        Assert( pfcbT == this || ( FDeleteCommitted() && ObjidFDP() != pfcbT->ObjidFDP() ) );
    }
#endif
    DecrementRefCountAndUnlink_( pfucbNil, fTrue );
}

INLINE BOOL FCB::FInHashTable( IFMP ifmp, PGNO pgnoFDP, FCB **ppfcb )
{
    INST                *pinst = PinstFromIfmp( ifmp );
    FCBHash::CLock      lockFCBHash;
    FCBHashKey          keyFCBHash( ifmp, pgnoFDP );
    FCBHashEntry        entryFCBHash;


    pinst->m_pfcbhash->ReadLockKey( keyFCBHash, &lockFCBHash );


    FCBHash::ERR errFCBHash = pinst->m_pfcbhash->ErrRetrieveEntry( &lockFCBHash, &entryFCBHash );
    Assert( errFCBHash == FCBHash::ERR::errSuccess || errFCBHash == FCBHash::ERR::errEntryNotFound );


    pinst->m_pfcbhash->ReadUnlockKey( &lockFCBHash );

    if ( ppfcb != NULL )
    {


        *ppfcb = ( errFCBHash == FCBHash::ERR::errSuccess ? entryFCBHash.m_pfcb : pfcbNil );
        Assert( pfcbNil == *ppfcb || entryFCBHash.m_pgnoFDP == pgnoFDP );
    }

    return errFCBHash == FCBHash::ERR::errSuccess;
}



INLINE VOID FCB::LinkPrimaryIndex()
{
    FCB *pfcbPrimary = this;
    FCB *pfcbIdx;

    Assert( g_rgfmp[ Ifmp() ].Dbid() != dbidTemp );
    Assert( FPrimaryIndex() );
    Assert( !( FTemplateTable() && FDerivedTable() ) );

    for ( pfcbIdx = PfcbNextIndex(); pfcbIdx != pfcbNil; pfcbIdx = pfcbIdx->PfcbNextIndex() )
    {
        Assert( pfcbIdx->FTypeSecondaryIndex() );
        Assert( pfcbIdx->Pidb() != pidbNil );

        Assert( !pfcbIdx->Pidb()->FVersioned() );

        pfcbIdx->SetPfcbTable( pfcbPrimary );

        Assert( !( pfcbIdx->FTemplateIndex() && pfcbIdx->FDerivedIndex() ) );
        Assert( ( pfcbIdx->FTemplateIndex() && FTemplateTable() )
                || ( !pfcbIdx->FTemplateIndex() && !FTemplateTable() ) );
        Assert( !pfcbIdx->FDerivedIndex() || FDerivedTable() );
        Assert( !pfcbIdx->Pidb()->FTemplateIndex()
            || pfcbIdx->FTemplateIndex()
            || pfcbIdx->FDerivedIndex() );
        Assert( pfcbIdx->Pidb()->FIDBOwnedByFCB() || !pfcbIdx->Pidb()->FDerivedIndex() );
    }
}

INLINE VOID FCB::LinkSecondaryIndex( FCB *pfcbIdx )
{
    AssertDDL();
    Assert( FPrimaryIndex() );
    Assert( FTypeTable() );
    Assert( pfcbIdx->FTypeSecondaryIndex() );
    pfcbIdx->SetPfcbNextIndex( PfcbNextIndex() );
    SetPfcbNextIndex( pfcbIdx );
    pfcbIdx->SetPfcbTable( this );
    return;
}

INLINE VOID FCB::UnlinkSecondaryIndex( FCB *pfcbIdx )
{
    Assert( FPrimaryIndex() );
    Assert( FTypeTable() );
    Assert( pfcbIdx->FTypeSecondaryIndex() );
    Assert( Ptdb() != ptdbNil );
    AssertDDL();

    Assert( !( pfcbIdx->FTemplateIndex() ) );
    Assert( !( pfcbIdx->FDerivedIndex() ) );

    pfcbIdx->UnlinkIDB( this );

    FCB *pfcbT;
    for ( pfcbT = this;
        pfcbT != pfcbNil;
        pfcbT = pfcbT->PfcbNextIndex() )
    {
        if ( pfcbT->PfcbNextIndex() == pfcbIdx )
        {
            pfcbT->SetPfcbNextIndex( pfcbIdx->PfcbNextIndex() );
            break;
        }
    }

    pfcbIdx->SetPfcbTable( pfcbNil );
}


INLINE VOID FCB::ResetDeleteIndex()
{
    Assert( FTypeSecondaryIndex() );
    Assert( pfcbNil != PfcbTable() );
    PfcbTable()->EnterDDL();

    Assert( Pidb() != pidbNil );
    Assert( Pidb()->FDeleted() );
    Pidb()->ResetFDeleted();

    if ( Pidb()->FVersioned() )
    {
        if ( !Pidb()->FVersionedCreate() )
        {
            Pidb()->ResetFVersioned();
        }
    }
    else
    {
        Assert( !Pidb()->FVersionedCreate() );
    }

    Assert( FDeletePending() );
    Assert( !FDeleteCommitted() );

    Lock();
    ResetDeletePending();
    ResetDomainDenyRead();
    Unlock();

    PfcbTable()->LeaveDDL();
}

INLINE VOID FCB::ReleasePidb( const BOOL fTerminating )
{
    Assert( Pidb() != pidbNil );

    if ( Pidb()->FIDBOwnedByFCB() || !FDerivedIndex() )
    {
        Assert( !Pidb()->FVersioned() || fTerminating || errFCBUnusable == ErrErrInit() );
        Assert( Pidb()->CrefVersionCheck() == 0 );
        Assert( Pidb()->CrefCurrentIndex() == 0 );
        delete Pidb();
    }
    SetPidb( pidbNil );
}




INLINE PGNO FCB::PgnoNextAvailSE() const
{
    return m_pgnoNextAvailSE;
}
INLINE VOID FCB::SetPgnoNextAvailSE( const PGNO pgno )
{
    m_pgnoNextAvailSE = pgno;
}


INLINE SPLITBUF_DANGLING *FCB::Psplitbufdangling_() const
{
    return m_psplitbufdangling;
}

INLINE VOID FCB::SetPsplitbufdangling_( SPLITBUF_DANGLING * const psplitbufdangling )
{
    m_psplitbufdangling = psplitbufdangling;
}

INLINE SPLIT_BUFFER *FCB::Psplitbuf( const BOOL fAvailExt )
{
    SPLITBUF_DANGLING   * const psplitbufdangling = Psplitbufdangling_();

    if ( NULL == psplitbufdangling )
        return NULL;

    return ( fAvailExt ? psplitbufdangling->PsplitbufAE() : psplitbufdangling->PsplitbufOE() );
}

INLINE ERR FCB::ErrEnableSplitbuf( const BOOL fAvailExt )
{
    AssertTrack( fFalse, "UnexpectedDangSpBuf" );

    if ( NULL == Psplitbufdangling_() )
    {
        SetPsplitbufdangling_( (SPLITBUF_DANGLING *)PvOSMemoryHeapAlloc( sizeof(SPLITBUF_DANGLING) ) );
        if ( NULL == Psplitbufdangling_() )
            return ErrERRCheck( JET_errOutOfMemory );
        memset( Psplitbufdangling_(), 0, sizeof(SPLITBUF_DANGLING) );
    }

    if ( fAvailExt )
    {
        Psplitbufdangling_()->SetFAvailExtDangling();
    }
    else
    {
        Psplitbufdangling_()->SetFOwnExtDangling();
    }

    return JET_errSuccess;
}

INLINE VOID FCB::DisableSplitbuf( const BOOL fAvailExt )
{
    SPLITBUF_DANGLING   * const psplitbufdangling   = Psplitbufdangling_();
    BOOL                fFree;

    Assert( NULL != psplitbufdangling );
    if ( fAvailExt )
    {
        Assert( psplitbufdangling->FAvailExtDangling() );
        psplitbufdangling->ResetFAvailExtDangling();
        fFree = !psplitbufdangling->FOwnExtDangling();
    }
    else
    {
        Assert( psplitbufdangling->FOwnExtDangling() );
        psplitbufdangling->ResetFOwnExtDangling();
        fFree = !psplitbufdangling->FAvailExtDangling();
    }

    if ( fFree )
    {
        OSMemoryHeapFree( psplitbufdangling );
        SetPsplitbufdangling_( NULL );
    }
}


INLINE RECDANGLING *FCB::Precdangling() const
{
    return m_precdangling;
}
INLINE VOID FCB::SetPrecdangling( RECDANGLING * const precdangling )
{
    m_precdangling = precdangling;
}
INLINE VOID FCB::RemovePrecdangling( RECDANGLING * const precdanglingRemove )
{
    for ( RECDANGLING ** pprecdangling = &m_precdangling;
        NULL != *pprecdangling;
        pprecdangling = &( (*pprecdangling)->precdanglingNext ) )
    {
        if ( *pprecdangling == precdanglingRemove )
        {
            *pprecdangling = precdanglingRemove->precdanglingNext;
            OSMemoryHeapFree( precdanglingRemove );
            return;
        }
    }

    Assert( fFalse );
}

INLINE ERR FCB::ErrSetLS( const LSTORE ls )
{
    ERR             err     = JET_errSuccess;
    const LSTORE    lsT     = m_ls;

    if ( JET_LSNil == ls )
    {
        m_ls = JET_LSNil;
    }
    else if ( JET_LSNil != lsT
        || LSTORE( AtomicCompareExchangePointer( (void**)&m_ls, (void*)lsT, (void*)ls ) ) != lsT  )
    {
        err = ErrERRCheck( JET_errLSAlreadySet );
    }

    return err;
}
INLINE ERR FCB::ErrGetLS( LSTORE *pls, const BOOL fReset )
{
    if ( fReset )
    {
        *pls = (ULONG_PTR) AtomicExchangePointer( (void **)(&m_ls), (void *)JET_LSNil );
    }
    else
    {
        *pls = m_ls;
    }

    return ( JET_LSNil == *pls ? ErrERRCheck( JET_errLSNotSet ) : JET_errSuccess );
}

INLINE TICK FCB::TickMNVNLastReported() const                       { return m_tickMNVNLastReported; }
INLINE VOID FCB::SetTickMNVNLastReported( const TICK tick )         { m_tickMNVNLastReported = tick; }

INLINE ULONG FCB::CMNVNIncidentsSinceLastReported() const           { return m_cMNVNIncidentsSinceLastReported & 0x000FFFFF; }
INLINE VOID FCB::ResetCMNVNIncidentsSinceLastReported()             { m_cMNVNIncidentsSinceLastReported = 0; }
INLINE VOID FCB::IncrementCMNVNIncidentsSinceLastReported()         { m_cMNVNIncidentsSinceLastReported = ( m_cMNVNIncidentsSinceLastReported & 0x000FFFFF ) + 1; }

INLINE ULONG FCB::CMNVNNodesSkippedSinceLastReported() const        { return m_cMNVNNodesSkippedSinceLastReported & 0x000FFFFF; }
INLINE VOID FCB::ResetCMNVNNodesSkippedSinceLastReported()          { m_cMNVNNodesSkippedSinceLastReported = 0; }
INLINE VOID FCB::IncrementCMNVNNodesSkippedSinceLastReported( const ULONG cNodes )
                                                                    { m_cMNVNNodesSkippedSinceLastReported = ( m_cMNVNNodesSkippedSinceLastReported & 0x000FFFFF ) + cNodes; }

INLINE ULONG FCB::CMNVNPagesVisitedSinceLastReported() const        { return m_cMNVNPagesVisitedSinceLastReported & 0x000FFFFF; }
INLINE VOID FCB::ResetCMNVNPagesVisitedSinceLastReported()          { m_cMNVNPagesVisitedSinceLastReported = 0; }
INLINE VOID FCB::IncrementCMNVNPagesVisitedSinceLastReported( const ULONG cPages )
                                                                    { m_cMNVNPagesVisitedSinceLastReported = ( m_cMNVNPagesVisitedSinceLastReported & 0x000FFFFF ) + cPages; }



#ifdef DEBUG
VOID FCBAssertAllClean( INST *pinst );
#else
#define FCBAssertAllClean( pinst )
#endif

