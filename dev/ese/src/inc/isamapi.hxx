// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef ISAMAPI_H
#error ISAMAPI.H already included
#endif  
#define ISAMAPI_H

#define ISAMAPI

struct CPCOL
{
    JET_COLUMNID columnidSrc;
    JET_COLUMNID columnidDest;
};

struct STATUSINFO
{
    JET_SESID       sesid;
    JET_PFNSTATUS   pfnStatus;
    JET_SNP         snp;
    JET_SNT         snt;
    ULONG           cunitTotal;
    ULONG           cunitDone;
    ULONG           cunitPerProgression;
    ULONG           cunitProjected;

    BOOL            fDumpStats;
    FILE            *hfCompactStats;
    ULONG           timerInitDB;
    ULONG           timerCopyDB;
    ULONG           timerInitTable;
    ULONG           timerCopyRecords;
    ULONG           timerRebuildIndexes;
    ULONG           timerCopyTable;

    ULONG           cDBPagesOwned;
    ULONG           cDBPagesAvail;
    char            *szTableName;
    ULONG           cTableFixedVarColumns;
    ULONG           cTableTaggedColumns;
    ULONG           cTableInitialPages;
    ULONG           cTablePagesOwned;
    ULONG           cTablePagesAvail;
    QWORD           cbRawData;
    QWORD           cbRawDataLV;
    ULONG           cLeafPagesTraversed;
    ULONG           cLVPagesTraversed;
    ULONG           cSecondaryIndexes;
    ULONG           cDeferredSecondaryIndexes;
};


extern "C" {

ERR ErrIsamResetCounter( JET_SESID sesid, INT CounterType );
ERR ErrIsamGetCounter( JET_SESID sesid, INT CounterType, LONG *plValue );
ERR ErrIsamSetSessionContext( JET_SESID sesid, DWORD_PTR dwContext );
ERR ErrIsamResetSessionContext( JET_SESID sesid );

ERR ISAMAPI ErrIsamSystemInit();

VOID ISAMAPI IsamSystemTerm();

ERR ErrIsamInit( JET_INSTANCE inst, JET_GRBIT grbit );

ERR ErrIsamTerm( JET_INSTANCE inst, JET_GRBIT grbit );

ERR ErrIsamBeginSession( JET_INSTANCE inst, JET_SESID *pvsesid );

ERR ErrIsamEndSession( JET_SESID sesid, JET_GRBIT grbit );

ERR ErrIsamGetSessionInfo(
    JET_SESID                       sesid,
    _Out_bytecap_(cbMax) VOID *     pvResult,
    const ULONG                     cbMax,
    const ULONG                     ulInfoLevel );

ERR ErrIsamIdle( JET_SESID sesid, JET_GRBIT grbit );

ERR ErrIsamAttachDatabase(
    JET_SESID                   sesid,
    __in PCWSTR                 wszDatabaseName,
    const BOOL                  fAllowTrimDBTask,
    const JET_SETDBPARAM* const rgsetdbparam,
    const ULONG                 csetdbparam,
    const JET_GRBIT             grbit );

ERR ErrIsamDetachDatabase(
    JET_SESID               sesid,
    IFileSystemAPI* const   pfsapiDB,
    const WCHAR             *wszFileName,
    const INT               flags = 0 );

ERR ErrIsamCreateDatabase(
    JET_SESID                   sesid,
    const WCHAR* const          wszDatabaseName,
    JET_DBID* const             pjifmp,
    const JET_SETDBPARAM* const rgsetdbparam,
    const ULONG                 csetdbparam,
    const JET_GRBIT             grbit );

ERR ErrIsamOpenDatabase(
    JET_SESID       sesid,
    const WCHAR     *wszDatabase,
    const WCHAR     *wszConnect,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit );

ERR ErrIsamPrereadTables( __in JET_SESID sesid, __in JET_DBID vdbid, __in_ecount( cwszTables ) PCWSTR * rgwszTables, __in INT cwszTables, JET_GRBIT grbit );

ERR ErrIsamCloseDatabase( JET_SESID sesid, JET_DBID vdbid, JET_GRBIT grbit );

typedef __int64 TRXID;

ERR ErrIsamSetSessionParameter(
    _In_opt_ JET_SESID                          sesid,
    _In_ ULONG                          sesparamid,
    _In_reads_bytes_opt_( cbParam ) void *      pvParam,
    _In_ ULONG                      cbParam );

ERR ErrIsamGetSessionParameter(
    _In_opt_ JET_SESID                                          sesid,
    _In_ ULONG                                          sesparamid,
    _Out_cap_post_count_(cbParamMax, *pcbParamActual) void *    pvParam,
    _In_ ULONG                                          cbParamMax,
    _Out_opt_ ULONG *                                   pcbParamActual );

ERR ErrIsamBeginTransaction( JET_SESID sesid, TRXID trxid, JET_GRBIT grbit );

ERR ErrIsamPrepareToCommitTransaction(
    JET_SESID       sesid,
    const VOID      * const pvData,
    const ULONG     cbData );

ERR ErrIsamCommitTransaction( JET_SESID sesid, JET_GRBIT grbit, DWORD cmsecDurableCommit = 0, JET_COMMIT_ID *pCommitId = NULL );

ERR ErrIsamRollback( JET_SESID sesid, const JET_GRBIT grbit );

ERR ErrIsamBackup(
    JET_INSTANCE jinst,
    const WCHAR* szBackupPath,
    JET_GRBIT grbit,
    JET_PFNINITCALLBACK pfnStatus,
    void * pvStatusContext );

ERR ErrIsamRestore(
    JET_INSTANCE jinst,
    __in JET_PCWSTR wszRestoreFromPath,
    __in JET_PCWSTR wszDestPath,
    JET_PFNINITCALLBACK pfnStatus,
    void * pvStatusContext );

ERR ErrIsamBeginExternalBackup( JET_INSTANCE jinst, JET_GRBIT grbit );

ERR ErrIsamEndExternalBackup(  JET_INSTANCE jinst, JET_GRBIT grbit );

ERR ErrIsamBeginSurrogateBackup( __in JET_INSTANCE jinst, __in ULONG lgenFirst, __in ULONG lgenLast, __in JET_GRBIT grbit );

ERR ErrIsamEndSurrogateBackup( __inout JET_INSTANCE jinst, __in JET_GRBIT grbit );

ERR ErrIsamGetLogInfo(  JET_INSTANCE jinst, __out_bcount_part_opt(cbMax, *pcbActual) PWSTR wszzLogs, ULONG cbMax, ULONG *pcbActual, JET_LOGINFO_W *pLogInfo );

ERR ErrIsamGetTruncateLogInfo(  JET_INSTANCE jinst, __out_bcount_part_opt(cbMax, *pcbActual) PWSTR wszzLogs, ULONG cbMax, ULONG *pcbActual );

ERR ErrIsamGetAttachInfo( JET_INSTANCE jinst, __out_bcount_part_opt(cbMax, *pcbActual) PWSTR szDatabases, ULONG cbMax, ULONG *pcbActual );

ERR ErrIsamOpenFile( JET_INSTANCE jinst, const WCHAR *wszFileName, JET_HANDLE   *phfFile, const QWORD ibRead,
                    ULONG *pulFileSizeLow, ULONG *pulFileSizeHigh );

ERR ErrIsamCloseFile( JET_INSTANCE jinst,  JET_HANDLE hfFile );

ERR ErrIsamReadFile( JET_INSTANCE jinst, JET_HANDLE hfFile, void *pv, ULONG cbMax, ULONG *pcbActual );

ERR ErrIsamTruncateLog(  JET_INSTANCE jinst );

ERR ErrIsamExternalRestore(
    JET_INSTANCE jinst,
    __in JET_PCWSTR wszCheckpointFilePath,
    __in JET_PCWSTR wszLogPath,
    __in_ecount_opt(crstfilemap) JET_RSTMAP_W *rgstmap,
    INT crstfilemap,
    __in JET_PCWSTR wszBackupLogPath,
    LONG genLow,
    LONG genHigh,
    __in JET_PCWSTR wszTargetLogPath,
    LONG lGenHighTarget,
    JET_PFNINITCALLBACK pfn,
    void * pvContext );


ERR ErrIsamCompact( JET_SESID sesid,
    const WCHAR *wszDatabaseSrc,
    IFileSystemAPI *pfsapiDest,
    const WCHAR *wszDatabaseDest,
    JET_PFNSTATUS pfnStatus, JET_CONVERT_W*pconvert, JET_GRBIT grbit );

ERR ErrIsamTrimDatabase(
    _In_ JET_SESID sesid,
    _In_ JET_PCWSTR wszDatabaseName,
    _In_ IFileSystemAPI* pfsapi,
    _In_ CPRINTF * pcprintf,
    _In_ JET_GRBIT grbit );

ERR ErrIsamDBUtilities( JET_SESID sesid, JET_DBUTIL_W *pdbutil );

ERR VTAPI ErrIsamCreateObject( JET_SESID sesid, JET_DBID vdbid,
    OBJID objidParentId, const char *szName, JET_OBJTYP objtyp );

ERR VTAPI ErrIsamRenameObject( JET_SESID vsesid, JET_DBID vdbid,
    const char *szContainerName, const char *szObjectName, const char  *szObjectNameNew );

ERR ErrIsamDeleteObject( JET_SESID sesid, JET_DBID vdbid, OBJID objid );

ERR ErrIsamGetDatabaseInfo(
    JET_SESID       vsesid,
    JET_DBID        vdbid,
    VOID            *pvResult,
    const ULONG     cbMax,
    const ULONG     ulInfoLevel );

ERR
ErrIsamGetDatabasePages(
    __in JET_SESID                              vsesid,
    __in JET_DBID                               vdbid,
    __in ULONG                          pgnoStart,
    __in ULONG                          cpg,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    __in ULONG                          cb,
    __out ULONG *                       pcbActual,
    __in JET_GRBIT                              grbit );

ERR
ErrIsamOnlinePatchDatabasePage(
    __in JET_SESID                              vsesid,
    __in JET_DBID                               vdbid,
    __in ULONG                          pgno,
    __in_bcount(cbData) const void *            pvToken,
    __in ULONG                          cbToken,
    __in_bcount(cbData) const void *            pvData,
    __in ULONG                          cbData,
    __in JET_GRBIT                              grbit );

ERR
ErrIsamBeginDatabaseIncrementalReseed(
    __in JET_INSTANCE   jinst,
    __in JET_PCWSTR     szDatabase,
    __in JET_GRBIT      grbit );

ERR
ErrIsamEndDatabaseIncrementalReseed(
    __in JET_INSTANCE   jinst,
    __in JET_PCWSTR     szDatabase,
    __in ULONG  genMinRequired,
    __in ULONG  genFirstDivergedLog,
    __in ULONG  genMaxRequired,
    __in JET_GRBIT      grbit );

ERR
ErrIsamPatchDatabasePages(
    __in JET_INSTANCE               jinst,
    __in JET_PCWSTR                 szDatabase,
    __in ULONG              pgnoStart,
    __in ULONG              cpg,
    __in_bcount( cb ) const void *  pv,
    __in ULONG              cb,
    __in JET_GRBIT                  grbit );

ERR ErrIsamRemoveLogfile(
    _In_ IFileSystemAPI * const     pfsapi,
    _In_z_ const WCHAR * const      wszDatabase,
    _In_z_ const WCHAR * const      wszLogfile,
    _In_ const JET_GRBIT            grbit );

ERR ErrIsamGetObjectInfo( JET_SESID vsesid, JET_DBID ifmp, JET_OBJTYP objtyp,
    const char *szContainerName, const char *szObjectName, VOID *pv, ULONG cbMax,
    ULONG lInfoLevel, const BOOL fUnicodeNames );

ERR ErrIsamGetColumnInfo( JET_SESID vsesid, JET_DBID vdbid, const char *szTable,
    const char *szColumnName, JET_COLUMNID *pcolid, VOID *pv, ULONG cbMax, ULONG lInfoLevel, const BOOL fUnicodeNames );

ERR ErrIsamGetIndexInfo( JET_SESID vsesid, JET_DBID vdbid, const char *szTable,
    const char *szIndexName, VOID *pv, ULONG cbMax, ULONG lInfoLevel, const BOOL fUnicodeNames );

ERR ErrIsamGetSysTableColumnInfo( JET_SESID vsesid, JET_DBID vdbid,
    __in JET_PCSTR szColumnName, VOID *pv, ULONG cbMax, LONG lInfoLevel );

ERR ErrIsamCreateTable( JET_SESID vsesid, JET_DBID vdbid, JET_TABLECREATE5_A *ptablecreate );

ERR ErrIsamDeleteTable( JET_SESID vsesid, JET_DBID vdbid, const CHAR *szName );

ERR ErrIsamRenameTable( JET_SESID vsesid, JET_DBID vdbid,
    const CHAR *szName, const CHAR *szNameNew );

ERR VTAPI ErrIsamOpenTable( JET_SESID vsesid, JET_DBID vdbid,
    JET_TABLEID *ptableid, __in const JET_PCSTR szPath, JET_GRBIT grbit );

ERR ErrIsamOpenTempTable(
    JET_SESID           sesid,
    const JET_COLUMNDEF *prgcolumndef,
    ULONG       ccolumn,
    JET_UNICODEINDEX2   *pidxunicode,
    JET_GRBIT           grbit,
    JET_TABLEID         *ptableid,
    JET_COLUMNID        *prgcolumnid,
    const ULONG         cbKeyMost,
    const ULONG         cbVarSegMac );

ERR ErrIsamIndexRecordCount(
    JET_SESID       sesid,
    JET_VTID        vtid,
    ULONG   *pcrec,
    ULONG   crecMax );

ERR ErrIsamSetColumns(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_SETCOLUMN   *psetcols,
    ULONG   csetcols );

ERR ErrIsamRetrieveColumns(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_RETRIEVECOLUMN  *pretcols,
    ULONG   cretcols );

ERR ErrIsamEnumerateColumns(
    JET_SESID               vsesid,
    JET_VTID                vtid,
    ULONG           cEnumColumnId,
    JET_ENUMCOLUMNID*       rgEnumColumnId,
    ULONG*          pcEnumColumn,
    JET_ENUMCOLUMN**        prgEnumColumn,
    JET_PFNREALLOC          pfnRealloc,
    void*                   pvReallocContext,
    ULONG           cbDataMost,
    JET_GRBIT               grbit );

ERR ErrIsamRetrieveTaggedColumnList(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    ULONG               *pcentries,
    VOID                *pv,
    const ULONG         cbMax,
    const JET_COLUMNID  columnidStart,
    const JET_GRBIT     grbit );

ERR ErrIsamGetRecordSize(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    JET_RECSIZE3 *      precsize,
    JET_GRBIT           grbit );

ERR ErrIsamSetSequential(
    const JET_SESID     vsesid,
    const JET_VTID      vtid,
    const JET_GRBIT     grbit );

ERR ErrIsamResetSequential(
    const JET_SESID     vsesid,
    const JET_VTID      vtid,
    const JET_GRBIT     grbit );

ERR ErrIsamSetCurrentIndex(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    const CHAR          *szName,
    const JET_INDEXID   *pindexid       = NULL,
    const JET_GRBIT     grbit           = JET_bitMoveFirst,
    const ULONG         itagSequence    = 1 );



ERR ErrIsamDefragment(
    JET_SESID       vsesid,
    JET_DBID        vdbid,
    const CHAR      *szTable,
    ULONG           *pcPasses,
    ULONG           *pcsec,
    JET_CALLBACK    callback,
    JET_GRBIT       grbit );

ERR ErrIsamDefragment2(
    JET_SESID       vsesid,
    const CHAR      *szDatabase,
    const CHAR      *szTable,
    ULONG           *pcPasses,
    ULONG           *pcsec,
    JET_CALLBACK    callback,
    void            *pvContext,
    JET_GRBIT       grbit );

ERR ISAMAPI ErrIsamSetMaxDatabaseSize(
    const JET_SESID vsesid,
    const JET_DBID  vdbid,
    const ULONG     cpg,
    const JET_GRBIT grbit );

ERR ISAMAPI ErrIsamGetMaxDatabaseSize(
    const JET_SESID vsesid,
    const JET_DBID  vdbid,
    ULONG * const   pcpg,
    const JET_GRBIT grbit );

ERR ErrIsamSetDatabaseSize(
    JET_SESID       vsesid,
    const WCHAR *   szDatabase,
    DWORD           cpg,
    DWORD           *pcpgReal );

ERR ISAMAPI ErrIsamResizeDatabase(
    __in JET_SESID      vsesid,
    __in JET_DBID       vdbid,
    __in ULONG  cpg,
    _Out_opt_ ULONG *pcpgActual,
    __in JET_GRBIT      grbit );

ERR ErrIsamSetColumnDefaultValue(
    JET_SESID       vsesid,
    JET_DBID        vdbid,
    const CHAR      *szTableName,
    const CHAR      *szColumnName,
    const VOID      *pvData,
    const ULONG     cbData,
    const ULONG     grbit );

ERR ErrIsamRBSPrepareRevert(
    __in    JET_INSTANCE    jinst,
    __in    JET_LOGTIME     jltRevertExpected,
    __in    LONG            cpgCache,
    __in    JET_GRBIT       grbit,
    _Out_   JET_LOGTIME*    pjltRevertActual );

ERR ErrIsamRBSExecuteRevert(
    __in    JET_INSTANCE            jinst,
    __in    JET_GRBIT               grbit,
    _Out_   JET_RBSREVERTINFOMISC*  prbsrevertinfo );

ERR ErrIsamRBSCancelRevert(
    __in    JET_INSTANCE   jinst );

}

ERR ErrIsamIntersectIndexes(
    const JET_SESID sesid,
    _In_count_( cindexrange ) const JET_INDEXRANGE * const rgindexrange,
    _In_range_( 1, 64 ) const ULONG cindexrange,
    _Out_ JET_RECORDLIST * const precordlist,
    const JET_GRBIT grbit );

ERR VTAPI ErrIsamPrereadKeys(
    const JET_SESID                                 sesid,
    const JET_VTID                                  vtid,
    __in_ecount(ckeys) const void * const * const   rgpvKeys,
    __in_ecount(ckeys) const ULONG * const  rgcbKeys,
    const LONG                                      ckeys,
    __out_opt LONG * const                          pckeysPreread,
    const JET_GRBIT                                 grbit );

ERR VTAPI ErrIsamPrereadIndexRanges(
    const JET_SESID                                 sesid,
    const JET_VTID                                  vtid,
    __in_ecount(cIndexRanges) const JET_INDEX_RANGE * const rgIndexRanges,
    __in const ULONG                        cIndexRanges,
    __out_opt ULONG * const                 pcRangesPreread,
    __in_ecount(ccolumnidPreread) const JET_COLUMNID * const rgcolumnidPreread,
    const ULONG                                     ccolumnidPreread,
    const JET_GRBIT                                 grbit );

ERR VTAPI ErrIsamPrereadKeyRanges(
    _In_ JET_SESID                                              sesid,
    _In_ JET_TABLEID                                            tableid,
    _In_reads_(ckeys) void **                                   rgpvKeysStart,
    _In_reads_(ckeys) ULONG *                           rgcbKeysStart,
    _In_reads_opt_(ckeys) void **                               rgpvKeysEnd,
    _In_reads_opt_(ckeys) ULONG *                       rgcbKeysEnd,
    _In_ LONG                                                   ckeys,
    _Out_opt_ ULONG * const                             pckeyRangesPreread,
    _In_reads_(ccolumnidPreread) const JET_COLUMNID * const     rgcolumnidPreread,
    _In_ const ULONG                                    ccolumnidPreread,
    _In_ JET_GRBIT                                              grbit );

ERR VTAPI ErrIsamPrereadIndexRange(
    _In_ const JET_SESID                sesid,
    _In_ const JET_TABLEID              tableid,
    _In_ const JET_INDEX_RANGE * const  pIndexRange,
    _In_ const ULONG            cPageCacheMin,
    _In_ const ULONG            cPageCacheMax,
    _In_ const JET_GRBIT                grbit,
    _Out_opt_ ULONG * const     pcPageCacheActual );

ERR VTAPI ErrIsamRetrieveColumnByReference(
    _In_ const JET_SESID                                            sesid,
    _In_ const JET_TABLEID                                          tableid,
    _In_reads_bytes_( cbReference ) const void * const              pvReference,
    _In_ const ULONG                                        cbReference,
    _In_ const ULONG                                        ibData,
    _Out_writes_bytes_to_opt_( cbData, min( cbData, *pcbActual ) ) void * const pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit );

ERR VTAPI ErrIsamPrereadColumnsByReference(
    _In_ const JET_SESID                                    sesid,
    _In_ const JET_TABLEID                                  tableid,
    _In_reads_( cReferences ) const void * const * const    rgpvReferences,
    _In_reads_( cReferences ) const ULONG * const   rgcbReferences,
    _In_ const ULONG                                cReferences,
    _In_ const ULONG                                cPageCacheMin,
    _In_ const ULONG                                cPageCacheMax,
    _Out_opt_ ULONG * const                         pcReferencesPreread,
    _In_ const JET_GRBIT                                    grbit );

ERR VTAPI ErrIsamStreamRecords(
    _In_ JET_SESID                                                  sesid,
    _In_ JET_TABLEID                                                tableid,
    _In_ const ULONG                                        ccolumnid,
    _In_reads_opt_( ccolumnid ) const JET_COLUMNID * const          rgcolumnid,
    _Out_writes_bytes_to_opt_( cbData, *pcbActual ) void * const    pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit );

ERR VTAPI ErrIsamRetrieveColumnFromRecordStream(
    _Inout_updates_bytes_( cbData ) void * const    pvData,
    _In_ const ULONG                        cbData,
    _Out_ ULONG * const                     piRecord,
    _Out_ JET_COLUMNID * const                      pcolumnid,
    _Out_ ULONG * const                     pitagSequence,
    _Out_ ULONG * const                     pibValue,
    _Out_ ULONG * const                     pcbValue );


#ifndef ESENT
#pragma prefast(push)
#pragma prefast(disable:28251, "Inconsistent annotations. Prefast seems unable to deal with the function pointers' annotations.")
#endif
VTFNCreateIndex                 ErrIsamCreateIndex;
VTFNDeleteIndex                 ErrIsamDeleteIndex;
VTFNRenameColumn                ErrIsamRenameColumn;
VTFNRenameIndex                 ErrIsamRenameIndex;
VTFNDeleteColumn                ErrIsamDeleteColumn;
VTFNGetTableIndexInfo           ErrIsamGetTableIndexInfo;
VTFNGotoPosition                ErrIsamGotoPosition;
VTFNGetRecordPosition           ErrIsamGetRecordPosition;
VTFNGetCurrentIndex             ErrIsamGetCurrentIndex;
VTFNSetCurrentIndex             ErrIsamSetCurrentIndex;
VTFNGetTableInfo                ErrIsamGetTableInfo;
VTFNSetTableInfo                ErrIsamSetTableInfo;
VTFNGetTableColumnInfo          ErrIsamGetTableColumnInfo;
VTFNGetCursorInfo               ErrIsamGetCursorInfo;
VTFNGetLock                     ErrIsamGetLock;
VTFNMove                        ErrIsamMove;
VTFNSetCursorFilter             ErrIsamSetCursorFilter;
VTFNSeek                        ErrIsamSeek;
VTFNUpdate                      ErrIsamUpdate;
VTFNDelete                      ErrIsamDelete;
VTFNSetColumn                   ErrIsamSetColumn;
VTFNSetColumns                  ErrIsamSetColumns;
VTFNRetrieveColumn              ErrIsamRetrieveColumn;
VTFNRetrieveColumns             ErrIsamRetrieveColumns;
VTFNPrepareUpdate               ErrIsamPrepareUpdate;
VTFNDupCursor                   ErrIsamDupCursor;
VTFNGotoBookmark                ErrIsamGotoBookmark;
VTFNGotoIndexBookmark           ErrIsamGotoIndexBookmark;
VTFNMakeKey                     ErrIsamMakeKey;
VTFNRetrieveKey                 ErrIsamRetrieveKey;
VTFNSetIndexRange               ErrIsamSetIndexRange;
VTFNAddColumn                   ErrIsamAddColumn;
VTFNComputeStats                ErrIsamComputeStats;
VTFNGetBookmark                 ErrIsamGetBookmark;
VTFNGetIndexBookmark            ErrIsamGetIndexBookmark;
VTFNCloseTable                  ErrIsamCloseTable;
VTFNRegisterCallback            ErrIsamRegisterCallback;
VTFNUnregisterCallback          ErrIsamUnregisterCallback;
VTFNSetLS                       ErrIsamSetLS;
VTFNGetLS                       ErrIsamGetLS;
VTFNIndexRecordCount            ErrIsamIndexRecordCount;
VTFNRetrieveTaggedColumnList    ErrIsamRetrieveTaggedColumnList;
VTFNSetSequential               ErrIsamSetSequential;
VTFNResetSequential             ErrIsamResetSequential;
VTFNEnumerateColumns            ErrIsamEnumerateColumns;
VTFNGetRecordSize               ErrIsamGetRecordSize;
VTFNPrereadIndexRange           ErrIsamPrereadIndexRange;
VTFNRetrieveColumnByReference   ErrIsamRetrieveColumnByReference;
VTFNPrereadColumnsByReference   ErrIsamPrereadColumnsByReference;
VTFNStreamRecords               ErrIsamStreamRecords;
#ifndef ESENT
#pragma prefast(pop)
#endif



class PIB;
struct FUCB;

INLINE ERR VTAPI ErrIsamMove( PIB *ppib, FUCB *pfucb, LONG crow, JET_GRBIT grbit )
{
    return ErrIsamMove( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), crow, grbit );
}
INLINE ERR VTAPI ErrIsamSeek( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
{
    return ErrIsamSeek( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), grbit );
}

INLINE ERR VTAPI ErrIsamUpdate(
    PIB *ppib,
    FUCB *pfucb,
    _Out_writes_bytes_to_opt_(cbMax, *pcbActual) VOID *pv,
    ULONG cbMax,
    ULONG *pcbActual,
    JET_GRBIT grbit )
{
    return ErrIsamUpdate( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), pv, cbMax, pcbActual, grbit );
}
INLINE ERR VTAPI ErrIsamDelete( PIB *ppib, FUCB *pfucb )
{
    return ErrIsamDelete( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ) );
}

INLINE ERR VTAPI ErrIsamSetColumn(
    PIB*            ppib,
    FUCB*           pfucb,
    JET_COLUMNID    columnid,
    const VOID*     pvData,
    const ULONG     cbData,
    JET_GRBIT       grbit,
    JET_SETINFO*    psetinfo )
{
    return ErrIsamSetColumn(
                    reinterpret_cast<JET_SESID>( ppib ),
                    reinterpret_cast<JET_VTID>( pfucb ),
                    columnid,
                    pvData,
                    cbData,
                    grbit,
                    psetinfo );
}

INLINE ERR VTAPI ErrIsamRetrieveColumn(
    _In_ PIB*                                                                               ppib,
    _In_ FUCB*                                                                              pfucb,
    _In_ JET_COLUMNID                                                                       columnid,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) VOID*          pvData,
    _In_ const ULONG                                                                        cbDataMax,
    _Out_opt_ ULONG*                                                                        pcbDataActual,
    _In_ JET_GRBIT                                                                          grbit,
    _Inout_opt_ JET_RETINFO*                                                                pretinfo )
{
    return ErrIsamRetrieveColumn(
                    reinterpret_cast<JET_SESID>( ppib ),
                    reinterpret_cast<JET_VTID>( pfucb ),
                    columnid,
                    pvData,
                    cbDataMax,
                    pcbDataActual,
                    grbit,
                    pretinfo );
}

INLINE ERR VTAPI ErrIsamPrepareUpdate( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
{
    return ErrIsamPrepareUpdate( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), grbit );
}
INLINE ERR VTAPI ErrIsamDupCursor( PIB* ppib, FUCB* pfucb, FUCB ** ppfucb, ULONG ul )
{
    return ErrIsamDupCursor( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
                             reinterpret_cast<JET_TABLEID *>( ppfucb ), ul );
}
INLINE ERR VTAPI ErrIsamGotoBookmark(
    PIB *               ppib,
    FUCB *              pfucb,
    const VOID * const  pvBookmark,
    const ULONG         cbBookmark )
{
    return ErrIsamGotoBookmark(
                        reinterpret_cast<JET_SESID>( ppib ),
                        reinterpret_cast<JET_VTID>( pfucb ),
                        pvBookmark,
                        cbBookmark );
}
INLINE ERR VTAPI ErrIsamGotoIndexBookmark(
    PIB *               ppib,
    FUCB *              pfucb,
    const VOID * const  pvSecondaryKey,
    const ULONG         cbSecondaryKey,
    const VOID * const  pvPrimaryBookmark,
    const ULONG         cbPrimaryBookmark,
    const JET_GRBIT     grbit )
{
    return ErrIsamGotoIndexBookmark(
                        reinterpret_cast<JET_SESID>( ppib ),
                        reinterpret_cast<JET_VTID>( pfucb ),
                        pvSecondaryKey,
                        cbSecondaryKey,
                        pvPrimaryBookmark,
                        cbPrimaryBookmark,
                        grbit );
}
INLINE ERR VTAPI ErrIsamGotoPosition( PIB *ppib, FUCB *pfucb, JET_RECPOS *precpos )
{
    return ErrIsamGotoPosition( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
                                    precpos );
}

INLINE ERR VTAPI ErrIsamGetCurrentIndex( PIB *ppib, FUCB *pfucb, _Out_z_bytecap_(cbMax) PSTR szCurIdx, ULONG cbMax )
{
    return ErrIsamGetCurrentIndex( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
                                    szCurIdx, cbMax );
}
INLINE ERR VTAPI ErrIsamSetCurrentIndex( PIB *ppib, FUCB *pfucb, const CHAR *szName )
{
    return ErrIsamSetCurrentIndex( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
                                    szName );
}

INLINE ERR VTAPI ErrIsamMakeKey(
    PIB*            ppib,
    FUCB*           pfucb,
    __in_bcount( cbKeySeg ) const VOID* pvKeySeg,
    const ULONG     cbKeySeg,
    JET_GRBIT       grbit )
{
    return ErrIsamMakeKey(
                    reinterpret_cast<JET_SESID>( ppib ),
                    reinterpret_cast<JET_VTID>( pfucb ),
                    pvKeySeg,
                    cbKeySeg,
                    grbit );
}
INLINE ERR VTAPI ErrIsamRetrieveKey(
    PIB*            ppib,
    FUCB*           pfucb,
    VOID*           pvKey,
    const ULONG     cbMax,
    ULONG*          pcbKeyActual,
    JET_GRBIT       grbit )
{
    return ErrIsamRetrieveKey(
                    reinterpret_cast<JET_SESID>( ppib ),
                    reinterpret_cast<JET_VTID>( pfucb ),
                    pvKey,
                    cbMax,
                    pcbKeyActual,
                    grbit );
}
INLINE ERR VTAPI ErrIsamSetIndexRange( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
{
    return ErrIsamSetIndexRange( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), grbit );
}

INLINE ERR VTAPI ErrIsamComputeStats( PIB *ppib, FUCB *pfucb )
{
    return ErrIsamComputeStats( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ) );
}

INLINE ERR VTAPI ErrIsamRenameColumn( PIB *ppib, FUCB *pfucb, const CHAR *szName, const CHAR *szNameNew, const JET_GRBIT grbit )
{
    return ErrIsamRenameColumn( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
                                szName, szNameNew, grbit );
}
INLINE ERR VTAPI ErrIsamRenameIndex( PIB *ppib, FUCB *pfucb, const CHAR *szName, const CHAR *szNameNew )
{
    return ErrIsamRenameIndex( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
                                szName, szNameNew);
}

INLINE ERR VTAPI ErrIsamAddColumn(
            PIB             *ppib,
            FUCB            *pfucb,
    const   CHAR            * const szName,
    const   JET_COLUMNDEF   * const pcolumndef,
    const   VOID            * const pvDefault,
            ULONG           cbDefault,
            JET_COLUMNID    * const pcolumnid )
{
    return ErrIsamAddColumn( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
                                szName, pcolumndef, pvDefault, cbDefault, pcolumnid );
}

INLINE ERR VTAPI ErrIsamDeleteColumn( PIB *ppib, FUCB *pfucb, const CHAR *szName, const JET_GRBIT grbit )
{
    return ErrIsamDeleteColumn( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), szName, grbit );
}
INLINE ERR VTAPI ErrIsamDeleteIndex( PIB *ppib, FUCB *pfucb, const CHAR *szName )
{
    return ErrIsamDeleteIndex( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), szName );
}
INLINE ERR VTAPI ErrIsamGetBookmark(
    PIB *           ppib,
    FUCB *          pfucb,
    VOID * const    pvBookmark,
    const ULONG     cbMax,
    ULONG * const   pcbActual )
{
    return ErrIsamGetBookmark(
                    reinterpret_cast<JET_SESID>( ppib ),
                    reinterpret_cast<JET_VTID>( pfucb ),
                    pvBookmark,
                    cbMax,
                    pcbActual );
}
INLINE ERR VTAPI ErrIsamGetIndexBookmark(
    PIB *           ppib,
    FUCB *          pfucb,
    VOID * const    pvSecondaryKey,
    const ULONG     cbSecondaryKeyMax,
    ULONG * const   pcbSecondaryKeyActual,
    VOID * const    pvPrimaryBookmark,
    const ULONG     cbPrimaryBookmarkMax,
    ULONG * const   pcbPrimaryBookmarkActual )
{
    return ErrIsamGetIndexBookmark(
                    reinterpret_cast<JET_SESID>( ppib ),
                    reinterpret_cast<JET_VTID>( pfucb ),
                    pvSecondaryKey,
                    cbSecondaryKeyMax,
                    pcbSecondaryKeyActual,
                    pvPrimaryBookmark,
                    cbPrimaryBookmarkMax,
                    pcbPrimaryBookmarkActual );
}

INLINE ERR VTAPI ErrIsamCloseTable( PIB *ppib, FUCB *pfucb )
{
    return ErrIsamCloseTable( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ) );
}

