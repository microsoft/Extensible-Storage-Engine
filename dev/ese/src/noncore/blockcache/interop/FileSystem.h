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
                public ref class FileSystem : public FileSystemBase<IFileSystem,IFileSystemAPI,CFileSystemWrapper<IFileSystem,IFileSystemAPI>>
                {
                    public:

                        static ERR ErrWrap( IFileSystem^% fs, IFileSystemAPI** const ppfsapi )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<IFileSystem,IFileSystemAPI,CFileSystemWrapper<IFileSystem,IFileSystemAPI>, FileSystemRemotable>( fs, ppfsapi );
                        }

                    public:

                        FileSystem( IFileSystem^ fs ) : FileSystemBase( fs ) { }

                        FileSystem( IFileSystemAPI** const ppfsapi ) : FileSystemBase( ppfsapi ) {}
                };
            }
        }
    }
}
