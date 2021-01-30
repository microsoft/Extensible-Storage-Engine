// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <stdio.h>

#include "winperf.h"
#include "winreg.h"

#include "perfmon.hxx"

extern void PerfUtilLogEvent( DWORD evncat, WORD evntyp, const char *szDescription );

extern HANDLE g_hOurEventSource;



extern DWORD DwPerfUtilRegOpenKeyEx( HKEY hkeyRoot, LPCWSTR lpszSubKey, PHKEY phkResult );
extern DWORD DwPerfUtilRegOpenKeyEx( HKEY hkeyRoot, LPCWSTR lpszSubKey, REGSAM samDesired, PHKEY phkResult );
extern DWORD DwPerfUtilRegCloseKeyEx( HKEY hkey );
extern DWORD DwPerfUtilRegEnumKeyEx( HKEY hkeyRoot, DWORD dwIndex, __out_ecount(*pcchSubKey) LPWSTR lpszSubKey, DWORD* const pcchSubKey );
extern DWORD DwPerfUtilRegCreateKeyEx( HKEY hkeyRoot, LPCWSTR lpszSubKey, PHKEY phkResult, LPDWORD lpdwDisposition );
extern DWORD DwPerfUtilRegDeleteKeyEx( HKEY hkeyRoot, LPCWSTR lpszSubKey );
extern DWORD DwPerfUtilRegDeleteValueEx( HKEY hkey, LPCWSTR lpszValue );
extern DWORD DwPerfUtilRegSetValueEx( HKEY hkey, LPCWSTR lpszValue, DWORD fdwType, CONST BYTE *lpbData, DWORD cbData );
extern DWORD DwPerfUtilRegQueryValueEx( HKEY hkey, __in LPCWSTR lpszValue, LPDWORD lpdwType, LPBYTE *lplpbData );



extern DWORD DwPerfUtilInit( VOID );
extern VOID PerfUtilTerm( VOID );



extern HANDLE   g_hPERFGlobalMutex;
extern HANDLE   g_hPERFGDAMMF;
extern PGDA     g_pgdaPERFGDA;



