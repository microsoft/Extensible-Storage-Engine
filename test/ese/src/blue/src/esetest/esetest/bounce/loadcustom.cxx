// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "ese_common.hxx"
#include "esetest_bounce.hxx"
#include <stdlib.h>
#include <strsafe.h>
#include <windows.h>

JET_ERR
BounceJetEnableMultiInstance(
    JET_SETSYSPARAM *   psetsysparam,
    unsigned long       csetsysparam,
    unsigned long *     pcsetsucceed
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetEnableMultiInstance ) (     JET_SETSYSPARAM *   psetsysparam,
                                            unsigned long       csetsysparam,
                                            unsigned long *     pcsetsucceed );

    static PFN_JetEnableMultiInstance pfnJetEnableMultiInstance = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetEnableMultiInstanceA ) (    JET_SETSYSPARAM *   psetsysparam,
                                            unsigned long       csetsysparam,
                                            unsigned long *     pcsetsucceed );

    static PFN_JetEnableMultiInstanceA pfnJetEnableMultiInstanceA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetEnableMultiInstanceW ) (    JET_SETSYSPARAM_W * psetsysparam,
                                            unsigned long       csetsysparam,
                                            unsigned long *     pcsetsucceed );

    static PFN_JetEnableMultiInstanceW pfnJetEnableMultiInstanceW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    JET_SETSYSPARAM_W*  wpsetsysparam   = NULL;

    if ( fWiden )
    {
        wpsetsysparam = EsetestWidenJET_SETSYSPARAM( __FUNCTION__, psetsysparam, csetsysparam );
        if ( NULL == wpsetsysparam )
        {
            tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "psetsysparam", "EsetestWidenJET_SETSYSPARAM" );
            err = JET_errOutOfMemory;
            goto Cleanup;
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetEnableMultiInstanceW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetEnableMultiInstanceW = ( PFN_JetEnableMultiInstanceW ) ( GetProcAddress( hEseDll, "JetEnableMultiInstanceW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetEnableMultiInstanceW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetEnableMultiInstanceW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetEnableMultiInstanceW)( wpsetsysparam, csetsysparam, pcsetsucceed );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetEnableMultiInstance )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetEnableMultiInstance = ( PFN_JetEnableMultiInstance ) ( GetProcAddress( hEseDll, "JetEnableMultiInstance" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetEnableMultiInstance )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetEnableMultiInstance", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetEnableMultiInstance)( psetsysparam, csetsysparam, pcsetsucceed );
        }
        else
        {

            if ( NULL == pfnJetEnableMultiInstanceA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetEnableMultiInstanceA = ( PFN_JetEnableMultiInstanceA ) ( GetProcAddress( hEseDll, "JetEnableMultiInstanceA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetEnableMultiInstanceA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetEnableMultiInstanceA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetEnableMultiInstanceA)( psetsysparam, csetsysparam, pcsetsucceed );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestUnwidenJET_SETSYSPARAM( __FUNCTION__, wpsetsysparam, csetsysparam, psetsysparam );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenJET_SETSYSPARAM" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }
    return err;
}




JET_ERR
BounceJetCreateTableColumnIndex(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE *ptablecreate
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetCreateTableColumnIndex ) (
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE *ptablecreate  );

    static PFN_JetCreateTableColumnIndex pfnJetCreateTableColumnIndex = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateTableColumnIndexA ) (
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE *ptablecreate  );

    static PFN_JetCreateTableColumnIndexA pfnJetCreateTableColumnIndexA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateTableColumnIndexW ) (
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE_W   *ptablecreate  );

    static PFN_JetCreateTableColumnIndexW pfnJetCreateTableColumnIndexW = NULL;
    bool    fWiden = FEsetestWidenParameters();

    bool    fChanged    = false;
    JET_TABLECREATE*    ptablecreateCompatible  = NULL;

    JET_TABLECREATE_W* wptablecreate    = NULL;
    JET_TABLECREATE*  ptablecreateT     = NULL;

    bool fAllocated = false;
    JET_COLUMNCREATE* pcolumncreateInternal = NULL;
    if ( ptablecreate ){
        pcolumncreateInternal = ptablecreate->rgcolumncreate;
        ptablecreate->rgcolumncreate = EsetestCompressJET_COLUMNCREATE( pcolumncreateInternal, ptablecreate->cColumns, &fAllocated );
    }

    Call( EsetestAdaptJET_TABLECREATE( __FUNCTION__, ptablecreate, &ptablecreateCompatible, &fChanged ) );
    if ( fChanged && NULL == ptablecreateCompatible )
    {
        tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "ptablecreate", "EsetestAdaptJET_TABLECREATE" );
        err = JET_errOutOfMemory;
        goto Cleanup;
    }

    ptablecreateT = fChanged ? ptablecreateCompatible : ptablecreate;

    if ( fWiden )
    {
        wptablecreate = EsetestWidenJET_TABLECREATE( __FUNCTION__, ptablecreateT );
        if ( NULL == wptablecreate )
        {
            tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "ptablecreate", "EsetestWidenJET_TABLECREATE" );
            err = JET_errOutOfMemory;
            goto Cleanup;
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetCreateTableColumnIndexW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateTableColumnIndexW = ( PFN_JetCreateTableColumnIndexW ) ( GetProcAddress( hEseDll, "JetCreateTableColumnIndexW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateTableColumnIndexW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateTableColumnIndexW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateTableColumnIndexW)( sesid, dbid, wptablecreate );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetCreateTableColumnIndex )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateTableColumnIndex = ( PFN_JetCreateTableColumnIndex ) ( GetProcAddress( hEseDll, "JetCreateTableColumnIndex" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateTableColumnIndex )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateTableColumnIndex", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateTableColumnIndex)( sesid, dbid, ptablecreateT );
        }
        else
        {

            if ( NULL == pfnJetCreateTableColumnIndexA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateTableColumnIndexA = ( PFN_JetCreateTableColumnIndexA ) ( GetProcAddress( hEseDll, "JetCreateTableColumnIndexA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateTableColumnIndexA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateTableColumnIndexA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateTableColumnIndexA)( sesid, dbid, ptablecreateT );
        }
    }


Cleanup:

    if ( fWiden )
    {
        
        const JET_ERR errT = EsetestUnwidenJET_TABLECREATE( __FUNCTION__, wptablecreate, ptablecreateT );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenJET_TABLECREATE" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
        
    }

    if ( fChanged )
    {
        const JET_ERR errT = EsetestUnadaptJET_TABLECREATE( __FUNCTION__, ptablecreate, &ptablecreateCompatible );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnadaptJET_TABLECREATE" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }

    if ( fAllocated ){
        for ( unsigned long i = 0 ; i < ptablecreate->cColumns ; i++ ){
            pcolumncreateInternal[ i ].columnid = ptablecreate->rgcolumncreate[ i ].columnid;
            pcolumncreateInternal[ i ].err = ptablecreate->rgcolumncreate[ i ].err;
        }
        delete []( ptablecreate->rgcolumncreate );
        ptablecreate->rgcolumncreate = pcolumncreateInternal;
    }

    return err;
}



JET_ERR
BounceJetCreateTableColumnIndex2(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE2    *ptablecreate
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetCreateTableColumnIndex2 ) (
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE2    *ptablecreate  );

    static PFN_JetCreateTableColumnIndex2 pfnJetCreateTableColumnIndex2 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateTableColumnIndex2A ) (
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE2    *ptablecreate  );

    static PFN_JetCreateTableColumnIndex2A pfnJetCreateTableColumnIndex2A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateTableColumnIndex2W ) (
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE2_W  *ptablecreate  );

    static PFN_JetCreateTableColumnIndex2W pfnJetCreateTableColumnIndex2W = NULL;
    bool    fWiden = FEsetestWidenParameters();

    bool    fChanged    = false;
    JET_TABLECREATE2*   ptablecreateCompatible  = NULL;

    JET_TABLECREATE2_W* wptablecreate   = NULL;
    JET_TABLECREATE2* ptablecreateT = NULL;

    bool fAllocated = false;
    JET_COLUMNCREATE* pcolumncreateInternal = NULL;
    if ( ptablecreate ){
        pcolumncreateInternal = ptablecreate->rgcolumncreate;
        ptablecreate->rgcolumncreate = EsetestCompressJET_COLUMNCREATE( pcolumncreateInternal, ptablecreate->cColumns, &fAllocated );
    }

    Call( EsetestAdaptJET_TABLECREATE2( __FUNCTION__, ptablecreate, &ptablecreateCompatible, &fChanged ) );
    if ( fChanged && NULL == ptablecreateCompatible )
    {
        tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "ptablecreate", "EsetestAdaptJET_TABLECREATE" );
        err = JET_errOutOfMemory;
        goto Cleanup;
    }

    ptablecreateT = fChanged ? ptablecreateCompatible : ptablecreate;
    
    if ( fWiden )
    {
        wptablecreate = EsetestWidenJET_TABLECREATE2( __FUNCTION__, ptablecreateT );
        if ( NULL == wptablecreate )
        {
            tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "ptablecreate", "EsetestWidenJET_TABLECREATE2" );
            err = JET_errOutOfMemory;
            goto Cleanup;
        }

    }


    if ( fWiden )
    {

        if ( NULL == pfnJetCreateTableColumnIndex2W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateTableColumnIndex2W = ( PFN_JetCreateTableColumnIndex2W ) ( GetProcAddress( hEseDll, "JetCreateTableColumnIndex2W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateTableColumnIndex2W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateTableColumnIndex2W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateTableColumnIndex2W)( sesid, dbid, wptablecreate );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetCreateTableColumnIndex2 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateTableColumnIndex2 = ( PFN_JetCreateTableColumnIndex2 ) ( GetProcAddress( hEseDll, "JetCreateTableColumnIndex2" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateTableColumnIndex2 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateTableColumnIndex2", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateTableColumnIndex2)( sesid, dbid, ptablecreateT );
        }
        else
        {

            if ( NULL == pfnJetCreateTableColumnIndex2A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateTableColumnIndex2A = ( PFN_JetCreateTableColumnIndex2A ) ( GetProcAddress( hEseDll, "JetCreateTableColumnIndex2A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateTableColumnIndex2A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateTableColumnIndex2A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateTableColumnIndex2A)( sesid, dbid, ptablecreateT );
        }
    }

    goto Cleanup;

Cleanup:
    if ( fWiden )
    {
        const JET_ERR errT = EsetestUnwidenJET_TABLECREATE2( __FUNCTION__, wptablecreate, ptablecreateT );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenJET_TABLECREATE2" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }

    if ( fChanged )
    {
        const JET_ERR errT = EsetestUnadaptJET_TABLECREATE2( __FUNCTION__, ptablecreate, &ptablecreateCompatible );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnadaptJET_TABLECREATE2" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }

    if ( fAllocated ){
        for ( unsigned long i = 0 ; i < ptablecreate->cColumns ; i++ ){
            pcolumncreateInternal[ i ].columnid = ptablecreate->rgcolumncreate[ i ].columnid;
            pcolumncreateInternal[ i ].err = ptablecreate->rgcolumncreate[ i ].err;
        }
        delete []( ptablecreate->rgcolumncreate );
        ptablecreate->rgcolumncreate = pcolumncreateInternal;
    }

    return err;
}



JET_ERR
BounceJetCreateTableColumnIndex3(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE3    *ptablecreate
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetCreateTableColumnIndex3A ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE3    *ptablecreate  );

    static PFN_JetCreateTableColumnIndex3A pfnJetCreateTableColumnIndex3A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateTableColumnIndex3W ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE3_W *ptablecreate  );

    static PFN_JetCreateTableColumnIndex3W pfnJetCreateTableColumnIndex3W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    JET_TABLECREATE3_W* wptablecreate   = NULL;

    bool fAllocated = false;
    JET_COLUMNCREATE* pcolumncreateInternal = NULL;
    if ( ptablecreate ){
        pcolumncreateInternal = ptablecreate->rgcolumncreate;
        ptablecreate->rgcolumncreate = EsetestCompressJET_COLUMNCREATE( pcolumncreateInternal, ptablecreate->cColumns, &fAllocated );
    }

    if ( fWiden )
    {
        if( NULL != ptablecreate ) 
        {
            wptablecreate = EsetestWidenJET_TABLECREATE3( __FUNCTION__, ptablecreate );
            if ( NULL == wptablecreate )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "ptablecreate", "EsetestWidenJET_TABLECREATE3" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetCreateTableColumnIndex3W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateTableColumnIndex3W = ( PFN_JetCreateTableColumnIndex3W ) ( GetProcAddress( hEseDll, "JetCreateTableColumnIndex3W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateTableColumnIndex3W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateTableColumnIndex3W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateTableColumnIndex3W)( sesid, dbid, wptablecreate );
    }
    else
    {
        if ( NULL == pfnJetCreateTableColumnIndex3A )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateTableColumnIndex3A = ( PFN_JetCreateTableColumnIndex3A ) ( GetProcAddress( hEseDll, "JetCreateTableColumnIndex3A" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateTableColumnIndex3A )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateTableColumnIndex3A", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateTableColumnIndex3A)( sesid, dbid, ptablecreate );
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestUnwidenJET_TABLECREATE3( __FUNCTION__, wptablecreate, ptablecreate );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenJET_TABLECREATE3" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }

    if ( fAllocated ){
        for ( unsigned long i = 0 ; i < ptablecreate->cColumns ; i++ ){
            pcolumncreateInternal[ i ].columnid = ptablecreate->rgcolumncreate[ i ].columnid;
            pcolumncreateInternal[ i ].err = ptablecreate->rgcolumncreate[ i ].err;
        }
        delete []( ptablecreate->rgcolumncreate );
        ptablecreate->rgcolumncreate = pcolumncreateInternal;
    }

    return err;
}



JET_ERR
BounceJetCreateTableColumnIndex5(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE5    *ptablecreate
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetCreateTableColumnIndex5A ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE5    *ptablecreate  );

    static PFN_JetCreateTableColumnIndex5A pfnJetCreateTableColumnIndex5A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateTableColumnIndex5W ) ( 
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE5_W  *ptablecreate  );

    static PFN_JetCreateTableColumnIndex5W pfnJetCreateTableColumnIndex5W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    JET_TABLECREATE5_W* wptablecreate   = NULL;

    bool fAllocated = false;
    JET_COLUMNCREATE* pcolumncreateInternal = NULL;
    if ( ptablecreate ){
        pcolumncreateInternal = ptablecreate->rgcolumncreate;
        ptablecreate->rgcolumncreate = EsetestCompressJET_COLUMNCREATE( pcolumncreateInternal, ptablecreate->cColumns, &fAllocated );
    }

    if ( fWiden )
    {
        if( NULL != ptablecreate ) 
        {
            wptablecreate = EsetestWidenJET_TABLECREATE5( __FUNCTION__, ptablecreate );
            if ( NULL == wptablecreate )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "ptablecreate", "EsetestWidenJET_TABLECREATE5" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetCreateTableColumnIndex5W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateTableColumnIndex5W = ( PFN_JetCreateTableColumnIndex5W ) ( GetProcAddress( hEseDll, "JetCreateTableColumnIndex5W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateTableColumnIndex5W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateTableColumnIndex5W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateTableColumnIndex5W)( sesid, dbid, wptablecreate );
    }
    else
    {
        if ( NULL == pfnJetCreateTableColumnIndex5A )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateTableColumnIndex5A = ( PFN_JetCreateTableColumnIndex5A ) ( GetProcAddress( hEseDll, "JetCreateTableColumnIndex5A" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateTableColumnIndex5A )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateTableColumnIndex5A", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateTableColumnIndex5A)( sesid, dbid, ptablecreate );
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestUnwidenJET_TABLECREATE5( __FUNCTION__, wptablecreate, ptablecreate );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenJET_TABLECREATE5" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }

    if ( fAllocated ){
        for ( unsigned long i = 0 ; i < ptablecreate->cColumns ; i++ ){
            pcolumncreateInternal[ i ].columnid = ptablecreate->rgcolumncreate[ i ].columnid;
            pcolumncreateInternal[ i ].err = ptablecreate->rgcolumncreate[ i ].err;
        }
        delete []( ptablecreate->rgcolumncreate );
        ptablecreate->rgcolumncreate = pcolumncreateInternal;
    }

    return err;
}



JET_ERR
BounceJetCreateIndex2(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in_ecount(cIndexCreate) JET_INDEXCREATE   *pindexcreate,
    unsigned long   cIndexCreate
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetCreateIndex2 ) (
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in_ecount(cIndexCreate) JET_INDEXCREATE   *pindexcreate,
    unsigned long   cIndexCreate  );

    static PFN_JetCreateIndex2 pfnJetCreateIndex2 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateIndex2A ) (
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in_ecount(cIndexCreate) JET_INDEXCREATE   *pindexcreate,
    unsigned long   cIndexCreate  );

    static PFN_JetCreateIndex2A pfnJetCreateIndex2A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateIndex2W ) (
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in_ecount(cIndexCreate) JET_INDEXCREATE_W *pindexcreate,
    unsigned long   cIndexCreate  );

    static PFN_JetCreateIndex2W pfnJetCreateIndex2W = NULL;
    bool    fWiden = FEsetestWidenParameters();

    JET_INDEXCREATE_W*  wpindexcreate   = NULL;
    bool    fChanged    = false;
    JET_INDEXCREATE*    pindexcreateCompatible  = NULL;
    JET_INDEXCREATE*    pindexcreateT       = NULL;

    Call( EsetestAdaptJET_INDEXCREATE( __FUNCTION__, pindexcreate, cIndexCreate, &pindexcreateCompatible, &fChanged ) );
    if ( fChanged && NULL == pindexcreateCompatible )
    {
        tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "pindexcreate", "EsetestAdaptJET_INDEXCREATE" );
        err = JET_errOutOfMemory;
        goto Cleanup;
    }

    pindexcreateT = fChanged ? pindexcreateCompatible : pindexcreate;

    if ( fWiden )
    {
        wpindexcreate = EsetestWidenJET_INDEXCREATE( __FUNCTION__, pindexcreateT, cIndexCreate );
        if ( NULL == wpindexcreate )
        {
            tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "pindexcreate", "EsetestWidenJET_INDEXCREATE" );
            err = JET_errOutOfMemory;
            goto Cleanup;
        }
    }

    if ( fWiden )
    {

        if ( NULL == pfnJetCreateIndex2W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateIndex2W = ( PFN_JetCreateIndex2W ) ( GetProcAddress( hEseDll, "JetCreateIndex2W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateIndex2W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateIndex2W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateIndex2W)( sesid, tableid, wpindexcreate, cIndexCreate );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetCreateIndex2 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateIndex2 = ( PFN_JetCreateIndex2 ) ( GetProcAddress( hEseDll, "JetCreateIndex2" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateIndex2 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateIndex2", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateIndex2)( sesid, tableid, pindexcreateT, cIndexCreate );
        }
        else
        {

            if ( NULL == pfnJetCreateIndex2A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateIndex2A = ( PFN_JetCreateIndex2A ) ( GetProcAddress( hEseDll, "JetCreateIndex2A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateIndex2A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateIndex2A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateIndex2A)( sesid, tableid, pindexcreateT, cIndexCreate );
        }
    }
    goto Cleanup;

Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestUnwidenJET_INDEXCREATE( __FUNCTION__, wpindexcreate, cIndexCreate, pindexcreateT );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenJET_INDEXCREATE" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }

    if ( fChanged )
    {
        const JET_ERR errT = EsetestUnadaptJET_INDEXCREATE( __FUNCTION__, pindexcreate, cIndexCreate, &pindexcreateCompatible );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnadaptJET_INDEXCREATE" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }

    return err;
}



JET_ERR
BounceJetExternalRestore(
    __in JET_PSTR   szCheckpointFilePath,
    __in JET_PSTR   szLogPath,
    JET_RSTMAP *    rgrstmap,
    long        crstfilemap,
    __in JET_PSTR   szBackupLogPath,
    long        genLow,
    long        genHigh,
    JET_PFNSTATUS   pfn
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetExternalRestore ) (
    __in JET_PSTR   szCheckpointFilePath,
    __in JET_PSTR   szLogPath,
    JET_RSTMAP *    rgrstmap,
    long        crstfilemap,
    __in JET_PSTR   szBackupLogPath,
    long        genLow,
    long        genHigh,
    JET_PFNSTATUS   pfn  );

    static PFN_JetExternalRestore pfnJetExternalRestore = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetExternalRestoreA ) (
    __in JET_PSTR   szCheckpointFilePath,
    __in JET_PSTR   szLogPath,
    JET_RSTMAP *    rgrstmap,
    long        crstfilemap,
    __in JET_PSTR   szBackupLogPath,
    long        genLow,
    long        genHigh,
    JET_PFNSTATUS   pfn  );

    static PFN_JetExternalRestoreA pfnJetExternalRestoreA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetExternalRestoreW ) (
    __in JET_PWSTR  wszCheckpointFilePath,
    __in JET_PWSTR  wszLogPath,
    JET_RSTMAP_W *  rgrstmap,
    long        crstfilemap,
    __in JET_PWSTR  wszBackupLogPath,
    long        genLow,
    long        genHigh,
    JET_PFNSTATUS   pfn  );

    static PFN_JetExternalRestoreW pfnJetExternalRestoreW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszBackupLogPath    = NULL;
    wchar_t*    wszLogPath  = NULL;
    wchar_t*    wszCheckpointFilePath   = NULL;
    JET_RSTMAP_W*   wrgrstmap   = NULL;

    if ( fWiden )
    {
        wszBackupLogPath = EsetestWidenString( __FUNCTION__, szBackupLogPath );
        if ( NULL == wszBackupLogPath )
        {
            tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szBackupLogPath", "EsetestWidenString" );
            err = JET_errOutOfMemory;
            goto Cleanup;
        }

        wszLogPath = EsetestWidenString( __FUNCTION__, szLogPath );
        if ( NULL == wszLogPath )
        {
            tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szLogPath", "EsetestWidenString" );
            err = JET_errOutOfMemory;
            goto Cleanup;
        }

        wszCheckpointFilePath = EsetestWidenString( __FUNCTION__, szCheckpointFilePath );
        if ( NULL == wszCheckpointFilePath )
        {
            tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szCheckpointFilePath", "EsetestWidenString" );
            err = JET_errOutOfMemory;
            goto Cleanup;
        }

        if(rgrstmap != NULL)
        {
            wrgrstmap = EsetestWidenJET_RSTMAP( __FUNCTION__, rgrstmap, crstfilemap  );
            if ( NULL == wrgrstmap )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "rgrstmap", "EsetestWidenJET_RSTMAP" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetExternalRestoreW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetExternalRestoreW = ( PFN_JetExternalRestoreW ) ( GetProcAddress( hEseDll, "JetExternalRestoreW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetExternalRestoreW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetExternalRestoreW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetExternalRestoreW)( wszCheckpointFilePath, wszLogPath, wrgrstmap, crstfilemap, wszBackupLogPath, genLow, genHigh, pfn );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetExternalRestore )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetExternalRestore = ( PFN_JetExternalRestore ) ( GetProcAddress( hEseDll, "JetExternalRestore" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetExternalRestore )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetExternalRestore", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetExternalRestore)( szCheckpointFilePath, szLogPath, rgrstmap, crstfilemap, szBackupLogPath, genLow, genHigh, pfn );
        }
        else
        {

            if ( NULL == pfnJetExternalRestoreA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetExternalRestoreA = ( PFN_JetExternalRestoreA ) ( GetProcAddress( hEseDll, "JetExternalRestoreA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetExternalRestoreA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetExternalRestoreA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetExternalRestoreA)( szCheckpointFilePath, szLogPath, rgrstmap, crstfilemap, szBackupLogPath, genLow, genHigh, pfn );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszBackupLogPath, szBackupLogPath );
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
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszLogPath, szLogPath );
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
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszCheckpointFilePath, szCheckpointFilePath );
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
        if(wrgrstmap != NULL)
        {
            const JET_ERR errT = EsetestUnwidenJET_RSTMAP( __FUNCTION__, wrgrstmap, crstfilemap, rgrstmap );
            if ( JET_errSuccess != errT )
            {
                tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenJET_RSTMAP" );
                if ( JET_errSuccess == err )
                {
                    err = errT;
                }
            }
        }
    }


    return err;
}



JET_ERR
BounceJetExternalRestore2(
    __in JET_PSTR   szCheckpointFilePath,
    __in JET_PSTR   szLogPath,
    JET_RSTMAP *    rgrstmap,
    long        crstfilemap,
    __in JET_PSTR   szBackupLogPath,
    JET_LOGINFO *   pLogInfo,
    __in JET_PSTR   szTargetInstanceName,
    __in JET_PSTR   szTargetInstanceLogPath,
    __in JET_PSTR   szTargetInstanceCheckpointPath,
    JET_PFNSTATUS pfn
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetExternalRestore2 ) (
    __in JET_PSTR   szCheckpointFilePath,
    __in JET_PSTR   szLogPath,
    JET_RSTMAP *    rgrstmap,
    long        crstfilemap,
    __in JET_PSTR   szBackupLogPath,
    JET_LOGINFO *   pLogInfo,
    __in JET_PSTR   szTargetInstanceName,
    __in JET_PSTR   szTargetInstanceLogPath,
    __in JET_PSTR   szTargetInstanceCheckpointPath,
    JET_PFNSTATUS pfn  );

    static PFN_JetExternalRestore2 pfnJetExternalRestore2 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetExternalRestore2A ) (
    __in JET_PSTR   szCheckpointFilePath,
    __in JET_PSTR   szLogPath,
    JET_RSTMAP *    rgrstmap,
    long        crstfilemap,
    __in JET_PSTR   szBackupLogPath,
    JET_LOGINFO *   pLogInfo,
    __in JET_PSTR   szTargetInstanceName,
    __in JET_PSTR   szTargetInstanceLogPath,
    __in JET_PSTR   szTargetInstanceCheckpointPath,
    JET_PFNSTATUS pfn  );

    static PFN_JetExternalRestore2A pfnJetExternalRestore2A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetExternalRestore2W ) (
    __in JET_PWSTR  wszCheckpointFilePath,
    __in JET_PWSTR  wszLogPath,
    JET_RSTMAP_W *  rgrstmap,
    long        crstfilemap,
    __in JET_PWSTR  wszBackupLogPath,
    JET_LOGINFO_W * pLogInfo,
    __in JET_PWSTR  wszTargetInstanceName,
    __in JET_PWSTR  wszTargetInstanceLogPath,
    __in JET_PWSTR  wszTargetInstanceCheckpointPath,
    JET_PFNSTATUS pfn  );

    static PFN_JetExternalRestore2W pfnJetExternalRestore2W = NULL;
    
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszTargetInstanceLogPath    = NULL;
    wchar_t*    wszTargetInstanceName   = NULL;
    wchar_t*    wszBackupLogPath    = NULL;
    wchar_t*    wszLogPath  = NULL;
    wchar_t*    wszCheckpointFilePath   = NULL;
    wchar_t*    wszTargetInstanceCheckpointPath = NULL;
    JET_RSTMAP_W*   wrgrstmap   = NULL;
    JET_LOGINFO_W* wpLogInfo = NULL;

    if ( fWiden )
    {
        if ( szTargetInstanceLogPath != NULL )
        {
            wszTargetInstanceLogPath = EsetestWidenString( __FUNCTION__, szTargetInstanceLogPath );
            if ( NULL == wszTargetInstanceLogPath )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szTargetInstanceLogPath", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }
        
        if ( szTargetInstanceName != NULL )
        {
            wszTargetInstanceName = EsetestWidenString( __FUNCTION__, szTargetInstanceName );
            if ( NULL == wszTargetInstanceName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szTargetInstanceName", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }
        wszBackupLogPath = EsetestWidenString( __FUNCTION__, szBackupLogPath );
        if ( NULL == wszBackupLogPath )
        {
            tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szBackupLogPath", "EsetestWidenString" );
            err = JET_errOutOfMemory;
            goto Cleanup;
        }

        wszLogPath = EsetestWidenString( __FUNCTION__, szLogPath );
        if ( NULL == wszLogPath )
        {
            tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szLogPath", "EsetestWidenString" );
            err = JET_errOutOfMemory;
            goto Cleanup;
        }

        if ( szCheckpointFilePath != NULL )
        {
            wszCheckpointFilePath = EsetestWidenString( __FUNCTION__, szCheckpointFilePath );
            if ( NULL == wszCheckpointFilePath )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szCheckpointFilePath", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if ( szTargetInstanceCheckpointPath != NULL )
        {
            wszTargetInstanceCheckpointPath = EsetestWidenString( __FUNCTION__, szTargetInstanceCheckpointPath );
            if ( NULL == wszTargetInstanceCheckpointPath )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szTargetInstanceCheckpointPath", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if(rgrstmap != NULL)
        {
            wrgrstmap = EsetestWidenJET_RSTMAP( __FUNCTION__, rgrstmap, crstfilemap );
            if ( NULL == wrgrstmap )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "rgrstmap", "EsetestWidenJET_RSTMAP" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }
        
        wpLogInfo = EsetestWidenJET_LOGINFO( __FUNCTION__, pLogInfo );
        if ( NULL == wpLogInfo)
        {
            tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "pLogInfo", "EsetestWidenJET_LOGINFO" );
            err = JET_errOutOfMemory;
            goto Cleanup;
        }


    }

    if ( fWiden )
    {

        if ( NULL == pfnJetExternalRestore2W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetExternalRestore2W = ( PFN_JetExternalRestore2W ) ( GetProcAddress( hEseDll, "JetExternalRestore2W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetExternalRestore2W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetExternalRestore2W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetExternalRestore2W)( wszCheckpointFilePath, wszLogPath, wrgrstmap, crstfilemap, wszBackupLogPath,
                wpLogInfo, wszTargetInstanceName, wszTargetInstanceLogPath, wszTargetInstanceCheckpointPath, pfn );
        
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetExternalRestore2 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetExternalRestore2 = ( PFN_JetExternalRestore2 ) ( GetProcAddress( hEseDll, "JetExternalRestore2" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetExternalRestore2 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetExternalRestore2", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetExternalRestore2)( szCheckpointFilePath, szLogPath, rgrstmap, crstfilemap, szBackupLogPath, pLogInfo, szTargetInstanceName, szTargetInstanceLogPath, szTargetInstanceCheckpointPath, pfn );
        }
        else
        {

            if ( NULL == pfnJetExternalRestore2A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetExternalRestore2A = ( PFN_JetExternalRestore2A ) ( GetProcAddress( hEseDll, "JetExternalRestore2A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetExternalRestore2A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetExternalRestore2A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetExternalRestore2A)( szCheckpointFilePath, szLogPath, rgrstmap, crstfilemap, szBackupLogPath, pLogInfo, szTargetInstanceName, szTargetInstanceLogPath, szTargetInstanceCheckpointPath, pfn );          
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszTargetInstanceLogPath, szTargetInstanceLogPath );
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
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszTargetInstanceName, szTargetInstanceName );
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
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszBackupLogPath, szBackupLogPath );
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
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszLogPath, szLogPath );
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
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszCheckpointFilePath, szCheckpointFilePath );
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
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszTargetInstanceCheckpointPath, szTargetInstanceCheckpointPath );
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
        if(wrgrstmap != NULL)
        {
            const JET_ERR errT = EsetestUnwidenJET_RSTMAP( __FUNCTION__, wrgrstmap, crstfilemap, rgrstmap );
            if ( JET_errSuccess != errT )
            {
                tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenJET_RSTMAP" );
                if ( JET_errSuccess == err )
                {
                    err = errT;
                }
            }
        }
    }

    if ( fWiden )
    {
        const JET_ERR errT = EsetestUnwidenJET_LOGINFO( __FUNCTION__, wpLogInfo, pLogInfo );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenJET_LOGINFO" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}


JET_ERR
BounceJetGetSystemParameter(
    JET_INSTANCE    instance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __out_bcount_opt(cbMax) JET_API_PTR     *plParam,
    __out_bcount_opt(cbMax) JET_PSTR        szParam,
    unsigned long   cbMax
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetGetSystemParameter ) ( 
    JET_INSTANCE    instance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __out_bcount_opt(cbMax) JET_API_PTR     *plParam,
    __out_bcount_opt(cbMax) JET_PSTR        szParam,
    unsigned long   cbMax  );

    static PFN_JetGetSystemParameter pfnJetGetSystemParameter = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetGetSystemParameterA ) ( 
    JET_INSTANCE    instance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __out_bcount_opt(cbMax) JET_API_PTR     *plParam,
    __out_bcount_opt(cbMax) JET_PSTR        szParam,
    unsigned long   cbMax  );

    static PFN_JetGetSystemParameterA pfnJetGetSystemParameterA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetGetSystemParameterW ) ( 
    JET_INSTANCE    instance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __out_bcount_opt(cbMax) JET_API_PTR     *plParam,
    __out_bcount_opt(cbMax) JET_PWSTR       wszParam,
    unsigned long   cbMax  );

    static PFN_JetGetSystemParameterW pfnJetGetSystemParameterW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszParam    = NULL;

    if ( fWiden )
    {
        if ( cbMax != 0 && szParam != NULL )
        {
            wszParam = EsetestWidenStringWithLength( __FUNCTION__, szParam, cbMax );
            if ( NULL == wszParam )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szParam", "EsetestWidenStringWithLength" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetGetSystemParameterW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetGetSystemParameterW = ( PFN_JetGetSystemParameterW ) ( GetProcAddress( hEseDll, "JetGetSystemParameterW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetGetSystemParameterW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetGetSystemParameterW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetGetSystemParameterW)( instance, sesid, paramid, plParam, wszParam, 2 * cbMax );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetGetSystemParameter )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetGetSystemParameter = ( PFN_JetGetSystemParameter ) ( GetProcAddress( hEseDll, "JetGetSystemParameter" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetGetSystemParameter )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetGetSystemParameter", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetGetSystemParameter)( instance, sesid, paramid, plParam, szParam, cbMax );
        }
        else
        {

            if ( NULL == pfnJetGetSystemParameterA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetGetSystemParameterA = ( PFN_JetGetSystemParameterA ) ( GetProcAddress( hEseDll, "JetGetSystemParameterA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetGetSystemParameterA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetGetSystemParameterA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetGetSystemParameterA)( instance, sesid, paramid, plParam, szParam, cbMax );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        if ( cbMax != 0 && wszParam != NULL )
        {
            const JET_ERR errT = EsetestUnwidenStringWithLength( __FUNCTION__, wszParam, cbMax, szParam );
            if ( JET_errSuccess != errT )
            {
                tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenStringWithLength" );
                if ( JET_errSuccess == err )
                {
                    err = errT;
                }
            }
        }
    }


    return err;
}



JET_ERR
BounceJetSetColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_COLUMNID    columnid,
    const void      *pvData,
    unsigned long   cbData,
    JET_GRBIT       grbit,
    JET_SETINFO     *psetinfo
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetSetColumn ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_COLUMNID    columnid,
    const void      *pvData,
    unsigned long   cbData,
    JET_GRBIT       grbit,
    JET_SETINFO     *psetinfo  );

    static PFN_JetSetColumn pfnJetSetColumn = NULL;

    grbit |= GrbitDataCompression( grbit );

    if ( NULL == pfnJetSetColumn )
    {
        const HMODULE       hEseDll = HmodEsetestEseDll();

        if ( NULL != hEseDll )
        {
            pfnJetSetColumn = ( PFN_JetSetColumn ) ( GetProcAddress( hEseDll, "JetSetColumn" ) );
        }
        if ( NULL == hEseDll || NULL == pfnJetSetColumn )
        {
            tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                __FUNCTION__, hEseDll, "JetSetColumn", GetLastError() );
            err = JET_errTestError;
            goto Cleanup;
        }
    }

    err = (*pfnJetSetColumn)( sesid, tableid, columnid, pvData, cbData, grbit, psetinfo );
    goto Cleanup;
Cleanup:

    return err;
}


JET_ERR
BounceJetSetColumns(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_SETCOLUMN   *psetcolumn,
    unsigned long   csetcolumn
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetSetColumns ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_SETCOLUMN   *psetcolumn,
    unsigned long   csetcolumn  );

    static PFN_JetSetColumns pfnJetSetColumns = NULL;

    bool fAllocated = false;
    JET_SETCOLUMN* psetcolumnInternal = EsetestCompressJET_SETCOLUMN( psetcolumn, csetcolumn, &fAllocated );

    if ( NULL == pfnJetSetColumns )
    {
        const HMODULE       hEseDll = HmodEsetestEseDll();

        if ( NULL != hEseDll )
        {
            pfnJetSetColumns = ( PFN_JetSetColumns ) ( GetProcAddress( hEseDll, "JetSetColumns" ) );
        }
        if ( NULL == hEseDll || NULL == pfnJetSetColumns )
        {
            tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                __FUNCTION__, hEseDll, "JetSetColumns", GetLastError() );
            err = JET_errTestError;
            goto Cleanup;
        }
    }

    err = (*pfnJetSetColumns)( sesid, tableid, psetcolumnInternal, csetcolumn );
    goto Cleanup;
Cleanup:

    if ( fAllocated ){
        for ( unsigned long i = 0 ; i < csetcolumn ; i++ ){
            psetcolumn[ i ].err = psetcolumnInternal[ i ].err;
        }
        delete []( psetcolumnInternal );
    }

    return err;
}


JET_ERR
BounceJetGetCurrentIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount(cbIndexName) JET_PSTR szIndexName,
    unsigned long   cbIndexName
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetGetCurrentIndex ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount(cbIndexName) JET_PSTR szIndexName,
    unsigned long   cbIndexName  );

    static PFN_JetGetCurrentIndex pfnJetGetCurrentIndex = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetGetCurrentIndexA ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount(cbIndexName) JET_PSTR szIndexName,
    unsigned long   cbIndexName  );

    static PFN_JetGetCurrentIndexA pfnJetGetCurrentIndexA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetGetCurrentIndexW ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount(cbIndexName) JET_PWSTR wszIndexName,
    unsigned long   cbIndexName  );

    static PFN_JetGetCurrentIndexW pfnJetGetCurrentIndexW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszIndexName    = NULL;

    if ( fWiden )
    {
        if ( cbIndexName != 0 )
        {
            wszIndexName = EsetestWidenStringWithLength( __FUNCTION__, szIndexName, cbIndexName );
            if ( NULL == wszIndexName )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szIndexName", "EsetestWidenStringWithLength" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetGetCurrentIndexW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetGetCurrentIndexW = ( PFN_JetGetCurrentIndexW ) ( GetProcAddress( hEseDll, "JetGetCurrentIndexW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetGetCurrentIndexW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetGetCurrentIndexW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetGetCurrentIndexW)( sesid, tableid, wszIndexName, 2 * cbIndexName );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetGetCurrentIndex )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetGetCurrentIndex = ( PFN_JetGetCurrentIndex ) ( GetProcAddress( hEseDll, "JetGetCurrentIndex" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetGetCurrentIndex )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetGetCurrentIndex", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetGetCurrentIndex)( sesid, tableid, szIndexName, cbIndexName );
        }
        else
        {

            if ( NULL == pfnJetGetCurrentIndexA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetGetCurrentIndexA = ( PFN_JetGetCurrentIndexA ) ( GetProcAddress( hEseDll, "JetGetCurrentIndexA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetGetCurrentIndexA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetGetCurrentIndexA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetGetCurrentIndexA)( sesid, tableid, szIndexName, cbIndexName );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestUnwidenStringWithLength( __FUNCTION__, wszIndexName, cbIndexName, szIndexName );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenStringWithLength" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}


JET_ERR
BounceJetAddColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName,
    const JET_COLUMNDEF *pcolumndef,
    __in_bcount_opt(cbDefault) const void*      pvDefault,
    unsigned long   cbDefault,
    __out_opt JET_COLUMNID  *pcolumnid
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetAddColumn ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName,
    const JET_COLUMNDEF *pcolumndef,
    __in_bcount_opt(cbDefault) const void*      pvDefault,
    unsigned long   cbDefault,
    __out_opt JET_COLUMNID  *pcolumnid  );

    static PFN_JetAddColumn pfnJetAddColumn = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetAddColumnA ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName,
    const JET_COLUMNDEF *pcolumndef,
    __in_bcount_opt(cbDefault) const void*      pvDefault,
    unsigned long   cbDefault,
    __out_opt JET_COLUMNID  *pcolumnid  );

    static PFN_JetAddColumnA pfnJetAddColumnA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetAddColumnW ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const WCHAR     *wszColumnName,
    const JET_COLUMNDEF *pcolumndef,
    __in_bcount_opt(cbDefault) const void*      pvDefault,
    unsigned long   cbDefault,
    __out_opt JET_COLUMNID  *pcolumnid  );

    static PFN_JetAddColumnW pfnJetAddColumnW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszColumnName   = NULL;

    bool fAllocated = false;
    JET_COLUMNDEF* pcolumndefInternal = EsetestCompressJET_COLUMNDEF( pcolumndef, &fAllocated );

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

        if ( NULL == pfnJetAddColumnW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetAddColumnW = ( PFN_JetAddColumnW ) ( GetProcAddress( hEseDll, "JetAddColumnW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetAddColumnW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetAddColumnW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetAddColumnW)( sesid, tableid, wszColumnName, pcolumndefInternal, pvDefault, cbDefault, pcolumnid );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetAddColumn )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetAddColumn = ( PFN_JetAddColumn ) ( GetProcAddress( hEseDll, "JetAddColumn" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetAddColumn )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetAddColumn", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetAddColumn)( sesid, tableid, szColumnName, pcolumndefInternal, pvDefault, cbDefault, pcolumnid );
        }
        else
        {

            if ( NULL == pfnJetAddColumnA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetAddColumnA = ( PFN_JetAddColumnA ) ( GetProcAddress( hEseDll, "JetAddColumnA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetAddColumnA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetAddColumnA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetAddColumnA)( sesid, tableid, szColumnName, pcolumndefInternal, pvDefault, cbDefault, pcolumnid );
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

    if ( fAllocated ){
        ( ( JET_COLUMNDEF* )pcolumndef )->columnid = pcolumndefInternal->columnid;
        delete pcolumndefInternal;
    }

    return err;
}


JET_ERR
BounceJetCreateIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szIndexName,
    JET_GRBIT       grbit,
    const char      *szKey,
    unsigned long   cbKey,
    unsigned long   lDensity
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetCreateIndex ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szIndexName,
    JET_GRBIT       grbit,
    const char      *szKey,
    unsigned long   cbKey,
    unsigned long   lDensity  );

    static PFN_JetCreateIndex pfnJetCreateIndex = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateIndexA ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szIndexName,
    JET_GRBIT       grbit,
    const char      *szKey,
    unsigned long   cbKey,
    unsigned long   lDensity  );

    static PFN_JetCreateIndexA pfnJetCreateIndexA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetCreateIndexW ) ( 
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCWSTR  wszIndexName,
    JET_GRBIT       grbit,
    const WCHAR     *wszKey,
    unsigned long   cbKey,
    unsigned long   lDensity  );

    static PFN_JetCreateIndexW pfnJetCreateIndexW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszKey  = NULL;
    wchar_t*    wszIndexName    = NULL;

    if ( fWiden )
    {
        if( NULL != szKey ) 
        {
            wszKey = EsetestWidenCbString( __FUNCTION__, szKey, cbKey );
            if ( NULL == wszKey )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szKey", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

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

        if ( NULL == pfnJetCreateIndexW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetCreateIndexW = ( PFN_JetCreateIndexW ) ( GetProcAddress( hEseDll, "JetCreateIndexW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetCreateIndexW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetCreateIndexW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetCreateIndexW)( sesid, tableid, wszIndexName, grbit, wszKey, cbKey * sizeof( WCHAR ) / sizeof( CHAR ) , lDensity );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetCreateIndex )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateIndex = ( PFN_JetCreateIndex ) ( GetProcAddress( hEseDll, "JetCreateIndex" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateIndex )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateIndex", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateIndex)( sesid, tableid, szIndexName, grbit, szKey, cbKey, lDensity );
        }
        else
        {

            if ( NULL == pfnJetCreateIndexA )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetCreateIndexA = ( PFN_JetCreateIndexA ) ( GetProcAddress( hEseDll, "JetCreateIndexA" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetCreateIndexA )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetCreateIndexA", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetCreateIndexA)( sesid, tableid, szIndexName, grbit, szKey, cbKey, lDensity );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszKey, szKey );
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
BounceJetInit3(
    JET_INSTANCE *pinstance,
    JET_RSTINFO *prstInfo,
    JET_GRBIT grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetInit3 ) ( 
    JET_INSTANCE *pinstance,
    JET_RSTINFO *prstInfo,
    JET_GRBIT grbit  );

    static PFN_JetInit3 pfnJetInit3 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetInit3A ) ( 
    JET_INSTANCE *pinstance,
    JET_RSTINFO *prstInfo,
    JET_GRBIT grbit  );

    static PFN_JetInit3A pfnJetInit3A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetInit3W ) ( 
    JET_INSTANCE *pinstance,
    JET_RSTINFO_W *wprstInfo,
    JET_GRBIT grbit  );

    static PFN_JetInit3W pfnJetInit3W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    JET_RSTINFO_W*  wprstInfo   = NULL;

    if ( fWiden )
    {
        if( NULL != prstInfo ) 
        {
            wprstInfo = EsetestWidenJET_RSTINFO( __FUNCTION__, prstInfo );
            if ( NULL == wprstInfo )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "prstInfo", "EsetestWidenJET_RSTINFO" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetInit3W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetInit3W = ( PFN_JetInit3W ) ( GetProcAddress( hEseDll, "JetInit3W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetInit3W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetInit3W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetInit3W)( pinstance, wprstInfo, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetInit3 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetInit3 = ( PFN_JetInit3 ) ( GetProcAddress( hEseDll, "JetInit3" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetInit3 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetInit3", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetInit3)( pinstance, prstInfo, grbit );
        }
        else
        {

            if ( NULL == pfnJetInit3A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetInit3A = ( PFN_JetInit3A ) ( GetProcAddress( hEseDll, "JetInit3A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetInit3A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetInit3A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetInit3A)( pinstance, prstInfo, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestUnwidenJET_RSTINFO( __FUNCTION__, wprstInfo, prstInfo );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenJET_RSTINFO" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}


JET_ERR
BounceJetInit4(
    JET_INSTANCE *pinstance,
    JET_RSTINFO2 *prstInfo,
    JET_GRBIT grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetInit4 ) ( 
    JET_INSTANCE *pinstance,
    JET_RSTINFO2 *prstInfo,
    JET_GRBIT grbit  );

    static PFN_JetInit4 pfnJetInit4 = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetInit4A ) ( 
    JET_INSTANCE *pinstance,
    JET_RSTINFO2 *prstInfo,
    JET_GRBIT grbit  );

    static PFN_JetInit4A pfnJetInit4A = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetInit4W ) ( 
    JET_INSTANCE *pinstance,
    JET_RSTINFO2_W *wprstInfo,
    JET_GRBIT grbit  );

    static PFN_JetInit4W pfnJetInit4W = NULL;
    bool    fWiden = FEsetestWidenParameters();


    JET_RSTINFO2_W* wprstInfo   = NULL;

    if ( fWiden )
    {
        if( NULL != prstInfo ) 
        {
            wprstInfo = EsetestWidenJET_RSTINFO2( __FUNCTION__, prstInfo );
            if ( NULL == wprstInfo )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "prstInfo", "EsetestWidenJET_RSTINFO2" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetInit4W )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetInit4W = ( PFN_JetInit4W ) ( GetProcAddress( hEseDll, "JetInit4W" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetInit4W )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetInit4W", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetInit4W)( pinstance, wprstInfo, grbit );
    }
    else
    {
        if ( ( !FEsetestFeaturePresent( EseFeatureApisExportedWithA ) )  || 0 == ( rand() % 3 ) )
        {

            if ( NULL == pfnJetInit4 )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetInit4 = ( PFN_JetInit4 ) ( GetProcAddress( hEseDll, "JetInit4" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetInit4 )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetInit4", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetInit4)( pinstance, prstInfo, grbit );
        }
        else
        {

            if ( NULL == pfnJetInit4A )
            {
                const HMODULE       hEseDll = HmodEsetestEseDll();

                if ( NULL != hEseDll )
                {
                    pfnJetInit4A = ( PFN_JetInit4A ) ( GetProcAddress( hEseDll, "JetInit4A" ) );
                }
                if ( NULL == hEseDll || NULL == pfnJetInit4A )
                {
                    tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                        __FUNCTION__, hEseDll, "JetInit4A", GetLastError() );
                    err = JET_errTestError;
                    goto Cleanup;
                }
            }

            err = (*pfnJetInit4A)( pinstance, prstInfo, grbit );
        }
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestUnwidenJET_RSTINFO2( __FUNCTION__, wprstInfo, prstInfo );
        if ( JET_errSuccess != errT )
        {
            tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnwidenJET_RSTINFO2" );
            if ( JET_errSuccess == err )
            {
                err = errT;
            }
        }
    }


    return err;
}


JET_ERR
BounceJetRemoveLogfile(
    __in JET_PCSTR szDatabase,
    __in JET_PCSTR szLogfile,
    __in JET_GRBIT grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetRemoveLogfileA ) ( 
    __in JET_PCSTR szDatabase,
    __in JET_PCSTR szLogfile,
    __in JET_GRBIT grbit  );

    static PFN_JetRemoveLogfileA pfnJetRemoveLogfileA = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetRemoveLogfileW ) ( 
    __in JET_PCWSTR wszDatabase,
    __in JET_PCWSTR wszLogfile,
    __in JET_GRBIT grbit  );

    static PFN_JetRemoveLogfileW pfnJetRemoveLogfileW = NULL;
    bool    fWiden = FEsetestWidenParameters();


    wchar_t*    wszDatabase = NULL;
    wchar_t*    wszLogfile  = NULL;

    if ( fWiden )
    {
        if( NULL != szDatabase ) 
        {
            wszDatabase = EsetestWidenString( __FUNCTION__, szDatabase );
            if ( NULL == wszDatabase )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szDatabase", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

        if( NULL != szLogfile ) 
        {
            wszLogfile = EsetestWidenString( __FUNCTION__, szLogfile );
            if ( NULL == wszLogfile )
            {
                tprintf( "%s(): Failed to widen %s calling %s" CRLF, __FUNCTION__, "szLogfile", "EsetestWidenString" );
                err = JET_errOutOfMemory;
                goto Cleanup;
            }
        }

    }

    if ( fWiden )
    {

        if ( NULL == pfnJetRemoveLogfileW )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetRemoveLogfileW = ( PFN_JetRemoveLogfileW ) ( GetProcAddress( hEseDll, "JetRemoveLogfileW" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetRemoveLogfileW )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetRemoveLogfileW", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetRemoveLogfileW)( wszDatabase, wszLogfile, grbit );
    }
    else
    {
        if ( NULL == pfnJetRemoveLogfileA )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetRemoveLogfileA = ( PFN_JetRemoveLogfileA ) ( GetProcAddress( hEseDll, "JetRemoveLogfileA" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetRemoveLogfileA )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetRemoveLogfileA", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetRemoveLogfileA)( szDatabase, szLogfile, grbit );
    }
    goto Cleanup;
Cleanup:

    if ( fWiden )
    {
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszDatabase, szDatabase );
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
        const JET_ERR errT = EsetestCleanupWidenString( __FUNCTION__, wszLogfile, szLogfile );
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
BounceJetGetDatabasePages(
    __in JET_SESID                              sesid,
    __in JET_DBID                               dbid,
    __in unsigned long                          pgnoStart,
    __in unsigned long                          cpg,
    __out_bcount_part( cb,
    *pcbActual ) void * pv,
    __in unsigned long                          cb,
    __out unsigned long *                       pcbActual,
    __in JET_GRBIT                              grbit
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetGetDatabasePages ) ( 
    __in JET_SESID                              sesid,
    __in JET_DBID                               dbid,
    __in unsigned long                          pgno,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    __in unsigned long                          cb,
    __out unsigned long *                       pcb,
    __in JET_GRBIT                              grbit  );

    static PFN_JetGetDatabasePages pfnJetGetDatabasePages = NULL;

    typedef JET_ERR ( __stdcall *PFN_JetGetDatabasePages_2 ) ( 
    __in JET_SESID                              sesid,
    __in JET_DBID                               dbid,
    __in unsigned long                          pgnoStart,
    __in unsigned long                          cpg,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    __in unsigned long                          cb,
    __out unsigned long *                       pcbActual,
    __in JET_GRBIT                              grbit  );

    static PFN_JetGetDatabasePages_2 pfnJetGetDatabasePages_2 = NULL;

    bool    fIncReseed = FEsetestFeaturePresent( EseFeatureIncrementalReseedApis );

    if ( !fIncReseed )
    {
        if ( NULL == pfnJetGetDatabasePages )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetGetDatabasePages = ( PFN_JetGetDatabasePages ) ( GetProcAddress( hEseDll, "JetGetDatabasePages" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetGetDatabasePages )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetGetDatabasePages", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetGetDatabasePages)( sesid, dbid, pgnoStart, pv, cb, pcbActual, grbit );
    }
    else
    {
        if ( NULL == pfnJetGetDatabasePages_2 )
        {
            const HMODULE       hEseDll = HmodEsetestEseDll();

            if ( NULL != hEseDll )
            {
                pfnJetGetDatabasePages_2 = ( PFN_JetGetDatabasePages_2 ) ( GetProcAddress( hEseDll, "JetGetDatabasePages" ) );
            }
            if ( NULL == hEseDll || NULL == pfnJetGetDatabasePages_2 )
            {
                tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                    __FUNCTION__, hEseDll, "JetGetDatabasePages", GetLastError() );
                err = JET_errTestError;
                goto Cleanup;
            }
        }

        err = (*pfnJetGetDatabasePages_2)( sesid, dbid, pgnoStart, cpg, pv, cb, pcbActual, grbit );
    }
    goto Cleanup;
Cleanup:

    return err;
}

