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
                /// File Filter interface.
                /// </summary>
                public interface class IFileFilter : IFile
                {
                    /// <summary>
                    /// Returns the physical id of the current file.
                    /// </summary>
                    /// <param name="volumeid">The volume id of the current file.</param>
                    /// <param name="fileid">The file id of the current file.</param>
                    /// <param name="fileserial">The serial number of the current file.</param>
                    void GetPhysicalId(
                        [Out] VolumeId% volumeid,
                        [Out] FileId% fileid,
                        [Out] FileSerial% fileserial );

                    /// <summary>
                    /// Reads a block of data of the specified size from the current file at the specified offset and
                    /// places it into the specified buffer.
                    /// </summary>
                    /// <remarks>
                    /// The offset, size, and buffer pointer must be aligned to the block size for the file.
                    ///
                    /// If no completion function is given, the function will not return
                    /// until the I/O has been completed.  If an error occurs, that error
                    /// will be thrown as an exception.
                    ///
                    /// If a completion function is given, the function will return
                    /// immediately and the I/O will be processed asynchronously.  The
                    /// completion function will be called when the I/O has completed.  If
                    /// an error occurs, that error will be returned via the completion
                    /// function.  No error will be returned from this function.  There is
                    /// no guarantee that an asynchronous I/O will be issued unless
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
                    /// <param name="ioMode">IO operation mode.</param>
                    /// <param name="ioComplete">An optional IO completion delegate.</param>
                    /// <param name="ioHandoff">An optional IO handoff delegate.</param>
                    void Read(
                        Int64 offsetInBytes,
                        ArraySegment<byte> data,
                        FileQOS fileQOS,
                        IOMode ioMode,
                        IOComplete^ ioComplete,
                        IOHandoff^ ioHandoff );

                    /// <summary>
                    /// Writes a block of data of the specified size from the specified buffer at the specified offset
                    /// and places it into the current file.
                    /// </summary>
                    /// <remarks>
                    /// The offset, size, and buffer pointer must be aligned to the block size for the file.
                    ///
                    /// If no completion function is given, the function will not return
                    /// until the I/O has been completed.  If an error occurs, that error
                    /// will be thrown as an exception.
                    ///
                    /// If a completion function is given, the function will return
                    /// immediately and the I/O will be processed asynchronously.  The
                    /// completion function will be called when the I/O has completed.  If
                    /// an error occurs, that error will be returned via the completion
                    /// function.  No error will be returned from this function.  There is
                    /// no guarantee that an asynchronous I/O will be issued unless
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
                    /// <param name="ioMode">IO operation mode.</param>
                    /// <param name="ioComplete">An optional IO completion delegate.</param>
                    /// <param name="ioHandoff">An optional IO handoff delegate.</param>
                    void Write(
                        Int64 offsetInBytes,
                        ArraySegment<byte> data,
                        FileQOS fileQOS,
                        IOMode ioMode,
                        IOComplete^ ioComplete,
                        IOHandoff^ ioHandoff );

                    /// <summary>
                    /// Causes any unissued asynchronous I/Os for the current file to be issued eventually.
                    /// </summary>
                    /// <param name="ioMode">IO operation mode.</param>
                    void Issue( IOMode ioMode );
                };
            }
        }
    }
}