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
                /// File System Interface.
                /// </summary>
                public interface class IFileSystem : IDisposable
                {
                    /// <summary>
                    /// Returns the amount of free/total disk space on the drive hosting the specified path.
                    /// </summary>
                    /// <param name="path">The path.</param>
                    /// <param name="bytesFreeForUser">The available storage for the current user context in bytes.</param>
                    /// <param name="bytesTotalForUser">The total storage for the current user context in bytes.</param>
                    /// <param name="bytesFreeOnDisk">The available storage on the device in bytes.</param>
                    void DiskSpace(
                        String^ path,
                        [Out] Int64% bytesFreeForUser,
                        [Out] Int64% bytesTotalForUser,
                        [Out] Int64% bytesFreeOnDisk );

                    /// <summary>
                    /// Returns the sector size for the specified path.
                    /// </summary>
                    /// <param name="path">The path.</param>
                    /// <returns>The sector size in bytes for the device holding that path.</returns>
                    int FileSectorSize( String^ path );

                    /// <summary>
                    /// Returns the atomic write size for the specified path
                    /// </summary>
                    /// <param name="path">The path.</param>
                    /// <returns>The atomic write size in bytes for the device holding that path.</returns>
                    int FileAtomicWriteSize( String^ path );

                    /// <summary>
                    /// Converts a Win32 error to a JET error.
                    /// </summary>
                    /// <param name="error">The Win32 error.</param>
                    /// <returns>An EsentErrorException or null if the operation was successful.</returns>
                    EsentErrorException^ GetLastError( int error );

                    /// <summary>
                    /// Computes the root path of the specified path.
                    /// </summary>
                    /// <param name="path">The path.</param>
                    /// <returns>The volume root path of the specified path.</returns>
                    String^ PathRoot( String^ path );

                    /// <summary>
                    /// Computes the volume canonical path and disk ID of the specified root path.
                    /// </summary>
                    /// <param name="volumePath">The volume path.</param>
                    /// <param name="volumeCanonicalPath">The canonical volume path.</param>
                    /// <param name="diskId">The disk id.</param>
                    /// <param name="diskNumber">The disk number.</param>
                    void PathVolumeCanonicalAndDiskId(
                        String^ volumePath,
                        [Out] String^% volumeCanonicalPath,
                        [Out] String^% diskId,
                        [Out] int% diskNumber);

                    /// <summary>
                    /// Computes the absolute path of the specified path.
                    /// </summary>
                    /// <param name="path">The path.</param>
                    /// <returns>The full path of the specified path.</returns>
                    String^ PathComplete( String^ path );

                    /// <summary>
                    /// Breaks the given path into folder, filename, and extension components.
                    /// </summary>
                    /// <param name="path">The path.</param>
                    /// <param name="folder">The folder component of the specified path.</param>
                    /// <param name="fileBase">The filename component of the specified path.</param>
                    /// <param name="fileExt">The file extension component of the specified path.</param>
                    void PathParse(String^ path, [Out] String^% folder, [Out] String^% fileBase, [Out] String^% fileExt);

                    /// <summary>
                    /// Returns the portion of the passed in String representing just the filename.ext piece.
                    /// </summary>
                    /// <param name="path">The path.</param>
                    /// <returns>The filename and extension portion of the path.</returns>
                    String^ PathFileName(String^ path);

                    /// <summary>
                    /// Composes a path from folder, filename, and extension components.
                    /// </summary>
                    /// <param name="folder">The folder component of the path.</param>
                    /// <param name="fileBase">The filename component of the path.</param>
                    /// <param name="fileExt">The file extension component of the path.</param>
                    /// <returns>The path comprised of the specified path components.</returns>
                    String^ PathBuild( String^ folder, String^ fileBase, String^ fileExt );

                    /// <summary>
                    /// Appends a folder delimiter (in NT L"\") if it doesn't already have a
                    /// folder delimiter at the end of the String.
                    /// </summary>
                    /// <param name="folder">The folder path.</param>
                    /// <returns>The normalized folder path.</returns>
                    String^ PathFolderNorm( String^ folder );

                    /// <summary>
                    /// Returns whether the input path is absolute or relative.
                    /// </summary>
                    /// <remarks>
                    /// Note that a leading backslash (e.g. \windows\win.ini) is still relative.
                    /// </remarks>
                    /// <param name="path">The path.</param>
                    /// <returns>True if the path is relative.</returns>
                    bool PathIsRelative( String^ path );

                    /// <summary>
                    /// Returns whether the specified file/path exists.
                    /// </summary>
                    /// <param name="path">The path.</param>
                    /// <returns>True if the path points to a directory.</returns>
                    bool PathExists( String^ path );

                    /// <summary>
                    /// Retrieves the default location that files should be stored.
                    /// </summary>
                    /// <remarks>
                    /// The "." directory (current working dir) is not writeable by Windows Store apps.
                    /// </remarks>
                    /// <param name="canProcessUseRelativePaths">True if the process may use relative paths.</param>
                    /// <returns>The path to the default folder.</returns>
                    String^ PathFolderDefault( [Out] bool% canProcessUseRelativePaths );

                    /// <summary>
                    /// Creates the specified folder.
                    /// </summary>
                    /// <remarks>
                    /// If the folder already exists, EsentFileAccessDeniedException will be thrown.
                    /// </remarks>
                    /// <param name="path">The path of the folder to create.</param>
                    void FolderCreate( String^ path );

                    /// <summary>
                    /// Removes the specified folder.
                    /// </summary>
                    /// <remarks>
                    /// If the folder is not empty, EsentFileAccessDeniedException will be thrown.  if the folder does
                    /// not exist, EsentInvalidPathException will be thrown.
                    /// </remarks>
                    /// <param name="path">The path of the folder to remove.</param>
                    void FolderRemove( String^ path );

                    /// <summary>
                    /// Creates a File Find iterator that will traverse the absolute paths
                    /// of all files and folders that match the specified path with wildcards.
                    /// </summary>
                    /// <param name="find">The criteria that will be used to find files.</param>
                    /// <returns>The file find iterator.</returns>
                    IFileFind^ FileFind( String^ find );

                    /// <summary>
                    /// Deletes the file specified.
                    /// </summary>
                    /// <remarks>
                    /// If the file cannot be deleted, EsentFileAccessDeniedException will be thrown.
                    /// </remarks>
                    /// <param name="path">The path of the file to be deleted.</param>
                    void FileDelete( String^ path );

                    /// <summary>
                    /// Moves the file specified from a source path to a destination path, overwriting
                    /// any file at the destination path if requested.
                    /// </summary>
                    /// <remarks>
                    /// If the file cannot be copied, EsentFileAccessDeniedException will be thrown.
                    /// </remarks>
                    /// <param name="pathSource">The path of the file to be moved.</param>
                    /// <param name="pathDest">The path where the file is to be moved.</param>
                    /// <param name="overwriteExisting">True if any existing file at the destination may be deleted.</param>
                    void FileMove( String^ pathSource, String^ pathDest, bool overwriteExisting );

                    /// <summary>
                    /// Copies the file specified from a source path to a destination path, overwriting
                    /// any file at the destination path if requested.
                    /// </summary>
                    /// <remarks>
                    /// If the file cannot be copied, EsentFileAccessDeniedException will be thrown.
                    /// </remarks>
                    /// <param name="pathSource">The path of the file to be copied.</param>
                    /// <param name="pathDest">The path where the file is to be copied.</param>
                    /// <param name="overwriteExisting">True if any existing file at the destination may be deleted.</param>
                    void FileCopy( String^ pathSource, String^ pathDest, bool overwriteExisting );

                    /// <summary>
                    /// Creates the file specified and returns its handle.
                    /// </summary>
                    /// <remarks>
                    /// If the file cannot be created, EsentFileAccessDeniedException or EsentDiskFullException will be
                    /// thrown.
                    /// </remarks>
                    /// <param name="path">The path of the file to create.</param>
                    /// <param name="fileModeFlags">The options for the file create operation.</param>
                    /// <returns>The file that was created.</returns>
                    IFile^ FileCreate( String^ path, FileModeFlags fileModeFlags );

                    /// <summary>
                    /// Opens the file specified with the specified access privileges and returns its handle.
                    /// </summary>
                    /// <param name="path">The path of the file to open.</param>
                    /// <param name="fileModeFlags">The options for the file open operation.</param>
                    /// <param name="file">The file that was opened</param>
                    /// <returns>The file that was opened.</returns>
                    IFile^ FileOpen( String^ path, FileModeFlags fileModeFlags );
                };
            }
        }
    }
}