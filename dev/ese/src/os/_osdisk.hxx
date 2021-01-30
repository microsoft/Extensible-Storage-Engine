// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef __OSDISK_HXX_INCLUDED
#define __OSDISK_HXX_INCLUDED

#include <winioctl.h>



class _OSFILE;
class COSDisk;
typedef _OSFILE* P_OSFILE;

struct IFILEIBOFFSET
{
    QWORD                   iFile;
    QWORD                   ibOffset;


    const static IFILEIBOFFSET   IfileiboffsetNotFound;

    bool operator==( const IFILEIBOFFSET& rhs ) const
    {
        return ( iFile == rhs.iFile && ibOffset == rhs.ibOffset );
    }

    bool operator<( const IFILEIBOFFSET& rhs ) const
    {
        return ( iFile != rhs.iFile ?
                iFile < rhs.iFile :
                ibOffset < rhs.ibOffset );
    }
    bool operator>( const IFILEIBOFFSET& rhs ) const
    {
        return ( iFile != rhs.iFile ?
                iFile > rhs.iFile :
                ibOffset > rhs.ibOffset );
    }
};



template< typename T >
class MemCopyable
{
    public:
        MemCopyable() {};
        MemCopyable( const T* pSrc )
        {
            memcpy( this, pSrc, sizeof( T ) );
        }
};

class IOREQ : MemCopyable<IOREQ>
{
    public:
        static SIZE_T OffsetOfAPIC()        { return OffsetOf( IOREQ, m_apic ); }

    private:
        IOREQ& operator=( const IOREQ& ioreq );
        
    public:
    
        IOREQ();
        IOREQ( const IOREQ* pioreqToCopy );

    public:
        enum IOREQTYPE
        {
            ioreqUnknown    = 0,

            ioreqInAvailPool,
            ioreqCachedInTLS,
            ioreqInReservePool,

            ioreqAllocFromAvail,
            ioreqAllocFromEwreqLookaside,

            ioreqInIOCombiningList,

            ioreqEnqueuedInIoHeap,
            ioreqEnqueuedInVipList,
            ioreqEnqueuedInMetedQ,

            ioreqRemovedFromQueue,

            ioreqIssuedSyncIO,
            ioreqIssuedAsyncIO,

            ioreqSetSize,
            ioreqExtendingWriteIssued,

            ioreqCompleted,

            ioreqMax
        };

        enum IOMETHOD
        {
            iomethodNone = 0,
            iomethodSync,
            iomethodSemiSync,
            iomethodAsync,
            iomethodScatterGather
        };

    public:



        OVERLAPPED              ovlp;


        CPool< IOREQ, OffsetOfAPIC >::CInvasiveContext      m_apic;
        IOREQ*                  pioreqIorunNext;
        LONG                    ipioreqHeap;
            
    private:

        volatile DWORD          m_ciotime;

        CCriticalSection        m_crit;

        IOREQTYPE               m_ioreqtype:8;

        IOMETHOD                m_iomethod:8;

        DWORD                   m_reserved:16;

    public:

        volatile DWORD          m_grbitHungActionsTaken;

    public:

        IOREQTYPE   Ioreqtype() const           { return m_ioreqtype; }
        IOMETHOD    Iomethod() const            { return m_iomethod; }
        DWORD       Ciotime() const             { return m_ciotime; }

#ifdef DEBUG
        BOOL        FNotOwner() const           { return m_crit.FNotOwner(); }
        BOOL        FOwner() const              { return m_crit.FOwner(); }
#endif


        HRT                     hrtIOStart;

        COSDisk *               m_posdCurrentIO;


        FLAG32                  fWrite:1;
        INT                     group:2;
        FLAG32                  fCoalesced:1;
        FLAG32                  fFromReservePool:1;
        FLAG32                  m_fFromTLSCache:1;

        FLAG32                  m_fHasHeapReservation:1;
        FLAG32                  m_fHasBackgroundReservation:1;
        FLAG32                  m_fHasUrgentBackgroundReservation:1;

        FLAG32                  m_fCanCombineIO:1;

        static const INT        cRetriesMax = 0x7ffe;
        INT                     m_cRetries:15;
        INT                     m_reserved1:7;


        FullTraceContext        m_tc;
        OSFILEQOS               grbitQOS;



        QWORD                   ibOffset;
        const BYTE *            pbData;
        DWORD                   cbData;


        P_OSFILE                p_osf;
        DWORD_PTR               dwCompletionKey;
        PFN                     pfnCompletion;

        static SIZE_T OffsetOfHIIC()                        { return OffsetOf( IOREQ, m_hiic ); }
    private:
        CInvasiveList< IOREQ, OffsetOfHIIC >::CElement      m_hiic;

    public:

        DWORD                   m_tidAlloc;
        TICK                    m_tickAlloc;


        DWORD_PTR               m_iocontext;


    public:
        static SIZE_T OffsetOfMetedQueueIC()        { return OffsetOf( IOREQ, m_irbticMetedQ ); }
    private:
        InvasiveRedBlackTree< IFILEIBOFFSET, IOREQ, OffsetOfMetedQueueIC >::InvasiveContext
                                m_irbticMetedQ;

    public:
        IOREQ*                  pioreqVipList;

    public:




#ifndef RTM
        FLAG32                  m_fDequeueCombineIO:1;

        FLAG32                  m_fSplitIO:1;
        FLAG32                  m_fReMergeSplitIO:1;
        FLAG32                  m_fOutOfMemory:1;

        INT                     m_cTooComplex:3;
        FLAG32                  m_reservedNRtm:25;
#endif


    public:
    
    
        VOID ValidateIOREQTypeTransition( const IOREQTYPE ioreqtypeNext );
    
        VOID SetIOREQType( const IOREQTYPE ioreqtypeNext, const IOMETHOD iomethod = iomethodNone );

        VOID BeginIO( const IOMETHOD iomethod, const HRT hrtIOStart, const BOOL fHead );
        typedef enum { CompletedIO  = 1, ReEnqueueingIO = 2 } COMPLETION_STATUS;
        VOID CompleteIO( const COMPLETION_STATUS eCompletionStatus );

        VOID IncrementIOTime();
        


        QWORD IbBlockMax() const
        {
            Assert( ibOffset < (QWORD)llMax );
            Assert( cbData != 0 );
            return ibOffset + (QWORD)cbData;
        }
        

#ifndef RTM
        void AssertValid( void )  const;
#endif


        friend VOID DumpOneIOREQ( const IOREQ * const pioreqDebuggee, const IOREQ * const pioreq );


        friend ERR ErrIOMgrPatrolDogICheckOutIOREQ( __in ULONG ichunk, __in ULONG iioreq, __in IOREQ * pioreq, void * pctx );
        friend DWORD IOMgrIOPatrolDogThread( DWORD_PTR dwContext );

    public:

        INLINE BOOL FInAvailPool() const                        { return ioreqInAvailPool == m_ioreqtype; }
        INLINE BOOL FCachedInTLS() const                        { return ioreqCachedInTLS == m_ioreqtype; }
        INLINE BOOL FInReservePool() const                      { return ioreqInReservePool == m_ioreqtype; }
        INLINE BOOL FAllocFromAvail() const                     { return ioreqAllocFromAvail == m_ioreqtype; }
        INLINE BOOL FAllocFromEwreqLookaside() const            { return ioreqAllocFromEwreqLookaside == m_ioreqtype; }
        INLINE BOOL FInIOCombiningList() const                  { return ioreqInIOCombiningList == m_ioreqtype; }
        INLINE BOOL FEnqueuedInHeap() const                     { return ioreqEnqueuedInIoHeap == m_ioreqtype; }
        INLINE BOOL FEnqueuedInVIPList() const                  { return ioreqEnqueuedInVipList == m_ioreqtype; }
        INLINE BOOL FEnqueuedInMetedQ() const                   { return ioreqEnqueuedInMetedQ == m_ioreqtype; }
        INLINE BOOL FRemovedFromQueue() const                   { return ioreqRemovedFromQueue == m_ioreqtype; }
        INLINE BOOL FIssuedSyncIO() const                       { return ioreqIssuedSyncIO == m_ioreqtype; }
        INLINE BOOL FIssuedAsyncIO() const                      { return ioreqIssuedAsyncIO == m_ioreqtype; }
        INLINE BOOL FSetSize() const                            { return ioreqSetSize == m_ioreqtype; }
        INLINE BOOL FExtendingWriteIssued() const               { return ioreqExtendingWriteIssued == m_ioreqtype; }
        INLINE BOOL FCompleted() const                          { return ioreqCompleted == m_ioreqtype; }


#ifndef RTM
        BOOL FInAvailState() const                          { return FInAvailPool() || FCachedInTLS() || FInReservePool(); }
        BOOL FInPendingState() const                        { return FAllocFromAvail() || FAllocFromEwreqLookaside() || FInIOCombiningList(); }
#endif
        BOOL FInEnqueuedState() const                       { return FEnqueuedInHeap() || FEnqueuedInVIPList() || FEnqueuedInMetedQ(); }
        INLINE BOOL FVirtuallyIssuedState() const           { return FRemovedFromQueue() ; }
        INLINE BOOL FOtherIssuedState() const               { return FSetSize() || FExtendingWriteIssued(); }
        INLINE BOOL FOSIssuedState() const                  { return FIssuedSyncIO() || FIssuedAsyncIO(); }

        BOOL FInIssuedState() const                         { return FVirtuallyIssuedState() ||
                                                                                FOSIssuedState() ||
                                                                                FOtherIssuedState(); }
#ifndef RTM
        INLINE BOOL FEnqueueable() const                    { return m_fHasHeapReservation || grbitQOS & qosIOPoolReserved; }
#endif
        INLINE BOOL FUseMetedQ() const                      { return !!( grbitQOS & qosIODispatchBackground ) || !!( grbitQOS & qosIODispatchWriteMeted ); }

};

inline INT CmpIOREQOffset( const IOREQ * const pioreq1, const IOREQ * const pioreq2 )
{
    if ( pioreq1->ibOffset < pioreq2->ibOffset )
    {
        return -1;
    }
    else if ( pioreq1->ibOffset > pioreq2->ibOffset )
    {
        return 1;
    }
    Assert( pioreq1->ibOffset == pioreq2->ibOffset );
    return 0;
}




typedef struct _IOREQCHUNK
{
    DWORD                   cioreq;
    DWORD                   cioreqMac;
    struct _IOREQCHUNK *    pioreqchunkNext;

    BYTE                    rgbReserved[32-sizeof(DWORD)-sizeof(DWORD)-sizeof(struct _IOREQCHUNK *)];

    IOREQ                   rgioreq[];
} IOREQCHUNK;



class COSDisk : public CZeroInit
{

    public:
        static SIZE_T OffsetOfILE() { return OffsetOf( COSDisk, m_ile ); }
    private:
        CInvasiveList< COSDisk, OffsetOfILE >::CElement m_ile;


    public:
        COSDisk();
        ERR ErrInitDisk( __in_z const WCHAR * const wszDiskPathId, __in const DWORD dwDiskNumber );
        ~COSDisk();

        enum OSDiskState {
            eOSDiskInitCtor = 1,
            eOSDiskConnected
    };

        void AddRef();
        static void Release( COSDisk * posd );
        ULONG CRef() const;
        BOOL FIsDisk( __in_z const WCHAR * const wszTargetDisk ) const;

        void TraceStationId( const TraceStationIdentificationReason tsidr );

        void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;

    private:
        OSDiskState     m_eState;

        volatile ULONG  m_cref;

        DWORD           m_dwDiskNumber;

        COSEventTraceIdCheck m_traceidcheckDisk;

        WCHAR           m_wszDiskPathId[IFileSystemAPI::cchPathMax];

        #define         szEseNoLoadSmartData        "EseNoLo-S%d-E%d"
        #define         cchMaxEseNoLoadSmartData    ( 10 + 2  + 12 + 1 + 2  )
        enum { eSmartLoadDiskOpenFailed = 1, eSmartLoadDiskOpenRoFailed = 2, eSmartLoadDevIoCtrlGetVerFailed = 3, eSmartLoadSmartVersionUnexpected = 4,
                eSmartLoadSmartRevisionUnexpected = 5, eSmartLoadSmartCmdCapabilityNotSet = 6, eSmartLoadDevIoCtrlRcvDriveDataFailed = 7 };
        void SetSmartEseNoLoadFailed( __in const ULONG iStep, __in const DWORD error, __out_bcount_z(cbIdentifier) CHAR * szIdentifier, __in const ULONG cbIdentifier );

        typedef struct OSDiskInfo_
        {
            BYTE            m_bDiskSmartVersion;
            BYTE            m_bDiskSmartRevision;
            BYTE            m_bDiskSmartIdeDevMap;
            DWORD           m_grbitDiskSmartCapabilities;

            CHAR            m_szDiskModel[ max( 50 + 1, cchMaxEseNoLoadSmartData ) ];
            CHAR            m_szDiskModelSmart[ max( 39 + 1, cchMaxEseNoLoadSmartData ) ];
            CHAR            m_szDiskSerialNumber[ max( 50 + 1, cchMaxEseNoLoadSmartData ) ];
            CHAR            m_szDiskFirmwareRev[ 20 + 1 ];

            DWORD                           m_errorOsdci;
            _Field_size_( sizeof(DISK_CACHE_INFORMATION) )
            DISK_CACHE_INFORMATION          m_osdci;

            DWORD                           m_errorOsswcp;
            _Field_size_( sizeof(STORAGE_WRITE_CACHE_PROPERTY) )
            STORAGE_WRITE_CACHE_PROPERTY    m_osswcp;

            DWORD                           m_errorOssad;
            _Field_size_( sizeof(STORAGE_ADAPTER_DESCRIPTOR) )
            STORAGE_ADAPTER_DESCRIPTOR      m_ossad;

            DWORD                           m_errorOsdspd;
            _Field_size_( sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR) )
            DEVICE_SEEK_PENALTY_DESCRIPTOR  m_osdspd;

            DWORD                           m_errorOsdtd;
            _Field_size_( sizeof(DEVICE_TRIM_DESCRIPTOR) )
            DEVICE_TRIM_DESCRIPTOR          m_osdtd;

            DWORD                           m_errorOsdcod;
            _Field_size_( sizeof(DEVICE_COPY_OFFLOAD_DESCRIPTOR) )
            DEVICE_COPY_OFFLOAD_DESCRIPTOR  m_osdcod;
        }
        OSDiskInfo;

        OSDiskInfo      m_osdi;

        void LoadDiskInfo_( __in_z PCWSTR wszDiskPath, __in const DWORD dwDiskNumber );
        void LoadCachePerf_( HANDLE hDisk );

    public:

        const WCHAR * WszDiskPathId() const;

        
        ERR ErrDiskId( ULONG_PTR* const pulDiskId ) const;

        DWORD DwDiskNumber() const { return m_dwDiskNumber; }
        
        BOOL FSeekPenalty() const;


    public:

#ifdef DEBUG
        void AssertValid() const;
#endif



    public:

        class IORun
        {
            private:
                CSimpleQueue<IOREQ>     m_storage;
#ifdef DEBUG
                ULONG                   m_cbRun;
#endif

                void InitRun_( IOREQ * pioreqRun );

            public:

#ifdef DEBUG
                void AssertValid( ) const;
#endif

                IORun() : m_storage() { OnDebug( m_cbRun = 0 ); }
                IORun( IOREQ * pioreqHead ) : m_storage()
                {
                    OnDebug( m_cbRun = 0 );
                    FAddToRun( pioreqHead );
                }

                bool FEmpty( ) const                { ASSERT_VALID( this ); return 0 == m_storage.CElements(); }
                ULONG Cioreq( ) const               { ASSERT_VALID( this ); return m_storage.CElements(); }
#ifdef DEBUG
                BOOL FRunMember( const IOREQ * pioreqCheck ) const;
#endif
                ULONG CbRun( ) const;

                QWORD IbRunMax() const              { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? 0 : ( m_storage.Head()->ibOffset + (QWORD)CbRun() ); }

                QWORD IbOffset( ) const             { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? 0 : m_storage.Head()->ibOffset; }
                BOOL FWrite( ) const                { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? 2 : !!m_storage.Head()->fWrite; }
                _OSFILE * P_OSF( ) const            { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? NULL : m_storage.Head()->p_osf; }
                BOOL FHasHeapReservation( ) const   { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? fFalse : m_storage.Head()->m_fHasHeapReservation; }
                BOOL FUseMetedQ( ) const            { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? 0 : m_storage.Head()->FUseMetedQ(); }
                BOOL FIssuedFromQueue( ) const      { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? fFalse : m_storage.Head()->FIssuedAsyncIO(); }
                
                BOOL FMultiBlock( ) const           { ASSERT_VALID( this ); Expected( !FEmpty() ); return FEmpty() ?  fFalse : ( m_storage.Head() != m_storage.Tail() ); }
                IOREQ::IOMETHOD IomethodGet( ) const;

#ifndef RTM
                const IOREQ * PioreqHead( ) const   { ASSERT_VALID( this ); return m_storage.Head(); }
#endif

                bool FAddToRun( IOREQ * pioreq );
                void SetIOREQType( const IOREQ::IOREQTYPE ioreqtypeNext );

                void PrepareForIssue(
                            __in const IOREQ::IOMETHOD              iomethod,
                            __out DWORD * const                     pcbRun,
                            __out BOOL * const                      pfIOOSLowPriority,
                            __inout PFILE_SEGMENT_ELEMENT const     rgfse,
                            __in const DWORD                        cfse );
                IOREQ * PioreqGetRun();
                
        };

        static DWORD CbIOLength( const IOREQ * const    pioreqHead )
        {
            QWORD           ibOffsetMin = _I64_MAX;
            QWORD           ibOffsetMax = 0;
            DWORD           cbOffsetMaxData = 0;


            for ( const IOREQ * pioreqT = pioreqHead; pioreqT; pioreqT = pioreqT->pioreqIorunNext )
            {
                QWORD   ibOffset    = pioreqT->ibOffset;
                if ( ibOffsetMin > ibOffset )
                {
                    ibOffsetMin = ibOffset;
                }
                if ( ibOffsetMax <= ibOffset )
                {
                    ibOffsetMax = ibOffset;
                    cbOffsetMaxData = pioreqT->cbData;
                }
            }

            Assert( ibOffsetMin <= ibOffsetMax );
            Assert( cbOffsetMaxData );
            return (DWORD)( ibOffsetMax - ibOffsetMin ) + cbOffsetMaxData;
        }


        class CIoRunPool
        {
        private:
            static const INT        s_cConcurrentFileIoRuns = 2;
            static const INT        irunNil = -1;

            HRT                     m_rghrtIoRunStarted[ s_cConcurrentFileIoRuns ];
            IORun                   m_rgiorunIoRuns[ s_cConcurrentFileIoRuns ];

            INT IrunIoRunPoolIFindFileRun_( _In_ const _OSFILE * const p_osf ) const;
            IOREQ * PioreqIoRunPoolIRemoveRunSlot_( _In_ const INT irun );
            void IoRunPoolIAddNewRunSlot_( _In_ const INT irun, _Inout_ IOREQ * pioreqToAdd );

        public:

#ifdef ENABLE_JET_UNIT_TEST
            CIoRunPool()
            {
                memset( m_rgiorunIoRuns, 0, sizeof(m_rgiorunIoRuns) );
                ASSERT_VALID( &( m_rgiorunIoRuns[0] ) );
                Expected( m_rgiorunIoRuns[0].PioreqHead() == NULL );
            }

            ~CIoRunPool()
            {
            }
#endif

            void Dump() const;

            BOOL FCanCombineWithExistingRun(
                _In_ const _OSFILE *            p_osf,
                _In_ const BOOL                 fWrite,
                _In_ const QWORD                ibOffsetCombine,
                _In_ const DWORD                cbDataCombine,
                _In_ const BOOL                 fOverrideIoMax ) const;
#ifdef DEBUG
            BOOL FCanCombineWithExistingRun( _In_ const IOREQ * const pioreq ) const;
            INT Cioruns() const;
#endif


            BOOL FContainsFileIoRun( _In_ const _OSFILE * const p_osf ) const;


            enum IoRunPoolEmptyCheck { fCheckAllEmpty = 1, fCheckOneEmpty = 2 };

            BOOL FEmpty( _In_ const IoRunPoolEmptyCheck fCheck ) const;

            
            IOREQ * PioreqSwapAddIoreq( _Inout_ IOREQ * const pioreqToAdd );


            IOREQ * PioreqEvictFileRuns( _In_ const _OSFILE * const p_osf );

        };
    


    public:

        class QueueOp
        {
            public:

                enum EQueueOp { eUnknown = 0, eSpecialOperation, eIOOperation };

            private:
                EQueueOp            m_eQueueOp;
                IOREQ *             m_pioreq;
                COSDisk::IORun      m_iorun;

            public:

#ifdef DEBUG
                bool FValid( ) const;
#endif

                QueueOp() : m_eQueueOp( eUnknown ), m_pioreq( NULL ), m_iorun() { }
                QueueOp( IOREQ * pioreqHead ) :
                    m_eQueueOp( eUnknown ), m_pioreq( NULL ), m_iorun()
                {
                    bool fAccepted = FAddToRun( pioreqHead );
                    Assert( fAccepted );
                }

                bool FEmpty() const;
                BOOL FIssuedFromQueue() const;
                BOOL FHasHeapReservation() const;
                BOOL FUseMetedQ() const { return m_pioreq ? m_pioreq->FUseMetedQ() : m_iorun.FUseMetedQ(); }
                BOOL FWrite() const { Assert( FValid() ); Assert( !FEmpty() ); return m_pioreq ? m_pioreq->fWrite : m_iorun.FWrite(); }
                P_OSFILE P_osfPerfctrUpdates() const;
                DWORD CbRun() const;
                ULONG Cioreq() const;
#ifdef DEBUG
                BOOL FRunMember( const IOREQ * pioreqCheck ) const;
#endif

                bool FAddToRun( IOREQ * pioreq );
                void SetIOREQType( const IOREQ::IOREQTYPE ioreqtypeNext );

                IOREQ * PioreqOp() const;
                COSDisk::IORun * PiorunIO();
                IOREQ * PioreqGetRun();

        };



    public:

        __int64 IpassBeginDispatchPass()        { return m_pIOQueue->IpassBeginDispatchPass(); }
        __int64 IpassContinuingDispatchPass()   { return m_pIOQueue->IpassContinuingDispatchPass(); }

        ERR ErrReserveQueueSpace( __in OSFILEQOS grbitQOS, __inout IOREQ * pioreq );
        ERR ErrAllocIOREQ( __in OSFILEQOS grbitQOS, __in const _OSFILE * p_osf, __in const BOOL fWrite, __in QWORD ibOffsetCombine, __in DWORD cbDataCombine, __out IOREQ ** ppioreq );
        VOID FreeIOREQ( __in IOREQ * pioreq );
        void EnqueueIORun( __in IOREQ * pioreqHead );
        static void EnqueueDeferredIORun( _In_ const _OSFILE * const p_osf );
        void EnqueueIOREQ( __in IOREQ * pioreq );
        ERR ErrDequeueIORun( __inout COSDisk::QueueOp * pqop );
        void IncCioDispatching( void );
        void DecCioDispatching( void );
        void IncCioAsyncDispatching( _In_ const BOOL fWrite );
        void QueueCompleteIORun( __in IOREQ * pioreqHead );
        BOOL FQueueCompleteIOREQ( __in IOREQ * const pioreq );
        ERR ErrRemoveDeferredIoOp( _In_ TraceContext& tc, _In_ const QWORD ibOffsetMatch, _In_ const DWORD cbDataMatch, _In_ const IOREQ * const pioreqMatch, _Out_ IOREQ ** ppioreqHead );

#ifdef DEBUG
        LONG CioOutstanding() const;
#endif
        LONG CioDispatched() const;
        LONG CioUrgentEnqueued() const;
        LONG CioAllowedMetedOps( _In_ const LONG cioWaitingQ ) const;
        LONG CioReadyMetedEnqueued() const;
        LONG CioReadyWriteEnqueued() const         { return m_pIOQueue->CioWriteQueue(); }
        LONG CioAllEnqueued() const;
        LONG CioAsyncReadDispatching() const       { return m_cioAsyncReadDispatching; }
        LONG CioMetedReadQueue() const             { return m_pIOQueue->CioMetedReadQueue(); }

        void IncCioForeground( void )
        {
            AtomicIncrement( (LONG*)&m_cioForeground );
            Assert( m_cioForeground > 0 );
        }
        void DecCioForeground( void )
        {
            AtomicDecrement( (LONG*)&m_cioForeground );
            Assert( m_cioForeground >= 0 );
        }
        ULONG CioForeground() const
        {
            return m_cioForeground;
        }

        ERR ErrBeginConcurrentIo( const BOOL fForegroundSyncIO );
        void EndConcurrentIo( const BOOL fForegroundSyncIO, const BOOL fWrite );
        void BeginExclusiveIo();
        void EndExclusiveIo();

        void SuspendDiskIo();
        void ResumeDiskIo();

        void TrackOsFfbBegin( const IOFLUSHREASON iofr, const QWORD hFile );
        void TrackOsFfbComplete( const IOFLUSHREASON iofr, const DWORD error, const HRT hrtStart, const QWORD usFfb, const LONG64 cioFlushing, const WCHAR * const wszFileName );

    private:
        __int64         m_cioDequeued;
        volatile ULONG  m_cioForeground;
        volatile ULONG  m_cioDispatching;
        volatile LONG   m_cioAsyncReadDispatching;
        volatile LONG   m_cioAsyncWriteDispatching;
        volatile BOOL   m_fExclusiveIo;
        volatile ULONG  m_cFfbOutstanding;
        HRT             m_hrtLastFfb;
        HRT             m_hrtLastMetedDispatch;



    public:
        class IOQueueToo : CZeroInit
        {
            public:
                enum QueueInitType
                {
                    qitRead = 0x1,
                    qitWrite = 0x2,
                };

                enum QueueFirstIoFlags
                {
                    qfifDraining = 0x1,
                    qfifBuilding = 0x2,
                    qfifEither   = ( qfifDraining | qfifBuilding ),
                };

            public:
                IOQueueToo( CCriticalSection * pcritController, const QueueInitType qitInit );
                ~IOQueueToo();

            public:
                BOOL FInsertIo( _In_ IOREQ * pioreqHead, _In_ const LONG cioreqRun, _Out_ IOREQ ** ppioreqHeadAccepted );
                void ExtractIo( _Inout_ COSDisk::QueueOp * const pqop, _Out_ IOREQ ** ppioreqTraceHead, _Inout_ BOOL * pfCycles );
                void Cycle();

                IFILEIBOFFSET IfileiboffsetFirstIo( _In_ const QueueFirstIoFlags qfif );

                BOOL FStarvedMetedOp() const;

                LONG CioEnqueued() const
                {
                    return m_cioEnqueued;
                }
                
            
            private:
                OnDebug( QueueInitType          m_qit );
                OnDebug( CCriticalSection *     m_pcritIoQueue );
                ULONG                           m_cioEnqueued;
                ULONG                           m_cioreqEnqueued;

                HRT                             m_hrtBuildingStart;
                typedef InvasiveRedBlackTree< IFILEIBOFFSET, IOREQ, IOREQ::OffsetOfMetedQueueIC > IRBTQ;
                IRBTQ                           m_irbtBuilding;
                IRBTQ                           m_irbtDraining;

                friend void COSDisk::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const;
        };

    private:

        class IOQueue : CZeroInit
        {
            public:
                IOQueue( CCriticalSection * pcritController );
                ERR ErrIOQueueInit( __in LONG cIOEnqueuedMax, __in LONG cIOBackgroundMax, __in LONG cIOUrgentBackgroundMax );
                ~IOQueue();
            private:
                void IOQueueTerm();
                ERR _ErrIOHeapInit( __in LONG cIOEnqueuedMax );
                void _IOHeapTerm();

                void TrackIorunEnqueue( _In_ const IOREQ * const pioreqHead, _In_ const DWORD cbRun, _In_ HRT hrtEnqueueBegin, _In_ OSDiskIoQueueManagement dioqm ) const;
                void TrackIorunDequeue( _In_ const IOREQ * const pioreqHead, _In_ const DWORD cbRun, _In_ HRT hrtDequeueBegin, _In_ OSDiskIoQueueManagement dioqm, _In_ const USHORT cIorunCombined, _In_ const IOREQ * const pioreqAdded ) const;

            public:

                __int64 IpassBeginDispatchPass();
                __int64 IpassContinuingDispatchPass();
                ERR ErrReserveQueueSpace( __in OSFILEQOS grbitQOS, __inout IOREQ * pioreq );
                BOOL FReleaseQueueSpace( __inout IOREQ * pioreq );
                void InsertOp( __inout COSDisk::QueueOp * pqop );
                void ExtractOp( _In_ const COSDisk * const posd, _Inout_ COSDisk::QueueOp * pqop );
                ERR ErrRemoveDeferredIoOp( _In_ const IOREQ * const pioreqFind, _Out_ IOREQ ** ppioreqHead );

                INLINE LONG CioreqIOHeap() const;
                INLINE LONG CioVIPList() const;
                INLINE LONG CioMetedReadQueue() const;
                INLINE LONG CioWriteQueue() const          { return m_qWriteIo.CioEnqueued(); }

            private:
                __int64                 m_cDispatchPass;
                __int64                 m_cDispatchContinues;


            private:


                class IOHeapA
                {
                    friend ERR COSDisk::IOQueue::_ErrIOHeapInit( __in LONG cIOEnqueuedMax );

                    private:
                        IOREQ* volatile *           rgpioreqIOAHeap;
                        LONG                        ipioreqIOAHeapMax;
                        volatile LONG               ipioreqIOAHeapMac;
                        CCriticalSection * const    m_pcrit;



                    private:
                        void _HeapATerm();
                    public:
                        IOHeapA( CCriticalSection * pcritController ) :
                            m_pcrit( pcritController ),
                            rgpioreqIOAHeap( NULL ),
                            ipioreqIOAHeapMax( 0 ),
                            ipioreqIOAHeapMac( 0 )
                        {
                        }
                        ERR ErrHeapAInit( __in LONG cIOEnqueuedMax );
                        ~IOHeapA()              { _HeapATerm(); }

                    public:
                        BOOL FHeapAEmpty() const;
                        LONG CioreqHeapA() const;
                        IOREQ* PioreqHeapATop();
                        void HeapAAdd( IOREQ* pioreq );
                        void HeapARemove( IOREQ* pioreq );
                        BOOL FHeapAFrom( const IOREQ * pioreq ) const;
                    private:
                        BOOL FHeapAISmaller( IOREQ* pioreq1, IOREQ* pioreq2 ) const;
                        LONG IpioreqHeapAIParent( LONG ipioreq ) const;
                        LONG IpioreqHeapAILeftChild( LONG ipioreq ) const;
                        LONG IpioreqHeapAIRightChild( LONG ipioreq ) const;
                    private:
                        void HeapAIUpdate( IOREQ* pioreq );

                };

                class IOHeapB
                {
                    private:

                        IOREQ* volatile *           rgpioreqIOBHeap;
                        LONG                        ipioreqIOBHeapMax;
                        volatile LONG               ipioreqIOBHeapMic;
                        CCriticalSection * const    m_pcrit;


                    private:
                        void _HeapBTerm();
                    public:
                        IOHeapB( CCriticalSection * pcritController ) :
                            m_pcrit( pcritController ),
                            rgpioreqIOBHeap( NULL ),
                            ipioreqIOBHeapMax( 0 ),
                            ipioreqIOBHeapMic( 0 )
                        {
                        }
                        ERR ErrHeapBInit(
                                IOREQ* volatile *   rgpioreqIOAHeap,
                                LONG                ipioreqIOAHeapMax );
                        ~IOHeapB()          { _HeapBTerm(); }

                    public:
                        BOOL FHeapBEmpty() const;
                        LONG CioreqHeapB() const;
                        IOREQ* PioreqHeapBTop();
                        void HeapBAdd( IOREQ* pioreq );
                        void HeapBRemove( IOREQ* pioreq );
                    private:
                        BOOL FHeapBISmaller( IOREQ* pioreq1, IOREQ* pioreq2 ) const;
                        LONG IpioreqHeapBIParent( LONG ipioreq ) const;
                        LONG IpioreqHeapBILeftChild( LONG ipioreq ) const;
                        LONG IpioreqHeapBIRightChild( LONG ipioreq ) const;
                        void HeapBIUpdate( IOREQ* pioreq );

                };


            private:


                CCriticalSection * m_pcritIoQueue;


                LONG                m_cioreqMax;
                CSemaphore          m_semIOQueue;


#ifdef DEBUG
                friend COSDisk::~COSDisk();
#endif
                IOHeapA * m_pIOHeapA;
                IOHeapB * m_pIOHeapB;


                BOOL    fUseHeapA;

                QWORD   iFileCurrent;
                QWORD   ibOffsetCurrent;


            private:

                INLINE void IOHeapAdd( IOREQ* pioreq, _Out_ OSDiskIoQueueManagement * const pdioqmTypeTracking );
                INLINE void IOHeapRemove( IOREQ* pioreq, _Out_ OSDiskIoQueueManagement * const pdioqmTypeTracking );
                INLINE BOOL FIOHeapEmpty();
                INLINE IOREQ* PioreqIOHeapTop();



            private:

                LONG                            m_cioVIPList;
                CLocklessLinkedList<IOREQ>      m_VIPListHead;
                

                BOOL    FValidVIPList() const
                {
                    Assert( m_pcritIoQueue->FOwner() );

                    Assert( 0 == m_cioVIPList || !m_VIPListHead.FEmpty() );
                    Assert( 0 != m_cioVIPList || m_VIPListHead.FEmpty() );
                    #ifdef DEBUG
                    const IOREQ * pioreqT = m_VIPListHead.Head();
                    LONG cioreqT = 0;
                    while ( pioreqT )
                    {
                        cioreqT++;
                        Assert( pioreqT->FEnqueuedInVIPList() );
                        pioreqT = pioreqT->pioreqVipList;
                    }
                    Assert( cioreqT == m_cioVIPList );
                    Assert( cioreqT < 100 ||
                            ( FNegTestSet( fDisableTimeoutDeadlockDetection ) && cioreqT < 800 ) );
                    return fTrue;
                    #endif
                }



            private:
                IOQueueToo              m_qMetedAsyncReadIo;
                IOQueueToo              m_qWriteIo;


            private:

                INT                 m_cioQosBackgroundMax;
                INT                 m_cioreqQOSBackgroundLow;
                volatile INT            m_cioQosBackgroundCurrent;

                INT                 m_cioQosUrgentBackgroundMax;
                volatile INT            m_cioQosUrgentBackgroundCurrent;

                
                friend void COSDisk::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const;

        };

    private:


        IOQueue *           m_pIOQueue;


        CCriticalSection    m_critIOQueue;


    public:


        INLINE DWORD CioOsQueueDepth();

    private:


        HANDLE              m_hDisk;


        static const TICK   s_dtickPerformancePeriod        = 100;
        TICK                m_tickPerformanceLastMeasured;
        DWORD               m_cioOsQueueDepth;


        INLINE VOID RefreshDiskPerformance();


        VOID QueryDiskPerformance();

};



ERR ErrOSDiskConnect( __in_z const WCHAR * const wszDiskPathId, __in const DWORD dwDiskNumber, __out IDiskAPI ** ppdiskapi );


void OSDiskDisconnect( __inout IDiskAPI * pdiskapi, __in const _OSFILE * p_osf );



class PatrolDogSynchronizer
{
    private:
        enum PatrolDogState
        {
            pdsInactive,
            pdsActivating,
            pdsActive,
            pdsDeactivating
        };
        
        CAutoResetSignal    m_asigNudgePatrolDog;

        BOOL volatile       m_fPutDownPatrolDog;

        DWORD volatile      m_cLoiters;

        THREAD              m_threadPatrolDog;

        LONG volatile       m_patrolDogState;

        void*               m_pvContext;

        void InitPatrolDogState_();

        void AssertPatrolDogInitialState_( const PatrolDogState pdsExpected = pdsInactive ) const;

    public:

        
        PatrolDogSynchronizer();


        ~PatrolDogSynchronizer();


        ERR ErrInitPatrolDog( const PUTIL_THREAD_PROC pfnPatrolDog,
                                const EThreadPriority priority,
                                void* const pvContext );


        void TermPatrolDog();


        void EnterPerimeter();


        void LeavePerimeter();


        BOOL FCheckForLoiter( const TICK dtickTimeout );


        void* PvContext() const;

        
        SIZE_T CbSizeOf() const;
};

extern PatrolDogSynchronizer g_patrolDogSync;



QWORD CmsecLatencyOfOSOperation( const IOREQ* const pioreq, const HRT dhrtIOElapsed );
QWORD CmsecLatencyOfOSOperation( const IOREQ* const pioreq );
void OSDiskIOThreadStartIssue( const P_OSFILE p_osf );
void OSDiskIIOThreadCompleteWithErr( DWORD error, DWORD cbTransfer, IOREQ* pioreqHead );

void OSDiskIIOThreadIComplete(  const DWORD     dwError,
                                const DWORD_PTR dwThreadContext,
                                DWORD           cbTransfer,
                                IOREQ           *pioreqHead );
VOID OSDiskIIOThreadIRetryIssue();



BOOL GetOverlappedResult_(  HANDLE          hFile,
                            LPOVERLAPPED    lpOverlapped,
                            LPDWORD         lpNumberOfBytesTransferred,
                            BOOL            bWait );

#define hNoCPEvent  ((DWORD_PTR)0x1)

#endif

