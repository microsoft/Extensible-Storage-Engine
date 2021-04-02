// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

//  ================================================================

JET_ERR ErrMathAlignmentRounding( void )
{
    JET_ERR err = JET_wrnNyi;

    wprintf(L"\t\tAlignment rounding tests...\n");

    OSTestCheck( 0 == rounddn( 0, 2 ) );
    OSTestCheck( 0 == roundup( 0, 2 ) );
    OSTestCheck( 0 == rounddn( 1, 2 ) );
    OSTestCheck( 2 == roundup( 1, 2 ) );
    OSTestCheck( 2 == rounddn( 2, 2 ) );
    OSTestCheck( 2 == roundup( 2, 2 ) );

    OSTestCheck( 0 == rounddn( 0, 32 ) );
    OSTestCheck( 0 == roundup( 0, 32 ) );
    OSTestCheck( 0 == rounddn( 1, 32 ) );
    OSTestCheck( 32 == roundup( 1, 32 ) );
    OSTestCheck( 32 == roundup( 28, 32 ) ); // fairly relevant
    OSTestCheck( 0 == rounddn( 31, 32 ) );
    OSTestCheck( 32 == roundup( 31, 32 ) );
    OSTestCheck( 32 == rounddn( 32, 32 ) );
    OSTestCheck( 32 == roundup( 32, 32 ) );
    OSTestCheck( 32 == rounddn( 33, 32 ) );
    OSTestCheck( 32 != roundup( 33, 32 ) ); // xtra sure
    OSTestCheck( 64 == roundup( 33, 32 ) );

    OSTestCheck( 0xFFFFFFF8 == roundup( 0xFFFFFFF1, 8 ) );
    // Rather comfortingly, this actually fails to compile with integral constant overflow.
    //OSTestCheck( 0x0 == roundup( 0xFFFFFFF1, 16 ) );

    err = JET_errSuccess;

HandleError:

    return err;
}

JET_ERR ErrMathDivisionRounding( void )
{
    JET_ERR err = JET_wrnNyi;

    wprintf(L"\t\tDivision rounding tests...\n");

    OSTestCheck( 0 == roundupdiv( 0, 3 ) );
    OSTestCheck( 1 == roundupdiv( 1, 3 ) );
    OSTestCheck( 1 == roundupdiv( 2, 3 ) );
    OSTestCheck( 1 == roundupdiv( 3, 3 ) );
    OSTestCheck( 2 == roundupdiv( 4, 3 ) );

    OSTestCheck( 0 == roundupdiv( 0, 4096 ) );
    OSTestCheck( 1 == roundupdiv( 1, 4096 ) );
    OSTestCheck( 1 == roundupdiv( 4096, 4096 ) );
    OSTestCheck( 2 == roundupdiv( 4097, 4096 ) );
    OSTestCheck( 2 == roundupdiv( 8192, 4096 ) );
    OSTestCheck( 3 == roundupdiv( 8193, 4096 ) );

    err = JET_errSuccess;

HandleError:

    return err;
}

//  ================================================================

JET_ERR ErrFPowerOf2( void )
{
    JET_ERR err = JET_wrnNyi;

    wprintf(L"\t\tFPowerOf2 tests...\n");

    OSTestCheck( false == FPowerOf2( 0 ) );
    OSTestCheck( true == FPowerOf2( 1 ) );
    OSTestCheck( true == FPowerOf2( 2 ) );
    OSTestCheck( false == FPowerOf2( 3 ) );
    OSTestCheck( true == FPowerOf2( 4 ) );
    OSTestCheck( false == FPowerOf2( 5 ) );
    OSTestCheck( false == FPowerOf2( 6 ) );
    OSTestCheck( false == FPowerOf2( 7 ) );
    OSTestCheck( true == FPowerOf2( 8 ) );
    OSTestCheck( false == FPowerOf2( 9 ) );

    for ( DWORD x = 4; x != 0; x <<= 1 )
    {
        OSTestCheck( false == FPowerOf2( x - 1 ) );
        OSTestCheck( true == FPowerOf2( x ) );
        OSTestCheck( false == FPowerOf2( x + 1 ) );
    }

    err = JET_errSuccess;

HandleError:

    return err;
}

//  ================================================================

JET_ERR ErrLog2( void )
{
    JET_ERR err = JET_wrnNyi;

    wprintf(L"\t\tLog2 tests...\n");

    OSTestCheck( 0 == Log2( 1 ) );
    OSTestCheck( 1 == Log2( 2 ) );
    OSTestCheck( 2 == Log2( 4 ) );
    OSTestCheck( 3 == Log2( 8 ) );
    OSTestCheck( 4 == Log2( 16 ) );

    for ( DWORD x = 1; x != 0; x <<= 1 )
    {
        OSTestCheck( x == ( DWORD( 1 ) << Log2( x ) ) );
    }

    err = JET_errSuccess;

HandleError:

    return err;
}

//  ================================================================

JET_ERR ErrLNextPowerOf2( void )
{
    JET_ERR err = JET_wrnNyi;

    wprintf(L"\t\tLNextPowerOf2 tests...\n");

    OSTestCheck( 1 == LNextPowerOf2( 0 ) );
    OSTestCheck( 1 == LNextPowerOf2( 1 ) );
    OSTestCheck( 2 == LNextPowerOf2( 2 ) );
    OSTestCheck( 4 == LNextPowerOf2( 3 ) );
    OSTestCheck( 4 == LNextPowerOf2( 4 ) );
    OSTestCheck( 8 == LNextPowerOf2( 5 ) );
    OSTestCheck( 8 == LNextPowerOf2( 6 ) );
    OSTestCheck( 8 == LNextPowerOf2( 7 ) );
    OSTestCheck( 8 == LNextPowerOf2( 8 ) );
    OSTestCheck( 16 == LNextPowerOf2( 9 ) );
    OSTestCheck( 16 == LNextPowerOf2( 10 ) );

    for ( LONG x = 4; x < ( lMax / 2 + 1 ); x <<= 1 )
    {
        const LONG y = LNextPowerOf2( x + 1 );
        OSTestCheck( true == FPowerOf2( x ) );
        OSTestCheck( true == FPowerOf2( y ) );
        OSTestCheck( x == LNextPowerOf2( x - 1 ) );
        OSTestCheck( x == LNextPowerOf2( x ) );
        OSTestCheck( y == LNextPowerOf2( x + 2 ) );
        OSTestCheck( 2 == ( y / x ) );
        OSTestCheck( 1 == ( Log2( y ) - Log2( x ) ) );
    }

    OSTestCheck( ( lMax / 2 + 1 ) == LNextPowerOf2( lMax / 2 - 1 ) );
    OSTestCheck( ( lMax / 2 + 1 ) == LNextPowerOf2( lMax / 2 ) );
    OSTestCheck( ( lMax / 2 + 1 ) == LNextPowerOf2( lMax / 2 + 1 ) );
    OSTestCheck( -1 == LNextPowerOf2( lMax / 2 + 2 ) );
    OSTestCheck( -1 == LNextPowerOf2( lMax / 2 + 3 ) );
    OSTestCheck( -1 == LNextPowerOf2( lMax / 2 + 4 ) );

    err = JET_errSuccess;

HandleError:

    return err;
}

//  ================================================================

JET_ERR ErrHalfAvalancheHash()
{
    // Hash a 16-bit number space into 8-bit buckets, and count collisions for each bucket.
    //
    const INT cBits = 8;
    const INT cbBuckets = 1 << cBits;
    const UINT maxVal = 0xffffffff;
    JET_ERR err = JET_wrnNyi;

    wprintf( L"HalfAvalancheHash() on %lu integers to %d bits.\n", maxVal, cBits );

    UINT rgBuckets[ cbBuckets ];
    for ( UINT i = 0; i < maxVal; i++ )
    {
        UINT hash = HalfAvalancheHash( i, cBits );
        OSTestCheck( hash < cbBuckets );
        rgBuckets[ hash ]++;
    }

    // Output the buckets
    wprintf( L"[Bucket] = count\n" );
    const INT cColumns = 4;
    const INT cRows = cbBuckets / cColumns;
    for ( INT row = 0; row < cRows; row++ )
    {
        INT index = row;
        wprintf( L"[0x%02x] = %10lu", index, rgBuckets[ index ] );
        for ( INT col = 1; col < cColumns; col++ )
        {
            index = col * cRows + row;
            wprintf( L"\t[0x%02x] = %10lu", index, rgBuckets[ index ] );
        }
        wprintf( L"\n" );
    }

    err = JET_errSuccess;

HandleError:
    return err;
}

//  ================================================================

CUnitTest( MathTest, 0, "And you thought you were done with math tests after college." );
ERR MathTest::ErrTest()
{
    JET_ERR         err = JET_errSuccess;

// Note: The math functionality should all work without OS pre-init and OS init
// having been run.  This is ensuring that.
//
//  COSLayerPreInit     oslayer;
//  if ( !oslayer.FInitd() )
//      {
//      wprintf( L"Out of memory error during OS Layer pre-init." );
//      err = JET_errOutOfMemory;
//      goto HandleError;
//      }
//  OSTestCall( ErrOSInit() );

    wprintf( L"\tYou may open your booklets and begin your math test...\n");

    OSTestCall( ErrMathAlignmentRounding() );
    OSTestCall( ErrMathDivisionRounding() );
    OSTestCall( ErrFPowerOf2() );
    OSTestCall( ErrLog2() );
    OSTestCall( ErrLNextPowerOf2() );
    // FUTURE[SOMEONE] Revert when merge conflict for explicit test comes along.
    //OSTestCall( ErrHalfAvalancheHash() );

HandleError:
    if ( err )
    {
        wprintf( L"\tDone(error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

//  OSTerm();

    return err;
}

//  ================================================================

USHORT UsBitsSlow( const DWORD dw )
{
    USHORT usRet = 0;
    for( ULONG curr = dw; curr; curr = curr >> 1 )
    {
        // SOMEONE
        if ( curr & 0x1 )
        {
            usRet++;
        }
    }
    return usRet;
}

CUnitTest( MathTestCountingALLTHEBITS, bitExplicitOnly, "Checks UsBits() function." );
ERR MathTestCountingALLTHEBITS::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    ULONG c = 0;
    ULONG ul = 0;
    C_ASSERT( sizeof( ULONG ) == sizeof( DWORD ) );
    do
    {
        if ( ( ul & 0x00FFFFFF ) == 0 ) // 256 status dots
        {
            printf( "." );
        }
        // test only UsBits() or __popcnt speed.
        //c += UsBits( ul );
        //c += __popcnt( (INT) ul );
        OSTestCheck( UsBits( ul ) == UsBitsSlow( ul ) );
        //OSTestCheck( UsBits( ul ) == __popcnt( (INT) ul ) ); // only works on some CPUs
        ul++;
    }
    while( ul < ulMax );
    c = ul;  // pointless.

HandleError:
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

CUnitTest( MathTestCountingBits, 0, "Checks UsBits() function." );
ERR MathTestCountingBits::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    OSTestCheck( 0 == UsBits( 0 ) );
    OSTestCheck( 1 == UsBits( 1 ) );
    OSTestCheck( 1 == UsBits( 2 ) );
    OSTestCheck( 1 == UsBits( 0x80000000 ) );
    OSTestCheck( 32 == UsBits( 0xFFFFFFFF ) );
    for( ULONG shf = 0; shf < 32; shf++ )
    {
        OSTestCheck( 1 == UsBits( 1 << shf ) );
    }

HandleError:
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