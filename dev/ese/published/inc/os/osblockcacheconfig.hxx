// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSBLOCKCACHECONFIG_HXX_INCLUDED
#define _OSBLOCKCACHECONFIG_HXX_INCLUDED

//  Block Cache Configuration

//  Cached File Configuration Interface.

class ICachedFileConfiguration  //  cfconfig
{
    public:

        virtual ~ICachedFileConfiguration() {}

        //  Indicates if the file should be cached.

        virtual BOOL FCachingEnabled() = 0;

        //  Indicates the path to the caching file that should cache the contents of this file.

        virtual VOID CachingFilePath( __out_bcount( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath ) = 0;

        //  The unit of caching for the file.

        virtual ULONG CbBlockSize() = 0;

        //  The maximum number of concurrent block write backs that can occur for the file.

        virtual ULONG CConcurrentBlockWriteBackMax() = 0;

        //  The file number to use for cache telemetry.

        virtual ULONG LCacheTelemetryFileNumber() = 0;
};

//  Default Cached File Configuration.

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

//  Cache Configuration Interface.

class ICacheConfiguration  //  cconfig
{
    public:

        enum { cbGuid = sizeof(GUID) };

    public:

        virtual ~ICacheConfiguration() {}

        //  Indicates if this cache is enabled.

        virtual BOOL FCacheEnabled() = 0;

        //  Indicates the desired cache type.

        virtual VOID CacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType ) = 0;

        //  The path of this caching file.

        virtual VOID Path( __out_bcount( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath ) = 0;

        //  The maximum size of this caching file.

        virtual QWORD CbMaximumSize() = 0;

        //  The amount of this caching file dedicated to write back caching.

        virtual double PctWrite() = 0;
};

//  Default Cache Configuration.

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

class IBlockCacheConfiguration  //  bcconfig
{
    public:

        virtual ~IBlockCacheConfiguration() {}

        //  Gets the Cached File Configuration for a file at the provided key path.

        virtual ERR ErrGetCachedFileConfiguration(  _In_z_  const WCHAR* const                  wszKeyPathCachedFile, 
                                                    _Out_   ICachedFileConfiguration** const    ppcfconfig  ) = 0;

        //  Gets the Cache Configuration for a caching file at the provided key path.

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

#endif  //  _OSBLOCKCACHECONFIG_HXX_INCLUDED

