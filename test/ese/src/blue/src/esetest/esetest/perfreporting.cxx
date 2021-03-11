// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ese_common.hxx"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
#include <tchar.h>
#include <stdlib.h>

static BOOL
ILogToXml(
    __in HANDLE h,
    __in const char* szFormat,
    ...
)
{
    JET_ERR err     = JET_errSuccess;
    BOOL fReturn    = FALSE;
    DWORD dw;
    va_list valst;
    char szBuffer[ 100 * MAX_PATH + 1 ];
    
    va_start( valst, szFormat );
    vsprintf_s( szBuffer, _countof( szBuffer ), szFormat, valst );
    va_end( valst );
    CallNoWarning( !WriteFile( h, szBuffer, ( DWORD )strlen( szBuffer ), &dw, NULL ) );
    fReturn = TRUE;

Cleanup:
    return fReturn;
}

static BOOL
IReadTagFromXml(
    __in HANDLE h,
    __out char* szBuffer,
    __out BOOL* fMoreToRead
)
{
    JET_ERR err     = JET_errSuccess;
    char chBuffer;
    DWORD cbRead    = 0;
    BOOL fReturn    = FALSE;
    BOOL fLoop      = TRUE;
    BOOL fInTag     = FALSE;

    *fMoreToRead = FALSE;
    while ( fLoop )
    {
        CallNoWarning( !ReadFile( h, &chBuffer, sizeof( chBuffer ), &cbRead, NULL ) );
        if ( cbRead )
        {
            if ( fInTag )
            {
                if ( chBuffer == '>' )
                {
                    *( szBuffer++ ) = chBuffer;
                    *fMoreToRead = TRUE;
                    fLoop = FALSE;
                }
                else
                {
                    *( szBuffer++ ) = chBuffer;
                }
            }
            else
            {
                if ( chBuffer == '<' )
                {
                    *( szBuffer++ ) = chBuffer;
                    fInTag = TRUE;
                }
            }
        }
        else
        {
            fLoop = FALSE;
        }
    }

    if ( *fMoreToRead )
    {
        *szBuffer = '\0';
    }
    fReturn = TRUE;

Cleanup:
    return fReturn;
}

static BOOL
IIntializeXmlFilePointer(
    __in HANDLE h
)
{
    JET_ERR err             = JET_errSuccess;
    BOOL fLoop              = TRUE;
    BOOL fInRuns            = FALSE;
    BOOL fConsistent        = FALSE;
    LARGE_INTEGER cbOffset  = { 0 };
    const LARGE_INTEGER cb0 = { 0 };
    BOOL fReturn            = FALSE;
    BOOL fMoreToRead        = FALSE;
    char szTag[ MAX_PATH + 1 ];

    while ( fLoop )
    {
        CallNoWarning( !SetFilePointerEx( h, cb0, &cbOffset, FILE_CURRENT ) );
        CallNoWarning( !IReadTagFromXml( h, szTag, &fMoreToRead ) );
        if ( fMoreToRead )
        {
            if ( !strcmp( szTag, "<Counters>" ) )
            {
                fInRuns = TRUE;
            }
            else if ( !strcmp( szTag, "</Counters>" ) )
            {
                fLoop = FALSE;
                if ( fInRuns )
                {
                    fConsistent = TRUE;
                }
            }
        }
        else
        {
            fLoop = FALSE;
        }
    }

    if ( fConsistent )
    {
        CallNoWarning( !SetFilePointerEx( h, cbOffset, NULL, FILE_BEGIN ) );
        CallNoWarning( !SetEndOfFile( h ) );
        CallNoWarning( !ILogToXml( h, CRLF ) );
    }
    else
    {
        CallNoWarning( !SetFilePointerEx( h, cb0, NULL, FILE_BEGIN ) );
        CallNoWarning( !SetEndOfFile( h ) );
        CallNoWarning( !ILogToXml( h, "<Counters>" CRLF ) );
    }
    fReturn = TRUE;

Cleanup:
    return fReturn;
}

static BOOL
IFinalizeXmlFilePointer(
    __in HANDLE h
)
{
    JET_ERR err     = JET_errSuccess;
    BOOL fReturn    = FALSE;

    CallNoWarning( !ILogToXml( h, "</Counters>" CRLF ) );
    fReturn = TRUE;

Cleanup:
    return fReturn;
}

HANDLE
PerfReportingCreateFileA(
    __in PCSTR szFile
)
{
    WCHAR* wszFile  = EsetestWidenString( __FUNCTION__, szFile );
    HANDLE hReturn  = PerfReportingCreateFileW( wszFile );
    delete []wszFile;

    return hReturn;
}

HANDLE
PerfReportingCreateFileW(
    __in PCWSTR wszFile
)
{
    JET_ERR err     = JET_errSuccess;
    BOOL fReturn    = FALSE;
    HANDLE hReturn  = CreateFileW( wszFile,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL );

    CallNoWarning( !( hReturn != INVALID_HANDLE_VALUE ) );
    CallNoWarning( !IIntializeXmlFilePointer( hReturn ) );
    fReturn = TRUE;

Cleanup:
    if ( hReturn != INVALID_HANDLE_VALUE && !fReturn )
    {
        CloseHandle( hReturn );
        hReturn = INVALID_HANDLE_VALUE;
    }
    return hReturn;
}


BOOL
PerfReportingReportValueA(
    __in HANDLE     hPerfReporting,
    __in PCSTR      szCounterName,
    __in PCSTR      szCounterUnit,
    __in PCSTR      szPrintfFormat,
    __in BOOL       fHigherIsBetter,
    __in double     dblValue
)
{
    WCHAR* wszCounterName   = EsetestWidenString( __FUNCTION__, szCounterName );
    WCHAR* wszCounterUnit   = EsetestWidenString( __FUNCTION__, szCounterUnit );
    WCHAR* wszPrintfFormat  = EsetestWidenString( __FUNCTION__, szPrintfFormat );
    BOOL fReturn            = PerfReportingReportValueW( hPerfReporting,
                                                            wszCounterName,
                                                            wszCounterUnit,
                                                            wszPrintfFormat,
                                                            fHigherIsBetter,
                                                            dblValue );

    delete []wszCounterName;
    delete []wszCounterUnit;
    delete []wszPrintfFormat;

    return fReturn;
}

BOOL
PerfReportingReportValueW(
    __in HANDLE     hPerfReporting,
    __in PCWSTR     wszCounterName,
    __in PCWSTR     wszCounterUnit,
    __in PCWSTR     wszPrintfFormat,
    __in BOOL       fHigherIsBetter,
    __in double     dblValue
)
{
    JET_ERR err             = JET_errSuccess;
    BOOL fReturn            = FALSE;
    char szBufferXml[ 10 * MAX_PATH + 1 ];

    memset( szBufferXml, 0, sizeof( szBufferXml ) );
    CallVTrue( SUCCEEDED( StringCchPrintfA( szBufferXml, _countof( szBufferXml ),
                                            "\t<Counter Name=\"%S\">%s"
                                            "\t\t<Units HigherIsBetter=\"%s\">%S</Units>%s"
                                            "\t\t<Result>%S</Result>%s"
                                            "\t</Counter>%s",
                                            wszCounterName, CRLF,
                                            fHigherIsBetter ? "True" : "False", wszCounterUnit, CRLF,
                                            wszPrintfFormat, CRLF,
                                            CRLF ) ) );

    CallNoWarning( !ILogToXml( hPerfReporting, szBufferXml, dblValue ) );
    fReturn = TRUE;

Cleanup:
    return fReturn;
}

BOOL
PerfReportingCloseFile(
    __in HANDLE hPerfReporting
)
{
    JET_ERR err     = JET_errSuccess;
    BOOL fReturn    = FALSE;

    CallNoWarning( !IFinalizeXmlFilePointer( hPerfReporting ) );
    CallNoWarning( !CloseHandle( hPerfReporting ) );
    fReturn = TRUE;

Cleanup:
    return fReturn;
}

