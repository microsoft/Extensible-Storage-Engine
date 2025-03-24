// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//================================================================
// Disk helper functions.
//================================================================
//

#include "ese_common.hxx"
#include <strsafe.h>
#include <stdlib.h>

// =============================================================================
DWORD GetAttributeListSizeA(
    __in PCSTR const szFilename,
    __out ULONG64* const pcbAttributeList
)
// =============================================================================
{
    PCWSTR const wszFilename    = EsetestWidenString( __FUNCTION__, szFilename );
    const DWORD dwReturn        = GetAttributeListSizeW( wszFilename, pcbAttributeList );
    delete []wszFilename;

    return dwReturn;
}

// =============================================================================
DWORD GetAttributeListSizeW(
    __in PCWSTR const wszFilename,
    __out ULONG64* const pcbAttributeList
)
// =============================================================================
{
    DWORD           dwGLE = 0;
    HANDLE          hFile = INVALID_HANDLE_VALUE;
    WCHAR           wszAttrList[ MAX_PATH ];
    LARGE_INTEGER   li;

    AssertM( pcbAttributeList != NULL );

    wcscpy_s( wszAttrList, wszFilename );
    wcscat_s( wszAttrList, L"::$ATTRIBUTE_LIST" );
    hFile = CreateFileW( wszAttrList, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        dwGLE = GetLastError();
        goto Cleanup;
    }

    if ( !GetFileSizeEx( hFile, &li ) )
    {
        dwGLE = GetLastError();
        goto Cleanup;
    }

    *pcbAttributeList = (ULONG64)li.QuadPart;

Cleanup:
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }

    return dwGLE;
}

// =============================================================================
DWORD GetExtentCountA(
    __in PCSTR const szFilename,
    __out DWORD* const pcExtent
)
// =============================================================================
{
    PCWSTR const wszFilename    = EsetestWidenString( __FUNCTION__, szFilename );
    const DWORD dwReturn        = GetExtentCountW( wszFilename, pcExtent );
    delete []wszFilename;

    return dwReturn;
}

// =============================================================================
DWORD GetExtentCountW(
    __in PCWSTR const wszFilename,
    __out DWORD* const pcExtent
)
// =============================================================================
{
    DWORD                       dwGLE = 0;
    HANDLE                      hFile = INVALID_HANDLE_VALUE;
    void*                       pData = NULL;
    ULONG                       cExtent = 0;
    STARTING_VCN_INPUT_BUFFER   startingVCN = { 0 };
    ULONG                       dataSize = 0x100000;
    ULONG                       retSize = 0;
    HANDLE                      hProcHeap = INVALID_HANDLE_VALUE;

    AssertM( pcExtent != NULL );

    hFile = CreateFileW( wszFilename, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        dwGLE = GetLastError();
        goto Cleanup;
    }

    hProcHeap = GetProcessHeap();
    pData = HeapAlloc( hProcHeap, 0, dataSize );
    if ( pData == NULL )
    {
        // can't use GetLastError for HeapAlloc, oddly enough
        dwGLE = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    
    do
    {
        dwGLE = 0;
        if ( !DeviceIoControl(
            hFile,
            FSCTL_GET_RETRIEVAL_POINTERS,
            &startingVCN, sizeof( startingVCN ),
            (RETRIEVAL_POINTERS_BUFFER *) pData, dataSize,
            &retSize, NULL ) )
        {
            dwGLE = GetLastError();
        }

        AssertM( dwGLE != ERROR_INSUFFICIENT_BUFFER );  // shouldnt return that with a 1MB buffer

        if ( dwGLE == ERROR_MORE_DATA || dwGLE == 0 )
        {
            RETRIEVAL_POINTERS_BUFFER *pRetPtrs = (RETRIEVAL_POINTERS_BUFFER *) pData;
            cExtent += pRetPtrs->ExtentCount;
            startingVCN.StartingVcn = pRetPtrs->Extents[ pRetPtrs->ExtentCount - 1 ].NextVcn;
        }
    } while ( dwGLE == ERROR_MORE_DATA );

    *pcExtent = cExtent;
    
Cleanup:
    if ( pData )
    {
        HeapFree( hProcHeap, 0, pData );
    }
    
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }
    
    return dwGLE;
}

// =============================================================================
DISK_GEOMETRY* GetDiskGeometry(
    __in unsigned long ulDisk
)
// =============================================================================
{
    HANDLE hDevice;
    BOOL bResult        = FALSE;
    DWORD dwBuffer;
    char szDevice[ MAX_PATH + 1 ];
    DISK_GEOMETRY* pdg  = NULL;

    StringCchPrintfA( szDevice,
                        _countof( szDevice ),
                        "\\\\.\\PhysicalDrive%lu",
                        ulDisk );
    hDevice = CreateFile( szDevice,
                            FILE_ANY_ACCESS,
                            FILE_SHARE_READ |
                            FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );
    if ( hDevice == INVALID_HANDLE_VALUE ){
        goto Cleanup;
    }
    pdg = new DISK_GEOMETRY;
    bResult = DeviceIoControl( hDevice,
                                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                NULL, 0,
                                pdg, sizeof( *pdg ),
                                &dwBuffer,
                                NULL );
    CloseHandle( hDevice );

Cleanup:
    if ( !bResult ){
        delete pdg;
        pdg = NULL;
    }

    return pdg;
}

// =============================================================================
DISK_CACHE_INFORMATION* GetDiskCacheInfo(
    __in unsigned long ulDisk
)
// =============================================================================
{
    HANDLE hDevice;
    BOOL bResult                = FALSE;
    DWORD dwBuffer;
    char szDevice[ MAX_PATH + 1 ];
    DISK_CACHE_INFORMATION* pdc = NULL;

    StringCchPrintfA( szDevice,
                        _countof( szDevice ),
                        "\\\\.\\PhysicalDrive%lu",
                        ulDisk );
    hDevice = CreateFile( szDevice,
                            FILE_READ_ACCESS,
                            FILE_SHARE_READ |
                            FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );
    if ( hDevice == INVALID_HANDLE_VALUE ){
        goto Cleanup;
    }
    pdc = new DISK_CACHE_INFORMATION;
    bResult = DeviceIoControl( hDevice,
                                IOCTL_DISK_GET_CACHE_INFORMATION,
                                NULL, 0,
                                pdc, sizeof( *pdc ),
                                &dwBuffer,
                                NULL );
    CloseHandle( hDevice );

Cleanup:
    if ( !bResult ){
        delete pdc;
        pdc = NULL;
    }

    return pdc;
}

// =============================================================================
BOOL SetDiskCacheInfo(
    __in const DISK_CACHE_INFORMATION* const pdc,
    __in unsigned long ulDisk
)
// =============================================================================
{
    HANDLE hDevice;
    BOOL bResult                = FALSE;
    DWORD dwBuffer;
    char szDevice[ MAX_PATH + 1 ];

    StringCchPrintfA( szDevice,
                        _countof( szDevice ),
                        "\\\\.\\PhysicalDrive%lu",
                        ulDisk );
    hDevice = CreateFile( szDevice,
                            FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                            FILE_SHARE_READ |
                            FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );
    if ( hDevice == INVALID_HANDLE_VALUE ){
        goto Cleanup;
    }
    bResult = DeviceIoControl( hDevice,
                                IOCTL_DISK_SET_CACHE_INFORMATION,
                                ( LPVOID )pdc, sizeof( *pdc ),
                                NULL, 0,
                                &dwBuffer,
                                NULL );

    CloseHandle( hDevice );
Cleanup:
    return bResult;
}

// =============================================================================
BOOL GetDiskReadCache(
    __in unsigned long ulDisk,
    __out BOOLEAN* pfOn
)
// =============================================================================
{
    BOOL bResult = FALSE;
    DISK_CACHE_INFORMATION *pdc = GetDiskCacheInfo( ulDisk );

    if ( !pdc ){
        goto Cleanup;
    }
    *pfOn = pdc->ReadCacheEnabled;

    bResult = TRUE;
Cleanup:
    delete pdc;
    return bResult;
}

// =============================================================================
BOOL SetDiskReadCache(
    __in unsigned long ulDisk,
    __in BOOLEAN fOn
)
// =============================================================================
{
    BOOL bResult = FALSE;
    DISK_CACHE_INFORMATION *pdc = GetDiskCacheInfo( ulDisk );

    if ( !pdc ){
        goto Cleanup;
    }
    pdc->ReadCacheEnabled = fOn;
    if ( !SetDiskCacheInfo( pdc, ulDisk ) ){
        goto Cleanup;
    }

    bResult = TRUE;
Cleanup:
    delete pdc;
    return bResult;
}

// =============================================================================
BOOL GetDiskWriteCache(
    __in unsigned long ulDisk,
    __out BOOLEAN* pfOn
)
// =============================================================================
{
    BOOL bResult = FALSE;
    DISK_CACHE_INFORMATION *pdc = GetDiskCacheInfo( ulDisk );

    if ( !pdc ){
        goto Cleanup;
    }
    *pfOn = pdc->WriteCacheEnabled;

    bResult = TRUE;
Cleanup:
    delete pdc;
    return bResult;
}

// =============================================================================
BOOL SetDiskWriteCache(
    __in unsigned long ulDisk,
    __in BOOLEAN fOn
)
// =============================================================================
{
    BOOL bResult = FALSE;
    DISK_CACHE_INFORMATION *pdc = GetDiskCacheInfo( ulDisk );

    if ( !pdc ){
        goto Cleanup;
    }
    pdc->WriteCacheEnabled = fOn;
    if ( !SetDiskCacheInfo( pdc, ulDisk ) ){
        goto Cleanup;
    }

    bResult = TRUE;
Cleanup:
    delete pdc;
    return bResult;
}


//  2008/11/26 - SOMEONE: I believe this is an undocumented CTL_CODE for DeviceIoControl(). I've figured it out
//  by attaching the disk management console to a debugger and watching the data being passed-in/returned-from
//  DeviceIoControl().
//  We can read/change the regular cache configuration by using IOCTL_DISK_GET_CACHE_INFORMATION (function 0x0035)
//  and IOCTL_DISK_SET_CACHE_INFORMATION (function 0x0036) and this is documented.
//  The reason I believe the "advanced cache configuration" is hidden is because  we can find functions 0x0035 and
//  0x0036, but we can't find 0x0038 and 0x0039 which I observed are the CTL_CODEs used to read/change that
//  option, respectively.

#define IOCTL_DISK_GET_CACHE_INFORMATION_ADVANCED    CTL_CODE(IOCTL_DISK_BASE, 0x0038, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_SET_CACHE_INFORMATION_ADVANCED    CTL_CODE(IOCTL_DISK_BASE, 0x0039, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

typedef struct _DISK_CACHE_INFORMATION_ADVANCED {
    DWORD cbStruct;
    BYTE rgNotSureWhatItIs1[ 4 ];
    BYTE AdvancedWriteCacheEnabled;
    BYTE rgNotSureWhatItIs2[ 3 ];
} DISK_CACHE_INFORMATION_ADVANCED, *PDISK_CACHE_INFORMATION_ADVANCED;


// =============================================================================
static DISK_CACHE_INFORMATION_ADVANCED* IGetAdvancedDiskCacheInfo(
    __in unsigned long ulDisk
)
// =============================================================================
{
    HANDLE hDevice;
    BOOL bResult                            = FALSE;
    DWORD dwBuffer;
    char szDevice[ MAX_PATH + 1 ];
    DISK_CACHE_INFORMATION_ADVANCED* pdca   = NULL;

    StringCchPrintfA( szDevice,
                        _countof( szDevice ),
                        "\\\\.\\PhysicalDrive%lu",
                        ulDisk );
    hDevice = CreateFile( szDevice,
                            FILE_READ_ACCESS,
                            FILE_SHARE_READ |
                            FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );
    if ( hDevice == INVALID_HANDLE_VALUE ){
        goto Cleanup;
    }
    pdca = new DISK_CACHE_INFORMATION_ADVANCED;
    bResult = DeviceIoControl( hDevice,
                                IOCTL_DISK_GET_CACHE_INFORMATION_ADVANCED,
                                NULL, 0,
                                pdca, sizeof( *pdca ),
                                &dwBuffer,
                                NULL );
    CloseHandle( hDevice );

Cleanup:
    if ( !bResult ){
        delete pdca;
        pdca = NULL;
    }

    return pdca;
}

// =============================================================================
static BOOL ISetAdvancedDiskCacheInfo(
    __in const DISK_CACHE_INFORMATION_ADVANCED* const pdca,
    __in unsigned long ulDisk
)
// =============================================================================
{
    HANDLE hDevice;
    BOOL bResult                = FALSE;
    DWORD dwBuffer;
    char szDevice[ MAX_PATH + 1 ];

    StringCchPrintfA( szDevice,
                        _countof( szDevice ),
                        "\\\\.\\PhysicalDrive%lu",
                        ulDisk );
    hDevice = CreateFile( szDevice,
                            FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                            FILE_SHARE_READ |
                            FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );
    if ( hDevice == INVALID_HANDLE_VALUE ){
        goto Cleanup;
    }
    bResult = DeviceIoControl( hDevice,
                                IOCTL_DISK_SET_CACHE_INFORMATION_ADVANCED,
                                ( LPVOID )pdca, sizeof( *pdca ),
                                NULL, 0,
                                &dwBuffer,
                                NULL );

    CloseHandle( hDevice );
Cleanup:
    return bResult;
}

// =============================================================================
BOOL GetAdvancedDiskWriteCache(
    __in unsigned long ulDisk,
    __out BOOLEAN* pfOn
)
// =============================================================================
{
    BOOL bResult = FALSE;
    DISK_CACHE_INFORMATION_ADVANCED *pdca = IGetAdvancedDiskCacheInfo( ulDisk );

    if ( !pdca ){
        goto Cleanup;
    }
    *pfOn = pdca->AdvancedWriteCacheEnabled;

    bResult = TRUE;
Cleanup:
    delete pdca;
    return bResult;
}

// =============================================================================
BOOL SetAdvancedDiskWriteCache(
    __in unsigned long ulDisk,
    __in BOOLEAN fOn
)
// =============================================================================
{
    BOOL bResult = FALSE;
    DISK_CACHE_INFORMATION_ADVANCED *pdca = IGetAdvancedDiskCacheInfo( ulDisk );

    if ( !pdca ){
        goto Cleanup;
    }
    pdca->AdvancedWriteCacheEnabled = (BYTE)fOn;
    if ( !ISetAdvancedDiskCacheInfo( pdca, ulDisk ) ){
        goto Cleanup;
    }

    bResult = TRUE;
Cleanup:
    delete pdca;
    return bResult;
}


// =============================================================================
BOOL RunDiskPart(
    __in PCSTR const szDiskPartScript
)
// =============================================================================
{
    const DWORD cRetry = 10;
    bool fReturn = false;
    DWORD dwExitCode = ERROR_SUCCESS;
    char szCmd[ MAX_PATH + 1 ];
    STARTUPINFOA si = { 0 };
    PROCESS_INFORMATION pi = { 0 };

    StringCchPrintfA( szCmd, _countof( szCmd ), "DiskPart.exe /s %hs", szDiskPartScript );
    
    //  DiskPart.exe mysteriously fails sometimes. Adding a hacky retry logic.
    for ( DWORD iRetry = 0; iRetry < cRetry; iRetry++ )
    {
        dwExitCode = ERROR_SUCCESS;
        ZeroMemory( &si, sizeof(si) );
        si.cb = sizeof(si);
        ZeroMemory( &pi, sizeof(pi) );

        if ( !CreateProcessA( NULL, szCmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi ) )
        {
            pi.hProcess = NULL;
            pi.hThread = NULL;
            fReturn = false;
            dwExitCode = GetLastError();
            break;
        }
        
        WaitForSingleObject( pi.hProcess, INFINITE );

        if ( !GetExitCodeProcess( pi.hProcess, &dwExitCode ) )
        {
            fReturn = false;
            dwExitCode = GetLastError();
            break;
        }
        else if ( dwExitCode == ERROR_SUCCESS  )
        {
            fReturn = true;
            break;
        }
    }

    CloseHandleP( &pi.hProcess );
    CloseHandleP( &pi.hThread );

    if ( !fReturn )
    {
        SetLastError( dwExitCode );
    }

    return fReturn;
}

// =============================================================================
BOOL CreateVirtualDisk(
    __in PCWSTR const wszVhdFilePath,
    __in const ULONG cmbSize,
    __in const WCHAR* const wszMountPoint
)
// =============================================================================
{
    char szFile[ MAX_PATH + 1 ];
    BOOL bReturn = TRUE;
    FILE *stream = NULL;

    StringCchPrintfA( szFile, _countof( szFile ), "DiskPartScript-%lu-%lu.dps", ( DWORD )GetCurrentProcessId(), ( DWORD )GetCurrentThreadId() );
    if ( ( stream = fopen( szFile, "w" ) ) == NULL )
    {
        bReturn = FALSE;
        goto Cleanup;
    }
    fwprintf( stream, L"CREATE VDISK FILE=\"%ws\" MAXIMUM=%lu" WCRLF, wszVhdFilePath, cmbSize );
    fwprintf( stream, L"SELECT VDISK FILE=\"%ws\"" WCRLF, wszVhdFilePath );
    fwprintf( stream, L"ATTACH VDISK" WCRLF );
    fwprintf( stream, L"CREATE PARTITION PRIMARY" WCRLF );
    fwprintf( stream, L"FORMAT QUICK OVERRIDE" WCRLF );
    fwprintf( stream, L"ASSIGN MOUNT=\"%ws\"" WCRLF, wszMountPoint );
    fflush( stream );
    fclose( stream );
    stream = NULL;

    if ( !RunDiskPart( szFile ) )
    {
        bReturn = FALSE;
        goto Cleanup;
    }

Cleanup:
    if ( stream )
    {
        fclose( stream );
    }
    
    DeleteFileA( szFile );

    return bReturn;
}

// =============================================================================
BOOL DestroyVirtualDisk(
    __in PCWSTR const wszVhdFilePath
)
// =============================================================================
{
    char szFile[ MAX_PATH + 1 ];
    BOOL bReturn = TRUE;
    FILE *stream = NULL;

    StringCchPrintfA( szFile, _countof( szFile ), "DiskPartScript-%lu-%lu.dps", ( DWORD )GetCurrentProcessId(), ( DWORD )GetCurrentThreadId() );
    if ( ( stream = fopen( szFile, "w" ) ) == NULL )
    {
        bReturn = FALSE;
        goto Cleanup;
    }
    fwprintf( stream, L"SELECT VDISK FILE=\"%ws\"" WCRLF, wszVhdFilePath );
    fwprintf( stream, L"DETACH VDISK" WCRLF );
    fflush( stream );
    fclose( stream );
    stream = NULL;

    if ( !RunDiskPart( szFile ) )
    {
        bReturn = FALSE;
        goto Cleanup;
    }

Cleanup:
    if ( stream )
    {
        fclose( stream );
    }
    
    DeleteFileA( szFile );

    return bReturn;
}

// =============================================================================
VOLUME_DISK_EXTENTS* GetVolumeExtents(
    __in WCHAR** pwszLogicalVolume
)
// =============================================================================
{
    HANDLE hDevice;
    BOOL bResult        = FALSE;
    DWORD dwBuffer;
    WCHAR wszDevice[ MAX_PATH + 1 ];
    PBYTE pBuffer       = NULL;
    DWORD dwBufferSize;
    size_t cch1, cch2;
    DWORD dwError = ERROR_SUCCESS;

    StringCchPrintfW( wszDevice,
                        _countof( wszDevice ),
                        L"\\\\.\\%s",
                        *pwszLogicalVolume );
    cch1 = wcslen( wszDevice );
    if ( ( cch1 > 0 ) && ( wszDevice[ cch1 - 1 ] == L'\\' ) ){
        wszDevice[ cch1 - 1 ] = L'\0';
    }
    cch2 = wcslen( *pwszLogicalVolume );
    *pwszLogicalVolume += ( cch2 + 1 );
    hDevice = CreateFileW( wszDevice,
                            FILE_ANY_ACCESS,
                            FILE_SHARE_READ |
                            FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );
    if ( hDevice == INVALID_HANDLE_VALUE ){
        tprintf( "0: %lu" CRLF, GetLastError() );
        goto Cleanup;
    }
    dwBufferSize = sizeof( VOLUME_DISK_EXTENTS );
    do{
        delete []pBuffer;
        pBuffer = new BYTE[ dwBufferSize ];
        bResult = DeviceIoControl( hDevice,
                                    IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                    NULL, 0,
                                    pBuffer, dwBufferSize,
                                    &dwBuffer,
                                    NULL );
        dwError = ERROR_SUCCESS;
        if ( !bResult ){
            dwError = GetLastError();
            if ( dwError == ERROR_MORE_DATA ){
                AssertM( ( ( VOLUME_DISK_EXTENTS* )pBuffer )->NumberOfDiskExtents > 1 );
                dwBufferSize += ( ( ( VOLUME_DISK_EXTENTS* )pBuffer )->NumberOfDiskExtents - 1 ) * sizeof( DISK_EXTENT );
            }
        }
    } while( !bResult && dwError == ERROR_MORE_DATA );
    CloseHandle( hDevice );

Cleanup:
    if ( !bResult ){
        delete []pBuffer;
        pBuffer = NULL;
    }

    return ( VOLUME_DISK_EXTENTS* )pBuffer;
}

// =============================================================================
BOOL DefragmentVolumeA(
    __in PCSTR const szVolumeName
)
// =============================================================================
{
    PCWSTR const wszVolumeName  = EsetestWidenString( __FUNCTION__, szVolumeName );
    const BOOL fReturn          = DefragmentVolumeW( wszVolumeName );
    delete []wszVolumeName;

    return fReturn;
}

// =============================================================================
BOOL DefragmentVolumeW(
    __in PCWSTR const wszVolumeName
)
// =============================================================================
{
    WCHAR wszVolumeNameT[ MAX_PATH ] = { L'\0' };
    BOOL fReturn = FALSE; 

    // The user may have passed a file name, so let's retrieve the actual volume name to defrag.
    if ( !GetVolumePathNameW( wszVolumeName, wszVolumeNameT, _countof( wszVolumeNameT ) ) )
    {
        goto Cleanup;
    }

    // Do the job.
    WCHAR wszCmd[ MAX_PATH ];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof( si ) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof( pi ) );
    
    StringCchPrintfW( wszCmd, _countof( wszCmd ), L"defrag.exe %ls", wszVolumeNameT );
    if ( !CreateProcessW( NULL, wszCmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi ) ){
        goto Cleanup;
    }
    WaitForSingleObject( pi.hProcess, INFINITE );
    DWORD dwExitCode = ERROR_SUCCESS;
    if ( !GetExitCodeProcess( pi.hProcess, &dwExitCode ) || dwExitCode != ERROR_SUCCESS ){
        goto Cleanup;
    }

    fReturn = TRUE;

Cleanup:
    return fReturn;
}

// =============================================================================
DRIVE_LAYOUT_INFORMATION_EX* GetDiskPartitions(
    __in unsigned long ulDisk
)
// =============================================================================
{
    HANDLE hDevice;
    BOOL bResult        = FALSE;
    DWORD dwBuffer, dwBufferSize;
    char szDevice[ MAX_PATH + 1 ];
    BYTE* pdl           = NULL;
    DWORD dwError = ERROR_SUCCESS;

    StringCchPrintfA( szDevice,
                        _countof( szDevice ),
                        "\\\\.\\PhysicalDrive%lu",
                        ulDisk );
    hDevice = CreateFile( szDevice,
                            FILE_ANY_ACCESS,
                            FILE_SHARE_READ |
                            FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );
    if ( hDevice == INVALID_HANDLE_VALUE ){
        goto Cleanup;
    }

    dwBufferSize = sizeof( DRIVE_LAYOUT_INFORMATION_EX );
    do{
        delete []pdl;
        pdl = new BYTE[ dwBufferSize ];
        bResult = DeviceIoControl( hDevice,
                                    IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                    NULL, 0,
                                    pdl, dwBufferSize,
                                    &dwBuffer,
                                    NULL );
        dwError = ERROR_SUCCESS;
        if ( !bResult ){
            dwError = GetLastError();
            if ( dwError == ERROR_INSUFFICIENT_BUFFER ){
                dwBufferSize += sizeof( PARTITION_INFORMATION_EX );
            }
        }
    } while( !bResult &&  dwError == ERROR_INSUFFICIENT_BUFFER );
    CloseHandle( hDevice );

Cleanup:
    if ( !bResult ){
        delete []pdl;
        pdl = NULL;
    }

    return ( DRIVE_LAYOUT_INFORMATION_EX* )pdl;
}

// =============================================================================
BOOL OnlineDisk(
    __in unsigned long ulDisk
)
// =============================================================================
{
    char szFile[ MAX_PATH + 1 ];
    BOOL bReturn = TRUE;
    FILE *stream = NULL;

    StringCchPrintfA( szFile, _countof( szFile ), "DiskPartScript-%lu-%lu.dps", ( DWORD )GetCurrentProcessId(), ( DWORD )GetCurrentThreadId() );
    if ( ( stream = fopen( szFile, "w" ) ) == NULL ){
        bReturn = FALSE;
        goto Cleanup;
    }
    fprintf( stream, "SELECT DISK %lu" CRLF, ulDisk );
    OSVERSIONINFOW osInfo;
    osInfo.dwOSVersionInfoSize = static_cast< DWORD >( sizeof( osInfo ) );
    if ( !GetVersionExW( &osInfo ) ){
        // Assume it's Vista/W2k8.
        osInfo.dwMajorVersion = 6;
    }
    if ( osInfo.dwMajorVersion >= 6 ){
        fprintf( stream, "ONLINE DISK NOERR" CRLF );
        fprintf( stream, "ATTRIBUTES DISK CLEAR READONLY NOERR" CRLF );
    }
    else{
        fprintf( stream, "ONLINE NOERR" CRLF );
    }
    fflush( stream );
    fclose( stream );
    stream = NULL;

    if ( !RunDiskPart( szFile ) ){
        bReturn = FALSE;
        goto Cleanup;
    }

Cleanup:
    if ( stream ){
        fclose( stream );
    }
    DeleteFileA( szFile );

    return bReturn;
}

// =============================================================================
BOOL DeleteDiskPartition(
    __in unsigned long ulDisk,
    __in unsigned long ulPartition
)
// =============================================================================
{
    char szFile[ MAX_PATH + 1 ];
    BOOL bReturn = TRUE;
    FILE *stream = NULL;

    StringCchPrintfA( szFile, _countof( szFile ), "DiskPartScript-%lu-%lu.dps", ( DWORD )GetCurrentProcessId(), ( DWORD )GetCurrentThreadId() );
    if ( ( stream = fopen( szFile, "w" ) ) == NULL ){
        bReturn = FALSE;
        goto Cleanup;
    }
    fprintf( stream, "SELECT DISK %lu" CRLF, ulDisk );
    fprintf( stream, "SELECT PARTITION %lu" CRLF, ulPartition );
    fprintf( stream, "DELETE PARTITION OVERRIDE" CRLF );
    fflush( stream );
    fclose( stream );
    stream = NULL;

    if ( !RunDiskPart( szFile ) ){
        bReturn = FALSE;
        goto Cleanup;
    }

Cleanup:
    if ( stream ){
        fclose( stream );
    }
    DeleteFileA( szFile );

    return bReturn;
}

// =============================================================================
BOOL DeleteDiskPartitions(
    __in unsigned long ulDisk
)
// =============================================================================
{
    DRIVE_LAYOUT_INFORMATION_EX* pdl;
    BOOL bReturn = TRUE;

    // Partition enumeration.
    pdl = GetDiskPartitions( ulDisk );
    if ( pdl && pdl->PartitionCount > 0 ){
        for ( unsigned long ulPartition = pdl->PartitionCount ; ulPartition > 0 ; ulPartition-- ){
            // Delete only valid partitions.
            DWORD dwPartitionNumber = pdl->PartitionEntry[ ulPartition - 1 ].PartitionNumber;
            if ( dwPartitionNumber > 0 ){
                bReturn = bReturn && DeleteDiskPartition( ulDisk, dwPartitionNumber );
            }
        }
    }
    if ( !pdl ){
        bReturn = FALSE;
    }
    delete []( BYTE* )pdl;

    return bReturn;
}

// =============================================================================
BOOL CreateFormatDiskPartition(
    __in unsigned long  ulDisk,
    __in ULONGLONG      cmbDiskSize,
    __in char           chLetter    
)
// =============================================================================
{
    char szFile[ MAX_PATH + 1 ];
    char szCmd[ MAX_PATH + 1 ];
    BOOL bReturn = TRUE;
    FILE *stream = NULL;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof( si ) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof( pi ) );

    StringCchPrintfA( szFile, _countof( szFile ), "DiskPartScript-%lu-%lu.dps", ( DWORD )GetCurrentProcessId(), ( DWORD )GetCurrentThreadId() );
    if ( ( stream = fopen( szFile, "w" ) ) == NULL ){
        bReturn = FALSE;
        goto Cleanup;
    }
    fprintf( stream, "SELECT DISK %lu" CRLF, ulDisk );
    if ( cmbDiskSize ){
        fprintf( stream, "CREATE PARTITION PRIMARY SIZE=%I64u" CRLF, cmbDiskSize );
    }
    else{
        fprintf( stream, "CREATE PARTITION PRIMARY" CRLF );
    }
    OSVERSIONINFOW osInfo;
    osInfo.dwOSVersionInfoSize = static_cast< DWORD >( sizeof( osInfo ) );
    if ( !GetVersionExW( &osInfo ) ){
        // Assume it's Vista/W2k8.
        osInfo.dwMajorVersion = 6;
    }
    if ( osInfo.dwMajorVersion >= 6 ){
        fprintf( stream, "FORMAT QUICK OVERRIDE" CRLF );
        fprintf( stream, "ASSIGN LETTER=%c" CRLF, chLetter );
    }
    fflush( stream );
    fclose( stream );
    stream = NULL;

    if ( !RunDiskPart( szFile ) ){
        bReturn = FALSE;
        goto Cleanup;
    }

    if ( osInfo.dwMajorVersion < 6 ){
        StringCchPrintfA( szCmd, _countof( szCmd ), "format.com %c: /Q /FS:NTFS /Y", chLetter );
        if ( !CreateProcessA( NULL, szCmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi ) ){
            bReturn = FALSE;
            goto Cleanup;
        }
        WaitForSingleObject( pi.hProcess, INFINITE );
        DWORD dwExitCode = ERROR_SUCCESS;
        if ( !GetExitCodeProcess( pi.hProcess, &dwExitCode ) || dwExitCode != ERROR_SUCCESS ){
            bReturn = FALSE;
        }
    }

Cleanup:
    if ( stream ){
        fclose( stream );
    }
    DeleteFileA( szFile );
    CloseHandleP( &pi.hProcess );
    CloseHandleP( &pi.hThread );

    return bReturn;
}

// =============================================================================
BOOL FormatVolume(
    __in char   chLetter
)
// =============================================================================
{
    char szFile[ MAX_PATH + 1 ];
    char szCmd[ MAX_PATH + 1 ];
    BOOL bReturn = TRUE;
    FILE *stream = NULL;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof( si ) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof( pi ) );

    OSVERSIONINFOW osInfo;
    osInfo.dwOSVersionInfoSize = static_cast< DWORD >( sizeof( osInfo ) );
    if ( !GetVersionExW( &osInfo ) ){
        // Assume it's Vista/W2k8.
        osInfo.dwMajorVersion = 6;
    }
    if ( osInfo.dwMajorVersion >= 6 ){
        StringCchPrintfA( szFile, _countof( szFile ), "DiskPartScript-%lu-%lu.dps", ( DWORD )GetCurrentProcessId(), ( DWORD )GetCurrentThreadId() );
        if ( ( stream = fopen( szFile, "w" ) ) == NULL ){
            bReturn = FALSE;
            goto TryFormatCom;
        }
        fprintf( stream, "SELECT VOLUME=%c" CRLF, chLetter );
        fprintf( stream, "FORMAT QUICK OVERRIDE" CRLF );
        fflush( stream );
        fclose( stream );
        stream = NULL;

        if ( !RunDiskPart( szFile ) ){
            bReturn = FALSE;
            goto Cleanup;
        }
    }
    else{
        bReturn = FALSE;
    }
    
TryFormatCom:
    if ( !bReturn ){
        bReturn = TRUE;
        StringCchPrintfA( szCmd, _countof( szCmd ), "format.com %c: /Q /FS:NTFS /Y", chLetter );
        if ( !CreateProcessA( NULL, szCmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi ) ){
            bReturn = FALSE;
            goto Cleanup;
        }
        WaitForSingleObject( pi.hProcess, INFINITE );
        DWORD dwExitCode = ERROR_SUCCESS;
        if ( !GetExitCodeProcess( pi.hProcess, &dwExitCode ) || dwExitCode != ERROR_SUCCESS ){
            bReturn = FALSE;
        }
    }

Cleanup:
    if ( stream ){
        fclose( stream );
    }
    DeleteFileA( szFile );
    CloseHandleP( &pi.hProcess );
    CloseHandleP( &pi.hThread );

    return bReturn;
}

