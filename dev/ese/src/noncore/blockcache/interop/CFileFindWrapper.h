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
                class CFileFindWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CFileFindWrapper( TM^ ff ) : CWrapper( ff ) { }

                    public:

                        ERR ErrNext() override;

                        ERR ErrIsFolder( BOOL* const pfFolder ) override;
                        ERR ErrPath( _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const wszAbsPath ) override;
                        ERR ErrSize(    _Out_ QWORD* const pcbSize,
                                        _In_ const IFileAPI::FILESIZE filesize ) override;
                        ERR ErrIsReadOnly( BOOL* const pfReadOnly ) override;
                };

                template< class TM, class TN >
                inline ERR CFileFindWrapper<TM,TN>::ErrNext()
                {
                    ERR     err         = JET_errSuccess;
                    BOOL    fSuccess    = fFalse;

                    ExCall( fSuccess = I()->Next() ? fTrue : fFalse );

                    Call( fSuccess ? JET_errSuccess : ErrERRCheck( JET_errFileNotFound ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileFindWrapper<TM,TN>::ErrIsFolder( BOOL* const pfFolder )
                {
                    ERR     err         = JET_errSuccess;
                    bool    isFolder    = false;

                    *pfFolder = fFalse;

                    ExCall( isFolder = I()->IsFolder() );

                    *pfFolder = isFolder ? fTrue : fFalse;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pfFolder = fFalse;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileFindWrapper<TM,TN>::ErrPath( _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsFoundPath )
                {
                    ERR     err         = JET_errSuccess;
                    String^ foundPath   = nullptr;

                    wszAbsFoundPath[ 0 ] = L'\0';

                    ExCall( foundPath = I()->Path() );

                    pin_ptr<const Char> wszFoundPath = PtrToStringChars( foundPath );
                    Call( ErrOSStrCbCopyW( wszAbsFoundPath, cbOSFSAPI_MAX_PATHW, (STRSAFE_LPCWSTR)wszFoundPath ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        wszAbsFoundPath[ 0 ] = L'\0';
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileFindWrapper<TM,TN>::ErrSize(    _Out_ QWORD* const pcbSize,
                                                                _In_ const IFileAPI::FILESIZE filesize )
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
                inline ERR CFileFindWrapper<TM,TN>::ErrIsReadOnly( BOOL* const pfReadOnly )
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
            }
        }
    }
}
