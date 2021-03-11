// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

n
JET_ERR JET_API JetInit( JET_INSTANCE *pinstance);

n
JET_ERR JET_API JetInit2( JET_INSTANCE *pinstance, JET_GRBIT grbit );

c
JET_RSTINFO_W*  prstInfo    EsetestWidenJET_RSTINFO EsetestUnwidenJET_RSTINFO
JET_ERR JET_API JetInit3(
    JET_INSTANCE *pinstance,
    JET_RSTINFO *prstInfo,
    JET_GRBIT grbit );

c
JET_RSTINFO2_W* prstInfo    EsetestWidenJET_RSTINFO2    EsetestUnwidenJET_RSTINFO2
JET_ERR JET_API JetInit4(
    JET_INSTANCE *pinstance,
    JET_RSTINFO2 *prstInfo,
    JET_GRBIT grbit );
w
wchar_t*    szInstanceName  EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetCreateInstance( JET_INSTANCE *pinstance, const char * szInstanceName );

w
wchar_t*    szInstanceName  EsetestWidenString  EsetestCleanupWidenString
wchar_t*    szDisplayName   EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetCreateInstance2(
    JET_INSTANCE *pinstance,
    const char * szInstanceName,
    const char * szDisplayName,
    JET_GRBIT grbit );

n
JET_ERR JET_API JetGetInstanceMiscInfo(
    JET_INSTANCE    instance,
    void *          pvResult,
    unsigned long       cbMax,
    unsigned long       InfoLevel );


n
JET_ERR JET_API JetTerm( JET_INSTANCE instance );

n
JET_ERR JET_API JetTerm2( JET_INSTANCE instance, JET_GRBIT grbit );

n
JET_ERR JET_API JetStopService();

n
JET_ERR JET_API JetStopServiceInstance( JET_INSTANCE instance );

n
JET_ERR JET_API JetStopServiceInstance2( JET_INSTANCE instance, JET_GRBIT grbit );

n
JET_ERR JET_API JetStopBackup();

n
JET_ERR JET_API JetStopBackupInstance( JET_INSTANCE instance );

w
wchar_t*    szParam     EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetSetSystemParameter(
    JET_INSTANCE    *pinstance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCSTR      szParam );

c
wchar_t*    szParam,cbMax       EsetestWidenStringWithLength    EsetestUnwidenStringWithLength
JET_ERR JET_API JetGetSystemParameter(
    JET_INSTANCE    instance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __out_bcount_opt(cbMax) JET_API_PTR     *plParam,
    __out_bcount_opt(cbMax) JET_PSTR        szParam,
    unsigned long   cbMax );

n
JET_ERR JET_API JetSetSessionParameter(
    JET_SESID           sesid,
    unsigned long       sesparamid,
    void *              pvParam,
    unsigned long       cbParam );

n
JET_ERR JET_API JetSetResourceParam(
    JET_INSTANCE    instance,
    JET_RESOPER     resoper,
    JET_RESID       resid,
    JET_API_PTR     ulParam );

n
JET_ERR JET_API JetGetResourceParam(
    JET_INSTANCE    instance,
    JET_RESOPER     resoper,
    JET_RESID       resid,
    JET_API_PTR     *pulParam );

c
JET_SETSYSPARAM_W*  psetsysparam,csetsysparam   EsetestWidenJET_SETSYSPARAM EsetestUnwidenJET_SETSYSPARAM
JET_ERR JET_API JetEnableMultiInstance(     
    __in_ecount(csetsysparam) JET_SETSYSPARAM   *psetsysparam, 
    unsigned long       csetsysparam, 
    unsigned long *pcsetsucceed );

n
JET_ERR JET_API JetResetCounter( JET_SESID sesid, long CounterType );

n
JET_ERR JET_API JetGetCounter( JET_SESID sesid, long CounterType, long *plValue );

n
JET_ERR JET_API JetGetThreadStats(
    __out_bcount(cbMax) void*           pvResult,
    unsigned long   cbMax );

w
wchar_t*    szUserName  EsetestWidenString  EsetestCleanupWidenString
wchar_t*    szPassword  EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetBeginSession(
    JET_INSTANCE    instance,
    JET_SESID       *psesid,
    __in_opt JET_PCSTR  szUserName,
    __in_opt JET_PCSTR  szPassword );

n
JET_ERR JET_API JetDupSession( JET_SESID sesid, JET_SESID *psesid );

n
JET_ERR JET_API JetEndSession( JET_SESID sesid, JET_GRBIT grbit );

n
JET_ERR JET_API JetGetSessionInfo(
    JET_SESID           sesid,
    __out_bcount(cbMax) void *  pvResult,
    const unsigned long cbMax,
    const unsigned long ulInfoLevel );

n
JET_ERR JET_API JetGetVersion( JET_SESID sesid, unsigned long *pwVersion );

n
JET_ERR JET_API JetIdle( JET_SESID sesid, JET_GRBIT grbit );

w
wchar_t*    szFilename  EsetestWidenString  EsetestCleanupWidenString
wchar_t*    szConnect   EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetCreateDatabase(
    JET_SESID       sesid,
    const char      *szFilename,
    const char      *szConnect,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit );

w
wchar_t*    szFilename  EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetCreateDatabase2(
    JET_SESID       sesid,
    const char      *szFilename,
    const unsigned long cpgDatabaseSizeMax,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit );

w
wchar_t*    szDbFileName    EsetestWidenString  EsetestCleanupWidenString
wchar_t*    szSLVFileName   EsetestWidenString  EsetestCleanupWidenString
wchar_t*    szSLVRootName   EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetCreateDatabaseWithStreaming(
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const char      *szSLVRootName,
    const unsigned long cpgDatabaseSizeMax,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit );

w
wchar_t*    szFilename  EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetAttachDatabase(
    JET_SESID       sesid,
    const char      *szFilename,
    JET_GRBIT       grbit );

w
wchar_t*    szFilename  EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetAttachDatabase2(
    JET_SESID       sesid,
    const char      *szFilename,
    const unsigned long cpgDatabaseSizeMax,
    JET_GRBIT       grbit );

w
wchar_t*    szFilename  EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetAttachDatabase3(
    JET_SESID       sesid,
    const char      *szFilename,
    const JET_SETDBPARAM* const rgsetdbparam,
    const unsigned long csetdbparam,
    JET_GRBIT       grbit );

w
wchar_t*    szDbFileName    EsetestWidenString  EsetestCleanupWidenString
wchar_t*    szSLVFileName   EsetestWidenString  EsetestCleanupWidenString
wchar_t*    szSLVRootName   EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetAttachDatabaseWithStreaming(
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const char      *szSLVRootName,
    const unsigned long cpgDatabaseSizeMax,
    JET_GRBIT       grbit );

w
wchar_t*    szFilename  EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetDetachDatabase(
    JET_SESID       sesid,
    const char      *szFilename );

w
wchar_t*    szFilename  EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetDetachDatabase2(
    JET_SESID       sesid,
    const char      *szFilename,
    JET_GRBIT       grbit);



n
JET_ERR JET_API JetGetObjectInfo(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_OBJTYP      objtyp,
    const char      *szContainerName,
    const char      *szObjectName,
    __out_bcount(cbMax) void *  pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel );

n
JET_ERR JET_API JetGetTableInfo(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount(cbMax) void *  pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel );

n
JET_ERR JET_API JetGetPageInfo(
    __in_bcount(cbData)         const void *    pvPages,
                                unsigned long   cbData,
    __inout_bcount(cbPageInfo)  JET_PAGEINFO *  rgPageInfo,
                                unsigned long   cbPageInfo,
                                JET_GRBIT       grbit,
                                unsigned long   ulInfoLevel );

n
JET_ERR JET_API JetGetPageInfo2(
    __in_bcount(cbData)         const void *    pvPages,
                                unsigned long   cbData,
    __inout_bcount(cbPageInfo)  JET_PAGEINFO *  rgPageInfo,
                                unsigned long   cbPageInfo,
                                JET_GRBIT       grbit,
                                unsigned long   ulInfoLevel );

c
wchar_t*    szDatabase  EsetestWidenString  EsetestCleanupWidenString
wchar_t*    szLogfile   EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetRemoveLogfile(
    __in JET_PCSTR szDatabase,
    __in JET_PCSTR szLogfile,
    __in JET_GRBIT grbit );

c
JET_ERR JET_API
JetGetDatabasePages(
    __in JET_SESID                              sesid,
    __in JET_DBID                               dbid,
    __in unsigned long                          pgnoStart,
    __in unsigned long                          cpg,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    __in unsigned long                          cb,
    __out unsigned long *                       pcbActual,
    __in JET_GRBIT                              grbit );

w
wchar_t*    szTableName EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetCreateTable(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   lPages,
    unsigned long   lDensity,
    JET_TABLEID     *ptableid );

c
JET_TABLECREATE_W*  ptablecreate    EsetestWidenJET_TABLECREATE EsetestUnwidenJET_TABLECREATE
JET_ERR JET_API JetCreateTableColumnIndex(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE *ptablecreate );

c
JET_TABLECREATE2_W* ptablecreate    EsetestWidenJET_TABLECREATE2    EsetestUnwidenJET_TABLECREATE2
JET_ERR JET_API JetCreateTableColumnIndex2(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE2    *ptablecreate );

c
JET_TABLECREATE3_W* ptablecreate    EsetestWidenJET_TABLECREATE3    EsetestUnwidenJET_TABLECREATE3
JET_ERR JET_API JetCreateTableColumnIndex3(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE3    *ptablecreate );

c
JET_TABLECREATE5_W* ptablecreate    EsetestWidenJET_TABLECREATE5    EsetestUnwidenJET_TABLECREATE5
JET_ERR JET_API JetCreateTableColumnIndex5(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE5    *ptablecreate );

w
wchar_t*    szTableName EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetDeleteTable(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName );

w
wchar_t*    szName EsetestWidenString   EsetestCleanupWidenString
wchar_t*    szNameNew EsetestWidenString    EsetestCleanupWidenString
JET_ERR JET_API JetRenameTable(
    JET_SESID sesid,
    JET_DBID dbid,
    const char *szName,
    const char *szNameNew );


n
JET_ERR JET_API JetGetTableColumnInfo(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName,
    __out_bcount(cbMax) void *  pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel );


n
JET_ERR JET_API JetGetColumnInfo(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    const char      *szColumnName,
    __out_bcount(cbMax) void *  pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel );

c
wchar_t*    szColumnName EsetestWidenString EsetestCleanupWidenString   
JET_ERR JET_API JetAddColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName,
    const JET_COLUMNDEF *pcolumndef,
    __in_bcount_opt(cbDefault) const void*      pvDefault,
    unsigned long   cbDefault,
    __out_opt JET_COLUMNID  *pcolumnid );

w
wchar_t*    szColumnName EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetDeleteColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName );

w
wchar_t*    szColumnName EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetDeleteColumn2(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName,
    const JET_GRBIT grbit );

w
wchar_t*    szName EsetestWidenString EsetestCleanupWidenString
wchar_t*    szNameNew EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetRenameColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szName,
    __in JET_PCSTR  szNameNew,
    JET_GRBIT       grbit );

w
wchar_t*    szTableName EsetestWidenString EsetestCleanupWidenString
wchar_t*    szColumnName EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetSetColumnDefaultValue(
    JET_SESID           sesid,
    JET_DBID            dbid,
    __in JET_PCSTR      szTableName,
    __in JET_PCSTR      szColumnName,
    __in_bcount(cbData) const void          *pvData,
    const unsigned long cbData,
    const JET_GRBIT     grbit );


n
JET_ERR JET_API JetGetTableIndexInfo(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    __out_bcount(cbResult) void         *pvResult,
    unsigned long   cbResult,
    unsigned long   InfoLevel );


n
JET_ERR JET_API JetGetIndexInfo(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    const char      *szIndexName,
    __out_bcount(cbResult) void         *pvResult,
    unsigned long   cbResult,
    unsigned long   InfoLevel );

c
wchar_t*    szIndexName EsetestWidenString  EsetestCleanupWidenString
wchar_t*    szKey   EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetCreateIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szIndexName,
    JET_GRBIT       grbit,
    const char      *szKey,
    unsigned long   cbKey,
    unsigned long   lDensity );

c
JET_INDEXCREATE_W*  pindexcreate,cIndexCreate   EsetestWidenJET_INDEXCREATE EsetestUnwidenJET_INDEXCREATE
JET_ERR JET_API JetCreateIndex2(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in_ecount(cIndexCreate) JET_INDEXCREATE   *pindexcreate,
    unsigned long   cIndexCreate );

w
wchar_t*    szIndexName EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetDeleteIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szIndexName );

n
JET_ERR JET_API JetBeginTransaction( JET_SESID sesid );

n
JET_ERR JET_API JetBeginTransaction2( JET_SESID sesid, JET_GRBIT grbit );

n
JET_ERR JET_API JetPrepareToCommitTransaction(
    JET_SESID       sesid,
    const void      * pvData,
    unsigned long   cbData,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetCommitTransaction( JET_SESID sesid, JET_GRBIT grbit );

n
JET_ERR JET_API JetRollback( JET_SESID sesid, JET_GRBIT grbit );

n
JET_ERR JET_API JetGetDatabaseInfo(
    JET_SESID       sesid,
    JET_DBID        dbid,
    void            *pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel );


w
wchar_t*    szDatabaseName  EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetGetDatabaseFileInfo(
    const char      *szDatabaseName,
    void            *pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel );

w
wchar_t*    szFilename  EsetestWidenString  EsetestCleanupWidenString
wchar_t*    szConnect   EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetOpenDatabase(
    JET_SESID       sesid,
    const char      *szFilename,
    const char      *szConnect,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetCloseDatabase(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_GRBIT       grbit );

w
wchar_t*    szTableName EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetOpenTable(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    const void      *pvParameters,
    unsigned long   cbParameters,
    JET_GRBIT       grbit,
    JET_TABLEID     *ptableid );

n
JET_ERR JET_API JetSetTableInfo(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const void      *pvParam,
    unsigned long   cbParam,
    unsigned long   InfoLevel );

n
JET_ERR JET_API JetSetTableSequential(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetResetTableSequential(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetCloseTable( JET_SESID sesid, JET_TABLEID tableid );

n
JET_ERR JET_API JetDelete( JET_SESID sesid, JET_TABLEID tableid );

n
JET_ERR JET_API JetUpdate(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    void            *pvBookmark,
    unsigned long   cbBookmark,
    unsigned long   *pcbActual);

n
JET_ERR JET_API JetUpdate2(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    void            *pvBookmark,
    unsigned long   cbBookmark,
    unsigned long   *pcbActual,
    const JET_GRBIT grbit );

n
JET_ERR JET_API JetEscrowUpdate(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_COLUMNID    columnid,
    void            *pv,
    unsigned long   cbMax,
    void            *pvOld,
    unsigned long   cbOldMax,
    unsigned long   *pcbOldActual,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetRetrieveColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_COLUMNID    columnid,
    void            *pvData,
    unsigned long   cbData,
    unsigned long   *pcbActual,
    JET_GRBIT       grbit,
    JET_RETINFO     *pretinfo );

n
JET_ERR JET_API JetRetrieveColumns(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_RETRIEVECOLUMN  *pretrievecolumn,
    unsigned long   cretrievecolumn );

n
JET_ERR JET_API JetEnumerateColumns(
    JET_SESID               sesid,
    JET_TABLEID             tableid,
    unsigned long           cEnumColumnId,
    JET_ENUMCOLUMNID*       rgEnumColumnId,
    unsigned long*          pcEnumColumn,
    JET_ENUMCOLUMN**        prgEnumColumn,
    JET_PFNREALLOC          pfnRealloc,
    void*                   pvReallocContext,
    unsigned long           cbDataMost,
    JET_GRBIT               grbit );

n
JET_ERR JET_API JetRetrieveTaggedColumnList(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    unsigned long   *pcColumns,
    void            *pvData,
    unsigned long   cbData,
    JET_COLUMNID    columnidStart,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetGetRecordSize(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_RECSIZE *   precsize,
    const JET_GRBIT grbit );

c
JET_ERR JET_API JetSetColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_COLUMNID    columnid,
    const void      *pvData,
    unsigned long   cbData,
    JET_GRBIT       grbit,
    JET_SETINFO     *psetinfo );

c
JET_ERR JET_API JetSetColumns(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_SETCOLUMN   *psetcolumn,
    unsigned long   csetcolumn );

n
JET_ERR JET_API JetPrepareUpdate(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    unsigned long   prep );

n
JET_ERR JET_API JetGetRecordPosition(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_RECPOS      *precpos,
    unsigned long   cbRecpos );

n
JET_ERR JET_API JetGotoPosition(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_RECPOS      *precpos );

n
JET_ERR JET_API JetGetCursorInfo(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount(cbMax) void            *pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel );

n
JET_ERR JET_API JetDupCursor(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_TABLEID     *ptableid,
    JET_GRBIT       grbit );

c
wchar_t*    szIndexName,cchIndexName    EsetestWidenStringWithLength    EsetestUnwidenStringWithLength  
JET_ERR JET_API JetGetCurrentIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount(cchIndexName) JET_PSTR szIndexName,
    unsigned long   cchIndexName );

w
wchar_t*    szIndexName EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetSetCurrentIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName );

w
wchar_t*    szIndexName EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetSetCurrentIndex2(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_GRBIT       grbit );

w
wchar_t*    szIndexName EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetSetCurrentIndex3(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_GRBIT       grbit,
    unsigned long   itagSequence );

w
wchar_t*    szIndexName EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetSetCurrentIndex4(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_INDEXID     *pindexid,
    JET_GRBIT       grbit,
    unsigned long   itagSequence );

n
JET_ERR JET_API JetMove(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    long            cRow,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetGetLock(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetMakeKey(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in_bcount(cbData) const void      *pvData,
    unsigned long   cbData,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetSeek(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetGetBookmark(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount_part_opt(cbMax, *pcbActual) void *         pvBookmark,
    unsigned long   cbMax,
    unsigned long * pcbActual );

n
JET_ERR JET_API JetGetSecondaryIndexBookmark(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount_part(cbSecondaryKeyMax, *pcbSecondaryKeyActual) void *         pvSecondaryKey,
    unsigned long   cbSecondaryKeyMax,
    unsigned long * pcbSecondaryKeyActual,
    __out_bcount_part(cbPrimaryBookmarkMax, *pcbPrimaryKeyActual) void *            pvPrimaryBookmark,
    unsigned long   cbPrimaryBookmarkMax,
    unsigned long * pcbPrimaryKeyActual,
    const JET_GRBIT grbit );

w
wchar_t*    szDatabaseSrc EsetestWidenString EsetestCleanupWidenString
wchar_t*    szDatabaseDest EsetestWidenString EsetestCleanupWidenString
JET_CONVERT_W*  pconvert    EsetestWidenJET_CONVERT EsetestUnwidenJET_CONVERT
JET_ERR JET_API JetCompact(
    JET_SESID       sesid,
    const char      *szDatabaseSrc,
    const char      *szDatabaseDest,
    JET_PFNSTATUS   pfnStatus,
    JET_CONVERT     *pconvert,
    JET_GRBIT       grbit );

w
wchar_t*    szTableName EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetDefragment(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_GRBIT       grbit );

w
wchar_t*    szTableName EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetDefragment2(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_CALLBACK    callback,
    JET_GRBIT       grbit );

w
wchar_t*    szDatabaseName EsetestWidenString EsetestCleanupWidenString
wchar_t*    szTableName EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetDefragment3(
    JET_SESID       vsesid,
    const char      *szDatabaseName,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_CALLBACK    callback,
    void            *pvContext,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetDatabaseScan(
    JET_SESID       sesid,
    JET_DBID        dbid,
    unsigned long * pcSeconds,
    unsigned long   cmsecSleep,
    JET_CALLBACK    pfnCallback,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetConvertDDL(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_OPDDLCONV   convtyp,
    __out_bcount(cbData) void           *pvData,
    unsigned long   cbData );

w
wchar_t*    szDbFileName EsetestWidenString EsetestCleanupWidenString
wchar_t*    szSLVFileName EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetUpgradeDatabase(
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const JET_GRBIT grbit );

n
JET_ERR JET_API JetSetMaxDatabaseSize(
    JET_SESID       sesid,
    JET_DBID        dbid,
    unsigned long   cpg,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetGetMaxDatabaseSize(
    JET_SESID       sesid,
    JET_DBID        dbid,
    unsigned long   * pcpg,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetSetDatabaseSize(
    JET_SESID       sesid,
    const char      *szDatabaseName,
    unsigned long   cpg,
    unsigned long   *pcpgReal );

n
JET_ERR JET_API JetGrowDatabase(
    JET_SESID       sesid,
    JET_DBID        dbid,
    unsigned long   cpg,
    unsigned long   *pcpgReal );

n
JET_ERR JET_API JetResizeDatabase(
    JET_SESID       sesid,
    JET_DBID        dbid,
    unsigned long   cpg,
    unsigned long   *pcpgActual,
    const JET_GRBIT grbit );

n
JET_ERR JET_API JetSetSessionContext(
    JET_SESID       sesid,
    JET_API_PTR     ulContext );

n
JET_ERR JET_API JetResetSessionContext(
    JET_SESID       sesid );

n
JET_ERR JET_API JetDBUtilities( JET_DBUTIL *pdbutil );

n
JET_ERR JET_API JetGotoBookmark(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in_bcount(cbBookmark) void *          pvBookmark,
    unsigned long   cbBookmark );

n
JET_ERR JET_API JetGotoSecondaryIndexBookmark(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in_bcount(cbSecondaryKey) void *          pvSecondaryKey,
    unsigned long   cbSecondaryKey,
    __in_bcount(cbPrimaryBookmark) void *           pvPrimaryBookmark,
    unsigned long   cbPrimaryBookmark,
    const JET_GRBIT grbit );

n
JET_ERR JET_API JetIntersectIndexes(
    JET_SESID sesid,
    __in_ecount(cindexrange) JET_INDEXRANGE * rgindexrange,
    unsigned long cindexrange,
    JET_RECORDLIST * precordlist,
    JET_GRBIT grbit );

n
JET_ERR JET_API JetComputeStats( JET_SESID sesid, JET_TABLEID tableid );

n
JET_ERR JET_API JetOpenTempTable(
    JET_SESID sesid,
    __in_ecount(ccolumn) const JET_COLUMNDEF *prgcolumndef,
    unsigned long ccolumn,
    JET_GRBIT grbit,
    JET_TABLEID *ptableid,
    JET_COLUMNID *prgcolumnid);

n
JET_ERR JET_API JetOpenTempTable2(
    JET_SESID           sesid,
    __in_ecount(ccolumn) const JET_COLUMNDEF    *prgcolumndef,
    unsigned long       ccolumn,
    unsigned long       lcid,
    JET_GRBIT           grbit,
    JET_TABLEID         *ptableid,
    JET_COLUMNID        *prgcolumnid );

n
JET_ERR JET_API JetOpenTempTable3(
    JET_SESID           sesid,
    __in_ecount(ccolumn) const JET_COLUMNDEF    *prgcolumndef,
    unsigned long       ccolumn,
    JET_UNICODEINDEX    *pidxunicode,
    JET_GRBIT           grbit,
    JET_TABLEID         *ptableid,
    JET_COLUMNID        *prgcolumnid );

n
JET_ERR JET_API JetOpenTemporaryTable(
    JET_SESID               sesid,
    JET_OPENTEMPORARYTABLE  *popentemporarytable );

w
wchar_t*    szBackupPath EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetBackup( const char *szBackupPath, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus );

w
wchar_t*    szBackupPath EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetBackupInstance(  
        JET_INSTANCE    instance,
        const char      *szBackupPath,
        JET_GRBIT       grbit,
        JET_PFNSTATUS   pfnStatus );

w
wchar_t*    sz  EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetRestore(
    __in JET_PCSTR      sz,
    JET_PFNSTATUS       pfn );

w
wchar_t*    sz  EsetestWidenString  EsetestCleanupWidenString
wchar_t*    szDest  EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetRestore2(
    JET_PCSTR           sz,
    JET_PCSTR           szDest,
    JET_PFNSTATUS       pfn );

w
wchar_t*    sz EsetestWidenString EsetestCleanupWidenString
wchar_t*    szDest EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetRestoreInstance(     
        JET_INSTANCE instance,
        const char *sz,
        const char *szDest,
        JET_PFNSTATUS pfn );

n
JET_ERR JET_API JetSetIndexRange(JET_SESID sesid,
    JET_TABLEID tableidSrc, JET_GRBIT grbit);

n
JET_ERR JET_API JetIndexRecordCount(JET_SESID sesid,
    JET_TABLEID tableid, unsigned long *pcrec, unsigned long crecMax );

n
JET_ERR JET_API JetRetrieveKey(
    JET_SESID sesid,
    JET_TABLEID tableid,
    __out_bcount_part(cbMax, *pcbActual) void *pvData,
    unsigned long cbMax,
    unsigned long *pcbActual,
    JET_GRBIT grbit );

n
JET_ERR JET_API JetBeginExternalBackup( JET_GRBIT grbit );

n
JET_ERR JET_API JetBeginExternalBackupInstance( JET_INSTANCE instance, JET_GRBIT grbit );

n
JET_ERR JET_API JetGetAttachInfo(
    __out_bcount_part_opt(cbMax, *pcbActual) void *pv,
    unsigned long cbMax,
    unsigned long *pcbActual );

n
JET_ERR JET_API JetGetAttachInfoInstance(   JET_INSTANCE    instance,
    __out_bcount_part_opt(cbMax, *pcbActual) void           *pv,
    unsigned long   cbMax,
    unsigned long   *pcbActual );

w
wchar_t*    szFileName  EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetOpenFile( const char *szFileName,
    JET_HANDLE  *phfFile,
    unsigned long *pulFileSizeLow,
    unsigned long *pulFileSizeHigh );

w
wchar_t*    szFileName  EsetestWidenString  EsetestCleanupWidenString
JET_ERR JET_API JetOpenFileInstance( JET_INSTANCE instance,
                const char *szFileName,
                JET_HANDLE  *phfFile,
                unsigned long *pulFileSizeLow,
                unsigned long *pulFileSizeHigh );

w
wchar_t*    szFile EsetestWidenString EsetestCleanupWidenString
JET_ERR JET_API JetOpenFileSectionInstance(
    JET_INSTANCE    instance,
    __in JET_PSTR   szFile,
    JET_HANDLE *    phFile,
    long        iSection,
    long        cSections,
    unsigned long * pulSectionSizeLow,
    long *      plSectionSizeHigh );

n
JET_ERR JET_API JetReadFile( JET_HANDLE hfFile,
    __out_bcount_part(cb, *pcbActual ) void *pv,
    unsigned long cb,
    unsigned long *pcbActual );

n
JET_ERR JET_API JetReadFileInstance(    JET_INSTANCE instance,
                                        JET_HANDLE hfFile,
    __out_bcount_part(cb, *pcb) void *pv,
                                        unsigned long cb,
                                        unsigned long *pcb );



n
JET_ERR JET_API JetCloseFile( JET_HANDLE hfFile );

n
JET_ERR JET_API JetCloseFileInstance( JET_INSTANCE instance, JET_HANDLE hfFile );

n
JET_ERR JET_API JetGetLogInfo(
    __out_bcount_part_opt(cbMax, *pcbActual) void *pv,
    unsigned long cbMax,
    unsigned long *pcbActual );

n
JET_ERR JET_API JetGetLogInfoInstance(  JET_INSTANCE instance,
                                        __out_bcount_part_opt(cbMax, *pcbActual) void *pv,
                                        unsigned long cbMax,
                                        unsigned long *pcbActual );

n
JET_ERR JET_API JetGetLogInfoInstance2( JET_INSTANCE instance,
                                        __out_bcount_part_opt(cbMax, *pcbActual) void *pv,
                                        unsigned long cbMax,
                                        unsigned long *pcbActual,
                                        JET_LOGINFO * pLogInfo);

n
JET_ERR JET_API JetGetTruncateLogInfoInstance(  JET_INSTANCE instance,
                                                __out_bcount_part_opt(cbMax, *pcbActual) void *pv,
                                                unsigned long cbMax,
                                                unsigned long *pcbActual );

n
JET_ERR JET_API JetTruncateLog( void );

n
JET_ERR JET_API JetTruncateLogInstance( JET_INSTANCE instance );

n
JET_ERR JET_API JetEndExternalBackup( void );

n
JET_ERR JET_API JetEndExternalBackupInstance( JET_INSTANCE instance );

n
JET_ERR JET_API JetEndExternalBackupInstance2( JET_INSTANCE instance, JET_GRBIT grbit );

c
wchar_t*    szCheckpointFilePath    EsetestWidenString EsetestCleanupWidenString
wchar_t*    szLogPath               EsetestWidenString EsetestCleanupWidenString
wchar_t*    szBackupLogPath         EsetestWidenString EsetestCleanupWidenString
JET_RSTMAP_W*   rgrstmap,crstfilemap            EsetestWidenJET_RSTMAP  EsetestUnwidenJET_RSTMAP
JET_ERR JET_API JetExternalRestore(
    __in JET_PSTR   szCheckpointFilePath,
    __in JET_PSTR   szLogPath,
    JET_RSTMAP *    rgrstmap,
    long        crstfilemap,
    __in JET_PSTR   szBackupLogPath,
    long        genLow,
    long        genHigh,
    JET_PFNSTATUS   pfn );

c
wchar_t*    szCheckpointFilePath    EsetestWidenString EsetestCleanupWidenString
wchar_t*    szLogPath               EsetestWidenString EsetestCleanupWidenString
wchar_t*    szBackupLogPath         EsetestWidenString EsetestCleanupWidenString
wchar_t*    szTargetInstanceName                EsetestWidenString EsetestCleanupWidenString
wchar_t*    szTargetInstanceLogPath             EsetestWidenString EsetestCleanupWidenString
wchar_t*    szTargetInstanceCheckpointPath      EsetestWidenString EsetestCleanupWidenString
JET_RSTMAP_W*   rgrstmap,crstfilemap            EsetestWidenJET_RSTMAP  EsetestUnwidenJET_RSTMAP
JET_ERR JET_API JetExternalRestore2(
    __in JET_PSTR   szCheckpointFilePath,
    __in JET_PSTR   szLogPath,
    JET_RSTMAP *    rgrstmap,
    long        crstfilemap,
    __in JET_PSTR   szBackupLogPath,
    JET_LOGINFO *   pLogInfo,
    __in JET_PSTR   szTargetInstanceName,
    __in JET_PSTR   szTargetInstanceLogPath,
    __in JET_PSTR   szTargetInstanceCheckpointPath,
    JET_PFNSTATUS pfn );




n
JET_ERR JET_API JetRegisterCallback(
    JET_SESID               sesid,
    JET_TABLEID             tableid,
    JET_CBTYP               cbtyp,
    JET_CALLBACK            pCallback,
    void *                  pvContext,
    JET_HANDLE              *phCallbackId );


n
JET_ERR JET_API JetUnregisterCallback(
    JET_SESID               sesid,
    JET_TABLEID             tableid,
    JET_CBTYP               cbtyp,
    JET_HANDLE              hCallbackId );

n
JET_ERR JET_API JetGetInstanceInfo( unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo );

n
JET_ERR JET_API JetFreeBuffer( __notnull char * pbBuf );

n
JET_ERR JET_API JetSetLS(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_LS          ls,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetGetLS(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_LS          *pls,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetTracing(
    const JET_TRACEOP   traceop,
    const JET_TRACETAG  tracetag,
    const JET_API_PTR   ul );

n
JET_ERR JET_API JetOSSnapshotPrepare( JET_OSSNAPID * psnapId, const JET_GRBIT grbit );

n
JET_ERR JET_API JetOSSnapshotPrepareInstance( JET_OSSNAPID snapId, JET_INSTANCE instance, const JET_GRBIT grbit );

n
JET_ERR JET_API JetOSSnapshotFreeze( const JET_OSSNAPID snapId, unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo, const JET_GRBIT grbit );

n
JET_ERR JET_API JetOSSnapshotThaw( const JET_OSSNAPID snapId, const JET_GRBIT grbit );

n
JET_ERR JET_API JetOSSnapshotAbort( const JET_OSSNAPID snapId, const JET_GRBIT grbit );

n
JET_ERR JET_API JetOSSnapshotTruncateLog( const JET_OSSNAPID snapId, const JET_GRBIT grbit );

n
JET_ERR JET_API JetOSSnapshotTruncateLogInstance( const JET_OSSNAPID snapId, JET_INSTANCE   instance, const JET_GRBIT grbit );

n
JET_ERR JET_API JetOSSnapshotGetFreezeInfo( const JET_OSSNAPID snapId, unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo, const JET_GRBIT grbit );

n
JET_ERR JET_API JetOSSnapshotEnd( const JET_OSSNAPID snapId, const JET_GRBIT grbit );

n
JET_ERR JET_API JetBeginSurrogateBackup(
    JET_INSTANCE    instance,
    unsigned long       lgenFirst,
    unsigned long       lgenLast,
    JET_GRBIT       grbit );

n
JET_ERR JET_API JetEndSurrogateBackup(
    JET_INSTANCE    instance,
    JET_GRBIT       grbit );

w
wchar_t*    szLog EsetestWidenString    EsetestCleanupWidenString
JET_ERR JET_API JetGetLogFileInfo( char *szLog, 
    __out_bcount(cbMax) void *pvResult,
    const unsigned long cbMax,
    const unsigned long InfoLevel );

n
JET_ERR JET_API JetPrereadKeys(
    __in JET_SESID                              sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount(ckeys) const void **                rgpvKeys,
    __in_ecount(ckeys) const unsigned long *            rgcbKeys,
    __in long                                       ckeys,
    __out_opt long *                                pckeysPreread,
    __in JET_GRBIT                              grbit );

n
JET_ERR JET_API JetConsumeLogData(
    __in    JET_INSTANCE        instance,
    __in    JET_EMITDATACTX *   pEmitLogDataCtx,
    __in    void *              pvLogData,
    __in    unsigned long       cbLogData,
    __in    JET_GRBIT           grbits );

n
JET_ERR JET_API JetTestHook(
    __in        const TESTHOOK_OP   opcode,
    __inout_opt void * const        pv );

