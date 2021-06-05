// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_dump.hxx"

const RBS_POS rbsposMin = { 0x0,  0x0 };

ERR ErrBeginDatabaseIncReseedTracing( _In_ IFileSystemAPI* pfsapi, _In_ JET_PCWSTR wszDatabase, _Out_ CPRINTF** ppcprintf );
ERR ErrDBFormatFeatureEnabled_( const JET_ENGINEFORMATVERSION efvFormatFeature, const DbVersion& dbvCurrentFromFile );
VOID TraceFuncBegun( CPRINTF* const pcprintf, const CHAR* const szFunction );
VOID TraceFuncComplete( CPRINTF* const pcprintf, const CHAR* const szFunction, const ERR err );

#ifdef ENABLE_JET_UNIT_TEST
JETUNITTEST( CmpRbspos, Test )
{
    RBS_POS pos1 = { 10, 10 }, pos2 = { 20, 10 }, pos3 = { 10, 20 }, pos4 = { 30, 20 };
    CHECK( CmpRbspos( pos1, pos1 ) == 0 );
    CHECK( CmpRbspos( pos1, pos2 ) < 0 );
    CHECK( CmpRbspos( pos2, pos1 ) > 0 );
    CHECK( CmpRbspos( pos2, pos3 ) < 0 );
    CHECK( CmpRbspos( pos3, pos2 ) > 0 );
    CHECK( CmpRbspos( pos1, pos3 ) < 0 );
    CHECK( CmpRbspos( pos3, pos1 ) > 0 );
    CHECK( CmpRbspos( pos1, pos4 ) < 0 );
    CHECK( CmpRbspos( pos4, pos1 ) > 0 );
}
#endif

VOID RBSResourcesCleanUpFromInst( _In_ INST* const pinst )
{
    if( pinst->m_prbs )
    {
        delete pinst->m_prbs;
        pinst->m_prbs = NULL;
    }

    if( pinst->m_prbsfp )
    {
        delete pinst->m_prbsfp;
        pinst->m_prbsfp = NULL;
    }

    if ( pinst->m_prbscleaner )
    {
        delete pinst->m_prbscleaner;
        pinst->m_prbscleaner = NULL;
    }

    if ( pinst->m_prbsrc )
    {
        delete pinst->m_prbsrc;
        pinst->m_prbsrc = NULL;
    }
}

LOCAL ERR ErrAllocAndSetStr( _In_ PCWSTR wszName, _Out_ WCHAR** pwszResult )
{
    Assert( wszName );
    ERR err = JET_errSuccess;
    LONG cbDest =  ( LOSStrLengthW( wszName ) + 1 ) * sizeof(WCHAR);

    if ( *pwszResult )
    {
        OSMemoryPageFree( *pwszResult );
    }

    *pwszResult = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbDest ) );
    Alloc( *pwszResult );

    Call( ErrOSStrCbCopyW( *pwszResult, cbDest, wszName ) );
HandleError:
    return err;
}

LOCAL ERR ErrRBSAbsRootDir( INST* pinst, __out_bcount( cbRBSRootDir ) PWSTR wszRBSRootDirPath, LONG cbRBSRootDir )
{
    ERR err                 = JET_errSuccess;
    PCWSTR wszRBSFilePath   = FDefaultParam( pinst, JET_paramRBSFilePath ) ? SzParam( pinst, JET_paramLogFilePath ) : SzParam( pinst, JET_paramRBSFilePath );
    WCHAR wszAbsDirRootPath[ IFileSystemAPI::cchPathMax ];

    if ( NULL == wszRBSFilePath || 0 == *wszRBSFilePath )
    {
        return ErrERRCheck(JET_errInvalidParameter);
    }

    Call( pinst->m_pfsapi->ErrPathComplete( wszRBSFilePath, wszAbsDirRootPath ) );
    Call( pinst->m_pfsapi->ErrPathFolderNorm( wszAbsDirRootPath, sizeof( wszAbsDirRootPath ) ) );
    Call( ErrOSStrCbAppendW( wszAbsDirRootPath, sizeof( wszAbsDirRootPath ), wszRBSDirRoot ) );
    Call( pinst->m_pfsapi->ErrPathFolderNorm( wszAbsDirRootPath, sizeof( wszAbsDirRootPath ) ) );

    Assert( cbRBSRootDir > LOSStrLengthW( wszAbsDirRootPath ) * sizeof(WCHAR) ); 

    Call( ErrOSStrCbCopyW( wszRBSRootDirPath, cbRBSRootDir, wszAbsDirRootPath ) );

HandleError:
    return err;
}

LOCAL ERR ErrRBSInitPaths_( INST* pinst, _Out_ WCHAR** wszRBSAbsRootDir, _Out_ WCHAR** wszRBSBaseName )
{
    Assert( pinst );
    Assert( wszRBSAbsRootDir );

    WCHAR   wszAbsDirRootPath[ IFileSystemAPI::cchPathMax ];
    PCWSTR  wszBaseName     = SzParam( pinst, JET_paramBaseName );
    ERR     err             = JET_errSuccess;

    if ( NULL == wszBaseName || 0 == *wszBaseName )
    {
        return ErrERRCheck(JET_errInvalidParameter);
    }

    Call( ErrRBSAbsRootDir( pinst, wszAbsDirRootPath, sizeof( wszAbsDirRootPath ) ) );
    Call( ErrAllocAndSetStr( wszAbsDirRootPath, wszRBSAbsRootDir ) );
    Call( ErrAllocAndSetStr( wszBaseName, wszRBSBaseName ) );

HandleError:
    return err;
}

LOCAL BOOL FRBSCheckForDbConsistency(
    const SIGNATURE* const psignDbHdrFlushFromDb,
    const SIGNATURE* const psigRBSHdrFlushFromDb,
    const SIGNATURE* const psignDbHdrFlushFromRBS,
    const SIGNATURE* const psignRBSHdrFlushFromRBS )
{
    // At least one of the signatures must match and not be uninitialized.
    return ( ( FSIGSignSet( psignDbHdrFlushFromDb ) && 
               memcmp( psignDbHdrFlushFromDb, psignDbHdrFlushFromRBS, sizeof( SIGNATURE ) ) == 0 ) ||
             ( FSIGSignSet( psigRBSHdrFlushFromDb ) &&
               memcmp( psigRBSHdrFlushFromDb, psignRBSHdrFlushFromRBS, sizeof( SIGNATURE ) ) == 0 ) );
}

LOCAL ERR ErrRBSGetDirSize( IFileSystemAPI *pfsapi, PCWSTR wszDirPath, _Out_ QWORD* pcbSize )
{
    ERR err;
    IFileFindAPI    *pffapi = NULL;
    QWORD dirSize = 0;
    *pcbSize = 0;

    WCHAR wszPath[ IFileSystemAPI::cchPathMax ];
    Call( ErrOSStrCbCopyW( wszPath, sizeof( wszPath ), wszDirPath ) );
    Call( pfsapi->ErrPathFolderNorm( wszPath, sizeof( wszPath ) ) );
    Call( ErrOSStrCbAppendW( wszPath, sizeof( wszPath ), L"*" ) );

    Call( pfsapi->ErrFileFind( wszPath, &pffapi ) );

    while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
    {
        WCHAR wszFileName[IFileSystemAPI::cchPathMax];
        WCHAR wszDirT[IFileSystemAPI::cchPathMax];
        WCHAR wszFileT[IFileSystemAPI::cchPathMax];
        WCHAR wszExtT[IFileSystemAPI::cchPathMax];
        WCHAR wszFileNameT[IFileSystemAPI::cchPathMax];
        QWORD cbSize;
        BOOL fFolder;

        Call( pffapi->ErrIsFolder( &fFolder ) );

        Call( pffapi->ErrPath( wszFileName ) );
        Call( pfsapi->ErrPathParse( wszFileName, wszDirT, wszFileT, wszExtT ) );
        wszDirT[0] = 0;
        Call( pfsapi->ErrPathBuild( wszDirT, wszFileT, wszExtT, wszFileNameT ) );

         if ( LOSStrCompareW( wszFileNameT, L"." ) == 0 ||
             LOSStrCompareW( wszFileNameT, L".." ) == 0 )
         {
             continue;
         }

        if ( fFolder )
        {
            Call( ErrRBSGetDirSize ( pfsapi, wszFileName, &cbSize ) );
            dirSize += cbSize;
        }
        else
        {
            Call( pffapi->ErrSize( &cbSize, IFileAPI::filesizeOnDisk ) );
            dirSize += cbSize;
        }
    }

    Call( err == JET_errFileNotFound ? JET_errSuccess : err );

    err = JET_errSuccess;
    *pcbSize = dirSize;

HandleError:

    if ( pffapi != NULL )
    {
        delete pffapi;
    }

    //OSTrace( JET_tracetagRBSCleaner, OSFormat( "\tErrRBSGetDirSize: Directory size (%ls) - %ld, Error - %d\n",  wszDirPath, *pcbSize, err ) );
    return err;
}

/*  Deletes RBS file for a specific RBS generation.
   caller has to make sure szDir has enough space for appending "*"
/**/
LOCAL ERR ErrRBSDeleteAllFiles( IFileSystemAPI *const pfsapi, PCWSTR wszDir, PCWSTR wszFilter, BOOL fRecursive )
{
    Assert( pfsapi );

    ERR             err     = JET_errSuccess;
    IFileFindAPI*   pffapi  = NULL;

    Assert( LOSStrLengthW( wszDir ) + 1 + 1 < IFileSystemAPI::cchPathMax );

    //OSTrace( JET_tracetagRBSCleaner, OSFormat( "\tRBSCleaner delete all files (%ls)\n",  wszDir ) );

    WCHAR wszPath[ IFileSystemAPI::cchPathMax ];
    Call( ErrUtilPathExists( pfsapi, wszDir, wszPath ) );
    Call( pfsapi->ErrPathFolderNorm( wszPath, sizeof( wszPath ) ) );

    if ( !wszFilter )
    {
        Call( ErrOSStrCbAppendW( wszPath, sizeof( wszPath ), L"*" ) );
    }
    else
    {
        Call( ErrOSStrCbAppendW( wszPath, sizeof( wszPath ), wszFilter ) );
    }

    //  iterate over all files in this directory
    Call( pfsapi->ErrFileFind( wszPath, &pffapi ) );
    while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
    {
        WCHAR wszFileName[IFileSystemAPI::cchPathMax];
        WCHAR wszDirT[IFileSystemAPI::cchPathMax];
        WCHAR wszFileT[IFileSystemAPI::cchPathMax];
        WCHAR wszExtT[IFileSystemAPI::cchPathMax];
        WCHAR wszFileNameT[IFileSystemAPI::cchPathMax];
        BOOL fFolder;

        Call( pffapi->ErrIsFolder( &fFolder ) );

        Call( pffapi->ErrPath( wszFileName ) );
        Call( pfsapi->ErrPathParse( wszFileName, wszDirT, wszFileT, wszExtT ) );
        wszDirT[0] = 0;
        Call( pfsapi->ErrPathBuild( wszDirT, wszFileT, wszExtT, wszFileNameT ) );

        /* not . , and .. and not temp
        /**/
        if (    LOSStrCompareW( wszFileNameT, L"." ) != 0 &&
                LOSStrCompareW( wszFileNameT, L".." ) != 0 )
        {
            if ( !fFolder )
            {
                //OSTrace( JET_tracetagRBSCleaner, OSFormat( "\tRBSCleaner deleting file (%ls)\n",  wszFileName ) );
                Call( pfsapi->ErrFileDelete( wszFileName ) );
            }
            else if ( fRecursive )
            {
                Call( ErrRBSDeleteAllFiles( pfsapi, wszFileName, wszFilter, fRecursive ) );
            }
        }
    }

    Call( err == JET_errFileNotFound ? JET_errSuccess : err );
    err = JET_errSuccess;

    if ( fRecursive )
    {
        Call( pfsapi->ErrFolderRemove( wszDir ) );
    }

HandleError:
    err = err == JET_errFileNotFound ? JET_errSuccess : err;

    /*  assert restored szDir
    /**/
    Assert( wszDir[LOSStrLengthW(wszDir)] != L'*' );

    if ( pffapi != NULL )
    {
        delete pffapi;
    }

    //OSTrace( JET_tracetagRBSCleaner, OSFormat( "\tErrRBSDeleteAllFiles error code %d\n",  err ) );
    return err;
}

LOCAL ERR ErrRBSDirPrefix(
    PCWSTR wszLogBaseName,
    __out_bcount( cbDirPrefix ) PWSTR wszRBSDirPrefix,
    LONG cbDirPrefix )
{
    Assert( wszRBSDirPrefix );
    Assert( cbDirPrefix > ( LOSStrLengthW( wszRBSDirBase ) + LOSStrLengthW( wszLogBaseName ) + 1 ) * sizeof(WCHAR) );

    ERR err = JET_errSuccess;
    Call( ErrOSStrCbCopyW( wszRBSDirPrefix, cbDirPrefix, wszRBSDirBase ) );

    // We will use log base name along with static snapshot directory base name to generate each snapshot directory.
    Call( ErrOSStrCbAppendW( wszRBSDirPrefix, cbDirPrefix, wszLogBaseName ) );

HandleError:
    return err;
}

LOCAL ERR ErrRBSGetLowestAndHighestGen_( 
    IFileSystemAPI *const pfsapi, 
    _In_ const PCWSTR wszRBSDirRootPath, 
    _In_ const PCWSTR wszLogBaseName,  
    LONG *rbsGenMin,
    LONG *rbsGenMax )
{
    Assert( rbsGenMin );
    Assert( rbsGenMax );
    Assert( wszRBSDirRootPath );
    Assert( wszLogBaseName );
    Assert( pfsapi );

    WCHAR       wszRBSDirPrefix[ IFileSystemAPI::cchPathMax ];
    ERR             err                 = JET_errSuccess;
    IFileFindAPI*   pffapi              = NULL;
    LONG            lRBSGen                = 0;
    ULONG           cCurrentRBSDigits   = 0;

    *rbsGenMin = 0;
    *rbsGenMax = 0;

    Assert( LOSStrLengthW( wszRBSDirRootPath ) + 1 + 1 < IFileSystemAPI::cchPathMax );

    WCHAR wszSearchPath[ IFileSystemAPI::cchPathMax ];
    Call( ErrOSStrCbCopyW( wszSearchPath, sizeof( wszSearchPath ), wszRBSDirRootPath ) );
    Call( pfsapi->ErrPathFolderNorm( wszSearchPath, sizeof( wszSearchPath ) ) );

    Call( ErrRBSDirPrefix( wszLogBaseName, wszRBSDirPrefix, sizeof( wszRBSDirPrefix ) ) );

    // We will append the snapshot directory base name to the directory path for snapshots and search for all snapshot directories in the given path.
    Call( ErrOSStrCbAppendW( wszSearchPath, sizeof( wszSearchPath ), wszRBSDirPrefix ) );
    Call( ErrOSStrCbAppendW( wszSearchPath, sizeof( wszSearchPath ), L"*" ) );

    //  iterate over all files in this directory
    Call( pfsapi->ErrFileFind( wszSearchPath, &pffapi ) );
    while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
    {
        WCHAR wszFileName[IFileSystemAPI::cchPathMax];
        WCHAR wszDirT[IFileSystemAPI::cchPathMax];
        WCHAR wszFileT[IFileSystemAPI::cchPathMax];
        WCHAR wszExtT[IFileSystemAPI::cchPathMax];

        Call( pffapi->ErrPath( wszFileName ) );
        Call( pfsapi->ErrPathParse( wszFileName, wszDirT, wszFileT, wszExtT ) );

        // Snapshot directory wouldn't have any extension.
        if ( 0 == LOSStrLengthW( wszExtT ) )
        {
            // We can reuse the LG gen generation method to extract the snapshot generation from the snapshot directory name. 
            // TODO SOMEONE: Change this if we our naming convention is going to be different. Naming is specific to Log as well so may be rename or duplicate code?
            err = LGFileHelper::ErrLGGetGeneration( pfsapi, wszFileT, wszRBSDirPrefix, &lRBSGen, wszExtT, &cCurrentRBSDigits);

            if ( lRBSGen < *rbsGenMin || *rbsGenMin == 0 )
            {
                *rbsGenMin = lRBSGen;
            }

            if ( lRBSGen > *rbsGenMax || *rbsGenMax == 0 )
            {
                *rbsGenMax = lRBSGen;
            }
        }
    }
    Call( err == JET_errFileNotFound ? JET_errSuccess : err );

    err = JET_errSuccess;

HandleError:

    delete pffapi;

    return err;
}

// TODO SOMEONE: Taken from LGFileHelper::LGSzLogIdAppend. Can't just reuse due to some asserts on base name. We can may be consider modifying that or just create a new one.
const WCHAR rgwchRBSFileDigits[] =
{
    L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7',
    L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F',
};
const LONG lRBSFileBase = sizeof( rgwchRBSFileDigits )/sizeof( rgwchRBSFileDigits[0] );
const LONG cchRBSDigits = 8; // should we make this configurable?

LOCAL VOID RBSSzIdAppend( __inout_bcount_z( cbFName ) PWSTR wszRBSFileName, size_t cbFName, LONG rbsGeneration )
{
    LONG    ich;
    LONG    ichBase;

    Assert( wszRBSFileName );

    ichBase = LOSStrLengthW( wszRBSFileName ); // LOSStrLengthW(wszLogFileName) for log base name or res log base size

    Assert( cbFName >= ( ( ichBase+cchRBSDigits+1 )*sizeof(WCHAR) ) );

    ich = cchRBSDigits + ichBase - 1;  //  + ichBase name's size - 1 for zero offset

    if ( ( sizeof(WCHAR)*( cchRBSDigits+ichBase+1 ) ) > cbFName )
    {
        AssertSz(false, "Someone passed in a path too short for RBSSzIdAppend(), we'll recover, though I don't know how well");
        if ( cbFName >= sizeof(WCHAR) )
        { // lets at least make sure it is NUL terminated.
            OSStrCbAppendW( wszRBSFileName, cbFName, L"" );
        }
        return;
    }

    for ( ; ich > (ichBase-1); ich-- )
    {
        wszRBSFileName[ich] = rgwchRBSFileDigits[rbsGeneration % lRBSFileBase];
        rbsGeneration = rbsGeneration / lRBSFileBase;
    }
    Assert( rbsGeneration == 0 );
    wszRBSFileName[ichBase+cchRBSDigits] = 0;
}

LOCAL ERR ErrRBSFilePathForGen_( 
    PCWSTR cwszPath,
    PCWSTR cwszRBSBaseName,
    _In_ IFileSystemAPI* const pfsapi,
    __out_bcount ( cbDirPath ) WCHAR* wszRBSDirPath,
    LONG cbDirPath,
    __out_bcount ( cbFilePath ) WCHAR* wszRBSFilePath,
    LONG cbFilePath,
    LONG lRBSGen )
{
    WCHAR       wszFileName[ IFileSystemAPI::cchPathMax ];
    ERR         err     = JET_errSuccess;
    
    Assert ( cwszPath );
    Assert ( wszRBSDirBase );
    Assert ( wszRBSFilePath );
    Assert ( wszRBSDirPath );

    // 2 for folder normalization and 1 for delimiter.
    Assert ( ( LOSStrLengthW( cwszPath ) + LOSStrLengthW( wszRBSDirBase ) + LOSStrLengthW( cwszRBSBaseName ) + cchRBSDigits + 2 + 1) * sizeof(WCHAR) < cbDirPath );

    Call( ErrOSStrCbCopyW( wszRBSDirPath, cbDirPath, cwszPath ) );
    Call( pfsapi->ErrPathFolderNorm( wszRBSDirPath, cbDirPath ) );

    Call( ErrOSStrCbAppendW( wszRBSDirPath, cbDirPath, wszRBSDirBase ) );
    Call( ErrOSStrCbAppendW( wszRBSDirPath, cbDirPath, cwszRBSBaseName ) );

    // Fill in the digits part ( either XXXXX | XXXXXXXX ) ...
    RBSSzIdAppend ( wszRBSDirPath, cbDirPath, lRBSGen );

    Call( pfsapi->ErrPathFolderNorm( wszRBSDirPath, cbDirPath ) );

    // +1 for null character
    Assert ( sizeof( wszFileName ) > ( LOSStrLengthW( cwszRBSBaseName ) + cchRBSDigits + LOSStrLengthW( wszRBSExt ) + 1 ) * sizeof(WCHAR) ); 
    Call( ErrOSStrCbCopyW( wszFileName, sizeof( wszFileName ), cwszRBSBaseName ) );
    
    // Fill in the digits part for the file name ( either XXXXX | XXXXXXXX ) ...
    RBSSzIdAppend ( wszFileName, sizeof( wszFileName ), lRBSGen );

    Assert ( cbFilePath > ( LOSStrLengthW( wszRBSDirPath ) + LOSStrLengthW( wszFileName ) + LOSStrLengthW( wszRBSExt ) + 1 ) * sizeof(WCHAR) );

    Call( pfsapi->ErrPathBuild( wszRBSDirPath, wszFileName, wszRBSExt, wszRBSFilePath ) );

HandleError:
    return err;
}

LOCAL VOID RBSInitFileHdr(
    _In_ const LONG             lRBSGen,
    _Out_ RBSFILEHDR * const    prbsfilehdr,
    _In_ const LOGTIME          tmPrevGen,
    _In_ const SIGNATURE        signPrevRBSHdrFlush )
{
    prbsfilehdr->rbsfilehdr.le_lGeneration          = lRBSGen;
    LGIGetDateTime( &prbsfilehdr->rbsfilehdr.tmCreate );
    prbsfilehdr->rbsfilehdr.tmPrevGen = tmPrevGen;
    prbsfilehdr->rbsfilehdr.le_ulMajor              = ulRBSVersionMajor;
    prbsfilehdr->rbsfilehdr.le_ulMinor              = ulRBSVersionMinor;
    prbsfilehdr->rbsfilehdr.le_filetype             = JET_filetypeSnapshot;
    prbsfilehdr->rbsfilehdr.le_cbLogicalFileSize    = QWORD( 2 * sizeof( RBSFILEHDR ) );
    prbsfilehdr->rbsfilehdr.le_cbDbPageSize         = (USHORT)g_cbPage;

    SIGGetSignature( &prbsfilehdr->rbsfilehdr.signRBSHdrFlush );
    UtilMemCpy( &prbsfilehdr->rbsfilehdr.signPrevRBSHdrFlush, &signPrevRBSHdrFlush, sizeof( SIGNATURE ) );
}

LOCAL ERR ErrRBSInitAttachInfo(
    _In_ RBSATTACHINFO* prbsattachinfo,
    _In_ PCWSTR wszDatabaseName,
    LONG lGenMinRequired,
    LONG lGenMaxRequired,
    _In_ DBTIME dbtimePrevDirtied,
    SIGNATURE signDb, 
    SIGNATURE signDbHdrFlush )
{
    Assert( prbsattachinfo );

    prbsattachinfo->SetPresent( 1 );
    prbsattachinfo->SetDbtimeDirtied( 0 );
    prbsattachinfo->SetDbtimePrevDirtied( dbtimePrevDirtied );
    prbsattachinfo->SetLGenMinRequired( lGenMinRequired );
    prbsattachinfo->SetLGenMaxRequired( lGenMaxRequired );

    // Copy corresponding db signature.
    UtilMemCpy( &prbsattachinfo->signDb, &signDb, sizeof( SIGNATURE ) );
    UtilMemCpy( &prbsattachinfo->signDbHdrFlush, &signDbHdrFlush, sizeof( SIGNATURE ) );

    DWORD cbDatabaseName = (LOSStrLengthW(wszDatabaseName) + 1)*sizeof(WCHAR);
    if ( cbDatabaseName > sizeof(prbsattachinfo->wszDatabaseName) )
    {
        return ErrERRCheck( JET_errBufferTooSmall );
    }
    UtilMemCpy( prbsattachinfo->wszDatabaseName, wszDatabaseName, cbDatabaseName );
    return JET_errSuccess;
}

LOCAL VOID RBSLoadRequiredGenerationFromFMP( INST* pinst, _Out_ LONG* plgenLow, _Out_ LONG* plgenHigh )
{
    LONG lgenLow        = lMax;
    LONG lgenHigh       = lMin;
    BOOL fDatabaseFound = fFalse;

    *plgenLow = 0;
    *plgenHigh = 0;

    // Find the min and max generation required across all the databases in the instance.
    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; ++dbid )
    {
        IFMP        ifmp    = pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP         *pfmp   = &g_rgfmp[ifmp];
        PdbfilehdrReadOnly pdbfilehdr    = pfmp->Pdbfilehdr();
        if ( pdbfilehdr == NULL || ( pdbfilehdr->le_dbstate != JET_dbstateDirtyShutdown && pdbfilehdr->le_dbstate != JET_dbstateDirtyAndPatchedShutdown ) )
            continue;

        fDatabaseFound      = fTrue;

        Assert( pdbfilehdr.get() );

        if ( pdbfilehdr->le_lGenMinRequired < lgenLow )
        {
            lgenLow = min( lgenLow, pdbfilehdr->le_lGenMinRequired );
        }

        if ( pdbfilehdr->le_lGenMaxRequired > lgenHigh )
        {
            lgenHigh = max( lgenHigh, pdbfilehdr->le_lGenMaxRequired );
        }
    }

    if ( fDatabaseFound )
    {
        *plgenLow = lgenLow;
        *plgenHigh = lgenHigh;
    }
}

LOCAL ERR ErrRBSCopyRequiredLogs_( 
    INST* pinst, 
    long lLogGenMinReq, 
    long lLogGenMaxReq, 
    PCWSTR wszSrcDir, 
    PCWSTR wszDestDir, 
    BOOL fOverwriteExisting,
    BOOL fCopyCurrentIfMaxMissing )
{
    Assert( pinst != NULL );
    Assert( pinst->m_pfsapi != NULL );
    Assert( lLogGenMaxReq >= lLogGenMinReq );
    Assert( wszDestDir != NULL );
    Assert( wszSrcDir != NULL );

    WCHAR wszDestLogFilePath[IFileSystemAPI::cchPathMax];
    WCHAR wszLogFilePath[IFileSystemAPI::cchPathMax];
    WCHAR wszDirT[IFileSystemAPI::cchPathMax];
    WCHAR wszFileT[IFileSystemAPI::cchPathMax];
    WCHAR wszExtT[IFileSystemAPI::cchPathMax];

    IFileSystemAPI* pfsapi  = pinst->m_pfsapi;
    ERR             err     = JET_errSuccess;

    Assert( lLogGenMinReq > 0 || lLogGenMinReq == lLogGenMaxReq );

    for (long lGenToCopy = lLogGenMinReq; lGenToCopy <= lLogGenMaxReq && lGenToCopy > 0; ++lGenToCopy )
    {
        wszDestLogFilePath[0] = 0;
        wszFileT[0] = 0;
        wszExtT[0] = 0;

        Call( LGFileHelper::ErrLGMakeLogNameBaselessEx( 
            wszLogFilePath,
            sizeof( wszLogFilePath ),
            pfsapi,
            wszSrcDir,
            SzParam( pinst, JET_paramBaseName ),
            eArchiveLog,
            lGenToCopy,
            ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ? wszOldLogExt : wszNewLogExt,
            ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat ) ? 0 : 8 ) );       

        // Last log may need to be copied from exx.log
        if ( lGenToCopy == lLogGenMaxReq &&
            ErrUtilPathExists( pfsapi, wszLogFilePath ) == JET_errFileNotFound )
        {
            // We don't want to look for current and max log is missing return FileNotFound
            if ( !fCopyCurrentIfMaxMissing )
            {
                continue;
            }

            Call( LGFileHelper::ErrLGMakeLogNameBaselessEx( 
                wszLogFilePath,
                sizeof( wszLogFilePath ),
                pfsapi,
                wszSrcDir,
                SzParam( pinst, JET_paramBaseName ),
                eCurrentLog,
                0,
                ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ? wszOldLogExt : wszNewLogExt,
                ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat ) ? 0 : 8 ) );
        }

        Call( pfsapi->ErrPathParse( wszLogFilePath, wszDirT, wszFileT, wszExtT ) );
        Call( pfsapi->ErrPathBuild( wszDestDir, wszFileT, wszExtT, wszDestLogFilePath ) );

        err = pfsapi->ErrFileCopy( wszLogFilePath, wszDestLogFilePath, fOverwriteExisting );
        Call( err == JET_errFileAlreadyExists ? JET_errSuccess : err );
    }

HandleError:
    return err;
}

LOCAL ERR ErrFolderCopy( IFileSystemAPI *const pfsapi, PCWSTR wszSrcDir, PCWSTR wszDestDir, PCWSTR wszFilter, BOOL fOverwriteExisting, BOOL fRecursive )
{
    ERR             err     = JET_errSuccess;
    IFileFindAPI*   pffapi  = NULL;

    Assert( wszSrcDir );
    Assert( wszDestDir );
    Assert( LOSStrLengthW( wszSrcDir ) + 1 + 1 < IFileSystemAPI::cchPathMax );
    Assert( LOSStrLengthW( wszDestDir ) + 1 + 1 < IFileSystemAPI::cchPathMax );

    WCHAR wszSrcPath[ IFileSystemAPI::cchPathMax ];
    WCHAR wszDestDirPath[ IFileSystemAPI::cchPathMax ];

    Call( ErrUtilPathExists( pfsapi, wszSrcDir, wszSrcPath ) );
    Call( pfsapi->ErrPathFolderNorm( wszSrcPath, sizeof( wszSrcPath ) ) );

    Call( ErrUtilCreatePathIfNotExist( pfsapi, wszDestDir, wszDestDirPath, sizeof ( wszDestDirPath ) ) );
    Call( pfsapi->ErrPathFolderNorm( wszDestDirPath, sizeof( wszDestDirPath ) ) );

    if ( !wszFilter )
    {
        Call( ErrOSStrCbAppendW( wszSrcPath, sizeof( wszSrcPath ), L"*" ) );
    }
    else
    {
        Call( ErrOSStrCbAppendW( wszSrcPath, sizeof( wszSrcPath ), wszFilter ) );
    }

    //  iterate over all files in this directory
    Call( pfsapi->ErrFileFind( wszSrcPath, &pffapi ) );
    while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
    {
        WCHAR wszSrcFileName[IFileSystemAPI::cchPathMax];
        WCHAR wszSrcDirT[IFileSystemAPI::cchPathMax];
        WCHAR wszSrcFileT[IFileSystemAPI::cchPathMax];
        WCHAR wszSrcExtT[IFileSystemAPI::cchPathMax];
        WCHAR wszSrcFileNameT[IFileSystemAPI::cchPathMax];
        WCHAR wszDestFilePathT[IFileSystemAPI::cchPathMax];
        BOOL fFolder;

        Call( pffapi->ErrIsFolder( &fFolder ) );

        Call( pffapi->ErrPath( wszSrcFileName ) );
        Call( pfsapi->ErrPathParse( wszSrcFileName, wszSrcDirT, wszSrcFileT, wszSrcExtT ) );
        wszSrcDirT[0] = 0;
        Call( pfsapi->ErrPathBuild( wszSrcDirT, wszSrcFileT, wszSrcExtT, wszSrcFileNameT ) );

        Assert( LOSStrLengthW( wszDestDirPath ) + LOSStrLengthW( wszSrcFileT ) + LOSStrLengthW( wszSrcExtT ) + 1 + 1 < IFileSystemAPI::cchPathMax );
        Call( pfsapi->ErrPathBuild( wszDestDirPath, wszSrcFileT, wszSrcExtT, wszDestFilePathT ) );

        /* not . , and .. and not temp
        /**/
        if ( LOSStrCompareW( wszSrcFileNameT, L"." ) != 0 &&
            LOSStrCompareW( wszSrcFileNameT, L".." ) != 0 )
        {
            if ( !fFolder )
            {
                Call( pfsapi->ErrFileCopy( wszSrcFileName, wszDestFilePathT, fOverwriteExisting ) );
            }
            else if ( fRecursive )
            {
                Call( pfsapi->ErrPathFolderNorm( wszDestFilePathT, sizeof( wszDestFilePathT ) ) );
                Call( ErrFolderCopy( pfsapi, wszSrcFileName, wszDestFilePathT, wszFilter, fOverwriteExisting, fRecursive ) );
            }
        }
    }

    Call( err == JET_errFileNotFound ? JET_errSuccess : err );
    err = JET_errSuccess;

HandleError:
    if ( pffapi != NULL )
    {
        delete pffapi;
    }

    return err;
}

LOCAL VOID RBSLogCreateSkippedEvent( INST* pinst, PCWSTR wszRBSFilePath, ERR errSkip, ERR errLoad )
{
    WCHAR wszErrSkip[16], wszErrLoad[16];
    OSStrCbFormatW( wszErrSkip, sizeof(wszErrSkip), L"%d", errSkip );
    OSStrCbFormatW( wszErrLoad, sizeof(wszErrLoad), L"%d", errLoad );

    PCWSTR rgcwsz[3];
    rgcwsz[0] = wszRBSFilePath;
    rgcwsz[1] = wszErrSkip;
    rgcwsz[2] = wszErrLoad;

    UtilReportEvent(
        eventError,
        GENERAL_CATEGORY,
        RBS_CREATE_SKIPPED_ID,
        3,
        rgcwsz,
        0,
        NULL,
        pinst );
}

LOCAL VOID RBSLogCreateOrLoadEvent( INST* pinst, PCWSTR wszRBSFilePath, BOOL fNewlyCreated, BOOL fSuccess, ERR err )
{
    WCHAR wszCreatedOrLoaded[16], wszErr[16];
    OSStrCbFormatW( wszCreatedOrLoaded, sizeof(wszCreatedOrLoaded),  L"%ws", fNewlyCreated ? L"created" : L"loaded" );
    OSStrCbFormatW( wszErr, sizeof(wszErr), L"%d", err );

    PCWSTR rgcwsz[3];
    rgcwsz[0] = wszRBSFilePath;
    rgcwsz[1] = wszCreatedOrLoaded;
    rgcwsz[2] = wszErr;

    UtilReportEvent(
            fSuccess ? eventInformation : eventError,
            GENERAL_CATEGORY,
            fSuccess ? RBS_CREATEORLOAD_SUCCESS_ID : RBS_CREATEORLOAD_FAILED_ID,
            3,
            rgcwsz,
            0,
            NULL,
            pinst );
}

LOCAL ERR ErrRBSLoadRbsGen(
    INST*               pinst,
    PWSTR               wszRBSAbsFilePath,
    LONG                lRBSGen,
    _Out_ RBSFILEHDR *  prbshdr, 
    _Out_ IFileAPI**    ppfapiRBS )
{
    Assert( ppfapiRBS );
    Assert( wszRBSAbsFilePath );

    ERR err = JET_errSuccess;

    Call( CIOFilePerf::ErrFileOpen( 
            pinst->m_pfsapi,
            pinst,
            wszRBSAbsFilePath,
            BoolParam( pinst, JET_paramUseFlushForWriteDurability ) ? IFileAPI::fmfStorageWriteBack : IFileAPI::fmfRegular,
            iofileRBS,
            QwInstFileID( qwRBSFileID, pinst->m_iInstance, lRBSGen ),
            ppfapiRBS ) );

    Call( ErrUtilReadShadowedHeader( pinst, pinst->m_pfsapi, *ppfapiRBS, (BYTE*) prbshdr, sizeof( RBSFILEHDR ), -1, urhfNoAutoDetectPageSize ) );

    // check filetype
    if (  JET_filetypeSnapshot != prbshdr->rbsfilehdr.le_filetype )
    {
        // not a rbs file
        Call( ErrERRCheck( JET_errFileInvalidType ) );
    }

    if ( prbshdr->rbsfilehdr.le_ulMajor != ulRBSVersionMajor || prbshdr->rbsfilehdr.le_ulMinor > ulRBSVersionMinor )
    {
        // wrong version
        Call( ErrERRCheck( JET_errBadRBSVersion ) );
    }

    if ( !FSIGSignSet( &prbshdr->rbsfilehdr.signRBSHdrFlush ) )
    {
        // RBS signature not set (intentionally done to invalidate RBS).
        Call( ErrERRCheck( JET_errRBSInvalidSign ) );
    }

HandleError:
    return err;
}

LOCAL ERR ErrRBSPerformLogChecks( 
    INST* pinst, 
    PCWSTR wszRBSAbsRootDirPath,
    PCWSTR wszRBSBaseName,
    LONG lRBSGen,
    BOOL fPerformDivergenceCheck,
    _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) PWSTR wszRBSAbsLogDirPath )
{
    Assert( pinst );
    Assert( wszRBSAbsRootDirPath );
    Assert( wszRBSBaseName );

    WCHAR wszRBSAbsDirPath[ IFileSystemAPI::cchPathMax ];
    WCHAR wszRBSAbsFilePath[ IFileSystemAPI::cchPathMax ];
    WCHAR wszRBSLogGenPath[ IFileSystemAPI::cchPathMax ];
    WCHAR wszMaxReqLogPathActual[ IFileSystemAPI::cchPathMax ];
    RBSFILEHDR rbsfilehdr;

    PCWSTR  wszLogExt       = ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ? wszOldLogExt : wszNewLogExt;
    ULONG   cLogDigits      = ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat )  ? 0 : 8;
    PCWSTR  wszBaseName     = SzParam( pinst, JET_paramBaseName );
    BOOL    fLogsDiverged   = fFalse;
    ERR     err             = JET_errSuccess;
    IFileAPI* pfapirbs      = NULL;

    IFileSystemAPI* pfsapi = pinst->m_pfsapi;
    Assert( pfsapi );

    Call( ErrRBSFilePathForGen_( wszRBSAbsRootDirPath, wszRBSBaseName, pinst->m_pfsapi, wszRBSAbsDirPath, sizeof( wszRBSAbsDirPath ), wszRBSAbsFilePath, cbOSFSAPI_MAX_PATHW, lRBSGen ) );
    Call( CIOFilePerf::ErrFileOpen( pinst->m_pfsapi, pinst, wszRBSAbsFilePath, IFileAPI::fmfReadOnly, iofileRBS, qwRBSFileID, &pfapirbs ) );

    Call( ErrUtilReadShadowedHeader( pinst, pinst->m_pfsapi, pfapirbs, (BYTE*) &rbsfilehdr, sizeof( RBSFILEHDR ), -1, urhfNoAutoDetectPageSize | urhfReadOnly | urhfNoEventLogging ) );
    
    Assert( pinst->m_plog );
    Assert( rbsfilehdr.rbsfilehdr.le_lGenMaxLogCopied >= rbsfilehdr.rbsfilehdr.le_lGenMinLogCopied );

    if ( rbsfilehdr.rbsfilehdr.bLogsCopied != 1 )
    {
        Error( ErrERRCheck( JET_errRBSMissingReqLogs ) );
    }

    Assert( LOSStrLengthW( wszRBSAbsDirPath ) + LOSStrLengthW( wszRBSLogDir ) < IFileSystemAPI::cchPathMax );

    Call( ErrOSStrCbCopyW( wszRBSAbsLogDirPath, cbOSFSAPI_MAX_PATHW, wszRBSAbsDirPath ) );
    Call( ErrOSStrCbAppendW( wszRBSAbsLogDirPath, cbOSFSAPI_MAX_PATHW, wszRBSLogDir ) );

    for ( LONG lgen = rbsfilehdr.rbsfilehdr.le_lGenMaxLogCopied; lgen >= rbsfilehdr.rbsfilehdr.le_lGenMinLogCopied && lgen > 0; --lgen )
    {
        // Check if the RBS log directory has all the required logs
        Call( LGFileHelper::ErrLGMakeLogNameBaselessEx(
            wszRBSLogGenPath,
            sizeof( wszRBSLogGenPath ),
            pfsapi,
            wszRBSAbsLogDirPath,
            wszBaseName,
            eArchiveLog,
            lgen,
            wszLogExt,
            cLogDigits ) );

        err = ErrUtilPathExists( pfsapi, wszRBSLogGenPath );

        if ( err == JET_errFileNotFound )
        {
            // If we are checking the max log gen, it is possible it was the E00.log at the time it was copied.
            if ( lgen == rbsfilehdr.rbsfilehdr.le_lGenMaxLogCopied )
            {
                Call( LGFileHelper::ErrLGMakeLogNameBaselessEx(
                    wszRBSLogGenPath,
                    sizeof( wszRBSLogGenPath ),
                    pfsapi,
                    wszRBSAbsLogDirPath,
                    wszBaseName,
                    eCurrentLog,
                    0,
                    wszLogExt,
                    cLogDigits ) );

                err = ErrUtilPathExists( pfsapi, wszRBSLogGenPath );
            }

            if ( err == JET_errFileNotFound )
            {
                Error( ErrERRCheck( JET_errRBSMissingReqLogs ) );
            }
        }

        Call( err );

        if ( fPerformDivergenceCheck && lgen == rbsfilehdr.rbsfilehdr.le_lGenMaxLogCopied )
        {
            // Do divergence check with the max required log from the snapshot with corresponding log in log directory
            // If max required is missing either in the log directory or snapshot directory we will fail as we don't know if we are diverged then.
            Call( LGFileHelper::ErrLGMakeLogNameBaselessEx(
                wszMaxReqLogPathActual,
                sizeof( wszMaxReqLogPathActual ),
                pfsapi,
                SzParam( pinst, JET_paramLogFilePath ),
                SzParam( pinst, JET_paramBaseName ),
                eArchiveLog,
                lgen,
                ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ? wszOldLogExt : wszNewLogExt,
                ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat ) ? 0 : 8 ) );

            err = ErrUtilPathExists( pfsapi, wszMaxReqLogPathActual );

            // The corresponding log is not available in log directory, so we can't confirm if there is divergence.
            if ( err == JET_errFileNotFound )
            {
                Error( ErrERRCheck( JET_errRBSCannotDetermineDivergence ) );
            }

            Call( err );
            Assert( pinst->m_plog );
            Call( pinst->m_plog->ErrCompareLogs( wszRBSLogGenPath, wszMaxReqLogPathActual, fTrue, &fLogsDiverged ) );

            if ( fLogsDiverged )
            {
                Error( ErrERRCheck( JET_errRBSLogDivergenceFailed ) );
            }
        }
    }

HandleError:
    if ( pfapirbs )
    {
        delete pfapirbs;
    }

    return err;
}

LOCAL BOOL FRBSFormatFeatureEnabled( const RBSVersion rbsvFormatFeature, const RBSFILEHDR* const prbsfilehdr )
{
    if ( prbsfilehdr->rbsfilehdr.le_ulMajor > rbsvFormatFeature.ulMajor )
    {
        return fTrue;
    }

    if ( prbsfilehdr->rbsfilehdr.le_ulMajor < rbsvFormatFeature.ulMajor )
    {
        return fFalse;
    }

    return prbsfilehdr->rbsfilehdr.le_ulMinor >= rbsvFormatFeature.ulMinor;
}

// Find the RBSAttachInfo corresponding to the given database name in our RBS file header
// If rbs attach info not found we will return the first free slot.
LOCAL ERR ErrRBSFindAttachInfoForDBName( _In_ RBSFILEHDR* prbsfilehdrCurrent, _In_ PCWSTR wszDatabaseName, _Out_ RBSATTACHINFO** prbsattachinfo )
{
    Assert( prbsattachinfo );

    ERR err;
    BOOL fFound = fFalse;

    *prbsattachinfo = (RBSATTACHINFO*) prbsfilehdrCurrent->rgbAttach;

    for ( const BYTE * pbT = prbsfilehdrCurrent->rgbAttach; 0 != *pbT; pbT += sizeof( RBSATTACHINFO ), *prbsattachinfo = (RBSATTACHINFO*) pbT )
    {
        if ( (*prbsattachinfo)->FPresent() == 0 )
        {
            break;
        }
        
        // Found the corresponding attach info.
        CAutoWSZ wszDatabase;
        CallR( wszDatabase.ErrSet( (*prbsattachinfo)->wszDatabaseName ) );
        if ( UtilCmpFileName( wszDatabaseName, wszDatabase ) == 0 )
        {
            fFound = fTrue;
            break;
        }
    }

    return fFound ? JET_errSuccess : ErrERRCheck( errRBSAttachInfoNotFound );
}

LOCAL ERR ErrRBSRollAttachInfo( 
    BOOL fPrevRBSValid,
    RBSFILEHDR* prbsfilehdrPrev,
    RBSATTACHINFO* prbsattachinfoNew,
    _In_ PCWSTR wszDatabaseName,
    LONG lGenMinRequired,
    LONG lGenMaxRequired,
    SIGNATURE signDb, 
    SIGNATURE signDbHdrFlush )
{
    RBSATTACHINFO* prbsattachinfoOld           = NULL;
    JET_ERR err                             = fPrevRBSValid ? ErrRBSFindAttachInfoForDBName( prbsfilehdrPrev, wszDatabaseName, &prbsattachinfoOld ) : JET_errRBSHeaderCorrupt;
    LittleEndian<DBTIME>  dbtimePrevDirtied = ( err == JET_errSuccess ) ? prbsattachinfoOld->DbtimeDirtied() : 0;

    return ErrRBSInitAttachInfo( prbsattachinfoNew, wszDatabaseName, lGenMinRequired, lGenMaxRequired, dbtimePrevDirtied, signDb, signDbHdrFlush );
}


// TODO SOMEONE: For now we will just go through all the dbs currently part of this instance. 
// If some were part of previous instance, but they aren't part of this one anymore, 
// they will be dropped as we would lack certain information required for RBS consistency.
LOCAL ERR ErrRBSRollAttachInfos( INST* pinst, RBSFILEHDR* prbsfilehdrCurrent, RBSFILEHDR* prbsfilehdrPrev, BOOL fPrevRBSValid, BOOL fInferFromRstmap )
{
    Assert( pinst );
    Assert( prbsfilehdrCurrent );
    Assert( prbsfilehdrPrev );

    RBSATTACHINFO* prbsattachinfoCur    = (RBSATTACHINFO*) prbsfilehdrCurrent->rgbAttach;
    ERR err                             = JET_errSuccess;

    if ( fInferFromRstmap )
    {
        Assert( pinst->m_plog );

        RSTMAP          *rgrstmap   = pinst->m_plog->Rgrstmap();
        INT             irstmapMac  = pinst->m_plog->IrstmapMac();

        for ( INT irstmap = 0; irstmap < irstmapMac; irstmap++ )
        {
            const RSTMAP * const prstmap = rgrstmap + irstmap;

            if ( prstmap->fFileNotFound )
            {
                continue;
            }

            Call( ErrRBSRollAttachInfo( fPrevRBSValid, prbsfilehdrPrev, prbsattachinfoCur, prstmap->wszNewDatabaseName, prstmap->lGenMinRequired, prstmap->lGenMaxRequired, prstmap->signDatabase, prstmap->signDatabaseHdrFlush ) );
            prbsattachinfoCur++;
        }
    }
    else
    {
        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            if ( pinst->m_mpdbidifmp[ dbid ] >= g_ifmpMax )
            {
                continue;
            }

            FMP* pfmp                       = &g_rgfmp[ pinst->m_mpdbidifmp[ dbid ] ];
            PdbfilehdrReadOnly pdbHeader    = pfmp->Pdbfilehdr();

           Call( ErrRBSRollAttachInfo( fPrevRBSValid, prbsfilehdrPrev, prbsattachinfoCur, pfmp->WszDatabaseName(), pdbHeader->le_lGenMinRequired, pdbHeader->le_lGenMaxRequired, pdbHeader->signDb, pdbHeader->signDbHdrFlush ) );
            prbsattachinfoCur++;
        }
    }

HandleError:
    return err;
}

// static
VOID *CSnapshotBuffer::s_pReserveBuffer = NULL;

//  ================================================================
//                  CRevertSnapshot
//  ================================================================

CRevertSnapshot::CRevertSnapshot( _In_ INST* const pinst ) :
    m_pinst ( pinst ),
    m_cresRBSBuf( pinst ),
    m_critBufferLock( CLockBasicInfo( CSyncBasicInfo( szRBSBuf ), rankRBSBuf, 0 ) ),
    m_critWriteLock( CLockBasicInfo( CSyncBasicInfo( szRBSWrite ), rankRBSWrite, 0 ) )
{
    Assert( pinst );
    m_wszRBSAbsRootDirPath  = NULL;
    m_wszRBSBaseName        = NULL;
    m_fInitialized          = 0;
    m_fInvalid              = 0;
    m_fDumping              = 0;
    m_fPatching             = 0;
    m_cNextActiveSegment    = 0;
    m_cNextWriteSegment     = 0;
    m_cNextFlushSegment     = 0;
    m_fWriteInProgress      = fFalse;
    m_pActiveBuffer         = NULL;
    m_pBuffersToWrite       = NULL;
    m_pBuffersToWriteLast   = NULL;
    m_pReadBuffer           = NULL;
    m_tickLastFlush         = 0;
    m_prbsfilehdrCurrent    = NULL;
    m_wszRBSCurrentFile     = NULL;
    m_wszRBSCurrentLogDir   = NULL;
    m_pfapiRBS              = NULL;
}

CRevertSnapshot::~CRevertSnapshot( )
{
    FreeFileApi( );
    FreeHdr( );
    FreePaths( );

    if ( m_pActiveBuffer )
    {
        delete m_pActiveBuffer;
        m_pActiveBuffer = NULL;
    }

    for ( CSnapshotBuffer *pBuffer = m_pBuffersToWrite; pBuffer != NULL; )
    {
        CSnapshotBuffer *pBufferNext = pBuffer->m_pNextBuffer;
        delete pBuffer;
        pBuffer = pBufferNext;
    }
    m_pBuffersToWrite = NULL;
    m_pBuffersToWriteLast = NULL;

    m_cresRBSBuf.Term();

    if ( m_pReadBuffer )
    {
        delete m_pReadBuffer;
        m_pReadBuffer = NULL;
    }

    CSnapshotBuffer::FreeReserveBuffer();
}

ERR CRevertSnapshot::ErrResetHdr( )
{
    ERR err = JET_errSuccess;
    FreeHdr( );
    Alloc( m_prbsfilehdrCurrent = (RBSFILEHDR *)PvOSMemoryPageAlloc( sizeof(RBSFILEHDR), NULL ) );

HandleError:
    return err;
}

VOID CRevertSnapshot::EnterDbHeaderFlush( CRevertSnapshot* prbs, _Out_ SIGNATURE* const psignRBSHdrFlush )
{
    if ( prbs != NULL && prbs->m_fInitialized && !prbs->m_fInvalid )
    {
        UtilMemCpy( psignRBSHdrFlush, &prbs->m_prbsfilehdrCurrent->rbsfilehdr.signRBSHdrFlush, sizeof( SIGNATURE ) );
    }
    else
    {
        SIGResetSignature( psignRBSHdrFlush );
    }
}

// Used to set file api for dumping snapshot
ERR CRevertSnapshot::ErrSetRBSFileApi( _In_ IFileAPI *pfapiRBS )
{
    // Make sure it hasn't already been initialized to something else.
    Assert( !m_fInitialized );
    Assert( !m_fInvalid );
    Assert( m_pfapiRBS == NULL );
    Assert( m_wszRBSCurrentFile == NULL );
    Assert( m_prbsfilehdrCurrent == NULL );
    Assert( pfapiRBS != NULL );

    ERR err = JET_errSuccess;
    m_pfapiRBS = pfapiRBS;

    WCHAR       wszRBSAbsFilePath[ IFileSystemAPI::cchPathMax ];

    Call( pfapiRBS->ErrPath( wszRBSAbsFilePath ) );
    
    const SIZE_T    cchRBSName       = sizeof( WCHAR ) * ( LOSStrLengthW( wszRBSAbsFilePath ) + 1 );
    Alloc( m_wszRBSCurrentFile = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cchRBSName ) ) );
    Call( ErrOSStrCbCopyW( m_wszRBSCurrentFile, cchRBSName, wszRBSAbsFilePath ) );

    Alloc( m_prbsfilehdrCurrent = (RBSFILEHDR *)PvOSMemoryPageAlloc( sizeof(RBSFILEHDR), NULL ) );    

    // Load the header in the snapshot based on the set file api
    Call( ErrUtilReadShadowedHeader( m_pinst, m_pinst->m_pfsapi, m_pfapiRBS, (BYTE*) m_prbsfilehdrCurrent, sizeof( RBSFILEHDR ), -1, urhfNoAutoDetectPageSize | urhfReadOnly | urhfNoEventLogging ) );
    m_fInitialized = fTrue;

HandleError:
    return err;
}

ERR CRevertSnapshot::ErrRBSCreateOrLoadRbsGen(
    long lRBSGen, 
    LOGTIME tmPrevGen,
    _In_ const SIGNATURE signPrevRBSHdrFlush,
    _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) PWSTR wszRBSAbsFilePath,
    _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) PWSTR wszRBSAbsLogDirPath )
{
    WCHAR       wszAbsDirRootPath[ IFileSystemAPI::cchPathMax ];
    WCHAR       wszRBSAbsDirPath[ IFileSystemAPI::cchPathMax ];
    WCHAR       wszRBSAbsLogPath[ IFileSystemAPI::cchPathMax ];

    PCWSTR      wszBaseName             = SzParam( m_pinst, JET_paramBaseName );
    ERR         err                     = JET_errSuccess;
    BOOL        fRBSFileExists          = fFalse;
    BOOL        fRBSGenDirExists        = fFalse;
    BOOL        fLogDirCreated          = fFalse;
    BOOL        fRBSFileCreated         = fFalse;
    BOOL        fRBSGenDirCreated       = fFalse;
    QWORD       cbDefaultFileSize       = QWORD( 2 * sizeof( RBSFILEHDR ) );

    if ( NULL == wszBaseName || 0 == *wszBaseName )
    {
        return ErrERRCheck(JET_errInvalidParameter);
    }

    Call( ErrRBSAbsRootDir( m_pinst, wszAbsDirRootPath, sizeof( wszAbsDirRootPath ) ) );
    Call( ErrRBSFilePathForGen_( wszAbsDirRootPath, wszBaseName, m_pinst->m_pfsapi, wszRBSAbsDirPath, sizeof( wszRBSAbsDirPath ), wszRBSAbsFilePath, cbOSFSAPI_MAX_PATHW, lRBSGen ) );

    // Generate Logs directory for snapshot.
    Assert ( sizeof( wszRBSAbsLogPath ) > ( LOSStrLengthW( wszRBSAbsDirPath ) + LOSStrLengthW( wszRBSLogDir ) + 1 + 1 ) * sizeof(WCHAR) ); // + 2 for normalization and delimiter
    Call( ErrOSStrCbCopyW( wszRBSAbsLogPath, sizeof( wszRBSAbsLogPath ), wszRBSAbsDirPath ) );
    Call( ErrOSStrCbAppendW( wszRBSAbsLogPath, sizeof( wszRBSAbsLogPath ), wszRBSLogDir ) );
    Call( m_pinst->m_pfsapi->ErrPathFolderNorm( wszRBSAbsLogPath, sizeof( wszRBSAbsLogPath ) ) );

    fRBSGenDirExists = ( ErrUtilPathExists( m_pinst->m_pfsapi, wszRBSAbsDirPath ) == JET_errSuccess );

    if ( !fRBSGenDirExists )
    {
        Call( ErrUtilCreatePathIfNotExist( m_pinst->m_pfsapi, wszRBSAbsFilePath, NULL, 0 ) );
        fRBSGenDirCreated = fTrue;
    }

    fRBSFileExists = ( ErrUtilPathExists( m_pinst->m_pfsapi, wszRBSAbsFilePath ) == JET_errSuccess );

    if ( !fRBSFileExists )
    {
        // TODO SOMEONE: Check how is this being used and what params to set.
        TraceContextScope tcHeader( iorpRBS, iorsHeader );

        // TODO SOMEONE: If this is the first snapshot then it is expected. If not, we got interrupted after creating the snapshot directory and before creating the snapshot file. 
        // How do we handle the interruption case? Need to consider snapshot rolling failure scenarios.
        // For now just create the snapshot file if doesn't exist.
        Call( CIOFilePerf::ErrFileCreate(
            m_pinst->m_pfsapi,
            m_pinst,
            wszRBSAbsFilePath,
            BoolParam( m_pinst, JET_paramUseFlushForWriteDurability ) ? IFileAPI::fmfStorageWriteBack : IFileAPI::fmfRegular,
            iofileRBS,
            QwInstFileID( qwRBSFileID, m_pinst->m_iInstance, lRBSGen ),
            &m_pfapiRBS ) );

        fRBSFileCreated = true;

        // Set size to store header and shadow header.
        Call( m_pfapiRBS->ErrSetSize( *tcHeader, cbDefaultFileSize, fTrue, qosIONormal ) );

        RBSInitFileHdr( lRBSGen, m_prbsfilehdrCurrent, tmPrevGen, signPrevRBSHdrFlush );

        Call( ErrUtilWriteRBSHeaders( m_pinst, m_pinst->m_pfsapi, NULL, m_prbsfilehdrCurrent, m_pfapiRBS ) );
    }
    else
    {
        Call( ErrRBSLoadRbsGen( m_pinst, wszRBSAbsFilePath, lRBSGen, m_prbsfilehdrCurrent, &m_pfapiRBS ) );
    }

    m_cNextFlushSegment = m_cNextWriteSegment = m_cNextActiveSegment = IsegRBSSegmentOfFileOffset( m_prbsfilehdrCurrent->rbsfilehdr.le_cbLogicalFileSize );
    if ( m_pActiveBuffer != NULL )
    {
        m_pActiveBuffer->Reset( m_cNextActiveSegment );
    }

    Call( ErrOSStrCbCopyW( wszRBSAbsLogDirPath, cbOSFSAPI_MAX_PATHW, wszRBSAbsLogPath ) );

    if ( ErrUtilPathExists( m_pinst->m_pfsapi, wszRBSAbsLogPath ) != JET_errSuccess )
    {
        Call( ErrUtilCreatePathIfNotExist( m_pinst->m_pfsapi, wszRBSAbsLogPath, NULL, 0 ) );
        fLogDirCreated = fTrue;
    }

    RBSLogCreateOrLoadEvent( m_pinst, wszRBSAbsFilePath, fRBSFileCreated, fTrue, JET_errSuccess );
    return JET_errSuccess;

HandleError:

    RBSLogCreateOrLoadEvent( m_pinst, wszRBSAbsFilePath, fRBSFileCreated, fFalse, err );
    if ( fRBSFileCreated )
    {
        CallSx( m_pinst->m_pfsapi->ErrFileDelete( wszRBSAbsFilePath ), JET_errFileNotFound );
    }

    if ( fLogDirCreated )
    {
        CallSx( m_pinst->m_pfsapi->ErrFolderRemove( wszRBSAbsLogPath ), JET_errFileNotFound );
    }

    if ( fRBSGenDirCreated )
    {
        CallSx( m_pinst->m_pfsapi->ErrFolderRemove( wszRBSAbsDirPath ), JET_errFileNotFound );
    }

    return err;
}

ERR CRevertSnapshot::ErrRBSInit( BOOL fRBSCreateIfRequired, ERR createSkippedError )
{    
    //OSTrace( JET_tracetagRBS, OSFormat( "\tErrRBSInit(DBLogPath - %ws, RBSEnabled - %d)\n", SzParam( m_pinst, JET_paramRBSFilePath ), BoolParam( m_pinst, JET_paramEnableRBS ) ) );
    
    // For now, we will not init if revert snapshot hasn't been enabled. Enable/disable is only allowed during db attach/create
    if ( !BoolParam( m_pinst, JET_paramEnableRBS ) || ( m_pinst->m_plog->FLogDisabled() && !m_fPatching ) )
    {
        return JET_errSuccess;
    }

    WCHAR   wszAbsDirRootPath[ IFileSystemAPI::cchPathMax ];
    WCHAR   wszRBSAbsFilePath[ IFileSystemAPI::cchPathMax ];
    WCHAR   wszRBSAbsLogDirPath[ IFileSystemAPI::cchPathMax ];

    LONG    rbsGenMax;
    LONG    rbsGenMin;
    ERR     err             = JET_errSuccess;
    LOGTIME defaultLogTime  = { 0 };
    SIGNATURE defaultSign   = { 0 };

    Call( ErrRBSInitPaths_( m_pinst, &m_wszRBSAbsRootDirPath, &m_wszRBSBaseName ) );

    // Create the snapshot directory root path if not already exists. 
    Call( ErrUtilCreatePathIfNotExist( m_pinst->m_pfsapi, m_wszRBSAbsRootDirPath, wszAbsDirRootPath, sizeof ( wszAbsDirRootPath ) ) );
    Call( m_pinst->m_pfsapi->ErrPathFolderNorm( wszAbsDirRootPath, sizeof( wszAbsDirRootPath ) ) );

    Assert( LOSStrLengthW( wszAbsDirRootPath ) > 0 );
    Assert( LOSStrLengthW( m_wszRBSBaseName ) > 0 );

    Call( ErrRBSGetLowestAndHighestGen_( m_pinst->m_pfsapi, wszAbsDirRootPath, m_wszRBSBaseName, &rbsGenMin, &rbsGenMax ) );

    Call( ErrResetHdr() );

    // We don't have any snapshot directory. Lets create one if allowed.
    if ( rbsGenMax == 0 )
    {
        // We should not create snapshot if not allowed.
        if ( !fRBSCreateIfRequired )
        {
            RBSLogCreateSkippedEvent( m_pinst, SzParam( m_pinst, JET_paramRBSFilePath ), createSkippedError, JET_errFileNotFound );
            Error( ErrERRCheck( createSkippedError ) );
        }

        rbsGenMax = 1;
    }

    err = ErrRBSCreateOrLoadRbsGen( rbsGenMax, defaultLogTime, defaultSign, wszRBSAbsFilePath, wszRBSAbsLogDirPath );
    
    // Current snapshot is corrupted. We can't use any snapshot beyond this point.
    // We will start a fresh snapshot and let RBSCleaner cleanup any of the older snapshots.
    if ( err == JET_errReadVerifyFailure || err == JET_errFileInvalidType || err == JET_errBadRBSVersion || err == JET_errRBSInvalidSign )
    {
        Call( ErrResetHdr() );
        FreeFileApi();

        if ( m_pinst->m_prbscleaner != NULL )
        {
            m_pinst->m_prbscleaner->SetFirstValidGen( rbsGenMax + 1 );
        }

        if ( fRBSCreateIfRequired )
        {
            // Increase the gen by 1.
            Call( ErrRBSCreateOrLoadRbsGen( rbsGenMax + 1, defaultLogTime, defaultSign, wszRBSAbsFilePath, wszRBSAbsLogDirPath ) );
        }
        else
        {
            RBSLogCreateSkippedEvent( m_pinst, SzParam( m_pinst, JET_paramRBSFilePath ), createSkippedError, err );
            Error( ErrERRCheck( createSkippedError ) );
        }
    }
    Call( err );

    Assert( m_cNextActiveSegment > 0 );
    if ( m_cNextActiveSegment > cRBSSegmentMax )
    {
        Error( ErrERRCheck( JET_errOutOfRBSSpace ) );
    }

    const SIZE_T    cchRBSName       = sizeof( WCHAR ) * ( LOSStrLengthW( wszRBSAbsFilePath ) + 1 );
    m_wszRBSCurrentFile = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cchRBSName ) );
    Alloc( m_wszRBSCurrentFile );

    const SIZE_T    cchRBSLogDirName       = sizeof( WCHAR ) * ( LOSStrLengthW( wszRBSAbsLogDirPath ) + 1 );
    m_wszRBSCurrentLogDir = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cchRBSLogDirName ) );
    Alloc( m_wszRBSCurrentLogDir );

    Call( ErrOSStrCbCopyW( m_wszRBSCurrentFile, cchRBSName, wszRBSAbsFilePath ) );
    Call( ErrOSStrCbCopyW( m_wszRBSCurrentLogDir, cchRBSLogDirName, wszRBSAbsLogDirPath ) );

    Call( m_cresRBSBuf.ErrInit( JET_residRBSBuf ) );

    Alloc( m_pActiveBuffer = new (&m_cresRBSBuf) CSnapshotBuffer( m_cNextActiveSegment, &m_cresRBSBuf ) );
    Call( m_pActiveBuffer->ErrAllocBuffer() );

    // Pre-allocate reserve buffer, just in case, it is needed in future.
    CSnapshotBuffer::PreAllocReserveBuffer();

    m_fInitialized = fTrue;

    Assert( m_prbsfilehdrCurrent );
    Assert( m_prbsfilehdrCurrent->rbsfilehdr.le_lGeneration > 0 );

    // tmPrevGen is not set. So should be invalid.
    if ( m_pinst->m_prbscleaner && !m_prbsfilehdrCurrent->rbsfilehdr.tmPrevGen.FIsSet() )
    {
        m_pinst->m_prbscleaner->SetFirstValidGen( m_prbsfilehdrCurrent->rbsfilehdr.le_lGeneration );
    }

    return JET_errSuccess;

HandleError:
    FreeFileApi();
    FreeHdr();
    delete m_pActiveBuffer;
    m_pActiveBuffer = NULL;

    return err;
}

ERR CRevertSnapshot::ErrRBSInvalidate()
{
    Assert( FInitialized() );

    m_fInvalid = fTrue;

    // Reset signature so that next time we load the RBS is considered invalid.
    SIGResetSignature( &m_prbsfilehdrCurrent->rbsfilehdr.signRBSHdrFlush );
    return ErrUtilWriteRBSHeaders( m_pinst, m_pinst->m_pfsapi, NULL, m_prbsfilehdrCurrent, m_pfapiRBS );
}

VOID RBSICompressPreImage(
    INST *pinst,
    IFMP ifmp,
    PGNO pgno,
    const LONG cbPage,
    DATA &dataToSet,
    BYTE *pbDataDehydrated,
    BYTE *pbDataCompressed,
    ULONG *pcompressionPerformed )
{
    *pcompressionPerformed = 0;
    INT cbDataCompressedActual = 0;

#if 0
    // Uncomment to have each page being capture go through data checksum to make sure there is no corruption
    {
    CPAGE cpageT;
    if ( dataToSet.Cb() == cbPage )
    {
        cpageT.LoadPage( ifmp, pgno, dataToSet.Pv(), dataToSet.Cb() );
    }
    else
    {
        cpageT.LoadDehydratedPage( ifmp, pgno, dataToSet.Pv(), dataToSet.Cb(), cbPage );
    }
    cpageT.LoggedDataChecksum();
    cpageT.UnloadPage();
    }
#endif

    // First try dehydration (may already be dehydrated)
    if ( dataToSet.Cb() == cbPage )
    {
        memcpy( pbDataDehydrated, dataToSet.Pv(), dataToSet.Cb() );
        CPAGE cpageT;
        cpageT.LoadPage( ifmp, pgno, pbDataDehydrated, dataToSet.Cb() );
        if ( cpageT.FPageIsDehydratable( (ULONG *)&cbDataCompressedActual, fTrue ) )
        {
            cpageT.DehydratePage( cbDataCompressedActual, fTrue );
            dataToSet.SetPv( pbDataDehydrated );
            dataToSet.SetCb( cbDataCompressedActual );
            *pcompressionPerformed |= fRBSPreimageDehydrated;
        }
        cpageT.UnloadPage();
    }
    else
    {
        Assert( dataToSet.Cb() < cbPage );
        *pcompressionPerformed |= fRBSPreimageDehydrated;
    }

    CompressFlags compressFlags = compressXpress;
    // If xpress10 is enabled, use that.
    if ( BoolParam( pinst, JET_paramFlight_EnableXpress10Compression ) )
    {
        compressFlags = CompressFlags( compressFlags | compressXpress10 );
    }
    // If lz4 is enabled, use that.
    if ( BoolParam( pinst, JET_paramFlight_EnableLz4Compression ) )
    {
        compressFlags = CompressFlags( compressFlags | compressLz4 );
    }

    // Now try xpress compression on (the possibly dehydrated) page
    if ( ErrPKCompressData( dataToSet, compressFlags, pinst, pbDataCompressed, CbPKCompressionBuffer(), &cbDataCompressedActual ) >= JET_errSuccess &&
         cbDataCompressedActual < dataToSet.Cb() )
    {
        dataToSet.SetPv( pbDataCompressed );
        dataToSet.SetCb( cbDataCompressedActual );
        *pcompressionPerformed |= fRBSPreimageCompressed;
    }
}

ERR ErrRBSDecompressPreimage(
    DATA &data,
    const LONG cbPage,
    BYTE *pbDataDecompressed,
    PGNO pgno,
    ULONG fFlags )
{
    ERR err;

    // First xpress decompression
    if ( fFlags & fRBSPreimageCompressed )
    {
        INT cbActual = 0;
        CallR( ErrPKDecompressData( data, ifmpNil, pgno, pbDataDecompressed, cbPage, &cbActual ) );
        Assert( err != JET_wrnBufferTruncated );
        data.SetPv( pbDataDecompressed );
        data.SetCb( cbActual );
    }

    // Then rehydration
    if ( ( fFlags & fRBSPreimageDehydrated ) || data.Cb() < cbPage )
    {
        // If xpress also done, then data is already in decompression buffer
        if ( fFlags & fRBSPreimageCompressed )
        {
            Assert( data.Pv() == pbDataDecompressed );
        }
        else
        {
            memcpy( pbDataDecompressed, data.Pv(), data.Cb() );
        }
        CPAGE cpageT;
        cpageT.LoadDehydratedPage( ifmpNil, pgno, pbDataDecompressed, data.Cb(), cbPage );
        cpageT.RehydratePage();
        data.SetPv( pbDataDecompressed );
        Assert( cpageT.CbBuffer() == (ULONG)cbPage );
        data.SetCb( cbPage );
        cpageT.UnloadPage();
    }

    Assert( data.Cb() == cbPage );
    return JET_errSuccess;
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( FRBSCheckForDbConsistency, Test )
{
    SIGNATURE signDbHdrFlush, sigRBSHdrFlush;
    SIGGetSignature( &signDbHdrFlush );
    SIGGetSignature( &sigRBSHdrFlush );

    SIGNATURE signDbHdrFlushFromDb, sigRBSHdrFlushFromDb, signDbHdrFlushFromRBS, signRBSHdrFlushFromRBS;
    CHECK( FRBSCheckForDbConsistency( &signDbHdrFlushFromDb, &sigRBSHdrFlushFromDb, &signDbHdrFlushFromRBS, &signRBSHdrFlushFromRBS ) == fFalse );

    UtilMemCpy( &signDbHdrFlushFromDb, &signDbHdrFlush, sizeof( SIGNATURE ) );
    UtilMemCpy( &signRBSHdrFlushFromRBS, &sigRBSHdrFlush, sizeof( SIGNATURE ) );
    CHECK( FRBSCheckForDbConsistency( &signDbHdrFlushFromDb, &sigRBSHdrFlushFromDb, &signDbHdrFlushFromRBS, &signRBSHdrFlushFromRBS ) == fFalse );

    UtilMemCpy( &sigRBSHdrFlushFromDb, &sigRBSHdrFlush, sizeof( SIGNATURE ) );
    CHECK( FRBSCheckForDbConsistency( &signDbHdrFlushFromDb, &sigRBSHdrFlushFromDb, &signDbHdrFlushFromRBS, &signRBSHdrFlushFromRBS ) == fTrue );

    SIGResetSignature( &sigRBSHdrFlushFromDb );
    UtilMemCpy( &signDbHdrFlushFromRBS, &signDbHdrFlush, sizeof( SIGNATURE ) );
    CHECK( FRBSCheckForDbConsistency( &signDbHdrFlushFromDb, &sigRBSHdrFlushFromDb, &signDbHdrFlushFromRBS, &signRBSHdrFlushFromRBS ) == fTrue );
}

//  ================================================================
JETUNITTESTDB( RBSPreImageCompression, Dehydration, dwOpenDatabase )
//  ================================================================
{
    const ULONG cbPage = 32 * 1024;
    CPAGE cpage, cpageT;
    FMP * pfmp = g_rgfmp + IfmpTest();
    LINE line, lineT;
    INT iline = 0;
    DATA data, dataRec;
    BYTE rgbData[5];
    BYTE *pbDehydrationBuffer = NULL, *pbCompressionBuffer = NULL;
    ULONG compressionPerformed;

    cpage.LoadNewTestPage( cbPage, IfmpTest() );
    memset( rgbData, '0', sizeof(rgbData) );
    dataRec.SetPv( rgbData );
    dataRec.SetCb( sizeof(rgbData) );
    cpage.Insert( 0, &dataRec, 1, 0 );

    data.SetPv( cpage.PvBuffer() );
    data.SetCb( cbPage );
    pbDehydrationBuffer = new BYTE[cbPage];
    pbCompressionBuffer = new BYTE[cbPage];

    PKTermCompression();    // Compression is initialized with a smaller page size for db tests
    CHECK( JET_errSuccess == ErrPKInitCompression( cbPage, 1024, cbPage ) );

    RBSICompressPreImage( pfmp->Pinst(), pfmp->Ifmp(), cpage.PgnoThis(), cbPage, data, pbDehydrationBuffer, pbCompressionBuffer, &compressionPerformed );
    CHECK( compressionPerformed == fRBSPreimageDehydrated );
    CHECK( data.Pv() == pbDehydrationBuffer );
    CHECK( data.Cb() < cbPage );

    CHECK( JET_errSuccess == ErrRBSDecompressPreimage( data, cbPage, pbCompressionBuffer, cpage.PgnoThis(), compressionPerformed ) );
    CHECK( data.Pv() == pbCompressionBuffer );
    CHECK( data.Cb() == cbPage );

    cpageT.LoadPage( data.Pv(), data.Cb() );
    CHECK( cpage.Clines() == cpageT.Clines() );
    cpage.GetPtrExternalHeader( &line );
    cpageT.GetPtrExternalHeader( &lineT );
    iline = 0;
    while ( fTrue )
    {
        CHECK( line.cb == lineT.cb );
        CHECK( memcmp( line.pv, lineT.pv, line.cb ) == 0 );
        CHECK( line.fFlags == lineT.fFlags );

        if ( iline >= cpage.Clines() )
            break;

        cpage.GetPtr( iline, &line );
        cpageT.GetPtr( iline, &lineT );
        iline++;
    }

    delete[] pbCompressionBuffer;
    delete[] pbDehydrationBuffer;
}


//  ================================================================
JETUNITTESTDB( RBSPreImageCompression, DehydrationAndXpress, dwOpenDatabase )
//  ================================================================
{
    const ULONG cbPage = 32 * 1024;
    CPAGE cpage, cpageT;
    FMP * pfmp = g_rgfmp + IfmpTest();
    LINE line, lineT;
    INT iline = 0;
    DATA data, dataRec;
    BYTE rgbData[100];
    BYTE *pbDehydrationBuffer = NULL, *pbCompressionBuffer = NULL;
    ULONG compressionPerformed;

    cpage.LoadNewTestPage( cbPage, IfmpTest() );
    memset( rgbData, '0', sizeof(rgbData) );
    dataRec.SetPv( rgbData );
    dataRec.SetCb( sizeof(rgbData) );
    for ( iline=0; iline<100; iline++ )
    {
        cpage.Insert( iline, &dataRec, 1, 0 );
    }

    data.SetPv( cpage.PvBuffer() );
    data.SetCb( cbPage );
    pbDehydrationBuffer = new BYTE[cbPage];
    pbCompressionBuffer = new BYTE[cbPage];
    
    PKTermCompression();    // Compression is initialized with a smaller page size for db tests
    CHECK( JET_errSuccess == ErrPKInitCompression( cbPage, 1024, cbPage ) );

    RBSICompressPreImage( pfmp->Pinst(), pfmp->Ifmp(), cpage.PgnoThis(), cbPage, data, pbDehydrationBuffer, pbCompressionBuffer, &compressionPerformed );
    CHECK( compressionPerformed == (fRBSPreimageDehydrated | fRBSPreimageCompressed) );
    CHECK( data.Pv() == pbCompressionBuffer );
    CHECK( data.Cb() < cbPage );

    CHECK( JET_errSuccess == ErrRBSDecompressPreimage( data, cbPage, pbDehydrationBuffer, cpage.PgnoThis(), compressionPerformed ) );
    CHECK( data.Pv() == pbDehydrationBuffer );
    CHECK( data.Cb() == cbPage );

    cpageT.LoadPage( data.Pv(), data.Cb() );
    CHECK( cpage.Clines() == cpageT.Clines() );
    cpage.GetPtrExternalHeader( &line );
    cpageT.GetPtrExternalHeader( &lineT );
    iline = 0;
    while ( fTrue )
    {
        CHECK( line.cb == lineT.cb );
        CHECK( memcmp( line.pv, lineT.pv, line.cb ) == 0 );
        CHECK( line.fFlags == lineT.fFlags );

        if ( iline >= cpage.Clines() )
            break;

        cpage.GetPtr( iline, &line );
        cpageT.GetPtr( iline, &lineT );
        iline++;
    }

    delete[] pbCompressionBuffer;
    delete[] pbDehydrationBuffer;
}

//  ================================================================
JETUNITTESTDB( RBSPreImageCompression, Xpress, dwOpenDatabase )
//  ================================================================
{
    const ULONG cbPage = 32 * 1024;
    CPAGE cpage;
    FMP * pfmp = g_rgfmp + IfmpTest();
    DATA data, dataRec;
    BYTE rgbData[100];
    BYTE *pbDehydrationBuffer = NULL, *pbCompressionBuffer = NULL;
    ULONG compressionPerformed;

    cpage.LoadNewTestPage( cbPage, IfmpTest() );
    memset( rgbData, '0', sizeof(rgbData) );
    dataRec.SetPv( rgbData );
    dataRec.SetCb( sizeof(rgbData) );
    for ( INT i=0; i<310; i++ )
    {
        cpage.Insert( i, &dataRec, 1, 0 );
    }

    data.SetPv( cpage.PvBuffer() );
    data.SetCb( cbPage );
    pbDehydrationBuffer = new BYTE[cbPage];
    pbCompressionBuffer = new BYTE[cbPage];
    
    PKTermCompression();    // Compression is initialized with a smaller page size for db tests
    CHECK( JET_errSuccess == ErrPKInitCompression( cbPage, 1024, cbPage ) );

    RBSICompressPreImage( pfmp->Pinst(), pfmp->Ifmp(), cpage.PgnoThis(), cbPage, data, pbDehydrationBuffer, pbCompressionBuffer, &compressionPerformed );
    CHECK( compressionPerformed == fRBSPreimageCompressed );
    CHECK( data.Pv() == pbCompressionBuffer );
    CHECK( data.Cb() < cbPage );

    CHECK( JET_errSuccess == ErrRBSDecompressPreimage( data, cbPage, pbDehydrationBuffer, cpage.PgnoThis(), compressionPerformed ) );
    CHECK( data.Pv() == pbDehydrationBuffer );
    CHECK( data.Cb() == cbPage );
    CHECK( memcmp( cpage.PvBuffer(), data.Pv(), cbPage ) == 0 );

    delete[] pbCompressionBuffer;
    delete[] pbDehydrationBuffer;
}

#endif // ENABLE_JET_UNIT_TEST

ERR CRevertSnapshot::ErrCapturePreimage(
        DBID dbid,
        PGNO pgno,
        _In_reads_( cbImage ) const BYTE *pbImage,
        ULONG cbImage,
        RBS_POS *prbsposRecord,
        ULONG fFlags )
{
    Assert( m_fInitialized );
    Assert( !m_fInvalid );
    Assert( fFlags == 0 || fFlags == fRBSPreimageRevertAlways );

    ERR err = JET_errSuccess;
    BYTE *pbDataDehydrated = NULL, *pbDataCompressed = NULL;
    IFMP ifmp = ifmpNil;

    RBSDbPageRecord dbRec;
    ULONG fFlagsCompression;
    DATA dataRec;
    dataRec.SetPv( (VOID *)pbImage );
    dataRec.SetCb( cbImage );
    // This image may not be checksummed (if dirty), should we checksum it here before writing to snapshot?

    Alloc( pbDataDehydrated = PbPKAllocCompressionBuffer() );
    Alloc( pbDataCompressed = PbPKAllocCompressionBuffer() );

    if ( !m_fPatching )
    {
        ifmp = m_pinst->m_mpdbidifmp[ dbid ];
    }

    RBSICompressPreImage( m_pinst, ifmp, pgno, g_cbPage, dataRec, pbDataDehydrated, pbDataCompressed, &fFlagsCompression );

    dbRec.m_bRecType = rbsrectypeDbPage;
    dbRec.m_usRecLength = sizeof( RBSDbPageRecord ) + dataRec.Cb();
    dbRec.m_dbid = dbid;
    dbRec.m_pgno = pgno;
    dbRec.m_fFlags = fFlags | fFlagsCompression;

    err = ErrCaptureRec( &dbRec, &dataRec, prbsposRecord );

HandleError:
    PKFreeCompressionBuffer( pbDataDehydrated );
    PKFreeCompressionBuffer( pbDataCompressed );

    return err;
}

ERR CRevertSnapshot::ErrCaptureNewPage(
        DBID dbid,
        PGNO pgno,
        RBS_POS *prbsposRecord )
{
    Assert( m_fInitialized );
    Assert( !m_fInvalid );

    DATA dataDummy;
    dataDummy.Nullify();

    RBSDbNewPageRecord dbRec;
    dbRec.m_bRecType = rbsrectypeDbNewPage;
    dbRec.m_usRecLength = sizeof( RBSDbNewPageRecord );
    dbRec.m_dbid = dbid;
    dbRec.m_pgno = pgno;

    return ErrCaptureRec( &dbRec, &dataDummy, prbsposRecord );
}

ERR CRevertSnapshot::ErrCaptureDbAttach( WCHAR* wszDatabaseName, const DBID dbid )
{
    RBS_POS dummy;

    DATA dataRec;
    dataRec.SetPv( wszDatabaseName );
    dataRec.SetCb( ( LOSStrLengthW( wszDatabaseName ) + 1 ) * sizeof(WCHAR) );

    RBSDbAttachRecord dbRec;
    dbRec.m_bRecType = rbsrectypeDbAttach;
    dbRec.m_dbid = dbid;
    dbRec.m_usRecLength = sizeof( RBSDbAttachRecord ) + dataRec.Cb();

    return ErrCaptureRec( &dbRec, &dataRec, &dummy );
}

ERR CRevertSnapshot::ErrCaptureEmptyPages(
    DBID dbid,
    PGNO pgnoFirst,
    CPG  cpg )
{
    Assert( m_fInitialized );
    Assert( !m_fInvalid );
    Assert( pgnoFirst > 0 );
    Assert( dbid != dbidTemp );
    Assert( cpg > 0 );

    RBS_POS rbspos;
    DATA dataDummy;
    dataDummy.Nullify();

    RBSDbEmptyPagesRecord dbRec;
    dbRec.m_bRecType    = rbsrectypeDbEmptyPages;
    dbRec.m_usRecLength = sizeof( RBSDbEmptyPagesRecord );
    dbRec.m_dbid        = dbid;
    dbRec.m_pgnoFirst   = pgnoFirst;
    dbRec.m_cpg         = cpg;

    return ErrCaptureRec( &dbRec, &dataDummy, &rbspos );
}

ERR CRevertSnapshot::ErrQueueCurrentAndAllocBuffer()
{
    ERR err;

    Assert( m_critBufferLock.FOwner() );

    if ( m_pActiveBuffer != NULL && m_pActiveBuffer->m_pBuffer != NULL )
    {
        Assert( m_pActiveBuffer->m_ibNextRecord >= cbRBSBufferSize );

        m_pActiveBuffer->m_cbValidData = cbRBSBufferSize;
        if ( m_pBuffersToWrite == NULL )
        {
            m_pBuffersToWrite = m_pBuffersToWriteLast = m_pActiveBuffer;
        }
        else
        {
            m_pBuffersToWriteLast->m_pNextBuffer = m_pActiveBuffer;
            m_pBuffersToWriteLast = m_pActiveBuffer;
        }
        Assert( m_pActiveBuffer->m_pNextBuffer == NULL );
        m_cNextActiveSegment += CsegRBSCountSegmentOfOffset( m_pActiveBuffer->m_cbValidData );
        m_pActiveBuffer = NULL;
        if ( !m_fWriteInProgress )
        {
            m_fWriteInProgress = fTrue;
            if ( m_pinst->Taskmgr().ErrTMPost( WriteBuffers_, this ) < JET_errSuccess )
            {
                m_fWriteInProgress = fFalse;
            }
        }

        if ( m_cNextActiveSegment > cRBSSegmentMax )
        {
            return ErrERRCheck( JET_errOutOfRBSSpace );
        }
    }
    if ( m_pActiveBuffer == NULL )
    {
        AllocR( m_pActiveBuffer = new (&m_cresRBSBuf) CSnapshotBuffer( m_cNextActiveSegment, &m_cresRBSBuf ) );
    }
    Assert( m_pActiveBuffer->m_pBuffer == NULL );
    return m_pActiveBuffer->ErrAllocBuffer();
}

ERR CRevertSnapshot::ErrCaptureRec(
        const RBSRecord * pRec,
        const DATA      * pExtraData,
              RBS_POS   * prbsposRecord )
{
    ERR err;

    // Pre-allocate reserve buffer in case we need it below.
    CSnapshotBuffer::PreAllocReserveBuffer();

    ENTERCRITICALSECTION critBuf( &m_critBufferLock );

    Assert( FInitialized() );

    USHORT cbRec = CbRBSRecFixed( pRec->m_bRecType );
    Assert( pRec->m_usRecLength == cbRec + pExtraData->Cb() );
    ULONG cbRemaining = cbRec + pExtraData->Cb();
    BOOL fFirstPass = fTrue;

    do
    {
        // If this buffer is full, queue it to be written and get a fresh one
        if ( m_pActiveBuffer == NULL ||
             m_pActiveBuffer->m_pBuffer == NULL ||
             m_pActiveBuffer->m_ibNextRecord >= cbRBSBufferSize )
        {
            Call( ErrQueueCurrentAndAllocBuffer() );
        }

        Assert( m_pActiveBuffer->m_cStartSegment == m_cNextActiveSegment );

        ULONG cbSegmentSpaceRemaining = cbRBSSegmentSize - IbRBSSegmentOffsetFromFullOffset( m_pActiveBuffer->m_ibNextRecord );
        Assert( cbSegmentSpaceRemaining >= sizeof( RBSFragBegin ) );
        Assert( cbSegmentSpaceRemaining <= cbRBSSegmentSize - sizeof( RBSSEGHDR ) );

        // On first pass, either copy full record or start fragmenting
        if ( fFirstPass )
        {
            if ( cbSegmentSpaceRemaining >= cbRemaining )
            {
                // Enough space to copy full record
                UtilMemCpy( m_pActiveBuffer->m_pBuffer + m_pActiveBuffer->m_ibNextRecord, pRec, cbRec );
                UtilMemCpy( m_pActiveBuffer->m_pBuffer + m_pActiveBuffer->m_ibNextRecord + cbRec, pExtraData->Pv(), pExtraData->Cb() );
                m_pActiveBuffer->m_ibNextRecord += cbRemaining;
                cbSegmentSpaceRemaining -= cbRemaining;
                cbRemaining = 0;
            }
            else
            {
                // Copy first fragment with fragment begin header
                RBSFragBegin fragBeginRec;
                fragBeginRec.m_bRecType = rbsrectypeFragBegin;
                fragBeginRec.m_usRecLength = (USHORT)cbSegmentSpaceRemaining;
                fragBeginRec.m_usTotalRecLength = (USHORT)cbRemaining;

                UtilMemCpy( m_pActiveBuffer->m_pBuffer + m_pActiveBuffer->m_ibNextRecord, &fragBeginRec, sizeof( RBSFragBegin ) );
                m_pActiveBuffer->m_ibNextRecord += sizeof( RBSFragBegin );
                cbSegmentSpaceRemaining -= sizeof( RBSFragBegin );
                if ( cbSegmentSpaceRemaining > 0 )
                {
                    ULONG cbToCopy = min( cbRec, cbSegmentSpaceRemaining );
                    UtilMemCpy( m_pActiveBuffer->m_pBuffer + m_pActiveBuffer->m_ibNextRecord, pRec, cbToCopy );
                    m_pActiveBuffer->m_ibNextRecord += cbToCopy;
                    cbSegmentSpaceRemaining -= cbToCopy;
                    cbRemaining -= cbToCopy;
                }
                if ( cbSegmentSpaceRemaining > 0 )
                {
                    ULONG cbToCopy = min( (ULONG)pExtraData->Cb(), cbSegmentSpaceRemaining );
                    UtilMemCpy( m_pActiveBuffer->m_pBuffer + m_pActiveBuffer->m_ibNextRecord, pExtraData->Pv(), cbToCopy );
                    m_pActiveBuffer->m_ibNextRecord += cbToCopy;
                    cbSegmentSpaceRemaining -= cbToCopy;
                    cbRemaining -= cbToCopy;
                }
                Assert( cbSegmentSpaceRemaining == 0 );
                Assert( cbRemaining > 0 );
            }
            fFirstPass = fFalse;
        }
        else
        {
            // Continue copying remaining fragment with the fragment continue header
            Assert( cbSegmentSpaceRemaining == cbRBSSegmentSize - sizeof ( RBSSEGHDR ) );

            RBSFragContinue fragContdRec;
            fragContdRec.m_bRecType = rbsrectypeFragContinue;
            fragContdRec.m_usRecLength = min( sizeof( RBSFragContinue ) + cbRemaining, cbSegmentSpaceRemaining );

            UtilMemCpy( m_pActiveBuffer->m_pBuffer + m_pActiveBuffer->m_ibNextRecord, &fragContdRec, sizeof( RBSFragContinue ) );
            m_pActiveBuffer->m_ibNextRecord += sizeof( RBSFragContinue );
            cbSegmentSpaceRemaining -= sizeof( RBSFragContinue );

            // If any of the original record remains uncopied, copy it now
            if ( cbRemaining > (ULONG)pExtraData->Cb() )
            {
                UtilMemCpy( m_pActiveBuffer->m_pBuffer + m_pActiveBuffer->m_ibNextRecord, (BYTE *)pRec + cbRec - ( cbRemaining - pExtraData->Cb() ), cbRemaining - pExtraData->Cb() );
                m_pActiveBuffer->m_ibNextRecord += cbRemaining - pExtraData->Cb();
                cbSegmentSpaceRemaining -= cbRemaining - pExtraData->Cb();
                cbRemaining = pExtraData->Cb();
            }
            // Now copy the extra data
            ULONG cbToCopy = min( cbRemaining, cbSegmentSpaceRemaining );
            UtilMemCpy( m_pActiveBuffer->m_pBuffer + m_pActiveBuffer->m_ibNextRecord, (BYTE *)pExtraData->Pv() + pExtraData->Cb() - cbRemaining, cbToCopy );
            m_pActiveBuffer->m_ibNextRecord += cbToCopy;
            cbSegmentSpaceRemaining -= cbToCopy;
            cbRemaining -= cbToCopy;
        }

        // If all data is copied, return which segment the last piece of data resides int
        if ( cbRemaining == 0 )
        {
            prbsposRecord->lGeneration = m_prbsfilehdrCurrent->rbsfilehdr.le_lGeneration;
            prbsposRecord->iSegment = m_pActiveBuffer->m_cStartSegment + CsegRBSCountSegmentOfOffset( m_pActiveBuffer->m_ibNextRecord + cbSegmentSpaceRemaining ) - 1;
        }

        // If segment is full, checksum it.
        // TODO: SOMEONE - Earlier checksumming allows better detection of in-memory corruption, but doing it under buffer lock is probably not nice.
        if ( cbSegmentSpaceRemaining < sizeof( RBSFragBegin ) )
        {
            // first fill unused section with 0s
            if ( cbSegmentSpaceRemaining > 0 )
            {
                memset( m_pActiveBuffer->m_pBuffer + m_pActiveBuffer->m_ibNextRecord, 0, cbSegmentSpaceRemaining );
            }
            RBSSEGHDR *pHdr = (RBSSEGHDR *)( m_pActiveBuffer->m_pBuffer + m_pActiveBuffer->m_ibNextRecord + cbSegmentSpaceRemaining - cbRBSSegmentSize );
            pHdr->le_iSegment = m_pActiveBuffer->m_cStartSegment + CsegRBSCountSegmentOfOffset((ULONG)( (BYTE *)pHdr - m_pActiveBuffer->m_pBuffer ));
            LGIGetDateTime( &pHdr->logtimeSegment );
            SetPageChecksum( pHdr, cbRBSSegmentSize, rbsPage, pHdr->le_iSegment );

            m_pActiveBuffer->m_ibNextRecord += cbSegmentSpaceRemaining + sizeof( RBSSEGHDR );
        }
    }
    while( cbRemaining > 0 );

    return JET_errSuccess;

HandleError:
    delete m_pActiveBuffer;
    m_pActiveBuffer = NULL;
    return err;
}

ERR CRevertSnapshot::ErrSetReadBuffer( ULONG iStartSegment )
{
    Assert( m_fInitialized );
    Assert( !m_fInvalid );

    // Free the existing read buffer
    if ( m_pReadBuffer != NULL )
    {
        delete m_pReadBuffer;
    }
    
    ERR err = JET_errSuccess;
    AllocR( m_pReadBuffer = new CSnapshotReadBuffer( iStartSegment ) );
    return err;
}

VOID CRevertSnapshot::LogCorruptionEvent( PCWSTR wszReason, __int64 checksumExpected, __int64 checksumActual )
{
    QWORD ibOffset = IbRBSFileOffsetOfSegment( m_pReadBuffer->m_cStartSegment ) + m_pReadBuffer->m_ibNextRecord;
    ULONG cbSegment = m_pReadBuffer->m_cStartSegment + CsegRBSCountSegmentOfOffset( m_pReadBuffer->m_ibNextRecord );

    WCHAR wszOffsetCurrent[64], wszSegmentCurrent[16], wszChecksumExpected[64], wszChecksumActual[64];
    OSStrCbFormatW( wszOffsetCurrent, sizeof(wszOffsetCurrent),  L"%I64u (0x%I64x)", ibOffset, ibOffset );
    OSStrCbFormatW( wszSegmentCurrent, sizeof(wszSegmentCurrent), L"%u", cbSegment );
    OSStrCbFormatW( wszChecksumExpected, sizeof(wszChecksumExpected), L"%I64u (0x%I64x)", checksumExpected, checksumExpected );
    OSStrCbFormatW( wszChecksumActual, sizeof(wszChecksumActual), L"%I64u (0x%I64x)", checksumActual, checksumActual );

    PCWSTR rgcwsz[6];
    rgcwsz[0] = m_wszRBSCurrentFile;
    rgcwsz[1] = wszReason;
    rgcwsz[2] = wszOffsetCurrent;
    rgcwsz[3] = wszSegmentCurrent;
    rgcwsz[4] = wszChecksumExpected;
    rgcwsz[5] = wszChecksumActual;

    UtilReportEvent(
            eventError,
            GENERAL_CATEGORY,
            RBS_CORRUPT_ID,
            6,
            rgcwsz,
            0,
            NULL,
            m_pinst );

    if ( m_fDumping )
    {
        DUMPPrintF( "!Corruption! - reason: %ws, Offset: %ws, Segment: %ws, Expected checksum: %ws, Actual checksum: %ws\n",
                wszReason, wszOffsetCurrent, wszSegmentCurrent, wszChecksumExpected, wszChecksumActual );
    }
}

ERR CRevertSnapshot::ErrGetNextRecord( RBSRecord **ppRecord, RBS_POS* rbsposRecStart, _Out_ PWSTR wszErrReason )
{
    Assert( m_fInitialized );
    Assert( !m_fInvalid );

    ERR err = JET_errSuccess;
    PCWSTR wszReason = L"Unknown";
    PAGECHECKSUM checksumExpected, checksumActual;
    
    // For dumping purposes we will allow to start dumping records from any particular page.
    // If the first record on the page we are starting from has a FragContinue record, we want to keep moving on till we find the first valid record.
    BOOL fValidRecFound = fFalse;

    *ppRecord = NULL;
    rbsposRecStart->lGeneration = m_prbsfilehdrCurrent == NULL ? 0 : m_prbsfilehdrCurrent->rbsfilehdr.le_lGeneration;

    if ( m_pReadBuffer == NULL )
    {
        Alloc( m_pReadBuffer = new CSnapshotReadBuffer( 0 ) );
    }
    if ( m_pReadBuffer->m_pBuffer == NULL )
    {
        Call( m_pReadBuffer->ErrAllocBuffer() );
    }

    ULONG cbTotal =0, cbRemaining = 0;

    if ( IbRBSSegmentOffsetFromFullOffset( m_pReadBuffer->m_ibNextRecord ) != 0 &&
         *( m_pReadBuffer->m_pBuffer + m_pReadBuffer->m_ibNextRecord ) == rbsrectypeNOP )
    {
        m_pReadBuffer->m_ibNextRecord += cbRBSSegmentSize - IbRBSSegmentOffsetFromFullOffset( m_pReadBuffer->m_ibNextRecord );
    }

    do
    {
        if ( m_pReadBuffer->m_cStartSegment == 0 || m_pReadBuffer->m_ibNextRecord >= m_pReadBuffer->m_cbValidData )
        {
            m_pReadBuffer->m_cStartSegment = ( m_pReadBuffer->m_cStartSegment == 0 ) ? 1 : ( m_pReadBuffer->m_cStartSegment + CsegRBSCountSegmentOfOffset( m_pReadBuffer->m_cbValidData ) );
            m_pReadBuffer->m_ibNextRecord = 0;
            QWORD ibOffset = IbRBSFileOffsetOfSegment( m_pReadBuffer->m_cStartSegment );
            if ( ibOffset >= m_prbsfilehdrCurrent->rbsfilehdr.le_cbLogicalFileSize )
            {
                err = ErrERRCheck( JET_wrnNoMoreRecords );
                goto HandleError;
            }
            DWORD cbToRead = (DWORD)min( cbRBSBufferSize, m_prbsfilehdrCurrent->rbsfilehdr.le_cbLogicalFileSize - ibOffset );
            TraceContextScope tcScope( iorpRBS );
            Call( m_pfapiRBS->ErrIORead( *tcScope, ibOffset, cbToRead, m_pReadBuffer->m_pBuffer, QosSyncDefault( m_pinst ) ) );
            m_pReadBuffer->m_cbValidData = cbToRead;
        }

        if ( IbRBSSegmentOffsetFromFullOffset( m_pReadBuffer->m_ibNextRecord ) == 0 )
        {
            BOOL fCorrectableError;
            INT ibitCorrupted;
            ChecksumAndPossiblyFixPage(
                    m_pReadBuffer->m_pBuffer + m_pReadBuffer->m_ibNextRecord,
                    cbRBSSegmentSize,
                    rbsPage,
                    m_pReadBuffer->m_cStartSegment + CsegRBSCountSegmentOfOffset( m_pReadBuffer->m_ibNextRecord ),
                    fTrue,
                    &checksumExpected,
                    &checksumActual,
                    &fCorrectableError,
                    &ibitCorrupted );
            if ( checksumExpected != checksumActual )
            {
                wszReason = L"BadChecksum";
                err = ErrERRCheck( JET_errRBSFileCorrupt );
                goto HandleError;
            }
            RBSSEGHDR *pHdr = (RBSSEGHDR *)( m_pReadBuffer->m_pBuffer + m_pReadBuffer->m_ibNextRecord );
            if ( pHdr->le_iSegment != m_pReadBuffer->m_cStartSegment + CsegRBSCountSegmentOfOffset( m_pReadBuffer->m_ibNextRecord ) )
            {
                wszReason = L"BadSegmentNumber";
                err = ErrERRCheck( JET_errRBSFileCorrupt );
                goto HandleError;
            }

            if ( m_fDumping )
            {
                DUMPPrintF( "Segment:%u Checksum: 0x%016I64x, logtime ",
                        (SHORT)pHdr->le_iSegment,
                        (XECHECKSUM)pHdr->checksum );
                DUMPPrintLogTime( &pHdr->logtimeSegment );
                DUMPPrintF("\n");
            }

            m_pReadBuffer->m_ibNextRecord += sizeof( RBSSEGHDR );
        }

        RBSRecord *pRecord = (RBSRecord *)( m_pReadBuffer->m_pBuffer + m_pReadBuffer->m_ibNextRecord );
        if ( pRecord->m_usRecLength + IbRBSSegmentOffsetFromFullOffset( m_pReadBuffer->m_ibNextRecord ) > cbRBSSegmentSize )
        {
            wszReason = L"RecordTooLong";
            Assert( pRecord->m_usRecLength + IbRBSSegmentOffsetFromFullOffset( m_pReadBuffer->m_ibNextRecord ) <= cbRBSSegmentSize );
            err = ErrERRCheck( JET_errRBSFileCorrupt );
            goto HandleError;
        }
        switch ( pRecord->m_bRecType )
        {
            case rbsrectypeFragBegin:
                RBSFragBegin *pRecBegin;
                pRecBegin = (RBSFragBegin *)pRecord;
                cbRemaining = cbTotal = pRecBegin->m_usTotalRecLength;
                if ( m_pReadBuffer->m_cbAssembledRec < cbTotal )
                {
                    BYTE *pvRealloc;
                    Alloc( pvRealloc = (BYTE *)realloc( m_pReadBuffer->m_pvAssembledRec, cbTotal ) );
                    m_pReadBuffer->m_pvAssembledRec = pvRealloc;
                    m_pReadBuffer->m_cbAssembledRec = cbTotal;
                }
                Assert( cbTotal <= m_pReadBuffer->m_cbAssembledRec );
                if ( cbRemaining <= pRecord->m_usRecLength - sizeof( RBSFragBegin ) )
                {
                    wszReason = L"FragBeginTooLong";
                    Assert( cbRemaining > pRecord->m_usRecLength - sizeof( RBSFragBegin ) );
                    err = ErrERRCheck( JET_errRBSFileCorrupt );
                    goto HandleError;
                }
                UtilMemCpy( m_pReadBuffer->m_pvAssembledRec, pRecBegin + 1, pRecBegin->m_usRecLength - sizeof( RBSFragBegin ) );
                cbRemaining -= pRecBegin->m_usRecLength - sizeof( RBSFragBegin );
                fValidRecFound = fTrue;
                rbsposRecStart->iSegment = CsegRBSCountSegmentOfOffset( ( m_pReadBuffer->m_ibNextRecord + sizeof( RBSFragBegin ) ) ) + m_pReadBuffer->m_cStartSegment;

                Assert( cbRemaining != 0 );
                break;

            case rbsrectypeFragContinue:
                if ( fValidRecFound )
                {
                    RBSFragContinue *pRecContinue;
                    pRecContinue = (RBSFragContinue *)pRecord;
                    Assert( cbTotal <= m_pReadBuffer->m_cbAssembledRec );
                    if ( cbRemaining < pRecord->m_usRecLength - sizeof( RBSFragContinue ) )
                    {
                        wszReason = L"FragContinueTooLong";
                        Assert( cbRemaining >= pRecord->m_usRecLength - sizeof( RBSFragContinue ) );
                        err = ErrERRCheck( JET_errRBSFileCorrupt );
                        goto HandleError;
                    }
                    UtilMemCpy( m_pReadBuffer->m_pvAssembledRec + cbTotal - cbRemaining, pRecContinue + 1, pRecContinue->m_usRecLength - sizeof( RBSFragContinue ) );
                    cbRemaining -= pRecContinue->m_usRecLength - sizeof( RBSFragContinue );
                    if ( cbRemaining == 0 )
                    {
                        *ppRecord = (RBSRecord *)m_pReadBuffer->m_pvAssembledRec;
                    }
                }
                break;

            default:
                *ppRecord = pRecord;
                fValidRecFound = true;
                cbRemaining = 0;
                rbsposRecStart->iSegment = CsegRBSCountSegmentOfOffset( m_pReadBuffer->m_ibNextRecord ) + m_pReadBuffer->m_cStartSegment;
                break;
        }

        m_pReadBuffer->m_ibNextRecord += pRecord->m_usRecLength;
    }
    while ( cbRemaining > 0 || !fValidRecFound );

    if ( (*ppRecord)->m_bRecType >= rbsrectypeMax )
    {
        Assert( (*ppRecord)->m_bRecType == rbsrectypeDbAttach || (*ppRecord)->m_bRecType == rbsrectypeDbHdr || (*ppRecord)->m_bRecType == rbsrectypeDbAttach );
        wszReason = L"UnknownRecType";
        err = ErrERRCheck( JET_errRBSFileCorrupt );
    }

HandleError:
    if ( err == JET_errRBSFileCorrupt )
    {
        LogCorruptionEvent( wszReason, checksumExpected.rgChecksum[0], checksumActual.rgChecksum[0] );

        CallS( ErrOSStrCbCopyW( wszErrReason, cbOSFSAPI_MAX_PATHW, wszReason ) );

        Assert( fFalse );
    }
    return err;
}

// static
DWORD CRevertSnapshot::WriteBuffers_( VOID *pvThis )
{
    CRevertSnapshot *pSnapshot = (CRevertSnapshot *)pvThis;
    (VOID)pSnapshot->ErrWriteBuffers();
    return 0;
}

ERR CRevertSnapshot::ErrWriteBuffers()
{
    ERR err = JET_errSuccess;
    CSnapshotBuffer *pBuffer, *pBufferToDelete = NULL;
    BOOL fLogSpaceUsage = fFalse;

    Assert( FInitialized() );
    Assert( !FInvalid() );

    {
    ENTERCRITICALSECTION critWrite( &m_critWriteLock );
    while ( fTrue )
    {
        // ErrCaptureRec can only add and not remove from the list, so only need to take buffer lock if list is empty
        pBuffer = m_pBuffersToWrite;
        if ( pBuffer == NULL )
        {
            ENTERCRITICALSECTION critBuf( &m_critBufferLock );
            pBuffer = m_pBuffersToWrite;
            if ( pBuffer == NULL )
            {
                m_fWriteInProgress = fFalse;
                break;
            }
        }

        Assert( pBuffer == m_pBuffersToWrite );
        Assert( pBuffer->m_cStartSegment == m_cNextWriteSegment );
        QWORD ibOffset = IbRBSFileOffsetOfSegment( pBuffer->m_cStartSegment );
        QWORD ibOffsetEnd = ibOffset + pBuffer->m_cbValidData;
        QWORD cbFileSize;
        Call( m_pfapiRBS->ErrSize( &cbFileSize, IFileAPI::filesizeLogical ) );
        if ( ibOffsetEnd > cbFileSize )
        {
            TraceContextScope tcHeader( iorpRBS, iorsHeader );
            ULONG cbExtensionSize = (ULONG) max( UlParam( m_pinst, JET_paramDbExtensionSize ), cbRBSBufferSize );
            Call( m_pfapiRBS->ErrSetSize( *tcHeader, cbFileSize + cbExtensionSize, fFalse, qosIONormal ) );
        }
        TraceContextScope tcScope( iorpRBS );
        Call( m_pfapiRBS->ErrIOWrite( *tcScope, ibOffset, pBuffer->m_cbValidData, pBuffer->m_pBuffer, QosSyncDefault( m_pinst ) ) );

        {
        ENTERCRITICALSECTION critBuf( &m_critBufferLock );
        Assert( pBuffer == m_pBuffersToWrite );
        m_pBuffersToWrite = pBuffer->m_pNextBuffer;
        m_cNextWriteSegment += CsegRBSCountSegmentOfOffset( pBuffer->m_cbValidData );
        OSTrace( JET_tracetagRBS, OSFormat( "RBS write position:%u,%u\n", (LONG) m_prbsfilehdrCurrent->rbsfilehdr.le_lGeneration, m_cNextWriteSegment ) );
        pBufferToDelete = pBuffer;

        if ( m_pBuffersToWrite == NULL )
        {
            m_pBuffersToWriteLast = NULL;
            m_fWriteInProgress = fFalse;
            break;
        }
        }

        delete pBufferToDelete;
        pBufferToDelete = NULL;
    }

    delete pBufferToDelete;
    pBufferToDelete = NULL;

    // TODO: SOMEONE: make flush interval configurable?
    if ( m_cNextFlushSegment - m_cNextWriteSegment >= ( cbRBSSegmentsInBuffer ) * 2 )
    {
        Call( ErrFlush() );
        fLogSpaceUsage = fTrue;
    }
    }

    if ( fLogSpaceUsage )
    {
        RBSCheckSpaceUsage();
    }

HandleError:
    // There is already a file I/O event, any more specific event here?
    return err;
}

ERR CRevertSnapshot::ErrFlush()
{
    ERR err = JET_errSuccess;

    Assert( FInitialized() );
    Assert( m_critWriteLock.FOwner() );

    ULONG cNextWriteSegment = m_cNextWriteSegment;
    if ( m_cNextFlushSegment < cNextWriteSegment )
    {
        // First flush out update to the snapshot before flushing out the header update to point to it
        Call( ErrUtilFlushFileBuffers( m_pfapiRBS, iofrRBS ) );

        m_prbsfilehdrCurrent->rbsfilehdr.le_cbLogicalFileSize = IbRBSFileOffsetOfSegment( cNextWriteSegment );
        
        Call( ErrUtilWriteRBSHeaders( m_pinst, m_pinst->m_pfsapi, NULL, m_prbsfilehdrCurrent, m_pfapiRBS ) );

        m_cNextFlushSegment = cNextWriteSegment;
        m_tickLastFlush = TickOSTimeCurrent();
        OSTrace( JET_tracetagRBS, OSFormat("RBS flush position:%u,%u\n", (LONG)m_prbsfilehdrCurrent->rbsfilehdr.le_lGeneration, m_cNextFlushSegment ) );
    }

HandleError:
    return err;
}

ERR CRevertSnapshot::ErrFlushAll()
{
    ERR err;
    Assert( m_fInitialized );
    Assert( !m_fInvalid );

    if ( m_pActiveBuffer != NULL )
    {
        ENTERCRITICALSECTION critBuf( &m_critBufferLock );

        if ( m_pActiveBuffer != NULL )
        {
            ULONG ibOffset = IbRBSSegmentOffsetFromFullOffset( m_pActiveBuffer->m_ibNextRecord );
            Assert( ibOffset >= sizeof( RBSSEGHDR ) );
            if ( ibOffset == sizeof( RBSSEGHDR ) )
            {
                // No partial sector
                m_pActiveBuffer->m_cbValidData = m_pActiveBuffer->m_ibNextRecord - ibOffset;
            }
            else
            {
                // checksum partial sector (first fill unused section with 0s)
                RBSSEGHDR *pHdr = (RBSSEGHDR *)( m_pActiveBuffer->m_pBuffer + m_pActiveBuffer->m_ibNextRecord - ibOffset );
                memset( (BYTE *)pHdr + ibOffset, 0, cbRBSSegmentSize - ibOffset );
                pHdr->le_iSegment = m_pActiveBuffer->m_cStartSegment + CsegRBSCountSegmentOfOffset((ULONG)( (BYTE *)pHdr - m_pActiveBuffer->m_pBuffer ));
                LGIGetDateTime( &pHdr->logtimeSegment );
                SetPageChecksum( pHdr, cbRBSSegmentSize, rbsPage, pHdr->le_iSegment );

                m_pActiveBuffer->m_cbValidData = m_pActiveBuffer->m_ibNextRecord - ibOffset + cbRBSSegmentSize;
            }

            if ( m_pActiveBuffer->m_cbValidData > 0 )
            {
                // Move filled segments to write buffer
                if ( m_pBuffersToWrite == NULL )
                {
                    m_pBuffersToWrite = m_pBuffersToWriteLast = m_pActiveBuffer;
                }
                else
                {
                    m_pBuffersToWriteLast->m_pNextBuffer = m_pActiveBuffer;
                    m_pBuffersToWriteLast = m_pActiveBuffer;
                }
                Assert( m_pActiveBuffer->m_pNextBuffer == NULL );
                m_cNextActiveSegment += CsegRBSCountSegmentOfOffset( m_pActiveBuffer->m_cbValidData );
                m_pActiveBuffer = NULL;
            }
        }
    }

    Call( ErrWriteBuffers() );

    {
    ENTERCRITICALSECTION critWrite( &m_critWriteLock );
    Call( ErrFlush() );
    }

    RBSCheckSpaceUsage();

HandleError:
    return err;
}

//  ================================================================
//                  CRevertSnapshotForAttachedDbs
//  ================================================================

CRevertSnapshotForAttachedDbs::CRevertSnapshotForAttachedDbs( _In_ INST* const pinst ) :
    CRevertSnapshot( pinst ),
    m_cbFileSizeSpaceUsageLastRun( 0 ),
    m_lGenSpaceUsageLastRun( 0 ),
    m_ftSpaceUsageLastLogged( 0 )
{
}

DBTIME CRevertSnapshotForAttachedDbs::GetDbtimeForFmp( FMP *pfmp )
{
    Assert( FInitialized() );
    Assert( !FInvalid() );

    RBSATTACHINFO *pAttachInfo = NULL;
    ERR err = ErrRBSFindAttachInfoForDBName( m_prbsfilehdrCurrent, pfmp->WszDatabaseName(), &pAttachInfo );
    CallS( err );
    Assert( pAttachInfo != NULL );
    return pAttachInfo->DbtimeDirtied();
}

ERR CRevertSnapshotForAttachedDbs::ErrSetDbtimeForFmp( FMP *pfmp, DBTIME dbtime )
{
    Assert( FInitialized() );
    Assert( !FInvalid() );

    ENTERCRITICALSECTION critWrite( &m_critWriteLock );
    RBSATTACHINFO *pAttachInfo = NULL;
    ERR err = ErrRBSFindAttachInfoForDBName( m_prbsfilehdrCurrent, pfmp->WszDatabaseName(), &pAttachInfo );
    CallS( err );
    Assert( pAttachInfo != NULL );
    Assert( pAttachInfo->DbtimeDirtied() == 0 );
    pAttachInfo->SetDbtimeDirtied( dbtime );
    return ErrUtilWriteRBSHeaders( m_pinst, m_pinst->m_pfsapi, NULL, m_prbsfilehdrCurrent, m_pfapiRBS );
}

ERR CRevertSnapshotForAttachedDbs::ErrRBSInvalidateFmps()
{
    Assert( m_pinst );

    // Reset RBS on all the FMPs before we mark RBS as invalid.
    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; ++dbid )
    {
        IFMP        ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP* pfmp = &g_rgfmp[ ifmp ];
        pfmp->ResetRBSOn();
        pfmp->ResetNeedUpdateDbtimeBeginRBS();
    }

    return ErrRBSInvalidate();
}

BOOL CRevertSnapshotForAttachedDbs::FRBSRaiseFailureItemIfNeeded()
{
    _int64 ftCurrent = UtilGetCurrentFileTime();

    // If either RBS file size is greater than what is allowed and enough time has passed to warrant a roll or if the RBS has reached its max allowed time, 
    // raise a failure item so that HA can take appropriate action to do a roll of the RBS.
    // Note: This is temporary solution to roll RBS till we have live roll available.
    if ( ( UtilConvertFileTimeToSeconds( ftCurrent - ConvertLogTimeToFileTime( &m_prbsfilehdrCurrent->rbsfilehdr.tmCreate ) ) > (long long) UlParam( m_pinst, JET_paramFlight_RBSForceRollIntervalSec ) ||
        m_prbsfilehdrCurrent->rbsfilehdr.le_cbLogicalFileSize > cbMaxRBSSizeAllowed ) && 
        FRollSnapshot() )
    {
        return fTrue;
    }

    return fFalse;
}

VOID CRevertSnapshotForAttachedDbs::RBSCheckSpaceUsage()
{
    Assert( m_pinst );
    Assert( m_pinst->m_plog );
    Assert( FInitialized() );

    if ( FPatching() )
    {
        return;
    }

    WCHAR   wszTimeCreate[32], wszDateCreate[32], wszTimePrevRun[32], wszDatePrevRun[32], wszSizeGrown[32], wszNumLogs[16];
    BOOL fRBSRaiseFailureItem = fFalse;

    {
        ENTERCRITICALSECTION critWrite( &m_critWriteLock );
        _int64 ftCurrent    = UtilGetCurrentFileTime();
        LONG   lGenCurrent  = m_pinst->m_plog->LGGetCurrentFileGenNoLock();

        // Initialize if not yet initialized.
        if ( m_ftSpaceUsageLastLogged == 0 )
        {
            m_ftSpaceUsageLastLogged        = ftCurrent;
            m_lGenSpaceUsageLastRun         = lGenCurrent;
            m_cbFileSizeSpaceUsageLastRun   = m_prbsfilehdrCurrent->rbsfilehdr.le_cbLogicalFileSize;
            return;
        }

        // Enough time has passed. Log the space usage stats.
        if ( UtilConvertFileTimeToSeconds( ftCurrent - m_ftSpaceUsageLastLogged ) > csecSpaceUsagePeriodicLog )
        {
            fRBSRaiseFailureItem = FRBSRaiseFailureItemIfNeeded();

            QWORD cbSpaceGrowth = m_prbsfilehdrCurrent->rbsfilehdr.le_cbLogicalFileSize - m_cbFileSizeSpaceUsageLastRun;
            LONG  cLogsGrowth   = lGenCurrent - m_lGenSpaceUsageLastRun;
            __int64 ftCreate    = ConvertLogTimeToFileTime( &m_prbsfilehdrCurrent->rbsfilehdr.tmCreate );

            size_t  cchRequired;

            ErrUtilFormatFileTimeAsTimeWithSeconds( ftCreate, wszTimeCreate, _countof( wszTimeCreate ), &cchRequired );
            ErrUtilFormatFileTimeAsDate( ftCreate, wszDateCreate, _countof( wszDateCreate ), &cchRequired );

            ErrUtilFormatFileTimeAsTimeWithSeconds( m_ftSpaceUsageLastLogged, wszTimePrevRun, _countof( wszTimePrevRun ), &cchRequired );
            ErrUtilFormatFileTimeAsDate( m_ftSpaceUsageLastLogged, wszDatePrevRun, _countof( wszDatePrevRun ), &cchRequired );

            OSStrCbFormatW( wszSizeGrown, sizeof( wszSizeGrown ), L"%I64u", cbSpaceGrowth ),
                OSStrCbFormatW( wszNumLogs, sizeof( wszNumLogs ), L"%d", cLogsGrowth );

            m_cbFileSizeSpaceUsageLastRun   = m_prbsfilehdrCurrent->rbsfilehdr.le_cbLogicalFileSize;
            m_lGenSpaceUsageLastRun         = lGenCurrent;
            m_ftSpaceUsageLastLogged        = ftCurrent;
        }
        else
        {
            return;
        }
    }

    if ( fRBSRaiseFailureItem )
    {
        OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRBSRollRequired, L"13aa7a33-59d7-4f1e-bea5-1020fd5a9819" );
    }

    const WCHAR* rgcwsz[] =
    {
        m_wszRBSCurrentFile,
        OSFormatW( L"%ws %ws", wszTimeCreate, wszDateCreate ),
        wszSizeGrown,
        OSFormatW( L"%ws %ws", wszTimePrevRun, wszDatePrevRun ),
        wszNumLogs
    };

    UtilReportEvent(
        eventInformation,
        GENERAL_CATEGORY,
        RBS_SPACE_GROWTH_ID,
        5,
        rgcwsz,
        0,
        NULL,
        m_pinst );
}

ERR CRevertSnapshotForAttachedDbs::ErrRBSRecordDbAttach( _In_ FMP* const pfmp )
{
    ERR err                     = JET_errSuccess;

    //OSTrace( JET_tracetagRBS, OSFormat( "\tErrRBSRecordDbAttach(DBName - %ls, RBSEnabled - %d, m_fInitialized - %d, m_fInvalid - %d)\n", pfmp->WszDatabaseName(), BoolParam( m_pinst, JET_paramEnableRBS ), m_fInitialized, m_fInvalid ) );

    // Skip capturing page preimages for temp db.
    if ( pfmp->Dbid() == dbidTemp )
    {
        return JET_errSuccess;
    }

    // If database doesn't support the revert snapshot format version, the revert snapshot shouldn't have been initialized to begin with.
    // TODO SOMEONE: Is it possible for a newly created db to not support even though existing databases support?
    if ( pfmp->ErrDBFormatFeatureEnabled( JET_efvRevertSnapshot ) < JET_errSuccess )
    {
        Assert( !FInitialized() );
        return JET_errSuccess;
    }

    // Snapshot has been marked as invalid. We will not allow any attach.
    if ( FInvalid() )
    {
        return JET_errSuccess;
    }

    RBSATTACHINFO* prbsattachinfo   = NULL;

    err = ErrRBSFindAttachInfoForDBName( m_prbsfilehdrCurrent, pfmp->WszDatabaseName(), &prbsattachinfo );

    Assert( prbsattachinfo );

    {
        ENTERCRITICALSECTION critWrite( &m_critWriteLock );

        {
            PdbfilehdrReadOnly pdbfilehdr    = pfmp->Pdbfilehdr();

            // FMP should have been initialized from Rstmap unless this is a newly create db in which case we will attach it to snapshot as part of this call.
            if ( err == JET_errSuccess )
            {
                // If it were inconsistent or if there was signature mismatch, should have been caught and snapshot rolled in ErrRBSInitDBFromRstmap
                Assert( memcmp( &pdbfilehdr->signDb, &prbsattachinfo->signDb, sizeof( SIGNATURE ) ) == 0 );
                Assert( FRBSCheckForDbConsistency( &pdbfilehdr->signDbHdrFlush, &pdbfilehdr->signRBSHdrFlush, &prbsattachinfo->signDbHdrFlush,  &m_prbsfilehdrCurrent->rbsfilehdr.signRBSHdrFlush ) );
            }
            else
            {
                if( pdbfilehdr->Dbstate() != JET_dbstateJustCreated && pdbfilehdr->Dbstate() != JET_dbstateCleanShutdown )
                {
                    Call( ErrRBSInvalidateFmps() );
                    goto HandleError;
                }

                if ( (BYTE *)(prbsattachinfo + 1) > (m_prbsfilehdrCurrent->rgbAttach + sizeof( m_prbsfilehdrCurrent->rgbAttach )) )
                {
                    Error( ErrERRCheck( JET_errBufferTooSmall ) );
                }
                Assert( prbsattachinfo->FPresent() == 0 );

                //OSTrace( JET_tracetagRBS, OSFormat( "\tErrRBSRecordDbAttach copied logs(DBName - %ls, Min - %ld, Max - %ld)\n", pfmp->WszDatabaseName(), (LONG) pdbfilehdr->le_lGenMinRequired, (LONG) pdbfilehdr->le_lGenMaxRequired ) );

                Call( ErrRBSInitAttachInfo( prbsattachinfo, pfmp->WszDatabaseName(), pdbfilehdr->le_lGenMinRequired, pdbfilehdr->le_lGenMaxRequired, 0, pdbfilehdr->signDb, pdbfilehdr->signDbHdrFlush ) );
                Call( ErrUtilWriteRBSHeaders( m_pinst, m_pinst->m_pfsapi, NULL, m_prbsfilehdrCurrent, m_pfapiRBS ) );
            }
        }
    }

    pfmp->SetRBSOn();

    Call( ErrCaptureDbAttach( pfmp->WszDatabaseName(), pfmp->Dbid() ) );
    Call( ErrCaptureDbHeader( pfmp ) );
    Call( ErrFlushAll() );

HandleError:
    //OSTrace( JET_tracetagRBS, OSFormat( "\tErrRBSRecordDbAttach Err %d, DBName - %ls\n", err, pfmp->WszDatabaseName() ) );
    return err;
}

ERR CRevertSnapshotForAttachedDbs::ErrRBSInitDBFromRstmap( _In_ const RSTMAP* const prstmap, LONG lgenLow, LONG lgenHigh )
{
    Assert( prstmap );
    Assert( FInitialized() );
    Assert( !FInvalid() );

    if ( m_prbsfilehdrCurrent->rbsfilehdr.bLogsCopied || m_prbsfilehdrCurrent->rbsfilehdr.le_lGenMinLogCopied > 0 )
    {
        lgenLow = m_prbsfilehdrCurrent->rbsfilehdr.le_lGenMinLogCopied;
        lgenHigh = m_prbsfilehdrCurrent->rbsfilehdr.le_lGenMaxLogCopied;
    }

    ERR err                     = JET_errSuccess;

    //OSTrace( JET_tracetagRBS, OSFormat( "\tErrRBSInitDBFromRstmap(DBName - %ls, RBSEnabled - %d )\n", prstmap->wszNewDatabaseName, BoolParam( m_pinst, JET_paramEnableRBS ) ) );

    RBSATTACHINFO* prbsattachinfo   = NULL;

    err = ErrRBSFindAttachInfoForDBName( m_prbsfilehdrCurrent, prstmap->wszNewDatabaseName, &prbsattachinfo );

    if ( err == JET_errSuccess )
    {
        Assert( prbsattachinfo->LGenMaxRequired() >= prbsattachinfo->LGenMinRequired() );
        Assert( prbsattachinfo->LGenMaxRequired() <= lgenHigh || prbsattachinfo->LGenMaxRequired() == 0 );
        Assert( prbsattachinfo->LGenMinRequired() >= lgenLow  || prbsattachinfo->LGenMaxRequired() == 0 );

        if ( memcmp( &prstmap->signDatabase, &prbsattachinfo->signDb, sizeof( SIGNATURE ) ) != 0 ||
            !FRBSCheckForDbConsistency( &prstmap->signDatabaseHdrFlush, &prstmap->signRBSHdrFlush, &prbsattachinfo->signDbHdrFlush,  &m_prbsfilehdrCurrent->rbsfilehdr.signRBSHdrFlush ) )
        {
            Error( ErrERRCheck( JET_errRBSDbMismatch ) );
        }

        //OSTrace( JET_tracetagRBS, OSFormat( "\tErrRBSInitDBFromRstmap Found attach info for %ls\n", prstmap->wszNewDatabaseName ) );
    }
    else
    {
        Assert( prstmap->lGenMaxRequired >= prstmap->lGenMinRequired );
        Assert( prstmap->lGenMinRequired >= lgenLow );
        Assert( prstmap->lGenMaxRequired <= lgenHigh );

        if ( (BYTE *)(prbsattachinfo + 1) > ( m_prbsfilehdrCurrent->rgbAttach + sizeof( m_prbsfilehdrCurrent->rgbAttach ) ) )
        {
            Error( ErrERRCheck( JET_errBufferTooSmall ) );
        }
        Assert( prbsattachinfo->FPresent() == 0 );

        //OSTrace( JET_tracetagRBS, OSFormat( "\tErrRBSInitDBFromRstmap copied logs(DBName - %ls, Min - %ld, Max - %ld)\n", prstmap->wszNewDatabaseName, (LONG) prstmap->lGenMinRequired, (LONG) prstmap->lGenMaxRequired ) );

        Call( ErrRBSInitAttachInfo( prbsattachinfo, prstmap->wszNewDatabaseName, prstmap->lGenMinRequired, prstmap->lGenMaxRequired, 0, prstmap->signDatabase, prstmap->signDatabaseHdrFlush ) );
        Call( ErrUtilWriteRBSHeaders( m_pinst, m_pinst->m_pfsapi, NULL, m_prbsfilehdrCurrent, m_pfapiRBS ) );
    }

HandleError:
    //OSTrace( JET_tracetagRBS, OSFormat( "\tErrRBSInitDBFromRstmap Err %d, DBName - %ls\n", err, prstmap->wszNewDatabaseName ) );
    return err;
}

ERR CRevertSnapshotForAttachedDbs::ErrCaptureDbHeader( FMP * const pfmp )
{
    RBS_POS dummy;
    RBSDbHdrRecord dbRec;
    dbRec.m_bRecType = rbsrectypeDbHdr;
    dbRec.m_usRecLength = sizeof( RBSDbHdrRecord ) + sizeof( DBFILEHDR );
    dbRec.m_dbid = pfmp->Dbid();

    DATA dataRec;
    dataRec.SetPv( (VOID *)pfmp->Pdbfilehdr().get() );
    dataRec.SetCb( sizeof( DBFILEHDR ) );

    return ErrCaptureRec( &dbRec, &dataRec, &dummy );
}

ERR CRevertSnapshotForAttachedDbs::ErrRBSInitFromRstmap( INST* const pinst )
{
    Assert( pinst );
    Assert( pinst->m_plog );
    Assert( !pinst->m_prbs ); // revert snapshot shouldn't have been initialized

    ERR err = JET_errSuccess;
    LONG lgenLow = 0;
    LONG lgenHigh = 0;
    BOOL fRBSCreateIfRequired = fFalse;
    CRevertSnapshotForAttachedDbs* prbs = NULL;

    if ( pinst->m_plog->FLogDisabled() ||
        !BoolParam( pinst, JET_paramEnableRBS ) || 
        !pinst->m_plog->FRBSFeatureEnabledFromRstmap() )
    {
        // We will skip setting up the revert snapshot if either it is not enabled, not supported or if the required range is too wide.
        // Enable/disable is only allowed during db attach/create
        return JET_errSuccess;
    }

    pinst->m_plog->LoadRBSGenerationFromRstmap( &lgenLow, &lgenHigh );

    Assert( lgenLow >= 0 );
    Assert( lgenHigh >= 0 );
    Assert( lgenHigh - lgenLow >= 0 );

    fRBSCreateIfRequired = (lgenHigh - lgenLow) <= ( (LONG) UlParam( pinst, JET_paramFlight_RBSMaxRequiredRange ) );

    Alloc( prbs = new CRevertSnapshotForAttachedDbs( pinst ) );
    Call( prbs->ErrRBSInit( fRBSCreateIfRequired, errRBSRequiredRangeTooLarge ) );

    RSTMAP          *rgrstmap   = pinst->m_plog->Rgrstmap();
    INT             irstmapMac  = pinst->m_plog->IrstmapMac();

    for ( INT irstmap = 0; irstmap < irstmapMac; irstmap++ )
    {
        const RSTMAP * const prstmap = rgrstmap + irstmap;

        if ( prstmap->fFileNotFound )
        {
            continue;
        }

        err = prbs->ErrRBSInitDBFromRstmap( prstmap, lgenLow, lgenHigh );

        // If some DB's attach info doesn't match with db info from rst map, we will roll snapshot if allowed.
        if ( err == JET_errRBSDbMismatch )
        {
            // all prev RBS generations are invalid.
            if ( pinst->m_prbscleaner && prbs->FInitialized() )
            {
                LONG lRBSCurrentGen = prbs->RBSFileHdr()->rbsfilehdr.le_lGeneration;
                pinst->m_prbscleaner->SetFirstValidGen( lRBSCurrentGen + 1 );
            }

            if ( fRBSCreateIfRequired )
            {
                Call( prbs->ErrRollSnapshot( fFalse, fTrue ) );
            }
            else
            {
                RBSLogCreateSkippedEvent( pinst, SzParam( pinst, JET_paramRBSFilePath ), errRBSRequiredRangeTooLarge, err );
                Error( ErrERRCheck( errRBSRequiredRangeTooLarge ) );
            }
        }
        else
        {
            Call( err );
        }
    }

    Call( prbs->ErrRBSCopyRequiredLogs( fTrue ) );
    pinst->m_prbs = prbs;
    return JET_errSuccess;

HandleError:
    if ( prbs != NULL )
    {
        delete prbs;
        prbs = NULL;
    }

    // No current usable snapshot and we don't want to create a new one at this point. So just return success.
    if ( err == errRBSRequiredRangeTooLarge )
    {
        err = JET_errSuccess;
    }

    return err;
}

ERR CRevertSnapshotForAttachedDbs::ErrRBSSetRequiredLogs( BOOL fInferFromRstmap )
{
    // Already set so return.
    if ( !FInitialized() || 
        m_prbsfilehdrCurrent->rbsfilehdr.bLogsCopied || 
        m_prbsfilehdrCurrent->rbsfilehdr.le_lGenMinLogCopied > 0 )
    {
        return JET_errSuccess;
    }

    LONG lgenLow = 0;
    LONG lgenHigh = 0;

    if ( fInferFromRstmap )
    {
        m_pinst->m_plog->LoadRBSGenerationFromRstmap( &lgenLow, &lgenHigh );
    }
    else
    {
        RBSLoadRequiredGenerationFromFMP( m_pinst, &lgenLow, &lgenHigh );
    }

    m_prbsfilehdrCurrent->rbsfilehdr.le_lGenMinLogCopied = lgenLow;
    m_prbsfilehdrCurrent->rbsfilehdr.le_lGenMaxLogCopied = lgenHigh;
    return ErrUtilWriteRBSHeaders( m_pinst, m_pinst->m_pfsapi, NULL, m_prbsfilehdrCurrent, m_pfapiRBS );
}

ERR CRevertSnapshotForAttachedDbs::ErrRBSCopyRequiredLogs( BOOL fInferFromRstmap )
{
    if ( !FInitialized() || 
        m_prbsfilehdrCurrent->rbsfilehdr.bLogsCopied )
    {
        return JET_errSuccess;
    }

    JET_ERR err = JET_errSuccess;

    // If we already collected range of logs to copy but just didn't finish copying them, skip to copying
    if ( m_prbsfilehdrCurrent->rbsfilehdr.le_lGenMinLogCopied )
    {
        goto CopyLogs;
    }

    Call( ErrRBSSetRequiredLogs( fInferFromRstmap ) );

CopyLogs:
    // This should really be asynchronous (especially when we start rolling snapshots)
    Call( ErrRBSCopyRequiredLogs_( 
        m_pinst,
        m_prbsfilehdrCurrent->rbsfilehdr.le_lGenMinLogCopied, 
        m_prbsfilehdrCurrent->rbsfilehdr.le_lGenMaxLogCopied, 
        SzParam( m_pinst, JET_paramLogFilePath ), 
        m_wszRBSCurrentLogDir,
        fTrue,
        fTrue ) );

    m_prbsfilehdrCurrent->rbsfilehdr.bLogsCopied = 1;
    Call( ErrUtilWriteRBSHeaders( m_pinst, m_pinst->m_pfsapi, NULL, m_prbsfilehdrCurrent, m_pfapiRBS ) );

HandleError:
    return err;
}

ERR CRevertSnapshotForAttachedDbs::ErrRollSnapshot( BOOL fPrevRBSValid, BOOL fInferFromRstmap )
{
    Assert( m_prbsfilehdrCurrent );

    WCHAR   wszRBSAbsFilePath[ IFileSystemAPI::cchPathMax ];
    WCHAR   wszRBSAbsLogDirPath[ IFileSystemAPI::cchPathMax ];
    JET_ERR err = JET_errSuccess;

    // Use the current header to carry over some information.
    RBSFILEHDR* prbsfilehdrPrev     = m_prbsfilehdrCurrent;
    LOGTIME defaultLogTime          = { 0 };
    SIGNATURE defaultSign           = { 0 };
    LOGTIME logtimePrevGen          = fPrevRBSValid ? prbsfilehdrPrev->rbsfilehdr.tmCreate : defaultLogTime;
    SIGNATURE signPrevRBSHdrFlush   = fPrevRBSValid ? prbsfilehdrPrev->rbsfilehdr.signRBSHdrFlush : defaultSign;
    LONG    lRBSGen                 = prbsfilehdrPrev->rbsfilehdr.le_lGeneration + 1;

    // TODO: SOMEONE - if rolling snapshot when live, need to atomically update each FMPs DbtimeBeginSnapshot and
    // update state so ErrCaptureRec starts collecting records for the next snapshot.

    if ( fPrevRBSValid )
    {
        Call( ErrFlushAll() );
    }

    Alloc( m_prbsfilehdrCurrent = (RBSFILEHDR *)PvOSMemoryPageAlloc( sizeof(RBSFILEHDR), NULL ) );

    // Go through all dbs corresponding to this instance and add their corresponding rbs attach infos.
    Call( ErrRBSRollAttachInfos( m_pinst, m_prbsfilehdrCurrent, prbsfilehdrPrev, fPrevRBSValid, fInferFromRstmap ) );

    //OSTrace( JET_tracetagRBS, OSFormat( "\tErrRollSnapshot(NewGen - %ld, PrevValid - %d)\n", lRBSGen, fPrevRBSValid ) );

    // Free previous instances.
    delete m_pfapiRBS;
    m_pfapiRBS = NULL;
    FreeCurrentFilePath();
    FreeCurrentLogDirPath();
    OSMemoryPageFree( prbsfilehdrPrev );
    prbsfilehdrPrev = NULL;

    // TODO SOMEONE: Copy any required data from the previous snapshot.
    Call( ErrRBSCreateOrLoadRbsGen( 
        lRBSGen,
        logtimePrevGen,
        signPrevRBSHdrFlush,
        wszRBSAbsFilePath, 
        wszRBSAbsLogDirPath ) );

    const SIZE_T    cchRBSName       = sizeof( WCHAR ) * ( LOSStrLengthW( wszRBSAbsFilePath ) + 1 );
    Alloc( m_wszRBSCurrentFile = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cchRBSName ) ) );

    const SIZE_T    cchRBSLogDirName       = sizeof( WCHAR ) * ( LOSStrLengthW( wszRBSAbsLogDirPath ) + 1 );
    Alloc( m_wszRBSCurrentLogDir = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cchRBSLogDirName ) ) );

    Call( ErrOSStrCbCopyW( m_wszRBSCurrentFile, cchRBSName, wszRBSAbsFilePath ) );
    Call( ErrOSStrCbCopyW( m_wszRBSCurrentLogDir, cchRBSLogDirName, wszRBSAbsLogDirPath ) );

    Call( ErrRBSCopyRequiredLogs( fInferFromRstmap ) );

    // Reset space usage last logged when we roll a snapshot. 
    // We will start timer when we do the first flush.
    m_ftSpaceUsageLastLogged = 0;

HandleError:
    return err;
}

BOOL CRevertSnapshotForAttachedDbs::FRollSnapshot()
{
    if ( !FInitialized() || FInvalid() || m_prbsfilehdrCurrent == NULL )
    {
        return fFalse;
    }

    const __int64 cSecSinceFileCreate = UtilConvertFileTimeToSeconds( ConvertLogTimeToFileTime( &( m_prbsfilehdrCurrent->rbsfilehdr.tmCreate ) ) );
    const __int64 cSecCurrentTime = UtilConvertFileTimeToSeconds( UtilGetCurrentFileTime( ) );

    //OSTrace( JET_tracetagRBS, OSFormat( "\tFRollSnapshot(cSecCurrentTime - %lld, cSecSinceFileCreate - %lld, JET_paramFlight_RBSRollIntervalSec - %ld)\n", cSecCurrentTime, cSecSinceFileCreate, (ULONG) UlParam( JET_paramFlight_RBSRollIntervalSec ) ) );

    if( cSecCurrentTime - cSecSinceFileCreate > ( (ULONG) UlParam( m_pinst, JET_paramFlight_RBSRollIntervalSec ) ) )
    {
        return fTrue;
    }

    return fFalse;
}

//  ================================================================
//                  CRevertSnapshotForPatch
//  ================================================================

CRevertSnapshotForPatch::CRevertSnapshotForPatch( _In_ INST* const pinst ) : CRevertSnapshot( pinst )
{
    m_dbidPatchingMaxInUse = 0;
}

// Captures a fake Dbattach record which will be used to map patched pages to their corresponding dbs.
//
ERR CRevertSnapshotForPatch::ErrCaptureFakeDbAttach( CIrsOpContext* const pirs )
{
    Assert( pirs );

    // Snapshot has been marked as invalid. We will not allow any attach.
    if ( FInvalid() )
    {
        return JET_errSuccess;
    }

    ERR err = JET_errSuccess;

    pirs->SetDbidRBS( m_dbidPatchingMaxInUse );
    m_dbidPatchingMaxInUse++;

    Call( ErrCaptureDbAttach( pirs->WszDatabasePath(), pirs->DbidRBS() ) );
    Call( ErrFlushAll() );

HandleError:
    return err;
}

// Invalidates the CIrsOpContexts and the RBS.
//
ERR CRevertSnapshotForPatch::ErrRBSInvalidateIrs( PCWSTR wszReason )
{
    Assert( m_pinst );

    ERR err = JET_errSuccess;

    // Reset RBS on all the IRS contexts before we mark RBS as invalid.
    for ( ULONG ipirs = 0; ipirs < dbidMax; ++ipirs )
    {
        CIrsOpContext* pirs = m_pinst->m_rgpirs[ ipirs ];

        if ( !pirs )
            break;

        pirs->ResetRBSOn();
    }

    Call( ErrRBSInvalidate() );

    PCWSTR rgcwsz[3];
    rgcwsz[0] = m_wszRBSCurrentFile;
    rgcwsz[1] = wszReason;

    UtilReportEvent(
        eventError,
        GENERAL_CATEGORY,
        RBS_INVALIDATED_ID,
        2,
        rgcwsz,
        0,
        NULL,
        m_pinst );

HandleError:
    return err;
}

// Initializes the RBS related info for the given database's CIrsOpContext 
//
ERR CRevertSnapshotForPatch::ErrRBSInitDB( CIrsOpContext* const pirs )
{
    Assert( FInitialized() );
    Assert( !FInvalid() );
    Assert( pirs );
    Assert( pirs->Pdbfilehdr() );

    ERR err                         = JET_errSuccess;
    RBSATTACHINFO* prbsattachinfo   = NULL;
    DBFILEHDR* pdbfilehdr           = pirs->Pdbfilehdr();

    err = ErrRBSFindAttachInfoForDBName( m_prbsfilehdrCurrent, pirs->WszDatabasePath(), &prbsattachinfo );

    if ( err == JET_errSuccess )
    {
        // DB sign doesn't match. 
        if ( memcmp( &pdbfilehdr->signDb, &prbsattachinfo->signDb, sizeof( SIGNATURE ) ) != 0 ||
            !FRBSCheckForDbConsistency( &pdbfilehdr->signDbHdrFlush, &pdbfilehdr->signRBSHdrFlush, &prbsattachinfo->signDbHdrFlush,  &m_prbsfilehdrCurrent->rbsfilehdr.signRBSHdrFlush ) )
        {
            Call( ErrRBSInvalidateIrs( L"DbRBSSignMismatch" ) );
            return JET_errSuccess;
        }
    }
    else if ( err == errRBSAttachInfoNotFound )
    {
        // We couldn't find the attach info for the db being patched in RBS. Invalidate RBS.
        Call( ErrRBSInvalidateIrs( L"IRSDbAttachInfoNotFound" ) );
        return JET_errSuccess;
    }

    Call( err );
    Call( ErrCaptureFakeDbAttach( pirs ) );

    pirs->SetDbtimeBeginRBS( prbsattachinfo->DbtimeDirtied() );
    pirs->SetRBSOn();

HandleError:
    return err;
}

// Initializes RBS structures for patching purposes. Does not create a new RBS if existing one is corrupted or doesn't exist.
//
ERR CRevertSnapshotForPatch::ErrRBSInitForPatch( INST* const pinst )
{
    Assert( pinst );

    // If either RBS is disabled or already initialized skip initialization.
    if ( !BoolParam( pinst, JET_paramEnableRBS ) || ( pinst->m_prbsfp && pinst->m_prbsfp->FInitialized() ) )
    {
        return JET_errSuccess;
    }

    ERR err = JET_errSuccess;
    CRevertSnapshotForPatch* prbsfp = NULL;

    Alloc( prbsfp = new CRevertSnapshotForPatch( pinst ) );

    prbsfp->SetPatching();
    Call( prbsfp->ErrRBSInit( fFalse, errRBSPatching ) );

    Assert( prbsfp->FInitialized() );

    RBSFILEHDR* prbsfilehdr = prbsfp->RBSFileHdr();

    // If we still haven't copied the required logs for the snapshot, don't try to patch the snapshot.
    if ( !prbsfilehdr->rbsfilehdr.bLogsCopied )
    {
        // We will just invalidate instead of not assigning it to pinst->m_prbsfp, so that we can avoid unnecessary retries when the next partition starts to patch.
        Call( prbsfp->ErrRBSInvalidateIrs( L"IRSNoLogsCopiedByRBS" ) );
    }

    pinst->m_prbsfp = prbsfp;

    return JET_errSuccess;

HandleError:
    if ( prbsfp != NULL )
    {
        delete prbsfp;
        prbsfp = NULL;
    }

    // We couldn't load the existing RBS either because it is corrupted or it doesn't exist. Nothing to patch here.
    if ( err == errRBSPatching )
    {
        return JET_errSuccess;
    }

    return err;
}

//  ================================================================
//                  RBSCleaner
//  ================================================================

// Create a cleaner object for automatic cleanup of RBS.
// This doesn't start the cleaner thread.
ERR RBSCleanerFactory::ErrRBSCleanerCreate( INST*  pinst, _Out_ RBSCleaner ** prbscleaner)
{
     ERR err = JET_errSuccess;

    if ( pinst && prbscleaner )
    {
        unique_ptr<RBSCleanerIOOperator> prbscleaneriooperator( new RBSCleanerIOOperator( pinst ) );
        unique_ptr<RBSCleanerState> prbscleanerstate( new RBSCleanerState() );
        unique_ptr<RBSCleanerConfig> prbscleanerconfig( new RBSCleanerConfig( pinst ) );

        Alloc( prbscleaneriooperator.get() );
        Alloc( prbscleanerstate.get() );
        Alloc( prbscleanerconfig.get() );

        Call( prbscleaneriooperator->ErrRBSInitPaths() );

        *prbscleaner = new RBSCleaner( pinst, prbscleaneriooperator.release(), prbscleanerstate.release(), prbscleanerconfig.release() );
        Alloc( *prbscleaner );
    }

    return JET_errSuccess;

HandleError:
    return err;
}

RBSCleanerState::RBSCleanerState() :
    m_ftPrevPassCompletionTime( 0 ),
    m_ftPassStartTime( 0 ),
    m_cPassesFinished( 0 )
{
}

RBSCleanerIOOperator::RBSCleanerIOOperator( INST* pinst )
{
    Assert( pinst );
    m_pinst = pinst;
    m_wszRBSAbsRootDirPath = NULL;
    m_wszRBSBaseName = NULL;
}

RBSCleanerIOOperator::~RBSCleanerIOOperator( )
{
    if ( m_wszRBSAbsRootDirPath )
    {
        OSMemoryHeapFree( m_wszRBSAbsRootDirPath );
        m_wszRBSAbsRootDirPath = NULL;
    }

    if ( m_wszRBSBaseName )
    {
        OSMemoryHeapFree( m_wszRBSBaseName );
        m_wszRBSBaseName = NULL;
    }
}

ERR RBSCleanerIOOperator::ErrRBSInitPaths()
{
    return ErrRBSInitPaths_( m_pinst, &m_wszRBSAbsRootDirPath, &m_wszRBSBaseName );
}

ERR RBSCleanerIOOperator::ErrRBSDiskSpace( QWORD* pcbFreeForUser )
{
    Assert( m_pinst->m_pfsapi );
    return m_pinst->m_pfsapi->ErrDiskSpace( m_wszRBSAbsRootDirPath, pcbFreeForUser );
}

ERR RBSCleanerIOOperator::ErrGetDirSize( PCWSTR wszDirPath, _Out_ QWORD* pcbSize )
{
    Assert( m_pinst->m_pfsapi );
    return ErrRBSGetDirSize( m_pinst->m_pfsapi, m_wszRBSAbsRootDirPath, pcbSize );
}

ERR RBSCleanerIOOperator::ErrRemoveFolder( PCWSTR wszDirPath, PCWSTR wszRBSRemoveReason )
{
    Assert( m_pinst->m_pfsapi );

    ERR err = JET_errSuccess;
    Call( ErrRBSDeleteAllFiles( m_pinst->m_pfsapi, wszDirPath, NULL, fTrue ) );

    PCWSTR rgcwsz[2];
    rgcwsz[0] = wszDirPath;
    rgcwsz[1] = wszRBSRemoveReason;

    UtilReportEvent(
            eventInformation,
            GENERAL_CATEGORY,
            RBSCLEANER_REMOVEDRBS_ID,
            2,
            rgcwsz,
            0,
            NULL,
            m_pinst );
HandleError:
    return err;
}

ERR RBSCleanerIOOperator::ErrRBSAbsRootDirPathToUse(
    __out_bcount ( cbDirPath ) WCHAR* wszRBSAbsRootDirPath,
    LONG cbDirPath,
    BOOL fBackupDir )
{
    Assert( LOSStrLengthW( m_wszRBSAbsRootDirPath ) + 1 < cbDirPath );

    ERR err = JET_errSuccess;

    Call( ErrOSStrCbCopyW( wszRBSAbsRootDirPath, cbDirPath, m_wszRBSAbsRootDirPath ) );

    if ( fBackupDir )
    {
        Assert( LOSStrLengthW( m_wszRBSAbsRootDirPath ) + LOSStrLengthW( wszRBSBackupDir ) + 1 + 1 < cbDirPath );

        Call( ErrOSStrCbAppendW( wszRBSAbsRootDirPath, cbDirPath, wszRBSBackupDir ) );
        Call( m_pinst->m_pfsapi->ErrPathFolderNorm( wszRBSAbsRootDirPath, cbDirPath ) );
    }

HandleError:
    return err;
}

ERR RBSCleanerIOOperator::ErrRBSGetLowestAndHighestGen( LONG* plRBSGenMin, LONG* plRBSGenMax, BOOL fBackupDir )
{
    Assert( m_pinst->m_pfsapi );

    WCHAR wszRBSAbsRootDirPath[ IFileSystemAPI::cchPathMax ];
    ERR err = JET_errSuccess;

    Call( ErrRBSAbsRootDirPathToUse( wszRBSAbsRootDirPath, sizeof( wszRBSAbsRootDirPath ), fBackupDir ) );
    Call( ErrRBSGetLowestAndHighestGen_( m_pinst->m_pfsapi, wszRBSAbsRootDirPath, m_wszRBSBaseName, plRBSGenMin, plRBSGenMax ) );

HandleError:
    return err;
}

ERR RBSCleanerIOOperator::ErrRBSFilePathForGen( __out_bcount ( cbDirPath ) WCHAR* wszRBSDirPath, LONG cbDirPath, __out_bcount ( cbFilePath ) WCHAR* wszRBSFilePath, LONG cbFilePath, LONG lRBSGen, BOOL fBackupDir )
{
    Assert( m_pinst->m_pfsapi );

    WCHAR wszRBSAbsRootDirPath[ IFileSystemAPI::cchPathMax ];
    ERR err = JET_errSuccess;

    Call( ErrRBSAbsRootDirPathToUse( wszRBSAbsRootDirPath, sizeof( wszRBSAbsRootDirPath ), fBackupDir ) );
    Call( ErrRBSFilePathForGen_( wszRBSAbsRootDirPath, m_wszRBSBaseName, m_pinst->m_pfsapi, wszRBSDirPath, cbDirPath, wszRBSFilePath, cbFilePath, lRBSGen ) );

HandleError:
    return err;
}

ERR RBSCleanerIOOperator::ErrRBSFileHeader( PCWSTR wszRBSFilePath, _Out_ RBSFILEHDR* prbsfilehdr )
{
    Assert( wszRBSFilePath );
    Assert( prbsfilehdr );

    ERR             err         = JET_errSuccess;
    IFileAPI        *pfapiRBS   = NULL;
    IFileSystemAPI  *pfsapi  = m_pinst->m_pfsapi;

    Assert( pfsapi );

    Call( CIOFilePerf::ErrFileOpen( pfsapi, m_pinst, wszRBSFilePath, IFileAPI::fmfReadOnly, iofileRBS, qwRBSFileID, &pfapiRBS ) );
    Call( ErrUtilReadShadowedHeader( m_pinst, pfsapi, pfapiRBS, (BYTE*) prbsfilehdr, sizeof( RBSFILEHDR ), -1, urhfNoAutoDetectPageSize | urhfReadOnly | urhfNoEventLogging ) );

HandleError:
    if ( pfapiRBS )
    {
        delete pfapiRBS;
        pfapiRBS = NULL;
    }

    return err;
}

RBSCleaner::RBSCleaner( 
    INST*                           pinst,
    IRBSCleanerIOOperator* const    prbscleaneriooperator,
    IRBSCleanerState* const         prbscleanerstate,
    IRBSCleanerConfig* const        prbscleanerconfig ) : 
    CZeroInit( sizeof( RBSCleaner ) ),
    m_pinst( pinst ),
    m_msigRBSCleanerStop( CSyncBasicInfo( _T("RBSCleaner::m_msigRBSCleanerStop" ) ) ),
    m_critRBSFirstValidGen( CLockBasicInfo( CSyncBasicInfo( szRBSFirstValidGen ), rankRBSFirstValidGen, 0 ) ),
    m_prbscleaneriooperator( prbscleaneriooperator ),
    m_prbscleanerstate( prbscleanerstate ),
    m_prbscleanerconfig( prbscleanerconfig ),
    m_threadRBSCleaner( 0 ),
    m_lFirstValidRBSGen( 1 ),
    m_fValidRBSGenSet( fFalse )
{
    Assert( pinst );
    Assert( prbscleanerconfig );
    Assert( prbscleaneriooperator );
    Assert( prbscleanerstate );
}

RBSCleaner::~RBSCleaner( )
{
    TermCleaner( );
}

DWORD RBSCleaner::DwRBSCleanerThreadProc( DWORD_PTR dwContext )
{
    RBSCleaner * const prbscleaner = reinterpret_cast<RBSCleaner *>( dwContext );
    Assert( NULL != prbscleaner );
    return prbscleaner->DwRBSCleaner();
}

DWORD RBSCleaner::DwRBSCleaner()
{
    WaitForMinPassTime();
    ComputeFirstValidRBSGen();
    while ( !m_msigRBSCleanerStop.FIsSet() )
    {
        ErrDoOneCleanupPass();
        m_prbscleanerstate->CompletedPass();

        // ErrDoOneCleanupPass can ask for the thread to stop. So we will remove the unnecessary wait time and break early.
        if ( m_msigRBSCleanerStop.FIsSet() || FMaxPassesReached() )
        {
            break;
        }

        WaitForMinPassTime();
    }

    return 0;
}

void RBSCleaner::WaitForMinPassTime()
{
    const __int64 ftNow = UtilGetCurrentFileTime();
    const __int64 csecSincePrevScanCompleted = UtilConvertFileTimeToSeconds( ftNow - m_prbscleanerstate->FtPrevPassCompletionTime() );
    const __int64 csecBeforeNextPassCanStart = m_prbscleanerconfig->CSecMinCleanupIntervalTime() - csecSincePrevScanCompleted;

    if ( csecBeforeNextPassCanStart > 0 )
    {
        // negative wait time turns off deadlock detection
        Assert( csecBeforeNextPassCanStart <= ( INT_MAX / 1000 ) ); // overflow?
        ( void )m_msigRBSCleanerStop.FWait( -1000 * ( INT )csecBeforeNextPassCanStart );
    }
}

VOID RBSCleaner::ComputeFirstValidRBSGen()
{
    Assert( m_pinst );

    WCHAR       wszRBSAbsDirPath[ IFileSystemAPI::cchPathMax ];
    WCHAR       wszRBSAbsFilePath[ IFileSystemAPI::cchPathMax ];
    LONG        lRBSGenMax;
    LONG        lRBSGenMin;
    LOGTIME     tmPrevRBSCreate;
    RBSFILEHDR  rbsfilehdr;
    ERR         err = JET_errSuccess;

    // First valid gen has already been computed. 
    if ( m_fValidRBSGenSet )
    {
        return;
    }

    // Lets find the lowest and highest snapshot generation we have.
    Call( m_prbscleaneriooperator->ErrRBSGetLowestAndHighestGen( &lRBSGenMin, &lRBSGenMax, fFalse ) );

    // We will let the main instance thread which creates/loads RBS check if the generation before max is invalid since it computes the info anyways.
    lRBSGenMax = BoolParam( m_pinst, JET_paramEnableRBS ) ? lRBSGenMax - 1 : lRBSGenMax;

    for ( LONG lRBSGen = lRBSGenMax; lRBSGen >= lRBSGenMin && !m_fValidRBSGenSet; --lRBSGen )
    {
        Call( m_prbscleaneriooperator->ErrRBSFilePathForGen( wszRBSAbsDirPath, sizeof( wszRBSAbsDirPath ), wszRBSAbsFilePath, sizeof( wszRBSAbsFilePath ), lRBSGen, fFalse ) );
        Call( m_prbscleaneriooperator->ErrRBSFileHeader( wszRBSAbsFilePath, &rbsfilehdr ) );

        // The RBS create time doesn't match the prev RBS time set on the next snapshot. So, we can't use the snapshot.
        if ( tmPrevRBSCreate.FIsSet() && LOGTIME::CmpLogTime( rbsfilehdr.rbsfilehdr.tmCreate, tmPrevRBSCreate ) != 0 )
        {
            SetFirstValidGen( lRBSGen );
            return;
        }

        memcpy( &tmPrevRBSCreate, &rbsfilehdr.rbsfilehdr.tmPrevGen, sizeof( LOGTIME ) );

        // If prev RBS log time isn't set, it must have been invalid. So set current RBS gen as valid generation.
        if ( !tmPrevRBSCreate.FIsSet() )
        {
            SetFirstValidGen( lRBSGen );
            return;
        }
    }

HandleError:
    return;
}

// Cleans up RBS files which were backed up after a revert for investigations, if conditions satisfy.
//
ERR RBSCleaner::ErrRBSCleanupBackup( QWORD* cbFreeRBSDisk, QWORD* cbTotalRBSDiskSpace )
{
    WCHAR   wszRBSAbsDirPath[ IFileSystemAPI::cchPathMax ];
    WCHAR   wszRBSAbsFilePath[ IFileSystemAPI::cchPathMax ];
    QWORD   cbMaxRBSSpaceLowDiskSpace   = m_prbscleanerconfig->CbMaxSpaceForRBSWhenLowDiskSpace();
    QWORD   cbLowDiskSpace              = m_prbscleanerconfig->CbLowDiskSpaceThreshold();
    BOOL    fRBSCleanupBackup           = fFalse;
    ERR     err                         = JET_errSuccess;

    LONG        lRBSGenMaxBackup;
    LONG        lRBSGenMinBackup;
    RBSFILEHDR  rbsfilehdr;

    __int64 ftCreate                    = 0;
    PCWSTR  wszRBSBackupRemoveReason    = L"Unknown";

    // Lets find the lowest snapshot generation we have.
    Call( m_prbscleaneriooperator->ErrRBSGetLowestAndHighestGen( &lRBSGenMinBackup, &lRBSGenMaxBackup, fTrue ) );

    if ( lRBSGenMinBackup == 0 )
    {
        return JET_errSuccess;
    }

    if ( *cbFreeRBSDisk < cbLowDiskSpace && *cbTotalRBSDiskSpace > cbMaxRBSSpaceLowDiskSpace )
    {
        // Low disk space, lets clean up all the backup snapshots we have for investigation.
        fRBSCleanupBackup = fTrue;
        wszRBSBackupRemoveReason = L"LowDiskSpace";
    }
    else
    {
        Call( m_prbscleaneriooperator->ErrRBSFilePathForGen( wszRBSAbsDirPath, sizeof( wszRBSAbsDirPath ), wszRBSAbsFilePath, sizeof( wszRBSAbsFilePath ), lRBSGenMinBackup, fTrue ) );

        err = m_prbscleaneriooperator->ErrRBSFileHeader( wszRBSAbsFilePath, &rbsfilehdr );

        if ( err == JET_errReadVerifyFailure )
        {
            // If we are unable to read the header, we will clean up all the backup snapshots since one of the snapshot is anyways not usable.
            fRBSCleanupBackup = fTrue;
            wszRBSBackupRemoveReason = L"CorruptHeader";
            err = JET_errSuccess;
        }
        else if ( err == JET_errFileNotFound )
        {
            fRBSCleanupBackup = fTrue;
            wszRBSBackupRemoveReason = L"RBSFileMissing";
            err = JET_errSuccess;
        }
        else if ( err == JET_errSuccess )
        {
            ftCreate = ConvertLogTimeToFileTime( &( rbsfilehdr.rbsfilehdr.tmCreate ) );

            if ( UtilConvertFileTimeToSeconds( UtilGetCurrentFileTime() - ftCreate ) > m_prbscleanerconfig->CSecRBSMaxTimeSpan() )
            {
                fRBSCleanupBackup = fTrue;
                wszRBSBackupRemoveReason = L"Scavenged";
            }
        }
        else
        {
            Call( err );
        }
    }

    // We will clean up all the snapshots we have backed up for investigation since we lose the chain of snapshots we used to revert anyways and it would be incomplete data.
    if ( fRBSCleanupBackup )
    {
        while ( !m_msigRBSCleanerStop.FIsSet() && lRBSGenMinBackup > 0 )
        {
            QWORD cbRBSDiskSpace = 0;

            Call( m_prbscleaneriooperator->ErrRBSFilePathForGen( wszRBSAbsDirPath, sizeof( wszRBSAbsDirPath ), wszRBSAbsFilePath, sizeof( wszRBSAbsFilePath ), lRBSGenMinBackup, fTrue ) );

            Call( m_prbscleaneriooperator->ErrGetDirSize( wszRBSAbsDirPath, &cbRBSDiskSpace ) );
            Call( m_prbscleaneriooperator->ErrRemoveFolder( wszRBSAbsDirPath, wszRBSBackupRemoveReason ) );
            *cbTotalRBSDiskSpace = *cbTotalRBSDiskSpace - cbRBSDiskSpace;
            *cbFreeRBSDisk = *cbFreeRBSDisk + cbRBSDiskSpace;

            Call( m_prbscleaneriooperator->ErrRBSGetLowestAndHighestGen( &lRBSGenMinBackup, &lRBSGenMaxBackup, fTrue ) );
        }
    }

HandleError:
    return err;
}

ERR RBSCleaner::ErrDoOneCleanupPass()
{
    QWORD       cbFreeRBSDisk;
    QWORD       cbTotalRBSDiskSpace;
    ERR         err                         = JET_errSuccess;
    QWORD       cbMaxRBSSpaceLowDiskSpace   = m_prbscleanerconfig->CbMaxSpaceForRBSWhenLowDiskSpace();
    QWORD       cbLowDiskSpace              = m_prbscleanerconfig->CbLowDiskSpaceThreshold();

    LONG        lRBSGenMax;
    LONG        lRBSGenMin;

    m_prbscleanerstate->SetPassStartTime();

    // Find the amount of free user space on the disk hosting the snapshots based on provided snapshot root directory.
    Call( m_prbscleaneriooperator->ErrRBSDiskSpace( &cbFreeRBSDisk ) );

    // Find the amount of space on the disk taken up by the revert snapshots.
    // We will exit this pass if we are unable to get RBS directory size.
    Call( m_prbscleaneriooperator->ErrGetDirSize( m_prbscleaneriooperator->WszRBSAbsRootDirPath(), &cbTotalRBSDiskSpace ) );
    Call( ErrRBSCleanupBackup( &cbFreeRBSDisk, &cbTotalRBSDiskSpace ) );

    while ( !m_msigRBSCleanerStop.FIsSet( ) )
    {
        WCHAR       wszRBSAbsDirPath[ IFileSystemAPI::cchPathMax ];
        WCHAR       wszRBSAbsFilePath[ IFileSystemAPI::cchPathMax ];
        PCWSTR      wszRBSRemoveReason  = L"Unknown";
        BOOL        fRBSCleanupMinGen   = fFalse;
        QWORD       cbRBSDiskSpace      = 0;
        __int64     ftCreate            = 0;
        RBSFILEHDR  rbsfilehdr;

        // Lets find the lowest snapshot generation we have.
        Call( m_prbscleaneriooperator->ErrRBSGetLowestAndHighestGen( &lRBSGenMin, &lRBSGenMax, fFalse ) );
        
        //OSTrace( JET_tracetagRBSCleaner, OSFormat( "\tIsValidGen(Min - %ld, Max - %ld, %d)\n", lRBSGenMin, lRBSGenMax, FGenValid( lRBSGenMin ) ) );

        // RBS max gen is 0 and RBS isn't enabled for this instance. No point in running the thread further.
        if ( lRBSGenMax == 0 && !BoolParam( m_pinst, JET_paramEnableRBS ) )
        {
            m_msigRBSCleanerStop.Set();
            goto HandleError;
        }

        // We will not remove the only snapshot directory we have unless RBS is actually disabled.
        if ( lRBSGenMin != lRBSGenMax || !BoolParam( m_pinst, JET_paramEnableRBS ) )
        {
            fRBSCleanupMinGen = !FGenValid( lRBSGenMin );
            Call( m_prbscleaneriooperator->ErrRBSFilePathForGen( wszRBSAbsDirPath, sizeof( wszRBSAbsDirPath ), wszRBSAbsFilePath, sizeof( wszRBSAbsFilePath ), lRBSGenMin, fFalse ) );

            if ( !fRBSCleanupMinGen )
            {
                err = m_prbscleaneriooperator->ErrRBSFileHeader( wszRBSAbsFilePath, &rbsfilehdr );

                if ( err != JET_errFileNotFound && err != JET_errSuccess && err != JET_errReadVerifyFailure)
                {
                    goto HandleError;
                }

                if( err == JET_errSuccess ||  err == JET_errReadVerifyFailure )
                {
                    // If we are unable to read the header, we will clean up the snapshot since the snapshot is not usable.
                    if ( err == JET_errReadVerifyFailure )
                    {
                        fRBSCleanupMinGen = fTrue;
                        wszRBSRemoveReason = L"CorruptHeader";
                    }
                    else
                    {
                        ftCreate = ConvertLogTimeToFileTime( &(rbsfilehdr.rbsfilehdr.tmCreate) );

                        if ( UtilConvertFileTimeToSeconds( UtilGetCurrentFileTime() - ftCreate ) > m_prbscleanerconfig->CSecRBSMaxTimeSpan() )
                        {
                            fRBSCleanupMinGen = fTrue;
                            wszRBSRemoveReason = L"Scavenged";
                        }
                    }
                }
                else
                {
                    fRBSCleanupMinGen = fTrue;
                    wszRBSRemoveReason = L"RBSFileMissing";
                }
            }
            else
            {
                wszRBSRemoveReason = L"InvalidRBS";
            }

            if ( fRBSCleanupMinGen || ( cbFreeRBSDisk < cbLowDiskSpace && cbTotalRBSDiskSpace > cbMaxRBSSpaceLowDiskSpace ) )
            {
                if ( !fRBSCleanupMinGen )
                {
                    wszRBSRemoveReason = L"LowDiskSpace";
                }

                Call( m_prbscleaneriooperator->ErrGetDirSize( wszRBSAbsDirPath, &cbRBSDiskSpace ) );
                Call( m_prbscleaneriooperator->ErrRemoveFolder( wszRBSAbsDirPath, wszRBSRemoveReason ) );
                cbTotalRBSDiskSpace -= cbRBSDiskSpace;
                cbFreeRBSDisk       += cbRBSDiskSpace;
            }
            else
            {
                // Nothing to delete in this pass. Exit this pass.
                goto HandleError;
            }
        }
        else
        {
            if ( cbFreeRBSDisk < m_prbscleanerconfig->CbLowDiskSpaceDisableRBSThreshold() )
            {
                OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRBSRollRequired, L"928d9bfa-9dd1-4d7f-ad7b-28a9d8e33b2d" );
            }

            break;
        }
    }

HandleError:
    //OSTrace( JET_tracetagRBSCleaner, OSFormat( "\tError %d", err ) );
    
    // We have file not found error and RBS isn't enabled. Let's terminate cleanup for this instance.
    if ( err == JET_errFileNotFound && !BoolParam( m_pinst, JET_paramEnableRBS ) )
    {
        m_msigRBSCleanerStop.Set();
    }

    return err;
}

ERR RBSCleaner::ErrStartCleaner( )
{
    ERR err = JET_errSuccess;

    if ( !m_prbscleanerconfig->FEnableCleanup( ) )
    {
        return err;
    }

    Assert( !FIsCleanerRunning() );
    Call( ErrUtilThreadCreate( DwRBSCleanerThreadProc, 0, priorityNormal, &m_threadRBSCleaner, ( DWORD_PTR )this ) );
    Assert( FIsCleanerRunning() );

HandleError:
    if ( err < JET_errSuccess )
    {
        TermCleaner();
    }

    return err;
}

VOID RBSCleaner::TermCleaner()
{
    m_msigRBSCleanerStop.Set();

    if ( m_threadRBSCleaner != NULL )
    {
        UtilThreadEnd( m_threadRBSCleaner );
    }
    m_threadRBSCleaner = NULL;
  
    Assert( !FIsCleanerRunning() );
}

//  ================================================================
//                  CRBSDatabaseRevertContext
//  ================================================================

CRBSDatabaseRevertContext::CRBSDatabaseRevertContext( _In_ INST* const pinst )
    : CZeroInit( sizeof( CRBSDatabaseRevertContext ) ),
    m_pinst ( pinst ),
    m_dbidCurrent ( dbidMax ),
    m_asigWritePossible( CSyncBasicInfo( _T( "CRBSDatabaseRevertContext::m_asigWritePossible" ) ) )
{
    Assert( pinst );
}

CRBSDatabaseRevertContext::~CRBSDatabaseRevertContext()
{
    if ( m_wszDatabaseName )
    {
        OSMemoryHeapFree( m_wszDatabaseName );
        m_wszDatabaseName = NULL;
    }

    if ( m_pdbfilehdr )
    {
        OSMemoryPageFree( m_pdbfilehdr );
        m_pdbfilehdr = NULL;
    }

    if ( m_pdbfilehdrFromRBS )
    {
        OSMemoryPageFree( m_pdbfilehdrFromRBS );
        m_pdbfilehdrFromRBS = NULL;
    }

    if ( m_pfapiDb )
    {
        delete m_pfapiDb;
        m_pfapiDb = NULL;
    }

    if ( m_psbmDbPages )
    {
        delete m_psbmDbPages;
        m_psbmDbPages = NULL;
    }

    if ( m_psbmCachedDbPages )
    {
        delete m_psbmCachedDbPages;
        m_psbmCachedDbPages = NULL;
    }
    
    if ( m_pfm )
    {
        delete m_pfm;
        m_pfm = NULL;
    }

    if ( m_rgRBSDbPage )
    {
        CPG cpgTotal                            = m_rgRBSDbPage->Size();
        CArray< CPagePointer >::ERR errArray    = CArray< CPagePointer >::ERR::errSuccess;

        for ( int i = 0; i < cpgTotal; ++i )
        {
            if ( m_rgRBSDbPage->Entry( i ).DwPage() )
            {
                // Free the memory here after flushing the page.
                OSMemoryPageFree( (void*) m_rgRBSDbPage->Entry( i ).DwPage() );
            }

            errArray = m_rgRBSDbPage->ErrSetEntry( i, NULL );
            Assert( errArray == CArray< CPagePointer >::ERR::errSuccess );
        }

        delete m_rgRBSDbPage;
        m_rgRBSDbPage = NULL;
    }

    if ( m_pcprintfIRSTrace != NULL &&
        CPRINTFNULL::PcprintfInstance() != m_pcprintfIRSTrace )
    {
        (*m_pcprintfIRSTrace)( "Closing IRS tracing file from revert.\r\n" );
        delete m_pcprintfIRSTrace;
    }

    m_pcprintfIRSTrace = NULL;
}

// Reset the spare bit map of the pages by freeing and reallocating memory.
//
ERR CRBSDatabaseRevertContext::ErrResetSbmPages( IBitmapAPI** ppsbm )
{
    ERR err = JET_errSuccess;
    IBitmapAPI::ERR errBM = IBitmapAPI::ERR::errSuccess;

    if ( *ppsbm )
    {
        delete *ppsbm;
        *ppsbm = NULL;
    }

    // Reserve bitmap space for maximum number of pages the database is allowed to have.
    Alloc( *ppsbm = new CSparseBitmap() );
    errBM = (*ppsbm)->ErrInitBitmap( pgnoSysMax );

    if ( errBM != IBitmapAPI::ERR::errSuccess )
    {
        Assert( errBM == IBitmapAPI::ERR::errOutOfMemory );
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

HandleError:
    return err;
}

// Initialize the database revert context.
//
ERR CRBSDatabaseRevertContext::ErrRBSDBRCInit( RBSATTACHINFO* prbsattachinfo, SIGNATURE* psignRBSHdrFlush, CPG cacheSize )
{
    ERR     err = JET_errSuccess;
    CAutoWSZ wszDatabaseName;
    CallR( wszDatabaseName.ErrSet( prbsattachinfo->wszDatabaseName ) );
    m_wszDatabaseName = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( wszDatabaseName.Cb() + 2 ) );

    Alloc( m_wszDatabaseName );
    Call( ErrOSStrCbCopyW( m_wszDatabaseName, wszDatabaseName.Cb() + 2, wszDatabaseName ) );

    //  open the requested database
    //
    Call( m_pinst->m_pfsapi->ErrFileOpen( 
                m_wszDatabaseName,  
                BoolParam( JET_paramEnableFileCache ) ? IFileAPI::fmfCached : IFileAPI::fmfNone, 
                &m_pfapiDb ) );

    //  get some other resources together
    Alloc( m_pdbfilehdr = (DBFILEHDR*)PvOSMemoryPageAlloc( g_cbPage, NULL ) );

    //  read in the header of this database
    //
    err = ErrUtilReadShadowedHeader(    
            m_pinst,
            m_pinst->m_pfsapi,
            m_pfapiDb,
            (BYTE*)m_pdbfilehdr,
            g_cbPage,
            OffsetOf( DBFILEHDR, le_cbPageSize ) );

    if ( FErrIsDbHeaderCorruption( err ) || JET_errFileIOBeyondEOF == err )
    {
        err = ErrERRCheck( JET_errDatabaseCorrupted );
    }

    Call( err );

    // We will allow for FRBSCheckForDbConsistency to fail if we are already in the process of applying revert snapshot.
    if ( memcmp( &m_pdbfilehdr->signDb, &prbsattachinfo->signDb, sizeof( SIGNATURE ) ) != 0 ||
         !FRBSCheckForDbConsistency( &m_pdbfilehdr->signDbHdrFlush, &m_pdbfilehdr->signRBSHdrFlush, &prbsattachinfo->signDbHdrFlush,  psignRBSHdrFlush ) )
    {
        Error( ErrERRCheck( JET_errRBSRCInvalidRBS ) );
    }

    if ( m_pdbfilehdr->Dbstate() != JET_dbstateRevertInProgress &&
        ErrDBFormatFeatureEnabled_( JET_efvApplyRevertSnapshot, m_pdbfilehdr->Dbv() ) < JET_errSuccess )
    {
        Error( ErrERRCheck( JET_errRBSRCInvalidDbFormatVersion ) );
    }

    Alloc( m_rgRBSDbPage = new CArray< CPagePointer >() );
    m_rgRBSDbPage->ErrSetCapacity( (size_t) cacheSize );

    Call( ErrResetSbmPages( &m_psbmDbPages ) );
    Call( ErrResetSbmPages( &m_psbmCachedDbPages ) ); 

    //  initialize persisted flush map
    //
    Call( CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( m_wszDatabaseName, m_pdbfilehdr, m_pinst, &m_pfm ) );

HandleError:
    return err;
}

// Prepares the database for revert.
//
ERR CRBSDatabaseRevertContext::ErrSetDbstateForRevert( ULONG rbsrchkstate, LOGTIME logtimeRevertTo )
{
    Assert( m_pdbfilehdr );
    Assert( m_wszDatabaseName );
    Assert( m_pfapiDb );

    ERR err = JET_errSuccess;

    if ( rbsrchkstate != JET_revertstateNone && m_pdbfilehdr->Dbstate() != JET_dbstateRevertInProgress )
    {
        return ErrERRCheck( JET_errRBSRCBadDbState );
    }

    //  update the cached header to reflect that we are now in the revert in progress state
    //
    if ( m_pdbfilehdr->Dbstate() != JET_dbstateRevertInProgress )
    {
        m_pdbfilehdr->SetDbstate( JET_dbstateRevertInProgress, lGenerationInvalid, lGenerationInvalid, NULL, fTrue );
        m_pdbfilehdr->le_ulRevertCount++;
        LGIGetDateTime( &m_pdbfilehdr->logtimeRevertFrom );
        m_pdbfilehdr->le_ulRevertPageCount = 0;
    }

    // This time might change if the a revert fails and if the user decide to revert to further in the past.
    memcpy( &m_pdbfilehdr->logtimeRevertTo, &logtimeRevertTo, sizeof( LOGTIME ) );

    //  write the header back to the database
    Call( ErrUtilWriteUnattachedDatabaseHeaders( m_pinst, m_pinst->m_pfsapi, m_wszDatabaseName, m_pdbfilehdr, m_pfapiDb, m_pfm, fFalse ) );
    Call( ErrUtilFlushFileBuffers( m_pfapiDb, iofrRBSRevertUtil ) );

HandleError:
    return err;
}

// Sets database state once revert is done.
//
ERR CRBSDatabaseRevertContext::ErrSetDbstateAfterRevert( SIGNATURE* psignRbsHdrFlush )
{
    Assert( m_pdbfilehdr );
    Assert( m_pdbfilehdrFromRBS );

    m_pdbfilehdrFromRBS->le_ulRevertCount                           = m_pdbfilehdr->le_ulRevertCount;
    m_pdbfilehdrFromRBS->le_ulRevertPageCount                       = m_pdbfilehdr->le_ulRevertPageCount;
    m_pdbfilehdrFromRBS->le_lgposCommitBeforeRevert                 = lgposMax;
    m_pdbfilehdrFromRBS->le_lgposCommitBeforeRevert.le_lGeneration  = m_pdbfilehdr->le_lGenMaxCommitted;

    memcpy( &m_pdbfilehdrFromRBS->logtimeRevertFrom, &m_pdbfilehdr->logtimeRevertFrom, sizeof( LOGTIME ) );
    memcpy( &m_pdbfilehdrFromRBS->logtimeRevertTo, &m_pdbfilehdr->logtimeRevertTo, sizeof( LOGTIME ) );
    memcpy( &m_pdbfilehdrFromRBS->signRBSHdrFlush, psignRbsHdrFlush, sizeof( SIGNATURE ) );

    ERR err = JET_errSuccess;

    if ( m_pfm )
    {
        if ( m_pdbfilehdrFromRBS->Dbstate() == JET_dbstateCleanShutdown )
        {
            Call( m_pfm->ErrCleanFlushMap() );
        }
        else
        {
            // fixup DB header state
            //
            m_pfm->SetDbGenMinRequired( m_pdbfilehdrFromRBS->le_lGenMinRequired );
            m_pfm->SetDbGenMinConsistent( m_pdbfilehdrFromRBS->le_lGenMinConsistent );

            // flush the flush map
            //
            Call( m_pfm->ErrFlushAllSections( OnDebug( fTrue ) ) );
        }
    }

    //  write the header back to the database
    Call( ErrUtilWriteUnattachedDatabaseHeaders( m_pinst, m_pinst->m_pfsapi, m_wszDatabaseName, m_pdbfilehdrFromRBS, m_pfapiDb, m_pfm, fFalse ) );
    Call( ErrUtilFlushFileBuffers( m_pfapiDb, iofrRBSRevertUtil ) );

HandleError:
    return err;
}

// Copies db header from the rbs record to the actual database, but retains values of a few fields
//
ERR CRBSDatabaseRevertContext::ErrRBSCaptureDbHdrFromRBS( RBSDbHdrRecord* prbsdbhdrrec, BOOL* pfGivenDbfilehdrCaptured )
{
    // Already captured the appropriate header from RBS.
    if ( m_pdbfilehdrFromRBS )
    {
        return JET_errSuccess;
    }

    Assert( prbsdbhdrrec->m_usRecLength == sizeof( RBSDbHdrRecord ) + sizeof( DBFILEHDR ) );
    Assert( m_pdbfilehdr );

    ERR err = JET_errSuccess;

    Alloc( m_pdbfilehdrFromRBS = (DBFILEHDR*)PvOSMemoryPageAlloc( g_cbPage, NULL ) );

    // Change some db header states as per current db header.
    DBFILEHDR* pdbfilehdr               = (DBFILEHDR*) prbsdbhdrrec->m_rgbHeader;

    Assert( m_pdbfilehdr->le_dbstate == JET_dbstateRevertInProgress );

    UtilMemCpy( m_pdbfilehdrFromRBS, pdbfilehdr, sizeof( DBFILEHDR ) );

    if ( pfGivenDbfilehdrCaptured )
    {
        *pfGivenDbfilehdrCaptured = fTrue;
    }

HandleError:
    return err;
}

// Whether the page is already capture in the bitmap
//
BOOL CRBSDatabaseRevertContext::FPageAlreadyCaptured( PGNO pgno )
{
    BOOL fPageAlreadyCaptured;

    IBitmapAPI::ERR errbm = m_psbmDbPages->ErrGet( pgno, &fPageAlreadyCaptured );    
    Assert( errbm == IBitmapAPI::ERR::errSuccess );

    return fPageAlreadyCaptured;
}

// Add the given page to our array and mark it in our page bitmap
//
ERR CRBSDatabaseRevertContext::ErrAddPage( void* pvPage, PGNO pgno, BOOL fReplaceCached, BOOL* pfPageAddedToCache )
{
    Assert( pvPage );
    Assert( m_rgRBSDbPage );
    Assert( m_rgRBSDbPage->Size() < m_rgRBSDbPage->Capacity() );
    
    BOOL                        fPageInCache    = fFalse;
    IBitmapAPI::ERR             errbm           = IBitmapAPI::ERR::errSuccess;
    CArray< CPagePointer >::ERR errArray        = CArray< CPagePointer >::ERR::errSuccess;
    size_t ientry                               = m_rgRBSDbPage->Size();
    size_t ientryExisting                       = CArray< CPagePointer >::iEntryNotFound;
    CPagePointer pageptr( (DWORD_PTR) pvPage, pgno );

    *pfPageAddedToCache = fTrue;

    // We want to replace the page in cache in cases where the flag fRBSPreimageRevertAlways is set for dbpage record
    // but we currently have another image of the page in cache.
    // We are replacing in cache to guarantee that the given image is the one which ends up on the disk.
    //
    if ( fReplaceCached )
    {
        errbm = m_psbmCachedDbPages->ErrGet( pgno, &fPageInCache );

        // If page is in cache, find the entry and replace it. 
        // It is also possible page was captured as part of the snapshot but is not in the cache due to a flush.
        // In that case we don't have to worry about replacing page in cache.
        //
        if ( fPageInCache )
        {
            ientryExisting = m_rgRBSDbPage->SearchLinear( &pageptr, CRBSDatabaseRevertContext::ICRBSDatabaseRevertContextPgEquals );
            Assert( ientryExisting != CArray< CPagePointer >::iEntryNotFound );

            CPagePointer ppTempPage( m_rgRBSDbPage->Entry( ientryExisting ) );

            // Free the memory here after getting the existing page from cache and replace it with the given page image.
            OSMemoryPageFree( (void*) ppTempPage.DwPage() );
            errArray = m_rgRBSDbPage->ErrSetEntry( ientryExisting, NULL );

            if ( errArray != CArray< CPagePointer >::ERR::errSuccess )
            {
                Assert( errArray == CArray< CPagePointer >::ERR::errOutOfMemory );
                return ErrERRCheck( JET_errOutOfMemory );
            }

            ientry = ientryExisting;
            *pfPageAddedToCache = fFalse;
        }
    }

    errArray = m_rgRBSDbPage->ErrSetEntry( ientry, pageptr );

    if ( CArray< CPagePointer >::ERR::errSuccess != errArray )
    {
        Assert( CArray< CPagePointer >::ERR::errOutOfMemory == errArray );
        return ErrERRCheck( JET_errOutOfMemory );
    }

    // Set the corresponding bit in the bitmap.
    errbm = m_psbmDbPages->ErrSet( pgno, fTrue );
    Assert( errbm == IBitmapAPI::ERR::errSuccess );

    errbm = m_psbmCachedDbPages->ErrSet( pgno, fTrue );
    Assert( errbm == IBitmapAPI::ERR::errSuccess );

    return JET_errSuccess;
}

// Comparer to allow sorting of pages in our array to try and get sequential writes.
//
INLINE INT __cdecl CRBSDatabaseRevertContext::ICRBSDatabaseRevertContextCmpPgRec( const CPagePointer* ppg1, const CPagePointer* ppg2 )
{
    Assert( ppg1 );
    Assert( ppg2 );

    // Duplicates shouldn't be put in the array as we want to capture only the first page image from the snapshot.
    Assert( ppg1->PgNo() != ppg2->PgNo() );

    return ( ( ppg1->PgNo() < ppg2->PgNo() ) ? -1 : +1 );
}

// Equals method to say if both page are the same or not. We will use a different method than ICRBSDatabaseRevertContextCmpPgRec since we want the assert
// of pgno not equal while sorting.
//
INLINE INT __cdecl CRBSDatabaseRevertContext::ICRBSDatabaseRevertContextPgEquals( const CPagePointer* ppg1, const CPagePointer* ppg2 )
{
    Assert( ppg1 );
    Assert( ppg2 );

    return ( ( ppg1->PgNo() == ppg2->PgNo() ) ? 0 : ( ( ppg1->PgNo() < ppg2->PgNo() ) ? -1 : +1 ) );
}

void CRBSDatabaseRevertContext::OsWriteIoComplete(
    const ERR errIo,
    IFileAPI* const pfapi,
    const FullTraceContext& tc,
    const OSFILEQOS grbitQOS,
    const QWORD ibOffset,
    const DWORD cbData,
    const BYTE* const pbData,
    const DWORD_PTR keyIOComplete )
{
    CRBSDatabaseRevertContext* const prbsdbrc = (CRBSDatabaseRevertContext*) keyIOComplete;
    prbsdbrc->m_asigWritePossible.Set();
    AtomicDecrement( &(prbsdbrc->m_cpgWritePending) );
}

ERR CRBSDatabaseRevertContext::ErrFlushDBPage( void* pvPage, PGNO pgno, USHORT cbDbPageSize, const OSFILEQOS qos )
{
    Assert( pvPage );
    Assert( pgno > 0 );

    ERR err = JET_errSuccess;
    CPageValidationNullAction nullaction;
    TraceContextScope tcRevertPage( iorpRevertPage );

    QWORD cbSize = 0;
    DBTIME dbtimePage;

    CPAGE cpageT;
    cpageT.LoadPage( ifmpNil, pgno, pvPage, cbDbPageSize );

    if ( m_pfm )
    {
        m_pfm->SetPgnoFlushType( pgno, cpageT.Pgft(), cpageT.Dbtime() );
    }

    dbtimePage = cpageT.Dbtime();
    cpageT.UnloadPage();

    // if any part of the page range is beyond EOF, extend the database to accommodate the page
    // it is possible db shrunk and we reverting back to unshrunk state.
    Call( m_pfapiDb->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );

    const QWORD cbNewSize   = OffsetOfPgno( pgno ) + cbDbPageSize;
    if ( cbNewSize > cbSize )
    {
        //  extend the database, zero-filled
        const QWORD cbNewSizeEffective = roundup( cbNewSize, (QWORD)UlParam( m_pinst, JET_paramDbExtensionSize ) * g_cbPage );
        Call( m_pfapiDb->ErrSetSize( *tcRevertPage, cbNewSizeEffective, fTrue, QosSyncDefault( m_pinst ) ) );
    }

    AtomicIncrement( &m_cpgWritePending ); 

    Call( m_pfapiDb->ErrIOWrite(
                *tcRevertPage,
                OffsetOfPgno( pgno ),
                cbDbPageSize,
                (BYTE*) pvPage,
                qos,
                OsWriteIoComplete,
                (DWORD_PTR)this ) );

    m_pdbfilehdr->le_ulRevertPageCount++;

    (*m_pcprintfRevertTrace)( "Pg %ld,%I64x\r\n", pgno, dbtimePage );

HandleError:
    return err;
}

ERR CRBSDatabaseRevertContext::ErrFlushDBPages( USHORT cbDbPageSize, BOOL fFlushDbHdr, CPG* pcpgReverted )
{
    ERR err = JET_errSuccess;
    m_rgRBSDbPage->Sort( CRBSDatabaseRevertContext::ICRBSDatabaseRevertContextCmpPgRec );
    CPG cpgTotal = m_rgRBSDbPage->Size();
    ULONG ulUrgentLevel = (ULONG) UlParam( m_pinst, JET_paramFlight_RBSRevertIOUrgentLevel );
    OSFILEQOS qosIO = QosOSFileFromUrgentLevel( ulUrgentLevel );

    (*m_pcprintfRevertTrace)( "Flushing database pages for database %ws\r\n", m_wszDatabaseName );

    INT ipgno = 0;
    m_asigWritePossible.Reset();

    while ( ipgno < cpgTotal )
    {
        for ( ; ipgno < cpgTotal; ++ipgno )
        {
            CPagePointer ppTempPage( m_rgRBSDbPage->Entry( ipgno ) );
            Assert( ppTempPage.DwPage() );

            err = ErrFlushDBPage( (void*) ppTempPage.DwPage(), ppTempPage.PgNo(), cbDbPageSize, qosIO );

            if ( err == errDiskTilt )
            {
                // Last IOWrite failed. Decrement the write pending counter.
                AtomicDecrement( &m_cpgWritePending );

                // Issue the I/O.
                Call( m_pfapiDb->ErrIOIssue() );

                break;
            }
            else
            {
                Call( err );
            }
        }

        // If there are still writes pending and if we haven't received a signal to continue lets wait till we get the signal.
        // negative wait time turns off deadlock detection
        m_asigWritePossible.FWait( -1000 );
    }

    // Issue the remaining outstanding I/Os and wait for the number of writes to get to 0.
    Call( m_pfapiDb->ErrIOIssue() );

    while ( m_cpgWritePending != 0 )
    {
        // negative wait time turns off deadlock detection
        m_asigWritePossible.FWait( -1000 );
    }

    CArray< CPagePointer >::ERR errArray    = CArray< CPagePointer >::ERR::errSuccess;

    for ( int i = 0; i < cpgTotal; ++i )
    {
        CPagePointer ppTempPage( m_rgRBSDbPage->Entry( i ) );

        // Free the memory here after flushing the page. Same memory pointer as the one stored in the array entry.
        OSMemoryPageFree( (void*) ppTempPage.DwPage() );
        errArray = m_rgRBSDbPage->ErrSetEntry( i, NULL );

        if ( errArray != CArray< CPagePointer >::ERR::errSuccess )
        {
            Assert( errArray == CArray< CPagePointer >::ERR::errOutOfMemory );
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }
    }

    errArray = m_rgRBSDbPage->ErrSetSize( 0 );

    if ( errArray != CArray< CPagePointer >::ERR::errSuccess )
    {
        Assert( errArray == CArray< CPagePointer >::ERR::errOutOfMemory );
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    // This will be NULL if there is no .jfm file.
    if ( m_pfm )
    {
        // flush the flush map
        // Temporarily set min req and min consistent as per the current header. We will update it to the right values at end of the revert.
        //
        m_pfm->SetDbGenMinRequired( max( 1, m_pdbfilehdr->le_lGenMinRequired ) );
        m_pfm->SetDbGenMinConsistent( max( 1, m_pdbfilehdr->le_lGenMinConsistent ) );

        Call( m_pfm->ErrFlushAllSections( OnDebug( fTrue ) ) );
    }

    if ( fFlushDbHdr )
    {
        Call( ErrUtilWriteUnattachedDatabaseHeaders( m_pinst, m_pinst->m_pfsapi, m_wszDatabaseName, m_pdbfilehdr, m_pfapiDb, m_pfm, fFalse ) );
    }

    Call( ErrUtilFlushFileBuffers( m_pfapiDb, iofrRBSRevertUtil ) );

    // Clear the bitmap of cached db pages once we have flushed the pages we had in cache.
    Call( ErrResetSbmPages( &m_psbmCachedDbPages ) );

    *pcpgReverted = cpgTotal;

HandleError:
    return err;
}

ERR CRBSDatabaseRevertContext::ErrBeginTracingToIRS()
{
    Assert( m_pinst->m_pfsapi );
    return ErrBeginDatabaseIncReseedTracing( m_pinst->m_pfsapi, m_wszDatabaseName, &m_pcprintfIRSTrace );
}

//  ================================================================
//                  CRBSRevertContext
//  ================================================================

CRBSRevertContext::CRBSRevertContext( _In_ INST* const pinst )
    : CZeroInit( sizeof( CRBSRevertContext ) ),
    m_pinst ( pinst )
{
    Assert( pinst );
    m_cpgCacheMax       = (LONG)UlParam( JET_paramCacheSizeMax );
    m_irbsdbrcMaxInUse  = -1;

    for ( DBID dbid = 0; dbid < dbidMax; ++dbid )
    {
        m_mpdbidirbsdbrc[ dbid ] = irbsdbrcInvalid;
    }
}

CRBSRevertContext::~CRBSRevertContext()
{
    if ( m_prbsrchk )
    {
        OSMemoryPageFree( (void*)m_prbsrchk );
        m_prbsrchk = NULL;
    }

    if ( m_pfapirbsrchk )
    {
        delete m_pfapirbsrchk;
        m_pfapirbsrchk = NULL;
    }

    if ( m_wszRBSAbsRootDirPath )
    {
        OSMemoryHeapFree( m_wszRBSAbsRootDirPath );
        m_wszRBSAbsRootDirPath = NULL;
    }

    if ( m_wszRBSBaseName )
    {
        OSMemoryHeapFree( m_wszRBSBaseName );
        m_wszRBSBaseName = NULL;
    }

    for ( IRBSDBRC irbsdbrc = 0; irbsdbrc <= m_irbsdbrcMaxInUse; ++irbsdbrc )
    {
        if ( m_rgprbsdbrcAttached[ irbsdbrc ] )
        {
            delete m_rgprbsdbrcAttached[ irbsdbrc ];
            m_rgprbsdbrcAttached[ irbsdbrc ] = NULL;
        }
    }

    if ( m_pcprintfRevertTrace != NULL &&
        CPRINTFNULL::PcprintfInstance() != m_pcprintfRevertTrace )
    {
        (*m_pcprintfRevertTrace)( "Closing revert tracing file.\r\n" );
        delete m_pcprintfRevertTrace;
    }

    m_pcprintfRevertTrace = NULL;
}

ERR CRBSRevertContext::ErrMakeRevertTracingNames(
    _In_ IFileSystemAPI* pfsapi,
    __in_range( cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW ) ULONG cbRBSRCRawPath,
    __out_bcount_z( cbRBSRCRawPath ) WCHAR* wszRBSRCRawPath,
    __in_range( cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW ) ULONG cbRBSRCRawBackupPath,
    __out_bcount_z( cbRBSRCRawBackupPath ) WCHAR* wszRBSRCRawBackupPath )
{
    Assert( pfsapi );

    ERR err = JET_errSuccess;

    WCHAR * szRevertRawExt = L".RBSRC.RAW";
    WCHAR * szRevertRawBackupExt = L".RBSRC.RAW.Prev";

    // init out params

    wszRBSRCRawPath[0] = L'\0';
    wszRBSRCRawBackupPath[0] = L'\0';

    Call( pfsapi->ErrPathBuild( m_wszRBSAbsRootDirPath, m_wszRBSBaseName, szRevertRawExt, wszRBSRCRawPath ) );
    Call( pfsapi->ErrPathBuild( m_wszRBSAbsRootDirPath, m_wszRBSBaseName, szRevertRawBackupExt, wszRBSRCRawBackupPath ) );

HandleError:

    return err;
}

ERR CRBSRevertContext::ErrBeginRevertTracing( bool fDeleteOldTraceFile )
{
    Assert( m_pinst );
    Assert( m_pinst->m_pfsapi );

    ERR err = JET_errSuccess;

    WCHAR wszRBSRCRawFile[ IFileSystemAPI::cchPathMax ]   = { 0 };
    WCHAR wszRBSRCRawBackupFile[ IFileSystemAPI::cchPathMax ] = { 0 };

    IFileAPI *      pfapiSizeCheck  = NULL;
    IFileSystemAPI* pfsapi          = m_pinst->m_pfsapi;

    //  initialize to NULL tracer, in case we fail ...
    m_pcprintfRevertTrace = CPRINTFNULL::PcprintfInstance();

    //  create the tracing path names
    Call( ErrMakeRevertTracingNames( pfsapi, sizeof(wszRBSRCRawFile), wszRBSRCRawFile, sizeof(wszRBSRCRawBackupFile), wszRBSRCRawBackupFile ) );

    if ( fDeleteOldTraceFile )
    {
        Call( pfsapi->ErrFileDelete( wszRBSRCRawFile ) );
        Call( pfsapi->ErrFileDelete( wszRBSRCRawBackupFile ) );
    }

    err = pfsapi->ErrFileOpen( wszRBSRCRawFile, IFileAPI::fmfNone, &pfapiSizeCheck );
    if ( JET_errSuccess == err )
    {
        QWORD cbSize;
        Call( pfapiSizeCheck->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
        delete pfapiSizeCheck;  // delete here b/c we may be about rename it
        pfapiSizeCheck = NULL;

        if ( cbSize > ( 50 * 1024 * 1024 )  )
        {
            Call( pfsapi->ErrFileDelete( wszRBSRCRawBackupFile ) );
            Call( pfsapi->ErrFileMove( wszRBSRCRawFile, wszRBSRCRawBackupFile ) );
        }
    }

    //  create the tracing file
    CPRINTF * const pcprintfAlloc = new CPRINTFFILE( wszRBSRCRawFile );
    Alloc( pcprintfAlloc ); // avoid clobbering the default / NULL tracer

    //  set tracing to goto the tracing file
    m_pcprintfRevertTrace = pcprintfAlloc;

    (*m_pcprintfRevertTrace)( "Please ignore this file.  This is a tracing file for available lag revert information.  You can delete this file if it bothers you.\r\n" );

HandleError:

    if ( pfapiSizeCheck )
    {
        delete pfapiSizeCheck;
        pfapiSizeCheck = NULL;
    }

    return err;
}

// Checks whether the given database name already exists in our list of database revert context.
//
BOOL CRBSRevertContext::FRBSDBRC( PCWSTR wszDatabaseName, IRBSDBRC* pirbsdbrc )
{
    for ( IRBSDBRC irbsdbrc = 0; irbsdbrc <= m_irbsdbrcMaxInUse; ++irbsdbrc )
    {
        if ( UtilCmpFileName( wszDatabaseName, m_rgprbsdbrcAttached[ irbsdbrc ]->WszDatabaseName() ) == 0 )
        {
            if ( pirbsdbrc )
            {
                *pirbsdbrc = irbsdbrc;
            }

            return fTrue;
        }
    }

    return fFalse;
}

// Initializes the checkpoint we will be maintaining for the revert to persist the revert context.
//
ERR CRBSRevertContext::ErrRevertCheckpointInit()
{
    Assert( m_wszRBSAbsRootDirPath );
    Assert( m_wszRBSBaseName );

    WCHAR           wszChkFileBase[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszChkFullName[ IFileSystemAPI::cchPathMax ];
    BOOL            fChkFileExists  = fFalse;
    IFileSystemAPI* pfsapi          = m_pinst->m_pfsapi;
    ERR             err             = JET_errSuccess;

    Assert( pfsapi );
    Assert( !m_prbsrchk );
    Assert( !m_pfapirbsrchk );

    Alloc( m_prbsrchk = (RBSREVERTCHECKPOINT *)PvOSMemoryPageAlloc(  sizeof( RBSREVERTCHECKPOINT ), NULL ) );
    memset( m_prbsrchk, 0, sizeof( RBSREVERTCHECKPOINT ) );

    Call( ErrRBSDirPrefix( m_wszRBSBaseName, wszChkFileBase, sizeof( wszChkFileBase ) ) );
    Call( pfsapi->ErrPathBuild( m_wszRBSAbsRootDirPath, wszChkFileBase, wszNewChkExt, wszChkFullName ) );

    fChkFileExists = ( ErrUtilPathExists( pfsapi, wszChkFullName ) == JET_errSuccess );

    if ( fChkFileExists )
    {
        Call( CIOFilePerf::ErrFileOpen( 
            pfsapi, 
            m_pinst, 
            wszChkFullName, 
            BoolParam( JET_paramEnableFileCache ) ? IFileAPI::fmfCached : IFileAPI::fmfNone, 
            iofileRBS, 
            qwRBSRevertChkFileID, 
            &m_pfapirbsrchk ) );

        err = ErrUtilReadShadowedHeader( m_pinst, pfsapi, m_pfapirbsrchk, (BYTE*) m_prbsrchk, sizeof( RBSREVERTCHECKPOINT ), -1, urhfNoAutoDetectPageSize );

        if ( err < JET_errSuccess )
        {
            // Assume we don't have a checkpoint if checkpoint file is corrupt.
            memset( m_prbsrchk, 0, sizeof( RBSREVERTCHECKPOINT ) );
        }
    }
    else
    {
        Call( CIOFilePerf::ErrFileCreate(
            pfsapi,
            m_pinst,
            wszChkFullName,
            BoolParam( JET_paramEnableFileCache ) ? IFileAPI::fmfCached : IFileAPI::fmfNone,
            iofileRBS,
            qwRBSRevertChkFileID,
            &m_pfapirbsrchk ) );

        LGIGetDateTime( &m_prbsrchk->rbsrchkfilehdr.tmCreate );
        m_prbsrchk->rbsrchkfilehdr.le_filetype = JET_filetypeRBSRevertCheckpoint;

        Call( ErrUtilWriteRBSRevertCheckpointHeaders(
            m_pinst,
            m_pinst->m_pfsapi,
            NULL,
            m_prbsrchk,
            m_pfapirbsrchk ) );
    }

HandleError:
    return err;
}

ERR CRBSRevertContext::ErrRevertCheckpointCleanup()
{
    Assert( m_pfapirbsrchk );
    Assert( m_pinst->m_pfsapi );

    WCHAR           wszChkFullName[ IFileSystemAPI::cchPathMax ];
    m_pfapirbsrchk->ErrPath( wszChkFullName );

    delete m_pfapirbsrchk;
    m_pfapirbsrchk = NULL;

    return m_pinst->m_pfsapi->ErrFileDelete( wszChkFullName );
}

// Initialize database revert context from the RBS attach info only if database revert context for that database doesn't exist already.
//
ERR CRBSRevertContext::ErrRBSDBRCInitFromAttachInfo( const BYTE* pbRBSAttachInfo, SIGNATURE* psignRBSHdrFlush )
{
    ERR err                 = JET_errSuccess;

    for ( const BYTE * pbT = pbRBSAttachInfo; 0 != *pbT; pbT += sizeof( RBSATTACHINFO ) )
    {
        RBSATTACHINFO* prbsattachinfo = (RBSATTACHINFO*) pbT;
        CAutoWSZPATH                    wszTempDbAlignedName;
        IRBSDBRC irbsdbrc;

        if ( prbsattachinfo->FPresent() == 0 )
        {
            break;
        }

        CallS( wszTempDbAlignedName.ErrSet( (UnalignedLittleEndian< WCHAR > *)prbsattachinfo->wszDatabaseName ) );

        if ( !FRBSDBRC( wszTempDbAlignedName, &irbsdbrc ) )
        {
            CRBSDatabaseRevertContext* prbsdbrc              = new CRBSDatabaseRevertContext( m_pinst );

            Alloc( prbsdbrc );
            Assert( m_irbsdbrcMaxInUse + 1 < dbidMax );

            m_rgprbsdbrcAttached[ ++m_irbsdbrcMaxInUse ] = prbsdbrc;

            Call( prbsdbrc->ErrRBSDBRCInit( prbsattachinfo, psignRBSHdrFlush, m_cpgCacheMax ) );
            prbsdbrc->SetDbTimePrevDirtied( prbsattachinfo->DbtimePrevDirtied() );
        }
        else if ( m_rgprbsdbrcAttached[ irbsdbrc ]->DbTimePrevDirtied() != prbsattachinfo->DbtimeDirtied() )
        {
            // If a future RBS with the current db's attach info didn't have previous db time set, it probably means that db was created around that time or the previous one was invalid.
            // If created around that time, we shouldn't have past attach infos.
            Error( ErrERRCheck( JET_errRBSRCInvalidRBS ) );
        }
        else
        {
            m_rgprbsdbrcAttached[ irbsdbrc ]->SetDbTimePrevDirtied( prbsattachinfo->DbtimePrevDirtied() );
        }
    }

HandleError:
    return err;
}

// Calculates the RBS generation range to apply to revert the databases beyond the expected time.
//
ERR CRBSRevertContext::ErrComputeRBSRangeToApply( PCWSTR wszRBSAbsRootDirPath, LOGTIME ltRevertExpected, LOGTIME* pltRevertActual )
{
    Assert( m_prbsrchk );

    LONG        lRBSGenMin, lRBSGenMax;
    WCHAR       wszRBSAbsDirPath[ IFileSystemAPI::cchPathMax ];
    WCHAR       wszRBSAbsFilePath[ IFileSystemAPI::cchPathMax ];
    LOGTIME     tmPrevRBSGen;
    SIGNATURE   signPrevRBSHdrFlush;
    IFileAPI*   pfileapi            = NULL;
    ERR         err                 = JET_errSuccess;    

    Call( ErrRBSGetLowestAndHighestGen_( m_pinst->m_pfsapi, wszRBSAbsRootDirPath, m_wszRBSBaseName, &lRBSGenMin, &lRBSGenMax ) );

    for ( LONG rbsGen = lRBSGenMax; rbsGen >= lRBSGenMin && rbsGen > 0; rbsGen-- )
    {
        RBSFILEHDR rbsfilehdr;

        Call( ErrRBSFilePathForGen_( wszRBSAbsRootDirPath, m_wszRBSBaseName, m_pinst->m_pfsapi, wszRBSAbsDirPath, sizeof( wszRBSAbsDirPath ), wszRBSAbsFilePath, cbOSFSAPI_MAX_PATHW, rbsGen ) );
        Call( ErrRBSLoadRbsGen( m_pinst, wszRBSAbsFilePath, rbsGen, &rbsfilehdr, &pfileapi ) );
        Call( ErrRBSDBRCInitFromAttachInfo( rbsfilehdr.rgbAttach, &rbsfilehdr.rbsfilehdr.signRBSHdrFlush ) );

        if ( LOGTIME::CmpLogTime( rbsfilehdr.rbsfilehdr.tmCreate, ltRevertExpected ) > 0 )
        {
            // If there is no previous RBS gen time set, the previous RBS gen is probably invalid, so we can't revert past this point.
            if ( !rbsfilehdr.rbsfilehdr.tmPrevGen.FIsSet() )
            {
                Error( ErrERRCheck( JET_errRBSRCInvalidRBS ) );
            }

            // If this is not the max RBS gen, we should have captured tmPrevGen from the next RBS gen. 
            // We will compare it with that time with the tmCreate on RBS gen for validation.
            if ( rbsGen != lRBSGenMax )
            {
                if ( LOGTIME::CmpLogTime( rbsfilehdr.rbsfilehdr.tmCreate, tmPrevRBSGen ) != 0 ||
                     ( FRBSFormatFeatureEnabled( JET_rbsfvSignPrevRbsHdrFlush, &rbsfilehdr ) &&
                       memcmp( &rbsfilehdr.rbsfilehdr.signRBSHdrFlush, &signPrevRBSHdrFlush, sizeof( SIGNATURE ) ) != 0 ) )
                {
                    Error( ErrERRCheck( JET_errRBSRCInvalidRBS ) );
                }
            }
            
             // We will capture the previous RBS gen time set and prev RBS header flush signature, so that we can compare with the previous snapshot.
            tmPrevRBSGen = rbsfilehdr.rbsfilehdr.tmPrevGen;
            signPrevRBSHdrFlush = rbsfilehdr.rbsfilehdr.signPrevRBSHdrFlush;
        }
        else
        {
            m_lRBSMinGenToApply = rbsfilehdr.rbsfilehdr.le_lGeneration;
            m_lRBSMaxGenToApply = lRBSGenMax;
            UtilMemCpy( pltRevertActual, &rbsfilehdr.rbsfilehdr.tmCreate, sizeof( LOGTIME ) );
            break;
        }

        if ( pfileapi != NULL )
        {
            delete pfileapi;
            pfileapi = NULL;
        }
    }

    if ( m_lRBSMaxGenToApply == 0 )
    {
        Error( ErrERRCheck( JET_errRBSRCNoRBSFound ) );
    }

    Assert( m_lRBSMinGenToApply > 0 );

HandleError:
    if ( err == JET_errReadVerifyFailure || err == JET_errFileInvalidType || err == JET_errBadRBSVersion || err == JET_errRBSInvalidSign )
    {
        err = ErrERRCheck( JET_errRBSRCInvalidRBS );
    }

    if ( pfileapi != NULL )
    {
        delete pfileapi;
    }

    return err;
}

// Adjust the expected revert time if we are already in the middle of a revert. 
// We will look at the revert checkpoint to determine that.
//
ERR CRBSRevertContext::ErrUpdateRevertTimeFromCheckpoint( LOGTIME* pltRevertExpected )
{
    Assert( m_prbsrchk );

    if ( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate != JET_revertstateNone )
    {
        INT fCompare = LOGTIME::CmpLogTime( m_prbsrchk->rbsrchkfilehdr.tmCreateCurrentRBSGen, *pltRevertExpected );
        
        // If we are already in reverting state and revert time of current RBS gen being applied is lower than expected revert time
        // we will use the former as our expected revert time since we have already gone beyond the point we want to revert to.
        if ( fCompare < 0 )
        {
            UtilMemCpy( pltRevertExpected, &m_prbsrchk->rbsrchkfilehdr.tmCreateCurrentRBSGen, sizeof( LOGTIME ) );
        }
        else if ( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateCopingLogs && fCompare > 0 )
        {
            // If we are in copying logs state, we will only allow revert to the current RBS for which we are copying logs for. 
            // We won't allow going further behind as we might have already copied logs partially.
            // TODO SOMEONE: If user pass delete logs flag we should be able to allow, but requires extra handling from checkpoint file as we delete logs only at the end of revert.
            return ErrERRCheck( JET_errRBSRCCopyLogsRevertState );
        }
    }

    return JET_errSuccess;
}

// Update RBS range based on revert checkpoint file as we might have already applied certain generations.
//
VOID CRBSRevertContext::UpdateRBSGenToApplyFromCheckpoint()
{
    Assert( m_prbsrchk );

    if ( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate != JET_revertstateNone )
    {
        m_lRBSMaxGenToApply = m_prbsrchk->rbsrchkfilehdr.le_rbsposCheckpoint.le_lGeneration;
        Assert( m_lRBSMaxGenToApply >= m_lRBSMinGenToApply );
    }
}

// Update revert checkpoint to the given state and RBSPOS.
//
ERR CRBSRevertContext::ErrUpdateRevertCheckpoint( ULONG revertstate, RBS_POS rbspos, LOGTIME tmCreateCurrentRBSGen, BOOL fUpdateRevertedPageCount )
{
    Assert( revertstate != JET_revertstateNone );
    Assert( m_pfapirbsrchk );
    Assert( m_prbsrchk );

    __int64 ftSinceLastUpdate = UtilGetCurrentFileTime() - m_ftRevertLastUpdate;
    LONG lGenMinReq = lGenerationMax;
    LONG lGenMaxReq = 0;

    switch ( revertstate )
    {
        case JET_revertstateInProgress:
        {
            AssertRTL( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateNone ||
                m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateInProgress );
            break;
        }
        case JET_revertstateCopingLogs:
        {
            AssertRTL( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateInProgress );
            break;
        }
        case JET_revertstateBackupSnapshot:
        {
            AssertRTL( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateCopingLogs );
            break;
        }
        case JET_revertstateRemoveSnapshot:
        {
            AssertRTL( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateBackupSnapshot );
            break;
        }
    }

    if ( m_ftRevertLastUpdate != 0 )
    {
        m_prbsrchk->rbsrchkfilehdr.le_cSecInRevert = m_prbsrchk->rbsrchkfilehdr.le_cSecInRevert + UtilConvertFileTimeToSeconds( ftSinceLastUpdate );

        // Update after each time we flush the checkpoint.
        m_ftRevertLastUpdate = UtilGetCurrentFileTime();
    }

    if ( m_irbsdbrcMaxInUse > 0 )
    {
        if ( m_prbsrchk->rbsrchkfilehdr.le_lGenMinRevertStart == 0 || m_prbsrchk->rbsrchkfilehdr.le_lGenMaxRevertStart == 0 )
        {            
            for ( IRBSDBRC irbsdbrc = 0; irbsdbrc <= m_irbsdbrcMaxInUse; ++irbsdbrc )
            {
                LONG lgenMinReqDb = m_rgprbsdbrcAttached[ irbsdbrc ]->LGenMinRequired();
                LONG lgenMaxReqDb = m_rgprbsdbrcAttached[ irbsdbrc ]->LGenMaxRequired();

                Assert( lgenMinReqDb <= lgenMaxReqDb );
                Assert( ( lgenMinReqDb != 0 && lgenMaxReqDb !=0 ) || lgenMinReqDb == lgenMaxReqDb );
                
                // If db was clean shutdown we will set min and max required to be the last consistent state as we need this main for reporting purpose.
                if ( lgenMinReqDb == 0 )
                {
                    lgenMinReqDb = m_rgprbsdbrcAttached[ irbsdbrc ]->LgenLastConsistent();
                    lgenMaxReqDb = m_rgprbsdbrcAttached[ irbsdbrc ]->LgenLastConsistent();
                }

                lGenMinReq = min( lGenMinReq, lgenMinReqDb );
                lGenMaxReq = max( lGenMaxReq, lgenMaxReqDb );
            }

            m_prbsrchk->rbsrchkfilehdr.le_lGenMinRevertStart = lGenMinReq;
            m_prbsrchk->rbsrchkfilehdr.le_lGenMaxRevertStart = lGenMaxReq;
        }
    }

    if ( !m_prbsrchk->rbsrchkfilehdr.tmCreate.FIsSet() )
    {
        LGIGetDateTime( &m_prbsrchk->rbsrchkfilehdr.tmCreate );
        m_prbsrchk->rbsrchkfilehdr.le_filetype = JET_filetypeRBSRevertCheckpoint;
    }

    if ( !m_prbsrchk->rbsrchkfilehdr.tmExecuteRevertBegin.FIsSet() && revertstate == JET_revertstateInProgress )
    {
        LGIGetDateTime( &m_prbsrchk->rbsrchkfilehdr.tmExecuteRevertBegin );
    }

    if ( fUpdateRevertedPageCount )
    {
        LittleEndian<QWORD> le_cpgRevertedCurRBSGen = m_cPagesRevertedCurRBSGen;
        m_prbsrchk->rbsrchkfilehdr.le_cPagesReverted += le_cpgRevertedCurRBSGen;
        m_cPagesRevertedCurRBSGen = 0; // Reset the counter.
    }

    Assert( m_prbsrchk->rbsrchkfilehdr.le_rbsposCheckpoint.le_lGeneration >= rbspos.lGeneration ||  
            ( m_prbsrchk->rbsrchkfilehdr.le_rbsposCheckpoint.le_lGeneration == 0 && 
              m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateNone ) );
    Assert( !m_prbsrchk->rbsrchkfilehdr.tmCreateCurrentRBSGen.FIsSet() ||
            LOGTIME::CmpLogTime( tmCreateCurrentRBSGen, m_prbsrchk->rbsrchkfilehdr.tmCreateCurrentRBSGen ) <= 0 );

    m_prbsrchk->rbsrchkfilehdr.le_rbsposCheckpoint = rbspos;
    m_prbsrchk->rbsrchkfilehdr.tmCreateCurrentRBSGen = tmCreateCurrentRBSGen;

    m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate = revertstate;

    return ErrUtilWriteRBSRevertCheckpointHeaders(
                m_pinst,
                m_pinst->m_pfsapi,
                NULL,
                m_prbsrchk,
                m_pfapirbsrchk );
}

// Initializes RBS revert context based on the expected time we want to revert to.
//
ERR CRBSRevertContext::ErrInitContext( LOGTIME ltRevertExpected, LOGTIME* pltRevertActual, CPG cpgCache, BOOL fDeleteExistingLogs )
{
    ERR err = JET_errSuccess;
    WCHAR wszRBSAbsRootDirPath[ IFileSystemAPI::cchPathMax ];
    WCHAR wszRBSAbsLogPath[ IFileSystemAPI::cchPathMax ];

    Call( ErrRBSInitPaths_( m_pinst, &m_wszRBSAbsRootDirPath, &m_wszRBSBaseName ) );

    // Make sure they are initialized to valid paths.
    Assert( m_wszRBSBaseName );
    Assert( m_wszRBSBaseName[ 0 ] );

    Assert( m_wszRBSAbsRootDirPath );
    Assert( m_wszRBSAbsRootDirPath[ 0 ] );

    m_cpgCacheMax = cpgCache;
    Call( ErrRevertCheckpointInit() );

    Call ( ErrUpdateRevertTimeFromCheckpoint( &ltRevertExpected ) );

    Call( ErrOSStrCbCopyW( wszRBSAbsRootDirPath, sizeof( wszRBSAbsRootDirPath ), m_wszRBSAbsRootDirPath ) );

    if ( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateRemoveSnapshot  )
    {
        Call( ErrOSStrCbAppendW( wszRBSAbsRootDirPath, sizeof( wszRBSAbsRootDirPath ), wszRBSBackupDir ) );
        Call( m_pinst->m_pfsapi->ErrPathFolderNorm( wszRBSAbsRootDirPath, sizeof( wszRBSAbsRootDirPath ) ) );
    }

    Call( ErrComputeRBSRangeToApply( wszRBSAbsRootDirPath, ltRevertExpected, pltRevertActual ) );
    UpdateRBSGenToApplyFromCheckpoint();

    m_ltRevertTo            = *pltRevertActual;
    m_fDeleteExistingLogs   = fDeleteExistingLogs;

    // We will rerun the log checks even though we do this in the prepare stage as the state could have changed between the time we prepared and completed revert.
    Call( ErrRBSPerformLogChecks( m_pinst, wszRBSAbsRootDirPath, m_wszRBSBaseName, m_lRBSMinGenToApply, !fDeleteExistingLogs, wszRBSAbsLogPath ) );

HandleError:
    return err;
}

// Create and initalize RBS revert context for the given instance.
//
ERR CRBSRevertContext::ErrRBSRevertContextInit( 
                INST*       pinst, 
        _In_    LOGTIME     ltRevertExpected,
        _In_    CPG         cpgCache,
        _In_    JET_GRBIT   grbit,
        _Out_   LOGTIME*    pltRevertActual,
        _Out_   CRBSRevertContext**    pprbsrc )
{
    Assert( pprbsrc );
    Assert( *pprbsrc == NULL );

    ERR      err     = JET_errSuccess;
    CRBSRevertContext*  prbsrc  = NULL;

    Alloc( prbsrc = new CRBSRevertContext( pinst ) );
    Call( prbsrc->ErrInitContext( ltRevertExpected, pltRevertActual, cpgCache, grbit & JET_bitDeleteAllExistingLogs ) );   
    *pprbsrc = prbsrc;
    return JET_errSuccess;

HandleError:
    if ( prbsrc != NULL )
    {
        delete prbsrc;
    }

    return err;
}

// Add the given page record to the corresponding database's revert context.
//
ERR CRBSRevertContext::ErrAddPageRecord( void* pvPage, DBID dbid, PGNO pgno, BOOL fReplaceCached )
{
    Assert( m_mpdbidirbsdbrc[ dbid ] != irbsdbrcInvalid );
    Assert( m_mpdbidirbsdbrc[ dbid ] <= m_irbsdbrcMaxInUse );
    Assert( m_rgprbsdbrcAttached[ m_mpdbidirbsdbrc[ dbid ] ] );
    
    ERR err = JET_errSuccess;
    BOOL fPageAddedToCache = fTrue;

    Call( m_rgprbsdbrcAttached[ m_mpdbidirbsdbrc[ dbid ] ]->ErrAddPage( pvPage, pgno, fReplaceCached, &fPageAddedToCache ) );

    if ( fPageAddedToCache )
    {
        m_cpgCached++;
    }

HandleError:
    return err;
}

// Check if a page is already capture in pages' corresonding database revert context.
//
BOOL CRBSRevertContext::FPageAlreadyCaptured( DBID dbid, PGNO pgno )
{
    Assert( m_mpdbidirbsdbrc[ dbid ] != irbsdbrcInvalid );
    Assert( m_mpdbidirbsdbrc[ dbid ] <= m_irbsdbrcMaxInUse );
    Assert( m_rgprbsdbrcAttached[ m_mpdbidirbsdbrc[ dbid ] ] );

    return m_rgprbsdbrcAttached[ m_mpdbidirbsdbrc[ dbid ] ]->FPageAlreadyCaptured( pgno );
}

// Adds the given pgno as a reverted new page to the list of pages to reverted for the given database.
//
ERR CRBSRevertContext::ErrAddRevertedNewPage( DBID dbid, PGNO pgnoRevertNew )
{
    ERR err         = JET_errSuccess;
    void*   pvPage  = NULL;

    if ( !FPageAlreadyCaptured( dbid, pgnoRevertNew ) )
    {
        pvPage = PvOSMemoryPageAlloc( m_cbDbPageSize, NULL );
        Alloc( pvPage );

        CPAGE cpage;
        cpage.GetRevertedNewPage( pgnoRevertNew, pvPage, m_cbDbPageSize );
        Assert( cpage.Pgft() == CPAGE::PageFlushType::pgftUnknown );

        // For new page/multi new page record type, we don't store the preimage of the page since it indicates that it was a new page before it was updated.
        // We will just write out an empty page for such a page.
        Call( ErrAddPageRecord( pvPage, dbid, pgnoRevertNew, fFalse ) );
    }

HandleError:
    if ( err < JET_errSuccess && pvPage )
    {
        OSMemoryPageFree( pvPage );
    }

    return err;
}

// Checks whether we continue applying RBS, taking any required actions.
//
ERR CRBSRevertContext::ErrCheckApplyRBSContinuation()
{
    ERR err = JET_errSuccess;

    // Flush the cached pages if full.
    if ( m_cpgCached >= m_cpgCacheMax )
    {
        Call( ErrFlushPages( fFalse ) );
    }

    // Cancel revert if requested.
    if ( m_fRevertCancelled )
    {
        Error( ErrERRCheck( JET_errRBSRCRevertCancelled ) );
    }

HandleError:
    return err;
}

// Apply the given RBS record to the databases.
//
// fDbHeaderOnly - We will go through the just to capture the database header. fDbHeaderOnly should be passed only for the lowest RBS gen we applied.
//                 We will need this only if we were in the final phase of revert (CopyingLogs) and we hit a failure
ERR CRBSRevertContext::ErrApplyRBSRecord( RBSRecord* prbsrec, BOOL fCaptureDbHdrFromRBS, BOOL fDbHeaderOnly, BOOL* pfGivenDbfilehdrCaptured )
{
    BYTE                bRecType        = prbsrec->m_bRecType;
    void*               pvPage          = NULL;
    ERR                 err             = JET_errSuccess;

    if ( pfGivenDbfilehdrCaptured )
    {
        *pfGivenDbfilehdrCaptured = fFalse;
    }

    switch ( bRecType )
    {
        // No action to be taken for database header.
        case rbsrectypeDbHdr:
        {
            // TODO SOMEONE: Once we have checkpoint for specific RBS position we need to probably capture this in our Revert checkpoint file.
            if ( !fCaptureDbHdrFromRBS )
            {
                break;
            }

            RBSDbHdrRecord* prbsdbhdrrec    = ( RBSDbHdrRecord* ) prbsrec;
            Assert( prbsdbhdrrec->m_dbid < dbidMax );

            IRBSDBRC irbsdbrc               = m_mpdbidirbsdbrc[ prbsdbhdrrec->m_dbid ];
            CRBSDatabaseRevertContext* prbsdbrc              = NULL;

            Assert( irbsdbrc != irbsdbrcInvalid );
            Assert( irbsdbrc <= m_irbsdbrcMaxInUse );

            prbsdbrc = m_rgprbsdbrcAttached[ irbsdbrc ];
            Assert( prbsdbrc );
            Assert( prbsdbrc->DBIDCurrent() < dbidMax );

            prbsdbrc->ErrRBSCaptureDbHdrFromRBS( prbsdbhdrrec, pfGivenDbfilehdrCaptured );

            break;
        }

        // For db attach record, map dbid to the corresponding RBS DB revert context index as dbid might have been different before.
        case rbsrectypeDbAttach:
        {
            IRBSDBRC irbsdbrc                   = irbsdbrcInvalid;
            BOOL     fRBSDBRC                   = fFalse;
            CRBSDatabaseRevertContext* prbsdbrc = NULL;
            RBSDbAttachRecord* prbsdbatchrec    = ( RBSDbAttachRecord* ) prbsrec;
            CAutoWSZPATH wszDbName;
            Call( wszDbName.ErrSet( prbsdbatchrec->m_wszDbName ) );

            fRBSDBRC = FRBSDBRC( wszDbName, &irbsdbrc );

            Assert( fRBSDBRC );
            Assert( irbsdbrc != irbsdbrcInvalid );
            Assert( irbsdbrc <= m_irbsdbrcMaxInUse );

            prbsdbrc = m_rgprbsdbrcAttached[ irbsdbrc ];
            Assert( prbsdbrc );
            Assert( prbsdbatchrec->m_dbid < dbidMax );

            // Set the dbid to irbsdbrc mapping based on the dbid from the attach info.
            if ( prbsdbatchrec->m_dbid != prbsdbrc->DBIDCurrent() )
            {
                if ( prbsdbrc->DBIDCurrent() == dbidMax )
                {
                    prbsdbrc->SetDBIDCurrent( prbsdbatchrec->m_dbid );
                }
                else if ( m_mpdbidirbsdbrc[ prbsdbrc->DBIDCurrent() ] == irbsdbrc )
                {
                    // the dbid for the database has changed. Set the old dbid to irbsdbrc mapping to invalid
                    m_mpdbidirbsdbrc[ prbsdbrc->DBIDCurrent() ] = irbsdbrcInvalid;
                }

                if ( m_mpdbidirbsdbrc[ prbsdbatchrec->m_dbid ] != irbsdbrcInvalid )
                {
                    // There is a different db with this dbid. Let's reset the dbid of that database.
                    m_rgprbsdbrcAttached[ m_mpdbidirbsdbrc[ prbsdbatchrec->m_dbid ] ]->SetDBIDCurrent( dbidMax ); 
                }

                m_mpdbidirbsdbrc[ prbsdbatchrec->m_dbid ] = irbsdbrc;
            }
            else if ( m_mpdbidirbsdbrc[ prbsdbrc->DBIDCurrent() ] != irbsdbrc )
            {
                Assert( m_mpdbidirbsdbrc[ prbsdbrc->DBIDCurrent() ] == irbsdbrcInvalid );                
                m_mpdbidirbsdbrc[ prbsdbatchrec->m_dbid ] = irbsdbrc;
            }
            
            break;
        }
        case rbsrectypeDbPage:
        {
            if ( fDbHeaderOnly )
            {
                return JET_errSuccess;
            }

            DATA dataImage;

            RBSDbPageRecord* prbsdbpgrec = ( RBSDbPageRecord* ) prbsrec;
            dataImage.SetPv( prbsdbpgrec->m_rgbData );
            dataImage.SetCb( prbsdbpgrec->m_usRecLength - sizeof(RBSDbPageRecord) );

            BOOL fPageAlreadyCaptured = FPageAlreadyCaptured( prbsdbpgrec->m_dbid, prbsdbpgrec->m_pgno );

            // If either revert always flag is set or if we have not already captured page preimage to revert to, capture the page record.
            //
            if ( prbsdbpgrec->m_fFlags & fRBSPreimageRevertAlways || !fPageAlreadyCaptured )
            {
                pvPage = PvOSMemoryPageAlloc( m_cbDbPageSize, NULL );
                Alloc( pvPage );

                if ( prbsdbpgrec->m_fFlags )
                {
                    Call( ErrRBSDecompressPreimage( dataImage, m_cbDbPageSize, (BYTE*) pvPage, prbsdbpgrec->m_pgno, prbsdbpgrec->m_fFlags ) );
                }
                else
                {
                    // If not decompressing copy the page dage directly into the buffer.
                    UtilMemCpy( pvPage, prbsdbpgrec->m_rgbData, dataImage.Cb() );
                }

                Assert( m_cbDbPageSize == dataImage.Cb() );

                CPAGE cpage;
                cpage.LoadPage( ifmpNil, prbsdbpgrec->m_pgno, pvPage, m_cbDbPageSize );
                cpage.PreparePageForWrite( CPAGE::PageFlushType::pgftUnknown, fTrue, fTrue );
                cpage.UnloadPage();

                Call( ErrAddPageRecord( pvPage, prbsdbpgrec->m_dbid, prbsdbpgrec->m_pgno, fPageAlreadyCaptured ) );
            }

            break;
        }
        
        case rbsrectypeDbNewPage:
        {
            if ( fDbHeaderOnly )
            {
                return JET_errSuccess;
            }

            RBSDbNewPageRecord* prbsdbnewpgrec = ( RBSDbNewPageRecord* ) prbsrec;
            Call( ErrAddRevertedNewPage( prbsdbnewpgrec->m_dbid, prbsdbnewpgrec->m_pgno ) );
            break;
        }

        case rbsrectypeDbEmptyPages:
        {
            if ( fDbHeaderOnly )
            {
                return JET_errSuccess;
            }

            RBSDbEmptyPagesRecord* prbsdbemptypgrec = ( RBSDbEmptyPagesRecord* ) prbsrec;
            PGNO pgnoLast = prbsdbemptypgrec->m_pgnoFirst + prbsdbemptypgrec->m_cpg - 1;

            for ( PGNO pgno = prbsdbemptypgrec->m_pgnoFirst; pgno <= pgnoLast; ++pgno )
            {
                Call( ErrAddRevertedNewPage( prbsdbemptypgrec->m_dbid, pgno ) );
                Call( ErrCheckApplyRBSContinuation() );
            }

            break;
        }

        default:
            Call( ErrERRCheck( JET_errRBSInvalidRecord ) );
    }

    return JET_errSuccess;

HandleError:
    if ( pvPage )
    {
        OSMemoryPageFree( pvPage );
    }

    return err;
}

ERR CRBSRevertContext::ErrFlushPages( BOOL fFlushDbHdr )
{
    // TODO SOMEONE: Can perform async flush across HDD/SSD.
    ERR err = JET_errSuccess;
    
    for ( IRBSDBRC irbsdbrc = 0; irbsdbrc <= m_irbsdbrcMaxInUse; ++irbsdbrc )
    {
        CPG cpgReverted;
        Call( m_rgprbsdbrcAttached[ irbsdbrc ]->ErrFlushDBPages( m_cbDbPageSize, fFlushDbHdr, &cpgReverted ) );
        m_cPagesRevertedCurRBSGen += cpgReverted;
    }

    // Reset the number of pages currently cached once we flush the pages in the current cache.
    m_cpgCached = 0;

HandleError:
    return err;
}

// Apply given RBS gen to the databases.
//
// fDbHeaderOnly - We will go through the just to capture the database header. fDbHeaderOnly should be passed only for the lowest RBS gen we applied.
//                 We will need this only if we were in the final phase of revert (CopyingLogs) and we hit a failure
ERR CRBSRevertContext::ErrRBSGenApply( LONG lRBSGen, RBSFILEHDR* prbsfilehdr, BOOL fDbHeaderOnly, BOOL fUseBackupDir )
{
    ERR              err = JET_errSuccess;

    WCHAR wszRBSAbsRootDirPath[ IFileSystemAPI::cchPathMax ];
    WCHAR wszRBSAbsDirPath[ IFileSystemAPI::cchPathMax ];
    WCHAR wszRBSAbsFilePath[ IFileSystemAPI::cchPathMax ];
    WCHAR wszErrorReason[ cbOSFSAPI_MAX_PATHW ];
    
    RBS_POS             rbsposRecStart          = rbsposMin;
    CRevertSnapshot*    prbs                    = NULL;
    IFileAPI*           pfapirbs                = NULL;
    RBSRecord*          prbsRecord              = NULL;
    RBS_POS             rbspos                  = { 0, lRBSGen };
    BOOL                fGivenDbfilehdrCaptured = fFalse;

    Assert( lRBSGen == m_lRBSMinGenToApply || !fDbHeaderOnly );
    Assert( m_irbsdbrcMaxInUse >= 0 );

    (*m_pcprintfRevertTrace)( "RBSGen - %ld, DbHeaderOnly - %ld, UseBackupDir - %ld.\r\n", lRBSGen, fDbHeaderOnly, fUseBackupDir );
    Call( ErrOSStrCbCopyW( wszRBSAbsRootDirPath, sizeof( wszRBSAbsRootDirPath ), m_wszRBSAbsRootDirPath ) );

    if ( fUseBackupDir )
    {
        Call( ErrOSStrCbAppendW( wszRBSAbsRootDirPath, sizeof( wszRBSAbsRootDirPath ), wszRBSBackupDir ) );
        Call( m_pinst->m_pfsapi->ErrPathFolderNorm( wszRBSAbsRootDirPath, sizeof( wszRBSAbsRootDirPath ) ) );
    }

    Call( ErrRBSFilePathForGen_( wszRBSAbsRootDirPath, m_wszRBSBaseName, m_pinst->m_pfsapi, wszRBSAbsDirPath, sizeof( wszRBSAbsDirPath ), wszRBSAbsFilePath, cbOSFSAPI_MAX_PATHW, lRBSGen ) );
    Call( CIOFilePerf::ErrFileOpen( m_pinst->m_pfsapi, m_pinst, wszRBSAbsFilePath, IFileAPI::fmfReadOnly, iofileRBS, qwRBSFileID, &pfapirbs ) );

    Alloc( prbs = new CRevertSnapshot( m_pinst ) );
    Call( prbs->ErrSetRBSFileApi( pfapirbs ) );

    Assert( prbs->RBSFileHdr() );

    // We will copy and return rbs file header for the last snapshot we are applying.
    if ( lRBSGen == m_lRBSMinGenToApply )
    {
        Assert( prbsfilehdr );
        UtilMemCpy( prbsfilehdr, prbs->RBSFileHdr(), sizeof( RBSFILEHDR ) );
    }

    // Don't need to update checkpoint if we are just trying to capture the database header.
    if ( !fDbHeaderOnly )
    {
        Call( ErrUpdateRevertCheckpoint( JET_revertstateInProgress, rbspos, prbs->RBSFileHdr()->rbsfilehdr.tmCreate, fTrue ) );
    }

    // Fault injection to test checkpoint helps in skipping applying RBS generation again and we don't hit any issues continuing from checkpoint.
    if ( lRBSGen < m_lRBSMaxGenToApply )
    {
        Call( ErrFaultInjection( 50123 ) );
    }

    m_cbDbPageSize = prbs->RBSFileHdr()->rbsfilehdr.le_cbDbPageSize;

    err = prbs->ErrGetNextRecord( &prbsRecord, &rbsposRecStart, wszErrorReason );

    while ( err != JET_wrnNoMoreRecords && err == JET_errSuccess )
    {
        Call( ErrApplyRBSRecord( prbsRecord, lRBSGen == m_lRBSMinGenToApply, fDbHeaderOnly, &fGivenDbfilehdrCaptured ) );
        Call( ErrCheckApplyRBSContinuation() );

        // We just want to go through the RBS till we capture the database header for all the databases attached.
        // If the current record was db file header record and was captured, we will check if we are done and break out of the loop.
        if ( fDbHeaderOnly && fGivenDbfilehdrCaptured )
        {
            BOOL fAllCaptured = fTrue;

            for ( IRBSDBRC irbsdbrc = 0; irbsdbrc <= m_irbsdbrcMaxInUse; ++irbsdbrc )
            {
                fAllCaptured = fAllCaptured && m_rgprbsdbrcAttached[ irbsdbrc ]->PDbfilehdrFromRBS();
            }

            if ( fAllCaptured )
            {
                break;
            }
        }

        err = prbs->ErrGetNextRecord( &prbsRecord, &rbsposRecStart, wszErrorReason );
    }

    if ( err == JET_wrnNoMoreRecords && !fDbHeaderOnly )
    {
        // We will update revert page count in db header at the end of generation since we might crash and have wrong numbers.
        // 
        Call( ErrFlushPages( fTrue ) );
        err = JET_errSuccess;
    }
    
HandleError:
    if ( prbs )
    {
        // This should the delete the file handle as well.
        delete prbs;
        prbs = NULL;
    }

    if ( m_pcprintfRevertTrace )
    {
        //  trace the error status ... 
        TraceFuncComplete( m_pcprintfRevertTrace, __FUNCTION__, err );
    }

    return err;
}

// Manages database state, checkpoint file
//
ERR CRBSRevertContext::ErrUpdateDbStatesAfterRevert( SIGNATURE* psignRbsHdrFlush )
{
    WCHAR wszPathJetChkLog[ IFileSystemAPI::cchPathMax ];
    ERR err = JET_errSuccess;

    Assert( m_pinst->m_plog );

    CallS( m_pinst->m_pfsapi->ErrPathBuild( 
        SzParam( m_pinst, JET_paramSystemPath ), 
        SzParam( m_pinst, JET_paramBaseName ), 
        ( UlParam( m_pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ? wszOldChkExt : wszNewChkExt, 
        wszPathJetChkLog ) );

    Assert( m_pinst->m_pfsapi );

    // Delete the checkpoint file.
    err = m_pinst->m_pfsapi->ErrFileDelete( wszPathJetChkLog );

    Call( err == JET_errFileNotFound ? JET_errSuccess : err );

    Assert( m_prbsrchk );
    Assert( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateRemoveSnapshot );

    for ( IRBSDBRC irbsdbrc = 0; irbsdbrc <= m_irbsdbrcMaxInUse; ++irbsdbrc )
    {
        m_rgprbsdbrcAttached[ irbsdbrc ]->ErrSetDbstateAfterRevert( psignRbsHdrFlush );
    }

HandleError:
    return err;
}

// Copies the required logs needed for the databases at the end of revert.
//
ERR CRBSRevertContext::ErrCopyRequiredLogsAfterRevert( LONG lgenMinToCopy, LONG lgenMaxToCopy )
{
    Assert( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateCopingLogs );

    WCHAR wszRBSAbsLogPath[ IFileSystemAPI::cchPathMax ];
    ERR     err         = JET_errSuccess;

    // We will rerun the log checks even though we do this in the prepare stage as the state could have changed between the time we prepared and completed revert.
    Call( ErrRBSPerformLogChecks( m_pinst, m_wszRBSAbsRootDirPath, m_wszRBSBaseName, m_lRBSMinGenToApply, !m_fDeleteExistingLogs, wszRBSAbsLogPath ) );
 
    if ( m_fDeleteExistingLogs )
    {    
        WCHAR wszLogDeleteFilter[ IFileSystemAPI::cchPathMax ];
        Call( ErrOSStrCbCopyW( wszLogDeleteFilter, sizeof( wszLogDeleteFilter ), L"*" ) );
        Call( ErrOSStrCbAppendW( wszLogDeleteFilter, sizeof( wszLogDeleteFilter ), ( UlParam( m_pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ? wszOldLogExt : wszNewLogExt ) );
        Call( ErrRBSDeleteAllFiles( m_pinst->m_pfsapi, SzParam( m_pinst, JET_paramLogFilePath ), wszLogDeleteFilter, fFalse ) );
    }

    // Now copy logs from RBS log directory to actual log directory.
    // We will try to find eCurrent log if max numbered log is missing in the case when we are deleting logs from target directory.
    // If we are not deleting, target directory should have a more complete version of the current log and we do a subset check for the same above.
    // But in case we are deleting, we should copy the current log if max numbered log is missing.
    Call( ErrRBSCopyRequiredLogs_(
        m_pinst,
        lgenMinToCopy,
        lgenMaxToCopy,
        wszRBSAbsLogPath,
        SzParam( m_pinst, JET_paramLogFilePath ),
        fFalse,
        m_fDeleteExistingLogs ) );

    // Fault injection to test continuing copying logs state.
    Call( ErrFaultInjection( 50124 ) );
    
    Call( ErrUpdateRevertCheckpoint( JET_revertstateBackupSnapshot, m_prbsrchk->rbsrchkfilehdr.le_rbsposCheckpoint, m_prbsrchk->rbsrchkfilehdr.tmCreateCurrentRBSGen, fFalse ) );

HandleError:
    if ( err < JET_errSuccess && m_pcprintfRevertTrace )
    {
        //  if we managed to at least getting tracing up, trace the issue ... 
        TraceFuncComplete( m_pcprintfRevertTrace, __FUNCTION__, err );
    }

    return err;
}

// Backs up the revert snapshot after we have completed the revert operation, to help with any investigations.
//
ERR CRBSRevertContext::ErrBackupRBSAfterRevert()
{
    Assert( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateBackupSnapshot );
    Assert( m_pinst->m_pfsapi );

    ERR     err         = JET_errSuccess;

    WCHAR wszRBSAbsBackupDirPath[ IFileSystemAPI::cchPathMax ];
    Call( ErrOSStrCbCopyW( wszRBSAbsBackupDirPath, sizeof( wszRBSAbsBackupDirPath ), m_wszRBSAbsRootDirPath ) );
    Call( ErrOSStrCbAppendW( wszRBSAbsBackupDirPath, sizeof( wszRBSAbsBackupDirPath ), wszRBSBackupDir ) );
    Call( m_pinst->m_pfsapi->ErrPathFolderNorm( wszRBSAbsBackupDirPath, sizeof( wszRBSAbsBackupDirPath ) ) );

    // Backup all the RBS generations we have applied in case we need them for investigation later.
    for ( LONG rbsGenToBackup = m_lRBSMaxGenToApply; rbsGenToBackup >= m_lRBSMinGenToApply; --rbsGenToBackup )
    {
        if ( m_fRevertCancelled )
        {
            Error( ErrERRCheck( JET_errRBSRCRevertCancelled ) );
        }

        WCHAR wszRBSGenAbsBackupDirPath[ IFileSystemAPI::cchPathMax ];
        WCHAR wszRBSGenAbsSrcDirPath[ IFileSystemAPI::cchPathMax ];
        WCHAR wszRBSGenAbsFilePath[ IFileSystemAPI::cchPathMax ];
        Call( ErrRBSFilePathForGen_( wszRBSAbsBackupDirPath, m_wszRBSBaseName, m_pinst->m_pfsapi, wszRBSGenAbsBackupDirPath, sizeof( wszRBSGenAbsBackupDirPath ), wszRBSGenAbsFilePath, sizeof( wszRBSGenAbsFilePath ), rbsGenToBackup ) );
        Call( ErrRBSFilePathForGen_( m_wszRBSAbsRootDirPath, m_wszRBSBaseName, m_pinst->m_pfsapi, wszRBSGenAbsSrcDirPath, sizeof( wszRBSGenAbsSrcDirPath ), wszRBSGenAbsFilePath, sizeof( wszRBSGenAbsFilePath ), rbsGenToBackup ) );
        Call( ErrFolderCopy( m_pinst->m_pfsapi, wszRBSGenAbsSrcDirPath, wszRBSGenAbsBackupDirPath, NULL, fTrue, fTrue ) );
    }

    // Fault injection to test continuing from JET_revertstateRemoveSnapshot.
    Call( ErrFaultInjection( 50125 ) );

    Call( ErrUpdateRevertCheckpoint( JET_revertstateRemoveSnapshot, m_prbsrchk->rbsrchkfilehdr.le_rbsposCheckpoint, m_prbsrchk->rbsrchkfilehdr.tmCreateCurrentRBSGen, fFalse ) );

HandleError:
    if ( err < JET_errSuccess && m_pcprintfRevertTrace )
    {
        //  if we managed to at least getting tracing up, trace the issue ...
        TraceFuncComplete( m_pcprintfRevertTrace, __FUNCTION__, err );
    }

    return err;
}

// Removes the revert snapshots we have applied after we have completed the revert operation, 
// so that we will still consider the snapshot before the min generation we have applied valid.
//
ERR CRBSRevertContext::ErrRemoveRBSAfterRevert()
{
    Assert( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateRemoveSnapshot );
    Assert( m_pinst->m_pfsapi );

    ERR     err         = JET_errSuccess;

    // Backup all the RBS generations we have applied in case we need them for investigation later.
    for ( LONG rbsGenToBackup = m_lRBSMaxGenToApply; rbsGenToBackup >= m_lRBSMaxGenToApply; --rbsGenToBackup )
    {
        if ( m_fRevertCancelled )
        {
            Error( ErrERRCheck( JET_errRBSRCRevertCancelled ) );
        }

        WCHAR wszRBSGenAbsSrcDirPath[ IFileSystemAPI::cchPathMax ];
        WCHAR wszRBSGenAbsFilePath[ IFileSystemAPI::cchPathMax ];
        Call( ErrRBSFilePathForGen_( m_wszRBSAbsRootDirPath, m_wszRBSBaseName, m_pinst->m_pfsapi, wszRBSGenAbsSrcDirPath, sizeof( wszRBSGenAbsSrcDirPath ), wszRBSGenAbsFilePath, sizeof( wszRBSGenAbsFilePath ), rbsGenToBackup ) );
        Call( ErrRBSDeleteAllFiles( m_pinst->m_pfsapi, wszRBSGenAbsSrcDirPath, NULL, fTrue ) );
    }

    // Fault injection to test continuing from JET_revertstateRemoveSnapshot
    Call( ErrFaultInjection( 50126 ) );

HandleError:
    if ( err < JET_errSuccess && m_pcprintfRevertTrace )
    {
        //  if we managed to at least getting tracing up, trace the issue ... 
        TraceFuncComplete( m_pcprintfRevertTrace, __FUNCTION__, err );
    }

    return err;
}

ERR CRBSRevertContext::ErrExecuteRevert( JET_GRBIT grbit, JET_RBSREVERTINFOMISC* prbsrevertinfo )
{
    Assert( m_lRBSMaxGenToApply > 0 );
    Assert( m_lRBSMinGenToApply > 0 );
    Assert( m_lRBSMinGenToApply <= m_lRBSMaxGenToApply );

    size_t  cchRequired;
    LittleEndian<QWORD> le_cPagesRevertedCurRBSGen;
    __int64 ftSinceLastUpdate = 0;
    RBSFILEHDR rbsfilehdr;
    SIGNATURE signDefault = { 0 };

    m_fRevertCancelled = fFalse;
    m_fExecuteRevertStarted = fTrue;
    m_ftRevertLastUpdate = UtilGetCurrentFileTime();

    ERR err = JET_errSuccess;

    // Lets keep old trace files around in case we want to investigate.
    Call( ErrBeginRevertTracing( fFalse ) );

    TraceFuncBegun( m_pcprintfRevertTrace, __FUNCTION__ );

    for ( IRBSDBRC irbsdbrc = 0; irbsdbrc <= m_irbsdbrcMaxInUse; ++irbsdbrc )
    {
        m_rgprbsdbrcAttached[ irbsdbrc ]->SetPrintfTrace( m_pcprintfRevertTrace );
        Call( m_rgprbsdbrcAttached[ irbsdbrc ]->ErrBeginTracingToIRS() );
        TraceFuncBegun( m_rgprbsdbrcAttached[ irbsdbrc ]->CprintfIRSTrace(), __FUNCTION__ );
    }

    switch ( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate )
    {
        case JET_revertstateNone:
        {
            // If a new revert, delete the previously backed up snapshots from previous revert,
            // to clear up that space and also to avoid conflicts under certain failure conditions.
            WCHAR wszRBSAbsBackupDirPath[ IFileSystemAPI::cchPathMax ];
            Call( ErrOSStrCbCopyW( wszRBSAbsBackupDirPath, sizeof( wszRBSAbsBackupDirPath ), m_wszRBSAbsRootDirPath ) );
            Call( ErrOSStrCbAppendW( wszRBSAbsBackupDirPath, sizeof( wszRBSAbsBackupDirPath ), wszRBSBackupDir ) );

            Call( ErrRBSDeleteAllFiles( m_pinst->m_pfsapi, wszRBSAbsBackupDirPath, NULL, fTrue ) );
        }
        case JET_revertstateInProgress:
        {
            // Set all required databases' state to JET_dbstateRevertInProgress
            for ( IRBSDBRC irbsdbrc = 0; irbsdbrc <= m_irbsdbrcMaxInUse; ++irbsdbrc )
            {
                Call( m_rgprbsdbrcAttached[ irbsdbrc ]->ErrSetDbstateForRevert( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate, m_ltRevertTo ) );
            }

            for ( LONG lRBSGen = m_lRBSMaxGenToApply; lRBSGen >= m_lRBSMinGenToApply; --lRBSGen )
            {
                Call( ErrRBSGenApply( lRBSGen, &rbsfilehdr, fFalse, fFalse ) );

                // Reset bit map state once we have completed applying a revert snapshot.
                for ( IRBSDBRC irbsdbrc = 0; irbsdbrc <= m_irbsdbrcMaxInUse; ++irbsdbrc )
                {
                    Call( m_rgprbsdbrcAttached[ irbsdbrc ]->ErrResetSbmPages( m_rgprbsdbrcAttached[ irbsdbrc ]->PpsbmDbPages() ) );
                }

                if ( m_fRevertCancelled )
                {
                    Error( ErrERRCheck( JET_errRBSRCRevertCancelled ) );
                }
            }

            // We will keep rest of the states the same and update only the state.
            // We might already be in copying logs state due to a crash. We will skip updating checkpoint again, if so.
            Call( ErrUpdateRevertCheckpoint( JET_revertstateCopingLogs, m_prbsrchk->rbsrchkfilehdr.le_rbsposCheckpoint, m_prbsrchk->rbsrchkfilehdr.tmCreateCurrentRBSGen, fFalse ) );
        }
        break;

        case JET_revertstateCopingLogs:
        case JET_revertstateBackupSnapshot:
        case JET_revertstateRemoveSnapshot:
        {
            // Since we have already reached the copying logs stage, we need to re-capture the database header alone from the lowest RBS gen being applied.
            // If we are in the stage where we are about to remove snapshot, we will use the backup directory as we might have already removed the RBS file when we failed/crashed.
            Call( ErrRBSGenApply( m_lRBSMinGenToApply, &rbsfilehdr, fTrue, m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateRemoveSnapshot ) );
        }
        break;
    }
    
    if ( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateCopingLogs )
    {
        if ( m_fRevertCancelled )
        {
            Error( ErrERRCheck( JET_errRBSRCRevertCancelled ) );
        }

        Call( ErrCopyRequiredLogsAfterRevert( rbsfilehdr.rbsfilehdr.le_lGenMinLogCopied, rbsfilehdr.rbsfilehdr.le_lGenMaxLogCopied ) );
    }

    if ( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateBackupSnapshot )
    {
        Call( ErrBackupRBSAfterRevert() );
    }

    if ( m_prbsrchk->rbsrchkfilehdr.le_rbsrevertstate == JET_revertstateRemoveSnapshot )
    {
        Call( ErrRemoveRBSAfterRevert() );
    }

    Call( ErrUpdateDbStatesAfterRevert( FRBSFormatFeatureEnabled( JET_rbsfvSignPrevRbsHdrFlush, &rbsfilehdr ) ? &rbsfilehdr.rbsfilehdr.signPrevRBSHdrFlush : &signDefault ) );

    ftSinceLastUpdate = UtilGetCurrentFileTime() - m_ftRevertLastUpdate;

    le_cPagesRevertedCurRBSGen  = m_cPagesRevertedCurRBSGen;

    memcpy( &prbsrevertinfo->logtimeRevertFrom, &m_prbsrchk->rbsrchkfilehdr.tmExecuteRevertBegin, sizeof( LOGTIME ) );
    prbsrevertinfo->cSecRevert          = m_prbsrchk->rbsrchkfilehdr.le_cSecInRevert + UtilConvertFileTimeToSeconds( ftSinceLastUpdate );
    prbsrevertinfo->cPagesReverted      = m_prbsrchk->rbsrchkfilehdr.le_cPagesReverted + le_cPagesRevertedCurRBSGen;
    prbsrevertinfo->lGenMinRevertStart  = m_prbsrchk->rbsrchkfilehdr.le_lGenMinRevertStart;
    prbsrevertinfo->lGenMaxRevertStart  = m_prbsrchk->rbsrchkfilehdr.le_lGenMaxRevertStart;
    prbsrevertinfo->lGenMinRevertEnd    = rbsfilehdr.rbsfilehdr.le_lGenMinLogCopied;
    prbsrevertinfo->lGenMaxRevertEnd    = rbsfilehdr.rbsfilehdr.le_lGenMaxLogCopied;

    TraceFuncComplete( m_pcprintfRevertTrace, __FUNCTION__, JET_errSuccess );

    // Set revert complete on all the databases' IRS raw files.
    for ( IRBSDBRC irbsdbrc = 0; irbsdbrc <= m_irbsdbrcMaxInUse; ++irbsdbrc )
    {
        TraceFuncComplete( m_rgprbsdbrcAttached[ irbsdbrc ]->CprintfIRSTrace(), __FUNCTION__, JET_errSuccess );
    }      

    Call( ErrRevertCheckpointCleanup() );    

HandleError:
    __int64 fileTimeRevertFrom  = ConvertLogTimeToFileTime( &m_prbsrchk->rbsrchkfilehdr.tmExecuteRevertBegin );
    __int64 fileTimeRevertTo    = ConvertLogTimeToFileTime( &m_ltRevertTo );

    WCHAR   wszTimeFrom[32];
    WCHAR   wszDateFrom[32];
    
    WCHAR   wszTimeTo[32];
    WCHAR   wszDateTo[32];

    ErrUtilFormatFileTimeAsTimeWithSeconds( fileTimeRevertFrom, wszTimeFrom, _countof(wszTimeFrom), &cchRequired );
    ErrUtilFormatFileTimeAsDate( fileTimeRevertFrom, wszDateFrom, _countof( wszDateFrom ), &cchRequired );

    ErrUtilFormatFileTimeAsTimeWithSeconds( fileTimeRevertTo, wszTimeTo, _countof(wszTimeTo), &cchRequired );
    ErrUtilFormatFileTimeAsDate( fileTimeRevertTo, wszDateTo, _countof( wszDateTo ), &cchRequired );

    const WCHAR* rgcwsz[] =
    {
        OSFormatW( L"%ws %ws", wszTimeFrom, wszDateFrom ),
        OSFormatW( L"%ws %ws", wszTimeTo, wszDateTo ),
        OSFormatW( L"%d", err )
    };

    UtilReportEvent(
        err == JET_errSuccess ? eventInformation : eventError,
        GENERAL_CATEGORY,
        err == JET_errSuccess ? RBSREVERT_EXECUTE_SUCCESS_ID : RBSREVERT_EXECUTE_FAILED_ID,
        3,
        rgcwsz,
        0,
        NULL,
        m_pinst );

    if ( err < JET_errSuccess )
    {
        if ( m_pcprintfRevertTrace )
        {
            //  if we managed to at least getting tracing up, trace the issue ... 
            TraceFuncComplete( m_pcprintfRevertTrace, __FUNCTION__, err );
        }

        // Set revert complete on all the databases' IRS raw files.
        for ( IRBSDBRC irbsdbrc = 0; irbsdbrc <= m_irbsdbrcMaxInUse; ++irbsdbrc )
        {
            if ( m_rgprbsdbrcAttached[ irbsdbrc ]->CprintfIRSTrace() )
            {
                TraceFuncComplete( m_rgprbsdbrcAttached[ irbsdbrc ]->CprintfIRSTrace(), __FUNCTION__, JET_errSuccess );
            }
        }
    }
    else
    {
        (*m_pcprintfRevertTrace)( 
            "Successfully completed reverting the database files from %ws %ws to %ws %ws. Total pages reverted %lld.\r\n", 
            wszTimeFrom,
            wszDateFrom,
            wszTimeTo,
            wszDateTo,
            m_prbsrchk->rbsrchkfilehdr.le_cPagesReverted + le_cPagesRevertedCurRBSGen );
    }

    return err;
}
