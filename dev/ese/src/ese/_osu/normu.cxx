// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"


// Line for line copy of _strupr() behavior from base\crts\crtw32\string\strupr.c : _strupr() when
// the locale is "C locale".  This behavior hasn't changes in at least 6 years, perhaps 20 or 30 even.
// minor change made to SAL annotate function.
char * __cdecl _crt_clocale_strupr(
    __inout_z PSTR string
    )
{
    char *cp;       /* traverses string for C locale conversion */

    for ( cp = string ; *cp ; ++cp )
        if ( ('a' <= *cp) && (*cp <= 'z') )
            *cp -= 'a' - 'A';

    return(string);
}   /* C locale */


// Line for li... (well there's only one line I guess) copy of _toupper() from public\sdk\crts\ctype.h 
// in win2k3.  This is important to maintain compatibility, because _toupper() has become locale 
// dependant in Vista+, and we want the old CRT behavior.  Further we don't even want the __ascii_toupper()
// behavior, see comments near usage of _crt_clocale_toupper_broken().
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

        //To prevent normalizing text strings with embedded null-terminators
        Assert( cbText >= (LONG_PTR)strlen( pbNorm ) );
        cbText = (INT)strlen( pbNorm );

        _crt_clocale_strupr( pbNorm );

        *pcbNorm    = cbText + 1;
        CallS( err );
    }
    else if ( cbKeyBufLeft > 0 )
    {
        INT cbTextToNormalise   = cbKeyBufLeft - 1;     //  make room for null-terminator

        memcpy( pbNorm, pbText, cbTextToNormalise );
        pbNorm[ cbTextToNormalise ] = 0;

        //To prevent normalizing text strings with embedded null-terminators
        Assert( cbTextToNormalise >= (LONG_PTR)strlen( pbNorm ) );
        cbTextToNormalise = (INT)strlen( pbNorm );

        _crt_clocale_strupr( pbNorm );

        Assert( cbKeyBufLeft >= cbTextToNormalise + 1 );
        if ( cbKeyBufLeft == ( cbTextToNormalise + 1 )
            && ( 0 != pbText[cbTextToNormalise] ) )
        {
            // no null-terminators are embedded inside the text string
            // and cbText >= cbKeyBufLeft
            //
            // IMPORTANT: This function as of win2k3 assumed that _strupr() was equivalent
            // to calling _toupper() on each character individually.  This is actually not 
            // true, as you can see from the _crt_clocale* versions of _strupr()  and 
            // _toupper() above, _toupper() doesn't first check it is a valid lower case 
            // character before subtracting 0x20!  Ergo, the last character here is 
            // normalized slightly differently, in that it has 0x20 subtracted no matter 
            // what, instead of subtracted only if it is a lower case character.
            pbNorm[ cbTextToNormalise ] = (CHAR)_crt_clocale_toupper_broken( pbText[ cbTextToNormalise ] );
            err = ErrERRCheck( wrnFLDKeyTooBig );
        }
        else
        {
            //  embedded null-terminators caused us to halt normalisation
            //  before the end of the keyspace, so we still have
            //  keyspace left
            CallS( err );
        }

        *pcbNorm    = cbTextToNormalise + 1;
    }
    else
    {
        //  UNDONE: how come we don't return a warning when there
        //  is no keyspace left?
        *pcbNorm    = 0;
        CallS( err );
    }

    CallSx( err, wrnFLDKeyTooBig );
    return err;
}


//  terminate norm subsystem

void OSUNormTerm()
{
    //  nop
}

//  init norm subsystem

ERR ErrOSUNormInit()
{
    //  nop

    return JET_errSuccess;
}


