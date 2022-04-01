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
                public ref class FileSystemFilterBase : FileSystemBase<TM,TN,TW>, IFileSystemFilter
                {
                    public:

                        FileSystemFilterBase( TM^ fsf ) : FileSystemBase( fsf ) { }

                        FileSystemFilterBase( TN** const ppfsf ) : FileSystemBase( ppfsf ) {}

                        FileSystemFilterBase( TN* const pfsf ) : FileSystemBase( pfsf ) {}

                    public:

                        virtual IFile^ FileOpenById(
                            VolumeId volumeid,
                            FileId fileid,
                            FileSerial fileserial,
                            FileModeFlags fileModeFlags );

                        virtual void FileRename( File^ f, String^ pathDest, bool overwriteExisting );

                    protected:

                        IFile^ Wrap( IFileAPI** ppfapi ) override;
                };

                template< class TM, class TN, class TW >
                inline IFile^ FileSystemFilterBase<TM, TN, TW>::FileOpenById(
                    VolumeId volumeid,
                    FileId fileid, 
                    FileSerial fileserial,
                    FileModeFlags fileModeFlags )
                {
                    ERR             err = JET_errSuccess;
                    ::IFileFilter*  pff = NULL;
                    FileFilter^     ff  = nullptr;

                    Call( Pi->ErrFileOpenById(  (::VolumeId)volumeid,
                                                (::FileId)fileid,
                                                (::FileSerial)fileserial,
                                                (IFileAPI::FileModeFlags)fileModeFlags,
                                                (IFileAPI**)&pff ) );

                    ff = (FileFilter^)Wrap( (IFileAPI**)&pff );
                    pff = NULL;

                    return ff;

                HandleError:
                    delete pff;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileSystemFilterBase<TM,TN,TW>::FileRename( File^ f, String^ pathDest, bool overwriteExisting )
                {
                    ERR err = JET_errSuccess;

                    pin_ptr<const Char> wszPathDest = PtrToStringChars( pathDest );
                    Call( Pi->ErrFileRename(    f->Pi,
                                                (const WCHAR*)wszPathDest,
                                                overwriteExisting ? fTrue : fFalse ) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline IFile^ FileSystemFilterBase<TM, TN, TW>::Wrap( IFileAPI** ppfapi )
                {
                    return *ppfapi ? gcnew FileFilter( (::IFileFilter**)ppfapi ) : nullptr;
                }
            }
        }
    }
}
