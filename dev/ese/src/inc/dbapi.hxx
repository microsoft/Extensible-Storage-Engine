// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

extern BOOL g_fExtentPageCountCacheCreateOverride;

ERR ErrDBOpenDatabase( PIB *ppib, __in PCWSTR wszDatabaseName, IFMP *pifmp, ULONG grbit );
ERR ErrDBCloseDatabase( PIB *ppib, IFMP ifmp, ULONG grbit );
ERR ErrUtilWriteAttachedDatabaseHeaders(    const INST* const           pinst,
                                            IFileSystemAPI *const       pfsapi,
                                            const WCHAR                 *wszFileName,
                                            FMP *const                  pfmp,
                                            IFileAPI *const             pfapi = NULL );
ERR ErrUtilWriteUnattachedDatabaseHeaders(  const INST* const               pinst,
                                            IFileSystemAPI *const           pfsapi,
                                            const WCHAR                     *wszFileName,
                                            DBFILEHDR                       *pdbfilehdr,
                                            IFileAPI *const                 pfapi = NULL,
                                            CFlushMapForUnattachedDb *const pfm = NULL,
                                            BOOL                            fResetRBSHdrFlush = fTrue );

void AssertDatabaseHeaderConsistent( const DBFILEHDR * const pdbfilehdr,
                                    const DWORD cbDbHeader,
                                    const DWORD cbPage );
    
ERR ErrDBOpenDatabaseByIfmp( PIB *ppib, IFMP ifmp );
#ifdef DEBUG
//  This is the no lies minimal DB we can have with ESE's current base schema, and it
//  is not meant to be used except for Assert()s and in compact and recovery.  So to 
//  facilitate that design while the function does exist in retail, we do not widely 
//  expose it.
CPG CpgDBDatabaseMinMin();
#endif


enum class FEATURECONTROL {
    fcNotSpecified = 0,
    fcDisableFromParam,
    fcEnableFromParam,
    fcDisableFromOverride,
    fcEnableFromOverride,
};

ERR ErrDBParseDbParams(
    _In_reads_opt_( csetdbparam )const JET_SETDBPARAM* const    rgsetdbparam,
    _In_ const ULONG                                            csetdbparam,
    _Out_opt_ CPG* const                                        pcpgDatabaseSizeMax,                // JET_dbparamDbSizeMaxPages
    _Out_opt_ ULONG* const                                      ppctCachePriority,                  // JET_dbparamCachePriority
    _Out_opt_ JET_GRBIT* const                                  pgrbitShrinkDatabaseOptions,        // JET_dbparamShrinkDatabaseOptions
    _Out_opt_ LONG* const                                       pdtickShrinkDatabaseTimeQuota,      // JET_dbparamShrinkDatabaseTimeQuota
    _Out_opt_ CPG* const                                        pcpgShrinkDatabaseSizeLimit,        // JET_dbparamShrinkDatabaseSizeLimit
    _Out_opt_ BOOL* const                                       pfLeakReclaimerEnabled,             // JET_dbparamLeakReclaimerEnabled
    _Out_opt_ LONG* const                                       pdtickLeakReclaimerTimeQuota,       // JET_dbparamLeakReclaimerTimeQuota
    _Out_opt_ FEATURECONTROL* const                             pfcMaintainExtentPageCountCache );  // JET_dbparamMaintainExtentPageCountCache

ERR ErrDBCreateDatabase(
    _In_ PIB                                                    *ppib,
    _In_ IFileSystemAPI                                         *pfsapiDest,
    _In_z_ const WCHAR                                          *wszDatabaseName,
    _Out_ IFMP                                                  *pifmp,
    _In_ DBID                                                   dbidGiven,
    _In_ CPG                                                    cpgPrimary,
    _In_ const BOOL                                             fSparseEnabledFile,
    _In_opt_ SIGNATURE                                          *psignDb,
    _In_reads_opt_( csetdbparam ) const JET_SETDBPARAM* const   rgsetdbparam,
    _In_ const ULONG                                            csetdbparam,
    _In_ const JET_GRBIT                                        grbit );

ERR ErrDBCreateDBFinish(
    PIB             *ppib,
    IFMP            ifmp,
    const ULONG     grbit );

bool FDBIsLVChunkSizeCompatible(
    const ULONG cbPage,
    const ULONG ulDAEVersion,
    const ULONG ulDAEUpdateMajor );

ERR ErrDBSetLastPage( PIB* const ppib, const IFMP ifmp );
ERR ErrDBSetupAttachedDB( INST *pinst, IFileSystemAPI *const pfsapi );
ERR ErrDBReadHeaderCheckConsistency( IFileSystemAPI *const pfsapi, FMP *pfmp );
ERR ErrDBGetDesiredVersion(
    _In_ const INST * const                 pinst,
    _In_ const DBFILEHDR_FIX * const        pdbfilehdr,
    _Out_ const FormatVersions ** const     ppfmtvers,
    _Out_ BOOL * const                      pfAllowPersistedFormat = NULL );
ERR ErrDBCloseAllDBs( PIB *ppib );

ERR ErrDBSetUserDbHeaderInfo(
    FMP * const pfmp,
    const ULONG cbdbinfomisc,
    JET_DBINFOMISC7 * pdbinfomisc );

ERR ErrDBRedoSetDbVersion(
    _In_    const INST* const               pinst,
    _In_    const IFMP                      ifmp,
    _In_    const DbVersion&                dbv );

ERR ErrDBReadAndCheckDBTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    const WCHAR * const wszFileName,
    __out_bcount( cbBuffer ) BYTE * const pbBuffer,
    const INT cbBuffer );
ERR ErrDBUpdateHeaderFromTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    DBFILEHDR   * const pdbfilehdr,
    const WCHAR * const wszDatabase,
    BOOL    fSkipMinLogChecks );
ERR ErrDBUpdateHeaderFromTrailer(
    const WCHAR * const wszDatabase,
    BOOL    fSkipMinLogChecks );
ERR ErrDBTryToZeroDBTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    const WCHAR * const wszFileName );

ERR ErrDBUpgradeDatabase(
    JET_SESID   sesid,
    const WCHAR *wszDatabaseName,
    JET_GRBIT   grbit );

ERR ErrDBAttachDatabaseForConvert(
    PIB         *ppib,
    IFMP        *pifmp,
    const CHAR  *szDatabaseName );

INLINE VOID DBSetOpenDatabaseFlag( PIB *ppib, IFMP ifmp )
{
    FMP *pfmp = &g_rgfmp[ifmp];
    DBID dbid = pfmp->Dbid();
    
    pfmp->GetWriteLatch( ppib );
    
    ((ppib)->rgcdbOpen[ dbid ]++);
    Assert( ((ppib)->rgcdbOpen[ dbid ] > 0 ) );

    pfmp->IncCPin();

    pfmp->ReleaseWriteLatch( ppib );
}

INLINE VOID DBResetOpenDatabaseFlag( PIB *ppib, IFMP ifmp )
{
    FMP *pfmp = &g_rgfmp[ ifmp ];
    DBID dbid = pfmp->Dbid();
    
    pfmp->GetWriteLatch( ppib );

    Assert( ((ppib)->rgcdbOpen[ dbid ] > 0 ) );
    ((ppib)->rgcdbOpen[ dbid ]--);

    Assert( pfmp->FWriteLatchByMe( ppib ) );
    pfmp->DecCPin();

    pfmp->ReleaseWriteLatch( ppib );
}

INLINE BOOL FLastOpen( PIB * const ppib, const IFMP ifmp )
{
    Assert( ifmp < g_ifmpMax );
    const DBID  dbid    = g_rgfmp[ifmp].Dbid();
    return ppib->rgcdbOpen[ dbid ] == 1;
}

INLINE BOOL FUserIfmp( const IFMP ifmp )
{
    Assert( ifmp < g_ifmpMax );
    const DBID  dbid    = g_rgfmp[ifmp].Dbid();
    return ( dbid > dbidTemp && dbid < dbidMax );
}

INLINE BOOL ErrDBCheckUserDbid( const IFMP ifmp )
{
    return ( ifmp < g_ifmpMax && FUserIfmp( ifmp ) ? JET_errSuccess : ErrERRCheck( JET_errInvalidDatabaseId ) );
}

INLINE VOID CheckDBID( PIB * const ppib, const IFMP ifmp )
{
    Assert( ifmp < g_ifmpMax );
    Assert( FPIBUserOpenedDatabase( ppib, g_rgfmp[ifmp].Dbid() ) );
}
    
VOID DBResetFMP( FMP *pfmp, const LOG *plog );

