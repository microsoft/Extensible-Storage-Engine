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
                public ref class FileSystemConfiguration : public FileSystemConfigurationBase<IFileSystemConfiguration,::IFileSystemConfiguration,CFileSystemConfigurationWrapper<IFileSystemConfiguration,::IFileSystemConfiguration>>
                {
                    public:

                        static ERR ErrWrap( IFileSystemConfiguration^% fsconfig, ::IFileSystemConfiguration** const ppfsconfig )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<IFileSystemConfiguration,::IFileSystemConfiguration,CFileSystemConfigurationWrapper<IFileSystemConfiguration,::IFileSystemConfiguration>, FileSystemConfigurationRemotable>( fsconfig, ppfsconfig );
                        }

                    public:

                        FileSystemConfiguration( IFileSystemConfiguration^ fsconfig ) : FileSystemConfigurationBase( fsconfig ) { }

                        FileSystemConfiguration( ::IFileSystemConfiguration** const ppfsconfig ) : FileSystemConfigurationBase( ppfsconfig ) { }

                        FileSystemConfiguration( ::IFileSystemConfiguration* const pfsconfig ) : FileSystemConfigurationBase( pfsconfig ) {}
                };
            }
        }
    }
}
