// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_MATH_HXX_INCLUDED
#define _OS_MATH_HXX_INCLUDED

#include <intrin.h>


#define roundup( val, align ) ( ( ( ( val ) + ( align ) - 1 ) / ( align ) ) * ( align ) )
#define rounddn( val, align ) ( ( ( val ) / ( align ) ) * ( align ) )


#define roundupdiv( num, den ) ( ( ( num ) + ( den ) - 1 ) / ( den ) )


template<class T>
inline bool FPowerOf2( T x )
{
    return ( ( 0 < x ) && ( 0 == ( x & ( x - 1 ) ) ) );
}

#if defined ( _M_IX86 ) || defined( _M_ARM )
inline ULONG Log2( ULONG x )
{
    ULONG log2;
    BYTE ret = _BitScanReverse( &log2, x );
    return ret > 0 ? log2 : -1;
}

#elif defined ( _M_AMD64 ) || defined( _M_ARM64 )
inline ULONG Log2( unsigned __int64 x )
{
    ULONG log2;
    BYTE ret = _BitScanReverse64( &log2, x );
    return ret > 0 ? log2 : -1;
}

#else
static_assert( false, "We don't think there is any arch case where we need non-optimized Log2()" );

template<class T>
inline ULONG Log2( T x )
{
    ULONG log2 = 0;

    for ( log2 = 0; ( T( 1 ) << log2 ) < x; log2++ );

    return log2;
}
#endif

inline USHORT UsBits( const DWORD dw )
{
    DWORD ret = dw;
    ret = ret - ( ( ret >> 1 ) & 0x55555555 );
    ret = ( ret & 0x33333333 ) + ( ( ret >> 2 ) & 0x33333333 );
    ret = ( ret + ( ret >> 4 ) ) & 0x0f0f0f0f;
    ret = ret + ( ret >> 8 );
    ret = ret + ( ret >> 16 );
    ret = ret & 0x3f;

    return (USHORT)ret;
}

inline LONG LNextPowerOf2( LONG l )
{
    LONG i = 0;

    if ( !FPowerOf2( lMax / 2 + 1 ) || ( l > lMax / 2 + 1 ) )
    {
        return -1;
    }
    
    for ( i = 1; i < l; i += i )
    {
    }

    return i;
}


#define FBounded( ibQuestionable, ibLow, cbRange )  FBounded_( (unsigned __int64)(ibQuestionable), (unsigned __int64)(ibLow), (unsigned __int64)(cbRange) )
inline bool FBounded_( unsigned __int64 ibQuestionable, unsigned __int64 ibLow, unsigned __int64 cbRange )
{
    if ( ( ibLow + cbRange ) < ibLow )
    {
        return false;
    }
    if ( ibQuestionable >= ibLow &&
         ibQuestionable < ( ibLow + cbRange ) )
    {
        return true;
    }
    return false;
}



#define DwMBs( cb )         (DWORD)( ((__int64)cb) / (__int64)1024 / (__int64)1024 )
#define DwMBsFracKB( cb )   (DWORD)( roundup(((__int64)cb),1024) / (__int64)1024 * (__int64)1000 / (__int64)1024 % (__int64)1000 )
#define DblMBs( cb )        (double)( ( (double)(cb) ) / ( 1024.0 * 1024.0 ) )


#define DwKBs( cb )         (DWORD)( ((__int64)cb) / (__int64)1024 )
#define DwKBsFrac( cb )     (DWORD)( ((__int64)cb) % (__int64)1024 )

#define CbBuffSize( c, cbPer)   ( ((__int64)c) * ((__int64)cbPer) )

inline UINT HalfAvalancheHash( const UINT valueToHash, INT cBits )
{
    UINT output = valueToHash;
    output = ( output + 0x479ab41d ) + ( output << 8 );
    output = ( output ^ 0xe4aa10ce ) ^ ( output >> 5 );
    output = ( output + 0x9942f0a6 ) - ( output << 14 );
    output = ( output ^ 0x5aedd67d ) ^ ( output >> 3 );
    output = ( output + 0x17bea992 ) + ( output << 7 );

    return output >> ( sizeof( valueToHash ) * 8 - cBits );
}

unsigned __int64 Ui64FNVHash(   _In_reads_bytes_opt_( cb )  const void* const   pv,
                                _In_                        const INT           cb );

const INT CHISQUARED_FREQTABLE_SIZE = 256;
double ChiSquaredSignificanceTest( _In_reads_bytes_( cbSample ) const BYTE* const pbSample, const INT cbSample, _Out_writes_bytes_( sizeof( WORD ) * CHISQUARED_FREQTABLE_SIZE ) WORD* rgwFreqTable );

#endif

