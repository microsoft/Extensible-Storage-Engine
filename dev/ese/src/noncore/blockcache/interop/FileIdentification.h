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
                public ref class FileIdentification : public FileIdentificationBase<IFileIdentification, ::IFileIdentification, CFileIdentificationWrapper<IFileIdentification, ::IFileIdentification>>
                {
                    public:

                        static ERR ErrWrap( IFileIdentification^% fident, ::IFileIdentification** const ppfident )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<IFileIdentification, ::IFileIdentification, CFileIdentificationWrapper<IFileIdentification, ::IFileIdentification>, FileIdentificationRemotable>( fident, ppfident );
                        }

                    public:

                        FileIdentification( IFileIdentification^ fident ) : FileIdentificationBase( fident ) { }

                        FileIdentification( ::IFileIdentification** const ppfident ) : FileIdentificationBase( ppfident ) {}
                };
            }
        }
    }
}
