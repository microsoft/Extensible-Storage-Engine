// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

enum IOREASONPRIMARY : BYTE
{
    iorpInvalid = 0,

    iorpOSUnitTest
};

#define MemoryMappedIoTestInit      \
    COSLayerPreInit oslayer;                                    \
    JET_ERR err = JET_errSuccess;                               \
    const DWORD cbPage = OSMemoryPageCommitGranularity();       \
    \
    IFileSystemAPI * pfsapi = NULL;                             \
    IFileAPI * pfapi = NULL;                                    \
    \
    WCHAR * wszFileDat = L".\\MappableFile.dat";                \
    OSTestCall( ErrCreateIncrementFile( wszFileDat, fFalse ) );     \
    \
    wprintf( L"\tReopen file in mapped mode\n" );               \
    \
    OSTestCall( ErrOSInit() );                                      \
    OSTestCall( ErrOSFSCreate( NULL, &pfsapi ) );     \
    \
    wprintf( L"\t\tPre-open purging file first.\n" );           \
    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapi ) );  \
    delete pfapi;                                               \
    pfapi = NULL;                                               \
    \
    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );    \


#define MemoryMappedIoTestTerm      \
    delete pfapi;                   \
    delete pfsapi;                  \
    OSTerm();                       \


extern WCHAR g_rgwchSysDriveTesting[IFileSystemAPI::cchPathMax];


CUnitTest( OSLayerErrMMIOReadFunctions, 0, "Tests that OS Layer can detect non resident cache." );
ERR OSLayerErrMMIOReadFunctions::ErrTest()
{
    COSLayerPreInit oslayer;
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pvMap1 = NULL;
    BYTE * pvMap2 = NULL;

    const BOOL fMmCopyInstead = fFalse;
    const BOOL fBigFileRandPage = fFalse;

    wprintf( L"\tTest Options: %d \n", fMmCopyInstead );
    srand( TickOSTimeCurrent() );
    
    WCHAR * wszFileDat = fBigFileRandPage ? L".\\MappableBigFile.dat" : L".\\MappableFile.dat";
    Call( ErrCreateIncrementFile( wszFileDat, fBigFileRandPage ) );

    wprintf( L"\tReopen file in mapped mode\n" );

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL, &pfsapi ) );

    if ( rand() % 2 == 0 )
    {
RetryWithPathInjection:
        WCHAR wszFullPath[IFileSystemAPI::cchPathMax];
        Call( pfsapi->ErrPathComplete( wszFileDat, wszFullPath ) );
        Call( pfsapi->ErrPathParse( wszFullPath, g_rgwchSysDriveTesting, NULL, NULL ) );
    }
    else
    {
        JET_ERR errT = ErrEnableTestInjection( 46860, 3, JET_TestInjectConfigOverride, 100, JET_bitInjectionProbabilityPct );
        if ( errT == JET_errTestInjectionNotSupported )
        {
            goto RetryWithPathInjection;
        }
    }

    wprintf( L"\t\tPre-open purging file first.\n" );
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapi ) );
    delete pfapi;
    pfapi = NULL;

    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

{
    QWORD ipg = 37;
    if ( fBigFileRandPage )
    {
        ipg = ( ( rand() << 16 ) | rand() ) % ( CbIncrementalFileSize( fTrue ) / cbPage );
        wprintf( L"\tSelecting random page %I64d\n", ipg );
    }
    const HRT dhrtMapStart = HrtHRTCount();
    OSTestCall( pfapi->ErrMMRead( ipg * cbPage, cbPage, (void**)&pvMap1 ) );
    const double dbltimeMap = DblHRTElapsedTimeFromHrtStart( dhrtMapStart );
    OSTestCheck( NULL != pvMap1 );

    OSTestCheck( !FOSMemoryPageResident( pvMap1, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pvMap1, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pvMap1, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pvMap1, cbPage ) );

    const HRT hrtIoStart = HrtHRTCount();
    OSTestCall( pfapi->ErrMMIORead( ipg * cbPage, pvMap1, cbPage, IFileAPI::fmmiorfKeepCleanMapped ) );
    const double dbltimeIo = DblHRTElapsedTimeFromHrtStart( hrtIoStart );

    OSTestCheck( FOSMemoryPageResident( pvMap1, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pvMap1, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pvMap1, cbPage ) );

    OSTestCheck( !FOSMemoryFileMappedCowed( pvMap1, cbPage ) );

    OSTestCheck( pvMap1[3] == ipg  || fBigFileRandPage );
    OSTestCheck( pvMap1[30] == ipg || fBigFileRandPage );

    wprintf( L"\tMap and IO Latencies (us): %f / %f.\n", dbltimeMap * 1000000.0, dbltimeIo * 1000000.0 );
}


{
    QWORD ipg = 73;
    if ( fBigFileRandPage )
    {
        ipg = ( ( rand() << 16 ) | rand() ) % ( CbIncrementalFileSize( fTrue ) / cbPage );
        wprintf( L"\tSelecting random page %I64d\n", ipg );
    }
    const HRT dhrtMapStart = HrtHRTCount();
    OSTestCall( pfapi->ErrMMCopy( ipg * cbPage, cbPage, (void**)&pvMap2 ) );
    const double dbltimeMap = DblHRTElapsedTimeFromHrtStart( dhrtMapStart );
    OSTestCheck( NULL != pvMap2 );

    OSTestCheck( !FOSMemoryPageResident( pvMap2, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pvMap2, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pvMap2, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pvMap2, cbPage ) );

    const HRT hrtIoStart = HrtHRTCount();
    OSTestCall( pfapi->ErrMMIORead( ipg * cbPage, pvMap2, cbPage, IFileAPI::fmmiorfCopyWritePage ) );
    const double dbltimeIo = DblHRTElapsedTimeFromHrtStart( hrtIoStart );

    OSTestCheck( FOSMemoryPageResident( pvMap2, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pvMap2, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pvMap2, cbPage ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pvMap2, cbPage ) );

    OSTestCheck( pvMap2[3] == ipg  || fBigFileRandPage );
    OSTestCheck( pvMap2[30] == ipg || fBigFileRandPage );

    wprintf( L"\tMap and IO Latencies (us): %f / %f.\n", dbltimeMap * 1000000.0, dbltimeIo * 1000000.0 );
}

HandleError:

    g_rgwchSysDriveTesting[0] = L'\0';
    ErrEnableTestInjection( 46860, 0, JET_TestInjectConfigOverride, 0, JET_bitInjectionProbabilityPct );

    if ( pfapi )
    {
        pfapi->ErrMMFree( pvMap1 );
        pfapi->ErrMMFree( pvMap2 );
    }
    delete pfapi;
    delete pfsapi;
    OSTerm();

    return err;
}

CUnitTest( OSLayerErrMMIOReadFunctionCowsOnUnsafeDisks, 0, "Tests that OS Layer will auto-COW memory on non-safe disks." );
ERR OSLayerErrMMIOReadFunctionCowsOnUnsafeDisks::ErrTest()
{
    COSLayerPreInit oslayer;
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pvMap1 = NULL;

    srand( TickOSTimeCurrent() );
    
    WCHAR * wszFileDat = L".\\MappableFile.dat";
    Call( ErrCreateIncrementFile( wszFileDat, fFalse ) );

    wprintf( L"\tReopen file in mapped mode\n" );

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL, &pfsapi ) );

    wprintf( L"\t\tPre-open purging file first.\n" );
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapi ) );
    delete pfapi;
    pfapi = NULL;

    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

    JET_ERR errT = ErrEnableTestInjection( 46860, 0, JET_TestInjectConfigOverride, 100, JET_bitInjectionProbabilityPct );
    if ( errT == JET_errTestInjectionNotSupported )
    {
        wprintf( L"\tSkipping this test in retail, because fault injection is not enabled.\n" );
        Assert( fFalse );
        err = JET_errSuccess;
        goto HandleError;
    }


{
    QWORD ipg = 37;
    const HRT dhrtMapStart = HrtHRTCount();
    OSTestCall( pfapi->ErrMMCopy( ipg * cbPage, cbPage, (void**)&pvMap1 ) );
    const double dbltimeMap = DblHRTElapsedTimeFromHrtStart( dhrtMapStart );
    OSTestCheck( NULL != pvMap1 );

    OSTestCheck( !FOSMemoryPageResident( pvMap1, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pvMap1, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pvMap1, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pvMap1, cbPage ) );

    const HRT hrtIoStart = HrtHRTCount();
    OSTestCall( pfapi->ErrMMIORead( ipg * cbPage, pvMap1, cbPage, IFileAPI::fmmiorfKeepCleanMapped ) );
    const double dbltimeIo = DblHRTElapsedTimeFromHrtStart( hrtIoStart );

    OSTestCheck( FOSMemoryPageResident( pvMap1, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pvMap1, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pvMap1, cbPage ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pvMap1, cbPage ) );  

    OSTestCheck( pvMap1[3] == ipg );
    OSTestCheck( pvMap1[30] == ipg );

    wprintf( L"\tMap and IO Latencies (us): %f / %f.\n", dbltimeMap * 1000000.0, dbltimeIo * 1000000.0 );
}

HandleError:

    ErrEnableTestInjection( 46860, 0, JET_TestInjectConfigOverride, 0, JET_bitInjectionProbabilityPct );

    if ( pfapi )
    {
        pfapi->ErrMMFree( pvMap1 );
    }
    delete pfapi;
    delete pfsapi;
    OSTerm();

    return err;
}

CUnitTest( OSLayerErrMMIOReadFunctionCowsOnUnsafePaths, 0, "Tests that OS Layer will auto-COW memory on non-safe _paths_ (as opposed to disks in previous test)." );
ERR OSLayerErrMMIOReadFunctionCowsOnUnsafePaths::ErrTest()
{
    COSLayerPreInit oslayer;
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pvMap1 = NULL;


    wprintf( L"\tTest Options: %d \n", 0x0 );
    srand( TickOSTimeCurrent() );
    
    WCHAR * wszFileDat = L".\\MappableFile.dat";
    Call( ErrCreateIncrementFile( wszFileDat, fFalse ) );

    wprintf( L"\tReopen file in mapped mode\n" );

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL, &pfsapi ) );

    WCHAR wszFullPath[IFileSystemAPI::cchPathMax];
    Call( pfsapi->ErrPathComplete( wszFileDat, wszFullPath ) );
    Call( pfsapi->ErrPathParse( wszFullPath, g_rgwchSysDriveTesting, NULL, NULL ) );
    OSStrCbCopyW( wszFullPath, sizeof(wszFullPath), g_rgwchSysDriveTesting );

    wprintf( L"\t\tPre-open purging file first.\n" );
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapi ) );
    delete pfapi;
    pfapi = NULL;

    wprintf( L"\tTesting for a drive that is NOT a \"fixed\" drive (by ESE's funky definition of not being a system drive) ...\n" );
    
    BOOL fSucceeded = fFalse;
    for( WCHAR wch = L'C'; !fSucceeded || wch < L'I' ; wch++  )
    {
        QWORD ipg = 45;

        if ( wch == 'Z' )
        {
            OSTestCheck( fSucceeded );
            break;
        }

        g_rgwchSysDriveTesting[0] = wch;
        g_rgwchSysDriveTesting[1] = L':';
        g_rgwchSysDriveTesting[2] = L'\\';
        g_rgwchSysDriveTesting[3] = L'\0';

        wprintf( L"\t\tTested (%ws) and ..", g_rgwchSysDriveTesting );

        Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

        OSTestCall( pfapi->ErrMMCopy( ipg * cbPage, cbPage, (void**)&pvMap1 ) );
        OSTestCheck( NULL != pvMap1 );

        OSTestCheck( !FOSMemoryPageAllocated( pvMap1, cbPage ) );
        OSTestCheck( FOSMemoryFileMapped( pvMap1, cbPage ) );
        OSTestCheck( !FOSMemoryFileMappedCowed( pvMap1, cbPage ) );

        OSTestCall( pfapi->ErrMMIORead( ipg * cbPage, pvMap1, cbPage, IFileAPI::fmmiorfKeepCleanMapped ) );

        OSTestCheck( FOSMemoryPageResident( pvMap1, cbPage ) );
        OSTestCheck( FOSMemoryFileMapped( pvMap1, cbPage ) );

        wprintf( L".. and it was fFixed / !fCow = %d\n", !FOSMemoryFileMappedCowed( pvMap1, cbPage ) );
        fSucceeded = fSucceeded || FOSMemoryFileMappedCowed( pvMap1, cbPage );

        pfapi->ErrMMFree( pvMap1 );
        pvMap1 = NULL;
        delete pfapi;
        pfapi = NULL;
        }

HandleError:

    g_rgwchSysDriveTesting[0] = L'\0';

    if ( pfapi )
    {
        pfapi->ErrMMFree( pvMap1 );
    }
    delete pfapi;
    delete pfsapi;
    OSTerm();

    return err;
}


CUnitTest( OSLayerErrMMReadThrowsFileIOBeyondEOF, 0, "Tests that ErrMMIORead/ErrMMCopy throws a JET_errFileIOBeyondEOF error when we try to map past EOF." );
ERR OSLayerErrMMReadThrowsFileIOBeyondEOF::ErrTest()
{
    BYTE * pvMap = NULL;

    const QWORD ipg = 256 * 32 * 1025 / 4096 + 3;

    MemoryMappedIoTestInit;

    wprintf( L"\t\tTrying to read current status of (%I64d) ...\n", ipg );

    err = pfapi->ErrMMRead( ipg * cbPage, cbPage, (void**)&pvMap );
    OSTestCheck( NULL == pvMap );
    OSTestCheck( err == JET_errFileIOBeyondEOF );
    wprintf( L"\t\t\tErrMMRead() --> %d\n", err );

    err = pfapi->ErrMMCopy( ipg * cbPage, cbPage, (void**)&pvMap );
    OSTestCheck( NULL == pvMap );
    OSTestCheck( err == JET_errFileIOBeyondEOF );
    wprintf( L"\t\t\tErrMMCopy() --> %d\n", err );

    err = JET_errSuccess;

HandleError:

    if ( pfapi )
    {
        pfapi->ErrMMFree( pvMap );
    }

    MemoryMappedIoTestTerm;

    return err;
}

CUnitTest( OSLayerErrMMReadToErrMMIOReadFaultInjectionThrowsDiskIoErr, 0, "Tests that ErrMMRead-based Fault injection (which is what esetest _may be_ using) throws a JET_errDiskIO error on a generic error case." );
ERR OSLayerErrMMReadToErrMMIOReadFaultInjectionThrowsDiskIoErr::ErrTest()
{

    const BOOL fMmCopyInstead = fFalse;
    BYTE * pvMap1 = NULL;
    BYTE * pvMap2 = NULL;

    MemoryMappedIoTestInit;

{
    wprintf( L"\tBegin first set of erroring MMIORead ... \n" );
    const QWORD ipg = rand() % ( CbIncrementalFileSize( fFalse ) / OSMemoryPageCommitGranularity() ) + 1;

    Call( ErrEnableTestInjection( 34060, (ULONG_PTR)-1, JET_TestInjectFault, 100, JET_bitInjectionProbabilityPct ) );
    if ( !fMmCopyInstead )
    {
        OSTestCall( pfapi->ErrMMRead( ipg * cbPage, cbPage, (void**)&pvMap1 ) );
    }
    else
    {
        OSTestCall( pfapi->ErrMMCopy( ipg * cbPage, cbPage, (void**)&pvMap1 ) );
    }
    OSTestCheck( NULL != pvMap1 );
    OSTestCall( ErrEnableTestInjection( 34060, (ULONG_PTR)JET_errSuccess, JET_TestInjectFault, 0, JET_bitInjectionProbabilityCleanup ) );

    err = pfapi->ErrMMIORead( ipg * cbPage, pvMap1, cbPage, IFileAPI::fmmiorfKeepCleanMapped );
    OSTestCheck( JET_errDiskIO == err );
    err = JET_errSuccess;

    OSTestCheck( FOSMemoryFileMapped( pvMap1, cbPage ) );
    OSTestCheck( !FOSMemoryPageResident( pvMap1, cbPage ) );
    wprintf( L"\tEnd first set of erroring test.\n" );
    }

{
    wprintf( L"\tBegin second set of erroring MMIORead ... \n" );
    const QWORD ipg = rand() % 255 + 1;
    OSTestCall( ErrEnableTestInjection( 34060, (ULONG_PTR)-1, JET_TestInjectFault, 100, JET_bitInjectionProbabilityPct ) );
    OSTestCall( pfapi->ErrMMCopy( ipg * cbPage, cbPage, (void**)&pvMap2 ) );
    OSTestCheck( NULL != pvMap2 );
    Call( ErrEnableTestInjection( 34060, (ULONG_PTR)JET_errSuccess, JET_TestInjectFault, 0, JET_bitInjectionProbabilityCleanup ) );

    err = pfapi->ErrMMIORead( ipg * cbPage, pvMap2, cbPage, IFileAPI::fmmiorfCopyWritePage );
    OSTestCheck( JET_errDiskIO == err );
    err = JET_errSuccess;

    OSTestCheck( FOSMemoryFileMapped( pvMap2, cbPage ) );
    OSTestCheck( !FOSMemoryPageResident( pvMap2, cbPage ) );
    wprintf( L"\tEnd second set of erroring test.\n" );
}

HandleError:

    if ( JET_errTestInjectionNotSupported == err )
    {
        wprintf( L"\tCould not run test due to lack of fault injection.  Ok.\n" );
        err = JET_errSuccess;
    }

    if ( pfapi )
    {
        pfapi->ErrMMFree( pvMap1 );
        pfapi->ErrMMFree( pvMap2 );
    }

    MemoryMappedIoTestTerm;

    return err;
}

CUnitTest( OSLayerErrMMRevertMaterializesLastWriteCallData, 0, "Tests that OS Layer can write an update page and revert to the previously written data." );
ERR OSLayerErrMMRevertMaterializesLastWriteCallData::ErrTest()
{
    COSLayerPreInit oslayer;
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pvMap1 = NULL;


    wprintf( L"\tTest Options: %d \n", 0x0 );
    srand( TickOSTimeCurrent() );
    
    WCHAR * wszFileDat = L".\\MappableFile.dat";
    Call( ErrCreateIncrementFile( wszFileDat, fFalse ) );

    wprintf( L"\tReopen file in mapped mode\n" );

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL, &pfsapi ) );

    WCHAR wszFullPath[IFileSystemAPI::cchPathMax];
    Call( pfsapi->ErrPathComplete( wszFileDat, wszFullPath ) );
    Call( pfsapi->ErrPathParse( wszFullPath, g_rgwchSysDriveTesting, NULL, NULL ) );

    wprintf( L"\t\tPre-open purging file first.\n" );
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapi ) );
    delete pfapi;
    pfapi = NULL;

    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

{
    QWORD ipg = 42;
    Call( pfapi->ErrMMCopy( ipg * cbPage, cbPage, (void**)&pvMap1 ) );
    OSTestCheck( NULL != pvMap1 );

    OSTestCheck( !FOSMemoryPageResident( pvMap1, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pvMap1, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pvMap1, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pvMap1, cbPage ) );

    OSTestCall( pfapi->ErrMMIORead( ipg * cbPage, pvMap1, cbPage, IFileAPI::fmmiorfKeepCleanMapped ) );

    OSTestCheck( FOSMemoryPageResident( pvMap1, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pvMap1, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pvMap1, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pvMap1, cbPage ) );

    OSTestCheck( pvMap1[3] == ipg );
    OSTestCheck( pvMap1[30] == ipg );

    OSTestCheck( pvMap1[3] != 0x43 );

    pvMap1[3] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pvMap1, cbPage ) );

    if ( !FUtilOsIsGeq( 6, 2 ) )
    {
        wprintf(L"Feature ErrMMRevert() is not available on Win NT %d.%d, skipping remainder of test.\n", DwUtilSystemVersionMajor(), DwUtilSystemVersionMinor() );
        err = JET_errSuccess;
        goto HandleError;
    }

    OSTestCheck( pvMap1[3] == 0x42 );
    OSTestCall( pfapi->ErrMMRevert( ipg * cbPage, pvMap1, cbPage ) );
    OSTestCheck( pvMap1[3] == ipg );
    OSTestCheck( !FOSMemoryFileMappedCowed( pvMap1, cbPage ) );

    pvMap1[3] = 0x43;
    OSTestCheck( FOSMemoryFileMappedCowed( pvMap1, cbPage ) );

    OSTestCall( pfapi->ErrIOWrite( *TraceContextScope( iorpOSUnitTest ), ipg * cbPage, cbPage, pvMap1, qosIONormal ) );
    OSTestCheck( pvMap1[3] == 0x43 );

    pvMap1[3] = 0x44;

    OSTestCall( pfapi->ErrMMRevert( ipg * cbPage, pvMap1, cbPage ) );
    OSTestCheck( pvMap1[3] == 0x43 );
}

HandleError:

    g_rgwchSysDriveTesting[0] = L'\0';

    if ( pfapi )
    {
        pfapi->ErrMMFree( pvMap1 );
    }
    delete pfapi;
    delete pfsapi;
    OSTerm();

    return err;
}


