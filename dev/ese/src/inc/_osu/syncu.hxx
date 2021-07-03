// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSU_SYNC_HXX_INCLUDED
#define _OSU_SYNC_HXX_INCLUDED


class AGENT
{
    public:
        AGENT() : m_cWait( 0 ), m_fAgentReleased( fFalse ), m_irksem( CKernelSemaphorePool::irksemNil )
        {
            Assert( IsAtomicallyModifiable( (LONG*)&m_dw ) );
        }
        ~AGENT() {}
        AGENT& operator=( AGENT& );  //  disallowed

        void    Wait( CCriticalSection& crit );
        void    Release( CCriticalSection& crit );
        DWORD   CWait() const;

    private:
        union
        {
            volatile LONG        m_dw;       // must be DWORD-aligned
            struct
            {
                volatile LONG    m_cWait:31;
                volatile FLAG32  m_fAgentReleased:1; // agent may only be released once
            };
        };
        
        CKernelSemaphorePool::IRKSEM    m_irksem;
        BYTE                            m_rgbFiller[2];
};

INLINE void AGENT::Wait( CCriticalSection& crit )
{
    //  must be in critical section

    Assert( crit.FOwner() );

    //  should never attempt to wait on an agent that's been freed.
    Assert( !m_fAgentReleased );

    //  no one is waiting yet

    if ( 0 == CWait() )
    {
        //  allocate a semaphore to wait on
        Assert( CKernelSemaphorePool::irksemNil == m_irksem );
        m_irksem = g_ksempoolGlobal.Allocate( (const CLockObject* const) &m_irksem );
    }

    //  one more thread is waiting on this gate

    Assert( CWait() + 1 < 0x8000 );
    m_cWait++;

    Assert( CKernelSemaphorePool::irksemNil != m_irksem );

    //  wait until released

    crit.Leave();
    
    g_ksempoolGlobal.Ksem( m_irksem, (const CLockObject* const) &m_irksem ).Acquire();

    Assert( m_fAgentReleased );

    Assert( CWait() > 0 );
    AtomicDecrement( (LONG*)&m_dw );

    // UNDONE: If caller doesn't need to reenter critical section,
    // then we shouldn't bother.  Right now, we force reentry
    // only for the sake of coding elegance -- since we had the
    // critical section coming into this function, we should
    // also have it when leaving this function.
    crit.Enter();
}

INLINE void AGENT::Release( CCriticalSection& crit )
{
    //  must be in critical section

    Assert( crit.FOwner() );

    Assert( !m_fAgentReleased );    // agent may only be released once
    m_fAgentReleased = fTrue;

    //  free anyone waiting for agent to finish
    if ( CWait() > 0 )
    {
        Assert( CKernelSemaphorePool::irksemNil != m_irksem );
        g_ksempoolGlobal.Ksem( m_irksem, (const CLockObject* const) &m_irksem ).Release( CWait() );
        while ( CWait() > 0 )
        {
            // Ensure all waiters get released.  Must stay
            // in the critical section while looping to
            // ensure no new waiters come in.
            UtilSleep( 10 );
        }
        
        //  free the semaphore
        g_ksempoolGlobal.Unreference( m_irksem );
        m_irksem = CKernelSemaphorePool::irksemNil;
    }

    Assert( m_fAgentReleased );
    Assert( CWait() == 0 );
    Assert( CKernelSemaphorePool::irksemNil == m_irksem );
}
    
INLINE DWORD AGENT::CWait() const
{
    Assert( m_cWait >= 0 );
    return m_cWait;
}


//  Critical Section Use Convenience Class

//  ================================================================
class ENTERCRITICALSECTION
//  ================================================================
//
//  Constructor and destructor enter and leave the given critical
//  section. The optional constructor argument allows the critical
//  section not to be entered if necessary
//
//-
{
    private:
        CCriticalSection* const m_pcrit;
        
    public:
        ENTERCRITICALSECTION( CCriticalSection* const pcrit, const BOOL fEnter = fTrue )
            :   m_pcrit( fEnter ? pcrit : NULL )
        {
            if ( m_pcrit )
            {
                m_pcrit->Enter();
            }
        }
        
        ~ENTERCRITICALSECTION()
        {
            if ( m_pcrit )
            {
                m_pcrit->Leave();
            }
        }

    private:
        ENTERCRITICALSECTION();
        ENTERCRITICALSECTION( const ENTERCRITICALSECTION& );
        ENTERCRITICALSECTION& operator=( const ENTERCRITICALSECTION& );
};

//  Reader Writer Lock Use Convenience Class

//  ================================================================
class ENTERREADERWRITERLOCK
//  ================================================================
//
//  Constructor and destructor enter and leave the given reader
//  writer lock. The optional constructor argument allows the reader
//  writer lock not to be entered if necessary
//
//-
{
    private:
        CReaderWriterLock* const    m_prwl;
        const BOOL                  m_fReader;

    public:
        ENTERREADERWRITERLOCK( CReaderWriterLock* const prwl, const BOOL fReader, const BOOL fEnter = fTrue )
            : m_prwl( fEnter ? prwl : NULL ), m_fReader( fReader )
        {
            if ( m_prwl )
            {
                if ( m_fReader )
                {
                    m_prwl->EnterAsReader();
                }
                else
                {
                    m_prwl->EnterAsWriter();
                }
            }
        }

        ~ENTERREADERWRITERLOCK()
        {
            if ( m_prwl )
            {
                if ( m_fReader )
                {
                    m_prwl->LeaveAsReader();
                }
                else
                {
                    m_prwl->LeaveAsWriter();
                }
            }
        }

    private:
        ENTERREADERWRITERLOCK();
        ENTERREADERWRITERLOCK( const ENTERREADERWRITERLOCK& );
        ENTERREADERWRITERLOCK& operator=( const ENTERREADERWRITERLOCK& );
};


//  Critical Section Pool

template<class T>
class CRITPOOL
{
    public:
        CRITPOOL( void );
        ~CRITPOOL( void );
        CRITPOOL& operator=( CRITPOOL& );  //  disallowed

        BOOL FInit( const LONG cThread, const INT rank, const _TCHAR* szName );
        void Term( void );

        CCriticalSection& Crit( const T* const pt );

    private:
        LONG                m_ccrit;
        CCriticalSection*   m_rgcrit;
        LONG                m_cShift;
        LONG                m_mask;
};
    
//  constructor

template<class T>
CRITPOOL<T>::CRITPOOL( void )
{
    //  reset all members

    m_ccrit = 0;
    m_rgcrit = NULL;
    m_cShift = 0;
    m_mask = 0;
}

//  destructor

template<class T>
CRITPOOL<T>::~CRITPOOL( void )
{
    //  ensure that we have freed our resources

    Term();
}

//  initializes the critical section pool for use by cThread threads, returning
//  fTrue on success or fFalse on failure

template<class T>
BOOL CRITPOOL<T>::FInit( const LONG cThread, const INT rank, const _TCHAR* szName )
{
    //  ensure that Term() was called or ErrInit() was never called

    Assert( m_ccrit == 0 );
    Assert( m_rgcrit == NULL );
    Assert( m_cShift == 0 );
    Assert( m_mask == 0 );

    //  ensure that we have a valid number of threads

    Assert( cThread > 0 );

    //  we will use the next higher power of two than two times the number of
    //  threads that will use the critical section pool

    m_ccrit = LNextPowerOf2( cThread * 2 + 1 );

    //  the mask is one less ( x & ( n - 1 ) is cheaper than x % n if n
    //  is a power of two )
    
    m_mask = m_ccrit - 1;

    //  the hash shift const is the largest power of two that can divide the
    //  size of a <T> evenly so that the step we use through the CS array
    //  and the size of the array are relatively prime, ensuring that we use
    //  all the CSs

    LONG n;
    for ( m_cShift = -1, n = 1; sizeof( T ) % n == 0; m_cShift++, n *= 2 );

    //  allocate CS storage, but not as CSs (no default initializer)

    if ( !( m_rgcrit = (CCriticalSection *)( new BYTE[m_ccrit * sizeof( CCriticalSection )] ) ) )
    {
        goto HandleError;
    }

    //  initialize all CSs using placement new operator

    LONG icrit;
    for ( icrit = 0; icrit < m_ccrit; icrit++ )
    {
        new( m_rgcrit + icrit ) CCriticalSection( CLockBasicInfo( CSyncBasicInfo( szName ), rank, icrit ) );
    }

    return fTrue;

HandleError:
    Term();
    return fFalse;
}

//  terminates the critical section pool

template<class T>
void CRITPOOL<T>::Term( void )
{
    //  we must explicitly call each CCriticalSection's destructor due to use of placement new

    // ErrInit can fail in m_rgcrit allocation and Term() is called from HandleError with m_rgcrit NULL
    if (NULL != m_rgcrit)
    {
        LONG icrit;
        for ( icrit = 0; icrit < m_ccrit; icrit++ )
        {
            m_rgcrit[icrit].~CCriticalSection();
        }

        //  free CCriticalSection storage

        delete [] (BYTE *)m_rgcrit;
    }

    //  reset all members

    m_ccrit = 0;
    m_rgcrit = NULL;
    m_cShift = 0;
    m_mask = 0;
}

//  returns the critical section associated with the given instance of T

template<class T>
INLINE CCriticalSection& CRITPOOL<T>::Crit( const T* const pt )
{
    return m_rgcrit[( LONG_PTR( pt ) >> m_cShift ) & m_mask];
}


//  Reader Writer lock Pool

class RWLPOOL
{
    public:
        RWLPOOL( void );
        ~RWLPOOL( void );
        RWLPOOL& operator=( RWLPOOL& );  //  disallowed

        BOOL FInit( const LONG cThread, const LONG cbObjectSize, const INT rank, const _TCHAR* szName );
        void Term( void );

        CReaderWriterLock& Rwl( const VOID* const pObj );

    private:
        LONG                m_crwl;
        CReaderWriterLock*  m_rgrwl;
        LONG                m_cShift;
        LONG                m_mask;
        LONG                m_cMaskShift;
};
    
//  constructor

INLINE RWLPOOL::RWLPOOL( void )
{
    //  reset all members

    m_crwl = 0;
    m_rgrwl = NULL;
    m_cShift = 0;
    m_mask = 0;
    m_cMaskShift = 0;
}

//  destructor

INLINE RWLPOOL::~RWLPOOL( void )
{
    //  ensure that we have freed our resources

    Term();
}

//  initializes the reader writer lock pool for use by cThread threads, returning
//  fTrue on success or fFalse on failure

INLINE BOOL RWLPOOL::FInit( const LONG cThread, const LONG cbObjectSize, const INT rank, const _TCHAR* szName )
{
    //  ensure that Term() was called or ErrInit() was never called

    Assert( m_crwl == 0 );
    Assert( m_rgrwl == NULL );
    Assert( m_cShift == 0 );
    Assert( m_mask == 0 );
    Assert( m_cMaskShift == 0 );

    //  ensure that we have a valid number of threads

    Assert( cThread > 0 );

    //  we will use the next higher power of two than the number of
    //  threads that will use the reader writer lock pool

    m_crwl = LNextPowerOf2( cThread );

    //  the mask is one less ( x & ( n - 1 ) is cheaper than x % n if n
    //  is a power of two )
    
    m_mask = m_crwl - 1;
    m_cMaskShift = Log2( m_crwl );

    //  the hash shift const is the largest power of two that can divide the
    //  size of object evenly so that the step we use through the RWL array
    //  and the size of the array are relatively prime, ensuring that we use
    //  all the RWLs

    LONG n;
    for ( m_cShift = -1, n = 1; cbObjectSize % n == 0; m_cShift++, n *= 2 );

    //  allocate RWL storage, but not as RWLs (no default initializer)

    if ( !( m_rgrwl = (CReaderWriterLock *)( new BYTE[m_crwl * sizeof( CReaderWriterLock )] ) ) )
    {
        goto HandleError;
    }

    //  initialize all RWLs using placement new operator

    LONG irwl;
    for ( irwl = 0; irwl < m_crwl; irwl++ )
    {
        new( m_rgrwl + irwl ) CReaderWriterLock( CLockBasicInfo( CSyncBasicInfo( szName ), rank, irwl ) );
    }

    return fTrue;

HandleError:
    Term();
    return fFalse;
}

//  terminates the reader writer lock pool

INLINE void RWLPOOL::Term( void )
{
    //  we must explicitly call each CReaderWriterLock's destructor due to use of placement new

    // ErrInit can fail in m_rgrwl allocation and Term() is called from HandleError with m_rgrwl NULL
    if (NULL != m_rgrwl)
    {
        LONG irwl;
        for ( irwl = 0; irwl < m_crwl; irwl++ )
        {
            m_rgrwl[irwl].~CReaderWriterLock();
        }

        //  free CReaderWriterLock storage

        delete [] (BYTE *)m_rgrwl;
    }

    //  reset all members

    m_crwl = 0;
    m_rgrwl = NULL;
    m_cShift = 0;
    m_mask = 0;
    m_cMaskShift = 0;
}

//  returns the reader write lock associated with the given instance of T

INLINE CReaderWriterLock& RWLPOOL::Rwl( const VOID* const pObj )
{
    LONG_PTR dwValue = LONG_PTR( pObj ) >> m_cShift;
    LONG dwHash = 0;
    while ( dwValue != 0 )
    {
        dwHash = dwHash ^ (dwValue & m_mask);
        dwValue = dwValue >> m_cMaskShift;
    }
    Assert( dwHash < m_crwl );
    return m_rgrwl[ dwHash ];
}


//  Processor Local Storage

typedef struct BF* PBF;

class BFNominee
{
    public:

        volatile PBF    pbf;
        ULONG           cCacheReq;
};

const size_t cBFNominee = 4;

class BFHashedLatch
{
    public:

        BFHashedLatch()
            :    sxwl( CLockBasicInfo( CSyncBasicInfo( "BFHashedLatch Read/RDW/Write" ), 1000, CLockDeadlockDetectionInfo::subrankNoDeadlock ) )
        {
        }
        
    public:

        CSXWLatch       sxwl;
        PBF             pbf;
        ULONG           cCacheReq;
};

const size_t cBFHashedLatch = 16;

class PLS
{
    public:

        //  BF Nominees
        //
        //  BFs that are very hot will be nominated for promotion to a hashed
        //  S/X/W Latch.  Concurrently, the winner of the previous nomination
        //  round will be profiled for possible promotion.  The winner sits at
        //  the start of the array with the current nominees following.

        BFNominee       rgBFNominee[ cBFNominee ];

        //  BF Hashed S/X/W Latches
        //
        //  BFs that are very hot will have their single in-struct S/X/W Latch
        //  enhanced by adding one of these hashed latches.  The resulting
        //  latch will then allow multiple processors to latch the BF without
        //  causing contention over a single cache line.
        //
        //  Rules for use:
        //  -  waiting to acquire an S Latch on a hashed latch is forbidden
        //     -  if you can't try acquire the S Latch on the hashed latch and
        //        you need to wait then acquire the S Latch on the BF's latch
        //     -  this is necessary to support the demotion process
        //  -  to acquire the W Latch you must:
        //     -  acquire the W Latch on the BF's latch first
        //     -  then check pbf->fHashedLatch and, if set:
        //        -  acquire the W Latch on each instance of the hashed latch
        //  -  to convert from normal to hashed latch mode (promotion):
        //     -  acquire the X Latch on the BF
        //  -  to convert from hashed latch to normal mode (demotion):
        //     -  acquire the X Latch on the BF
        //     -  get the W Latch on each instance of the hashed latch

        BFHashedLatch   rgBFHashedLatch[ cBFHashedLatch ];

        //  BF Hashed S/X/W Latch Maintenance
        //
        //  This is performance data kept by the clean thread to maintain these
        //  latches.

        ULONG           rgcreqBFNominee[ 2 ][ cBFNominee ];
        ULONG           rgdcreqBFNominee[ cBFNominee ];
        ULONG           rgcreqBFHashedLatch[ 2 ][ cBFHashedLatch ];
        ULONG           rgdcreqBFHashedLatch[ cBFHashedLatch ];

#ifdef DEBUGGER_EXTENSION
        public:
#else   // !DEBUGGER_EXTENSION
        private:
#endif  // DEBUGGER_EXTENSION

        //  PERFInstance counter data
        //
        //  The perf counter data is stored as an array of blobs as follows:
        //  -  global counter blob
        //  -  array of instance counter blobs
        //
        //  There are N instance counter blobs, where N is currently max( max( g_cpinstMax, g_ifmpMax ), tceMax ).
        //  Each blob (global and instance) is g_cbPlsMemRequiredPerPerfInstance bytes in length. The size is
        //  computed during CRT init and is made available to ErrOSUSyncInit() so that the correct size may be computed.
        //
        //  s_pbPerfCounters is the space that is initially reserved and gets committed on-demand, as instances are
        //  modified or queried. It is split evenly across PLS objects.
        //
        //  s_cbPerfCounters is the total size pointed by s_pbPerfCounters.
        //
        //  fPerfCountersDisabled signals that performance counters are disabled.
        //
        //  pbPerfCounters is the pointer to the memory blob for a specific PLS object.
        //
        //  cbPerfCountersCommitted is the amount of committed memory, pointed by pbPerfCounters.
        //
        //  critPerfCounters is used to synchronize committing pbPerfCounters.

        static BYTE*        s_pbPerfCounters;
        static ULONG        s_cbPerfCounters;

        BOOL                fPerfCountersDisabled;
        BYTE*               pbPerfCounters;
        ULONG               cbPerfCountersCommitted;
        CCriticalSection    critPerfCounters;

    private:

        //  Commits the amount of memory necessary to host cb bytes.
        //
        BOOL FEnsurePerfCounterBuffer_( const ULONG cb )
        {
            BOOL fOwnsCritSec = fFalse;
            BOOL fIsBufferLargeEnough = fTrue;

            Assert( !fPerfCountersDisabled );
            Assert( s_pbPerfCounters != NULL );
            Assert( s_cbPerfCounters > 0 );

            if ( cb <= (ULONG)AtomicRead( (LONG*)&cbPerfCountersCommitted ) )
            {
                goto Return;
            }

            //  Buffer is too small, we'll try and commit the memory we need.
            critPerfCounters.Enter();
            fOwnsCritSec = fTrue;

            //  Re-check.
            if ( cb <= cbPerfCountersCommitted )
            {
                goto Return;
            }

            const ULONG cbPerfCountersNeeded = roundup( cb, OSMemoryPageCommitGranularity() );
            Assert( cbPerfCountersNeeded > cbPerfCountersCommitted );
            const ULONG dcbPerfCountersNeeded = cbPerfCountersNeeded - cbPerfCountersCommitted;

            if ( FOSMemoryPageCommit( pbPerfCounters + cbPerfCountersCommitted, dcbPerfCountersNeeded ) )
            {
                (void)AtomicExchange( (LONG*)&cbPerfCountersCommitted, cbPerfCountersNeeded );
            }
            else
            {
                fIsBufferLargeEnough = fFalse;
            }

        Return:
            if ( fOwnsCritSec )
            {
                critPerfCounters.Leave();
            }

            return fIsBufferLargeEnough;
        }

    public:

        //  Static that reserves memory for perf counters.
        //
        static BOOL FAllocatePerfCountersMemory( const ULONG cbPerfCountersPerProc )
        {
            Assert( s_pbPerfCounters == NULL );
            Assert( s_cbPerfCounters == 0 );
            Assert( cbPerfCountersPerProc > 0 );

            const ULONG cbPerfCounters = roundup( cbPerfCountersPerProc, OSMemoryPageReserveGranularity() ) * OSSyncGetProcessorCountMax();
            s_pbPerfCounters = (BYTE*)PvOSMemoryPageReserve( cbPerfCounters, NULL );
            if ( s_pbPerfCounters == NULL )
            {
                return fFalse;
            }

            s_cbPerfCounters = cbPerfCounters;

            return fTrue;
        }

        //  Static that frees memory for perf counters.
        //
        static void FreePerfCountersMemory()
        {
            OSMemoryPageFree( s_pbPerfCounters );
            s_pbPerfCounters = NULL;
            s_cbPerfCounters = 0;
        }

        //  Constructor.
        //
        PLS( const ULONG iProc, const BOOL fPerfCountersDisabledNew ) :
            fPerfCountersDisabled( fPerfCountersDisabledNew ),
            rgBFHashedLatch(),
            pbPerfCounters( NULL ),
            cbPerfCountersCommitted( 0 ),
            critPerfCounters( CLockBasicInfo( CSyncBasicInfo( "PLS::critPerfCounters" ), 0, 0 ) )
        {
            if ( fPerfCountersDisabled )
            {
                return;
            }
            
            Assert( s_pbPerfCounters != NULL );
            Assert( s_cbPerfCounters > 0 );

            const ULONG cProc = OSSyncGetProcessorCountMax();
            const ULONG cbPerfCounters = s_cbPerfCounters / cProc;
            const ULONG ibPerfCounters = iProc * cbPerfCounters;
            Assert( ibPerfCounters < s_cbPerfCounters );

            cbPerfCountersCommitted = 0;
            pbPerfCounters = s_pbPerfCounters + ibPerfCounters;
            Assert( ( pbPerfCounters + cbPerfCounters ) <= ( s_pbPerfCounters + s_cbPerfCounters ) );
        }

        //  Destructor.
        //
        ~PLS()
        {
            fPerfCountersDisabled = fFalse;
            pbPerfCounters = NULL;
            cbPerfCountersCommitted = 0;
        }

        //  Commits the amount of memory necessary to host cb bytes for all processors.
        //
        static BOOL FEnsurePerfCounterBuffer( const ULONG cb )
        {
            const size_t cProcs = (size_t)OSSyncGetProcessorCountMax();
            for ( size_t iProc = 0; iProc < cProcs; iProc++ )
            {
                PLS* const ppls = (PLS*)OSSyncGetProcessorLocalStorage( iProc );
                if ( !ppls->FEnsurePerfCounterBuffer_( cb ) )
                {
                    return fFalse;
                }
            }

            return fTrue;
        }

        //  Returns the pointer to the buffer. Does not commit extra memory.
        //
        INLINE BYTE* PbGetPerfCounterBuffer( const ULONG ib, const ULONG cb )
        {
            Assert( !fPerfCountersDisabled );
            Assert( s_pbPerfCounters != NULL );
            Assert( s_cbPerfCounters > 0 );
            Assert( ( ib + cb ) <= cbPerfCountersCommitted );

            return ( pbPerfCounters + ib );
        }

        //  Returns the number of committed bytes.
        //
        INLINE ULONG CbPerfCountersCommitted()
        {
            return cbPerfCountersCommitted;
        }
};

//  returns a pointer to this thread's PLS

INLINE PLS* Ppls()
{
    return (PLS*)OSSyncGetProcessorLocalStorage();
}

//  returns a pointer to the requested PLS

INLINE PLS* Ppls( const size_t iProc )
{
    return (PLS*)OSSyncGetProcessorLocalStorage( iProc );
}


//  init sync subsystem

ERR ErrOSUSyncInit();

//  terminate sync subsystem

void OSUSyncTerm();


#endif  //  _OSU_SYNC_HXX_INCLUDED

