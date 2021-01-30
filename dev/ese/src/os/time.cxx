// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"


#undef GetTickCount




ULONG_PTR UlUtilGetSeconds()
{
    return ULONG_PTR( time( NULL ) );
}



#ifndef ESENT
#endif

#if !defined(RTM) || defined(EXCHANGE_DATACENTER_CACHE_TIMEBOMB)
#define ENABLE_TIME_INJECTION               1
#endif

#ifdef ENABLE_TIME_INJECTION




enum TimeInjWrapMode
{
    tiwmNone = 0,
    tiwmMaxNegToPos,
    tiwmMaxUnsignedToLow,
    tiwmMax
};
TimeInjWrapMode g_eTimeInjNegWrap           = tiwmNone;
TICK g_dtickTimeInjWrapOffset               = 0;
TICK g_dtickTimeInjAccelerant               = 0;


TICK g_dtickTimeInjRealOffset               = 0;
TICK g_tickTimeInjNO2                       = 1;

#endif


static  INLINE TICK TickOSTimeIGetTickCountI()
{
#ifdef EXCHANGE_DATACENTER_CACHE_TIMEBOMB

    return GetTickCount();

#else

#ifdef ENABLE_TIME_INJECTION

    return ( g_dtickTimeInjAccelerant == 0 ) ? GetTickCount() :
            ( g_dtickTimeInjAccelerant == 1 ) ? AtomicIncrement( (LONG*)&g_tickTimeInjNO2 ) :
                ( GetTickCount() * g_dtickTimeInjAccelerant );

#else

#pragma prefast(suppress:28159, "ESE has fixed all known 49-day-rollover problems, and GetTickCount64() is slower.")
    return GetTickCount();
    
#endif

#endif
}




TICK g_tickLastStartRaw     = 0;
TICK g_tickLastSysPreinit   = 0;
TICK g_tickLastSysInit      = 0;
TICK g_tickLastGiven        = 0;
DATETIME g_dtmLastDayUpd    = { 0 };
TICK g_tickLastDayUpd       = 0;

INT g_dtickLastTickGivenUpd = 0;
HRT g_dhrtLastTickGivenUpd  = 0;
HRT g_dhrtfreqLastTickGivenUpd = 0;
HRT g_hrtLastTickGivenUpd   = 0;

#ifdef EXCHANGE_DATACENTER_CACHE_TIMEBOMB
ULONG g_dtickTimebomb       = 23 * 24 * 60 * 60 * 1000;
#endif

static  INLINE TICK TickOSTimeIGetTickCount()
{
#ifdef EXCHANGE_DATACENTER_CACHE_TIMEBOMB
    return TICK( TickOSTimeIGetTickCountI() ) + g_dtickTimeInjRealOffset;
#else
    return TICK( TickOSTimeIGetTickCountI() ) OnNonRTM( + g_dtickTimeInjRealOffset );
#endif
}


TICK TickOSTimeCurrent()
{



    const TICK tickCurrent = TickOSTimeIGetTickCount();

    const TICK tickLastGivenGlobal = g_tickLastGiven;
    if ( TickCmp( tickLastGivenGlobal, tickCurrent ) < 0 )
    {
        const HRT hrtLastTickGiven = HrtHRTCount();
        do
        {
            const TICK tickCheck = AtomicCompareExchange( (LONG*)&g_tickLastGiven, (LONG)tickLastGivenGlobal, (LONG)tickCurrent );
            if ( tickCheck == tickLastGivenGlobal  )
            {
                g_dtickLastTickGivenUpd = tickCurrent - tickLastGivenGlobal;
                g_dhrtLastTickGivenUpd = hrtLastTickGiven - g_hrtLastTickGivenUpd;
                g_dhrtfreqLastTickGivenUpd = HrtHRTFreq();
                g_hrtLastTickGivenUpd = hrtLastTickGiven;
                
                if ( ( tickCurrent - g_tickLastDayUpd ) > 86400000  )
                {
                    UtilGetCurrentDateTime( &g_dtmLastDayUpd );
                    g_tickLastDayUpd = tickCurrent;
                }
            }
        }
        while( TickCmp( g_tickLastGiven, tickCurrent ) < 0 );
        Assert( TickCmp( g_tickLastGiven, tickCurrent ) >= 0 );

#ifdef EXCHANGE_DATACENTER_CACHE_TIMEBOMB
#ifndef ESENT
        Enforce( (ULONG)DtickDelta( g_tickLastStartRaw, GetTickCount() ) < g_dtickTimebomb );
#endif
#endif
    }
    return tickCurrent;
}




enum HRTType
{
    hrttUninit,
    hrttNone,
    hrttWin32,
#ifdef TIME_USE_X86_ASM
    hrttPentium,
#endif
} g_hrtt;


HRT g_hrtHRTFreq;

#ifdef TIME_USE_X86_ASM


#define rdtsc __asm _emit 0x0f __asm _emit 0x31

#endif


BOOL IsRDTSCAvailable()
{
    static BOOL     fRDTSCAvailable                 = fFalse;

    if ( !fRDTSCAvailable )
    {
        fRDTSCAvailable = IsProcessorFeaturePresent( PF_RDTSC_INSTRUCTION_AVAILABLE );
    }

    return fRDTSCAvailable;
}


void UtilHRTInit()
{

    if ( QueryPerformanceFrequency( (LARGE_INTEGER *) &g_hrtHRTFreq ) )
    {
        g_hrtt = hrttWin32;
    }


    else
    {

        QWORDX qwx;
        qwx.SetDwLow( 1000 );
        qwx.SetDwHigh( 0 );
        g_hrtHRTFreq = qwx.Qw();

        g_hrtt = hrttNone;
    }

#ifdef TIME_USE_X86_ASM


    if ( IsRDTSCAvailable() )
    {

        QWORDX qwxTime1a;
        QWORDX qwxTime1b;
        QWORDX qwxTime2a;
        QWORDX qwxTime2b;
        if ( g_hrtt == hrttWin32 )
        {
            __asm xchg      eax, edx
            __asm rdtsc
            __asm mov       qwxTime1a.m_l,eax
            __asm mov       qwxTime1a.m_h,edx
            QueryPerformanceCounter( (LARGE_INTEGER*) qwxTime1b.Pqw() );
            Sleep( 50 );
            __asm xchg      eax, edx
            __asm rdtsc
            __asm mov       qwxTime2a.m_l,eax
            __asm mov       qwxTime2a.m_h,edx
            QueryPerformanceCounter( (LARGE_INTEGER*) qwxTime2b.Pqw() );
            g_hrtHRTFreq =  ( g_hrtHRTFreq * ( qwxTime2a.Qw() - qwxTime1a.Qw() ) ) /
                        ( qwxTime2b.Qw() - qwxTime1b.Qw() );
            g_hrtHRTFreq = ( ( g_hrtHRTFreq + 50000 ) / 100000 ) * 100000;
        }
        else
        {
            __asm xchg      eax, edx
            __asm rdtsc
            __asm mov       qwxTime1a.m_l,eax
            __asm mov       qwxTime1a.m_h,edx
            qwxTime1b.SetDwLow( GetTickCount() );
            qwxTime1b.SetDwHigh( 0 );
            Sleep( 2000 );
            __asm xchg      eax, edx
            __asm rdtsc
            __asm mov       qwxTime2a.m_l,eax
            __asm mov       qwxTime2a.m_h,edx
            qwxTime2b.SetDwLow( GetTickCount() );
            qwxTime2b.SetDwHigh( 0 );
            g_hrtHRTFreq =  ( g_hrtHRTFreq * ( qwxTime2a.Qw() - qwxTime1a.Qw() ) ) /
                        ( qwxTime2b.Qw() - qwxTime1b.Qw() );
            g_hrtHRTFreq = ( ( g_hrtHRTFreq + 500000 ) / 1000000 ) * 1000000;
        }

        g_hrtt = hrttPentium;
    }

#endif

}


HRT HrtHRTFreq()
{
    if ( g_hrtt == hrttUninit )
    {
        UtilHRTInit();
    }

    return g_hrtHRTFreq;
}


HRT g_hrtLastGiven = 0;
HRT g_dhrtLastGivenUpdTimeout = 0;

HRT HrtHRTCount()
{
    QWORDX qwx;

    if ( g_hrtt == hrttUninit )
    {
        UtilHRTInit();
    }

    switch ( g_hrtt )
    {
        case hrttNone:
#pragma prefast(suppress:28159, "ESE has fixed all known 49-day-rollover problems, and GetTickCount64() is slower.")
            qwx.SetDwLow( GetTickCount() );
            qwx.SetDwHigh( 0 );
            break;

        case hrttWin32:
            QueryPerformanceCounter( (LARGE_INTEGER*) qwx.Pqw() );
            break;

#ifdef TIME_USE_X86_ASM

        case hrttPentium:
            __asm xchg      eax, edx
            __asm rdtsc
            __asm mov       qwx.m_l,eax
            __asm mov       qwx.m_h,edx
            break;

#endif
    }

    const HRT hrt = qwx.Qw();



#ifdef _X86_


    if ( ( (DWORD)hrt - (DWORD)g_hrtLastGiven ) > (DWORD)g_dhrtLastGivenUpdTimeout )
    {
        AtomicExchange( (__int64*)&g_hrtLastGiven, (__int64)hrt );
    }

#else

    if ( ( hrt - g_hrtLastGiven ) > g_dhrtLastGivenUpdTimeout )
    {
        g_hrtLastGiven = hrt;
    }

#endif

    return hrt;
}



void UtilGetCurrentDateTime( DATETIME *pdate )
{
    SYSTEMTIME              systemtime;

    GetSystemTime( &systemtime );

    pdate->month    = systemtime.wMonth;
    pdate->day      = systemtime.wDay;
    pdate->year     = systemtime.wYear;
    pdate->hour     = systemtime.wHour;
    pdate->minute   = systemtime.wMinute;
    pdate->second   = systemtime.wSecond;
    pdate->millisecond = systemtime.wMilliseconds;
}

__int64 UtilConvertDateTimeToFileTime( DATETIME* pdt )
{
    SYSTEMTIME systemtime;
    systemtime.wYear = WORD( pdt->year );
    systemtime.wMonth = WORD( pdt->month );
    systemtime.wDay = WORD( pdt->day );
    systemtime.wHour = WORD( pdt->hour );
    systemtime.wMinute = WORD( pdt->minute );
    systemtime.wSecond = WORD( pdt->second );
    systemtime.wMilliseconds = WORD( pdt->millisecond );
    
    FILETIME filetime = {0, };
    SystemTimeToFileTime( &systemtime, &filetime );
    return *( __int64* )&filetime;
}


LOCAL SYSTEMTIME ConvertFileTimeToUTCSystemTime( const __int64 time )
{
    SYSTEMTIME systemtimeUTC;
    FileTimeToSystemTime(
        (FILETIME *)&time,
        &systemtimeUTC);

    return systemtimeUTC;
}

LOCAL SYSTEMTIME ConvertFileTimeToLocalSystemTime( const __int64 time )
{
    SYSTEMTIME systemtimeUTC = ConvertFileTimeToUTCSystemTime( time );
    SYSTEMTIME systemtime;
    SystemTimeToTzSpecificLocalTime(
        0,
        &systemtimeUTC,
        &systemtime );

    return systemtime;
}

void UtilConvertFileTimeToDateTime( __int64 ft, DATETIME* pdt )
{
    SYSTEMTIME systemtime = ConvertFileTimeToUTCSystemTime( ft );

    pdt->month  = systemtime.wMonth;
    pdt->day    = systemtime.wDay;
    pdt->year   = systemtime.wYear;
    pdt->hour   = systemtime.wHour;
    pdt->minute = systemtime.wMinute;
    pdt->second = systemtime.wSecond;
    pdt->millisecond = systemtime.wMilliseconds;
}
 
LOCAL ERR ErrFromWin32Error( DWORD dwError )
{
    ERR err;
    switch( dwError )
    {
        case ERROR_INSUFFICIENT_BUFFER:
            err = ErrERRCheck( JET_errBufferTooSmall );
            break;
        case ERROR_INVALID_FLAGS:
        case ERROR_INVALID_PARAMETER:
            err = ErrERRCheck( JET_errInvalidParameter );
            break;
        default:
            err = ErrERRCheck( JET_errInternalError );
            break;
    }
    return err;
}

const __int64 fileTimeToSeconds = 10000000;

__int64 UtilConvertFileTimeToSeconds( const __int64 fileTime )
{
    return fileTime / fileTimeToSeconds;
}

__int64 UtilConvertSecondsToFileTime( const INT csec )
{
    return csec * fileTimeToSeconds;
}

__int64 UtilConvertFileTimeToDays( const __int64 fileTime )
{
    return max( 1, UtilConvertFileTimeToSeconds( fileTime ) / ( 60 * 60 * 24 ) );
}

__int64 UtilGetCurrentFileTime()
{
    __int64 time;
    GetSystemTimeAsFileTime( (FILETIME *)&time );
    Assert( 0 != time );
    return time;
}

__int64 ConvertUtcFileTimeToLocalFileTime( __int64 filetimeUTC )
{
    SYSTEMTIME systemtimeLocal = ConvertFileTimeToLocalSystemTime( filetimeUTC );
    __int64 filetimeLocal = 0;
    if( SystemTimeToFileTime( &systemtimeLocal, (FILETIME *)&filetimeLocal ) )
    {
        return filetimeLocal;
    }
    return filetimeUTC;
}

ERR ErrUtilFormatFileTimeAsDate(
    const __int64 time,
    __out_ecount(cwchOut) PWSTR const pwszOut,
    const size_t cwchOut,
    __out size_t * const pcwchRequired)
{
    SYSTEMTIME systemtime = ConvertFileTimeToLocalSystemTime( time );
    *pcwchRequired = GetDateFormatW(
        LOCALE_USER_DEFAULT,
        DATE_SHORTDATE,
        &systemtime,
        0,
        pwszOut,
        cwchOut );
    
    if(0 == *pcwchRequired)
    {
        return ErrFromWin32Error( GetLastError() );
    }
    return JET_errSuccess;
}

ERR ErrUtilFormatFileTime(
    const __int64 time,
    const DWORD dwFlags,
    __out_ecount(cwchOut) PWSTR const pwszOut,
    const size_t cwchOut,
    __out size_t * const pcwchRequired)
{
    SYSTEMTIME systemtime = ConvertFileTimeToLocalSystemTime( time );
    *pcwchRequired = GetTimeFormatW(
        LOCALE_USER_DEFAULT,
        dwFlags,
        &systemtime,
        0,
        pwszOut,
        cwchOut );

    if(0 == *pcwchRequired)
    {
        return ErrFromWin32Error( GetLastError() );
    }
    return JET_errSuccess;
}

ERR ErrUtilFormatFileTimeAsTime(
    const __int64 time,
    __out_ecount(cwchOut) PWSTR const pwszOut,
    const size_t cwchOut,
    __out size_t * const pcwchRequired)
{
    return ErrUtilFormatFileTime( time, TIME_NOSECONDS, pwszOut, cwchOut, pcwchRequired );
}

ERR ErrUtilFormatFileTimeAsTimeWithSeconds(
    const __int64 time,
    __out_ecount(cwchOut) PWSTR const pwszOut,
    const size_t cwchOut,
    __out size_t * const pcwchRequired)
{
    return ErrUtilFormatFileTime( time, 0, pwszOut, cwchOut, pcwchRequired );
}



void OSTimePostterm()
{
}


DWORD DwRand()
{
    C_ASSERT( RAND_MAX == 32 * 1024 - 1 );
    return ( ( rand() & 0x3 ) << 30 ) + ( rand() << 15 ) + rand();
}


void OSTimeIInitLast( const TICK tickNow )
{
    g_tickLastGiven = tickNow;

    UtilGetCurrentDateTime( &g_dtmLastDayUpd );
    g_tickLastDayUpd = tickNow;

    g_hrtLastGiven = HrtHRTCount();
}


BOOL FOSTimePreinit()
{

#pragma prefast(suppress:28159, "ESE has fixed all known 49-day-rollover problems, and GetTickCount64() is slower.")
    g_tickLastStartRaw = GetTickCount();
    g_tickLastSysPreinit = TickOSTimeIGetTickCount();

    OSTimeIInitLast( g_tickLastSysPreinit );


#ifdef _X86_

    g_dhrtLastGivenUpdTimeout = max( 1, DhrtHRTFromCmsec( 1 ) );

#else

    g_dhrtLastGivenUpdTimeout = (DWORD)min( g_dhrtLastGivenUpdTimeout, ( ~(DWORD)0 ) / 2 );

#endif



    return fTrue;
}



void OSTimeTerm()
{

    g_tickLastSysInit = 0;
}


void OSTimeSetTimeInjection( const DWORD eTimeInjNegWrapMode, const TICK dtickTimeInjWrapOffset, const TICK dtickTimeInjAccelerant )
{
#ifdef ENABLE_TIME_INJECTION
    Assert( dtickTimeInjWrapOffset  );
    g_eTimeInjNegWrap = TimeInjWrapMode( eTimeInjNegWrapMode );
    g_dtickTimeInjWrapOffset = dtickTimeInjWrapOffset;
    g_dtickTimeInjAccelerant = dtickTimeInjAccelerant;
#else
#endif
}


ERR ErrOSTimeInit()
{
#ifdef ENABLE_TIME_INJECTION

#ifdef EXCHANGE_DATACENTER_CACHE_TIMEBOMB

    g_eTimeInjNegWrap                           = tiwmNone;
    g_dtickTimeInjWrapOffset                    = 1; 
    g_dtickTimeInjAccelerant                    = 0;

    const TICK tickTimeInjStart                 = 1000000 - g_dtickTimeInjWrapOffset;

#else


    if ( 0 == g_dtickTimeInjWrapOffset )
    {

        srand( GetTickCount() );
        g_eTimeInjNegWrap                           = TimeInjWrapMode( rand() % tiwmMax );

        g_dtickTimeInjWrapOffset                    = 1; 
        if ( g_eTimeInjNegWrap != tiwmNone )
        {
            switch( rand() % 8 )
            {
                default:
                case 0: ;                                          break;
                case 1: g_dtickTimeInjWrapOffset    = 1 + DwRand() % 100;                       break;
                case 2: g_dtickTimeInjWrapOffset    = 1 + DwRand() % 2000;                      break;
                case 3: g_dtickTimeInjWrapOffset    = 1 + DwRand() % ( 60 * 1000 );             break;
                case 4: g_dtickTimeInjWrapOffset    = 1 + DwRand() % ( 10 * 60 * 1000 );        break;
                case 5: g_dtickTimeInjWrapOffset    = 1 + DwRand() % ( 60 * 60 * 1000 );        break;
                case 6: g_dtickTimeInjWrapOffset    = -INT( DwRand() % ( 19 * 60 * 1000 ) );    break;
                case 7: g_dtickTimeInjWrapOffset    = -INT( DwRand() % ( 60 * 60 * 1000 ) );    break;
            }
        }


#ifdef ENABLE_TIME_INJECTION_ACCELERANT

        if ( g_dtickTimeInjAccelerant == 0 )
        {
            switch( rand() % 3 )
            {
                default:
                case 0: g_dtickTimeInjAccelerant    = 0;     break;
                case 1: g_dtickTimeInjAccelerant    = 1;      break;
                case 2: g_dtickTimeInjAccelerant    = rand() % 100;         break;
            }
        }

#else

        Assert( g_dtickTimeInjAccelerant == 0 );

#endif

    }


    const TICK tickTimeInjStart                 = ( g_eTimeInjNegWrap == tiwmNone ) ?
                                                    ( TickOSTimeIGetTickCountI() ) :
                                                    ( ( g_eTimeInjNegWrap == tiwmMaxNegToPos ? 0x7FFFFFFF : 0xFFFFFFFF ) - g_dtickTimeInjWrapOffset );

#endif

    if ( g_dtickTimeInjAccelerant == 1 )
    {
        g_tickTimeInjNO2                    = tickTimeInjStart;
    }

    g_dtickTimeInjRealOffset                = tickTimeInjStart - TickOSTimeIGetTickCountI();

#else

#pragma prefast(suppress:28159, "ESE has fixed all known 49-day-rollover problems, and GetTickCount64() is slower.")
    const TICK tickTimeInjStart             = GetTickCount();

#endif


    g_tickLastSysInit = TickOSTimeIGetTickCount();


    if ( DtickDelta( tickTimeInjStart, g_tickLastSysInit ) > ( 2 * 60 * 1000 ) )
    {
        FireWall( "OsTimeInitHung" );
    }

    OSTimeIInitLast( g_tickLastSysInit );

    return JET_errSuccess;
}


