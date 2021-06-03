// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

#include <tchar.h>
#include "os.hxx"


//  ================================================================
//  Exhastive and paranoid tests for CompEndianLowSpLos16b
//  ================================================================

CUnitTest( TypesCompEndianLowSpLos16bBasic, 0, "Very rudimentary test for CompEndianLowSpLos16b compressing USHORT." );
ERR TypesCompEndianLowSpLos16bBasic::ErrTest()
{
    JET_ERR err = JET_errSuccess;
    BYTE    rgb[2];
    ZeroBuffer( rgb );

    //  Basic compression code ...
    //
{
    CompEndianLowSpLos16b       ce_usToComp( 0x24 );

    OSTestCheck( ce_usToComp.Us() == 0x24 );
    OSTestCheck( ce_usToComp.Cb() == 1 );

    ce_usToComp.CopyTo( rgb, sizeof(rgb) );
}

    //  Basic uncompression code ...
    //
{
    // Note: utilizing previously compressed buffer
    CompEndianLowSpLos16b       ce_usToUnComp( rgb, sizeof(rgb) );

    OSTestCheck( ce_usToUnComp.Us() == 0x24 );
    OSTestCheck( ce_usToUnComp.Cb() == 1 );
}

    //  Compression + UnCompression with a larger value

{
    CompEndianLowSpLos16b       ce_usToCompLarge( 0x346F );
    OSTestCheck( ce_usToCompLarge.Us() == 0x346F );

    OSTestCheck( ce_usToCompLarge.Cb() == 2 );
    ce_usToCompLarge.CopyTo( rgb, sizeof(rgb) );
}
{
    CompEndianLowSpLos16b       ce_usToUnCompLarge( rgb, sizeof(rgb) );
    OSTestCheck( ce_usToUnCompLarge.Us() == 0x346F );
    OSTestCheck( ce_usToUnCompLarge.Cb() == 2 );
}

HandleError:

    return err;
}

CUnitTest( TypesCompEndianLowSpLos16bAllValuesBasic, 0, "Runs the range of possible CompEndianLowSpLos16b values through compression and decompression." );
ERR TypesCompEndianLowSpLos16bAllValuesBasic::ErrTest()
{
    JET_ERR err = JET_errSuccess;
    BYTE    rgb[2];

    for( ULONG ul = 0; ul <= CompEndianLowSpLos16b::usMac; ul++ )
    {
        ZeroBuffer( rgb );
        CompEndianLowSpLos16b ce_usOrig( (USHORT)ul );
        OSTestCheck( (ULONG)(ce_usOrig.Us()) == ul );
        OSTestCheck( ce_usOrig.Cb() == 1 || ce_usOrig.Cb() == 2 );
        RandomizeBuffer( rgb, sizeof(rgb) );
        ce_usOrig.CopyTo( rgb, sizeof(rgb) );
    {
        CompEndianLowSpLos16b       ce_usComp( rgb, ce_usOrig.Cb() );
        OSTestCheck( (ULONG)(ce_usComp.Us()) == ul );
        OSTestCheck( ce_usComp.Cb() == ce_usOrig.Cb() );
    }
    }

HandleError:

    return err;
}

/* Expensive, ran once.
//      Runs in 92890 ms (for retail)!
CUnitTest( TypesCompEndianLowSpLos16bDoubledPackingChecks, 0, "Tests that all combinations of 2 USHORTs will pack into 4 bytes, and unpack again." );
ERR TypesCompEndianLowSpLos16bDoubledPackingChecks::ErrTest()
{
    JET_ERR err = JET_errSuccess;
    BYTE    rgb[4];

    for( ULONG ulFirst = 0; ulFirst <= CompEndianLowSpLos16b::usMac; ulFirst++ )
    {
        for( ULONG ulSecond = 0; ulSecond <= CompEndianLowSpLos16b::usMac; ulSecond++ )
        {
            CompEndianLowSpLos16b ce_usFirstOrig( (USHORT)ulFirst );
            CompEndianLowSpLos16b ce_usSecondOrig( (USHORT)ulSecond );

            RandomizeBuffer( rgb, sizeof(rgb) );

            //  compress first
            OSTestCheck( (ULONG)(ce_usFirstOrig.Us()) == ulFirst );
            OSTestCheck( ce_usFirstOrig.Cb() == 1 || ce_usFirstOrig.Cb() == 2 );
            ce_usFirstOrig.CopyTo( rgb, sizeof(rgb) );
            //  compress second
            OSTestCheck( (ULONG)(ce_usSecondOrig.Us()) == ulSecond );
            OSTestCheck( ce_usSecondOrig.Cb() == 1 || ce_usSecondOrig.Cb() == 2 );
            ce_usSecondOrig.CopyTo( rgb+ce_usFirstOrig.Cb(), sizeof(rgb)-ce_usFirstOrig.Cb() );

        {
            //  uncompress first
            CompEndianLowSpLos16b       ce_usFirstComp( rgb, sizeof(rgb) );
            //  uncompress second
            CompEndianLowSpLos16b       ce_usSecondComp( rgb + ce_usFirstOrig.Cb(), sizeof(rgb)-ce_usFirstOrig.Cb() );
            //  check results
            OSTestCheck( (ULONG)(ce_usFirstComp.Us()) == ulFirst );
            OSTestCheck( ce_usFirstComp.Cb() == ce_usFirstOrig.Cb() );
            OSTestCheck( (ULONG)(ce_usSecondComp.Us()) == ulSecond );
            OSTestCheck( ce_usSecondComp.Cb() == ce_usSecondOrig.Cb() );
        }
        }
    }

HandleError:

    return err;
}

CUnitTest( FOR_TIMING, 0, "Tests how long it takes to accumulate ~4B ULONGs.  Just to get an idea of timing." );
ERR FOR_TIMING::ErrTest()
{
    JET_ERR err = JET_errSuccess;
    BYTE    rgb[4];

    ULONG cbAccum = 0;
    for( ULONG ulFirst = 0; ulFirst <= 0xfffffffe; ulFirst++ )
    {
        OSTestCheck( ulFirst == ulFirst || ulFirst < 0xfffffffF );
        cbAccum += ulFirst;
    }

    wprintf(L"cbAccum %d ", cbAccum );

HandleError:

    return err;
}

*/



