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
                class CCacheRepositoryWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CCacheRepositoryWrapper( TM^ crep ) : CWrapper( crep ) { }

                    public:

                        ERR ErrOpen(    _In_    ::IFileSystemFilter* const          pfsf,
                                        _In_    ::IFileSystemConfiguration* const   pfsconfig,
                                        _Inout_ ::ICacheConfiguration** const       ppcconfig,
                                        _Out_   ::ICache** const                    ppc ) override;

                        ERR ErrOpenById(    _In_                ::IFileSystemFilter* const          pfsf,
                                            _In_                ::IFileSystemConfiguration* const   pfsconfig,
                                            _In_                ::IBlockCacheConfiguration* const   pbcconfig,
                                            _In_                const ::VolumeId                    volumeid,
                                            _In_                const ::FileId                      fileid,
                                            _In_reads_(cbGuid)  const BYTE* const                   rgbUniqueId,
                                            _Out_               ::ICache** const                    ppc ) override;
                };

                template< class TM, class TN >
                inline ERR CCacheRepositoryWrapper<TM, TN>::ErrOpen(    _In_    ::IFileSystemFilter* const          pfsf,
                                                                        _In_    ::IFileSystemConfiguration* const   pfsconfig,
                                                                        _Inout_ ::ICacheConfiguration** const       ppcconfig,
                                                                        _Out_   ::ICache** const                    ppc )
                {
                    ERR     err         = JET_errSuccess;
                    ICache^ c           = nullptr;

                    *ppc = NULL;

                    ExCall( c = I()->Open(  gcnew FileSystemFilter( pfsf ),
                                            gcnew FileSystemConfiguration( pfsconfig ),
                                            gcnew CacheConfiguration( ppcconfig ) ) );

                    Call( Cache::ErrWrap( c, ppc ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        delete c;
                        delete *ppc;
                        *ppc = NULL;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CCacheRepositoryWrapper<TM, TN>::ErrOpenById(    _In_                ::IFileSystemFilter* const          pfsf,
                                                                            _In_                ::IFileSystemConfiguration* const   pfsconfig,
                                                                            _In_                ::IBlockCacheConfiguration* const   pbcconfig,
                                                                            _In_                const ::VolumeId                    volumeid,
                                                                            _In_                const ::FileId                      fileid,
                                                                            _In_reads_(cbGuid)  const BYTE* const                   rgbUniqueId,
                                                                            _Out_               ::ICache** const                    ppc )
                {
                    ERR     err         = JET_errSuccess;
                    ICache^ c           = nullptr;
                    GUID*   pguid       = (GUID*)rgbUniqueId;
                    Guid    uniqueId    = Guid( pguid->Data1,
                                                pguid->Data2, 
                                                pguid->Data3,
                                                pguid->Data4[ 0 ],
                                                pguid->Data4[ 1 ],
                                                pguid->Data4[ 2 ],
                                                pguid->Data4[ 3 ],
                                                pguid->Data4[ 4 ],
                                                pguid->Data4[ 5 ],
                                                pguid->Data4[ 6 ],
                                                pguid->Data4[ 7 ] );

                    *ppc = NULL;

                    ExCall( c = I()->OpenById(  gcnew FileSystemFilter( pfsf ),
                                                gcnew FileSystemConfiguration( pfsconfig ),
                                                gcnew BlockCacheConfiguration( pbcconfig ),
                                                (VolumeId)volumeid,
                                                (FileId)(Int64)fileid,
                                                uniqueId ) );

                    Call( Cache::ErrWrap( c, ppc ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        delete c;
                        delete *ppc;
                        *ppc = NULL;
                    }
                    return err;
                }
            }
        }
    }
}
