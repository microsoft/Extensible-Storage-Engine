// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"


#include <math.h>
#define AFX_STATIC_DATA
#define AFX_STATIC
#define AFXAPI
#define DATE double
#define FALSE 0
#define TRUE 1



#define MAX_TIME_BUFFER_SIZE    128
#define MIN_DATE                (-657434L)
#define MAX_DATE                2958465L

#define HALF_SECOND  (1.0/172800.0)

AFX_STATIC_DATA INT _afxMonthDays[13] =
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};


AFX_STATIC BOOL AFXAPI _AfxOleDateFromTm(WORD wYear, WORD wMonth, WORD wDay,
    WORD wHour, WORD wMinute, WORD wSecond, DATE& dtDest)
{
    if (wYear > 9999 || wMonth < 1 || wMonth > 12)
        return FALSE;

    BOOL bLeapYear = ((wYear & 3) == 0) &&
        ((wYear % 100) != 0 || (wYear % 400) == 0);

    INT nDaysInMonth =
        _afxMonthDays[wMonth] - _afxMonthDays[wMonth-1] +
        ((bLeapYear && wDay == 29 && wMonth == 2) ? 1 : 0);

    if (wDay < 1 || wDay > nDaysInMonth ||
        wHour > 23 || wMinute > 59 ||
        wSecond > 59)
    {
        return FALSE;
    }

    LONG nDate;
    double dblTime;

    nDate = wYear*365L + wYear/4 - wYear/100 + wYear/400 +
        _afxMonthDays[wMonth-1] + wDay;

    if (wMonth <= 2 && bLeapYear)
        --nDate;

    nDate -= 693959L;

    dblTime = (((LONG)wHour * 3600L) +
        ((LONG)wMinute * 60L) +
        ((LONG)wSecond)) / 86400.;

    dtDest = (double) nDate + ((nDate >= 0) ? dblTime : -dblTime);

    return TRUE;
}

AFX_STATIC BOOL AFXAPI _AfxTmFromOleDate(DATE dtSrc, struct tm& tmDest)
{
    if (dtSrc > MAX_DATE || dtSrc < MIN_DATE)
        return FALSE;

    LONG nDays;
    LONG nDaysAbsolute;
    LONG nSecsInDay;
    LONG nMinutesInDay;

    LONG n400Years;
    LONG n400Century;
    LONG n4Years;
    LONG n4Day;
    LONG n4Yr;
    BOOL bLeap4 = TRUE;

    double dblDate = dtSrc;

    nDays = (LONG)dblDate;

    dblDate += ((dtSrc > 0.0) ? HALF_SECOND : -HALF_SECOND);

    nDaysAbsolute = (LONG)dblDate + 693959L;

    dblDate = fabs(dblDate);
    nSecsInDay = (LONG)((dblDate - floor(dblDate)) * 86400.);

    tmDest.tm_wday = (INT)((nDaysAbsolute - 1) % 7L) + 1;

    n400Years = (LONG)(nDaysAbsolute / 146097L);

    nDaysAbsolute %= 146097L;

    n400Century = (LONG)((nDaysAbsolute - 1) / 36524L);

    if (n400Century != 0)
    {
        nDaysAbsolute = (nDaysAbsolute - 1) % 36524L;

        n4Years = (LONG)((nDaysAbsolute + 1) / 1461L);

        if (n4Years != 0)
            n4Day = (LONG)((nDaysAbsolute + 1) % 1461L);
        else
        {
            bLeap4 = FALSE;
            n4Day = (LONG)nDaysAbsolute;
        }
    }
    else
    {
        n4Years = (LONG)(nDaysAbsolute / 1461L);
        n4Day = (LONG)(nDaysAbsolute % 1461L);
    }

    if (bLeap4)
    {
        n4Yr = (n4Day - 1) / 365;

        if (n4Yr != 0)
            n4Day = (n4Day - 1) % 365;
    }
    else
    {
        n4Yr = n4Day / 365;
        n4Day %= 365;
    }

    tmDest.tm_yday = (INT)n4Day + 1;
    tmDest.tm_year = n400Years * 400 + n400Century * 100 + n4Years * 4 + n4Yr;

    if (n4Yr == 0 && bLeap4)
    {
        if (n4Day == 59)
        {
            
            tmDest.tm_mon = 2;
            tmDest.tm_mday = 29;
            goto DoTime;
        }

        if (n4Day >= 60)
            --n4Day;
    }

    ++n4Day;

    for (tmDest.tm_mon = (n4Day >> 5) + 1;
        (unsigned)tmDest.tm_mon < 13 && n4Day > _afxMonthDays[tmDest.tm_mon]; tmDest.tm_mon++);

    tmDest.tm_mday = (INT)(n4Day - _afxMonthDays[tmDest.tm_mon-1]);

DoTime:
    if (nSecsInDay == 0)
        tmDest.tm_hour = tmDest.tm_min = tmDest.tm_sec = 0;
    else
    {
        tmDest.tm_sec = (INT)nSecsInDay % 60L;
        nMinutesInDay = nSecsInDay / 60L;
        tmDest.tm_min = (INT)nMinutesInDay % 60;
        tmDest.tm_hour = (INT)nMinutesInDay / 60;
    }

    return TRUE;
}

AFX_STATIC void AFXAPI _AfxTmConvertToStandardFormat(struct tm& tmSrc)
{
    tmSrc.tm_year -= 1900;
    tmSrc.tm_mon -= 1;
    tmSrc.tm_wday -= 1;
    tmSrc.tm_yday -= 1;
}

AFX_STATIC double AFXAPI _AfxDoubleFromDate(DATE dt)
{
    if (dt >= 0)
        return dt;

    double temp = ceil(dt);
    return temp - (dt - temp);
}

AFX_STATIC DATE AFXAPI _AfxDateFromDouble(double dbl)
{
    if (dbl >= 0)
        return dbl;

    double temp = floor(dbl);
    return temp + (temp - dbl);
}




void UtilGetDateTime( DATESERIAL *pdt )
{

    DATETIME dt;
    UtilGetCurrentDateTime( &dt );


    if ( !_AfxOleDateFromTm(    WORD( dt.year ),
                                WORD( dt.month ),
                                WORD( dt.day ),
                                WORD( dt.hour ),
                                WORD( dt.minute ),
                                WORD( dt.second ),
                                *pdt ) )
    {
        AssertSz( fFalse, "Invalid date:  Is it really 10000 A.D. already?" );
    }
}



void OSUTimeTerm()
{
}


ERR ErrOSUTimeInit()
{

    return JET_errSuccess;
}


