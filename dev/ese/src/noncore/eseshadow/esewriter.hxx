// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#ifndef _ESEWRITER_
#define _ESEWRITER_

#include <windows.h>

#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with vss headers, someone else owns that.")
#pragma prefast(disable:28205, "Do not bother us with vss headers, someone else owns that.")
#pragma prefast(disable:33006, "Do not bother us with vss headers, someone else owns that.")
#include <vss.h>
#include <vswriter.h>
#pragma prefast(pop)

#ifdef __cplusplus
extern "C" {
#endif


static const VSS_ID EseRecoveryWriterId =
{ 0xbcedcf69, 0xe5b4, 0x49ed, { 0x97, 0x23, 0x7b, 0xe9, 0xf7, 0xcd, 0x09, 0x92 } };

#ifdef ESENT
static const GUID AdamWriterId =
{ 0xdd846aaa, 0xa1b6, 0x42a8, { 0xaa, 0xf8,  0x03,  0xdc,  0xb6,  0x11,  0x4b, 0xfd } };
static const GUID AdWriterId =
{ 0xb2014c9e, 0x8711, 0x4c5c, { 0xa5, 0xa9,  0x3c,  0xf3,  0x84,  0x48,  0x47, 0x57 } };
#else
static const GUID StoreWriterId =
{ 0x76fe1ac4, 0x15f7, 0x4bcd, { 0x98, 0x7e,  0x8e,  0x1a,  0xcb,  0x46,  0x2f, 0xb7 } };
#endif

static const wchar_t* const  EseRecoveryWriterName = L"EseRecoveryWriter";




class EseRecoveryWriter : public CVssWriter    {
private:
public:
    static void StaticInitialize()  {  }
    
    EseRecoveryWriter();
    virtual ~EseRecoveryWriter();

    HRESULT STDMETHODCALLTYPE Initialize();
    HRESULT STDMETHODCALLTYPE Uninitialize();
#pragma prefast(push)
#pragma prefast(disable:28204, "Mismatched annotations; the prototypes are part of the SDK.")
    bool STDMETHODCALLTYPE OnIdentify(_In_ IVssCreateWriterMetadata *pMetadata);
    bool STDMETHODCALLTYPE OnPrepareBackup(_In_ IVssWriterComponents *pComponents);
    bool STDMETHODCALLTYPE OnPrepareSnapshot();
    bool STDMETHODCALLTYPE OnFreeze();
    bool STDMETHODCALLTYPE OnThaw();
    bool STDMETHODCALLTYPE OnPostSnapshot(_In_ IVssWriterComponents *pComponents);
    bool STDMETHODCALLTYPE OnAbort();
    bool STDMETHODCALLTYPE OnBackupComplete(_In_ IVssWriterComponents *pComponents);
    bool STDMETHODCALLTYPE OnBackupShutdown(_In_ VSS_ID SnapshotSetId);
    bool STDMETHODCALLTYPE OnPreRestore(_In_ IVssWriterComponents *pComponents);
    bool STDMETHODCALLTYPE OnPostRestore(_In_ IVssWriterComponents *pComponents);
#pragma prefast(pop)

public:
    static EseRecoveryWriter * GetSingleton()
    {
        if (NULL == EseRecoveryWriter::m_pExWriter)
        {
            EseRecoveryWriter::m_pExWriter = new EseRecoveryWriter;
        }
        return EseRecoveryWriter::m_pExWriter;
    }

    static void DeleteSingleton()
    {
        if (NULL != EseRecoveryWriter::m_pExWriter)
        {
            delete EseRecoveryWriter::m_pExWriter;
            EseRecoveryWriter::m_pExWriter = NULL;
        }
    }

    static HRESULT CreateWriter();
    static VOID DestroyWriter();
    


    HRESULT
    RecoverEseDatabase(
        __in PCWSTR szOldDbName,
        __in PCWSTR szNewDbName,
        __in PCWSTR szLogPath,
        __in PCWSTR szSystemPath
    );

    static EseRecoveryWriter*   m_pExWriter;

private:
    bool m_fVssInitialized;
    bool m_fVssSubscribed;
}
;


struct EseRecoveryWriterConfig
{
    PCWSTR  m_szDatabaseFileFullPath;
    PCWSTR m_szDatabasePath;

    PCWSTR m_szLogDirectory;
    PCWSTR m_szSystemDirectory;

    WCHAR m_szEseBaseName[ 4 ];

    BOOL m_fIgnoreMissingDb;
    BOOL m_fIgnoreLostLogs;
}
;


extern EseRecoveryWriterConfig g_eseRecoveryWriterConfig;
extern bool g_fVerbose;



#define DBGV( x ) if ( g_fVerbose ) { x; }

#define CallHr( func ) \
        hr = func; \
        if ( FAILED( hr ) ) { \
            DBGV( wprintf( L"Failed! %hs returned hr = %#x, [%hs %hs:%d]\n", #func, hr, __FUNCTION__, __FILE__, __LINE__ ) ); \
            goto Cleanup; \
        } \

#define CallEse( func ) \
        err = func; \
        if ( err < 0 ) { \
            DBGV( wprintf( L"Failed! %hs returned err = %#x, [%hs %hs:%d]\n", #func, err, __FUNCTION__, __FILE__, __LINE__ ) ); \
            hr = MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ESE, err ); \
            goto Cleanup; \
        } \

#define CHECK_SUCCESS Call
#define CHECK_SUCCESS_NOLOG Call
#define CHECK_NOFAIL Call
    
#define CHECK_NONZERO( func )                                                                           \
        {                                                                                                   \
            if (0 == (func)) {                                                                              \
                const DWORD dwErr = GetLastError();                                                         \
                hr = HRESULT_FROM_WIN32(dwErr);                                                             \
                goto Cleanup;                                                                               \
            }                                                                                               \
        }
    
#define CHECK_BOOL( func )                                                                              \
        {                                                                                                   \
            if ( FALSE == (func) ) {                                                                        \
                const DWORD dwErr = GetLastError();                                                         \
                hr = HRESULT_FROM_WIN32(dwErr);                                                             \
                goto Cleanup;                                                                               \
            }                                                                                               \
        }
    
    
#define ASSERT(x)

const DWORD FACILITY_ESE = 0xe5e;

#ifdef __cplusplus
}
#endif

#endif 

