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
                ref class FileSystemRemotable : Remotable, IFileSystem
                {
                    public:

                        FileSystemRemotable( IFileSystem^ target )
                            :   target( target )
                        {
                        }

                        ~FileSystemRemotable() {}

                    public:

                        virtual void DiskSpace(
                            String^ path,
                            [Out] Int64% bytesFreeForUser,
                            [Out] Int64% bytesTotalForUser,
                            [Out] Int64% bytesFreeOnDisk )
                        {
                            this->target->DiskSpace( path, bytesFreeForUser, bytesTotalForUser, bytesFreeOnDisk );
                        }

                        virtual int FileSectorSize( String^ path )
                        {
                            return this->target->FileSectorSize( path );
                        }

                        virtual int FileAtomicWriteSize( String^ path )
                        {
                            return this->target->FileAtomicWriteSize( path );
                        }

                        virtual EsentErrorException^ GetLastError( int error )
                        {
                            return this->target->GetLastError( error );
                        }

                        virtual String^ PathRoot( String^ path )
                        {
                            return this->target->PathRoot( path );
                        }

                        virtual void PathVolumeCanonicalAndDiskId(
                            String^ volumePath,
                            [Out] String^% volumeCanonicalPath,
                            [Out] String^% diskId,
                            [Out] int% diskNumber )
                        {
                            return this->target->PathVolumeCanonicalAndDiskId( volumePath, volumeCanonicalPath, diskId, diskNumber );
                        }

                        virtual  String^ PathComplete( String^ path )
                        {
                            return this->target->PathComplete( path );
                        }

                        virtual void PathParse( String^ path, [Out] String^% folder, [Out] String^% fileBase, [Out] String^% fileExt )
                        {
                            return this->target->PathParse( path, folder, fileBase, fileExt );
                        }

                        virtual String^ PathFileName( String^ path )
                        {
                            return this->target->PathFileName( path );
                        }

                        virtual String^ PathBuild( String^ folder, String^ fileBase, String^ fileExt )
                        {
                            return this->target->PathBuild( folder, fileBase, fileExt );
                        }

                        virtual String^ PathFolderNorm( String^ folder )
                        {
                            return this->target->PathFolderNorm( folder );
                        }

                        virtual bool PathIsRelative( String^ path )
                        {
                            return this->target->PathIsRelative( path );
                        }

                        virtual bool PathExists( String^ path )
                        {
                            return this->target->PathExists( path );
                        }

                        virtual String^ PathFolderDefault( [Out] bool% canProcessUseRelativePaths )
                        {
                            return this->target->PathFolderDefault( canProcessUseRelativePaths );
                        }

                        virtual String^ GetTempFolder()
                        {
                            return this->target->GetTempFolder();
                        }

                        virtual String^ GetTempFileName( String^ folder, String^ prefix )
                        {
                            return this->target->GetTempFileName( folder, prefix );
                        }

                        virtual void FolderCreate( String^ path )
                        {
                            return this->target->FolderCreate( path );
                        }

                        virtual void FolderRemove( String^ path )
                        {
                            return this->target->FolderRemove( path );
                        }

                        virtual IFileFind^ FileFind( String^ find )
                        {
                            return this->target->FileFind( find );
                        }

                        virtual void FileDelete( String^ path )
                        {
                            return this->target->FileDelete( path );
                        }

                        virtual void FileMove( String^ pathSource, String^ pathDest, bool overwriteExisting )
                        {
                            return this->target->FileMove( pathSource, pathDest, overwriteExisting );
                        }

                        virtual void FileCopy( String^ pathSource, String^ pathDest, bool overwriteExisting )
                        {
                            return this->target->FileCopy( pathSource, pathDest, overwriteExisting );
                        }

                        virtual IFile^ FileCreate( String^ path, FileModeFlags fileModeFlags )
                        {
                            return this->target->FileCreate( path, fileModeFlags );
                        }

                        virtual IFile^ FileOpen( String^ path, FileModeFlags fileModeFlags )
                        {
                            return this->target->FileOpen( path, fileModeFlags );
                        }

                    private:

                        IFileSystem^ target;
                };
            }
        }
    }
}