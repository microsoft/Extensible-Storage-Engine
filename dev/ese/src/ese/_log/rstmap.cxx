// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "logstd.hxx"

ERR LOG::ErrReplaceRstMapEntryBySignature( const WCHAR *wszName, const SIGNATURE * pDbSignature )
{
    ERR err = JET_errSuccess;
    INT  irstmap;

    Assert ( pDbSignature );

    for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
    {
        if ( !m_rgrstmap[irstmap].fFileNotFound &&
            0 == memcmp( pDbSignature, &m_rgrstmap[irstmap].signDatabase, sizeof(SIGNATURE) ) )
        {
            Assert( NULL != m_rgrstmap[irstmap].wszNewDatabaseName );

            if ( NULL == m_rgrstmap[irstmap].wszDatabaseName
                || 0 != UtilCmpFileName( m_rgrstmap[irstmap].wszDatabaseName, wszName ) )
            {
                const ULONG     cbName              = (ULONG)( sizeof( WCHAR ) * ( LOSStrLengthW( wszName ) + 1 ) );
                WCHAR * const   wszSrcDatabaseName  = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbName ) );

                if ( NULL == wszSrcDatabaseName )
                    return ErrERRCheck( JET_errOutOfMemory );

                CallR( ErrOSStrCbCopyW( wszSrcDatabaseName, cbName, wszName ) );

                OSMemoryHeapFree( m_rgrstmap[irstmap].wszDatabaseName );
                m_rgrstmap[irstmap].wszDatabaseName = wszSrcDatabaseName;
                m_rgrstmap[irstmap].cbDatabaseName = cbName;
            }

            Assert( NULL != m_rgrstmap[irstmap].wszDatabaseName );
            Assert( NULL != m_rgrstmap[irstmap].wszNewDatabaseName );

            if ( 0 != UtilCmpFileName( m_rgrstmap[irstmap].wszDatabaseName, m_rgrstmap[irstmap].wszNewDatabaseName ) )
            {
                const UINT      csz     = 2;
                const WCHAR *   rgszT[csz];

                rgszT[0] = m_rgrstmap[irstmap].wszDatabaseName;
                rgszT[1] = m_rgrstmap[irstmap].wszNewDatabaseName;

                UtilReportEvent(
                        eventInformation,
                        LOGGING_RECOVERY_CATEGORY,
                        DB_LOCATION_CHANGE_DETECTED,
                        csz,
                        rgszT,
                        0,
                        NULL,
                        m_pinst );
            }

            return JET_errSuccess;
        }
    }

    return ErrERRCheck( JET_errFileNotFound );
}


ERR LOG::ErrReplaceRstMapEntryByName( const WCHAR *wszName, const SIGNATURE * pDbSignature )
{
    ERR err = JET_errSuccess;
    INT         irstmap;

    INT         irstmapBestFit          = m_irstmapMac;
    ULONG       cbBestFit               = 0;
    ULONG       cbName                  = sizeof( WCHAR ) * ( LOSStrLengthW( wszName ) + 1 );
    WCHAR *     wszSrcDatabaseName;

    Assert ( pDbSignature );

    for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
    {
        if ( m_rgrstmap[irstmap].fFileNotFound )
        {
            Assert ( m_rgrstmap[irstmap].wszDatabaseName );

            if (cbName >= m_rgrstmap[irstmap].cbDatabaseName &&
                cbBestFit < m_rgrstmap[irstmap].cbDatabaseName)
            {

                if ( 0 == UtilCmpFileName(m_rgrstmap[irstmap].wszDatabaseName, wszName + ( (cbName - m_rgrstmap[irstmap].cbDatabaseName ) / sizeof(WCHAR) ) ))
                {
                    cbBestFit = m_rgrstmap[irstmap].cbDatabaseName;
                    irstmapBestFit = irstmap;
                }
            }
        }
    }

        if ( irstmapBestFit >= m_irstmapMac )
        {
            return ErrERRCheck( JET_errFileNotFound );
        }
    
        Assert( m_rgrstmap[irstmapBestFit].fFileNotFound );

        if ( 0 != UtilCmpFileName(m_rgrstmap[irstmapBestFit].wszDatabaseName, wszName) )
        {
            if ( ( wszSrcDatabaseName = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbName ) ) ) == NULL )
                return ErrERRCheck( JET_errOutOfMemory );

            CallR( ErrOSStrCbCopyW( wszSrcDatabaseName, cbName, wszName ) );

            OSMemoryHeapFree( m_rgrstmap[irstmapBestFit].wszDatabaseName );
            m_rgrstmap[irstmapBestFit].wszDatabaseName = wszSrcDatabaseName;
            
        }

        m_rgrstmap[irstmapBestFit].signDatabase = (*pDbSignature);
        m_rgrstmap[irstmapBestFit].fFileNotFound = fFalse;
        return JET_errSuccess;
}

INT LOG::IrstmapGetRstMapEntry( const WCHAR *wszName )
{
    INT  irstmap;
    BOOL fFound = fFalse;

    Assert( wszName );

    for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
    {
        WCHAR           wszPathT[IFileSystemAPI::cchPathMax];
        WCHAR           wszFNameT[IFileSystemAPI::cchPathMax] = { 0 };
        const WCHAR *   wszT = NULL;
        const WCHAR *   wszRst;

        if ( FExternalRestore() || !FHardRestore() )
        {
            
            wszT = wszName;
            wszRst = m_rgrstmap[irstmap].wszDatabaseName;
        }
        else
        {
            
            CallS( m_pinst->m_pfsapi->ErrPathParse( wszName, wszPathT, wszFNameT, wszPathT ) );
            wszT = wszFNameT;
            wszRst = m_rgrstmap[irstmap].wszGenericName;
        }

        if ( wszRst && _wcsicmp( wszRst, wszT ) == 0 )
        {
            fFound = fTrue;
            break;
        }
    }
    if ( !fFound )
        return -1;
    else
        return irstmap;
}


INT LOG::IrstmapSearchNewName( _In_z_ const WCHAR *wszName, _Deref_out_opt_z_ WCHAR ** pwszRstmapDbName )
{
    INT  irstmap;

    Assert( wszName );

    for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
    {
        if ( m_rgrstmap[irstmap].wszNewDatabaseName && 0 == UtilCmpFileName( m_rgrstmap[irstmap].wszNewDatabaseName, wszName ) )
        {
            Assert ( m_rgrstmap[irstmap].wszDatabaseName );
            if ( pwszRstmapDbName != NULL )
            {
                *pwszRstmapDbName = m_rgrstmap[irstmap].wszDatabaseName;
            }
            return irstmap;
        }
    }

    if ( pwszRstmapDbName != NULL )
    {
        *pwszRstmapDbName = NULL;
    }
    return -1;
}

VOID LOG::FreeRstmap( VOID )
{
    if ( NULL != m_rgrstmap )
    {
        RSTMAP* prstmapCur = m_rgrstmap;
        RSTMAP* const prstmapMax = m_rgrstmap + m_irstmapMac;

        while ( prstmapCur < prstmapMax )
        {
            if ( prstmapCur->wszDatabaseName )
            {
                OSMemoryHeapFree( prstmapCur->wszDatabaseName );
            }

            if ( prstmapCur->wszNewDatabaseName )
            {
                OSMemoryHeapFree( prstmapCur->wszNewDatabaseName );
            }

            if ( prstmapCur->wszGenericName )
            {
                OSMemoryHeapFree( prstmapCur->wszGenericName );
            }

            JET_SETDBPARAM* psetdbparamCur = prstmapCur->rgsetdbparam;
            if ( psetdbparamCur )
            {
                JET_SETDBPARAM* const psetdbparamMax = prstmapCur->rgsetdbparam + prstmapCur->csetdbparam;
                while ( psetdbparamCur < psetdbparamMax )
                {
                    if ( psetdbparamCur->pvParam )
                    {
                        OSMemoryHeapFree( psetdbparamCur->pvParam );
                    }
                    psetdbparamCur++;
                }
            }

            if ( prstmapCur->rgsetdbparam )
            {
                OSMemoryHeapFree( prstmapCur->rgsetdbparam );
            }

            prstmapCur++;
        }

        OSMemoryHeapFree( m_rgrstmap );
        m_rgrstmap = NULL;
    }

    m_irstmapMac = 0;
}


ERR LOG::ErrBuildRstmapForRestore( PCWSTR wszRestorePath )
{
    ERR             err         = JET_errSuccess;
    INT             irstmap     = 0;
    INT             irstmapMac  = 0;
    RSTMAP*         rgrstmap    = NULL;
    RSTMAP*         prstmap;
    WCHAR           wszSearch[IFileSystemAPI::cchPathMax];
    WCHAR           wszFileName[IFileSystemAPI::cchPathMax];
    WCHAR           wszFile[IFileSystemAPI::cchPathMax];
    WCHAR           wszT[IFileSystemAPI::cchPathMax];
    IFileFindAPI*   pffapi      = NULL;

    
    Assert( FOSSTRTrailingPathDelimiterW( wszRestorePath ) );
    Assert( LOSStrLengthW( wszRestorePath ) + LOSStrLengthW( L"*" ) + LOSStrLengthW( wszPatExt ) < IFileSystemAPI::cchPathMax - 1 );
    if ( LOSStrLengthW( wszRestorePath ) + LOSStrLengthW( L"*" ) + LOSStrLengthW( wszPatExt ) >= IFileSystemAPI::cchPathMax - 1 )
    {
        Error( ErrERRCheck( JET_errInvalidPath ) );
    }
    Call( ErrOSStrCbCopyW( wszSearch, sizeof(wszSearch), wszRestorePath ) );
    Call( ErrOSStrCbAppendW( wszSearch, sizeof(wszSearch), L"*" ) );
    Call( ErrOSStrCbAppendW( wszSearch, sizeof(wszSearch), wszPatExt ) );


    Call( m_pinst->m_pfsapi->ErrFileFind( wszSearch, &pffapi ) );
    while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
    {
        
        if ( irstmap + 1 >= irstmapMac )
        {
            Alloc( prstmap = static_cast<RSTMAP *>( PvOSMemoryHeapAlloc( sizeof(RSTMAP) * ( irstmap + 8 ) ) ) );
            memset( prstmap + irstmap, 0, sizeof( RSTMAP ) * 8 );
            if ( rgrstmap != NULL )
            {
                UtilMemCpy( prstmap, rgrstmap, sizeof(RSTMAP) * irstmap );
                OSMemoryHeapFree( rgrstmap );
            }
            rgrstmap = prstmap;
            irstmapMac += 8;
        }

        
        Call( pffapi->ErrPath( wszFileName ) );
        Call( m_pinst->m_pfsapi->ErrPathParse( wszFileName, wszT, wszFile, wszT ) );
        prstmap = rgrstmap + irstmap;
        const ULONG cbGenericNameT1 = sizeof( WCHAR ) * ( LOSStrLengthW( wszFile ) + 1 );
        Alloc( prstmap->wszGenericName = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbGenericNameT1 ) ) );
        OSStrCbCopyW( prstmap->wszGenericName, cbGenericNameT1, wszFile );
        irstmap++;
    }

    if ( JET_errFileNotFound != err )
    {
        Call( err );
    }

    if ( 0 == irstmap )
    {
        Error( ErrERRCheck( JET_errPatchFileMissing ) );
    }

    m_irstmapMac = irstmap;
    m_rgrstmap = rgrstmap;

    delete pffapi;
    return JET_errSuccess;

HandleError:
    while ( rgrstmap && --irstmap >= 0 )
    {
        OSMemoryHeapFree( rgrstmap[ irstmap ].wszGenericName );
    }
    OSMemoryHeapFree( rgrstmap );
    delete pffapi;
    return err;
}

ERR ErrLGCheckDir( IFileSystemAPI *const pfsapi, __inout_bcount(cbDir) const PWSTR wszDir, ULONG cbDir, __in_opt PCWSTR wszSearch );
ERR ErrLGCheckDBFiles(
    INST *pinst,
    RSTMAP * pDbMapEntry,
    INT genLow,
    INT genHigh,
    LGPOS *plgposSnapshotRestore = NULL );

#define cRestoreStatusPadding   0.10

ERR LOG::ErrGetDestDatabaseName(
    const WCHAR *wszDatabaseName,
    PCWSTR wszRestorePath,
    PCWSTR wszNewDestination,
    INT *pirstmap,
    LGSTATUSINFO *plgstat )
{
    ERR     err;
    WCHAR   wszDirT[IFileSystemAPI::cchPathMax];
    WCHAR   wszFNameT[IFileSystemAPI::cchPathMax];
    WCHAR   wszExtT[IFileSystemAPI::cchPathMax];
    WCHAR   wszRestoreT[IFileSystemAPI::cchPathMax];
    WCHAR   wszT[IFileSystemAPI::cchPathMax];
    WCHAR   *wszNewDatabaseName;
    INT     irstmap;

    Assert( wszDatabaseName );

    irstmap = IrstmapGetRstMapEntry( wszDatabaseName );
    *pirstmap = irstmap;

    if ( irstmap < 0 )
    {
        return( ErrERRCheck( JET_errFileNotFound ) );
    }
    else if ( m_rgrstmap[irstmap].fDestDBReady )
        return JET_errSuccess;

    
    CallS( m_pinst->m_pfsapi->ErrPathParse( wszDatabaseName, wszDirT, wszFNameT, wszExtT ) );
    CallR( ErrOSStrCbAppendW( wszFNameT, sizeof(wszFNameT), wszExtT ) );

    
    if ( LOSStrLengthW( wszRestorePath ) >= IFileSystemAPI::cchPathMax - 1 )
    {
        return ( ErrERRCheck( JET_errInvalidPath ) );
    }
    CallR( ErrOSStrCbCopyW( wszRestoreT, sizeof(wszRestoreT), wszRestorePath ) );
    if ( ErrLGCheckDir( m_pinst->m_pfsapi, wszRestoreT, sizeof(wszRestoreT), wszFNameT ) != JET_errBackupDirectoryNotEmpty )
        return( ErrERRCheck( JET_errFileNotFound ) );

    
    if ( FExternalRestore() )
    {
        Assert( _wcsicmp( m_rgrstmap[irstmap].wszDatabaseName, wszDatabaseName ) == 0 );
        Assert( irstmap < m_irstmapMac );

        wszNewDatabaseName = m_rgrstmap[irstmap].wszNewDatabaseName;
    }
    else
    {
        WCHAR       *wszSrcDatabaseName;
        WCHAR       wszFullPathT[IFileSystemAPI::cchPathMax];

        
        const ULONG cbSrcDatabaseNameT = sizeof( WCHAR ) * ( LOSStrLengthW( wszDatabaseName ) + 1 );
        AllocR( wszSrcDatabaseName = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbSrcDatabaseNameT ) ) );
        OSStrCbCopyW( wszSrcDatabaseName, cbSrcDatabaseNameT, wszDatabaseName );
        m_rgrstmap[irstmap].wszDatabaseName = wszSrcDatabaseName;

        
        if ( wszNewDestination[0] != L'\0' )
        {
            const ULONG cbNewDatabaseNameT = sizeof( WCHAR ) * ( LOSStrLengthW( wszNewDestination ) + LOSStrLengthW( wszFNameT ) + 1 );
            AllocR( wszNewDatabaseName = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbNewDatabaseNameT  ) ) );
            OSStrCbCopyW( wszNewDatabaseName, cbNewDatabaseNameT, wszNewDestination );
            OSStrCbAppendW( wszNewDatabaseName, cbNewDatabaseNameT, wszFNameT );
        }
        else
        {
            const ULONG cbNewDatabaseNameT = sizeof( WCHAR ) * ( LOSStrLengthW( wszDatabaseName ) + 1 );
            AllocR( wszNewDatabaseName = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbNewDatabaseNameT ) ) );
            OSStrCbCopyW( wszNewDatabaseName, cbNewDatabaseNameT, wszDatabaseName );
        }
        m_rgrstmap[irstmap].wszNewDatabaseName = wszNewDatabaseName;

        
        CallS( m_pinst->m_pfsapi->ErrPathParse( wszDatabaseName, wszDirT, wszFNameT, wszExtT ) );
        
        if ( LOSStrLengthW( wszRestorePath ) + LOSStrLengthW(wszFNameT) + LOSStrLengthW(wszExtT) >= IFileSystemAPI::cchPathMax - 1 )
        {
            return ( ErrERRCheck( JET_errInvalidPath ) );
        }
        CallR( ErrOSStrCbCopyW( wszT, sizeof(wszT), wszRestorePath ) );
        CallR( ErrOSStrCbAppendW( wszT, sizeof(wszT), wszFNameT ) );

        if ( wszExtT[0] != L'\0' )
        {
            CallR( ErrOSStrCbAppendW( wszT, sizeof(wszT), wszExtT ) );
        }

        CallR( ErrUtilPathExists( m_pinst->m_pfsapi, wszT, wszFullPathT ) );

        if ( _wcsicmp( wszFullPathT, wszNewDatabaseName ) != 0 )
        {
            CallR( m_pinst->m_pfsapi->ErrFileCopy( wszT, wszNewDatabaseName, fTrue ) );
        }

        LONG lgenLowRestore, lgenHighRestore;
        LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
        CallR( ErrLGCheckDBFiles( m_pinst, m_rgrstmap + irstmap, lgenLowRestore, lgenHighRestore ) );
        Assert( FRstmapCheckDuplicateSignature( ) );
    }

    m_rgrstmap[irstmap].fDestDBReady = fTrue;
    *pirstmap = irstmap;

    if ( plgstat != NULL )
    {
        JET_SNPROG  *psnprog = &plgstat->snprog;
        ULONG       cPercentSoFar;
        ULONG       cDBCopyEstimate;

        cDBCopyEstimate = max((ULONG)(plgstat->cGensExpected * cRestoreStatusPadding / m_irstmapMac), 1);
        plgstat->cGensExpected += cDBCopyEstimate;
        plgstat->cGensSoFar += cDBCopyEstimate;

        cPercentSoFar = (ULONG)( ( cDBCopyEstimate * 100 ) / plgstat->cGensExpected );
        Assert( cPercentSoFar > 0  &&  cPercentSoFar < 100 );
        Assert( cPercentSoFar <= ( cDBCopyEstimate * 100) / plgstat->cGensExpected );

        if ( cPercentSoFar > psnprog->cunitDone )
        {
            Assert( psnprog->cbStruct == sizeof(JET_SNPROG)  &&
                    psnprog->cunitTotal == 100 );
            psnprog->cunitDone = cPercentSoFar;
            ( plgstat->pfnCallback )( JET_snpRestore, JET_sntProgress, psnprog, plgstat->pvCallbackContext );
        }
    }

    return JET_errSuccess;
}

ERR ErrDBFormatFeatureEnabled_( const JET_ENGINEFORMATVERSION efvFormatFeature, const DbVersion& dbvCurrentFromFile );

ERR ErrRstmapSoftCheckDBFiles( INST *pinst, RSTMAP * pDbMapEntry )
{
    ERR err;
    DBFILEHDR_FIX *pdbfilehdrDb;

#ifdef DEBUG
    SIGNATURE signEmpty;
    memset( &signEmpty, '\0', sizeof(signEmpty) );
    Assert( 0 == memcmp ( &pDbMapEntry->signDatabase, &signEmpty , sizeof(SIGNATURE) ) );
#endif

    Assert ( pDbMapEntry );
    const WCHAR * wszDatabase = pDbMapEntry->wszNewDatabaseName;

    
    pdbfilehdrDb = (DBFILEHDR_FIX * )PvOSMemoryPageAlloc( g_cbPage, NULL );
    if ( pdbfilehdrDb == NULL )
        return ErrERRCheck( JET_errOutOfMemory );

    err = ErrUtilPathExists( pinst->m_pfsapi, wszDatabase );

    if ( err >= JET_errSuccess )
    {
        err = ErrUtilReadShadowedHeader(
                            pinst,
                            pinst->m_pfsapi,
                            wszDatabase,
                            (BYTE*)pdbfilehdrDb,
                            g_cbPage,
                            OffsetOf( DBFILEHDR_FIX, le_cbPageSize ),
                            urhfReadOnly );

        Assert( JET_errFileNotFound != err );
        Assert( JET_errInvalidPath != err );
    }

    if ( ( err == JET_errFileNotFound ) ||
        ( err < JET_errSuccess && pDbMapEntry->grbit & ( JET_bitRestoreMapIgnoreWhenMissing | JET_bitRestoreMapOverwriteOnCreate ) ) )
    {

        if ( !pDbMapEntry->wszDatabaseName || LOSStrLengthW( pDbMapEntry->wszDatabaseName ) < 1 )
        {
            Assert( fFalse );
            err = ErrERRCheck( JET_errDatabaseInvalidName );
        }
        else
        {
            pDbMapEntry->fFileNotFound = fTrue;
            err = JET_errSuccess;
        }

        goto HandleError;
    }
    else
    {
        pDbMapEntry->fFileNotFound = fFalse;
    }

    if ( err < 0 )
    {
        if ( FErrIsDbHeaderCorruption( err ) )
        {
            OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"2ff045a8-b617-467b-8626-1f55a6193b36" );
            err = ErrERRCheck( JET_errDatabaseCorrupted );
        }
        Call( err );
    }

    memcpy ( &pDbMapEntry->signDatabase, &pdbfilehdrDb->signDb , sizeof(SIGNATURE) );
    memcpy ( &pDbMapEntry->signDatabaseHdrFlush, &pdbfilehdrDb->signDbHdrFlush , sizeof(SIGNATURE) );
    memcpy ( &pDbMapEntry->signRBSHdrFlush, &pdbfilehdrDb->signRBSHdrFlush , sizeof(SIGNATURE) );

    pDbMapEntry->dbstate = pdbfilehdrDb->le_dbstate;
    pDbMapEntry->lGenMinRequired = pdbfilehdrDb->le_lGenMinRequired;
    pDbMapEntry->lGenMaxRequired = pdbfilehdrDb->le_lGenMaxRequired;
    pDbMapEntry->lGenLastConsistent = pdbfilehdrDb->le_lgposConsistent.le_lGeneration;
    pDbMapEntry->lGenLastAttach = pdbfilehdrDb->le_lgposAttach.le_lGeneration;
    pDbMapEntry->fRBSFeatureEnabled = ErrDBFormatFeatureEnabled_( JET_efvRevertSnapshot, pdbfilehdrDb->Dbv() ) >= JET_errSuccess;

    if ( attribDb != pdbfilehdrDb->le_attrib )
    {
        if ( pDbMapEntry->grbit & ( JET_bitRestoreMapIgnoreWhenMissing | JET_bitRestoreMapOverwriteOnCreate ) )
        {
            pDbMapEntry->fFileNotFound = fTrue;
            err = JET_errSuccess;
        }
        else
        {
            Assert( fFalse );
            OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"9ae9463b-24a0-435f-af3f-fe6652286ef0" );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }

HandleError:
    if ( err < 0 )
    {
        const UINT      csz     = 2;
        const WCHAR *   rgszT[csz];
        WCHAR           szErr[16];

        OSStrCbFormatW( szErr, sizeof( szErr ), L"%d", err );
        rgszT[0] = wszDatabase;
        rgszT[1] = szErr;

        UtilReportEvent(
                eventError,
                LOGGING_RECOVERY_CATEGORY,
                RSTMAP_FAIL_ID,
                csz,
                rgszT,
                0,
                NULL,
                pinst );
    }
    OSMemoryPageFree( pdbfilehdrDb );

    return err;
}

VOID LOG::LoadCheckpointGenerationFromRstmap( LONG * const plgenLow )
{
    BOOL fAtLeastOneDbFound = fFalse;
    LONG lgenLow = lMax;

    *plgenLow = 0;
    for ( INT irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
    {
        const RSTMAP * const rstmap = m_rgrstmap + irstmap;
        if ( !rstmap->fFileNotFound )
        {
            fAtLeastOneDbFound = fTrue;
            if ( rstmap->dbstate == JET_dbstateCleanShutdown )
            {
                lgenLow = min( lgenLow, rstmap->lGenLastConsistent );
            }
            else
            {
                lgenLow = min( lgenLow, rstmap->lGenMinRequired );
            }
        }
        else if ( !( rstmap->grbit & JET_bitRestoreMapIgnoreWhenMissing ) )
        {
            return;
        }
    }

    if ( fAtLeastOneDbFound )
    {
        *plgenLow = lgenLow;
    }

    Assert( *plgenLow <= lGenerationMax );
}

VOID LOG::LoadHighestLgenAttachFromRstmap( LONG * const plgenAttach )
{
    LONG lgenAttach = 0;

    for ( INT irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
    {
        const RSTMAP * const rstmap = m_rgrstmap + irstmap;
        if ( !rstmap->fFileNotFound && ( rstmap->grbit & JET_bitRestoreMapFailWhenMissing ) )
        {
            lgenAttach = max( lgenAttach, max( rstmap->lGenLastAttach, rstmap->lGenLastConsistent ) );
        }
    }

    *plgenAttach = lgenAttach;

    Assert( *plgenAttach <= lGenerationMax );
}

VOID LOG::LoadRBSGenerationFromRstmap( LONG * const plgenLow, LONG * plgenHigh )
{
    BOOL fAtLeastOneDbFound = fFalse;
    LONG lgenLow = lMax;
    LONG lgenHigh = lMin;

    *plgenLow = 0;
    *plgenHigh = 0;
    for ( INT irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
    {
        const RSTMAP * const rstmap = m_rgrstmap + irstmap;

        if ( !rstmap->fFileNotFound && ( rstmap->dbstate == JET_dbstateDirtyShutdown || rstmap->dbstate == JET_dbstateDirtyAndPatchedShutdown ) )
        {
            fAtLeastOneDbFound = fTrue;
            lgenLow = min( lgenLow, rstmap->lGenMinRequired );
            lgenHigh = max ( lgenHigh, rstmap->lGenMaxRequired );
        }
    }

    if ( fAtLeastOneDbFound )
    {
        *plgenLow = lgenLow;
        *plgenHigh = lgenHigh;
    }

    Assert( *plgenLow <= lGenerationMax );
    Assert( *plgenHigh <= lGenerationMax );
}

BOOL LOG::FRBSFeatureEnabledFromRstmap()
{
    BOOL fRBSFeatureEnabled = fFalse;

    for ( INT irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
    {
        const RSTMAP * const rstmap = m_rgrstmap + irstmap;
        if ( rstmap->fFileNotFound )
        {
            if ( rstmap->grbit & JET_bitRestoreMapFailWhenMissing  )
            {
                return fFalse;
            }

            continue;
        }
        else if ( !rstmap->fRBSFeatureEnabled || rstmap->dbstate == JET_dbstateIncrementalReseedInProgress || rstmap->dbstate == JET_dbstateRevertInProgress )
        {
            return fFalse;
        }
        else
        {
            fRBSFeatureEnabled = fTrue;
        }
    }

    return fRBSFeatureEnabled;
}

ERR LOG::ErrBuildRstmapForSoftRecovery( const JET_RSTMAP2_W * const rgjrstmap, const INT cjrstmap )
{
    ERR         err;
    INT         irstmap = 0;
    RSTMAP      *rgrstmap;

    if ( cjrstmap > ( dbidMax * 2 ) || cjrstmap < 0 )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( cjrstmap == 0 )
    {
        m_irstmapMac = 0;
        m_rgrstmap = NULL;
        return JET_errSuccess;
    }

    AllocR( rgrstmap = static_cast<RSTMAP *>( PvOSMemoryHeapAlloc( sizeof(RSTMAP) * cjrstmap ) ) );
    memset( rgrstmap, 0, sizeof(RSTMAP) * cjrstmap );

    m_irstmapMac    = cjrstmap;
    m_rgrstmap      = rgrstmap;

    for ( irstmap = 0; irstmap < cjrstmap; irstmap++ )
    {
        const JET_RSTMAP2_W * const pjrstmap        = rgjrstmap + irstmap;
        RSTMAP * const              prstmap         = rgrstmap + irstmap;
        ULONG                       cchDbName       = 0;
        WCHAR                       wszFullName[ IFileSystemAPI::cchPathMax ];

        if ( NULL == pjrstmap->szNewDatabaseName )
        {
            Error( ErrERRCheck( JET_errDatabaseInvalidName ) );
        }

        err = ErrUtilPathComplete( m_pinst->m_pfsapi, pjrstmap->szNewDatabaseName, wszFullName, fFalse );
        if ( err < JET_errSuccess )
        {
            Call( ErrOSStrCbCopyW( wszFullName, sizeof(wszFullName), pjrstmap->szNewDatabaseName ) );
        }

        if ( pjrstmap->grbit & ~( JET_bitRestoreMapIgnoreWhenMissing | JET_bitRestoreMapFailWhenMissing | JET_bitRestoreMapOverwriteOnCreate ) )
        {
            Error( ErrERRCheck( JET_errInvalidGrbit ) );
        }

        const ULONG cchNewDbName    = LOSStrLengthW( wszFullName );
        if ( cchNewDbName < 1 )
        {
            Error( ErrERRCheck( JET_errDatabaseInvalidName ) );
        }

        const ULONG cbNewDbName     = sizeof( WCHAR ) * ( cchNewDbName + 1 );
        Alloc( prstmap->wszNewDatabaseName = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbNewDbName ) ) );
        OSStrCbCopyW( prstmap->wszNewDatabaseName, cbNewDbName, wszFullName );

        if ( NULL != pjrstmap->szDatabaseName )
        {
            err = ErrUtilPathComplete( m_pinst->m_pfsapi, pjrstmap->szDatabaseName, wszFullName, fFalse );
            if ( err < JET_errSuccess )
            {
                Call( ErrOSStrCbCopyW( wszFullName, sizeof(wszFullName), pjrstmap->szDatabaseName ) );
                err = JET_errSuccess;
            }
            cchDbName = LOSStrLengthW( wszFullName );
        }

        if ( cchDbName > 0 )
        {
            prstmap->cbDatabaseName = sizeof( WCHAR ) * ( cchDbName + 1 );
            Alloc( prstmap->wszDatabaseName = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( prstmap->cbDatabaseName ) ) );
            Call( ErrOSStrCbCopyW( prstmap->wszDatabaseName, prstmap->cbDatabaseName, wszFullName ) );
        }
        else
        {
            prstmap->cbDatabaseName = cbNewDbName;
            Alloc( prstmap->wszDatabaseName = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbNewDbName ) ) );
            Call( ErrOSStrCbCopyW( prstmap->wszDatabaseName, cbNewDbName, prstmap->wszNewDatabaseName ) );
        }

        if ( ( pjrstmap->rgsetdbparam == NULL ) && ( pjrstmap->csetdbparam > 0 ) )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        if ( pjrstmap->csetdbparam > 0 )
        {
            Alloc( prstmap->rgsetdbparam = static_cast<JET_SETDBPARAM *>( PvOSMemoryHeapAlloc( sizeof(JET_SETDBPARAM) * pjrstmap->csetdbparam ) ) );
            for ( ULONG isetdbparam = 0; isetdbparam < pjrstmap->csetdbparam; isetdbparam++ )
            {
                const JET_SETDBPARAM* const pjsetdbparamSrc = &pjrstmap->rgsetdbparam[ isetdbparam ];
                JET_SETDBPARAM* const pjsetdbparamDst = &prstmap->rgsetdbparam[ isetdbparam ];

                if ( pjsetdbparamSrc->cbParam > 0 )
                {
                    Alloc( pjsetdbparamDst->pvParam = PvOSMemoryHeapAlloc( pjsetdbparamSrc->cbParam ) );
                    UtilMemCpy( pjsetdbparamDst->pvParam, pjsetdbparamSrc->pvParam, pjsetdbparamSrc->cbParam );
                }

                pjsetdbparamDst->dbparamid = pjsetdbparamSrc->dbparamid;
                pjsetdbparamDst->cbParam = pjsetdbparamSrc->cbParam;
                prstmap->csetdbparam++;
            }
        }

        prstmap->grbit = pjrstmap->grbit;

        prstmap->fFileNotFound = fFalse;
        prstmap->fDestDBReady = fTrue;

        Call( ErrRstmapSoftCheckDBFiles( m_pinst, prstmap ) );
    }

    Assert( m_irstmapMac == irstmap );
    Assert( m_rgrstmap == rgrstmap );

    return JET_errSuccess;

HandleError:
    Assert( rgrstmap != NULL );
    FreeRstmap( );

    Assert( m_irstmapMac == 0 );
    Assert( m_rgrstmap == NULL );

    return err;
}

BOOL LOG::FRstmapCheckDuplicateSignature(  )
{
    INT  irstmap;

    for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
    {
        INT  irstmapSearch;

        if ( !m_rgrstmap[irstmap].wszDatabaseName )
            continue;

        for ( irstmapSearch = irstmap + 1; irstmapSearch < m_irstmapMac; irstmapSearch++ )
        {
            if ( !m_rgrstmap[irstmap].wszNewDatabaseName )
                continue;

            if ( 0 == memcmp(   &m_rgrstmap[irstmap].signDatabase,
                                &m_rgrstmap[irstmapSearch].signDatabase,
                                sizeof(SIGNATURE) ) )
            {
                return fFalse;
            }
        }
    }

    return fTrue;
}

ERR LOG::ErrBuildRstmapForExternalRestore( JET_RSTMAP_W *rgjrstmap, INT cjrstmap )
{
    ERR             err;
    INT             irstmap = 0;
    RSTMAP          *rgrstmap;
    RSTMAP          *prstmap;
    JET_RSTMAP_W    *pjrstmap;

    if ( cjrstmap > ( dbidMax * 2 ) || cjrstmap < 0 )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    AllocR( rgrstmap = static_cast<RSTMAP *>( PvOSMemoryHeapAlloc( sizeof(RSTMAP) * cjrstmap ) ) );
    memset( rgrstmap, 0, sizeof( RSTMAP ) * cjrstmap );

    for ( irstmap = 0; irstmap < cjrstmap; irstmap++ )
    {
        pjrstmap = rgjrstmap + irstmap;
        prstmap = rgrstmap + irstmap;
        const ULONG cbDatabaseNameT = sizeof( WCHAR ) * ( LOSStrLengthW( pjrstmap->szDatabaseName ) + 1 );
        Alloc( prstmap->wszDatabaseName = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbDatabaseNameT  ) ) );
        OSStrCbCopyW( prstmap->wszDatabaseName, cbDatabaseNameT, pjrstmap->szDatabaseName );

        const ULONG cbNewDatabaseNameT = sizeof( WCHAR ) * ( LOSStrLengthW( pjrstmap->szNewDatabaseName ) + 1 );
        Alloc( prstmap->wszNewDatabaseName = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbNewDatabaseNameT ) ) );
        OSStrCbCopyW( prstmap->wszNewDatabaseName, cbNewDatabaseNameT, pjrstmap->szNewDatabaseName );

        prstmap->fDestDBReady = fTrue;
    }

    m_irstmapMac = irstmap;
    m_rgrstmap = rgrstmap;

    return JET_errSuccess;

HandleError:
    Assert( rgrstmap != NULL );
    FreeRstmap( );

    Assert( m_irstmapMac == 0 );
    Assert( m_rgrstmap == NULL );

    return err;
}


