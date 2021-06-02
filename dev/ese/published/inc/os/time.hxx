// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_TIME_HXX_INCLUDED
#define _OS_TIME_HXX_INCLUDED

//  Build Options

//    use x86 assembly for time

#ifdef _M_IX86
#endif  //  _M_IX86


//  Low Resolution Timer

//  returns the current timer count (1 Hz)

ULONG_PTR UlUtilGetSeconds();


//  Medium Resolution Timer

//  a TICK is one millisecond

typedef ULONG TICK;

//  Time Injection Configuration

void OSTimeSetTimeInjection( const DWORD eTimeInjNegWrapMode, const TICK dtickTimeInjWrapOffset, const TICK dtickTimeInjAccelerant );

//  returns the current timer tick count (1000 Hz)

TICK TickOSTimeCurrent();

//  performs an overflow aware comparison of two absolute tick counts

INLINE LONG TickCmp( TICK tick1, TICK tick2 )
{
    return LONG( tick1 - tick2 );
}

//  performs a subtraction of two absolute tick counts

INLINE LONG DtickDelta( TICK tickEarlier, TICK tickLater )
{
    //  I hope this holds, b/c I think it's easier to reason about using code if
    //  this is guaranteed.  I think SOMEONE told me there is some reason this doesn't
    //  hold, but maybe we should fix that client code then.
    //  The reason this doesn't hold, is we used this in multi-threaded 
    //  situtations, where we're racing against another thread, such as the usage
    //  of g_tickLastUpdateStatistics in BFIMaintCacheResidencyScheduleTask().
    //Expected( long( tickLater - tickEarlier ) >= 0 );
    return LONG( tickLater - tickEarlier );
}

//  performs an overflow aware min computation of two absolute tick counts

INLINE TICK TickMin( TICK tick1, TICK tick2 )
{
    return TickCmp( tick1, tick2 ) < 0 ? tick1 : tick2;
}

//  performs an overflow aware max computation of two absolute tick counts

INLINE TICK TickMax( TICK tick1, TICK tick2 )
{
    return TickCmp( tick1, tick2 ) > 0 ? tick1 : tick2;
}


//  High Resolution Timer

//  an HRT is one unit of time such that there are HrtHRTFreq() units contained in one second

typedef QWORD HRT;

//  returns the HRT frequency in Hz

HRT HrtHRTFreq();

//  returns the current HRT count

HRT HrtHRTCount();

//  returns the elapsed time in seconds between the given HRT count and the
//  current HRT count

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

//  returns the high resolution timer value in units of milliseconds, given the frequency

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

//  returns the delta HRT elapsed since a given hrtStartCount

INLINE HRT DhrtHRTElapsedFromHrtStart( const HRT hrtStartCount )
{
    const HRT dhrt = HrtHRTCount() - hrtStartCount;
    return ( dhrt <= (HRT)llMax ? dhrt : 0 );
}

//  returns the high resolution timer value in units of milliseconds

INLINE QWORD CmsecHRTFromDhrt( const HRT dhrt )
{
    return CmsecHRTFromDhrt( dhrt, HrtHRTFreq() );
}

//  returns the high resolution timer value in units of milliseconds

INLINE QWORD CmsecHRTFromHrtStart( const HRT hrtStart )
{
    return CmsecHRTFromDhrt( DhrtHRTElapsedFromHrtStart( hrtStart ), HrtHRTFreq() );
}

//  returns the high resolution timer value in units of microseconds, given the frequency

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

//  returns the high resolution timer interval that corresponds to a time range in milliseconds

INLINE HRT DhrtHRTFromCmsec( const DWORD cmsec )
{
    const HRT hrtFreq = HrtHRTFreq();
    const HRT dhrt = cmsec * hrtFreq;

    Assert( dhrt >= cmsec || hrtFreq == 0 );
    Assert( dhrt >= hrtFreq || cmsec == 0 );

    return ( dhrt / 1000 );
}


//  Date and Time

//  returns the current system date and time

void UtilGetCurrentDateTime( DATETIME* pdt );
__int64 UtilConvertDateTimeToFileTime( DATETIME* pdt );
void UtilConvertFileTimeToDateTime( __int64 ft, DATETIME* pdt );


//  FileTime functions. These expose a UTC FileTime as a 64-bit number
//  and provide formatting functions.

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

    
#endif  //  _OS_TIME_HXX_INCLUDED

