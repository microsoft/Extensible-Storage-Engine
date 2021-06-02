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
                /// <summary>
                /// File System Filter interface.
                /// </summary>
                public interface class IFileSystemFilter : IFileSystem
                {
                    /// <summary>
                    /// Opens the file specified with the specified access privileges and returns its handle.
                    /// </summary>
                    /// <param name="volumeid">The volume id of the file to open.</param>
                    /// <param name="fileid">The file id of the file to open.</param>
                    /// <param name="fileserial">The serial number of the cached file.</param>
                    /// <param name="fileModeFlags">The options for the file open operation.</param>
                    /// <returns>The file that was opened.</returns>
                    IFile^ FileOpenById(
                        VolumeId volumeid,
                        FileId fileid,
                        FileSerial fileserial,
                        FileModeFlags fileModeFlags );


                    /// <summary>
                    /// Renames an open file.
                    /// </summary>
                    /// <remarks>
                    /// The file must stay on the same volume.
                    /// </remarks>
                    /// <param name="f">The file to rename.</param>
                    /// <param name="pathDest">The new path for the file.</param>
                    /// <param name="overwriteExisting">True if any existing file at the destination may be deleted.</param>
                    void FileRename( IFile^ f, String^ pathDest, bool overwriteExisting );
                };
            }
        }
    }
}
