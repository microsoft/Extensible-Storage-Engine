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
                public interface class IFileSystem : IDisposable
                {
                    void DiskSpace(
                        String^ path,
                        [Out] Int64% bytesFreeForUser,
                        [Out] Int64% bytesTotalForUser,
                        [Out] Int64% bytesFreeOnDisk );

                    int FileSectorSize( String^ path );

                    int FileAtomicWriteSize( String^ path );

                    EsentErrorException^ GetLastError( int error );

                    String^ PathRoot( String^ path );

                    void PathVolumeCanonicalAndDiskId(
                        String^ volumePath,
                        [Out] String^% volumeCanonicalPath,
                        [Out] String^% diskId,
                        [Out] int% diskNumber);

                    String^ PathComplete( String^ path );

                    void PathParse(String^ path, [Out] String^% folder, [Out] String^% fileBase, [Out] String^% fileExt);

                    String^ PathFileName(String^ path);

                    String^ PathBuild( String^ folder, String^ fileBase, String^ fileExt );

                    String^ PathFolderNorm( String^ folder );

                    bool PathIsRelative( String^ path );

                    bool PathExists( String^ path );

                    String^ PathFolderDefault( [Out] bool% canProcessUseRelativePaths );

                    void FolderCreate( String^ path );

                    void FolderRemove( String^ path );

                    IFileFind^ FileFind( String^ find );

                    void FileDelete( String^ path );

                    void FileMove( String^ pathSource, String^ pathDest, bool overwriteExisting );

                    void FileCopy( String^ pathSource, String^ pathDest, bool overwriteExisting );

                    IFile^ FileCreate( String^ path, FileModeFlags fileModeFlags );

                    IFile^ FileOpen( String^ path, FileModeFlags fileModeFlags );
                };
            }
        }
    }
}
