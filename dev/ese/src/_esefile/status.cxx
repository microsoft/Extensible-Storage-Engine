// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "esefile.hxx"

static INT iLastPercentage;

void PrintWindowsError( const wchar_t * const szMessage )
{
    DWORD dwGLE = GetLastError();
    LPVOID lpMsgBuf = NULL;
    if( FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dwGLE,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0,
            NULL ) )
    {
        (void)fwprintf( stderr, L"%ls (%d), %ls", szMessage, dwGLE, (wchar_t *)lpMsgBuf );
    }

    LocalFree( lpMsgBuf );
}

void InitStatus( const wchar_t * const szOperation )
{
    const SIZE_T    cbOper      = wcslen( szOperation );
    const SIZE_T    cbPadding   = ( 51 - cbOper ) / 2;

    wprintf( L"          %*ls\r\n\r\n", (INT)(cbPadding+cbOper), szOperation );
    wprintf( L"          0    10   20   30   40   50   60   70   80   90  100\r\n" );
    wprintf( L"          |----|----|----|----|----|----|----|----|----|----|\r\n" );
    wprintf( L"          " );

    iLastPercentage = 0;
}


void UpdateStatus( const INT iPercentage )
{
    INT dPercentage = min( iPercentage, 100 ) - iLastPercentage;
    while ( dPercentage >= 2 )
    {
        wprintf( L"." );
        iLastPercentage += 2;
        dPercentage -= 2;
    }
}


void TermStatus()
{
    UpdateStatus( 100 );
    wprintf( L".\r\n" );
}
