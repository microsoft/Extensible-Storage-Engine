// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

enum IOREASONPRIMARY : BYTE
{
    // iorpNone publically defined
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
    OSTestCall( ErrOSFSCreate( NULL/* pinstNil */, &pfsapi ) );     \
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


//  There is no config override injection for complex or vector types, so just reach into
//  the OS layer and config override it myself.
extern WCHAR g_rgwchSysDriveTesting[IFileSystemAPI::cchPathMax];


CUnitTest( OSLayerErrMMIOReadFunctions, 0, "Tests that OS Layer can detect non resident cache." );
ERR OSLayerErrMMIOReadFunctions::ErrTest()
{
    COSLayerPreInit oslayer; // FOSPreinit()
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pvMap1 = NULL;
    BYTE * pvMap2 = NULL;

    //  test mode(s)
    const BOOL fMmCopyInstead = fFalse;
    const BOOL fBigFileRandPage = fFalse;

    wprintf( L"\tTest Options: %d \n", fMmCopyInstead );
    srand( TickOSTimeCurrent() );
    
    WCHAR * wszFileDat = fBigFileRandPage ? L".\\MappableBigFile.dat" : L".\\MappableFile.dat";
    Call( ErrCreateIncrementFile( wszFileDat, fBigFileRandPage ) );

    wprintf( L"\tReopen file in mapped mode\n" );

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL/* pinstNil */, &pfsapi ) );

    //  Force this file to be considered to be on fixed disk (note we have two methods of doing this
    //  so we'll randomly try both - note in other tests we reply on each of these once).
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
            //  probably because of retail ...
            goto RetryWithPathInjection;
        }
    }

    wprintf( L"\t\tPre-open purging file first.\n" );
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapi ) );
    delete pfapi;   //  Close it again.
    pfapi = NULL;

    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

    //  Test the ErrMMIORead() API ...
{
    // consider setting ipg = rand() % 256;
    QWORD ipg = 37;
    if ( fBigFileRandPage )
    {
        ipg = ( ( rand() << 16 ) | rand() ) % ( CbIncrementalFileSize( fTrue ) / cbPage );
        wprintf( L"\tSelecting random page %I64d\n", ipg );
    }
    const HRT dhrtMapStart = HrtHRTCount();
    //  by default this convienently checks both ErrMMRead() and ErrMMCopy() - read here and must be copy in the 2nd attempt.
    OSTestCall( pfapi->ErrMMRead( ipg * cbPage, cbPage, (void**)&pvMap1 ) );
    const double dbltimeMap = DblHRTElapsedTimeFromHrtStart( dhrtMapStart );
    OSTestCheck( NULL != pvMap1 );

    OSTestCheck( !FOSMemoryPageResident( pvMap1, cbPage ) );    //  expected unless it ?already? got recached, this should be false?  may go off.
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

    //  honestly IO is not what I would expect, UNTIL I implemented fBigFileRandPage - must've been cached by HD cache
    wprintf( L"\tMap and IO Latencies (us): %f / %f.\n", dbltimeMap * 1000000.0, dbltimeIo * 1000000.0 );
}


    //  Do a version but with COWing the page
{
    QWORD ipg = 73;
    if ( fBigFileRandPage )
    {
        ipg = ( ( rand() << 16 ) | rand() ) % ( CbIncrementalFileSize( fTrue ) / cbPage );
        wprintf( L"\tSelecting random page %I64d\n", ipg );
    }
    const HRT dhrtMapStart = HrtHRTCount();
    //  Can't do ErrMMRead() or we'd AV at IFileAPI::fmmiorfCopyWritePage usage.
    OSTestCall( pfapi->ErrMMCopy( ipg * cbPage, cbPage, (void**)&pvMap2 ) );
    const double dbltimeMap = DblHRTElapsedTimeFromHrtStart( dhrtMapStart );
    OSTestCheck( NULL != pvMap2 );

    OSTestCheck( !FOSMemoryPageResident( pvMap2, cbPage ) );    //  expected unless it ?already? got recached, this should be false?  may go off.
    OSTestCheck( !FOSMemoryPageAllocated( pvMap2, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pvMap2, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pvMap2, cbPage ) );

    const HRT hrtIoStart = HrtHRTCount();
    OSTestCall( pfapi->ErrMMIORead( ipg * cbPage, pvMap2, cbPage, IFileAPI::fmmiorfCopyWritePage ) );
    const double dbltimeIo = DblHRTElapsedTimeFromHrtStart( hrtIoStart );

    OSTestCheck( FOSMemoryPageResident( pvMap2, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pvMap2, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pvMap2, cbPage ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pvMap2, cbPage ) );      // different from previous test

    OSTestCheck( pvMap2[3] == ipg  || fBigFileRandPage );
    OSTestCheck( pvMap2[30] == ipg || fBigFileRandPage );

    wprintf( L"\tMap and IO Latencies (us): %f / %f.\n", dbltimeMap * 1000000.0, dbltimeIo * 1000000.0 );
}

HandleError:

    g_rgwchSysDriveTesting[0] = L'\0';
    ErrEnableTestInjection( 46860, 0, JET_TestInjectConfigOverride, 0, JET_bitInjectionProbabilityPct );

    if ( pfapi )
    {
        //  not necessary, but cleaner ...
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
    COSLayerPreInit oslayer; // FOSPreinit()
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
    Call( ErrOSFSCreate( NULL/* pinstNil */, &pfsapi ) );

    wprintf( L"\t\tPre-open purging file first.\n" );
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapi ) );
    delete pfapi;   //  Close it again.
    pfapi = NULL;

    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

    //  Force this file to be considered to be on a NON-fixed disk.
    JET_ERR errT = ErrEnableTestInjection( 46860, 0, JET_TestInjectConfigOverride, 100, JET_bitInjectionProbabilityPct );
    if ( errT == JET_errTestInjectionNotSupported )
    {
        wprintf( L"\tSkipping this test in retail, because fault injection is not enabled.\n" );
        Assert( fFalse );   //  In debug we should never get here!
        err = JET_errSuccess;
        goto HandleError;
    }


    //  Test the ErrMMIORead() API ...
{
    // consider setting ipg = rand() % 256;
    QWORD ipg = 37;
    const HRT dhrtMapStart = HrtHRTCount();
    OSTestCall( pfapi->ErrMMCopy( ipg * cbPage, cbPage, (void**)&pvMap1 ) );
    const double dbltimeMap = DblHRTElapsedTimeFromHrtStart( dhrtMapStart );
    OSTestCheck( NULL != pvMap1 );

    OSTestCheck( !FOSMemoryPageResident( pvMap1, cbPage ) );    //  expected unless it ?already? got recached, this should be false?  may go off.
    OSTestCheck( !FOSMemoryPageAllocated( pvMap1, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pvMap1, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pvMap1, cbPage ) );

    const HRT hrtIoStart = HrtHRTCount();
    OSTestCall( pfapi->ErrMMIORead( ipg * cbPage, pvMap1, cbPage, IFileAPI::fmmiorfKeepCleanMapped ) );
    const double dbltimeIo = DblHRTElapsedTimeFromHrtStart( hrtIoStart );

    OSTestCheck( FOSMemoryPageResident( pvMap1, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pvMap1, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pvMap1, cbPage ) );
    //  In test OSLayerErrMMIOReadFunctions() we check !..Cowed() - this is opposite b/c fFixedDisk == fFalse
    OSTestCheck( FOSMemoryFileMappedCowed( pvMap1, cbPage ) );  

    OSTestCheck( pvMap1[3] == ipg );    //   and should still be valid data
    OSTestCheck( pvMap1[30] == ipg );

    //  honestly IO is not what I would expect, UNTIL I implemented fBigFileRandPage - must've been cached by HD cache
    wprintf( L"\tMap and IO Latencies (us): %f / %f.\n", dbltimeMap * 1000000.0, dbltimeIo * 1000000.0 );
}

HandleError:

    ErrEnableTestInjection( 46860, 0, JET_TestInjectConfigOverride, 0, JET_bitInjectionProbabilityPct );

    if ( pfapi )
    {
        //  not necessary, but cleaner ...
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
    COSLayerPreInit oslayer; // FOSPreinit()
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pvMap1 = NULL;

    //  test mode(s)

    wprintf( L"\tTest Options: %d \n", 0x0 );
    srand( TickOSTimeCurrent() );
    
    WCHAR * wszFileDat = L".\\MappableFile.dat";
    Call( ErrCreateIncrementFile( wszFileDat, fFalse ) );

    wprintf( L"\tReopen file in mapped mode\n" );

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL/* pinstNil */, &pfsapi ) );

    WCHAR wszFullPath[IFileSystemAPI::cchPathMax];
    Call( pfsapi->ErrPathComplete( wszFileDat, wszFullPath ) );
    Call( pfsapi->ErrPathParse( wszFullPath, g_rgwchSysDriveTesting, NULL, NULL ) );
    OSStrCbCopyW( wszFullPath, sizeof(wszFullPath), g_rgwchSysDriveTesting );

    wprintf( L"\t\tPre-open purging file first.\n" );
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapi ) );
    delete pfapi;   //  Close it again.
    pfapi = NULL;

    wprintf( L"\tTesting for a drive that is NOT a \"fixed\" drive (by ESE's funky definition of not being a system drive) ...\n" );
    
    //  Iterate through all the drives, at least one should fail ...
    BOOL fSucceeded = fFalse;
    for( WCHAR wch = L'C'; !fSucceeded || wch < L'I' /* want it to go at least through G:\ */; wch++  )
    {
        QWORD ipg = 45;

        if ( wch == 'Z' )
        {
            //  Obviously we have failed to find a single non-fixed disk, so throw a fit ...
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
        fSucceeded = fSucceeded || FOSMemoryFileMappedCowed( pvMap1, cbPage );      // different from previous test

        pfapi->ErrMMFree( pvMap1 );
        pvMap1 = NULL;
        delete pfapi;
        pfapi = NULL;
        }

HandleError:

    g_rgwchSysDriveTesting[0] = L'\0';

    if ( pfapi )
    {
        //  not necessary, but cleaner ...
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

    //  by default this convienently checks both ErrMMRead() and ErrMMCopy() - read here and must be copy in the 2nd attempt.
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
    //  test mode(s)

    const BOOL fMmCopyInstead = fFalse;
    BYTE * pvMap1 = NULL;
    BYTE * pvMap2 = NULL;

    MemoryMappedIoTestInit;

    //  Test the ErrMMIORead() API ...
{
    wprintf( L"\tBegin first set of erroring MMIORead ... \n" );
    const QWORD ipg = rand() % ( CbIncrementalFileSize( fFalse ) / OSMemoryPageCommitGranularity() ) + 1;

    //  by default this convienently checks both ErrMMRead() and ErrMMCopy() - read here and must be copy in the 2nd attempt.
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
    OSTestCheck( JET_errDiskIO == err );    //  test we gave back the right error
    err = JET_errSuccess;

    OSTestCheck( FOSMemoryFileMapped( pvMap1, cbPage ) );       //  should still be true after err, important for asserts in BF
    // note this isn't a REAL test of residency ... b/c we're emulating the fault, so we're just bailing before 
    // the deref, rather than validating what the OS is giving us.
    OSTestCheck( !FOSMemoryPageResident( pvMap1, cbPage ) );    //  shouldn't be referencable b/c of error
    wprintf( L"\tEnd first set of erroring test.\n" );
    }

    //  Do a version but with COWing the page
{
    wprintf( L"\tBegin second set of erroring MMIORead ... \n" );
    const QWORD ipg = rand() % 255 + 1;
    //  Can't do ErrMMRead() or we'd AV at IFileAPI::fmmiorfCopyWritePage usage.
    OSTestCall( ErrEnableTestInjection( 34060, (ULONG_PTR)-1, JET_TestInjectFault, 100, JET_bitInjectionProbabilityPct ) );
    OSTestCall( pfapi->ErrMMCopy( ipg * cbPage, cbPage, (void**)&pvMap2 ) );
    OSTestCheck( NULL != pvMap2 );
    Call( ErrEnableTestInjection( 34060, (ULONG_PTR)JET_errSuccess, JET_TestInjectFault, 0, JET_bitInjectionProbabilityCleanup ) );

    err = pfapi->ErrMMIORead( ipg * cbPage, pvMap2, cbPage, IFileAPI::fmmiorfCopyWritePage );
    OSTestCheck( JET_errDiskIO == err );    //  test we gave back the right error
    err = JET_errSuccess;

    OSTestCheck( FOSMemoryFileMapped( pvMap2, cbPage ) );       //  should still be true after err, important for asserts in BF
    // note this isn't a REAL test of residency ... b/c we're emulating the fault, so we're just bailing before 
    // the deref, rather than validating what the OS is giving us.
    OSTestCheck( !FOSMemoryPageResident( pvMap2, cbPage ) );    //  shouldn't be referencable b/c of error
    wprintf( L"\tEnd second set of erroring test.\n" );
}

HandleError:

    if ( JET_errTestInjectionNotSupported == err )
    {
        //  Well, not much you can do about that.
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
    COSLayerPreInit oslayer; // FOSPreinit()
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pvMap1 = NULL;

    //  test mode(s)

    wprintf( L"\tTest Options: %d \n", 0x0 );
    srand( TickOSTimeCurrent() );
    
    WCHAR * wszFileDat = L".\\MappableFile.dat";
    Call( ErrCreateIncrementFile( wszFileDat, fFalse ) );

    wprintf( L"\tReopen file in mapped mode\n" );

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL/* pinstNil */, &pfsapi ) );

    WCHAR wszFullPath[IFileSystemAPI::cchPathMax];
    Call( pfsapi->ErrPathComplete( wszFileDat, wszFullPath ) );
    Call( pfsapi->ErrPathParse( wszFullPath, g_rgwchSysDriveTesting, NULL, NULL ) );

    wprintf( L"\t\tPre-open purging file first.\n" );
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapi ) );
    delete pfapi;   //  Close it again.
    pfapi = NULL;

    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

    //  Test the ErrMMIORead() API ...
{
    // consider setting ipg = rand() % 256;
    QWORD ipg = 42;
    Call( pfapi->ErrMMCopy( ipg * cbPage, cbPage, (void**)&pvMap1 ) );
    OSTestCheck( NULL != pvMap1 );

    OSTestCheck( !FOSMemoryPageResident( pvMap1, cbPage ) );    //  expected unless it ?already? got recached, this should be false?  may go off.
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

    OSTestCheck( pvMap1[3] != 0x43 );   //  otherwise tests below aren't quite right ...

    //  This will COW the page
    pvMap1[3] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pvMap1, cbPage ) );

    if ( !FUtilOsIsGeq( 6, 2 ) )
    {
        wprintf(L"Feature ErrMMRevert() is not available on Win NT %d.%d, skipping remainder of test.\n", DwUtilSystemVersionMajor(), DwUtilSystemVersionMinor() );
        err = JET_errSuccess;
        goto HandleError;
    }

    //  Revert the page (just doing this for practice) ... 
    OSTestCheck( pvMap1[3] == 0x42 );
    OSTestCall( pfapi->ErrMMRevert( ipg * cbPage, pvMap1, cbPage ) );
    OSTestCheck( pvMap1[3] == ipg );
    OSTestCheck( !FOSMemoryFileMappedCowed( pvMap1, cbPage ) );

    //  OK, now re-COW the page, this image we'll be writing ...
    pvMap1[3] = 0x43;
    OSTestCheck( FOSMemoryFileMappedCowed( pvMap1, cbPage ) );

    OSTestCall( pfapi->ErrIOWrite( *TraceContextScope( iorpOSUnitTest ), ipg * cbPage, cbPage, pvMap1, qosIONormal ) );
    OSTestCheck( pvMap1[3] == 0x43 );

    //  Update it again, before we revert it (we should lose this update)
    pvMap1[3] = 0x44;

    //  The actual test ...
    OSTestCall( pfapi->ErrMMRevert( ipg * cbPage, pvMap1, cbPage ) );
    OSTestCheck( pvMap1[3] == 0x43 );
}

HandleError:

    g_rgwchSysDriveTesting[0] = L'\0';

    if ( pfapi )
    {
        //  not necessary, but cleaner ...
        pfapi->ErrMMFree( pvMap1 );
    }
    delete pfapi;
    delete pfsapi;
    OSTerm();

    return err;
}


