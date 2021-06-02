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

//  ================================================================
class IDataCompressorStats
//  ================================================================
//
//  Abstract class passed into the compression routine to collect performance
//  statistics.
//
//-
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

//  ================================================================
class CDataCompressorPerfCounters : public IDataCompressorStats
//  ================================================================
//
//  Updates perf counters with the passed-in statistics.
//
//-
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

//  ================================================================
CDataCompressorPerfCounters::CDataCompressorPerfCounters( const INT iInstance ) :
//  ================================================================
    IDataCompressorStats(),
    m_iInstance( iInstance )
{
}

//  ================================================================
CDataCompressorPerfCounters::~CDataCompressorPerfCounters()
//  ================================================================
{
}

//  ================================================================
void CDataCompressorPerfCounters::AddUncompressedBytes( const INT cb )
//  ================================================================
{
    PERFOpt( CHECK_INST( s_cbUncompressedBytes.Add( m_iInstance, cb ) ) );
}

//  ================================================================
void CDataCompressorPerfCounters::AddCompressedBytes( const INT cb )
//  ================================================================
{
    PERFOpt( CHECK_INST( s_cbCompressedBytes.Add( m_iInstance, cb ) ) );
}

//  ================================================================
void CDataCompressorPerfCounters::IncCompressionCalls()
//  ================================================================
{
    PERFOpt( CHECK_INST( s_cCompressionCalls.Inc( m_iInstance ) ) );
}

//  ================================================================
void CDataCompressorPerfCounters::AddCompressionDhrts( const QWORD dhrts )
//  ================================================================
{
    PERFOpt( CHECK_INST( s_cCompressionTotalDhrts.Add( m_iInstance, dhrts ) ) );
}

//  ================================================================
void CDataCompressorPerfCounters::AddDecompressionBytes( const INT cb )
//  ================================================================
{
    PERFOpt( CHECK_INST( s_cbDecompressionBytes.Add( m_iInstance, cb ) ) );
}

//  ================================================================
void CDataCompressorPerfCounters::IncDecompressionCalls()
//  ================================================================
{
    PERFOpt( CHECK_INST( s_cDecompressionCalls.Inc( m_iInstance ) ) );
}

//  ================================================================
void CDataCompressorPerfCounters::AddDecompressionDhrts( const QWORD dhrts )
//  ================================================================
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

#endif // PERFMON_SUPPORT

//  ================================================================
class CCompressionBufferCache
    //  ================================================================
    //
    //  To avoid constantly allocating and freeing buffers to compress/decompress
    //  data we cache them here.
    //  
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

//  ================================================================
CCompressionBufferCache::CCompressionBufferCache() :
    //  ================================================================
    m_cbBufferSize( 0 ),
    m_cpbCachedCompressionBuffers( 0 ),
    m_rgpbCachedCompressionBuffers( NULL )
{
}

//  ================================================================
CCompressionBufferCache::~CCompressionBufferCache()
//  ================================================================
{
}

//  ================================================================
INT CCompressionBufferCache::CbBufferSize() const
//  ================================================================
{
    return m_cbBufferSize;
}

//  ================================================================
ERR CCompressionBufferCache::ErrInit( const INT cbBufferSize )
//  ================================================================
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

//  ================================================================
void CCompressionBufferCache::Term()
//  ================================================================
{
    for ( INT ipb = 0; ipb < m_cpbCachedCompressionBuffers; ++ipb )
    {
        delete[] m_rgpbCachedCompressionBuffers[ ipb ];
    }

    delete[] m_rgpbCachedCompressionBuffers;
    m_rgpbCachedCompressionBuffers = NULL;
    m_cbBufferSize = 0;
}

//  ================================================================
BYTE * CCompressionBufferCache::PbAlloc()
//  ================================================================
{
    BYTE * pb = GetCachedPtr<BYTE *>( m_rgpbCachedCompressionBuffers, m_cpbCachedCompressionBuffers );
    if ( NULL == pb )
    {
        pb = new BYTE[ CbBufferSize() ];
    }
    return pb;
}

//  ================================================================
void CCompressionBufferCache::Free( BYTE * const pb )
//  ================================================================
{
    if ( pb && !FCachePtr<BYTE *>( pb, m_rgpbCachedCompressionBuffers, m_cpbCachedCompressionBuffers ) )
    {
        delete[] pb;
    }
}

// This is a global instance of the LV compression buffer cache, which can be used by the entire process
static CCompressionBufferCache g_compressionBufferCache;

//  ================================================================
class CDataCompressor
//  ================================================================
//
//  A class that contains compress/decompress routines
//
//-
{
    public:
        CDataCompressor();
        ~CDataCompressor();

        ERR ErrInit( const INT cbMin, const INT cbMax );
        void Term();                        // frees the cached compression/decompression workspaces
        
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
        // PERSISTED
        enum COMPRESSION_SCHEME
            {
                COMPRESS_NONE,
                COMPRESS_7BITASCII = 0x1,
                COMPRESS_7BITUNICODE = 0x2,
                COMPRESS_XPRESS = 0x3,
                COMPRESS_SCRUB = 0x4,
                COMPRESS_XPRESS9 = 0x5,
                COMPRESS_XPRESS10 = 0x6,
                COMPRESS_MAXIMUM = 0x1f, // We only have 5 bits due to 3 bits used by 7 bit compression
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
            // SSE 4.2 compatible CRC of the uncompressed data
            UnalignedLittleEndian<ULONG>        mle_ulUncompressedChecksum;
            // Corsica compatible CRC of the compressed data
            UnalignedLittleEndian<ULONGLONG>    mle_ullCompressedChecksum;
        };
#include <poppack.h>

    private:
        // Xpress (legacy) compression level, ranging from 0 (fastest) to 9 (smallest)
        static const INT xpressLegacyCompressionLevel = 2;

        // if xpress compression is attempted and shrinks the data by less than
        // this amount we consider the effort wasted and report it
        static const INT pctCompressionWastedEffort = 10;

        // cached encoders/decoders
        INT m_cencodeCachedMax;
        INT m_cdecodeCachedMax;

        XpressEncodeStream* m_rgencodeXpress;
        XpressDecodeStream* m_rgdecodeXpress;
#ifdef XPRESS9_COMPRESSION
        XPRESS9_ENCODER* m_rgencodeXpress9;
        XPRESS9_DECODER* m_rgdecodeXpress9;
#endif

        // minimum practical size threshold for compression (not including 7 bit compression)
        INT m_cbMin;

        // maximum size of data that will be compressed (may inform compression window)
        INT m_cbMax;

        // get/free Xpress (legacy) encoders/decoders
        ERR ErrXpressEncodeOpen_( _Out_ XpressEncodeStream * const pencode );
        void XpressEncodeClose_( XpressEncodeStream encode );
        void XpressEncodeRelease_( XpressEncodeStream encode );
        ERR ErrXpressDecodeOpen_( _Out_ XpressDecodeStream * const pdecode );
        void XpressDecodeClose_( XpressDecodeStream decode );
        void XpressDecodeRelease_( XpressDecodeStream decode );

#ifdef XPRESS9_COMPRESSION
        // get/free Xpress 9 encoders/decoders
        ERR ErrXpress9EncodeOpen_( _Out_ XPRESS9_ENCODER * const pencode );
        void Xpress9EncodeClose_( XPRESS9_ENCODER encode );
        void Xpress9EncodeRelease_( XPRESS9_ENCODER encode );
        ERR ErrXpress9DecodeOpen_( _Out_ XPRESS9_DECODER * const pdecode );
        void Xpress9DecodeClose_( XPRESS9_DECODER decode );
        void Xpress9DecodeRelease_( XPRESS9_DECODER decode );
#endif

        // user-supplied callback function that allocates memory
        // if there is no memory available it shall return NULL
        static void * XPRESS_CALL PvXpressAlloc_(
            _In_opt_ void * pvContext,  // user-defined context (as passed to XpressEncodeCreate)
            INT             cbAlloc );  // size of memory block to allocate (bytes)

        // user-supplied callback function that releases memory
        static void XPRESS_CALL XpressFree_(
            _In_opt_ void *             pvContext,      // user-defined context (as passed to XpressEncodeClose)
            _Post_ptr_invalid_ void *   pvAlloc );      // pointer to the block to be freed

    private:
        // calculate the size of compressed data
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

//  ================================================================
CDataCompressor::CDataCompressor() :
//  ================================================================
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

//  ================================================================
CDataCompressor::~CDataCompressor()
//  ================================================================
{
    // Asserting that Term() has been called doesn't always work for
    // global objects because the destructor is called if the DLL
    // is unloaded because of a crash.
}

//  ================================================================
void * XPRESS_CALL CDataCompressor::PvXpressAlloc_( _In_opt_ void * pvContext, INT cbAlloc )
//  ================================================================
{
    return new BYTE[cbAlloc];
}

//  ================================================================
void XPRESS_CALL CDataCompressor::XpressFree_( _In_opt_ void * pvContext, _Post_ptr_invalid_ void * pvAlloc )
//  ================================================================
{
    delete [] pvAlloc;
}

//  ================================================================
ERR CDataCompressor::ErrXpressEncodeOpen_( _Out_ XpressEncodeStream * const pencode )
//  ================================================================
{
    C_ASSERT( sizeof(void*) == sizeof(XpressEncodeStream) );
    *pencode = 0;

    XpressEncodeStream encode = GetCachedPtr<XpressEncodeStream>( m_rgencodeXpress, m_cencodeCachedMax );
    if ( 0 != encode )
    {
        // we found a cached XpressEncodeStream
        *pencode = encode;
        return JET_errSuccess;
    }

    // no cached XpressEncodeStream was found. create one
    encode = XpressEncodeCreate(
                    m_cbMax,                        // max size of data to compress
                    0,                              // user defined context info (passed to allocfn)
                    PvXpressAlloc_,                 // user allocfn
                    xpressLegacyCompressionLevel ); // compression quality - 0 fastest, 9 best
    if( NULL == encode )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    *pencode = encode;
    return JET_errSuccess;
}

//  ================================================================
void CDataCompressor::XpressEncodeClose_( XpressEncodeStream encode )
//  ================================================================
{
    if ( encode )
    {
        if( FCachePtr<XpressEncodeStream>( encode, m_rgencodeXpress, m_cencodeCachedMax ) )
        {
            // we cached the XpressEncodeStream in an empty slot
            return;
        }

        // we didn't find an empty slot, free the XpressEncodeStream        
        XpressEncodeRelease_( encode );
    }
}

//  ================================================================
void CDataCompressor::XpressEncodeRelease_( XpressEncodeStream encode )
//  ================================================================
{
    if ( encode )
    {
        XpressEncodeClose( encode, 0, XpressFree_ );
    }
}

//  ================================================================
ERR CDataCompressor::ErrXpressDecodeOpen_( _Out_ XpressDecodeStream * const pdecode )
//  ================================================================
{
    C_ASSERT( sizeof(void*) == sizeof(XpressDecodeStream) );
    
    *pdecode = 0;

    XpressDecodeStream decode = GetCachedPtr<XpressDecodeStream>( m_rgdecodeXpress, m_cdecodeCachedMax );
    if( 0 != decode )
    {
        // found a cached XpressDecodeStream
        *pdecode = decode;
        return JET_errSuccess;
    }

    // no cached XpressDecodeStream was found. create one
    decode = XpressDecodeCreate(
                    0,                  // user defined context info (passed to allocfn)
                    PvXpressAlloc_ );   // user allocfn
    if (NULL == decode)
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    *pdecode = decode;
    return JET_errSuccess;
}

//  ================================================================
void CDataCompressor::XpressDecodeClose_( XpressDecodeStream decode )
//  ================================================================
{
    if ( decode )
    {
        if( FCachePtr<XpressDecodeStream>( decode, m_rgdecodeXpress, m_cdecodeCachedMax ) )
        {
            // we cached the XpressDecodeStream in an empty slot
            return;
        }

        // we didn't find an empty slot, free the XpressDecodeStream        
        XpressDecodeRelease_( decode );
    }
}

//  ================================================================
void CDataCompressor::XpressDecodeRelease_( XpressDecodeStream decode )
//  ================================================================
{
    if ( decode )
    {
        XpressDecodeClose( decode, 0, XpressFree_ );
    }
}

#ifdef XPRESS9_COMPRESSION
//  ================================================================
ERR CDataCompressor::ErrXpress9EncodeOpen_( _Out_ XPRESS9_ENCODER * const pencode )
//  ================================================================
{
    C_ASSERT( sizeof( void* ) == sizeof( XPRESS9_ENCODER ) );

    ERR err = JET_errSuccess;
    XPRESS9_STATUS status = { 0 };

    *pencode = 0;

    XPRESS9_ENCODER encode = GetCachedPtr<XPRESS9_ENCODER>( m_rgencodeXpress9, m_cencodeCachedMax );
    if ( 0 == encode )
    {
        // no cached XPRESS9_ENCODER was found. create one
        encode = Xpress9EncoderCreate( &status, NULL, PvXpressAlloc_, XPRESS9_WINDOW_SIZE_LOG2_MIN, Xpress9Flag_UseSSE2 );
        Call( ErrXpress9StatusToJetErr( status ) );
    }

    // return the XPRESS9_ENCODER
    *pencode = encode;

HandleError:
    if ( err < JET_errSuccess )
    {
        Xpress9EncodeRelease_( encode );
    }
    return err;
}

//  ================================================================
void CDataCompressor::Xpress9EncodeClose_( XPRESS9_ENCODER encode )
//  ================================================================
{
    if ( encode )
    {
        if ( FCachePtr<XPRESS9_ENCODER>( encode, m_rgencodeXpress9, m_cencodeCachedMax ) )
        {
            // we cached the XPRESS9_ENCODER in an empty slot
            return;
        }

        // we didn't find an empty slot, free the XPRESS9_ENCODER
        Xpress9EncodeRelease_( encode );
    }
}

//  ================================================================
void CDataCompressor::Xpress9EncodeRelease_( XPRESS9_ENCODER encode )
//  ================================================================
{
    if ( encode )
    {
        XPRESS9_STATUS status = { 0 };
        Xpress9EncoderDestroy( &status, encode, NULL, XpressFree_ );
        Assert( status.m_uStatus == Xpress9Status_OK );
    }
}

//  ================================================================
ERR CDataCompressor::ErrXpress9DecodeOpen_( _Out_ XPRESS9_DECODER * const pdecode )
//  ================================================================
{
    C_ASSERT( sizeof( void* ) == sizeof( XPRESS9_DECODER ) );

    ERR err = JET_errSuccess;
    XPRESS9_STATUS status = { 0 };

    *pdecode = 0;

    XPRESS9_DECODER decode = GetCachedPtr<XPRESS9_DECODER>( m_rgdecodeXpress9, m_cdecodeCachedMax );
    if ( 0 == decode )
    {
        // no cached XPRESS9_DECODER was found. create one
        decode = Xpress9DecoderCreate( &status, NULL, PvXpressAlloc_, XPRESS9_WINDOW_SIZE_LOG2_MIN, Xpress9Flag_UseSSE2 );
        Call( ErrXpress9StatusToJetErr( status ) );
    }

    // return the XPRESS9_DECODER
    *pdecode = decode;

HandleError:
    if ( err < JET_errSuccess )
    {
        Xpress9DecodeRelease_( decode );
    }
    return err;
}

//  ================================================================
void CDataCompressor::Xpress9DecodeClose_( XPRESS9_DECODER decode )
//  ================================================================
{
    if ( decode )
    {
        if ( FCachePtr<XPRESS9_DECODER>( decode, m_rgdecodeXpress9, m_cdecodeCachedMax ) )
        {
            // we cached the XPRESS9_DECODER in an empty slot
            return;
        }

        // we didn't find an empty slot, free the XPRESS9_DECODER
        Xpress9DecodeRelease_( decode );
    }
}

//  ================================================================
void CDataCompressor::Xpress9DecodeRelease_( XPRESS9_DECODER decode )
//  ================================================================
{
    if ( decode )
    {
        XPRESS9_STATUS status = { 0 };
        Xpress9DecoderDestroy( &status, decode, NULL, XpressFree_ );
        Assert( status.m_uStatus == Xpress9Status_OK );
    }
}
#endif // XPRESS9_COMPRESSION

//  ================================================================
INT CDataCompressor::CbCompressed7BitAscii_( const INT cb )
//  ================================================================
{
    // calculate the size as:
    //  number of input characters(data size) * bits-per-character(7) + header byte
    // remember to round _up_
    const INT cbCompressed = ( ((cb*7) + 7) / 8 ) + 1;
    return cbCompressed;
}

//  ================================================================
INT CDataCompressor::CbCompressed7BitUnicode_( const INT cb )
//  ================================================================
{
    // calculate the size as:
    //  number of input characters(data size) * bits-per-character(7) + header byte
    // remember to round _up_
    const INT cbCompressed = ( (((cb/sizeof(WORD))*7) + 7) / 8 ) + 1;
    return cbCompressed;
}

//  ================================================================
CDataCompressor::COMPRESSION_SCHEME CDataCompressor::Calculate7BitCompressionScheme_( const DATA& data, BOOL fUnicodeOnly )
//  ================================================================
//
//  7-bit Unicode (56% savings) is preferable to 7-bit ASCII (12.5% savings)
//  If 7-bit ASCII is impossible then 7-bit Unicode is impossible as well,
//  so we know no compression is possible at that point.
//
//-
{
    bool fUnicodePossible = ( 0 == data.Cb() % sizeof(WORD) );
    if ( fUnicodeOnly && !fUnicodePossible )
    {
        return COMPRESS_NONE;
    }

    // the 7-bit compression schemes can only be applied if all the data
    // consists of characters which are less than 0x80. use these masks
    // to quickly determine if there are any characters which have their
    // high bits set
    const DWORD dwMaskAscii     = fUnicodeOnly ? 0x007f007f : 0x7f7f7f7f;
    const DWORD dwMaskUnicode   = 0x007f007f;

    // first process by DWORD until we have less than one DWORD of
    // data remaining
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

    // now process the remaining bytes  
    const BYTE * const  pb = (BYTE *)data.Pv();
    Assert( ( data.Cb() - (cdw * sizeof(DWORD)) ) < sizeof(DWORD) );
    for( INT ib = cdw * sizeof(DWORD); ib < data.Cb(); ++ib )
    {
        if( pb[ib] > 0x7f )
        {
            return COMPRESS_NONE;
        }
        // for Unicode we only check each WORD
        // (if the data length isn't a multiple of sizeof(WORD)
        // then fUnicodePossible is set to false at the top of
        // this function)
        if( fUnicodePossible && ( 0 == ( ib % sizeof(WORD) ) ) )
        {
            const Unaligned<WORD> * const pw = (Unaligned<WORD> *)(pb + ib);
            if( *pw > 0x007f )
            {
                fUnicodePossible = false;
            }
        }
    }

    // prefer 7-bit Unicode to 7-bit ASCII
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

// This is the same idea as ReverseEightBytes, but it operates with eight 7-bit blocks, instead of eight 8-bit bytes.
// The bit values of the masks used below are:
//
// qwMask1
// 00000000 00000001 11111100 00000111 11111111 11100000 00000000 01111111
// Mask2
// 00000000 11111110 00000011 11111000 00001111 11100000 00111111 10000000
//
// qwMask3
// 00000000 00000000 00000011 11111111 11110000 00000000 00111111 11111111
// qwMask4
// 00000000 11111111 11111100 00000000 00001111 11111111 11000000 00000000
// 
// qwMask5
// 00000000 00000000 00000000 00000000 00001111 11111111 11111111 11111111
// qwMask6
// 00000000 11111111 11111111 11111111 11110000 00000000 00000000 00000000
//
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

//  ================================================================
ERR CDataCompressor::ErrCompress7BitAscii_(
    const DATA& data,
    _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    _In_ const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual )
//  ================================================================
//
//  In this scheme we remove the top bit (Which is always 0)
//  and store only the bottom 7-bits of each BYTE in the output array
//
//  This compression format is PERSISTED
//
//-
{
    *pcbDataCompressedActual = 0;

    const INT cbData = data.Cb();

    const INT cbCompressed = CbCompressed7BitAscii_(cbData);
    if( cbCompressed > cbDataCompressedMax )
    {
        return ErrERRCheck( JET_errBufferTooSmall );
    }

    // byte 0 is the header byte
    INT ibOutput    = 1;
    INT ibInput     = 0;

    INT cqwOutputCheck = 0;
    INT cdwOutputCheck = 0;
    INT cbOutputCheck = 0;

    // first do fast 64-bit compression. we can only do this if all 64-bits of the input data are needed
    // one extra byte in the output data may be overwritten, so make sure it is part of the output buffer
    const INT cqwInput  = cbData / sizeof(QWORD);
    const INT cqwOutput = ( cbDataCompressedMax - ibOutput - 1 ) / 7;

    // Here we want to take 8 uncompressed bytes (64 bits) and compress them into
    // 7 compressed bytes (56 bits). We do this by loading the 8 bytes into a 
    // QWORD, processing the QWORD and then writing out the entire QWORD. Although
    // a QWORD is written, the output buffer index is only advanced by 7 bytes so the
    // last byte will be overwritten with the correct value. The check above ensures
    // that the user buffer has space for the extra byte.
    //
    // Endian-ness makes this ugly. Once the bytes are loaded from memory into a QWORD,
    // the QWORD has to be reversed. After the processing the QWORD has to be reversed
    // again, but this time by 7-bit values (not by bytes).
    //
    // Suppose we have the ASCII string "01234567"
    //
    // In memory it looks like this:
    //
    //  76543210 76543210 76543210 76543210 76543210 76543210 76543210 76543210
    //  -------- -------- -------- -------- -------- -------- -------- --------
    //  00110000 00110001 00110010 00110011 00110100 00110101 00110110 00110111  30 31 32 33 34 35 36 37
    //
    // Loaded into a QWORD it looks like this:
    //
    //  76543210 76543210 76543210 76543210 76543210 76543210 76543210 76543210
    //  -------- -------- -------- -------- -------- -------- -------- --------
    //  00110111 00110110 00110101 00110100 00110011 00110010 00110001 00110000
    //
    // After ReverseEightBytes we have:
    //
    //  76543210 76543210 76543210 76543210 76543210 76543210 76543210 76543210
    //  -------- -------- -------- -------- -------- -------- -------- --------
    //  00110000 00110001 00110010 00110011 00110100 00110101 00110110 00110111
    //
    // Processing with ASCII7BITMASKANDSHIFT turns it into:
    //
    //  76543210 76543210 76543210 76543210 76543210 76543210 76543210 76543210
    //  -------- -------- -------- -------- -------- -------- -------- --------
    //  00110000 00110001 00110010 00110011 00110100 00110101 00110110 00110111
    //  00011000 00011000 10011001 00011001 10011010 00011010 10011011 00110111
    //  ...
    //  00000001 10000001 10001001 10010011 00110110 10001101 01011011 00110111
    //  00000000 11000000 11000101 10010011 00110110 10001101 01011011 00110111
    //  00000000 01100000 11000101 10010011 00110110 10001101 01011011 00110111
    //
    // After ReverseEightCompressedBytes the value becomes:
    //
    //  76543210 76543210 76543210 76543210 76543210 76543210 76543210 76543210
    //  -------- -------- -------- -------- -------- -------- -------- --------
    //  00000000 01101110 11011001 10101011 01000110 01101100 10011000 10110000
    //
    // Written to the output buffer we end up with:
    //
    //  76543210 76543210 76543210 76543210 76543210 76543210 76543210 76543210
    //  -------- -------- -------- -------- -------- -------- -------- --------
    //  10110000 10011000 01101100 01000110 10101011 11011001 01101110 00000000
    //  
    // Note that the last byte is 0. The output buffer is then advanced by 7 bytes.
    //
    for( INT iqw = 0; iqw < min( cqwInput, cqwOutput ); ++iqw )
    {
        BYTE * const pbInput    = (BYTE *)data.Pv() + ibInput;
        BYTE * const pbOutput   = pbDataCompressed + ibOutput;
        
        QWORD qw = *(Unaligned<QWORD> *)pbInput;

        qw = ReverseEightBytes( qw );

        // there are 56 wanted bits in the QWORD, shift them
        // to the bottom 7 bytes
        
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 7  ) );
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 14 ) );
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 21 ) );
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 28 ) );
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 35 ) );
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 42 ) );
        qw = ASCII7BITMASKANDSHIFT( qw, NBITMASK( 49 ) );

        qw = ReverseEightCompressedBytes( qw );

        // although only the bottom 7 bytes contain useful data
        // we will write the entire QWORD to the output buffer
        // (that is why why checked to see if there was space)
        // the output buffer pointer is only advanced by 7 bytes
        // so the last byte will be overwritten

        *(Unaligned<QWORD> *)pbOutput = qw;

        ibInput     += sizeof(QWORD);
        ibOutput    += 7;

        cqwOutputCheck++;
    }

    // now fill a DWORD with output bytes and then write it to the output buffer
    DWORD dwOutput = 0;
    INT ibitOutputCurr = 0;
    const BYTE * const pb = (BYTE *)data.Pv();
    for( INT ib = ibInput; ib < cbData; ++ib )
    {
        INT i = pb[ib];
        Assert( i < 0x80 );

        // now we have to write the correct bits of the value into the output array
        // calculate how many bits from the current compression index will fit 
        // into the current DWORD
        
        const INT cbits = 7;
        INT cbitsRemaining = cbits; // # of bits to be copied from i to the output buffer
        while( true )
        {
            const INT cbitsToCopy = min( cbitsRemaining, 32 - ibitOutputCurr );
            Assert( cbitsToCopy > 0 );
            Assert( cbitsToCopy < 32 );

            // create a bit mask for the bits we want to copy
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

    // the last DWORD we calculated may have been incomplete. 
    // if so, write the bytes that contain data
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

    // so now we have to create the header byte. it has two parts, the compression signature
    // and the count of the number of bits used in the final byte. it will be laid out like
    // this
    // 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
    //      signature    | bit count-1
    // 
    // note that the bit count is stored as 0-7 rather than 1-8

    // if ibitOutputCurr is 0 then we haven't written anything to the current DWORD, so the previous
    // DWORD is fully used and is actually the last DWORD
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

//  ================================================================
ERR CDataCompressor::ErrCompress7BitUnicode_(
    const DATA& data,
    _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    _In_ const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual )
//  ================================================================
//
//  In this scheme we remove the top 9 bits (Which are always 0)
//  and store only the bottom 7-bits of each WORD in the output array
//
//  This compression format is PERSISTED
//
//-
{
    Assert( 0 == data.Cb() % sizeof(WORD) );
    *pcbDataCompressedActual = 0;

    const INT cbCompressed = CbCompressed7BitUnicode_(data.Cb());
    if( cbCompressed > cbDataCompressedMax )
    {
        return ErrERRCheck( JET_errBufferTooSmall );
    }
    
    INT ibitOutputCurr  = 0;
    // byte 0 is the header byte    
    Unaligned<DWORD> * const pdwOutput = (Unaligned<DWORD> *)(pbDataCompressed + 1);
    INT idwOutput = 0;
    
    DWORD dwOutput = 0;
    const WORD * const pw = (WORD *)data.Pv();
    for( INT iw = 0; iw < (INT)( data.Cb() / sizeof(WORD) ); ++iw )
    {
        INT i = pw[iw];
        Assert( i < 0x80 );

        // now we have to write the correct bits of the value into the output array
        // calculate how many bits from the current compression index will fit 
        // into the current byte
        
        const INT cbits = 7;
        INT cbitsRemaining = cbits; // # of bits to be copied from i to the output buffer
        while( true )
        {
            const INT cbitsToCopy = min( cbitsRemaining, 32 - ibitOutputCurr );
            Assert( cbitsToCopy > 0 );
            Assert( cbitsToCopy < 32 );

            // create a bit mask for the bits we want to copy
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

    // we need to write the last partial bits
    AssertPREFIX( (ULONG)( idwOutput * sizeof( DWORD ) ) < (ULONG)( cbDataCompressedMax - 1 - ( ( ibitOutputCurr + 7 ) / 8) ) );
    BYTE * pbOutput = (BYTE *)(pdwOutput + idwOutput);
    for( INT ib = 0; ib < (ibitOutputCurr+7) / 8; ++ib )
    {
        BYTE b = (BYTE)( dwOutput & 0XFF );
        *pbOutput++ = b;
        dwOutput >>= 8;
    }

    // so now we have to create the header byte. it has two parts, the compression signature
    // and the count of the number of bits used in the final byte. it will be laid out like
    // this
    // 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
    //      signature    | bit count-1
    // 
    // note that the bit count is stored as 0-7 rather than 1-8
    
    // if ibitOutputCurr is 0 then we haven't written anything to the current DWORD, so the previous
    // DWORD is fully used and is actually the last DWORD
    if( 0 == ibitOutputCurr )
    {
        Assert( idwOutput > 0 );
        ibitOutputCurr = 32;
        idwOutput--;
    }
    const INT cbOutput =
        1                                               // header byte
        + idwOutput*sizeof(DWORD)                       // fully used DWORDs
        + (ibitOutputCurr+7) / 8                        // fully or partially used bytes in the last DWORD
        ;
    const INT cbitFinalByte = ( ibitOutputCurr - 1 ) % 8;
    
    const BYTE bSignature = ( COMPRESS_7BITUNICODE << 3 ) | (BYTE)(cbitFinalByte);
    pbDataCompressed[0] = bSignature;

    *pcbDataCompressedActual = cbOutput;
    return JET_errSuccess;
}

//  ================================================================
ERR CDataCompressor::ErrCompressXpress_(
    const DATA& data,
    _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual,
    IDataCompressorStats * const pstats )
//  ================================================================
{
    PERFOptDeclare( const HRT hrtStart = HrtHRTCount() );

    Assert( data.Cb() >= m_cbMin );
    Assert( data.Cb() <= wMax );                // size must fit in a WORD
    Assert( data.Cb() <= m_cbMax );             // Xpress (legacy) only supports compressing one window at a time
    Assert( pstats );
    
    ERR err = JET_errSuccess;
    
    XpressEncodeStream encode = 0;
    Call( ErrXpressEncodeOpen_( &encode ) );

    // Reserve 1 BYTE (for the signature) and 1 WORD (for the data length)
    const INT cbReserved = sizeof(BYTE) + sizeof(WORD);
    
    const INT cbCompressed = XpressEncode(
                        encode,                             // encoding workspace
                        pbDataCompressed + cbReserved,      // pointer to output buffer for compressed data
                        cbDataCompressedMax - cbReserved,   // size of output buffer
                        data.Pv(),                          // pointer to input buffer
                        data.Cb(),                          // size of input buffer
                        0,                                  // NULL or progress callback
                        0,                                  // user-defined context that will be passed to ProgressFn
                        0 );                                // call ProgressFn each time ProgressSize bytes processed
    Assert( cbCompressed <= data.Cb() );
    
    if( cbCompressed == 0 || cbCompressed + cbReserved >= data.Cb() )
    {
        // Xpress was unable to compress.
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

//  ================================================================
ERR CDataCompressor::ErrScrub(
    DATA& data,
    const CHAR chScrub )
//  ================================================================
//
//  This code scrubs the byte array in a way that is detectable by
//  decompression. We set a known "compression type" in the first
//  byte and scrub with the passed in pattern fill after that.
//
//-
{
    BYTE * const pbData = (BYTE *)data.Pv();
    const INT cbData = data.Cb();

    const BYTE bSignature = ( COMPRESS_SCRUB << 3 );
    pbData[0] = bSignature;
    memset( pbData + 1, chScrub, cbData - 1 );

    return JET_errSuccess;
}

#ifdef XPRESS9_COMPRESSION
//  ================================================================
ERR CDataCompressor::ErrXpress9StatusToJetErr(
    const XPRESS9_STATUS& status )
//  ================================================================
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

//  ================================================================
// This function decompresses data compressed with xpress9 to prove that
// xpress9 compression isn't introducing data corruption.

ERR CDataCompressor::ErrVerifyCompressXpress9_(
    _In_reads_bytes_( cbDataCompressed ) const BYTE * const pbDataCompressed,
    const INT cbDataCompressed,
    const INT cbDataCompressedMax,
    IDataCompressorStats * const pstats )
    //  ================================================================
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
        // Only allowed if running unit tests with uninitialized caches
        Alloc( pbDecompress = new BYTE[ cbDataCompressedMax ] );
    }

    err = ErrDecompressXpress9_( dataCompressed, pbDecompress, cbDataCompressedMax, &cbDataDecompressed, pstats );
    if ( err != JET_errSuccess && err != JET_errOutOfMemory )
    {
        // We have caught a potential data corruption issue in our compress/decompress pipeline !
        // Crash and diagnose.
        // If we couldn't decompress because of OOM, do nothing. Verification is an optional step.

        FireWall( "Xpress9CannotCompress" );
        err = ErrERRCheck( errRECCannotCompress );
    }

    if ( g_compressionBufferCache.CbBufferSize() != 0 )
    {
        g_compressionBufferCache.Free( pbDecompress );
    }
    else
    {
        // Only allowed if running unit tests with uninitialized caches
        delete[] pbDecompress;
    }

HandleError:
    return err;
}

//  ================================================================
ERR CDataCompressor::ErrCompressXpress9_(
    const DATA& data,
    _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual )
//  ================================================================
{
    Assert( data.Cb() >= m_cbMin );
    Assert( data.Cb() <= lMax );                // size must fit in a DWORD

    ERR err = JET_errSuccess;
    const INT cbReserved = sizeof( Xpress9Header ); // Reserve 5 bytes for the header
    XPRESS9_STATUS status = { 0 };
    XPRESS9_ENCODER encode = 0;
    Call( ErrXpress9EncodeOpen_( &encode ) );

    // start an encoding session using the exact parameters for Cosmos Level 6 compression
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

    // Compress the data
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
            // The buffer was too small for the compressed data.
            // Very likely, data didn't compress well and we ended up 
            // needing more bytes than original because of xpress9 format.
            
            // Empty the encoder. Testing shows that Detach/StartSession/Attach doesn't reset the encoder.
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
#endif // XPRESS9_COMPRESSION

#ifdef XPRESS10_COMPRESSION
//  ================================================================
// This function decompresses data compressed with xpress10 to prove that
// xpress10 compression isn't introducing data corruption.

ERR CDataCompressor::ErrVerifyCompressXpress10_(
    _In_reads_bytes_( cbDataCompressed ) const BYTE * const pbDataCompressed,
    const INT cbDataCompressed,
    const INT cbDataUncompressed,
    IDataCompressorStats * const pstats )
    //  ================================================================
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
        // Only allowed if running unit tests with uninitialized caches
        Alloc( pbDecompress = new BYTE[ cbDataUncompressed ] );
    }

    // This is probably cheap enough that we can leave this in forever for peace of mind.
    err = ErrDecompressXpress10_( dataCompressed, pbDecompress, cbDataUncompressed, &cbDataDecompressed, pstats, fFalse, &fUsedCorsica );
    if ( ( err == JET_errSuccess && cbDataUncompressed != cbDataDecompressed ) ||
         ( err != JET_errSuccess && err != JET_errOutOfMemory ) )
    {
        // We have caught a potential data corruption issue in our compress/decompress pipeline !
        // Crash and diagnose.
        // If we couldn't decompress because of OOM, do nothing. Verification is an optional step.

        FireWall( "Xpress10CorsicaCannotDecompress" );
        err = ErrERRCheck( errRECCannotCompress );
    }
    else if ( fUsedCorsica )
    {
        // This is hopefully cheap enough that we can leave this in forever for peace of mind.
        // If it is too expensive, we may need to do verification on less than 100% of the data, maybe progressively
        // (start with 100% on process start and go down slowly, eg)
        err = ErrDecompressXpress10_( dataCompressed, pbDecompress, cbDataUncompressed, &cbDataDecompressed, pstats, fTrue, &fUsedCorsica );
        AssertTrack( err != JET_errSuccess || !fUsedCorsica, "SoftwareDecompressionFlagNotHonored" );
        if ( ( err == JET_errSuccess && cbDataUncompressed != cbDataDecompressed ) ||
             ( err != JET_errSuccess && err != JET_errOutOfMemory ) )
        {
            // We have caught a potential data corruption issue in our compress/decompress pipeline !
            // Crash and diagnose.
            // If we couldn't decompress because of OOM, do nothing. Verification is an optional step.

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
        // Only allowed if running unit tests with uninitialized caches
        delete[] pbDecompress;
    }

HandleError:
    return err;
}

// Software Xpress10 compression is only for unit-test
BOOL g_fAllowXpress10SoftwareCompression = fFalse;
CInitOnce< ERR, decltype(&ErrXpress10CorsicaInit) > g_CorsicaInitOnce;

//  ================================================================
ERR CDataCompressor::ErrCompressXpress10_(
    const DATA& data,
    _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual,
    IDataCompressorStats * const pstats )
//  ================================================================
{
    PERFOptDeclare( const HRT hrtStart = HrtHRTCount() );
    ERR err;

    (VOID)g_CorsicaInitOnce.Init( ErrXpress10CorsicaInit );

    Assert( data.Cb() <= wMax );

    // Reserve 15 BYTE for signature
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
#endif // XPRESS10_COMPRESSION

 //  ================================================================
ERR CDataCompressor::ErrDecompress7BitAscii_(
    const DATA& dataCompressed,
    _Out_writes_bytes_to_opt_( cbDataMax, *pcbDataActual ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual )
//  ================================================================
//
//  This compression format is PERSISTED
//
//-
{
    ERR err = JET_errSuccess;
    
    // first calculate the final length
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

    // no output buffer? just return the size
    if( 0 == cbDataMax || NULL == pbData )
    {
        goto HandleError;
    }

    // calculate the number of bytes to extract
    const INT ibDataMax = min( cbTotal, cbDataMax );

    INT ibCompressed = 1;   // skip the header
    INT ibitCompressed = 0;
    for( INT ibData = 0; ibData < ibDataMax; ++ibData )
    {
        Assert( ibCompressed < dataCompressed.Cb() );
        BYTE bDecompressed;
        if( ibitCompressed <= 1 )
        {
            // all the bits we need are in this one byte
            const BYTE bCompressed = pbCompressed[ibCompressed];
            bDecompressed = (BYTE)(( bCompressed >> ibitCompressed ) & 0x7F);
        }
        else
        {
            // all the bits we need are in this word
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

//  ================================================================
ERR CDataCompressor::ErrDecompress7BitUnicode_(
    const DATA& dataCompressed,
    _Out_writes_bytes_to_opt_( cbDataMax, *pcbDataActual ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual )
//  ================================================================
//
//  This compression format is PERSISTED
//
//-
{
    ERR err = JET_errSuccess;
    
    // first calculate the final length
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

    // no output buffer? just return the size
    if( 0 == cbDataMax || NULL == pbData )
    {
        goto HandleError;
    }

    // calculate the number of bytes to extract
    const INT ibDataMax = min( *pcbDataActual, cbDataMax );

    INT ibCompressed = 1;   // skip the header
    INT ibitCompressed = 0;
    for( INT ibData = 0; ibData < ibDataMax; )
    {
        Assert( ibCompressed < dataCompressed.Cb() );
        BYTE bDecompressed;
        if( ibitCompressed <= 1 )
        {
            // all the bits we need are in this one byte
            const BYTE bCompressed = pbCompressed[ibCompressed];
            bDecompressed = (BYTE)(( bCompressed >> ibitCompressed ) & 0x7F);
        }
        else
        {
            // all the bits we need are in this word
            Assert( ibCompressed < dataCompressed.Cb()-1 );
            const WORD wCompressed = (WORD)pbCompressed[ibCompressed] | ( (WORD)pbCompressed[ibCompressed+1] << 8 );
            bDecompressed = (BYTE)(( wCompressed >> ibitCompressed ) & 0x7F);
        }

        // insert the byte and the following null
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

//  ================================================================
ERR CDataCompressor::ErrDecompressXpress_(
    const DATA& dataCompressed,
    _Out_writes_bytes_to_opt_( cbDataMax, *pcbDataActual ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual,
    IDataCompressorStats * const pstats )
//  ================================================================
{
    ERR err = JET_errSuccess;
    PERFOptDeclare( const HRT hrtStart = HrtHRTCount() );

    XpressDecodeStream decode = 0;

    // first calculate the final length
    const BYTE * const  pbCompressed    = (BYTE *)dataCompressed.Pv();
    const UnalignedLittleEndian<WORD> * const pwSize    = (UnalignedLittleEndian<WORD> *)(pbCompressed + 1);
    const INT cbUncompressed            = *pwSize;
    
    const BYTE bHeader      = *(BYTE *)dataCompressed.Pv();
    OnDebug( const BYTE bIdentifier     = bHeader >> 3 );
    Assert( bIdentifier == COMPRESS_XPRESS );

    *pcbDataActual = cbUncompressed;
    
    // there is a header byte and then a WORD containing the size of the data
    const INT cbHeader              = sizeof(BYTE) + sizeof(WORD);
    const BYTE * pbCompressedData   = (BYTE *)dataCompressed.Pv() + cbHeader;
    const INT cbCompressedData      = dataCompressed.Cb() - cbHeader;
    const INT cbWanted              = min( *pcbDataActual, cbDataMax );

    if ( NULL == pbData || 0 == cbDataMax )
    {
        // no data, just the size
        err = ErrERRCheck( JET_wrnBufferTruncated );
        goto HandleError;
    }

    Assert( cbWanted <= cbUncompressed );

    Call( ErrXpressDecodeOpen_( &decode ) );

    const INT cbDecoded = XpressDecode(
        decode,             // decoder's workspace
        pbData,             // output buffer (decompressed)
        cbUncompressed,     // original size of the data
        cbWanted,           // # of bytes to decode (should be <= original data size)
        pbCompressedData,   // compressed data buffer
        cbCompressedData ); // size of compressed data
    if( -1 == cbDecoded )
    {
        // decompression has failed
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

//  ================================================================
ERR CDataCompressor::ErrDecompressScrub_(
    const DATA& data )
//  ================================================================
//
//  Decompressing data that was scrubbed using the COMPRESS_SCRUB
//  does not return any valid data and does not change the buffer
//  in any way. This function merely signals wrnRECCompressionScrubDetected
//  so that callers can take proper actions.
//
//-
{
#ifdef DEBUG
    // first calculate the final length
    const BYTE * const pbData = (BYTE *)data.Pv();
    const INT cbData = data.Cb();
    Assert( cbData >= 1 );
    const BYTE bHeader = pbData[0];
    const BYTE bIdentifier = bHeader >> 3;

    Assert( bIdentifier == COMPRESS_SCRUB );
    Expected( ( bHeader & 0x07 ) == 0 );    //  no meta-data for this compression type.

    // make sure all the characters are the same
    if ( cbData > 1 )
    {
        const CHAR chKnownPattern = pbData[1];

        // currently only used to scrub orphaned LV chunks.
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
//  ================================================================
ERR CDataCompressor::ErrDecompressXpress9_(
    const DATA& dataCompressed,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual,
    IDataCompressorStats * const pstats )
//  ================================================================
{
    ERR err = JET_errSuccess;
    PERFOptDeclare( const HRT hrtStart = HrtHRTCount() );

    XPRESS9_STATUS status = { 0 };
    XPRESS9_DECODER decode = 0;
    unsigned cbDecompressed = 0;
    unsigned cbCompressedNeeded = 0;

    // Reserved 5 bytes (for the signature)
    const INT cbReserved = sizeof( Xpress9Header );

    // get the compressed data
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

    // start a decoding session
    Call( ErrXpress9DecodeOpen_( &decode ) );
    Xpress9DecoderStartSession( &status, decode, TRUE );
    Call( ErrXpress9StatusToJetErr( status ) );

    // decompress the requested portion of the data
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
        // Verify checksum
        // Note that we can't verify the checksum if JET_wrnBufferTruncated was returned
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
#endif // XPRESS9_COMPRESSION

#ifdef XPRESS10_COMPRESSION
//  ================================================================
ERR CDataCompressor::ErrDecompressXpress10_(
    const DATA& dataCompressed,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual,
    IDataCompressorStats * const pstats,
    BOOL fForceSoftwareDecompression,
    BOOL *pfUsedCorsica )
//  ================================================================
{
    ERR err = JET_errSuccess;
    BYTE *pbAlloc = NULL;

    (VOID)g_CorsicaInitOnce.Init( ErrXpress10CorsicaInit );

    PERFOptDeclare( const HRT hrtStart = HrtHRTCount() );
    HRT dhrtHardwareLatency;

    // Reserved 15 BYTE (for the signature)
    C_ASSERT( sizeof( Xpress10Header ) == 15 );
    const INT cbReserved = sizeof( Xpress10Header );
    const INT cbCompressedData = dataCompressed.Cb() - cbReserved;
    if ( cbCompressedData < 0 )
    {
        return ErrERRCheck( JET_errDecompressionFailed );
    }

    // Verify the header flags
    const Xpress10Header * const pHdr = (Xpress10Header *)dataCompressed.Pv();
    if ( ( pHdr->m_fCompressScheme >> 3 ) != COMPRESS_XPRESS10 )
    {
        return ErrERRCheck( JET_errDecompressionFailed );
    }
    // first calculate the final length
    const INT cbUncompressed = pHdr->mle_cbUncompressed;
    if ( NULL == pbData || 0 == cbDataMax )
    {
        // no data, just the size
        *pcbDataActual = cbUncompressed;
        return ErrERRCheck( JET_wrnBufferTruncated );
    }

    // Xpress10 decompression cannot provide partial data for decompression,
    // so allocate a buffer big enough to hold the whole data.
    if ( cbDataMax < cbUncompressed )
    {
        if ( g_compressionBufferCache.CbBufferSize() != 0 )
        {
            Assert( g_compressionBufferCache.CbBufferSize() >= cbUncompressed );
            AllocR( pbAlloc = g_compressionBufferCache.PbAlloc() );
        }
        else
        {
            // Only allowed if running unit tests with uninitialized caches
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
        // Only allowed if running unit tests with uninitialized caches
        delete[] pbAlloc;
    }
    return err;
}
#endif // XPRESS10_COMPRESSION

//  ================================================================
ERR CDataCompressor::ErrCompress(
    const DATA& data,
    const CompressFlags compressFlags,
    IDataCompressorStats * const pstats,
    _Out_writes_bytes_to_opt_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual )
//  ================================================================
{
    ERR err = JET_errSuccess;
    BOOL fCompressed = fFalse;

    Assert( 0 != compressFlags );

    if ( data.Cb() >= m_cbMin )
    {
        // try the advanced compression algorithms in priority order (highest compression potential first)
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

            // Verify that compressed data is decompressible, otherwise a firmware bug/hardware issue with Corsica can result in replicated corruption.
            CallJ( ErrVerifyCompressXpress10_( pbDataCompressed, *pcbDataCompressedActual, data.Cb(), pstats ), TryXpress9Compression );

            fCompressed = fTrue;
        }

TryXpress9Compression:
#endif // XPRESS10_COMPRESSION
#ifdef XPRESS9_COMPRESSION
        if ( !fCompressed && ( compressFlags & compressXpress9 ) )
        {
            CallJ( ErrCompressXpress9_(
                data,
                pbDataCompressed,
                cbDataCompressedMax,
                pcbDataCompressedActual ),
                TryXpressCompression );

            // Verify that compressed data is decompressible.
            // Just to be on the safe side, we consider OOM during verify a failure to compress
            // (not a verification failure which would indicate a serious corruption bug).

            CallJ( ErrVerifyCompressXpress9_( pbDataCompressed, *pcbDataCompressedActual, cbDataCompressedMax, pstats ), TryXpressCompression );

            fCompressed = fTrue;
        }

TryXpressCompression:
#endif // XPRESS9_COMPRESSION
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

        // if this compression was better than 7-bit unicode can do then we're done

        if ( fCompressed && *pcbDataCompressedActual <= CbCompressed7BitUnicode_( data.Cb() ) )
        {
            return JET_errSuccess;
        }
    }

Try7bitCompression:
    // We don't want to clobber any errors reported by lower layers unless its errRECCannotCompress or OOM.
    // OOM is a transient condition, optionally try 7-bit compression and/or return errRECCannotCompress
    if ( err != JET_errSuccess && err != errRECCannotCompress && err != JET_errOutOfMemory )
    {
        return err;
    }

    // try 7 bit compression
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

//  ================================================================
ERR CDataCompressor::ErrDecompress(
    const DATA& dataCompressed,
    IDataCompressorStats * const pstats,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual )
//  ================================================================
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
            //  No one can call this to actually decompress any data, caller can only query the size.
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
            // fUnused *is* used in the conditionally compiled XPRESS10_COMPRESSION above,
            // but if XPRESS10_COMPRESSION is not defined, we get a compiler warning, so
            // let's "use" fUnused here:
            Unused( fUnused );
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

//  ================================================================
ERR CDataCompressor::ErrInit( const INT cbMin, const INT cbMax )
//  ================================================================
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

//  ================================================================
void CDataCompressor::Term()
//  ================================================================
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

// This is a global instance of the data compressor, which can be used by the entire process
static CDataCompressor g_dataCompressor;

//  ================================================================
ERR ErrRECScrubLVChunk(
    DATA& data,
    const CHAR chScrub )
//  ================================================================
{
    // we are only expecting LV chunks for now.
    return g_dataCompressor.ErrScrub( data, chScrub );
}

//  ================================================================
ERR ErrPKCompressData(
    const DATA& data,
    const CompressFlags compressFlags,
    const INST* const pinst,
    _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual )
//  ================================================================
{
    CDataCompressorPerfCounters perfcounters( pinst ? pinst->m_iInstance : 0 );
    
    return g_dataCompressor.ErrCompress( data, compressFlags, &perfcounters, pbDataCompressed, cbDataCompressedMax, pcbDataCompressedActual );
}

//  ================================================================
ERR ErrPKIDecompressData(
    const DATA& dataCompressed,
    const INST* const pinst,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual )
//  ================================================================
{
    CDataCompressorPerfCounters perfcounters( pinst ? pinst->m_iInstance : 0 );
    return g_dataCompressor.ErrDecompress( dataCompressed, &perfcounters, pbData, cbDataMax, pcbDataActual );
}

//  ================================================================
VOID PKIReportDecompressionFailed(
        const IFMP ifmp,
        const PGNO pgno,
        const INT iline,
        const OBJID objidFDP,
        const PGNO pgnoFDP )
//  ================================================================
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

//  ================================================================
ERR ErrPKDecompressData(
    const DATA& dataCompressed,
    const FUCB* const pfucb,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual )
//  ================================================================
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

//  ================================================================
ERR ErrPKDecompressData(
    const DATA& dataCompressed,
    const IFMP ifmp,
    const PGNO pgno,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual )
//  ================================================================
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

//  ================================================================
ERR ErrPKAllocAndDecompressData(
    const DATA& dataCompressed,
    const FUCB* const pfucb,
    _Outptr_result_bytebuffer_( *pcbDataActual )  BYTE ** const ppbData,
    _Out_ INT * const pcbDataActual )
//  ================================================================
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

//  ================================================================
BYTE * PbPKAllocCompressionBuffer()
//  ================================================================
{
    return g_compressionBufferCache.PbAlloc();
}

//  ================================================================
INT CbPKCompressionBuffer()
//  ================================================================
{
    return g_compressionBufferCache.CbBufferSize();
}

//  ================================================================
VOID PKFreeCompressionBuffer( _Post_ptr_invalid_ BYTE * const pb )
//  ================================================================
{
    g_compressionBufferCache.Free(pb);
}

//  ================================================================
ERR ErrPKInitCompression( const INT cbPage, const INT cbCompressMin, const INT cbCompressMax )
//  ================================================================
{
    ERR err = JET_errSuccess;
    // Buffer size needs to be as big as the page size for compressing intrinsic LVs ( extrinsic LVs only need JET_paramLVChunkSizeMost + AES256 overhead )
    CallJ( g_compressionBufferCache.ErrInit( cbPage ), Term );
    CallJ( g_dataCompressor.ErrInit( cbCompressMin, cbCompressMax ), TermBufferCache );
    return err;

TermBufferCache:
    g_compressionBufferCache.Term();

Term:
    return err;
}

//  ================================================================
void PKTermCompression()
//  ================================================================
{
#ifdef XPRESS10_COMPRESSION
    Xpress10CorsicaTerm();
    g_CorsicaInitOnce.Reset();
#endif
    g_dataCompressor.Term();
    g_compressionBufferCache.Term();
}

#ifdef ENABLE_JET_UNIT_TEST

//  ================================================================
class TestCompressorStats : public IDataCompressorStats
//  ================================================================
//
//  Used in the unit-tests
//
//-
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
    
//  ================================================================
JETUNITTEST( CDataCompressor, 7BitAscii )
//  ================================================================
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

    // data that is too small to compress
    sz = "foo";
    data.SetPv( sz );
    data.SetCb( strlen(sz) );
    err = compressor.ErrCompress( data, compress7Bit, &stats, rgbBuf1, cbBuf, &cbDataActual );
    CHECK( errRECCannotCompress == err );

    // data that is just big enough to compress
    sz = "1234567890ABCDEF";
    data.SetPv( sz );
    data.SetCb( strlen(sz) );
    err = compressor.ErrCompress( data, compress7Bit, &stats, rgbBuf1, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( 15 == cbDataActual );

    // decompress the data
    data.SetPv( rgbBuf1 );
    data.SetCb( cbDataActual );
    err = compressor.ErrDecompress( data, &stats, rgbBuf2, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( 16 == cbDataActual );
    CHECK( 0 == memcmp( sz, rgbBuf2, 16 ) );

    // decompress 1 byte of the data
    data.SetPv( rgbBuf1 );
    data.SetCb( 15 );
    err = compressor.ErrDecompress( data, &stats, rgbBuf2, 1, &cbDataActual );
    CHECK( JET_wrnBufferTruncated == err );
    CHECK( 16 == cbDataActual );
    CHECK( rgbBuf2[0] == sz[0] );

    compressor.Term();
}

//  ================================================================
JETUNITTEST( CDataCompressor, 7BitUnicode )
//  ================================================================
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

    // data that is too small to compress
    sz = L"f";
    data.SetPv( sz );
    data.SetCb( wcslen(sz) );
    err = compressor.ErrCompress( data, compress7Bit, &stats, rgbBuf1, cbBuf, &cbDataActual );
    CHECK( errRECCannotCompress == err );

    // data that is just big enough to compress
    sz = L"12";
    data.SetPv( sz );
    data.SetCb( wcslen(sz)*sizeof(sz[0]) );
    err = compressor.ErrCompress( data, compress7Bit, &stats, rgbBuf1, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( 3 == cbDataActual );

    // decompress the data
    data.SetPv( rgbBuf1 );
    data.SetCb( cbDataActual );
    err = compressor.ErrDecompress( data, &stats, rgbBuf2, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( 4 == cbDataActual );
    CHECK( 0 == memcmp( sz, rgbBuf2, 4 ) );

    // decompress 1 byte of the data
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

//  ================================================================
void DataCompressorBasic( const CompressFlags compressFlags, BYTE* rgbBuf = NULL, INT cbBuf = 2048 )
//  ================================================================
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

    // query the size of the data
    data.SetPv( rgbBufCompressed );
    data.SetCb( cbDataActual );

    err = compressor.ErrDecompress( data, &stats, rgbBufDecompressed, cbBuf - 1, &cbDataActual );
    
    CHECK( JET_wrnBufferTruncated == err );
    CHECK( cbBuf == cbDataActual );

    // decompress the data
    err = compressor.ErrDecompress( data, &stats, rgbBufDecompressed, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( cbBuf == cbDataActual );
    CHECK( 0 == memcmp( rgbBuf, rgbBufDecompressed, cbDataActual ) );

    // decompress 1 byte of the data
    rgbBufDecompressed[0] = 0;
    err = compressor.ErrDecompress( data, &stats, rgbBufDecompressed, 1, &cbDataActual );
    CHECK( JET_wrnBufferTruncated == err );
    CHECK( cbBuf == cbDataActual );
    CHECK( rgbBuf[0] == rgbBufDecompressed[0] );

    compressor.Term();
}

#pragma pop_macro("CHECK")

//  ================================================================
JETUNITTEST( CDataCompressor, Xpress )
//  ================================================================
{
    DataCompressorBasic( compressXpress );
}

#ifdef XPRESS9_COMPRESSION
//  ================================================================
JETUNITTEST( CDataCompressor, Xpress9Cpu )
//  ================================================================
{
    DataCompressorBasic( compressXpress9 );
}
#endif

#ifdef XPRESS10_COMPRESSION
//  ================================================================
JETUNITTEST( CDataCompressor, Xpress10 )
//  ================================================================
{
    CHECK( JET_errSuccess == ErrPKInitCompression( 32*1024, 1024, 32*1024 ) );
    g_fAllowXpress10SoftwareCompression = fTrue;
    DataCompressorBasic( compressXpress10 );
    DataCompressorBasic( compressXpress10, NULL, 8150 );
    g_fAllowXpress10SoftwareCompression = fFalse;
    PKTermCompression();
}

//  ================================================================
JETUNITTEST( CDataCompressor, Xpress10FallbackXpress )
//  ================================================================
{
    // If neither Corsica nor software compression for xpress10 is enabled,
    // we should fallback to xpress, if specified.
    CHECK( JET_errSuccess == ErrPKInitCompression( 32*1024, 1024, 32*1024 ) );
    DataCompressorBasic( CompressFlags( compressXpress10 | compressXpress ) );
    DataCompressorBasic( CompressFlags( compressXpress10 | compressXpress ), NULL, 8150 );
    PKTermCompression();
}
#endif

//  ================================================================
JETUNITTEST( CDataCompressor, XpressThreshold )
//  ================================================================
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

    // Smallest size that does compress
    data.SetCb( cbMinDataForXpress );
    err = compressor.ErrCompress( data, compressXpress, &stats, rgbBufCompressed, sizeof(rgbBufCompressed), &cbDataActual );
    CHECK( JET_errSuccess == err );

    // Too small to compress
    data.SetCb( cbMinDataForXpress - 1 );
    err = compressor.ErrCompress( data, compressXpress, &stats, rgbBufCompressed, sizeof(rgbBufCompressed), &cbDataActual );
    CHECK( errRECCannotCompress == err );

    compressor.Term();
}

//  ================================================================
JETUNITTEST( CDataCompressor, Scrub )
//  ================================================================
{
    CDataCompressor compressor;
    TestCompressorStats stats;

    ERR err;
    const INT cbBuf = 2048;
    BYTE rgbBufOrig[cbBuf];
    BYTE rgbBufScrubbed[cbBuf];
    DATA data;

    CHECK( JET_errSuccess == compressor.ErrInit( 1024, 2048 ) );

    // compress the data
    memset( rgbBufOrig, 0, cbBuf );
    memset( rgbBufScrubbed, chSCRUBDBMaintLVChunkFill, cbBuf );
    data.SetPv( rgbBufOrig );
    data.SetCb( cbBuf );
    err = compressor.ErrScrub( data, chSCRUBDBMaintLVChunkFill );
    CHECK( JET_errSuccess == err );
    CHECK( 0 == memcmp( rgbBufOrig + 1, rgbBufScrubbed, cbBuf - 1 ) );

    // decompress the data
    INT cbDataActual = INT_MAX;
    UtilMemCpy( rgbBufScrubbed, rgbBufOrig, cbBuf );
    FNegTestSet( fInvalidUsage );
    err = compressor.ErrDecompress( data, &stats, NULL, 0, &cbDataActual );
    FNegTestUnset( fInvalidUsage );
    CHECK( wrnRECCompressionScrubDetected == err );
    CHECK( 0 == cbDataActual );
    CHECK( 0 == memcmp( rgbBufOrig, rgbBufScrubbed, cbBuf ) );

    // compress 1 byte of data
    memset( rgbBufOrig, 0, cbBuf );
    data.SetPv( rgbBufOrig );
    data.SetCb( 1 );
    err = compressor.ErrScrub( data, chSCRUBDBMaintLVChunkFill );
    CHECK( JET_errSuccess == err );

    // decompress 1 byte of data
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

//  ================================================================
void DataCompressorEfficiencyCases( const CompressFlags compressFlags )
//  ================================================================
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

    // data that compresses well
    memset( rgbBuf1, 0xEE, cbBuf );
    data.SetPv( rgbBuf1 );
    data.SetCb( cbBuf );
    err = compressor.ErrCompress( data, compressFlags, &stats, rgbBuf2, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( cbDataActual < cbBuf );

    // data that doesn't compress
    // fill the buffer with a pattern (so all bytes are equally likely)
    // and then randomize the order of the bytes
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

    // data that compresses poorly
    // take the randomized buffer from above and set 1/8 of it to be compressable data
    memset( rgbBuf1, 0xCC, cbBuf/8 );
    
    data.SetPv( rgbBuf1 );
    data.SetCb( cbBuf );
    err = compressor.ErrCompress( data, compressFlags, &stats, rgbBuf2, cbBuf, &cbDataActual );
    CHECK( JET_errSuccess == err );
    CHECK( cbDataActual < cbBuf );

    compressor.Term();
}

#pragma pop_macro("CHECK")

//  ================================================================
JETUNITTEST( CDataCompressor, XpressEfficiencyCases )
//  ================================================================
{
    DataCompressorEfficiencyCases( compressXpress );
}

#ifdef XPRESS9_COMPRESSION
//  ================================================================
JETUNITTEST( CDataCompressor, Xpress9CpuEfficiencyCases )
//  ================================================================
{
    DataCompressorEfficiencyCases( compressXpress9 );
}
#endif

#ifdef XPRESS10_COMPRESSION
//  ================================================================
JETUNITTEST( CDataCompressor, Xpress10EfficiencyCases )
//  ================================================================
{
    CHECK( JET_errSuccess == ErrPKInitCompression( 32*1024, 1024, 32*1024 ) );
    g_fAllowXpress10SoftwareCompression = fTrue;
    DataCompressorEfficiencyCases( compressXpress10 );
    g_fAllowXpress10SoftwareCompression = fFalse;
    PKTermCompression();
}
#endif

//  ================================================================
JETUNITTEST( CCompressionBufferCache, Basic )
//  ================================================================
{
    CCompressionBufferCache cache;

    CHECK( JET_errSuccess == cache.ErrInit( 1024 ) );
    CHECK( 1024 == cache.CbBufferSize() );

    // allocate a buffer
    BYTE * const pb1 = cache.PbAlloc();
    CHECK( NULL != pb1 );
    memset( pb1, 0, cache.CbBufferSize() );
    cache.Free(pb1);

    // if we free one buffer we should get the same buffer back
    BYTE * const pb2 = cache.PbAlloc();
    CHECK( pb1 == pb2 );
    memset( pb2, 0xFF, cache.CbBufferSize() );

    // but allocating a second buffer concurrently should give a different buffer
    BYTE * const pb3 = cache.PbAlloc();
    CHECK( pb2 != pb3 );
    
    cache.Free(pb2);
    cache.Free(pb3);
    
    cache.Term();
}

#endif // ENABLE_JET_UNIT_TEST

