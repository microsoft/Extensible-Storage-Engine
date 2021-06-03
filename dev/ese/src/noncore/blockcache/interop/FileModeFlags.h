// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

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
                /// Flags to control the creation and operating mode of / for a given file.
                /// </summary>
                [Flags]
                public enum class FileModeFlags : int
                {
                    /// <summary>
                    /// A regular file means read-write (!read-only), non-cached / non-write-back, non-temporary.
                    /// </summary>
                    None = 0,

                    /// <summary>
                    /// A regular file means read-write (!read-only), non-cached / non-write-back, non-temporary.
                    /// </summary>
                    Regular = 0,

                    /// <summary>
                    /// Avoid passing (as is the default) FILE_FLAG_NO_BUFFERING to OS, and instead passing FILE_FLAG_RANDOM_ACCESS.
                    /// </summary>
                    Cached = 0x1,

                    /// <summary>
                    /// Allows ESE to open a file without FILE_FLAG_WRITE_THROUGH to allow HD based caching.
                    /// </summary>
                    StorageWriteBack = 0x2,

                    /// <summary>
                    /// If fmfCached also specified, then this avoids passing FILE_FLAG_WRITE_THROUGH to the OS, with further no
                    /// intention of using FFB to maintain consistency.
                    /// </summary>
                    LossyWriteBack = 0x4,

                    /// <summary>
                    /// Controls if we try to put the file into sparse mode (FSCTL_SET_SPARSE set to true).
                    /// </summary>
                    Sparse = 0x8,

                    /// <summary>
                    /// Utilize FILE_FLAG_DELETE_ON_CLOSE when creating the file.
                    /// </summary>
                    Temporary = 0x100,

                    /// <summary>
                    /// Passed CREATE_ALWAYS (instead of CREATE_NEW) for the create disposition.
                    /// </summary>
                    OverwriteExisting = 0x200,

                    /// <summary>
                    /// With this flag we do not request GENERIC_WRITE in the permissions (we always ask for GENERIC_READ).
                    /// </summary>
                    ReadOnly = 0x10000,

                    /// <summary>
                    /// With this flag we disallow in the OS layer any write file APIs, but still open the file with GENERIC_WRITE
                    /// (to support FFB).
                    /// </summary>
                    ReadOnlyClient = 0x20000,

                    /// <summary>
                    /// With this flag we do read only, but also allow SHARE_WRITE (called "Mode D" in ErrFileOpen) allowing
                    /// interoperability with fmfReadOnlyClient.  Downside it doesn't lock out a errant writers.
                    /// </summary>
                    ReadOnlyPermissive = 0x40000,
                };
            }
        }
    }
}
