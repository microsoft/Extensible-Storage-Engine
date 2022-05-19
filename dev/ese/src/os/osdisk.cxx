// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

#include <winioctl.h>

//
//  I/O Manager [Lower Layer]

//
//

//
//  See _osfs.hxx for the complete architectural layering ... but this file
//  consists of 3 components, and a sub-components or two:
//
//      1. The IOREQ Pool - A growable pool of IOREQs.  The IOREQ is the critical data
//          structure to the I/O Manager as it holds all the relevant information for
//          the present state (free, enqueued in a queue, pending enqueueing, or even
//          actually issued to the OS) and relevant info for in-flight I/Os.  The IOREQ
//          data structure is owned / used by many aspects of the OS File & IO layer.
//
//      2. The OS Disk - Our version of a logical disk with better I/O characteristics,
//          emulating Quality of Service constraints and managing the total amount of
//          background I/O submitted to the system / disks.
//
//          2.b IO Queue / Heap - This is the "IO Queue" though it is not always queue
//              like.  Our current implementation is split amongst:
//              2.b.i - VIP Queue - Infrequently used, usually for cbData = 0 ops.
//              2.b.ii - Oscillating heaps (IOHeapA, IOHeapB) managed by COSDisk::IOQueue
//                  and COSDisk::IOQueue::IOHeapA/B used only for writes (in the new 
//                  Smooth pre-read IO model).  This mode is being deprecated by the 
//                  next implementation (2.b.iii).
//                  This Q also is the "unlimited" Q in that once ErrIOIssue is called at
//                  the file level, the whole Q / all these IOs will be pushed to the OS.
//              2.b.iii - Oscillating RedBlackTrees (m_irbtBuilding, m_irbtDraining) used
//                  for async prereads (at the moment, extending to writes in future).
//                  This Q is meted and will only let a handful of IOs be outstanding for
//                  this purpose at a time.
//
//              Note: The qosIODispatch* bits for Read IO will control whether you end up
//              in the unlimited-IOHeap or in the new-meted-RedBlackTree queue.
//                - qosIODispatchBackground will send you to the Meted Q.
//                    Note: This didn't use to happen for Sync IO ever, but now even sync
//                    IO can end up in the meted Q if it's QOS is qosIODispatchBackground.
//                - qosIODispatchImmediate will send you to the unlimited IOHeap, and thus
//                    immediate issue once the IOMgr issue thread starts.
//                    Note: Sync IO does not even wait for IOMgr as it issues itself right
//                    on the foreground thread.
//
//              Write IOs always go through the IOHeap, but ARE meted but at higher layers
//              (ex: like one imagines if you were writng a DB engine on this OS layer, 
//              then you might have some tasks that say "scavenge" a DB cache to create 
//              avail pool OR a task that like checkpoints dirty buffers out of a DB cache 
//              to minimize crash rebound time) by responding to the push back signal of 
//              errDiskTilt that is passed to the IO write attempt.
//
//      3. The IO Thread - A simple taskmgr thread that pulls IOs from the open OS Disk
//          for the system and issues the IO to the OS.
//

//  Other required headers specific to osdisk

#include "collection.hxx"

// Required for _alloca() in NT build environment.
#include <malloc.h>

COSFilePerfDummy    g_cosfileperfDefault;

BYTE* g_rgbGapping = NULL;            // buffer, sized to OS commit gran, aligned to OS align gran,

const LOCAL OSDiskMappingMode g_diskModeDefault = eOSDiskOneDiskPerPhysicalDisk;


////////////////////////////////////////
//  Forward Decls

void OSDiskIIOThreadTerm( void );
ERR ErrOSDiskIIOThreadInit( void );


////////////////////////////////////////
//  Support Functions

//  returns the integer result of subtracting iFile1/ibOffset1 from iFile2/ibOffset2

INLINE __int64 CmpOSDiskIFileIbIFileIb( QWORD iFile1, QWORD ibOffset1, QWORD iFile2, QWORD ibOffset2 )
{
    if ( iFile1 - iFile2 )
    {
        return iFile1 - iFile2;
    }
    else
    {
        return ibOffset1 - ibOffset2;
    }
}


// =============================================================================================
//  I/O Request Pool
// =============================================================================================
//


//  critical section protecting the I/O Heap and the growable IOREQ pool

CCriticalSection g_critIOREQPool( CLockBasicInfo( CSyncBasicInfo( "IOREQ Pool" ), rankIOREQPoolCrit, 0 ) );

//  growable IOREQ storage

IOREQCHUNK *    g_pioreqchunkRoot;
DWORD           g_cbIoreqChunk;

INLINE DWORD CioreqPerChunk( DWORD cbChunk )
{
    return ( ( cbChunk - sizeof(IOREQCHUNK) ) / sizeof(IOREQ) );
}

//  This returns the next IOREQ storage slot, and if no more are availabe allocates 
//  a new chunk of IOREQs and returns a storage slot from there.  This does not 
//  protect someone from allocating more than cioreq.

IOREQ * PioreqOSDiskIIOREQAllocSlot( void )
{
    ERR err = JET_errSuccess;

    IOREQ * pioreqSlot = NULL;

    Assert( g_critIOREQPool.FOwner() );

    if ( g_pioreqchunkRoot->cioreqMac >= g_pioreqchunkRoot->cioreq )
    {
        Assert( g_pioreqchunkRoot->cioreqMac == g_pioreqchunkRoot->cioreq );

        //  need to allocate a whole new chunk of IOREQs ...

        IOREQCHUNK * pioreqchunkNew;
        Alloc( pioreqchunkNew = (IOREQCHUNK*)PvOSMemoryHeapAlloc( g_cbIoreqChunk ) );
        memset( pioreqchunkNew, 0, g_cbIoreqChunk );

        //  init the IOREQ chunk and link into the list

        pioreqchunkNew->pioreqchunkNext = g_pioreqchunkRoot;
        pioreqchunkNew->cioreq = CioreqPerChunk( g_cbIoreqChunk );
        pioreqchunkNew->cioreqMac = 0;
        g_pioreqchunkRoot = pioreqchunkNew;
    }
    Assert( g_pioreqchunkRoot->cioreqMac < g_pioreqchunkRoot->cioreq );

    //  set the new IOREQ storage slot

    pioreqSlot = &(g_pioreqchunkRoot->rgioreq[g_pioreqchunkRoot->cioreqMac]);

    g_pioreqchunkRoot->cioreqMac++;

HandleError:
    
    return pioreqSlot;
}

IOREQ::IOREQ() :
    m_crit( CLockBasicInfo( CSyncBasicInfo( "IOREQ" ), rankIOREQ, 0 ) )
{
}

IOREQ::IOREQ( const IOREQ* pioreqToCopy ) :
    MemCopyable<IOREQ>( pioreqToCopy ),
    m_crit( CLockBasicInfo( CSyncBasicInfo( "IOREQ Copy Ctor" ), rankIOREQ, 0 ) ),

    // We don't want the copy to have USER_CONTEXT_DESC because it requires a heap allocation
    // We avoid that by passing in false to the constructor
    // We copy the rest as is
    m_tc( pioreqToCopy->m_tc, false )
{
    // We expect we only copy an IOREQ on completion, so after the 
    // memcpy() it should be FCompleted().
    Expected( FCompleted() );
}

//  This intializes an IOREQ

void OSDiskIIOREQInit( IOREQ * const pioreq, HANDLE hEvent )
{
    Assert( pioreq );
    Assert( hEvent );

    // initialize the IOREQ to an allocatable state ...
#ifdef DEBUG
    BYTE rgbioreq[sizeof(IOREQ)] = { 0 };
    Assert( memcmp( rgbioreq, pioreq, sizeof(rgbioreq) ) == 0 );
#endif
    memset( pioreq, 0, sizeof(*pioreq) );
    new( pioreq ) IOREQ;
    Assert( pioreq->FNotOwner() );
    pioreq->ovlp.hEvent = hEvent;
    SetHandleInformation( pioreq->ovlp.hEvent, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
}


////////////////////////////////////////
//  I/O Request Quota System

//  IOREQ pool

typedef CPool< IOREQ, IOREQ::OffsetOfAPIC > IOREQPool;

//  IOREQ quota / current max / and general pool

volatile DWORD  g_cioreqInUse;
IOREQPool*      g_pioreqpool;

//  IOREQ reserve available pool

IOREQPool*      g_pioreqrespool;
volatile DWORD  g_cioreqrespool = 0;
volatile DWORD  g_cioreqrespoolleak = 0;

//  frees an IOREQ

INLINE void OSDiskIIOREQFree( IOREQ* pioreq )
{

    Assert( NULL == pioreq->pioreqIorunNext );  // should already be de-linked ...
    Assert( NULL == pioreq->pioreqVipList );

    pioreq->p_osf = NULL;

    if ( pioreq->fFromReservePool )
    {
        Assert( !pioreq->m_fFromTLSCache );

        pioreq->SetIOREQType( IOREQ::ioreqInReservePool );

        //  this IOREQ is from the reserve pool, so free it back to the reserve avail pool

        g_pioreqrespool->Insert( pioreq );
    }
    else
    {
        pioreq->m_fFromTLSCache = fFalse;
        pioreq->SetIOREQType( IOREQ::ioreqInAvailPool );

        //  free the IOREQ to the pool

        g_pioreqpool->Insert( pioreq );
    }
}

//  attemps to create a new IOREQ and, if successful, frees it to the IOREQ
//  pool

INLINE ERR ErrOSDiskIIOREQCreate()
{
    ERR     err             = JET_errSuccess;
    HANDLE  hEvent          = NULL;

    //  In the unlikely event of a water landing ...

    if ( g_cioreqInUse >= 10000000 )
    {
        //  We've got to stop somewhere ...

        AssertSz( fFalse, "If we hit this our quota system has gone wrong as it's not protecting things enough" );
        return JET_errSuccess;
    }

    g_critIOREQPool.Enter();

    AtomicIncrement( (LONG*)&g_cioreqInUse );

    //  pre-allocate all resources we will need to grow the pool

    Alloc( hEvent = CreateEventW( NULL, TRUE, FALSE, NULL ) );

    //  Attempt to get a new IOREQ storage slot

    IOREQ * pioreqT;
    Alloc( pioreqT = PioreqOSDiskIIOREQAllocSlot() );
    //  nothing after here must fail, otherwise we get an odd slot w/o a proper event handle

    //  init the new IOREQ storage slot ...

    OSDiskIIOREQInit( pioreqT, hEvent );
    hEvent = NULL;  // consumed event, ensure we don't free it ...

    //  add the new IOREQ to the pool

    OSDiskIIOREQFree( pioreqT );

HandleError:

    if ( hEvent )
    {
        CloseHandle( hEvent );
    }

    g_critIOREQPool.Leave();

    return err;
}

//  allocates an IOREQ, waiting for a free IOREQ if necessary
//
LOCAL ERR ErrOSDiskIOREQReserveAndLeak();
INLINE IOREQ* PioreqOSDiskIIOREQAlloc( BOOL fUsePreReservedIOREQs, BOOL * pfUsedTLSSlot )
{
    IOREQ *     pioreq;

    if ( pfUsedTLSSlot )
    {
        *pfUsedTLSSlot = fFalse;
    }

    //  user requests a pre-reserved IOREQ ...

    if ( fUsePreReservedIOREQs )
    {

        //  We should always have an available IOREQ, that's the purpose of the reserve pool!

        Assert( g_pioreqrespool->Cobject() >= 1 );

        if ( !g_pioreqrespool->Cobject() )
        {
            //  HOWEVER ... I want to attempt to recover, if we have a bug ...
            FireWall( "OutOfReservedIoreqs" );

            //  Get (AND LEAK!) another IOREQ to the reserve pool
            //
            (void)ErrOSDiskIOREQReserveAndLeak();

            //  If this func fails twice (b/c 1 IOREQ is initially in the reserve pool), then
            //  we tried, but we'll be horked.
        }

        //  allocate an IOREQ from the IOREQ reserve avail pool ... shouldn't ever have to wait.

        IOREQPool::ERR errIOREQ = g_pioreqrespool->ErrRemove( &pioreq );
        Assert( errIOREQ == IOREQPool::ERR::errSuccess );

        Assert( pioreq->FInReservePool() );
        Assert( pioreq->fFromReservePool );
        Assert( !pioreq->m_fFromTLSCache );
    }

    //  we have a cached IOREQ available

    else if ( Postls()->pioreqCache )
    {
        //  retrieve and return the cached IOREQ

        pioreq = Postls()->pioreqCache;
        Postls()->pioreqCache = NULL;

        Assert( pioreq->FCachedInTLS() );

        pioreq->m_fFromTLSCache = fTrue;

        Assert( !pioreq->fFromReservePool );

        //  Anyone who can get it from the TLS should know of it ... I think ... this means
        //  we won't reserve or unreserve on the IOThread.
        Assert( pfUsedTLSSlot );
        if ( pfUsedTLSSlot )
        {
            *pfUsedTLSSlot = fTrue;
        }
    }

    //  we don't have a cached IOREQ available

    else
    {
        //  allocate an IOREQ from the IOREQ pool, waiting forever if necessary

        pioreq = NULL;
        IOREQPool::ERR errAvail;

        if ( ( errAvail = g_pioreqpool->ErrRemove( &pioreq, cmsecTest ) ) == IOREQPool::ERR::errOutOfObjects )
        {
            //  there are no free IOREQs

            Assert( NULL == pioreq );
            const LONG cRFSCountdownOld = RFSThreadDisable( 10 );
            const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );
            do
            {
                //  try to grow the IOREQ pool

                Assert( NULL == pioreq );

                if ( ErrOSDiskIIOREQCreate() < JET_errSuccess )
                {
                    //  if we get out of memory, issue IOs we have on our thread ...
                    //  push IO request from tls first, then issue the IO.
                    COSDisk::EnqueueDeferredIORun( NULL );
                    OSDiskIOThreadStartIssue( NULL );
                }
            }
            while ( ( errAvail = g_pioreqpool->ErrRemove( &pioreq, 2 /* dtickFastRetry */ ) ) == IOREQPool::ERR::errOutOfObjects );
            RFSThreadReEnable( cRFSCountdownOld );
            FOSSetCleanupState( fCleanUpStateSaved );
        }
        Assert( IOREQPool::ERR::errSuccess == errAvail );

        Assert( !pioreq->fFromReservePool );
        Assert( !pioreq->m_fFromTLSCache );
    }

    Assert( pioreq );

#ifndef RTM
    pioreq->m_fDequeueCombineIO = fFalse;

    pioreq->m_fSplitIO = fFalse;
    pioreq->m_fReMergeSplitIO = fFalse;
    pioreq->m_fOutOfMemory = fFalse;
    pioreq->m_cTooComplex = 0;
#endif
    pioreq->m_cRetries = 0;

    //  initialise I/O timer to ensure it's got a sane value
    //
    pioreq->hrtIOStart = HrtHRTCount();

    //  track time from alloc (different than hrtIOStart which is reset when IO is started)
    //
    pioreq->m_tickAlloc = TickOSTimeCurrent();

    //  mark IOREQ as allocated
    //
    pioreq->SetIOREQType( IOREQ::ioreqAllocFromAvail );

    //  return the allocated IOREQ

    return pioreq;
}

//  This allows the pre-reservation of an IOREQ for specific usage.
//
//  note: usage of the IOREQ, is by calling PioreqOSDiskIIOREQAlloc( fTrue );

ERR ErrOSDiskIOREQReserve()
{
    //  reserving an IOREQ is as simple as moving an IOREQ via allocating it from the IOREQ pool, and 
    //  free'ing it to the reserve pool...

    AtomicIncrement( (LONG*)&g_cioreqrespool );
    IOREQ* pioreq = PioreqOSDiskIIOREQAlloc( fFalse, NULL );
    if ( !pioreq )
    {
        AtomicDecrement( (LONG*)&g_cioreqrespool );   // haha, just kidding...
        return ErrERRCheck( JET_errOutOfMemory );
    }

    //  put the IOREQ in the reserve available pool
    //

    pioreq->fFromReservePool = fTrue;
    OSDiskIIOREQFree( pioreq );

    return JET_errSuccess;
}

LOCAL ERR ErrOSDiskIOREQReserveAndLeak()
{
    //  We'll allow a limited number of IOREQs to be leaked to detect runaway resources ...
    const ULONG cioreqrespoolleakMax = 100;
    ULONG cioreqTemp;
    if ( !FAtomicIncrementMax( &g_cioreqrespoolleak, &cioreqTemp, cioreqrespoolleakMax ) )
    {
        AssertSz( fFalse, "I would not expect this to happen in anything, but a grossly mis-configured app, or an ESE bug reserving too much IO" );
        return ErrERRCheck( JET_errTooManyIO );
    }

    const ERR err = ErrOSDiskIOREQReserve();
    if ( err < JET_errSuccess )
    {
        AtomicDecrement( (LONG*)&g_cioreqrespoolleak );
    }

    return err;
}

//  Release an IOREQ from specific usage

VOID OSDiskIOREQUnreserve()
{
    //  Unreserving an IOREQ is merely the reverse of reserve, allocate it from the reserve pool, and
    //  free it back to the regular IOREQ pool

    IOREQ* pioreq = PioreqOSDiskIIOREQAlloc( fTrue, NULL );
    Assert( pioreq );   // this shouldn't happen

    if ( pioreq )
    {
        Assert( pioreq->fFromReservePool );
        pioreq->fFromReservePool = fFalse;
        OSDiskIIOREQFree( pioreq );
        AtomicDecrement( (LONG*)&g_cioreqrespool );
    }

}

//  frees an IOREQ to the IOREQ cache

INLINE void OSDiskIIOREQFreeToCache( IOREQ* pioreq )
{
    //  purge any IOREQs currently in the TLS cache

    if ( Postls()->pioreqCache )
    {
        OSDiskIIOREQFree( Postls()->pioreqCache );
        Postls()->pioreqCache = NULL;
    }

    //  put the IOREQ in the TLS cache

    Assert( !pioreq->FInReservePool() );
    pioreq->p_osf = NULL;
    pioreq->SetIOREQType( IOREQ::ioreqCachedInTLS );
    Postls()->pioreqCache = pioreq;
}

//  allocates an IOREQ from the IOREQ cache, returning NULL if empty

INLINE IOREQ* PioreqOSDiskIIOREQAllocFromCache()
{
    //  get IOREQ from cache, if any

    IOREQ* pioreq = Postls()->pioreqCache;

    //  clear cache

    Postls()->pioreqCache = NULL;

    //  return cached IOREQ, if any

    Assert( NULL == pioreq || pioreq->FCachedInTLS() );
    return pioreq;
}

#ifndef RTM
void IOREQ::AssertValid( void ) const
{

    //  This switch pretty well documents the approximate lifecycle of an IOREQ,
    //  some phases it skips, but for the most part it goes through each of these
    //  phases, with ioreqEnqueuedInIoHeap/ioreqEnqueuedInPriQ being most skipable (for sync IO).
    //
    switch( m_ioreqtype )
    {

        //  ------------------------------------------------
        //
        //  We have a few versions of "free" :)
        //
        case ioreqInAvailPool:
        case ioreqCachedInTLS:
        case ioreqInReservePool:

            Assert( FInAvailState() );
            AssertRTL( !m_fHasHeapReservation );
            AssertRTL( !m_fHasBackgroundReservation );
            AssertRTL( !m_fHasUrgentBackgroundReservation );
            AssertRTL( !m_fCanCombineIO );
            AssertRTL( NULL == p_osf );
            AssertRTL( m_posdCurrentIO == NULL );
            AssertRTL( m_ciotime == 0 );
            AssertRTL( m_iomethod == iomethodNone );
            AssertRTL( m_tc.etc.FEmpty() ); // shouldn't have any left over trace context

            switch( m_ioreqtype )   // validate the sub-state ...
            {
                case ioreqInAvailPool:
                    break;

                case ioreqCachedInTLS:
                    AssertRTL( Postls()->pioreqCache == NULL );     // the TLS slot must be empty to put this in
                    break;

                case ioreqInReservePool:
                    AssertRTL( fFromReservePool );
                    break;

                default:
                    AssertSz( fFalse, "Unknown available type!!!" );
            }
            break;

        //  ------------------------------------------------
        //
        //  Just allocated, pending enqueuing or issuing ...
        //
        case ioreqAllocFromAvail:
        case ioreqInIOCombiningList:
        case ioreqAllocFromEwreqLookaside:

            Assert( FInPendingState() );

            AssertRTL( m_ciotime == 0 );
            AssertRTL( m_iomethod == iomethodNone );

            AssertRTL( !m_fDequeueCombineIO );
            AssertRTL( !m_fSplitIO );
            AssertRTL( !m_fReMergeSplitIO );
            AssertRTL( !m_fOutOfMemory );
            AssertRTL( m_cTooComplex == 0 );
            AssertRTL( m_cRetries == 0 );

            switch( m_ioreqtype )   // validate the sub-state ...
            {
                //      IO just allocated ...
                //
                case ioreqAllocFromAvail:
                    AssertRTL( NULL == p_osf );
                    AssertRTL( m_tc.etc.FEmpty() ); // shouldn't have any left over trace context
                    break;

                //      IO "Enqueued" in TLS for batch Enqueuing.
                //
                case ioreqInIOCombiningList:
                    AssertRTL( p_osf );
                    AssertRTL( m_fHasHeapReservation || m_fCanCombineIO );
                    AssertRTL( m_tc.etc.iorReason.Iorp() != iorpNone ); // The ioreq should have an iorp associated (for tracing) in this state
                    break;

                //      IO just allocated from the PEWREQ lookaside reserved IOREQ ...
                //
                case ioreqAllocFromEwreqLookaside:
                    AssertRTL( p_osf );
                    AssertRTL( m_fHasHeapReservation );
                    AssertRTL( m_tc.etc.iorReason.Iorp() != iorpNone ); // The ioreq should have an iorp associated (for tracing) in this state
                    break;

                default:
                    AssertSz( fFalse, "Unknown pending type!!!" );
            }

            break;

        //  ------------------------------------------------
        //
        //  Enqueued in IO Queue / Heap
        //
        case ioreqEnqueuedInIoHeap:
        case ioreqEnqueuedInVipList:
        case ioreqEnqueuedInMetedQ:

            Assert( FInEnqueuedState() );
            AssertRTL( p_osf );

            AssertRTL( m_posdCurrentIO == NULL );
            AssertRTL( m_ciotime == 0 );
            AssertRTL( m_iomethod == iomethodNone );
            AssertRTL( m_tc.etc.iorReason.Iorp() != iorpNone ); // The ioreq should have an iorp associated (for tracing) in this state

            switch( m_ioreqtype )   // validate the sub-state ...
            {
                //      IO Enqueued in IO heap for sorted issuing.
                //
                case ioreqEnqueuedInIoHeap:
                    AssertRTL( m_fHasHeapReservation || m_fCanCombineIO );
                    break;

                case ioreqEnqueuedInMetedQ:
                    AssertRTL( m_fHasHeapReservation || m_fCanCombineIO );
                    break;

                //      IO pushed into VIP list due to IO heap constraints.
                //
                case ioreqEnqueuedInVipList:
                    // Now that we handle all forms of IO (when flighted) through the MetedQ, we 
                    // get the basic same conditions as above due to ibOffset conflicts.  We use
                    // dioqmFreeVipUpgrade to indicate this at enqueue time, but that flag isn't
                    // stored or available here.  For now - just disable.
                    //AssertRTL( !m_fCanCombineIO );
                    // These is now possible ... because the new MetedQ can not handle key dups (i.e. same file + offset) and then
                    // punts it to the VIP queue instead, we can have a VIP operation that has a heap res..  Leaving for context.
                    //AssertRTL( !m_fHasHeapReservation );
                    break;

                default:
                    AssertSz( fFalse, "Unknown enqueued type!!!" );
            }
            break;

        //  ------------------------------------------------
        //
        //  IO is being processed by IO Thread
        //

        case ioreqRemovedFromQueue:
            //  We still call this issued for 3 reasons:
            //  1. Because the IO thread is in the process of issuing this IO, meaning that it 
            //      could be circling through a chain of IOREQs, issuing each async.
            //  2. Because if we issue scatter/gather, we only update the head IOREQ with the
            //      ioreqIssuedAsyncIO state, and we leave the chain of IOREQs hanging off it 
            //      in the ioreqRemovedFromQueue state, so the whole IOREQ chain is technically 
            //      issued.
            //  3. Because internal Ops which don't go through an OS-level IO mechanism get 
            //      treated as if they're an IO (i.e. m_cioDispatching gets incremented/decremented
            //      for this).  Not that this aspect probably shouldn't be fixed at some point.
            Assert( FInIssuedState() );
            AssertRTL( p_osf );
            AssertRTL( m_ciotime == 0 );
            AssertRTL( m_iomethod == iomethodNone );
            AssertRTL( m_tc.etc.iorReason.Iorp() != iorpNone ); // The ioreq should have an iorp associated (for tracing) in this state
            break;

        //  ------------------------------------------------
        //
        //  IO is issued / dispatched to the OS
        //
        case ioreqIssuedSyncIO:         // sync IO (which is technically async IO to the OS)
        case ioreqIssuedAsyncIO:        // async IO (including degraded sync IO, async, scatter / gather)
            AssertRTL( m_posdCurrentIO );
            // except for "special op"(s) we should have iotime init'd
            AssertRTL( ( m_iomethod == iomethodNone && cbData == 0 ) ||  m_ciotime > 0 );
            AssertRTL( m_tc.etc.iorReason.Iorp() != iorpNone ); // The ioreq should have an iorp associated (for tracing) in this state

        case ioreqSetSize:
        case ioreqExtendingWriteIssued:

            Assert( FInIssuedState() );
            AssertRTL( p_osf );

            switch( m_ioreqtype )   // validate the sub-state ...
            {
                //      IO Issued "Sync"
                //
                case ioreqIssuedSyncIO:
                    // Can't assert this because of the abuse of this state from osfile.cxx's cbData == 0
                    // fake completions.  Should fix that.
                    //AssertRTL( m_iomethod != iomethodNone );
                    break;

                //      IO Issued Async
                //
                case ioreqIssuedAsyncIO:
                    AssertRTL( m_iomethod != iomethodNone );
                    break;

                //      IO Issued / used for extending the file
                //
                case ioreqSetSize:
                    AssertRTL( m_iomethod == iomethodNone );
                    break;

                case ioreqExtendingWriteIssued:
                    AssertRTL( m_iomethod == iomethodNone );
                    break;

                default:
                    AssertSz( fFalse, "Unknown issued/dispatched type!!!" );
            }
            break;

        //  ------------------------------------------------
        //
        //  IO is completing / completed from the OS
        //

        case ioreqCompleted:

            Assert( FCompleted() );

            AssertRTL( p_osf );

            AssertRTL( m_ciotime == 0 );
            AssertRTL( m_iomethod == iomethodNone );
            AssertRTL( m_tc.etc.iorReason.Iorp() != iorpNone ); // The ioreq should have an iorp associated (for tracing) in this state

            break;

        //  ------------------------------------------------
        //  Huh?
        default:
            AssertSz( fFalse, "Unknown IOREQ type (%d)!!!", m_ioreqtype );

    }
}
#endif

VOID IOREQ::ValidateIOREQTypeTransition( const IOREQTYPE ioreqtypeNext )
{
#ifndef RTM
    //  Switch validates the state transition is allowed.
    //
    switch( ioreqtypeNext )
    {

        //  ------------------------------------------------
        //
        //  We have a few versions of "free" :)
        //
        case ioreqInAvailPool:
            AssertRTL( 0 ==  m_ioreqtype ||         // post init, first add to avail pool
                        FAllocFromAvail() ||    // re-free'd immediately ... such as if not enough IO heap space.
                        FCachedInTLS() ||   // freeing TLS cached IOREQ to general pool
                        FCompleted() );
            break;

        case ioreqCachedInTLS:
            AssertRTL( FCompleted() );
            break;

        case ioreqInReservePool:
            AssertRTL( FAllocFromAvail() || // happens when ErrOSDiskIOREQReserve is called.
                        FCompleted() );
            break;

        //  ------------------------------------------------
        //
        //  Just allocated, pending enqueuing or issuing ...
        //
        //      IO just allocated ...
        //
        case ioreqAllocFromAvail:
            AssertRTL( FInAvailState() );
            break;

        //      IO allocated from the pewreq lookaside IOREQ stored there at extending write issue
        //
        case ioreqAllocFromEwreqLookaside:
            //  Could FCompleted() also come through here?
            AssertRTL( FExtendingWriteIssued() );
            break;

        //      IO "Enqueued" in TLS for batch Enqueuing.
        //
        case ioreqInIOCombiningList:
            AssertRTL( FAllocFromAvail() ||
                        FAllocFromEwreqLookaside() );
            break;

        //  ------------------------------------------------
        //
        //  Enqueued in IO Queue / Heap
        //
        //      IO Enqueued in IO heap for sorted issuing.
        //
        case ioreqEnqueuedInIoHeap:
        case ioreqEnqueuedInMetedQ:
            AssertRTL( FInPendingState() ||
                        FRemovedFromQueue() ||  // due to out of memory on async scatter/gather OS IO op, IO may be pushed to heap.
                        FIssuedSyncIO() ||      // due to out of memory on sync OS IO op, IO may be pushed to heap.
                        FIssuedAsyncIO() );     // due to out of memory on (any) "async" OS IO op, IO may come back to heap.
            break;

        //      IO pushed into VIP list due to IO heap constraints.
        //
        case ioreqEnqueuedInVipList:
            AssertRTL( FInPendingState() ||
                        FEnqueuedInMetedQ() ||  // due to conflicted IOREQs in meted Q, getting free upgrade to VIP path
                        FIssuedSyncIO() ||      // due to out of memory on sync OS IO op, IO may be pushed to VIP list instead.
                        FIssuedAsyncIO() );     // due to out of memory on async OS IO op, IO may be pushed to VIP list instead.
            break;

        //  ------------------------------------------------
        //
        //  IO is being processed by IO Thread
        //
        //      IO removed from queue (heap | VIP)
        //
        //  Note: In the case of non-head IOREQs, this can be an issued to OS IOREQ, see IOREQ::AssertValid()

        case ioreqRemovedFromQueue:
            AssertRTL( FInEnqueuedState() );
            break;


        //  ------------------------------------------------
        //
        //  IO is issued / dispatched to the OS
        //

        //      IO Issued Sync from foreground thread
        //
        case ioreqIssuedSyncIO:
            AssertRTL( FAllocFromAvail() ||
                        FAllocFromEwreqLookaside() );
            break;

        //      IO Issued from IO Thread
        //
        case ioreqIssuedAsyncIO:
            AssertRTL( FRemovedFromQueue() );
            break;

        //      IO Issued / used for extending the file
        //
        case ioreqSetSize:
            AssertRTL( FAllocFromAvail() );
            break;

        case ioreqExtendingWriteIssued:
            AssertRTL( FAllocFromAvail() ||
                        //  file may not be extended enough, and so we may have to just re-defer it
                        FAllocFromEwreqLookaside() );
            break;


        //  ------------------------------------------------
        //
        //  IO is completing / completed from the OS
        //

        case ioreqCompleted:
            AssertRTL( FInIssuedState() );

            //  Should have lock to instigate this state transition
            
            Assert( m_crit.FOwner() );
            break;
 
        //  ------------------------------------------------
        //  Huh?
        default:
            AssertSz( fFalse, "Unknown IOREQ type!!!" );

    }
#endif
}

//  Determines if IOREQ is supposed to be exclusively executed against disk.

const BOOL FExclusiveIoreq( const IOREQ * const pioreqHead )
{
    BOOL fRet = ( pioreqHead->grbitQOS & qosIOReadCopyMask );
    if ( fRet )
    {
        Assert( qosIODispatchImmediate & ( pioreqHead->grbitQOS & qosIODispatchMask ) );
        Assert( pioreqHead->pfnCompletion == PFN( COSFile::IOSyncComplete_ ) );
        Assert( pioreqHead->pioreqIorunNext == NULL );
        Assert( !FIOThread() );
    }

    return fRet;
}


//  This sets the IOREQTYPE to the requested type ...

void IOREQ::SetIOREQType( const IOREQTYPE ioreqtypeNext, const IOMETHOD iomethod )
{
    // Check our intended state transition is valid.

    ValidateIOREQTypeTransition( ioreqtypeNext );

    //  Perform the state transition.

    m_ioreqtype = ioreqtypeNext;

    if ( iomethod != iomethodNone )
    {
        m_iomethod = iomethod;
    }

    //  Check IOREQ is valid.

    ASSERT_VALID( this );

}

VOID IOREQ::BeginIO( const IOMETHOD iomethod, const HRT hrtNow, const BOOL fHead )
{
    //  start timing stats

    Assert( m_ciotime == 0 );
    Assert( p_osf );

    hrtIOStart = hrtNow;

    const LONG ciotimePre = AtomicCompareExchange( (LONG*)&(m_ciotime), 0, 1 );

    AssertRTL( ciotimePre == 0 );

    g_patrolDogSync.EnterPerimeter();
    Assert( m_ciotime > 0 );

    Assert( hrtIOStart > 0 );
    Assert( m_posdCurrentIO == NULL );
    Assert( m_iomethod == iomethodNone );

    //  some settings are only applicable to head IOREQs

    if ( fHead )
    {
        Assert( iomethodNone != iomethod );

        // note: hrtIOStart not reset

        //  attach the IOREQ to the disk

        m_posdCurrentIO = p_osf->m_posd;
        Assert( m_posdCurrentIO );

        //  update type/state variable

        if ( FIOThread() )
        {
            SetIOREQType( IOREQ::ioreqIssuedAsyncIO, iomethod );
        }
        else
        {
            Assert( iomethodSemiSync == iomethod );
            SetIOREQType( IOREQ::ioreqIssuedSyncIO, iomethod );
        }

        Assert( m_posdCurrentIO );
    }
}

VOID IOREQ::CompleteIO(
    const COMPLETION_STATUS eCompletionStatus
    )
{
    Assert( m_posdCurrentIO || m_iomethod == iomethodNone );

    Assert( FInIssuedState() );

    m_crit.Enter();

#ifdef DEBUG
    if ( m_iomethod == iomethodNone )
    {
        //  Non-OS-active IOREQs

        Assert( FOtherIssuedState() ||  // set size or extending write fake completions
                FRemovedFromQueue() ||      // not the head IOREQ ...
                ( cbData == 0 && FIssuedSyncIO() ) /* other fake completions */ );
    }
    else
    {
        //  Active OS IO
        //

        Assert( m_ciotime > 0 );
    }
#endif  // DEBUG

    //  don't need tracking anymore.
    //

    if ( m_ciotime > 0 )
    {
        g_patrolDogSync.LeavePerimeter();
    }

    //  Reset the IO time, need a convergence loop
    //

    while ( m_ciotime > 0 )
    {
        //  Atomically reset the ciotime
        //

        (void)AtomicCompareExchange( (LONG*)&(m_ciotime), m_ciotime, 0 );
    }
    Assert( m_ciotime == 0 );

    //  We're no longer undergoing IO, so reset the iomethod
    //

    m_iomethod = iomethodNone;

    //  Finally, update the IOREQTYPE to be completed if we completed
    //
    
    if ( eCompletionStatus == CompletedIO )
    {
        //  Yeah we completed an IO!  Well ... we may have failed the IO, but we still
        //  completed it.

        SetIOREQType( ioreqCompleted );
    }
    else
    {
        //  We are in here only to reset the m_iomethod, not force a type/state transistion ...

        Assert( eCompletionStatus == ReEnqueueingIO );
    }

    Assert( !FInIssuedState() || eCompletionStatus == ReEnqueueingIO );
    Assert( m_ciotime == 0 );
    Assert( m_iomethod == iomethodNone );

    m_posdCurrentIO = NULL;

    //  In case any hung IO actions were taken, clear them so the next IO can be
    //  processed for being a hung IO.

    m_grbitHungActionsTaken = 0;

    m_crit.Leave();
}

#ifdef _WIN64
C_ASSERT( sizeof(OVERLAPPED) == 32 );
C_ASSERT( sizeof( CPool< IOREQ, IOREQ::OffsetOfAPIC >::CInvasiveContext ) == 16 );
C_ASSERT( (OffsetOf( IOREQ, m_apic )%8) == 0 );     // check ic alignment, made more sense when was union on rgbAPIC
C_ASSERT( sizeof(IOREQ) <= 256+32 );                    // be conscious if you use egregious space.
C_ASSERT( (sizeof(IOREQ)%8) == 0 );                 // check minimum alignment

#else
C_ASSERT( sizeof(OVERLAPPED) == 20 );
C_ASSERT( sizeof( CPool< IOREQ, IOREQ::OffsetOfAPIC >::CInvasiveContext ) == 8 );
C_ASSERT( (OffsetOf( IOREQ, m_apic )%4) == 0 );     // check ic alignment, made more sense when was union on rgbAPIC
C_ASSERT( sizeof(IOREQ) <= 176 + 64 );                  // be conscious if you use egregious space.
C_ASSERT( (sizeof(IOREQ)%4) == 0 );                 // check minimum alignment

#endif


//  processes (read-write) or enumerates (read-only) all IOREQs in the pool

//  note this is an inherently unsafe function, but often we know that we can use this
//  on each IOREQ.  E.g. ensuring no references to a _OSFILE* during tear down of a file
//  handle, but immediately before actually freeing the memory.  But we couldn't say for
//  example ASSERT_VALID( pioreq ) on each.

typedef ERR (*PfnErrProcessIOREQ)( _In_ ULONG ichunk, _In_ ULONG iioreq, _In_ IOREQ * pioreq, void * pctx );
typedef ERR (*PfnErrEnumerateIOREQ)( _In_ ULONG ichunk, _In_ ULONG iioreq, _In_ const IOREQ * pioreq, void * pctx );

ERR ErrProcessIOREQs( _In_ PfnErrProcessIOREQ pfnErrProcessIOREQ, void * pvCtx )
{
    ERR err = JET_errSuccess;

    g_critIOREQPool.Enter();

    ULONG ichunk = 0;
    IOREQCHUNK * pioreqchunk = g_pioreqchunkRoot;
    while ( pioreqchunk )
    {
        for ( DWORD iioreq = 0; iioreq < pioreqchunk->cioreqMac; iioreq++ )
        {
            Call( pfnErrProcessIOREQ( ichunk, iioreq, &(pioreqchunk->rgioreq[iioreq]), pvCtx ) );
        }
        pioreqchunk = pioreqchunk->pioreqchunkNext;
        ichunk++;
    }

HandleError:

    g_critIOREQPool.Leave();

    return err;
}

ERR ErrEnumerateIOREQs( _In_ PfnErrEnumerateIOREQ pfnErrEnumerateIOREQ, void * pvCtx )
{
    return ErrProcessIOREQs( (PfnErrProcessIOREQ) pfnErrEnumerateIOREQ, pvCtx );
}


#ifdef DEBUG

//  a callback for ErrEnumerateIOREQs() that prints all IOREQs and thier current state.

ERR ErrEnumerateIOREQsPrint( _In_ ULONG ichunk, _In_ ULONG iioreq, _In_ const IOREQ * pioreq, void * pvCtx )
{
    wprintf(L"IOREQ[%d.%d] = %p, state = %d\n", ichunk, iioreq, pioreq, pioreq->Ioreqtype() );
    return JET_errSuccess;
}


ERR ErrEnumerateIOREQsAssertValid( _In_ ULONG ichunk, _In_ ULONG iioreq, _In_ const IOREQ * pioreq, void * pvCtx )
{
    ASSERT_VALID( pioreq );
    return JET_errSuccess;
}

#endif

#ifndef RTM

//  checks that a pioreq pointer is actually in our quota of IOREQs
//
//  a little slow, not terribly slow ... except in debug where the chunk 
//  size is small.

BOOL FOSDiskIIOREQSlowlyCheckIsOurOverlappedIOREQ( const IOREQ * pioreq )
{
    BOOL fOk = fFalse;

    //  whenever you take a lock on the IO completion path you have the potential to 
    //  stop IOREQ processing / completion and thus freezing everything if the allocation 
    //  side has your lock then it can deadlock and is waiting for an IOREQ to be free'd.  
    //  The reason I don't think this is the case here, b/c this lock is for allocating 
    //  an _actual_ NEW IOREQs / new IOREQCHUNK as opposed to waiting for an IOREQ to be 
    //  released to the pool.  So I _think_ this is safe.  I'll update the comment.  This 
    //  is only !RTM and/or DEBUG also.
    g_critIOREQPool.Enter();

    IOREQCHUNK * pioreqchunk = g_pioreqchunkRoot;
    while ( pioreqchunk )
    {
        if ( (BYTE*)pioreq >= (BYTE*)pioreqchunk &&
                (BYTE*)pioreq < (BYTE*)( ((IOREQ*)pioreq) + pioreqchunk->cioreqMac ) )
        {
            fOk = fTrue;
            break;
        }
        pioreqchunk = pioreqchunk->pioreqchunkNext;
    }

    g_critIOREQPool.Leave();

    return fOk;
}

#endif

//  terminates the IOREQ pool

void OSDiskIIOREQPoolTerm()
{
    //  term IOREQ pool

    if ( g_pioreqpool )
    {
        g_pioreqpool->Term();
        g_pioreqpool->IOREQPool::~IOREQPool();
        OSMemoryHeapFreeAlign( g_pioreqpool );
        g_pioreqpool = NULL;
    }

    if ( g_pioreqrespool )
    {
        //  either during sysinit / OSDiskIIOREQInit() allocation failure or should
        //  otherwise have 1 left ... if not, we leaked a res IOREQ "ref" somewhere, or
        //  we "deref'd" res IOREQs too many times ...
        Assert( 0 == g_pioreqrespool->Cobject() || 1 == g_pioreqrespool->Cobject() );

        g_pioreqrespool->Term();
        g_pioreqrespool->IOREQPool::~IOREQPool();
        OSMemoryHeapFreeAlign( g_pioreqrespool );
        g_pioreqrespool = NULL;
    }

#ifdef DEBUG
    //  validate nothing fluky

    CallS( ErrEnumerateIOREQs( ErrEnumerateIOREQsAssertValid, NULL ) );
#endif

    //  free IOREQ storage if allocated

    g_critIOREQPool.Enter();

    while ( g_pioreqchunkRoot )
    {
        IOREQCHUNK * const pioreqchunkNext = g_pioreqchunkRoot->pioreqchunkNext;

        for ( DWORD iioreq = 0; iioreq < g_pioreqchunkRoot->cioreqMac; iioreq++ )
        {
            if ( g_pioreqchunkRoot->rgioreq[iioreq].ovlp.hEvent )
            {
                SetHandleInformation( g_pioreqchunkRoot->rgioreq[iioreq].ovlp.hEvent, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
                CloseHandle( g_pioreqchunkRoot->rgioreq[iioreq].ovlp.hEvent );
            }
            g_pioreqchunkRoot->rgioreq[iioreq].~IOREQ();
        }
        //  Remove this chunk from the list.
        g_pioreqchunkRoot->pioreqchunkNext = NULL;
        OSMemoryHeapFree( g_pioreqchunkRoot );
        g_pioreqchunkRoot = pioreqchunkNext;
    }

    g_critIOREQPool.Leave();

}

//  initializes the IOREQ pool, or returns JET_errOutOfMemory

ERR ErrOSDiskIIOREQPoolInit()
{
    ERR     err;

    //  reset all pointers

    Assert( NULL == g_pioreqchunkRoot );    // otherwise we're leaking?

    g_pioreqchunkRoot = NULL;
    g_cioreqInUse = 0;
    g_pioreqpool  = NULL;
    g_pioreqrespool = NULL;

    //  allocate IOREQ storage

    C_ASSERT( ( 1024 - sizeof( IOREQCHUNK ) ) >= 4 );
    g_cbIoreqChunk = 5 * sizeof( IOREQ );

    Alloc( g_pioreqchunkRoot = (IOREQCHUNK*)PvOSMemoryHeapAlloc( g_cbIoreqChunk ) );
    memset( g_pioreqchunkRoot, 0, g_cbIoreqChunk );
    g_pioreqchunkRoot->cioreq = CioreqPerChunk( g_cbIoreqChunk );
    g_pioreqchunkRoot->cioreqMac = 0;
    g_pioreqchunkRoot->pioreqchunkNext = NULL;

    //  allocate IOREQ pool in cache aligned memory

    BYTE *rgbIOREQPool;
    rgbIOREQPool = (BYTE*)PvOSMemoryHeapAllocAlign( sizeof( IOREQPool ), cbCacheLine );
    if ( !rgbIOREQPool )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }
    g_pioreqpool = new( rgbIOREQPool ) IOREQPool();

    //  init IOREQ pool

    switch ( g_pioreqpool->ErrInit( 0.0 ) )
    {
        default:
            AssertSz( fFalse, "Unexpected error initializing IOREQ Pool" );
        case IOREQPool::ERR::errOutOfMemory:
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        case IOREQPool::ERR::errSuccess:
            break;
    }

    //  free an initial IOREQ to the pool

    Call( ErrOSDiskIIOREQCreate() );


    //  allocate IOREQ reserve available pool 
    // 

    // reusing BYTE *rgbIOREQPool;
    rgbIOREQPool = (BYTE*)PvOSMemoryHeapAllocAlign( sizeof( IOREQPool ), cbCacheLine );
    if ( !rgbIOREQPool )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }
    g_cioreqrespool = g_cioreqrespoolleak = 0;
    g_pioreqrespool = new( rgbIOREQPool ) IOREQPool();

    //  init IOREQ reserve pool

    switch ( g_pioreqrespool->ErrInit( 0.0 ) )
    {
        default:
            AssertSz( fFalse, "Unexpected error initializing IOREQ Pool" );
        case IOREQPool::ERR::errOutOfMemory:
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        case IOREQPool::ERR::errSuccess:
            break;
    }

    return JET_errSuccess;

HandleError:
    OSDiskIIOREQPoolTerm();
    return err;
}


//  IO Time Constants
//

//  IO Time
//
//  The IO Time can give us a range of real times based upon the granularity
//  of g_dtickIOTime.  So assuming g_dtickIOTime = 5 seconds ...
//
//  ciotime   low   high
//      1   =   0 -  5 seconds
//      2   =   0 -  5 seconds
//      3   =   5 - 10 seconds
//      4   =  10 - 15 seconds
//      5   =  15 - 20 seconds
//      etc ...

const TICK                  g_dtickIOTime = OnDebugOrRetail( 150, 4 * 1000 );
const LONG                  g_ciotimeMax = 0x7fff;

C_ASSERT( ( (__int64)g_dtickIOTime * (__int64)g_ciotimeMax ) < (__int64)0x7fffffff /* check for wrap */ );
C_ASSERT( g_dtickIOTime != 0 );

//  PatrolDog's Process IOs data context
//

typedef struct tagHUNGIOCTX
{
    DWORD                                           cioActive;
    DWORD                                           ciotimeLongestOutstanding;
    QWORD                                           cmsecLongestOutstanding;

    CInvasiveList< IOREQ, IOREQ::OffsetOfHIIC >     ilHungIOs;
} HUNGIOCTX;


// =============================================================================================
//  I/O "WatchDog" / PatrolDog Functionality
// =============================================================================================

//
//  Synchronization object to control PatrolDog
//

void PatrolDogSynchronizer::InitPatrolDogState_()
{
    m_asigNudgePatrolDog.Reset();
    m_fPutDownPatrolDog = fTrue;
    m_cLoiters = 0;
    m_threadPatrolDog = NULL;
    m_pvContext = NULL;
    m_patrolDogState = pdsInactive;
    AssertPatrolDogInitialState_();
}

void PatrolDogSynchronizer::AssertPatrolDogInitialState_( const PatrolDogState pdsExpected ) const
{
    Assert( !m_asigNudgePatrolDog.FIsSet() );
    Assert( m_fPutDownPatrolDog );
    Assert( m_cLoiters == 0 );
    Assert( m_threadPatrolDog == NULL );
    Assert( m_pvContext == NULL );
    Assert( m_patrolDogState == pdsExpected );
}

PatrolDogSynchronizer::PatrolDogSynchronizer() :
    m_asigNudgePatrolDog( CSyncBasicInfo( "PatrolDogSynchronizer::m_asigNudgePatrolDog" ) )
{
    InitPatrolDogState_();
}

PatrolDogSynchronizer::~PatrolDogSynchronizer()
{
#ifdef DEBUG
    if ( !FUtilProcessAbort() )
    {
        AssertPatrolDogInitialState_();
    }
#endif  // DEBUG
}

ERR PatrolDogSynchronizer::ErrInitPatrolDog( const PUTIL_THREAD_PROC pfnPatrolDog,
                                                const EThreadPriority priority,
                                                void* const pvContext )
{
    ERR err = JET_errSuccess;

    //  Parameter validation.

    if ( pfnPatrolDog == NULL )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    //  Verify state transition.

    LONG state = AtomicCompareExchange( (LONG*)&m_patrolDogState, pdsInactive, pdsActivating );

    if ( state != pdsInactive )
    {
        return ErrERRCheck( JET_errInvalidOperation );
    }

    AssertPatrolDogInitialState_( pdsActivating );

    m_fPutDownPatrolDog = fFalse;
    m_pvContext = pvContext;
    Call( ErrUtilThreadCreate(  PUTIL_THREAD_PROC( pfnPatrolDog ),
                                OSMemoryPageReserveGranularity(),
                                priority,
                                &m_threadPatrolDog,
                                (DWORD_PTR)this ) );
    Assert( m_threadPatrolDog != NULL );

    //  WARNING: point of no return.

    m_patrolDogState = pdsActive;

HandleError:

    if ( err < JET_errSuccess )
    {
        Assert( m_threadPatrolDog == NULL );
        Assert( m_patrolDogState == pdsActivating );
        InitPatrolDogState_();
    }

    return err;
}

void PatrolDogSynchronizer::TermPatrolDog()
{
    //  Verify state transition.

    LONG state = AtomicCompareExchange( (LONG*)&m_patrolDogState, pdsActive, pdsDeactivating );

    if ( state != pdsActive )
    {
        return;
    }

    //  Assert current state.

    Assert( !m_fPutDownPatrolDog );
    Assert( m_cLoiters == 0 );
    Assert( m_threadPatrolDog != NULL );

    //  Deactivate thread.

    m_fPutDownPatrolDog = fTrue;
    m_asigNudgePatrolDog.Set();
    (void)UtilThreadEnd( m_threadPatrolDog );
    Assert( m_cLoiters == 0 );

    InitPatrolDogState_();
}

void PatrolDogSynchronizer::EnterPerimeter()
{
    Expected( m_patrolDogState == pdsActive );
    const LONG cLoitersNew = AtomicIncrement( (LONG*)&m_cLoiters );
    Assert( cLoitersNew > 0 );

    //  May need to nudge the patrol dog.

    if ( cLoitersNew == 1 )
    {
        m_asigNudgePatrolDog.Set();
    }
}

void PatrolDogSynchronizer::LeavePerimeter()
{
    Expected( m_patrolDogState == pdsActive );
    const LONG cLoitersNew = AtomicDecrement( (LONG*)&m_cLoiters );
    Assert( cLoitersNew >= 0 );

    //  May need to nudge the patrol dog.

    if ( cLoitersNew == 0 )
    {
        m_asigNudgePatrolDog.Set();
    }
}

BOOL PatrolDogSynchronizer::FCheckForLoiter( const TICK dtickTimeout )
{
    Assert( m_patrolDogState == pdsActivating || m_patrolDogState == pdsActive || m_patrolDogState == pdsDeactivating );

    //  Our sync library does not support more than that because it treats negative values differently.

    Assert( dtickTimeout <= INT_MAX );

    BOOL fLoitersBefore = fFalse;

    OSSYNC_FOREVER
    {
        //  Bail out immediately if we got signaled to exit.

        if ( m_fPutDownPatrolDog )
        {
            return fFalse;
        }

        const BOOL fLoitersNow = ( m_cLoiters > 0 );

        //  If there are loiters now and there were before, it's time to check things out.

        if ( fLoitersNow && fLoitersBefore )
        {
            return fTrue;
        }

        //  If there aren't any loiters right now, wait forever.
        //  If there are loiters now and there weren't before (otherwise, we would have returned above),
        //  wait for the requested timeout. Note that the timeout to wait may be slightly off the
        //  default (and thus innacurate) for subsequent waits (i.e., iterations within this loop).

        const INT cmsecTimeout = !fLoitersNow ? cmsecInfiniteNoDeadlock : -( (INT)min( dtickTimeout, INT_MAX ) );
        fLoitersBefore = fLoitersNow;
        (void)m_asigNudgePatrolDog.FWait( cmsecTimeout );
    }
}

void* PatrolDogSynchronizer::PvContext() const
{
    return m_pvContext;
}

SIZE_T PatrolDogSynchronizer::CbSizeOf() const
{
    return sizeof( PatrolDogSynchronizer );
}


// --------------------------------------------
//  IO PatrolDog Sub-System Init/Term
//

//  globals

PatrolDogSynchronizer g_patrolDogSync;

//  terminates the I/O PatrolDog thread

void OSDiskIIOPatrolDogThreadTerm( void )
{
    g_patrolDogSync.TermPatrolDog();
}

// fwd decl

DWORD IOMgrIOPatrolDogThread( DWORD_PTR dwContext );

//  initializes the I/O PatrolDog thread, or returns either JET_errOutOfMemory or
//  JET_errOutOfThreads

ERR ErrOSDiskIIOPatrolDogThreadInit( void )
{
    ERR err;

    Call( g_patrolDogSync.ErrInitPatrolDog( IOMgrIOPatrolDogThread,
                                            priorityAboveNormal,
                                            NULL ) );
    return JET_errSuccess;

HandleError:

    Assert( err < JET_errSuccess );
    Expected( err != JET_errInvalidOperation && err != JET_errInvalidParameter );

    OSDiskIIOPatrolDogThreadTerm();

    return err;
}

//  returns the lowest possible time this IO has been outstanding based
//  upon it's ciotime.

ULONG CmsecLowWaitFromIOTime( const ULONG ciotime )
{
    // Must be 2 or greater because if an IOREQ goes 0 -> 1 (for dispatched state) and
    // then the patrol dog immediately drops by, it'll go 1 -> 2, and still mean we've
    // got between 0 - N seconds ... so we subtract 2 for low-estimate wait time.
    if ( ciotime > 2 )
    {
        const ULONG cmsec = ( ciotime - 2 ) * g_dtickIOTime;
        //  may be as low as zero
        Assert( cmsec <= ( g_dtickIOTime * g_ciotimeMax ) /* check for wrap */ );
        return cmsec;
    }
    return 0;
}

//  returns the highest possible time this IO has been outstanding based
//  upon it's ciotime.

ULONG CmsecHighWaitFromIOTime( const ULONG ciotime )
{
    if ( ciotime >= 2 )
    {
        const ULONG cmsec = ciotime * g_dtickIOTime;
        Assert( cmsec >= g_dtickIOTime / 2 );
        Assert( cmsec <= ( g_dtickIOTime * g_ciotimeMax ) /* check for wrap */ );
        return cmsec;
    }
    return g_dtickIOTime;
}

VOID IOREQ::IncrementIOTime()
{
    //  Testing the size of and atomically modifiableness of the m_ciotime ...
    C_ASSERT( sizeof(m_ciotime) == 4 );
    C_ASSERT( ( OffsetOf( IOREQ, m_ciotime ) % 4 ) == 0 );

    //  convergence loop, 
    //      if IOREQ is outstanding ( ciotimeCurrent = m_ciotime > 0 )
    //          and ciotime won't overflow ( ciotimeCurrent < 0x7fff | ciotimeCurrent < g_ciotimeMax )
    //              Note: 0x7fff accounts for potential future needs
    //          THEN try to increment by one
    //      retry if pre-conditions fail ...
    //

    //  we want to perform the update if there is a m_ciotime and it's less 
    //  than g_ciotimeMax ...

    DWORD ciotimePre;

    //  Be nice to use FAtomicIncrementMax( &m_ciotime, &ciotimePre, g_ciotimeMax + 2 ), but we
    //  do not want to increment off m_ciotime = 0.

    while( ( ciotimePre = m_ciotime ) > 0 &&
            ciotimePre < g_ciotimeMax )
    {

        //  Atomically increment the ciotime
        //

        const DWORD ciotimeCheck = AtomicCompareExchange( (LONG*)&(m_ciotime), ciotimePre, ciotimePre + 1 );

        //  If successful break out ...
        //

        if ( ciotimeCheck == ciotimePre )
        {
            break;
        }

        //  otherwise try again if the conditions for incrementing 
        //  the m_ciotime still apply ...

    }

}


ERR ErrIOMgrPatrolDogICheckOutIOREQ( _In_ ULONG ichunk, _In_ ULONG iioreq, _In_ IOREQ * pioreq, void * pctx )
{
    //  Only update stats and detect hung I/Os for head IOREQs (in practice, one real I/O).
    //

    if ( pioreq->FOSIssuedState() )
    {
        pioreq->m_crit.Enter();

        //  Re-check after acquiring critical section.
        //

        if ( pioreq->FOSIssuedState() )
        {
            HUNGIOCTX * const pHungIOCtx = reinterpret_cast<HUNGIOCTX *>( pctx );

            //  Track how many outstanding / active IOs we have
            //

            if ( pioreq->m_ciotime > 0 )
            {
                pHungIOCtx->cioActive++;
            }

            //  For fun, lets track the longest outstanding IO (via both wall clock time and debugger-less runtime) ...
            //

            const QWORD cmsec = CmsecHRTFromDhrt( HrtHRTCount() - pioreq->hrtIOStart );

            if ( pioreq->m_ciotime && cmsec > pHungIOCtx->cmsecLongestOutstanding )
            {
                pHungIOCtx->cmsecLongestOutstanding = cmsec;
            }

            if ( pioreq->m_ciotime > pHungIOCtx->ciotimeLongestOutstanding )
            {
                pHungIOCtx->ciotimeLongestOutstanding = pioreq->m_ciotime;
            }

            //  Now lets add to a list any IOs that appear over our hung IO threshold ...
            //

            if ( CmsecLowWaitFromIOTime( pioreq->m_ciotime ) > pioreq->p_osf->Pfsconfig()->DtickHungIOThreshhold() ||
                 FFaultInjection( 36002 ) )
            {
                pHungIOCtx->ilHungIOs.InsertAsPrevMost( pioreq );
            }
        }

        pioreq->m_crit.Leave();
    }

    //  Maintain the IO Time counts
    //

    pioreq->IncrementIOTime();

    return JET_errSuccess;
}

//  Describes the file or volume to disk mapping mode.
//
enum OSDiskIReportHungIOFailureItem {
    eOSDiskIReportHungFailureItemLowThreshold = 0,
    eOSDiskIReportHungFailureItemMediumThreshold = 1,
    eOSDiskIReportHungFailureItemExceededThreshold = 2,
    eOSDiskIReportHungFailureItemExceededThresholdDoubleDisk = 3,
};

#if defined( USE_HAPUBLISH_API )
INLINE HaDbFailureTag OSDiskIHaTagOfFailureItemEnum( const OSDiskIReportHungIOFailureItem failureItem )
{
    switch ( failureItem )
    {
        case eOSDiskIReportHungFailureItemLowThreshold:
            return HaDbFailureTagHungIoLowThreshold;

        case eOSDiskIReportHungFailureItemMediumThreshold:
            return HaDbFailureTagHungIoMediumThreshold;

        case eOSDiskIReportHungFailureItemExceededThreshold:
            return HaDbFailureTagHungIoExceededThreshold;

        case eOSDiskIReportHungFailureItemExceededThresholdDoubleDisk:
            return HaDbFailureTagHungIoExceededThresholdDoubleDisk;

        default:
            AssertSz( fFalse, "Unexpected failure item enum: %d", failureItem );
            return HaDbFailureTagNoOp;
    }
}
#endif

VOID OSDiskIReportHungIO(
    const IOREQ *   pioreqHead,
    const DWORD     csecHangTime,
    const OSDiskIReportHungIOFailureItem        failureItem     // indicates which failure item we should log.
    )
{
    const COSDisk::IORun    iorun( (IOREQ*)pioreqHead );

    //  Injecting concurrency between establishing the IO run, tells us if we have complete control
    //  of this iorun until we're done.
    if ( pioreqHead->pioreqIorunNext && FFaultInjection( 36002 )  )
    {
        UtilSleep( 1 );
    }

    Assert( pioreqHead->FOwner() );

    IFileAPI * pfapi = (IFileAPI*)pioreqHead->p_osf->keyFileIOComplete; //  HACK!

    const WCHAR *   rgpwsz[4];
    WCHAR           wszAbsPath[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszOffset[ 64 ];
    WCHAR           wszLength[ 64 ];
    WCHAR           wszHangTime[ 64 ];

    OSStrCbFormatW( wszOffset, sizeof( wszOffset ), L"%I64i (0x%016I64x)", iorun.IbOffset(), iorun.IbOffset() );
    OSStrCbFormatW( wszLength, sizeof( wszLength ), L"%u (0x%08x)", iorun.CbRun(), iorun.CbRun() );
    OSStrCbFormatW( wszHangTime, sizeof( wszHangTime ), L"%u", csecHangTime );

    CallS( pfapi->ErrPath( wszAbsPath ) );
    rgpwsz[0]    = wszAbsPath;
    rgpwsz[1]    = wszOffset;
    rgpwsz[2]    = wszLength;
    rgpwsz[3]    = wszHangTime;

    pioreqHead->p_osf->Pfsconfig()->EmitEvent(  eventWarning,
                                                GENERAL_CATEGORY,
                                                pioreqHead->fWrite ?
                                                    IOMGR_HUNG_WRITE_IO_DETECTED_ID :
                                                    IOMGR_HUNG_READ_IO_DETECTED_ID,
                                                _countof(rgpwsz),
                                                rgpwsz,
                                                JET_EventLoggingLevelMin );

#if defined( USE_HAPUBLISH_API )
    pioreqHead->p_osf->Pfsconfig()->EmitEvent(  OSDiskIHaTagOfFailureItemEnum( failureItem ),
                                                Ese2HaId( GENERAL_CATEGORY ),
                                                Ese2HaId( pioreqHead->fWrite ?
                                                    IOMGR_HUNG_WRITE_IO_DETECTED_ID :
                                                    IOMGR_HUNG_READ_IO_DETECTED_ID ),
                                                _countof(rgpwsz),
                                                rgpwsz,
                                                pioreqHead->fWrite ? HaDbIoErrorWrite : HaDbIoErrorRead,
                                                wszAbsPath,
                                                iorun.IbOffset(),
                                                iorun.CbRun() );
#endif

    OSDiagTrackHungIO( pioreqHead->fWrite, iorun.CbRun(), csecHangTime );
}

LOCAL TICK g_tickPatrolDogLastRun = 0;

DWORD IOMgrIOPatrolDogThread( DWORD_PTR dwContext )
{
    Assert( &g_patrolDogSync == (PatrolDogSynchronizer*)dwContext );

    //  IO time is very inspecific, so we must have a IO time that is much less
    //  than the Hung IO Threshhold ...

    TICK dtickNextScheduledPatrol = g_dtickIOTime;

    while ( g_patrolDogSync.FCheckForLoiter( dtickNextScheduledPatrol ) )
    {
        // we can still drift, if there is delay coming out of the FCheckForLoiter() above to here ... solution would
        // be to base it on a single point in time at sys init ... not important enough ...
        const TICK tickStartWatch = TickOSTimeCurrent();
        g_tickPatrolDogLastRun = tickStartWatch;

        //  Hung IO Context Info
        //

        HUNGIOCTX HungIOCtx = { 0 };

        Assert( HungIOCtx.cioActive == 0 );
        Assert( HungIOCtx.ciotimeLongestOutstanding == 0 );
        Assert( HungIOCtx.cmsecLongestOutstanding == 0 );
        // we happen to know the first element should be OffsetOfHIIC(), check that as
        // to be sure the CInvasiveList .ctor is getting called on our struct.
        Expected( HungIOCtx.ilHungIOs.FEmpty() );

        //  Process all IOREQs in the pool
        //

        CallS( ErrProcessIOREQs( ErrIOMgrPatrolDogICheckOutIOREQ, &HungIOCtx ) );

        //  Now process just the hung IOs, collecting stats, and taking actions as appropriate ...
        //

        COSDisk *   posdFirst = NULL;
        BOOL        fDoubleDiskWhammy = fFalse;
        IOREQ *     pioreqDoubleDiskCanary = NULL;
        DWORD       cioHung = 0;

        for ( IOREQ * pioreq = HungIOCtx.ilHungIOs.PrevMost(); pioreq; pioreq = HungIOCtx.ilHungIOs.PrevMost() )
        {
            //  Process the hung IO entry ...
            //
            cioHung++;

            //  Now, reacquire the state lock and check that the IO is still hung
            //
            pioreq->m_crit.Enter();

            TICK dtickLowestHungIoThreshold;
            if (    pioreq->Ciotime() && // Note: the ->Ciotime() != 0 ref defends against the pioreq->p_osf-> derefs next ...
                    ( CmsecLowWaitFromIOTime( pioreq->Ciotime() ) > ( dtickLowestHungIoThreshold = pioreq->p_osf->Pfsconfig()->DtickHungIOThreshhold() )
                      //  If we are fault injecting hung IOs into the watchdog pipeline, we'll pretend the IO
                      //  is hung - even though it is not, to pump our event processing.
                      OnDebug( || ChitsFaultInj( 36002 ) )
                    )
               )
            {
                //  Yes it is still active and still hung ...
                //
                Expected( pioreq->FInIssuedState() );

                const DWORD grbitHungIOActions = pioreq->p_osf->Pfsconfig()->GrbitHungIOActions();

                Assert( ( dtickLowestHungIoThreshold == ( g_dtickIOTime * g_ciotimeMax + 1 ) ) ||  // i.e. disabled
                        ( g_dtickIOTime < ( dtickLowestHungIoThreshold / 4 ) ) );

                //  For each action we check if global configuration for the IO action 
                //  is set and to ensure we haven't performed that action on this IO
                //  already ...

                if ( ( grbitHungIOActions & JET_bitHungIOEvent ) &&
                     ( 0 == ( pioreq->m_grbitHungActionsTaken & JET_bitHungIOEvent ) ) &&
                     ( CmsecLowWaitFromIOTime( pioreq->m_ciotime ) > dtickLowestHungIoThreshold 
                          OnDebug( || ChitsFaultInj( 36002 ) )
                     )
                   )
                {
                    OSTrace(    JET_tracetagIOProblems,
                                OSFormat(   "Hung IO Patrol Dog found this %p IOREQ is lurking too long.  m_ioreqtype = %d, m_iomethod = %d, m_ciotime = %u, cmsecLow = %u",
                                            pioreq, pioreq->m_ioreqtype, pioreq->m_iomethod, pioreq->m_ciotime,
                                            CmsecLowWaitFromIOTime( pioreq->m_ciotime ) ) );

                    OSDiskIReportHungIO( pioreq, CmsecLowWaitFromIOTime( pioreq->m_ciotime ) / 1000, eOSDiskIReportHungFailureItemLowThreshold );

                    pioreq->m_grbitHungActionsTaken = ( pioreq->m_grbitHungActionsTaken | JET_bitHungIOEvent );
                }

                if ( ( grbitHungIOActions & JET_bitHungIOEnforce ) &&
                     ( 0 == ( pioreq->m_grbitHungActionsTaken & JET_bitHungIOEnforce ) ) &&
                     ( CmsecLowWaitFromIOTime( pioreq->m_ciotime ) > ( 9 * dtickLowestHungIoThreshold ) ) )
                {
                    OSDiskIReportHungIO( pioreq, CmsecLowWaitFromIOTime( pioreq->m_ciotime ) / 1000, eOSDiskIReportHungFailureItemMediumThreshold );

                    //  This IO has been outstanding for _too long_, it seems hung.
                    EnforceSz( fFalse, "HungIoEnforceAction" );

                    pioreq->m_grbitHungActionsTaken = ( pioreq->m_grbitHungActionsTaken | JET_bitHungIOEnforce );
                }

                if ( ( grbitHungIOActions & JET_bitHungIOTimeout ) &&
                     ( 0 == ( pioreq->m_grbitHungActionsTaken & JET_bitHungIOTimeout ) ) &&
                     ( CmsecLowWaitFromIOTime( pioreq->m_ciotime ) > ( 12 * dtickLowestHungIoThreshold ) ) )
                {
                    OSDiskIReportHungIO( pioreq, CmsecLowWaitFromIOTime( pioreq->m_ciotime ) / 1000, eOSDiskIReportHungFailureItemExceededThreshold );

                    AssertSz( fFalse, "This IO has been outstanding for at least %u msec, it seems hung?", CmsecLowWaitFromIOTime( pioreq->m_ciotime ) );

                    pioreq->m_grbitHungActionsTaken = ( pioreq->m_grbitHungActionsTaken | JET_bitHungIOTimeout );
                }

                if ( CmsecLowWaitFromIOTime( pioreq->m_ciotime ) > ( 12 * dtickLowestHungIoThreshold ) )
                {
                    if ( posdFirst == NULL )
                    {
                        //  It's a first IO with a set COSDisk *, store what disk is having trouble ...
                        posdFirst = pioreq->m_posdCurrentIO;
                    }
                    else if ( posdFirst == pioreq->m_posdCurrentIO )
                    {
                        //  We've already hit this disk, not big deal, move along, nothing to see here ...
                    }
                    else
                    {
                        //  Whoa!  Two disks are having troubles concurrently ...

                        fDoubleDiskWhammy = fTrue;
                        pioreqDoubleDiskCanary = pioreq;
                    }

                }

            }

            //  Clear the Hung IO from the list
            //

            HungIOCtx.ilHungIOs.Remove( pioreq );

            pioreq->m_crit.Leave();

        }

        if ( fDoubleDiskWhammy )
        {
            OSTrace( JET_tracetagIOProblems, OSFormat( "IO Patrol Dog has detected a DOUBLE DISK WHAMMY!!!" ) );

            pioreqDoubleDiskCanary->m_crit.Enter();

            TICK dtickLowestHungIOThreshold = pioreqDoubleDiskCanary->p_osf->Pfsconfig()->DtickHungIOThreshhold();

            if ( CmsecLowWaitFromIOTime( pioreqDoubleDiskCanary->m_ciotime ) >= ( 4 * dtickLowestHungIOThreshold ) )
            {
#if defined( USE_HAPUBLISH_API )                
                pioreqDoubleDiskCanary->p_osf->Pfsconfig()->EmitFailureTag( HaDbFailureTagHungIoExceededThresholdDoubleDisk,
                                                                            L"9fd28644-eb87-4680-be17-8d9563f2afe8", NULL );
#endif
            }

            pioreqDoubleDiskCanary->m_crit.Leave();
        }

        OSTrace(    JET_tracetagIOQueue,
                    OSFormat(   "IO Patrol Dog: cioActive = %d, cioHung = %d, LongestWallTime = %u msec, LongestCIoTime = %u ciotime (=%u msec)",
                                HungIOCtx.cioActive,
                                cioHung,
                                (DWORD)HungIOCtx.cmsecLongestOutstanding,
                                (DWORD)HungIOCtx.ciotimeLongestOutstanding,
                                g_dtickIOTime ) );

        //  Figure out the next scheduled patrol time
        //

        const TICK dtickCurrentPatrol = (TICK)DtickDelta( tickStartWatch, TickOSTimeCurrent() );
        dtickNextScheduledPatrol = g_dtickIOTime - UlFunctionalMin( dtickCurrentPatrol, g_dtickIOTime - 100 );
        C_ASSERT( g_dtickIOTime >= 100 );
    }

    return JET_errSuccess;
}


// =============================================================================================
//  OS Disk Subsystem
// =============================================================================================
//
//

//  The List of all disks attached or connected to the engine
//
CSXWLatch g_sxwlOSDisk( CLockBasicInfo( CSyncBasicInfo( "OS Disk SXWL" ), rankOSDiskSXWL, 0 ) );
CInvasiveList< COSDisk, COSDisk::OffsetOfILE >  g_ilDiskList;

// --------------------------------------------
//  OS Disk Global Init / Term
//

//  post-terminate IO/disk subsystem

void OSDiskPostterm()
{
    Assert( g_ilDiskList.FEmpty() || FUtilProcessAbort() || FNegTest( fLeakStuff ) );
}

//  pre-init IO/disk subsystem

BOOL FOSDiskPreinit()
{
    return fTrue;
}

//  fwd decl.
void AssertIoContextQuiesced( _In_ const _OSFILE * p_osf );

//  terminate IO/disk subsystem

void OSDiskTerm()
{
    //  validate things that should be empty, are empty ...

    AssertIoContextQuiesced( NULL );

    //  trace output stats

    OSTrace( JET_tracetagDiskVolumeManagement,
            OSFormat( "End Stats: g_cioreqInUse(High Water)=%d", g_cioreqInUse ) );

    //  terminate all service threads

    // note: Must be terminated before OSDiskIIOREQPoolTerm();
    OSDiskIIOPatrolDogThreadTerm();

    OSDiskIIOThreadTerm();

    //  terminate all components

    OSDiskIIOREQPoolTerm();

    //  We can only assert that the disk list is empty after all the disk subsystems above
    //  have quiesced, since they may add/remove references to disks. The patrol dog thread,
    //  for example, is one of these cases.

    Assert( g_ilDiskList.FEmpty() );

    SetDiskMappingMode( eOSDiskInvalidMode );

    // deallocate gapping read buffer
    if ( g_rgbGapping )
    {
        OSMemoryPageFree( g_rgbGapping );
        g_rgbGapping = NULL;
    }
}

//  init IO/disk subsystem

ERR ErrOSDiskInit()
{
    ERR     err     = JET_errSuccess;

    //  init disk mapping mode and latency threshold

    const INT   cchBuf          = 256;
    WCHAR       wszBuf[ cchBuf ];

    LONG lValue = (LONG)UlConfigOverrideInjection( 57391, g_diskModeDefault );
    if ( FOSConfigGet( L"OS/IO", L"Disk Mappping Mode", wszBuf, sizeof( wszBuf ) )
        && wszBuf[0] )
    {
        lValue = _wtol( wszBuf );
        if ( lValue != eOSDiskLastDiskOnEarthMode &&
            lValue != eOSDiskOneDiskPerVolumeMode &&
            lValue != eOSDiskOneDiskPerPhysicalDisk &&
            lValue != eOSDiskMonogamousMode )
        {
            //  Person doesn't know, default back to default mode.
            lValue = (LONG)g_diskModeDefault;
        }
    }
    
    // Note this only works b/c the m_ile, is first in the COSDisk, and thus the offset = 0.
    Assert( g_ilDiskList.FEmpty() );
    SetDiskMappingMode( (OSDiskMappingMode)lValue );

    //  allocate the gapping buffer for coalesced read
    Assert( NULL == g_rgbGapping );
    if ( NULL == ( g_rgbGapping = ( BYTE* )PvOSMemoryPageAlloc( OSMemoryPageCommitGranularity(), NULL ) ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    //  init all components

    Call( ErrOSDiskIIOREQPoolInit() );

    //  start all service threads

    Call( ErrOSDiskIIOThreadInit() );

    Call( ErrOSDiskIIOPatrolDogThreadInit() );

    return JET_errSuccess;

HandleError:
    OSDiskTerm();
    return err;
}

// --------------------------------------------
//  COSDisk / OS Disk
//

//  ctor/dtor

COSDisk::COSDisk() :
    CZeroInit( sizeof(*this) ),
    m_eState( eOSDiskInitCtor ),
    m_hrtLastFfb( HrtHRTCount() ),
    m_critIOQueue( CLockBasicInfo( CSyncBasicInfo( "IO Queue/Heap" ), rankOSDiskIOQueueCrit, 0 ) ),
    m_hDisk( INVALID_HANDLE_VALUE ),
    m_tickPerformanceLastMeasured( TickOSTimeCurrent() - s_dtickPerformancePeriod ),
    m_traceidcheckDisk()
{
    Assert( g_sxwlOSDisk.FOwnWriteLatch() );
    Assert( 0 == CRef() );
    m_wszDiskPathId[0] = L'\0';
    memset( &m_osdi, 0, sizeof(m_osdi) );
    ASSERT_VALID( this );
}

COSDisk::~COSDisk()
{
    Assert( 0 == CRef() );
    Assert( 0 == m_cioDispatching );
    Assert( 0 == m_cioAsyncReadDispatching );
    Assert( 0 == m_cioForeground );
    if ( m_pIOQueue )
    {
#ifdef DEBUG
        if ( m_pIOQueue->m_pIOHeapA && m_pIOQueue->m_pIOHeapB )
        {
            Assert( 0 == CioOutstanding() );
        }
#endif
        delete m_pIOQueue;
        m_pIOQueue = NULL;
    }

    if ( m_hDisk != INVALID_HANDLE_VALUE )
    {
        CloseHandle( m_hDisk );
        m_hDisk = INVALID_HANDLE_VALUE;
    }
}

#ifdef DEBUG
void COSDisk::AssertValid() const
{
    Assert( NULL != this );

    switch( m_eState )
    {

        case eOSDiskInitCtor:
            Assert( 0 == CRef() );
            Assert( m_wszDiskPathId[0] == L'\0' );
            Assert( !g_ilDiskList.FMember( this ) );
            break;

        case eOSDiskConnected:
            Assert( CRef() );
            Assert( m_wszDiskPathId[0] != L'\0' );
            Assert( g_ilDiskList.FMember( this ) );
            break;

        default:
            AssertSz( fFalse, "Unknown OSDisk State!" );
            break;
    }
}
#endif

BOOL COSDisk::FIsDisk( __in_z const WCHAR * const wszTargetDisk ) const
{
    return ( 0 == LOSStrCompareW( m_wszDiskPathId, wszTargetDisk ) );
}

void COSDisk::TraceStationId( const TraceStationIdentificationReason tsidr )
{
    //  Called (with tsidrPulseInfo) for every IOs.

    if ( !m_traceidcheckDisk.FAnnounceTime< _etguidDiskStationId >( tsidr ) )
    {
        return;
    }

    Expected( tsidr == tsidrOpenInit ||
                tsidr == tsidrPulseInfo );

    const DWORD dwDiskNumber = DwDiskNumber();

    //  Typically the Smart version of model is longer and more specific.
    const char * szDiskModelBest = ( strstr( m_osdi.m_szDiskModelSmart, m_osdi.m_szDiskModel ) != NULL ) ? m_osdi.m_szDiskModelSmart : m_osdi.m_szDiskModel;

    ETDiskStationId( tsidr, dwDiskNumber, WszDiskPathId(), szDiskModelBest, m_osdi.m_szDiskFirmwareRev, m_osdi.m_szDiskSerialNumber );

    //  Because these OS structures are the same on 64-bit and 32-bit it's just easier
    //  to trace them as a whole data structure.
    if ( m_osdi.m_errorOsdci == ERROR_SUCCESS )
    {
        static_assert( sizeof( m_osdi.m_osdci ) == 24, "Size of struct must stay matching trace definition decleared in .mc file on all CPU architectures." );
        ETDiskOsDiskCacheInfo( tsidr, (BYTE*)&m_osdi.m_osdci );
    }
    if ( m_osdi.m_errorOsswcp == ERROR_SUCCESS )
    {
        static_assert( sizeof( m_osdi.m_osswcp ) == 28, "Size of struct must stay matching trace definition decleared in .mc file on all CPU architectures." );
        ETDiskOsStorageWriteCacheProp( tsidr, (BYTE*)&m_osdi.m_osswcp );
    }
    if ( m_osdi.m_errorOsdspd == ERROR_SUCCESS )
    {
        static_assert( sizeof( m_osdi.m_osdspd ) == 12, "Size of struct must stay matching trace definition decleared in .mc file on all CPU architectures." );
        ETDiskOsDeviceSeekPenaltyDesc( tsidr, (BYTE*)&m_osdi.m_osdspd );
    }
}


//  The disk list (g_ilDiskList) and COSDisk ref counting and lifecycle ...
//

//  There are 4 basic operations:
//
//      Find/Ref: ErrOSDiskIFind / posd->AddRef:
//
//          Must have s-latch.  If the disk is already created, adding files
//          to the disk context will require little locking.
//
//      Create: ErrOSDiskICreate()
//
//          Must have x-latch, to exclude anyone from deleting from the disk 
//          list.  Today upgraded to w-latch during create, may not be needed.
//
//      Release/Delete: posd->Release() / OSDiskIDelete()
//
//          Must have x-latch, may be escalated to w-latch to perform garbage
//          collection on the COSDisk object if ref count falls to zero.
//
//      Enumerate: COSDiskEnumerator->PosdNext()
//
//          Uses an x-latch to use AddRef() / Release(), to walk itself along 
//          the list of disks.
//

//  The lifecycle is this
//
//      ErrOSDiskICreate() is called to create the posd.
//      Some number of AddRef()s are called on it.
//      Some number of Release()s are called on it.
//      On the Release() that reduces the ref count to zero, the OS Disk GC's itself.
//

void COSDisk::AddRef()
{
    Assert( g_sxwlOSDisk.FOwnSharedLatch() || g_sxwlOSDisk.FOwnExclusiveLatch() || g_sxwlOSDisk.FOwnWriteLatch() );

    AtomicIncrement( (LONG*)&m_cref );
}

void OSDiskIDelete( __inout COSDisk * posd );

void COSDisk::Release( COSDisk * posd )
{
    Assert( g_sxwlOSDisk.FOwnExclusiveLatch() );

    AtomicDecrement( (LONG*)&posd->m_cref );

    Assert( posd->m_cref < 0x7FFFFFFF );    // check for underflow

    if ( 0 == posd->m_cref )
    {
        CSXWLatch::ERR errSXWL = g_sxwlOSDisk.ErrUpgradeExclusiveLatchToWriteLatch();
        if ( CSXWLatch::ERR::errWaitForWriteLatch == errSXWL )
        {
            g_sxwlOSDisk.WaitForWriteLatch();
            errSXWL = CSXWLatch::ERR::errSuccess;
        }
        Assert( errSXWL == CSXWLatch::ERR::errSuccess );

        if ( 0 == posd->m_cref )
        {
            //  We trace before, because the object will be invalid after we delete. ;-)
            OSTrace( JET_tracetagDiskVolumeManagement, OSFormat( "Deleted Disk p=%p, PathId=%ws", posd, posd->WszDiskPathId() ) );

            Assert( 0 == posd->CioOutstanding() );
            OSDiskIDelete( posd );
            // MUST NOT deref member variables after here.
            posd = NULL;
        }
        else
        {
            OSTrace( JET_tracetagDiskVolumeManagement, OSFormat( "Del Ref Disk p=%p, PathId=%ws, Cref=%d", posd, posd->WszDiskPathId(), posd->CRef() ) );
        }

        g_sxwlOSDisk.DowngradeWriteLatchToExclusiveLatch();
    }
    else
    {
        OSTrace( JET_tracetagDiskVolumeManagement, OSFormat( "Del Ref Disk p=%p, PathId=%ws, Cref=%d", posd, posd->WszDiskPathId(), posd->CRef() ) );
    }

    // NOTE: posd may be NULL here, if m_cref == 0.

    Assert( g_sxwlOSDisk.FOwnExclusiveLatch() );
}

//  returns the number of referrers

ULONG COSDisk::CRef() const
{
    return m_cref;
}

const WCHAR * COSDisk::WszDiskPathId() const
{
    return m_wszDiskPathId;
}

ERR COSDisk::ErrDiskId( ULONG_PTR* const pulDiskId ) const
{
    ERR err = JET_errSuccess;
    
    Assert( pulDiskId != NULL );

    *pulDiskId = (ULONG_PTR)this;

    return err;
}

BOOL COSDisk::FSeekPenalty() const
{
    // SeekPenalty is used to determine whether OLD should be
    // enabled on the current database. If FSeekPenalty is true
    // (i.e. this is on an HDD) then OLD is beneficial, but if
    // it is not (i.e. on an SSD) then OLD is not beneficial.
    // For testing, though, we need to deterministically run
    // or not run OLD no matter what hardware we are on, so 
    // the following FaultInjection will be used to make the 
    // current hardware behave as an SSD (spNo) or an HDD (spYes).
    // If unset (spAuto) it will make the determination based
    // on the actual hardware.
    // 0 = spAuto
    // 1 = spNo
    // 2 = spYes
    const INT spMode = ErrFaultInjection( 17353 );

    if ( spMode == 1 /* SSD Mode */ )
    {
        return fFalse;
    }
    else if ( spMode == 2 /* HDD Mode */ )
    {
        return fTrue;
    }

    Assert( spMode == 0 /* Auto Mode */ );
    return m_osdi.m_errorOsdspd != ERROR_SUCCESS ||
           m_osdi.m_osdspd.Size < 12 /* offset + size of the .IncursSeekPenalty member*/ ||
           m_osdi.m_osdspd.IncursSeekPenalty;
}

ERR ErrValidateNotUsingIoContext( _In_ ULONG ichunk, _In_ ULONG iioreq, _In_ const IOREQ * pioreq, void * pvCtx )
{
    _In_ const _OSFILE * p_osf = (_OSFILE *)pvCtx;

    if ( p_osf )
    {
        // I think I ensured that the p_osf is properly reset in completion / free ...
        Assert( p_osf != pioreq->p_osf );
    }
    else
    {
        //  When called with NULL we interprit to mean, ensure there are no outstanding IOs
        Assert( NULL == pioreq->p_osf );
    }

    return JET_errSuccess;
}

//  If p_osf is NULL we validate there is no active IO in the system
//  If p_osf is non-NULL we validate there is no active IO in the system matching this IO Context / p_osf.

void AssertIoContextQuiesced( _In_ const _OSFILE * p_osf )
{
    (void)ErrEnumerateIOREQs( ErrValidateNotUsingIoContext, (void*)p_osf );
}

//  Consolidated the calculation for urgent outstanding max, to create a single point of truth

ULONG CioDefaultUrgentOutstandingIOMax( _In_ const ULONG cioOutstandingMax )
{
    return cioOutstandingMax / 2;
}

ULONG CioBackgroundIOLow( _In_ const ULONG cioBackgroundMax )
{
    return max( 1, cioBackgroundMax / 20 );   // 5% ...
}

char * SzSkipPreceding( char * szT, char chSkip )
{
    //  Trim up past last preceding char
    char * szStart = szT;
    for( INT ich = 0; szT[ich] != L'\0'; ich++ )
    {
        if ( szT[ich] != chSkip )
        {
            break;
        }
        szStart = &( szT[ ich + 1 ] );
    }
    // note if all characters are matching chSkip return " " (single blank space) as return value.

    return szStart;
}

//  The S.M.A.R.T. driver string is not very smart to say the least, I (SOMEONE) would actually say downright
//  bizarre ... it is ASCII, but it is packed into 2-byte WORDs that are big-endian, so all the characters must
//  be flipped to put it in order, and then in addition has either trailing or leading white space 0x20 chars
//  that you'll want to pull out ... 
VOID ConvertSmartDriverString( BYTE * sSmartDriverString, INT cbSmartDriverString, PSTR szProperString, INT cbProperString )
{
    Assert( cbProperString < 50 );
    char * szT = (char*)alloca( cbProperString );

    Assert( cbSmartDriverString < cbProperString );  // proper string will need space for added NUL, so should be at least 1 byte larger

    //  For every WORD / 2-byte of chars, swap them ...
    for( INT ich = 0; ich < cbSmartDriverString; ich += 2 /* this is NOT sizeof(WCHAR), see func desc */ )
    {
        szT[ich] = sSmartDriverString[ich+1];
        szT[ich+1] = sSmartDriverString[ich];
    }

    //  Even though we're processing the converted string, we use driver string size because we only want to
    //  evaluate the driver provided data (while proper string may be longer than nesc and have garbage out
    //  past the NUL we're adding here).
    szT[ cbSmartDriverString ] = '\0';

    //  because the conversion is so weird, just make sure that we aren't going to lose any chars when we
    //  copy it to the new string.
    BOOL fSawNullChar = fFalse;
    for( INT ich = 0; ich < cbSmartDriverString; ich++ )
    {
        if ( szT[ich] == '\0' )
        {
            fSawNullChar = fTrue;
        }
        if ( fSawNullChar && szT[ich] != '\0' )
        {
            FireWall( "SmartDataStrConversionSawTruncatedChars" );
            break;
        }
    }

    //  Trim off trailing space
    for( INT ich = cbSmartDriverString - 1; ich >= 0; ich-- )
    {
        if ( szT[ich] != ' ' )
        {
            break;
        }
        szT[ich] = '\0';
    }

    //  Trim up past last preceding space
    char * szStart = szT;
    for( INT ich = 0; ich < ( cbSmartDriverString - 1 ); ich++ )
    {
        if ( szT[ich] != ' ' )
        {
            break;
        }
        szStart = &( szT[ ich + 1 ] );
    }
    // note if all space characters will return " " (single blank space) as return value.

    OSStrCbCopyA( szProperString, cbProperString, szStart );
}

void COSDisk::SetSmartEseNoLoadFailed( _In_ const ULONG iStep, _In_ const DWORD error, __out_bcount_z(cbIdentifier) CHAR * szIdentifier, _In_ const ULONG cbIdentifier )
{
    OSStrCbFormatA( szIdentifier, cbIdentifier, szEseNoLoadSmartData, iStep, error );

    CHAR szNoLoadFireWallMsg[ 50 ];
    Assert( strlen( szIdentifier ) + 15 < _countof(szNoLoadFireWallMsg) );
    OSStrCbFormatA( szNoLoadFireWallMsg, sizeof(szNoLoadFireWallMsg), "%hs%hs", "SmartData", szIdentifier );

    OSTrace( JET_tracetagFile, OSFormat( "Failed S.M.A.R.T. load: %hs \n", szNoLoadFireWallMsg ) );
    
}

//  Load optional and extra disk identity and performance configuration.
//  This function may fail, but will always initialize m_wszModelNumber/SerialNumber/FirmwareRev
void COSDisk::LoadDiskInfo_( __in_z PCWSTR wszDiskPath, _In_ const DWORD dwDiskNumber )
{
    BOOL fSuccess;
    DWORD cbRet;
    HRT hrtStart;

    //  Just in case we mess up and quit without configuring, check things generally are init'd to zero.
    Assert( m_osdi.m_grbitDiskSmartCapabilities == 0 );
    Assert( m_osdi.m_szDiskModel[0] == '\0' );
    Assert( m_osdi.m_szDiskSerialNumber[0] == '\0' );
    Assert( m_osdi.m_szDiskFirmwareRev[0] == '\0' );

    //  Given we're filling out the m_wszModel/Serial/Firmware strings on the COSDisk it would
    //  be odd if these don't match.
    Expected( m_dwDiskNumber == dwDiskNumber );

    //
    //      First try to open the PhysicalDiskX to perform IOCTL_s on.
    //

    //  While we do have a m_hDisk handle it is not opened with GENERIC_READ | GENERIC_WRITE, nor FILE_SHARE_WRITE, nor FILE_ATTRIBUTE_SYSTEM
    BOOL fDiskRw = fTrue;
    HANDLE hDisk = NULL;
#ifndef ESENT
    hDisk = CreateFileW(    wszDiskPath,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_SYSTEM,
                            NULL );
#endif
    if ( hDisk == INVALID_HANDLE_VALUE || hDisk == NULL )
    {
        DWORD error = GetLastError();
        //  The S.M.A.R.T. operation, even for SMART_RCV_DRIVE_DATA / retrieve data requires a RW disk
        //  handle, so set error ID in the full model number that SMART is supposed to fill out.
        SetSmartEseNoLoadFailed( eSmartLoadDiskOpenFailed, error, m_osdi.m_szDiskModelSmart, sizeof(m_osdi.m_szDiskModelSmart) );
        fDiskRw = fFalse;
        hDisk = CreateFileW( wszDiskPath,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_SYSTEM,
                                    NULL );
        if ( hDisk == INVALID_HANDLE_VALUE || hDisk == NULL )
        {
            error = GetLastError();
            //  We can't even open the disk RO, so we won't be able to load anything.
            SetSmartEseNoLoadFailed( eSmartLoadDiskOpenFailed, error, m_osdi.m_szDiskModel, sizeof(m_osdi.m_szDiskModel) );
            SetSmartEseNoLoadFailed( eSmartLoadDiskOpenFailed, error, m_osdi.m_szDiskSerialNumber, sizeof(m_osdi.m_szDiskSerialNumber) );
            OSStrCbCopyA( m_osdi.m_szDiskFirmwareRev, sizeof(m_osdi.m_szDiskFirmwareRev), "EseNoLo" );

            return;
        }
    }

    //
    //      Second retrieve non-SMART identity data.
    //

    STORAGE_PROPERTY_QUERY osspqSdp = { StorageDeviceProperty, PropertyStandardQuery, 0 };
    //  If 1024 is ever not enough, we can use STORAGE_DESCRIPTOR_HEADER first to query size of data and then allocate something
    //  of appropriate size.
    BYTE rgbDevDesc[1024] = { 0 };
    STORAGE_DEVICE_DESCRIPTOR* possdd = (STORAGE_DEVICE_DESCRIPTOR *)rgbDevDesc;
    C_ASSERT( sizeof(*possdd) <= sizeof(rgbDevDesc) );

    hrtStart = HrtHRTCount();
    fSuccess = DeviceIoControl( hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &osspqSdp, sizeof(osspqSdp), possdd, sizeof(rgbDevDesc), &cbRet, NULL );
    if ( !fSuccess || cbRet < sizeof(*possdd) )
    {
        const DWORD error = GetLastError();
        OSTrace( JET_tracetagFile, OSFormat( "FAILED: DevIoCtrl( query prop \\ StorageDeviceProperty ) -> %d / %d / %d in %I64u us \n", fSuccess, cbRet, error, CusecHRTFromDhrt( HrtHRTCount() - hrtStart ) ) );

        //  This is the most basic ID data we can't read.
        SetSmartEseNoLoadFailed( eSmartLoadDiskOpenFailed, error, m_osdi.m_szDiskModel, sizeof(m_osdi.m_szDiskModel) );
        SetSmartEseNoLoadFailed( eSmartLoadDiskOpenFailed, error, m_osdi.m_szDiskSerialNumber, sizeof(m_osdi.m_szDiskSerialNumber) );
        OSStrCbCopyA( m_osdi.m_szDiskFirmwareRev, sizeof(m_osdi.m_szDiskFirmwareRev), "EseNoLo" );

        // we will continue on, seeing if we can load other things, but I doubt anything else will work.
    }
    else
    {
        OSTrace( JET_tracetagFile, OSFormat( "\t possdd = { Ver.Size=%d.%d, ib[] = %d,%d,%d,%d, DevType.Mod = %d.%d, BusType = %d, CommandQueueing = %d };\n",
                    possdd->Version, possdd->Size, possdd->VendorIdOffset, possdd->ProductIdOffset, possdd->ProductRevisionOffset,
                    possdd->SerialNumberOffset, possdd->DeviceType, possdd->DeviceTypeModifier, possdd->BusType, possdd->CommandQueueing ) );

        if ( possdd->ProductIdOffset != 0 )
        {
            if ( ErrOSStrCbCopyA( m_osdi.m_szDiskModel, sizeof(m_osdi.m_szDiskModel), SzSkipPreceding( (char*)&rgbDevDesc[possdd->ProductIdOffset], ' ' ) ) < JET_errSuccess )
            {
                FireWall( "OsDiskTruncateBasicDiskModel" );
            }
        }
        if ( possdd->SerialNumberOffset != 0 )
        {
            if ( ErrOSStrCbCopyA( m_osdi.m_szDiskSerialNumber, sizeof(m_osdi.m_szDiskSerialNumber), SzSkipPreceding( (char*)&rgbDevDesc[possdd->SerialNumberOffset], ' ' ) ) < JET_errSuccess )
            {
                FireWall( "OsDiskTruncateBasicDiskModel" );
            }
        }
        if ( possdd->ProductRevisionOffset != 0 )
        {
            if ( ErrOSStrCbCopyA( m_osdi.m_szDiskFirmwareRev, sizeof(m_osdi.m_szDiskFirmwareRev), SzSkipPreceding( (char*)&rgbDevDesc[possdd->ProductRevisionOffset], ' ' ) ) < JET_errSuccess )
            {
                FireWall( "OsDiskTruncateDiskFirmwareRev" );
            }
        }
        OSTrace( JET_tracetagFile, OSFormat( "\t\t ->Vendor       = %hs;\n", possdd->VendorIdOffset ? ( (char*) &rgbDevDesc[possdd->VendorIdOffset] ) : "<null>" ) ); // this one largely useless.
        OSTrace( JET_tracetagFile, OSFormat( "\t\t ->Product      = %hs;\n", m_osdi.m_szDiskModel ) );
        OSTrace( JET_tracetagFile, OSFormat( "\t\t ->SerialNum    = %hs;\n", m_osdi.m_szDiskSerialNumber ) );
        OSTrace( JET_tracetagFile, OSFormat( "\t\t ->ProductRev   = %hs;\n", m_osdi.m_szDiskFirmwareRev ) );
    }

    //
    //      Third retrieve S.M.A.R.T. identity data (and compare)
    //

    BOOL fSmartCmds = fFalse;

    cbRet = 0x42;
    GETVERSIONINPARAMS osgvip = { 0 };
    hrtStart = HrtHRTCount();
    fSuccess = DeviceIoControl( hDisk, SMART_GET_VERSION, NULL, 0, &osgvip, sizeof(osgvip), &cbRet, NULL);
    if ( !fSuccess || cbRet < sizeof(osgvip) )
    {
        // Some notes:
        //  Our Windows Git Enlistment PCI SSD devices returns: ERROR_IO_DEVICE / 1117L
        //  And a run of the mill SSD thumb drive returns: ERROR_NOT_SUPPORTED /  50L

        const DWORD error = GetLastError();
        OSTrace( JET_tracetagFile, OSFormat( "FAILED: DevIoCtrl( SMART_GET_VERSION ) -> %d / %d / %d in %I64u us \n", fSuccess, cbRet, error, CusecHRTFromDhrt( HrtHRTCount() - hrtStart ) ) );

        if ( m_osdi.m_szDiskModelSmart[0] == '\0' )
        {
            SetSmartEseNoLoadFailed( eSmartLoadDevIoCtrlGetVerFailed, GetLastError(), m_osdi.m_szDiskModelSmart, sizeof(m_osdi.m_szDiskModelSmart) );
        }
    }
    else
    {
        m_osdi.m_bDiskSmartVersion = osgvip.bVersion;
        m_osdi.m_bDiskSmartRevision = osgvip.bRevision;
        m_osdi.m_bDiskSmartIdeDevMap = osgvip.bIDEDeviceMap;
        m_osdi.m_grbitDiskSmartCapabilities = osgvip.fCapabilities;

        OSTrace( JET_tracetagFile, OSFormat( "\t DevIoCtrl( SMART_GET_VERSION ) --> %d / %d\n", cbRet, GetLastError() ) );
        OSTrace( JET_tracetagFile, OSFormat( "\t osgvip = { Ver.Rev = %d.%d, bIDEDeviceMap = %#x, fCapabilities = %#x }\n",
                    osgvip.bVersion, osgvip.bReserved, osgvip.bIDEDeviceMap, osgvip.fCapabilities ) );

        // note: CAP_SMART_CMD = 0x4
        fSmartCmds = ( m_osdi.m_bDiskSmartVersion == 1 &&    //  These first two checks may be overly defensive
                        m_osdi.m_bDiskSmartRevision == 1 &&  //  Is a revision of 0 acceptable? Probably?
                        m_osdi.m_grbitDiskSmartCapabilities & CAP_SMART_CMD );  // last definitely required
        if ( !fSmartCmds )
        {
            //  Break apart the reason 
            if ( m_osdi.m_bDiskSmartVersion != 1 )
            {
                if ( m_osdi.m_szDiskModelSmart[0] == '\0' )
                {
                    SetSmartEseNoLoadFailed( eSmartLoadSmartVersionUnexpected, 0, m_osdi.m_szDiskModelSmart, sizeof(m_osdi.m_szDiskModelSmart) );
                }
            }
            else if ( m_osdi.m_bDiskSmartRevision != 1 )
            {
                if ( m_osdi.m_szDiskModelSmart[0] == '\0' )
                {
                    SetSmartEseNoLoadFailed( eSmartLoadSmartRevisionUnexpected, 0, m_osdi.m_szDiskModelSmart, sizeof(m_osdi.m_szDiskModelSmart) );
                }
            }
            else
            {
                if ( m_osdi.m_szDiskModelSmart[0] == '\0' )
                {
                    SetSmartEseNoLoadFailed( eSmartLoadSmartCmdCapabilityNotSet, 0, m_osdi.m_szDiskModelSmart, sizeof(m_osdi.m_szDiskModelSmart) );
                }
            }
        }
    }

    if ( fSmartCmds )
    {
        typedef struct
        {
            WORD    wGenConfig;
            WORD    wNumCyls;
            WORD    wReserved;
            WORD    wNumHeads;
            WORD    wBytesPerTrack;
            WORD    wBytesPerSector;
            WORD    wSectorsPerTrack;
            WORD    wVendorUnique[3];
            BYTE    sSerialNumber[20];
            WORD    wBufferType;
            WORD    wBufferSize;
            WORD    wECCSize;
            BYTE    sFirmwareRev[8];
            BYTE    sModelNumber[39];
            WORD    wMoreVendorUnique;
            WORD    wDoubleWordIO;
            WORD    wCapabilities;
            WORD    wReserved1;
            WORD    wPIOTiming;
            WORD    wDMATiming;
            WORD    wBS;
            WORD    wNumCurrentCyls;
            WORD    wNumCurrentHeads;
            WORD    wNumCurrentSectorsPerTrack;
            WORD    ulCurrentSectorCapacity;
            WORD    wMultSectorStuff;
            DWORD   dwTotalAddressableSectors;
            WORD    wSingleWordDMA;
            WORD    wMultiWordDMA;
            BYTE    bReserved[127];
        } SmartIdSector;

        //  originally called DRIVE_HEAD_REG, but think this is just b/c of the variable it was assigned to, but it isn't
        //  head register, it is just how the command is sent through per the SMART .pdf I found.
        #define eSmartPacketCmd  (0xA0)

        SENDCMDINPARAMS osscip = {0};

        //  The SENDCMDINPARAMS has a single byte for the beginning of the buffer.
        C_ASSERT( 16 == sizeof( SENDCMDOUTPARAMS ) - 1 );

        C_ASSERT( sizeof(SmartIdSector) <= IDENTIFY_BUFFER_SIZE );
        const ULONG cbOutput = ( IDENTIFY_BUFFER_SIZE + sizeof( SENDCMDOUTPARAMS ) - 1 );
        char rgbOutput[ cbOutput ] = { 0 };
        SENDCMDOUTPARAMS * pCOP = (SENDCMDOUTPARAMS *) rgbOutput;

        osscip.cBufferSize = IDENTIFY_BUFFER_SIZE;
        osscip.bDriveNumber = (BYTE)dwDiskNumber;
        osscip.irDriveRegs.bFeaturesReg = 0;
        osscip.irDriveRegs.bSectorCountReg = 1;
        osscip.irDriveRegs.bSectorNumberReg = 1;
        osscip.irDriveRegs.bCylLowReg = 0;
        osscip.irDriveRegs.bCylHighReg = 0;
        osscip.irDriveRegs.bDriveHeadReg = eSmartPacketCmd;
        osscip.irDriveRegs.bCommandReg = ID_CMD;
        
        cbRet = 0x42;
        hrtStart = HrtHRTCount();
        fSuccess = DeviceIoControl( hDisk, SMART_RCV_DRIVE_DATA, &osscip, sizeof(osscip), pCOP, cbOutput, &cbRet, NULL );
        if ( !fSuccess )
        {
            const DWORD error = GetLastError();
            OSTrace( JET_tracetagFile, OSFormat( "FAILED: DevIoCtrl( SMART_RCV_DRIVE_DATA ) -> %d / %d / %d in %I64u us \n", fSuccess, cbRet, error, CusecHRTFromDhrt( HrtHRTCount() - hrtStart ) ) );

            if ( m_osdi.m_szDiskModelSmart[0] == '\0' )
            {
                SetSmartEseNoLoadFailed( eSmartLoadDevIoCtrlRcvDriveDataFailed, error, m_osdi.m_szDiskModelSmart, sizeof(m_osdi.m_szDiskModelSmart) );
            }
        }
        else
        {
            //  Just checking that the bBuffer works out to right after the SENDCMDOUTPARAMS - 1 as documented online.
            Assert( ( ((BYTE*)pCOP) + sizeof( SENDCMDOUTPARAMS ) - 1 ) == pCOP->bBuffer );
            SmartIdSector * pidsec = (SmartIdSector *)pCOP->bBuffer;

            CHAR szSmartDiskSerialNumber[ sizeof(pidsec->sSerialNumber) + 1 ];
            CHAR szSmartDiskFirmwareRev[ sizeof(pidsec->sFirmwareRev) + 1 ];

            //  Strings come out in bizarre format, need conversion to standard strings.
            ConvertSmartDriverString( pidsec->sModelNumber, sizeof(pidsec->sModelNumber), m_osdi.m_szDiskModelSmart, sizeof(m_osdi.m_szDiskModelSmart) );
            ConvertSmartDriverString( pidsec->sSerialNumber, sizeof(pidsec->sSerialNumber), szSmartDiskSerialNumber, sizeof(szSmartDiskSerialNumber) );
            ConvertSmartDriverString( pidsec->sFirmwareRev, sizeof(pidsec->sFirmwareRev), szSmartDiskFirmwareRev, sizeof(szSmartDiskFirmwareRev) );

            OSTrace( JET_tracetagFile, OSFormat( "Full S.M.A.R.T. Versions: %hs - %hs - %hs\n",
                        m_osdi.m_szDiskModelSmart, szSmartDiskFirmwareRev, szSmartDiskSerialNumber ) );
        }
    }


    //
    //      Finally load info about the disk caches, buses, etc.
    //

    LoadCachePerf_( hDisk );

    CloseHandle( hDisk );
    hDisk = INVALID_HANDLE_VALUE;
}

DWORD ErrorOSDiskIOsStorageQueryProp_( HANDLE hDisk, STORAGE_PROPERTY_ID ePropId, const CHAR * const szPropId, void * pvOsStruct, const ULONG cbOsStruct )
{
    DWORD cbRet = 0x42;
    STORAGE_PROPERTY_QUERY osspq = { ePropId, PropertyStandardQuery, 0 };
    //  experience has taught me to be cautious with obscure IOCTL_s and esp. STORAGE_PROPERTY_QUERY, and that various 
    //  drivers don't always do a good job with validating args.  So pass in conservative buffer.
    BYTE rgbBuffer[512] = { 0 };

    if ( cbOsStruct > sizeof( rgbBuffer ) )
    {
        AssertSz( fFalse, "Local buffer is too small." );
        return 0xFFFFFFF1;
    }

    HRT hrtStart = HrtHRTCount();
    const BOOL fSuccess = DeviceIoControl( hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &osspq, sizeof(osspq), rgbBuffer, cbOsStruct, &cbRet, NULL );
    const DWORD error = GetLastError();

    if ( !fSuccess )
    {
        OSTrace( JET_tracetagFile, OSFormat( "FAILED: DevIoCtrl( query prop \\ %hs ) -> %d / %d / %d in %I64u us \n", szPropId, fSuccess, cbRet, error, CusecHRTFromDhrt( HrtHRTCount() - hrtStart ) ) );
        if ( error != ERROR_SUCCESS )
        {
            return error;
        }
        //  Hmmm, no success but no error either/
        return 0xFFFFFFF2;
    }
    if ( cbRet < cbOsStruct )
    {
        //  This is only used for fixed structures, so client expected at least this much data.
        return 0xFFFFFFF3;
    }

    memcpy( pvOsStruct, rgbBuffer, cbOsStruct );

    return ERROR_SUCCESS;
}

#define ErrorOSDiskIOsStorageQueryProp( hD, e, pv, cb )     ErrorOSDiskIOsStorageQueryProp_( hD, e, #e, pv, cb )

void COSDisk::LoadCachePerf_( HANDLE hDisk )
{
    Assert( hDisk != NULL && hDisk != INVALID_HANDLE_VALUE );

    DWORD cbRet = 0x42;
    HRT hrtStart;

    hrtStart = HrtHRTCount();
    BOOL fSuccess = DeviceIoControl( hDisk, IOCTL_DISK_GET_CACHE_INFORMATION, NULL, 0, &m_osdi.m_osdci, sizeof(m_osdi.m_osdci), &cbRet, NULL );
    if ( !fSuccess || cbRet < sizeof(m_osdi.m_osdci) )
    {
        m_osdi.m_errorOsdci = GetLastError();
        // Not sure if GetLastError() or CusecHRTFromDhrt() / HrtHRTCount() is first.
        OSTrace( JET_tracetagFile, OSFormat( "FAILED: DevIoCtrl( IOCTL_DISK_GET_CACHE_INFORMATION ) -> %d / %d / %d in %I64u us \n", fSuccess, cbRet, GetLastError(), CusecHRTFromDhrt( HrtHRTCount() - hrtStart ) ) );
    }
    else
    {
        OSTrace( JET_tracetagFile, OSFormat( "\t m_osdi.m_osdci = { ParamSavable=%d, Read/Write.CacheEnabled=%d/%d, Read/Write.RetentionPriority=%d/%d, S/B=%d - [ %d - %d, %d } }\n",
                    m_osdi.m_osdci.ParametersSavable, m_osdi.m_osdci.ReadCacheEnabled, m_osdi.m_osdci.WriteCacheEnabled, m_osdi.m_osdci.ReadRetentionPriority, m_osdi.m_osdci.WriteRetentionPriority,
                    m_osdi.m_osdci.PrefetchScalar, m_osdi.m_osdci.ScalarPrefetch.Minimum, m_osdi.m_osdci.ScalarPrefetch.Maximum, m_osdi.m_osdci.PrefetchScalar ? m_osdi.m_osdci.ScalarPrefetch.MaximumBlocks : 0 ) );
    }

    m_osdi.m_errorOsswcp = ERROR_INVALID_PARAMETER;
    // quick disable of retrieval of property that slows down the disk response time.
    // m_osdi.m_errorOsswcp = ErrorOSDiskIOsStorageQueryProp( hDisk, StorageDeviceWriteCacheProperty, &m_osdi.m_osswcp, sizeof(m_osdi.m_osswcp) );
    if ( m_osdi.m_errorOsswcp == ERROR_SUCCESS )
    {
        OSTrace( JET_tracetagFile, OSFormat( "\t m_osdi.m_osswcp = { Ver.Size=%d.%d, WriteCacheType=%d, WriteCacheEnabled=%d, WriteCacheChangeable=%d, WriteThrough=%d, Flush=%d, PowerProt=%d, NVCache=%d };\n",
                    m_osdi.m_osswcp.Version, m_osdi.m_osswcp.Size, m_osdi.m_osswcp.WriteCacheType, m_osdi.m_osswcp.WriteCacheEnabled, m_osdi.m_osswcp.WriteCacheChangeable,
                    m_osdi.m_osswcp.WriteThroughSupported, m_osdi.m_osswcp.FlushCacheSupported, m_osdi.m_osswcp.UserDefinedPowerProtection, m_osdi.m_osswcp.NVCacheEnabled ) );
    }

    m_osdi.m_errorOssad = ErrorOSDiskIOsStorageQueryProp( hDisk, StorageAdapterProperty, &m_osdi.m_ossad, sizeof(m_osdi.m_ossad) );
    if ( m_osdi.m_errorOssad == ERROR_SUCCESS )
    {
        OSTrace( JET_tracetagFile, OSFormat( "\t m_osdi.m_ossad = { Ver.Size=%d.%d, MaxTransferLength/PhysicalPages=%d/%d, AlignMsk=%d, Pio=%d, CommandQueueing=%d, "
                    "Accel=%d, BusType.Maj.Minor=%d.%d.%d };\n",
                    m_osdi.m_ossad.Version, m_osdi.m_ossad.Size, m_osdi.m_ossad.MaximumTransferLength, m_osdi.m_ossad.MaximumPhysicalPages, m_osdi.m_ossad.AlignmentMask,
                    m_osdi.m_ossad.AdapterUsesPio, /* skipped AdapterScansDown, */ m_osdi.m_ossad.CommandQueueing, m_osdi.m_ossad.AcceleratedTransfer,
                    m_osdi.m_ossad.BusType, m_osdi.m_ossad.BusMajorVersion, m_osdi.m_ossad.BusMinorVersion ) );
    }

    m_osdi.m_errorOsdtd = ErrorOSDiskIOsStorageQueryProp( hDisk, StorageDeviceTrimProperty, &m_osdi.m_osdtd, sizeof(m_osdi.m_osdtd) );
    if ( m_osdi.m_errorOsdtd == ERROR_SUCCESS )
    {
        OSTrace( JET_tracetagFile, OSFormat( "\t m_osdi.m_osdtd = { Ver.Size=%d.%d, TrimEnabled=%d };\n",
                    m_osdi.m_osdtd.Version, m_osdi.m_osdtd.Size, m_osdi.m_osdtd.TrimEnabled ) );
    }

    m_osdi.m_errorOsdcod = ErrorOSDiskIOsStorageQueryProp( hDisk, StorageDeviceCopyOffloadProperty, &m_osdi.m_osdcod, sizeof(m_osdi.m_osdcod) );
    if ( m_osdi.m_errorOsdcod == ERROR_SUCCESS )
    {
        OSTrace( JET_tracetagFile, OSFormat( "\t m_osdi.m_osdcod = { Ver.Size=%d.%d, Max/OptimalTransfer=%I64u/%I64u };\n",
                    m_osdi.m_osdcod.Version, m_osdi.m_osdcod.Size, m_osdi.m_osdcod.MaximumTransferSize, m_osdi.m_osdcod.OptimalTransferCount ) );
    }

    // Does not compile - need some sort of new OS header maybe?
    //m_osdi.m_errorOssmptd = ErrorOSDiskIOsStorageQueryProp( hDisk, StorageDeviceMediumProductType, &m_osdi.m_ssmptd, sizeof(m_osdi.m_ssmptd) );
}


//  Initialize the DISK.

ERR COSDisk::ErrInitDisk(   _In_    IFileSystemConfiguration* const pfsconfig,
                            _In_z_  const WCHAR * const             wszDiskPathId, 
                            _In_    const DWORD                     dwDiskNumber )
{
    ERR         err     = JET_errSuccess;
    const INT       cchBuf      = 256;
    WCHAR           wszBuf[ cchBuf ];

    m_dwDiskNumber = dwDiskNumber;
    Call( ErrOSStrCbCopyW( m_wszDiskPathId, sizeof(m_wszDiskPathId), wszDiskPathId ) );

    Call( ErrFaultInjection( 17352 ) );
    Alloc( m_pIOQueue = new IOQueue( &m_critIOQueue ) );

    //
    //  Process the registry parameters and defaults
    //

    m_cioOutstandingMax = pfsconfig->CIOMaxOutstanding();
    if ( FOSConfigGet( L"OS/IO", L"Total Quota", wszBuf, sizeof( wszBuf ) )
        && wszBuf[0] )
    {
        m_cioOutstandingMax = _wtol( wszBuf );
    }

    m_cioBackgroundMax = pfsconfig->CIOMaxOutstandingBackground();
    if ( FOSConfigGet( L"OS/IO", L"Background Quota", wszBuf, sizeof( wszBuf ) )
        && wszBuf[0] )
    {
        m_cioBackgroundMax = _wtol( wszBuf );
    }

    //  For urgent background (max) we choose 1/2 and 1/2 with foreground IOs.

    m_cioUrgentBackMax = CioDefaultUrgentOutstandingIOMax( m_cioOutstandingMax );
    if ( FOSConfigGet( L"OS/IO", L"Urgent Background Quota Max", wszBuf, sizeof( wszBuf ) )
        && wszBuf[0] )
    {
        m_cioUrgentBackMax = _wtol( wszBuf );
    }

    //
    //  Sanitize a bit
    //

    if ( m_cioOutstandingMax < 4 )
    {
        //  calculations won't work out if we don't have at least 3 outstanding.
        AssertSz( fFalse, "Total Quota / JET_paramOutstandingIOMax too low." );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    m_cioBackgroundMax = UlBound( m_cioBackgroundMax, 1, m_cioOutstandingMax - 2 );
    m_cioUrgentBackMax = UlBound( m_cioUrgentBackMax, m_cioBackgroundMax +1, m_cioOutstandingMax - 1 );
    
    // ensure urgent background is larger than background
    Assert( m_cioBackgroundMax <= (m_cioOutstandingMax / 2 ) );
    Assert( m_cioUrgentBackMax > m_cioBackgroundMax );

    //
    //  Initialize the IO Queue
    //

    Call( m_pIOQueue->ErrIOQueueInit( m_cioOutstandingMax, m_cioBackgroundMax, m_cioUrgentBackMax ) );

    //  open the physical disk handle

    WCHAR wszDiskPath[IFileSystemAPI::cchPathMax];
    OSStrCbFormatW( wszDiskPath, sizeof( wszDiskPath ), L"\\\\.\\PhysicalDrive%u", dwDiskNumber );

    m_hDisk = CreateFileW(  wszDiskPath,
                            0,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );

    m_osdi.m_errorOsdspd = ErrorOSDiskIOsStorageQueryProp( m_hDisk, StorageDeviceSeekPenaltyProperty, &m_osdi.m_osdspd, sizeof(m_osdi.m_osdspd) );
    if ( m_osdi.m_errorOsdspd == ERROR_SUCCESS )
    {
        OSTrace( JET_tracetagFile, OSFormat( "\t m_osdi.m_osdspd = { Ver.Size=%d.%d, IncursSeekPenalty=%d };\n",
                    m_osdi.m_osdspd.Version, m_osdi.m_osdspd.Size, m_osdi.m_osdspd.IncursSeekPenalty ) );
    }

    //  Best effort (at least some of this will not work / load if not admin or system)
    //  Disabling because of contention seen in repl because of repeated calls
    //  LoadDiskInfo_( wszDiskPath, dwDiskNumber );
    SetSmartEseNoLoadFailed( eSmartLoadDiskOpenFailed, ERROR_CALL_NOT_IMPLEMENTED, m_osdi.m_szDiskModel, sizeof(m_osdi.m_szDiskModel) );
    SetSmartEseNoLoadFailed( eSmartLoadDiskOpenFailed, ERROR_CALL_NOT_IMPLEMENTED, m_osdi.m_szDiskSerialNumber, sizeof(m_osdi.m_szDiskSerialNumber) );
    OSStrCbCopyA( m_osdi.m_szDiskFirmwareRev, sizeof(m_osdi.m_szDiskFirmwareRev), "EseNoLo" );

    m_eState = eOSDiskConnected;

HandleError:

    return err;
}

// --------------------------------------------
//  OS Disk Global Connect / Disconnect
//

ERR ErrOSDiskICreate(   _In_    IFileSystemConfiguration* const pfsconfig,
                        _In_z_  const WCHAR * const             wszDiskPathId, 
                        _In_    const DWORD                     dwDiskNumber,
                        _Out_   COSDisk** const                 pposd )
{
    ERR         err     = JET_errSuccess;
    COSDisk *   posd    = NULL;

    Assert( g_sxwlOSDisk.FOwnExclusiveLatch() );

    //  I'm not 100% sure we need to upgrade to an w-latch here ... but I'm afraid
    //  that an s-latch enumerator going against g_ilDiskList at the same time we're 
    //  calling InsertAsPrevMost() and causing some sort badness ...

    CSXWLatch::ERR errSXW = g_sxwlOSDisk.ErrUpgradeExclusiveLatchToWriteLatch();
    if ( CSXWLatch::ERR::errWaitForWriteLatch == errSXW )
    {
        g_sxwlOSDisk.WaitForWriteLatch();
        errSXW = CSXWLatch::ERR::errSuccess;
    }
    Assert( errSXW == CSXWLatch::ERR::errSuccess );

    Call( ErrFaultInjection( 17348 ) );
    Alloc( posd = new COSDisk() );

    Call( posd->ErrInitDisk( pfsconfig, wszDiskPathId, dwDiskNumber ) );

    //
    //  Must not fail after here ...
    //

    posd->AddRef();

    Assert( posd->CRef() >= 1 );

    g_ilDiskList.InsertAsPrevMost( posd );

    Assert( eOSDiskLastDiskOnEarthMode != GetDiskMappingMode() || NULL == g_ilDiskList.Next( posd ) );
    Assert( eOSDiskInvalidMode != GetDiskMappingMode() );

    *pposd = posd;

    ASSERT_VALID( posd );

    err = JET_errSuccess;

HandleError:

    if ( err < JET_errSuccess )
    {
        delete posd;
        Assert( NULL == *pposd );
    }

    g_sxwlOSDisk.DowngradeWriteLatchToExclusiveLatch();

    return err;
}

ERR ErrOSDiskIFind( __in_z const WCHAR * const wszDiskPath, _Out_ COSDisk ** pposd )
{
    ERR         err     = errNotFound;

    Assert( wszDiskPath );
    Assert( pposd );

    Assert( g_sxwlOSDisk.FOwnSharedLatch() || g_sxwlOSDisk.FOwnExclusiveLatch() );

    COSDisk * posd = NULL;

    *pposd = NULL;

    for ( posd = g_ilDiskList.PrevMost(); posd != NULL; posd = g_ilDiskList.Next( posd ) )
    {
        if ( posd->FIsDisk( wszDiskPath ) )
        {
            *pposd = (COSDisk*)posd;
            err = JET_errSuccess;
        }
    }

    return err;
}

void OSDiskIDelete( __inout COSDisk * posd )
{
    Assert( g_sxwlOSDisk.FOwnWriteLatch() );

    Assert( posd );
    Assert( 0 == posd->CRef() );
    Assert( 0 == posd->CioOutstanding() );

    Assert( g_ilDiskList.FMember( posd ) );
    g_ilDiskList.Remove( posd );

    //  Should be only 1 disk in this mode.
    Assert( eOSDiskLastDiskOnEarthMode != GetDiskMappingMode() || g_ilDiskList.FEmpty() );
    Assert( eOSDiskInvalidMode != GetDiskMappingMode() );

    delete posd;
}

class OSDiskEnumerator
{
    private:
        COSDisk * m_posdCurr;

    public:
        OSDiskEnumerator() :
            m_posdCurr( NULL )
            { }

        COSDisk * PosdNext()
        {
            if ( NULL == m_posdCurr )
            {
                //  Null, so start at beginning of list.

                g_sxwlOSDisk.AcquireSharedLatch();

                m_posdCurr = g_ilDiskList.PrevMost();
            }
            else
            {
                //  Jump to the next COSDisk in a safe way

                g_sxwlOSDisk.AcquireExclusiveLatch();

                COSDisk * posdPrev = m_posdCurr;

                m_posdCurr = g_ilDiskList.Next( m_posdCurr );

                COSDisk::Release( posdPrev );   // Must not touch posdPrev after this point ...

                g_sxwlOSDisk.DowngradeExclusiveLatchToSharedLatch();
            }

            if ( m_posdCurr )
            {
                m_posdCurr->AddRef();
            }

            g_sxwlOSDisk.ReleaseSharedLatch();

            return m_posdCurr;
        }

        void Quit()
        {
            if ( m_posdCurr )
            {
                g_sxwlOSDisk.AcquireExclusiveLatch();

                COSDisk::Release( m_posdCurr ); // Must not touch m_posdCurr after this point ...

                m_posdCurr = NULL;

                g_sxwlOSDisk.ReleaseExclusiveLatch();
            }
        }

        ~OSDiskEnumerator()
        {
            // should have finished or called Quit() ...
            Assert( NULL == m_posdCurr );

            Quit(); // just in case.
        }

};

ERR ErrOSDiskConnect(   _In_    IFileSystemConfiguration* const pfsconfig,
                        _In_z_  const WCHAR * const             wszDiskPathId, 
                        _In_    const DWORD                     dwDiskNumber,
                        _Out_   IDiskAPI** const                ppdiskapi )
{
    ERR         err     = JET_errSuccess;
    COSDisk *   posd    = NULL;
    BOOL        fEL     = fFalse;

    Assert( eOSDiskInvalidMode != GetDiskMappingMode() );

    g_sxwlOSDisk.AcquireSharedLatch();

    CallSx( err = ErrOSDiskIFind( wszDiskPathId, &posd ), errNotFound );
    Assert( err == JET_errSuccess || posd == NULL );

    if ( NULL == posd )
    {
        g_sxwlOSDisk.ReleaseSharedLatch();
        g_sxwlOSDisk.AcquireExclusiveLatch();
        fEL = fTrue;

        CallSx( err = ErrOSDiskIFind( wszDiskPathId, &posd ), errNotFound );
        Assert( err == JET_errSuccess || posd == NULL );
    }

    if ( NULL == posd )
    {
        Call( ErrOSDiskICreate( pfsconfig, wszDiskPathId, dwDiskNumber, &posd ) );
        OSTrace( JET_tracetagDiskVolumeManagement, OSFormat( "Created Disk p=%p, PathId=%ws", posd, posd->WszDiskPathId() ) );
    }
    else
    {
        posd->AddRef();
        OSTrace( JET_tracetagDiskVolumeManagement, OSFormat( "Add Ref Disk p=%p, PathId=%ws, Cref=%d", posd, posd->WszDiskPathId(), posd->CRef() ) );
    }

    ASSERT_VALID( posd );

    Assert( *ppdiskapi == NULL || *ppdiskapi == posd );
    *ppdiskapi = (void*)posd;

HandleError:

    if ( fEL )
    {
        g_sxwlOSDisk.ReleaseExclusiveLatch();
    }
    else
    {
        g_sxwlOSDisk.ReleaseSharedLatch();
    }

    return err;
}

void OSDiskConnect( _Inout_ COSDisk * posd )
{
    Assert( posd );

    g_sxwlOSDisk.AcquireSharedLatch();

    posd->AddRef();
    OSTrace( JET_tracetagDiskVolumeManagement, OSFormat( "Add[Fast] Ref Disk p=%p, PathId=%ws, Cref=%d", posd, posd->WszDiskPathId(), posd->CRef() ) );

    ASSERT_VALID( posd );

    g_sxwlOSDisk.ReleaseSharedLatch();
}


//  Dereferences the  pdiskapi and asserts
//  the IO context (_OSFILE*) has no lingering references / outstanding IO.

void OSDiskDisconnect(
    __inout IDiskAPI *          pdiskapi,
    _In_    const _OSFILE *     p_osf )
{

    g_sxwlOSDisk.AcquireExclusiveLatch();

    Assert( pdiskapi );

    COSDisk * posd = (COSDisk*)pdiskapi;

    if ( p_osf )
    {
        AssertIoContextQuiesced( p_osf );
    }
    
    COSDisk::Release( posd );

    g_sxwlOSDisk.ReleaseExclusiveLatch();

}



// =============================================================================================
//  I/O Queue & Heap Implementation
// =============================================================================================
//

////////////////
//  I/O Heap A

//  initializes the I/O Heap A, or returns JET_errOutOfMemory

ERR COSDisk::IOQueue::IOHeapA::ErrHeapAInit( _In_ LONG cIOEnqueuedMax )
{
    ERR err;

    //  reset all pointers

    rgpioreqIOAHeap = NULL;

    ipioreqIOAHeapMax = cIOEnqueuedMax;

    //  ensure mathematical saneness ...
    if( ((0x7FFFFFFF / ipioreqIOAHeapMax) <= sizeof(IOREQ*)) ||
        ((ipioreqIOAHeapMax * sizeof(IOREQ*)) < max(ipioreqIOAHeapMax, sizeof(IOREQ*)))
#ifdef DEBUGGER_EXTENSION
    // not sure why the build complains this clause always evaluates to false when MINESE / MINIMAL_FUNCTIONALITY build option is set.
        ||
        ((ipioreqIOAHeapMax * sizeof(IOREQ*)) < 0)
#endif
        )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    //  allocate storage for the I/O Heap A
    Call( ErrFaultInjection( 17368 ) );
    if ( !( rgpioreqIOAHeap = (IOREQ**) PvOSMemoryHeapAlloc( ipioreqIOAHeapMax * sizeof( IOREQ* ) ) ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    //  initialize the I/O Heap A to be empty

    ipioreqIOAHeapMac = 0;

    return JET_errSuccess;

HandleError:
    _HeapATerm();
    return err;
}

//  terminates the I/O Heap A

void COSDisk::IOQueue::IOHeapA::_HeapATerm()
{
    Assert( 0 == CioreqHeapA() );

    //  free I/O Heap A storage

    if ( rgpioreqIOAHeap != NULL )
    {
        OSMemoryHeapFree( (void*)rgpioreqIOAHeap );
        rgpioreqIOAHeap = NULL;
    }
}

//  returns fTrue if the I/O Heap A is empty

INLINE BOOL COSDisk::IOQueue::IOHeapA::FHeapAEmpty() const
{
    Assert( m_pcrit->FOwner() );
    return !ipioreqIOAHeapMac;
}

//  returns the count of IOREQs in the I/O Heap A

INLINE LONG COSDisk::IOQueue::IOHeapA::CioreqHeapA() const
{
    return ipioreqIOAHeapMac;
}

//  returns IOREQ at the top of the I/O Heap A, or NULL if empty

INLINE IOREQ* COSDisk::IOQueue::IOHeapA::PioreqHeapATop()
{
    Assert( m_pcrit->FOwner() );
    Assert( !FHeapAEmpty() );
    return rgpioreqIOAHeap[0];
}

//  adds a IOREQ to the I/O Heap A

INLINE void COSDisk::IOQueue::IOHeapA::HeapAAdd( IOREQ* pioreq )
{
    //  critical section

    Assert( m_pcrit->FOwner() );

    //  new value starts at bottom of heap

    LONG ipioreq = ipioreqIOAHeapMac++;

    //  percolate new value up the heap

    while ( ipioreq > 0 &&
            FHeapAISmaller( pioreq, rgpioreqIOAHeap[IpioreqHeapAIParent( ipioreq )] ) )
    {
        Assert( rgpioreqIOAHeap[IpioreqHeapAIParent( ipioreq )]->ipioreqHeap == IpioreqHeapAIParent( ipioreq ) );
        rgpioreqIOAHeap[ipioreq] = rgpioreqIOAHeap[IpioreqHeapAIParent( ipioreq )];
        rgpioreqIOAHeap[ipioreq]->ipioreqHeap = ipioreq;
        ipioreq = IpioreqHeapAIParent( ipioreq );
    }

    //  put new value in its designated spot

    rgpioreqIOAHeap[ipioreq] = pioreq;
    pioreq->ipioreqHeap = ipioreq;
}

//  removes a IOREQ from the I/O Heap A

INLINE void COSDisk::IOQueue::IOHeapA::HeapARemove( IOREQ* pioreq )
{
    //  critical section

    Assert( m_pcrit->FOwner() );

    //  remove the specified IOREQ from the heap

    LONG ipioreq = pioreq->ipioreqHeap;

    //  if this IOREQ was at the end of the heap, we're done

    if ( ipioreq == ipioreqIOAHeapMac - 1 )
    {
#ifdef DEBUG
        rgpioreqIOAHeap[ipioreqIOAHeapMac - 1] = (IOREQ*) 0xBAADF00DBAADF00D;
#endif  //  DEBUG
        ipioreqIOAHeapMac--;
        return;
    }

    //  copy IOREQ from tend of heap to fill removed IOREQ's vacancy

    rgpioreqIOAHeap[ipioreq] = rgpioreqIOAHeap[ipioreqIOAHeapMac - 1];
    rgpioreqIOAHeap[ipioreq]->ipioreqHeap = ipioreq;
#ifdef DEBUG
    rgpioreqIOAHeap[ipioreqIOAHeapMac - 1] = (IOREQ*) 0xBAADF00DBAADF00D;
#endif  //  DEBUG
    ipioreqIOAHeapMac--;

    //  update filler OSFiles position

    HeapAIUpdate( rgpioreqIOAHeap[ipioreq] );
}

//  updates a IOREQ's position in the I/O Heap A if its weight has changed

void COSDisk::IOQueue::IOHeapA::HeapAIUpdate( IOREQ* pioreq )
{
    //  critical section

    Assert( m_pcrit->FOwner() );

    //  get the specified IOREQ's position

    LONG ipioreq = pioreq->ipioreqHeap;
    Assert( rgpioreqIOAHeap[ipioreq] == pioreq );

    //  percolate IOREQ up the heap

    while ( ipioreq > 0 &&
            FHeapAISmaller( pioreq, rgpioreqIOAHeap[IpioreqHeapAIParent( ipioreq )] ) )
    {
        Assert( rgpioreqIOAHeap[IpioreqHeapAIParent( ipioreq )]->ipioreqHeap == IpioreqHeapAIParent( ipioreq ) );
        rgpioreqIOAHeap[ipioreq] = rgpioreqIOAHeap[IpioreqHeapAIParent( ipioreq )];
        rgpioreqIOAHeap[ipioreq]->ipioreqHeap = ipioreq;
        ipioreq = IpioreqHeapAIParent( ipioreq );
    }

    //  percolate IOREQ down the heap

    while ( ipioreq < ipioreqIOAHeapMac )
    {
        //  if we have no children, stop here

        if ( IpioreqHeapAILeftChild( ipioreq ) >= ipioreqIOAHeapMac )
            break;

        //  set child to smaller child

        LONG ipioreqChild;
        if (    IpioreqHeapAIRightChild( ipioreq ) < ipioreqIOAHeapMac &&
                FHeapAISmaller( rgpioreqIOAHeap[IpioreqHeapAIRightChild( ipioreq )],
                                        rgpioreqIOAHeap[IpioreqHeapAILeftChild( ipioreq )] ) )
        {
            ipioreqChild = IpioreqHeapAIRightChild( ipioreq );
        }
        else
        {
            ipioreqChild = IpioreqHeapAILeftChild( ipioreq );
        }

        //  if we are smaller than the smallest child, stop here

        if ( FHeapAISmaller( pioreq, rgpioreqIOAHeap[ipioreqChild] ) )
            break;

        //  trade places with smallest child and continue down

        Assert( rgpioreqIOAHeap[ipioreqChild]->ipioreqHeap == ipioreqChild );
        rgpioreqIOAHeap[ipioreq] = rgpioreqIOAHeap[ipioreqChild];
        rgpioreqIOAHeap[ipioreq]->ipioreqHeap = ipioreq;
        ipioreq = ipioreqChild;
    }
    Assert( ipioreq < ipioreqIOAHeapMac );

    //  put IOREQ in its designated spot

    rgpioreqIOAHeap[ipioreq] = pioreq;
    pioreq->ipioreqHeap = ipioreq;
}

//  returns fTrue if the first IOREQ is smaller than the second IOREQ

INLINE BOOL COSDisk::IOQueue::IOHeapA::FHeapAISmaller( IOREQ* pioreq1, IOREQ* pioreq2 ) const
{
    return CmpOSDiskIFileIbIFileIb(   pioreq1->p_osf->iFile,
                                        pioreq1->ibOffset,
                                        pioreq2->p_osf->iFile,
                                        pioreq2->ibOffset ) < 0;
}

//  returns the index to the parent of the given child

INLINE LONG COSDisk::IOQueue::IOHeapA::IpioreqHeapAIParent( LONG ipioreq ) const
{
    return ( ipioreq - 1 ) / 2;
}

//  returns the index to the left child of the given parent

INLINE LONG COSDisk::IOQueue::IOHeapA::IpioreqHeapAILeftChild( LONG ipioreq ) const
{
    return 2 * ipioreq + 1;
}

//  returns the index to the right child of the given parent

INLINE LONG COSDisk::IOQueue::IOHeapA::IpioreqHeapAIRightChild( LONG ipioreq ) const
{
    return 2 * ipioreq + 2;
}

INLINE BOOL COSDisk::IOQueue::IOHeapA::FHeapAFrom( const IOREQ * pioreq ) const
{
    return pioreq->ipioreqHeap < ipioreqIOAHeapMac;
}

////////////////
//  I/O Heap B

//  initializes the I/O Heap B, or returns JET_errOutOfMemory

ERR COSDisk::IOQueue::IOHeapB::ErrHeapBInit(
                IOREQ* volatile *   rgpioreqIOAHeap,
                LONG                ipioreqIOAHeapMax
    )
{
    //  I/O Heap B uses the heap memory that is not used by the
    //  I/O Heap A, and therefore must be initialized second

    Assert( rgpioreqIOAHeap );

    rgpioreqIOBHeap = rgpioreqIOAHeap;
    ipioreqIOBHeapMax = ipioreqIOAHeapMax;

    //  initialize the I/O Heap B to be empty

    ipioreqIOBHeapMic = ipioreqIOBHeapMax;

    return JET_errSuccess;
}

//  terminates the I/O Heap B

void COSDisk::IOQueue::IOHeapB::_HeapBTerm()
{
    Assert( 0 == CioreqHeapB() );
    //  nop
}

//  returns fTrue if the I/O Heap B is empty

INLINE BOOL COSDisk::IOQueue::IOHeapB::FHeapBEmpty() const
{
    Assert( m_pcrit->FOwner() );
    return ipioreqIOBHeapMic == ipioreqIOBHeapMax;
}

//  returns the count of IOREQs in the I/O Heap B

INLINE LONG COSDisk::IOQueue::IOHeapB::CioreqHeapB() const
{
    return LONG( ipioreqIOBHeapMax - ipioreqIOBHeapMic );
}

//  returns IOREQ at the top of the I/O Heap B, or NULL if empty

INLINE IOREQ* COSDisk::IOQueue::IOHeapB::PioreqHeapBTop()
{
    Assert( m_pcrit->FOwner() );
    Assert( !FHeapBEmpty() );
    return rgpioreqIOBHeap[ipioreqIOBHeapMax - 1];
}

//  adds a IOREQ to the I/O Heap B

INLINE void COSDisk::IOQueue::IOHeapB::HeapBAdd( IOREQ* pioreq )
{
    //  critical section

    Assert( m_pcrit->FOwner() );

    //  new value starts at bottom of heap

    LONG ipioreq = --ipioreqIOBHeapMic;

    //  percolate new value up the heap

    while ( ipioreqIOBHeapMax > 0 &&
            ipioreq < ipioreqIOBHeapMax - 1 &&
            FHeapBISmaller( pioreq, rgpioreqIOBHeap[IpioreqHeapBIParent( ipioreq )] ) )
    {
        Assert( rgpioreqIOBHeap[IpioreqHeapBIParent( ipioreq )]->ipioreqHeap == IpioreqHeapBIParent( ipioreq ) );
        rgpioreqIOBHeap[ipioreq] = rgpioreqIOBHeap[IpioreqHeapBIParent( ipioreq )];
        rgpioreqIOBHeap[ipioreq]->ipioreqHeap = ipioreq;
        ipioreq = IpioreqHeapBIParent( ipioreq );
    }

    //  put new value in its designated spot

    rgpioreqIOBHeap[ipioreq] = pioreq;
    pioreq->ipioreqHeap = ipioreq;
}

//  removes a IOREQ from the I/O Heap B

INLINE void COSDisk::IOQueue::IOHeapB::HeapBRemove( IOREQ* pioreq )
{
    //  critical section

    Assert( m_pcrit->FOwner() );

    //  remove the specified IOREQ from the heap

    LONG ipioreq = pioreq->ipioreqHeap;

    //  if this IOREQ was at the end of the heap, we're done

    if ( ipioreq == ipioreqIOBHeapMic )
    {
#ifdef DEBUG
        rgpioreqIOBHeap[ipioreqIOBHeapMic] = (IOREQ*)0xBAADF00DBAADF00D;
#endif  //  DEBUG
        ipioreqIOBHeapMic++;
        return;
    }

    //  copy IOREQ from end of heap to fill removed IOREQ's vacancy

    rgpioreqIOBHeap[ipioreq] = rgpioreqIOBHeap[ipioreqIOBHeapMic];
    rgpioreqIOBHeap[ipioreq]->ipioreqHeap = ipioreq;
#ifdef DEBUG
    rgpioreqIOBHeap[ipioreqIOBHeapMic] = (IOREQ*)0xBAADF00DBAADF00D;
#endif  //  DEBUG
    ipioreqIOBHeapMic++;

    //  update filler IOREQs position

    HeapBIUpdate( rgpioreqIOBHeap[ipioreq] );
}

//  returns fTrue if the first IOREQ is smaller than the second IOREQ

INLINE BOOL COSDisk::IOQueue::IOHeapB::FHeapBISmaller( IOREQ* pioreq1, IOREQ* pioreq2 ) const
{
    return CmpOSDiskIFileIbIFileIb(   pioreq1->p_osf->iFile,
                                        pioreq1->ibOffset,
                                        pioreq2->p_osf->iFile,
                                        pioreq2->ibOffset ) < 0;
}

//  returns the index to the parent of the given child

INLINE LONG COSDisk::IOQueue::IOHeapB::IpioreqHeapBIParent( LONG ipioreq ) const
{
    return ipioreqIOBHeapMax - 1 - ( ipioreqIOBHeapMax - 1 - ipioreq - 1 ) / 2;
}

//  returns the index to the left child of the given parent

INLINE LONG COSDisk::IOQueue::IOHeapB::IpioreqHeapBILeftChild( LONG ipioreq ) const
{
    return ipioreqIOBHeapMax - 1 - ( 2 * ( ipioreqIOBHeapMax - 1 - ipioreq ) + 1 );
}

//  returns the index to the right child of the given parent

INLINE LONG COSDisk::IOQueue::IOHeapB::IpioreqHeapBIRightChild( LONG ipioreq ) const
{
    return ipioreqIOBHeapMax - 1 - ( 2 * ( ipioreqIOBHeapMax - 1 - ipioreq ) + 2 );
}

//  updates a IOREQ's position in the I/O Heap B if its weight has changed

void COSDisk::IOQueue::IOHeapB::HeapBIUpdate( IOREQ* pioreq )
{
    //  get the specified IOREQ's position

    LONG ipioreq = pioreq->ipioreqHeap;
    Assert( rgpioreqIOBHeap[ipioreq] == pioreq );

    //  percolate IOREQ up the heap

    while ( ipioreqIOBHeapMax > 0 &&
            ipioreq < ipioreqIOBHeapMax - 1 &&
            FHeapBISmaller( pioreq, rgpioreqIOBHeap[IpioreqHeapBIParent( ipioreq )] ) )
    {
        Assert( rgpioreqIOBHeap[IpioreqHeapBIParent( ipioreq )]->ipioreqHeap == IpioreqHeapBIParent( ipioreq ) );
        rgpioreqIOBHeap[ipioreq] = rgpioreqIOBHeap[IpioreqHeapBIParent( ipioreq )];
        rgpioreqIOBHeap[ipioreq]->ipioreqHeap = ipioreq;
        ipioreq = IpioreqHeapBIParent( ipioreq );
    }

    //  percolate IOREQ down the heap

    while ( ipioreq >= ipioreqIOBHeapMic )
    {
        //  if we have no children, stop here

        if ( IpioreqHeapBILeftChild( ipioreq ) < ipioreqIOBHeapMic )
            break;

        //  set child to smaller child

        LONG ipioreqChild;
        if (    IpioreqHeapBIRightChild( ipioreq ) >= ipioreqIOBHeapMic &&
                FHeapBISmaller( rgpioreqIOBHeap[IpioreqHeapBIRightChild( ipioreq )],
                                            rgpioreqIOBHeap[IpioreqHeapBILeftChild( ipioreq )] ) )
        {
            ipioreqChild = IpioreqHeapBIRightChild( ipioreq );
        }
        else
        {
            ipioreqChild = IpioreqHeapBILeftChild( ipioreq );
        }

        //  if we are smaller than the smallest child, stop here

        if ( FHeapBISmaller( pioreq, rgpioreqIOBHeap[ipioreqChild] ) )
            break;

        //  trade places with smallest child and continue down

        Assert( rgpioreqIOBHeap[ipioreqChild]->ipioreqHeap == ipioreqChild );
        rgpioreqIOBHeap[ipioreq] = rgpioreqIOBHeap[ipioreqChild];
        rgpioreqIOBHeap[ipioreq]->ipioreqHeap = ipioreq;
        ipioreq = ipioreqChild;
    }
    Assert( ipioreq >= ipioreqIOBHeapMic );

    //  put IOREQ in its designated spot

    rgpioreqIOBHeap[ipioreq] = pioreq;
    pioreq->ipioreqHeap = ipioreq;
}

COSDisk::IOQueueToo::IOQueueToo( CCriticalSection * pcritController, const QueueInitType qitInit ) :
    CZeroInit( sizeof(*this) ),

#ifdef DEBUG
    m_qit( qitInit ),
    m_pcritIoQueue( pcritController ),
#endif

    m_hrtBuildingStart( 0 ),
    // implicitly: m_irbtBuilding(),
    // implicitly: m_irbtDraining(),
    m_cioreqEnqueued( 0 ),
    m_cioEnqueued( 0 )
{
}

COSDisk::IOQueueToo::~IOQueueToo()
{
    Assert( m_cioreqEnqueued == 0 );
    Assert( m_cioEnqueued == 0 );
}

//  Returns fTrue if and only if the iorun was accepted as a new IO (and not merged with another).  Returns ppioreqHeadAccepted
//  of NULL if the function failed to add the IO to the Q (which can happen for duplicates).

BOOL COSDisk::IOQueueToo::FInsertIo( _In_ IOREQ * pioreqToEnqueue, _In_ const LONG cioreqRun, _Out_ IOREQ ** ppioreqHeadAccepted )
{
    *ppioreqHeadAccepted = pioreqToEnqueue;

    Assert( pioreqToEnqueue->FEnqueuedInMetedQ() );  // we just set of qop, so should be true on IOREQ * from there ...

    Assert( !!( m_qit & qitWrite ) == !!pioreqToEnqueue->fWrite );
    Assert( !!( m_qit & qitRead ) == !pioreqToEnqueue->fWrite ); // not really necessary (but would go off if someone passes both read & write).

    const IFILEIBOFFSET ifileiboffsetToEnqueue = { pioreqToEnqueue->p_osf->iFile, pioreqToEnqueue->ibOffset };

    //  MUST own this critical section to manage the m_irbtBuilding/m_irbtDraining.
    Assert( m_pcritIoQueue->FOwner() );

    BOOL fConsumedInExistingRun = fFalse;

    if ( m_irbtBuilding.FEmpty() )
    {
        m_hrtBuildingStart = HrtHRTCount();
    }

    IFILEIBOFFSET ifileiboffsetIoFound = IFILEIBOFFSET::IfileiboffsetNotFound;
    IOREQ * pioreqIo = NULL;
    if ( IRBTQ::ERR::errSuccess == m_irbtBuilding.ErrFindNearest( ifileiboffsetToEnqueue, &ifileiboffsetIoFound, &pioreqIo ) )
    {
        //  N^2 even for serial IO ...  
        IORun iorun( pioreqIo );

        if ( ifileiboffsetToEnqueue.iFile == pioreqIo->p_osf->iFile &&
                ifileiboffsetToEnqueue.ibOffset == pioreqIo->ibOffset )
        {
            // Well, we both seem to want to wear the same outfit ... this is awkward.  The RedBlackTree does not support
            // key duplicates, and so we won't be able to insert this IOREQ there.  NULL out the returned pioreqTraceHead
            // to indicating we did not accept the IOREQ.
            *ppioreqHeadAccepted = NULL;
            return fFalse;
        }

        if ( iorun.FAddToRun( pioreqToEnqueue ) )
        {
            //  This was subsummed in a run already in the Q
            IOREQ * pioreqIoNew = iorun.PioreqGetRun();
            if ( pioreqIoNew != pioreqIo )
            {
                //  Oh awkward, we've changed the pioreq head (of the IO found) ... 
                Assert( pioreqIoNew == pioreqToEnqueue );

                AssertTrack( ifileiboffsetIoFound.iFile == pioreqIo->p_osf->iFile, "JustFoundIoMismatchFileId" );
                AssertTrack( ifileiboffsetIoFound.ibOffset == pioreqIo->ibOffset, "JustFoundIoMismatchOffset" );
                AssertTrack( ifileiboffsetToEnqueue.iFile == pioreqIoNew->p_osf->iFile, "CombinedIoRunShouldMatchInsertTargetFileId" );
                AssertTrack( ifileiboffsetToEnqueue.ibOffset == pioreqIoNew->ibOffset, "CombinedIoRunShouldMatchInsertTargetOffset" );

                // either order would be fine ... 
                const IRBTQ::ERR errDel = m_irbtBuilding.ErrDelete( ifileiboffsetIoFound );
                const IRBTQ::ERR errIns = m_irbtBuilding.ErrInsert( ifileiboffsetToEnqueue, pioreqIoNew );
                AssertTrack( IRBTQ::ERR::errSuccess == errDel, "JustFoundIoCouldntDelete" );
                AssertTrack( IRBTQ::ERR::errSuccess == errIns, "AllocationlessInsertShouldSucceed" );

                IOREQ * pioreqT = NULL;
                AssertSz( IRBTQ::ERR::errSuccess == m_irbtBuilding.ErrFind( ifileiboffsetToEnqueue, &pioreqT ), "EnqueuedIoShouldBePresentInBuildingQ" );
                AssertSz( IRBTQ::ERR::errEntryNotFound == m_irbtBuilding.ErrFind( ifileiboffsetIoFound, &pioreqT ), "OrigIoShouldNotBePresentInBuildingQ" );
            }
            m_cioreqEnqueued += cioreqRun;
            fConsumedInExistingRun = fTrue;
            *ppioreqHeadAccepted = pioreqIoNew;
        }
    }

    if ( !fConsumedInExistingRun )
    {
        const IRBTQ::ERR errIns = m_irbtBuilding.ErrInsert( ifileiboffsetToEnqueue, pioreqToEnqueue );
        AssertTrack( IRBTQ::ERR::errSuccess == errIns, "AllocationlessInsertShouldSucceedPossDupEntry" );
        m_cioreqEnqueued += cioreqRun;
        m_cioEnqueued++;
    }

    Assert( !m_irbtBuilding.FEmpty() );

    return !fConsumedInExistingRun;
}


const IFILEIBOFFSET IFILEIBOFFSET::IfileiboffsetNotFound = { qwMax, qwMax };

IFILEIBOFFSET COSDisk::IOQueueToo::IfileiboffsetFirstIo( _In_ const QueueFirstIoFlags qfif )
{
    Assert( m_pcritIoQueue->FOwner() );

    IFILEIBOFFSET ifileiboffsetLowest = { 0, 0 };
    IFILEIBOFFSET ifileiboffsetIoFound = IFILEIBOFFSET::IfileiboffsetNotFound;
    IOREQ * pioreqT = NULL;

    if ( ( qfif & qfifDraining ) &&
         ( IRBTQ::ERR::errSuccess == m_irbtDraining.ErrFindNearest( ifileiboffsetLowest, &ifileiboffsetIoFound, &pioreqT ) ) )
    {
        return ifileiboffsetIoFound;
    }
    Assert( IFILEIBOFFSET::IfileiboffsetNotFound == ifileiboffsetIoFound );

    if ( ( qfif & qfifBuilding ) &&
         ( IRBTQ::ERR::errSuccess == m_irbtBuilding.ErrFindNearest( ifileiboffsetLowest, &ifileiboffsetIoFound, &pioreqT ) ) )
    {
        return ifileiboffsetIoFound;
    }

    Assert( IFILEIBOFFSET::IfileiboffsetNotFound == ifileiboffsetIoFound );
    
    return ifileiboffsetIoFound;
}

void COSDisk::IOQueueToo::Cycle()
{
    Assert( m_pcritIoQueue->FOwner() );

    (void)m_irbtDraining.ErrMerge( &m_irbtBuilding );
    //  Even though an error may leave some left overs in m_irbtBuilding, eventually m_irbtDraining will be
    //  fully drained and then m_irbtBuilding will be able to merge over.
}

//  Retrieves an IO from the "Q"
//
//    pfCycles IN / OUT - 
//        IN  - Whether or not to auto-cycle the Q and swap the building into the draining if the building is empty.
//        OUT - Whether auto-cycle happened and the building WAS swapped into building.

void COSDisk::IOQueueToo::ExtractIo( _Inout_ COSDisk::QueueOp * const pqop, _Out_ IOREQ ** ppioreqTraceHead, _Inout_ BOOL * pfCycles )
{
    Assert( m_pcritIoQueue->FOwner() );
    Assert( pfCycles );

    const BOOL fAutoCycle = *pfCycles;
    *pfCycles = fFalse;

    //  find the next appropriate IO

    IFILEIBOFFSET ifileiboffsetLowest = { 0, 0 };
    IFILEIBOFFSET ifileiboffsetIoFound = IFILEIBOFFSET::IfileiboffsetNotFound;

    OnDebug( IFILEIBOFFSET ifileiboffsetBuildingBefore = IfileiboffsetFirstIo( qfifBuilding ) );
    OnDebug( IFILEIBOFFSET ifileiboffsetDrainingBefore = IfileiboffsetFirstIo( qfifDraining ) );
    Assert( !m_irbtBuilding.FEmpty() || ifileiboffsetBuildingBefore == IFILEIBOFFSET::IfileiboffsetNotFound );
    Assert( !m_irbtDraining.FEmpty() || ifileiboffsetDrainingBefore == IFILEIBOFFSET::IfileiboffsetNotFound );

    IOREQ * pioreqIo = NULL;
    IRBTQ::ERR err = m_irbtDraining.ErrFindNearest( ifileiboffsetLowest, &ifileiboffsetIoFound, &pioreqIo );
    if ( err != IRBTQ::ERR::errSuccess )
    {
        Assert( err == IRBTQ::ERR::errEntryNotFound );
        AssertTrack( m_irbtDraining.FEmpty(), "Searched00AndNotEmptyRbt" );  //  we just searched for 0,0 and found nothing ... should be empty.

        if ( fAutoCycle )
        {
            //  nothing in draining Q, swap the building one to be draining ...

            Cycle();
            AssertTrack( m_irbtBuilding.FEmpty(), "TransferShouldLeaveBuildingQEmpty" );
            *pfCycles = fTrue;

            //  also reset the starvation level

            m_hrtBuildingStart = 0;

            ifileiboffsetIoFound = IFILEIBOFFSET::IfileiboffsetNotFound; // reset just in case
            Assert( ifileiboffsetIoFound == IFILEIBOFFSET::IfileiboffsetNotFound );
            Assert( ifileiboffsetIoFound.iFile == qwMax );    // C still works right?  Just checking.
            Assert( ifileiboffsetIoFound.ibOffset == qwMax );
            err = m_irbtDraining.ErrFindNearest( ifileiboffsetLowest, &ifileiboffsetIoFound, &pioreqIo );
        }


        if ( err != IRBTQ::ERR::errSuccess )
        {
            //  Queue is empty ...
            Assert( err == IRBTQ::ERR::errEntryNotFound );
            Assert( ifileiboffsetDrainingBefore == IFILEIBOFFSET::IfileiboffsetNotFound );
            if ( fAutoCycle )
            {
                Assert( ifileiboffsetBuildingBefore == IFILEIBOFFSET::IfileiboffsetNotFound || !fAutoCycle );
                Assert( m_cioEnqueued == 0 );
                Assert( m_cioreqEnqueued == 0 );
                Assert( *pfCycles );
            }
            else
            {
                Assert( !*pfCycles );
            }
            return;
        }
    }

    //  check find (and/or cycle) consistent

    Assert( pioreqIo );
    Assert( ifileiboffsetIoFound.iFile != qwMax && ifileiboffsetIoFound.ibOffset != qwMax );
    AssertTrack( !( ifileiboffsetIoFound == IFILEIBOFFSET::IfileiboffsetNotFound ), "FoundIoKeyShouldNotBeMaxNotFound" );
    Assert( !*pfCycles || ifileiboffsetDrainingBefore == IFILEIBOFFSET::IfileiboffsetNotFound );
    Assert( !*pfCycles || !( ifileiboffsetBuildingBefore == IFILEIBOFFSET::IfileiboffsetNotFound ) ); // or we would've bailed from "Queue is empty" clause above ...

    if ( ifileiboffsetIoFound.iFile != pioreqIo->p_osf->iFile )
    {
        //  This means the RedBlackTree is operating incorrectly - and while at this point it's fine we do not
        //  depend on this value, the FInsertIo() code would actually have serious problems.
        FireWall( "FoundIoKeyAndObjectMismatchFileId" );
    }
    if ( ifileiboffsetIoFound.ibOffset != pioreqIo->ibOffset )
    {
        //  This means the RedBlackTree is operating incorrectly - and while at this point it's fine we do not
        //  depend on this value, the FInsertIo() code would actually have serious problems.
        FireWall( "FoundIoKeyAndObjectMismatchOffset" );
    }

    Assert( pioreqIo->FEnqueuedInMetedQ() );

    //  remove from Q   

    const IRBTQ::ERR errDel = m_irbtDraining.ErrDelete( ifileiboffsetIoFound );
    AssertTrack( IRBTQ::ERR::errSuccess == errDel, "JustFoundIoCouldntDelete" );


    //  set out var and queue tracking numbers

    *ppioreqTraceHead = pioreqIo;
    OnDebug( const BOOL fAdd = )
        pqop->FAddToRun( pioreqIo );
    Assert( fAdd );

    m_cioreqEnqueued -= pqop->Cioreq();
    m_cioEnqueued--;
}

LONG CioreqRun( const IOREQ * pioreq )
{
    LONG cioreq = 0;
    while( pioreq )
    {
        cioreq++;
        pioreq = pioreq->pioreqIorunNext;
    }
    return cioreq;
}

LONG g_dtickStarvedMetedOpThreshold = 3000; // controlled by JET_paramFlight_MetedOpStarvedThreshold

BOOL COSDisk::IOQueueToo::FStarvedMetedOp() const
{
    Assert( FIOThread() );
    
    if ( m_hrtBuildingStart != 0 )
    {
        //  m_hrtBuildingStart is also set at beginning of enqueuing the first IO ... we use this here to 
        //  know if an outstanding / enqueued IO is being starved.  At which point we will start draining
        //  everything currently in the m_irbtDraining to pull over the m_irbtBuilding Q, essentially degrading
        //  to something very similar to the previous IO heap model.
        const QWORD cmsec = CmsecHRTFromDhrt( HrtHRTCount() - m_hrtBuildingStart );
        if ( cmsec > g_dtickStarvedMetedOpThreshold )
        {
            return fTrue;
        }
    }

    return fFalse;
}

    

//////////////
//  I/O Heap

//  ctor/dtor

COSDisk::IOQueue::IOQueue( CCriticalSection * pcritController ) :
    CZeroInit( sizeof(*this) ),

    //  Queue lists and control
    //
    m_pcritIoQueue( pcritController ),
    m_qMetedAsyncReadIo( pcritController, COSDisk::IOQueueToo::qitRead ),
    m_qWriteIo( pcritController, COSDisk::IOQueueToo::qitWrite ),
    m_semIOQueue( CLockBasicInfo( CSyncBasicInfo( "IO Heap Semaphore Protection" ), rankOSDiskIOQueueCrit, 0 ) )
{
    Assert( m_pcritIoQueue->FNotOwner() );
}

COSDisk::IOQueue::~IOQueue()
{
    //  We can't have one and not the other.
    Assert( ( m_pIOHeapA == NULL ) == ( m_pIOHeapB == NULL ) );

    //  We can't check this unless the IO heap got up and running.
    Assert( m_pIOHeapA == NULL || m_cioreqMax == m_semIOQueue.CAvail() );

    Assert( 0 == m_cioVIPList && m_VIPListHead.FEmpty() );
    Assert( 0 == m_cioQosUrgentBackgroundCurrent );
    Assert( 0 == m_cioQosBackgroundCurrent );

    IOQueueTerm();
}


//  terminates the I/O Heap

void COSDisk::IOQueue::IOQueueTerm()
{
    Assert( 0 == m_cioVIPList );
    Assert( m_VIPListHead.FEmpty() );

    //  terminate IO heap

    _IOHeapTerm();
}

//  This initializes the Queue Quotas and IO Heap.

ERR COSDisk::IOQueue::ErrIOQueueInit( _In_ LONG cIOEnqueuedMax, _In_ LONG cIOBackgroundMax, _In_ LONG cIOUrgentBackgroundMax )
{
    ERR err = JET_errSuccess;

    Assert( cIOBackgroundMax < cIOEnqueuedMax );
    Assert( cIOUrgentBackgroundMax < cIOEnqueuedMax );

    //  init primary heap

    Call( _ErrIOHeapInit( cIOEnqueuedMax ) );

    //  init other IO quota and low queue thresholds ...

    m_cioQosBackgroundMax = cIOBackgroundMax;
    m_cioreqQOSBackgroundLow = CioBackgroundIOLow( cIOBackgroundMax );
    m_cioQosUrgentBackgroundMax = cIOUrgentBackgroundMax;

    return JET_errSuccess;

HandleError:
    IOQueueTerm();
    return err;
}

//  terminates the I/O Heap

void COSDisk::IOQueue::_IOHeapTerm()
{
    //  terminate sub-heaps

    delete m_pIOHeapA;
    m_pIOHeapA = NULL;
    delete m_pIOHeapB;
    m_pIOHeapB = NULL;
}

//  initializes the I/O Heap, or returns JET_errOutOfMemory

ERR COSDisk::IOQueue::_ErrIOHeapInit( _In_ LONG cIOEnqueuedMax )
{
    ERR err = JET_errSuccess;
    COSDisk::IOQueue::IOHeapA * pIOHeapA = NULL;
    COSDisk::IOQueue::IOHeapB * pIOHeapB = NULL;

    Call( ErrFaultInjection( 17360 ) );
    Alloc( pIOHeapA = new IOHeapA( m_pcritIoQueue ) );
    Call( ErrFaultInjection( 17364 ) );
    Alloc( pIOHeapB = new IOHeapB( m_pcritIoQueue ) );

    //  init sub-heaps

    m_cioreqMax = cIOEnqueuedMax;

    Call( pIOHeapA->ErrHeapAInit( m_cioreqMax ) );
    Call( pIOHeapB->ErrHeapBInit( pIOHeapA->rgpioreqIOAHeap, pIOHeapA->ipioreqIOAHeapMax ) );

    //  setup the queue, must not fail after this point

    m_pIOHeapA = pIOHeapA;
    m_pIOHeapB = pIOHeapB;
    pIOHeapA = NULL;    // avoid deallocation
    pIOHeapB = NULL;

    m_semIOQueue.Release( m_cioreqMax );

    //  reset current I/O stats

    fUseHeapA       = fTrue;

    iFileCurrent        = 0;
    ibOffsetCurrent = 0;

    return JET_errSuccess;

HandleError:

    //  Clean up temporary variables if we didn't complete initialization.
    delete pIOHeapA;
    delete pIOHeapB;

    return err;
}

//  returns fTrue if the I/O Heap is empty

INLINE BOOL COSDisk::IOQueue::FIOHeapEmpty()
{
    Assert( m_pcritIoQueue->FOwner() );
    return m_pIOHeapA->FHeapAEmpty() && m_pIOHeapB->FHeapBEmpty();
}

//  returns the count of IOREQs enqueued in the I/O Heap

INLINE LONG COSDisk::IOQueue::CioreqIOHeap() const
{
    Assert( m_pIOHeapA );
    Assert( m_pIOHeapB );
    return m_pIOHeapA->CioreqHeapA() + m_pIOHeapB->CioreqHeapB();
}

//  returns the count of IOREQs enqueued in the VIP List

INLINE LONG COSDisk::IOQueue::CioVIPList() const
{
    #ifdef DEBUG
    if ( !m_pcritIoQueue->FNotOwner() )
    {
        Assert( FValidVIPList() );
    }
    #endif
    //  Note: Since we only pull off one IOREQ at a time the cioreq count is the cio count.
    return m_cioVIPList;
}

//  returns the count of IOREQs enqueued in the I/O "Slace" Read Queue

INLINE LONG COSDisk::IOQueue::CioMetedReadQueue() const
{
    return m_qMetedAsyncReadIo.CioEnqueued();
}

//  returns IOREQ at the top of the I/O Heap, or NULL if empty

INLINE IOREQ* COSDisk::IOQueue::PioreqIOHeapTop()
{
    //  critical section

    Assert( m_pcritIoQueue->FOwner() );

    Assert( m_pIOHeapA );
    Assert( m_pIOHeapB );

    //  the I/O Heap A is empty

    if ( m_pIOHeapA->FHeapAEmpty() )
    {
        //  the I/O Heap B is empty

        if ( m_pIOHeapB->FHeapBEmpty() )
        {
            //  the I/O Heap is empty

            return NULL;
        }

        //  the I/O Heap B is not empty

        else
        {
            //  return the top of the I/O Heap B

            return m_pIOHeapB->PioreqHeapBTop();
        }
    }

    //  the I/O Heap A is not empty

    else
    {
        //  the I/O Heap B is empty

        if ( m_pIOHeapB->FHeapBEmpty() )
        {
            //  return the top of the I/O Heap A

            return m_pIOHeapA->PioreqHeapATop();
        }

        //  the I/O Heap B is not empty

        else
        {
            //  select the IOREQ on top of the current I/O Heap

            if ( fUseHeapA )
            {
                return m_pIOHeapA->PioreqHeapATop();
            }
            else
            {
                return m_pIOHeapB->PioreqHeapBTop();
            }
        }
    }
}

//  adds a IOREQ to the I/O Heap

INLINE void COSDisk::IOQueue::IOHeapAdd( IOREQ* pioreq, _Out_ OSDiskIoQueueManagement * const pdioqmTypeTracking )
{

    //  critical section

    Assert( pioreq );
    Assert( pioreq->m_fHasHeapReservation );    // forgot to call ErrReserveQueueSpace()
    Assert( m_pcritIoQueue->FOwner() );

    //  determine if the new IOREQ is beyond the current iFile/ibOffset

    const BOOL fForward =   CmpOSDiskIFileIbIFileIb(  pioreq->p_osf->iFile,
                                                        pioreq->ibOffset,
                                                        iFileCurrent,
                                                        ibOffsetCurrent ) > 0;

    //  if the new IOREQ is beyond the current iFile/ibOffset then put it in
    //  the active I/O Heap.  otherwise, put the IOREQ in the inactive I/O Heap.
    //  this will force us to always issue I/Os in ascending order by
    //  iFile/ibOffset

    Enforce( ( CioreqIOHeap() + 1 ) <= m_cioreqMax );

    if ( fForward && fUseHeapA || !fForward && !fUseHeapA )
    {
        m_pIOHeapA->HeapAAdd( pioreq );
        *pdioqmTypeTracking |= dioqmTypeHeapA;
        Assert( ( *pdioqmTypeTracking & mskQueueType ) == dioqmTypeHeapA );
    }
    else
    {
        m_pIOHeapB->HeapBAdd( pioreq );
        *pdioqmTypeTracking |= dioqmTypeHeapB;
        Assert( ( *pdioqmTypeTracking & mskQueueType ) == dioqmTypeHeapB );
    }
}

//  removes a IOREQ from the I/O Heap

INLINE void COSDisk::IOQueue::IOHeapRemove( IOREQ* pioreq, _Out_ OSDiskIoQueueManagement * const pdioqmTypeTracking )
{
    //  critical section

    Assert( m_pcritIoQueue->FOwner() );

    Assert( m_pIOHeapA );
    Assert( m_pIOHeapB );

    Assert( pioreq->FEnqueuedInHeap() );

    //  remove the IOREQ from whichever I/O Heap contains it

    const BOOL fRemoveFromHeapA = m_pIOHeapA->FHeapAFrom( pioreq );

    if ( fRemoveFromHeapA )
    {
        *pdioqmTypeTracking |= dioqmTypeHeapA;
        Assert( ( *pdioqmTypeTracking & mskQueueType ) == dioqmTypeHeapA || ( *pdioqmTypeTracking & mskQueueType ) == dioqmTypeHeapAandB );
        m_pIOHeapA->HeapARemove( pioreq );
    }
    else
    {
        *pdioqmTypeTracking |= dioqmTypeHeapB;
        Assert( ( *pdioqmTypeTracking & mskQueueType ) == dioqmTypeHeapB || ( *pdioqmTypeTracking & mskQueueType ) == dioqmTypeHeapAandB );
        m_pIOHeapB->HeapBRemove( pioreq );
    }

    //  set our affinity to whichever I/O Heap we just used

    fUseHeapA = fRemoveFromHeapA;

    //  remember the iFile/ibOffset of the last IOREQ removed from the I/O Heap

    iFileCurrent    = pioreq->p_osf->iFile;
    ibOffsetCurrent = pioreq->ibOffset;
}


// =============================================================================================
//  I/O Thread
// =============================================================================================
//

CTaskManager*       g_postaskmgrFile;
    
//  registers the given file for use with the I/O thread

INLINE ERR ErrOSDiskIIOThreadRegisterFile( const P_OSFILE p_osf )
{
    ERR err;

    //  attach the thread to the completion port

    Call( ErrFaultInjection( 17340 ) ); // fault inject ErrOSDiskIIOThreadRegisterFile / g_postaskmgrFile->FTMRegisterFile ...
    if ( !g_postaskmgrFile->FTMRegisterFile(  p_osf->hFile,
                                            CTaskManager::PfnCompletion( OSDiskIIOThreadIComplete ) ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    return JET_errSuccess;

HandleError:
    return err;
}

//  returns fTrue if the specified _OSFILE can use SGIO

INLINE BOOL FOSDiskIFileSGIOCapable( const _OSFILE * const p_osf )
{
    return  p_osf->fRegistered &&                                                   //  Completion ports enabled for this file
            p_osf->iomethodMost >= IOREQ::iomethodScatterGather;                    //  SGIO enabled for this file
}

//  returns fTrue if the specified IOREQ data can be processed using SGIO

INLINE BOOL FOSDiskIDataSGIOCapable( _In_ const BYTE * pbData, _In_ const DWORD cbData )
{
    return  cbData % OSMemoryPageCommitGranularity() == 0 &&                //  data is vmem page sized
            DWORD_PTR( pbData ) % OSMemoryPageCommitGranularity() == 0;     //  data is vmem page aligned
}

//  returns the start and stop offsets of an IO run / chain of IOREQs ...

INLINE void OSDiskIGetRunBound(
    const IOREQ *   pioreqRun,
    _Out_ QWORD *   pibOffsetStart,
    _Out_ QWORD *   pibOffsetEnd,
    bool            fAlwaysIncreasing = true );

INLINE void OSDiskIGetRunBound(
    const IOREQ *   pioreqRun,
    _Out_ QWORD *   pibOffsetStart,
    _Out_ QWORD *   pibOffsetEnd,
    bool            fAlwaysIncreasing )
{
    Assert( pibOffsetStart );
    Assert( pibOffsetEnd );

    Assert( pioreqRun );
    Assert( pioreqRun->cbData );
    Assert( ( pioreqRun->IbBlockMax() > pioreqRun->ibOffset ) &&
            ( pioreqRun->IbBlockMax() >= pioreqRun->cbData ) );

    //  Since we know there is 1 IOREQ, we can get this party started (w/o using -1, and 0 for fake bounds) ...
    *pibOffsetStart = pioreqRun->ibOffset;
    *pibOffsetEnd = pioreqRun->IbBlockMax();

    for ( const IOREQ * pioreqT = pioreqRun->pioreqIorunNext; pioreqT; pioreqT = pioreqT->pioreqIorunNext )
    {
        Assert( pioreqT->ibOffset );
        Assert( pioreqT->cbData );
        Assert( ( pioreqT->IbBlockMax() > pioreqT->ibOffset ) &&
                ( pioreqT->IbBlockMax() > pioreqT->cbData ) );

        if ( pioreqT->ibOffset < (*pibOffsetStart) )
        {
            //  This IOREQ is less than / before the previous one ...

            *pibOffsetStart = pioreqT->ibOffset;

            // generally speaking ESE only uses IO chains with IOs in increasing order ... so by 
            // default we will assert this is the case.
            Assert( !fAlwaysIncreasing );
        }
        if ( pioreqT->IbBlockMax() > (*pibOffsetEnd) )
        {
            //  This IOREQ is greater than / after the previous one ...

            *pibOffsetEnd = pioreqT->IbBlockMax();
        }

    }
}

INLINE bool FOSDiskIOverlappingRuns( const QWORD ibOffsetStartRun1, const QWORD ibOffsetEndRun1, const QWORD ibOffsetStartRun2, const QWORD ibOffsetEndRun2, bool * pfContiguous )
{
    if ( ibOffsetStartRun1 < ibOffsetStartRun2 )
    {
        //  Run in ascending order ... then end of run1 must be before or at begin of run2

        if ( pfContiguous )
        {
            *pfContiguous = ( ibOffsetEndRun1 == ibOffsetStartRun2 );
        }
        return ( ibOffsetEndRun1 > ibOffsetStartRun2 );
    }
    else
    {
        //  Run in descending order ... then end of run2 must be before or at begin of run1

        if ( pfContiguous )
        {
            *pfContiguous = ( ibOffsetEndRun2 == ibOffsetStartRun1 );
        }
        return ( ibOffsetEndRun2 > ibOffsetStartRun1 );
    }
}

INLINE bool FOSDiskIOverlappingRuns( const QWORD ibOffsetStartRun1, const QWORD ibOffsetEndRun1, const IOREQ * pioreqRun2 )
{
    Expected( pioreqRun2->pioreqIorunNext == NULL ); // check this will be retail fast.

    QWORD ibOffsetStartRun2, ibOffsetEndRun2;

    OSDiskIGetRunBound( pioreqRun2, &ibOffsetStartRun2, &ibOffsetEndRun2 );

    return FOSDiskIOverlappingRuns( ibOffsetStartRun1, ibOffsetEndRun1, ibOffsetStartRun2, ibOffsetEndRun2, NULL );
}

#ifdef DEBUG

//  returns true if the extends described by the two IO runs overlap ... generally this should not happen.

INLINE bool FOSDiskIOverlappingRuns( const IOREQ * pioreqRun1, const IOREQ * pioreqRun2, bool * pfContiguous = NULL );
INLINE bool FOSDiskIOverlappingRuns( const IOREQ * pioreqRun1, const IOREQ * pioreqRun2, bool * pfContiguous )
{
    QWORD ibOffsetStartRun1, ibOffsetEndRun1;
    QWORD ibOffsetStartRun2, ibOffsetEndRun2;

    OSDiskIGetRunBound( pioreqRun1, &ibOffsetStartRun1, &ibOffsetEndRun1 );
    OSDiskIGetRunBound( pioreqRun2, &ibOffsetStartRun2, &ibOffsetEndRun2 );

    return FOSDiskIOverlappingRuns( ibOffsetStartRun1, ibOffsetEndRun1, ibOffsetStartRun2, ibOffsetEndRun2, pfContiguous );
}
#endif

//  returns true if the second IO run can be appended to the first IO run without offending our max IO limits.

INLINE bool FOSDiskICanAppendRun(
    _In_ const _OSFILE *    p_osf,
    _In_ const BOOL         fWrite,
    _In_ const BOOL         fOverrideIOMax,
    _In_ const QWORD        ibOffsetRunBefore,
    _In_ const DWORD        cbDataRunBefore,
    _In_ const QWORD        ibOffsetRunAfter,
    _In_ const DWORD        cbDataRunAfter
    )
{
    Assert( cbDataRunBefore && cbDataRunAfter );
    // fOverrideIOMax(qosIOOptimizeOverrideMaxIOLimits) is only used on fWrite scenario 
    Expected( !fOverrideIOMax || fWrite );
    Assert( ibOffsetRunBefore < ibOffsetRunAfter );
    Assert( ( ibOffsetRunBefore + cbDataRunBefore ) <= ibOffsetRunAfter );  // handled by FOSDiskICanAddToRun( piorun ... ) & FOSDiskIMergeRuns

    const DWORD cbGap = fWrite ? 0 : p_osf->Pfsconfig()->CbMaxReadGapSize();  // gap MUST be 0 for write
    const DWORD cbMax = fWrite ? p_osf->Pfsconfig()->CbMaxWriteSize() : p_osf->Pfsconfig()->CbMaxReadSize();

    return ( ( ( ibOffsetRunBefore + cbDataRunBefore ) <= ibOffsetRunAfter ) &&                                     // RunBefore distinct and before RunAfter
             ( ( fOverrideIOMax || ( ibOffsetRunAfter - ibOffsetRunBefore + cbDataRunAfter ) <= cbMax ) ) &&        // total run less than max or fOverrideIOMax
             ( ( ibOffsetRunAfter - ibOffsetRunBefore - cbDataRunBefore ) <= cbGap ) );                             // gap size less than gap max
}

bool FOSDiskICompatibleRuns(
    __inout const COSDisk::IORun * const    piorun,
    _In_ const IOREQ *                      pioreq
    )
{
    Assert( piorun );
    Assert( piorun->CbRun() );
    Assert( pioreq->p_osf );

    //  Note: we do not have to check FOSDiskIFileSGIOCapable( piorun->P_OSF() ) 
    //  and FOSDiskIDataSGIOCapable( pbData, cbDataCombine ) ) because these are
    //  checked on the IO Manager's issue side to determine the iomethod for dispatching
    //  the IO run to the OS.
    if ( piorun->P_OSF() == pioreq->p_osf &&                // run in same file
        !!piorun->FWrite() == !!pioreq->fWrite )            // run for same I/O type (Read vs Write)
    {
        return fTrue;
    }

    return fFalse;
}

bool FOSDiskIMergeRuns( COSDisk::IORun * const piorunBase, COSDisk::IORun * const piorunToAdd, const BOOL fTest );

//  returns true if the specified new blocks geometries (ibOffsetCombine / cbDataCombine) can be
//  combined with the piorun without offending our max IO limits.

INLINE bool FOSDiskICanAddToRun(
    __inout COSDisk::IORun * const  piorun,
    _In_ const _OSFILE *            p_osf,
    _In_ const BOOL                 fWrite,
    _In_ const QWORD                ibOffsetCombine,
    _In_ const DWORD                cbDataCombine,
    _In_ const BOOL                 fOverrideIOMax
    )
{
    Assert( piorun );
    Assert( piorun->CbRun() );
    Assert( p_osf );
    Assert( cbDataCombine );
    // fOverrideIOMax(qosIOOptimizeOverrideMaxIOLimits) is only used on fWrite scenario 
    Expected( !fOverrideIOMax || fWrite );

    Expected( piorun->P_OSF() == p_osf );
    Expected( !!piorun->FWrite() == !!fWrite );

    //  this is just defense in depth at this point
    if ( piorun->P_OSF() != p_osf ||
        !!piorun->FWrite() != !!fWrite )
    {
        return false;
    }

    //  Well it matches on the file, same kind of op, file is SGIO capable ...
    
    if( piorun->IbOffset() == ibOffsetCombine )
    {
        // The offsets can match on read, because the ESE backup code / LGBKReadPagesCompleted() can
        // issue reads for data being read in through regular BF mechanisms / BFIAsyncReadComplete().
        // Note we can see overlapping writes ONLY from eseio iostress, which we use neg testing and
        // invalidusage to handle this.
        AssertSz( !fWrite && !piorun->FWrite() || FNegTest( fInvalidUsage ), "Concurrent write IOs for the same file+offset are not allowed!  Check the m_tidAlloc on this piorun and the second pioreq (which is probably up a stack frame)." );
    }
    else if( piorun->IbOffset() < ibOffsetCombine && ( ( ibOffsetCombine - piorun->IbOffset() ) >= piorun->CbRun() ) )
    {
        //  We can try to append this new IO run ...

        return FOSDiskICanAppendRun( p_osf, fWrite, fOverrideIOMax, piorun->IbOffset(), piorun->CbRun(), ibOffsetCombine, cbDataCombine );
    }
    else if( ibOffsetCombine < piorun->IbOffset() && ( ( piorun->IbOffset() - ibOffsetCombine ) >= cbDataCombine ) )
    {
        //  We can try to prepend this new IO run (which is just append in reverse) ...

        Assert( piorun->IbOffset() > ibOffsetCombine );
        
        return FOSDiskICanAppendRun( p_osf, fWrite, fOverrideIOMax, ibOffsetCombine, cbDataCombine, piorun->IbOffset(), piorun->CbRun() );
    }
    else
    {
        //  We can try to infix / "fill in" this IO to the IO run

        Enforce( p_osf == piorun->P_OSF() );

        IOREQ ioreqCombine;
        ioreqCombine.p_osf = (P_OSFILE)p_osf;
        ioreqCombine.fWrite = fWrite;
        ioreqCombine.pioreqIorunNext = NULL;
        ioreqCombine.ibOffset = ibOffsetCombine;
        ioreqCombine.cbData = cbDataCombine;
        ioreqCombine.m_fCanCombineIO = fTrue;
        // We are pulling in fOverrideIOMax - but it's a fair question if any other grbits should be used in
        // evaluation of combinability.
        ioreqCombine.grbitQOS = fOverrideIOMax ? qosIOOptimizeOverrideMaxIOLimits : 0;
        COSDisk::IORun iorunCombine( &ioreqCombine );
        const bool fRet = FOSDiskIMergeRuns( piorun, &iorunCombine, fTrue /* fTest only - don't combine */ );
        Assert( !piorun->FRunMember( &ioreqCombine ) );
        return fRet;
    }

    return fFalse;
}

//  simplified version of previous function for dealing w/ IOREQs

INLINE bool FOSDiskICanAddToRun(
    __inout COSDisk::IORun * const  piorun,
    _In_ const IOREQ *              pioreq
    )
{
    if ( !FOSDiskICompatibleRuns( piorun, pioreq ) )
    {
        return fFalse;
    }
    return FOSDiskICanAddToRun( piorun,
                pioreq->p_osf,
                pioreq->fWrite,
                pioreq->ibOffset,
                pioreq->cbData,
                pioreq->grbitQOS & qosIOOptimizeOverrideMaxIOLimits );
}

#ifdef DEBUG
void COSDisk::IORun::AssertValid( ) const
{

    if ( 0 == m_storage.CElements() )
    {
        //  This IO Run is empty ... this is a valid state.

        Assert( NULL == m_storage.Head() );
        Assert( NULL == m_storage.Tail() );
        Assert( 0 == m_cbRun );

    }
    else
    {

        Assert( m_storage.Head() );
        Assert( m_storage.Tail() );
        Assert( m_cbRun );

        Assert( m_storage.Head()->ibOffset <= m_storage.Tail()->ibOffset );

        if ( 1 == m_storage.CElements() )
        {
            //  Single IOREQ IO run ...

            Assert( m_storage.Head() == m_storage.Tail() );
            Assert( m_storage.Head()->pioreqIorunNext == NULL );
            Assert( m_storage.Head()->cbData == m_cbRun );

            //  check physical alignments ... 
            // N/A.
        }
        else
        {
            //  Multi-IOREQ IO run ...

            Assert( m_storage.Head() != m_storage.Tail() );

            //  check physical alignments ...
            Assert( m_storage.Head()->ibOffset < m_storage.Tail()->ibOffset );
            Assert( m_storage.Head()->IbBlockMax() <= m_storage.Tail()->ibOffset );
            Assert( m_storage.Head()->ibOffset + m_cbRun == m_storage.Tail()->IbBlockMax() );   // run size matches ioreq list
            Assert( m_storage.Head()->cbData < m_cbRun );
            Assert( m_storage.Tail()->cbData < m_cbRun );
            Assert( m_storage.Head()->pioreqIorunNext );

            //  check chain of IOREQs is consistent
            LONGLONG ibOffsetLast = -1;
            ULONG cioreq = 0;
            for ( IOREQ * pioreqT = m_storage.Head(); pioreqT; pioreqT = pioreqT->pioreqIorunNext )
            {
                Assert( ibOffsetLast < (LONGLONG)pioreqT->ibOffset );
                cioreq++;
            }
            Assert( cioreq == m_storage.CElements() );
        }

    }
}
#endif

//  Init's an IO Run with the pioreq

void COSDisk::IORun::InitRun_( IOREQ * pioreqRun )
{
    QWORD ibOffsetStart;
    QWORD ibOffsetEnd;

    Assert( m_storage.FEmpty() );
    Assert( m_cbRun == 0 );

    //   Init the run.
    m_storage.~CSimpleQueue<IOREQ>();
    new ( &m_storage ) CSimpleQueue<IOREQ>( pioreqRun, OffsetOf(IOREQ,pioreqIorunNext) );

    OSDiskIGetRunBound( pioreqRun, &ibOffsetStart, &ibOffsetEnd );
    Assert( ( ibOffsetEnd - ibOffsetStart ) == ( (DWORD)( ibOffsetEnd - ibOffsetStart ) ) );

    OnDebug( m_cbRun = (DWORD)( ibOffsetEnd - ibOffsetStart ) );
    ASSERT_VALID( this );
}

//  This takes two IORuns, piorunBase (which is where the merged IO run will go if it can happen), and piorunToAdd
//  to merge into piorunBase. 
//  Also the fTest flag controls whether it actually alters the IO chains, or just checks if the COULD be 
//  combined.

bool FOSDiskIMergeRuns( COSDisk::IORun * const piorunBase, COSDisk::IORun * const piorunToAdd, const BOOL fTest )
{
    bool fMergable = false;

    Assert( piorunBase );
    Assert( piorunToAdd );

    Assert( !piorunBase->FEmpty() );
    Assert( !piorunToAdd->FEmpty() );

    if ( piorunBase->IbOffset() == piorunToAdd->IbOffset() )
    {
        return false; // need to establish lowest to boot strap, so deal with the simplest beginning overlap ...
    }

    const BOOL fWrite = !!piorunBase->FWrite();
    const _OSFILE * p_osf = piorunBase->P_OSF();

    //  WARNING: Note that PioreqGetRun() is destructive to the IORun, removing the IOREQ chain from
    //  the IORun structure... and FURTHER since we pass the TLS's IORun into here, it means that we
    //  have temporarily seized / taken the IORun from the TLS / ioreqInIOCombiningList state for our
    //  own TLS and thus we have a duty to restore it (or we'd lose the IO).  So that is why const is 
    //  critical ... as it is used to restore the two IORuns when run under fTest.
    //  Note: I(SOMEONE) thought we _had_ to pull the IOREQ chain out of the IO run because I thought
    //  there was a code path that stole IOREQ chains when we were low on IOREQs, but it seems like we
    //  only do that for our own thread from the above.
    IOREQ * const /* critical const */ pioreqBaseHead = piorunBase->PioreqGetRun();
    IOREQ * const /* critical const */ pioreqToAddHead = piorunToAdd->PioreqGetRun();
    //  NOTE: THIS action is destructive, so have to re-init the IORuns below.
    Assert( piorunBase->FEmpty() && piorunToAdd->FEmpty() );
    Assert( pioreqBaseHead );
    Assert( pioreqToAddHead );

    const BOOL fOverrideIOMax = !!( pioreqToAddHead->grbitQOS & qosIOOptimizeOverrideMaxIOLimits );

    //
    //  Boot strap our new IO run head
    //

    IOREQ * pioreqNewHeadFirst = NULL;
    IOREQ * pioreqNewTailLast = NULL;

    IOREQ * pioreqBaseMid = pioreqBaseHead;
    IOREQ * pioreqToAddMid = pioreqToAddHead;
    OnDebug( LONG cioreqBase = 0 );
    OnDebug( LONG cioreqToAdd = 0 );

    //  update our new IO run's head & tail markers and consume first IOREQ off which
    //  ever IO run starts at lower offset.  Also get would be final/merged size.

    Enforce( pioreqBaseMid->ibOffset != pioreqToAddMid->ibOffset ); // checked above.
    if ( CmpIOREQOffset( pioreqBaseMid, pioreqToAddMid ) < 0 )
    {
        pioreqNewHeadFirst = pioreqBaseMid;
        pioreqBaseMid = pioreqBaseMid->pioreqIorunNext;
        OnDebug( cioreqBase++ );
    }
    else
    {
        //  The to ToAdd IO run is actually starting lower than the base IO run.
        pioreqNewHeadFirst = pioreqToAddMid;
        pioreqToAddMid = pioreqToAddMid->pioreqIorunNext;
        OnDebug( cioreqToAdd++ );

        //  Also, we must move the heap reservation to the head of the list ...
        //
        if ( !pioreqToAddHead->m_fHasHeapReservation )
        {
            Assert( pioreqToAddHead->m_fCanCombineIO );
            Assert( pioreqBaseHead->m_fHasHeapReservation );
            if ( !fTest )
            {
                pioreqToAddHead->m_fHasHeapReservation = pioreqBaseHead->m_fHasHeapReservation;
                //  Note: We don't transfer m_fHasBackgroundReservation or urgent background because
                //  this is rare case and these counts only need be approx accurate.
                pioreqBaseHead->m_fHasHeapReservation = fFalse;
                pioreqBaseHead->m_fCanCombineIO = pioreqToAddHead->m_fCanCombineIO;
            }
        }
    }
    pioreqNewTailLast = pioreqNewHeadFirst;

    //
    //  Now walk both IO runs by "lowest IOREQ", accumulating new IO run, until both are exhausted ... 
    //

    LONG cioreqTotal = 1;
    while( pioreqBaseMid != NULL || pioreqToAddMid != NULL )
    {
        //  Select next lowest IOREQ from our two input IO runs, pull it off (i.e. increment
        //  to next for that IO run)...

        IOREQ * pioreqLowest = NULL;

        // The offsets can match on read, because the ESE backup code / LGBKReadPagesCompleted() can
        // issue reads for data being read in through regular BF mechanisms / BFIAsyncReadComplete()
        // or DBScan.

        if ( pioreqBaseMid && pioreqToAddMid &&
                CmpIOREQOffset( pioreqBaseMid, pioreqToAddMid ) == 0 )
        {
            //  Note: while we support filling in a gap, we (or OS Scatter/Gather IO) cannot support
            //  actual blocks / IOREQs having overlapping offsets.  So we say these IOs are not
            //  combinable ... at least not without a whole bunch of extra trickiness at OS layer.
            //  This is super rare (such as DBScan or Backup or BF read on same offset, so we don't
            //  care to implement it (esp. since HD/device cache will probably take care of it).
            Assert( !fMergable );
            Assert( !fWrite || FNegTest( fInvalidUsage ) /* eseio iostress doesn't control it's writes */ );
            Assert( fTest );
            goto ReconstructIoRuns;
        }
        else if ( pioreqToAddMid == NULL || ( pioreqBaseMid != NULL && CmpIOREQOffset( pioreqBaseMid, pioreqToAddMid ) < 0 ) )
        {
            Assert( pioreqBaseMid != NULL );

            pioreqLowest = pioreqBaseMid;
            pioreqBaseMid = pioreqBaseMid->pioreqIorunNext;
            OnDebug( cioreqBase++ );
        }
        else if ( pioreqBaseMid == NULL || ( pioreqToAddMid != NULL /* superflous */ && CmpIOREQOffset( pioreqBaseMid, pioreqToAddMid ) > 0 ) )
        {
            Assert( pioreqToAddMid != NULL );
    
            pioreqLowest = pioreqToAddMid;
            pioreqToAddMid = pioreqToAddMid->pioreqIorunNext;
            OnDebug( cioreqToAdd++ );
        }
        else
        {
            AssertSz( fFalse, "Should have either pioreqBaseMid or pioreqToAddMid not-NULL or one larger or smaller than other" );
        }

        OnDebug( cioreqTotal++ );
        Assert( fWrite == !!pioreqLowest->fWrite );
        Assert( p_osf == pioreqLowest->p_osf );

        //  NOTE: SHOULD NOT reference pioreqBaseMid or pioreqToAddMid after this point as 
        //  they are already incremented to next IOREQ in thier chain ... pioreqLowest is 
        //  the IOREQ of interest.

        //  Check if can merge "lowest" without overlap into the new run ...

        if ( pioreqNewTailLast->IbBlockMax() > pioreqLowest->ibOffset )
        {
            //  Oh noes!  There is an overlap ...

            Assert( !fWrite || FNegTest( fInvalidUsage ) /* eseio iostress doesn't control it's writes */ );
            Assert( !fMergable );
            Assert( fTest );
            goto ReconstructIoRuns;
        }

        //  Check if the merge would offend IO limits ...

        const QWORD cbNewCurr = pioreqNewTailLast->IbBlockMax() - pioreqNewHeadFirst->ibOffset;
        if ( cbNewCurr >= (QWORD)dwMax ||
            !FOSDiskICanAppendRun( p_osf, fWrite, fOverrideIOMax, pioreqNewHeadFirst->ibOffset, (DWORD)cbNewCurr, pioreqLowest->ibOffset, pioreqLowest->cbData ) )
        {
            //  Too big (or too far a gap), too bad!
            Assert( !fMergable );
            Assert( fTest );
            goto ReconstructIoRuns;
        }

        //  Merge IO

        if ( !fTest )
        {
            pioreqNewTailLast->pioreqIorunNext = pioreqLowest;
        }
        pioreqNewTailLast = pioreqLowest;
        if ( !fTest )
        {
            //  Note pioreqBaseMid & ToAddMid are moved to pioreqIorunNext above (so this is safe).
            pioreqNewTailLast->pioreqIorunNext = NULL;
        }
    }

    Assert( pioreqBaseMid == NULL );
    Assert( pioreqToAddMid == NULL );
    OnDebug( Assert( cioreqTotal == cioreqBase + cioreqToAdd ) );
    fMergable = true;

ReconstructIoRuns:

    if ( fTest || !fMergable )
    {
        Assert( fTest );
        piorunBase->FAddToRun( pioreqBaseHead );
        piorunToAdd->FAddToRun( pioreqToAddHead );
    }
    else
    {
        piorunBase->FAddToRun( pioreqNewHeadFirst );
        Assert( piorunToAdd->FEmpty() );
    }

    return fMergable;
}

bool COSDisk::IORun::FAddToRun( IOREQ * pioreq )
{
    ASSERT_VALID( this );

    Assert( pioreq );
    Assert( pioreq->cbData );

    //  Note: The meted Q now attemps to merge ioruns in foreground, so p_osf file checks here are crucial.

    //  this means we're not only adding an IOREQ, we're adding a whole IO run to this existing run.

    const bool bAddingRun = NULL != pioreq->pioreqIorunNext;

    if ( m_storage.FEmpty() )
    {

        Assert( 0 == m_cbRun );
        Assert( NULL == m_storage.Head() && NULL == m_storage.Tail() );

        if ( bAddingRun )
        {
            //  We are adding a full pioreqIorunNext-based IO run to an empty IO run object
            InitRun_( pioreq );
            Assert( m_cbRun > 0 );  // m_cbRun set by InitRun_()
        }
        else
        {
            m_storage.InsertAsPrevMost( pioreq, OffsetOf( IOREQ, pioreqIorunNext ) );
            OnDebug( m_cbRun = pioreq->cbData );
        }
        ASSERT_VALID( this );
    }
    else
    {
        //  Checks same file and IO type (read vs. write IO)

        if ( !FOSDiskICompatibleRuns( this, pioreq ) )
        {
            ASSERT_VALID( this );
            return false;
        }
        
        Assert( PioreqHead()->p_osf == pioreq->p_osf );

        if ( !bAddingRun && !FOSDiskIOverlappingRuns( this->IbOffset(), this->IbRunMax(), pioreq ) )
        {
            //  Fast path, strictly appending/prepending singleton (most of the time, but not always) ...

            if ( FOSDiskICanAddToRun( this, pioreq ) )
            {
                Assert( !FOSDiskIOverlappingRuns( m_storage.Head(), pioreq ) );

                if ( IbRunMax() <= pioreq->ibOffset )
                {
                    //  The added IOREQ is after IO run, append this IOREQ ...
                    
                    OnDebug( m_cbRun += (DWORD)( ( pioreq->ibOffset + pioreq->cbData ) - ( IbOffset() + CbRun() ) ) );
                    m_storage.InsertAsNextMost( pioreq, OffsetOf(IOREQ,pioreqIorunNext ) );

                    ASSERT_VALID( this );
                }
                else if ( pioreq->IbBlockMax() <= IbOffset() )
                {
                    //  The added IOREQ is preceeding IO run, prepend this IOREQ ...

                    //  First, we must move the heap reservation to the head of the list ...
                    //
                    if ( !pioreq->m_fHasHeapReservation )
                    {
                        Assert( pioreq->m_fCanCombineIO );
                        Assert( m_storage.Head()->m_fHasHeapReservation );
                        //  Note: We don't transfer m_fHasBackgroundReservation or urgent background because
                        //  this is rare case and these counts only need be approx accurate.
                        pioreq->m_fHasHeapReservation = m_storage.Head()->m_fHasHeapReservation;
                        m_storage.Head()->m_fHasHeapReservation = fFalse;
                        m_storage.Head()->m_fCanCombineIO = pioreq->m_fCanCombineIO;
                    }
                    
                    OnDebug( m_cbRun += (DWORD)( IbOffset() - pioreq->ibOffset ) );
                    m_storage.InsertAsPrevMost( pioreq, OffsetOf(IOREQ,pioreqIorunNext) );

                    ASSERT_VALID( this );
                }
                else
                {
                    // There is overlap!!!  Something has gone horribly wrong.
                    AssertSz( fFalse, "There is IO overlap in the attempt to add an IO" );
                    ASSERT_VALID( this );
                    return false;
                }
            }
            else
            {
                //  simply either would make IO too big, or even just starts too far away from this to gap.
                ASSERT_VALID( this );
                return false;
            }
        }
        else
        {
            COSDisk::IORun iorunMerging( pioreq );

            if ( !FOSDiskIMergeRuns( this, &iorunMerging, fTrue ) )
            {
                //  simply either would make IO too big, or even just starts too far away from this to gap.
                ASSERT_VALID( this );
                return false;
            }

            const BOOL fRet = FOSDiskIMergeRuns( this, &iorunMerging, fFalse );
            Assert( fRet );
            Assert( iorunMerging.FEmpty() );
            Assert( !FEmpty() );
            ASSERT_VALID( this );
        }
    }
    Assert( !m_storage.FEmpty() );

    ASSERT_VALID( this );
    return true;
}

//  returns the size of the IORun (from beginning of head IOREQ to end of tail IOREQ)

ULONG COSDisk::IORun::CbRun() const
{
    ASSERT_VALID( this );
    if ( FEmpty() )
    {
        return 0; // obviously
    }
    
    const QWORD cbRunT = m_storage.Tail()->IbBlockMax() - m_storage.Head()->ibOffset;
    Assert( m_cbRun == cbRunT );

    Enforce( cbRunT < ulMax );
    return (ULONG)cbRunT;
}

#ifdef DEBUG

//  Checks if the IOREQ specified is in this IORun's pioreqIorunNext chain

BOOL COSDisk::IORun::FRunMember( const IOREQ * pioreqCheck ) const
{
    for ( IOREQ * pioreqT = m_storage.Head(); pioreqT; pioreqT = pioreqT->pioreqIorunNext )
    {
        if ( pioreqT == pioreqCheck )
        {
            return fTrue;
        }
    }
    return fFalse;
}

#endif // DEBUG

//  sets the IOREQType on all IOREQs in the I/O run

void COSDisk::IORun::SetIOREQType( const IOREQ::IOREQTYPE ioreqtypeNext )
{
    for ( IOREQ * pioreqT = m_storage.Head(); pioreqT; pioreqT = pioreqT->pioreqIorunNext )
    {
        // not really right place to assert this, but gets the job done, we shouldn't
        // be adding things to the io heap or moving them to any other state w/ this
        // set, it's only supposed to be set during IO.
        Assert( pioreqT->m_posdCurrentIO == NULL );
        pioreqT->SetIOREQType( ioreqtypeNext );
    }
}

//  destructively retrieves the head IOREQ of the I/O run

IOREQ * COSDisk::IORun::PioreqGetRun( )
{
    OnDebug( m_cbRun = 0 );
    return m_storage.RemoveList();
}


//
//  Queue Op support
//

#ifdef DEBUG

//  determines if the basic QueueOp struct is self-consistent

bool COSDisk::QueueOp::FValid( ) const
{
    switch( m_eQueueOp )
    {
        case eUnknown:
            //   unknown ok, as long as we're empty
            if ( NULL != m_pioreq ||
                    !m_iorun.FEmpty() )
            {
                return false;
            }
            break;
        case eSpecialOperation:
            if ( !m_iorun.FEmpty() ||
                    NULL == m_pioreq )
            {
                return false;
            }
            break;
        case eIOOperation:
            if ( m_pioreq ||
                    m_iorun.FEmpty() )
            {
                return false;
            }
            break;
    }
    return true;
}
#endif

bool COSDisk::QueueOp::FEmpty() const
{
    Assert( FValid() );
    switch( m_eQueueOp )
    {
        case eUnknown:
            return true;
        case eSpecialOperation:
            return NULL == m_pioreq;
        case eIOOperation:
            return m_iorun.FEmpty();
    }
    AssertSz( fFalse, "Unknown Queue Operation Type!" );
    return false;
}

BOOL COSDisk::QueueOp::FIssuedFromQueue() const
{
    Assert( FValid() );
    Assert( !FEmpty() );
    const BOOL fRet = m_pioreq ?
                        // Note: I'm not 100% sure using this is right for the first part.
                        m_pioreq->FInIssuedState() :
                        m_iorun.FIssuedFromQueue();
#ifndef RTM
    if ( NULL == m_pioreq )
    {
        //  we would not expect removed from queue here ... and if we got it, we
        //  probably need to update stuff that relies on this code to know if we
        //  need to decrement m_cioDispatching and reset m_posdCurrentIO.
        AssertRTL( m_iorun.PioreqHead() && !m_iorun.PioreqHead()->FRemovedFromQueue() );
    }
    else
    {
        Assert( !fRet );
    }
#endif
    return fRet;
}
BOOL COSDisk::QueueOp::FHasHeapReservation() const
{
    Assert( FValid() );
    Assert( !FEmpty() );
    return m_pioreq ?
                m_pioreq->m_fHasHeapReservation :
                m_iorun.FHasHeapReservation();
}

ULONG COSDisk::QueueOp::Cioreq( ) const
{
    Assert( FValid() );
    switch( m_eQueueOp )
    {
        case eSpecialOperation:
            Assert( m_pioreq->pioreqIorunNext == NULL );
            Expected( m_pioreq->pioreqVipList == NULL ); // sort of a presumptive to check VIP list here.
            return 1;
        case eIOOperation:
            return m_iorun.Cioreq();
        case eUnknown:
        default:
            AssertSz( fFalse, "Unexpected call to Cioreq() on empty QueueOp" );
    }
    return 0;
}

P_OSFILE COSDisk::QueueOp::P_osfPerfctrUpdates() const
{
    Assert( FValid() );
    switch( m_eQueueOp )
    {
        case eSpecialOperation:
            Assert( m_pioreq->pioreqIorunNext == NULL );
            Expected( m_pioreq->pioreqVipList == NULL ); // sort of a presumptive to check VIP list here.
            return NULL;
        case eIOOperation:
            return m_iorun.P_OSF();
        case eUnknown:
        default:
            AssertSz( fFalse, "Unexpected call to Cioreq() on empty QueueOp" );
    }
    return 0;
}


#ifdef DEBUG

BOOL COSDisk::QueueOp::FRunMember( const IOREQ * pioreqCheck ) const
{
    if ( m_pioreq )
    {
        Assert( m_eQueueOp == eSpecialOperation );
        return m_pioreq == pioreqCheck;
    }
    // else
    return m_iorun.FRunMember( pioreqCheck );
}

#endif // DEBUG

DWORD COSDisk::QueueOp::CbRun( ) const
{
    Assert( FValid() );
    Assert( m_eQueueOp == eSpecialOperation || m_eQueueOp == eIOOperation );
    return ( m_eQueueOp == eSpecialOperation ) ? m_pioreq->cbData : m_iorun.CbRun();
}


bool COSDisk::QueueOp::FAddToRun( IOREQ * pioreq )
{
    bool fRet = false;

    Assert( pioreq );

    if ( eUnknown == m_eQueueOp )
    {
        //  Nothing has been set / decided yet, make determination based upon passed in pioreq ...

        if ( 0 == pioreq->cbData )
        {
            m_eQueueOp = eSpecialOperation;
        }
        else
        {
            m_eQueueOp = eIOOperation;
        }
    }

    switch( m_eQueueOp )
    {
        case eSpecialOperation:

            //  Attempt to add special op if it is not a regular IO op
            if ( 0 == pioreq->cbData )
            {
                //  We do not support chaining of special ops, so only accept if we are an 
                //  empty QueueOp.

                Assert( NULL == pioreq->pioreqIorunNext );
                if ( NULL == m_pioreq )
                {
                    m_pioreq = pioreq;
                    fRet = true;
                }
            }
            // else fRet = false
            break;

        case eIOOperation:

            //  Attempt to add IOREQ[Chain/Run] if it is not a special op

            if ( pioreq->cbData )
            {
                OnDebug( bool fEmptyPre = m_iorun.FEmpty() );
                fRet = m_iorun.FAddToRun( pioreq );
                Assert( fRet || !fEmptyPre );
            }
            else
            {
                Assert( !m_iorun.FEmpty() );
            }
            break;

        default:
            AssertSz( fFalse, "Unknown Queue Operation Type!!!" );
    }

    return fRet;
}

//  sets the IOREQType on the IOREQs involved in this Op

void COSDisk::QueueOp::SetIOREQType( const IOREQ::IOREQTYPE ioreqtypeNext )
{
    //  We used this for things we remove from the queue and for
    //  things we put into the queue / IO Heap.
    Expected( IOREQ::ioreqRemovedFromQueue == ioreqtypeNext ||
                IOREQ::ioreqEnqueuedInMetedQ == ioreqtypeNext ||
                IOREQ::ioreqEnqueuedInIoHeap == ioreqtypeNext );
    return m_pioreq ?
                m_pioreq->SetIOREQType( ioreqtypeNext ) :
                m_iorun.SetIOREQType( ioreqtypeNext );
}

IOREQ * COSDisk::QueueOp::PioreqOp() const
{
    Assert( FValid() );
    return ( eSpecialOperation == m_eQueueOp ) ? m_pioreq : NULL;
}
COSDisk::IORun * COSDisk::QueueOp::PiorunIO()
{
    Assert( FValid() );
    return ( eIOOperation == m_eQueueOp ) ? (&m_iorun) : NULL;
}

IOREQ * COSDisk::QueueOp::PioreqGetRun()
{
    Assert( FValid() );
    Assert( !FEmpty() );

    IOREQ * pioreqT = NULL;
    if ( m_pioreq )
    {
        Assert( eSpecialOperation == m_eQueueOp );
        pioreqT =  m_pioreq;
        m_pioreq = NULL;
    }
    else if ( !m_iorun.FEmpty() )
    {
        Assert( eIOOperation == m_eQueueOp );
        pioreqT = m_iorun.PioreqGetRun();   // destructive
    }
    else
    {
        AssertSz( fFalse, "Unknown EType()!" );
    }
    m_eQueueOp = eUnknown;
    Assert( FEmpty() && NULL == m_pioreq && m_iorun.FEmpty() );
    return pioreqT;
}
    
//  This converts a "urgent level" (see qosIODispatchUrgentBackground* arguments) to a QOS for 
//  consumption by the API.

OSFILEQOS QosOSFileFromUrgentLevel( _In_ const ULONG iUrgentLevel )
{
    Assert( iUrgentLevel >= 1 );
    Assert( iUrgentLevel <= qosIODispatchUrgentBackgroundLevelMax );

    DWORD grbitQOS = ( iUrgentLevel << qosIODispatchUrgentBackgroundShft );

    Assert( qosIODispatchUrgentBackgroundMask & grbitQOS ); // a urgent bit / level is set
    Assert( 0 == ( ~qosIODispatchUrgentBackgroundMask & grbitQOS ) );   // should set no other bits...

    return grbitQOS;
}

//  This is the opposite of QosOSFileFromUrgentLevel(), it converts a QOS to an "urgent level".

ULONG IOSDiskIUrgentLevelFromQOS( _In_ const OSFILEQOS grbitQOS )
{
    Assert( qosIODispatchUrgentBackgroundMask & grbitQOS ); // a urgent bit / level is set
    Assert( 0 == ( ( ~qosIODispatchUrgentBackgroundMask & grbitQOS ) & qosIODispatchMask ) );   // no other dispatch bits should be set

    ULONG   iUrgentLevel = ( grbitQOS & qosIODispatchUrgentBackgroundMask ) >> qosIODispatchUrgentBackgroundShft;

    Assert( iUrgentLevel >= 1 );
    Assert( iUrgentLevel <= qosIODispatchUrgentBackgroundLevelMax );

    return iUrgentLevel;
}

//  This takes an urgent level and turns this into a max outstanding IO target we should hit for that urgency.  This
//  function is different from it's predecesors in that it starts out very gentle, rising only an outstanding IO or 
//  two or three for a while, and then starts ramping up more powerfully as the urgent level increases.

LONG CioOSDiskIFromUrgentLevelSmoothish( _In_ const ULONG iUrgentLevel, _In_ const DWORD cioUrgentMaxMax )
{
    Assert( iUrgentLevel >= 1 );
    Assert( iUrgentLevel <= qosIODispatchUrgentBackgroundLevelMax );

    ULONG cioOutstanding = 0;

    const ULONG iRamp1End = ( qosIODispatchUrgentBackgroundLevelMax / 8 );
    const ULONG iRamp1Len = iRamp1End;
    const ULONG cioRamp1Max = UlBound( cioUrgentMaxMax / 128, 1, cioUrgentMaxMax );

    const ULONG iRamp2End = ( qosIODispatchUrgentBackgroundLevelMax / 4 );
    const ULONG iRamp2Len = iRamp2End - iRamp1End;
    const ULONG cioRamp2Max = ( ( cioUrgentMaxMax / 32 ) > ( cioRamp1Max ) ) ?
                                UlBound( cioUrgentMaxMax / 32 - cioRamp1Max, 1, cioUrgentMaxMax ) :
                                1;

    const ULONG iRamp3End = ( qosIODispatchUrgentBackgroundLevelMax / 2 );
    const ULONG iRamp3Len = iRamp3End - iRamp2End;
    const ULONG cioRamp3Max = ( ( cioUrgentMaxMax / 4 ) > ( cioRamp1Max + cioRamp2Max ) ) ?
                                UlBound( cioUrgentMaxMax / 4 - ( cioRamp1Max + cioRamp2Max ), 1, cioUrgentMaxMax ) :
                                1;

    const ULONG iRamp4End = ( qosIODispatchUrgentBackgroundLevelMax / 1 );
    const ULONG iRamp4Len = iRamp4End - iRamp3End;
    const ULONG cioRamp4Max = cioUrgentMaxMax - ( cioRamp1Max + cioRamp2Max + cioRamp3Max );

    Assert( iRamp1End <= iRamp2End );
    Assert( iRamp2End <= iRamp3End );
    Assert( iRamp3End <= iRamp4End );

    Assert( cioRamp1Max );
    Assert( cioRamp2Max );
    Assert( cioRamp3Max );
    Assert( cioRamp4Max );

    Assert( cioRamp1Max <= cioUrgentMaxMax );
    Assert( cioRamp2Max <= cioUrgentMaxMax );
    Assert( cioRamp3Max <= cioUrgentMaxMax );
    Assert( cioRamp4Max <= cioUrgentMaxMax );

    Assert( ( cioRamp1Max + cioRamp2Max + cioRamp3Max + cioRamp4Max ) == cioUrgentMaxMax );

    if ( iUrgentLevel <= iRamp1End )
    {
        cioOutstanding = UlBound( iUrgentLevel * cioRamp1Max / iRamp1Len, 1, cioRamp1Max );
    }
    else if ( iUrgentLevel <= iRamp2End )
    {
        const ULONG iRelativeUrgency = iUrgentLevel - iRamp1End;
        Assert( iRelativeUrgency );
        cioOutstanding = UlBound( iRelativeUrgency * cioRamp2Max / iRamp2Len, 1, cioRamp2Max );
        cioOutstanding += ( cioRamp1Max );
    }
    else if ( iUrgentLevel <= iRamp3End )
    {
        const ULONG iRelativeUrgency = iUrgentLevel - iRamp2End;
        Assert( iRelativeUrgency );
        cioOutstanding = UlBound( iRelativeUrgency * cioRamp3Max / iRamp3Len, 1, cioRamp3Max );
        cioOutstanding += ( cioRamp1Max + cioRamp2Max );
    }
    else
    {
        const ULONG iRelativeUrgency = iUrgentLevel - iRamp3End;
        Assert( iRelativeUrgency );
        cioOutstanding = UlBound( iRelativeUrgency * cioRamp4Max / iRamp4Len, 1, cioRamp4Max );
        cioOutstanding += ( cioRamp1Max + cioRamp2Max + cioRamp3Max );
    }

    Assert( cioOutstanding >= 1 );
    Assert( cioOutstanding <= cioUrgentMaxMax );

    return cioOutstanding;
}

LONG CioOSDiskIFromUrgentLevelBiLinear( _In_ const ULONG iUrgentLevel, _In_ const DWORD cioUrgentMaxMax )
{
    Assert( iUrgentLevel >= 1 );
    Assert( iUrgentLevel <= qosIODispatchUrgentBackgroundLevelMax );

    ULONG cioOutstanding = 0;

    const ULONG iRamp1 = ( qosIODispatchUrgentBackgroundLevelMax / 4 );
    const ULONG cioRamp1Max = UlBound( cioUrgentMaxMax / 64, 1, cioUrgentMaxMax );

    const ULONG iRamp2 = ( qosIODispatchUrgentBackgroundLevelMax / 1 ) - iRamp1;
    const ULONG cioRamp2Max = cioUrgentMaxMax - cioRamp1Max;

    Assert( iRamp1 < iRamp2 );
    Assert( cioRamp1Max < cioUrgentMaxMax );
    Assert( cioRamp2Max < cioUrgentMaxMax );
    Assert( ( cioRamp1Max + cioRamp2Max ) == cioUrgentMaxMax );

    if ( iUrgentLevel < ( qosIODispatchUrgentBackgroundLevelMax / 4 ) )
    {
        cioOutstanding = UlBound( iUrgentLevel * cioRamp1Max / iRamp1, 1, cioRamp1Max );
    }
    else
    {
        const ULONG iRelativeUrgency = iUrgentLevel - iRamp1;
        cioOutstanding = UlBound( cioRamp1Max + ( iRelativeUrgency * cioRamp2Max / iRamp2 ), 1, cioRamp1Max + cioRamp2Max );
    }

    Assert( cioOutstanding >= 1 );
    Assert( cioOutstanding <= cioUrgentMaxMax );

    return cioOutstanding;
}


LONG CioOSDiskIFromUrgentLevelLinear( _In_ const ULONG iUrgentLevel, _In_ const DWORD cioUrgentMaxMax )
{
    Assert( iUrgentLevel >= 1 );
    Assert( iUrgentLevel <= qosIODispatchUrgentBackgroundLevelMax );

    ULONG cioMax = (LONG) iUrgentLevel * cioUrgentMaxMax / qosIODispatchUrgentBackgroundLevelMax;

    cioMax = UlBound( cioMax, 1, cioUrgentMaxMax );

    Assert( cioMax >= 1 );
    Assert( cioMax <= cioUrgentMaxMax );

    return (LONG)cioMax;
}

LONG CioOSDiskIFromUrgentLevel( _In_ const ULONG iUrgentLevel, _In_ const DWORD cioUrgentMaxMax )
{
    // Evolved this over time... keeping old versions...
    //const ULONG cioMax = CioOSDiskIFromUrgentLevelLinear( iUrgentLevel, cioUrgentMaxMax );
    //const ULONG cioMax = CioOSDiskIFromUrgentLevelBiLinear( iUrgentLevel, cioUrgentMaxMax );
    const ULONG cioMax = CioOSDiskIFromUrgentLevelSmoothish( iUrgentLevel, cioUrgentMaxMax );

    Assert( cioMax >= 1 );
    Assert( cioMax <= cioUrgentMaxMax );

    return (LONG)cioMax;
}

LONG CioOSDiskIFromUrgentQOS( _In_ const OSFILEQOS grbitQOS, _In_ const DWORD cioUrgentMaxMax )
{
    const ULONG iUrgentLevel = IOSDiskIUrgentLevelFromQOS( grbitQOS );
    return CioOSDiskIFromUrgentLevel( iUrgentLevel, cioUrgentMaxMax );
}

LONG CioOSDiskPerfCounterIOMaxFromUrgentQOS( _In_ IFileSystemConfiguration* const pfsconfig, _In_ const OSFILEQOS grbitQOS )
{
    return CioOSDiskIFromUrgentQOS( grbitQOS, CioDefaultUrgentOutstandingIOMax( pfsconfig->CIOMaxOutstanding() ) );
}

LONG CusecOSDiskSmallDuration( HRT dhrtDuration )
{
    QWORD cusecRaw = CusecHRTFromDhrt( dhrtDuration );
    if ( cusecRaw >= lMax )
    {
        if ( cusecRaw <= ulMax )
        {
            FireWall( "SmallDurationNotSoSmallTruncation" );
        }
        else
        {
            FireWall( "SmallDurationLargerThan35MinTruncation" );
        }
        return lMax;
    }
    return (LONG)cusecRaw;
}

ULONG CbSumRun( const IOREQ * pioreq )
{
    ULONG cbRun = 0;
    const IOREQ * pioreqPrev = NULL;
    while( pioreq )
    {
        cbRun += pioreq->cbData;
        if ( pioreqPrev && ( pioreqPrev->IbBlockMax() != pioreq->ibOffset ) )
        {
            // add any gaps
            Expected( !pioreq->fWrite );
            cbRun += ULONG( pioreq->ibOffset - pioreqPrev->IbBlockMax() );
        }
        pioreqPrev = pioreq;
        pioreq = pioreq->pioreqIorunNext;
    }
    return cbRun;
}


ERR COSDisk::IOQueue::ErrReserveQueueSpace( _In_ OSFILEQOS grbitQOS, __inout IOREQ * pioreq )
{
    ERR err = JET_errSuccess;

    //  First we try to acquire the right to the spurious / lower quotas
    //

    DWORD cioreqT;
    switch ( qosIODispatchMask & grbitQOS )
    {

        case qosIODispatchBackground:

            if ( pioreq->fWrite )
            {
                Assert( !( qosIOPoolReserved & grbitQOS ) );
                if ( !FAtomicIncrementMax( (DWORD*)&m_cioQosBackgroundCurrent, &cioreqT, m_cioQosBackgroundMax ) )
                {
                    Assert( m_cioQosBackgroundCurrent <= m_cioQosBackgroundMax );
                    Error( ErrERRCheck( errDiskTilt ) );
                }

                //  success, mark IOREQ as using background quota

                pioreq->m_fHasBackgroundReservation = fTrue;
            }
            else
            {
                //  For a background read, we pass it onto the Meted Q, and so it does not need any other
                //  reservations.  Oh and we don't push back to the ErrIORead() site and reject operations
                //  like we do for write ops.
            }
            break;

        case qosIODispatchImmediate:
            break;

        case qosIODispatchIdle:
        //case qosIODispatchUrgentBackground:   - effectively -
        default:

            Assert( pioreq->fWrite );
            if ( qosIODispatchUrgentBackgroundMask & grbitQOS ) // case qosIODispatchUrgentBackground:  - effectively -
            {

                //  Extrapolate the urgent-ness of it...
                LONG    cioreqUrgentMax = CioOSDiskIFromUrgentQOS( grbitQOS, m_cioQosUrgentBackgroundMax );
                Assert( cioreqUrgentMax <= m_cioQosUrgentBackgroundMax );

                Assert( !( qosIOPoolReserved & grbitQOS ) );
                if ( !FAtomicIncrementMax( (DWORD*)&m_cioQosUrgentBackgroundCurrent, &cioreqT, cioreqUrgentMax ) )
                {
                    Assert( m_cioQosUrgentBackgroundCurrent <= m_cioQosUrgentBackgroundMax );
                    Error( ErrERRCheck( errDiskTilt ) );
                }
                
                //  success, mark IOREQ as using urgent background quota

                pioreq->m_fHasUrgentBackgroundReservation = fTrue;

            }
            else
            {
                AssertSz( fFalse, "Unknown IO Dispatch QOS!" );
            }

            break;
    }

    //  We even reserve IO heap space here _even_ for sync IO because on out of memory 
    //  condition, sync IO enqueues the IO in the IO queue / heap.
    //  As well we also effectively protect our quota mechanism of max outstanding
    //  IOs to the disk.

    if ( !m_semIOQueue.FTryAcquire() )
    {

        COSDisk::EnqueueDeferredIORun( NULL );
        OSDiskIOThreadStartIssue( NULL );

        // Should we wait for qosIODispatchBackground and qosIODispatchUrgentBackground

        //  If next alloc is guaranteed or we're requesting from the reserved pool, we can't wait.
        if ( qosIOPoolReserved & grbitQOS )
        {
            //  We can't wait on this semaphore or we might deadlock ...
            Assert( qosIODispatchImmediate == ( qosIODispatchMask & grbitQOS ) );
            pioreq->m_fHasHeapReservation = fFalse;
            err = JET_errSuccess;
        }
        else
        {

            m_semIOQueue.Acquire();
            pioreq->m_fHasHeapReservation = fTrue;
            err = JET_errSuccess;
        }
    }
    else
    {
        pioreq->m_fHasHeapReservation = fTrue;
        err = JET_errSuccess;
    }

    return err;
    
HandleError:

    return err;
}

__int64 COSDisk::IOQueue::IpassBeginDispatchPass()
{
    Assert( FIOThread() );
    m_cDispatchPass++;
    return m_cDispatchPass;
}

__int64 COSDisk::IOQueue::IpassContinuingDispatchPass()
{
    Assert( FIOThread() );
    m_cDispatchContinues++;
    return m_cDispatchContinues;
}

void COSDisk::IOQueue::TrackIorunEnqueue( _In_ const IOREQ * const pioreqHead, _In_ const DWORD cbRun, _In_ HRT hrtEnqueueBegin, _In_ OSDiskIoQueueManagement dioqm ) const
{
    const LONG cusecEnqueueLatency = (LONG)CusecHRTFromDhrt( HrtHRTCount() - hrtEnqueueBegin );

    Assert( ( dioqm & mskQueueType ) != 0 );

    dioqm |= ( ( pioreqHead->m_cRetries > 0 ) ? dioqmReEnqueued : dioqmInvalid );
    Assert( dioqm != dioqmInvalid ); // should already have queue, see above.

    const DWORD fWrite = pioreqHead->fWrite ? 1 : 0;

    //  Because the meted Q can now consume a inserted IoRun in any form into a new merged IoRun,
    //  then the cbRun may not be represented by the final run's chain of IOREQs.
    Assert( ( dioqm & dioqmMergedIorun ) || CbSumRun( pioreqHead ) == cbRun );  // proper originating (freshly added) iorun, want to ensure tracking is correct.
    
    OSTrace(    JET_tracetagIOQueue,
                // Renaming this "IOREQ-Heap-Enqueue" to IorunEnqueue would require updates to Exchange exmon parser!
                OSFormat(   "IOREQ-Heap-Enqueue %I64X:%016I64X for TID 0x%x into I/O %hs (%s at %016I64X for %08X bytes), EngineFile=%d:0x%I64x ql=%d",
                            pioreqHead->p_osf->iFile,
                            pioreqHead->ibOffset,
                            pioreqHead->m_tidAlloc,
                            SzOSDiskMapQueueType( dioqm ),
                            fWrite ? "write" : "read",
                            pioreqHead->ibOffset,
                            cbRun,
                            pioreqHead->p_osf->pfpapi->DwEngineFileType(),
                            pioreqHead->p_osf->pfpapi->QwEngineFileId(),
                            cusecEnqueueLatency ) );

    static_assert( sizeof(dioqm) == sizeof(BOOL)/* original size of fHeapA */, "The enum should match original size of argument." );
    static_assert( sizeof(dioqm) == sizeof(DWORD)/* what we consider it now */, "The enum should match new size of argument." );

    ETIorunEnqueue( pioreqHead->p_osf->iFile,
            pioreqHead->ibOffset,
            cbRun,
            pioreqHead->m_tidAlloc,
            dioqm,
            fWrite,
            pioreqHead->p_osf->pfpapi->DwEngineFileType(),
            pioreqHead->p_osf->pfpapi->QwEngineFileId(),
            cusecEnqueueLatency );

    PERFOpt( pioreqHead->p_osf->pfpapi->IncrementIOInHeap( pioreqHead->fWrite ) );
}

void COSDisk::IOQueue::TrackIorunDequeue( _In_ const IOREQ * const pioreqHead, _In_ const DWORD cbRun, _In_ HRT hrtDequeueBegin, _In_ const OSDiskIoQueueManagement dioqm, _In_ const USHORT cIorunCombined, _In_ const IOREQ * const pioreqAdded ) const
{
    // A set of headers usuable for logparser ...
    //IOREQ-Dequeue, iFile, IOR, Iorp, Iors, Ioru, Iorf,             Heap, OP, Oper, ibOffset,   cbData, QOS, msecQueueWait

    Assert( pioreqHead );
    Assert( pioreqAdded );  // note can/should be == pioreqHead on first Q removal, while later removals that can combine ioruns would have different values.

    const HRT hrtDone = HrtHRTCount();
    const LONG cusecDequeueLatency = CusecOSDiskSmallDuration( hrtDone - hrtDequeueBegin );
    const QWORD cusecTimeInQueue = CusecHRTFromDhrt( hrtDone - pioreqAdded->hrtIOStart ); // this is time in the queue

    PERFOpt( pioreqAdded->p_osf->pfpapi->DecrementIOInHeap( pioreqAdded->fWrite ) );

    Expected( FIOThread() );
    Assert( ( dioqm & mskQueueType ) != 0 );

    //  Should be same file and file identity (probably should just check p_osf's match ;)
    Assert( pioreqHead->p_osf->iFile == pioreqAdded->p_osf->iFile );
    Assert( pioreqHead->p_osf->pfpapi->DwEngineFileType() == pioreqAdded->p_osf->pfpapi->DwEngineFileType() );
    Assert( pioreqHead->p_osf->pfpapi->QwEngineFileId() == pioreqAdded->p_osf->pfpapi->QwEngineFileId() );

    Assert( !!pioreqHead->fWrite == !!pioreqAdded->fWrite );

    const DWORD fWrite = pioreqHead->fWrite ? 1 : 0;

    const DWORD cbAdded = CbSumRun( pioreqAdded );

    OSTrace(    JET_tracetagIOQueue,
                OSFormat(   "IOREQ-Heap-Dequeue, %I64X:%016I64X, for TID 0x%x IOR, %d, %d, %d, 0x%x, from I/O Heap %hs, OP, %s, 0x%08X, 0x%I64x, %I64u, EngineFile=%d:0x%I64x, ql=%d",
                            pioreqAdded->p_osf->iFile,
                            pioreqAdded->ibOffset,
                            pioreqAdded->m_tidAlloc,
                            (DWORD)pioreqAdded->m_tc.etc.iorReason.Iorp(),
                            (DWORD)pioreqAdded->m_tc.etc.iorReason.Iors(),
                            (DWORD)pioreqAdded->m_tc.etc.iorReason.Ioru(),
                            (DWORD)pioreqAdded->m_tc.etc.iorReason.Iorf(),
                            SzOSDiskMapQueueType( dioqm ),
                            fWrite ? "write" : "read",
                            cbAdded,
                            pioreqAdded->grbitQOS,
                            cusecTimeInQueue,
                            pioreqHead->p_osf->pfpapi->DwEngineFileType(),
                            pioreqHead->p_osf->pfpapi->QwEngineFileId(),
                            cusecDequeueLatency
                            ) );
    
    static_assert( sizeof(dioqm) == sizeof(UINT/*UInt32 from trace file*/), "The enum type should be 32-bit or could lose data in the trace." );

    Assert( ( pioreqAdded->grbitQOS & 0xFFFFFFFF00000000 ) == 0 ); // We truncate in trace here - but should only bitIOComplete* Signals in the high DWORD - which are not set yet.
    ETIorunDequeue( pioreqAdded->p_osf->iFile,
            pioreqAdded->ibOffset,
            cbAdded,  // should we also trace the whole cbRun as well?
            pioreqAdded->m_tidAlloc,
            dioqm,
            fWrite,
            (DWORD)pioreqAdded->m_tc.etc.iorReason.Iorp(),
            (DWORD)pioreqAdded->m_tc.etc.iorReason.Iors(),
            (DWORD)pioreqAdded->m_tc.etc.iorReason.Ioru(),
            (DWORD)pioreqAdded->m_tc.etc.iorReason.Iorf(),
            (DWORD)pioreqAdded->grbitQOS,
            cusecTimeInQueue,
            pioreqHead->p_osf->pfpapi->DwEngineFileType(),
            pioreqHead->p_osf->pfpapi->QwEngineFileId(),
            m_cDispatchPass,
            cIorunCombined,
            cusecDequeueLatency );
}

void COSDisk::IOQueue::InsertOp( __inout COSDisk::QueueOp * pqop )
{
    Assert( pqop );
    Assert( pqop->FValid() );
    Assert( !pqop->FEmpty() );

    const HRT hrtExtractBegin = HrtHRTCount();

    OSDiskIoQueueManagement dioqm = dioqmInvalid;

    if ( !m_pcritIoQueue->FTryEnter() )
    {
        m_pcritIoQueue->Enter();
        dioqm |= dioqmAccessStall;
    }
    Assert( m_pcritIoQueue->FOwner() );

    const DWORD cbRun = pqop->CbRun();
    IOREQ * pioreqHead = NULL;


    OnDebug( const BOOL fUseMetedQueue = pqop->FUseMetedQ() );

    if ( //  i.e. qosIODispatchBackground passed for Read IO or qosIODispatchWriteMeted
         pqop->FUseMetedQ() &&
         //  but we want to avoid the "null" qops that we traditionally pass through the VIP list
         ( pqop->PioreqOp() == NULL && pqop->CbRun() != 0 ) &&
         //  this one probably technically not necessary, because doesn't go through the IO heap anymore,
         //  but just in case OOM handling shunts it there, I'm going to make it set for now.
         pqop->FHasHeapReservation() )
    {
        IOREQ * pioreqRealIo;

        pqop->SetIOREQType( IOREQ::ioreqEnqueuedInMetedQ );

        const LONG cioreqRun = pqop->Cioreq();
        pioreqHead = pqop->PioreqGetRun();

        const BOOL fNewRun = pioreqHead->fWrite ?
                                m_qWriteIo.FInsertIo( pioreqHead, cioreqRun, &pioreqRealIo ) :
                                m_qMetedAsyncReadIo.FInsertIo( pioreqHead, cioreqRun, &pioreqRealIo );
        if ( pioreqRealIo == NULL )
        {
            //  Overbooked flight, your seat is taken already, upgrade to VIP / first class ...
            pqop->FAddToRun( pioreqHead );
            dioqm = dioqm | dioqmFreeVipUpgrade;
            goto FreeUpgradeToFirstClass;
        }

        dioqm = dioqmTypeMetedQ | ( fNewRun ? dioqmInvalid : dioqmMergedIorun );
    }
    else if ( pqop->FHasHeapReservation() )
    {
        //  mark IOREQ as being in I/O Heap
        //
        pqop->SetIOREQType( IOREQ::ioreqEnqueuedInIoHeap );

        pioreqHead = pqop->PioreqGetRun();

        Expected( pioreqHead->fWrite || !fUseMetedQueue ||
                    //  Ensuring that BF reason were not sent here ... layering violation, drop some day.
                    ( pioreqHead->m_tc.etc.iorReason.Iorp() != 34 /* iorpBFPreread */ ) );

        IOHeapAdd( pioreqHead, &dioqm );
    }
    else
    {
    FreeUpgradeToFirstClass:
        //  Hey, we're a VIP operation, there is always room for one more.
        //

        pioreqHead = pqop->PioreqGetRun();
        Assert( pioreqHead->grbitQOS & qosIOPoolReserved || NULL == Postls()->pioreqCache );

        Assert( NULL == pioreqHead->pioreqVipList );
        pioreqHead->SetIOREQType( IOREQ::ioreqEnqueuedInVipList );
        m_VIPListHead.AtomicInsertAsPrevMost( pioreqHead, OffsetOf(IOREQ,pioreqVipList) );
        m_cioVIPList++;

        dioqm = ( dioqm & ~mskQueueType ) | dioqmTypeVIP;
        Assert( FValidVIPList() );
    }

    Assert( pqop->FEmpty() );

    TrackIorunEnqueue( pioreqHead, cbRun, hrtExtractBegin, dioqm );

    pioreqHead = NULL;  // after we leave critical section, IOREQ could be removed from heap/pri queue at any time.
    m_pcritIoQueue->Leave();
}

//  This pulls out and creates an IO run from the heap / queue, and removes the 
//  relevant IOREQs from the heap.

void COSDisk::IOQueue::ExtractOp(
    _In_ const COSDisk * const      posd,
    _Inout_ COSDisk::QueueOp *      pqop )
{
    Assert( pqop );
    Assert( pqop->FEmpty() );

    const HRT hrtExtractBegin = HrtHRTCount();

    OSDiskIoQueueManagement dioqm = dioqmInvalid;

    if ( !m_pcritIoQueue->FTryEnter() )
    {
        m_pcritIoQueue->Enter();
        dioqm |= dioqmAccessStall;
    }
    Assert( m_pcritIoQueue->FOwner() );

    if ( m_cioVIPList || !m_VIPListHead.FEmpty() )
    {
        Assert( m_cioVIPList );
        Assert( !m_VIPListHead.FEmpty() );
        Assert( FValidVIPList() );

        //  Uh-oh we have a VIP operation here, lets show them first class treatment ...

        //  Remove the first IO from the list

        IOREQ * const pioreqT = m_VIPListHead.RemovePrevMost( OffsetOf(IOREQ,pioreqVipList) );
        m_cioVIPList--;

        //  Create a QueueOp.

        const bool fAccepted = pqop->FAddToRun( pioreqT );
        Assert( fAccepted );

        //  Validate VIP list is consistent.

        Assert( FValidVIPList() );

        TrackIorunDequeue( pioreqT, pqop->CbRun(), hrtExtractBegin, dioqm | dioqmTypeVIP, 1, pioreqT );
    }
    else if (
                PioreqIOHeapTop() == NULL ||
                m_qMetedAsyncReadIo.FStarvedMetedOp() ||
                posd->CioReadyMetedEnqueued() > 0 ||
                m_qWriteIo.CioEnqueued() > 0 )
    {
        if ( m_qMetedAsyncReadIo.IfileiboffsetFirstIo( IOQueueToo::qfifDraining ) == IFILEIBOFFSET::IfileiboffsetNotFound &&
             m_qWriteIo.IfileiboffsetFirstIo( IOQueueToo::qfifDraining ) == IFILEIBOFFSET::IfileiboffsetNotFound )
        {
            //  If we are out of IOs on both draining lists, then cycle both Qs up front to load any building 
            //  list IOs for evaluation.
            m_qMetedAsyncReadIo.Cycle();
            m_qWriteIo.Cycle();
        }

        const IFILEIBOFFSET ifileiboffsetNextReadIo = m_qMetedAsyncReadIo.IfileiboffsetFirstIo( IOQueueToo::qfifDraining );
        const IFILEIBOFFSET ifileiboffsetNextWriteIo = m_qWriteIo.IfileiboffsetFirstIo( IOQueueToo::qfifDraining );

        //  this just means the draining side is empty, not the same as _totally_ empty, could
        //  have IOs in building side of Q.
        const BOOL fReadIoDrained = ( ifileiboffsetNextReadIo == IFILEIBOFFSET::IfileiboffsetNotFound );
        const BOOL fWriteIoDrained = ( ifileiboffsetNextWriteIo == IFILEIBOFFSET::IfileiboffsetNotFound );

        const BOOL fReadsLower = ( !fReadIoDrained && fWriteIoDrained ) || // the null case if only read IOs, obviously lower.
                                 ( !fReadIoDrained && !fWriteIoDrained &&
                                    ( ifileiboffsetNextReadIo < ifileiboffsetNextWriteIo ||
                                      // tie goes to reader
                                      ifileiboffsetNextReadIo == ifileiboffsetNextWriteIo ) );
        const BOOL fMetedQReady = m_qMetedAsyncReadIo.FStarvedMetedOp() ||
                                   posd->CioReadyMetedEnqueued() > 0;

        IOREQ * pioreqT = NULL;

        if ( !m_qMetedAsyncReadIo.FStarvedMetedOp() &&
                posd->CioReadyMetedEnqueued() <= 0 &&
                m_qWriteIo.CioEnqueued() <= 0 )
        {
            //  Note: The main IO mgr loop should not do this, but to make the contract on the unit tests
            //  seem complete, we reject such a dequeue request (also good as defense in depth).
            Assert( FNegTest( fInvalidUsage ) );
            Assert( fWriteIoDrained );

            if ( PioreqIOHeapTop() != NULL )
            {
                Assert( pqop->FEmpty() );
                goto ExtractFromIoHeap;
            }

            m_pcritIoQueue->Leave();

            Assert( pqop->FEmpty() );
            return;
        }

        BOOL fCycleQ = fFalse;
        if ( fMetedQReady && fReadsLower )
        {
            fCycleQ = fTrue;
            dioqm |= dioqmMetedQCycled;
            m_qMetedAsyncReadIo.ExtractIo( pqop, &pioreqT, &fCycleQ );
        }

        //  Note: the fCycleQ will only come back as true when the m_qMetedAsyncReadIo queue
        //  ran out of IO in draining, and moved the building Q to draining ... and at that
        //  time we want to pull any building Write IOs as well.
        //  This is not to ensure that write IOs keep pace, but more to ensure our meted Read 
        //  IOs, don't allow more write go through for the same batch of Reads.  Later we may 
        //  even investigate other ways of disadvantaging background write IOs.

        if ( fCycleQ )
        {
            m_qWriteIo.Cycle();
        }

        if ( pqop->FEmpty() )
        {
            //  Didn't get a Read IO to try, check for a Write IO.
            

            BOOL fNoCycling = fFalse;
            m_qWriteIo.ExtractIo( pqop, &pioreqT, &fNoCycling );
            Assert( fNoCycling == fFalse );
        }
        
        if ( pqop->FEmpty() )
        {
            if ( PioreqIOHeapTop() != NULL )
            {
                Assert( pqop->FEmpty() );
                goto ExtractFromIoHeap;
            }

            m_pcritIoQueue->Leave();

            Assert( pqop->FEmpty() );
            return;
        }
        
        TrackIorunDequeue( pioreqT, pqop->CbRun(), hrtExtractBegin, dioqm | dioqmTypeMetedQ, 1, pioreqT );
    }
    else
    {
ExtractFromIoHeap:
        Assert( pqop->FEmpty() );

        //  collect a run of IOREQs with continuous p_osf/ibs that are the same
        //  I/O type (read vs write)

        IOREQ *         pioreqHead = NULL;
        IOREQ *         pioreqT;
        USHORT          cIorunCombined = 0;
        while ( pioreqT = PioreqIOHeapTop() )       //  get top of I/O heap
        {
            //  more IOREQs / IO runs in the heap ...

            const bool fNewRun = pqop->FEmpty();
            if ( fNewRun )
            {
                pioreqHead = pioreqT;
            }
            Assert( pioreqHead );
            
            //  we have moved _most_ reads to m_qMetedAsyncReadIo.

            Expected( pqop->FEmpty() || !pqop->FUseMetedQ() || pqop->FWrite() );

            //  attempt to add top IOREQ to IO run

            const bool fAccepted = pqop->FAddToRun( pioreqT );

            Assert( pqop->FValid() );
            Assert( !pqop->FEmpty() );

            if( !fAccepted )
            {
                //  determined this IOREQ was not accumulated in the current IO run / op, done, bail.
                Assert( !pqop->FEmpty() );
                break;
            }

            //  take IO run out of I/O Heap

            IOHeapRemove( pioreqT, &dioqm );
            Assert( pioreqT != PioreqIOHeapTop() );

            if ( !fNewRun )
            {
                //  we combined it on back end / dequeue
                OnNonRTM( pioreqT->m_fDequeueCombineIO = fTrue );
            }


            cIorunCombined++;
            TrackIorunDequeue( pioreqHead, pqop->CbRun(), hrtExtractBegin, dioqm, cIorunCombined, pioreqT );
        }

        Assert( cIorunCombined > 0 ); // should be at least one
    }

    //  Validate the extracted QueueOp's consistency.

    Assert( pqop->FValid() );
    Assert( !pqop->FEmpty() );
    //  We can have either a NonIO or an IO Run, but not both.
    Assert( ( pqop->PioreqOp() && NULL == pqop->PiorunIO() ) ||
            ( NULL == pqop->PioreqOp() && pqop->PiorunIO() ) );

    //  mark IOREQ as being removed from Meted Q (or VIP list or Heap)
    //

    pqop->SetIOREQType( IOREQ::ioreqRemovedFromQueue );

    //  defer register this file with the I/O thread.  if we fail, we will
    //  be forced to fallback to sync I/O

    //  probably don't need to register for CPs on these fake cbData == 0 / PioreqOp()...
    _OSFILE * const p_osfCP = pqop->PioreqOp() ? pqop->PioreqOp()->p_osf : pqop->PiorunIO()->P_OSF();
    Assert( p_osfCP );
    if ( !p_osfCP->fRegistered )
    {
        if ( ErrOSDiskIIOThreadRegisterFile( p_osfCP ) >= JET_errSuccess )
        {
            p_osfCP->fRegistered = fTrue;
        }
    }

    m_pcritIoQueue->Leave();
}


BOOL COSDisk::IOQueue::FReleaseQueueSpace( __inout IOREQ * pioreq )
{
    BOOL fFreedIoQuota = fFalse;

    Assert( m_cioQosUrgentBackgroundCurrent >= 0 );
    Assert( m_cioQosBackgroundCurrent >= 0 );

    if ( pioreq->m_fHasUrgentBackgroundReservation )
    {
        Assert( !pioreq->m_fHasBackgroundReservation );

        LONG cioreqQueueOutstanding = AtomicDecrement( (LONG*)&m_cioQosUrgentBackgroundCurrent );

        //  Send the go signal back to the client if we've fallen below the previous urgent level
        LONG iUrgentLevel = IOSDiskIUrgentLevelFromQOS( pioreq->grbitQOS );
        if ( iUrgentLevel > 1 )
        {
            iUrgentLevel--;
        }
        if ( cioreqQueueOutstanding <= CioOSDiskIFromUrgentLevel( iUrgentLevel, m_cioQosUrgentBackgroundMax ) )
        {
            fFreedIoQuota = fTrue;
        }

        pioreq->m_fHasUrgentBackgroundReservation = fFalse;
    }
    else if ( pioreq->m_fHasBackgroundReservation )
    {
        Assert( !pioreq->m_fHasUrgentBackgroundReservation );

        LONG cioreqQueueOutstanding = AtomicDecrement( (LONG*)&m_cioQosBackgroundCurrent );
        if ( cioreqQueueOutstanding <= m_cioreqQOSBackgroundLow )
        {
            fFreedIoQuota = fTrue;
        }

        pioreq->m_fHasBackgroundReservation = fFalse;
    }

    if ( pioreq->m_fHasHeapReservation )
    {
        m_semIOQueue.Release();
        pioreq->m_fHasHeapReservation = fFalse;
    }

    return fFreedIoQuota;
}

DWORD OSDiskIFillFSEAddress(
    const BYTE*                     pbData,
    DWORD                           cbData,
    bool                                fRepeat,        // use pbData repeatedly
    _In_ const DWORD                    cfse,
    _Out_ PFILE_SEGMENT_ELEMENT const   rgfse )
{
    DWORD cbDelta = fRepeat ? 0 : OSMemoryPageCommitGranularity();
    DWORD cospage = 0;

    Enforce( 0 == cbData % OSMemoryPageCommitGranularity() );

    for ( DWORD cb = 0; cb < cbData && cospage < cfse; cb += OSMemoryPageCommitGranularity() )
    {
        //  Must cast to avoid sign extension of pbData.
        rgfse[ cospage ].Buffer = PVOID64( ULONG_PTR( pbData ) );

        Assert( rgfse[ cospage ].Buffer != NULL );
        Assert( 0 == (ULONG64)rgfse[ cospage ].Buffer % OSMemoryPageCommitGranularity() );

        pbData += cbDelta;
        cospage++;
    }

    return cospage;
}

//  This takes an IO run / pioreqHead and sets up the IOREQs for IO, constructs
//  the appropriate rgfse, and gets the highest IO priority.

void OSDiskIIOPrepareScatterGatherIO(
    _In_ IOREQ * const                  pioreqHead,
    _In_ const DWORD                    cbRun,
    _In_ const DWORD                    cfse,
    _Out_ PFILE_SEGMENT_ELEMENT const   rgfse,
    _Out_ BOOL *                        pfIOOSLowPriority
    )
{
    Assert( pioreqHead );
    Assert( pioreqHead->pioreqIorunNext );  // should be more than one IOREQ
    Assert( cbRun && ( cbRun <= ( pioreqHead->fWrite ? pioreqHead->p_osf->Pfsconfig()->CbMaxWriteSize() : pioreqHead->p_osf->Pfsconfig()->CbMaxReadSize() ) ||
            ( pioreqHead->grbitQOS & qosIOOptimizeOverrideMaxIOLimits ) ) );

    DWORD cospage = 0;
    QWORD ibOffset = pioreqHead->ibOffset;
    for ( IOREQ* pioreq = pioreqHead; pioreq; pioreq = pioreq->pioreqIorunNext )
    {
        Assert( pioreq->cbData );

        //  upgrade OS IO priority to "highest common denominator"
        if ( 0 == ( qosIOOSLowPriority & pioreq->grbitQOS ) )
        {
            *pfIOOSLowPriority = fFalse;
        }

        // Note: this is the only way in which these IOREQs are "prepared for IO" here ...
        if ( pioreq != pioreqHead )
        {
            pioreq->fCoalesced = fTrue;
        }

        //  setup Scatter/Gather I/O source array for this pioreq
        Assert( ibOffset <= pioreq->ibOffset ); // ioreqs must be monotonically increasing

        //  a gap exists before pioreq
        if ( ibOffset < pioreq->ibOffset )
        {
            DWORD cbGap = DWORD( pioreq->ibOffset - ibOffset );
            Assert( !pioreq->fWrite );          // only read can have a gap
            Assert( cbGap <= pioreqHead->p_osf->Pfsconfig()->CbMaxReadGapSize() );    // and gap size is valid

            cospage += OSDiskIFillFSEAddress( g_rgbGapping, cbGap, true, cfse - cospage - 1, &rgfse[ cospage ] );
            ibOffset = pioreq->ibOffset;
        }

        // pioreq itself
        cospage += OSDiskIFillFSEAddress( pioreq->pbData, pioreq->cbData, false, cfse - cospage - 1, &rgfse[ cospage ] );
        ibOffset += pioreq->cbData;
    }

    //  null terminate the scatter list
    Assert( cospage < cfse );
    rgfse[ cospage ].Buffer = NULL;
}


DWORD ErrorRFSIssueFailedIO()
{
    return ERROR_IO_DEVICE;
}


//  workaround the fact that the current implementation of GetOverlappedResult
//  has a race condition where it can return before the kernel sets the event
//  in the overlapped struct.  this can cause subsequent I/Os that use this
//  event to return from GetOverlappedResult before the I/O has been completed

BOOL GetOverlappedResult_(  HANDLE          hFile,
                            LPOVERLAPPED    lpOverlapped,
                            LPDWORD         lpNumberOfBytesTransferred,
                            BOOL            bWait )
{
    DWORD error;

    OnThreadWaitBegin();
    error = WaitForSingleObjectEx(  lpOverlapped->hEvent ? lpOverlapped->hEvent : hFile,
                                    bWait ? INFINITE : 0,
                                    FALSE );
    OnThreadWaitEnd();

    if ( error == WAIT_OBJECT_0 )
    {
        Assert( HasOverlappedIoCompleted( lpOverlapped ) );
        return GetOverlappedResult( hFile,
                                    lpOverlapped,
                                    lpNumberOfBytesTransferred,
                                    FALSE );
    }
    else if ( error == WAIT_TIMEOUT )
    {
        SetLastError( ERROR_IO_INCOMPLETE );
        return FALSE;
    }
    else
    {
        //  return error from WaitForSingleObjectEx
        return FALSE;
    }
}

//  This issue's an IO run against sync, async, or scatter gather OS IO functions ...

// Note: This function returns a Win32 Error, not a JET error.

DWORD ErrorIOMgrIssueIO(
    _In_ COSDisk::IORun*                    piorun,
    _In_ const IOREQ::IOMETHOD              iomethod,
    __inout const PFILE_SEGMENT_ELEMENT     rgfse,
    _In_ const DWORD                        cfse,
    _Out_ BOOL *                            pfIOCompleted,
    _Out_ DWORD *                           pcbTransfer,
    _Out_ IOREQ **                          ppioreqHead = NULL
    )
{
    DWORD error = ERROR_SUCCESS;
    DWORD cbRun = 0;
    BOOL fIOOSLowPriority = fFalse;

    Assert( piorun != NULL );

    //  Init out variables

    *pfIOCompleted = fFalse;
    *pcbTransfer = 0;

    //  Prepare I/O

    piorun->PrepareForIssue( iomethod, &cbRun, &fIOOSLowPriority, rgfse, cfse );
    IOREQ * const pioreqHead = piorun->PioreqGetRun();
    piorun = NULL;  // ensure no deref, IORun is not valid from this point on.

    if ( ppioreqHead != NULL )
    {
        *ppioreqHead = pioreqHead;
    }

    //  RFS:  pre-completion error

    BOOL fIOSucceeded = RFSAlloc( pioreqHead->fWrite ? OSFileWrite : OSFileRead );
    if ( !fIOSucceeded )
    {
        return ErrorRFSIssueFailedIO();
    }
    error = ErrFaultInjection( 17384 );
    if ( error )
    {
        return error;
    }

    //  exclusive IOREQs can't handle OOM, so to avoid failures for this handleable condition we'll only fault 
    //  inject this OOM for non-exclusive IOs.  If you want the regular fault inject, new 42980 can be used.
    if ( !FExclusiveIoreq( pioreqHead ) && ErrFaultInjection( 64738 ) < JET_errSuccess )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    if ( ErrFaultInjection( 42980 ) < JET_errSuccess )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //  Lower IO priority, if needed

    if ( fIOOSLowPriority )
    {
        UtilThreadBeginLowIOPriority();
    }

    //  I don't think we should clobber these values ...
    Assert( !pioreqHead->FSetSize() &&
            !pioreqHead->FExtendingWriteIssued() );

    Assert( pioreqHead->pioreqVipList == NULL );

    //  issue the I/O

    Assert( iomethod != IOREQ::iomethodNone );

    switch ( iomethod )
    {
        case IOREQ::iomethodSync:
        {
            Assert( NULL == pioreqHead->pioreqIorunNext );
            Assert( cbRun == pioreqHead->cbData );

            pioreqHead->p_osf->semFilePointer.Acquire();


            Assert( pioreqHead->ovlp.OffsetHigh <= lMax );
            LARGE_INTEGER ibOffset;
            ibOffset.LowPart = pioreqHead->ovlp.Offset;
            ibOffset.HighPart = LONG( pioreqHead->ovlp.OffsetHigh );
            fIOSucceeded = SetFilePointerEx( pioreqHead->p_osf->hFile, ibOffset, NULL, FILE_BEGIN );

            if ( fIOSucceeded )
            {
                if ( pioreqHead->fWrite )
                {
                    fIOSucceeded = WriteFile( pioreqHead->p_osf->hFile,
                                                    pioreqHead->pbData,
                                                    cbRun,
                                                    pcbTransfer,
                                                    NULL );
                }
                else
                {
                    fIOSucceeded = ReadFile( pioreqHead->p_osf->hFile,
                                                    (BYTE*)pioreqHead->pbData,
                                                    cbRun,
                                                    pcbTransfer,
                                                    NULL );
                }
                //  In current builds we are always failing here with invalid parameter, because
                //  the overlapped structure is NULL when we opened the file FILE_FLAG_OVERLAPPED.
            }

 
            //  for synchronous IO, success means we immediatley completed, signal such ...
            *pfIOCompleted = fIOSucceeded;

            pioreqHead->p_osf->semFilePointer.Release();
            break;
        }

        case IOREQ::iomethodSemiSync:
        {
            Assert( NULL == pioreqHead->pioreqIorunNext );
            Assert( cbRun == pioreqHead->cbData );

            //  directly issue the I/O

            pioreqHead->ovlp.hEvent = HANDLE( DWORD_PTR( pioreqHead->ovlp.hEvent ) | DWORD_PTR( hNoCPEvent ) );

            if ( pioreqHead->fWrite )
            {
                fIOSucceeded = WriteFile( pioreqHead->p_osf->hFile,
                                                    pioreqHead->pbData,
                                                    cbRun,
                                                    pcbTransfer,
                                                    LPOVERLAPPED( pioreqHead ) );
            }
            else
            {
                fIOSucceeded = ReadFile( pioreqHead->p_osf->hFile,
                                                    (BYTE*)pioreqHead->pbData,
                                                    cbRun,
                                                    pcbTransfer,
                                                    LPOVERLAPPED( pioreqHead ) );
            }
            Assert( fIOSucceeded || ERROR_SUCCESS != GetLastError() );

            error               = fIOSucceeded ? ERROR_SUCCESS : GetLastError();
            const ERR errIO     = fIOSucceeded ? JET_errSuccess : ErrOSFileIFromWinError( error );

            if ( errIO == wrnIOPending )
            {
                //  wait for the I/O to complete and complete the I/O with the
                //  appropriate error code

                *pcbTransfer = 0;

                fIOSucceeded = GetOverlappedResult_(    pioreqHead->p_osf->hFile,
                                                        LPOVERLAPPED( pioreqHead ),
                                                        pcbTransfer,
                                                        TRUE );
                error   = fIOSucceeded ? ERROR_SUCCESS : GetLastError();
            }
            else
            {
                Assert( errIO <= JET_errSuccess );  //  either success or failure ...
            }

            if ( fIOSucceeded )
            {
                Assert( error == ERROR_SUCCESS );
                //  for synchronous IO, success means we immediately completed, signal such ...
                *pfIOCompleted = fTrue;
            }
            else
            {
                //  Failed IO ... should have error ...
                Assert( GetLastError() != ERROR_SUCCESS && GetLastError() != ERROR_IO_PENDING );
                Assert(  ErrOSFileIGetLastError() < JET_errSuccess );
                Assert(  ErrOSFileIGetLastError() != wrnIOPending );
            }

            pioreqHead->ovlp.hEvent = HANDLE( DWORD_PTR( pioreqHead->ovlp.hEvent ) & ( ~DWORD_PTR( hNoCPEvent ) ) );

            break;
        }

        case IOREQ::iomethodAsync:

            Assert( NULL == pioreqHead->pioreqIorunNext );
            Assert( cbRun == pioreqHead->cbData );
            Assert( pioreqHead->p_osf->fRegistered );

            if ( pioreqHead->fWrite )
            {
                fIOSucceeded = WriteFile( pioreqHead->p_osf->hFile,
                                                    pioreqHead->pbData,
                                                    cbRun,
                                                    NULL,
                                                    LPOVERLAPPED( pioreqHead ) );
            }
            else
            {
                fIOSucceeded = ReadFile( pioreqHead->p_osf->hFile,
                                                    (BYTE*)pioreqHead->pbData,
                                                    cbRun,
                                                    NULL,
                                                    LPOVERLAPPED( pioreqHead ) );
            }
            break;

        case IOREQ::iomethodScatterGather:

            //  Should be a multi-IOREQ I/O ...
            Assert( NULL != pioreqHead->pioreqIorunNext );
            Assert( cbRun > pioreqHead->cbData );
            Assert( pioreqHead->p_osf->fRegistered );

            if ( pioreqHead->fWrite )
            {
                fIOSucceeded = WriteFileGather( pioreqHead->p_osf->hFile,
                                                rgfse,
                                                cbRun,
                                                NULL,
                                                LPOVERLAPPED( pioreqHead ) );
            }
            else
            {
                fIOSucceeded = ReadFileScatter( pioreqHead->p_osf->hFile,
                                                rgfse,
                                                cbRun,
                                                NULL,
                                                LPOVERLAPPED( pioreqHead ) );
            }
            break;

        default:
            AssertSz( fFalse, "What just happened?  What iomethod = %d is this?", iomethod );
    }

    if ( !fIOSucceeded )
    {
        error = GetLastError();
    }
    else
    {
        Assert( ERROR_SUCCESS == error );
        error = ERROR_SUCCESS;
    }

    //  Reset IO priority to normal

    if ( fIOOSLowPriority )
    {
        UtilThreadEndLowIOPriority();
    }

    return error;
}


//
//  IO Run Pool
//

INT COSDisk::CIoRunPool::IrunIoRunPoolIFindFileRun_( _In_ const _OSFILE * const p_osf ) const
{

    for( INT irun = 0; irun < _countof( m_rgiorunIoRuns ); irun++ )
    {

        if ( !m_rgiorunIoRuns[irun].FEmpty() &&
                ( ( p_osf == NULL ) ||
                  ( m_rgiorunIoRuns[irun].P_OSF() == p_osf ) ) )
        {
            return irun;
        }
    }
    return irunNil;
}

IOREQ * COSDisk::CIoRunPool::PioreqIoRunPoolIRemoveRunSlot_( _In_ const INT irun )
{
    Assert( !m_rgiorunIoRuns[irun].FEmpty() );

    IOREQ * pioreqRet = m_rgiorunIoRuns[irun].PioreqGetRun();
    m_rghrtIoRunStarted[irun] = 0;

    Assert( m_rgiorunIoRuns[irun].FEmpty() );

    return pioreqRet;
}

void COSDisk::CIoRunPool::IoRunPoolIAddNewRunSlot_( _In_ const INT irun, _Inout_ IOREQ * pioreqToAdd )
{
    //  to add a new run, intended slot should be empty
    Assert( m_rgiorunIoRuns[irun].FEmpty() );
    
    const BOOL fAdded = m_rgiorunIoRuns[irun].FAddToRun( pioreqToAdd );
    m_rghrtIoRunStarted[irun] = pioreqToAdd->hrtIOStart;    //  we could grab a new one, but for debugging this is a bit easier

    Enforce( fAdded );  //  this would mean hung IOs

    Assert( !m_rgiorunIoRuns[irun].FEmpty() );
    Assert( FContainsFileIoRun( pioreqToAdd->p_osf ) );
}

BOOL COSDisk::CIoRunPool::FEmpty( _In_ const IoRunPoolEmptyCheck fCheck ) const
{
    Assert( fCheck == fCheckOneEmpty || fCheck == fCheckAllEmpty );

    if ( fCheck == fCheckOneEmpty )
    {
        for( INT irun = 0; irun < _countof( m_rgiorunIoRuns ); irun++ )
        {
            if ( m_rgiorunIoRuns[irun].FEmpty() )
            {
                return fTrue;
            }
        }
        return fFalse;
    }
    // else

    const INT irun = IrunIoRunPoolIFindFileRun_( NULL );
    //  If there no irun returned it means there are not any runs in the pool,
    //  so it is empty.
    return irun == irunNil;
}

BOOL COSDisk::CIoRunPool::FContainsFileIoRun( _In_ const _OSFILE * const p_osf ) const
{
    Assert( p_osf );

    const INT irun = IrunIoRunPoolIFindFileRun_( p_osf );
    //  If there no irun returned it means there is no matching run for this file.
    return irun != irunNil;
}

BOOL COSDisk::CIoRunPool::FCanCombineWithExistingRun(
    _In_ const _OSFILE *            p_osf,
    _In_ const BOOL                 fWrite,
    _In_ const QWORD                ibOffsetCombine,
    _In_ const DWORD                cbDataCombine,
    _In_ const BOOL                 fOverrideIoMax ) const
{
    Assert( p_osf );

    const INT irunFile = IrunIoRunPoolIFindFileRun_( p_osf );
    if ( irunFile == irunNil )
    {
        return fFalse;  //  no existing run we can combine it with
    }
    Assert( m_rgiorunIoRuns[irunFile].P_OSF() == p_osf ); // the basic pool search criteria

    Assert( !m_rgiorunIoRuns[irunFile].FEmpty() );

    //  Until the IO pool supports accumulated separately writes and reads for the same
    //  file we have to check the IO run type matches.  This is probably only useful for
    //  like small client caches, where they might drop from a pre-read to do some sync
    //  scavenging, and then going back to doing pre-reads.
    if ( !!m_rgiorunIoRuns[irunFile].FWrite() != !!fWrite )
    {
        return fFalse;
    }

    return FOSDiskICanAddToRun( (COSDisk::IORun*)&( m_rgiorunIoRuns[irunFile] ),
                    p_osf,
                    fWrite,
                    ibOffsetCombine,
                    cbDataCombine,
                    fOverrideIoMax );
}

//  The contract of this function is a little weird, but the simplest way to
//  implement a small interface for a limited size pool ... func has two jobs:
//   1) It will consume and add the provied pioreqToAdd to the most appropriate
//      already existing iorun, OR create a new empty iorun.
//   2) It will return any iorun that was forcibly evicted / removed from the
//      pool as a side effect of performing job (1).

IOREQ * COSDisk::CIoRunPool::PioreqSwapAddIoreq( _Inout_ IOREQ * const pioreqToAdd )
{
    Assert( pioreqToAdd );

    Assert( pioreqToAdd->p_osf );

    Assert( pioreqToAdd->hrtIOStart );
    Assert( pioreqToAdd->m_fHasHeapReservation || pioreqToAdd->m_fCanCombineIO );

    IOREQ * pioreqEvictedRun = NULL;
    BOOL fAdded = fFalse;

    INT irun = IrunIoRunPoolIFindFileRun_( pioreqToAdd->p_osf );
    if ( irun != irunNil )
    {
        //  There is an existing run for this file, attempt to combine ...
        OnDebug( const BOOL fShouldBeAdded = FCanCombineWithExistingRun( pioreqToAdd ) );
        Assert( fShouldBeAdded || pioreqToAdd->m_fHasHeapReservation );
        Assert( !pioreqToAdd->m_fCanCombineIO || fShouldBeAdded );  //  or we've really messed up

        //  Since we only allow one iorun per file, and we've found a iorun for
        //  the file that we're adding the IO to, see if the run can be extended.
        fAdded = m_rgiorunIoRuns[irun].FAddToRun( pioreqToAdd );

        Assert( !!fAdded == !!fShouldBeAdded || fShouldBeAdded /* fShouldBeAdded is a little more optimistic */ );
        Assert( fAdded || pioreqToAdd->m_fHasHeapReservation );
        Assert( !pioreqToAdd->m_fCanCombineIO || fAdded );       // or we've really messed up

        if ( !fAdded )
        {
            //  This IO run can not add this IOREQ (policy setting does not want a larger 
            //  IO runs, or write IO is not adjacent, or file doesn't use CP, etc), so 
            //  move to evict the currently accumulated IO run to the IO queue first ...
            
            pioreqEvictedRun = PioreqIoRunPoolIRemoveRunSlot_( irun );

            //  should've cleared this slot (and p_osf associated slot), and this should've 
            //  cleared any IOs for this file
            
            Assert( m_rgiorunIoRuns[irun].FEmpty() );
            Assert( irunNil == IrunIoRunPoolIFindFileRun_( pioreqToAdd->p_osf ) );
        }
    }
    else
    {
        //  There is no existing run for this file, find a new one irun slot ...

        C_ASSERT( _countof(m_rghrtIoRunStarted) == _countof(m_rgiorunIoRuns) );

        Assert( pioreqToAdd->m_fHasHeapReservation );
        
        INT irunOldest = 0;
        for( irun = 0; irun < _countof( m_rgiorunIoRuns ); irun++ )
        {
            if ( m_rgiorunIoRuns[irun].FEmpty() )
            {
                break;
            }

            //  ! empty ... should have a file
            Expected( m_rgiorunIoRuns[irun].P_OSF() );

            if ( ( (__int64)m_rghrtIoRunStarted[irun] - (__int64)m_rghrtIoRunStarted[irunOldest] ) < 0 )
            {
                //  Update potential evict victim
                irunOldest = irun;
            }
        }
        if ( irun == _countof( m_rgiorunIoRuns ) )
        {
            //  No slots available, all used with other files, evict the oldest victim

            irun = irunOldest;
            Assert( !m_rgiorunIoRuns[irun].FEmpty() );  // or the previous for loop didn't work ...

            //  clear this out of the way so we can create a new run ... we unfortunately may
            //  be stomping on someone's building IO chain ...
            pioreqEvictedRun = PioreqIoRunPoolIRemoveRunSlot_( irun );
        }

        Assert( m_rgiorunIoRuns[irun].FEmpty() );
        Assert( irunNil == IrunIoRunPoolIFindFileRun_( pioreqToAdd->p_osf ) );
    }

    if ( !fAdded )
    {
        //  Adding to an existing iorun should've been handled above, so we should have an 
        //  empty run slot ...

        Assert( m_rgiorunIoRuns[irun].FEmpty() );

        Assert( pioreqToAdd->m_fHasHeapReservation );
        Expected( !pioreqToAdd->m_fCanCombineIO );

        IoRunPoolIAddNewRunSlot_( irun, pioreqToAdd );
    }

    return pioreqEvictedRun;
}

IOREQ * COSDisk::CIoRunPool::PioreqEvictFileRuns( _In_ const _OSFILE * const p_osf )
{

    const INT irun = IrunIoRunPoolIFindFileRun_( p_osf );

    if ( irun == irunNil )
    {
#ifdef DEBUG
        if ( p_osf == NULL )
        {
            //  If directed to find ANY iorun and nothing was returned, it implies that we're entirely empty ... so double check.
            Assert( FEmpty( fCheckAllEmpty ) );
            //  And since techically FEmpty() is implemented off IrunIoRunPoolIFindFileRun( NULL ) ... triple check! ;-)
            for( INT irundbg = 0; irundbg < _countof( m_rgiorunIoRuns ); irundbg++ )
            {
                Assert( m_rgiorunIoRuns[irundbg].FEmpty() );
            }
        }
#endif
        return NULL;
    }
    Assert( !m_rgiorunIoRuns[irun].FEmpty() );

    IOREQ * const pioreqRet = m_rgiorunIoRuns[irun].PioreqGetRun();

    Assert( m_rgiorunIoRuns[irun].PioreqHead() == NULL );   //  should've emptied that iorun

    return pioreqRet;
}

#ifdef DEBUG
//  Note: This function doesn't honor IO maxes for writes so we'll leave it debug only (for Asserts)

BOOL COSDisk::CIoRunPool::FCanCombineWithExistingRun( _In_ const IOREQ * const pioreq ) const
{
    return FCanCombineWithExistingRun( pioreq->p_osf, pioreq->fWrite, pioreq->ibOffset, pioreq->cbData, pioreq->fWrite /* only allowed for that */ );
}

//  Only needed for testing (so debug only) ...

INT COSDisk::CIoRunPool::Cioruns() const
{
    INT cioruns = 0;
    for( INT irun = 0; irun < _countof( m_rgiorunIoRuns ); irun++ )
    {
        if ( !m_rgiorunIoRuns[irun].FEmpty() )
        {
            cioruns++;
        }
    }
    return cioruns;
}

#endif  //  DEBUG


ERR COSDisk::ErrReserveQueueSpace( _In_ OSFILEQOS grbitQOS, __inout IOREQ * pioreq )
{
    const ERR errReserve = m_pIOQueue->ErrReserveQueueSpace( grbitQOS, pioreq );

    Assert( JET_errSuccess == errReserve ||
            errDiskTilt == errReserve );

    Assert( errDiskTilt == errReserve || pioreq->m_fHasHeapReservation || ( qosIOPoolReserved & grbitQOS ) );

    return errReserve;
}

//  Allocates an IOREQ if available.
//
//      grbitQOS - This parameter refers to any Quality Of Service arguments, which can
//          cause the IO manager to deem the disk sub-system too busy right now.
//      ib / cbDataCombine - Relevant parameter for determining if the IO can be combined,
//          pass 0,0 if it can not be combined.
//
ERR COSDisk::ErrAllocIOREQ(
    _In_ OSFILEQOS          grbitQOS,
    _In_ const _OSFILE *    p_osf,
    _In_ const BOOL         fWrite,
    _In_ const QWORD        ibOffsetCombine,
    _In_ const DWORD        cbDataCombine,
    _Out_ IOREQ **          ppioreq )
{
    ERR err = JET_errSuccess;

    Assert( ppioreq );

    BOOL fUsedTLSSlot = fFalse;

    *ppioreq = PioreqOSDiskIIOREQAlloc( qosIOPoolReserved & grbitQOS, &fUsedTLSSlot );
    grbitQOS |= ( fUsedTLSSlot ? qosIOPoolReserved : 0 );

    if ( qosIOPoolReserved & grbitQOS )
    {
        Assert( qosIODispatchImmediate == ( qosIODispatchMask & grbitQOS ) );
    }

    (*ppioreq)->m_tidAlloc = DwUtilThreadId();

    (*ppioreq)->m_fCanCombineIO = cbDataCombine &&              // the type of IOREQ you can try to add to a iorun
                    Postls()->iorunpool.FCanCombineWithExistingRun( p_osf, fWrite, ibOffsetCombine, cbDataCombine, grbitQOS & qosIOOptimizeOverrideMaxIOLimits );

    (*ppioreq)->fWrite = fWrite;

    if ( !fWrite &&
            ( qosIOOptimizeCombinable & grbitQOS ) &&
            !( (*ppioreq)->m_fCanCombineIO ) )
    {
        Error( ErrERRCheck( errDiskTilt ) );
    }

    Assert( !(*ppioreq)->m_fHasHeapReservation );
    Assert( !(*ppioreq)->m_fHasBackgroundReservation );
    Assert( !(*ppioreq)->m_fHasUrgentBackgroundReservation );

    Assert( !(*ppioreq)->m_fDequeueCombineIO );
    Assert( !(*ppioreq)->m_fSplitIO );
    Assert( !(*ppioreq)->m_fReMergeSplitIO );
    Assert( !(*ppioreq)->m_fOutOfMemory );
    Assert( (*ppioreq)->m_cTooComplex == 0 );
    Assert( (*ppioreq)->m_cRetries == 0 );

    //  We only need a heap reservation if we can not combine the IO with the currently 
    //  accumulating IO run ...

    if ( !(*ppioreq)->m_fCanCombineIO )
    {
        Call( ErrReserveQueueSpace( grbitQOS, (*ppioreq) ) );

        Assert( (*ppioreq)->m_fHasHeapReservation || ( qosIOPoolReserved & grbitQOS ) );
    }

HandleError:

    if ( errDiskTilt == err )
    {
        // ha, ha, just kidding ...

        Assert( errDiskTilt == err );
        OSDiskIIOREQFree( *ppioreq );
        *ppioreq = NULL;
    }
    else
    {
        Assert( (*ppioreq)->FEnqueueable() || (*ppioreq)->m_fCanCombineIO  || Postls()->pioreqCache == NULL /* because we just used it */ );
        CallS( err );
    }

    return err;
}

VOID COSDisk::FreeIOREQ(
    _In_ IOREQ * const  pioreq
    )
{
    //  Release the Queue reservations

    if ( !pioreq->m_fCanCombineIO )
    {
        (void)m_pIOQueue->FReleaseQueueSpace( pioreq );
    }

    Assert( !pioreq->m_fHasHeapReservation );
    Assert( !pioreq->m_fHasBackgroundReservation );
    Assert( !pioreq->m_fHasUrgentBackgroundReservation );

    //  Reset whether can combine IO
    
    pioreq->m_fCanCombineIO = fFalse;

    // free the IOREQ

    OSDiskIIOREQFree( pioreq );
}

//  This takes an IO Run and enqueues it / inserts it into the heap

void COSDisk::EnqueueIORun( _In_ IOREQ * pioreqHead )
{
    Assert( pioreqHead );
    Assert( pioreqHead->p_osf->m_posd == this );
    Assert( !FExclusiveIoreq( pioreqHead ) );  //   exclusive IOs must be done on foreground

    //  Create a QueueOp out of the IOREQ * chain.

    COSDisk::QueueOp    qop( pioreqHead );
    Assert( !qop.FEmpty() );

    BOOL fReEnqueueIO = fFalse;
    if ( qop.FIssuedFromQueue() )
    {
        //  Whoops this IO didn't quite make it out to the OS ... re-enqueueing

        AtomicDecrement( (LONG*)&m_cioDispatching );
        Assert( m_cioDispatching >= 0 );

        if ( pioreqHead->fWrite )
        {
            Assert( FIOThread() ); // "locking"
            m_cioAsyncWriteDispatching--;
            Assert( m_cioAsyncWriteDispatching >= 0 );
        }
        else
        {
            Assert( FIOThread() ); // "locking"
            m_cioAsyncReadDispatching--;
            Assert( m_cioAsyncReadDispatching >= 0 );
        }

        fReEnqueueIO = fTrue;
    }
    else if ( pioreqHead->FIssuedSyncIO() )
    {
        //  may be a sync IO being "re-enqueued" (technically enqueued for the first time) ...

        fReEnqueueIO = fTrue;
    }
    else
    {
        Assert( !pioreqHead->FRemovedFromQueue() );
        Assert( pioreqHead->m_posdCurrentIO == NULL );
    }

    if ( !fReEnqueueIO )
    {
        pioreqHead->m_iocontext = pioreqHead->p_osf->pfpapi->IncrementIOIssue( pioreqHead->p_osf->m_posd->CioOsQueueDepth(), pioreqHead->fWrite );
    }

    if ( fReEnqueueIO )
    {
        for ( IOREQ* pioreq = pioreqHead; pioreq != NULL; pioreq = pioreq->pioreqIorunNext )
        {
            pioreq->CompleteIO( IOREQ::ReEnqueueingIO );
        }
    }

    Assert( pioreqHead->Iomethod() == IOREQ::iomethodNone );
    Assert( pioreqHead->Ciotime() == 0 );

    m_pIOQueue->InsertOp( &qop );
}

//  This takes an IOREQ and enqueues it either in the TLS or inserts it into the heap,
//  depending ...

void COSDisk::EnqueueIOREQ( _In_ IOREQ * pioreq )
{
    Assert( pioreq );
    Assert( NULL == pioreq->pioreqIorunNext );

    if ( !pioreq->m_fHasHeapReservation && !pioreq->m_fCanCombineIO )
    {
        //  not appropriate for TLS combining, move straight to go.
        EnqueueIORun( pioreq );
        return;
    }

    if ( 0 == pioreq->cbData )
    {
        //  special file extension type stuff, enqueue directly.
        EnqueueIORun( pioreq );
        return;
    }

    if ( pioreq->pioreqIorunNext )
    {
        //  for now we can not conjoin two IO chains, only a singleton IOREQ
        AssertSz( fFalse, "This should never happen, if so we didn't NULL out pioreqIorunNext upon free, or something else odd ..." );
        EnqueueIORun( pioreq );     // try to recover anyway.
        return;
    }

    Assert( pioreq->m_fHasHeapReservation || pioreq->m_fCanCombineIO );
    Assert( !pioreq->m_fCanCombineIO || Postls()->iorunpool.FCanCombineWithExistingRun( pioreq ) ); //  or we've really messed up

    //  put this I/O into TLS I/O combining list

    pioreq->SetIOREQType( IOREQ::ioreqInIOCombiningList );

    IOREQ * const pioreqEvictedRun = Postls()->iorunpool.PioreqSwapAddIoreq( pioreq );

    Assert( Postls()->iorunpool.FContainsFileIoRun( pioreq->p_osf ) ); //   we should've just added something to the combining list
    
    if ( pioreqEvictedRun )
    {
        // if the IoRunPool was full, we may have evicted a run to make room for our IO ... so we must
        // handle (and not drop or lose ;) that IO run now ...

        // send the IO run off for processing (on its disk, which may not be ours!)
    
        pioreqEvictedRun->p_osf->m_posd->EnqueueIORun( pioreqEvictedRun );
    }

}


//  This enqueues any deferred (in the TLS) IO run(s) into the IO heap.

void COSDisk::EnqueueDeferredIORun( _In_ const _OSFILE * const p_osf )
{
    // drain the thread local I/O combining list(s) for the specified file or all files (if p_osf == NULL)

    IOREQ * pioreqT = NULL;
    while( pioreqT = Postls()->iorunpool.PioreqEvictFileRuns( p_osf ) )
    {
        // send the IO run off for processing (on its disk, which may not be ours!)
    
        pioreqT->p_osf->m_posd->EnqueueIORun( pioreqT );
    }

    // since we're the only thread to add items to our TLS, this is a safe check.

    if( p_osf )
    {
        Assert( !Postls()->iorunpool.FContainsFileIoRun( p_osf ) );
    }
    else
    {
        Assert( Postls()->iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    }
    Assert( NULL == Postls()->iorunpool.PioreqEvictFileRuns( p_osf ) ); // note slightly dangerous in that it would lose an IO (if it were there, but debug only, so no biggie ...) ...
}


ERR COSDisk::ErrBeginConcurrentIo( const BOOL fForegroundSyncIO )
{
    ERR err = JET_errSuccess;

    while ( true ) //  fForegroundSyncIO && !fAcquired && err == JET_errSuccess )
    {
        m_critIOQueue.Enter();
        if ( !m_fExclusiveIo )
        {
            if ( fForegroundSyncIO )
            {
                IncCioForeground();
            }
            else
            {
                IncCioDispatching();
            }
            break;
        }
        else
        {
            if ( !fForegroundSyncIO )
            {
                err = ErrERRCheck( errDiskTilt );
                break;
            }
        }
        m_critIOQueue.Leave();
        Assert( fForegroundSyncIO );
        UtilSleep( 2 );
    }

    Assert( !m_fExclusiveIo || err == errDiskTilt );    // exclusive IO locked out til we're done (unless we failed )

    m_critIOQueue.Leave();

    Assert( err != errDiskTilt || !fForegroundSyncIO ); // foreground IO doesn't fail

    return err;
}

void COSDisk::EndConcurrentIo( const BOOL fForegroundSyncIO, const BOOL fWrite )
{
    if ( fForegroundSyncIO )
    {
        DecCioForeground();
    }
    else
    {
        DecCioDispatching();

        if ( fWrite )
        {
            Assert( FIOThread() ); // "locking"
            m_cioAsyncWriteDispatching--;
            Assert( m_cioAsyncWriteDispatching >= 0 );
        }
        else
        {
            Assert( FIOThread() ); // "locking"
            m_cioAsyncReadDispatching--;
            Assert( m_cioAsyncReadDispatching >= 0 );
        }
    }
    
}

void COSDisk::BeginExclusiveIo()
{
    Assert( !FIOThread() );

    const ULONG cmsecIoWait = 10;

    ULONG cSleepWaits = 0;
    BOOL fAcquired = fFalse;
    while( !fAcquired )
    {
        m_critIOQueue.Enter();
        if ( !m_fExclusiveIo )
        {
            fAcquired = fTrue;
            m_fExclusiveIo = fTrue;
        }
        m_critIOQueue.Leave();
        cSleepWaits++;
        UtilSleep( cmsecIoWait );
    }
    Assert( m_fExclusiveIo == fTrue );
    //  wait for background and foreground IO to quiesce ...
    while( CioDispatched() || CioForeground() )
    {
        cSleepWaits++;
        UtilSleep( cmsecIoWait );
    }
    Assert( CioDispatched() == 0 );
    Assert( CioForeground() == 0 );
    Assert( m_fExclusiveIo == fTrue );

    //  We use this path during Re-attach DB (from recovery KeepDbAttached), so we want an early warning
    //  if we're likely stalling out Attach operations.
    AssertTrack( cSleepWaits * cmsecIoWait <= 2 * 60 * 1000, "ExclusiveIoAcquireTooLong" );
}

void COSDisk::EndExclusiveIo()
{
    Assert( CioDispatched() == 0 );
    Assert( CioForeground() == 0 );
    Assert( m_fExclusiveIo == fTrue );
    C_ASSERT( sizeof(LONG) == sizeof(BOOL) );
    OnDebug( BOOL fPre = ) AtomicCompareExchange( (LONG*)&m_fExclusiveIo, fTrue, fFalse );
    Assert( fPre == fTrue );
}

void COSDisk::SuspendDiskIo()
{
    //  we can accomplish this by pretending need exclusive IO access.
    BeginExclusiveIo();
}

void COSDisk::ResumeDiskIo()
{
    EndExclusiveIo();
}

ERR COSDisk::ErrDequeueIORun(
    __inout COSDisk::QueueOp *      pqop
    )
{

    Assert( pqop );
    Assert( pqop->FValid() );
    Assert( pqop->FEmpty() );

    //  Grab the right to do an IO (a background / async IO)

    ERR err = ErrBeginConcurrentIo( fFalse );
    CallR( err );

    //  Retrieve an IO (for issue) from the Q

    m_pIOQueue->ExtractOp( this, pqop );

    Assert( pqop->FValid() );
    Assert( !pqop->PioreqOp() || ( NULL == pqop->PioreqOp()->pioreqIorunNext ) /* special ops should be 1 IOREQ long */ );

    Expected( pqop->FEmpty() ||
                pqop->PioreqOp() != NULL ||
                pqop->PiorunIO()->PioreqHead()->fWrite ||
                pqop->PiorunIO()->PioreqHead()->FUseMetedQ() ||
                !pqop->FUseMetedQ() );

    //  It would have been nice to increase these in ErrBeginConcurrentIo, but that does a inc
    //  before we know if the Op we're pulling is a read or write, and we need this fact.

    IncCioAsyncDispatching( !pqop->FEmpty() /* count empty as read */ && pqop->FWrite() );

    //  If it turns out we tried to extract something from the IOQueue and we got an empty
    //  operations, now "End" the IO and bail out.

    if ( pqop->FEmpty() )
    {
        EndConcurrentIo( fFalse, !pqop->FEmpty() && pqop->FWrite() );
        return ErrERRCheck( errDiskTilt );
    }

    m_cioDequeued++;

    if ( pqop->P_osfPerfctrUpdates() )
    {
        Expected( pqop->CbRun() > 0 ); // should be real run
        PERFOpt( pqop->P_osfPerfctrUpdates()->pfpapi->SetCurrentQueueDepths(
                    m_pIOQueue->CioMetedReadQueue(),
                    CioAllowedMetedOps( m_pIOQueue->CioMetedReadQueue() ),
                    m_cioAsyncReadDispatching ) );
    }

    if ( pqop->PioreqOp() == NULL &&
            !pqop->FWrite() && 
            pqop->FUseMetedQ() )
    {
        // We want to only track true read-IO meted Q entries
        Expected( !pqop->PiorunIO()->PioreqHead()->fWrite );
        m_hrtLastMetedDispatch = HrtHRTCount();
    }

    Assert( (LONG)m_cioDispatching == ( m_cioAsyncReadDispatching + m_cioAsyncWriteDispatching ) );

#ifdef DEBUG
    const IOREQ * pioreqT = pqop->PioreqOp() ? pqop->PioreqOp() : pqop->PiorunIO()->PioreqHead();
    while ( pioreqT )
    {
        Assert( pioreqT->FRemovedFromQueue() );
        Assert( pioreqT != pioreqT->pioreqIorunNext );
        pioreqT = pioreqT->pioreqIorunNext;
    }
#endif

    return JET_errSuccess;
}

void COSDisk::IncCioDispatching()
{
    AtomicIncrement( (LONG*)&m_cioDispatching );
    Assert( m_cioDispatching > 0 );
}

void COSDisk::DecCioDispatching()
{
    AtomicDecrement( (LONG*)&m_cioDispatching );
    Assert( m_cioDispatching >= 0 );
}

void COSDisk::IncCioAsyncDispatching( _In_ const BOOL fWrite )
{
    if ( fWrite )
    {
        Assert( FIOThread() ); // "locking"
        m_cioAsyncWriteDispatching++;
        Assert( m_cioAsyncWriteDispatching > 0 );
    }
    else
    {
        Assert( FIOThread() ); // "locking"
        m_cioAsyncReadDispatching++;
        Assert( m_cioAsyncReadDispatching > 0 );
    }
}

void COSDisk::QueueCompleteIORun( _In_ IOREQ * const pioreqHead )
{
    Assert( pioreqHead );

    if ( pioreqHead->FRemovedFromQueue() || // <-- internal ops are reported as this ...
            pioreqHead->FIssuedAsyncIO() )
    {
        pioreqHead->p_osf->m_posd->EndConcurrentIo( fFalse, pioreqHead->fWrite );
        Assert( m_cioDispatching >= 0 );

        const _OSFILE * const p_osf = pioreqHead->p_osf;
        if ( p_osf )
        {
            PERFOpt( p_osf->pfpapi->SetCurrentQueueDepths(
                        CioMetedReadQueue(),
                        CioAllowedMetedOps( CioMetedReadQueue() ),
                        m_cioAsyncReadDispatching ) );
        }
    }
    // else it might've been a "sync" IO, or set file size, or zeroing/extending IO ...

    IOREQ * pioreqT = pioreqHead;
    while ( pioreqT )
    {
        //  Validate this set of IOREQs is consistent with each other ...

        AssertRTL( pioreqT->FInIssuedState() );
        AssertRTL( pioreqT->Ioreqtype() == pioreqHead->Ioreqtype() ||
                    // this part can happen b/c we only set the pioreqHead to an outstanding OS operation ioreqtype
                    pioreqT->FRemovedFromQueue() );
        AssertRTL( pioreqT->p_osf == pioreqHead->p_osf );
        AssertRTL( pioreqT->fWrite == pioreqHead->fWrite );
        if ( pioreqT->pioreqIorunNext )
        {
            if ( pioreqT->fWrite /* read gapping causes us to be less strict in our check */ )
            {
                AssertRTL( pioreqT->IbBlockMax() == pioreqT->pioreqIorunNext->ibOffset );
            }
            else
            {
                AssertRTL( pioreqT->IbBlockMax() <= pioreqT->pioreqIorunNext->ibOffset );
            }
        }

        //  Complete the IOREQ

        pioreqT->CompleteIO( IOREQ::CompletedIO );

        //  Increment to next IOREQ in this IO

        Assert( pioreqT != pioreqT->pioreqIorunNext );
        pioreqT = pioreqT->pioreqIorunNext;
    }
}

BOOL COSDisk::FQueueCompleteIOREQ(
    _In_ IOREQ * const  pioreq
    )
{
    //  Release the Queue reservations

    const BOOL fFreedIoQuota = m_pIOQueue->FReleaseQueueSpace( pioreq );

    Assert( !pioreq->m_fHasHeapReservation );
    Assert( !pioreq->m_fHasBackgroundReservation );
    Assert( !pioreq->m_fHasUrgentBackgroundReservation );

    //  Reset whether can combine IO
    
    pioreq->m_fCanCombineIO = fFalse;

    return fFreedIoQuota;
}

INLINE LONG COSDisk::CioDispatched() const
{
    return m_cioDispatching;
}

INLINE LONG COSDisk::CioUrgentEnqueued() const
{
    return m_pIOQueue->CioreqIOHeap() + m_pIOQueue->CioVIPList() + m_pIOQueue->CioWriteQueue();
}

LONG g_cioConcurrentMetedOpsMax = 2;
LONG g_cioLowQueueThreshold = 10;
void FlightConcurrentMetedOps( INT cioOpsMax, INT cioLowThreshold, TICK dtickStarvation )
{
    if ( cioOpsMax > 0 )
    {
        g_cioConcurrentMetedOpsMax = cioOpsMax;
    }
    g_cioLowQueueThreshold = cioLowThreshold;
    if ( dtickStarvation > 9 /* nothing below 10 ms makes sense - might as well turn off whole Meted Q at that point */ )
    {
        g_dtickStarvedMetedOpThreshold = dtickStarvation;
    }
}
    
INLINE LONG COSDisk::CioAllowedMetedOps( _In_ const LONG cioWaitingQ ) const
{
    Assert( FIOThread() ); // "locking" for m_cioAsyncReadDispatching (ALSO FIOThread() set in COSDisk::Dump() debugger extension briefly).

    //  As a first guess - we don't want to serialize fast SSD ops, so allow 4 x those, and
    //  also respond a little to back pressure of a large meted queue.


    return !FSeekPenalty() ?
            m_cioOutstandingMax /* effectively unlimited for SSDs */ :
            ( cioWaitingQ < g_cioLowQueueThreshold ?
                1 : g_cioConcurrentMetedOpsMax );
}

INLINE LONG COSDisk::CioReadyMetedEnqueued() const
{
    Assert( FIOThread() ); // "locking" for m_cioAsyncReadDispatching

    const LONG cioWaitingQ = m_pIOQueue->CioMetedReadQueue();
    const LONG cioAllowed = CioAllowedMetedOps( cioWaitingQ );
    const LONG cioOpenSlots = ( m_cioAsyncReadDispatching > cioAllowed ) ? 0 : ( cioAllowed - m_cioAsyncReadDispatching );

    return min( cioWaitingQ, cioOpenSlots );
}

INLINE LONG COSDisk::CioAllEnqueued() const
{
    return m_pIOQueue->CioreqIOHeap() + m_pIOQueue->CioVIPList() + m_pIOQueue->CioMetedReadQueue() + m_pIOQueue->CioWriteQueue();
}

#ifdef DEBUG
INLINE LONG COSDisk::CioOutstanding() const
{
    //  This is an all up accounting for background "forms" of outstanding IO against 
    //  the disk, today this includes:
    //      - IO Enqueued
    //          - in the IO Heap A or B.
    //          - in the VIP List.
    //          - in the Meted Q.
    //      - IO Dispatched to the OS (from IO thread).
    //
    //  Does NOT include "semi-sync" / foreground IO, use CioForeground() for that.
    //
    //  We should proably include IO pending enqueue (such as those build up in TLS 
    //  at some point).
    //
    //  Note: We should also point out it is impossible for this to be 100% accurate
    //  without getting an interlock with the IO Thread, which we do not today.  It 
    //  can be +1 too high for a moment (while taking IO from queue) before it has
    //  been dispatched to OS.
    return CioAllEnqueued() + CioDispatched();
}
#endif

void COSDisk::TrackOsFfbBegin( const IOFLUSHREASON iofr, const QWORD hFile )
{
    if ( !( iofrOsFakeFlushSkipped & iofr ) )
    {
        /*OnDebug( LONG lAfter = )*/ AtomicIncrement( (LONG*)&m_cFfbOutstanding );
    }
    ETDiskFlushFileBuffersBegin( m_dwDiskNumber, hFile, iofr );
}

void COSDisk::TrackOsFfbComplete( const IOFLUSHREASON iofr, const DWORD error, const HRT hrtStart, const QWORD usFfb, const LONG64 cioreqFileFlushing, const WCHAR * const wszFileName )
{
    /*OnDebug( LONG lAfter = )*/ AtomicDecrement( (LONG*)&m_cFfbOutstanding );

    ETDiskFlushFileBuffers( m_dwDiskNumber, wszFileName, iofr, cioreqFileFlushing, usFfb, error );

    OSTrace( JET_tracetagFlushFileBuffers, OSFormat( "COSDisk::Flush[%8x] Res: %d Delta: +%8.3f +(%8.3f op) - %I64d - %ws",
                (DWORD)iofr,
                error,
                (float)CusecHRTFromDhrt( hrtStart - m_hrtLastFfb ) / 1000.0,
                (float)usFfb / 1000.0,
                cioreqFileFlushing,
                wszFileName ) );

    m_hrtLastFfb = HrtHRTCount();
}

// Refreshes the physical disk performance information if necessary

INLINE VOID COSDisk::RefreshDiskPerformance()
{
    if ( TickOSTimeCurrent() - m_tickPerformanceLastMeasured >= s_dtickPerformancePeriod )
    {
        QueryDiskPerformance();
    }
}

// Queries the performance of the physical disk

VOID COSDisk::QueryDiskPerformance()
{
    DISK_PERFORMANCE diskPerformance;
    DWORD dwSize;
    if (    m_hDisk != INVALID_HANDLE_VALUE &&
            DeviceIoControl(    m_hDisk,
                                IOCTL_DISK_PERFORMANCE,
                                NULL,
                                0,
                                &diskPerformance,
                                sizeof( diskPerformance ),
                                &dwSize,
                                NULL ) &&
            sizeof( diskPerformance ) >= dwSize )
    {
        m_cioOsQueueDepth = diskPerformance.QueueDepth;
    }

    m_tickPerformanceLastMeasured = TickOSTimeCurrent();
}

// Returns the actual physical / OS-level disk queue depth (not internal OSDisk level)

INLINE DWORD COSDisk::CioOsQueueDepth()
{
    RefreshDiskPerformance();

    return m_cioOsQueueDepth;
}

//  prepares the I/O run for issuing to the O.S., sets up the overlapped structure, calculates OS
//  IO priority, sets the scatter/gather list pointers, etc.

void COSDisk::IORun::PrepareForIssue(
    _In_ const IOREQ::IOMETHOD              iomethod,
    _Out_ DWORD * const                     pcbRun,
    _Out_ BOOL * const                      pfIOOSLowPriority,
    __inout PFILE_SEGMENT_ELEMENT const     rgfse,
    _In_ const DWORD                        cfse
    )
{
    IOREQ * const pioreqHead    = m_storage.Head();

    Assert( pioreqHead );

    //  setup embedded OVERLAPPED structure in the head IOREQ for the I/O

    Assert( IbOffset() == pioreqHead->ibOffset );
    QWORD ibOffset = pioreqHead->ibOffset;
    pioreqHead->ovlp.Offset     = ULONG( ibOffset );
    pioreqHead->ovlp.OffsetHigh = ULONG( ibOffset >> 32 );
    *pcbRun                     = CbRun();

    //  setup the coallesced IOREQs for I/O, and setup the scatter / gather information

    *pfIOOSLowPriority = qosIOOSLowPriority & pioreqHead->grbitQOS;

    if ( IOREQ::iomethodScatterGather == iomethod )
    {
        Assert( cfse && rgfse );
        //  Note this sets fCoalesced on all IOREQs past pioreqHead ...
        OSDiskIIOPrepareScatterGatherIO( pioreqHead, *pcbRun, cfse, rgfse, pfIOOSLowPriority );
    }
    else
    {
        Assert( pioreqHead->pioreqIorunNext == NULL );
    }

    //  begin I/O for all IOREQs

    Assert( pioreqHead->m_grbitHungActionsTaken == 0 );

    const HRT hrtNow = HrtHRTCount();
    for ( IOREQ* pioreq = pioreqHead; pioreq != NULL; pioreq = pioreq->pioreqIorunNext )
    {
        pioreq->BeginIO( iomethod, hrtNow, pioreq == pioreqHead );
    }
}


//  Determines if this I/O hit a temporary resource issue that may cleanup later

INLINE bool FIOTemporaryResourceIssue( ERR errIO )
{
    return errIO == JET_errOutOfMemory;
}

//  Determines if this I/O method does not work on this file

INLINE bool FIOMethodTooComplex(
    _In_ const IOREQ::IOMETHOD      iomethodCurrentFile,
    _In_ const IOREQ::IOMETHOD      iomethodIO,
    _In_ const ERR                  errIO )
{
    return iomethodCurrentFile >= iomethodIO &&     // current IO method is the one used or better
            iomethodIO > IOREQ::iomethodSync && // IO method is still degradable
            errIO == JET_errInvalidParameter;       // IO method did not actually work out
}

//  Handles the IO results from an IO issue attempt ... 
//  Returns true if too many IOs and we need start servicing completions

BOOL FIOMgrHandleIOResult(
    _In_ const IOREQ::IOMETHOD      iomethod,
    __inout IOREQ *                 pioreqHead,
    _In_ BOOL                       fIOCompleted,
    _In_ DWORD                      error,
    _In_ DWORD                      cbTransfer
    )
{
    BOOL fReIssue = fFalse;

    if ( fIOCompleted )
    {
        //  the issue succeeded and completed immediately

        Assert( IOREQ::iomethodSync == iomethod || IOREQ::iomethodSemiSync == iomethod );
        OSDiskIIOThreadCompleteWithErr( ERROR_SUCCESS, cbTransfer, pioreqHead );
    }
    else
    {
        //  either the I/O was issued and is pending or there was an error

        const ERR errIO = ErrOSFileIFromWinError( error );

        //  validate a constraint ...

        if ( errIO >= 0 )
        {
            //  the I/O is pending

            //  the I/O completion will be posted to this thread later

            //  WARNING! WARNING!
            //  this code currently relies on the I/O completion being posted
            //  to this thread (and therefore the IOREQ not having been freed yet)
            //
            PERFOptDeclare( IFilePerfAPI * const    pfpapi  = pioreqHead->p_osf->pfpapi );
            PERFOpt( pfpapi->IncrementIOAsyncPending( pioreqHead->fWrite ) );
        }
        else if ( FIOTemporaryResourceIssue( errIO ) &&
                    //  This turns exclusive IO ops into OOM errors ...
                    !FExclusiveIoreq( pioreqHead ) )
        {
            //  we issued too many I/Os

#ifndef RTM
            pioreqHead->m_fOutOfMemory = fTrue;
#endif
            pioreqHead->m_cRetries = min( IOREQ::cRetriesMax, pioreqHead->m_cRetries + 1 );
            //  return run to the I/O Heap so that we can try issuing it again later
            Assert( pioreqHead->p_osf && pioreqHead->p_osf->m_posd );
            pioreqHead->p_osf->m_posd->EnqueueIORun( pioreqHead );

            //  stop issuing I/O and re-issue any re-enqueued I/Os later ...
            fReIssue = fTrue;
        }
        else if ( FIOMethodTooComplex(  pioreqHead->p_osf->iomethodMost, iomethod, errIO ) &&
                    //  I don't thinkt his can happen on semi-sync, but and just in case (which would then turns exclusive IO ops 
                    //  into generic invalid parameter errors! :-P ) ...
                    !FExclusiveIoreq( pioreqHead ) )
        {
            Assert( iomethod != IOREQ::iomethodSemiSync );

            //  This I/O method does not work on this file
            //
            //  This is not an uncommon case, when FILE_FLAG_NO_BUFFERING is not specified (such as 
            //  in accept isambasic - small config) we fall down to Async IO ...

#ifndef RTM
            pioreqHead->m_cTooComplex = min( 0x6, pioreqHead->m_cTooComplex + 1 );
#endif
            pioreqHead->m_cRetries = min( IOREQ::cRetriesMax, pioreqHead->m_cRetries + 1 );

            //  reduce I/O capability for this file
            pioreqHead->p_osf->iomethodMost = IOREQ::IOMETHOD( pioreqHead->p_osf->iomethodMost - 1 );
            AssertRTL( pioreqHead->p_osf->iomethodMost >= 0 );
            //  if we degrade this much, we'll probably be in serious trouble ... as our IO thread
            //  would degrade to sync IO, so it would be bottlenecked as if there was a single disk
            //  in the system (in-spite of having separate queues).
            AssertRTL( IOREQ::iomethodSync != pioreqHead->p_osf->iomethodMost );
            AssertRTL( IOREQ::iomethodSemiSync != pioreqHead->p_osf->iomethodMost );

            //  return run to the I/O Heap so that we can try issuing it again later
            Assert( pioreqHead->p_osf && pioreqHead->p_osf->m_posd );
            pioreqHead->p_osf->m_posd->EnqueueIORun( pioreqHead );

            fReIssue = fTrue;
        }
        else
        {
            //  some other fatal error occurred

            //  complete the I/O with the error
            OSDiskIIOThreadCompleteWithErr( error, 0, pioreqHead );
        }
    }

    return fReIssue;
}

QWORD g_cSplitAndIssueRunning = fFalse;

BOOL FIOMgrSplitAndIssue(
    _In_ const IOREQ::IOMETHOD              iomethod,
    __inout COSDisk::IORun *                piorun
    )
{
    BOOL fReIssue = fFalse;

    //  Note: This code path is hit by ViewCache, because you can't do scatter/gather IO on view
    //  mapped files for reads.
    AtomicAdd( &g_cSplitAndIssueRunning, 1 );

    IOREQ * const pioreqHead = piorun->PioreqGetRun();
    piorun = NULL;  // ensure no deref, IORun is not valid from this point on.
    OnDebug( const _OSFILE * const p_osfHead = pioreqHead->p_osf );

    Assert( pioreqHead );
    Assert( pioreqHead->m_fHasHeapReservation );
    Assert( iomethod != IOREQ::iomethodScatterGather );

    CLocklessLinkedList< IOREQ > IOChain( pioreqHead );
    Assert( !IOChain.FEmpty() );
    
    IOREQ * pioreq;
    while ( pioreq = IOChain.RemovePrevMost( OffsetOf( IOREQ, pioreqIorunNext ) ) )
    {
        Assert( NULL == pioreq->pioreqIorunNext );
        Assert( pioreq->FRemovedFromQueue() );
        
        //  We need to snap off the heap reservation in case this IO is successful
        //  but the next / rest of the IO run gets something like out of memory.
        const BOOL fHasHeapReservation = pioreq->m_fHasHeapReservation;
        Assert( fHasHeapReservation );
        if ( !IOChain.FEmpty() && !IOChain.Head()->m_fHasHeapReservation /* next IO has no reservation */ )
        {
            //  More IOREQs that may need heap reservation ...
            pioreq->m_fHasHeapReservation = fFalse;
            IOChain.Head()->m_fHasHeapReservation = fHasHeapReservation;
            //  Note: We don't need to m_fHasBackgroundReservation because it is only used
            //  for approximate quotas, not correctness.
        }

        if ( pioreq != pioreqHead )
        {
            Assert( pioreq->p_osf->m_posd->CioDispatched() >= 1 );

            //  Since we are spliting our IO run up, we need to increment the dispatched IO ...
            //  Note: not calling ErrBeginConcurrentIo(), so offending the desire for exclusive IO, but is limited case
            //  and should be safe b/c we're already dispatching one IO.
            pioreq->p_osf->m_posd->IncCioDispatching();

            pioreq->p_osf->m_posd->IncCioAsyncDispatching( pioreq->fWrite );

            Assert( p_osfHead == pioreq->p_osf );
            const _OSFILE * const p_osf = pioreq->p_osf;
            if ( p_osf )
            {
                PERFOpt( p_osf->pfpapi->SetCurrentQueueDepths(
                            pioreq->p_osf->m_posd->CioMetedReadQueue(),
                            pioreq->p_osf->m_posd->CioAllowedMetedOps( pioreq->p_osf->m_posd->CioMetedReadQueue() ),
                            pioreq->p_osf->m_posd->CioAsyncReadDispatching() ) );
            }
        }

        OnNonRTM( pioreq->m_fSplitIO = fTrue );

        COSDisk::IORun  iorunSplit;
        const BOOL fAccepted = iorunSplit.FAddToRun( pioreq );
        Assert( fAccepted );

        //
        //  Issue the actual I/O
        //

        DWORD cbTransfer    = 0;
        BOOL  fIOCompleted  = fFalse;

        const DWORD errorIO = ErrorIOMgrIssueIO( &iorunSplit, iomethod, NULL, 0, &fIOCompleted, &cbTransfer );
        //  iorunSplit is invalid after this call

        const ERR errIO = ErrOSFileIFromWinError( errorIO );

        //
        //  Handle the result
        //

        const BOOL fReEnqueue = FIOTemporaryResourceIssue( errIO ) ||
                                    FIOMethodTooComplex( pioreq->p_osf->iomethodMost, iomethod, errIO );

        if ( fReEnqueue )
        {
            if ( !IOChain.FEmpty() )
            {
                Assert( IOChain.Head()->m_fHasHeapReservation );
                if ( !pioreq->m_fHasHeapReservation )
                {
                    //  the new head will not have a heap reservation, move the heap 
                    //  reservation that must exist on the split head.
                    Assert( IOChain.Head()->m_fHasHeapReservation );
                    IOChain.Head()->m_fHasHeapReservation = fFalse;
                    pioreq->m_fHasHeapReservation = fTrue;
                }
            }
            else
            {
                Assert( pioreq->m_fHasHeapReservation );    // should not have stripped heap res if last IOREQ in IOChain/run
            }
            OnNonRTM( pioreq->m_fReMergeSplitIO = fTrue );

            IOChain.AtomicInsertAsPrevMost( pioreq, OffsetOf(IOREQ,pioreqIorunNext) );
            Assert( IOChain.Head()->m_fHasHeapReservation );
            Assert( IOChain.Head() == pioreq );
        }

        fReIssue = FIOMgrHandleIOResult( iomethod, pioreq, fIOCompleted, errorIO, cbTransfer );

        AssertRTL( fReEnqueue == fReIssue );

        if ( fReEnqueue )
        {
            //  We hit an issue that caused us to re-enqueue, give up on this IO run for now ...
            break;
        }

    }

    return fReIssue;
}

BOOL FIOMgrIssueIORunRegular(
    _In_ const IOREQ::IOMETHOD          iomethod,
    __inout COSDisk::IORun *            piorun
    )
{
    //
    //  prepare all IOREQs for I/O
    //
    //  allocate the scatter/gather array argument if necessary
    DWORD cfse = 0;
    DWORD cbfse = 0;
    PFILE_SEGMENT_ELEMENT rgfse = NULL;
    BOOL fAllocatedFromHeap = fFalse;
    USHORT cbStackAllocMax = 4096;

    if ( IOREQ::iomethodScatterGather == iomethod )
    {
        //  allocating one extra for NULL terminating the list.
        cfse = ( piorun->CbRun() + OSMemoryPageCommitGranularity() - 1 ) / OSMemoryPageCommitGranularity() + 1;
        cbfse = sizeof( *rgfse ) * cfse;

        //  allocate from stack if the cbfse <=(192K/4K)+1)*8 = 392 in debug / 4K in retail 
        //  otherwise allocating from heap 
        OnDebug( cbStackAllocMax = 392 );
        if ( cbfse <= cbStackAllocMax )
        {
            Assert( !fAllocatedFromHeap );
            rgfse = ( PFILE_SEGMENT_ELEMENT )_alloca( cbfse );
        }
        else
        {
            rgfse = ( ErrFaultInjection( 17370 ) < JET_errSuccess ) ?
                ( PFILE_SEGMENT_ELEMENT )NULL:
                ( PFILE_SEGMENT_ELEMENT )PvOSMemoryHeapAllocAlign( cbfse, sizeof( FILE_SEGMENT_ELEMENT ) );
            if ( rgfse == NULL )
            {
                //  We have a full IO run, but could not alloc enough mem to even do scatter/gather IO right now,
                //  so we pull the IO run apart and issue each piece separately.
                return FIOMgrSplitAndIssue( static_cast<const IOREQ::IOMETHOD>( iomethod - 1 ), piorun );
            }
            fAllocatedFromHeap = fTrue;
        }
        Assert( 0 == ( UINT_PTR )rgfse % sizeof( FILE_SEGMENT_ELEMENT ) );
    }

    //
    //  Issue the actual I/O
    //

    DWORD cbTransfer    = 0;
    BOOL  fIOCompleted  = fFalse;
    IOREQ* pioreqHead   = NULL;

    const DWORD errorIO = ErrorIOMgrIssueIO( piorun, iomethod, rgfse, cfse, &fIOCompleted, &cbTransfer, &pioreqHead );
    piorun = NULL;  //  the IORun is invalid after this call
    
    //
    //  Handle the result
    //
    
    if( fAllocatedFromHeap )
    {
        Assert( cbfse > cbStackAllocMax );
        OSMemoryHeapFreeAlign( rgfse );
    }
    return FIOMgrHandleIOResult( iomethod, pioreqHead, fIOCompleted, errorIO, cbTransfer );
}

//  This either issues a sync IO directly on this thread if it can, or if
//  it gets OutOfMemory, it enqueues it to the IO heap and issues it 
//  asynchronously.  Either way the IO is either completed with success,
//  or with an error (other than OutOfMemory), or will be issued soon 
//  from the background thread when this function exits.

VOID IOMgrIssueSyncIO( IOREQ * pioreqSingle )
{
    //
    //  Grab the right to do IO.
    //

    const BOOL fExclusiveIo = FExclusiveIoreq( pioreqSingle ); // should not change, but just to be sure ...
    if ( fExclusiveIo )
    {
        Assert( !FIOThread() );
        pioreqSingle->p_osf->m_posd->BeginExclusiveIo();
        if ( ( pioreqSingle->grbitQOS & qosIOReadCopyMask ) != qosIOReadCopyTestExclusiveIo )
        {
            (VOID)pioreqSingle->p_osf->ErrSetReadCopyNumber( IReadCopyFromQos( pioreqSingle->grbitQOS ) );
        }
    }
    else
    {
        CallS( pioreqSingle->p_osf->m_posd->ErrBeginConcurrentIo( fTrue ) );
    }

    //
    //  prepare the IOREQ for I/O
    //

    COSDisk::IORun iorunSingle;
    const BOOL fAccepted = iorunSingle.FAddToRun( pioreqSingle );
    Assert( fAccepted );

    //
    //  Issue the actual I/O
    //

    pioreqSingle->m_iocontext = pioreqSingle->p_osf->pfpapi->IncrementIOIssue( pioreqSingle->p_osf->m_posd->CioOsQueueDepth(), pioreqSingle->fWrite );

    DWORD cbTransfer    = 0;
    BOOL  fIOCompleted  = fFalse;

    const DWORD errorIO = ErrorIOMgrIssueIO( &iorunSingle, IOREQ::iomethodSemiSync, NULL, 0, &fIOCompleted, &cbTransfer );
    //  iorunSingle is invalid after this call

    Assert( pioreqSingle->FIssuedSyncIO() );

    if ( fExclusiveIo )
    {
        if ( ( pioreqSingle->grbitQOS & qosIOReadCopyMask ) != qosIOReadCopyTestExclusiveIo )
        {
            (VOID)pioreqSingle->p_osf->ErrSetReadCopyNumber( -1 );
        }
        pioreqSingle->p_osf->m_posd->EndExclusiveIo();
    }
    else
    {
        pioreqSingle->p_osf->m_posd->EndConcurrentIo( fTrue, pioreqSingle->fWrite );
    }

    //
    //  Handle the result
    //

    const BOOL fNeedIssue = FIOMgrHandleIOResult( IOREQ::iomethodSemiSync, pioreqSingle, fIOCompleted, errorIO, cbTransfer );
    // Exclusive IO cannot be reissued
    Assert( !fExclusiveIo || !fNeedIssue );

    //  NOTE: Can not reference pioreqSingle after this.

    //
    //  Re-Issue if needed
    //

    // Technically this is not needed because both sync ErrIORead() / ErrIOWrite() 
    // call COSFile::ErrIOIssue(), but I like to have it hear to match the other /
    // similar functions (FIOMgrIssueIORunRegular / FIOMgrSplitAndIssue) which all
    // return a bool to ensure someone restarts issue.
    if ( fNeedIssue )
    {
        // we now allow an deferred iorun to be built (on TLS) concurrent while synchronous 
        // IOs for can be issued... so we do not check for empty iorunpool.
        
        // tell I/O thread to issue from I/O heap to FS
        
        // note: ideally we'd pass in pioreqSingle->p_osf, but we can't reference the pioreqSingle.  We 
        // may be able to cache the p_osf and pass that in.  Since it's a sync IO we can't lose the reference
        // to it until we return I believe.  But it isn't important to optimize the OOM case.
        
        OSDiskIOThreadStartIssue( NULL );
    }
}

void IOMgrCompleteOp(
    __inout IOREQ * const       pioreq
    )
{

    Assert( pioreq );
    Assert( pioreq->cbData == 0 );  // these special ops always have a zero cbData ...

    QWORD ibOffset = pioreq->ibOffset;
    pioreq->ovlp.Offset         = ULONG( ibOffset );
    pioreq->ovlp.OffsetHigh     = ULONG( ibOffset >> 32 );

    //  if this is a zero sized I/O then complete it immediately w/o calling the
    //  OS to avoid the overhead and ruining our I/O stats from the OS perspective

    OSDiskIIOThreadCompleteWithErr( ERROR_SUCCESS, 0, pioreq );
}

//  determine I/O method for this run

IOREQ::IOMETHOD COSDisk::IORun::IomethodGet( ) const
{
    const IOREQ * pioreqHead = m_storage.Head();
    IOREQ * pioreqT = m_storage.Head();

    IOREQ::IOMETHOD iomethodBest = FMultiBlock() ?
                            IOREQ::iomethodScatterGather :
                            IOREQ::iomethodAsync;

    while ( pioreqT )
    {
        Assert( pioreqHead->p_osf == pioreqT->p_osf );
        Assert( pioreqHead->fWrite == pioreqT->fWrite );

        if ( !FOSDiskIFileSGIOCapable( P_OSF() )    ||                          // file handle can do SGIO
            !FOSDiskIDataSGIOCapable( pioreqT->pbData, pioreqT->cbData ) )      // new data would be accepted by the OS SGIO API
        {
            iomethodBest = IOREQ::iomethodAsync;
            break;
        }

        pioreqT = pioreqT->pioreqIorunNext;
    }

    return iomethodBest;
}

ERR ErrIOMgrIssueIORun(
    __inout COSDisk::IORun *            piorun
    )
{
    BOOL fReIssue = fFalse;
    
    //  validate some post IO run extraction info ...

    Assert( piorun );
    ASSERT_VALID( piorun );
    Assert( piorun->CbRun() != 0 );

    //  determine I/O method for this run

    IOREQ::IOMETHOD iomethod = min( piorun->P_OSF()->iomethodMost,
                                            piorun->IomethodGet() );

    // drop iomethod if we aren't registered for completion ports ...
    if ( !piorun->P_OSF()->fRegistered )
    {
        iomethod = IOREQ::iomethodSemiSync;
    }

    Assert( iomethod <= IOREQ::iomethodScatterGather );
    // single combined issue and completion thread makes this Assert() concurrent safe.
    Assert( iomethod <= piorun->P_OSF()->iomethodMost );
    Assert( piorun->CbRun() != 0 || iomethod != IOREQ::iomethodScatterGather );

    if ( IOREQ::iomethodScatterGather != iomethod && piorun->FMultiBlock() )
    {
        //  We have a full IO run, but we do not support scatter / gather IO right now
        //  so we need to pull the IO run apart and issue each piece separately.

        fReIssue = FIOMgrSplitAndIssue( iomethod, piorun );
    }
    else
    {
        //  Single IOREQ or IO run and we support scatter / gather IO, issue normally.

        fReIssue = FIOMgrIssueIORunRegular( iomethod, piorun );
    }


    return fReIssue ? ErrERRCheck( errDiskTilt ) : JET_errSuccess;
}

ERR ErrOSDiskProcessIO(
    _Inout_ COSDisk * const     posd,
    _In_ const BOOL             fFromCompletion
    )
{
    ERR err = JET_errSuccess; // disk serviced completely and successfully ...
    const HRT hrtIssueStart = HrtHRTCount();

    Assert( posd );

    Assert( FIOThread() );

    __int64 ipass = 0;
    if ( !fFromCompletion )
    {
        ipass = posd->IpassBeginDispatchPass();
    }
    else
    {
        ipass = posd->IpassContinuingDispatchPass();
    }

    //  issue as many I/Os as possible

    ULONG cioToGo;
    ULONG cioProcessed = 0;
    while ( cioToGo = ( ( fFromCompletion ? 0 : posd->CioUrgentEnqueued() ) + posd->CioReadyMetedEnqueued() ) )
    {
        Assert( JET_errSuccess == err );

        //
        //  Extract an IO Run from the IO Queue / Heap.
        //

        COSDisk::QueueOp    qop;

        Assert( qop.FEmpty() );

        //  note this grabs and releases IOQueue::m_pcritIoQueue for the disk (and checks we can
        //  perform concurrent IO currently).

        Call( posd->ErrDequeueIORun( &qop ) );

        //  validate some post IO run extraction info ...

        Assert( qop.FValid() );
        Assert( !qop.FEmpty() );

        if ( qop.PioreqOp() )
        {
            //  Issue the Special Op ...

            Assert( qop.PiorunIO() == NULL );
            IOMgrCompleteOp( qop.PioreqOp() );
        }

        if ( qop.PiorunIO() )
        {
            //  Issue (or Re-enqueue) the IO ...

            Assert( qop.PioreqOp() == NULL );
            cioProcessed++;
            Call( ErrIOMgrIssueIORun( qop.PiorunIO() ) );
        }

    }// while more IOs to issue ...

HandleError:
    //  if we failed to issue an I/O due to low resource conditions (or
    //  due to exclusive IO / no-concurrent IO mode) then remember to
    //  try again in a bit once we go idle

    if ( err == errDiskTilt )
    {
        OSDiskIIOThreadIRetryIssue();
    }

    ETIOThreadIssuedDisk( posd->DwDiskNumber(), (BYTE)fFromCompletion, ipass, err, cioProcessed, CusecHRTFromDhrt( HrtHRTCount() - hrtIssueStart ) );

    return err;
}

void IOMgrProcessIO()
{
    Assert( FIOThread() );

    const HRT hrtIssueStart = HrtHRTCount();

    //  For each disk issue relevant enqueued IO ...

    OSDiskEnumerator    osdisks;
    COSDisk *           posdCurr;
    ERR                 errDisk = errCodeInconsistency;
    ULONG               cDisksProcessed = 0;

    while ( posdCurr = osdisks.PosdNext() )
    {

        Assert( posdCurr->CRef() >= 1 );

        errDisk = ErrOSDiskProcessIO( posdCurr, fFalse );
        cDisksProcessed++;

        if ( JET_errSuccess == errDisk )
        {
            //  The simple case, we've drained that disks IO queue, move to the next ...
            
        }
        else
        {
            Assert( errDiskTilt == errDisk );
        }

    }

    ETIOThreadIssueProcessedIO( errDisk, cDisksProcessed, CusecHRTFromDhrt( HrtHRTCount() - hrtIssueStart ) );
}

//  process I/O Thread Issue command

void OSDiskIIOThreadIIssue( const DWORD     dwError,
                            const DWORD_PTR dwReserved1,
                            const DWORD     dwReserved2,
                            const DWORD_PTR dwReserved3 )
{
    //  Signal IO thread is processing.

    ETIOThreadIssueStart();

    //  Mark thread as IO thread.

    Postls()->fIOThread = fTrue;

    //  Setup this thread (and stack) for processing async / mark IO ...
    
    //  Have the IO Manager process all outstanding IO ...

    IOMgrProcessIO();

    //  Unmark thread.

    Postls()->fIOThread = fFalse;
}

VOID IFilePerfAPI::Init()
{
    m_tickLastDiskFull = TickOSTimeCurrent() - dtickOSFileDiskFullEvent;
    m_tickLastAbnormalIOLatencyEvent = 0;
    m_cmsecLastAbnormalIOLatency = 0;
    m_cAbnormalIOLatencySinceLastEvent = 0;
}

//  disk full has been hit, update stats and report error if appropriate
//
BOOL IFilePerfAPI::FReportDiskFull()
{
    const TICK tickNow = TickOSTimeCurrent();
    const BOOL  fReportError = ( ( tickNow - m_tickLastDiskFull ) > dtickOSFileDiskFullEvent );
    if ( fReportError )
    {
        m_tickLastDiskFull = tickNow;
    }
    return fReportError;
}

BOOL IFilePerfAPI::FReportAbnormalIOLatency( const BOOL fWrite, const QWORD cmsecIOElapsed )
{
    BOOL fReportError = fTrue;
    const TICK tickNow = TickOSTimeCurrent();

    //  if we're planning on reporting this abnormal I/O latency,
    //  verify we're not spamming the eventlog excessively with
    //  the same error
    //
    if ( ( tickNow - m_tickLastAbnormalIOLatencyEvent ) < dtickOSFileAbnormalIOLatencyEvent
        && cmsecIOElapsed < m_cmsecLastAbnormalIOLatency * 3 / 2 )
    {
        //  if we recently reported an abnormal I/O latency and the current
        //  abnormal I/O latency is not significantly more than the previous one,,
        //  then don't bother reporting again
        //
        fReportError = fFalse;

        //  decided not to report this event, so just keep a count of
        //  how many we've skipped
        //
        AtomicIncrement( &m_cAbnormalIOLatencySinceLastEvent );
    }

    OSTrace( JET_tracetagIOProblems, OSFormat( "Long IO, %I64u msec, fReport = %d", cmsecIOElapsed, fReportError ) );

    return fReportError;
}

VOID IFilePerfAPI::ResetAbnormalIOLatency( const QWORD cmsecIOElapsed )
{
    m_tickLastAbnormalIOLatencyEvent = TickOSTimeCurrent();
    m_cmsecLastAbnormalIOLatency = cmsecIOElapsed;
    m_cAbnormalIOLatencySinceLastEvent = 0;
}


#if defined( USE_HAPUBLISH_API )
HaDbFailureTag OSDiskIIOHaTagOfErr( const ERR err, const BOOL fWrite )
{
    //  we should never report JET_errFileIOBeyondEOF because it can be a normal
    //  consequence of replaying logs (also see OSDiskIIOThreadCompleteWithErr)
    //
    Assert( err != JET_errFileIOBeyondEOF );
    
    switch ( err )
    {
        case JET_errOutOfMemory:
            return HaDbFailureTagMemory;

        case JET_errDiskReadVerificationFailure:
            return HaDbFailureTagRepairable;

        case JET_errDiskFull:
            return HaDbFailureTagSpace;

        case JET_errFileSystemCorruption:
            return HaDbFailureTagFileSystemCorruption;

        case JET_errDiskIO:
            // Page patch requests are issued for -1022 errors on read, so
            // we use a repairable tag
            // And for logs, this repairable tag is also helpful. HA can recognize the need to repair a log file
            if ( fWrite )
            {
                return HaDbFailureTagIoHard;
            }
            return HaDbFailureTagRepairable;

        default:
            return HaDbFailureTagIoHard;
    }
}
#endif

//  report I/O errors encountered on completion


VOID OSDiskIIOReportIOLatency(
    IOREQ * const   pioreqHead,
    const QWORD     cmsecIOElapsed )
{
    IFileAPI *      pfapi       = (IFileAPI*)pioreqHead->p_osf->keyFileIOComplete;  //  HACK!
    QWORD           ibOffset    = ( QWORD( pioreqHead->ovlp.Offset ) +
                                ( QWORD( pioreqHead->ovlp.OffsetHigh ) << 32 ) );
    DWORD           cbLength    = 0;
    const ULONG     cwsz        = 7;
    const WCHAR *   rgpwsz[ cwsz ];
    DWORD           irgpwsz     = 0;
    WCHAR           wszAbsPath[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszOffset[ 64 ];
    WCHAR           wszLength[ 64 ];
    WCHAR           wszTimeElapsed[ 64 ];

    if ( pioreqHead->pioreqIorunNext == NULL &&
          pioreqHead->cbData == 0 )
    {
        // we have a special NULL completion op, like file extension,
        // file zeroing, or change file size request.
        Expected( COSFile::FOSFileManagementOperation( pioreqHead ) );
        cbLength = 0;
    }
    else
    {
        cbLength = COSDisk::CbIOLength( pioreqHead );
    }

    CallS( pfapi->ErrPath( wszAbsPath ) );
    OSStrCbFormatW( wszOffset, sizeof( wszOffset ), L"%I64i (0x%016I64x)", ibOffset, ibOffset );
    OSStrCbFormatW( wszLength, sizeof( wszLength ), L"%u (0x%08x)", cbLength, cbLength );
    OSStrCbFormatW( wszTimeElapsed, sizeof( wszTimeElapsed ), L"%I64u", cmsecIOElapsed / 1000 );

    rgpwsz[ irgpwsz++ ] = wszAbsPath;
    rgpwsz[ irgpwsz++ ] = wszOffset;
    rgpwsz[ irgpwsz++ ] = wszLength;

    WCHAR       wszPreviousAbnormalIOs[ 64 ];
    WCHAR       wszPreviousAbnormalIOEvent[ 64 ];
    MessageId   msgid;
    const TICK  tickNow     = TickOSTimeCurrent();

    //  caused problems with BVT hardware
    //
    //  AssertSz( fFalse, "I/O's are taking an abnormally long time to complete. The hardware on this machine is flaky!" );

    rgpwsz[ irgpwsz++ ] = wszTimeElapsed;

    if ( 0 == pioreqHead->p_osf->pfpapi->TickLastAbnormalIOLatencyEvent() )
    {
        //  first time this error has been encountered
        //  (well, there's actually the pathological case
        //  where the last time this error was encountered
        //  the tick counter had wrapped back to exactly
        //  zero, but we won't worry about it, you just end
        //  up getting the original eventlog message instead
        //  of the "AGAIN" eventlog message)
        //
        msgid = ( pioreqHead->fWrite ? OSFILE_WRITE_TOO_LONG_ID : OSFILE_READ_TOO_LONG_ID );
    }
    else
    {
        //  we've encountered this error before
        //
        OSStrCbFormatW( wszPreviousAbnormalIOs, sizeof( wszPreviousAbnormalIOs ), L"%d", pioreqHead->p_osf->pfpapi->CAbnormalIOLatencySinceLastEvent() );
        OSStrCbFormatW( wszPreviousAbnormalIOEvent, sizeof( wszPreviousAbnormalIOEvent ), L"%d", ( tickNow - pioreqHead->p_osf->pfpapi->TickLastAbnormalIOLatencyEvent() ) / 1000 );

        rgpwsz[ irgpwsz++ ] = wszPreviousAbnormalIOs;
        rgpwsz[ irgpwsz++ ] = wszPreviousAbnormalIOEvent;

        msgid = ( pioreqHead->fWrite ? OSFILE_WRITE_TOO_LONG_AGAIN_ID : OSFILE_READ_TOO_LONG_AGAIN_ID );
    }

    pioreqHead->p_osf->pfpapi->ResetAbnormalIOLatency( cmsecIOElapsed );

    Assert( irgpwsz <= cwsz );
    pioreqHead->p_osf->Pfsconfig()->EmitEvent(  eventWarning,
                                                PERFORMANCE_CATEGORY,
                                                msgid,
                                                irgpwsz,
                                                rgpwsz,
                                                JET_EventLoggingLevelMin );

#if defined( USE_HAPUBLISH_API )
    pioreqHead->p_osf->Pfsconfig()->EmitEvent(  HaDbFailureTagPerformance,
                                                Ese2HaId( PERFORMANCE_CATEGORY ),
                                                Ese2HaId( msgid ),
                                                irgpwsz,
                                                rgpwsz,
                                                pioreqHead->fWrite ? HaDbIoErrorWrite : HaDbIoErrorRead,
                                                wszAbsPath,
                                                ibOffset,
                                                cbLength );
#endif

}

VOID OSDiskIIOReportError(
    IOREQ * const   pioreqHead,
    const ERR       err,
    const DWORD     errSystem,
    const QWORD     cmsecIOElapsed )
{
    QWORD           ibOffset    = ( QWORD( pioreqHead->ovlp.Offset ) +
                                ( QWORD( pioreqHead->ovlp.OffsetHigh ) << 32 ) );
    DWORD           cbLength    = COSDisk::CbIOLength( pioreqHead );

    OSFileIIOReportError(
        pioreqHead->p_osf->m_posf,
        pioreqHead->fWrite,
        ibOffset,
        cbLength,
        err,
        errSystem,
        cmsecIOElapsed );
}

VOID OSFileIIOReportError(
    COSFile * const posf,
    const BOOL      fWrite,
    const QWORD     ibOffset,
    const DWORD     cbLength,
    const ERR       err,
    const DWORD     errSystem,
    const QWORD     cmsecIOElapsed )
{
    WCHAR           wszAbsPath[ IFileSystemAPI::cchPathMax ];

    CallS( posf->ErrPath( wszAbsPath ) );

    OSFileIIOReportError(   posf->Pfsconfig(),
                            wszAbsPath,
                            fWrite,
                            ibOffset,
                            cbLength,
                            err,
                            errSystem,
                            cmsecIOElapsed );
}

VOID OSFileIIOReportError(
    _In_ IFileSystemConfiguration* const    pfsconfig,
    _In_ const WCHAR* const                 wszAbsPath,
    _In_ const BOOL                         fWrite,
    _In_ const QWORD                        ibOffset,
    _In_ const DWORD                        cbLength,
    _In_ const ERR                          err,
    _In_ const DWORD                        errSystem,
    _In_ const QWORD                        cmsecIOElapsed )
{
    const ULONG     cwsz         = 7;
    const WCHAR *   rgpwsz[ cwsz ];
    DWORD           irgpwsz                      = 0;
    WCHAR           wszOffset[ 64 ];
    WCHAR           wszLength[ 64 ];
    WCHAR           wszTimeElapsed[ 64 ];

    OSStrCbFormatW( wszOffset, sizeof( wszOffset ), L"%I64i (0x%016I64x)", ibOffset, ibOffset );
    OSStrCbFormatW( wszLength, sizeof( wszLength ), L"%u (0x%08x)", cbLength, cbLength );
    OSStrCbFormatW( wszTimeElapsed, sizeof( wszTimeElapsed ), L"%I64u.%03I64u", cmsecIOElapsed / 1000, cmsecIOElapsed % 1000 );

    rgpwsz[ irgpwsz++ ]   = wszAbsPath;
    rgpwsz[ irgpwsz++ ]   = wszOffset;
    rgpwsz[ irgpwsz++ ]   = wszLength;

    WCHAR   wszError[ 64 ];
    WCHAR   wszSystemError[ 64 ];
    WCHAR * wszSystemErrorDescription    = NULL;

    Assert( err < JET_errSuccess );

    OSStrCbFormatW( wszError, sizeof( wszError ), L"%i (0x%08x)", err, err );
    OSStrCbFormatW( wszSystemError, sizeof( wszSystemError ), L"%u (0x%08x)", errSystem, errSystem );
    FormatMessageW( (   FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK ),
                    NULL,
                    errSystem,
                    MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
                    LPWSTR( &wszSystemErrorDescription ),
                    0,
                    NULL );

    rgpwsz[ irgpwsz++ ] = wszError;
    rgpwsz[ irgpwsz++ ] = wszSystemError;
    rgpwsz[ irgpwsz++ ] = wszSystemErrorDescription ? wszSystemErrorDescription : L"";
    rgpwsz[ irgpwsz++ ] = wszTimeElapsed;

    Assert( irgpwsz <= cwsz );
    pfsconfig->EmitEvent(   eventError,
                            GENERAL_CATEGORY,
                            fWrite ? OSFILE_WRITE_ERROR_ID : OSFILE_READ_ERROR_ID,
                            irgpwsz,
                            rgpwsz,
                            JET_EventLoggingLevelMin );

#if defined( USE_HAPUBLISH_API )
    pfsconfig->EmitEvent(   OSDiskIIOHaTagOfErr( err, fWrite ),
                            Ese2HaId( GENERAL_CATEGORY ),
                            Ese2HaId( fWrite ? OSFILE_WRITE_ERROR_ID : OSFILE_READ_ERROR_ID ),
                            irgpwsz,
                            rgpwsz,
                            fWrite ? HaDbIoErrorWrite : HaDbIoErrorRead,
                            wszAbsPath,
                            ibOffset, 
                            cbLength );
#endif

    if ( fWrite )
    {
        OSDiagTrackWriteError( err, errSystem, cmsecIOElapsed );
    }

    LocalFree( wszSystemErrorDescription );
}

//  helper for accumulating reasons

class OSIOREASONALLUP {

#ifdef DEBUG
    //   OSIOREASONALLUP State checking
    BOOL                m_fAdded;
    BOOL                m_fDead;
#endif

    //   Runtime stats tracking
    BOOL                m_fTooLong;

    //   IOREQ stats tracking
    BOOL                m_fSeekPenalty;     // Is it an SSD IO (no seek penalty) or an HDD IO?
    BOOL                m_fWrite;
    IOREQ::IOMETHOD     m_iomethod;
    QWORD               m_iFile;
    const WCHAR*        m_pwszFilePath;
    DWORD               m_dwEngineFileType;
    QWORD               m_qwEngineFileId;
    DWORD               m_dwDiskNumber;
    IFileAPI::FileModeFlags m_fmfFile;
    QWORD               m_ibOffset;
    DWORD               m_error;
    DWORD               m_cbTransfer;
    QWORD               m_cmsecIOElapsed;

    TICK                m_tickStartCompletions;

    DWORD               m_grbitMultiContext;
    const UserTraceContext* m_putc; // Stores a pointer, works because OSIOREASONALLUP is a short-lived object on the stack
    TraceContext        m_etc;

    DWORD               m_tidAlloc;
    DWORD               m_dtickQueueWorst;

    OSFILEQOS           m_qosDispatchHighest;
    OSFILEQOS           m_qosDispatchLowest;

    OSFILEQOS           m_qosOther;
    OSFILEQOS           m_qosHighestFirst;  //added for ETW trace

    //  Interim Variables - not logged directly, used to track things IOREQ to IOREQ.
    QWORD               m_ibOffsetLast;
    QWORD               m_cbDataLast;

public:

    OSIOREASONALLUP( const IOREQ * const pioreqHead, DWORD error, QWORD cmsecIOElapsed, QWORD ibOffset, DWORD cbTransfer ) :
        m_putc( &pioreqHead->m_tc.utc ),
        m_etc( pioreqHead->m_tc.etc )
    {
        OnDebug( m_fAdded = fFalse );
        OnDebug( m_fDead = fFalse );

        m_fSeekPenalty = pioreqHead->p_osf->m_posd->FSeekPenalty();
        m_fWrite = pioreqHead->fWrite;
        m_iomethod = pioreqHead->Iomethod();
        m_iFile = pioreqHead->p_osf->iFile;

        // m_pwszFilePath and m_putc are cached for tracing. The lifetime is for the duration of the tracing call.
        // pioreqHead must stay reserved for this duration. Releasing it back to the pool will cause bad things to happen.
        // This means that the trace must be logged before the completion is run.
        // Because we release the ioreq just before completion is run.
        m_pwszFilePath = pioreqHead->p_osf->m_posf->WszFile();
        
        m_dwEngineFileType = (DWORD)pioreqHead->p_osf->pfpapi->DwEngineFileType();
        m_qwEngineFileId = pioreqHead->p_osf->pfpapi->QwEngineFileId();
        m_dwDiskNumber = pioreqHead->p_osf->m_posd->DwDiskNumber();
        m_fmfFile = pioreqHead->p_osf->fmfTracing;
        m_ibOffset = ibOffset;
        m_error = error;
        m_cbTransfer = cbTransfer;
        m_cmsecIOElapsed = cmsecIOElapsed;
        m_tidAlloc = pioreqHead->m_tidAlloc;
        m_dtickQueueWorst = DtickDelta( pioreqHead->m_tickAlloc, TickOSTimeCurrent() ) - ( ( m_cmsecIOElapsed < (QWORD)lMax ) ? (DWORD)m_cmsecIOElapsed : 0 );
        if ( m_dtickQueueWorst > (ULONG)lMax )
            {
            m_dtickQueueWorst = 0;  // math is hard.
            }
        m_tickStartCompletions = TickOSTimeCurrent();

        m_grbitMultiContext = 0x0;
        m_qosDispatchLowest = qosIODispatchMask; // Ensures every dispatch enum will be less...
        m_qosDispatchHighest = 0;
        m_qosOther = 0;
        m_qosHighestFirst = 0;

        m_ibOffsetLast = 0;
        m_cbDataLast = 0;
    }

    ~OSIOREASONALLUP() { }

    void SetTooLong()
    {
        m_fTooLong = fTrue;
    }

    void Add( const IOREQ * const pioreq )
    {
        Assert( !m_fDead );
        OnDebug( m_fAdded = fTrue );

        OSFILEQOS qos = pioreq->grbitQOS;

        Assert( pioreq->m_tc.etc.iorReason.FValid() );

        //  should be impossible to combine IOs from different files!!!
        Assert( m_iFile == pioreq->p_osf->iFile );
        // EngineFileType can change from iofileDbRecovery to iofileDbAttached
        // Assert( m_dwEngineFileType == pioreq->p_osf->pfpapi->DwEngineFileType() );
        Assert( m_qwEngineFileId == pioreq->p_osf->pfpapi->QwEngineFileId() );

        if ( m_cbTransfer != 0 && COSFile::FOSFileSyncComplete( pioreq ) )
        {
            m_grbitMultiContext |= bitIoTraceSyncIo;
        }

        if ( pioreq->m_cRetries )
        {
            m_grbitMultiContext |= bitIoTraceIoRetries;
        }

        if ( m_fTooLong )
        {
            m_grbitMultiContext |= bitIoTraceTooLong;
        }
        if ( pioreq->grbitQOS & qosIOOptimizeOverwriteTracing )
        {
            m_grbitMultiContext |= bitIoTraceWriteGapped;
        }

        //  process the TraceContext

        #define CombineNewIorxElement( Iorx, SetIorx, iorxNone, bitMultiIorx )  \
            if ( m_etc.iorReason.Iorx() != pioreq->m_tc.etc.iorReason.Iorx() )  \
            {                                                                   \
                m_grbitMultiContext |= bitMultiIorx;                            \
            }

        CombineNewIorxElement( Iorp, SetIorp, iorpNone, bitIoTraceMultiIorp );
        CombineNewIorxElement( Iors, SetIors, iorsNone, bitIoTraceMultiIors );
        CombineNewIorxElement( Iort, SetIort, iortNone, bitIoTraceMultiIort );
        CombineNewIorxElement( Ioru, SetIoru, ioruNone, bitIoTraceMultiIoru );

        //  check our work, don't trust macros ... too new of a language feature. ;)
        Assert( m_etc.iorReason.Iorp() == pioreq->m_tc.etc.iorReason.Iorp() || m_grbitMultiContext & bitIoTraceMultiIorp );
        Assert( m_etc.iorReason.Iors() == pioreq->m_tc.etc.iorReason.Iors() || m_grbitMultiContext & bitIoTraceMultiIors );
        Assert( m_etc.iorReason.Iort() == pioreq->m_tc.etc.iorReason.Iort() || m_grbitMultiContext & bitIoTraceMultiIort );
        Assert( m_etc.iorReason.Ioru() == pioreq->m_tc.etc.iorReason.Ioru() || m_grbitMultiContext & bitIoTraceMultiIoru );

        #define CombineNewTcElement( elemTc, bitMultiFlag )   \
            if ( m_putc->elemTc != pioreq->m_tc.utc.elemTc )  \
            {                                                 \
                m_grbitMultiContext |= bitMultiFlag;          \
            }

        CombineNewTcElement( dwCorrelationID, bitIoTraceMultiCorrelationID );
        CombineNewTcElement( context.dwUserID, bitIoTraceMultiContextUserID );
        CombineNewTcElement( context.nOperationID, bitIoTraceMultiContextOperationID );
        CombineNewTcElement( context.nOperationType, bitIoTraceMultiContextOperationType );
        CombineNewTcElement( context.nClientType, bitIoTraceMultiContextClientType );

        if ( m_etc.nParentObjectClass != pioreq->m_tc.etc.nParentObjectClass )
        {
            m_grbitMultiContext |= bitIoTraceMultiParentObjectClass;
        }

        if ( m_etc.dwEngineObjid != pioreq->m_tc.etc.dwEngineObjid )
        {
            m_grbitMultiContext |= bitIoTraceMultiObjid;
        }

        m_etc.iorReason.AddFlag( pioreq->m_tc.etc.iorReason.Iorf() );

        //  process the TID

        if ( m_tidAlloc != pioreq->m_tidAlloc )
        {
            m_grbitMultiContext |= bitIoTraceMultiTidAlloc;
        }

        const TICK dtickQueueEst = DtickDelta( pioreq->m_tickAlloc, TickOSTimeCurrent() ) - ( ( m_cmsecIOElapsed < (QWORD)lMax ) ? (DWORD)m_cmsecIOElapsed : 0 );
        if ( dtickQueueEst < (ULONG)lMax && m_dtickQueueWorst < dtickQueueEst )
            {
            Expected( dtickQueueEst < (ULONG)lMax );
            m_dtickQueueWorst = dtickQueueEst;
            }

        //  process the QOS

        //      to clarify what these mean:
        //         qosDispatchHighest - Is the highest value in the QOS dispatch field from the entire IOREQ chain, and 
        //                              _should_ be the way / level it was issued at.
        //         qosDispatchLowest  - Is the lowest value in the QOS dispatch field from the IOREQ chain, and essentially
        //                              what got escallated to.  Though note qosIODispatchImmediate is a "low value" (even
        //                              though it's actually the high priority).
        //         qosHighestFirst    - Is the value that the IOREQ would have been actually issued at, and includes all QOS
        //                              flag fields (include Dispatch field and all the others).
        //         qosOther           - Is all QOS values, aside from the Dispatch flags, or'd together from all IOREQs.

        m_qosOther |= ( qos & ~qosIODispatchMask );

        Assert( qos & qosIODispatchMask );

        if ( m_qosDispatchHighest < ( qos & qosIODispatchMask ) )
        {
            m_qosDispatchHighest = qos & qosIODispatchMask;
        }
        if ( m_qosDispatchLowest > ( qos & qosIODispatchMask ) )
        {
            m_qosDispatchLowest = qos & qosIODispatchMask;
        }
        if ( m_qosHighestFirst == 0 )
        {
            Assert( ( m_qosHighestFirst & qosIODispatchMask ) < ( qos & qosIODispatchMask ) );
            m_qosHighestFirst = qos;
        }

        if ( m_cbTransfer != 0 )
        {
            if ( m_cbDataLast != 0 &&
                 ( m_ibOffsetLast + m_cbDataLast ) < pioreq->ibOffset )
            {
                m_grbitMultiContext |= bitIoTraceReadGapped;
            }

            Assert( pioreq->cbData );
            m_ibOffsetLast = pioreq->ibOffset;
            m_cbDataLast = pioreq->cbData;
        }

    }

    void OSTraceMe( const ERR errFirstIoreq, const OSFILEQOS qosFirstCompletionSignal )
    {
        Assert( m_fAdded );
        Assert( !m_fDead );

        // A set of headers usuable for logparser ...
        // Component, TraceId, iFile, TID, IOR, MulSinIorp, Iorp, MulSinIors, Iors, MulSinIoru, Ioru, Iorf, OP, Oper, ibOffset, cbTrans, Error,  QOSDispatchH, QOSDispatchL, QOSOptions, TM, cmsecIO, dtickComplete
        // Be nicer to add m_tidAlloc earlier right after file to be consistent with other traces, but I think SOMEONE's tool utilizes this trace.

        const BYTE fWrite = ( m_fWrite ? 1 : 0 );

        Assert( ( m_grbitMultiContext & mskIoTraceIomethod ) == 0 );
        static_assert( ( IOREQ::iomethodScatterGather & mskIoTraceIomethod ) == IOREQ::iomethodScatterGather, "Should hold the largest iomethod we currently have." );
        m_grbitMultiContext |= (DWORD)m_iomethod;

        const IOREASONPRIMARY iorp = m_etc.iorReason.Iorp();
        const IOREASONSECONDARY iors = m_etc.iorReason.Iors();
        const IOREASONTERTIARY iort = m_etc.iorReason.Iort();
        const IOREASONUSER ioru = m_etc.iorReason.Ioru();
        const IOREASONFLAGS iorf = m_etc.iorReason.Iorf();

        Assert( iorp != iorpNone );
    
        Assert( 0 == ( m_qosDispatchHighest & 0xFFFFFFFF00000000 ) ); // only bitIOComplete* signals use high DWORD.
        Assert( 0 == ( m_qosDispatchLowest & 0xFFFFFFFF00000000 ) ); // only bitIOComplete* signals use high DWORD.
        Assert( 0 == ( m_qosOther & 0xFFFFFFFF00000000 ) ); // only bitIOComplete* signals use high DWORD.
        OSTrace( JET_tracetagIO,
                    OSFormat(   "IO, IO-Completion, %I64X, IOR, %s, %d, %s, %d, %s, %d, 0x%x, OP, %s, 0x%016I64X, 0x%08X, Err=%d, Disp=%d, %d, 0x%x, TM, %I64u, %lu, for TID 0x%x, EngineFile=%d:0x%I64x, Fmf=0x%x",
                                m_iFile,
                                ( m_grbitMultiContext & bitIoTraceMultiIorp ) ? "MultiIorp" : "SingleIorp",
                                (DWORD)iorp,
                                ( m_grbitMultiContext & bitIoTraceMultiIors ) ? "MultiIors" : "SingleIors",
                                (DWORD)iors,
                                ( m_grbitMultiContext & bitIoTraceMultiIoru ) ? "MultiIoru" : "SingleIoru",
                                (DWORD)ioru,
                                (DWORD)iorf,
                                fWrite ? "write" : "read",
                                m_ibOffset,
                                m_cbTransfer,
                                m_error,
                                (DWORD)m_qosDispatchHighest,
                                (DWORD)m_qosDispatchLowest,
                                (DWORD)m_qosOther,
                                m_cmsecIOElapsed,
                                m_dtickQueueWorst,
                                m_tidAlloc,
                                m_dwEngineFileType,
                                m_qwEngineFileId,
                                m_fmfFile
                                ) );

        Assert( 0 == ( m_qosDispatchHighest & 0xFFFFFFFF00000000 ) ); // only bitIOComplete* signals use high DWORD, which is in m_grbitMultiContext.
        ETIOCompletion(
            m_iFile,
            m_grbitMultiContext,
            fWrite,
            m_putc->context.dwUserID,
            m_putc->context.nOperationID,
            m_putc->context.nOperationType,
            m_putc->context.nClientType,
            m_putc->context.fFlags,
            m_putc->dwCorrelationID,
            iorp,
            iors,
            iort,
            ioru,
            iorf,
            m_etc.nParentObjectClass,
            m_ibOffset,
            m_cbTransfer,
            m_error,
            (DWORD)m_qosHighestFirst,
            m_cmsecIOElapsed,
            m_dtickQueueWorst,    //  Note: Of course if there is a really idle IO, it might look bad, but might be OK if a higher QOS IO got combined and was issued promptly.
            m_tidAlloc,
            m_dwEngineFileType,
            m_qwEngineFileId,
            m_fmfFile,
            m_dwDiskNumber,
            m_etc.dwEngineObjid,
            m_qosHighestFirst /* the 64-bit one has the completion flags */ );

        // An even more detailed trace with extra Exchange specific client context
        Assert( 0 == ( m_qosDispatchHighest & 0xFFFFFFFF00000000 ) ); // only bitIOComplete* signals use high DWORD, which is in m_grbitMultiContext.
        ETIOCompletion2(
            m_pwszFilePath,
            m_grbitMultiContext,
            fWrite,
            m_putc->context.dwUserID,
            m_putc->context.nOperationID,
            m_putc->context.nOperationType,
            m_putc->context.nClientType,
            m_putc->context.fFlags,
            m_putc->dwCorrelationID,
            iorp,
            iors,
            iort,
            ioru,
            iorf,
            m_etc.nParentObjectClass,
            m_putc->pUserContextDesc->szClientComponent,
            m_putc->pUserContextDesc->szClientAction,
            m_putc->pUserContextDesc->szClientActionContext,
            &m_putc->pUserContextDesc->guidActivityId,
            m_ibOffset,
            m_cbTransfer,
            m_error,
            (DWORD)m_qosHighestFirst,
            m_cmsecIOElapsed,
            m_dtickQueueWorst,
            m_tidAlloc,
            m_dwEngineFileType,
            m_qwEngineFileId,
            m_fmfFile,
            m_dwDiskNumber,
            m_etc.dwEngineObjid );

        // Trace the session specific traces that can be selectively enabled/disabled on the JET session
        // allowing the caller to only collect traces for activity generated by specific sessions.
        bool fTraceReads = !!( m_putc->dwIOSessTraceFlags & JET_bitIOSessTraceReads );
        bool fTraceWrites = !!( m_putc->dwIOSessTraceFlags & JET_bitIOSessTraceWrites );
        bool fTraceHDD = !!( m_putc->dwIOSessTraceFlags & JET_bitIOSessTraceHDD );
        bool fTraceSSD = !!( m_putc->dwIOSessTraceFlags & JET_bitIOSessTraceSSD );

        if ( ( ( fWrite && fTraceWrites ) || ( !fWrite && fTraceReads ) ) &&
             ( ( m_fSeekPenalty && fTraceHDD ) || ( !m_fSeekPenalty && fTraceSSD ) ) )
        {
            Assert( 0 == ( m_qosDispatchHighest & 0xFFFFFFFF00000000 ) ); // only bitIOComplete* signals use high DWORD, which is in m_grbitMultiContext.
            ETIOCompletion2Sess(
                m_pwszFilePath,
                m_grbitMultiContext,
                fWrite,
                m_putc->context.dwUserID,
                m_putc->context.nOperationID,
                m_putc->context.nOperationType,
                m_putc->context.nClientType,
                m_putc->context.fFlags,
                m_putc->dwCorrelationID,
                iorp,
                iors,
                iort,
                ioru,
                iorf,
                m_etc.nParentObjectClass,
                m_putc->pUserContextDesc->szClientComponent,
                m_putc->pUserContextDesc->szClientAction,
                m_putc->pUserContextDesc->szClientActionContext,
                &m_putc->pUserContextDesc->guidActivityId,
                m_ibOffset,
                m_cbTransfer,
                m_error,
                (DWORD)m_qosHighestFirst,
                m_cmsecIOElapsed,
                m_dtickQueueWorst,
                m_tidAlloc,
                m_dwEngineFileType,
                m_qwEngineFileId,
                m_fmfFile,
                m_dwDiskNumber,
                m_etc.dwEngineObjid );
        }
    }

    void Kill()
    {
        OnDebug( m_fDead = fTrue );
    }
};

QWORD CmsecLatencyOfOSOperation( const IOREQ* const pioreq, const HRT dhrtIOElapsed )
{
    //  Get the wall-clock time for this IO operation

    QWORD       cmsecIOElapsed      = CmsecHRTFromDhrt( dhrtIOElapsed );

    //  Get the "logical" time for this IO operation (factors out debugger frozen periods)

    const ULONG ciotime             = pioreq->Ciotime();

    if ( cmsecIOElapsed > CmsecHighWaitFromIOTime( ciotime ) )
    {
        //  Over the worst case actual process runtime for this IO operation

        //  if the wall-clock time is longer than the highest possible IO time, we
        //  know this IO was hung up by the debugger, so assume it the shortest
        //  amount of time possible (or 20 ms if zero) ...

        cmsecIOElapsed = UlFunctionalMax( CmsecLowWaitFromIOTime( ciotime ), 20 );
    }

    return cmsecIOElapsed;
}

QWORD CmsecLatencyOfOSOperation( const IOREQ* const pioreq )
{
    // Note: this can happen on a pioreq free'd to the TLS.  So should not reset hrtIOStart until re-used. 
    return CmsecLatencyOfOSOperation( pioreq, HrtHRTCount() - pioreq->hrtIOStart );
}


//  process I/O completions

void OSDiskIIOThreadCompleteWithErr( DWORD error, DWORD cbTransfer, IOREQ* pioreqHead )
{
    IFilePerfAPI * const    pfpapi              = pioreqHead->p_osf->pfpapi;

    //  compute properties of total IO operation / run

    HRT                     hrtIoreqCompleteStart = HrtHRTCount();
    const HRT               dhrtIOElapsed       = hrtIoreqCompleteStart - pioreqHead->hrtIOStart;
    const QWORD             cmsecIOElapsed      = CmsecLatencyOfOSOperation( pioreqHead, dhrtIOElapsed );
    BOOL                    fIOTookTooLong      = ( cmsecIOElapsed > ( pioreqHead->p_osf->Pfsconfig()->DtickHungIOThreshhold() / 2 ) );
    const QWORD             ibOffsetHead        = QWORD( pioreqHead->ovlp.Offset ) + ( QWORD( pioreqHead->ovlp.OffsetHigh ) << 32 );

    OSIOREASONALLUP osiorall( pioreqHead, error, cmsecIOElapsed, ibOffsetHead, cbTransfer );

    ASSERT_VALID_RTL( pioreqHead );

    AssertRTL( pioreqHead->FInIssuedState() );

    Assert( pioreqHead->p_osf );
    Assert( pioreqHead->p_osf->m_posd );

    //  RFS:  post-completion error

    if ( !RFSAlloc( pioreqHead->fWrite ? OSFileWrite : OSFileRead ) )
    {
        //  if it's a zero-length I/O that succeeded, then we should not inject failures.

        if ( !( pioreqHead->cbData == 0 && pioreqHead->pioreqIorunNext == NULL ) && ( error == ERROR_SUCCESS ) )
        {
            error = ERROR_IO_DEVICE;
        }
    }

    if ( ErrFaultInjection( 64867 ) && cbTransfer != 0 && error == ERROR_SUCCESS /* avoid dummy completions and errors */ )
    {
        //  for this RFS form we'll lie about the length of the IO ...
        fIOTookTooLong = fTrue;
    }

    pfpapi->IncrementIOCompletion(
                pioreqHead->m_iocontext,
                pioreqHead->p_osf->m_posd->DwDiskNumber(),
                pioreqHead->grbitQOS,
                dhrtIOElapsed,
                cbTransfer,
                pioreqHead->fWrite );

    //  if this I/O failed then report it to the event log
    //
    //  exceptions:
    //
    //  -  we do not report ERROR_HANDLE_EOF
    //  -  we limit the frequency of ERROR_DISK_FULL reports

    Assert( ERROR_IO_PENDING != error ); // we wouldn't expect this here ...
    const ERR   err             = ErrOSFileIFromWinError( error );
    BOOL        fReportError        = fFalse;

    if ( ERROR_HANDLE_EOF == error )
    {
        Assert( JET_errFileIOBeyondEOF == err );
    }
    else if ( ERROR_DISK_FULL == error )
    {
        //  only report DiskFull if we haven't done so in a while
        //
        Assert( JET_errDiskFull == err );
        fReportError = pfpapi->FReportDiskFull();
    }
    else if ( err < JET_errSuccess )
    {
        fReportError = fTrue;
    }

    if ( fReportError )
    {
        OSDiskIIOReportError( pioreqHead, err, error, cmsecIOElapsed );
    }

    //  if this I/O took a long time then report it (unless it was an error or we've reported it too much)
    //

    if ( fIOTookTooLong &&
        err >= JET_errSuccess )
    {
        osiorall.SetTooLong();
        if ( pfpapi->FReportAbnormalIOLatency( pioreqHead->fWrite, cmsecIOElapsed ) )
        {
            OSDiskIIOReportIOLatency( pioreqHead, cmsecIOElapsed );
        }
    }

#ifdef DEBUG
    //  Used to verify IOREQ chain doesn't cross files/disks, which would be bad ... unless we've
    //  invented cross-file IO coallescing ... write many logs with one IO. ;)
    _OSFILE * p_osfHead = pioreqHead->p_osf;
    COSDisk * posdHead = pioreqHead->p_osf->m_posd;
    Assert( p_osfHead != NULL );
    Assert( posdHead != NULL );
#endif

    //  Release the locked up IO dispatched count ...

    pioreqHead->p_osf->m_posd->QueueCompleteIORun( pioreqHead );

    // note: pioreqHead is invalid after first iteration ...
    CLocklessLinkedList< IOREQ > IOChain( pioreqHead );

    //  Trace the IO (before completion)

    // Since IO thread / completion is on a 2nd thread from any actual user activity, any other test 
    // thread that may be waiting for like a DML read to complete will be unblocked as soon as we 
    // complete the IO, and then may not get the IO trace for the done IO before the test checks its
    // trace / test results.
    IOREQ * pioreq = pioreqHead;
    while ( pioreq ) // non-destructive iteration to accumulate the trace.
    {
        osiorall.Add( pioreq );
        pioreq = pioreq->pioreqIorunNext;
    }

    //  process callback for each IOREQ

    INT iIoreqTracking = 0;
    while ( pioreq = IOChain.RemovePrevMost( OffsetOf( IOREQ, pioreqIorunNext ) ) )
    {
        const BYTE fWriteTr = (BYTE)pioreq->fWrite;
        const QWORD iFileTr = pioreq->p_osf->iFile;
        const QWORD ibOffsetTr = pioreq->ibOffset;
        const DWORD cbDataTr = pioreq->cbData;
        const DWORD dwDiskNumberTr = pioreq->p_osf->m_posd->DwDiskNumber();
        const DWORD tidAllocTr = pioreq->m_tidAlloc;

        OnDebug( const IOREQ::IOMETHOD iomethod = pioreq->Iomethod() );

        hrtIoreqCompleteStart = HrtHRTCount();

        //
        //  validate our IO chain is intact ...
        //

        Assert( p_osfHead == pioreq->p_osf );
        Assert( posdHead == pioreq->p_osf->m_posd );
        Assert( pioreq->m_posdCurrentIO == NULL || pioreq == pioreqHead );  // only first IOREQ should have m_posdCurrentIO set.

        //  we will run ALL IOREQs through this function to test it's internal asserts are valid
        Assert( COSFile::FOSFileManagementOperation( pioreq ) ||
                !COSFile::FOSFileManagementOperation( pioreq ) );

        //
        //  Complete the IOREQ and set the QOS Complete signals
        //

        //  none of the complete flags should be set.
        Assert( 0 == ( pioreq->grbitQOS & qosIOCompleteMask ) );

        Assert( pioreq->fCoalesced || pioreq == pioreqHead ); //  guess the flag is redundant now?  but dubious use of pioreqHead here is barely ok.
        if ( pioreq->fCoalesced )
        {
             //  Indicate the IO was optimized / combined ...
            pioreq->grbitQOS |= qosIOCompleteIoCombined;
        }
        if ( fIOTookTooLong )
        {
            //  Indicate this IO took an abnormally long time ...
            pioreq->grbitQOS |= qosIOCompleteIoSlow;
        }
 
        //  Release the locked up queue space (and merge in IO Queue Status - i.e. if queue is "low")...

        if ( pioreq->p_osf->m_posd->FQueueCompleteIOREQ( pioreq ) )
        {
            Assert( pioreq->fWrite );
            pioreq->grbitQOS |= qosIOCompleteWriteGameOn;
        }

        if ( FIOThread() && pioreq->p_osf->m_posd->CioReadyMetedEnqueued() > 0 )
        {
            //  Note: Here this ReadGameOn is best effort, we re-check the CioReadyMetedEnqueued()
            //  properly up a level to actually decide to issue more.
            pioreq->grbitQOS |= qosIOCompleteReadGameOn;
        }

        //  reset when we reset the IOREQs queue status (i.e. no longer dispatched)

        Assert( 0 == pioreq->Ciotime() );

        //  get the error for this IOREQ, specifically with respect to reading
        //  past the end of file.  it is possible to get a run of IOREQs where
        //  some are before the EOF and some are after the EOF.  each must be
        //  passed or failed accordingly

        const DWORD errorIOREQ  = ( error != ERROR_SUCCESS ?
                                        error :
                                        // note can not use pioreq->IbBlockMax() as it defends against cbData 
                                        (   pioreq->ibOffset + pioreq->cbData <= ibOffsetHead + cbTransfer ?
                                                ERROR_SUCCESS :
                                                ERROR_HANDLE_EOF ) );

        Assert( ERROR_IO_PENDING != errorIOREQ ); // we wouldn't expect this here ...
        ERR errIOREQ = ErrOSFileIFromWinError( errorIOREQ );

        //  merge in "bad" IO signal ...

        if ( fIOTookTooLong && JET_errSuccess <= errIOREQ && ( pioreq->grbitQOS & qosIOSignalSlowSyncIO ) )
        {
            errIOREQ = ErrERRCheck( wrnIOSlow );    // did not use wrnSlow as this condition usually means disk issues.
        }
        
        if ( pioreq == pioreqHead )
        {
            osiorall.OSTraceMe( errIOREQ, pioreq->grbitQOS );
        }

        //  Remove pioreqHead dependencies
        //
        //  Note: only important / true on first iter - but declare pioreqHead / osiorall dead.
        pioreqHead = NULL;
        osiorall.Kill();

        const OSFILEQOS qosTr = pioreq->grbitQOS; // snag after all Complete / Output signals computed.

        //  backup the status of this IOREQ before we free it

        IOREQ ioreq( pioreq );
        ASSERT_VALID_RTL( &ioreq );
        Assert( ioreq.FCompleted() );

        // Clear trace context info before freeing the ioreq
        pioreq->m_tc.Clear();

        //  cache IOREQ (not from reserved pool) for possible reuse by an I/O issued by this callback
        if ( pioreq->fFromReservePool )
        {
            OSDiskIIOREQFree( pioreq );
        }
        else
        {
            OSDiskIIOREQFreeToCache( pioreq );
        }

#ifdef DEBUG
        //  check / validate our lock state

        if ( iomethod == IOREQ::iomethodAsync ||
                iomethod == IOREQ::iomethodScatterGather ||
                // probably only need this last thing
                FIOThread() )
        {
            Expected( FIOThread() );
            //  we should be guaranteed not to have locks if we're completing from the async
            //  completion thread
            CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );
        }
#endif // DEBUG

        //  for all (async and sync) IOs though should not acquire / release locks during the
        //  IO completion function

        DWORD cDisableDeadlockDetection = 0;
        DWORD cDisableOwnershipTracking = 0;
        DWORD cLocks = 0;
        CLockDeadlockDetectionInfo::GetApiEntryState( &cDisableDeadlockDetection, &cDisableOwnershipTracking, &cLocks );

        //  perform per-file I/O completion callback

        const ERR errTr = errIOREQ;
        ioreq.p_osf->pfnFileIOComplete( &ioreq,
                                        errIOREQ,
                                        ioreq.p_osf->keyFileIOComplete );

        //  expect to have the same sync lock state as when we started (even for sync IO, as
        //  a sync IO can be converted to async IO (under OOM conditions)) so then the lock 
        //  would be acquired or released on an unexepcted thread context

        CLockDeadlockDetectionInfo::AssertCleanApiExit( cDisableDeadlockDetection, cDisableOwnershipTracking, cLocks );

        //  free IOREQ in cache if unused

        pioreq = PioreqOSDiskIIOREQAllocFromCache();
        if ( pioreq )
        {
            OSDiskIIOREQFree( pioreq );
        }

        const HRT dhrtIOCompleteElapsed = HrtHRTCount() - hrtIoreqCompleteStart;
        // for perspective a full / non-fast ErrBFReadLatch() takes on the order of 0.3 and 1.2 us.  We want 
        // to find out if anything is holding us up.
        if ( CusecHRTFromDhrt( dhrtIOCompleteElapsed ) > 500 /* us */ )
        {
            ETIOIoreqCompletion( fWriteTr, iFileTr, ibOffsetTr, cbDataTr, dwDiskNumberTr, tidAllocTr, qosTr, iIoreqTracking, errTr, CusecHRTFromDhrt( dhrtIOCompleteElapsed ) );
        }

        iIoreqTracking++;
    }
    
}

void OSDiskIIOThreadIComplete(  const DWORD     dwError,
                                const DWORD_PTR dwThreadContext,
                                DWORD           cbTransfer,
                                IOREQ           *pioreqHead )
{
    Expected( GetLastError() == dwError );  // but no longer required, only error var has to be right

    Postls()->fIOThread = fTrue;

    Assert( pioreqHead );

    if ( pioreqHead )   // defense in depth
    {
        //  this is a completion packet caused by our I/O functions

        Assert( FOSDiskIIOREQSlowlyCheckIsOurOverlappedIOREQ( pioreqHead ) );

        //  does this I/O completion provide meted IO, that we will want to issue more of ... 

        COSDisk * posdReDequeue = NULL;
        Assert( pioreqHead->m_posdCurrentIO );

        //  It might be tempting to check && pioreqHead->FUseMetedQ() but we increase the Async Read count
        //  for regular qosIODispatchImmediate IOs as well as qosIODispatchBackground, so it could be a non-meted
        //  Q operation that suppresses our background IOs, and so after a regular IO completes we also need to see
        //  if there is a suppressed meted Q operation to dispatch.
        //  Note: immediate IOs in this context is not sync IOs, but async IOs that pass qosIODispatchImmediate.
        //  There are a few paths that haven't been upgraded to pass qosIODispatchBackground, such as log file, or
        //  flushmap file, or any file's header page reads.  Search for QosSyncDefault() to see the qosIODispatchImmediate
        //  cases left, and note this func is used in Async paths even though it has Sync in the name.

        if ( pioreqHead->m_posdCurrentIO /* only check jic */ )
        {
            posdReDequeue = pioreqHead->m_posdCurrentIO;

            //  Must increase the ref count, because as soon as the IOREQ gets completed, nothing holds
            //  the OSDisk in memory.
            OSDiskConnect( posdReDequeue );
        }

        //  decrement our pending I/O count

        PERFOptDeclare( IFilePerfAPI * const    pfpapi  = pioreqHead->p_osf->pfpapi );
        PERFOpt( pfpapi->DecrementIOAsyncPending( pioreqHead->fWrite ) );

        //  call completion function with error

        Assert( pioreqHead->FIssuedAsyncIO() );

        OSDiskIIOThreadCompleteWithErr( dwError, cbTransfer, pioreqHead );

        //  Check if the meted Q needs a little more attention to complete issue more of the deferred IO work
        //  lingering in the Meted q.
        
        if ( posdReDequeue )
        {
            Assert( FIOThread() );

            if ( posdReDequeue->CioReadyMetedEnqueued() > 0 ||
                 posdReDequeue->CioReadyWriteEnqueued() > 0 )
            {
                const ERR errRe = ErrOSDiskProcessIO( posdReDequeue, fTrue );
                //  Note if we couldn't issue IO ErrOSDiskProcessIO() triggers OSDiskIIOThreadIRetryIssue(), but only
                //  on errDiskTilt ... shouldn't we also if we get OOM?.  Well let's see if this comes up.
                if ( errRe < JET_errSuccess && errRe != errDiskTilt )
                {
                    //  For any reason we failed our IO, then enque another proper dispatch pass ... 
                    CHAR szMsg[40];
                    OSStrCbFormatA( szMsg, sizeof(szMsg), "UnhandledProcessIoErr=%d", errRe );
                    FireWall( szMsg );
                    OSDiskIIOThreadIRetryIssue();
                }
            }
            
            OSDiskDisconnect( posdReDequeue, NULL );
        }
    }

    Postls()->fIOThread = fFalse;
}

//  terminates the I/O Thread

void OSDiskIIOThreadTerm( void )
{
    //  term the task manager

    if ( g_postaskmgrFile )
    {
        g_postaskmgrFile->TMTerm();
        delete g_postaskmgrFile;
        g_postaskmgrFile = NULL;
    }
}

//  initializes the I/O Thread, or returns either JET_errOutOfMemory or
//  JET_errOutOfThreads

ERR ErrOSDiskIIOThreadInit( void )
{
    ERR err;

    //  reset all pointers

    g_postaskmgrFile  = NULL;

    //  initialize our task manager (1 thread, no local contexts)
    //  NOTE:  1 thread required to serialize I/O on Win9x

    g_postaskmgrFile = new CTaskManager;
    if ( !g_postaskmgrFile )
    {
        err = ErrERRCheck( JET_errOutOfMemory );
        goto HandleError;
    }
    Call( g_postaskmgrFile->ErrTMInit( 1 ) );

    return JET_errSuccess;

HandleError:
    OSDiskIIOThreadTerm();
    return err;
}

//  tells I/O Thread to start issuing scheduled I/O

void OSDiskIOThreadStartIssue( const P_OSFILE p_osf )
{
    Assert( p_osf == NULL || p_osf->m_posd );

    //  would be generally good to check that by issue we've emptied the deferred IO run 
    //  for any specific file(p_osf) that we are starting issue for.

    Expected( p_osf == NULL || !Postls()->iorunpool.FContainsFileIoRun( p_osf ) );

    //  retry forever until we successfully post an issue task to the I/O thread.
    //  we must do this because we can temporarily fail to post due to low
    //  resources and we don't want to make the I/O thread timeout to see if
    //  it missed any signals

    //  If an I/O Context / OS Disk is not provided, that's fine just start the I/O 
    //  thread anyway, if it finds nothing to do it will quit.  This gets called 
    //  with NULL p_osf in a few rare scenarios.
    //  Consider: We could pass the m_posd to the OSDiskIIOThreadIIssue thread.
    const ULONG cioDiskEnqueued = ( p_osf ) ? p_osf->m_posd->CioAllEnqueued() : 0;

    HRT  hrtStart = HrtHRTCount();
    ETIOIssueThreadPost( p_osf, cioDiskEnqueued );

    LONG cDispatchAttempts = 0;
    while ( ( ( p_osf ) ? cioDiskEnqueued : fTrue ) &&
            ( ++cDispatchAttempts ) &&
            g_postaskmgrFile->ErrTMPost( OSDiskIIOThreadIIssue, 0, 0 ) < JET_errSuccess )
    {
        UtilSleep( 1 );
    }

    ETIOIssueThreadPosted( p_osf, cDispatchAttempts, CusecHRTFromDhrt( HrtHRTCount() - hrtStart ) );
}


BOOL FIOThread( void )
{
    Assert( Postls() ); // possible to be NULL?
    return Postls() ? Postls()->fIOThread : fFalse;
}

//  called when the I/O thread needs to retry to issue I/Os due to a "too many
//  I/Os" condition.  this is different from a normal StartIssue call because
//  this can only be requested by the I/O thread and because it cannot fail and
//  because it will only be called once after it is requested and the I/O thread
//  is idle for a period of time

VOID OSDiskIIOThreadIRetryIssue()
{
    Assert( FIOThread() );
    
    g_postaskmgrFile->TMRequestIdleCallback( 2, OSDiskIIOThreadIIssue );
}

