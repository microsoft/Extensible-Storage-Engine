// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Block Cache Lock Ranks

const INT rankThrottleContexts = 0;
const INT rankThrottleContext = 0;
const INT rankFileFilterReferences = 0;
const INT rankCachedFileHash = 0;
const INT rankCacheThreadLocalStorage = 0;
const INT rankClusterReferenceHash = 0;
const INT rankClusterWrites = 0;
const INT rankSlabWrites = 0;
const INT rankSlabWriteBackHash = 0;
const INT rankSlabHash = 0;
const INT rankCachedBlockWriteCounts = 0;
const INT rankCacheRepository = 0;
const INT rankFileFilter = 0;
const INT rankFileIdentification = 0;
const INT rankIOCompleteHash = 0;
const INT rankJournalSegment = 0;
const INT rankIORangeLock = 0;
const INT rankCachedFileSparseMap = 0;
const INT rankFilePathHash = 1;
const INT rankSuspendedThreads = 1;
const INT rankSlabsToWriteBack = 1;
const INT rankIssued = 1;
const INT rankJournalAppend = 5;

//  IO offset range.

class COffsets
{
    public:

        COffsets( _In_ const QWORD ibStart, _In_ const QWORD ibEnd )
            :   m_ibStart( ibStart ),
                m_ibEnd( ibEnd )
        {
        }

        COffsets() : COffsets( 0, 0 ) {}

        COffsets& operator=( _In_ const COffsets& other )
        {
            m_ibStart = other.m_ibStart;
            m_ibEnd = other.m_ibEnd;
            return *this;
        }

        QWORD IbStart() const { return m_ibStart; }
        QWORD IbEnd() const { return m_ibEnd; }
        QWORD Cb() const { return min( qwMax - 1, m_ibEnd - m_ibStart ) + 1; }

        BOOL FOverlaps( _In_ const COffsets& other ) const
        {
            return IbEnd() >= other.IbStart() && IbStart() <= other.IbEnd();
        }

        BOOL FContains( _In_ const COffsets& other ) const
        {
            return IbStart() <= other.IbStart() && other.IbEnd() <= IbEnd();
        }

    private:

        QWORD  m_ibStart;
        QWORD  m_ibEnd;
};


//  Buffer of the same size as another type.

template< class T >
class Buffer
{
    private:

        BYTE    m_rgb[sizeof( T )];
};


//  Error handling.

INLINE void BlockCacheNotableEvent( _In_opt_    const WCHAR* const  wszCachingFilePath,
                                    _In_        const char* const   szTag )
{
    WCHAR           wszSource[ 512 ]    = { 0 };
    WCHAR           wszVersion[ 512 ]   = { 0 };
    WCHAR           wszLocation[ 512 ]  = { 0 };
    WCHAR           wszTag[ 512 ]       = { 0 };
    const WCHAR*    rgpwsz[]            = { wszSource,
                                            wszVersion,
                                            L"",
                                            wszLocation,
                                            L"",
                                            wszTag };

    OSStrCbFormatW( wszSource,
                    sizeof( wszSource ),
                    L"%ws %ws",
                    WszUtilProcessFriendlyName(),
                    wszCachingFilePath ? wszCachingFilePath : L"" );
    OSStrCbFormatW( wszVersion, 
                    sizeof( wszVersion ),
                    L",,,%u.%u.%u.%u",
                    DwUtilImageVersionMajor(), 
                    DwUtilImageVersionMinor(), 
                    DwUtilImageBuildNumberMajor(),
                    DwUtilImageBuildNumberMinor() );
    OSStrCbFormatW( wszLocation,
                    sizeof( wszLocation ), 
                    L"%hs(%i)",
                    strrchr( __FILE__, '\\' ) + 1,
                    __LINE__ );
    OSStrCbFormatW( wszTag, sizeof( wszTag ), L"ASSERTTRACKTAG:EBC_%hs", szTag );

    OSEventReportEvent( WszUtilImageVersionName(),
                        eventfacilityOsDiagTracking | eventfacilityRingBufferCache | eventfacilityOsEventTrace | eventfacilityOsTrace | eventfacilityReportOsEvent,
                        eventWarning,
                        GENERAL_CATEGORY,
                        INTERNAL_TRACE_ID,
                        _countof( rgpwsz ),
                        rgpwsz );
}

INLINE void BlockCacheNotableEvent( _In_ IFileFilter* const pffCaching,
                                    _In_ const char* const  szTag )
{
    WCHAR   wszCachingFilePath[ IFileSystemAPI::cchPathMax ]    = { 0 };

    if ( pffCaching )
    {
        CallS( pffCaching->ErrPath( wszCachingFilePath ) );
    }

    BlockCacheNotableEvent( wszCachingFilePath, szTag );
}

INLINE ERR ErrBlockCacheInternalError(  _In_ const WCHAR* const wszCachingFilePath,
                                        _In_ const char* const  szTag )
{
    BlockCacheNotableEvent( wszCachingFilePath, szTag );
    return ErrERRCheck( JET_errInternalError );
}

INLINE ERR ErrBlockCacheInternalError(  _In_ ICacheConfiguration* const pcconfig,
                                        _In_ const char* const          szTag )
{
    WCHAR   wszCachingFilePath[ IFileSystemAPI::cchPathMax ]    = { 0 };

    if ( pcconfig )
    {
        pcconfig->Path( wszCachingFilePath );
    }

    return ErrBlockCacheInternalError( wszCachingFilePath, szTag );
}

INLINE ERR ErrBlockCacheInternalError(  _In_ ICachedFileConfiguration* const    pcfconfig,
                                        _In_ const char* const                  szTag )
{
    WCHAR   wszCachingFilePath[ IFileSystemAPI::cchPathMax ]    = { 0 };

    if ( pcfconfig )
    {
        pcfconfig->CachingFilePath( wszCachingFilePath );
    }

    return ErrBlockCacheInternalError( wszCachingFilePath, szTag );
}

INLINE ERR ErrBlockCacheInternalError(  _In_ IFileFilter* const pffCaching,
                                        _In_ const char* const  szTag )
{
    WCHAR   wszCachingFilePath[ IFileSystemAPI::cchPathMax ]    = { 0 };

    if ( pffCaching )
    {
        CallS( pffCaching->ErrPath( wszCachingFilePath ) );
    }

    return ErrBlockCacheInternalError( wszCachingFilePath, szTag );
}

INLINE BOOL FVerificationError( _In_ const ERR err )
{
    switch ( err )
    {
        case JET_errSuccess:
        case JET_errReadVerifyFailure:
        case JET_errPageNotInitialized:
        case JET_errDiskReadVerificationFailure:
        case JET_errReadLostFlushVerifyFailure:
            return fTrue;

        default:
            return fFalse;
    }
}

INLINE ERR ErrIgnoreVerificationErrors( _In_ const ERR err )
{
    return FVerificationError( err ) ? JET_errSuccess : err;
}

INLINE ERR ErrAccumulateError( _In_ const ERR errExisting, _In_ const ERR errNew )
{
    return errExisting < JET_errSuccess ? errExisting : ( errNew < JET_errSuccess ? errNew : JET_errSuccess );
}


//  Error translation.

template< class T >
INLINE ERR ErrToErr( _In_ const typename T::ERR err )
{
    switch ( err )
    {
        case T::ERR::errSuccess:
            return JET_errSuccess;
        case T::ERR::errOutOfMemory:
            return ErrERRCheck( JET_errOutOfMemory );
        default:
            Assert( fFalse );
            return ErrERRCheck( JET_errInternalError );
    }
}

template<>
INLINE ERR ErrToErr<IBitmapAPI>( _In_ const typename IBitmapAPI::ERR err )
{
    switch ( err )
    {
        case IBitmapAPI::ERR::errSuccess:
            return JET_errSuccess;
        case IBitmapAPI::ERR::errInvalidParameter:
            return ErrERRCheck( JET_errInvalidParameter );
        case IBitmapAPI::ERR::errOutOfMemory:
            return ErrERRCheck( JET_errOutOfMemory );
        default:
            Assert( fFalse );
            return ErrERRCheck( JET_errInternalError );
    }
}

//  Trace context scope

class CTraceContextScope
{
    public:

        CTraceContextScope( _In_ const FullTraceContext& ftc )
            :   m_petcCurr( const_cast<TraceContext*>( PetcTLSGetEngineContext() ) ),
                m_etcSaved( *m_petcCurr ),
                m_putcSaved( TLSSetUserTraceContext( &ftc.utc ) )
        {
            *m_petcCurr = ftc.etc;
        }

        ~CTraceContextScope()
        {
            *m_petcCurr = m_etcSaved;
            TLSSetUserTraceContext( m_putcSaved );
        }

    private:

        CTraceContextScope( _In_ const CTraceContextScope& tc ) = delete;
        const CTraceContextScope& operator=( _In_ const CTraceContextScope& tc ) = delete;

    private:

        TraceContext* const             m_petcCurr;
        const TraceContext              m_etcSaved;
        const UserTraceContext* const   m_putcSaved;
};


//  OSFormat


INLINE const char* OSFormat(    _In_ const VolumeId     volumeid,
                                _In_ const FileId       fileid )
{
    return OSFormat( "0x%08x-0x%016I64x", volumeid, fileid );
}

INLINE const char* OSFormat(    _In_ const VolumeId     volumeid,
                                _In_ const FileId       fileid,
                                _In_ const FileSerial   fileserial )
{
    return OSFormat( "%s-0x%08x", OSFormat( volumeid, fileid ), fileserial );
}

INLINE const char* OSFormatFileId( _In_ IFileFilter* const pff )
{
    VolumeId    volumeid    = volumeidInvalid;
    FileId      fileid      = fileidInvalid;
    FileSerial  fileserial  = fileserialInvalid;

    CallS( pff->ErrGetPhysicalId( &volumeid, &fileid, &fileserial ) );

    return OSFormat( volumeid, fileid, fileserial );
}

INLINE const char* OSFormat( _In_ IFileFilter* const pff )
{
    WCHAR   wszAbsPath[ IFileSystemAPI::cchPathMax ]    = { 0 };

    CallS( pff->ErrPath( wszAbsPath ) );

    return OSFormat( "%ws (%s)", wszAbsPath, OSFormatFileId( pff ) );
}

INLINE const char* OSFormatFileId( _In_ ICache* const pc )
{
    VolumeId    volumeid                        = volumeidInvalid;
    FileId      fileid                          = fileidInvalid;
    BYTE        rgbUniqueId[ sizeof( GUID ) ]   = { 0 };

    CallS( pc->ErrGetPhysicalId( &volumeid, &fileid, rgbUniqueId ) );

    return OSFormat( volumeid, fileid );
}
