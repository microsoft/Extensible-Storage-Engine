// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

#include <tchar.h>
#include "os.hxx"


#define OSLayerFTLConfig()                              \

#define OSLayerStart()                  \
    COSLayerPreInit     oslayer;        \
    if ( !oslayer.FInitd() )            \
        {                               \
        wprintf( L"Out of memory error during OS Layer pre-init." );    \
        err = JET_errOutOfMemory;       \
        goto HandleError;               \
    }                               \
    OSTestCall( ErrOSInit() );


CFastTraceLogBuffer g_ftlb;


ERR __stdcall ErrNullFlushBuffer( __inout void * const pvFlushBufferContext, __in const BYTE * const rgbBuffer, __in const ULONG cbBuffer )
{
    return JET_errSuccess;
}

BOOL g_fFlushed = fFalse;

ERR __stdcall ErrSignalFlushBuffer( __inout void * const pvFlushBufferContext, __in const BYTE * const rgbBuffer, __in const ULONG cbBuffer )
{
    g_fFlushed = fTrue;
    return JET_errSuccess;
}




CUnitTest( FTLBufferWrapFlushTestBasic, 0, "Tests for writing to / through the end of the buffer, and that the buffer is flushed." );
ERR FTLBufferWrapFlushTestBasic::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    OSLayerFTLConfig();
    OSLayerStart();

    QWORD qwTrace;

    OSTestCall( g_ftlb.ErrFTLBInit( ErrSignalFlushBuffer, NULL ) );

    g_fFlushed = fFalse;

    qwTrace = 0x33432;
    
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, TickOSTimeCurrent() ) );


HandleError:
    if ( err )
    {
        wprintf( L"\tDone(error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    g_ftlb.FTLBTerm();

    OSTerm();

    return err;
}


CUnitTest( FTLBufferWrapFlushTestComprehensive, 0, "tests for writing to / through the end of the buffer, and that the buffer is flushed in several contexts and trace sizes." );
ERR FTLBufferWrapFlushTestComprehensive::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    OSLayerFTLConfig();
    OSLayerStart();

    ULONG i;
    QWORD qwTrace;

    const TICK tickTrace    = TickOSTimeCurrent();

    OSTestCall( g_ftlb.ErrFTLBInit( ErrSignalFlushBuffer, NULL, tickTrace ) );

    qwTrace = 0x33432;


    g_fFlushed = fFalse;

    for ( i = 0; i < ( g_ftlb.CbFTLBBuffer() - g_ftlb.CbFTLBHeader() ) / 9; i++ )
    {
        OSTestCall( g_ftlb.ErrFTLBTrace( 1, ftltdescNone, (BYTE*)&qwTrace, 7, tickTrace ) );
        Assert( g_fFlushed == fFalse );
    }

    OSTestCheck( g_fFlushed == fFalse );
    OSTestCall( g_ftlb.ErrFTLBTrace( 1, ftltdescNone, (BYTE*)&qwTrace, 7, tickTrace ) );
    OSTestCheck( g_fFlushed == fTrue );


    g_fFlushed = fFalse;

    OSTestCheck( g_fFlushed == fFalse );
    g_ftlb.FTLBTerm();
    OSTestCheck( g_fFlushed == fTrue );


    g_fFlushed = fFalse;

    OSTestCall( g_ftlb.ErrFTLBInit( ErrSignalFlushBuffer, NULL, tickTrace ) );

    OSTestCheck( g_fFlushed == fFalse );

    for ( i = 0; i < ( g_ftlb.CbFTLBBuffer() - g_ftlb.CbFTLBHeader() ) / 8; i++ )
    {
        OSTestCall( g_ftlb.ErrFTLBTrace( 1, ftltdescNone, (BYTE*)&qwTrace, 6, tickTrace ) );
    }

    OSTestCheck( g_fFlushed == fFalse );
    OSTestCall( g_ftlb.ErrFTLBTrace( 1, ftltdescNone, (BYTE*)&qwTrace, 7, tickTrace ) );
    OSTestCheck( g_fFlushed == fTrue );
    

    g_ftlb.FTLBTerm();
    OSTestCall( g_ftlb.ErrFTLBInit( ErrSignalFlushBuffer, NULL, tickTrace ) );
    g_fFlushed = fFalse;

    for ( i = 0; i < ( g_ftlb.CbFTLBBuffer() - g_ftlb.CbFTLBHeader() ) / 10; i++ )
    {
        OSTestCall( g_ftlb.ErrFTLBTrace( 1, ftltdescNone, (BYTE*)&qwTrace, 7, tickTrace + 2 ) );
    }

    OSTestCheck( g_fFlushed == fFalse );
    OSTestCall( g_ftlb.ErrFTLBTrace( 1, ftltdescNone, (BYTE*)&qwTrace, 7, tickTrace + 2 ) );
    OSTestCheck( g_fFlushed == fTrue );

HandleError:
    if ( err )
    {
        wprintf( L"\tDone(error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    g_ftlb.FTLBTerm();

    OSTerm();

    return err; 
}


CUnitTest( FTLBufferValidationTestBasic, 0, "Tests for writing to the buffer, and validate the traces were saved." );
ERR FTLBufferValidationTestBasic::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    OSLayerFTLConfig();
    OSLayerStart();

    QWORD qwTrace;

    OSTestCall( g_ftlb.ErrFTLBInit( ErrSignalFlushBuffer, NULL ) );

    g_fFlushed = fFalse;

    qwTrace = 0x33432;
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, TickOSTimeCurrent() ) );

HandleError:
    if ( err )
    {
        wprintf( L"\tDone(error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    g_ftlb.FTLBTerm();

    OSTerm();

    return err;
}

CUnitTest( FTLBufferTraceTestComprehensive, 0, "Tests most writing various states to the buffer, no read validation is done." );
ERR FTLBufferTraceTestComprehensive::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    OSLayerFTLConfig();
    OSLayerStart();

    QWORD qwTrace;

    OSTestCall( g_ftlb.ErrFTLBInit( ErrNullFlushBuffer, NULL ) );

    OSTestCheck( g_ftlb.CbFTLBIUsed() == g_ftlb.CbFTLBHeader() );

    qwTrace = 0x33432;

    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, TickOSTimeCurrent() ) );
    OSTestCheck( g_ftlb.CbFTLBIUsed() > 7 );

    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, TickOSTimeCurrent() ) );
    OSTestCheck( g_ftlb.CbFTLBIUsed() > 14 );

    TICK tick = TickOSTimeCurrent();
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick ) );
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick ) );
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick + 1 ) );
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick + 15 ) );
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick + 16 ) );
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick + 64 ) );
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick + 127 ) );
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick + 128 ) );
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick + 250 ) );
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick + 255 ) );
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick + 256 ) );
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick + 1023 ) );
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick + 0x60000000 ) );
    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, (BYTE*)&qwTrace, 7, tick + 0xFFFFFFFF ) );

    err = JET_errSuccess;

HandleError:
    if ( err )
    {
        wprintf( L"\tDone(error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    g_ftlb.FTLBTerm();

    OSTerm();

    return err;
}

CUnitTest( FTLBufferTraceTestBasic, 0, "Tests most basic writing to the buffer, no read validation is done." );
ERR FTLBufferTraceTestBasic::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    OSLayerFTLConfig();
    OSLayerStart();

    const TICK tickTraceStart = TickOSTimeCurrent();
    BYTE rgbTrace[8];
    rgbTrace[0] = 0x10;

    const FTLTDESC  ftltdescFixed1 = (FTLTDESC)0x1 & mskFtltdescFixedSize;
    const FTLTDESC  ftltdescFixed6 = (FTLTDESC)0x6 & mskFtltdescFixedSize;

    OSTestCall( g_ftlb.ErrFTLBInit( ErrNullFlushBuffer, NULL, tickTraceStart ) );

    OSTestCheck( g_ftlb.CbFTLBIUsed() == g_ftlb.CbFTLBHeader() );


    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescFixed6, rgbTrace, 6, tickTraceStart ) );
    OSTestCheck( g_ftlb.CbFTLBIUsed() == g_ftlb.CbFTLBHeader() + 7  );


    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescFixed6, rgbTrace, 6, tickTraceStart ) );
    OSTestCheck( g_ftlb.CbFTLBIUsed() == g_ftlb.CbFTLBHeader() + 7 + 7 );


    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescFixed6, rgbTrace, 6, tickTraceStart + 1 ) );
    OSTestCheck( g_ftlb.CbFTLBIUsed() == g_ftlb.CbFTLBHeader() + 7 + 7 + 8 );


    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescFixed6, rgbTrace, 6, tickTraceStart + 256 ) );
    OSTestCheck( g_ftlb.CbFTLBIUsed() == g_ftlb.CbFTLBHeader() + 7 + 7 + 8 + 11 );


    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescFixed1, rgbTrace, 1, tickTraceStart + 1 ) );
    OSTestCheck( g_ftlb.CbFTLBIUsed() == g_ftlb.CbFTLBHeader() + 7 + 7 + 8 + 11 + 3 );

    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, rgbTrace, 1, tickTraceStart + 1 ) );
    OSTestCheck( g_ftlb.CbFTLBIUsed() == g_ftlb.CbFTLBHeader() + 7 + 7 + 8 + 11 + 3 + 4 );

    OSTestCall( g_ftlb.ErrFTLBTrace( 0x1, ftltdescNone, rgbTrace, 0, tickTraceStart ) );
    OSTestCheck( g_ftlb.CbFTLBIUsed() == g_ftlb.CbFTLBHeader() + 7 + 7 + 8 + 11 + 3 + 4 + 1 );

    err = JET_errSuccess;

HandleError:
    if ( err )
    {
        wprintf( L"\tDone(error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    g_ftlb.FTLBTerm();

    OSTerm();

    return err;
}



enum IOREASONPRIMARY : BYTE { e = 1 };

class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
{
    public:
        CFileSystemConfiguration()
        {
            m_dtickAccessDeniedRetryPeriod = 85;
            m_cbMaxReadSize = 384 * 1024;
            m_cbMaxWriteSize = 384 * 1024;
        }
} g_fsconfig;

CFastTraceLog   g_ftl( NULL, &g_fsconfig );

CUnitTest( FTLTraceValidationTestBasic, 0, "Tests basic writing to the trace log file, and that we can read the same data." );
ERR FTLTraceValidationTestBasic::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    QWORD qwTrace1 = 0xf3234324df;
    BYTE rgbTrace2[5] = { 0x12, 0xf3, 0x34, 0xab, 0x32 };
    BYTE rgbTrace3[7] = { 0x02, 0x34, 0x9a, 0xff, 0x11, 0xb3, 0x10 };
    BYTE rgbTrace4[6] = { 0x80, 0x01, 0x3f, 0x23, 0x23, 0x33 };

    IOREASON ior( e );

    OSLayerFTLConfig();
    OSLayerStart();

    OSTestCall( g_ftl.ErrFTLInitWriter( L".\\FtlUnitTrace.ftl", &ior, CFastTraceLog::ftlifReCreateOverwrite ) );


    TICK tickTrace = g_ftl.TickFTLTickBaseCurrent();

    OSTestCall( g_ftl.ErrFTLTrace( 0x1, (BYTE*)&qwTrace1, sizeof(qwTrace1), tickTrace ) );
    OSTestCall( g_ftl.ErrFTLTrace( 0x4, rgbTrace2, sizeof(rgbTrace2), tickTrace ) );
    OSTestCall( g_ftl.ErrFTLTrace( 0x2, rgbTrace3, sizeof(rgbTrace3), tickTrace+1 ) );
    OSTestCall( g_ftl.ErrFTLTrace( 0x41, rgbTrace4, sizeof(rgbTrace4), tickTrace+1 ) );

    OSTestCall( g_ftl.ErrFTLTrace( 0x41, rgbTrace4, sizeof(rgbTrace4)*2, tickTrace+1 ) );

    g_ftl.FTLTerm();

    CFastTraceLog::CFTLReader * pftlr = NULL;

    OSTestCall( g_ftl.ErrFTLInitReader( L".\\FtlUnitTrace.ftl", &ior, CFastTraceLog::ftlifNone, &pftlr ) );

    CFastTraceLog::FTLTrace     ftltrace;

    OSTestCall( pftlr->ErrFTLGetNextTrace( &ftltrace ) );

    OSTestCheck( ftltrace.ftltid == 0x1 );
    OSTestCheck( ftltrace.tick == tickTrace );
    OSTestCheck( ftltrace.cbTraceData == sizeof(qwTrace1) );
    OSTestCheck( 0 == memcmp( ftltrace.pbTraceData, &qwTrace1, ftltrace.cbTraceData ) );
    
    OSTestCall( pftlr->ErrFTLGetNextTrace( &ftltrace ) );
    OSTestCheck( ftltrace.ftltid == 0x4 );
    OSTestCheck( ftltrace.tick == tickTrace );
    OSTestCheck( ftltrace.cbTraceData == sizeof(rgbTrace2) );
    OSTestCheck( 0 == memcmp( ftltrace.pbTraceData, rgbTrace2, ftltrace.cbTraceData ) );

HandleError:

    if ( err )
    {
        wprintf( L"\tDone(error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    g_ftl.FTLTerm();

    OSTerm();

    return err;
}


CUnitTest( FTLDumpHeader, 0, "Horrible abuse of unit test framework here: Dumps the FTL header of the file FtlUnitTrace.ftl." );
ERR FTLDumpHeader::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    IOREASON ior( e );

    OSLayerFTLConfig();
    OSLayerStart();

    OSTestCall( g_ftl.ErrFTLInitReader( L".\\FtlUnitTrace.ftl", &ior, CFastTraceLog::ftlifNone, NULL ) );

    g_ftl.ErrFTLDumpHeader();

HandleError:

    if ( err )
    {
        wprintf( L"\tDone(error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    g_ftl.FTLTerm();

    OSTerm();

    return err;
}



CUnitTest( FTLTraceValidationTestLoop, 0, "Tests a variety of test arguments from a loop, and then tests the exact same traces are readable after." );
ERR FTLTraceValidationTestLoop::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    BYTE rgbTrace[9] = { 0xff, 0x82, 0x02, 0x34, 0x9a, 0xff, 0x11, 0xb3, 0x10 };

    const ULONG     cIter = 400000;
    ULONG           i = (ULONG)-1;
    BOOL            fReading = fFalse;

    IOREASON ior( e );

    OSLayerFTLConfig();
    OSLayerStart();

    IFileSystemAPI * pfsapi;
    OSTestCall( ErrOSFSCreate( NULL, &pfsapi ) );
    ERR errT = pfsapi->ErrFileDelete( L".\\FtlLoopTrace.ftl" );
    delete pfsapi;
    OSTestCall( errT );

    OSTestCall( g_ftl.ErrFTLInitWriter( L".\\FtlLoopTrace.ftl", &ior, CFastTraceLog::ftlifReOpenExisting ) );

    TICK tickTrace = g_ftl.TickFTLTickBaseCurrent();

    for( i = 0; i < cIter; i++ )
    {
        FTLTID t = 1 + i * i % ftltidMax;
        t = ( t == 15 ? 3 : t );
        OSTestCall( g_ftl.ErrFTLTrace( t, rgbTrace, sizeof(rgbTrace)-(i % sizeof(rgbTrace)), tickTrace + i / 34 ) );
    }

    g_ftl.FTLTerm();

    CFastTraceLog::CFTLReader * pftlr;

    OSTestCall( g_ftl.ErrFTLInitReader( L".\\FtlLoopTrace.ftl", &ior, CFastTraceLog::ftlifNone, &pftlr ) );

    CFastTraceLog::FTLTrace     ftltrace;
    fReading = fTrue;

    for ( i = 0; i < cIter; i++ )
    {
        FTLTID t = 1 + i * i % ftltidMax;
        t = ( t == 15 ? 3 : t );
        OSTestCall( pftlr->ErrFTLGetNextTrace( &ftltrace ) );
        OSTestCall( err );
        OSTestCheck( ftltrace.ftltid == t );
        OSTestCheck( ftltrace.tick == tickTrace + i / 34 );
        OSTestCheck( ftltrace.cbTraceData == ( sizeof(rgbTrace)-(i % sizeof(rgbTrace)) ) );
        OSTestCheck( 0 == memcmp( ftltrace.pbTraceData, rgbTrace, ftltrace.cbTraceData ) );
    }

HandleError:

    wprintf(L"\tPerformed iter = %d of %d (in %ws)\n", i, cIter, fReading ? L"reading" : L"writing" );

    if ( err )
    {
        wprintf( L"\tDone(error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    g_ftl.FTLTerm();

    OSTerm();

    return err;
}


CUnitTest( FTLTraceValidationTestHeader, 0, "Tests basic header properties of the trace log file, and that we can read the same header out." );
ERR FTLTraceValidationTestHeader::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    typedef struct
    {
        BYTE                            bTrace;
        USHORT                          ifmp;
        UnalignedLittleEndian<ULONG>    le_pgno;
    } TEST_TRACE_1;

    IOREASON ior( e );

    OSLayerFTLConfig();
    OSLayerStart();

    OSTestCall( g_ftl.ErrFTLInitWriter( L".\\FtlUnitTrace.ftl", &ior, CFastTraceLog::ftlifReOpenExisting ) );

HandleError:
    if ( err )
    {
        wprintf( L"\tDone(error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    g_ftl.FTLTerm();

    OSTerm();

    return err;
}

#ifdef DISABLE_FOR_A_MOMENT

CUnitTest( FTLTraceValidationTestFailedInit, 0, "Tests that if we get a bad buffer size, FTL will fail to init, AND most importantly that we can term without hanging." );
ERR FTLTraceValidationTestFailedInit::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    IOREASON ior( e );

    OSLayerStart();

    err = g_ftl.ErrFTLInitWriter( L".\\FtlUnitTrace.ftl", &ior, CFastTraceLog::ftlifReOpenExisting );
    OSTestCheck( err == JET_errInvalidBufferSize );
    err = JET_errSuccess;


HandleError:

    g_ftl.FTLTerm();

    OSTerm();

    if ( err )
    {
        wprintf( L"\tDone(error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    return err;
}

#endif

CFastTraceLog   g_ftlDenied( NULL );

CUnitTest( FTLTraceValidationTestFailedInitDisabledTrace, 0, "Tests that if we fail to open the file, we can then disable the FTL tracer and that trace calls silently succeed." );
ERR FTLTraceValidationTestFailedInitDisabledTrace::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    IOREASON ior( e );
    BYTE rgbTrace[9] = { 0xff, 0x82, 0x02, 0x34, 0x9a, 0xff, 0x11, 0xb3, 0x10 };

    OSLayerFTLConfig();
    OSLayerStart();

    OSTestCall( g_ftl.ErrFTLInitWriter( L".\\FtlUnitTrace.ftl", &ior, CFastTraceLog::ftlifReOpenExisting ) );

    err = g_ftlDenied.ErrFTLInitWriter( L".\\FtlUnitTrace.ftl", &ior, CFastTraceLog::ftlifReOpenExisting );
    OSTestCheck( err == JET_errFileAccessDenied );

    g_ftlDenied.SetFTLDisabled();

    OSTestCall( g_ftlDenied.ErrFTLTrace( 2, rgbTrace, sizeof(rgbTrace), g_ftl.TickFTLTickBaseCurrent() ) );
    OSTestCall( g_ftlDenied.ErrFTLTrace( 2, rgbTrace, sizeof(rgbTrace), g_ftl.TickFTLTickBaseCurrent() ) );

HandleError:

    g_ftl.FTLTerm();

    g_ftlDenied.FTLTerm();

    OSTerm();

    if ( err )
    {
        wprintf( L"\tDone(error or unexpected error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    return err;
}


