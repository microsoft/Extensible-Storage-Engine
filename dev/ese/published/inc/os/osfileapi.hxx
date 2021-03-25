// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSFILEAPI_HXX_INCLUDED
#define _OSFILEAPI_HXX_INCLUDED

#include "collection.hxx"

// This is to set in cchPathMax, b/c OACR gets confused.
#define OSFSAPI_MAX_PATH    (260)
#define cbOSFSAPI_MAX_PATHW (OSFSAPI_MAX_PATH * sizeof(WCHAR))

struct SparseFileSegment
{
    QWORD ibFirst;  // Offset of the first byte of the sparse segment.
    QWORD ibLast;   // Offset of the last byte of the sparse segment.

    SparseFileSegment()
    {
        ibFirst = 0;
        ibLast = 0;
    }
};

//
//  IO Reason Interface
//

//
//  The IO reason is a client (such as ESE, or even the user) provided set of 
//  integer contexts that describe the purpose of the IO for that client.  The 
//  reasons are  client specific, and used for tracing and monitoring, and are 
//  otherwise not interprited in any meaniful way by the OS File IO APIs.
//
//  For composability of sub-systems, the IO reasons are constituted from 4 
//  potential sub-reasons:
//
//      Primary     - The primary reason, which may not be zero on any IO request,
//                    and is limited to less than or equal 0x7F in value.
//
//      Secondary   - The secondary may be zero (for now) and is reserved for 
//                    future use.  In the future we will probably allow us this 
//                    field as a flag of usage.
//
//      User        - The user field may be zero.  In the future we will probably 
//                    allow us this field as a flag of usage.
//
//      Flags       - Modifiers to the primary reason for the IO.
//

//  Reserve IOREQ pool
//
//  There is a limited pool for outstanding IO Requests, if some aspect of
//  ESE simply must not block on waiting for the IO completion thread to 
//  empty an IOREQ, then an IOREQ can be reserved preemptively while there 
//  are plenty of IOREQs, and can be retrieved / used by specifying the QOS
//  grbit qosIOPoolReservednote 

ERR ErrOSDiskIOREQReserve();
VOID OSDiskIOREQUnreserve();

//
//  Quality of Service (QOS)
//

//  The OS File layer Quality of Service (QOS) is a set of flags describing the 
//  liberties or constraints for the OS layer to take in completing this IO 
//  operation.

typedef QWORD       OSFILEQOS;

#define qosIONormal                         (qosIODispatchImmediate)

//  OS Related Quality of Service parameters
//
#define qosIOOSNormalPriority               (0x0)
#define qosIOOSLowPriority                  (0x2)
#define qosIOOSMask                         (qosIOOSNormalPriority | qosIOOSLowPriority)

//  Misc Options
//
#define qosIOPoolReserved                   (0x4)         //  Pool QOS - preserves IO to prevent deadlock.
#define qosIOSignalSlowSyncIO               (0x8)         //  Used to indicate sync-APIs want a wrnIOSlow for slow IOs.
#define qosIOMiscOptionsMask                (qosIOPoolReserved | qosIOSignalSlowSyncIO)

//  Dispatch urgency Quality of (un)Service parameters
//
#define qosIODispatchImmediate                  (0x10)
#define qosIODispatchIdle                       (0x20)
#define qosIODispatchBackground                 (0x40)
//#define qosIODispatchReserve1                 (0x80)
#define qosIODispatchUrgentBackgroundLevelMin   (0x0001)
#define qosIODispatchUrgentBackgroundLevelMax   (0x007F)
#define qosIODispatchUrgentBackgroundShft       (8)
#define qosIODispatchUrgentBackgroundMask       (0x7F00)    // covers 127 levels of urgency (qosIODispatchUrgentBackgroundLevelMin through  qosIODispatchUrgentBackgroundLevelMax)
#define qosIODispatchUrgentBackgroundMin        (0x0100)    // == qosIODispatchUrgentBackgroundLevelMin << qosIODispatchUrgentBackgroundShft
#define qosIODispatchUrgentBackgroundMax        (0x7F00)    // == qosIODispatchUrgentBackgroundLevelMax << qosIODispatchUrgentBackgroundShft
static_assert( qosIODispatchUrgentBackgroundMin == ( qosIODispatchUrgentBackgroundLevelMin << qosIODispatchUrgentBackgroundShft ), "Inconsistent qosIODispatchUrgentBackgroundMin/qosIODispatchUrgentBackgroundLevelMin." );
static_assert( qosIODispatchUrgentBackgroundMax == ( qosIODispatchUrgentBackgroundLevelMax << qosIODispatchUrgentBackgroundShft ), "Inconsistent qosIODispatchUrgentBackgroundMax/qosIODispatchUrgentBackgroundLevelMax." );

//  This function allows the client to translate from 127 levels to the UrgentBackground QOS
OSFILEQOS QosOSFileFromUrgentLevel( _In_ const ULONG iUrgentLevel );
//  This function only provided for performance counter support, not advised for use by components other than IO manager.
LONG CioOSDiskPerfCounterIOMaxFromUrgentQOS( _In_ const OSFILEQOS grbitQOS );

#define qosIODispatchMask                   (qosIODispatchImmediate | qosIODispatchUrgentBackgroundMask | qosIODispatchBackground | qosIODispatchIdle)

//  Geometry Optimization Quality of Service parameters.
//

#define qosIOOptimizeCombinable             (0x10000)
#define qosIOOptimizeOverrideMaxIOLimits    (0x20000)
#define qosIOOptimizeOverwriteTracing       (0x80000)
#define qosIOOptimizeMask                   (qosIOOptimizeCombinable | qosIOOptimizeOverrideMaxIOLimits | qosIOOptimizeOverwriteTracing)

// Temporary dispatch (for flighting purposes) to force write IOs to the new Q model.

#define qosIODispatchWriteMeted             (0x40000)

//  Storage spaces copy access.
//

#define qosIOReadCopyZero                   (0x100000)
#define qosIOReadCopyN                      (0x600000)  // we support 6-way copy
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

//   User IO Priority Tracing Info
//

#define qosUserPriorityClassIdMask          (0x0F000000)  //  User Level Priority Class ID.  Only used for tracing, produces no ESE behavioral changes.
#define qosUserPriorityMarkAsMaintenance    (0x40000000)  //  Identifies the IO activity produced by this session as Maintenance (vs. Transactional) for perfmon.  Only used for perfmon, produces no ESE behavioral changes.
#define qosIOUserPriorityTraceMask          (qosUserPriorityClassIdMask | qosUserPriorityMarkAsMaintenance) // RESERVED for User Priority Tracing specifiers.


//  All (input) QOS bits
#define qosIOInMask                         (qosIOOSMask | qosIODispatchMask | qosIOOptimizeMask | qosIOReadCopyMask | qosIOMiscOptionsMask)

//  IO Quality of Service reporting.
//    (output QOS signals)
//
#define qosIOCompleteIoCombined             (0x1000000000000)
#define qosIOCompleteReadGameOn             (0x2000000000000)
#define qosIOCompleteWriteGameOn            (0x4000000000000)
#define qosIOCompleteIoSlow                 (0x8000000000000)
//#define qosIOCompleteMask                  
#define qosIOCompleteMask                   (qosIOCompleteIoCombined | qosIOCompleteReadGameOn | qosIOCompleteWriteGameOn | qosIOCompleteIoSlow)

#define osfileMaxReadCopy                   6

//
//  FilePerf API
//
class IFilePerfAPI
{
    public:
        //  ctor/dtor
        //
        IFilePerfAPI( const DWORD dwEngineFileType, const QWORD qwEngineFileId ) :
            m_dwEngineFileType( dwEngineFileType ),
            m_qwEngineFileId( qwEngineFileId )
        {
        }
        virtual ~IFilePerfAPI()                                         {}

        //  virtual functions to update counters on a file-type basis
        //
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
        DWORD               m_dwEngineFileType;                     //  opaque file type for logging (in ESE it is IOFILE)
    private:
        QWORD               m_qwEngineFileId;                       //  opaque file ID for logging (in ESE DBs and flush maps = ifmp, logs = lgen)

        TICK                m_tickLastDiskFull;                     //  last time we reported disk full

        TICK                m_tickLastAbnormalIOLatencyEvent;       //  last time we reported abnormal I/O latency
        QWORD               m_cmsecLastAbnormalIOLatency;           //  actual latency of I/O last time abnormal I/O latency event was reported
        LONG                m_cAbnormalIOLatencySinceLastEvent;     //  number of I/O's with abnormal latency since last abnormal I/O latency event was reported
};


//  File API Notes

//  This abstraction exists to allow multiple file systems to be addressed in a
//  compatible manner
//
//  All paths may be absolute or relative unless otherwise specified.  All paths
//  are case insensitive and can be segmented by '\\' or '/' or both.  All
//  other characters take on their usual meaning for the implemented OS.  This
//  is intended to allow OS dependent paths to be passed down from the topmost
//  levels to these functions and still function properly
//
//  The maximum path length under any circumstance is specified by cchPathMax.
//  Buffers of at least this size must be given for all out parameters that will
//  contain paths and paths that are in parameters must not exceed this size
//
//  All file I/O is performed in blocks and is unbuffered and write-through
//  by default (can be overridden).
//
//  All file sharing is enforced as if the file were protected by a Reader /
//  Writer lock.  A file can be opened with multiple handles for Read Only
//  access but a file can only be opened with one handle for Read / Write
//  access.  A file can never be opened for both Read Only and Read / Write
//  access

////////////////////////////////////////
// NTFS only deals with sparse-ness at a 64k granularity.
#define cbSparseFileGranularity 65536

//  The "shared interface" for the reasons is this simple list of predefined
//  "integer" types that must be defined by the client library linking to the
//  OS File IO APIs.
enum IOFLUSHREASON : ULONG;

//  This is logged with the begin FFB trace call to indicate we would have
//  begun a flush at this point if the file was opened in WriteBack mode.
#define iofrOsFakeFlushSkipped      ((IOFLUSHREASON)0x80000000)

//
//  File API
//
class IFileAPI  //  fapi
{
    public:

        //  ctor/dtor

        IFileAPI()          {}
        virtual ~IFileAPI() {}

        //  flags to control the creation and operating mode of / for a given file
        
        enum FileModeFlags {  // fmf
            fmfNone                 = 0,
            fmfRegular              = 0,        //  a regular file means read-write (!read-only), non-cached / non-write-back, non-temporary.

                //  Common Flags
            
            fmfCached               = 0x1,      //  Avoid passing (as is the default) FILE_FLAG_NO_BUFFERING to OS, and instead passing FILE_FLAG_RANDOM_ACCESS.
            fmfStorageWriteBack     = 0x2,      //  Allows ESE to open a file without FILE_FLAG_WRITE_THROUGH to allow HD based caching.
            fmfLossyWriteBack       = 0x4,      //  If fmfCached also specified, then this avoids passing FILE_FLAG_WRITE_THROUGH to the OS, with further no intention of using FFB to maintain consistency.

            fmfSparse               = 0x8,      //  Controls if we try to put the file into sparse mode (FSCTL_SET_SPARSE set to true).

                //  ErrFileCreate() specific flags

            fmfTemporary            = 0x100,    //  Utilize FILE_FLAG_DELETE_ON_CLOSE when creating the file.
            fmfOverwriteExisting    = 0x200,    //  Passed CREATE_ALWAYS (instead of CREATE_NEW) for the create disposition.

                //  ErrFileOpen() specific flags

            fmfReadOnly             = 0x10000,  //  With this flag we do not request GENERIC_WRITE in the permissions (we always ask for GENERIC_READ).

            fmfReadOnlyClient       = 0x20000,  //  With this flag we disallow in the OS layer any write file APIs, but still open the file with GENERIC_WRITE (to support FFB).
            fmfReadOnlyPermissive   = 0x40000,  //  With this flag we do read only, but also allow SHARE_WRITE (called "Mode D" in ErrFileOpen) allowing interoperability with fmfReadOnlyClient.  Downside it doesn't lock out a errant writers.
        };

        //  returns the mode(s) or disposition(s) of the file, such as Read-Only, or Temporary file. See FileModeFlags.

        virtual FileModeFlags Fmf() const = 0;

        //  returns the absolute path of the current file

        virtual ERR ErrPath( _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const wszAbsPath ) = 0;

        ////////////////////////////////////////
        //  FILESIZE
        enum FILESIZE
        {
            filesizeLogical,    // The logical file size, as reported by `dir`.
            filesizeOnDisk,     // The physical files size; the number of bytes it takes up on disk.
        };

        //  returns the current size of the current file

        virtual ERR ErrSize(
            _Out_ QWORD* const pcbSize,
            _In_ const FILESIZE filesize ) = 0;

        //  returns the read only attribute of the current file

        virtual ERR ErrIsReadOnly( BOOL* const pfReadOnly ) = 0;

        // If file is backed by storage spaces, how many logical copies of data are present?

        virtual LONG CLogicalCopies() = 0;

        //  Property Control

        //  sets the size of the current file.  if there is insufficient disk
        //  space to satisfy the request, JET_errDiskFull will be returned

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

        //  I/O

        //  returns the size of the smallest block of the current file that is
        //  addressable by the I/O functions for the current file

        virtual ERR ErrIOSize( DWORD* const pcbSize ) = 0;

        //  returns the size sector size associated with the current file.

        virtual ERR ErrSectorSize( DWORD* const pcbSize ) = 0;

        //  type of the function that will be called when an asynchronously
        //  issued I/O has been successfully handed off to the IO Queue(s) in
        //  the lower IO Manager.  When this is called, the API is guaranteed
        //  to later surface a PfnIOComplete.  If the ErrIORead / ErrIOWrite 
        //  functions are going to return a quota exceeded / errDiskTilt error,
        //  this function will not be called.

        typedef void (*PfnIOHandoff)(   const ERR               err,
                                        IFileAPI* const         pfapi,
                                        const FullTraceContext& tc,
                                        const OSFILEQOS         grbitQOS,
                                        const QWORD             ibOffset,
                                        const DWORD             cbData,
                                        const BYTE* const       pbData,
                                        const DWORD_PTR         keyIOComplete,
                                        void* const             pvIOContext );

        //  type of the function that will be called when an asynchronously
        //  issued I/O has completed.  the function will be given the error
        //  code and parameters for the completed I/O as well as the key
        //  specified when the I/O was issued.  the key is intended for
        //  identification and post-processing of the I/O by the user

        typedef void (*PfnIOComplete)(  const ERR               err,
                                        IFileAPI* const         pfapi,
                                        const FullTraceContext& tc,
                                        const OSFILEQOS         grbitQOS,
                                        const QWORD             ibOffset,
                                        const DWORD             cbData,
                                        const BYTE* const       pbData,
                                        const DWORD_PTR         keyIOComplete );

        //  reserve/release I/O Reqs

        virtual ERR ErrReserveIOREQ(    const QWORD     ibOffset,
                                        const DWORD     cbData,
                                        const OSFILEQOS grbitQOS,
                                        VOID **         ppioreq ) = 0;
        virtual VOID ReleaseUnusedIOREQ( VOID * pioreq ) = 0;

        //  reads / writes a block of data of the specified size from the
        //  current file at the specified offset and places it into the
        //  specified buffer.  the offset, size, and buffer pointer must be
        //  aligned to the block size for the file
        //
        //  if no completion function is given, the function will not return
        //  until the I/O has been completed.  if an error occurs, that error
        //  will be returned
        //
        //  if a completion function is given, the function will return
        //  immediately and the I/O will be processed asynchronously.  the
        //  completion function will be called when the I/O has completed.  if
        //  an error occurs, that error will be returned via the completion
        //  function.  no error will be returned from this function.  there is
        //  no guarantee that an asynchronous I/O will be issued unless
        //  ErrIOIssue() is called or the file handle is closed
        //
        //  possible I/O errors:
        //
        //    JET_errDiskIO            general I/O failure
        //    JET_errDiskFull          a write caused the file to be
        //                             extended and there was insufficient
        //                             disk space to meet the request
        //    JET_errFileAccessDenied  a write was issued to a read only file
        //    JET_errFileIOBeyondEOF   a read was issued to a location beyond
        //                             the end of the file

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

        //  causes any unissued asynchronous I/Os for the current file to be
        //  issued eventually

        virtual ERR ErrIOIssue() = 0;

        //  Memory Mapped I/O

        enum FileMmIoReadFlag   //  fmmiorf
        {
            fmmiorfKeepCleanMapped      = 0x1,  //  reads it by deref with a read(only) operation, BUT ONLY honors this on "fixed" / system disks.
            fmmiorfCopyWritePage        = 0x2,  //  reads it by AtomicAdd of 0 to each MM page, forcing the OS to make a copy of the page (and doubling the memory usage).
            fmmiorfPessimisticReRead    = 0x4,  //  indicating the purpose of the reread is because we are worried the page is paged out
        };

        //  creates a Read Only or Copy On Write mapping of the current file.
        //  the resulting address of the mapping is returned.  the current
        //  file may then be directly accessed via this extent of virtual
        //  address space.
        //
        //  Revert can be used to "un-COW" a page after it has been modified
        //  and rewritten to the file system cache
        //  after mapping and before direct access ErrMMIORead() should be
        //  called to force disk IO access and bubble up errors.
        //  after IO direct access to any mapping _still may_ generate an exception 
        //  to indicate that an I/O error has occurred if the OS MM decided to page
        //  out this piece of memory.  these exceptions MUST be handled
        //  by the user.

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

        //  releases the specified mapping of the current file

        virtual ERR ErrMMFree( void* const pvMap ) = 0;

        //  register an IFilePerfAPI object with this file

        virtual VOID RegisterIFilePerfAPI( IFilePerfAPI * const pfpapi ) = 0;
        virtual VOID UpdateIFilePerfAPIEngineFileTypeId( _In_ const DWORD dwEngineFileType, _In_ const QWORD qwEngineFileId ) = 0;

        //  get current attribute list size (NTFS only)
        
        virtual ERR ErrNTFSAttributeListSize( QWORD* const pcbSize ) = 0;

        //  get disk ID for this file
        
        virtual ERR ErrDiskId( ULONG_PTR* const pulDiskId ) const = 0;

        //  get number of Write IOs that are unflushed or flush pending 
        //  since last ErrFlushFileBuffers call

        virtual LONG64 CioNonFlushed() const = 0;

        //  get seek penalty (in order to identify SSD)

        virtual BOOL FSeekPenalty() const = 0;

        //  IFilePerfAPI context info for debugging

        OnDebug( virtual DWORD DwEngineFileType() const = 0 );
        OnDebug( virtual QWORD QwEngineFileId() const = 0 );

        //  get the elapsed IO time of a pending IO via the pvIOContext from PfnIOHandoff

        virtual TICK DtickIOElapsed( void* const pvIOContext ) const = 0;
};

DEFINE_ENUM_FLAG_OPERATORS_BASIC( IFileAPI::FileModeFlags )

BOOL FIOThread( void );

// Exposing for log zero filling
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
const DWORD bitIoTraceMultiContextReserved      =     0x4000;  // RESERVED for Multi-User Priorities
const DWORD bitIoTraceMultiTidAlloc             =     0x8000;
const DWORD bitIoTraceSyncIo                    =   0x010000;
const DWORD bitIoTraceIoRetries                 =   0x020000; // Almost assuredly due to OOM (though technically there are retries when we are _first_ working out ScatterGather vs. Async vs. Sync IO).
const DWORD bitIoTraceTooLong                   =   0x040000;
const DWORD bitIoTraceReadGapped                =   0x080000;
const DWORD bitIoTraceWriteGapped               =   0x100000;
const DWORD bitIoTraceMultiObjid                =   0x200000;

//  Utility methods

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



#endif  //  _OSFILEAPI_HXX_INCLUDED

