// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "xpress.h"
#ifdef XPRESS9_COMPRESSION
#include "xpress9.h"
#endif
#ifdef XPRESS10_COMPRESSION
#include "xpress10sw.h"
#include "xpress10corsica.h"
#endif

#include "PageSizeClean.hxx"

class IDataCompressorStats
{
    public:
        ~IDataCompressorStats() {}

        virtual void AddUncompressedBytes( const INT cb ) = 0;
        virtual void AddCompressedBytes( const INT cb ) = 0;
        virtual void IncCompressionCalls() = 0;
        virtual void AddCompressionDhrts( const QWORD dhrts ) = 0;
        virtual void AddDecompressionBytes( const INT cb ) = 0;
        virtual void IncDecompressionCalls() = 0;
        virtual void AddDecompressionDhrts( const QWORD dhrts ) = 0;

        virtual void AddCpuXpress9DecompressionBytes( const INT cb ) = 0;
        virtual void IncCpuXpress9DecompressionCalls() = 0;
        virtual void AddCpuXpress9DecompressionDhrts( const QWORD dhrts ) = 0;

        virtual void AddXpress10SoftwareDecompressionBytes( const INT cb ) = 0;
        virtual void IncXpress10SoftwareDecompressionCalls() = 0;
        virtual void AddXpress10SoftwareDecompressionDhrts( const QWORD dhrts ) = 0;

        virtual void AddXpress10CorsicaCompressionBytes( const INT cb ) = 0;
        virtual void AddXpress10CorsicaCompressedBytes( const INT cb ) = 0;
        virtual void IncXpress10CorsicaCompressionCalls() = 0;
        virtual void AddXpress10CorsicaCompressionDhrts( const QWORD dhrts ) = 0;
        virtual void AddXpress10CorsicaCompressionHardwareDhrts( const QWORD dhrts ) = 0;
        virtual void AddXpress10CorsicaDecompressionBytes( const INT cb ) = 0;
        virtual void IncXpress10CorsicaDecompressionCalls() = 0;
        virtual void AddXpress10CorsicaDecompressionDhrts( const QWORD dhrts ) = 0;
        virtual void AddXpress10CorsicaDecompressionHardwareDhrts( const QWORD dhrts ) = 0;

    protected:
        IDataCompressorStats() {}
};

#define CHECK_INST( x ) if ( m_iInstance != 0 ) { x; }

class CDataCompressorPerfCounters : public IDataCompressorStats
{
    public:
        CDataCompressorPerfCounters( const INT iInstance );
        ~CDataCompressorPerfCounters();

        void AddUncompressedBytes( const INT cb );
        void AddCompressedBytes( const INT cb );
        void IncCompressionCalls();
        void AddCompressionDhrts( const QWORD dhrts );
        void AddDecompressionBytes( const INT cb );
        void IncDecompressionCalls();
        void AddDecompressionDhrts( const QWORD dhrts );

        void AddCpuXpress9DecompressionBytes( const INT cb )        { PERFOpt( CHECK_INST( s_cbCpuXpress9DecompressionBytes.Add( m_iInstance, cb ) ) ); }
        void IncCpuXpress9DecompressionCalls()                      { PERFOpt( CHECK_INST( s_cCpuXpress9DecompressionCalls.Inc( m_iInstance ) ) ); }
        void AddCpuXpress9DecompressionDhrts( const QWORD dhrts )   { PERFOpt( CHECK_INST( s_cCpuXpress9DecompressionTotalDhrts.Add( m_iInstance, dhrts ) ) ); }

        void AddXpress10SoftwareDecompressionBytes( const INT cb )        { PERFOpt( CHECK_INST( s_cbXpress10SoftwareDecompressionBytes.Add( m_iInstance, cb ) ) ); }
        void IncXpress10SoftwareDecompressionCalls()                      { PERFOpt( CHECK_INST( s_cXpress10SoftwareDecompressionCalls.Inc( m_iInstance ) ) ); }
        void AddXpress10SoftwareDecompressionDhrts( const QWORD dhrts )   { PERFOpt( CHECK_INST( s_cXpress10SoftwareDecompressionTotalDhrts.Add( m_iInstance, dhrts ) ) ); }

        void AddXpress10CorsicaCompressionBytes( const INT cb )         { PERFOpt( CHECK_INST( s_cbXpress10CorsicaCompressionBytes.Add( m_iInstance, cb ) ) ); }
        void AddXpress10CorsicaCompressedBytes( const INT cb )           { PERFOpt( CHECK_INST( s_cbXpress10CorsicaCompressedBytes.Add( m_iInstance, cb ) ) ); }
        void IncXpress10CorsicaCompressionCalls()                        { PERFOpt( CHECK_INST( s_cXpress10CorsicaCompressionCalls.Inc( m_iInstance ) ) ); }
        void AddXpress10CorsicaCompressionDhrts( const QWORD dhrts )     { PERFOpt( CHECK_INST( s_cXpress10CorsicaCompressionTotalDhrts.Add( m_iInstance, dhrts ) ) ); }
        void AddXpress10CorsicaCompressionHardwareDhrts( const QWORD dhrts )     { PERFOpt( CHECK_INST( s_cXpress10CorsicaCompressionHardwareTotalDhrts.Add( m_iInstance, dhrts ) ) ); }
        void AddXpress10CorsicaDecompressionBytes( const INT cb )        { PERFOpt( CHECK_INST( s_cbXpress10CorsicaDecompressionBytes.Add( m_iInstance, cb ) ) ); }
        void IncXpress10CorsicaDecompressionCalls()                      { PERFOpt( CHECK_INST( s_cXpress10CorsicaDecompressionCalls.Inc( m_iInstance ) ) ); }
        void AddXpress10CorsicaDecompressionDhrts( const QWORD dhrts )   { PERFOpt( CHECK_INST( s_cXpress10CorsicaDecompressionTotalDhrts.Add( m_iInstance, dhrts ) ) ); }
        void AddXpress10CorsicaDecompressionHardwareDhrts( const QWORD dhrts )   { PERFOpt( CHECK_INST( s_cXpress10CorsicaDecompressionHardwareTotalDhrts.Add( m_iInstance, dhrts ) ) ); }

    private:
        const INT m_iInstance;

    public:
        static PERFInstanceLiveTotal<> s_cbUncompressedBytes;
        static PERFInstanceLiveTotal<> s_cbCompressedBytes;
        static PERFInstanceLiveTotal<> s_cCompressionCalls;
        static PERFInstanceLiveTotal<QWORD> s_cCompressionTotalDhrts;
        static PERFInstanceLiveTotal<> s_cbDecompressionBytes;
        static PERFInstanceLiveTotal<> s_cDecompressionCalls;
        static PERFInstanceLiveTotal<QWORD> s_cDecompressionTotalDhrts;

        static PERFInstanceLiveTotal<> s_cbCpuXpress9DecompressionBytes;
        static PERFInstanceLiveTotal<> s_cCpuXpress9DecompressionCalls;
        static PERFInstanceLiveTotal<QWORD> s_cCpuXpress9DecompressionTotalDhrts;

        static PERFInstanceLiveTotal<> s_cbXpress10SoftwareDecompressionBytes;
        static PERFInstanceLiveTotal<> s_cXpress10SoftwareDecompressionCalls;
        static PERFInstanceLiveTotal<QWORD> s_cXpress10SoftwareDecompressionTotalDhrts;

        static PERFInstanceLiveTotal<> s_cbXpress10CorsicaCompressionBytes;
        static PERFInstanceLiveTotal<> s_cbXpress10CorsicaCompressedBytes;
        static PERFInstanceLiveTotal<> s_cXpress10CorsicaCompressionCalls;
        static PERFInstanceLiveTotal<QWORD> s_cXpress10CorsicaCompressionTotalDhrts;
        static PERFInstanceLiveTotal<QWORD> s_cXpress10CorsicaCompressionHardwareTotalDhrts;
        static PERFInstanceLiveTotal<> s_cbXpress10CorsicaDecompressionBytes;
        static PERFInstanceLiveTotal<> s_cXpress10CorsicaDecompressionCalls;
        static PERFInstanceLiveTotal<QWORD> s_cXpress10CorsicaDecompressionTotalDhrts;
        static PERFInstanceLiveTotal<QWORD> s_cXpress10CorsicaDecompressionHardwareTotalDhrts;
};

PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cbUncompressedBytes;
PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cbCompressedBytes;
PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cCompressionCalls;
PERFInstanceLiveTotal<QWORD> CDataCompressorPerfCounters::s_cCompressionTotalDhrts;
PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cbDecompressionBytes;
PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cDecompressionCalls;
PERFInstanceLiveTotal<QWORD> CDataCompressorPerfCounters::s_cDecompressionTotalDhrts;

PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cbCpuXpress9DecompressionBytes;
PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cCpuXpress9DecompressionCalls;
PERFInstanceLiveTotal<QWORD> CDataCompressorPerfCounters::s_cCpuXpress9DecompressionTotalDhrts;

PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cbXpress10SoftwareDecompressionBytes;
PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cXpress10SoftwareDecompressionCalls;
PERFInstanceLiveTotal<QWORD> CDataCompressorPerfCounters::s_cXpress10SoftwareDecompressionTotalDhrts;

PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cbXpress10CorsicaCompressionBytes;
PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cbXpress10CorsicaCompressedBytes;
PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cXpress10CorsicaCompressionCalls;
PERFInstanceLiveTotal<QWORD> CDataCompressorPerfCounters::s_cXpress10CorsicaCompressionTotalDhrts;
PERFInstanceLiveTotal<QWORD> CDataCompressorPerfCounters::s_cXpress10CorsicaCompressionHardwareTotalDhrts;
PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cbXpress10CorsicaDecompressionBytes;
PERFInstanceLiveTotal<> CDataCompressorPerfCounters::s_cXpress10CorsicaDecompressionCalls;
PERFInstanceLiveTotal<QWORD> CDataCompressorPerfCounters::s_cXpress10CorsicaDecompressionTotalDhrts;
PERFInstanceLiveTotal<QWORD> CDataCompressorPerfCounters::s_cXpress10CorsicaDecompressionHardwareTotalDhrts;

CDataCompressorPerfCounters::CDataCompressorPerfCounters( const INT iInstance ) :
    IDataCompressorStats(),
    m_iInstance( iInstance )
{
}

CDataCompressorPerfCounters::~CDataCompressorPerfCounters()
{
}

void CDataCompressorPerfCounters::AddUncompressedBytes( const INT cb )
{
    PERFOpt( CHECK_INST( s_cbUncompressedBytes.Add( m_iInstance, cb ) ) );
}

void CDataCompressorPerfCounters::AddCompressedBytes( const INT cb )
{
    PERFOpt( CHECK_INST( s_cbCompressedBytes.Add( m_iInstance, cb ) ) );
}

void CDataCompressorPerfCounters::IncCompressionCalls()
{
    PERFOpt( CHECK_INST( s_cCompressionCalls.Inc( m_iInstance ) ) );
}

void CDataCompressorPerfCounters::AddCompressionDhrts( const QWORD dhrts )
{
    PERFOpt( CHECK_INST( s_cCompressionTotalDhrts.Add( m_iInstance, dhrts ) ) );
}

void CDataCompressorPerfCounters::AddDecompressionBytes( const INT cb )
{
    PERFOpt( CHECK_INST( s_cbDecompressionBytes.Add( m_iInstance, cb ) ) );
}

void CDataCompressorPerfCounters::IncDecompressionCalls()
{
    PERFOpt( CHECK_INST( s_cDecompressionCalls.Inc( m_iInstance ) ) );
}

void CDataCompressorPerfCounters::AddDecompressionDhrts( const QWORD dhrts )
{
    PERFOpt( CHECK_INST( s_cDecompressionTotalDhrts.Add( m_iInstance, dhrts ) ) );
}

#undef CHECK_INST

#ifdef PERFMON_SUPPORT

LONG LUncompressedBytesCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cbUncompressedBytes.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LCompressedBytesCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cbCompressedBytes.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LCompressionCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cCompressionCalls.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LCompressionLatencyCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*) pvBuf = CusecHRTFromDhrt( CDataCompressorPerfCounters::s_cCompressionTotalDhrts.Get( iInstance ) );
    }
    return 0;
}

LONG LDecompressionBytesCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cbDecompressionBytes.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDecompressionCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cDecompressionCalls.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDecompressionLatencyCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*) pvBuf = CusecHRTFromDhrt( CDataCompressorPerfCounters::s_cDecompressionTotalDhrts.Get( iInstance ) );
    }
    return 0;
}

LONG LCpuXpress9DecompressionBytesCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cbCpuXpress9DecompressionBytes.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LCpuXpress9DecompressionCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cCpuXpress9DecompressionCalls.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LCpuXpress9DecompressionLatencyCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*) pvBuf = CusecHRTFromDhrt( CDataCompressorPerfCounters::s_cCpuXpress9DecompressionTotalDhrts.Get( iInstance ) );
    }
    return 0;
}

LONG LXpress10SoftwareDecompressionBytesCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cbXpress10SoftwareDecompressionBytes.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LXpress10SoftwareDecompressionCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cXpress10SoftwareDecompressionCalls.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LXpress10SoftwareDecompressionLatencyCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*) pvBuf = CusecHRTFromDhrt( CDataCompressorPerfCounters::s_cXpress10SoftwareDecompressionTotalDhrts.Get( iInstance ) );
    }
    return 0;
}

LONG LXpress10CorsicaCompressionBytesCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cbXpress10CorsicaCompressionBytes.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LXpress10CorsicaCompressedBytesCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cbXpress10CorsicaCompressedBytes.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LXpress10CorsicaCompressionCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cXpress10CorsicaCompressionCalls.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LXpress10CorsicaCompressionLatencyCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*) pvBuf = CusecHRTFromDhrt( CDataCompressorPerfCounters::s_cXpress10CorsicaCompressionTotalDhrts.Get( iInstance ) );
    }
    return 0;
}

LONG LXpress10CorsicaCompressionHardwareLatencyCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*) pvBuf = CusecHRTFromDhrt( CDataCompressorPerfCounters::s_cXpress10CorsicaCompressionHardwareTotalDhrts.Get( iInstance ) );
    }
    return 0;
}

LONG LXpress10CorsicaDecompressionBytesCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cbXpress10CorsicaDecompressionBytes.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LXpress10CorsicaDecompressionCEFLPv( LONG iInstance, void *pvBuf )
{
    CDataCompressorPerfCounters::s_cXpress10CorsicaDecompressionCalls.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LXpress10CorsicaDecompressionLatencyCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*) pvBuf = CusecHRTFromDhrt( CDataCompressorPerfCounters::s_cXpress10CorsicaDecompressionTotalDhrts.Get( iInstance ) );
    }
    return 0;
}

LONG LXpress10CorsicaDecompressionHardwareLatencyCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*) pvBuf = CusecHRTFromDhrt( CDataCompressorPerfCounters::s_cXpress10CorsicaDecompressionHardwareTotalDhrts.Get( iInstance ) );
    }
    return 0;
}

#endif

class CCompressionBufferCache
{
    public:
        CCompressionBufferCache();
        ~CCompressionBufferCache();

        ERR ErrInit( const INT cbBufferSize );
        void Term();

        BYTE * PbAlloc();
        void Free( BYTE * const pb );
        INT CbBufferSize() const;

    private:
        INT m_cbBufferSize;

        INT m_cpbCachedCompressionBuffers;
        BYTE** m_rgpbCachedCompressionBuffers;

    private:
        CCompressionBufferCache( const CCompressionBufferCache& );
        CCompressionBufferCache& operator=( const CCompressionBufferCache& );
};

CCompressionBufferCache::CCompressionBufferCache() :
    m_cbBufferSize( 0 ),
    m_cpbCachedCompressionBuffers( 0 ),
    m_rgpbCachedCompressionBuffers( NULL )
{
}

CCompressionBufferCache::~CCompressionBufferCache()
{
}

INT CCompressionBufferCache::CbBufferSize() const
{
    return m_cbBufferSize;
}

ERR CCompressionBufferCache::ErrInit( const INT cbBufferSize )
{
    ERR err = JET_errSuccess;
    Assert( 0 <= m_cbBufferSize );
    Assert( NULL == m_rgpbCachedCompressionBuffers );
    
    m_cbBufferSize = cbBufferSize;
    m_cpbCachedCompressionBuffers = OSSyncGetProcessorCountMax();
    Alloc( m_rgpbCachedCompressionBuffers = new BYTE*[ m_cpbCachedCompressionBuffers ]() );

HandleError:
    return err;
}

void CCompressionBufferCache::Term()
{
    for ( INT ipb = 0; ipb < m_cpbCachedCompressionBuffers; ++ipb )
    {
        delete[] m_rgpbCachedCompressionBuffers[ ipb ];
    }

    delete[] m_rgpbCachedCompressionBuffers;
    m_rgpbCachedCompressionBuffers = NULL;
    m_cbBufferSize = 0;
}

BYTE * CCompressionBufferCache::PbAlloc()
{
    BYTE * pb = GetCachedPtr<BYTE *>( m_rgpbCachedCompressionBuffers, m_cpbCachedCompressionBuffers );
    if ( NULL == pb )
    {
        pb = new BYTE[ CbBufferSize() ];
    }
    return pb;
}

void CCompressionBufferCache::Free( BYTE * const pb )
{
    if ( pb && !FCachePtr<BYTE *>( pb, m_rgpbCachedCompressionBuffers, m_cpbCachedCompressionBuffers ) )
    {
        delete[] pb;
    }
}

static CCompressionBufferCache g_compressionBufferCache;

class CDataCompressor
{
    public:
        CDataCompressor();
        ~CDataCompressor();

        ERR ErrInit( const INT cbMin, const INT cbMax );
        void Term();
        
        ERR ErrCompress(
            const DATA& data,
            const CompressFlags compressFlags,
            IDataCompressorStats * const pstats,
            _Out_writes_bytes_to_opt_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
            const INT cbDataCompressedMax,
            _Out_ INT * const pcbDataCompressedActual );
        
        ERR ErrDecompress(
            const DATA& dataCompressed,
            IDataCompressorStats * const pstats,
            _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
            const INT cbDataMax,
            _Out_ INT * const pcbDataActual );

        ERR ErrScrub(
            DATA& data,
            const CHAR chScrub );
        
    private:
        enum COMPRESSION_SCHEME
            {
                COMPRESS_NONE,
                COMPRESS_7BITASCII = 0x1,
                COMPRESS_7BITUNICODE = 0x2,
                COMPRESS_XPRESS = 0x3,
                COMPRESS_SCRUB = 0x4,
                COMPRESS_XPRESS9 = 0x5,
                COMPRESS_XPRESS10 = 0x6,
                COMPRESS_MAXIMUM = 0x1f,
            };

#include <pshpack1.h>
        PERSISTED
        struct Xpress9Header
        {
            BYTE                            m_fCompressScheme;
            UnalignedLittleEndian<ULONG>    m_ulChecksum;
        };

        PERSISTED
        struct Xpress10Header
        {
            BYTE                                m_fCompressScheme;
            UnalignedLittleEndian<WORD>         mle_cbUncompressed;
            UnalignedLittleEndian<ULONG>        mle_ulUncompressedChecksum;
            UnalignedLittleEndian<ULONGLONG>    mle_ullCompressedChecksum;
        };
#include <poppack.h>

    private:
        static const INT xpressLegacyCompressionLevel = 2;

        static const INT pctCompressionWastedEffort = 10;

        INT m_cencodeCachedMax;
        INT m_cdecodeCachedMax;

        XpressEncodeStream* m_rgencodeXpress;
        XpressDecodeStream* m_rgdecodeXpress;
#ifdef XPRESS9_COMPRESSION
        XPRESS9_ENCODER* m_rgencodeXpress9;
        XPRESS9_DECODER* m_rgdecodeXpress9;
#endif

        INT m_cbMin;

        INT m_cbMax;

        ERR ErrXpressEncodeOpen_( _Out_ XpressEncodeStream * const pencode );
        void XpressEncodeClose_( XpressEncodeStream encode );
        void XpressEncodeRelease_( XpressEncodeStream encode );
        ERR ErrXpressDecodeOpen_( _Out_ XpressDecodeStream * const pdecode );
        void XpressDecodeClose_( XpressDecodeStream decode );
        void XpressDecodeRelease_( XpressDecodeStream decode );

#ifdef XPRESS9_COMPRESSION
        ERR ErrXpress9EncodeOpen_( _Out_ XPRESS9_ENCODER * const pencode );
        void Xpress9EncodeClose_( XPRESS9_ENCODER encode );
        void Xpress9EncodeRelease_( XPRESS9_ENCODER encode );
        ERR ErrXpress9DecodeOpen_( _Out_ XPRESS9_DECODER * const pdecode );
        void Xpress9DecodeClose_( XPRESS9_DECODER decode );
        void Xpress9DecodeRelease_( XPRESS9_DECODER decode );
#endif

        static void * XPRESS_CALL PvXpressAlloc_(
            _In_opt_ void * pvContext,
            INT             cbAlloc );

        static void XPRESS_CALL XpressFree_(
            _In_opt_ void *             pvContext,
            _Post_ptr_invalid_ void *   pvAlloc );

    private:
        INT CbCompressed7BitAscii_( const INT cb );
        INT CbCompressed7BitUnicode_( const INT cb );

        COMPRESSION_SCHEME Calculate7BitCompressionScheme_( const DATA& data, BOOL fUnicodeOnly );

#ifdef XPRESS9_COMPRESSION
        ERR ErrXpress9StatusToJetErr( const XPRESS9_STATUS& status );
#endif
        
        ERR ErrCompress7BitAscii_(
            const DATA& data,
            _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
            _In_ const INT cbDataCompressedMax,
            _Out_ INT * const pcbDataCompressedActual );
        ERR ErrCompress7BitUnicode_(
            const DATA& data,
            _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
            _In_ const INT cbDataCompressedMax,
            _Out_ INT * const pcbDataCompressedActual );
        ERR ErrCompressXpress_(
            const DATA& data,
            _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
            const INT cbDataCompressedMax,
            _Out_ INT * const pcbDataCompressedActual,
            IDataCompressorStats * const pstats );
#ifdef XPRESS9_COMPRESSION
        ERR ErrCompressXpress9_(
            const DATA& data,
            _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
            const INT cbDataCompressedMax,
            _Out_ INT * const pcbDataCompressedActual );
        ERR ErrVerifyCompressXpress9_(
            _In_reads_bytes_( cbDataCompressed ) const BYTE * const pbDataCompressed,
            const INT cbDataCompressed,
            const INT cbDataCompressedMax,
            IDataCompressorStats * const pstats );
#endif
#ifdef XPRESS10_COMPRESSION
        ERR ErrCompressXpress10_(
            const DATA& data,
            _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
            const INT cbDataCompressedMax,
            _Out_ INT * const pcbDataCompressedActual,
            IDataCompressorStats * const pstats );
        ERR ErrVerifyCompressXpress10_(
            _In_reads_bytes_( cbDataCompressed ) const BYTE * const pbDataCompressed,
            const INT cbDataCompressed,
            const INT cbDataUncompressed,
            IDataCompressorStats * const pstats );
#endif

        ERR ErrDecompress7BitAscii_(
            const DATA& dataCompressed,
            _Out_writes_bytes_to_opt_( cbDataMax, *pcbDataActual ) BYTE * const pbData,
            const INT cbDataMax,
            _Out_ INT * const pcbDataActual );
        ERR ErrDecompress7BitUnicode_(
            const DATA& dataCompressed,
            _Out_writes_bytes_to_opt_( cbDataMax, *pcbDataActual ) BYTE * const pbData,
            const INT cbDataMax,
            _Out_ INT * const pcbDataActual );
        ERR ErrDecompressXpress_(
            const DATA& dataCompressed,
            _Out_writes_bytes_to_opt_( cbDataMax, *pcbDataActual ) BYTE * const pbData,
            const INT cbDataMax,
            _Out_ INT * const pcbDataActual,
            IDataCompressorStats * const pstats );
        ERR ErrDecompressScrub_(
            const DATA& data );
#ifdef XPRESS9_COMPRESSION
        ERR ErrDecompressXpress9_(
            const DATA& dataCompressed,
            _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
            const INT cbDataMax,
            _Out_ INT * const pcbDataActual,
            IDataCompressorStats * const pstats );
#endif
#ifdef XPRESS10_COMPRESSION
        ERR ErrDecompressXpress10_(
            const DATA& dataCompressed,
            _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
            const INT cbDataMax,
            _Out_ INT * const pcbDataActual,
            IDataCompressorStats * const pstats,
            const BOOL fForceSoftwareDecompression,
            BOOL * pfUsedCorsica );
#endif

private:
    CDataCompressor( const CDataCompressor& );
    CDataCompressor& operator=( const CDataCompressor& );
};

CDataCompressor::CDataCompressor() :
    m_cbMin( 0 ),
    m_cbMax( 0 ),
    m_cencodeCachedMax( 0 ),
    m_cdecodeCachedMax( 0 ),
    m_rgencodeXpress( NULL ),
    m_rgdecodeXpress( NULL )
#ifdef XPRESS9_COMPRESSION
    ,m_rgencodeXpress9( NULL )
    ,m_rgdecodeXpress9 ( NULL )
#endif
{
}

CDataCompressor::~CDataCompressor()
{
}

void * XPRESS_CALL CDataCompressor::PvXpressAlloc_( _In_opt_ void * pvContext, INT cbAlloc )
{
    return new BYTE[cbAlloc];
}

void XPRESS_CALL CDataCompressor::XpressFree_( _In_opt_ void * pvContext, _Post_ptr_invalid_ void * pvAlloc )
{
    delete [] pvAlloc;
}

ERR CDataCompressor::ErrXpressEncodeOpen_( _Out_ XpressEncodeStream * const pencode )
{
    C_ASSERT( sizeof(void*) == sizeof(XpressEncodeStream) );
    *pencode = 0;

    XpressEncodeStream encode = GetCachedPtr<XpressEncodeStream>( m_rgencodeXpress, m_cencodeCachedMax );
    if ( 0 != encode )
    {
        *pencode = encode;
        return JET_errSuccess;
    }

    encode = XpressEncodeCreate(
                    m_cbMax,
                    0,
                    PvXpressAlloc_,
                    xpressLegacyCompressionLevel );
    if( NULL == encode )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    *pencode = encode;
    return JET_errSuccess;
}

void CDataCompressor::XpressEncodeClose_( XpressEncodeStream encode )
{
    if ( encode )
    {
        if( FCachePtr<XpressEncodeStream>( encode, m_rgencodeXpress, m_cencodeCachedMax ) )
        {
            return;
        }

        XpressEncodeRelease_( encode );
    }
}

void CDataCompressor::XpressEncodeRelease_( XpressEncodeStream encode )
{
    if ( encode )
    {
        XpressEncodeClose( encode, 0, XpressFree_ );
    }
}

ERR CDataCompressor::ErrXpressDecodeOpen_( _Out_ XpressDecodeStream * const pdecode )
{
    C_ASSERT( sizeof(void*) == sizeof(XpressDecodeStream) );
    
    *pdecode = 0;

    XpressDecodeStream decode = GetCachedPtr<XpressDecodeStream>( m_rgdecodeXpress, m_cdecodeCachedMax );
    if( 0 != decode )
    {
        *pdecode = decode;
        return JET_errSuccess;
    }

    decode = XpressDecodeCreate(
                    0,
                    PvXpressAlloc_ );
    if (NULL == decode)
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    *pdecode = decode;
    return JET_errSuccess;
}

void CDataCompressor::XpressDecodeClose_( XpressDecodeStream decode )
{
    if ( decode )
    {
        if( FCachePtr<XpressDecodeStream>( decode, m_rgdecodeXpress, m_cdecodeCachedMax ) )
        {
            return;
        }

        XpressDecodeRelease_( decode );
    }
}

void CDataCompressor::XpressDecodeRelease_( XpressDecodeStream decode )
{
    if ( decode )
    {
        XpressDecodeClose( decode, 0, XpressFree_ );
    }
}

#ifdef XPRESS9_COMPRESSION
ERR CDataCompressor::ErrXpress9EncodeOpen_( _Out_ XPRESS9_ENCODER * const pencode )
{
    C_ASSERT( sizeof( void* ) == sizeof( XPRESS9_ENCODER ) );

    ERR err = JET_errSuccess;
    XPRESS9_STATUS status = { 0 };

    *pencode = 0;

    XPRESS9_ENCODER encode = GetCachedPtr<XPRESS9_ENCODER>( m_rgencodeXpress9, m_cencodeCachedMax );
    if ( 0 == encode )
    {
        encode = Xpress9EncoderCreate( &status, NULL, PvXpressAlloc_, XPRESS9_WINDOW_SIZE_LOG2_MIN, Xpress9Flag_UseSSE2 );
        Call( ErrXpress9StatusToJetErr( status ) );
    }

    *pencode = encode;

HandleError:
    if ( err < JET_errSuccess )
    {
        Xpress9EncodeRelease_( encode );
    }
    return err;
}

void CDataCompressor::Xpress9EncodeClose_( XPRESS9_ENCODER encode )
{
    if ( encode )
    {
        if ( FCachePtr<XPRESS9_ENCODER>( encode, m_rgencodeXpress9, m_cencodeCachedMax ) )
        {
            return;
        }

        Xpress9EncodeRelease_( encode );
    }
}

void CDataCompressor::Xpress9EncodeRelease_( XPRESS9_ENCODER encode )
{
    if ( encode )
    {
        XPRESS9_STATUS status = { 0 };
        Xpress9EncoderDestroy( &status, encode, NULL, XpressFree_ );
        Assert( status.m_uStatus == Xpress9Status_OK );
    }
}

ERR CDataCompressor::ErrXpress9DecodeOpen_( _Out_ XPRESS9_DECODER * const pdecode )
{
    C_ASSERT( sizeof( void* ) == sizeof( XPRESS9_DECODER ) );

    ERR err = JET_errSuccess;
    XPRESS9_STATUS status = { 0 };

    *pdecode = 0;

    XPRESS9_DECODER decode = GetCachedPtr<XPRESS9_DECODER>( m_rgdecodeXpress9, m_cdecodeCachedMax );
    if ( 0 == decode )
    {
        decode = Xpress9DecoderCreate( &status, NULL, PvXpressAlloc_, XPRESS9_WINDOW_SIZE_LOG2_MIN, Xpress9Flag_UseSSE2 );
        Call( ErrXpress9StatusToJetErr( status ) );
    }

    *pdecode = decode;

HandleError:
    if ( err < JET_errSuccess )
    {
        Xpress9DecodeRelease_( decode );
    }
    return err;
}

void CDataCompressor::Xpress9DecodeClose_( XPRESS9_DECODER decode )
{
    if ( decode )
    {
        if ( FCachePtr<XPRESS9_DECODER>( decode, m_rgdecodeXpress9, m_cdecodeCachedMax ) )
        {
            return;
        }

        Xpress9DecodeRelease_( decode );
    }
}

void CDataCompressor::Xpress9DecodeRelease_( XPRESS9_DECODER decode )
{
    if ( decode )
    {
        XPRESS9_STATUS status = { 0 };
        Xpress9DecoderDestroy( &status, decode, NULL, XpressFree_ );
        Assert( status.m_uStatus == Xpress9Status_OK );
    }
}
#endif

INT CDataCompressor::CbCompressed7BitAscii_( const INT cb )
{
    const INT cbCompressed = ( ((cb*7) + 7) / 8 ) + 1;
    return cbCompressed;
}

INT CDataCompressor::CbCompressed7BitUnicode_( const INT cb )
{
    const INT cbCompressed = ( (((cb/sizeof(WORD))*7) + 7) / 8 ) + 1;
    return cbCompressed;
}

CDataCompressor::COMPRESSION_SCHEME CDataCompressor::Calculate7BitCompressionScheme_( const DATA& data, BOOL fUnicodeOnly )
{
    bool fUnicodePossible = ( 0 == data.Cb() % sizeof(WORD) );
    if ( fUnicodeOnly && !fUnicodePossible )
    {
        return COMPRESS_NONE;
    }

    const DWORD dwMaskAscii     = fUnicodeOnly ? 0x007f007f : 0x7f7f7f7f;
    const DWORD dwMaskUnicode   = 0x007f007f;

    const Unaligned<DWORD> * const  pdw     = (Unaligned<DWORD> *)data.Pv();
    const INT                       cdw     = data.Cb() / sizeof(DWORD);

    INT idw = 0;
    switch( cdw % 8 )
    {
        case 0:
        while ( idw < cdw )
        {
            if( pdw[idw] & ~dwMaskAscii )
                return COMPRESS_NONE;
            fUnicodePossible = fUnicodePossible && !( pdw[idw] & ~dwMaskUnicode );
            idw++;
        case 7:
            if( pdw[idw] & ~dwMaskAscii )
                return COMPRESS_NONE;
            fUnicodePossible = fUnicodePossible && !( pdw[idw] & ~dwMaskUnicode );
            idw++;
        case 6:
            if( pdw[idw] & ~dwMaskAscii )
                return COMPRESS_NONE;
            fUnicodePossible = fUnicodePossible && !( pdw[idw] & ~dwMaskUnicode );
            idw++;
        case 5:
            if( pdw[idw] & ~dwMaskAscii )
                return COMPRESS_NONE;
            fUnicodePossible = fUnicodePossible && !( pdw[idw] & ~dwMaskUnicode );
            idw++;
        case 4:
            if( pdw[idw] & ~dwMaskAscii )
                return COMPRESS_NONE;
            fUnicodePossible = fUnicodePossible && !( pdw[idw] & ~dwMaskUnicode );
            idw++;
        case 3:
            if( pdw[idw] & ~dwMaskAscii )
                return COMPRESS_NONE;
            fUnicodePossible = fUnicodePossible && !( pdw[idw] & ~dwMaskUnicode );
            idw++;
        case 2:
            if( pdw[idw] & ~dwMaskAscii )
                return COMPRESS_NONE;
            fUnicodePossible = fUnicodePossible && !( pdw[idw] & ~dwMaskUnicode );
            idw++;
        case 1:
            if( pdw[idw] & ~dwMaskAscii )
                return COMPRESS_NONE;
            fUnicodePossible = fUnicodePossible && !( pdw[idw] & ~dwMaskUnicode );
            idw++;
        }
    }

    const BYTE * const  pb = (BYTE *)data.Pv();
    Assert( ( data.Cb() - (cdw * sizeof(DWORD)) ) < sizeof(DWORD) );
    for( INT ib = cdw * sizeof(DWORD); ib < data.Cb(); ++ib )
    {
        if( pb[ib] > 0x7f )
        {
            return COMPRESS_NONE;
        }
        if( fUnicodePossible && ( 0 == ( ib % sizeof(WORD) ) ) )
        {
            const Unaligned<WORD> * const pw = (Unaligned<WORD> *)(pb + ib);
            if( *pw > 0x007f )
            {
                fUnicodePossible = false;
            }
        }
    }

    if( fUnicodePossible )
    {
        const INT cbCompressed = CbCompressed7BitUnicode_(data.Cb());
        return cbCompressed < data.Cb() ? COMPRESS_7BITUNICODE : COMPRESS_NONE;
    }
    else if ( fUnicodeOnly )
    {
        return COMPRESS_NONE;
    }

    const INT cbCompressed = CbCompressed7BitAscii_(data.Cb());
    return cbCompressed < data.Cb() ? COMPRESS_7BITASCII : COMPRESS_NONE;
}

#define NBITMASK( n ) ((QWORD)( ( (QWORD)1 << n ) - 1 ))
#define ASCII7BITMASKANDSHIFT( qw, mask ) ( ( qw & mask ) | ((qw >> 1) & ~mask) )

__forceinline QWORD ReverseEightCompressedBytes( const unsigned __int64 qw )
{
    const unsigned __int64 qwMask1 = (NBITMASK(7) <<  0) | (NBITMASK(7) << 14) | (NBITMASK(7) << 28) | (NBITMASK(7) << 42);
    const unsigned __int64 qwMask2 = (NBITMASK(7) <<  7) | (NBITMASK(7) << 21) | (NBITMASK(7) << 35) | (NBITMASK(7) << 49);
    const INT shf1 = 7;

    const unsigned __int64 qwMask3 = (NBITMASK(7) <<  0) | (NBITMASK(7) <<  7) | (NBITMASK(7) << 28) | (NBITMASK(7) << 35);
    const unsigned __int64 qwMask4 = (NBITMASK(7) << 14) | (NBITMASK(7) << 21) | (NBITMASK(7) << 42) | (NBITMASK(7) << 49);
    const INT shf2 = 14;

    const unsigned __int64 qwMask5 = (NBITMASK(7) <<  0) | (NBITMASK(7) <<  7) | (NBITMASK(7) << 14) | (NBITMASK(7) << 21);
    const unsigned __int64 qwMask6 = (NBITMASK(7) << 28) | (NBITMASK(7) << 35) | (NBITMASK(7) << 42) | (NBITMASK(7) << 49);
    const INT shf3 = 28;

    C_ASSERT( 0xFFFFFFFFFFFFFF == ( qwMask1 ^ qwMask2 ) );
    C_ASSERT( 0xFFFFFFFFFFFFFF == ( qwMask3 ^ qwMask4 ) );
    C_ASSERT( 0xFFFFFFFFFFFFFF == ( qwMask5 ^ qwMask6 ) );
    
    const unsigned __int64 qw2 =    ( ( qw & qwMask2 ) >> shf1 ) |
                                    ( ( qw & qwMask1 ) << shf1 );
    const unsigned __int64 qw3 =    ( ( qw2 & qwMask4 ) >> shf2 ) |
                                    ( ( qw2 & qwMask3 ) << shf2 );
    return  ( ( qw3 & qwMask6 ) >> shf3 ) |
            ( ( qw3 & qwMask5 ) << shf3 );
}

ERR CDataCompressor::ErrCompress7BitAscii_(
    const DATA& data,
    _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    _In_ const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual )
{
    *pcbDataCompressedActual = 0;

    const INT cbData = data.Cb();

    const INT cbCompressed = CbCompressed7BitAscii_(cbData);
    if( cbCompressed > cbDataCompressedMax )
    {
        return ErrERRCheck( JET_errBufferTooSmall );
    }

    INT ibOutput    = 1;
    INT ibInput     = 0;

    INT cqwOutputCheck = 0;
    INT cdwOutputCheck = 0;
    INT cbOutputCheck = 0;

    const INT cqwInput  = cbData / sizeof(QWORD);
    const INT cqwOutput = ( cbDataCompressedMax - ibOutput - 1 ) / 7;

    for( INT iqw = 0; iqw < min( cqwInput, cqwOutput ); ++iqw )
    {
        BYTE * const pbInput    = (BYTE *)data.Pv() + ibInput;
        BYTE * const pbOutput   = pbDataCompressed + ibOutput;
        
        QWORD qw = *(Unaligned<QWORD> *)pbInput;

        qw = ReverseEightBytes( qw );

        
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 7  ) );
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 14 ) );
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 21 ) );
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 28 ) );
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 35 ) );
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 42 ) );
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 49 ) );

        qw = ReverseEightCompressedBytes( qw );


        *(Unaligned<QWORD> *)pbOutput = qw;

        ibInput     += sizeof(QWORD);
        ibOutput    += 7;

        cqwOutputCheck++;
    }

    DWORD dwOutput = 0;
    INT ibitOutputCurr = 0;
    const BYTE * const pb = (BYTE *)data.Pv();
    for( INT ib = ibInput; ib < cbData; ++ib )
    {
        INT i = pb[ib];
        Assert( i < 0x80 );

        
        const INT cbits = 7;
        INT cbitsRemaining = cbits;
        while( true )
        {
            const INT cbitsToCopy = min( cbitsRemaining, 32 - ibitOutputCurr );
            Assert( cbitsToCopy > 0 );
            Assert( cbitsToCopy < 32 );

            const ULONG ulBitMask = ( 1 << cbitsToCopy ) - 1;

            dwOutput |= (i & ulBitMask) << ibitOutputCurr;

            ibitOutputCurr += cbitsToCopy;
            if( 32 == ibitOutputCurr )
            {
                Unaligned<DWORD> * const pdwOutput = (Unaligned<DWORD> *)(pbDataCompressed + ibOutput);
                *pdwOutput = dwOutput;
                ibOutput += sizeof(DWORD);
                dwOutput = 0;
                ibitOutputCurr = 0;

                cdwOutputCheck++;
            }

            cbitsRemaining -= cbitsToCopy;
            if( 0 == cbitsRemaining )
            {
                break;
            }
            
            i >>= cbitsToCopy;

            Assert( ibitOutputCurr >= 0 );
            Assert( ibitOutputCurr < 32 );
            Assert( cbitsRemaining < cbits );
            Assert( cbitsRemaining >= 0 );
        }
    }

    AssertPREFIX( ibOutput < cbDataCompressedMax - 1 - ((ibitOutputCurr+7) / 8) );
    for( INT ib = 0; ib < (ibitOutputCurr+7) / 8; ++ib )
    {
        BYTE * const pbOutput = (BYTE *)(pbDataCompressed + ibOutput);
        BYTE b = (BYTE)( dwOutput & 0XFF );
        *pbOutput = b;
        ibOutput++;
        dwOutput >>= 8;

        cbOutputCheck++;
    }


    if( 0 == ibitOutputCurr )
    {
        ibitOutputCurr = 32;
    }
    
    const INT cbitFinalByte = ( ibitOutputCurr - 1 ) % 8;
    
    const BYTE bSignature = ( COMPRESS_7BITASCII << 3 ) | (BYTE)(cbitFinalByte);
    pbDataCompressed[0] = bSignature;

    AssertPREFIX( ibOutput == (INT)( 1 + cbOutputCheck + cdwOutputCheck * sizeof( DWORD ) + cqwOutputCheck * 7 ) );
    *pcbDataCompressedActual = ibOutput;
    return JET_errSuccess;
}

ERR CDataCompressor::ErrCompress7BitUnicode_(
    const DATA& data,
    _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    _In_ const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual )
{
    Assert( 0 == data.Cb() % sizeof(WORD) );
    *pcbDataCompressedActual = 0;

    const INT cbCompressed = CbCompressed7BitUnicode_(data.Cb());
    if( cbCompressed > cbDataCompressedMax )
    {
        return ErrERRCheck( JET_errBufferTooSmall );
    }
    
    INT ibitOutputCurr  = 0;
    Unaligned<DWORD> * const pdwOutput = (Unaligned<DWORD> *)(pbDataCompressed + 1);
    INT idwOutput = 0;
    
    DWORD dwOutput = 0;
    const WORD * const pw = (WORD *)data.Pv();
    for( INT iw = 0; iw < (INT)( data.Cb() / sizeof(WORD) ); ++iw )
    {
        INT i = pw[iw];
        Assert( i < 0x80 );

        
        const INT cbits = 7;
        INT cbitsRemaining = cbits;
        while( true )
        {
            const INT cbitsToCopy = min( cbitsRemaining, 32 - ibitOutputCurr );
            Assert( cbitsToCopy > 0 );
            Assert( cbitsToCopy < 32 );

            const ULONG ulBitMask = ( 1 << cbitsToCopy ) - 1;
            
            dwOutput |= (i & ulBitMask) << ibitOutputCurr;

            ibitOutputCurr += cbitsToCopy;
            if( 32 == ibitOutputCurr )
            {
                pdwOutput[idwOutput] = dwOutput;
                ++idwOutput;
                dwOutput = 0;
                ibitOutputCurr = 0;
            }

            cbitsRemaining -= cbitsToCopy;
            if( 0 == cbitsRemaining )
            {
                break;
            }
            
            i >>= cbitsToCopy;

            Assert( ibitOutputCurr >= 0 );
            Assert( ibitOutputCurr < 32 );
            Assert( cbitsRemaining < cbits );
            Assert( cbitsRemaining >= 0 );
        }
    }

    AssertPREFIX( (ULONG)( idwOutput * sizeof( DWORD ) ) < (ULONG)( cbDataCompressedMax - 1 - ( ( ibitOutputCurr + 7 ) / 8) ) );
    BYTE * pbOutput = (BYTE *)(pdwOutput + idwOutput);
    for( INT ib = 0; ib < (ibitOutputCurr+7) / 8; ++ib )
    {
        BYTE b = (BYTE)( dwOutput & 0XFF );
        *pbOutput++ = b;
        dwOutput >>= 8;
    }

    
    if( 0 == ibitOutputCurr )
    {
        Assert( idwOutput > 0 );
        ibitOutputCurr = 32;
        idwOutput--;
    }
    const INT cbOutput =
        1
        + idwOutput*sizeof(DWORD)
        + (ibitOutputCurr+7) / 8
        ;
    const INT cbitFinalByte = ( ibitOutputCurr - 1 ) % 8;
    
    const BYTE bSignature = ( COMPRESS_7BITUNICODE << 3 ) | (BYTE)(cbitFinalByte);
    pbDataCompressed[0] = bSignature;

    *pcbDataCompressedActual = cbOutput;
    return JET_errSuccess;
}

ERR CDataCompressor::ErrCompressXpress_(
    const DATA& data,
    _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual,
    IDataCompressorStats * const pstats )
{
    PERFOptDeclare( const HRT hrtStart = HrtHRTCount() );

    Assert( data.Cb() >= m_cbMin );
    Assert( data.Cb() <= wMax );
    Assert( data.Cb() <= m_cbMax );
    Assert( pstats );
    
    ERR err = JET_errSuccess;
    
    XpressEncodeStream encode = 0;
    Call( ErrXpressEncodeOpen_( &encode ) );

    const INT cbReserved = sizeof(BYTE) + sizeof(WORD);
    
    const INT cbCompressed = XpressEncode(
                        encode,
                        pbDataCompressed + cbReserved,
                        cbDataCompressedMax - cbReserved,
                        data.Pv(),
                        data.Cb(),
                        0,
                        0,
                        0 );
    Assert( cbCompressed <= data.Cb() );
    
    if( cbCompressed == 0 || cbCompressed + cbReserved >= data.Cb() )
    {
        err = ErrERRCheck( errRECCannotCompress );
    }
    else
    {
        BYTE * const pbSignature    = pbDataCompressed;
        UnalignedLittleEndian<WORD> * const pwSize  = (UnalignedLittleEndian<WORD> *)(pbSignature+1);

        *pbSignature    = ( COMPRESS_XPRESS << 3 );
        *pwSize         = (WORD)data.Cb();
        *pcbDataCompressedActual = cbCompressed + cbReserved;
    }

HandleError:
    XpressEncodeClose_( encode );

    if ( err == JET_errSuccess )
    {
        PERFOpt( pstats->AddUncompressedBytes( data.Cb() ) );
        PERFOpt( pstats->AddCompressedBytes( *pcbDataCompressedActual ) );
        PERFOpt( pstats->IncCompressionCalls() );
        PERFOpt( pstats->AddCompressionDhrts( HrtHRTCount() - hrtStart ) );
    }

    return err;
}

ERR CDataCompressor::ErrScrub(
    DATA& data,
    const CHAR chScrub )
{
    BYTE * const pbData = (BYTE *)data.Pv();
    const INT cbData = data.Cb();

    const BYTE bSignature = ( COMPRESS_SCRUB << 3 );
    pbData[0] = bSignature;
    memset( pbData + 1, chScrub, cbData - 1 );

    return JET_errSuccess;
}

#ifdef XPRESS9_COMPRESSION
ERR CDataCompressor::ErrXpress9StatusToJetErr(
    const XPRESS9_STATUS& status )
{
    if ( status.m_uStatus == Xpress9Status_OK )
    {
        return JET_errSuccess;
    }

    switch ( status.m_uStatus )
    {
        case Xpress9Status_NotEnoughMemory:
            return ErrERRCheck( JET_errOutOfMemory );

        case Xpress9Status_DecoderCorruptedData:
        case Xpress9Status_DecoderCorruptedHeader:
        case Xpress9Status_DecoderUnknownFormat:
            return ErrERRCheck( JET_errDecompressionFailed );

        case Xpress9Status_DecoderWindowTooLarge:
        case Xpress9Status_DecoderNotDetached:
        case Xpress9Status_DecoderNotDrained:
            AssertSz( fFalse, "Decoder usage error during decompress, status: %d", status.m_uStatus );
            return ErrERRCheck( JET_errInternalError );

        default:
            AssertSz( fFalse, "Unknown status(%d) coming from xpress9 compress or decompress", status.m_uStatus );
            return ErrERRCheck( JET_errInternalError );
    }
}


ERR CDataCompressor::ErrVerifyCompressXpress9_(
    _In_reads_bytes_( cbDataCompressed ) const BYTE * const pbDataCompressed,
    const INT cbDataCompressed,
    const INT cbDataCompressedMax,
    IDataCompressorStats * const pstats )
{
    ERR err = JET_errSuccess;
    DATA dataCompressed;
    dataCompressed.SetPv( const_cast<BYTE*>( pbDataCompressed ) );
    dataCompressed.SetCb( cbDataCompressed );

    BYTE* pbDecompress;
    INT cbDataDecompressed = 0;

    if ( g_compressionBufferCache.CbBufferSize() != 0 )
    {
        Alloc( pbDecompress = g_compressionBufferCache.PbAlloc() );
    }
    else
    {
        Alloc( pbDecompress = new BYTE[ cbDataCompressedMax ] );
    }

    err = ErrDecompressXpress9_( dataCompressed, pbDecompress, cbDataCompressedMax, &cbDataDecompressed, pstats );
    if ( err != JET_errSuccess && err != JET_errOutOfMemory )
    {

        FireWall( "Xpress9CannotCompress" );
        err = ErrERRCheck( errRECCannotCompress );
    }

    if ( g_compressionBufferCache.CbBufferSize() != 0 )
    {
        g_compressionBufferCache.Free( pbDecompress );
    }
    else
    {
        delete[] pbDecompress;
    }

HandleError:
    return err;
}

ERR CDataCompressor::ErrCompressXpress9_(
    const DATA& data,
    _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual )
{
    Assert( data.Cb() >= m_cbMin );
    Assert( data.Cb() <= lMax );

    ERR err = JET_errSuccess;
    const INT cbReserved = sizeof( Xpress9Header );
    XPRESS9_STATUS status = { 0 };
    XPRESS9_ENCODER encode = 0;
    Call( ErrXpress9EncodeOpen_( &encode ) );

    XPRESS9_ENCODER_PARAMS params;
    memset( &params, 0, sizeof( params ) );
    params.m_cbSize = sizeof( params );
    params.m_uMaxStreamLength = (unsigned)data.Cb();
    params.m_uMtfEntryCount = 4;
    params.m_uPtrMinMatchLength = 4;
    params.m_uLookupDepth = 6;
    params.m_uOptimizationLevel = 0;
    params.m_uWindowSizeLog2 = XPRESS9_WINDOW_SIZE_LOG2_MIN;
    params.m_uMtfMinMatchLength = 2;
    Xpress9EncoderStartSession( &status, encode, &params, fTrue );
    Call( ErrXpress9StatusToJetErr( status ) );

    Xpress9EncoderAttach( &status, encode, data.Pv(), (unsigned)data.Cb(), fTrue );
    Call( ErrXpress9StatusToJetErr( status ) );

    INT cbDataCompressed = 0;
    while ( Xpress9EncoderCompress( &status, encode, NULL, NULL ) > 0 )
    {
        Call( ErrXpress9StatusToJetErr( status ) );

        unsigned cbCompressed = 0;
        unsigned cbCompressedRemainder = Xpress9EncoderFetchCompressedData(
                &status,
                encode,
                pbDataCompressed + cbReserved + cbDataCompressed,
                (unsigned)( cbDataCompressedMax - cbReserved - cbDataCompressed ),
                &cbCompressed );
        Call( ErrXpress9StatusToJetErr( status ) );

        if ( cbCompressedRemainder > 0 )
        {
            
            do
            {
                cbCompressedRemainder = Xpress9EncoderFetchCompressedData(
                        &status,
                        encode,
                        pbDataCompressed + cbReserved + cbDataCompressed,
                        (unsigned) ( cbDataCompressedMax - cbReserved - cbDataCompressed ),
                        &cbCompressed );
            }
            while ( cbCompressedRemainder > 0 );

            Call( ErrERRCheck( errRECCannotCompress ) );
        }
        cbDataCompressed += cbCompressed;
    }

    if ( cbDataCompressed + cbReserved >= data.Cb() )
    {
        Call( ErrERRCheck( errRECCannotCompress ) );
    }

    Xpress9Header* const pHdr = (Xpress9Header*) pbDataCompressed;
    pHdr->m_fCompressScheme = ( COMPRESS_XPRESS9 << 3 );
    pHdr->m_ulChecksum = Crc32Checksum( (BYTE*)data.Pv(), data.Cb() );
    *pcbDataCompressedActual = cbDataCompressed + cbReserved;

HandleError:
    if ( encode != 0 )
    {
        Xpress9EncoderDetach( &status, encode, data.Pv(), (unsigned)data.Cb() );
        Xpress9EncodeClose_( encode );
    }

    return err;
}
#endif

#ifdef XPRESS10_COMPRESSION

ERR CDataCompressor::ErrVerifyCompressXpress10_(
    _In_reads_bytes_( cbDataCompressed ) const BYTE * const pbDataCompressed,
    const INT cbDataCompressed,
    const INT cbDataUncompressed,
    IDataCompressorStats * const pstats )
{
    ERR err = JET_errSuccess;
    DATA dataCompressed;
    dataCompressed.SetPv( const_cast<BYTE*>( pbDataCompressed ) );
    dataCompressed.SetCb( cbDataCompressed );

    BYTE* pbDecompress;
    INT cbDataDecompressed = 0;
    BOOL fUsedCorsica = fFalse;

    if ( g_compressionBufferCache.CbBufferSize() != 0 )
    {
        Assert( g_compressionBufferCache.CbBufferSize() >= cbDataUncompressed );
        Alloc( pbDecompress = g_compressionBufferCache.PbAlloc() );
    }
    else
    {
        Alloc( pbDecompress = new BYTE[ cbDataUncompressed ] );
    }

    err = ErrDecompressXpress10_( dataCompressed, pbDecompress, cbDataUncompressed, &cbDataDecompressed, pstats, fFalse, &fUsedCorsica );
    if ( ( err == JET_errSuccess && cbDataUncompressed != cbDataDecompressed ) ||
         ( err != JET_errSuccess && err != JET_errOutOfMemory ) )
    {

        FireWall( "Xpress10CorsicaCannotDecompress" );
        err = ErrERRCheck( errRECCannotCompress );
    }
    else if ( fUsedCorsica )
    {
        err = ErrDecompressXpress10_( dataCompressed, pbDecompress, cbDataUncompressed, &cbDataDecompressed, pstats, fTrue, &fUsedCorsica );
        AssertTrack( err != JET_errSuccess || !fUsedCorsica, "SoftwareDecompressionFlagNotHonored" );
        if ( ( err == JET_errSuccess && cbDataUncompressed != cbDataDecompressed ) ||
             ( err != JET_errSuccess && err != JET_errOutOfMemory ) )
        {

            FireWall( "Xpress10SoftwareCannotDecompress" );
            err = ErrERRCheck( errRECCannotCompress );
        }
    }

    if ( g_compressionBufferCache.CbBufferSize() != 0 )
    {
        g_compressionBufferCache.Free( pbDecompress );
    }
    else
    {
        delete[] pbDecompress;
    }

HandleError:
    return err;
}

BOOL g_fAllowXpress10SoftwareCompression = fFalse;
CInitOnce< ERR, decltype(&ErrXpress10CorsicaInit) > g_CorsicaInitOnce;

ERR CDataCompressor::ErrCompressXpress10_(
    const DATA& data,
    _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual,
    IDataCompressorStats * const pstats )
{
    PERFOptDeclare( const HRT hrtStart = HrtHRTCount() );
    ERR err;

    (VOID)g_CorsicaInitOnce.Init( ErrXpress10CorsicaInit );

    Assert( data.Cb() <= wMax );

    C_ASSERT( sizeof( Xpress10Header ) == 15 );
    const INT cbReserved = sizeof( Xpress10Header );
    ULONGLONG ullCompressedCrc;
    HRT dhrtHardwareLatency;

    if ( FXpress10CorsicaHealthy() )
    {
        err = ErrXpress10CorsicaCompress(
                (PBYTE)data.Pv(),
                data.Cb(),
                pbDataCompressed + cbReserved,
                cbDataCompressedMax - cbReserved,
                (PULONG)pcbDataCompressedActual,
                &ullCompressedCrc,
                &dhrtHardwareLatency );

        if ( err == JET_errSuccess )
        {
            PERFOpt( pstats->AddXpress10CorsicaCompressionBytes( data.Cb() ) );
            PERFOpt( pstats->AddXpress10CorsicaCompressedBytes( *pcbDataCompressedActual ) );
            PERFOpt( pstats->IncXpress10CorsicaCompressionCalls() );
            PERFOpt( pstats->AddXpress10CorsicaCompressionDhrts( HrtHRTCount() - hrtStart ) );
            PERFOpt( pstats->AddXpress10CorsicaCompressionHardwareDhrts( dhrtHardwareLatency ) );
        }
    }
    else if ( g_fAllowXpress10SoftwareCompression )
    {
        err = ErrXpress10SoftwareCompress(
                (PBYTE)data.Pv(),
                data.Cb(),
                pbDataCompressed + cbReserved,
                cbDataCompressedMax - cbReserved,
                (PULONG)pcbDataCompressedActual,
                &ullCompressedCrc );
    }
    else
    {
        err = ErrERRCheck( errRECCannotCompress );
    }
    if ( err == JET_errSuccess )
    {
        Xpress10Header * const pHdr = (Xpress10Header *)pbDataCompressed;
        pHdr->m_fCompressScheme = ( COMPRESS_XPRESS10 << 3 );
        pHdr->mle_cbUncompressed  = (WORD)data.Cb();
        pHdr->mle_ulUncompressedChecksum = Crc32Checksum( (BYTE*)data.Pv(), data.Cb() );
        pHdr->mle_ullCompressedChecksum = ullCompressedCrc;
        *pcbDataCompressedActual += cbReserved;
    }

    return err;
}
#endif

ERR CDataCompressor::ErrDecompress7BitAscii_(
    const DATA& dataCompressed,
    _Out_writes_bytes_to_opt_( cbDataMax, *pcbDataActual ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual )
{
    ERR err = JET_errSuccess;
    
    const BYTE * const pbCompressed = (BYTE *)dataCompressed.Pv();
    const BYTE bHeader = *(BYTE *)dataCompressed.Pv();
    const BYTE bIdentifier = bHeader >> 3;
    Assert( bIdentifier == COMPRESS_7BITASCII );
    const INT cbitFinal = (bHeader & 0x7)+1;
    Assert( cbitFinal > 0 );
    Assert( cbitFinal <= 8 );

    const INT cbitTotal = ( ( dataCompressed.Cb() - 2 ) * 8 ) + cbitFinal;
    Assert( 0 == cbitTotal % 7 );
    const INT cbTotal = cbitTotal / 7;
    
    *pcbDataActual = cbTotal;
    if( cbTotal > cbDataMax )
    {
        err = ErrERRCheck( JET_wrnBufferTruncated );
    }

    if( 0 == cbDataMax || NULL == pbData )
    {
        goto HandleError;
    }

    const INT ibDataMax = min( cbTotal, cbDataMax );

    INT ibCompressed = 1;
    INT ibitCompressed = 0;
    for( INT ibData = 0; ibData < ibDataMax; ++ibData )
    {
        Assert( ibCompressed < dataCompressed.Cb() );
        BYTE bDecompressed;
        if( ibitCompressed <= 1 )
        {
            const BYTE bCompressed = pbCompressed[ibCompressed];
            bDecompressed = (BYTE)(( bCompressed >> ibitCompressed ) & 0x7F);
        }
        else
        {
            Assert( ibCompressed < dataCompressed.Cb()-1 );
            const WORD wCompressed = (WORD)pbCompressed[ibCompressed] | ( (WORD)pbCompressed[ibCompressed+1] << 8 );
            bDecompressed = (BYTE)(( wCompressed >> ibitCompressed ) & 0x7F);
        }
        pbData[ibData] = bDecompressed;
        ibitCompressed += 7;
        if( ibitCompressed >= 8 )
        {
            ibitCompressed = ( ibitCompressed % 8 );
            ++ibCompressed;
        }
    }

HandleError:
#pragma prefast(suppress : 26030, "In case of JET_wrnBufferTruncated, we return what the buffer size should be.")
    return err;
}

ERR CDataCompressor::ErrDecompress7BitUnicode_(
    const DATA& dataCompressed,
    _Out_writes_bytes_to_opt_( cbDataMax, *pcbDataActual ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual )
{
    ERR err = JET_errSuccess;
    
    const BYTE * const pbCompressed = (BYTE *)dataCompressed.Pv();
    const BYTE bHeader = *(BYTE *)dataCompressed.Pv();
    const BYTE bIdentifier = bHeader >> 3;
    Assert( bIdentifier == COMPRESS_7BITUNICODE );
    const INT cbitFinal = (bHeader & 0x7)+1;
    Assert( cbitFinal > 0 );
    Assert( cbitFinal <= 8 );

    const INT cbitTotal = ( ( dataCompressed.Cb() - 2 ) * 8 ) + cbitFinal;
    Assert( 0 == cbitTotal % 7 );
    const INT cwTotal = cbitTotal / 7;
    
    *pcbDataActual = cwTotal * sizeof(WORD);
    if( *pcbDataActual > cbDataMax )
    {
        err = ErrERRCheck( JET_wrnBufferTruncated );
    }

    if( 0 == cbDataMax || NULL == pbData )
    {
        goto HandleError;
    }

    const INT ibDataMax = min( *pcbDataActual, cbDataMax );

    INT ibCompressed = 1;
    INT ibitCompressed = 0;
    for( INT ibData = 0; ibData < ibDataMax; )
    {
        Assert( ibCompressed < dataCompressed.Cb() );
        BYTE bDecompressed;
        if( ibitCompressed <= 1 )
        {
            const BYTE bCompressed = pbCompressed[ibCompressed];
            bDecompressed = (BYTE)(( bCompressed >> ibitCompressed ) & 0x7F);
        }
        else
        {
            Assert( ibCompressed < dataCompressed.Cb()-1 );
            const WORD wCompressed = (WORD)pbCompressed[ibCompressed] | ( (WORD)pbCompressed[ibCompressed+1] << 8 );
            bDecompressed = (BYTE)(( wCompressed >> ibitCompressed ) & 0x7F);
        }

        pbData[ibData++] = bDecompressed;
        if( ibData >= ibDataMax )
        {
            break;
        }
        pbData[ibData++] = 0x0;
        if( ibData >= ibDataMax )
        {
            break;
        }
        
        ibitCompressed += 7;
        if( ibitCompressed >= 8 )
        {
            ibitCompressed = ( ibitCompressed % 8 );
            ++ibCompressed;
        }
    }

HandleError:
#pragma prefast(suppress : 26030, "In case of JET_wrnBufferTruncated, we return what the buffer size should be.")
    return err;
}

ERR CDataCompressor::ErrDecompressXpress_(
    const DATA& dataCompressed,
    _Out_writes_bytes_to_opt_( cbDataMax, *pcbDataActual ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual,
    IDataCompressorStats * const pstats )
{
    ERR err = JET_errSuccess;
    PERFOptDeclare( const HRT hrtStart = HrtHRTCount() );

    XpressDecodeStream decode = 0;

    const BYTE * const  pbCompressed    = (BYTE *)dataCompressed.Pv();
    const UnalignedLittleEndian<WORD> * const pwSize    = (UnalignedLittleEndian<WORD> *)(pbCompressed + 1);
    const INT cbUncompressed            = *pwSize;
    
    const BYTE bHeader      = *(BYTE *)dataCompressed.Pv();
    OnDebug( const BYTE bIdentifier     = bHeader >> 3 );
    Assert( bIdentifier == COMPRESS_XPRESS );

    *pcbDataActual = cbUncompressed;
    
    const INT cbHeader              = sizeof(BYTE) + sizeof(WORD);
    const BYTE * pbCompressedData   = (BYTE *)dataCompressed.Pv() + cbHeader;
    const INT cbCompressedData      = dataCompressed.Cb() - cbHeader;
    const INT cbWanted              = min( *pcbDataActual, cbDataMax );

    if ( NULL == pbData || 0 == cbDataMax )
    {
        err = ErrERRCheck( JET_wrnBufferTruncated );
        goto HandleError;
    }

    Assert( cbWanted <= cbUncompressed );

    Call( ErrXpressDecodeOpen_( &decode ) );

    const INT cbDecoded = XpressDecode(
        decode,
        pbData,
        cbUncompressed,
        cbWanted,
        pbCompressedData,
        cbCompressedData );
    if( -1 == cbDecoded )
    {
        Call( ErrERRCheck( JET_errDecompressionFailed ) );
    }
    Assert( cbDecoded == cbWanted );

    if( cbUncompressed > cbDataMax )
    {
        err = ErrERRCheck( JET_wrnBufferTruncated );
    }

HandleError:
    XpressDecodeClose_( decode );

    if ( err >= JET_errSuccess )
    {
        PERFOpt( pstats->AddDecompressionBytes( cbWanted ) );
        PERFOpt( pstats->IncDecompressionCalls() );
        PERFOpt( pstats->AddDecompressionDhrts( HrtHRTCount() - hrtStart ) );
    }

#pragma prefast(suppress : 26030, "In case of JET_wrnBufferTruncated, we return what the buffer size should be.")
    return err;
}

ERR CDataCompressor::ErrDecompressScrub_(
    const DATA& data )
{
#ifdef DEBUG
    const BYTE * const pbData = (BYTE *)data.Pv();
    const INT cbData = data.Cb();
    Assert( cbData >= 1 );
    const BYTE bHeader = pbData[0];
    const BYTE bIdentifier = bHeader >> 3;

    Assert( bIdentifier == COMPRESS_SCRUB );
    Expected( ( bHeader & 0x07 ) == 0 );

    if ( cbData > 1 )
    {
        const CHAR chKnownPattern = pbData[1];

        Expected( chKnownPattern == chSCRUBLegacyLVChunkFill || chKnownPattern == chSCRUBDBMaintLVChunkFill );
        
        const INT cbKnownPattern = cbData - 1;
        BYTE* pbKnownPattern = new BYTE[cbKnownPattern];
        if ( pbKnownPattern != NULL )
        {
            memset( pbKnownPattern, chKnownPattern, cbKnownPattern );
            Assert( memcmp( pbKnownPattern, pbData + 1, cbKnownPattern ) == 0 );
        }
        delete [] pbKnownPattern;
    }
#endif
    return ErrERRCheck( wrnRECCompressionScrubDetected );
}

#ifdef XPRESS9_COMPRESSION
ERR CDataCompressor::ErrDecompressXpress9_(
    const DATA& dataCompressed,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual,
    IDataCompressorStats * const pstats )
{
    ERR err = JET_errSuccess;
    PERFOptDeclare( const HRT hrtStart = HrtHRTCount() );

    XPRESS9_STATUS status = { 0 };
    XPRESS9_DECODER decode = 0;
    unsigned cbDecompressed = 0;
    unsigned cbCompressedNeeded = 0;

    const INT cbReserved = sizeof( Xpress9Header );

    const BYTE * pbCompressedData = (BYTE *)dataCompressed.Pv() + cbReserved;
    const INT cbCompressedData = dataCompressed.Cb() - cbReserved;
    if ( cbCompressedData < 0 )
    {
        return ErrERRCheck( JET_errDecompressionFailed );
    }

    Xpress9Header* pHdr = (Xpress9Header*) dataCompressed.Pv();
    const BYTE bIdentifier = pHdr->m_fCompressScheme >> 3;
    if ( ( bIdentifier != COMPRESS_XPRESS9 ) )
    {
        return ErrERRCheck( JET_errDecompressionFailed );
    }

    Call( ErrXpress9DecodeOpen_( &decode ) );
    Xpress9DecoderStartSession( &status, decode, TRUE );
    Call( ErrXpress9StatusToJetErr( status ) );

    Xpress9DecoderAttach( &status, decode, pbCompressedData, (unsigned)cbCompressedData );
    Call( ErrXpress9StatusToJetErr( status ) );

    const unsigned cbDecompressedRemainder = Xpress9DecoderFetchDecompressedData(
            &status,
            decode,
            pbData,
            cbDataMax,
            &cbDecompressed,
            &cbCompressedNeeded );
    Call( ErrXpress9StatusToJetErr( status ) );

    if ( cbCompressedNeeded != 0 )
    {
        Call( ErrERRCheck( JET_errDecompressionFailed ) );
    }

    *pcbDataActual = cbDecompressed + cbDecompressedRemainder;
    if ( *pcbDataActual > cbDataMax )
    {
        err = ErrERRCheck( JET_wrnBufferTruncated );
    }
    else
    {
        pHdr = (Xpress9Header*) dataCompressed.Pv();
        ULONG computedChecksum = Crc32Checksum( pbData, *pcbDataActual );
        if ( computedChecksum != pHdr->m_ulChecksum )
        {
            Call( ErrERRCheck( JET_errCompressionIntegrityCheckFailed ) );
        }
    }

HandleError:
    if ( decode != NULL )
    {
        Xpress9DecoderDetach( &status, decode, pbCompressedData, (unsigned)cbCompressedData );
        Xpress9DecodeClose_( decode );
    }

    if ( err >= JET_errSuccess )
    {
        PERFOpt( pstats->AddCpuXpress9DecompressionBytes( cbDecompressed ) );
        PERFOpt( pstats->IncCpuXpress9DecompressionCalls() );
        PERFOpt( pstats->AddCpuXpress9DecompressionDhrts( HrtHRTCount() - hrtStart ) );
    }

    return err;
}
#endif

#ifdef XPRESS10_COMPRESSION
ERR CDataCompressor::ErrDecompressXpress10_(
    const DATA& dataCompressed,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual,
    IDataCompressorStats * const pstats,
    BOOL fForceSoftwareDecompression,
    BOOL *pfUsedCorsica )
{
    ERR err = JET_errSuccess;
    BYTE *pbAlloc = NULL;

    (VOID)g_CorsicaInitOnce.Init( ErrXpress10CorsicaInit );

    PERFOptDeclare( const HRT hrtStart = HrtHRTCount() );
    HRT dhrtHardwareLatency;

    C_ASSERT( sizeof( Xpress10Header ) == 15 );
    const INT cbReserved = sizeof( Xpress10Header );
    const INT cbCompressedData = dataCompressed.Cb() - cbReserved;
    if ( cbCompressedData < 0 )
    {
        return ErrERRCheck( JET_errDecompressionFailed );
    }

    const Xpress10Header * const pHdr = (Xpress10Header *)dataCompressed.Pv();
    if ( ( pHdr->m_fCompressScheme >> 3 ) != COMPRESS_XPRESS10 )
    {
        return ErrERRCheck( JET_errDecompressionFailed );
    }
    const INT cbUncompressed = pHdr->mle_cbUncompressed;
    if ( NULL == pbData || 0 == cbDataMax )
    {
        *pcbDataActual = cbUncompressed;
        return ErrERRCheck( JET_wrnBufferTruncated );
    }

    if ( cbDataMax < cbUncompressed )
    {
        if ( g_compressionBufferCache.CbBufferSize() != 0 )
        {
            Assert( g_compressionBufferCache.CbBufferSize() >= cbUncompressed );
            AllocR( pbAlloc = g_compressionBufferCache.PbAlloc() );
        }
        else
        {
            AllocR( pbAlloc = new BYTE[ cbUncompressed ] );
        }
    }

    if ( FXpress10CorsicaHealthy() && !fForceSoftwareDecompression )
    {
        err = ErrXpress10CorsicaDecompress(
                pbAlloc ? pbAlloc : pbData,
                pbAlloc ? cbUncompressed : cbDataMax,
                (BYTE *)dataCompressed.Pv() + cbReserved,
                cbCompressedData,
                pHdr->mle_ullCompressedChecksum,
                (PULONG)pcbDataActual,
                &dhrtHardwareLatency );
        if ( err == JET_errSuccess )
        {
            Assert( cbUncompressed == *pcbDataActual );

            *pfUsedCorsica = fTrue;
            PERFOpt( pstats->AddXpress10CorsicaDecompressionBytes( *pcbDataActual ) );
            PERFOpt( pstats->IncXpress10CorsicaDecompressionCalls() );
            PERFOpt( pstats->AddXpress10CorsicaDecompressionDhrts( HrtHRTCount() - hrtStart ) );
            PERFOpt( pstats->AddXpress10CorsicaDecompressionHardwareDhrts( dhrtHardwareLatency ) );
        }
    }
    else
    {
        err = ErrERRCheck( JET_errDeviceFailure );
    }

    if ( err < JET_errSuccess )
    {
        err = ErrXpress10SoftwareDecompress(
                pbAlloc ? pbAlloc : pbData,
                pbAlloc ? cbUncompressed : cbDataMax,
                (BYTE *)dataCompressed.Pv() + cbReserved,
                cbCompressedData,
                pHdr->mle_ullCompressedChecksum,
                (PULONG)pcbDataActual );
        if ( err == JET_errSuccess )
        {
            Assert( cbUncompressed == *pcbDataActual );

            *pfUsedCorsica = fFalse;
            PERFOpt( pstats->AddXpress10SoftwareDecompressionBytes( *pcbDataActual ) );
            PERFOpt( pstats->IncXpress10SoftwareDecompressionCalls() );
            PERFOpt( pstats->AddXpress10SoftwareDecompressionDhrts( HrtHRTCount() - hrtStart ) );
        }
    }

    if ( err == JET_errSuccess &&
         pHdr->mle_ulUncompressedChecksum != Crc32Checksum( pbAlloc ? pbAlloc : pbData, *pcbDataActual ) )
    {
        err = ErrERRCheck( JET_errCompressionIntegrityCheckFailed );
    }

    if ( err == JET_errSuccess && pbAlloc )
    {
        Assert( cbDataMax < *pcbDataActual );
        UtilMemCpy( pbData, pbAlloc, cbDataMax );
        err = ErrERRCheck( JET_wrnBufferTruncated );
    }

    if ( g_compressionBufferCache.CbBufferSize() != 0 )
    {
        g_compressionBufferCache.Free( pbAlloc );
    }
    else
    {
        delete[] pbAlloc;
    }
    return err;
}
#endif

ERR CDataCompressor::ErrCompress(
    const DATA& data,
    const CompressFlags compressFlags,
    IDataCompressorStats * const pstats,
    _Out_writes_bytes_to_opt_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual )
{
    ERR err = JET_errSuccess;
    BOOL fCompressed = fFalse;

    Assert( 0 != compressFlags );

    if ( data.Cb() >= m_cbMin )
    {
#ifdef XPRESS10_COMPRESSION
        if ( ( compressFlags & compressXpress10 ) && data.Cb() <= wMax )
        {
            CallJ( ErrCompressXpress10_(
                data,
                pbDataCompressed,
                cbDataCompressedMax,
                pcbDataCompressedActual,
                pstats ),
                TryXpress9Compression );

            CallJ( ErrVerifyCompressXpress10_( pbDataCompressed, *pcbDataCompressedActual, data.Cb(), pstats ), TryXpress9Compression );

            fCompressed = fTrue;
        }

TryXpress9Compression:
#endif
#ifdef XPRESS9_COMPRESSION
        if ( !fCompressed && ( compressFlags & compressXpress9 ) )
        {
            CallJ( ErrCompressXpress9_(
                data,
                pbDataCompressed,
                cbDataCompressedMax,
                pcbDataCompressedActual ),
                TryXpressCompression );


            CallJ( ErrVerifyCompressXpress9_( pbDataCompressed, *pcbDataCompressedActual, cbDataCompressedMax, pstats ), TryXpressCompression );

            fCompressed = fTrue;
        }

TryXpressCompression:
#endif
        if ( !fCompressed && ( compressFlags & compressXpress ) )
        {
            CallJ( ErrCompressXpress_(
                        data,
                        pbDataCompressed,
                        cbDataCompressedMax,
                        pcbDataCompressedActual,
                        pstats ),
                        Try7bitCompression );

            fCompressed = fTrue;
        }


        if ( fCompressed && *pcbDataCompressedActual <= CbCompressed7BitUnicode_( data.Cb() ) )
        {
            return JET_errSuccess;
        }
    }

Try7bitCompression:
    if ( err != JET_errSuccess && err != errRECCannotCompress && err != JET_errOutOfMemory )
    {
        return err;
    }

    if ( compressFlags & compress7Bit )
    {
        switch ( Calculate7BitCompressionScheme_( data, fCompressed ) )
        {
            case COMPRESS_7BITASCII:
                Assert( !fCompressed );
                return ErrCompress7BitAscii_(
                        data,
                        pbDataCompressed,
                        cbDataCompressedMax,
                        pcbDataCompressedActual );

            case COMPRESS_7BITUNICODE:
                return ErrCompress7BitUnicode_(
                        data,
                        pbDataCompressed,
                        cbDataCompressedMax,
                        pcbDataCompressedActual );
        }
    }

    if ( fCompressed )
    {
        return JET_errSuccess;
    }

    return ErrERRCheck( errRECCannotCompress );
}

ERR CDataCompressor::ErrDecompress(
    const DATA& dataCompressed,
    IDataCompressorStats * const pstats,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual )
{
    ERR err = JET_errSuccess;
    BOOL fUnused = fFalse;

    const BYTE bHeader = *(BYTE *)dataCompressed.Pv();
    const BYTE bIdentifier = bHeader >> 3;

    switch( bIdentifier )
    {
        case COMPRESS_7BITASCII:
            Call( ErrDecompress7BitAscii_( dataCompressed, pbData, cbDataMax, pcbDataActual ) );
            break;
        case COMPRESS_7BITUNICODE:
            Call( ErrDecompress7BitUnicode_( dataCompressed, pbData, cbDataMax, pcbDataActual ) );
            break;
        case COMPRESS_XPRESS:
            Call( ErrDecompressXpress_( dataCompressed, pbData, cbDataMax, pcbDataActual, pstats ) );
            break;
        case COMPRESS_SCRUB:
            Expected( pbData == NULL );
            Expected( cbDataMax == 0 );
            Assert( g_fRepair || FNegTest( fInvalidUsage ) );
            *pcbDataActual = 0;
            Call( ErrDecompressScrub_( dataCompressed ) );
            break;
#ifdef XPRESS9_COMPRESSION
        case COMPRESS_XPRESS9:
            Call( ErrDecompressXpress9_( dataCompressed, pbData, cbDataMax, pcbDataActual, pstats ) );
            break;
#endif
#ifdef XPRESS10_COMPRESSION
        case COMPRESS_XPRESS10:
            Call( ErrDecompressXpress10_( dataCompressed, pbData, cbDataMax, pcbDataActual, pstats, fFalse, &fUnused ) );
            break;
#endif
        default:
            *pcbDataActual = 0;
            Call( ErrERRCheck( JET_errDecompressionFailed ) );
            break;
    }

    if( *pcbDataActual > cbDataMax )
    {
        Assert( JET_wrnBufferTruncated == err );
    }

HandleError:
    Expected( ( bIdentifier != COMPRESS_SCRUB ) || ( err == wrnRECCompressionScrubDetected ) );
    
    return err;
}

ERR CDataCompressor::ErrInit( const INT cbMin, const INT cbMax )
{
    ERR err = JET_errSuccess;

    Assert( cbMin >= 0 );
    m_cbMin = cbMin;
    Assert( cbMax >= cbMin );
    m_cbMax = cbMax;

    m_cencodeCachedMax = OSSyncGetProcessorCountMax();
    m_cdecodeCachedMax = OSSyncGetProcessorCountMax();

    Assert( m_rgencodeXpress == NULL );
    Assert( m_rgdecodeXpress == NULL );
    Alloc( m_rgencodeXpress = new XpressEncodeStream[ m_cencodeCachedMax ]() );
    Alloc( m_rgdecodeXpress = new XpressDecodeStream[ m_cdecodeCachedMax ]() );

#ifdef XPRESS9_COMPRESSION
    Assert( m_rgencodeXpress9 == NULL );
    Assert( m_rgdecodeXpress9 == NULL );
    Alloc( m_rgencodeXpress9 = new XPRESS9_ENCODER[ m_cencodeCachedMax ]() );
    Alloc( m_rgdecodeXpress9 = new XPRESS9_DECODER[ m_cdecodeCachedMax ]() );
#endif

    return err;

HandleError:
    delete[] m_rgencodeXpress;
    m_rgencodeXpress = NULL;
    delete[] m_rgdecodeXpress;
    m_rgdecodeXpress = NULL;
#ifdef XPRESS9_COMPRESSION
    delete[] m_rgencodeXpress9;
    m_rgencodeXpress9 = NULL;
    delete[] m_rgdecodeXpress9;
    m_rgdecodeXpress9 = NULL;
#endif

    return err;
}

void CDataCompressor::Term()
{
    for ( INT iencode = 0; iencode < m_cencodeCachedMax; ++iencode )
    {
        if ( m_rgencodeXpress != NULL && m_rgencodeXpress[iencode] != NULL )
        {
            XpressEncodeRelease_( m_rgencodeXpress[iencode] );
            m_rgencodeXpress[iencode] = 0;
        }
#ifdef XPRESS9_COMPRESSION
        if ( m_rgencodeXpress9 != NULL && m_rgencodeXpress9[iencode] != NULL )
        {
            Xpress9EncodeRelease_( m_rgencodeXpress9[iencode] );
            m_rgencodeXpress9[iencode] = 0;
        }
#endif
    }
    for ( INT idecode = 0; idecode < m_cdecodeCachedMax; ++idecode )
    {
        if ( m_rgdecodeXpress != NULL && m_rgdecodeXpress[idecode] != NULL )
        {
            XpressDecodeRelease_( m_rgdecodeXpress[idecode] );
            m_rgdecodeXpress[idecode] = 0;
        }
#ifdef XPRESS9_COMPRESSION
        if ( m_rgdecodeXpress9 != NULL && m_rgdecodeXpress9[idecode] != NULL )
        {
            Xpress9DecodeRelease_( m_rgdecodeXpress9[idecode] );
            m_rgdecodeXpress9[idecode] = 0;
        }
#endif
    }

    delete[] m_rgencodeXpress;
    m_rgencodeXpress = NULL;
    delete[] m_rgdecodeXpress;
    m_rgdecodeXpress = NULL;
#ifdef XPRESS9_COMPRESSION
    delete[] m_rgencodeXpress9;
    m_rgencodeXpress9 = NULL;
    delete[] m_rgdecodeXpress9;
    m_rgdecodeXpress9 = NULL;
#endif

    m_cbMin = 0;
    m_cbMax = 0;
}

static CDataCompressor g_dataCompressor;

ERR ErrRECScrubLVChunk(
    DATA& data,
    const CHAR chScrub )
{
    return g_dataCompressor.ErrScrub( data, chScrub );
}

ERR ErrPKCompressData(
    const DATA& data,
    const CompressFlags compressFlags,
    const INST* const pinst,
    _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual )
{
    CDataCompressorPerfCounters perfcounters( pinst ? pinst->m_iInstance : 0 );
    
    return g_dataCompressor.ErrCompress( data, compressFlags, &perfcounters, pbDataCompressed, cbDataCompressedMax, pcbDataCompressedActual );
}

ERR ErrPKIDecompressData(
    const DATA& dataCompressed,
    const INST* const pinst,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual )
{
    CDataCompressorPerfCounters perfcounters( pinst ? pinst->m_iInstance : 0 );
    return g_dataCompressor.ErrDecompress( dataCompressed, &perfcounters, pbData, cbDataMax, pcbDataActual );
}

VOID PKIReportDecompressionFailed(
        const IFMP ifmp,
        const PGNO pgno,
        const INT iline,
        const OBJID objidFDP,
        const PGNO pgnoFDP )
{
    INST *pinst = ( ifmp != ifmpNil ) ? PinstFromIfmp( ifmp ) : pinstNil;
    const WCHAR *rgsz[5];
    INT irgsz = 0;

    WCHAR wszpgno[16];
    WCHAR wsziline[16];
    WCHAR wszObjid[16];
    WCHAR wszpgnoFDP[16];

    OSStrCbFormatW( wszpgno, sizeof(wszpgno), L"%d", pgno );
    OSStrCbFormatW( wsziline, sizeof(wsziline), L"%d", iline );
    OSStrCbFormatW( wszObjid, sizeof(wszObjid), L"%u", objidFDP );
    OSStrCbFormatW( wszpgnoFDP, sizeof(wszpgnoFDP), L"%d", pgnoFDP );

    rgsz[irgsz++] = ( ifmp != ifmpNil ) ? g_rgfmp[ifmp].WszDatabaseName() : L"";
    rgsz[irgsz++] = wsziline;
    rgsz[irgsz++] = wszpgno;
    rgsz[irgsz++] = wszObjid;
    rgsz[irgsz++] = wszpgnoFDP;

    UtilReportEvent(
            eventError,
            DATABASE_CORRUPTION_CATEGORY,
            DECOMPRESSION_FAILED,
            irgsz,
            rgsz,
            0,
            NULL,
            pinst );

    if ( pinst != pinstNil )
    {
        OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"55b1156e-8d81-434c-8cdb-2917279e08b5" );
    }
}

ERR ErrPKDecompressData(
    const DATA& dataCompressed,
    const FUCB* const pfucb,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual )
{
    const ERR err = ErrPKIDecompressData(
            dataCompressed,
            pfucb ? PinstFromPfucb( pfucb ) : NULL,
            pbData,
            cbDataMax,
            pcbDataActual );
    if ( err == JET_errDecompressionFailed )
    {
        PKIReportDecompressionFailed(
                pfucb ? pfucb->u.pfcb->Ifmp() : ifmpNil,
                pfucb ? Pcsr( pfucb )->Pgno() : 0,
                pfucb ? Pcsr( pfucb )->ILine() : 0,
                pfucb ? ObjidFDP( pfucb ) : 0,
                pfucb ? PgnoFDP( pfucb ) : 0 );
    }
    return err;
}

ERR ErrPKDecompressData(
    const DATA& dataCompressed,
    const IFMP ifmp,
    const PGNO pgno,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual )
{
    const ERR err = ErrPKIDecompressData(
            dataCompressed,
            ( ifmp != ifmpNil ) ? PinstFromIfmp( ifmp ) : NULL,
            pbData,
            cbDataMax,
            pcbDataActual );
    if ( err == JET_errDecompressionFailed )
    {
        PKIReportDecompressionFailed( ifmp, pgno, -1, 0, 0 );
    }
    return err;
}

ERR ErrPKAllocAndDecompressData(
    const DATA& dataCompressed,
    const FUCB* const pfucb,
    _Outptr_result_bytebuffer_( *pcbDataActual )  BYTE ** const ppbData,
    _Out_ INT * const pcbDataActual )
{
    ERR err = JET_errSuccess;

    *ppbData = NULL;
    *pcbDataActual = 0;

    INT cbData;
    Call( ErrPKDecompressData( dataCompressed, pfucb, NULL, 0, &cbData ) );
    Alloc( *ppbData = new BYTE[cbData] );
    Call( ErrPKDecompressData( dataCompressed, pfucb, *ppbData, cbData, pcbDataActual ) );
    Assert( JET_wrnBufferTruncated != err );
    AssumePREFAST( *pcbDataActual == cbData );

HandleError:
    if( err < JET_errSuccess )
    {
        delete[] *ppbData;
        *ppbData = NULL;
        *pcbDataActual = 0;
    }
    return err;
}

BYTE * PbPKAllocCompressionBuffer()
{
    return g_compressionBufferCache.PbAlloc();
}

INT CbPKCompressionBuffer()
{
    return g_compressionBufferCache.CbBufferSize();
}

VOID PKFreeCompressionBuffer( _Post_ptr_invalid_ BYTE * const pb )
{
    g_compressionBufferCache.Free(pb);
}

ERR ErrPKInitCompression( const INT cbPage, const INT cbCompressMin, const INT cbCompressMax )
{
    ERR err = JET_errSuccess;
    CallJ( g_compressionBufferCache.ErrInit( cbPage ), Term );
    CallJ( g_dataCompressor.ErrInit( cbCompressMin, cbCompressMax ), TermBufferCache );
    return err;

TermBufferCache:
    g_compressionBufferCache.Term();

Term:
    return err;
}

void PKTermCompression()
{
#ifdef XPRESS10_COMPRESSION
    Xpress10CorsicaTerm();
    g_CorsicaInitOnce.Reset();
#endif
    g_dataCompressor.Term();
    g_compressionBufferCache.Term();
}

#ifdef ENABLE_JET_UNIT_TEST

class TestCompressorStats : public IDataCompressorStats
{
    public:
        TestCompressorStats() :
            IDataCompressorStats(),
            m_cbUncompressed( 0 ),
            m_cbCompressed( 0 ),
            m_cbDecompression( 0 ),
            m_cCompressionCalls( 0 ),
            m_cDecompressionCalls( 0 ),
            m_dhrtsCompression( 0 ),
            m_dhrtsDecompression( 0 )
        {
        }
        ~TestCompressorStats() {}

        void AddUncompressedBytes( const INT cb ) { m_cbUncompressed = cb; }
        void AddCompressedBytes( const INT cb ) { m_cbCompressed = cb; }
        void IncCompressionCalls() { m_cCompressionCalls++; }
        void AddCompressionDhrts( const QWORD dhrts ) { m_dhrtsCompression = dhrts; }
        void AddDecompressionBytes( const INT cb ) { m_cbDecompression = cb; }
        void IncDecompressionCalls() { m_cDecompressionCalls++; }
        void AddDecompressionDhrts( const QWORD dhrts ) { m_dhrtsDecompression = dhrts; }

        void AddCpuXpress9DecompressionBytes( const INT cb ) { m_cbDecompression = cb; }
        void IncCpuXpress9DecompressionCalls() { m_cDecompressionCalls++; }
        void AddCpuXpress9DecompressionDhrts( const QWORD dhrts ) { m_dhrtsDecompression = dhrts; }

        void AddXpress10SoftwareDecompressionBytes( const INT cb ) { m_cbDecompression = cb; }
        void IncXpress10SoftwareDecompressionCalls() { m_cDecompressionCalls++; }
        void AddXpress10SoftwareDecompressionDhrts( const QWORD dhrts ) { m_dhrtsDecompression = dhrts; }

        void AddXpress10CorsicaCompressionBytes( const INT cb ) { m_cbUncompressed = cb; }
        void AddXpress10CorsicaCompressedBytes( const INT cb ) { m_cbCompressed = cb; }
        void IncXpress10CorsicaCompressionCalls() { m_cCompressionCalls++; }
        void AddXpress10CorsicaCompressionDhrts( const QWORD dhrts ) { m_dhrtsCompression = dhrts; }
        void AddXpress10CorsicaCompressionHardwareDhrts( const QWORD dhrts ) { m_dhrtsCompression = dhrts; }
        void AddXpress10CorsicaDecompressionBytes( const INT cb ) { m_cbDecompression = cb; }
        void IncXpress10CorsicaDecompressionCalls() { m_cDecompressionCalls++; }
        void AddXpress10CorsicaDecompressionDhrts( const QWORD dhrts ) { m_dhrtsDecompression = dhrts; }
        void AddXpress10CorsicaDecompressionHardwareDhrts( const QWORD dhrts ) { m_dhrtsDecompression = dhrts; }

        INT m_cbUncompressed;
        INT m_cbCompressed;
        INT m_cbDecompression;
        INT m_cCompressionCalls;
        INT m_cDecompressionCalls;
        QWORD m_dhrtsCompression;
        QWORD m_dhrtsDecompression;
};
    
JETUNITTEST( CDataCompressor, 7BitAscii )
{
    CDataCompressor compressor;
    TestCompressorStats stats;
    
    ERR err;
    const INT cbBuf = 256;
    BYTE rgbBuf1[cbBuf];
    BYTE rgbBuf2[cbBuf];
    INT cbDataActual;
    DATA data;
    char * sz;

    CHECK( JET_errSuccess == compressor.ErrInit( 1024, 4096 ) );

    sz = "foo";
    data.SetPv( sz );
    data.SetCb( strlen(sz) );
    err = compressor.ErrCompress( data, compress7Bit, &stats, rgbBuf1, cbBuf, &cbDataActual );
    CHECK( errRECCannotCompress == err );

    sz = "1234567890ABCDEF";
    data.SetPv( sz );
    data.SetCb( strlen(sz) );
    err = compressor.ErrCompress( data, compress7Bit, &stats, rgbBuf1, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( 15 == cbDataActual );

    data.SetPv( rgbBuf1 );
    data.SetCb( cbDataActual );
    err = compressor.ErrDecompress( data, &stats, rgbBuf2, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( 16 == cbDataActual );
    CHECK( 0 == memcmp( sz, rgbBuf2, 16 ) );

    data.SetPv( rgbBuf1 );
    data.SetCb( 15 );
    err = compressor.ErrDecompress( data, &stats, rgbBuf2, 1, &cbDataActual );
    CHECK( JET_wrnBufferTruncated == err );
    CHECK( 16 == cbDataActual );
    CHECK( rgbBuf2[0] == sz[0] );

    compressor.Term();
}

JETUNITTEST( CDataCompressor, 7BitUnicode )
{
    CDataCompressor compressor;
    TestCompressorStats stats;
    
    ERR err;
    const INT cbBuf = 256;
    BYTE rgbBuf1[cbBuf];
    BYTE rgbBuf2[cbBuf];
    INT cbDataActual;
    DATA data;
    wchar_t * sz;

    CHECK( JET_errSuccess == compressor.ErrInit( 1024, 8192 ) );

    sz = L"f";
    data.SetPv( sz );
    data.SetCb( wcslen(sz) );
    err = compressor.ErrCompress( data, compress7Bit, &stats, rgbBuf1, cbBuf, &cbDataActual );
    CHECK( errRECCannotCompress == err );

    sz = L"12";
    data.SetPv( sz );
    data.SetCb( wcslen(sz)*sizeof(sz[0]) );
    err = compressor.ErrCompress( data, compress7Bit, &stats, rgbBuf1, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( 3 == cbDataActual );

    data.SetPv( rgbBuf1 );
    data.SetCb( cbDataActual );
    err = compressor.ErrDecompress( data, &stats, rgbBuf2, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( 4 == cbDataActual );
    CHECK( 0 == memcmp( sz, rgbBuf2, 4 ) );

    data.SetPv( rgbBuf1 );
    data.SetCb( 3 );
    err = compressor.ErrDecompress( data, &stats, rgbBuf2, 1, &cbDataActual );
    CHECK( JET_wrnBufferTruncated == err );
    CHECK( 4 == cbDataActual );
    CHECK( rgbBuf2[0] == ((BYTE*)sz)[0] );

    compressor.Term();
}

#pragma push_macro("CHECK")
#undef CHECK
#define CHECK Enforce

void DataCompressorBasic( const CompressFlags compressFlags, BYTE* rgbBuf = NULL, INT cbBuf = 2048 )
{
    CDataCompressor compressor;
    TestCompressorStats stats;

    ERR err;
    BYTE* rgbBufCompressed = (BYTE*) alloca( cbBuf );
    BYTE* rgbBufDecompressed =  (BYTE*) alloca( cbBuf );
    INT cbDataActual;
    DATA data;

    if ( rgbBuf == NULL )
    {
        rgbBuf = (BYTE*) alloca( cbBuf );
        memset( rgbBuf, chRECCompressTestFill, cbBuf );
    }

    CHECK( JET_errSuccess == compressor.ErrInit( 1024, 8192 ) );

    data.SetPv( rgbBuf );
    data.SetCb( cbBuf );
    err = compressor.ErrCompress( data, compressFlags, &stats, rgbBufCompressed, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( cbDataActual < cbBuf );

    data.SetPv( rgbBufCompressed );
    data.SetCb( cbDataActual );

    err = compressor.ErrDecompress( data, &stats, rgbBufDecompressed, cbBuf - 1, &cbDataActual );
    
    CHECK( JET_wrnBufferTruncated == err );
    CHECK( cbBuf == cbDataActual );

    err = compressor.ErrDecompress( data, &stats, rgbBufDecompressed, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( cbBuf == cbDataActual );
    CHECK( 0 == memcmp( rgbBuf, rgbBufDecompressed, cbDataActual ) );

    rgbBufDecompressed[0] = 0;
    err = compressor.ErrDecompress( data, &stats, rgbBufDecompressed, 1, &cbDataActual );
    CHECK( JET_wrnBufferTruncated == err );
    CHECK( cbBuf == cbDataActual );
    CHECK( rgbBuf[0] == rgbBufDecompressed[0] );

    compressor.Term();
}

#pragma pop_macro("CHECK")

JETUNITTEST( CDataCompressor, Xpress )
{
    DataCompressorBasic( compressXpress );
}

#ifdef XPRESS9_COMPRESSION
JETUNITTEST( CDataCompressor, Xpress9Cpu )
{
    DataCompressorBasic( compressXpress9 );
}
#endif

#ifdef XPRESS10_COMPRESSION
JETUNITTEST( CDataCompressor, Xpress10 )
{
    CHECK( JET_errSuccess == ErrPKInitCompression( 32*1024, 1024, 32*1024 ) );
    g_fAllowXpress10SoftwareCompression = fTrue;
    DataCompressorBasic( compressXpress10 );
    DataCompressorBasic( compressXpress10, NULL, 8150 );
    g_fAllowXpress10SoftwareCompression = fFalse;
    PKTermCompression();
}

JETUNITTEST( CDataCompressor, Xpress10FallbackXpress )
{
    CHECK( JET_errSuccess == ErrPKInitCompression( 32*1024, 1024, 32*1024 ) );
    DataCompressorBasic( CompressFlags( compressXpress10 | compressXpress ) );
    DataCompressorBasic( CompressFlags( compressXpress10 | compressXpress ), NULL, 8150 );
    PKTermCompression();
}
#endif

JETUNITTEST( CDataCompressor, XpressThreshold )
{
    CDataCompressor compressor;
    TestCompressorStats stats;
    
    const INT cbBuf = 2048;
    const INT cbMinDataForXpress = 64;

    ERR err;
    BYTE rgbBufOrig[cbBuf];
    BYTE rgbBufCompressed[cbBuf];
    DATA data;
    INT cbDataActual;

    CHECK( JET_errSuccess == compressor.ErrInit( cbMinDataForXpress, 8192 ) );

    memset( rgbBufOrig, chRECCompressTestFill, cbBuf );
    data.SetPv( rgbBufOrig );

    data.SetCb( cbMinDataForXpress );
    err = compressor.ErrCompress( data, compressXpress, &stats, rgbBufCompressed, sizeof(rgbBufCompressed), &cbDataActual );
    CHECK( JET_errSuccess == err );

    data.SetCb( cbMinDataForXpress - 1 );
    err = compressor.ErrCompress( data, compressXpress, &stats, rgbBufCompressed, sizeof(rgbBufCompressed), &cbDataActual );
    CHECK( errRECCannotCompress == err );

    compressor.Term();
}

JETUNITTEST( CDataCompressor, Scrub )
{
    CDataCompressor compressor;
    TestCompressorStats stats;

    ERR err;
    const INT cbBuf = 2048;
    BYTE rgbBufOrig[cbBuf];
    BYTE rgbBufScrubbed[cbBuf];
    DATA data;

    CHECK( JET_errSuccess == compressor.ErrInit( 1024, 2048 ) );

    memset( rgbBufOrig, 0, cbBuf );
    memset( rgbBufScrubbed, chSCRUBDBMaintLVChunkFill, cbBuf );
    data.SetPv( rgbBufOrig );
    data.SetCb( cbBuf );
    err = compressor.ErrScrub( data, chSCRUBDBMaintLVChunkFill );
    CHECK( JET_errSuccess == err );
    CHECK( 0 == memcmp( rgbBufOrig + 1, rgbBufScrubbed, cbBuf - 1 ) );

    INT cbDataActual = INT_MAX;
    UtilMemCpy( rgbBufScrubbed, rgbBufOrig, cbBuf );
    FNegTestSet( fInvalidUsage );
    err = compressor.ErrDecompress( data, &stats, NULL, 0, &cbDataActual );
    FNegTestUnset( fInvalidUsage );
    CHECK( wrnRECCompressionScrubDetected == err );
    CHECK( 0 == cbDataActual );
    CHECK( 0 == memcmp( rgbBufOrig, rgbBufScrubbed, cbBuf ) );

    memset( rgbBufOrig, 0, cbBuf );
    data.SetPv( rgbBufOrig );
    data.SetCb( 1 );
    err = compressor.ErrScrub( data, chSCRUBDBMaintLVChunkFill );
    CHECK( JET_errSuccess == err );

    cbDataActual = INT_MAX;
    UtilMemCpy( rgbBufScrubbed, rgbBufOrig, cbBuf );
    FNegTestSet( fInvalidUsage );
    err = compressor.ErrDecompress( data, &stats, NULL, 0, &cbDataActual );
    FNegTestUnset( fInvalidUsage );
    CHECK( wrnRECCompressionScrubDetected == err );
    CHECK( 0 == cbDataActual );
    CHECK( 0 == memcmp( rgbBufOrig, rgbBufScrubbed, cbBuf ) );

    compressor.Term();
}

#pragma push_macro("CHECK")
#undef CHECK
#define CHECK Enforce

void DataCompressorEfficiencyCases( const CompressFlags compressFlags )
{
    CDataCompressor compressor;
    TestCompressorStats stats;
    
    ERR err;
    const INT cbBuf = 4096;
    BYTE rgbBuf1[cbBuf];
    BYTE rgbBuf2[cbBuf];
    INT cbDataActual;
    DATA data;

    CHECK( JET_errSuccess == compressor.ErrInit( 1024, 4096 ) );

    memset( rgbBuf1, 0xEE, cbBuf );
    data.SetPv( rgbBuf1 );
    data.SetCb( cbBuf );
    err = compressor.ErrCompress( data, compressFlags, &stats, rgbBuf2, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( cbDataActual < cbBuf );

    for( INT ib = 0; ib < cbBuf; ++ib )
    {
        rgbBuf1[ib] = ib % 256;
    }
    for( INT ib = 0; ib < cbBuf-1; ++ib )
    {
        const INT cbRemaining = cbBuf - ib;
        const INT ibSwap = ib + ( rand() % cbRemaining );
        const BYTE bSwap = rgbBuf1[ibSwap];
        rgbBuf1[ibSwap] = rgbBuf1[ib];
        rgbBuf1[ib] = bSwap;
    }

    data.SetPv( rgbBuf1 );
    data.SetCb( cbBuf );
    err = compressor.ErrCompress( data, compressFlags, &stats, rgbBuf2, cbBuf, &cbDataActual );
    CHECK( errRECCannotCompress == err );

    memset( rgbBuf1, 0xCC, cbBuf/8 );
    
    data.SetPv( rgbBuf1 );
    data.SetCb( cbBuf );
    err = compressor.ErrCompress( data, compressFlags, &stats, rgbBuf2, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( cbDataActual < cbBuf );

    compressor.Term();
}

#pragma pop_macro("CHECK")

JETUNITTEST( CDataCompressor, XpressEfficiencyCases )
{
    DataCompressorEfficiencyCases( compressXpress );
}

#ifdef XPRESS9_COMPRESSION
JETUNITTEST( CDataCompressor, Xpress9CpuEfficiencyCases )
{
    DataCompressorEfficiencyCases( compressXpress9 );
}
#endif

#ifdef XPRESS10_COMPRESSION
JETUNITTEST( CDataCompressor, Xpress10EfficiencyCases )
{
    CHECK( JET_errSuccess == ErrPKInitCompression( 32*1024, 1024, 32*1024 ) );
    g_fAllowXpress10SoftwareCompression = fTrue;
    DataCompressorEfficiencyCases( compressXpress10 );
    g_fAllowXpress10SoftwareCompression = fFalse;
    PKTermCompression();
}
#endif

JETUNITTEST( CCompressionBufferCache, Basic )
{
    CCompressionBufferCache cache;

    CHECK( JET_errSuccess == cache.ErrInit( 1024 ) );
    CHECK( 1024 == cache.CbBufferSize() );

    BYTE * const pb1 = cache.PbAlloc();
    CHECK( NULL != pb1 );
    memset( pb1, 0, cache.CbBufferSize() );
    cache.Free(pb1);

    BYTE * const pb2 = cache.PbAlloc();
    CHECK( pb1 == pb2 );
    memset( pb2, 0xFF, cache.CbBufferSize() );

    BYTE * const pb3 = cache.PbAlloc();
    CHECK( pb2 != pb3 );
    
    cache.Free(pb2);
    cache.Free(pb3);
    
    cache.Term();
}

#endif

