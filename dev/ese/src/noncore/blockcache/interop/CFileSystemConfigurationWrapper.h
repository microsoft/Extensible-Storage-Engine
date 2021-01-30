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
                class CFileSystemConfigurationWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CFileSystemConfigurationWrapper( TM^ fsconfig ) : CWrapper( fsconfig ) { }

                    public:

                        ULONG DtickAccessDeniedRetryPeriod() override;
                        ULONG DtickHungIOThreshhold() override;
                        DWORD GrbitHungIOActions() override;
                        ULONG CbMaxReadSize() override;
                        ULONG CbMaxWriteSize() override;
                        ULONG CbMaxReadGapSize() override;
                        ULONG PermillageSmoothIo() override;
                        BOOL FBlockCacheEnabled() override;
                        ERR ErrGetBlockCacheConfiguration( _Out_ ::IBlockCacheConfiguration** const ppbcconfig ) override;

                    public:

                        void EmitEvent( const EEventType    type,
                                        const CategoryId    catid,
                                        const MessageId     msgid,
                                        const DWORD         cString,
                                        const WCHAR *       rgpwszString[],
                                        const LONG          lEventLoggingLevel ) override;

                        void EmitEvent( int                 haTag,
                                        const CategoryId    catid,
                                        const MessageId     msgid,
                                        const DWORD         cString,
                                        const WCHAR *       rgpwszString[],
                                        int                 haCategory,
                                        const WCHAR*        wszFilename,
                                        unsigned _int64     qwOffset,
                                        DWORD               cbSize ) override;

                        void EmitFailureTag(    const int           haTag,
                                                const WCHAR* const  wszGuid,
                                                const WCHAR* const  wszAdditional ) override;

                        const void* const PvTraceContext() override;
                };

                template< class TM, class TN >
                inline ULONG CFileSystemConfigurationWrapper<TM,TN>::DtickAccessDeniedRetryPeriod()
                {
                    return (int)I()->AccessDeniedRetryPeriod().TotalMilliseconds;
                }

                template< class TM, class TN >
                inline ULONG CFileSystemConfigurationWrapper<TM,TN>::DtickHungIOThreshhold()
                {
                    return (int)I()->HungIOThreshhold().TotalMilliseconds;
                }

                template< class TM, class TN >
                inline DWORD CFileSystemConfigurationWrapper<TM,TN>::GrbitHungIOActions()
                {
                    return I()->GrbitHungIOActions();
                }

                template< class TM, class TN >
                inline ULONG CFileSystemConfigurationWrapper<TM,TN>::CbMaxReadSize()
                {
                    return I()->MaxReadSizeInBytes();
                }

                template< class TM, class TN >
                inline ULONG CFileSystemConfigurationWrapper<TM,TN>::CbMaxWriteSize()
                {
                    return I()->MaxWriteSizeInBytes();
                }

                template< class TM, class TN >
                inline ULONG CFileSystemConfigurationWrapper<TM,TN>::CbMaxReadGapSize()
                {
                    return I()->MaxReadGapSizeInBytes();
                }

                template< class TM, class TN >
                inline ULONG CFileSystemConfigurationWrapper<TM,TN>::PermillageSmoothIo()
                {
                    return I()->PermillageSmoothIo();
                }

                template< class TM, class TN >
                inline BOOL CFileSystemConfigurationWrapper<TM,TN>::FBlockCacheEnabled()
                {
                    return I()->IsBlockCacheEnabled() ? fTrue : fFalse;
                }

                template< class TM, class TN >
                inline ERR CFileSystemConfigurationWrapper<TM, TN>::ErrGetBlockCacheConfiguration( _Out_ ::IBlockCacheConfiguration** const ppbcconfig )
                {
                    ERR                         err         = JET_errSuccess;
                    IBlockCacheConfiguration^   bcconfig    = nullptr;

                    *ppbcconfig = NULL;

                    ExCall( bcconfig = I()->BlockCacheConfiguration() );

                    Call( BlockCacheConfiguration::ErrWrap( bcconfig, ppbcconfig ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        delete bcconfig;
                        delete *ppbcconfig;
                        *ppbcconfig = NULL;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline void CFileSystemConfigurationWrapper<TM,TN>::EmitEvent(  const EEventType    type,
                                                                                const CategoryId    catid,
                                                                                const MessageId     msgid,
                                                                                const DWORD         cString,
                                                                                const WCHAR *       rgpwszString[],
                                                                                const LONG          lEventLoggingLevel )
                {
                }

                template< class TM, class TN >
                inline void CFileSystemConfigurationWrapper<TM,TN>::EmitEvent(  int                 haTag,
                                                                                const CategoryId    catid,
                                                                                const MessageId     msgid,
                                                                                const DWORD         cString,
                                                                                const WCHAR *       rgpwszString[],
                                                                                int                 haCategory,
                                                                                const WCHAR*        wszFilename,
                                                                                unsigned _int64     qwOffset,
                                                                                DWORD               cbSize )
                {
                }

                template< class TM, class TN >
                inline void CFileSystemConfigurationWrapper<TM,TN>::EmitFailureTag( const int           haTag,
                                                                                    const WCHAR* const  wszGuid,
                                                                                    const WCHAR* const  wszAdditional )
                {
                }

                template< class TM, class TN >
                inline const void* const CFileSystemConfigurationWrapper<TM,TN>::PvTraceContext()
                {
                    return NULL;
                }
            }
        }
    }
}
