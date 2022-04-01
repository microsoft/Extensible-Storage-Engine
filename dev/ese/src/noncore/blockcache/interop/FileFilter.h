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
                public ref class FileFilter : FileFilterBase<IFileFilter,::IFileFilter,CFileFilterWrapper<IFileFilter,::IFileFilter>>
                {
                    public:

                        static ERR ErrWrap( IFileFilter^% ff, ::IFileFilter** const ppffapi )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<IFileFilter,::IFileFilter,CFileFilterWrapper<IFileFilter,::IFileFilter>, FileFilterRemotable>( ff, ppffapi );
                        }

                    public:

                        FileFilter( IFileFilter^ ff ) : FileFilterBase( ff ) { }

                        FileFilter( ::IFileFilter** const ppff ) : FileFilterBase( ppff ) { }
                };
            }
        }
    }
}
