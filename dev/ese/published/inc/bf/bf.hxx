// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef BF_H_INCLUDED
#define BF_H_INCLUDED




ERR ErrBFInit( __in const LONG cbPageSizeMax );
void BFTerm();


ERR ErrBFGetCacheSize( ULONG_PTR* const pcpg );
ERR ErrBFSetCacheSize( const ULONG_PTR cpg );
ERR ErrBFSetCacheSizeRange();

LONG LBFICacheCleanPercentage( void );
LONG LBFICacheSizePercentage( void );
LONG LBFICachePinnedPercentage( void );


ERR ErrBFCheckMaintAvailPoolStatus();





void BFSetBFFMPContextAttached( IFMP ifmp );
void BFResetBFFMPContextAttached( IFMP ifmp );


enum BFConsumeSetting
{
    bfcsCacheSize       = 0x00000001,
    bfcsCheckpoint      = 0x00000002,
};

ERR ErrBFConsumeSettings( BFConsumeSetting bfcs, const IFMP ifmp );


ERR ErrBFFlush( IFMP ifmp, const OBJID objidFDP = objidNil, const PGNO pgnoFirst = pgnoNull, const PGNO pgnoLast = pgnoNull );
ERR ErrBFFlushSync( IFMP ifmp );
void BFPurge( IFMP ifmp, PGNO pgnoFirst = pgnoNull, CPG cpg = 0 );


void BFGetBestPossibleWaypoint(
    __in    IFMP        ifmp,
    __in    const LONG  lgenCommitted,
    __out   LGPOS *     plgposBestWaypoint );
void BFGetLgposOldestBegin0( IFMP ifmp, LGPOS* plgpos, LGPOS lgposOldestTrx );




const ULONG shfFaultIoPriority = 9;

enum BFPriority
{
    bfpriCacheResourcePriorityMask     = 0x000003FF,
    bfpriFaultIoPriorityMask           = ( ( qosIODispatchMask | qosIOOSLowPriority ) << shfFaultIoPriority ),
    bfpriUserIoPriorityTagMask         = JET_IOPriorityUserClassIdMask | JET_IOPriorityMarkAsMaintenance,
};
C_ASSERT( bfpriFaultIoPriorityMask    == 0x00FEE400 );
C_ASSERT( bfpriUserIoPriorityTagMask  == 0x4F000000 );

C_ASSERT( ( bfpriCacheResourcePriorityMask & bfpriFaultIoPriorityMask ) == 0 );
C_ASSERT( ( bfpriCacheResourcePriorityMask & bfpriUserIoPriorityTagMask ) == 0 );
C_ASSERT( ( bfpriFaultIoPriorityMask & bfpriUserIoPriorityTagMask ) == 0 );

C_ASSERT( ( ~bfpriFaultIoPriorityMask & ( ( qosIODispatchMask | qosIOOSLowPriority ) << shfFaultIoPriority ) ) == 0 );

enum BFTEMPOSFILEQOS : QWORD
{
    doNotUseThis = 0xFFFFFFFF,
};

INLINE BFPriority BfpriBFMake( const ULONG_PTR pctCachePriority, const BFTEMPOSFILEQOS qosPassed )
{
    OSFILEQOS qosIoPriority = qosPassed;
#ifndef ENABLE_JET_UNIT_TEST
    Assert( pctCachePriority <= 1000 );
    Assert( ( pctCachePriority & ~bfpriCacheResourcePriorityMask ) == 0 );
#endif

    Assert( ( ( ( qosIoPriority & qosIODispatchMask ) << shfFaultIoPriority ) & ~bfpriFaultIoPriorityMask ) == 0 );

#ifndef ENABLE_JET_UNIT_TEST
    Assert( ( qosIoPriority & ~qosIODispatchMask & ~qosIOOSLowPriority & ~bfpriUserIoPriorityTagMask ) == 0 );
#endif 

    return (BFPriority)( pctCachePriority & bfpriCacheResourcePriorityMask | 
                         ( ( qosIoPriority << shfFaultIoPriority ) & bfpriFaultIoPriorityMask ) | 
                         ( qosIoPriority & ( JET_IOPriorityUserClassIdMask | JET_IOPriorityMarkAsMaintenance ) ) );
}

INLINE ULONG_PTR PctBFCachePri( const BFPriority bfpri )
{
    return bfpri & bfpriCacheResourcePriorityMask;
}
inline BFTEMPOSFILEQOS QosBFUserAndIoPri( const BFPriority bfpri )
{
    return (BFTEMPOSFILEQOS)( ( bfpri & bfpriUserIoPriorityTagMask ) |
                              ( ( bfpri & bfpriFaultIoPriorityMask ) >> shfFaultIoPriority ) );
}



enum BFPreReadFlags
{
    bfprfNone               = 0x00000000,
    bfprfDefault            = 0x00000000,

    bfprfNoIssue            = 0x00000100,
    bfprfCombinableOnly     = 0x00000200,
    bfprfDBScan             = 0x00000400,
};
DEFINE_ENUM_FLAG_OPERATORS_BASIC( BFPreReadFlags );


void BFPrereadPageRange( IFMP ifmp, const PGNO pgnoFirst, CPG cpg, CPG* pcpgActual, BYTE *rgfPageAlreadyCached, const BFPreReadFlags bfprf, const BFPriority bfpri, const TraceContext& tc );
void BFPrereadPageList( IFMP ifmp, PGNO* prgpgno, CPG* pcpgActual, const BFPreReadFlags bfprf, const BFPriority bfpri, const TraceContext& tc );
ERR ErrBFPrereadPage( const IFMP ifmp, const PGNO pgno, const BFPreReadFlags bfprf, const BFPriority bfpri, const TraceContext& tc );

inline void BFPrereadPageList( IFMP ifmp, PGNO* prgpgno, const BFPreReadFlags bfprf, const BFPriority bfpri, const TraceContext& tc )
{
    BFPrereadPageList( ifmp, prgpgno, NULL, bfprf, bfpri, tc );
}

inline void BFPrereadPageRange( IFMP ifmp, PGNO pgnoFirst, CPG cpg, const BFPreReadFlags bfprf, const BFPriority bfpri, const TraceContext& tc )
{
    BFPrereadPageRange( ifmp, pgnoFirst, cpg, NULL, NULL, bfprf, bfpri, tc );
}


class BFReserveAvailPages
{
    public:
        BFReserveAvailPages( const CPG cpgWanted );
        ~BFReserveAvailPages();

        CPG CpgReserved() const;

    private:
        LONG m_cpgReserved;

        static LONG s_cpgReservedTotal;
        
    private:

#pragma push_macro( "new" )
#undef new
        void *  operator new[]( size_t );
        void    operator delete[]( void* );
        void *  operator new( size_t );
        void    operator delete( void* );
#pragma pop_macro( "new" )

        BFReserveAvailPages();
        BFReserveAvailPages( const BFReserveAvailPages& );
        BFReserveAvailPages& operator=( const BFReserveAvailPages& );
};

#ifdef DEBUG
INLINE BOOL FBFApiClean()
{
    return Ptls()->cbfAsyncReadIOs == 0;
}
#endif



struct BFLatch
{
    void*       pv;
    DWORD_PTR   dwContext;
};

enum BFLatchFlags
{
    bflfNone            = 0x00000000,
    bflfDefault         = 0x00000000,

    bflfNoTouch         = 0x00000100,
    bflfNoWait          = 0x00000200,
    bflfNoCached        = 0x00000400,
    bflfNoUncached      = 0x00000800,
    bflfNoFaultFail     = 0x00001000,
    bflfNew             = 0x00002000,
    bflfHint            = 0x00004000,
    bflfNoEventLogging  = 0x00008000,
    bflfUninitPageOk    = 0x00010000,
    bflfExtensiveChecks = 0x00020000,

    bflfDBScan          = 0x00040000,
    bflfNewIfUncached   = 0x00080000,
    bflfLatchAbandoned  = 0x00100000,
};
DEFINE_ENUM_FLAG_OPERATORS_BASIC( BFLatchFlags );

ERR ErrBFReadLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf, const BFPriority bfpri, const TraceContext& tc );
ERR ErrBFRDWLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf, const BFPriority bfpri, const TraceContext& tc );
ERR ErrBFWARLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf, const BFPriority bfpri, const TraceContext& tc );
ERR ErrBFWriteLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf, const BFPriority bfpri, const TraceContext& tc, BOOL* const pfCachedNewPage = NULL );

ERR ErrBFUpgradeReadLatchToRDWLatch( BFLatch* pbfl );
ERR ErrBFUpgradeReadLatchToWARLatch( BFLatch* pbfl );
ERR ErrBFUpgradeReadLatchToWriteLatch( BFLatch* pbfl, const BOOL fCOWAllowed = fTrue );
ERR ErrBFUpgradeRDWLatchToWARLatch( BFLatch* pbfl );
ERR ErrBFUpgradeRDWLatchToWriteLatch( BFLatch* pbfl );

void BFDowngradeWriteLatchToRDWLatch( BFLatch* pbfl );
void BFDowngradeWARLatchToRDWLatch( BFLatch* pbfl );
void BFDowngradeWriteLatchToReadLatch( BFLatch* pbfl );
void BFDowngradeWARLatchToReadLatch( BFLatch* pbfl );
void BFDowngradeRDWLatchToReadLatch( BFLatch* pbfl );

void BFWriteUnlatch( BFLatch* pbfl );
void BFWARUnlatch( BFLatch* pbfl );
void BFRDWUnlatch( BFLatch* pbfl );
void BFReadUnlatch( BFLatch* pbfl );

DWORD_PTR BFGetLatchHint( const BFLatch* pbfl );

void BFMarkAsSuperCold( IFMP ifmp, PGNO pgno, const BFLatchFlags bflf = bflfDefault );
void BFMarkAsSuperCold( BFLatch* pbfl );

enum BFDirtyFlags;
void BFCacheStatus( const IFMP ifmp, const PGNO pgno, BOOL* const pfInCache, ERR* const perrBF = NULL, BFDirtyFlags* const pbfdf = NULL );
BOOL FBFInCache( const IFMP ifmp, const PGNO pgno );
BOOL FBFPreviouslyCached( const IFMP ifmp, const PGNO pgno );

#ifdef DEBUG

BOOL FBFReadLatched( const BFLatch* pbfl );
BOOL FBFNotReadLatched( const BFLatch* pbfl );
BOOL FBFReadLatched( IFMP ifmp, PGNO pgno );
BOOL FBFNotReadLatched( IFMP ifmp, PGNO pgno );

BOOL FBFRDWLatched( const BFLatch* pbfl );
BOOL FBFNotRDWLatched( const BFLatch* pbfl );
BOOL FBFRDWLatched( IFMP ifmp, PGNO pgno );
BOOL FBFNotRDWLatched( IFMP ifmp, PGNO pgno );

BOOL FBFWARLatched( const BFLatch* pbfl );
BOOL FBFNotWARLatched( const BFLatch* pbfl );
BOOL FBFWARLatched( IFMP ifmp, PGNO pgno );
BOOL FBFNotWARLatched( IFMP ifmp, PGNO pgno );

BOOL FBFWriteLatched( const BFLatch* pbfl );
BOOL FBFNotWriteLatched( const BFLatch* pbfl );
BOOL FBFWriteLatched( IFMP ifmp, PGNO pgno );
BOOL FBFNotWriteLatched( IFMP ifmp, PGNO pgno );

BOOL FBFLatched( const BFLatch* pbfl );
BOOL FBFNotLatched( const BFLatch* pbfl );
BOOL FBFLatched( IFMP ifmp, PGNO pgno );
BOOL FBFNotLatched( IFMP ifmp, PGNO pgno );

BOOL FBFCurrentLatch( const BFLatch* pbfl, IFMP ifmp, PGNO pgno );
BOOL FBFCurrentLatch( const BFLatch* pbfl );

BOOL FBFUpdatableLatch( const BFLatch* pbfl );

#endif




ERR ErrBFLatchStatus( const BFLatch * pbfl );

enum BFDirtyFlags
{
    bfdfMin     = 0,
    bfdfClean   = 0,
    bfdfUntidy  = 1,
    bfdfDirty   = 2,
    bfdfFilthy  = 3,
    bfdfMax     = 4,
};

void BFInitialize( BFLatch* pbfl, const TraceContext& tc );
void BFDirty( const BFLatch* pbfl, BFDirtyFlags bfdf, const TraceContext& tc );
BFDirtyFlags FBFDirty( const BFLatch* pbfl );


LONG CbBFGetBufferSize( const LONG cbSize );
LONG CbBFBufferSize( const BFLatch* pbfl );
LONG CbBFPageSize( const BFLatch* pbfl );
void BFSetBufferSize( __inout BFLatch* pbfl, __in const INT cbNewSize );


void BFSetLgposModify( const BFLatch* pbfl, LGPOS lgpos );
void BFSetLgposBegin0( const BFLatch* pbfl, LGPOS lgpos, const TraceContext& tc );


void BFRenouncePage( BFLatch* const pbfl, const BOOL fRenounceDirty = fFalse );
void BFAbandonNewPage( BFLatch* const pbfl, const TraceContext& tc );

ERR ErrBFPreparePageRangeForExternalZeroing( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg, const TraceContext& tc );
CPG CpgBFGetOptimalLockPageRangeSizeForExternalZeroing( const IFMP ifmp );
ERR ErrBFLockPageRangeForExternalZeroing( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg, const BOOL fTrimming, const TraceContext& tc, _Out_ DWORD_PTR* const pdwContext );
void BFPurgeLockedPageRangeForExternalZeroing( const DWORD_PTR dwContext, const TraceContext& tc );
void BFUnlockPageRangeForExternalZeroing( const DWORD_PTR dwContext, const TraceContext& tc );


void BFAddUndoInfo( const BFLatch* pbfl, RCE* prce );
void BFRemoveUndoInfo( RCE* const prce, const LGPOS lgposModify = lgposMin );


ERR ErrBFPatchPage(
    __in                        const IFMP      ifmp,
    __in                        const PGNO      pgno,
    __in_bcount( cbToken )      const void *    pvToken,
    __in                        const INT       cbToken,
    __in_bcount( cbPageImage )  const void *    pvPageImage,
    __in                        const INT       cbPageImage );



enum BFAllocState
{
    bfasMin             = 0,
    bfasTemporary       = 0,
    bfasForDisk         = 1,
    bfasIndeterminate   = 2,
    bfasMax             = 3,
};

void BFAlloc( __in_range( bfasMin, bfasMax - 1 ) const BFAllocState bfas, void** ppv, INT cbBufferSize = 0 );
void BFFree( void* pv );



ERR ErrBFConfigureProcessForCrashDump( const JET_GRBIT grbit );
ERR ErrBFTestEvictPage( _In_ const IFMP ifmp, _In_ const PGNO pgno );

class CPagePointer
{
public:
    CPagePointer() : m_dwPage( 0 )                      {}
    CPagePointer( const DWORD_PTR dw ) : m_dwPage( dw ) {}
    CPagePointer( const DWORD_PTR dw, PGNO pgno ) : 
        m_dwPage( dw ),
        m_pgno( pgno ) {}
    ~CPagePointer()                                     {}

    CPagePointer& operator=( const CPagePointer& pagepointer )
    {
        m_dwPage    = pagepointer.m_dwPage;
        m_pgno      = pagepointer.m_pgno;
        return *this;
    }

    const DWORD_PTR     DwPage() const  { return m_dwPage; }
    const PGNO          PgNo()   const  { return m_pgno; }

private:
    DWORD_PTR           m_dwPage;
    PGNO                m_pgno;
};

#endif

