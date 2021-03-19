// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"


CPRINTFNULL g_cprintfNull;

CPRINTFSTDOUT g_cprintfStdout;

#ifdef DEBUG

CPRINTFDEBUG g_cprintfDEBUG;

#endif  //  DEBUG


//  ================================================================
CPRINTF* CPRINTFDBGOUT::PcprintfInstance()
//  ================================================================
{
    static CPRINTFDBGOUT cprintfDbgout;
    return &cprintfDbgout;
}

//  ================================================================
void __cdecl CPRINTFDBGOUT::operator()( const CHAR* szFormat, ... )
//  ================================================================
{
    const size_t    cchBuf          = 1024;
    CHAR            rgchBuf[ cchBuf ];

    //  print into a temp buffer, truncating the string if too large

    va_list arg_ptr;
    va_start( arg_ptr, szFormat );
    StringCbVPrintfA( rgchBuf, cchBuf, szFormat, arg_ptr );
    va_end( arg_ptr );

    //  output the string to the debugger

    OutputDebugString( rgchBuf );
}

CPRINTFFILE::CPRINTFFILE( const WCHAR* wszFile )
{
    //  open the file for append

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
    //  close the file

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

//  ================================================================
void __cdecl CPRINTFFILE::operator()( const CHAR* szFormat, ... )
//  ================================================================
{
    if ( HANDLE( m_hFile ) != INVALID_HANDLE_VALUE )
    {
        const size_t    cchBuf          = 1024;
        CHAR            rgchBuf[ cchBuf ];

        //  print into a temp buffer, truncating the string if too large

        va_list arg_ptr;
        va_start( arg_ptr, szFormat );
        StringCbVPrintfA( rgchBuf, cchBuf, szFormat, arg_ptr );
        va_end( arg_ptr );

        //  append the string to the file

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
    //  open the file for append

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
        // If we created this file fresh, we need to push in the Unicode byte order mark.
        Assert( GetLastError() == ERROR_SUCCESS );
        DWORD cbWritten;
        WCHAR rgwchBuf[] = { 0xFEFF }; // little endian byte order mark.
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
    //  close the file

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

// If _UNICODE is defined, then we only want a single function.
// If _UNICODE is not defined, we need two different functions.
#ifndef _UNICODE
//  ================================================================
void __cdecl CWPRINTFFILE::operator()( const _TCHAR* szFormat, ... )
//  ================================================================
{
    AssertSz( fFalse, "NYI" );
    // removed basically the same body as CPRINTFFILE::operator(), b/c didn't want to 
    // upconvert to WCHAR.
}
#endif

//  ================================================================
void __cdecl CWPRINTFFILE::operator()( const WCHAR* wszFormat, ... )
//  ================================================================
{
    if ( HANDLE( m_hFile ) != INVALID_HANDLE_VALUE
         && JET_errSuccess == m_errLast )
    {
        const size_t    cchBuf          = 1024;
        WCHAR           rgwchBuf[ cchBuf ]; // 2k on the stack, sheesh

        //  print into a temp buffer, truncating the string if too large

        va_list arg_ptr;
        va_start( arg_ptr, wszFormat );
        m_errLast = ErrFromStrsafeHr( StringCbVPrintfW( rgwchBuf, sizeof(rgwchBuf), wszFormat, arg_ptr ));
        if (JET_errSuccess != m_errLast)
        {
            // Stop writing after first error
            return;
        }
        va_end( arg_ptr );

#if DBG
        // UNDONE: Move this to something like VerifyOnlyDOSTextFileLineReturns() , maybe put in osfile, 
        // as we should be doing this in other places.
        for (WCHAR * wszT = wcschr(rgwchBuf, L'\n'); wszT; wszT = wcschr(wszT, L'\n') )
        {
            if ( (wszT == rgwchBuf) // this would mean rgchBuf[0] == L'\n', so that's bad.
                   ||
                 ((wszT + 1 > rgwchBuf) &&
                   (*(wszT-1)) != L'\r') ){
                AssertSz( fFalse, "We've detected someone trying to print a \\n to a file, only \\r\\n is supported as line return!" );
            }
            wszT++; // presumes NUL terminated to avoid running off end.
        }
#endif

        //  append the string to the file
        if (WAIT_OBJECT_0 == WaitForSingleObject( HANDLE( m_hMutex ), INFINITE ))
        {
            DWORD cbWritten;
            const LARGE_INTEGER ibOffset = { 0, 0 };
            if (   (!SetFilePointerEx( HANDLE( m_hFile ), ibOffset, NULL, FILE_END ))
                || (!WriteFile( HANDLE( m_hFile ), rgwchBuf, (ULONG)(LOSStrLengthW( rgwchBuf ) * sizeof( WCHAR )), &cbWritten, NULL )))
            {
                // Stop writing after first error
                m_errLast = ErrOSErrFromWin32Err(GetLastError());
            }
            ReleaseMutex( HANDLE( m_hMutex ) );
        }
        else
        {
            // Stop writing after first error
            m_errLast = ErrOSErrFromWin32Err(GetLastError());
        }
    }
}



//  ================================================================
CPRINTFTLSPREFIX::CPRINTFTLSPREFIX( CPRINTF* pcprintf, const CHAR* const szPrefix ) :
//  ================================================================
    m_cindent( 0 ),
    m_pcprintf( pcprintf ),
    m_szPrefix( szPrefix )
{
}


//  ================================================================
void __cdecl CPRINTFTLSPREFIX::operator()( const CHAR* szFormat, ... )
//  ================================================================
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

    //  print into a temp buffer, truncating the string if too large

    va_list arg_ptr;
    va_start( arg_ptr, szFormat );
    StringCbVPrintfA( pchBuf, cchBuf - ( pchBuf - rgchBuf ), szFormat, arg_ptr );
    va_end( arg_ptr );

    //  output the string to the next lower level

    (*m_pcprintf)( "%s", rgchBuf );
}

void CPRINTF::SetThreadPrintfPrefix( _In_ const _TCHAR * szPrefix )
{
    Postls()->szCprintfPrefix = szPrefix;
}

//  ================================================================
INLINE void CPRINTFTLSPREFIX::Indent()
//  ================================================================
{
}


//  ================================================================
INLINE void CPRINTFTLSPREFIX::Unindent()
//  ================================================================
{
}



//  retrieves the current width of stdout

DWORD UtilCprintfStdoutWidth()
{
    //  open stdout
    //
    HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
    if( INVALID_HANDLE_VALUE == hConsole )
    {
        return 80;
    }

    //  get attributes of console receiving stdout
    //
    CONSOLE_SCREEN_BUFFER_INFO csbi;
#ifdef MINIMAL_FUNCTIONALITY
    const BOOL fSuccess = fFalse;
#else
    const BOOL fSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );
#endif

    //  return width of console window or the standard 80 if unknown
    //
    return fSuccess ? csbi.dwMaximumWindowSize.X : 80;
}


//  post-terminate cprintf subsystem

void OSCprintfPostterm()
{
    //  nop
}

//  pre-init cprintf subsystem

BOOL FOSCprintfPreinit()
{
    //  nop

    return fTrue;
}


//  terminate cprintf subsystem

void OSCprintfTerm()
{
    //  nop
}

//  init cprintf subsystem

ERR ErrOSCprintfInit()
{
    //  nop

    return JET_errSuccess;
}


