// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#ifndef ESETEST_BOUNCE_HXX_INCLUDED

#ifdef  __cplusplus
extern "C" {
#endif

#include "ese_common.hxx"
#include "esetest_bounce.hxx"
#include <stdlib.h>
#include <strsafe.h>
#include <windows.h>

JET_ERR
BounceJetInit(
    JET_INSTANCE *pinstance
);

JET_ERR
BounceJetInit2(
    JET_INSTANCE *pinstance,
    JET_GRBIT grbit
);

JET_ERR
BounceJetInit3(
    JET_INSTANCE *pinstance,
    JET_RSTINFO *prstInfo,
    JET_GRBIT grbit
);

JET_ERR
BounceJetInit4(
    JET_INSTANCE *pinstance,
    JET_RSTINFO2 *prstInfo,
    JET_GRBIT grbit
);

JET_ERR
BounceJetCreateInstance(
    JET_INSTANCE *pinstance,
    const char * szInstanceName
);

JET_ERR
BounceJetCreateInstance2(
    JET_INSTANCE *pinstance,
    const char * szInstanceName,
    const char * szDisplayName,
    JET_GRBIT grbit
);

JET_ERR
BounceJetGetInstanceMiscInfo(
    JET_INSTANCE    instance,
    void *          pvResult,
    unsigned long       cbMax,
    unsigned long       InfoLevel
);

JET_ERR
BounceJetTerm(
    JET_INSTANCE instance
);

JET_ERR
BounceJetTerm2(
    JET_INSTANCE instance,
    JET_GRBIT grbit
);

JET_ERR
BounceJetStopService(
    
);

JET_ERR
BounceJetStopServiceInstance(
    JET_INSTANCE instance
);

JET_ERR
BounceJetStopServiceInstance2(
    JET_INSTANCE instance,
    JET_GRBIT grbit
);

JET_ERR
BounceJetStopBackup(
    
);

JET_ERR
BounceJetStopBackupInstance(
    JET_INSTANCE instance
);

JET_ERR
BounceJetSetSystemParameter(
    JET_INSTANCE    *pinstance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCSTR      szParam
);

JET_ERR
BounceJetGetSystemParameter(
    JET_INSTANCE    instance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __out_bcount_opt(cbMax) JET_API_PTR     *plParam,
    __out_bcount_opt(cbMax) JET_PSTR        szParam,
    unsigned long   cbMax
);

JET_ERR
BounceJetSetSessionParameter(
    JET_SESID           sesid,
    unsigned long       sesparamid,
    void *              pvParam,
    unsigned long       cbParam
);

JET_ERR
BounceJetSetResourceParam(
    JET_INSTANCE    instance,
    JET_RESOPER     resoper,
    JET_RESID       resid,
    JET_API_PTR     ulParam
);

JET_ERR
BounceJetGetResourceParam(
    JET_INSTANCE    instance,
    JET_RESOPER     resoper,
    JET_RESID       resid,
    JET_API_PTR     *pulParam
);

JET_ERR
BounceJetEnableMultiInstance(
    __in_ecount(csetsysparam) JET_SETSYSPARAM   *psetsysparam,
    unsigned long       csetsysparam,
    unsigned long *pcsetsucceed
);

JET_ERR
BounceJetResetCounter(
    JET_SESID sesid,
    long CounterType
);

JET_ERR
BounceJetGetCounter(
    JET_SESID sesid,
    long CounterType,
    long *plValue
);

JET_ERR
BounceJetGetThreadStats(
    __out_bcount(cbMax) void*           pvResult,
    unsigned long   cbMax
);

JET_ERR
BounceJetBeginSession(
    JET_INSTANCE    instance,
    JET_SESID       *psesid,
    __in_opt JET_PCSTR  szUserName,
    __in_opt JET_PCSTR  szPassword
);

JET_ERR
BounceJetDupSession(
    JET_SESID sesid,
    JET_SESID *psesid
);

JET_ERR
BounceJetEndSession(
    JET_SESID sesid,
    JET_GRBIT grbit
);

JET_ERR
BounceJetGetSessionInfo(
    JET_SESID           sesid,
    __out_bcount(cbMax) void *  pvResult,
    const unsigned long cbMax,
    const unsigned long ulInfoLevel
);

JET_ERR
BounceJetGetVersion(
    JET_SESID sesid,
    unsigned long *pwVersion
);

JET_ERR
BounceJetIdle(
    JET_SESID sesid,
    JET_GRBIT grbit
);

JET_ERR
BounceJetCreateDatabase(
    JET_SESID       sesid,
    const char      *szFilename,
    const char      *szConnect,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetCreateDatabase2(
    JET_SESID       sesid,
    const char      *szFilename,
    const unsigned long cpgDatabaseSizeMax,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetCreateDatabaseWithStreaming(
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const char      *szSLVRootName,
    const unsigned long cpgDatabaseSizeMax,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetAttachDatabase(
    JET_SESID       sesid,
    const char      *szFilename,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetAttachDatabase2(
    JET_SESID       sesid,
    const char      *szFilename,
    const unsigned long cpgDatabaseSizeMax,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetAttachDatabase3(
    JET_SESID       sesid,
    const char      *szFilename,
    const JET_SETDBPARAM* const rgsetdbparam,
    const unsigned long csetdbparam,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetAttachDatabaseWithStreaming(
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const char      *szSLVRootName,
    const unsigned long cpgDatabaseSizeMax,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetDetachDatabase(
    JET_SESID       sesid,
    const char      *szFilename
);

JET_ERR
BounceJetDetachDatabase2(
    JET_SESID       sesid,
    const char      *szFilename,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetGetObjectInfo(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_OBJTYP      objtyp,
    const char      *szContainerName,
    const char      *szObjectName,
    __out_bcount(cbMax) void *  pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel
);

JET_ERR
BounceJetGetTableInfo(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount(cbMax) void *  pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel
);

JET_ERR
BounceJetGetPageInfo(
    __in_bcount(cbData)         const void *    pvPages,
    unsigned long   cbData,
    __inout_bcount(cbPageInfo)  JET_PAGEINFO *  rgPageInfo,
    unsigned long   cbPageInfo,
    JET_GRBIT       grbit,
    unsigned long   ulInfoLevel
);

JET_ERR
BounceJetGetPageInfo2(
    __in_bcount(cbData)         const void *    pvPages,
    unsigned long   cbData,
    __inout_bcount(cbPageInfo)  JET_PAGEINFO *  rgPageInfo,
    unsigned long   cbPageInfo,
    JET_GRBIT       grbit,
    unsigned long   ulInfoLevel
);

JET_ERR
BounceJetRemoveLogfile(
    __in JET_PCSTR szDatabase,
    __in JET_PCSTR szLogfile,
    __in JET_GRBIT grbit
);

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
);

JET_ERR
BounceJetCreateTable(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   lPages,
    unsigned long   lDensity,
    JET_TABLEID     *ptableid
);

JET_ERR
BounceJetCreateTableColumnIndex(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE *ptablecreate
);

JET_ERR
BounceJetCreateTableColumnIndex2(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE2    *ptablecreate
);

JET_ERR
BounceJetCreateTableColumnIndex3(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE3    *ptablecreate
);

JET_ERR
BounceJetCreateTableColumnIndex5(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLECREATE5    *ptablecreate
);

JET_ERR
BounceJetDeleteTable(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName
);

JET_ERR
BounceJetRenameTable(
    JET_SESID sesid,
    JET_DBID dbid,
    const char *szName,
    const char *szNameNew
);

JET_ERR
BounceJetGetTableColumnInfo(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName,
    __out_bcount(cbMax) void *  pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel
);

JET_ERR
BounceJetGetColumnInfo(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    const char      *szColumnName,
    __out_bcount(cbMax) void *  pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel
);

JET_ERR
BounceJetAddColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName,
    const JET_COLUMNDEF *pcolumndef,
    __in_bcount_opt(cbDefault) const void*      pvDefault,
    unsigned long   cbDefault,
    __out_opt JET_COLUMNID  *pcolumnid
);

JET_ERR
BounceJetDeleteColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName
);

JET_ERR
BounceJetDeleteColumn2(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumnName,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetRenameColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szName,
    __in JET_PCSTR  szNameNew,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetSetColumnDefaultValue(
    JET_SESID           sesid,
    JET_DBID            dbid,
    __in JET_PCSTR      szTableName,
    __in JET_PCSTR      szColumnName,
    __in_bcount(cbData) const void          *pvData,
    const unsigned long cbData,
    const JET_GRBIT     grbit
);

JET_ERR
BounceJetGetTableIndexInfo(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    __out_bcount(cbResult) void         *pvResult,
    unsigned long   cbResult,
    unsigned long   InfoLevel
);

JET_ERR
BounceJetGetIndexInfo(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    const char      *szIndexName,
    __out_bcount(cbResult) void         *pvResult,
    unsigned long   cbResult,
    unsigned long   InfoLevel
);

JET_ERR
BounceJetCreateIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szIndexName,
    JET_GRBIT       grbit,
    const char      *szKey,
    unsigned long   cbKey,
    unsigned long   lDensity
);

JET_ERR
BounceJetCreateIndex2(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in_ecount(cIndexCreate) JET_INDEXCREATE   *pindexcreate,
    unsigned long   cIndexCreate
);

JET_ERR
BounceJetDeleteIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in JET_PCSTR  szIndexName
);

JET_ERR
BounceJetBeginTransaction(
    JET_SESID sesid
);

JET_ERR
BounceJetBeginTransaction2(
    JET_SESID sesid,
    JET_GRBIT grbit
);

JET_ERR
BounceJetPrepareToCommitTransaction(
    JET_SESID       sesid,
    const void      * pvData,
    unsigned long   cbData,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetCommitTransaction(
    JET_SESID sesid,
    JET_GRBIT grbit
);

JET_ERR
BounceJetRollback(
    JET_SESID sesid,
    JET_GRBIT grbit
);

JET_ERR
BounceJetGetDatabaseInfo(
    JET_SESID       sesid,
    JET_DBID        dbid,
    void            *pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel
);

JET_ERR
BounceJetGetDatabaseFileInfo(
    const char      *szDatabaseName,
    void            *pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel
);

JET_ERR
BounceJetOpenDatabase(
    JET_SESID       sesid,
    const char      *szFilename,
    const char      *szConnect,
    JET_DBID        *pdbid,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetCloseDatabase(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetOpenTable(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    const void      *pvParameters,
    unsigned long   cbParameters,
    JET_GRBIT       grbit,
    JET_TABLEID     *ptableid
);

JET_ERR
BounceJetSetTableInfo(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const void      *pvParam,
    unsigned long   cbParam,
    unsigned long   InfoLevel
);

JET_ERR
BounceJetSetTableSequential(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetResetTableSequential(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetCloseTable(
    JET_SESID sesid,
    JET_TABLEID tableid
);

JET_ERR
BounceJetDelete(
    JET_SESID sesid,
    JET_TABLEID tableid
);

JET_ERR
BounceJetUpdate(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    void            *pvBookmark,
    unsigned long   cbBookmark,
    unsigned long   *pcbActual
);

JET_ERR
BounceJetUpdate2(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    void            *pvBookmark,
    unsigned long   cbBookmark,
    unsigned long   *pcbActual,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetEscrowUpdate(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_COLUMNID    columnid,
    void            *pv,
    unsigned long   cbMax,
    void            *pvOld,
    unsigned long   cbOldMax,
    unsigned long   *pcbOldActual,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetRetrieveColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_COLUMNID    columnid,
    void            *pvData,
    unsigned long   cbData,
    unsigned long   *pcbActual,
    JET_GRBIT       grbit,
    JET_RETINFO     *pretinfo
);

JET_ERR
BounceJetRetrieveColumns(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_RETRIEVECOLUMN  *pretrievecolumn,
    unsigned long   cretrievecolumn
);

JET_ERR
BounceJetEnumerateColumns(
    JET_SESID               sesid,
    JET_TABLEID             tableid,
    unsigned long           cEnumColumnId,
    JET_ENUMCOLUMNID*       rgEnumColumnId,
    unsigned long*          pcEnumColumn,
    JET_ENUMCOLUMN**        prgEnumColumn,
    JET_PFNREALLOC          pfnRealloc,
    void*                   pvReallocContext,
    unsigned long           cbDataMost,
    JET_GRBIT               grbit
);

JET_ERR
BounceJetRetrieveTaggedColumnList(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    unsigned long   *pcColumns,
    void            *pvData,
    unsigned long   cbData,
    JET_COLUMNID    columnidStart,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetGetRecordSize(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_RECSIZE *   precsize,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetSetColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_COLUMNID    columnid,
    const void      *pvData,
    unsigned long   cbData,
    JET_GRBIT       grbit,
    JET_SETINFO     *psetinfo
);

JET_ERR
BounceJetSetColumns(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_SETCOLUMN   *psetcolumn,
    unsigned long   csetcolumn
);

JET_ERR
BounceJetPrepareUpdate(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    unsigned long   prep
);

JET_ERR
BounceJetGetRecordPosition(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_RECPOS      *precpos,
    unsigned long   cbRecpos
);

JET_ERR
BounceJetGotoPosition(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_RECPOS      *precpos
);

JET_ERR
BounceJetGetCursorInfo(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount(cbMax) void            *pvResult,
    unsigned long   cbMax,
    unsigned long   InfoLevel
);

JET_ERR
BounceJetDupCursor(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_TABLEID     *ptableid,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetGetCurrentIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount(cchIndexName) JET_PSTR szIndexName,
    unsigned long   cchIndexName
);

JET_ERR
BounceJetSetCurrentIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName
);

JET_ERR
BounceJetSetCurrentIndex2(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetSetCurrentIndex3(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_GRBIT       grbit,
    unsigned long   itagSequence
);

JET_ERR
BounceJetSetCurrentIndex4(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    JET_INDEXID     *pindexid,
    JET_GRBIT       grbit,
    unsigned long   itagSequence
);

JET_ERR
BounceJetMove(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    long            cRow,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetGetLock(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetMakeKey(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in_bcount(cbData) const void      *pvData,
    unsigned long   cbData,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetSeek(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetGetBookmark(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount_part_opt(cbMax,
    *pcbActual) void *          pvBookmark,
    unsigned long   cbMax,
    unsigned long * pcbActual
);

JET_ERR
BounceJetGetSecondaryIndexBookmark(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __out_bcount_part(cbSecondaryKeyMax,
    *pcbSecondaryKeyActual) void *          pvSecondaryKey,
    unsigned long   cbSecondaryKeyMax,
    unsigned long * pcbSecondaryKeyActual,
    __out_bcount_part(cbPrimaryBookmarkMax,
    *pcbPrimaryKeyActual) void *            pvPrimaryBookmark,
    unsigned long   cbPrimaryBookmarkMax,
    unsigned long * pcbPrimaryKeyActual,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetCompact(
    JET_SESID       sesid,
    const char      *szDatabaseSrc,
    const char      *szDatabaseDest,
    JET_PFNSTATUS   pfnStatus,
    JET_CONVERT     *pconvert,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetDefragment(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetDefragment2(
    JET_SESID       sesid,
    JET_DBID        dbid,
    const char      *szTableName,
    unsigned long   *pcPasses,
    unsigned long   *pcSeconds,
    JET_CALLBACK    callback,
    JET_GRBIT       grbit
);

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
);

JET_ERR
BounceJetDatabaseScan(
    JET_SESID       sesid,
    JET_DBID        dbid,
    unsigned long * pcSeconds,
    unsigned long   cmsecSleep,
    JET_CALLBACK    pfnCallback,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetConvertDDL(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_OPDDLCONV   convtyp,
    __out_bcount(cbData) void           *pvData,
    unsigned long   cbData
);

JET_ERR
BounceJetUpgradeDatabase(
    JET_SESID       sesid,
    const char      *szDbFileName,
    const char      *szSLVFileName,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetSetMaxDatabaseSize(
    JET_SESID       sesid,
    JET_DBID        dbid,
    unsigned long   cpg,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetGetMaxDatabaseSize(
    JET_SESID       sesid,
    JET_DBID        dbid,
    unsigned long   * pcpg,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetSetDatabaseSize(
    JET_SESID       sesid,
    const char      *szDatabaseName,
    unsigned long   cpg,
    unsigned long   *pcpgReal
);

JET_ERR
BounceJetGrowDatabase(
    JET_SESID       sesid,
    JET_DBID        dbid,
    unsigned long   cpg,
    unsigned long   *pcpgReal
);

JET_ERR
BounceJetResizeDatabase(
    JET_SESID       sesid,
    JET_DBID        dbid,
    unsigned long   cpg,
    unsigned long   *pcpgActual,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetSetSessionContext(
    JET_SESID       sesid,
    JET_API_PTR     ulContext
);

JET_ERR
BounceJetResetSessionContext(
    JET_SESID       sesid
);

JET_ERR
BounceJetDBUtilities(
    JET_DBUTIL *pdbutil
);

JET_ERR
BounceJetGotoBookmark(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in_bcount(cbBookmark) void *          pvBookmark,
    unsigned long   cbBookmark
);

JET_ERR
BounceJetGotoSecondaryIndexBookmark(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    __in_bcount(cbSecondaryKey) void *          pvSecondaryKey,
    unsigned long   cbSecondaryKey,
    __in_bcount(cbPrimaryBookmark) void *           pvPrimaryBookmark,
    unsigned long   cbPrimaryBookmark,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetIntersectIndexes(
    JET_SESID sesid,
    __in_ecount(cindexrange) JET_INDEXRANGE * rgindexrange,
    unsigned long cindexrange,
    JET_RECORDLIST * precordlist,
    JET_GRBIT grbit
);

JET_ERR
BounceJetComputeStats(
    JET_SESID sesid,
    JET_TABLEID tableid
);

JET_ERR
BounceJetOpenTempTable(
    JET_SESID sesid,
    __in_ecount(ccolumn) const JET_COLUMNDEF *prgcolumndef,
    unsigned long ccolumn,
    JET_GRBIT grbit,
    JET_TABLEID *ptableid,
    JET_COLUMNID *prgcolumnid
);

JET_ERR
BounceJetOpenTempTable2(
    JET_SESID           sesid,
    __in_ecount(ccolumn) const JET_COLUMNDEF    *prgcolumndef,
    unsigned long       ccolumn,
    unsigned long       lcid,
    JET_GRBIT           grbit,
    JET_TABLEID         *ptableid,
    JET_COLUMNID        *prgcolumnid
);

JET_ERR
BounceJetOpenTempTable3(
    JET_SESID           sesid,
    __in_ecount(ccolumn) const JET_COLUMNDEF    *prgcolumndef,
    unsigned long       ccolumn,
    JET_UNICODEINDEX    *pidxunicode,
    JET_GRBIT           grbit,
    JET_TABLEID         *ptableid,
    JET_COLUMNID        *prgcolumnid
);

JET_ERR
BounceJetOpenTemporaryTable(
    JET_SESID               sesid,
    JET_OPENTEMPORARYTABLE  *popentemporarytable
);

JET_ERR
BounceJetBackup(
    const char *szBackupPath,
    JET_GRBIT grbit,
    JET_PFNSTATUS pfnStatus
);

JET_ERR
BounceJetBackupInstance(
    JET_INSTANCE    instance,
    const char      *szBackupPath,
    JET_GRBIT       grbit,
    JET_PFNSTATUS   pfnStatus
);

JET_ERR
BounceJetRestore(
    __in JET_PCSTR      sz,
    JET_PFNSTATUS       pfn
);

JET_ERR
BounceJetRestore2(
    JET_PCSTR           sz,
    JET_PCSTR           szDest,
    JET_PFNSTATUS       pfn
);

JET_ERR
BounceJetRestoreInstance(
    JET_INSTANCE instance,
    const char *sz,
    const char *szDest,
    JET_PFNSTATUS pfn
);

JET_ERR
BounceJetSetIndexRange(
    JET_SESID sesid,
    JET_TABLEID tableidSrc,
    JET_GRBIT grbit
);

JET_ERR
BounceJetIndexRecordCount(
    JET_SESID sesid,
    JET_TABLEID tableid,
    unsigned long *pcrec,
    unsigned long crecMax
);

JET_ERR
BounceJetRetrieveKey(
    JET_SESID sesid,
    JET_TABLEID tableid,
    __out_bcount_part(cbMax,
    *pcbActual) void *pvData,
    unsigned long cbMax,
    unsigned long *pcbActual,
    JET_GRBIT grbit
);

JET_ERR
BounceJetBeginExternalBackup(
    JET_GRBIT grbit
);

JET_ERR
BounceJetBeginExternalBackupInstance(
    JET_INSTANCE instance,
    JET_GRBIT grbit
);

JET_ERR
BounceJetGetAttachInfo(
    __out_bcount_part_opt(cbMax,
    *pcbActual) void *pv,
    unsigned long cbMax,
    unsigned long *pcbActual
);

JET_ERR
BounceJetGetAttachInfoInstance(
    JET_INSTANCE    instance,
    __out_bcount_part_opt(cbMax,
    *pcbActual) void            *pv,
    unsigned long   cbMax,
    unsigned long   *pcbActual
);

JET_ERR
BounceJetOpenFile(
    const char *szFileName,
    JET_HANDLE  *phfFile,
    unsigned long *pulFileSizeLow,
    unsigned long *pulFileSizeHigh
);

JET_ERR
BounceJetOpenFileInstance(
    JET_INSTANCE instance,
    const char *szFileName,
    JET_HANDLE  *phfFile,
    unsigned long *pulFileSizeLow,
    unsigned long *pulFileSizeHigh
);

JET_ERR
BounceJetOpenFileSectionInstance(
    JET_INSTANCE    instance,
    __in JET_PSTR   szFile,
    JET_HANDLE *    phFile,
    long        iSection,
    long        cSections,
    unsigned long * pulSectionSizeLow,
    long *      plSectionSizeHigh
);

JET_ERR
BounceJetReadFile(
    JET_HANDLE hfFile,
    __out_bcount_part(cb,
    *pcbActual ) void *pv,
    unsigned long cb,
    unsigned long *pcbActual
);

JET_ERR
BounceJetReadFileInstance(
    JET_INSTANCE instance,
    JET_HANDLE hfFile,
    __out_bcount_part(cb,
    *pcb) void *pv,
    unsigned long cb,
    unsigned long *pcb
);

JET_ERR
BounceJetCloseFile(
    JET_HANDLE hfFile
);

JET_ERR
BounceJetCloseFileInstance(
    JET_INSTANCE instance,
    JET_HANDLE hfFile
);

JET_ERR
BounceJetGetLogInfo(
    __out_bcount_part_opt(cbMax,
    *pcbActual) void *pv,
    unsigned long cbMax,
    unsigned long *pcbActual
);

JET_ERR
BounceJetGetLogInfoInstance(
    JET_INSTANCE instance,
    __out_bcount_part_opt(cbMax,
    *pcbActual) void *pv,
    unsigned long cbMax,
    unsigned long *pcbActual
);

JET_ERR
BounceJetGetLogInfoInstance2(
    JET_INSTANCE instance,
    __out_bcount_part_opt(cbMax,
    *pcbActual) void *pv,
    unsigned long cbMax,
    unsigned long *pcbActual,
    JET_LOGINFO * pLogInfo
);

JET_ERR
BounceJetGetTruncateLogInfoInstance(
    JET_INSTANCE instance,
    __out_bcount_part_opt(cbMax,
    *pcbActual) void *pv,
    unsigned long cbMax,
    unsigned long *pcbActual
);

JET_ERR
BounceJetTruncateLog(
    void
);

JET_ERR
BounceJetTruncateLogInstance(
    JET_INSTANCE instance
);

JET_ERR
BounceJetEndExternalBackup(
    void
);

JET_ERR
BounceJetEndExternalBackupInstance(
    JET_INSTANCE instance
);

JET_ERR
BounceJetEndExternalBackupInstance2(
    JET_INSTANCE instance,
    JET_GRBIT grbit
);

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
);

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
);

JET_ERR
BounceJetRegisterCallback(
    JET_SESID               sesid,
    JET_TABLEID             tableid,
    JET_CBTYP               cbtyp,
    JET_CALLBACK            pCallback,
    void *                  pvContext,
    JET_HANDLE              *phCallbackId
);

JET_ERR
BounceJetUnregisterCallback(
    JET_SESID               sesid,
    JET_TABLEID             tableid,
    JET_CBTYP               cbtyp,
    JET_HANDLE              hCallbackId
);

JET_ERR
BounceJetGetInstanceInfo(
    unsigned long *pcInstanceInfo,
    JET_INSTANCE_INFO ** paInstanceInfo
);

JET_ERR
BounceJetFreeBuffer(
    __notnull char * pbBuf
);

JET_ERR
BounceJetSetLS(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_LS          ls,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetGetLS(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_LS          *pls,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetTracing(
    const JET_TRACEOP   traceop,
    const JET_TRACETAG  tracetag,
    const JET_API_PTR   ul
);

JET_ERR
BounceJetOSSnapshotPrepare(
    JET_OSSNAPID * psnapId,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetOSSnapshotPrepareInstance(
    JET_OSSNAPID snapId,
    JET_INSTANCE    instance,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetOSSnapshotFreeze(
    const JET_OSSNAPID snapId,
    unsigned long *pcInstanceInfo,
    JET_INSTANCE_INFO ** paInstanceInfo,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetOSSnapshotThaw(
    const JET_OSSNAPID snapId,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetOSSnapshotAbort(
    const JET_OSSNAPID snapId,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetOSSnapshotTruncateLog(
    const JET_OSSNAPID snapId,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetOSSnapshotTruncateLogInstance(
    const JET_OSSNAPID snapId,
    JET_INSTANCE    instance,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetOSSnapshotGetFreezeInfo(
    const JET_OSSNAPID snapId,
    unsigned long *pcInstanceInfo,
    JET_INSTANCE_INFO ** paInstanceInfo,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetOSSnapshotEnd(
    const JET_OSSNAPID snapId,
    const JET_GRBIT grbit
);

JET_ERR
BounceJetBeginSurrogateBackup(
    JET_INSTANCE    instance,
    unsigned long       lgenFirst,
    unsigned long       lgenLast,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetEndSurrogateBackup(
    JET_INSTANCE    instance,
    JET_GRBIT       grbit
);

JET_ERR
BounceJetGetLogFileInfo(
    char *szLog,
    __out_bcount(cbMax) void *pvResult,
    const unsigned long cbMax,
    const unsigned long InfoLevel
);

JET_ERR
BounceJetPrereadKeys(
    __in JET_SESID                              sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount(ckeys) const void **                rgpvKeys,
    __in_ecount(ckeys) const unsigned long *            rgcbKeys,
    __in long                                       ckeys,
    __out_opt long *                                pckeysPreread,
    __in JET_GRBIT                              grbit
);

JET_ERR
BounceJetConsumeLogData(
    __in    JET_INSTANCE        instance,
    __in    JET_EMITDATACTX *   pEmitLogDataCtx,
    __in    void *              pvLogData,
    __in    unsigned long       cbLogData,
    __in    JET_GRBIT           grbits
);

JET_ERR
BounceJetTestHook(
    __in        const TESTHOOK_OP   opcode,
    __inout_opt void * const        pv
);

#ifdef  __cplusplus
}
#endif
#endif