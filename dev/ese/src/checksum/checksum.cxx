// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.



#include "checksumstd.hxx"

#ifdef _IA64_

extern "C" {
    void __lfetch( INT Level,void const *Address );
    #pragma intrinsic( __lfetch )
}

#define MD_LFHINT_NONE  0x00
#define MD_LFHINT_NT1   0x01
#define MD_LFHINT_NT2   0x02
#define MD_LFHINT_NTA   0x03

#endif


typedef ULONG( *PFNCHECKSUMOLDFORMAT )( const unsigned char * const, const ULONG );

ULONG ChecksumSelectOldFormat( const unsigned char * const pb, const ULONG cb );
ULONG ChecksumOldFormatSlowly( const unsigned char * const pb, const ULONG cb );
ULONG ChecksumOldFormat64Bit( const unsigned char * const pb, const ULONG cb );


ULONG ChecksumOldFormatSSE( const unsigned char * const pb, const ULONG cb );
ULONG ChecksumOldFormatSSE2( const unsigned char * const pb, const ULONG cb );

static PFNCHECKSUMOLDFORMAT pfnChecksumOldFormat = ChecksumSelectOldFormat;

ULONG ChecksumOldFormat( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormat;
    Unused( pfn );

    return pfnChecksumOldFormat( pb, cb );
}


typedef XECHECKSUM( *PFNCHECKSUMNEWFORMAT )( const unsigned char * const, const ULONG, const ULONG, BOOL );

XECHECKSUM ChecksumSelectNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );


enum ChecksumParityMaskFunc
{
    ParityMaskFuncDefault = 0,
    ParityMaskFuncPopcnt,
};

XECHECKSUM ChecksumNewFormatSlowly( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );
XECHECKSUM ChecksumNewFormat64Bit( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );
XECHECKSUM ChecksumNewFormatSSE( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );
template <ChecksumParityMaskFunc TParityMaskFunc> XECHECKSUM ChecksumNewFormatSSE2( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue);
XECHECKSUM ChecksumNewFormatAVX( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );

PFNCHECKSUMNEWFORMAT pfnChecksumNewFormat = ChecksumSelectNewFormat;

XECHECKSUM ChecksumNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormat;
    Unused( pfn );

    return pfnChecksumNewFormat( pb, cb, pgno, fHeaderBlock );
}


ULONG ChecksumSelectOldFormat( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumSelectOldFormat;

#if defined _X86_ && defined _CHPE_X86_ARM64_
    pfn = ChecksumOldFormatSlowly;
#else
    if( FSSEInstructionsAvailable() )
    {
        if( FSSE2InstructionsAvailable() )
        {
            pfn = ChecksumOldFormatSSE2;
        }
        else
        {
            pfn = ChecksumOldFormatSSE;
        }
    }
    else if( sizeof( void * ) == sizeof( ULONG ) * 2 )
    {
        pfn = ChecksumOldFormat64Bit;
    }
    else
    {
        pfn = ChecksumOldFormatSlowly;
    }
#endif

    pfnChecksumOldFormat = pfn;

    return (*pfn)( pb, cb );
}


ULONG ChecksumOldFormatSlowly( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormatSlowly;

    Unused( pfn );

    const ULONG * pdw   = (ULONG *)pb;
    const INT cDWords           = 8;
    const INT cbStep            = cDWords * sizeof( ULONG );
    __int64 cbT                 = cb;
    Assert( 0 == ( cbT % cbStep ) );


    ULONG   dwChecksum = 0x89abcdef ^ pdw[0];

    while ( ( cbT -= cbStep ) >= 0 )
    {
        dwChecksum  ^= pdw[0]
                    ^ pdw[1]
                    ^ pdw[2]
                    ^ pdw[3]
                    ^ pdw[4]
                    ^ pdw[5]
                    ^ pdw[6]
                    ^ pdw[7];
        pdw += cDWords;
    }

    return dwChecksum;
}


ULONG ChecksumOldFormat64Bit( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormat64Bit;

    Unused( pfn );

    const unsigned __int64* pqw = (unsigned __int64 *)pb;
    unsigned __int64 qwChecksum = 0;
    const INT cQWords           = 4;
    const INT cbStep            = cQWords * sizeof( unsigned __int64 );
    __int64 cbT                 = cb;
    Assert( 0 == ( cbT % cbStep ) );


    qwChecksum ^= pqw[0] & 0x00000000FFFFFFFF;

    while ( ( cbT -= cbStep ) >= 0 )
    {
#ifdef _IA64_
        __lfetch( MD_LFHINT_NTA, (unsigned char *)(pqw + 4) );
#endif
        qwChecksum ^= pqw[0];
        qwChecksum ^= pqw[1];
        qwChecksum ^= pqw[2];
        qwChecksum ^= pqw[3];
        pqw += cQWords;
    }
    
    const unsigned __int64 qwUpper = ( qwChecksum >> ( sizeof( ULONG ) * 8 ) );
    const unsigned __int64 qwLower = qwChecksum & 0x00000000FFFFFFFF;
    qwChecksum = qwUpper ^ qwLower;

    const ULONG ulChecksum = static_cast<ULONG>( qwChecksum ) ^ 0x89abcdef;
    return ulChecksum;
}


XECHECKSUM ChecksumSelectNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumSelectNewFormat;

#if defined _X86_ && defined _CHPE_X86_ARM64_
    pfn = ChecksumNewFormatSlowly;
#else
    if( FAVXEnabled() && FPopcntAvailable() )
    {
        pfn = ChecksumNewFormatAVX;
    }
    else if( FSSEInstructionsAvailable() )
    {
        if( FSSE2InstructionsAvailable() )
        {
            if( FPopcntAvailable() )
            {
                pfn = ChecksumNewFormatSSE2<ParityMaskFuncPopcnt>;
            }
            else
            {
                pfn = ChecksumNewFormatSSE2<ParityMaskFuncDefault>;
            }
        }
        else
        {
            pfn = ChecksumNewFormatSSE;
        }
    }
    else if( sizeof( DWORD_PTR ) == sizeof( ULONG ) * 2 )
    {
        pfn = ChecksumNewFormat64Bit;
    }
    else
    {
        pfn = ChecksumNewFormatSlowly;
    }
#endif

    pfnChecksumNewFormat = pfn;

    return (*pfn)( pb, cb, pgno, fHeaderBlock );
}


ULONG DwECCChecksumFromXEChecksum( const XECHECKSUM checksum )
{
    return (ULONG)( checksum >> 32 );
}


ULONG DwXORChecksumFromXEChecksum( const XECHECKSUM checksum )
{
    return (ULONG)( checksum & 0xffffffff );
}


INT CbitSet( const ULONG dw )
{
    INT cbit = 0;
    for( INT ibit = 0; ibit < 32; ++ibit )
    {
        if( dw & ( 1 << ibit ) )
        {
            ++cbit;
        }
    }
    return cbit;
}


BOOL FECCErrorIsCorrectable( const UINT cb, const XECHECKSUM xeChecksumExpected, const XECHECKSUM xeChecksumActual )
{
    Assert( xeChecksumActual != xeChecksumExpected );

    const DWORD dwECCChecksumExpected = DwECCChecksumFromXEChecksum( xeChecksumExpected );
    const DWORD dwECCChecksumActual = DwECCChecksumFromXEChecksum( xeChecksumActual );

    if ( FECCErrorIsCorrectable( cb, dwECCChecksumExpected, dwECCChecksumActual ) )
    {
        const ULONG dwXor = DwXORChecksumFromXEChecksum( xeChecksumActual ) ^ DwXORChecksumFromXEChecksum( xeChecksumExpected );

        if ( 1 == CbitSet( dwXor ) )
        {
            return fTrue;
        }
    }

    return fFalse;
}


UINT IbitCorrupted( const UINT cb, const XECHECKSUM xeChecksumExpected, const XECHECKSUM xeChecksumActual )
{
    Assert( xeChecksumExpected != xeChecksumActual );
    Assert( FECCErrorIsCorrectable( cb, xeChecksumExpected, xeChecksumActual ) );

    const DWORD dwECCChecksumExpected = DwECCChecksumFromXEChecksum( xeChecksumExpected );
    const DWORD dwECCChecksumActual = DwECCChecksumFromXEChecksum( xeChecksumActual );
    Assert( dwECCChecksumExpected != dwECCChecksumActual );

    return IbitCorrupted( cb, dwECCChecksumExpected, dwECCChecksumActual );
}

BOOL FECCErrorIsCorrectable( const UINT cb, const ULONG dwECCChecksumExpected, const ULONG dwECCChecksumActual )
{
    const ULONG dwEcc = dwECCChecksumActual ^ dwECCChecksumExpected;

    const ULONG ulMask = ( ( cb << 3 ) - 1 );
    const ULONG ulX = ( ( dwEcc >> 16 ) ^ dwEcc ) & ulMask;


    return ulMask == ulX;
}

UINT IbitCorrupted( const UINT cb, const ULONG dwECCChecksumExpected, const ULONG dwECCChecksumActual )
{
    const ULONG dwEcc = dwECCChecksumActual ^ dwECCChecksumExpected;

    if ( dwEcc == 0 )
    {
        return ulMax;
    }

    const UINT ibitCorrupted    = (UINT)( dwEcc & 0xffff );
    const UINT ibCorrupted      = ibitCorrupted / 8;

    if ( ibCorrupted >= cb )
    {
        return ulMax;
    }

    return ibitCorrupted;
}
