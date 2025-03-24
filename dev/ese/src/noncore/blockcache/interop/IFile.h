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
                /// File Interface.
                /// </summary>
                public interface class IFile : IDisposable
                {
                    /// <summary>
                    /// Delegate that will be called when an asynchronously issued I/O has completed.
                    /// </summary>
                    /// <remarks>
                    /// The function will be given the error code and parameters for the completed I/O.
                    /// </remarks>
                    /// <param name="ex">An EsentErrorException or null if the IO was successful.</param>
                    /// <param name="file">The file that experienced the IO.</param>
                    /// <param name="fileQOS">File Quality Of Service for the IO.</param>
                    /// <param name="offsetInBytes">The offset in the file of the IO.</param>
                    /// <param name="data">The data payload for the IO.</param>
                    delegate void IOComplete(
                        EsentErrorException^ ex,
                        IFile^ file,
                        FileQOS fileQOS,
                        Int64 offsetInBytes,
                        ArraySegment<byte> data );

                    /// <summary>
                    /// Delegate that will be called when an asynchronously issued I/O has been successfully handed off to the IO
                    /// Queue(s) in the lower IO Manager.
                    /// </summary>
                    /// <remarks>
                    /// When this is called, the API is guaranteed to call FileIOComplete.  If the IORead / IOWrite methods are going
                    /// to return a quota exceeded / errDiskTilt error this function will not be called.
                    /// </remarks>
                    /// <param name="ex">An EsentErrorException or null if the IO was successful.</param>
                    /// <param name="file">The file that experienced the IO.</param>
                    /// <param name="fileQOS">File Quality Of Service for the IO.</param>
                    /// <param name="offsetInBytes">The offset in the file of the IO.</param>
                    /// <param name="data">The data payload for the IO.</param>
                    delegate void IOHandoff(
                        EsentErrorException^ ex,
                        IFile^ file,
                        FileQOS fileQOS,
                        Int64 offsetInBytes,
                        ArraySegment<byte> data );

                    /// <summary>
                    /// Returns the mode(s) or disposition(s) of the file, such as Read-Only, or Temporary file. See FileModeFlags.
                    /// </summary>
                    /// <returns>The FileModeFlags used to create or open the file.</returns>
                    FileModeFlags FileModeFlags();

                    /// <summary>
                    /// Causes any previous writes to the file to be written to the media.
                    /// </summary>
                    void FlushFileBuffers();

                    /// <summary>
                    /// Indicates that the caller doesn't care if any previous writes to the file make it to the media.
                    /// </summary>
                    void SetNoFlushNeeded();

                    /// <summary>
                    /// Returns the absolute path of the current file.
                    /// </summary>
                    /// <returns>The absolute path of the current file.</returns>
                    String^ Path();

                    /// <summary>
                    /// Returns the current size of the current file.
                    /// </summary>
                    /// <param name="fileSize">The type of file size.</param>
                    /// <returns>The size of the file in bytes.</returns>
                    Int64 Size( FileSize fileSize );

                    /// <summary>
                    /// Returns the read only attribute of the current file.
                    /// </summary>
                    /// <returns>True if the file is read only.</returns>
                    bool IsReadOnly();

                    /// <summary>
                    /// If file is backed by storage spaces, how many logical copies of data are present?
                    /// </summary>
                    /// <returns>The number of locail copies of the data.</returns>
                    int CountLogicalCopies();

                    /// <summary>
                    /// Sets the size of the current file.
                    /// </summary>
                    /// <remarks>
                    /// If there is insufficient disk space to satisfy the request, EsentDiskFullException will be thrown.
                    /// </remarks>
                    /// <param name="sizeInBytes">The size of the file in bytes.</param>
                    /// <param name="zeroFill">True if any new offset range in the file should be explicitly zero filled.</param>
                    /// <param name="fileQOS">File Quality Of Service.</param>
                    void SetSize(Int64 sizeInBytes, bool zeroFill, FileQOS fileQOS);

                    /// <summary>
                    /// Renames an open file.
                    /// </summary>
                    /// <remarks>
                    /// The file must stay on the same volume.
                    /// </remarks>
                    /// <param name="pathDest">The new path for the file.</param>
                    /// <param name="overwriteExisting">True if any existing file at the destination may be deleted.</param>
                    void Rename(String^ pathDest, bool overwriteExisting);

                    /// <summary>
                    /// Attempts to make the file sparse.
                    /// </summary>
                    /// <remarks>
                    /// This method must be successful so that IOTrim may be used.
                    /// 
                    /// If EsentUnloadableOSFunctionalityException is thrown then the file may not be sparse.
                    /// </remarks>
                    void SetSparseness();

                    /// <summary>
                    /// Deletes the storage behind a range of a sparse file.
                    /// </summary>
                    /// <remarks>
                    /// SetSparseness() must succeed for this function to work.
                    /// 
                    /// The bytes behind the deleted offsets can still be read but will be zero.
                    /// 
                    /// If EsentUnloadableOSFunctionalityException is thrown then the file may not be sparse.
                    /// </remarks>
                    /// <param name="offsetInBytes">The file offset in bytes.</param>
                    /// <param name="sizeInBytes">The size in bytes to delete.</param>
                    void IOTrim( Int64 offsetInBytes, Int64 sizeInBytes );

                    /// <summary>
                    /// Finds the allocated region of a sparse file that contains a given offset.
                    /// </summary>
                    /// <remarks>
                    /// If the offset is not in an allocated range then an offset and size of zero are returned.
                    /// 
                    /// If EsentUnloadableOSFunctionalityException is thrown then the file may not be sparse.
                    /// </remarks>
                    /// <param name="offsetToQuery">File offset to query.</param>
                    /// <param name="offsetInBytes">Offset in bytes of the allocated region of the file containing the query offset.</param>
                    /// <param name="sizeInBytes">Size in bytes of the allocated region of the file containing the query offset.</param>
                    void RetrieveAllocatedRegion(
                        Int64 offsetToQuery, 
                        [Out] Int64% offsetInBytes, 
                        [Out] Int64% sizeInBytes);

                    /// <summary>
                    /// Returns the size of the smallest block of the current file that is
                    /// addressable by the I/O functions for the current file.
                    /// </summary>
                    /// <returns>The smallest IO size in bytes.</returns>
                    int IOSize();

                    /// <summary>
                    /// Returns the size sector size associated with the current file.
                    /// </summary>
                    /// <returns>The sector size in bytes.</returns>
                    int SectorSize();

                    /// <summary>
                    /// Reads a block of data of the specified size from the current file at the specified offset and
                    /// places it into the specified buffer.
                    /// </summary>
                    /// <remarks>
                    /// The offset, size, and buffer pointer must be aligned to the block size for the file.
                    ///
                    /// If no completion function is given, the function will not return
                    /// until the I/O has been completed.  If an error occurs, that error
                    /// will be returned.
                    ///
                    /// If a completion function is given, the function will return
                    /// immediately and the I/O will be processed asynchronously.  The
                    /// completion function will be called when the I/O has completed.  If
                    /// an error occurs, that error will be returned via the completion
                    /// function.
                    ///
                    /// There is no guarantee that an asynchronous I/O will be issued unless
                    /// IOIssue() is called or the file handle is closed.
                    ///
                    /// Possible I/O errors:
                    ///
                    ///   EsentDiskIOException            general I/O failure
                    ///   EsentFileIOBeyondEOFException   a read was issued to a location beyond
                    ///                                   the end of the file
                    /// </remarks>
                    /// <param name="offsetInBytes">The offset in the file of the IO.</param>
                    /// <param name="data">The data payload for the IO.</param>
                    /// <param name="fileQOS">File Quality Of Service for the IO.</param>
                    /// <param name="ioComplete">An optional IO completion delegate.</param>
                    /// <param name="ioHandoff">An optional IO handoff delegate.</param>
                    void IORead(
                        Int64 offsetInBytes,
                        ArraySegment<byte> data,
                        FileQOS fileQOS,
                        IOComplete^ ioComplete,
                        IOHandoff^ ioHandoff );

                    /// <summary>
                    /// writes a block of data of the specified size from the specified buffer at the specified offset
                    /// and places it into the current file.
                    /// </summary>
                    /// <remarks>
                    /// The offset, size, and buffer pointer must be aligned to the block size for the file.
                    ///
                    /// If no completion function is given, the function will not return
                    /// until the I/O has been completed.  If an error occurs, that error
                    /// will be returned.
                    ///
                    /// If a completion function is given, the function will return
                    /// immediately and the I/O will be processed asynchronously.  The
                    /// completion function will be called when the I/O has completed.  If
                    /// an error occurs, that error will be returned via the completion
                    /// function.
                    ///
                    /// There is no guarantee that an asynchronous I/O will be issued unless
                    /// IOIssue() is called or the file handle is closed.
                    ///
                    /// Possible I/O errors:
                    ///
                    ///   EsentDiskIOException            general I/O failure
                    ///   EsentDiskFullException          a write caused the file to be
                    ///                                   extended and there was insufficient
                    ///                                   disk space to meet the request
                    ///   EsentFileAccessDeniedException  a write was issued to a read only file
                    /// </remarks>
                    /// <param name="offsetInBytes">The offset in the file of the IO.</param>
                    /// <param name="data">The data payload for the IO.</param>
                    /// <param name="fileQOS">File Quality Of Service for the IO.</param>
                    /// <param name="ioComplete">An optional IO completion delegate.</param>
                    /// <param name="ioHandoff">An optional IO handoff delegate.</param>
                    void IOWrite(
                        Int64 offsetInBytes,
                        ArraySegment<byte> data,
                        FileQOS fileQOS,
                        IOComplete^ ioComplete,
                        IOHandoff^ ioHandoff );

                    /// <summary>
                    /// Causes any unissued asynchronous I/Os for the current file to be issued eventually.
                    /// </summary>
                    void IOIssue();

                    /// <summary>
                    /// Get current attribute list size (NTFS only).
                    /// </summary>
                    /// <returns>The size in bytes of the attribute list size.</returns>
                    Int64 NTFSAttributeListSize();

                    /// <summary>
                    /// Get disk ID for this file.
                    /// </summary>
                    /// <returns>The disk ID of the file.</returns>
                    IntPtr DiskId();

                    /// <summary>
                    /// Get number of write IOs that are unflushed or flush pending 
                    /// since last ErrFlushFileBuffers call.
                    /// </summary>
                    /// <returns>The current unflushed write count for the file.</returns>
                    Int64 CountIoNonFlushed();

                    /// <summary>
                    /// Get seek penalty (in order to identify SSD).
                    /// </summary>
                    /// <returns>True if random IO on the device is slow.</returns>
                    bool SeekPenalty();
                };
            }
        }
    }
}