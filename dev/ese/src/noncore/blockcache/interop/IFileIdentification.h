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
                /// File Identification Interface.
                /// </summary>
                public interface class IFileIdentification : IDisposable
                {
                    /// <summary>
                    /// Returns the unique volume id and file id of a file by path.
                    /// </summary>
                    /// <param name="path">A path of the file.</param>
                    /// <param name="volumeid">The volume id of the file.</param>
                    /// <param name="fileid">The file id of the file.</param>
                    void GetFileId( String^ path, [Out] VolumeId% volumeid, [Out] FileId% fileid );

                    /// <summary>
                    /// Returns the unique path of a file by path.
                    /// </summary>
                    /// <remarks>
                    /// The unique path returned for the file is intended for use in a hash table not for file system access.
                    /// </remarks>
                    /// <param name="path">A path of the file.</param>
                    /// <returns>A unique path of the file.</returns>
                    String^ GetFileKeyPath( String^ path );

                    /// <summary>
                    /// Returns any absolute path and the unique path of a file by file id.
                    /// </summary>
                    /// <remarks>
                    /// A given file may have many absolute paths due to multiple mount points, hard links, etc.
                    ///
                    /// The unique path returned for the file is intended for use in a hash table not for file system access.
                    /// </remarks>
                    /// <param name="volumeid">The volume id of the file.</param>
                    /// <param name="fileid">The file id of the file.</param>
                    /// <param name="absPath">An absolute path of the file.</param>
                    /// <param name="keyPath">A unique path of the file.</param>
                    void GetFilePathById(
                        VolumeId volumeid,
                        FileId fileid,
                        [Out] String^% anyAbsPath,
                        [Out] String^% keyPath );
                };
            }
        }
    }
}