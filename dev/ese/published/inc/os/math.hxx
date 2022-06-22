// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_MATH_HXX_INCLUDED
#define _OS_MATH_HXX_INCLUDED

#include <intrin.h>

//  Pseudo-Function math

#define roundup( val, align ) ( ( ( ( val ) + ( align ) - 1 ) / ( align ) ) * ( align ) )
#define rounddn( val, align ) ( ( ( val ) / ( align ) ) * ( align ) )

//  roundupdiv rounds up the int division ( num / den ) to the next int.
//  one common usage of it is to compute the number of fixed chunks required
//  to fit a given number of elements, in which case you'd do:
//      roundupdiv( cElements, cElementsPerChunk )

#define roundupdiv( num, den ) ( ( ( num ) + ( den ) - 1 ) / ( den ) )

//  Power-of-2 related helpers.

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
    return ret > 0 ? log2 : -1; // log( 0 ) is undefined, represented by -1
}

#elif defined ( _M_AMD64 ) || defined( _M_ARM64 )
inline ULONG Log2( unsigned __int64 x )
{
    ULONG log2;
    BYTE ret = _BitScanReverse64( &log2, x );
    return ret > 0 ? log2 : -1; // log( 0 ) is undefined, represented by -1
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

#pragma warning (push)
#pragma warning (disable: 4293)  //  '>>': shift count negative or too big, undefined behavior

template<class T>
inline T NextPowerOf2( _In_ const T x )
{
    C_ASSERT( sizeof( T ) <= 8 );

    T n = x;

    if ( n == 0 )
    {
        return 1;
    }

    n--;

    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;

    if ( sizeof( T ) >= 2 )
    {
        n |= n >> 8;
    }
    if ( sizeof( T ) >= 4 )
    {
        n |= n >> 16;
    }
    if ( sizeof( T ) >= 8 )
    {
        n |= n >> 32;
    }

    n++;

    //  return -1 on overflow

    return n >= x ? n : ( (T)0 - 1 );
}

#pragma warning (pop)

inline LONG LNextPowerOf2( LONG l )
{
    return NextPowerOf2( l );
}

//  Returns fTrue if ibQuestionable is in the range [ibLow, ibLow+cbRange).

#define FBounded( ibQuestionable, ibLow, cbRange )  FBounded_( (unsigned __int64)(ibQuestionable), (unsigned __int64)(ibLow), (unsigned __int64)(cbRange) )
inline bool FBounded_( unsigned __int64 ibQuestionable, unsigned __int64 ibLow, unsigned __int64 cbRange )
{
    if ( ( ibLow + cbRange ) < ibLow )
    {
        //  overflow means something is wrong
        return false;
    }
    if ( ibQuestionable >= ibLow &&
         ibQuestionable < ( ibLow + cbRange ) )
    {
        return true;
    }
    return false;
}

//  Convenient output functions

//  The DwMBs()/DwMBsFracKB() pair of functions does a good job of showing numbers with a large dynamic range, into 
//  the multi-GB range, and conveys small numbers (down to 1 KB granularity .. below which is often not of interest).
//  One can also use DblMBs() to achieve similar results, using a simpler format string.
//  Examples:
//      printf( "   MBs used:  %10d.%03d\n", DwMBs( xxx ), DwMBsFracKB( xxx ) );
//      printf( "   MBs used:  %14.3f\n", DblMBs( xxx ) );
//  yielding something like:
//
//  Tables (FCB):
//    Objects in use:               0
//    MBs committed:           72.312
//    MBs used:                12.024   (ave=320 bytes)
//  Key buffers (RESKEY):
//    Objects in use:               0
//    MBs committed:            0.062
//    MBs used:                 0.000   (ave=2016 bytes)
//

#define DwMBs( cb )         (DWORD)( ((__int64)cb) / (__int64)1024 / (__int64)1024 )
#define DwMBsFracKB( cb )   (DWORD)( roundup(((__int64)cb),1024) / (__int64)1024 * (__int64)1000 / (__int64)1024 % (__int64)1000 )
#define DblMBs( cb )        (double)( ( (double)(cb) ) / ( 1024.0 * 1024.0 ) )

//  However if you really want to know all the bytes, you can use DwKBs()/DwKBsFrac() to get down to byte level
//  granularity.

#define DwKBs( cb )         (DWORD)( ((__int64)cb) / (__int64)1024 )
#define DwKBsFrac( cb )     (DWORD)( ((__int64)cb) % (__int64)1024 )

#define CbBuffSize( c, cbPer)   ( ((__int64)c) * ((__int64)cbPer) )

// Hashes an integer into the given amount of bits using a common half avalanche hash
// Half avalanche hash is defined as: a function where every input bit affects its own position and every higher position in the output.
inline UINT HalfAvalancheHash( const UINT valueToHash, INT cBits )
{
    //Assert( cBits <= sizeof( valueToHash ) * 8 );
    UINT output = valueToHash;
    output = ( output + 0x479ab41d ) + ( output << 8 );
    output = ( output ^ 0xe4aa10ce ) ^ ( output >> 5 );
    output = ( output + 0x9942f0a6 ) - ( output << 14 );
    output = ( output ^ 0x5aedd67d ) ^ ( output >> 3 );
    output = ( output + 0x17bea992 ) + ( output << 7 );

    return output >> ( sizeof( valueToHash ) * 8 - cBits );
}

// Returns a 64-bit hashcode of an array of bytes computed by the FNV-1a hash algorithm
unsigned __int64 Ui64FNVHash(   _In_reads_bytes_opt_( cb )  const void* const   pv,
                                _In_                        const INT           cb );

// Chi-squared significance test: Sum [(E - O)^2 / E]
//   where  E = expected freq
//          O = observed freq
// Symbol size = byte
// Returns the chi-squared statistic
const INT CHISQUARED_FREQTABLE_SIZE = 256;
double ChiSquaredSignificanceTest( _In_reads_bytes_( cbSample ) const BYTE* const pbSample, const INT cbSample, _Out_writes_bytes_( sizeof( WORD ) * CHISQUARED_FREQTABLE_SIZE ) WORD* rgwFreqTable );

#endif  //  _OS_MATH_HXX_INCLUDED

