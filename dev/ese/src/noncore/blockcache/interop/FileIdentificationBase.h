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
                public ref class FileIdentificationBase : public Base<TM, TN, TW>, IFileIdentification
                {
                    public:

                        FileIdentificationBase( TM^ fident ) : Base( fident ) { }

                        FileIdentificationBase( TN** const ppfident ) : Base( ppfident ) {}

                    public:

                        virtual void GetFileId( String^ path, [Out] VolumeId% volumeid, [Out] FileId% fileid );

                        virtual String^ GetFileKeyPath( String^ path );

                        virtual void GetFilePathById(
                            VolumeId volumeid,
                            FileId fileid,
                            [Out] String^% anyAbsPath,
                            [Out] String^% keyPath );
                };
#pragma warning (disable: 4702) // C4702: unreachable code
                template< class TM, class TN, class TW >
                inline void FileIdentificationBase<TM, TN, TW>::GetFileId(
                    String^ path, 
                    [Out] VolumeId% volumeid, 
                    [Out] FileId% fileid )
                {
                    ERR         err             = JET_errSuccess;
                    ::VolumeId  volumeidNative  = ::volumeidInvalid;
                    ::FileId    fileidNative    = ::fileidInvalid;

                    volumeid = VolumeId::Invalid;
                    fileid = FileId::Invalid;

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrGetFileId( wszPath, &volumeidNative, &fileidNative ) );

                    volumeid = (VolumeId)volumeidNative;
                    fileid = (FileId)(Int64)fileidNative;

                    return;

                HandleError:
                    volumeid = VolumeId::Invalid;
                    fileid = FileId::Invalid;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline String^ FileIdentificationBase<TM, TN, TW>::GetFileKeyPath( String^ path )
                {
                    ERR     err                                 = JET_errSuccess;
                    WCHAR   wszKeyPath[ OSFSAPI_MAX_PATH ]      = { 0 };

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrGetFileKeyPath( wszPath, wszKeyPath ) );

                    return gcnew String( wszKeyPath );

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileIdentificationBase<TM, TN, TW>::GetFilePathById(
                    VolumeId volumeid,
                    FileId fileid,
                    [Out] String^% anyAbsPath,
                    [Out] String^% keyPath )
                {
                    ERR     err                                 = JET_errSuccess;
                    WCHAR   wszAnyAbsPath[ OSFSAPI_MAX_PATH ]   = { 0 };
                    WCHAR   wszKeyPath[ OSFSAPI_MAX_PATH ]      = { 0 };

                    anyAbsPath = nullptr;
                    keyPath = nullptr;

                    Call( Pi->ErrGetFilePathById( (::VolumeId)volumeid, (::FileId)fileid, wszAnyAbsPath, wszKeyPath ) );

                    anyAbsPath = gcnew String( wszAnyAbsPath );
                    keyPath = gcnew String( wszKeyPath );

                    return;

                HandleError:
                    anyAbsPath = nullptr;
                    keyPath = nullptr;
                    throw EseException( err );
                }
            }
        }
    }
}
