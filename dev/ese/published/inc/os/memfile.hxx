// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef _OS_MEMFILE_H_INCLUDED
#error MEMFILE.HXX already included
#endif
#define _OS_MEMFILE_H_INCLUDED

#ifndef JET_errNyi
#define JET_errNyi (-1)
#endif

class CFileFromMemory : public IFileAPI
{
public:

    CFileFromMemory( __in_bcount( cbData ) BYTE *pbData, QWORD cbData, __in PCWSTR wszPath );
    virtual ~CFileFromMemory();

    FileModeFlags Fmf() const override { AssertSz( fFalse, "NYI!" ); return IFileAPI::fmfNone; }
    ERR ErrPath( _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const wszAbsPath ) override;
    ERR ErrSize(
            _Out_ QWORD* const pcbSize,
            _In_ const FILESIZE file ) override;
    ERR ErrIsReadOnly( BOOL* const pfReadOnly ) override;
    LONG CLogicalCopies() override { return 1; }
    ERR ErrSetSize( const TraceContext& tc,
                            const QWORD         cbSize,
                            const BOOL          fZeroFill,
                            const OSFILEQOS     grbitQOS ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrRename(  const WCHAR* const  wszAbsPathDest,
                            const BOOL          fOverwriteExisting = fFalse ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrSetSparseness() override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrIOTrim( const TraceContext&  tc,
                           const QWORD          ibOffset,
                           const QWORD          cbToFree
                           ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrRetrieveAllocatedRegion( const QWORD     ibOffsetToQuery,
                                       _Out_ QWORD* const   pibStartTrimmedRegion,
                                       _Out_ QWORD* const   pcbTrimmed ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrFlushFileBuffers( const IOFLUSHREASON iofr ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    void SetNoFlushNeeded() override { AssertSz( fFalse, "NYI!" ); }
    ERR ErrIOSize( DWORD* const pcbSize ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrSectorSize( DWORD* const pcbSize ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrReserveIOREQ( const QWORD            ibOffset,
                                 const DWORD            cbData,
                                 const OSFILEQOS        grbitQOS,
                                       VOID **          ppioreq ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    VOID ReleaseUnusedIOREQ( VOID * pioreq ) override { AssertSz( fFalse, "NYI!" ); }
    ERR ErrIORead(  const TraceContext& tc,
                            const QWORD         ibOffset,
                            const DWORD         cbData,
    __out_bcount( cbData )  BYTE* const         pbData,
                            const OSFILEQOS     grbitQOS,
                            const PfnIOComplete pfnIOComplete   = NULL,
                            const DWORD_PTR     keyIOComplete   = NULL,
                            const PfnIOHandoff  pfnIOHandoff    = NULL,
                            const VOID *        pioreq          = NULL  ) override;
    ERR ErrIOWrite( const TraceContext& tc,
                            const QWORD         ibOffset,
                            const DWORD         cbData,
                            const BYTE* const       pbData,
                            const OSFILEQOS     grbitQOS,
                            const PfnIOComplete pfnIOComplete   = NULL,
                            const DWORD_PTR     keyIOComplete   = NULL,
                            const PfnIOHandoff  pfnIOHandoff    = NULL ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrIOIssue() override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrMMRead(  const QWORD     ibOffset,
                            const QWORD     cbSize,
                            void** const    ppvMap ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrMMCopy(  const QWORD     ibOffset,
                            const QWORD     cbSize,
                            void** const    ppvMap ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrMMIORead( _In_                   const QWORD         ibOffset,
                             _Out_writes_bytes_(cb) BYTE * const            pb,
                             _In_                   ULONG                   cb,
                             _In_                   const FileMmIoReadFlag fmmiorf ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrMMRevert( _In_                   const QWORD             ibOffset,
                             _In_reads_bytes_(cbSize)void* const            pvMap,
                             _In_                   const QWORD             cbSize ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrMMFree( void* const pvMap ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    VOID RegisterIFilePerfAPI( IFilePerfAPI * const pfpapi ) override { AssertSz( fFalse, "NYI!" ); }
    VOID UpdateIFilePerfAPIEngineFileTypeId( _In_ const DWORD dwEngineFileType, _In_ const QWORD qwEngineFileId ) override { AssertSz( fFalse, "NYI!" ); }
    ERR ErrNTFSAttributeListSize( QWORD* const pcbSize ) override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    ERR ErrDiskId( ULONG_PTR* const pulDiskId ) const override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    LONG64 CioNonFlushed() const override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }
    BOOL FSeekPenalty() const override { AssertSz( fFalse, "NYI!" ); return ErrERRCheck( JET_errNyi ); }

#ifdef DEBUG
    DWORD DwEngineFileType() const override { AssertSz( fFalse, "NYI!" ); return 0; }
    QWORD QwEngineFileId() const override {  AssertSz( fFalse, "NYI!" ); return 0; }
#endif

    TICK DtickIOElapsed( void* const pvIOContext ) const override { AssertSz( fFalse, "NYI!" ); return 0; }

private:
    BYTE  *m_pbBuffer;
    QWORD  m_cbBuffer;
    BYTE  *m_pbBufferToFree;
    WCHAR  m_wszPath[ OSFSAPI_MAX_PATH ];
};



