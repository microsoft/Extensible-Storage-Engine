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
                public interface class IFile : IDisposable
                {
                    delegate void IOComplete(
                        EsentErrorException^ ex,
                        IFile^ file,
                        FileQOS fileQOS,
                        Int64 offsetInBytes,
                        ArraySegment<byte> data );

                    delegate void IOHandoff(
                        EsentErrorException^ ex,
                        IFile^ file,
                        FileQOS fileQOS,
                        Int64 offsetInBytes,
                        ArraySegment<byte> data );

                    FileModeFlags FileModeFlags();

                    void FlushFileBuffers();

                    void SetNoFlushNeeded();

                    String^ Path();

                    Int64 Size( FileSize fileSize );

                    bool IsReadOnly();

                    int CountLogicalCopies();

                    void SetSize(Int64 sizeInBytes, bool zeroFill, FileQOS fileQOS);

                    void Rename(String^ pathDest, bool overwriteExisting);

                    void SetSparseness();

                    void IOTrim( Int64 offsetInBytes, Int64 sizeInBytes );

                    void RetrieveAllocatedRegion(
                        Int64 offsetToQuery, 
                        [Out] Int64% offsetInBytes, 
                        [Out] Int64% sizeInBytes);

                    int IOSize();

                    int SectorSize();

                    void IORead(
                        Int64 offsetInBytes,
                        ArraySegment<byte> data,
                        FileQOS fileQOS,
                        IOComplete^ ioComplete,
                        IOHandoff^ ioHandoff );

                    void IOWrite(
                        Int64 offsetInBytes,
                        ArraySegment<byte> data,
                        FileQOS fileQOS,
                        IOComplete^ ioComplete,
                        IOHandoff^ ioHandoff );

                    void IOIssue();

                    Int64 NTFSAttributeListSize();

                    IntPtr DiskId();

                    Int64 CountIoNonFlushed();

                    bool SeekPenalty();
                };
            }
        }
    }
}
