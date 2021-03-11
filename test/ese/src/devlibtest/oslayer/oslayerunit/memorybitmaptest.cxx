// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

#include <tchar.h>
#include "os.hxx"

#define OSLayerStart()                  \
    COSLayerPreInit     oslayer;        \
    if ( !oslayer.FInitd() )            \
        {                               \
        wprintf( L"Out of memory error during OS Layer pre-init." );    \
        err = JET_errOutOfMemory;       \
        goto HandleError;               \
    }                               \
    OSTestCall( ErrOSInit() );



#define CHECK   OSTestCheck

ERR ValidateBasicBitMapFunctionality( IBitmapAPI * const pbm )
{
    JET_ERR err = JET_errSuccess;
    BOOL f = 3;

    for ( ULONG i = 0; i < 16 * 8; i++ )
    {
        f = 3;
        CHECK( IBitmapAPI::ERR::errSuccess == pbm->ErrGet( i, &f ) );
        CHECK( fFalse == f );
    
        if ( i % 2 == 0 )
        {
            CHECK( IBitmapAPI::ERR::errSuccess == pbm->ErrSet( i, fTrue ) );
            CHECK( IBitmapAPI::ERR::errSuccess == pbm->ErrGet( i, &f ) );
            CHECK( fTrue == f );
        }
    
        if ( i % 3 == 0 )
        {
            CHECK( IBitmapAPI::ERR::errSuccess == pbm->ErrSet( i, fFalse ) );
            CHECK( IBitmapAPI::ERR::errSuccess == pbm->ErrGet( i, &f ) );
            CHECK( fFalse == f );
        }
    }
    
    for ( ULONG i = 0; i < 16 * 8; i++ )
    {
        BOOL fExpected = fFalse;
        if ( i % 2 == 0 )
        {
            fExpected = fTrue;
        }
        if ( i % 3 == 0 )
        {
            fExpected = fFalse;
        }
        CHECK( IBitmapAPI::ERR::errSuccess == pbm->ErrGet( i, &f ) );
        CHECK( fExpected == f );
    }
    
    CHECK( IBitmapAPI::ERR::errInvalidParameter == pbm->ErrGet( 16 * 8, &f ) );
    
HandleError:

    return err;
}

CUnitTest( MemorySparseBitMapBasicTest, 0, "This tests that some basic operations work on the CSparseBitmap implementation." );
ERR MemorySparseBitMapBasicTest::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    OSLayerStart();

    IBitmapAPI * pbm = new CSparseBitmap();
    CHECK( IBitmapAPI::ERR::errSuccess == ((CSparseBitmap*)pbm)->ErrInitBitmap( 16 * 8 ) );

    OSTestCall( ValidateBasicBitMapFunctionality( pbm ) );

    delete pbm;

HandleError:

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

