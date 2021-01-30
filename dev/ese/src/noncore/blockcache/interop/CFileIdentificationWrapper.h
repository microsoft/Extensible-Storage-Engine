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
                class CFileIdentificationWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CFileIdentificationWrapper( TM^ fident ) : CWrapper( fident ) { }

                    public:

                        ERR ErrGetFileId(   _In_z_  const WCHAR* const  wszPath,
                                            _Out_   ::VolumeId* const   pvolumeid,
                                            _Out_   ::FileId* const pfileid ) override;

                        ERR ErrGetFileKeyPath(  _In_z_                                  const WCHAR* const  wszPath,
                                                _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszKeyPath ) override;

                        ERR ErrGetFilePathById( _In_                                    const ::VolumeId    volumeid,
                                                _In_                                    const ::FileId      fileid,
                                                _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszAnyAbsPath,
                                                _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszKeyPath ) override;
                };

                template< class TM, class TN >
                inline ERR CFileIdentificationWrapper<TM, TN>::ErrGetFileId(    _In_z_  const WCHAR* const  wszPath,
                                                                                _Out_   ::VolumeId* const   pvolumeid,
                                                                                _Out_   ::FileId* const     pfileid )
                {
                    ERR         err         = JET_errSuccess;
                    VolumeId    volumeid    = VolumeId::Invalid;
                    FileId      fileid      = FileId::Invalid;

                    *pvolumeid = ::volumeidInvalid;
                    *pfileid = ::fileidInvalid;

                    ExCall( I()->GetFileId( gcnew String( wszPath ), volumeid, fileid ) );

                    *pvolumeid = (::VolumeId)volumeid;
                    *pfileid = (::FileId)fileid;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pvolumeid = ::volumeidInvalid;
                        *pfileid = ::fileidInvalid;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileIdentificationWrapper<TM, TN>::ErrGetFileKeyPath(   _In_z_                                  const WCHAR* const  wszPath,
                                                                                    _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszKeyPath )
                {
                    ERR     err         = JET_errSuccess;
                    String^ keyPath     = nullptr;

                    wszKeyPath[ 0 ] = L'\0';

                    ExCall( keyPath = I()->GetFileKeyPath( gcnew String( wszPath ) ) );

                    if ( keyPath != nullptr )
                    {
                        pin_ptr<const Char> wszKeyPathT = PtrToStringChars( keyPath );
                        Call( ErrOSStrCbCopyW( wszKeyPath, cbOSFSAPI_MAX_PATHW, (STRSAFE_LPCWSTR)wszKeyPathT ) );
                    }

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        wszKeyPath[ 0 ] = L'\0';
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileIdentificationWrapper<TM, TN>::ErrGetFilePathById(  _In_                                    const ::VolumeId    volumeid,
                                                                                    _In_                                    const ::FileId      fileid,
                                                                                    _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszAnyAbsPath,
                                                                                    _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszKeyPath )
                {
                    ERR     err         = JET_errSuccess;
                    String^ anyAbsPath  = nullptr;
                    String^ keyPath     = nullptr;

                    wszAnyAbsPath[ 0 ] = L'\0';
                    wszKeyPath[ 0 ] = L'\0';

                    ExCall( I()->GetFilePathById( (VolumeId)volumeid, (FileId)(Int64)fileid, anyAbsPath, keyPath ) );

                    pin_ptr<const Char> wszAnyAbsPathT = PtrToStringChars( anyAbsPath );
                    Call( ErrOSStrCbCopyW( wszAnyAbsPath, cbOSFSAPI_MAX_PATHW, (STRSAFE_LPCWSTR)wszAnyAbsPathT ) );
                    pin_ptr<const Char> wszKeyPathT = PtrToStringChars( keyPath );
                    Call( ErrOSStrCbCopyW( wszKeyPath, cbOSFSAPI_MAX_PATHW, (STRSAFE_LPCWSTR)wszKeyPathT ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        wszAnyAbsPath[ 0 ] = L'\0';
                        wszKeyPath[ 0 ] = L'\0';
                    }
                    return err;
                }
            }
        }
    }
}
