// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_TRACE_HXX_INCLUDED
#define _OS_TRACE_HXX_INCLUDED

// Required for std::move()
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
        ULONG_PTR   m_ul;           //  for user data
        BOOL        m_fEnabled;     //  is this tag enabled?
};


//  must match JET_PFNTRACEEMIT
//
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


//  Info Strings
//
//  "info strings" are garbage-collected strings that can be used to greatly
//  simplify tracing code.  these strings may be created by any one of the
//  debug info calls and passed directly into OSFormat in its variable argument
//  list.  the strings can then be printed via the %s format specification.
//  these strings make code like this possible:
//
//      OSTrace( ostlLow,
//               OSFormat( "IRowset::QueryInterface() called for IID %s (%s)",
//                         OSFormatGUID( iid ),
//                         OSFormatIID( iid ) ) );
//
//  OSFormatGUID() would return an info string containing a sprintf()ed IID
//  and OSFormatIID() would return a pointer to a const string from our
//  image.  note that the programmer doesn't have to know which type of string
//  is returned!  the programmer also doesn't have to worry about out-of-memory
//  conditions because these will cause the function to return NULL which
//  printf will safely convert into "(null)"
//
//  an info string may be directly allocated via OSAllocInfoString( cch ) where
//  cch is the maximum string length desired not including the null terminator.
//  a null terminator is automatically added at the last position in the string
//  to facilitate the use of _snprintf
//
//  because info strings are garbage-collected, cleanup must be triggered
//  somewhere.  it is important to know when this happens:
//
//      -  at the end of OSTrace()
//      -  manually via OSFreeInfoStrings()
//      -  on image unload

char* OSAllocInfoString( const size_t cch );
WCHAR* OSAllocInfoStringW( const size_t cch );
void OSFreeInfoStrings();


//  Tracing
//
//  OSTrace() provides a method for emitting useful information to the debugger
//  during operation.  Several detail levels are supported to restrict the flow
//  of information as desired.  Here are the current conventions tied to each
//  trace level:
//
//      ostlNone
//          - used to disable individual traces
//      ostlLow
//          - exceptions, asserts, error traps
//      ostlMedium
//          - method execution summary
//          - calls yielding a fatal result
//      ostlHigh
//          - method execution details
//      ostlVeryHigh
//          - calls yielding a failure result
//      ostlFull
//          - method internal operations
//
//  OSTraceIndent() provides a method for setting the indent level of traces
//  emitted to the debugger for the current thread.  A positive value will
//  increase the indent level by that number of indentations.  A negative value
//  will reduce the indent level by that number of indentations.  A value of
//  zero will reset the indent level to the default.
//
//  Tracing can also be controlled within an individual trace.  Inserting the
//  control string OSTRACEINDENTSZ into a trace string will cause subsequent
//  lines in that trace string to be indented by one more level than the indent
//  level for the current thread.  Similarly, inserting the control string
//  OSTRACEUNINDENTSZ into a trace string will cause subsequent lines in that
//  trace string to be indented by one fewer level than the indent level for
//  the current thread.

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


//  Trace Formatting
//
//  OSFormat() returns an Info String containing the specified data formatted to
//  the printf() specifications in the format string.  This function is used to
//  generate most of the data that are fed to the format string in OSTrace()
//  either directly or indirectly via other more specialized OSFormat*()
//  functions.

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
// This has the return parameter, b/c it differs from the W designator.
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

//
//  Ref Log
//

typedef VOID *  POSTRACEREFLOG;

#define ostrlSystemFixed            ((POSTRACEREFLOG)(0x0)) //  A basic ref log for system, w/ 16 bytes extra info.
#define ostrlMaxFixed               ((POSTRACEREFLOG)(0x1))

enum sysosirtl
{
    sysosrtlErrorTrackStart = 12,
    sysosrtlErrorThrow      = 13,
};

//  this is how much the client expects to be able to stuff in the system ref log
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
    //  should make other kinds of .ctors illegal
};

BOOL FOSRefTraceErrors();

#ifndef MINIMAL_FUNCTIONALITY
//
//  Fast Trace Logging / FTL
//

//  This is organized into a few objects 
//
//  CFastTraceLog / CFastTraceLogBuffer / FTLTID / FTLTDESC / etc


//
//  FTL Primitive Types
//

typedef USHORT  FTLTID;     //  ftltid (Fast Trace Log Trace ID)
typedef BYTE    FTLTDESC;   //  ftltdesc (Fast Trace Log Trace DESCriptor)

typedef ERR ( __stdcall * PfnErrFTLBFlushBuffer )( __inout void * const pvFlushBufferContext, __in const BYTE * const rgbBuffer, __in const ULONG cbBuffer );


//
//  FTL Trace ID constants and functions
//

const static FTLTID     ftltidMax               = (FTLTID)(0x8000); //  we support ~32k trace IDs
const static FTLTID     ftltidSmallMax          = (FTLTID)(0x000F); //  the first ftltid value that will not get a short trace ID when traced

INLINE BOOL FFTLValidFTLTID( __in const FTLTID ftltid )
{
    return ftltid != 0 &&
            ftltid < ftltidMax &&
            // Actually, it could equal ftltidSmallMax, it would just be stored as a
            // medium format trace ID ... consider removing this.
            ftltid != ftltidSmallMax;
}

//
//  FTL Trace Descriptor constants and functions
//

const static FTLTDESC   ftltdescNone            = (FTLTDESC)(0x00); //  this is the empty descriptor, has no fixed size, by default stores cb after, etc
const static FTLTDESC   mskFtltdescFixedSize    = (FTLTDESC)(0x0F); //  defines the portion of the descriptor reserved to define fixed size


INLINE USHORT CbFTLBIFixedSize( __in const FTLTDESC bTraceDescriptor )
{
    return bTraceDescriptor & mskFtltdescFixedSize;
}

//  Silly helper function to dump a line of data

INLINE VOID PrintfDataLine( const BYTE * const rgbBuffer, ULONG cLines )
{
    DWORD ibOffset = 0; // line offset in bytes
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


//  Fwd decls required for ErrFTL[B]Trace ... as tracing is kind of ahead of it's time.
const DWORD DwUtilThreadId();


//  This class implements the tracing buffer and the tracing of all fields that are 
//  common to all traces (such as the Trace ID, time/TICK, size of trace, etc).

class CFastTraceLogBuffer   //  ftlb
{

private:

    volatile DWORD          m_ibOffset;
    //  well we'll be unable to re-establish an order of traces w/o a TID *once we move to multi-buffer*
    volatile DWORD          m_dwTIDLast;
    DWORD                   m_cbHeader;
    BYTE                    m_rgbBuffer[4 * 1024];

    BOOL                    m_fTracingDisabled;

    PfnErrFTLBFlushBuffer   m_pfnErrFlushBuffer;
    void *                  m_pvFlushBufferContext;

    CCriticalSection        m_crit;

    //
    //  The FTL Trace Format:
    //

    //  Trace Header:
    //
    //  |11111111|22222222|33333333|44444444|55555555|
    //  | bHeader|        |        |        |        |
    //  |    TrID| - This low nibble / 4 bits is for the short FTL Trace IDs / ftltid (i.e. <= ftltidShortMac)
    //  |TF      | - These two bits are the Tick Compression Flags
    //  |  ??    | - This is reserved space, will likely be used for TID/Thread & cb/compaction scheme
    //
    //  Trace Common Fields:
    //
    //  |11111111|22222222|33333333|44444444|55555555|
    //      Extended FTL Trace ID - Then depending upon if bHeader & mskHeaderShortTraceID == bHeaderLongTraceID
    //           | <nothing>
    //           [0|ftltid]
    //           [1| Longer ftltid ]
    //      Time / TICK Field - Then depending upon bHeader & mskHeaderTick == bHeaderTickHeaderMatch   | bHeaderTickDelta | bHeaderTickRaw
    //           | <nothing>
    //           [  dtick ]
    //           [    Full Raw Tick (4 bytes)        ]
    //      CbUserTraceData - Then depending upon ftltdesc[indexed by ftltid] fixed size
    //           | <nothing>
    //           [ cbUser ]
    //
    //  Trace User Data:
    //
    //      This is remainder of the user's specific trace data.
    //

    //  Ramifications:
    //   - A trace must be at least 1 byte.
    //   - Often the bHeaderTickHeaderMatch will allow us to not log any TICK / time info.
    //   - We do not support more than 31k traces
    //   - We do not support tracing more than 255 bytes of user payload data.
    //   - The first byte of any valid trace is guaranteed to be non-zero.
    //

    INLINE void FTLBISetTickBase( const TICK tickBase )
    {
        *((TICK*)m_rgbBuffer) = tickBase;
        Assert( tickBase == TickFTLBITickBase() );
    }

    //  public for testing only
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

        // else uncompressible

        *pfTickTrace = bHeaderTickRaw;
        *pdtick = tickTrace;
        *pcbTick = 4;
    }

private:
    
                    const static FTLTID     ftltidShortMac          = (FTLTID)0x0E;
    /* PERSISTED */ const static BYTE       bHeaderLongTraceID      = (BYTE)0x0F;
                    const static BYTE       mskHeaderShortTraceID   = (BYTE)0x0F;

    /* PERSISTED */ const static BYTE       bHeaderTickHeaderMatch  = (BYTE)0x00;
    /* PERSISTED */ const static BYTE       bHeaderTickDelta        = (BYTE)0x40;
                    const static BYTE       bHeaderTickReserved     = (BYTE)0x80;
    /* PERSISTED */ const static BYTE       bHeaderTickRaw          = (BYTE)0xC0;
                    const static BYTE       mskHeaderTick           = (BYTE)0xC0;

                    const static BYTE       cbTraceHeader           = 1;
    
public:

    //
    //  FTL Buffer Init/Term
    //

    CFastTraceLogBuffer();
    ERR ErrFTLBInit( PfnErrFTLBFlushBuffer pfnFlushBuffer, void * pvFlushBufferContext, const TICK tickInitialBase = TickOSTimeCurrent() );
    void FTLBTerm( const BOOL fAbrubt = fFalse );
    ~CFastTraceLogBuffer();

    BOOL FFTLBInitialized() const
    {
        return ( m_pfnErrFlushBuffer != NULL );
    }

private:

    //
    //  FTL Buffer Tracing
    //

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
        ERR err = 0x0/*JET_errSuccess*/;

        if ( m_fTracingDisabled )
        {
            return err;
        }

        Assert( FFTLBInitialized() );

        //  acquire the right to trace to the trace buffer

        m_crit.Enter();

        //  Pre-trace calculations
        //

        //  calculate ftltid / TraceID compression

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

        //  calculate tickTrace compression

        BYTE fHeaderTickTraceFlags;
        ULONG cbTickTrace;
        TICK tickTraceCompressed;
        FTLBITraceFlagsTick( tickTrace, &fHeaderTickTraceFlags, &tickTraceCompressed, &cbTickTrace );
        Assert( 0 == ( ~mskHeaderTick & fHeaderTickTraceFlags ) );


        //  calculate size compression

        Assert( cbTraceData == (ULONG)(USHORT)cbTraceData );    // check for truncation

        const USHORT cbLeft = (USHORT)cbTraceData - CbFTLBIFixedSize( ftltdesc );
        CompEndianLowSpLos16b   ce_cbLeft( cbLeft );
        const ULONG cbTraceDataFieldSize = cbLeft ? ce_cbLeft.Cb() : 0;
        
        //  identify required buffer
        //

        DWORD cbBufferNeeded = cbTraceHeader + cbTraceID + cbTickTrace + cbTraceDataFieldSize + cbTraceData;

        //  try to acquire the required buffer
        //

        if ( m_ibOffset + cbBufferNeeded > _countof( m_rgbBuffer ) )
        {
            //  buffer overflow, need to recharge the FTL buffer ...

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

        //  should have required buffer now

        AssertPREFIX( m_ibOffset + cbBufferNeeded <= _countof( m_rgbBuffer ) );

        //  Trace
        //

        BYTE * pbTraceDest = m_rgbBuffer + m_ibOffset;

        //  should not be data in way of where we're putting these tick flag data
        Assert( ( bHeaderTraceID & mskHeaderTick ) == 0 );

        //  trace the traceid / ftltid and header (with compression schemes)

        Assert( cbTraceID != 0 || ( ftltid <= ftltidShortMac && bHeaderTraceID <= ftltidShortMac ) );
        Assert( 0 == ( bHeaderTraceID & fHeaderTickTraceFlags ) );

        *pbTraceDest = bHeaderTraceID | fHeaderTickTraceFlags;
        pbTraceDest += cbTraceHeader;

        //  trace residual TraceID / ftltid (if any)

        if ( cbTraceID )
        {
            //  residual TraceID left to be traced ...

            Assert( bHeaderTraceID == bHeaderLongTraceID );

            ce_ftltid.CopyTo( pbTraceDest, cbTraceID );
            pbTraceDest += ce_ftltid.Cb();
        }

        //  trace compressed tickTrace (if any)

        if ( cbTickTrace )
        {
            //  residual tick time to be traced
            memcpy( pbTraceDest, &tickTraceCompressed, cbTickTrace );
            pbTraceDest += cbTickTrace;
        }

        //  trace compressed data size field (if any)

        if ( cbTraceDataFieldSize )
        {
            //  ensure we still have room for the data size field
            Assert( ( pbTraceDest + cbTraceDataFieldSize ) <= ( m_rgbBuffer + _countof( m_rgbBuffer ) ) );

            Assert( ce_cbLeft.Us() != 0 );
            Assert( ce_cbLeft.Cb() == cbTraceDataFieldSize );

            //  residual cbLeft left to be traced ...
            ce_cbLeft.CopyTo( pbTraceDest, cbTraceDataFieldSize );
            pbTraceDest += ce_cbLeft.Cb();
        }

        //  ensure we still have room for the data
        AssertPREFIX( ( pbTraceDest + cbTraceData ) <= ( m_rgbBuffer + _countof( m_rgbBuffer ) ) );

        //  trace the trace data

        memcpy( pbTraceDest, pbTraceData, cbTraceData );
        pbTraceDest += cbTraceData; // technically not necessary

        // convienent for debugging ...
        //wprintf( L"WTrace(%d = h:%d + cbftltid:%d + cbtick:%d + cbcb:%d + %d ) ib = %I64d: ", cbBufferNeeded, cbTraceHeader, cbTraceID, cbTickTrace, cbTraceDataFieldSize, cbTraceData, m_ibOffset );
        //PrintfDataLine( m_rgbBuffer + m_ibOffset, 1 + cbBufferNeeded / 16 );

        //  update the trace state

        m_ibOffset += cbBufferNeeded;
        Assert( pbTraceDest == m_rgbBuffer + m_ibOffset );  // world is sane.

        //  leave the trace buffer lock

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



// PERSISTED
enum FASTTRACELOGSTATE
{
    eFTLCreated         = 0,    //  the file has just been created
    eFTLDirty           = 1,    //  the file was being logged to last time it was attached
    eFTLClean           = 2,    //  the file had a clean / order detach, and did not lose any traces
    eFTLPostProcessed   = 3     //  the file can not be opened for update again after it is post processed
};

#include <pshpack1.h>


// PERSISTED
struct FTLFILEHDR
{
    //  Basic Header Info
    //
    LittleEndian<ULONG>     le_ulChecksumReserved;      //  in case we later want a header like checksum of the 4k page

    LittleEndian<ULONG>     le_ulFTLVersionMajor;       //  major version of fast trace file
    LittleEndian<ULONG>     le_ulFTLVersionMinor;       //  minor version of fast trace file
    LittleEndian<ULONG>     le_ulFTLVersionUpdate;      //  update version of fast trace file

    LittleEndian<FASTTRACELOGSTATE>     le_tracelogstate;// this tracks the state of the log file
    BYTE                    rgbReservedBasicHdrInfo[12];
//  32 bytes

    //  File Maintenance Information
    //
    LittleEndian<QWORD>     le_ftFirstOpen;             //  time of the first open
    LittleEndian<ULONG>     le_cReOpens;                //  number of times the file was [re]opened (for appending to) from a clean state
    LittleEndian<ULONG>     le_cRecoveries;             //  number of times the file was [re]opened (for appending to) from a dirty state
    LittleEndian<QWORD>     le_ftLastOpen;              //  time of the most recent open
    LittleEndian<QWORD>     le_ftLastClose;             //  time of the last / final close of the file (not including post processing close)
    LittleEndian<QWORD>     le_ftPostProcessed;         //  time of the post processing

    LittleEndian<QWORD>     le_cbLastKnownBuffer;       //  size of formatted and filled trace file (note size is cb, but always in 4 KB buffers multiples)

    LittleEndian<QWORD>     le_cWriteFailures;          //  number of times we tried to write a chunk of data and failed
    BYTE                    rgbReservedFileMaintInfo[40];
//  128 bytes

    //  Runtime Efficiency Statistics
    //
    LittleEndian<ULONG>     le_cMaxWriteIOs;            //  maximum number of write IOs that were outstanding at a given time
    LittleEndian<ULONG>     le_cMaxWriteBuffers;        //  maximum number of buffers that were used to buffer data for disk
    BYTE                    rgbReservedRuntimeStatsInfo[88];
//  224 bytes

    //  Specific Trace Schema Definition
    //
    LittleEndian<ULONG>     le_ulSchemaID;              //  An ID that tells us what schema (and CFTLDescriptor definition) to use
    LittleEndian<ULONG>     le_ulSchemaVersionMajor;    //  major version of the trace schema used within
    LittleEndian<ULONG>     le_ulSchemaVersionMinor;    //  minor version of the trace schema used within
    LittleEndian<ULONG>     le_ulSchemaVersionUpdate;   //  update version of the trace schema used within
    BYTE                    rgbReservedSchemaHdrInfo[16];
//  256 bytes

    BYTE                    rgbReservedFileTypeAlignment[667-256];  // buffer for the mis-alignment of le_filetype
                                                        // keeping the le_filetype in the same
                                                        // place as log file header and checkpoint
                                                        // and DB file header
//  667 bytes

    //  WARNING: MUST be placed at this offset for
    //  uniformity with db/log headers
    UnalignedLittleEndian<ULONG>    le_filetype;    //  JET_filetypeDatabase or JET_filetypeStreamingFile
//  671 bytes

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

    FTLTDESC        m_rgftltdescShortTraceDescriptors[128]; // technically handles the medium ones as well
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
    // ------------------------------------------------------------------------
    //
    //          FTL writing
    //

    //  Tracing Buffer
    //
    CFastTraceLogBuffer                 m_ftlb;
    BOOL                                m_fTerminating;

    //  File Management and Settings
    //

    //      File version info
    //          [2010/10/31] 1.0.0 - Initial version.
    //          [2013/10/18] 1.1.0 - Added fNewPage to BFCache_ and BFTouch_ and shrunk size
    //                               of those structures by 2 bytes by using a union.
    //          [2014/01/29] 1.2.0 - Added fNoTouch to BFTouch_.
    //                               Added fDBScan to BFTouch_ and BFCache_.
    //          [2014/08/22] 1.3.0 - Added BFDirty_ and BFWrite_.
    //          [2015/11/19] 2.0.0 - Removed fKeepHistory from BFEvict_.
    //          [2016/10/05] 2.1.0 - Added clientType to BFCache_ and BFTouch_.
    //          [2019/09/05] 2.2.0 - Added BFSetLgposModify_.
    //PERSISTED
    const static ULONG                  ulFTLVersionMajor = 2;
    const static ULONG                  ulFTLVersionMinor = 2;
    const static ULONG                  ulFTLVersionUpdate = 0;
    //      header format offsets
    const static ULONG                  ibTraceLogHeaderOffset              = 0;
    const static ULONG                  ibPrivateHeaderOffset               = 1024;
    const static ULONG                  ibTraceLogPostProcessedHeaderOffset = 2048;
    const static ULONG                  ibPrivatePostProcessedHeaderOffset  = 3072;
    //      settings
    INT                                 m_cbWriteBufferMax;
    IOREASON *                          m_piorTraceLog;     //  must be provided by client
    //      active file management
    IFileSystemConfiguration * const    m_pfsconfig;
    IFileSystemAPI *                    m_pfsapi;
    IFileAPI *                          m_pfapiTraceLog;
    BYTE *                              m_pbTraceLogHeader;

    ERR ErrFTLICheckVersions();
    ERR ErrFTLIFlushHeader();

    //  Specific Schema's FTL Descriptor info
    //
    const static FTLTDESC   ftltdescDefaultDescriptor = ftltdescNone;
    FTLDescriptor           m_ftldesc;          //  describes the schema of this specific trace file

    FTLTDESC FtltdescFTLIGetDescriptor( __in const USHORT usTraceID ) const;


    //  Writing / Flushing Buffers
    //

    //      state values (for m_rgfBufferState & m_ipbWriteBufferCurrent)
    const static INT        fBufferAvailable    = 0x0;
    const static INT        fBufferInUse        = 0x1;
    const static INT        fBufferFlushed      = 0x2;
    const static INT        fBufferDone         = 0x3;
    const static INT        ibufUninitialized   = -1;
    //      current write buffer state
    QWORD                   m_ibWriteBufferCurrent;
    volatile INT            m_ipbWriteBufferCurrent;
    volatile INT            m_cbWriteBufferFull;
    CSXWLatch               m_sxwlFlushing;
    //      write buffers
    CSemaphore              m_semBuffersInUse;
    INT                     m_rgfBufferState[80];
    BYTE *                  m_rgpbWriteBuffers[80]; // today: 384K * 80 = 30 MBs of backlog potentially

    void FTLIResetWriteBuffering( void );
    INT IFTLIGetFlushBuffer();
    static ERR ErrFTLFlushBuffer( __inout void * const pvFlushBufferContext, __in_bcount(cbBuffer) const BYTE * const rgbBuffer, __in const ULONG cbBuffer );
    ERR ErrFTLIFlushBuffer( __in_bcount(cbBuffer) const BYTE * rgbBuffer, __in const INT cbBuffer, const BOOL fForceTerm );
    static void FTLFlushBufferComplete( const ERR           err,
                                        IFileAPI* const     pfapi,
                                        const FullTraceContext& ptc,
                                        const QWORD/*OSFILEQOS*/ grbitQOS,
                                        const QWORD         ibOffset,
                                        const DWORD         cbData,
                                        const BYTE* const       pbData,
                                        const DWORD_PTR     keyIOComplete );

    //      stats
    volatile LONG           m_cOutstandingIO;
    volatile LONG           m_cOutstandingIOHighWater;


    // ------------------------------------------------------------------------
    //
    //      FTL Reading 
    //

public:
    class CFTLReader;

private:
    friend CFTLReader;

    CFTLReader *            m_pftlr;

    ERR ErrFTLIReadBuffer( __out_bcount(cbBuffer) void * pvBuffer, __in QWORD ibOffset, __in ULONG cbBuffer );


public:

    // ------------------------------------------------------------------------
    //
    //          FTL API
    //

    //  FTL Init / Term
    //

    CFastTraceLog(  const FTLDescriptor * const         pftldesc,
                    IFileSystemConfiguration * const    pfsconfig = NULL );

    enum FTLInitFlags   // ftlif
    {
        ftlifNone               = 0,

        //  write file disposition flags
        ftlifNewlyCreated       = 1,
        ftlifReOpenExisting     = 2,
        ftlifReCreateOverwrite  = 3,
        ftlifmskFileDisposition = ( ftlifNewlyCreated | ftlifReOpenExisting | ftlifReCreateOverwrite ),

        //  reader flags
        ftlifKeepStats          = 0x100,

    };

    //  Must init the FTL trace log with intention of writing or reading, but not both.
    ERR ErrFTLInitWriter( __in_z const WCHAR * wszTraceLogFile, IOREASON * pior, __in const FTLInitFlags ftlif );
    ERR ErrFTLInitReader( __in_z const WCHAR * wszTraceLogFile, IOREASON * pior, __in const FTLInitFlags ftlif, __out CFTLReader ** ppftlr );

    //  cleans up all FTL allocated resources (note including the ppftlr returned by ErrFTLInitReader()).
    void FTLTerm();

    //  FTL Tracing APIs
    //

    void SetFTLDisabled();
    ERR ErrFTLTrace( __in const USHORT usTraceID, __in_bcount(cbTrace) const BYTE * pbTrace, __in const ULONG cbTrace, __in const TICK tickTrace = TickOSTimeCurrent() );

    //  FTL Reading APIs
    //

    //      header
    FTLFILEHDR *    PftlhdrFTLTraceLogHeader()      { return (FTLFILEHDR *)( m_pbTraceLogHeader + ibTraceLogHeaderOffset ); }
    // reserved
    //void *        PvFTLPrivateHeader()            { return m_pbTraceLogHeader + ibPrivateHeaderOffset; }
    //void *        PvFTLTraceLogPostHeader()       { return m_pbTraceLogHeader + ibTraceLogPostProcessedHeaderOffset; }
    void *          PvFTLPrivatePostHeader()        { return m_pbTraceLogHeader + ibPrivatePostProcessedHeaderOffset; }
    ULONG           CbFullHeader()                  { return ibPrivatePostProcessedHeaderOffset + 1024; }
    ULONG           CbPrivateHeader()               { return 1024; }

    ERR ErrFTLDumpHeader();
    ERR ErrFTLSetPostProcessHeader( void * pv1kHeader );

    struct BFFTLFilePostProcessHeader
    {
        LittleEndian<ULONG>     le_cifmp;               //  number of valid IFMPs set in the le_mpifmpcpg array
        LittleEndian<ULONG>     le_mpifmpcpg[255];      //  for each ifmp a count of pages referenced in this file
    };

    //      retrieving traces
    struct FTLTrace
    {
        QWORD           ibBookmark;     //  Location in trace file (can be saved, restored)
        USHORT          ftltid;         //  Trace ID
        TICK            tick;           //  Tick of Trace
        const BYTE *    pbTraceData;    //  Trace Data
        ULONG           cbTraceData;    //  Trace Data Size
    };

    class CFTLReader    // ftlr
    {
        friend class CFastTraceLog;

    private:
        //  needs access to the m_pfapiXxx.
        CFastTraceLog *     m_pftl;

        //  Settings and Constants
        //

        ULONG               m_cbReadBufferSize;
        BOOL                m_fKeepStats;
        const static INT    ibufUninitialized = -1;

        //  Read Buffers
        //
        typedef struct ReadBuffers_
        {
            QWORD           ibBookmark;
            BYTE *          pbReadBuffer;
        } ReadBuffers;
        ReadBuffers         m_rgbufReadBuffers[10];
        INT                 m_ibufReadLast;
        QWORD               m_ibBookmarkNext;
        TICK                m_tickBase;

        //  Usage Efficiency Stats
        //

        QWORD               m_cReadIOs;

        //  Trace File Stats
        //
        QWORD               m_cFullBuffers;
        QWORD               m_cPartialBuffers;
        QWORD               m_cbPartialEmpty;
        QWORD               m_cBlankBuffers;
        QWORD               m_cbBlankEmpty;

#ifdef DEBUG
        //  Debugging
        //
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

    //  testing only
    TICK TickFTLTickBaseCurrent() const;

};

//  The "shared interface" for the 4 sub-reasons is this simple list of predefined
//  "integer" types that must be defined by the client library linking to the OS
//  File IO APIs.
//

enum IOREASONPRIMARY : BYTE;
enum IOREASONSECONDARY : BYTE;
enum IOREASONTERTIARY : BYTE;
enum IOREASONUSER : BYTE;
enum IOREASONFLAGS : BYTE;

//  Null IO reasons defined for the client...
//      Note: In some contexts the clients may use 0x0 to not mean none, but mean
//      unspecified or generic.
#define iorpNone        ((IOREASONPRIMARY)0x0)
#define iorsNone        ((IOREASONSECONDARY)0x0)
#define iortNone        ((IOREASONTERTIARY)0x0)
#define ioruNone        ((IOREASONUSER)0x0)
#define iorfNone        ((IOREASONFLAGS)0x0)

//  Helper class that combines the 4 sub-reasons into a solitary chunk of memory
//  for easy shipping into and out of the IO APIs.
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
        // reserving the top bit, why? why not?
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
        //  Set the various IO reason sub-elements ...
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
        //  Set the various IO reason sub-elements ...
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

    // Exchange specific context
    GUID    guidActivityId;

    char    szClientComponent[ cchStrMax ];
    char    szClientAction[ cchStrMax ];
    char    szClientActionContext[ cchStrMax ];

    // Copies a description string, truncating it if it doesn't fit
    static ERR ErrCopyDescString( char* const szDest, const char* const szSrc, const INT cbSrc );
};

#define OC_bitInternalUser  0x80000000      // grbit used with OPERATION_CONTEXT::dwUserID to identify internal users of the system
#define OCUSER_SYSTEM       ( OC_bitInternalUser | 0x7fffffff )     // Default context for system operations that can't be classified into any othe context

//  shortcut
#define IOR IOREASON

const BYTE  pocNone = 0xff;
inline bool FParentObjectClassSet( BYTE nParentObjectClass )
{
    return nParentObjectClass != pocNone;
}

//  Holds tracing context information supplied by the user
//  Used to track the context and emit it in IO traces
class UserTraceContext
{
public:
    static const USER_CONTEXT_DESC s_ucdNil;    // Constant object used to initialize pUserContextDesc

    USER_CONTEXT_DESC*  pUserContextDesc;   //  Exchange specific user context
    OPERATION_CONTEXT   context;            //  True user context (note: provided from above Engine [i.e. above ESE]).
    DWORD               dwCorrelationID;    //  True user correlation context.
    DWORD               dwIOSessTraceFlags; //  Flags for enabling session specific IO traces

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

    // Copy constructor, but the user has to explicitly select whether he wants to copy the user context desc
    // Because it requires a heap allocation
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
        // Call to UserTraceContext::.move_ctor not allowed in retail. UserTraceContext creation should always be inlined.
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
            // We don't want to keep freeing and reallocating memory if we are switching between null and real contexts
            // So we just reset the allocated structure to zeroes.

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
    // Disallowed
    UserTraceContext( const UserTraceContext& rhs );
    const UserTraceContext& operator=( const UserTraceContext& rhs );
};


const ULONG dwEngineObjidNone = 0xFFFFFFFF;
 
//  Holds mutable tracing information related to an IO
//  TraceContext is kept on the callstack (and in the TLS).
//  This information is supposed to change frequently between various components and layers.
//  Use TraceContextScope below to break down the shared state into separate scopes with distinct state
class TraceContext
{
public:
    IOREASON                    iorReason;          // Reasons identifying the nature of this IO for the Engine.
    ULONG                       dwEngineObjid;      // objidFDP the current operation targets.
    BYTE                        nParentObjectClass; // The type of logical object that this IO belongs to (e.g. tce).

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

    // FEmpty() is NOT an inverse of FValid()
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

//  Stores a copy of all the tracing context information related to an IO
//  Required to make deep copies of the context
class FullTraceContext
{
public:
    UserTraceContext utc;
    TraceContext etc;

    FullTraceContext()
    {
    }

    // Copy constructor, but the user has to explicitly select whether he wants to copy the user context desc
    // Because it requires a heap allocation
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
    // Disallowed
    FullTraceContext( FullTraceContext& rhs );
    const FullTraceContext& operator= ( const FullTraceContext& rhs );
};

//  Used to manage a logical scope for a tracing context.
//  The current scope is stored in some thread-local context, accessible through the TFnGetEtc functor.
//  The functor allows the source of the TraceContext to be configurable, e.g. it is used to reduce calls to TlsGetValue()
//  by sourcing the tc from higher level scopes (like PIB).
//  A functor is used to allow inlining the call to get the tc. A simple pfn wouldn't inline.
//
//  Restores the context to the previous state when the scope ends.
//  The scope is defined by the caller. The caller should ensure that tracing info isn't mixed unintentionally between
//  unrelated operations (unrelated at caller's layer). Create a new scope for a new unrelated operations.
//  When in doubt, create a new scope.
// For example:

//      Layer A     Layer B     Layer C                 TraceContext
//      op1                                             create scope1
//                  op1.a                               create scope1.a (inherits scope1 automatically)
//                              op1.a.1                 create scope1.a.1 (can choose to reuse scope1.a)
//                  op1.b                               create scope1.b (inherits scope1)
//                              op1.b.1                 reuse scope1.b (can reuse scope1.b.1)
//      op2                                             create scope2
//                  op2.a                               reuse scope2
//                              op2.a.1                 reuse scope2
template< typename TFnGetEtc >
class _TraceContextScope
{
    TraceContext*       m_ptcCurr;
    TraceContext        m_tcSaved;

public:
    _TraceContextScope( const TFnGetEtc& tfnGetEtc )
    {
        m_ptcCurr = tfnGetEtc();
        m_tcSaved = *m_ptcCurr; // save a copy to restore back
    }

    _TraceContextScope( TFnGetEtc tfnGetEtc, IOREASONPRIMARY iorp, IOREASONSECONDARY iors = iorsNone, IOREASONTERTIARY iort = iortNone, IOREASONFLAGS iorf = iorfNone )
    {
        m_ptcCurr = tfnGetEtc();
        m_tcSaved = *m_ptcCurr; // save a copy to restore back

        iorp != iorpNone ? m_ptcCurr->iorReason.SetIorp( iorp ) : 0;
        iors != iorsNone ? m_ptcCurr->iorReason.SetIors( iors ) : 0;
        iort != iortNone ? m_ptcCurr->iorReason.SetIort( iort ) : 0;
        m_ptcCurr->iorReason.AddFlag( iorf );
    }

    _TraceContextScope( TFnGetEtc tfnGetEtc, IOREASONSECONDARY iors, IOREASONTERTIARY iort = iortNone, IOREASONFLAGS iorf = iorfNone )
    {
        m_ptcCurr = tfnGetEtc();
        m_tcSaved = *m_ptcCurr; // save a copy to restore back

        iors != iorsNone ? m_ptcCurr->iorReason.SetIors( iors ) : 0;
        iort != iortNone ? m_ptcCurr->iorReason.SetIort( iort ) : 0;
        m_ptcCurr->iorReason.AddFlag( iorf );
    }

    _TraceContextScope( TFnGetEtc tfnGetEtc, IOREASONTERTIARY iort, IOREASONFLAGS iorf = iorfNone )
    {
        m_ptcCurr = tfnGetEtc();
        m_tcSaved = *m_ptcCurr; // save a copy to restore back

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

    // Special handling for copies, move semantics
    //  - Copying isn't allowed. Always create a new scope.
    //  - That means return by value must use RVO/NRVO in retail. We expect the compiler to always be able to do this. We Enforce() if it doesn't.
    //  - In debug, more general return by value is explicitly allowed through move semantics.
    //  - This is how the move .ctor works:
    //      .ctor <temp>: set TLS
    //      .move_ctor <actual>: setup temp to 'fix' TLS upon .dtor <temp>
    //      .dtor <temp>: clear TLS (fixes up TLS to point to actual)
    //      scope is properly constructed
    //      .dtor <actual>: clear TLS   (when the user is done with the scope)

    _TraceContextScope( _TraceContextScope&& rvalue )
    {
#ifdef DEBUG
        m_ptcCurr = rvalue.m_ptcCurr;
        m_tcSaved = rvalue.m_tcSaved;

        // Fix up rvalue
        rvalue.m_tcSaved = *m_ptcCurr;
#else
        // Call to _TraceContextScope::.move_ctor not allowed in retail. _TraceContextScope creation should always allow RVO (Return Value Optimization).
        EnforceSz( false, "DisallowedTraceContextScopeCtor" );
#endif
    }

    // Disallowed
    _TraceContextScope( const _TraceContextScope& tc ) = delete;
    const _TraceContextScope& operator=( const _TraceContextScope& tc ) = delete;
};

// A helper class to get the current user trace context from the TLS
// Returns an empty system context, if the TLS has a null user context
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
const TraceContext* PetcTLSGetEngineContextCached( TLS *ptlsCached );    // use this variant if you have a cached TLS pointer

// A functor that gets the engine TraceContext from the TLS to use with _TraceContextScope
class TLSGetEtcFunctor
{
public:
    TraceContext* operator()() const
    {
        return const_cast<TraceContext*>( PetcTLSGetEngineContext() );
    }
};

// TraceContextScope that works with the current TraceContext on the TLS
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

#endif  //  MINIMAL_FUNCTIONALITY

#endif  //  _OS_TRACE_HXX_INCLUDED


