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
                            MemoryStream^ data,
                            FileQOS fileQOS,
                            IFile::IOComplete^ ioComplete,
                            IFile::IOHandoff^ ioHandoff );

                        virtual void IOWrite(
                            Int64 offsetInBytes,
                            MemoryStream^ data,
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

                    Call( Pi->ErrFlushFileBuffers( (IOFLUSHREASON)0 ) );

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

                    TraceContextScope tcScope( iorpBlockCache );
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

                    TraceContextScope tcScope( iorpBlockCache );
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
                    MemoryStream^ data,
                    FileQOS fileQOS,
                    IFile::IOComplete^ ioComplete,
                    IFile::IOHandoff^ ioHandoff )
                {
                    ERR                 err                 = JET_errSuccess;
                    const DWORD         cbData              = data == nullptr ? 0 : (DWORD)data->Length;
                    void*               pvBuffer            = NULL;
                    IOCompleteInverse^  ioCompleteInverse   = nullptr;
                    TraceContextScope   tcScope( iorpBlockCache );

                    if ( cbData )
                    {
                        Alloc( pvBuffer = PvOSMemoryPageAlloc( roundup( cbData, OSMemoryPageCommitGranularity() ), NULL ) );
                        array<byte>^ bytes = data->ToArray();
                        pin_ptr<const byte> rgbData = &bytes[ 0 ];
                        UtilMemCpy( pvBuffer, (const BYTE*)rgbData, cbData );
                    }

                    if ( ioComplete != nullptr || ioHandoff != nullptr )
                    {
                        ioCompleteInverse = gcnew IOCompleteInverse( this, false, data, ioComplete, ioHandoff, &pvBuffer );
                    }

                    Call( Pi->ErrIORead(    *tcScope,
                                            offsetInBytes,
                                            cbData,
                                            (BYTE*)( ioCompleteInverse == nullptr ? pvBuffer : ioCompleteInverse->PvBuffer ),
                                            (OSFILEQOS)fileQOS,
                                            ioCompleteInverse == nullptr ? NULL : ioCompleteInverse->PfnIOComplete,
                                            ioCompleteInverse == nullptr ? NULL : ioCompleteInverse->KeyIOComplete,
                                            ioCompleteInverse == nullptr ? NULL : ioCompleteInverse->PfnIOHandoff,
                                            NULL ) );

                HandleError:
                    if ( ioComplete == nullptr && cbData )
                    {
                        array<byte>^ bytes = gcnew array<byte>( cbData );
                        pin_ptr<const byte> rgbData = &bytes[ 0 ];
                        UtilMemCpy( (BYTE*)rgbData, ioCompleteInverse == nullptr ? pvBuffer : ioCompleteInverse->PvBuffer, cbData );
                        data->Position = 0;
                        data->Write( bytes, 0, bytes->Length );
                    }
                    OSMemoryPageFree( pvBuffer );
                    if ( ioComplete == nullptr )
                    {
                        delete ioCompleteInverse;
                    }
                    if ( err < JET_errSuccess )
                    {
                        delete ioCompleteInverse;
                        throw EseException( err );
                    }
                }

                template< class TM, class TN, class TW >
                inline void FileBase<TM,TN,TW>::IOWrite(
                    Int64 offsetInBytes,
                    MemoryStream^ data,
                    FileQOS fileQOS,
                    IFile::IOComplete^ ioComplete,
                    IFile::IOHandoff^ ioHandoff )
                {
                    ERR                 err                 = JET_errSuccess;
                    const DWORD         cbData              = data == nullptr ? 0 : (DWORD)data->Length;
                    void*               pvBuffer            = NULL;
                    IOCompleteInverse^  ioCompleteInverse   = nullptr;
                    TraceContextScope   tcScope( iorpBlockCache );

                    if ( cbData )
                    {
                        Alloc( pvBuffer = PvOSMemoryPageAlloc( roundup( cbData, OSMemoryPageCommitGranularity() ), NULL ) );
                        array<byte>^ bytes = data->ToArray();
                        pin_ptr<const byte> rgbData = &bytes[ 0 ];
                        UtilMemCpy( pvBuffer, (const BYTE*)rgbData, cbData );
                    }

                    if ( ioComplete != nullptr || ioHandoff != nullptr )
                    {
                        ioCompleteInverse = gcnew IOCompleteInverse( this, true, data, ioComplete, ioHandoff, &pvBuffer );
                    }

                    Call( Pi->ErrIOWrite(   *tcScope,
                                            offsetInBytes,
                                            cbData,
                                            (const BYTE*)( ioCompleteInverse == nullptr ? pvBuffer : ioCompleteInverse->PvBuffer ),
                                            (OSFILEQOS)fileQOS,
                                            ioCompleteInverse == nullptr ? NULL : ioCompleteInverse->PfnIOComplete,
                                            ioCompleteInverse == nullptr ? NULL : ioCompleteInverse->KeyIOComplete,
                                            ioCompleteInverse == nullptr ? NULL : ioCompleteInverse->PfnIOHandoff ) );

                HandleError:
                    OSMemoryPageFree( pvBuffer );
                    if ( ioComplete == nullptr )
                    {
                        delete ioCompleteInverse;
                    }
                    if ( err < JET_errSuccess )
                    {
                        delete ioCompleteInverse;
                        throw EseException( err );
                    }
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
