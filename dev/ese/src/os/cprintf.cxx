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
    OSStrCbVFormatA( rgchBuf, cchBuf, szFormat, arg_ptr );
    va_end( arg_ptr );

    //  output the string to the debugger

    OutputDebugString( rgchBuf );
}


CPRINTFFILE::CPRINTFFILE( const WCHAR* wszFile, CPRINTFFILE::FILEENCODING feEncodingType )
{
    DWORD dwWin32Err;
    BOOL  fNewFile = fFalse;
    
    //  open the file for append
    
    m_hFile = INVALID_HANDLE_VALUE;
    m_hMutex = NULL;
    m_errLast = JET_errInvalidParameter;
    
    switch ( feEncodingType )
    {
        case FILEENCODING::ASCII:
        case FILEENCODING::UTF16:
            m_feEncodingType = feEncodingType;
            break;

        default:
            // Default any unrecognized encoding type to ASCII.
            m_feEncodingType = FILEENCODING::ASCII;
            Assert( fFalse );
            break;
    }

    if ( NULL == wszFile )
    {
        return;
    }

    m_hFile = ( void* )CreateFileW(
        wszFile,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL );

    dwWin32Err = GetLastError();

    if ( m_hFile == INVALID_HANDLE_VALUE )
    {
        m_errLast = ErrOSErrFromWin32Err( dwWin32Err );
        return;
    }

    switch ( dwWin32Err )
    {
    case ERROR_SUCCESS:
        fNewFile = fTrue;
        break;
        
    case ERROR_ALREADY_EXISTS:
        // Opened existing file.
        fNewFile = fFalse;
        break;

    default:
        // What error did we get where we also got a valid file handle?  We're going to
        // deal with it as a successful fresh file, which is what we've historically done.
        Assert( fFalse );
        fNewFile = fTrue;
        break;
    }
    
    SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

    m_hMutex = ( void* )CreateMutexW( NULL, FALSE, NULL );

    dwWin32Err = GetLastError();

    if ( m_hMutex == NULL )
    {
        m_errLast = ErrOSErrFromWin32Err( dwWin32Err );
        if ( m_errLast == JET_errSuccess )
        {
            // Force an error into errLast so we don't write to the file.
            m_errLast = JET_errInvalidParameter;
        }
        
        SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( HANDLE( m_hFile ) );
        m_hFile = INVALID_HANDLE_VALUE;
        return;
    }
    
    SetHandleInformation( HANDLE( m_hMutex ), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
    m_errLast = JET_errSuccess;

    // If we created this file fresh and we're writing Unicode, we need to push in the Unicode byte order mark.
    BYTE rgbBOMUTF16[] = { 0xFF, 0xFE };  // UTF-16little endian byte order mark.
    switch ( m_feEncodingType )
    {
        case FILEENCODING::UTF16:
            if ( fNewFile )
            {
                this->PutBytesInFile_( rgbBOMUTF16, _countof(rgbBOMUTF16) );
            }
            // else
            // {
            //     We could verify the BOM in an existing Unicode file.
            // }
            break;

        case FILEENCODING::ASCII:
            // if ( fNewFile )
            // {
            //     No BOM.
            // }
            // else
            // {
            //     We could verify the lack of BOM in an existing ASCII file.
            // }
            break;

        default:
            Assert( fFalse ); // Not possible.
            break;
    }
}

CPRINTFFILE::~CPRINTFFILE()
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

//  ================================================================
void __cdecl CPRINTFFILE::VerifyOnlyDOSTextFileLineReturns_( PCWSTR wsz )
//  ================================================================
{    
    // UNDONE: Maybe put in osfile, as we should be doing this in other places.
    for ( PCWSTR wszT = wcschr( wsz, L'\n' ); wszT; wszT = wcschr( wszT, L'\n' ) )
    {
        if ( ( wszT == wsz ) // this would mean rgchBuf[0] == L'\n', so that's bad.
             ||
             (( wszT + 1 > wsz ) &&
              ( *( wszT-1 ) ) != L'\r' ) ){
            AssertSz( fFalse, "We've detected someone trying to print a \\n to a file, only \\r\\n is supported as line return!" );
        }
        wszT++; // presumes NULL terminated to avoid running off end.
    }
}

//  ================================================================
void __cdecl CPRINTFFILE::PutBytesInFile_( BYTE *pb, ULONG cb )
//  ================================================================
{
    if ( WAIT_OBJECT_0 == WaitForSingleObject( HANDLE( m_hMutex ), INFINITE ) )
    {
        DWORD cbWritten;
        const LARGE_INTEGER ibOffset = { 0, 0 };

        // Stop writing after first error
        if ( !SetFilePointerEx( HANDLE( m_hFile ), ibOffset, NULL, FILE_END ) )
        {
            m_errLast = ErrOSErrFromWin32Err( GetLastError() );
            if ( m_errLast == JET_errSuccess )
            {
                // Force an error into errLast so we don't write to the file.
                m_errLast = JET_errInvalidParameter;
            }
        }
        else if ( !WriteFile(
                     HANDLE( m_hFile ),
                     pb,
                     cb,
                     &cbWritten,
                     NULL ) )
        {
            m_errLast = ErrOSErrFromWin32Err( GetLastError() );
            if ( m_errLast == JET_errSuccess )
            {
                // Force an error into errLast so we don't write to the file.
                m_errLast = JET_errInvalidParameter;
            }
        }
        ReleaseMutex( HANDLE( m_hMutex ) );
    }
    else
    {
        // Stop writing after first error
        m_errLast = ErrOSErrFromWin32Err( GetLastError() );
        if ( m_errLast == JET_errSuccess )
        {
            // Force an error into errLast so we don't write to the file.
            m_errLast = JET_errInvalidParameter;
        }
    }
}

//  ================================================================
void __cdecl CPRINTFFILE::operator()( const CHAR* szFormat, ... )
//  ================================================================
{
    if ( HANDLE( m_hFile ) == INVALID_HANDLE_VALUE )
    {
        return;
    }

    if ( JET_errSuccess != m_errLast )
    {
        return;
    }

    const size_t    cchBuf          = 1024;
    CHAR            rgchBuf[ cchBuf ];
    ULONG           cbData;

    //  print into a temp buffer, truncating the string if too large

    va_list arg_ptr;
    va_start( arg_ptr, szFormat );
    m_errLast = ErrOSStrCbVFormatA( rgchBuf, sizeof( rgchBuf ), szFormat, arg_ptr );
    va_end( arg_ptr );

    if ( JET_errSuccess != m_errLast )
    {
        // Stop writing after first error
        return;
    }

    cbData = LOSStrLengthA( rgchBuf );

    switch ( m_feEncodingType )
    {
        case FILEENCODING::UTF16:
            // A Unicode file, but you're using ASCII printing.  We can do that
            // simply by calling the WCHAR () operator.  Also.  Why are you doing
            // this?  Use Unicode printing.
            Expected( fFalse );
            operator()( L"%hs", rgchBuf );
            break;

        case FILEENCODING::ASCII:
            //  append the string to the file
            this->PutBytesInFile_( ( BYTE * )rgchBuf, cbData );
            break;

        default:
            Assert( fFalse ); // Not possible, constructor validates this member.
            break;
    }
}

//  ================================================================
void __cdecl CPRINTFFILE::operator()( const WCHAR* wszFormat, ... )
//  ================================================================
{
    if ( HANDLE( m_hFile ) == INVALID_HANDLE_VALUE )
    {
        return;
    }

    if ( JET_errSuccess != m_errLast )
    {
        return;
    }

    const size_t    cchBuf          = 1024;
    WCHAR           rgwchBuf[ cchBuf ]; // 2k on the stack, sheesh
    ULONG           cbData;

    //  print into a temp buffer, truncating the string if too large

    va_list arg_ptr;
    va_start( arg_ptr, wszFormat );
    m_errLast = ErrOSStrCbVFormatW( rgwchBuf, sizeof( rgwchBuf ), wszFormat, arg_ptr );
    va_end( arg_ptr );

    if ( JET_errSuccess != m_errLast )
    {
        // Stop writing after first error
        return;
    }


    cbData = LOSStrLengthW( rgwchBuf ) * sizeof( WCHAR );

    switch ( m_feEncodingType )
    {
        case FILEENCODING::UTF16:
#ifdef DEBUG
            this->VerifyOnlyDOSTextFileLineReturns_( rgwchBuf );
#endif
            this->PutBytesInFile_( ( BYTE * )rgwchBuf, cbData );
            break;

        case FILEENCODING::ASCII:
            // An ASCII file, but you're using Unicode printing.  We can do that
            // simply by calling the CHAR () operator, but it's potentially expensive
            // and lossy.
            Expected( fFalse );
            operator()( L"%ls", rgwchBuf );
            break;

        default:
            Assert( fFalse ); // Not possible, constructor validates this member.
            break;
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
    OSStrCbVFormatA( pchBuf, cchBuf - ( pchBuf - rgchBuf ), szFormat, arg_ptr );
    va_end( arg_ptr );

    //  output the string to the next lower level

    (*m_pcprintf)( "%s", rgchBuf );
}

void CPRINTF::SetThreadPrintfPrefix( _In_ const CHAR * szPrefix )
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


