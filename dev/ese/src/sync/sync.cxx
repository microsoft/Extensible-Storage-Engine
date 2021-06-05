// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// tchar.h must be above where strsafe.h is includes.
#pragma prefast(push)
#pragma prefast(disable:26006, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:26007, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28718, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28726, "Dont bother us with tchar, someone else owns that.")
#include <tchar.h>
#pragma prefast(pop)

#include "sync.hxx"

#define STRSAFE_NO_DEPRECATE 1
#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with strsafe, someone else owns that.")
#pragma prefast(disable:28205, "Do not bother us with strsafe, someone else owns that.")
#include <strsafe.h>
#pragma prefast(pop)


//  Random Fault Injection

#ifdef DEBUG
#define FRandomlyFaultTrue()        ( rand() % 2 )
#else
#define FRandomlyFaultTrue()        ( fFalse )
#endif

#ifdef DEBUG
#define FRandomlyFaultFalse()       ( rand() % 2 )
#else
#define FRandomlyFaultFalse()       ( fTrue )
#endif


//  Performance Monitoring Support

#ifdef MINIMAL_FUNCTIONALITY
#define PERFOpt( x )
#else  //  !MINIMAL_FUNCTIONALITY
#define PERFOpt( x ) x

LONG g_cOSSYNCThreadBlock;
LONG LOSSYNCThreadBlockCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = g_cOSSYNCThreadBlock;
    }

    return 0;
}

LONG g_cOSSYNCThreadResume;
LONG LOSSYNCThreadsBlockedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = g_cOSSYNCThreadBlock - g_cOSSYNCThreadResume;
    }

    return 0;
}

#endif  //  MINIMAL_FUNCTIONALITY


namespace OSSYNC {

//  system is aborting unhappily

BOOL g_fSyncProcessAbort = fFalse;

//  enable/disable timeout deadlock detection

enum SYNCDeadLockTimeOutState // sdltos
{
    sdltosDisabled          = 0,
    sdltosEnabled           = 1,
    sdltosCheckInProgress   = 2
};
SYNCDeadLockTimeOutState g_sdltosState = sdltosEnabled;

//  system max spin count

INT g_cSpinMax;


//  Page Memory Allocation

void* PvPageAlloc( const size_t cbSize, void* const pv );
void* PvPageReserve( const size_t cbSize, void* const pv );
void PageFree( void* const pv );
BOOL FPageCommit( void* const pv, const size_t cb );
void PageDecommit( void* const pv, const size_t cb );


//  Performance Data Dump

void OSSyncStatsDump(   const char*         szTypeName,
                        const char*         szInstanceName,
                        const CSyncObject*  psyncobj,
                        DWORD               group,
                        QWORD               cWait,
                        double              csecWaitElapsed,
                        QWORD               cAcquire,
                        QWORD               cContend,
                        QWORD               cHold,
                        double              csecHoldElapsed );


//  Kernel Semaphore Pool

//  ctor

CKernelSemaphorePool::CKernelSemaphorePool()
{
}

//  dtor

CKernelSemaphorePool::~CKernelSemaphorePool()
{
}

//  init

const BOOL CKernelSemaphorePool::FInit()
{
    //  semaphore pool should be terminated

    OSSYNCAssert( !FInitialized() );

    //  reset members

    m_mpirksemrksem = NULL;
    m_cksem         = 0;

    //  allocate kernel semaphore array

    m_mpirksemrksem = (CReferencedKernelSemaphore*)PvPageReserve( sizeof( CReferencedKernelSemaphore ) * 65536, NULL );

    if ( !( m_mpirksemrksem ) )
    {
        Term();
        return fFalse;
    }

    //  initially commit enough memory for 1k semaphores to mitigate the risk
    //  that we will trip the Enforce in AllocateNew

    if ( !FPageCommit( m_mpirksemrksem, sizeof( m_mpirksemrksem[ 0 ] ) * 1000 ) )
    {
        Term();
        return fFalse;
    }

    //  init successful

    return fTrue;
}

//  term

void CKernelSemaphorePool::Term()
{
    //  the kernel semaphore array is allocated

    if ( m_mpirksemrksem )
    {
        //  terminate all initialized kernel semaphores

        const LONG cksem = m_cksem;
        for ( m_cksem-- ; m_cksem >= 0; m_cksem-- )
        {
            m_mpirksemrksem[m_cksem].Term();
            m_mpirksemrksem[m_cksem].~CReferencedKernelSemaphore();
        }

        //  delete the kernel semaphore array

        PageDecommit( m_mpirksemrksem, sizeof( m_mpirksemrksem[ 0 ] ) * cksem );
        PageFree( m_mpirksemrksem );
    }

    //  reset data members

    m_mpirksemrksem = 0;
    m_cksem         = 0;
}

//  Referenced Kernel Semaphore

//  ctor

CKernelSemaphorePool::CReferencedKernelSemaphore::CReferencedKernelSemaphore()
    :   CKernelSemaphore( CSyncBasicInfo( "CKernelSemaphorePool::CReferencedKernelSemaphore" ) )
{
    //  reset data members

    m_cReference    = 0;
    m_fInUse        = 0;
    m_fAvailable    = 0;
#ifdef SYNC_VALIDATE_IRKSEM_USAGE
    m_psyncobjUser  = 0;
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE
}

//  dtor

CKernelSemaphorePool::CReferencedKernelSemaphore::~CReferencedKernelSemaphore()
{
}

//  init

const BOOL CKernelSemaphorePool::CReferencedKernelSemaphore::FInit()
{
    //  reset data members

    m_cReference    = 0;
    m_fInUse        = 0;
    m_fAvailable    = 0;
#ifdef SYNC_VALIDATE_IRKSEM_USAGE
    m_psyncobjUser  = 0;
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE

    //  initialize the kernel semaphore

    return CKernelSemaphore::FInit();
}

//  term

void CKernelSemaphorePool::CReferencedKernelSemaphore::Term()
{
    //  terminate the kernel semaphore

    CKernelSemaphore::Term();

    //  reset data members

    m_cReference    = 0;
    m_fInUse        = 0;
    m_fAvailable    = 0;
#ifdef SYNC_VALIDATE_IRKSEM_USAGE
    m_psyncobjUser  = 0;
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE
}

//  fwd decl because in header it is restricted to !RTM to discourage other users

DWORD OSSYNCAPI DwOSSyncITickTime();

//  allocates a new irksem and adds it to the stack's irksem pool

const CKernelSemaphorePool::IRKSEM CKernelSemaphorePool::AllocateNew()
{
    //  atomically allocate a position in the stack's irksem pool for our new
    //  irksem

    const LONG lDelta = 0x00000001;
    const LONG lBI = AtomicExchangeAdd( (LONG*) &m_cksem, lDelta );

    const IRKSEM irksem = IRKSEM( lBI );

    //  ensure that the memory backing this irksem is committed

    DWORD tickStart = DwOSSyncITickTime();
    BOOL fCommit = fFalse;
    DWORD dwTimeElapsed = 0;
    do
    {
        fCommit = FPageCommit( m_mpirksemrksem, sizeof( m_mpirksemrksem[ 0 ] ) * ( irksem + 1 ) );
        //  When using GetTickCount, subtraction is safe and returns elapsed time even if DwOSSyncITickTime wraps around
        dwTimeElapsed = (DwOSSyncITickTime() - tickStart);
    }
    while( !fCommit &&
          ( dwTimeElapsed < ( 3 * 60 * 1000 ) /* 3 minutes, we're done!  Drop through to enforce. */ ) );
    
    OSSYNCEnforceSz( fCommit, "Could not allocate a Kernel Semaphore" );

    //  initialize this irksem

    new ( &m_mpirksemrksem[irksem] ) CReferencedKernelSemaphore;

    tickStart = DwOSSyncITickTime();
    BOOL fInitKernelSemaphore = fFalse;
    do
    {
        fInitKernelSemaphore = m_mpirksemrksem[irksem].FInit();
        //  When using GetTickCount, subtraction is safe and returns elapsed time even if DwOSSyncITickTime wraps around
        dwTimeElapsed = (DwOSSyncITickTime() - tickStart);
    }
    while( !fInitKernelSemaphore &&
        ( dwTimeElapsed < ( 3 * 60 * 1000 ) /* 3 minutes, we're done!  Drop through to enforce. */ ) );
    

    OSSYNCEnforceSz( fInitKernelSemaphore, "Could not allocate a Kernel Semaphore" );

    //  return the irksem for use

    return irksem;
}


//  Global Kernel Semaphore Pool

CKernelSemaphorePool g_ksempoolGlobal;


//  Synchronization Object Performance:  Acquisition

//  ctor

CSyncPerfAcquire::CSyncPerfAcquire()
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    m_cAcquire = 0;
    m_cContend = 0;

#endif  //  SYNC_ANALYZE_PERFORMANCE
}

//  dtor

CSyncPerfAcquire::~CSyncPerfAcquire()
{
}


//  Auto-Reset Signal

//  ctor

CAutoResetSignal::CAutoResetSignal( const CSyncBasicInfo& sbi )
    :   CEnhancedStateContainer< CAutoResetSignalState, CSyncStateInitNull, CAutoResetSignalInfo, CSyncBasicInfo >( syncstateNull, sbi )
{
    //  further init of CSyncBasicInfo

    State().SetTypeName( "CAutoResetSignal" );
    State().SetInstance( (CSyncObject*)this );
}

//  dtor

CAutoResetSignal::~CAutoResetSignal()
{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

    //  dump performance data

    OSSyncStatsDump(    State().SzTypeName(),
                        State().SzInstanceName(),
                        State().Instance(),
                        (DWORD)-1,
                        State().CWaitTotal(),
                        State().CsecWaitElapsed(),
                        State().CAcquireTotal(),
                        State().CContendTotal(),
                        0,
                        0 );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
}

//  waits for the signal to be set, returning fFalse if unsuccessful in the time
//  permitted.  Infinite and Test-Only timeouts are supported.

const BOOL CAutoResetSignal::_FWait( const INT cmsecTimeout )
{
    //  if we spin, we will spin for the full amount recommended by the OS

    INT cSpin = g_cSpinMax;

    //  we start with no kernel semaphore allocated

    CKernelSemaphorePool::IRKSEM irksemAlloc = CKernelSemaphorePool::irksemNil;

    //  try forever until we successfully change the state of the signal

    OSSYNC_FOREVER
    {
        //  read the current state of the signal

        const CAutoResetSignalState stateCur = (CAutoResetSignalState&) State();

        //  the signal is set

        if ( stateCur.FNoWaitAndSet() )
        {
            //  we successfully changed the signal state to reset with no waiters

            if ( State().FChange( stateCur, CAutoResetSignalState( 0 ) ) )
            {
                //  if we allocated a kernel semaphore, release it

                if ( irksemAlloc != CKernelSemaphorePool::irksemNil )
                {
                    g_ksempoolGlobal.Unreference( irksemAlloc );
                }

                //  return success

                State().SetAcquire();
                return fTrue;
            }
        }

        //  the signal is not set and we still have spins left

        else if ( cSpin )
        {
            //  spin once and try again

            cSpin--;
            continue;
        }

        //  the signal is not set and there are no waiters

        else if ( stateCur.FNoWaitAndNotSet() )
        {
            //  allocate and reference a kernel semaphore if we haven't already

            if ( irksemAlloc == CKernelSemaphorePool::irksemNil )
            {
                irksemAlloc = g_ksempoolGlobal.Allocate( this );
            }

            //  we successfully installed ourselves as the first waiter

            if ( State().FChange( stateCur, CAutoResetSignalState( 1, irksemAlloc ) ) )
            {
                //  wait for signal to be set

                State().StartWait();
                const BOOL fCompleted = g_ksempoolGlobal.Ksem( irksemAlloc, this ).FAcquire( cmsecTimeout );
                State().StopWait();

                //  our wait completed

                if ( fCompleted )
                {
                    //  unreference the kernel semaphore

                    g_ksempoolGlobal.Unreference( irksemAlloc );

                    //  we successfully waited for the signal

                    State().SetAcquire();
                    return fTrue;
                }

                //  our wait timed out

                else
                {
                    //  try forever until we successfully change the state of the signal

                    OSSYNC_INNER_FOREVER
                    {
                        //  read the current state of the signal

                        const CAutoResetSignalState stateAfterWait = (CAutoResetSignalState&) State();

                        //  there are no waiters or the kernel semaphore currently
                        //  in the signal is not the same as the one we allocated

                        if ( stateAfterWait.FNoWait() || stateAfterWait.Irksem() != irksemAlloc )
                        {
                            //  the kernel semaphore we allocated is no longer in
                            //  use, so another context released it.  this means that
                            //  there is a count on the kernel semaphore that we must
                            //  absorb, so we will

                            //  NOTE:  we could end up blocking because the releasing
                            //  context may not have released the semaphore yet

                            g_ksempoolGlobal.Ksem( irksemAlloc, this ).Acquire();

                            //  unreference the kernel semaphore

                            g_ksempoolGlobal.Unreference( irksemAlloc );

                            //  we successfully waited for the signal

                            return fTrue;
                        }

                        //  there is one waiter and the kernel semaphore currently
                        //  in the signal is the same as the one we allocated

                        else if ( stateAfterWait.CWait() == 1 )
                        {
                            OSSYNCAssert( stateAfterWait.FWait() );
                            OSSYNCAssert( stateAfterWait.Irksem() == irksemAlloc );

                            //  we successfully changed the signal to the reset with
                            //  no waiters state

                            if ( State().FChange( stateAfterWait, CAutoResetSignalState( 0 ) ) )
                            {
                                //  unreference the kernel semaphore

                                g_ksempoolGlobal.Unreference( irksemAlloc );

                                //  we did not successfully wait for the signal

                                return fFalse;
                            }
                        }

                        //  there are many waiters and the kernel semaphore currently
                        //  in the signal is the same as the one we allocated

                        else
                        {
                            OSSYNCAssert( stateAfterWait.CWait() > 1 );
                            OSSYNCAssert( stateAfterWait.FWait() );
                            OSSYNCAssert( stateAfterWait.Irksem() == irksemAlloc );

                            //  we successfully reduced the number of waiters on the
                            //  signal by one

                            if ( State().FChange( stateAfterWait, CAutoResetSignalState( stateAfterWait.CWait() - 1, stateAfterWait.Irksem() ) ) )
                            {
                                //  unreference the kernel semaphore

                                g_ksempoolGlobal.Unreference( irksemAlloc );

                                //  we did not successfully wait for the signal

                                return fFalse;
                            }
                        }
                    }
                }
            }
        }

        //  there are waiters

        else
        {
            OSSYNCAssert( stateCur.FWait() );

            //  reference the kernel semaphore already in use

            g_ksempoolGlobal.Reference( stateCur.Irksem() );

            //  we successfully added ourself as another waiter

            if ( State().FChange( stateCur, CAutoResetSignalState( stateCur.CWait() + 1, stateCur.Irksem() ) ) )
            {
                //  if we allocated a kernel semaphore, unreference it

                if ( irksemAlloc != CKernelSemaphorePool::irksemNil )
                {
                    g_ksempoolGlobal.Unreference( irksemAlloc );
                }

                //  wait for signal to be set

                State().StartWait();
                const BOOL fCompleted = g_ksempoolGlobal.Ksem( stateCur.Irksem(), this ).FAcquire( cmsecTimeout );
                State().StopWait();

                //  our wait completed

                if ( fCompleted )
                {
                    //  unreference the kernel semaphore

                    g_ksempoolGlobal.Unreference( stateCur.Irksem() );

                    //  we successfully waited for the signal

                    State().SetAcquire();
                    return fTrue;
                }

                //  our wait timed out

                else
                {
                    //  try forever until we successfully change the state of the signal

                    OSSYNC_INNER_FOREVER
                    {
                        //  read the current state of the signal

                        const CAutoResetSignalState stateAfterWait = (CAutoResetSignalState&) State();

                        //  there are no waiters or the kernel semaphore currently
                        //  in the signal is not the same as the one we waited on

                        if ( stateAfterWait.FNoWait() || stateAfterWait.Irksem() != stateCur.Irksem() )
                        {
                            //  the kernel semaphore we waited on is no longer in
                            //  use, so another context released it.  this means that
                            //  there is a count on the kernel semaphore that we must
                            //  absorb, so we will

                            //  NOTE:  we could end up blocking because the releasing
                            //  context may not have released the semaphore yet

                            g_ksempoolGlobal.Ksem( stateCur.Irksem(), this ).Acquire();

                            //  unreference the kernel semaphore

                            g_ksempoolGlobal.Unreference( stateCur.Irksem() );

                            //  we successfully waited for the signal

                            return fTrue;
                        }

                        //  there is one waiter and the kernel semaphore currently
                        //  in the signal is the same as the one we waited on

                        else if ( stateAfterWait.CWait() == 1 )
                        {
                            OSSYNCAssert( stateAfterWait.FWait() );
                            OSSYNCAssert( stateAfterWait.Irksem() == stateCur.Irksem() );

                            //  we successfully changed the signal to the reset with
                            //  no waiters state

                            if ( State().FChange( stateAfterWait, CAutoResetSignalState( 0 ) ) )
                            {
                                //  unreference the kernel semaphore

                                g_ksempoolGlobal.Unreference( stateCur.Irksem() );

                                //  we did not successfully wait for the signal

                                return fFalse;
                            }
                        }

                        //  there are many waiters and the kernel semaphore currently
                        //  in the signal is the same as the one we waited on

                        else
                        {
                            OSSYNCAssert( stateAfterWait.CWait() > 1 );
                            OSSYNCAssert( stateAfterWait.FWait() );
                            OSSYNCAssert( stateAfterWait.Irksem() == stateCur.Irksem() );

                            //  we successfully reduced the number of waiters on the
                            //  signal by one

                            if ( State().FChange( stateAfterWait, CAutoResetSignalState( stateAfterWait.CWait() - 1, stateAfterWait.Irksem() ) ) )
                            {
                                //  unreference the kernel semaphore

                                g_ksempoolGlobal.Unreference( stateCur.Irksem() );

                                //  we did not successfully wait for the signal

                                return fFalse;
                            }
                        }
                    }
                }
            }

            //  unreference the kernel semaphore

            g_ksempoolGlobal.Unreference( stateCur.Irksem() );
        }
    }
}

//  sets the signal, releasing up to one waiter.  if a waiter is released, then
//  the signal will be reset.  if a waiter is not released, the signal will
//  remain set

void CAutoResetSignal::_Set()
{
    //  try forever until we successfully change the state of the signal

    OSSYNC_FOREVER
    {
        //  read the current state of the signal

        const CAutoResetSignalState stateCur = (CAutoResetSignalState&) State();

        //  there are no waiters

        if ( stateCur.FNoWait() )
        {
            //  we successfully changed the signal state from reset with no
            //  waiters to set or from set to set (a nop)

            if ( State().FSimpleSet() )
            {
                //  we're done

                return;
            }
        }

        //  there are waiters

        else
        {
            OSSYNCAssert( stateCur.FWait() );

            //  there is only one waiter

            if ( stateCur.CWait() == 1 )
            {
                //  we successfully changed the signal to the reset with no waiters state

                if ( State().FChange( stateCur, CAutoResetSignalState( 0 ) ) )
                {
                    //  release the lone waiter

                    g_ksempoolGlobal.Ksem( stateCur.Irksem(), this ).Release();

                    //  we're done

                    return;
                }
            }

            //  there is more than one waiter

            else
            {
                OSSYNCAssert( stateCur.CWait() > 1 );

                //  we successfully reduced the number of waiters on the signal by one

                if ( State().FChange( stateCur, CAutoResetSignalState( stateCur.CWait() - 1, stateCur.Irksem() ) ) )
                {
                    //  release one waiter

                    g_ksempoolGlobal.Ksem( stateCur.Irksem(), this ).Release();

                    //  we're done

                    return;
                }
            }
        }
    }
}


//  Manual-Reset Signal

//  ctor

CManualResetSignal::CManualResetSignal( const CSyncBasicInfo& sbi )
    :   CEnhancedStateContainer< CManualResetSignalState, CSyncStateInitNull, CManualResetSignalInfo, CSyncBasicInfo >( syncstateNull, sbi )
{
    //  further init of CSyncBasicInfo

    State().SetTypeName( "CManualResetSignal" );
    State().SetInstance( (CSyncObject*)this );
}

//  dtor

CManualResetSignal::~CManualResetSignal()
{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

    //  dump performance data

    OSSyncStatsDump(    State().SzTypeName(),
                        State().SzInstanceName(),
                        State().Instance(),
                        (DWORD)-1,
                        State().CWaitTotal(),
                        State().CsecWaitElapsed(),
                        State().CAcquireTotal(),
                        State().CContendTotal(),
                        0,
                        0 );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
}

//  waits for the signal to be set, returning fFalse if unsuccessful in the time
//  permitted.  Infinite and Test-Only timeouts are supported.

const BOOL CManualResetSignal::_FWait( const INT cmsecTimeout )
{
    //  if we spin, we will spin for the full amount recommended by the OS

    INT cSpin = g_cSpinMax;

    //  we start with no kernel semaphore allocated

    CKernelSemaphorePool::IRKSEM irksemAlloc = CKernelSemaphorePool::irksemNil;

    //  try forever until we successfully change the state of the signal

    OSSYNC_FOREVER
    {
        //  read the current state of the signal

        const CManualResetSignalState stateCur = (CManualResetSignalState&) State();

        //  the signal is set

        if ( stateCur.FNoWaitAndSet() )
        {
            //  if we allocated a kernel semaphore, release it

            if ( irksemAlloc != CKernelSemaphorePool::irksemNil )
            {
                g_ksempoolGlobal.Unreference( irksemAlloc );
            }

            //  we successfully waited for the signal

            State().SetAcquire();
            return fTrue;
        }

        //  the signal is not set and we still have spins left

        else if ( cSpin )
        {
            //  spin once and try again

            cSpin--;
            continue;
        }

        //  the signal is not set and there are no waiters

        else if ( stateCur.FNoWaitAndNotSet() )
        {
            //  allocate and reference a kernel semaphore if we haven't already

            if ( irksemAlloc == CKernelSemaphorePool::irksemNil )
            {
                irksemAlloc = g_ksempoolGlobal.Allocate( this );
            }

            //  we successfully installed ourselves as the first waiter

            if ( State().FChange( stateCur, CManualResetSignalState( 1, irksemAlloc ) ) )
            {
                //  wait for signal to be set

                State().StartWait();
                const BOOL fCompleted = g_ksempoolGlobal.Ksem( irksemAlloc, this ).FAcquire( cmsecTimeout );
                State().StopWait();

                //  our wait completed

                if ( fCompleted )
                {
                    //  unreference the kernel semaphore

                    g_ksempoolGlobal.Unreference( irksemAlloc );

                    //  we successfully waited for the signal

                    State().SetAcquire();
                    return fTrue;
                }

                //  our wait timed out

                else
                {
                    //  try forever until we successfully change the state of the signal

                    OSSYNC_INNER_FOREVER
                    {
                        //  read the current state of the signal

                        const CManualResetSignalState stateAfterWait = (CManualResetSignalState&) State();

                        //  there are no waiters or the kernel semaphore currently
                        //  in the signal is not the same as the one we allocated

                        if ( stateAfterWait.FNoWait() || stateAfterWait.Irksem() != irksemAlloc )
                        {
                            //  the kernel semaphore we allocated is no longer in
                            //  use, so another context released it.  this means that
                            //  there is a count on the kernel semaphore that we must
                            //  absorb, so we will

                            //  NOTE:  we could end up blocking because the releasing
                            //  context may not have released the semaphore yet

                            g_ksempoolGlobal.Ksem( irksemAlloc, this ).Acquire();

                            //  unreference the kernel semaphore

                            g_ksempoolGlobal.Unreference( irksemAlloc );

                            //  we successfully waited for the signal

                            return fTrue;
                        }

                        //  there is one waiter and the kernel semaphore currently
                        //  in the signal is the same as the one we allocated

                        else if ( stateAfterWait.CWait() == 1 )
                        {
                            OSSYNCAssert( stateAfterWait.FWait() );
                            OSSYNCAssert( stateAfterWait.Irksem() == irksemAlloc );

                            //  we successfully changed the signal to the reset with
                            //  no waiters state

                            if ( State().FChange( stateAfterWait, CManualResetSignalState( 0 ) ) )
                            {
                                //  unreference the kernel semaphore

                                g_ksempoolGlobal.Unreference( irksemAlloc );

                                //  we did not successfully wait for the signal

                                return fFalse;
                            }
                        }

                        //  there are many waiters and the kernel semaphore currently
                        //  in the signal is the same as the one we allocated

                        else
                        {
                            OSSYNCAssert( stateAfterWait.CWait() > 1 );
                            OSSYNCAssert( stateAfterWait.FWait() );
                            OSSYNCAssert( stateAfterWait.Irksem() == irksemAlloc );

                            //  we successfully reduced the number of waiters on the
                            //  signal by one

                            if ( State().FChange( stateAfterWait, CManualResetSignalState( stateAfterWait.CWait() - 1, stateAfterWait.Irksem() ) ) )
                            {
                                //  unreference the kernel semaphore

                                g_ksempoolGlobal.Unreference( irksemAlloc );

                                //  we did not successfully wait for the signal

                                return fFalse;
                            }
                        }
                    }
                }
            }
        }

        //  there are waiters

        else
        {
            OSSYNCAssert( stateCur.FWait() );

            //  reference the kernel semaphore already in use

            g_ksempoolGlobal.Reference( stateCur.Irksem() );

            //  we successfully added ourself as another waiter

            if ( State().FChange( stateCur, CManualResetSignalState( stateCur.CWait() + 1, stateCur.Irksem() ) ) )
            {
                //  if we allocated a kernel semaphore, unreference it

                if ( irksemAlloc != CKernelSemaphorePool::irksemNil )
                {
                    g_ksempoolGlobal.Unreference( irksemAlloc );
                }

                //  wait for signal to be set

                State().StartWait();
                const BOOL fCompleted = g_ksempoolGlobal.Ksem( stateCur.Irksem(), this ).FAcquire( cmsecTimeout );
                State().StopWait();

                //  our wait completed

                if ( fCompleted )
                {
                    //  unreference the kernel semaphore

                    g_ksempoolGlobal.Unreference( stateCur.Irksem() );

                    //  we successfully waited for the signal

                    State().SetAcquire();
                    return fTrue;
                }

                //  our wait timed out

                else
                {
                    //  try forever until we successfully change the state of the signal

                    OSSYNC_INNER_FOREVER
                    {
                        //  read the current state of the signal

                        const CManualResetSignalState stateAfterWait = (CManualResetSignalState&) State();

                        //  there are no waiters or the kernel semaphore currently
                        //  in the signal is not the same as the one we waited on

                        if ( stateAfterWait.FNoWait() || stateAfterWait.Irksem() != stateCur.Irksem() )
                        {
                            //  the kernel semaphore we waited on is no longer in
                            //  use, so another context released it.  this means that
                            //  there is a count on the kernel semaphore that we must
                            //  absorb, so we will

                            //  NOTE:  we could end up blocking because the releasing
                            //  context may not have released the semaphore yet

                            g_ksempoolGlobal.Ksem( stateCur.Irksem(), this ).Acquire();

                            //  unreference the kernel semaphore

                            g_ksempoolGlobal.Unreference( stateCur.Irksem() );

                            //  we successfully waited for the signal

                            return fTrue;
                        }

                        //  there is one waiter and the kernel semaphore currently
                        //  in the signal is the same as the one we waited on

                        else if ( stateAfterWait.CWait() == 1 )
                        {
                            OSSYNCAssert( stateAfterWait.FWait() );
                            OSSYNCAssert( stateAfterWait.Irksem() == stateCur.Irksem() );

                            //  we successfully changed the signal to the reset with
                            //  no waiters state

                            if ( State().FChange( stateAfterWait, CManualResetSignalState( 0 ) ) )
                            {
                                //  unreference the kernel semaphore

                                g_ksempoolGlobal.Unreference( stateCur.Irksem() );

                                //  we did not successfully wait for the signal

                                return fFalse;
                            }
                        }

                        //  there are many waiters and the kernel semaphore currently
                        //  in the signal is the same as the one we waited on

                        else
                        {
                            OSSYNCAssert( stateAfterWait.CWait() > 1 );
                            OSSYNCAssert( stateAfterWait.FWait() );
                            OSSYNCAssert( stateAfterWait.Irksem() == stateCur.Irksem() );

                            //  we successfully reduced the number of waiters on the
                            //  signal by one

                            if ( State().FChange( stateAfterWait, CManualResetSignalState( stateAfterWait.CWait() - 1, stateAfterWait.Irksem() ) ) )
                            {
                                //  unreference the kernel semaphore

                                g_ksempoolGlobal.Unreference( stateCur.Irksem() );

                                //  we did not successfully wait for the signal

                                return fFalse;
                            }
                        }
                    }
                }
            }

            //  unreference the kernel semaphore

            g_ksempoolGlobal.Unreference( stateCur.Irksem() );
        }
    }
}


//  Lock Object Basic Information

//  ctor

#ifdef SYNC_ENHANCED_STATE

CLockBasicInfo::CLockBasicInfo( const CSyncBasicInfo& sbi, const INT rank, const INT subrank )
    :   CSyncBasicInfo( sbi )
{
#ifdef SYNC_DEADLOCK_DETECTION

    m_rank          = rank;
    m_subrank       = subrank;

#endif  //  SYNC_DEADLOCK_DETECTION
}

//  dtor

CLockBasicInfo::~CLockBasicInfo()
{
}

#endif  //  SYNC_ENHANCED_STATE


//  Lock Object Performance:  Hold

//  ctor

CLockPerfHold::CLockPerfHold()
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    m_cHold = 0;
    m_qwHRTHoldElapsed = 0;

#endif  //  SYNC_ANALYZE_PERFORMANCE
}

//  dtor

CLockPerfHold::~CLockPerfHold()
{
}


//  Lock Owner Record

//  ctor

COwner::COwner()
{
#ifdef SYNC_DEADLOCK_DETECTION

    m_pclsOwner         = NULL;
    m_pownerContextNext = NULL;
    m_plddiOwned        = NULL;
    m_pownerLockNext    = NULL;
    m_group             = 0;

#endif  //  SYNC_DEADLOCK_DETECTION
}

//  dtor

COwner::~COwner()
{
}


//  Lock Object Deadlock Detection Information

//  ctor/dtor

#ifdef SYNC_DEADLOCK_DETECTION

CLockDeadlockDetectionInfo::CLockDeadlockDetectionInfo( const CLockBasicInfo& lbi )
    :   m_semOwnerList( CSyncBasicInfo( "CLockDeadlockDetectionInfo::m_semOwnerList" ) )
{
    m_plbiParent = &lbi;
    m_semOwnerList.Release();
}

CLockDeadlockDetectionInfo::~CLockDeadlockDetectionInfo()
{
    if ( NULL != m_ownerHead.m_pclsOwner )
    {
        OSSYNCAssertSz( g_fSyncProcessAbort, "There is still an owner for the lock of this object that is being deleted." );
        
        //  We don't want to keep references to this object in the deadlock detection data structures.
        
        if ( FOwner() )
        {
            RemoveAsOwner();
        }
    }
}

#else  //  !SYNC_DEADLOCK_DETECTION

CLockDeadlockDetectionInfo::CLockDeadlockDetectionInfo( const CLockBasicInfo& lbi )
{
}

CLockDeadlockDetectionInfo::~CLockDeadlockDetectionInfo()
{
}

#endif  //  SYNC_DEADLOCK_DETECTION


//  Critical Section (non-nestable) State

//  ctor

CCriticalSectionState::CCriticalSectionState( const CSyncBasicInfo& sbi )
    :   m_sem( sbi )
{
}

//  dtor

CCriticalSectionState::~CCriticalSectionState()
{
}


//  Critical Section (non-nestable)

//  ctor

CCriticalSection::CCriticalSection( const CLockBasicInfo& lbi )
    :   CEnhancedStateContainer< CCriticalSectionState, CSyncBasicInfo, CCriticalSectionInfo, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
{
    //  further init of CSyncBasicInfo

    State().SetTypeName( "CCriticalSection" );
    State().SetInstance( (CSyncObject*)this );

    //  release semaphore

    State().Semaphore().Release();
}

//  dtor

CCriticalSection::~CCriticalSection()
{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

    //  dump performance data

    OSSyncStatsDump(    State().SzTypeName(),
                        State().SzInstanceName(),
                        State().Instance(),
                        (DWORD)-1,
                        0,
                        0,
                        0,
                        0,
                        State().CHoldTotal(),
                        State().CsecHoldElapsed() );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
}


//  Nestable Critical Section State

//  ctor

CNestableCriticalSectionState::CNestableCriticalSectionState( const CSyncBasicInfo& sbi )
    :   m_sem( sbi ),
        m_pclsOwner( 0 ),
        m_cEntry( 0 )
{
}

//  dtor

CNestableCriticalSectionState::~CNestableCriticalSectionState()
{
}


//  Nestable Critical Section

//  ctor

CNestableCriticalSection::CNestableCriticalSection( const CLockBasicInfo& lbi )
    :   CEnhancedStateContainer< CNestableCriticalSectionState, CSyncBasicInfo, CNestableCriticalSectionInfo, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
{
    //  further init of CSyncBasicInfo

    State().SetTypeName( "CNestableCriticalSection" );
    State().SetInstance( (CSyncObject*)this );

    //  release semaphore

    State().Semaphore().Release();
}

//  dtor

CNestableCriticalSection::~CNestableCriticalSection()
{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

    //  dump performance data

    OSSyncStatsDump(    State().SzTypeName(),
                        State().SzInstanceName(),
                        State().Instance(),
                        (DWORD)-1,
                        0,
                        0,
                        0,
                        0,
                        State().CHoldTotal(),
                        State().CsecHoldElapsed() );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
}


//  Gate State

//  ctor

CGateState::CGateState( const INT cWait, const INT irksem )
{
    //  validate IN args

    OSSYNCAssert( cWait >= 0 );
    OSSYNCAssert( cWait <= 0x7FFF );
    OSSYNCAssert( irksem >= 0 );
    OSSYNCAssert( irksem <= 0xFFFE );

    //  set waiter count

    m_cWait = (USHORT) cWait;

    //  set semaphore

    m_irksem = (USHORT) irksem;
}


//  Gate

//  ctor

CGate::CGate( const CSyncBasicInfo& sbi )
    :   CEnhancedStateContainer< CGateState, CSyncStateInitNull, CGateInfo, CSyncBasicInfo >( syncstateNull, sbi )
{
    //  further init of CSyncBasicInfo

    State().SetTypeName( "CGate" );
    State().SetInstance( (CSyncObject*)this );
}

//  dtor

CGate::~CGate()
{
    //  no one should be waiting

    // if a thread waiting is killed, the CWait() can be != 0
    // OSSYNCAssert( State().CWait() == 0 );

    //OSSYNCAssert( State().Irksem() == CKernelSemaphorePool::irksemNil );

#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

    //  dump performance data

    OSSyncStatsDump(    State().SzTypeName(),
                        State().SzInstanceName(),
                        State().Instance(),
                        (DWORD)-1,
                        State().CWaitTotal(),
                        State().CsecWaitElapsed(),
                        0,
                        0,
                        0,
                        0 );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
}

//  waits forever on the gate until released by someone else.  this function
//  expects to be called while in the specified critical section.  when the
//  function returns, the caller will NOT be in the critical section

void CGate::Wait( CCriticalSection& crit )
{
    //  we must be in the specified critical section

    OSSYNCAssert( crit.FOwner() );

    //  there can not be too many waiters on the gate

    OSSYNCAssert( State().CWait() < 0x7FFF );

    //  add ourselves as a waiter

    const INT cWait = State().CWait() + 1;
    State().SetWaitCount( cWait );

    //  we are the first waiter

    CKernelSemaphorePool::IRKSEM irksem;
#ifdef DEBUG
    irksem = CKernelSemaphorePool::irksemNil;
#endif  //  DEBUG
    if ( cWait == 1 )
    {
        //  allocate a semaphore for the gate and remember it before leaving
        //  the critical section

        OSSYNCAssert( State().Irksem() == CKernelSemaphorePool::irksemNil );
        irksem = g_ksempoolGlobal.Allocate( this );
        State().SetIrksem( irksem );
    }

    //  we are not the first waiter

    else
    {
        //  reference the semaphore already in the gate and remember it before
        //  leaving the critical section

        OSSYNCAssert( State().Irksem() != CKernelSemaphorePool::irksemNil );
        irksem = State().Irksem();
        g_ksempoolGlobal.Reference( irksem );
    }
    OSSYNCAssert( irksem != CKernelSemaphorePool::irksemNil );

    //  leave critical section, never to return

    crit.Leave();

    //  wait to be released

    State().StartWait();
    g_ksempoolGlobal.Ksem( irksem, this ).Acquire();
    State().StopWait();

    //  unreference the semaphore

    g_ksempoolGlobal.Unreference( irksem );
}

//  releases the specified number of waiters from the gate.  this function
//  expects to be called while in the specified critical section.  when the
//  function returns, the caller will NOT be in the critical section
//
//  NOTE:  it is illegal to release more waiters than are waiting on the gate
//         and it is also illegal to release less than one waiter

void CGate::Release( CCriticalSection& crit, const INT cToRelease )
{

    //  we must be in the specified critical section

    OSSYNCAssert( crit.FOwner() );

    //  you must release at least one waiter

    OSSYNCAssert( cToRelease > 0 );

    //  we cannot release more waiters than are waiting on the gate

    OSSYNCAssert( cToRelease <= State().CWait() );

    //  reduce the waiter count

    State().SetWaitCount( State().CWait() - cToRelease );

    //  remember semaphore to release before leaving the critical section

    const CKernelSemaphorePool::IRKSEM irksem = State().Irksem();

#ifdef DEBUG

    //  we released all the waiters

    if ( State().CWait() == 0 )
    {
        //  set the semaphore to nil

        State().SetIrksem( CKernelSemaphorePool::irksemNil );
    }

#endif  //  DEBUG

    //  leave critical section, never to return

    crit.Leave();

    //  release the specified number of waiters

    g_ksempoolGlobal.Ksem( irksem, this ).Release( cToRelease );
}

//  releases all the waiters from the gate.  this function expects to be called while 
//  in the specified critical section.  when the function returns, the caller will NOT be 
//  in the critical section

void CGate::ReleaseAll( CCriticalSection& crit )
{
    //  we must be in the specified critical section

    OSSYNCAssert( crit.FOwner() );

    //  retrieve the number of waiters that we need to release

    const INT cWaitersToRelease = State().CWait();

    //  consumer must not call this API, unless there is at least one waiter to release

    OSSYNCAssert( cWaitersToRelease > 0 );

    //  cToRelease should still be the same as State().CWait() a moment later

    OSSYNCAssert( State().CWait() == cWaitersToRelease );

    //  finally release all the waiters (and releases the crit-sec)

    Release( crit, cWaitersToRelease );
}

//  releases the specified number of waiters from the gate.  this function
//  expects to be called while in the specified critical section.  it is
//  guaranteed that the caller will remain in the critical section at all times
//
//  NOTE:  it is illegal to release more waiters than are waiting on the gate
//         and it is also illegal to release less than one waiter

void CGate::ReleaseAndHold( CCriticalSection& crit, const INT cToRelease )
{
    //  we must be in the specified critical section

    OSSYNCAssert( crit.FOwner() );

    //  you must release at least one waiter

    OSSYNCAssert( cToRelease > 0 );

    //  we cannot release more waiters than are waiting on the gate

    OSSYNCAssert( cToRelease <= State().CWait() );

    //  reduce the waiter count

    State().SetWaitCount( State().CWait() - cToRelease );

    //  remember semaphore to release before leaving the critical section

    const CKernelSemaphorePool::IRKSEM irksem = State().Irksem();

#ifdef DEBUG

    //  we released all the waiters

    if ( State().CWait() == 0 )
    {
        //  set the semaphore to nil

        State().SetIrksem( CKernelSemaphorePool::irksemNil );
    }

#endif  //  DEBUG

    //  release the specified number of waiters

    g_ksempoolGlobal.Ksem( irksem, this ).Release( cToRelease );
}


//  Null Lock Object State Initializer

const CLockStateInitNull lockstateNull;


//  Binary Lock State

//  ctor

CBinaryLockState::CBinaryLockState( const CSyncBasicInfo& sbi )
    :   m_cw( 0 ),
        m_cOwner( 0 ),
        m_sem1( sbi ),
        m_sem2( sbi )
{
}

//  dtor

CBinaryLockState::~CBinaryLockState()
{
}


//  Binary Lock

//  ctor

CBinaryLock::CBinaryLock( const CLockBasicInfo& lbi )
    :   CEnhancedStateContainer< CBinaryLockState, CSyncBasicInfo, CBinaryLockInfo, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
{
    //  further init of CSyncBasicInfo

    State().SetTypeName( "CBinaryLock" );
    State().SetInstance( (CSyncObject*)this );
}

//  dtor

CBinaryLock::~CBinaryLock()
{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

    //  dump performance data

    for ( INT iGroup = 0; iGroup < 2; iGroup++ )
    {
        OSSyncStatsDump(    State().SzTypeName(),
                            State().SzInstanceName(),
                            State().Instance(),
                            iGroup,
                            State().CWaitTotal( iGroup ),
                            State().CsecWaitElapsed( iGroup ),
                            State().CAcquireTotal( iGroup ),
                            State().CContendTotal( iGroup ),
                            State().CHoldTotal( iGroup ),
                            State().CsecHoldElapsed( iGroup ) );
    }

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
}

//  maps an arbitrary combination of zero and non-zero components into a
//  valid state number of the invalid state number (-1)

const INT mpindexstate[16] =
{
     0, -1, -1, -1,
    -1, -1,  1, -1,
    -1,  2, -1,  3,
    -1, -1,  4,  5,
};

//  returns the state number of the specified control word or -1 if it is not
//  a legal state

INT CBinaryLock::_StateFromControlWord( const ControlWord cw )
{
    //  convert the control word into a state index

    INT index = 0;
    index = index | ( ( cw & 0x80000000 ) ? 8 : 0 );
    index = index | ( ( cw & 0x7FFF0000 ) ? 4 : 0 );
    index = index | ( ( cw & 0x00008000 ) ? 2 : 0 );
    index = index | ( ( cw & 0x00007FFF ) ? 1 : 0 );

    //  convert the state index into a state number

    const INT state = mpindexstate[index];

    //  return the computed state number

    return state;
}

//  state transition reachability matrix (starting state is the major axis)
//
//  each entry contains bits representing valid reasons for making the
//  transition (made by oring together the valid TransitionReasons)

#define NO  CBinaryLock::trIllegal
#define E1  CBinaryLock::trEnter1
#define L1  CBinaryLock::trLeave1
#define E2  CBinaryLock::trEnter2
#define L2  CBinaryLock::trLeave2

const DWORD mpstatestatetrmask[6][6] =
{
        { NO, E2, E1, NO, NO, NO, },
        { L2, E2 | L2, NO, E1, NO, NO, },
        { L1, NO, E1 | L1, NO, E2, NO, },
        { NO, NO, L2, E1 | L2, NO, E2, },
        { NO, L1, NO, NO, L1 | E2, E1, },
        { NO, NO, NO, L1, L2, E1 | L1 | E2 | L2, },
};

#undef NO
#undef E1
#undef L1
#undef E2
#undef L2

//  returns fTrue if the specified control word is in a legal state

BOOL CBinaryLock::_FValidStateTransition( const ControlWord cwBI, const ControlWord cwAI, const TransitionReason tr )
{
    //  convert the specified control words into state numbers

    const INT stateBI = _StateFromControlWord( cwBI );
    const INT stateAI = _StateFromControlWord( cwAI );

    //  if either state is invalid, the transition is invalid

    if ( stateBI < 0 || stateAI < 0 )
    {
        return fFalse;
    }

    //  verify that cOOW2 and cOOW1 only change by +1, 0, -1, or go to 0

    const LONG dcOOW2 = ( ( cwAI & 0x7FFF0000 ) >> 16 ) - ( ( cwBI & 0x7FFF0000 ) >> 16 );
    if ( ( dcOOW2 < -1 || dcOOW2 > 1 ) && ( cwAI & 0x7FFF0000 ) != 0 )
    {
        return fFalse;
    }

    const LONG dcOOW1 = ( cwAI & 0x00007FFF ) - ( cwBI & 0x00007FFF );
    if ( ( dcOOW1 < -1 || dcOOW1 > 1 ) && ( cwAI & 0x00007FFF ) != 0 )
    {
        return fFalse;
    }

    //  return the reachability of stateAI from stateBI

    OSSYNCAssert( tr == trEnter1 || tr == trLeave1 || tr == trEnter2 || tr == trLeave2 );
    return ( mpstatestatetrmask[stateBI][stateAI] & tr ) != 0;
}

//  wait for ownership of the lock as a member of Group 1

void CBinaryLock::_Enter1( const ControlWord cwBIOld )
{
    //  we just jumped from state 1 to state 3

    if ( ( cwBIOld & 0x80008000 ) == 0x00008000 )
    {
        //  update the quiesced owner count with the owner count that we displaced from
        //  the control word, possibly releasing waiters.  we update the count as if we
        //  were a member of Group 2 as members of Group 1 can be released

        _UpdateQuiescedOwnerCountAsGroup2( ( cwBIOld & 0x7FFF0000 ) >> 16 );
    }

    //  wait for ownership of the lock on our semaphore

    State().AddAsWaiter( 0 );
    State().StartWait( 0 );

    State().m_sem1.Acquire();

    State().StopWait( 0 );
    State().RemoveAsWaiter( 0 );
}

//  wait for ownership of the lock as a member of Group 2

void CBinaryLock::_Enter2( const ControlWord cwBIOld )
{
    //  we just jumped from state 2 to state 4

    if ( ( cwBIOld & 0x80008000 ) == 0x80000000 )
    {
        //  update the quiesced owner count with the owner count that we displaced from
        //  the control word, possibly releasing waiters.  we update the count as if we
        //  were a member of Group 1 as members of Group 2 can be released

        _UpdateQuiescedOwnerCountAsGroup1( cwBIOld & 0x00007FFF );
    }

    //  wait for ownership of the lock on our semaphore

    State().AddAsWaiter( 1 );
    State().StartWait( 1 );

    State().m_sem2.Acquire();

    State().StopWait( 1 );
    State().RemoveAsWaiter( 1 );
}

//  updates the quiesced owner count as a member of Group 1

void CBinaryLock::_UpdateQuiescedOwnerCountAsGroup1( const DWORD cOwnerDelta )
{
    //  update the quiesced owner count using the provided delta

    const DWORD cOwnerBI = AtomicExchangeAdd( (LONG*)&State().m_cOwner, cOwnerDelta );
    const DWORD cOwnerAI = cOwnerBI + cOwnerDelta;

    //  our update resulted in a zero quiesced owner count

    if ( !cOwnerAI )
    {
        //  we must release the waiters for Group 2 because we removed the last
        //  quiesced owner count

        //  try forever until we successfully change the lock state

        ControlWord cwBI;
        OSSYNC_FOREVER
        {
            //  read the current state of the control word as our expected before image

            const ControlWord cwBIExpected = State().m_cw;

            //  compute the after image of the control word such that we jump from state
            //  state 4 to state 1 or from state 5 to state 3, whichever is appropriate

            #pragma prefast(push)
            #pragma prefast(disable:6297, "Carrying of the high bit is intentional.")
            const ControlWord cwAI =    ControlWord( cwBIExpected &
                                        ( ( ( LONG_PTR( LONG( ( cwBIExpected + 0xFFFF7FFF ) << 16 ) ) >> 31 ) &
                                        0xFFFF0000 ) ^ 0x8000FFFF ) );
            #pragma prefast(pop)

            //  validate the transaction

            OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trLeave1 ) );

            //  attempt to perform the transacted state transition on the control word

            cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

            //  the transaction failed because another context changed the control word

            if ( cwBI != cwBIExpected )
            {
                //  try again

                continue;
            }

            //  the transaction succeeded

            else
            {
                //  we're done

                break;
            }
        }

        //  we just jumped from state 5 to state 3

        if ( cwBI & 0x00007FFF )
        {
            //  update the quiesced owner count with the owner count that we displaced
            //  from the control word
            //
            //  NOTE:  we do not have to worry about releasing any more waiters because
            //  either this context owns one of the owner counts or at least one context
            //  that owns an owner count are currently blocked on the semaphore

            const DWORD cOwnerDeltaT = ( cwBI & 0x7FFF0000 ) >> 16;
            AtomicExchangeAdd( (LONG*)&State().m_cOwner, cOwnerDeltaT );
        }

        //  release the waiters for Group 2 that we removed from the lock state

        State().m_sem2.Release( ( cwBI & 0x7FFF0000 ) >> 16 );
    }
}


//  updates the quiesced owner count as a member of Group 2

void CBinaryLock::_UpdateQuiescedOwnerCountAsGroup2( const DWORD cOwnerDelta )
{
    //  update the quiesced owner count using the provided delta

    const DWORD cOwnerBI = AtomicExchangeAdd( (LONG*)&State().m_cOwner, cOwnerDelta );
    const DWORD cOwnerAI = cOwnerBI + cOwnerDelta;

    //  our update resulted in a zero quiesced owner count

    if ( !cOwnerAI )
    {
        //  we must release the waiters for Group 1 because we removed the last
        //  quiesced owner count

        //  try forever until we successfully change the lock state

        ControlWord cwBI;
        OSSYNC_FOREVER
        {
            //  read the current state of the control word as our expected before image

            const ControlWord cwBIExpected = State().m_cw;

            //  compute the after image of the control word such that we jump from state
            //  state 3 to state 2 or from state 5 to state 4, whichever is appropriate

            const ControlWord cwAI =    ControlWord( cwBIExpected &
                                        ( ( ( LONG_PTR( LONG( cwBIExpected + 0x7FFF0000 ) ) >> 31 ) &
                                        0x0000FFFF ) ^ 0xFFFF8000 ) );

            //  validate the transaction

            OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trLeave2 ) );

            //  attempt to perform the transacted state transition on the control word

            cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

            //  the transaction failed because another context changed the control word

            if ( cwBI != cwBIExpected )
            {
                //  try again

                continue;
            }

            //  the transaction succeeded

            else
            {
                //  we're done

                break;
            }
        }

        //  we just jumped from state 5 to state 4

        if ( cwBI & 0x7FFF0000 )
        {
            //  update the quiesced owner count with the owner count that we displaced
            //  from the control word
            //
            //  NOTE:  we do not have to worry about releasing any more waiters because
            //  either this context owns one of the owner counts or at least one context
            //  that owns an owner count are currently blocked on the semaphore

            const DWORD cOwnerDeltaT = cwBI & 0x00007FFF;
            AtomicExchangeAdd( (LONG*)&State().m_cOwner, cOwnerDeltaT );
        }

        //  release the waiters for Group 1 that we removed from the lock state

        State().m_sem1.Release( cwBI & 0x00007FFF );
    }
}


//  Reader / Writer Lock State

//  ctor

CReaderWriterLockState::CReaderWriterLockState( const CSyncBasicInfo& sbi )
    :   m_cw( 0 ),
        m_cOwner( 0 ),
        m_semWriter( sbi ),
        m_semReader( sbi )
{
}

//  dtor

CReaderWriterLockState::~CReaderWriterLockState()
{
}


//  Reader / Writer Lock


//  ctor

CReaderWriterLock::CReaderWriterLock( const CLockBasicInfo& lbi )
    :   CEnhancedStateContainer< CReaderWriterLockState, CSyncBasicInfo, CReaderWriterLockInfo, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
{
    //  further init of CSyncBasicInfo

    State().SetTypeName( "CReaderWriterLock" );
    State().SetInstance( (CSyncObject*)this );
}

//  dtor

CReaderWriterLock::~CReaderWriterLock()
{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

    //  dump performance data

    for ( INT iGroup = 0; iGroup < 2; iGroup++ )
    {
        OSSyncStatsDump(    State().SzTypeName(),
                            State().SzInstanceName(),
                            State().Instance(),
                            iGroup,
                            State().CWaitTotal( iGroup ),
                            State().CsecWaitElapsed( iGroup ),
                            State().CAcquireTotal( iGroup ),
                            State().CContendTotal( iGroup ),
                            State().CHoldTotal( iGroup ),
                            State().CsecHoldElapsed( iGroup ) );
    }

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
}

//  maps an arbitrary combination of zero and non-zero components into a
//  valid state number of the invalid state number (-1)

const INT mpindexstateRW[16] =
{
     0, -1, -1, -1,
    -1, -1,  1, -1,
    -1,  2, -1,  3,
    -1, -1,  4,  5,
};

//  returns the state number of the specified control word or -1 if it is not
//  a legal state

INT CReaderWriterLock::_StateFromControlWord( const ControlWord cw )
{
    //  convert the control word into a state index

    INT index = 0;
    index = index | ( ( cw & 0x80000000 ) ? 8 : 0 );
    index = index | ( ( cw & 0x7FFF0000 ) ? 4 : 0 );
    index = index | ( ( cw & 0x00008000 ) ? 2 : 0 );
    index = index | ( ( cw & 0x00007FFF ) ? 1 : 0 );

    //  convert the state index into a state number

    const INT state = mpindexstateRW[index];

    //  return the computed state number

    return state;
}

//  state transition reachability matrix (starting state is the major axis)
//
//  each entry contains bits representing valid reasons for making the
//  transition (made by oring together the valid TransitionReasons)

#define NO  CReaderWriterLock::trIllegal
#define EW  CReaderWriterLock::trEnterAsWriter
#define LW  CReaderWriterLock::trLeaveAsWriter
#define ER  CReaderWriterLock::trEnterAsReader
#define LR  CReaderWriterLock::trLeaveAsReader

const DWORD mpstatestatetrmaskRW[6][6] =
{
        { NO, ER, EW, NO, NO, NO, },
        { LR, ER | LR, NO, EW, NO, NO, },
        { LW, NO, EW | LW, NO, ER, ER, },
        { NO, NO, LR, EW | LR, NO, ER, },
        { NO, LW, NO, NO, ER, EW, },
        { NO, NO, NO, LW, LR, EW | ER | LR, },
};

#undef NO
#undef EW
#undef LW
#undef ER
#undef LR

//  returns fTrue if the specified control word is in a legal state

BOOL CReaderWriterLock::_FValidStateTransition( const ControlWord cwBI, const ControlWord cwAI, const TransitionReason tr )
{
    //  convert the specified control words into state numbers

    const INT stateBI = _StateFromControlWord( cwBI );
    const INT stateAI = _StateFromControlWord( cwAI );

    //  if either state is invalid, the transition is invalid

    if ( stateBI < 0 || stateAI < 0 )
    {
        return fFalse;
    }

    //  verify that cOOW2 and cOOW1 only change by +1, 0, -1, or cOOW2 can go to 0

    const LONG dcOOW2 = ( ( cwAI & 0x7FFF0000 ) >> 16 ) - ( ( cwBI & 0x7FFF0000 ) >> 16 );
    if ( ( dcOOW2 < -1 || dcOOW2 > 1 ) && ( cwAI & 0x7FFF0000 ) != 0 )
    {
        return fFalse;
    }

    const LONG dcOOW1 = ( cwAI & 0x00007FFF ) - ( cwBI & 0x00007FFF );
    if ( dcOOW1 < -1 || dcOOW1 > 1 )
    {
        return fFalse;
    }

    //  return the reachability of stateAI from stateBI

    OSSYNCAssert(   tr == trEnterAsWriter ||
            tr == trLeaveAsWriter ||
            tr == trEnterAsReader ||
            tr == trLeaveAsReader );
    return ( mpstatestatetrmaskRW[stateBI][stateAI] & tr ) != 0;
}

//  wait for ownership of the lock as a writer

void CReaderWriterLock::_EnterAsWriter( const ControlWord cwBIOld )
{
    //  we just jumped from state 1 to state 3

    if ( ( cwBIOld & 0x80008000 ) == 0x00008000 )
    {
        //  update the quiesced owner count with the owner count that we displaced from
        //  the control word, possibly releasing a waiter.  we update the count as if we
        //  were a reader as a writer can be released

        _UpdateQuiescedOwnerCountAsReader( ( cwBIOld & 0x7FFF0000 ) >> 16 );
    }

    //  wait for ownership of the lock on our semaphore

    State().AddAsWaiter( 0 );
    State().StartWait( 0 );

    State().m_semWriter.Acquire();

    State().StopWait( 0 );
    State().RemoveAsWaiter( 0 );
}

//  wait for ownership of the lock as a reader

void CReaderWriterLock::_EnterAsReader( const ControlWord cwBIOld )
{
    //  we just jumped from state 2 to state 4 or from state 2 to state 5

    if ( ( cwBIOld & 0x80008000 ) == 0x80000000 )
    {
        //  update the quiesced owner count with the owner count that we displaced from
        //  the control word, possibly releasing waiters.  we update the count as if we
        //  were a writer as readers can be released

        _UpdateQuiescedOwnerCountAsWriter( 0x00000001 );
    }

    //  wait for ownership of the lock on our semaphore

    State().AddAsWaiter( 1 );
    State().StartWait( 1 );

    State().m_semReader.Acquire();

    State().StopWait( 1 );
    State().RemoveAsWaiter( 1 );
}

//  updates the quiesced owner count as a writer

void CReaderWriterLock::_UpdateQuiescedOwnerCountAsWriter( const DWORD cOwnerDelta )
{
    //  update the quiesced owner count using the provided delta

    const DWORD cOwnerBI = AtomicExchangeAdd( (LONG*)&State().m_cOwner, cOwnerDelta );
    const DWORD cOwnerAI = cOwnerBI + cOwnerDelta;

    //  our update resulted in a zero quiesced owner count

    if ( !cOwnerAI )
    {
        //  we must release the waiting readers because we removed the last
        //  quiesced owner count

        //  try forever until we successfully change the lock state

        ControlWord cwBI;
        OSSYNC_FOREVER
        {
            //  read the current state of the control word as our expected before image

            const ControlWord cwBIExpected = State().m_cw;

            //  compute the after image of the control word such that we jump from state
            //  state 4 to state 1 or from state 5 to state 3, whichever is appropriate

            #pragma prefast(push)
            #pragma prefast(disable:6297, "Carrying of the high bit is intentional.")
            const ControlWord cwAI =    ControlWord( cwBIExpected &
                                        ( ( ( LONG_PTR( LONG( ( cwBIExpected + 0xFFFF7FFF ) << 16 ) ) >> 31 ) &
                                        0xFFFF0000 ) ^ 0x8000FFFF ) );
            #pragma prefast(pop)

            //  validate the transaction

            OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trLeaveAsWriter ) );

            //  attempt to perform the transacted state transition on the control word

            cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

            //  the transaction failed because another context changed the control word

            if ( cwBI != cwBIExpected )
            {
                //  try again

                continue;
            }

            //  the transaction succeeded

            else
            {
                //  we're done

                break;
            }
        }

        //  we just jumped from state 5 to state 3

        if ( cwBI & 0x00007FFF )
        {
            //  update the quiesced owner count with the owner count that we displaced
            //  from the control word
            //
            //  NOTE:  we do not have to worry about releasing any more waiters because
            //  either this context owns one of the owner counts or at least one context
            //  that owns an owner count are currently blocked on the semaphore

            const DWORD cOwnerDeltaT = ( cwBI & 0x7FFF0000 ) >> 16;
            AtomicExchangeAdd( (LONG*)&State().m_cOwner, cOwnerDeltaT );
        }

        //  release the waiting readers that we removed from the lock state

        State().m_semReader.Release( ( cwBI & 0x7FFF0000 ) >> 16 );
    }
}


//  updates the quiesced owner count as a reader

void CReaderWriterLock::_UpdateQuiescedOwnerCountAsReader( const DWORD cOwnerDelta )
{
    //  update the quiesced owner count using the provided delta

    const DWORD cOwnerBI = AtomicExchangeAdd( (LONG*)&State().m_cOwner, cOwnerDelta );
    const DWORD cOwnerAI = cOwnerBI + cOwnerDelta;

    //  our update resulted in a zero quiesced owner count

    if ( !cOwnerAI )
    {
        //  we must release a waiting writer because we removed the last
        //  quiesced owner count

        //  try forever until we successfully change the lock state

        ControlWord cwBI;
        OSSYNC_FOREVER
        {
            //  read the current state of the control word as our expected before image

            const ControlWord cwBIExpected = State().m_cw;

            //  compute the after image of the control word such that we jump from state
            //  state 3 to state 2, from state 5 to state 4, or from state 5 to state 5,
            //  whichever is appropriate

            const ControlWord cwAI =    cwBIExpected + ( ( cwBIExpected & 0x7FFF0000 ) ?
                                            0xFFFFFFFF :
                                            0xFFFF8000 );

            //  validate the transaction

            OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trLeaveAsReader ) );

            //  attempt to perform the transacted state transition on the control word

            cwBI = AtomicCompareExchange( (LONG*)&State().m_cw, cwBIExpected, cwAI );

            //  the transaction failed because another context changed the control word

            if ( cwBI != cwBIExpected )
            {
                //  try again

                continue;
            }

            //  the transaction succeeded

            else
            {
                //  we're done

                break;
            }
        }

        //  we just jumped from state 5 to state 4 or from state 5 to state 5

        if ( cwBI & 0x7FFF0000 )
        {
            //  update the quiesced owner count with the owner count that we displaced
            //  from the control word
            //
            //  NOTE:  we do not have to worry about releasing any more waiters because
            //  either this context owns one of the owner counts or at least one context
            //  that owns an owner count are currently blocked on the semaphore

            AtomicExchangeAdd( (LONG*)&State().m_cOwner, 1 );
        }

        //  release the waiting writer that we removed from the lock state

        State().m_semWriter.Release();
    }
}


//  S / X / W Latch State

//  ctor

CSXWLatchState::CSXWLatchState( const CSyncBasicInfo& sbi )
    :   m_cw( 0 ),
        m_cQS( 0 ),
        m_semS( sbi ),
        m_semX( sbi ),
        m_semW( sbi )
{
}

//  dtor

CSXWLatchState::~CSXWLatchState()
{
}


//  S / X / W Latch


//  ctor

CSXWLatch::CSXWLatch( const CLockBasicInfo& lbi )
    :   CEnhancedStateContainer< CSXWLatchState, CSyncBasicInfo, CSXWLatchInfo, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
{
    //  further init of CSyncBasicInfo

    State().SetTypeName( "CSXWLatch" );
    State().SetInstance( (CSyncObject*)this );
}

//  dtor

CSXWLatch::~CSXWLatch()
{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

    //  dump performance data

    for ( INT iGroup = 0; iGroup < 3; iGroup++ )
    {
        OSSyncStatsDump(    State().SzTypeName(),
                            State().SzInstanceName(),
                            State().Instance(),
                            iGroup,
                            State().CWaitTotal( iGroup ),
                            State().CsecWaitElapsed( iGroup ),
                            State().CAcquireTotal( iGroup ),
                            State().CContendTotal( iGroup ),
                            State().CHoldTotal( iGroup ),
                            State().CsecHoldElapsed( iGroup ) );
    }

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
}


}; // namespace OSSYNC


//////////////////////////////////////////////////
//  Everything below this line is OS dependent


//  We need some lowish level NT-level primitive APIs/structs, so we need to include
//  either nt.h,ntrtl.h,nturtl.h or winnt.h
//
// Windows headers are such a mess ... we can't include nt*.h after windows.h, this
// causes errors, but we can't include winnt.h before windows.h as this also causes
// errors.


#include <windows.h>

#ifndef ESENT
//
//  For non-NT builds, this gets NT-level primities (but no TEB)
//
#include <winnt.h>
#endif

#include <dbgeng.h>

// Until Exchange updates their dbgeng.h file:
#ifndef DEBUG_OUTCTL_AMBIENT_DML
#define DEBUG_OUTCTL_AMBIENT_DML       0xfffffffe
#endif

#ifndef DEBUG_OUTCTL_AMBIENT_TEXT
#define DEBUG_OUTCTL_AMBIENT_TEXT      0xffffffff
#endif

#include <math.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

namespace OSSYNC {


//  Global Synchronization Constants

//    wait time used for testing the state of the kernel object

const INT cmsecTest = 0;

//    wait time used for infinite wait on a kernel object

const INT cmsecInfinite = INFINITE;

//    maximum wait time on a kernel object before a deadlock is suspected

const INT cmsecDeadlock = 600000; // 10 min

//    wait time used for infinite wait on a kernel object without deadlock

const INT cmsecInfiniteNoDeadlock = INFINITE - 1;

//    cache line size
//
//      the following chart describes cache-line configurations for each supported
//      architecture.
//
//      cache line size         = read size (prefetch)
//      cache line sector size  = write size (flush)
//
//
//      Pocessor    Cache Line Size     Cache Line Sector Size
//
//      Pentium         16B                 16B
//      PPro            32B                 32B
//      PII             32B                 32B
//      PIII            32B                 32B
//      AXP             64B                 64B
//      Willamette      128B                64B
//      Merced          128B?               128B?
//
//      NOTE:   when changing this, you must fix all structures/classes whose definitions
//              are based on its present value
//              (e.g. the object has space rsvd as filler for the rest of the cache line)

const INT cbCacheLine = 32;


//  Thread Wait Notifications

static void OSSYNCAPI OSSyncThreadWaitBegin()
{
}

static PfnThreadWait g_pfnThreadWaitBegin = OSSyncThreadWaitBegin;

void OSSYNCAPI OSSyncOnThreadWaitBegin( const PfnThreadWait pfn )
{
    g_pfnThreadWaitBegin = pfn ? pfn : OSSyncThreadWaitBegin;
}

void OnThreadWaitBegin()
{
    g_pfnThreadWaitBegin();
}

static void OSSYNCAPI OSSyncThreadWaitEnd()
{
}

static PfnThreadWait g_pfnThreadWaitEnd = OSSyncThreadWaitEnd;

void OSSYNCAPI OSSyncOnThreadWaitEnd( const PfnThreadWait pfn )
{
    g_pfnThreadWaitEnd = pfn ? pfn : OSSyncThreadWaitEnd;
}

void OnThreadWaitEnd()
{
    g_pfnThreadWaitEnd();
}


//  Page Memory Allocation

//  reserves and commits a range of virtual addresses of the specifed size,
//  returning NULL if there is insufficient address space or backing store to
//  satisfy the request.  Note that the page reserve granularity applies to
//  this range

void* PvPageAlloc( const size_t cbSize, void* const pv )
{
    //  allocate address space and backing store of the specified size

    void* const pvRet = VirtualAlloc( pv, cbSize, MEM_COMMIT, PAGE_READWRITE );
    if ( !pvRet )
    {
        return pvRet;
    }
    OSSYNCAssert( !pv || pvRet == pv );

    return pvRet;
}

//  reserve a range of virtual addresses of the specified size, returning NULL
//  if there is insufficient address space to satisfy the request.  Note that
//  the page reserve granularity applies to this range

void* PvPageReserve( const size_t cbSize, void* const pv )
{
    //  allocate address space of the specified size

    void* const pvRet = VirtualAlloc( pv, cbSize, MEM_RESERVE, PAGE_READWRITE );
    if ( !pvRet )
    {
        return pvRet;
    }
    OSSYNCAssert( !pv || pvRet == pv );

    return pvRet;
}

//  free the reserved range of virtual addresses starting at the specified
//  address, freeing any backing store committed to this range

void PageFree( void* const pv )
{
    if ( pv )
    {
        //  free backing store and address space for the specified range

        BOOL fMemFreed = VirtualFree( pv, 0, MEM_RELEASE );
        OSSYNCAssert( fMemFreed );
    }
}

//  commit the specified range of virtual addresses, returning fFalse if there
//  is insufficient backing store to satisfy the request.  Note that the page
//  commit granularity applies to this range

BOOL FPageCommit( void* const pv, const size_t cb )
{
    //  verify input

    if ( !pv )
    {
        return fFalse;
    }

    //  commit the memory

    const BOOL fAllocOK = VirtualAlloc( pv, cb, MEM_COMMIT, PAGE_READWRITE ) != NULL;

    return fAllocOK;
}

//  decommit the specified range of virtual addresses, freeing any backing
//  store committed to this range.  Note that the page commit granularity
//  applies to this range

void PageDecommit( void* const pv, const size_t cb )
{
    //  verify input

    if ( !pv )
    {
        return;
    }

    //  free backing store for the specified range

#pragma prefast(suppress:6250, "We specifically are only decommitting, not releasing." )
    const BOOL fFreeOK = VirtualFree( pv, cb, MEM_DECOMMIT );
    OSSYNCAssert( fFreeOK );
}


//  Context Local Storage

//  Internal CLS structure

struct _CLS
{
    DWORD               dwContextId;    //  context ID
    HANDLE              hContext;       //  context handle

    _CLS*               pclsNext;       //  next CLS in global list
    _CLS**              ppclsNext;      //  pointer to the pointer to this CLS

    CLS                 cls;            //  external CLS structure
};

//  Global CLS List

CRITICAL_SECTION g_csClsSyncGlobal;
BOOL g_fcsClsSyncGlobalInit;
_CLS* g_pclsSyncGlobal;
_CLS* g_pclsSyncCleanGlobal;
DWORD g_cclsSyncGlobal;

//  Allocated CLS Entries

//NOTE: Cannot initialise this variable because the code that allocates
//      TLS and uses this variable to store the index executes before
//      CRTInit, which would subsequently re-initialise the variable
//      with the value specified here
//DWORD g_dwClsSyncIndex      = dwClsInvalid;
//DWORD g_dwClsProcIndex      = dwClsInvalid;
DWORD       g_dwClsSyncIndex;
DWORD       g_dwClsProcIndex;

const DWORD dwClsInvalid            = 0xFFFFFFFF;       //  this is the value returned by TlsAlloc() on error

const LONG  lOSSyncUnlocked         = 0x7fffffff;
const LONG  lOSSyncLocked           = 0x80000000;
const LONG  lOSSyncLockedForInit    = 0x80000001;
const LONG  lOSSyncLockedForTerm    = 0x80000000;

static BOOL FOSSyncIInit();
static void OSSyncITerm();
static void OSSyncIClsFree( _CLS* pcls );

//  registers the given CLS structure as the CLS for this context

BOOL FOSSyncIClsRegister( _CLS* pcls )
{
    BOOL    fAllocatedCls   = fFalse;

    //  we are the first to register CLS

    EnterCriticalSection( &g_csClsSyncGlobal );

    if ( NULL == g_pclsSyncGlobal )
    {
        //  allocate our CLS entries

        g_dwClsSyncIndex = TlsAlloc();
        if ( dwClsInvalid == g_dwClsSyncIndex )
        {
            LeaveCriticalSection( &g_csClsSyncGlobal );
            return fFalse;
        }

        g_dwClsProcIndex = TlsAlloc();
        if ( dwClsInvalid == g_dwClsProcIndex )
        {
            const BOOL  fTLSFreed   = TlsFree( g_dwClsSyncIndex );
#if !defined(_AMD64_)
            OSSYNCAssert( fTLSFreed );      //  leak the TLS entries if we fail
#else // !_AMD64_
            Unused( fTLSFreed );
#endif // _AMD64_
            g_dwClsSyncIndex = dwClsInvalid;

            LeaveCriticalSection( &g_csClsSyncGlobal );
            return fFalse;
        }

        fAllocatedCls = fTrue;
    }

    OSSYNCAssert( dwClsInvalid != g_dwClsSyncIndex );
    OSSYNCAssert( dwClsInvalid != g_dwClsProcIndex );

    //  save the pointer to the given CLS

    const BOOL  fTLSPointerSet  = TlsSetValue( g_dwClsSyncIndex, pcls );
    if ( !fTLSPointerSet )
    {
        if ( fAllocatedCls )
        {
            OSSYNCAssert( NULL == g_pclsSyncGlobal );

            const BOOL  fTLSFreed1  = TlsFree( g_dwClsSyncIndex );
            const BOOL  fTLSFreed2  = TlsFree( g_dwClsProcIndex );

#if !defined(_AMD64_)
            OSSYNCAssert( fTLSFreed1 );     //  leak the TLS entries if we fail
            OSSYNCAssert( fTLSFreed2 );
#else // !_AMD64_
            Unused( fTLSFreed1 );
            Unused( fTLSFreed2 );
#endif // _AMD64_

            g_dwClsSyncIndex = dwClsInvalid;
            g_dwClsProcIndex = dwClsInvalid;
        }

        LeaveCriticalSection( &g_csClsSyncGlobal );
        return fFalse;
    }

    //  add this CLS into the global list

    pcls->pclsNext = g_pclsSyncGlobal;
    if ( pcls->pclsNext )
    {
        pcls->pclsNext->ppclsNext = &pcls->pclsNext;
    }
    pcls->ppclsNext = &g_pclsSyncGlobal;
    g_pclsSyncGlobal = pcls;
    g_cclsSyncGlobal++;
    OSSYNCEnforceSz(    g_cclsSyncGlobal <= 32768,
                "Too many threads are attached to the Synchronization Library!" );

    //  try to cleanup two entries in the global CLS list

    for ( INT i = 0; i < 2; i++ )
    {
        //  we have a CLS to clean

        _CLS* pclsClean = g_pclsSyncCleanGlobal ? g_pclsSyncCleanGlobal : g_pclsSyncGlobal;

        if ( pclsClean )
        {
            //  set the next CLS to clean

            g_pclsSyncCleanGlobal = pclsClean->pclsNext;

            //  we can cleanup this CLS if the thread has exited

            DWORD dwExitCode;
            if (    pclsClean->hContext &&
                    GetExitCodeThread( pclsClean->hContext, &dwExitCode ) &&
                    dwExitCode != STILL_ACTIVE )
            {
                //  detach this CLS

                OSSyncIClsFree( pclsClean );
            }
        }
    }

    LeaveCriticalSection( &g_csClsSyncGlobal );
    return fTrue;
}

//  unregisters the given CLS structure as the CLS for this context

void OSSyncIClsUnregister( _CLS* pcls )
{
    //  there should be CLSs registered

    EnterCriticalSection( &g_csClsSyncGlobal );
    OSSYNCAssert( g_pclsSyncGlobal != NULL );

    //  make sure that the clean pointer is not pointing at this CLS

    if ( g_pclsSyncCleanGlobal == pcls )
    {
        g_pclsSyncCleanGlobal = pcls->pclsNext;
    }

    //  remove our CLS from the global CLS list

    if( pcls->pclsNext )
    {
        pcls->pclsNext->ppclsNext = pcls->ppclsNext;
    }
    *( pcls->ppclsNext ) = pcls->pclsNext;
    g_cclsSyncGlobal--;

    //  we are the last to unregister our CLS

    if ( g_pclsSyncGlobal == NULL )
    {
        //  deallocate CLS entries

        OSSYNCAssert( dwClsInvalid != g_dwClsSyncIndex );
        OSSYNCAssert( dwClsInvalid != g_dwClsProcIndex );

        const BOOL  fTLSFreed1  = TlsFree( g_dwClsSyncIndex );
        const BOOL  fTLSFreed2  = TlsFree( g_dwClsProcIndex );

#if !defined(_AMD64_)
        OSSYNCAssert( fTLSFreed1 );     //  leak the TLS entries if we fail
        OSSYNCAssert( fTLSFreed2 );
#else // !_AMD64_
        Unused( fTLSFreed1 );
        Unused( fTLSFreed2 );
#endif // _AMD64_

        g_dwClsSyncIndex = dwClsInvalid;
        g_dwClsProcIndex = dwClsInvalid;
    }

    LeaveCriticalSection( &g_csClsSyncGlobal );
}

//  attaches to the current context, returning fFalse on failure

static BOOL OSSYNCAPI FOSSyncIClsAlloc()
{
    //  we don't yet have any CLS

    _CLS* pcls = ( NULL != g_pclsSyncGlobal ?
                        reinterpret_cast<_CLS *>( TlsGetValue( g_dwClsSyncIndex ) ) :
                        NULL );
    if ( NULL == pcls )
    {
        //  allocate memory for this context's CLS

        pcls = (_CLS*) LocalAlloc( LMEM_ZEROINIT, sizeof( _CLS ) );

        if ( !( pcls ) )
        {
            return fFalse;
        }

        //  initialize internal CLS fields

        pcls->dwContextId   = GetCurrentThreadId();
        if ( DuplicateHandle(   GetCurrentProcess(),
                                GetCurrentThread(),
                                GetCurrentProcess(),
                                &pcls->hContext,
                                THREAD_QUERY_INFORMATION,
                                FALSE,
                                0 ) )
        {
            SetHandleInformation( pcls->hContext, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
        }

        //  register our CLS

        if ( !FOSSyncIClsRegister( pcls ) )
        {
            if ( pcls->hContext )
            {
                SetHandleInformation( pcls->hContext, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
                CloseHandle( pcls->hContext );
            }
            LocalFree( (void*) pcls );
            return fFalse;
        }

        //  set our initial processor number to be defer init

        OSSyncSetCurrentProcessor( -1 );
    }

    return fTrue;
}

//  detaches from the specified context

static void OSSyncIClsFree( _CLS* pcls )
{
#ifdef SYNC_DEADLOCK_DETECTION
    //  UNDONE: detect if we're trying to detach
    //  when this context is holding locks
#endif

    //  unregister our CLS

    OSSyncIClsUnregister( pcls );

    //  close our context handle

    if ( pcls->hContext )
    {
        SetHandleInformation( pcls->hContext, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        const BOOL  fCloseOK    = CloseHandle( pcls->hContext );
        OSSYNCAssert( fCloseOK );
    }

    //  free our CLS

    const BOOL  fFreedCLSOK = !LocalFree( pcls );
    OSSYNCAssert( fFreedCLSOK );
}


//  returns the pointer to the current context's local storage.  if the context
//  does not yet have CLS, allocate it.

CLS* const OSSYNCAPI Pcls()
{
    _CLS* pcls = ( NULL != g_pclsSyncGlobal ?
                        reinterpret_cast<_CLS *>( TlsGetValue( g_dwClsSyncIndex ) ) :
                        NULL );
    if ( NULL == pcls )
    {
        while ( !FOSSyncIClsAlloc() )
        {
            Sleep( 2 );
        }

        OSSYNCAssert( dwClsInvalid != g_dwClsSyncIndex );
        pcls = reinterpret_cast<_CLS *>( TlsGetValue( g_dwClsSyncIndex ) );
    }

    OSSYNCAssert( NULL != pcls );
    return &( pcls->cls );
}


//  Processor Information

//  returns the maximum number of processors this process can utilize

DWORD g_cProcessorMax;

INT OSSYNCAPI OSSyncGetProcessorCountMax()
{
    //  this returns the number of processors that the system had at the time
    //  that the sync library is initialized.  if the system adds more processors
    //  later then we will alias those onto existing processor "slots" to hide
    //  that from the higher levels of the code.  we will use these additional
    //  processors "inefficiently" until we next refresh this count
    //
    
    return g_cProcessorMax;
}

//  returns the current number of processors this process can utilize

DWORD g_cProcessor;

INT OSSYNCAPI OSSyncGetProcessorCount()
{
    //  no one uses this correctly right now so set to # system processors
    return OSSyncGetProcessorCountMax();
}

//  returns the processor number that the current context _MAY_ be executing on
//
//  NOTE:  the current context may change processors at any time

//  SKU specific get current processor implmentation
INT IprocOSSyncIGetCurrentProcessor();

INT OSSYNCAPI OSSyncGetCurrentProcessor()
{
    INT iProc = 0;

    //  get the current processor number via the favored method
    
    iProc = IprocOSSyncIGetCurrentProcessor();

    //  one of those previous methods should have worked to give us a preferred CPU, but 
    //  we used to have a fallback using SetThreadIdealProcessor() / *AffinityMask APIs
    //  to set a TLS variable.  This fallback was removed in Windows DS Depot, sdv 378819.

    OSSYNCAssert( iProc != -1 );

    //  we don't know what number to return yet

    if ( iProc == -1 )
    {
        iProc = 0;          // fall back to single threaded
    }

    //  ensure that the processor number we will return is within the valid
    //  range indicated by OSSyncGetProcessorCountMax().  this handles the
    //  case where a processor is added to the system after the sync library is
    //  initialized.  if the system adds more processors later then we will alias
    //  those onto existing processor "slots" to hide that from the higher levels
    //  of the code.  we will use these additional processors "inefficiently"
    //  until we next refresh this count
    //

    if ( iProc >= OSSyncGetProcessorCountMax() )
    {
        iProc = iProc % OSSyncGetProcessorCountMax();
    }

    return iProc;
}

//  sets the processor number returned by OSSyncGetCurrentProcessor()

void OSSYNCAPI OSSyncSetCurrentProcessor( const INT iProc )
{
    OSSYNCAssert( dwClsInvalid != g_dwClsProcIndex );
    TlsSetValue( g_dwClsProcIndex, (void*) INT_PTR( iProc == -1 ? -1 : iProc % OSSyncGetProcessorCount() ) );
}


//  Processor Local Storage

void* g_rgPLS[ MAXIMUM_PROCESSORS ];

//  configures the size of processor local storage

BOOL OSSYNCAPI FOSSyncConfigureProcessorLocalStorage( const size_t cbPLS )
{
    //  PLS is aligned on very large boundaries to make super sure it's isolated

    const size_t    cbAlign     = 256;
    const size_t    cbPLSAlign  = ( ( cbPLS + cbAlign - 1 ) / cbAlign ) * cbAlign;

    //  if PLS already exists then release it

    if ( g_rgPLS[ 0 ] )
    {
        VirtualFree( g_rgPLS[ 0 ], 0, MEM_RELEASE );
        memset( g_rgPLS, 0, sizeof( g_rgPLS ) );
    }

    //  allocate room for the new PLS, if requested

    if ( cbPLS )
    {
        g_rgPLS[ 0 ] = VirtualAlloc( NULL, g_cProcessorMax * cbPLSAlign, MEM_COMMIT, PAGE_READWRITE );
        
        if ( g_rgPLS[ 0 ] )
        {
            for ( size_t iPLS = 1; iPLS < g_cProcessorMax; iPLS++ )
            {
                g_rgPLS[ iPLS ] = (BYTE*)g_rgPLS[ 0 ] + cbPLSAlign * iPLS;
            }
        }
    }

    return cbPLS == 0 || g_rgPLS[ 0 ] != NULL;
}

//  retrieves a pointer to the current context's processor local storage

void* OSSYNCAPI OSSyncGetProcessorLocalStorage()
{
    return g_rgPLS[ OSSyncGetCurrentProcessor() ];
}

//  retrieves a pointer to a given processor's local storage

void* OSSYNCAPI OSSyncGetProcessorLocalStorage( const size_t iProc )
{
    return iProc < g_cProcessorMax ? g_rgPLS[ iProc ] : NULL;
}

#ifdef SYNC_ANALYZE_PERFORMANCE // only OSSYNC should depend upon this function, isolate to catch

//  High Resolution Timer

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )

//    QWORDX - used for 32 bit access to a 64 bit integer
//    For intel pentium only

union QWORDX {
        QWORD   qw;
        struct
        {
            DWORD l;
            DWORD h;
        };
    };
#endif

//    High Resolution Timer Type

enum HRTType
{
    hrttUninit,
    hrttNone,
    hrttWin32,
#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )
    hrttPentium,
#endif  //  _M_IX86 && SYNC_USE_X86_ASM
} g_hrttSync;

//    HRT Frequency

QWORD qwSyncHRTFreq;

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )

//    Pentium Time Stamp Counter Fetch

#define rdtsc __asm _emit 0x0f __asm _emit 0x31

#endif  //  _MM_IX86 && SYNC_USE_X86_ASM

//  returns fTrue if we are allowed to use RDTSC

BOOL IsRDTSCAvailable()
{
    static BOOL                     fRDTSCAvailable                 = fFalse;

    if ( fRDTSCAvailable )
    {
        return fRDTSCAvailable;
    }

    typedef WINBASEAPI BOOL WINAPI PFNIsProcessorFeaturePresent( IN DWORD ProcessorFeature );

    HMODULE                         hmodProcessThreads                  = NULL;
    PFNIsProcessorFeaturePresent*   pfnIsProcessorFeaturePresent    = NULL;

    hmodProcessThreads = LoadLibraryExW( L"api-ms-win-core-processthreads-l1-1-1.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32 );

    if ( hmodProcessThreads == NULL )
    {
        hmodProcessThreads = LoadLibraryExW( L"kernel32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32 );

        if ( hmodProcessThreads == NULL )
        {
            goto NoIsProcessorFeaturePresent;
        }
    }

    pfnIsProcessorFeaturePresent = (PFNIsProcessorFeaturePresent*)GetProcAddress( hmodProcessThreads, "IsProcessorFeaturePresent" );

    if ( !( pfnIsProcessorFeaturePresent ) )
    {
        goto NoIsProcessorFeaturePresent;
    }

    fRDTSCAvailable = pfnIsProcessorFeaturePresent( PF_RDTSC_INSTRUCTION_AVAILABLE );

NoIsProcessorFeaturePresent:

    if ( hmodProcessThreads )
    {
        FreeLibrary( hmodProcessThreads );
    }

    return fRDTSCAvailable;
}

//  initializes the HRT subsystem

static void OSSyncHRTIInit()
{
    //  Win32 high resolution counter is available

    if ( QueryPerformanceFrequency( (LARGE_INTEGER *) &qwSyncHRTFreq ) )
    {
        g_hrttSync = hrttWin32;
    }

    //  Win32 high resolution counter is not available

    else

    {
        //  fall back on GetTickCount() (ms since Windows has started)

        qwSyncHRTFreq = 1000;
        g_hrttSync = hrttNone;
    }

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )

    //  can we use the TSC?

    if ( IsRDTSCAvailable() )
    {
        //  use pentium TSC register, but first find clock frequency experimentally

        QWORDX qwxTime1a;
        QWORDX qwxTime1b;
        QWORDX qwxTime2a;
        QWORDX qwxTime2b;
        if ( g_hrttSync == hrttWin32 )
        {
            __asm xchg      eax, edx  //  HACK:  cl 11.00.7022 needs this
            __asm rdtsc
            __asm mov       qwxTime1a.l,eax //lint !e530
            __asm mov       qwxTime1a.h,edx //lint !e530
            QueryPerformanceCounter( (LARGE_INTEGER*) &qwxTime1b.qw );
            Sleep( 50 );
            __asm xchg      eax, edx  //  HACK:  cl 11.00.7022 needs this
            __asm rdtsc
            __asm mov       qwxTime2a.l,eax //lint !e530
            __asm mov       qwxTime2a.h,edx //lint !e530
            QueryPerformanceCounter( (LARGE_INTEGER*) &qwxTime2b.qw );
            qwSyncHRTFreq = ( qwSyncHRTFreq * ( qwxTime2a.qw - qwxTime1a.qw ) ) /
                        ( qwxTime2b.qw - qwxTime1b.qw );
            qwSyncHRTFreq = ( ( qwSyncHRTFreq + 50000 ) / 100000 ) * 100000;
        }
        else
        {
            __asm xchg      eax, edx  //  HACK:  cl 11.00.7022 needs this
            __asm rdtsc
            __asm mov       qwxTime1a.l,eax
            __asm mov       qwxTime1a.h,edx
            qwxTime1b.l = GetTickCount();
            qwxTime1b.h = 0;
            Sleep( 2000 );
            __asm xchg      eax, edx  //  HACK:  cl 11.00.7022 needs this
            __asm rdtsc
            __asm mov       qwxTime2a.l,eax
            __asm mov       qwxTime2a.h,edx
            qwxTime2b.l = GetTickCount();
            qwxTime2b.h = 0;
            qwSyncHRTFreq = ( qwSyncHRTFreq * ( qwxTime2a.qw - qwxTime1a.qw ) ) /
                        ( qwxTime2b.qw - qwxTime1b.qw );
            qwSyncHRTFreq = ( ( qwSyncHRTFreq + 500000 ) / 1000000 ) * 1000000;
        }

        g_hrttSync = hrttPentium;
    }

#endif  //  _M_IX86 && SYNC_USE_X86_ASM

}

//  returns the current HRT frequency

static QWORD OSSYNCAPI QwOSSyncIHRTFreq()
{
    if ( g_hrttSync == hrttUninit )
    {
        OSSyncHRTIInit();
    }

    return qwSyncHRTFreq;
}

//  returns the current HRT count

static QWORD OSSYNCAPI QwOSSyncIHRTCount()
{
    QWORD qw    = 0;

    if ( g_hrttSync == hrttUninit )
    {
        OSSyncHRTIInit();
    }

    switch ( g_hrttSync )
    {
        case hrttNone:
            qw = DwOSSyncITickTime();
            break;

        case hrttWin32:
            QueryPerformanceCounter( (LARGE_INTEGER*) &qw );
            break;

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )

        case hrttPentium:
        {
            QWORDX qwx;
            __asm xchg      eax, edx  //  HACK:  cl 11.00.7022 needs this
            __asm rdtsc
            __asm mov       qwx.l,eax
            __asm mov       qwx.h,edx

            qw = qwx.qw;
        }
            break;

#endif  //  _M_IX86 && SYNC_USE_X86_ASM

        default:
            OSSYNCAssert( fFalse );
            qw = 0;
    }

    return qw;
}

#endif  // SYNC_ANALYZE_PERFORMANCE


//  Timer

//  returns the current tick count where one tick is one millisecond

DWORD OSSYNCAPI DwOSSyncITickTime()
{
#pragma prefast(suppress:28159, "ESE has fixed all known 49-day-rollover problems, and GetTickCount64() is slower.")
    return GetTickCount();
}


#ifdef SYNC_ENHANCED_STATE

//  Enhanced Synchronization Object State Container

struct MemoryBlock
{
    MemoryBlock*    pmbNext;
    MemoryBlock*    pmbPrev;
    SIZE_T          ibFreeMic;
    SIZE_T          ibFreeMicPrev;
};

struct MemoryToken
{
    LONG_PTR        ibFreeMicPrev;
};

SIZE_T              g_cbMemoryBlock;
MemoryBlock*        g_pmbRoot;
MemoryBlock         g_mbSentry;
MemoryBlock*        g_pmbRootFree;
MemoryBlock         g_mbSentryFree;
CRITICAL_SECTION    g_csESMemory;
BOOL                g_fcsESMemoryInit;

#if defined(_WIN64)
const LONG_PTR maskDeletedToken = (LONG_PTR)0x8000000000000000; //  most significant bit
#else
const LONG_PTR maskDeletedToken = (LONG_PTR)0x80000000; //  most significant bit
#endif

void* OSSYNCAPI ESMemoryNew( size_t cb )
{
    if ( !FOSSyncInitForES() )
    {
        return NULL;
    }

    size_t ib = sizeof( MemoryToken );
    ib += sizeof( size_t ) - 1;
    ib -= ib % sizeof( size_t );

    cb += sizeof( size_t ) - 1;
    cb -= cb % sizeof( size_t );
    cb += ib;

    OSSYNCAssert( ( cb + sizeof( MemoryBlock ) ) <= g_cbMemoryBlock );

    EnterCriticalSection( &g_csESMemory );

    MemoryBlock* pmb = g_pmbRoot;

    //  Is there still some space left in this block?

    if ( ( pmb->ibFreeMic + cb ) > g_cbMemoryBlock )
    {
        if ( g_pmbRootFree != &g_mbSentryFree )
        {
            //  No space left, but there are blocks in the freed list.

            pmb = g_pmbRootFree;

            OSSYNCAssert( pmb->ibFreeMicPrev == 0 );
            OSSYNCAssert( pmb->ibFreeMic == sizeof( MemoryBlock ) );
    
            pmb->pmbNext->pmbPrev = NULL;
            g_pmbRootFree = pmb->pmbNext;
        }
        else
        {
            //  We'll have to allocate a new block.
            
            pmb = (MemoryBlock*) VirtualAlloc( NULL, g_cbMemoryBlock, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
            if ( pmb == NULL )
            {
                LeaveCriticalSection( &g_csESMemory );
                OSSyncTermForES();
                return NULL;
            }
        }

        pmb->pmbNext        = g_pmbRoot;
        pmb->pmbPrev        = NULL;
        pmb->ibFreeMic      = sizeof( MemoryBlock );
        pmb->ibFreeMicPrev  = 0;

        g_pmbRoot->pmbPrev      = pmb;
        g_pmbRoot               = pmb;
    }

    BYTE* pb = (BYTE*)pmb + pmb->ibFreeMic;

    MemoryToken mt = { 0 };
    mt.ibFreeMicPrev = pmb->ibFreeMicPrev;
    pmb->ibFreeMicPrev = pmb->ibFreeMic;
    pmb->ibFreeMic += cb;

    LeaveCriticalSection( &g_csESMemory );

    memset( pb, 0, cb );
    *((MemoryToken*)pb) = mt;

    return (void*)( pb + ib );
}

void OSSYNCAPI ESMemoryDelete( void* pv )
{
    if ( pv != NULL )
    {
        BYTE* pb = (BYTE*)pv - sizeof( MemoryToken );

        OSSYNCAssert( pb != NULL );

        MemoryBlock* const pmb = (MemoryBlock*)( UINT_PTR( pb ) - UINT_PTR( pb ) % g_cbMemoryBlock );
        OSSYNCAssert( pmb != &g_mbSentry );

        EnterCriticalSection( &g_csESMemory );

        //  Tag the token as deallocated by setting its most significant bit.

        OSSYNCAssert( ( ((MemoryToken*)pb)->ibFreeMicPrev & maskDeletedToken ) == 0 );
        ((MemoryToken*)pb)->ibFreeMicPrev |= maskDeletedToken;

        //  Free space in this block as long as we can find free blocks, starting at the last one.

        for ( MemoryToken* pmt = (MemoryToken*)( (BYTE*)pmb + pmb->ibFreeMicPrev );
                ( pmb->ibFreeMicPrev > 0 ) && ( ( pmt->ibFreeMicPrev & maskDeletedToken ) != 0 );
                pmt = (MemoryToken*)( (BYTE*)pmb + pmt->ibFreeMicPrev ) )
        {
            //  Memory token must be within a valid range in the block.

            OSSYNCAssert( (BYTE*)pmt >= ( (BYTE*)pmb + sizeof( MemoryBlock ) ) );
            OSSYNCAssert( (BYTE*)pmt < ( (BYTE*)pmb + g_cbMemoryBlock ) );

            //  Update offsets.

            pmt->ibFreeMicPrev &= ~maskDeletedToken;
            pmb->ibFreeMic = pmb->ibFreeMicPrev;
            pmb->ibFreeMicPrev = pmt->ibFreeMicPrev;
        }
        OSSYNCAssert( pmb->ibFreeMicPrev < pmb->ibFreeMic );

        //  We can free the entire block if we've freed all the tokens.

        if ( pmb->ibFreeMicPrev == 0 )
        {
            OSSYNCAssert( pmb->ibFreeMic == sizeof( MemoryBlock ) );
            if ( pmb->pmbNext != NULL )
            {
                pmb->pmbNext->pmbPrev = pmb->pmbPrev;
            }

            if ( pmb->pmbPrev != NULL )
            {
                pmb->pmbPrev->pmbNext = pmb->pmbNext;
            }
            else
            {
                g_pmbRoot = pmb->pmbNext;
            }
            
            pmb->pmbNext        = g_pmbRootFree;
            pmb->pmbPrev        = NULL;
            pmb->ibFreeMic      = sizeof( MemoryBlock );
            pmb->ibFreeMicPrev  = 0;

            g_pmbRootFree->pmbPrev  = pmb;
            g_pmbRootFree           = pmb;
        }

        LeaveCriticalSection( &g_csESMemory );

        OSSyncTermForES();
    }
}

#endif  //  SYNC_ENHANCED_STATE


//  Synchronization Object Basic Information

#ifdef SYNC_ENHANCED_STATE

//  ctor

CSyncBasicInfo::CSyncBasicInfo( const char* szInstanceName )
{
    m_szInstanceName    = szInstanceName;
    m_szTypeName        = NULL;
    m_psyncobj          = NULL;
}

//  dtor

CSyncBasicInfo::~CSyncBasicInfo()
{
}

#endif  //  SYNC_ENHANCED_STATE


//  Synchronization Object Performance:  Wait Times

//  ctor

CSyncPerfWait::CSyncPerfWait()
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    m_cWait = 0;
    m_qwHRTWaitElapsed = 0;

#endif  //  SYNC_ANALYZE_PERFORMANCE
}

//  dtor

CSyncPerfWait::~CSyncPerfWait()
{
}


//  Null Synchronization Object State Initializer

const CSyncStateInitNull syncstateNull;


//  Kernel Semaphore

//  ctor

CKernelSemaphore::CKernelSemaphore( const CSyncBasicInfo& sbi )
    :   CEnhancedStateContainer< CKernelSemaphoreState, CSyncStateInitNull, CKernelSemaphoreInfo, CSyncBasicInfo >( syncstateNull, sbi )
{
    //  further init of CSyncBasicInfo

    State().SetTypeName( "CKernelSemaphore" );
    State().SetInstance( (CSyncObject*)this );
}

//  dtor

CKernelSemaphore::~CKernelSemaphore()
{
    //  semaphore should not be initialized

    OSSYNCAssert( !FInitialized() );

#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

    //  dump performance data

    OSSyncStatsDump(    State().SzTypeName(),
                        State().SzInstanceName(),
                        State().Instance(),
                        (DWORD)-1,
                        State().CWaitTotal(),
                        State().CsecWaitElapsed(),
                        0,
                        0,
                        0,
                        0 );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
}

typedef
WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
PFNCreateSemaphoreW(
    _In_opt_ LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    _In_     LONG lInitialCount,
    _In_     LONG lMaximumCount,
    _In_opt_ LPCWSTR lpName
    );

typedef
WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
PFNCreateSemaphoreExW(
    _In_opt_ LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    _In_ LONG lInitialCount,
    _In_ LONG lMaximumCount,
    _In_opt_ LPCWSTR lpName,
    _Reserved_ DWORD dwFlags,
    _In_ DWORD dwDesiredAccess
    );


PFNCreateSemaphoreW *   g_pfnCreateSemaphoreW = NULL;
PFNCreateSemaphoreExW * g_pfnCreateSemaphoreExW = NULL;


BOOL FKernelSemaphoreICreateILoad()
{
    //  attempt to retrieve the DLL / library with the functionality we need
    //

    HMODULE hmodCS = LoadLibraryExW( L"api-ms-win-core-synch-l1-1-0.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32 );
    if ( hmodCS == NULL )
    {
        hmodCS = LoadLibraryExW( L"kernel32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32 );
    }
    OSSYNCAssert( hmodCS );

    //  attempt to retrieve the function
    //

    g_pfnCreateSemaphoreW = (PFNCreateSemaphoreW*)GetProcAddress( hmodCS, "CreateSemaphoreW" );
    g_pfnCreateSemaphoreExW = (PFNCreateSemaphoreExW*)GetProcAddress( hmodCS, "CreateSemaphoreExW" );

    return ( g_pfnCreateSemaphoreW != NULL || g_pfnCreateSemaphoreExW != NULL );
}

HANDLE HKernelSemaphoreICreate( void )
{
    const LONG lMaxCount = 0x7FFFFFFFL;

    //  fast / post init path
    //

    OSSYNCAssert( g_pfnCreateSemaphoreW != NULL || g_pfnCreateSemaphoreExW != NULL );

    if ( g_pfnCreateSemaphoreW && FRandomlyFaultFalse() )
    {
        return g_pfnCreateSemaphoreW( 0, 0, lMaxCount, NULL );
    }
    if ( g_pfnCreateSemaphoreExW )
    {
        return g_pfnCreateSemaphoreExW( 0, 0, lMaxCount, NULL, 0, SEMAPHORE_ALL_ACCESS );
    }

    return NULL;
}

//  initialize the semaphore, returning 0 on failure

const BOOL CKernelSemaphore::FInit()
{
    //  semaphore should not be initialized

    OSSYNCAssert( !FInitialized() );

    //  allocate kernel semaphore object

    State().SetHandle( HKernelSemaphoreICreate() );

    if ( State().Handle() )
    {
        SetHandleInformation( State().Handle(), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
    }

    //  semaphore should have no available counts, if allocated

    OSSYNCAssert( State().Handle() == 0 || FReset() );

    //  return result of init

    return State().Handle() != 0;
}

//  terminate the semaphore

void CKernelSemaphore::Term()
{
    //  semaphore should be initialized

    OSSYNCAssert( FInitialized() );

    //  semaphore should have no available counts

    OSSYNCAssert( FReset() || g_fSyncProcessAbort );

    //  deallocate kernel semaphore object

    SetHandleInformation( State().Handle(), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
    INT fSuccess = CloseHandle( State().Handle() );
    OSSYNCAssert( fSuccess );

    //  reset state

    State().SetHandle( 0 );
}

//  acquire one count of the semaphore, waiting only for the specified interval.
//  returns 0 if the wait timed out before a count could be acquired
//
//  for negative intervals not related to INF, deadlock detection logic is turned off

const BOOL CKernelSemaphore::FAcquire( const INT cmsecTimeout )
{
    //  semaphore should be initialized

    OSSYNCAssert( FInitialized() );

    //  wait for semaphore

    BOOL fSuccess;

    if ( cmsecTimeout != cmsecTest )
    {
        PERFOpt( AtomicIncrement( (LONG *)&g_cOSSYNCThreadBlock ) );
        State().StartWait();
    }

#ifdef SYNC_DEADLOCK_DETECTION
    if ( cmsecTimeout == cmsecInfinite || cmsecTimeout > cmsecDeadlock )
    {
        OnThreadWaitBegin();
        fSuccess = WaitForSingleObjectEx( State().Handle(), cmsecDeadlock, FALSE ) == WAIT_OBJECT_0;
        OnThreadWaitEnd();

        if ( !fSuccess )
        {
#ifdef DEBUG
            SYNCDeadLockTimeOutState sdltosStatePre = sdltosEnabled;
            OSSYNC_FOREVER  // spin until we get a non- check-in-progress state ...
            {
                C_ASSERT( sizeof(g_sdltosState) == sizeof(LONG) );
                sdltosStatePre = (SYNCDeadLockTimeOutState)AtomicCompareExchange( (LONG*)&g_sdltosState, sdltosEnabled, sdltosCheckInProgress );
                if ( sdltosStatePre != sdltosCheckInProgress )
                {
                    break;
                }
                Sleep( 16 );
            }
#else
            SYNCDeadLockTimeOutState sdltosStatePre = sdltosEnabled;
#endif

            OSSYNCAssertSzRTL( fSuccess /* superflous */ || sdltosStatePre == sdltosDisabled, "Potential Deadlock Detected (Timeout);  FYI ed [dll]!OSSYNC::g_sdltosState to 0 to disable." );

#ifdef DEBUG
            if ( sdltosStatePre != sdltosDisabled )
            {
                //  needs re-enabling (if SOMEONE/debugger didn't play with state)
                OSSYNCAssert( sdltosStatePre != sdltosCheckInProgress ); // that'd be wrong on convergence loop above.

                //  while it seems really simple this should alwyas be reset, this is designed to allow the user to kill
                //  timeout detection dynamically in the debugger by setting g_sdltosState = 0 (i.e. sdltosDisabled) via 
                //  debugger, thus g_sdltosState won't == sdltosCheckInProgress and we won't reset it to sdltosEnabled / 
                //  i.e. disabling deadlock detection for all subsequent hits.

                const SYNCDeadLockTimeOutState sdltosCheck = (SYNCDeadLockTimeOutState)AtomicCompareExchange( (LONG*)&g_sdltosState, sdltosCheckInProgress, sdltosEnabled );
                OSSYNCAssertSzRTL( sdltosCheck != sdltosDisabled, "Devs, the debugger used to set g_sdltosState to 0 and disables further timeout detection asserts!?  Just an FYI.  If you did not, then code is confused." );
            }
#endif

            const INT cmsecWait =   cmsecTimeout == cmsecInfinite ?
                                        cmsecInfinite :
                                        cmsecTimeout - cmsecDeadlock;

            OnThreadWaitBegin();
            fSuccess = WaitForSingleObjectEx( State().Handle(), cmsecWait, FALSE ) == WAIT_OBJECT_0;
            OnThreadWaitEnd();
        }
    }
    else
#endif  //  SYNC_DEADLOCK_DETECTION
    {
        const INT cmsecWait =   cmsecInfinite == cmsecTimeout || cmsecInfiniteNoDeadlock == cmsecTimeout ?
                            cmsecInfinite :
                            abs( cmsecTimeout );

        if ( cmsecTimeout != cmsecTest )
        {
            OnThreadWaitBegin();
        }
        fSuccess = WaitForSingleObjectEx( State().Handle(), cmsecWait, FALSE ) == WAIT_OBJECT_0;
        if ( cmsecTimeout != cmsecTest )
        {
            OnThreadWaitEnd();
        }
    }

    if ( cmsecTimeout != cmsecTest )
    {
        State().StopWait();
        PERFOpt( AtomicIncrement( (LONG *)&g_cOSSYNCThreadResume ) );
    }

    return fSuccess;
}

//  releases the given number of counts to the semaphore, waking the appropriate
//  number of waiters

void CKernelSemaphore::Release( const INT cToRelease )
{
    //  semaphore should be initialized

    OSSYNCAssert( FInitialized() );

    //  release semaphore

    const BOOL fSuccess = ReleaseSemaphore( HANDLE( State().Handle() ), cToRelease, 0 );
    OSSYNCAssert( fSuccess );
}


//  Semaphore

//  ctor

CSemaphore::CSemaphore( const CSyncBasicInfo& sbi )
    :   CEnhancedStateContainer< CSemaphoreState, CSyncStateInitNull, CSemaphoreInfo, CSyncBasicInfo >( syncstateNull, sbi )
{
    //  further init of CSyncBasicInfo

    State().SetTypeName( "CSemaphore" );
    State().SetInstance( (CSyncObject*)this );
}

//  dtor

CSemaphore::~CSemaphore()
{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

    //  dump performance data

    OSSyncStatsDump(    State().SzTypeName(),
                        State().SzInstanceName(),
                        State().Instance(),
                        (DWORD)-1,
                        State().CWaitTotal(),
                        State().CsecWaitElapsed(),
                        State().CAcquireTotal(),
                        State().CContendTotal(),
                        0,
                        0 );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
}


const DWORD CSemaphore::_DwOSTimeout( const INT cmsecTimeout )
{
    if ( cmsecTimeout == cmsecInfinite || cmsecTimeout == cmsecInfiniteNoDeadlock )
    {
        return INFINITE;
    }
    else if ( cmsecTimeout >= 0 )
    {
        return cmsecTimeout;
    }
    else
    {
        return 0;
    }
}

//  attempts to acquire a count from the semaphore, returning fFalse if unsuccessful
//  in the time permitted.  Infinite and Test-Only timeouts are supported.

const BOOL CSemaphore::_FAcquire( const DWORD dwTimeout )
{
    const DWORD dwStart = DwOSSyncITickTime();
    DWORD dwRemaining = dwTimeout;

    State().IncWait();
    OSSYNC_FOREVER
    {
        const CSemaphoreState stateCur = State();

        if ( stateCur.CAvail() > 0 )
        {
            OSSYNCAssert( stateCur.CWait() > 0 );

            // Atomically acquire the semaphore and decrement the waiting counter.
            const CSemaphoreState stateNew( stateCur.CAvail() - 1, stateCur.CWait() - 1 );
            if ( State().FChange( stateCur, stateNew ) )
            {
                return fTrue;
            }
        }
        else if ( dwRemaining == 0 )
        {
            break;
        }
        else
        {
            const DWORD dwElapsed = DwOSSyncITickTime() - dwStart;
            if ( dwElapsed > dwTimeout )
            {
                break;
            }

            dwRemaining = dwTimeout - dwElapsed;

            if ( !_FWait( stateCur.CAvail(), dwRemaining ) )
            {
                break;
            }
        }
    }
    State().DecWait();

    return fFalse;
}

//  releases all waiters on the semaphore

void CSemaphore::ReleaseAllWaiters()
{
    OSSYNC_FOREVER
    {
        const CSemaphoreState stateCur = State();

        if ( stateCur.CAvail() > 0 && stateCur.CWait() > 0 )
        {
            // The existing waiters are in transition.
            continue;
        }
        else
        {
            const CSemaphoreState stateNew( stateCur.CWait(), stateCur.CWait() );
            if ( State().FChange(stateCur, stateNew ) )
            {
                volatile void *pv = State().PAvail();

                WakeByAddressAll( (void*)pv );

                return;
            }
        }
    }
}

//  releases the given number of counts to the semaphore, waking the appropriate
//  number of waiters

void CSemaphore::_Release( const INT cToRelease )
{
    if ( cToRelease <= 0 )
    {
        return;
    }

    State().IncAvail( cToRelease );

    const INT cWait = State().CWait();
    if ( cWait == 0 )
    {
        // No one is waiting.
    }
    else if ( cWait <= cToRelease )
    {
        volatile void *pv = State().PAvail();
        // No more waiting threads than `cToRelease`, wake everyone.
        WakeByAddressAll( (void*)pv );
    }
    else
    {
        volatile void *pv = State().PAvail();
        // Wake at most `cToRelease` threads.  The benefit from not waking unnecessary
        // threads is expected to be greater than the loss on extra calls below.
        for ( INT i = 0; i < cToRelease; i++ )
        {
            WakeByAddressSingle( (void*)pv );
        }
    }
}


const BOOL CSemaphore::_FWait( const INT cAvail, const DWORD dwTimeout )
{
    PERFOpt( AtomicIncrement( (LONG*)&g_cOSSYNCThreadBlock ) );
    State().StartWait();

    BOOL fSuccess;

#ifdef SYNC_DEADLOCK_DETECTION
    if (dwTimeout > cmsecDeadlock )
    {
        fSuccess = _FOSWait( cAvail, cmsecDeadlock );
        if ( !fSuccess )
        {
#ifdef DEBUG
            SYNCDeadLockTimeOutState sdltosStatePre = sdltosEnabled;
            OSSYNC_FOREVER
            {
                C_ASSERT( sizeof(g_sdltosState) == sizeof(LONG) );
                sdltosStatePre = (SYNCDeadLockTimeOutState)AtomicCompareExchange( (LONG*)&g_sdltosState, sdltosEnabled, sdltosCheckInProgress );
                if ( sdltosStatePre != sdltosCheckInProgress )
                {
                    break;
                }
                Sleep( 16 );
            }
#else
            SYNCDeadLockTimeOutState sdltosStatePre = sdltosEnabled;
#endif

            OSSYNCAssertSzRTL( fSuccess  || sdltosStatePre == sdltosDisabled, "Potential Deadlock Detected (Timeout);  FYI ed [dll]!OSSYNC::g_sdltosState to 0 to disable." );

#ifdef DEBUG
            if ( sdltosStatePre != sdltosDisabled )
            {
                OSSYNCAssert( sdltosStatePre != sdltosCheckInProgress );


                const SYNCDeadLockTimeOutState sdltosCheck = (SYNCDeadLockTimeOutState)AtomicCompareExchange( (LONG*)&g_sdltosState, sdltosCheckInProgress, sdltosEnabled );
                OSSYNCAssertSzRTL( sdltosCheck != sdltosDisabled, "Devs, the debugger used to set g_sdltosState to 0 and disables further timeout detection asserts!?  Just an FYI.  If you did not, then code is confused." );
            }
#endif

            DWORD dwNewTimeout = dwTimeout;

            if ( dwNewTimeout < INFINITE )
            {
                dwNewTimeout -= cmsecDeadlock;
            }

            fSuccess = _FOSWait( cAvail, dwNewTimeout );
        }
    }
    else
#endif
    {
        fSuccess = _FOSWait( cAvail, dwTimeout );
    }

    State().StopWait();
    PERFOpt( AtomicIncrement( (LONG*)&g_cOSSYNCThreadResume ) );

    return fSuccess;
}


const BOOL CSemaphore::_FOSWait( const INT cAvail, const DWORD dwTimeout )
{
    volatile void *pv = State().PAvail();

    OnThreadWaitBegin();
    BOOL fSuccess = WaitOnAddress( pv, (PVOID)&cAvail, sizeof(cAvail), dwTimeout );
    OnThreadWaitEnd();

    return fSuccess;
}


//  performance data dumping

#include<stdarg.h>
#include<stdio.h>
#include<tchar.h>

//  ================================================================
class CPrintF
//  ================================================================
{
    public:
        CPrintF() {}
        virtual ~CPrintF() {}

    public:
        virtual void __cdecl operator()( const CHAR* szFormat, ... ) = 0;
};

//  ================================================================
class CIPrintF : public CPrintF
//  ================================================================
{
    public:
        CIPrintF( CPrintF* pprintf );

        void __cdecl operator()( const CHAR* szFormat, ... );

        virtual void Indent();
        virtual void Unindent();

    protected:
        CIPrintF();

    private:
        CPrintF* const      m_pprintf;
        INT                 m_cindent;
        BOOL                m_fBOL;
};

//  ================================================================
inline CIPrintF::CIPrintF( CPrintF* pprintf ) :
//  ================================================================
    m_cindent( 0 ),
    m_pprintf( pprintf ),
    m_fBOL( fTrue )
{
}

//  ================================================================
inline void __cdecl CIPrintF::operator()( const CHAR* szFormat, ... )
//  ================================================================
{
    CHAR szT[ 1024 ];
    va_list arg_ptr;
    va_start( arg_ptr, szFormat );
    StringCbVPrintfA( szT, sizeof(szT), szFormat, arg_ptr );
    va_end( arg_ptr );

    CHAR*   szLast  = szT;
    CHAR*   szCurr  = szT;
    while ( *szCurr )
    {
        if ( m_fBOL )
        {
            for ( INT i = 0; i < m_cindent; i++ )
            {
                (*m_pprintf)( "\t" );
            }
            m_fBOL = fFalse;
        }

        szCurr = szLast + strcspn( szLast, "\r\n" );
        while ( *szCurr == '\r' )
        {
            szCurr++;
            m_fBOL = fTrue;
        }
        if ( *szCurr == '\n' )
        {
            szCurr++;
            m_fBOL = fTrue;
        }

        (*m_pprintf)( "%.*s", szCurr - szLast, szLast );

        szLast = szCurr;
    }
}

//  ================================================================
inline void CIPrintF::Indent()
//  ================================================================
{
    ++m_cindent;
}

//  ================================================================
inline void CIPrintF::Unindent()
//  ================================================================
{
    if ( m_cindent > 0 )
    {
        --m_cindent;
    }
}

//  ================================================================
inline CIPrintF::CIPrintF( ) :
//  ================================================================
    m_cindent( 0 ),
    m_pprintf( 0 ),
    m_fBOL( fTrue )
{
}

//  ================================================================
class CFPrintF : public CPrintF
//  ================================================================
{
    public:
        CFPrintF( const WCHAR* szFile );
        ~CFPrintF();

        void __cdecl operator()( const char* szFormat, ... );

    private:
        void* m_hFile;
        void* m_hMutex;
};

CFPrintF::CFPrintF( const WCHAR* szFile )
    : m_hMutex( NULL )
{
    //  open the file for append
    m_hFile = (void*)CreateFileW( szFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

    if ( INVALID_HANDLE_VALUE == m_hFile )
    {
        return;
    }
    SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

    m_hMutex = (void*)CreateMutexW( NULL, FALSE, NULL );
    
    if ( !( m_hMutex ) )
    {
        SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( HANDLE( m_hFile ) );
        m_hFile = INVALID_HANDLE_VALUE;
        return;
    }
    SetHandleInformation( HANDLE( m_hMutex ), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
}

CFPrintF::~CFPrintF()
{
    //  close the file

    if ( m_hMutex )
    {
        SetHandleInformation( HANDLE( m_hMutex ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( HANDLE( m_hMutex ) );
        m_hMutex = NULL;
    }

    if ( m_hFile != INVALID_HANDLE_VALUE )
    {
        SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( HANDLE( m_hFile ) );
        m_hFile = INVALID_HANDLE_VALUE;
    }
}

//  ================================================================
void __cdecl CFPrintF::operator()( const char* szFormat, ... )
//  ================================================================
{
    if ( HANDLE( m_hFile ) != INVALID_HANDLE_VALUE )
    {
        const SIZE_T cchBuf = 1024;
        char szBuf[ cchBuf ];

        //  print into a temp buffer, truncating the string if too large

        va_list arg_ptr;
        va_start( arg_ptr, szFormat );
        StringCbVPrintfA( szBuf, cchBuf, szFormat, arg_ptr );
        va_end( arg_ptr );

        //  append the string to the file

        WaitForSingleObject( HANDLE( m_hMutex ), INFINITE );

        const LARGE_INTEGER ibOffset = { 0, 0 };
        SetFilePointerEx( HANDLE( m_hFile ), ibOffset, NULL, FILE_END );

        DWORD cbWritten;
        WriteFile( HANDLE( m_hFile ), szBuf, DWORD( strlen( szBuf ) * sizeof( char ) ), &cbWritten, NULL );

        ReleaseMutex( HANDLE( m_hMutex ) );
    }
}


#ifdef DEBUGGER_EXTENSION

#define LOCAL static

// Allows easier porting from the older wdbgexts-style extensions.
#define dprintf EDBGPrintf


namespace OSSYM {

LOCAL PDEBUG_CLIENT5 g_DebugClient;
LOCAL PDEBUG_CONTROL4 g_DebugControl;
LOCAL PDEBUG_SYMBOLS g_DebugSymbols;
LOCAL PDEBUG_SYSTEM_OBJECTS g_DebugSystemObjects;
LOCAL PDEBUG_DATA_SPACES g_DebugDataSpaces;

//  ================================================================
LOCAL BOOL FOSEdbgCreateDebuggerInterface(
PDEBUG_CLIENT   pdebugClient
)
//  ================================================================
{
    HRESULT hr;

    if ((hr = pdebugClient->QueryInterface(__uuidof(IDebugClient5),
                                          (void **)&g_DebugClient)) != S_OK)
    {
        goto HandleError;
    }

    if ((hr = g_DebugClient->QueryInterface(__uuidof(IDebugControl4),
                                          (void **)&g_DebugControl)) != S_OK)
    {
        goto HandleError;
    }

    if ((hr = g_DebugClient->QueryInterface(__uuidof(IDebugSymbols),
                                          (void **)&g_DebugSymbols)) != S_OK)
    {
        goto HandleError;
    }


    if ((hr = g_DebugClient->QueryInterface(__uuidof(IDebugSystemObjects),
                                          (void **)&g_DebugSystemObjects)) != S_OK)
    {
        goto HandleError;
    }

    if ((hr = g_DebugClient->QueryInterface(__uuidof(IDebugDataSpaces),
                                          (void **)&g_DebugDataSpaces)) != S_OK)
    {
        goto HandleError;
    }


HandleError:
    if ( FAILED( hr ) )
    {
        // Do I have to release the pointers?
        g_DebugClient = NULL;
        g_DebugControl = NULL;
    }
    return SUCCEEDED( hr );
}

//  ================================================================
HRESULT
EDBGPrintf(
    __in PCSTR szFormat,
    ...
)
//  ================================================================
{
    HRESULT hr = S_OK;
    va_list Args;

    va_start(Args, szFormat);
    if ( g_DebugControl == NULL )
    {
        // Can't print out a failure message -- it would just recurse.
    }
    else
    {
        hr = g_DebugControl->ControlledOutputVaList(
            DEBUG_OUTCTL_AMBIENT_TEXT, DEBUG_OUTPUT_NORMAL, szFormat, Args );
    }
    va_end(Args);
    return hr;
}

//  ================================================================
HRESULT
EDBGPrintfDml(
    __in PCSTR szFormat,
    ...
)
//  ================================================================
{
    HRESULT hr = S_OK;
    va_list Args;
    static BOOL fDmlAttemptedAndFailed = fFalse;

    va_start(Args, szFormat);
    if ( g_DebugControl == NULL )
    {
        dprintf( "Using an old debugger! No DML output.\n" );
    }
    else
    {
        hr = g_DebugControl->ControlledOutputVaList(
            DEBUG_OUTCTL_AMBIENT_DML, DEBUG_OUTPUT_NORMAL, szFormat, Args );
        if ( FAILED( hr ) )
        {
            if ( ! fDmlAttemptedAndFailed )
            {
                fDmlAttemptedAndFailed = fTrue;
                g_DebugControl->Output(
                    DEBUG_OUTPUT_ERROR, "Failed to output DML (%#x)! Please use a debugger 6.6.7 or newer.\n", hr );
            }
            hr = g_DebugControl->ControlledOutputVaList(
                DEBUG_OUTCTL_AMBIENT_TEXT, DEBUG_OUTPUT_NORMAL, szFormat, Args );
            if ( FAILED( hr ) )
            {
                g_DebugControl->Output(
                    DEBUG_OUTPUT_ERROR, "Failed to write regular output. hr = %#x.\n", hr );
            }
        }
    }
    va_end(Args);
    return hr;
}

//  ================================================================
ULONG64
GetExpression(
    __in PCSTR  szExpression
)
//  ================================================================
{
    HRESULT hr = S_OK;
    ULONG EndIdx;
    DEBUG_VALUE FullValue;
    ULONG64 Address = 0;

    hr = g_DebugControl->Evaluate( szExpression, DEBUG_VALUE_INT64, &FullValue, &EndIdx );
    if ( SUCCEEDED( hr ) )
    {
        Address = FullValue.I64;
    }

    return Address;
}



//  ================================================================
BOOL
FEDBGMemoryRead(
    ULONG64                         ulAddressInDebuggee,
    __out_bcount(cbBuffer) PVOID    pbBuffer,
    __in ULONG                      cbBuffer,
    __out PULONG                    pcbRead
)
//  ================================================================
{
    HRESULT hr;
    hr = g_DebugDataSpaces->ReadVirtual( ulAddressInDebuggee, pbBuffer, cbBuffer, pcbRead );
    return SUCCEEDED( hr );
}

//  ================================================================
LOCAL BOOL FUlFromSz( const char* const sz, ULONG* const pul, const INT base = 16 )
//  ================================================================
{
    if( sz && *sz )
    {
        char* pchEnd;
        *pul = strtoul( sz, &pchEnd, base );
        return !( *pchEnd );
    }
    return fFalse;
}

//  ================================================================
template< class T >
LOCAL BOOL FAddressFromSz( const char* const sz, T** const ppt )
//  ================================================================
{
    BOOL f = fFalse;
    if ( sz && *sz )
    {
        *ppt = (T*) GetExpression( sz );
        if ( *ppt != 0 )
        {
            f = fTrue;
        }
    }
    return f;
}

//  ================================================================
template< class T >
LOCAL BOOL FAddressFromGlobal( const char* const szGlobal, T** const ppt )
//  ================================================================
{
    HRESULT hr = S_OK;
    ULONG EndIdx;
    DEBUG_VALUE FullValue;
    ULONG64 Address = 0;

    hr = g_DebugControl-> Evaluate( szGlobal, DEBUG_VALUE_INT64, &FullValue, &EndIdx);

    if ( SUCCEEDED( hr ) )
    {
        Address = FullValue.I64;

        //  return the address of the symbol
        *ppt = (T*)Address;
    }

    return SUCCEEDED( hr );

}

//  ================================================================
template< class T >
LOCAL BOOL FGlobalFromAddress( T* const pt, __out_bcount(cbMax) PSTR szGlobal, const SIZE_T cbMax, DWORD_PTR* const pdwOffset = NULL )
//  ================================================================
{
    ULONG64 ulAddress = (ULONG64) pt;
    DWORD64 dwOffset;
    DWORD   cbActual;

    HRESULT hr;
    hr = g_DebugSymbols->GetNameByOffset(
        ulAddress,
        szGlobal,
        (ULONG) cbMax,
        &cbActual,
        &dwOffset
        );

    if ( pdwOffset )
    {
        *pdwOffset = (DWORD_PTR) dwOffset;
    }

    return SUCCEEDED( hr );
}

//  ================================================================
template< class T >
LOCAL BOOL FFetchVariable( T* const rgtDebuggee, T** const prgt, SIZE_T ct = 1 )
//  ================================================================
{
    //  allocate enough storage to retrieve the requested type array
    // debugger extensions only, not real danger but no regression danger either
    //
    // check to see if it fits in a SIZE_T
    if ( ( ~( SIZE_T( 0 ) ) / sizeof( T ) ) <= ct )
    {
        dprintf( "FFetchVariable failed with data overflow\n" );
        return fFalse;
    }

    // it won't overflow a SIZE_T but we need a DWORD
    //
    if ( SIZE_T( sizeof( T ) * ct ) >= SIZE_T( ~( DWORD( 0 ) ) ) )
    {
        dprintf( "FFetchVariable failed with data too big\n" );
        return fFalse;
    }


    const SIZE_T cbrgt = sizeof( T ) * ct;

    *prgt = (T*)LocalAlloc( 0, cbrgt );

    if ( !( *prgt ) )
    {
        return fFalse;
    }

    //  retrieve the requested type array

    if ( !FEDBGMemoryRead( (ULONG64)rgtDebuggee, (void*)*prgt, DWORD( cbrgt ), NULL ) )
    {
        LocalFree( (void*)*prgt );
        return fFalse;
    }

    return fTrue;
}

//  ================================================================
template< class T >
LOCAL BOOL FFetchGlobal( const char* const szGlobal, T** const prgt, SIZE_T ct = 1 )
//  ================================================================
{
    //  get the address of the global in the debuggee and fetch it

    T*  rgtDebuggee;

    if ( FAddressFromGlobal( szGlobal, &rgtDebuggee )
        && FFetchVariable( rgtDebuggee, prgt, ct ) )
        return fTrue;
    else
    {
        dprintf( "Error: Could not fetch global variable '%s'.\n", szGlobal );
        return fFalse;
    }
}

//  ================================================================
template< class T >
LOCAL BOOL FFetchSz( T* const szDebuggee, T** const psz )
//  ================================================================
{
    //  scan for the null terminator in the debuggee starting at the given
    //  address to get the size of the string

    const SIZE_T    ctScan              = 256;
    const SIZE_T    cbScan              = ctScan * sizeof( T );
    BYTE            rgbScan[ cbScan ];
    T*              rgtScan             = (T*)rgbScan;  //  because T can be const
    SIZE_T          itScan              = SIZE_T( ~0 );
    SIZE_T          itScanLim           = 0;

    do  {
        if ( !( ++itScan % ctScan ) )
        {
            ULONG   cbRead;
            FEDBGMemoryRead(
                                ULONG64( szDebuggee + itScan ),
                                (void*)rgbScan,
                                cbScan,
                                &cbRead );

            itScanLim = itScan + cbRead / sizeof( T );
        }
    }
    while ( itScan < itScanLim && rgtScan[ itScan % ctScan ] );

    //  we found a null terminator

    if ( itScan < itScanLim )
    {
        //  fetch the string using the determined string length

        return FFetchVariable( szDebuggee, psz, itScan + 1 );
    }

    //  we did not find a null terminator

    else
    {
        //  fail the operation

        return fFalse;
    }
}

//  ================================================================
template< class T >
LOCAL void Unfetch( T* const rgt )
//  ================================================================
{
    LocalFree( (void*)rgt );
}

}; // namespace OSSYM


using namespace OSSYM;


//  member dumping functions

class CDumpContext
{
    public:

        CDumpContext(   CIPrintF&       iprintf,
                        const DWORD_PTR dwOffset,
                        const INT       cLevel )
            :   m_iprintf( iprintf ),
                m_dwOffset( dwOffset ),
                m_cLevel( cLevel )
        {
        }

        void* operator new( size_t cb ) { return LocalAlloc( 0, cb ); }
        void operator delete( void* pv ) { LocalFree( pv ); }

    public:

        CIPrintF&       m_iprintf;
        const DWORD_PTR m_dwOffset;
        const INT       m_cLevel;
};

#define SYMBOL_LEN_MAX      24
#define VOID_CB_DUMP        8

//  ================================================================
LOCAL VOID SprintHex(
    __in_bcount(cbDest) PSTR const szDest,
    const INT           cbDest,
    const BYTE * const  rgbSrc,
    const INT           cbSrc,
    const INT           cbWidth,
    const INT           cbChunk,
    const INT           cbAddress,
    const INT           cbStart)
//  ================================================================
{
    static const CHAR rgchConvert[] =   { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };

    const BYTE * const pbMax = rgbSrc + cbSrc;
    const INT cchHexWidth = ( cbWidth * 2 ) + (  cbWidth / cbChunk );

    const BYTE * pb = rgbSrc;
    CHAR * sz = szDest;
    while( pbMax != pb )
    {
        if ( 0 != cbAddress )
        {
            StringCbPrintfA( sz, cbDest-(sz-szDest), "%*.*lx    ", cbAddress, cbAddress, (DWORD)(pb - rgbSrc) + cbStart );
            sz += strlen( sz );
        }
        CHAR * szHex    = sz;
        CHAR * szText   = sz + cchHexWidth;
        do
        {
            for( INT cb = 0; cbChunk > cb && pbMax != pb; ++cb, ++pb )
            {
                *szHex++    = rgchConvert[ *pb >> 4 ];
                *szHex++    = rgchConvert[ *pb & 0x0F ];
                *szText++   = isprint( *pb ) ? *pb : '.';
            }
            *szHex++ = ' ';
        } while( ( ( pb - rgbSrc ) % cbWidth ) && pbMax > pb );
        while( szHex != sz + cchHexWidth )
        {
            *szHex++ = ' ';
        }
        *szText++ = '\n';
        *szText = '\0';
        sz = szText;
    }
}

template< class M >
inline void DumpMemberValue(    const CDumpContext& dc,
                                const M&        m )
{
    char szHex[ 1024 ] = "\n";

    SprintHex(  szHex,
                sizeof(szHex),
                (BYTE *)&m,
                min( sizeof( M ), VOID_CB_DUMP ),
                min( sizeof( M ), VOID_CB_DUMP ) + 1,
                4,
                0,
                0 );

    dc.m_iprintf( "%s", szHex );
}

template< class M >
inline void DumpMember_(    const CDumpContext&     dc,
                            const M&            m,
                            const CHAR* const   szM )
{
    if ( dc.m_cLevel > 0 )
    {
        dc.m_iprintf.Indent();

        dc.m_iprintf(   "%*.*s <0x%0*I64x,%3i>:  ",
                        SYMBOL_LEN_MAX,
                        SYMBOL_LEN_MAX,
                        szM,
                        sizeof( void* ) * 2,
                        QWORD( (char*)&m + dc.m_dwOffset ),
                        sizeof( M ) );
        DumpMemberValue( CDumpContext( dc.m_iprintf, dc.m_dwOffset, dc.m_cLevel - 1 ), m );

        dc.m_iprintf.Unindent();
    }
}

#define DumpMember( dc, member ) ( DumpMember_( dc, member, #member ) )

template< class M >
inline void DumpMemberValueBF(  const CDumpContext& dc,
                                const M         m )
{
    if ( M( 0 ) - M( 1 ) < M( 0 ) )
    {
        dc.m_iprintf(   "%I64i (0x%0*I64x)\n",
                        QWORD( m ),
                        sizeof( m ) * 2,
                        QWORD( m ) & ( ( QWORD( 1 ) << ( sizeof( m ) * 8 ) ) - 1 ) );
    }
    else
    {
        dc.m_iprintf(   "%I64u (0x%0*I64x)\n",
                        QWORD( m ),
                        sizeof( m ) * 2,
                        QWORD( m ) & ( ( QWORD( 1 ) << ( sizeof( m ) * 8 ) ) - 1 ) );
    }
}

template< class M >
inline void DumpMemberBF_(  const CDumpContext&     dc,
                            const M             m,
                            const CHAR* const   szM )
{
    if ( dc.m_cLevel > 0 )
    {
        dc.m_iprintf.Indent();

        dc.m_iprintf(   "%*.*s <%-*.*s>:  ",
                        SYMBOL_LEN_MAX,
                        SYMBOL_LEN_MAX,
                        szM,
                        2 + sizeof( void* ) * 2 + 1 + 3,
                        2 + sizeof( void* ) * 2 + 1 + 3,
                        "Bit-Field" );
        DumpMemberValueBF( CDumpContext( dc.m_iprintf, dc.m_dwOffset, dc.m_cLevel - 1 ), m );

        dc.m_iprintf.Unindent();
    }
}

#define DumpMemberBF( dc, member ) ( DumpMemberBF_( dc, member, #member ) )


LOCAL BOOL      fInit               = fFalse;   //  debugger extensions have geen initialized
LOCAL ULONG     g_cSymInitFail        = 0;        //  number of times attempts to initialise the symbols sub-system failed
const ULONG     cSymInitAttemptsMax = 3;        //  max attempts to initialise the symbols sub-system

//  ================================================================
class CDPrintF : public CPrintF
//  ================================================================
{
    public:
        VOID __cdecl operator()( const char * szFormat, ... )
        {
            va_list arg_ptr;
            va_start( arg_ptr, szFormat );
            g_DebugControl->ControlledOutputVaList( DEBUG_OUTCTL_AMBIENT_TEXT, DEBUG_OUTPUT_NORMAL, szFormat, arg_ptr );
            va_end( arg_ptr );
        }

        static CPrintF& PrintfInstance();

        ~CDPrintF() {}

    private:
        CDPrintF() {}
};

//  ================================================================
CPrintF& CDPrintF::PrintfInstance()
//  ================================================================
{
    static CDPrintF s_dprintf;
    return s_dprintf;
}

CIPrintF g_idprintf( &CDPrintF::PrintfInstance() );


//  ================================================================
class CDUMP
//  ================================================================
{
    public:
        CDUMP() {}

        virtual ~CDUMP() {}
        virtual VOID Dump( PDEBUG_CLIENT, INT, const CHAR * const [] ) = 0;
};


//  ================================================================
template< class _STRUCT>
class CDUMPA : public CDUMP
//  ================================================================
{
    public:
        VOID Dump(
                PDEBUG_CLIENT pdebugClient,
                INT argc,
                const CHAR * const argv[] );
        static CDUMPA   instance;
};

template< class _STRUCT>
CDUMPA<_STRUCT> CDUMPA<_STRUCT>::instance;


//  ****************************************************************
//  PROTOTYPES
//  ****************************************************************

#define DEBUG_EXT( name )                   \
    LOCAL VOID name(                        \
        const PDEBUG_CLIENT pdebugClient,   \
        const INT argc,                     \
        const CHAR * const argv[]  )

DEBUG_EXT( EDBGDump );
DEBUG_EXT( EDBGTest );
#ifdef SYNC_DEADLOCK_DETECTION
DEBUG_EXT( EDBGLocks );
#endif  //  SYNC_DEADLOCK_DETECTION
DEBUG_EXT( EDBGHelp );
DEBUG_EXT( EDBGHelpDump );


//  ****************************************************************
//  COMMAND DISPATCH
//  ****************************************************************


typedef VOID (*EDBGFUNC)(
    const PDEBUG_CLIENT pdebugClient,
    const INT argc,
    const CHAR * const argv[]
    );


//  ================================================================
struct EDBGFUNCMAP
//  ================================================================
{
    const char *    szCommand;
    EDBGFUNC        function;
    const char *    szHelp;
};


//  ================================================================
struct CSYNCDUMPMAP
//  ================================================================
{
    const char *    szCommand;
    CDUMP      *    pcdump;
    const char *    szHelp;
};


//  ================================================================
LOCAL const EDBGFUNCMAP rgfuncmap[] = {
//  ================================================================

{
        "Help",     EDBGHelp,
        "Help                   - Print this help message"
},
{
        "Dump",     EDBGDump,
        "Dump <Object> <Address> [<Depth>|*]\n\t"
        "                       - Dump a given synchronization object's state"
},
#ifdef SYNC_DEADLOCK_DETECTION
{
        "Locks",        EDBGLocks,
        "Locks [<tid>|* [<OSSYNC::g_pclsSyncGlobal>]]\n\t"
        "                       - List all locks owned by the specified thread or by all threads"
},
#endif  //  SYNC_DEADLOCK_DETECTION
{
        "Test",     EDBGTest,
        "Test                   - Test function"
},
};

LOCAL const INT cfuncmap = sizeof( rgfuncmap ) / sizeof( EDBGFUNCMAP );


#define DUMPA(_struct)  { #_struct, &(CDUMPA<_struct>::instance), #_struct " <Address> [<Depth>|*]" }

//  ================================================================
LOCAL const CSYNCDUMPMAP rgcsyncdumpmap[] = {
//  ================================================================

    DUMPA( CAutoResetSignal ),
    DUMPA( CBinaryLock ),
    DUMPA( CCriticalSection ),
    DUMPA( CGate ),
    DUMPA( CKernelSemaphore ),
    DUMPA( CManualResetSignal ),
    DUMPA( CMeteredSection ),
    DUMPA( CNestableCriticalSection ),
    DUMPA( CReaderWriterLock ),
    DUMPA( CSemaphore ),
    DUMPA( CSXWLatch )
};

LOCAL const INT ccsyncdumpmap = sizeof( rgcsyncdumpmap ) / sizeof( rgcsyncdumpmap[0] );


//  ================================================================
LOCAL BOOL FArgumentMatch( const CHAR * const sz, const CHAR * const szCommand )
//  ================================================================
{
    const BOOL fMatch = ( ( strlen( sz ) == strlen( szCommand ) )
            && !( _strnicmp( sz, szCommand, strlen( szCommand ) ) ) );
    return fMatch;
}


//  ================================================================
DEBUG_EXT( EDBGDump )
//  ================================================================
{
    if( argc < 2 )
    {
        EDBGHelpDump( pdebugClient, argc, argv );
        return;
    }

    for( INT icdumpmap = 0; icdumpmap < ccsyncdumpmap; ++icdumpmap )
    {
        if( FArgumentMatch( argv[0], rgcsyncdumpmap[icdumpmap].szCommand ) )
        {
            (rgcsyncdumpmap[icdumpmap].pcdump)->Dump(
                pdebugClient,
                argc - 1, argv + 1 );
            return;
        }
    }
    EDBGHelpDump( pdebugClient, argc, argv );
}


//  ================================================================
DEBUG_EXT( EDBGHelp )
//  ================================================================
{
    for( INT ifuncmap = 0; ifuncmap < cfuncmap; ifuncmap++ )
    {
        dprintf( "\t%s\n", rgfuncmap[ifuncmap].szHelp );
    }
    dprintf( "\n--------------------\n\n" );
}


//  ================================================================
DEBUG_EXT( EDBGHelpDump )
//  ================================================================
{
    dprintf( "Supported objects:\n\n" );
    for( INT icdumpmap = 0; icdumpmap < ccsyncdumpmap; icdumpmap++ )
    {
        dprintf( "\t%s\n", rgcsyncdumpmap[icdumpmap].szHelp );
    }
    dprintf( "\n--------------------\n\n" );
}


//  ================================================================
DEBUG_EXT( EDBGTest )
//  ================================================================
{
    if ( argc >= 1 )
    {
        void* pv;
        if ( FAddressFromGlobal( argv[ 0 ], &pv ) )
        {
            dprintf(    "The address of %s is 0x%0*I64X.\n",
                        argv[ 0 ],
                        sizeof( void* ) * 2,
                        QWORD( pv ) );
        }
        else
        {
            dprintf( "Could not find the symbol.\n" );
        }
    }
    if ( argc >= 2 )
    {
        void* pv;
        if ( FAddressFromSz( argv[ 1 ], &pv ) )
        {
            char        szGlobal[ 1024 ];
            DWORD_PTR   dwOffset;
            if ( FGlobalFromAddress( pv, szGlobal, sizeof( szGlobal ), &dwOffset ) )
            {
                dprintf(    "The symbol closest to 0x%0*I64X is %s+0x%I64X.\n",
                            sizeof( void* ) * 2,
                            QWORD( pv ),
                            szGlobal,
                            QWORD( dwOffset ) );
            }
            else
            {
                dprintf( "Could not map this address to a symbol.\n" );
            }
        }
        else
        {
            dprintf( "That is not a valid address.\n" );
        }
    }

    dprintf( "\n--------------------\n\n" );
}


#ifdef SYNC_DEADLOCK_DETECTION

//  ================================================================
DEBUG_EXT( EDBGLocks )
//  ================================================================
{
    //  load default arguments

    _CLS*   pclsDebuggee                = NULL;
    DWORD   tid                         = 0;
    BOOL    fFoundLock                  = fFalse;
    BOOL    fValidUsage;

    //  load actual arguments

    switch ( argc )
    {
        case 0:
            //  use defaults
            fValidUsage = fTrue;
            break;
        case 1:
            //  thread-id only
            fValidUsage = ( FUlFromSz( argv[0], &tid )
                            || '*' == argv[0][0] );
            break;
        case 2:
            //  thread-id followed by <g_pclsSyncGlobal>
            fValidUsage = ( ( FUlFromSz( argv[0], &tid ) || '*' == argv[0][0] )
                            && FAddressFromSz( argv[1], &pclsDebuggee ) );
            break;
        default:
            fValidUsage = fFalse;
            break;
        }

    if ( !fValidUsage )
    {
        dprintf( "Usage: Locks [<tid>|* [<OSSYNC::g_pclsSyncGlobal>]]\n" );
        return;
    }

    if ( NULL == pclsDebuggee )
    {
        _CLS**  ppclsDebuggee   = NULL;
        if ( FFetchGlobal( "OSSYNC::g_pclsSyncGlobal", &ppclsDebuggee ) )
        {
            pclsDebuggee = *ppclsDebuggee;
            Unfetch( ppclsDebuggee );
        }
        else
        {
            dprintf( "Error: Could not find the global TLS list in the debuggee.\n" );
            return;
        }
    }

    while ( pclsDebuggee )
    {
        _CLS* pcls;
        if ( !FFetchVariable( pclsDebuggee, &pcls ) )
        {
            dprintf( "An error occurred while scanning Thread IDs.\n" );
            break;
        }

        if ( !tid || pcls->dwContextId == tid )
        {
            if ( pcls->cls.plddiLockWait )
            {
                CLockDeadlockDetectionInfo* plddi           = NULL;
                const CLockBasicInfo*       plbi            = NULL;
                const char*                 pszTypeName     = NULL;
                const char*                 pszInstanceName = NULL;

                if (    FFetchVariable( pcls->cls.plddiLockWait, &plddi ) &&
                        FFetchVariable( &plddi->Info(), &plbi ) &&
                        FFetchSz( plbi->SzTypeName(), &pszTypeName ) &&
                        FFetchSz( plbi->SzInstanceName(), &pszInstanceName ) )
                {
                    fFoundLock = fTrue;

                    EDBGPrintfDml(  "TID <link cmd=\"~~[%x]s\">0x%x</link> is a waiter for %s 0x%0*I64X ( \"%s\", %d, %d )",
                                pcls->dwContextId,
                                pcls->dwContextId,
                                pszTypeName,
                                sizeof( void* ) * 2,
                                QWORD( plbi->Instance() ),
                                pszInstanceName,
                                plbi->Rank(),
                                plbi->SubRank() );
                    if ( pcls->cls.groupLockWait != -1 )
                    {
                        dprintf( " as Group %d", pcls->cls.groupLockWait );
                    }
                    dprintf( ".\n" );
                }
                else
                {
                    dprintf(    "An error occurred while reading TLS for Thread ID %d.\n",
                                pcls->dwContextId );
                }

                Unfetch( pszInstanceName );
                Unfetch( pszTypeName );
                Unfetch( plbi );
                Unfetch( plddi );
            }

            COwner* pownerDebuggee;
            pownerDebuggee = pcls->cls.pownerLockHead;

            while ( pownerDebuggee )
            {
                COwner*                     powner          = NULL;
                CLockDeadlockDetectionInfo* plddi           = NULL;
                const CLockBasicInfo*       plbi            = NULL;
                const char*                 pszTypeName     = NULL;
                const char*                 pszInstanceName = NULL;

                if (    FFetchVariable( pownerDebuggee, &powner ) &&
                        FFetchVariable( powner->m_plddiOwned, &plddi ) &&
                        FFetchVariable( &plddi->Info(), &plbi ) &&
                        FFetchSz( plbi->SzTypeName(), &pszTypeName ) &&
                        FFetchSz( plbi->SzInstanceName(), &pszInstanceName ) )

                {
                    fFoundLock = fTrue;

                    EDBGPrintfDml(  "TID <link cmd=\"~~[%x]s\">0x%x</link> is an owner of %s 0x%0*I64X ( \"%s\", %d, %d )",
                                pcls->dwContextId,
                                pcls->dwContextId,
                                pszTypeName,
                                sizeof( void* ) * 2,
                                QWORD( plbi->Instance() ),
                                pszInstanceName,
                                plbi->Rank(),
                                plbi->SubRank() );
                    if ( powner->m_group != -1 )
                    {
                        dprintf( " as Group %d", powner->m_group );
                    }
                    dprintf( ".\n" );
                }
                else
                {
                    dprintf(    "An error occurred while scanning the lock chain for Thread ID %d.\n",
                                pcls->dwContextId );
                }

                pownerDebuggee = powner ? powner->m_pownerLockNext : NULL;

                Unfetch( pszInstanceName );
                Unfetch( pszTypeName );
                Unfetch( plbi );
                Unfetch( plddi );
                Unfetch( powner );
            }
        }

        pclsDebuggee = pcls ? pcls->pclsNext : NULL;

        Unfetch( pcls );
    }

    if ( fFoundLock )
    {
        dprintf( "\n--------------------\n\n" );
    }
    else if ( !tid )
    {
        dprintf( "No thread owns or is waiting for any locks.\n" );
    }
    else
    {
        dprintf( "This thread does not own and is not waiting for any locks.\n" );
    }
}

#endif  //  SYNC_DEADLOCK_DETECTION

//  ================================================================
template< class _STRUCT>
VOID CDUMPA<_STRUCT>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT argc, const CHAR * const argv[]
    )
//  ================================================================
{
    _STRUCT*    ptDebuggee;
    ULONG       cLevelInput;
    if (    argc < 1 ||
            argc > 2 ||
            !FAddressFromSz( argv[ 0 ], &ptDebuggee ) ||
            argc == 2 && !FUlFromSz( argv[ 1 ], &cLevelInput, 10 ) && strcmp( argv[ 1 ], "*" ) )
    {
        dprintf( "Usage: Dump <Object> <Address> [<Depth>|*]\n" );
        return;
    }

    INT cLevel;
    if ( argc == 2 )
    {
        if ( FUlFromSz( argv[ 1 ], &cLevelInput, 10 ) )
        {
            cLevel = INT( cLevelInput );
        }
        else
        {
            cLevel = INT_MAX;
        }
    }
    else
    {
        cLevel = 1;
    }

    _STRUCT* pt;
    if ( FFetchVariable( ptDebuggee, &pt ) )
    {
        dprintf(    "0x%0*I64X bytes @ 0x%0*I64X\n",
                    sizeof( size_t ) * 2,
                    QWORD( sizeof( _STRUCT ) ),
                    sizeof( _STRUCT* ) * 2,
                    QWORD( ptDebuggee ) );

        pt->Dump( CDumpContext( g_idprintf, (BYTE*)ptDebuggee - (BYTE*)pt, cLevel ) );

        Unfetch( pt );
    }
    else
    {
        dprintf( "Error: Could not fetch the requested object.\n" );
    }
}


VOID OSSYNCAPI OSSyncDebuggerExtension(
                                        PDEBUG_CLIENT pdebugClient,
                                        const INT argc,
                                        const CHAR * const argv[] )
{

    if ( ( g_DebugClient == NULL  || g_DebugControl == NULL ) && g_cSymInitFail < cSymInitAttemptsMax )
    {
        //  get our containing image name so that we can look up our variables
        //
        fInit = FOSEdbgCreateDebuggerInterface( pdebugClient );

        if ( fInit )
        {
            dprintf( "SYNC symbols sub-system successfully initialised.\n" );
        }
        else
        {
            dprintf( "WARNING: Failed initialisation of SYNC symbols sub-system.\n" );
            if ( ++g_cSymInitFail >= cSymInitAttemptsMax )
                dprintf( "WARNING: No further attempts will be made to initialise the SYNC symbols sub-system.\n" );
        }
    }

    if( argc < 1 )
    {
        EDBGHelp( pdebugClient, argc, (const CHAR **)argv );
        return;
    }

    INT ifuncmap;
    for( ifuncmap = 0; ifuncmap < cfuncmap; ifuncmap++ )
    {
        if( FArgumentMatch( argv[0], rgfuncmap[ifuncmap].szCommand ) )
        {
            (rgfuncmap[ifuncmap].function)(
                pdebugClient,
                argc - 1, (const CHAR **)( argv + 1 ) );
            return;
        }
    }
    EDBGHelp( pdebugClient, argc, (const CHAR **)argv );
}


//  custom member dump functions

template<>
inline void DumpMemberValue< const char* >( const CDumpContext&     dc,
                                            const char* const & sz )
{
    const char* szFetch = NULL;

    dc.m_iprintf( "0x%0*I64x ", sizeof( sz ) * 2, QWORD( sz ) );
    if ( FFetchSz( sz, &szFetch ) )
    {
        dc.m_iprintf( "\"%s\"", szFetch );
    }
    dc.m_iprintf( "\n" );

    Unfetch( szFetch );
}

template<>
inline void DumpMemberValue< CSemaphore >(  const CDumpContext&     dc,
                                            const CSemaphore&   sem )
{
    dc.m_iprintf( "\n" );
    sem.Dump( dc );
}


//  Enhanced Synchronization Object State Container Dump Support

template< class CState, class CStateInit, class CInformation, class CInformationInit >
void CEnhancedStateContainer< CState, CStateInit, CInformation, CInformationInit >::
Dump( const CDumpContext& dc ) const
{
#ifdef SYNC_ENHANCED_STATE

    CEnhancedState* pes = NULL;

    if ( FFetchVariable( m_pes, &pes ) )
    {
        CDumpContext dcES( dc.m_iprintf, (BYTE*)m_pes - (BYTE*)pes, dc.m_cLevel );

        pes->CState::Dump( dcES );
        pes->CInformation::Dump( dcES );
    }
    else
    {
        dprintf( "Error: Could not fetch the enhanced state of the requested object.\n" );
    }

    Unfetch( pes );

#else  //  !SYNC_ENHANCED_STATE

    CEnhancedState* pes = (CEnhancedState*) m_rgbState;

    pes->CState::Dump( dc );
    pes->CInformation::Dump( dc );

#endif  //  SYNC_ENHANCED_STATE
}


//  Synchronization Object Basic Information Dump Support

void CSyncBasicInfo::Dump( const CDumpContext& dc ) const
{
#ifdef SYNC_ENHANCED_STATE

    DumpMember( dc, m_szInstanceName );
    DumpMember( dc, m_szTypeName );
    DumpMember( dc, m_psyncobj );

#endif  //  SYNC_ENHANCED_STATE
}


//  Synchronization Object Performance Dump Support

void CSyncPerfWait::Dump( const CDumpContext& dc ) const
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    DumpMember( dc, m_cWait );
    DumpMember( dc, m_qwHRTWaitElapsed );

#endif  //  SYNC_ANALYZE_PERFORMANCE
}

void CSyncPerfAcquire::Dump( const CDumpContext& dc ) const
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    DumpMember( dc, m_cAcquire );
    DumpMember( dc, m_cContend );

#endif  //  SYNC_ANALYZE_PERFORMANCE
}


//  Lock Object Basic Information Dump Support

void CLockBasicInfo::Dump( const CDumpContext& dc ) const
{
    CSyncBasicInfo::Dump( dc );

#ifdef SYNC_DEADLOCK_DETECTION

    DumpMember( dc, m_rank );
    DumpMember( dc, m_subrank );

#endif  //  SYNC_DEADLOCK_DETECTION
}


//  Lock Object Performance Dump Support

void CLockPerfHold::Dump( const CDumpContext& dc ) const
{
#ifdef SYNC_ANALYZE_PERFORMANCE

    DumpMember( dc, m_cHold );
    DumpMember( dc, m_qwHRTHoldElapsed );

#endif  //  SYNC_ANALYZE_PERFORMANCE
}


//  Lock Object Deadlock Detection Information Dump Support

void CLockDeadlockDetectionInfo::Dump( const CDumpContext& dc ) const
{
#ifdef SYNC_DEADLOCK_DETECTION

    DumpMember( dc, m_plbiParent );
    DumpMember( dc, m_semOwnerList );
    DumpMember( dc, m_ownerHead );

#endif  //  SYNC_DEADLOCK_DETECTION
}


//  Kernel Semaphore Dump Support

void CKernelSemaphoreState::Dump( const CDumpContext& dc ) const
{
    DumpMember( dc, m_handle );
}

void CKernelSemaphoreInfo::Dump( const CDumpContext& dc ) const
{
    CSyncBasicInfo::Dump( dc );
    CSyncPerfWait::Dump( dc );
}

void CKernelSemaphore::Dump( const CDumpContext& dc ) const
{
    CEnhancedStateContainer< CKernelSemaphoreState, CSyncStateInitNull, CKernelSemaphoreInfo, CSyncBasicInfo >::Dump( dc );
}


//  Semaphore Dump Support

void CSemaphoreState::Dump( const CDumpContext& dc ) const
{
    DumpMember( dc, m_cAvail );
    DumpMember( dc, m_cWait );
}

void CSemaphoreInfo::Dump( const CDumpContext& dc ) const
{
    CSyncBasicInfo::Dump( dc );
    CSyncPerfWait::Dump( dc );
    CSyncPerfAcquire::Dump( dc );
}

void CSemaphore::Dump( const CDumpContext& dc ) const
{
    CEnhancedStateContainer< CSemaphoreState, CSyncStateInitNull, CSemaphoreInfo, CSyncBasicInfo >::Dump( dc );
}


//  Auto-Reset Signal Dump Support

void CAutoResetSignalState::Dump( const CDumpContext& dc ) const
{
    if ( FNoWait() )
    {
        DumpMember( dc, m_fSet );
    }
    else
    {
        DumpMember( dc, m_irksem );
        DumpMember( dc, m_cWaitNeg );
    }
}

void CAutoResetSignalInfo::Dump( const CDumpContext& dc ) const
{
    CSyncBasicInfo::Dump( dc );
    CSyncPerfWait::Dump( dc );
    CSyncPerfAcquire::Dump( dc );
}

void CAutoResetSignal::Dump( const CDumpContext& dc ) const
{
    CEnhancedStateContainer< CAutoResetSignalState, CSyncStateInitNull, CAutoResetSignalInfo, CSyncBasicInfo >::Dump( dc );
}


//  Manual-Reset Signal Dump Support

void CManualResetSignalState::Dump( const CDumpContext& dc ) const
{
    if ( FNoWait() )
    {
        DumpMember( dc, m_fSet );
    }
    else
    {
        DumpMember( dc, m_irksem );
        DumpMember( dc, m_cWaitNeg );
    }
}

void CManualResetSignalInfo::Dump( const CDumpContext& dc ) const
{
    CSyncBasicInfo::Dump( dc );
    CSyncPerfWait::Dump( dc );
    CSyncPerfAcquire::Dump( dc );
}

void CManualResetSignal::Dump( const CDumpContext& dc ) const
{
    CEnhancedStateContainer< CManualResetSignalState, CSyncStateInitNull, CManualResetSignalInfo, CSyncBasicInfo >::Dump( dc );
}


//  Critical Section (non-nestable) Dump Support

void CCriticalSectionState::Dump( const CDumpContext& dc ) const
{
    DumpMember( dc, m_sem );
}

void CCriticalSectionInfo::Dump( const CDumpContext& dc ) const
{
    CLockBasicInfo::Dump( dc );
    CLockPerfHold::Dump( dc );
    CLockDeadlockDetectionInfo::Dump( dc );
}

void CCriticalSection::Dump( const CDumpContext& dc ) const
{
    CEnhancedStateContainer< CCriticalSectionState, CSyncBasicInfo, CCriticalSectionInfo, CLockBasicInfo >::Dump( dc );
}


//  Nestable Critical Section Dump Support

void CNestableCriticalSectionState::Dump( const CDumpContext& dc ) const
{
    DumpMember( dc, m_sem );
    DumpMember( dc, m_pclsOwner );
    DumpMember( dc, m_cEntry );
}

void CNestableCriticalSectionInfo::Dump( const CDumpContext& dc ) const
{
    CLockBasicInfo::Dump( dc );
    CLockPerfHold::Dump( dc );
    CLockDeadlockDetectionInfo::Dump( dc );
}

void CNestableCriticalSection::Dump( const CDumpContext& dc ) const
{
    CEnhancedStateContainer< CNestableCriticalSectionState, CSyncBasicInfo, CNestableCriticalSectionInfo, CLockBasicInfo >::Dump( dc );
}


//  Gate Dump Support

void CGateState::Dump( const CDumpContext& dc ) const
{
    DumpMember( dc, m_cWait );
    DumpMember( dc, m_irksem );
}

void CGateInfo::Dump( const CDumpContext& dc ) const
{
    CSyncBasicInfo::Dump( dc );
    CSyncPerfWait::Dump( dc );
}

void CGate::Dump( const CDumpContext& dc ) const
{
    CEnhancedStateContainer< CGateState, CSyncStateInitNull, CGateInfo, CSyncBasicInfo >::Dump( dc );
}


//  Binary Lock Dump Support

void CBinaryLockState::Dump( const CDumpContext& dc ) const
{
    DumpMember( dc, m_cw );
    DumpMemberBF( dc, m_cOOW1 );
    DumpMemberBF( dc, m_fQ1 );
    DumpMemberBF( dc, m_cOOW2 );
    DumpMemberBF( dc, m_fQ2 );
    DumpMember( dc, m_cOwner );
    DumpMember( dc, m_sem1 );
    DumpMember( dc, m_sem2 );
}

void CBinaryLockPerfInfo::Dump( const CDumpContext& dc ) const
{
    CSyncPerfWait::Dump( dc );
    CSyncPerfAcquire::Dump( dc );
    CLockPerfHold::Dump( dc );
}

void CBinaryLockGroupInfo::Dump( const CDumpContext& dc ) const
{
    for ( INT iGroup = 0; iGroup < 2; iGroup++ )
    {
        m_rginfo[iGroup].Dump( dc );
    }
}

void CBinaryLockInfo::Dump( const CDumpContext& dc ) const
{
    CLockBasicInfo::Dump( dc );
    CBinaryLockGroupInfo::Dump( dc );
    CLockDeadlockDetectionInfo::Dump( dc );
}

void CBinaryLock::Dump( const CDumpContext& dc ) const
{
    CEnhancedStateContainer< CBinaryLockState, CSyncBasicInfo, CBinaryLockInfo, CLockBasicInfo >::Dump( dc );
}


//  Reader / Writer Lock Dump Support

void CReaderWriterLockState::Dump( const CDumpContext& dc ) const
{
    DumpMember( dc, m_cw );
    DumpMemberBF( dc, m_cOAOWW );
    DumpMemberBF( dc, m_fQW );
    DumpMemberBF( dc, m_cOOWR );
    DumpMemberBF( dc, m_fQR );
    DumpMember( dc, m_cOwner );
    DumpMember( dc, m_semWriter );
    DumpMember( dc, m_semReader );
}

void CReaderWriterLockPerfInfo::Dump( const CDumpContext& dc ) const
{
    CSyncPerfWait::Dump( dc );
    CSyncPerfAcquire::Dump( dc );
    CLockPerfHold::Dump( dc );
}

void CReaderWriterLockGroupInfo::Dump( const CDumpContext& dc ) const
{
    for ( INT iGroup = 0; iGroup < 2; iGroup++ )
    {
        m_rginfo[iGroup].Dump( dc );
    }
}

void CReaderWriterLockInfo::Dump( const CDumpContext& dc ) const
{
    CLockBasicInfo::Dump( dc );
    CReaderWriterLockGroupInfo::Dump( dc );
    CLockDeadlockDetectionInfo::Dump( dc );
}

void CReaderWriterLock::Dump( const CDumpContext& dc ) const
{
    CEnhancedStateContainer< CReaderWriterLockState, CSyncBasicInfo, CReaderWriterLockInfo, CLockBasicInfo >::Dump( dc );
}


//  Metered Section Dump Support

void CMeteredSection::Dump( const CDumpContext& dc ) const
{
    DumpMember( dc, m_pfnPartitionComplete );
    DumpMember( dc, m_dwPartitionCompleteKey );

    DumpMember( dc, m_cw );
    DumpMemberBF( dc, m_cCurrent );
    DumpMemberBF( dc, m_groupCurrent );
    DumpMemberBF( dc, m_cQuiesced );
    DumpMemberBF( dc, m_groupQuiesced );
}


//  S / X / W Latch Dump Support

void CSXWLatchState::Dump( const CDumpContext& dc ) const
{
    DumpMember( dc, m_cw );
    DumpMemberBF( dc, m_cOAWX );
    DumpMemberBF( dc, m_cOOWS );
    DumpMemberBF( dc, m_fQS );
    DumpMemberBF( dc, m_cQS );
    DumpMember( dc, m_semS );
    DumpMember( dc, m_semX );
    DumpMember( dc, m_semW );
}

void CSXWLatchPerfInfo::Dump( const CDumpContext& dc ) const
{
    CSyncPerfWait::Dump( dc );
    CSyncPerfAcquire::Dump( dc );
    CLockPerfHold::Dump( dc );
}

void CSXWLatchGroupInfo::Dump( const CDumpContext& dc ) const
{
    for ( INT iGroup = 0; iGroup < 3; iGroup++ )
    {
        m_rginfo[iGroup].Dump( dc );
    }
}

void CSXWLatchInfo::Dump( const CDumpContext& dc ) const
{
    CLockBasicInfo::Dump( dc );
    CSXWLatchGroupInfo::Dump( dc );
    CLockDeadlockDetectionInfo::Dump( dc );
}

void CSXWLatch::Dump( const CDumpContext& dc ) const
{
    CEnhancedStateContainer< CSXWLatchState, CSyncBasicInfo, CSXWLatchInfo, CLockBasicInfo >::Dump( dc );
}

#endif  //  DEBUGGER_EXTENSION

//  Sync Stats support

CIPrintF*       g_piprintfPerfData;
CFPrintF*       g_pfprintfPerfData;

//  dump column descriptors

void OSSyncStatsDumpColumns()
{
    (*g_piprintfPerfData)(    "Type Name\t"
                            "Instance Name\t"
                            "Instance ID\t"
                            "Group ID\t"
                            "Wait Count\t"
                            "Wait Time Elapsed\t"
                            "Acquire Count\t"
                            "Contention Count\t"
                            "Hold Count\t"
                            "Hold Time Elapsed\r\n" );
}

//  dump cooked stats from provided raw stats

void OSSyncStatsDump(   const char*         szTypeName,
                        const char*         szInstanceName,
                        const CSyncObject*  psyncobj,
                        DWORD               group,
                        QWORD               cWait,
                        double              csecWaitElapsed,
                        QWORD               cAcquire,
                        QWORD               cContend,
                        QWORD               cHold,
                        double              csecHoldElapsed )
{
    //  dump data, if interesting

    if ( cWait || cAcquire || cContend || cHold )
    {
        (*g_piprintfPerfData)(    "\"%s\"\t"
                                "\"%s\"\t"
                                "\"0x%0*I64X\"\t"
                                "%d\t"
                                "%I64d\t"
                                "%f\t"
                                "%I64d\t"
                                "%I64d\t"
                                "%I64d\t"
                                "%f\r\n",
                                szTypeName,
                                szInstanceName,
                                sizeof(LONG_PTR) * 2,           //  need 2 hex digits for each byte
                                QWORD( LONG_PTR( psyncobj ) ),  //  assumes QWORD is largest pointer size we'll ever use
                                group,
                                cWait,
                                csecWaitElapsed,
                                cAcquire,
                                cContend,
                                cHold,
                                csecHoldElapsed );
    }
}

//  CInitTermLock init/term in progress convergence wait

void CInitTermLock::SleepAwayQuanta()
{
    Sleep( 0 );
}

volatile DWORD g_cOSSyncInit;  //  assumed init to 0 by the loader

//  cleanup sync subsystem for term (or error on init)

static void OSSYNCAPI OSSyncICleanup()
{
    OSSYNCAssert( (LONG)g_cOSSyncInit == lOSSyncLockedForTerm
                || (LONG)g_cOSSyncInit == lOSSyncLockedForInit );

    //  free PLS

    FOSSyncConfigureProcessorLocalStorage( 0 );

#ifdef SYNC_DUMP_PERF_DATA

    //  terminate performance data dump context

    if ( NULL != g_piprintfPerfData )
    {
        ((CIPrintF*)g_piprintfPerfData)->~CIPrintF();
        LocalFree( g_piprintfPerfData );
        g_piprintfPerfData = NULL;
    }

    if ( NULL != g_piprintfPerfData )
    {
        ((CFPrintF*)g_pfprintfPerfData)->~CFPrintF();
        LocalFree( g_pfprintfPerfData );
        g_pfprintfPerfData = NULL;
    }

#endif  //  SYNC_DUMP_PERF_DATA

    //  terminate the Kernel Semaphore Pool

    if ( g_ksempoolGlobal.FInitialized() )
    {
        g_ksempoolGlobal.Term();
    }

#ifdef SYNC_ENHANCED_STATE

    //  terminate the CEnhancedState allocator

    OSSYNCAssert( g_pmbRoot == &g_mbSentry );
    MemoryBlock* pmb = g_pmbRootFree;
    while ( pmb != &g_mbSentryFree )
    {
        MemoryBlock* const pmbT = pmb;
        pmb = pmb->pmbNext;
        BOOL fMemFreed = VirtualFree( pmbT, 0, MEM_RELEASE );
        OSSYNCAssert( fMemFreed );
    }
    if ( g_fcsESMemoryInit )
    {
        DeleteCriticalSection( &g_csESMemory );
        g_fcsESMemoryInit = fFalse;
    }

#endif

    if ( g_fcsClsSyncGlobalInit )
    {
        DeleteCriticalSection( &g_csClsSyncGlobal );
        g_fcsClsSyncGlobalInit = fFalse;
    }

    //  unlock sync subsystem

    OSSYNCAssert( (LONG)g_cOSSyncInit == lOSSyncLockedForTerm
                || (LONG)g_cOSSyncInit == lOSSyncLockedForInit );
    AtomicExchange( (LONG*)&g_cOSSyncInit, 0 );
}


//  init sync subsystem

static BOOL FOSSyncIInit()
{
    BOOL    fInitRequired   = fFalse;

    OSSYNC_FOREVER
    {
        const LONG  lInitial    = ( g_cOSSyncInit & lOSSyncUnlocked );
        const LONG  lFinal      = ( 0 == lInitial ? lOSSyncLockedForInit : lInitial + 1 );

        //  if multiple threads try to init/term simultaneously, one will win
        //  and the others will loop until he's done

        if ( lInitial == AtomicCompareExchange( (LONG*)&g_cOSSyncInit, lInitial, lFinal ) )
        {
            fInitRequired = ( 0 == lInitial );
            break;
        }
        else if ( g_cOSSyncInit & lOSSyncLocked )
        {
            //  our AtomicCompareExchange() appears to have failed because
            //  the sync subsystem is locked for init/term
            //  this function is called everytime we call ESMemoryNew(),
            //  so just give up our timeslice instead of actually sleep
            Sleep( 0 );
        }
    }

    if ( fInitRequired )
    {
        //  reset all pointers

        g_piprintfPerfData    = NULL;
        g_pfprintfPerfData    = NULL;

        //  reset global CLS list

        if ( !InitializeCriticalSectionAndSpinCount( &g_csClsSyncGlobal, 0 ) )
        {
            goto HandleError;
        }
        g_fcsClsSyncGlobalInit = fTrue;
        g_pclsSyncGlobal      = NULL;
        g_pclsSyncCleanGlobal = NULL;
        g_cclsSyncGlobal      = 0;
        g_dwClsSyncIndex      = dwClsInvalid;
        g_dwClsProcIndex      = dwClsInvalid;

        const BOOL fCreateSemaphorePfns = FKernelSemaphoreICreateILoad();
        OSSYNCAssert( fCreateSemaphorePfns );

        if ( !fCreateSemaphorePfns )
        {
            goto HandleError;
        }

#ifdef SYNC_ENHANCED_STATE

        //  initialize the CEnhancedState allocator

        SYSTEM_INFO sinf;
        GetSystemInfo( &sinf );
        g_cbMemoryBlock = sinf.dwAllocationGranularity;
        g_pmbRoot = &g_mbSentry;
        g_mbSentry.pmbNext = NULL;
        g_mbSentry.pmbPrev = NULL;
        g_mbSentry.ibFreeMic = g_cbMemoryBlock;
        g_mbSentry.ibFreeMicPrev = 0;
        g_pmbRootFree = &g_mbSentryFree;
        g_mbSentryFree.pmbNext = NULL;
        g_mbSentryFree.pmbPrev = NULL;
        g_mbSentryFree.ibFreeMic = g_cbMemoryBlock;
        g_mbSentryFree.ibFreeMicPrev = 0;
        if ( !InitializeCriticalSectionAndSpinCount( &g_csESMemory, 0 ) )
        {
            goto HandleError;
        }
        g_fcsESMemoryInit = fTrue;

#endif

        //  initialize the Kernel Semaphore Pool

        if ( !g_ksempoolGlobal.FInit() )
        {
            goto HandleError;
        }

        //  cache the processor count

#ifdef MINIMAL_FUNCTIONALITY
        g_cProcessor = 1;
        g_cProcessorMax = 1;
#else
        DWORD_PTR maskProcess;
        DWORD_PTR maskSystem;
        BOOL fGotAffinityMask;
        fGotAffinityMask = GetProcessAffinityMask(  GetCurrentProcess(),
                                                    &maskProcess,
                                                    &maskSystem );
        OSSYNCAssert( fGotAffinityMask );

        for ( g_cProcessor = 0; maskProcess != 0; maskProcess >>= 1 )
        {
            if ( maskProcess & 1 )
            {
                g_cProcessor++;
            }
        }

        for ( g_cProcessorMax = 0; maskSystem != 0; maskSystem >>= 1 )
        {
            if ( maskSystem & 1 )
            {
                g_cProcessorMax++;
            }
        }
#endif

        //  cache system max spin count
        //
        //  NOTE:  spins heavily as WaitForSingleObject() is extremely expensive
        //
        //  CONSIDER:  get spin count from persistent configuration

        g_cSpinMax = g_cProcessor == 1 ? 0 : 256;

#ifdef SYNC_DUMP_PERF_DATA

        //  initialize performance data dump context

        char szTempPath[_MAX_PATH];
        if ( !GetTempPath( _MAX_PATH, szTempPath ) )
        {
            goto HandleError;
        }

        char szProcessPath[_MAX_PATH];
        char szProcess[_MAX_PATH];
        if ( !GetModuleFileName( NULL, szProcessPath, _MAX_PATH ) )
        {
            goto HandleError;
        }
        _splitpath( szProcessPath, NULL, NULL, szProcess, NULL );

        MEMORY_BASIC_INFORMATION mbi;
        if ( !VirtualQueryEx( GetCurrentProcess(), FOSSyncIInit, &mbi, sizeof( mbi ) ) )
        {
            goto HandleError;
        }
        char szModulePath[_MAX_PATH];
        char szModule[_MAX_PATH];
        if ( !GetModuleFileName( HINSTANCE( mbi.AllocationBase ), szModulePath, sizeof( szModulePath )/sizeof( szModulePath[0]  ) ) )
        {
            goto HandleError;
        }
        _splitpath( szModulePath, NULL, NULL, szModule, NULL );

        SYSTEMTIME systemtime;
        GetLocalTime( &systemtime );

        const size_t cchPerfData = _MAX_PATH;
        char szPerfData[cchPerfData];
        sprintf_s(  szPerfData,
                    cchPerfData,
                    "%s%s %s Sync Stats %04d%02d%02d%02d%02d%02d.TXT",
                    szTempPath,
                    szProcess,
                    szModule,
                    systemtime.wYear,
                    systemtime.wMonth,
                    systemtime.wDay,
                    systemtime.wHour,
                    systemtime.wMinute,
                    systemtime.wSecond );

        if ( !( g_pfprintfPerfData = (CFPrintF*)LocalAlloc( 0, sizeof( CFPrintF ) ) ) )
        {
            goto HandleError;
        }
        new( (CFPrintF*)g_pfprintfPerfData ) CFPrintF( szPerfData );

        if ( !( g_piprintfPerfData = (CIPrintF*)LocalAlloc( 0, sizeof( CIPrintF ) ) ) )
        {
            goto HandleError;
        }
        new( (CIPrintF*)g_piprintfPerfData ) CIPrintF( g_pfprintfPerfData );

        OSSyncStatsDumpColumns();

#endif  //  SYNC_DUMP_PERF_DATA


        //  unlock sync subsystem

        OSSYNCAssert( (LONG)g_cOSSyncInit == lOSSyncLockedForInit );
        AtomicExchange( (LONG*)&g_cOSSyncInit, 1 );
    }

    return fTrue;

HandleError:
    OSSyncICleanup();
    return fFalse;
}

BOOL OSSYNCAPI FOSSyncPreinit()
{
    //  make sure we are initialized and
    //  add an initial ref for the process

    g_fSyncProcessAbort = fFalse;
    
    return FOSSyncIInit();
}

//  enable/disable timeout deadlock detection

void OSSYNCAPI OSSyncConfigDeadlockTimeoutDetection( const BOOL fEnable )
{
    g_sdltosState = fEnable ? sdltosEnabled : sdltosDisabled;
}

//  terminate sync subsystem

static void OSSyncITerm()
{
    OSSYNC_FOREVER
    {
        const LONG  lInitial        = ( g_cOSSyncInit & lOSSyncUnlocked );
        if ( 0 == lInitial )
        {
            //  sync subsystem not initialised
            //  (or someone else is already
            //  terminating it)
            break;
        }

#ifdef SYNC_ENHANCED_STATE
        const BOOL  fTermRequired   = ( g_ksempoolGlobal.CksemAlloc() + 1 == lInitial );
#else
        const BOOL  fTermRequired   = ( 1 == lInitial );
#endif
        const LONG  lFinal          = ( fTermRequired ? lOSSyncLockedForTerm : lInitial - 1 );

        //  if multiple threads try to init/term simultaneously, one will win
        //  and the others will loop until he's done

        if ( lInitial == AtomicCompareExchange( (LONG*)&g_cOSSyncInit, lInitial, lFinal ) )
        {
            if ( fTermRequired )
            {
                OSSyncICleanup();
            }
            break;
        }
        else if ( g_cOSSyncInit & lOSSyncLocked )
        {
            //  our AtomicCompareExchange() appears to have failed because
            //  the sync subsystem is locked for init/term
            //  this function is called everytime we call ESMemoryDelete(),
            //  so just give up our timeslice instead of actually sleep
            Sleep( 0 );
        }
    }
}

void OSSYNCAPI OSSyncProcessAbort()
{
    g_fSyncProcessAbort = fTrue;
}

void OSSYNCAPI OSSyncPostterm()
{
    //  purge our CLS list

    while ( NULL != g_pclsSyncGlobal )
    {
        OSSyncIClsFree( g_pclsSyncGlobal );
    }

    //  on a normal shutdown, there should be exactly 1 refcount left,
    //  but there may also be none if this is being called as
    //  part of error-handling during init
#ifdef SYNC_ENHANCED_STATE
#else  //  !SYNC_ENHANCED_STATE
    OSSYNCAssert( g_fSyncProcessAbort || 0 == g_cOSSyncInit || 1 == g_cOSSyncInit );
#endif  //  SYNC_ENHANCED_STATE
    OSSyncITerm();
}



const BOOL OSSYNCAPI FOSSyncInitForES()
{
#ifdef SYNC_ENHANCED_STATE
    return FOSSyncIInit();
#else
    return fTrue;
#endif
}

void OSSYNCAPI OSSyncTermForES()
{
#ifdef SYNC_ENHANCED_STATE
    if ( lOSSyncLockedForTerm == g_cOSSyncInit )
    {
        //  SPECIAL CASE: we're in the middle of
        //  terminating the subsystem and end up
        //  recursively calling OSSyncTermForES
        //  when we clean up g_ksempoolGlobal
        return;
    }

    OSSyncITerm();
#endif
}

}; // namespace OSSYNC

