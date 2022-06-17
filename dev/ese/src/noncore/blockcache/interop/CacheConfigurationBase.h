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
                template< class TM, class TN, class TW >
                public ref class CacheConfigurationBase : Base<TM, TN, TW>, ICacheConfiguration
                {
                    public:

                        CacheConfigurationBase( TM^ cconfig ) : Base( cconfig ) { }

                        CacheConfigurationBase( TN** const ppcconfig ) : Base( ppcconfig ) {}

                        CacheConfigurationBase( TN* const pcconfig ) : Base( pcconfig ) {}

                    public:

                        virtual bool IsCacheEnabled()
                        {
                            return Pi->FCacheEnabled() ? true : false;
                        }

                        virtual Guid CacheType()
                        {
                            GUID guidNative = { 0 };

                            Pi->CacheType( (BYTE*)&guidNative );

                            return Guid(    guidNative.Data1,
                                            guidNative.Data2,
                                            guidNative.Data3,
                                            guidNative.Data4[ 0 ],
                                            guidNative.Data4[ 1 ],
                                            guidNative.Data4[ 2 ],
                                            guidNative.Data4[ 3 ],
                                            guidNative.Data4[ 4 ],
                                            guidNative.Data4[ 5 ],
                                            guidNative.Data4[ 6 ],
                                            guidNative.Data4[ 7 ] );
                        }

                        virtual String^ Path()
                        {
                            WCHAR   wszPath[ IFileSystemAPI::cchPathMax ]   = { 0 };

                            Pi->Path( wszPath );

                            return gcnew String( wszPath );
                        }

                        virtual Int64 MaximumSize()
                        {
                            return Pi->CbMaximumSize();
                        }

                        virtual double PercentWrite()
                        {
                            return Pi->PctWrite();
                        }

                        virtual Int64 JournalSegmentsMaximumSize()
                        {
                            return Pi->CbJournalSegmentsMaximumSize();
                        }

                        virtual double PercentJournalSegmentsInUse()
                        {
                            return Pi->PctJournalSegmentsInUse();
                        }

                        virtual Int64 JournalSegmentsMaximumCacheSize()
                        {
                            return Pi->CbJournalSegmentsMaximumCacheSize();
                        }

                        virtual Int64 JournalClustersMaximumSize()
                        {
                            return Pi->CbJournalClustersMaximumSize();
                        }

                        virtual Int64 CachingFilePerSlab()
                        {
                            return Pi->CbCachingFilePerSlab();
                        }

                        virtual Int64 CachedFilePerSlab()
                        {
                            return Pi->CbCachedFilePerSlab();
                        }

                        virtual Int64 SlabMaximumCacheSize()
                        {
                            return Pi->CbSlabMaximumCacheSize();
                        }

                        virtual bool IsAsyncWriteBackEnabled()
                        {
                            return Pi->FAsyncWriteBackEnabled() ? true : false;
                        }

                        virtual Int32 MaxConcurrentIODestage()
                        {
                            return Pi->CIOMaxOutstandingDestage();
                        }

                        virtual Int32 MaxConcurrentIOSizeDestage()
                        {
                            return Pi->CbIOMaxOutstandingDestage();
                        }
                };
            }
        }
    }
}
