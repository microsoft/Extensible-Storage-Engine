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
                public ref class FileSystemConfigurationBase : Base<TM,TN,TW>, IFileSystemConfiguration
                {
                    public:

                        FileSystemConfigurationBase( TM^ fsconfig ) : Base( fsconfig ) { }

                        FileSystemConfigurationBase( TN** const ppfsconfig ) : Base( ppfsconfig ) {}

                        FileSystemConfigurationBase( TN* const pfsconfig ) : Base( pfsconfig ) {}

                    public:

                        virtual TimeSpan AccessDeniedRetryPeriod();

                        virtual int MaxConcurrentIO();

                        virtual int MaxConcurrentBackgroundIO();

                        virtual TimeSpan HungIOThreshhold();

                        virtual int GrbitHungIOActions();

                        virtual int MaxReadSizeInBytes();

                        virtual int MaxWriteSizeInBytes();

                        virtual int MaxReadGapSizeInBytes();

                        virtual int PermillageSmoothIo();

                        virtual bool IsBlockCacheEnabled();

                        virtual IBlockCacheConfiguration^ BlockCacheConfiguration();
                };

                template< class TM, class TN, class TW >
                inline TimeSpan FileSystemConfigurationBase<TM,TN,TW>::AccessDeniedRetryPeriod()
                {
                    return TimeSpan::FromMilliseconds( Pi->DtickAccessDeniedRetryPeriod() );
                }

                template< class TM, class TN, class TW >
                inline int FileSystemConfigurationBase<TM, TN, TW>::MaxConcurrentIO()
                {
                    return Pi->CIOMaxOutstanding();
                }

                template< class TM, class TN, class TW >
                inline int FileSystemConfigurationBase<TM, TN, TW>::MaxConcurrentBackgroundIO()
                {
                    return Pi->CIOMaxOutstandingBackground();
                }

                template< class TM, class TN, class TW >
                inline TimeSpan FileSystemConfigurationBase<TM,TN,TW>::HungIOThreshhold()
                {
                    return TimeSpan::FromMilliseconds( Pi->DtickHungIOThreshhold() );
                }

                template< class TM, class TN, class TW >
                inline int FileSystemConfigurationBase<TM,TN,TW>::GrbitHungIOActions()
                {
                    return Pi->GrbitHungIOActions();
                }

                template< class TM, class TN, class TW >
                inline int FileSystemConfigurationBase<TM,TN,TW>::MaxReadSizeInBytes()
                {
                    return Pi->CbMaxReadSize();
                }

                template< class TM, class TN, class TW >
                inline int FileSystemConfigurationBase<TM,TN,TW>::MaxWriteSizeInBytes()
                {
                    return Pi->CbMaxWriteSize();
                }

                template< class TM, class TN, class TW >
                inline int FileSystemConfigurationBase<TM,TN,TW>::MaxReadGapSizeInBytes()
                {
                    return Pi->CbMaxReadGapSize();
                }

                template< class TM, class TN, class TW >
                inline int FileSystemConfigurationBase<TM,TN,TW>::PermillageSmoothIo()
                {
                    return Pi->PermillageSmoothIo();
                }

                template< class TM, class TN, class TW >
                inline bool FileSystemConfigurationBase<TM,TN,TW>::IsBlockCacheEnabled()
                {
                    return Pi->FBlockCacheEnabled();
                }

                template< class TM, class TN, class TW >
                inline IBlockCacheConfiguration^ FileSystemConfigurationBase<TM, TN, TW>::BlockCacheConfiguration()
                {
                    ERR                         err         = JET_errSuccess;
                    ::IBlockCacheConfiguration* pbcconfig   = NULL;

                    Call( Pi->ErrGetBlockCacheConfiguration( &pbcconfig ) );

                    return pbcconfig ? gcnew Internal::Ese::BlockCache::Interop::BlockCacheConfiguration( &pbcconfig ) : nullptr;

                HandleError:
                    delete pbcconfig;
                    throw EseException( err );
                }
            }
        }
    }
}
