// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "ese_common.hxx"
#include "esetest_bounce.hxx"
#include <stdlib.h>
#include <strsafe.h>
#include <windows.h>

JET_ERR
BounceJetCreateInstance(
    JET_INSTANCE *pinstance,
    const char * szInstanceName
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetCreateInstance ) (  JET_INSTANCE *pinstance, const char * szInstanceName  );

    static PFN_JetCreateInstance pfnJetCreateInstance = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateInstanceA ) (  JET_INSTANCE *pinstance, const char * szInstanceName  );

    static PFN_JetCreateInstanceA pfnJetCreateInstanceA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateInstanceW ) (  JET_INSTANCE *pinstance, const WCHAR * wszInstanceName  );

    static PFN_JetCreateInstanceW pfnJetCreateInstanceW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszInstanceName = NULL;

    if ( fWiden )
    {
        if( NULL != szInstanceName ) 
        {
            wszInstanceName = EsetestWidenString( __FUNCTION__, szInstanceName );
            if ( NULL == wszInstanceName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szInstanceName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetCreateInstanceW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateInstanceW = ( PFN_JetCreateInstanceW ) ( GetProcAddress( hEseDll, "JetCreateInstanceW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateInstanceW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateInstanceW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateInstanceW)( pinstance, wszInstanceName );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetCreateInstance )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateInstance = ( PFN_JetCreateInstance ) ( GetProcAddress( hEseDll, "JetCreateInstance" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateInstance )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateInstance", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateInstance)( pinstance, szInstanceName );
        }
        else
        {

            if ( NULL == pfnJetCreateInstanceA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateInstanceA = ( PFN_JetCreateInstanceA ) ( GetProcAddress( hEseDll, "JetCreateInstanceA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateInstanceA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateInstanceA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateInstanceA)( pinstance, szInstanceName );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszInstanceName, szInstanceName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetCreateInstance2(
    JET_INSTANCE *pinstance,
    const char * szInstanceName,
    const char * szDisplayName,
    JET_GRBIT grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetCreateInstance2 ) ( 
    JET_INSTANCE *pinstance,
    const char * szInstanceName,
    const char * szDisplayName,
    JET_GRBIT grbit  );

    static PFN_JetCreateInstance2 pfnJetCreateInstance2 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateInstance2A ) ( 
    JET_INSTANCE *pinstance,
    const char * szInstanceName,
    const char * szDisplayName,
    JET_GRBIT grbit  );

    static PFN_JetCreateInstance2A pfnJetCreateInstance2A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateInstance2W ) ( 
    JET_INSTANCE *pinstance,
    const WCHAR * wszInstanceName,
    const WCHAR * wszDisplayName,
    JET_GRBIT grbit  );

    static PFN_JetCreateInstance2W pfnJetCreateInstance2W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszDisplayName  = NULL;
    wchar_t*    wszInstanceName = NULL;

    if ( fWiden )
    {
        if( NULL != szDisplayName ) 
        {
            wszDisplayName = EsetestWidenString( __FUNCTION__, szDisplayName );
            if ( NULL == wszDisplayName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szDisplayName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szInstanceName ) 
        {
            wszInstanceName = EsetestWidenString( __FUNCTION__, szInstanceName );
            if ( NULL == wszInstanceName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szInstanceName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetCreateInstance2W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateInstance2W = ( PFN_JetCreateInstance2W ) ( GetProcAddress( hEseDll, "JetCreateInstance2W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateInstance2W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateInstance2W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateInstance2W)( pinstance, wszInstanceName, wszDisplayName, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetCreateInstance2 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateInstance2 = ( PFN_JetCreateInstance2 ) ( GetProcAddress( hEseDll, "JetCreateInstance2" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateInstance2 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateInstance2", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateInstance2)( pinstance, szInstanceName, szDisplayName, grbit );
        }
        else
        {

            if ( NULL == pfnJetCreateInstance2A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateInstance2A = ( PFN_JetCreateInstance2A ) ( GetProcAddress( hEseDll, "JetCreateInstance2A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateInstance2A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateInstance2A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateInstance2A)( pinstance, szInstanceName, szDisplayName, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszDisplayName, szDisplayName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszInstanceName, szInstanceName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetSetSystemParameter(
    JET_INSTANCE    *pinstance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCSTR      szParam
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetSetSystemParameter ) ( 
    JET_INSTANCE    *pinstance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCSTR      szParam  );

    static PFN_JetSetSystemParameter pfnJetSetSystemParameter = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetSetSystemParameterA ) ( 
    JET_INSTANCE    *pinstance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCSTR      szParam  );

    static PFN_JetSetSystemParameterA pfnJetSetSystemParameterA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetSetSystemParameterW ) ( 
    JET_INSTANCE    *pinstance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCWSTR     wszParam  );

    static PFN_JetSetSystemParameterW pfnJetSetSystemParameterW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszParam    = NULL;

    if ( fWiden )
    {
        if( NULL != szParam ) 
        {
            wszParam = EsetestWidenString( __FUNCTION__, szParam );
            if ( NULL == wszParam )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szParam", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetSetSystemParameterW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetSetSystemParameterW = ( PFN_JetSetSystemParameterW ) ( GetProcAddress( hEseDll, "JetSetSystemParameterW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetSetSystemParameterW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetSetSystemParameterW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetSetSystemParameterW)( pinstance, sesid, paramid, lParam, wszParam );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetSetSystemParameter )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetSetSystemParameter = ( PFN_JetSetSystemParameter ) ( GetProcAddress( hEseDll, "JetSetSystemParameter" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetSetSystemParameter )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetSetSystemParameter", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetSetSystemParameter)( pinstance, sesid, paramid, lParam, szParam );
        }
        else
        {

            if ( NULL == pfnJetSetSystemParameterA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetSetSystemParameterA = ( PFN_JetSetSystemParameterA ) ( GetProcAddress( hEseDll, "JetSetSystemParameterA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetSetSystemParameterA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetSetSystemParameterA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetSetSystemParameterA)( pinstance, sesid, paramid, lParam, szParam );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszParam, szParam );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetBeginSession(
    JET_INSTANCE    instance,
    JET_SESID       *psesid,
    __in_opt JET_PCSTR  szUserName,
    __in_opt JET_PCSTR  szPassword
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetBeginSession ) ( 
    JET_INSTANCE    instance,
    JET_SESID       *psesid,
    __in_opt JET_PCSTR  szUserName,
    __in_opt JET_PCSTR  szPassword  );

    static PFN_JetBeginSession pfnJetBeginSession = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetBeginSessionA ) ( 
    JET_INSTANCE    instance,
    JET_SESID       *psesid,
    __in_opt JET_PCSTR  szUserName,
    __in_opt JET_PCSTR  szPassword  );

    static PFN_JetBeginSessionA pfnJetBeginSessionA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetBeginSessionW ) ( 
    JET_INSTANCE    instance,
    JET_SESID       *psesid,
    __in_opt JET_PCWSTR  wszUserName,
    __in_opt JET_PCWSTR  wszPassword  );

    static PFN_JetBeginSessionW pfnJetBeginSessionW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszUserName = NULL;
    wchar_t*    wszPassword = NULL;

    if ( fWiden )
    {
        if( NULL != szUserName ) 
        {
            wszUserName = EsetestWidenString( __FUNCTION__, szUserName );
            if ( NULL == wszUserName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szUserName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szPassword ) 
        {
            wszPassword = EsetestWidenString( __FUNCTION__, szPassword );
            if ( NULL == wszPassword )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szPassword", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetBeginSessionW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetBeginSessionW = ( PFN_JetBeginSessionW ) ( GetProcAddress( hEseDll, "JetBeginSessionW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetBeginSessionW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetBeginSessionW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetBeginSessionW)( instance, psesid, wszUserName, wszPassword );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetBeginSession )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetBeginSession = ( PFN_JetBeginSession ) ( GetProcAddress( hEseDll, "JetBeginSession" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetBeginSession )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetBeginSession", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetBeginSession)( instance, psesid, szUserName, szPassword );
        }
        else
        {

            if ( NULL == pfnJetBeginSessionA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetBeginSessionA = ( PFN_JetBeginSessionA ) ( GetProcAddress( hEseDll, "JetBeginSessionA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetBeginSessionA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetBeginSessionA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetBeginSessionA)( instance, psesid, szUserName, szPassword );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszUserName, szUserName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszPassword, szPassword );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetCreateDatabase(
    JET_SESID       sesid,
    const char      *szFilename,
    const char      *szConnect,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetCreateDatabase ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    const char      *szConnect,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit  );

    static PFN_JetCreateDatabase pfnJetCreateDatabase = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateDatabaseA ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    const char      *szConnect,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit  );

    static PFN_JetCreateDatabaseA pfnJetCreateDatabaseA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateDatabaseW ) ( 
    JET_SESID       sesid,
    const WCHAR     *wszFilename,
    const WCHAR     *wszConnect,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit  );

    static PFN_JetCreateDatabaseW pfnJetCreateDatabaseW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszConnect  = NULL;
    wchar_t*    wszFilename = NULL;

    if ( fWiden )
    {
        if( NULL != szConnect ) 
        {
            wszConnect = EsetestWidenString( __FUNCTION__, szConnect );
            if ( NULL == wszConnect )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szConnect", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szFilename ) 
        {
            wszFilename = EsetestWidenString( __FUNCTION__, szFilename );
            if ( NULL == wszFilename )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szFilename", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetCreateDatabaseW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateDatabaseW = ( PFN_JetCreateDatabaseW ) ( GetProcAddress( hEseDll, "JetCreateDatabaseW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateDatabaseW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateDatabaseW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateDatabaseW)( sesid, wszFilename, wszConnect, pdbid, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetCreateDatabase )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateDatabase = ( PFN_JetCreateDatabase ) ( GetProcAddress( hEseDll, "JetCreateDatabase" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateDatabase )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateDatabase", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateDatabase)( sesid, szFilename, szConnect, pdbid, grbit );
        }
        else
        {

            if ( NULL == pfnJetCreateDatabaseA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateDatabaseA = ( PFN_JetCreateDatabaseA ) ( GetProcAddress( hEseDll, "JetCreateDatabaseA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateDatabaseA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateDatabaseA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateDatabaseA)( sesid, szFilename, szConnect, pdbid, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszConnect, szConnect );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszFilename, szFilename );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetCreateDatabase2(
    JET_SESID       sesid,
    const char      *szFilename,
    const unsigned long cpgDatabaseSizeMax,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetCreateDatabase2 ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    const unsigned long cpgDatabaseSizeMax,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit  );

    static PFN_JetCreateDatabase2 pfnJetCreateDatabase2 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateDatabase2A ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    const unsigned long cpgDatabaseSizeMax,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit  );

    static PFN_JetCreateDatabase2A pfnJetCreateDatabase2A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateDatabase2W ) ( 
    JET_SESID       sesid,
    const WCHAR     *wszFilename,
    const unsigned long cpgDatabaseSizeMax,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit  );

    static PFN_JetCreateDatabase2W pfnJetCreateDatabase2W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszFilename = NULL;

    if ( fWiden )
    {
        if( NULL != szFilename ) 
        {
            wszFilename = EsetestWidenString( __FUNCTION__, szFilename );
            if ( NULL == wszFilename )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szFilename", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetCreateDatabase2W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateDatabase2W = ( PFN_JetCreateDatabase2W ) ( GetProcAddress( hEseDll, "JetCreateDatabase2W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateDatabase2W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateDatabase2W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateDatabase2W)( sesid, wszFilename, cpgDatabaseSizeMax, pdbid, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetCreateDatabase2 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateDatabase2 = ( PFN_JetCreateDatabase2 ) ( GetProcAddress( hEseDll, "JetCreateDatabase2" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateDatabase2 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateDatabase2", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateDatabase2)( sesid, szFilename, cpgDatabaseSizeMax, pdbid, grbit );
        }
        else
        {

            if ( NULL == pfnJetCreateDatabase2A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateDatabase2A = ( PFN_JetCreateDatabase2A ) ( GetProcAddress( hEseDll, "JetCreateDatabase2A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateDatabase2A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateDatabase2A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateDatabase2A)( sesid, szFilename, cpgDatabaseSizeMax, pdbid, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszFilename, szFilename );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetCreateDatabaseWithStreaming(
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const char      *szSLVRootName,
    const unsigned long cpgDatabaseSizeMax,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetCreateDatabaseWithStreaming ) ( 
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const char      *szSLVRootName,
    const unsigned long cpgDatabaseSizeMax,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit  );

    static PFN_JetCreateDatabaseWithStreaming pfnJetCreateDatabaseWithStreaming = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateDatabaseWithStreamingA ) ( 
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const char      *szSLVRootName,
    const unsigned long cpgDatabaseSizeMax,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit  );

    static PFN_JetCreateDatabaseWithStreamingA pfnJetCreateDatabaseWithStreamingA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateDatabaseWithStreamingW ) ( 
    JET_SESID       sesid,
    const WCHAR     *wszDbFileName,
    const WCHAR     *wszSLVFileName,
    const WCHAR     *wszSLVRootName,
    const unsigned long cpgDatabaseSizeMax,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit  );

    static PFN_JetCreateDatabaseWithStreamingW pfnJetCreateDatabaseWithStreamingW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszSLVFileName  = NULL;
    wchar_t*    wszSLVRootName  = NULL;
    wchar_t*    wszDbFileName   = NULL;

    if ( fWiden )
    {
        if( NULL != szSLVFileName ) 
        {
            wszSLVFileName = EsetestWidenString( __FUNCTION__, szSLVFileName );
            if ( NULL == wszSLVFileName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szSLVFileName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szSLVRootName ) 
        {
            wszSLVRootName = EsetestWidenString( __FUNCTION__, szSLVRootName );
            if ( NULL == wszSLVRootName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szSLVRootName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szDbFileName ) 
        {
            wszDbFileName = EsetestWidenString( __FUNCTION__, szDbFileName );
            if ( NULL == wszDbFileName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szDbFileName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetCreateDatabaseWithStreamingW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateDatabaseWithStreamingW = ( PFN_JetCreateDatabaseWithStreamingW ) ( GetProcAddress( hEseDll, "JetCreateDatabaseWithStreamingW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateDatabaseWithStreamingW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateDatabaseWithStreamingW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateDatabaseWithStreamingW)( sesid, wszDbFileName, wszSLVFileName, wszSLVRootName, cpgDatabaseSizeMax, pdbid, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetCreateDatabaseWithStreaming )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateDatabaseWithStreaming = ( PFN_JetCreateDatabaseWithStreaming ) ( GetProcAddress( hEseDll, "JetCreateDatabaseWithStreaming" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateDatabaseWithStreaming )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateDatabaseWithStreaming", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateDatabaseWithStreaming)( sesid, szDbFileName, szSLVFileName, szSLVRootName, cpgDatabaseSizeMax, pdbid, grbit );
        }
        else
        {

            if ( NULL == pfnJetCreateDatabaseWithStreamingA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateDatabaseWithStreamingA = ( PFN_JetCreateDatabaseWithStreamingA ) ( GetProcAddress( hEseDll, "JetCreateDatabaseWithStreamingA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateDatabaseWithStreamingA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateDatabaseWithStreamingA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateDatabaseWithStreamingA)( sesid, szDbFileName, szSLVFileName, szSLVRootName, cpgDatabaseSizeMax, pdbid, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszSLVFileName, szSLVFileName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszSLVRootName, szSLVRootName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszDbFileName, szDbFileName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetAttachDatabase(
    JET_SESID       sesid,
    const char      *szFilename,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetAttachDatabase ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    JET_GRBIT       grbit  );

    static PFN_JetAttachDatabase pfnJetAttachDatabase = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetAttachDatabaseA ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    JET_GRBIT       grbit  );

    static PFN_JetAttachDatabaseA pfnJetAttachDatabaseA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetAttachDatabaseW ) ( 
    JET_SESID       sesid,
    const WCHAR     *wszFilename,
    JET_GRBIT       grbit  );

    static PFN_JetAttachDatabaseW pfnJetAttachDatabaseW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszFilename = NULL;

    if ( fWiden )
    {
        if( NULL != szFilename ) 
        {
            wszFilename = EsetestWidenString( __FUNCTION__, szFilename );
            if ( NULL == wszFilename )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szFilename", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetAttachDatabaseW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetAttachDatabaseW = ( PFN_JetAttachDatabaseW ) ( GetProcAddress( hEseDll, "JetAttachDatabaseW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetAttachDatabaseW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetAttachDatabaseW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetAttachDatabaseW)( sesid, wszFilename, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetAttachDatabase )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetAttachDatabase = ( PFN_JetAttachDatabase ) ( GetProcAddress( hEseDll, "JetAttachDatabase" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetAttachDatabase )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetAttachDatabase", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetAttachDatabase)( sesid, szFilename, grbit );
        }
        else
        {

            if ( NULL == pfnJetAttachDatabaseA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetAttachDatabaseA = ( PFN_JetAttachDatabaseA ) ( GetProcAddress( hEseDll, "JetAttachDatabaseA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetAttachDatabaseA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetAttachDatabaseA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetAttachDatabaseA)( sesid, szFilename, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszFilename, szFilename );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetAttachDatabase2(
    JET_SESID       sesid,
    const char      *szFilename,
    const unsigned long cpgDatabaseSizeMax,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetAttachDatabase2 ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    const unsigned long cpgDatabaseSizeMax,
    JET_GRBIT       grbit  );

    static PFN_JetAttachDatabase2 pfnJetAttachDatabase2 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetAttachDatabase2A ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    const unsigned long cpgDatabaseSizeMax,
    JET_GRBIT       grbit  );

    static PFN_JetAttachDatabase2A pfnJetAttachDatabase2A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetAttachDatabase2W ) ( 
    JET_SESID       sesid,
    const WCHAR     *wszFilename,
    const unsigned long cpgDatabaseSizeMax,
    JET_GRBIT       grbit  );

    static PFN_JetAttachDatabase2W pfnJetAttachDatabase2W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszFilename = NULL;

    if ( fWiden )
    {
        if( NULL != szFilename ) 
        {
            wszFilename = EsetestWidenString( __FUNCTION__, szFilename );
            if ( NULL == wszFilename )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szFilename", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetAttachDatabase2W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetAttachDatabase2W = ( PFN_JetAttachDatabase2W ) ( GetProcAddress( hEseDll, "JetAttachDatabase2W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetAttachDatabase2W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetAttachDatabase2W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetAttachDatabase2W)( sesid, wszFilename, cpgDatabaseSizeMax, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetAttachDatabase2 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetAttachDatabase2 = ( PFN_JetAttachDatabase2 ) ( GetProcAddress( hEseDll, "JetAttachDatabase2" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetAttachDatabase2 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetAttachDatabase2", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetAttachDatabase2)( sesid, szFilename, cpgDatabaseSizeMax, grbit );
        }
        else
        {

            if ( NULL == pfnJetAttachDatabase2A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetAttachDatabase2A = ( PFN_JetAttachDatabase2A ) ( GetProcAddress( hEseDll, "JetAttachDatabase2A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetAttachDatabase2A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetAttachDatabase2A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetAttachDatabase2A)( sesid, szFilename, cpgDatabaseSizeMax, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszFilename, szFilename );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}


JET_ERR
BounceJetAttachDatabase3(
    JET_SESID       sesid,
    const char      *szFilename,
    const JET_SETDBPARAM* const rgsetdbparam,
    const unsigned long csetdbparam,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetAttachDatabase3 ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    const JET_SETDBPARAM* const rgsetdbparam,
    const unsigned long csetdbparam,
    JET_GRBIT       grbit  );

    static PFN_JetAttachDatabase3 pfnJetAttachDatabase3 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetAttachDatabase3A ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    const JET_SETDBPARAM* const rgsetdbparam,
    const unsigned long csetdbparam,
    JET_GRBIT       grbit  );

    static PFN_JetAttachDatabase3A pfnJetAttachDatabase3A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetAttachDatabase3W ) ( 
    JET_SESID       sesid,
    const WCHAR     *wszFilename,
    const JET_SETDBPARAM* const rgsetdbparam,
    const unsigned long csetdbparam,
    JET_GRBIT       grbit  );

    static PFN_JetAttachDatabase3W pfnJetAttachDatabase3W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszFilename = NULL;

    if ( fWiden )
    {
        if( NULL != szFilename ) 
        {
            wszFilename = EsetestWidenString( __FUNCTION__, szFilename );
            if ( NULL == wszFilename )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szFilename", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetAttachDatabase3W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetAttachDatabase3W = ( PFN_JetAttachDatabase3W ) ( GetProcAddress( hEseDll, "JetAttachDatabase3W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetAttachDatabase3W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetAttachDatabase3W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetAttachDatabase3W)( sesid, wszFilename, rgsetdbparam, csetdbparam, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetAttachDatabase3 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetAttachDatabase3 = ( PFN_JetAttachDatabase3 ) ( GetProcAddress( hEseDll, "JetAttachDatabase3" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetAttachDatabase3 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetAttachDatabase3", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetAttachDatabase3)( sesid, szFilename, rgsetdbparam, csetdbparam, grbit );
        }
        else
        {

            if ( NULL == pfnJetAttachDatabase3A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetAttachDatabase3A = ( PFN_JetAttachDatabase3A ) ( GetProcAddress( hEseDll, "JetAttachDatabase3A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetAttachDatabase3A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetAttachDatabase3A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetAttachDatabase3A)( sesid, szFilename, rgsetdbparam, csetdbparam, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszFilename, szFilename );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetAttachDatabaseWithStreaming(
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const char      *szSLVRootName,
    const unsigned long cpgDatabaseSizeMax,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetAttachDatabaseWithStreaming ) ( 
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const char      *szSLVRootName,
    const unsigned long cpgDatabaseSizeMax,
    JET_GRBIT       grbit  );

    static PFN_JetAttachDatabaseWithStreaming pfnJetAttachDatabaseWithStreaming = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetAttachDatabaseWithStreamingA ) ( 
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const char      *szSLVRootName,
    const unsigned long cpgDatabaseSizeMax,
    JET_GRBIT       grbit  );

    static PFN_JetAttachDatabaseWithStreamingA pfnJetAttachDatabaseWithStreamingA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetAttachDatabaseWithStreamingW ) ( 
    JET_SESID       sesid,
    const WCHAR     *wszDbFileName,
    const WCHAR     *wszSLVFileName,
    const WCHAR     *wszSLVRootName,
    const unsigned long cpgDatabaseSizeMax,
    JET_GRBIT       grbit  );

    static PFN_JetAttachDatabaseWithStreamingW pfnJetAttachDatabaseWithStreamingW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszSLVFileName  = NULL;
    wchar_t*    wszSLVRootName  = NULL;
    wchar_t*    wszDbFileName   = NULL;

    if ( fWiden )
    {
        if( NULL != szSLVFileName ) 
        {
            wszSLVFileName = EsetestWidenString( __FUNCTION__, szSLVFileName );
            if ( NULL == wszSLVFileName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szSLVFileName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szSLVRootName ) 
        {
            wszSLVRootName = EsetestWidenString( __FUNCTION__, szSLVRootName );
            if ( NULL == wszSLVRootName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szSLVRootName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szDbFileName ) 
        {
            wszDbFileName = EsetestWidenString( __FUNCTION__, szDbFileName );
            if ( NULL == wszDbFileName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szDbFileName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetAttachDatabaseWithStreamingW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetAttachDatabaseWithStreamingW = ( PFN_JetAttachDatabaseWithStreamingW ) ( GetProcAddress( hEseDll, "JetAttachDatabaseWithStreamingW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetAttachDatabaseWithStreamingW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetAttachDatabaseWithStreamingW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetAttachDatabaseWithStreamingW)( sesid, wszDbFileName, wszSLVFileName, wszSLVRootName, cpgDatabaseSizeMax, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetAttachDatabaseWithStreaming )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetAttachDatabaseWithStreaming = ( PFN_JetAttachDatabaseWithStreaming ) ( GetProcAddress( hEseDll, "JetAttachDatabaseWithStreaming" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetAttachDatabaseWithStreaming )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetAttachDatabaseWithStreaming", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetAttachDatabaseWithStreaming)( sesid, szDbFileName, szSLVFileName, szSLVRootName, cpgDatabaseSizeMax, grbit );
        }
        else
        {

            if ( NULL == pfnJetAttachDatabaseWithStreamingA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetAttachDatabaseWithStreamingA = ( PFN_JetAttachDatabaseWithStreamingA ) ( GetProcAddress( hEseDll, "JetAttachDatabaseWithStreamingA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetAttachDatabaseWithStreamingA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetAttachDatabaseWithStreamingA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetAttachDatabaseWithStreamingA)( sesid, szDbFileName, szSLVFileName, szSLVRootName, cpgDatabaseSizeMax, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszSLVFileName, szSLVFileName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszSLVRootName, szSLVRootName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszDbFileName, szDbFileName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetDetachDatabase(
    JET_SESID       sesid,
    const char      *szFilename
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetDetachDatabase ) ( 
    JET_SESID       sesid,
    const char      *szFilename  );

    static PFN_JetDetachDatabase pfnJetDetachDatabase = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDetachDatabaseA ) ( 
    JET_SESID       sesid,
    const char      *szFilename  );

    static PFN_JetDetachDatabaseA pfnJetDetachDatabaseA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDetachDatabaseW ) ( 
    JET_SESID       sesid,
    const WCHAR     *wszFilename  );

    static PFN_JetDetachDatabaseW pfnJetDetachDatabaseW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszFilename = NULL;

    if ( fWiden )
    {
        if( NULL != szFilename ) 
        {
            wszFilename = EsetestWidenString( __FUNCTION__, szFilename );
            if ( NULL == wszFilename )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szFilename", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetDetachDatabaseW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetDetachDatabaseW = ( PFN_JetDetachDatabaseW ) ( GetProcAddress( hEseDll, "JetDetachDatabaseW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetDetachDatabaseW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetDetachDatabaseW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetDetachDatabaseW)( sesid, wszFilename );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetDetachDatabase )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDetachDatabase = ( PFN_JetDetachDatabase ) ( GetProcAddress( hEseDll, "JetDetachDatabase" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDetachDatabase )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDetachDatabase", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDetachDatabase)( sesid, szFilename );
        }
        else
        {

            if ( NULL == pfnJetDetachDatabaseA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDetachDatabaseA = ( PFN_JetDetachDatabaseA ) ( GetProcAddress( hEseDll, "JetDetachDatabaseA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDetachDatabaseA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDetachDatabaseA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDetachDatabaseA)( sesid, szFilename );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszFilename, szFilename );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetDetachDatabase2(
    JET_SESID       sesid,
    const char      *szFilename,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetDetachDatabase2 ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    JET_GRBIT       grbit );

    static PFN_JetDetachDatabase2 pfnJetDetachDatabase2 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDetachDatabase2A ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    JET_GRBIT       grbit );

    static PFN_JetDetachDatabase2A pfnJetDetachDatabase2A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDetachDatabase2W ) ( 
    JET_SESID       sesid,
    const WCHAR     *wszFilename,
    JET_GRBIT       grbit );

    static PFN_JetDetachDatabase2W pfnJetDetachDatabase2W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszFilename = NULL;

    if ( fWiden )
    {
        if( NULL != szFilename ) 
        {
            wszFilename = EsetestWidenString( __FUNCTION__, szFilename );
            if ( NULL == wszFilename )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szFilename", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetDetachDatabase2W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetDetachDatabase2W = ( PFN_JetDetachDatabase2W ) ( GetProcAddress( hEseDll, "JetDetachDatabase2W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetDetachDatabase2W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetDetachDatabase2W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetDetachDatabase2W)( sesid, wszFilename, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetDetachDatabase2 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDetachDatabase2 = ( PFN_JetDetachDatabase2 ) ( GetProcAddress( hEseDll, "JetDetachDatabase2" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDetachDatabase2 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDetachDatabase2", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDetachDatabase2)( sesid, szFilename, grbit );
        }
        else
        {

            if ( NULL == pfnJetDetachDatabase2A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDetachDatabase2A = ( PFN_JetDetachDatabase2A ) ( GetProcAddress( hEseDll, "JetDetachDatabase2A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDetachDatabase2A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDetachDatabase2A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDetachDatabase2A)( sesid, szFilename, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszFilename, szFilename );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetCreateTable(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   lPages,
    unsigned long   lDensity,
    JET_TABLEID     *ptableid
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetCreateTable ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   lPages,
    unsigned long   lDensity,
    JET_TABLEID     *ptableid  );

    static PFN_JetCreateTable pfnJetCreateTable = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateTableA ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   lPages,
    unsigned long   lDensity,
    JET_TABLEID     *ptableid  );

    static PFN_JetCreateTableA pfnJetCreateTableA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateTableW ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const WCHAR     *wszTableName,
    unsigned long   lPages,
    unsigned long   lDensity,
    JET_TABLEID     *ptableid  );

    static PFN_JetCreateTableW pfnJetCreateTableW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszTableName    = NULL;

    if ( fWiden )
    {
        if( NULL != szTableName ) 
        {
            wszTableName = EsetestWidenString( __FUNCTION__, szTableName );
            if ( NULL == wszTableName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szTableName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetCreateTableW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateTableW = ( PFN_JetCreateTableW ) ( GetProcAddress( hEseDll, "JetCreateTableW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateTableW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateTableW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateTableW)( sesid, dbid, wszTableName, lPages, lDensity, ptableid );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetCreateTable )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateTable = ( PFN_JetCreateTable ) ( GetProcAddress( hEseDll, "JetCreateTable" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateTable )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateTable", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateTable)( sesid, dbid, szTableName, lPages, lDensity, ptableid );
        }
        else
        {

            if ( NULL == pfnJetCreateTableA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateTableA = ( PFN_JetCreateTableA ) ( GetProcAddress( hEseDll, "JetCreateTableA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateTableA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateTableA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateTableA)( sesid, dbid, szTableName, lPages, lDensity, ptableid );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszTableName, szTableName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetDeleteTable(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetDeleteTable ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName  );

    static PFN_JetDeleteTable pfnJetDeleteTable = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDeleteTableA ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName  );

    static PFN_JetDeleteTableA pfnJetDeleteTableA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDeleteTableW ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const WCHAR     *wszTableName  );

    static PFN_JetDeleteTableW pfnJetDeleteTableW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszTableName    = NULL;

    if ( fWiden )
    {
        if( NULL != szTableName ) 
        {
            wszTableName = EsetestWidenString( __FUNCTION__, szTableName );
            if ( NULL == wszTableName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szTableName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetDeleteTableW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetDeleteTableW = ( PFN_JetDeleteTableW ) ( GetProcAddress( hEseDll, "JetDeleteTableW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetDeleteTableW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetDeleteTableW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetDeleteTableW)( sesid, dbid, wszTableName );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetDeleteTable )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDeleteTable = ( PFN_JetDeleteTable ) ( GetProcAddress( hEseDll, "JetDeleteTable" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDeleteTable )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDeleteTable", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDeleteTable)( sesid, dbid, szTableName );
        }
        else
        {

            if ( NULL == pfnJetDeleteTableA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDeleteTableA = ( PFN_JetDeleteTableA ) ( GetProcAddress( hEseDll, "JetDeleteTableA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDeleteTableA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDeleteTableA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDeleteTableA)( sesid, dbid, szTableName );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszTableName, szTableName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetRenameTable(
    JET_SESID sesid,
    JET_DBID dbid,
    const char *szName,
    const char *szNameNew
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetRenameTable ) ( 
    JET_SESID sesid,
    JET_DBID dbid,
    const char *szName,
    const char *szNameNew  );

    static PFN_JetRenameTable pfnJetRenameTable = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetRenameTableA ) ( 
    JET_SESID sesid,
    JET_DBID dbid,
    const char *szName,
    const char *szNameNew  );

    static PFN_JetRenameTableA pfnJetRenameTableA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetRenameTableW ) ( 
    JET_SESID sesid,
    JET_DBID dbid,
    const WCHAR *wszName,
    const WCHAR *wszNameNew  );

    static PFN_JetRenameTableW pfnJetRenameTableW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszName = NULL;
    wchar_t*    wszNameNew  = NULL;

    if ( fWiden )
    {
        if( NULL != szName ) 
        {
            wszName = EsetestWidenString( __FUNCTION__, szName );
            if ( NULL == wszName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szNameNew ) 
        {
            wszNameNew = EsetestWidenString( __FUNCTION__, szNameNew );
            if ( NULL == wszNameNew )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szNameNew", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetRenameTableW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetRenameTableW = ( PFN_JetRenameTableW ) ( GetProcAddress( hEseDll, "JetRenameTableW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetRenameTableW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetRenameTableW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetRenameTableW)( sesid, dbid, wszName, wszNameNew );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetRenameTable )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetRenameTable = ( PFN_JetRenameTable ) ( GetProcAddress( hEseDll, "JetRenameTable" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetRenameTable )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetRenameTable", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetRenameTable)( sesid, dbid, szName, szNameNew );
        }
        else
        {

            if ( NULL == pfnJetRenameTableA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetRenameTableA = ( PFN_JetRenameTableA ) ( GetProcAddress( hEseDll, "JetRenameTableA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetRenameTableA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetRenameTableA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetRenameTableA)( sesid, dbid, szName, szNameNew );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszName, szName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszNameNew, szNameNew );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetDeleteColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetDeleteColumn ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName  );

    static PFN_JetDeleteColumn pfnJetDeleteColumn = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDeleteColumnA ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName  );

    static PFN_JetDeleteColumnA pfnJetDeleteColumnA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDeleteColumnW ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const WCHAR     *wszColumnName  );

    static PFN_JetDeleteColumnW pfnJetDeleteColumnW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszColumnName   = NULL;

    if ( fWiden )
    {
        if( NULL != szColumnName ) 
        {
            wszColumnName = EsetestWidenString( __FUNCTION__, szColumnName );
            if ( NULL == wszColumnName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szColumnName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetDeleteColumnW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetDeleteColumnW = ( PFN_JetDeleteColumnW ) ( GetProcAddress( hEseDll, "JetDeleteColumnW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetDeleteColumnW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetDeleteColumnW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetDeleteColumnW)( sesid, tableid, wszColumnName );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetDeleteColumn )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDeleteColumn = ( PFN_JetDeleteColumn ) ( GetProcAddress( hEseDll, "JetDeleteColumn" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDeleteColumn )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDeleteColumn", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDeleteColumn)( sesid, tableid, szColumnName );
        }
        else
        {

            if ( NULL == pfnJetDeleteColumnA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDeleteColumnA = ( PFN_JetDeleteColumnA ) ( GetProcAddress( hEseDll, "JetDeleteColumnA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDeleteColumnA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDeleteColumnA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDeleteColumnA)( sesid, tableid, szColumnName );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszColumnName, szColumnName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetDeleteColumn2(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName,
    const JET_GRBIT grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetDeleteColumn2 ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName,
    const JET_GRBIT grbit  );

    static PFN_JetDeleteColumn2 pfnJetDeleteColumn2 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDeleteColumn2A ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName,
    const JET_GRBIT grbit  );

    static PFN_JetDeleteColumn2A pfnJetDeleteColumn2A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDeleteColumn2W ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const WCHAR     *wszColumnName,
    const JET_GRBIT grbit  );

    static PFN_JetDeleteColumn2W pfnJetDeleteColumn2W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszColumnName   = NULL;

    if ( fWiden )
    {
        if( NULL != szColumnName ) 
        {
            wszColumnName = EsetestWidenString( __FUNCTION__, szColumnName );
            if ( NULL == wszColumnName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szColumnName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetDeleteColumn2W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetDeleteColumn2W = ( PFN_JetDeleteColumn2W ) ( GetProcAddress( hEseDll, "JetDeleteColumn2W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetDeleteColumn2W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetDeleteColumn2W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetDeleteColumn2W)( sesid, tableid, wszColumnName, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetDeleteColumn2 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDeleteColumn2 = ( PFN_JetDeleteColumn2 ) ( GetProcAddress( hEseDll, "JetDeleteColumn2" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDeleteColumn2 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDeleteColumn2", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDeleteColumn2)( sesid, tableid, szColumnName, grbit );
        }
        else
        {

            if ( NULL == pfnJetDeleteColumn2A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDeleteColumn2A = ( PFN_JetDeleteColumn2A ) ( GetProcAddress( hEseDll, "JetDeleteColumn2A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDeleteColumn2A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDeleteColumn2A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDeleteColumn2A)( sesid, tableid, szColumnName, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszColumnName, szColumnName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetRenameColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szName,
    __in JET_PCSTR  szNameNew,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetRenameColumn ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szName,
    __in JET_PCSTR  szNameNew,
    JET_GRBIT       grbit  );

    static PFN_JetRenameColumn pfnJetRenameColumn = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetRenameColumnA ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szName,
    __in JET_PCSTR  szNameNew,
    JET_GRBIT       grbit  );

    static PFN_JetRenameColumnA pfnJetRenameColumnA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetRenameColumnW ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCWSTR wszName,
    __in JET_PCWSTR wszNameNew,
    JET_GRBIT       grbit  );

    static PFN_JetRenameColumnW pfnJetRenameColumnW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszName = NULL;
    wchar_t*    wszNameNew  = NULL;

    if ( fWiden )
    {
        if( NULL != szName ) 
        {
            wszName = EsetestWidenString( __FUNCTION__, szName );
            if ( NULL == wszName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szNameNew ) 
        {
            wszNameNew = EsetestWidenString( __FUNCTION__, szNameNew );
            if ( NULL == wszNameNew )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szNameNew", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetRenameColumnW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetRenameColumnW = ( PFN_JetRenameColumnW ) ( GetProcAddress( hEseDll, "JetRenameColumnW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetRenameColumnW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetRenameColumnW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetRenameColumnW)( sesid, tableid, wszName, wszNameNew, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetRenameColumn )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetRenameColumn = ( PFN_JetRenameColumn ) ( GetProcAddress( hEseDll, "JetRenameColumn" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetRenameColumn )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetRenameColumn", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetRenameColumn)( sesid, tableid, szName, szNameNew, grbit );
        }
        else
        {

            if ( NULL == pfnJetRenameColumnA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetRenameColumnA = ( PFN_JetRenameColumnA ) ( GetProcAddress( hEseDll, "JetRenameColumnA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetRenameColumnA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetRenameColumnA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetRenameColumnA)( sesid, tableid, szName, szNameNew, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszName, szName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszNameNew, szNameNew );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetSetColumnDefaultValue(
    JET_SESID           sesid,
    JET_DBID            dbid,
    __in JET_PCSTR      szTableName,
    __in JET_PCSTR      szColumnName,
    __in_bcount(cbData) const void          *pvData,
    const unsigned long cbData,
    const JET_GRBIT     grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetSetColumnDefaultValue ) ( 
    JET_SESID           sesid,
    JET_DBID            dbid,
    __in JET_PCSTR      szTableName,
    __in JET_PCSTR      szColumnName,
    __in_bcount(cbData) const void          *pvData,
    const unsigned long cbData,
    const JET_GRBIT     grbit  );

    static PFN_JetSetColumnDefaultValue pfnJetSetColumnDefaultValue = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetSetColumnDefaultValueA ) ( 
    JET_SESID           sesid,
    JET_DBID            dbid,
    __in JET_PCSTR      szTableName,
    __in JET_PCSTR      szColumnName,
    __in_bcount(cbData) const void          *pvData,
    const unsigned long cbData,
    const JET_GRBIT     grbit  );

    static PFN_JetSetColumnDefaultValueA pfnJetSetColumnDefaultValueA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetSetColumnDefaultValueW ) ( 
    JET_SESID           sesid,
    JET_DBID            dbid,
    __in JET_PCWSTR     wszTableName,
    __in JET_PCWSTR     wszColumnName,
    __in_bcount(cbData) const void          *pvData,
    const unsigned long cbData,
    const JET_GRBIT     grbit  );

    static PFN_JetSetColumnDefaultValueW pfnJetSetColumnDefaultValueW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszColumnName   = NULL;
    wchar_t*    wszTableName    = NULL;

    if ( fWiden )
    {
        if( NULL != szColumnName ) 
        {
            wszColumnName = EsetestWidenString( __FUNCTION__, szColumnName );
            if ( NULL == wszColumnName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szColumnName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szTableName ) 
        {
            wszTableName = EsetestWidenString( __FUNCTION__, szTableName );
            if ( NULL == wszTableName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szTableName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetSetColumnDefaultValueW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetSetColumnDefaultValueW = ( PFN_JetSetColumnDefaultValueW ) ( GetProcAddress( hEseDll, "JetSetColumnDefaultValueW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetSetColumnDefaultValueW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetSetColumnDefaultValueW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetSetColumnDefaultValueW)( sesid, dbid, wszTableName, wszColumnName, pvData, cbData, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetSetColumnDefaultValue )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetSetColumnDefaultValue = ( PFN_JetSetColumnDefaultValue ) ( GetProcAddress( hEseDll, "JetSetColumnDefaultValue" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetSetColumnDefaultValue )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetSetColumnDefaultValue", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetSetColumnDefaultValue)( sesid, dbid, szTableName, szColumnName, pvData, cbData, grbit );
        }
        else
        {

            if ( NULL == pfnJetSetColumnDefaultValueA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetSetColumnDefaultValueA = ( PFN_JetSetColumnDefaultValueA ) ( GetProcAddress( hEseDll, "JetSetColumnDefaultValueA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetSetColumnDefaultValueA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetSetColumnDefaultValueA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetSetColumnDefaultValueA)( sesid, dbid, szTableName, szColumnName, pvData, cbData, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszColumnName, szColumnName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszTableName, szTableName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetDeleteIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szIndexName
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetDeleteIndex ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szIndexName  );

    static PFN_JetDeleteIndex pfnJetDeleteIndex = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDeleteIndexA ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szIndexName  );

    static PFN_JetDeleteIndexA pfnJetDeleteIndexA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDeleteIndexW ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCWSTR wszIndexName  );

    static PFN_JetDeleteIndexW pfnJetDeleteIndexW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszIndexName    = NULL;

    if ( fWiden )
    {
        if( NULL != szIndexName ) 
        {
            wszIndexName = EsetestWidenString( __FUNCTION__, szIndexName );
            if ( NULL == wszIndexName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szIndexName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetDeleteIndexW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetDeleteIndexW = ( PFN_JetDeleteIndexW ) ( GetProcAddress( hEseDll, "JetDeleteIndexW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetDeleteIndexW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetDeleteIndexW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetDeleteIndexW)( sesid, tableid, wszIndexName );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetDeleteIndex )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDeleteIndex = ( PFN_JetDeleteIndex ) ( GetProcAddress( hEseDll, "JetDeleteIndex" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDeleteIndex )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDeleteIndex", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDeleteIndex)( sesid, tableid, szIndexName );
        }
        else
        {

            if ( NULL == pfnJetDeleteIndexA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDeleteIndexA = ( PFN_JetDeleteIndexA ) ( GetProcAddress( hEseDll, "JetDeleteIndexA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDeleteIndexA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDeleteIndexA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDeleteIndexA)( sesid, tableid, szIndexName );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszIndexName, szIndexName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetGetDatabaseFileInfo(
    const char      *szDatabaseName,
    void            *pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetGetDatabaseFileInfo ) ( 
    const char      *szDatabaseName,
    void            *pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel  );

    static PFN_JetGetDatabaseFileInfo pfnJetGetDatabaseFileInfo = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetGetDatabaseFileInfoA ) ( 
    const char      *szDatabaseName,
    void            *pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel  );

    static PFN_JetGetDatabaseFileInfoA pfnJetGetDatabaseFileInfoA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetGetDatabaseFileInfoW ) ( 
    const WCHAR     *wszDatabaseName,
    void            *pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel  );

    static PFN_JetGetDatabaseFileInfoW pfnJetGetDatabaseFileInfoW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszDatabaseName = NULL;

    if ( fWiden )
    {
        if( NULL != szDatabaseName ) 
        {
            wszDatabaseName = EsetestWidenString( __FUNCTION__, szDatabaseName );
            if ( NULL == wszDatabaseName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szDatabaseName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetGetDatabaseFileInfoW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetGetDatabaseFileInfoW = ( PFN_JetGetDatabaseFileInfoW ) ( GetProcAddress( hEseDll, "JetGetDatabaseFileInfoW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetGetDatabaseFileInfoW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetGetDatabaseFileInfoW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetGetDatabaseFileInfoW)( wszDatabaseName, pvResult, cbMax, InfoLevel );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetGetDatabaseFileInfo )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetGetDatabaseFileInfo = ( PFN_JetGetDatabaseFileInfo ) ( GetProcAddress( hEseDll, "JetGetDatabaseFileInfo" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetGetDatabaseFileInfo )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetGetDatabaseFileInfo", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetGetDatabaseFileInfo)( szDatabaseName, pvResult, cbMax, InfoLevel );
        }
        else
        {

            if ( NULL == pfnJetGetDatabaseFileInfoA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetGetDatabaseFileInfoA = ( PFN_JetGetDatabaseFileInfoA ) ( GetProcAddress( hEseDll, "JetGetDatabaseFileInfoA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetGetDatabaseFileInfoA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetGetDatabaseFileInfoA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetGetDatabaseFileInfoA)( szDatabaseName, pvResult, cbMax, InfoLevel );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszDatabaseName, szDatabaseName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetOpenDatabase(
    JET_SESID       sesid,
    const char      *szFilename,
    const char      *szConnect,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetOpenDatabase ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    const char      *szConnect,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit  );

    static PFN_JetOpenDatabase pfnJetOpenDatabase = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetOpenDatabaseA ) ( 
    JET_SESID       sesid,
    const char      *szFilename,
    const char      *szConnect,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit  );

    static PFN_JetOpenDatabaseA pfnJetOpenDatabaseA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetOpenDatabaseW ) ( 
    JET_SESID       sesid,
    const WCHAR     *wszFilename,
    const WCHAR     *wszConnect,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit  );

    static PFN_JetOpenDatabaseW pfnJetOpenDatabaseW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszConnect  = NULL;
    wchar_t*    wszFilename = NULL;

    if ( fWiden )
    {
        if( NULL != szConnect ) 
        {
            wszConnect = EsetestWidenString( __FUNCTION__, szConnect );
            if ( NULL == wszConnect )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szConnect", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szFilename ) 
        {
            wszFilename = EsetestWidenString( __FUNCTION__, szFilename );
            if ( NULL == wszFilename )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szFilename", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetOpenDatabaseW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetOpenDatabaseW = ( PFN_JetOpenDatabaseW ) ( GetProcAddress( hEseDll, "JetOpenDatabaseW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetOpenDatabaseW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetOpenDatabaseW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetOpenDatabaseW)( sesid, wszFilename, wszConnect, pdbid, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetOpenDatabase )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetOpenDatabase = ( PFN_JetOpenDatabase ) ( GetProcAddress( hEseDll, "JetOpenDatabase" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetOpenDatabase )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetOpenDatabase", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetOpenDatabase)( sesid, szFilename, szConnect, pdbid, grbit );
        }
        else
        {

            if ( NULL == pfnJetOpenDatabaseA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetOpenDatabaseA = ( PFN_JetOpenDatabaseA ) ( GetProcAddress( hEseDll, "JetOpenDatabaseA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetOpenDatabaseA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetOpenDatabaseA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetOpenDatabaseA)( sesid, szFilename, szConnect, pdbid, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszConnect, szConnect );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszFilename, szFilename );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetOpenTable(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    const void      *pvParameters,
    unsigned long   cbParameters,
    JET_GRBIT       grbit,
    JET_TABLEID     *ptableid
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetOpenTable ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    const void      *pvParameters,
    unsigned long   cbParameters,
    JET_GRBIT       grbit,
    JET_TABLEID     *ptableid  );

    static PFN_JetOpenTable pfnJetOpenTable = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetOpenTableA ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    const void      *pvParameters,
    unsigned long   cbParameters,
    JET_GRBIT       grbit,
    JET_TABLEID     *ptableid  );

    static PFN_JetOpenTableA pfnJetOpenTableA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetOpenTableW ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const WCHAR     *wszTableName,
    const void      *pvParameters,
    unsigned long   cbParameters,
    JET_GRBIT       grbit,
    JET_TABLEID     *ptableid  );

    static PFN_JetOpenTableW pfnJetOpenTableW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszTableName    = NULL;

    if ( fWiden )
    {
        if( NULL != szTableName ) 
        {
            wszTableName = EsetestWidenString( __FUNCTION__, szTableName );
            if ( NULL == wszTableName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szTableName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetOpenTableW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetOpenTableW = ( PFN_JetOpenTableW ) ( GetProcAddress( hEseDll, "JetOpenTableW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetOpenTableW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetOpenTableW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetOpenTableW)( sesid, dbid, wszTableName, pvParameters, cbParameters, grbit, ptableid );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetOpenTable )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetOpenTable = ( PFN_JetOpenTable ) ( GetProcAddress( hEseDll, "JetOpenTable" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetOpenTable )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetOpenTable", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetOpenTable)( sesid, dbid, szTableName, pvParameters, cbParameters, grbit, ptableid );
        }
        else
        {

            if ( NULL == pfnJetOpenTableA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetOpenTableA = ( PFN_JetOpenTableA ) ( GetProcAddress( hEseDll, "JetOpenTableA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetOpenTableA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetOpenTableA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetOpenTableA)( sesid, dbid, szTableName, pvParameters, cbParameters, grbit, ptableid );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszTableName, szTableName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetSetCurrentIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetSetCurrentIndex ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName  );

    static PFN_JetSetCurrentIndex pfnJetSetCurrentIndex = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetSetCurrentIndexA ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName  );

    static PFN_JetSetCurrentIndexA pfnJetSetCurrentIndexA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetSetCurrentIndexW ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const WCHAR     *wszIndexName  );

    static PFN_JetSetCurrentIndexW pfnJetSetCurrentIndexW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszIndexName    = NULL;

    if ( fWiden )
    {
        if( NULL != szIndexName ) 
        {
            wszIndexName = EsetestWidenString( __FUNCTION__, szIndexName );
            if ( NULL == wszIndexName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szIndexName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetSetCurrentIndexW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetSetCurrentIndexW = ( PFN_JetSetCurrentIndexW ) ( GetProcAddress( hEseDll, "JetSetCurrentIndexW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetSetCurrentIndexW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetSetCurrentIndexW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetSetCurrentIndexW)( sesid, tableid, wszIndexName );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetSetCurrentIndex )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetSetCurrentIndex = ( PFN_JetSetCurrentIndex ) ( GetProcAddress( hEseDll, "JetSetCurrentIndex" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetSetCurrentIndex )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetSetCurrentIndex", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetSetCurrentIndex)( sesid, tableid, szIndexName );
        }
        else
        {

            if ( NULL == pfnJetSetCurrentIndexA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetSetCurrentIndexA = ( PFN_JetSetCurrentIndexA ) ( GetProcAddress( hEseDll, "JetSetCurrentIndexA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetSetCurrentIndexA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetSetCurrentIndexA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetSetCurrentIndexA)( sesid, tableid, szIndexName );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszIndexName, szIndexName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetSetCurrentIndex2(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetSetCurrentIndex2 ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_GRBIT       grbit  );

    static PFN_JetSetCurrentIndex2 pfnJetSetCurrentIndex2 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetSetCurrentIndex2A ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_GRBIT       grbit  );

    static PFN_JetSetCurrentIndex2A pfnJetSetCurrentIndex2A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetSetCurrentIndex2W ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const WCHAR     *wszIndexName,
    JET_GRBIT       grbit  );

    static PFN_JetSetCurrentIndex2W pfnJetSetCurrentIndex2W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszIndexName    = NULL;

    if ( fWiden )
    {
        if( NULL != szIndexName ) 
        {
            wszIndexName = EsetestWidenString( __FUNCTION__, szIndexName );
            if ( NULL == wszIndexName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szIndexName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetSetCurrentIndex2W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetSetCurrentIndex2W = ( PFN_JetSetCurrentIndex2W ) ( GetProcAddress( hEseDll, "JetSetCurrentIndex2W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetSetCurrentIndex2W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetSetCurrentIndex2W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetSetCurrentIndex2W)( sesid, tableid, wszIndexName, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetSetCurrentIndex2 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetSetCurrentIndex2 = ( PFN_JetSetCurrentIndex2 ) ( GetProcAddress( hEseDll, "JetSetCurrentIndex2" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetSetCurrentIndex2 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetSetCurrentIndex2", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetSetCurrentIndex2)( sesid, tableid, szIndexName, grbit );
        }
        else
        {

            if ( NULL == pfnJetSetCurrentIndex2A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetSetCurrentIndex2A = ( PFN_JetSetCurrentIndex2A ) ( GetProcAddress( hEseDll, "JetSetCurrentIndex2A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetSetCurrentIndex2A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetSetCurrentIndex2A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetSetCurrentIndex2A)( sesid, tableid, szIndexName, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszIndexName, szIndexName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetSetCurrentIndex3(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_GRBIT       grbit,
    unsigned long   itagSequence
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetSetCurrentIndex3 ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_GRBIT       grbit,
    unsigned long   itagSequence  );

    static PFN_JetSetCurrentIndex3 pfnJetSetCurrentIndex3 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetSetCurrentIndex3A ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_GRBIT       grbit,
    unsigned long   itagSequence  );

    static PFN_JetSetCurrentIndex3A pfnJetSetCurrentIndex3A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetSetCurrentIndex3W ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const WCHAR     *wszIndexName,
    JET_GRBIT       grbit,
    unsigned long   itagSequence  );

    static PFN_JetSetCurrentIndex3W pfnJetSetCurrentIndex3W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszIndexName    = NULL;

    if ( fWiden )
    {
        if( NULL != szIndexName ) 
        {
            wszIndexName = EsetestWidenString( __FUNCTION__, szIndexName );
            if ( NULL == wszIndexName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szIndexName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetSetCurrentIndex3W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetSetCurrentIndex3W = ( PFN_JetSetCurrentIndex3W ) ( GetProcAddress( hEseDll, "JetSetCurrentIndex3W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetSetCurrentIndex3W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetSetCurrentIndex3W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetSetCurrentIndex3W)( sesid, tableid, wszIndexName, grbit, itagSequence );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetSetCurrentIndex3 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetSetCurrentIndex3 = ( PFN_JetSetCurrentIndex3 ) ( GetProcAddress( hEseDll, "JetSetCurrentIndex3" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetSetCurrentIndex3 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetSetCurrentIndex3", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetSetCurrentIndex3)( sesid, tableid, szIndexName, grbit, itagSequence );
        }
        else
        {

            if ( NULL == pfnJetSetCurrentIndex3A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetSetCurrentIndex3A = ( PFN_JetSetCurrentIndex3A ) ( GetProcAddress( hEseDll, "JetSetCurrentIndex3A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetSetCurrentIndex3A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetSetCurrentIndex3A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetSetCurrentIndex3A)( sesid, tableid, szIndexName, grbit, itagSequence );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszIndexName, szIndexName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetSetCurrentIndex4(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_INDEXID     *pindexid,
    JET_GRBIT       grbit,
    unsigned long   itagSequence
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetSetCurrentIndex4 ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_INDEXID     *pindexid,
    JET_GRBIT       grbit,
    unsigned long   itagSequence  );

    static PFN_JetSetCurrentIndex4 pfnJetSetCurrentIndex4 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetSetCurrentIndex4A ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_INDEXID     *pindexid,
    JET_GRBIT       grbit,
    unsigned long   itagSequence  );

    static PFN_JetSetCurrentIndex4A pfnJetSetCurrentIndex4A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetSetCurrentIndex4W ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const WCHAR     *wszIndexName,
    JET_INDEXID     *pindexid,
    JET_GRBIT       grbit,
    unsigned long   itagSequence  );

    static PFN_JetSetCurrentIndex4W pfnJetSetCurrentIndex4W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszIndexName    = NULL;

    if ( fWiden )
    {
        if( NULL != szIndexName ) 
        {
            wszIndexName = EsetestWidenString( __FUNCTION__, szIndexName );
            if ( NULL == wszIndexName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szIndexName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetSetCurrentIndex4W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetSetCurrentIndex4W = ( PFN_JetSetCurrentIndex4W ) ( GetProcAddress( hEseDll, "JetSetCurrentIndex4W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetSetCurrentIndex4W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetSetCurrentIndex4W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetSetCurrentIndex4W)( sesid, tableid, wszIndexName, pindexid, grbit, itagSequence );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetSetCurrentIndex4 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetSetCurrentIndex4 = ( PFN_JetSetCurrentIndex4 ) ( GetProcAddress( hEseDll, "JetSetCurrentIndex4" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetSetCurrentIndex4 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetSetCurrentIndex4", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetSetCurrentIndex4)( sesid, tableid, szIndexName, pindexid, grbit, itagSequence );
        }
        else
        {

            if ( NULL == pfnJetSetCurrentIndex4A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetSetCurrentIndex4A = ( PFN_JetSetCurrentIndex4A ) ( GetProcAddress( hEseDll, "JetSetCurrentIndex4A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetSetCurrentIndex4A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetSetCurrentIndex4A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetSetCurrentIndex4A)( sesid, tableid, szIndexName, pindexid, grbit, itagSequence );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszIndexName, szIndexName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetCompact(
    JET_SESID       sesid,
    const char      *szDatabaseSrc,
    const char      *szDatabaseDest,
    JET_PFNSTATUS   pfnStatus,
    JET_CONVERT     *pconvert,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetCompact ) ( 
    JET_SESID       sesid,
    const char      *szDatabaseSrc,
    const char      *szDatabaseDest,
    JET_PFNSTATUS   pfnStatus,
    JET_CONVERT     *pconvert,
    JET_GRBIT       grbit  );

    static PFN_JetCompact pfnJetCompact = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCompactA ) ( 
    JET_SESID       sesid,
    const char      *szDatabaseSrc,
    const char      *szDatabaseDest,
    JET_PFNSTATUS   pfnStatus,
    JET_CONVERT     *pconvert,
    JET_GRBIT       grbit  );

    static PFN_JetCompactA pfnJetCompactA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCompactW ) ( 
    JET_SESID       sesid,
    const WCHAR     *wszDatabaseSrc,
    const WCHAR     *wszDatabaseDest,
    JET_PFNSTATUS   pfnStatus,
    JET_CONVERT     *pconvert,
    JET_GRBIT       grbit  );

    static PFN_JetCompactW pfnJetCompactW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    JET_CONVERT_W*  wpconvert   = NULL;
    wchar_t*    wszDatabaseSrc  = NULL;
    wchar_t*    wszDatabaseDest = NULL;

    if ( fWiden )
    {
        if( NULL != pconvert ) 
        {
            wpconvert = EsetestWidenJET_CONVERT( __FUNCTION__, pconvert );
            if ( NULL == wpconvert )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "pconvert", "EsetestWidenJET_CONVERT" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szDatabaseSrc ) 
        {
            wszDatabaseSrc = EsetestWidenString( __FUNCTION__, szDatabaseSrc );
            if ( NULL == wszDatabaseSrc )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szDatabaseSrc", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szDatabaseDest ) 
        {
            wszDatabaseDest = EsetestWidenString( __FUNCTION__, szDatabaseDest );
            if ( NULL == wszDatabaseDest )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szDatabaseDest", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetCompactW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCompactW = ( PFN_JetCompactW ) ( GetProcAddress( hEseDll, "JetCompactW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCompactW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCompactW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCompactW)( sesid, wszDatabaseSrc, wszDatabaseDest, pfnStatus, pconvert, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetCompact )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCompact = ( PFN_JetCompact ) ( GetProcAddress( hEseDll, "JetCompact" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCompact )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCompact", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCompact)( sesid, szDatabaseSrc, szDatabaseDest, pfnStatus, pconvert, grbit );
        }
        else
        {

            if ( NULL == pfnJetCompactA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCompactA = ( PFN_JetCompactA ) ( GetProcAddress( hEseDll, "JetCompactA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCompactA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCompactA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCompactA)( sesid, szDatabaseSrc, szDatabaseDest, pfnStatus, pconvert, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestUnwidenJET_CONVERT( __FUNCTION__, wpconvert, pconvert );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenJET_CONVERT" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszDatabaseSrc, szDatabaseSrc );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszDatabaseDest, szDatabaseDest );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetDefragment(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetDefragment ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_GRBIT       grbit  );

    static PFN_JetDefragment pfnJetDefragment = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDefragmentA ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_GRBIT       grbit  );

    static PFN_JetDefragmentA pfnJetDefragmentA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDefragmentW ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const WCHAR     *wszTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_GRBIT       grbit  );

    static PFN_JetDefragmentW pfnJetDefragmentW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszTableName    = NULL;

    if ( fWiden )
    {
        if( NULL != szTableName ) 
        {
            wszTableName = EsetestWidenString( __FUNCTION__, szTableName );
            if ( NULL == wszTableName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szTableName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetDefragmentW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetDefragmentW = ( PFN_JetDefragmentW ) ( GetProcAddress( hEseDll, "JetDefragmentW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetDefragmentW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetDefragmentW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetDefragmentW)( sesid, dbid, wszTableName, pcPasses, pcSeconds, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetDefragment )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDefragment = ( PFN_JetDefragment ) ( GetProcAddress( hEseDll, "JetDefragment" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDefragment )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDefragment", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDefragment)( sesid, dbid, szTableName, pcPasses, pcSeconds, grbit );
        }
        else
        {

            if ( NULL == pfnJetDefragmentA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDefragmentA = ( PFN_JetDefragmentA ) ( GetProcAddress( hEseDll, "JetDefragmentA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDefragmentA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDefragmentA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDefragmentA)( sesid, dbid, szTableName, pcPasses, pcSeconds, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszTableName, szTableName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetDefragment2(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_CALLBACK    callback,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetDefragment2 ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_CALLBACK    callback,
    JET_GRBIT       grbit  );

    static PFN_JetDefragment2 pfnJetDefragment2 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDefragment2A ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_CALLBACK    callback,
    JET_GRBIT       grbit  );

    static PFN_JetDefragment2A pfnJetDefragment2A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDefragment2W ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    const WCHAR     *wszTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_CALLBACK    callback,
    JET_GRBIT       grbit  );

    static PFN_JetDefragment2W pfnJetDefragment2W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszTableName    = NULL;

    if ( fWiden )
    {
        if( NULL != szTableName ) 
        {
            wszTableName = EsetestWidenString( __FUNCTION__, szTableName );
            if ( NULL == wszTableName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szTableName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetDefragment2W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetDefragment2W = ( PFN_JetDefragment2W ) ( GetProcAddress( hEseDll, "JetDefragment2W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetDefragment2W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetDefragment2W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetDefragment2W)( sesid, dbid, wszTableName, pcPasses, pcSeconds, callback, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetDefragment2 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDefragment2 = ( PFN_JetDefragment2 ) ( GetProcAddress( hEseDll, "JetDefragment2" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDefragment2 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDefragment2", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDefragment2)( sesid, dbid, szTableName, pcPasses, pcSeconds, callback, grbit );
        }
        else
        {

            if ( NULL == pfnJetDefragment2A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDefragment2A = ( PFN_JetDefragment2A ) ( GetProcAddress( hEseDll, "JetDefragment2A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDefragment2A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDefragment2A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDefragment2A)( sesid, dbid, szTableName, pcPasses, pcSeconds, callback, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszTableName, szTableName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetDefragment3(
    JET_SESID       vsesid,
    const char      *szDatabaseName,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_CALLBACK    callback,
    void            *pvContext,
    JET_GRBIT       grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetDefragment3 ) ( 
    JET_SESID       vsesid,
    const char      *szDatabaseName,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_CALLBACK    callback,
    void            *pvContext,
    JET_GRBIT       grbit  );

    static PFN_JetDefragment3 pfnJetDefragment3 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDefragment3A ) ( 
    JET_SESID       vsesid,
    const char      *szDatabaseName,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_CALLBACK    callback,
    void            *pvContext,
    JET_GRBIT       grbit  );

    static PFN_JetDefragment3A pfnJetDefragment3A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetDefragment3W ) ( 
    JET_SESID       vsesid,
    const WCHAR     *wszDatabaseName,
    const WCHAR     *wszTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_CALLBACK    callback,
    void            *pvContext,
    JET_GRBIT       grbit  );

    static PFN_JetDefragment3W pfnJetDefragment3W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszTableName    = NULL;
    wchar_t*    wszDatabaseName = NULL;

    if ( fWiden )
    {
        if( NULL != szTableName ) 
        {
            wszTableName = EsetestWidenString( __FUNCTION__, szTableName );
            if ( NULL == wszTableName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szTableName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szDatabaseName ) 
        {
            wszDatabaseName = EsetestWidenString( __FUNCTION__, szDatabaseName );
            if ( NULL == wszDatabaseName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szDatabaseName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetDefragment3W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetDefragment3W = ( PFN_JetDefragment3W ) ( GetProcAddress( hEseDll, "JetDefragment3W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetDefragment3W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetDefragment3W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetDefragment3W)( vsesid, wszDatabaseName, wszTableName, pcPasses, pcSeconds, callback, pvContext, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetDefragment3 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDefragment3 = ( PFN_JetDefragment3 ) ( GetProcAddress( hEseDll, "JetDefragment3" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDefragment3 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDefragment3", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDefragment3)( vsesid, szDatabaseName, szTableName, pcPasses, pcSeconds, callback, pvContext, grbit );
        }
        else
        {

            if ( NULL == pfnJetDefragment3A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetDefragment3A = ( PFN_JetDefragment3A ) ( GetProcAddress( hEseDll, "JetDefragment3A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetDefragment3A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetDefragment3A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetDefragment3A)( vsesid, szDatabaseName, szTableName, pcPasses, pcSeconds, callback, pvContext, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszTableName, szTableName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszDatabaseName, szDatabaseName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetUpgradeDatabase(
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const JET_GRBIT grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetUpgradeDatabase ) ( 
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const JET_GRBIT grbit  );

    static PFN_JetUpgradeDatabase pfnJetUpgradeDatabase = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetUpgradeDatabaseA ) ( 
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const JET_GRBIT grbit  );

    static PFN_JetUpgradeDatabaseA pfnJetUpgradeDatabaseA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetUpgradeDatabaseW ) ( 
    JET_SESID       sesid,
    const WCHAR     *wszDbFileName,
    const WCHAR     *wszSLVFileName,
    const JET_GRBIT grbit  );

    static PFN_JetUpgradeDatabaseW pfnJetUpgradeDatabaseW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszSLVFileName  = NULL;
    wchar_t*    wszDbFileName   = NULL;

    if ( fWiden )
    {
        if( NULL != szSLVFileName ) 
        {
            wszSLVFileName = EsetestWidenString( __FUNCTION__, szSLVFileName );
            if ( NULL == wszSLVFileName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szSLVFileName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szDbFileName ) 
        {
            wszDbFileName = EsetestWidenString( __FUNCTION__, szDbFileName );
            if ( NULL == wszDbFileName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szDbFileName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetUpgradeDatabaseW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetUpgradeDatabaseW = ( PFN_JetUpgradeDatabaseW ) ( GetProcAddress( hEseDll, "JetUpgradeDatabaseW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetUpgradeDatabaseW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetUpgradeDatabaseW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetUpgradeDatabaseW)( sesid, wszDbFileName, wszSLVFileName, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetUpgradeDatabase )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetUpgradeDatabase = ( PFN_JetUpgradeDatabase ) ( GetProcAddress( hEseDll, "JetUpgradeDatabase" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetUpgradeDatabase )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetUpgradeDatabase", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetUpgradeDatabase)( sesid, szDbFileName, szSLVFileName, grbit );
        }
        else
        {

            if ( NULL == pfnJetUpgradeDatabaseA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetUpgradeDatabaseA = ( PFN_JetUpgradeDatabaseA ) ( GetProcAddress( hEseDll, "JetUpgradeDatabaseA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetUpgradeDatabaseA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetUpgradeDatabaseA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetUpgradeDatabaseA)( sesid, szDbFileName, szSLVFileName, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszSLVFileName, szSLVFileName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszDbFileName, szDbFileName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetBackup(
    const char *szBackupPath,
    JET_GRBIT grbit,
    JET_PFNSTATUS pfnStatus
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetBackup ) (  const char *szBackupPath, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus  );

    static PFN_JetBackup pfnJetBackup = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetBackupA ) (  const char *szBackupPath, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus  );

    static PFN_JetBackupA pfnJetBackupA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetBackupW ) (  const WCHAR *wszBackupPath, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus  );

    static PFN_JetBackupW pfnJetBackupW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszBackupPath   = NULL;

    if ( fWiden )
    {
        if( NULL != szBackupPath ) 
        {
            wszBackupPath = EsetestWidenString( __FUNCTION__, szBackupPath );
            if ( NULL == wszBackupPath )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szBackupPath", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetBackupW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetBackupW = ( PFN_JetBackupW ) ( GetProcAddress( hEseDll, "JetBackupW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetBackupW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetBackupW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetBackupW)( wszBackupPath, grbit, pfnStatus );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetBackup )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetBackup = ( PFN_JetBackup ) ( GetProcAddress( hEseDll, "JetBackup" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetBackup )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetBackup", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetBackup)( szBackupPath, grbit, pfnStatus );
        }
        else
        {

            if ( NULL == pfnJetBackupA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetBackupA = ( PFN_JetBackupA ) ( GetProcAddress( hEseDll, "JetBackupA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetBackupA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetBackupA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetBackupA)( szBackupPath, grbit, pfnStatus );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszBackupPath, szBackupPath );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetBackupInstance(
    JET_INSTANCE    instance,
    const char      *szBackupPath,
    JET_GRBIT       grbit,
    JET_PFNSTATUS   pfnStatus
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetBackupInstance ) (  
        JET_INSTANCE    instance,
        const char      *szBackupPath,
        JET_GRBIT       grbit,
        JET_PFNSTATUS   pfnStatus  );

    static PFN_JetBackupInstance pfnJetBackupInstance = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetBackupInstanceA ) (     
        JET_INSTANCE    instance,
        const char      *szBackupPath,
        JET_GRBIT       grbit,
        JET_PFNSTATUS   pfnStatus  );

    static PFN_JetBackupInstanceA pfnJetBackupInstanceA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetBackupInstanceW ) (     
        JET_INSTANCE    instance,
        const WCHAR     *wszBackupPath,
        JET_GRBIT       grbit,
        JET_PFNSTATUS   pfnStatus  );

    static PFN_JetBackupInstanceW pfnJetBackupInstanceW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszBackupPath   = NULL;

    if ( fWiden )
    {
        if( NULL != szBackupPath ) 
        {
            wszBackupPath = EsetestWidenString( __FUNCTION__, szBackupPath );
            if ( NULL == wszBackupPath )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szBackupPath", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetBackupInstanceW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetBackupInstanceW = ( PFN_JetBackupInstanceW ) ( GetProcAddress( hEseDll, "JetBackupInstanceW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetBackupInstanceW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetBackupInstanceW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetBackupInstanceW)( instance, wszBackupPath, grbit, pfnStatus );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetBackupInstance )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetBackupInstance = ( PFN_JetBackupInstance ) ( GetProcAddress( hEseDll, "JetBackupInstance" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetBackupInstance )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetBackupInstance", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetBackupInstance)( instance, szBackupPath, grbit, pfnStatus );
        }
        else
        {

            if ( NULL == pfnJetBackupInstanceA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetBackupInstanceA = ( PFN_JetBackupInstanceA ) ( GetProcAddress( hEseDll, "JetBackupInstanceA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetBackupInstanceA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetBackupInstanceA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetBackupInstanceA)( instance, szBackupPath, grbit, pfnStatus );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszBackupPath, szBackupPath );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetRestore(
    __in JET_PCSTR      sz,
    JET_PFNSTATUS       pfn
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetRestore ) ( 
    __in JET_PCSTR      sz,
    JET_PFNSTATUS       pfn  );

    static PFN_JetRestore pfnJetRestore = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetRestoreA ) ( 
    __in JET_PCSTR      sz,
    JET_PFNSTATUS       pfn  );

    static PFN_JetRestoreA pfnJetRestoreA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetRestoreW ) ( 
    __in JET_PCWSTR     wsz,
    JET_PFNSTATUS       pfn  );

    static PFN_JetRestoreW pfnJetRestoreW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wsz = NULL;

    if ( fWiden )
    {
        if( NULL != sz ) 
        {
            wsz = EsetestWidenString( __FUNCTION__, sz );
            if ( NULL == wsz )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "sz", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetRestoreW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetRestoreW = ( PFN_JetRestoreW ) ( GetProcAddress( hEseDll, "JetRestoreW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetRestoreW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetRestoreW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetRestoreW)( wsz, pfn );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetRestore )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetRestore = ( PFN_JetRestore ) ( GetProcAddress( hEseDll, "JetRestore" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetRestore )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetRestore", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetRestore)( sz, pfn );
        }
        else
        {

            if ( NULL == pfnJetRestoreA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetRestoreA = ( PFN_JetRestoreA ) ( GetProcAddress( hEseDll, "JetRestoreA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetRestoreA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetRestoreA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetRestoreA)( sz, pfn );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wsz, sz );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetRestore2(
    JET_PCSTR           sz,
    JET_PCSTR           szDest,
    JET_PFNSTATUS       pfn
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetRestore2 ) ( 
    JET_PCSTR           sz,
    JET_PCSTR           szDest,
    JET_PFNSTATUS       pfn  );

    static PFN_JetRestore2 pfnJetRestore2 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetRestore2A ) ( 
    JET_PCSTR           sz,
    JET_PCSTR           szDest,
    JET_PFNSTATUS       pfn  );

    static PFN_JetRestore2A pfnJetRestore2A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetRestore2W ) ( 
    JET_PCWSTR          wsz,
    JET_PCWSTR          wszDest,
    JET_PFNSTATUS       pfn  );

    static PFN_JetRestore2W pfnJetRestore2W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wsz = NULL;
    wchar_t*    wszDest = NULL;

    if ( fWiden )
    {
        if( NULL != sz ) 
        {
            wsz = EsetestWidenString( __FUNCTION__, sz );
            if ( NULL == wsz )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "sz", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szDest ) 
        {
            wszDest = EsetestWidenString( __FUNCTION__, szDest );
            if ( NULL == wszDest )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szDest", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetRestore2W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetRestore2W = ( PFN_JetRestore2W ) ( GetProcAddress( hEseDll, "JetRestore2W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetRestore2W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetRestore2W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetRestore2W)( wsz, wszDest, pfn );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetRestore2 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetRestore2 = ( PFN_JetRestore2 ) ( GetProcAddress( hEseDll, "JetRestore2" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetRestore2 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetRestore2", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetRestore2)( sz, szDest, pfn );
        }
        else
        {

            if ( NULL == pfnJetRestore2A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetRestore2A = ( PFN_JetRestore2A ) ( GetProcAddress( hEseDll, "JetRestore2A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetRestore2A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetRestore2A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetRestore2A)( sz, szDest, pfn );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wsz, sz );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszDest, szDest );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetRestoreInstance(
    JET_INSTANCE instance,
    const char *sz,
    const char *szDest,
    JET_PFNSTATUS pfn
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetRestoreInstance ) (     
        JET_INSTANCE instance,
        const char *sz,
        const char *szDest,
        JET_PFNSTATUS pfn  );

    static PFN_JetRestoreInstance pfnJetRestoreInstance = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetRestoreInstanceA ) (    
        JET_INSTANCE instance,
        const char *sz,
        const char *szDest,
        JET_PFNSTATUS pfn  );

    static PFN_JetRestoreInstanceA pfnJetRestoreInstanceA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetRestoreInstanceW ) (    
        JET_INSTANCE instance,
        const WCHAR *wsz,
        const WCHAR *wszDest,
        JET_PFNSTATUS pfn  );

    static PFN_JetRestoreInstanceW pfnJetRestoreInstanceW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wsz = NULL;
    wchar_t*    wszDest = NULL;

    if ( fWiden )
    {
        if( NULL != sz ) 
        {
            wsz = EsetestWidenString( __FUNCTION__, sz );
            if ( NULL == wsz )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "sz", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szDest ) 
        {
            wszDest = EsetestWidenString( __FUNCTION__, szDest );
            if ( NULL == wszDest )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szDest", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetRestoreInstanceW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetRestoreInstanceW = ( PFN_JetRestoreInstanceW ) ( GetProcAddress( hEseDll, "JetRestoreInstanceW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetRestoreInstanceW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetRestoreInstanceW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetRestoreInstanceW)( instance, wsz, wszDest, pfn );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetRestoreInstance )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetRestoreInstance = ( PFN_JetRestoreInstance ) ( GetProcAddress( hEseDll, "JetRestoreInstance" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetRestoreInstance )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetRestoreInstance", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetRestoreInstance)( instance, sz, szDest, pfn );
        }
        else
        {

            if ( NULL == pfnJetRestoreInstanceA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetRestoreInstanceA = ( PFN_JetRestoreInstanceA ) ( GetProcAddress( hEseDll, "JetRestoreInstanceA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetRestoreInstanceA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetRestoreInstanceA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetRestoreInstanceA)( instance, sz, szDest, pfn );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wsz, sz );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszDest, szDest );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetOpenFile(
    const char *szFileName,
    JET_HANDLE  *phfFile,
    unsigned long *pulFileSizeLow,
    unsigned long *pulFileSizeHigh
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetOpenFile ) (  const char *szFileName,
    JET_HANDLE  *phfFile,
    unsigned long *pulFileSizeLow,
    unsigned long *pulFileSizeHigh  );

    static PFN_JetOpenFile pfnJetOpenFile = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetOpenFileA ) (  const char *szFileName,
    JET_HANDLE  *phfFile,
    unsigned long *pulFileSizeLow,
    unsigned long *pulFileSizeHigh  );

    static PFN_JetOpenFileA pfnJetOpenFileA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetOpenFileW ) (  const WCHAR *wszFileName,
    JET_HANDLE  *phfFile,
    unsigned long *pulFileSizeLow,
    unsigned long *pulFileSizeHigh  );

    static PFN_JetOpenFileW pfnJetOpenFileW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszFileName = NULL;

    if ( fWiden )
    {
        if( NULL != szFileName ) 
        {
            wszFileName = EsetestWidenString( __FUNCTION__, szFileName );
            if ( NULL == wszFileName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szFileName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetOpenFileW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetOpenFileW = ( PFN_JetOpenFileW ) ( GetProcAddress( hEseDll, "JetOpenFileW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetOpenFileW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetOpenFileW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetOpenFileW)( wszFileName, phfFile, pulFileSizeLow, pulFileSizeHigh );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetOpenFile )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetOpenFile = ( PFN_JetOpenFile ) ( GetProcAddress( hEseDll, "JetOpenFile" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetOpenFile )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetOpenFile", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetOpenFile)( szFileName, phfFile, pulFileSizeLow, pulFileSizeHigh );
        }
        else
        {

            if ( NULL == pfnJetOpenFileA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetOpenFileA = ( PFN_JetOpenFileA ) ( GetProcAddress( hEseDll, "JetOpenFileA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetOpenFileA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetOpenFileA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetOpenFileA)( szFileName, phfFile, pulFileSizeLow, pulFileSizeHigh );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszFileName, szFileName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetOpenFileInstance(
    JET_INSTANCE instance,
    const char *szFileName,
    JET_HANDLE  *phfFile,
    unsigned long *pulFileSizeLow,
    unsigned long *pulFileSizeHigh
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetOpenFileInstance ) (  JET_INSTANCE instance,
                const char *szFileName,
                JET_HANDLE  *phfFile,
                unsigned long *pulFileSizeLow,
                unsigned long *pulFileSizeHigh  );

    static PFN_JetOpenFileInstance pfnJetOpenFileInstance = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetOpenFileInstanceA ) (  JET_INSTANCE instance,
                const char *szFileName,
                JET_HANDLE  *phfFile,
                unsigned long *pulFileSizeLow,
                unsigned long *pulFileSizeHigh  );

    static PFN_JetOpenFileInstanceA pfnJetOpenFileInstanceA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetOpenFileInstanceW ) (  JET_INSTANCE instance,
                const WCHAR *wszFileName,
                JET_HANDLE  *phfFile,
                unsigned long *pulFileSizeLow,
                unsigned long *pulFileSizeHigh  );

    static PFN_JetOpenFileInstanceW pfnJetOpenFileInstanceW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszFileName = NULL;

    if ( fWiden )
    {
        if( NULL != szFileName ) 
        {
            wszFileName = EsetestWidenString( __FUNCTION__, szFileName );
            if ( NULL == wszFileName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szFileName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetOpenFileInstanceW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetOpenFileInstanceW = ( PFN_JetOpenFileInstanceW ) ( GetProcAddress( hEseDll, "JetOpenFileInstanceW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetOpenFileInstanceW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetOpenFileInstanceW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetOpenFileInstanceW)( instance, wszFileName, phfFile, pulFileSizeLow, pulFileSizeHigh );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetOpenFileInstance )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetOpenFileInstance = ( PFN_JetOpenFileInstance ) ( GetProcAddress( hEseDll, "JetOpenFileInstance" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetOpenFileInstance )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetOpenFileInstance", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetOpenFileInstance)( instance, szFileName, phfFile, pulFileSizeLow, pulFileSizeHigh );
        }
        else
        {

            if ( NULL == pfnJetOpenFileInstanceA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetOpenFileInstanceA = ( PFN_JetOpenFileInstanceA ) ( GetProcAddress( hEseDll, "JetOpenFileInstanceA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetOpenFileInstanceA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetOpenFileInstanceA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetOpenFileInstanceA)( instance, szFileName, phfFile, pulFileSizeLow, pulFileSizeHigh );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszFileName, szFileName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetOpenFileSectionInstance(
    JET_INSTANCE    instance,
    __in JET_PSTR   szFile,
    JET_HANDLE *    phFile,
    long        iSection,
    long        cSections,
    unsigned long * pulSectionSizeLow,
    long *      plSectionSizeHigh
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetOpenFileSectionInstance ) ( 
    JET_INSTANCE    instance,
    __in JET_PSTR   szFile,
    JET_HANDLE *    phFile,
    long        iSection,
    long        cSections,
    unsigned long * pulSectionSizeLow,
    long *      plSectionSizeHigh  );

    static PFN_JetOpenFileSectionInstance pfnJetOpenFileSectionInstance = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetOpenFileSectionInstanceA ) ( 
    JET_INSTANCE    instance,
    __in JET_PSTR   szFile,
    JET_HANDLE *    phFile,
    long        iSection,
    long        cSections,
    unsigned long * pulSectionSizeLow,
    long *      plSectionSizeHigh  );

    static PFN_JetOpenFileSectionInstanceA pfnJetOpenFileSectionInstanceA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetOpenFileSectionInstanceW ) ( 
    JET_INSTANCE    instance,
    __in JET_PWSTR  wszFile,
    JET_HANDLE *    phFile,
    long        iSection,
    long        cSections,
    unsigned long * pulSectionSizeLow,
    long *      plSectionSizeHigh  );

    static PFN_JetOpenFileSectionInstanceW pfnJetOpenFileSectionInstanceW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszFile = NULL;

    if ( fWiden )
    {
        if( NULL != szFile ) 
        {
            wszFile = EsetestWidenString( __FUNCTION__, szFile );
            if ( NULL == wszFile )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szFile", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetOpenFileSectionInstanceW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetOpenFileSectionInstanceW = ( PFN_JetOpenFileSectionInstanceW ) ( GetProcAddress( hEseDll, "JetOpenFileSectionInstanceW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetOpenFileSectionInstanceW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetOpenFileSectionInstanceW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetOpenFileSectionInstanceW)( instance, wszFile, phFile, iSection, cSections, pulSectionSizeLow, plSectionSizeHigh );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetOpenFileSectionInstance )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetOpenFileSectionInstance = ( PFN_JetOpenFileSectionInstance ) ( GetProcAddress( hEseDll, "JetOpenFileSectionInstance" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetOpenFileSectionInstance )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetOpenFileSectionInstance", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetOpenFileSectionInstance)( instance, szFile, phFile, iSection, cSections, pulSectionSizeLow, plSectionSizeHigh );
        }
        else
        {

            if ( NULL == pfnJetOpenFileSectionInstanceA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetOpenFileSectionInstanceA = ( PFN_JetOpenFileSectionInstanceA ) ( GetProcAddress( hEseDll, "JetOpenFileSectionInstanceA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetOpenFileSectionInstanceA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetOpenFileSectionInstanceA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetOpenFileSectionInstanceA)( instance, szFile, phFile, iSection, cSections, pulSectionSizeLow, plSectionSizeHigh );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszFile, szFile );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



JET_ERR
BounceJetGetLogFileInfo(
    char *szLog,
    __out_bcount(cbMax) void *pvResult,
    const unsigned long cbMax,
    const unsigned long InfoLevel
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetGetLogFileInfo ) (  char *szLog, 
    __out_bcount(cbMax) void *pvResult,
    const unsigned long cbMax,
    const unsigned long InfoLevel  );

    static PFN_JetGetLogFileInfo pfnJetGetLogFileInfo = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetGetLogFileInfoA ) (  char *szLog, 
    __out_bcount(cbMax) void *pvResult,
    const unsigned long cbMax,
    const unsigned long InfoLevel  );

    static PFN_JetGetLogFileInfoA pfnJetGetLogFileInfoA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetGetLogFileInfoW ) (  WCHAR *wszLog, 
    __out_bcount(cbMax) void *pvResult,
    const unsigned long cbMax,
    const unsigned long InfoLevel  );

    static PFN_JetGetLogFileInfoW pfnJetGetLogFileInfoW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszLog  = NULL;

    if ( fWiden )
    {
        if( NULL != szLog ) 
        {
            wszLog = EsetestWidenString( __FUNCTION__, szLog );
            if ( NULL == wszLog )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szLog", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetGetLogFileInfoW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetGetLogFileInfoW = ( PFN_JetGetLogFileInfoW ) ( GetProcAddress( hEseDll, "JetGetLogFileInfoW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetGetLogFileInfoW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetGetLogFileInfoW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetGetLogFileInfoW)( wszLog, pvResult, cbMax, InfoLevel );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetGetLogFileInfo )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetGetLogFileInfo = ( PFN_JetGetLogFileInfo ) ( GetProcAddress( hEseDll, "JetGetLogFileInfo" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetGetLogFileInfo )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetGetLogFileInfo", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetGetLogFileInfo)( szLog, pvResult, cbMax, InfoLevel );
        }
        else
        {

            if ( NULL == pfnJetGetLogFileInfoA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetGetLogFileInfoA = ( PFN_JetGetLogFileInfoA ) ( GetProcAddress( hEseDll, "JetGetLogFileInfoA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetGetLogFileInfoA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetGetLogFileInfoA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetGetLogFileInfoA)( szLog, pvResult, cbMax, InfoLevel );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszLog, szLog );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestCleanupWidenString" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}



bool FEsetestAlwaysNarrow()
{
    return false;
}


