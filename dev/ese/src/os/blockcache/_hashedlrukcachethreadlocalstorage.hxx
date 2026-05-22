// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  A cache thread local storage for the hashed LRUK cache.

template< class I >
class CHashedLRUKCacheThreadLocalStorage  //  ctls
    :   public CCacheThreadLocalStorageBase
{
    using CRequest = typename THashedLRUKCache<I>::CRequest;

    public:

        CHashedLRUKCacheThreadLocalStorage( _In_ const CacheThreadId ctid )
            :   CCacheThreadLocalStorageBase( ctid ),
                m_pc( NULL ),
                m_ptpwIssue( NULL ),
                m_cwAsyncIOWorkerState( ControlWord::cwNone ),
                m_ctidAsyncIOWorker( ctidInvalid ),
                m_critAsyncIOWorkerState( CLockBasicInfo( CSyncBasicInfo( "CHashedLRUKCacheThreadLocalStorage::m_critAsyncIOWorkerState" ), rankIssued, 0 ) ),
                m_rgibSlab { 0 },
                m_rgpcbsSlab { NULL },
                m_ibSlabWait( 0 ),
                m_cIORangeLocked( 0 ),
                m_cbIORangeLocked( 0 )
        {
        }

        void Initialize( _In_ THashedLRUKCache<I>* const pc, _Inout_ TP_WORK** const pptpwIssue )
        {
            m_pc = pc;
            m_ptpwIssue = *pptpwIssue;
            *pptpwIssue = NULL;
        }

        THashedLRUKCache<I>* Pc() const { return m_pc; }
        TP_WORK* PtpwIssue() const { return m_ptpwIssue; }
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>& IlIORequested() { return m_ilIORequested; }
        CCriticalSection& CritAsyncIOWorkerState() { return m_critAsyncIOWorkerState; }
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>& IlIOIssued() { return m_ilIOIssued; }
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>& IlIORangeLockPending() { return m_ilIORangeLockPending; }
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>& IlIORangeLocked() { return m_ilIORangeLocked; }
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>& IlCachedFileIORequested() { return m_ilCachedFileIORequested; }
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>& IlCachingFileIORequested() { return m_ilCachingFileIORequested; }
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>& IlIOPending() { return m_ilIOPending; }
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>& IlIOCompleted() { return m_ilIOCompleted; }
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>& IlFinalizeIORequested() { return m_ilFinalizeIORequested; }
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>& IlFinalizeIOPending() { return m_ilFinalizeIOPending; }
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>& IlFinalizeIOCompleted() { return m_ilFinalizeIOCompleted; }
        QWORD IbSlabWait() const { return AtomicRead( (__int64*)&m_ibSlabWait ); }
        volatile DWORD& CIORangeLocked() { return m_cIORangeLocked; }
        volatile QWORD& CbIORangeLocked() { return m_cbIORangeLocked; }

        void AddRequest( _Inout_ CRequest** const pprequest )
        {
            CRequest* const    prequest = *pprequest;

            *pprequest = NULL;

            CritAsyncIOWorkerState().Enter();
            m_ilRequests.InsertAsNextMost( prequest );
            CritAsyncIOWorkerState().Leave();
        }

        void RemoveRequest( _In_ CRequest* const prequest )
        {
            CritAsyncIOWorkerState().Enter();
            m_ilRequests.Remove( prequest );
            CritAsyncIOWorkerState().Leave();

            CRequest* prequestT = prequest;
            (void)CRequest::ErrRelease( &prequestT, JET_errSuccess );
        }

        void RegisterOpenSlab( _In_ ICachedBlockSlab* const pcbs )
        {
            QWORD ibSlab = 0;
            CallS( pcbs->ErrGetPhysicalId( &ibSlab ) );
            Assert( ibSlab );

            for ( size_t iibSlab = 0; iibSlab < s_cibSlab; iibSlab++ )
            {
                if ( !m_rgibSlab[ iibSlab ] )
                {
                    m_rgibSlab[ iibSlab ] = ibSlab;
                    m_rgpcbsSlab[ iibSlab ] = pcbs;

                    AddRef();

                    return;
                }
            }

            EnforceSz( fFalse, "RegisterOpenSlab" );
        }

        ICachedBlockSlab* PcbsOpenSlab( _In_ const QWORD ibSlab )
        {
            for ( size_t iibSlab = 0; iibSlab < s_cibSlab; iibSlab++ )
            {
                if ( m_rgibSlab[ iibSlab ] == ibSlab )
                {
                    return m_rgpcbsSlab[ iibSlab ];
                }
            }

            return NULL;
        }

        BOOL FOpenSlab( _In_ ICachedBlockSlab* const pcbs )
        {
            if ( pcbs )
            {
                QWORD ibSlab = 0;
                CallS( pcbs->ErrGetPhysicalId( &ibSlab ) );
                Assert( ibSlab );

                for ( size_t iibSlab = 0; iibSlab < s_cibSlab; iibSlab++ )
                {
                    if ( m_rgibSlab[ iibSlab ] == ibSlab )
                    {
                        return fTrue;
                    }
                }
            }

            return fFalse;
        }

        BOOL FAnyOpenSlab()
        {
            for ( size_t iibSlab = 0; iibSlab < s_cibSlab; iibSlab++ )
            {
                if ( m_rgibSlab[ iibSlab ] )
                {
                    return fTrue;
                }
            }

            return fFalse;
        }

        void UnregisterOpenSlab( _In_ ICachedBlockSlab* const pcbs )
        {
            QWORD ibSlab = 0;
            CallS( pcbs->ErrGetPhysicalId( &ibSlab ) );
            Assert( ibSlab );

            for ( size_t iibSlab = 0; iibSlab < s_cibSlab; iibSlab++ )
            {
                if ( m_rgibSlab[ iibSlab ] == ibSlab )
                {
                    m_rgibSlab[ iibSlab ] = 0;
                    m_rgpcbsSlab[ iibSlab ] = NULL;

                    CHashedLRUKCacheThreadLocalStorage<I>* pctlsT = this;
                    Release( &pctlsT );

                    return;
                }
            }

            EnforceSz( fFalse, "UnregisterOpenSlab" );
        }

        void RegisterOpenSlabWait( _In_ const QWORD ibSlab )
        {
            Assert( ibSlab );
            EnforceSz( !IbSlabWait(), "RegisterOpenSlabWait" );
            AtomicExchange( (__int64*)&m_ibSlabWait, ibSlab );
        }

        void UnregisterOpenSlabWait( _In_ const QWORD ibSlab )
        {
            Assert( ibSlab );
            EnforceSz( IbSlabWait(), "UnregisterOpenSlabWait" );
            AtomicExchange( (__int64*)&m_ibSlabWait, 0 );
        }

        void BeginAsyncIOWorker()
        {
            m_ctidAsyncIOWorker = CtidCurrentThread();

            OSTrace(    JET_tracetagBlockCacheOperations,
                        OSFormat(   "C=%s BeginAsyncIOWorker .%x",
                                    OSFormatFileId( Pc() ),
                                    Ctid() ) );
        }

        BOOL FTryEndAsyncIOWorker()
        {
            BOOL fNewRequest = fFalse;

            //  if the worker is requested and running then clear the requested state
            //  if the worker is not requested and running then clear the running state

            OSSYNC_FOREVER
            {
                const ControlWord cwAsyncIOWorkerStateBIExpected    = (ControlWord)AtomicRead( (LONG*)&m_cwAsyncIOWorkerState );
                const ControlWord cwAsyncIOWorkerStateAI            = cwAsyncIOWorkerStateBIExpected == ControlWord::cwRunning ?
                                                                        ControlWord::cwNone :
                                                                        ControlWord::cwRunning;
                const ControlWord cwAsyncIOWorkerStateBI            = (ControlWord)AtomicCompareExchange(   (LONG*)&m_cwAsyncIOWorkerState,
                                                                                                            (LONG)cwAsyncIOWorkerStateBIExpected,
                                                                                                            (LONG)cwAsyncIOWorkerStateAI );

                if ( cwAsyncIOWorkerStateBI == cwAsyncIOWorkerStateBIExpected )
                {
                    fNewRequest = cwAsyncIOWorkerStateBI == ControlWord::cwRequestedAndRunning;
                    break;
                }
            }

            OSTrace(    JET_tracetagBlockCacheOperations,
                        OSFormat(   "C=%s FTryEndAsyncIOWorker .%x = %s",
                                    OSFormatFileId( Pc() ),
                                    Ctid(),
                                    OSFormatBoolean( !fNewRequest ) ) );

            //  if we are no longer running then release the ref count for the async IO worker

            if ( !fNewRequest )
            {
                CHashedLRUKCacheThreadLocalStorage<I>* pctlsT = this;
                Release( &pctlsT );
            }

            //  we have succeeded in ending if there is no new request

            return !fNewRequest;
        }

        void CueAsyncIOWorker()
        {
            //  request the async IO worker

            const ControlWord   cwAsyncIOWorkerStateBI  = (ControlWord)AtomicExchange(  (LONG*)&m_cwAsyncIOWorkerState,
                                                                                        (LONG)ControlWord::cwRequestedAndRunning );

            const BOOL          fNewRequest             = ( cwAsyncIOWorkerStateBI == ControlWord::cwNone ||
                                                            cwAsyncIOWorkerStateBI == ControlWord::cwRunning );
            const BOOL          fSignalNeeded           = cwAsyncIOWorkerStateBI == ControlWord::cwNone;

            //  signal the async IO worker if necessary

            if ( fSignalNeeded )
            {
                //  add a ref count for the async IO worker

                AddRef();

                //  submit a work item for the async IO worker

                SubmitThreadpoolWork( PtpwIssue() );
            }

            OSTrace(    JET_tracetagBlockCacheOperations,
                        OSFormat(   "C=%s CueAsyncIOWorker .%x fNewRequest = %s fSignalNeeded = %s",
                                    OSFormatFileId( Pc() ),
                                    Ctid(),
                                    OSFormatBoolean( fNewRequest ),
                                    OSFormatBoolean( fSignalNeeded ) ) );
        }

        static void Release( _Inout_ CHashedLRUKCacheThreadLocalStorage<I>** const ppctls )
        {
            CCacheThreadLocalStorageBase::Release( (CCacheThreadLocalStorageBase** const)ppctls );
        }

    protected:

        ~CHashedLRUKCacheThreadLocalStorage()
        {
            m_pc->ReleaseThreadpoolState( &m_ptpwIssue );
        }

    private:

        enum class ControlWord : LONG
        {
            cwNone = 0,
            cwRequested = 1,
            cwRunning = 2,
            cwRequestedAndRunning = 3,
        };

    private:

        THashedLRUKCache<I>*                                                m_pc;
        TP_WORK*                                                            m_ptpwIssue;

        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>               m_ilIORequested;
        volatile ControlWord                                                m_cwAsyncIOWorkerState;
        CacheThreadId                                                       m_ctidAsyncIOWorker;
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>               m_ilIORangeLockPending;
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>               m_ilIORangeLocked;
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>               m_ilCachedFileIORequested;
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>               m_ilCachingFileIORequested;
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>               m_ilIOPending;
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>               m_ilIOCompleted;
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>               m_ilFinalizeIORequested;
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>               m_ilFinalizeIOPending;
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>               m_ilFinalizeIOCompleted;

        CCriticalSection                                                    m_critAsyncIOWorkerState;
        CCountedInvasiveList<CRequest, CRequest::OffsetOfRequestsByThread>  m_ilRequests;
        CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>               m_ilIOIssued;

        static const size_t                                                 s_cibSlab                           = 2;
        QWORD                                                               m_rgibSlab[ s_cibSlab ];
        ICachedBlockSlab*                                                   m_rgpcbsSlab[ s_cibSlab ];
        volatile QWORD                                                      m_ibSlabWait;

        volatile DWORD                                                      m_cIORangeLocked;
        volatile QWORD                                                      m_cbIORangeLocked;
};
