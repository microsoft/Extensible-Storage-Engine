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
                template< class TM, class TN >
                class CBlockCacheConfigurationWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CBlockCacheConfigurationWrapper( TM^ bcconfig ) : CWrapper( bcconfig ) { }

                    public:

                        ERR ErrGetCachedFileConfiguration(  _In_z_  const WCHAR* const                  wszKeyPathCachedFile, 
                                                            _Out_   ::ICachedFileConfiguration** const  ppcfconfig  ) override;
                        ERR ErrGetCacheConfiguration(   _In_z_  const WCHAR* const              wszKeyPathCachingFile,
                                                        _Out_   ::ICacheConfiguration** const   ppcconfig ) override;
                };

                template< class TM, class TN >
                inline ERR CBlockCacheConfigurationWrapper<TM, TN>::ErrGetCachedFileConfiguration(  _In_z_  const WCHAR* const                  wszKeyPathCachedFile, 
                                                                                                    _Out_   ::ICachedFileConfiguration** const  ppcfconfig )
                {
                    ERR                         err         = JET_errSuccess;
                    ICachedFileConfiguration^   cfconfig    = nullptr;

                    *ppcfconfig = NULL;

                    ExCall( cfconfig = I()->CachedFileConfiguration( gcnew String( wszKeyPathCachedFile ) ) );

                    Call( CachedFileConfiguration::ErrWrap( cfconfig, ppcfconfig ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        delete cfconfig;
                        delete *ppcfconfig;
                        *ppcfconfig = NULL;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CBlockCacheConfigurationWrapper<TM, TN>::ErrGetCacheConfiguration(   _In_z_  const WCHAR* const              wszKeyPathCachingFile,
                                                                                                _Out_   ::ICacheConfiguration** const   ppcconfig )
                {
                    ERR                     err         = JET_errSuccess;
                    ICacheConfiguration^    cconfig     = nullptr;

                    *ppcconfig = NULL;

                    ExCall( cconfig = I()->CacheConfiguration( gcnew String( wszKeyPathCachingFile ) ) );

                    Call( CacheConfiguration::ErrWrap( cconfig, ppcconfig ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        delete cconfig;
                        delete *ppcconfig;
                        *ppcconfig = NULL;
                    }
                    return err;
                }
            }
        }
    }
}
