// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef __OSFILE_HXX_INCLUDED
#define __OSFILE_HXX_INCLUDED


class COSFilePerfDummy
    : public IFilePerfAPI
{
    public:
        COSFilePerfDummy() : IFilePerfAPI( 0, 0xFFFFFFFFFFFFFFFF )      {}
        virtual ~COSFilePerfDummy()                 {}

        virtual DWORD_PTR IncrementIOIssue(
            const DWORD     diskQueueDepth,
            const BOOL      fWrite )
        {
            return NULL;
        }

        virtual VOID IncrementIOCompletion(
            const DWORD_PTR iocontext,
            const DWORD     dwDiskNumber,
            const OSFILEQOS qosIo,
            const HRT       dhrtIOElapsed,
            const DWORD     cbTransfer,
            const BOOL      fWrite )
        {
        }

        virtual VOID IncrementIOInHeap( const BOOL fWrite )
        {
        }

        virtual VOID DecrementIOInHeap( const BOOL fWrite )
        {
        }

        virtual VOID IncrementIOAsyncPending( const BOOL fWrite )
        {
        }

        virtual VOID DecrementIOAsyncPending( const BOOL fWrite )
        {
        }

        virtual VOID IncrementFlushFileBuffer( const HRT dhrtElapsed )
        {
        }
        
        virtual VOID SetCurrentQueueDepths( const LONG cioMetedReadQueue, const LONG cioAllowedMetedOps, const LONG cioAsyncRead )
        {
        }
};

extern COSFilePerfDummy     g_cosfileperfDefault;



class IOREQ;
class COSFile;

class _OSFILE
{
    public:

        typedef void (*PfnFileIOComplete)(  const IOREQ* const      pioreq,
                                            const ERR               err,
                                            const DWORD_PTR         keyFileIOComplete );

    public:

        _OSFILE()
            :   semFilePointer( CSyncBasicInfo( "_OSFILE::semFilePointer" ) ),
                m_posd( NULL ),
                pfpapi( NULL )
        {
            semFilePointer.Release();
        }

        ~_OSFILE()
        {
            Assert( NULL != pfpapi );
            if ( pfpapi != &g_cosfileperfDefault )
            {
                delete pfpapi;
            }
            pfpapi = NULL;
        }

        ERR ErrSetReadCopyNumber( LONG iCopyNumber );

        IFileSystemConfiguration* const Pfsconfig() const;

    public:

        COSDisk *                   m_posd;
        COSFile *                   m_posf;

        HANDLE                      hFile;
        PfnFileIOComplete           pfnFileIOComplete;
        DWORD_PTR                   keyFileIOComplete;

        IFileSystemConfiguration*   pfsconfig;
        IFilePerfAPI *              pfpapi;

        QWORD                       iFile;
        IOREQ::IOMETHOD             iomethodMost;
        volatile BOOL               fRegistered;
        CSemaphore                  semFilePointer;

        IFileAPI::FileModeFlags     fmfTracing;
};

typedef _OSFILE* P_OSFILE;



class COSFile
    :   public IFileAPI
{
    friend VOID DumpOneIOREQ( const IOREQ * const pioreqDebuggee, const IOREQ * const pioreq );

    public:


        COSFile();


        ERR ErrInitFile(    COSFileSystem* const                posfs,
                            COSVolume * const                   posv,
                            __in PCWSTR const                   wszAbsPath,
                            const HANDLE                        hFile,
                            const QWORD                         cbFileSize,
                            const FileModeFlags                 fmf,
                            const DWORD                         cbIOSize,
                            const DWORD                         cbSectorSize );

        void TraceStationId( const TraceStationIdentificationReason tsidr );
   

        const WCHAR * WszFile() const   { return m_wszAbsPath; }
        HANDLE Handle() const           { return m_hFile; }


        void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;


        IFileSystemConfiguration* const Pfsconfig() const { return m_posfs->Pfsconfig(); }

    public:

        virtual ~COSFile();

        IFileAPI::FileModeFlags Fmf() const override;

        ERR ErrFlushFileBuffers( const IOFLUSHREASON iofr ) override;
        void SetNoFlushNeeded() override;

        ERR ErrPath( _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const wszAbsPath ) override;
        ERR ErrSize(    _Out_ QWORD* const              pcbSize,
                        _In_ const IFileAPI::FILESIZE   filesize ) override;
        ERR ErrIsReadOnly( BOOL* const pfReadOnly ) override;

        LONG CLogicalCopies() override;

        ERR ErrSetSize( const TraceContext& tc,
                        const QWORD         cbSize,
                        const BOOL          fZeroFill,
                        const OSFILEQOS     grbitQOS ) override;

        ERR ErrRename(  const WCHAR* const  wszPathDest,
                        const BOOL          fOverwriteExisting = fFalse ) override;

        ERR ErrSetSparseness() override;

        ERR ErrIOTrim(  const TraceContext& tc,
                        const QWORD         ibOffset,
                        const QWORD         cbToFree ) override;

        ERR ErrRetrieveAllocatedRegion( const QWORD         ibOffsetToQuery,
                                        _Out_ QWORD* const  pibStartTrimmedRegion,
                                        _Out_ QWORD* const  pcbTrimmed ) override;

        ERR ErrIOSize( DWORD* const pcbSize ) override;

        ERR ErrSectorSize( DWORD* const pcbSize ) override;
 
        ERR ErrReserveIOREQ(    const QWORD     ibOffset,
                                const DWORD     cbData,
                                const OSFILEQOS grbitQOS,
                                VOID **         ppioreq ) override;
        VOID ReleaseUnusedIOREQ( VOID * pioreq ) override;

        ERR ErrIORead(  const TraceContext&                 tc,
                        const QWORD                         ibOffset,
                        const DWORD                         cbData,
                        __out_bcount( cbData ) BYTE* const  pbData,
                        const OSFILEQOS                     grbitQOS,
                        const PfnIOComplete                 pfnIOComplete   = NULL,
                        const DWORD_PTR                     keyIOComplete   = NULL,
                        const PfnIOHandoff                  pfnIOHandoff    = NULL,
                        const VOID *                        pioreq          = NULL  ) override;
        ERR ErrIOWrite( const TraceContext& tc,
                        const QWORD         ibOffset,
                        const DWORD         cbData,
                        const BYTE* const   pbData,
                        const OSFILEQOS     grbitQOS,
                        const PfnIOComplete pfnIOComplete   = NULL,
                        const DWORD_PTR     keyIOComplete   = NULL,
                        const PfnIOHandoff  pfnIOHandoff    = NULL  ) override;
        ERR ErrIOIssue() override;

        ERR ErrMMRead(  const QWORD     ibOffset,
                        const QWORD     cbSize,
                        void** const    ppvMap ) override;
        ERR ErrMMCopy(  const QWORD     ibOffset,
                        const QWORD     cbSize,
                        void** const    ppvMap ) override;
        ERR ErrMMIORead(    _In_ const QWORD                    ibOffset,
                            _Out_writes_bytes_(cb) BYTE * const pb,
                            _In_ ULONG                          cb,
                            _In_ const FileMmIoReadFlag         fmmiorf ) override;
        ERR ErrMMRevert(    _In_ const QWORD                    ibOffset,
                            _In_reads_bytes_(cbSize)void* const pvMap,
                            _In_ const QWORD                    cbSize ) override;
        ERR ErrMMFree( void* const pvMap ) override;

        VOID RegisterIFilePerfAPI( IFilePerfAPI * const pfpapi ) override
        {
            Assert( NULL != m_p_osf );
            Assert( &g_cosfileperfDefault == m_p_osf->pfpapi );
            m_p_osf->pfpapi = pfpapi;
            pfpapi->Init();
            TraceStationId( TraceStationIdentificationReason::tsidrOpenInit );
        }

        VOID UpdateIFilePerfAPIEngineFileTypeId(    _In_ const DWORD    dwEngineFileType,
                                                    _In_ const QWORD    qwEngineFileId ) override
        {
            Assert( &g_cosfileperfDefault != m_p_osf->pfpapi );

            const BOOL fChangingEngineType = dwEngineFileType != m_p_osf->pfpapi->DwEngineFileType();
            if ( fChangingEngineType )
            {
                m_p_osf->m_posd->SuspendDiskIo();
            }

            m_p_osf->pfpapi->UpdateEngineFileTypeId( dwEngineFileType, qwEngineFileId );
            TraceStationId( TraceStationIdentificationReason::tsidrFileChangeEngineFileType );

            if ( fChangingEngineType )
            {
                m_p_osf->m_posd->ResumeDiskIo();
            }
        }

        ERR ErrNTFSAttributeListSize( QWORD* const pcbSize ) override;

        ERR ErrDiskId( ULONG_PTR* const pulDiskId ) const override;

        LONG64 CioNonFlushed() const override;

        BOOL FSeekPenalty() const override
        {
            return m_posv->FSeekPenalty();
        }

#ifdef DEBUG
        DWORD DwEngineFileType() const override { return m_p_osf->pfpapi->DwEngineFileType(); }
        QWORD QwEngineFileId() const override { return m_p_osf->pfpapi->QwEngineFileId(); }
#endif

        TICK DtickIOElapsed( void* const pvIOContext ) const override;

    private:

        class CIOComplete
        {
            friend VOID DumpOneIOREQ( const IOREQ * const pioreqDebuggee, const IOREQ * const pioreq );

            public:

                CIOComplete()
                    :   m_signal( CSyncBasicInfo( _T( "CIOComplete::m_signal" ) ) ),
                        m_err( JET_errSuccess ),
                        m_tidWait( DwUtilThreadId() ),
                        m_keyIOComplete( NULL ),
                        m_pfnIOHandoff( NULL )
                {
                }

                VOID Complete( const ERR err )
                {

                    m_err = err;


                    m_signal.Set();
                }

                BOOL FComplete()
                {
                    return m_signal.FTryWait();
                }

                VOID Wait()
                {
                    m_signal.Wait();
                }

            public:

                CManualResetSignal  m_signal;
                ERR                 m_err;
                DWORD               m_tidWait;
                DWORD_PTR           m_keyIOComplete;
                PfnIOHandoff        m_pfnIOHandoff;
        };

        friend ERR ErrIOWriteContiguous(    IFileAPI* const                         pfapi,
                                            __in_ecount( cData ) const TraceContext rgtc[],
                                            const QWORD                             ibOffset,
                                            __in_ecount( cData ) const DWORD        rgcbData[],
                                            __in_ecount( cData ) const BYTE* const  rgpbData[],
                                            const size_t                            cData,
                                            const OSFILEQOS                         grbitQOS );
        friend void IOWriteContiguousComplete_( const ERR                   err,
                                                COSFile* const              posf,
                                                const FullTraceContext&     tc,
                                                const OSFILEQOS             grbitQOS,
                                                const QWORD                 ibOffset,
                                                const DWORD                 cbData,
                                                BYTE* const                 pbData,
                                                CIOComplete* const piocomplete );

        class CExtendingWriteRequest
        {
            public:
                static SIZE_T OffsetOfILE() { return OffsetOf( CExtendingWriteRequest, m_ileDefer ); }

            public:

                CInvasiveList< CExtendingWriteRequest, OffsetOfILE >::CElement m_ileDefer;

                COSFile*        m_posf;
                IOREQ*          m_pioreq;
                INT             m_group;
                FullTraceContext m_tc;
                QWORD           m_ibOffset;
                DWORD           m_cbData;
                BYTE*           m_pbData;
                OSFILEQOS       m_grbitQOS;
                PfnIOComplete   m_pfnIOComplete;
                DWORD_PTR       m_keyIOComplete;
                ERR             m_err;

                
                TICK            m_tickReqStart;
                TICK            m_tickReqStep;
                TICK            m_tickReqComplete;
        };

        typedef CInvasiveList< CExtendingWriteRequest, CExtendingWriteRequest::OffsetOfILE > CDeferList;

    private:


        static void IOComplete_(    IOREQ* const    pioreq,
                                    const ERR       err,
                                    COSFile* const  posf );
        void IOComplete( IOREQ* const pioreq, const ERR err );

#ifdef DEBUG
    public:
#endif
        static void IOSyncComplete_(    const ERR           err,
                                        COSFile* const      posf,
                                        const FullTraceContext& tc,
                                        OSFILEQOS           grbitQOS,
                                        const QWORD         ibOffset,
                                        const DWORD         cbData,
                                        BYTE* const         pbData,
                                        CIOComplete* const  piocomplete );
    private:

        static void IOSyncHandoff_( const ERR           err,
                                        COSFile* const      posf,
                                        const FullTraceContext& tc,
                                        const OSFILEQOS     grbitQOS,
                                        const QWORD         ibOffset,
                                        const DWORD         cbData,
                                        const BYTE* const   pbData,
                                        CIOComplete* const  piocomplete,
                                        void* const         pioreq );

        static void IOASyncComplete_(   const ERR           err,
                                        COSFile* const      posf,
                                        const FullTraceContext& tc,
                                        OSFILEQOS           grbitQOS,
                                        const QWORD         ibOffset,
                                        const DWORD         cbData,
                                        BYTE* const         pbData,
                                        CIOComplete* const  piocomplete );
        
        void IOSyncComplete(    const ERR           err,
                                const FullTraceContext& tc,
                                OSFILEQOS           grbitQOS,
                                const QWORD         ibOffset,
                                const DWORD         cbData,
                                BYTE* const         pbData,
                                CIOComplete* const  piocomplete );

        ERR ErrIOAsync( IOREQ* const        pioreq,
                        const BOOL          fWrite,
                        const OSFILEQOS     grbitQOS,
                        const INT           group,
                        const QWORD         ibOffset,
                        const DWORD         cbData,
                        BYTE* const         pbData,
                        const PfnIOComplete pfnIOComplete,
                        const DWORD_PTR     keyIOComplete,
                        const PfnIOHandoff  pfnIOHandoff = NULL );

    private:
        static BOOL FOSFileISyncIOREQ( const IOREQ * const pioreq );
        static BOOL FOSFileIZeroingSyncIOREQ( const IOREQ * const pioreq );
        static BOOL FOSFileIExtendingSyncIOREQ( const IOREQ * const pioreq );
    public:
        static BOOL FOSFileSyncComplete( const IOREQ * const pioreq );

        static void IOZeroingWriteComplete_(    const ERR                       err,
                                                COSFile* const                  posf,
                                                const FullTraceContext&         tc,
                                                const OSFILEQOS                 grbitQOS,
                                                const QWORD                     ibOffset,
                                                const DWORD                     cbData,
                                                BYTE* const                     pbData,
                                                CExtendingWriteRequest* const   pewreq );
        void IOZeroingWriteComplete(    const ERR                       err,
                                        const QWORD                     ibOffset,
                                        const DWORD                     cbData,
                                        BYTE* const                     pbData,
                                        CExtendingWriteRequest* const   pewreq );

        static void IOExtendingWriteComplete_(  const ERR                       err,
                                                COSFile* const                  posf,
                                                const FullTraceContext&         tc,
                                                const OSFILEQOS                 grbitQOS,
                                                const QWORD                     ibOffset,
                                                const DWORD                     cbData,
                                                BYTE* const                     pbData,
                                                CExtendingWriteRequest* const   pewreq );
        void IOExtendingWriteComplete(  const ERR                       err,
                                        const QWORD                     ibOffset,
                                        const DWORD                     cbData,
                                        BYTE* const                     pbData,
                                        CExtendingWriteRequest* const   pewreq );

        static void IOChangeFileSizeComplete_( CExtendingWriteRequest* const pewreq );
        void IOChangeFileSizeComplete( CExtendingWriteRequest* const pewreq );

        ERR ErrIOSetFileSize( const QWORD cbSize,
                            const BOOL fReportError );

        ERR ErrIOSetFileSparse(
           _In_ const HANDLE hfile,
           _In_ const BOOL fReportError );

        ERR ErrIOSetFileRegionSparse(
           _In_ const HANDLE hfile,
           _In_ const QWORD ibOffset,
           _In_ const QWORD cbZeroes,
           _In_ const BOOL fReportError );

        ERR ErrIOGetAllocatedRange(
           _In_ const HANDLE    hfile,
           _In_ const QWORD     ibOffsetToQuery,
           _Out_ QWORD* const   pibStartTrimmedRegion,
           _Out_ QWORD* const   pcbTrimmed );

        BOOL FOSFileTestUrgentIo();

    public:

        static BOOL FOSFileManagementOperation( const IOREQ * const pioreq )
        {
            if ( pioreq->pfnCompletion == PFN( COSFile::IOZeroingWriteComplete_ ) ||
                 pioreq->pfnCompletion == PFN( COSFile::IOExtendingWriteComplete_ ) ||
                 pioreq->pfnCompletion == PFN( COSFile::IOChangeFileSizeComplete_ ) )
            {
                Assert( pioreq->pioreqIorunNext == NULL );
                Assert( pioreq->pioreqVipList == NULL );
                return fTrue;
            }
            Assert( pioreq->cbData != 0 );
            return fFalse;
        }

        void TMPSetNoFlushNeeded();

    private:
        BOOL FDiskFixed() const
        {
            return m_posv->FDiskFixed();
        }

    private:

        COSFileSystem*      m_posfs;
        WCHAR               m_wszAbsPath[ IFileSystemAPI::cchPathMax ];
        HANDLE              m_hFile;
        QWORD               m_cbFileSize;
        DWORD               m_cbIOSize;
        DWORD               m_cbSectorSize;
        FileModeFlags       m_fmf;

        LONG                m_cLogicalCopies;

        _OSFILE*            m_p_osf;
        COSVolume *         m_posv;

        LONG64              m_cioUnflushed;
        LONG64              m_cioFlushing;
        ERR                 m_errFlushFailed;

        CMeteredSection     m_msFileSize;
        QWORD               m_rgcbFileSize[ 2 ];
        HANDLE              m_rghFileMap[ 2 ];
        CSemaphore          m_semChangeFileSize;

        CCriticalSection    m_critDefer;
        CDeferList          m_ilDefer;

        COSEventTraceIdCheck m_traceidcheckFile;

        friend COSVolume * PosvEDBGAccessor( const COSFile * const posf );

};


ERR ErrOSFileIFromWinError_( _In_ const DWORD error, _In_z_ PCSTR szFile, _In_ const LONG lLine );
#define ErrOSFileIFromWinError( error )     ErrOSFileIFromWinError_( error, __FILE__, __LINE__ )
#define ErrOSFileIGetLastError()            ErrOSFileIFromWinError( GetLastError() )

VOID OSFileIIOReportError(
    COSFile * const posf,
    const BOOL fWrite,
    const QWORD ibOffset,
    const DWORD cbLength,
    const ERR err,
    const DWORD errSystem,
    const QWORD cmsecIOElapsed );

#endif


