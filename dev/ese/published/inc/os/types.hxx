// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_TYPES_HXX_INCLUDED
#define _OS_TYPES_HXX_INCLUDED

#include <guiddef.h>

//  build options

//  Compiler defs

#define INLINE      inline
#define NOINLINE    __declspec(noinline)

#define PUBLIC      extern
#define LOCAL_BROKEN
#define LOCAL     static

///////////////////////////////////
//
//  Platform dependent defs
//


//  use x86 assembly for endian conversion

#define TYPES_USE_X86_ASM


//  select the native word size for the checksumming code in LOG::UlChecksumBytes

#if defined(_M_AMD64) || defined(_M_IA64) || defined(_M_ARM64)
#define NATIVE_WORD QWORD
#else
#define NATIVE_WORD DWORD
#endif


//  Path Delimiter

#define chPathDelimiter   '\\'
#define wchPathDelimiter L'\\'
#define wszPathDelimiter L"\\"


//  host endian-ness

inline constexpr BOOL FHostIsLittleEndian();

//
//
///////////////////////////////////

//  types

inline BOOL FOS64Bit()
{
    return ( sizeof(VOID *) == sizeof(QWORD) );
}

class QWORDX
{
private:
    union {
        QWORD   m_qw;
        struct
        {
            DWORD m_l;
            DWORD m_h;
        };
    };
public:
    QWORDX() {};
    ~QWORDX() {};
    void SetQw( QWORD qw )      { m_qw = qw; }
    void SetDwLow( DWORD dw )   { FHostIsLittleEndian() ? m_l = dw : m_h = dw; }
    void SetDwHigh( DWORD dw )  { FHostIsLittleEndian() ? m_h = dw : m_l = dw; }
    QWORD Qw() const            { return m_qw; }
    QWORD *Pqw()                { return &m_qw; }
    DWORD DwLow() const         { return FHostIsLittleEndian() ? m_l : m_h; }
    DWORD DwHigh() const        { return FHostIsLittleEndian() ? m_h : m_l; }
    DWORD *PdwLow()             { return FHostIsLittleEndian() ? &m_l : &m_h; }
    DWORD *PdwHigh()            { return FHostIsLittleEndian() ? &m_h : &m_l; }

    //  set qword, setting low dword first, then high dword
    //  (allows unserialised reading of this qword via
    //  QwHighLow() as long as it can be guaranteed that
    //  writing to it is serialised)
    VOID SetQwLowHigh( const QWORD qw )
    {
        if ( FOS64Bit() )
        {
            SetQw( qw );
        }
        else
        {
            QWORDX  qwT;
            qwT.SetQw( qw );
            SetDwLow( qwT.DwLow() );
            SetDwHigh( qwT.DwHigh() );
        }
    }

    //  retrieve qword, retrieving high qword first, then low dword
    //  (allows unserialised retrieving of this qword as long as
    //  it can be guaranteed that writing to it is serialised and
    //  performed using SetQwLowHigh())
    QWORD QwHighLow() const
    {
        if ( FOS64Bit() )
        {
            return Qw();
        }
        else
        {
            QWORDX  qwT;
            qwT.SetDwHigh( DwHigh() );
            qwT.SetDwLow( DwLow() );
            return qwT.Qw();
        }
    }
};

#define VOID void

typedef WORD LANGID;
typedef DWORD LCID;
typedef GUID SORTID;
#define SORTIDNil { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } }

typedef wchar_t WCHAR;

typedef struct
{
    INT month;
    INT day;
    INT year;
    INT hour;
    INT minute;
    INT second;
    INT millisecond;
} DATETIME;

typedef INT_PTR (*PFN)();

typedef DWORD_PTR LIBRARY;

typedef DWORD (*PUTIL_THREAD_PROC)( DWORD_PTR );
typedef DWORD_PTR THREAD;


//  binary operator translation templates

template< class T >
inline T& operator++( T& t )
{
    t = t + 1;
    return t;
}

template< class T >
inline T operator++( T& t, const INT not_used )
{
    T tOld = t;
    t = t + 1;
    return tOld;
}

template< class T >
inline T& operator--( T& t )
{
    t = t - 1;
    return t;
}

template< class T >
inline T operator--( T& t, const INT not_used )
{
    T tOld = t;
    t = t - 1;
    return tOld;
}

template< class T1, class T2 >
inline T1& operator%=( T1& t1, const T2& t2 )
{
    t1 = t1 % t2;
    return t1;
}

template< class T1, class T2 >
inline T1& operator&=( T1& t1, const T2& t2 )
{
    t1 = t1 & t2;
    return t1;
}

template< class T1, class T2 >
inline T1& operator*=( T1& t1, const T2& t2 )
{
    t1 = t1 * t2;
    return t1;
}

template< class T1, class T2 >
inline T1& operator+=( T1& t1, const T2& t2 )
{
    t1 = t1 + t2;
    return t1;
}

template< class T1, class T2 >
inline T1& operator-=( T1& t1, const T2& t2 )
{
    t1 = t1 - t2;
    return t1;
}

template< class T1, class T2 >
inline T1& operator/=( T1& t1, const T2& t2 )
{
    t1 = t1 / t2;
    return t1;
}

template< class T1, class T2 >
inline T1& operator<<=( T1& t1, const T2& t2 )
{
    t1 = t1 << t2;
    return t1;
}

template< class T1, class T2 >
inline T1& operator>>=( T1& t1, const T2& t2 )
{
    t1 = t1 >> t2;
    return t1;
}

template< class T1, class T2 >
inline T1& operator^=( T1& t1, const T2& t2 )
{
    t1 = t1 ^ t2;
    return t1;
}

template< class T1, class T2 >
inline T1& operator|=( T1& t1, const T2& t2 )
{
    t1 = t1 | t2;
    return t1;
}

//  host endian-ness

const BOOL  fHostIsLittleEndian     = fTrue;
inline constexpr BOOL FHostIsLittleEndian()
{
    return fHostIsLittleEndian;
}

//  byte swap functions

inline unsigned __int16 ReverseTwoBytes( const unsigned __int16 w )
{
    return ( unsigned __int16 )( ( ( w & 0xFF00 ) >> 8 ) | ( ( w & 0x00FF ) << 8 ) );
}

#if defined( _M_IX86 ) && defined( TYPES_USE_X86_ASM )

#pragma warning( disable:  4035 )

inline unsigned __int32 ReverseFourBytes( const unsigned __int32 dw )
{
    __asm mov   eax, dw
    __asm bswap eax
}

inline unsigned __int64 ReverseEightBytes( const unsigned __int64 qw )
{
    __asm mov   edx, DWORD PTR qw
    __asm mov   eax, DWORD PTR qw[4]
    __asm bswap edx
    __asm bswap eax
}

#pragma warning( default:  4035 )

#else  //  !_M_IX86 || !TYPES_USE_X86_ASM

inline DWORD ReverseFourBytes( const unsigned __int32 dw )
{
    return _lrotl(  ( ( dw & 0xFF00FF00 ) >> 8 ) |
                    ( ( dw & 0x00FF00FF ) << 8 ), 16);
}

inline QWORD ReverseEightBytes( const unsigned __int64 qw )
{
    const unsigned __int64 qw2 =    ( ( qw & 0xFF00FF00FF00FF00 ) >> 8 ) |
                                    ( ( qw & 0x00FF00FF00FF00FF ) << 8 );
    const unsigned __int64 qw3 =    ( ( qw2 & 0xFFFF0000FFFF0000 ) >> 16 ) |
                                    ( ( qw2 & 0x0000FFFF0000FFFF ) << 16 );
    return  ( ( qw3 & 0xFFFFFFFF00000000 ) >> 32 ) |
            ( ( qw3 & 0x00000000FFFFFFFF ) << 32 );
}

#endif  //  _M_IX86 && TYPES_USE_X86_ASM

template< class T >
inline T ReverseNBytes( const T t )
{
    T tOut;

    const size_t        cb          = sizeof( T );
    const char*         pbSrc       = (char*) &t;
    const char* const   pbSrcMac    = pbSrc + cb;
    char*               pbDest      = (char*) &tOut + cb - 1;

    while ( pbSrc < pbSrcMac )
    {
        *( pbDest-- ) = *( pbSrc++ );
    }

    return tOut;
}

template< class T >
inline T ReverseBytes( const T t )
{
    return ReverseNBytes( t );
}

template<>
inline __int8 ReverseBytes< __int8 >( const __int8 b )
{
    return b;
}

template<>
inline unsigned __int8 ReverseBytes< unsigned __int8 >( const unsigned __int8 b )
{
    return b;
}

template<>
inline __int16 ReverseBytes< __int16 >( const __int16 w )
{
    return ReverseTwoBytes( (const unsigned __int16) w );
}

template<>
inline unsigned __int16 ReverseBytes< unsigned __int16 >( const unsigned __int16 w )
{
    return ReverseTwoBytes( (const unsigned __int16) w );
}

#if _MSC_FULL_VER < 13009111
template<>
inline SHORT ReverseBytes< SHORT >( const SHORT w )
{
    return ReverseTwoBytes( (const unsigned __int16) w );
}

template<>
inline USHORT ReverseBytes< USHORT >( const USHORT w )
{
    return ReverseTwoBytes( (const unsigned __int16) w );
}
#endif

template<>
inline __int32 ReverseBytes< __int32 >( const __int32 dw )
{
    return ReverseFourBytes( (const unsigned __int32) dw );
}

template<>
inline unsigned __int32 ReverseBytes< unsigned __int32 >( const unsigned __int32 dw )
{
    return ReverseFourBytes( (const unsigned __int32) dw );
}

template<>
inline LONG ReverseBytes< LONG >( const LONG dw )
{
    return ReverseFourBytes( (const unsigned __int32) dw );
}

template<>
inline ULONG ReverseBytes< ULONG >( const ULONG dw )
{
    return ReverseFourBytes( (const unsigned __int32) dw );
}

template<>
inline __int64 ReverseBytes< __int64 >( const __int64 qw )
{
    return ReverseEightBytes( (const unsigned __int64) qw );
}

template<>
inline unsigned __int64 ReverseBytes< unsigned __int64 >( const unsigned __int64 qw )
{
    return ReverseEightBytes( (const unsigned __int64) qw );
}

template< class T >
inline T ReverseBytesOnLE( const T t )
{
    return fHostIsLittleEndian ? ReverseBytes( t ) : t;
}

template< class T >
inline T ReverseBytesOnBE( const T t )
{
    return fHostIsLittleEndian ? t : ReverseBytes( t );
}


//  big endian type template

template< class T >
class BigEndian
{
    public:
        BigEndian< T >() {};
        BigEndian< T >( const BigEndian< T >& be_t );
        BigEndian< T >( const T& t );

        operator T() const;

        BigEndian< T >& operator=( const BigEndian< T >& be_t );
        BigEndian< T >& operator=( const T& t );

    private:
        T m_t;
};

template< class T >
inline BigEndian< T >::BigEndian( const BigEndian< T >& be_t )
    :   m_t( be_t.m_t )
{
}

template< class T >
inline BigEndian< T >::BigEndian( const T& t )
    :   m_t( ReverseBytesOnLE( t ) )
{
}

template< class T >
inline BigEndian< T >::operator T() const
{
    return ReverseBytesOnLE( m_t );
}

template< class T >
inline BigEndian< T >& BigEndian< T >::operator=( const BigEndian< T >& be_t )
{
    m_t = be_t.m_t;

    return *this;
}

template< class T >
inline BigEndian< T >& BigEndian< T >::operator=( const T& t )
{
    m_t = ReverseBytesOnLE( t );

    return *this;
}


//  little endian type template

template< class T >
class LittleEndian
{
    public:
        LittleEndian< T >() {};
        LittleEndian< T >( const LittleEndian< T >& le_t );
        LittleEndian< T >( const T& t );

        operator T() const;

        LittleEndian< T >& operator=( const LittleEndian< T >& le_t );
        LittleEndian< T >& operator=( const T& t );

    private:
        T m_t;
};

template< class T >
inline LittleEndian< T >::LittleEndian( const LittleEndian< T >& le_t )
    :   m_t( le_t.m_t )
{
}

template< class T >
inline LittleEndian< T >::LittleEndian( const T& t )
    :   m_t( ReverseBytesOnBE( t ) )
{
}

template< class T >
inline LittleEndian< T >::operator T() const
{
    return ReverseBytesOnBE( m_t );
}

template< class T >
inline LittleEndian< T >& LittleEndian< T >::operator=( const LittleEndian< T >& le_t )
{
    m_t = le_t.m_t;

    return *this;
}

template< class T >
inline LittleEndian< T >& LittleEndian< T >::operator=( const T& t )
{
    m_t = ReverseBytesOnBE( t );

    return *this;
}


#ifndef _M_IX86
#define PERMIT_UNALIGNED_ACCESS __unaligned
#else
#define PERMIT_UNALIGNED_ACCESS
#endif

//  unaligned type template

#define UCAST(T) *(T PERMIT_UNALIGNED_ACCESS *)

template< class T >
class Unaligned
{
    public:
        Unaligned< T >() PERMIT_UNALIGNED_ACCESS {};
        Unaligned< T >( const Unaligned< T >& u_t ) PERMIT_UNALIGNED_ACCESS;
        Unaligned< T >( const T& t ) PERMIT_UNALIGNED_ACCESS;

        operator T() PERMIT_UNALIGNED_ACCESS const;
        
        Unaligned< T >& operator=( const Unaligned< T >& u_t ) PERMIT_UNALIGNED_ACCESS;
        Unaligned< T >& operator=( const T& t ) PERMIT_UNALIGNED_ACCESS;

    private:
        T m_t;
};

template< class T >
inline Unaligned< T >::Unaligned( const Unaligned< T >& u_t ) PERMIT_UNALIGNED_ACCESS
{
    UCAST(T)&m_t = UCAST(T)&u_t.m_t;
}

template< class T >
inline Unaligned< T >::Unaligned( const T& t ) PERMIT_UNALIGNED_ACCESS
{
    UCAST(T)&m_t = t;
}

template< class T >
inline Unaligned< T >::operator T() PERMIT_UNALIGNED_ACCESS const
{
    return UCAST(T)&m_t;
}

template< class T >
inline Unaligned< T >& Unaligned< T >::operator=( const Unaligned< T >& u_t ) PERMIT_UNALIGNED_ACCESS
{
    UCAST(T)&m_t = UCAST(T)&u_t.m_t;

    return *this;
}

template< class T >
inline Unaligned< T >& Unaligned< T >::operator=( const T& t ) PERMIT_UNALIGNED_ACCESS
{
    UCAST(T)&m_t = t;

    return *this;
}


//  unaligned big endian type template

template< class T >
class UnalignedBigEndian
{
    public:
        UnalignedBigEndian< T >() PERMIT_UNALIGNED_ACCESS {};
        UnalignedBigEndian< T >( const UnalignedBigEndian< T >& ube_t ) PERMIT_UNALIGNED_ACCESS;
        UnalignedBigEndian< T >( const T& t ) PERMIT_UNALIGNED_ACCESS;

        operator T() PERMIT_UNALIGNED_ACCESS const;

        UnalignedBigEndian< T >& operator=( const UnalignedBigEndian< T >& ube_t ) PERMIT_UNALIGNED_ACCESS;
        UnalignedBigEndian< T >& operator=( const T& t ) PERMIT_UNALIGNED_ACCESS;

    private:
        T m_t;
};

template< class T >
inline UnalignedBigEndian< T >::UnalignedBigEndian( const UnalignedBigEndian< T >& ube_t ) PERMIT_UNALIGNED_ACCESS
{
    UCAST(T)&m_t = UCAST(T)&ube_t.m_t;
}

template< class T >
inline UnalignedBigEndian< T >::UnalignedBigEndian( const T& t ) PERMIT_UNALIGNED_ACCESS
{
    UCAST(T)&m_t = ReverseBytesOnLE( t );
}

template< class T >
inline UnalignedBigEndian< T >::operator T() PERMIT_UNALIGNED_ACCESS const
{
    T t = UCAST(T)&m_t;
    return ReverseBytesOnLE( t );
}

template< class T >
inline UnalignedBigEndian< T >& UnalignedBigEndian< T >::operator=( const UnalignedBigEndian< T >& ube_t ) PERMIT_UNALIGNED_ACCESS
{
    UCAST(T)&m_t = UCAST(T)&ube_t.m_t;

    return *this;
}

template< class T >
inline UnalignedBigEndian< T >& UnalignedBigEndian< T >::operator=( const T& t ) PERMIT_UNALIGNED_ACCESS
{
    UCAST(T)&m_t = ReverseBytesOnLE( t );

    return *this;
}


//  unaligned little endian type template

template< class T >
class UnalignedLittleEndian
{
    public:
        UnalignedLittleEndian< T >() PERMIT_UNALIGNED_ACCESS {};
        UnalignedLittleEndian< T >( const UnalignedLittleEndian< T >& ule_t ) PERMIT_UNALIGNED_ACCESS;
        UnalignedLittleEndian< T >( const T& t ) PERMIT_UNALIGNED_ACCESS;

        operator T() PERMIT_UNALIGNED_ACCESS const;

        UnalignedLittleEndian< T >& operator=( const UnalignedLittleEndian< T >& ule_t ) PERMIT_UNALIGNED_ACCESS;
        UnalignedLittleEndian< T >& operator=( const T& t ) PERMIT_UNALIGNED_ACCESS;

    private:
        T m_t;
};

template< class T >
inline UnalignedLittleEndian< T >::UnalignedLittleEndian( const UnalignedLittleEndian< T >& ule_t ) PERMIT_UNALIGNED_ACCESS
{
    UCAST(T)&m_t = UCAST(T)&ule_t.m_t;
}

template< class T >
inline UnalignedLittleEndian< T >::UnalignedLittleEndian( const T& t ) PERMIT_UNALIGNED_ACCESS
{
    UCAST(T)&m_t = ReverseBytesOnBE( t );
}

template< class T >
inline UnalignedLittleEndian< T >::operator T() PERMIT_UNALIGNED_ACCESS const
{
    T t = UCAST(T)&m_t;
    return ReverseBytesOnBE( t );
}

template< class T >
inline UnalignedLittleEndian< T >& UnalignedLittleEndian< T >::operator=( const UnalignedLittleEndian< T >& ule_t ) PERMIT_UNALIGNED_ACCESS
{
    UCAST(T)&m_t = UCAST(T)&ule_t.m_t;

    return *this;
}

template< class T >
inline UnalignedLittleEndian< T >& UnalignedLittleEndian< T >::operator=( const T& t ) PERMIT_UNALIGNED_ACCESS
{
    UCAST(T)&m_t = ReverseBytesOnBE( t );

    return *this;
}


//  special type qualifier to allow unaligned access to variables
//
//  NOTE:  NEVER DIRECTLY USE THIS TYPE QUALIFIER!  USE THE Unaligned* TEMPLATES!

#undef PERMIT_UNALIGNED_ACCESS


//
//  Compressed Endian Types
//
//  These are types like BigEndian<> and LittleEndian<> that are suitable for storage on
//  any platform, but these types take up variable space depending upon the value stored 
//  and the probability to be an expected (and thus compressable) value.
//
//  Compression Schemes
//
//      Random values can never be compressed because they have no higher probability to
//      be one value than another.  The key is to pick / create a compression scheme that 
//      can map smaller set of bytes to the most probable of values.
//
//      LowSp   - This "Low Spread" method expects values that have a high tendency to 
//                cluster towards low numerical values.
//                Good For: Code enums, flags, small cb (like cb of a single field)
//
//      Los     - This will be appended to any scheme that is slightly lossy
//                compare to the native bit size
//
//  Naming
//
//      Type Naming will be this: CompEndian<CompressionSchema>[Los]<MaxBitSize>b
//      Hungarian Naming will be: ce_us (USHORT), ce_cb (count of bytes), ce_grbit, etc
//
//  Usage
//
//      All such types will have two .ctor's:
//          CompEndianLowSpLos16b( TYPE us ) :
//          CompEndianLowSpLos16b( __in_bcount( cbBufferMax ) const BYTE * pbe_us, ULONG cbBufferMax )
//
//      And after constructing from the native type (uncompressed var) or from the 
//      buffer (compressed var) respectively, then both the value can be retrieved
//      or the CopyTo buffer routing can be called.  Also the size of the buffer
//      will be available.

//  A low spread compressed unsigned short helper class. We lose only a single bit of 16b USHORT and 
//  can represent values from 0 to 32767 (i.e. 2^15 - 1) with the ability to store values below 128 
//  in a single byte.

#ifdef DEBUG
//  Unfortunately too early for asserts, define a simple one.
#define TYPESAssertSz( expr, str )      if ( !(expr)  ) { *((ULONG*)NULL) = 9; }
#define TYPESAssert( expr )             TYPESAssertSz( (expr), #expr )
#else
#define TYPESAssertSz( expr, str )
#define TYPESAssert( expr )
#endif

class CompEndianLowSpLos16b     //  ce_?? | ce_us | ce_cb | ce_grbit | etc ...
{
    //  Format: 1 byte min, 2 bytes max
    //
    //      1<x>    cb = 2 form, x = 15 bit USHORT
    //      0<x>    cb = 1 form, x = 7 bit "USHORT"

public:
    const static USHORT     usMac   = 0x7FFF;

private:
    const static BYTE       bExpandedFlag = (BYTE)0x80;
    const static USHORT     usExpandedFlag = (USHORT)0x8000;
    const static USHORT     usMaxShortShort = (USHORT)bExpandedFlag;
    const static USHORT     mskExpandedShort = (USHORT)~usExpandedFlag;

    USHORT          m_us;

    //  this function compresses the m_us (set in CompEndianLowSpLos16b( us ) .ctor) into the pb provided, the
    //  opposite / correlated function is unobviously

    INLINE VOID CopyTo_( __out_bcount( cbBufferMax ) BYTE * const pb, __in const ULONG cbBufferMax )
    {
        TYPESAssert( Cb() <= cbBufferMax );
        if ( Cb() == 1 )
        {
            TYPESAssert( m_us < usMaxShortShort );
            pb[0] = (BYTE)m_us;
        }
        else
        {
            *((UnalignedBigEndian<USHORT>*)pb) = m_us | usExpandedFlag;
        }
    }

    INLINE USHORT CopyOut_( __in_bcount( cbBufferMax ) const BYTE * pbe_us, ULONG cbBufferMax )
    {
        USHORT us;
        if ( cbBufferMax == 0 )     // fuzz safety first
        {
            TYPESAssertSz( fFalse, "Tried to unpack more data when we have 0 bytes in buffer!" );
            us = 0;
        }
        else
        {
            TYPESAssert( cbBufferMax >= 1 );
            if ( pbe_us[0] & bExpandedFlag )
            {
                if ( cbBufferMax < 2 )  // fuzz safety first
                {
                    TYPESAssertSz( fFalse, "Tried to unpack more data than we have in buffer!" );
                    us = 0;
                }
                else
                {
                    us = *((UnalignedBigEndian<USHORT>*)pbe_us) & mskExpandedShort;
                    TYPESAssert( us <= mskExpandedShort );
                }
            }
            else
            {
                us = (USHORT)(pbe_us[0]);
                TYPESAssert( us < usMaxShortShort );
            }
        }
        return us;
    }

public:

    //  initializes a compressed USHORT object, expected user would copy out to buffer 
    //  of Cb() bytes with CopyTo().

    INLINE CompEndianLowSpLos16b( USHORT us ) :
        m_us( us )
    {
        C_ASSERT( usExpandedFlag == usMac + 1 );
        TYPESAssert( 0 == ( m_us & usExpandedFlag ) );  // trying to pack too big of a USHORT in here
        TYPESAssert( m_us <= usMac );
    }

    //  gets the USHORT value stored in the class

    INLINE USHORT Us() const
    {
        TYPESAssert( m_us <= usMac );
        return m_us;
    }

    //  this function uncompresses the USHORT out of the pbe_us, expected user would get
    //  the USHORT with Us().

    INLINE CompEndianLowSpLos16b( __in_bcount( cbBufferMax ) const BYTE * pbe_us, ULONG cbBufferMax )
    {
        m_us = CopyOut_( pbe_us, cbBufferMax );

        //  validate recompressibility / consistency
#ifdef DEBUG
        CompEndianLowSpLos16b   ce_usCheck( m_us );
        BYTE    rgbCheck[3];
        ce_usCheck.CopyTo_( rgbCheck, sizeof(rgbCheck) );
        TYPESAssert( ce_usCheck.Cb() == Cb() );
        TYPESAssert( ( rgbCheck[0] & bExpandedFlag ) == ( pbe_us[0] & bExpandedFlag ) );
        TYPESAssert( 0 == memcmp( rgbCheck, pbe_us, Cb() ) );   // just to be sure.
#endif
    }

    //  returns the size of the buffer required by CopyTo().

    INLINE ULONG Cb() const
    {
        //  dealing with m_us
        if ( m_us < usMaxShortShort )
        {
            return 1;
        }
        else
        {
            return 2;
        }
    }

    //  this function compresses the m_us (set in CompEndianLowSpLos16b( us ) .ctor) into the pb provided

    INLINE VOID CopyTo( BYTE * pb, __in const ULONG cbBufferMax )
    {
        CopyTo_( pb, cbBufferMax );

        //  validate exit criteria are consistent with our understanding
#ifdef DEBUG
        const BOOL fHighSet = ( Cb() != 1 );
        TYPESAssert( ( *(reinterpret_cast<BYTE*>(pb)) & bExpandedFlag ) || !fHighSet );
        // This may AV at end of buffer? Could combine w/ previous TYPESAssert, if & 0x80 is true, this should be safe ...
        TYPESAssert( ( *(reinterpret_cast<UnalignedBigEndian<USHORT>*>(pb)) & usExpandedFlag ) || !fHighSet );
        CompEndianLowSpLos16b   ce_usComp( pb, Cb() );
        TYPESAssert( m_us == ce_usComp.Us() );
#endif
    }

};


//      defines natural bit-wise operations (basic, or advanced) on an enum, preserving the enum type

//  note: copied (and renamed) from ntdef.h

#define DEFINE_ENUM_FLAG_OPERATORS_BASIC(ENUMTYPE)  \
    extern "C++" {                                  \
        inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((INT)a) | ((INT)b)); }               \
        inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((INT)a) & ((INT)b)); }               \
        inline ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((INT)a)); }                                     \
    }

#define DEFINE_ENUM_FLAG_OPERATORS_ADV(ENUMTYPE)        \
    extern "C++" {                                  \
        inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((INT)a) | ((INT)b)); }               \
        inline ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((INT &)a) |= ((INT)b)); }     \
        inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((INT)a) & ((INT)b)); }               \
        inline ENUMTYPE &operator &= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((INT &)a) &= ((INT)b)); }     \
        inline ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((INT)a)); }                                     \
        inline ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((INT)a) ^ ((INT)b)); }               \
        inline ENUMTYPE &operator ^= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((INT &)a) ^= ((INT)b)); }     \
    }


//
//      Class Init Support
//

//  Derive from this class to memset all members of your derived class to zero
//  prior to the exec of its constructor or any constructors of its members
//
//  NOTE:  this only works with one level of derivation
//
class CZeroInit
{
    public:

        CZeroInit( const size_t cbDerivedClass )
        {
            memset( this, 0, cbDerivedClass );
        }

};


#endif  //  _OS_TYPES_HXX_INCLUDED


