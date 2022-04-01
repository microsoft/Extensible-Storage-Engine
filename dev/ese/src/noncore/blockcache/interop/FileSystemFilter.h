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
                public ref class FileSystemFilter : FileSystemFilterBase<IFileSystemFilter,::IFileSystemFilter,CFileSystemFilterWrapper<IFileSystemFilter,::IFileSystemFilter>>
                {
                    public:

                        static ERR ErrWrap( IFileSystemFilter^% fsf, ::IFileSystemFilter** const ppfsfapi )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<IFileSystemFilter,::IFileSystemFilter,CFileSystemFilterWrapper<IFileSystemFilter,::IFileSystemFilter>, FileSystemFilterRemotable>( fsf, ppfsfapi );
                        }

                    public:

                        FileSystemFilter( IFileSystemFilter^ fsf ) : FileSystemFilterBase( fsf ) { }

                        FileSystemFilter( ::IFileSystemFilter** const ppfsf ) : FileSystemFilterBase( ppfsf ) {}

                        FileSystemFilter( ::IFileSystemFilter* const pfsf ) : FileSystemFilterBase( pfsf ) {}
                };
            }
        }
    }
}
