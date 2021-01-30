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
        AGENT& operator=( AGENT& );

        void    Wait( CCriticalSection& crit );
        void    Release( CCriticalSection& crit );
        DWORD   CWait() const;

    private:
        union
        {
            volatile LONG        m_dw;
            struct
            {
                volatile LONG    m_cWait:31;
                volatile FLAG32  m_fAgentReleased:1;
            };
        };
        
        CKernelSemaphorePool::IRKSEM    m_irksem;
        BYTE                            m_rgbFiller[2];
};

INLINE void AGENT::Wait( CCriticalSection& crit )
{

    Assert( crit.FOwner() );

    Assert( !m_fAgentReleased );


    if ( 0 == CWait() )
    {
        Assert( CKernelSemaphorePool::irksemNil == m_irksem );
        m_irksem = g_ksempoolGlobal.Allocate( (const CLockObject* const) &m_irksem );
    }


    Assert( CWait() + 1 < 0x8000 );
    m_cWait++;

    Assert( CKernelSemaphorePool::irksemNil != m_irksem );


    crit.Leave();
    
    g_ksempoolGlobal.Ksem( m_irksem, (const CLockObject* const) &m_irksem ).Acquire();

    Assert( m_fAgentReleased );

    Assert( CWait() > 0 );
    AtomicDecrement( (LONG*)&m_dw );

    crit.Enter();
}

INLINE void AGENT::Release( CCriticalSection& crit )
{

    Assert( crit.FOwner() );

    Assert( !m_fAgentReleased );
    m_fAgentReleased = fTrue;

    if ( CWait() > 0 )
    {
        Assert( CKernelSemaphorePool::irksemNil != m_irksem );
        g_ksempoolGlobal.Ksem( m_irksem, (const CLockObject* const) &m_irksem ).Release( CWait() );
        while ( CWait() > 0 )
        {
            UtilSleep( 10 );
        }
        
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



class ENTERCRITICALSECTION
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


class ENTERREADERWRITERLOCK
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



template<class T>
class CRITPOOL
{
    public:
        CRITPOOL( void );
        ~CRITPOOL( void );
        CRITPOOL& operator=( CRITPOOL& );

        BOOL FInit( const LONG cThread, const INT rank, const _TCHAR* szName );
        void Term( void );

        CCriticalSection& Crit( const T* const pt );

    private:
        LONG                m_ccrit;
        CCriticalSection*   m_rgcrit;
        LONG                m_cShift;
        LONG                m_mask;
};
    

template<class T>
CRITPOOL<T>::CRITPOOL( void )
{

    m_ccrit = 0;
    m_rgcrit = NULL;
    m_cShift = 0;
    m_mask = 0;
}


template<class T>
CRITPOOL<T>::~CRITPOOL( void )
{

    Term();
}


template<class T>
BOOL CRITPOOL<T>::FInit( const LONG cThread, const INT rank, const _TCHAR* szName )
{

    Assert( m_ccrit == 0 );
    Assert( m_rgcrit == NULL );
    Assert( m_cShift == 0 );
    Assert( m_mask == 0 );


    Assert( cThread > 0 );


    m_ccrit = LNextPowerOf2( cThread * 2 + 1 );

    
    m_mask = m_ccrit - 1;


    LONG n;
    for ( m_cShift = -1, n = 1; sizeof( T ) % n == 0; m_cShift++, n *= 2 );


    if ( !( m_rgcrit = (CCriticalSection *)( new BYTE[m_ccrit * sizeof( CCriticalSection )] ) ) )
    {
        goto HandleError;
    }


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


template<class T>
void CRITPOOL<T>::Term( void )
{

    if (NULL != m_rgcrit)
    {
        LONG icrit;
        for ( icrit = 0; icrit < m_ccrit; icrit++ )
        {
            m_rgcrit[icrit].~CCriticalSection();
        }


        delete [] (BYTE *)m_rgcrit;
    }


    m_ccrit = 0;
    m_rgcrit = NULL;
    m_cShift = 0;
    m_mask = 0;
}


template<class T>
INLINE CCriticalSection& CRITPOOL<T>::Crit( const T* const pt )
{
    return m_rgcrit[( LONG_PTR( pt ) >> m_cShift ) & m_mask];
}



template<class T>
class RWLPOOL
{
    public:
        RWLPOOL( void );
        ~RWLPOOL( void );
        RWLPOOL& operator=( RWLPOOL& );

        BOOL FInit( const LONG cThread, const INT rank, const _TCHAR* szName );
        void Term( void );

        CReaderWriterLock& Rwl( const T* const pt );

    private:
        LONG                m_crwl;
        CReaderWriterLock*  m_rgrwl;
        LONG                m_cShift;
        LONG                m_mask;
};
    

template<class T>
RWLPOOL<T>::RWLPOOL( void )
{

    m_crwl = 0;
    m_rgrwl = NULL;
    m_cShift = 0;
    m_mask = 0;
}


template<class T>
RWLPOOL<T>::~RWLPOOL( void )
{

    Term();
}


template<class T>
BOOL RWLPOOL<T>::FInit( const LONG cThread, const INT rank, const _TCHAR* szName )
{

    Assert( m_crwl == 0 );
    Assert( m_rgrwl == NULL );
    Assert( m_cShift == 0 );
    Assert( m_mask == 0 );


    Assert( cThread > 0 );


    m_crwl = LNextPowerOf2( cThread * 2 + 1 );

    
    m_mask = m_crwl - 1;


    LONG n;
    for ( m_cShift = -1, n = 1; sizeof( T ) % n == 0; m_cShift++, n *= 2 );


    if ( !( m_rgrwl = (CReaderWriterLock *)( new BYTE[m_crwl * sizeof( CReaderWriterLock )] ) ) )
    {
        goto HandleError;
    }


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


template<class T>
void RWLPOOL<T>::Term( void )
{

    if (NULL != m_rgrwl)
    {
        LONG irwl;
        for ( irwl = 0; irwl < m_crwl; irwl++ )
        {
            m_rgrwl[irwl].~CReaderWriterLock();
        }


        delete [] (BYTE *)m_rgrwl;
    }


    m_crwl = 0;
    m_rgrwl = NULL;
    m_cShift = 0;
    m_mask = 0;
}


template<class T>
INLINE CReaderWriterLock& RWLPOOL<T>::Rwl( const T* const pt )
{
    return m_rgrwl[( LONG_PTR( pt ) >> m_cShift ) & m_mask];
}



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


        BFNominee       rgBFNominee[ cBFNominee ];


        BFHashedLatch   rgBFHashedLatch[ cBFHashedLatch ];


        ULONG           rgcreqBFNominee[ 2 ][ cBFNominee ];
        ULONG           rgdcreqBFNominee[ cBFNominee ];
        ULONG           rgcreqBFHashedLatch[ 2 ][ cBFHashedLatch ];
        ULONG           rgdcreqBFHashedLatch[ cBFHashedLatch ];

#ifdef DEBUGGER_EXTENSION
        public:
#else
        private:
#endif


        static BYTE*        s_pbPerfCounters;
        static ULONG        s_cbPerfCounters;

        BOOL                fPerfCountersDisabled;
        BYTE*               pbPerfCounters;
        ULONG               cbPerfCountersCommitted;
        CCriticalSection    critPerfCounters;

    private:

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

            critPerfCounters.Enter();
            fOwnsCritSec = fTrue;

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

        static void FreePerfCountersMemory()
        {
            OSMemoryPageFree( s_pbPerfCounters );
            s_pbPerfCounters = NULL;
            s_cbPerfCounters = 0;
        }

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

        ~PLS()
        {
            fPerfCountersDisabled = fFalse;
            pbPerfCounters = NULL;
            cbPerfCountersCommitted = 0;
        }

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

        INLINE BYTE* PbGetPerfCounterBuffer( const ULONG ib, const ULONG cb )
        {
            Assert( !fPerfCountersDisabled );
            Assert( s_pbPerfCounters != NULL );
            Assert( s_cbPerfCounters > 0 );
            Assert( ( ib + cb ) <= cbPerfCountersCommitted );

            return ( pbPerfCounters + ib );
        }

        INLINE ULONG CbPerfCountersCommitted()
        {
            return cbPerfCountersCommitted;
        }
};


INLINE PLS* Ppls()
{
    return (PLS*)OSSyncGetProcessorLocalStorage();
}


INLINE PLS* Ppls( const size_t iProc )
{
    return (PLS*)OSSyncGetProcessorLocalStorage( iProc );
}



ERR ErrOSUSyncInit();


void OSUSyncTerm();


#endif

