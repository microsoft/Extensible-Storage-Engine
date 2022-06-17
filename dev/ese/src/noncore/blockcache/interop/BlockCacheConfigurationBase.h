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
                public ref class BlockCacheConfigurationBase : Base<TM, TN, TW>, IBlockCacheConfiguration
                {
                    public:

                        BlockCacheConfigurationBase( TM^ bcconfig ) : Base( bcconfig ) { }

                        BlockCacheConfigurationBase( TN** const ppbcconfig ) : Base( ppbcconfig ) {}

                        BlockCacheConfigurationBase( TN* const pbcconfig ) : Base( pbcconfig ) {}

                    public:

                        virtual ICachedFileConfiguration^ CachedFileConfiguration( String^ keyPathCachedFile );

                        virtual ICacheConfiguration^ CacheConfiguration( String^ keyPathCachingFile );

                        virtual bool IsDetachEnabled();
                };

                template< class TM, class TN, class TW >
                inline ICachedFileConfiguration^ BlockCacheConfigurationBase<TM, TN, TW>::CachedFileConfiguration( String^ keyPathCachedFile )
                {
                    ERR                         err         = JET_errSuccess;
                    ::ICachedFileConfiguration* pcfconfig   = NULL;

                    pin_ptr<const Char> wszKeyPathCachedFile = PtrToStringChars( keyPathCachedFile );
                    Call( Pi->ErrGetCachedFileConfiguration( (const WCHAR*)wszKeyPathCachedFile, &pcfconfig ) );

                    return pcfconfig ? gcnew Internal::Ese::BlockCache::Interop::CachedFileConfiguration( &pcfconfig ) : nullptr;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline ICacheConfiguration^ BlockCacheConfigurationBase<TM, TN, TW>::CacheConfiguration( String^ keyPathCachingFile )
                {
                    ERR                         err         = JET_errSuccess;
                    ::ICacheConfiguration*      pcconfig    = NULL;

                    pin_ptr<const Char> wszKeyPathCachingFile = PtrToStringChars( keyPathCachingFile );
                    Call( Pi->ErrGetCacheConfiguration( (const WCHAR*)wszKeyPathCachingFile, &pcconfig ) );

                    return pcconfig ? gcnew Internal::Ese::BlockCache::Interop::CacheConfiguration( &pcconfig ) : nullptr;

                HandleError:
                    delete pcconfig;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline bool BlockCacheConfigurationBase<TM, TN, TW>::IsDetachEnabled()
                {
                    return Pi->FDetachEnabled() ? true : false;
                }
            }
        }
    }
}
