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
                ref class FileSystemFilterRemotable : FileSystemRemotable, IFileSystemFilter
                {
                    public:

                        FileSystemFilterRemotable( IFileSystemFilter^ target )
                            :   FileSystemRemotable( target ),
                                target( target )
                        {
                        }

                        ~FileSystemFilterRemotable() {}

                    public:

                        virtual IFile^ FileOpenById(
                            VolumeId volumeid,
                            FileId fileid,
                            FileSerial fileserial,
                            FileModeFlags fileModeFlags )
                        {
                            return this->target->FileOpenById( volumeid, fileid, fileserial, fileModeFlags );
                        }

                        virtual void FileRename( File^ f, String^ pathDest, bool overwriteExisting )
                        {
                            return this->target->FileRename( f, pathDest, overwriteExisting );
                        }

                    private:

                        IFileSystemFilter^ target;
                };
            }
        }
    }
}