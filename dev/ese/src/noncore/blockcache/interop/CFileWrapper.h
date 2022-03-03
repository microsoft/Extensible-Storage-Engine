// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                template< class TM, class TN >
                class CFileWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CFileWrapper( TM^ f ) : CWrapper( f ) { }

                    public:

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
                                        const IFileAPI::PfnIOComplete       pfnIOComplete   = NULL,
                                        const DWORD_PTR                     keyIOComplete   = NULL,
                                        const IFileAPI::PfnIOHandoff        pfnIOHandoff    = NULL,
                                        const VOID *                        pioreq          = NULL  ) override;
                        ERR ErrIOWrite( const TraceContext&             tc,
                                        const QWORD                     ibOffset,
                                        const DWORD                     cbData,
                                        const BYTE* const               pbData,
                                        const OSFILEQOS                 grbitQOS,
                                        const IFileAPI::PfnIOComplete   pfnIOComplete   = NULL,
                                        const DWORD_PTR                 keyIOComplete   = NULL,
                                        const IFileAPI::PfnIOHandoff    pfnIOHandoff    = NULL  ) override;
                        ERR ErrIOIssue() override;

                        ERR ErrMMRead(  const QWORD     ibOffset,
                                        const QWORD     cbSize,
                                        void** const    ppvMap ) override;
                        ERR ErrMMCopy(  const QWORD     ibOffset,
                                        const QWORD     cbSize,
                                        void** const    ppvMap ) override;
                        ERR ErrMMIORead(    _In_ const QWORD                        ibOffset,
                                            _Out_writes_bytes_(cb) BYTE * const     pb,
                                            _In_ ULONG                              cb,
                                            _In_ const IFileAPI::FileMmIoReadFlag   fmmiorf ) override;
                        ERR ErrMMRevert(    _In_ const QWORD                    ibOffset,
                                            _In_reads_bytes_(cbSize)void* const pvMap,
                                            _In_ const QWORD                    cbSize ) override;
                        ERR ErrMMFree( void* const pvMap ) override;

                        VOID RegisterIFilePerfAPI( IFilePerfAPI * const pfpapi ) override;

                        VOID UpdateIFilePerfAPIEngineFileTypeId(    _In_ const DWORD    dwEngineFileType,
                                                                    _In_ const QWORD    qwEngineFileId ) override;

                        ERR ErrNTFSAttributeListSize( QWORD* const pcbSize ) override;

                        ERR ErrDiskId( ULONG_PTR* const pulDiskId ) const override;

                        LONG64 CioNonFlushed() const override;

                        BOOL FSeekPenalty() const override;

#ifdef DEBUG
                        DWORD DwEngineFileType() const override;
                        QWORD QwEngineFileId() const override;
#endif

                        TICK DtickIOElapsed( void* const pvIOContext ) override;
                };

                template< class TM, class TN >
                inline IFileAPI::FileModeFlags CFileWrapper<TM, TN>::Fmf() const
                {
                    return (IFileAPI::FileModeFlags)I()->FileModeFlags();
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrFlushFileBuffers( const IOFLUSHREASON iofr )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->FlushFileBuffers() );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline void CFileWrapper<TM, TN>::SetNoFlushNeeded()
                {
                    I()->SetNoFlushNeeded();
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrPath( _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath )
                {
                    ERR     err     = JET_errSuccess;
                    String^ absPath = nullptr;

                    wszAbsPath[ 0 ] = L'\0';

                    ExCall( absPath = I()->Path() );

                    pin_ptr<const Char> wszAbsPathT = PtrToStringChars( absPath );
                    Call( ErrOSStrCbCopyW( wszAbsPath, cbOSFSAPI_MAX_PATHW, (STRSAFE_LPCWSTR)wszAbsPathT ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        wszAbsPath[ 0 ] = L'\0';
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrSize(   _Out_ QWORD* const              pcbSize,
                                                            _In_ const IFileAPI::FILESIZE   filesize )
                {
                    ERR     err         = JET_errSuccess;
                    Int64   sizeInBytes = 0;

                    *pcbSize = fFalse;

                    ExCall( sizeInBytes = I()->Size( (FileSize)filesize ) );

                    *pcbSize = sizeInBytes;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pcbSize = 0;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrIsReadOnly( BOOL* const pfReadOnly )
                {
                    ERR     err         = JET_errSuccess;
                    bool    isReadOnly  = false;

                    *pfReadOnly = fFalse;

                    ExCall( isReadOnly = I()->IsReadOnly() );

                    *pfReadOnly = isReadOnly ? fTrue : fFalse;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pfReadOnly = fFalse;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline LONG CFileWrapper<TM, TN>::CLogicalCopies()
                {
                    return I()->CountLogicalCopies();
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrSetSize(    const TraceContext& tc,
                                                                const QWORD         cbSize,
                                                                const BOOL          fZeroFill,
                                                                const OSFILEQOS     grbitQOS )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->SetSize( cbSize, fZeroFill ? true : false, (FileQOS)grbitQOS ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrRename( const WCHAR* const  wszPathDest,
                                                            const BOOL          fOverwriteExisting )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->Rename( gcnew String( wszPathDest ), fOverwriteExisting ? true : false ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrSetSparseness()
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->SetSparseness() );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrIOTrim( const TraceContext& tc,
                                                            const QWORD         ibOffset,
                                                            const QWORD         cbToFree )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->IOTrim( ibOffset, cbToFree ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrRetrieveAllocatedRegion(    const QWORD         ibOffsetToQuery,
                                                                                _Out_ QWORD* const  pibStartTrimmedRegion,
                                                                                _Out_ QWORD* const  pcbTrimmed )
                {
                    ERR     err             = JET_errSuccess;
                    Int64   offsetInBytes   = 0;
                    Int64   sizeInBytes     = 0;

                    *pibStartTrimmedRegion = 0;
                    *pcbTrimmed = 0;

                    ExCall( I()->RetrieveAllocatedRegion( ibOffsetToQuery, offsetInBytes, sizeInBytes ) );

                    *pibStartTrimmedRegion = offsetInBytes;
                    *pcbTrimmed = sizeInBytes;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pibStartTrimmedRegion = 0;
                        *pcbTrimmed = 0;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrIOSize( DWORD* const pcbSize )
                {
                    ERR err         = JET_errSuccess;
                    int sizeInBytes = 0;

                    *pcbSize = 0;

                    ExCall( sizeInBytes = I()->IOSize() );

                    *pcbSize = sizeInBytes;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pcbSize = 0;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrSectorSize( DWORD* const pcbSize )
                {
                    ERR err         = JET_errSuccess;
                    int sizeInBytes = 0;

                    *pcbSize = 0;

                    ExCall( sizeInBytes = I()->SectorSize() );

                    *pcbSize = sizeInBytes;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pcbSize = 0;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrReserveIOREQ(   const QWORD     ibOffset,
                                                                    const DWORD     cbData,
                                                                    const OSFILEQOS grbitQOS,
                                                                    VOID **         ppioreq )
                {
                    return ErrERRCheck( JET_errFeatureNotAvailable );
                }

                template< class TM, class TN >
                inline VOID CFileWrapper<TM, TN>::ReleaseUnusedIOREQ( VOID * pioreq )
                {
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrIORead( const TraceContext&                 tc,
                                                            const QWORD                         ibOffset,
                                                            const DWORD                         cbData,
                                                            __out_bcount( cbData ) BYTE* const  pbData,
                                                            const OSFILEQOS                     grbitQOS,
                                                            const IFileAPI::PfnIOComplete       pfnIOComplete,
                                                            const DWORD_PTR                     keyIOComplete,
                                                            const IFileAPI::PfnIOHandoff        pfnIOHandoff,
                                                            const VOID *                        pioreq )
                {
                    ERR             err         = JET_errSuccess;
                    array<byte>^    buffer      = !pbData ? nullptr : gcnew array<byte>( cbData );
                    pin_ptr<byte>   rgbData     = ( !pbData || !cbData ) ? nullptr : &buffer[ 0 ];
                    MemoryStream^   stream      = !pbData ? nullptr : gcnew MemoryStream( buffer, true );
                    IOComplete^     iocomplete  = ( pfnIOComplete || pfnIOHandoff ) ?
                                                        gcnew IOComplete(   this,
                                                                            false, 
                                                                            pbData, 
                                                                            pfnIOComplete, 
                                                                            keyIOComplete, 
                                                                            pfnIOHandoff ) :
                                                        nullptr;

                    if ( cbData )
                    {
                        UtilMemCpy( (BYTE*)rgbData, pbData, cbData );
                    }

                    ExCall( I()->IORead(    ibOffset,
                                            stream,
                                            (FileQOS)grbitQOS,
                                            pfnIOComplete ?
                                                gcnew IFile::IOComplete( iocomplete, &IOComplete::Complete ) :
                                                nullptr,
                                            iocomplete != nullptr ?
                                                gcnew IFile::IOHandoff( iocomplete, &IOComplete::Handoff ) :
                                                nullptr ) );

                HandleError:
                    if ( !pfnIOComplete && cbData )
                    {
                        UtilMemCpy( pbData, (const BYTE*)rgbData, cbData );
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrIOWrite(    const TraceContext&             tc,
                                                                const QWORD                     ibOffset,
                                                                const DWORD                     cbData,
                                                                const BYTE* const               pbData,
                                                                const OSFILEQOS                 grbitQOS,
                                                                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                                                const DWORD_PTR                 keyIOComplete,
                                                                const IFileAPI::PfnIOHandoff    pfnIOHandoff )
                {
                    ERR             err         = JET_errSuccess;
                    array<byte>^    buffer      = !pbData ? nullptr : gcnew array<byte>( cbData );
                    pin_ptr<byte>   rgbData     = ( !pbData || !cbData ) ? nullptr : &buffer[ 0 ];
                    MemoryStream^   stream      = !pbData ? nullptr : gcnew MemoryStream( buffer, false );
                    IOComplete^     iocomplete  = ( pfnIOComplete || pfnIOHandoff ) ?
                                                        gcnew IOComplete(   this,
                                                                            true, 
                                                                            const_cast<BYTE*>( pbData ), 
                                                                            pfnIOComplete, 
                                                                            keyIOComplete, 
                                                                            pfnIOHandoff ) :
                                                        nullptr;

                    if ( cbData )
                    {
                        UtilMemCpy( (BYTE*)rgbData, pbData, cbData );
                    }

                    ExCall( I()->IOWrite(   ibOffset,
                                            stream,
                                            (FileQOS)grbitQOS,
                                            pfnIOComplete ?
                                                gcnew IFile::IOComplete( iocomplete, &IOComplete::Complete ) :
                                                nullptr,
                                            iocomplete != nullptr ?
                                                gcnew IFile::IOHandoff( iocomplete, &IOComplete::Handoff ) :
                                                nullptr ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrIOIssue()
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->IOIssue() );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrMMRead( const QWORD     ibOffset,
                                                            const QWORD     cbSize,
                                                            void** const    ppvMap )
                {
                    return ErrERRCheck( JET_errFeatureNotAvailable );
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrMMCopy( const QWORD     ibOffset,
                                                            const QWORD     cbSize,
                                                            void** const    ppvMap )
                {
                    return ErrERRCheck( JET_errFeatureNotAvailable );
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrMMIORead(   _In_ const QWORD                        ibOffset,
                                                                _Out_writes_bytes_( cb ) BYTE * const     pb,
                                                                _In_ ULONG                              cb,
                                                                _In_ const IFileAPI::FileMmIoReadFlag   fmmiorf )
                {
                    return ErrERRCheck( JET_errFeatureNotAvailable );
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrMMRevert(   _In_ const QWORD                    ibOffset,
                                                                _In_reads_bytes_( cbSize )void* const pvMap,
                                                                _In_ const QWORD                    cbSize )
                {
                    return ErrERRCheck( JET_errFeatureNotAvailable );
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrMMFree( void* const pvMap )
                {
                    return ErrERRCheck( JET_errFeatureNotAvailable );
                }

                template< class TM, class TN >
                inline VOID CFileWrapper<TM, TN>::RegisterIFilePerfAPI( IFilePerfAPI * const pfpapi )
                {
                }

                template< class TM, class TN >
                inline VOID CFileWrapper<TM, TN>::UpdateIFilePerfAPIEngineFileTypeId(   _In_ const DWORD    dwEngineFileType,
                                                                                        _In_ const QWORD    qwEngineFileId )
                {
                }

                template< class TM, class TN >
                inline ERR CFileWrapper<TM, TN>::ErrNTFSAttributeListSize( QWORD* const pcbSize )
                {
                    ERR     err         = JET_errSuccess;
                    Int64   sizeInBytes = 0;

                    *pcbSize = 0;

                    ExCall( sizeInBytes = I()->NTFSAttributeListSize() );

                    *pcbSize = sizeInBytes;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pcbSize = 0;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline  ERR CFileWrapper<TM, TN>::ErrDiskId( ULONG_PTR* const pulDiskId ) const
                {
                    ERR     err     = JET_errSuccess;
                    IntPtr  diskId  = IntPtr::Zero;

                    *pulDiskId = 0;

                    ExCall( diskId = I()->DiskId() );

                    *pulDiskId = (Int64)diskId;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pulDiskId = 0;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline LONG64 CFileWrapper<TM, TN>::CioNonFlushed() const
                {
                    return I()->CountIoNonFlushed();
                }

                template< class TM, class TN >
                inline BOOL CFileWrapper<TM, TN>::FSeekPenalty() const
                {
                    return I()->SeekPenalty() ? fTrue : fFalse;
                }

#ifdef DEBUG
                template< class TM, class TN >
                inline DWORD CFileWrapper<TM, TN>::DwEngineFileType() const
                {
                    return 0;
                }

                template< class TM, class TN >
                inline QWORD CFileWrapper<TM, TN>::QwEngineFileId() const
                {
                    return 0;
                }
#endif

                template< class TM, class TN >
                inline TICK CFileWrapper<TM, TN>::DtickIOElapsed( void* const pvIOContext )
                {
                    return 0;
                }
            }
        }
    }
}
