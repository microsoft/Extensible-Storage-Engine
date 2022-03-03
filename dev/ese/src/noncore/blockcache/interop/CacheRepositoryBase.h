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
                public ref class CacheRepositoryBase : public Base<TM, TN, TW>, ICacheRepository
                {
                    public:

                        CacheRepositoryBase( TM^ crep ) : Base( crep ) { }

                        CacheRepositoryBase( TN** const ppcrep ) : Base( ppcrep ) {}

                    public:

                        virtual ICache^ Open(
                            IFileSystemFilter^ fsf,
                            IFileSystemConfiguration^ fsconfig,
                            ICacheConfiguration^ cconfig );

                        virtual ICache^ OpenById(
                            IFileSystemFilter^ fsf,
                            IFileSystemConfiguration^ fsconfig,
                            IBlockCacheConfiguration^ bcconfig,
                            VolumeId volumeid,
                            FileId fileid,
                            Guid^ guid );
                };

                template< class TM, class TN, class TW >
                inline ICache^ CacheRepositoryBase<TM, TN, TW>::Open(
                    IFileSystemFilter^ fsf,
                    IFileSystemConfiguration^ fsconfig,
                    ICacheConfiguration^ cconfig )
                {
                    ERR                         err         = JET_errSuccess;
                    ::IFileSystemFilter*        pfsf        = NULL;
                    ::IFileSystemConfiguration* pfsconfig   = NULL;
                    ::ICacheConfiguration*      pcconfig    = NULL;
                    ::ICache*                   pc          = NULL;
                    Cache^                      c           = nullptr;

                    Call( FileSystemFilter::ErrWrap( fsf, &pfsf ) );
                    Call( FileSystemConfiguration::ErrWrap( fsconfig, &pfsconfig ) );
                    Call( CacheConfiguration::ErrWrap( cconfig, &pcconfig ) );
                    Call( Pi->ErrOpen( pfsf, pfsconfig, &pcconfig, &pc ) );

                    c = pc ? gcnew Cache( &pc ) : nullptr;

                    delete pc;
                    delete pcconfig;
                    delete pfsconfig;
                    delete pfsf;
                    return c;

                HandleError:
                    delete pc;
                    delete pcconfig;
                    delete pfsconfig;
                    delete pfsf;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline ICache^ CacheRepositoryBase<TM, TN, TW>::OpenById(
                    IFileSystemFilter^ fsf,
                    IFileSystemConfiguration^ fsconfig,
                    IBlockCacheConfiguration^ bcconfig,
                    VolumeId volumeid,
                    FileId fileid,
                    Guid^ guid )
                {
                    ERR                         err         = JET_errSuccess;
                    ::IFileSystemFilter*        pfsf        = NULL;
                    ::IFileSystemConfiguration* pfsconfig   = NULL;
                    ::IBlockCacheConfiguration* pbcconfig   = NULL;
                    ::ICache*                   pc          = NULL;
                    Cache^                      c           = nullptr;

                    Call( FileSystemFilter::ErrWrap( fsf, &pfsf ) );
                    Call( FileSystemConfiguration::ErrWrap( fsconfig, &pfsconfig ) );
                    Call( BlockCacheConfiguration ::ErrWrap( bcconfig, &pbcconfig ) );
                    array<Byte>^ uniqueIdBytes = guid->ToByteArray();
                    pin_ptr<Byte> uniqueId = &uniqueIdBytes[ 0 ];
                    Call( Pi->ErrOpenById(  pfsf,
                                            pfsconfig,
                                            pbcconfig,
                                            (::VolumeId)volumeid,
                                            (::FileId)fileid,
                                            uniqueId,
                                            &pc ) );

                    c = pc ? gcnew Cache( &pc ) : nullptr;

                    delete pbcconfig;
                    delete pfsconfig;
                    delete pfsf;
                    return c;

                HandleError:
                    delete pbcconfig;
                    delete pfsconfig;
                    delete pfsf;
                    throw EseException( err );
                }
            }
        }
    }
}
