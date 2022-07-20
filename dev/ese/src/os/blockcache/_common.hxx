// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Block Cache Lock Ranks

const INT rankPool = 0;
const INT rankPresenceFilterSlabs = 0;
const INT rankPresenceFilter = 0;
const INT rankDestagingFiles = 0;
const INT rankThrottleContexts = 0;
const INT rankThrottleContext = 0;
const INT rankFileFilterReferences = 0;
const INT rankCachedFileHash = 0;
const INT rankCacheThreadLocalStorage = 0;
const INT rankClusterReferenceHash = 0;
const INT rankClusterWrites = 0;
const INT rankSlabWrites = 0;
const INT rankSlabWriteBackHash = 0;
const INT rankSlabHash = 1;
const INT rankCachedBlockWriteCounts = 0;
const INT rankCacheRepository = 0;
const INT rankRegisterIFilePerfAPI = 0;
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

        BOOL FAdjacent( _In_ const COffsets& other ) const
        {
            return IbEnd() + 1 == other.IbStart() || other.IbEnd() + 1 == IbStart();
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


//  Pool of objects with a minimum lifetime

template< class T, BOOL fHeap = fTrue, TICK dtickMin = 10 * 1000 >
class TPool
{
    public:

        static void* PvAllocate( _In_ const BOOL fZero = !fHeap )
        {
            return PvAllocate( sizeof( T ), fZero );
        }

        static void* PvAllocate( _In_ const size_t cb, _In_ const BOOL fZero = !fHeap )
        {
            void* pv = NULL;

            if ( s_state.m_il.PrevMost() )
            {
                s_state.m_crit.Enter();

                CHeader* pheader = s_state.m_il.PrevMost();

                pheader = pheader && pheader->Cb() >= cb ? pheader : NULL;

                if ( pheader )
                {
                    s_state.m_il.Remove( pheader );
                }

                s_state.m_crit.Leave();

                if ( pheader )
                {
                    pheader->~CHeader();

                    pv = pheader;

                    if ( fZero )
                    {
                        memset( pv, 0, cb );
                    }
                }
            }

            if ( !pv )
            {
                pv = PvAllocate_( cb );
            }

            return pv;
        }

        static void Free( _Inout_ void** const ppv )
        {
            Free( sizeof( T ), ppv );
        }

        static void Free( _In_ const size_t cb, _Inout_ void** const ppv )
        {
            CInvasiveList<CHeader, CHeader::OffsetOfILE>    il;
            void*                                           pv      = ppv ? *ppv : NULL;
            CHeader*                                        pheader =   NULL;

            if ( ppv )
            {
                *ppv = NULL;
            }

            if ( pv && cb >= sizeof( CHeader ) )
            {
                pheader = new( pv ) CHeader( cb );
                pv = NULL;
            }

            s_state.m_crit.Enter();

            if ( pheader )
            {
                s_state.m_il.InsertAsPrevMost( pheader );
                pheader = NULL;
            }

            while ( s_state.m_il.NextMost() && s_state.m_il.NextMost()->FRelease() )
            {
                pheader = s_state.m_il.NextMost();
                s_state.m_il.Remove( pheader );
                il.InsertAsNextMost( pheader );
                pheader = NULL;
            }

            s_state.m_crit.Leave();

            s_state.Release( il );

            Free_( pv );
        }

    private:

        static void* PvAllocate_( _In_ const size_t cb )
        {
            return fHeap ?
                PvOSMemoryHeapAlloc( cb ) :
                PvOSMemoryPageAlloc( roundup( cb, OSMemoryPageCommitGranularity() ), NULL );
        }

        static void Free_( _In_ void* const pv )
        {
            fHeap ? OSMemoryHeapFree( pv ) : OSMemoryPageFree( pv );
        }

        static TICK TickNow_()
        {
            return TickOSTimeCurrent();
        }

        static TICK DtickElapsed_( _In_ const TICK tickStart )
        {
            return DtickDelta( tickStart, TickNow_() );
        }

    private:

        class CHeader
        {
            public:

                CHeader( _In_ const size_t cb )
                    :   m_cb( cb ),
                        m_tickRelease( TickNow_() )
                {
                }

                size_t Cb() const { return m_cb; }

                BOOL FRelease() const
                {
                    return DtickElapsed_( m_tickRelease ) >= dtickMin;
                }

                static SIZE_T OffsetOfILE() { return OffsetOf( CHeader, m_ile ); }

            private:

                typename CInvasiveList<CHeader, CHeader::OffsetOfILE>::CElement m_ile;
                const size_t                                                    m_cb;
                const TICK                                                      m_tickRelease;
        };

    private:

        class CState
        {
            public:

                CState()
                    :   m_crit( CLockBasicInfo( CSyncBasicInfo( "TPool<T, fHeap, dtickMin>::CState::m_crit" ), rankPool, 0 ) )
                {
                }

                ~CState()
                {
                    Release( m_il );
                }

                static void Release( CInvasiveList<CHeader, CHeader::OffsetOfILE>& il )
                {
                    while ( CHeader* const pheader = il.PrevMost() )
                    {
                        il.Remove( pheader );
                        pheader->~CHeader();

                        Free_( pheader );
                    }
                }

                CCriticalSection                                                m_crit;
                typename CCountedInvasiveList<CHeader, CHeader::OffsetOfILE>    m_il;
        };

    private:

        static CState   s_state;
};

template< class T, BOOL fHeap, TICK dtickMin >
typename TPool<T, fHeap, dtickMin>::CState TPool<T, fHeap, dtickMin>::s_state;
