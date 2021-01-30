// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSFSCONFIG_HXX_INCLUDED
#define _OSFSCONFIG_HXX_INCLUDED

#include "osblockcacheconfig.hxx"


class IFileSystemConfiguration
{

    public:



        virtual ULONG DtickAccessDeniedRetryPeriod() = 0;



        virtual ULONG DtickHungIOThreshhold() = 0;


        virtual DWORD GrbitHungIOActions() = 0;


        virtual ULONG CbMaxReadSize() = 0;


        virtual ULONG CbMaxWriteSize() = 0;


        virtual ULONG CbMaxReadGapSize() = 0;


        virtual ULONG PermillageSmoothIo() = 0;


        virtual BOOL FBlockCacheEnabled() = 0;


        virtual ERR ErrGetBlockCacheConfiguration( _Out_ IBlockCacheConfiguration** const ppbcconfig ) = 0;


    public:

        virtual void EmitEvent( const EEventType    type,
                                const CategoryId    catid,
                                const MessageId     msgid,
                                const DWORD         cString,
                                const WCHAR *       rgpszString[],
                                const LONG          lEventLoggingLevel ) = 0;

        virtual void EmitEvent( int                 haTag,
                                const CategoryId    catid,
                                const MessageId     msgid,
                                const DWORD         cString,
                                const WCHAR *       rgpszString[],
                                int                 haCategory,
                                const WCHAR*        wszFilename,
                                unsigned _int64     qwOffset = 0,
                                DWORD               cbSize = 0 ) = 0;

        virtual void EmitFailureTag(    const int           haTag,
                                        const WCHAR* const  wszGuid,
                                        const WCHAR* const  wszAdditional = NULL ) = 0;

        virtual const void* const PvTraceContext() = 0;
};


class CDefaultFileSystemConfiguration : public IFileSystemConfiguration
{
    public:

        CDefaultFileSystemConfiguration();

        ULONG DtickAccessDeniedRetryPeriod() override;
        ULONG DtickHungIOThreshhold() override;
        DWORD GrbitHungIOActions() override;
        ULONG CbMaxReadSize() override;
        ULONG CbMaxWriteSize() override;
        ULONG CbMaxReadGapSize() override;
        ULONG PermillageSmoothIo() override;
        BOOL FBlockCacheEnabled() override;
        ERR ErrGetBlockCacheConfiguration( _Out_ IBlockCacheConfiguration** const ppbcconfig ) override;

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

    protected:

        ULONG m_dtickAccessDeniedRetryPeriod;
        ULONG m_dtickHungIOThreshhold;
        DWORD m_grbitHungIOActions;
        ULONG m_cbMaxReadSize;
        ULONG m_cbMaxWriteSize;
        ULONG m_cbMaxReadGapSize;
        ULONG m_permillageSmoothIo;
        BOOL m_fBlockCacheEnabled;
};

#endif

