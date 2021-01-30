// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSFILEAPI_HXX_INCLUDED
#define _OSFILEAPI_HXX_INCLUDED

#include "collection.hxx"

#define OSFSAPI_MAX_PATH    (260)
#define cbOSFSAPI_MAX_PATHW (OSFSAPI_MAX_PATH * sizeof(WCHAR))

struct SparseFileSegment
{
    QWORD ibFirst;
    QWORD ibLast;

    SparseFileSegment()
    {
        ibFirst = 0;
        ibLast = 0;
    }
};




ERR ErrOSDiskIOREQReserve();
VOID OSDiskIOREQUnreserve();



typedef QWORD       OSFILEQOS;

#define qosIONormal                         (qosIODispatchImmediate)

#define qosIOOSNormalPriority               (0x0)
#define qosIOOSLowPriority                  (0x2)
#define qosIOOSMask                         (qosIOOSNormalPriority | qosIOOSLowPriority)

#define qosIOPoolReserved                   (0x4)
#define qosIOSignalSlowSyncIO               (0x8)
#define qosIOMiscOptionsMask                (qosIOPoolReserved | qosIOSignalSlowSyncIO)

#define qosIODispatchImmediate                  (0x10)
#define qosIODispatchIdle                       (0x20)
#define qosIODispatchBackground                 (0x40)
#define qosIODispatchUrgentBackgroundLevelMin   (0x0001)
#define qosIODispatchUrgentBackgroundLevelMax   (0x007F)
#define qosIODispatchUrgentBackgroundShft       (8)
#define qosIODispatchUrgentBackgroundMask       (0x7F00)
#define qosIODispatchUrgentBackgroundMin        (0x0100)
#define qosIODispatchUrgentBackgroundMax        (0x7F00)
static_assert( qosIODispatchUrgentBackgroundMin == ( qosIODispatchUrgentBackgroundLevelMin << qosIODispatchUrgentBackgroundShft ), "Inconsistent qosIODispatchUrgentBackgroundMin/qosIODispatchUrgentBackgroundLevelMin." );
static_assert( qosIODispatchUrgentBackgroundMax == ( qosIODispatchUrgentBackgroundLevelMax << qosIODispatchUrgentBackgroundShft ), "Inconsistent qosIODispatchUrgentBackgroundMax/qosIODispatchUrgentBackgroundLevelMax." );

OSFILEQOS QosOSFileFromUrgentLevel( __in const ULONG iUrgentLevel );
LONG CioOSDiskPerfCounterIOMaxFromUrgentQOS( _In_ const OSFILEQOS grbitQOS );

#define qosIODispatchMask                   (qosIODispatchImmediate | qosIODispatchUrgentBackgroundMask | qosIODispatchBackground | qosIODispatchIdle)


#define qosIOOptimizeCombinable             (0x10000)
#define qosIOOptimizeOverrideMaxIOLimits    (0x20000)
#define qosIOOptimizeOverwriteTracing       (0x80000)
#define qosIOOptimizeMask                   (qosIOOptimizeCombinable | qosIOOptimizeOverrideMaxIOLimits | qosIOOptimizeOverwriteTracing)


#define qosIODispatchWriteMeted             (0x40000)


#define qosIOReadCopyZero                   (0x100000)
#define qosIOReadCopyN                      (0x600000)
#define qosIOReadCopyTestExclusiveIo        (0x700000)
#define QosOSFileReadCopy( n )              ( ( n <= 5 ) ? ( qosIOReadCopyZero * ( n + 1 ) ) : qosIOReadCopyN )
#define qosIOReadCopyMask                   (qosIOReadCopyTestExclusiveIo)
#define IReadCopyFromQos( qos )             ( ( qos & qosIOReadCopyMask ) / qosIOReadCopyZero - 1 )
C_ASSERT( QosOSFileReadCopy( -1 ) == 0 );
C_ASSERT( IReadCopyFromQos( QosOSFileReadCopy( -1 ) ) == -1 );
C_ASSERT( QosOSFileReadCopy( 0 ) == qosIOReadCopyZero );
C_ASSERT( IReadCopyFromQos( QosOSFileReadCopy( 0 ) ) == 0 );
C_ASSERT( QosOSFileReadCopy( 5 ) == qosIOReadCopyN );
C_ASSERT( IReadCopyFromQos( QosOSFileReadCopy( 5 ) ) == 5 );
C_ASSERT( QosOSFileReadCopy( 6 ) == qosIOReadCopyN );
C_ASSERT( qosIOReadCopyMask == ( QosOSFileReadCopy( 0 ) | QosOSFileReadCopy( 1 ) | QosOSFileReadCopy( 2 ) | QosOSFileReadCopy( 3 ) | QosOSFileReadCopy( 4 ) | QosOSFileReadCopy( 5 ) ) );
C_ASSERT( 0 == ( ~qosIOReadCopyMask & ( QosOSFileReadCopy( 0 ) | QosOSFileReadCopy( 1 ) | QosOSFileReadCopy( 2 ) | QosOSFileReadCopy( 3 ) | QosOSFileReadCopy( 4 ) | QosOSFileReadCopy( 5 ) ) ) );


#define qosUserPriorityClassIdMask          (0x0F000000)
#define qosUserPriorityMarkAsMaintenance    (0x40000000)
#define qosIOUserPriorityTraceMask          (qosUserPriorityClassIdMask | qosUserPriorityMarkAsMaintenance)


#define qosIOInMask                         (qosIOOSMask | qosIODispatchMask | qosIOOptimizeMask | qosIOReadCopyMask | qosIOMiscOptionsMask)

#define qosIOCompleteIoCombined             (0x1000000000000)
#define qosIOCompleteReadGameOn             (0x2000000000000)
#define qosIOCompleteWriteGameOn            (0x4000000000000)
#define qosIOCompleteIoSlow                 (0x8000000000000)
#define qosIOCompleteMask                   (qosIOCompleteIoCombined | qosIOCompleteReadGameOn | qosIOCompleteWriteGameOn | qosIOCompleteIoSlow)

#define osfileMaxReadCopy                   6

class IFilePerfAPI
{
    public:
        IFilePerfAPI( const DWORD dwEngineFileType, const QWORD qwEngineFileId ) :
            m_dwEngineFileType( dwEngineFileType ),
            m_qwEngineFileId( qwEngineFileId )
        {
        }
        virtual ~IFilePerfAPI()                                         {}

        virtual DWORD_PTR IncrementIOIssue(
                        const DWORD     diskQueueDepth,
                        const BOOL fWrite ) = 0;
        virtual VOID IncrementIOCompletion(
                        const DWORD_PTR iocontext,
                        const DWORD     dwDiskNumber,
                        const OSFILEQOS qosIo,
                        const HRT       dhrtIOElapsed,
                        const DWORD     cbTransfer,
                        const BOOL      fWrite ) = 0;
        virtual VOID IncrementIOInHeap( const BOOL fWrite ) = 0;
        virtual VOID DecrementIOInHeap( const BOOL fWrite ) = 0;
        virtual VOID IncrementIOAsyncPending( const BOOL fWrite ) = 0;
        virtual VOID DecrementIOAsyncPending( const BOOL fWrite ) = 0;
        virtual VOID IncrementFlushFileBuffer( const HRT dhrtElapsed ) = 0;
        virtual VOID SetCurrentQueueDepths( const LONG cioMetedReadQueue, const LONG cioAllowedMetedOps, const LONG cioAsyncRead ) = 0;

    public:
        enum { dtickOSFileDiskFullEvent = 60 * 1000 };
        enum { dtickOSFileAbnormalIOLatencyEvent = 60 * 1000 };

    public:
        DWORD DwEngineFileType() const                      { return m_dwEngineFileType; }
        QWORD QwEngineFileId() const                        { return m_qwEngineFileId; }
        TICK TickLastAbnormalIOLatencyEvent() const         { return m_tickLastAbnormalIOLatencyEvent; }
        LONG CAbnormalIOLatencySinceLastEvent() const       { return m_cAbnormalIOLatencySinceLastEvent; }

        VOID UpdateEngineFileTypeId( _In_ const DWORD dwEngineFileType, _In_ const QWORD qwEngineFileId )
        {
            m_dwEngineFileType = dwEngineFileType;
            m_qwEngineFileId = qwEngineFileId;
        }

        VOID Init();
        BOOL FReportDiskFull();
        BOOL FReportAbnormalIOLatency( const BOOL fWrite, const QWORD cmsecIOElapsed );
        VOID ResetAbnormalIOLatency( const QWORD cmsecIOElapsed );

    protected:
        DWORD               m_dwEngineFileType;
    private:
        QWORD               m_qwEngineFileId;

        TICK                m_tickLastDiskFull;

        TICK                m_tickLastAbnormalIOLatencyEvent;
        QWORD               m_cmsecLastAbnormalIOLatency;
        LONG                m_cAbnormalIOLatencySinceLastEvent;
};




#define cbSparseFileGranularity 65536

enum IOFLUSHREASON;

#define iofrOsFakeFlushSkipped      ((IOFLUSHREASON)0x80000000)

class IFileAPI
{
    public:


        IFileAPI()          {}
        virtual ~IFileAPI() {}

        
        enum FileModeFlags {
            fmfNone                 = 0,
            fmfRegular              = 0,

            
            fmfCached               = 0x1,
            fmfStorageWriteBack     = 0x2,
            fmfLossyWriteBack       = 0x4,

            fmfSparse               = 0x8,


            fmfTemporary            = 0x100,
            fmfOverwriteExisting    = 0x200,


            fmfReadOnly             = 0x10000,

            fmfReadOnlyClient       = 0x20000,
            fmfReadOnlyPermissive   = 0x40000,
        };


        virtual FileModeFlags Fmf() const = 0;


        virtual ERR ErrPath( _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const wszAbsPath ) = 0;

        enum FILESIZE
        {
            filesizeLogical,
            filesizeOnDisk,
        };


        virtual ERR ErrSize(
            _Out_ QWORD* const pcbSize,
            _In_ const FILESIZE filesize ) = 0;


        virtual ERR ErrIsReadOnly( BOOL* const pfReadOnly ) = 0;


        virtual LONG CLogicalCopies() = 0;



        virtual ERR ErrSetSize( const TraceContext& tc,
                                const QWORD         cbSize,
                                const BOOL          fZeroFill,
                                const OSFILEQOS     grbitQOS ) = 0;

        virtual ERR ErrRename(  const WCHAR* const  wszAbsPathDest,
                                const BOOL          fOverwriteExisting = fFalse ) = 0;

        virtual ERR ErrSetSparseness() = 0;

        virtual ERR ErrIOTrim( const TraceContext&  tc,
                               const QWORD          ibOffset,
                               const QWORD          cbToFree
                               ) = 0;

        virtual ERR ErrRetrieveAllocatedRegion( const QWORD     ibOffsetToQuery,
                                           _Out_ QWORD* const   pibStartTrimmedRegion,
                                           _Out_ QWORD* const   pcbTrimmed ) = 0;

        virtual ERR ErrFlushFileBuffers( const IOFLUSHREASON iofr ) = 0;
        virtual void SetNoFlushNeeded() = 0;



        virtual ERR ErrIOSize( DWORD* const pcbSize ) = 0;


        virtual ERR ErrSectorSize( DWORD* const pcbSize ) = 0;


        typedef void (*PfnIOHandoff)(   const ERR               err,
                                        IFileAPI* const         pfapi,
                                        const FullTraceContext& tc,
                                        const OSFILEQOS         grbitQOS,
                                        const QWORD             ibOffset,
                                        const DWORD             cbData,
                                        const BYTE* const       pbData,
                                        const DWORD_PTR         keyIOComplete,
                                        void* const             pvIOContext );


        typedef void (*PfnIOComplete)(  const ERR               err,
                                        IFileAPI* const         pfapi,
                                        const FullTraceContext& tc,
                                        const OSFILEQOS         grbitQOS,
                                        const QWORD             ibOffset,
                                        const DWORD             cbData,
                                        const BYTE* const       pbData,
                                        const DWORD_PTR         keyIOComplete );


        virtual ERR ErrReserveIOREQ(    const QWORD     ibOffset,
                                        const DWORD     cbData,
                                        const OSFILEQOS grbitQOS,
                                        VOID **         ppioreq ) = 0;
        virtual VOID ReleaseUnusedIOREQ( VOID * pioreq ) = 0;


        virtual ERR ErrIORead(  const TraceContext& tc,
                                const QWORD         ibOffset,
                                const DWORD         cbData,
        __out_bcount( cbData )  BYTE* const         pbData,
                                const OSFILEQOS     grbitQOS,
                                const PfnIOComplete pfnIOComplete   = NULL,
                                const DWORD_PTR     keyIOComplete   = NULL,
                                const PfnIOHandoff  pfnIOHandoff    = NULL,
                                const VOID *        pioreq          = NULL ) = 0;
        virtual ERR ErrIOWrite( const TraceContext& tc,
                                const QWORD         ibOffset,
                                const DWORD         cbData,
                                const BYTE* const   pbData,
                                const OSFILEQOS     grbitQOS,
                                const PfnIOComplete pfnIOComplete   = NULL,
                                const DWORD_PTR     keyIOComplete   = NULL,
                                const PfnIOHandoff  pfnIOHandoff    = NULL ) = 0;


        virtual ERR ErrIOIssue() = 0;


        enum FileMmIoReadFlag
        {
            fmmiorfKeepCleanMapped      = 0x1,
            fmmiorfCopyWritePage        = 0x2,
            fmmiorfPessimisticReRead    = 0x4,
        };


        virtual ERR ErrMMRead(  const QWORD     ibOffset,
                                const QWORD     cbSize,
                                void** const    ppvMap ) = 0;
        virtual ERR ErrMMCopy(  const QWORD     ibOffset,
                                const QWORD     cbSize,
                                void** const    ppvMap ) = 0;
        virtual ERR ErrMMIORead( _In_                   const QWORD             ibOffset,
                                 _Out_writes_bytes_(cb) BYTE * const            pb,
                                 _In_                   ULONG                   cb,
                                 _In_                   const FileMmIoReadFlag  fmmiorf ) = 0;
        virtual ERR ErrMMRevert( _In_                   const QWORD             ibOffset,
                                 _In_reads_bytes_(cbSize)void* const            pvMap,
                                 _In_                   const QWORD             cbSize ) = 0;


        virtual ERR ErrMMFree( void* const pvMap ) = 0;


        virtual VOID RegisterIFilePerfAPI( IFilePerfAPI * const pfpapi ) = 0;
        virtual VOID UpdateIFilePerfAPIEngineFileTypeId( _In_ const DWORD dwEngineFileType, _In_ const QWORD qwEngineFileId ) = 0;

        
        virtual ERR ErrNTFSAttributeListSize( QWORD* const pcbSize ) = 0;

        
        virtual ERR ErrDiskId( ULONG_PTR* const pulDiskId ) const = 0;


        virtual LONG64 CioNonFlushed() const = 0;


        virtual BOOL FSeekPenalty() const = 0;


        OnDebug( virtual DWORD DwEngineFileType() const = 0 );
        OnDebug( virtual QWORD QwEngineFileId() const = 0 );


        virtual TICK DtickIOElapsed( void* const pvIOContext ) const = 0;
};

DEFINE_ENUM_FLAG_OPERATORS_BASIC( IFileAPI::FileModeFlags )

BOOL FIOThread( void );

extern QWORD g_cbZero;
extern BYTE* g_rgbZero;

#ifdef OS_LAYER_UNIT_TESTING_ACCESS
HANDLE HOSLayerTestGetFileHandle( IFileAPI * pfapi );
#endif

const DWORD mskIoTraceIomethod      = 0x0F;
const DWORD bitIoTraceMultiIorp     = 0x10;
const DWORD bitIoTraceMultiIors     = 0x20;
const DWORD bitIoTraceMultiIort     = 0x40;
const DWORD bitIoTraceMultiIoru     = 0x80;
const DWORD bitIoTraceMultiParentObjectClass    = 0x100;
const DWORD bitIoTraceMultiCorrelationID        = 0x200;
const DWORD bitIoTraceMultiContextUserID        = 0x400;
const DWORD bitIoTraceMultiContextOperationID   = 0x800;
const DWORD bitIoTraceMultiContextOperationType = 0x1000;
const DWORD bitIoTraceMultiContextClientType    = 0x2000;
const DWORD bitIoTraceMultiContextReserved      =     0x4000;
const DWORD bitIoTraceMultiTidAlloc             =     0x8000;
const DWORD bitIoTraceSyncIo                    =   0x010000;
const DWORD bitIoTraceIoRetries                 =   0x020000;
const DWORD bitIoTraceTooLong                   =   0x040000;
const DWORD bitIoTraceReadGapped                =   0x080000;
const DWORD bitIoTraceWriteGapped               =   0x100000;
const DWORD bitIoTraceMultiObjid                =   0x200000;


ERR ErrIOWriteContiguous(   IFileAPI* const                         pfapi,
                            __in_ecount( cData ) const TraceContext rgtc[],
                            const QWORD                             ibOffset,
                            __in_ecount( cData ) const DWORD        rgcbData[],
                            __in_ecount( cData ) const BYTE* const  rgpbData[],
                            const size_t                            cData,
                            const OSFILEQOS                         grbitQOS );

ERR ErrIORetrieveSparseSegmentsInRegion(    IFileAPI* const                             pfapi,
                                            _In_ QWORD                                  ibFirst,
                                            _In_ QWORD                                  ibLast,
                                            _Inout_ CArray<SparseFileSegment>* const    parrsparseseg );



#endif

