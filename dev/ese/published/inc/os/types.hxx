// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_TYPES_HXX_INCLUDED
#define _OS_TYPES_HXX_INCLUDED

#include <guiddef.h>



#define INLINE      inline
#define NOINLINE    __declspec(noinline)

#define PUBLIC      extern
#define LOCAL_BROKEN
#define LOCAL     static




#define TYPES_USE_X86_ASM



#if defined(_M_AMD64) || defined(_M_IA64) || defined(_M_ARM64)
#define NATIVE_WORD QWORD
#else
#define NATIVE_WORD DWORD
#endif



#define chPathDelimiter   '\\'
#define wchPathDelimiter L'\\'
#define wszPathDelimiter L"\\"



inline constexpr BOOL FHostIsLittleEndian();



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


const BOOL  fHostIsLittleEndian     = fTrue;
inline constexpr BOOL FHostIsLittleEndian()
{
    return fHostIsLittleEndian;
}


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

#else

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

#endif

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



#undef PERMIT_UNALIGNED_ACCESS




#ifdef DEBUG
#define TYPESAssertSz( expr, str )      if ( !(expr)  ) { *((ULONG*)NULL) = 9; }
#define TYPESAssert( expr )             TYPESAssertSz( (expr), #expr )
#else
#define TYPESAssertSz( expr, str )
#define TYPESAssert( expr )
#endif

class CompEndianLowSpLos16b
{

public:
    const static USHORT     usMac   = 0x7FFF;

private:
    const static BYTE       bExpandedFlag = (BYTE)0x80;
    const static USHORT     usExpandedFlag = (USHORT)0x8000;
    const static USHORT     usMaxShortShort = (USHORT)bExpandedFlag;
    const static USHORT     mskExpandedShort = (USHORT)~usExpandedFlag;

    USHORT          m_us;


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
        if ( cbBufferMax == 0 )
        {
            TYPESAssertSz( fFalse, "Tried to unpack more data when we have 0 bytes in buffer!" );
            us = 0;
        }
        else
        {
            TYPESAssert( cbBufferMax >= 1 );
            if ( pbe_us[0] & bExpandedFlag )
            {
                if ( cbBufferMax < 2 )
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


    INLINE CompEndianLowSpLos16b( USHORT us ) :
        m_us( us )
    {
        C_ASSERT( usExpandedFlag == usMac + 1 );
        TYPESAssert( 0 == ( m_us & usExpandedFlag ) );
        TYPESAssert( m_us <= usMac );
    }


    INLINE USHORT Us() const
    {
        TYPESAssert( m_us <= usMac );
        return m_us;
    }


    INLINE CompEndianLowSpLos16b( __in_bcount( cbBufferMax ) const BYTE * pbe_us, ULONG cbBufferMax )
    {
        m_us = CopyOut_( pbe_us, cbBufferMax );

#ifdef DEBUG
        CompEndianLowSpLos16b   ce_usCheck( m_us );
        BYTE    rgbCheck[3];
        ce_usCheck.CopyTo_( rgbCheck, sizeof(rgbCheck) );
        TYPESAssert( ce_usCheck.Cb() == Cb() );
        TYPESAssert( ( rgbCheck[0] & bExpandedFlag ) == ( pbe_us[0] & bExpandedFlag ) );
        TYPESAssert( 0 == memcmp( rgbCheck, pbe_us, Cb() ) );
#endif
    }


    INLINE ULONG Cb() const
    {
        if ( m_us < usMaxShortShort )
        {
            return 1;
        }
        else
        {
            return 2;
        }
    }


    INLINE VOID CopyTo( BYTE * pb, __in const ULONG cbBufferMax )
    {
        CopyTo_( pb, cbBufferMax );

#ifdef DEBUG
        const BOOL fHighSet = ( Cb() != 1 );
        TYPESAssert( ( *(reinterpret_cast<BYTE*>(pb)) & bExpandedFlag ) || !fHighSet );
        TYPESAssert( ( *(reinterpret_cast<UnalignedBigEndian<USHORT>*>(pb)) & usExpandedFlag ) || !fHighSet );
        CompEndianLowSpLos16b   ce_usComp( pb, Cb() );
        TYPESAssert( m_us == ce_usComp.Us() );
#endif
    }

};




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



class CZeroInit
{
    public:

        CZeroInit( const size_t cbDerivedClass )
        {
            memset( this, 0, cbDerivedClass );
        }

};


#endif


