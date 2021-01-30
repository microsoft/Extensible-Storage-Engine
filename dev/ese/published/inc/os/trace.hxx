// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_TRACE_HXX_INCLUDED
#define _OS_TRACE_HXX_INCLUDED

#include <utility>

class TRACEINFO
{
    public:
        TRACEINFO()                 {}
        ~TRACEINFO()                {}

        VOID SetUl( const ULONG_PTR ul )        { m_ul = ul; }
        VOID SetFEnabled()                      { m_fEnabled = fTrue; }
        VOID SetFDisabled()                     { m_fEnabled = fFalse; }

        const ULONG_PTR Ul()                    { return m_ul; }
        const BOOL FEnabled()                   { return m_fEnabled; }
        const BOOL FDisabled()                  { return !m_fEnabled; }

    private:
        ULONG_PTR   m_ul;
        BOOL        m_fEnabled;
};


typedef ULONG               TRACETAG;
typedef void (__stdcall * PFNTRACEEMIT)( const TRACETAG tag, const char * szPrefix, const char * szTrace, const ULONG_PTR ul );

extern BOOL                 g_fDisableTracingForced;
extern BOOL                 g_fTracing;
extern TRACEINFO            g_rgtraceinfo[];
extern DWORD                g_tidTrace;

extern const WCHAR* const g_rgwszTraceDesc[];
#define JET_tracetagDescCbMax   ( 64 * sizeof( WCHAR ) )

inline void OSTraceSetGlobal( const BOOL fEnable )
{
    g_fTracing = fEnable;
}

inline void OSTraceSetTag( const TRACETAG tag, const BOOL fEnable )
{
    if ( fEnable )
    {
        g_rgtraceinfo[ tag ].SetFEnabled();
    }
    else
    {
        g_rgtraceinfo[ tag ].SetFDisabled();
    }
}

inline void OSTraceSetTagData( const TRACETAG tag, const ULONG_PTR ul )
{
    g_rgtraceinfo[ tag ].SetUl( ul );
}

inline void OSTraceSetThreadidFilter( const DWORD tidTrace )
{
    g_tidTrace = tidTrace;
}



char* OSAllocInfoString( const size_t cch );
WCHAR* OSAllocInfoStringW( const size_t cch );
void OSFreeInfoStrings();



void OSTraceRegisterEmitCallback( PFNTRACEEMIT pfnEmit );
void __stdcall OSTraceEmit( const TRACETAG tag, const char* const szPrefixNYI, const char* const szRawTrace, const ULONG_PTR ul );

void OSTrace_( const TRACETAG tag, const char* const szTrace );
void OSTraceIndent_( const INT dLevel );
void OSTraceSuspendGC_();
void OSTraceResumeGC_();

#define OSTraceSuspendGC()      OSTraceSuspendGC_()
#define OSTraceResumeGC()       OSTraceResumeGC_()

#define FOSTraceTagEnabled_( __tag, __fMeetsExternalFilterCriteria )    \
    ( g_fTracing                                                        \
        && !g_fDisableTracingForced                                     \
        && g_rgtraceinfo[ __tag ].FEnabled()                            \
        && ( 0 == g_tidTrace || DwUtilThreadId() == g_tidTrace )        \
        && ( __fMeetsExternalFilterCriteria ) )

#define OSTrace( __tag, __szTrace )                                     \
{                                                                       \
    if ( FOSTraceTagEnabled_( __tag, fTrue ) )                          \
    {                                                                   \
        OSTraceSuspendGC_();                                            \
        OSTrace_( __tag, __szTrace );                                   \
        OSTraceResumeGC_();                                             \
    }                                                                   \
}

#define OSTraceIndent( __tag, __dLevel )                                \
{                                                                       \
    if ( FOSTraceTagEnabled_( __tag, fTrue ) )                          \
    {                                                                   \
        OSTraceIndent_( __dLevel );                                     \
    }                                                                   \
}

#define OSTraceFiltered( __tag, __szTrace, __fMeetsExternalFilterCriteria ) \
{                                                                       \
    if ( FOSTraceTagEnabled_( __tag, __fMeetsExternalFilterCriteria ) ) \
    {                                                                   \
        OSTraceSuspendGC_();                                            \
        OSTrace_( __tag, __szTrace );                                   \
        OSTraceResumeGC_();                                             \
    }                                                                   \
}

#define FOSTraceTagEnabled( __tag ) FOSTraceTagEnabled_( __tag, fTrue )


#define OSTRACEINDENTSZ     "\r\r+\r"
#define OSTRACEUNINDENTSZ   "\r\r-\r"
#define OSTRACENULLPARAM    "<null>"
#define OSTRACENULLPARAMW   L"<null>"



const char* __cdecl OSFormat( __format_string const char* const szFormat, ... );
const WCHAR* __cdecl OSFormatW( __format_string const WCHAR* const wszFormat, ... );

inline const char* OSFormatBoolean( const BOOL f )
{
    return ( f ? "True" : "False" );
}

inline const char* OSFormatPointer( const void* const pv )
{
    return ( NULL != pv ? OSFormat( "%0*I64X", (INT)(2 * sizeof( pv )), __int64( pv ) ) : OSTRACENULLPARAM );
}

inline const char* OSFormatSigned( const LONG_PTR l )
{
    return OSFormat( "%I64d", __int64( l ) );
}

inline const char* OSFormatUnsigned( const ULONG_PTR ul )
{
    return OSFormat( "%I64u", __int64( ul ) );
}

const char* OSFormatFileLine( const char* const szFile, const INT iLine );
const char* OSFormatImageVersion();
const char* OSFormatError( const ERR err );

const char* OSFormatString( const char* sz );
const char* SzOSFormatStringW( const WCHAR* wsz );
const char* OSFormatRawData(    const BYTE* const   rgbData,
                                const size_t        cbData,
                                const size_t        cbAddr  = sizeof( void* ),
                                const size_t        cbLine  = 16,
                                const size_t        cbWord  = 1,
                                const size_t        ibData  = 0 );
const char* OSFormatRawDataParam(
                                const BYTE* const   rgbData,
                                const size_t        cbData );


typedef VOID *  POSTRACEREFLOG;

#define ostrlSystemFixed            ((POSTRACEREFLOG)(0x0))
#define ostrlMaxFixed               ((POSTRACEREFLOG)(0x1))

enum sysosirtl
{
    sysosrtlErrorTrackStart = 12,
    sysosrtlErrorThrow      = 13,
};

#define cbSystemRefLogExtra         max( sizeof(ULONG) * 4, sizeof(void*) * 2 )

ERR ErrOSTraceCreateRefLog( IN ULONG cLogSize, IN ULONG cbExtraBytes, OUT POSTRACEREFLOG *ppRefLog );
VOID OSTraceDestroyRefLog( IN POSTRACEREFLOG pLog );

VOID __cdecl OSTraceWriteRefLog(
    IN POSTRACEREFLOG pLog,
    IN LONG NewRefCount,
    IN void * pContext,
    __in_bcount(cbExtraInformation) void * pExtraInformation = NULL,
    IN LONG cbExtraInformation = 0 );

class COSTraceTrackErrors
{
public:
    COSTraceTrackErrors( const CHAR * const szFunc );
    ~COSTraceTrackErrors();
};

BOOL FOSRefTraceErrors();

#ifndef MINIMAL_FUNCTIONALITY




typedef USHORT  FTLTID;
typedef BYTE    FTLTDESC;

typedef ERR ( __stdcall * PfnErrFTLBFlushBuffer )( __inout void * const pvFlushBufferContext, __in const BYTE * const rgbBuffer, __in const ULONG cbBuffer );



const static FTLTID     ftltidMax               = (FTLTID)(0x8000);
const static FTLTID     ftltidSmallMax          = (FTLTID)(0x000F);

INLINE BOOL FFTLValidFTLTID( __in const FTLTID ftltid )
{
    return ftltid != 0 &&
            ftltid < ftltidMax &&
            ftltid != ftltidSmallMax;
}


const static FTLTDESC   ftltdescNone            = (FTLTDESC)(0x00);
const static FTLTDESC   mskFtltdescFixedSize    = (FTLTDESC)(0x0F);


INLINE USHORT CbFTLBIFixedSize( __in const FTLTDESC bTraceDescriptor )
{
    return bTraceDescriptor & mskFtltdescFixedSize;
}


INLINE VOID PrintfDataLine( const BYTE * const rgbBuffer, ULONG cLines )
{
    DWORD ibOffset = 0;
    const DWORD cbLine = 16;
    for( ULONG i = 0; i < cLines; i++ )
    {
        wprintf(L"  0x%p = %02x %02x %02x %02x %02x %02x %02x %02x -  %02x %02x %02x %02x %02x %02x %02x %02x\n",
            rgbBuffer + ibOffset,
            (DWORD) rgbBuffer[ibOffset+0], (DWORD) rgbBuffer[ibOffset+1], (DWORD) rgbBuffer[ibOffset+2], (DWORD) rgbBuffer[ibOffset+3],
            (DWORD) rgbBuffer[ibOffset+4], (DWORD) rgbBuffer[ibOffset+5], (DWORD) rgbBuffer[ibOffset+6], (DWORD) rgbBuffer[ibOffset+7],
            (DWORD) rgbBuffer[ibOffset+8], (DWORD) rgbBuffer[ibOffset+9], (DWORD) rgbBuffer[ibOffset+10], (DWORD) rgbBuffer[ibOffset+11],
            (DWORD) rgbBuffer[ibOffset+12], (DWORD) rgbBuffer[ibOffset+13], (DWORD) rgbBuffer[ibOffset+14], (DWORD) rgbBuffer[ibOffset+15] );
        ibOffset += cbLine;
    }
}


const DWORD DwUtilThreadId();



class CFastTraceLogBuffer
{

private:

    volatile DWORD          m_ibOffset;
    volatile DWORD          m_dwTIDLast;
    DWORD                   m_cbHeader;
    BYTE                    m_rgbBuffer[4 * 1024];

    BOOL                    m_fTracingDisabled;

    PfnErrFTLBFlushBuffer   m_pfnErrFlushBuffer;
    void *                  m_pvFlushBufferContext;

    CCriticalSection        m_crit;




    INLINE void FTLBISetTickBase( const TICK tickBase )
    {
        *((TICK*)m_rgbBuffer) = tickBase;
        Assert( tickBase == TickFTLBITickBase() );
    }

public:
    INLINE TICK TickFTLBITickBase() const
    {
        return (*((TICK*)m_rgbBuffer));
    }
private:

    INLINE void FTLBITraceFlagsTick( __in const TICK tickTrace, __out BYTE * pfTickTrace, __out TICK * pdtick, __out ULONG * pcbTick ) const
    {
        Assert( pfTickTrace );
        Assert( pdtick );
        Assert( pcbTick );

        const TICK dtickTrace = tickTrace - TickFTLBITickBase();
        if( dtickTrace == 0 )
        {
            *pfTickTrace = bHeaderTickHeaderMatch;
            *pdtick = 0;
            *pcbTick = 0;
            return;
        }
        else if ( dtickTrace < (ULONG)0x80 )
        {
            *pfTickTrace = bHeaderTickDelta;
            *pdtick = dtickTrace;
            *pcbTick = 1;
            return;
        }


        *pfTickTrace = bHeaderTickRaw;
        *pdtick = tickTrace;
        *pcbTick = 4;
    }

private:
    
                    const static FTLTID     ftltidShortMac          = (FTLTID)0x0E;
     const static BYTE       bHeaderLongTraceID      = (BYTE)0x0F;
                    const static BYTE       mskHeaderShortTraceID   = (BYTE)0x0F;

     const static BYTE       bHeaderTickHeaderMatch  = (BYTE)0x00;
     const static BYTE       bHeaderTickDelta        = (BYTE)0x40;
                    const static BYTE       bHeaderTickReserved     = (BYTE)0x80;
     const static BYTE       bHeaderTickRaw          = (BYTE)0xC0;
                    const static BYTE       mskHeaderTick           = (BYTE)0xC0;

                    const static BYTE       cbTraceHeader           = 1;
    
public:


    CFastTraceLogBuffer();
    ERR ErrFTLBInit( PfnErrFTLBFlushBuffer pfnFlushBuffer, void * pvFlushBufferContext, const TICK tickInitialBase = TickOSTimeCurrent() );
    void FTLBTerm( const BOOL fAbrubt = fFalse );
    ~CFastTraceLogBuffer();

    BOOL FFTLBInitialized() const
    {
        return ( m_pfnErrFlushBuffer != NULL );
    }

private:


private:
    friend class FTLBufferTraceTestBasic;
    friend class FTLBufferTraceTestComprehensive;
    friend class FTLBufferWrapFlushTestComprehensive;

    ULONG CbFTLBIUsed() const
    {
        return m_ibOffset;
    }

public:

    ULONG CbFTLBHeader() const
    {
        return m_cbHeader;
    }

    ULONG CbFTLBBuffer() const
    {
        return sizeof(m_rgbBuffer);
    }

    const BYTE * PbFTLBuffer() const
    {
        return m_rgbBuffer;
    }

public:

    void SetFTLBTracingDisabled()
    {
        m_fTracingDisabled = fTrue;
    }

    INLINE ERR ErrFTLBTrace( __in const USHORT ftltid, __in const FTLTDESC ftltdesc, __in_bcount(cbTraceData) const BYTE * pbTraceData, __in const DWORD cbTraceData, __in const TICK tickTrace )
    {
        ERR err = 0x0;

        if ( m_fTracingDisabled )
        {
            return err;
        }

        Assert( FFTLBInitialized() );


        m_crit.Enter();



        BYTE                    bHeaderTraceID;
        ULONG                   cbTraceID;
        CompEndianLowSpLos16b   ce_ftltid( ftltid );

        C_ASSERT( ftltidShortMac + 1 == bHeaderLongTraceID );
        C_ASSERT( bHeaderLongTraceID == mskHeaderShortTraceID );

        if ( ftltid <= ftltidShortMac )
        {
            bHeaderTraceID = (BYTE)ftltid;
            cbTraceID = 0;
        }
        else
        {
            bHeaderTraceID = bHeaderLongTraceID;
            Assert( ce_ftltid.Cb() == 1 || ce_ftltid.Cb() == 2 );
            cbTraceID = ce_ftltid.Cb();
        }


        BYTE fHeaderTickTraceFlags;
        ULONG cbTickTrace;
        TICK tickTraceCompressed;
        FTLBITraceFlagsTick( tickTrace, &fHeaderTickTraceFlags, &tickTraceCompressed, &cbTickTrace );
        Assert( 0 == ( ~mskHeaderTick & fHeaderTickTraceFlags ) );



        Assert( cbTraceData == (ULONG)(USHORT)cbTraceData );

        const USHORT cbLeft = (USHORT)cbTraceData - CbFTLBIFixedSize( ftltdesc );
        CompEndianLowSpLos16b   ce_cbLeft( cbLeft );
        const ULONG cbTraceDataFieldSize = cbLeft ? ce_cbLeft.Cb() : 0;
        

        DWORD cbBufferNeeded = cbTraceHeader + cbTraceID + cbTickTrace + cbTraceDataFieldSize + cbTraceData;


        if ( m_ibOffset + cbBufferNeeded > _countof( m_rgbBuffer ) )
        {

            memset( m_rgbBuffer + m_ibOffset, 0, _countof( m_rgbBuffer ) - m_ibOffset );

            (void)m_pfnErrFlushBuffer( m_pvFlushBufferContext, m_rgbBuffer, _countof( m_rgbBuffer ) );

            FTLBISetTickBase( tickTrace );
            Assert( tickTrace == TickFTLBITickBase() );
            
            m_ibOffset = m_cbHeader;

            FTLBITraceFlagsTick( tickTrace, &fHeaderTickTraceFlags, &tickTraceCompressed, &cbTickTrace );
            Assert( 0 == ( ~mskHeaderTick & fHeaderTickTraceFlags ) );

            cbBufferNeeded = cbTraceHeader + cbTraceID + cbTickTrace + cbTraceDataFieldSize + cbTraceData;

            AssertPREFIX( m_ibOffset + cbBufferNeeded < _countof( m_rgbBuffer ) );
        }

        m_dwTIDLast = DwUtilThreadId();


        AssertPREFIX( m_ibOffset + cbBufferNeeded <= _countof( m_rgbBuffer ) );


        BYTE * pbTraceDest = m_rgbBuffer + m_ibOffset;

        Assert( ( bHeaderTraceID & mskHeaderTick ) == 0 );


        Assert( cbTraceID != 0 || ( ftltid <= ftltidShortMac && bHeaderTraceID <= ftltidShortMac ) );
        Assert( 0 == ( bHeaderTraceID & fHeaderTickTraceFlags ) );

        *pbTraceDest = bHeaderTraceID | fHeaderTickTraceFlags;
        pbTraceDest += cbTraceHeader;


        if ( cbTraceID )
        {

            Assert( bHeaderTraceID == bHeaderLongTraceID );

            ce_ftltid.CopyTo( pbTraceDest, cbTraceID );
            pbTraceDest += ce_ftltid.Cb();
        }


        if ( cbTickTrace )
        {
            memcpy( pbTraceDest, &tickTraceCompressed, cbTickTrace );
            pbTraceDest += cbTickTrace;
        }


        if ( cbTraceDataFieldSize )
        {
            Assert( ( pbTraceDest + cbTraceDataFieldSize ) <= ( m_rgbBuffer + _countof( m_rgbBuffer ) ) );

            Assert( ce_cbLeft.Us() != 0 );
            Assert( ce_cbLeft.Cb() == cbTraceDataFieldSize );

            ce_cbLeft.CopyTo( pbTraceDest, cbTraceDataFieldSize );
            pbTraceDest += ce_cbLeft.Cb();
        }

        AssertPREFIX( ( pbTraceDest + cbTraceData ) <= ( m_rgbBuffer + _countof( m_rgbBuffer ) ) );


        memcpy( pbTraceDest, pbTraceData, cbTraceData );
        pbTraceDest += cbTraceData;



        m_ibOffset += cbBufferNeeded;
        Assert( pbTraceDest == m_rgbBuffer + m_ibOffset );


        m_crit.Leave();
    
        return err;
    }

#ifdef DEBUG
    static const BYTE * s_pbPrevPrevPrev;
    static const BYTE * s_pbPrevPrev;
    static const BYTE * s_pbPrev;
#endif

    static const BYTE * PbFTLBParseTraceTick( __in const BYTE fTickInfo, const BYTE * pbTick, __in const TICK tickBase, __out TICK * const ptick );
    static ERR ErrFTLBParseTraceHeader( const BYTE * pbTrace, __out FTLTID * const pftltid, __in const TICK tickBase, __out TICK * const ptick );
    static ERR ErrFTLBParseTraceData( const BYTE * pbTrace, __in const FTLTID ftltid, __in const FTLTDESC ftltdesc, __out ULONG * pcbTraceData, __out const BYTE ** ppbTraceData );

};



enum FASTTRACELOGSTATE
{
    eFTLCreated         = 0,
    eFTLDirty           = 1,
    eFTLClean           = 2,
    eFTLPostProcessed   = 3
};

#include <pshpack1.h>


struct FTLFILEHDR
{
    LittleEndian<ULONG>     le_ulChecksumReserved;

    LittleEndian<ULONG>     le_ulFTLVersionMajor;
    LittleEndian<ULONG>     le_ulFTLVersionMinor;
    LittleEndian<ULONG>     le_ulFTLVersionUpdate;

    LittleEndian<FASTTRACELOGSTATE>     le_tracelogstate;
    BYTE                    rgbReservedBasicHdrInfo[12];

    LittleEndian<QWORD>     le_ftFirstOpen;
    LittleEndian<ULONG>     le_cReOpens;
    LittleEndian<ULONG>     le_cRecoveries;
    LittleEndian<QWORD>     le_ftLastOpen;
    LittleEndian<QWORD>     le_ftLastClose;
    LittleEndian<QWORD>     le_ftPostProcessed;

    LittleEndian<QWORD>     le_cbLastKnownBuffer;

    LittleEndian<QWORD>     le_cWriteFailures;
    BYTE                    rgbReservedFileMaintInfo[40];

    LittleEndian<ULONG>     le_cMaxWriteIOs;
    LittleEndian<ULONG>     le_cMaxWriteBuffers;
    BYTE                    rgbReservedRuntimeStatsInfo[88];

    LittleEndian<ULONG>     le_ulSchemaID;
    LittleEndian<ULONG>     le_ulSchemaVersionMajor;
    LittleEndian<ULONG>     le_ulSchemaVersionMinor;
    LittleEndian<ULONG>     le_ulSchemaVersionUpdate;
    BYTE                    rgbReservedSchemaHdrInfo[16];

    BYTE                    rgbReservedFileTypeAlignment[667-256];

    UnalignedLittleEndian<ULONG>    le_filetype;

};

#include <poppack.h>

C_ASSERT( OffsetOf( FTLFILEHDR, le_ftFirstOpen ) == 32 );
C_ASSERT( OffsetOf( FTLFILEHDR, le_cMaxWriteIOs ) == 128 );
C_ASSERT( OffsetOf( FTLFILEHDR, le_ulSchemaID ) == 224 );
C_ASSERT( OffsetOf( FTLFILEHDR, le_filetype ) == 667 );


typedef struct FTLDescriptor_
{
    ULONG           m_ulSchemaID;

    ULONG           m_ulSchemaVersionMajor;
    ULONG           m_ulSchemaVersionMinor;
    ULONG           m_ulSchemaVersionUpdate;

    INT             m_cShortTraceDescriptors;
    INT             m_cLongTraceDescriptors;

    FTLTDESC        m_rgftltdescShortTraceDescriptors[128];
    FTLTDESC *      m_rgftltdescLongTraceDescriptors;
}
FTLDescriptor;

class IFileSystemAPI;
class IFileSystemConfiguration;
class IFileAPI;
class IOREASON;
class FullTraceContext;

class CFastTraceLog
{

private:

    CFastTraceLogBuffer                 m_ftlb;
    BOOL                                m_fTerminating;


    const static ULONG                  ulFTLVersionMajor = 2;
    const static ULONG                  ulFTLVersionMinor = 2;
    const static ULONG                  ulFTLVersionUpdate = 0;
    const static ULONG                  ibTraceLogHeaderOffset              = 0;
    const static ULONG                  ibPrivateHeaderOffset               = 1024;
    const static ULONG                  ibTraceLogPostProcessedHeaderOffset = 2048;
    const static ULONG                  ibPrivatePostProcessedHeaderOffset  = 3072;
    INT                                 m_cbWriteBufferMax;
    IOREASON *                          m_piorTraceLog;
    IFileSystemConfiguration * const    m_pfsconfig;
    IFileSystemAPI *                    m_pfsapi;
    IFileAPI *                          m_pfapiTraceLog;
    BYTE *                              m_pbTraceLogHeader;

    ERR ErrFTLICheckVersions();
    ERR ErrFTLIFlushHeader();

    const static FTLTDESC   ftltdescDefaultDescriptor = ftltdescNone;
    FTLDescriptor           m_ftldesc;

    FTLTDESC FtltdescFTLIGetDescriptor( __in const USHORT usTraceID ) const;



    const static INT        fBufferAvailable    = 0x0;
    const static INT        fBufferInUse        = 0x1;
    const static INT        fBufferFlushed      = 0x2;
    const static INT        fBufferDone         = 0x3;
    const static INT        ibufUninitialized   = -1;
    QWORD                   m_ibWriteBufferCurrent;
    volatile INT            m_ipbWriteBufferCurrent;
    volatile INT            m_cbWriteBufferFull;
    CSXWLatch               m_sxwlFlushing;
    CSemaphore              m_semBuffersInUse;
    INT                     m_rgfBufferState[80];
    BYTE *                  m_rgpbWriteBuffers[80];

    void FTLIResetWriteBuffering( void );
    INT IFTLIGetFlushBuffer();
    static ERR ErrFTLFlushBuffer( __inout void * const pvFlushBufferContext, __in_bcount(cbBuffer) const BYTE * const rgbBuffer, __in const ULONG cbBuffer );
    ERR ErrFTLIFlushBuffer( __in_bcount(cbBuffer) const BYTE * rgbBuffer, __in const INT cbBuffer, const BOOL fForceTerm );
    static void FTLFlushBufferComplete( const ERR           err,
                                        IFileAPI* const     pfapi,
                                        const FullTraceContext& ptc,
                                        const QWORD grbitQOS,
                                        const QWORD         ibOffset,
                                        const DWORD         cbData,
                                        const BYTE* const       pbData,
                                        const DWORD_PTR     keyIOComplete );

    volatile LONG           m_cOutstandingIO;
    volatile LONG           m_cOutstandingIOHighWater;



public:
    class CFTLReader;

private:
    friend CFTLReader;

    CFTLReader *            m_pftlr;

    ERR ErrFTLIReadBuffer( __out_bcount(cbBuffer) void * pvBuffer, __in QWORD ibOffset, __in ULONG cbBuffer );


public:



    CFastTraceLog(  const FTLDescriptor * const         pftldesc,
                    IFileSystemConfiguration * const    pfsconfig = NULL );

    enum FTLInitFlags
    {
        ftlifNone               = 0,

        ftlifNewlyCreated       = 1,
        ftlifReOpenExisting     = 2,
        ftlifReCreateOverwrite  = 3,
        ftlifmskFileDisposition = ( ftlifNewlyCreated | ftlifReOpenExisting | ftlifReCreateOverwrite ),

        ftlifKeepStats          = 0x100,

    };

    ERR ErrFTLInitWriter( __in_z const WCHAR * wszTraceLogFile, IOREASON * pior, __in const FTLInitFlags ftlif );
    ERR ErrFTLInitReader( __in_z const WCHAR * wszTraceLogFile, IOREASON * pior, __in const FTLInitFlags ftlif, __out CFTLReader ** ppftlr );

    void FTLTerm();


    void SetFTLDisabled();
    ERR ErrFTLTrace( __in const USHORT usTraceID, __in_bcount(cbTrace) const BYTE * pbTrace, __in const ULONG cbTrace, __in const TICK tickTrace = TickOSTimeCurrent() );


    FTLFILEHDR *    PftlhdrFTLTraceLogHeader()      { return (FTLFILEHDR *)( m_pbTraceLogHeader + ibTraceLogHeaderOffset ); }
    void *          PvFTLPrivatePostHeader()        { return m_pbTraceLogHeader + ibPrivatePostProcessedHeaderOffset; }
    ULONG           CbFullHeader()                  { return ibPrivatePostProcessedHeaderOffset + 1024; }
    ULONG           CbPrivateHeader()               { return 1024; }

    ERR ErrFTLDumpHeader();
    ERR ErrFTLSetPostProcessHeader( void * pv1kHeader );

    struct BFFTLFilePostProcessHeader
    {
        LittleEndian<ULONG>     le_cifmp;
        LittleEndian<ULONG>     le_mpifmpcpg[255];
    };

    struct FTLTrace
    {
        QWORD           ibBookmark;
        USHORT          ftltid;
        TICK            tick;
        const BYTE *    pbTraceData;
        ULONG           cbTraceData;
    };

    class CFTLReader
    {
        friend class CFastTraceLog;

    private:
        CFastTraceLog *     m_pftl;


        ULONG               m_cbReadBufferSize;
        BOOL                m_fKeepStats;
        const static INT    ibufUninitialized = -1;

        typedef struct ReadBuffers_
        {
            QWORD           ibBookmark;
            BYTE *          pbReadBuffer;
        } ReadBuffers;
        ReadBuffers         m_rgbufReadBuffers[10];
        INT                 m_ibufReadLast;
        QWORD               m_ibBookmarkNext;
        TICK                m_tickBase;


        QWORD               m_cReadIOs;

        QWORD               m_cFullBuffers;
        QWORD               m_cPartialBuffers;
        QWORD               m_cbPartialEmpty;
        QWORD               m_cBlankBuffers;
        QWORD               m_cbBlankEmpty;

#ifdef DEBUG
        INT                 m_ibufTracePrevPrev;
        FTLTrace            m_ftltracePrevPrev;
        INT                 m_ibufTracePrev;
        FTLTrace            m_ftltracePrev;
#endif

        ERR ErrFTLIFillBuffer( __in const QWORD ibBookmarkRead );

        CFTLReader( CFastTraceLog * pftl, BOOL fKeepStats );
        ~CFTLReader( );

    public:

        ERR ErrFTLGetNextTrace( FTLTrace * pftltrace );

        ERR ErrFTLDumpStats();

    };

    TICK TickFTLTickBaseCurrent() const;

};


enum IOREASONPRIMARY : BYTE;
enum IOREASONSECONDARY : BYTE;
enum IOREASONTERTIARY : BYTE;
enum IOREASONUSER : BYTE;
enum IOREASONFLAGS : BYTE;

#define iorpNone        ((IOREASONPRIMARY)0x0)
#define iorsNone        ((IOREASONSECONDARY)0x0)
#define iortNone        ((IOREASONTERTIARY)0x0)
#define ioruNone        ((IOREASONUSER)0x0)
#define iorfNone        ((IOREASONFLAGS)0x0)

class IOREASON {

private:

    union
    {
        struct
        {
            IOREASONPRIMARY     m_iorp : 8;
            IOREASONSECONDARY   m_iors : 4;
            IOREASONTERTIARY    m_iort : 4;
            IOREASONUSER        m_ioru : 8;
            IOREASONFLAGS       m_iorf : 8;
        };
        DWORD m_ior;
    };

public:
    IOREASON( const IOREASON& rhs )
    {
        m_ior = rhs.m_ior;
    }

    const IOREASON& operator=( const IOREASON& rhs )
    {
        m_ior = rhs.m_ior;
        Assert( Iorp() == rhs.Iorp() );
        Assert( Iors() == rhs.Iors() );
        Assert( Iort() == rhs.Iort() );
        Assert( Ioru() == rhs.Ioru() );
        Assert( Iorf() == rhs.Iorf() );
        return *this;
    }
    
    IOREASONPRIMARY Iorp( ) const
    {
        return m_iorp;
    }

    IOREASONSECONDARY Iors( ) const
    {
        return m_iors;
    }

    IOREASONTERTIARY Iort( ) const
    {
        return m_iort;
    }

    IOREASONUSER Ioru( ) const
    {
        return m_ioru;
    }

    IOREASONFLAGS Iorf( ) const
    {
        return m_iorf;
    }

    DWORD DwIor() const
    {
        return m_ior;
    }

    void SetIorp( IOREASONPRIMARY iorp )
    {
        Assert( iorp <= 0x7f );
        m_iorp = iorp;
    }

    void SetIors( IOREASONSECONDARY iors )
    {
        Assert( iors <= 0x0f );
        m_iors = iors;
    }

    void SetIort( IOREASONTERTIARY iort )
    {
        Assert( iort <= 0x0f );
        m_iort = iort;
    }

    void SetIoru( IOREASONUSER ioru )
    {
        Assert( ioru <= 0xff );
        m_ioru = ioru;
    }

    void AddFlag( IOREASONFLAGS iorf )
    {
        m_iorf = (IOREASONFLAGS)(iorf | (BYTE)m_iorf);
    }

    void Clear( )
    {
        m_ior = 0;
    }

    bool FValid( void ) const
    {
        if ( Iorp( ) >= 0x80 )
        {
            return fFalse;
        }

        return fTrue;
    }

    IOREASON(
    enum IOREASONPRIMARY iorp,
    enum IOREASONSECONDARY iors,
    enum IOREASONTERTIARY iort,
    enum IOREASONUSER ioru,
    enum IOREASONFLAGS iorf ) :
        m_iorp( iorp ), m_iors( iors ), m_iort( iort ), m_ioru( ioru ), m_iorf( iorf )
    {
        Assert( iorp <= 0x7f );
        Assert( FValid( ) );
    }

    IOREASON(
    enum IOREASONPRIMARY iorp,
    enum IOREASONSECONDARY iors,
    enum IOREASONUSER ioru,
    enum IOREASONFLAGS iorf ) :
        m_iorp( iorp ), m_iors( iors ), m_iort( iortNone ), m_ioru( ioru ), m_iorf( iorf )
    {
        Assert( iorp <= 0x7f );
        Assert( FValid( ) );
    }

    IOREASON(
    enum IOREASONPRIMARY iorp,
    enum IOREASONUSER ioru,
    enum IOREASONFLAGS iorf ) :
        m_iorp( iorp ), m_iors( iorsNone ), m_iort( iortNone ), m_ioru( ioru ), m_iorf( iorf )
    {
        Assert( iorp <= 0x7f );
        Assert( FValid( ) );
    }

    IOREASON(
    enum IOREASONPRIMARY iorp,
    enum IOREASONSECONDARY iors,
    enum IOREASONTERTIARY iort ) :
        m_iorp( iorp ), m_iors( iors ), m_ioru( ioruNone ), m_iort( iort ), m_iorf( iorfNone )
    {
        Assert( iorp <= 0x7f );
        Assert( FValid( ) );
    }

    IOREASON(
    enum IOREASONPRIMARY iorp,
    enum IOREASONSECONDARY iors ) :
        m_iorp( iorp ), m_iors( iors ), m_iort( iortNone ), m_ioru( ioruNone ), m_iorf( iorfNone )
    {
        Assert( iorp <= 0x7f );
        Assert( FValid( ) );
    }

    IOREASON(
    enum IOREASONPRIMARY iorp,
    enum IOREASONUSER ioru ) :
        m_iorp( iorp ), m_iors( iorsNone ), m_iort( iortNone ), m_ioru( ioru ), m_iorf( iorfNone )
    {
        Assert( iorp <= 0x7f );
        Assert( FValid( ) );
    }

    IOREASON(
    enum IOREASONPRIMARY iorp ) :
        m_iorp( iorp ), m_iors( iorsNone ), m_iort( iortNone ), m_ioru( ioruNone ), m_iorf( iorfNone )
    {
        Assert( iorp <= 0x7f );
        Assert( FValid( ) );
    }

    IOREASON(
    enum IOREASONPRIMARY iorp,
    enum IOREASONFLAGS iorf ) :
        m_iorp( iorp ), m_iors( iorsNone ), m_iort( iortNone ), m_ioru( ioruNone ), m_iorf( iorf )
    {
        Assert( iorp <= 0x7f );
        Assert( FValid( ) );
    }

    ~IOREASON( ) {}

    IOREASON( )
    {
        m_iorp = iorpNone;
        m_iors = iorsNone;
        m_iort = iortNone;
        m_ioru = ioruNone;
        m_iorf = iorfNone;
    }

    bool operator==( const IOREASON& rhs ) const
    {
        return ( m_ior == rhs.m_ior );
    }
};

static_assert( sizeof( IOREASON ) == 4, "IOREASON should be 4-bytes" );

struct OPERATION_CONTEXT
{
    DWORD   dwUserID;
    BYTE    nOperationID;
    BYTE    nOperationType;
    BYTE    nClientType;
    BYTE    fFlags;
};

struct USER_CONTEXT_DESC
{
    static const INT cchStrMax = 32;
    static const INT cbStrMax = cchStrMax * sizeof( char );

    GUID    guidActivityId;

    char    szClientComponent[ cchStrMax ];
    char    szClientAction[ cchStrMax ];
    char    szClientActionContext[ cchStrMax ];

    static ERR ErrCopyDescString( char* const szDest, const char* const szSrc, const INT cbSrc );
};

#define OC_bitInternalUser  0x80000000
#define OCUSER_SYSTEM       ( OC_bitInternalUser | 0x7fffffff )

#define IOR IOREASON

const BYTE  pocNone = 0xff;
inline bool FParentObjectClassSet( BYTE nParentObjectClass )
{
    return nParentObjectClass != pocNone;
}

class UserTraceContext
{
public:
    static const USER_CONTEXT_DESC s_ucdNil;

    USER_CONTEXT_DESC*  pUserContextDesc;
    OPERATION_CONTEXT   context;
    DWORD               dwCorrelationID;
    DWORD               dwIOSessTraceFlags;

    UserTraceContext()
    {
        pUserContextDesc = const_cast<USER_CONTEXT_DESC*>( &s_ucdNil );
        memset( &context, 0, sizeof( context ) );
        dwCorrelationID = 0;
        dwIOSessTraceFlags = false;
    }

    UserTraceContext( DWORD dwUserId )
    {
        pUserContextDesc = const_cast<USER_CONTEXT_DESC*>( &s_ucdNil );
        memset( &context, 0, sizeof( context ) );
        context.dwUserID = dwUserId;
        dwCorrelationID = 0;
        dwIOSessTraceFlags = false;
    }

    UserTraceContext( const UserTraceContext& rhs, bool fCopyUserContextDesc );

    UserTraceContext( UserTraceContext&& rhs )
    {
#ifdef DEBUG
        pUserContextDesc = rhs.pUserContextDesc;
        context = rhs.context;
        dwCorrelationID = rhs.dwCorrelationID;
        dwIOSessTraceFlags = rhs.dwIOSessTraceFlags;

        rhs.pUserContextDesc = const_cast<USER_CONTEXT_DESC*>( &s_ucdNil );
#else
        EnforceSz( false, "DisallowedUserTraceContextMoveCtor" );
#endif
    }

    bool FUserContextDescInitialized() const        { return pUserContextDesc != &s_ucdNil; }

    ~UserTraceContext()
    {
        if ( FUserContextDescInitialized() )
        {
            delete pUserContextDesc;
        }
    }

    ERR ErrLazyInitUserContextDesc();
    void DeepCopy( const UserTraceContext& utcFrom );

    void Clear()
    {
        if ( FUserContextDescInitialized() )
        {

            pUserContextDesc->szClientComponent[ 0 ] = 0;
            pUserContextDesc->szClientAction[ 0 ] = 0;
            pUserContextDesc->szClientActionContext[ 0 ] = 0;
            memset( &pUserContextDesc->guidActivityId, 0, sizeof( pUserContextDesc->guidActivityId ) );
        }

        memset( &context, 0, sizeof( context ) );
        dwCorrelationID = 0;
    }

    inline static UserTraceContext MakeInternalContext( DWORD dwUserId )
    {
        return UserTraceContext( dwUserId );
    }

private:
    UserTraceContext( const UserTraceContext& rhs );
    const UserTraceContext& operator=( const UserTraceContext& rhs );
};


const ULONG dwEngineObjidNone = 0xFFFFFFFF;
 
class TraceContext
{
public:
    IOREASON                    iorReason;
    ULONG                       dwEngineObjid;
    BYTE                        nParentObjectClass;

    TraceContext() :
        nParentObjectClass( pocNone ), dwEngineObjid( dwEngineObjidNone )
    {
        iorReason.Clear();
    }

    TraceContext( const TraceContext& rhs )
    {
        iorReason = rhs.iorReason;
        nParentObjectClass = rhs.nParentObjectClass;
        dwEngineObjid = rhs.dwEngineObjid;
    }

    ~TraceContext() {}

    const TraceContext& operator=( const TraceContext& rhs )
    {
        iorReason = rhs.iorReason;
        nParentObjectClass = rhs.nParentObjectClass;
        dwEngineObjid = rhs.dwEngineObjid;
        return *this;
    }

    bool operator==( const TraceContext& rhs ) const
    {
        return ( iorReason == rhs.iorReason && nParentObjectClass == rhs.nParentObjectClass && dwEngineObjid == rhs.dwEngineObjid );
    }

    inline void SetDwEngineObjid( DWORD dwEngineObjidNew )
    {
        dwEngineObjid = dwEngineObjidNew;
    }

    bool FEmpty() const
    {
        return ( iorReason.FValid() && nParentObjectClass == pocNone && dwEngineObjid == dwEngineObjidNone ) ? true : false;
    }

    void Clear()
    {
        iorReason.Clear();
        nParentObjectClass = pocNone;
        dwEngineObjid = dwEngineObjidNone;
    }
};

INLINE bool FEngineObjidSet( DWORD dwEngineObjid )
{
    return dwEngineObjid != dwEngineObjidNone;
}

class FullTraceContext
{
public:
    UserTraceContext utc;
    TraceContext etc;

    FullTraceContext()
    {
    }

    FullTraceContext( const FullTraceContext& rhs, bool fCopyUserContextDesc ) :
        utc( rhs.utc, fCopyUserContextDesc ),
        etc( rhs.etc )
    {
    }

    void DeepCopy( const UserTraceContext& _utc, const TraceContext& _etc )
    {
        utc.DeepCopy( _utc );
        etc = _etc;
    }

    void Clear()
    {
        utc.Clear();
        etc.Clear();
    }

private:
    FullTraceContext( FullTraceContext& rhs );
    const FullTraceContext& operator= ( const FullTraceContext& rhs );
};


template< typename TFnGetEtc >
class _TraceContextScope
{
    TraceContext*       m_ptcCurr;
    TraceContext        m_tcSaved;

public:
    _TraceContextScope( const TFnGetEtc& tfnGetEtc )
    {
        m_ptcCurr = tfnGetEtc();
        m_tcSaved = *m_ptcCurr;
    }

    _TraceContextScope( TFnGetEtc tfnGetEtc, IOREASONPRIMARY iorp, IOREASONSECONDARY iors = iorsNone, IOREASONTERTIARY iort = iortNone, IOREASONFLAGS iorf = iorfNone )
    {
        m_ptcCurr = tfnGetEtc();
        m_tcSaved = *m_ptcCurr;

        iorp != iorpNone ? m_ptcCurr->iorReason.SetIorp( iorp ) : 0;
        iors != iorsNone ? m_ptcCurr->iorReason.SetIors( iors ) : 0;
        iort != iortNone ? m_ptcCurr->iorReason.SetIort( iort ) : 0;
        m_ptcCurr->iorReason.AddFlag( iorf );
    }

    _TraceContextScope( TFnGetEtc tfnGetEtc, IOREASONSECONDARY iors, IOREASONTERTIARY iort = iortNone, IOREASONFLAGS iorf = iorfNone )
    {
        m_ptcCurr = tfnGetEtc();
        m_tcSaved = *m_ptcCurr;

        iors != iorsNone ? m_ptcCurr->iorReason.SetIors( iors ) : 0;
        iort != iortNone ? m_ptcCurr->iorReason.SetIort( iort ) : 0;
        m_ptcCurr->iorReason.AddFlag( iorf );
    }

    _TraceContextScope( TFnGetEtc tfnGetEtc, IOREASONTERTIARY iort, IOREASONFLAGS iorf = iorfNone )
    {
        m_ptcCurr = tfnGetEtc();
        m_tcSaved = *m_ptcCurr;

        iort != iortNone ? m_ptcCurr->iorReason.SetIort( iort ) : 0;
        m_ptcCurr->iorReason.AddFlag( iorf );
    }

    void Reset()
    {
        *m_ptcCurr = m_tcSaved;
    }

    ~_TraceContextScope()               { Reset(); }
    TraceContext* operator->()          { return m_ptcCurr; }
    TraceContext& operator*()           { return *m_ptcCurr; }
    TraceContext* Ptc()                 { return m_ptcCurr; }


    _TraceContextScope( _TraceContextScope&& rvalue )
    {
#ifdef DEBUG
        m_ptcCurr = rvalue.m_ptcCurr;
        m_tcSaved = rvalue.m_tcSaved;

        rvalue.m_tcSaved = *m_ptcCurr;
#else
        EnforceSz( false, "DisallowedTraceContextScopeCtor" );
#endif
    }

    _TraceContextScope( const _TraceContextScope& tc ) = delete;
    const _TraceContextScope& operator=( const _TraceContextScope& tc ) = delete;
};

class GetCurrUserTraceContext
{
    const UserTraceContext* m_putcTls;
    const UserTraceContext  m_utcSysDefault;

    public:
        GetCurrUserTraceContext();
        inline const UserTraceContext& Utc() const      { return ( m_putcTls != NULL ? *m_putcTls : m_utcSysDefault ); }
        inline const UserTraceContext* operator->( ) const  { return &Utc(); }
};

const UserTraceContext* TLSSetUserTraceContext( const UserTraceContext* ptcUser );
const TraceContext* PetcTLSGetEngineContext();
const UserTraceContext* PutcTLSGetUserContext();

struct TLS;
const TraceContext* PetcTLSGetEngineContextCached( TLS *ptlsCached );

class TLSGetEtcFunctor
{
public:
    TraceContext* operator()() const
    {
        return const_cast<TraceContext*>( PetcTLSGetEngineContext() );
    }
};

class TraceContextScope : public _TraceContextScope< TLSGetEtcFunctor >
{
public:
    TraceContextScope() : _TraceContextScope( TLSGetEtcFunctor{} )
    {}

    TraceContextScope( IOREASONPRIMARY iorp, IOREASONSECONDARY iors = iorsNone, IOREASONTERTIARY iort = iortNone, IOREASONFLAGS iorf = iorfNone )
        : _TraceContextScope( TLSGetEtcFunctor{}, iorp, iors, iort, iorf )
    {}

    TraceContextScope( IOREASONSECONDARY iors, IOREASONTERTIARY iort = iortNone, IOREASONFLAGS iorf = iorfNone )
        : _TraceContextScope( TLSGetEtcFunctor{}, iors, iort, iorf )
    {}

    TraceContextScope( IOREASONTERTIARY iort, IOREASONFLAGS iorf = iorfNone )
        : _TraceContextScope( TLSGetEtcFunctor{}, iort, iorf )
    {}

    TraceContextScope( TraceContextScope&& rvalue ) : _TraceContextScope( std::move( rvalue ) )
    {}
};

ULONG OpTraceContext();

inline const TraceContext& TcCurr()
{
    return *PetcTLSGetEngineContext();
}

#endif

#endif


