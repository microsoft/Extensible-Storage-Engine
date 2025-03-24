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
                /// File Find Interface.
                /// </summary>
                public interface class IFileFind : IDisposable
                {
                    /// <summary>
                    /// Moves to the next file or folder in the set of files or folders found by this File Find iterator.
                    /// </summary>
                    /// <returns>False if there are no more files or folders to be found.</returns>
                    bool Next();

                    /// <summary>
                    /// Returns the folder attribute of the current file or folder.
                    /// </summary>
                    /// <remarks>
                    /// If there is no current file or folder, EsentFileNotFoundException is thrown.
                    /// </remarks>
                    /// <returns>True if the matched item is a folder.</returns>
                    bool IsFolder();

                    /// <summary>
                    /// Retrieves the absolute path of the current file or folder.
                    /// </summary>
                    /// <remarks>
                    /// If there is no current file or folder, EsentFileNotFoundException is thrown.
                    /// </remarks>
                    /// <returns>The absolute path of the matched item.</returns>
                    String^ Path();

                    /// <summary>
                    /// Returns the size of the current file or folder.
                    /// </summary>
                    /// <remarks>
                    /// If there is no current file or folder, EsentFileNotFoundException is thrown.
                    /// </remarks>
                    /// <param name="fileSize">The type of size requested for the matched item.</param>
                    /// <returns>The size in bytes of the matched item.</returns>
                    Int64 Size( FileSize fileSize );

                    /// <summary>
                    /// Returns the read only attribute of the current file or folder.
                    /// </summary>
                    /// <remarks>
                    /// If there is no current file or folder, EsentFileNotFoundException is thrown.
                    /// </remarks>
                    /// <returns>True if the mached item is read only.</returns>
                    bool IsReadOnly();
                };
            }
        }
    }
}