// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_TIME_HXX_INCLUDED
#define _OS_TIME_HXX_INCLUDED



#ifdef _M_IX86
#endif




ULONG_PTR UlUtilGetSeconds();




typedef ULONG TICK;


void OSTimeSetTimeInjection( const DWORD eTimeInjNegWrapMode, const TICK dtickTimeInjWrapOffset, const TICK dtickTimeInjAccelerant );


TICK TickOSTimeCurrent();


INLINE LONG TickCmp( TICK tick1, TICK tick2 )
{
    return LONG( tick1 - tick2 );
}


INLINE LONG DtickDelta( TICK tickEarlier, TICK tickLater )
{
    return LONG( tickLater - tickEarlier );
}


INLINE TICK TickMin( TICK tick1, TICK tick2 )
{
    return TickCmp( tick1, tick2 ) < 0 ? tick1 : tick2;
}


INLINE TICK TickMax( TICK tick1, TICK tick2 )
{
    return TickCmp( tick1, tick2 ) > 0 ? tick1 : tick2;
}




typedef QWORD HRT;


HRT HrtHRTFreq();


HRT HrtHRTCount();


INLINE double DblHRTElapsedTimeFromHrtStart( const HRT hrtStartCount )
{
    return  (signed __int64) ( HrtHRTCount() - hrtStartCount )
            / (double) (signed __int64) HrtHRTFreq();
}

INLINE double DblHRTSecondsElapsed( const signed __int64 dhrtElapsed )
{
    return  dhrtElapsed
            / (double) (signed __int64) HrtHRTFreq();
}

INLINE double DblHRTDelta( const HRT hrtStartCount, const HRT hrtEndCount )
{
    return  (signed __int64) ( hrtEndCount - hrtStartCount )
            / (double) (signed __int64) HrtHRTFreq();
}


INLINE QWORD CmsecHRTFromDhrt( const HRT dhrt, const HRT hrtFreq )
{
    if ( dhrt < ( ~(QWORD)0 ) / 1000 )
    {
        return ( ( 1000 * dhrt ) / hrtFreq );
    }
    else
    {
        return ( 1000 * ( dhrt / hrtFreq ) );
    }
}


INLINE HRT DhrtHRTElapsedFromHrtStart( const HRT hrtStartCount )
{
    const HRT dhrt = HrtHRTCount() - hrtStartCount;
    return ( dhrt <= (HRT)llMax ? dhrt : 0 );
}


INLINE QWORD CmsecHRTFromDhrt( const HRT dhrt )
{
    return CmsecHRTFromDhrt( dhrt, HrtHRTFreq() );
}


INLINE QWORD CmsecHRTFromHrtStart( const HRT hrtStart )
{
    return CmsecHRTFromDhrt( DhrtHRTElapsedFromHrtStart( hrtStart ), HrtHRTFreq() );
}


INLINE QWORD CusecHRTFromDhrt( const HRT dhrt, const HRT hrtFreq = HrtHRTFreq() );
INLINE QWORD CusecHRTFromDhrt( const HRT dhrt, const HRT hrtFreq )
{
    if ( dhrt < ( ~(QWORD)0 ) / ( 1000 * 1000 ) )
    {
        return ( ( 1000 * 1000 * dhrt ) / hrtFreq );
    }
    else if ( dhrt < ( ~(QWORD)0 ) / 1000 )
    {
        return ( 1000 * (( 1000 * dhrt ) / hrtFreq ) );
    }
    else
    {
        return ( 1000 * 1000 * ( dhrt / hrtFreq ) );
    }
}


INLINE HRT DhrtHRTFromCmsec( const DWORD cmsec )
{
    const HRT hrtFreq = HrtHRTFreq();
    const HRT dhrt = cmsec * hrtFreq;

    Assert( dhrt >= cmsec || hrtFreq == 0 );
    Assert( dhrt >= hrtFreq || cmsec == 0 );

    return ( dhrt / 1000 );
}




void UtilGetCurrentDateTime( DATETIME* pdt );
__int64 UtilConvertDateTimeToFileTime( DATETIME* pdt );
void UtilConvertFileTimeToDateTime( __int64 ft, DATETIME* pdt );



__int64 UtilConvertFileTimeToSeconds( const __int64 fileTime );
__int64 UtilConvertSecondsToFileTime( const INT csec );
__int64 UtilConvertFileTimeToDays( const __int64 fileTime );
__int64 UtilGetCurrentFileTime();
__int64 ConvertUtcFileTimeToLocalFileTime( const __int64 filetime );
ERR ErrUtilFormatFileTimeAsDate(
    const __int64 time,
    __out_ecount(cwchOut) wchar_t * const pwszOut,
    const size_t cwchOut,
    __out size_t * const pcwchRequired);
ERR ErrUtilFormatFileTimeAsTime(
    const __int64 time,
    __out_ecount(cwchOut) wchar_t * const pwszOut,
    const size_t cwchOut,
    __out size_t * const pcwchRequired);
ERR ErrUtilFormatFileTimeAsTimeWithSeconds(
    const __int64 time,
    __out_ecount(cwchOut) wchar_t * const pwszOut,
    const size_t cwchOut,
    __out size_t * const pcwchRequired);

    
#endif

