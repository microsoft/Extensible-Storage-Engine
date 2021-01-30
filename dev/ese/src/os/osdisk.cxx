// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

#include <winioctl.h>





#include "collection.hxx"

#include <malloc.h>

COSFilePerfDummy    g_cosfileperfDefault;

BYTE* g_rgbGapping = NULL;

const LOCAL OSDiskMappingMode g_diskModeDefault = eOSDiskOneDiskPerPhysicalDisk;



void OSDiskIIOThreadTerm( void );
ERR ErrOSDiskIIOThreadInit( void );




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





CCriticalSection g_critIOREQPool( CLockBasicInfo( CSyncBasicInfo( "IOREQ Pool" ), rankIOREQPoolCrit, 0 ) );


IOREQCHUNK *    g_pioreqchunkRoot;
DWORD           g_cbIoreqChunk;

INLINE DWORD CioreqPerChunk( DWORD cbChunk )
{
    return ( ( cbChunk - sizeof(IOREQCHUNK) ) / sizeof(IOREQ) );
}


IOREQ * PioreqOSDiskIIOREQAllocSlot( void )
{
    ERR err = JET_errSuccess;

    IOREQ * pioreqSlot = NULL;

    Assert( g_critIOREQPool.FOwner() );

    if ( g_pioreqchunkRoot->cioreqMac >= g_pioreqchunkRoot->cioreq )
    {
        Assert( g_pioreqchunkRoot->cioreqMac == g_pioreqchunkRoot->cioreq );


        IOREQCHUNK * pioreqchunkNew;
        Alloc( pioreqchunkNew = (IOREQCHUNK*)PvOSMemoryHeapAlloc( g_cbIoreqChunk ) );
        memset( pioreqchunkNew, 0, g_cbIoreqChunk );


        pioreqchunkNew->pioreqchunkNext = g_pioreqchunkRoot;
        pioreqchunkNew->cioreq = CioreqPerChunk( g_cbIoreqChunk );
        pioreqchunkNew->cioreqMac = 0;
        g_pioreqchunkRoot = pioreqchunkNew;
    }
    Assert( g_pioreqchunkRoot->cioreqMac < g_pioreqchunkRoot->cioreq );


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

    m_tc( pioreqToCopy->m_tc, false )
{
    Expected( FCompleted() );
}


void OSDiskIIOREQInit( IOREQ * const pioreq, HANDLE hEvent )
{
    Assert( pioreq );
    Assert( hEvent );

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




typedef CPool< IOREQ, IOREQ::OffsetOfAPIC > IOREQPool;


DWORD           g_cioOutstandingMax;
DWORD           g_cioBackgroundMax;
volatile DWORD  g_cioreqInUse;
IOREQPool*      g_pioreqpool;


IOREQPool*      g_pioreqrespool;
volatile DWORD  g_cioreqrespool = 0;
volatile DWORD  g_cioreqrespoolleak = 0;


INLINE void OSDiskIIOREQFree( IOREQ* pioreq )
{

    Assert( NULL == pioreq->pioreqIorunNext );
    Assert( NULL == pioreq->pioreqVipList );

    pioreq->p_osf = NULL;

    if ( pioreq->fFromReservePool )
    {
        Assert( !pioreq->m_fFromTLSCache );

        pioreq->SetIOREQType( IOREQ::ioreqInReservePool );


        g_pioreqrespool->Insert( pioreq );
    }
    else
    {
        pioreq->m_fFromTLSCache = fFalse;
        pioreq->SetIOREQType( IOREQ::ioreqInAvailPool );


        g_pioreqpool->Insert( pioreq );
    }
}


INLINE ERR ErrOSDiskIIOREQCreate()
{
    ERR     err             = JET_errSuccess;
    HANDLE  hEvent          = NULL;


    if ( g_cioreqInUse >= 10000000 )
    {

        AssertSz( fFalse, "If we hit this our quota system has gone wrong as it's not protecting things enough" );
        return JET_errSuccess;
    }

    g_critIOREQPool.Enter();

    AtomicIncrement( (LONG*)&g_cioreqInUse );


    Alloc( hEvent = CreateEventW( NULL, TRUE, FALSE, NULL ) );


    IOREQ * pioreqT;
    Alloc( pioreqT = PioreqOSDiskIIOREQAllocSlot() );


    OSDiskIIOREQInit( pioreqT, hEvent );
    hEvent = NULL;


    OSDiskIIOREQFree( pioreqT );

HandleError:

    if ( hEvent )
    {
        CloseHandle( hEvent );
    }

    g_critIOREQPool.Leave();

    return err;
}

LOCAL ERR ErrOSDiskIOREQReserveAndLeak();
INLINE IOREQ* PioreqOSDiskIIOREQAlloc( BOOL fUsePreReservedIOREQs, BOOL * pfUsedTLSSlot )
{
    IOREQ *     pioreq;

    if ( pfUsedTLSSlot )
    {
        *pfUsedTLSSlot = fFalse;
    }


    if ( fUsePreReservedIOREQs )
    {


        Assert( g_pioreqrespool->Cobject() >= 1 );

        if ( !g_pioreqrespool->Cobject() )
        {
            FireWall( "OutOfReservedIoreqs" );

            (void)ErrOSDiskIOREQReserveAndLeak();

        }


        IOREQPool::ERR errIOREQ = g_pioreqrespool->ErrRemove( &pioreq );
        Assert( errIOREQ == IOREQPool::ERR::errSuccess );

        Assert( pioreq->FInReservePool() );
        Assert( pioreq->fFromReservePool );
        Assert( !pioreq->m_fFromTLSCache );
    }


    else if ( Postls()->pioreqCache )
    {

        pioreq = Postls()->pioreqCache;
        Postls()->pioreqCache = NULL;

        Assert( pioreq->FCachedInTLS() );

        pioreq->m_fFromTLSCache = fTrue;

        Assert( !pioreq->fFromReservePool );

        Assert( pfUsedTLSSlot );
        if ( pfUsedTLSSlot )
        {
            *pfUsedTLSSlot = fTrue;
        }
    }


    else
    {

        pioreq = NULL;
        IOREQPool::ERR errAvail;

        if ( ( errAvail = g_pioreqpool->ErrRemove( &pioreq, cmsecTest ) ) == IOREQPool::ERR::errOutOfObjects )
        {

            Assert( NULL == pioreq );
            const LONG cRFSCountdownOld = RFSThreadDisable( 10 );
            const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );
            do
            {

                Assert( NULL == pioreq );

                if ( ErrOSDiskIIOREQCreate() < JET_errSuccess )
                {
                    COSDisk::EnqueueDeferredIORun( NULL );
                    OSDiskIOThreadStartIssue( NULL );
                }
            }
            while ( ( errAvail = g_pioreqpool->ErrRemove( &pioreq, 2  ) ) == IOREQPool::ERR::errOutOfObjects );
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

    pioreq->hrtIOStart = HrtHRTCount();

    pioreq->m_tickAlloc = TickOSTimeCurrent();

    pioreq->SetIOREQType( IOREQ::ioreqAllocFromAvail );


    return pioreq;
}


ERR ErrOSDiskIOREQReserve()
{

    AtomicIncrement( (LONG*)&g_cioreqrespool );
    IOREQ* pioreq = PioreqOSDiskIIOREQAlloc( fFalse, NULL );
    if ( !pioreq )
    {
        AtomicDecrement( (LONG*)&g_cioreqrespool );
        return ErrERRCheck( JET_errOutOfMemory );
    }


    pioreq->fFromReservePool = fTrue;
    OSDiskIIOREQFree( pioreq );

    return JET_errSuccess;
}

LOCAL ERR ErrOSDiskIOREQReserveAndLeak()
{
    const ULONG cioreqrespoolleakMax = g_cioOutstandingMax / 2;
    ULONG cioreqTemp;
    if ( cioreqrespoolleakMax == 0 || cioreqrespoolleakMax > g_cioOutstandingMax ||
        !FAtomicIncrementMax( &g_cioreqrespoolleak, &cioreqTemp, cioreqrespoolleakMax ) )
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


VOID OSDiskIOREQUnreserve()
{

    IOREQ* pioreq = PioreqOSDiskIIOREQAlloc( fTrue, NULL );
    Assert( pioreq );

    if ( pioreq )
    {
        Assert( pioreq->fFromReservePool );
        pioreq->fFromReservePool = fFalse;
        OSDiskIIOREQFree( pioreq );
        AtomicDecrement( (LONG*)&g_cioreqrespool );
    }

}


INLINE void OSDiskIIOREQFreeToCache( IOREQ* pioreq )
{

    if ( Postls()->pioreqCache )
    {
        OSDiskIIOREQFree( Postls()->pioreqCache );
        Postls()->pioreqCache = NULL;
    }


    Assert( !pioreq->FInReservePool() );
    pioreq->p_osf = NULL;
    pioreq->SetIOREQType( IOREQ::ioreqCachedInTLS );
    Postls()->pioreqCache = pioreq;
}


INLINE IOREQ* PioreqOSDiskIIOREQAllocFromCache()
{

    IOREQ* pioreq = Postls()->pioreqCache;


    Postls()->pioreqCache = NULL;


    Assert( NULL == pioreq || pioreq->FCachedInTLS() );
    return pioreq;
}

#ifndef RTM
void IOREQ::AssertValid( void ) const
{

    switch( m_ioreqtype )
    {

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
            AssertRTL( m_tc.etc.FEmpty() );

            switch( m_ioreqtype )
            {
                case ioreqInAvailPool:
                    break;

                case ioreqCachedInTLS:
                    AssertRTL( Postls()->pioreqCache == NULL );
                    break;

                case ioreqInReservePool:
                    AssertRTL( fFromReservePool );
                    break;

                default:
                    AssertSz( fFalse, "Unknown available type!!!" );
            }
            break;

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

            switch( m_ioreqtype )
            {
                case ioreqAllocFromAvail:
                    AssertRTL( NULL == p_osf );
                    AssertRTL( m_tc.etc.FEmpty() );
                    break;

                case ioreqInIOCombiningList:
                    AssertRTL( p_osf );
                    AssertRTL( m_fHasHeapReservation || m_fCanCombineIO );
                    AssertRTL( m_tc.etc.iorReason.Iorp() != iorpNone );
                    break;

                case ioreqAllocFromEwreqLookaside:
                    AssertRTL( p_osf );
                    AssertRTL( m_fHasHeapReservation );
                    AssertRTL( m_tc.etc.iorReason.Iorp() != iorpNone );
                    break;

                default:
                    AssertSz( fFalse, "Unknown pending type!!!" );
            }

            break;

        case ioreqEnqueuedInIoHeap:
        case ioreqEnqueuedInVipList:
        case ioreqEnqueuedInMetedQ:

            Assert( FInEnqueuedState() );
            AssertRTL( p_osf );

            AssertRTL( m_posdCurrentIO == NULL );
            AssertRTL( m_ciotime == 0 );
            AssertRTL( m_iomethod == iomethodNone );
            AssertRTL( m_tc.etc.iorReason.Iorp() != iorpNone );

            switch( m_ioreqtype )
            {
                case ioreqEnqueuedInIoHeap:
                    AssertRTL( m_fHasHeapReservation || m_fCanCombineIO );
                    break;

                case ioreqEnqueuedInMetedQ:
                    AssertRTL( m_fHasHeapReservation || m_fCanCombineIO );
                    break;

                case ioreqEnqueuedInVipList:
                    break;

                default:
                    AssertSz( fFalse, "Unknown enqueued type!!!" );
            }
            break;


        case ioreqRemovedFromQueue:
            Assert( FInIssuedState() );
            AssertRTL( p_osf );
            AssertRTL( m_ciotime == 0 );
            AssertRTL( m_iomethod == iomethodNone );
            AssertRTL( m_tc.etc.iorReason.Iorp() != iorpNone );
            break;

        case ioreqIssuedSyncIO:
        case ioreqIssuedAsyncIO:
            AssertRTL( m_posdCurrentIO );
            AssertRTL( ( m_iomethod == iomethodNone && cbData == 0 ) ||  m_ciotime > 0 );
            AssertRTL( m_tc.etc.iorReason.Iorp() != iorpNone );

        case ioreqSetSize:
        case ioreqExtendingWriteIssued:

            Assert( FInIssuedState() );
            AssertRTL( p_osf );

            switch( m_ioreqtype )
            {
                case ioreqIssuedSyncIO:
                    break;

                case ioreqIssuedAsyncIO:
                    AssertRTL( m_iomethod != iomethodNone );
                    break;

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


        case ioreqCompleted:

            Assert( FCompleted() );

            AssertRTL( p_osf );

            AssertRTL( m_ciotime == 0 );
            AssertRTL( m_iomethod == iomethodNone );
            AssertRTL( m_tc.etc.iorReason.Iorp() != iorpNone );

            break;

        default:
            AssertSz( fFalse, "Unknown IOREQ type (%d)!!!", m_ioreqtype );

    }
}
#endif

VOID IOREQ::ValidateIOREQTypeTransition( const IOREQTYPE ioreqtypeNext )
{
#ifndef RTM
    switch( ioreqtypeNext )
    {

        case ioreqInAvailPool:
            AssertRTL( 0 ==  m_ioreqtype ||
                        FAllocFromAvail() ||
                        FCachedInTLS() ||
                        FCompleted() );
            break;

        case ioreqCachedInTLS:
            AssertRTL( FCompleted() );
            break;

        case ioreqInReservePool:
            AssertRTL( FAllocFromAvail() ||
                        FCompleted() );
            break;

        case ioreqAllocFromAvail:
            AssertRTL( FInAvailState() );
            break;

        case ioreqAllocFromEwreqLookaside:
            AssertRTL( FExtendingWriteIssued() );
            break;

        case ioreqInIOCombiningList:
            AssertRTL( FAllocFromAvail() ||
                        FAllocFromEwreqLookaside() );
            break;

        case ioreqEnqueuedInIoHeap:
        case ioreqEnqueuedInMetedQ:
            AssertRTL( FInPendingState() ||
                        FRemovedFromQueue() ||
                        FIssuedSyncIO() ||
                        FIssuedAsyncIO() );
            break;

        case ioreqEnqueuedInVipList:
            AssertRTL( FInPendingState() ||
                        FEnqueuedInMetedQ() ||
                        FIssuedSyncIO() ||
                        FIssuedAsyncIO() );
            break;


        case ioreqRemovedFromQueue:
            AssertRTL( FInEnqueuedState() );
            break;



        case ioreqIssuedSyncIO:
            AssertRTL( FAllocFromAvail() ||
                        FAllocFromEwreqLookaside() );
            break;

        case ioreqIssuedAsyncIO:
            AssertRTL( FRemovedFromQueue() );
            break;

        case ioreqSetSize:
            AssertRTL( FAllocFromAvail() );
            break;

        case ioreqExtendingWriteIssued:
            AssertRTL( FAllocFromAvail() ||
                        FAllocFromEwreqLookaside() );
            break;



        case ioreqCompleted:
            AssertRTL( FInIssuedState() );

            
            Assert( m_crit.FOwner() );
            break;
 
        default:
            AssertSz( fFalse, "Unknown IOREQ type!!!" );

    }
#endif
}


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



void IOREQ::SetIOREQType( const IOREQTYPE ioreqtypeNext, const IOMETHOD iomethod )
{

    ValidateIOREQTypeTransition( ioreqtypeNext );


    m_ioreqtype = ioreqtypeNext;

    if ( iomethod != iomethodNone )
    {
        m_iomethod = iomethod;
    }


    ASSERT_VALID( this );

}

VOID IOREQ::BeginIO( const IOMETHOD iomethod, const HRT hrtNow, const BOOL fHead )
{

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


    if ( fHead )
    {
        Assert( iomethodNone != iomethod );



        m_posdCurrentIO = p_osf->m_posd;
        Assert( m_posdCurrentIO );


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

        Assert( FOtherIssuedState() ||
                FRemovedFromQueue() ||
                ( cbData == 0 && FIssuedSyncIO() )  );
    }
    else
    {

        Assert( m_ciotime > 0 );
    }
#endif


    if ( m_ciotime > 0 )
    {
        g_patrolDogSync.LeavePerimeter();
    }


    while ( m_ciotime > 0 )
    {

        (void)AtomicCompareExchange( (LONG*)&(m_ciotime), m_ciotime, 0 );
    }
    Assert( m_ciotime == 0 );


    m_iomethod = iomethodNone;

    
    if ( eCompletionStatus == CompletedIO )
    {

        SetIOREQType( ioreqCompleted );
    }
    else
    {

        Assert( eCompletionStatus == ReEnqueueingIO );
    }

    Assert( !FInIssuedState() || eCompletionStatus == ReEnqueueingIO );
    Assert( m_ciotime == 0 );
    Assert( m_iomethod == iomethodNone );

    m_posdCurrentIO = NULL;


    m_grbitHungActionsTaken = 0;

    m_crit.Leave();
}

#ifdef _WIN64
C_ASSERT( sizeof(OVERLAPPED) == 32 );
C_ASSERT( sizeof( CPool< IOREQ, IOREQ::OffsetOfAPIC >::CInvasiveContext ) == 16 );
C_ASSERT( (OffsetOf( IOREQ, m_apic )%8) == 0 );
C_ASSERT( sizeof(IOREQ) <= 256+32 );
C_ASSERT( (sizeof(IOREQ)%8) == 0 );

#else
C_ASSERT( sizeof(OVERLAPPED) == 20 );
C_ASSERT( sizeof( CPool< IOREQ, IOREQ::OffsetOfAPIC >::CInvasiveContext ) == 8 );
C_ASSERT( (OffsetOf( IOREQ, m_apic )%4) == 0 );
C_ASSERT( sizeof(IOREQ) <= 176 + 64 );
C_ASSERT( (sizeof(IOREQ)%4) == 0 );

#endif




typedef ERR (*PfnErrProcessIOREQ)( __in ULONG ichunk, __in ULONG iioreq, __in IOREQ * pioreq, void * pctx );
typedef ERR (*PfnErrEnumerateIOREQ)( __in ULONG ichunk, __in ULONG iioreq, __in const IOREQ * pioreq, void * pctx );

ERR ErrProcessIOREQs( __in PfnErrProcessIOREQ pfnErrProcessIOREQ, void * pvCtx )
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

ERR ErrEnumerateIOREQs( __in PfnErrEnumerateIOREQ pfnErrEnumerateIOREQ, void * pvCtx )
{
    return ErrProcessIOREQs( (PfnErrProcessIOREQ) pfnErrEnumerateIOREQ, pvCtx );
}


#ifdef DEBUG


ERR ErrEnumerateIOREQsPrint( __in ULONG ichunk, __in ULONG iioreq, __in const IOREQ * pioreq, void * pvCtx )
{
    wprintf(L"IOREQ[%d.%d] = %p, state = %d\n", ichunk, iioreq, pioreq, pioreq->Ioreqtype() );
    return JET_errSuccess;
}


ERR ErrEnumerateIOREQsAssertValid( __in ULONG ichunk, __in ULONG iioreq, __in const IOREQ * pioreq, void * pvCtx )
{
    ASSERT_VALID( pioreq );
    return JET_errSuccess;
}

#endif

#ifndef RTM


BOOL FOSDiskIIOREQSlowlyCheckIsOurOverlappedIOREQ( const IOREQ * pioreq )
{
    BOOL fOk = fFalse;

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


void OSDiskIIOREQPoolTerm()
{

    if ( g_pioreqpool )
    {
        g_pioreqpool->Term();
        g_pioreqpool->IOREQPool::~IOREQPool();
        OSMemoryHeapFreeAlign( g_pioreqpool );
        g_pioreqpool = NULL;
    }

    if ( g_pioreqrespool )
    {
        Assert( 0 == g_pioreqrespool->Cobject() || 1 == g_pioreqrespool->Cobject() );

        g_pioreqrespool->Term();
        g_pioreqrespool->IOREQPool::~IOREQPool();
        OSMemoryHeapFreeAlign( g_pioreqrespool );
        g_pioreqrespool = NULL;
    }

#ifdef DEBUG

    CallS( ErrEnumerateIOREQs( ErrEnumerateIOREQsAssertValid, NULL ) );
#endif


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
        g_pioreqchunkRoot->pioreqchunkNext = NULL;
        OSMemoryHeapFree( g_pioreqchunkRoot );
        g_pioreqchunkRoot = pioreqchunkNext;
    }

    g_critIOREQPool.Leave();

}


ERR ErrOSDiskIIOREQPoolInit()
{
    ERR     err;


    Assert( NULL == g_pioreqchunkRoot );

    g_pioreqchunkRoot = NULL;
    g_cioreqInUse = 0;
    g_pioreqpool  = NULL;
    g_pioreqrespool = NULL;


    if (  ( (~(DWORD)0) / sizeof( IOREQ ) < g_cioOutstandingMax ) )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }

    C_ASSERT( ( 1024 - sizeof( IOREQCHUNK ) ) >= 4 );
    g_cbIoreqChunk = 1024;
#ifndef DEBUG
    if ( g_cioOutstandingMax > 256 )
    {
        g_cbIoreqChunk = 64 * 1024;
    }
#endif

    Alloc( g_pioreqchunkRoot = (IOREQCHUNK*)PvOSMemoryHeapAlloc( g_cbIoreqChunk ) );
    memset( g_pioreqchunkRoot, 0, g_cbIoreqChunk );
    g_pioreqchunkRoot->cioreq = CioreqPerChunk( g_cbIoreqChunk );
    g_pioreqchunkRoot->cioreqMac = 0;
    g_pioreqchunkRoot->pioreqchunkNext = NULL;


    BYTE *rgbIOREQPool;
    rgbIOREQPool = (BYTE*)PvOSMemoryHeapAllocAlign( sizeof( IOREQPool ), cbCacheLine );
    if ( !rgbIOREQPool )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }
    g_pioreqpool = new( rgbIOREQPool ) IOREQPool();


    switch ( g_pioreqpool->ErrInit( 0.0 ) )
    {
        default:
            AssertSz( fFalse, "Unexpected error initializing IOREQ Pool" );
        case IOREQPool::ERR::errOutOfMemory:
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        case IOREQPool::ERR::errSuccess:
            break;
    }


    Call( ErrOSDiskIIOREQCreate() );



    rgbIOREQPool = (BYTE*)PvOSMemoryHeapAllocAlign( sizeof( IOREQPool ), cbCacheLine );
    if ( !rgbIOREQPool )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }
    g_cioreqrespool = g_cioreqrespoolleak = 0;
    g_pioreqrespool = new( rgbIOREQPool ) IOREQPool();


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




const TICK                  g_dtickIOTime = OnDebugOrRetail( 150, 4 * 1000 );
const LONG                  g_ciotimeMax = 0x7fff;

C_ASSERT( ( (__int64)g_dtickIOTime * (__int64)g_ciotimeMax ) < (__int64)0x7fffffff  );
C_ASSERT( g_dtickIOTime != 0 );


typedef struct tagHUNGIOCTX
{
    DWORD                                           cioActive;
    DWORD                                           ciotimeLongestOutstanding;
    QWORD                                           cmsecLongestOutstanding;

    CInvasiveList< IOREQ, IOREQ::OffsetOfHIIC >     ilHungIOs;
} HUNGIOCTX;




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
#endif
}

ERR PatrolDogSynchronizer::ErrInitPatrolDog( const PUTIL_THREAD_PROC pfnPatrolDog,
                                                const EThreadPriority priority,
                                                void* const pvContext )
{
    ERR err = JET_errSuccess;


    if ( pfnPatrolDog == NULL )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }


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

    LONG state = AtomicCompareExchange( (LONG*)&m_patrolDogState, pdsActive, pdsDeactivating );

    if ( state != pdsActive )
    {
        return;
    }


    Assert( !m_fPutDownPatrolDog );
    Assert( m_cLoiters == 0 );
    Assert( m_threadPatrolDog != NULL );


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


    if ( cLoitersNew == 0 )
    {
        m_asigNudgePatrolDog.Set();
    }
}

BOOL PatrolDogSynchronizer::FCheckForLoiter( const TICK dtickTimeout )
{
    Assert( m_patrolDogState == pdsActivating || m_patrolDogState == pdsActive || m_patrolDogState == pdsDeactivating );


    Assert( dtickTimeout <= INT_MAX );

    BOOL fLoitersBefore = fFalse;

    OSSYNC_FOREVER
    {

        if ( m_fPutDownPatrolDog )
        {
            return fFalse;
        }

        const BOOL fLoitersNow = ( m_cLoiters > 0 );


        if ( fLoitersNow && fLoitersBefore )
        {
            return fTrue;
        }


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




PatrolDogSynchronizer g_patrolDogSync;


void OSDiskIIOPatrolDogThreadTerm( void )
{
    g_patrolDogSync.TermPatrolDog();
}


DWORD IOMgrIOPatrolDogThread( DWORD_PTR dwContext );


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


ULONG CmsecLowWaitFromIOTime( const ULONG ciotime )
{
    if ( ciotime > 2 )
    {
        const ULONG cmsec = ( ciotime - 2 ) * g_dtickIOTime;
        Assert( cmsec <= ( g_dtickIOTime * g_ciotimeMax )  );
        return cmsec;
    }
    return 0;
}


ULONG CmsecHighWaitFromIOTime( const ULONG ciotime )
{
    if ( ciotime >= 2 )
    {
        const ULONG cmsec = ciotime * g_dtickIOTime;
        Assert( cmsec >= g_dtickIOTime / 2 );
        Assert( cmsec <= ( g_dtickIOTime * g_ciotimeMax )  );
        return cmsec;
    }
    return g_dtickIOTime;
}

VOID IOREQ::IncrementIOTime()
{
    C_ASSERT( sizeof(m_ciotime) == 4 );
    C_ASSERT( ( OffsetOf( IOREQ, m_ciotime ) % 4 ) == 0 );



    DWORD ciotimePre;


    while( ( ciotimePre = m_ciotime ) > 0 &&
            ciotimePre < g_ciotimeMax )
    {


        const DWORD ciotimeCheck = AtomicCompareExchange( (LONG*)&(m_ciotime), ciotimePre, ciotimePre + 1 );


        if ( ciotimeCheck == ciotimePre )
        {
            break;
        }


    }

}


ERR ErrIOMgrPatrolDogICheckOutIOREQ( __in ULONG ichunk, __in ULONG iioreq, __in IOREQ * pioreq, void * pctx )
{

    if ( pioreq->FOSIssuedState() )
    {
        pioreq->m_crit.Enter();


        if ( pioreq->FOSIssuedState() )
        {
            HUNGIOCTX * const pHungIOCtx = reinterpret_cast<HUNGIOCTX *>( pctx );


            if ( pioreq->m_ciotime > 0 )
            {
                pHungIOCtx->cioActive++;
            }


            const QWORD cmsec = CmsecHRTFromDhrt( HrtHRTCount() - pioreq->hrtIOStart );

            if ( pioreq->m_ciotime && cmsec > pHungIOCtx->cmsecLongestOutstanding )
            {
                pHungIOCtx->cmsecLongestOutstanding = cmsec;
            }

            if ( pioreq->m_ciotime > pHungIOCtx->ciotimeLongestOutstanding )
            {
                pHungIOCtx->ciotimeLongestOutstanding = pioreq->m_ciotime;
            }


            if ( CmsecLowWaitFromIOTime( pioreq->m_ciotime ) > pioreq->p_osf->Pfsconfig()->DtickHungIOThreshhold() ||
                 FFaultInjection( 36002 ) )
            {
                pHungIOCtx->ilHungIOs.InsertAsPrevMost( pioreq );
            }
        }

        pioreq->m_crit.Leave();
    }


    pioreq->IncrementIOTime();

    return JET_errSuccess;
}

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
    const OSDiskIReportHungIOFailureItem        failureItem
    )
{
    const COSDisk::IORun    iorun( (IOREQ*)pioreqHead );

    if ( pioreqHead->pioreqIorunNext && FFaultInjection( 36002 )  )
    {
        UtilSleep( 1 );
    }

    Assert( pioreqHead->FOwner() );

    IFileAPI * pfapi = (IFileAPI*)pioreqHead->p_osf->keyFileIOComplete;

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


    TICK dtickNextScheduledPatrol = g_dtickIOTime;

    while ( g_patrolDogSync.FCheckForLoiter( dtickNextScheduledPatrol ) )
    {
        const TICK tickStartWatch = TickOSTimeCurrent();
        g_tickPatrolDogLastRun = tickStartWatch;


        HUNGIOCTX HungIOCtx = { 0 };

        Assert( HungIOCtx.cioActive == 0 );
        Assert( HungIOCtx.ciotimeLongestOutstanding == 0 );
        Assert( HungIOCtx.cmsecLongestOutstanding == 0 );
        Expected( IOREQ::OffsetOfHIIC() == (size_t) *((void**)(&(HungIOCtx.ilHungIOs))) );


        CallS( ErrProcessIOREQs( ErrIOMgrPatrolDogICheckOutIOREQ, &HungIOCtx ) );


        COSDisk *   posdFirst = NULL;
        BOOL        fDoubleDiskWhammy = fFalse;
        IOREQ *     pioreqDoubleDiskCanary = NULL;
        DWORD       cioHung = 0;

        for ( IOREQ * pioreq = HungIOCtx.ilHungIOs.PrevMost(); pioreq; pioreq = HungIOCtx.ilHungIOs.PrevMost() )
        {
            cioHung++;

            pioreq->m_crit.Enter();

            TICK dtickLowestHungIoThreshold;
            if (    pioreq->Ciotime() &&
                    ( CmsecLowWaitFromIOTime( pioreq->Ciotime() ) > ( dtickLowestHungIoThreshold = pioreq->p_osf->Pfsconfig()->DtickHungIOThreshhold() )
                      OnDebug( || ChitsFaultInj( 36002 ) )
                    )
               )
            {
                Expected( pioreq->FInIssuedState() );

                const DWORD grbitHungIOActions = pioreq->p_osf->Pfsconfig()->GrbitHungIOActions();

                Assert( ( dtickLowestHungIoThreshold == ( g_dtickIOTime * g_ciotimeMax + 1 ) ) ||
                        ( g_dtickIOTime < ( dtickLowestHungIoThreshold / 4 ) ) );


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
                        posdFirst = pioreq->m_posdCurrentIO;
                    }
                    else if ( posdFirst == pioreq->m_posdCurrentIO )
                    {
                    }
                    else
                    {

                        fDoubleDiskWhammy = fTrue;
                        pioreqDoubleDiskCanary = pioreq;
                    }

                }

            }


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


        const TICK dtickCurrentPatrol = (TICK)DtickDelta( tickStartWatch, TickOSTimeCurrent() );
        dtickNextScheduledPatrol = g_dtickIOTime - UlFunctionalMin( dtickCurrentPatrol, g_dtickIOTime - 100 );
        C_ASSERT( g_dtickIOTime >= 100 );
    }

    return JET_errSuccess;
}



CSXWLatch g_sxwlOSDisk( CLockBasicInfo( CSyncBasicInfo( "OS Disk SXWL" ), rankOSDiskSXWL, 0 ) );
CInvasiveList< COSDisk, COSDisk::OffsetOfILE >  g_ilDiskList;



void OSDiskPostterm()
{
    Assert( g_ilDiskList.FEmpty() || FUtilProcessAbort() || FNegTest( fLeakStuff ) );
}


BOOL FOSDiskPreinit()
{
    return fTrue;
}

void AssertIoContextQuiesced( __in const _OSFILE * p_osf );


void OSDiskTerm()
{

    AssertIoContextQuiesced( NULL );


    OSTrace( JET_tracetagDiskVolumeManagement,
            OSFormat( "End Stats: g_cioreqInUse(High Water)=%d", g_cioreqInUse ) );


    OSDiskIIOPatrolDogThreadTerm();

    OSDiskIIOThreadTerm();


    OSDiskIIOREQPoolTerm();


    Assert( g_ilDiskList.FEmpty() );

    SetDiskMappingMode( eOSDiskInvalidMode );

    if ( g_rgbGapping )
    {
        OSMemoryPageFree( g_rgbGapping );
        g_rgbGapping = NULL;
    }
}


ERR ErrOSDiskInit()
{
    ERR     err     = JET_errSuccess;


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
            lValue = (LONG)g_diskModeDefault;
        }
    }
    
    Assert( g_ilDiskList.FEmpty() );
    SetDiskMappingMode( (OSDiskMappingMode)lValue );

    Assert( NULL == g_rgbGapping );
    if ( NULL == ( g_rgbGapping = ( BYTE* )PvOSMemoryPageAlloc( OSMemoryPageCommitGranularity(), NULL ) ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }


    Call( ErrOSDiskIIOREQPoolInit() );


    Call( ErrOSDiskIIOThreadInit() );

    Call( ErrOSDiskIIOPatrolDogThreadInit() );

    return JET_errSuccess;

HandleError:
    OSDiskTerm();
    return err;
}



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

    if ( !m_traceidcheckDisk.FAnnounceTime< _etguidDiskStationId >( tsidr ) )
    {
        return;
    }

    Expected( tsidr == tsidrOpenInit ||
                tsidr == tsidrPulseInfo );

    const DWORD dwDiskNumber = DwDiskNumber();

    const char * szDiskModelBest = ( strstr( m_osdi.m_szDiskModelSmart, m_osdi.m_szDiskModel ) != NULL ) ? m_osdi.m_szDiskModelSmart : m_osdi.m_szDiskModel;

    ETDiskStationId( tsidr, dwDiskNumber, WszDiskPathId(), szDiskModelBest, m_osdi.m_szDiskFirmwareRev, m_osdi.m_szDiskSerialNumber );

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

    Assert( posd->m_cref < 0x7FFFFFFF );

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
            OSTrace( JET_tracetagDiskVolumeManagement, OSFormat( "Deleted Disk p=%p, PathId=%ws", posd, posd->WszDiskPathId() ) );

            Assert( 0 == posd->CioOutstanding() );
            OSDiskIDelete( posd );
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


    Assert( g_sxwlOSDisk.FOwnExclusiveLatch() );
}


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
    BOOL fFakeNoSeekPenalty = ErrFaultInjection( 17353 );

    return !fFakeNoSeekPenalty &&
        (   m_osdi.m_errorOsdspd != ERROR_SUCCESS ||
            m_osdi.m_osdspd.Size < 12  ||
            m_osdi.m_osdspd.IncursSeekPenalty );
}

ERR ErrValidateNotUsingIoContext( __in ULONG ichunk, __in ULONG iioreq, __in const IOREQ * pioreq, void * pvCtx )
{
    __in const _OSFILE * p_osf = (_OSFILE *)pvCtx;

    if ( p_osf )
    {
        Assert( p_osf != pioreq->p_osf );
    }
    else
    {
        Assert( NULL == pioreq->p_osf );
    }

    return JET_errSuccess;
}


void AssertIoContextQuiesced( __in const _OSFILE * p_osf )
{
    (void)ErrEnumerateIOREQs( ErrValidateNotUsingIoContext, (void*)p_osf );
}


ULONG CioDefaultUrgentOutstandingIOMax( __in const ULONG cioOutstandingMax )
{
    return cioOutstandingMax / 2;
}

char * SzSkipPreceding( char * szT, char chSkip )
{
    char * szStart = szT;
    for( INT ich = 0; szT[ich] != L'\0'; ich++ )
    {
        if ( szT[ich] != chSkip )
        {
            break;
        }
        szStart = &( szT[ ich + 1 ] );
    }

    return szStart;
}

VOID ConvertSmartDriverString( BYTE * sSmartDriverString, INT cbSmartDriverString, PSTR szProperString, INT cbProperString )
{
    Assert( cbProperString < 50 );
    char * szT = (char*)alloca( cbProperString );

    Assert( cbSmartDriverString < cbProperString );

    for( INT ich = 0; ich < cbSmartDriverString; ich += 2  )
    {
        szT[ich] = sSmartDriverString[ich+1];
        szT[ich+1] = sSmartDriverString[ich];
    }

    szT[ cbSmartDriverString ] = '\0';

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

    for( INT ich = cbSmartDriverString - 1; ich >= 0; ich-- )
    {
        if ( szT[ich] != ' ' )
        {
            break;
        }
        szT[ich] = '\0';
    }

    char * szStart = szT;
    for( INT ich = 0; ich < ( cbSmartDriverString - 1 ); ich++ )
    {
        if ( szT[ich] != ' ' )
        {
            break;
        }
        szStart = &( szT[ ich + 1 ] );
    }

    OSStrCbCopyA( szProperString, cbProperString, szStart );
}

void COSDisk::SetSmartEseNoLoadFailed( __in const ULONG iStep, __in const DWORD error, __out_bcount_z(cbIdentifier) CHAR * szIdentifier, __in const ULONG cbIdentifier )
{
    OSStrCbFormatA( szIdentifier, cbIdentifier, szEseNoLoadSmartData, iStep, error );

    CHAR szNoLoadFireWallMsg[ 50 ];
    Assert( strlen( szIdentifier ) + 15 < _countof(szNoLoadFireWallMsg) );
    OSStrCbFormatA( szNoLoadFireWallMsg, sizeof(szNoLoadFireWallMsg), "%hs%hs", "SmartData", szIdentifier );

    OSTrace( JET_tracetagFile, OSFormat( "Failed S.M.A.R.T. load: %hs \n", szNoLoadFireWallMsg ) );
    
}

void COSDisk::LoadDiskInfo_( __in_z PCWSTR wszDiskPath, __in const DWORD dwDiskNumber )
{
    BOOL fSuccess;
    DWORD cbRet;
    HRT hrtStart;

    Assert( m_osdi.m_grbitDiskSmartCapabilities == 0 );
    Assert( m_osdi.m_szDiskModel[0] == '\0' );
    Assert( m_osdi.m_szDiskSerialNumber[0] == '\0' );
    Assert( m_osdi.m_szDiskFirmwareRev[0] == '\0' );

    Expected( m_dwDiskNumber == dwDiskNumber );


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
            SetSmartEseNoLoadFailed( eSmartLoadDiskOpenFailed, error, m_osdi.m_szDiskModel, sizeof(m_osdi.m_szDiskModel) );
            SetSmartEseNoLoadFailed( eSmartLoadDiskOpenFailed, error, m_osdi.m_szDiskSerialNumber, sizeof(m_osdi.m_szDiskSerialNumber) );
            OSStrCbCopyA( m_osdi.m_szDiskFirmwareRev, sizeof(m_osdi.m_szDiskFirmwareRev), "EseNoLo" );

            return;
        }
    }


    STORAGE_PROPERTY_QUERY osspqSdp = { StorageDeviceProperty, PropertyStandardQuery, 0 };
    BYTE rgbDevDesc[1024] = { 0 };
    STORAGE_DEVICE_DESCRIPTOR* possdd = (STORAGE_DEVICE_DESCRIPTOR *)rgbDevDesc;
    C_ASSERT( sizeof(*possdd) <= sizeof(rgbDevDesc) );

    hrtStart = HrtHRTCount();
    fSuccess = DeviceIoControl( hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &osspqSdp, sizeof(osspqSdp), possdd, sizeof(rgbDevDesc), &cbRet, NULL );
    if ( !fSuccess || cbRet < sizeof(*possdd) )
    {
        const DWORD error = GetLastError();
        OSTrace( JET_tracetagFile, OSFormat( "FAILED: DevIoCtrl( query prop \\ StorageDeviceProperty ) -> %d / %d / %d in %I64u us \n", fSuccess, cbRet, error, CusecHRTFromDhrt( HrtHRTCount() - hrtStart ) ) );

        SetSmartEseNoLoadFailed( eSmartLoadDiskOpenFailed, error, m_osdi.m_szDiskModel, sizeof(m_osdi.m_szDiskModel) );
        SetSmartEseNoLoadFailed( eSmartLoadDiskOpenFailed, error, m_osdi.m_szDiskSerialNumber, sizeof(m_osdi.m_szDiskSerialNumber) );
        OSStrCbCopyA( m_osdi.m_szDiskFirmwareRev, sizeof(m_osdi.m_szDiskFirmwareRev), "EseNoLo" );

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
        OSTrace( JET_tracetagFile, OSFormat( "\t\t ->Vendor       = %hs;\n", possdd->VendorIdOffset ? ( (char*) &rgbDevDesc[possdd->VendorIdOffset] ) : "<null>" ) );
        OSTrace( JET_tracetagFile, OSFormat( "\t\t ->Product      = %hs;\n", m_osdi.m_szDiskModel ) );
        OSTrace( JET_tracetagFile, OSFormat( "\t\t ->SerialNum    = %hs;\n", m_osdi.m_szDiskSerialNumber ) );
        OSTrace( JET_tracetagFile, OSFormat( "\t\t ->ProductRev   = %hs;\n", m_osdi.m_szDiskFirmwareRev ) );
    }


    BOOL fSmartCmds = fFalse;

    cbRet = 0x42;
    GETVERSIONINPARAMS osgvip = { 0 };
    hrtStart = HrtHRTCount();
    fSuccess = DeviceIoControl( hDisk, SMART_GET_VERSION, NULL, 0, &osgvip, sizeof(osgvip), &cbRet, NULL);
    if ( !fSuccess || cbRet < sizeof(osgvip) )
    {

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

        fSmartCmds = ( m_osdi.m_bDiskSmartVersion == 1 &&
                        m_osdi.m_bDiskSmartRevision == 1 &&
                        m_osdi.m_grbitDiskSmartCapabilities & CAP_SMART_CMD );
        if ( !fSmartCmds )
        {
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

        #define eSmartPacketCmd  (0xA0)

        SENDCMDINPARAMS osscip = {0};

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
            Assert( ( ((BYTE*)pCOP) + sizeof( SENDCMDOUTPARAMS ) - 1 ) == pCOP->bBuffer );
            SmartIdSector * pidsec = (SmartIdSector *)pCOP->bBuffer;

            CHAR szSmartDiskSerialNumber[ sizeof(pidsec->sSerialNumber) + 1 ];
            CHAR szSmartDiskFirmwareRev[ sizeof(pidsec->sFirmwareRev) + 1 ];

            ConvertSmartDriverString( pidsec->sModelNumber, sizeof(pidsec->sModelNumber), m_osdi.m_szDiskModelSmart, sizeof(m_osdi.m_szDiskModelSmart) );
            ConvertSmartDriverString( pidsec->sSerialNumber, sizeof(pidsec->sSerialNumber), szSmartDiskSerialNumber, sizeof(szSmartDiskSerialNumber) );
            ConvertSmartDriverString( pidsec->sFirmwareRev, sizeof(pidsec->sFirmwareRev), szSmartDiskFirmwareRev, sizeof(szSmartDiskFirmwareRev) );

            OSTrace( JET_tracetagFile, OSFormat( "Full S.M.A.R.T. Versions: %hs - %hs - %hs\n",
                        m_osdi.m_szDiskModelSmart, szSmartDiskFirmwareRev, szSmartDiskSerialNumber ) );
        }
    }



    LoadCachePerf_( hDisk );

    CloseHandle( hDisk );
    hDisk = INVALID_HANDLE_VALUE;
}

DWORD ErrorOSDiskIOsStorageQueryProp_( HANDLE hDisk, STORAGE_PROPERTY_ID ePropId, const CHAR * const szPropId, void * pvOsStruct, const ULONG cbOsStruct )
{
    DWORD cbRet = 0x42;
    STORAGE_PROPERTY_QUERY osspq = { ePropId, PropertyStandardQuery, 0 };
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
        return 0xFFFFFFF2;
    }
    if ( cbRet < cbOsStruct )
    {
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
        OSTrace( JET_tracetagFile, OSFormat( "FAILED: DevIoCtrl( IOCTL_DISK_GET_CACHE_INFORMATION ) -> %d / %d / %d in %I64u us \n", fSuccess, cbRet, GetLastError(), CusecHRTFromDhrt( HrtHRTCount() - hrtStart ) ) );
    }
    else
    {
        OSTrace( JET_tracetagFile, OSFormat( "\t m_osdi.m_osdci = { ParamSavable=%d, Read/Write.CacheEnabled=%d/%d, Read/Write.RetentionPriority=%d/%d, S/B=%d - [ %d - %d, %d } }\n",
                    m_osdi.m_osdci.ParametersSavable, m_osdi.m_osdci.ReadCacheEnabled, m_osdi.m_osdci.WriteCacheEnabled, m_osdi.m_osdci.ReadRetentionPriority, m_osdi.m_osdci.WriteRetentionPriority,
                    m_osdi.m_osdci.PrefetchScalar, m_osdi.m_osdci.ScalarPrefetch.Minimum, m_osdi.m_osdci.ScalarPrefetch.Maximum, m_osdi.m_osdci.PrefetchScalar ? m_osdi.m_osdci.ScalarPrefetch.MaximumBlocks : 0 ) );
    }

    m_osdi.m_errorOsswcp = ERROR_INVALID_PARAMETER;
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
                    m_osdi.m_ossad.AdapterUsesPio,  m_osdi.m_ossad.CommandQueueing, m_osdi.m_ossad.AcceleratedTransfer,
                    m_osdi.m_ossad.BusType, m_osdi.m_ossad.BusMajorVersion, m_osdi.m_ossad.BusMinorVersion ) );
    }

    m_osdi.m_errorOsdspd = ErrorOSDiskIOsStorageQueryProp( hDisk, StorageDeviceSeekPenaltyProperty, &m_osdi.m_osdspd, sizeof(m_osdi.m_osdspd) );
    if ( m_osdi.m_errorOsdspd == ERROR_SUCCESS )
    {
        OSTrace( JET_tracetagFile, OSFormat( "\t m_osdi.m_osdspd = { Ver.Size=%d.%d, IncursSeekPenalty=%d };\n",
                    m_osdi.m_osdspd.Version, m_osdi.m_osdspd.Size, m_osdi.m_osdspd.IncursSeekPenalty ) );
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

}



ERR COSDisk::ErrInitDisk( __in_z const WCHAR * const wszDiskPathId, __in const DWORD dwDiskNumber )
{
    ERR         err     = JET_errSuccess;
    const INT       cchBuf      = 256;
    WCHAR           wszBuf[ cchBuf ];

    m_dwDiskNumber = dwDiskNumber;
    Call( ErrOSStrCbCopyW( m_wszDiskPathId, sizeof(m_wszDiskPathId), wszDiskPathId ) );

    Call( ErrFaultInjection( 17352 ) );
    Alloc( m_pIOQueue = new IOQueue( &m_critIOQueue ) );


    ULONG cioOutstandingMax = g_cioOutstandingMax;
    if ( FOSConfigGet( L"OS/IO", L"Total Quota", wszBuf, sizeof( wszBuf ) )
        && wszBuf[0] )
    {
        cioOutstandingMax = _wtol( wszBuf );
    }

    ULONG cioBackgroundMax = g_cioBackgroundMax;
    if ( FOSConfigGet( L"OS/IO", L"Background Quota", wszBuf, sizeof( wszBuf ) )
        && wszBuf[0] )
    {
        cioBackgroundMax = _wtol( wszBuf );
    }


    ULONG cioUrgentBackMax = CioDefaultUrgentOutstandingIOMax( cioOutstandingMax );
    if ( FOSConfigGet( L"OS/IO", L"Urgent Background Quota Max", wszBuf, sizeof( wszBuf ) )
        && wszBuf[0] )
    {
        cioUrgentBackMax = _wtol( wszBuf );
    }


    if ( cioOutstandingMax < 4 )
    {
        AssertSz( fFalse, "Total Quota / JET_paramOutstandingIOMax too low." );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    cioBackgroundMax = UlBound( cioBackgroundMax, 1, cioOutstandingMax - 2 );
    cioUrgentBackMax = UlBound( cioUrgentBackMax, cioBackgroundMax+1, cioOutstandingMax - 1 );
    
    Assert( cioBackgroundMax <= ( cioOutstandingMax / 2 ) );
    Assert( cioUrgentBackMax > cioBackgroundMax );


    Call( m_pIOQueue->ErrIOQueueInit( cioOutstandingMax, cioBackgroundMax, cioUrgentBackMax ) );


    WCHAR wszDiskPath[IFileSystemAPI::cchPathMax];
    OSStrCbFormatW( wszDiskPath, sizeof( wszDiskPath ), L"\\\\.\\PhysicalDrive%u", dwDiskNumber );

    m_hDisk = CreateFileW(  wszDiskPath,
                            0,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );

    LoadDiskInfo_( wszDiskPath, dwDiskNumber );

    m_eState = eOSDiskConnected;

HandleError:

    return err;
}


ERR ErrOSDiskICreate( __in_z const WCHAR * const wszDiskPathId, const DWORD dwDiskNumber, __out COSDisk ** pposd )
{
    ERR         err     = JET_errSuccess;
    COSDisk *   posd    = NULL;

    Assert( g_sxwlOSDisk.FOwnExclusiveLatch() );


    CSXWLatch::ERR errSXW = g_sxwlOSDisk.ErrUpgradeExclusiveLatchToWriteLatch();
    if ( CSXWLatch::ERR::errWaitForWriteLatch == errSXW )
    {
        g_sxwlOSDisk.WaitForWriteLatch();
        errSXW = CSXWLatch::ERR::errSuccess;
    }
    Assert( errSXW == CSXWLatch::ERR::errSuccess );

    Call( ErrFaultInjection( 17348 ) );
    Alloc( posd = new COSDisk() );

    Call( posd->ErrInitDisk( wszDiskPathId, dwDiskNumber ) );


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

ERR ErrOSDiskIFind( __in_z const WCHAR * const wszDiskPath, __out COSDisk ** pposd )
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

                g_sxwlOSDisk.AcquireSharedLatch();

                m_posdCurr = g_ilDiskList.PrevMost();
            }
            else
            {

                g_sxwlOSDisk.AcquireExclusiveLatch();

                COSDisk * posdPrev = m_posdCurr;

                m_posdCurr = g_ilDiskList.Next( m_posdCurr );

                COSDisk::Release( posdPrev );

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

                COSDisk::Release( m_posdCurr );

                m_posdCurr = NULL;

                g_sxwlOSDisk.ReleaseExclusiveLatch();
            }
        }

        ~OSDiskEnumerator()
        {
            Assert( NULL == m_posdCurr );

            Quit();
        }

};

ERR ErrOSDiskConnect( __in_z const WCHAR * const wszDiskPathId, __in const DWORD dwDiskNumber, __out IDiskAPI ** ppdiskapi )
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
        Call( ErrOSDiskICreate( wszDiskPathId, dwDiskNumber, &posd ) );
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



void OSDiskDisconnect(
    __inout IDiskAPI *          pdiskapi,
    __in    const _OSFILE *     p_osf )
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






ERR COSDisk::IOQueue::IOHeapA::ErrHeapAInit( __in LONG cIOEnqueuedMax )
{
    ERR err;


    rgpioreqIOAHeap = NULL;

    ipioreqIOAHeapMax = cIOEnqueuedMax;

    if( ((0x7FFFFFFF / ipioreqIOAHeapMax) <= sizeof(IOREQ*)) ||
        ((ipioreqIOAHeapMax * sizeof(IOREQ*)) < max(ipioreqIOAHeapMax, sizeof(IOREQ*)))
#ifdef DEBUGGER_EXTENSION
        ||
        ((ipioreqIOAHeapMax * sizeof(IOREQ*)) < 0)
#endif
        )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    Call( ErrFaultInjection( 17368 ) );
    if ( !( rgpioreqIOAHeap = (IOREQ**) PvOSMemoryHeapAlloc( ipioreqIOAHeapMax * sizeof( IOREQ* ) ) ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }


    ipioreqIOAHeapMac = 0;

    return JET_errSuccess;

HandleError:
    _HeapATerm();
    return err;
}


void COSDisk::IOQueue::IOHeapA::_HeapATerm()
{
    Assert( 0 == CioreqHeapA() );


    if ( rgpioreqIOAHeap != NULL )
    {
        OSMemoryHeapFree( (void*)rgpioreqIOAHeap );
        rgpioreqIOAHeap = NULL;
    }
}


INLINE BOOL COSDisk::IOQueue::IOHeapA::FHeapAEmpty() const
{
    Assert( m_pcrit->FOwner() );
    return !ipioreqIOAHeapMac;
}


INLINE LONG COSDisk::IOQueue::IOHeapA::CioreqHeapA() const
{
    return ipioreqIOAHeapMac;
}


INLINE IOREQ* COSDisk::IOQueue::IOHeapA::PioreqHeapATop()
{
    Assert( m_pcrit->FOwner() );
    Assert( !FHeapAEmpty() );
    return rgpioreqIOAHeap[0];
}


INLINE void COSDisk::IOQueue::IOHeapA::HeapAAdd( IOREQ* pioreq )
{

    Assert( m_pcrit->FOwner() );


    LONG ipioreq = ipioreqIOAHeapMac++;


    while ( ipioreq > 0 &&
            FHeapAISmaller( pioreq, rgpioreqIOAHeap[IpioreqHeapAIParent( ipioreq )] ) )
    {
        Assert( rgpioreqIOAHeap[IpioreqHeapAIParent( ipioreq )]->ipioreqHeap == IpioreqHeapAIParent( ipioreq ) );
        rgpioreqIOAHeap[ipioreq] = rgpioreqIOAHeap[IpioreqHeapAIParent( ipioreq )];
        rgpioreqIOAHeap[ipioreq]->ipioreqHeap = ipioreq;
        ipioreq = IpioreqHeapAIParent( ipioreq );
    }


    rgpioreqIOAHeap[ipioreq] = pioreq;
    pioreq->ipioreqHeap = ipioreq;
}


INLINE void COSDisk::IOQueue::IOHeapA::HeapARemove( IOREQ* pioreq )
{

    Assert( m_pcrit->FOwner() );


    LONG ipioreq = pioreq->ipioreqHeap;


    if ( ipioreq == ipioreqIOAHeapMac - 1 )
    {
#ifdef DEBUG
        rgpioreqIOAHeap[ipioreqIOAHeapMac - 1] = (IOREQ*) 0xBAADF00DBAADF00D;
#endif
        ipioreqIOAHeapMac--;
        return;
    }


    rgpioreqIOAHeap[ipioreq] = rgpioreqIOAHeap[ipioreqIOAHeapMac - 1];
    rgpioreqIOAHeap[ipioreq]->ipioreqHeap = ipioreq;
#ifdef DEBUG
    rgpioreqIOAHeap[ipioreqIOAHeapMac - 1] = (IOREQ*) 0xBAADF00DBAADF00D;
#endif
    ipioreqIOAHeapMac--;


    HeapAIUpdate( rgpioreqIOAHeap[ipioreq] );
}


void COSDisk::IOQueue::IOHeapA::HeapAIUpdate( IOREQ* pioreq )
{

    Assert( m_pcrit->FOwner() );


    LONG ipioreq = pioreq->ipioreqHeap;
    Assert( rgpioreqIOAHeap[ipioreq] == pioreq );


    while ( ipioreq > 0 &&
            FHeapAISmaller( pioreq, rgpioreqIOAHeap[IpioreqHeapAIParent( ipioreq )] ) )
    {
        Assert( rgpioreqIOAHeap[IpioreqHeapAIParent( ipioreq )]->ipioreqHeap == IpioreqHeapAIParent( ipioreq ) );
        rgpioreqIOAHeap[ipioreq] = rgpioreqIOAHeap[IpioreqHeapAIParent( ipioreq )];
        rgpioreqIOAHeap[ipioreq]->ipioreqHeap = ipioreq;
        ipioreq = IpioreqHeapAIParent( ipioreq );
    }


    while ( ipioreq < ipioreqIOAHeapMac )
    {

        if ( IpioreqHeapAILeftChild( ipioreq ) >= ipioreqIOAHeapMac )
            break;


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


        if ( FHeapAISmaller( pioreq, rgpioreqIOAHeap[ipioreqChild] ) )
            break;


        Assert( rgpioreqIOAHeap[ipioreqChild]->ipioreqHeap == ipioreqChild );
        rgpioreqIOAHeap[ipioreq] = rgpioreqIOAHeap[ipioreqChild];
        rgpioreqIOAHeap[ipioreq]->ipioreqHeap = ipioreq;
        ipioreq = ipioreqChild;
    }
    Assert( ipioreq < ipioreqIOAHeapMac );


    rgpioreqIOAHeap[ipioreq] = pioreq;
    pioreq->ipioreqHeap = ipioreq;
}


INLINE BOOL COSDisk::IOQueue::IOHeapA::FHeapAISmaller( IOREQ* pioreq1, IOREQ* pioreq2 ) const
{
    return CmpOSDiskIFileIbIFileIb(   pioreq1->p_osf->iFile,
                                        pioreq1->ibOffset,
                                        pioreq2->p_osf->iFile,
                                        pioreq2->ibOffset ) < 0;
}


INLINE LONG COSDisk::IOQueue::IOHeapA::IpioreqHeapAIParent( LONG ipioreq ) const
{
    return ( ipioreq - 1 ) / 2;
}


INLINE LONG COSDisk::IOQueue::IOHeapA::IpioreqHeapAILeftChild( LONG ipioreq ) const
{
    return 2 * ipioreq + 1;
}


INLINE LONG COSDisk::IOQueue::IOHeapA::IpioreqHeapAIRightChild( LONG ipioreq ) const
{
    return 2 * ipioreq + 2;
}

INLINE BOOL COSDisk::IOQueue::IOHeapA::FHeapAFrom( const IOREQ * pioreq ) const
{
    return pioreq->ipioreqHeap < ipioreqIOAHeapMac;
}



ERR COSDisk::IOQueue::IOHeapB::ErrHeapBInit(
                IOREQ* volatile *   rgpioreqIOAHeap,
                LONG                ipioreqIOAHeapMax
    )
{

    Assert( rgpioreqIOAHeap );

    rgpioreqIOBHeap = rgpioreqIOAHeap;
    ipioreqIOBHeapMax = ipioreqIOAHeapMax;


    ipioreqIOBHeapMic = ipioreqIOBHeapMax;

    return JET_errSuccess;
}


void COSDisk::IOQueue::IOHeapB::_HeapBTerm()
{
    Assert( 0 == CioreqHeapB() );
}


INLINE BOOL COSDisk::IOQueue::IOHeapB::FHeapBEmpty() const
{
    Assert( m_pcrit->FOwner() );
    return ipioreqIOBHeapMic == ipioreqIOBHeapMax;
}


INLINE LONG COSDisk::IOQueue::IOHeapB::CioreqHeapB() const
{
    return LONG( ipioreqIOBHeapMax - ipioreqIOBHeapMic );
}


INLINE IOREQ* COSDisk::IOQueue::IOHeapB::PioreqHeapBTop()
{
    Assert( m_pcrit->FOwner() );
    Assert( !FHeapBEmpty() );
    return rgpioreqIOBHeap[ipioreqIOBHeapMax - 1];
}


INLINE void COSDisk::IOQueue::IOHeapB::HeapBAdd( IOREQ* pioreq )
{

    Assert( m_pcrit->FOwner() );


    LONG ipioreq = --ipioreqIOBHeapMic;


    while ( ipioreqIOBHeapMax > 0 &&
            ipioreq < ipioreqIOBHeapMax - 1 &&
            FHeapBISmaller( pioreq, rgpioreqIOBHeap[IpioreqHeapBIParent( ipioreq )] ) )
    {
        Assert( rgpioreqIOBHeap[IpioreqHeapBIParent( ipioreq )]->ipioreqHeap == IpioreqHeapBIParent( ipioreq ) );
        rgpioreqIOBHeap[ipioreq] = rgpioreqIOBHeap[IpioreqHeapBIParent( ipioreq )];
        rgpioreqIOBHeap[ipioreq]->ipioreqHeap = ipioreq;
        ipioreq = IpioreqHeapBIParent( ipioreq );
    }


    rgpioreqIOBHeap[ipioreq] = pioreq;
    pioreq->ipioreqHeap = ipioreq;
}


INLINE void COSDisk::IOQueue::IOHeapB::HeapBRemove( IOREQ* pioreq )
{

    Assert( m_pcrit->FOwner() );


    LONG ipioreq = pioreq->ipioreqHeap;


    if ( ipioreq == ipioreqIOBHeapMic )
    {
#ifdef DEBUG
        rgpioreqIOBHeap[ipioreqIOBHeapMic] = (IOREQ*)0xBAADF00DBAADF00D;
#endif
        ipioreqIOBHeapMic++;
        return;
    }


    rgpioreqIOBHeap[ipioreq] = rgpioreqIOBHeap[ipioreqIOBHeapMic];
    rgpioreqIOBHeap[ipioreq]->ipioreqHeap = ipioreq;
#ifdef DEBUG
    rgpioreqIOBHeap[ipioreqIOBHeapMic] = (IOREQ*)0xBAADF00DBAADF00D;
#endif
    ipioreqIOBHeapMic++;


    HeapBIUpdate( rgpioreqIOBHeap[ipioreq] );
}


INLINE BOOL COSDisk::IOQueue::IOHeapB::FHeapBISmaller( IOREQ* pioreq1, IOREQ* pioreq2 ) const
{
    return CmpOSDiskIFileIbIFileIb(   pioreq1->p_osf->iFile,
                                        pioreq1->ibOffset,
                                        pioreq2->p_osf->iFile,
                                        pioreq2->ibOffset ) < 0;
}


INLINE LONG COSDisk::IOQueue::IOHeapB::IpioreqHeapBIParent( LONG ipioreq ) const
{
    return ipioreqIOBHeapMax - 1 - ( ipioreqIOBHeapMax - 1 - ipioreq - 1 ) / 2;
}


INLINE LONG COSDisk::IOQueue::IOHeapB::IpioreqHeapBILeftChild( LONG ipioreq ) const
{
    return ipioreqIOBHeapMax - 1 - ( 2 * ( ipioreqIOBHeapMax - 1 - ipioreq ) + 1 );
}


INLINE LONG COSDisk::IOQueue::IOHeapB::IpioreqHeapBIRightChild( LONG ipioreq ) const
{
    return ipioreqIOBHeapMax - 1 - ( 2 * ( ipioreqIOBHeapMax - 1 - ipioreq ) + 2 );
}


void COSDisk::IOQueue::IOHeapB::HeapBIUpdate( IOREQ* pioreq )
{

    LONG ipioreq = pioreq->ipioreqHeap;
    Assert( rgpioreqIOBHeap[ipioreq] == pioreq );


    while ( ipioreqIOBHeapMax > 0 &&
            ipioreq < ipioreqIOBHeapMax - 1 &&
            FHeapBISmaller( pioreq, rgpioreqIOBHeap[IpioreqHeapBIParent( ipioreq )] ) )
    {
        Assert( rgpioreqIOBHeap[IpioreqHeapBIParent( ipioreq )]->ipioreqHeap == IpioreqHeapBIParent( ipioreq ) );
        rgpioreqIOBHeap[ipioreq] = rgpioreqIOBHeap[IpioreqHeapBIParent( ipioreq )];
        rgpioreqIOBHeap[ipioreq]->ipioreqHeap = ipioreq;
        ipioreq = IpioreqHeapBIParent( ipioreq );
    }


    while ( ipioreq >= ipioreqIOBHeapMic )
    {

        if ( IpioreqHeapBILeftChild( ipioreq ) < ipioreqIOBHeapMic )
            break;


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


        if ( FHeapBISmaller( pioreq, rgpioreqIOBHeap[ipioreqChild] ) )
            break;


        Assert( rgpioreqIOBHeap[ipioreqChild]->ipioreqHeap == ipioreqChild );
        rgpioreqIOBHeap[ipioreq] = rgpioreqIOBHeap[ipioreqChild];
        rgpioreqIOBHeap[ipioreq]->ipioreqHeap = ipioreq;
        ipioreq = ipioreqChild;
    }
    Assert( ipioreq >= ipioreqIOBHeapMic );


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
    m_cioreqEnqueued( 0 ),
    m_cioEnqueued( 0 )
{
}

COSDisk::IOQueueToo::~IOQueueToo()
{
    Assert( m_cioreqEnqueued == 0 );
    Assert( m_cioEnqueued == 0 );
}


BOOL COSDisk::IOQueueToo::FInsertIo( _In_ IOREQ * pioreqToEnqueue, _In_ const LONG cioreqRun, _Out_ IOREQ ** ppioreqHeadAccepted )
{
    *ppioreqHeadAccepted = pioreqToEnqueue;

    Assert( pioreqToEnqueue->FEnqueuedInMetedQ() );

    Assert( !!( m_qit & qitWrite ) == !!pioreqToEnqueue->fWrite );
    Assert( !!( m_qit & qitRead ) == !pioreqToEnqueue->fWrite );

    const IFILEIBOFFSET ifileiboffsetToEnqueue = { pioreqToEnqueue->p_osf->iFile, pioreqToEnqueue->ibOffset };

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
        IORun iorun( pioreqIo );

        if ( ifileiboffsetToEnqueue.iFile == pioreqIo->p_osf->iFile &&
                ifileiboffsetToEnqueue.ibOffset == pioreqIo->ibOffset )
        {
            *ppioreqHeadAccepted = NULL;
            return fFalse;
        }

        if ( iorun.FAddToRun( pioreqToEnqueue ) )
        {
            IOREQ * pioreqIoNew = iorun.PioreqGetRun();
            if ( pioreqIoNew != pioreqIo )
            {
                Assert( pioreqIoNew == pioreqToEnqueue );

                AssertTrack( ifileiboffsetIoFound.iFile == pioreqIo->p_osf->iFile, "JustFoundIoMismatchFileId" );
                AssertTrack( ifileiboffsetIoFound.ibOffset == pioreqIo->ibOffset, "JustFoundIoMismatchOffset" );
                AssertTrack( ifileiboffsetToEnqueue.iFile == pioreqIoNew->p_osf->iFile, "CombinedIoRunShouldMatchInsertTargetFileId" );
                AssertTrack( ifileiboffsetToEnqueue.ibOffset == pioreqIoNew->ibOffset, "CombinedIoRunShouldMatchInsertTargetOffset" );

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
}


void COSDisk::IOQueueToo::ExtractIo( _Inout_ COSDisk::QueueOp * const pqop, _Out_ IOREQ ** ppioreqTraceHead, _Inout_ BOOL * pfCycles )
{
    Assert( m_pcritIoQueue->FOwner() );
    Assert( pfCycles );

    const BOOL fAutoCycle = *pfCycles;
    *pfCycles = fFalse;


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
        AssertTrack( m_irbtDraining.FEmpty(), "Searched00AndNotEmptyRbt" );

        if ( fAutoCycle )
        {

            Cycle();
            AssertTrack( m_irbtBuilding.FEmpty(), "TransferShouldLeaveBuildingQEmpty" );
            *pfCycles = fTrue;


            m_hrtBuildingStart = 0;

            ifileiboffsetIoFound = IFILEIBOFFSET::IfileiboffsetNotFound;
            Assert( ifileiboffsetIoFound == IFILEIBOFFSET::IfileiboffsetNotFound );
            Assert( ifileiboffsetIoFound.iFile == qwMax );
            Assert( ifileiboffsetIoFound.ibOffset == qwMax );
            err = m_irbtDraining.ErrFindNearest( ifileiboffsetLowest, &ifileiboffsetIoFound, &pioreqIo );
        }


        if ( err != IRBTQ::ERR::errSuccess )
        {
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


    Assert( pioreqIo );
    Assert( ifileiboffsetIoFound.iFile != qwMax && ifileiboffsetIoFound.ibOffset != qwMax );
    AssertTrack( !( ifileiboffsetIoFound == IFILEIBOFFSET::IfileiboffsetNotFound ), "FoundIoKeyShouldNotBeMaxNotFound" );
    Assert( !*pfCycles || ifileiboffsetDrainingBefore == IFILEIBOFFSET::IfileiboffsetNotFound );
    Assert( !*pfCycles || !( ifileiboffsetBuildingBefore == IFILEIBOFFSET::IfileiboffsetNotFound ) );

    if ( ifileiboffsetIoFound.iFile != pioreqIo->p_osf->iFile )
    {
        FireWall( "FoundIoKeyAndObjectMismatchFileId" );
    }
    if ( ifileiboffsetIoFound.ibOffset != pioreqIo->ibOffset )
    {
        FireWall( "FoundIoKeyAndObjectMismatchOffset" );
    }

    Assert( pioreqIo->FEnqueuedInMetedQ() );


    const IRBTQ::ERR errDel = m_irbtDraining.ErrDelete( ifileiboffsetIoFound );
    AssertTrack( IRBTQ::ERR::errSuccess == errDel, "JustFoundIoCouldntDelete" );



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

LONG g_dtickStarvedMetedOpThreshold = 3000;

BOOL COSDisk::IOQueueToo::FStarvedMetedOp() const
{
    Assert( FIOThread() );
    
    if ( m_hrtBuildingStart != 0 )
    {
        const QWORD cmsec = CmsecHRTFromDhrt( HrtHRTCount() - m_hrtBuildingStart );
        if ( cmsec > g_dtickStarvedMetedOpThreshold )
        {
            return fTrue;
        }
    }

    return fFalse;
}

    



COSDisk::IOQueue::IOQueue( CCriticalSection * pcritController ) :
    CZeroInit( sizeof(*this) ),

    m_pcritIoQueue( pcritController ),
    m_qMetedAsyncReadIo( pcritController, COSDisk::IOQueueToo::qitRead ),
    m_qWriteIo( pcritController, COSDisk::IOQueueToo::qitWrite ),
    m_semIOQueue( CLockBasicInfo( CSyncBasicInfo( "IO Heap Semaphore Protection" ), rankOSDiskIOQueueCrit, 0 ) )
{
    Assert( m_pcritIoQueue->FNotOwner() );
}

COSDisk::IOQueue::~IOQueue()
{
    Assert( ( m_pIOHeapA == NULL ) == ( m_pIOHeapB == NULL ) );

    Assert( m_pIOHeapA == NULL || m_cioreqMax == m_semIOQueue.CAvail() );

    Assert( 0 == m_cioVIPList && m_VIPListHead.FEmpty() );
    Assert( 0 == m_cioQosUrgentBackgroundCurrent );
    Assert( 0 == m_cioQosBackgroundCurrent );

    IOQueueTerm();
}



void COSDisk::IOQueue::IOQueueTerm()
{
    Assert( 0 == m_cioVIPList );
    Assert( m_VIPListHead.FEmpty() );


    _IOHeapTerm();
}


ERR COSDisk::IOQueue::ErrIOQueueInit( __in LONG cIOEnqueuedMax, __in LONG cIOBackgroundMax, __in LONG cIOUrgentBackgroundMax )
{
    ERR err = JET_errSuccess;

    Assert( cIOBackgroundMax < cIOEnqueuedMax );
    Assert( cIOUrgentBackgroundMax < cIOEnqueuedMax );


    Call( _ErrIOHeapInit( cIOEnqueuedMax ) );


    m_cioQosBackgroundMax = cIOBackgroundMax;
    m_cioreqQOSBackgroundLow = cIOBackgroundMax / 20;
    m_cioreqQOSBackgroundLow = max( m_cioreqQOSBackgroundLow, 1 );

    m_cioQosUrgentBackgroundMax = cIOUrgentBackgroundMax;

    return JET_errSuccess;

HandleError:
    IOQueueTerm();
    return err;
}


void COSDisk::IOQueue::_IOHeapTerm()
{

    delete m_pIOHeapA;
    m_pIOHeapA = NULL;
    delete m_pIOHeapB;
    m_pIOHeapB = NULL;
}


ERR COSDisk::IOQueue::_ErrIOHeapInit( __in LONG cIOEnqueuedMax )
{
    ERR err = JET_errSuccess;
    COSDisk::IOQueue::IOHeapA * pIOHeapA = NULL;
    COSDisk::IOQueue::IOHeapB * pIOHeapB = NULL;

    Call( ErrFaultInjection( 17360 ) );
    Alloc( pIOHeapA = new IOHeapA( m_pcritIoQueue ) );
    Call( ErrFaultInjection( 17364 ) );
    Alloc( pIOHeapB = new IOHeapB( m_pcritIoQueue ) );


    m_cioreqMax = cIOEnqueuedMax;

    Call( pIOHeapA->ErrHeapAInit( m_cioreqMax ) );
    Call( pIOHeapB->ErrHeapBInit( pIOHeapA->rgpioreqIOAHeap, pIOHeapA->ipioreqIOAHeapMax ) );


    m_pIOHeapA = pIOHeapA;
    m_pIOHeapB = pIOHeapB;
    pIOHeapA = NULL;
    pIOHeapB = NULL;

    m_semIOQueue.Release( m_cioreqMax );


    fUseHeapA       = fTrue;

    iFileCurrent        = 0;
    ibOffsetCurrent = 0;

    return JET_errSuccess;

HandleError:

    delete pIOHeapA;
    delete pIOHeapB;

    return err;
}


INLINE BOOL COSDisk::IOQueue::FIOHeapEmpty()
{
    Assert( m_pcritIoQueue->FOwner() );
    return m_pIOHeapA->FHeapAEmpty() && m_pIOHeapB->FHeapBEmpty();
}


INLINE LONG COSDisk::IOQueue::CioreqIOHeap() const
{
    Assert( m_pIOHeapA );
    Assert( m_pIOHeapB );
    return m_pIOHeapA->CioreqHeapA() + m_pIOHeapB->CioreqHeapB();
}


INLINE LONG COSDisk::IOQueue::CioVIPList() const
{
    #ifdef DEBUG
    if ( !m_pcritIoQueue->FNotOwner() )
    {
        Assert( FValidVIPList() );
    }
    #endif
    return m_cioVIPList;
}


INLINE LONG COSDisk::IOQueue::CioMetedReadQueue() const
{
    return m_qMetedAsyncReadIo.CioEnqueued();
}


INLINE IOREQ* COSDisk::IOQueue::PioreqIOHeapTop()
{

    Assert( m_pcritIoQueue->FOwner() );

    Assert( m_pIOHeapA );
    Assert( m_pIOHeapB );


    if ( m_pIOHeapA->FHeapAEmpty() )
    {

        if ( m_pIOHeapB->FHeapBEmpty() )
        {

            return NULL;
        }


        else
        {

            return m_pIOHeapB->PioreqHeapBTop();
        }
    }


    else
    {

        if ( m_pIOHeapB->FHeapBEmpty() )
        {

            return m_pIOHeapA->PioreqHeapATop();
        }


        else
        {

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


INLINE void COSDisk::IOQueue::IOHeapAdd( IOREQ* pioreq, _Out_ OSDiskIoQueueManagement * const pdioqmTypeTracking )
{


    Assert( pioreq );
    Assert( pioreq->m_fHasHeapReservation );
    Assert( m_pcritIoQueue->FOwner() );


    const BOOL fForward =   CmpOSDiskIFileIbIFileIb(  pioreq->p_osf->iFile,
                                                        pioreq->ibOffset,
                                                        iFileCurrent,
                                                        ibOffsetCurrent ) > 0;


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


INLINE void COSDisk::IOQueue::IOHeapRemove( IOREQ* pioreq, _Out_ OSDiskIoQueueManagement * const pdioqmTypeTracking )
{

    Assert( m_pcritIoQueue->FOwner() );

    Assert( m_pIOHeapA );
    Assert( m_pIOHeapB );

    Assert( pioreq->FEnqueuedInHeap() );


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


    fUseHeapA = fRemoveFromHeapA;


    iFileCurrent    = pioreq->p_osf->iFile;
    ibOffsetCurrent = pioreq->ibOffset;
}



CTaskManager*       g_postaskmgrFile;

const ULONG g_cioMaxMax = CMeteredSection::cMaxActive - 1 ;

void COSLayerPreInit::SetIOMaxOutstanding( ULONG cIOs )
{
    if ( cIOs > g_cioMaxMax )
    {
        cIOs = g_cioMaxMax;
    }
    g_cioOutstandingMax = cIOs;
}

void COSLayerPreInit::SetIOMaxOutstandingBackground( ULONG cIOs )
{
    if ( cIOs > g_cioMaxMax )
    {
        AssertSz( FNegTest( fInvalidUsage ), "The cIOs (%d) value is too high (> %d)", cIOs, g_cioMaxMax );
        cIOs = g_cioMaxMax;
    }
    g_cioBackgroundMax = cIOs;
}
    

INLINE ERR ErrOSDiskIIOThreadRegisterFile( const P_OSFILE p_osf )
{
    ERR err;


    Call( ErrFaultInjection( 17340 ) );
    if ( !g_postaskmgrFile->FTMRegisterFile(  p_osf->hFile,
                                            CTaskManager::PfnCompletion( OSDiskIIOThreadIComplete ) ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    return JET_errSuccess;

HandleError:
    return err;
}


INLINE BOOL FOSDiskIFileSGIOCapable( const _OSFILE * const p_osf )
{
    return  p_osf->fRegistered &&
            p_osf->iomethodMost >= IOREQ::iomethodScatterGather;
}


INLINE BOOL FOSDiskIDataSGIOCapable( __in const BYTE * pbData, __in const DWORD cbData )
{
    return  cbData % OSMemoryPageCommitGranularity() == 0 &&
            DWORD_PTR( pbData ) % OSMemoryPageCommitGranularity() == 0;
}


INLINE void OSDiskIGetRunBound(
    const IOREQ *   pioreqRun,
    __out QWORD *   pibOffsetStart,
    __out QWORD *   pibOffsetEnd,
    bool            fAlwaysIncreasing = true );

INLINE void OSDiskIGetRunBound(
    const IOREQ *   pioreqRun,
    __out QWORD *   pibOffsetStart,
    __out QWORD *   pibOffsetEnd,
    bool            fAlwaysIncreasing )
{
    Assert( pibOffsetStart );
    Assert( pibOffsetEnd );

    Assert( pioreqRun );
    Assert( pioreqRun->cbData );
    Assert( ( pioreqRun->IbBlockMax() > pioreqRun->ibOffset ) &&
            ( pioreqRun->IbBlockMax() >= pioreqRun->cbData ) );

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

            *pibOffsetStart = pioreqT->ibOffset;

            Assert( !fAlwaysIncreasing );
        }
        if ( pioreqT->IbBlockMax() > (*pibOffsetEnd) )
        {

            *pibOffsetEnd = pioreqT->IbBlockMax();
        }

    }
}

INLINE bool FOSDiskIOverlappingRuns( const QWORD ibOffsetStartRun1, const QWORD ibOffsetEndRun1, const QWORD ibOffsetStartRun2, const QWORD ibOffsetEndRun2, bool * pfContiguous )
{
    if ( ibOffsetStartRun1 < ibOffsetStartRun2 )
    {

        if ( pfContiguous )
        {
            *pfContiguous = ( ibOffsetEndRun1 == ibOffsetStartRun2 );
        }
        return ( ibOffsetEndRun1 > ibOffsetStartRun2 );
    }
    else
    {

        if ( pfContiguous )
        {
            *pfContiguous = ( ibOffsetEndRun2 == ibOffsetStartRun1 );
        }
        return ( ibOffsetEndRun2 > ibOffsetStartRun1 );
    }
}

INLINE bool FOSDiskIOverlappingRuns( const QWORD ibOffsetStartRun1, const QWORD ibOffsetEndRun1, const IOREQ * pioreqRun2 )
{
    Expected( pioreqRun2->pioreqIorunNext == NULL );

    QWORD ibOffsetStartRun2, ibOffsetEndRun2;

    OSDiskIGetRunBound( pioreqRun2, &ibOffsetStartRun2, &ibOffsetEndRun2 );

    return FOSDiskIOverlappingRuns( ibOffsetStartRun1, ibOffsetEndRun1, ibOffsetStartRun2, ibOffsetEndRun2, NULL );
}

#ifdef DEBUG


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


INLINE bool FOSDiskICanAppendRun(
    __in const _OSFILE *    p_osf,
    __in const BOOL         fWrite,
    __in const BOOL         fOverrideIOMax,
    __in const QWORD        ibOffsetRunBefore,
    __in const DWORD        cbDataRunBefore,
    __in const QWORD        ibOffsetRunAfter,
    __in const DWORD        cbDataRunAfter
    )
{
    Assert( cbDataRunBefore && cbDataRunAfter );
    Expected( !fOverrideIOMax || fWrite );
    Assert( ibOffsetRunBefore < ibOffsetRunAfter );
    Assert( ( ibOffsetRunBefore + cbDataRunBefore ) <= ibOffsetRunAfter );

    const DWORD cbGap = fWrite ? 0 : p_osf->Pfsconfig()->CbMaxReadGapSize();
    const DWORD cbMax = fWrite ? p_osf->Pfsconfig()->CbMaxWriteSize() : p_osf->Pfsconfig()->CbMaxReadSize();

    return ( ( ( ibOffsetRunBefore + cbDataRunBefore ) <= ibOffsetRunAfter ) &&
             ( ( fOverrideIOMax || ( ibOffsetRunAfter - ibOffsetRunBefore + cbDataRunAfter ) <= cbMax ) ) &&
             ( ( ibOffsetRunAfter - ibOffsetRunBefore - cbDataRunBefore ) <= cbGap ) );
}

bool FOSDiskICompatibleRuns(
    __inout const COSDisk::IORun * const    piorun,
    __in const IOREQ *                      pioreq
    )
{
    Assert( piorun );
    Assert( piorun->CbRun() );
    Assert( pioreq->p_osf );

    if ( piorun->P_OSF() == pioreq->p_osf &&
        !!piorun->FWrite() == !!pioreq->fWrite )
    {
        return fTrue;
    }

    return fFalse;
}

bool FOSDiskIMergeRuns( COSDisk::IORun * const piorunBase, COSDisk::IORun * const piorunToAdd, const BOOL fTest );


INLINE bool FOSDiskICanAddToRun(
    __inout COSDisk::IORun * const  piorun,
    __in const _OSFILE *            p_osf,
    __in const BOOL                 fWrite,
    __in const QWORD                ibOffsetCombine,
    __in const DWORD                cbDataCombine,
    __in const BOOL                 fOverrideIOMax
    )
{
    Assert( piorun );
    Assert( piorun->CbRun() );
    Assert( p_osf );
    Assert( cbDataCombine );
    Expected( !fOverrideIOMax || fWrite );

    Expected( piorun->P_OSF() == p_osf );
    Expected( !!piorun->FWrite() == !!fWrite );

    if ( piorun->P_OSF() != p_osf ||
        !!piorun->FWrite() != !!fWrite )
    {
        return false;
    }

    
    if( piorun->IbOffset() == ibOffsetCombine )
    {
        AssertSz( !fWrite && !piorun->FWrite() || FNegTest( fInvalidUsage ), "Concurrent write IOs for the same file+offset are not allowed!  Check the m_tidAlloc on this piorun and the second pioreq (which is probably up a stack frame)." );
    }
    else if( piorun->IbOffset() < ibOffsetCombine && ( ( ibOffsetCombine - piorun->IbOffset() ) >= piorun->CbRun() ) )
    {

        return FOSDiskICanAppendRun( p_osf, fWrite, fOverrideIOMax, piorun->IbOffset(), piorun->CbRun(), ibOffsetCombine, cbDataCombine );
    }
    else if( ibOffsetCombine < piorun->IbOffset() && ( ( piorun->IbOffset() - ibOffsetCombine ) >= cbDataCombine ) )
    {

        Assert( piorun->IbOffset() > ibOffsetCombine );
        
        return FOSDiskICanAppendRun( p_osf, fWrite, fOverrideIOMax, ibOffsetCombine, cbDataCombine, piorun->IbOffset(), piorun->CbRun() );
    }
    else
    {

        Enforce( p_osf == piorun->P_OSF() );

        IOREQ ioreqCombine;
        ioreqCombine.p_osf = (P_OSFILE)p_osf;
        ioreqCombine.fWrite = fWrite;
        ioreqCombine.pioreqIorunNext = NULL;
        ioreqCombine.ibOffset = ibOffsetCombine;
        ioreqCombine.cbData = cbDataCombine;
        ioreqCombine.m_fCanCombineIO = fTrue;
        ioreqCombine.grbitQOS = fOverrideIOMax ? qosIOOptimizeOverrideMaxIOLimits : 0;
        COSDisk::IORun iorunCombine( &ioreqCombine );
        const bool fRet = FOSDiskIMergeRuns( piorun, &iorunCombine, fTrue  );
        Assert( !piorun->FRunMember( &ioreqCombine ) );
        return fRet;
    }

    return fFalse;
}


INLINE bool FOSDiskICanAddToRun(
    __inout COSDisk::IORun * const  piorun,
    __in const IOREQ *              pioreq
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

            Assert( m_storage.Head() == m_storage.Tail() );
            Assert( m_storage.Head()->pioreqIorunNext == NULL );
            Assert( m_storage.Head()->cbData == m_cbRun );

        }
        else
        {

            Assert( m_storage.Head() != m_storage.Tail() );

            Assert( m_storage.Head()->ibOffset < m_storage.Tail()->ibOffset );
            Assert( m_storage.Head()->IbBlockMax() <= m_storage.Tail()->ibOffset );
            Assert( m_storage.Head()->ibOffset + m_cbRun == m_storage.Tail()->IbBlockMax() );
            Assert( m_storage.Head()->cbData < m_cbRun );
            Assert( m_storage.Tail()->cbData < m_cbRun );
            Assert( m_storage.Head()->pioreqIorunNext );

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


void COSDisk::IORun::InitRun_( IOREQ * pioreqRun )
{
    QWORD ibOffsetStart;
    QWORD ibOffsetEnd;

    Assert( m_storage.FEmpty() );
    Assert( m_cbRun == 0 );

    m_storage.~CSimpleQueue<IOREQ>();
    new ( &m_storage ) CSimpleQueue<IOREQ>( pioreqRun, OffsetOf(IOREQ,pioreqIorunNext) );

    OSDiskIGetRunBound( pioreqRun, &ibOffsetStart, &ibOffsetEnd );
    Assert( ( ibOffsetEnd - ibOffsetStart ) == ( (DWORD)( ibOffsetEnd - ibOffsetStart ) ) );

    OnDebug( m_cbRun = (DWORD)( ibOffsetEnd - ibOffsetStart ) );
    ASSERT_VALID( this );
}


bool FOSDiskIMergeRuns( COSDisk::IORun * const piorunBase, COSDisk::IORun * const piorunToAdd, const BOOL fTest )
{
    bool fMergable = false;

    Assert( piorunBase );
    Assert( piorunToAdd );

    Assert( !piorunBase->FEmpty() );
    Assert( !piorunToAdd->FEmpty() );

    if ( piorunBase->IbOffset() == piorunToAdd->IbOffset() )
    {
        return false;
    }

    const BOOL fWrite = !!piorunBase->FWrite();
    const _OSFILE * p_osf = piorunBase->P_OSF();

    IOREQ * const  pioreqBaseHead = piorunBase->PioreqGetRun();
    IOREQ * const  pioreqToAddHead = piorunToAdd->PioreqGetRun();
    Assert( piorunBase->FEmpty() && piorunToAdd->FEmpty() );
    Assert( pioreqBaseHead );
    Assert( pioreqToAddHead );

    const BOOL fOverrideIOMax = !!( pioreqToAddHead->grbitQOS & qosIOOptimizeOverrideMaxIOLimits );


    IOREQ * pioreqNewHeadFirst = NULL;
    IOREQ * pioreqNewTailLast = NULL;

    IOREQ * pioreqBaseMid = pioreqBaseHead;
    IOREQ * pioreqToAddMid = pioreqToAddHead;
    OnDebug( LONG cioreqBase = 0 );
    OnDebug( LONG cioreqToAdd = 0 );


    Enforce( pioreqBaseMid->ibOffset != pioreqToAddMid->ibOffset );
    if ( CmpIOREQOffset( pioreqBaseMid, pioreqToAddMid ) < 0 )
    {
        pioreqNewHeadFirst = pioreqBaseMid;
        pioreqBaseMid = pioreqBaseMid->pioreqIorunNext;
        OnDebug( cioreqBase++ );
    }
    else
    {
        pioreqNewHeadFirst = pioreqToAddMid;
        pioreqToAddMid = pioreqToAddMid->pioreqIorunNext;
        OnDebug( cioreqToAdd++ );

        if ( !pioreqToAddHead->m_fHasHeapReservation )
        {
            Assert( pioreqToAddHead->m_fCanCombineIO );
            Assert( pioreqBaseHead->m_fHasHeapReservation );
            if ( !fTest )
            {
                pioreqToAddHead->m_fHasHeapReservation = pioreqBaseHead->m_fHasHeapReservation;
                pioreqBaseHead->m_fHasHeapReservation = fFalse;
                pioreqBaseHead->m_fCanCombineIO = pioreqToAddHead->m_fCanCombineIO;
            }
        }
    }
    pioreqNewTailLast = pioreqNewHeadFirst;


    LONG cioreqTotal = 1;
    while( pioreqBaseMid != NULL || pioreqToAddMid != NULL )
    {

        IOREQ * pioreqLowest = NULL;


        if ( pioreqBaseMid && pioreqToAddMid &&
                CmpIOREQOffset( pioreqBaseMid, pioreqToAddMid ) == 0 )
        {
            Assert( !fMergable );
            Assert( !fWrite || FNegTest( fInvalidUsage )  );
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
        else if ( pioreqBaseMid == NULL || ( pioreqToAddMid != NULL  && CmpIOREQOffset( pioreqBaseMid, pioreqToAddMid ) > 0 ) )
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



        if ( pioreqNewTailLast->IbBlockMax() > pioreqLowest->ibOffset )
        {

            Assert( !fWrite || FNegTest( fInvalidUsage )  );
            Assert( !fMergable );
            Assert( fTest );
            goto ReconstructIoRuns;
        }


        const QWORD cbNewCurr = pioreqNewTailLast->IbBlockMax() - pioreqNewHeadFirst->ibOffset;
        if ( cbNewCurr >= (QWORD)dwMax ||
            !FOSDiskICanAppendRun( p_osf, fWrite, fOverrideIOMax, pioreqNewHeadFirst->ibOffset, (DWORD)cbNewCurr, pioreqLowest->ibOffset, pioreqLowest->cbData ) )
        {
            Assert( !fMergable );
            Assert( fTest );
            goto ReconstructIoRuns;
        }


        if ( !fTest )
        {
            pioreqNewTailLast->pioreqIorunNext = pioreqLowest;
        }
        pioreqNewTailLast = pioreqLowest;
        if ( !fTest )
        {
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



    const bool bAddingRun = NULL != pioreq->pioreqIorunNext;

    if ( m_storage.FEmpty() )
    {

        Assert( 0 == m_cbRun );
        Assert( NULL == m_storage.Head() && NULL == m_storage.Tail() );

        if ( bAddingRun )
        {
            InitRun_( pioreq );
            Assert( m_cbRun > 0 );
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

        if ( !FOSDiskICompatibleRuns( this, pioreq ) )
        {
            ASSERT_VALID( this );
            return false;
        }
        
        Assert( PioreqHead()->p_osf == pioreq->p_osf );

        if ( !bAddingRun && !FOSDiskIOverlappingRuns( this->IbOffset(), this->IbRunMax(), pioreq ) )
        {

            if ( FOSDiskICanAddToRun( this, pioreq ) )
            {
                Assert( !FOSDiskIOverlappingRuns( m_storage.Head(), pioreq ) );

                if ( IbRunMax() <= pioreq->ibOffset )
                {
                    
                    OnDebug( m_cbRun += (DWORD)( ( pioreq->ibOffset + pioreq->cbData ) - ( IbOffset() + CbRun() ) ) );
                    m_storage.InsertAsNextMost( pioreq, OffsetOf(IOREQ,pioreqIorunNext ) );

                    ASSERT_VALID( this );
                }
                else if ( pioreq->IbBlockMax() <= IbOffset() )
                {

                    if ( !pioreq->m_fHasHeapReservation )
                    {
                        Assert( pioreq->m_fCanCombineIO );
                        Assert( m_storage.Head()->m_fHasHeapReservation );
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
                    AssertSz( fFalse, "There is IO overlap in the attempt to add an IO" );
                    ASSERT_VALID( this );
                    return false;
                }
            }
            else
            {
                ASSERT_VALID( this );
                return false;
            }
        }
        else
        {
            COSDisk::IORun iorunMerging( pioreq );

            if ( !FOSDiskIMergeRuns( this, &iorunMerging, fTrue ) )
            {
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


ULONG COSDisk::IORun::CbRun() const
{
    ASSERT_VALID( this );
    if ( FEmpty() )
    {
        return 0;
    }
    
    const QWORD cbRunT = m_storage.Tail()->IbBlockMax() - m_storage.Head()->ibOffset;
    Assert( m_cbRun == cbRunT );

    Enforce( cbRunT < ulMax );
    return (ULONG)cbRunT;
}

#ifdef DEBUG


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

#endif


void COSDisk::IORun::SetIOREQType( const IOREQ::IOREQTYPE ioreqtypeNext )
{
    for ( IOREQ * pioreqT = m_storage.Head(); pioreqT; pioreqT = pioreqT->pioreqIorunNext )
    {
        Assert( pioreqT->m_posdCurrentIO == NULL );
        pioreqT->SetIOREQType( ioreqtypeNext );
    }
}


IOREQ * COSDisk::IORun::PioreqGetRun( )
{
    OnDebug( m_cbRun = 0 );
    return m_storage.RemoveList();
}



#ifdef DEBUG


bool COSDisk::QueueOp::FValid( ) const
{
    switch( m_eQueueOp )
    {
        case eUnknown:
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
                        m_pioreq->FInIssuedState() :
                        m_iorun.FIssuedFromQueue();
#ifndef RTM
    if ( NULL == m_pioreq )
    {
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
            Expected( m_pioreq->pioreqVipList == NULL );
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
            Expected( m_pioreq->pioreqVipList == NULL );
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
    return m_iorun.FRunMember( pioreqCheck );
}

#endif

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

            if ( 0 == pioreq->cbData )
            {

                Assert( NULL == pioreq->pioreqIorunNext );
                if ( NULL == m_pioreq )
                {
                    m_pioreq = pioreq;
                    fRet = true;
                }
            }
            break;

        case eIOOperation:


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


void COSDisk::QueueOp::SetIOREQType( const IOREQ::IOREQTYPE ioreqtypeNext )
{
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
        pioreqT = m_iorun.PioreqGetRun();
    }
    else
    {
        AssertSz( fFalse, "Unknown EType()!" );
    }
    m_eQueueOp = eUnknown;
    Assert( FEmpty() && NULL == m_pioreq && m_iorun.FEmpty() );
    return pioreqT;
}
    

OSFILEQOS QosOSFileFromUrgentLevel( __in const ULONG iUrgentLevel )
{
    Assert( iUrgentLevel >= 1 );
    Assert( iUrgentLevel <= qosIODispatchUrgentBackgroundLevelMax );

    DWORD grbitQOS = ( iUrgentLevel << qosIODispatchUrgentBackgroundShft );

    Assert( qosIODispatchUrgentBackgroundMask & grbitQOS );
    Assert( 0 == ( ~qosIODispatchUrgentBackgroundMask & grbitQOS ) );

    return grbitQOS;
}


ULONG IOSDiskIUrgentLevelFromQOS( _In_ const OSFILEQOS grbitQOS )
{
    Assert( qosIODispatchUrgentBackgroundMask & grbitQOS );
    Assert( 0 == ( ( ~qosIODispatchUrgentBackgroundMask & grbitQOS ) & qosIODispatchMask ) );

    ULONG   iUrgentLevel = ( grbitQOS & qosIODispatchUrgentBackgroundMask ) >> qosIODispatchUrgentBackgroundShft;

    Assert( iUrgentLevel >= 1 );
    Assert( iUrgentLevel <= qosIODispatchUrgentBackgroundLevelMax );

    return iUrgentLevel;
}


LONG CioOSDiskIFromUrgentLevelSmoothish( __in const ULONG iUrgentLevel, __in const DWORD cioUrgentMaxMax )
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

LONG CioOSDiskIFromUrgentLevelBiLinear( __in const ULONG iUrgentLevel, __in const DWORD cioUrgentMaxMax )
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


LONG CioOSDiskIFromUrgentLevelLinear( __in const ULONG iUrgentLevel, __in const DWORD cioUrgentMaxMax )
{
    Assert( iUrgentLevel >= 1 );
    Assert( iUrgentLevel <= qosIODispatchUrgentBackgroundLevelMax );

    ULONG cioMax = (LONG) iUrgentLevel * cioUrgentMaxMax / qosIODispatchUrgentBackgroundLevelMax;

    cioMax = UlBound( cioMax, 1, cioUrgentMaxMax );

    Assert( cioMax >= 1 );
    Assert( cioMax <= cioUrgentMaxMax );

    return (LONG)cioMax;
}

LONG CioOSDiskIFromUrgentQOS( _In_ const OSFILEQOS grbitQOS, _In_ const DWORD cioUrgentMaxMax )
{
    const ULONG cioMax = CioOSDiskIFromUrgentLevelSmoothish( IOSDiskIUrgentLevelFromQOS( grbitQOS ), cioUrgentMaxMax );

    Assert( cioMax >= 1 );
    Assert( cioMax <= cioUrgentMaxMax );

    return (LONG)cioMax;
}

LONG CioOSDiskPerfCounterIOMaxFromUrgentQOS( _In_ const OSFILEQOS grbitQOS )
{
    return CioOSDiskIFromUrgentQOS( grbitQOS, CioDefaultUrgentOutstandingIOMax( g_cioOutstandingMax) );
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
            Expected( !pioreq->fWrite );
            cbRun += ULONG( pioreq->ibOffset - pioreqPrev->IbBlockMax() );
        }
        pioreqPrev = pioreq;
        pioreq = pioreq->pioreqIorunNext;
    }
    return cbRun;
}


ERR COSDisk::IOQueue::ErrReserveQueueSpace( __in OSFILEQOS grbitQOS, __inout IOREQ * pioreq )
{
    ERR err = JET_errSuccess;


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


                pioreq->m_fHasBackgroundReservation = fTrue;
            }
            else
            {
            }
            break;

        case qosIODispatchImmediate:
            break;

        case qosIODispatchIdle:
        default:

            Assert( pioreq->fWrite );
            if ( qosIODispatchUrgentBackgroundMask & grbitQOS )
            {

                LONG    cioreqUrgentMax = CioOSDiskIFromUrgentQOS( grbitQOS, m_cioQosUrgentBackgroundMax );
                Assert( cioreqUrgentMax <= m_cioQosUrgentBackgroundMax );

                Assert( !( qosIOPoolReserved & grbitQOS ) );
                if ( !FAtomicIncrementMax( (DWORD*)&m_cioQosUrgentBackgroundCurrent, &cioreqT, cioreqUrgentMax ) )
                {
                    Assert( m_cioQosUrgentBackgroundCurrent <= m_cioQosUrgentBackgroundMax );
                    Error( ErrERRCheck( errDiskTilt ) );
                }
                

                pioreq->m_fHasUrgentBackgroundReservation = fTrue;

            }
            else
            {
                AssertSz( fFalse, "Unknown IO Dispatch QOS!" );
            }

            break;
    }


    if ( !m_semIOQueue.FTryAcquire() )
    {

        OSDiskIOThreadStartIssue( NULL );


        if ( qosIOPoolReserved & grbitQOS )
        {
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
    Assert( dioqm != dioqmInvalid );

    const DWORD fWrite = pioreqHead->fWrite ? 1 : 0;

    Assert( ( dioqm & dioqmMergedIorun ) || CbSumRun( pioreqHead ) == cbRun );
    
    OSTrace(    JET_tracetagIOQueue,
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

    static_assert( sizeof(dioqm) == sizeof(BOOL), "The enum should match original size of argument." );
    static_assert( sizeof(dioqm) == sizeof(DWORD), "The enum should match new size of argument." );

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

    Assert( pioreqHead );
    Assert( pioreqAdded );

    const HRT hrtDone = HrtHRTCount();
    const LONG cusecDequeueLatency = CusecOSDiskSmallDuration( hrtDone - hrtDequeueBegin );
    const QWORD cusecTimeInQueue = CusecHRTFromDhrt( hrtDone - pioreqAdded->hrtIOStart );

    PERFOpt( pioreqAdded->p_osf->pfpapi->DecrementIOInHeap( pioreqAdded->fWrite ) );

    Expected( FIOThread() );
    Assert( ( dioqm & mskQueueType ) != 0 );

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
    
    static_assert( sizeof(dioqm) == sizeof(UINT), "The enum type should be 32-bit or could lose data in the trace." );

    Assert( ( pioreqAdded->grbitQOS & 0xFFFFFFFF00000000 ) == 0 );
    ETIorunDequeue( pioreqAdded->p_osf->iFile,
            pioreqAdded->ibOffset,
            cbAdded,
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

    if (
         pqop->FUseMetedQ() &&
         ( pqop->PioreqOp() == NULL && pqop->CbRun() != 0 ) &&
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
            pqop->FAddToRun( pioreqHead );
            dioqm = dioqm | dioqmFreeVipUpgrade;
            goto FreeUpgradeToFirstClass;
        }

        dioqm = dioqmTypeMetedQ | ( fNewRun ? dioqmInvalid : dioqmMergedIorun );
    }
    else if ( pqop->FHasHeapReservation() )
    {
        pqop->SetIOREQType( IOREQ::ioreqEnqueuedInIoHeap );

        pioreqHead = pqop->PioreqGetRun();

        Expected( pioreqHead->fWrite || !fUseMetedQueue ||
                    ( pioreqHead->m_tc.etc.iorReason.Iorp() != 34  ) );

        IOHeapAdd( pioreqHead, &dioqm );
    }
    else
    {
    FreeUpgradeToFirstClass:

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

    pioreqHead = NULL;
    m_pcritIoQueue->Leave();
}


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



        IOREQ * const pioreqT = m_VIPListHead.RemovePrevMost( OffsetOf(IOREQ,pioreqVipList) );
        m_cioVIPList--;


        const bool fAccepted = pqop->FAddToRun( pioreqT );
        Assert( fAccepted );


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
            m_qMetedAsyncReadIo.Cycle();
            m_qWriteIo.Cycle();
        }

        const IFILEIBOFFSET ifileiboffsetNextReadIo = m_qMetedAsyncReadIo.IfileiboffsetFirstIo( IOQueueToo::qfifDraining );
        const IFILEIBOFFSET ifileiboffsetNextWriteIo = m_qWriteIo.IfileiboffsetFirstIo( IOQueueToo::qfifDraining );

        const BOOL fReadIoDrained = ( ifileiboffsetNextReadIo == IFILEIBOFFSET::IfileiboffsetNotFound );
        const BOOL fWriteIoDrained = ( ifileiboffsetNextWriteIo == IFILEIBOFFSET::IfileiboffsetNotFound );

        const BOOL fReadsLower = ( !fReadIoDrained && fWriteIoDrained ) ||
                                 ( !fReadIoDrained && !fWriteIoDrained &&
                                    ( ifileiboffsetNextReadIo < ifileiboffsetNextWriteIo ||
                                      ifileiboffsetNextReadIo == ifileiboffsetNextWriteIo ) );
        const BOOL fMetedQReady = m_qMetedAsyncReadIo.FStarvedMetedOp() ||
                                   posd->CioReadyMetedEnqueued() > 0;

        IOREQ * pioreqT = NULL;

        if ( !m_qMetedAsyncReadIo.FStarvedMetedOp() &&
                posd->CioReadyMetedEnqueued() <= 0 &&
                m_qWriteIo.CioEnqueued() <= 0 )
        {
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


        if ( fCycleQ )
        {
            m_qWriteIo.Cycle();
        }

        if ( pqop->FEmpty() )
        {
            

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


        IOREQ *         pioreqHead = NULL;
        IOREQ *         pioreqT;
        USHORT          cIorunCombined = 0;
        while ( pioreqT = PioreqIOHeapTop() )
        {

            const bool fNewRun = pqop->FEmpty();
            if ( fNewRun )
            {
                pioreqHead = pioreqT;
            }
            Assert( pioreqHead );
            

            Expected( pqop->FEmpty() || !pqop->FUseMetedQ() || pqop->FWrite() );


            const bool fAccepted = pqop->FAddToRun( pioreqT );

            Assert( pqop->FValid() );
            Assert( !pqop->FEmpty() );

            if( !fAccepted )
            {
                Assert( !pqop->FEmpty() );
                break;
            }


            IOHeapRemove( pioreqT, &dioqm );
            Assert( pioreqT != PioreqIOHeapTop() );

            if ( !fNewRun )
            {
                OnNonRTM( pioreqT->m_fDequeueCombineIO = fTrue );
            }


            cIorunCombined++;
            TrackIorunDequeue( pioreqHead, pqop->CbRun(), hrtExtractBegin, dioqm, cIorunCombined, pioreqT );
        }

        Assert( cIorunCombined > 0 );
    }


    Assert( pqop->FValid() );
    Assert( !pqop->FEmpty() );
    Assert( ( pqop->PioreqOp() && NULL == pqop->PiorunIO() ) ||
            ( NULL == pqop->PioreqOp() && pqop->PiorunIO() ) );


    pqop->SetIOREQType( IOREQ::ioreqRemovedFromQueue );


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

        LONG iUrgentLevel = IOSDiskIUrgentLevelFromQOS( pioreq->grbitQOS );
        if ( iUrgentLevel > 1 )
        {
            iUrgentLevel--;
        }
        if ( cioreqQueueOutstanding <= CioOSDiskIFromUrgentLevelSmoothish( iUrgentLevel, m_cioQosUrgentBackgroundMax ) )
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
    bool                                fRepeat,
    __in const DWORD                    cfse,
    __out PFILE_SEGMENT_ELEMENT const   rgfse )
{
    DWORD cbDelta = fRepeat ? 0 : OSMemoryPageCommitGranularity();
    DWORD cospage = 0;

    Enforce( 0 == cbData % OSMemoryPageCommitGranularity() );

    for ( DWORD cb = 0; cb < cbData && cospage < cfse; cb += OSMemoryPageCommitGranularity() )
    {
        rgfse[ cospage ].Buffer = PVOID64( ULONG_PTR( pbData ) );

        Assert( rgfse[ cospage ].Buffer != NULL );
        Assert( 0 == (ULONG64)rgfse[ cospage ].Buffer % OSMemoryPageCommitGranularity() );

        pbData += cbDelta;
        cospage++;
    }

    return cospage;
}


void OSDiskIIOPrepareScatterGatherIO(
    __in IOREQ * const                  pioreqHead,
    __in const DWORD                    cbRun,
    __in const DWORD                    cfse,
    __out PFILE_SEGMENT_ELEMENT const   rgfse,
    __out BOOL *                        pfIOOSLowPriority
    )
{
    Assert( pioreqHead );
    Assert( pioreqHead->pioreqIorunNext );
    Assert( cbRun && ( cbRun <= ( pioreqHead->fWrite ? pioreqHead->p_osf->Pfsconfig()->CbMaxWriteSize() : pioreqHead->p_osf->Pfsconfig()->CbMaxReadSize() ) ||
            ( pioreqHead->grbitQOS & qosIOOptimizeOverrideMaxIOLimits ) ) );

    DWORD cospage = 0;
    QWORD ibOffset = pioreqHead->ibOffset;
    for ( IOREQ* pioreq = pioreqHead; pioreq; pioreq = pioreq->pioreqIorunNext )
    {
        Assert( pioreq->cbData );

        if ( 0 == ( qosIOOSLowPriority & pioreq->grbitQOS ) )
        {
            *pfIOOSLowPriority = fFalse;
        }

        if ( pioreq != pioreqHead )
        {
            pioreq->fCoalesced = fTrue;
        }

        Assert( ibOffset <= pioreq->ibOffset );

        if ( ibOffset < pioreq->ibOffset )
        {
            DWORD cbGap = DWORD( pioreq->ibOffset - ibOffset );
            Assert( !pioreq->fWrite );
            Assert( cbGap <= pioreqHead->p_osf->Pfsconfig()->CbMaxReadGapSize() );

            cospage += OSDiskIFillFSEAddress( g_rgbGapping, cbGap, true, cfse - cospage - 1, &rgfse[ cospage ] );
            ibOffset = pioreq->ibOffset;
        }

        cospage += OSDiskIFillFSEAddress( pioreq->pbData, pioreq->cbData, false, cfse - cospage - 1, &rgfse[ cospage ] );
        ibOffset += pioreq->cbData;
    }

    Assert( cospage < cfse );
    rgfse[ cospage ].Buffer = NULL;
}


DWORD ErrorRFSIssueFailedIO()
{
    return ERROR_IO_DEVICE;
}



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
        return FALSE;
    }
}



DWORD ErrorIOMgrIssueIO(
    __in COSDisk::IORun*                    piorun,
    __in const IOREQ::IOMETHOD              iomethod,
    __inout const PFILE_SEGMENT_ELEMENT     rgfse,
    __in const DWORD                        cfse,
    __out BOOL *                            pfIOCompleted,
    __out DWORD *                           pcbTransfer,
    __out IOREQ **                          ppioreqHead = NULL
    )
{
    DWORD error = ERROR_SUCCESS;
    DWORD cbRun = 0;
    BOOL fIOOSLowPriority = fFalse;

    Assert( piorun != NULL );


    *pfIOCompleted = fFalse;
    *pcbTransfer = 0;


    piorun->PrepareForIssue( iomethod, &cbRun, &fIOOSLowPriority, rgfse, cfse );
    IOREQ * const pioreqHead = piorun->PioreqGetRun();
    piorun = NULL;

    if ( ppioreqHead != NULL )
    {
        *ppioreqHead = pioreqHead;
    }


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

    if ( !FExclusiveIoreq( pioreqHead ) && ErrFaultInjection( 64738 ) < JET_errSuccess )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    if ( ErrFaultInjection( 42980 ) < JET_errSuccess )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    if ( fIOOSLowPriority )
    {
        UtilThreadBeginLowIOPriority();
    }

    Assert( !pioreqHead->FSetSize() &&
            !pioreqHead->FExtendingWriteIssued() );

    Assert( pioreqHead->pioreqVipList == NULL );


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
            }

 
            *pfIOCompleted = fIOSucceeded;

            pioreqHead->p_osf->semFilePointer.Release();
            break;
        }

        case IOREQ::iomethodSemiSync:
        {
            Assert( NULL == pioreqHead->pioreqIorunNext );
            Assert( cbRun == pioreqHead->cbData );


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

                *pcbTransfer = 0;

                fIOSucceeded = GetOverlappedResult_(    pioreqHead->p_osf->hFile,
                                                        LPOVERLAPPED( pioreqHead ),
                                                        pcbTransfer,
                                                        TRUE );
                error   = fIOSucceeded ? ERROR_SUCCESS : GetLastError();
            }
            else
            {
                Assert( errIO <= JET_errSuccess );
            }

            if ( fIOSucceeded )
            {
                Assert( error == ERROR_SUCCESS );
                *pfIOCompleted = fTrue;
            }
            else
            {
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


    if ( fIOOSLowPriority )
    {
        UtilThreadEndLowIOPriority();
    }

    return error;
}



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
    Assert( m_rgiorunIoRuns[irun].FEmpty() );
    
    const BOOL fAdded = m_rgiorunIoRuns[irun].FAddToRun( pioreqToAdd );
    m_rghrtIoRunStarted[irun] = pioreqToAdd->hrtIOStart;

    Enforce( fAdded );

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

    const INT irun = IrunIoRunPoolIFindFileRun_( NULL );
    return irun == irunNil;
}

BOOL COSDisk::CIoRunPool::FContainsFileIoRun( _In_ const _OSFILE * const p_osf ) const
{
    Assert( p_osf );

    const INT irun = IrunIoRunPoolIFindFileRun_( p_osf );
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
        return fFalse;
    }
    Assert( m_rgiorunIoRuns[irunFile].P_OSF() == p_osf );

    Assert( !m_rgiorunIoRuns[irunFile].FEmpty() );

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
        OnDebug( const BOOL fShouldBeAdded = FCanCombineWithExistingRun( pioreqToAdd ) );
        Assert( fShouldBeAdded || pioreqToAdd->m_fHasHeapReservation );
        Assert( !pioreqToAdd->m_fCanCombineIO || fShouldBeAdded );

        fAdded = m_rgiorunIoRuns[irun].FAddToRun( pioreqToAdd );

        Assert( !!fAdded == !!fShouldBeAdded || fShouldBeAdded  );
        Assert( fAdded || pioreqToAdd->m_fHasHeapReservation );
        Assert( !pioreqToAdd->m_fCanCombineIO || fAdded );

        if ( !fAdded )
        {
            
            pioreqEvictedRun = PioreqIoRunPoolIRemoveRunSlot_( irun );

            
            Assert( m_rgiorunIoRuns[irun].FEmpty() );
            Assert( irunNil == IrunIoRunPoolIFindFileRun_( pioreqToAdd->p_osf ) );
        }
    }
    else
    {

        C_ASSERT( _countof(m_rghrtIoRunStarted) == _countof(m_rgiorunIoRuns) );

        Assert( pioreqToAdd->m_fHasHeapReservation );
        
        INT irunOldest = 0;
        for( irun = 0; irun < _countof( m_rgiorunIoRuns ); irun++ )
        {
            if ( m_rgiorunIoRuns[irun].FEmpty() )
            {
                break;
            }

            Expected( m_rgiorunIoRuns[irun].P_OSF() );

            if ( ( (__int64)m_rghrtIoRunStarted[irun] - (__int64)m_rghrtIoRunStarted[irunOldest] ) < 0 )
            {
                irunOldest = irun;
            }
        }
        if ( irun == _countof( m_rgiorunIoRuns ) )
        {

            irun = irunOldest;
            Assert( !m_rgiorunIoRuns[irun].FEmpty() );

            pioreqEvictedRun = PioreqIoRunPoolIRemoveRunSlot_( irun );
        }

        Assert( m_rgiorunIoRuns[irun].FEmpty() );
        Assert( irunNil == IrunIoRunPoolIFindFileRun_( pioreqToAdd->p_osf ) );
    }

    if ( !fAdded )
    {

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
            Assert( FEmpty( fCheckAllEmpty ) );
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

    Assert( m_rgiorunIoRuns[irun].PioreqHead() == NULL );

    return pioreqRet;
}

#ifdef DEBUG

BOOL COSDisk::CIoRunPool::FCanCombineWithExistingRun( _In_ const IOREQ * const pioreq ) const
{
    return FCanCombineWithExistingRun( pioreq->p_osf, pioreq->fWrite, pioreq->ibOffset, pioreq->cbData, pioreq->fWrite  );
}


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

#endif


ERR COSDisk::ErrReserveQueueSpace( __in OSFILEQOS grbitQOS, __inout IOREQ * pioreq )
{
    const ERR errReserve = m_pIOQueue->ErrReserveQueueSpace( grbitQOS, pioreq );

    Assert( JET_errSuccess == errReserve ||
            errDiskTilt == errReserve );

    Assert( errDiskTilt == errReserve || pioreq->m_fHasHeapReservation || ( qosIOPoolReserved & grbitQOS ) );

    return errReserve;
}

ERR COSDisk::ErrAllocIOREQ(
    __in OSFILEQOS          grbitQOS,
    __in const _OSFILE *    p_osf,
    __in const BOOL         fWrite,
    __in const QWORD        ibOffsetCombine,
    __in const DWORD        cbDataCombine,
    __out IOREQ **          ppioreq )
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

    (*ppioreq)->m_fCanCombineIO = cbDataCombine &&
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


    if ( !(*ppioreq)->m_fCanCombineIO )
    {
        Call( ErrReserveQueueSpace( grbitQOS, (*ppioreq) ) );

        Assert( (*ppioreq)->m_fHasHeapReservation || ( qosIOPoolReserved & grbitQOS ) );
    }

HandleError:

    if ( errDiskTilt == err )
    {

        Assert( errDiskTilt == err );
        OSDiskIIOREQFree( *ppioreq );
        *ppioreq = NULL;
    }
    else
    {
        Assert( (*ppioreq)->FEnqueueable() || (*ppioreq)->m_fCanCombineIO  || Postls()->pioreqCache == NULL  );
        CallS( err );
    }

    return err;
}

VOID COSDisk::FreeIOREQ(
    __in IOREQ * const  pioreq
    )
{

    if ( !pioreq->m_fCanCombineIO )
    {
        (void)m_pIOQueue->FReleaseQueueSpace( pioreq );
    }

    Assert( !pioreq->m_fHasHeapReservation );
    Assert( !pioreq->m_fHasBackgroundReservation );
    Assert( !pioreq->m_fHasUrgentBackgroundReservation );

    
    pioreq->m_fCanCombineIO = fFalse;


    OSDiskIIOREQFree( pioreq );
}


void COSDisk::EnqueueIORun( __in IOREQ * pioreqHead )
{
    Assert( pioreqHead );
    Assert( pioreqHead->p_osf->m_posd == this );
    Assert( !FExclusiveIoreq( pioreqHead ) );


    COSDisk::QueueOp    qop( pioreqHead );
    Assert( !qop.FEmpty() );

    BOOL fReEnqueueIO = fFalse;
    if ( qop.FIssuedFromQueue() )
    {

        AtomicDecrement( (LONG*)&m_cioDispatching );
        Assert( m_cioDispatching >= 0 );

        if ( pioreqHead->fWrite )
        {
            Assert( FIOThread() );
            m_cioAsyncWriteDispatching--;
            Assert( m_cioAsyncWriteDispatching >= 0 );
        }
        else
        {
            Assert( FIOThread() );
            m_cioAsyncReadDispatching--;
            Assert( m_cioAsyncReadDispatching >= 0 );
        }

        fReEnqueueIO = fTrue;
    }
    else if ( pioreqHead->FIssuedSyncIO() )
    {

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


void COSDisk::EnqueueIOREQ( __in IOREQ * pioreq )
{
    Assert( pioreq );
    Assert( NULL == pioreq->pioreqIorunNext );

    if ( !pioreq->m_fHasHeapReservation && !pioreq->m_fCanCombineIO )
    {
        EnqueueIORun( pioreq );
        return;
    }

    if ( 0 == pioreq->cbData )
    {
        EnqueueIORun( pioreq );
        return;
    }

    if ( pioreq->pioreqIorunNext )
    {
        AssertSz( fFalse, "This should never happen, if so we didn't NULL out pioreqIorunNext upon free, or something else odd ..." );
        EnqueueIORun( pioreq );
        return;
    }

    Assert( pioreq->m_fHasHeapReservation || pioreq->m_fCanCombineIO );
    Assert( !pioreq->m_fCanCombineIO || Postls()->iorunpool.FCanCombineWithExistingRun( pioreq ) );


    pioreq->SetIOREQType( IOREQ::ioreqInIOCombiningList );

    IOREQ * const pioreqEvictedRun = Postls()->iorunpool.PioreqSwapAddIoreq( pioreq );

    Assert( Postls()->iorunpool.FContainsFileIoRun( pioreq->p_osf ) );
    
    if ( pioreqEvictedRun )
    {

    
        pioreqEvictedRun->p_osf->m_posd->EnqueueIORun( pioreqEvictedRun );
    }

}



void COSDisk::EnqueueDeferredIORun( _In_ const _OSFILE * const p_osf )
{

    IOREQ * pioreqT = NULL;
    while( pioreqT = Postls()->iorunpool.PioreqEvictFileRuns( p_osf ) )
    {
    
        pioreqT->p_osf->m_posd->EnqueueIORun( pioreqT );
    }


    if( p_osf )
    {
        Assert( !Postls()->iorunpool.FContainsFileIoRun( p_osf ) );
    }
    else
    {
        Assert( Postls()->iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    }
    Assert( NULL == Postls()->iorunpool.PioreqEvictFileRuns( p_osf ) );
}


ERR COSDisk::ErrBeginConcurrentIo( const BOOL fForegroundSyncIO )
{
    ERR err = JET_errSuccess;

    while ( true )
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

    Assert( !m_fExclusiveIo || err == errDiskTilt );

    m_critIOQueue.Leave();

    Assert( err != errDiskTilt || !fForegroundSyncIO );

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
            Assert( FIOThread() );
            m_cioAsyncWriteDispatching--;
            Assert( m_cioAsyncWriteDispatching >= 0 );
        }
        else
        {
            Assert( FIOThread() );
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
    while( CioDispatched() || CioForeground() )
    {
        cSleepWaits++;
        UtilSleep( cmsecIoWait );
    }
    Assert( CioDispatched() == 0 );
    Assert( CioForeground() == 0 );
    Assert( m_fExclusiveIo == fTrue );

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


    ERR err = ErrBeginConcurrentIo( fFalse );
    CallR( err );


    m_pIOQueue->ExtractOp( this, pqop );

    Assert( pqop->FValid() );
    Assert( !pqop->PioreqOp() || ( NULL == pqop->PioreqOp()->pioreqIorunNext )  );

    Expected( pqop->FEmpty() ||
                pqop->PioreqOp() != NULL ||
                pqop->PiorunIO()->PioreqHead()->fWrite ||
                pqop->PiorunIO()->PioreqHead()->FUseMetedQ() ||
                !pqop->FUseMetedQ() );


    IncCioAsyncDispatching( !pqop->FEmpty()  && pqop->FWrite() );


    if ( pqop->FEmpty() )
    {
        EndConcurrentIo( fFalse, !pqop->FEmpty() && pqop->FWrite() );
        return ErrERRCheck( errDiskTilt );
    }

    m_cioDequeued++;

    if ( pqop->P_osfPerfctrUpdates() )
    {
        Expected( pqop->CbRun() > 0 );
        PERFOpt( pqop->P_osfPerfctrUpdates()->pfpapi->SetCurrentQueueDepths(
                    m_pIOQueue->CioMetedReadQueue(),
                    CioAllowedMetedOps( m_pIOQueue->CioMetedReadQueue() ),
                    m_cioAsyncReadDispatching ) );
    }

    if ( pqop->PioreqOp() == NULL &&
            !pqop->FWrite() && 
            pqop->FUseMetedQ() )
    {
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
        Assert( FIOThread() );
        m_cioAsyncWriteDispatching++;
        Assert( m_cioAsyncWriteDispatching > 0 );
    }
    else
    {
        Assert( FIOThread() );
        m_cioAsyncReadDispatching++;
        Assert( m_cioAsyncReadDispatching > 0 );
    }
}

void COSDisk::QueueCompleteIORun( __in IOREQ * const pioreqHead )
{
    Assert( pioreqHead );

    if ( pioreqHead->FRemovedFromQueue() ||
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

    IOREQ * pioreqT = pioreqHead;
    while ( pioreqT )
    {

        AssertRTL( pioreqT->FInIssuedState() );
        AssertRTL( pioreqT->Ioreqtype() == pioreqHead->Ioreqtype() ||
                    pioreqT->FRemovedFromQueue() );
        AssertRTL( pioreqT->p_osf == pioreqHead->p_osf );
        AssertRTL( pioreqT->fWrite == pioreqHead->fWrite );
        if ( pioreqT->pioreqIorunNext )
        {
            if ( pioreqT->fWrite  )
            {
                AssertRTL( pioreqT->IbBlockMax() == pioreqT->pioreqIorunNext->ibOffset );
            }
            else
            {
                AssertRTL( pioreqT->IbBlockMax() <= pioreqT->pioreqIorunNext->ibOffset );
            }
        }


        pioreqT->CompleteIO( IOREQ::CompletedIO );


        Assert( pioreqT != pioreqT->pioreqIorunNext );
        pioreqT = pioreqT->pioreqIorunNext;
    }
}

BOOL COSDisk::FQueueCompleteIOREQ(
    __in IOREQ * const  pioreq
    )
{

    const BOOL fFreedIoQuota = m_pIOQueue->FReleaseQueueSpace( pioreq );

    Assert( !pioreq->m_fHasHeapReservation );
    Assert( !pioreq->m_fHasBackgroundReservation );
    Assert( !pioreq->m_fHasUrgentBackgroundReservation );

    
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
    if ( dtickStarvation > 9  )
    {
        g_dtickStarvedMetedOpThreshold = dtickStarvation;
    }
}
    
INLINE LONG COSDisk::CioAllowedMetedOps( _In_ const LONG cioWaitingQ ) const
{
    Assert( FIOThread() );



    return !FSeekPenalty() ?
            g_cioOutstandingMax  :
            ( cioWaitingQ < g_cioLowQueueThreshold ?
                1 : g_cioConcurrentMetedOpsMax );
}

INLINE LONG COSDisk::CioReadyMetedEnqueued() const
{
    Assert( FIOThread() );

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
    return CioAllEnqueued() + CioDispatched();
}
#endif

void COSDisk::TrackOsFfbBegin( const IOFLUSHREASON iofr, const QWORD hFile )
{
    if ( !( iofrOsFakeFlushSkipped & iofr ) )
    {
         AtomicIncrement( (LONG*)&m_cFfbOutstanding );
    }
    ETDiskFlushFileBuffersBegin( m_dwDiskNumber, hFile, iofr );
}

void COSDisk::TrackOsFfbComplete( const IOFLUSHREASON iofr, const DWORD error, const HRT hrtStart, const QWORD usFfb, const LONG64 cioreqFileFlushing, const WCHAR * const wszFileName )
{
     AtomicDecrement( (LONG*)&m_cFfbOutstanding );

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


INLINE VOID COSDisk::RefreshDiskPerformance()
{
    if ( TickOSTimeCurrent() - m_tickPerformanceLastMeasured >= s_dtickPerformancePeriod )
    {
        QueryDiskPerformance();
    }
}


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


INLINE DWORD COSDisk::CioOsQueueDepth()
{
    RefreshDiskPerformance();

    return m_cioOsQueueDepth;
}


void COSDisk::IORun::PrepareForIssue(
    __in const IOREQ::IOMETHOD              iomethod,
    __out DWORD * const                     pcbRun,
    __out BOOL * const                      pfIOOSLowPriority,
    __inout PFILE_SEGMENT_ELEMENT const     rgfse,
    __in const DWORD                        cfse
    )
{
    IOREQ * const pioreqHead    = m_storage.Head();

    Assert( pioreqHead );


    Assert( IbOffset() == pioreqHead->ibOffset );
    QWORD ibOffset = pioreqHead->ibOffset;
    pioreqHead->ovlp.Offset     = ULONG( ibOffset );
    pioreqHead->ovlp.OffsetHigh = ULONG( ibOffset >> 32 );
    *pcbRun                     = CbRun();


    *pfIOOSLowPriority = qosIOOSLowPriority & pioreqHead->grbitQOS;

    if ( IOREQ::iomethodScatterGather == iomethod )
    {
        Assert( cfse && rgfse );
        OSDiskIIOPrepareScatterGatherIO( pioreqHead, *pcbRun, cfse, rgfse, pfIOOSLowPriority );
    }
    else
    {
        Assert( pioreqHead->pioreqIorunNext == NULL );
    }


    Assert( pioreqHead->m_grbitHungActionsTaken == 0 );

    const HRT hrtNow = HrtHRTCount();
    for ( IOREQ* pioreq = pioreqHead; pioreq != NULL; pioreq = pioreq->pioreqIorunNext )
    {
        pioreq->BeginIO( iomethod, hrtNow, pioreq == pioreqHead );
    }
}



INLINE bool FIOTemporaryResourceIssue( ERR errIO )
{
    return errIO == JET_errOutOfMemory;
}


INLINE bool FIOMethodTooComplex(
    __in const IOREQ::IOMETHOD      iomethodCurrentFile,
    __in const IOREQ::IOMETHOD      iomethodIO,
    __in const ERR                  errIO )
{
    return iomethodCurrentFile >= iomethodIO &&
            iomethodIO > IOREQ::iomethodSync &&
            errIO == JET_errInvalidParameter;
}


BOOL FIOMgrHandleIOResult(
    __in const IOREQ::IOMETHOD      iomethod,
    __inout IOREQ *                 pioreqHead,
    __in BOOL                       fIOCompleted,
    __in DWORD                      error,
    __in DWORD                      cbTransfer
    )
{
    BOOL fReIssue = fFalse;

    if ( fIOCompleted )
    {

        Assert( IOREQ::iomethodSync == iomethod || IOREQ::iomethodSemiSync == iomethod );
        OSDiskIIOThreadCompleteWithErr( ERROR_SUCCESS, cbTransfer, pioreqHead );
    }
    else
    {

        const ERR errIO = ErrOSFileIFromWinError( error );


        if ( errIO >= 0 )
        {


            PERFOptDeclare( IFilePerfAPI * const    pfpapi  = pioreqHead->p_osf->pfpapi );
            PERFOpt( pfpapi->IncrementIOAsyncPending( pioreqHead->fWrite ) );
        }
        else if ( FIOTemporaryResourceIssue( errIO ) &&
                    !FExclusiveIoreq( pioreqHead ) )
        {

#ifndef RTM
            pioreqHead->m_fOutOfMemory = fTrue;
#endif
            pioreqHead->m_cRetries = min( IOREQ::cRetriesMax, pioreqHead->m_cRetries + 1 );
            Assert( pioreqHead->p_osf && pioreqHead->p_osf->m_posd );
            pioreqHead->p_osf->m_posd->EnqueueIORun( pioreqHead );

            fReIssue = fTrue;
        }
        else if ( FIOMethodTooComplex(  pioreqHead->p_osf->iomethodMost, iomethod, errIO ) &&
                    !FExclusiveIoreq( pioreqHead ) )
        {
            Assert( iomethod != IOREQ::iomethodSemiSync );


#ifndef RTM
            pioreqHead->m_cTooComplex = min( 0x6, pioreqHead->m_cTooComplex + 1 );
#endif
            pioreqHead->m_cRetries = min( IOREQ::cRetriesMax, pioreqHead->m_cRetries + 1 );

            pioreqHead->p_osf->iomethodMost = IOREQ::IOMETHOD( pioreqHead->p_osf->iomethodMost - 1 );
            AssertRTL( pioreqHead->p_osf->iomethodMost >= 0 );
            AssertRTL( IOREQ::iomethodSync != pioreqHead->p_osf->iomethodMost );
            AssertRTL( IOREQ::iomethodSemiSync != pioreqHead->p_osf->iomethodMost );

            Assert( pioreqHead->p_osf && pioreqHead->p_osf->m_posd );
            pioreqHead->p_osf->m_posd->EnqueueIORun( pioreqHead );

            fReIssue = fTrue;
        }
        else
        {

            OSDiskIIOThreadCompleteWithErr( error, 0, pioreqHead );
        }
    }

    return fReIssue;
}

QWORD g_cSplitAndIssueRunning = fFalse;

BOOL FIOMgrSplitAndIssue(
    __in const IOREQ::IOMETHOD              iomethod,
    __inout COSDisk::IORun *                piorun
    )
{
    BOOL fReIssue = fFalse;

    AtomicAdd( &g_cSplitAndIssueRunning, 1 );

    IOREQ * const pioreqHead = piorun->PioreqGetRun();
    piorun = NULL;
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
        
        const BOOL fHasHeapReservation = pioreq->m_fHasHeapReservation;
        Assert( fHasHeapReservation );
        if ( !IOChain.FEmpty() && !IOChain.Head()->m_fHasHeapReservation  )
        {
            pioreq->m_fHasHeapReservation = fFalse;
            IOChain.Head()->m_fHasHeapReservation = fHasHeapReservation;
        }

        if ( pioreq != pioreqHead )
        {
            Assert( pioreq->p_osf->m_posd->CioDispatched() >= 1 );

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


        DWORD cbTransfer    = 0;
        BOOL  fIOCompleted  = fFalse;

        const DWORD errorIO = ErrorIOMgrIssueIO( &iorunSplit, iomethod, NULL, 0, &fIOCompleted, &cbTransfer );

        const ERR errIO = ErrOSFileIFromWinError( errorIO );


        const BOOL fReEnqueue = FIOTemporaryResourceIssue( errIO ) ||
                                    FIOMethodTooComplex( pioreq->p_osf->iomethodMost, iomethod, errIO );

        if ( fReEnqueue )
        {
            if ( !IOChain.FEmpty() )
            {
                Assert( IOChain.Head()->m_fHasHeapReservation );
                if ( !pioreq->m_fHasHeapReservation )
                {
                    Assert( IOChain.Head()->m_fHasHeapReservation );
                    IOChain.Head()->m_fHasHeapReservation = fFalse;
                    pioreq->m_fHasHeapReservation = fTrue;
                }
            }
            else
            {
                Assert( pioreq->m_fHasHeapReservation );
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
            break;
        }

    }

    return fReIssue;
}

BOOL FIOMgrIssueIORunRegular(
    __in const IOREQ::IOMETHOD          iomethod,
    __inout COSDisk::IORun *            piorun
    )
{
    DWORD cfse = 0;
    DWORD cbfse = 0;
    PFILE_SEGMENT_ELEMENT rgfse = NULL;
    BOOL fAllocatedFromHeap = fFalse;
    USHORT cbStackAllocMax = 4096;

    if ( IOREQ::iomethodScatterGather == iomethod )
    {
        cfse = ( piorun->CbRun() + OSMemoryPageCommitGranularity() - 1 ) / OSMemoryPageCommitGranularity() + 1;
        cbfse = sizeof( *rgfse ) * cfse;

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
                return FIOMgrSplitAndIssue( static_cast<const IOREQ::IOMETHOD>( iomethod - 1 ), piorun );
            }
            fAllocatedFromHeap = fTrue;
        }
        Assert( 0 == ( UINT_PTR )rgfse % sizeof( FILE_SEGMENT_ELEMENT ) );
    }


    DWORD cbTransfer    = 0;
    BOOL  fIOCompleted  = fFalse;
    IOREQ* pioreqHead   = NULL;

    const DWORD errorIO = ErrorIOMgrIssueIO( piorun, iomethod, rgfse, cfse, &fIOCompleted, &cbTransfer, &pioreqHead );
    piorun = NULL;
    
    
    if( fAllocatedFromHeap )
    {
        Assert( cbfse > cbStackAllocMax );
        OSMemoryHeapFreeAlign( rgfse );
    }
    return FIOMgrHandleIOResult( iomethod, pioreqHead, fIOCompleted, errorIO, cbTransfer );
}


VOID IOMgrIssueSyncIO( IOREQ * pioreqSingle )
{

    const BOOL fExclusiveIo = FExclusiveIoreq( pioreqSingle );
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


    COSDisk::IORun iorunSingle;
    const BOOL fAccepted = iorunSingle.FAddToRun( pioreqSingle );
    Assert( fAccepted );


    pioreqSingle->m_iocontext = pioreqSingle->p_osf->pfpapi->IncrementIOIssue( pioreqSingle->p_osf->m_posd->CioOsQueueDepth(), pioreqSingle->fWrite );

    DWORD cbTransfer    = 0;
    BOOL  fIOCompleted  = fFalse;

    const DWORD errorIO = ErrorIOMgrIssueIO( &iorunSingle, IOREQ::iomethodSemiSync, NULL, 0, &fIOCompleted, &cbTransfer );

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


    const BOOL fNeedIssue = FIOMgrHandleIOResult( IOREQ::iomethodSemiSync, pioreqSingle, fIOCompleted, errorIO, cbTransfer );
    Assert( !fExclusiveIo || !fNeedIssue );



    if ( fNeedIssue )
    {
        
        
        
        OSDiskIOThreadStartIssue( NULL );
    }
}

void IOMgrCompleteOp(
    __inout IOREQ * const       pioreq
    )
{

    Assert( pioreq );
    Assert( pioreq->cbData == 0 );

    QWORD ibOffset = pioreq->ibOffset;
    pioreq->ovlp.Offset         = ULONG( ibOffset );
    pioreq->ovlp.OffsetHigh     = ULONG( ibOffset >> 32 );


    OSDiskIIOThreadCompleteWithErr( ERROR_SUCCESS, 0, pioreq );
}


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

        if ( !FOSDiskIFileSGIOCapable( P_OSF() )    ||
            !FOSDiskIDataSGIOCapable( pioreqT->pbData, pioreqT->cbData ) )
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
    

    Assert( piorun );
    ASSERT_VALID( piorun );
    Assert( piorun->CbRun() != 0 );


    IOREQ::IOMETHOD iomethod = min( piorun->P_OSF()->iomethodMost,
                                            piorun->IomethodGet() );

    if ( !piorun->P_OSF()->fRegistered )
    {
        iomethod = IOREQ::iomethodSemiSync;
    }

    Assert( iomethod <= IOREQ::iomethodScatterGather );
    Assert( iomethod <= piorun->P_OSF()->iomethodMost );
    Assert( piorun->CbRun() != 0 || iomethod != IOREQ::iomethodScatterGather );

    if ( IOREQ::iomethodScatterGather != iomethod && piorun->FMultiBlock() )
    {

        fReIssue = FIOMgrSplitAndIssue( iomethod, piorun );
    }
    else
    {

        fReIssue = FIOMgrIssueIORunRegular( iomethod, piorun );
    }


    return fReIssue ? ErrERRCheck( errDiskTilt ) : JET_errSuccess;
}

ERR ErrOSDiskProcessIO(
    _Inout_ COSDisk * const     posd,
    _In_ const BOOL             fFromCompletion
    )
{
    ERR err = JET_errSuccess;
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


    ULONG cioToGo;
    ULONG cioProcessed = 0;
    while ( cioToGo = ( ( fFromCompletion ? 0 : posd->CioUrgentEnqueued() ) + posd->CioReadyMetedEnqueued() ) )
    {
        Assert( JET_errSuccess == err );


        COSDisk::QueueOp    qop;

        Assert( qop.FEmpty() );


        Call( posd->ErrDequeueIORun( &qop ) );


        Assert( qop.FValid() );
        Assert( !qop.FEmpty() );

        if ( qop.PioreqOp() )
        {

            Assert( qop.PiorunIO() == NULL );
            IOMgrCompleteOp( qop.PioreqOp() );
        }

        if ( qop.PiorunIO() )
        {

            Assert( qop.PioreqOp() == NULL );
            cioProcessed++;
            Call( ErrIOMgrIssueIORun( qop.PiorunIO() ) );
        }

    }// while more IOs to issue ...

HandleError:

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
            
        }
        else
        {
            Assert( errDiskTilt == errDisk );
        }

    }

    ETIOThreadIssueProcessedIO( errDisk, cDisksProcessed, CusecHRTFromDhrt( HrtHRTCount() - hrtIssueStart ) );
}


void OSDiskIIOThreadIIssue( const DWORD     dwError,
                            const DWORD_PTR dwReserved1,
                            const DWORD     dwReserved2,
                            const DWORD_PTR dwReserved3 )
{

    ETIOThreadIssueStart();


    Postls()->fIOThread = fTrue;

    

    IOMgrProcessIO();


    Postls()->fIOThread = fFalse;
}

VOID IFilePerfAPI::Init()
{
    m_tickLastDiskFull = TickOSTimeCurrent() - dtickOSFileDiskFullEvent;
    m_tickLastAbnormalIOLatencyEvent = 0;
    m_cmsecLastAbnormalIOLatency = 0;
    m_cAbnormalIOLatencySinceLastEvent = 0;
}

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

    if ( ( tickNow - m_tickLastAbnormalIOLatencyEvent ) < dtickOSFileAbnormalIOLatencyEvent
        && cmsecIOElapsed < m_cmsecLastAbnormalIOLatency * 3 / 2 )
    {
        fReportError = fFalse;

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



VOID OSDiskIIOReportIOLatency(
    IOREQ * const   pioreqHead,
    const QWORD     cmsecIOElapsed )
{
    IFileAPI *      pfapi       = (IFileAPI*)pioreqHead->p_osf->keyFileIOComplete;
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


    rgpwsz[ irgpwsz++ ] = wszTimeElapsed;

    if ( 0 == pioreqHead->p_osf->pfpapi->TickLastAbnormalIOLatencyEvent() )
    {
        msgid = ( pioreqHead->fWrite ? OSFILE_WRITE_TOO_LONG_ID : OSFILE_READ_TOO_LONG_ID );
    }
    else
    {
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
    const ULONG     cwsz         = 7;
    const WCHAR *   rgpwsz[ cwsz ];
    DWORD           irgpwsz                      = 0;
    WCHAR           wszAbsPath[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszOffset[ 64 ];
    WCHAR           wszLength[ 64 ];
    WCHAR           wszTimeElapsed[ 64 ];

    CallS( posf->ErrPath( wszAbsPath ) );
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
    posf->Pfsconfig()->EmitEvent(   eventError,
                                    GENERAL_CATEGORY,
                                    fWrite ? OSFILE_WRITE_ERROR_ID : OSFILE_READ_ERROR_ID,
                                    irgpwsz,
                                    rgpwsz,
                                    JET_EventLoggingLevelMin );

#if defined( USE_HAPUBLISH_API )
    posf->Pfsconfig()->EmitEvent(   OSDiskIIOHaTagOfErr( err, fWrite ),
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


class OSIOREASONALLUP {

#ifdef DEBUG
    BOOL                m_fAdded;
    BOOL                m_fDead;
#endif

    BOOL                m_fTooLong;

    BOOL                m_fSeekPenalty;
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
    const UserTraceContext* m_putc;
    TraceContext        m_etc;

    DWORD               m_tidAlloc;
    DWORD               m_dtickQueueWorst;

    OSFILEQOS           m_qosDispatchHighest;
    OSFILEQOS           m_qosDispatchLowest;

    OSFILEQOS           m_qosOther;
    OSFILEQOS           m_qosHighestFirst;

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
            m_dtickQueueWorst = 0;
            }
        m_tickStartCompletions = TickOSTimeCurrent();

        m_grbitMultiContext = 0x0;
        m_qosDispatchLowest = qosIODispatchMask;
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

        Assert( m_iFile == pioreq->p_osf->iFile );
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


        #define CombineNewIorxElement( Iorx, SetIorx, iorxNone, bitMultiIorx )  \
            if ( m_etc.iorReason.Iorx() != pioreq->m_tc.etc.iorReason.Iorx() )  \
            {                                                                   \
                m_grbitMultiContext |= bitMultiIorx;                            \
            }

        CombineNewIorxElement( Iorp, SetIorp, iorpNone, bitIoTraceMultiIorp );
        CombineNewIorxElement( Iors, SetIors, iorsNone, bitIoTraceMultiIors );
        CombineNewIorxElement( Iort, SetIort, iortNone, bitIoTraceMultiIort );
        CombineNewIorxElement( Ioru, SetIoru, ioruNone, bitIoTraceMultiIoru );

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
    
        Assert( 0 == ( m_qosDispatchHighest & 0xFFFFFFFF00000000 ) );
        Assert( 0 == ( m_qosDispatchLowest & 0xFFFFFFFF00000000 ) );
        Assert( 0 == ( m_qosOther & 0xFFFFFFFF00000000 ) );
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

        Assert( 0 == ( m_qosDispatchHighest & 0xFFFFFFFF00000000 ) );
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
            m_dtickQueueWorst,
            m_tidAlloc,
            m_dwEngineFileType,
            m_qwEngineFileId,
            m_fmfFile,
            m_dwDiskNumber,
            m_etc.dwEngineObjid,
            m_qosHighestFirst  );

        Assert( 0 == ( m_qosDispatchHighest & 0xFFFFFFFF00000000 ) );
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

        bool fTraceReads = !!( m_putc->dwIOSessTraceFlags & JET_bitIOSessTraceReads );
        bool fTraceWrites = !!( m_putc->dwIOSessTraceFlags & JET_bitIOSessTraceWrites );
        bool fTraceHDD = !!( m_putc->dwIOSessTraceFlags & JET_bitIOSessTraceHDD );
        bool fTraceSSD = !!( m_putc->dwIOSessTraceFlags & JET_bitIOSessTraceSSD );

        if ( ( ( fWrite && fTraceWrites ) || ( !fWrite && fTraceReads ) ) &&
             ( ( m_fSeekPenalty && fTraceHDD ) || ( !m_fSeekPenalty && fTraceSSD ) ) )
        {
            Assert( 0 == ( m_qosDispatchHighest & 0xFFFFFFFF00000000 ) );
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

    QWORD       cmsecIOElapsed      = CmsecHRTFromDhrt( dhrtIOElapsed );


    const ULONG ciotime             = pioreq->Ciotime();

    if ( cmsecIOElapsed > CmsecHighWaitFromIOTime( ciotime ) )
    {


        cmsecIOElapsed = UlFunctionalMax( CmsecLowWaitFromIOTime( ciotime ), 20 );
    }

    return cmsecIOElapsed;
}

QWORD CmsecLatencyOfOSOperation( const IOREQ* const pioreq )
{
    return CmsecLatencyOfOSOperation( pioreq, HrtHRTCount() - pioreq->hrtIOStart );
}



void OSDiskIIOThreadCompleteWithErr( DWORD error, DWORD cbTransfer, IOREQ* pioreqHead )
{
    IFilePerfAPI * const    pfpapi              = pioreqHead->p_osf->pfpapi;


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


    if ( !RFSAlloc( pioreqHead->fWrite ? OSFileWrite : OSFileRead ) )
    {

        if ( !( pioreqHead->cbData == 0 && pioreqHead->pioreqIorunNext == NULL ) && ( error == ERROR_SUCCESS ) )
        {
            error = ERROR_IO_DEVICE;
        }
    }

    if ( ErrFaultInjection( 64867 ) && cbTransfer != 0 && error == ERROR_SUCCESS  )
    {
        fIOTookTooLong = fTrue;
    }

    pfpapi->IncrementIOCompletion(
                pioreqHead->m_iocontext,
                pioreqHead->p_osf->m_posd->DwDiskNumber(),
                pioreqHead->grbitQOS,
                dhrtIOElapsed,
                cbTransfer,
                pioreqHead->fWrite );


    Assert( ERROR_IO_PENDING != error );
    const ERR   err             = ErrOSFileIFromWinError( error );
    BOOL        fReportError        = fFalse;

    if ( ERROR_HANDLE_EOF == error )
    {
        Assert( JET_errFileIOBeyondEOF == err );
    }
    else if ( ERROR_DISK_FULL == error )
    {
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
    _OSFILE * p_osfHead = pioreqHead->p_osf;
    COSDisk * posdHead = pioreqHead->p_osf->m_posd;
    Assert( p_osfHead != NULL );
    Assert( posdHead != NULL );
#endif


    pioreqHead->p_osf->m_posd->QueueCompleteIORun( pioreqHead );

    CLocklessLinkedList< IOREQ > IOChain( pioreqHead );


    IOREQ * pioreq = pioreqHead;
    while ( pioreq )
    {
        osiorall.Add( pioreq );
        pioreq = pioreq->pioreqIorunNext;
    }


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


        Assert( p_osfHead == pioreq->p_osf );
        Assert( posdHead == pioreq->p_osf->m_posd );
        Assert( pioreq->m_posdCurrentIO == NULL || pioreq == pioreqHead );

        Assert( COSFile::FOSFileManagementOperation( pioreq ) ||
                !COSFile::FOSFileManagementOperation( pioreq ) );


        Assert( 0 == ( pioreq->grbitQOS & qosIOCompleteMask ) );

        Assert( pioreq->fCoalesced || pioreq == pioreqHead );
        if ( pioreq->fCoalesced )
        {
            pioreq->grbitQOS |= qosIOCompleteIoCombined;
        }
        if ( fIOTookTooLong )
        {
            pioreq->grbitQOS |= qosIOCompleteIoSlow;
        }
 

        if ( pioreq->p_osf->m_posd->FQueueCompleteIOREQ( pioreq ) )
        {
            Assert( pioreq->fWrite );
            pioreq->grbitQOS |= qosIOCompleteWriteGameOn;
        }

        if ( FIOThread() && pioreq->p_osf->m_posd->CioReadyMetedEnqueued() > 0 )
        {
            pioreq->grbitQOS |= qosIOCompleteReadGameOn;
        }


        Assert( 0 == pioreq->Ciotime() );


        const DWORD errorIOREQ  = ( error != ERROR_SUCCESS ?
                                        error :
                                        (   pioreq->ibOffset + pioreq->cbData <= ibOffsetHead + cbTransfer ?
                                                ERROR_SUCCESS :
                                                ERROR_HANDLE_EOF ) );

        Assert( ERROR_IO_PENDING != errorIOREQ );
        ERR errIOREQ = ErrOSFileIFromWinError( errorIOREQ );


        if ( fIOTookTooLong && JET_errSuccess <= errIOREQ && ( pioreq->grbitQOS & qosIOSignalSlowSyncIO ) )
        {
            errIOREQ = ErrERRCheck( wrnIOSlow );
        }
        
        if ( pioreq == pioreqHead )
        {
            osiorall.OSTraceMe( errIOREQ, pioreq->grbitQOS );
        }

        pioreqHead = NULL;
        osiorall.Kill();

        const OSFILEQOS qosTr = pioreq->grbitQOS;


        IOREQ ioreq( pioreq );
        ASSERT_VALID_RTL( &ioreq );
        Assert( ioreq.FCompleted() );

        pioreq->m_tc.Clear();

        if ( pioreq->fFromReservePool )
        {
            OSDiskIIOREQFree( pioreq );
        }
        else
        {
            OSDiskIIOREQFreeToCache( pioreq );
        }

#ifdef DEBUG

        if ( iomethod == IOREQ::iomethodAsync ||
                iomethod == IOREQ::iomethodScatterGather ||
                FIOThread() )
        {
            Expected( FIOThread() );
            CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );
        }
#endif


        DWORD cDisableDeadlockDetection = 0;
        DWORD cDisableOwnershipTracking = 0;
        DWORD cLocks = 0;
        CLockDeadlockDetectionInfo::GetApiEntryState( &cDisableDeadlockDetection, &cDisableOwnershipTracking, &cLocks );


        const ERR errTr = errIOREQ;
        ioreq.p_osf->pfnFileIOComplete( &ioreq,
                                        errIOREQ,
                                        ioreq.p_osf->keyFileIOComplete );


        CLockDeadlockDetectionInfo::AssertCleanApiExit( cDisableDeadlockDetection, cDisableOwnershipTracking, cLocks );


        pioreq = PioreqOSDiskIIOREQAllocFromCache();
        if ( pioreq )
        {
            OSDiskIIOREQFree( pioreq );
        }

        const HRT dhrtIOCompleteElapsed = HrtHRTCount() - hrtIoreqCompleteStart;
        if ( CusecHRTFromDhrt( dhrtIOCompleteElapsed ) > 500  )
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
    Expected( GetLastError() == dwError );

    Postls()->fIOThread = fTrue;

    Assert( pioreqHead );

    if ( pioreqHead )
    {

        Assert( FOSDiskIIOREQSlowlyCheckIsOurOverlappedIOREQ( pioreqHead ) );


        COSDisk * posdReDequeue = NULL;
        Assert( pioreqHead->m_posdCurrentIO );


        if ( pioreqHead->m_posdCurrentIO  )
        {
            posdReDequeue = pioreqHead->m_posdCurrentIO;

            OSDiskConnect( posdReDequeue );
        }


        PERFOptDeclare( IFilePerfAPI * const    pfpapi  = pioreqHead->p_osf->pfpapi );
        PERFOpt( pfpapi->DecrementIOAsyncPending( pioreqHead->fWrite ) );


        Assert( pioreqHead->FIssuedAsyncIO() );

        OSDiskIIOThreadCompleteWithErr( dwError, cbTransfer, pioreqHead );

        
        if ( posdReDequeue )
        {
            Assert( FIOThread() );

            if ( posdReDequeue->CioReadyMetedEnqueued() > 0 ||
                 posdReDequeue->CioReadyWriteEnqueued() > 0 )
            {
                const ERR errRe = ErrOSDiskProcessIO( posdReDequeue, fTrue );
                if ( errRe < JET_errSuccess && errRe != errDiskTilt )
                {
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


void OSDiskIIOThreadTerm( void )
{

    if ( g_postaskmgrFile )
    {
        g_postaskmgrFile->TMTerm();
        delete g_postaskmgrFile;
        g_postaskmgrFile = NULL;
    }
}


ERR ErrOSDiskIIOThreadInit( void )
{
    ERR err;


    g_postaskmgrFile  = NULL;


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


void OSDiskIOThreadStartIssue( const P_OSFILE p_osf )
{
    Assert( p_osf == NULL || p_osf->m_posd );


    Expected( p_osf == NULL || !Postls()->iorunpool.FContainsFileIoRun( p_osf ) );


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
    Assert( Postls() );
    return Postls() ? Postls()->fIOThread : fFalse;
}


VOID OSDiskIIOThreadIRetryIssue()
{
    Assert( FIOThread() );
    
    g_postaskmgrFile->TMRequestIdleCallback( 2, OSDiskIIOThreadIIssue );
}

