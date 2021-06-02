// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifdef DEBUG
#include <stdarg.h>
#endif

VOID UtilLoadDbinfomiscFromPdbfilehdr(
    JET_DBINFOMISC7* pdbinfomisc,
    const ULONG     cbdbinfomisc,
    const DBFILEHDR_FIX *pdbfilehdr )
{
    Assert( cbdbinfomisc == sizeof( JET_DBINFOMISC ) ||
        cbdbinfomisc == sizeof( JET_DBINFOMISC2 ) ||
        cbdbinfomisc == sizeof( JET_DBINFOMISC3 ) ||
        cbdbinfomisc == sizeof( JET_DBINFOMISC4 ) ||
        cbdbinfomisc == sizeof( JET_DBINFOMISC5 ) ||
        cbdbinfomisc == sizeof( JET_DBINFOMISC6 ) ||
        cbdbinfomisc == sizeof( JET_DBINFOMISC7 ));

    pdbinfomisc->ulVersion          = pdbfilehdr->le_ulVersion;
    pdbinfomisc->dbstate            = pdbfilehdr->Dbstate();
    pdbinfomisc->signDb             = *(JET_SIGNATURE *) &(pdbfilehdr->signDb);
    pdbinfomisc->lgposConsistent    = *(JET_LGPOS *) &(pdbfilehdr->le_lgposConsistent);
    pdbinfomisc->logtimeConsistent  = *(JET_LOGTIME *) &(pdbfilehdr->logtimeConsistent);
    pdbinfomisc->logtimeAttach      = *(JET_LOGTIME *) &(pdbfilehdr->logtimeAttach);
    pdbinfomisc->lgposAttach        = *(JET_LGPOS *) &(pdbfilehdr->le_lgposAttach);
    pdbinfomisc->logtimeDetach      = *(JET_LOGTIME *) &(pdbfilehdr->logtimeDetach);
    pdbinfomisc->lgposDetach        = *(JET_LGPOS *) &(pdbfilehdr->le_lgposDetach);
    pdbinfomisc->signLog            = *(JET_SIGNATURE *) &(pdbfilehdr->signLog);
    pdbinfomisc->bkinfoFullPrev     = *(JET_BKINFO *) &(pdbfilehdr->bkinfoFullPrev);
    pdbinfomisc->bkinfoIncPrev      = *(JET_BKINFO *) &(pdbfilehdr->bkinfoIncPrev);
    pdbinfomisc->bkinfoFullCur      = *(JET_BKINFO *) &(pdbfilehdr->bkinfoFullCur);

    C_ASSERT( sizeof(JET_BKLOGTIME) == sizeof(JET_LOGTIME) );
    if ( DBFILEHDR::backupOSSnapshot == pdbfilehdr->bkinfoTypeFullPrev)
        pdbinfomisc->bkinfoFullPrev.bklogtimeMark.fOSSnapshot = 1;
    else
        pdbinfomisc->bkinfoFullPrev.bklogtimeMark.fOSSnapshot = 0;

    if ( DBFILEHDR::backupOSSnapshot == pdbfilehdr->bkinfoTypeIncPrev)
        pdbinfomisc->bkinfoIncPrev.bklogtimeMark.fOSSnapshot = 1;
    else
        pdbinfomisc->bkinfoIncPrev.bklogtimeMark.fOSSnapshot = 0;

    pdbinfomisc->fShadowingDisabled = pdbfilehdr->FShadowingDisabled();
    pdbinfomisc->dwMajorVersion     = pdbfilehdr->le_dwMajorVersion;
    pdbinfomisc->dwMinorVersion     = pdbfilehdr->le_dwMinorVersion;
    pdbinfomisc->dwBuildNumber      = pdbfilehdr->le_dwBuildNumber;
    pdbinfomisc->lSPNumber          = pdbfilehdr->le_lSPNumber;
    pdbinfomisc->ulUpdate           = pdbfilehdr->le_ulDaeUpdateMajor;
    pdbinfomisc->cbPageSize         = pdbfilehdr->le_cbPageSize == 0 ? (ULONG) g_cbPageDefault : (ULONG) pdbfilehdr->le_cbPageSize;

    // if the call is still with the older structure
    // return right here
    //
    if ( cbdbinfomisc < sizeof( JET_DBINFOMISC2 ) )
    {
        Assert( sizeof( JET_DBINFOMISC ) == cbdbinfomisc );
        return;
    }

    pdbinfomisc->genMinRequired         = pdbfilehdr->le_lGenMinRequired;
    pdbinfomisc->genMaxRequired         = pdbfilehdr->le_lGenMaxRequired;
    pdbinfomisc->logtimeGenMaxCreate    = *(JET_LOGTIME *) &(pdbfilehdr->logtimeGenMaxCreate);

    pdbinfomisc->ulRepairCount          = pdbfilehdr->le_ulRepairCount;
    pdbinfomisc->logtimeRepair          = *(JET_LOGTIME *) &(pdbfilehdr->logtimeRepair);
    pdbinfomisc->ulRepairCountOld       = pdbfilehdr->le_ulRepairCountOld;

    pdbinfomisc->ulECCFixSuccess        = pdbfilehdr->le_ulECCFixSuccess;
    pdbinfomisc->logtimeECCFixSuccess   = *(JET_LOGTIME *) &(pdbfilehdr->logtimeECCFixSuccess);
    pdbinfomisc->ulECCFixSuccessOld     = pdbfilehdr->le_ulECCFixSuccessOld;

    pdbinfomisc->ulECCFixFail           = pdbfilehdr->le_ulECCFixFail;
    pdbinfomisc->logtimeECCFixFail      = *(JET_LOGTIME *) &(pdbfilehdr->logtimeECCFixFail);
    pdbinfomisc->ulECCFixFailOld        = pdbfilehdr->le_ulECCFixFailOld;

    pdbinfomisc->ulBadChecksum          = pdbfilehdr->le_ulBadChecksum;
    pdbinfomisc->logtimeBadChecksum     = *(JET_LOGTIME *) &(pdbfilehdr->logtimeBadChecksum);
    pdbinfomisc->ulBadChecksumOld       = pdbfilehdr->le_ulBadChecksumOld;

    if ( cbdbinfomisc < sizeof( JET_DBINFOMISC3 ) )
    {
        Assert( sizeof( JET_DBINFOMISC2 ) == cbdbinfomisc );
        return;
    }

    pdbinfomisc->genCommitted           = pdbfilehdr->le_lGenMaxCommitted;

    if ( cbdbinfomisc < sizeof( JET_DBINFOMISC4 ) )
    {
        Assert( sizeof( JET_DBINFOMISC3 ) == cbdbinfomisc );
        return;
    }

    pdbinfomisc->bkinfoCopyPrev = *( JET_BKINFO* )&pdbfilehdr->bkinfoCopyPrev;
    pdbinfomisc->bkinfoDiffPrev = *( JET_BKINFO* )&pdbfilehdr->bkinfoDiffPrev;

    pdbinfomisc->bkinfoCopyPrev.bklogtimeMark.fOSSnapshot = DBFILEHDR::backupOSSnapshot == pdbfilehdr->bkinfoTypeCopyPrev;
    pdbinfomisc->bkinfoDiffPrev.bklogtimeMark.fOSSnapshot = DBFILEHDR::backupOSSnapshot == pdbfilehdr->bkinfoTypeDiffPrev;

    if ( cbdbinfomisc < sizeof( JET_DBINFOMISC5 ) )
    {
        Assert( sizeof( JET_DBINFOMISC4 ) == cbdbinfomisc );
        return;
    }

    pdbinfomisc->ulIncrementalReseedCount       = pdbfilehdr->le_ulIncrementalReseedCount;
    pdbinfomisc->logtimeIncrementalReseed       = *(JET_LOGTIME *) &(pdbfilehdr->logtimeIncrementalReseed);
    pdbinfomisc->ulIncrementalReseedCountOld    = pdbfilehdr->le_ulIncrementalReseedCountOld;

    pdbinfomisc->ulPagePatchCount               = pdbfilehdr->le_ulPagePatchCount;
    pdbinfomisc->logtimePagePatch               = *(JET_LOGTIME *) &(pdbfilehdr->logtimePagePatch);
    pdbinfomisc->ulPagePatchCountOld            = pdbfilehdr->le_ulPagePatchCountOld;

    if ( cbdbinfomisc < sizeof( JET_DBINFOMISC6 ) )
    {
        Assert( sizeof( JET_DBINFOMISC5 ) == cbdbinfomisc );
        return;
    }

    pdbinfomisc->logtimeChecksumPrev = *( JET_LOGTIME* )&pdbfilehdr->logtimeDbscanPrev;
    pdbinfomisc->logtimeChecksumStart = *( JET_LOGTIME* )&pdbfilehdr->logtimeDbscanStart;
    if (pdbfilehdr->le_pgnoDbscanHighestContinuous)
    {
        pdbinfomisc->cpgDatabaseChecked = pdbfilehdr->le_pgnoDbscanHighestContinuous - 1; // pgnoDbscanHighestContinuous is the next page to be checked
    }
    else
    {
        pdbinfomisc->cpgDatabaseChecked = 0;
    }

    if ( cbdbinfomisc < sizeof( JET_DBINFOMISC7 ) )
    {
        Assert( sizeof( JET_DBINFOMISC6 ) == cbdbinfomisc );
        return;
    }

    Assert( sizeof( JET_DBINFOMISC7 ) == cbdbinfomisc );
    pdbinfomisc->logtimeLastReAttach    = *(JET_LOGTIME *) &(pdbfilehdr->logtimeLastReAttach);
    pdbinfomisc->lgposLastReAttach      = *(JET_LGPOS *) &(pdbfilehdr->le_lgposLastReAttach);

    // Do we need JET_DBINFOMISC8?
    // pdbinfomisc->logtimeGenMaxRequired  = *(JET_LOGTIME *) &(pdbfilehdr->logtimeGenMaxRequired);
}

VOID UtilLoadRBSinfomiscFromRBSfilehdr(
    JET_RBSINFOMISC*  prbsinfomisc,
    const ULONG         cbrbsinfomisc,
    const RBSFILEHDR*   prbsfilehdr )
{
    Assert( cbrbsinfomisc == sizeof( JET_RBSINFOMISC ) );

    prbsinfomisc->lRBSGeneration            = prbsfilehdr->rbsfilehdr.le_lGeneration;
    prbsinfomisc->logtimeCreate             = *(JET_LOGTIME *) &(prbsfilehdr->rbsfilehdr.tmCreate);
    prbsinfomisc->logtimeCreatePrevRBS      = *(JET_LOGTIME *) &(prbsfilehdr->rbsfilehdr.tmPrevGen);
    prbsinfomisc->ulMajor                   = prbsfilehdr->rbsfilehdr.le_ulMajor;
    prbsinfomisc->ulMinor                   = prbsfilehdr->rbsfilehdr.le_ulMinor;
    prbsinfomisc->cbLogicalFileSize         = prbsfilehdr->rbsfilehdr.le_cbLogicalFileSize;
}

LOCAL CODECONST(unsigned char) rgbValidName[16] = {
    0xff,                  /* 00-07 No control characters */
    0xff,                  /* 08-0F No control characters */
    0xff,                  /* 10-17 No control characters */
    0xff,                  /* 18-1F No control characters */
    0x02,                  /* 20-27 No ! */
    0x40,                  /* 28-2F No . */
    0x00,                  /* 30-37 */
    0x00,                  /* 38-3F */
    0x00,                  /* 40-47 */
    0x00,                  /* 48-4F */
    0x00,                  /* 50-57 */
    0x28,                  /* 58-5F No [ or ] */
    0x00,                  /* 60-67 */
    0x00,                  /* 68-6F */
    0x00,                  /* 70-77 */
    0x00,                  /* 78-7F */
};


#ifdef DEBUG
static CHAR g_szLastInvalidName[JET_cbNameMost+1] = "";
static ULONG g_ulCheckErrorLine = 0;
#endif // DEBUG

//  WARNING: Assumes an output buffer of JET_cbNameMost+1
//
ERR ErrUTILICheckName(
    __out_bcount(JET_cbNameMost+1) CHAR * const     szNewName,
    const CHAR * const  szName,
    const BOOL          fTruncate )
{
    ERR                 err = JET_errSuccess;
    const CHAR *                pchLast     = szNewName;
    SIZE_T              cch;
    BYTE                ch;

    C_ASSERT( JET_cbNameMost == 64 ); // ensure we're not getting the Unicode version.

    //  a name must exist and may not begin with a space
    if ( NULL == szName || ' ' == *szName )
    {
        OnDebug( g_ulCheckErrorLine = __LINE__ );
        Error( ErrERRCheck( JET_errInvalidName ) );
    }

    for ( cch = 0;
        cch < JET_cbNameMost && ( ( ch = (BYTE)szName[cch] ) != '\0' );
        cch++ )
    {
        //  extended characters always valid
        if ( ch < 0x80 )
        {
            if ( ( rgbValidName[ch >> 3] >> (ch & 0x7) ) & 1 )
            {
                OnDebug( g_ulCheckErrorLine = __LINE__ );
                Error( ErrERRCheck( JET_errInvalidName ) );
            }
        }

        szNewName[cch] = (CHAR)ch;

        //  last significant character
        if ( ' ' != ch )
            pchLast = szNewName + cch + 1;
    }

    //  check name too long
    //  UNDONE: insignificant trailing spaces that cause
    //  the length of the name to exceed cbNameMost will
    //  currently trigger an error
    if ( JET_cbNameMost == cch )
    {
        if ( !fTruncate && '\0' != szName[JET_cbNameMost] )
        {
            OnDebug( g_ulCheckErrorLine = __LINE__ );
            Error( ErrERRCheck( JET_errInvalidName ) );
        }
    }

    //  length of significant portion
    Assert( pchLast >= szNewName );
    Assert( pchLast <= szNewName + JET_cbNameMost );
    cch = pchLast - szNewName;

    if ( 0 == cch )
    {
        OnDebug( g_ulCheckErrorLine = __LINE__ );
        Error( ErrERRCheck( JET_errInvalidName ) );
    }

    //  we assume an output buffer of JET_cbNameMost+1
    Assert( cch <= JET_cbNameMost );
    szNewName[cch] = '\0';

HandleError:

#ifdef DEBUG
    if ( JET_errInvalidName == err )
    {
        memcpy( g_szLastInvalidName, szName, sizeof(g_szLastInvalidName) );
    }
#endif // DEBUG

    return err;
}


//  WARNING: Assumes an output buffer of IFileSystemAPI::cchPathMax
//
ERR ErrUTILICheckPathName(
    __out_bcount(OSFSAPI_MAX_PATH * sizeof(WCHAR)) PWSTR const      wszNewName,
    const WCHAR * const wszName,
    const BOOL          fTruncate )
{
    SIZE_T              ichT;

    //  path must exist and may not begin with a space
    //
    if ( NULL == wszName || ' ' == *wszName )
    {
        return ErrERRCheck( JET_errInvalidPath );
    }

    for ( ichT = 0;
        ( ( ichT < IFileSystemAPI::cchPathMax ) && ( wszName[ichT] != L'\0' ) );
        ichT++ )
    {
            wszNewName[ichT] = wszName[ichT];
    }

    //  check for empty path
    //
    if ( 0 == ichT )
    {
        return ErrERRCheck( JET_errInvalidPath );
    }

    //  check name too long
    //
    //  FUTURE: insignificant trailing spaces
    //  that cause the length of the name to exceed IFileSystemAPI::cchPathMax
    //  will currently trigger an error.
    //
    if ( IFileSystemAPI::cchPathMax == ichT )
    {
        if ( !fTruncate )
        {
            return ErrERRCheck( JET_errInvalidPath );
        }
        else
        {
            ichT = IFileSystemAPI::cchPathMax - 1;
        }
    }

    //  we assume an output buffer of IFileSystemAPI::cchPathMax
    //
    Assert( ichT < IFileSystemAPI::cchPathMax );
    wszNewName[ichT] = L'\0';

    return JET_errSuccess;
}



#ifdef DEBUG

typedef void ( *PFNvprintf)(const char  *, va_list);

struct {
    PFNvprintf pfnvprintf;
}  pfn = { NULL };


void __cdecl DebugPrintf(const char  *szFmt, ...)
{
    va_list arg_ptr;

    if (pfn.pfnvprintf == NULL)        /* No op if no callback registered */
        return;

    va_start(arg_ptr, szFmt);
    (*pfn.pfnvprintf)(szFmt, arg_ptr);
    va_end(arg_ptr);
}


    /*  The following pragma affects the code generated by the C
    /*  compiler for all FAR functions.  Do NOT place any non-API
    /*  functions beyond this point in this file.
    /**/

void JET_API JetDBGSetPrintFn(JET_SESID sesid, PFNvprintf pfnParm)
{
    Unused( sesid );

    pfn.pfnvprintf = pfnParm;
}

#endif  /* DEBUG */
