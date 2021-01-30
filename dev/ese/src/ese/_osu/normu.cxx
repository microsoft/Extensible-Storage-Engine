// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"


char * __cdecl _crt_clocale_strupr(
    __inout_z PSTR string
    )
{
    char *cp;       

    for ( cp = string ; *cp ; ++cp )
        if ( ('a' <= *cp) && (*cp <= 'z') )
            *cp -= 'a' - 'A';

    return(string);
}   


#define _crt_clocale_toupper_broken(_c)    ( (_c)-'a'+'A' )


ERR ErrUtilNormText(
    __in_bcount(cbText+1) PCSTR pbText,
    INT             cbText,
    INT             cbKeyBufLeft,
    __out_bcount_part(cbKeyBufLeft, *pcbNorm) char          *pbNorm,
    INT             *pcbNorm )
{
    ERR             err         = JET_errSuccess;

    Assert( cbText >= 0 );
    Assert( cbKeyBufLeft >= 0 );

    if ( cbKeyBufLeft > cbText )
    {
        memcpy( pbNorm, pbText, cbText );
        pbNorm[ cbText ] = 0;

        Assert( cbText >= (LONG_PTR)strlen( pbNorm ) );
        cbText = (INT)strlen( pbNorm );

        _crt_clocale_strupr( pbNorm );

        *pcbNorm    = cbText + 1;
        CallS( err );
    }
    else if ( cbKeyBufLeft > 0 )
    {
        INT cbTextToNormalise   = cbKeyBufLeft - 1;

        memcpy( pbNorm, pbText, cbTextToNormalise );
        pbNorm[ cbTextToNormalise ] = 0;

        Assert( cbTextToNormalise >= (LONG_PTR)strlen( pbNorm ) );
        cbTextToNormalise = (INT)strlen( pbNorm );

        _crt_clocale_strupr( pbNorm );

        Assert( cbKeyBufLeft >= cbTextToNormalise + 1 );
        if ( cbKeyBufLeft == ( cbTextToNormalise + 1 )
            && ( 0 != pbText[cbTextToNormalise] ) )
        {
            pbNorm[ cbTextToNormalise ] = (CHAR)_crt_clocale_toupper_broken( pbText[ cbTextToNormalise ] );
            err = ErrERRCheck( wrnFLDKeyTooBig );
        }
        else
        {
            CallS( err );
        }

        *pcbNorm    = cbTextToNormalise + 1;
    }
    else
    {
        *pcbNorm    = 0;
        CallS( err );
    }

    CallSx( err, wrnFLDKeyTooBig );
    return err;
}



void OSUNormTerm()
{
}


ERR ErrOSUNormInit()
{

    return JET_errSuccess;
}


