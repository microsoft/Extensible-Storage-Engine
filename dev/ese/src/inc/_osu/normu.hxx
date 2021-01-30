// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSU_NORM_HXX_INCLUDED
#define _OSU_NORM_HXX_INCLUDED



#define BINARY_NAMES

#ifndef ESENT
#define __ascii_toupper(c)      ( (((c) >= 'a') && ((c) <= 'z')) ? ((c) - 'a' + 'A') : (c) )
#endif

INLINE CHAR ChUtilNormChar( const CHAR ch )
{
    return (CHAR)__ascii_toupper( ch );
}

ERR ErrUtilNormText(
    __in_bcount(cbText+1) PCSTR pbText,
    INT             cbText,
    INT             cbKeyBufLeft,
    __out_bcount_part(cbKeyBufLeft, *pcbNorm) char          *pbNorm,
    INT             *pcbNorm );

INLINE INT UtilCmpName( const char *sz1, const char *sz2 )
{
    return _stricmp( sz1, sz2 );
}
INLINE INT UtilCmpFileName( const WCHAR *sz1, const WCHAR *sz2 )
{
    return _wcsicmp( sz1, sz2 );
}
    
typedef USHORT STRHASH;

INLINE STRHASH StrHashValue( const char *sz )
{
    STRHASH     uAcc            = 0;
    BYTE        bXORReg         = 0;
    const char* szBegin;
    char        rgchNorm[ 64 ];
    INT         cchNorm;
    
    (void)ErrUtilNormText( sz, (ULONG)strlen( sz ), (ULONG)sizeof( rgchNorm ), rgchNorm, &cchNorm );

    szBegin = sz = rgchNorm;
    
    for ( ; cchNorm-- > 0; )
    {
        uAcc = (STRHASH)( uAcc + ( bXORReg ^= *sz++ ) );
    }
    
    uAcc |= STRHASH( ( sz - szBegin )<<14 );

    return uAcc;
}



ERR ErrOSUNormInit();


void OSUNormTerm();

#endif


