// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _BF_HXX_INCLUDED
#define _BF_HXX_INCLUDED

#include "resmgr.hxx"

#include "_bfconst.hxx"

#include "bfftl.hxx"

typedef LONG_PTR IPG;
const IPG ipgNil = IPG( -1 );

struct IFMPPGNO                                     //  IFMPPGNO -- Page Key
{
    IFMPPGNO() {}
    IFMPPGNO( IFMP ifmpIn, PGNO pgnoIn )
        :   ifmp( ifmpIn ),
            pgno( pgnoIn )
    {
    }

    BOOL operator==( const IFMPPGNO& ifmppgno ) const
    {
        return ifmp == ifmppgno.ifmp && pgno == ifmppgno.pgno;
    }

    BOOL operator<( const IFMPPGNO& ifmppgno ) const
    {
        return ( Cmp( this, &ifmppgno ) < 0 );
    }

    BOOL operator>( const IFMPPGNO& ifmppgno ) const
    {
        return ( Cmp( this, &ifmppgno ) > 0 );
    }

    const IFMPPGNO& operator=( const IFMPPGNO& ifmppgno )
    {
        ifmp = ifmppgno.ifmp;
        pgno = ifmppgno.pgno;

        return *this;
    }

    static INT Cmp( const IFMPPGNO * pifmppgno1, const IFMPPGNO * pifmppgno2 )
    {
        const INT cmp   = (INT)( pifmppgno1->ifmp - pifmppgno2->ifmp );
        return ( 0 != cmp ? cmp : (INT)( pifmppgno1->pgno - pifmppgno2->pgno ) );
    }

    ULONG_PTR Hash() const;

    IFMP    ifmp;
    PGNO    pgno;
};


// CAtomicBfBitField class

class CAtomicBfBitField
{
    private:
        typedef union
        {
            // DWORD that contains the bits.
            FLAG32 bits;

            // Actual bits.
            struct
            {
                FLAG32 FDependentPurged:1;      //  BF we were dependent on has been purged.
                FLAG32 FImpedingCheckpoint:1;   //  BF where someone dependant upon this page
                                                //  is impeding the checkpoint.
                FLAG32 FRangeLocked:1;          //  BF is range-locked.
                FLAG32 rgbitReserved:29;        //  Free space.
            };
        } BfBitField;

        BfBitField m_bfbf;

        // Declaration macro.
        #define BitFieldDecl( _Type, _Name )                                                    \
        inline _Type _Name() const                                                              \
        {                                                                                       \
            BfBitField bfbf;                                                                    \
            bfbf.bits = (FLAG32)AtomicRead( (LONG*)&m_bfbf.bits );                              \
            return (_Type)( bfbf._Name );                                                       \
        }                                                                                       \
                                                                                                \
        inline void Set##_Name( const _Type _Name )                                             \
        {                                                                                       \
            OSSYNC_FOREVER                                                                      \
            {                                                                                   \
                BfBitField bfbfInitial, bfbfFinal;                                              \
                bfbfInitial.bits = bfbfFinal.bits = (FLAG32)AtomicRead( (LONG*)&m_bfbf.bits );  \
                bfbfFinal._Name = _Name;                                                        \
                if ( AtomicCompareExchange(                                                     \
                        (LONG*)&m_bfbf.bits,                                                    \
                        (LONG)bfbfInitial.bits,                                                 \
                        (LONG)bfbfFinal.bits ) == (LONG)bfbfInitial.bits )                      \
                {                                                                               \
                    break;                                                                      \
                }                                                                               \
            }                                                                                   \
        }

    public:
        // Initialize all bits with zero.
        CAtomicBfBitField()
        {
            m_bfbf.bits = 0;
        }

        // Getter/setter declaration.
        BitFieldDecl( BOOL, FDependentPurged );
        BitFieldDecl( BOOL, FImpedingCheckpoint );
        BitFieldDecl( BOOL, FRangeLocked );
};
C_ASSERT( sizeof( CAtomicBfBitField ) == sizeof( FLAG32 ) );


//  BF struct

struct BF;
typedef BF* PBF;
const PBF pbfNil = PBF( 0 );
typedef LONG_PTR IBF;
const IBF ibfNil = IBF( -1 );
typedef IBF CBF;

struct BF                                           //  BF  --  IFMP/PGNO buffer state
{
    BF()
        :   ifmp( ifmpNil ),
            err( JET_errSuccess ),
            fNewlyEvicted( fFalse ),
            fQuiesced( fTrue ),
            fAvailable( fFalse ),
            fWARLatch( fFalse ),
            bfdf( bfdfClean ),
            fInOB0OL( fFalse ),
            irangelock( 0 ),
            fCurrentVersion( fFalse ),
            fOlderVersion( fFalse ),
            fFlushed( fFalse ),
            bfls( bflsNormal ),
            sxwl( CLockBasicInfo( CSyncBasicInfo( szBFLatch ), rankBFLatch, CLockDeadlockDetectionInfo::subrankNoDeadlock ) ),
            lgposOldestBegin0( lgposMax ),
            lgposModify( lgposMin ),
            rbsposSnapshot( rbsposMin ),
            prceUndoInfoNext( prceNil ),
            pgno( pgnoNull ),
            tce( tceNone ),
            pbfTimeDepChainPrev( pbfNil ),
            pbfTimeDepChainNext( pbfNil ),
            pv( NULL ),
            bfrs( bfrsNotCommitted ),
            fLazyIO( fFalse ),
            pWriteSignalComplete( NULL ), // and thus pbfNext( NULL )
            icbPage( icbPageInvalid ),
            icbBuffer( icbPageInvalid ),
            fSuspiciouslySlowRead( fFalse ),
            fSyncRead( fFalse ),
            bfat( bfatNone ),
            fAbandoned( fFalse ),
            pvIOContext( NULL )
    {
        const TICK tickNow = TickOSTimeCurrent();

        tickEligibleForNomination = tickNow;
        tickLastDirtied = tickNow;
    }

    ~BF()
    {
    }

    void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;   //  dumps BF state

    static SIZE_T OffsetOfLRUKIC()      { return OffsetOf( BF, lrukic ); }

    static SIZE_T OffsetOfAPIC()        { return OffsetOf( BF, lrukic ); }
    C_ASSERT( sizeof( CPool< BF, BF::OffsetOfAPIC >::CInvasiveContext ) <= sizeof( CLRUKResourceUtilityManager< Kmax, BF, OffsetOfLRUKIC, IFMPPGNO >::CInvasiveContext ) );

    static SIZE_T OffsetOfQPIC()        { return OffsetOf( BF, lrukic ); }
    C_ASSERT( sizeof( CInvasiveList< BF, BF::OffsetOfQPIC >::CElement ) <= sizeof( CLRUKResourceUtilityManager< Kmax, BF, OffsetOfLRUKIC, IFMPPGNO >::CInvasiveContext ) );

    static SIZE_T OffsetOfOB0IC()       { return OffsetOf( BF, ob0ic ); }

    static SIZE_T OffsetOfOB0OLILE()    { return OffsetOf( BF, ob0ic ); }
    C_ASSERT( sizeof( CInvasiveList< BF, BF::OffsetOfOB0OLILE >::CElement ) <= sizeof( CApproximateIndex< LGPOS, BF, OffsetOfOB0IC >::CInvasiveContext ) );

#ifdef _WIN64

    //   0 B /////////////////////////////////////////////////////////////////////////////////////

    //  UNION: This is a union, but hidden via the OffsetOfOB0IC() / OffsetOfOB0OLILE() functions.
    CApproximateIndex< LGPOS, BF, OffsetOfOB0IC >::CInvasiveContext ob0ic;
                                                    //  Invasive Context for the OldestBegin0 index or
                                                    //    Invasive List Element for the Overflow List

    LGPOS               lgposOldestBegin0;          //  log position of Begin0 of the oldest
                                                    //    transaction to dirty this IFMP/PGNO

    LGPOS               lgposModify;                //  log position of most recent log record
                                                    //    to reference this IFMP/PGNO

    //  32 B //////////////////////////////////////////////////////////////////////////////////////

    CSXWLatch           sxwl;                       //  S/X/W Latch protecting this BF state and
                                                    //    its associated cached page

    //  64 B //////////////////////////////////////////////////////////////////////////////////////

    IFMP                ifmp;                       //  IFMP of this cached page
    PGNO                pgno;                       //  PGNO of this cached page

    TICK                tickLastDirtied;            //  time when the BF was last dirtied (unique pages modified)

    SHORT               err;                        //  I/O error
    BYTE                fLazyIO:1;                  //  BF is issued at non-immediate dispatch priority
    BYTE                fNewlyEvicted:1;            //  BF cache memory is newly evicted
    BYTE                fQuiesced:1;                //  BF is quiesced for shrinking the cache
    BYTE                fAvailable:1;               //  BF is in the avail pool
    BYTE                fReserved4:1;               //  Available for re-use (was fMemory)
    BYTE                fWARLatch:1;                //  BF is WAR Latched (valid only if exclusively latched)
    BYTE                bfdf:2;                     //  BF dirty flags
    BYTE                fInOB0OL:1;                 //  BF is in the Oldest Begin 0 index Overflow List
    BYTE                irangelock:1;               //  active rangelock for this attempted flush
    BYTE                fCurrentVersion:1;          //  BF contains the current version of this IFMP / PGNO
    BYTE                fOlderVersion:1;            //  BF contains an older version of this IFMP / PGNO
    BYTE                fFlushed:1;                 //  BF has been successfully flushed at least once
    BYTE                bfls:3;                     //  BF latch state

    union
    {
        ULONG           iHashedLatch;               //  bflsHashed:     offset of hashed latch in PLS
        TICK            tickEligibleForNomination;  //  !bflsHashed:    time BF is eligible for nomination
        TICK            tickViewLastRefreshed;      //  [bfat==bfatViewMapped]: tick of the last time all pages were definitely read (typically within an exception handler)
    };

    BFResidenceState    bfrs;                       //  BF residence state

    void*               pv;                         //  Cached page image

    //  96 B //////////////////////////////////////////////////////////////////////////////////////

    union
    {
        volatile ULONG_PTR  pWriteSignalComplete;   //  write IO completion signal/information
//      volatile PBF    pbfNext;                    //  future, this will be used for evict immediate
    };

    //  Important: Next 4 bytes protected from modify x-latch, can be read locklessly
    BYTE                icbPage:4;                  //  Index into g_rgcbPageSize[] for the size of the page
    BYTE                icbBuffer:4;                //  Index into g_rgcbPageSize[] for the CURRENT size of the buffer (dehydrated)

    BYTE                fSuspiciouslySlowRead:1;    //  BF is for IO that has a suspiciously slow read
    BYTE                fSyncRead:1;                //  BF was read synchronously (otherwise BF was read async)
    BYTE                bfat:2;                     //  Method used to allocate the pv buffer
    BYTE                fAbandoned:1;               //  Client abandoned this page, ok to purge / evict with dirty data immediately
    BYTE                grbitReserved:3;

    BYTE                rgbReserved2[ 1 ];          //  Free space

    TCE                 tce;                        //  table class for perfmon tracking of pages

    CAtomicBfBitField   bfbitfield;                 //  Bit field that can be read from and written to without any latches.

    volatile PBF        pbfTimeDepChainPrev;        //  prev BF in our time dependency chain
    volatile PBF        pbfTimeDepChainNext;        //  next BF in our time dependency chain

    //  128 B /////////////////////////////////////////////////////////////////////////////////////

    RCE*                prceUndoInfoNext;           //  Undo Info chain

    void*               pvIOContext;                //  I/O context (in practice, an IOREQ)

    //  144 B /////////////////////////////////////////////////////////////////////////////////////

    CLRUKResourceUtilityManager< Kmax, BF, OffsetOfLRUKIC, IFMPPGNO >::CInvasiveContext lrukic;
                                                    //  Invasive Context the LRUK Resource Utility Manager, Avail pool
                                                    //  or Quiesced list.

    //  184 B /////////////////////////////////////////////////////////////////////////////////////


    RBS_POS             rbsposSnapshot;           //  Position of the rollback snapshot containing preimage for this page

    // 192 B (64-bit/cache aligned) ///////////////////////////////////////////////////////////////

#else  //  !_WIN64

    //   0 B /////////////////////////////////////////////////////////////////////////////////////

    CApproximateIndex< LGPOS, BF, OffsetOfOB0IC >::CInvasiveContext ob0ic;
                                                    //  Invasive Context for the OldestBegin0 index or
                                                    //    Invasive List Element for the Overflow List

    LGPOS               lgposOldestBegin0;          //  log position of Begin0 of the oldest
                                                    //    transaction to dirty this IFMP/PGNO

    LGPOS               lgposModify;                //  log position of most recent log record
                                                    //    to reference this IFMP/PGNO

    volatile PBF        pbfTimeDepChainPrev;        //  prev BF in our time dependency chain
    volatile PBF        pbfTimeDepChainNext;        //  next BF in our time dependency chain (aka "older")

    //  32 B //////////////////////////////////////////////////////////////////////////////////////

    CSXWLatch           sxwl;                       //  S/X/W Latch protecting this BF state and
                                                    //    its associated cached page

    void*               pvIOContext;                //  I/O context (in practice, an IOREQ)

    IFMP                ifmp;                       //  IFMP of this cached page
    PGNO                pgno;                       //  PGNO of this cached page

    //  64 B //////////////////////////////////////////////////////////////////////////////////////

    void*               pv;                         //  Cached page image

    SHORT               err;                        //  I/O error
    BYTE                fLazyIO:1;                  //  BF is issued at non-immediate dispatch priority
    BYTE                fNewlyEvicted:1;            //  BF cache memory is newly evicted
    BYTE                fQuiesced:1;                //  BF is quiesced for shrinking the cache
    BYTE                fAvailable:1;               //  BF is in the avail pool
    BYTE                fReserved4:1;               //  Available for re-use (was fMemory)
    BYTE                fWARLatch:1;                //  BF is WAR Latched (valid only if exclusively latched)
    BYTE                bfdf:2;                     //  BF dirty flags
    BYTE                fInOB0OL:1;                 //  BF is in the Oldest Begin 0 index Overflow List
    BYTE                irangelock:1;               //  active rangelock for this attempted flush
    BYTE                fCurrentVersion:1;          //  BF contains the current version of this IFMP / PGNO
    BYTE                fOlderVersion:1;            //  BF contains an older version of this IFMP / PGNO
    BYTE                fFlushed:1;                 //  BF has been successfully flushed at least once
    BYTE                bfls:3;                     //  BF latch state

    union
    {
        ULONG           iHashedLatch;               //  bflsHashed:     offset of hashed latch in PLS
        TICK            tickEligibleForNomination;  //  !bflsHashed:    time BF is eligible for nomination
        TICK            tickViewLastRefreshed;      //  [bfat==bfatViewMapped]: tick of the last time all pages were definitely read (typically within an exception handler)
    };

    BFResidenceState    bfrs;                       //  BF residence state

    //  80 B //////////////////////////////////////////////////////////////////////////////////////

    RCE*                prceUndoInfoNext;           //  Undo Info chain

    CAtomicBfBitField   bfbitfield;                 //  Bit field that can be read from and written to without any latches.

    //  88 B //////////////////////////////////////////////////////////////////////////////////////

    CLRUKResourceUtilityManager< Kmax, BF, OffsetOfLRUKIC, IFMPPGNO >::CInvasiveContext lrukic;
                                                    //  Invasive Context the LRUK Resource Utility Manager, Avail pool
                                                    //  or Quiesced list.
    //  120 B /////////////////////////////////////////////////////////////////////////////////////

    union
    {
        volatile ULONG_PTR  pWriteSignalComplete;   //  write IO completion signal/information
//      volatile PBF    pbfNext;                    //  future, this will be used for evict immediate
    };

    //  Important: Next 4 bytes protected from modify x-latch, can be read locklessly

    BYTE                icbPage:4;                  //  Index into g_rgcbPageSize[] for the size of the page
    BYTE                icbBuffer:4;                //  Index into g_rgcbPageSize[] for the CURRENT size of the buffer (dehydrated)

    BYTE                fSuspiciouslySlowRead:1;    //  BF is for IO that has a suspiciously slow read
    BYTE                fSyncRead:1;                //  BF was read synchronously (otherwise BF was read async)
    BYTE                bfat:2;                     //  Method used to allocate the pv buffer
    BYTE                fAbandoned:1;               //  Client abandoned this page, ok to purge / evict with dirty data immediately
    BYTE                grbitReserved:3;

    BYTE                rgbReserved1[ 1 ];          //  Free space

    TCE                 tce;                        //  table class for perfmon tracking of pages

    // 128 B (64-bit/cache aligned) //////////////////////////////////////////////////////////////

    RBS_POS             rbsposSnapshot;             //  Position of the rollback snapshot containing preimage for this page

    TICK                tickLastDirtied;            //  time when the BF was last dirtied (unique pages modified)

    BYTE                rgbReserved3[ 20 ];         //  Free space

    // 160 B (64-bit/cache aligned) ///////////////////////////////////////////////////////////////

#endif  //  _WIN64
};

// The CInvasiveList class will return NULL for the Prev()/Next() pointers that are returned IF it gets
// to the end AND IF (ONLY IF) the invasive context is at the beginning of (offset 0) the datastructure.  
// BUT IF the invasive context is not at the beginning of the datastructure the experience of the Prev()
// or Next() pointers are return as this very odd / unpleasant negative offset.
C_ASSERT( OffsetOf( BF, ob0ic ) == 0 );

// Atomic bit fields must be DWORD-aligned so that we can use interlocked operations to read from and write to them
// consistently.
C_ASSERT( ( OffsetOf( BF, bfbitfield ) % sizeof( FLAG32 ) ) == 0 );
C_ASSERT( sizeof( BF::bfbitfield ) == sizeof( FLAG32 ) );

//  Be conscious of the size if you're changing it ...
#ifdef _WIN64
C_ASSERT( sizeof(BF) == 192 );
#else  //  !_WIN64
C_ASSERT( sizeof(BF) == 160 );
#endif  //  _WIN64

//  Buffer Manager Global Flags

extern BOOL     g_fBFInitialized;

extern BYTE*    g_rgbBFTemp;

extern TICK     g_tickBFPreparingCrashDump;
extern size_t   cBFInspectedForInclusionInCrashDump;
extern size_t   cBFMismatchedVMPageIncludedInCrashDump;
extern size_t   cBFDirtiedPageIncludedInCrashDump;
extern size_t   cBFCachedPageIncludedInCrashDump;
extern size_t   cBFLatchedPageIncludedInCrashDump;
extern size_t   cBFReferencedPageIncludedInCrashDump;
extern size_t   cBFRecentlyTouchedPageIncludedInCrashDump;
extern size_t   cBFErrorIncludedInCrashDump;
extern size_t   cBFIOIncludedInCrashDump;
extern size_t   cBFUnverifiedIncludedInCrashDump;
extern size_t   cBFMayBeRemovedFromCrashDump;
extern size_t   cBFVMPagesIncludedInCrashDump;
extern size_t   cBFVMPagesRemovedFromCrashDump;
extern TICK     g_tickBFCrashDumpPrepared;
extern ERR      g_errBFCrashDumpResult;
extern BOOL     g_fBFErrorBuildingReferencedPageListForCrashDump;

//  Buffer Manager Global Statistics

extern ULONG cBFOpportuneWriteIssued;


//  Buffer Manager Global Constants

extern double   g_dblBFSpeedSizeTradeoff;


//  IFMP/PGNO Hash Table

struct PGNOPBF
{
    PGNOPBF() {}
    PGNOPBF( PGNO pgnoIn, PBF pbfIn )
        :   pgno( pgnoIn ),
            pbf( pbfIn )
    {
    }

    BOOL operator==( const PGNOPBF& pgnopbf ) const
    {
        return pbf == pgnopbf.pbf;  //  pbf alone uniquely identifies this entry
    }

    const PGNOPBF& operator=( const PGNOPBF& pgnopbf )
    {
        pgno    = pgnopbf.pgno;
        pbf     = pgnopbf.pbf;

        return *this;
    }

    PGNO    pgno;
    PBF     pbf;
};


typedef CDynamicHashTable< IFMPPGNO, PGNOPBF > BFHash;

inline BFHash::NativeCounter HashIfmpPgno( const IFMP ifmp, const PGNO pgno )
{
    //  CONSIDER:  revise this hash function

    return BFHash::NativeCounter( pgno + ( ifmp << 13 ) + ( pgno >> 17 ) );
}

inline BFHash::NativeCounter BFHash::CKeyEntry::Hash( const IFMPPGNO& ifmppgno )
{
    return HashIfmpPgno( ifmppgno.ifmp, ifmppgno.pgno );
}

inline BFHash::NativeCounter BFHash::CKeyEntry::Hash() const
{
    return HashIfmpPgno( m_entry.pbf->ifmp, m_entry.pgno );
}

inline BOOL BFHash::CKeyEntry::FEntryMatchesKey( const IFMPPGNO& ifmppgno ) const
{
    return m_entry.pgno == ifmppgno.pgno && m_entry.pbf->ifmp == ifmppgno.ifmp;
}

inline void BFHash::CKeyEntry::SetEntry( const PGNOPBF& pgnopbf )
{
    m_entry = pgnopbf;
}

inline void BFHash::CKeyEntry::GetEntry( PGNOPBF* const ppgnopbf ) const
{
    *ppgnopbf = m_entry;
}

// IFMPPGNO uses the same hash function
inline ULONG_PTR IFMPPGNO::Hash() const
{
    return (ULONG_PTR) HashIfmpPgno( this->ifmp, this->pgno );
}

extern BFHash g_bfhash;
extern double g_dblBFHashLoadFactor;
extern double g_dblBFHashUniformity;


//  Avail Pool

typedef CPool< BF, BF::OffsetOfAPIC > BFAvail;
extern BFAvail g_bfavail;

//  Quiesced List

typedef CInvasiveList< BF, BF::OffsetOfQPIC > BFQuiesced;
extern BFQuiesced g_bfquiesced;

//  lookaside cache for BFAlloc

class CSmallLookasideCache
{
public:
    CSmallLookasideCache();
    ~CSmallLookasideCache();

    void Init( const INT cbBufferSize );
    void Term();
    
    void * PvAlloc();
    void Free( void * const pb );
    INT CbBufferSize() const;

#ifdef DEBUGGER_EXTENSION
    //  This should be concurrently safe (though probably would give you a stale value).  But it 
    //  was intended only for debugger use, as it won't be very efficient as the lookaside list 
    //  is rather long.
    __int64 CbCacheSize()
    {
        __int64 cbTotal = 0;
        for( LONG i = 0; i < CSmallLookasideCache::m_cLocalLookasideBuffers; i++ )
        {
            if ( m_rgpvLocalLookasideBuffers[i] )
            {
                cbTotal += m_cbBufferSize;
            }
        }
        return cbTotal;
    }
#endif

private:
    INT m_cbBufferSize;

    static const INT m_cLocalLookasideBuffers = ( 128 /* large CPU cache line */ / sizeof(void*) ) * 16 /* 16 CPUs worst case */;

    void * m_rgpvLocalLookasideBuffers[m_cLocalLookasideBuffers];

    //  stats

#ifdef DEBUG
#define MEMORY_STATS_TRACKING
#endif
#ifdef MEMORY_STATS_TRACKING
    QWORD   m_cHits;
    QWORD   m_cAllocs;
    QWORD   m_cFrees;   // essentially overallocs
    QWORD   m_cFailures;
#endif

private:
    CSmallLookasideCache( const CSmallLookasideCache& );
    CSmallLookasideCache& operator=( const CSmallLookasideCache& );
};

#ifdef DEBUG
// Lookaside buffer for validating pages in the IO thread
extern void * g_pvIoThreadImageCheckCache;
#endif

//  LRUK

DECLARE_LRUK_RESOURCE_UTILITY_MANAGER( Kmax, BF, BF::OffsetOfLRUKIC, IFMPPGNO, BFLRUK );

extern BFLRUK g_bflruk;
extern double g_csecBFLRUKUncertainty;

//  BF FTL Tracing

ERR ErrBFIFTLInit();
void BFIFTLTerm();

//  BF tracing

INLINE void BFITraceResMgrInit(
    const INT       K,
    const double    csecCorrelatedTouch,
    const double    csecTimeout,
    const double    csecUncertainty,
    const double    dblHashLoadFactor,
    const double    dblHashUniformity,
    const double    dblSpeedSizeTradeoff );

INLINE void BFITraceResMgrTerm();

INLINE void BFITraceCachePage(
    const TICK                  tickCache,
    const PBF                   pbf,
    const BFLatchType           bflt,
    const ULONG                 pctPriority,
    const BFLatchFlags          bflf,
    const BFRequestTraceFlags   bfrtf,
    const TraceContext&         tc );

void BFITraceNewPageIdentity( const PBF pbf );

INLINE void BFITraceRequestPage(
    const TICK                  tickTouch,
    const PBF                   pbf,
    const ULONG                 pctPriority,
    const BFLatchType           bflt,
    const BFLatchFlags          bflf,
    const BFRequestTraceFlags   bfrtf,
    const TraceContext&         tc );

INLINE void BFITraceMarkPageAsSuperCold(
    const IFMP  ifmp,
    const PGNO  pgno );

INLINE void BFITraceEvictPage(
    const IFMP  ifmp,
    const PGNO  pgno,
    const BOOL  fCurrentVersion,
    const ERR   errBF,
    const ULONG bfef );

INLINE void BFITraceDirtyPage(
    const PBF               pbf,
    const BFDirtyFlags      bfdf,
    const TraceContext&     tc );

INLINE void BFITraceWritePage(
    const PBF               pbf,
    const FullTraceContext&     tc );

INLINE void BFITraceSetLgposModify(
    const PBF       pbf,
    const LGPOS&    lgposModify );


//  Oldest Begin 0 Index and Overflow List

DECLARE_APPROXIMATE_INDEX( QWORD, BF, BF::OffsetOfOB0IC, BFOB0 );
typedef CInvasiveList< BF, BF::OffsetOfOB0OLILE > BFOB0OverflowList;

QWORD BFIOB0Offset( const IFMP ifmp, const LGPOS* const plgpos );
INLINE LGPOS BFIOB0Lgpos( const IFMP ifmp, LGPOS lgpos, const BOOL fNextBucket = fFalse );


//
// structs to maintain a histogram of something vs log generation
// 
struct LogHistData
{
    LONG m_lgenBase;    // min log generation we hold for histogram
    LONG m_cgen;        // # of log generations in histogram

    LONG* m_rgc;        // histogram, m_rgc = new LONG[ m_cgenb ];
    LONG m_cOverflow;   // overflow bucket

    LogHistData( void )
        :   m_lgenBase( 0 ),
            m_cgen( 0 ),
            m_rgc( NULL ),
            m_cOverflow( 0 )
    {
    }
    ~LogHistData( void )
    {
        delete[] m_rgc;
        m_rgc = NULL;
    }
};

struct BFSTAT
{
    LONG m_cBFMod;      // modified BFs
    LONG m_cBFPin;      // pinned BFs

    BFSTAT( LONG cBFMod, LONG cBFPin )
        :   m_cBFMod( cBFMod ),
            m_cBFPin( cBFPin )
    {
    }
    BFSTAT( BFSTAT& copy )
    {
        m_cBFMod = copy.m_cBFMod;
        m_cBFPin = copy.m_cBFPin;
    }

    BFSTAT& operator=( BFSTAT& other )
    {
        m_cBFMod = other.m_cBFMod;
        m_cBFPin = other.m_cBFPin;
        return *this;
    }
};

struct BFLogHistogram
{
    CCriticalSection    m_crit;
    CMeteredSection m_ms;

    LogHistData     m_rgdata[ 2 ];

    BFLogHistogram( void )
        :   m_crit( CLockBasicInfo( CSyncBasicInfo( szBFLgposModifyHist ), rankBFLgposModifyHist, 0 ) )
    {
    }
    ~BFLogHistogram( void )
    {
    }

    VOID Update( const LGPOS lgposOld, const LGPOS lgposNew, IFMP ifmp );
    VOID ReBase( IFMP ifmp, LONG lgenLatest );
    static BFSTAT Read( void );

    enum { cgenNewMin = 64 };
};

struct BFFMPContext
{
    BFFMPContext()
        :   bfob0( rankBFOB0 ),
            critbfob0ol( CLockBasicInfo( CSyncBasicInfo( szBFOB0 ), rankBFOB0, 0 ) )
    {

        memset( &ChkAdvData, 0, sizeof(ChkAdvData) );
        tickMaintCheckpointDepthLast    = TickOSTimeCurrent();
        tickMaintCheckpointDepthNext    = tickMaintCheckpointDepthLast - dtickMaintCheckpointDepthDelay;
        errLastCheckpointMaint          = JET_errSuccess;
        lgposFlusherBM                  = lgposMin;
        lgposVersionerBM                = lgposMin;
        lgposLastLogTip                 = lgposMin;

        lgposOldestBegin0Last           = lgposMax;

        lgposNewestModify               = lgposMin;
        fCurrentlyAttached              = fFalse;
    }

    void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;   //  dumps BFFMPContext state

    BFOB0               bfob0;

    CCriticalSection    critbfob0ol;
    //  protected by critbfob0ol
    BFOB0OverflowList   bfob0ol;

    //
    //  Checkpoint depth maint properties.
    //
    //  protected implicitly by 1 checkpoint thread on this struct

    ERR                 errLastCheckpointMaint;
    TICK                tickMaintCheckpointDepthLast;

    LGPOS               lgposFlusherBM;
    LGPOS               lgposVersionerBM;
    LGPOS               lgposLastLogTip;

    typedef struct
    {
        ULONG           cEntriesVisited;
        ULONG           cCleanRemoved;
        ULONG           cFlushErrSuccess;
        ULONG           cFlushErrOther;
        ULONG           cFlushErrPageFlushed;
        ULONG           cFlushErrPageFlushPending;
        ULONG           cFlushErrRemainingDependencies;
        ULONG           cFlushErrDependentPurged;
        ULONG           cFlushErrLatchConflict;
        ULONG           cFlushErrPageTouchTooRecent;
    } ChkAdvStats;
    ChkAdvStats         ChkAdvData;

    // not protected, but OK.
    TICK                tickMaintCheckpointDepthNext;   //  The next soonest time this should run. (unless IO completes)

    //  maximum lgposModify for any cached BF that is found to be pinned by LLR
    //
    LGPOS               lgposNewestModify;

    //  last value returned by BFGetOldestLgposBegin0 (lgposMax = not set)
    //
    LGPOS               lgposOldestBegin0Last;

    //
    // histogram for BF/LogPosModify.lGeneration
    // (how many BFs are modified by each log generation)
    //
    BFLogHistogram      m_logHistModify;

    BYTE                fCurrentlyAttached:1;           // BFFMPContext currently has an attached database
    BYTE                m_rgbReserved[ 7 ];
};


//  Deferred Undo Information

extern CRITPOOL< BF > g_critpoolBFDUI;


//  Cache

extern CSmallLookasideCache*    g_pBFAllocLookasideList;

extern CCriticalSection         g_critCacheSizeSetTarget;
extern CCriticalSection         g_critCacheSizeResize;

extern BOOL                     g_fBFCacheInitialized;

extern LONG_PTR                 g_cbfCacheUserOverride;
extern volatile LONG_PTR        cbfCacheTarget;
extern volatile LONG_PTR        g_cbfCacheTargetOptimal;
extern LONG                     g_rgcbfCachePages[icbPageMax];
extern volatile LONG_PTR        cbfCacheAddressable;
extern volatile LONG_PTR        cbfCacheSize;
extern LONG                     g_cbfCacheResident;
extern LONG                     g_cbfCacheClean;
extern ICBPage                  g_icbCacheMax;

extern DWORD                    g_cbfNewlyCommitted;
extern DWORD                    g_cbfNewlyEvictedUsed;
extern DWORD                    g_cpgReclaim;
extern DWORD                    g_cResidenceCalc;

extern ULONG_PTR                g_cbCacheCommittedSize;
extern ULONG_PTR                g_cbCacheReservedSize;

extern LONG_PTR                 g_cpgChunk;
extern void**                   g_rgpvChunk;

extern LONG_PTR                 cbfInit;
extern LONG_PTR                 g_cbfChunk;
extern BF**                     g_rgpbfChunk;


ERR ErrBFICacheInit( _In_ const LONG cbPageSizeMax );
void BFICacheTerm();

enum eResidentCacheStatusChange
{
    eResidentCacheStatusNoChange = 0,
    eResidentCacheStatusDrop,
    eResidentCacheStatusRestore,
};

struct BFCacheStatsChanges : public CZeroInit   //  BFCacheStatsChanges derives from CZeroInit so we don't have to remember to
{                                           //  initililze it with { 0 } in case of local variables (as in the unit tests).
    __int64                     ftResidentLastEvent;
    eResidentCacheStatusChange  eResidentLastEventType;
    LONG                        cbfResidentLast;
    LONG                        cbfCacheLast;
    eResidentCacheStatusChange  eResidentCurrentEventType;
    __int64                     csecLastEventDelta;

    BFCacheStatsChanges() : CZeroInit( sizeof( BFCacheStatsChanges ) )
    {
        C_ASSERT( eResidentCacheStatusNoChange == 0 );
    }
};

void BFICacheIResetTarget();
void BFICacheSetTarget( OnDebug( const LONG_PTR cbfCacheOverrideCheck ) );
ERR ErrBFICacheGrow();
void BFICacheIShrinkAddressable();
void BFICacheIFree();
void BFICacheINotifyCacheSizeChanges(
    const LONG_PTR cbfCacheAddressableInitial,
    const LONG_PTR cbfCacheSizeInitial,
    const LONG_PTR cbfCacheAddressableFinal,
    const LONG_PTR cbfCacheSizeFinal );
ERR ErrBFICacheUpdateStatistics();

INLINE BOOL FBFICacheValidPv( const void* const pv );
INLINE BOOL FBFICacheValidPbf( const PBF pbf );
INLINE PBF PbfBFICacheIbf( const IBF ibf );
INLINE void* PvBFICacheIpg( const IPG ipg );
IBF IbfBFICachePbf( const PBF pbf );
IPG IpgBFICachePv( const void* const pv );

ERR ErrBFICacheISetSize( const LONG_PTR cbfCacheNew );


//  Cache Resource Allocation Manager

class CCacheRAM
    :   public CDBAResourceAllocationManager< cMaintCacheSamplesAvg >
{
    public:

        CCacheRAM();
        virtual ~CCacheRAM();

        void Reset();
        DWORD CpgReclaim()  { return m_cpgReclaimNorm; }
        DWORD CpgEvict()    { return m_cpgEvictNorm; }

        virtual void UpdateStatistics();
        virtual void ConsumeResourceAdjustments( _Out_ double * const pdcbTotalResource, _In_ const double cbResourceSize );
        void OverrideResourceAdjustments( double const dcbRource );

        virtual size_t  TotalPhysicalMemory();
        virtual size_t  AvailablePhysicalMemory();

    protected:

        virtual size_t  TotalPhysicalMemoryEvicted();

        virtual QWORD   TotalResources();
        virtual QWORD   TotalResourcesEvicted();

    public:

        QWORD   GetOptimalResourcePoolSize();
        void    SetOptimalResourcePoolSize();

    private:

        DWORD       m_cpgReclaimCurr;
        DWORD       m_cpgReclaimLast;
        DWORD       m_cpgReclaimNorm;
        DWORD       m_cpgEvictCurr;
        DWORD       m_cpgEvictLast;
        DWORD       m_cpgEvictNorm;

        DWORD       m_cpgPhysicalMemoryEvictedLast;
        size_t      m_cbTotalPhysicalMemoryEvicted;

        DWORD       m_cbTotalResourcesEvictedLast;
        QWORD       m_cbTotalResourcesEvicted;
        
        LONG_PTR    m_cbfCacheNewDiscrete;
        QWORD       m_cbOptimalResourcePoolSizeUsedLast;

        double      m_dcbAdjustmentOverride;
};

extern CCacheRAM g_cacheram;

bool FBFIFaultInBuffer( const PBF pbf, LONG * pcmmpgReclaimed = NULL );


//  Issue List

class CBFIssueList
{
    public:

        CBFIssueList();
        ~CBFIssueList();

        ERR ErrPrepareWrite( const IFMP ifmp );
        ERR ErrPrepareLogWrite( const IFMP ifmp );
        ERR ErrPrepareRBSWrite( const IFMP ifmp );

        VOID NullifyDiskTiltFake( const IFMP ifmp );

        ERR ErrIssue( const BOOL fSync = fFalse );
        VOID AbandonLogOps();

        BOOL FEmpty() const;

        static ERR ErrSync();

    private:

        class CEntry
        {
            public:

                enum eOper
                {
                    operWrite,
                    operLogWrite,
                    operRBSWrite,
                };

                CEntry( const IFMP ifmp, const eOper oper )
                    :   m_ifmp( ifmp ),
                        m_oper( oper ),
                        m_cRequests( 1 )
                {
                }

                IFMP Ifmp() const       { return m_ifmp; }
                eOper Oper() const      { return m_oper; }
                ULONG CRequests() const { return m_cRequests; }

                VOID IncRequests()      { m_cRequests++; }

                static SIZE_T OffsetOfILE() { return OffsetOf( CEntry, m_ile ); }

            private:

                const IFMP  m_ifmp;
                const eOper m_oper;
                ULONG       m_cRequests;
                CInvasiveList< CEntry, OffsetOfILE >::CElement
                            m_ile;
        };

    private:

        ERR ErrPrepareOper( const IFMP ifmp, const CEntry::eOper oper );

    private:

        //  used to keep issue list stack for this thread

        CBFIssueList*           m_pbfilPrev;

        //  used to keep track of pending flush operations for this thread

        CMeteredSection::Group  m_group;
        CInvasiveList< CEntry, CEntry::OffsetOfILE >
                                m_il;

        //  used to sync with all pending flush operations

        static CCriticalSection s_critSync;
        static CMeteredSection  s_msSync;
};


//  Maintenance

    //  Init / Term

ERR ErrBFIMaintInit();
void BFIMaintTerm();

ERR ErrBFIMaintScheduleTask(    POSTIMERTASK        postt,
                                const VOID * const  pvContext,
                                const TICK          dtickDelay,
                                const TICK          dtickSlop );

    //  Concurrency simulation

INLINE BOOL FBFIChance( INT pctChance );
INLINE void BFISynchronicity( void );

    //  Scavenging

enum BFIScavengeSatisfiedReason
{
    eScavengeInvalid                    = 0,
    eScavengeCompleted                  = 1,
    eScavengeCompletedPendingWrites     = 2,
    eScavengeBailedDiskTilt             = 3,
    eScavengeBailedExternalResize       = 4,
    eScavengeVisitedAllLrukEntries      = 5,
#ifdef DEBUG
    eScavengeBailedRandomlyDebug        = 6,
#endif
};

typedef struct
{
    //  If you change order of / add a field / remove / etc try to remember to update
    //  the "!ese cachescavengeruns repro" output.

    __int64         iRun;
    BOOL            fSync;
    TICK            tickStart;
    TICK            tickEnd;

    LONG            cbfCacheSize;
    LONG            cbfCacheTarget;
    LONG            cCacheBoosts;
    __int64         cbCacheBoosted;
    LONG            cbfCacheSizeStartShrink;
    TICK            dtickShrinkDuration;
    LONG            cbfCacheDeadlock;

    LONG            cbfAvail;
    LONG            cbfAvailPoolLow;
    LONG            cbfAvailPoolHigh;
    LONG            cbfAvailPoolTarget;

    LONG            cbfVisited;
    LONG            cbfFlushed;
    LONG            cbfEvictedAvailPool;
    LONG            cbfEvictedShrink;
    LONG            cbfShrinkFromAvailPool;

    LONG            cbfFlushPending;            // includes cbfFlushPendingSlow and cbfFlushPendingHung
    LONG            cbfFlushPendingSlow;
    LONG            cbfFlushPendingHung;
    LONG            cbfFaultPending;            // includes cbfFaultPendingHung
    LONG            cbfFaultPendingHung;
    LONG            cbfOutOfMemory;
    
    LONG            cbfLatched;
    LONG            cbfDiskTilt;
    LONG            cbfAbandoned;
    LONG            cbfDependent;
    LONG            cbfTouchTooRecent;
    LONG            cbfPermanentErrs;
    LONG            cbfHungIOs;

    ERR             errRun;
    BFIScavengeSatisfiedReason eStopReason;
}
BFScavengeStats;

extern ULONG                g_iScavengeLastRun;
extern BFScavengeStats*     g_rgScavengeLastRuns;
const size_t                g_cScavengeLastRuns = OnDebugOrRetailOrRtm( 150, 30, 4 );
extern ULONG                g_iScavengeTimeSeqLast;
extern BFScavengeStats*     g_rgScavengeTimeSeq;
extern size_t               g_cScavengeTimeSeq;
#ifndef RTM
extern BFScavengeStats*     g_rgScavengeTimeSeqCumulative;
#endif

extern CSemaphore g_semMaintScavenge;

ERR ErrBFIMaintScavengePreInit(
    INT     K,
    double  csecCorrelatedTouch,
    double  csecTimeout,
    double  csecUncertainty,
    double  dblHashLoadFactor,
    double  dblHashUniformity,
    double  dblSpeedSizeTradeoff );
ERR ErrBFIMaintScavengeInit( void );
void BFIMaintScavengeTerm( void );
ERR ErrBFIMaintScavengeIScavengePages( const char* const szContextTraceOnly, const BOOL fSync );

    //  Avail Pool

extern CSemaphore       g_semMaintAvailPoolRequestUrgent;
extern CSemaphore       g_semMaintAvailPoolRequest;

extern LONG_PTR         cbfAvailPoolLow;
extern LONG_PTR         cbfAvailPoolHigh;
extern LONG_PTR         cbfAvailPoolTarget;

extern LONG_PTR         cbfCacheDeadlock;
extern LONG_PTR         g_cbfCacheDeadlockMax;

enum BFIMaintAvailPoolRequestType   // bfmaprt
{
    bfmaprtUnspecific   = 1,
    bfmaprtSync         = 2,
    bfmaprtAsync        = 3,
};

enum BFIMaintCacheStatsRequestType  // bfmcsrt
{
    bfmcsrtNormal       = 1,
    bfmcsrtForce        = 2,
};

extern POSTIMERTASK     g_posttBFIMaintAvailPoolIUrgentTask;
extern POSTIMERTASK     g_posttBFIMaintAvailPoolITask;

ERR ErrBFIMaintAvailPoolRequest( const BFIMaintAvailPoolRequestType bfmaprt = bfmaprtUnspecific );
void BFIMaintAvailPoolIUrgentTask( void*, void* );
void BFIMaintAvailPoolITask( void*, void* );
void BFIMaintAvailPoolUpdateThresholds( const LONG_PTR cbfCacheTargetOptimalNew );

    //  Checkpoint Depth

extern CSemaphore       g_semMaintCheckpointDepthRequest;

extern TICK             tickMaintCheckpointDepthLast;
extern IFMP             g_ifmpMaintCheckpointDepthStart;

extern POSTIMERTASK     g_posttBFIMaintCheckpointDepthITask;

enum BFCheckpointDepthMainReason {
    bfcpdmrRequestNone = 0,
    bfcpdmrRequestOB0Movement,
    bfcpdmrRequestIOThreshold,
    bfcpdmrRequestRemoveCleanEntries,
    bfcpdmrRequestConsumeSettings,
};
void BFIMaintCheckpointDepthRequest( FMP * pfmp, const BFCheckpointDepthMainReason eRequestReason );

void BFIMaintCheckpointDepthITask( void*, void* );
void BFIMaintCheckpointDepthIFlushPages( TICK * ptickNextScheduleDelta );
ERR ErrBFIMaintCheckpointDepthIFlushPagesByIFMP( const IFMP ifmp, BOOL * const pfUpdateCheckpoint );

    //  Checkpoint

extern CSemaphore       g_semMaintCheckpointRequest;
extern TICK             g_tickMaintCheckpointLast;
extern POSTIMERTASK     g_posttBFIMaintCheckpointITask;
    
void BFIMaintCheckpointRequest();

void BFIMaintCheckpointITask( VOID* pvGroupContext, VOID* pvRuntimeContext );
void BFIMaintCheckpointIUpdateInst( const size_t ipinst );
void BFIMaintCheckpointIUpdate();

#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY

    //  Hashed Latches

extern const BOOL       g_fBFMaintHashedLatches;
extern size_t           g_icReqOld;
extern size_t           g_icReqNew;
extern ULONG            g_rgcReqSystem[ 2 ];
extern ULONG            g_dcReqSystem;

TICK TickBFIHashedLatchTime( const TICK tickIn );
void BFIMaintHashedLatchesITask( DWORD_PTR );
void BFIMaintHashedLatchesIRedistribute();

#endif  //  MINIMAL_FUNCTIONALITY

    //  Cache Size

extern POSTIMERTASK     g_posttBFIMaintCacheStatsITask;
extern POSTIMERTASK     g_posttBFIMaintIdleCacheStatsITask;

extern CSemaphore       g_semMaintCacheStatsRequest;

extern DWORD_PTR        cCacheStatsInit;

extern CSemaphore       g_semMaintCacheSize;
extern LONG             g_cMaintCacheSizePending;

void BFIMaintLowMemoryCallback( DWORD_PTR pvUnused );
void BFIMaintIdleCacheStatsITask( VOID * pvGroupContext, VOID * );
void BFIMaintCacheStatsITask( VOID *, VOID * pvContext );
INLINE ERR ErrBFIMaintCacheStatsRequest( const BFIMaintCacheStatsRequestType bfmcsrt );
INLINE BOOL FBFIMaintCacheStatsTryAcquire();
INLINE void BFIMaintCacheStatsRelease();

INLINE CBF CbfBFICacheUsed();
INLINE CBF CbfBFICacheCommitted();
__int64 CbBFICacheSizeUsedDehydrated();
__int64 CbBFICacheISizeUsedHydrated();
__int64 CbBFICacheIMemoryReserved();
__int64 CbBFICacheIMemoryCommitted();
__int64 CbBFIAveResourceSize();
LONG_PTR CbfBFICredit();
LONG_PTR CbfBFIAveCredit();
__int64 CbBFICacheBufferSize();
INLINE INT CbBFISize( ICBPage icb );
extern const LONG g_rgcbPageSize[icbPageMax];

extern POSTIMERTASK     g_posttBFIMaintCacheSizeITask;
INLINE ERR ErrBFIMaintCacheSizeRequest( OnDebug( BOOL* const pfAcquiredSemaphoreCheck = NULL ) );
void BFIMaintCacheSizeITask( void*, void* );
void BFIMaintCacheSizeIShrink();
INLINE BOOL FBFIMaintCacheSizeAcquire();
INLINE ERR ErrBFIMaintCacheSizeReleaseAndRescheduleIfPending();
TICK DtickBFIMaintCacheSizeDuration();

    //  Buffer Size and Management

void BFIFaultInBuffer( __inout void * pv, _In_ LONG cb );
void BFIFaultInBuffer( const PBF pbf );

ICBPage IcbBFIBufferSize( _In_ const INT cbSize );
LONG CbBFIBufferSize( const PBF pbf );
inline _Ret_range_( icbPageInvalid, icbPageBiggest ) ICBPage IcbBFIPageSize( _In_ const INT cbSize );
LONG CbBFIPageSize( const PBF pbf );

ERR ErrBFISetBufferSize( __inout PBF pbf, _In_ const ICBPage icbNewSize, _In_ const BOOL fWait );
void BFIGrowBuffer( __inout PBF pbf, _In_ const ICBPage icbNewSize );
void BFIShrinkBuffer( __inout PBF pbf, _In_ const ICBPage icbNewSize );

void BFIDehydratePage( PBF pbf, _In_ const BOOL fAllowReorg );
void BFIRehydratePage( PBF pbf );


    //  Idle Database

extern CSemaphore       g_semMaintIdleDatabaseRequest;

extern TICK             g_tickMaintIdleDatabaseLast;

extern POSTIMERTASK     g_posttBFIMaintIdleDatabaseITask;

void BFIMaintIdleDatabaseRequest( PBF pbf );

void BFIMaintIdleDatabaseITask( void*, void* );
void BFIMaintIdleDatabaseIRollLogs();
void BFIMaintIdleDatabaseIRollLogs( INST * const pinst );
BOOL FBFIMaintIdleDatabaseIDatabaseHasPinnedPages( const INST * const pinst, const DBID dbid );

    //  Cache residency map and statistics

extern POSTIMERTASK     g_posttBFIMaintCacheResidencyITask;

void BFIMaintCacheResidencyInit();
void BFIMaintCacheResidencyTerm();
INLINE void BFIMaintCacheResidencyRequest();
void BFIMaintCacheResidencyITask( void*, void* );
INLINE BFResidenceState BfrsBFIUpdateResidentState( PBF const pbf, const BFResidenceState bfrsNew );
INLINE BFResidenceState BfrsBFIUpdateResidentState( PBF const pbf, const BFResidenceState bfrsNew, const BFResidenceState bfrsIfOld );

    //  Cache telemetry

extern POSTIMERTASK    g_posttBFIMaintTelemetryITask;

void BFIMaintTelemetryRequest();

void BFIMaintTelemetryITask( VOID *, VOID * pvContext );

//  Internal Functions

    //  Hashed Latch

INLINE BOOL FBFILatchValidContext( const DWORD_PTR dwContext );
INLINE PBF PbfBFILatchContext( const DWORD_PTR dwContext );
INLINE CSXWLatch* PsxwlBFILatchContext( const DWORD_PTR dwContext );
void BFILatchNominate( const PBF pbf );
BOOL FBFILatchDemote( const PBF pbf );

    //  BF FMP Context

ERR ErrBFISetupBFFMPContext( IFMP ifmp );

    //  Page Manipulation

void BFIAssertNewlyAllocatedPage( const PBF pbfNew, const BOOL fAvailPoolAdd = fFalse );
ERR ErrBFIAllocPage( PBF* const ppbf, _In_ const ICBPage icbBufferSize, const BOOL fWait = fTrue, const BOOL fMRU = fTrue );
void BFIReleaseBuffer( PBF pbf );
void BFIFreePage( PBF pbf, const BOOL fMRU = fTrue, const BFFreePageFlags bffpfDangerousOptions = bffpfNone );

ERR ErrBFICachePage(    PBF* const ppbf,
                        const IFMP ifmp,
                        const PGNO pgno,
                        const BOOL fNewPage,
                        const BOOL fWait,
                        const BOOL fMRU,
                        const ULONG_PTR pctCachePriority,
                        const TraceContext& tc,
                        const BFLatchType bfltTraceOnly,
                        const BFLatchFlags bflfTraceOnly,
                        BOOL* const pfRepeatedRead );

INLINE BOOL FBFICacheViewFresh( const PBF pbf );

void BFIOpportunisticallyFlushPage( PBF pbf, IOREASONPRIMARY iorp );

//  Page / Buffer maintenance occasionally has to work against foreground threads that
//  happen to have the latch.  These functions perform that maintenance.  Note the two
//  functions have significantly different contracts.  The deprecated one is first,
//  and should only be used in places where you can't guarantee at the BF API level
//  that you will be able to unburden the page.  The 2nd version is preferred, but
//  can only be used in places where the current/newer version of the page can be moved
//  to a newer buffer.
void BFIMaintImpedingPage( PBF pbf );
ERR ErrBFIMaintImpedingPageLatch( PBF pbf, _In_ const BOOL fOwnsWrite, BFLatch* pbfl );

//  While the name claims it is for versioning pages, the truth is these are only used
//  to unload a hot or behind the checkpoint page.
void BFIOpportunisticallyVersionPage( PBF pbf, PBF * ppbfOpportunisticCheckpointAdv );
void BFIOpportunisticallyVersionCopyPage( PBF pbf, PBF * ppbfNew, _In_ const BOOL fOwnsWrite );

//  Versioning a page can happen in two directions, one where the newly allocated buffer
//  is the "older" version (ErrBFIVersionPage()) and one where the newly allocated
//  buffer is the "current" version of the page (ErrBFIVersionCopyPage()).
ERR ErrBFIVersionPage( PBF pbf, PBF* ppbfOld, const BOOL fWait = fTrue );
ERR ErrBFIVersionCopyPage( PBF pbfOrigOld, PBF* ppbfNewCurr, const BOOL fWait, _In_ const BOOL fOwnsWrite );
void BFICleanVersion( PBF pbf, BOOL fTearDownFMP );

BOOL CmpPgno( _In_ const PGNO& pgno1, _In_ const PGNO& pgno2 );
ERR ErrBFIPrereadPage( IFMP ifmp, PGNO pgno, const BFPreReadFlags bfprf, const BFPriority bfpri, const TraceContext& tc );
INLINE void BFIMarkAsSuperCold( PBF pbf, const BOOL fUser );
INLINE void BFITouchResource(
    _In_ const PBF                  pbf,
    _In_ const BFLatchType          bfltTraceOnly,
    _In_ const BFLatchFlags         bflfTraceOnly,
    _In_ const BOOL                 fTouch,
    _In_ const ULONG_PTR            pctCachePriority,
    _In_ const TraceContext&        tc );

INLINE BOOL FBFIDatabasePage( const PBF pbf );
LOCAL BOOL FBFIBufferIsZeroed( const PBF pbf );

typedef INT CPageEvents;
CPageEvents CpeBFICPageEventsFromBflf(const BFLatchFlags bflf);

void BFITrackCacheMissLatency( const PBF pbf, HRT hrtStart, const BFTraceCacheMissReason bftcmr, const OSFILEQOS qosIoPriorities, const TraceContext& tc, ERR errValidate );

INLINE ERR ErrBFIValidatePage( const PBF pbf, const BFLatchType bflt, const CPageEvents cpe, const TraceContext& tc );
ERR ErrBFIValidatePageSlowly( PBF pbf, const BFLatchType bflt, const CPageEvents cpe, const TraceContext& tc );
void BFIPatchRequestIORange( PBF pbf, const CPageEvents cpe, const TraceContext& tc );
#define BFIValidatePagePgno( pbf )      BFIValidatePagePgno_( pbf, __FUNCTION__ )
void BFIValidatePagePgno_( const PBF pbf, PCSTR szFunction );
void BFIValidatePageUsed( const PBF pbf );

ERR ErrBFIVerifyPage( const PBF pbf, const CPageEvents cpe, const BOOL fFixErrors );

bool FBFICurrentPage( _In_ const PBF pbf, _In_ const IFMP ifmp, _In_ const PGNO pgno );
bool FBFIUpdatablePage( _In_ const PBF pbf );

BOOL FBFIOwnsLatchType( const PBF pbf, const BFLatchType bfltHave );
void BFIInitialize( _In_ PBF pbf, const TraceContext& tc );
ERR ErrBFILatchPage(    _Out_ BFLatch* const    pbfl,
                        const IFMP              ifmp,
                        const PGNO              pgno,
                        const BFLatchFlags      bflf,
                        const BFLatchType       bfltReq,
                        const BFPriority        bfpri,
                        const TraceContext&     tc,
                        _Out_ BOOL* const       pfCachedNewPage = NULL );
void BFIReleaseSXWL( __inout PBF const pbf, const BFLatchType bfltHave );
void BFIUnlatchMaintPage( __inout PBF const pbf, _In_ const BFLatchType bfltHave );

PBF PbfBFIGetFlushOrderLeaf( const PBF pbf, const BOOL fFlagCheckpointImpeders );

void BFIAssertReadyForWrite( _In_ const PBF pbf );
ERR ErrBFITryPrepareFlushPage(
    _Inout_ const PBF           pbf,
    _In_ const BFLatchType  bfltHave,
    _In_       IOREASON     ior,
    _In_ const OSFILEQOS    qos,
    _In_ const BOOL         fRemoveDependencies );
ERR ErrBFIPrepareFlushPage(
                        _In_ const PBF      pbf,
                        _In_ const BFLatchType  bfltHave,
                        _In_       IOREASON     ior,
                        _In_ const OSFILEQOS    qos,
                        _In_ const BOOL         fRemoveDependencies = fTrue,
                        _Out_opt_ BOOL * const  pfPermanentErr      = NULL );
BOOL FBFITryAcquireExclusiveLatchForMaint( const PBF pbf );
ERR ErrBFIAcquireExclusiveLatchForFlush( PBF pbf, _In_ const BOOL fUnencumberedPath );
ERR ErrBFIFlushExclusiveLatchedAndPreparedBF(   __inout const PBF       pbf,
                                    _In_ const IOREASON     iorBase,
                                    _In_ const OSFILEQOS    qos,
                                    _In_ const BOOL         fOpportune );
ERR ErrBFIFlushPage(    __inout const PBF       pbf,
                        _In_ const IOREASON     iorBase,
                        _In_ const OSFILEQOS    qos,
                        _In_ const BFDirtyFlags bfdfFlushMin    = bfdfDirty,
                        _In_ const BOOL         fOpportune      = fFalse,
                        __out_opt BOOL * const  pfPermanentErr  = NULL );
bool FBFICompleteFlushPage( _Inout_ PBF pbf, _In_ const BFLatchType bflt, _In_ const BOOL fUnencumberedPath = fFalse, _In_ const BOOL fCompleteRemapReVerify = fTrue, _In_ const BOOL fAllowTearDownClean = fFalse );

INLINE BOOL FBFIIsCleanEnoughForEvict( const PBF pbf );
void BFIFlagDependenciesImpeding( PBF pbf );
ERR ErrBFIEvictRemoveCleanVersions( PBF pbf );
ERR ErrBFIEvictPage( PBF pbf, BFLRUK::CLock* plockLRUK, const BFEvictFlags bfefDangerousOptions );
void BFIPurgeAllPageVersions( _Inout_ BFLatch* const pbfl, const TraceContext& tc );
void BFIPurgeNewPage( _Inout_ const PBF pbf, const TraceContext& tc );
void BFIPurgePage( _Inout_ const PBF pbf, _In_ const IFMP ifmpCheck, _In_ const PGNO pgnoCheck, _In_ const BFLatchType bfltHave, _In_ const BFEvictFlags bfefDangerousOptions );
void BFIRenouncePage( _Inout_ PBF pbf, _In_ const BOOL fRenounceDirty );

extern const CHAR mpbfdfsz[ bfdfMax - bfdfMin ][ 16 ];
void BFIDirtyPage( PBF pbf, BFDirtyFlags bfdf, const TraceContext& tc );
void BFICleanPage( __inout PBF pbf, _In_ const BFLatchType bfltHave, _In_ const BFCleanFlags bfcf = bfcfNone );

    //  Range-locking for external zeroing.

struct BFIPageRangeLock
{
    IFMP                        ifmp;           // IFMP.
    PGNO                        pgnoFirst;      // First page to be locked.
    PGNO                        pgnoLast;       // Last page to be locked.
    PGNO                        pgnoDbLast;     // Last page of the database before starting to lock the range.
    CPG                         cpg;            // Number of pages to be locked.
    BFLatch*                    rgbfl;          // Array of BFLatch objects.
    _Field_size_opt_(cpg) BOOL* rgfLatched;     // Whether or not each page is currently latched.
    _Field_size_opt_(cpg) BOOL* rgfUncached;    // Whether or not each page was uncached when first write-latched.
    BOOL                        fRangeLocked;   // Whether or not the range is range-locked (file-level range-locked).
    CMeteredSection::Group      irangelock;     // Range-lock group.

    //  Ctor.
    BFIPageRangeLock() :
        ifmp( ifmpNil ),
        pgnoFirst( pgnoNull ),
        pgnoLast( pgnoNull ),
        pgnoDbLast( pgnoNull ),
        cpg( 0 ),
        rgbfl( NULL ),
        rgfLatched( NULL ),
        rgfUncached( NULL ),
        fRangeLocked( fFalse ),
        irangelock( CMeteredSection::groupTooManyActiveErr )
    {
    }

    //  Dtor.
    ~BFIPageRangeLock()
    {
#ifdef DEBUG
        Assert( !fRangeLocked );
        if ( rgfLatched != NULL )
        {
            for ( size_t ipg = 0; ipg < (size_t)cpg; ipg++ )
            {
                Assert( !rgfLatched[ ipg ] );
            }
        }
#endif  // DEBUG.

        delete[] rgbfl;
        delete[] rgfLatched;
        delete[] rgfUncached;
    }
};

    //  Log I/O

ERR ErrBFIWriteLog( _In_ const IFMP ifmp, _In_ const BOOL fSync );
ERR ErrBFIFlushLog( _In_ const IFMP ifmp, _In_ const IOFLUSHREASON iofr, const BOOL fMayOwnBFLatch = fFalse );


    //  I/O

void* PvBFIAcquireIOContext( PBF pbf );
void BFIReleaseIOContext( PBF pbf, void* const pvIOContext );
void BFISetIOContext( PBF pbf, void* const pvIOContextNew );
void BFIResetIOContext( PBF pbf );

BOOL FBFIIsIOHung( PBF pbf );
BYTE PctBFIIsIOHung( PBF pbf, void* const pvIOContext );
ERR ErrBFIFlushPendingStatus( PBF pbf );

void BFIPrepareReadPage( PBF pbf );
void BFIPrepareWritePage( PBF pbf );

void BFISyncRead( PBF pbf, const OSFILEQOS qosIoPriorities, const TraceContext& tc );
void BFISyncReadHandoff(    const ERR err,
                            IFileAPI *const pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS grbitQOS,
                            const QWORD ibOffset,
                            const DWORD cbData,
                            const BYTE* const pbData,
                            const PBF pbf,
                            void* const pvIOContext );
void BFISyncReadComplete(   const ERR err,
                            IFileAPI *const pfapi,
                            const OSFILEQOS grbitQOS,
                            const QWORD ibOffset,
                            const DWORD cbData,
                            const BYTE* const pbData,
                            const PBF pbf );

ERR ErrBFIAsyncPreReserveIOREQ( IFMP ifmp, PGNO pgno, OSFILEQOS qos, VOID ** ppioreq );
VOID BFIAsyncReleaseUnusedIOREQ( IFMP ifmp, VOID * pioreq );

//  this function performs a Async Read into the specified Write Latched BF

ERR ErrBFIAsyncRead( PBF pbf, OSFILEQOS qos, VOID * pioreq, const TraceContext& tc );
void BFIAsyncReadHandoff(   const ERR err,
                            IFileAPI *const pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS grbitQOS,
                            const QWORD ibOffset,
                            const DWORD cbData,
                            const BYTE* const pbData,
                            const PBF pbf,
                            void* const pvIOContext );
void BFIAsyncReadComplete(  const ERR err,
                            IFileAPI *const pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS grbitQOS,
                            const QWORD ibOffset,
                            const DWORD cbData,
                            const BYTE* const pbData,
                            const PBF pbf );
void BFIAsyncReadTempComplete(  const ERR err,
                                IFileAPI *const pfapi,
                                const FullTraceContext& tc,
                                const OSFILEQOS grbitQOS,
                                const QWORD ibOffset,
                                const DWORD cbData,
                                const BYTE* const pbData,
                                const IFMP ifmp);

ERR ErrBFISyncWrite( PBF pbf, const BFLatchType bfltHave, OSFILEQOS qos, const TraceContext& tc );
void BFISyncWriteHandoff(   const ERR err,
                            IFileAPI *const pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS     grbitQOS,
                            const QWORD ibOffset,
                            const DWORD cbData,
                            const BYTE* const pbData,
                            const PBF pbf,
                            void* const pvIOContext );
void BFISyncWriteComplete(  const ERR err,
                            IFileAPI *const pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS     grbitQOS,
                            const QWORD ibOffset,
                            const DWORD cbData,
                            const BYTE* const pbData,
                            const PBF pbf,
                            const BFLatchType bfltHave );
ERR ErrBFIAsyncWrite( PBF pbf, OSFILEQOS qos, const TraceContext& tc );
void BFIAsyncWriteHandoff(  const ERR err,
                            IFileAPI *const pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS     grbitQOS,
                            const QWORD ibOffset,
                            const DWORD cbData,
                            const BYTE* const pbData,
                            const PBF pbf,
                            void* const pvIOContext );
void BFIAsyncWriteComplete( const ERR err,
                            IFileAPI *const pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS     grbitQOS,
                            const QWORD ibOffset,
                            const DWORD cbData,
                            const BYTE* const pbData,
                            const PBF pbf );
ERR ErrBFIWriteSignalIError( ULONG_PTR pSignalNext ); // for edbg.cxx
ERR ErrBFIWriteSignalState( const PBF pbf );
void BFIFlushComplete( _Inout_ const PBF pbf, _In_ const BFLatchType bfltHave, _In_ const BOOL fUnencumberedPath, _In_ const BOOL fCompleteRemapReVerify, _In_ const BOOL fAllowTearDownClean );

INLINE BOOL FBFIOpportuneWrite( PBF pbf );

    //  Time Dependencies

extern CCriticalSection     g_critBFDepend;
    
    //  Transaction/Logging Support

void BFISetLgposOldestBegin0( PBF pbf, LGPOS lgpos, const TraceContext& tc );
void BFIResetLgposOldestBegin0( PBF pbf, BOOL fCalledFromSet = fFalse );

void BFISetLgposModify( PBF pbf, LGPOS lgpos );
void BFIResetLgposModify( PBF pbf );

void BFIAddUndoInfo( PBF pbf, RCE* prce, BOOL fMove = fFalse );
void BFIRemoveUndoInfo( PBF pbf, RCE* prce, LGPOS lgposModify = lgposMin, BOOL fMove = fFalse );

#ifdef PERFMON_SUPPORT
//  Performance Monitoring Support


extern PERFInstanceLiveTotalWithClass<> cBFPagesReadAsync;
extern PERFInstanceLiveTotalWithClass<> cBFPagesReadSync;
extern PERFInstanceLiveTotalWithClass<> cBFPagesCoalescedRead;
extern PERFInstanceLiveTotalWithClass<> cBFPagesRepeatedlyRead;
extern PERFInstanceLiveTotalWithClass<> cBFPagesPreread;
extern PERFInstanceLiveTotalWithClass<> cBFPrereadStall;
extern PERFInstanceLiveTotalWithClass<> cBFPagesPrereadUnnecessary;

extern PERFInstanceLiveTotalWithClass<ULONG> cBFCacheMiss;
extern PERFInstanceLiveTotalWithClass<ULONG> cBFCacheReq;
extern TICK g_tickBFUniqueReqLast;
extern PERFInstanceLiveTotalWithClass<ULONG, INST, 2> cBFCacheUniqueHit;
extern PERFInstanceLiveTotalWithClass<ULONG, INST, 2> cBFCacheUniqueReq;
extern PERFInstance<> cBFSlowLatch;
extern PERFInstance<> cBFBadLatchHint;
extern PERFInstance<> cBFLatchConflict;
extern PERFInstance<> cBFLatchStall;
extern PERFInstanceDelayedTotalWithClass< LONG, INST, 2 > cBFCache;

extern PERFInstanceLiveTotalWithClass<> cBFDirtied;
extern PERFInstanceLiveTotalWithClass<> cBFDirtiedRepeatedly;

extern PERFInstanceLiveTotalWithClass<> cBFSuperColdsUser;
extern PERFInstanceLiveTotalWithClass<> cBFSuperColdsInternal;

extern PERFInstanceLiveTotalWithClass<> cBFPagesDehydrated;
extern PERFInstanceLiveTotalWithClass<> cBFPagesRehydrated;

extern PERFInstanceLiveTotalWithClass<> cBFPagesVersioned;
extern PERFInstanceLiveTotalWithClass<> cBFPagesVersionCopied;
extern LONG g_cBFVersioned;

extern LONG g_cbfTrimmed;
extern LONG g_cbfNonResidentReclaimedSuccess;
extern LONG g_cbfNonResidentReclaimedFailed;
extern LONG g_cbfNonResidentRedirectedToDatabase;
extern LONG g_cbfNonResidentEvicted;
extern LONG g_cbfNonResidentReclaimedHardSuccess;
extern unsigned __int64 g_cusecNonResidentFaultedInLatencyTotal;

extern PERFInstanceDelayedTotal< LONG, INST, fFalse > cBFCheckpointMaintOutstandingIOMax;

extern PERFInstanceLiveTotalWithClass<> cBFPagesWritten;
extern PERFInstanceLiveTotalWithClass<> cBFPagesRepeatedlyWritten;
extern PERFInstanceLiveTotalWithClass<> cBFPagesCoalescedWritten;
// various kinds of flushes
extern PERFInstanceLiveTotalWithClass<> cBFPagesFlushedCacheShrink;
extern PERFInstanceLiveTotalWithClass<> cBFPagesFlushedCheckpoint;
extern PERFInstanceLiveTotalWithClass<> cBFPagesFlushedCheckpointForeground;

#endif //PERFMON_SUPPORT

extern PERFInstanceLiveTotalWithClass<> cBFPagesFlushedContextFlush;

#ifdef PERFMON_SUPPORT

extern PERFInstanceLiveTotalWithClass<> cBFPagesFlushedFilthyForeground;
extern PERFInstanceLiveTotalWithClass<> cBFPagesFlushedAvailPool;
extern PERFInstanceLiveTotalWithClass<> cBFPagesFlushedScavengeSuperColdInternal;
extern PERFInstanceLiveTotalWithClass<> cBFPagesFlushedScavengeSuperColdUser;
extern PERFInstanceLiveTotalWithClass<> cBFPagesFlushedOpportunely;
extern PERFInstanceLiveTotalWithClass<> cBFPagesFlushedOpportunelyClean;

extern PERFInstanceLiveTotalWithClass<> cBFCacheEvictUntouched;
extern PERFInstanceLiveTotalWithClass<> cBFCacheEvictk1;
extern PERFInstanceLiveTotalWithClass<> cBFCacheEvictk2;
extern PERFInstanceLiveTotalWithClass<> rgcBFCacheEvictReasons[bfefReasonMax];
extern PERFInstanceLiveTotalWithClass<> cBFCacheEvictScavengeSuperColdInternal;
extern PERFInstanceLiveTotalWithClass<> cBFCacheEvictScavengeSuperColdUser;

extern PERFInstanceLiveTotalWithClass<> tickBFCacheOldestEvict;
extern __int64 g_cbCacheUnattached;

extern PERFInstanceLiveTotalWithClass<QWORD> cBFCacheMissLatencyTotalTicksAttached;
extern PERFInstanceLiveTotalWithClass<> cBFCacheMissLatencyTotalOperationsAttached;

extern PERFInstanceLiveTotalWithClass<> cBFCacheUnused;

#endif  //  PERFMON_SUPPORT

#endif  //  _BF_HXX_INCLUDED

