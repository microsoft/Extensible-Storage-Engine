// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"


CPRINTFNULL g_cprintfNull;

CPRINTFSTDOUT g_cprintfStdout;

#ifdef DEBUG

CPRINTFDEBUG g_cprintfDEBUG;

#endif


CPRINTF* CPRINTFDBGOUT::PcprintfInstance()
{
    static CPRINTFDBGOUT cprintfDbgout;
    return &cprintfDbgout;
}

void __cdecl CPRINTFDBGOUT::operator()( const CHAR* szFormat, ... )
{
    const size_t    cchBuf          = 1024;
    CHAR            rgchBuf[ cchBuf ];


    va_list arg_ptr;
    va_start( arg_ptr, szFormat );
    StringCbVPrintfA( rgchBuf, cchBuf, szFormat, arg_ptr );
    va_end( arg_ptr );


    OutputDebugString( rgchBuf );
}

CPRINTFFILE::CPRINTFFILE( const WCHAR* wszFile )
{

    m_hFile = INVALID_HANDLE_VALUE;
    m_hMutex = NULL;

    if ( ( m_hFile = (void*)CreateFileW( wszFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ) ) == INVALID_HANDLE_VALUE )
    {
        return;
    }
    SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
    if ( !( m_hMutex = (void*)CreateMutexW( NULL, FALSE, NULL ) ) )
    {
        SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( HANDLE( m_hFile ) );
        m_hFile = INVALID_HANDLE_VALUE;
        return;
    }
    SetHandleInformation( HANDLE( m_hMutex ), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
}

CPRINTFFILE::~CPRINTFFILE()
{

    if ( m_hMutex )
    {
        SetHandleInformation( HANDLE( m_hMutex ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( HANDLE( m_hMutex ) );
        m_hMutex = NULL;
    }

    if ( m_hFile != INVALID_HANDLE_VALUE )
    {
        SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( HANDLE( m_hFile ) );
        m_hFile = INVALID_HANDLE_VALUE;
    }
}

void __cdecl CPRINTFFILE::operator()( const CHAR* szFormat, ... )
{
    if ( HANDLE( m_hFile ) != INVALID_HANDLE_VALUE )
    {
        const size_t    cchBuf          = 1024;
        CHAR            rgchBuf[ cchBuf ];


        va_list arg_ptr;
        va_start( arg_ptr, szFormat );
        StringCbVPrintfA( rgchBuf, cchBuf, szFormat, arg_ptr );
        va_end( arg_ptr );


        WaitForSingleObject( HANDLE( m_hMutex ), INFINITE );

        const LARGE_INTEGER ibOffset = { 0, 0 };
        SetFilePointerEx( HANDLE( m_hFile ), ibOffset, NULL, FILE_END );

        DWORD cbWritten;
        WriteFile( HANDLE( m_hFile ), rgchBuf, (ULONG)(_tcslen( rgchBuf ) * sizeof( _TCHAR )), &cbWritten, NULL );

        ReleaseMutex( HANDLE( m_hMutex ) );
    }
}

CWPRINTFFILE::CWPRINTFFILE( const WCHAR* wszFile )
{

    m_hFile = INVALID_HANDLE_VALUE;
    m_hMutex = NULL;
    m_errLast = JET_errInvalidParameter;

    if ( NULL == wszFile )
    {
        return;
    }

    if ( ( m_hFile = (void*)CreateFileW( wszFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ) ) == INVALID_HANDLE_VALUE )
    {
        m_errLast = ErrOSErrFromWin32Err(GetLastError());
        return;
    }
    if ( ERROR_ALREADY_EXISTS != GetLastError() )
    {
        Assert( GetLastError() == ERROR_SUCCESS );
        DWORD cbWritten;
        WCHAR rgwchBuf[] = { 0xFEFF };
        if (!WriteFile( HANDLE( m_hFile ), rgwchBuf, (ULONG)sizeof( WCHAR ), &cbWritten, NULL ))
        {
            m_errLast = ErrOSErrFromWin32Err(GetLastError());
            CloseHandle( HANDLE( m_hFile ) );
            m_hFile = INVALID_HANDLE_VALUE;
            return;
        }
    }
    SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
    if ( !( m_hMutex = (void*)CreateMutexW( NULL, FALSE, NULL ) ) )
    {
        m_errLast = ErrOSErrFromWin32Err(GetLastError());
        SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( HANDLE( m_hFile ) );
        m_hFile = INVALID_HANDLE_VALUE;
        return;
    }
    SetHandleInformation( HANDLE( m_hMutex ), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
    m_errLast = JET_errSuccess;
}

CWPRINTFFILE::~CWPRINTFFILE()
{

    m_errLast = JET_errInvalidParameter;

    if ( m_hMutex )
    {
        SetHandleInformation( HANDLE( m_hMutex ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( HANDLE( m_hMutex ) );
        m_hMutex = NULL;
    }

    if ( m_hFile != INVALID_HANDLE_VALUE )
    {
        SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( HANDLE( m_hFile ) );
        m_hFile = INVALID_HANDLE_VALUE;
    }
}

#ifndef _UNICODE
void __cdecl CWPRINTFFILE::operator()( const _TCHAR* szFormat, ... )
{
    AssertSz( fFalse, "NYI" );
}
#endif

void __cdecl CWPRINTFFILE::operator()( const WCHAR* wszFormat, ... )
{
    if ( HANDLE( m_hFile ) != INVALID_HANDLE_VALUE
         && JET_errSuccess == m_errLast )
    {
        const size_t    cchBuf          = 1024;
        WCHAR           rgwchBuf[ cchBuf ];


        va_list arg_ptr;
        va_start( arg_ptr, wszFormat );
        m_errLast = ErrFromStrsafeHr( StringCbVPrintfW( rgwchBuf, sizeof(rgwchBuf), wszFormat, arg_ptr ));
        if (JET_errSuccess != m_errLast)
        {
            return;
        }
        va_end( arg_ptr );

#if DBG
        for (WCHAR * wszT = wcschr(rgwchBuf, L'\n'); wszT; wszT = wcschr(wszT, L'\n') )
        {
            if ( (wszT == rgwchBuf)
                   ||
                 ((wszT + 1 > rgwchBuf) &&
                   (*(wszT-1)) != L'\r') ){
                AssertSz( fFalse, "We've detected someone trying to print a \\n to a file, only \\r\\n is supported as line return!" );
            }
            wszT++;
        }
#endif

        if (WAIT_OBJECT_0 == WaitForSingleObject( HANDLE( m_hMutex ), INFINITE ))
        {
            DWORD cbWritten;
            const LARGE_INTEGER ibOffset = { 0, 0 };
            if (   (!SetFilePointerEx( HANDLE( m_hFile ), ibOffset, NULL, FILE_END ))
                || (!WriteFile( HANDLE( m_hFile ), rgwchBuf, (ULONG)(wcslen( rgwchBuf ) * sizeof( WCHAR )), &cbWritten, NULL )))
            {
                m_errLast = ErrOSErrFromWin32Err(GetLastError());
            }
            ReleaseMutex( HANDLE( m_hMutex ) );
        }
        else
        {
            m_errLast = ErrOSErrFromWin32Err(GetLastError());
        }
    }
}



CPRINTFTLSPREFIX::CPRINTFTLSPREFIX( CPRINTF* pcprintf, const CHAR* const szPrefix ) :
    m_cindent( 0 ),
    m_pcprintf( pcprintf ),
    m_szPrefix( szPrefix )
{
}


void __cdecl CPRINTFTLSPREFIX::operator()( const CHAR* szFormat, ... )
{
    const size_t    cchBuf          = 1024;
    CHAR            rgchBuf[ cchBuf ];
    CHAR*           pchBuf          = rgchBuf;

    if( Postls()->szCprintfPrefix )
    {
        OSStrCbFormatA( pchBuf, cchBuf - ( pchBuf - rgchBuf ), "%s:\t%d:\t", Postls()->szCprintfPrefix, DwUtilThreadId() );
        pchBuf += strlen( pchBuf );
    }
    if( m_szPrefix )
    {
        OSStrCbFormatA( pchBuf, cchBuf - ( pchBuf - rgchBuf ), "%s", m_szPrefix );
        pchBuf += strlen( pchBuf );
    }


    va_list arg_ptr;
    va_start( arg_ptr, szFormat );
    StringCbVPrintfA( pchBuf, cchBuf - ( pchBuf - rgchBuf ), szFormat, arg_ptr );
    va_end( arg_ptr );


    (*m_pcprintf)( "%s", rgchBuf );
}

void CPRINTF::SetThreadPrintfPrefix( __in const _TCHAR * szPrefix )
{
    Postls()->szCprintfPrefix = szPrefix;
}

INLINE void CPRINTFTLSPREFIX::Indent()
{
}


INLINE void CPRINTFTLSPREFIX::Unindent()
{
}




DWORD UtilCprintfStdoutWidth()
{
    HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
    if( INVALID_HANDLE_VALUE == hConsole )
    {
        return 80;
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
#ifdef MINIMAL_FUNCTIONALITY
    const BOOL fSuccess = fFalse;
#else
    const BOOL fSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );
#endif

    return fSuccess ? csbi.dwMaximumWindowSize.X : 80;
}



void OSCprintfPostterm()
{
}


BOOL FOSCprintfPreinit()
{

    return fTrue;
}



void OSCprintfTerm()
{
}


ERR ErrOSCprintfInit()
{

    return JET_errSuccess;
}


