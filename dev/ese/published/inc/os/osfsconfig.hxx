// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSFSCONFIG_HXX_INCLUDED
#define _OSFSCONFIG_HXX_INCLUDED

#include "osblockcacheconfig.hxx"

//  File System Configuration

class IFileSystemConfiguration  //  fsconfig
{
    //  configuration

    public:

        virtual ~IFileSystemConfiguration() {}

        //virtual ULONG CbZeroExtend() = 0;

        //  Access denied retry period.

        virtual ULONG DtickAccessDeniedRetryPeriod() = 0;

        //virtual ULONG CIOMaxOutstanding() = 0;
        //virtual ULONG CIOMaxOutstandingBackground() = 0;

        //  Hung IO threshold.

        virtual ULONG DtickHungIOThreshhold() = 0;

        //  Hung IO actions.

        virtual DWORD GrbitHungIOActions() = 0;

        //  The maximum read size.

        virtual ULONG CbMaxReadSize() = 0;

        //  The maximum write size.

        virtual ULONG CbMaxWriteSize() = 0;

        //  The maximum read gap size.

        virtual ULONG CbMaxReadGapSize() = 0;

        //  Permillage of IO forced to be smooth IO.

        virtual ULONG PermillageSmoothIo() = 0;

        //  Indicates if the block cache should be enabled.

        virtual BOOL FBlockCacheEnabled() = 0;

        //  Gets the Block Cache Configuration.

        virtual ERR ErrGetBlockCacheConfiguration( _Out_ IBlockCacheConfiguration** const ppbcconfig ) = 0;

    //  error reporting

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

//  Default File System Configuration.

class CDefaultFileSystemConfiguration : public IFileSystemConfiguration
{
    public:

        CDefaultFileSystemConfiguration();

        //ULONG CbZeroExtend() override;
        ULONG DtickAccessDeniedRetryPeriod() override;
        //ULONG CIOMaxOutstanding() override;
        //ULONG CIOMaxOutstandingBackground() override;
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

        //ULONG m_cbZeroExtend;
        ULONG m_dtickAccessDeniedRetryPeriod;
        //ULONG m_cIOMaxOutstanding;
        //ULONG m_cIOMaxOutstandingBackground;
        ULONG m_dtickHungIOThreshhold;
        DWORD m_grbitHungIOActions;
        ULONG m_cbMaxReadSize;
        ULONG m_cbMaxWriteSize;
        ULONG m_cbMaxReadGapSize;
        ULONG m_permillageSmoothIo;
        BOOL m_fBlockCacheEnabled;
};

#endif  //  _OSFSCONFIG_HXX_INCLUDED

