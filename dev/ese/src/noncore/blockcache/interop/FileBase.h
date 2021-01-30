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
                template< class TM, class TN, class TW >
                public ref class FileBase : public Base<TM,TN,TW>, IFile
                {
                    public:

                        FileBase( TM^ f ) : Base( f ) { }

                        FileBase( TN** const ppfapi ) : Base( ppfapi ) {}

                        FileBase( TN* const pfapi ) : Base( pfapi ) {}

                    public:

                        virtual FileModeFlags FileModeFlags();

                        virtual void FlushFileBuffers();

                        virtual void SetNoFlushNeeded();

                        virtual String^ Path();

                        virtual Int64 Size( FileSize fileSize );

                        virtual bool IsReadOnly();

                        virtual int CountLogicalCopies();

                        virtual void SetSize( Int64 sizeInBytes, bool zeroFill, FileQOS fileQOS );

                        virtual void Rename( String^ pathDest, bool overwriteExisting );

                        virtual void SetSparseness();

                        virtual void IOTrim( Int64 offsetInBytes, Int64 sizeInBytes );

                        virtual void RetrieveAllocatedRegion(
                            Int64 offsetToQuery,
                            [Out] Int64% offsetInBytes,
                            [Out] Int64% sizeInBytes );

                        virtual int IOSize();

                        virtual int SectorSize();

                        virtual void IORead(
                            Int64 offsetInBytes,
                            ArraySegment<byte> data,
                            FileQOS fileQOS,
                            IFile::IOComplete^ ioComplete,
                            IFile::IOHandoff^ ioHandoff );

                        virtual void IOWrite(
                            Int64 offsetInBytes,
                            ArraySegment<byte> data,
                            FileQOS fileQOS,
                            IFile::IOComplete^ ioComplete,
                            IFile::IOHandoff^ ioHandoff );

                        virtual void IOIssue();

                        virtual Int64 NTFSAttributeListSize();

                        virtual IntPtr DiskId();

                        virtual Int64 CountIoNonFlushed();

                        virtual bool SeekPenalty();
                };

                template< class TM, class TN, class TW >
                inline FileModeFlags FileBase<TM,TN,TW>::FileModeFlags()
                {
                    return (Interop::FileModeFlags)Pi->Fmf();
                }

                template< class TM, class TN, class TW >
                inline void FileBase<TM,TN,TW>::FlushFileBuffers()
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrFlushFileBuffers( (IOFLUSHREASON)0) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileBase<TM,TN,TW>::SetNoFlushNeeded()
                {
                    Pi->SetNoFlushNeeded();
                }

                template< class TM, class TN, class TW >
                inline String^ FileBase<TM,TN,TW>::Path()
                {
                    ERR     err                                 = JET_errSuccess;
                    WCHAR   wszAbsolutePath[ OSFSAPI_MAX_PATH ] = { 0 };

                    Call( Pi->ErrPath( wszAbsolutePath ) );

                    return gcnew String( wszAbsolutePath );

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline Int64 FileBase<TM,TN,TW>::Size( FileSize fileSize )
                {
                    ERR     err         = JET_errSuccess;
                    QWORD   cbSize      = 0;

                    Call( Pi->ErrSize( &cbSize, (IFileAPI::FILESIZE)fileSize ) );

                    return cbSize;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline bool FileBase<TM,TN,TW>::IsReadOnly()
                {
                    ERR     err         = JET_errSuccess;
                    BOOL    fReadOnly   = fFalse;

                    Call( Pi->ErrIsReadOnly( &fReadOnly ) );

                    return fReadOnly ? true : false;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline int FileBase<TM,TN,TW>::CountLogicalCopies()
                {
                    return Pi->CLogicalCopies();
                }

                template< class TM, class TN, class TW >
                inline void FileBase<TM,TN,TW>::SetSize( Int64 sizeInBytes, bool zeroFill, FileQOS fileQOS )
                {
                    ERR err = JET_errSuccess;

                    TraceContextScope tcScope;
                    Call( Pi->ErrSetSize(   *tcScope,
                                            sizeInBytes, 
                                            zeroFill ? fTrue : fFalse, 
                                            (OSFILEQOS)fileQOS ) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileBase<TM,TN,TW>::Rename( String^ pathDest, bool overwriteExisting )
                {
                    ERR err = JET_errSuccess;

                    pin_ptr<const Char> wszPathDest = PtrToStringChars( pathDest );
                    Call( Pi->ErrRename( (const WCHAR*)wszPathDest, overwriteExisting ? fTrue : fFalse ) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileBase<TM,TN,TW>::SetSparseness()
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrSetSparseness() );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileBase<TM,TN,TW>::IOTrim( Int64 offsetInBytes, Int64 sizeInBytes )
                {
                    ERR err = JET_errSuccess;

                    TraceContextScope tcScope;
                    Call( Pi->ErrIOTrim( *tcScope, offsetInBytes, sizeInBytes ) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileBase<TM,TN,TW>::RetrieveAllocatedRegion(
                    Int64 offsetToQuery,
                    [Out] Int64% offsetInBytes,
                    [Out] Int64% sizeInBytes )
                {
                    ERR     err                     = JET_errSuccess;
                    QWORD   ibStartTrimmedRegion    = 0;
                    QWORD   cbTrimmed               = 0;

                    offsetInBytes = 0;
                    sizeInBytes = 0;

                    Call( Pi->ErrRetrieveAllocatedRegion( offsetToQuery, &ibStartTrimmedRegion, &cbTrimmed ) );

                    offsetInBytes = ibStartTrimmedRegion;
                    sizeInBytes = cbTrimmed;

                    return;

                HandleError:
                    offsetInBytes = 0;
                    sizeInBytes = 0;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline int FileBase<TM,TN,TW>::IOSize()
                {
                    ERR     err     = JET_errSuccess;
                    DWORD   cbSize  = 0;

                    Call( Pi->ErrIOSize( &cbSize ) );

                    return cbSize;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline int FileBase<TM,TN,TW>::SectorSize()
                {
                    ERR     err     = JET_errSuccess;
                    DWORD   cbSize  = 0;

                    Call( Pi->ErrSectorSize( &cbSize ) );

                    return cbSize;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileBase<TM,TN,TW>::IORead(
                    Int64 offsetInBytes,
                    ArraySegment<byte> data,
                    FileQOS fileQOS,
                    IFile::IOComplete^ ioComplete,
                    IFile::IOHandoff^ ioHandoff )
                {
                    ERR                 err             = JET_errSuccess;
                    const DWORD         cbBlock         = OSMemoryPageCommitGranularity();
                    DWORD               cbBuffer        = ( ( data.Count - 1 + cbBlock ) / cbBlock ) * cbBlock;
                    VOID*               pvBuffer        = NULL;
                    BOOL                fReleaseBuffer  = fTrue;
                    TraceContextScope   tcScope;
                    CIOComplete*        piocomplete     = NULL;

                    Alloc( pvBuffer = PvOSMemoryPageAlloc( cbBuffer, NULL ) );

                    if ( ioComplete != nullptr || ioHandoff != nullptr )
                    {
                        if ( ioComplete != nullptr )
                        {
                            Alloc( piocomplete = new CIOComplete( fTrue, this, fFalse, data.Array, data.Offset, data.Count, ioComplete, ioHandoff, pvBuffer ) );
                        }
                        else
                        {
                            piocomplete = new( _alloca( sizeof( CIOComplete ) ) ) CIOComplete( fFalse, this, fFalse, data.Array, data.Offset, data.Count, ioComplete, ioHandoff, pvBuffer );
                        }

                        fReleaseBuffer = fFalse;
                    }

                    pin_ptr<const byte> rgbDataIn = &data.Array[ data.Offset ];
                    UtilMemCpy( pvBuffer, (const BYTE*)rgbDataIn, data.Count );

                    Call( Pi->ErrIORead(    *tcScope,
                                            offsetInBytes,
                                            data.Count,
                                            (BYTE*)pvBuffer,
                                            (OSFILEQOS)fileQOS,
                                            ioComplete != nullptr ? IFileAPI::PfnIOComplete( CIOComplete::Complete ) : NULL,
                                            DWORD_PTR( piocomplete ),
                                            piocomplete ? IFileAPI::PfnIOHandoff( CIOComplete::Handoff ) : NULL,
                                            NULL ) );

                HandleError:
                    if ( ioComplete == nullptr )
                    {
                        pin_ptr<byte> rgbData = &data.Array[ data.Offset ];
                        UtilMemCpy( (BYTE*)rgbData, pvBuffer, data.Count );
                    }
                    if ( piocomplete && ( ioComplete == nullptr || err < JET_errSuccess ) )
                    {
                        piocomplete->Release();
                    }
                    if ( fReleaseBuffer )
                    {
                        OSMemoryPageFree( pvBuffer );
                    }
                    if ( err >= JET_errSuccess )
                    {
                        return;
                    }
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileBase<TM,TN,TW>::IOWrite(
                    Int64 offsetInBytes,
                    ArraySegment<byte> data,
                    FileQOS fileQOS,
                    IFile::IOComplete^ ioComplete,
                    IFile::IOHandoff^ ioHandoff )
                {
                    ERR                 err             = JET_errSuccess;
                    const DWORD         cbBlock         = OSMemoryPageCommitGranularity();
                    DWORD               cbBuffer        = ( ( data.Count - 1 + cbBlock ) / cbBlock ) * cbBlock;
                    VOID*               pvBuffer        = NULL;
                    BOOL                fReleaseBuffer  = fTrue;
                    TraceContextScope   tcScope;
                    CIOComplete*        piocomplete     = NULL;

                    Alloc( pvBuffer = PvOSMemoryPageAlloc( cbBuffer, NULL ) );

                    if ( ioComplete != nullptr || ioHandoff != nullptr )
                    {
                        if ( ioComplete != nullptr )
                        {
                            Alloc( piocomplete = new CIOComplete( fTrue, this, fTrue, data.Array, data.Offset, data.Count, ioComplete, ioHandoff, pvBuffer ) );
                        }
                        else
                        {
                            piocomplete = new( _alloca( sizeof( CIOComplete ) ) ) CIOComplete( fFalse, this, fTrue, data.Array, data.Offset, data.Count, ioComplete, ioHandoff, pvBuffer );
                        }

                        fReleaseBuffer = fFalse;
                    }

                    pin_ptr<const byte> rgbData = &data.Array[ data.Offset ];
                    UtilMemCpy( pvBuffer, (const BYTE*)rgbData, data.Count );

                    Call( Pi->ErrIOWrite(   *tcScope,
                                            offsetInBytes,
                                            data.Count,
                                            (const BYTE*)pvBuffer,
                                            (OSFILEQOS)fileQOS,
                                            ioComplete != nullptr ? IFileAPI::PfnIOComplete( CIOComplete::Complete ) : NULL,
                                            DWORD_PTR( piocomplete ),
                                            piocomplete ? IFileAPI::PfnIOHandoff( CIOComplete::Handoff ) : NULL ) );

                HandleError:
                    if ( piocomplete && ( ioComplete == nullptr || err < JET_errSuccess ) )
                    {
                        piocomplete->Release();
                    }
                    if ( fReleaseBuffer )
                    {
                        OSMemoryPageFree( pvBuffer );
                    }
                    if ( err >= JET_errSuccess )
                    {
                        return;
                    }
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileBase<TM,TN,TW>::IOIssue()
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrIOIssue() );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline Int64 FileBase<TM,TN,TW>::NTFSAttributeListSize()
                {
                    ERR     err     = JET_errSuccess;
                    QWORD   cbSize  = 0;

                    Call( Pi->ErrNTFSAttributeListSize( &cbSize ) );

                    return cbSize;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline IntPtr FileBase<TM,TN,TW>::DiskId()
                {
                    ERR         err         = JET_errSuccess;
                    ULONG_PTR   ulDiskId    = 0;

                    Call( Pi->ErrDiskId( &ulDiskId ) );

                    return IntPtr( (Int64)ulDiskId );

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline Int64 FileBase<TM,TN,TW>::CountIoNonFlushed()
                {
                    return Pi->CioNonFlushed();
                }

                template< class TM, class TN, class TW >
                inline bool FileBase<TM,TN,TW>::SeekPenalty()
                {
                    return Pi->FSeekPenalty() ? true : false;
                }
            }
        }
    }
}
