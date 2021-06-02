// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

//  This is the one file that is allowed to use GetTickCount which it adjusts
//  for time injection.

#undef GetTickCount


//  Low Resolution Timer

//  returns the current timer count (1 Hz)

ULONG_PTR UlUtilGetSeconds()
{
    return ULONG_PTR( time( NULL ) );
}


//  Medium Resolution Timer

#ifndef ESENT   //  just in case this integrates to phone
#endif

#if !defined(RTM) || defined(EXCHANGE_DATACENTER_CACHE_TIMEBOMB)
#define ENABLE_TIME_INJECTION               1
//  this option is probably dangerous (and burns down oslayerunit.exe)
//#define ENABLE_TIME_INJECTION_ACCELERANT  1
#endif

#ifdef ENABLE_TIME_INJECTION

//  for time injection of tick wrap and accelerated time scenarios

//  if you ever need the true system GetTickCount() value from an ESE Time-Injected TICK
//  value, just take the ESE tickTime and subtract ese!g_dtickTimeInjRealOffset from it.

//      configuration variables, that describe the Time-Injection type chosen

enum TimeInjWrapMode
{
    tiwmNone = 0,               // 0 = no wrap
    tiwmMaxNegToPos,            // 1 = 0x7f..ff -> 0x80..00 wrap;
    tiwmMaxUnsignedToLow,       // 2 = regular wrap 0xFF..FF -> 0x00
    tiwmMax
};
TimeInjWrapMode g_eTimeInjNegWrap           = tiwmNone; //  Configured (randomly in preinit)
TICK g_dtickTimeInjWrapOffset               = 0;        //  Configured (randomly in preinit): delta time before wrap.
TICK g_dtickTimeInjAccelerant               = 0;        //  Configured (if compiled, in preinit): 0 = none; 1 = auto-inc time; 2+ multiplier on GetTickCount()

//      runtime variables, that are used for the actual time alteration / injection

TICK g_dtickTimeInjRealOffset               = 0;        //  Runtime Variable: Offset to apply to real OS GetTickCount()
TICK g_tickTimeInjNO2                       = 1;        //  This is the tick time Auto-inc mode runtime var

#endif  //  ENABLE_TIME_INJECTION

//  this function internally takes the accelerant into account to "speed up" time

static /* very private */ INLINE TICK TickOSTimeIGetTickCountI()
{
#ifdef EXCHANGE_DATACENTER_CACHE_TIMEBOMB

    return GetTickCount();  //  the one true source of time!!!

#else   //  !EXCHANGE_DATACENTER_CACHE_TIMEBOMB

#ifdef ENABLE_TIME_INJECTION

    return ( g_dtickTimeInjAccelerant == 0 ) ? GetTickCount() :
            ( g_dtickTimeInjAccelerant == 1 ) ? AtomicIncrement( (LONG*)&g_tickTimeInjNO2 ) :
                ( GetTickCount() * g_dtickTimeInjAccelerant );

#else   //  !ENABLE_TIME_INJECTION

#pragma prefast(suppress:28159, "ESE has fixed all known 49-day-rollover problems, and GetTickCount64() is slower.")
    return GetTickCount();  //  the one true source of time!!!
    
#endif  //  ENABLE_TIME_INJECTION

#endif  //  EXCHANGE_DATACENTER_CACHE_TIMEBOMB
}



//  for debugging - it makes it trivial to detect if hung IOs are holding up ESE, by keeping a 
//  reasonably current tick in a global.

TICK g_tickLastStartRaw     = 0;    // guaranteed NON-adjusted time of first FOSTimePreinit() call
TICK g_tickLastSysPreinit   = 0;    // NON-adjusted time on first FOSTimePreinit() call, adjusted on subsequent runs ...
TICK g_tickLastSysInit      = 0;    // time-injection-adjusted time of ErrOSTimeInit() ...
TICK g_tickLastGiven        = 0;    // we maintain this for debugging, 99.999997% accurate ...
DATETIME g_dtmLastDayUpd    = { 0 };// updated ~once/day
                                        // so gives idea what time of day ESE init'd ...
                                        // as well as current time when taken w/ the two tick vars.
TICK g_tickLastDayUpd       = 0;

INT g_dtickLastTickGivenUpd = 0;
HRT g_dhrtLastTickGivenUpd  = 0;
HRT g_dhrtfreqLastTickGivenUpd = 0;
HRT g_hrtLastTickGivenUpd   = 0;

#ifdef EXCHANGE_DATACENTER_CACHE_TIMEBOMB
ULONG g_dtickTimebomb       = 23 * 24 * 60 * 60 * 1000; // 23 days
#endif

static /* very private */ INLINE TICK TickOSTimeIGetTickCount()
{
#ifdef EXCHANGE_DATACENTER_CACHE_TIMEBOMB
    return TICK( TickOSTimeIGetTickCountI() ) + g_dtickTimeInjRealOffset;
#else
    return TICK( TickOSTimeIGetTickCountI() ) OnNonRTM( + g_dtickTimeInjRealOffset );
#endif
}

//  returns the current timer count (1000 Hz)

TICK TickOSTimeCurrent()
{
    //  ensure we've been initialized to avoid a time schism


    //  get tick time (adjusted for time injection)

    const TICK tickCurrent = TickOSTimeIGetTickCount();

    //  the TickCmp() call keeps the global variables from being updated more frequently
    //  than once every GetTickCount() resolution, which is 15.6 msec by default.  The
    //  CPU cost of this will of course jump by 16x if the OS ever makes the underlying
    //  tick infra more granular. ;)
    const TICK tickLastGivenGlobal = g_tickLastGiven;
    if ( TickCmp( tickLastGivenGlobal, tickCurrent ) < 0 )
    {
        const HRT hrtLastTickGiven = HrtHRTCount();
        do
        {
            const TICK tickCheck = AtomicCompareExchange( (LONG*)&g_tickLastGiven, (LONG)tickLastGivenGlobal, (LONG)tickCurrent );
            if ( tickCheck == tickLastGivenGlobal /* we won the race */ )
            {
                //  Does not guarantee solitary update of these vars, but makes it very very likely.
                g_dtickLastTickGivenUpd = tickCurrent - tickLastGivenGlobal;
                g_dhrtLastTickGivenUpd = hrtLastTickGiven - g_hrtLastTickGivenUpd;
                g_dhrtfreqLastTickGivenUpd = HrtHRTFreq();
                g_hrtLastTickGivenUpd = hrtLastTickGiven;
                
                if ( ( tickCurrent - g_tickLastDayUpd ) > 86400000 /* 1 day */ )
                {
                    //  Get the time as well, so with the tick we can determine true time.
                    UtilGetCurrentDateTime( &g_dtmLastDayUpd );
                    g_tickLastDayUpd = tickCurrent;
                }
            }
        }
        while( TickCmp( g_tickLastGiven, tickCurrent ) < 0 );
        Assert( TickCmp( g_tickLastGiven, tickCurrent ) >= 0 );

#ifdef EXCHANGE_DATACENTER_CACHE_TIMEBOMB
#ifndef ESENT   //  (double) just in case this integrates to phone
        Enforce( (ULONG)DtickDelta( g_tickLastStartRaw, GetTickCount() ) < g_dtickTimebomb );               //  raw tick check
#endif
#endif
    }
    return tickCurrent;
}


//  High Resolution Timer

//    High Resolution Timer Type

enum HRTType
{
    hrttUninit,
    hrttNone,
    hrttWin32,
#ifdef TIME_USE_X86_ASM
    hrttPentium,
#endif  //  TIME_USE_X86_ASM
} g_hrtt;

//    HRT Frequency

HRT g_hrtHRTFreq;

#ifdef TIME_USE_X86_ASM

//    Pentium Time Stamp Counter Fetch

#define rdtsc __asm _emit 0x0f __asm _emit 0x31

#endif  //  TIME_USE_X86_ASM

//  returns fTrue if we are allowed to use RDTSC

BOOL IsRDTSCAvailable()
{
    static BOOL     fRDTSCAvailable                 = fFalse;

    if ( !fRDTSCAvailable )
    {
        fRDTSCAvailable = IsProcessorFeaturePresent( PF_RDTSC_INSTRUCTION_AVAILABLE );
    }

    return fRDTSCAvailable;
}

//  initializes the HRT subsystem

void UtilHRTInit()
{
    //  Win32 high resolution counter is available

    if ( QueryPerformanceFrequency( (LARGE_INTEGER *) &g_hrtHRTFreq ) )
    {
        g_hrtt = hrttWin32;
    }

    //  Win32 high resolution counter is not available

    else
    {
        //  fall back on GetTickCount() (ms since Windows has started)

        QWORDX qwx;
        qwx.SetDwLow( 1000 );
        qwx.SetDwHigh( 0 );
        g_hrtHRTFreq = qwx.Qw();

        g_hrtt = hrttNone;
    }

#ifdef TIME_USE_X86_ASM

    //  can we use the TSC?

    if ( IsRDTSCAvailable() )
    {
        //  use pentium TSC register, but first find clock frequency experimentally

        QWORDX qwxTime1a;
        QWORDX qwxTime1b;
        QWORDX qwxTime2a;
        QWORDX qwxTime2b;
        if ( g_hrtt == hrttWin32 )
        {
            __asm xchg      eax, edx  //  HACK:  cl 11.00.7022 needs this
            __asm rdtsc
            __asm mov       qwxTime1a.m_l,eax   //lint !e530
            __asm mov       qwxTime1a.m_h,edx   //lint !e530
            QueryPerformanceCounter( (LARGE_INTEGER*) qwxTime1b.Pqw() );
            Sleep( 50 );
            __asm xchg      eax, edx  //  HACK:  cl 11.00.7022 needs this
            __asm rdtsc
            __asm mov       qwxTime2a.m_l,eax   //lint !e530
            __asm mov       qwxTime2a.m_h,edx   //lint !e530
            QueryPerformanceCounter( (LARGE_INTEGER*) qwxTime2b.Pqw() );
            g_hrtHRTFreq =  ( g_hrtHRTFreq * ( qwxTime2a.Qw() - qwxTime1a.Qw() ) ) /
                        ( qwxTime2b.Qw() - qwxTime1b.Qw() );
            g_hrtHRTFreq = ( ( g_hrtHRTFreq + 50000 ) / 100000 ) * 100000;
        }
        else
        {
            __asm xchg      eax, edx  //  HACK:  cl 11.00.7022 needs this
            __asm rdtsc
            __asm mov       qwxTime1a.m_l,eax
            __asm mov       qwxTime1a.m_h,edx
            qwxTime1b.SetDwLow( GetTickCount() );
            qwxTime1b.SetDwHigh( 0 );
            Sleep( 2000 );
            __asm xchg      eax, edx  //  HACK:  cl 11.00.7022 needs this
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

#endif  //  TIME_USE_X86_ASM

}

//  returns the current HRT frequency

HRT HrtHRTFreq()
{
    if ( g_hrtt == hrttUninit )
    {
        UtilHRTInit();
    }

    return g_hrtHRTFreq;
}

//  returns the current HRT count

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
            __asm xchg      eax, edx  //  HACK:  cl 11.00.7022 needs this
            __asm rdtsc
            __asm mov       qwx.m_l,eax
            __asm mov       qwx.m_h,edx
            break;

#endif  //  TIME_USE_X86_ASM
    }

    const HRT hrt = qwx.Qw();

    //  prevent hot update of the global


#ifdef _X86_

    //  given that we don't want to pay synchronization costs here, we will do
    //  a dirty compare-and-set, but for 32-bit processors, this may lead to
    //  inconsistent 64-bit values being compared and set, so we will use a
    //  DWORD version to compare and an interlocked instruction to set.

    if ( ( (DWORD)hrt - (DWORD)g_hrtLastGiven ) > (DWORD)g_dhrtLastGivenUpdTimeout )
    {
        AtomicExchange( (__int64*)&g_hrtLastGiven, (__int64)hrt );
    }

#else   //   !_X86_

    if ( ( hrt - g_hrtLastGiven ) > g_dhrtLastGivenUpdTimeout )
    {
        g_hrtLastGiven = hrt;
    }

#endif  //   _X86_

    return hrt;
}


//  returns the current system date and time

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

//  FileTime functions. These expose a UTC FileTime as a 64-bit number
//  and provide formatting functions.

//  ================================================================
LOCAL SYSTEMTIME ConvertFileTimeToUTCSystemTime( const __int64 time )
//  ================================================================
{
    SYSTEMTIME systemtimeUTC;
    FileTimeToSystemTime(
        (FILETIME *)&time,
        &systemtimeUTC);

    return systemtimeUTC;
}

//  ================================================================
LOCAL SYSTEMTIME ConvertFileTimeToLocalSystemTime( const __int64 time )
//  ================================================================
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
 
//  ================================================================
LOCAL ERR ErrFromWin32Error( DWORD dwError )
//  ================================================================
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

//  ================================================================
__int64 UtilConvertFileTimeToSeconds( const __int64 fileTime )
//  ================================================================
{
    return fileTime / fileTimeToSeconds;
}

//  ================================================================
__int64 UtilConvertSecondsToFileTime( const INT csec )
//  ================================================================
{
    return csec * fileTimeToSeconds;
}

//  ================================================================
__int64 UtilConvertFileTimeToDays( const __int64 fileTime )
//  ================================================================
//
// Normally truncation would give us a days value of zero for anything
// less than one day. For reporting we want the number of days to
// always be at least 1
//
//-
{
    return max( 1, UtilConvertFileTimeToSeconds( fileTime ) / ( 60 * 60 * 24 ) );
}

//  ================================================================
__int64 UtilGetCurrentFileTime()
//  ================================================================
{
    __int64 time;
    GetSystemTimeAsFileTime( (FILETIME *)&time );
    Assert( 0 != time );
    return time;
}

//  ================================================================
__int64 ConvertUtcFileTimeToLocalFileTime( __int64 filetimeUTC )
//  ================================================================
{
    SYSTEMTIME systemtimeLocal = ConvertFileTimeToLocalSystemTime( filetimeUTC );
    __int64 filetimeLocal = 0;
    if( SystemTimeToFileTime( &systemtimeLocal, (FILETIME *)&filetimeLocal ) )
    {
        return filetimeLocal;
    }
    // if we can't convert return the original time
    return filetimeUTC;
}

//  ================================================================
ERR ErrUtilFormatFileTimeAsDate(
    const __int64 time,
    __out_ecount(cwchOut) PWSTR const pwszOut,
    const size_t cwchOut,
    __out size_t * const pcwchRequired)
//  ================================================================
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

//  ================================================================
ERR ErrUtilFormatFileTime(
    const __int64 time,
    const DWORD dwFlags,
    __out_ecount(cwchOut) PWSTR const pwszOut,
    const size_t cwchOut,
    __out size_t * const pcwchRequired)
//  ================================================================
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

//  ================================================================
ERR ErrUtilFormatFileTimeAsTime(
    const __int64 time,
    __out_ecount(cwchOut) PWSTR const pwszOut,
    const size_t cwchOut,
    __out size_t * const pcwchRequired)
//  ================================================================
{
    return ErrUtilFormatFileTime( time, TIME_NOSECONDS, pwszOut, cwchOut, pcwchRequired );
}

//  ================================================================
ERR ErrUtilFormatFileTimeAsTimeWithSeconds(
    const __int64 time,
    __out_ecount(cwchOut) PWSTR const pwszOut,
    const size_t cwchOut,
    __out size_t * const pcwchRequired)
//  ================================================================
{
    return ErrUtilFormatFileTime( time, 0, pwszOut, cwchOut, pcwchRequired );
}


// post- terminate time subsystem

void OSTimePostterm()
{
    //  nop
}

//  gives a random evenly distributed number of 32-bits
//  note: presumes rand()'s RAND_MAX is at least 32k

DWORD DwRand()
{
    //  better would be to shift ...
    C_ASSERT( RAND_MAX == 32 * 1024 - 1 );
    return ( ( rand() & 0x3 ) << 30 ) + ( rand() << 15 ) + rand();
}

//  initializes all our debugger time variables (from preinit and init)

void OSTimeIInitLast( const TICK tickNow )
{
    g_tickLastGiven = tickNow;

    UtilGetCurrentDateTime( &g_dtmLastDayUpd );
    g_tickLastDayUpd = tickNow;

    g_hrtLastGiven = HrtHRTCount();
}

//  pre-init time subsystem

BOOL FOSTimePreinit()
{
    //  init the diagnostic variables

#pragma prefast(suppress:28159, "ESE has fixed all known 49-day-rollover problems, and GetTickCount64() is slower.")
    g_tickLastStartRaw = GetTickCount();
    g_tickLastSysPreinit = TickOSTimeIGetTickCount();

    OSTimeIInitLast( g_tickLastSysPreinit );

    //  set the update rate for the hrt last given variable

#ifdef _X86_

    g_dhrtLastGivenUpdTimeout = max( 1, DhrtHRTFromCmsec( 1 ) );

#else

    g_dhrtLastGivenUpdTimeout = (DWORD)min( g_dhrtLastGivenUpdTimeout, ( ~(DWORD)0 ) / 2 );

#endif  //   _X86_

    //  for this to hold we need to time term ... 

    //Assert( g_dtickTimeInjWrapOffset == 0 );

    return fTrue;
}


//  terminate time subsystem

void OSTimeTerm()
{
    //  nop

    g_tickLastSysInit = 0;
}

//  allows tests to control the time injection

void OSTimeSetTimeInjection( const DWORD eTimeInjNegWrapMode, const TICK dtickTimeInjWrapOffset, const TICK dtickTimeInjAccelerant )
{
#ifdef ENABLE_TIME_INJECTION
    Assert( dtickTimeInjWrapOffset /* must at least be 1 */ );
    g_eTimeInjNegWrap = TimeInjWrapMode( eTimeInjNegWrapMode );
    g_dtickTimeInjWrapOffset = dtickTimeInjWrapOffset;
    g_dtickTimeInjAccelerant = dtickTimeInjAccelerant;
#else
    // return error maybe?
#endif
}

//  init time subsystem

ERR ErrOSTimeInit()
{
#ifdef ENABLE_TIME_INJECTION

#ifdef EXCHANGE_DATACENTER_CACHE_TIMEBOMB

    g_eTimeInjNegWrap                           = tiwmNone;
    g_dtickTimeInjWrapOffset                    = 1; /* effectively none */
    g_dtickTimeInjAccelerant                    = 0;

    //  We use 1M, because the LRU-k approx index has a max range of 500k, and we
    //  seem to use low buckets.  This is as if store started only 16 minutes after
    //  the server started.
    const TICK tickTimeInjStart                 = 1000000 - g_dtickTimeInjWrapOffset;

#else   //  !EXCHANGE_DATACENTER_CACHE_TIMEBOMB

    //  check to see if we've been configured by test

    if ( 0 == g_dtickTimeInjWrapOffset )
    {
        //  configure (and store for debugging) the time injection settings

        srand( GetTickCount() );
        g_eTimeInjNegWrap                           = TimeInjWrapMode( rand() % tiwmMax );

        g_dtickTimeInjWrapOffset                    = 1; /* effectively none */
        if ( g_eTimeInjNegWrap != tiwmNone )
        {
            switch( rand() % 8 )
            {
                default:
                case 0: ; /* use default value above */                                         break;
                case 1: g_dtickTimeInjWrapOffset    = 1 + DwRand() % 100;                       break;  //  rand w/in 100 ms
                case 2: g_dtickTimeInjWrapOffset    = 1 + DwRand() % 2000;                      break;  //  rand w/in 2 sec
                case 3: g_dtickTimeInjWrapOffset    = 1 + DwRand() % ( 60 * 1000 );             break;  //  rand w/in 1 min
                case 4: g_dtickTimeInjWrapOffset    = 1 + DwRand() % ( 10 * 60 * 1000 );        break;  //  rand w/in 10 min
                case 5: g_dtickTimeInjWrapOffset    = 1 + DwRand() % ( 60 * 60 * 1000 );        break;  //  rand w/in 1 hour
                case 6: g_dtickTimeInjWrapOffset    = -INT( DwRand() % ( 19 * 60 * 1000 ) );    break;  //  rand ahead w/in 10 min ... past rollover
                case 7: g_dtickTimeInjWrapOffset    = -INT( DwRand() % ( 60 * 60 * 1000 ) );    break;  //  rand ahead w/in 1 hour
            }
        }

        //  For when you want to temporarily hard code time injection ... 
        //g_eTimeInjNegWrap = tiwmMaxUnsignedToLow; // options: tiwmNone; tiwmMaxNegToPos; tiwmMaxUnsignedToLow;
        //g_dtickTimeInjWrapOffset = 5 * 60 * 1000; // options: (DWORD)-INT( 5 * 60 * 1000 ); 5 * 60 * 1000;

#ifdef ENABLE_TIME_INJECTION_ACCELERANT

        if ( g_dtickTimeInjAccelerant == 0 )
        {
            switch( rand() % 3 )
            {
                default:
                case 0: g_dtickTimeInjAccelerant    = 0; /* no accel. */    break;
                case 1: g_dtickTimeInjAccelerant    = 1; /* auto-inc */     break;
                case 2: g_dtickTimeInjAccelerant    = rand() % 100;         break;
            }
        }

#else   //  !ENABLE_TIME_INJECTION_ACCELERANT

        Assert( g_dtickTimeInjAccelerant == 0 );

#endif  //  ENABLE_TIME_INJECTION_ACCELERANT

    }

    //  set the live / runtime time inj variables

    const TICK tickTimeInjStart                 = ( g_eTimeInjNegWrap == tiwmNone ) ?
                                                    ( TickOSTimeIGetTickCountI() ) :
                                                    ( ( g_eTimeInjNegWrap == tiwmMaxNegToPos ? 0x7FFFFFFF : 0xFFFFFFFF ) - g_dtickTimeInjWrapOffset );

#endif  //  !EXCHANGE_DATACENTER_CACHE_TIMEBOMB

    if ( g_dtickTimeInjAccelerant == 1 )
    {
        //  if in "auto-inc" accelerant mode, then seed auto-inc variable
        g_tickTimeInjNO2                    = tickTimeInjStart; // just in case used
    }

    g_dtickTimeInjRealOffset                = tickTimeInjStart - TickOSTimeIGetTickCountI();

#else   //  !ENABLE_TIME_INJECTION

#pragma prefast(suppress:28159, "ESE has fixed all known 49-day-rollover problems, and GetTickCount64() is slower.")
    const TICK tickTimeInjStart             = GetTickCount();   // serves to test we are using unadjusted ticks below

#endif  //  ENABLE_TIME_INJECTION

    //  init the debugger time variables

    g_tickLastSysInit = TickOSTimeIGetTickCount();

    // spent 2 minutes in this function?  should be exceedingly rare / impossible

    if ( DtickDelta( tickTimeInjStart, g_tickLastSysInit ) > ( 2 * 60 * 1000 ) )
    {
        FireWall( "OsTimeInitHung" );
    }

    OSTimeIInitLast( g_tickLastSysInit );

    return JET_errSuccess;
}


