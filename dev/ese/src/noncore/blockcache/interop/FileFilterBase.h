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
                public ref class FileFilterBase : public FileBase<TM,TN,TW>, IFileFilter
                {
                    public:

                        FileFilterBase( TM^ ff ) : FileBase( ff ) { }

                        FileFilterBase( TN** const ppff ) : FileBase( ppff ) { }

                    public:

                        virtual void GetPhysicalId(
                            [Out] VolumeId% volumeid,
                            [Out] FileId% fileid, 
                            [Out] FileSerial% fileserial );

                        virtual void Read(
                            Int64 offsetInBytes,
                            MemoryStream^ data,
                            FileQOS fileQOS,
                            IOMode ioMode,
                            IFile::IOComplete^ ioComplete,
                            IFile::IOHandoff^ ioHandoff );

                        virtual void Write(
                            Int64 offsetInBytes,
                            MemoryStream^ data,
                            FileQOS fileQOS,
                            IOMode ioMode,
                            IFile::IOComplete^ ioComplete,
                            IFile::IOHandoff^ ioHandoff );

                        virtual void Issue( IOMode ioMode );

                        virtual void Flush( IOMode ioMode );
                };

                template< class TM, class TN, class TW >
                inline void FileFilterBase<TM, TN, TW>::GetPhysicalId(
                    [Out] VolumeId% volumeid,
                    [Out] FileId% fileid,
                    [Out] FileSerial% fileserial )
                {
                    ERR             err                 = JET_errSuccess;
                    ::VolumeId      volumeidNative      = ::volumeidInvalid;
                    ::FileId        fileidNative        = ::fileidInvalid;
                    ::FileSerial    fileserialNative    = ::fileserialInvalid;

                    volumeid = VolumeId::Invalid;
                    fileid = FileId::Invalid;
                    fileserial = FileSerial::Invalid;

                    Call( Pi->ErrGetPhysicalId( &volumeidNative, &fileidNative, &fileserialNative ) );

                    volumeid = (VolumeId)volumeidNative;
                    fileid = (FileId)(Int64)fileidNative;
                    fileserial = (FileSerial)fileserialNative;

                    return;

                HandleError:
                    volumeid = VolumeId::Invalid;
                    fileid = FileId::Invalid;
                    fileserial = FileSerial::Invalid;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileFilterBase<TM,TN,TW>::Read(
                    Int64 offsetInBytes,
                    MemoryStream^ data,
                    FileQOS fileQOS,
                    IOMode ioMode,
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
                        array<BYTE>^ bytes = data->ToArray();
                        pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                        UtilMemCpy( pvBuffer, (const BYTE*)rgbData, cbData );
                    }

                    if ( ioComplete != nullptr || ioHandoff != nullptr )
                    {
                        ioCompleteInverse = gcnew IOCompleteInverse( this, false, data, ioComplete, ioHandoff, &pvBuffer );
                    }

                    Call( Pi->ErrRead(  *tcScope,
                                        offsetInBytes,
                                        cbData,
                                        (BYTE*)( ioCompleteInverse == nullptr ? pvBuffer : ioCompleteInverse->PvBuffer ),
                                        (OSFILEQOS)fileQOS,
                                        (::IFileFilter::IOMode)ioMode,
                                        ioCompleteInverse == nullptr ? NULL : ioCompleteInverse->PfnIOComplete,
                                        ioCompleteInverse == nullptr ? NULL : ioCompleteInverse->KeyIOComplete,
                                        ioCompleteInverse == nullptr ? NULL : ioCompleteInverse->PfnIOHandoff,
                                        NULL ) );

                HandleError:
                    if ( ioComplete == nullptr && cbData )
                    {
                        array<BYTE>^ bytes = gcnew array<BYTE>( cbData );
                        pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
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
                inline void FileFilterBase<TM,TN,TW>::Write(
                    Int64 offsetInBytes,
                    MemoryStream^ data,
                    FileQOS fileQOS,
                    IOMode ioMode,
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
                        array<BYTE>^ bytes = data->ToArray();
                        pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                        UtilMemCpy( pvBuffer, (const BYTE*)rgbData, cbData );
                    }

                    if ( ioComplete != nullptr || ioHandoff != nullptr )
                    {
                        ioCompleteInverse = gcnew IOCompleteInverse( this, true, data, ioComplete, ioHandoff, &pvBuffer );
                    }

                    Call( Pi->ErrWrite( *tcScope,
                                        offsetInBytes,
                                        cbData,
                                        (const BYTE*)( ioCompleteInverse == nullptr ? pvBuffer : ioCompleteInverse->PvBuffer ),
                                        (OSFILEQOS)fileQOS,
                                        (::IFileFilter::IOMode)ioMode,
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
                inline void FileFilterBase<TM, TN, TW>::Issue( IOMode ioMode )
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrIssue( (::IFileFilter::IOMode)ioMode ) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileFilterBase<TM, TN, TW>::Flush( IOMode ioMode )
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrFlush( (IOFLUSHREASON)0, (::IFileFilter::IOMode)ioMode ) );

                    return;

                HandleError:
                    throw EseException( err );
                }
            }
        }
    }
}
