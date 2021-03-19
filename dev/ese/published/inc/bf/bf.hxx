// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef BF_H_INCLUDED
#define BF_H_INCLUDED


// -----------------------------------------------------------------------------------------------
//
//      Global Cache Control
//

//  Init / Term

ERR ErrBFInit( _In_ const LONG cbPageSizeMax );
void BFTerm();

//  System Parameters

ERR ErrBFGetCacheSize( ULONG_PTR* const pcpg );
ERR ErrBFSetCacheSize( const ULONG_PTR cpg );
ERR ErrBFSetCacheSizeRange();

LONG LBFICacheCleanPercentage( void );
LONG LBFICacheSizePercentage( void );
LONG LBFICachePinnedPercentage( void );

//  Interogation of background/idle (or maint in BF lingo) work status

ERR ErrBFCheckMaintAvailPoolStatus();



// -----------------------------------------------------------------------------------------------
//
//      BF Context Control
//

//  Context Control - Active State

void BFSetBFFMPContextAttached( IFMP ifmp );
void BFResetBFFMPContextAttached( IFMP ifmp );

//  Context Control - System Parameters

enum BFConsumeSetting
{
    bfcsCacheSize       = 0x00000001,
    bfcsCheckpoint      = 0x00000002,
};

ERR ErrBFConsumeSettings( BFConsumeSetting bfcs, const IFMP ifmp );

//  Context Control - Purge / Flush

ERR ErrBFFlush( IFMP ifmp, const OBJID objidFDP = objidNil, const PGNO pgnoFirst = pgnoNull, const PGNO pgnoLast = pgnoNull );
ERR ErrBFFlushSync( IFMP ifmp );
void BFPurge( IFMP ifmp, PGNO pgnoFirst = pgnoNull, CPG cpg = 0 );

//  Context - Logging / Recovery Support

void BFGetBestPossibleWaypoint(
    _In_    IFMP        ifmp,
    _In_    const LONG  lgenCommitted,
    _Out_   LGPOS *     plgposBestWaypoint );
void BFGetLgposOldestBegin0( IFMP ifmp, LGPOS* plgpos, LGPOS lgposOldestTrx );


// -----------------------------------------------------------------------------------------------
//
//      BF Priority Hints
//

//  BFPriority is a unified priority enum that contains the priority for the resource in the 
//  eviction algorithm as well as the IO priority with with to fault in the resource if not
//  present in the cache.

const ULONG shfFaultIoPriority = 9;

enum BFPriority // bfpri
{
    bfpriCacheResourcePriorityMask     = 0x000003FF,  // enough to hold 1 - 1000%, the range of cache resource percentages that we support.
    bfpriFaultIoPriorityMask           = ( ( qosIODispatchMask | qosIOOSLowPriority ) << shfFaultIoPriority ),
    bfpriUserIoPriorityTagMask         = JET_IOPriorityUserClassIdMask | JET_IOPriorityMarkAsMaintenance,
};
// Visually align / check exclusive masks:
C_ASSERT( bfpriFaultIoPriorityMask    == 0x00FEE400 );  // lined up with above.
C_ASSERT( bfpriUserIoPriorityTagMask  == 0x4F000000 );

// Double check with math, the masks are exclusive.
C_ASSERT( ( bfpriCacheResourcePriorityMask & bfpriFaultIoPriorityMask ) == 0 );
C_ASSERT( ( bfpriCacheResourcePriorityMask & bfpriUserIoPriorityTagMask ) == 0 );
C_ASSERT( ( bfpriFaultIoPriorityMask & bfpriUserIoPriorityTagMask ) == 0 );

C_ASSERT( ( ~bfpriFaultIoPriorityMask & ( ( qosIODispatchMask | qosIOOSLowPriority ) << shfFaultIoPriority ) ) == 0 );  //  ensure no Dispatch bits are outside the bfpri mask.

enum BFTEMPOSFILEQOS : QWORD
{
    doNotUseThis = 0xFFFFFFFF,
};

INLINE BFPriority BfpriBFMake( const ULONG_PTR pctCachePriority, const BFTEMPOSFILEQOS qosPassed )
{
    OSFILEQOS qosIoPriority = qosPassed;
#ifndef ENABLE_JET_UNIT_TEST // unit test attempts bad values
    Assert( pctCachePriority <= 1000 );
    Assert( ( pctCachePriority & ~bfpriCacheResourcePriorityMask ) == 0 ); // should be no remaining data outside mask.
#endif

    Assert( ( ( ( qosIoPriority & qosIODispatchMask ) << shfFaultIoPriority ) & ~bfpriFaultIoPriorityMask ) == 0 ); // should be no remaining bits outside mask.

#ifndef ENABLE_JET_UNIT_TEST // unit test attempts bad values
    Assert( ( qosIoPriority & ~qosIODispatchMask & ~qosIOOSLowPriority & ~bfpriUserIoPriorityTagMask ) == 0 ); // should be no remaining bits outside UserTag, Dispatch priority, or OS Low Pri.
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


// -----------------------------------------------------------------------------------------------
//
//          Preread
//

enum BFPreReadFlags     //  bfprf
{
    bfprfNone               = 0x00000000,
    bfprfDefault            = 0x00000000,

    bfprfNoIssue            = 0x00000100,   //  don't call pfapi->ErrIOIssue(), caller must do it
    bfprfCombinableOnly     = 0x00000200,   //  only pre-read if the IO operation is combinable with an already building IO
    bfprfDBScan             = 0x00000400,   //  pre-read operation is coming from DBM
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

//  This class can be used to cooperatively reserve a number of pages
//  from the avail pool. This can be used by preread functions. Each
//  object reserved up to cpgWanted pages from the avail pool and other
//  objects will respect that reservation.

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
        // non-implemented functions. these objects should be allocated off the
        // stack using the public constructor

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


// -----------------------------------------------------------------------------------------------
//
//      Page Latches
//

struct BFLatch  //  bfl
{
    void*       pv;
    DWORD_PTR   dwContext;
};

enum BFLatchFlags  //  bflf
{
    bflfNone            = 0x00000000,
    bflfDefault         = 0x00000000,

    bflfNoTouch         = 0x00000100,       //  don't touch the page
    bflfNoWait          = 0x00000200,       //  don't wait to resolve latch conflicts
    bflfNoCached        = 0x00000400,       //  don't latch cached pages
    bflfNoUncached      = 0x00000800,       //  don't latch uncached pages
    bflfNoFaultFail     = 0x00001000,       //  don't unlatch pages that fail IO or page validation
                                            //    NOTE:  error should be retrieved with ErrBFLatchStatus()
                                            //    consider: separating to bflfNoIoFail and bflfNoValidationFail cases.
    bflfNew             = 0x00002000,       //  latch a new page (Write Latch only)
    bflfHint            = 0x00004000,       //  the provided BFLatch may already point to the
                                            //    desired IFMP / PGNO
    bflfNoEventLogging  = 0x00008000,       //  don't log events for -1018/-1019 errors
    bflfUninitPageOk    = 0x00010000,       //  -1019 errors are expected, don't log an event
    bflfExtensiveChecks = 0x00020000,       //  do more extensive validation of the page

    bflfDBScan          = 0x00040000,       //  we're latching a page during DBM
    bflfNewIfUncached   = 0x00080000,       //  always latches the page, but behaves like bflfNew if the page is not currently cached
    bflfLatchAbandoned  = 0x00100000,       //  it's legal to latch an abandoned page
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

//  Use only use when latched.
BOOL FBFCurrentLatch( const BFLatch* pbfl, IFMP ifmp, PGNO pgno );  // this one checks for the most truthiness.
BOOL FBFCurrentLatch( const BFLatch* pbfl );

//  Use only when latch is RDW or higher latched.
BOOL FBFUpdatableLatch( const BFLatch* pbfl );

#endif  //  DEBUG


// -----------------------------------------------------------------------------------------------
//
//      Page State and Control
//

//  Page State

ERR ErrBFLatchStatus( const BFLatch * pbfl );

enum BFDirtyFlags  //  bfdf
{
    bfdfMin     = 0,
    bfdfClean   = 0,        //  the page will not be written
    bfdfUntidy  = 1,        //  the page will be written only when idle
    bfdfDirty   = 2,        //  the page will be written only when necessary
    bfdfFilthy  = 3,        //  the page will be written as soon as possible
    bfdfMax     = 4,
};

void BFInitialize( BFLatch* pbfl, const TraceContext& tc );
void BFDirty( const BFLatch* pbfl, BFDirtyFlags bfdf, const TraceContext& tc );
BFDirtyFlags FBFDirty( const BFLatch* pbfl );

//  Page Size and Buffer Management

LONG CbBFGetBufferSize( const LONG cbSize );
//      Note: Subtle difference between these two, one is the size of the page we 
//      cached, the other is the (potentially) dehydrated size.
LONG CbBFBufferSize( const BFLatch* pbfl );
LONG CbBFPageSize( const BFLatch* pbfl );
void BFSetBufferSize( __inout BFLatch* pbfl, _In_ const INT cbNewSize );

//  Logging / Recovery

void BFSetLgposModify( const BFLatch* pbfl, LGPOS lgpos );
void BFSetLgposBegin0( const BFLatch* pbfl, LGPOS lgpos, const TraceContext& tc );

//  Renounce / Abandon

void BFRenouncePage( BFLatch* const pbfl, const BOOL fRenounceDirty = fFalse );
void BFAbandonNewPage( BFLatch* const pbfl, const TraceContext& tc );

//  Range-locking for external zeroing.
ERR ErrBFPreparePageRangeForExternalZeroing( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg, const TraceContext& tc );
CPG CpgBFGetOptimalLockPageRangeSizeForExternalZeroing( const IFMP ifmp );
ERR ErrBFLockPageRangeForExternalZeroing( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg, const BOOL fTrimming, const TraceContext& tc, _Out_ DWORD_PTR* const pdwContext );
void BFPurgeLockedPageRangeForExternalZeroing( const DWORD_PTR dwContext, const TraceContext& tc );
void BFUnlockPageRangeForExternalZeroing( const DWORD_PTR dwContext, const TraceContext& tc );

//  Deferred Undo Information

void BFAddUndoInfo( const BFLatch* pbfl, RCE* prce );
void BFRemoveUndoInfo( RCE* const prce, const LGPOS lgposModify = lgposMin );

//  Page Patching

ERR ErrBFPatchPage(
    _In_                        const IFMP      ifmp,
    _In_                        const PGNO      pgno,
    __in_bcount( cbToken )      const void *    pvToken,
    _In_                        const INT       cbToken,
    __in_bcount( cbPageImage )  const void *    pvPageImage,
    _In_                        const INT       cbPageImage );


// -----------------------------------------------------------------------------------------------
//
//      Memory Allocation
//

enum BFAllocState  //  bfas
{
    bfasMin             = 0,
    bfasTemporary       = 0,        //  allocate and free on same thread, CPU bound, temporary
    bfasForDisk         = 1,        //  allocate for writing / updating disk, semi-temporary
    bfasIndeterminate   = 2,        //  allocate for an unbounded length of time
    bfasMax             = 3,
};

void BFAlloc( __in_range( bfasMin, bfasMax - 1 ) const BFAllocState bfas, void** ppv, INT cbBufferSize = 0 );
void BFFree( void* pv );


// -----------------------------------------------------------------------------------------------
//
//      Diagnostics / Debug / Test Support
//

ERR ErrBFConfigureProcessForCrashDump( const JET_GRBIT grbit );
ERR ErrBFTestEvictPage( _In_ const IFMP ifmp, _In_ const PGNO pgno );

//  this class is required so that CArray can be used
//  to store an array of pointers
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

#endif  //  BF_H_INCLUDED

