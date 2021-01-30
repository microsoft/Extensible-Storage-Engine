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
                class CFileSystemFilterWrapper : public CFileSystemWrapper<TM, TN>
                {
                    public:

                        CFileSystemFilterWrapper( TM^ fsf ) : CFileSystemWrapper( fsf ) { }

                    public:

                        ERR ErrFileOpenById(    _In_    const ::VolumeId                volumeid,
                                                _In_    const ::FileId                  fileid,
                                                _In_    const ::FileSerial              fileserial,
                                                _In_    const IFileAPI::FileModeFlags   fmf,
                                                _Out_   IFileAPI** const                ppfapi ) override;
                        ERR ErrFileRename(  _In_    IFileAPI* const    pfapi,
                                            _In_z_  const WCHAR* const wszPathDest,
                                            _In_    const BOOL         fOverwriteExisting ) override;

                    protected:

                        ERR ErrWrap( _In_ IFile^% f, _Out_ IFileAPI** const ppfapi ) override;
                };

                template< class TM, class TN >
                inline ERR CFileSystemFilterWrapper<TM, TN>::ErrFileOpenById(   _In_    const ::VolumeId                volumeid,
                                                                                _In_    const ::FileId                  fileid,
                                                                                _In_    const ::FileSerial              fileserial,
                                                                                _In_    const IFileAPI::FileModeFlags   fmf,
                                                                                _Out_   IFileAPI** const                ppfapi )
                {
                    ERR     err     = JET_errSuccess;
                    IFile^  f       = nullptr;

                    *ppfapi = NULL;

                    ExCall( f = I()->FileOpenById(  (VolumeId)volumeid,
                                                    (FileId)(Int64)fileid,
                                                    (FileSerial)fileserial,
                                                    (FileModeFlags)fmf ) );

                    Call( ErrWrap( f, ppfapi ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        delete f;
                        delete *ppfapi;
                        *ppfapi = NULL;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemFilterWrapper<TM, TN>::ErrFileRename( _In_    IFileAPI* const     pfapi,
                                                                            _In_z_  const WCHAR* const  wszPathDest,
                                                                            _In_    const BOOL          fOverwriteExisting )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->FileRename(    gcnew File( pfapi ),
                                                gcnew String( wszPathDest ),
                                                fOverwriteExisting ? true : false ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemFilterWrapper<TM, TN>::ErrWrap( _In_ IFile^% f, _Out_ IFileAPI** const ppfapi )
                {
                    ERR             err     = JET_errSuccess;
                    IFileFilter^    ff      = nullptr;
                    ::IFileFilter*  pffapi  = NULL;

                    *ppfapi = NULL;

                    ff = (IFileFilter^)f;

                    Call( FileFilter::ErrWrap( ff, &pffapi ) );

                    f = nullptr;
                    *ppfapi = pffapi;
                    pffapi = NULL;

                HandleError:
                    delete pffapi;
                    if ( err < JET_errSuccess )
                    {
                        delete *ppfapi;
                        *ppfapi = NULL;
                    }
                    return err;
                }
            }
        }
    }
}
