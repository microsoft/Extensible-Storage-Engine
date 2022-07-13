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
                /// Cache Interface.
                /// </summary>
                public interface class ICache : IDisposable
                {
                    /// <summary>
                    /// Delegate that will be called when a cache read/write has completed.
                    /// </summary>
                    /// <remarks>
                    /// The function will be given the error code and parameters for the completed request.
                    /// </remarks>
                    /// <param name="ex">An EsentErrorException or null if the IO was successful.</param>
                    /// <param name="volumeid">The volume id of the cached file.</param>
                    /// <param name="fileid">The file id of the cached file.</param>
                    /// <param name="fileserial">The serial number of the cached file.</param>
                    /// <param name="fileQOS">File Quality Of Service for the IO.</param>
                    /// <param name="offsetInBytes">The offset of the region of the cached file affected by the request.</param>
                    /// <param name="data">The data payload for the request.</param>
                    delegate void Complete(
                        EsentErrorException^ ex,
                        VolumeId volumeid, 
                        FileId fileid,
                        FileSerial fileserial,
                        FileQOS fileQOS,
                        Int64 offsetInBytes,
                        MemoryStream^ data );

                    /// <summary>
                    /// Creates a new cache.
                    /// </summary>
                    void Create();

                    /// <summary>
                    /// Mounts an existing cache.
                    /// </summary>
                    void Mount();

                    /// <summary>
                    /// Prepares for a clean dismount of the cache.
                    /// </summary>
                    void PrepareToDismount();

                    /// <summary>
                    /// Indicates if the cache is currently enabled.
                    /// </summary>
                    /// <returns>True if the cache is currently enabled.</returns>
                    bool IsEnabled();

                    /// <summary>
                    /// Returns the physical identity of the caching file.
                    /// </summary>
                    /// <param name="volumeid">The volume id of the caching file.</param>
                    /// <param name="FileId">The file id of the caching file.</param>
                    /// <param name="guid">The guid of the caching file.</param>
                    void GetPhysicalId( [Out] VolumeId% volumeid, [Out] FileId% fileid, [Out] Guid% guid );

                    /// <summary>
                    /// Closes the given cached file if currently open.
                    /// </summary>
                    /// <param name="volumeid">The volume id of the cached file.</param>
                    /// <param name="fileid">The file id of the cached file.</param>
                    /// <param name="fileserial">The serial number of the cached file.</param>
                    void Close( VolumeId volumeid, FileId fileid, FileSerial fileserial );

                    /// <summary>
                    /// Flushes all data previously written for the given cached file.
                    /// </summary>
                    /// <param name="volumeid">The volume id of the cached file.</param>
                    /// <param name="fileid">The file id of the cached file.</param>
                    /// <param name="fileserial">The serial number of the cached file.</param>
                    void Flush( VolumeId volumeid, FileId fileid, FileSerial fileserial );

                    /// <summary>
                    /// Delegate that will be called during destage to indicate progress.
                    /// </summary>
                    /// <param name="i">The amount of work completed.</param>
                    /// <param name="c">The total amount of work required.</param>
                    delegate void DestageStatus( int i, int c );

                    /// <summary>
                    /// Writes all data previously written for the given cached file back to that cached file.
                    /// </summary>
                    /// <param name="volumeid">The volume id of the cached file.</param>
                    /// <param name="fileid">The file id of the cached file.</param>
                    /// <param name="fileserial">The serial number of the cached file.</param>
                    void Destage(
                        VolumeId volumeid, 
                        FileId fileid,
                        FileSerial fileserial, 
                        DestageStatus^ destageStatus );

                    /// <summary>
                    /// Invalidates cached data for the specified offset range of the given cached file.
                    /// </summary>
                    /// <param name="volumeid">The volume id of the cached file.</param>
                    /// <param name="fileid">The file id of the cached file.</param>
                    /// <param name="fileserial">The serial number of the cached file.</param>
                    /// <param name="offsetInBytes">The offset of the region of the cached file to invalidate.</param>
                    /// <param name="sizeInBytes">The size of the region of the cached file to invalidate.</param>
                    void Invalidate(
                        VolumeId volumeid, 
                        FileId fileid, 
                        FileSerial fileserial, 
                        Int64 offsetInBytes, 
                        Int64 sizeInBytes );

                    /// <summary>
                    /// Reads a block of data of the specified size from the given cached file at the specified offset
                    /// and places it into the specified buffer.
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
                    /// If asynchronous completion is requested by providing a completion function then there is no
                    /// guarantee that completion will occur until Issue() is called.
                    /// </remarks>
                    /// <param name="volumeid">The volume id of the cached file.</param>
                    /// <param name="fileid">The file id of the cached file.</param>
                    /// <param name="fileserial">The serial number of the cached file.</param>
                    /// <param name="offsetInBytes">The offset of the region of the cached file.</param>
                    /// <param name="data">The data payload for the request.</param>
                    /// <param name="fileQOS">File Quality Of Service for the request.</param>
                    /// <param name="cachingPolicy">Caching policy.</param>
                    /// <param name="complete">A request completion delegate.</param>
                    void Read(
                        VolumeId volumeid,
                        FileId fileid,
                        FileSerial fileserial,
                        Int64 offsetInBytes,
                        MemoryStream^ data,
                        FileQOS fileQOS,
                        CachingPolicy cachingPolicy,
                        Complete^ complete );

                    /// <summary>
                    /// Writes a block of data of the specified size from the specified buffer at the specified offset
                    /// and places it into the given cached file.
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
                    /// If asynchronous completion is requested by providing a completion function then there is no
                    /// guarantee that completion will occur until Issue() is called.
                    /// </remarks>
                    /// <param name="volumeid">The volume id of the cached file.</param>
                    /// <param name="fileid">The file id of the cached file.</param>
                    /// <param name="fileserial">The serial number of the cached file.</param>
                    /// <param name="offsetInBytes">The offset of the region of the cached file.</param>
                    /// <param name="data">The data payload for the request.</param>
                    /// <param name="fileQOS">File Quality Of Service for the request.</param>
                    /// <param name="cachingPolicy">Caching policy.</param>
                    /// <param name="complete">A request completion delegate.</param>
                    void Write(
                        VolumeId volumeid,
                        FileId fileid,
                        FileSerial fileserial,
                        Int64 offsetInBytes,
                        MemoryStream^ data,
                        FileQOS fileQOS,
                        CachingPolicy cachingPolicy,
                        Complete^ complete );

                    /// <summary>
                    /// Causes any previously requested asynchronous cache reads or writes to be executed eventually.
                    /// </summary>
                    /// <param name="volumeid">The volume id of the cached file.</param>
                    /// <param name="fileid">The file id of the cached file.</param>
                    /// <param name="fileserial">The serial number of the cached file.</param>
                    void Issue(
                        VolumeId volumeid,
                        FileId fileid,
                        FileSerial fileserial );
                };
            }
        }
    }
}