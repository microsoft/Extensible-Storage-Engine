// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

#include <tchar.h>
#include "os.hxx"



CUnitTest( TypesCompEndianLowSpLos16bBasic, 0, "Very rudimentary test for CompEndianLowSpLos16b compressing USHORT." );
ERR TypesCompEndianLowSpLos16bBasic::ErrTest()
{
    JET_ERR err = JET_errSuccess;
    BYTE    rgb[2];
    ZeroBuffer( rgb );

{
    CompEndianLowSpLos16b       ce_usToComp( 0x24 );

    OSTestCheck( ce_usToComp.Us() == 0x24 );
    OSTestCheck( ce_usToComp.Cb() == 1 );

    ce_usToComp.CopyTo( rgb, sizeof(rgb) );
}

{
    CompEndianLowSpLos16b       ce_usToUnComp( rgb, sizeof(rgb) );

    OSTestCheck( ce_usToUnComp.Us() == 0x24 );
    OSTestCheck( ce_usToUnComp.Cb() == 1 );
}


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





