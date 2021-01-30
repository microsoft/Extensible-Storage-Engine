// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

#undef interface

#pragma managed

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                public interface class IFileSystemFilter : IFileSystem
                {
                    IFile^ FileOpenById(
                        VolumeId volumeid,
                        FileId fileid,
                        FileSerial fileserial,
                        FileModeFlags fileModeFlags );


                    void FileRename( IFile^ f, String^ pathDest, bool overwriteExisting );
                };
            }
        }
    }
}
