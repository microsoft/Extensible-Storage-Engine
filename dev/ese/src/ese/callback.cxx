// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"

//  store an array of all the librarys we have open, close them at termination
//  we assume there will be a very small number of callback DLLs used so a relatively
//  poor memory allocation scheme is used

struct LIBRARYMAP
{
    WCHAR *     wszLibrary;
    LIBRARY library;
};

LOCAL INT           g_clibrary        = 0;
LOCAL LIBRARYMAP *  g_rglibrarymap    = NULL;

LOCAL CCriticalSection g_critCallback( CLockBasicInfo( CSyncBasicInfo( szCritCallbacks ), rankCallbacks, 0 ) );


//  ================================================================
ERR ErrCALLBACKInit()
//  ================================================================
{
    g_clibrary = 0;
    g_rglibrarymap = NULL;
    return JET_errSuccess;
}


//  ================================================================
VOID CALLBACKTerm()
//  ================================================================
//
//  Close all the module handles
//
//-
{
    INT ilibrary;
    for( ilibrary = 0; ilibrary < g_clibrary; ++ilibrary )
    {
        UtilFreeLibrary( g_rglibrarymap[ilibrary].library );
        OSMemoryHeapFree( g_rglibrarymap[ilibrary].wszLibrary );
    }
    OSMemoryHeapFree( g_rglibrarymap );
}


//  ================================================================
LOCAL BOOL FCALLBACKISearchForLibrary( const WCHAR * const wszLibrary, LIBRARY * plibrary )
//  ================================================================
{
    //  see if the library is already loaded
    INT ilibrary;
    for( ilibrary = 0; ilibrary < g_clibrary; ++ilibrary )
    {
        if( 0 == _wcsicmp( wszLibrary, g_rglibrarymap[ilibrary].wszLibrary ) )
        {
            // this library is already loaded
            *plibrary = g_rglibrarymap[ilibrary].library;
            return fTrue;
        }
    }
    return fFalse;
}


//  ================================================================
ERR ErrCALLBACKResolve( const CHAR * const szCallback, JET_CALLBACK * pcallback )
//  ================================================================
{
    JET_ERR err = JET_errSuccess;
    Assert( pcallback );

    CHAR szCallbackT[JET_cbColumnMost+1+3];
    Assert( strlen( szCallback ) < sizeof( szCallbackT ) );

    CAutoWSZDDL     wszLibrary;

    ENTERCRITICALSECTION entercritcallback( &g_critCallback );

    //  do not use sizeof(szCallbackT) here because three characters
    //  are reserved in szCallbackT for '@32'.
    //
    if ( strlen( szCallback ) > JET_cbColumnMost )
    {
        err = ErrERRCheck( JET_errInvalidParameter );
        goto HandleError;
    }
    OSStrCbCopyA( szCallbackT, sizeof(szCallbackT), szCallback );
    const CHAR * const szLibrary = szCallbackT;
    
    CHAR * const pchSep = strchr( szCallbackT, chCallbackSep );
    CHAR * const szFunction = pchSep + 1;
    ULONG        cbFunction = sizeof(szCallbackT) - (szFunction - szCallbackT);
    if( NULL == pchSep )
    {
        err = ErrERRCheck( JET_errInvalidParameter );
        goto HandleError;
    }
    *pchSep = 0;

    Call( wszLibrary.ErrSet( szLibrary ) );
    
    LIBRARY library;

    if( !FCALLBACKISearchForLibrary( (WCHAR*)wszLibrary, &library ) )
    {
        if( FUtilLoadLibrary( (WCHAR*)wszLibrary, &library, g_fEseutil ) )
        {
            //  we were able to load the library. allocate a new rglibrary array
            //
            //  swap the arrays so that threads not in the critical section
            //  can continue to traverse the array.
            //
            const INT clibraryT = g_clibrary + 1;
            LIBRARYMAP * const rglibrarymapOld = g_rglibrarymap;
            LIBRARYMAP * const rglibrarymapNew = (LIBRARYMAP *)PvOSMemoryHeapAlloc( clibraryT * sizeof( LIBRARYMAP ) );
            ULONG cchLibraryT = LOSStrLengthW( (WCHAR*)wszLibrary ) + 1;
            WCHAR * const wszLibraryT = (WCHAR *)PvOSMemoryHeapAlloc( cchLibraryT * sizeof( WCHAR ) );

            //  check for memory allocation failure
            //
            if( NULL == rglibrarymapNew || NULL == wszLibraryT )
            {
                if( NULL != rglibrarymapNew )
                {
                    OSMemoryHeapFree( rglibrarymapNew );
                }
                if( NULL != wszLibraryT )
                {
                    OSMemoryHeapFree( wszLibraryT );
                }
                UtilFreeLibrary( library );
                Call( ErrERRCheck( JET_errOutOfMemory ) );
            }

            OSStrCbCopyW( wszLibraryT, sizeof(WCHAR) * cchLibraryT, (WCHAR*)wszLibrary );
            memcpy( rglibrarymapNew, rglibrarymapOld, g_clibrary * sizeof( LIBRARYMAP ) );
            rglibrarymapNew[g_clibrary].wszLibrary = wszLibraryT;
            rglibrarymapNew[g_clibrary].library = library;

            Assert( rglibrarymapOld == g_rglibrarymap );
            Assert( clibraryT - 1 == g_clibrary );
            g_rglibrarymap = rglibrarymapNew;
            g_clibrary = clibraryT;
            if( NULL != rglibrarymapOld )
            {
                OSMemoryHeapFree( rglibrarymapOld );
            }
        }
        else
        {
            //  log the fact that we couldn't find the callback
            const WCHAR *rgszT[1];
            rgszT[0] = (WCHAR*)wszLibrary;
            UtilReportEvent( eventError, GENERAL_CATEGORY, FILE_NOT_FOUND_ERROR_ID, 1, rgszT );
            Call( ErrERRCheck( JET_errCallbackNotResolved ) );
        }
    }

    //  we now have the library
    *pcallback = (JET_CALLBACK)PfnUtilGetProcAddress( library, szFunction );
    if( NULL == *pcallback )
    {
        OSStrCbAppendA( szFunction, cbFunction, "@32" );
        *pcallback = (JET_CALLBACK)PfnUtilGetProcAddress( library, szFunction );
        if( NULL == *pcallback )
        {
            WCHAR wszFunction[JET_cbColumnMost+1+3];
            
            const WCHAR *rgszT[2];
            OSStrCbFormatW( wszFunction, sizeof( wszFunction ), L"%hs", szFunction );
            
            rgszT[0] = wszFunction;
            rgszT[1] = (WCHAR*)wszLibrary;
            UtilReportEvent( eventError, GENERAL_CATEGORY, FUNCTION_NOT_FOUND_ERROR_ID, 2, rgszT );
            Call( ErrERRCheck( JET_errCallbackNotResolved ) );
        }
    }

HandleError:
    return err;
}


ERR VTAPI ErrIsamSetLS(
    JET_SESID       sesid,
    JET_VTID        vtid,
    JET_LS          ls,
    JET_GRBIT       grbit )
{
    ERR             err;
    PIB             *ppib       = reinterpret_cast<PIB *>( sesid );
    FUCB            *pfucb      = reinterpret_cast<FUCB *>( vtid );
    const BOOL      fReset      = ( grbit & JET_bitLSReset );
    const LSTORE    lsT         = ( fReset ? JET_LSNil : ls );

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRNoLatch( ppib );

    if ( NULL == PvParam( PinstFromPpib( ppib ), JET_paramRuntimeCallback ) )
    {
        return ErrERRCheck( JET_errLSCallbackNotSpecified );
    }

    if ( grbit & JET_bitLSTable )
    {
        if ( grbit & JET_bitLSCursor )
        {
            err = ErrERRCheck( JET_errInvalidGrbit );
        }
        else
        {
            err = pfucb->u.pfcb->ErrSetLS( lsT );
        }
    }
    else
    {
        err = ErrSetLS( pfucb, lsT );
    }

    return err;
}

ERR VTAPI ErrIsamGetLS(
    JET_SESID       sesid,
    JET_VTID        vtid,
    JET_LS          *pls,
    JET_GRBIT       grbit )
{
    ERR             err;
    PIB             *ppib       = reinterpret_cast<PIB *>( sesid );
    FUCB            *pfucb      = reinterpret_cast<FUCB *>( vtid );
    const BOOL      fReset      = ( grbit & JET_bitLSReset );

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRNoLatch( ppib );

    if ( grbit & JET_bitLSTable )
    {
        if ( grbit & JET_bitLSCursor )
        {
            err = ErrERRCheck( JET_errInvalidGrbit );
        }
        else
        {
            err = pfucb->u.pfcb->ErrGetLS( pls, fReset );
        }
    }
    else
    {
        err = ErrGetLS( pfucb, pls, fReset );
    }

    //  if successfully able to retrieve LS, there must be an associated callback
    Assert( err < 0 || NULL != PvParam( PinstFromPpib( ppib ), JET_paramRuntimeCallback ) );

    return err;
}

