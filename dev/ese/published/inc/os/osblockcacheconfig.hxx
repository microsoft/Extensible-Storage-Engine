// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSBLOCKCACHECONFIG_HXX_INCLUDED
#define _OSBLOCKCACHECONFIG_HXX_INCLUDED



class ICachedFileConfiguration
{
    public:

        virtual ~ICachedFileConfiguration() {}


        virtual BOOL FCachingEnabled() = 0;


        virtual VOID CachingFilePath( __out_bcount( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath ) = 0;


        virtual ULONG CbBlockSize() = 0;


        virtual ULONG CConcurrentBlockWriteBackMax() = 0;


        virtual ULONG LCacheTelemetryFileNumber() = 0;
};


class CDefaultCachedFileConfiguration : public ICachedFileConfiguration
{
    public:

        CDefaultCachedFileConfiguration();

    public:

        BOOL FCachingEnabled() override;
        VOID CachingFilePath( __out_bcount( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath ) override;
        ULONG CbBlockSize() override;
        ULONG CConcurrentBlockWriteBackMax() override;
        ULONG LCacheTelemetryFileNumber() override;

    protected:

        BOOL    m_fCachingEnabled;
        WCHAR   m_wszAbsPathCachingFile[ OSFSAPI_MAX_PATH ];
        ULONG   m_cbBlockSize;
        ULONG   m_cConcurrentBlockWriteBackMax;
        ULONG   m_lCacheTelemetryFileNumber;
};


class ICacheConfiguration
{
    public:

        enum { cbGuid = sizeof(GUID) };

    public:

        virtual ~ICacheConfiguration() {}


        virtual BOOL FCacheEnabled() = 0;


        virtual VOID CacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType ) = 0;


        virtual VOID Path( __out_bcount( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath ) = 0;


        virtual QWORD CbMaximumSize() = 0;


        virtual double PctWrite() = 0;
};


class CDefaultCacheConfiguration : public ICacheConfiguration
{
    public:

        CDefaultCacheConfiguration();

    public:

        BOOL FCacheEnabled() override;
        VOID CacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType ) override;
        VOID Path( __out_bcount( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath ) override;
        QWORD CbMaximumSize() override;
        double PctWrite() override;

    protected:

        BOOL    m_fCacheEnabled;
        BYTE    m_rgbCacheType[ cbGuid ];
        WCHAR   m_wszAbsPathCachingFile[ OSFSAPI_MAX_PATH ];
        QWORD   m_cbMaximumSize;
        double  m_pctWrite;
};

class IBlockCacheConfiguration
{
    public:

        virtual ~IBlockCacheConfiguration() {}


        virtual ERR ErrGetCachedFileConfiguration(  _In_z_  const WCHAR* const                  wszKeyPathCachedFile, 
                                                    _Out_   ICachedFileConfiguration** const    ppcfconfig  ) = 0;


        virtual ERR ErrGetCacheConfiguration(   _In_z_  const WCHAR* const          wszKeyPathCachingFile,
                                                _Out_   ICacheConfiguration** const ppcconfig ) = 0;
};

class CDefaultBlockCacheConfiguration : public IBlockCacheConfiguration
{
    public:

        CDefaultBlockCacheConfiguration();

    public:

        ERR ErrGetCachedFileConfiguration(  _In_z_  const WCHAR* const                  wszKeyPathCachedFile, 
                                            _Out_   ICachedFileConfiguration** const    ppcfconfig  ) override;
        ERR ErrGetCacheConfiguration(   _In_z_  const WCHAR* const          wszKeyPathCachingFile,
                                        _Out_   ICacheConfiguration** const ppcconfig ) override;
};

#endif

