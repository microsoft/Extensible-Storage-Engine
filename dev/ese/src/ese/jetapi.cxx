// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "errdata.hxx"
#include "_bf.hxx"

#ifdef ESENT
#include "slpolicylist.h"
#else
const LOCAL WCHAR g_wszLP_PARAMCONFIGURATION[] = L"ExtensibleStorageEngine-ISAM-ParamConfiguration";
#endif





JET_ERR ErrERRLookupErrorCategory(
    __in const JET_ERR errLookup,
    __out JET_ERRCAT* perrortype
    )
{
    if ( !FERRLookupErrorCategory( errLookup, perrortype ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    return JET_errSuccess;
}

JET_ERR ErrERRLookupErrorHierarchy(
    __in JET_ERRCAT             errortype,
    __out_bcount(8) BYTE* const pbHierarchy
    )
{
    if ( !FERRLookupErrorHierarchy( errortype, pbHierarchy ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    return JET_errSuccess;
}

#if !defined( RTM ) || defined( DEBUG )
LGPOS   g_lgposRedoTrap     = lgposMax;

#define SYSTEM_PIB_RFS

#endif


#ifdef SYSTEM_PIB_RFS
INT g_cSystemPibAlloc       = 0;
INT g_cSystemPibAllocFail   = 0;
#endif


#pragma warning(disable: 4100)           


ERR VTAPI ErrIllegalAddColumn(JET_SESID sesid, JET_VTID vtid,
    const char  *szColumn, const JET_COLUMNDEF  *pcolumndef,
    const void  *pvDefault, ULONG cbDefault,
    JET_COLUMNID  *pcolumnid)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalCloseTable(JET_SESID vsesid, JET_VTID vtid)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalComputeStats(JET_SESID vsesid, JET_VTID vtid)
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalCopyBookmarks(JET_SESID sesid, JET_VTID vtidSrc,
    JET_TABLEID tableidDest, JET_COLUMNID columnidDest, ULONG crecMax)
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalCreateIndex(JET_SESID, JET_VTID, JET_INDEXCREATE3_A *, ULONG )
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalDelete(JET_SESID vsesid, JET_VTID vtid)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalDeleteColumn(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    const char      *szColumn,
    const JET_GRBIT grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalDeleteIndex(JET_SESID vsesid, JET_VTID vtid,
    const char  *szIndexName)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalDupCursor(JET_SESID vsesid, JET_VTID vtid,
    JET_TABLEID  *ptableid, JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalEscrowUpdate( JET_SESID vsesid,
    JET_VTID vtid,
    JET_COLUMNID columnid,
    void *pv,
    ULONG cbMax,
    void *pvOld,
    ULONG cbOldMax,
    ULONG *pcbOldActual,
    JET_GRBIT grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalGetBookmark(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    VOID * const    pvBookmark,
    const ULONG     cbMax,
    ULONG * const   pcbActual )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalGetIndexBookmark(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    VOID * const    pvSecondaryKey,
    const ULONG     cbSecondaryKeyMax,
    ULONG * const   pcbSecondaryKeyActual,
    VOID * const    pvPrimaryBookmark,
    const ULONG     cbPrimaryBookmarkMax,
    ULONG * const   pcbPrimaryBookmarkActual )
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalGetChecksum(JET_SESID vsesid, JET_VTID vtid,
    ULONG  *pChecksum)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalGetCurrentIndex(JET_SESID vsesid, JET_VTID vtid,
    _Out_z_bytecap_(cchIndexName) PSTR szIndexName, ULONG cchIndexName)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalGetCursorInfo(JET_SESID vsesid, JET_VTID vtid,
    void  *pvResult, ULONG cbMax, ULONG InfoLevel)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalGetRecordPosition(JET_SESID vsesid, JET_VTID vtid,
    JET_RECPOS  *pkeypos, ULONG cbKeypos)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalGetTableColumnInfo(JET_SESID vsesid, JET_VTID vtid,
    const char  *szColumnName, const JET_COLUMNID *pcolid, void  *pvResult,
    ULONG cbMax, ULONG InfoLevel, const BOOL fUnicodeNames )
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalGetTableIndexInfo(JET_SESID vsesid, JET_VTID vtid,
    const char  *szIndexName, void  *pvResult,
    ULONG cbMax, ULONG InfoLevel, const BOOL fUnicodeNames )
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalGetTableInfo(JET_SESID vsesid, JET_VTID vtid,
    void  *pvResult, ULONG cbMax, ULONG InfoLevel)
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalSetTableInfo(JET_SESID sesid, JET_VTID vtid,
    _In_reads_bytes_opt_(cbParam) const void  *pvParam, ULONG cbParam, ULONG InfoLevel)
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalGotoBookmark(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    const VOID * const  pvBookmark,
    const ULONG         cbBookmark )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalGotoIndexBookmark(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    const VOID * const  pvSecondaryKey,
    const ULONG         cbSecondaryKey,
    const VOID * const  pvPrimaryBookmark,
    const ULONG         cbPrimaryBookmark,
    const JET_GRBIT     grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalGotoPosition(JET_SESID vsesid, JET_VTID vtid,
    JET_RECPOS *precpos)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalVtIdle(JET_SESID vsesid, JET_VTID vtid)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalMakeKey(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    const VOID*     pvData,
    const ULONG     cbData,
    JET_GRBIT       grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalGetLock(JET_SESID vsesid, JET_VTID vtid, JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalMove(JET_SESID vsesid, JET_VTID vtid,
    LONG cRow, JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalSetCursorFilter(JET_SESID vsesid, JET_VTID vtid,
    JET_INDEX_COLUMN *rgFilters, DWORD cFilters, JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalNotifyBeginTrans(JET_SESID vsesid, JET_VTID vtid)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalNotifyCommitTrans(JET_SESID vsesid, JET_VTID vtid,
    JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalNotifyRollback(JET_SESID vsesid, JET_VTID vtid,
    JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalNotifyUpdateUfn(JET_SESID vsesid, JET_VTID vtid)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalPrepareUpdate(JET_SESID vsesid, JET_VTID vtid,
    ULONG prep)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalRenameColumn(JET_SESID vsesid, JET_VTID vtid,
    const char  *szColumn, const char  *szColumnNew, const JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalRenameIndex(JET_SESID vsesid, JET_VTID vtid,
    const char  *szIndex, const char  *szIndexNew)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalRetrieveColumn(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_COLUMNID    columnid,
    VOID*           pvData,
    const ULONG     cbData,
    ULONG*          pcbActual,
    JET_GRBIT       grbit,
    JET_RETINFO*    pretinfo )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalRetrieveColumns(JET_SESID vsesid, JET_VTID vtid,
    JET_RETRIEVECOLUMN  *pretcols, ULONG cretcols )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalEnumerateColumns(
    JET_SESID               vsesid,
    JET_VTID                vtid,
    ULONG           cEnumColumnId,
    JET_ENUMCOLUMNID*       rgEnumColumnId,
    ULONG*          pcEnumColumn,
    JET_ENUMCOLUMN**        prgEnumColumn,
    JET_PFNREALLOC          pfnRealloc,
    void*                   pvReallocContext,
    ULONG           cbDataMost,
    JET_GRBIT               grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalGetRecordSize(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_RECSIZE3 *  precsize,
    const JET_GRBIT grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalRetrieveKey(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    VOID*           pvKey,
    const ULONG     cbMax,
    ULONG*          pcbActual,
    JET_GRBIT       grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalSeek(JET_SESID vsesid, JET_VTID vtid,
    JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalPrereadKeys(
    const JET_SESID             sesid,
    const JET_VTID              vtid,
    const void * const * const  rgpvKeys,
    const ULONG * const rgcbKeys,
    const LONG                  ckeys,
    LONG * const                pckeysPreread,
    const JET_GRBIT             grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalPrereadIndexRanges(
    const JET_SESID             sesid,
    const JET_VTID              vtid,
    const JET_INDEX_RANGE * const   rgIndexRanges,
    const ULONG         cIndexRanges,
    ULONG * const       pcRangesPreread,
    const JET_COLUMNID * const  rgcolumnidPreread,
    const ULONG         ccolumnidPreread,
    const JET_GRBIT             grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalSetColumn(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_COLUMNID    columnid,
    const VOID*     pvData,
    const ULONG     cbData,
    JET_GRBIT       grbit,
    JET_SETINFO*    psetinfo )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalSetColumns(JET_SESID vsesid, JET_VTID vtid,
    JET_SETCOLUMN   *psetcols, ULONG csetcols )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalSetIndexRange(JET_SESID vsesid, JET_VTID vtid,
    JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errIllegalOperation );
}


ERR VTAPI ErrIllegalUpdate(JET_SESID vsesid, JET_VTID vtid,
    void  *pvBookmark, ULONG cbBookmark,
    ULONG  *pcbActual, JET_GRBIT grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalRegisterCallback(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_CBTYP       cbtyp,
    JET_CALLBACK    pCallback,
    VOID *          pvContext,
    JET_HANDLE      *phCallbackId )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalUnregisterCallback(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_CBTYP       cbtyp,
    JET_HANDLE      hCallbackId )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalSetLS(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_LS          ls,
    JET_GRBIT       grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalGetLS(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_LS          *pls,
    JET_GRBIT       grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalIndexRecordCount(
    JET_SESID sesid, JET_VTID vtid, ULONG64 *pcrec,
    ULONG64 crecMax )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalRetrieveTaggedColumnList(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    ULONG               *pcentries,
    VOID                *pv,
    const ULONG         cbMax,
    const JET_COLUMNID  columnidStart,
    const JET_GRBIT     grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalSetSequential(
    const JET_SESID,
    const JET_VTID,
    const JET_GRBIT )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalResetSequential(
    const JET_SESID,
    const JET_VTID,
    const JET_GRBIT )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalSetCurrentIndex(
    JET_SESID           sesid,
    JET_VTID            tableid,
    const CHAR          *szIndexName,
    const JET_INDEXID   *pindexid,
    const JET_GRBIT     grbit,
    const ULONG         itagSequence )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalPrereadIndexRange(
    const JET_SESID                 sesid,
    const JET_VTID                  vtid,
    const JET_INDEX_RANGE * const   pIndexRange,
    const ULONG             cPageCacheMin,
    const ULONG             cPageCacheMax,
    const JET_GRBIT                 grbit,
    ULONG * const           pcPageCacheActual )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalRetrieveColumnByReference(
    _In_ const JET_SESID                                            sesid,
    _In_ const JET_TABLEID                                          tableid,
    _In_reads_bytes_( cbReference ) const void * const              pvReference,
    _In_ const ULONG                                        cbReference,
    _In_ const ULONG                                        ibData,
    _Out_writes_bytes_to_opt_( cbData, *pcbActual ) void * const    pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalPrereadColumnsByReference(
    _In_ const JET_SESID                                    sesid,
    _In_ const JET_TABLEID                                  tableid,
    _In_reads_( cReferences ) const void * const * const    rgpvReferences,
    _In_reads_( cReferences ) const ULONG * const   rgcbReferences,
    _In_ const ULONG                                cReferences,
    _In_ const ULONG                                cPageCacheMin,
    _In_ const ULONG                                cPageCacheMax,
    _Out_opt_ ULONG * const                         pcReferencesPreread,
    _In_ const JET_GRBIT                                    grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrIllegalStreamRecords(
    _In_ JET_SESID                                                  sesid,
    _In_ JET_TABLEID                                                tableid,
    _In_ const ULONG                                        ccolumnid,
    _In_reads_opt_( ccolumnid ) const JET_COLUMNID * const          rgcolumnid,
    _Out_writes_bytes_to_opt_( cbData, *pcbActual ) void * const    pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrInvalidAddColumn(JET_SESID sesid, JET_VTID vtid,
    const char  *szColumn, const JET_COLUMNDEF  *pcolumndef,
    const void  *pvDefault, ULONG cbDefault,
    JET_COLUMNID  *pcolumnid)
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidCloseTable(JET_SESID vsesid, JET_VTID vtid)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidComputeStats(JET_SESID vsesid, JET_VTID vtid)
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidCopyBookmarks(JET_SESID sesid, JET_VTID vtidSrc,
    JET_TABLEID tableidDest, JET_COLUMNID columnidDest, ULONG crecMax)
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidCreateIndex(JET_SESID vsesid, JET_VTID vtid, JET_INDEXCREATE3_A *, ULONG)
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidDelete(JET_SESID vsesid, JET_VTID vtid)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidDeleteColumn(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    const char      *szColumn,
    const JET_GRBIT grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidDeleteIndex(JET_SESID vsesid, JET_VTID vtid,
    const char  *szIndexName)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidDupCursor(JET_SESID vsesid, JET_VTID vtid,
    JET_TABLEID  *ptableid, JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidEscrowUpdate(
    JET_SESID vsesid,
    JET_VTID vtid,
    JET_COLUMNID columnid,
    void *pv,
    ULONG cbMax,
    void *pvOld,
    ULONG cbOldMax,
    ULONG *pcbOldActual,
    JET_GRBIT grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidGetBookmark(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    VOID * const    pvBookmark,
    const ULONG     cbMax,
    ULONG * const   pcbActual )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidGetIndexBookmark(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    VOID * const    pvSecondaryKey,
    const ULONG     cbSecondaryKeyMax,
    ULONG * const   pcbSecondaryKeyActual,
    VOID * const    pvPrimaryBookmark,
    const ULONG     cbPrimaryBookmarkMax,
    ULONG * const   pcbPrimaryBookmarkActual )
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidGetChecksum(JET_SESID vsesid, JET_VTID vtid,
    ULONG  *pChecksum)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidGetCurrentIndex(JET_SESID vsesid, JET_VTID vtid,
    __out_bcount(cchIndexName) PSTR szIndexName, ULONG cchIndexName)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidGetCursorInfo(JET_SESID vsesid, JET_VTID vtid,
    void  *pvResult, ULONG cbMax, ULONG InfoLevel)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidGetRecordPosition(JET_SESID vsesid, JET_VTID vtid,
    JET_RECPOS  *pkeypos, ULONG cbKeypos)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidGetTableColumnInfo(JET_SESID vsesid, JET_VTID vtid,
    const char  *szColumnName, const JET_COLUMNID *pcolid, void  *pvResult,
    ULONG cbMax, ULONG InfoLevel, const BOOL fUnicodeNames )
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidGetTableIndexInfo(JET_SESID vsesid, JET_VTID vtid,
    const char  *szIndexName, void  *pvResult,
    ULONG cbMax, ULONG InfoLevel, const BOOL fUnicodeNames )
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidGetTableInfo(JET_SESID vsesid, JET_VTID vtid,
    void  *pvResult, ULONG cbMax, ULONG InfoLevel)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidSetTableInfo(JET_SESID sesid, JET_VTID vtid,
    _In_reads_bytes_opt_(cbParam) const void  *pvParam, ULONG cbParam, ULONG InfoLevel)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidGotoBookmark(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    const VOID * const  pvBookmark,
    const ULONG         cbBookmark )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidGotoIndexBookmark(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    const VOID * const  pvSecondaryKey,
    const ULONG         cbSecondaryKey,
    const VOID * const  pvPrimaryBookmark,
    const ULONG         cbPrimaryBookmark,
    const JET_GRBIT     grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidGotoPosition(JET_SESID vsesid, JET_VTID vtid,
    JET_RECPOS *precpos)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidVtIdle(JET_SESID vsesid, JET_VTID vtid)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidMakeKey(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    const VOID*     pvData,
    const ULONG     cbData,
    JET_GRBIT       grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidMove(JET_SESID vsesid, JET_VTID vtid,
    LONG cRow, JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidSetCursorFilter(JET_SESID vsesid, JET_VTID vtid,
    JET_INDEX_COLUMN * rgFilters, DWORD cFilters, JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidNotifyBeginTrans(JET_SESID vsesid, JET_VTID vtid)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidNotifyCommitTrans(JET_SESID vsesid, JET_VTID vtid,
    JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidNotifyRollback(JET_SESID vsesid, JET_VTID vtid,
    JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidNotifyUpdateUfn(JET_SESID vsesid, JET_VTID vtid)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidPrepareUpdate(JET_SESID vsesid, JET_VTID vtid,
    ULONG prep)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidRenameColumn(JET_SESID vsesid, JET_VTID vtid,
    const char  *szColumn, const char  *szColumnNew, const JET_GRBIT grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidRenameIndex(JET_SESID vsesid, JET_VTID vtid,
    const char  *szIndex, const char  *szIndexNew)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidRetrieveColumn(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_COLUMNID    columnid,
    VOID*           pvData,
    const ULONG     cbData,
    ULONG*          pcbActual,
    JET_GRBIT       grbit,
    JET_RETINFO*    pretinfo )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidRetrieveColumns(JET_SESID vsesid, JET_VTID vtid,
    JET_RETRIEVECOLUMN  *pretcols, ULONG cretcols )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidEnumerateColumns(
    JET_SESID               vsesid,
    JET_VTID                vtid,
    ULONG           cEnumColumnId,
    JET_ENUMCOLUMNID*       rgEnumColumnId,
    ULONG*          pcEnumColumn,
    JET_ENUMCOLUMN**        prgEnumColumn,
    JET_PFNREALLOC          pfnRealloc,
    void*                   pvReallocContext,
    ULONG           cbDataMost,
    JET_GRBIT               grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidGetRecordSize(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_RECSIZE3 *  precsize,
    const JET_GRBIT grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidRetrieveKey(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    VOID*           pvKey,
    const ULONG     cbMax,
    ULONG*          pcbActual,
    JET_GRBIT       grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidSeek( JET_SESID vsesid, JET_VTID vtid, JET_GRBIT grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidPrereadKeys(
    const JET_SESID             sesid,
    const JET_VTID              vtid,
    const void * const * const  rgpvKeys,
    const ULONG * const rgcbKeys,
    const LONG                  ckeys,
    LONG * const                pckeysPreread,
    const JET_GRBIT             grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidPrereadIndexRanges(
    const JET_SESID             sesid,
    const JET_VTID              vtid,
    const JET_INDEX_RANGE * const   rgIndexRanges,
    const ULONG         cIndexRanges,
    ULONG * const       pcRangesPreread,
    const JET_COLUMNID * const  rgcolumnidPreread,
    const ULONG         ccolumnidPreread,
    const JET_GRBIT             grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidSetColumn(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_COLUMNID    columnid,
    const VOID*     pvData,
    const ULONG     cbData,
    JET_GRBIT       grbit,
    JET_SETINFO*    psetinfo )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidSetColumns(JET_SESID vsesid, JET_VTID vtid,
    JET_SETCOLUMN   *psetcols, ULONG csetcols )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidSetIndexRange(JET_SESID vsesid, JET_VTID vtid,
    JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidUpdate(JET_SESID vsesid, JET_VTID vtid,
    void  *pvBookmark, ULONG cbBookmark,
    ULONG  *pcbActual, JET_GRBIT grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidGetLock(JET_SESID vsesid, JET_VTID vtid,
    JET_GRBIT grbit)
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidRegisterCallback(JET_SESID  vsesid, JET_VTID vtid, JET_CBTYP cbtyp,
    JET_CALLBACK pCallback, VOID * pvContext, JET_HANDLE *phCallbackId )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidUnregisterCallback(JET_SESID vsesid, JET_VTID vtid, JET_CBTYP cbtyp,
    JET_HANDLE hCallbackId )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidSetLS(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_LS          ls,
    JET_GRBIT       grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidGetLS(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_LS          *pls,
    JET_GRBIT       grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidIndexRecordCount(
    JET_SESID sesid, JET_VTID vtid, ULONG64 *pcrec,
    ULONG64 crecMax )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidRetrieveTaggedColumnList(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    ULONG               *pcentries,
    VOID                *pv,
    const ULONG         cbMax,
    const JET_COLUMNID  columnidStart,
    const JET_GRBIT     grbit )
{
    return ErrERRCheck( JET_errInvalidTableId );
}


ERR VTAPI ErrInvalidSetSequential(
    const JET_SESID,
    const JET_VTID,
    const JET_GRBIT )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidResetSequential(
    const JET_SESID,
    const JET_VTID,
    const JET_GRBIT )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidSetCurrentIndex(
    JET_SESID           sesid,
    JET_VTID            tableid,
    const CHAR          *szIndexName,
    const JET_INDEXID   *pindexid,
    const JET_GRBIT     grbit,
    const ULONG         itagSequence )
{
    return ErrERRCheck( JET_errInvalidTableId );
}

ERR VTAPI ErrInvalidPrereadIndexRange(
    const JET_SESID                 sesid,
    const JET_VTID                  vtid,
    const JET_INDEX_RANGE * const   pIndexRange,
    const ULONG             cPageCacheMin,
    const ULONG             cPageCacheMax,
    const JET_GRBIT                 grbit,
    ULONG * const           pcPageCacheActual )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrInvalidRetrieveColumnByReference(
    _In_ const JET_SESID                                            sesid,
    _In_ const JET_TABLEID                                          tableid,
    _In_reads_bytes_( cbReference ) const void * const              pvReference,
    _In_ const ULONG                                        cbReference,
    _In_ const ULONG                                        ibData,
    _Out_writes_bytes_to_opt_( cbData, *pcbActual ) void * const    pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrInvalidPrereadColumnsByReference(
    _In_ const JET_SESID                                    sesid,
    _In_ const JET_TABLEID                                  tableid,
    _In_reads_( cReferences ) const void * const * const    rgpvReferences,
    _In_reads_( cReferences ) const ULONG * const   rgcbReferences,
    _In_ const ULONG                                cReferences,
    _In_ const ULONG                                cPageCacheMin,
    _In_ const ULONG                                cPageCacheMax,
    _Out_opt_ ULONG * const                         pcReferencesPreread,
    _In_ const JET_GRBIT                                    grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}

ERR VTAPI ErrInvalidStreamRecords(
    _In_ JET_SESID                                                  sesid,
    _In_ JET_TABLEID                                                tableid,
    _In_ const ULONG                                        ccolumnid,
    _In_reads_opt_( ccolumnid ) const JET_COLUMNID * const          rgcolumnid,
    _Out_writes_bytes_to_opt_( cbData, *pcbActual ) void * const    pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit )
{
    return ErrERRCheck( JET_errIllegalOperation );
}



#ifdef DEBUG
CODECONST(VTDBGDEF) vtdbgdefInvalidTableid =
{
    sizeof(VTFNDEF),
    0,
    "Invalid Tableid",
    0,
{
        0,
        0,
        0,
        0,
},
};
#endif  

extern const VTFNDEF vtfndefInvalidTableid =
{
    sizeof(VTFNDEF),
    0,
#ifndef DEBUG
    NULL,
#else   
    &vtdbgdefInvalidTableid,
#endif  
    ErrInvalidAddColumn,
    ErrInvalidCloseTable,
    ErrInvalidComputeStats,
    ErrInvalidCreateIndex,
    ErrInvalidDelete,
    ErrInvalidDeleteColumn,
    ErrInvalidDeleteIndex,
    ErrInvalidDupCursor,
    ErrInvalidEscrowUpdate,
    ErrInvalidGetBookmark,
    ErrInvalidGetIndexBookmark,
    ErrInvalidGetChecksum,
    ErrInvalidGetCurrentIndex,
    ErrInvalidGetCursorInfo,
    ErrInvalidGetRecordPosition,
    ErrInvalidGetTableColumnInfo,
    ErrInvalidGetTableIndexInfo,
    ErrInvalidGetTableInfo,
    ErrInvalidSetTableInfo,
    ErrInvalidGotoBookmark,
    ErrInvalidGotoIndexBookmark,
    ErrInvalidGotoPosition,
    ErrInvalidMakeKey,
    ErrInvalidMove,
    ErrInvalidSetCursorFilter,
    ErrInvalidPrepareUpdate,
    ErrInvalidRenameColumn,
    ErrInvalidRenameIndex,
    ErrInvalidRetrieveColumn,
    ErrInvalidRetrieveColumns,
    ErrInvalidRetrieveKey,
    ErrInvalidSeek,
    ErrInvalidPrereadKeys,
    ErrInvalidPrereadIndexRanges,
    ErrInvalidSetCurrentIndex,
    ErrInvalidSetColumn,
    ErrInvalidSetColumns,
    ErrInvalidSetIndexRange,
    ErrInvalidUpdate,
    ErrInvalidGetLock,
    ErrInvalidRegisterCallback,
    ErrInvalidUnregisterCallback,
    ErrInvalidSetLS,
    ErrInvalidGetLS,
    ErrInvalidIndexRecordCount,
    ErrInvalidRetrieveTaggedColumnList,
    ErrInvalidSetSequential,
    ErrInvalidResetSequential,
    ErrInvalidEnumerateColumns,
    ErrInvalidGetRecordSize,
    ErrInvalidPrereadIndexRange,
    ErrInvalidRetrieveColumnByReference,
    ErrInvalidPrereadColumnsByReference,
    ErrInvalidStreamRecords,
};

const VTFNDEF vtfndefIsamCallback =
{
    sizeof(VTFNDEF),
    0,
    NULL,
    ErrIllegalAddColumn,
    ErrIllegalCloseTable,
    ErrIllegalComputeStats,
    ErrIllegalCreateIndex,
    ErrIllegalDelete,
    ErrIllegalDeleteColumn,
    ErrIllegalDeleteIndex,
    ErrIsamDupCursor,
    ErrIllegalEscrowUpdate,
    ErrIsamGetBookmark,
    ErrIsamGetIndexBookmark,
    ErrIllegalGetChecksum,
    ErrIsamGetCurrentIndex,
    ErrIsamGetCursorInfo,
    ErrIsamGetRecordPosition,
    ErrIsamGetTableColumnInfo,
    ErrIsamGetTableIndexInfo,
    ErrIsamGetTableInfo,
    ErrIsamSetTableInfo,
    ErrIllegalGotoBookmark,
    ErrIllegalGotoIndexBookmark,
    ErrIllegalGotoPosition,
    ErrIllegalMakeKey,
    ErrIllegalMove,
    ErrIllegalSetCursorFilter,
    ErrIllegalPrepareUpdate,
    ErrIllegalRenameColumn,
    ErrIllegalRenameIndex,
    ErrIsamRetrieveColumn,
    ErrIsamRetrieveColumns,
    ErrIsamRetrieveKey,
    ErrIllegalSeek,
    ErrIllegalPrereadKeys,
    ErrIllegalPrereadIndexRanges,
    ErrIllegalSetCurrentIndex,
    ErrIsamSetColumn,
    ErrIsamSetColumns,
    ErrIllegalSetIndexRange,
    ErrIllegalUpdate,
    ErrIllegalGetLock,
    ErrIsamRegisterCallback,
    ErrIsamUnregisterCallback,
    ErrIsamSetLS,
    ErrIsamGetLS,
    ErrIllegalIndexRecordCount,
    ErrIllegalRetrieveTaggedColumnList,
    ErrIllegalSetSequential,
    ErrIllegalResetSequential,
    ErrIsamEnumerateColumns,
    ErrIsamGetRecordSize,
    ErrIllegalPrereadIndexRange,
    ErrIllegalRetrieveColumnByReference,
    ErrIllegalPrereadColumnsByReference,
    ErrIllegalStreamRecords,
};

extern const ULONG  cbIDXLISTNewMembersSinceOriginalFormat;

#ifdef PERFMON_SUPPORT

PERFInstanceDelayedTotal<LONG, INST, fFalse, fFalse> cInstanceStatus;

LONG LInstanceStatusCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cInstanceStatus.PassTo( iInstance, pvBuf );
    return 0;
}
#endif

VOID INST::SetInstanceStatusReporting( const INT iInstance, const INT perfStatus )
{
    Assert( 0 != perfStatus );
    if ( perfStatus != perfStatusError )
    {
        m_perfstatusEvent = perfStatus;
    }
    PERFOpt( cInstanceStatus.Set( iInstance, perfStatus ) );
}

VOID INST::SetStatusRedo()
{
    SetInstanceStatusReporting( m_iInstance, perfStatusRecoveryRedo );
    TraceStationId( tsidrEngineInstBeginRedo );
}

VOID INST::SetStatusUndo()
{
    SetInstanceStatusReporting( m_iInstance, perfStatusRecoveryUndo );
    TraceStationId( tsidrEngineInstBeginUndo );
}

VOID INST::SetStatusRuntime()
{
    SetInstanceStatusReporting( m_iInstance, perfStatusRuntime );
    TraceStationId( tsidrEngineInstBeginDo );
}

VOID INST::SetStatusTerm()
{
    SetInstanceStatusReporting( m_iInstance, perfStatusTerm );
    TraceStationId( tsidrCloseTerm );
}

VOID INST::SetStatusError()
{
    SetInstanceStatusReporting( m_iInstance, perfStatusError );
}

VOID INST::TraceStationId( const TraceStationIdentificationReason tsidr )
{

    if ( tsidr == tsidrPulseInfo )
    {

        OSSysTraceStationId( tsidr );
    }

    if ( !m_traceidcheckInst.FAnnounceTime< _etguidInstStationId >( tsidr ) )
    {
        return;
    }

    ETInstStationId( tsidr, m_iInstance, (BYTE)m_perfstatusEvent, m_wszInstanceName, m_wszDisplayName );
}


LONG INST::iInstanceActiveMin = lMax;
LONG INST::iInstanceActiveMac = 1;
LONG INST::cInstancesCounter = 0;

LOCAL VOID InitPERFCounters( INT iInstance )
{
}

LONG g_cbPlsMemRequiredPerPerfInstance = 0;

LOCAL LONG g_lRefreshPerfInstanceList = 0;

WCHAR*      g_wszInstanceNames              = NULL;
WCHAR*      g_wszInstanceNamesOut           = NULL;
__range( 0, ( g_cpinstMax + 1 ) * ( cchPerfmonInstanceNameMax + 1 ) + 1 ) ULONG g_cchInstanceNames = 0;
BYTE*       g_rgbInstanceAggregationIDs     = NULL;
LOCAL const BYTE bInstanceNameAggregationID = 1;

WCHAR*      g_wszDatabaseNames              = NULL;
WCHAR*      g_wszDatabaseNamesOut           = NULL;
__range( 0, g_ifmpMax * ( cchPerfmonInstanceNameMax + 1 ) + 1 ) ULONG g_cchDatabaseNames = 0;
BYTE*       g_rgbDatabaseAggregationIDs     = NULL;


INT g_cInstances = 0;
INT g_cDatabases = 0;

static CCriticalSection g_critInstanceNames( CLockBasicInfo( CSyncBasicInfo( "g_critInstanceNames" ), 0, 0 ) );
static CCriticalSection g_critDatabaseNames( CLockBasicInfo( CSyncBasicInfo( "g_critDatabaseNames" ), 0, 0 ) );

INT CPERFESEInstanceObjects( const INT cUsedInstances )
{
    return cUsedInstances + 1;
}

ULONG CPERFDatabaseObjects( const ULONG cIfmpsInUse )
{
    return max( 1, cIfmpsInUse );
}

ULONG CPERFObjectsMinReq()
{

    const ULONG cperfinstESEInstanceMinReq = (ULONG)CPERFESEInstanceObjects( 0 );


    const ULONG cperfinstIFMPMinReq = CPERFDatabaseObjects( 0 );


    const ULONG cperfinstTCEMinReq = (ULONG)CPERFTCEObjects();


    return max( max( cperfinstESEInstanceMinReq, cperfinstIFMPMinReq ), cperfinstTCEMinReq );
}


VOID PERFSetInstanceNames()
{
    if ( g_fDisablePerfmon )
    {
        Assert( NULL == g_rgbInstanceAggregationIDs );
        Assert( 0 == g_cchInstanceNames );
        return;
    }

    g_critInstanceNames.Enter();
    Assert( INST::FOwnerCritInst() );
    WCHAR * szT = g_wszInstanceNames;
    ULONG   cbT = g_cchInstanceNames * sizeof( WCHAR );
    OSStrCbCopyW( szT, cbT, L"_Total" );
    g_rgbInstanceAggregationIDs[ 0 ] = bInstanceNameAggregationID;

    if ( sizeof( WCHAR ) * ( wcslen( szT ) + 1 ) > g_cchInstanceNames * sizeof( WCHAR ) )
    {
        Assert( fFalse );
        g_critInstanceNames.Leave();
        return;
    }
    cbT -= sizeof( WCHAR ) * ( wcslen( szT ) + 1 );
    szT += wcslen( szT ) + 1;

    INT ipinstLastUsed = 0;
    for ( ipinstLastUsed = g_cpinstMax - 1; ipinstLastUsed > 0; ipinstLastUsed-- )
    {
        if( g_rgpinst[ipinstLastUsed] )
        {
            break;
        }
    }

    for ( INT ipinst = 0; ipinst <= ipinstLastUsed; ++ipinst )
    {
        if( g_rgpinst[ipinst] )
        {
            const WCHAR * const wszInstanceName = g_rgpinst[ipinst]->m_wszDisplayName ?
                    g_rgpinst[ipinst]->m_wszDisplayName : g_rgpinst[ipinst]->m_wszInstanceName;

            if ( wszInstanceName )
            {
                OSStrCbFormatW( szT, cbT, L"%.*ws", (ULONG)cchPerfmonInstanceNameMax, wszInstanceName );
            }
            else
            {
                OSStrCbFormatW( szT, cbT, L"%d", ipinst );
            }
        }
        else
        {
            OSStrCbFormatW( szT, cbT, L"_Unused%d", ipinst );
        }
        cbT -= sizeof( WCHAR ) * ( wcslen( szT ) + 1 );
        szT += wcslen( szT ) + 1;
    }

    g_cInstances = CPERFESEInstanceObjects( ipinstLastUsed + 1 );

    AtomicExchange( &g_lRefreshPerfInstanceList, 1 );

    g_critInstanceNames.Leave();
}

VOID PERFSetDatabaseNames( IFileSystemAPI* const pfsapi )
{
    if ( g_fDisablePerfmon )
    {
        Assert( NULL == g_rgbDatabaseAggregationIDs );
        Assert( 0 == g_cchDatabaseNames );
        return;
    }

    g_critDatabaseNames.Enter();
    Assert( FMP::FWriterFMPPool() );
    WCHAR * szT = g_wszDatabaseNames;
    ULONG   cbT = g_cchDatabaseNames * sizeof( WCHAR );

    for ( IFMP ifmp = 0; ifmp <= FMP::IfmpMacInUse(); ++ifmp )
    {
        const FMP* pfmp = NULL;

        if ( ( ifmp >= FMP::IfmpMinInUse() ) && ( ( pfmp = &g_rgfmp[ ifmp ] ) != NULL ) && ( pfmp->FInUse() ) )
        {
            const WCHAR * const wszDatabasePath = pfmp->WszDatabaseName();

            if ( wszDatabasePath )
            {
                OSStrCbFormatW( szT, cbT, L"%.*ws", (ULONG)cchPerfmonInstanceNameMax, pfsapi->WszPathFileName( wszDatabasePath ) );
            }
            else
            {
                FireWall( "IfmpInUseNullPath" );
                OSStrCbFormatW( szT, cbT, L"%u", ifmp );
            }
        }
        else
        {
            OSStrCbFormatW( szT, cbT, L"_Unused%u", ifmp );
        }

        cbT -= sizeof( WCHAR ) * ( wcslen( szT ) + 1 );
        szT += wcslen( szT ) + 1;
    }

    g_cDatabases = FMP::IfmpMacInUse() + 1;

    AtomicExchange( &g_lRefreshPerfInstanceList, 1 );

    g_critDatabaseNames.Leave();
}



INT CIsamSequenceDiagLog::s_iDumpVersion = 2;

CIsamSequenceDiagLog::SEQFIXEDDATA CIsamSequenceDiagLog::s_uDummyFixedData = { 0 };

void CIsamSequenceDiagLog::InitSequence( _In_ const IsamSequenceDiagLogType isdltype, _In_range_( 3, 128 ) const BYTE cseqMax )
{
    ERR err = JET_errSuccess;

    Expected( cseqMax < 128 );


    Assert( m_isdltype == isdltype );
    m_cseqMax = cseqMax;

    OnDebug( m_tidOwner = DwUtilThreadId() );

    m_fInitialized = fTrue;


    Assert( !FAllocatedFixedData_() );
    if ( !m_fPreAllocatedSeqDiagInfo )
    {
        Assert( m_rgDiagInfo == NULL );
        if( m_rgDiagInfo )
        {
            TermSequence();
        }
        Alloc( m_rgDiagInfo = PvFaultInj( 58220, new SEQDIAGINFO[m_cseqMax] ) );
    }
    else
    {
        Assert( m_rgDiagInfo );
    }
    memset( m_rgDiagInfo, 0, sizeof(SEQDIAGINFO) * m_cseqMax );
    Assert( m_rgDiagInfo[1].secThrottled == 0.0 );

    SEQFIXEDDATA * pT = PvFaultInj( 40922, new SEQFIXEDDATA );
    if ( pT )
    {
        m_pFixedData = pT;
        Assert( FAllocatedFixedData_() );
        memset( m_pFixedData, 0, sizeof(SEQFIXEDDATA) );
    }
    Assert( m_pFixedData );


    Trigger( eSequenceStart );

    return;

HandleError:

    Expected( m_rgDiagInfo == NULL );
    Expected( !FAllocatedFixedData_() );

    TermSequence();

    OnDebug( m_tidOwner = DwUtilThreadId() );
    m_fAllocFailure = fTrue;
    m_fInitialized = fTrue;

    m_cseqMax = cseqMax;
    Assert( m_cseqMac == 0 );
}


void CIsamSequenceDiagLog::InitConsumeSequence( _In_ const IsamSequenceDiagLogType isdltype, _In_ CIsamSequenceDiagLog * const pisdlSeed, _In_range_( 3, 128 ) const BYTE cseqMax )
{
    Assert( pisdlSeed->m_cseqMax > 1 || pisdlSeed->m_rgDiagInfo == NULL );
    Expected( pisdlSeed->m_cseqMac < 7 );
    Assert( pisdlSeed->m_cseqMac < ( cseqMax - 1 ) );


    InitSequence( isdltype, cseqMax );


    if( m_rgDiagInfo && pisdlSeed->m_rgDiagInfo )
    {
        memcpy( m_rgDiagInfo, pisdlSeed->m_rgDiagInfo, pisdlSeed->m_cseqMac * sizeof(m_rgDiagInfo[0]) );
    }
    else
    {
        Trigger( pisdlSeed->m_cseqMax - 1 );
    }

    pisdlSeed->TermSequence();
}

void CIsamSequenceDiagLog::Trigger( _In_ const BYTE seqTrigger )
{
    Assert( m_tidOwner == DwUtilThreadId() );
    if( m_rgDiagInfo == NULL )
    {
        Assert( m_fAllocFailure );
        return;
    }

    if ( m_cseqMac > 0 && m_cseqMac - 1 != seqTrigger )
    {
        Assert( FTriggeredStep( m_cseqMac - 1 ) || m_rgDiagInfo[ m_cseqMac - 1 ].cCallbacks == 0 );
        Assert( FTriggeredStep( m_cseqMac - 1 ) || m_rgDiagInfo[ m_cseqMac - 1 ].cThrottled == 0 );
    }

    Assert( FValidSequence_( seqTrigger ) );
    if ( FValidSequence_( seqTrigger ) )
    {
        m_rgDiagInfo[seqTrigger].hrtEnd = HrtHRTCount();
        UtilGetCurrentDateTime( &(m_rgDiagInfo[seqTrigger].dtm) );
        m_rgDiagInfo[seqTrigger].seq = seqTrigger;
        C_ASSERT( sizeof(m_rgDiagInfo[seqTrigger].thstat) == sizeof(Ptls()->threadstats) );
        memcpy( &(m_rgDiagInfo[seqTrigger].thstat), &(Ptls()->threadstats), sizeof(m_rgDiagInfo[seqTrigger].thstat) );
        OSMemoryGetProcessMemStats( &(m_rgDiagInfo[seqTrigger].memstat) );
        extern ULONG_PTR        g_cbCacheCommittedSize;
        m_rgDiagInfo[seqTrigger].cbCacheMem = g_cbCacheCommittedSize;

        AssertSz( (INT)m_cseqMac <= seqTrigger + 1, "Timing sequence seems to be jumping back in time!?!?" );
        m_cseqMac = max( m_cseqMac, seqTrigger + 1 );

        Assert( FTriggeredSequence_( seqTrigger ) );
    }
}

void CIsamSequenceDiagLog::AddCallbackTime( const double secsCallback, const __int64 cCallbacks )
{
    Assert( m_tidOwner == DwUtilThreadId() );
    Assert( FValidSequence_( m_cseqMac ) );
    if( m_rgDiagInfo == NULL || m_cseqMac <= 0 )
    {
        Assert( m_fAllocFailure );
        return;
    }

    m_rgDiagInfo[m_cseqMac].cCallbacks += cCallbacks;
    m_rgDiagInfo[m_cseqMac].secInCallback += secsCallback;
}

double CIsamSequenceDiagLog::GetCallbackTime( __int64 *pcCallbacks )
{
    Assert( m_tidOwner == DwUtilThreadId() );
    Assert( FValidSequence_( m_cseqMac ) );
    if( m_rgDiagInfo == NULL || m_cseqMac <= 0 )
    {
        Assert( m_fAllocFailure );
        *pcCallbacks = 0;
        return 0;
    }

    *pcCallbacks = m_rgDiagInfo[m_cseqMac].cCallbacks;
    return m_rgDiagInfo[m_cseqMac].secInCallback;
}

void CIsamSequenceDiagLog::AddThrottleTime( const double secsThrottle, const __int64 cThrottled )
{
    Assert( m_tidOwner == DwUtilThreadId() );
    Assert( FValidSequence_( m_cseqMac ) );
    if( m_rgDiagInfo == NULL || m_cseqMac <= 0 )
    {
        Assert( m_fAllocFailure );
        return;
    }

    m_rgDiagInfo[m_cseqMac].cThrottled += cThrottled;
    m_rgDiagInfo[m_cseqMac].secThrottled += secsThrottle;
}

double CIsamSequenceDiagLog::GetThrottleTime( __int64 *pcThrottled )
{
    Assert( m_tidOwner == DwUtilThreadId() );
    Assert( FValidSequence_( m_cseqMac ) );
    if( m_rgDiagInfo == NULL || m_cseqMac <= 0 )
    {
        Assert( m_fAllocFailure );
        *pcThrottled = 0;
        return 0.0;
    }

    *pcThrottled = m_rgDiagInfo[m_cseqMac].cThrottled;
    return m_rgDiagInfo[m_cseqMac].secThrottled;
}

__int64 CIsamSequenceDiagLog::UsecTimer( _In_ INT seqBegin, _In_ const INT seqEnd ) const
{
    Assert( m_tidOwner == DwUtilThreadId() );
    Expected( ( ( seqBegin + 1 ) == seqEnd ) ||
              ( ( seqBegin == eSequenceStart ) && ( seqEnd == ( m_cseqMax - 1 ) ) ) );

    if( m_rgDiagInfo == NULL )
    {
        Assert( m_fAllocFailure );
        return 0;
    }

    if ( !FValidSequence_( seqBegin ) ||
        !FValidSequence_( seqEnd ) ||
        seqBegin >= seqEnd ||
        seqBegin < 0 )
    {
        AssertSz( fFalse, "Passed an invalid sequence number for this timer sequence" );
        return 0;
    }

    while ( !FTriggeredSequence_( seqBegin ) && seqBegin > 0 )
    {
        seqBegin--;
    }

    if ( !FTriggeredSequence_( seqBegin ) ||
        !FTriggeredSequence_( seqEnd ) )
    {
        return 0;
    }

    const __int64 retT = __int64( DblHRTDelta( m_rgDiagInfo[seqBegin].hrtEnd, m_rgDiagInfo[seqEnd].hrtEnd ) * 1000000.0 );
    Assert( retT >= 0 );
    return retT;
}

void CIsamSequenceDiagLog::SprintFixedData( _Out_writes_bytes_(cbFixedData) WCHAR * wszFixedData, _In_range_( 4, 4000 ) ULONG cbFixedData ) const
{
    Assert( cbFixedData > 2 );
    wszFixedData[0] = L'\0';

    WCHAR * pwszCurr = wszFixedData;
    SIZE_T cbCurrLeft = cbFixedData;
    SIZE_T cchUsed = 0;
    if ( FAllocatedFixedData_() )
    {
        switch ( m_isdltype )
        {
        case isdltypeInit:
            if ( 0 != CmpLgpos( &FixedData().sInitData.lgposRecoveryStartMin, &lgposMin ) ||
                    0 != CmpLgpos( &FixedData().sInitData.lgposRecoveryUndoEnd, &lgposMin ) )
            {
                OSStrCbFormatW( pwszCurr, cbCurrLeft,
                    L"lgposV%d[] = %08X:%04X:%04X - %08X:%04X:%04X - %08X:%04X:%04X - %08X:%04X:%04X (%08X:%04X:%04X)\n",
                    s_iDumpVersion,
                    FixedData().sInitData.lgposRecoveryStartMin.lGeneration, FixedData().sInitData.lgposRecoveryStartMin.isec, FixedData().sInitData.lgposRecoveryStartMin.ib,
                    FixedData().sInitData.lgposRecoverySilentRedoEnd.lGeneration, FixedData().sInitData.lgposRecoverySilentRedoEnd.isec, FixedData().sInitData.lgposRecoverySilentRedoEnd.ib,
                    FixedData().sInitData.lgposRecoveryVerboseRedoEnd.lGeneration, FixedData().sInitData.lgposRecoveryVerboseRedoEnd.isec, FixedData().sInitData.lgposRecoveryVerboseRedoEnd.ib,
                    FixedData().sInitData.lgposRecoveryUndoEnd.lGeneration, FixedData().sInitData.lgposRecoveryUndoEnd.isec, FixedData().sInitData.lgposRecoveryUndoEnd.ib,
                    FixedData().sInitData.lgposRecoveryForwardLogs.lGeneration, FixedData().sInitData.lgposRecoveryForwardLogs.isec, FixedData().sInitData.lgposRecoveryForwardLogs.ib
                    );
            }
            cchUsed = wcslen( pwszCurr );
            pwszCurr += cchUsed;
            cbCurrLeft -= ( cchUsed * 2 );

            if ( FixedData().sInitData.hrtRecoveryForwardLogs )
            {
                Assert( FTriggeredStep( eForwardLogBaselineStep ) );

                ULONG clogs = 0;
                if ( FixedData().sInitData.lgposRecoveryStartMin.lGeneration &&
                        ( FixedData().sInitData.lgposRecoveryForwardLogs.lGeneration - FixedData().sInitData.lgposRecoveryStartMin.lGeneration ) )
                {
                    clogs = ( FixedData().sInitData.lgposRecoveryForwardLogs.lGeneration - FixedData().sInitData.lgposRecoveryStartMin.lGeneration );
                }

                OSStrCbFormatW( pwszCurr, cbCurrLeft, L"ForwardLogsV%d = %0.6f s - %d lgens\n",
                    s_iDumpVersion,
                    DblHRTDelta( m_rgDiagInfo[eForwardLogBaselineStep].hrtEnd, FixedData().sInitData.hrtRecoveryForwardLogs ),
                    clogs );
                cchUsed = wcslen( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );

            }

            if ( FixedData().sInitData.cReInits )
            {
                OSStrCbFormatW( pwszCurr, cbCurrLeft, L"cReInits = %d\n", FixedData().sInitData.cReInits );
            }
            cchUsed = wcslen( pwszCurr );
            pwszCurr += cchUsed;
            cbCurrLeft -= ( cchUsed * 2 );

            break;


        #define ArgSplitLgposToUl( lgpos )      (ULONG)((lgpos).lGeneration), (ULONG)((lgpos).isec), (ULONG)((lgpos).ib)

        case isdltypeTerm:
            if ( 0 != CmpLgpos( &FixedData().sTermData.lgposTerm, &lgposMin ) )
            {
                OSStrCbFormatW( pwszCurr, cbCurrLeft, L"lgposTerm = %08X:%04X:%04X", ArgSplitLgposToUl( FixedData().sTermData.lgposTerm ) );
            }
            break;

        case isdltypeCreate:
            if ( 0 != CmpLgpos( &FixedData().sCreateData.lgposCreate, &lgposMin ) )
            {
                OSStrCbFormatW( pwszCurr, cbCurrLeft, L"lgposCreate = %08X:%04X:%04X", ArgSplitLgposToUl( FixedData().sCreateData.lgposCreate ) );
            }
            break;

        case isdltypeAttach:
            if ( 0 != CmpLgpos( &FixedData().sAttachData.lgposAttach, &lgposMin ) )
            {
                OSStrCbFormatW( pwszCurr, cbCurrLeft, L"lgposAttach = %08X:%04X:%04X", ArgSplitLgposToUl( FixedData().sAttachData.lgposAttach ) );
            }
            break;

        case isdltypeDetach:
            if ( 0 != CmpLgpos( &FixedData().sDetachData.lgposDetach, &lgposMin ) )
            {
                OSStrCbFormatW( pwszCurr, cbCurrLeft, L"lgposDetach = %08X:%04X:%04X", ArgSplitLgposToUl( FixedData().sDetachData.lgposDetach ) );
            }
            break;


        default:
            AssertSz( fFalse, "Unknown additional FixedData operation for %hc, or calling on non-active sequence.", m_isdltype );
            break;
        }
    }
}

void CIsamSequenceDiagLog::SprintTimings( _Out_writes_bytes_(cbTimeSeq) WCHAR * wszTimeSeq, _In_range_( 4, 4000 ) ULONG cbTimeSeq ) const
{
    Assert( m_tidOwner == DwUtilThreadId() );
    const BOOL fWithJetstat = fTrue;
    const BOOL fWithMemstat = fTrue;

    const BOOL fWithLineReturns = fTrue;
    if( m_rgDiagInfo == NULL )
    {
        OSStrCbCopyW( wszTimeSeq, cbTimeSeq, L"[*] = *." );
        return;
    }

    WCHAR * pwszCurr = wszTimeSeq;
    SIZE_T cbCurrLeft = cbTimeSeq;
    SIZE_T cchUsed;

    Assert( pwszCurr );
    Assert( cbTimeSeq > 2 );
    wszTimeSeq[0] = L'\0';

#ifdef DEBUG
    OSStrCbFormatW( pwszCurr, cbCurrLeft, L"%dV%d", m_isdltype, s_iDumpVersion );
    cchUsed = wcslen( pwszCurr );
    pwszCurr += cchUsed;
    cbCurrLeft -= ( cchUsed * 2 );
#endif

    Assert( FTriggeredSequence_( m_cseqMax - 1 ) );

    for( ULONG seq = 1; seq < m_cseqMax; seq++ )
    {
        if ( seq > 1 || fWithLineReturns  )
        {
            const INT errT = fWithLineReturns ?
                                ErrOSStrCbAppendW( pwszCurr, cbCurrLeft, L"\n" ) :
                                ErrOSStrCbAppendW( pwszCurr, cbCurrLeft, L", " );
            Assert( 0 == errT );
            cchUsed = wcslen( pwszCurr );
            pwszCurr += cchUsed;
            cbCurrLeft -= ( cchUsed * 2 );
        }

        const __int64 usecStepTime = UsecTimer( seq - 1, seq );
        ULONG secs = ULONG( usecStepTime / 1000000 );
        ULONG usecs = ULONG( usecStepTime % 1000000 );
        if ( !FTriggeredSequence_( seq ) )
        {
            OSStrCbFormatW( pwszCurr, cbCurrLeft, L"[%d] -", seq );
        }
        else if ( usecStepTime == 0 )
        {
            OSStrCbFormatW( pwszCurr, cbCurrLeft, L"[%d] 0.0", seq );
        }
        else
        {
            Assert( FTriggeredSequence_( seq ) );
            ULONG seqBefore = seq - 1;
            while ( !FTriggeredSequence_( seqBefore ) && seqBefore > 0 )
            {
                seqBefore--;
            }

            OSStrCbFormatW( pwszCurr, cbCurrLeft, L"[%d] %d.%06d", seq, secs, usecs );

            if ( m_rgDiagInfo[seq].thstat.cusecPageCacheMiss - m_rgDiagInfo[seqBefore].thstat.cusecPageCacheMiss )
            {
                cchUsed = wcslen( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );

                OSStrCbFormatW( pwszCurr, cbCurrLeft, L" -%0.06I64f (%d) CM",
                        double( m_rgDiagInfo[seq].thstat.cusecPageCacheMiss - m_rgDiagInfo[seqBefore].thstat.cusecPageCacheMiss ) / 1000000.0,
                        m_rgDiagInfo[seq].thstat.cPageCacheMiss - m_rgDiagInfo[seqBefore].thstat.cPageCacheMiss );
            }
            if ( m_rgDiagInfo[seq].secInCallback != 0.0 )
            {
                cchUsed = wcslen( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );

                OSStrCbFormatW( pwszCurr, cbCurrLeft, L" -%0.06f (%I64d) CB", m_rgDiagInfo[seq].secInCallback, m_rgDiagInfo[seq].cCallbacks );
            }
            if ( m_rgDiagInfo[seq].secThrottled != 0.0 )
            {
                cchUsed = wcslen( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );

                OSStrCbFormatW( pwszCurr, cbCurrLeft, L" -%0.06f (%I64d) TT", m_rgDiagInfo[seq].secThrottled, m_rgDiagInfo[seq].cThrottled );
            }
            if ( m_rgDiagInfo[seq].thstat.cusecWait - m_rgDiagInfo[seqBefore].thstat.cusecWait )
            {
                cchUsed = wcslen( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );

                OSStrCbFormatW( pwszCurr, cbCurrLeft, L" -%0.06I64f (%d) WT",
                        double( m_rgDiagInfo[seq].thstat.cusecWait - m_rgDiagInfo[seqBefore].thstat.cusecWait ) / 1000000.0,
                        m_rgDiagInfo[seq].thstat.cWait - m_rgDiagInfo[seqBefore].thstat.cWait );
            }
        }
        cchUsed = wcslen( pwszCurr );
        pwszCurr += cchUsed;
        cbCurrLeft -= ( cchUsed * 2 );

        if ( FTriggeredSequence_( seq ) )
        {
            ULONG seqBefore = seq - 1;
            while ( !FTriggeredSequence_( seqBefore ) && seqBefore > 0 )
            {
                seqBefore--;
            }


            if ( fWithJetstat &&
                    ( m_rgDiagInfo[seq].thstat.cPageCacheMiss - m_rgDiagInfo[seqBefore].thstat.cPageCacheMiss ||
                        m_rgDiagInfo[seq].thstat.cPagePreread - m_rgDiagInfo[seqBefore].thstat.cPagePreread ||
                        m_rgDiagInfo[seq].thstat.cPageRead - m_rgDiagInfo[seqBefore].thstat.cPageRead ||
                        m_rgDiagInfo[seq].thstat.cPageDirtied - m_rgDiagInfo[seqBefore].thstat.cPageDirtied ||
                        m_rgDiagInfo[seq].thstat.cPageRedirtied - m_rgDiagInfo[seqBefore].thstat.cPageRedirtied ||
                        m_rgDiagInfo[seq].thstat.cPageReferenced - m_rgDiagInfo[seqBefore].thstat.cPageReferenced ||
                        m_rgDiagInfo[seq].thstat.cbLogRecord - m_rgDiagInfo[seqBefore].thstat.cbLogRecord ||
                        m_rgDiagInfo[seq].thstat.cLogRecord - m_rgDiagInfo[seqBefore].thstat.cLogRecord )
                    )
            {
                OSStrCbFormatW( pwszCurr, cbCurrLeft, L" +J(CM:%d, PgRf:%d, Rd:%d/%d, Dy:%d/%d, Lg:%d/%d)",
                                    m_rgDiagInfo[seq].thstat.cPageCacheMiss - m_rgDiagInfo[seqBefore].thstat.cPageCacheMiss,
                                    m_rgDiagInfo[seq].thstat.cPageReferenced - m_rgDiagInfo[seqBefore].thstat.cPageReferenced,
                                    m_rgDiagInfo[seq].thstat.cPagePreread - m_rgDiagInfo[seqBefore].thstat.cPagePreread,
                                    m_rgDiagInfo[seq].thstat.cPageRead - m_rgDiagInfo[seqBefore].thstat.cPageRead,
                                    m_rgDiagInfo[seq].thstat.cPageDirtied - m_rgDiagInfo[seqBefore].thstat.cPageDirtied,
                                    m_rgDiagInfo[seq].thstat.cPageRedirtied - m_rgDiagInfo[seqBefore].thstat.cPageRedirtied,
                                    m_rgDiagInfo[seq].thstat.cbLogRecord - m_rgDiagInfo[seqBefore].thstat.cbLogRecord,
                                    m_rgDiagInfo[seq].thstat.cLogRecord - m_rgDiagInfo[seqBefore].thstat.cLogRecord
                                    );
                cchUsed = wcslen( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );
            }
            else
            {
                OSStrCbFormatW( pwszCurr, cbCurrLeft, L" +J(0)" );
                cchUsed = wcslen( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );
            }

            #define DckbMemDelta( cbBefore, cbAfter )       ( __int64( cbBefore <= cbAfter ) ?                          \
                                                                __int64( ( cbAfter - cbBefore ) / 1024 ) :              \
                                                                __int64( __int64( cbBefore - cbAfter ) / 1024LL * -1 ) )

            const __int64 dckbCacheMem = DckbMemDelta( m_rgDiagInfo[seqBefore].cbCacheMem, m_rgDiagInfo[seq].cbCacheMem );
            const size_t dckbWorkingSetSize = DckbMemDelta( m_rgDiagInfo[seqBefore].memstat.cbWorkingSetSize, m_rgDiagInfo[seq].memstat.cbWorkingSetSize );
            const size_t dckbWorkingSetSizePeak = DckbMemDelta( m_rgDiagInfo[seqBefore].memstat.cbPeakWorkingSetSize, m_rgDiagInfo[seq].memstat.cbPeakWorkingSetSize );
            const size_t dckbPagefileUsage = DckbMemDelta( m_rgDiagInfo[seqBefore].memstat.cbPagefileUsage, m_rgDiagInfo[seq].memstat.cbPagefileUsage );
            const size_t dckbPagefileUsagePeak = DckbMemDelta( m_rgDiagInfo[seqBefore].memstat.cbPeakPagefileUsage, m_rgDiagInfo[seq].memstat.cbPeakPagefileUsage );
            const __int64 dckbPrivateUsage = DckbMemDelta( m_rgDiagInfo[seqBefore].memstat.cbPrivateUsage, m_rgDiagInfo[seq].memstat.cbPrivateUsage );

            if ( fWithMemstat &&
                    ( dckbCacheMem ||
                        m_rgDiagInfo[seq].memstat.cPageFaultCount - m_rgDiagInfo[seqBefore].memstat.cPageFaultCount ||
                        dckbWorkingSetSize ||
                        dckbWorkingSetSizePeak ||
                        dckbPagefileUsage ||
                        dckbPagefileUsagePeak ||
                        dckbPrivateUsage ) )
            {
                OSStrCbFormatW( pwszCurr, cbCurrLeft, L" +M(C:%I64dK, Fs:%d, WS:%IdK # %IdK, PF:%IdK # %IdK, P:%IdK)",
                                    dckbCacheMem,
                                    m_rgDiagInfo[seq].memstat.cPageFaultCount - m_rgDiagInfo[seqBefore].memstat.cPageFaultCount,
                                    dckbWorkingSetSize,
                                    dckbWorkingSetSizePeak,
                                    dckbPagefileUsage,
                                    dckbPagefileUsagePeak,
                                    dckbPrivateUsage );
                cchUsed = wcslen( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );
            }

            if ( m_isdltype == isdltypeInit )
            {
                ULONG clogs = 0;
                if ( seq == eInitLogRecoverySilentRedoDone &&
                        FixedData().sInitData.lgposRecoverySilentRedoEnd.lGeneration  &&
                        FixedData().sInitData.lgposRecoveryStartMin.lGeneration &&
                        ( FixedData().sInitData.lgposRecoverySilentRedoEnd.lGeneration - FixedData().sInitData.lgposRecoveryStartMin.lGeneration ) )
                {
                    clogs = ( FixedData().sInitData.lgposRecoverySilentRedoEnd.lGeneration - FixedData().sInitData.lgposRecoveryStartMin.lGeneration );
                }
                if ( seq == eInitLogRecoveryVerboseRedoDone &&
                        FixedData().sInitData.lgposRecoverySilentRedoEnd.lGeneration &&
                        FixedData().sInitData.lgposRecoveryVerboseRedoEnd.lGeneration &&
                        ( FixedData().sInitData.lgposRecoveryVerboseRedoEnd.lGeneration - FixedData().sInitData.lgposRecoverySilentRedoEnd.lGeneration ) )
                {
                    clogs = ( FixedData().sInitData.lgposRecoveryVerboseRedoEnd.lGeneration - FixedData().sInitData.lgposRecoverySilentRedoEnd.lGeneration );
                }

                if ( seq == eInitLogRecoveryUndoDone &&
                        FixedData().sInitData.lgposRecoveryVerboseRedoEnd.lGeneration &&
                        FixedData().sInitData.lgposRecoveryUndoEnd.lGeneration &&
                        ( FixedData().sInitData.lgposRecoveryUndoEnd.lGeneration - FixedData().sInitData.lgposRecoveryVerboseRedoEnd.lGeneration ) )
                {
                    clogs = ( FixedData().sInitData.lgposRecoveryUndoEnd.lGeneration - FixedData().sInitData.lgposRecoveryVerboseRedoEnd.lGeneration );
                }
                if ( clogs )
                {
                    OSStrCbFormatW( pwszCurr, cbCurrLeft, L" + %d lgens", clogs );
                    cchUsed = wcslen( pwszCurr );
                    pwszCurr += cchUsed;
                    cbCurrLeft -= ( cchUsed * 2 );
                }
            }
        }

    }

    OSStrCbAppendW( pwszCurr, cbCurrLeft, L"." );
}

void CIsamSequenceDiagLog::TermSequence()
{
    OnDebug( m_tidOwner = 0x0 );

    if ( FAllocatedFixedData_() )
    {
        SEQFIXEDDATA * pT = m_pFixedData;
        m_pFixedData = &s_uDummyFixedData;
        Assert( !FAllocatedFixedData_() );
        delete pT;
    }
    if ( !m_fPreAllocatedSeqDiagInfo )
    {
        delete [] m_rgDiagInfo;
    }
    m_rgDiagInfo = NULL;
    m_fAllocFailure = fFalse;
    m_cseqMax = 0;
    m_cseqMac = 0;
    m_fInitialized = fFalse;
}


extern CJetParam* const g_rgparam;
extern const size_t g_cparam;

INST::INST( INT iInstance )
    :   CZeroInit( sizeof( INST ) ),
#ifdef DEBUGGER_EXTENSION
        m_rgEDBGGlobals( rgEDBGGlobalsArray ),
#else
        m_rgEDBGGlobals( NULL ),
#endif
        m_rgfmp( &g_rgfmp ),
        m_rgpinst( g_rgpinst ),
        m_iInstance( iInstance + 1 ),
        m_updateid( updateidMin ),
        m_critPIB( CLockBasicInfo( CSyncBasicInfo( szPIBGlobal ), rankPIBGlobal, 0 ) ),
        m_critFCBList( CLockBasicInfo( CSyncBasicInfo( szFCBList ), rankFCBList, 0 ) ),
        m_critFCBCreate( CLockBasicInfo( CSyncBasicInfo( szFCBCreate ), rankFCBCreate, 0 ) ),
        m_critLV( CLockBasicInfo( CSyncBasicInfo( szLVCreate ), rankLVCreate, 0 ) ),
        m_critLNPPIB( CLockBasicInfo( CSyncBasicInfo( "ListNodePPIB" ), rankPIBGlobal, 1 ) ),
        m_trxOldestCached(trxMax),
        m_sigTrxOldestDuringRecovery(CSyncBasicInfo( "TrxOldestDuringRecovery" )),
        m_cresPIB( this ),
        m_cresFCB( this ),
        m_cresTDB( this ),
        m_cresIDB( this ),
        m_cresFUCB( this ),
        m_cresSCB( this ),
        m_fTempDBCreated( fFalse ),
        m_critTempDbCreate( CLockBasicInfo( CSyncBasicInfo( szTempDbCreate ), rankTempDbCreate, 0 ) ),
        m_critOLD( CLockBasicInfo( CSyncBasicInfo( szCritOLD ), rankOLD, 0 ) ),
        m_isdlInit( isdltypeInit ),
        m_isdlTerm( isdltypeTerm )
{
    if ( m_iInstance > INST::iInstanceActiveMac )
    {
        INST::iInstanceActiveMac = m_iInstance;
    }
    if ( m_iInstance < INST::iInstanceActiveMin )
    {
        INST::iInstanceActiveMin = m_iInstance;
    }
#ifdef DEBUG
    m_trxNewest = trxMax - 1023;
#else
    m_trxNewest = trxMin;
#endif
    m_fTrxNewestSetByRecovery = fFalse;

    for ( DBID dbid = 0; dbid < dbidMax; dbid++ )
        m_mpdbidifmp[ dbid ] = g_ifmpMax;

    C_ASSERT( sizeof( m_rgfStaticBetaFeatures[0] ) == sizeof( LONG ) );
    for ( ULONG featureid = 0; featureid < _countof( m_rgfStaticBetaFeatures ); featureid++ )
    {
        m_rgfStaticBetaFeatures[featureid] = fUninitBetaFeature;
    }

    InitPERFCounters( m_iInstance );
}

INST::~INST()
{
    for ( DBID dbid = 0; dbid < dbidMax; dbid++ )
        Assert( m_mpdbidifmp[ dbid ] == g_ifmpMax );

    if ( INST::iInstanceActiveMin == m_iInstance )
    {
        while ( (ULONG)INST::iInstanceActiveMin <= g_cpinstMax && pinstNil == g_rgpinst[INST::iInstanceActiveMin-1] )
        {
            INST::iInstanceActiveMin++;
        }
    }
    if ( INST::iInstanceActiveMac == m_iInstance )
    {
        while ( INST::iInstanceActiveMac >= 1 && pinstNil == g_rgpinst[INST::iInstanceActiveMac-1] )
        {
            INST::iInstanceActiveMac--;
        }
    }

    ListNodePPIB *plnppib;
    m_critLNPPIB.Enter();
    if ( NULL != m_plnppibEnd )
    {
        Assert( 0 == m_cUsedSystemPibs );
        Assert( NULL != m_plnppibBegin );
        Assert( m_plnppibEnd->pNext == m_plnppibBegin );
        plnppib     = m_plnppibEnd;
        m_plnppibEnd    = NULL;
        m_plnppibBegin  = NULL;
    }
    else
    {
        plnppib = NULL;
    }
    m_critLNPPIB.Leave();
    if ( NULL != plnppib )
    {
        ListNodePPIB *plnppibFirst = plnppib;
        do
        {
            ListNodePPIB *plnppibMove = plnppib->pNext;
            delete plnppib;
            plnppib = plnppibMove;
        }
        while ( plnppibFirst != plnppib );
    }

    if ( m_wszInstanceName )
    {
        delete[] m_wszInstanceName;
    }
    if ( m_wszDisplayName )
    {
        delete[] m_wszDisplayName;
    }

    delete m_pbackup;
    delete m_plog;
    VER::VERFree( m_pver );

    delete[] m_rgoldstatDB;

    if( m_prbs )
    {
        delete m_prbs;
        m_prbs = NULL;
    }

    if ( m_prbscleaner )
    {
        delete m_prbscleaner;
        m_prbscleaner = NULL;
    }

    delete m_pfsapi;
    delete m_pfsconfig;

    Assert( m_cIOReserved >= 0 );
    while ( m_cIOReserved > 0 )
    {
        UnreserveIOREQ();
    }

    m_isdlInit.TermSequence();
    m_isdlTerm.TermSequence();

    const size_t cProcs = (size_t)OSSyncGetProcessorCountMax();
    const LONG cbPlsMemRequiredPerPerfInstance = g_fDisablePerfmon ? 0 : g_cbPlsMemRequiredPerPerfInstance;

    Assert( g_fDisablePerfmon || ( cbPlsMemRequiredPerPerfInstance > 0 ) );

    if ( cbPlsMemRequiredPerPerfInstance > 0 )
    {
        for ( size_t iProc = 0; iProc < cProcs; iProc++ )
        {
            const ULONG ibOffset = (ULONG)( m_iInstance * cbPlsMemRequiredPerPerfInstance );
            const ULONG cbPerfCountersCommitted = ::Ppls( iProc )->CbPerfCountersCommitted();
            if ( cbPerfCountersCommitted > ibOffset )
            {
                BYTE* const pbDataBuffer = ::Ppls( iProc )->PbGetPerfCounterBuffer( ibOffset, 0 );
                memset( pbDataBuffer, 0, UlFunctionalMin( cbPerfCountersCommitted - ibOffset, cbPlsMemRequiredPerPerfInstance ) );
            }
        }
    }

    for ( size_t iProc = 0; m_rgpls && iProc < cProcs; iProc++ )
    {
        m_rgpls[iProc].~PLS();
    }
    OSMemoryPageFree( m_rgpls );

    m_rwlpoolPIBTrx.Term();

    Assert( m_rgparam != g_rgparam );
    delete [] m_rgparam;
}

VOID INST::SaveDBMSParams( DBMS_PARAM *pdbms_param )
{
    Assert( LOSStrLengthW( SzParam( this, JET_paramSystemPath ) ) <= IFileSystemAPI::cchPathMax );

    if ( JET_errSuccess > ErrOSSTRUnicodeToAscii( SzParam( this, JET_paramSystemPath ),
        pdbms_param->szSystemPathDebugOnly, sizeof(pdbms_param->szSystemPathDebugOnly), NULL, OSSTR_ALLOW_LOSSY )   )
    {
        OSStrCbCopyA( pdbms_param->szSystemPathDebugOnly, sizeof(pdbms_param->szSystemPathDebugOnly), "???" );
    }

    Assert( LOSStrLengthW( SzParam( this, JET_paramLogFilePath ) ) <= IFileSystemAPI::cchPathMax );
    if ( JET_errSuccess > ErrOSSTRUnicodeToAscii( SzParam( this, JET_paramLogFilePath ),
        pdbms_param->szLogFilePathDebugOnly, sizeof(pdbms_param->szLogFilePathDebugOnly), NULL, OSSTR_ALLOW_LOSSY ) )
    {
        OSStrCbCopyA( pdbms_param->szLogFilePathDebugOnly, sizeof(pdbms_param->szLogFilePathDebugOnly), "???" );
    }

    pdbms_param->fDBMSPARAMFlags = 0;
    if ( BoolParam( this, JET_paramCircularLog ) )
        pdbms_param->fDBMSPARAMFlags |= fDBMSPARAMCircularLogging;

    pdbms_param->le_lSessionsMax = (ULONG)UlParam( this, JET_paramMaxSessions );
    pdbms_param->le_lOpenTablesMax = (ULONG)UlParam( this, JET_paramMaxOpenTables );
    pdbms_param->le_lVerPagesMax = (ULONG)UlParam( this, JET_paramMaxVerPages );
    pdbms_param->le_lCursorsMax = (ULONG)UlParam( this, JET_paramMaxCursors );
    pdbms_param->le_lLogBuffers = (ULONG)UlParam( this, JET_paramLogBuffers );
    pdbms_param->le_lcsecLGFile = m_plog->CSecLGFile();
    pdbms_param->le_ulCacheSizeMax = (ULONG)UlParam( this, JET_paramCacheSizeMax );

    memset( pdbms_param->rgbReserved, 0, sizeof(pdbms_param->rgbReserved) );
    return;
}

VOID INST::RestoreDBMSParams( DBMS_PARAM *pdbms_param )
{
    m_plog->SetCSecLGFile( pdbms_param->le_lcsecLGFile );
}

BOOL INST::FSetInstanceName( PCWSTR wszInstanceName )
{
    ULONG newSize = wszInstanceName ? ULONG( wcslen(wszInstanceName) ) : 0;

    if (m_wszInstanceName)
    {
        delete [] m_wszInstanceName;
        m_wszInstanceName = NULL;
    }
    Assert ( NULL ==  m_wszInstanceName);
    if ( 0 == newSize)
    {
        return fTrue;
    }

    m_wszInstanceName = new WCHAR[newSize + 1];
    if ( NULL == m_wszInstanceName)
    {
        return fFalse;
    }
    m_wszInstanceName[0] = 0;

    if ( wszInstanceName )
    {
        OSStrCbCopyW( m_wszInstanceName, (newSize+1)*sizeof(WCHAR), wszInstanceName );
    }

    return fTrue;
}

BOOL INST::FSetDisplayName( PCWSTR wszDisplayName )
{
    ULONG newSize = wszDisplayName ? ULONG( wcslen(wszDisplayName) ) : 0;

    if (m_wszDisplayName)
    {
        delete [] m_wszDisplayName;
        m_wszDisplayName = NULL;
    }
    Assert ( NULL ==  m_wszDisplayName);
    if ( 0 == newSize)
    {
        return fTrue;
    }

    m_wszDisplayName = new WCHAR[newSize + 1];
    if ( NULL == m_wszDisplayName)
    {
        return fFalse;
    }
    m_wszDisplayName[0] = 0;

    if ( wszDisplayName )
    {
        OSStrCbCopyW( m_wszDisplayName, (newSize+1)*sizeof(WCHAR), wszDisplayName );
    }

    return fTrue;
}

ERR INST::ErrGetSystemPib( PIB **pppib )
{
    ERR err = JET_errSuccess;
    ListNodePPIB *plnppib = NULL;
    OPERATION_CONTEXT opContext = { OCUSER_INTERNAL, 0, 0, 0, 0 };
    ENTERCRITICALSECTION enter( &m_critLNPPIB );
    Assert( NULL != m_plnppibEnd );
    Assert( NULL != m_plnppibBegin );

#ifdef SYSTEM_PIB_RFS


    if( g_cSystemPibAllocFail
        && 0 == ( ++g_cSystemPibAlloc % g_cSystemPibAllocFail ) )
    {
        Error( ErrERRCheck( JET_errOutOfSessions ) );
    }

#endif

    if ( FInstanceUnavailable( ) )
    {
        Call( ErrInstanceUnavailableErrorCode( ) );
    }

    if ( m_plnppibEnd != m_plnppibBegin )
    {
        *pppib = m_plnppibBegin->ppib;
#ifdef DEBUG
        m_plnppibBegin->ppib = NULL;
#endif
        m_plnppibBegin = m_plnppibBegin->pNext;
        goto HandleError;
    }
    else if ( m_cOpenedSystemPibs >= cpibSystemFudge )
    {
        Error( ErrERRCheck( JET_errOutOfSessions ) );
    }
    else
    {
        Alloc( plnppib = new ListNodePPIB );

        Call( ErrPIBBeginSession( this, pppib, procidNil, fFalse ) );
        Call( (*pppib)->ErrSetOperationContext( &opContext, sizeof( opContext ) ) );

        m_cOpenedSystemPibs++;
        (*pppib)->grbitCommitDefault = JET_bitCommitLazyFlush;
        (*pppib)->SetFSystemCallback();
        Assert( NULL != m_plnppibEnd );
        plnppib->pNext  = m_plnppibEnd->pNext;
#ifdef DEBUG
        plnppib->ppib   = NULL;
#endif
        m_plnppibEnd->pNext = plnppib;
        plnppib = NULL;
    }

HandleError:
    if( JET_errSuccess == err )
    {
        Assert( *pppib );
        ++m_cUsedSystemPibs;
    }
    else
    {
        Assert( err < JET_errSuccess );
        delete plnppib;
    }
    return err;
}

VOID INST::ReleaseSystemPib( PIB *ppib )
{
    ENTERCRITICALSECTION enter( &m_critLNPPIB );
    Assert( NULL != m_plnppibEnd );
    Assert( NULL == m_plnppibEnd->ppib );
    m_plnppibEnd->ppib  = ppib;
    m_plnppibEnd        = m_plnppibEnd->pNext;
    --m_cUsedSystemPibs;
}

CCriticalSection g_critInst( CLockBasicInfo( CSyncBasicInfo( szInstance ), rankInstance, 0 ) );

INST**  g_rgpinst                       = NULL;
ULONG   g_cpinstMax                     = 0;

ULONG   g_cpinstInit                    = 0;
CRITPOOL< INST* > g_critpoolPinstAPI;

ULONG   g_cTermsInProgress              = 0;

POSTRACEREFLOG g_pJetApiTraceLog        = NULL;


PM_PIF_PROC ProcInfoPIFPwszPf;

void ProcInfoPIFPwszPf( const wchar_t** const pwszFileName, bool* const pfRefreshInstanceList )
{
    if ( pwszFileName != NULL )
    {
        *pwszFileName = WszUtilProcessFileName();
    }

    if ( pfRefreshInstanceList != NULL )
    {
        const bool fRefreshPerfInstanceList = ( AtomicExchange( &g_lRefreshPerfInstanceList, 0 ) > 0 );
        *pfRefreshInstanceList = fRefreshPerfInstanceList;
    }
}


PM_ICF_PROC LProcFriendlyNameICFLPwszPpb;

LOCAL WCHAR g_wszProcName[cchPerfmonInstanceNameMax + 1];
LOCAL const BYTE bProcFriendlyNameAggregationID = 1;

LONG LProcFriendlyNameICFLPwszPpb( _In_ LONG icf, _Inout_opt_ void* const pvParam1, _Inout_opt_ void* const pvParam2 )
{
    switch ( icf )
    {
        case ICFInit:
        {
            (void) ErrOSStrCbFormatW( g_wszProcName, sizeof(g_wszProcName), L"%ws\0" , WszUtilProcessFriendlyName() );

            AtomicExchange( &g_lRefreshPerfInstanceList, 1 );

            return 1;
        }

        case ICFTerm:
            break;

        case ICFData:
        {
            wchar_t** const pwszInstanceNames = (wchar_t**)pvParam1;
            unsigned char** const prgbAggregationIDs = (unsigned char**)pvParam2;

            if ( pwszInstanceNames != NULL && prgbAggregationIDs != NULL )
            {
                *pwszInstanceNames = g_wszProcName;
                *prgbAggregationIDs = (BYTE*)&bProcFriendlyNameAggregationID;

                return 1;
            }
        }
            break;

        default:
            AssertSz( fFalse, "Unknown icf %d", icf );
            break;
    }

    return 0;
}

TableClassNamesLifetimeManager g_tableclassnames;

PM_ICF_PROC LTableClassNamesICFLPwszPpb;

#define cchTCESuffixMax     (10)
const WCHAR * const g_wszUnknown    = L"_Unknown";
const WCHAR * const g_wszCatalog    = L"_Catalog";
const WCHAR * const g_wszShadowCatalog  = L"_ShadowCatalog";
const WCHAR * const g_wszPrimary    = L" (Primary)";
const WCHAR * const g_wszLV     = L" (LV)";
const WCHAR * const g_wszIndex  = L" (Indices)";
const WCHAR * const g_wszSpace = L" (Space)";

#define cchTCENameMax       (JET_cbNameMost + cchTCESuffixMax)

BYTE    g_pbAggregationIDs[tceMax] = { 0 };
LONG    g_cTableClassNames = 0;

LOCAL LONG CPERFTCEObjects( const ULONG paramidMost )
{
    ULONG cPublicTableClassParametersUsed = paramidMost >= JET_paramTableClass16Name ?
        paramidMost - JET_paramTableClass16Name + 1 + JET_paramTableClass15Name - JET_paramTableClass1Name + 1 :
        paramidMost - JET_paramTableClass1Name + 1;
    return 1 + ( tcePerTableClass * ( tableclassReservedMost - tableclassReservedMin + 1 ) ) + ( tcePerTableClass * cPublicTableClassParametersUsed );
}

LONG CPERFTCEObjects()
{
    ULONG paramidMost = JET_paramTableClass1Name - 1;
    for ( ULONG paramid = JET_paramTableClass1Name; paramid <= JET_paramTableClass15Name; paramid++ )
    {
        if ( !FDefaultParam( paramid ) )
        {
            paramidMost = paramid;
        }
    }

    for ( ULONG paramid = JET_paramTableClass16Name; paramid <= JET_paramTableClass31Name; paramid++ )
    {
        if ( !FDefaultParam( paramid ) )
        {
            paramidMost = paramid;
        }
    }

    return CPERFTCEObjects( paramidMost );
}


VOID InitTableClassNames()
{
    const size_t cchTableClassNames = tceMax * (cchTCENameMax + 1) + 1;

    const JET_ERR errAllocation = g_tableclassnames.ErrAllocateIfNeeded( cchTableClassNames );

    if ( errAllocation != JET_errSuccess )
    {
        return;
    }

    const LONG cTCEObjects = CPERFTCEObjects();

    if ( cTCEObjects == g_cTableClassNames )
    {
        return;
    }

    WCHAR* const wszTableClassNames = g_tableclassnames.WszTableClassNames();

    AssertSz( wszTableClassNames != NULL, "wszTableClassNames should have been initialized in ErrUtilPerfInit." );

    WCHAR* wszT = wszTableClassNames;
    const WCHAR* const wszTEnd = wszTableClassNames + cchTableClassNames;


    AtomicExchange( &g_lRefreshPerfInstanceList, 1 );

    OSStrCbFormatW( wszT, ( wszTEnd - wszT ) * sizeof( WCHAR ), L"%ws\0" , g_wszUnknown );

    wszT += wcslen( wszT ) + 1;

    for ( TCE tce = tceMin; tce < cTCEObjects; ++tce )
    {
        INT tableclass = (INT) TableClassFromTCE( tce );

        const WCHAR * wszSuffix = NULL;
        if ( FIsIndex( tce ) )
        {
            wszSuffix = g_wszIndex;
        }
        else if ( FIsLV( tce ) )
        {
            wszSuffix = g_wszLV;
        }
        else if ( FIsSpace( tce ) )
        {
            wszSuffix = g_wszSpace;
        }
        else
        {
            wszSuffix = g_wszPrimary;
        }

        PCWSTR wszParam;

        switch( tableclass )
        {
            case tableclassCatalog:
                wszParam = g_wszCatalog;
                break;

            case tableclassShadowCatalog:
                wszParam = g_wszShadowCatalog;
                break;

            default:
            {
                Assert( tableclass >= tableClassPublic );
                ULONG paramid = JET_paramTableClass1Name + tableclass - tableClassPublic;
                if ( paramid > JET_paramTableClass15Name )
                {
                    paramid += JET_paramTableClass16Name - JET_paramTableClass15Name - 1;
                }
                wszParam = SzParam( paramid );
            }
                break;
        }

        OSStrCbFormatW( wszT, (wszTEnd - wszT) * sizeof( WCHAR ), L"%s%ws\0" , wszParam,  wszSuffix );

        wszT += wcslen( wszT ) + 1;
    }

    g_cTableClassNames = cTCEObjects;

    for ( LONG iAggregationID = 0; iAggregationID < g_cTableClassNames; iAggregationID++ )
    {
        g_pbAggregationIDs[iAggregationID] = (BYTE)( iAggregationID + 1 );
        Assert( iAggregationID < UCHAR_MAX );
    }
}

LONG LTableClassNamesICFLPwszPpb( _In_ LONG icf, _Inout_opt_ void* const pvParam1, _Inout_opt_ void* const pvParam2 )
{
    switch ( icf )
    {
        case ICFInit:
        {
            InitTableClassNames();

            return CPERFTCEObjects( JET_paramTableClass31Name );
        }

        case ICFTerm:
            break;

        case ICFData:
        {
            wchar_t** const pwszInstanceNames = (wchar_t**)pvParam1;
            unsigned char** const prgbAggregationIDs = (unsigned char**)pvParam2;

            if ( pwszInstanceNames != NULL && prgbAggregationIDs != NULL )
            {
                InitTableClassNames();
                WCHAR* const wszTableClassNames = g_tableclassnames.WszTableClassNames();

                if ( wszTableClassNames )
                {
                    *pwszInstanceNames = wszTableClassNames;
                }
                else
                {
                    *pwszInstanceNames = L"";

                    Assert( 0 == g_cTableClassNames );
                    g_cTableClassNames = 0;
                }
                *prgbAggregationIDs = g_pbAggregationIDs;

                return g_cTableClassNames;
            }
        }
            break;

        default:
            AssertSz( fFalse, "Unknown icf %d", icf );
            break;
    }

    return 0;
}

RUNINSTMODE g_runInstMode = runInstModeNoSet;

INLINE VOID RUNINSTSetMode( RUNINSTMODE newMode )
{
    Assert ( g_critInst.FOwner() );
    Assert ( 0 == g_cpinstInit );

    if ( newMode != runInstModeNoSet )
    {
        FixDefaultSystemParameters();
    }
    else
    {
        Assert( g_cpinstInit == 0 );
        g_cpinstMax = 0;
        g_ifmpMax = 0;
    }

    g_runInstMode = newMode;
}

INLINE VOID RUNINSTSetModeOneInst()
{
    Assert ( runInstModeNoSet == g_runInstMode );
    RUNINSTSetMode(runInstModeOneInst);
    g_cpinstMax = 1;
    g_ifmpMax = g_cpinstMax * dbidMax + cfmpReserved;
}

INLINE VOID RUNINSTSetModeMultiInst()
{
    Assert ( runInstModeNoSet == g_runInstMode );
    RUNINSTSetMode(runInstModeMultiInst);
    g_cpinstMax = (ULONG)UlParam( JET_paramMaxInstances );
    g_ifmpMax = g_cpinstMax * dbidMax + cfmpReserved;
}

INLINE RUNINSTMODE RUNINSTGetMode()
{
    Assert ( runInstModeNoSet != g_runInstMode || 0 == g_cpinstInit );
    return g_runInstMode;
}


LOCAL ERR ErrRUNINSTCheckAndSetOneInstMode()
{
    ERR     err     = JET_errSuccess;

    INST::EnterCritInst();
    if ( RUNINSTGetMode() == runInstModeNoSet )
    {
        Assert( g_cpinstInit == 0 );
        RUNINSTSetModeOneInst();
    }
    else if ( RUNINSTGetMode() == runInstModeMultiInst )
    {
        err = ErrERRCheck( JET_errRunningInMultiInstanceMode );
    }
    INST::LeaveCritInst();

    return err;
}


LOCAL ERR ErrRUNINSTCheckOneInstMode()
{
    ERR     err     = JET_errSuccess;

    INST::EnterCritInst();
    if ( RUNINSTGetMode() == runInstModeNoSet )
    {
        Assert( g_cpinstInit == 0 );
        AssertSz( FNegTest( fInvalidAPIUsage ), "This is invalid API usage. Why don't you have an instance initialized?" );
        err = ErrERRCheck( JET_errInvalidParameter );
    }
    else if ( RUNINSTGetMode() == runInstModeMultiInst )
    {
        err = ErrERRCheck( JET_errRunningInMultiInstanceMode );
    }
    INST::LeaveCritInst();

    return err;
}


INLINE BOOL FINSTInvalid( const JET_INSTANCE instance )
{
    return ( 0 == instance || (JET_INSTANCE)JET_instanceNil == instance );
}


BOOL FINSTSomeInitialized()
{
    const BOOL  fInitAlready    = ( g_rgpinst != NULL );
#ifdef DEBUG
    ULONG cinsts = 0;
    if ( g_rgpinst )
    {
        for ( ULONG ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
        {
            if ( pinstNil != g_rgpinst[ ipinst ] )
            {
                cinsts++;
            }
        }
    }
    if ( fInitAlready )
    {
        Assert( g_cpinstInit > 0 );
        Assert( cinsts > 0 );
    }
    else
    {
        Assert( g_cpinstInit == 0 );
        Assert( cinsts == 0 );
    }
#endif
    return fInitAlready;
}

const WCHAR wszCachingFile[]                = L"Instance";
const WCHAR wszCachingFileExtension[]       = L".JBC";
const WCHAR wszDisplacedDataFileExtension[] = L".JBCAUX";

class CInstanceFileSystemConfiguration : public CDefaultFileSystemConfiguration
{
    public:

        CInstanceFileSystemConfiguration( INST * const pinst )
            :   m_pinst( pinst ),
                m_permillageSmoothIo( dwMax )
        {
            WCHAR wszBuf[ 16 ] = { 0 };
            if (    FOSConfigGet( L"DEBUG", L"BlockCacheEnabled", wszBuf, sizeof( wszBuf ) ) &&
                    wszBuf[ 0 ] )
            {
                m_fBlockCacheEnabled = !!_wtol( wszBuf );
            }
        }

    public:


        ULONG DtickAccessDeniedRetryPeriod() override
        {
            return (TICK)UlParam( m_pinst, JET_paramAccessDeniedRetryPeriod );
        }






        ULONG DtickHungIOThreshhold() override
        {
            return (DWORD)UlParam( m_pinst, JET_paramHungIOThreshold );
        }

        DWORD GrbitHungIOActions() override
        {
            return (DWORD)UlParam( m_pinst, JET_paramHungIOActions );
        }

        ULONG CbMaxReadSize() override
        {
            return (ULONG)UlParam( m_pinst, JET_paramMaxCoalesceReadSize );
        }

        ULONG CbMaxWriteSize() override
        {
            return (ULONG)UlParam( m_pinst, JET_paramMaxCoalesceWriteSize );
        }

        ULONG CbMaxReadGapSize() override
        {
            return (DWORD)UlParam( m_pinst, JET_paramMaxCoalesceReadGapSize );
        }

        ULONG PermillageSmoothIo() override
        {
            if ( m_permillageSmoothIo == dwMax )
            {
                ULONG permillageSmoothIo = OnDebugOrRetail( 2, CDefaultFileSystemConfiguration::PermillageSmoothIo() );

                if ( m_pinst )
                {
                    if ( !FDefaultParam( m_pinst, JET_paramFlight_SmoothIoTestPermillage ) )
                    {
                        permillageSmoothIo = (ULONG)UlParam( m_pinst, JET_paramFlight_SmoothIoTestPermillage );
                    }
                }
                Assert( permillageSmoothIo != dwMax );

                m_permillageSmoothIo = permillageSmoothIo;
            }

            Assert( m_permillageSmoothIo != dwMax );
            return m_permillageSmoothIo;
        }

        BOOL FBlockCacheEnabled() override
        {
            if ( m_pinst == pinstNil || !PvParam( m_pinst, JET_paramBlockCacheConfiguration ) )
            {
                return m_fBlockCacheEnabled;
            }

            return fTrue;
        }

        ERR ErrGetBlockCacheConfiguration( _Out_ IBlockCacheConfiguration** const ppbcconfig ) override
        {
            ERR err = JET_errSuccess;

            if ( m_pinst == pinstNil || !PvParam( m_pinst, JET_paramBlockCacheConfiguration ) )
            {
                Alloc( *ppbcconfig = new CInstanceBlockCacheConfiguration( m_pinst ) );
            }
            else
            {
                IBlockCacheConfiguration* pbcconfig = (IBlockCacheConfiguration*)PvParam( m_pinst, JET_paramBlockCacheConfiguration );
                Alloc( *ppbcconfig = new CBlockCacheConfigurationWrapper( pbcconfig ) );
            }

        HandleError:
            return err;
        }

    private:

        class CInstanceBlockCacheConfiguration : public IBlockCacheConfiguration
        {
            public:

                CInstanceBlockCacheConfiguration( _In_ INST* const pinst )
                    :   m_pinst( pinst )
                {
                }

            public:

                ERR ErrGetCachedFileConfiguration(  _In_z_  const WCHAR* const                  wszKeyPathCachedFile,
                                                    _Out_   ICachedFileConfiguration** const    ppcfconfig  ) override
                {
                    ERR err = JET_errSuccess;

                    Alloc( *ppcfconfig = new CInstanceCachedFileConfiguration( m_pinst, wszKeyPathCachedFile ) );

                HandleError:
                    return err;
                }

                ERR ErrGetCacheConfiguration(   _In_z_  const WCHAR* const          wszKeyPathCachingFile,
                                                _Out_   ICacheConfiguration** const ppcconfig ) override
                {
                    ERR err = JET_errSuccess;

                    Alloc( *ppcconfig = new CInstanceCacheConfiguration( m_pinst ) );

                HandleError:
                    return err;
                }

            private:

                INST* const m_pinst;
        };

        static void GetCachingFileFromInst( _In_                                INST* const     pinst,
                                            _Out_writes_z_( OSFSAPI_MAX_PATH )  WCHAR* const    wszAbsPathCachingFile )
        {
            WCHAR   rgwszDrive[ OSFSAPI_MAX_PATH ]  = { 0 };
            WCHAR   rgwszFolder[ OSFSAPI_MAX_PATH ] = { 0 };

            if (    pinst == pinstNil ||
                    _wsplitpath_s(  SzParam( pinst, JET_paramSystemPath ),
                                    rgwszDrive,
                                    _countof( rgwszDrive ),
                                    rgwszFolder,
                                    _countof( rgwszFolder ),
                                    NULL,
                                    0,
                                    NULL,
                                    0 ) ||
                    _wmakepath_s(   wszAbsPathCachingFile,
                                    OSFSAPI_MAX_PATH,
                                    rgwszDrive,
                                    rgwszFolder,
                                    pinst->m_wszInstanceName && pinst->m_wszInstanceName[ 0 ] ?
                                        pinst->m_wszInstanceName :
                                        wszCachingFile,
                                    wszCachingFileExtension ) )
            {
                wszAbsPathCachingFile[ 0 ] = 0;
            }
        }

        class CInstanceCachedFileConfiguration : public CDefaultCachedFileConfiguration
        {
            public:

                CInstanceCachedFileConfiguration(   _In_ INST* const            pinst,
                                                    _In_z_  const WCHAR* const  wszKeyPathCachedFile )
                {
                    WCHAR rgwszExt[ OSFSAPI_MAX_PATH ] = { 0 };

                    GetCachingFileFromInst( pinst, m_wszAbsPathCachingFile );

                    if ( _wsplitpath_s( wszKeyPathCachedFile,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        rgwszExt,
                                        _countof( rgwszExt ) ) ||
                        UtilCmpFileName( rgwszExt, wszCachingFileExtension ) == 0 ||
                        UtilCmpFileName( rgwszExt, wszDisplacedDataFileExtension ) == 0 )
                    {
                        m_wszAbsPathCachingFile[ 0 ] = 0;
                    }

                    m_fCachingEnabled = m_wszAbsPathCachingFile[ 0 ] != 0;
                }
        };

        class CInstanceCacheConfiguration : public CDefaultCacheConfiguration
        {
            public:

                CInstanceCacheConfiguration( _In_ INST* const pinst )
                {
                    GetCachingFileFromInst( pinst, m_wszAbsPathCachingFile );

                    m_fCacheEnabled = m_wszAbsPathCachingFile[ 0 ] != 0;
                }
        };

        class CBlockCacheConfigurationWrapper : public IBlockCacheConfiguration
        {
            public:

                CBlockCacheConfigurationWrapper( _In_ IBlockCacheConfiguration* const pbcconfig )
                    :   m_pbcconfig( pbcconfig )
                {
                }

            public:

                ERR ErrGetCachedFileConfiguration(  _In_z_  const WCHAR* const                  wszKeyPathCachedFile,
                                                    _Out_   ICachedFileConfiguration** const    ppcfconfig  ) override
                {
                    return m_pbcconfig->ErrGetCachedFileConfiguration( wszKeyPathCachedFile, ppcfconfig );
                }

                ERR ErrGetCacheConfiguration(   _In_z_  const WCHAR* const          wszKeyPathCachingFile,
                                                _Out_   ICacheConfiguration** const ppcconfig ) override
                {
                    return m_pbcconfig->ErrGetCacheConfiguration( wszKeyPathCachingFile, ppcconfig );
                }

            private:

                IBlockCacheConfiguration* const m_pbcconfig;
        };

    public:

        virtual void EmitEvent( const EEventType    type,
                                const CategoryId    catid,
                                const MessageId     msgid,
                                const DWORD         cString,
                                const WCHAR *       rgpszString[],
                                const LONG          lEventLoggingLevel )
        {
            UtilReportEvent(    type,
                                catid,
                                msgid,
                                cString,
                                rgpszString,
                                0,
                                NULL,
                                m_pinst,
                                lEventLoggingLevel);
        }

        virtual void EmitEvent( int                 haTag,
                                const CategoryId    catid,
                                const MessageId     msgid,
                                const DWORD         cString,
                                const WCHAR *       rgpszString[],
                                int                 haCategory,
                                const WCHAR*        wszFilename,
                                unsigned _int64     qwOffset,
                                DWORD               cbSize )
        {
            OSUHAPublishEvent(
                (HaDbFailureTag)haTag,
                m_pinst,
                catid,
                (HaDbIoErrorCategory)haCategory,
                wszFilename,
                qwOffset,
                cbSize,
                msgid,
                cString,
                rgpszString );
        }

        virtual void EmitFailureTag(    const int           haTag,
                                        const WCHAR* const  wszGuid,
                                        const WCHAR* const  wszAdditional )
        {
            OSUHAEmitFailureTagEx( m_pinst, (HaDbFailureTag)haTag, wszGuid, wszAdditional );
        }

        virtual const void* const PvTraceContext()
        {
            return m_pinst;
        }

    private:

        INST* const m_pinst;
        ULONG       m_permillageSmoothIo;
};

CInstanceFileSystemConfiguration g_fsconfigGlobal( pinstNil );
IFileSystemConfiguration* const g_pfsconfigGlobal = &g_fsconfigGlobal;

ERR ErrNewInst(
    INST **         ppinst,
    PCWSTR          wszInstanceName,
    PCWSTR          wszDisplayName,
    INT *           pipinst )
{
    ERR             err         = JET_errSuccess;
    INST *          pinst       = NULL;
    BOOL            fOSUInit    = fFalse;
    BOOL            fSysInit    = fFalse;
    ULONG           ipinst;
    CIsamSequenceDiagLog    isdlSysTimings( isdltypeOsinit );

    Assert( INST::FOwnerCritInst() );

    isdlSysTimings.InitSequence( isdltypeOsinit, eInitSelectAllocInstDone + 1 );


    if ( 0 == g_cpinstInit )
    {
        Call( ErrOSUInit() );
        fOSUInit = fTrue;
        isdlSysTimings.Trigger( eInitOslayerDone );

        Call( ErrIsamSystemInit() );
        fSysInit = fTrue;
        isdlSysTimings.Trigger( eInitIsamSystemDone );
    }

    if ( g_cpinstInit >= g_cpinstMax )
    {
        Error( ErrERRCheck( JET_errTooManyInstances ) );
    }


    for ( ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
    {
        if ( pinstNil == g_rgpinst[ ipinst ] )
        {
            continue;
        }
        if ( NULL == wszInstanceName &&
             NULL == g_rgpinst[ ipinst ]->m_wszInstanceName )
        {
            Error( ErrERRCheck( JET_errInstanceNameInUse ) );
        }
        if ( NULL != wszInstanceName &&
             NULL != g_rgpinst[ ipinst ]->m_wszInstanceName &&
             0 == wcscmp( wszInstanceName, g_rgpinst[ ipinst ]->m_wszInstanceName ) )
        {
            Error( ErrERRCheck( JET_errInstanceNameInUse ) );
        }
    }

    for ( ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
    {
        if ( pinstNil == g_rgpinst[ ipinst ] )
        {
            break;
        }
    }
    Assert( g_cpinstMax > ipinst );

    if ( !g_fDisablePerfmon && !PLS::FEnsurePerfCounterBuffer( CPERFESEInstanceObjects( ipinst + 1 ) * g_cbPlsMemRequiredPerPerfInstance ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    Alloc( pinst = new INST( ipinst ) );

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlInitBegin|sysosrtlContextInst, pinst, &ipinst, sizeof(ipinst) );
    isdlSysTimings.Trigger( eInitSelectAllocInstDone );

    pinst->m_isdlInit.InitConsumeSequence( isdltypeInit, &isdlSysTimings, eInitSeqMax );


#ifdef DEBUG
    for ( size_t iparam = 0; iparam < g_cparam; iparam++ )
    {
        Assert( iparam == g_rgparam[ iparam ].m_paramid );

        if ( g_rgparam[ iparam ].Type_() == JetParam::typeBoolean ||
             g_rgparam[ iparam ].Type_() == JetParam::typeGrbit ||
             g_rgparam[ iparam ].Type_() == JetParam::typeInteger ||
             g_rgparam[ iparam ].Type_() == JetParam::typeBlockSize )
        {
            Expected( g_rgparam[ iparam ].m_valueDefault[0] >= g_rgparam[ iparam ].m_rangeLow );
            Expected( g_rgparam[ iparam ].m_valueDefault[0] <= g_rgparam[ iparam ].m_rangeHigh );
            Expected( g_rgparam[ iparam ].m_valueDefault[1] >= g_rgparam[ iparam ].m_rangeLow );
            Expected( g_rgparam[ iparam ].m_valueDefault[1] <= g_rgparam[ iparam ].m_rangeHigh );
            Expected( g_rgparam[ iparam ].m_valueCurrent >= g_rgparam[ iparam ].m_rangeLow );
            Expected( g_rgparam[ iparam ].m_valueCurrent <= g_rgparam[ iparam ].m_rangeHigh );
        }

        AssertSz( g_rgparam[ iparam ].FGlobal() || !g_rgparam[ iparam ].m_fMayNotWriteAfterGlobalInit,
        "Inconsistent param (%d). Why do you have a param that claims to be instance wide, but can't be changed after engine system init?  That effectively makes it global, mark it as such.\n", iparam);
    }
#endif

    Alloc( pinst->m_rgparam = new CJetParam[ g_cparam ] );
    for ( size_t iparam = 0; iparam < g_cparam; iparam++ )
    {
        Call( g_rgparam[ iparam ].Clone( pinstNil, ppibNil, &pinst->m_rgparam[ iparam ], pinst, ppibNil ) );
    }

    if ( !g_rgparam[ JET_paramEngineFormatVersion ].FWritten() )
    {
        BOOL fExpired = fFalse;
        if ( ErrUtilOsDowngradeWindowExpired( &fExpired ) >= JET_errSuccess &&
             fExpired )
        {
            Expected( JET_efvUseEngineDefault == UlParam( pinst, JET_paramEngineFormatVersion ) );
            pinst->m_rgparam[ JET_paramEngineFormatVersion ].Reset( pinst, JET_efvUseEngineDefault );
        }
        else
        {
            pinst->m_rgparam[ JET_paramEngineFormatVersion ].Reset( pinst, JET_efvExchange2016Cu1Rtm | JET_efvAllowHigherPersistedFormat );
        }
    }

    if ( !pinst->FSetInstanceName( wszInstanceName ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    if ( !pinst->FSetDisplayName( wszDisplayName ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    if ( !pinst->m_rwlpoolPIBTrx.FInit( 64 * OSSyncGetProcessorCount() / 4, rankPIBTrx, szPIBTrx ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    Alloc( pinst->m_pfsconfig = new CInstanceFileSystemConfiguration( pinst ) );
    Call( ErrOSFSCreate( pinst->m_pfsconfig, &pinst->m_pfsapi ) );

    Alloc( pinst->m_plnppibEnd = new INST::ListNodePPIB );
#ifdef DEBUG
    pinst->m_plnppibEnd->ppib = NULL;
#endif
    pinst->m_plnppibEnd->pNext = pinst->m_plnppibEnd;
    pinst->m_plnppibBegin = pinst->m_plnppibEnd;

    Alloc( pinst->m_plog = new LOG( pinst ) );
    Call( pinst->m_plog->ErrLGPreInit() );
    Alloc( pinst->m_pbackup = new BACKUP_CONTEXT( pinst ) );

    Alloc( pinst->m_rgoldstatDB = new OLDDB_STATUS[ dbidMax ] );

    pinst->m_cpls = OSSyncGetProcessorCountMax();
    Alloc( pinst->m_rgpls = (INST::PLS*)PvOSMemoryPageAlloc( pinst->m_cpls * sizeof( INST::PLS ), NULL ) );
    for ( size_t iProc = 0; pinst->m_rgpls && iProc < pinst->m_cpls; iProc++ )
    {
        new(&pinst->m_rgpls[iProc]) INST::PLS;
    }

    AtomicExchangePointer( (void **)&g_rgpinst[ ipinst ], pinst );
    g_cpinstInit++;
    EnforceSz( ipinst < g_cpinstMax, "CorruptedInstArrayInNewInst" );

    PERFSetInstanceNames();

    INST::IncAllocInstances();
    *ppinst = pinst;
    if ( pipinst )
    {
        *pipinst = ipinst;
    }

HandleError:

    if ( err < JET_errSuccess )
    {
        delete pinst;
        if ( g_cpinstInit == 0 )
        {
            if ( fSysInit )
            {
                IsamSystemTerm();
            }

            if ( fOSUInit )
            {
                OSUTerm();
            }

            RUNINSTSetMode( runInstModeNoSet );
        }
        *ppinst = NULL;
        if ( pipinst )
        {
            *pipinst = -1;
        }
        isdlSysTimings.TermSequence();
    }

    return err;
}

VOID FreePinst( INST *pinst )
{
    Assert( pinst );



    INST::EnterCritInst();

    for ( size_t ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
    {
        if ( g_rgpinst[ ipinst ] == pinst )
        {

            CCriticalSection *pcritInst = &g_critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
            pcritInst->Enter();

            if ( g_rgpinst[ ipinst ] == pinst )
            {
                pinst->APILock( pinst->fAPIDeleting );
                AtomicExchangePointer( (void **)&g_rgpinst[ ipinst ], pinstNil );
                g_cpinstInit--;

                OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlInstDelete|sysosrtlContextInst, pinst, (PVOID)&pinst->m_iInstance, sizeof(pinst->m_iInstance) );

                delete pinst;
                (VOID)INST::DecAllocInstances();
                pcritInst->Leave();
                PERFSetInstanceNames();

                if ( g_cpinstInit == 0 )
                {
                    IsamSystemTerm();
                    OSUTerm();
                    Assert( RUNINSTGetMode() == runInstModeOneInst || RUNINSTGetMode() == runInstModeMultiInst );
                    RUNINSTSetMode( runInstModeNoSet );
                }
            }
            else
            {
                pcritInst->Leave();
            }

            break;
        }
    }

    INST::LeaveCritInst();
}

INLINE VOID SetPinst( JET_INSTANCE *pinstance, JET_SESID sesid, INST **ppinst )
{
    if ( !sesid || sesid == JET_sesidNil )
    {

        if ( pinstance && g_cpinstInit && *pinstance && *pinstance != JET_instanceNil )
            *ppinst = *(INST **) pinstance;
        else
            *ppinst = pinstNil;
    }
    else
    {
        *ppinst = PinstFromPpib( (PIB *)sesid );
    }
}

LOCAL ERR ErrFindPinst( JET_INSTANCE jinst, INST **ppinst, INT *pipinst = NULL )
{
    INST                *pinst  = (INST *)jinst;
    UINT                ipinst;
    const RUNINSTMODE   mode    = RUNINSTGetMode();

    Assert( ppinst );

    switch ( mode )
    {
        case runInstModeOneInst:
            for ( ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
            {
                if ( pinstNil != g_rgpinst[ ipinst ] )
                {
                    Assert( ipinst == 0 );
                    *ppinst = g_rgpinst[ ipinst ];
                    if ( pipinst )
                        *pipinst = ipinst;
                    return JET_errSuccess;
                }
            }
            break;
        case runInstModeMultiInst:
            if ( FINSTInvalid( jinst ) )
            {
                return ErrERRCheck( JET_errInvalidParameter );
            }
            for ( ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
            {
                if ( pinstNil != g_rgpinst[ ipinst ]
                    && pinst == g_rgpinst[ ipinst ] )
                {
                    *ppinst = g_rgpinst[ ipinst ];
                    if ( pipinst )
                        *pipinst = ipinst;
                    return JET_errSuccess;
                }
            }
            break;
        default:
            Assert( runInstModeNoSet == mode );
    }

    return ErrERRCheck( JET_errInvalidParameter );
}

INLINE INST *PinstFromSesid( JET_SESID sesid )
{
    AssertSzRTL( JET_SESID( ppibNil ) != sesid, "Invalid (NULL) Session ID parameter." );
    AssertSzRTL( JET_sesidNil != sesid, "Invalid (JET_sesidNil) Session ID parameter." );
    INST *pinst = PinstFromPpib( (PIB*)sesid );
    AssertSzRTL( NULL != pinst, "Invalid Session ID parameter - has NULL as an instance." );
    AssertSzRTL( (INST*)JET_instanceNil != pinst, "Invalid Session ID parameter - has (JET_instanceNil) as an instance." );
    return pinst;
}


VOID INST::SetInstanceUnavailable( const ERR err )
{
    Assert( err < JET_errSuccess );

    if ( JET_errSuccess == m_errInstanceUnavailable )
    {
        m_errInstanceUnavailable = err;

        ERR errLog;
        if ( JET_errLogWriteFail == err
            && m_plog->FNoMoreLogWrite( &errLog ) )
        {
            if ( JET_errSuccess != errLog )
            {
                m_errInstanceUnavailable = errLog;
            }
        }

        OSUHAEmitFailureTag( this, HaDbFailureTagRemount, L"78cac6b2-f8ab-477e-9301-a11e881d9012" );

        SetStatusError();
    }
}



#ifdef PROFILE_JET_API

#ifdef DEBUG
#else
#error "PROFILE_JET_API can be defined only in debug mode"
#endif

#define PROFILE_LOG_JET_OP              1
#define PROFILE_LOG_CALLS_SUMMARY       2

INT     profile_detailLevel     = 0;
FILE *  profile_pFile           = 0;
QWORD   profile_csecTimeOffset  = 0;
WCHAR   profile_wszFileName[ IFileSystemAPI::cchPathMax ]    = L"";

#define ProfileHrtPrintMs__( x ) (((double)(signed __int64)(x)*1000)/(signed _int64)HrtHRTFreq())

#endif



class APICALL
{
    protected:
        ERR         m_err;
        INT         m_op;
        INT         m_opOuter;
        IOREASONTERTIARY m_iortOuter;

#ifdef PROFILE_JET_API
        INT         m_profile_opCurrentOp;
        CHAR *      m_profile_szCurrentOp;
        HRT         m_profile_hrtStart;
#endif

        void SetJetOpInTraceContext( const INT op )
        {
            TraceContext* ptc = const_cast<TraceContext*>( PetcTLSGetEngineContext() );

            Assert( ptc->iorReason.Ioru() == ioruNone || Ptls()->fInCallback );
            Assert( op <= ioruMax );
            m_opOuter = ptc->iorReason.Ioru();
            ptc->iorReason.SetIoru( (IOREASONUSER) op );
            m_iortOuter = ptc->iorReason.Iort();
            ptc->iorReason.SetIort( iortISAM );
        }

        void ClearJetOpInTraceContext()
        {
            TraceContext* ptc = const_cast<TraceContext*>( PetcTLSGetEngineContext() );

            Assert( ptc->iorReason.Ioru() != ioruNone );
            Assert( m_opOuter == ioruNone || Ptls()->fInCallback );
            ptc->iorReason.SetIoru( (IOREASONUSER) m_opOuter );
            ptc->iorReason.SetIort( m_iortOuter );
        }

    public:
        APICALL( const INT op ) : m_op( op ), m_opOuter( ioruNone )
        {
            SetJetOpInTraceContext( op );

#ifdef PROFILE_JET_API
            if ( opMax != op
                && ( profile_detailLevel & PROFILE_LOG_JET_OP )
                && NULL != profile_pFile )
            {
                m_profile_opCurrentOp = op;
                m_profile_hrtStart = HrtHRTCount();
            }
            else
            {
                m_profile_opCurrentOp = opMax;
            }
#endif
        }

        ~APICALL()
        {
#ifdef PROFILE_JET_API
            if ( opMax != m_profile_opCurrentOp )
            {
                Assert( NULL != profile_pFile );
                Assert( profile_detailLevel & PROFILE_LOG_JET_OP );

                const HRT   profile_hrtEnd  = HrtHRTCount();
                const QWORD csecTime        = m_profile_hrtStart/HrtHRTFreq() + profile_csecTimeOffset;
                const INT   hour            = (INT)( ( csecTime / 3600 ) % 24 );
                const INT   min             = (INT)( ( csecTime / 60 ) % 60 );
                const INT   sec             = (INT)( csecTime % 60 );

                fprintf(
                    profile_pFile,
                    "%02d:%02d:%02d %03d %.3f ms\n",
                    hour,
                    min,
                    sec,
                    m_profile_opCurrentOp,
                    ProfileHrtPrintMs__( profile_hrtEnd - m_profile_hrtStart ) );
            }
#endif

            ClearJetOpInTraceContext();
        }

        ERR ErrResult() const
        {
            AssertRTL( m_err > -65536 && m_err < 65536 );
            if ( m_err < JET_errSuccess )
            {
                OSDiagTrackJetApiError( L"", m_op, m_err );
            }
            return m_err;
        }

        VOID SetErr( const ERR err )        { AssertRTL( err > -65536 && err < 65536 ); m_err = err; }
};

class APICALL_INST : public APICALL
{
    private:
        INST *      m_pinst;

    public:
        APICALL_INST( const INT op ) : APICALL( op ), m_pinst( NULL )   {}
        ~APICALL_INST()                                                 {}

        INST * Pinst()                                  { return m_pinst; }

        ERR ErrResult() const
        {
            AssertRTL( m_err > -65536 && m_err < 65536 );
            if ( m_err < JET_errSuccess )
            {
                const WCHAR* wszInstDisplayName = ( m_pinst != NULL && m_pinst->m_wszDisplayName != NULL ? m_pinst->m_wszDisplayName : L"_unknown_" );
                OSDiagTrackJetApiError( wszInstDisplayName, m_op, m_err );
            }
            return m_err;
        }

        BOOL FEnter( const JET_INSTANCE instance )
        {
            const ERR   errT    =   ErrFindPinst( instance, &m_pinst );

            SetErr( errT >= JET_errSuccess ?
                        m_pinst->ErrAPIEnter( fFalse ) :
                        errT );

            return ( m_err >= JET_errSuccess );
        }

        BOOL FEnterWithoutInit( const JET_INSTANCE instance )
        {
            const ERR   errT    =   ErrFindPinst( instance, &m_pinst );

            SetErr( errT >= JET_errSuccess ?
                        m_pinst->ErrAPIEnterWithoutInit( fFalse ) :
                        errT );

            return ( m_err >= JET_errSuccess );
        }

        BOOL FEnterForInit( INST * const pinst )
        {
            m_pinst = pinst;
            SetErr( pinst->ErrAPIEnterForInit() );
            return ( m_err >= JET_errSuccess );
        }

        BOOL FEnterForTerm( INST * const pinst )
        {
            m_pinst = pinst;
            SetErr( pinst->ErrAPIEnter( fTrue ) );
            return ( m_err >= JET_errSuccess );
        }

        BOOL FEnterFromInitCallback( INST * const pinst )
        {
            m_pinst = pinst;
            if ( !Ptls()->fInCallback )
            {
                SetErr( ErrERRCheck( JET_errInvalidParameter ) );
            }
            else
            {
                SetErr( pinst->ErrAPIEnterForInit() );
            }

            return ( m_err >= JET_errSuccess );
        }

        BOOL FEnterFromTermCallback( INST * const pinst )
        {
            m_pinst = pinst;
            if ( !Ptls()->fInCallback )
            {
                SetErr( ErrERRCheck( JET_errInvalidParameter ) );
            }
            else
            {
                SetErr( pinst->ErrAPIEnterFromTerm() );
            }

            return ( m_err >= JET_errSuccess );
        }

        BOOL FEnterWithoutInit( INST * const pinst, const BOOL fAllowInitInProgress = fFalse )
        {
            m_pinst = pinst;
            SetErr( pinst->ErrAPIEnterWithoutInit( fAllowInitInProgress ) );
            return ( m_err >= JET_errSuccess );
        }

        VOID LeaveAfterCall( const ERR err )
        {
            m_pinst->APILeave();
            SetErr( err );
        }

        VOID LeaveAfterTerm( const ERR err )
        {
            LeaveAfterCall( err );
            m_pinst = NULL;
        }
};

class APICALL_SESID : public APICALL
{
    private:
        PIB*                            m_ppib;
        const UserTraceContext* const   m_putcOuter;

    public:
        APICALL_SESID( const INT op ) :
            APICALL( op ),
            m_ppib( NULL ),
            m_putcOuter( PutcTLSGetUserContext() )
            {}
        ~APICALL_SESID()                                    {}

        PIB * Ppib()                                        { return m_ppib; }

        ERR ErrResult() const
        {
            AssertRTL( m_err > -65536 && m_err < 65536 );
            if ( m_err < JET_errSuccess )
            {
                const WCHAR* wszInstDisplayName = ( m_ppib != NULL && PinstFromPpib( m_ppib )->m_wszDisplayName != NULL ? PinstFromPpib( m_ppib )->m_wszDisplayName : L"_unknown_" );
                OSDiagTrackJetApiError( wszInstDisplayName, m_op, m_err );
            }
            return m_err;
        }

        BOOL FEnter(
            const JET_SESID     sesid,
            const BOOL          fIgnoreStopService = fFalse )
        {
            if ( g_cTermsInProgress >= g_cpinstInit )
            {
                SetErr( ErrERRCheck( 0 == g_cpinstInit ? JET_errNotInitialized : JET_errTermInProgress ) );
                return fFalse;
            }

            const INST * const  pinst   = PinstFromSesid( sesid );

            if ( pinst->m_fStopJetService && !fIgnoreStopService )
            {
                SetErr( ErrERRCheck( JET_errClientRequestToStopJetService ) );
            }
            else if ( pinst->FInstanceUnavailable() )
            {
                SetErr( pinst->ErrInstanceUnavailableErrorCode() );
            }
            else if ( pinst->m_fTermInProgress )
            {
                SetErr( ErrERRCheck( JET_errTermInProgress ) );
            }
            else
            {
                m_ppib = (PIB *)sesid;
                TLS* ptls = Ptls();

                m_ppib->ptlsApi = ptls;
                Assert( m_ppib->m_cInJetAPI >= 0 );
                if ( !m_ppib->FUserSession() &&
                     !ptls->fInCallback )
                {
                    FireWall( "InvalidSesid" );
                    SetErr( ErrERRCheck( JET_errInvalidSesid ) );
                }
                else if ( 1 != AtomicIncrement( &m_ppib->m_cInJetAPI )
                    && !ptls->fInCallback )
                {
                    PIBReportSessionSharingViolation( m_ppib );
                    FireWall( "SessionSharingViolationEnterApi" );
                    AtomicDecrement( &m_ppib->m_cInJetAPI );
                    SetErr( ErrERRCheck( JET_errSessionSharingViolation ) );
                }

                else if ( pinst->m_fTermInProgress )
                {
                    AtomicDecrement( &m_ppib->m_cInJetAPI );
                    SetErr( ErrERRCheck( JET_errTermInProgress ) );
                }
                else
                {
                    Assert( m_putcOuter == NULL || ptls->fInCallback );
                    m_ppib->SetUserTraceContextInTls();

                    SetErr( JET_errSuccess );
                }

                if ( m_err >= JET_errSuccess )
                {
                    if ( !ptls->fInCallback )
                    {
                        if ( ptls->fInJetAPI )
                        {
                            EnforceSz( fFalse, "No recursion allowed for Jet APIs except from Jet callbacks" );
                            SetErr( ErrERRCheck( JET_errInvalidOperation ) );
                        }

                        ptls->fInJetAPI = fTrue;
                    }
                }
#if ENABLE_API_TRACE
                OSTraceWriteRefLog( g_pJetApiTraceLog, ptls->fInJetAPI, NULL );
#endif
            }

            return ( m_err >= JET_errSuccess );
        }

        BOOL FEnter(
            const JET_SESID     sesid,
            const JET_DBID      dbid,
            const BOOL          fIgnoreStopService = fFalse )
        {
            ERR err = JET_errSuccess;
            if ( !FEnter( sesid, fIgnoreStopService ) )
            {
                Assert( m_err < JET_errSuccess );
                return fFalse;
            }
            if ( !FInRangeIFMP( IFMP(dbid) ) )
            {
                Error( ErrERRCheck( JET_errInvalidDatabaseId ) );
            }

            err = ErrPIBCheckIfmp( ( (PIB *) sesid ), IFMP( dbid ) );

        HandleError:

            if ( err < JET_errSuccess )
            {
                AssertSz( FNegTest( fInvalidAPIUsage ), "User provided an invalid JET_DBID to the JET API." );
                LeaveAfterCall( err );
            }
            else
            {
                Assert( err == JET_errSuccess );
                SetErr( JET_errSuccess );
            }

            return ( m_err >= JET_errSuccess );
        }

        __forceinline VOID LeaveAfterCall( const ERR err )
        {
            TLS *ptls = m_ppib->ptlsApi ? m_ppib->ptlsApi : Ptls();
            if ( !ptls->fInCallback )
            {
                Assert( ptls->fInJetAPI );
                ptls->fInJetAPI = fFalse;
            }
#if ENABLE_API_TRACE
            OSTraceWriteRefLog( g_pJetApiTraceLog, ptls->fInJetAPI, NULL );
#endif

            Assert( m_ppib->m_cInJetAPI > 0 );
            LONG cInJetAPI = AtomicDecrement( &m_ppib->m_cInJetAPI );
            TLSSetUserTraceContext( m_putcOuter );
            SetErr( err );

            if ( cInJetAPI == 0 )
            {
                m_ppib->ptlsApi = NULL;
            }
        }
        VOID LeaveAfterEndSession( const ERR err )
        {
            TLS *ptls = Ptls();
            if ( !ptls->fInCallback )
            {
                Assert( ptls->fInJetAPI );
                ptls->fInJetAPI = fFalse;
            }
#if ENABLE_API_TRACE
            OSTraceWriteRefLog( g_pJetApiTraceLog, ptls->fInJetAPI, NULL );
#endif

            if ( err >= JET_errSuccess )
            {
                m_ppib = NULL;
                Assert( PutcTLSGetUserContext() == NULL );
            }
            else
            {
                Assert( m_ppib->m_cInJetAPI > 0 );
                LONG cInJetAPI = AtomicDecrement( &m_ppib->m_cInJetAPI );
                if ( cInJetAPI == 0 )
                {
                    m_ppib->ptlsApi = NULL;
                }
            }

            TLSSetUserTraceContext( m_putcOuter );
            SetErr( err );
        }
};


INST::PLS* INST::Ppls()
{
    return &m_rgpls[ OSSyncGetCurrentProcessor() ];
}

INST::PLS* INST::Ppls( const size_t iProc )
{
    return iProc < m_cpls ? &m_rgpls[ iProc ] : NULL;
}

ERR INST::ErrAPIAbandonEnter_( const LONG lOld )
{
    ERR     err;

    AtomicExchangeAdd( &m_cSessionInJetAPI, -1 );

    if ( lOld & fAPITerminating )
    {
        err = ErrERRCheck( JET_errTermInProgress );
    }
    else if ( lOld & fAPIRestoring )
    {
        err = ErrERRCheck( JET_errRestoreInProgress );
    }
    else
    {
        err = ErrERRCheck( JET_errNotInitialized );
    }

    return err;
}

ERR INST::ErrAPIEnter( const BOOL fTerminating )
{
    if ( !fTerminating )
    {
        if ( m_fStopJetService )
        {
            return ErrERRCheck( JET_errClientRequestToStopJetService );
        }
        else if ( FInstanceUnavailable() )
        {
            return ErrInstanceUnavailableErrorCode();
        }
    }

    const LONG  lOld    = AtomicExchangeAdd( &m_cSessionInJetAPI, 1 );

    if ( ( lOld & maskAPILocked ) && !( lOld & fAPICheckpointing ) )
    {
        return ErrAPIAbandonEnter_( lOld );
    }

    if ( !m_fJetInitialized &&
         ( !FRecovering() || !m_fAllowAPICallDuringRecovery  ) )
    {
        return ErrAPIAbandonEnter_( lOld );
    }

    Assert( lOld < 0x0FFFFFFF );
    Assert( m_fJetInitialized || m_fAllowAPICallDuringRecovery );
    return JET_errSuccess;
}

ERR INST::ErrAPIEnterFromTerm()
{
    const LONG  lOld    = AtomicExchangeAdd( &m_cSessionInJetAPI, 1 );

    if ( ( ( lOld & maskAPILocked )
            && !( lOld & fAPICheckpointing )
            && !( lOld & fAPITerminating ) )
        ||  !m_fJetInitialized )
    {
        return ErrAPIAbandonEnter_( lOld );
    }

    Assert( m_fJetInitialized );
    return JET_errSuccess;
}
ERR INST::ErrAPIEnterForInit()
{
    ERR     err;

    if ( m_fJetInitialized )
    {
        err = ErrERRCheck( JET_errAlreadyInitialized );
    }
    else
    {
        LONG lT = AtomicExchangeAdd( &m_cSessionInJetAPI, 1 );
        Assert( lT >= 0 );
        err = JET_errSuccess;
    }

    return err;
}

ERR INST::ErrAPIEnterWithoutInit( const BOOL fAllowInitInProgress )
{
    ERR     err;
    LONG    lOld    = AtomicExchangeAdd( &m_cSessionInJetAPI, 1 );

    if ( ( lOld & maskAPILocked ) &&
            !( lOld & fAPICheckpointing ) &&
            !( ( lOld & fAPIInitializing ) && fAllowInitInProgress ) )
    {
        AtomicExchangeAdd( &m_cSessionInJetAPI, -1 );
        if ( lOld & fAPITerminating )
        {
            err = ErrERRCheck( JET_errTermInProgress );
        }
        else if ( lOld & fAPIRestoring )
        {
            err = ErrERRCheck( JET_errRestoreInProgress );
        }
        else
        {
            Assert( lOld & fAPIInitializing );
            err = ErrERRCheck( JET_errInitInProgress );
        }
    }
    else
    {
        Assert( ( lOld < 0x0FFFFFFF ) ||
                ( fAllowInitInProgress &&
                    ( lOld & fAPIInitializing ) &&
                    ( ( lOld & ~fAPIInitializing ) <= 1 ) &&
                    Ptls()->fInSoftStart &&
                    Ptls()->fInCallback ) );
        Assert( ( lOld < 0x0FFFFFFF ) ||
                ( fAllowInitInProgress && ( lOld & fAPIInitializing ) && ( ( lOld & ~fAPIInitializing ) <= 1 ) ) );
        err = JET_errSuccess;
    }

    return err;
}


INLINE VOID INST::APILeave()
{
    const LONG  lOld    = AtomicExchangeAdd( &m_cSessionInJetAPI, -1 );
    Assert( lOld >= 1 );

    UtilAssertNotInAnyCriticalSection( );
}

BOOL INST::APILock( const LONG fAPIAction )
{
    Assert( fAPIAction & maskAPILocked );
    Assert( !(fAPIAction & maskAPISessionCount) );

    ULONG lOld = AtomicExchangeSet( (ULONG*) &m_cSessionInJetAPI, fAPIAction );


    if( fAPICheckpointing == fAPIAction )
    {
        if( lOld & fAPICheckpointing )
        {
            return fFalse;
        }
        else if( lOld & maskAPILocked )
        {
            Assert( !(lOld & fAPICheckpointing ) );
            AtomicExchangeReset( (ULONG*) &m_cSessionInJetAPI, fAPIAction );
            return fFalse;
        }
    }
    else
    {
#ifdef DEBUG
        ULONG   cWaitForAllSessionsToLeaveJet = 0;
#endif
        Assert( !(fAPIAction & fAPICheckpointing) );
        while ( ( lOld & maskAPISessionCount ) > 1 || ( lOld & fAPICheckpointing ) )
        {
            AssertSz( ++cWaitForAllSessionsToLeaveJet < 10000,
                "The process has likely hung while attempting to terminate ESE.\nA thread was probably killed by the process while still in ESE." );
            UtilSleep( cmsecWaitGeneric );

            lOld = m_cSessionInJetAPI;
            Assert( lOld & fAPIAction );
        }
    }

    Assert( m_cSessionInJetAPI & fAPIAction );

    return fTrue;
}

VOID INST::APIUnlock( const LONG fAPIAction )
{
    const LONG  lOld    = AtomicExchangeReset( (ULONG*) &m_cSessionInJetAPI, fAPIAction );
    if( fAPICheckpointing == fAPIAction )
    {
        Assert( lOld & fAPIAction );
    }

#ifdef DEBUG
{
    const LONG cSessionInJetAPI = m_cSessionInJetAPI;
    Assert(     !( cSessionInJetAPI & maskAPILocked )
            ||  ( cSessionInJetAPI & fAPICheckpointing )
            ||  ( fAPIAction & fAPICheckpointing ) );
}
#endif
}

VOID INST::EnterCritInst()  { g_critInst.Enter(); }
VOID INST::LeaveCritInst()
{
    Assert( ( runInstModeNoSet == g_runInstMode && 2 > g_cpinstInit ) ||
            ( runInstModeOneInst == g_runInstMode && 2 > g_cpinstInit ) ||
            ( runInstModeMultiInst == g_runInstMode ) );
    g_critInst.Leave();
}

#ifdef DEBUG
BOOL INST::FOwnerCritInst() { return g_critInst.FOwner(); }
#endif

ERR INST::ErrINSTSystemInit()
{
    ERR err = JET_errSuccess;

#ifdef PROFILE_JET_API
    Assert( NULL == profile_pFile );

    if ( 0 == profile_szFileName[0] )
    {
        profile_detailLevel = 0;
    }
    if ( profile_detailLevel > 0 )
    {
        if( 0 == _wfopen_s( &profile_pFile, profile_szFileName, L"at" ) )
        {
            DATETIME datetime;
            profile_csecTimeOffset = HrtHRTCount();
            UtilGetCurrentDateTime( &datetime );
            profile_csecTimeOffset =
                (HRT)(datetime.hour*3600 + datetime.minute*60 + datetime.second)
                - profile_csecTimeOffset/HrtHRTFreq();
            fprintf(
                profile_pFile,
                "\n%02d:%02d:%02d [BEGIN %ws]\n",
                datetime.hour,
                datetime.minute,
                datetime.second,
                WszUtilProcessName() );
        }
    }
#endif


    Assert( INST::FOwnerCritInst() );
    Assert( 0 == g_cpinstInit );
    Assert( g_rgpinst == NULL );


    Alloc( g_rgpinst = new INST*[g_cpinstMax] );
    memset( g_rgpinst, 0, sizeof(INST*) * g_cpinstMax );
    Assert( g_rgpinst[0] == pinstNil );

#if ENABLE_API_TRACE
    Call( ErrOSTraceCreateRefLog( 1000, 0, &g_pJetApiTraceLog ) );
#endif


    if ( !g_critpoolPinstAPI.FInit( g_cpinstMax, rankAPI, szAPI ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

#ifdef SYSTEM_PIB_RFS
    g_cSystemPibAlloc       = 0;
    g_cSystemPibAllocFail   = 0;

    WCHAR wszT[64];
    if( FOSConfigGet( L"DEBUG", L"System PIB Failures", wszT, sizeof( wszT ) ) )
    {
        g_cSystemPibAllocFail = _wtoi( wszT );

        if( g_cSystemPibAllocFail )
        {
            WCHAR wszMessage[128];
            const WCHAR *rgpszT[1] = { wszMessage };

            OSStrCbFormatW( wszMessage,
                        sizeof( wszMessage ),
                        L"System PIB RFS enabled through the registry. System PIB allocation will fail every %d attempts",
                        g_cSystemPibAllocFail );

            UtilReportEvent( eventWarning, GENERAL_CATEGORY, PLAIN_TEXT_ID, 1, rgpszT );
        }
    }
#endif


HandleError:
    if ( JET_errSuccess > err )
    {
        INSTSystemTerm();
    }

    return err;
}


VOID INST::INSTSystemTerm()
{
    Assert( 0 == g_cpinstInit );

    g_critpoolPinstAPI.Term();


    Assert( INST::FOwnerCritInst() );

#if ENABLE_API_TRACE
    if ( g_pJetApiTraceLog != NULL )
    {
        OSTraceDestroyRefLog( g_pJetApiTraceLog );
        g_pJetApiTraceLog = NULL;
    }
#endif

    delete [] g_rgpinst;
    g_rgpinst = NULL;

#ifdef PROFILE_JET_API
    if ( NULL != profile_pFile )
    {
        DATETIME datetime;
        UtilGetCurrentDateTime( &datetime );
        fprintf( profile_pFile, "%02d:%02d:%02d [END %ws]\n", datetime.hour, datetime.minute, datetime.second, WszUtilProcessName() );
        fclose( profile_pFile );
        profile_pFile = NULL;
    }
#endif
}



PM_ICF_PROC LInstanceNamesICFLPwszPpb;

extern BOOL g_fSystemInit;

LONG LInstanceNamesICFLPwszPpb( _In_ LONG icf, _Inout_opt_ void* const pvParam1, _Inout_opt_ void* const pvParam2 )
{
    switch ( icf )
    {
        case ICFInit:
            return CPERFESEInstanceObjects( g_cpinstMax );

        case ICFTerm:
            break;

        case ICFData:
        {
            wchar_t** const pwszInstanceNames = (wchar_t**)pvParam1;
            unsigned char** const prgbAggregationIDs = (unsigned char**)pvParam2;

            if ( pwszInstanceNames != NULL && prgbAggregationIDs != NULL )
            {
                if ( g_runInstMode != runInstModeNoSet )
                {
                    g_critInstanceNames.Enter();
                    const LONG cInstancesT = g_cInstances;
                    memcpy( g_wszInstanceNamesOut, g_wszInstanceNames, sizeof( WCHAR ) * g_cchInstanceNames );
                    g_critInstanceNames.Leave();
                    *pwszInstanceNames = g_wszInstanceNamesOut;

                    *prgbAggregationIDs = g_rgbInstanceAggregationIDs;

                    return cInstancesT;
                }
                else
                {
                    *pwszInstanceNames = L"\0";
                    *prgbAggregationIDs = g_rgbInstanceAggregationIDs;

                    return 0;
                }
            }
        }
            break;

        default:
            AssertSz( fFalse, "Unknown icf %d", icf );
            break;
    }

    return 0;
}


PM_ICF_PROC LDatabaseNamesICFLPwszPpb;

LONG LDatabaseNamesICFLPwszPpb( _In_ LONG icf, _Inout_opt_ void* const pvParam1, _Inout_opt_ void* const pvParam2 )
{
    switch ( icf )
    {
        case ICFInit:
            return g_ifmpMax;

        case ICFTerm:
            break;

        case ICFData:
        {
            wchar_t** const pwszDatabaseNames = (wchar_t**)pvParam1;
            unsigned char** const prgbAggregationIDs = (unsigned char**)pvParam2;

            if ( pwszDatabaseNames != NULL && prgbAggregationIDs != NULL )
            {
                if ( g_runInstMode != runInstModeNoSet )
                {
                    g_critDatabaseNames.Enter();
                    const LONG cDatabasesT = CPERFDatabaseObjects( g_cDatabases );
                    memcpy( g_wszDatabaseNamesOut, g_wszDatabaseNames, sizeof( WCHAR ) * g_cchDatabaseNames );
                    g_critDatabaseNames.Leave();
                    *pwszDatabaseNames = g_wszDatabaseNamesOut;

                    *prgbAggregationIDs = g_rgbDatabaseAggregationIDs;

                    return cDatabasesT;
                }
                else
                {
                    *pwszDatabaseNames = L"\0";
                    *prgbAggregationIDs = g_rgbDatabaseAggregationIDs;

                    return 0;
                }
            }
        }
            break;

        default:
            AssertSz( fFalse, "Unknown icf %d", icf );
            break;
    }

    return 0;
}

CCriticalSection critDBGPrint( CLockBasicInfo( CSyncBasicInfo( szDBGPrint ), rankDBGPrint, 0 ) );

VOID JET_API DBGFPrintF( __in PCSTR sz )
{
    WCHAR wszJetTxt[ 8 ];

    OSStrCbFormatW( wszJetTxt, sizeof(wszJetTxt), L"%ws.txt", SzParam( JET_paramBaseName ) );

    critDBGPrint.Enter();
    FILE* f = NULL;
    (void) _wfopen_s( &f, wszJetTxt, L"a+" );
    if ( f != NULL )
    {
        fprintf( f, "%s", sz );
        fflush( f );
        fclose( f );
    }
    critDBGPrint.Leave();
}


ERR ErrCheckUniquePath( INST *pinst )
{
    ERR                 err                         = JET_errSuccess;
    ERR                 errFullPath                 = JET_errSuccess;
    const BOOL          fTempDbForThisInst          = ( UlParam( pinst, JET_paramMaxTemporaryTables ) > 0 && !pinst->FRecovering() );
    const BOOL          fRecoveryForThisInst        = !pinst->FComputeLogDisabled();
    IFileSystemAPI *    pfsapi                      = NULL;
    ULONG               ipinstChecked               = 0;
    ULONG               ipinst;
    WCHAR               rgwchFullNameNew[IFileSystemAPI::cchPathMax];
    WCHAR               rgwchFullNameExist[IFileSystemAPI::cchPathMax];

    CallR( ErrOSFSCreate( pinst ? pinst->m_pfsconfig : g_pfsconfigGlobal, &pfsapi ) );

    INST::EnterCritInst();

#ifdef DEBUG
    for ( ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
#else
    for ( ipinst = 0; ipinst < g_cpinstMax && ipinstChecked < g_cpinstInit; ipinst++ )
#endif
    {
        if ( !g_rgpinst[ ipinst ] )
        {
            continue;
        }
        ipinstChecked++;
        if ( g_rgpinst[ ipinst ] == pinst )
        {
            continue;
        }
        if ( !g_rgpinst[ ipinst ]->m_fJetInitialized )
        {
            continue;
        }



        const BOOL  fTempDbForCurrInst  = ( UlParam( g_rgpinst[ ipinst ], JET_paramMaxTemporaryTables ) > 0
                                            && !g_rgpinst[ ipinst ]->FRecovering() );
        if ( fTempDbForCurrInst && fTempDbForThisInst )
        {
            Call( pfsapi->ErrPathComplete( SzParam( g_rgpinst[ ipinst ], JET_paramTempPath ), rgwchFullNameExist ) );
            CallS( err );
            if ( JET_errSuccess == ( errFullPath = pfsapi->ErrPathComplete( SzParam( pinst, JET_paramTempPath ), rgwchFullNameNew ) ) )
            {
                if ( !UtilCmpFileName( rgwchFullNameExist, rgwchFullNameNew ) )
                {
                    err = ErrERRCheck( JET_errTempPathInUse );
                    break;
                }
            }
            else
            {
                Assert( JET_errInvalidPath == errFullPath );
            }
        }

        if ( !g_rgpinst[ ipinst ]->FComputeLogDisabled()
            && fRecoveryForThisInst )
        {


            Call( pfsapi->ErrPathComplete( SzParam( g_rgpinst[ ipinst ], JET_paramSystemPath ), rgwchFullNameExist ) );
            CallS( err );
            if ( JET_errSuccess == ( errFullPath = pfsapi->ErrPathComplete( SzParam( pinst, JET_paramSystemPath ), rgwchFullNameNew ) ) )
            {
                if ( !UtilCmpFileName( rgwchFullNameExist, rgwchFullNameNew ) )
                {
                    err = ErrERRCheck( JET_errSystemPathInUse );
                    break;
                }
            }
            else
            {
                Assert( JET_errInvalidPath == errFullPath );
            }

            Call( pfsapi->ErrPathComplete( SzParam( g_rgpinst[ ipinst ], JET_paramLogFilePath ), rgwchFullNameExist ) );
            CallS( err );
            if ( JET_errSuccess == ( errFullPath = pfsapi->ErrPathComplete( SzParam( pinst, JET_paramLogFilePath ), rgwchFullNameNew ) ) )
            {
                if ( !UtilCmpFileName( rgwchFullNameExist, rgwchFullNameNew ) )
                {
                    err = ErrERRCheck( JET_errLogFilePathInUse );
                    break;
                }
            }
            else
            {
                Assert( JET_errInvalidPath == errFullPath );
            }
        }
    }

HandleError:

    INST::LeaveCritInst();

    delete pfsapi;
    return err;
}

BOOL FUtilFileOnlyName( PCWSTR wszFileName)
{
    WCHAR* wszFound;

    OSStrCharFindW( wszFileName, wchPathDelimiter, &wszFound );
    if ( wszFound )
    {
        return fFalse;
    }

    OSStrCharFindW( wszFileName, L':', &wszFound );
    if ( wszFound )
    {
        return fFalse;
    }

    if ( !LOSStrCompareW( wszFileName, L"." ) )
    {
        return fFalse;
    }

    if ( !LOSStrCompareW( wszFileName, L".." ) )
    {
        return fFalse;
    }

    return fTrue;
}






LOCAL JET_ERR ErrInit(  INST        *pinst,
                        BOOL        fSkipIsamInit,
                        JET_GRBIT   grbit )
{
    JET_ERR     err     = JET_errSuccess;
    WCHAR       wszT[IFileSystemAPI::cchPathMax];

    Assert( !pinst->m_fJetInitialized );
    Assert( !pinst->m_fTermInProgress );

    pinst->m_ftInit = UtilGetCurrentFileTime();

    Param( pinst, JET_paramSystemPath )->Set( pinst, ppibNil, 0, SzParam( pinst, JET_paramSystemPath ) );

    if ( !pinst->m_pfsapi->FPathIsRelative( SzParam( pinst, JET_paramTempPath ) ) )
    {
        Param( pinst, JET_paramTempPath )->Set( pinst, ppibNil, 0, SzParam( pinst, JET_paramTempPath ) );
    }
    else
    {
        Call( pinst->m_pfsapi->ErrPathBuild( SzParam( pinst, JET_paramSystemPath ), SzParam( pinst, JET_paramTempPath ), NULL, wszT ) );
        Call( Param( pinst, JET_paramTempPath )->Set( pinst, ppibNil, 0, wszT ) );
    }

    if ( FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramTempPath ) ) )
    {
        Call( pinst->m_pfsapi->ErrPathBuild( SzParam( pinst, JET_paramTempPath ), wszDefaultTempDbFileName, wszDefaultTempDbExt, wszT ) );
        Call( Param( pinst, JET_paramTempPath )->Set( pinst, ppibNil, 0, wszT ) );
    }

    Param( pinst, JET_paramLogFilePath )->Set( pinst, ppibNil, 0, SzParam( pinst, JET_paramLogFilePath ) );

    Call( ErrCheckUniquePath( pinst ) );

    DWORD_PTR cPIBQuota;
    cPIBQuota = UlParam( pinst, JET_paramMaxSessions ) + cpibSystem;
    if ( cPIBQuota < CQuota::QUOTA_MAX )
    {
        Call( ErrRESSetResourceParam( pinst, JET_residPIB, JET_resoperMaxUse, cPIBQuota ) );
    }


    DWORD_PTR cFCBQuota;
    cFCBQuota  = 2 * UlParam( pinst, JET_paramMaxOpenTables ) + UlParam( pinst, JET_paramMaxTemporaryTables );
    cFCBQuota += 2 * UlParam( pinst, JET_paramCachedClosedTables );

    if ( cFCBQuota < CQuota::QUOTA_MAX )
    {
        Call( ErrRESSetResourceParam( pinst, JET_residFCB, JET_resoperMaxUse, cFCBQuota ) );
        Call( ErrRESSetResourceParam( pinst, JET_residTDB, JET_resoperMaxUse, cFCBQuota ) );
        Call( ErrRESSetResourceParam( pinst, JET_residIDB, JET_resoperMaxUse, cFCBQuota ) );
    }
    Call( ErrRESSetResourceParam( pinst, JET_residSCB, JET_resoperMaxUse, UlParam( pinst, JET_paramMaxTemporaryTables ) ) );
    Call( ErrRESSetResourceParam( pinst, JET_residFUCB, JET_resoperMaxUse, UlParam( pinst, JET_paramMaxCursors ) ) );

    if ( pinst->m_pver != NULL )
    {
        VER::VERFree( pinst->m_pver );
        pinst->m_pver = NULL;
    }
    Alloc( pinst->m_pver = VER::VERAlloc( pinst ) );

    DWORD_PTR cVERBUCKETQuota;
    cVERBUCKETQuota = UlParam( pinst, JET_paramMaxVerPages ) + cbucketSystem;
    if ( cVERBUCKETQuota < CQuota::QUOTA_MAX )
    {
        Call( ErrRESSetResourceParam( pinst, JET_residVERBUCKET, JET_resoperMaxUse, cVERBUCKETQuota ) );
    }

    if ( FDefaultParam( pinst, JET_paramCachedClosedTables ) )
    {
        DWORD_PTR cFCBPreferred;
        cFCBPreferred = max( cfcbCachedClosedTablesDefault, DWORD_PTR( 0.15 * cFCBQuota ) );
        Call( Param( pinst, JET_paramCachedClosedTables )->Set( pinst, ppibNil, cFCBPreferred, NULL ) );
    }
    if ( FDefaultParam( pinst, JET_paramPreferredVerPages ) )
    {
        DWORD_PTR cVERBUCKETPreferred;
        cVERBUCKETPreferred = DWORD_PTR( 0.9 * UlParam( pinst, JET_paramMaxVerPages ) );
        Call( Param( pinst, JET_paramPreferredVerPages )->Set( pinst, ppibNil, cVERBUCKETPreferred, NULL ) );
    }

    if ( PvParam( pinst, JET_paramRuntimeCallback ) )
    {
        Call( ( *(JET_CALLBACK)PvParam( pinst, JET_paramRuntimeCallback ) )(
                        JET_sesidNil,
                        JET_dbidNil,
                        JET_tableidNil,
                        JET_cbtypNull,
                        NULL,
                        NULL,
                        NULL,
                        NULL ) );
    }

    if ( UlParam( pinst, JET_paramEnableHaPublish ) )
    {
        Call( FUtilHaPublishInit() ? JET_errSuccess : ErrERRCheck( JET_errCallbackNotResolved ) );

        if ( 2 == UlParam( pinst, JET_paramEnableHaPublish ) )
        {
#ifdef USE_HAPUBLISH_API
            const WCHAR* rgwsz[] = { L"Ha Publishing started for this database.", };
#endif

            OSUHAPublishEvent(
                HaDbFailureTagNoOp, pinst, HA_GENERAL_CATEGORY,
                HaDbIoErrorNone, NULL, 0, 0,
                HA_PLAIN_TEXT_ID, _countof( rgwsz ), rgwsz );
        }
    }

    pinst->m_isdlInit.Trigger( eInitVariableInitDone );

    if ( !fSkipIsamInit )
    {
        Call( ErrIsamInit( JET_INSTANCE( pinst ), grbit ) );
    }

    return err;

HandleError:
    Assert( err < JET_errSuccess );
    return err;
}

LOCAL_BROKEN ERR ErrAPICheckInstInit( const INST * const pinst )
{
    Assert( pinstNil != pinst );
    return ( pinst->m_fJetInitialized ?
                    ErrERRCheck( JET_errAlreadyInitialized ) :
                    JET_errSuccess );
}

LOCAL_BROKEN ERR ErrAPICheckSomeInstInit()
{
    return ( FINSTSomeInitialized() ?
                    ErrERRCheck( JET_errAlreadyInitialized ) :
                    JET_errSuccess );
}

#if defined( DEBUG ) || defined( ESENT )
#define JET_VALIDATE
#else
#endif


#ifdef JET_VALIDATE

#define JET_VALIDATE_RES( resid, pv, err )                                          \
{                                                                                   \
    BOOL fValid = fFalse;                                                           \
                                                                                    \
    TRY                                                                             \
    {                                                                               \
        fValid = CResource::FCallingProgramPassedValidJetHandle( (resid), (const void* const)(pv) );                \
    }                                                                               \
    EXCEPT( efaExecuteHandler )                                                     \
    {                                                                               \
        AssertPREFIX( !fValid );                                                    \
    }                                                                               \
    ENDEXCEPT                                                                       \
                                                                                    \
    if ( !fValid )                                                                  \
    {                                                                               \
        return ErrERRCheck( err );                                                  \
    }                                                                               \
}

#define JET_VALIDATE_INSTANCE( instance )                                           \
{                                                                                   \
           \
    if ( (instance) && (instance) != JET_instanceNil )                              \
    {                                                                               \
        JET_VALIDATE_RES( JET_residINST, (instance), JET_errInvalidInstance );      \
    }                                                                               \
}
#define JET_VALIDATE_SESID( sesid )                                                 \
{                                                                                   \
    JET_VALIDATE_RES( JET_residPIB, (sesid), JET_errInvalidSesid );                 \
}
#define JET_VALIDATE_TABLEID( tableid )                                             \
{                                                                                   \
    JET_VALIDATE_RES( JET_residFUCB, (tableid), JET_errInvalidTableId );            \
}

enum eGrbitValid {
    bitValidSesid           = 1,
    bitValidTableId         = 2,
    bitValidSesidTableId    = 4,
};
const JET_ERR c_rgerrValid[] =
{
    JET_errInvalidSesid,
    JET_errInvalidTableId,
    JET_errInvalidSesid,
    JET_errSesidTableIdMismatch,
    JET_errInvalidSesid,
    JET_errInvalidTableId,
    JET_errInvalidSesid,
    JET_errSuccess,
};

#define JET_VALIDATE_SESID_TABLEID( sesid, tableid )                                \
{                                                                                   \
    eGrbitValid grbitValid = eGrbitValid( 0 );                                      \
                                                                                    \
    TRY                                                                             \
    {                                                                               \
        if ( CResource::FCallingProgramPassedValidJetHandle( JET_residPIB, (const void* const)(sesid) ) )       \
        {                                                                           \
            grbitValid = eGrbitValid( grbitValid | bitValidSesid );                 \
                                                                                    \
            if ( CResource::FCallingProgramPassedValidJetHandle( JET_residFUCB, (const void* const)(tableid) ) )    \
            {                                                                       \
                grbitValid = eGrbitValid( grbitValid | bitValidTableId );           \
                                                                                    \
                if ( ((FUCB*)(tableid))->ppib == (PIB*)(sesid) )                    \
                {                                                                   \
                    grbitValid = eGrbitValid( grbitValid | bitValidSesidTableId );  \
                }                                                                   \
            }                                                                       \
        }                                                                           \
    }                                                                               \
    EXCEPT( efaExecuteHandler )                                                     \
    {                                                                               \
        AssertPREFIX( grbitValid != eGrbitValid(    bitValidSesid |                 \
                                                    bitValidTableId |               \
                                                    bitValidSesidTableId ) );       \
    }                                                                               \
    ENDEXCEPT                                                                       \
                                                                                    \
    const JET_ERR errValidate = c_rgerrValid[ grbitValid ];                         \
                                                                                    \
    if ( errValidate < JET_errSuccess )                                             \
    {                                                                               \
        return ErrERRCheck( errValidate );                                          \
    }                                                                               \
}

#else

#define JET_VALIDATE_INSTANCE( instance )
#define JET_VALIDATE_SESID                  JET_VALIDATE_SESID_FAST
#define JET_VALIDATE_TABLEID                JET_VALIDATE_TABLEID_FAST
#define JET_VALIDATE_SESID_TABLEID          JET_VALIDATE_SESID_TABLEID_FAST

#endif


#define JET_VALIDATE_SESID_FAST( sesid )                                            \
{                                                                                   \
    if ( (sesid) == 0 || (sesid) == JET_sesidNil )                                  \
    {                                                                               \
        return ErrERRCheck( JET_errInvalidSesid );                                  \
    }                                                                               \
    Assert( CResource::FCallingProgramPassedValidJetHandle( JET_residPIB, (const void* const)(sesid) ) );   \
}

#define JET_VALIDATE_TABLEID_FAST( tableid )                                        \
{                                                                                   \
    if ( (tableid) == 0 || (tableid) == JET_tableidNil )                            \
    {                                                                               \
        return ErrERRCheck( JET_errInvalidTableId );                                \
    }                                                                               \
    Assert( CResource::FCallingProgramPassedValidJetHandle( JET_residFUCB, (const void* const)(tableid) ) );    \
}

#define JET_VALIDATE_SESID_TABLEID_FAST( sesid, tableid )                           \
{                                                                                   \
    JET_VALIDATE_SESID_FAST( sesid );                                               \
    JET_VALIDATE_TABLEID_FAST( tableid );                                           \
                                                                                    \
    if ( ((FUCB*)(tableid))->ppib != (PIB*)(sesid) )                                \
    {                                                                               \
        return ErrERRCheck( JET_errSesidTableIdMismatch );                          \
    }                                                                               \
}

ERR ErrJetValidateInstance( JET_INSTANCE instance )
{
    JET_VALIDATE_INSTANCE( instance );
    return JET_errSuccess;
}

ERR ErrJetValidateSession( JET_SESID sesid )
{
    JET_VALIDATE_SESID( sesid );
    return JET_errSuccess;
}



JetParam::
~JetParam()
{
    if ( m_fFreeValue )
    {
        delete [] (void*)m_valueCurrent;
    }
}

ULONG   g_paramidTrapParamSet = 0xFFFFFFFF;

#ifdef DEBUG
#define CheckParamTrap( paramid, szExtraneousFriendlyString )  \
    if ( ( g_paramidTrapParamSet != 0xFFFFFFFF ) &&     \
         ( g_paramidTrapParamSet == paramid ) ) \
    {   \
        Assert( !__FUNCTION__ " hit user specified g_paramidTrapParamSet." szExtraneousFriendlyString );    \
    }
#else
#define CheckParamTrap( paramid, szExtraneousFriendlyString )
#endif

inline CJetParam::CJetParam()
{
    m_paramid                       = 0xFF;
    m_type                          = typeNull;
    m_fCustomGetSet                 = fFalse;
    m_fCustomStorage                = fFalse;
    m_fAdvanced                     = fTrue;
    m_fGlobal                       = fTrue;
    m_fMayNotWriteAfterGlobalInit   = fTrue;
    m_fMayNotWriteAfterInstanceInit = fTrue;
    m_fWritten                      = fFalse;
    m_fFreeValue                    = fFalse;
    m_rangeLow                      = 0;
    m_rangeHigh                     = ULONG_PTR( ~0 );
    m_pfnGet                        = IllegalGet;
    m_pfnSet                        = IllegalSet;
    memset( &m_valueDefault, 0, sizeof( m_valueDefault ) );
    m_valueCurrent                  = 0;
}

ERR CJetParam::Get(
        INST* const         pinst,
        PIB* const          ppib,
        ULONG_PTR* const    pulParam,
        __out_bcount(cbParamMax) WCHAR* const       wszParam,
        const size_t        cbParamMax ) const
{
    return m_pfnGet( this, pinst, ppib, pulParam, wszParam, cbParamMax );
}

ERR CJetParam::Set(
        INST* const         pinst,
        PIB* const          ppib,
        const ULONG_PTR     ulParam,
        __in_opt PCWSTR     wszParam )
{

    CheckParamTrap( m_paramid, "(value to be set ulParam|wszParam" );

    return m_pfnSet( this, pinst, ppib, ulParam, wszParam );
}

ERR CJetParam::Reset(
        INST* const         pinst,
        const ULONG_PTR     ulpValue )
{
    if ( m_fMayNotWriteAfterGlobalInit && RUNINSTGetMode() != runInstModeNoSet )
    {
        return ErrERRCheck( JET_errAlreadyInitialized );
    }
    if ( m_fMayNotWriteAfterInstanceInit && pinst != pinstNil && pinst->m_fSTInit == fSTInitDone )
    {
        return ErrERRCheck( JET_errAlreadyInitialized );
    }

    CheckParamTrap( m_paramid, "(value to be set to ulpValue)." );

    if ( m_fFreeValue )
    {
        delete [] (void*)m_valueCurrent;
    }

    m_fFreeValue    = fFalse;

    const BOOL fUseSet = ( m_paramid != JET_paramConfiguration ) &&
                            m_fCustomGetSet &&
                            m_pfnSet != CJetParam::IllegalSet;

    if ( fUseSet )
    {
        CallS( Set( pinst, ppibNil, ulpValue, (PCWSTR)ulpValue ) );
    }
    else
    {
        m_valueCurrent  = ulpValue;
    }
    m_fWritten      = fFalse;

    return JET_errSuccess;
}

ERR CJetParam::Clone(
        INST* const         pinstSrc,
        PIB* const          ppibSrc,
        CJetParam* const    pjetparamDst,
        INST* const         pinstDst,
        PIB* const          ppibDst )
{
    CheckParamTrap( m_paramid, " (value to be set jetparam.m_valueCurrent)." );

    return m_pfnClone( this, pinstSrc, ppibSrc, pjetparamDst, pinstDst, ppibDst );
}

ERR CJetParam::IgnoreGet(
            const CJetParam* const  pjetparam,
            INST* const             pinst,
            PIB* const              ppib,
            ULONG_PTR* const        pulParam,
            __out_bcount(cbParamMax) WCHAR* const           wszParam,
            const size_t            cbParamMax )
{
    return JET_errSuccess;
}

ERR CJetParam::IllegalGet(
            const CJetParam* const  pjetparam,
            INST* const             pinst,
            PIB* const              ppib,
            ULONG_PTR* const        pulParam,
            __out_bcount(cbParamMax) WCHAR* const           wszParam,
            const size_t            cbParamMax )
{
    return ErrERRCheck( JET_errInvalidParameter );
}

ERR CJetParam::IgnoreSet(
            CJetParam* const    pjetparam,
            INST* const         pinst,
            PIB* const          ppib,
            const ULONG_PTR     ulParam,
            PCWSTR          wszParam )
{
    return JET_errSuccess;
}

ERR CJetParam::IllegalSet(
            CJetParam* const    pjetparam,
            INST* const         pinst,
            PIB* const          ppib,
            const ULONG_PTR     ulParam,
            PCWSTR          wszParam )
{
    return ErrERRCheck( JET_errInvalidParameter );
}

ERR CJetParam::GetInteger(
            const CJetParam* const  pjetparam,
            INST* const             pinst,
            PIB* const              ppib,
            ULONG_PTR* const        pulParam,
            __out_bcount(cbParamMax) WCHAR* const           wszParam,
            const size_t            cbParamMax )
{
    if ( !pulParam )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    *pulParam = pjetparam->m_valueCurrent;

    return JET_errSuccess;
}

ERR CJetParam::GetString(
            const CJetParam* const  pjetparam,
            INST* const             pinst,
            PIB* const              ppib,
            ULONG_PTR* const        pulParam,
            __out_bcount(cbParamMax) WCHAR*  const          wszParam,
            const size_t            cbParamMax )
{
    ERR err;

    if ( !wszParam || cbParamMax < sizeof( char ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    err = ErrOSStrCbCopyW( wszParam, cbParamMax, (WCHAR*)pjetparam->m_valueCurrent );
    if ( err != JET_errBufferTooSmall && err != JET_errSuccess )
    {
        Assert( fFalse );
    }

    return err;
}

ERR CJetParam::ValidateSet(
                CJetParam* const    pjetparam,
                INST* const         pinst,
                PIB* const          ppib,
                const ULONG_PTR     ulParam,
                PCWSTR              wszParam,
                const BOOL          fString )
{
    const ULONG_PTR newParam = fString ? (ULONG_PTR)wszParam : ulParam;
    if ( newParam == pjetparam->m_valueCurrent )
    {
        if ( pjetparam->Type_() == JetParam::typeBoolean ||
             pjetparam->Type_() == JetParam::typeGrbit ||
             pjetparam->Type_() == JetParam::typeInteger ||
             pjetparam->Type_() == JetParam::typeBlockSize )
        {
            Expected( pjetparam->m_valueCurrent >= pjetparam->m_rangeLow );
            Expected( pjetparam->m_valueCurrent <= pjetparam->m_rangeHigh );
        }

        return JET_errSuccess;
    }

    if ( pjetparam->m_fMayNotWriteAfterGlobalInit && RUNINSTGetMode() != runInstModeNoSet )
    {
        return ErrERRCheck( JET_errAlreadyInitialized );
    }
    if ( pjetparam->m_fMayNotWriteAfterInstanceInit && pinst != pinstNil && pinst->m_fSTInit == fSTInitDone )
    {
        return ErrERRCheck( JET_errAlreadyInitialized );
    }
    if ( !fString )
    {
        if ( ulParam < pjetparam->m_rangeLow || ulParam > pjetparam->m_rangeHigh )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
    }
    else
    {
        const size_t cchParam = wszParam ? wcslen( wszParam ) : 0;
        if ( cchParam < pjetparam->m_rangeLow || cchParam > pjetparam->m_rangeHigh )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
    }

    return JET_errSuccess;
}

ERR CJetParam::SetBoolean(
            CJetParam* const    pjetparam,
            INST* const         pinst,
            PIB* const          ppib,
            const ULONG_PTR     ulParam,
            PCWSTR          wszParam )
{
    ERR err = JET_errSuccess;

    Call( ValidateSet( pjetparam, pinst, ppib, ulParam, wszParam ) );

    pjetparam->m_valueCurrent   = ulParam ? fTrue : fFalse;
    pjetparam->m_fWritten       = fTrue;

HandleError:
    return err;
}

ERR CJetParam::SetGrbit(
            CJetParam* const    pjetparam,
            INST* const         pinst,
            PIB* const          ppib,
            const ULONG_PTR     ulParam,
            PCWSTR          wszParam )
{
    ERR err = JET_errSuccess;

    Call( ValidateSet( pjetparam, pinst, ppib, ulParam, wszParam ) );

    pjetparam->m_valueCurrent   = ulParam;
    pjetparam->m_fWritten       = fTrue;

HandleError:
    return err;
}

ERR CJetParam::SetInteger(
            CJetParam* const    pjetparam,
            INST* const         pinst,
            PIB* const          ppib,
            const ULONG_PTR     ulParam,
            PCWSTR          wszParam )
{
    ERR err = JET_errSuccess;

    Call( ValidateSet( pjetparam, pinst, ppib, ulParam, wszParam ) );

    pjetparam->m_valueCurrent   = ulParam;
    pjetparam->m_fWritten       = fTrue;

HandleError:
    return err;
}

ERR CJetParam::SetString(
            CJetParam* const    pjetparam,
            INST* const         pinst,
            PIB* const          ppib,
            const ULONG_PTR     ulParam,
            PCWSTR          wszParam )
{
    ERR     err         = JET_errSuccess;
    size_t  cchParam    = 0;
    WCHAR*  wszNewValue = NULL;

    Call( ValidateSet( pjetparam, pinst, ppib, ulParam, wszParam, fTrue ) );

    cchParam = wszParam ? wcslen( wszParam ) : 0;
    if ( cchParam > 10000 )
    {
        Assert( fFalse );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    Alloc( wszNewValue = new WCHAR[ cchParam + 1 ] );
    memcpy( wszNewValue, wszParam ? wszParam : L"", ( cchParam + 1 ) * sizeof( WCHAR ) );

    if ( pjetparam->m_fFreeValue )
    {
        delete [] (void*)pjetparam->m_valueCurrent;
    }
    pjetparam->m_valueCurrent   = (ULONG_PTR)wszNewValue;
    pjetparam->m_fWritten       = fTrue;
    pjetparam->m_fFreeValue     = fTrue;

    wszNewValue = NULL;

HandleError:
    delete [] wszNewValue;
    return err;
}

ERR CJetParam::SetPointer(
            CJetParam* const    pjetparam,
            INST* const         pinst,
            PIB* const          ppib,
            const ULONG_PTR     ulParam,
            PCWSTR          wszParam )
{
    ERR err = JET_errSuccess;

    Call( ValidateSet( pjetparam, pinst, ppib, ulParam, wszParam ) );

    pjetparam->m_valueCurrent   = ulParam;
    pjetparam->m_fWritten       = fTrue;

HandleError:
    return err;
}

ERR CJetParam::SetFolder(
            CJetParam* const    pjetparam,
            INST* const         pinst,
            PIB* const          ppib,
            const ULONG_PTR     ulParam,
            PCWSTR              wszParam )
{
    ERR             err         = JET_errSuccess;
    IFileSystemAPI* pfsapi      = NULL;
    WCHAR           rgwchPath[ IFileSystemAPI::cchPathMax ];
    size_t          cchPath     = 0;
    WCHAR*          wszNewValue = NULL;

    Call( ValidateSet( pjetparam, pinst, ppib, ulParam, wszParam, fTrue ) );

    Call( ErrOSFSCreate( pinst ? pinst->m_pfsconfig : g_pfsconfigGlobal, &pfsapi ) );
    Call( pfsapi->ErrPathComplete( wszParam, rgwchPath ) );
    Call( pfsapi->ErrPathFolderNorm( rgwchPath, sizeof(rgwchPath) ) );
    cchPath = LOSStrLengthW( rgwchPath );
    if ( cchPath < pjetparam->m_rangeLow || cchPath > pjetparam->m_rangeHigh )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Alloc( wszNewValue = new WCHAR[ cchPath + 1 ] );
    OSStrCbCopyW( wszNewValue, (cchPath+1)*sizeof(WCHAR), rgwchPath );

    if ( pjetparam->m_fFreeValue )
    {
        delete [] (void*)pjetparam->m_valueCurrent;
    }
    pjetparam->m_valueCurrent   = (ULONG_PTR)wszNewValue;
    pjetparam->m_fWritten       = fTrue;
    pjetparam->m_fFreeValue     = fTrue;

    wszNewValue = NULL;

HandleError:
    delete [] wszNewValue;
    delete pfsapi;
    return err;
}

ERR CJetParam::SetPath(
        CJetParam* const    pjetparam,
        INST* const         pinst,
        PIB* const          ppib,
        const ULONG_PTR     ulParam,
        PCWSTR      wszParam )
{
    ERR             err         = JET_errSuccess;
    IFileSystemAPI* pfsapi      = NULL;
    WCHAR           rgwchPath[ IFileSystemAPI::cchPathMax ];
    size_t          cchPath     = 0;
    WCHAR*          wszNewValue = NULL;

    Call( ValidateSet( pjetparam, pinst, ppib, ulParam, wszParam, fTrue ) );

    Call( ErrOSFSCreate( pinst ? pinst->m_pfsconfig : g_pfsconfigGlobal, &pfsapi ) );
    Call( pfsapi->ErrPathComplete( wszParam, rgwchPath ) );
    cchPath = LOSStrLengthW( rgwchPath );
    if ( cchPath < pjetparam->m_rangeLow || cchPath > pjetparam->m_rangeHigh )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( !FOSSTRTrailingPathDelimiterW( wszParam ) &&
            FUtilFileOnlyName( wszParam ) )
    {
        const size_t cchParam = wszParam ? LOSStrLengthW( wszParam ) : 0;
        Alloc( wszNewValue = new WCHAR[ cchParam + 1 ] );
        OSStrCbCopyW( wszNewValue, sizeof(WCHAR) * (cchParam + 1), wszParam ? wszParam : L"" );
    }
    else
    {
        Alloc( wszNewValue = new WCHAR[ cchPath + 1 ] );
        OSStrCbCopyW( wszNewValue, (cchPath+1)*sizeof(WCHAR), rgwchPath );
    }

    if ( pjetparam->m_fFreeValue )
    {
        delete [] (void*)pjetparam->m_valueCurrent;
    }
    pjetparam->m_valueCurrent   = (ULONG_PTR)wszNewValue;
    pjetparam->m_fWritten       = fTrue;
    pjetparam->m_fFreeValue     = fTrue;

    wszNewValue = NULL;

HandleError:
    delete [] wszNewValue;
    delete pfsapi;
    return err;
}

ERR CJetParam::CloneDefault(    CJetParam* const    pjetparamSrc,
                                INST* const         pinstSrc,
                                PIB* const          ppibSrc,
                                CJetParam* const    pjetparamDst,
                                INST* const         pinstDst,
                                PIB* const          ppibDst )
{
    memcpy( pjetparamDst, pjetparamSrc, sizeof( CJetParam ) );

    return JET_errSuccess;
}


ERR CJetParam::CloneString(     CJetParam* const    pjetparamSrc,
                                INST* const         pinstSrc,
                                PIB* const          ppibSrc,
                                CJetParam* const    pjetparamDst,
                                INST* const         pinstDst,
                                PIB* const          ppibDst )
{
    ERR     err         = JET_errSuccess;
    size_t  cchValue    = 0;
    WCHAR*  wszNewValue = NULL;


    if ( pjetparamSrc->m_fFreeValue )
    {
        Assert( pjetparamSrc->m_type == typeString || pjetparamSrc->m_type == typeFolder || pjetparamSrc->m_type == typePath );
        cchValue = wcslen( (WCHAR*)pjetparamSrc->m_valueCurrent );

        Alloc( wszNewValue = new WCHAR[ cchValue + 1 ] );
        memcpy( wszNewValue, (void*)pjetparamSrc->m_valueCurrent, (cchValue + 1)*sizeof(WCHAR) );
        Assert( !pjetparamDst->m_fFreeValue );

        memcpy( pjetparamDst, pjetparamSrc, sizeof( CJetParam ) );
        pjetparamDst->m_valueCurrent = (ULONG_PTR)wszNewValue;

        Assert( pjetparamDst->m_fFreeValue );
        wszNewValue = NULL;
    }
    else
    {
        Assert( !pjetparamDst->m_fFreeValue );
        memcpy( pjetparamDst, pjetparamSrc, sizeof( CJetParam ) );
    }

HandleError:
    delete [] wszNewValue;
    return err;
}

ERR CJetParam::IllegalClone(    CJetParam* const    pjetparamSrc,
                                INST* const         pinstSrc,
                                PIB* const          ppibSrc,
                                CJetParam* const    pjetparamDst,
                                INST* const         pinstDst,
                                PIB* const          ppibDst )
{
    return ErrERRCheck( JET_errInvalidParameter );
}


LOCAL VOID InitVariablesWithPageSize( CJetParam* rgParam )
{
    const ULONG cbPage = (ULONG)rgParam[ JET_paramDatabasePageSize ].m_valueCurrent;
    Assert( 1024 * 4 <= cbPage && cbPage <= 1024 * 32 );
    Assert( !( cbPage & ( cbPage - 1 ) ) );

    TAGFLD::InitStaticMembersWithPageSize( cbPage );

    CJetParam* pParam = &rgParam[ JET_paramLVChunkSizeMost ];
    if( FIsSmallPage( cbPage ) )
    {
        pParam->m_valueCurrent = cbPage - ( LVChunkOverheadCommon + CPAGE::CbPageHeader( cbPage ) );
    }
    else if( 32 * 1024 == cbPage )
    {
        pParam->m_valueCurrent = 8150;
    }
    else
    {
        Assert( 16 * 1024 == cbPage );
        pParam->m_valueCurrent = 4050;
    }
}

ERR CJetParam::SetBlockSize(
                CJetParam* const    pjetparam,
                INST* const         pinst,
                PIB* const          ppib,
                const ULONG_PTR     ulParam,
                PCWSTR              wszParam )
{
    ERR     err     = JET_errSuccess;
    size_t  n;
    size_t  log2n;

    Call( ValidateSet( pjetparam, pinst, ppib, ulParam, wszParam ) );

    for ( n = ulParam, log2n = size_t( -1 ); n; n = n / 2, log2n++ );
    if ( ulParam != ULONG_PTR( 1ull << log2n ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    pjetparam->m_valueCurrent   = ulParam;
    pjetparam->m_fWritten       = fTrue;

HandleError:
    return err;
}

JET_GRBIT g_grbitCommitFlagsMsk = ( JET_bitCommitLazyFlush | JET_bitWaitLastLevel0Commit | JET_bitWaitAllLevel0Commit | JET_bitForceNewLog | JET_bitCommitRedoCallback );
JET_GRBIT g_grbitCommitDefault  = 0;

ERR
GetCommitDefault(   const CJetParam* const  pjetparam,
                    INST* const             pinst,
                    PIB* const              ppib,
                    ULONG_PTR* const        pulParam,
                    __out_bcount(cbParamMax) PWSTR  const           wszParam,
                    const size_t            cbParamMax )
{
    if ( !pulParam )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( pinst != pinstNil )
    {
        if ( ppib != ppibNil )
        {
            ULONG ulActual;
            Expected( cbParamMax >= sizeof(JET_GRBIT) );
            CallS( ErrIsamGetSessionParameter( (JET_SESID)ppib, JET_sesparamCommitDefault, pulParam, sizeof(JET_GRBIT), &ulActual ) );
            Expected( ulActual == sizeof(JET_GRBIT) );
            Assert( *((JET_GRBIT*)pulParam) == (JET_GRBIT)ppib->grbitCommitDefault );
        }
        else
        {
            *((JET_GRBIT*)pulParam) = pinst->m_grbitCommitDefault;
        }
    }
    else
    {
        *((JET_GRBIT*)pulParam) = g_grbitCommitDefault;
    }

    return JET_errSuccess;
}

ERR
SetCommitDefault(   CJetParam* const    pjetparam,
                    INST* const         pinst,
                    PIB* const          ppib,
                    const ULONG_PTR     ulParam,
                    PCWSTR          wszParam )
{
    const JET_GRBIT grbitParam = JET_GRBIT( ulParam );

    if ( grbitParam & ~g_grbitCommitFlagsMsk )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    Assert( 0 == ( grbitParam & ~JET_bitCommitLazyFlush ) );

    if ( pinst != pinstNil )
    {
        if ( ppib != ppibNil )
        {
            CallS( ErrIsamSetSessionParameter( (JET_SESID)ppib, JET_sesparamCommitDefault, (void*)&grbitParam, sizeof(JET_GRBIT) ) );
            Assert( JET_GRBIT( ppib->grbitCommitDefault ) == grbitParam );
        }
        else
        {
            pinst->m_grbitCommitDefault = grbitParam;
        }
    }
    else
    {
        g_grbitCommitDefault = grbitParam;
    }

    return JET_errSuccess;
}

ERR
CloneCommitDefault( CJetParam* const    pjetparamSrc,
                    INST* const         pinstSrc,
                    PIB* const          ppibSrc,
                    CJetParam* const    pjetparamDst,
                    INST* const         pinstDst,
                    PIB* const          ppibDst )
{
    ERR err = JET_errSuccess;
    ULONG_PTR ulParam = 0;

    memcpy( pjetparamDst, pjetparamSrc, sizeof( CJetParam ) );
    Call( GetCommitDefault( pjetparamSrc, pinstSrc, ppibSrc, &ulParam, NULL, 0 ) );
    Call( SetCommitDefault( pjetparamDst, pinstDst, ppibDst, ulParam, NULL ) );

HandleError:
    return err;
}

ERR
GetTransactionLevel(    const CJetParam* const  pjetparam,
                        INST* const             pinst,
                        PIB* const              ppib,
                        ULONG_PTR* const        pulParam,
                        __out_bcount(cbParamMax) PWSTR const            wszParam,
                        const size_t            cbParamMax )
{
    if ( !pulParam || ppibNil == ppib )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    *pulParam = ppib->Level();

    return JET_errSuccess;
}

ERR
GetCacheSize(   const CJetParam* const  pjetparam,
                INST* const             pinst,
                PIB* const              ppib,
                ULONG_PTR* const        pulParam,
                __out_bcount(cbParamMax) PWSTR const            szParam,
                const size_t            cbParamMax )
{
    return ErrBFGetCacheSize( pulParam );
}

ERR
SetCacheSize(   CJetParam* const    pjetparam,
                INST* const         pinst,
                PIB* const          ppib,
                const ULONG_PTR     ulParam,
                PCWSTR      wszParam )
{
    return ErrBFSetCacheSize( ulParam );
}

ERR
SetCacheSizeRange(  CJetParam* const    pjetparam,
                    INST* const         pinst,
                    PIB* const          ppib,
                    const ULONG_PTR     ulParam,
                    PCWSTR      wszParam )
{
    ERR err = JET_errSuccess;

    Call( CJetParam::SetInteger( pjetparam, pinst, ppib, ulParam, wszParam ) );
    Call( ErrBFConsumeSettings( bfcsCacheSize, ifmpNil ) );

HandleError:
    return err;
}

ERR
SetCheckpointDepthMax(  CJetParam* const    pjetparam,
                INST* const         pinst,
                PIB* const          ppib,
                const ULONG_PTR     ulParam,
                PCWSTR      wszParam )
{
    ERR     err = JET_errSuccess;

    Assert( pjetparam->m_paramid == JET_paramCheckpointDepthMax );
    Expected( ppib == ppibNil );

    Call( CJetParam::SetInteger( pjetparam, pinst, ppib, ulParam, wszParam ) );

    Call( ErrIOUpdateCheckpoints( pinst ) );

HandleError:

    return err;
}

ERR
SetDatabasePageSize(    CJetParam* const    pjetparam,
                        INST* const         pinst,
                        PIB* const          ppib,
                        const ULONG_PTR     ulParam,
                        PCWSTR      wszParam )
{
    ERR     err     = JET_errSuccess;

    Call( CJetParam::SetBlockSize( pjetparam, pinst, ppib, ulParam, wszParam ) );
    InitVariablesWithPageSize( &pjetparam[ -JET_paramDatabasePageSize ] );

HandleError:
    return err;
}

ERR
ErrSetConfigStoreSpec(  CJetParam* const    pjetparam,
                INST* const             pinst,
                PIB* const              ppib,
                const ULONG_PTR         ulParam,
                PCWSTR                  wszParam );

C_ASSERT( JET_efvWindows10Rtm == ( JET_efvRollbackInsertSpaceStagedToTest + 60  ) );
C_ASSERT( JET_efvExchange2013Rtm == ( JET_efvScrubLastNodeOnEmptyPage + 60  ) );
C_ASSERT( JET_efvWindows10v2Rtm == JET_efvMsysLocalesGuidRefCountFixup );
C_ASSERT( JET_efvExchange2016Rtm == ( JET_efvLostFlushMapCrashResiliency + 60  ) );
C_ASSERT( JET_efvExchange2016Cu1Rtm == 8920  );

ERR
ErrSetEngineFormatVersionParameter( _In_ CJetParam* const   pjetparam,
                        _In_opt_ INST* const        pinst,
                        _Reserved_ PIB* const   ppib,
                        _In_ const ULONG_PTR    ulParam,
                        _Reserved_ PCWSTR       wszParam )
{
    ERR     err     = JET_errSuccess;

    if( ulParam < EfvMinSupported() )
    {
        err = ErrERRCheck( JET_errEngineFormatVersionNoLongerSupportedTooLow );
        AssertSz( FNegTest( fInvalidAPIUsage ), "Bad param arg: %d is below min supported value of %d, returning %d\n", ulParam, EfvMinSupported(), err );
        return err;
    }

    const BOOL fAllowHigherPersistedFormat = ( ulParam & JET_efvAllowHigherPersistedFormat ) == JET_efvAllowHigherPersistedFormat;
    const ULONG_PTR ulParamSansFlag = ulParam & ~( fAllowHigherPersistedFormat ? JET_efvAllowHigherPersistedFormat : 0 );

    if( ulParamSansFlag > EfvMaxSupported() &&
        ulParam != JET_efvUsePersistedFormat &&
        ulParam != JET_efvUseEngineDefault )
    {
        return ErrERRCheck( JET_errEngineFormatVersionNotYetImplementedTooHigh );
    }

    if ( ulParam & 0x40000000 && ulParamSansFlag < 0x5 )
    {
        AssertSz( fFalse, "Invalid set of flags for 0x%x", ulParam );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( ulParam != JET_efvUsePersistedFormat )
    {
        const FormatVersions * pfmtvers = NULL;
        err = ErrGetDesiredVersion( pinst, (JET_ENGINEFORMATVERSION)ulParam, &pfmtvers, fTrue );
        Call( err );
        Assert( pfmtvers != NULL );
        Assert( ( pfmtvers->efv == (JET_ENGINEFORMATVERSION)ulParamSansFlag ) ||
                ( ( ulParam == JET_efvUseEngineDefault ) && ( pfmtvers->efv == PfmtversEngineDefault()->efv ) ) );
    }

    if( pinst != pinstNil )
    {
        FMP::EnterFMPPoolAsWriter();
        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            if ( pinst->m_mpdbidifmp[ dbid ] >= g_ifmpMax )
            {
                continue;
            }
            const FMP * const pfmp = &g_rgfmp[ pinst->m_mpdbidifmp[ dbid ] ];
            if ( pfmp->FAttached() && !pfmp->FAttachedForRecovery() )
            {
                AssertSz( fFalse, "Someone is trying to set the EFV after a regular attach has been performed!  This is not allowed." );
                err = ErrERRCheck( JET_errAlreadyInitialized );
            }
        }
        FMP::LeaveFMPPoolAsWriter();

    }

    Call( CJetParam::SetInteger( pjetparam, pinst, ppib, ulParam, wszParam ) );

HandleError:

    return err;
}

ERR
ErrGetEngineFormatVersionParameter( _In_ const CJetParam* const pjetparam,
                    _In_opt_ INST* const            pinst,
                    _Reserved_ PIB* const               ppib,
                    _Out_ ULONG_PTR* const      pulParam,
                    _Reserved_ PWSTR const          wszParam,
                    _In_ const size_t           cbParamMax )
{
    if ( NULL == pulParam )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    Assert( pinst != (INST*)JET_instanceNil );

    JET_API_PTR efvT;
    CallS( CJetParam::GetInteger( pjetparam, pinst, ppib, (ULONG_PTR*)&efvT, NULL, sizeof(efvT) ) );
    Enforce( efvT <= (ULONGLONG)ulMax  );
    JET_ENGINEFORMATVERSION efv = (JET_ENGINEFORMATVERSION)efvT;

    if ( efv == JET_efvUseEngineDefault )
    {
        efv = EfvDefault();
    }

    Assert( efv != JET_efvUseEngineDefault );

    *pulParam = efv;

    return JET_errSuccess;
}


ERR
GetErrorToString(   const CJetParam* const  pjetparam,
                    INST* const             pinst,
                    PIB* const              ppib,
                    ULONG_PTR* const        pulParam,
                    __out_bcount(cbParamMax) PWSTR const            wszParam,
                    const size_t            cbParamMax )
{
    if ( NULL == pulParam )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    if ( !wszParam || cbParamMax < sizeof( char ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

#ifdef MINIMAL_FUNCTIONALITY
    return ErrOSStrCbFormatW( wszParam, cbParamMax, L"%i, (0x%08x)", *((JET_ERR *)pulParam), *((JET_ERR *)pulParam) );
#else
    ERR         err;
    CAutoWSZ    lwszErrorValueString;
    CAutoWSZ    lwszErrorDescString;
    const CHAR *        szError     = NULL;
    const CHAR *        szErrorText = NULL;

    JetErrorToString( *((JET_ERR *)pulParam), &szError, &szErrorText );

    CallR( lwszErrorValueString.ErrSet( szError ) );
    CallR( lwszErrorDescString.ErrSet( szErrorText ) );

    return( ErrOSStrCbFormatW( wszParam, cbParamMax, L"%ws, %ws", (WCHAR*)lwszErrorValueString, (WCHAR*)lwszErrorDescString ) );
#endif
}


static BOOL                 g_fUnicodeIndexDefaultSetByUser = fFalse;

static ULONG                g_dwLCMapFlagsDefaultSetByUser;
static WCHAR                g_wszLocaleNameDefaultSetByUser[NORM_LOCALE_NAME_MAX_LENGTH];

ERR
GetUnicodeIndexDefault( const CJetParam* const  pjetparam,
                        INST* const             pinst,
                        PIB* const              ppib,
                        ULONG_PTR* const        pulParam,
                        __out_bcount(cbParamMax) PWSTR const    wszParam,
                        const size_t            cbParamMax )
{
    ERR err = JET_errSuccess;

    if ( NULL == pulParam )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    if ( cbParamMax < sizeof( JET_UNICODEINDEX ) )
    {
        return ErrERRCheck( JET_errBufferTooSmall );
    }

    JET_UNICODEINDEX * pindexUnicode = (JET_UNICODEINDEX *)pulParam;

    if ( pinstNil != pinst )
    {
        pindexUnicode->dwMapFlags = pinst->m_dwLCMapFlagsDefault;
        CallR( ErrNORMLocaleToLcid( pinst->m_wszLocaleNameDefault, &(pindexUnicode->lcid) ) );
    }
    else if ( g_fUnicodeIndexDefaultSetByUser )
    {
        pindexUnicode->dwMapFlags = g_dwLCMapFlagsDefaultSetByUser;
        CallR( ErrNORMLocaleToLcid( g_wszLocaleNameDefaultSetByUser, &(pindexUnicode->lcid) ) );
    }
    else
    {
        pindexUnicode->dwMapFlags = dwLCMapFlagsDefault;
        CallR( ErrNORMLocaleToLcid( wszLocaleNameDefault, &(pindexUnicode->lcid) ) );
    }

    return err;
}

ERR
SetUnicodeIndexDefault( CJetParam* const    pjetparam,
                        INST* const         pinst,
                        PIB* const          ppib,
                        const ULONG_PTR     ulParam,
                        PCWSTR      wszParam )
{
    ERR err = JET_errSuccess;

    JET_UNICODEINDEX * pindexUnicode = (JET_UNICODEINDEX *)ulParam;

    if ( NULL == pindexUnicode )
    {
        if ( pinstNil != pinst )
        {
            pinst->m_dwLCMapFlagsDefault = dwLCMapFlagsDefault;
            CallR( ErrOSStrCbCopyW( pinst->m_wszLocaleNameDefault, sizeof( pinst->m_wszLocaleNameDefault ), wszLocaleNameDefault ) );
        }
        else
        {
            g_fUnicodeIndexDefaultSetByUser = fFalse;
        }
    }
    else
    {
        if ( pinstNil != pinst )
        {
            Call( ErrNORMCheckLCMapFlags( pinst, &(pindexUnicode->dwMapFlags), fFalse ) );
            pinst->m_dwLCMapFlagsDefault = pindexUnicode->dwMapFlags;
            Call( ErrNORMLcidToLocale( pindexUnicode->lcid, pinst->m_wszLocaleNameDefault, _countof( pinst->m_wszLocaleNameDefault ) ) );
            Call( ErrNORMCheckLocaleName( pinst,  pinst->m_wszLocaleNameDefault ) );
        }
        else
        {
            Call( ErrNORMLcidToLocale( pindexUnicode->lcid, g_wszLocaleNameDefaultSetByUser, _countof( g_wszLocaleNameDefaultSetByUser ) ) );
            Call( ErrNORMCheckLocaleName( pinst, g_wszLocaleNameDefaultSetByUser ) );
            Call( ErrNORMCheckLCMapFlags( pinst, &(pindexUnicode->dwMapFlags), fFalse ) );
            g_dwLCMapFlagsDefaultSetByUser = pindexUnicode->dwMapFlags;
            g_fUnicodeIndexDefaultSetByUser = fTrue;
        }
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        err = ErrERRCheck( JET_errInvalidParameter );
    }
    return err;
}

ERR
CloneUnicodeIndexDefault(   CJetParam* const    pjetparamSrc,
                            INST* const         pinstSrc,
                            PIB* const          ppibSrc,
                            CJetParam* const    pjetparamDst,
                            INST* const         pinstDst,
                            PIB* const          ppibDst )
{
    ERR err = JET_errSuccess;
    JET_UNICODEINDEX idxUnicode;

    memcpy( pjetparamDst, pjetparamSrc, sizeof( CJetParam ) );
    Call( GetUnicodeIndexDefault( pjetparamSrc, pinstSrc, ppibSrc, (ULONG_PTR*)&idxUnicode, NULL, sizeof( JET_UNICODEINDEX ) ) );
    Call( SetUnicodeIndexDefault( pjetparamDst, pinstDst, ppibDst, (ULONG_PTR)&idxUnicode, NULL ) );

HandleError:
    return err;
}

ERR
GetRecoveryCurrentLogfile(  const CJetParam* const  pjetparam,
                            INST* const             pinst,
                            PIB* const              ppib,
                            ULONG_PTR* const        pulParam,
                            __out_bcount(cbParamMax) PWSTR const            wszParam,
                            const size_t            cbParamMax )
{
    if ( NULL == pulParam )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( pinst == pinstNil )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    LONG lGenCurrent = pinst->m_plog->LGGetCurrentFileGenNoLock();
    if ( lGenCurrent == 0 )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    *pulParam = lGenCurrent;
    return JET_errSuccess;
}

ERR
SetCachePriority(   _In_ CJetParam* const   pjetparam,
                    _In_opt_ INST* const        pinst,
                    _In_opt_ PIB* const         ppib,
                    _In_ const ULONG_PTR    ulParam,
                    _Reserved_ PCWSTR       wszParam )
{
    ERR err = JET_errSuccess;

    Call( CJetParam::SetInteger( pjetparam, pinst, ppib, ulParam, wszParam ) );
    Assert( FIsCachePriorityValid( ulParam ) );

    if ( pinst == pinstNil )
    {
        goto HandleError;
    }


    pinst->m_critPIB.Enter();

    for ( DBID dbid = 0; dbid < _countof( pinst->m_mpdbidifmp ); dbid++ )
    {
        const IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
        if ( !FMP::FAllocatedFmp( ifmp ) )
        {
            continue;
        }

        const FMP* const pfmp = &g_rgfmp[ ifmp ];
        if ( !pfmp->FInUse() )
        {
            continue;
        }

        for ( PIB* ppibCurr = pinst->m_ppibGlobal; ppibCurr != NULL; ppibCurr = ppibCurr->ppibNext )
        {
            ppibCurr->ResolveCachePriorityForDb( dbid );
        }
    }

    pinst->m_critPIB.Leave();

HandleError:
    return err;
}

ERR
SetShrinkDatabaseParam( _In_ CJetParam* const   pjetparam,
                        _In_opt_ INST* const    pinst,
                        _In_opt_ PIB* const     ppib,
                        _In_ const ULONG_PTR    ulParam,
                        _Reserved_ PCWSTR       wszParam )
{
    ERR err = JET_errSuccess;

    const JET_GRBIT grbitShrink = (JET_GRBIT) ulParam;


    const JET_GRBIT grbitValidMask = ( JET_bitShrinkDatabaseOff | JET_bitShrinkDatabaseOn | JET_bitShrinkDatabaseRealtime | JET_bitShrinkDatabasePeriodically );

    if ( ( grbitShrink & ~grbitValidMask ) != 0 )
    {
        Error( ErrERRCheck ( JET_errInvalidGrbit ) );
    }

    if ( ( ( grbitShrink & JET_bitShrinkDatabaseOn ) == 0 ) && ( grbitShrink != JET_bitShrinkDatabaseOff ) )
    {
        Error( ErrERRCheck ( JET_errInvalidGrbit ) );
    }


    Call( pjetparam->SetGrbit( pjetparam, pinst, ppib, ulParam, wszParam ) );

HandleError:

    return err;
}



CJetParam::Config g_config = JET_configDefault;


struct ConfigSetOverrideValue
{
    ULONG_PTR   ulpValue;
    ULONG       paramid;
    ULONG       grbit;
};

#define coBytesToPages              (0x1)
#define coPctRamToPages             (0x2)
#define coPctCacheMaxToPages        (0x3)
#define coUnitConversionsMsk        (coBytesToPages | coPctRamToPages | coPctCacheMaxToPages)

#define CO( paramid, value, flags )     { value, paramid, flags }



const ConfigSetOverrideValue    g_rgJetConfigRemoveQuotas[] = {
    CO( JET_paramMaxOpenTables,                 2147483647, 0x0 ),
    CO( JET_paramMaxCursors,                    2147483647, 0x0 ),
    CO( JET_paramMaxVerPages,                   2147483647, 0x0 ),
    CO( JET_paramMaxTemporaryTables,            2147483647, 0x0 ),
    CO( JET_paramPreferredVerPages,             2147483647, 0x0 ),
    CO( JET_paramMaxSessions,                   30000,      0x0 ),
};




const ConfigSetOverrideValue    g_rgJetConfigLowMemory[] = {


    CO( JET_paramMaxInstances,                  1,          0x0 ),

    CO( JET_paramLogBuffers,                    40,         0x0 ),

    CO( JET_paramPageHintCacheSize,             32,         0x0 ),
    CO( JET_paramEnableFileCache,               fTrue,      0x0 ),
    CO( JET_paramEnableViewCache,               fTrue,      0x0 ),
    CO( JET_paramCacheSizeMin,                  64*1024,    coBytesToPages ),
    CO( JET_paramCacheSizeMax,                  256*1024,   coBytesToPages ),
    CO( JET_paramStartFlushThreshold,           1,          0x0 ),
    CO( JET_paramStopFlushThreshold,            2,          0x0 ),
    CO( JET_paramLRUKHistoryMax,                10,         0x0 ),

    CO( JET_paramGlobalMinVerPages,             1,          0x0 ),
    CO( JET_paramMaxVerPages,                   64,         0x0 ),
    CO( JET_paramVerPageSize,                   8192,       0x0 ),
    CO( JET_paramHungIOActions,                 JET_bitNil, 0x0 ),

    CO( JET_paramOutstandingIOMax,              16,         0x0 ),


};

const ConfigSetOverrideValue    g_rgJetConfigLowDiskFootprint[] = {
    CO( JET_paramDbExtensionSize,               16,         0x0 ),
    CO( JET_paramPageTempDBMin,                 14,         0x0 ),

    CO( JET_paramCircularLog,                   fTrue,      0x0 ),
    CO( JET_paramLogFileSize,                   64,         0x0 ),
    CO( JET_paramCheckpointDepthMax,            512*1024,   0x0 ),
};

const ConfigSetOverrideValue    g_rgJetConfigDynamicMediumMemory[] = {

    CO( JET_paramMaxInstances,                  1,          0x0 ),

    CO( JET_paramLogBuffers,                    512,        0x0 ),

    CO( JET_paramPageHintCacheSize,             64,         0x0 ),
    CO( JET_paramEnableFileCache,               fTrue,      0x0 ),
    CO( JET_paramEnableViewCache,               fTrue,      0x0 ),
    CO( JET_paramCacheSizeMin,                  128*1024,   coBytesToPages ),
    CO( JET_paramCacheSizeMax,                  5*1024*1024,coBytesToPages ),
    CO( JET_paramStartFlushThreshold,           2,          coPctCacheMaxToPages ),
    CO( JET_paramStopFlushThreshold,            3,          coPctCacheMaxToPages ),
    CO( JET_paramLRUKHistoryMax,                1000,       0x0 ),

    CO( JET_paramGlobalMinVerPages,             1,          0x0 ),
    CO( JET_paramVerPageSize,                   8192,       0x0 ),

    CO( JET_paramOutstandingIOMax,              48,         0x0 ),
};

const ConfigSetOverrideValue    g_rgJetConfigMediumDiskFootprint[] = {
    CO( JET_paramDbExtensionSize,               512*1024,   coBytesToPages ),
    CO( JET_paramPageTempDBMin,                 30,         0x0 ),

    CO( JET_paramCircularLog,                   fTrue,      0x0 ),
    CO( JET_paramLogFileSize,                   512,        0x0 ),
    CO( JET_paramCheckpointDepthMax,            4*1024*1024,0x0 ),
};

const ConfigSetOverrideValue    g_rgJetConfigSSDProfileIO[] = {
    CO( JET_paramMaxCoalesceReadSize,           256*1024,   0x0 ),
    CO( JET_paramMaxCoalesceWriteSize,          256*1024,   0x0 ),
    CO( JET_paramMaxCoalesceWriteGapSize,       0,          0x0 ),
    CO( JET_paramOutstandingIOMax,              16,         0x0 ),
    CO( JET_paramPrereadIOMax,                  5,          0x0 ),
};

const ConfigSetOverrideValue    g_rgJetConfigRunSilent[] = {
    CO( JET_paramNoInformationEvent,            1,          0x0 ),
    CO( JET_paramEventLoggingLevel,             JET_EventLoggingDisable, 0x0 ),
    CO( JET_paramDisablePerfmon,                fTrue,      0x0 ),
};


#undef CO

ERR
GetConfiguration(   const CJetParam* const  pjetparam,
                    INST* const             pinst,
                    PIB* const              ppib,
                    ULONG_PTR* const        pulParam,
                    __out_bcount(cbParamMax) PWSTR const            wszParam,
                    const size_t            cbParamMax )
{
    if ( NULL == pulParam )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    Assert( pjetparam->m_paramid == JET_paramConfiguration );

    OnDebug( const CJetParam::Config configInternal = pinst == pinstNil ? g_config : pinst->m_config );
    const ULONG_PTR ulpConfigUser = (CJetParam::Config)( pinst ? ( pinst->m_rgparam[JET_paramConfiguration].m_valueCurrent ) : ( g_rgparam[JET_paramConfiguration].m_valueCurrent ) );
    const CJetParam::Config configUser = (CJetParam::Config)ulpConfigUser;
    Assert( (ULONG_PTR)configUser == ulpConfigUser );
    Assert( configInternal == configUser ||
                configUser == 0  );
    *pulParam = configUser;
    return JET_errSuccess;
}

VOID SetJetConfigSet( INST * const pinst, _In_reads_(cConfigOverrides) const ConfigSetOverrideValue * const prgConfigOverrides, ULONG cConfigOverrides )
{
    const ULONG_PTR cbPage = g_cbPage;

    for ( size_t iiparam = 0; iiparam < cConfigOverrides; iiparam++ )
    {
        size_t iparam = prgConfigOverrides[iiparam].paramid;

        CJetParam* pjetparamT = NULL;
        if ( pinst == pinstNil || g_rgparam[ iparam ].FGlobal() )
        {
            pjetparamT = g_rgparam + iparam;
        }
        else
        {
            pjetparamT = pinst->m_rgparam + iparam;
        }

        Assert( JET_paramConfiguration != prgConfigOverrides[iiparam].paramid );

        ULONG_PTR ulpFinal = prgConfigOverrides[iiparam].ulpValue;
        switch( prgConfigOverrides[iiparam].grbit & coUnitConversionsMsk )
        {
            case 0:
                break;
            case coBytesToPages:
                ulpFinal = ulpFinal / cbPage;
                break;
            case coPctRamToPages:
                Assert( (QWORD)OSMemoryQuotaTotal() * (QWORD)ulpFinal > OSMemoryQuotaTotal() );
                Assert( (QWORD)OSMemoryQuotaTotal() * (QWORD)ulpFinal > ulpFinal );
                ulpFinal = ( (QWORD)OSMemoryQuotaTotal() * (QWORD)ulpFinal / (QWORD)100 / (QWORD)cbPage );
                Assert( (QWORD)ulpFinal == ( (QWORD)OSMemoryQuotaTotal() * (QWORD)prgConfigOverrides[iiparam].ulpValue / (QWORD)100 / (QWORD)cbPage ) );
                break;
            case coPctCacheMaxToPages:
                if ( g_rgparam[JET_paramCacheSizeMax].m_valueCurrent <= 100 )
                {
                    ulpFinal = max( g_rgparam[JET_paramCacheSizeMax].m_valueCurrent * ulpFinal / 100,
                                    ULONG( JET_paramStartFlushThreshold == prgConfigOverrides[iiparam].paramid ? 1 : 2 ) );
                }
                else
                {
                    ulpFinal = g_rgparam[JET_paramCacheSizeMax].m_valueCurrent * ulpFinal / 100;
                }
                break;

            default:
                AssertSz( fFalse, "Unknown conversion constant = %d (0x%x)", prgConfigOverrides[iiparam].grbit & coUnitConversionsMsk, prgConfigOverrides[iiparam].grbit & coUnitConversionsMsk );
        }

        if ( ( prgConfigOverrides[iiparam].paramid == JET_paramLogFileSize ) &&
                ( cbPage == ( 32 * 1024 ) ) &&
                ( ulpFinal == 64 ) )
        {
            ulpFinal *= 2;
        }


        if ( pinst == pinstNil || !pjetparamT->FGlobal() )
        {
            (void)pjetparamT->Reset( pinst, ulpFinal );
        }
    }
}

ERR
SetConfiguration(   CJetParam* const    pjetparam,
                    INST* const         pinst,
                    PIB* const          ppib,
                    const ULONG_PTR     ulParam,
                    PCWSTR          wszParam )
{
    JET_ERR err = JET_errSuccess;


    if ( pinst == pinstNil && RUNINSTGetMode() != runInstModeNoSet )
    {
        return ErrERRCheck( JET_errAlreadyInitialized );
    }

    if ( pinst != pinstNil && pinst->m_fSTInit == fSTInitDone )
    {
        return ErrERRCheck( JET_errAlreadyInitialized );
    }

    if ( ulParam < pjetparam->m_rangeLow || ulParam > pjetparam->m_rangeHigh )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    const CJetParam::Config     configLegacySmall   = 0;
    const CJetParam::Config     configLegacyLegacy  = 1;

    CJetParam::Config   configSet;
    CJetParam::Config   configFinal;
    CJetParam::Config   configLegacy;

    C_ASSERT( 1  == configLegacyLegacy );
    C_ASSERT( 1  == JET_configDefault );

    if ( ulParam == configLegacySmall  )
    {

        configSet = ( JET_configDefault | JET_configLowDiskFootprint | JET_configLowMemory | JET_configRemoveQuotas );
        configFinal = configSet;
        configLegacy = configLegacySmall ;
    }
    else if ( ulParam == configLegacyLegacy  )
    {
        configSet = ( JET_configDefault );
        configFinal = configSet;
        configLegacy = configLegacyLegacy;
        Assert( configSet == configFinal );
    }
    else
    {
        configSet = CJetParam::Config( ulParam );

        if ( configSet & JET_configDefault )
        {

            configFinal = ( JET_configDefault );
        }
        else
        {
            configFinal = ( pinst == pinstNil ) ? g_config : pinst->m_config;
        }


        configFinal |= ( configSet & ~JET_configDefault );
        configLegacy = configFinal;
    }


    Assert( ( configSet & ~( ( pinst == pinstNil ) ? g_config : pinst->m_config ) ) ==
                ( configFinal & ~( ( pinst == pinstNil ) ? g_config : pinst->m_config ) ) );


    Assert( configLegacy == configLegacySmall ||
                configLegacy == configFinal );

    CheckParamTrap( JET_paramConfiguration, " (value to be set configLegacy | configFinal depending upon how you look at it)." );
    if ( pinst == pinstNil )
    {
        g_rgparam[JET_paramConfiguration].m_valueCurrent    = configLegacy;
        g_rgparam[JET_paramConfiguration].m_fWritten        = fTrue;
        g_config = configFinal;
    }
    else
    {
        pinst->m_rgparam[JET_paramConfiguration].m_valueCurrent = configLegacy;
        pinst->m_rgparam[JET_paramConfiguration].m_fWritten     = fTrue;
        pinst->m_config = configFinal;
    }


    if ( configSet & JET_configDefault || configLegacy == configLegacySmall )
    {
        for ( size_t iparam = 0; iparam < g_cparam; iparam++ )
        {
            CJetParam* pjetparamT = NULL;

            if ( pinst == pinstNil || g_rgparam[ iparam ].FGlobal() )
            {
                pjetparamT = g_rgparam + iparam;
            }
            else
            {
                pjetparamT = pinst->m_rgparam + iparam;
            }

            if ( pjetparamT->m_paramid == JET_paramConfiguration )
            {
                continue;
            }

            if ( !( pinst != pinstNil && pjetparamT->FGlobal() ) )
            {
                (void)pjetparamT->Reset( pinst, pjetparamT->m_valueDefault[ configLegacy & JET_configDefault ] );
            }

            AssertSz( !( !pjetparamT->m_fCustomGetSet && pjetparamT->m_fCustomStorage ),
                        "You are probably adding a new parameter definition macro. Please, review the "
                        "system parameter code and verify the implications of having m_fCustomGetSet false "
                        "and m_fCustomStorage true." );
        }
    }
    configSet &= ~JET_configDefault;


    if ( configSet & JET_configLowDiskFootprint && configLegacy != configLegacySmall )
    {
        SetJetConfigSet( pinst, g_rgJetConfigLowDiskFootprint, _countof(g_rgJetConfigLowDiskFootprint) );
    }
    configSet &= ~JET_configLowDiskFootprint;

    if ( configSet & JET_configMediumDiskFootprint )
    {
        SetJetConfigSet( pinst, g_rgJetConfigMediumDiskFootprint, _countof(g_rgJetConfigMediumDiskFootprint) );
        configSet &= ~JET_configMediumDiskFootprint;
    }



    if ( configSet & JET_configLowMemory && configLegacy != configLegacySmall )
    {

        for ( size_t iparam = 0; iparam < g_cparam; iparam++ )
        {
            CJetParam* pjetparamT = NULL;
            if ( pinst == pinstNil || g_rgparam[ iparam ].FGlobal() )
            {
                pjetparamT = g_rgparam + iparam;
            }
            else
            {
                pjetparamT = pinst->m_rgparam + iparam;
            }

            if ( pjetparamT->m_paramid == JET_paramConfiguration )
            {
                continue;
            }

            if ( pinst == pinstNil || !pjetparamT->FGlobal() )
            {
                if ( pjetparamT->m_valueDefault[ configLegacySmall ] != pjetparamT->m_valueDefault[ configLegacyLegacy ] )
                {
                    (void)pjetparamT->Reset( pinst, pjetparamT->m_valueDefault[configLegacySmall] );
                }
            }
        }
    }
    configSet &= ~JET_configLowMemory;

    if ( configSet & JET_configDynamicMediumMemory )
    {
        SetJetConfigSet( pinst, g_rgJetConfigDynamicMediumMemory, _countof(g_rgJetConfigDynamicMediumMemory) );
        configSet &= ~JET_configDynamicMediumMemory;
    }
    if ( configSet & JET_configUnthrottledMemory )
    {
        configSet &= ~JET_configUnthrottledMemory;
    }
    if ( configSet & JET_configSSDProfileIO )
    {
        SetJetConfigSet( pinst, g_rgJetConfigSSDProfileIO, _countof(g_rgJetConfigSSDProfileIO) );
        configSet &= ~JET_configSSDProfileIO;
    }
    if ( configSet & JET_configLowPower )
    {
        configSet &= ~JET_configLowPower;
    }
    if ( configSet & JET_configRemoveQuotas )
    {
        SetJetConfigSet( pinst, g_rgJetConfigRemoveQuotas, _countof(g_rgJetConfigRemoveQuotas) );
        configSet &= ~JET_configRemoveQuotas;
    }
    if ( configSet & JET_configRunSilent )
    {
        SetJetConfigSet( pinst, g_rgJetConfigRunSilent, _countof(g_rgJetConfigRunSilent) );
        configSet &= ~JET_configRunSilent;
    }
    if ( configSet & JET_configHighConcurrencyScaling )
    {
        configSet &= ~JET_configHighConcurrencyScaling;
    }


    if ( configSet )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }


    return err;
}

ERR
CloneConfiguration( CJetParam* const    pjetparamSrc,
                    INST* const         pinstSrc,
                    PIB* const          ppibSrc,
                    CJetParam* const    pjetparamDst,
                    INST* const         pinstDst,
                    PIB* const          ppibDst )
{
    ERR err = JET_errSuccess;

    Assert( pinstDst != NULL );

    memcpy( pjetparamDst, pjetparamSrc, sizeof( CJetParam ) );
#ifdef DEBUG
    ULONG_PTR ulParam;
    if ( GetConfiguration( pjetparamSrc, pinstSrc, ppibSrc, &ulParam, NULL, 0 ) >= JET_errSuccess )
    {
        Assert( pjetparamDst->m_valueCurrent == ulParam );
    }
#endif

    pinstDst->m_config = ( pinstSrc == pinstNil ? g_config : pinstSrc->m_config );

    return err;
}

ERR
GetKeyMostMost( const CJetParam* const  pjetparam,
                INST* const             pinst,
                PIB* const              ppib,
                ULONG_PTR* const        pulParam,
                __out_bcount(cbParamMax) PWSTR const            wszParam,
                const size_t            cbParamMax )
{
    if ( NULL == pulParam )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    *pulParam = CbKeyMostForPage();

    return JET_errSuccess;
}

ERR
GetRecordSizeMost( const CJetParam* const  pjetparam,
    INST* const             pinst,
    PIB* const              ppib,
    ULONG_PTR* const        pulParam,
    __out_bcount(cbParamMax) PWSTR const            wszParam,
    const size_t            cbParamMax )
{
    if ( NULL == pulParam )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    Assert( FIsValidPageSize( g_cbPage ) );

    *pulParam = CPAGE::CbPageDataMaxNoInsert( g_cbPage );
    return JET_errSuccess;
}

ERR
ErrSetCacheClosedTables(    CJetParam* const    pjetparam,
                    INST* const         pinst,
                    PIB* const          ppib,
                    const ULONG_PTR     ulParam,
                    PCWSTR      wszParam )
{
    ERR err = JET_errSuccess;

    if ( pinstNil != pinst )
    {
        Call( CJetParam::SetInteger( pjetparam, pinst, ppib, ulParam, wszParam ) );
        FCB::RefreshPreferredPerfCounter( pinst );
    }

HandleError:
    return err;
}

#define PFNGET_OF_TYPE( type )                                      ( \
    type == CJetParam::typeBoolean ?        CJetParam::GetInteger   : ( \
    type == CJetParam::typeGrbit ?          CJetParam::GetInteger   : ( \
    type == CJetParam::typeInteger ?        CJetParam::GetInteger   : ( \
    type == CJetParam::typeString ?         CJetParam::GetString    : ( \
    type == CJetParam::typePointer ?        CJetParam::GetInteger   : ( \
    type == CJetParam::typeFolder ?         CJetParam::GetString    : ( \
    type == CJetParam::typePath ?           CJetParam::GetString    : ( \
    type == CJetParam::typeBlockSize ?      CJetParam::GetInteger   : ( \
                                  CJetParam::IllegalGet   ) ) ) ) ) ) ) ) )
#define PFNSET_OF_TYPE( type )                                      ( \
    type == CJetParam::typeBoolean ?        CJetParam::SetBoolean   : ( \
    type == CJetParam::typeGrbit ?          CJetParam::SetGrbit     : ( \
    type == CJetParam::typeInteger ?        CJetParam::SetInteger   : ( \
    type == CJetParam::typeString ?         CJetParam::SetString    : ( \
    type == CJetParam::typePointer ?        CJetParam::SetPointer   : ( \
    type == CJetParam::typeFolder ?         CJetParam::SetFolder    : ( \
    type == CJetParam::typePath ?           CJetParam::SetPath      : ( \
    type == CJetParam::typeBlockSize ?      CJetParam::SetBlockSize : ( \
                                  CJetParam::IllegalSet   ) ) ) ) ) ) ) ) )
#define PFNCLONE_OF_TYPE( type )                                    ( \
    type == CJetParam::typeBoolean ?        CJetParam::CloneDefault : ( \
    type == CJetParam::typeGrbit ?          CJetParam::CloneDefault : ( \
    type == CJetParam::typeInteger ?        CJetParam::CloneDefault : ( \
    type == CJetParam::typeString ?         CJetParam::CloneString  : ( \
    type == CJetParam::typePointer ?        CJetParam::CloneDefault : ( \
    type == CJetParam::typeFolder ?         CJetParam::CloneString  : ( \
    type == CJetParam::typePath ?           CJetParam::CloneString  : ( \
    type == CJetParam::typeBlockSize ?      CJetParam::CloneDefault : ( \
                                  CJetParam::IllegalClone ) ) ) ) ) ) ) ) )

#define ILLEGAL_PARAM(  paramid )                       \
    { #paramid, paramid, CJetParam::typeNull, fFalse, fFalse, fTrue, fTrue, fTrue, fTrue, fFalse, fFalse, fFalse, fFalse, 0, ULONG_PTR( ~0 ), CJetParam::IllegalGet, CJetParam::IllegalSet, CJetParam::CloneDefault }

#define CUSTOM_PARAM(   paramid,                        \
                        type,                           \
                        fAdvanced,                      \
                        fGlobal,                        \
                        fMayNotWriteAfterGlobalInit,    \
                        fMayNotWriteAfterInstanceInit,  \
                        pfnGet,                         \
                        pfnSet,                         \
                        pfnClone )                      \
    { #paramid, paramid, type, fTrue, fTrue, fAdvanced, fGlobal, fMayNotWriteAfterGlobalInit, fMayNotWriteAfterInstanceInit, fFalse, fFalse, fFalse, fFalse, 0, ULONG_PTR( ~0 ), pfnGet, pfnSet, pfnClone }

#define CUSTOM_PARAM2(  paramid,                        \
                        type,                           \
                        fAdvanced,                      \
                        fGlobal,                        \
                        fMayNotWriteAfterGlobalInit,    \
                        fMayNotWriteAfterInstanceInit,  \
                        rangeLow,                       \
                        rangeHigh,                      \
                        value,                          \
                        value2,                         \
                        pfnGet,                         \
                        pfnSet,                         \
                        pfnClone )                      \
    { #paramid, paramid, type, fTrue, fTrue, fAdvanced, fGlobal, fMayNotWriteAfterGlobalInit, fMayNotWriteAfterInstanceInit, fFalse, fFalse, fFalse, fFalse, ULONG_PTR( rangeLow ), ULONG_PTR( rangeHigh ), pfnGet, pfnSet, pfnClone, { ULONG_PTR( value ), ULONG_PTR( value2 ) }, ULONG_PTR( value2 ) }

#define CUSTOM_PARAM3(  paramid,                        \
                        type,                           \
                        fAdvanced,                      \
                        fGlobal,                        \
                        fMayNotWriteAfterGlobalInit,    \
                        fMayNotWriteAfterInstanceInit,  \
                        rangeLow,                       \
                        rangeHigh,                      \
                        value,                          \
                        value2,                         \
                        pfnGet,                         \
                        pfnSet,                         \
                        pfnClone )                      \
    { #paramid, paramid, type, fTrue, fFalse, fAdvanced, fGlobal, fMayNotWriteAfterGlobalInit, fMayNotWriteAfterInstanceInit, fFalse, fFalse, fFalse, fFalse, ULONG_PTR( rangeLow ), ULONG_PTR( rangeHigh ), pfnGet, pfnSet, pfnClone, { ULONG_PTR( value ), ULONG_PTR( value2 ) }, ULONG_PTR( value2 ) }
#define READONLY_PARAM( paramid,                        \
                        type,                           \
                        value )                         \
    { #paramid, paramid, type, fFalse, fFalse,   fFalse,   fTrue,                       fTrue,                          fTrue, fFalse, fFalse, fFalse, fFalse,                    0, ULONG_PTR( ~0 ), PFNGET_OF_TYPE( type ), CJetParam::IllegalSet, PFNCLONE_OF_TYPE( type ), { ULONG_PTR( value ), ULONG_PTR( value ) }, ULONG_PTR( value ) }
#define NORMAL_PARAMEX( paramname,                      \
                        paramid,                        \
                        type,                           \
                        fAdvanced,                      \
                        fGlobal,                        \
                        fMayNotWriteAfterGlobalInit,    \
                        fMayNotWriteAfterInstanceInit,  \
                        rangeLow,                       \
                        rangeHigh,                      \
                        value,                          \
                        value2 )                        \
    { paramname, paramid, type, fFalse, fFalse, fAdvanced, fGlobal, fMayNotWriteAfterGlobalInit, fMayNotWriteAfterInstanceInit, fFalse, fFalse, fFalse, fFalse, ULONG_PTR( rangeLow ), ULONG_PTR( rangeHigh ), PFNGET_OF_TYPE( type ), PFNSET_OF_TYPE( type ), PFNCLONE_OF_TYPE( type ), { ULONG_PTR( value ), ULONG_PTR( value2 ) }, ULONG_PTR( value2 ) }
#define NORMAL_PARAM2(  paramid,                        \
                        type,                           \
                        fAdvanced,                      \
                        fGlobal,                        \
                        fMayNotWriteAfterGlobalInit,    \
                        fMayNotWriteAfterInstanceInit,  \
                        rangeLow,                       \
                        rangeHigh,                      \
                        value,                          \
                        value2 )                        \
    { #paramid, paramid, type, fFalse, fFalse, fAdvanced, fGlobal, fMayNotWriteAfterGlobalInit, fMayNotWriteAfterInstanceInit, fFalse, fFalse, fFalse, fFalse, ULONG_PTR( rangeLow ), ULONG_PTR( rangeHigh ), PFNGET_OF_TYPE( type ), PFNSET_OF_TYPE( type ), PFNCLONE_OF_TYPE( type ), { ULONG_PTR( value ), ULONG_PTR( value2 ) }, ULONG_PTR( value2 ) }
#define NORMAL_PARAM(   paramid,                        \
                        type,                           \
                        fAdvanced,                      \
                        fGlobal,                        \
                        fMayNotWriteAfterGlobalInit,    \
                        fMayNotWriteAfterInstanceInit,  \
                        rangeLow,                       \
                        rangeHigh,                      \
                        value )                         \
    NORMAL_PARAMEX( #paramid, paramid, type, fAdvanced, fGlobal, fMayNotWriteAfterGlobalInit, fMayNotWriteAfterInstanceInit, rangeLow, rangeHigh, value, value )
#define IGNORED_PARAM(  paramid,                        \
                        type,                           \
                        fAdvanced,                      \
                        fGlobal,                        \
                        fMayNotWriteAfterGlobalInit,    \
                        fMayNotWriteAfterInstanceInit,  \
                        rangeLow,                       \
                        rangeHigh,                      \
                        value )                         \
    NORMAL_PARAMEX( #paramid, paramid, type, fAdvanced, fGlobal, fMayNotWriteAfterGlobalInit, fMayNotWriteAfterInstanceInit, rangeLow, rangeHigh, value, value )
#define IGNORED_PARAM2( paramid,                        \
                        type,                           \
                        fAdvanced,                      \
                        fGlobal,                        \
                        fMayNotWriteAfterGlobalInit,    \
                        fMayNotWriteAfterInstanceInit,  \
                        rangeLow,                       \
                        rangeHigh,                      \
                        value,                          \
                        value2 )                        \
    NORMAL_PARAMEX( #paramid, paramid, type, fAdvanced, fGlobal, fMayNotWriteAfterGlobalInit, fMayNotWriteAfterInstanceInit, rangeLow, rangeHigh, value, value2 )




#ifdef _PREFAST_
const
#endif


#ifdef ESENT
#define JET_paramLegacyFileNames_DEFAULT            (JET_bitESE98FileNames | JET_bitEightDotThreeSoftCompat)
#define JET_paramEnablePersistedCallbacks_DEFAULT   0

#else
#define JET_paramLegacyFileNames_DEFAULT            JET_bitESE98FileNames

#define JET_paramEnablePersistedCallbacks_DEFAULT   1
#endif

#ifdef PERFMON_SUPPORT
#define JET_paramDisablePerfmon_DEFAULT     fFalse
#else
#define JET_paramDisablePerfmon_DEFAULT     fTrue
#endif

#if ( DEBUG && !ESENT )
#define JET_paramEnableShrinkDatabase_DEFAULT       ( JET_bitShrinkDatabaseOn | JET_bitShrinkDatabaseRealtime | JET_bitShrinkDatabasePeriodically )
#else
#define JET_paramEnableShrinkDatabase_DEFAULT       JET_bitShrinkDatabaseOff
#endif

#if defined(ESENT) && !defined(DEBUG)
#define JET_paramPersistedLostFlushDetection_DEFAULT        ( JET_bitPersistedLostFlushDetectionEnabled | JET_bitPersistedLostFlushDetectionFailOnRuntimeOnly )
#else
#define JET_paramPersistedLostFlushDetection_DEFAULT        JET_bitPersistedLostFlushDetectionEnabled
#endif

#ifdef DEBUG
#define JET_paramFlight_ElasticWaypointLatency_DEFAULT 2
#else
#define JET_paramFlight_ElasticWaypointLatency_DEFAULT 0
#endif

#ifdef ESENT
#define JET_paramStageFlighting_DEFAULT 0x800000
#else
#define JET_paramStageFlighting_DEFAULT 0
#endif

#ifdef ESENT
#define grbitEseSkuQueueOptionDefault 0x0
#else
#define grbitEseSkuQueueOptionDefault ( bitUseMetedQEseTasks )
#endif

#define JET_paramFlight_RBSCleanupEnabledDEFAULT                OnDebugOrRetail( fTrue, fFalse )

#include "sysparamtable.g.cxx"

C_ASSERT( _countof( g_rgparamRaw ) == JET_paramMaxValueInvalid + 1 );

const size_t    g_cparam    = _countof( g_rgparamRaw );

C_ASSERT( sizeof( JetParam ) == sizeof( CJetParam ) );

CJetParam* const        g_rgparam   = (CJetParam*) &g_rgparamRaw[ 0 ];

LOCAL CCriticalSection  g_critSysParamFixup( CLockBasicInfo( CSyncBasicInfo( "g_critSysParamFixup" ), rankSysParamFixup, 0 ) );

VOID FixDefaultSystemParameters()
{
    static BOOL fDefaultSystemParametersFixedUp = fFalse;

    ERR err = JET_errSuccess;
    IFileSystemAPI* pfsapi = NULL;

    if ( fDefaultSystemParametersFixedUp )
    {
        return;
    }

    g_critSysParamFixup.Enter();


    if ( !fDefaultSystemParametersFixedUp )
    {

        DWORD cConfiguration = JET_configDefault;

        if ( ErrUtilSystemSlConfiguration( g_wszLP_PARAMCONFIGURATION, &cConfiguration ) >= JET_errSuccess )
        {
            SetConfiguration( &(g_rgparam[JET_paramConfiguration]), pinstNil, ppibNil, cConfiguration, NULL );
        }

    

        WCHAR rgwchDefaultPath[ IFileSystemAPI::cchPathMax ];
        BOOL fIsDefaultDirectory;


        Call( ErrOSFSCreate( g_pfsconfigGlobal, &pfsapi ) );
        Call( pfsapi->ErrPathFolderDefault( rgwchDefaultPath, _countof( rgwchDefaultPath ), &fIsDefaultDirectory ) );

        if ( !fIsDefaultDirectory )
        {
            const ULONG rgjpPathFixup[] =
            {
                JET_paramSystemPath,
                JET_paramLogFilePath,
            };

            for ( size_t i = 0; i < _countof( rgjpPathFixup ); ++i )
            {
                Call( CJetParam::SetPath( &g_rgparam[ rgjpPathFixup[ i ] ], NULL, NULL, 0, rgwchDefaultPath ) );
            }

        }

        Assert( fDefaultSystemParametersFixedUp == fFalse );
    }

HandleError:

    fDefaultSystemParametersFixedUp = fTrue;

    Assert( fDefaultSystemParametersFixedUp );

    g_critSysParamFixup.Leave();

    delete pfsapi;

    return;
}


BOOL FConfigStoreLoadableParam( CJetParam* const pjetparam )
{
    if ( pjetparam->m_paramid == JET_paramEnablePersistedCallbacks )
    {
        return fFalse;
    }
    if ( pjetparam->m_paramid == JET_paramConfigStoreSpec )
    {
        return fFalse;
    }

    return ( pjetparam->Type_() == JetParam::typeBlockSize ||
                pjetparam->Type_() == JetParam::typeBoolean ||
                pjetparam->Type_() == JetParam::typeGrbit ||
                pjetparam->Type_() == JetParam::typeInteger );
}

ERR ErrSysParamLoadOverride( _In_z_ PCWSTR wszConfigStoreSpec, _In_ CJetParam* const pjetparam, _Out_ ULONG_PTR * pulpParam )
{
    ERR err = JET_errSuccess;
    CConfigStore * pcs = NULL;

    if ( !FConfigStoreLoadableParam( pjetparam ) )
    {
        return ErrERRCheck( JET_wrnColumnNull );
    }

    Call( ErrOSConfigStoreInit( wszConfigStoreSpec, &pcs ) );

    Call( ErrConfigReadValue( pcs, csspSysParamOverride, pjetparam->m_szParamName, pulpParam ) );

HandleError:
    OSConfigStoreTerm( pcs );

    return err;
}

inline ERR ErrSetSystemParameter(
    INST*           pinst,
    JET_SESID       sesid,
    ULONG   paramid,
    ULONG_PTR       ulParam,
    __in PCWSTR     wszParam,
    const BOOL      fEnterCritInst  = fTrue )
{
    ERR                 err         = JET_errSuccess;
    PIB*                ppib        = ( JET_sesidNil != sesid ? (PIB*)sesid : ppibNil );
    CJetParam*          pjetparam   = NULL;
    BOOL                fEnableAdvanced = fFalse;
    BOOL                fOriginalSet = fFalse;
    ULONG_PTR           ulpOriginal = ulParam;
    BOOL                fLoadedOverride = fFalse;


    if ( fEnterCritInst )
    {
        INST::EnterCritInst();
    }


    if ( paramid >= g_cparam )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( pinst == pinstNil || g_rgparam[ paramid ].FGlobal() )
    {
        pjetparam = g_rgparam + paramid;
        fEnableAdvanced = BOOL( g_rgparam[ JET_paramEnableAdvanced ].Value( ) );
        if ( g_rgparam[JET_paramConfigStoreSpec].Value() )
        {
            if ( ErrSysParamLoadOverride( (PCWSTR)g_rgparam[JET_paramConfigStoreSpec].Value(), pjetparam, &ulParam ) == JET_errSuccess )
            {
                fLoadedOverride = fTrue;
                fOriginalSet = g_rgparam[paramid].FWritten();
                ulpOriginal = g_rgparam[paramid].Value();
            }
        }
    }
    else
    {
        pjetparam = pinst->m_rgparam + paramid;
        fEnableAdvanced = BoolParam( pinst, JET_paramEnableAdvanced );
        if ( pinst->m_rgparam[JET_paramConfigStoreSpec].Value() )
        {
            if ( ErrSysParamLoadOverride( (PCWSTR)pinst->m_rgparam[JET_paramConfigStoreSpec].Value(), pjetparam, &ulParam ) == JET_errSuccess )
            {
                fLoadedOverride = fTrue;
                fOriginalSet = pinst->m_rgparam[paramid].FWritten();
                ulpOriginal = pinst->m_rgparam[paramid].Value();
            }
        }
    }

    if ( pjetparam->FAdvanced() && !fEnableAdvanced )
    {
        Error( JET_errSuccess );
    }


    Call( pjetparam->Set( pinst, ppib, ulParam, wszParam ) );

    if ( fLoadedOverride )
    {
        pjetparam->m_fRegOverride = fLoadedOverride;

        WCHAR wszParamName[100];
        WCHAR wszOriginal[21];
        WCHAR wszOverride[21];
        OSStrCbFormatW( wszParamName, sizeof(wszParamName), L"%hs", pjetparam->m_szParamName );
        if ( fOriginalSet )
        {
            OSStrCbFormatW( wszOriginal, sizeof(wszOriginal), L"%#Ix", ulpOriginal );
        }
        else
        {
            OSStrCbFormatW( wszOriginal, sizeof(wszOriginal), L"---" );
        }
        OSStrCbFormatW( wszOverride, sizeof(wszOverride), L"%#Ix", ulParam );
        const WCHAR * rgwszT[] = { wszParamName, wszOriginal, wszOverride, SzParam( pinst, JET_paramConfigStoreSpec ) };
        UtilReportEvent(    eventWarning,
                            GENERAL_CATEGORY,
                            CONFIG_STORE_PARAM_OVERRIDE_ID,
                            _countof(rgwszT), rgwszT );
    }

HandleError:
    if ( fEnterCritInst )
    {
        INST::LeaveCritInst();
    }
    return err;
}

inline ERR ErrGetSystemParameter(
    INST*           pinst,
    JET_SESID       sesid,
    ULONG   paramid,
    ULONG_PTR*      pulParam,
    _Out_z_bytecap_(cbMax) JET_PWSTR            wszParam,
    ULONG   cbMax )
{
    ERR                 err         = JET_errSuccess;
    PIB*                ppib        = ( JET_sesidNil != sesid ? (PIB*)sesid : ppibNil );
    const CJetParam*    pjetparam   = NULL;


    if ( paramid >= g_cparam )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( pinst == pinstNil || g_rgparam[ paramid ].FGlobal() )
    {
        pjetparam = g_rgparam + paramid;
    }
    else
    {
        pjetparam = pinst->m_rgparam + paramid;
    }


    Call( pjetparam->Get( pinst, ppib, pulParam, wszParam, cbMax ) );

HandleError:
    return err;
}



ERR ErrSysParamLoadDefaults( const BOOL fHasCritInst, INST * pinst, CConfigStore * const pcs, _In_reads_(cparam) CJetParam * const prgparam, const ULONG cparam )
{
    ERR err = JET_errSuccess;

    Expected( cparam == _countof(g_rgparamRaw) );

    const BOOL fLoadingGlobal = ( prgparam == g_rgparamRaw );

    Assert( !fHasCritInst || INST::FOwnerCritInst() );

    for ( size_t iparamid = 0; iparamid < (size_t)cparam; iparamid++ )
    {
        Assert( prgparam[iparamid].m_paramid == iparamid );

        if ( !fLoadingGlobal &&
                prgparam[iparamid].FGlobal() &&
                iparamid != JET_paramConfiguration &&
                iparamid != JET_paramConfigStoreSpec )
        {
            continue;
        }

        if ( FConfigStoreLoadableParam( &(prgparam[iparamid]) ) )
        {
            ULONG_PTR ulpValue;
            BOOL fLoadedDefault = fFalse;

            err = ErrConfigReadValue( pcs, csspSysParamOverride, prgparam[ iparamid ].m_szParamName, &ulpValue );
            if ( err != JET_errSuccess )
            {
                Call( ErrConfigReadValue( pcs, csspSysParamDefault, prgparam[ iparamid ].m_szParamName, &ulpValue ) );
                fLoadedDefault = fTrue;
            }
            Assert( err == JET_errSuccess || err == JET_wrnColumnNull || err == JET_wrnTableEmpty );

            if ( err == JET_errSuccess )
            {
                err = ErrSetSystemParameter( pinst, NULL, iparamid, ulpValue, NULL, !fHasCritInst );
                if ( err < JET_errSuccess )
                {
                    WCHAR wszParamName[100];
                    WCHAR wszValue[21];
                    WCHAR wszErr[21];
                    OSStrCbFormatW( wszParamName, sizeof(wszParamName), L"%hs", prgparam[ iparamid ].m_szParamName );
                    OSStrCbFormatW( wszValue, sizeof(wszValue), L"%#Ix", ulpValue );
                    OSStrCbFormatW( wszErr, sizeof(wszErr), L"%d", err );
                    const WCHAR * rgwszT[] = { wszParamName, wszValue, wszErr, SzParam( pinst, JET_paramConfigStoreSpec ) };
                    UtilReportEvent(    eventWarning,
                                        GENERAL_CATEGORY,
                                        CONFIG_STORE_PARAM_DEFAULT_LOAD_FAIL_ID,
                                        _countof(rgwszT), rgwszT );
                    Error( err );
                }
                else
                {
                    prgparam[iparamid].m_fRegDefault = fLoadedDefault;
                }
            }
        }
        else if ( iparamid != JET_paramConfigStoreSpec )
        {
            AssertSz( prgparam[iparamid].Type_() == JetParam::typeNull ||
                        prgparam[iparamid].Type_() == JetParam::typeString ||
                        prgparam[iparamid].Type_() == JetParam::typeFolder ||
                        prgparam[iparamid].Type_() == JetParam::typePath ||
                        prgparam[iparamid].Type_() == JetParam::typePointer ||
                        prgparam[iparamid].Type_() == JetParam::typeUserDefined ||
                        iparamid == JET_paramEnablePersistedCallbacks,
                        "Unknown type(%d) provided\n", prgparam[iparamid].Type_() );

            if ( FConfigValuePresent( pcs, csspSysParamDefault, prgparam[ iparamid ].m_szParamName ) )
            {
                Call( ErrERRCheck( JET_errFeatureNotAvailable ) );
            }
        }

    }

    Assert( err >= JET_errSuccess );
    err = JET_errSuccess;

HandleError:

    return err;
}

ERR
ErrSetConfigStoreSpec(  CJetParam* const    pjetparam,
                INST* const             pinst,
                PIB* const              ppib,
                const ULONG_PTR         ulParam,
                PCWSTR                  wszParam )
{
    ERR err = JET_errSuccess;
    CConfigStore * pcs = NULL;

    if ( ppib != NULL && ppib != (PIB*)JET_sesidNil )
    {
        AssertSz( fFalse, "Can't supply a JET_SESID to the JET_paramConfigStoreSpec parameter" );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( wszParam == NULL )
    {
        if ( pjetparam->m_fFreeValue )
        {
            delete [] (void*)pjetparam->m_valueCurrent;
        }
        pjetparam->m_valueCurrent   = NULL;
        pjetparam->m_fWritten       = fTrue;
        pjetparam->m_fFreeValue     = fFalse;
        return JET_errSuccess;
    }

    CallR( CJetParam::SetString( pjetparam, pinst, ppib, ulParam, wszParam ) );


    if ( wszParam && wszParam[0] != L'\0' )
    {

        Call( ErrOSConfigStoreInit( wszParam, &pcs ) );
        Assert( pcs );

        Assert( FBounded( (QWORD)pjetparam, (QWORD)( pjetparam - pjetparam->m_paramid ), sizeof(JetParam) * _countof(g_rgparamRaw) ) );

        if ( pinst == NULL || pinst == (INST*)JET_instanceNil  )
        {
            Assert( FBounded( (QWORD)pjetparam, (QWORD)g_rgparamRaw, sizeof(JetParam) * _countof(g_rgparamRaw) ) );
            Call( ErrSysParamLoadDefaults( fTrue, pinstNil, pcs, g_rgparam, _countof(g_rgparamRaw) ) );
        }
        else
        {

            Assert( FBounded( (QWORD)pjetparam, (QWORD)(pinst->m_rgparam), sizeof(JetParam) * _countof(g_rgparamRaw) ) );
            Call( ErrSysParamLoadDefaults( fTrue, pinst, pcs, pinst->m_rgparam, _countof(g_rgparamRaw) ) );
        }
    }

HandleError:

    OSConfigStoreTerm( pcs );

    return err;
}


class InitCallbackWrapper
{
private:
    const JET_PFNSTATUS m_pfnStatus;

    ERR ErrCallback( const JET_SNT snt, const JET_SNP snp, void * const pv ) const
    {
        if ( NULL != m_pfnStatus )
        {
            return m_pfnStatus( JET_sesidNil, snt, snp, pv );
        }
        return JET_errSuccess;
    }

public:
    InitCallbackWrapper( const JET_PFNSTATUS pfnStatus ) : m_pfnStatus( pfnStatus )
    {
    }

    static JET_ERR PfnWrapper( JET_SNT snt, JET_SNP snp, void * pv, void * pvContext )
    {
        const InitCallbackWrapper * const pInitCallbackWrapper = (InitCallbackWrapper*)pvContext;
        Assert( NULL != pInitCallbackWrapper );
        return pInitCallbackWrapper->ErrCallback( snt, snp, pv );
    }
};

template< class JET_INDEXCREATE_T, class JET_INDEXCREATE2_T >
class CAutoINDEXCREATE1To2_T {
    JET_INDEXCREATE_T *    m_rgindexcreateFromAPI;
    JET_INDEXCREATE2_T *    m_rgindexcreateEngine;
    JET_SPACEHINTS *        m_rgspacehintsEngine;
    ULONG                   m_cindexcreate;
public:
    CAutoINDEXCREATE1To2_T() : m_rgindexcreateFromAPI( NULL ), m_rgindexcreateEngine( NULL ), m_rgspacehintsEngine( NULL ), m_cindexcreate( 0 ) { }
    ~CAutoINDEXCREATE1To2_T();
    ERR ErrSet( JET_INDEXCREATE_T * pindexcreate, ULONG cindexcreate = 1 );
    JET_INDEXCREATE2_T* GetPtr(void)        { return m_rgindexcreateEngine; }
    void Result( );
};

template< class JET_INDEXCREATE_T, class JET_INDEXCREATE2_T >
ERR CAutoINDEXCREATE1To2_T< JET_INDEXCREATE_T, JET_INDEXCREATE2_T >::ErrSet( JET_INDEXCREATE_T * pindexcreate, ULONG cindexcreate )
{
    ERR err = JET_errSuccess;

    delete [] m_rgindexcreateEngine;
    delete [] m_rgspacehintsEngine;
    m_rgindexcreateEngine = NULL;
    m_rgspacehintsEngine = NULL;

    if ( NULL == pindexcreate || 0 == cindexcreate )
    {
        if ( cindexcreate )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }
    else
    {
        Assert( pindexcreate );
        Assert( cindexcreate );

        Alloc( m_rgindexcreateEngine = new JET_INDEXCREATE2_T[cindexcreate] );
        Alloc( m_rgspacehintsEngine = new JET_SPACEHINTS[cindexcreate] );

        JET_INDEXCREATE_T * pidxcreateOld = pindexcreate;
        JET_INDEXCREATE_T * pidxcreateOldNext;

        for ( ULONG iIdx = 0; iIdx < cindexcreate; iIdx++, pidxcreateOld = pidxcreateOldNext )
        {
            pidxcreateOldNext = (JET_INDEXCREATE_T *)( ((BYTE*)pidxcreateOld) + pidxcreateOld->cbStruct );

            if ( pidxcreateOld->cbStruct != sizeof(*pindexcreate) &&
                    pidxcreateOld->cbStruct != sizeof(JET_INDEXCREATEOLD) )
            {
                return ErrERRCheck( JET_errInvalidParameter );
            }

            Assert( sizeof(*m_rgindexcreateEngine) > sizeof(*pindexcreate) );
            Enforce( sizeof(*m_rgindexcreateEngine) > pidxcreateOld->cbStruct );
            memcpy( &(m_rgindexcreateEngine[iIdx]), pidxcreateOld, pidxcreateOld->cbStruct );

            memset( &(m_rgspacehintsEngine[iIdx]), 0, sizeof(m_rgspacehintsEngine[iIdx]) );
            m_rgspacehintsEngine[iIdx].cbStruct = sizeof(m_rgspacehintsEngine[iIdx]);
            m_rgspacehintsEngine[iIdx].ulInitialDensity = pidxcreateOld->ulDensity;
            m_rgindexcreateEngine[iIdx].pSpacehints = &(m_rgspacehintsEngine[iIdx]);

            m_rgindexcreateEngine[iIdx].cbStruct = sizeof(m_rgindexcreateEngine[iIdx]);
        }

    }
    m_rgindexcreateFromAPI = pindexcreate;
    m_cindexcreate = cindexcreate;

HandleError:

    return err;
}

template< class JET_INDEXCREATE_T, class JET_INDEXCREATE2_T >
void CAutoINDEXCREATE1To2_T< JET_INDEXCREATE_T, JET_INDEXCREATE2_T >::Result( )
{
    JET_INDEXCREATE_T * pidxcreateFromAPI = m_rgindexcreateFromAPI;
    for ( ULONG iIdx = 0; iIdx < m_cindexcreate; iIdx++ )
    {
        pidxcreateFromAPI->err = m_rgindexcreateEngine[iIdx].err;
        pidxcreateFromAPI = (JET_INDEXCREATE_T *)( ((BYTE*)pidxcreateFromAPI) + pidxcreateFromAPI->cbStruct );
    }
}
template< class JET_INDEXCREATE_T, class JET_INDEXCREATE2_T >
CAutoINDEXCREATE1To2_T< JET_INDEXCREATE_T, JET_INDEXCREATE2_T >::~CAutoINDEXCREATE1To2_T()
{
    delete [] m_rgindexcreateEngine;
    delete [] m_rgspacehintsEngine;
}

template< class JET_INDEXCREATE2_T, class JET_INDEXCREATE3_T >
class CAutoINDEXCREATE2To3_T {
    JET_INDEXCREATE2_T *    m_rgindexcreateFromAPI;
    JET_INDEXCREATE3_T *    m_rgindexcreateEngine;
    WCHAR *                 m_rgwszLocaleName;
    JET_UNICODEINDEX2 *     m_rgunicodeindexEngine;
    ULONG                   m_cindexcreate;
public:
    CAutoINDEXCREATE2To3_T() : m_rgindexcreateFromAPI( NULL ), m_rgindexcreateEngine( NULL ), m_rgwszLocaleName( NULL ), m_rgunicodeindexEngine( NULL ), m_cindexcreate( 0 ) { }
    ~CAutoINDEXCREATE2To3_T();
    ERR ErrSet( JET_INDEXCREATE2_T * pindexcreate, ULONG cindexcreate = 1 );
    JET_INDEXCREATE3_T* GetPtr(void)        { return m_rgindexcreateEngine; }
    void Result( );
};

template< class JET_INDEXCREATE2_T, class JET_INDEXCREATE3_T >
ERR CAutoINDEXCREATE2To3_T< JET_INDEXCREATE2_T, JET_INDEXCREATE3_T >::ErrSet( JET_INDEXCREATE2_T * pindexcreate, ULONG cindexcreate )
{
    ERR err = JET_errSuccess;

    delete [] m_rgindexcreateEngine;
    delete [] m_rgunicodeindexEngine;
    delete [] m_rgwszLocaleName;
    m_rgindexcreateEngine = NULL;
    m_rgunicodeindexEngine = NULL;
    m_rgwszLocaleName = NULL;

    if ( NULL == pindexcreate || 0 == cindexcreate )
    {
        if ( cindexcreate )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }
    else
    {
        Assert( pindexcreate );
        Assert( cindexcreate );

        Alloc( m_rgindexcreateEngine = new JET_INDEXCREATE3_T[cindexcreate] );
        Alloc( m_rgunicodeindexEngine = new JET_UNICODEINDEX2[cindexcreate] );
        Alloc( m_rgwszLocaleName = new WCHAR[NORM_LOCALE_NAME_MAX_LENGTH * cindexcreate] );

        JET_INDEXCREATE2_T * pidxcreate2 = pindexcreate;
        JET_INDEXCREATE2_T * pidxcreate2Next;

        for ( ULONG iIdx = 0; iIdx < cindexcreate; iIdx++, pidxcreate2 = pidxcreate2Next )
        {
            pidxcreate2Next = (JET_INDEXCREATE2_T *)( ((BYTE*)pidxcreate2) + pidxcreate2->cbStruct );

            if ( pidxcreate2->cbStruct != sizeof(*pindexcreate) &&
                    pidxcreate2->cbStruct != sizeof(JET_INDEXCREATE2) )
            {
                return ErrERRCheck( JET_errInvalidParameter );
            }

            C_ASSERT( sizeof(*m_rgindexcreateEngine) == sizeof(*pindexcreate) );
            Assert( pidxcreate2->cbStruct <= sizeof(*m_rgindexcreateEngine) );
            memcpy( &(m_rgindexcreateEngine[iIdx]), pidxcreate2, pidxcreate2->cbStruct );

            m_rgindexcreateEngine[iIdx].cbStruct = sizeof(*m_rgindexcreateEngine);

            LCID lcidT;
            ULONG dwLCMapFlagsT;

            m_rgunicodeindexEngine[iIdx].szLocaleName = &(m_rgwszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH * iIdx]);
            if ( pidxcreate2->grbit & JET_bitIndexUnicode )
            {
                lcidT = pidxcreate2->pidxunicode->lcid;
                dwLCMapFlagsT = pidxcreate2->pidxunicode->dwMapFlags;
            }
            else
            {
                lcidT = LCID( DWORD_PTR( pidxcreate2->pidxunicode ) );
                dwLCMapFlagsT = 0;
            }

            if ( lcidNone != lcidT )
            {
                CallR( ErrNORMLcidToLocale( lcidT,
                                            m_rgunicodeindexEngine[iIdx].szLocaleName,
                                            NORM_LOCALE_NAME_MAX_LENGTH ) );
            }

            m_rgunicodeindexEngine[iIdx].dwMapFlags = dwLCMapFlagsT;

            AssertSz( ( ( ( pidxcreate2->grbit & JET_bitIndexUnicode ) != 0 ) || 0 == dwLCMapFlagsT ),
                      "When JET_bitIndexUnicode isn't set, then dwLCMapFlagsT must be zero." );

            if ( lcidNone != lcidT || 0 != dwLCMapFlagsT )
            {
                m_rgindexcreateEngine[iIdx].grbit |= JET_bitIndexUnicode;
            }

            if ( ( m_rgindexcreateEngine[iIdx].grbit & JET_bitIndexUnicode ) != 0 )
            {
                m_rgindexcreateEngine[iIdx].pidxunicode = &m_rgunicodeindexEngine[iIdx];
            }


            AssertSz( ( ( m_rgindexcreateEngine[iIdx].grbit & JET_bitIndexUnicode ) == 0 )
                    == ( ( DWORD_PTR( m_rgindexcreateEngine[iIdx].pidxunicode ) & 0xffffffff ) == NULL ),
                      "When JET_bitIndexUnicode is set, pidxunicode must be non-NULL." );

            m_rgindexcreateEngine[iIdx].cbStruct = sizeof(m_rgindexcreateEngine[iIdx]);
        }

    }
    m_rgindexcreateFromAPI = pindexcreate;
    m_cindexcreate = cindexcreate;

HandleError:

    return err;
}

template< class JET_INDEXCREATE2_T, class JET_INDEXCREATE3_T >
void CAutoINDEXCREATE2To3_T< JET_INDEXCREATE2_T, JET_INDEXCREATE3_T >::Result( )
{
    JET_INDEXCREATE2_T * pidxcreateFromAPI = m_rgindexcreateFromAPI;
    for ( ULONG iIdx = 0; iIdx < m_cindexcreate; iIdx++ )
    {
        pidxcreateFromAPI->err = m_rgindexcreateEngine[iIdx].err;
        pidxcreateFromAPI = (JET_INDEXCREATE2_T *)( ((BYTE*)pidxcreateFromAPI) + pidxcreateFromAPI->cbStruct );
    }
}
template< class JET_INDEXCREATE2_T, class JET_INDEXCREATE3_T >
CAutoINDEXCREATE2To3_T< JET_INDEXCREATE2_T, JET_INDEXCREATE3_T >::~CAutoINDEXCREATE2To3_T()
{
    delete [] m_rgindexcreateEngine;
    delete [] m_rgunicodeindexEngine;
    delete [] m_rgwszLocaleName;
}


template< class JET_INDEXCREATE3_T_FROM, class JET_INDEXCREATE3_T_TO >
class CAutoINDEXCREATE3To3_T {
    JET_INDEXCREATE3_T_FROM *    m_rgindexcreateFromAPI;
    JET_INDEXCREATE3_T_TO *    m_rgindexcreateEngine;
    ULONG                   m_cindexcreate;
public:
    CAutoINDEXCREATE3To3_T() : m_rgindexcreateFromAPI( NULL ), m_rgindexcreateEngine( NULL ), m_cindexcreate( 0 ) { }
    ~CAutoINDEXCREATE3To3_T();
    ERR ErrSet( JET_INDEXCREATE3_T_FROM * pindexcreate, ULONG cindexcreate = 1 );
    JET_INDEXCREATE3_T_TO* GetPtr(void)     { return m_rgindexcreateEngine; }
    void Result( );
};

template< class JET_INDEXCREATE3_T_FROM, class JET_INDEXCREATE3_T_TO >
ERR CAutoINDEXCREATE3To3_T< JET_INDEXCREATE3_T_FROM, JET_INDEXCREATE3_T_TO >::ErrSet( JET_INDEXCREATE3_T_FROM * pindexcreate, ULONG cindexcreate )
{
    ERR err = JET_errSuccess;

    delete [] m_rgindexcreateEngine;
    m_rgindexcreateEngine = NULL;

    if ( NULL == pindexcreate || 0 == cindexcreate )
    {
        if ( cindexcreate )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }
    else
    {
        Assert( pindexcreate );
        Assert( cindexcreate );

        Alloc( m_rgindexcreateEngine = new JET_INDEXCREATE3_T_TO[cindexcreate] );


        JET_INDEXCREATE3_T_FROM * pidxcreate3 = pindexcreate;
        JET_INDEXCREATE3_T_FROM * pidxcreate3Next;

        for ( ULONG iIdx = 0; iIdx < cindexcreate; iIdx++, pidxcreate3 = pidxcreate3Next )
        {
            pidxcreate3Next = (JET_INDEXCREATE3_T_FROM *)( ((BYTE*)pidxcreate3) + pidxcreate3->cbStruct );

            if ( pidxcreate3->cbStruct != sizeof(*pindexcreate) )
            {
                return ErrERRCheck( JET_errInvalidParameter );
            }

            C_ASSERT( sizeof(*m_rgindexcreateEngine) == sizeof(*pindexcreate) );
            Assert( pidxcreate3->cbStruct <= sizeof(*m_rgindexcreateEngine) );
            memcpy( &(m_rgindexcreateEngine[iIdx]), pidxcreate3, pidxcreate3->cbStruct );

            m_rgindexcreateEngine[iIdx].cbStruct = sizeof(*m_rgindexcreateEngine);


            Assert( m_rgindexcreateEngine[iIdx].pidxunicode == pidxcreate3->pidxunicode );
            Assert( m_rgindexcreateEngine[iIdx].cbStruct == sizeof(m_rgindexcreateEngine[iIdx]) );
        }

    }
    m_rgindexcreateFromAPI = pindexcreate;
    m_cindexcreate = cindexcreate;

HandleError:

    return err;
}

template< class JET_INDEXCREATE3_T_FROM, class JET_INDEXCREATE3_T_TO >
void CAutoINDEXCREATE3To3_T< JET_INDEXCREATE3_T_FROM, JET_INDEXCREATE3_T_TO >::Result( )
{
    JET_INDEXCREATE3_T_FROM * pidxcreateFromAPI = m_rgindexcreateFromAPI;
    for ( ULONG iIdx = 0; iIdx < m_cindexcreate; iIdx++ )
    {
        if ( 0 == ( m_rgindexcreateEngine[iIdx].grbit & JET_bitIndexImmutableStructure ) )
        {
            pidxcreateFromAPI->err = m_rgindexcreateEngine[iIdx].err;
            pidxcreateFromAPI = (JET_INDEXCREATE3_T_FROM *)( ((BYTE*)pidxcreateFromAPI) + pidxcreateFromAPI->cbStruct );
        }
    }
}
template< class JET_INDEXCREATE3_T_FROM, class JET_INDEXCREATE3_T_TO >
CAutoINDEXCREATE3To3_T< JET_INDEXCREATE3_T_FROM, JET_INDEXCREATE3_T_TO >::~CAutoINDEXCREATE3To3_T()
{
    delete [] m_rgindexcreateEngine;
}



template< class JET_TABLECREATE2_T, class JET_TABLECREATE3_T, class CAutoIndex_T >
class CAutoTABLECREATE2To3_T {
    JET_TABLECREATE2_T *    m_ptablecreateFromAPI;
    JET_TABLECREATE3_T      m_tablecreateEngine;
    CAutoIndex_T *          m_pautoindices;
public:
    CAutoTABLECREATE2To3_T() : m_ptablecreateFromAPI(NULL), m_pautoindices(NULL) { }
    ~CAutoTABLECREATE2To3_T();

    ERR ErrSet( JET_TABLECREATE2_T * ptablecreate );

    operator JET_TABLECREATE3_A*()      { return &(m_tablecreateEngine); }
    operator JET_TABLECREATE3_W*()      { return &(m_tablecreateEngine); }

    void Result( );
};


template< class JET_TABLECREATE2_T, class JET_TABLECREATE3_T, class CAutoIndex_T >
ERR CAutoTABLECREATE2To3_T< JET_TABLECREATE2_T, JET_TABLECREATE3_T, CAutoIndex_T >::ErrSet( JET_TABLECREATE2_T * ptablecreate )
{
    ERR err = JET_errSuccess;

    delete m_pautoindices;
    m_pautoindices = NULL;

    if ( ptablecreate->cbStruct != sizeof(*ptablecreate) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    Assert( sizeof(m_tablecreateEngine) > sizeof(*ptablecreate) );
    Assert( sizeof(m_tablecreateEngine) > ptablecreate->cbStruct );

    memcpy( &(m_tablecreateEngine), ptablecreate, sizeof(*ptablecreate) );

    Alloc( m_pautoindices = new CAutoIndex_T() );
    Call( m_pautoindices->ErrSet( ptablecreate->rgindexcreate, ptablecreate->cIndexes ) );
    m_tablecreateEngine.rgindexcreate = m_pautoindices->GetPtr();

    Assert( m_tablecreateEngine.cIndexes == ptablecreate->cIndexes );
    Assert( m_tablecreateEngine.cIndexes > 0 || NULL == m_tablecreateEngine.rgindexcreate );

    m_tablecreateEngine.pSeqSpacehints = NULL;
    m_tablecreateEngine.pLVSpacehints = NULL;
    m_tablecreateEngine.cbSeparateLV = 0;

    m_tablecreateEngine.cbStruct = sizeof(m_tablecreateEngine);

    m_ptablecreateFromAPI = ptablecreate;

HandleError:

    return err;
}

template< class JET_TABLECREATE2_T, class JET_TABLECREATE3_T, class CAutoIndex_T >
void CAutoTABLECREATE2To3_T< JET_TABLECREATE2_T, JET_TABLECREATE3_T, CAutoIndex_T >::Result( )
{
    Assert( m_ptablecreateFromAPI );
    if ( m_ptablecreateFromAPI )
    {


        Assert( m_pautoindices );
        if ( m_pautoindices )
        {
            m_pautoindices->Result();
        }

        if ( ( m_ptablecreateFromAPI->grbit & JET_bitTableCreateImmutableStructure ) == 0 )
        {
            m_ptablecreateFromAPI->tableid  = m_tablecreateEngine.tableid;
            m_ptablecreateFromAPI->cCreated     = m_tablecreateEngine.cCreated;
        }
    }
}

template< class JET_TABLECREATE2_T, class JET_TABLECREATE3_T, class CAutoIndex_T >
CAutoTABLECREATE2To3_T< JET_TABLECREATE2_T, JET_TABLECREATE3_T, CAutoIndex_T >::~CAutoTABLECREATE2To3_T()
{
    delete m_pautoindices;
}


template< class JET_TABLECREATE3_T, class JET_TABLECREATE4_T, class CAutoIndex_T >
class CAutoTABLECREATE3To4_T {
    JET_TABLECREATE3_T *    m_ptablecreateFromAPI;
    JET_TABLECREATE4_T      m_tablecreateEngine;
    CAutoIndex_T *          m_pautoindices;
public:
    CAutoTABLECREATE3To4_T() : m_ptablecreateFromAPI(NULL), m_pautoindices(NULL) { }
    ~CAutoTABLECREATE3To4_T();

    ERR ErrSet( JET_TABLECREATE3_T * ptablecreate );

    operator JET_TABLECREATE4_A*()      { return &(m_tablecreateEngine); }
    operator JET_TABLECREATE4_W*()      { return &(m_tablecreateEngine); }

    void Result( );
};

template< class JET_TABLECREATE3_T, class JET_TABLECREATE4_T, class CAutoIndex_T >
ERR CAutoTABLECREATE3To4_T< JET_TABLECREATE3_T, JET_TABLECREATE4_T, CAutoIndex_T >::ErrSet( JET_TABLECREATE3_T * ptablecreate )
{
    ERR err = JET_errSuccess;

    delete m_pautoindices;
    m_pautoindices = NULL;

    if ( ptablecreate->cbStruct != sizeof(*ptablecreate) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    C_ASSERT( sizeof(m_tablecreateEngine) == sizeof(*ptablecreate) );
    Assert( sizeof(m_tablecreateEngine) == ptablecreate->cbStruct );

    memcpy( &(m_tablecreateEngine), ptablecreate, sizeof(*ptablecreate) );

    Alloc( m_pautoindices = new CAutoIndex_T() );
    Call( m_pautoindices->ErrSet( ptablecreate->rgindexcreate, ptablecreate->cIndexes ) );
    m_tablecreateEngine.rgindexcreate = m_pautoindices->GetPtr();

    Assert( m_tablecreateEngine.cIndexes == ptablecreate->cIndexes );
    Assert( m_tablecreateEngine.cIndexes > 0 || NULL == m_tablecreateEngine.rgindexcreate );

    m_tablecreateEngine.cbStruct = sizeof(m_tablecreateEngine);

    m_ptablecreateFromAPI = ptablecreate;

HandleError:

    return err;
}

template< class JET_TABLECREATE3_T, class JET_TABLECREATE4_T, class CAutoIndex_T >
void CAutoTABLECREATE3To4_T< JET_TABLECREATE3_T, JET_TABLECREATE4_T, CAutoIndex_T >::Result( )
{
    Assert( m_ptablecreateFromAPI );
    if ( m_ptablecreateFromAPI )
    {


        if ( ( m_ptablecreateFromAPI->grbit & JET_bitTableCreateImmutableStructure ) == 0 )
        {
            Assert( m_pautoindices );
            if ( m_pautoindices )
            {
                m_pautoindices->Result();
            }

            m_ptablecreateFromAPI->tableid  = m_tablecreateEngine.tableid;
            m_ptablecreateFromAPI->cCreated     = m_tablecreateEngine.cCreated;
        }
    }
}

template< class JET_TABLECREATE3_T, class JET_TABLECREATE4_T, class CAutoIndex_T >
CAutoTABLECREATE3To4_T< JET_TABLECREATE3_T, JET_TABLECREATE4_T, CAutoIndex_T >::~CAutoTABLECREATE3To4_T()
{
    delete m_pautoindices;
}



template< class JET_TABLECREATE4_T, class JET_TABLECREATE5_T >
class CAutoTABLECREATE4To5_T {
    JET_TABLECREATE4_T *    m_ptablecreateFromAPI;
    JET_TABLECREATE5_T      m_tablecreateEngine;
public:
    CAutoTABLECREATE4To5_T() : m_ptablecreateFromAPI(NULL) { }
    ~CAutoTABLECREATE4To5_T();

    ERR ErrSet( JET_TABLECREATE4_T * ptablecreate );

    operator JET_TABLECREATE5_A*()      { return &(m_tablecreateEngine); }
    operator JET_TABLECREATE5_W*()      { return &(m_tablecreateEngine); }

    void Result( );
};

template< class JET_TABLECREATE4_T, class JET_TABLECREATE5_T >
ERR CAutoTABLECREATE4To5_T< JET_TABLECREATE4_T, JET_TABLECREATE5_T >::ErrSet( JET_TABLECREATE4_T * ptablecreate )
{
    if ( ptablecreate->cbStruct != sizeof(*ptablecreate) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    C_ASSERT( sizeof(m_tablecreateEngine) >= sizeof(*ptablecreate) );
    Assert( sizeof(m_tablecreateEngine) >= ptablecreate->cbStruct );

    memcpy( &(m_tablecreateEngine), ptablecreate, sizeof(*ptablecreate) );

    Assert( m_tablecreateEngine.cIndexes == ptablecreate->cIndexes );

    m_tablecreateEngine.cbLVChunkMax = 0;

    m_tablecreateEngine.cbStruct = sizeof(m_tablecreateEngine);

    m_ptablecreateFromAPI = ptablecreate;

    return JET_errSuccess;
}

template< class JET_TABLECREATE4_T, class JET_TABLECREATE5_T >
void CAutoTABLECREATE4To5_T< JET_TABLECREATE4_T, JET_TABLECREATE5_T >::Result( )
{
    Assert( m_ptablecreateFromAPI );
    if ( m_ptablecreateFromAPI )
    {


        if ( ( m_ptablecreateFromAPI->grbit & JET_bitTableCreateImmutableStructure ) == 0 )
        {
            m_ptablecreateFromAPI->tableid  = m_tablecreateEngine.tableid;
            m_ptablecreateFromAPI->cCreated     = m_tablecreateEngine.cCreated;
        }
    }
}

template< class JET_TABLECREATE4_T, class JET_TABLECREATE5_T >
CAutoTABLECREATE4To5_T< JET_TABLECREATE4_T, JET_TABLECREATE5_T >::~CAutoTABLECREATE4To5_T()
{
}



template< class JET_TABLECREATE5_T_FROM, class JET_TABLECREATE5_T_TO, class CAutoIndex_T >
class CAutoTABLECREATE5To5_T {
    JET_TABLECREATE5_T_FROM *   m_ptablecreateFromAPI;
    JET_TABLECREATE5_T_TO       m_tablecreateEngine;
    CAutoIndex_T *              m_pautoindices;
    JET_COLUMNCREATE_A *        m_rgcolumncreate;
public:
    CAutoTABLECREATE5To5_T() : m_ptablecreateFromAPI(NULL), m_pautoindices(NULL), m_rgcolumncreate(NULL) { }
    ~CAutoTABLECREATE5To5_T();

    ERR ErrSet( JET_TABLECREATE5_T_FROM * ptablecreate );

    operator JET_TABLECREATE5_A*()      { return &(m_tablecreateEngine); }
    operator JET_TABLECREATE5_W*()      { return &(m_tablecreateEngine); }

    void Result( );
};

template< class JET_TABLECREATE5_T_FROM, class JET_TABLECREATE5_T_TO, class CAutoIndex_T >
ERR CAutoTABLECREATE5To5_T< JET_TABLECREATE5_T_FROM, JET_TABLECREATE5_T_TO, CAutoIndex_T >::ErrSet( JET_TABLECREATE5_T_FROM * ptablecreate )
{
    ERR err = JET_errSuccess;

    delete m_pautoindices;
    m_pautoindices = NULL;

    if ( ptablecreate->cbStruct != sizeof(*ptablecreate) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    C_ASSERT( sizeof(m_tablecreateEngine) == sizeof(*ptablecreate) );
    Assert( sizeof(m_tablecreateEngine) == ptablecreate->cbStruct );

    memcpy( &(m_tablecreateEngine), ptablecreate, sizeof(*ptablecreate) );

    if ( ptablecreate->cColumns != 0 )
    {
        Assert( NULL != ptablecreate->rgcolumncreate );
        Alloc( m_rgcolumncreate = new JET_COLUMNCREATE_A[ ptablecreate->cColumns ] );
        memcpy( m_rgcolumncreate, ptablecreate->rgcolumncreate, sizeof( ptablecreate->rgcolumncreate[0] ) * ptablecreate->cColumns );

        m_tablecreateEngine.rgcolumncreate = m_rgcolumncreate;
    }

    Alloc( m_pautoindices = new CAutoIndex_T() );
    Call( m_pautoindices->ErrSet( ptablecreate->rgindexcreate, ptablecreate->cIndexes ) );
    m_tablecreateEngine.rgindexcreate = m_pautoindices->GetPtr();

    Assert( m_tablecreateEngine.cIndexes == ptablecreate->cIndexes );
    Assert( m_tablecreateEngine.cIndexes > 0 || NULL == m_tablecreateEngine.rgindexcreate );

    C_ASSERT( sizeof( *ptablecreate ) == sizeof( m_tablecreateEngine ) );

    Assert( m_tablecreateEngine.cbStruct == sizeof( m_tablecreateEngine ) );
    m_tablecreateEngine.cbStruct = sizeof( m_tablecreateEngine );

    m_ptablecreateFromAPI = ptablecreate;

HandleError:

    return err;
}

template< class JET_TABLECREATE5_T_FROM, class JET_TABLECREATE5_T_TO, class CAutoIndex_T >
void CAutoTABLECREATE5To5_T< JET_TABLECREATE5_T_FROM, JET_TABLECREATE5_T_TO, CAutoIndex_T >::Result( )
{
    Assert( m_ptablecreateFromAPI );
    if ( m_ptablecreateFromAPI )
    {


        if ( ( m_ptablecreateFromAPI->grbit & JET_bitTableCreateImmutableStructure ) == 0 )
        {
            for ( size_t i = 0; i < m_ptablecreateFromAPI->cColumns; ++i )
            {
                m_ptablecreateFromAPI->rgcolumncreate[ i ].columnid = m_tablecreateEngine.rgcolumncreate[ i ].columnid;
                m_ptablecreateFromAPI->rgcolumncreate[ i ].err = m_tablecreateEngine.rgcolumncreate[ i ].err;
            }

            Assert( m_pautoindices );
            if ( m_pautoindices )
            {
                m_pautoindices->Result();
            }

            m_ptablecreateFromAPI->tableid  = m_tablecreateEngine.tableid;
            m_ptablecreateFromAPI->cCreated     = m_tablecreateEngine.cCreated;
        }
    }
}

template< class JET_TABLECREATE5_T_FROM, class JET_TABLECREATE5_T_TO, class CAutoIndex_T >
CAutoTABLECREATE5To5_T< JET_TABLECREATE5_T_FROM, JET_TABLECREATE5_T_TO, CAutoIndex_T >::~CAutoTABLECREATE5To5_T()
{
    delete m_pautoindices;
    delete[] m_rgcolumncreate;
}



template<class LOGINFOMISC_T> void SetCommonLogInfoMiscMembers( const LGFILEHDR * const plgfilehdr, LOGINFOMISC_T * const ploginfomisc )
{
    ploginfomisc->ulGeneration = (ULONG)plgfilehdr->lgfilehdr.le_lGeneration;
    ploginfomisc->signLog = *(JET_SIGNATURE *)&plgfilehdr->lgfilehdr.signLog;
    ploginfomisc->logtimeCreate = *(JET_LOGTIME *)&plgfilehdr->lgfilehdr.tmCreate;
    ploginfomisc->logtimePreviousGeneration = *(JET_LOGTIME *)&plgfilehdr->lgfilehdr.tmPrevGen;
    ploginfomisc->ulFlags = plgfilehdr->lgfilehdr.fLGFlags;
    ploginfomisc->ulVersionMajor = plgfilehdr->lgfilehdr.le_ulMajor;
    ploginfomisc->cbSectorSize = plgfilehdr->lgfilehdr.le_cbSec;
    ploginfomisc->cbHeader = plgfilehdr->lgfilehdr.le_csecHeader;
    ploginfomisc->cbFile = plgfilehdr->lgfilehdr.le_csecLGFile;
    ploginfomisc->cbDatabasePageSize = plgfilehdr->lgfilehdr.le_cbPageSize;
}

template<class LOGINFOMISC_T> ERR ErrSetCommonLogInfoMiscMembers( const LGFILEHDR * const plgfilehdr, void * const pvResult, const ULONG cbMax )
{
    if ( sizeof( LOGINFOMISC_T ) != cbMax )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }
    LOGINFOMISC_T * const ploginfomisc = (LOGINFOMISC_T *)pvResult;
    SetCommonLogInfoMiscMembers<LOGINFOMISC_T>( plgfilehdr, ploginfomisc );
    return JET_errSuccess;
}

LOCAL ERR ErrGetLogInfoMiscFromLgfilehdr( const LGFILEHDR * const plgfilehdr, void * const pvResult, const ULONG cbMax, const ULONG infoLevel )
{
    ERR err = JET_errSuccess;

    if( JET_LogInfoMisc == infoLevel )
    {
        Call( ErrSetCommonLogInfoMiscMembers<JET_LOGINFOMISC>( plgfilehdr, pvResult, cbMax ) );

        JET_LOGINFOMISC2 * const ploginfomisc = (JET_LOGINFOMISC2 *)pvResult;
        ploginfomisc->ulVersionMinor = plgfilehdr->lgfilehdr.le_ulMinor;
        ploginfomisc->ulVersionUpdate = plgfilehdr->lgfilehdr.le_ulUpdateMajor;
    }
    else if( JET_LogInfoMisc2 == infoLevel )
    {
        Call( ErrSetCommonLogInfoMiscMembers<JET_LOGINFOMISC2>( plgfilehdr, pvResult, cbMax ) );

        JET_LOGINFOMISC2 * const ploginfomisc = (JET_LOGINFOMISC2 *)pvResult;
        ploginfomisc->ulVersionMinor = plgfilehdr->lgfilehdr.le_ulMinor;
        ploginfomisc->ulVersionUpdate = plgfilehdr->lgfilehdr.le_ulUpdateMajor;
        ploginfomisc->lgposCheckpoint = *(JET_LGPOS *) &(plgfilehdr->lgfilehdr.le_lgposCheckpoint);
    }
    else if( JET_LogInfoMisc3 == infoLevel )
    {
        Call( ErrSetCommonLogInfoMiscMembers<JET_LOGINFOMISC3>( plgfilehdr, pvResult, cbMax ) );

        JET_LOGINFOMISC3 * const ploginfomisc = (JET_LOGINFOMISC3 *)pvResult;
        ploginfomisc->lgposCheckpoint = *(JET_LGPOS *) &(plgfilehdr->lgfilehdr.le_lgposCheckpoint);
        ploginfomisc->ulVersionUpdateMajor = plgfilehdr->lgfilehdr.le_ulUpdateMajor;
        ploginfomisc->ulVersionUpdateMinor = plgfilehdr->lgfilehdr.le_ulUpdateMinor;
        ploginfomisc->ulVersionMinorDeprecated = plgfilehdr->lgfilehdr.le_ulMinor;
        ploginfomisc->checksumPrevLogAllSegments = plgfilehdr->lgfilehdr.checksumPrevLogAllSegments;
    }
    else
    {
        Assert( false );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

HandleError:
    return err;
}

void BFIMaintLowMemoryCallback( DWORD_PTR pvUnused );

LOCAL QWORD g_qwMarkerID = 0;

LONG LProgramMarkerIdCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf )
        *( (QWORD*) pvBuf ) = g_qwMarkerID;

    return 0;
}

extern "C" {










LOCAL JET_ERR JetIdleEx( __in JET_SESID sesid, __in JET_GRBIT grbit )
{
    APICALL_SESID   apicall( opIdle );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x)",
            __FUNCTION__,
            sesid,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamIdle( sesid, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetIdle( __in JET_SESID sesid, __in JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opIdle, JetIdleEx( sesid, grbit ) );
}


LOCAL JET_ERR JetGetTableIndexInfoEx(
    __in JET_SESID                  sesid,
    __in JET_TABLEID                tableid,
    __in_opt JET_PCSTR              szIndexName,
    __out_bcount( cbResult ) void * pvResult,
    __in ULONG              cbResult,
    __in ULONG              InfoLevel,
    __in const BOOL                 fUnicodeNames )
{
    ERR             err;
    ULONG           cbMin;
    APICALL_SESID   apicall( opGetTableIndexInfo );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p[%s],0x%p,0x%x,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            szIndexName,
            OSFormatString( szIndexName ),
            pvResult,
            cbResult,
            InfoLevel,
            fUnicodeNames ) );

    if ( !apicall.FEnter( sesid ) )
        return apicall.ErrResult();

    ProbeClientBuffer( pvResult, cbResult );

    switch( InfoLevel )
    {
        case JET_IdxInfo:
        case JET_IdxInfoList:
            cbMin = sizeof(JET_INDEXLIST) - cbIDXLISTNewMembersSinceOriginalFormat;
            break;
        case JET_IdxInfoSpaceAlloc:
        case JET_IdxInfoCount:
            cbMin = sizeof(ULONG);
            break;
        case JET_IdxInfoLangid:
            cbMin = sizeof(LANGID);
            break;
        case JET_IdxInfoVarSegMac:
        case JET_IdxInfoKeyMost:
            cbMin = sizeof(USHORT);
            break;
        case JET_IdxInfoIndexId:
            cbMin = sizeof(INDEXID);
            break;
        case JET_IdxInfoCreateIndex:
        case JET_IdxInfoCreateIndex2:
        case JET_IdxInfoCreateIndex3:
            cbMin = ( InfoLevel == JET_IdxInfoCreateIndex3 ) ?
                        ( fUnicodeNames ? sizeof(JET_INDEXCREATE3_W) : sizeof(JET_INDEXCREATE3_A) ) :
                        ( ( InfoLevel == JET_IdxInfoCreateIndex2 ) ?
                            ( fUnicodeNames ? sizeof(JET_INDEXCREATE2_W) : sizeof(JET_INDEXCREATE2_A) ) :
                            ( fUnicodeNames ? sizeof(JET_INDEXCREATE_W) : sizeof(JET_INDEXCREATE_A) ) );
            break;

        case JET_IdxInfoSortVersion:
        case JET_IdxInfoDefinedSortVersion:
            cbMin = sizeof(DWORD);
            break;
        case JET_IdxInfoSortId:
            cbMin = sizeof(SORTID);
            break;

        case JET_IdxInfoSysTabCursor:
        case JET_IdxInfoOLC:
        case JET_IdxInfoResetOLC:
            FireWall( OSFormat( "FeatureNAGetTableIdxInfo:%d", (INT)InfoLevel ) );
            Error( ErrERRCheck( JET_errFeatureNotAvailable ) );

        case JET_IdxInfoLocaleName:
            cbMin = NORM_LOCALE_NAME_MAX_LENGTH * sizeof(WCHAR);
            break;

        default:
            Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( cbResult >= cbMin )
    {
        err = ErrDispGetTableIndexInfo(
                    sesid,
                    tableid,
                    szIndexName,
                    pvResult,
                    cbResult,
                    InfoLevel,
                    fUnicodeNames );
    }
    else
    {
        err = ErrERRCheck( JET_errBufferTooSmall );
    }

HandleError:
    apicall.LeaveAfterCall( err );
    return apicall.ErrResult();
}

LOCAL JET_ERR JetGetTableIndexInfoExW(
    __in JET_SESID                  sesid,
    __in JET_TABLEID                tableid,
    __in_opt JET_PCWSTR             wszIndexName,
    __out_bcount( cbResult ) void * pvResult,
    __in ULONG              cbResult,
    __in ULONG              InfoLevel )
{
    ERR         err             = JET_errSuccess;
    CAutoSZDDL  lszIndexName;

    CallR( lszIndexName.ErrSet( wszIndexName ) );

    return JetGetTableIndexInfoEx( sesid, tableid, lszIndexName, pvResult, cbResult, InfoLevel, fTrue );
}

JET_ERR JET_API JetGetTableIndexInfoA(
    __in JET_SESID                  sesid,
    __in JET_TABLEID                tableid,
    __in_opt JET_PCSTR              szIndexName,
    __out_bcount( cbResult ) void * pvResult,
    __in ULONG              cbResult,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetTableIndexInfo, JetGetTableIndexInfoEx( sesid, tableid, szIndexName, pvResult, cbResult, InfoLevel, fFalse ) );
}

JET_ERR JET_API JetGetTableIndexInfoW(
    __in JET_SESID                  sesid,
    __in JET_TABLEID                tableid,
    __in_opt JET_PCWSTR             wszIndexName,
    __out_bcount( cbResult ) void * pvResult,
    __in ULONG              cbResult,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetTableIndexInfo, JetGetTableIndexInfoExW( sesid, tableid, wszIndexName, pvResult, cbResult, InfoLevel ) );
}


LOCAL JET_ERR JetGetIndexInfoEx(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_PCSTR                  szTableName,
    __in_opt JET_PCSTR              szIndexName,
    __out_bcount( cbResult ) void * pvResult,
    __in ULONG              cbResult,
    __in ULONG              InfoLevel,
    __in const BOOL                 fUnicodeNames )
{
    ERR             err;
    ULONG           cbMin;
    APICALL_SESID   apicall( opGetIndexInfo );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p[%s],0x%p,0x%x,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            szIndexName,
            OSFormatString( szIndexName ),
            pvResult,
            cbResult,
            InfoLevel,
            fUnicodeNames ) );

    if ( !apicall.FEnter( sesid, dbid ) )
        return apicall.ErrResult();

    ProbeClientBuffer( pvResult, cbResult );

    switch( InfoLevel )
    {
        case JET_IdxInfo:
        case JET_IdxInfoList:
            cbMin = sizeof(JET_INDEXLIST) - cbIDXLISTNewMembersSinceOriginalFormat;
            break;
        case JET_IdxInfoSpaceAlloc:
        case JET_IdxInfoCount:
            cbMin = sizeof(ULONG);
            break;
        case JET_IdxInfoLangid:
            cbMin = sizeof(LANGID);
            break;
        case JET_IdxInfoLocaleName:
            cbMin = NORM_LOCALE_NAME_MAX_LENGTH * sizeof(WCHAR);
            break;
        case JET_IdxInfoVarSegMac:
        case JET_IdxInfoKeyMost:
            cbMin = sizeof(USHORT);
            break;
        case JET_IdxInfoIndexId:
            cbMin = sizeof(INDEXID);
            break;
        case JET_IdxInfoCreateIndex:
        case JET_IdxInfoCreateIndex2:
        case JET_IdxInfoCreateIndex3:
            cbMin = ( InfoLevel == JET_IdxInfoCreateIndex3 ) ?
                        ( fUnicodeNames ? sizeof(JET_INDEXCREATE3_W) : sizeof(JET_INDEXCREATE3_A) ) :
                        ( ( InfoLevel == JET_IdxInfoCreateIndex2 ) ?
                            ( fUnicodeNames ? sizeof(JET_INDEXCREATE2_W) : sizeof(JET_INDEXCREATE2_A) ) :
                            ( fUnicodeNames ? sizeof(JET_INDEXCREATE_W) : sizeof(JET_INDEXCREATE_A) ) );
            break;
        case JET_IdxInfoSortVersion:
        case JET_IdxInfoDefinedSortVersion:
            cbMin = sizeof(DWORD);
            break;
        case JET_IdxInfoSortId:
            cbMin = sizeof(SORTID);
            break;

        case JET_IdxInfoSysTabCursor:
        case JET_IdxInfoOLC:
        case JET_IdxInfoResetOLC:
            FireWall( OSFormat( "FeatureNAGetIdxInfo:%d", (INT)InfoLevel ) );
            Error( ErrERRCheck( JET_errFeatureNotAvailable ) );

        default :
            Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( cbResult >= cbMin )
    {
        err = ErrIsamGetIndexInfo(
                    sesid,
                    dbid,
                    szTableName,
                    szIndexName,
                    pvResult,
                    cbResult,
                    InfoLevel,
                    fUnicodeNames );
    }
    else
    {
        err = ErrERRCheck( JET_errBufferTooSmall );
    }

HandleError:
    apicall.LeaveAfterCall( err );
    return apicall.ErrResult();
}

LOCAL JET_ERR JetGetIndexInfoExW(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_PCWSTR                 wszTableName,
    __in_opt JET_PCWSTR             wszIndexName,
    __out_bcount( cbResult ) void * pvResult,
    __in ULONG              cbResult,
    __in ULONG              InfoLevel )
{
    ERR         err         = JET_errSuccess;
    CAutoSZDDL  lszTableName;
    CAutoSZDDL  lszIndexName;

    CallR( lszTableName.ErrSet( wszTableName ) );
    CallR( lszIndexName.ErrSet( wszIndexName ) );

    return JetGetIndexInfoEx( sesid, dbid, lszTableName, lszIndexName, pvResult, cbResult, InfoLevel, fTrue );
}

JET_ERR JET_API JetGetIndexInfo(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_PCSTR                  szTableName,
    __in_opt JET_PCSTR              szIndexName,
    __out_bcount( cbResult ) void * pvResult,
    __in ULONG              cbResult,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetIndexInfo, JetGetIndexInfoEx( sesid, dbid, szTableName, szIndexName, pvResult, cbResult, InfoLevel, fFalse ) );
}

JET_ERR JET_API JetGetIndexInfoW(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_PCWSTR                 wszTableName,
    __in_opt JET_PCWSTR             wszIndexName,
    __out_bcount( cbResult ) void * pvResult,
    __in ULONG              cbResult,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetIndexInfo, JetGetIndexInfoExW( sesid, dbid, wszTableName, wszIndexName, pvResult, cbResult, InfoLevel ) );
}

LOCAL JET_ERR JetGetObjectInfoEx(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_OBJTYP                 objtyp,
    __in JET_PCSTR                  szContainerName,
    __in_opt JET_PCSTR              szObjectName,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel,
    __in const BOOL                 fUnicodeNames )
{
    ERR             err;
    ULONG           cbMin;
    APICALL_SESID   apicall( opGetObjectInfo );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%x,0x%p[%s],0x%p[%s],0x%p,0x%x,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            objtyp,
            szContainerName,
            OSFormatString( szContainerName ),
            szObjectName,
            OSFormatString( szObjectName ),
            pvResult,
            cbMax,
            InfoLevel,
            fUnicodeNames ) );

    if ( !apicall.FEnter( sesid, dbid ) )
        return apicall.ErrResult();

    ProbeClientBuffer( pvResult, cbMax );

    switch( InfoLevel )
    {
        case JET_ObjInfo:
        case JET_ObjInfoNoStats:
            cbMin = sizeof(JET_OBJECTINFO);
            break;
        case JET_ObjInfoListNoStats:
        case JET_ObjInfoList:
            cbMin = sizeof(JET_OBJECTLIST);
            break;

        case JET_ObjInfoSysTabCursor:
        case JET_ObjInfoSysTabReadOnly:
        case JET_ObjInfoListACM:
        case JET_ObjInfoRulesLoaded:
            FireWall( OSFormat( "FeatureNAGetObjInfo:%d", (INT)InfoLevel ) );
            Error( ErrERRCheck( JET_errFeatureNotAvailable ) );

        default:
            Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( cbMax >= cbMin )
    {
        err = ErrIsamGetObjectInfo(
                    sesid,
                    dbid,
                    objtyp,
                    szContainerName,
                    szObjectName,
                    pvResult,
                    cbMax,
                    InfoLevel,
                    fUnicodeNames );
    }
    else
    {
        err = ErrERRCheck( JET_errBufferTooSmall );
    }

HandleError:
    apicall.LeaveAfterCall( err );
    return apicall.ErrResult();
}

LOCAL JET_ERR JetGetObjectInfoExW(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_OBJTYP                 objtyp,
    __in_opt JET_PCWSTR             wszContainerName,
    __in_opt JET_PCWSTR             wszObjectName,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    ERR         err                 = JET_errSuccess;
    CAutoSZDDL  lszContainerName;
    CAutoSZDDL  lszObjectName;

    CallR( lszContainerName.ErrSet( wszContainerName ) );
    CallR( lszObjectName.ErrSet( wszObjectName ) );

    return JetGetObjectInfoEx( sesid, dbid, objtyp, lszContainerName, lszObjectName, pvResult, cbMax, InfoLevel, fTrue );
}

JET_ERR JET_API JetGetObjectInfoA(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_OBJTYP                 objtyp,
    __in_opt JET_PCSTR              szContainerName,
    __in_opt JET_PCSTR              szObjectName,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetObjectInfo, JetGetObjectInfoEx( sesid, dbid, objtyp, szContainerName, szObjectName, (void*) pvResult, cbMax, InfoLevel, fFalse ) );
}

JET_ERR JET_API JetGetObjectInfoW(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_OBJTYP                 objtyp,
    __in_opt JET_PCWSTR             wszContainerName,
    __in_opt JET_PCWSTR             wszObjectName,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetObjectInfo, JetGetObjectInfoExW( sesid, dbid, objtyp, wszContainerName, wszObjectName, (void*) pvResult, cbMax, InfoLevel ) );
}


LOCAL JET_ERR JetGetTableInfoEx(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    void            *pvResult,
    ULONG   cbMax,
    ULONG   InfoLevel )
{
    APICALL_SESID   apicall( opGetTableInfo );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            pvResult,
            cbMax,
            InfoLevel ) );

    if ( apicall.FEnter( sesid ) )
    {
        ProbeClientBuffer( pvResult, cbMax );
        apicall.LeaveAfterCall( ErrDispGetTableInfo(
                                        sesid,
                                        tableid,
                                        pvResult,
                                        cbMax,
                                        InfoLevel ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetGetTableInfoExW(
    __in JET_SESID                  sesid,
    __in JET_TABLEID                tableid,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    ERR             err = JET_errSuccess;
    void *          pv  = NULL;
    ULONG   cb  = 0;

    CAutoSZDDL      lszName;

    switch( InfoLevel )
    {
        case JET_TblInfoName:
        case JET_TblInfoMostMany:
        case JET_TblInfoTemplateTableName:

            CallR( lszName.ErrAlloc( cbMax ) );

            pv = lszName.Pv();
            cb = lszName.Cb();
            break;
        default:
            pv = pvResult;
            cb = cbMax;
            break;
    }

    CallR( JetGetTableInfoEx( sesid, tableid, pv, cb, InfoLevel ) );

    switch( InfoLevel )
    {
        case JET_TblInfoName:
        case JET_TblInfoMostMany:
        case JET_TblInfoTemplateTableName:
            err = lszName.ErrGet( (WCHAR*)pvResult, cbMax );
            break;

        default:
            break;
    }

    return err;
}

JET_ERR JET_API JetGetTableInfoA(
    __in JET_SESID                  sesid,
    __in JET_TABLEID                tableid,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetTableInfo, JetGetTableInfoEx( sesid, tableid, pvResult, cbMax, InfoLevel ) );
}

JET_ERR JET_API JetGetTableInfoW(
    __in JET_SESID                  sesid,
    __in JET_TABLEID                tableid,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetTableInfo, JetGetTableInfoExW( sesid, tableid, pvResult, cbMax, InfoLevel ) );
}

LOCAL JET_ERR JET_API JetSetTableInfoEx(
    JET_SESID sesid,
    JET_VTID vtid,
    _In_reads_bytes_opt_(cbParam) const void  *pvParam,
    ULONG cbParam,
    ULONG InfoLevel)
{
    APICALL_SESID   apicall( opSetTableInfo );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            vtid,
            pvParam,
            cbParam,
            InfoLevel ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispSetTableInfo(
                                        sesid,
                                        vtid,
                                        pvParam,
                                        cbParam,
                                        InfoLevel ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetSetTableInfoA(
    _In_opt_ JET_SESID sesid,
    _In_ JET_VTID vtid,
    _In_reads_bytes_opt_(cbParam) const void  *pvParam,
    _In_ ULONG cbParam,
    _In_ ULONG InfoLevel)
{
    JET_VALIDATE_SESID_TABLEID( sesid, vtid );
    JET_TRY( opSetTableInfo, JetSetTableInfoEx( sesid, vtid, pvParam, cbParam, InfoLevel ) );
}

LOCAL JET_ERR JET_API JetCreateEncryptionKeyEx(
    _In_ ULONG                                      encryptionAlgorithm,
    _Out_writes_bytes_to_opt_( cbKey, *pcbActual ) void *   pvKey,
    _In_ ULONG                                      cbKey,
    _Out_opt_ ULONG *                               pcbActual )
{
    if ( encryptionAlgorithm != JET_EncryptionAlgorithmAes256 )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    *pcbActual = cbKey;
    return ErrOSCreateAes256Key( (BYTE*)pvKey, pcbActual );
}

JET_ERR JET_API JetCreateEncryptionKey(
    _In_ ULONG                                      encryptionAlgorithm,
    _Out_writes_bytes_to_opt_( cbKey, *pcbActual ) void *   pvKey,
    _In_ ULONG                                      cbKey,
    _Out_opt_ ULONG *                               pcbActual )
{
    JET_TRY( opCreateEncryptionKey, JetCreateEncryptionKeyEx( encryptionAlgorithm, pvKey, cbKey, pcbActual ) );
}

LOCAL JET_ERR JetBeginTransactionEx( __in JET_SESID sesid, __in TRXID trxid, __in JET_GRBIT grbit )
{
    APICALL_SESID   apicall( opBeginTransaction );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x)",
            __FUNCTION__,
            sesid,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamBeginTransaction( sesid, trxid, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetBeginTransaction( __in JET_SESID sesid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opBeginTransaction, JetBeginTransactionEx( sesid, 57573, NO_GRBIT ) );
}
JET_ERR JET_API JetBeginTransaction2( __in JET_SESID sesid, __in JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opBeginTransaction, JetBeginTransactionEx( sesid, 32997, grbit ) );
}
JET_ERR JET_API JetBeginTransaction3( __in JET_SESID sesid, __in __int64 trxid, __in JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opBeginTransaction, JetBeginTransactionEx( sesid, trxid, grbit ) );
}


LOCAL JET_ERR JetPrepareToCommitTransactionEx(
    __in JET_SESID                      sesid,
    __in_bcount( cbData ) const void *  pvData,
    __in ULONG                  cbData,
    __in JET_GRBIT                      grbit )
{
    APICALL_SESID   apicall( opPrepareToCommitTransaction );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            pvData,
            cbData,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrERRCheck( JET_errFeatureNotAvailable ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetPrepareToCommitTransaction(
    __in JET_SESID                      sesid,
    __in_bcount( cbData ) const void *  pvData,
    __in ULONG                  cbData,
    __in JET_GRBIT                      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opPrepareToCommitTransaction, JetPrepareToCommitTransactionEx( sesid, pvData, cbData, grbit ) );
}


LOCAL JET_ERR JetCommitTransactionEx( __in JET_SESID sesid, __in JET_GRBIT grbit, __in DWORD cmsecDurableCommit, __out_opt JET_COMMIT_ID *pCommitId )
{
    APICALL_SESID   apicall( opCommitTransaction );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x)",
            __FUNCTION__,
            sesid,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamCommitTransaction( sesid, grbit, cmsecDurableCommit, pCommitId ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetCommitTransaction( __in JET_SESID sesid, __in JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCommitTransaction, JetCommitTransactionEx( sesid, grbit, 0, NULL ) );
}

JET_ERR JET_API
JetCommitTransaction2(
    _In_ JET_SESID              sesid,
    _In_ JET_GRBIT              grbit,
    _In_ ULONG          cmsecDurableCommit,
    _Out_opt_ JET_COMMIT_ID *   pCommitId )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCommitTransaction2, JetCommitTransactionEx( sesid, grbit, cmsecDurableCommit, pCommitId ) );
}


LOCAL JET_ERR JetRollbackEx( __in JET_SESID sesid, __in JET_GRBIT grbit )
{
    APICALL_SESID   apicall( opRollback );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x)",
            __FUNCTION__,
            sesid,
            grbit ) );

    if ( apicall.FEnter( sesid, fTrue ) )
    {
        apicall.LeaveAfterCall( grbit & ~( JET_bitRollbackAll | JET_bitRollbackRedoCallback ) ?
                                        ErrERRCheck( JET_errInvalidGrbit ) :
                                        ErrIsamRollback( sesid, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetRollback( __in JET_SESID sesid, __in JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opRollback, JetRollbackEx( sesid, grbit ) );
}


LOCAL JET_ERR JetGetSessionInfoEx(
    JET_SESID       sesid,
    VOID *          pvResult,
    const ULONG     cbMax,
    const ULONG     ulInfoLevel )
{
    APICALL_SESID   apicall( opGetSessionInfo );

    if ( apicall.FEnter( sesid, fTrue ) )
    {
        apicall.LeaveAfterCall( ErrIsamGetSessionInfo( sesid, pvResult, cbMax, ulInfoLevel ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetGetSessionInfo(
    __in JET_SESID                  sesid,
    __out_bcount( cbMax ) void *    pvResult,
    __in const ULONG        cbMax,
    __in const ULONG        ulInfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetSessionInfo, JetGetSessionInfoEx( sesid, (void*) pvResult, cbMax, ulInfoLevel ) );
}


LOCAL JET_ERR JetOpenTableEx(
    __in JET_SESID                                  sesid,
    __in JET_DBID                                   dbid,
    __in JET_PCSTR                                  szTableName,
    __in_bcount_opt( cbParameters ) const void *    pvParameters,
    __in ULONG                              cbParameters,
    __in JET_GRBIT                                  grbit,
    __out JET_TABLEID *                             ptableid )
{
    APICALL_SESID   apicall( opOpenTable );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p[%s],0x%p,0x%x,0x%x,0x%p)",
            __FUNCTION__,
            sesid,
            dbid,
            szTableName,
            OSFormatString( szTableName ),
            pvParameters,
            cbParameters,
            grbit,
            ptableid ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        apicall.LeaveAfterCall( ErrIsamOpenTable(
                                        sesid,
                                        dbid,
                                        ptableid,
                                        szTableName,
                                        grbit ) );
    }

    return apicall.ErrResult();
}


LOCAL JET_ERR JetOpenTableExW(
    __in JET_SESID                                  sesid,
    __in JET_DBID                                   dbid,
    __in JET_PCWSTR                                 wszTableName,
    __in_bcount_opt( cbParameters ) const void *    pvParameters,
    __in ULONG                              cbParameters,
    __in JET_GRBIT                                  grbit,
    __out JET_TABLEID *                             ptableid )
{
    ERR     err;
    CAutoSZDDL lszTableName;

    CallR( lszTableName.ErrSet( wszTableName ) );

    return JetOpenTableEx( sesid, dbid, lszTableName, pvParameters, cbParameters, grbit, ptableid );
}

JET_ERR JET_API JetOpenTableA(
    __in JET_SESID                                  sesid,
    __in JET_DBID                                   dbid,
    __in JET_PCSTR                                  szTableName,
    __in_bcount_opt( cbParameters ) const void *    pvParameters,
    __in ULONG                              cbParameters,
    __in JET_GRBIT                                  grbit,
    __out JET_TABLEID *                             ptableid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOpenTable, JetOpenTableEx( sesid, dbid, szTableName, pvParameters, cbParameters, grbit, ptableid ) );
}


JET_ERR JET_API JetOpenTableW(
    __in JET_SESID                                  sesid,
    __in JET_DBID                                   dbid,
    __in JET_PCWSTR                                 wszTableName,
    __in_bcount_opt( cbParameters ) const void *    pvParameters,
    __in ULONG                              cbParameters,
    __in JET_GRBIT                                  grbit,
    __out JET_TABLEID *                             ptableid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOpenTable, JetOpenTableExW( sesid, dbid, wszTableName, pvParameters, cbParameters, grbit, ptableid ) );
}

LOCAL JET_ERR JetSetTableSequentialEx(
    __in const JET_SESID    sesid,
    __in const JET_TABLEID  tableid,
    __in const JET_GRBIT    grbit )
{
    APICALL_SESID       apicall( opSetTableSequential );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispSetTableSequential( sesid, tableid, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetSetTableSequential(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetTableSequential, JetSetTableSequentialEx( sesid, tableid, grbit ) );
}


LOCAL JET_ERR JetResetTableSequentialEx(
    __in JET_SESID      sesid,
    __in JET_TABLEID        tableid,
    __in JET_GRBIT      grbit )
{
    APICALL_SESID   apicall( opResetTableSequential );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispResetTableSequential( sesid, tableid, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetResetTableSequential(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opResetTableSequential, JetResetTableSequentialEx( sesid, tableid, grbit ) );
}

LOCAL JET_ERR JetRegisterCallbackEx(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_CBTYP      cbtyp,
    __in JET_CALLBACK   pCallback,
    __in_opt void *     pvContext,
    __in JET_HANDLE *   phCallbackId )
{
    APICALL_SESID   apicall( opRegisterCallback );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x,0x%p,0x%p,0x%p)",
            __FUNCTION__,
            sesid,
            tableid,
            cbtyp,
            pCallback,
            pvContext,
            phCallbackId ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispRegisterCallback(
                                        sesid,
                                        tableid,
                                        cbtyp,
                                        pCallback,
                                        pvContext,
                                        phCallbackId ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetRegisterCallback(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_CBTYP      cbtyp,
    __in JET_CALLBACK   pCallback,
    __in_opt void *     pvContext,
    __in JET_HANDLE *   phCallbackId )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opRegisterCallback, JetRegisterCallbackEx( sesid, tableid, cbtyp, pCallback, pvContext, phCallbackId ) );
}


LOCAL JET_ERR JetUnregisterCallbackEx(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_CBTYP      cbtyp,
    __in JET_HANDLE     hCallbackId )
{
    APICALL_SESID   apicall( opUnregisterCallback );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x,0x%Ix)",
            __FUNCTION__,
            sesid,
            tableid,
            cbtyp,
            hCallbackId ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispUnregisterCallback(
                                        sesid,
                                        tableid,
                                        cbtyp,
                                        hCallbackId ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetUnregisterCallback(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_CBTYP      cbtyp,
    __in JET_HANDLE     hCallbackId )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opUnregisterCallback, JetUnregisterCallbackEx( sesid, tableid, cbtyp, hCallbackId ) );
}


LOCAL JET_ERR JetSetLSEx(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_LS         ls,
    __in JET_GRBIT      grbit )
{
    APICALL_SESID   apicall( opSetLS );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%Ix,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            ls,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispSetLS( sesid, tableid, ls, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetSetLS(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_LS         ls,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetLS, JetSetLSEx( sesid, tableid, ls, grbit ) );
}


LOCAL JET_ERR JetGetLSEx(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __out JET_LS *      pls,
    __in JET_GRBIT      grbit )
{
    APICALL_SESID   apicall( opGetLS );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            pls,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispGetLS( sesid, tableid, pls, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetGetLS(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __out JET_LS *      pls,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetLS, JetGetLSEx( sesid, tableid, pls, grbit ) );
}

LOCAL JET_ERR JetTracingEx(
    __in const JET_TRACEOP  traceop,
    __in const JET_TRACETAG tracetag,
    __in const JET_API_PTR  ul )
{
    JET_ERR             err     = JET_errSuccess;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%x,0x%x,0x%Ix)",
            __FUNCTION__,
            traceop,
            tracetag,
            ul ) );

#ifdef MINIMAL_FUNCTIONALITY

    Error( ErrERRCheck( JET_errDisabledFunctionality ) );

#else

    switch ( traceop )
    {
        case JET_traceopSetGlobal:
            OSTraceSetGlobal( !!ul );
            break;

        case JET_traceopSetTag:
            if ( tracetag > JET_tracetagNull && tracetag < JET_tracetagMax )
            {
                OSTraceSetTag( tracetag, !!ul );
            }
            else
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
            break;

        case JET_traceopSetAllTags:
            for ( TRACETAG itag = JET_tracetagNull + 1; itag < JET_tracetagMax; itag++ )
            {
                OSTraceSetTag( itag, !!ul );
            }
            break;

        case JET_traceopSetMessagePrefix:
            break;

        case JET_traceopRegisterTag:
            if ( 0 != ul && tracetag > JET_tracetagNull && tracetag < JET_tracetagMax )
            {
                ULONG_PTR   ulT;

                CAutoISZ<JET_tracetagDescCbMax> szTraceDesc;

                err = szTraceDesc.ErrSet( g_rgwszTraceDesc[tracetag] );
                CallS( err );
                Call( err );

                (*(JET_PFNTRACEREGISTER)ul)( tracetag, szTraceDesc, &ulT );

                OSTraceSetTagData( tracetag, ulT );
            }
            else
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
            break;

        case JET_traceopRegisterAllTags:
            if ( 0 != ul )
            {
                for ( TRACETAG itag = JET_tracetagNull + 1; itag < JET_tracetagMax; itag++ )
                {
                    ULONG_PTR   ulT;

                    CAutoISZ<JET_tracetagDescCbMax> szTraceDesc;

                    err = szTraceDesc.ErrSet( g_rgwszTraceDesc[tracetag] );
                    CallS( err );
                    Call( err );

                    (*(JET_PFNTRACEREGISTER)ul)( (JET_TRACETAG)itag, szTraceDesc, &ulT );

                    OSTraceSetTagData( itag, ulT );
                }
            }
            else
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
            break;

        case JET_traceopSetEmitCallback:
            OSTraceRegisterEmitCallback( (PFNTRACEEMIT)ul );
            break;

        case JET_traceopSetThreadidFilter:
            OSTraceSetThreadidFilter( (DWORD)ul );
            break;

        case JET_traceopSetDbidFilter:
            g_ifmpTrace = IFMP( ul );
            break;

        default:
            Error( ErrERRCheck( JET_errInvalidParameter ) );
            break;
    }

#endif

HandleError:
    return err;
}
JET_ERR JET_API JetTracing(
    __in const JET_TRACEOP  traceop,
    __in const JET_TRACETAG tracetag,
    __in const JET_API_PTR  ul )
{
    JET_TRY( opTracing, JetTracingEx( traceop, tracetag, ul ) );
}


LOCAL JET_ERR JetDupCursorEx(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __out JET_TABLEID * ptableid,
    __in JET_GRBIT      grbit )
{
    APICALL_SESID   apicall( opDupCursor );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            ptableid,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispDupCursor( sesid, tableid, ptableid, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetDupCursor(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __out JET_TABLEID * ptableid,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDupCursor, JetDupCursorEx( sesid, tableid, ptableid, grbit ) );
}


LOCAL JET_ERR JetCloseTableEx( __in JET_SESID sesid, __in JET_TABLEID tableid )
{
    APICALL_SESID   apicall( opCloseTable );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix)",
            __FUNCTION__,
            sesid,
            tableid ) );

    if ( apicall.FEnter( sesid, fTrue ) )
    {
        apicall.LeaveAfterCall( ErrDispCloseTable( sesid, tableid ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetCloseTable( __in JET_SESID sesid, __in JET_TABLEID tableid )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCloseTable, JetCloseTableEx( sesid, tableid ) );
}

const ULONG     maskGetColumInfoGrbits      = ( JET_ColInfoGrbitNonDerivedColumnsOnly | JET_ColInfoGrbitMinimalInfo | JET_ColInfoGrbitSortByColumnid | JET_ColInfoGrbitCompacting );
const ULONG     maskGetColumInfoLevelBase   = ~maskGetColumInfoGrbits;


LOCAL JET_ERR JetGetTableColumnInfoEx(
    __in JET_SESID                  sesid,
    __in JET_TABLEID                tableid,
    __in_opt JET_PCSTR              pColumnNameOrId,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel,
    __in const BOOL                 fUnicodeNames )
{
    ERR             err;
    ULONG           cbMin;
    const BOOL      fByColid        = ( ( InfoLevel & maskGetColumInfoLevelBase ) == JET_ColInfoByColid
                                        || ( InfoLevel & maskGetColumInfoLevelBase ) == JET_ColInfoBaseByColid );
    APICALL_SESID   apicall( opGetTableColumnInfo );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p[%s],0x%p,0x%x,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            pColumnNameOrId,
            ( fByColid ? OSFormat( "<0x%x>", *(JET_COLUMNID *)pColumnNameOrId ) : OSFormatString( pColumnNameOrId ) ),
            pvResult,
            cbMax,
            InfoLevel,
            fUnicodeNames ) );

    if ( !apicall.FEnter( sesid ) )
        return apicall.ErrResult();

    ProbeClientBuffer( pvResult, cbMax );

    const ULONG InfoLevelT = InfoLevel & maskGetColumInfoLevelBase;
    switch( InfoLevelT )
    {
        case JET_ColInfo:
        case JET_ColInfoByColid:
            cbMin = sizeof(JET_COLUMNDEF);
            break;
        case JET_ColInfoList:
        case JET_ColInfoListCompact:
        case JET_ColInfoListSortColumnid:
            cbMin = sizeof(JET_COLUMNLIST);
            break;
        case JET_ColInfoBase:
        case JET_ColInfoBaseByColid:
            cbMin = sizeof(JET_COLUMNBASE_A);
            break;

        case JET_ColInfoSysTabCursor:
            FireWall( OSFormat( "FeatureNAGetTableColInfo:%d", (INT)InfoLevelT ) );
            Error( ErrERRCheck( JET_errFeatureNotAvailable ) );

        default:
            Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( cbMax >= cbMin )
    {
        if ( fByColid )
        {
            err = ErrDispGetTableColumnInfo(
                            sesid,
                            tableid,
                            NULL,
                            (JET_COLUMNID *)pColumnNameOrId,
                            pvResult,
                            cbMax,
                            InfoLevel,
                            fUnicodeNames );
        }
        else
        {
            err = ErrDispGetTableColumnInfo(
                            sesid,
                            tableid,
                            pColumnNameOrId,
                            NULL,
                            pvResult,
                            cbMax,
                            InfoLevel,
                            fUnicodeNames );
        }
    }
    else
    {
        err = ErrERRCheck( JET_errBufferTooSmall );
    }

HandleError:
    apicall.LeaveAfterCall( err );
    return apicall.ErrResult();
}

class CAutoCOLUMNBASEA
{
    public:
        CAutoCOLUMNBASEA():m_pColumnBase( NULL ) { }
        ~CAutoCOLUMNBASEA();

    public:
        ERR ErrSet( const JET_COLUMNBASE_W * pColumnBase );
        ERR ErrGet( JET_COLUMNBASE_W * pColumnBase );
        operator JET_COLUMNBASE_A*()        { return m_pColumnBase; }

    private:
        JET_COLUMNBASE_A *  m_pColumnBase;
};

ERR CAutoCOLUMNBASEA::ErrSet( const JET_COLUMNBASE_W * pColumnBase )
{
    ERR                 err         = JET_errSuccess;

    delete m_pColumnBase;
    m_pColumnBase = NULL;

    if ( pColumnBase )
    {
        Alloc( m_pColumnBase = new JET_COLUMNBASE_A );
        memset( m_pColumnBase, 0, sizeof(JET_COLUMNBASE_A) );

        m_pColumnBase->cbStruct = sizeof( JET_COLUMNBASE_A );
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        delete m_pColumnBase;
        m_pColumnBase = NULL;
    }

    return err;
}

ERR CAutoCOLUMNBASEA::ErrGet( JET_COLUMNBASE_W * pColumnBase )
{
    ERR                 err         = JET_errSuccess;

    Assert( m_pColumnBase );

    if ( !pColumnBase )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    pColumnBase->cbStruct   = sizeof( JET_COLUMNBASE_W );

    CallR( ErrOSSTRAsciiToUnicode( m_pColumnBase->szBaseTableName, pColumnBase->szBaseTableName, _countof(pColumnBase->szBaseTableName), NULL, OSSTR_FIXED_CONVERSION ) );
    CallR( ErrOSSTRAsciiToUnicode( m_pColumnBase->szBaseColumnName, pColumnBase->szBaseColumnName, _countof(pColumnBase->szBaseColumnName), NULL, OSSTR_FIXED_CONVERSION ) );

    pColumnBase->columnid   = m_pColumnBase->columnid;
    pColumnBase->coltyp     = m_pColumnBase->coltyp;
    pColumnBase->wCountry   = m_pColumnBase->wCountry;
    pColumnBase->langid     = m_pColumnBase->langid;
    pColumnBase->cp         = m_pColumnBase->cp;
    pColumnBase->wFiller    = m_pColumnBase->wFiller;
    pColumnBase->cbMax      = m_pColumnBase->cbMax;
    pColumnBase->grbit      = m_pColumnBase->grbit;

HandleError:
    return err;
}

CAutoCOLUMNBASEA::~CAutoCOLUMNBASEA()
{
    delete m_pColumnBase;
}

LOCAL JET_ERR JetGetTableColumnInfoExW(
    __in JET_SESID                  sesid,
    __in JET_TABLEID                tableid,
    __in_opt JET_PCWSTR             pColumnNameOrId,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    ERR                 err         = JET_errSuccess;
    CAutoSZDDL          lcolumnName;
    CAutoCOLUMNBASEA    lcolumnBase;

    const BOOL      fByColid        = ( ( InfoLevel & maskGetColumInfoLevelBase ) == JET_ColInfoByColid
                                        || ( InfoLevel & maskGetColumInfoLevelBase ) == JET_ColInfoBaseByColid );

    const BOOL      fColumnBaseData     = ( ( InfoLevel & maskGetColumInfoLevelBase ) == JET_ColInfoBase
                                        || ( InfoLevel & maskGetColumInfoLevelBase ) == JET_ColInfoBaseByColid );

    void *          pvResultNew         = pvResult;
    ULONG   cbMaxNew            = cbMax;

    if ( fColumnBaseData )
    {
        if ( !pvResult )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }

        if ( cbMax < sizeof( JET_COLUMNBASE_W ) )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }

        CallR ( lcolumnBase.ErrSet( (JET_COLUMNBASE_W *)pvResult ) );

        pvResultNew     = (JET_COLUMNBASE_A*)lcolumnBase;
        cbMaxNew        = sizeof(JET_COLUMNBASE_A);
    }

    if ( fByColid )
    {
        err = JetGetTableColumnInfoEx( sesid, tableid, (char*)pColumnNameOrId, pvResultNew, cbMaxNew, InfoLevel, fTrue );
    }
    else
    {
        CallR( lcolumnName.ErrSet( pColumnNameOrId ) );

        err = JetGetTableColumnInfoEx( sesid, tableid, lcolumnName, pvResultNew, cbMaxNew, InfoLevel, fTrue );
    }

    if ( err >= JET_errSuccess && fColumnBaseData )
    {
        ERR     errTemp;

        errTemp = ( lcolumnBase.ErrGet( (JET_COLUMNBASE_W *)pvResult ) );

        if ( errTemp < JET_errSuccess )
        {
            err = errTemp;
        }
        else if ( err == JET_errSuccess )
        {
            err = errTemp;
        }
    }

    return err;
}

JET_ERR JET_API JetGetTableColumnInfoA(
    __in JET_SESID                  sesid,
    __in JET_TABLEID                tableid,
    __in_opt JET_PCSTR              szColumnName,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetTableColumnInfo, JetGetTableColumnInfoEx( sesid, tableid, szColumnName, (void*) pvResult, cbMax, InfoLevel, fFalse ) );
}

JET_ERR JET_API JetGetTableColumnInfoW(
    __in JET_SESID                  sesid,
    __in JET_TABLEID                tableid,
    __in_opt JET_PCWSTR             wszColumnName,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetTableColumnInfo, JetGetTableColumnInfoExW( sesid, tableid, wszColumnName, (void*) pvResult, cbMax, InfoLevel ) );
}

LOCAL JET_ERR JetGetColumnInfoEx(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_PCSTR                  szTableName,
    __in_opt JET_PCSTR              pColumnNameOrId,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel,
    __in const BOOL                 fUnicodeNames )
{
    ERR             err;
    ULONG           cbMin;
    const BOOL      fByColid        = ( ( InfoLevel & maskGetColumInfoLevelBase ) == JET_ColInfoByColid
                                        || ( InfoLevel & maskGetColumInfoLevelBase ) == JET_ColInfoBaseByColid );
    APICALL_SESID   apicall( opGetColumnInfo );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p[%s],0x%p[%s],0x%p,0x%x,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            szTableName,
            OSFormatString( szTableName ),
            pColumnNameOrId,
            ( fByColid ? OSFormat( "<0x%x>", *(JET_COLUMNID *)pColumnNameOrId ) : OSFormatString( pColumnNameOrId ) ),
            pvResult,
            cbMax,
            InfoLevel,
            fUnicodeNames ) );

    if ( !apicall.FEnter( sesid, dbid ) )
        return apicall.ErrResult();

    ProbeClientBuffer( pvResult, cbMax );

    const ULONG InfoLevelT = InfoLevel & maskGetColumInfoLevelBase;
    switch( InfoLevelT )
    {
        case JET_ColInfo:
        case JET_ColInfoByColid:
            cbMin = sizeof(JET_COLUMNDEF);
            break;
        case JET_ColInfoList:
        case JET_ColInfoListCompact:
        case JET_ColInfoListSortColumnid:
            cbMin = sizeof(JET_COLUMNLIST);
            break;
        case JET_ColInfoBase:
        case JET_ColInfoBaseByColid:
            cbMin = sizeof(JET_COLUMNBASE_A);
            break;

        case JET_ColInfoSysTabCursor:
            FireWall( OSFormat( "FeatureNAGetColInfo:%d", (INT)InfoLevelT ) );
            Error( ErrERRCheck( JET_errFeatureNotAvailable ) );

        default:
            Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( cbMax >= cbMin )
    {
        if ( fByColid )
        {
            err = ErrIsamGetColumnInfo(
                            sesid,
                            dbid,
                            szTableName,
                            NULL,
                            (JET_COLUMNID *)pColumnNameOrId,
                            pvResult,
                            cbMax,
                            InfoLevel,
                            fUnicodeNames );
        }
        else
        {
            err = ErrIsamGetColumnInfo(
                            sesid,
                            dbid,
                            szTableName,
                            pColumnNameOrId,
                            NULL,
                            pvResult,
                            cbMax,
                            InfoLevel,
                            fUnicodeNames );
        }
    }
    else
    {
        err = ErrERRCheck( JET_errBufferTooSmall );
    }

HandleError:
    apicall.LeaveAfterCall( err );
    return apicall.ErrResult();
}

JET_ERR JET_API JetGetColumnInfoExW(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_PCWSTR                 wszTableName,
    __in_opt JET_PCWSTR             pwColumnNameOrId,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    ERR         err             = JET_errSuccess;
    CAutoSZDDL  lszTableName;
    CAutoSZDDL  lszColumnName;
    CAutoCOLUMNBASEA    lcolumnBase;
    void *              pvResultNew = pvResult;
    ULONG       cbMaxNew    = cbMax;

    const BOOL      fByColid        = ( ( InfoLevel & maskGetColumInfoLevelBase ) == JET_ColInfoByColid
                                        || ( InfoLevel & maskGetColumInfoLevelBase ) == JET_ColInfoBaseByColid );
    const BOOL      fColumnBaseData = ( ( InfoLevel & maskGetColumInfoLevelBase ) == JET_ColInfoBase
                                        || ( InfoLevel & maskGetColumInfoLevelBase ) == JET_ColInfoBaseByColid );

    CallR( lszTableName.ErrSet( wszTableName ) );

    if ( fColumnBaseData )
    {
        if ( !pvResult )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }

        if ( cbMax < sizeof( JET_COLUMNBASE_W ) )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }

        CallR ( lcolumnBase.ErrSet( (JET_COLUMNBASE_W *)pvResult ) );

        pvResultNew     = (JET_COLUMNBASE_A*)lcolumnBase;
        cbMaxNew    = sizeof(JET_COLUMNBASE_A);
    }

    if ( fByColid )
    {
        err = JetGetColumnInfoEx( sesid, dbid, (CHAR*)lszTableName, (CHAR*)pwColumnNameOrId, pvResultNew, cbMaxNew, InfoLevel, fTrue );
    }
    else
    {
        CallR( lszColumnName.ErrSet( pwColumnNameOrId ) );

        err = JetGetColumnInfoEx( sesid, dbid, (CHAR*)lszTableName, (CHAR*)lszColumnName, pvResultNew, cbMaxNew, InfoLevel, fTrue );
    }

    if(fColumnBaseData && err >= JET_errSuccess)
    {
        ERR     errTemp;

        errTemp = ( lcolumnBase.ErrGet( (JET_COLUMNBASE_W *)pvResult ) );

        if ( errTemp < JET_errSuccess )
        {
            err = errTemp;
        }
        else if ( err == JET_errSuccess )
        {
            err = errTemp;
        }
    }

    return err;
}

JET_ERR JET_API JetGetColumnInfoA(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_PCSTR                  szTableName,
    __in_opt JET_PCSTR              pColumnNameOrId,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetColumnInfo, JetGetColumnInfoEx( sesid, dbid, szTableName, pColumnNameOrId, pvResult, cbMax, InfoLevel, fFalse ) );
}

JET_ERR JET_API JetGetColumnInfoW(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_PCWSTR                 wszTableName,
    __in_opt JET_PCWSTR             pwColumnNameOrId,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetColumnInfo, JetGetColumnInfoExW( sesid, dbid, wszTableName, pwColumnNameOrId, pvResult, cbMax, InfoLevel ) );
}

LOCAL JET_ERR JetRetrieveColumnEx(
    __in JET_SESID                                      sesid,
    __in JET_TABLEID                                    tableid,
    __in JET_COLUMNID                                   columnid,
    __out_bcount_part_opt( cbData, min( cbData, *pcbActual ) ) void *   pvData,
    __in ULONG                                  cbData,
    __out_opt ULONG *                           pcbActual,
    __in JET_GRBIT                                      grbit,
    __inout_opt JET_RETINFO *                           pretinfo )
{
    APICALL_SESID   apicall( opRetrieveColumn );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x,0x%p,0x%x,0x%p,0x%x,0x%p[%s])",
            __FUNCTION__,
            sesid,
            tableid,
            columnid,
            pvData,
            cbData,
            pcbActual,
            grbit,
            pretinfo,
            ( NULL != pretinfo ? OSFormat( "ib=0x%x,itag=%d", pretinfo->ibLongValue, pretinfo->itagSequence ) : OSTRACENULLPARAM ) ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispRetrieveColumn(
                                        sesid,
                                        tableid,
                                        columnid,
                                        pvData,
                                        cbData,
                                        pcbActual,
                                        grbit,
                                        pretinfo ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetRetrieveColumn(
    _In_ JET_SESID                                      sesid,
    _In_ JET_TABLEID                                    tableid,
    _In_ JET_COLUMNID                                   columnid,
    _Out_writes_bytes_to_opt_( cbData, min( cbData, *pcbActual ) ) void *   pvData,
    _In_ ULONG                                  cbData,
    _Out_opt_ ULONG *                           pcbActual,
    _In_ JET_GRBIT                                      grbit,
    _Inout_opt_ JET_RETINFO *                           pretinfo )
{

    JET_VALIDATE_SESID_TABLEID( sesid, tableid );

    JET_TRY( opRetrieveColumn, JetRetrieveColumnEx( sesid, tableid, columnid, pvData, cbData, pcbActual, grbit, pretinfo ) );
}


LOCAL JET_ERR JetRetrieveColumnsEx(
    __in JET_SESID                                              sesid,
    __in JET_TABLEID                                            tableid,
    __inout_ecount_opt( cretrievecolumn ) JET_RETRIEVECOLUMN *  pretrievecolumn,
    __in ULONG                                          cretrievecolumn )
{
    APICALL_SESID       apicall( opRetrieveColumns );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            pretrievecolumn,
            cretrievecolumn ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispRetrieveColumns(
                                        sesid,
                                        tableid,
                                        pretrievecolumn,
                                        cretrievecolumn ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetRetrieveColumns(
    _In_ JET_SESID                                              sesid,
    _In_ JET_TABLEID                                            tableid,
    _Inout_updates_opt_( cretrievecolumn ) JET_RETRIEVECOLUMN * pretrievecolumn,
    _In_ ULONG                                          cretrievecolumn )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opRetrieveColumns, JetRetrieveColumnsEx( sesid, tableid, pretrievecolumn, cretrievecolumn ) );
}


LOCAL JET_ERR JetEnumerateColumnsEx(
    JET_SESID           sesid,
    JET_TABLEID         tableid,
    ULONG       cEnumColumnId,
    JET_ENUMCOLUMNID*   rgEnumColumnId,
    ULONG*      pcEnumColumn,
    JET_ENUMCOLUMN**    prgEnumColumn,
    JET_PFNREALLOC      pfnRealloc,
    void*               pvReallocContext,
    ULONG       cbDataMost,
    JET_GRBIT           grbit )
{
    APICALL_SESID       apicall( opEnumerateColumns );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x,0x%p,0x%p,0x%p,0x%p,0x%p,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            cEnumColumnId,
            rgEnumColumnId,
            pcEnumColumn,
            prgEnumColumn,
            pfnRealloc,
            pvReallocContext,
            cbDataMost,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispEnumerateColumns(
                                        sesid,
                                        tableid,
                                        cEnumColumnId,
                                        rgEnumColumnId,
                                        pcEnumColumn,
                                        prgEnumColumn,
                                        pfnRealloc,
                                        pvReallocContext,
                                        cbDataMost,
                                        grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetEnumerateColumns(
    _In_ JET_SESID                                          sesid,
    _In_ JET_TABLEID                                        tableid,
    _In_ ULONG                                      cEnumColumnId,
    _In_reads_opt_( cEnumColumnId ) JET_ENUMCOLUMNID *      rgEnumColumnId,
    _Out_ ULONG *                                   pcEnumColumn,
    _Outptr_result_buffer_( *pcEnumColumn ) JET_ENUMCOLUMN **   prgEnumColumn,
    _In_ JET_PFNREALLOC                                     pfnRealloc,
    _In_opt_ void *                                         pvReallocContext,
    _In_ ULONG                                      cbDataMost,
    _In_ JET_GRBIT                                          grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opEnumerateColumns,
        JetEnumerateColumnsEx(  sesid,
                                tableid,
                                cEnumColumnId,
                                rgEnumColumnId,
                                pcEnumColumn,
                                prgEnumColumn,
                                pfnRealloc,
                                pvReallocContext,
                                cbDataMost,
                                grbit ) );
}


LOCAL JET_ERR JetRetrieveTaggedColumnListEx(
    __in JET_SESID                                                                              sesid,
    __in JET_TABLEID                                                                            tableid,
    __out ULONG *                                                                       pcColumns,
    __out_xcount_part_opt( cbData, *pcColumns * sizeof( JET_RETRIEVEMULTIVALUECOUNT ) ) void *  pvData,
    __in ULONG                                                                          cbData,
    __in JET_COLUMNID                                                                           columnidStart,
    __in JET_GRBIT                                                                              grbit )
{
    APICALL_SESID   apicall( opRetrieveTaggedColumnList );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%p,0x%x,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            pcColumns,
            pvData,
            cbData,
            columnidStart,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        ProbeClientBuffer( pvData, cbData, fTrue );
        apicall.LeaveAfterCall( ErrDispRetrieveTaggedColumnList(
                                        sesid,
                                        tableid,
                                        pcColumns,
                                        pvData,
                                        cbData,
                                        columnidStart,
                                        grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetRetrieveTaggedColumnList(
    _In_ JET_SESID                                                                              sesid,
    _In_ JET_TABLEID                                                                            tableid,
    _Out_ ULONG *                                                                       pcColumns,
    _Out_writes_bytes_to_opt_( cbData, *pcColumns * sizeof( JET_RETRIEVEMULTIVALUECOUNT ) )     void *  pvData,
    _In_ ULONG                                                                          cbData,
    _In_ JET_COLUMNID                                                                           columnidStart,
    _In_ JET_GRBIT                                                                              grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opRetrieveTaggedColumnList, JetRetrieveTaggedColumnListEx( sesid, tableid, pcColumns, pvData, cbData, columnidStart, grbit ) );
}

LOCAL JET_ERR JetGetRecordSize3Ex(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __inout JET_RECSIZE3 *  precsize,
    __in const JET_GRBIT    grbit )
{
    APICALL_SESID       apicall( opGetRecordSize );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( (grbit & ~(JET_bitRecordSizeInCopyBuffer|JET_bitRecordSizeRunningTotal|JET_bitRecordSizeLocal|JET_bitRecordSizeIntrinsicPhysicalOnly|JET_bitRecordSizeExtrinsicLogicalOnly)) ?
            ErrERRCheck( JET_errInvalidGrbit ) :
            ErrDispGetRecordSize(
                sesid,
                tableid,
                precsize,
                grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetGetRecordSize3(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __inout JET_RECSIZE3 *  precsize,
    __in const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetRecordSize, JetGetRecordSize3Ex( sesid, tableid, precsize, grbit ) );
}

LOCAL JET_ERR JetGetRecordSize2Ex(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __inout JET_RECSIZE2 *  precsize,
    __in const JET_GRBIT    grbit )
{
    JET_RECSIZE3 recsize3;

    if ( grbit & JET_bitRecordSizeRunningTotal )
    {
        UtilMemCpy( &recsize3, precsize, sizeof( *precsize ) );
    }

    const ERR err = JetGetRecordSize3Ex( sesid, tableid, &recsize3, grbit );
    UtilMemCpy( precsize, &recsize3, sizeof( *precsize ) );
    return err;
}
JET_ERR JET_API JetGetRecordSize2(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __inout JET_RECSIZE2 *  precsize,
    __in const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetRecordSize, JetGetRecordSize2Ex( sesid, tableid, precsize, grbit ) );
}

LOCAL JET_ERR JetGetRecordSizeEx(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __inout JET_RECSIZE *   precsize,
    __in const JET_GRBIT    grbit )
{
    JET_RECSIZE3 recsize3;

    if ( grbit & JET_bitRecordSizeRunningTotal )
    {
        UtilMemCpy( &recsize3, precsize, sizeof( *precsize ) );
    }

    const ERR err = JetGetRecordSize3Ex( sesid, tableid, &recsize3, grbit );
    UtilMemCpy( precsize, &recsize3, sizeof( *precsize ) );
    return err;
}
JET_ERR JET_API JetGetRecordSize(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __inout JET_RECSIZE *   precsize,
    __in const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetRecordSize, JetGetRecordSizeEx( sesid, tableid, precsize, grbit ) );
}


LOCAL JET_ERR JetSetColumnEx(
    __in JET_SESID                          sesid,
    __in JET_TABLEID                        tableid,
    __in JET_COLUMNID                       columnid,
    __in_bcount_opt( cbData ) const void *  pvData,
    __in ULONG                      cbData,
    __in JET_GRBIT                          grbit,
    __in_opt JET_SETINFO *                  psetinfo )
{
    APICALL_SESID   apicall( opSetColumn );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x,0x%p[%s],0x%x,0x%x,0x%p[%s])",
            __FUNCTION__,
            sesid,
            tableid,
            columnid,
            pvData,
            OSFormatRawDataParam( (BYTE *)pvData, cbData ),
            cbData,
            grbit,
            psetinfo,
            ( NULL != psetinfo ? OSFormat( "ib=0x%x,itag=%d", psetinfo->ibLongValue, psetinfo->itagSequence ) : OSTRACENULLPARAM ) ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispSetColumn(
                                        sesid,
                                        tableid,
                                        columnid,
                                        pvData,
                                        cbData,
                                        grbit,
                                        psetinfo ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetSetColumn(
    __in JET_SESID                          sesid,
    __in JET_TABLEID                        tableid,
    __in JET_COLUMNID                       columnid,
    __in_bcount_opt( cbData ) const void *  pvData,
    __in ULONG                      cbData,
    __in JET_GRBIT                          grbit,
    __in_opt JET_SETINFO *                  psetinfo )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetColumn, JetSetColumnEx( sesid, tableid, columnid, pvData, cbData, grbit, psetinfo ) );
}


LOCAL JET_ERR JetSetColumnsEx(
    __in JET_SESID                                  sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount_opt( csetcolumn ) JET_SETCOLUMN *   psetcolumn,
    __in ULONG                              csetcolumn )
{
    APICALL_SESID   apicall( opSetColumns );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            psetcolumn,
            csetcolumn ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispSetColumns(
                                        sesid,
                                        tableid,
                                        psetcolumn,
                                        csetcolumn ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetSetColumns(
    __in JET_SESID                                  sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount_opt( csetcolumn ) JET_SETCOLUMN *   psetcolumn,
    __in ULONG                              csetcolumn )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetColumns, JetSetColumnsEx( sesid, tableid, psetcolumn, csetcolumn ) );
}


LOCAL JET_ERR JetPrepareUpdateEx(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in ULONG  prep )
{
    APICALL_SESID   apicall( opPrepareUpdate );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            prep ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispPrepareUpdate( sesid, tableid, prep ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetPrepareUpdate(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in ULONG  prep )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opPrepareUpdate, JetPrepareUpdateEx( sesid, tableid, prep ) );
}


LOCAL JET_ERR JetUpdateEx(
    __in JET_SESID                                          sesid,
    __in JET_TABLEID                                        tableid,
    __out_bcount_part_opt( cbBookmark, *pcbActual ) void *  pvBookmark,
    __in ULONG                                      cbBookmark,
    __out_opt ULONG *                               pcbActual,
    __in const JET_GRBIT                                    grbit )
{
    APICALL_SESID   apicall( opUpdate );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%x,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            pvBookmark,
            cbBookmark,
            pcbActual,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispUpdate(
                                        sesid,
                                        tableid,
                                        pvBookmark,
                                        cbBookmark,
                                        pcbActual,
                                        grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetUpdate(
    _In_ JET_SESID                                          sesid,
    _In_ JET_TABLEID                                        tableid,
    _Out_writes_bytes_to_opt_( cbBookmark, *pcbActual ) void *  pvBookmark,
    _In_ ULONG                                      cbBookmark,
    _Out_opt_ ULONG *                               pcbActual )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opUpdate, JetUpdateEx( sesid, tableid, pvBookmark, cbBookmark, pcbActual, NO_GRBIT ) );
}
JET_ERR JET_API JetUpdate2(
    _In_ JET_SESID                                          sesid,
    _In_ JET_TABLEID                                        tableid,
    _Out_writes_bytes_to_opt_( cbBookmark, *pcbActual ) void *  pvBookmark,
    _In_ ULONG                                      cbBookmark,
    _Out_opt_ ULONG *                               pcbActual,
    _In_ const JET_GRBIT                                    grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opUpdate, JetUpdateEx( sesid, tableid, pvBookmark, cbBookmark, pcbActual, grbit ) );
}


LOCAL JET_ERR JetEscrowUpdateEx(
    __in JET_SESID                                          sesid,
    __in JET_TABLEID                                        tableid,
    __in JET_COLUMNID                                       columnid,
    __in_bcount( cbMax ) void *                             pv,
    __in ULONG                                      cbMax,
    __out_bcount_part_opt( cbOldMax, *pcbOldActual ) void * pvOld,
    __in ULONG                                      cbOldMax,
    __out_opt ULONG *                               pcbOldActual,
    __in JET_GRBIT                                          grbit )
{
    APICALL_SESID   apicall( opEscrowUpdate );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x,0x%p[%s],0x%x,0x%p,0x%x,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            columnid,
            pv,
            OSFormatRawDataParam( (BYTE *)pv, cbMax ),
            cbMax,
            pvOld,
            cbOldMax,
            pcbOldActual,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispEscrowUpdate(
                                        sesid,
                                        tableid,
                                        columnid,
                                        pv,
                                        cbMax,
                                        pvOld,
                                        cbOldMax,
                                        pcbOldActual,
                                        grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetEscrowUpdate(
    _In_ JET_SESID                                          sesid,
    _In_ JET_TABLEID                                        tableid,
    _In_ JET_COLUMNID                                       columnid,
    _In_reads_bytes_( cbMax ) void *                        pv,
    _In_ ULONG                                      cbMax,
    _Out_writes_bytes_to_opt_( cbOldMax, *pcbOldActual ) void * pvOld,
    _In_ ULONG                                      cbOldMax,
    _Out_opt_ ULONG *                               pcbOldActual,
    _In_ JET_GRBIT                                          grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opEscrowUpdate, JetEscrowUpdateEx( sesid, tableid, columnid, pv, cbMax, pvOld, cbOldMax, pcbOldActual, grbit ) );
}


LOCAL JET_ERR JetDeleteEx( __in JET_SESID sesid, __in JET_TABLEID tableid )
{
    APICALL_SESID   apicall( opDelete );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix)",
            __FUNCTION__,
            sesid,
            tableid ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispDelete( sesid, tableid ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetDelete( __in JET_SESID sesid, __in JET_TABLEID tableid )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDelete, JetDeleteEx( sesid, tableid ) );
}


LOCAL JET_ERR JetGetCursorInfoEx(
    __in JET_SESID                  sesid,
    __in JET_TABLEID                tableid,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    APICALL_SESID   apicall( opGetCursorInfo );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            pvResult,
            cbMax,
            InfoLevel ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispGetCursorInfo(
                                        sesid,
                                        tableid,
                                        pvResult,
                                        cbMax,
                                        InfoLevel ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetGetCursorInfo(
    __in JET_SESID                  sesid,
    __in JET_TABLEID                tableid,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetCursorInfo, JetGetCursorInfoEx( sesid, tableid, pvResult, cbMax, InfoLevel ) );
}


LOCAL JET_ERR JetGetCurrentIndexEx(
    __in JET_SESID                          sesid,
    __in JET_TABLEID                        tableid,
    _Out_z_bytecap_( cbIndexName ) JET_PSTR szIndexName,
    __in ULONG                      cbIndexName )
{
    APICALL_SESID   apicall( opGetCurrentIndex );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p[%s],0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            szIndexName,
            OSFormatString( szIndexName ),
            cbIndexName ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispGetCurrentIndex(
                                        sesid,
                                        tableid,
                                        szIndexName,
                                        cbIndexName ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetGetCurrentIndexExW(
    __in JET_SESID                          sesid,
    __in JET_TABLEID                        tableid,
    _Out_z_bytecap_( cbIndexName ) JET_PWSTR    szIndexName,
    __in ULONG                      cbIndexName )
{
    ERR         err     = JET_errSuccess;
    CAutoSZDDL  lszIndexName;

    CallR( lszIndexName.ErrAlloc( cbIndexName ) );

    CallR( JetGetCurrentIndexEx( sesid, tableid, lszIndexName.Pv(), lszIndexName.Cb() ) );

    return lszIndexName.ErrGet( szIndexName, cbIndexName );
}

JET_ERR JET_API JetGetCurrentIndexA(
    __in JET_SESID                          sesid,
    __in JET_TABLEID                        tableid,
    __out_bcount( cbIndexName ) JET_PSTR    szIndexName,
    __in ULONG                      cbIndexName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetCurrentIndex, JetGetCurrentIndexEx( sesid, tableid, szIndexName, cbIndexName ) );
}

JET_ERR JET_API JetGetCurrentIndexW(
    __in JET_SESID                          sesid,
    __in JET_TABLEID                        tableid,
    __out_bcount( cbIndexName ) JET_PWSTR   szIndexName,
    __in ULONG                      cbIndexName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetCurrentIndex, JetGetCurrentIndexExW( sesid, tableid, szIndexName, cbIndexName ) );
}

LOCAL JET_ERR JetSetCurrentIndexEx(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __in_opt JET_PCSTR      szIndexName,
    __in_opt JET_INDEXID *  pindexid,
    __in JET_GRBIT          grbit,
    __in ULONG      itagSequence )
{
    APICALL_SESID       apicall( opSetCurrentIndex );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p[%s],0x%p[%s],0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            szIndexName,
            OSFormatString( szIndexName ),
            pindexid,
            ( NULL != pindexid ?
                        OSFormat(
                                "objid=0x%x,pfcb=0x%p",
                                ( (const INDEXID *)pindexid )->objidFDP,
                                ( (const INDEXID *)pindexid )->pfcbIndex ) :
                        OSTRACENULLPARAM ),
            grbit,
            itagSequence ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( grbit & ~(JET_bitMoveFirst|JET_bitNoMove) ?
                                        ErrERRCheck( JET_errInvalidGrbit ) :
                                        ErrDispSetCurrentIndex( sesid, tableid, szIndexName, pindexid, grbit, itagSequence ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetSetCurrentIndexExW(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __in_opt JET_PCWSTR     wszIndexName,
    __in_opt JET_INDEXID *  pindexid,
    __in JET_GRBIT          grbit,
    __in ULONG      itagSequence )
{
    ERR         err;
    CAutoSZDDL  lszIndexName;

    CallR( lszIndexName.ErrSet( wszIndexName ) );

    return JetSetCurrentIndexEx( sesid, tableid, lszIndexName, pindexid, grbit, itagSequence );
}

JET_ERR JET_API JetSetCurrentIndexA(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in_opt JET_PCSTR  szIndexName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexEx( sesid, tableid, szIndexName, NULL, JET_bitMoveFirst, 1 ) );
}

JET_ERR JET_API JetSetCurrentIndexW(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in_opt JET_PCWSTR wszIndexName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexExW( sesid, tableid, wszIndexName, NULL, JET_bitMoveFirst, 1 ) );
}
JET_ERR JET_API JetSetCurrentIndex2A(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in_opt JET_PCSTR  szIndexName,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexEx( sesid, tableid, szIndexName, NULL, grbit, 1 ) );
}
JET_ERR JET_API JetSetCurrentIndex2W(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in_opt JET_PCWSTR wszIndexName,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexExW( sesid, tableid, wszIndexName, NULL, grbit, 1 ) );
}

JET_ERR JET_API JetSetCurrentIndex3A(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in_opt JET_PCSTR  szIndexName,
    __in JET_GRBIT      grbit,
    __in ULONG  itagSequence )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexEx( sesid, tableid, szIndexName, NULL, grbit, itagSequence ) );
}
JET_ERR JET_API JetSetCurrentIndex3W(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in_opt JET_PCWSTR wszIndexName,
    __in JET_GRBIT      grbit,
    __in ULONG  itagSequence )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexExW( sesid, tableid, wszIndexName, NULL, grbit, itagSequence ) );
}
JET_ERR JET_API JetSetCurrentIndex4A(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __in_opt JET_PCSTR      szIndexName,
    __in_opt JET_INDEXID *  pindexid,
    __in JET_GRBIT          grbit,
    __in ULONG      itagSequence )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexEx( sesid, tableid, szIndexName, pindexid, grbit, itagSequence ) );
}
JET_ERR JET_API JetSetCurrentIndex4W(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __in_opt JET_PCWSTR     wszIndexName,
    __in_opt JET_INDEXID *  pindexid,
    __in JET_GRBIT          grbit,
    __in ULONG      itagSequence )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexExW( sesid, tableid, wszIndexName, pindexid, grbit, itagSequence ) );
}


LOCAL JET_ERR JetMoveEx(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __in LONG               cRow,
    __in JET_GRBIT          grbit )
{
    APICALL_SESID           apicall( opMove );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            cRow,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispMove( sesid, tableid, cRow, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetMove(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in LONG           cRow,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opMove, JetMoveEx( sesid, tableid, cRow, grbit ) );
}

LOCAL JET_ERR JetSetCursorFilterEx(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __in_ecount(cFilters) JET_INDEX_COLUMN *rgFilters,
    __in DWORD              cFilters,
    __in JET_GRBIT          grbit )
{
    APICALL_SESID           apicall( opSetCursorFilter );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,%d,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            rgFilters,
            cFilters,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispSetCursorFilter( sesid, tableid, rgFilters, cFilters, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetSetCursorFilter(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __in_ecount(cFilters) JET_INDEX_COLUMN *rgFilters,
    __in DWORD              cFilters,
    __in JET_GRBIT          grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCursorFilter, JetSetCursorFilterEx( sesid, tableid, rgFilters, cFilters, grbit ) );
}

LOCAL JET_ERR JetMakeKeyEx(
    __in JET_SESID                          sesid,
    __in JET_TABLEID                        tableid,
    __in_bcount_opt( cbData ) const void *  pvData,
    __in ULONG                      cbData,
    __in JET_GRBIT                          grbit )
{
    APICALL_SESID   apicall( opMakeKey );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p[%s],0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            pvData,
            OSFormatRawDataParam( (BYTE *)pvData, cbData ),
            cbData,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispMakeKey(
                                        sesid,
                                        tableid,
                                        pvData,
                                        cbData,
                                        grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetMakeKey(
    __in JET_SESID                          sesid,
    __in JET_TABLEID                        tableid,
    __in_bcount_opt( cbData ) const void *  pvData,
    __in ULONG                      cbData,
    __in JET_GRBIT                          grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opMakeKey, JetMakeKeyEx( sesid, tableid, pvData, cbData, grbit ) );
}


LOCAL JET_ERR JetSeekEx( __in JET_SESID sesid, __in JET_TABLEID tableid, __in JET_GRBIT grbit )
{
    APICALL_SESID   apicall( opSeek );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispSeek( sesid, tableid, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetSeek( __in JET_SESID sesid, __in JET_TABLEID tableid, __in JET_GRBIT grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSeek, JetSeekEx( sesid, tableid, grbit ) );
}


LOCAL JET_ERR JetPrereadKeysEx(
    __in const JET_SESID                                sesid,
    __in const JET_TABLEID                              tableid,
    __in_ecount(ckeys) const void * const * const       rgpvKeys,
    __in_ecount(ckeys) const ULONG * const      rgcbKeys,
    __in const LONG                                     ckeys,
    __out_opt LONG * const                              pckeysPreread,
    __in const JET_GRBIT                                grbit )
{
    APICALL_SESID   apicall( opPrereadKeys );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%p,%d,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            rgpvKeys,
            rgcbKeys,
            ckeys,
            pckeysPreread,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispPrereadKeys( sesid, tableid, rgpvKeys, rgcbKeys, ckeys, pckeysPreread, grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetPrereadKeys(
    __in JET_SESID                                      sesid,
    __in JET_TABLEID                                    tableid,
    __in_ecount(ckeys) const void **                    rgpvKeys,
    __in_ecount(ckeys) const ULONG *            rgcbKeys,
    __in LONG                                           ckeys,
    __out_opt LONG *                                    pckeysPreread,
    __in JET_GRBIT                                      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opPrereadKeys, JetPrereadKeysEx( sesid, tableid, rgpvKeys, rgcbKeys, ckeys, pckeysPreread, grbit ) );
}

LOCAL JET_ERR JetPrereadIndexRangesEx(
    __in const JET_SESID                                sesid,
    __in const JET_TABLEID                              tableid,
    __in_ecount(cIndexRanges) const JET_INDEX_RANGE * const rgIndexRanges,
    __in const ULONG                            cIndexRanges,
    __out_opt ULONG * const                     pcRangesPreread,
    __in_ecount(ccolumnidPreread) const JET_COLUMNID * const rgcolumnidPreread,
    __in const ULONG                            ccolumnidPreread,
    __in const JET_GRBIT                                grbit )
{
    APICALL_SESID   apicall( opPrereadIndexRanges );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,%d,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            rgIndexRanges,
            cIndexRanges,
            pcRangesPreread,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispPrereadIndexRanges( sesid, tableid, rgIndexRanges, cIndexRanges, pcRangesPreread, rgcolumnidPreread, ccolumnidPreread, grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JetPrereadIndexRanges(
    __in const JET_SESID                                sesid,
    __in const JET_TABLEID                              tableid,
    __in_ecount(cIndexRanges) const JET_INDEX_RANGE * const rgIndexRanges,
    __in const ULONG                            cIndexRanges,
    __out_opt ULONG * const                     pcRangesPreread,
    __in_ecount(ccolumnidPreread) const JET_COLUMNID * const rgcolumnidPreread,
    __in const ULONG                            ccolumnidPreread,
    __in const JET_GRBIT                                grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opPrereadIndexRanges, JetPrereadIndexRangesEx( sesid, tableid, rgIndexRanges, cIndexRanges, pcRangesPreread, rgcolumnidPreread, ccolumnidPreread, grbit ) );
}

LOCAL JET_ERR JetGetBookmarkEx(
    __in JET_SESID                                      sesid,
    __in JET_TABLEID                                    tableid,
    __out_bcount_part_opt( cbMax, *pcbActual ) void *   pvBookmark,
    __in ULONG                                  cbMax,
    __out_opt ULONG *                           pcbActual )
{
    APICALL_SESID   apicall( opGetBookmark );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%x,0x%p)",
            __FUNCTION__,
            sesid,
            tableid,
            pvBookmark,
            cbMax,
            pcbActual ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispGetBookmark(
                                        sesid,
                                        tableid,
                                        pvBookmark,
                                        cbMax,
                                        pcbActual ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetGetBookmark(
    _In_ JET_SESID                                      sesid,
    _In_ JET_TABLEID                                    tableid,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) void *   pvBookmark,
    _In_ ULONG                                  cbMax,
    _Out_opt_ ULONG *                           pcbActual )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetBookmark, JetGetBookmarkEx( sesid, tableid, pvBookmark, cbMax, pcbActual ) );
}

JET_ERR JET_API JetGetSecondaryIndexBookmarkEx(
    __in JET_SESID                                                                  sesid,
    __in JET_TABLEID                                                                tableid,
    __out_bcount_part_opt( cbSecondaryKeyMax, *pcbSecondaryKeyActual ) void *       pvSecondaryKey,
    __in ULONG                                                              cbSecondaryKeyMax,
    __out_opt ULONG *                                                       pcbSecondaryKeyActual,
    __out_bcount_part_opt( cbPrimaryBookmarkMax, *pcbPrimaryBookmarkActual ) void * pvPrimaryBookmark,
    __in ULONG                                                              cbPrimaryBookmarkMax,
    __out_opt ULONG *                                                       pcbPrimaryBookmarkActual )
{
    APICALL_SESID   apicall( opGetBookmark );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%x,0x%p,0x%p,0x%x,0x%p)",
            __FUNCTION__,
            sesid,
            tableid,
            pvSecondaryKey,
            cbSecondaryKeyMax,
            pcbSecondaryKeyActual,
            pvPrimaryBookmark,
            cbPrimaryBookmarkMax,
            pcbPrimaryBookmarkActual ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispGetIndexBookmark(
                                        sesid,
                                        tableid,
                                        pvSecondaryKey,
                                        cbSecondaryKeyMax,
                                        pcbSecondaryKeyActual,
                                        pvPrimaryBookmark,
                                        cbPrimaryBookmarkMax,
                                        pcbPrimaryBookmarkActual ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetGetSecondaryIndexBookmark(
    _In_ JET_SESID                                                                  sesid,
    _In_ JET_TABLEID                                                                tableid,
    _Out_writes_bytes_to_opt_( cbSecondaryKeyMax, *pcbSecondaryKeyActual ) void *       pvSecondaryKey,
    _In_ ULONG                                                              cbSecondaryKeyMax,
    _Out_opt_ ULONG *                                                       pcbSecondaryKeyActual,
    _Out_writes_bytes_to_opt_( cbPrimaryBookmarkMax, *pcbPrimaryBookmarkActual ) void * pvPrimaryBookmark,
    _In_ ULONG                                                              cbPrimaryBookmarkMax,
    _Out_opt_ ULONG *                                                       pcbPrimaryBookmarkActual,
    _In_ const JET_GRBIT                                                            grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetBookmark,
        JetGetSecondaryIndexBookmarkEx(
                        sesid,
                        tableid,
                        pvSecondaryKey,
                        cbSecondaryKeyMax,
                        pcbSecondaryKeyActual,
                        pvPrimaryBookmark,
                        cbPrimaryBookmarkMax,
                        pcbPrimaryBookmarkActual ) );
}


LOCAL JET_ERR JetGotoBookmarkEx(
    __in JET_SESID                      sesid,
    __in JET_TABLEID                    tableid,
    __in_bcount( cbBookmark ) void *    pvBookmark,
    __in ULONG                  cbBookmark )
{
    APICALL_SESID   apicall( opGotoBookmark );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p[%s],0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            pvBookmark,
            OSFormatRawDataParam( (BYTE *)pvBookmark, cbBookmark ),
            cbBookmark ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispGotoBookmark(
                                        sesid,
                                        tableid,
                                        pvBookmark,
                                        cbBookmark ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetGotoBookmark(
    __in JET_SESID                      sesid,
    __in JET_TABLEID                    tableid,
    __in_bcount( cbBookmark ) void *    pvBookmark,
    __in ULONG                  cbBookmark )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGotoBookmark, JetGotoBookmarkEx( sesid, tableid, pvBookmark, cbBookmark ) );
}

JET_ERR JET_API JetGotoSecondaryIndexBookmarkEx(
    __in JET_SESID                              sesid,
    __in JET_TABLEID                            tableid,
    __in_bcount( cbSecondaryKey ) void *        pvSecondaryKey,
    __in ULONG                          cbSecondaryKey,
    __in_bcount_opt( cbPrimaryBookmark ) void * pvPrimaryBookmark,
    __in ULONG                          cbPrimaryBookmark,
    __in const JET_GRBIT                        grbit )
{
    APICALL_SESID   apicall( opGotoBookmark );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p[%s],0x%x,0x%p[%s],0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            pvSecondaryKey,
            OSFormatRawDataParam( (BYTE *)pvSecondaryKey, cbSecondaryKey ),
            cbSecondaryKey,
            pvPrimaryBookmark,
            OSFormatRawDataParam( (BYTE *)pvPrimaryBookmark, cbPrimaryBookmark ),
            cbPrimaryBookmark,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispGotoIndexBookmark(
                                        sesid,
                                        tableid,
                                        pvSecondaryKey,
                                        cbSecondaryKey,
                                        pvPrimaryBookmark,
                                        cbPrimaryBookmark,
                                        grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetGotoSecondaryIndexBookmark(
    __in JET_SESID                              sesid,
    __in JET_TABLEID                            tableid,
    __in_bcount( cbSecondaryKey ) void *        pvSecondaryKey,
    __in ULONG                          cbSecondaryKey,
    __in_bcount_opt( cbPrimaryBookmark ) void * pvPrimaryBookmark,
    __in ULONG                          cbPrimaryBookmark,
    __in const JET_GRBIT                        grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetBookmark,
        JetGotoSecondaryIndexBookmarkEx(
                        sesid,
                        tableid,
                        pvSecondaryKey,
                        cbSecondaryKey,
                        pvPrimaryBookmark,
                        cbPrimaryBookmark,
                        grbit ) );
}


JET_ERR JET_API JetIntersectIndexesEx(
    __in JET_SESID                              sesid,
    __in_ecount( cindexrange ) JET_INDEXRANGE * rgindexrange,
    __in ULONG                          cindexrange,
    __inout JET_RECORDLIST *                    precordlist,
    __in JET_GRBIT                              grbit )
{
    APICALL_SESID       apicall( opIntersectIndexes );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p,0x%x,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            rgindexrange,
            cindexrange,
            precordlist,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamIntersectIndexes(
                                        sesid,
                                        rgindexrange,
                                        cindexrange,
                                        precordlist,
                                        grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetIntersectIndexes(
    __in JET_SESID                              sesid,
    __in_ecount( cindexrange ) JET_INDEXRANGE * rgindexrange,
    __in ULONG                          cindexrange,
    __inout JET_RECORDLIST *                    precordlist,
    __in JET_GRBIT                              grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opIntersectIndexes, JetIntersectIndexesEx( sesid, rgindexrange, cindexrange, precordlist, grbit ) );
}


LOCAL JET_ERR JetGetRecordPositionEx(
    __in JET_SESID                          sesid,
    __in JET_TABLEID                        tableid,
    __out_bcount( cbRecpos ) JET_RECPOS *   precpos,
    __in ULONG                      cbRecpos )
{
    APICALL_SESID   apicall( opGetRecordPosition );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            precpos,
            cbRecpos ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispGetRecordPosition(
                                        sesid,
                                        tableid,
                                        precpos,
                                        cbRecpos ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetGetRecordPosition(
    __in JET_SESID                          sesid,
    __in JET_TABLEID                        tableid,
    __out_bcount( cbRecpos ) JET_RECPOS *   precpos,
    __in ULONG                      cbRecpos )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetRecordPosition, JetGetRecordPositionEx( sesid, tableid, precpos, cbRecpos ) );
}


LOCAL JET_ERR JetGotoPositionEx(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_RECPOS *   precpos )
{
    APICALL_SESID   apicall( opGotoPosition );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p[%s])",
            __FUNCTION__,
            sesid,
            tableid,
            precpos,
            ( NULL != precpos ? OSFormat( "LT=0x%x,Total=0x%x", precpos->centriesLT, precpos->centriesTotal ) : OSTRACENULLPARAM ) ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispGotoPosition(
                                        sesid,
                                        tableid,
                                        precpos ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetGotoPosition(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_RECPOS *   precpos )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGotoPosition, JetGotoPositionEx( sesid, tableid, precpos ) );
}


LOCAL JET_ERR JetRetrieveKeyEx(
    __in JET_SESID                                      sesid,
    __in JET_TABLEID                                    tableid,
    __out_bcount_part_opt( cbMax, *pcbActual ) void *   pvKey,
    __in ULONG                                  cbMax,
    __out_opt ULONG *                           pcbActual,
    __in JET_GRBIT                                      grbit )
{
    APICALL_SESID   apicall( opRetrieveKey );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%x,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            pvKey,
            cbMax,
            pcbActual,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispRetrieveKey(
                                        sesid,
                                        tableid,
                                        pvKey,
                                        cbMax,
                                        pcbActual,
                                        grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetRetrieveKey(
    _In_ JET_SESID                                      sesid,
    _In_ JET_TABLEID                                    tableid,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) void *   pvKey,
    _In_ ULONG                                  cbMax,
    _Out_opt_ ULONG *                           pcbActual,
    _In_ JET_GRBIT                                      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opRetrieveKey, JetRetrieveKeyEx( sesid, tableid, pvKey, cbMax, pcbActual, grbit ) );
}


LOCAL JET_ERR JetGetLockEx( __in JET_SESID sesid, __in JET_TABLEID tableid, __in JET_GRBIT grbit )
{
    APICALL_SESID   apicall( opGetLock );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispGetLock( sesid, tableid, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetGetLock( __in JET_SESID sesid, __in JET_TABLEID tableid, __in JET_GRBIT grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetLock, JetGetLockEx( sesid, tableid, grbit ) );
}

LOCAL JET_ERR JetGetVersionEx( __in JET_SESID sesid, __out ULONG  *pVersion )
{
    APICALL_SESID   apicall( opGetVersion );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p)",
            __FUNCTION__,
            sesid,
            pVersion ) );

    if ( apicall.FEnter( sesid ) )
    {

#ifdef ESENT
        Assert( DwUtilImageVersionMajor()     < 1 <<  4 );
        Assert( DwUtilImageVersionMinor()     < 1 <<  4 );
        Assert( DwUtilImageBuildNumberMajor() < 1 << 16 );


        Assert( DwUtilSystemServicePackNumber() < 1 << 8 );


        const ULONG ulVersion   = ( ( DwUtilImageVersionMajor()       & 0xF )    << 28 ) +
                                  ( ( DwUtilImageVersionMinor()       & 0xF )    << 24 ) +
                                  ( ( DwUtilImageBuildNumberMajor()   & 0xFFFF ) <<  8 ) +
                                    ( DwUtilSystemServicePackNumber() & 0xFF );

#else
        Assert( DwUtilImageVersionMajor()     < 1 <<  5 );
        Assert( DwUtilImageVersionMinor()     < 1 <<  5 );
        Assert( DwUtilImageBuildNumberMajor() < 1 << 14 );
        Assert( DwUtilImageBuildNumberMinor() < 1 <<  8 );

        const ULONG ulVersion   = ( ( DwUtilImageVersionMajor()       & 0x1F )   << 27 ) +
                                  ( ( DwUtilImageVersionMinor()       & 0x1F )   << 22 ) +
                                  ( ( DwUtilImageBuildNumberMajor()   & 0x3FFF ) <<  8 ) +
                                    ( DwUtilImageBuildNumberMinor()   & 0xFF );

#endif

        *pVersion = ulVersion;

        apicall.LeaveAfterCall( JET_errSuccess );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetGetVersion( __in JET_SESID sesid, __out ULONG  *pVersion )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetVersion, JetGetVersionEx( sesid, pVersion ) );
}

#pragma warning( disable : 4509 )


LOCAL JET_ERR JetGetSystemParameterEx(
    __in JET_INSTANCE                   instance,
    __in JET_SESID                      sesid,
    __in ULONG                  paramid,
    __out_opt JET_API_PTR *             plParam,
    _Out_opt_z_bytecap_( cbMax ) JET_PWSTR  wszParam,
    __in ULONG                  cbMax )
{
    APICALL_INST    apicall( opGetSystemParameter );
    INST*           pinst   = NULL;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x,0x%p,0x%p,0x%x)",
            __FUNCTION__,
            instance,
            sesid,
            paramid,
            plParam,
            wszParam,
            cbMax ) );

    INST::EnterCritInst();

    FixDefaultSystemParameters();

    if ( RUNINSTGetMode() == runInstModeOneInst )
    {
        if ( !g_rgparam[ paramid ].FGlobal() )
        {
            for ( size_t ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
            {
                if ( pinstNil != g_rgpinst[ ipinst ] )
                {
                    pinst = g_rgpinst[ ipinst ];
                }
            }
        }
    }
    else if ( RUNINSTGetMode() == runInstModeMultiInst )
    {
        if ( sesid && sesid != JET_sesidNil )
        {
            JET_VALIDATE_SESID( sesid );
            pinst = PinstFromPpib( (PIB*)sesid );
        }
        else if ( instance && instance != JET_instanceNil )
        {
            JET_VALIDATE_INSTANCE( instance );
            pinst = (INST *)instance;
        }
    }

    if ( !pinst )
    {
        ERR err = ErrGetSystemParameter( NULL, JET_sesidNil, paramid, plParam, wszParam, cbMax );
        INST::LeaveCritInst();
        return err;
    }
    else
    {
        const BOOL fInJetInitCallback = Ptls()->fInSoftStart && Ptls()->fInCallback;
        if ( apicall.FEnterWithoutInit( pinst, fInJetInitCallback ) )
        {
            ERR err = ErrGetSystemParameter( pinst, sesid, paramid, plParam, wszParam, cbMax );
            INST::LeaveCritInst();
            apicall.LeaveAfterCall( err );
        }
        else
        {
            INST::LeaveCritInst();
        }

        return apicall.ErrResult();
    }
}

#pragma warning( default : 4509 )

LOCAL JET_ERR JetGetSystemParameterExA(
    __in JET_INSTANCE                   instance,
    __in JET_SESID                      sesid,
    __in ULONG                  paramid,
    __out_opt JET_API_PTR *             plParam,
    _Out_opt_z_bytecap_( cbMax ) JET_PSTR   szParam,
    __in ULONG                  cbMax )
{
    ERR         err     = JET_errSuccess;
    ULONG       cbMaxT  = cbMax;
    CAutoWSZ    lwszParam;

    if ( szParam && cbMax )
    {
        CallR( lwszParam.ErrAlloc( cbMax * sizeof( WCHAR ) ) );
        cbMaxT = lwszParam.Cb();
    }

    CallR( JetGetSystemParameterEx( instance, sesid, paramid, plParam, lwszParam.Pv(), cbMaxT ) );

    if ( !szParam || !cbMax )
    {
        return err;
    }

    return lwszParam.ErrGet( (CHAR *)szParam, cbMax );
}


JET_ERR JET_API JetGetSystemParameterA(
    __in JET_INSTANCE                   instance,
    __in_opt JET_SESID                  sesid,
    __in ULONG                  paramid,
    __out_opt JET_API_PTR *             plParam,
    __out_bcount_opt( cbMax ) JET_PSTR  szParam,
    __in ULONG                  cbMax )
{
    JET_TRY( opGetSystemParameter, JetGetSystemParameterExA( instance, sesid, paramid, plParam, szParam, cbMax ) );
}

JET_ERR JET_API JetGetSystemParameterW(
    __in JET_INSTANCE                   instance,
    __in_opt JET_SESID                  sesid,
    __in ULONG                  paramid,
    __out_opt JET_API_PTR *             plParam,
    __out_bcount_opt( cbMax ) JET_PWSTR szParam,
    __in ULONG                  cbMax )
{
    JET_TRY( opGetSystemParameter, JetGetSystemParameterEx( instance, sesid, paramid, plParam, szParam, cbMax ) );
}



LOCAL JET_ERR JetBeginSessionEx(
    __in JET_INSTANCE   instance,
    __out JET_SESID *   psesid,
    __in_opt JET_PCSTR  szUserName,
    __in_opt JET_PCSTR  szPassword )
{
    APICALL_INST    apicall( opBeginSession );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p,0x%p[%s],0x%p[%s])",
            __FUNCTION__,
            instance,
            psesid,
            szUserName,
            OSFormatString( szUserName ),
            szPassword,
            OSFormatString( szPassword ) ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamBeginSession(
                                        (JET_INSTANCE)apicall.Pinst(),
                                        psesid ) );
    }

    return apicall.ErrResult();
}


LOCAL JET_ERR JetBeginSessionExW(
    __in JET_INSTANCE   instance,
    __out JET_SESID *   psesid,
    __in_opt JET_PCWSTR wszUserName,
    __in_opt JET_PCWSTR wszPassword )
{
    ERR     err;
    CAutoSZ lszUsername;
    CAutoSZ lszPassword;

    CallR( lszUsername.ErrSet( wszUserName ) );
    CallR( lszPassword.ErrSet( wszPassword ) );

    return JetBeginSessionEx( instance, psesid, lszUsername, lszPassword );
}


JET_ERR JET_API JetBeginSessionA(
    __in JET_INSTANCE   instance,
    __out JET_SESID *   psesid,
    __in_opt JET_PCSTR  szUserName,
    __in_opt JET_PCSTR  szPassword )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBeginSession, JetBeginSessionEx( instance, psesid, szUserName, szPassword ) );
}

JET_ERR JET_API JetBeginSessionW(
    __in JET_INSTANCE   instance,
    __out JET_SESID *   psesid,
    __in_opt JET_PCWSTR wszUserName,
    __in_opt JET_PCWSTR wszPassword )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBeginSession, JetBeginSessionExW( instance, psesid, wszUserName, wszPassword ) );
}

LOCAL JET_ERR JetDupSessionEx( __in JET_SESID sesid, __out JET_SESID *psesid )
{
    APICALL_SESID   apicall( opDupSession );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p)",
            __FUNCTION__,
            sesid,
            psesid ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamBeginSession(
                                        (JET_INSTANCE)PinstFromSesid( sesid ),
                                        psesid ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetDupSession( __in JET_SESID sesid, __out JET_SESID *psesid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDupSession, JetDupSessionEx( sesid, psesid ) );
}


LOCAL JET_ERR JetEndSessionEx( __in JET_SESID sesid, __in JET_GRBIT grbit )
{
    APICALL_SESID   apicall( opEndSession );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x)",
            __FUNCTION__,
            sesid,
            grbit ) );

    if ( apicall.FEnter( sesid, fTrue ) )
    {
        apicall.LeaveAfterEndSession( ErrIsamEndSession( sesid, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetEndSession( __in JET_SESID sesid, __in JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opEndSession, JetEndSessionEx( sesid, grbit ) );
}

LOCAL JET_ERR JetCreateDatabaseEx(
    __in JET_SESID                                              sesid,
    __in JET_PCWSTR                                             wszDbFileName,
    __out JET_DBID *                                            pdbid,
    _In_reads_opt_( csetdbparam ) const JET_SETDBPARAM * const  rgsetdbparam,
    _In_ const ULONG                                    csetdbparam,
    _In_ const JET_GRBIT                                        grbit )
{
    APICALL_SESID   apicall( opCreateDatabase );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p[%s],0x%p,0x%p,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            wszDbFileName,
            SzOSFormatStringW( wszDbFileName ),
            pdbid,
            rgsetdbparam,
            csetdbparam,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamCreateDatabase( sesid, wszDbFileName, pdbid, rgsetdbparam, csetdbparam, grbit ) );
    }

    Assert( apicall.ErrResult() < JET_errSuccess || FInRangeIFMP( IFMP(*pdbid) ) );

    return apicall.ErrResult();
}

LOCAL JET_ERR JetCreateDatabaseExA(
    __in JET_SESID                                              sesid,
    __in JET_PCSTR                                              szDbFileName,
    __out JET_DBID *                                            pdbid,
    _In_reads_opt_( csetdbparam ) const JET_SETDBPARAM * const  rgsetdbparam,
    _In_ const ULONG                                    csetdbparam,
    _In_ const JET_GRBIT                                        grbit )
{
    ERR             err;
    CAutoWSZPATH    lwszDatabaseName;

    CallR( lwszDatabaseName.ErrSet( szDbFileName ) );

    return JetCreateDatabaseEx( sesid, lwszDatabaseName, pdbid, rgsetdbparam, csetdbparam, grbit );
}

JET_ERR JET_API JetCreateDatabaseA(
    __in JET_SESID      sesid,
    __in JET_PCSTR      szFilename,
    __in_opt JET_PCSTR  szConnect,
    __out JET_DBID *    pdbid,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateDatabase, JetCreateDatabaseExA( sesid, szFilename, pdbid, NULL, 0, grbit ) );
}

JET_ERR JET_API JetCreateDatabaseW(
    __in JET_SESID      sesid,
    __in JET_PCWSTR     wszFilename,
    __in_opt JET_PCWSTR wszConnect,
    __out JET_DBID *    pdbid,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateDatabase, JetCreateDatabaseEx( sesid, wszFilename, pdbid, NULL, 0, grbit ) );
}

JET_ERR JET_API JetCreateDatabase2A(
    __in JET_SESID              sesid,
    __in JET_PCSTR              szFilename,
    __in ULONG          cpgDatabaseSizeMax,
    __out JET_DBID *            pdbid,
    __in JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    const JET_SETDBPARAM rgsetdbparam[] =
    {
            { JET_dbparamDbSizeMaxPages, &cpgDatabaseSizeMax, sizeof( cpgDatabaseSizeMax ) }
    };
    JET_TRY( opCreateDatabase, JetCreateDatabaseExA( sesid, szFilename, pdbid, rgsetdbparam, _countof( rgsetdbparam ), grbit ) );
}

JET_ERR JET_API JetCreateDatabase2W(
    __in JET_SESID              sesid,
    __in JET_PCWSTR             wszFilename,
    __in ULONG          cpgDatabaseSizeMax,
    __out JET_DBID *            pdbid,
    __in JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    const JET_SETDBPARAM rgsetdbparam[] =
    {
            { JET_dbparamDbSizeMaxPages, &cpgDatabaseSizeMax, sizeof( cpgDatabaseSizeMax ) }
    };
    JET_TRY( opCreateDatabase, JetCreateDatabaseEx( sesid, wszFilename, pdbid, rgsetdbparam, _countof( rgsetdbparam ), grbit ) );
}

JET_ERR JET_API JetCreateDatabase3A(
    _In_ JET_SESID                                  sesid,
    _In_ JET_PCSTR                                  szFilename,
    _Out_ JET_DBID *                                pdbid,
    _In_reads_opt_( csetdbparam ) JET_SETDBPARAM *  rgsetdbparam,
    _In_ ULONG                              csetdbparam,
    _In_ JET_GRBIT                                  grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateDatabase, JetCreateDatabaseExA( sesid, szFilename, pdbid, rgsetdbparam, csetdbparam, grbit ) );
}

JET_ERR JET_API JetCreateDatabase3W(
    _In_ JET_SESID                                  sesid,
    _In_ JET_PCWSTR                                 wszFilename,
    _Out_ JET_DBID *                                pdbid,
    _In_reads_opt_( csetdbparam ) JET_SETDBPARAM *  rgsetdbparam,
    _In_ ULONG                              csetdbparam,
    _In_ JET_GRBIT                                  grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateDatabase, JetCreateDatabaseEx( sesid, wszFilename, pdbid, rgsetdbparam, csetdbparam, grbit ) );
}

JET_ERR JET_API JetCreateDatabaseWithStreamingA(
    __in JET_SESID              sesid,
    __in JET_PCSTR              szDbFileName,
    _Reserved_ JET_PCSTR        szSLVFileName,
    _Reserved_ JET_PCSTR        szSLVRootName,
    __in const ULONG    cpgDatabaseSizeMax,
    __out JET_DBID *            pdbid,
    __in JET_GRBIT              grbit )
{
    return ErrERRCheck( JET_errFeatureNotAvailable );
}

JET_ERR JET_API JetCreateDatabaseWithStreamingW(
    __in JET_SESID              sesid,
    __in JET_PCWSTR             wszDbFileName,
    _Reserved_ JET_PCWSTR           wszSLVFileName,
    _Reserved_ JET_PCWSTR           wszSLVRootName,
    __in const ULONG    cpgDatabaseSizeMax,
    __out JET_DBID *            pdbid,
    __in JET_GRBIT              grbit )
{
    return ErrERRCheck( JET_errFeatureNotAvailable );
}

LOCAL JET_ERR JetOpenDatabaseEx(
    JET_SESID       sesid,
    __in PCWSTR     wszDatabase,
    __in PCWSTR     wszConnect,
    JET_DBID *      pdbid,
    JET_GRBIT       grbit )
{
    APICALL_SESID   apicall( opOpenDatabase );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p[%s],0x%p[%s],0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            wszDatabase,
            SzOSFormatStringW( wszDatabase ),
            wszConnect,
            SzOSFormatStringW( wszConnect ),
            pdbid,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamOpenDatabase(
                                        sesid,
                                        wszDatabase,
                                        wszConnect,
                                        pdbid,
                                        grbit ) );
    }

    Assert( apicall.ErrResult() < JET_errSuccess || FInRangeIFMP( IFMP(*pdbid) ) );

    return apicall.ErrResult();
}

LOCAL JET_ERR JetOpenDatabaseExA(
    __in JET_SESID      sesid,
    __in JET_PCSTR      szFilename,
    __in_opt JET_PCSTR  szConnect,
    __out JET_DBID*     pdbid,
    __in JET_GRBIT      grbit )
{
    ERR             err;
    CAutoWSZPATH    lwszDatabase;
    CAutoWSZ        lwszConnect;

    CallR( lwszDatabase.ErrSet( szFilename ) );
    CallR( lwszConnect.ErrSet( szConnect ) );

    return JetOpenDatabaseEx( sesid, lwszDatabase, lwszConnect, pdbid, grbit );
}

JET_ERR JET_API JetOpenDatabaseA(
    __in JET_SESID      sesid,
    __in JET_PCSTR      szFilename,
    __in_opt JET_PCSTR  szConnect,
    __out JET_DBID*     pdbid,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOpenDatabase, JetOpenDatabaseExA( sesid, szFilename, szConnect, pdbid, grbit ) );
}

JET_ERR JET_API JetOpenDatabaseW(
    __in JET_SESID      sesid,
    __in JET_PCWSTR     wszFilename,
    __in_opt JET_PCWSTR wszConnect,
    __out JET_DBID*     pdbid,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOpenDatabase, JetOpenDatabaseEx( sesid, wszFilename, wszConnect, pdbid, grbit ) );
}

LOCAL JET_ERR JetCloseDatabaseEx(  __in JET_SESID sesid,  __in JET_DBID dbid, __in JET_GRBIT grbit )
{
    APICALL_SESID   apicall( opCloseDatabase );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            grbit ) );

    if ( apicall.FEnter( sesid, dbid, fTrue ) )
    {
        apicall.LeaveAfterCall( ErrIsamCloseDatabase(
                                        sesid,
                                        dbid,
                                        grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetCloseDatabase( __in JET_SESID sesid, __in JET_DBID dbid, __in JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCloseDatabase, JetCloseDatabaseEx( sesid, dbid, grbit ) );
}


LOCAL JET_ERR JetGetDatabaseInfoEx(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    APICALL_SESID   apicall( opGetDatabaseInfo );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            pvResult,
            cbMax,
            InfoLevel ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        ProbeClientBuffer( pvResult, cbMax );
        apicall.LeaveAfterCall( ErrIsamGetDatabaseInfo(
                                        sesid,
                                        dbid,
                                        pvResult,
                                        cbMax,
                                        InfoLevel ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetGetDatabaseInfoExA(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    ERR         err     = JET_errSuccess;
    CAutoWSZFixedConversion lwsz;

    if ( JET_DbInfoFilename != InfoLevel )
    {
        return JetGetDatabaseInfoEx( sesid, dbid, pvResult, cbMax, InfoLevel );
    }

    if ( (~(ULONG)0) / sizeof(WCHAR) < cbMax )
    {
        CallR( JET_errOutOfMemory );
    }

    CallR( lwsz.ErrAlloc( cbMax * sizeof( WCHAR ) ) );

    CallR( JetGetDatabaseInfoEx( sesid, dbid, lwsz.Pv(), lwsz.Cb(), InfoLevel ) );

    return lwsz.ErrGet( (CHAR *)pvResult, cbMax );
}

JET_ERR JET_API JetGetDatabaseInfoA(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetDatabaseInfo, JetGetDatabaseInfoExA( sesid, dbid, pvResult, cbMax, InfoLevel ) );
}

JET_ERR JET_API JetGetDatabaseInfoW(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetDatabaseInfo, JetGetDatabaseInfoEx( sesid, dbid, pvResult, cbMax, InfoLevel ) );
}

LOCAL JET_ERR JetGetDatabaseFileInfoEx(
    __in JET_PCWSTR                 wszDatabaseName,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    ERR             err                 = JET_errSuccess;
    IFileSystemAPI* pfsapi              = NULL;
    IFileFindAPI*   pffapi              = NULL;
    WCHAR           wszFullDbName[ IFileSystemAPI::cchPathMax ];
    QWORD           cbFileSize          = 0;
    QWORD           cbFileSizeOnDisk    = 0;
    DBFILEHDR*      pdbfilehdr          = NULL;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p[%s],0x%p,0x%x,0x%x)",
            __FUNCTION__,
            wszDatabaseName,
            SzOSFormatStringW( wszDatabaseName ),
            pvResult,
            cbMax,
            InfoLevel ) );

    CallR( ErrOSUInit() );

    Call( ErrOSFSCreate( g_pfsconfigGlobal, &pfsapi ) );
    Call( pfsapi->ErrFileFind( wszDatabaseName, &pffapi ) );
    Call( pffapi->ErrNext() );
    Call( pffapi->ErrPath( wszFullDbName ) );
    Call( pffapi->ErrSize( &cbFileSize, IFileAPI::filesizeLogical ) );
    Call( pffapi->ErrSize( &cbFileSizeOnDisk, IFileAPI::filesizeOnDisk ) );

    HangInjection( 49660 );

    switch ( InfoLevel )
    {
        case JET_DbInfoFilesize:
            if ( sizeof( QWORD ) != cbMax )
            {
                Error( ErrERRCheck( JET_errInvalidBufferSize ) );
            }
            ProbeClientBuffer( pvResult, cbMax );

            memcpy( (BYTE*)pvResult, (BYTE*)&cbFileSize, sizeof( QWORD ) );
            break;

        case JET_DbInfoFilesizeOnDisk:
            if ( sizeof( QWORD ) != cbMax )
            {
                Error( ErrERRCheck( JET_errInvalidBufferSize ) );
            }
            ProbeClientBuffer( pvResult, cbMax );

            memcpy( (BYTE*)pvResult, (BYTE*)&cbFileSizeOnDisk, sizeof( QWORD ) );
            break;

        case JET_DbInfoUpgrade:
        {
            if ( sizeof(JET_DBINFOUPGRADE ) != cbMax )
            {
                Error( ErrERRCheck( JET_errInvalidBufferSize ) );
            }

            JET_DBINFOUPGRADE   *pdbinfoupgd    = (JET_DBINFOUPGRADE *)pvResult;

            if ( sizeof(JET_DBINFOUPGRADE) != pdbinfoupgd->cbStruct )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            memset( pdbinfoupgd, 0, sizeof(JET_DBINFOUPGRADE) );
            pdbinfoupgd->cbStruct = sizeof(JET_DBINFOUPGRADE);

            pdbinfoupgd->cbFilesizeLow  = DWORD( cbFileSize );
            pdbinfoupgd->cbFilesizeHigh = DWORD( cbFileSize >> 32 );

            pdbinfoupgd->csecToUpgrade = ULONG( ( cbFileSize * 3600 ) >> 30 );
            cbFileSize = ( cbFileSize * 10 ) >> 6;
            pdbinfoupgd->cbFreeSpaceRequiredLow     = DWORD( cbFileSize );
            pdbinfoupgd->cbFreeSpaceRequiredHigh    = DWORD( cbFileSize >> 32 );

            Alloc( pdbfilehdr = (DBFILEHDR_FIX * )PvOSMemoryPageAlloc( g_cbPage, NULL ) );

            memset( pdbfilehdr, 0, g_cbPage );

            Assert( !pdbinfoupgd->fUpgradable );
            Assert( !pdbinfoupgd->fAlreadyUpgraded );

            err = ErrUtilReadShadowedHeader(
                        pinstNil,
                        pfsapi,
                        wszFullDbName,
                        (BYTE*)pdbfilehdr,
                        g_cbPage,
                        OffsetOf( DBFILEHDR_FIX, le_cbPageSize ),
                        urhfReadOnly );
            Assert( err <= JET_errSuccess );

            err = JET_errSuccess;

            if ( ulDAEMagic == pdbfilehdr->le_ulMagic )
            {
                if ( pdbfilehdr->le_ulVersion == ulDAEVersionMax )
                {
                    Assert( !pdbinfoupgd->fUpgradable );
                    pdbinfoupgd->fAlreadyUpgraded = fTrue;
                    if ( pdbfilehdr->Dbstate() == JET_dbstateBeingConverted )
                        err = ErrERRCheck( JET_errDatabaseIncompleteUpgrade );
                }
                else
                {
                    switch ( pdbfilehdr->le_ulVersion )
                    {
                        case ulDAEVersion500:
                        case ulDAEVersion550:
                            pdbinfoupgd->fUpgradable = fTrue;
                            Assert( !pdbinfoupgd->fAlreadyUpgraded );
                            break;
                        case ulDAEVersion400:
                        {
                            WCHAR wszDbPath[IFileSystemAPI::cchPathMax];
                            WCHAR wszDbFileName[IFileSystemAPI::cchPathMax];
                            WCHAR wszDbFileExt[IFileSystemAPI::cchPathMax];

                            Call( pfsapi->ErrPathParse( wszFullDbName, wszDbPath, wszDbFileName, wszDbFileExt ) );
                            OSStrCbAppendW( wszDbFileName, sizeof(wszDbFileName), wszDbFileExt );

                            pdbinfoupgd->fUpgradable = ( 0 == _wcsicmp( wszDbFileName, L"dir.edb" ) ? fTrue : fFalse );
                            Assert( !pdbinfoupgd->fAlreadyUpgraded );
                            break;
                        }
                        default:
                            Assert( !pdbinfoupgd->fUpgradable );
                            Assert( !pdbinfoupgd->fAlreadyUpgraded );
                            break;
                    }
                }
            }

            break;
        }

        case JET_DbInfoMisc:
        case JET_DbInfoPageSize:
        case JET_DbInfoFileType:
        {
            ULONG cbPageHeader = 0;

            if (    (   InfoLevel == JET_DbInfoMisc &&
                        (   sizeof( JET_DBINFOMISC ) != cbMax &&
                            sizeof( JET_DBINFOMISC2 ) != cbMax &&
                            sizeof( JET_DBINFOMISC3 ) != cbMax &&
                            sizeof( JET_DBINFOMISC4 ) != cbMax &&
                            sizeof( JET_DBINFOMISC5 ) != cbMax &&
                            sizeof( JET_DBINFOMISC6 ) != cbMax &&
                            sizeof( JET_DBINFOMISC7 ) != cbMax ) ) ||
                    ( InfoLevel == JET_DbInfoPageSize && sizeof( ULONG ) != cbMax ) ||
                    ( InfoLevel == JET_DbInfoFileType && sizeof( LONG ) != cbMax ) )
            {
                Error( ErrERRCheck( JET_errInvalidBufferSize ) );
            }
            ProbeClientBuffer( pvResult, cbMax );

            Alloc( pdbfilehdr = (DBFILEHDR * )PvOSMemoryPageAlloc( sizeof( DBFILEHDR ), NULL ) );

            BOOL fHaveHeader = fFalse;
            if ( g_rgfmp )
            {
                FMP::EnterFMPPoolAsReader();

                for ( IFMP ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
                {
                    FMP * pfmp = &g_rgfmp[ifmp];

                    if ( !pfmp->FInUse() )
                    {
                        continue;
                    }

                    if ( 0 != UtilCmpFileName( pfmp->WszDatabaseName(), wszFullDbName ) )
                    {
                        continue;
                    }

                    if ( pfmp->Pdbfilehdr() == NULL )
                    {
                        continue;
                    }

                    UtilMemCpy( (BYTE *)pdbfilehdr, pfmp->Pdbfilehdr().get(), sizeof( DBFILEHDR ) );
                    const ULONG cbPageSize = pfmp->Pdbfilehdr()->le_cbPageSize;
                    cbPageHeader = cbPageSize ? cbPageSize : g_cbPageDefault;
                    fHaveHeader = fTrue;
                    break;
                }

                FMP::LeaveFMPPoolAsReader();
            }

            if ( !fHaveHeader )
            {
                Call( ErrUtilReadShadowedHeader(    pinstNil,
                                                    pfsapi,
                                                    wszFullDbName,
                                                    (BYTE *)pdbfilehdr,
                                                    sizeof( DBFILEHDR ),
                                                    OffsetOf( DBFILEHDR_FIX, le_cbPageSize ),
                                                    UtilReadHeaderFlags( urhfReadOnly | urhfNoEventLogging | urhfNoFailOnPageMismatch ),
                                                    &cbPageHeader ) );
                fHaveHeader = fTrue;
            }

            switch ( InfoLevel )
            {
                case JET_DbInfoMisc:
                    UtilLoadDbinfomiscFromPdbfilehdr( ( JET_DBINFOMISC7* )pvResult, cbMax, ( DBFILEHDR_FIX* )pdbfilehdr );
                    break;
                case JET_DbInfoPageSize:
                    *( ULONG *)pvResult = cbPageHeader;
                    break;
                case JET_DbInfoFileType:
                {
                    ULONG fileType = pdbfilehdr->le_filetype;
                    if ( fileType >= JET_filetypeMax )
                    {
                        fileType = JET_filetypeUnknown;
                    }
                    Assert( fileType <= lMax );
                    *(LONG *)pvResult = (LONG)fileType;
                }
                    break;
                default:
                    Assert( fFalse );
            }

            break;
        }

        case JET_DbInfoDBInUse:
        {

            if ( sizeof( BOOL ) != cbMax )
            {
                Error( ErrERRCheck( JET_errInvalidBufferSize ) );
            }

            *(BOOL *)pvResult = fFalse;

            if ( NULL == g_rgfmp)
            {
                break;
            }

            FMP::EnterFMPPoolAsReader();

            for ( IFMP ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
            {
                FMP * pfmp = &g_rgfmp[ ifmp ];

                if ( !pfmp->FInUse() )
                {
                    continue;
                }

                if ( 0 != UtilCmpFileName( pfmp->WszDatabaseName(), wszFullDbName ) )
                {
                    continue;
                }

                *(BOOL *)pvResult = fTrue;
                break;
            }

            FMP::LeaveFMPPoolAsReader();
        }
            break;
        default:
             err = ErrERRCheck( JET_errInvalidParameter );
    }

HandleError:
    OSMemoryPageFree( pdbfilehdr );

    delete pffapi;
    delete pfsapi;

    OSUTerm();

    return err;
}

LOCAL JET_ERR JetGetDatabaseFileInfoExA(
    __in JET_PCSTR                  szDatabaseName,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    ERR             err;
    CAutoWSZPATH    lszDatabaseName;

    CallR( lszDatabaseName.ErrSet( szDatabaseName ) );

    return JetGetDatabaseFileInfoEx( lszDatabaseName, pvResult, cbMax, InfoLevel );
}

LOCAL JET_ERR JetRemoveLogfileExW(
    _In_z_ const WCHAR * const wszDatabase,
    _In_z_ const WCHAR * const wszLogfile,
    _In_ const JET_GRBIT    grbit)
{
    JET_ERR err = JET_errSuccess;

    BOOL    fOSUInitCalled  = fFalse;
    IFileSystemAPI* pfsapi  = NULL;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p[%s],0x%p[%s],0x%x)",
            __FUNCTION__,
            wszDatabase,
            SzOSFormatStringW( wszDatabase ),
            wszLogfile,
            SzOSFormatStringW( wszLogfile ),
            grbit) );


    Assert( fFalse == fOSUInitCalled );

    Call( ErrOSUInit() );
    fOSUInitCalled = fTrue;
    Call( ErrOSFSCreate( g_pfsconfigGlobal, &pfsapi ) );


    Call( ErrIsamRemoveLogfile( pfsapi, wszDatabase, wszLogfile, grbit ) );

HandleError:
    delete pfsapi;
    if( fOSUInitCalled )
    {
        OSUTerm();
    }
    return err;
}

LOCAL JET_ERR JetRemoveLogfileExA(
    _In_z_ const char * const szDatabase,
    _In_z_ const char   * const szLogfile,
    _In_ const JET_GRBIT    grbit)
{
    ERR err = JET_errSuccess;

    CAutoWSZPATH    lwszDatabase;
    CAutoWSZPATH    lwszLogfile;

    CallR( lwszDatabase.ErrSet( szDatabase ) );
    CallR( lwszLogfile.ErrSet( szLogfile ) );

    return JetRemoveLogfileExW( lwszDatabase, lwszLogfile, grbit );
}

JET_ERR JET_API JetRemoveLogfileW(
    _In_z_ JET_PCWSTR wszDatabase,
    _In_z_ JET_PCWSTR wszLogfile,
    _In_ JET_GRBIT grbit )
{
    JET_TRY( opRemoveLogfile, JetRemoveLogfileExW( wszDatabase, wszLogfile, grbit ) );
}

JET_ERR JET_API JetRemoveLogfileA(
    _In_z_ JET_PCSTR szDatabase,
    _In_z_ JET_PCSTR szLogfile,
    _In_ JET_GRBIT grbit )
{
    JET_TRY( opRemoveLogfile, JetRemoveLogfileExA( szDatabase, szLogfile, grbit ) );
}

LOCAL
JET_ERR
JetBeginDatabaseIncrementalReseedEx(
    __in JET_INSTANCE   instance,
    __in JET_PCWSTR     wszDatabase,
    __in JET_GRBIT      grbit )
{
    APICALL_INST    apicall( opBeginDatabaseIncrementalReseed );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p[%s],0x%x)",
            __FUNCTION__,
            instance,
            wszDatabase,
            SzOSFormatStringW( wszDatabase ),
            grbit ) );

    if ( apicall.FEnterWithoutInit( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamBeginDatabaseIncrementalReseed( (JET_INSTANCE)apicall.Pinst(), wszDatabase, grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR
JET_API
JetBeginDatabaseIncrementalReseedW(
    __in JET_INSTANCE   instance,
    __in JET_PCWSTR     wszDatabase,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBeginDatabaseIncrementalReseed, JetBeginDatabaseIncrementalReseedEx( instance, wszDatabase, grbit ) );
}

LOCAL
JET_ERR
JetBeginDatabaseIncrementalReseedExA(
    __in JET_INSTANCE   instance,
    __in JET_PCSTR      szDatabase,
    __in JET_GRBIT      grbit )
{
    ERR             err             = JET_errSuccess;
    CAutoWSZPATH    lwszDatabase;

    CallR( lwszDatabase.ErrSet( szDatabase ) );

    return JetBeginDatabaseIncrementalReseedEx( instance, lwszDatabase, grbit );
}

JET_ERR
JET_API
JetBeginDatabaseIncrementalReseedA(
    __in JET_INSTANCE   instance,
    __in JET_PCSTR      szDatabase,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBeginDatabaseIncrementalReseed, JetBeginDatabaseIncrementalReseedExA( instance, szDatabase, grbit ) );
}

LOCAL
JET_ERR
JetEndDatabaseIncrementalReseedEx(
    __in JET_INSTANCE   instance,
    __in JET_PCWSTR     wszDatabase,
    __in ULONG  genMinRequired,
    __in ULONG  genFirstDivergedLog,
    __in ULONG  genMaxRequired,
    __in JET_GRBIT      grbit )
{
    APICALL_INST    apicall( opEndDatabaseIncrementalReseed );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p[%s],0x%x,0x%x,0x%x)",
            __FUNCTION__,
            instance,
            wszDatabase,
            SzOSFormatStringW( wszDatabase ),
            genMinRequired,
            genMaxRequired,
            grbit ) );

    if ( apicall.FEnterWithoutInit( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamEndDatabaseIncrementalReseed( (JET_INSTANCE)apicall.Pinst(), wszDatabase, genMinRequired, genFirstDivergedLog, genMaxRequired, grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR
JET_API
JetEndDatabaseIncrementalReseedW(
    __in JET_INSTANCE   instance,
    __in JET_PCWSTR     wszDatabase,
    __in ULONG  genMinRequired,
    __in ULONG  genFirstDivergedLog,
    __in ULONG  genMaxRequired,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opEndDatabaseIncrementalReseed, JetEndDatabaseIncrementalReseedEx( instance, wszDatabase, genMinRequired, genFirstDivergedLog, genMaxRequired, grbit ) );
}

LOCAL
JET_ERR
JetEndDatabaseIncrementalReseedExA(
    __in JET_INSTANCE   instance,
    __in JET_PCSTR      szDatabase,
    __in ULONG  genMinRequired,
    __in ULONG  genFirstDivergedLog,
    __in ULONG  genMaxRequired,
    __in JET_GRBIT      grbit )
{
    ERR             err             = JET_errSuccess;
    CAutoWSZPATH    lwszDatabase;

    CallR( lwszDatabase.ErrSet( szDatabase ) );

    return JetEndDatabaseIncrementalReseedEx( instance, lwszDatabase, genMinRequired, genFirstDivergedLog, genMaxRequired, grbit );
}

JET_ERR
JET_API
JetEndDatabaseIncrementalReseedA(
    __in JET_INSTANCE   instance,
    __in JET_PCSTR      szDatabase,
    __in ULONG  genMinRequired,
    __in ULONG  genFirstDivergedLog,
    __in ULONG  genMaxRequired,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opEndDatabaseIncrementalReseed, JetEndDatabaseIncrementalReseedExA( instance, szDatabase, genMinRequired, genFirstDivergedLog, genMaxRequired, grbit ) );
}


LOCAL
JET_ERR
JetPatchDatabasePagesEx(
    __in JET_INSTANCE               instance,
    __in JET_PCWSTR                 wszDatabase,
    __in ULONG              pgnoStart,
    __in ULONG              cpg,
    __in_bcount( cb ) const void *  pv,
    __in ULONG              cb,
    __in JET_GRBIT                  grbit )
{
    APICALL_INST    apicall( opPatchDatabasePages );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,%ls,0x%x,0x%x,0x%p,0x%x,0x%x)",
            __FUNCTION__,
            instance,
            wszDatabase,
            pgnoStart,
            cpg,
            pv,
            cb,
            grbit) );

    if ( apicall.FEnterWithoutInit( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamPatchDatabasePages(
                                        instance,
                                        wszDatabase,
                                        pgnoStart,
                                        cpg,
                                        pv,
                                        cb,
                                        grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR
JET_API
JetPatchDatabasePagesW(
    __in JET_INSTANCE               instance,
    __in JET_PCWSTR                 wszDatabase,
    __in ULONG              pgnoStart,
    __in ULONG              cpg,
    __in_bcount( cb ) const void *  pv,
    __in ULONG              cb,
    __in JET_GRBIT                  grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opPatchDatabasePages, JetPatchDatabasePagesEx( instance, wszDatabase, pgnoStart, cpg, pv, cb, grbit ) );
}

LOCAL
JET_ERR
JetPatchDatabasePagesExA(
    __in JET_INSTANCE               instance,
    __in JET_PCSTR                  szDatabase,
    __in ULONG              pgnoStart,
    __in ULONG              cpg,
    __in_bcount( cb ) const void *  pv,
    __in ULONG              cb,
    __in JET_GRBIT                  grbit )
{
    ERR             err             = JET_errSuccess;
    CAutoWSZPATH    lwszDatabase;

    CallR( lwszDatabase.ErrSet( szDatabase ) );

    return JetPatchDatabasePagesEx( instance, lwszDatabase, pgnoStart, cpg, pv, cb, grbit );
}

JET_ERR
JET_API
JetPatchDatabasePagesA(
    __in JET_INSTANCE               instance,
    __in JET_PCSTR                  szDatabase,
    __in ULONG              pgnoStart,
    __in ULONG              cpg,
    __in_bcount( cb ) const void *  pv,
    __in ULONG              cb,
    __in JET_GRBIT                  grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opPatchDatabasePages, JetPatchDatabasePagesExA( instance, szDatabase, pgnoStart, cpg, pv, cb, grbit ) );
}


JET_ERR JET_API JetGetDatabaseFileInfoA(
    __in JET_PCSTR                  szDatabaseName,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_TRY( opGetDatabaseFileInfo, JetGetDatabaseFileInfoExA( szDatabaseName, pvResult, cbMax, InfoLevel ) );
}

JET_ERR JET_API JetGetDatabaseFileInfoW(
    __in JET_PCWSTR                 wszDatabaseName,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_TRY( opGetDatabaseFileInfo, JetGetDatabaseFileInfoEx( wszDatabaseName, pvResult, cbMax, InfoLevel ) );
}

LOCAL JET_ERR JetGetLogFileInfoEx(
    __in JET_PCWSTR                 wszLog,
    __out_bcount( cbMax ) void *    pvResult,
    __in const ULONG        cbMax,
    __in const ULONG        InfoLevel )
{
    ERR                 err                 = JET_errSuccess;
    LGFILEHDR *         plgfilehdr          = NULL;
    IFileSystemAPI *    pfsapi              = NULL;
    IFileAPI *          pfapi               = NULL;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p[%s],0x%p,0x%x,0x%x)",
            __FUNCTION__,
            wszLog,
            SzOSFormatStringW( wszLog ),
            pvResult,
            cbMax,
            InfoLevel ) );

    CallR( ErrOSUInit() );

    Call( ErrOSFSCreate( g_pfsconfigGlobal, &pfsapi ) );
    Call( pfsapi->ErrFileOpen(
        wszLog,
        IFileAPI::fmfReadOnlyPermissive | ( BoolParam( JET_paramEnableFileCache ) ? IFileAPI::fmfCached : IFileAPI::fmfNone ),
        &pfapi ) );

    switch ( InfoLevel )
    {
        case JET_LogInfoMisc:
        case JET_LogInfoMisc2:
        case JET_LogInfoMisc3:
            ProbeClientBuffer( pvResult, cbMax );

            Alloc( plgfilehdr = (LGFILEHDR * )PvOSMemoryPageAlloc( sizeof(LGFILEHDR), NULL ) );

            Call( ErrLGIReadFileHeader( pfapi, *TraceContextScope( iorpDirectAccessUtil ), qosIONormal, plgfilehdr ) );

            Call( ErrGetLogInfoMiscFromLgfilehdr( plgfilehdr, pvResult, cbMax, InfoLevel ) );
            break;

        default:
            Error( ErrERRCheck( JET_errInvalidParameter ) );

    }

    CallS( err );

HandleError:
    if ( NULL != plgfilehdr )
    {
        OSMemoryPageFree( plgfilehdr );
    }

    delete pfapi;
    delete pfsapi;

    OSUTerm();

    return err;
}

JET_ERR JET_API JetGetLogFileInfoExA(
    __in JET_PCSTR                  szLog,
    __out_bcount( cbMax ) void *    pvResult,
    __in const ULONG        cbMax,
    __in const ULONG        InfoLevel )
{
        ERR err = JET_errSuccess;
    CAutoWSZPATH    lwszLogFileName;

    CallR( lwszLogFileName.ErrSet( szLog ) );

    return JetGetLogFileInfoEx( lwszLogFileName, pvResult, cbMax, InfoLevel );
}

JET_ERR JET_API JetGetLogFileInfoA(
    __in JET_PCSTR                  szLog,
    __out_bcount( cbMax ) void *    pvResult,
    __in const ULONG        cbMax,
    __in const ULONG        InfoLevel )
{
    JET_TRY( opGetLogFileInfo, JetGetLogFileInfoExA( szLog, pvResult, cbMax, InfoLevel ) );
}

JET_ERR JET_API JetGetLogFileInfoW(
    __in JET_PCWSTR                 wszLog,
    __out_bcount( cbMax ) void *    pvResult,
    __in const ULONG        cbMax,
    __in const ULONG        InfoLevel )
{
    JET_TRY( opGetLogFileInfo, JetGetLogFileInfoEx( wszLog, pvResult, cbMax, InfoLevel ) );
}


LOCAL JET_ERR JetGetPageInfoEx(
    __in_bcount(cbData)         void * const            pvPages,
                                const ULONG cbData,
    __inout_bcount(cbPageInfo)  void* const         rgPageInfo,
                                const ULONG cbPageInfo,
                                const JET_GRBIT     grbit,
                                ULONG       ulInfoLevel )
{
    JET_ERR err;
    ULONG cbStruct = 0;

    switch ( ulInfoLevel )
    {
        case JET_PageInfo:
            cbStruct = sizeof( JET_PAGEINFO );
            break;
        case JET_PageInfo2:
            cbStruct = sizeof( JET_PAGEINFO2 );
            break;
        default:
            Error( ErrERRCheck( JET_errInvalidParameter ) );
            break;
    }

    ULONG cPageInfo = cbPageInfo / cbStruct;

    if( 0 == pvPages || 0 == cbData || 0 == rgPageInfo || 0 == cbPageInfo )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if( 0 != ( ~( JET_bitPageInfoNoStructureChecksum ) & grbit ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    const BOOL fComputeStructureChecksum = ( JET_bitPageInfoNoStructureChecksum & grbit ) == 0;

    if( 0 != ( cbData % g_cbPage ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if( cbPageInfo % cbStruct )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if( ( cbData / g_cbPage ) != cPageInfo )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if( ( cbStruct * ( cbData / g_cbPage ) ) > cbPageInfo )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Assert( cbStruct  * ( cbData / g_cbPage ) == cbPageInfo );

    if( 0 != ( (unsigned __int64)pvPages % g_cbPage ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    BYTE* pbStruct = ( BYTE* )rgPageInfo;
    for( ULONG ipageinfo = 0; ipageinfo < cPageInfo; ++ipageinfo, pbStruct += cbStruct )
    {
        JET_PAGEINFO* const pInfo = ( JET_PAGEINFO* )pbStruct;

        pInfo->fPageIsInitialized = fFalse;
        pInfo->fCorrectableError = fFalse;
        pInfo->checksumActual = 0;
        pInfo->checksumExpected = 0;
        pInfo->dbtime = (unsigned __int64)dbtimeInvalid;
        pInfo->structureChecksum = 0;
        pInfo->flags = 0;

        if ( sizeof( JET_PAGEINFO2 ) <= cbStruct )
        {
            JET_PAGEINFO2* const pInfo2 = ( JET_PAGEINFO2* )pbStruct;
            memset( pInfo2->rgChecksumActual, 0, sizeof( pInfo2->rgChecksumActual ) );
            memset( pInfo2->rgChecksumExpected, 0, sizeof( pInfo2->rgChecksumExpected ) );
        }
    }


    err = JET_errSuccess;

    pbStruct = ( BYTE* )rgPageInfo;
    for( UINT ipageinfo = 0; ipageinfo < cPageInfo; ++ipageinfo, pbStruct += cbStruct )
    {
        VOID * const    pvPage  = (VOID *)(((BYTE *)pvPages) + ( ipageinfo * g_cbPage ));
        AssertPREFIX( pvPage >= pvPages );
        AssertPREFIX( pvPage < ( (BYTE *)pvPages + cbData ) );
        AssertPREFIX( pbStruct < ( (BYTE *)rgPageInfo + cbPageInfo ) );

        CPAGE   cpage;
        cpage.LoadPage( pvPage, g_cbPage );

        JET_PAGEINFO* const pInfo = ( JET_PAGEINFO* )pbStruct;
        pInfo->fPageIsInitialized = cpage.FPageIsInitialized();

        if( pInfo->fPageIsInitialized )
        {
            const DWORD * const     rgdwPage        = (DWORD *)pvPage;
            PGNO                    pgno            = pInfo->pgno;
            PAGETYPE                pagetype        = databasePage;
            PAGECHECKSUM            checksumExpected;
            PAGECHECKSUM            checksumActual;
            BOOL                    fCorrectableError;
            INT                     ibitCorrupted;

            if ( ( 0 == rgdwPage[1]
                && 0 == rgdwPage[2]
                && 0 == rgdwPage[3]
                && 0 == rgdwPage[6] )
                    || pgnoNull == pgno
                    || pgnoMax == pgno )
        {
                pagetype = databaseHeader;
                pgno = pgnoNull;
        }

            ChecksumAndPossiblyFixPage(
                pvPage,
                g_cbPage,
                pagetype,
                pgno,
                fTrue,
                &checksumExpected,
                &checksumActual,
                &fCorrectableError,
                &ibitCorrupted );

            pInfo->fCorrectableError = !!fCorrectableError;
            pInfo->checksumActual = checksumActual.rgChecksum[ 0 ];
            pInfo->checksumExpected = checksumExpected.rgChecksum[ 0 ];

            if ( sizeof( JET_PAGEINFO2 ) <= cbStruct )
            {
                JET_PAGEINFO2* const pInfo2 = ( JET_PAGEINFO2* )pbStruct;
                memcpy( pInfo2->rgChecksumActual, &checksumActual.rgChecksum[ 1 ], sizeof( pInfo2->rgChecksumActual ) );
                memcpy( pInfo2->rgChecksumExpected, &checksumExpected.rgChecksum[ 1 ], sizeof( pInfo2->rgChecksumExpected ) );
            }

            if( checksumActual != checksumExpected )
            {
                err = ErrERRCheck( JET_errReadVerifyFailure );
            }
            else if ( databasePage == pagetype )
            {
                pInfo->dbtime = (unsigned __int64)(cpage.Dbtime());
                if ( fComputeStructureChecksum )
                {
                    pInfo->structureChecksum = cpage.LoggedDataChecksum().rgChecksum[ 0 ];
                }
            }
            else if ( databaseHeader == pagetype )
            {
                pInfo->dbtime = ((DBFILEHDR_FIX*)pvPage)->le_dbtimeDirtied;
                if ( fComputeStructureChecksum )
                {
                    pInfo->structureChecksum = pInfo->checksumActual;
                }
            }
            else
            {
                AssertSz( fFalse, "Please, decide what to do for dbtime, structureChecksum and flags!" );
            }
        }
    }

HandleError:
    return err;
}

JET_ERR JET_API JetGetPageInfo(
    __in_bcount( cbData ) void * const          pvPages,
    __in ULONG                          cbData,
    __inout_bcount( cbPageInfo ) JET_PAGEINFO * rgPageInfo,
    __in ULONG                          cbPageInfo,
    __in JET_GRBIT                              grbit,
    __in ULONG                          ulInfoLevel )
{
    if ( JET_PageInfo != ulInfoLevel )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    JET_TRY( opGetPageInfo, JetGetPageInfoEx( pvPages, cbData, rgPageInfo, cbPageInfo, grbit, JET_PageInfo ) );
}

JET_ERR JET_API JetGetPageInfo2(
    __in_bcount( cbData ) void * const          pvPages,
    __in ULONG                          cbData,
    __inout_bcount( cbPageInfo ) void * const   rgPageInfo,
    __in ULONG                          cbPageInfo,
    __in JET_GRBIT                              grbit,
    __in ULONG                          ulInfoLevel )
{
    JET_TRY( opGetPageInfo, JetGetPageInfoEx( pvPages, cbData, rgPageInfo, cbPageInfo, grbit, ulInfoLevel ) );
}

LOCAL
JET_ERR
JetGetDatabasePagesEx(
    __in JET_SESID                              sesid,
    __in JET_DBID                               dbid,
    __in ULONG                          pgnoStart,
    __in ULONG                          cpg,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    __in ULONG                          cb,
    __out ULONG *                       pcbActual,
    __in JET_GRBIT                              grbit )
{
    APICALL_SESID   apicall( opGetDatabasePages );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%x,0x%x,0x%p,0x%x,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            pgnoStart,
            cpg,
            pv,
            cb,
            pcbActual,
            grbit) );

    if ( apicall.FEnter( sesid ) )
    {
        ProbeClientBuffer( pv, cb );
        apicall.LeaveAfterCall( ErrIsamGetDatabasePages(
                                        sesid,
                                        dbid,
                                        pgnoStart,
                                        cpg,
                                        pv,
                                        cb,
                                        pcbActual,
                                        grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR
JET_API
JetGetDatabasePages(
    __in JET_SESID                              sesid,
    __in JET_DBID                               dbid,
    __in ULONG                          pgnoStart,
    __in ULONG                          cpg,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    __in ULONG                          cb,
    __out ULONG *                       pcbActual,
    __in JET_GRBIT                              grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetDatabasePages, JetGetDatabasePagesEx( sesid, dbid, pgnoStart, cpg, pv, cb, pcbActual, grbit ) );
}

JET_ERR
JET_API
JetOnlinePatchDatabasePageEx(
    __in JET_SESID                              sesid,
    __in JET_DBID                               dbid,
    __in ULONG                          pgno,
    __in_bcount(cbToken)    const void *        pvToken,
    __in ULONG                          cbToken,
    __in_bcount(cbData) const void *            pvData,
    __in ULONG                          cbData,
    __in JET_GRBIT                              grbit )
{
    APICALL_SESID   apicall( opOnlinePatchDatabasePage );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%x,0x%p,0x%x,0x%p,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            pgno,
            pvToken,
            cbToken,
            pvData,
            cbData,
            grbit) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        apicall.LeaveAfterCall( ErrIsamOnlinePatchDatabasePage(
                                        sesid,
                                        dbid,
                                        pgno,
                                        pvToken,
                                        cbToken,
                                        pvData,
                                        cbData,
                                        grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR
JET_API
JetOnlinePatchDatabasePage(
    __in JET_SESID                              sesid,
    __in JET_DBID                               dbid,
    __in ULONG                          pgno,
    __in_bcount(cbToken)    const void *        pvToken,
    __in ULONG                          cbToken,
    __in_bcount(cbData) const void *            pvData,
    __in ULONG                          cbData,
    __in JET_GRBIT                              grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOnlinePatchDatabasePage, JetOnlinePatchDatabasePageEx( sesid, dbid, pgno, pvToken, cbToken, pvData, cbData, grbit ) );
}


LOCAL JET_ERR JetCreateTableEx(
    __in JET_SESID      sesid,
    __in JET_DBID       dbid,
    __in JET_PCSTR      szTableName,
    __in ULONG  lPages,
    __in ULONG  lDensity,
    __out JET_TABLEID * ptableid )
{
    APICALL_SESID       apicall( opCreateTable );
    JET_TABLECREATE5_A  tablecreate =
                            {   sizeof(JET_TABLECREATE5_A),
                                (CHAR *)szTableName,
                                NULL,
                                lPages,
                                lDensity,
                                NULL,
                                0,
                                NULL,
                                0,
                                NULL,
                                0,
                                0,
                                NULL,
                                NULL,
                                0,
                                0,
                                JET_tableidNil,
                                0
                            };

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p[%s],0x%x,0x%x,0x%p)",
            __FUNCTION__,
            sesid,
            dbid,
            szTableName,
            OSFormatString( szTableName ),
            lPages,
            lDensity,
            ptableid ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        apicall.LeaveAfterCall( ErrIsamCreateTable(
                                        sesid,
                                        dbid,
                                        &tablecreate ) );

        *ptableid = tablecreate.tableid;

        Assert( 0 == tablecreate.cCreated || 1 == tablecreate.cCreated );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetCreateTableExW(
    __in JET_SESID      sesid,
    __in JET_DBID       dbid,
    __in JET_PCWSTR     wszTableName,
    __in ULONG  lPages,
    __in ULONG  lDensity,
    __out JET_TABLEID * ptableid )
{
    ERR             err             = JET_errSuccess;
    CAutoSZDDL      lszTableName;

    CallR( lszTableName.ErrSet( wszTableName ) );

    return JetCreateTableEx( sesid, dbid, lszTableName, lPages, lDensity, ptableid );
}

JET_ERR JET_API JetCreateTableA(
    __in JET_SESID      sesid,
    __in JET_DBID       dbid,
    __in JET_PCSTR      szTableName,
    __in ULONG  lPages,
    __in ULONG  lDensity,
    __out JET_TABLEID * ptableid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTable, JetCreateTableEx( sesid, dbid, szTableName, lPages, lDensity, ptableid ) );
}

JET_ERR JET_API JetCreateTableW(
    __in JET_SESID      sesid,
    __in JET_DBID       dbid,
    __in JET_PCWSTR     wszTableName,
    __in ULONG  lPages,
    __in ULONG  lDensity,
    __out JET_TABLEID * ptableid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTable, JetCreateTableExW( sesid, dbid, wszTableName, lPages, lDensity, ptableid ) );
}


class CAutoIDXCREATE2
{
    public:
        CAutoIDXCREATE2():m_pindexcreate( NULL ),m_pcondcol( NULL ), m_rgconditionalcolumn( NULL ),
            m_pindexcreate1W( NULL ), m_pindexcreate2W( NULL ) { }
        ~CAutoIDXCREATE2();

    public:
        ERR ErrSet( JET_INDEXCREATE_W * pindexcreate1 );
        ERR ErrSet( JET_INDEXCREATE2_W * pindexcreate2 );
        ERR ErrSet( JET_INDEXCREATE3_W * pindexcreate3 );
        void Result( );
        operator JET_INDEXCREATE2_A*()      { return m_pindexcreate; }

    private:
        ERR _ErrSet( JET_INDEXCREATE_W * pindexcreate );

        class CAutoCONDCOL
        {
            public:
                CAutoCONDCOL() { m_condcol.cbStruct = sizeof(JET_CONDITIONALCOLUMN_A); }
                ~CAutoCONDCOL() { }

            public:
                ERR ErrSet( const JET_CONDITIONALCOLUMN_W * const pcondcol );
                operator JET_CONDITIONALCOLUMN_A*()     { return &m_condcol; }

            private:
                JET_CONDITIONALCOLUMN_A     m_condcol;
                CAutoSZDDL                  m_szColumnName;
        };

    private:
        JET_INDEXCREATE_W       * m_pindexcreate1W;
        JET_INDEXCREATE2_W       * m_pindexcreate2W;
        JET_INDEXCREATE3_W       * m_pindexcreate3W;
        JET_INDEXCREATE2_A       * m_pindexcreate;
        CAutoSZDDL                m_szIndexName;
        CAutoSZDDL                m_szKey;
        CAutoCONDCOL            * m_pcondcol;
        JET_CONDITIONALCOLUMN_A * m_rgconditionalcolumn;
        JET_UNICODEINDEX          m_idxunicode;
};

ERR CAutoIDXCREATE2::CAutoCONDCOL::ErrSet( const JET_CONDITIONALCOLUMN_W * const pcondcol )
{
    ERR err = JET_errSuccess;

    C_ASSERT( sizeof(JET_CONDITIONALCOLUMN_W) == sizeof(JET_CONDITIONALCOLUMN_A) );

    if ( !pcondcol || sizeof( JET_CONDITIONALCOLUMN_W ) !=  pcondcol->cbStruct )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Assert( sizeof(JET_CONDITIONALCOLUMN_A) == m_condcol.cbStruct );

    Call( m_szColumnName.ErrSet( pcondcol->szColumnName ) );
    m_condcol.szColumnName = m_szColumnName;

    m_condcol.grbit = pcondcol->grbit;

HandleError:
    return err;
}




ERR CAutoIDXCREATE2::_ErrSet( JET_INDEXCREATE_W * pindexcreate )
{
    ERR                 err         = JET_errSuccess;

    if ( pindexcreate )
    {
        if ( sizeof(JET_INDEXCREATE_W) != pindexcreate->cbStruct &&
                sizeof(JET_INDEXCREATE2_W) != pindexcreate->cbStruct )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        if ( pindexcreate->cbKey == 0 )
        {
            CallR( ErrERRCheck( JET_errInvalidParameter ) );
        }

        if ( pindexcreate->cbKey < sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate->szKey ) + 1 ) )
        {
            CallR( ErrERRCheck( JET_errIndexInvalidDef ) );
        }

        if ( ( pindexcreate->szKey[0] != L'+' && pindexcreate->szKey[0] != L'-' ) ||
            pindexcreate->szKey[ ( pindexcreate->cbKey / sizeof( WCHAR ) ) - 1] != L'\0' ||
            pindexcreate->szKey[ ( pindexcreate->cbKey / sizeof( WCHAR ) ) - 2] != L'\0' )
        {
            CallR( ErrERRCheck( JET_errIndexInvalidDef ) );
        }
    }

    delete[] m_pcondcol;
    delete[] m_rgconditionalcolumn;
    delete m_pindexcreate;

    m_pcondcol = NULL;
    m_rgconditionalcolumn = NULL;
    m_pindexcreate = NULL;
    m_pindexcreate1W = NULL;
    m_pindexcreate2W = NULL;
    m_pindexcreate3W = NULL;

    if ( !pindexcreate )
    {
        return JET_errSuccess;
    }

    Alloc( m_pindexcreate = (JET_INDEXCREATE2_A*)new JET_INDEXCREATE2_A );
    memset( m_pindexcreate, 0, sizeof(JET_INDEXCREATE2_A) );
    m_pindexcreate->cbStruct = sizeof(JET_INDEXCREATE2_A);

    Call( m_szIndexName.ErrSet( pindexcreate->szIndexName ) );
    m_pindexcreate->szIndexName = m_szIndexName;

    size_t cchActual;
    const ULONG cbAllocateMax = sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 + ( sizeof(LANGID) + 1 + 1 + sizeof( BYTE ) + 1 + 1 ) );
    Call( m_szKey.ErrAlloc( cbAllocateMax ) );
    Call( ErrOSSTRUnicodeToAsciiM( pindexcreate->szKey, m_szKey.Pv(),
                sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 ), &cchActual, OSSTR_FIXED_CONVERSION ) );
    m_pindexcreate->szKey = m_szKey;
    m_pindexcreate->cbKey = cchActual * sizeof( char );

    Assert ( pindexcreate->cbKey >= sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate->szKey ) + 1 ) );

    Assert( cbAllocateMax >= m_pindexcreate->cbKey );

    ULONG cbPastStringKey = pindexcreate->cbKey - sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate->szKey ) + 1 );
    if ( cbPastStringKey > 0 )
    {
        if ( cbPastStringKey == ( sizeof(LANGID) + ( sizeof(WCHAR) * 2 ) ) )
        {

            LANGID * pLangId = (LANGID *)( pindexcreate->szKey + LOSStrLengthMW( pindexcreate->szKey ) + 1 );

            memcpy( m_pindexcreate->szKey + m_pindexcreate->cbKey, pLangId, sizeof(LANGID) );
            m_pindexcreate->cbKey += sizeof(LANGID);

            memset( m_pindexcreate->szKey + m_pindexcreate->cbKey, '\0', 2 * sizeof(char) );
            m_pindexcreate->cbKey += 2 * sizeof(char);

        }
        else if ( cbPastStringKey == ( sizeof(LANGID) + ( sizeof(WCHAR) * 2 ) + sizeof( BYTE ) + ( sizeof(WCHAR) * 2 ) ) )
        {
            LANGID * pLangId = (LANGID *)( pindexcreate->szKey + LOSStrLengthMW( pindexcreate->szKey ) + 1 );

            memcpy( m_pindexcreate->szKey + m_pindexcreate->cbKey, pLangId, sizeof(LANGID) );
            m_pindexcreate->cbKey += sizeof(LANGID);

            memset( m_pindexcreate->szKey + m_pindexcreate->cbKey, '\0', 2 * sizeof(char) );
            m_pindexcreate->cbKey += 2 * sizeof(char);

            BYTE * pcbVarMac = ( (BYTE*) pLangId ) + ( sizeof(WCHAR) * 2 );

            memcpy( m_pindexcreate->szKey + m_pindexcreate->cbKey, pcbVarMac, sizeof(BYTE) );
            m_pindexcreate->cbKey += sizeof(BYTE);

            memset( m_pindexcreate->szKey + m_pindexcreate->cbKey, '\0', 2 * sizeof(char) );
            m_pindexcreate->cbKey += 2 * sizeof(char);

        }
        else
        {
            CallR( ErrERRCheck( JET_errIndexInvalidDef ) );
        }

    }
    Assert( cbAllocateMax >= m_pindexcreate->cbKey );

    m_pindexcreate->grbit = pindexcreate->grbit;
    m_pindexcreate->ulDensity = pindexcreate->ulDensity;
    m_pindexcreate->pidxunicode = pindexcreate->pidxunicode;
    m_pindexcreate->lcid = pindexcreate->lcid;

    m_pindexcreate->cbVarSegMac = pindexcreate->cbVarSegMac;
    m_pindexcreate->ptuplelimits = pindexcreate->ptuplelimits;

    m_pindexcreate->cConditionalColumn = pindexcreate->cConditionalColumn;
    if ( m_pindexcreate->cConditionalColumn )
    {
        Alloc( m_pcondcol = new CAutoCONDCOL[m_pindexcreate->cConditionalColumn] );
        Alloc( m_rgconditionalcolumn = new JET_CONDITIONALCOLUMN_A[m_pindexcreate->cConditionalColumn] );
        memset( m_rgconditionalcolumn, 0, m_pindexcreate->cConditionalColumn * sizeof(JET_CONDITIONALCOLUMN_A) );

        for( ULONG i = 0; i < m_pindexcreate->cConditionalColumn; i++ )
        {
            Call( m_pcondcol[i].ErrSet( pindexcreate->rgconditionalcolumn + i ) );
            memcpy( m_rgconditionalcolumn + i, (JET_CONDITIONALCOLUMN_A*)(m_pcondcol[i]), sizeof( JET_CONDITIONALCOLUMN_A ) );
        }
    }
    m_pindexcreate->rgconditionalcolumn = m_rgconditionalcolumn;

    m_pindexcreate->err = pindexcreate->err;
    m_pindexcreate->cbKeyMost = pindexcreate->cbKeyMost;


HandleError:
    return err;
}


ERR CAutoIDXCREATE2::ErrSet( JET_INDEXCREATE_W * pindexcreate1 )
{
    C_ASSERT( sizeof(JET_INDEXCREATE_W) == sizeof(JET_INDEXCREATE_A) );

    ERR err;
    CallR( _ErrSet( pindexcreate1 ) );

    m_pindexcreate->pSpacehints = NULL;

    m_pindexcreate1W = pindexcreate1;

    return err;
}

ERR CAutoIDXCREATE2::ErrSet( JET_INDEXCREATE2_W * pindexcreate2 )
{
    C_ASSERT( sizeof(JET_INDEXCREATE2_W) == sizeof(JET_INDEXCREATE2_A) );

    ERR err;
    CallR( _ErrSet( reinterpret_cast<JET_INDEXCREATE_W*>(pindexcreate2) ) );

    m_pindexcreate->pSpacehints = pindexcreate2->pSpacehints;

    m_pindexcreate2W = pindexcreate2;

    return err;
}

ERR CAutoIDXCREATE2::ErrSet( JET_INDEXCREATE3_W * pindexcreate3 )
{
    ERR                 err         = JET_errSuccess;

    if ( sizeof(JET_INDEXCREATE3_W) != pindexcreate3->cbStruct )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( pindexcreate3->cbKey == 0 )
    {
        CallR( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( pindexcreate3->cbKey < sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate3->szKey ) + 1 ) )
    {
        CallR( ErrERRCheck( JET_errIndexInvalidDef ) );
    }

    if ( ( pindexcreate3->szKey[0] != L'+' && pindexcreate3->szKey[0] != L'-' ) ||
        pindexcreate3->szKey[ ( pindexcreate3->cbKey / sizeof( WCHAR ) ) - 1] != L'\0' ||
        pindexcreate3->szKey[ ( pindexcreate3->cbKey / sizeof( WCHAR ) ) - 2] != L'\0' )
    {
        CallR( ErrERRCheck( JET_errIndexInvalidDef ) );
    }

    delete[] m_pcondcol;
    delete[] m_rgconditionalcolumn;
    delete m_pindexcreate;

    m_pcondcol = NULL;
    m_rgconditionalcolumn = NULL;
    m_pindexcreate = NULL;
    m_pindexcreate1W = NULL;
    m_pindexcreate2W = NULL;
    m_pindexcreate3W = NULL;

    Alloc( m_pindexcreate = (JET_INDEXCREATE2_A*)new JET_INDEXCREATE2_A );
    memset( m_pindexcreate, 0, sizeof(JET_INDEXCREATE2_A) );
    m_pindexcreate->cbStruct = sizeof(JET_INDEXCREATE2_A);

    Call( m_szIndexName.ErrSet( pindexcreate3->szIndexName ) );
    m_pindexcreate->szIndexName = m_szIndexName;

    size_t cchActual;
    const ULONG cbAllocateMax = sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 + ( sizeof(LANGID) + 1 + 1 + sizeof( BYTE ) + 1 + 1 ) );
    Call( m_szKey.ErrAlloc( cbAllocateMax ) );
    Call( ErrOSSTRUnicodeToAsciiM( pindexcreate3->szKey, m_szKey.Pv(),
                sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 ), &cchActual, OSSTR_FIXED_CONVERSION ) );
    m_pindexcreate->szKey = m_szKey;
    m_pindexcreate->cbKey = cchActual * sizeof( char );

    Assert ( pindexcreate3->cbKey >= sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate3->szKey ) + 1 ) );

    Assert( cbAllocateMax >= m_pindexcreate->cbKey );

    ULONG cbPastStringKey = pindexcreate3->cbKey - sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate3->szKey ) + 1 );
    if ( cbPastStringKey > 0 )
    {
        if ( cbPastStringKey == ( sizeof(LANGID) + ( sizeof(WCHAR) * 2 ) ) )
        {

            LANGID * pLangId = (LANGID *)( pindexcreate3->szKey + LOSStrLengthMW( pindexcreate3->szKey ) + 1 );

            memcpy( m_pindexcreate->szKey + m_pindexcreate->cbKey, pLangId, sizeof(LANGID) );
            m_pindexcreate->cbKey += sizeof(LANGID);

            memset( m_pindexcreate->szKey + m_pindexcreate->cbKey, '\0', 2 * sizeof(char) );
            m_pindexcreate->cbKey += 2 * sizeof(char);

        }
        else if ( cbPastStringKey == ( sizeof(LANGID) + ( sizeof(WCHAR) * 2 ) + sizeof( BYTE ) + ( sizeof(WCHAR) * 2 ) ) )
        {
            LANGID * pLangId = (LANGID *)( pindexcreate3->szKey + LOSStrLengthMW( pindexcreate3->szKey ) + 1 );

            memcpy( m_pindexcreate->szKey + m_pindexcreate->cbKey, pLangId, sizeof(LANGID) );
            m_pindexcreate->cbKey += sizeof(LANGID);

            memset( m_pindexcreate->szKey + m_pindexcreate->cbKey, '\0', 2 * sizeof(char) );
            m_pindexcreate->cbKey += 2 * sizeof(char);

            BYTE * pcbVarMac = ( (BYTE*) pLangId ) + ( sizeof(WCHAR) * 2 );

            memcpy( m_pindexcreate->szKey + m_pindexcreate->cbKey, pcbVarMac, sizeof(BYTE) );
            m_pindexcreate->cbKey += sizeof(BYTE);

            memset( m_pindexcreate->szKey + m_pindexcreate->cbKey, '\0', 2 * sizeof(char) );
            m_pindexcreate->cbKey += 2 * sizeof(char);

        }
        else
        {
            CallR( ErrERRCheck( JET_errIndexInvalidDef ) );
        }

    }
    Assert( cbAllocateMax >= m_pindexcreate->cbKey );

    m_pindexcreate->grbit = pindexcreate3->grbit;
    m_pindexcreate->ulDensity = pindexcreate3->ulDensity;

    if ( pindexcreate3->grbit & JET_bitIndexUnicode )
    {
        m_idxunicode.dwMapFlags = pindexcreate3->pidxunicode->dwMapFlags;
        CallR( ErrNORMLocaleToLcid( pindexcreate3->pidxunicode->szLocaleName,
                                    &m_idxunicode.lcid ) );
        m_pindexcreate->pidxunicode = &m_idxunicode;
    }

    m_pindexcreate->cbVarSegMac = pindexcreate3->cbVarSegMac;
    m_pindexcreate->ptuplelimits = pindexcreate3->ptuplelimits;

    m_pindexcreate->cConditionalColumn = pindexcreate3->cConditionalColumn;
    if ( m_pindexcreate->cConditionalColumn )
    {
        Alloc( m_pcondcol = new CAutoCONDCOL[m_pindexcreate->cConditionalColumn] );
        Alloc( m_rgconditionalcolumn = new JET_CONDITIONALCOLUMN_A[m_pindexcreate->cConditionalColumn] );
        memset( m_rgconditionalcolumn, 0, m_pindexcreate->cConditionalColumn * sizeof(JET_CONDITIONALCOLUMN_A) );

        for( ULONG i = 0; i < m_pindexcreate->cConditionalColumn; i++ )
        {
            Call( m_pcondcol[i].ErrSet( pindexcreate3->rgconditionalcolumn + i ) );
            memcpy( m_rgconditionalcolumn + i, (JET_CONDITIONALCOLUMN_A*)(m_pcondcol[i]), sizeof( JET_CONDITIONALCOLUMN_A ) );
        }
    }
    m_pindexcreate->rgconditionalcolumn = m_rgconditionalcolumn;

    m_pindexcreate->err = pindexcreate3->err;
    m_pindexcreate->cbKeyMost = pindexcreate3->cbKeyMost;

    m_pindexcreate->pSpacehints = pindexcreate3->pSpacehints;

    m_pindexcreate3W = pindexcreate3;

HandleError:
    return err;
}


void CAutoIDXCREATE2::Result( )
{
    if ( m_pindexcreate && m_pindexcreate1W )
    {
        m_pindexcreate1W->err = m_pindexcreate->err;
    }
    else if ( m_pindexcreate && m_pindexcreate2W )
    {
        m_pindexcreate2W->err = m_pindexcreate->err;
    }
    else if ( m_pindexcreate && m_pindexcreate3W )
    {
        m_pindexcreate3W->err = m_pindexcreate->err;
    }
}

CAutoIDXCREATE2::~CAutoIDXCREATE2()
{
    delete[] m_rgconditionalcolumn;
    delete[] m_pcondcol;
    delete m_pindexcreate;
}

class CAutoIDXCREATE3
{
    public:
        CAutoIDXCREATE3() : m_pindexcreate( NULL ), m_pcondcol( NULL ), m_rgconditionalcolumn( NULL ), m_pindexcreate2W( NULL ), m_pindexcreate3W( NULL ) { }
        ~CAutoIDXCREATE3();

    public:
        ERR ErrSet( JET_INDEXCREATE2_W * pindexcreate2 );
        ERR ErrSet( JET_INDEXCREATE3_W * pindexcreate3 );
        void Result( );
        operator JET_INDEXCREATE3_A*()      { return m_pindexcreate; }

    private:

        class CAutoCONDCOL
        {
            public:
                CAutoCONDCOL() { m_condcol.cbStruct = sizeof(JET_CONDITIONALCOLUMN_A); }
                ~CAutoCONDCOL() { }

            public:
                ERR ErrSet( const JET_CONDITIONALCOLUMN_W * const pcondcol );
                operator JET_CONDITIONALCOLUMN_A*()     { return &m_condcol; }

            private:
                JET_CONDITIONALCOLUMN_A     m_condcol;
                CAutoSZDDL                  m_szColumnName;
        };

    private:
        JET_INDEXCREATE2_W       * m_pindexcreate2W;
        JET_INDEXCREATE3_W       * m_pindexcreate3W;
        JET_INDEXCREATE3_A       * m_pindexcreate;
        CAutoSZDDL                m_szIndexName;
        CAutoSZDDL                m_szKey;
        CAutoCONDCOL            * m_pcondcol;
        JET_CONDITIONALCOLUMN_A * m_rgconditionalcolumn;
        JET_UNICODEINDEX2         m_idxunicode;
        WCHAR                     m_wszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH];
};

ERR CAutoIDXCREATE3::CAutoCONDCOL::ErrSet( const JET_CONDITIONALCOLUMN_W * const pcondcol )
{
    ERR err = JET_errSuccess;

    C_ASSERT( sizeof(JET_CONDITIONALCOLUMN_W) == sizeof(JET_CONDITIONALCOLUMN_A) );

    if ( !pcondcol || sizeof( JET_CONDITIONALCOLUMN_W ) !=  pcondcol->cbStruct )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Assert( sizeof(JET_CONDITIONALCOLUMN_A) == m_condcol.cbStruct );

    Call( m_szColumnName.ErrSet( pcondcol->szColumnName ) );
    m_condcol.szColumnName = m_szColumnName;

    m_condcol.grbit = pcondcol->grbit;

HandleError:
    return err;
}


ERR CAutoIDXCREATE3::ErrSet( JET_INDEXCREATE2_W * pindexcreate2 )
{
    ERR                 err         = JET_errSuccess;

    if ( sizeof(JET_INDEXCREATE2_W) != pindexcreate2->cbStruct )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( pindexcreate2->cbKey == 0 )
    {
        CallR( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( pindexcreate2->cbKey < sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate2->szKey ) + 1 ) )
    {
        CallR( ErrERRCheck( JET_errIndexInvalidDef ) );
    }

    if ( ( pindexcreate2->szKey[0] != L'+' && pindexcreate2->szKey[0] != L'-' ) ||
        pindexcreate2->szKey[ ( pindexcreate2->cbKey / sizeof( WCHAR ) ) - 1] != L'\0' ||
        pindexcreate2->szKey[ ( pindexcreate2->cbKey / sizeof( WCHAR ) ) - 2] != L'\0' )
    {
        CallR( ErrERRCheck( JET_errIndexInvalidDef ) );
    }

    delete[] m_pcondcol;
    delete[] m_rgconditionalcolumn;
    delete m_pindexcreate;

    m_pcondcol = NULL;
    m_rgconditionalcolumn = NULL;
    m_pindexcreate = NULL;
    m_pindexcreate2W = NULL;

    Alloc( m_pindexcreate = (JET_INDEXCREATE3_A*)new JET_INDEXCREATE3_A );
    memset( m_pindexcreate, 0, sizeof(JET_INDEXCREATE3_A) );
    m_pindexcreate->cbStruct = sizeof(JET_INDEXCREATE3_A);

    Call( m_szIndexName.ErrSet( pindexcreate2->szIndexName ) );
    m_pindexcreate->szIndexName = m_szIndexName;

    size_t cchActual;
    const ULONG cbAllocateMax = sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 + ( sizeof(LANGID) + 1 + 1 + sizeof( BYTE ) + 1 + 1 ) );
    Call( m_szKey.ErrAlloc( cbAllocateMax ) );
    Call( ErrOSSTRUnicodeToAsciiM( pindexcreate2->szKey, m_szKey.Pv(),
                sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 ), &cchActual, OSSTR_FIXED_CONVERSION ) );
    m_pindexcreate->szKey = m_szKey;
    m_pindexcreate->cbKey = cchActual * sizeof( char );

    Assert ( pindexcreate2->cbKey >= sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate2->szKey ) + 1 ) );

    Assert( cbAllocateMax >= m_pindexcreate->cbKey );

    ULONG cbPastStringKey = pindexcreate2->cbKey - sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate2->szKey ) + 1 );
    if ( cbPastStringKey > 0 )
    {
        if ( cbPastStringKey == ( sizeof(LANGID) + ( sizeof(WCHAR) * 2 ) ) )
        {

            LANGID * pLangId = (LANGID *)( pindexcreate2->szKey + LOSStrLengthMW( pindexcreate2->szKey ) + 1 );

            memcpy( m_pindexcreate->szKey + m_pindexcreate->cbKey, pLangId, sizeof(LANGID) );
            m_pindexcreate->cbKey += sizeof(LANGID);

            memset( m_pindexcreate->szKey + m_pindexcreate->cbKey, '\0', 2 * sizeof(char) );
            m_pindexcreate->cbKey += 2 * sizeof(char);

        }
        else if ( cbPastStringKey == ( sizeof(LANGID) + ( sizeof(WCHAR) * 2 ) + sizeof( BYTE ) + ( sizeof(WCHAR) * 2 ) ) )
        {
            LANGID * pLangId = (LANGID *)( pindexcreate2->szKey + LOSStrLengthMW( pindexcreate2->szKey ) + 1 );

            memcpy( m_pindexcreate->szKey + m_pindexcreate->cbKey, pLangId, sizeof(LANGID) );
            m_pindexcreate->cbKey += sizeof(LANGID);

            memset( m_pindexcreate->szKey + m_pindexcreate->cbKey, '\0', 2 * sizeof(char) );
            m_pindexcreate->cbKey += 2 * sizeof(char);

            BYTE * pcbVarMac = ( (BYTE*) pLangId ) + ( sizeof(WCHAR) * 2 );

            memcpy( m_pindexcreate->szKey + m_pindexcreate->cbKey, pcbVarMac, sizeof(BYTE) );
            m_pindexcreate->cbKey += sizeof(BYTE);

            memset( m_pindexcreate->szKey + m_pindexcreate->cbKey, '\0', 2 * sizeof(char) );
            m_pindexcreate->cbKey += 2 * sizeof(char);

        }
        else
        {
            CallR( ErrERRCheck( JET_errIndexInvalidDef ) );
        }

    }
    Assert( cbAllocateMax >= m_pindexcreate->cbKey );

    m_pindexcreate->grbit = pindexcreate2->grbit;
    m_pindexcreate->ulDensity = pindexcreate2->ulDensity;

    if ( pindexcreate2->grbit & JET_bitIndexUnicode )
    {
        m_idxunicode.dwMapFlags = pindexcreate2->pidxunicode->dwMapFlags;
        m_idxunicode.szLocaleName = m_wszLocaleName;
        CallR( ErrNORMLcidToLocale( pindexcreate2->pidxunicode->lcid,
                                    m_idxunicode.szLocaleName,
                                    _countof( m_wszLocaleName ) ) );
        m_pindexcreate->pidxunicode = &m_idxunicode;
    }

    m_pindexcreate->cbVarSegMac = pindexcreate2->cbVarSegMac;
    m_pindexcreate->ptuplelimits = pindexcreate2->ptuplelimits;

    m_pindexcreate->cConditionalColumn = pindexcreate2->cConditionalColumn;
    if ( m_pindexcreate->cConditionalColumn )
    {
        Alloc( m_pcondcol = new CAutoCONDCOL[m_pindexcreate->cConditionalColumn] );
        Alloc( m_rgconditionalcolumn = new JET_CONDITIONALCOLUMN_A[m_pindexcreate->cConditionalColumn] );
        memset( m_rgconditionalcolumn, 0, m_pindexcreate->cConditionalColumn * sizeof(JET_CONDITIONALCOLUMN_A) );

        for( ULONG i = 0; i < m_pindexcreate->cConditionalColumn; i++ )
        {
            Call( m_pcondcol[i].ErrSet( pindexcreate2->rgconditionalcolumn + i ) );
            memcpy( m_rgconditionalcolumn + i, (JET_CONDITIONALCOLUMN_A*)(m_pcondcol[i]), sizeof( JET_CONDITIONALCOLUMN_A ) );
        }
    }
    m_pindexcreate->rgconditionalcolumn = m_rgconditionalcolumn;

    m_pindexcreate->err = pindexcreate2->err;
    m_pindexcreate->cbKeyMost = pindexcreate2->cbKeyMost;

    m_pindexcreate->pSpacehints = pindexcreate2->pSpacehints;

    m_pindexcreate2W = pindexcreate2;

HandleError:
    return err;
}

ERR CAutoIDXCREATE3::ErrSet( JET_INDEXCREATE3_W * pindexcreate3 )
{
    ERR                 err         = JET_errSuccess;

    if ( sizeof(JET_INDEXCREATE3_W) != pindexcreate3->cbStruct )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( pindexcreate3->cbKey == 0 )
    {
        CallR( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( pindexcreate3->cbKey < sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate3->szKey ) + 1 ) )
    {
        CallR( ErrERRCheck( JET_errIndexInvalidDef ) );
    }

    if ( ( pindexcreate3->szKey[0] != L'+' && pindexcreate3->szKey[0] != L'-' ) ||
        pindexcreate3->szKey[ ( pindexcreate3->cbKey / sizeof( WCHAR ) ) - 1] != L'\0' ||
        pindexcreate3->szKey[ ( pindexcreate3->cbKey / sizeof( WCHAR ) ) - 2] != L'\0' )
    {
        CallR( ErrERRCheck( JET_errIndexInvalidDef ) );
    }

    delete[] m_pcondcol;
    delete[] m_rgconditionalcolumn;
    delete m_pindexcreate;

    m_pcondcol = NULL;
    m_rgconditionalcolumn = NULL;
    m_pindexcreate = NULL;
    m_pindexcreate2W = NULL;

    Alloc( m_pindexcreate = (JET_INDEXCREATE3_A*)new JET_INDEXCREATE3_A );
    memset( m_pindexcreate, 0, sizeof(JET_INDEXCREATE3_A) );
    m_pindexcreate->cbStruct = sizeof(JET_INDEXCREATE3_A);

    Call( m_szIndexName.ErrSet( pindexcreate3->szIndexName ) );
    m_pindexcreate->szIndexName = m_szIndexName;

    size_t cchActual;
    const ULONG cbAllocateMax = sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 + ( sizeof(LANGID) + 1 + 1 + sizeof( BYTE ) + 1 + 1 ) );
    Call( m_szKey.ErrAlloc( cbAllocateMax ) );
    Call( ErrOSSTRUnicodeToAsciiM( pindexcreate3->szKey, m_szKey.Pv(),
                sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 ), &cchActual, OSSTR_FIXED_CONVERSION ) );
    m_pindexcreate->szKey = m_szKey;
    m_pindexcreate->cbKey = cchActual * sizeof( char );

    Assert ( pindexcreate3->cbKey >= sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate3->szKey ) + 1 ) );

    Assert( cbAllocateMax >= m_pindexcreate->cbKey );

    ULONG cbPastStringKey = pindexcreate3->cbKey - sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate3->szKey ) + 1 );
    if ( cbPastStringKey > 0 )
    {
        if ( cbPastStringKey == ( sizeof(LANGID) + ( sizeof(WCHAR) * 2 ) ) )
        {

            LANGID * pLangId = (LANGID *)( pindexcreate3->szKey + LOSStrLengthMW( pindexcreate3->szKey ) + 1 );

            memcpy( m_pindexcreate->szKey + m_pindexcreate->cbKey, pLangId, sizeof(LANGID) );
            m_pindexcreate->cbKey += sizeof(LANGID);

            memset( m_pindexcreate->szKey + m_pindexcreate->cbKey, '\0', 2 * sizeof(char) );
            m_pindexcreate->cbKey += 2 * sizeof(char);

        }
        else if ( cbPastStringKey == ( sizeof(LANGID) + ( sizeof(WCHAR) * 2 ) + sizeof( BYTE ) + ( sizeof(WCHAR) * 2 ) ) )
        {
            LANGID * pLangId = (LANGID *)( pindexcreate3->szKey + LOSStrLengthMW( pindexcreate3->szKey ) + 1 );

            memcpy( m_pindexcreate->szKey + m_pindexcreate->cbKey, pLangId, sizeof(LANGID) );
            m_pindexcreate->cbKey += sizeof(LANGID);

            memset( m_pindexcreate->szKey + m_pindexcreate->cbKey, '\0', 2 * sizeof(char) );
            m_pindexcreate->cbKey += 2 * sizeof(char);

            BYTE * pcbVarMac = ( (BYTE*) pLangId ) + ( sizeof(WCHAR) * 2 );

            memcpy( m_pindexcreate->szKey + m_pindexcreate->cbKey, pcbVarMac, sizeof(BYTE) );
            m_pindexcreate->cbKey += sizeof(BYTE);

            memset( m_pindexcreate->szKey + m_pindexcreate->cbKey, '\0', 2 * sizeof(char) );
            m_pindexcreate->cbKey += 2 * sizeof(char);

        }
        else
        {
            CallR( ErrERRCheck( JET_errIndexInvalidDef ) );
        }

    }
    Assert( cbAllocateMax >= m_pindexcreate->cbKey );

    m_pindexcreate->grbit = pindexcreate3->grbit;
    m_pindexcreate->ulDensity = pindexcreate3->ulDensity;

    if ( pindexcreate3->grbit & JET_bitIndexUnicode )
    {
        m_idxunicode.dwMapFlags = pindexcreate3->pidxunicode->dwMapFlags;
        m_idxunicode.szLocaleName = m_wszLocaleName;
        if ( NULL != pindexcreate3->pidxunicode->szLocaleName )
        {
            Call( ErrOSStrCbCopyW( m_idxunicode.szLocaleName, sizeof( m_wszLocaleName ), pindexcreate3->pidxunicode->szLocaleName ) );
        }
        else
        {
            m_idxunicode.szLocaleName = NULL;
        }
        m_pindexcreate->pidxunicode = &m_idxunicode;
    }

    m_pindexcreate->cbVarSegMac = pindexcreate3->cbVarSegMac;
    m_pindexcreate->ptuplelimits = pindexcreate3->ptuplelimits;

    m_pindexcreate->cConditionalColumn = pindexcreate3->cConditionalColumn;
    if ( m_pindexcreate->cConditionalColumn )
    {
        Alloc( m_pcondcol = new CAutoCONDCOL[m_pindexcreate->cConditionalColumn] );
        Alloc( m_rgconditionalcolumn = new JET_CONDITIONALCOLUMN_A[m_pindexcreate->cConditionalColumn] );
        memset( m_rgconditionalcolumn, 0, m_pindexcreate->cConditionalColumn * sizeof(JET_CONDITIONALCOLUMN_A) );

        for( ULONG i = 0; i < m_pindexcreate->cConditionalColumn; i++ )
        {
            Call( m_pcondcol[i].ErrSet( pindexcreate3->rgconditionalcolumn + i ) );
            memcpy( m_rgconditionalcolumn + i, (JET_CONDITIONALCOLUMN_A*)(m_pcondcol[i]), sizeof( JET_CONDITIONALCOLUMN_A ) );
        }
    }
    m_pindexcreate->rgconditionalcolumn = m_rgconditionalcolumn;

    m_pindexcreate->err = pindexcreate3->err;
    m_pindexcreate->cbKeyMost = pindexcreate3->cbKeyMost;

    m_pindexcreate->pSpacehints = pindexcreate3->pSpacehints;

    m_pindexcreate3W = pindexcreate3;

HandleError:
    return err;
}



void CAutoIDXCREATE3::Result( )
{
    if ( m_pindexcreate && m_pindexcreate2W )
    {
        m_pindexcreate2W->err = m_pindexcreate->err;
    }
}

CAutoIDXCREATE3::~CAutoIDXCREATE3()
{
    delete[] m_rgconditionalcolumn;
    delete[] m_pcondcol;
    delete m_pindexcreate;
}


C_ASSERT( sizeof(JET_TABLECREATE2_W) != sizeof(JET_TABLECREATE3_W) );
C_ASSERT( sizeof(JET_TABLECREATE2_A) != sizeof(JET_TABLECREATE3_A) );
C_ASSERT( sizeof(JET_TABLECREATE2_A) != sizeof(JET_TABLECREATE3_W) );
C_ASSERT( sizeof(JET_TABLECREATE2_W) != sizeof(JET_TABLECREATE3_A) );

LOCAL JET_ERR JetCreateTableColumnIndexEx(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __inout JET_TABLECREATE5_A *    ptablecreate )
{
    JET_ERR err;
    APICALL_SESID       apicall( opCreateTableColumnIndex );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p[%s])",
            __FUNCTION__,
            sesid,
            dbid,
            ptablecreate,
            ( NULL != ptablecreate ? OSFormat( "szTable=%s", OSFormatString( ptablecreate->szTableName ) ) : OSTRACENULLPARAM ) ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        const BOOL  fInvalidParam   = ( NULL == ptablecreate
                                        || sizeof(*ptablecreate) != ptablecreate->cbStruct );

        if ( fInvalidParam )
        {
            apicall.LeaveAfterCall( ErrERRCheck( JET_errInvalidParameter ) );
        }
        else if ( ( ptablecreate->grbit & JET_bitTableCreateImmutableStructure ) == 0 )
        {
            err = ErrIsamCreateTable( sesid, dbid, ptablecreate );

            apicall.LeaveAfterCall( err );
        }
        else
        {
            CAutoTABLECREATE5To5_T< JET_TABLECREATE5_A, JET_TABLECREATE5_A, CAutoINDEXCREATE3To3_T< JET_INDEXCREATE3_A, JET_INDEXCREATE3_A > > tablecreate;

            Call( tablecreate.ErrSet( ptablecreate ) );
            err = ErrIsamCreateTable( sesid, dbid, (JET_TABLECREATE5_A*)tablecreate );

            tablecreate.Result();

HandleError:
            apicall.LeaveAfterCall( err );
        }
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetCreateTableColumnIndexEx4(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __inout JET_TABLECREATE4_A *    ptablecreate )
{
    ERR err = JET_errSuccess;

    CAutoTABLECREATE4To5_T< JET_TABLECREATE4_A, JET_TABLECREATE5_A > tablecreate;

    CallR( tablecreate.ErrSet( ptablecreate ) );

    err = JetCreateTableColumnIndexEx( sesid, dbid, (JET_TABLECREATE5_A*)tablecreate );

    tablecreate.Result( );

    return err;
}

LOCAL JET_ERR JetCreateTableColumnIndexEx3(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __inout JET_TABLECREATE3_A *    ptablecreate )
{
    ERR err = JET_errSuccess;

    CAutoTABLECREATE3To4_T< JET_TABLECREATE3_A, JET_TABLECREATE4_A, CAutoINDEXCREATE2To3_T< JET_INDEXCREATE2_A, JET_INDEXCREATE3_A > > tablecreate;

    CallR( tablecreate.ErrSet( ptablecreate ) );

    err = JetCreateTableColumnIndexEx4( sesid, dbid, (JET_TABLECREATE4_A*)tablecreate );

    tablecreate.Result( );

    return err;
}

LOCAL JET_ERR JetCreateTableColumnIndexEx2(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __inout JET_TABLECREATE2_A *    ptablecreate )
{
    ERR err = JET_errSuccess;

    CAutoTABLECREATE2To3_T< JET_TABLECREATE2_A, JET_TABLECREATE3_A, CAutoINDEXCREATE1To2_T< JET_INDEXCREATE_A, JET_INDEXCREATE2_A > > tablecreate;

    CallR( tablecreate.ErrSet( ptablecreate ) );

    err = JetCreateTableColumnIndexEx3( sesid, dbid, (JET_TABLECREATE3_A*)tablecreate );

    tablecreate.Result( );

    return err;
}

LOCAL JET_ERR JetCreateTableColumnIndexExOLD(
    __in JET_SESID              sesid,
    __in JET_DBID               dbid,
    __inout JET_TABLECREATE_A * ptablecreate )
{
    Assert( ptablecreate );

    JET_TABLECREATE2_A tablecreate;

    tablecreate.cbStruct                = sizeof( tablecreate );
    tablecreate.szTableName             = ptablecreate->szTableName;
    tablecreate.szTemplateTableName     = ptablecreate->szTemplateTableName;
    tablecreate.ulPages                 = ptablecreate->ulPages;
    tablecreate.ulDensity               = ptablecreate->ulDensity;
    tablecreate.rgcolumncreate          = ptablecreate->rgcolumncreate;
    tablecreate.cColumns                = ptablecreate->cColumns;
    tablecreate.rgindexcreate           = ptablecreate->rgindexcreate;
    tablecreate.cIndexes                = ptablecreate->cIndexes;
    tablecreate.szCallback              = NULL;
    tablecreate.cbtyp                   = 0;
    tablecreate.grbit                   = ptablecreate->grbit;
    tablecreate.tableid                 = ptablecreate->tableid;
    tablecreate.cCreated                = ptablecreate->cCreated;
    tablecreate.tableid                 = JET_tableidNil;

    const ERR err = JetCreateTableColumnIndexEx2( sesid, dbid, &tablecreate );

    if ( ( ptablecreate->grbit & JET_bitTableCreateImmutableStructure ) != 0 )
    {
        Assert( JET_tableidNil == tablecreate.tableid );
    }
    else
    {
        ptablecreate->tableid   = tablecreate.tableid;
        ptablecreate->cCreated  = tablecreate.cCreated;
    }

    return err;
}

LOCAL JET_ERR JetCreateTableColumnIndexEx2W(
    __in JET_SESID              sesid,
    __in JET_DBID               dbid,
    __inout JET_TABLECREATE2_W *    ptablecreate );


LOCAL JET_ERR JetCreateTableColumnIndexExW(
    __in JET_SESID              sesid,
    __in JET_DBID               dbid,
    __inout JET_TABLECREATE_W * ptablecreate )
{
    Assert( ptablecreate );

    JET_TABLECREATE2_W tablecreate;

    tablecreate.cbStruct                = sizeof( tablecreate );
    tablecreate.szTableName             = ptablecreate->szTableName;
    tablecreate.szTemplateTableName     = ptablecreate->szTemplateTableName;
    tablecreate.ulPages                 = ptablecreate->ulPages;
    tablecreate.ulDensity               = ptablecreate->ulDensity;
    tablecreate.rgcolumncreate          = ptablecreate->rgcolumncreate;
    tablecreate.cColumns                = ptablecreate->cColumns;
    tablecreate.rgindexcreate           = ptablecreate->rgindexcreate;
    tablecreate.cIndexes                = ptablecreate->cIndexes;
    tablecreate.szCallback              = NULL;
    tablecreate.cbtyp                   = 0;
    tablecreate.grbit                   = ptablecreate->grbit;
    tablecreate.tableid                 = ptablecreate->tableid;
    tablecreate.cCreated                = ptablecreate->cCreated;
    tablecreate.tableid                 = JET_tableidNil;

    const ERR err = JetCreateTableColumnIndexEx2W( sesid, dbid, &tablecreate );

    if ( ( ptablecreate->grbit & JET_bitTableCreateImmutableStructure ) != 0 )
    {
        Assert( JET_tableidNil == tablecreate.tableid );
    }
    else
    {
        ptablecreate->tableid   = tablecreate.tableid;
        ptablecreate->cCreated  = tablecreate.cCreated;
    }

    return err;
}

JET_ERR JET_API JetCreateTableColumnIndexA(
    __in JET_SESID              sesid,
    __in JET_DBID               dbid,
    __inout JET_TABLECREATE_A * ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexExOLD( sesid, dbid, ptablecreate ) );
}

JET_ERR JET_API JetCreateTableColumnIndexW(
    __in JET_SESID              sesid,
    __in JET_DBID               dbid,
    __inout JET_TABLECREATE_W * ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexExW( sesid, dbid, ptablecreate ) );
}

class CAutoTABLECREATE3
{
    public:
        CAutoTABLECREATE3():m_ptablecreate( NULL ),m_rgcolumncreate( NULL ), m_rgszColumnName( NULL ), m_rgindexcreate( NULL ), m_rgindexcreateauto( NULL ), m_ptablecreateW( NULL ) { }
        ~CAutoTABLECREATE3();

    public:
        ERR ErrSet( JET_TABLECREATE3_W * ptablecreate );
        void Result( );
        operator JET_TABLECREATE3_A*()      { return m_ptablecreate; }

    private:
        JET_TABLECREATE3_A *    m_ptablecreate;
        JET_TABLECREATE3_W *    m_ptablecreateW;
        CAutoSZDDL              m_szTableName;
        CAutoSZDDL              m_szTemplateTableName;
        CAutoSZDDL              m_szCallback;

        JET_COLUMNCREATE_A *    m_rgcolumncreate;
        CAutoSZDDL *            m_rgszColumnName;

        JET_INDEXCREATE2_A *    m_rgindexcreate;
        CAutoIDXCREATE2 *       m_rgindexcreateauto;
};


ERR CAutoTABLECREATE3::ErrSet( JET_TABLECREATE3_W * ptablecreate )
{
    ERR                 err         = JET_errSuccess;

    C_ASSERT( sizeof(JET_TABLECREATE3_W) == sizeof(JET_TABLECREATE3_A) );

    delete[] m_rgcolumncreate;
    delete[] m_rgszColumnName;

    delete[] m_rgindexcreate;
    delete[] m_rgindexcreateauto;
    delete m_ptablecreate;

    m_rgcolumncreate = NULL;
    m_rgszColumnName = NULL;
    m_rgindexcreate = NULL;
    m_rgindexcreate = NULL;
    m_rgindexcreateauto = NULL;
    m_ptablecreate = NULL;
    m_ptablecreateW = NULL;

    if ( ptablecreate )
    {
        if ( sizeof(JET_TABLECREATE3_W) != ptablecreate->cbStruct )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    if ( !ptablecreate )
    {
        return JET_errSuccess;
    }

    Alloc( m_ptablecreate = (JET_TABLECREATE3_A*)new JET_TABLECREATE3_A );
    memset( m_ptablecreate, 0, sizeof(JET_TABLECREATE3_A) );

    m_ptablecreate->cbStruct = sizeof(JET_TABLECREATE3_A);

    Call( m_szTableName.ErrSet( ptablecreate->szTableName ) );
    m_ptablecreate->szTableName = m_szTableName;

    Call( m_szTemplateTableName.ErrSet( ptablecreate->szTemplateTableName ) );
    m_ptablecreate->szTemplateTableName = m_szTemplateTableName;

    m_ptablecreate->ulPages = ptablecreate->ulPages;
    m_ptablecreate->ulDensity = ptablecreate->ulDensity;
    m_ptablecreate->cbSeparateLV = ptablecreate->cbSeparateLV;
    m_ptablecreate->pSeqSpacehints = ptablecreate->pSeqSpacehints;
    m_ptablecreate->pLVSpacehints = ptablecreate->pLVSpacehints;

    m_ptablecreate->cColumns = ptablecreate->cColumns;
    if ( m_ptablecreate->cColumns )
    {
        Alloc( m_rgszColumnName = new CAutoSZDDL[m_ptablecreate->cColumns] );
        Alloc( m_rgcolumncreate = new JET_COLUMNCREATE_A[m_ptablecreate->cColumns] );
        memset( m_rgcolumncreate, 0, m_ptablecreate->cColumns * sizeof(JET_COLUMNCREATE_A) );

        for( ULONG i = 0; i < m_ptablecreate->cColumns; i++ )
        {
            if ( sizeof( JET_COLUMNCREATE_W ) != ptablecreate->rgcolumncreate[i].cbStruct )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            m_rgcolumncreate[i].cbStruct = sizeof(JET_COLUMNCREATE_A);
            Call( m_rgszColumnName[i].ErrSet( ptablecreate->rgcolumncreate[i].szColumnName ) );
            m_rgcolumncreate[i].szColumnName = m_rgszColumnName[i];

            m_rgcolumncreate[i].coltyp = ptablecreate->rgcolumncreate[i].coltyp;
            m_rgcolumncreate[i].cbMax = ptablecreate->rgcolumncreate[i].cbMax;
            m_rgcolumncreate[i].grbit = ptablecreate->rgcolumncreate[i].grbit;
            m_rgcolumncreate[i].pvDefault = ptablecreate->rgcolumncreate[i].pvDefault;
            m_rgcolumncreate[i].cbDefault = ptablecreate->rgcolumncreate[i].cbDefault;
            m_rgcolumncreate[i].cp = ptablecreate->rgcolumncreate[i].cp;
            m_rgcolumncreate[i].columnid = ptablecreate->rgcolumncreate[i].columnid;
            m_rgcolumncreate[i].err = ptablecreate->rgcolumncreate[i].err;
        }
    }
    m_ptablecreate->rgcolumncreate = m_rgcolumncreate;

    m_ptablecreate->cIndexes = ptablecreate->cIndexes;
    if ( m_ptablecreate->cIndexes )
    {
        Alloc( m_rgindexcreateauto = new CAutoIDXCREATE2[m_ptablecreate->cIndexes] );
        Alloc( m_rgindexcreate = new JET_INDEXCREATE2_A[m_ptablecreate->cIndexes] );
        memset( m_rgindexcreate, 0, m_ptablecreate->cIndexes * sizeof(JET_INDEXCREATE2_A) );

        for( ULONG i = 0; i < m_ptablecreate->cIndexes; i++ )
        {
            Call( m_rgindexcreateauto[i].ErrSet( ptablecreate->rgindexcreate + i ) );
            memcpy( m_rgindexcreate + i, (JET_INDEXCREATE2_A*)(m_rgindexcreateauto[i]), sizeof(JET_INDEXCREATE2_A) );
        }
    }
    m_ptablecreate->rgindexcreate = m_rgindexcreate;

    Call( m_szCallback.ErrSet( ptablecreate->szCallback ) );
    m_ptablecreate->szCallback = m_szCallback;


    m_ptablecreate->cbtyp = ptablecreate->cbtyp;
    m_ptablecreate->grbit = ptablecreate->grbit;
    m_ptablecreate->tableid = ptablecreate->tableid;
    m_ptablecreate->cCreated = ptablecreate->cCreated;

    m_ptablecreateW = ptablecreate;

HandleError:
    return err;
}

void CAutoTABLECREATE3::Result( )
{
    if ( m_ptablecreate && m_ptablecreateW )
    {
        Assert( m_ptablecreateW->cColumns == m_ptablecreate->cColumns );
        for( ULONG iColumn = 0; iColumn < m_ptablecreate->cColumns ; iColumn++ )
        {
            m_ptablecreateW->rgcolumncreate[ iColumn ].columnid = m_rgcolumncreate[ iColumn ].columnid;
            m_ptablecreateW->rgcolumncreate[ iColumn ].err = m_rgcolumncreate[ iColumn ].err;
        }

        Assert( m_ptablecreateW->cIndexes == m_ptablecreate->cIndexes );
        for( ULONG iIndex = 0; iIndex < m_ptablecreate->cIndexes; iIndex++ )
        {
            m_ptablecreateW->rgindexcreate[ iIndex ].err = m_rgindexcreate[ iIndex ].err;
        }

        m_ptablecreateW->ulPages = m_ptablecreate->ulPages;
        m_ptablecreateW->tableid = m_ptablecreate->tableid;
        m_ptablecreateW->cCreated = m_ptablecreate->cCreated;
    }
}

CAutoTABLECREATE3::~CAutoTABLECREATE3()
{
    delete[] m_rgcolumncreate;
    delete[] m_rgszColumnName;

    delete[] m_rgindexcreate;
    delete[] m_rgindexcreateauto;
    delete m_ptablecreate;
}

class CAutoTABLECREATE4
{
    public:
        CAutoTABLECREATE4():m_ptablecreate( NULL ),m_rgcolumncreate( NULL ), m_rgszColumnName( NULL ), m_rgindexcreate( NULL ), m_rgindexcreateauto( NULL ), m_ptablecreateW( NULL ) { }
        ~CAutoTABLECREATE4();

    public:
        ERR ErrSet( JET_TABLECREATE4_W * ptablecreate );
        void Result( );
        operator JET_TABLECREATE4_A*()      { return m_ptablecreate; }

    private:
        JET_TABLECREATE4_A *    m_ptablecreate;
        JET_TABLECREATE4_W *    m_ptablecreateW;
        CAutoSZDDL              m_szTableName;
        CAutoSZDDL              m_szTemplateTableName;
        CAutoSZDDL              m_szCallback;

        JET_COLUMNCREATE_A *    m_rgcolumncreate;
        CAutoSZDDL *            m_rgszColumnName;

        JET_INDEXCREATE3_A *    m_rgindexcreate;
        CAutoIDXCREATE3 *       m_rgindexcreateauto;
};

ERR CAutoTABLECREATE4::ErrSet( JET_TABLECREATE4_W * ptablecreate )
{
    ERR                 err         = JET_errSuccess;

    C_ASSERT( sizeof(JET_TABLECREATE4_W) == sizeof(JET_TABLECREATE4_A) );

    delete[] m_rgcolumncreate;
    delete[] m_rgszColumnName;

    delete[] m_rgindexcreate;
    delete[] m_rgindexcreateauto;
    delete m_ptablecreate;

    m_rgcolumncreate = NULL;
    m_rgszColumnName = NULL;
    m_rgindexcreate = NULL;
    m_rgindexcreate = NULL;
    m_rgindexcreateauto = NULL;
    m_ptablecreate = NULL;
    m_ptablecreateW = NULL;

    if ( ptablecreate )
    {
        if ( sizeof(JET_TABLECREATE3_W) != ptablecreate->cbStruct )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    if ( !ptablecreate )
    {
        return JET_errSuccess;
    }

    Alloc( m_ptablecreate = (JET_TABLECREATE4_A*)new JET_TABLECREATE4_A );
    memset( m_ptablecreate, 0, sizeof(JET_TABLECREATE4_A) );

    m_ptablecreate->cbStruct = sizeof(JET_TABLECREATE4_A);

    Call( m_szTableName.ErrSet( ptablecreate->szTableName ) );
    m_ptablecreate->szTableName = m_szTableName;

    Call( m_szTemplateTableName.ErrSet( ptablecreate->szTemplateTableName ) );
    m_ptablecreate->szTemplateTableName = m_szTemplateTableName;

    m_ptablecreate->ulPages = ptablecreate->ulPages;
    m_ptablecreate->ulDensity = ptablecreate->ulDensity;
    m_ptablecreate->cbSeparateLV = ptablecreate->cbSeparateLV;
    m_ptablecreate->pSeqSpacehints = ptablecreate->pSeqSpacehints;
    m_ptablecreate->pLVSpacehints = ptablecreate->pLVSpacehints;

    m_ptablecreate->cColumns = ptablecreate->cColumns;
    if ( m_ptablecreate->cColumns )
    {
        Alloc( m_rgszColumnName = new CAutoSZDDL[m_ptablecreate->cColumns] );
        Alloc( m_rgcolumncreate = new JET_COLUMNCREATE_A[m_ptablecreate->cColumns] );
        memset( m_rgcolumncreate, 0, m_ptablecreate->cColumns * sizeof(JET_COLUMNCREATE_A) );

        for( ULONG i = 0; i < m_ptablecreate->cColumns; i++ )
        {
            if ( sizeof( JET_COLUMNCREATE_W ) != ptablecreate->rgcolumncreate[i].cbStruct )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            m_rgcolumncreate[i].cbStruct = sizeof(JET_COLUMNCREATE_A);
            Call( m_rgszColumnName[i].ErrSet( ptablecreate->rgcolumncreate[i].szColumnName ) );
            m_rgcolumncreate[i].szColumnName = m_rgszColumnName[i];

            m_rgcolumncreate[i].coltyp = ptablecreate->rgcolumncreate[i].coltyp;
            m_rgcolumncreate[i].cbMax = ptablecreate->rgcolumncreate[i].cbMax;
            m_rgcolumncreate[i].grbit = ptablecreate->rgcolumncreate[i].grbit;
            m_rgcolumncreate[i].pvDefault = ptablecreate->rgcolumncreate[i].pvDefault;
            m_rgcolumncreate[i].cbDefault = ptablecreate->rgcolumncreate[i].cbDefault;
            m_rgcolumncreate[i].cp = ptablecreate->rgcolumncreate[i].cp;
            m_rgcolumncreate[i].columnid = ptablecreate->rgcolumncreate[i].columnid;
            m_rgcolumncreate[i].err = ptablecreate->rgcolumncreate[i].err;
        }
    }
    m_ptablecreate->rgcolumncreate = m_rgcolumncreate;

    m_ptablecreate->cIndexes = ptablecreate->cIndexes;
    if ( m_ptablecreate->cIndexes )
    {
        Alloc( m_rgindexcreateauto = new CAutoIDXCREATE3[m_ptablecreate->cIndexes] );
        Alloc( m_rgindexcreate = new JET_INDEXCREATE3_A[m_ptablecreate->cIndexes] );
        memset( m_rgindexcreate, 0, m_ptablecreate->cIndexes * sizeof(JET_INDEXCREATE3_A) );

        for( ULONG i = 0; i < m_ptablecreate->cIndexes; i++ )
        {
            Call( m_rgindexcreateauto[i].ErrSet( ptablecreate->rgindexcreate + i ) );
            memcpy( m_rgindexcreate + i, (JET_INDEXCREATE3_A*)(m_rgindexcreateauto[i]), sizeof(JET_INDEXCREATE3_A) );
        }
    }
    m_ptablecreate->rgindexcreate = m_rgindexcreate;

    Call( m_szCallback.ErrSet( ptablecreate->szCallback ) );
    m_ptablecreate->szCallback = m_szCallback;


    m_ptablecreate->cbtyp = ptablecreate->cbtyp;
    m_ptablecreate->grbit = ptablecreate->grbit;
    m_ptablecreate->tableid = ptablecreate->tableid;
    m_ptablecreate->cCreated = ptablecreate->cCreated;

    m_ptablecreateW = ptablecreate;

HandleError:
    return err;
}

void CAutoTABLECREATE4::Result( )
{
    if ( m_ptablecreate && m_ptablecreateW )
    {
        Assert( m_ptablecreateW->cColumns == m_ptablecreate->cColumns );
        for( ULONG iColumn = 0; iColumn < m_ptablecreate->cColumns ; iColumn++ )
        {
            m_ptablecreateW->rgcolumncreate[ iColumn ].columnid = m_rgcolumncreate[ iColumn ].columnid;
            m_ptablecreateW->rgcolumncreate[ iColumn ].err = m_rgcolumncreate[ iColumn ].err;
        }

        Assert( m_ptablecreateW->cIndexes == m_ptablecreate->cIndexes );
        for( ULONG iIndex = 0; iIndex < m_ptablecreate->cIndexes; iIndex++ )
        {
            m_ptablecreateW->rgindexcreate[ iIndex ].err = m_rgindexcreate[ iIndex ].err;
        }

        m_ptablecreateW->ulPages = m_ptablecreate->ulPages;
        m_ptablecreateW->tableid = m_ptablecreate->tableid;
        m_ptablecreateW->cCreated = m_ptablecreate->cCreated;
    }
}

CAutoTABLECREATE4::~CAutoTABLECREATE4()
{
    delete[] m_rgcolumncreate;
    delete[] m_rgszColumnName;

    delete[] m_rgindexcreate;
    delete[] m_rgindexcreateauto;
    delete m_ptablecreate;
}

class CAutoTABLECREATE5
{
    public:
        CAutoTABLECREATE5():m_ptablecreate( NULL ),m_rgcolumncreate( NULL ), m_rgszColumnName( NULL ), m_rgindexcreate( NULL ), m_rgindexcreateauto( NULL ), m_ptablecreateW( NULL ) { }
        ~CAutoTABLECREATE5();

    public:
        ERR ErrSet( JET_TABLECREATE5_W * ptablecreate );
        void Result( );
        operator JET_TABLECREATE5_A*()      { return m_ptablecreate; }

    private:
        JET_TABLECREATE5_A *    m_ptablecreate;
        JET_TABLECREATE5_W *    m_ptablecreateW;
        CAutoSZDDL              m_szTableName;
        CAutoSZDDL              m_szTemplateTableName;
        CAutoSZDDL              m_szCallback;

        JET_COLUMNCREATE_A *    m_rgcolumncreate;
        CAutoSZDDL *            m_rgszColumnName;

        JET_INDEXCREATE3_A *    m_rgindexcreate;
        CAutoIDXCREATE3 *       m_rgindexcreateauto;
};

ERR CAutoTABLECREATE5::ErrSet( JET_TABLECREATE5_W * ptablecreate )
{
    ERR                 err         = JET_errSuccess;

    C_ASSERT( sizeof(JET_TABLECREATE5_W) == sizeof(JET_TABLECREATE5_A) );

    delete[] m_rgcolumncreate;
    delete[] m_rgszColumnName;

    delete[] m_rgindexcreate;
    delete[] m_rgindexcreateauto;
    delete m_ptablecreate;

    m_rgcolumncreate = NULL;
    m_rgszColumnName = NULL;
    m_rgindexcreate = NULL;
    m_rgindexcreate = NULL;
    m_rgindexcreateauto = NULL;
    m_ptablecreate = NULL;
    m_ptablecreateW = NULL;

    if ( ptablecreate )
    {
        if ( sizeof(JET_TABLECREATE5_W) != ptablecreate->cbStruct )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    if ( !ptablecreate )
    {
        return JET_errSuccess;
    }

    Alloc( m_ptablecreate = new JET_TABLECREATE5_A );
    memset( m_ptablecreate, 0, sizeof(JET_TABLECREATE5_A) );

    m_ptablecreate->cbStruct = sizeof(JET_TABLECREATE5_A);

    Call( m_szTableName.ErrSet( ptablecreate->szTableName ) );
    m_ptablecreate->szTableName = m_szTableName;

    Call( m_szTemplateTableName.ErrSet( ptablecreate->szTemplateTableName ) );
    m_ptablecreate->szTemplateTableName = m_szTemplateTableName;

    m_ptablecreate->ulPages = ptablecreate->ulPages;
    m_ptablecreate->ulDensity = ptablecreate->ulDensity;
    m_ptablecreate->cbSeparateLV = ptablecreate->cbSeparateLV;
    m_ptablecreate->pSeqSpacehints = ptablecreate->pSeqSpacehints;
    m_ptablecreate->pLVSpacehints = ptablecreate->pLVSpacehints;
    m_ptablecreate->cbLVChunkMax = ptablecreate->cbLVChunkMax;

    m_ptablecreate->cColumns = ptablecreate->cColumns;
    if ( m_ptablecreate->cColumns )
    {
        Alloc( m_rgszColumnName = new CAutoSZDDL[m_ptablecreate->cColumns] );
        Alloc( m_rgcolumncreate = new JET_COLUMNCREATE_A[m_ptablecreate->cColumns] );
        memset( m_rgcolumncreate, 0, m_ptablecreate->cColumns * sizeof(JET_COLUMNCREATE_A) );

        for( ULONG i = 0; i < m_ptablecreate->cColumns; i++ )
        {
            if ( sizeof( JET_COLUMNCREATE_W ) != ptablecreate->rgcolumncreate[i].cbStruct )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            m_rgcolumncreate[i].cbStruct = sizeof(JET_COLUMNCREATE_A);
            Call( m_rgszColumnName[i].ErrSet( ptablecreate->rgcolumncreate[i].szColumnName ) );
            m_rgcolumncreate[i].szColumnName = m_rgszColumnName[i];

            m_rgcolumncreate[i].coltyp = ptablecreate->rgcolumncreate[i].coltyp;
            m_rgcolumncreate[i].cbMax = ptablecreate->rgcolumncreate[i].cbMax;
            m_rgcolumncreate[i].grbit = ptablecreate->rgcolumncreate[i].grbit;
            m_rgcolumncreate[i].pvDefault = ptablecreate->rgcolumncreate[i].pvDefault;
            m_rgcolumncreate[i].cbDefault = ptablecreate->rgcolumncreate[i].cbDefault;
            m_rgcolumncreate[i].cp = ptablecreate->rgcolumncreate[i].cp;
            m_rgcolumncreate[i].columnid = ptablecreate->rgcolumncreate[i].columnid;
            m_rgcolumncreate[i].err = ptablecreate->rgcolumncreate[i].err;
        }
    }
    m_ptablecreate->rgcolumncreate = m_rgcolumncreate;

    m_ptablecreate->cIndexes = ptablecreate->cIndexes;
    if ( m_ptablecreate->cIndexes )
    {
        Alloc( m_rgindexcreateauto = new CAutoIDXCREATE3[m_ptablecreate->cIndexes] );
        Alloc( m_rgindexcreate = new JET_INDEXCREATE3_A[m_ptablecreate->cIndexes] );
        memset( m_rgindexcreate, 0, m_ptablecreate->cIndexes * sizeof(JET_INDEXCREATE3_A) );

        for( ULONG i = 0; i < m_ptablecreate->cIndexes; i++ )
        {
            Call( m_rgindexcreateauto[i].ErrSet( ptablecreate->rgindexcreate + i ) );
            memcpy( m_rgindexcreate + i, (JET_INDEXCREATE3_A*)(m_rgindexcreateauto[i]), sizeof(JET_INDEXCREATE3_A) );
        }
    }
    m_ptablecreate->rgindexcreate = m_rgindexcreate;

    Call( m_szCallback.ErrSet( ptablecreate->szCallback ) );
    m_ptablecreate->szCallback = m_szCallback;


    m_ptablecreate->cbtyp = ptablecreate->cbtyp;
    m_ptablecreate->grbit = ptablecreate->grbit;
    m_ptablecreate->tableid = ptablecreate->tableid;
    m_ptablecreate->cCreated = ptablecreate->cCreated;

    m_ptablecreateW = ptablecreate;

HandleError:
    return err;
}

void CAutoTABLECREATE5::Result( )
{
    if ( m_ptablecreate && m_ptablecreateW )
    {
        Assert( m_ptablecreateW->cColumns == m_ptablecreate->cColumns );
        for( ULONG iColumn = 0; iColumn < m_ptablecreate->cColumns ; iColumn++ )
        {
            m_ptablecreateW->rgcolumncreate[ iColumn ].columnid = m_rgcolumncreate[ iColumn ].columnid;
            m_ptablecreateW->rgcolumncreate[ iColumn ].err = m_rgcolumncreate[ iColumn ].err;
        }

        Assert( m_ptablecreateW->cIndexes == m_ptablecreate->cIndexes );
        for( ULONG iIndex = 0; iIndex < m_ptablecreate->cIndexes; iIndex++ )
        {
            m_ptablecreateW->rgindexcreate[ iIndex ].err = m_rgindexcreate[ iIndex ].err;
        }

        m_ptablecreateW->ulPages = m_ptablecreate->ulPages;
        m_ptablecreateW->tableid = m_ptablecreate->tableid;
        m_ptablecreateW->cCreated = m_ptablecreate->cCreated;
    }
}

CAutoTABLECREATE5::~CAutoTABLECREATE5()
{
    delete[] m_rgcolumncreate;
    delete[] m_rgszColumnName;

    delete[] m_rgindexcreate;
    delete[] m_rgindexcreateauto;
    delete m_ptablecreate;
}

JET_ERR JET_API JetCreateTableColumnIndexEx3W(
    __in JET_SESID              sesid,
    __in JET_DBID               dbid,
    __inout JET_TABLECREATE3_W *    ptablecreate );


LOCAL JET_ERR JetCreateTableColumnIndexEx2W(
    __in JET_SESID              sesid,
    __in JET_DBID               dbid,
    __inout JET_TABLECREATE2_W *    ptablecreate )
{
    ERR                 err             = JET_errSuccess;
    CAutoTABLECREATE2To3_T< JET_TABLECREATE2_W, JET_TABLECREATE3_W, CAutoINDEXCREATE1To2_T< JET_INDEXCREATE_W, JET_INDEXCREATE2_W > > tablecreate;

    CallR( tablecreate.ErrSet( ptablecreate ) );

    err = JetCreateTableColumnIndexEx3W( sesid, dbid, (JET_TABLECREATE3_W*)tablecreate );

    tablecreate.Result( );

    return err;
}

JET_ERR JET_API JetCreateTableColumnIndex2(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __inout JET_TABLECREATE2_A *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx2( sesid, dbid, ptablecreate ) );
}

JET_ERR JET_API JetCreateTableColumnIndex2W(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __inout JET_TABLECREATE2_W *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx2W( sesid, dbid, ptablecreate ) );
}

JET_ERR JET_API JetCreateTableColumnIndex3A(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __inout JET_TABLECREATE3_A *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx3( sesid, dbid, ptablecreate ) );
}

JET_ERR JET_API JetCreateTableColumnIndexEx3W(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __inout JET_TABLECREATE3_W *    ptablecreate )
{
    ERR                 err             = JET_errSuccess;
    CAutoTABLECREATE3   tablecreate;

    CallR( tablecreate.ErrSet( ptablecreate ) );

    Call( JetCreateTableColumnIndex3A( sesid, dbid, (JET_TABLECREATE3_A*)tablecreate ) );

HandleError:
    tablecreate.Result( );

    return err;
}

JET_ERR JET_API JetCreateTableColumnIndex3W(
    __in JET_SESID              sesid,
    __in JET_DBID               dbid,
    __inout JET_TABLECREATE3_W *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx3W( sesid, dbid, ptablecreate ) );
}

JET_ERR JET_API JetCreateTableColumnIndex4A(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __inout JET_TABLECREATE4_A *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx4( sesid, dbid, ptablecreate ) );
}

LOCAL JET_ERR JetCreateTableColumnIndexEx4W(
    __in JET_SESID              sesid,
    __in JET_DBID               dbid,
    __inout JET_TABLECREATE4_W *    ptablecreate )
{
    ERR                 err             = JET_errSuccess;
    CAutoTABLECREATE4   tablecreate;

    CallR( tablecreate.ErrSet( ptablecreate ) );

    Call( JetCreateTableColumnIndex4A( sesid, dbid, (JET_TABLECREATE4_A*)tablecreate ) );

HandleError:
    tablecreate.Result( );

    return err;
}

JET_ERR JET_API JetCreateTableColumnIndex4W(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __inout JET_TABLECREATE4_W *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx4W( sesid, dbid, ptablecreate ) );
}

JET_ERR JET_API JetCreateTableColumnIndex5A(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __inout JET_TABLECREATE5_A *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx( sesid, dbid, ptablecreate ) );
}

LOCAL JET_ERR JetCreateTableColumnIndexEx5W(
    __in JET_SESID              sesid,
    __in JET_DBID               dbid,
    __inout JET_TABLECREATE5_W *    ptablecreate )
{
    ERR                 err             = JET_errSuccess;
    CAutoTABLECREATE5   tablecreate;

    CallR( tablecreate.ErrSet( ptablecreate ) );

    Call( JetCreateTableColumnIndex5A( sesid, dbid, (JET_TABLECREATE5_A*)tablecreate ) );

HandleError:
    tablecreate.Result( );

    return err;
}

JET_ERR JET_API JetCreateTableColumnIndex5W(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __inout JET_TABLECREATE5_W *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx5W( sesid, dbid, ptablecreate ) );
}

LOCAL JET_ERR JetDeleteTableEx( __in JET_SESID sesid, __in JET_DBID dbid, __in JET_PCSTR szName )
{
    APICALL_SESID   apicall( opDeleteTable );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p[%s])",
            __FUNCTION__,
            sesid,
            dbid,
            szName,
            OSFormatString( szName ) ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        apicall.LeaveAfterCall( ErrIsamDeleteTable(
                                        sesid,
                                        dbid,
                                        (CHAR *)szName ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetDeleteTableExW( __in JET_SESID sesid, __in JET_DBID dbid, __in JET_PCWSTR wszName )
{
    ERR         err             = JET_errSuccess;
    CAutoSZDDL  lszName;

    CallR( lszName.ErrSet( wszName ) );

    return JetDeleteTableEx( sesid, dbid, lszName );
}

JET_ERR JET_API JetDeleteTableA( __in JET_SESID sesid, __in JET_DBID dbid, __in JET_PCSTR szName )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDeleteTable, JetDeleteTableEx( sesid, dbid, szName ) );
}

JET_ERR JET_API JetDeleteTableW( __in JET_SESID sesid, __in JET_DBID dbid, __in JET_PCWSTR wszName )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDeleteTable, JetDeleteTableExW( sesid, dbid, wszName ) );
}

LOCAL JET_ERR JetRenameTableEx(
    __in JET_SESID  sesid,
    __in JET_DBID   dbid,
    __in JET_PCSTR  szName,
    __in JET_PCSTR  szNameNew )
{
    APICALL_SESID   apicall( opRenameTable );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p[%s],0x%p[%s])",
            __FUNCTION__,
            sesid,
            dbid,
            szName,
            OSFormatString( szName ),
            szNameNew,
            OSFormatString( szNameNew ) ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        apicall.LeaveAfterCall( ErrIsamRenameTable(
                                        sesid,
                                        dbid,
                                        szName,
                                        szNameNew ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetRenameTableExW(
    __in JET_SESID  sesid,
    __in JET_DBID   dbid,
    __in JET_PCWSTR wszName,
    __in JET_PCWSTR wszNameNew )
{
    ERR         err;
    CAutoSZDDL  lszName;
    CAutoSZDDL  lszNameNew;

    CallR( lszName.ErrSet( wszName ) );
    CallR( lszNameNew.ErrSet( wszNameNew ) );

    return JetRenameTableEx( sesid, dbid, lszName, lszNameNew );
}

JET_ERR JET_API JetRenameTableA(
    __in JET_SESID  sesid,
    __in JET_DBID   dbid,
    __in JET_PCSTR  szName,
    __in JET_PCSTR  szNameNew )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opRenameTable, JetRenameTableEx( sesid, dbid, szName, szNameNew ) );
}

JET_ERR JET_API JetRenameTableW(
    __in JET_SESID  sesid,
    __in JET_DBID   dbid,
    __in JET_PCWSTR wszName,
    __in JET_PCWSTR wszNameNew )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opRenameTable, JetRenameTableExW( sesid, dbid, wszName, wszNameNew ) );
}


LOCAL JET_ERR JetRenameColumnEx(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_PCSTR      szName,
    __in JET_PCSTR      szNameNew,
    __in JET_GRBIT      grbit )
{
    APICALL_SESID       apicall( opRenameColumn );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p[%s],0x%p[%s],0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            szName,
            OSFormatString( szName ),
            szNameNew,
            OSFormatString( szNameNew ),
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispRenameColumn(
                                        sesid,
                                        tableid,
                                        szName,
                                        szNameNew,
                                        grbit ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetRenameColumnExW(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_PCWSTR     wszName,
    __in JET_PCWSTR     wszNameNew,
    __in JET_GRBIT      grbit )
{

    ERR         err;
    CAutoSZDDL  lszName;
    CAutoSZDDL  lszNameNew;

    CallR( lszName.ErrSet( wszName ) );
    CallR( lszNameNew.ErrSet( wszNameNew ) );

    return JetRenameColumnEx( sesid, tableid, lszName, lszNameNew, grbit );
}

JET_ERR JET_API JetRenameColumnA(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_PCSTR      szName,
    __in JET_PCSTR      szNameNew,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opRenameColumn, JetRenameColumnEx( sesid, tableid, szName, szNameNew, grbit ) );
}

JET_ERR JET_API JetRenameColumnW(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_PCWSTR     wszName,
    __in JET_PCWSTR     wszNameNew,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opRenameColumn, JetRenameColumnExW( sesid, tableid, wszName, wszNameNew, grbit ) );
}


LOCAL JET_ERR JetAddColumnEx(
    __in JET_SESID                              sesid,
    __in JET_TABLEID                            tableid,
    __in JET_PCSTR                              szColumnName,
    __in const JET_COLUMNDEF *                  pcolumndef,
    __in_bcount_opt( cbDefault ) const void *   pvDefault,
    __in ULONG                          cbDefault,
    __out_opt JET_COLUMNID *                    pcolumnid )
{
    APICALL_SESID       apicall( opAddColumn );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p[%s],0x%p[%s],0x%p,0x%x,0x%p)",
            __FUNCTION__,
            sesid,
            tableid,
            szColumnName,
            OSFormatString( szColumnName ),
            pcolumndef,
            ( NULL != pcolumndef ?
                        OSFormat(
                                "coltyp=0x%x,cp=0x%x,cbMax=0x%x,grbit=0x%x",
                                pcolumndef->coltyp,
                                pcolumndef->cp,
                                pcolumndef->cbMax,
                                pcolumndef->grbit ) :
                        OSTRACENULLPARAM ),
            pvDefault,
            cbDefault,
            pcolumnid ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispAddColumn(
                                        sesid,
                                        tableid,
                                        szColumnName,
                                        pcolumndef,
                                        pvDefault,
                                        cbDefault,
                                        pcolumnid ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetAddColumnExW(
    __in JET_SESID                              sesid,
    __in JET_TABLEID                            tableid,
    __in JET_PCWSTR                             wszColumnName,
    __in const JET_COLUMNDEF *                  pcolumndef,
    __in_bcount_opt( cbDefault ) const void *   pvDefault,
    __in ULONG                          cbDefault,
    __out_opt JET_COLUMNID *                    pcolumnid )
{
    ERR         err;
    CAutoSZDDL  lszColumnName;

    CallR( lszColumnName.ErrSet( wszColumnName ) );

    return JetAddColumnEx( sesid, tableid, lszColumnName, pcolumndef, pvDefault, cbDefault, pcolumnid );
}

JET_ERR JET_API JetAddColumnA(
    __in JET_SESID                              sesid,
    __in JET_TABLEID                            tableid,
    __in JET_PCSTR                              szColumnName,
    __in const JET_COLUMNDEF *                  pcolumndef,
    __in_bcount_opt( cbDefault ) const void *   pvDefault,
    __in ULONG                          cbDefault,
    __out_opt JET_COLUMNID *                    pcolumnid )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opAddColumn, JetAddColumnEx( sesid, tableid, szColumnName, pcolumndef, pvDefault, cbDefault, pcolumnid ) );
}

JET_ERR JET_API JetAddColumnW(
    __in JET_SESID                              sesid,
    __in JET_TABLEID                            tableid,
    __in JET_PCWSTR                             wszColumnName,
    __in const JET_COLUMNDEF *                  pcolumndef,
    __in_bcount_opt( cbDefault ) const void *   pvDefault,
    __in ULONG                          cbDefault,
    __out_opt JET_COLUMNID *                    pcolumnid )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opAddColumn, JetAddColumnExW( sesid, tableid, wszColumnName, pcolumndef, pvDefault, cbDefault, pcolumnid ) );
}


LOCAL JET_ERR JetDeleteColumnEx(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __in JET_PCSTR          szColumnName,
    __in const JET_GRBIT    grbit )
{
    APICALL_SESID   apicall( opDeleteColumn );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p[%s],0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            szColumnName,
            OSFormatString( szColumnName ),
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispDeleteColumn(
                                        sesid,
                                        tableid,
                                        szColumnName,
                                        grbit ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetDeleteColumnExW(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __in JET_PCWSTR         wszColumnName,
    __in const JET_GRBIT    grbit )
{
    ERR         err             = JET_errSuccess;
    CAutoSZDDL  lszColumnName;

    CallR( lszColumnName.ErrSet( wszColumnName ) );

    return JetDeleteColumnEx( sesid, tableid, lszColumnName, grbit );
}

JET_ERR JET_API JetDeleteColumnA(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_PCSTR      szColumnName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDeleteColumn, JetDeleteColumnEx( sesid, tableid, szColumnName, NO_GRBIT ) );
}
JET_ERR JET_API JetDeleteColumnW(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_PCWSTR     wszColumnName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDeleteColumn, JetDeleteColumnExW( sesid, tableid, wszColumnName, NO_GRBIT ) );
}
JET_ERR JET_API JetDeleteColumn2A(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __in JET_PCSTR          szColumnName,
    __in const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDeleteColumn, JetDeleteColumnEx( sesid, tableid, szColumnName, grbit ) );
}
JET_ERR JET_API JetDeleteColumn2W(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __in JET_PCWSTR         wszColumnName,
    __in const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDeleteColumn, JetDeleteColumnExW( sesid, tableid, wszColumnName, grbit ) );
}

LOCAL JET_ERR JetSetColumnDefaultValueEx(
    __in JET_SESID                      sesid,
    __in JET_DBID                       dbid,
    __in JET_PCSTR                      szTableName,
    __in JET_PCSTR                      szColumnName,
    __in_bcount( cbData ) const void *  pvData,
    __in const ULONG            cbData,
    __in const JET_GRBIT                grbit )
{
    APICALL_SESID       apicall( opSetColumnDefaultValue );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p[%s],0x%p[%s],0x%p,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            szTableName,
            OSFormatString( szTableName ),
            szColumnName,
            OSFormatString( szColumnName ),
            pvData,
            cbData,
            grbit ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        apicall.LeaveAfterCall( ErrIsamSetColumnDefaultValue(
                                        sesid,
                                        dbid,
                                        szTableName,
                                        szColumnName,
                                        pvData,
                                        cbData,
                                        grbit ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetSetColumnDefaultValueExW(
    __in JET_SESID                      sesid,
    __in JET_DBID                       dbid,
    __in JET_PCWSTR                     wszTableName,
    __in JET_PCWSTR                     wszColumnName,
    __in_bcount( cbData ) const void *  pvData,
    __in const ULONG            cbData,
    __in const JET_GRBIT                grbit )
{
    ERR         err             = JET_errSuccess;
    CAutoSZDDL  lszTableName;
    CAutoSZDDL  lszColumnName;

    CallR( lszTableName.ErrSet( wszTableName ) );
    CallR( lszColumnName.ErrSet( wszColumnName ) );

    return JetSetColumnDefaultValueEx(sesid, dbid, lszTableName, lszColumnName, pvData, cbData, grbit);
}

JET_ERR JET_API JetSetColumnDefaultValueA(
    __in JET_SESID                      sesid,
    __in JET_DBID                       dbid,
    __in JET_PCSTR                      szTableName,
    __in JET_PCSTR                      szColumnName,
    __in_bcount( cbData ) const void *  pvData,
    __in const ULONG            cbData,
    __in const JET_GRBIT                grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opSetColumnDefaultValue, JetSetColumnDefaultValueEx( sesid, dbid, szTableName, szColumnName, pvData, cbData, grbit ) );
}

JET_ERR JET_API JetSetColumnDefaultValueW(
    __in JET_SESID                      sesid,
    __in JET_DBID                       dbid,
    __in JET_PCWSTR                     wszTableName,
    __in JET_PCWSTR                     wszColumnName,
    __in_bcount( cbData ) const void *  pvData,
    __in const ULONG            cbData,
    __in const JET_GRBIT                grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opSetColumnDefaultValue, JetSetColumnDefaultValueExW( sesid, dbid, wszTableName, wszColumnName, pvData, cbData, grbit ) );
}

LOCAL JET_ERR JetCreateIndexEx(
    __in JET_SESID                                   sesid,
    __in JET_TABLEID                                 tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE3_A * pindexcreate,
    __in ULONG                               cIndexCreate )
{
    APICALL_SESID   apicall( opCreateIndex );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p[%s],0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            pindexcreate,
            ( NULL != pindexcreate ?
                        OSFormat( "szIndex=%s", OSFormatString( pindexcreate->szIndexName ) ) :
                        OSTRACENULLPARAM ),
            cIndexCreate ) );

    if ( apicall.FEnter( sesid ) )
    {
        {
            JET_ERR err;
            CAutoINDEXCREATE3To3_T< JET_INDEXCREATE3_A, JET_INDEXCREATE3_A > indexcreate;

            Call( indexcreate.ErrSet( pindexcreate, cIndexCreate ) );
            err = ErrDispCreateIndex(
                                        sesid,
                                        tableid,
                                        indexcreate.GetPtr(),
                                        cIndexCreate );

            indexcreate.Result();

HandleError:
            apicall.LeaveAfterCall( err );
        }
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetCreateIndexEx2(
    __in JET_SESID                                   sesid,
    __in JET_TABLEID                                 tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE2_A * pindexcreate,
    __in ULONG                               cIndexCreate )
{
    ERR err = JET_errSuccess;

    CAutoINDEXCREATE2To3_T< JET_INDEXCREATE2_A, JET_INDEXCREATE3_A > idxV3;

    Call( idxV3.ErrSet( pindexcreate, cIndexCreate ) );

    err = JetCreateIndexEx( sesid, tableid, idxV3.GetPtr(), cIndexCreate );

    idxV3.Result();

HandleError:

    return err;
}

LOCAL JET_ERR JetCreateIndexEx2W(
    __in JET_SESID                                  sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE2_W *    pindexcreate,
    __in ULONG                              cIndexCreate )
{
    ERR                   err               = JET_errSuccess;
    JET_INDEXCREATE2_A  * rgindexcreateEngine       = NULL;
    CAutoIDXCREATE2         * rgindexcreateauto     = NULL;
    UINT          iIndexCreate      = 0;

    if ( cIndexCreate )
    {
        Alloc( rgindexcreateEngine = new JET_INDEXCREATE2_A[cIndexCreate] );
        memset( rgindexcreateEngine, 0, cIndexCreate * sizeof(JET_INDEXCREATE_A) );
        Alloc( rgindexcreateauto = new CAutoIDXCREATE2[cIndexCreate] );
    }

    JET_INDEXCREATE2_W * pidxCurr = pindexcreate;
    JET_INDEXCREATE2_W * pidxNext = NULL;
    for( iIndexCreate = 0 ; iIndexCreate < cIndexCreate; iIndexCreate++, pidxCurr = pidxNext )
    {
        pidxNext = (JET_INDEXCREATE2_W *)(((BYTE*)pidxCurr) + pidxCurr->cbStruct );

        Assert( pidxCurr == ( pindexcreate + iIndexCreate ) );

        Call( rgindexcreateauto[iIndexCreate].ErrSet( pidxCurr ) );
        memcpy( rgindexcreateEngine + iIndexCreate, (JET_INDEXCREATE2_A*)(rgindexcreateauto[iIndexCreate]), sizeof( JET_INDEXCREATE2_A ) );
    }

    err = JetCreateIndexEx2( sesid, tableid, rgindexcreateEngine, cIndexCreate );

    for( iIndexCreate = 0 ; iIndexCreate < cIndexCreate; iIndexCreate++ )
    {
        rgindexcreateauto[iIndexCreate].Result( );
    }

HandleError:
    delete[] rgindexcreateEngine;
    delete[] rgindexcreateauto;
    return err;
}


JET_ERR JetCreateIndexEx1(
    __in JET_SESID                                  sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE_A * pindexcreate,
    __in ULONG                              cIndexCreate )
{
    ERR err = JET_errSuccess;

    CAutoINDEXCREATE1To2_T< JET_INDEXCREATE_A, JET_INDEXCREATE2_A > idxV2;

    Call( idxV2.ErrSet( pindexcreate, cIndexCreate ) );

    err = JetCreateIndexEx2( sesid, tableid, idxV2.GetPtr(), cIndexCreate );

    idxV2.Result();

HandleError:

    return err;
}


LOCAL JET_ERR JetCreateIndexEx1W(
    __in JET_SESID                                  sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE_W * pindexcreate,
    __in ULONG                              cIndexCreate )
{
    ERR                   err               = JET_errSuccess;
    JET_INDEXCREATE2_A  * rgindexcreateEngine       = NULL;
    CAutoIDXCREATE2         * rgindexcreateauto     = NULL;
    UINT          iIndexCreate      = 0;

    if ( cIndexCreate )
    {
        Alloc( rgindexcreateEngine = new JET_INDEXCREATE2_A[cIndexCreate] );
        memset( rgindexcreateEngine, 0, cIndexCreate * sizeof(JET_INDEXCREATE_A) );
        Alloc( rgindexcreateauto = new CAutoIDXCREATE2[cIndexCreate] );
    }

    JET_INDEXCREATE_W * pidxCurr = pindexcreate;
    JET_INDEXCREATE_W * pidxNext = NULL;
    for( iIndexCreate = 0 ; iIndexCreate < cIndexCreate; iIndexCreate++, pidxCurr = pidxNext )
    {
        pidxNext = (JET_INDEXCREATE_W *)(((BYTE*)pidxCurr) + pidxCurr->cbStruct );

        Assert( pidxCurr == ( pindexcreate + iIndexCreate ) );

        Call( rgindexcreateauto[iIndexCreate].ErrSet( pidxCurr ) );
        memcpy( rgindexcreateEngine + iIndexCreate, (JET_INDEXCREATE2_A*)(rgindexcreateauto[iIndexCreate]), sizeof( JET_INDEXCREATE2_A ) );
    }

    err = JetCreateIndexEx2( sesid, tableid, rgindexcreateEngine, cIndexCreate );

    for( iIndexCreate = 0 ; iIndexCreate < cIndexCreate; iIndexCreate++ )
    {
        rgindexcreateauto[iIndexCreate].Result( );
    }

HandleError:
    delete[] rgindexcreateEngine;
    delete[] rgindexcreateauto;
    return err;
}

LOCAL JET_ERR JetCreateIndexEx3W(
    __in JET_SESID                                  sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE3_W *pindexcreate,
    __in ULONG                              cIndexCreate )
{
    ERR                   err               = JET_errSuccess;
    JET_INDEXCREATE3_A  * rgindexcreateEngine       = NULL;
    CAutoIDXCREATE3         * rgindexcreateauto     = NULL;
    UINT          iIndexCreate      = 0;

    if ( cIndexCreate )
    {
        Alloc( rgindexcreateEngine = new JET_INDEXCREATE3_A[cIndexCreate] );
        memset( rgindexcreateEngine, 0, cIndexCreate * sizeof(JET_INDEXCREATE3_A) );
        Alloc( rgindexcreateauto = new CAutoIDXCREATE3[cIndexCreate] );
    }

    JET_INDEXCREATE3_W * pidxCurr = pindexcreate;
    JET_INDEXCREATE3_W * pidxNext = NULL;
    for ( iIndexCreate = 0 ; iIndexCreate < cIndexCreate; iIndexCreate++, pidxCurr = pidxNext )
    {
        pidxNext = (JET_INDEXCREATE3_W *)(((BYTE*)pidxCurr) + pidxCurr->cbStruct );

        Assert( pidxCurr == ( pindexcreate + iIndexCreate ) );

        Assert( pidxCurr->cbStruct == sizeof( JET_INDEXCREATE3_W ) );

        Call( rgindexcreateauto[iIndexCreate].ErrSet( pidxCurr ) );
        memcpy( rgindexcreateEngine + iIndexCreate, (JET_INDEXCREATE3_A*)(rgindexcreateauto[iIndexCreate]), sizeof( JET_INDEXCREATE3_A ) );
    }

    err = JetCreateIndexEx( sesid, tableid, rgindexcreateEngine, cIndexCreate );

    for( iIndexCreate = 0 ; iIndexCreate < cIndexCreate; iIndexCreate++ )
    {
        rgindexcreateauto[iIndexCreate].Result( );
    }

HandleError:
    delete[] rgindexcreateEngine;
    delete[] rgindexcreateauto;
    return err;
}

JET_ERR JET_API JetCreateIndexA(
    __in JET_SESID                      sesid,
    __in JET_TABLEID                    tableid,
    __in JET_PCSTR                      szIndexName,
    __in JET_GRBIT                      grbit,
    __in_bcount( cbKey ) const char *   szKey,
    __in ULONG                  cbKey,
    __in ULONG                  lDensity )
{
    JET_INDEXCREATE2_A  idxcreate;
    JET_SPACEHINTS      idxjsh;

    memset( &idxcreate, 0, sizeof(idxcreate) );
    idxcreate.cbStruct      = sizeof( idxcreate );
    idxcreate.szIndexName   = (CHAR *)szIndexName;
    idxcreate.szKey         = (CHAR *)szKey;
    idxcreate.cbKey         = cbKey;
    idxcreate.grbit         = grbit;
    idxcreate.ulDensity     = 0;
    idxcreate.lcid          = 0;
    idxcreate.cbVarSegMac   = 0;
    idxcreate.rgconditionalcolumn   = 0;
    idxcreate.cConditionalColumn    = 0;
    idxcreate.err           = JET_errSuccess;
    idxcreate.cbKeyMost     = 255;
    idxcreate.pSpacehints   = &idxjsh;

    memset( &idxjsh, 0, sizeof(idxjsh) );
    idxjsh.cbStruct         = sizeof(idxjsh);
    idxjsh.ulInitialDensity = lDensity;

    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx2( sesid, tableid, &idxcreate, 1 ) );
}

JET_ERR JET_API JetCreateIndexW(
    __in JET_SESID                      sesid,
    __in JET_TABLEID                    tableid,
    __in JET_PCWSTR                     wszIndexName,
    __in JET_GRBIT                      grbit,
    __in_bcount( cbKey ) const WCHAR *  wszKey,
    __in ULONG                  cbKey,
    __in ULONG                  lDensity )
{
    JET_INDEXCREATE_W   idxcreate;

    idxcreate.cbStruct      = sizeof( JET_INDEXCREATE_W );
    idxcreate.szIndexName   = (WCHAR *)wszIndexName;
    idxcreate.szKey         = (WCHAR *)wszKey;
    idxcreate.cbKey         = cbKey;
    idxcreate.grbit         = grbit;
    idxcreate.ulDensity     = lDensity;
    idxcreate.lcid          = 0;
    idxcreate.cbVarSegMac   = 0;
    idxcreate.rgconditionalcolumn   = 0;
    idxcreate.cConditionalColumn    = 0;
    idxcreate.err           = JET_errSuccess;
    idxcreate.cbKeyMost     = 255;

    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx1W( sesid, tableid, &idxcreate, 1 ) );
}

JET_ERR JET_API JetCreateIndex2A(
    __in JET_SESID                                  sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE_A * pindexcreate,
    __in ULONG                              cIndexCreate )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx1( sesid, tableid, pindexcreate, cIndexCreate ) );
}

JET_ERR JET_API JetCreateIndex2W(
    __in JET_SESID                                  sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE_W * pindexcreate,
    __in ULONG                              cIndexCreate )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx1W( sesid, tableid, pindexcreate, cIndexCreate ) );
}

JET_ERR JET_API JetCreateIndex3A(
    __in JET_SESID                                  sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE2_A *pindexcreate,
    __in ULONG                              cIndexCreate )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx2( sesid, tableid, pindexcreate, cIndexCreate ) );
}

JET_ERR JET_API JetCreateIndex3W(
    __in JET_SESID                                  sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE2_W *pindexcreate,
    __in ULONG                              cIndexCreate )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx2W( sesid, tableid, pindexcreate, cIndexCreate ) );
}


JET_ERR JET_API JetCreateIndex4A(
    __in JET_SESID                                  sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE3_A *pindexcreate,
    __in ULONG                              cIndexCreate )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx( sesid, tableid, pindexcreate, cIndexCreate ) );
}

JET_ERR JET_API JetCreateIndex4W(
    __in JET_SESID                                  sesid,
    __in JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE3_W *pindexcreate,
    __in ULONG                              cIndexCreate )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx3W( sesid, tableid, pindexcreate, cIndexCreate ) );
}


LOCAL JET_ERR JetDeleteIndexEx(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_PCSTR      szIndexName )
{
    APICALL_SESID   apicall( opDeleteIndex );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p[%s])",
            __FUNCTION__,
            sesid,
            tableid,
            szIndexName,
            OSFormatString( szIndexName ) ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispDeleteIndex(
                                        sesid,
                                        tableid,
                                        szIndexName ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetDeleteIndexExW(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_PCWSTR     wszIndexName )
{
    ERR         err;
    CAutoSZDDL  lszIndexName;

    CallR( lszIndexName.ErrSet( wszIndexName ) );

    return JetDeleteIndexEx( sesid, tableid, lszIndexName );
}

JET_ERR JET_API JetDeleteIndexA(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_PCSTR      szIndexName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDeleteIndex, JetDeleteIndexEx( sesid, tableid, szIndexName ) );
}


JET_ERR JET_API JetDeleteIndexW(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_PCWSTR     wszIndexName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDeleteIndex, JetDeleteIndexExW( sesid, tableid, wszIndexName ) );
}

LOCAL JET_ERR JetComputeStatsEx( __in JET_SESID sesid, __in JET_TABLEID tableid )
{
    APICALL_SESID   apicall( opComputeStats );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix)",
            __FUNCTION__,
            sesid,
            tableid ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispComputeStats( sesid, tableid ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetComputeStats( __in JET_SESID sesid, __in JET_TABLEID tableid )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opComputeStats, JetComputeStatsEx( sesid, tableid ) );
}


LOCAL JET_ERR JetAttachDatabaseEx(
    __in JET_SESID                                              sesid,
    __in JET_PCWSTR                                             wszDbFileName,
    _In_reads_opt_( csetdbparam ) const JET_SETDBPARAM * const  rgsetdbparam,
    _In_ const ULONG                                    csetdbparam,
    _In_ const JET_GRBIT                                        grbit )
{
    APICALL_SESID   apicall( opAttachDatabase );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p[%s],0x%p,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            wszDbFileName,
            SzOSFormatStringW( wszDbFileName ),
            rgsetdbparam,
            csetdbparam,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamAttachDatabase( sesid, wszDbFileName, fTrue, rgsetdbparam, csetdbparam, grbit ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetAttachDatabaseExA(
    __in JET_SESID                                              sesid,
    __in JET_PCSTR                                              szDbFileName,
    _In_reads_opt_( csetdbparam ) const JET_SETDBPARAM * const  rgsetdbparam,
    _In_ const ULONG                                    csetdbparam,
    _In_ const JET_GRBIT                                        grbit )
{
    ERR             err;
    CAutoWSZPATH    lwszDatabaseName;

    CallR( lwszDatabaseName.ErrSet( szDbFileName ) );

    return JetAttachDatabaseEx( sesid, lwszDatabaseName, rgsetdbparam, csetdbparam, grbit );
}

JET_ERR JET_API JetAttachDatabaseA(
    __in JET_SESID  sesid,
    __in JET_PCSTR  szFilename,
    __in JET_GRBIT  grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opAttachDatabase, JetAttachDatabaseExA( sesid, szFilename, NULL, 0, grbit ) );
}

JET_ERR JET_API JetAttachDatabaseW(
    __in JET_SESID  sesid,
    __in JET_PCWSTR wszFilename,
    __in JET_GRBIT  grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opAttachDatabase, JetAttachDatabaseEx( sesid, wszFilename, NULL, 0, grbit ) );
}

JET_ERR JET_API JetAttachDatabase2A(
    __in JET_SESID              sesid,
    __in JET_PCSTR              szFilename,
    __in const ULONG    cpgDatabaseSizeMax,
    __in JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    const JET_SETDBPARAM rgsetdbparam[] =
    {
            { JET_dbparamDbSizeMaxPages, (void*)&cpgDatabaseSizeMax, sizeof( cpgDatabaseSizeMax ) }
    };
    JET_TRY( opAttachDatabase, JetAttachDatabaseExA( sesid, szFilename, rgsetdbparam, _countof( rgsetdbparam ), grbit ) );
}

JET_ERR JET_API JetAttachDatabase2W(
    __in JET_SESID              sesid,
    __in JET_PCWSTR             wszFilename,
    __in const ULONG    cpgDatabaseSizeMax,
    __in JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    const JET_SETDBPARAM rgsetdbparam[] =
    {
            { JET_dbparamDbSizeMaxPages, (void*)&cpgDatabaseSizeMax, sizeof( cpgDatabaseSizeMax ) }
    };
    JET_TRY( opAttachDatabase, JetAttachDatabaseEx( sesid, wszFilename, rgsetdbparam, _countof( rgsetdbparam ), grbit ) );
}

JET_ERR JET_API JetAttachDatabase3A(
    _In_ JET_SESID                                  sesid,
    _In_ JET_PCSTR                                  szFilename,
    _In_reads_opt_( csetdbparam ) JET_SETDBPARAM *  rgsetdbparam,
    _In_ ULONG                              csetdbparam,
    _In_ JET_GRBIT                                  grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opAttachDatabase, JetAttachDatabaseExA( sesid, szFilename, rgsetdbparam, csetdbparam, grbit ) );
}

JET_ERR JET_API JetAttachDatabase3W(
    _In_ JET_SESID                                  sesid,
    _In_ JET_PCWSTR                                 wszFilename,
    _In_reads_opt_( csetdbparam ) JET_SETDBPARAM *  rgsetdbparam,
    _In_ ULONG                              csetdbparam,
    _In_ JET_GRBIT                                  grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opAttachDatabase, JetAttachDatabaseEx( sesid, wszFilename, rgsetdbparam, csetdbparam, grbit ) );
}

JET_ERR JET_API JetAttachDatabaseWithStreamingA(
    __in JET_SESID              sesid,
    __in JET_PCSTR              szDbFileName,
    _Reserved_ JET_PCSTR        szSLVFileName,
    _Reserved_ JET_PCSTR        szSLVRootName,
    __in const ULONG    cpgDatabaseSizeMax,
    __in JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    return ErrERRCheck( JET_errFeatureNotAvailable );
}

JET_ERR JET_API JetAttachDatabaseWithStreamingW(
    __in JET_SESID              sesid,
    __in JET_PCWSTR             wszDbFileName,
    _Reserved_ JET_PCWSTR       wszSLVFileName,
    _Reserved_ JET_PCWSTR       wszSLVRootName,
    __in const ULONG    cpgDatabaseSizeMax,
    __in JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    return ErrERRCheck( JET_errFeatureNotAvailable );
}

LOCAL JET_ERR JetDetachDatabaseEx(
    __in JET_SESID  sesid,
    __in_opt JET_PCWSTR wszFilename,
    __in JET_GRBIT  grbit)
{
    APICALL_SESID   apicall( opDetachDatabase );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p[%s],0x%x)",
            __FUNCTION__,
            sesid,
            wszFilename,
            SzOSFormatStringW( wszFilename ),
            grbit ) );

    if ( apicall.FEnter( sesid, fTrue ) )
    {
        const BOOL  fForceDetach    = ( JET_bitForceDetach & grbit );
        if ( fForceDetach )
        {
            apicall.LeaveAfterCall( ErrERRCheck( JET_errForceDetachNotAllowed ) );
        }
        else
        {
            apicall.LeaveAfterCall( ErrIsamDetachDatabase( sesid, NULL, wszFilename, grbit ) );
        }
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetDetachDatabaseExA(
    __in JET_SESID  sesid,
    __in_opt JET_PCSTR  szFilename,
    __in JET_GRBIT  grbit)
{
    ERR             err;
    CAutoWSZPATH    lwszFilename;

    CallR( lwszFilename.ErrSet( szFilename ) );

    return JetDetachDatabaseEx( sesid, lwszFilename, grbit );
}

JET_ERR JET_API JetDetachDatabaseA(
    __in JET_SESID  sesid,
    __in_opt JET_PCSTR  szFilename )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDetachDatabase, JetDetachDatabaseExA( sesid, szFilename, NO_GRBIT ) );
}

JET_ERR JET_API JetDetachDatabaseW(
    __in JET_SESID  sesid,
    __in_opt JET_PCWSTR wszFilename )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDetachDatabase, JetDetachDatabaseEx( sesid, wszFilename, NO_GRBIT ) );
}

JET_ERR JET_API JetDetachDatabase2A(
    __in JET_SESID  sesid,
    __in_opt JET_PCSTR  szFilename,
    __in JET_GRBIT  grbit)
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDetachDatabase, JetDetachDatabaseExA( sesid, szFilename, grbit ) );
}

JET_ERR JET_API JetDetachDatabase2W(
    __in JET_SESID  sesid,
    __in_opt JET_PCWSTR wszFilename,
    __in JET_GRBIT  grbit)
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDetachDatabase, JetDetachDatabaseEx( sesid, wszFilename, grbit ) );
}


LOCAL JET_ERR JetBackupInstanceEx(
    JET_INSTANCE    instance,
    __in PCWSTR     wszBackupPath,
    JET_GRBIT       grbit,
    JET_PFNSTATUS   pfnStatus )
{
    APICALL_INST    apicall( opBackupInstance );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p[%s],0x%x,0x%p)",
            __FUNCTION__,
            instance,
            wszBackupPath,
            SzOSFormatStringW( wszBackupPath ),
            grbit,
            pfnStatus ) );

    if ( apicall.FEnter( instance ) )
    {
        InitCallbackWrapper initCallbackWrapper(pfnStatus);
        apicall.LeaveAfterCall( apicall.Pinst()->m_fBackupAllowed ?
            ErrIsamBackup( (JET_INSTANCE)apicall.Pinst(), wszBackupPath, grbit, InitCallbackWrapper::PfnWrapper, &initCallbackWrapper ) :
            ErrERRCheck( JET_errBackupNotAllowedYet ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetBackupInstanceExA(
    __in JET_INSTANCE   instance,
    __in JET_PCSTR      szBackupPath,
    __in JET_GRBIT      grbit,
    __in_opt JET_PFNSTATUS  pfnStatus )
{
    ERR             err;
    CAutoWSZPATH    lwszBackupPath;

    CallR( lwszBackupPath.ErrSet( szBackupPath ) );

    return JetBackupInstanceEx( instance, lwszBackupPath, grbit, pfnStatus );
}

JET_ERR JET_API JetBackupInstanceA(
    __in JET_INSTANCE   instance,
    __in JET_PCSTR      szBackupPath,
    __in JET_GRBIT      grbit,
    __in_opt JET_PFNSTATUS  pfnStatus )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBackupInstance, JetBackupInstanceExA( instance, szBackupPath, grbit, pfnStatus ) );
}

JET_ERR JET_API JetBackupInstanceW(
    __in JET_INSTANCE   instance,
    __in JET_PCWSTR     wszBackupPath,
    __in JET_GRBIT      grbit,
    __in_opt JET_PFNSTATUS  pfnStatus )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBackupInstance, JetBackupInstanceEx( instance, wszBackupPath, grbit, pfnStatus ) );
}

JET_ERR JET_API JetBackupA(
    __in JET_PCSTR      szBackupPath,
    __in JET_GRBIT      grbit,
    __in_opt JET_PFNSTATUS  pfnStatus )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetBackupInstanceA( (JET_INSTANCE)g_rgpinst[0], szBackupPath, grbit, pfnStatus );
}

JET_ERR JET_API JetBackupW(
    __in JET_PCWSTR     wszBackupPath,
    __in JET_GRBIT      grbit,
    __in_opt JET_PFNSTATUS  pfnStatus )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetBackupInstanceW( (JET_INSTANCE)g_rgpinst[0], wszBackupPath, grbit, pfnStatus );
}

JET_ERR JET_API JetRestoreA(    __in JET_PCSTR szSource, __in_opt JET_PFNSTATUS pfn )
{
    ERR     err;

    OnDebug( const BOOL fInitd = ( RUNINSTGetMode() != runInstModeNoSet ) );

    CallR( ErrRUNINSTCheckAndSetOneInstMode() );

    Assert( fInitd == ( g_rgpinst != NULL ) );

    err = JetRestoreInstanceA( g_rgpinst ? (JET_INSTANCE)g_rgpinst[0] : NULL, szSource, NULL, pfn );

    Assert( fInitd == ( RUNINSTGetMode() != runInstModeNoSet ) );
    Assert( fInitd || ( RUNINSTGetMode() == runInstModeNoSet ) );

    return err;
}
JET_ERR JET_API JetRestoreW(    __in JET_PCWSTR wszSource, __in_opt JET_PFNSTATUS pfn )
{
    ERR     err;

    OnDebug( const BOOL fInitd = ( RUNINSTGetMode() != runInstModeNoSet ) );

    CallR( ErrRUNINSTCheckAndSetOneInstMode() );

    Assert( fInitd == ( g_rgpinst != NULL ) );

    err = JetRestoreInstanceW( g_rgpinst ? (JET_INSTANCE)g_rgpinst[0] : NULL, wszSource, NULL, pfn );

    Assert( fInitd == ( RUNINSTGetMode() != runInstModeNoSet ) );
    Assert( fInitd || ( RUNINSTGetMode() == runInstModeNoSet ) );

    return err;
}

JET_ERR JET_API JetRestore2A( __in JET_PCSTR sz, __in_opt JET_PCSTR szDest, __in_opt JET_PFNSTATUS pfn )
{
    ERR     err;

    OnDebug( const BOOL fInitd = ( RUNINSTGetMode() != runInstModeNoSet ) );

    CallR( ErrRUNINSTCheckAndSetOneInstMode() );

    Assert( fInitd == ( g_rgpinst != NULL ) );

    err = JetRestoreInstanceA( g_rgpinst ? (JET_INSTANCE)g_rgpinst[0] : NULL, sz, szDest, pfn );

    Assert( fInitd == ( RUNINSTGetMode() != runInstModeNoSet ) );
    Assert( fInitd || ( RUNINSTGetMode() == runInstModeNoSet ) );

    return err;
}

JET_ERR JET_API JetRestore2W(__in JET_PCWSTR wsz, __in_opt JET_PCWSTR wszDest, __in_opt JET_PFNSTATUS pfn )
{
    ERR     err;

    OnDebug( const BOOL fInitd = ( RUNINSTGetMode() != runInstModeNoSet ) );

    CallR( ErrRUNINSTCheckAndSetOneInstMode() );

    Assert( fInitd == ( g_rgpinst != NULL ) );

    err = JetRestoreInstanceW( g_rgpinst ? (JET_INSTANCE)g_rgpinst[0] : NULL, wsz, wszDest, pfn );

    Assert( fInitd == ( RUNINSTGetMode() != runInstModeNoSet ) );
    Assert( fInitd || ( RUNINSTGetMode() == runInstModeNoSet ) );

    return err;
}


LOCAL JET_ERR JetOpenTempTableEx(
    __in JET_SESID                                  sesid,
    __in_ecount( ccolumn ) const JET_COLUMNDEF *    prgcolumndef,
    __in ULONG                              ccolumn,
    __in_opt JET_UNICODEINDEX2 *                    pidxunicode,
    __in JET_GRBIT                                  grbit,
    __out JET_TABLEID *                             ptableid,
    __out_ecount( ccolumn ) JET_COLUMNID *          prgcolumnid,
    __in ULONG                              cbKeyMost       = 0,
    __in ULONG                              cbVarSegMax     = 0 )
{
    APICALL_SESID       apicall( opOpenTempTable );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p,0x%x,0x%p,0x%x,0x%p,0x%p)",
            __FUNCTION__,
            sesid,
            prgcolumndef,
            ccolumn,
            pidxunicode,
            grbit,
            ptableid,
            prgcolumnid ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamOpenTempTable(
                                        sesid,
                                        prgcolumndef,
                                        ccolumn,
                                        pidxunicode,
                                        grbit,
                                        ptableid,
                                        prgcolumnid,
                                        cbKeyMost,
                                        cbVarSegMax ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetOpenTempTableEx1(
    __in JET_SESID                                  sesid,
    __in_ecount( ccolumn ) const JET_COLUMNDEF *    prgcolumndef,
    __in ULONG                              ccolumn,
    __in_opt JET_UNICODEINDEX *                     pidxunicode,
    __in JET_GRBIT                                  grbit,
    __out JET_TABLEID *                             ptableid,
    __out_ecount( ccolumn ) JET_COLUMNID *          prgcolumnid,
    __in ULONG                              cbKeyMost       = 0,
    __in ULONG                              cbVarSegMax     = 0 )
{
    ERR err;
    JET_UNICODEINDEX2 idxunicode;
    WCHAR wszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH];

    if ( pidxunicode )
    {
        idxunicode.dwMapFlags = pidxunicode->dwMapFlags;
        idxunicode.szLocaleName = wszLocaleName;
        CallR( ErrNORMLcidToLocale( pidxunicode->lcid, idxunicode.szLocaleName, _countof( wszLocaleName ) ) );
    }

    return JetOpenTempTableEx( sesid,
            prgcolumndef,
            ccolumn,
            pidxunicode ? &idxunicode : NULL,
            grbit,
            ptableid,
            prgcolumnid,
            cbKeyMost,
            cbVarSegMax );
}

JET_ERR JET_API JetOpenTempTable(
    __in JET_SESID                                  sesid,
    __in_ecount( ccolumn ) const JET_COLUMNDEF *    prgcolumndef,
    __in ULONG                              ccolumn,
    __in JET_GRBIT                                  grbit,
    __out JET_TABLEID *                             ptableid,
    __out_ecount( ccolumn ) JET_COLUMNID *          prgcolumnid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOpenTempTable, JetOpenTempTableEx1( sesid, prgcolumndef, ccolumn, NULL, grbit, ptableid, prgcolumnid ) );
}
JET_ERR JET_API JetOpenTempTable2(
    __in JET_SESID                                  sesid,
    __in_ecount( ccolumn ) const JET_COLUMNDEF *    prgcolumndef,
    __in ULONG                              ccolumn,
    __in ULONG                              lcid,
    __in JET_GRBIT                                  grbit,
    __out JET_TABLEID *                             ptableid,
    __out_ecount( ccolumn ) JET_COLUMNID *          prgcolumnid )
{
    INST * const        pinst       = PinstFromSesid( sesid );
    JET_UNICODEINDEX2 idxunicode;
    WCHAR wszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH];

    idxunicode.dwMapFlags = pinst->m_dwLCMapFlagsDefault;
    if ( lcidNone == lcid )
    {
        ERR err = JET_errSuccess;
        CallR( ErrOSStrCbCopyW( wszLocaleName, sizeof( wszLocaleName ), pinst->m_wszLocaleNameDefault ) );
    }
    else
    {
        ERR err = JET_errSuccess;
        CallR( ErrNORMLcidToLocale( lcid, wszLocaleName, _countof( wszLocaleName ) ) );
    }

    idxunicode.szLocaleName = wszLocaleName;

    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOpenTempTable, JetOpenTempTableEx( sesid, prgcolumndef, ccolumn, &idxunicode, grbit, ptableid, prgcolumnid ) );
}
JET_ERR JET_API JetOpenTempTable3(
    __in JET_SESID                                  sesid,
    __in_ecount( ccolumn ) const JET_COLUMNDEF *    prgcolumndef,
    __in ULONG                              ccolumn,
    __in_opt JET_UNICODEINDEX *                     pidxunicode,
    __in JET_GRBIT                                  grbit,
    __out JET_TABLEID *                             ptableid,
    __out_ecount( ccolumn ) JET_COLUMNID *          prgcolumnid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOpenTempTable, JetOpenTempTableEx1( sesid, prgcolumndef, ccolumn, pidxunicode, grbit, ptableid, prgcolumnid ) );
}

JET_ERR JET_API JetOpenTemporaryTable(
    __in JET_SESID                  sesid,
    __in JET_OPENTEMPORARYTABLE *   popentemporarytable )
{
    JET_VALIDATE_SESID( sesid );

    if ( sizeof(JET_OPENTEMPORARYTABLE) != popentemporarytable->cbStruct )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    JET_TRY( opOpenTempTable,
        JetOpenTempTableEx1( sesid,
            popentemporarytable->prgcolumndef,
            popentemporarytable->ccolumn,
            popentemporarytable->pidxunicode,
            popentemporarytable->grbit,
            &popentemporarytable->tableid,
            popentemporarytable->prgcolumnid,
            popentemporarytable->cbKeyMost,
            popentemporarytable->cbVarSegMac
            ) );
}

JET_ERR JET_API JetOpenTemporaryTable2(
    __in JET_SESID                  sesid,
    __in JET_OPENTEMPORARYTABLE2 *   popentemporarytable )
{
    JET_VALIDATE_SESID( sesid );

    if ( sizeof(JET_OPENTEMPORARYTABLE2) != popentemporarytable->cbStruct )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    JET_TRY( opOpenTempTable,
        JetOpenTempTableEx( sesid,
            popentemporarytable->prgcolumndef,
            popentemporarytable->ccolumn,
            popentemporarytable->pidxunicode,
            popentemporarytable->grbit,
            &popentemporarytable->tableid,
            popentemporarytable->prgcolumnid,
            popentemporarytable->cbKeyMost,
            popentemporarytable->cbVarSegMac
            ) );
}

LOCAL JET_ERR JetSetIndexRangeEx(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_GRBIT      grbit )
{
    APICALL_SESID   apicall( opSetIndexRange );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispSetIndexRange(
                                        sesid,
                                        tableid,
                                        grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetSetIndexRange(
    __in JET_SESID      sesid,
    __in JET_TABLEID    tableid,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetIndexRange, JetSetIndexRangeEx( sesid, tableid, grbit ) );
}

LOCAL JET_ERR JetIndexRecordCountEx(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __out ULONG64 *         pcrec,
    __in ULONG64            crecMax )
{
    APICALL_SESID   apicall( opIndexRecordCount );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%I64x)",
            __FUNCTION__,
            sesid,
            tableid,
            pcrec,
            crecMax ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispIndexRecordCount(
                                        sesid,
                                        tableid,
                                        pcrec,
                                        crecMax ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetIndexRecordCountEx32Bit(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __out ULONG *           pcrec,
    __in ULONG              crecMax )
{
    ERR err = JET_errSuccess;
    ULONG64 crec = 0;

    ProbeClientBuffer( pcrec, sizeof( ULONG ) );
    *pcrec = 0;

    err = JetIndexRecordCountEx( sesid, tableid, &crec, (ULONG64) crecMax );

    if ( crec > ulMax )
    {
        err = ErrERRCheck( JET_errTooManyRecords );
    }
    else
    {
        *pcrec = (ULONG) crec;
    }

    return err;
}

JET_ERR JET_API JetIndexRecordCount(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __out ULONG *   pcrec,
    __in ULONG      crecMax )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opIndexRecordCount, JetIndexRecordCountEx32Bit( sesid, tableid, pcrec, crecMax ) );
}

JET_ERR JET_API JetIndexRecordCount2(
    __in JET_SESID          sesid,
    __in JET_TABLEID        tableid,
    __out ULONG64 *         pcrec,
    __in ULONG64            crecMax )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opIndexRecordCount, JetIndexRecordCountEx( sesid, tableid, pcrec, crecMax ) );
}


LOCAL JET_ERR JetBeginExternalBackupInstanceEx( __in JET_INSTANCE instance, __in JET_GRBIT grbit )
{
    APICALL_INST    apicall( opBeginExternalBackupInstance );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x)",
            __FUNCTION__,
            instance,
            grbit ) );

    if ( apicall.FEnter( instance ) )
    {
        const BOOL fInternalCopyBackup = ( JET_bitInternalCopy & grbit ) != 0;
        apicall.LeaveAfterCall( apicall.Pinst()->m_fBackupAllowed || ( apicall.Pinst()->FRecovering() && fInternalCopyBackup ) ?
                                        ErrIsamBeginExternalBackup( (JET_INSTANCE)apicall.Pinst() , grbit ) :
                                        ErrERRCheck( JET_errBackupNotAllowedYet ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetBeginExternalBackupInstance( __in JET_INSTANCE instance, __in JET_GRBIT grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBeginExternalBackupInstance, JetBeginExternalBackupInstanceEx( instance, grbit ) );
}
JET_ERR JET_API JetBeginExternalBackup( __in JET_GRBIT grbit )
{
    ERR     err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetBeginExternalBackupInstance( (JET_INSTANCE)g_rgpinst[0], grbit );
}


LOCAL JET_ERR JetGetAttachInfoInstanceEx(
    __in JET_INSTANCE                                       instance,
    __out_bcount_part_opt( cbMax, *pcbActual ) JET_PWSTR    wszzDatabases,
    __in ULONG                                      cbMax,
    __out_opt ULONG *                               pcbActual )
{
    APICALL_INST    apicall( opGetAttachInfoInstance );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,%ls,0x%x,0x%p)",
            __FUNCTION__,
            instance,
            wszzDatabases,
            cbMax,
            pcbActual ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamGetAttachInfo(
                                        (JET_INSTANCE)apicall.Pinst(),
                                        wszzDatabases,
                                        cbMax,
                                        pcbActual ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetGetAttachInfoInstanceExA(
    __in JET_INSTANCE                                   instance,
    __out_bcount_part_opt( cbMax, *pcbActual ) JET_PSTR szzDatabases,
    __in ULONG                                  cbMax,
    __out_opt ULONG *                           pcbActual )
{
    ERR             err         = JET_errSuccess;
    ULONG   cbActual;
    CAutoWSZ        lwszz;
    size_t  cchActual;

    Call( JetGetAttachInfoInstanceEx( instance, 0, NULL, &cbActual ) );

    Call( lwszz.ErrAlloc( cbActual ));

    Call( JetGetAttachInfoInstanceEx( instance, lwszz.Pv(), lwszz.Cb(), &cbActual ) );

    Call( ErrOSSTRUnicodeToAsciiM( lwszz.Pv(), szzDatabases, cbMax, &cchActual ) );

    if ( pcbActual )
    {
        *pcbActual = cchActual * sizeof( CHAR );
    }
HandleError:
    return err;
}

JET_ERR JET_API JetGetAttachInfoInstanceA(
    _In_ JET_INSTANCE                                   instance,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PSTR szzDatabases,
    _In_ ULONG                                  cbMax,
    _Out_opt_ ULONG *                           pcbActual )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opGetAttachInfoInstance, JetGetAttachInfoInstanceExA( instance, szzDatabases, cbMax, pcbActual ) );
}


JET_ERR JET_API JetGetAttachInfoInstanceW(
    _In_ JET_INSTANCE                                       instance,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    wszzDatabases,
    _In_ ULONG                                      cbMax,
    _Out_opt_ ULONG *                               pcbActual )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opGetAttachInfoInstance, JetGetAttachInfoInstanceEx( instance, wszzDatabases, cbMax, pcbActual ) );
}

JET_ERR JET_API JetGetAttachInfoA(
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PSTR szzDatabases,
    _In_ ULONG                                  cbMax,
    _Out_opt_ ULONG *                           pcbActual )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetGetAttachInfoInstanceA( (JET_INSTANCE)g_rgpinst[0], szzDatabases, cbMax, pcbActual );
}

JET_ERR JET_API JetGetAttachInfoW(
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    wszzDatabases,
    _In_ ULONG                                      cbMax,
    _Out_opt_ ULONG *                               pcbActual )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetGetAttachInfoInstanceW( (JET_INSTANCE)g_rgpinst[0], wszzDatabases, cbMax, pcbActual );
}


LOCAL JET_ERR JetOpenFileInstanceEx(
    __in JET_INSTANCE       instance,
    __in JET_PCWSTR         wszFileName,
    __out JET_HANDLE *      phfFile,
    __in ULONG64            ibRead,
    __out ULONG *   pulFileSizeLow,
    __out ULONG *   pulFileSizeHigh )
{
    APICALL_INST    apicall( opOpenFileInstance );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p[%s],0x%p,0x%p,0x%p)",
            __FUNCTION__,
            instance,
            wszFileName,
            SzOSFormatStringW( wszFileName ),
            phfFile,
            pulFileSizeLow,
            pulFileSizeHigh ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamOpenFile(
                                        (JET_INSTANCE)apicall.Pinst(),
                                        wszFileName,
                                        phfFile,
                                        ibRead,
                                        pulFileSizeLow,
                                        pulFileSizeHigh ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetOpenFileInstanceExA(
    __in JET_INSTANCE       instance,
    __in JET_PCSTR          szFileName,
    __out JET_HANDLE *      phfFile,
    __in ULONG64            ibRead,
    __out ULONG *   pulFileSizeLow,
    __out ULONG *   pulFileSizeHigh )
{
    ERR             err         = JET_errSuccess;
    CAutoWSZPATH    lwszFileName;

    CallR( lwszFileName.ErrSet( szFileName ) );

    return JetOpenFileInstanceEx( instance, lwszFileName, phfFile, ibRead, pulFileSizeLow, pulFileSizeHigh );
}

JET_ERR JET_API JetOpenFileInstanceA(
    __in JET_INSTANCE       instance,
    __in JET_PCSTR          szFileName,
    __out JET_HANDLE *      phfFile,
    __out ULONG *   pulFileSizeLow,
    __out ULONG *   pulFileSizeHigh )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opOpenFileInstance, JetOpenFileInstanceExA( instance, szFileName, phfFile, 0, pulFileSizeLow, pulFileSizeHigh ) );
}

JET_ERR JET_API JetOpenFileInstanceW(
    __in JET_INSTANCE       instance,
    __in JET_PCWSTR         wszFileName,
    __out JET_HANDLE *      phfFile,
    __out ULONG *   pulFileSizeLow,
    __out ULONG *   pulFileSizeHigh )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opOpenFileInstance, JetOpenFileInstanceEx( instance, wszFileName, phfFile, 0, pulFileSizeLow, pulFileSizeHigh ) );
}

LOCAL JET_ERR JET_API JetOpenFileSectionInstanceExA(
    __in JET_INSTANCE       instance,
    __in JET_PCSTR          szFileName,
    __out JET_HANDLE *      phfFile,
    __in ULONG64            ibRead,
    __out ULONG *   pulFileSizeLow,
    __out ULONG *   pulFileSizeHigh )
{
    ERR             err = JET_errSuccess;
    CAutoWSZPATH    lwszFileName;

    CallR( lwszFileName.ErrSet( szFileName ) );

    return JetOpenFileInstanceEx( instance, lwszFileName, phfFile, ibRead, pulFileSizeLow, pulFileSizeHigh );
}

JET_ERR JET_API JetOpenFileSectionInstanceA(
    __in JET_INSTANCE       instance,
    __in JET_PSTR           szFileName,
    __out JET_HANDLE *      phFile,
    __in LONG               iSection,
    __in LONG               cSections,
    __in ULONG64            ibRead,
    __out ULONG *   pulSectionSizeLow,
    __out LONG *            plSectionSizeHigh )
{
    if ( cSections > 1 )
    {
        return ErrERRCheck( JET_wrnNyi );
    }

    JET_VALIDATE_INSTANCE( instance );

    JET_TRY( opFileSectionInstance, JetOpenFileSectionInstanceExA( instance, szFileName, phFile, ibRead, pulSectionSizeLow, (ULONG *)plSectionSizeHigh ) );
};

JET_ERR JET_API JetOpenFileSectionInstanceW(
    __in JET_INSTANCE       instance,
    __in JET_PWSTR          wszFileName,
    __out JET_HANDLE *      phFile,
    __in LONG               iSection,
    __in LONG               cSections,
    __in ULONG64            ibRead,
    __out ULONG *   pulSectionSizeLow,
    __out LONG *            plSectionSizeHigh )
{

    if ( cSections > 1 )
    {
        return ErrERRCheck( JET_wrnNyi );
    }

    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opFileSectionInstance, JetOpenFileInstanceEx( instance, wszFileName, phFile, ibRead, pulSectionSizeLow, (ULONG *)plSectionSizeHigh ) );
};


JET_ERR JET_API JetOpenFileA(
    __in JET_PCSTR          szFileName,
    __out JET_HANDLE *      phfFile,
    __out ULONG *   pulFileSizeLow,
    __out ULONG *   pulFileSizeHigh )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetOpenFileInstanceA( (JET_INSTANCE)g_rgpinst[0], szFileName, phfFile, pulFileSizeLow, pulFileSizeHigh );
}

JET_ERR JET_API JetOpenFileW(
    __in JET_PCWSTR         wszFileName,
    __out JET_HANDLE *      phfFile,
    __out ULONG *   pulFileSizeLow,
    __out ULONG *   pulFileSizeHigh )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetOpenFileInstanceW( (JET_INSTANCE)g_rgpinst[0], wszFileName, phfFile, pulFileSizeLow, pulFileSizeHigh );
}


LOCAL JET_ERR JetReadFileInstanceEx(
    __in JET_INSTANCE                           instance,
    __in JET_HANDLE                             hfFile,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    __in ULONG                          cb,
    __out_opt ULONG *                   pcbActual )
{
    APICALL_INST    apicall( opReadFileInstance );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%p,0x%x,0x%p)",
            __FUNCTION__,
            instance,
            hfFile,
            pv,
            cb,
            pcbActual ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamReadFile(
                                        (JET_INSTANCE)apicall.Pinst(),
                                        hfFile,
                                        pv,
                                        cb,
                                        pcbActual ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetReadFileInstance(
    __in JET_INSTANCE                           instance,
    __in JET_HANDLE                             hfFile,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    __in ULONG                          cb,
    __out_opt ULONG *                   pcbActual )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opReadFileInstance, JetReadFileInstanceEx( instance, hfFile, pv, cb, pcbActual ) );
}
JET_ERR JET_API JetReadFile(
    __in JET_HANDLE                             hfFile,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    __in ULONG                          cb,
    __out_opt ULONG *                   pcbActual )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetReadFileInstance( (JET_INSTANCE)g_rgpinst[0], hfFile, pv, cb, pcbActual );
}

LOCAL JET_ERR JetCloseFileInstanceEx( __in JET_INSTANCE instance, __in JET_HANDLE hfFile )
{
    APICALL_INST    apicall( opCloseFileInstance );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix)",
            __FUNCTION__,
            instance,
            hfFile ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamCloseFile(
                                        (JET_INSTANCE)apicall.Pinst(),
                                        hfFile ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetCloseFileInstance( __in JET_INSTANCE instance, __in JET_HANDLE hfFile )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opCloseFileInstance, JetCloseFileInstanceEx( instance, hfFile ) );
}
JET_ERR JET_API JetCloseFile( __in JET_HANDLE hfFile )
{
    ERR     err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetCloseFileInstance( (JET_INSTANCE)g_rgpinst[0], hfFile );
}


class CAutoLOGINFOW
{
    public:
        CAutoLOGINFOW():m_pLogInfo( NULL ) { }
        ~CAutoLOGINFOW();

    public:
        ERR ErrReset( );
        ERR ErrGet( JET_LOGINFO_A * pLogInfo );
        operator JET_LOGINFO_W*()       { return m_pLogInfo; }

    private:
        JET_LOGINFO_W *     m_pLogInfo;
};

ERR CAutoLOGINFOW::ErrReset( )
{
    ERR                 err         = JET_errSuccess;

    delete m_pLogInfo;
    m_pLogInfo = NULL;

    Alloc( m_pLogInfo = new JET_LOGINFO_W );
    memset( m_pLogInfo, 0, sizeof(JET_LOGINFO_W) );
    m_pLogInfo->cbSize = sizeof( JET_LOGINFO_W );

HandleError:
    if ( err < JET_errSuccess )
    {
        delete m_pLogInfo;
        m_pLogInfo = NULL;
    }

    return err;
}

ERR CAutoLOGINFOW::ErrGet( JET_LOGINFO_A * pLogInfo )
{
    ERR                 err         = JET_errSuccess;

    Assert( m_pLogInfo );

    if ( !pLogInfo )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Assert( pLogInfo->cbSize == sizeof( JET_LOGINFO_A ) );

    CallR( ErrOSSTRUnicodeToAscii( m_pLogInfo->szBaseName, pLogInfo->szBaseName, _countof(pLogInfo->szBaseName), NULL ) );

    pLogInfo->ulGenLow = m_pLogInfo->ulGenLow;
    pLogInfo->ulGenHigh = m_pLogInfo->ulGenHigh;

HandleError:
    return err;
}

CAutoLOGINFOW::~CAutoLOGINFOW()
{
    delete m_pLogInfo;
}

LOCAL JET_ERR JetGetLogInfoInstanceEx(
    __in JET_INSTANCE                                       instance,
    __out_bcount_part_opt( cbMax, *pcbActual ) JET_PWSTR    wszzLogs,
    __in ULONG                                      cbMax,
    __out_opt ULONG *                               pcbActual,
    __inout_opt JET_LOGINFO_W *                             pLogInfo )
{
    APICALL_INST    apicall( opGetLogInfoInstance );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,%ls,0x%x,0x%p,0x%p)",
            __FUNCTION__,
            instance,
            wszzLogs,
            cbMax,
            pcbActual,
            pLogInfo ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamGetLogInfo(
                                        (JET_INSTANCE)apicall.Pinst(),
                                        wszzLogs,
                                        cbMax,
                                        pcbActual,
                                        pLogInfo ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetGetLogInfoInstanceExA(
    __in JET_INSTANCE                                   instance,
    __out_bcount_part_opt( cbMax, *pcbActual ) JET_PSTR szzLogs,
    __in ULONG                                  cbMax,
    __out_opt ULONG *                           pcbActual,
    __inout_opt JET_LOGINFO_A *                         pLogInfo )
{
    ERR             err         = JET_errSuccess;
    CAutoWSZ        lwsz;
    CAutoLOGINFOW   lLogInfoW;
    ULONG   cbActual;
    size_t      cchActual;

    if ( pLogInfo )
    {
        if ( sizeof(JET_LOGINFO_A) != pLogInfo->cbSize )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        Call( lLogInfoW.ErrReset() );
    }

    Call( JetGetLogInfoInstanceEx( instance, 0, NULL, &cbActual, (JET_LOGINFO_W*)lLogInfoW ) );

    Call( lwsz.ErrAlloc( cbActual ) );

    Call( JetGetLogInfoInstanceEx( instance, lwsz.Pv(), lwsz.Cb(), &cbActual, NULL ) );

    Call( ErrOSSTRUnicodeToAsciiM( lwsz.Pv(), szzLogs, cbMax, &cchActual ) );

    if ( pLogInfo )
    {
        Call( lLogInfoW.ErrGet( pLogInfo ) );
    }

    if ( pcbActual )
    {
        *pcbActual = cchActual * sizeof( CHAR );
    }
HandleError:
    return err;
}

JET_ERR JET_API JetGetLogInfoInstanceA(
    _In_ JET_INSTANCE                                   instance,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PSTR szzLogs,
    _In_ ULONG                                  cbMax,
    _Out_opt_ ULONG *                           pcbActual )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opGetLogInfoInstance, JetGetLogInfoInstanceExA( instance, szzLogs, cbMax, pcbActual, NULL ) );
}

JET_ERR JET_API JetGetLogInfoInstanceW(
    _In_ JET_INSTANCE                                       instance,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    wszzLogs,
    _In_ ULONG                                      cbMax,
    _Out_opt_ ULONG *                               pcbActual )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opGetLogInfoInstance, JetGetLogInfoInstanceEx( instance, wszzLogs, cbMax, pcbActual, NULL ) );
}


JET_ERR JET_API JetGetLogInfoInstance2A(
    _In_ JET_INSTANCE                                   instance,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PSTR szzLogs,
    _In_ ULONG                                  cbMax,
    _Out_opt_ ULONG *                           pcbActual,
    _Inout_opt_ JET_LOGINFO_A *                         pLogInfo )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opGetLogInfoInstance, JetGetLogInfoInstanceExA( instance, szzLogs, cbMax, pcbActual, pLogInfo ) );
}

JET_ERR JET_API JetGetLogInfoInstance2W(
    _In_ JET_INSTANCE                                       instance,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    wszzLogs,
    _In_ ULONG                                      cbMax,
    _Out_opt_ ULONG *                               pcbActual,
    _Inout_opt_ JET_LOGINFO_W *                             pLogInfo )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opGetLogInfoInstance, JetGetLogInfoInstanceEx( instance, wszzLogs, cbMax, pcbActual, pLogInfo ) );
}

JET_ERR JET_API JetGetLogInfoA(
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PSTR szzLogs,
    _In_ ULONG                                  cbMax,
    _Out_opt_ ULONG *                           pcbActual )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetGetLogInfoInstanceA( (JET_INSTANCE)g_rgpinst[0], szzLogs, cbMax, pcbActual );
}

JET_ERR JET_API JetGetLogInfoW(
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    wszzLogs,
    _In_ ULONG                                      cbMax,
    _Out_opt_ ULONG *                               pcbActual )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetGetLogInfoInstanceW( (JET_INSTANCE)g_rgpinst[0], wszzLogs, cbMax, pcbActual );
}

LOCAL JET_ERR JetGetTruncateLogInfoInstanceEx(
    __in JET_INSTANCE                                       instance,
    __out_bcount_part_opt( cbMax, *pcbActual ) JET_PWSTR    wszzLogs,
    __in ULONG                                      cbMax,
    __out_opt ULONG *                               pcbActual )
{
    APICALL_INST    apicall( opGetTruncateLogInfoInstance );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,%ls,0x%x,0x%p)",
            __FUNCTION__,
            instance,
            wszzLogs,
            cbMax,
            pcbActual ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamGetTruncateLogInfo(
                                        (JET_INSTANCE)apicall.Pinst(),
                                        wszzLogs,
                                        cbMax,
                                        pcbActual ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetGetTruncateLogInfoInstanceExA(
    __in JET_INSTANCE                                   instance,
    __out_bcount_part_opt( cbMax, *pcbActual ) JET_PSTR szzLogs,
    __in ULONG                                  cbMax,
    __out_opt ULONG *                           pcbActual )
{
    ERR             err         = JET_errSuccess;
    CAutoWSZ        lwszz;
    ULONG   cbActual;
    size_t      cchActual;

    Call( JetGetTruncateLogInfoInstanceEx( instance, 0, NULL, &cbActual ) );

    Call( lwszz.ErrAlloc( cbActual ) );

    Call( JetGetTruncateLogInfoInstanceEx( instance, lwszz.Pv(), lwszz.Cb(), &cbActual ) );

    Call( ErrOSSTRUnicodeToAsciiM( lwszz.Pv(), szzLogs, cbMax, &cchActual ) );

    if ( pcbActual )
    {
        *pcbActual = cchActual;
    }

HandleError:
    return err;
}

JET_ERR JET_API JetGetTruncateLogInfoInstanceA(
    _In_ JET_INSTANCE                                   instance,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PSTR szzLogs,
    _In_ ULONG                                  cbMax,
    _Out_opt_ ULONG *                           pcbActual )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opGetTruncateLogInfoInstance, JetGetTruncateLogInfoInstanceExA( instance, szzLogs, cbMax, pcbActual ) );
}

JET_ERR JET_API JetGetTruncateLogInfoInstanceW(
    _In_ JET_INSTANCE                                       instance,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    wszzLogs,
    _In_ ULONG                                      cbMax,
    _Out_opt_ ULONG *                               pcbActual )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opGetTruncateLogInfoInstance, JetGetTruncateLogInfoInstanceEx( instance, wszzLogs, cbMax, pcbActual ) );
}

LOCAL JET_ERR JetTruncateLogInstanceEx( __in JET_INSTANCE instance )
{
    APICALL_INST    apicall( opTruncateLogInstance );

    OSTrace( JET_tracetagAPI, OSFormat( "Start %s(0x%Ix)", __FUNCTION__, instance ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamTruncateLog( (JET_INSTANCE)apicall.Pinst() ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetTruncateLogInstance( __in JET_INSTANCE instance )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opTruncateLogInstance, JetTruncateLogInstanceEx( instance ) );
}
JET_ERR JET_API JetTruncateLog( void )
{
    ERR     err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetTruncateLogInstance( (JET_INSTANCE)g_rgpinst[0] );
}


LOCAL JET_ERR JetEndExternalBackupInstanceEx( __in JET_INSTANCE instance, __in JET_GRBIT grbit )
{
    APICALL_INST    apicall( opEndExternalBackupInstance );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x)",
            __FUNCTION__,
            instance,
            grbit ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamEndExternalBackup(
                                        (JET_INSTANCE)apicall.Pinst(),
                                        grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetEndExternalBackupInstance2( __in JET_INSTANCE instance, __in JET_GRBIT grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opEndExternalBackupInstance, JetEndExternalBackupInstanceEx( instance, grbit ) );
}


JET_ERR JET_API JetEndExternalBackupInstance( __in JET_INSTANCE instance )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opEndExternalBackupInstance, JetEndExternalBackupInstanceEx( instance, JET_bitBackupEndNormal  ) );
}
JET_ERR JET_API JetEndExternalBackup( void )
{
    ERR     err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetEndExternalBackupInstance( (JET_INSTANCE)g_rgpinst[0] );
}

LOCAL JET_ERR JetBeginSurrogateBackupEx(
    __in    JET_INSTANCE    instance,
    __in    ULONG   lgenFirst,
    __in    ULONG   lgenLast,
    __in    JET_GRBIT   grbit )
{
    APICALL_INST    apicall( opBeginSurrogateBackup );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,[0x%x-0x%x],0x%x)",
            __FUNCTION__,
            instance,
            lgenFirst, lgenLast,
            grbit ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( apicall.Pinst()->m_fBackupAllowed ?
                                        ErrIsamBeginSurrogateBackup( (JET_INSTANCE)apicall.Pinst(),
                                                    lgenFirst, lgenLast, grbit ) :
                                        ErrERRCheck( JET_errBackupNotAllowedYet ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetBeginSurrogateBackup(
    __in        JET_INSTANCE    instance,
    __in        ULONG       lgenFirst,
    __in        ULONG       lgenLast,
    __in        JET_GRBIT       grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBeginSurrogateBackup, JetBeginSurrogateBackupEx( instance, lgenFirst, lgenLast, grbit ) );
}

LOCAL JET_ERR JetEndSurrogateBackupEx(
    __in    JET_INSTANCE    instance,
    __in        JET_GRBIT       grbit )
{
    APICALL_INST    apicall( opEndSurrogateBackup );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x)",
            __FUNCTION__,
            instance,
            grbit ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamEndSurrogateBackup(
                                        (JET_INSTANCE)apicall.Pinst(),
                                        grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetEndSurrogateBackup(
    __in    JET_INSTANCE    instance,
    __in        JET_GRBIT       grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opEndSurrogateBackup, JetEndSurrogateBackupEx( instance, grbit ) );
}


volatile DWORD g_cRestoreInstance = 0;


ERR ErrINSTPrepareTargetInstance(
    INST * pinstTarget,
    __out_bcount(cbTargetInstanceLogPath) PWSTR wszTargetInstanceLogPath,
    __in_range(sizeof(WCHAR), cbOSFSAPI_MAX_PATHW) ULONG    cbTargetInstanceLogPath,
    __in PCWSTR wszRestoreLogBaseName,
    LONG * plGenHighTargetInstance  )
{
    ERR             err = JET_errSuccess;

    DATA            rgdata[3];
    LREXTRESTORE2   lrextrestore;
    LGPOS           lgposLogRec;
    WCHAR *         wszTargetInstanceName;

    Assert ( wszTargetInstanceLogPath );
    Assert ( plGenHighTargetInstance );

    *plGenHighTargetInstance = 0;
    wszTargetInstanceLogPath[0] = L'\0';



    CallR ( pinstTarget->ErrAPIEnter( fFalse ) );

    Assert ( pinstTarget );
    Assert ( pinstTarget->m_fJetInitialized );
    Assert ( pinstTarget->m_wszInstanceName );


    WCHAR wszDefaultBaseName[ 16 ];
    Call( ErrGetSystemParameter( NULL, JET_sesidNil, JET_paramBaseName, NULL, wszDefaultBaseName, sizeof( wszDefaultBaseName ) ) );
    const WCHAR * wszCurrentLogBaseName = wszRestoreLogBaseName?wszRestoreLogBaseName:wszDefaultBaseName;
    if ( 0 != _wcsnicmp( SzParam( pinstTarget, JET_paramBaseName ), wszCurrentLogBaseName, JET_BASE_NAME_LENGTH) )
    {
        Error( ErrERRCheck( JET_errBadRestoreTargetInstance ) );
    }

    if ( BoolParam( pinstTarget, JET_paramCircularLog ) )
    {
        Assert ( 0 == *plGenHighTargetInstance );
        Assert ( '\0' == wszTargetInstanceLogPath[0] );

        Assert ( JET_errSuccess == err );
        goto HandleError;
    }

    wszTargetInstanceName = pinstTarget->m_wszInstanceName?pinstTarget->m_wszInstanceName:L"";

    lrextrestore.lrtyp = lrtypExtRestore2;

    rgdata[0].SetPv( (BYTE *)&lrextrestore );
    rgdata[0].SetCb( sizeof(LREXTRESTORE) );

    rgdata[1].SetPv( (BYTE *) SzParam( pinstTarget, JET_paramLogFilePath ) );
    rgdata[1].SetCb( sizeof(WCHAR) * ( LOSStrLengthW( SzParam( pinstTarget, JET_paramLogFilePath ) ) + 1 ) );

    rgdata[2].SetPv( (BYTE *) wszTargetInstanceName );
    rgdata[2].SetCb( sizeof(WCHAR) * ( LOSStrLengthW( wszTargetInstanceName ) + 1 ) );

    lrextrestore.SetCbInfo( USHORT( sizeof(WCHAR) * ( LOSStrLengthW( SzParam( pinstTarget, JET_paramLogFilePath ) ) + 1 + LOSStrLengthW( wszTargetInstanceName ) + 1 ) ) );

    Call( pinstTarget->m_plog->ErrLGLogRec( rgdata, 3, fLGCreateNewGen, 0, &lgposLogRec ) );

    Call( pinstTarget->m_plog->ErrLGWaitForLogGen( lgposLogRec.lGeneration ) );
    Assert( lgposLogRec.lGeneration <= pinstTarget->m_plog->LGGetCurrentFileGenNoLock() );

    Assert ( lgposLogRec.lGeneration > 1 );
    *plGenHighTargetInstance = lgposLogRec.lGeneration - 1;
    OSStrCbCopyW( wszTargetInstanceLogPath, cbTargetInstanceLogPath, SzParam( pinstTarget, JET_paramLogFilePath ) );

    CallS( err );

HandleError:
{
    const LONG  lOld    = AtomicExchangeAdd( &(pinstTarget->m_cSessionInJetAPI), -1 );
    Assert( lOld >= 1 );
}

    return err;
}

CCriticalSection g_critRestoreInst( CLockBasicInfo( CSyncBasicInfo( szRestoreInstance ), rankRestoreInstance, 0 ) );

class CAutoRSTMAPW
{
    public:
        CAutoRSTMAPW():m_rgrstmap( NULL ), m_rgwszDatabaseName( NULL ), m_rgwszNewDatabaseName( NULL ), m_crstmap( 0 ) { }
        ~CAutoRSTMAPW();

    public:
        ERR ErrSet( const JET_RSTMAP_A* prstmap, const LONG crstmap );
        operator JET_RSTMAP_W*()    { return m_rgrstmap; }
        LONG Count()                    { return m_crstmap; }

    private:
        JET_RSTMAP_W *  m_rgrstmap;
        CAutoWSZ*           m_rgwszDatabaseName;
        CAutoWSZ*           m_rgwszNewDatabaseName;
        LONG            m_crstmap;
};

ERR CAutoRSTMAPW::ErrSet( const JET_RSTMAP_A * prstmap, const LONG crstmap )
{
    ERR                 err         = JET_errSuccess;

    delete[] m_rgrstmap;
    delete[] m_rgwszDatabaseName;
    delete[] m_rgwszNewDatabaseName;
    m_rgrstmap = NULL;
    m_rgwszDatabaseName = NULL;
    m_rgwszNewDatabaseName = NULL;
    m_crstmap = 0;

    C_ASSERT( sizeof(JET_RSTMAP_W) == sizeof(JET_RSTMAP_A) );

    if ( ( prstmap == NULL && crstmap != 0 ) ||
        ( prstmap != NULL && crstmap == 0 ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( crstmap )
    {
        AllocR( m_rgrstmap = new JET_RSTMAP_W[crstmap] );
        memset( m_rgrstmap, 0, crstmap * sizeof(JET_RSTMAP_W) );
        AllocR( m_rgwszDatabaseName = new CAutoWSZ[crstmap] );
        AllocR( m_rgwszNewDatabaseName = new CAutoWSZ[crstmap] );
        m_crstmap = crstmap;

        for( INT i = 0 ; i < crstmap; i++ )
        {
            Call( m_rgwszDatabaseName[i].ErrSet( prstmap[i].szDatabaseName ) );
            Call( m_rgwszNewDatabaseName[i].ErrSet( prstmap[i].szNewDatabaseName ) );
            m_rgrstmap[i].szDatabaseName = (WCHAR*)m_rgwszDatabaseName[i];
            m_rgrstmap[i].szNewDatabaseName = (WCHAR*)m_rgwszNewDatabaseName[i];
        }
    }

HandleError:
    return err;
}

CAutoRSTMAPW::~CAutoRSTMAPW()
{
    delete[] m_rgrstmap;
    delete[] m_rgwszDatabaseName;
    delete[] m_rgwszNewDatabaseName;
}

LOCAL JET_ERR JetExternalRestoreEx(
    __in JET_PWSTR                                  wszCheckpointFilePath,
    __in JET_PWSTR                                  wszLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_W *   rgrstmap,
    __in LONG                                       crstfilemap,
    __in JET_PWSTR                                  wszBackupLogPath,
    __in LONG                                       genLow,
    __in LONG                                       genHigh,
    __in JET_PWSTR                                  wszLogBaseName,
    __in_opt JET_PCWSTR                             wszTargetInstanceName,
    __in_opt JET_PCWSTR                             wszTargetInstanceCheckpointPath,
    __in_opt JET_PCWSTR                             wszTargetInstanceLogPath,
    __in JET_PFNSTATUS                              pfn )
{
    APICALL_INST    apicall( opInit );
    INST *          pinst = NULL;
    INT             ipinst = g_cpinstMax;
    ERR             err = JET_errSuccess;

    const WCHAR *           wszRestoreSystemPath = NULL;
    const WCHAR *           wszRestoreLogPath = NULL;

    INST *          pinstTarget = NULL;
    LONG            lGenHighTarget;
    WCHAR           wszTargetLogPath[IFileSystemAPI::cchPathMax];

    const BOOL      fTargetName = (NULL != wszTargetInstanceName);
    const BOOL      fTargetDirs = (NULL != wszTargetInstanceLogPath);

    WCHAR           wszTempDatabase[IFileSystemAPI::cchPathMax];

    WCHAR * wszNewInstanceName = NULL;
    BOOL fInCritInst = fFalse;
    BOOL fInCritRestoreInst = fFalse;

    CCriticalSection *pcritInst = NULL;

    WCHAR * wszTargetDisplayName = NULL;
    SIZE_T cchTargetDisplayName = 0;
    const WCHAR * wszRestoreInstanceNameUsed = wszRestoreInstanceName;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p[%s],0x%p[%s],0x%p,0x%x,0x%p[%s],0x%x,0x%x,0x%p[%s],0x%p[%s],0x%p[%s],0x%p[%s],0x%p)",
            __FUNCTION__,
            wszCheckpointFilePath,
            SzOSFormatStringW( wszCheckpointFilePath ),
            wszLogPath,
            SzOSFormatStringW( wszLogPath ),
            rgrstmap,
            crstfilemap,
            wszBackupLogPath,
            SzOSFormatStringW( wszBackupLogPath ),
            genLow,
            genHigh,
            wszLogBaseName,
            SzOSFormatStringW( wszLogBaseName ),
            wszTargetInstanceName,
            SzOSFormatStringW( wszTargetInstanceName ),
            wszTargetInstanceCheckpointPath,
            SzOSFormatStringW( wszTargetInstanceCheckpointPath ),
            wszTargetInstanceLogPath,
            SzOSFormatStringW( wszTargetInstanceLogPath ),
            pfn ) );

    lGenHighTarget = 0;
    wszTargetLogPath[0] = L'\0';

    if ( (wszTargetInstanceLogPath && !wszTargetInstanceCheckpointPath ) ||
        ( !wszTargetInstanceLogPath && wszTargetInstanceCheckpointPath ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( wszTargetInstanceName && wszTargetInstanceLogPath )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }


    g_critRestoreInst.Enter();
    fInCritRestoreInst = fTrue;

    INST::EnterCritInst();
    fInCritInst = fTrue;

    if ( runInstModeOneInst == RUNINSTGetMode() )
    {
        Error( ErrERRCheck( JET_errRunningInOneInstanceMode ) );
    }
    else if ( runInstModeNoSet == RUNINSTGetMode() )
    {
        RUNINSTSetModeMultiInst();
    }

    Assert ( runInstModeMultiInst == RUNINSTGetMode() );



    Assert ( !fTargetDirs || !fTargetName );

    if ( !fTargetDirs && !fTargetName )
    {
        wszRestoreSystemPath = wszCheckpointFilePath;
        wszRestoreLogPath = wszLogPath;
    }
    else
    {
        Assert ( fTargetDirs ^ fTargetName );

        if ( fTargetName )
        {
            pinstTarget = INST::GetInstanceByName( wszTargetInstanceName );
        }
        else
        {
            pinstTarget = INST::GetInstanceByFullLogPath( wszTargetInstanceLogPath );
        }

        Assert( pinstTarget == NULL || g_rgpinst != NULL );

        if ( pinstTarget )
        {
            wszRestoreSystemPath = wszCheckpointFilePath;
            wszRestoreLogPath = wszLogPath;

            Assert ( pinstTarget->m_wszInstanceName );
        }
        else
        {
            if ( fTargetName )
            {
                Assert ( !fTargetDirs );
                Error( ErrERRCheck( JET_errBadRestoreTargetInstance ) );
            }

            Assert ( fTargetDirs );
            wszRestoreSystemPath = wszTargetInstanceCheckpointPath;
            wszRestoreLogPath = wszTargetInstanceLogPath;
        }
    }

    Assert ( RUNINSTGetMode() == runInstModeMultiInst );

    if ( pinstTarget )
    {
        Call ( ErrINSTPrepareTargetInstance( pinstTarget, wszTargetLogPath, sizeof(wszTargetLogPath), wszLogBaseName, &lGenHighTarget ) );

        wszTargetDisplayName = pinstTarget->m_wszDisplayName ? pinstTarget->m_wszDisplayName : pinstTarget->m_wszInstanceName;

        if ( wszTargetDisplayName )
        {
            cchTargetDisplayName = ( wcslen(wszTargetDisplayName) + 1 );
            wszRestoreInstanceNameUsed = wszRestoreInstanceNamePrefix;
        }
        else
        {
            Assert( 0 == cchTargetDisplayName );
            Assert( wszRestoreInstanceNameUsed == wszRestoreInstanceName );
        }
    }

    ULONG cchNewInstanceName = cchTargetDisplayName + wcslen(wszRestoreInstanceNameUsed) + 4 + 1;
    Alloc( wszNewInstanceName = new WCHAR[ cchNewInstanceName ] );
    OSStrCbFormatW( wszNewInstanceName, cchNewInstanceName*sizeof(WCHAR), L"%-*s%s%04lu", (ULONG)cchTargetDisplayName, wszTargetDisplayName ? wszTargetDisplayName : L"",
            wszRestoreInstanceNameUsed, AtomicIncrement( (LONG*)&g_cRestoreInstance ) % 10000L );

    Call ( ErrNewInst( &pinst, wszNewInstanceName, NULL, &ipinst ) );
    Assert( !pinst->m_fJetInitialized );

    INST::LeaveCritInst();
    fInCritInst = fFalse;

    if ( NULL != wszLogBaseName)
    {
        Call ( ErrSetSystemParameter( pinst, JET_sesidNil, JET_paramBaseName, 0, wszLogBaseName ) );
    }


    if ( wszRestoreLogPath )
    {
        Call ( ErrSetSystemParameter( pinst, JET_sesidNil, JET_paramLogFilePath, 0, wszRestoreLogPath ) );
    }

    if ( wszRestoreSystemPath )
    {
        Call ( ErrSetSystemParameter( pinst, JET_sesidNil, JET_paramSystemPath, 0, wszRestoreSystemPath ) );

        OSStrCbCopyW( wszTempDatabase, sizeof(wszTempDatabase), wszRestoreSystemPath );
        if ( !FOSSTRTrailingPathDelimiterW(wszTempDatabase) )
        {
            OSStrCbAppendW( wszTempDatabase, sizeof(wszTempDatabase), wszPathDelimiter );
        }
        Call ( ErrSetSystemParameter( pinst, JET_sesidNil, JET_paramTempPath, 0, wszTempDatabase ) );
    }


    pcritInst = &g_critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
    pcritInst->Enter();

    if ( !apicall.FEnterForInit( pinst ) )
    {
        pcritInst->Leave();
        err = apicall.ErrResult();
        goto HandleError;
    }

    pinst->APILock( pinst->fAPIRestoring );
    pcritInst->Leave();

    Assert( !pinst->m_fJetInitialized );
    Assert( !pinst->m_fTermInProgress );

    err = ErrInit(  pinst,
                    fTrue,
                    NO_GRBIT );
    Assert( JET_errAlreadyInitialized != err );

    Assert( !pinst->m_fJetInitialized );
    Assert( !pinst->m_fTermInProgress );

    if ( err >= 0 )
    {
        pinst->m_fJetInitialized = fTrue;

        g_critRestoreInst.Leave();
        fInCritRestoreInst = fFalse;

        InitCallbackWrapper initCallbackWrapper(pfn);
        err = ErrIsamExternalRestore(
                    (JET_INSTANCE) pinst,
                    wszRestoreSystemPath,
                    wszRestoreLogPath,
                    rgrstmap,
                    crstfilemap,
                    wszBackupLogPath,
                    genLow,
                    genHigh,
                    wszTargetLogPath,
                    lGenHighTarget,
                    InitCallbackWrapper::PfnWrapper,
                    &initCallbackWrapper );



    }
    else
    {
        g_critRestoreInst.Leave();
        fInCritRestoreInst = fFalse;
    }

    pinst->m_fJetInitialized = fFalse;

    pinst->APIUnlock( pinst->fAPIRestoring );


    Assert ( !fInCritInst );

    apicall.LeaveAfterCall( err );
    err = apicall.ErrResult();

    FreePinst( pinst );

    Assert( NULL != wszNewInstanceName );
    delete[] wszNewInstanceName;

    return err;

HandleError:
    if ( pinst )
    {
        FreePinst( pinst );
    }

    if ( fInCritInst )
    {
        INST::LeaveCritInst();
    }

    if ( fInCritRestoreInst )
    {
        g_critRestoreInst.Leave();
        fInCritRestoreInst = fFalse;
    }

    if ( wszNewInstanceName )
    {
        delete[] wszNewInstanceName;
    }

    return err;
}

LOCAL JET_ERR JetExternalRestoreExA(
    __in JET_PSTR                                   szCheckpointFilePath,
    __in JET_PSTR                                   szLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP *     rgrstmap,
    __in LONG                                       crstfilemap,
    __in JET_PSTR                                   szBackupLogPath,
    __in LONG                                       genLow,
    __in LONG                                       genHigh,
    __in JET_PSTR                                   szLogBaseName,
    __in_opt JET_PCSTR                              szTargetInstanceName,
    __in_opt JET_PCSTR                              szTargetInstanceCheckpointPath,
    __in_opt JET_PCSTR                              szTargetInstanceLogPath,
    __in JET_PFNSTATUS                              pfn )
{
    ERR             err         = JET_errSuccess;
    CAutoRSTMAPW    lwrstmap;
    CAutoWSZ        lwszCheckpointFilePath;
    CAutoWSZ        lwszLogPath;
    CAutoWSZ        lwszBackupLogPath;
    CAutoWSZ        lwszLogBaseName;
    CAutoWSZ        lwszTargetInstanceName;
    CAutoWSZ        lwszTargetInstanceCheckpointPath;
    CAutoWSZ        lwszTargetInstanceLogPath;

    CallR( lwrstmap.ErrSet( rgrstmap, crstfilemap ) );
    Assert( lwrstmap.Count() == crstfilemap );

    CallR( lwszCheckpointFilePath.ErrSet( szCheckpointFilePath ) );
    CallR( lwszLogPath.ErrSet( szLogPath ) );
    CallR( lwszBackupLogPath.ErrSet( szBackupLogPath ) );
    CallR( lwszLogBaseName.ErrSet( szLogBaseName ) );
    CallR( lwszTargetInstanceName.ErrSet( szTargetInstanceName ) );
    CallR( lwszTargetInstanceCheckpointPath.ErrSet( szTargetInstanceCheckpointPath ) );
    CallR( lwszTargetInstanceLogPath.ErrSet( szTargetInstanceLogPath ) );

    return JetExternalRestoreEx(
                lwszCheckpointFilePath,
                lwszLogPath,
                lwrstmap,
                lwrstmap.Count(),
                lwszBackupLogPath,
                genLow,
                genHigh,
                lwszLogBaseName,
                lwszTargetInstanceName,
                lwszTargetInstanceCheckpointPath,
                lwszTargetInstanceLogPath,
                pfn);
}

LOCAL JET_ERR JetExternalRestore2ExA(
    __in JET_PSTR                                   szCheckpointFilePath,
    __in JET_PSTR                                   szLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_A *   rgrstmap,
    __in LONG                                       crstfilemap,
    __in JET_PSTR                                   szBackupLogPath,
    __inout JET_LOGINFO_A *                         pLogInfo,
    __in_opt JET_PCSTR                              szTargetInstanceName,
    __in_opt JET_PCSTR                              szTargetInstanceLogPath,
    __in_opt JET_PCSTR                              szTargetInstanceCheckpointPath,
    __in JET_PFNSTATUS                              pfn )
{

    if ( !pLogInfo || pLogInfo->cbSize != sizeof(JET_LOGINFO_A) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    return JetExternalRestoreExA(   szCheckpointFilePath,
                                    szLogPath,
                                    rgrstmap,
                                    crstfilemap,
                                    szBackupLogPath,
                                    pLogInfo->ulGenLow,
                                    pLogInfo->ulGenHigh,
                                    pLogInfo->szBaseName,
                                    szTargetInstanceName,
                                    szTargetInstanceCheckpointPath,
                                    szTargetInstanceLogPath,
                                    pfn );

}

LOCAL JET_ERR JetExternalRestore2Ex(
    __in JET_PWSTR                                  wszCheckpointFilePath,
    __in JET_PWSTR                                  wszLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_W *   rgrstmap,
    __in LONG                                       crstfilemap,
    __in JET_PWSTR                                  wszBackupLogPath,
    __inout JET_LOGINFO_W *                         pLogInfo,
    __in_opt JET_PCWSTR                             wszTargetInstanceName,
    __in_opt JET_PCWSTR                             wszTargetInstanceLogPath,
    __in_opt JET_PCWSTR                             wszTargetInstanceCheckpointPath,
    __in JET_PFNSTATUS                              pfn )
{

    if ( !pLogInfo || pLogInfo->cbSize != sizeof(JET_LOGINFO_W) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    return JetExternalRestoreEx(    wszCheckpointFilePath,
                                    wszLogPath,
                                    rgrstmap,
                                    crstfilemap,
                                    wszBackupLogPath,
                                    pLogInfo->ulGenLow,
                                    pLogInfo->ulGenHigh,
                                    pLogInfo->szBaseName,
                                    wszTargetInstanceName,
                                    wszTargetInstanceCheckpointPath,
                                    wszTargetInstanceLogPath,
                                    pfn );

}


JET_ERR JET_API JetExternalRestoreA(
    __in JET_PSTR                                   szCheckpointFilePath,
    __in JET_PSTR                                   szLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_A *   rgrstmap,
    __in LONG                                       crstfilemap,
    __in JET_PSTR                                   szBackupLogPath,
    __in LONG                                       genLow,
    __in LONG                                       genHigh,
    __in JET_PFNSTATUS                              pfn )
{
    JET_TRY( opInit, JetExternalRestoreExA( szCheckpointFilePath, szLogPath, rgrstmap, crstfilemap, szBackupLogPath, genLow, genHigh, NULL, NULL, NULL, NULL, pfn ) );
}

JET_ERR JET_API JetExternalRestoreW(
    __in JET_PWSTR                                  wszCheckpointFilePath,
    __in JET_PWSTR                                  wszLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_W *   rgrstmap,
    __in LONG                                       crstfilemap,
    __in JET_PWSTR                                  wszBackupLogPath,
    __in LONG                                       genLow,
    __in LONG                                       genHigh,
    __in JET_PFNSTATUS                              pfn )
{
    JET_TRY( opInit, JetExternalRestoreEx( wszCheckpointFilePath, wszLogPath, rgrstmap, crstfilemap, wszBackupLogPath, genLow, genHigh, NULL, NULL, NULL, NULL, pfn ) );
}

JET_ERR JET_API JetExternalRestore2A(
    __in JET_PSTR                                   szCheckpointFilePath,
    __in JET_PSTR                                   szLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_A *   rgrstmap,
    __in LONG                                       crstfilemap,
    __in JET_PSTR                                   szBackupLogPath,
    __inout JET_LOGINFO_A *                         pLogInfo,
    __in_opt JET_PSTR                               szTargetInstanceName,
    __in_opt JET_PSTR                               szTargetInstanceLogPath,
    __in_opt JET_PSTR                               szTargetInstanceCheckpointPath,
    __in JET_PFNSTATUS                              pfn )
{
    JET_TRY( opInit, JetExternalRestore2ExA( szCheckpointFilePath, szLogPath, rgrstmap, crstfilemap, szBackupLogPath, pLogInfo, szTargetInstanceName, szTargetInstanceCheckpointPath, szTargetInstanceLogPath, pfn ) );
}

JET_ERR JET_API JetExternalRestore2W(
    __in JET_PWSTR                                  wszCheckpointFilePath,
    __in JET_PWSTR                                  wszLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_W *   rgrstmap,
    __in LONG                                       crstfilemap,
    __in JET_PWSTR                                  wszBackupLogPath,
    __inout JET_LOGINFO_W *                         pLogInfo,
    __in_opt JET_PWSTR                              wszTargetInstanceName,
    __in_opt JET_PWSTR                              wszTargetInstanceLogPath,
    __in_opt JET_PWSTR                              wszTargetInstanceCheckpointPath,
    __in JET_PFNSTATUS                              pfn )
{
    JET_TRY( opInit, JetExternalRestore2Ex( wszCheckpointFilePath, wszLogPath, rgrstmap, crstfilemap, wszBackupLogPath, pLogInfo, wszTargetInstanceName, wszTargetInstanceCheckpointPath, wszTargetInstanceLogPath, pfn ) );
}

JET_ERR JET_API JetSnapshotStartA(
    __in JET_INSTANCE   instance,
    __in JET_PSTR       szDatabases,
    __in JET_GRBIT      grbit )
{
    return ErrERRCheck( JET_wrnNyi );
}

JET_ERR JET_API JetSnapshotStartW(
    __in JET_INSTANCE   instance,
    __in JET_PWSTR      szDatabases,
    __in JET_GRBIT      grbit )
{
    return ErrERRCheck( JET_wrnNyi );
}

JET_ERR JET_API JetSnapshotStop(
    __in JET_INSTANCE   instance,
    __in JET_GRBIT      grbit)
{
    return ErrERRCheck( JET_wrnNyi );
}


LOCAL JET_ERR JetResetCounterEx( __in JET_SESID sesid, __in LONG CounterType )
{
    APICALL_SESID   apicall( opResetCounter );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x)",
            __FUNCTION__,
            sesid,
            CounterType ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamResetCounter( sesid, CounterType ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetResetCounter( __in JET_SESID sesid, __in LONG CounterType )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opResetCounter, JetResetCounterEx( sesid, CounterType ) );
}


LOCAL JET_ERR JetGetCounterEx( __in JET_SESID sesid, __in LONG CounterType, __out LONG *plValue )
{
    APICALL_SESID   apicall( opGetCounter );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p)",
            __FUNCTION__,
            sesid,
            CounterType,
            plValue ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamGetCounter(
                                        sesid,
                                        CounterType,
                                        plValue ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetGetCounter( __in JET_SESID sesid, __in LONG CounterType, __out LONG *plValue )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetCounter, JetGetCounterEx( sesid, CounterType, plValue ) );
}

LOCAL JET_ERR JetGetThreadStatsEx(
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax )
{
    ProbeClientBuffer( pvResult, cbMax );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p,0x%x)",
            __FUNCTION__,
            pvResult,
            cbMax ) );

    if ( cbMax < sizeof( JET_THREADSTATS ) )
    {
        return ErrERRCheck( JET_errBufferTooSmall );
    }

    TLS* const ptls = Ptls();
    memcpy( pvResult, &ptls->threadstats, min( cbMax, sizeof( ptls->threadstats ) ) );
    ptls->RefreshTickThreadStatsLast();
    ((JET_THREADSTATS*)pvResult)->cbStruct = min( cbMax, sizeof( ptls->threadstats ) );

    return JET_errSuccess;
}

JET_ERR JET_API JetGetThreadStats(
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax )
{
    JET_TRY( opGetThreadStats, JetGetThreadStatsEx( (void*)pvResult, cbMax ) );
}


class CAutoCONVERTW
{
    public:
        CAutoCONVERTW():m_pconvert( NULL ) { }
        ~CAutoCONVERTW();

    public:
        ERR ErrSet( const JET_CONVERT_A * pconvert );
        operator JET_CONVERT_W*()       { return m_pconvert; }

    private:
        JET_CONVERT_W *     m_pconvert;
        CAutoWSZ            m_wszOldDll;
};

ERR CAutoCONVERTW::ErrSet( const JET_CONVERT_A * pconvert )
{
    ERR                 err         = JET_errSuccess;

    C_ASSERT( sizeof(JET_CONVERT_W) == sizeof(JET_CONVERT_A) );

    delete m_pconvert;
    m_pconvert = NULL;

    if ( pconvert )
    {
        Alloc( m_pconvert = new JET_CONVERT_W );
        memset( m_pconvert, 0, sizeof(JET_CONVERT_W) );
        Call( m_wszOldDll.ErrSet( pconvert->szOldDll) );


        m_pconvert->szOldDll = m_wszOldDll;
        m_pconvert->fFlags = pconvert->fFlags;
    }


HandleError:
    if ( err < JET_errSuccess )
    {
        delete m_pconvert;
        m_pconvert = NULL;
    }

    return err;
}

CAutoCONVERTW::~CAutoCONVERTW()
{
    delete m_pconvert;
}

LOCAL JET_ERR JetCompactEx(
    __in JET_SESID              sesid,
    __in JET_PCWSTR             wszDatabaseSrc,
    __in JET_PCWSTR             wszDatabaseDest,
    __in JET_PFNSTATUS          pfnStatus,
    __in_opt JET_CONVERT_W *    pconvert,
    __in JET_GRBIT              grbit )
{
    APICALL_SESID   apicall( opCompact );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p[%s],0x%p[%s],0x%p,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            wszDatabaseSrc,
            SzOSFormatStringW( wszDatabaseSrc ),
            wszDatabaseDest,
            SzOSFormatStringW( wszDatabaseDest ),
            pfnStatus,
            pconvert,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamCompact(
                                        sesid,
                                        wszDatabaseSrc,
                                        PinstFromSesid( sesid )->m_pfsapi,
                                        wszDatabaseDest,
                                        pfnStatus,
                                        pconvert,
                                        grbit ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetCompactExA(
    __in JET_SESID              sesid,
    __in JET_PCSTR              szDatabaseSrc,
    __in JET_PCSTR              szDatabaseDest,
    __in JET_PFNSTATUS          pfnStatus,
    __in_opt JET_CONVERT_A *    pconvert,
    __in JET_GRBIT              grbit )
{
    ERR                 err = JET_errSuccess;
    CAutoCONVERTW       lconvertW;
    CAutoWSZPATH        lwszDatabaseSrc;
    CAutoWSZPATH        lwszDatabaseDest;

    CallR( lconvertW.ErrSet( pconvert ) );
    CallR( lwszDatabaseSrc.ErrSet( szDatabaseSrc ) );
    CallR( lwszDatabaseDest.ErrSet( szDatabaseDest ) );

    return JetCompactEx( sesid, lwszDatabaseSrc, lwszDatabaseDest, pfnStatus, lconvertW, grbit );
}

JET_ERR JET_API JetCompactA(
    __in JET_SESID              sesid,
    __in JET_PCSTR              szDatabaseSrc,
    __in JET_PCSTR              szDatabaseDest,
    __in JET_PFNSTATUS          pfnStatus,
    __in_opt JET_CONVERT_A *    pconvert,
    __in JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCompact, JetCompactExA( sesid, szDatabaseSrc, szDatabaseDest, pfnStatus, pconvert, grbit ) );
}

JET_ERR JET_API JetCompactW(
    __in JET_SESID              sesid,
    __in JET_PCWSTR             wszDatabaseSrc,
    __in JET_PCWSTR             wszDatabaseDest,
    __in JET_PFNSTATUS          pfnStatus,
    __in_opt JET_CONVERT_W *    pconvert,
    __in JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCompact, JetCompactEx( sesid, wszDatabaseSrc, wszDatabaseDest, pfnStatus, pconvert, grbit ) );
}

LOCAL JET_ERR JetConvertDDLEx(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_OPDDLCONV              convtyp,
    __out_bcount( cbData ) void *   pvData,
    __in ULONG              cbData )
{
    ERR             err;
    APICALL_SESID   apicall( opConvertDDL );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%x,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            convtyp,
            pvData,
            cbData ) );

    if ( !apicall.FEnter( sesid, dbid ) )
        return apicall.ErrResult();

    PIB * const ppib = (PIB *)sesid;
    Call( ErrPIBCheck( ppib ) );
    Call( ErrPIBCheckUpdatable( ppib ) );
    Call( ErrPIBCheckIfmp( ppib, dbid ) );

    if( NULL == pvData
        || 0 == cbData )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    switch( convtyp )
    {
        case opDDLConvAddCallback:
        {
            if( sizeof( JET_DDLADDCALLBACK_A ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
            const JET_DDLADDCALLBACK_A * const paddcallback = (JET_DDLADDCALLBACK_A *)pvData;
            const CHAR * const szTable      = paddcallback->szTable;
            const JET_CBTYP cbtyp           = paddcallback->cbtyp;
            const CHAR * const szCallback   = paddcallback->szCallback;
            err = ErrCATAddCallbackToTable( ppib, dbid, szTable, cbtyp, szCallback );
        }
            break;

        case opDDLConvChangeColumn:
        {
            if( sizeof( JET_DDLCHANGECOLUMN_A ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
            const JET_DDLCHANGECOLUMN_A * const pchangecolumn = (JET_DDLCHANGECOLUMN_A *)pvData;
            const CHAR * const szTable  = pchangecolumn->szTable;
            const CHAR * const szColumn = pchangecolumn->szColumn;
            const JET_COLTYP coltyp     = pchangecolumn->coltypNew;
            const JET_GRBIT grbit       = pchangecolumn->grbitNew;
            err = ErrCATConvertColumn( ppib, dbid, szTable, szColumn, coltyp, grbit );
        }
            break;

        case opDDLConvAddConditionalColumnsToAllIndexes:
        {
            if( sizeof( JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_A ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
            const JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_A* const paddcondidx     = (JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_A *)pvData;
            const CHAR * const szTable                                          = paddcondidx->szTable;
            const JET_CONDITIONALCOLUMN_A * const rgconditionalcolumn           = paddcondidx->rgconditionalcolumn;
            const ULONG cConditionalColumn                                      = paddcondidx->cConditionalColumn;
            err = ErrCATAddConditionalColumnsToAllIndexes( ppib, dbid, szTable, rgconditionalcolumn, cConditionalColumn );
        }
            break;

        case opDDLConvAddColumnCallback:
        {
            if( sizeof( JET_DDLADDCOLUMNCALLBACK_A ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
            const JET_DDLADDCOLUMNCALLBACK_A * const paddcolumncallback = (JET_DDLADDCOLUMNCALLBACK_A *)pvData;

            const CHAR * const szTable          = paddcolumncallback->szTable;
            const CHAR * const szColumn         = paddcolumncallback->szColumn;
            const CHAR * const szCallback       = paddcolumncallback->szCallback;
            const VOID * const pvCallbackData   = paddcolumncallback->pvCallbackData;
            const ULONG cbCallbackData  = paddcolumncallback->cbCallbackData;

            err = ErrCATAddColumnCallback( ppib, dbid, szTable, szColumn, szCallback, pvCallbackData, cbCallbackData );
        }
            break;

        case opDDLConvIncreaseMaxColumnSize:
        {
            if ( sizeof( JET_DDLMAXCOLUMNSIZE_A ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            const JET_DDLMAXCOLUMNSIZE_A * const    pmaxcolumnsize  = (JET_DDLMAXCOLUMNSIZE_A *)pvData;

            const CHAR * const  szTable     = pmaxcolumnsize->szTable;
            const CHAR * const  szColumn    = pmaxcolumnsize->szColumn;
            const ULONG         cbMaxLen    = pmaxcolumnsize->cbMax;

            err = ErrCATIncreaseMaxColumnSize( ppib, dbid, szTable, szColumn, cbMaxLen );
            break;
        }

        case opDDLConvChangeIndexDensity:
        {
            if ( sizeof( JET_DDLINDEXDENSITY_A ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            const JET_DDLINDEXDENSITY_A * const pindexdensity   = (JET_DDLINDEXDENSITY_A *)pvData;

            const CHAR * const  szTable     = pindexdensity->szTable;
            const CHAR * const  szIndex     = pindexdensity->szIndex;
            const ULONG         ulDensity   = pindexdensity->ulDensity;

            err = ErrCATChangeIndexDensity( ppib, dbid, szTable, szIndex, ulDensity );
            break;
        }

        case opDDLConvChangeCallbackDLL:
        {
            if ( sizeof( JET_DDLCALLBACKDLL_A ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            const JET_DDLCALLBACKDLL_A * const  pcallbackdll    = (JET_DDLCALLBACKDLL_A *)pvData;

            const CHAR * const  szOldDLL        = pcallbackdll->szOldDLL;
            const CHAR * const  szNewDLL        = pcallbackdll->szNewDLL;

            err = ErrCATChangeCallbackDLL( ppib, dbid, szOldDLL, szNewDLL );
            break;
        }

        case opDDLConvNull:
        case opDDLConvMax:
        default:
            err = ErrERRCheck( JET_errInvalidParameter );
            break;
    }

HandleError:
    apicall.LeaveAfterCall( err );
    return apicall.ErrResult();
}

LOCAL JET_ERR JetConvertDDLExW(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_OPDDLCONV              convtyp,
    __out_bcount( cbData ) void *   pvData,
    __in ULONG              cbData )
{
    ERR             err         = JET_errSuccess;
    void *          lpvData     = NULL;
    ULONG   lcbData     = 0;

    CAutoSZDDL      lszTable;
    CAutoSZDDL      lszCallback;
    CAutoSZDDL      lszColumn;
    CAutoSZDDL      lszIndex;
    CAutoSZ         lszOldDLL;
    CAutoSZ         lszNewDLL;

    JET_DDLADDCALLBACK_A  addcallback;
    JET_DDLCHANGECOLUMN_A changecolumn;
    JET_DDLADDCOLUMNCALLBACK_A addcolumncallback;
    JET_DDLMAXCOLUMNSIZE_A maxcolumnsize;
    JET_DDLINDEXDENSITY_A indexdensity;
    JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_A addcondidx;
    JET_DDLCALLBACKDLL_A callbackdll;

    JET_CONDITIONALCOLUMN_A     * pcondcolumn = NULL;
    CAutoSZDDL                  * pcondcolumnName = NULL;

    switch( convtyp )
    {
        case opDDLConvAddCallback:
            if( sizeof( JET_DDLADDCALLBACK_W ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
            CallR( lszTable.ErrSet( ( (JET_DDLADDCALLBACK_W *)pvData )->szTable ) );
            CallR( lszCallback.ErrSet( ( (JET_DDLADDCALLBACK_W *)pvData )->szCallback ) );

            C_ASSERT( sizeof( JET_DDLADDCALLBACK_A ) == sizeof( JET_DDLADDCALLBACK_W ) );
            memcpy( &addcallback, pvData, sizeof( JET_DDLADDCALLBACK_A ) );
            addcallback.szTable = lszTable;
            addcallback.szCallback = lszCallback;

            lpvData = &addcallback;
            lcbData = sizeof(addcallback);
            break;

        case opDDLConvChangeColumn:
            if( sizeof( JET_DDLCHANGECOLUMN_W ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            CallR( lszTable.ErrSet( ( (JET_DDLCHANGECOLUMN_W *)pvData )->szTable ) );
            CallR( lszColumn.ErrSet( ( (JET_DDLCHANGECOLUMN_W *)pvData )->szColumn ) );

            C_ASSERT( sizeof( JET_DDLCHANGECOLUMN_A ) == sizeof( JET_DDLCHANGECOLUMN_W ) );
            memcpy( &changecolumn, pvData, sizeof( JET_DDLCHANGECOLUMN_W ) );
            changecolumn.szTable = lszTable;
            changecolumn.szColumn= lszColumn;

            lpvData = &changecolumn;
            lcbData = sizeof(changecolumn);
            break;

        case opDDLConvAddConditionalColumnsToAllIndexes:
            if( sizeof( JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_W ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
            C_ASSERT( sizeof( JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_A ) == sizeof( JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_W ) );
            memcpy( &addcondidx, pvData, sizeof( JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_A ) );

            CallR( lszTable.ErrSet( ( (JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_W *)pvData )->szTable ) );
            addcondidx.szTable = lszTable;

            Alloc( pcondcolumn = (JET_CONDITIONALCOLUMN_A*)new JET_CONDITIONALCOLUMN_A[addcondidx.cConditionalColumn] );
            memset( pcondcolumn, 0, addcondidx.cConditionalColumn * sizeof(JET_CONDITIONALCOLUMN_A) );
            addcondidx.rgconditionalcolumn = pcondcolumn;

            Alloc( pcondcolumnName = (CAutoSZDDL*)new CAutoSZDDL[addcondidx.cConditionalColumn] );

            for( ULONG iColumn = 0; iColumn < addcondidx.cConditionalColumn; iColumn++ )
            {
                if ( sizeof( JET_CONDITIONALCOLUMN_W ) != ( (JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_W *)pvData )->rgconditionalcolumn[iColumn].cbStruct )
                {
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }

                C_ASSERT( sizeof( JET_CONDITIONALCOLUMN_W ) == sizeof( JET_CONDITIONALCOLUMN_A ) );
                memcpy( pcondcolumn + iColumn, ( (JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_W *)pvData )->rgconditionalcolumn + iColumn, sizeof( JET_CONDITIONALCOLUMN_W ) );

                Call( pcondcolumnName[iColumn].ErrSet( ( (JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_W *)pvData )->rgconditionalcolumn[iColumn].szColumnName) );
                pcondcolumn[iColumn].szColumnName = pcondcolumnName[iColumn];
            }

            lpvData = &addcondidx;
            lcbData = sizeof(addcondidx);
            break;

        case opDDLConvAddColumnCallback:
            if( sizeof( JET_DDLADDCOLUMNCALLBACK_W ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            CallR( lszTable.ErrSet( ( (JET_DDLADDCOLUMNCALLBACK_W *)pvData )->szTable ) );
            CallR( lszColumn.ErrSet( ( (JET_DDLADDCOLUMNCALLBACK_W *)pvData )->szColumn ) );
            CallR( lszCallback.ErrSet( ( (JET_DDLADDCOLUMNCALLBACK_W *)pvData )->szCallback ) );

            C_ASSERT( sizeof( JET_DDLADDCOLUMNCALLBACK_A ) == sizeof( JET_DDLADDCOLUMNCALLBACK_W ) );
            memcpy( &addcolumncallback, pvData, sizeof( JET_DDLADDCOLUMNCALLBACK_W ) );
            addcolumncallback.szTable = lszTable;
            addcolumncallback.szColumn= lszColumn;
            addcolumncallback.szCallback= lszCallback;

            lpvData = &addcolumncallback;
            lcbData = sizeof(addcolumncallback);
            break;

        case opDDLConvIncreaseMaxColumnSize:
            if( sizeof( JET_DDLMAXCOLUMNSIZE_W ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            CallR( lszTable.ErrSet( ( (JET_DDLMAXCOLUMNSIZE_W *)pvData )->szTable ) );
            CallR( lszColumn.ErrSet( ( (JET_DDLMAXCOLUMNSIZE_W *)pvData )->szColumn ) );

            C_ASSERT( sizeof( JET_DDLMAXCOLUMNSIZE_A ) == sizeof( JET_DDLMAXCOLUMNSIZE_W ) );
            memcpy( &maxcolumnsize, pvData, sizeof( JET_DDLMAXCOLUMNSIZE_W ) );
            maxcolumnsize.szTable = lszTable;
            maxcolumnsize.szColumn= lszColumn;

            lpvData = &maxcolumnsize;
            lcbData = sizeof(maxcolumnsize);
            break;

        case opDDLConvChangeIndexDensity:
            if ( sizeof( JET_DDLINDEXDENSITY_W ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            CallR( lszTable.ErrSet( ( (JET_DDLINDEXDENSITY_W *)pvData )->szTable ) );
            CallR( lszIndex.ErrSet( ( (JET_DDLINDEXDENSITY_W *)pvData )->szIndex ) );

            C_ASSERT( sizeof( JET_DDLINDEXDENSITY_A ) == sizeof( JET_DDLINDEXDENSITY_W ) );
            memcpy( &indexdensity, pvData, sizeof( JET_DDLINDEXDENSITY_W ) );
            indexdensity.szTable = lszTable;
            indexdensity.szIndex = lszIndex;

            lpvData = &indexdensity;
            lcbData = sizeof(indexdensity);
            break;

        case opDDLConvChangeCallbackDLL:
            if( sizeof( JET_DDLCALLBACKDLL_W ) != cbData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            CallR( lszOldDLL.ErrSet( ( (JET_DDLCALLBACKDLL_W *)pvData )->szOldDLL ) );
            CallR( lszNewDLL.ErrSet( ( (JET_DDLCALLBACKDLL_W *)pvData )->szNewDLL ) );

            C_ASSERT( sizeof( JET_DDLCALLBACKDLL_A ) == sizeof( JET_DDLCALLBACKDLL_W ) );
            callbackdll.szOldDLL = lszOldDLL;
            callbackdll.szNewDLL = lszNewDLL;

            lpvData = &callbackdll;
            lcbData = sizeof(callbackdll);
            break;

        case opDDLConvNull:
        case opDDLConvMax:
        default:
            break;
    }

    err = JetConvertDDLEx( sesid, dbid, convtyp, lpvData, lcbData );

HandleError:
    delete[] pcondcolumn;
    delete[] pcondcolumnName;

    return err;
}

JET_ERR JET_API JetConvertDDLA(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_OPDDLCONV              convtyp,
    __out_bcount( cbData ) void *   pvData,
    __in ULONG              cbData )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opConvertDDL, JetConvertDDLEx( sesid, dbid, convtyp, pvData, cbData ) );
}

JET_ERR JET_API JetConvertDDLW(
    __in JET_SESID                  sesid,
    __in JET_DBID                   dbid,
    __in JET_OPDDLCONV              convtyp,
    __out_bcount( cbData ) void *   pvData,
    __in ULONG              cbData )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opConvertDDL, JetConvertDDLExW( sesid, dbid, convtyp, pvData, cbData ) );
}

LOCAL JET_ERR JetUpgradeDatabaseEx(
    __in JET_SESID          sesid,
    __in JET_PCWSTR         wszDbFileName,
    __in const JET_GRBIT    grbit )
{
    APICALL_SESID   apicall( opUpgradeDatabase );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p[%s],0x%x)",
            __FUNCTION__,
            sesid,
            wszDbFileName,
            SzOSFormatStringW( wszDbFileName ),
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDBUpgradeDatabase(
                                        sesid,
                                        wszDbFileName,
                                        grbit ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetUpgradeDatabaseExA(
    __in JET_SESID          sesid,
    __in JET_PCSTR          szDbFileName,
    __in const JET_GRBIT    grbit )
{
    ERR             err             = JET_errSuccess;
    CAutoWSZPATH    lwszDbFileName;

    CallR( lwszDbFileName.ErrSet( szDbFileName ) );

    return JetUpgradeDatabaseEx( sesid, lwszDbFileName, grbit );
}

JET_ERR JET_API JetUpgradeDatabaseA(
    __in JET_SESID          sesid,
    __in JET_PCSTR          szDbFileName,
    _Reserved_ JET_PCSTR    szSLVFileName,
    __in const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opUpgradeDatabase, JetUpgradeDatabaseExA( sesid, szDbFileName, grbit ) );
}

JET_ERR JET_API JetUpgradeDatabaseW(
    __in JET_SESID          sesid,
    __in JET_PCWSTR         wszDbFileName,
    _Reserved_ JET_PCWSTR   wszSLVFileName,
    __in const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opUpgradeDatabase, JetUpgradeDatabaseEx( sesid, wszDbFileName, grbit ) );
}

INLINE JET_ERR JetIUtilities( JET_SESID sesid, JET_DBUTIL_W *pdbutil )
{
    APICALL_SESID   apicall( opDBUtilities );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamDBUtilities( sesid, pdbutil ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetInitEx(    __inout_opt JET_INSTANCE    *pinstance,
                                    __in_opt JET_RSTINFO2_W     *prstInfo,
                                    __in JET_GRBIT      grbit );
class CAutoDBUTILW
{
    public:
        CAutoDBUTILW():m_pdbutil( NULL ) { }
        ~CAutoDBUTILW();

    public:
        ERR ErrSet( const JET_DBUTIL_A * pdbutil );
        operator JET_DBUTIL_W*()        { return m_pdbutil; }

    private:
        JET_DBUTIL_W *  m_pdbutil;
        CAutoWSZ        m_wszDatabase;
        CAutoWSZ        m_wszBackup;
        CAutoWSZ        m_wszTable;
        CAutoWSZ        m_wszIndex;
        CAutoWSZ        m_wszIntegPrefix;
        CAutoWSZ        m_wszLog;
        CAutoWSZ        m_wszBase;
};

ERR CAutoDBUTILW::ErrSet( const JET_DBUTIL_A * pdbutil )
{
    ERR                 err         = JET_errSuccess;

    C_ASSERT( sizeof(JET_DBUTIL_W) == sizeof(JET_DBUTIL_A) );

    delete m_pdbutil;
    m_pdbutil = NULL;

    if ( pdbutil )
    {
        if ( sizeof(JET_DBUTIL_A) != pdbutil->cbStruct )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        Alloc( m_pdbutil = new JET_DBUTIL_W );
        memset( m_pdbutil, 0, sizeof(JET_DBUTIL_W) );

        m_pdbutil->cbStruct = sizeof( JET_DBUTIL_W );
        m_pdbutil->sesid = pdbutil->sesid;
        m_pdbutil->dbid = pdbutil->dbid;
        m_pdbutil->tableid = pdbutil->tableid;

        m_pdbutil->op = pdbutil->op;
        m_pdbutil->edbdump = pdbutil->edbdump;
        m_pdbutil->grbitOptions = pdbutil->grbitOptions;

        if ( m_pdbutil->op == opDBUTILChecksumLogFromMemory )
        {
            Call( m_wszLog.ErrSet( pdbutil->checksumlogfrommemory.szLog ) );
            m_pdbutil->checksumlogfrommemory.szLog = m_wszLog;

            Call( m_wszBase.ErrSet( pdbutil->checksumlogfrommemory.szBase ) );
            m_pdbutil->checksumlogfrommemory.szBase = m_wszBase;

            m_pdbutil->checksumlogfrommemory.pvBuffer = pdbutil->checksumlogfrommemory.pvBuffer;
            m_pdbutil->checksumlogfrommemory.cbBuffer = pdbutil->checksumlogfrommemory.cbBuffer;
        }
        else if ( m_pdbutil->op == opDBUTILDumpSpaceCategory )
        {
            Call( m_wszDatabase.ErrSet( pdbutil->spcatOptions.szDatabase ) );
            m_pdbutil->spcatOptions.szDatabase = m_wszDatabase;

            m_pdbutil->spcatOptions.pgnoFirst = pdbutil->spcatOptions.pgnoFirst;
            m_pdbutil->spcatOptions.pgnoLast = pdbutil->spcatOptions.pgnoLast;
            m_pdbutil->spcatOptions.pfnSpaceCatCallback = pdbutil->spcatOptions.pfnSpaceCatCallback;
            m_pdbutil->spcatOptions.pvContext = pdbutil->spcatOptions.pvContext;
        }
        else if ( m_pdbutil->op == opDBUTILDumpRBSPages )
        {
            Call( m_wszDatabase.ErrSet( pdbutil->rbsOptions.szDatabase ) );
            m_pdbutil->rbsOptions.szDatabase    = m_wszDatabase;
            m_pdbutil->rbsOptions.pgnoFirst     = pdbutil->rbsOptions.pgnoFirst;
            m_pdbutil->rbsOptions.pgnoLast      = pdbutil->rbsOptions.pgnoLast;
        }
        else
        {
            Call( m_wszDatabase.ErrSet( pdbutil->szDatabase) );
            m_pdbutil->szDatabase = m_wszDatabase;

            Call( m_wszBackup.ErrSet( pdbutil->szBackup) );
            m_pdbutil->szBackup = m_wszBackup;

            Call( m_wszTable.ErrSet( pdbutil->szTable) );
            m_pdbutil->szTable = m_wszTable;

            Call( m_wszIndex.ErrSet( pdbutil->szIndex) );
            m_pdbutil->szIndex = m_wszIndex;

            Call( m_wszIntegPrefix.ErrSet( pdbutil->szIntegPrefix) );
            m_pdbutil->szIntegPrefix = m_wszIntegPrefix;

            m_pdbutil->pgno = pdbutil->pgno;
            m_pdbutil->iline = pdbutil->iline;

            m_pdbutil->lGeneration = pdbutil->lGeneration;
            m_pdbutil->isec = pdbutil->isec;
            m_pdbutil->ib = pdbutil->ib;

            m_pdbutil->cRetry = pdbutil->cRetry;

            m_pdbutil->pfnCallback = pdbutil->pfnCallback;
            m_pdbutil->pvCallback = pdbutil->pvCallback;
        }
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        delete m_pdbutil;
        m_pdbutil = NULL;
    }

    return err;
}

CAutoDBUTILW::~CAutoDBUTILW()
{
    delete m_pdbutil;
}

LOCAL JET_ERR ErrUpdateHeaderFromTrailer( const wchar_t * const szDatabase, BOOL fSkipMinLogChecks )
{
    ERR     err             = JET_errSuccess;


    CallR( ErrOSUInit() );

    Call( ErrDBUpdateHeaderFromTrailer( szDatabase, fSkipMinLogChecks ) );

HandleError:

    OSUTerm();

    return err;
}

LOCAL JET_ERR JetDBUtilitiesEx( JET_DBUTIL_W *pdbutilW )
{
    AssertSzRTL( (JET_DBUTIL_W*)0 != pdbutilW, "Invalid (NULL) pdbutil. Call JET dev." );
    AssertSzRTL( (JET_DBUTIL_W*)(-1) != pdbutilW, "Invalid (-1) pdbutil. Call JET dev." );


#ifdef _WIN64
    C_ASSERT( sizeof(JET_DBUTIL_W) == 136 );
#else
    C_ASSERT( sizeof(JET_DBUTIL_W) == 84 );
#endif

    JET_ERR         err             = JET_errSuccess;
    JET_INSTANCE    instance        = 0;
    JET_SESID       sesid           = pdbutilW->sesid;
    BOOL            fCreatedInst    = fFalse;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p[sesid=%Ix,dbid=%d,tableid=%Ix,op=0x%x,edbdump=%d,grbitOptions=0x%x,...])",
            __FUNCTION__,
            pdbutilW,
            pdbutilW->sesid,
            pdbutilW->dbid,
            pdbutilW->tableid,
            pdbutilW->op,
            pdbutilW->edbdump,
            pdbutilW->grbitOptions ) );

    if ( pdbutilW->cbStruct != sizeof(JET_DBUTIL_W) )
        return ErrERRCheck( JET_errInvalidParameter );

    switch( pdbutilW->op )
    {
        case opDBUTILUpdateDBHeaderFromTrailer:
            Call( ErrUpdateHeaderFromTrailer( pdbutilW->szDatabase, pdbutilW->grbitOptions & JET_bitDBUtilOptionSkipMinLogChecksUpdateHeader ) );
            break;

        case opDBUTILEDBDump:
        case opDBUTILEDBRepair:
        case opDBUTILEDBScrub:
        case opDBUTILDBDefragment:
        case opDBUTILDBTrim:
        case opDBUTILDumpPageUsage:
            Call( JetIUtilities( pdbutilW->sesid, pdbutilW ) );
            break;

        case opDBUTILDumpHeader:
        case opDBUTILDumpLogfile:
        case opDBUTILDumpFlushMapFile:
        case opDBUTILDumpCheckpoint:
        case opDBUTILDumpLogfileTrackNode:
        case opDBUTILDumpPage:
        case opDBUTILDumpNode:
        case opDBUTILDumpData:
        case opDBUTILDumpSpace:
        case opDBUTILDumpSpaceCategory:
        case opDBUTILChecksumLogFromMemory:
        case opDBUTILDumpCachedFileHeader:
        case opDBUTILDumpCacheFile:
        case opDBUTILDumpRBSHeader:
        case opDBUTILDumpRBSPages:
        default:
            if ( 0 == sesid || JET_sesidNil == sesid )
            {
                char szInstanceName[ 64 ];

                OSStrCbFormatA( szInstanceName, sizeof( szInstanceName ),
                                "JetDBUtilities - %lu",
                                DwUtilThreadId() );

                Call( JetCreateInstance( &instance, szInstanceName ) );
                fCreatedInst = fTrue;

                Call( JetSetSystemParameterW( &instance, 0, JET_paramRecovery,           0, L"off" ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableOnlineDefrag, 0, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxTemporaryTables, 0, NULL ) );

                Call( JetSetSystemParameterW( &instance, 0, JET_paramNoInformationEvent, 1, NULL ) );

                if ( pdbutilW->op == opDBUTILChecksumLogFromMemory )
                {
                    Call( JetSetSystemParameterW( &instance, 0, JET_paramBaseName, 0, pdbutilW->checksumlogfrommemory.szBase ) );
                }

                fCreatedInst = fFalse;
                Call( JetInit( &instance ) );
                fCreatedInst = fTrue;

                Call( JetBeginSession( instance, &sesid, "user", "" ) );
                Assert( sesid != 0 && sesid != JET_sesidNil );
            }

            Call( JetIUtilities( sesid, pdbutilW ) );
            break;

    }

HandleError:
    if ( fCreatedInst )
    {
        if ( sesid != 0 && sesid != JET_sesidNil )
        {
            JetEndSession( sesid, 0 );
        }
        JetTerm2( instance, err < JET_errSuccess ? JET_bitTermAbrupt : JET_bitTermComplete );
    }

    return err;
}

LOCAL JET_ERR JetDBUtilitiesExA( JET_DBUTIL_A *pdbutil )
{
    AssertSzRTL( (JET_DBUTIL_A*)0 != pdbutil, "Invalid (NULL) pdbutil. Call JET dev." );
    AssertSzRTL( (JET_DBUTIL_A*)(-1) != pdbutil, "Invalid (-1) pdbutil. Call JET dev." );
    ERR         err     = JET_errSuccess;
    CAutoDBUTILW ldbutilW;

    if ( pdbutil->cbStruct != sizeof(JET_DBUTIL_A) )
        return ErrERRCheck( JET_errInvalidParameter );

    CallR( ldbutilW.ErrSet( pdbutil ) );

    return JetDBUtilitiesEx( ldbutilW );
}

JET_ERR JET_API JetDBUtilitiesA( JET_DBUTIL_A *pdbutil )
{
    JET_TRY( opDBUtilities, JetDBUtilitiesExA( pdbutil ) );
}


JET_ERR JET_API JetDBUtilitiesW( JET_DBUTIL_W *pdbutil )
{
    JET_TRY( opDBUtilities, JetDBUtilitiesEx( pdbutil ) );
}

LOCAL JET_ERR JetDefragmentEx(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    _In_opt_ JET_PCSTR          szTableName,
    _Inout_opt_ ULONG * pcPasses,
    _Inout_opt_ ULONG * pcSeconds,
    _In_ JET_CALLBACK           callback,
    _In_ JET_GRBIT              grbit )
{
    APICALL_SESID   apicall( opDefragment );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p[%s],0x%p[%s],0x%p[%s],0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            szTableName,
            OSFormatString( szTableName ),
            pcPasses,
            ( NULL != pcPasses ? OSFormat( "<%d>", *pcPasses ) : OSTRACENULLPARAM ),
            pcSeconds,
            ( NULL != pcSeconds ? OSFormat( "<%d>", *pcSeconds ) : OSTRACENULLPARAM ),
            callback,
            grbit ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        apicall.LeaveAfterCall( ErrIsamDefragment(
                                        sesid,
                                        dbid,
                                        szTableName,
                                        pcPasses,
                                        pcSeconds,
                                        callback,
                                        grbit ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetDefragmentExW(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    _In_opt_ JET_PCWSTR         wszTableName,
    _Inout_opt_ ULONG * pcPasses,
    _Inout_opt_ ULONG * pcSeconds,
    _In_ JET_CALLBACK           callback,
    _In_ JET_GRBIT              grbit )
{
    ERR         err             = JET_errSuccess;
    CAutoSZDDL  lszTableName;

    CallR( lszTableName.ErrSet( wszTableName ) );

    return JetDefragmentEx( sesid, dbid, lszTableName, pcPasses, pcSeconds, callback, grbit );
}

JET_ERR JET_API JetDefragmentA(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    _In_opt_ JET_PCSTR          szTableName,
    _Inout_opt_ ULONG * pcPasses,
    _Inout_opt_ ULONG * pcSeconds,
    _In_ JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDefragment, JetDefragmentEx( sesid, dbid, szTableName, pcPasses, pcSeconds, NULL, grbit ) );
}
JET_ERR JET_API JetDefragmentW(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    _In_opt_ JET_PCWSTR         wszTableName,
    _Inout_opt_ ULONG * pcPasses,
    _Inout_opt_ ULONG * pcSeconds,
    _In_ JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDefragment, JetDefragmentExW( sesid, dbid, wszTableName, pcPasses, pcSeconds, NULL, grbit ) );
}
JET_ERR JET_API JetDefragment2A(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    _In_opt_ JET_PCSTR          szTableName,
    _Inout_opt_ ULONG * pcPasses,
    _Inout_opt_ ULONG * pcSeconds,
    _In_ JET_CALLBACK           callback,
    _In_ JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDefragment, JetDefragmentEx( sesid, dbid, szTableName, pcPasses, pcSeconds, callback, grbit ) );
}
JET_ERR JET_API JetDefragment2W(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    _In_opt_ JET_PCWSTR         wszTableName,
    _Inout_opt_ ULONG * pcPasses,
    _Inout_opt_ ULONG * pcSeconds,
    _In_ JET_CALLBACK           callback,
    _In_ JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDefragment, JetDefragmentExW( sesid, dbid, wszTableName, pcPasses, pcSeconds, callback, grbit ) );
}

JET_ERR JET_API JetDefragment3A(
    _In_ JET_SESID              sesid,
    _In_ JET_PCSTR              szDatabaseName,
    __in_opt JET_PCSTR          szTableName,
    _Inout_opt_ ULONG * pcPasses,
    _Inout_opt_ ULONG * pcSeconds,
    _In_ JET_CALLBACK           callback,
    _In_ void *                 pvContext,
    _In_ JET_GRBIT              grbit )
{
    return ErrERRCheck( JET_errInvalidParameter );
}

JET_ERR JET_API JetDefragment3W(
    _In_ JET_SESID              sesid,
    _In_ JET_PCWSTR             wszDatabaseName,
    _In_opt_ JET_PCWSTR         wszTableName,
    _Inout_opt_ ULONG * pcPasses,
    _Inout_opt_ ULONG * pcSeconds,
    _In_ JET_CALLBACK           callback,
    _In_ void *                 pvContext,
    _In_ JET_GRBIT              grbit )
{
    return ErrERRCheck( JET_errInvalidParameter );
}

LOCAL JET_ERR JetDatabaseScanEx(
    __in JET_SESID      sesid,
    __in JET_DBID       dbid,
    __inout_opt ULONG * const pcSeconds,
    __in ULONG  cmsecSleep,
    __in JET_CALLBACK   pfnCallback,
    __in JET_GRBIT      grbit )
{
    APICALL_SESID   apicall( opDatabaseScan );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p[%s],%d,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            pcSeconds,
            ( NULL != pcSeconds ? OSFormat( "<%d>", *pcSeconds ) : OSTRACENULLPARAM ),
            cmsecSleep,
            pfnCallback,
            grbit ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        apicall.LeaveAfterCall( ErrIsamDatabaseScan(
                                        sesid,
                                        dbid,
                                        pcSeconds,
                                        cmsecSleep,
                                        pfnCallback,
                                        grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetDatabaseScan(
    __in JET_SESID      sesid,
    __in JET_DBID       dbid,
    __inout_opt ULONG * pcSeconds,
    __in ULONG  cmsecSleep,
    __in JET_CALLBACK   pfnCallback,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDatabaseScan, JetDatabaseScanEx( sesid, dbid, pcSeconds, cmsecSleep, pfnCallback, grbit ) );
}

LOCAL JET_ERR JetSetMaxDatabaseSizeEx(
    __in JET_SESID      sesid,
    __in JET_DBID       dbid,
    __in ULONG  cpg,
    __in JET_GRBIT      grbit )
{
    APICALL_SESID       apicall( opSetDatabaseSize );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            cpg,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamSetMaxDatabaseSize(
                                        sesid,
                                        dbid,
                                        cpg,
                                        grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetSetMaxDatabaseSize(
    __in JET_SESID      sesid,
    __in JET_DBID       dbid,
    __in ULONG  cpg,
    __in JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opSetDatabaseSize, JetSetMaxDatabaseSizeEx( sesid, dbid, cpg, grbit ) );
}


LOCAL JET_ERR JetGetMaxDatabaseSizeEx(
    __in JET_SESID          sesid,
    __in JET_DBID           dbid,
    __out ULONG *   pcpg,
    __in JET_GRBIT          grbit )
{
    APICALL_SESID       apicall( opGetDatabaseSize );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            pcpg,
            grbit ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        apicall.LeaveAfterCall( ErrIsamGetMaxDatabaseSize(
                                        sesid,
                                        dbid,
                                        pcpg,
                                        grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetGetMaxDatabaseSize(
    __in JET_SESID          sesid,
    __in JET_DBID           dbid,
    __out ULONG *   pcpg,
    __in JET_GRBIT          grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetDatabaseSize, JetGetMaxDatabaseSizeEx( sesid, dbid, pcpg, grbit ) );
}


LOCAL JET_ERR JetSetDatabaseSizeEx(
    __in JET_SESID          sesid,
    __in JET_PCWSTR         wszDatabaseName,
    __in ULONG      cpg,
    __out ULONG *   pcpgReal )
{
    APICALL_SESID   apicall( opSetDatabaseSize );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p[%s],0x%x,0x%p)",
            __FUNCTION__,
            sesid,
            wszDatabaseName,
            SzOSFormatStringW( wszDatabaseName ),
            cpg,
            pcpgReal ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamSetDatabaseSize(
                                        sesid,
                                        wszDatabaseName,
                                        cpg,
                                        pcpgReal ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetSetDatabaseSizeExA(
    __in JET_SESID          sesid,
    __in JET_PCSTR          szDatabaseName,
    __in ULONG      cpg,
    __out ULONG *   pcpgReal )
{
    ERR             err             = JET_errSuccess;
    CAutoWSZPATH    lwszDatabaseName;

    CallR( lwszDatabaseName.ErrSet( szDatabaseName ) );


    return JetSetDatabaseSizeEx( sesid, lwszDatabaseName, cpg, pcpgReal );
}

JET_ERR JET_API JetSetDatabaseSizeA(
    __in JET_SESID          sesid,
    __in JET_PCSTR          szDatabaseName,
    __in ULONG      cpg,
    __out ULONG *   pcpgReal )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opSetDatabaseSize, JetSetDatabaseSizeExA( sesid, szDatabaseName, cpg, pcpgReal ) );
}

JET_ERR JET_API JetSetDatabaseSizeW(
    __in JET_SESID          sesid,
    __in JET_PCWSTR         wszDatabaseName,
    __in ULONG      cpg,
    __out ULONG *   pcpgReal )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opSetDatabaseSize, JetSetDatabaseSizeEx( sesid, wszDatabaseName, cpg, pcpgReal ) );
}

LOCAL JET_ERR JetGrowDatabaseEx(
    __in JET_SESID          sesid,
    __in JET_DBID           dbid,
    __in ULONG      cpg,
    __in ULONG *    pcpgReal )
{
    APICALL_SESID   apicall( opGrowDatabase );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%x,0x%p)",
            __FUNCTION__,
            sesid,
            dbid,
            cpg,
            pcpgReal ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        apicall.LeaveAfterCall( ErrIsamResizeDatabase(
                                        sesid,
                                        dbid,
                                        cpg,
                                        pcpgReal,
                                        JET_bitResizeDatabaseOnlyGrow ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetGrowDatabase(
    __in JET_SESID          sesid,
    __in JET_DBID           dbid,
    __in ULONG      cpg,
    __in ULONG *    pcpgReal )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGrowDatabase, JetGrowDatabaseEx( sesid, dbid, cpg, pcpgReal ) );
}

LOCAL JET_ERR JetResizeDatabaseEx(
    __in JET_SESID          sesid,
    __in JET_DBID           dbid,
    __in ULONG      cpg,
    __out ULONG *   pcpgActual,
    __in const JET_GRBIT    grbit )
{
    APICALL_SESID   apicall( opResizeDatabase );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%x,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            cpg,
            pcpgActual,
            grbit ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        apicall.LeaveAfterCall( ErrIsamResizeDatabase(
                                        sesid,
                                        dbid,
                                        cpg,
                                        pcpgActual,
                                        grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetResizeDatabase(
    __in JET_SESID          sesid,
    __in JET_DBID           dbid,
    __in ULONG      cpg,
    __out ULONG *   pcpgActual,
    __in const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opResizeDatabase, JetResizeDatabaseEx( sesid, dbid, cpg, pcpgActual, grbit ) );
}

LOCAL JET_ERR JetSetSessionContextEx( __in JET_SESID sesid, __in ULONG_PTR ulContext )
{
    APICALL_SESID   apicall( opSetSessionContext );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix)",
            __FUNCTION__,
            sesid,
            ulContext ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrIsamSetSessionContext( sesid, ulContext ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetSetSessionContext( __in JET_SESID vsesid, __in ULONG_PTR ulContext )
{
    JET_VALIDATE_SESID( vsesid );
    JET_TRY( opSetSessionContext, JetSetSessionContextEx( vsesid, ulContext ) );
}

LOCAL JET_ERR JetResetSessionContextEx( __in JET_SESID sesid )
{
    APICALL_SESID   apicall( opResetSessionContext );

    OSTrace( JET_tracetagAPI, OSFormat( "Start %s(0x%Ix)", __FUNCTION__, sesid ) );

    if ( apicall.FEnter( sesid, fTrue ) )
    {
        apicall.LeaveAfterCall( ErrIsamResetSessionContext( sesid ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetResetSessionContext( __in JET_SESID sesid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opResetSessionContext, JetResetSessionContextEx( sesid ) );
}

#pragma warning( disable : 4509 )



LOCAL JET_ERR JetSetSystemParameterEx(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_SESID          sesid,
    __in ULONG          paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCWSTR         wszParam )
{
    APICALL_INST    apicall( opSetSystemParameter );
    INST*           pinst   = NULL;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p,0x%Ix,0x%x,0x%Ix,0x%p[%s])",
            __FUNCTION__,
            pinstance,
            sesid,
            paramid,
            lParam,
            wszParam,
            SzOSFormatStringW( wszParam ) ) );

    INST::EnterCritInst();

    FixDefaultSystemParameters();

    if ( RUNINSTGetMode() == runInstModeOneInst )
    {
        if ( !g_rgparam[ paramid ].FGlobal() )
        {
            for ( size_t ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
            {
                if ( pinstNil != g_rgpinst[ ipinst ] )
                {
                    pinst = g_rgpinst[ ipinst ];
                }
            }
        }
    }
    else if ( RUNINSTGetMode() == runInstModeMultiInst )
    {
        if ( sesid && sesid != JET_sesidNil )
        {
            const ERR err = ErrJetValidateSession( sesid );
            if ( err < JET_errSuccess )
            {
                INST::LeaveCritInst();
                return ErrERRCheck( err );
            }

            pinst = PinstFromPpib( (PIB*)sesid );
        }
        else if ( pinstance && *pinstance && *pinstance != JET_instanceNil )
        {
            const ERR err = ErrJetValidateInstance( *pinstance );
            if ( err < JET_errSuccess )
            {
                INST::LeaveCritInst();
                return ErrERRCheck( err );
            }

            pinst = *(INST **)pinstance;
        }
    }

    if ( !pinst )
    {
        const ERR err = ErrSetSystemParameter( pinstNil, JET_sesidNil, paramid, lParam, wszParam, fFalse );
        INST::LeaveCritInst();
        return err;
    }


    const BOOL fInJetInitCallback = Ptls()->fInSoftStart && Ptls()->fInCallback;
    if ( apicall.FEnterWithoutInit( pinst, fInJetInitCallback ) )
    {
        const ERR err = ErrSetSystemParameter( pinst, sesid, paramid, lParam, wszParam, fFalse );
        INST::LeaveCritInst();
        apicall.LeaveAfterCall( err );
    }
    else
    {
        INST::LeaveCritInst();
    }

    return apicall.ErrResult();
}

#pragma warning( default : 4509 )

LOCAL JET_ERR JetSetSystemParameterExA(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_SESID          sesid,
    __in ULONG          paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCSTR          szParam )
{
    ERR         err         = JET_errSuccess;
    CAutoWSZ    lwszParam;

    CallR( lwszParam.ErrSet( szParam ) );

    return JetSetSystemParameterEx( pinstance, sesid, paramid, lParam, lwszParam );
}

JET_ERR JET_API JetSetSystemParameterA(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_SESID          sesid,
    __in ULONG          paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCSTR          szParam )
{
    JET_TRY( opSetSystemParameter, JetSetSystemParameterExA( pinstance, sesid, paramid, lParam, szParam ) );
}

JET_ERR JET_API JetSetSystemParameterW(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_SESID          sesid,
    __in ULONG          paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCWSTR         wszParam )
{
    JET_TRY( opSetSystemParameter, JetSetSystemParameterEx( pinstance, sesid, paramid, lParam, wszParam ) );
}

JET_ERR JetGetResourceParamEx(
    __in JET_INSTANCE   instance,
    __in JET_RESOPER    resoper,
    __in JET_RESID      resid,
    __out JET_API_PTR*  pulParam )
{
    APICALL_INST    apicall( opGetResourceParam );
    INST            *pinst;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%x,0x%p)",
            __FUNCTION__,
            instance,
            resoper,
            resid,
            pulParam ) );

    SetPinst( &instance, JET_sesidNil, &pinst );

    if ( pinstNil == pinst || ( JET_resoperMaxUse != resoper && JET_resoperCurrentUse != resoper ) )
    {
        return ErrRESGetResourceParam( pinstNil, resid, resoper, pulParam );
    }

    if ( apicall.FEnterWithoutInit( pinst ) )
    {
        apicall.LeaveAfterCall( ErrRESGetResourceParam( pinst, resid, resoper, pulParam ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetGetResourceParam(
    __in JET_INSTANCE   instance,
    __in JET_RESOPER    resoper,
    __in JET_RESID      resid,
    __out JET_API_PTR*  pulParam )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opGetResourceParam, JetGetResourceParamEx( instance, resoper, resid, pulParam ) );
}


JET_ERR JetSetResourceParamEx(
    __in JET_INSTANCE   instance,
    __in JET_RESOPER    resoper,
    __in JET_RESID      resid,
    __in JET_API_PTR    ulParam )
{
    if ( JET_residAll == resid )
    {
        ERR         err                             = JET_errSuccess;
        DWORD_PTR   rgulParamPrev[ JET_residMax ]   = { 0 };
        BOOL        rgfParamSet[ JET_residMax ]     = { 0 };
        JET_RESID   residT;

        for ( residT = JET_RESID( JET_residNull + 1 );
            residT < JET_residMax;
            residT = JET_RESID( residT + 1 ) )
        {
            if ( residT == JET_residPAGE )
            {
                continue;
            }
            Call( JetGetResourceParamEx( instance, resoper, residT, &rgulParamPrev[ residT ] ) );
            Call( JetSetResourceParamEx( instance, resoper, residT, ulParam ) );
            rgfParamSet[ residT ] = fTrue;
        }

    HandleError:
        if ( err < JET_errSuccess )
        {
            for ( residT = JET_residNull; residT < JET_residMax; residT = JET_RESID( residT + 1 ) )
            {
                if ( rgfParamSet[ residT ] )
                {
                    CallS( JetSetResourceParamEx( instance, resoper, residT, rgulParamPrev[ residT ] ) );
                }
            }
        }
        return err;
    }

    APICALL_INST    apicall( opSetResourceParam );
    INST            *pinst;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
        "Start %s(0x%Ix,0x%x,0x%x,0x%Ix)",
        __FUNCTION__,
        instance,
        resoper,
        resid,
        ulParam ) );

    SetPinst( &instance, JET_sesidNil, &pinst );

    if ( !BoolParam( pinst, JET_paramEnableAdvanced ) )
    {
        return JET_errSuccess;
    }

    if ( pinstNil == pinst || JET_resoperMaxUse != resoper )
    {
        if ( JET_residPAGE == resid && JET_resoperSize == resoper )
        {
            return ErrSetSystemParameter( pinstNil, JET_sesidNil, JET_paramDatabasePageSize, ulParam, NULL );
        }
        else if ( JET_residVERBUCKET == resid && JET_resoperSize == resoper )
        {
            return ErrSetSystemParameter( pinstNil, JET_sesidNil, JET_paramVerPageSize, ulParam, NULL );
        }
        else if ( JET_residVERBUCKET == resid && JET_resoperMinUse == resoper )
        {
            return ErrSetSystemParameter( pinstNil, JET_sesidNil, JET_paramGlobalMinVerPages, ulParam, NULL );
        }

        else
        {
            return ErrRESSetResourceParam( pinstNil, resid, resoper, ulParam );
        }
    }

    if ( apicall.FEnterWithoutInit( pinst ) )
    {
        switch ( resid )
        {
            case JET_residSCB:
                apicall.LeaveAfterCall( ErrSetSystemParameter( pinst, JET_sesidNil, JET_paramMaxTemporaryTables, ulParam, NULL ) );
                break;
            case JET_residFUCB:
                apicall.LeaveAfterCall( ErrSetSystemParameter( pinst, JET_sesidNil, JET_paramMaxCursors, ulParam, NULL ) );
                break;
            case JET_residPIB:
                apicall.LeaveAfterCall( ErrSetSystemParameter( pinst, JET_sesidNil, JET_paramMaxSessions, ulParam - cpibSystem, NULL ) );
                break;
            case JET_residVERBUCKET:
                apicall.LeaveAfterCall( ErrSetSystemParameter( pinst, JET_sesidNil, JET_paramMaxVerPages, ulParam - cbucketSystem, NULL ) );
                break;

            default:
                apicall.LeaveAfterCall( ErrRESSetResourceParam( pinst, resid, resoper, ulParam ) );
        }
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetSetResourceParam(
    __in JET_INSTANCE   instance,
    __in JET_RESOPER    resoper,
    __in JET_RESID      resid,
    __in JET_API_PTR    ulParam )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opSetResourceParam, JetSetResourceParamEx( instance, resoper, resid, ulParam ) );
}

ERR ErrTermAlloc( JET_INSTANCE instance )
{
    ERR     err     = JET_errSuccess;
    INST    *pinst;

    CallR( ErrFindPinst( instance, &pinst ) );

    IRSCleanUpAllIrsResources( pinst );

    FreePinst( pinst );

    return err;
}


ERR ErrInitComplete(    JET_INSTANCE    instance,
                        JET_RSTINFO2_W  *prstInfo,
                        JET_GRBIT       grbit )
{
    APICALL_INST    apicall( opInit );
    ERR             err;
    INST *          pinst;
    INT             ipinst;

    CallR( ErrFindPinst( instance, &pinst, &ipinst ) );


    CCriticalSection *pcritInst = &g_critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
    pcritInst->Enter();

    if ( !apicall.FEnterForInit( pinst ) )
    {
        pcritInst->Leave();
        return apicall.ErrResult();
    }

    pinst->APILock( pinst->fAPIInitializing );
    pcritInst->Leave();

    Assert( !pinst->m_fJetInitialized );
    Assert( !pinst->m_fTermInProgress );

    Assert ( err >= 0 );

    if ( prstInfo )
    {
        Call( pinst->m_plog->ErrBuildRstmapForSoftRecovery( prstInfo->rgrstmap, prstInfo->crstmap ) );

        pinst->m_plog->LGSetLgposRecoveryStop( *(LGPOS *)&prstInfo->lgposStop );

        pinst->m_pfnInitCallback = prstInfo->pfnCallback;
        pinst->m_pvInitCallbackContext = prstInfo->pvCallbackContext;
        pinst->m_fAllowAPICallDuringRecovery = !!( grbit & JET_bitExternalRecoveryControl );
        if ( pinst->m_fAllowAPICallDuringRecovery )
        {
            pinst->APIUnlock( pinst->fAPIInitializing );
        }
    }
    else
    {
        pinst->m_plog->LGSetLgposRecoveryStop( lgposMax );
        pinst->m_pfnInitCallback = NULL;
        pinst->m_pvInitCallbackContext = NULL;
        pinst->m_fAllowAPICallDuringRecovery = fFalse;
    }

    err = ErrInit(  pinst,
                    fFalse,
                    grbit );

    Assert( JET_errAlreadyInitialized != err );

    Assert( !pinst->m_fJetInitialized );
    Assert( !pinst->m_fTermInProgress );

    if ( err >= 0 )
    {
        pinst->m_fJetInitialized = fTrue;


        pinst->m_fBackupAllowed = fTrue;
    }

    Assert( !pinst->m_fTermInProgress );

HandleError:

    pinst->m_plog->FreeRstmap();

    if ( !pinst->m_fAllowAPICallDuringRecovery )
    {
        pinst->APIUnlock( pinst->fAPIInitializing );
    }
    Assert( !( pinst->m_cSessionInJetAPI & pinst->fAPIInitializing ) );

    apicall.LeaveAfterCall( err );
    return apicall.ErrResult();
}

ERR ErrTermComplete( JET_INSTANCE instance, JET_GRBIT grbit )
{
    APICALL_INST    apicall( opTerm );
    ERR             err;
    INST *          pinst;
    INT             ipinst;

    CallR( ErrFindPinst( instance, &pinst, &ipinst ) );

    OSTrace( JET_tracetagInitTerm, OSFormat( "JetTerm instance %p [grbit=0x%x]", pinst, grbit ) );

    CCriticalSection *pcritInst = &g_critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
    pcritInst->Enter();

    if ( g_rgpinst[ipinst] != pinst )
    {
        pcritInst->Leave();
        return ErrERRCheck( JET_errNotInitialized );
    }
    else if ( !pinst->m_fJetInitialized
        && ( ( pinst->m_fSTInit == fSTInitNotDone )
            || ( pinst->m_fSTInit == fSTInitFailed ) ) )
    {
        pcritInst->Leave();
        CallS( ErrTermAlloc( instance ) );
        return JET_errSuccess;
    }

    if ( !apicall.FEnterForTerm( pinst ) )
    {
        pcritInst->Leave();
        return apicall.ErrResult();
    }

    AtomicIncrement( (LONG *)&g_cTermsInProgress );

    Assert( !pinst->m_fTermInProgress );
    pinst->m_fTermInProgress = fTrue;
    if ( !(grbit & JET_bitTermDirty) )
    {
        pinst->m_fNoWaypointLatency = fTrue;
    }

    pinst->m_isdlTerm.InitSequence( isdltypeTerm, eTermSeqMax );
    Assert( pinst->m_isdlTerm.FActiveSequence() );

    IRSCleanUpAllIrsResources( pinst );

    size_t cpibInJetAPI;
    do
    {
        cpibInJetAPI = 0;

        pinst->m_critPIB.Enter();
        for ( PIB* ppibCurr = pinst->m_ppibGlobal; ppibCurr; ppibCurr = ppibCurr->ppibNext )
        {
            if ( levelNil != ppibCurr->Level() )
            {
                cpibInJetAPI += ppibCurr->m_cInJetAPI;
            }
        }
        pinst->m_critPIB.Leave();

        if ( cpibInJetAPI )
        {
            UtilSleep( cmsecWaitGeneric );
        }
    }
    while ( cpibInJetAPI );

    pinst->APILock( pinst->fAPITerminating );
    pcritInst->Leave();

    Assert( pinst->m_fJetInitialized );

    pinst->SetStatusTerm();
    pinst->m_fBackupAllowed = fFalse;

    if ( grbit & JET_bitTermStopBackup || grbit & JET_bitTermAbrupt )
    {
        CESESnapshotSession::SnapshotCritEnter();
        if ( pinst->m_pOSSnapshotSession )
        {

            pinst->m_pbackup->BKLockBackup();
            pinst->m_pbackup->BKUnlockBackup();

            while( pinst->m_pOSSnapshotSession != NULL )
            {
                CESESnapshotSession::SnapshotCritLeave();
                UtilSleep( 2 );
                CESESnapshotSession::SnapshotCritEnter();
            }
            Assert( !pinst->m_pbackup->FBKBackupInProgress() );

            pinst->m_pbackup->BKSetStopBackup( fFalse );

            pinst->m_pbackup->BKAssertNoBackupInProgress();

            CESESnapshotSession::SnapshotCritLeave();
        }
        else
        {
            CESESnapshotSession::SnapshotCritLeave();

            CallSx( pinst->m_pbackup->ErrBKExternalBackupCleanUp( JET_errBackupAbortByServer ), JET_errNoBackup );

            pinst->m_pbackup->BKAssertNoBackupInProgress();
        }
    }

    pinst->m_isdlTerm.Trigger( eTermWaitedForUserWorkerThreads );

    err = ErrIsamTerm( (JET_INSTANCE)pinst, grbit );
    Assert( pinst->m_fJetInitialized );

    AtomicDecrement( (LONG *)&g_cTermsInProgress );

    const BOOL  fTermCompleted  = ( fSTInitNotDone == pinst->m_fSTInit );

    if ( fTermCompleted )
    {
        pinst->m_fJetInitialized = fFalse;
    }

    OSTrace( JET_tracetagInitTerm, OSFormat( "Term[%p] finished err=%d, fTermCompleted=%d", pinst, err, fTermCompleted ) );

    pinst->APIUnlock( pinst->fAPITerminating );

    if ( err != JET_errSuccess )
    {
        pinst->m_fTermInProgress = fFalse;
        pinst->m_fNoWaypointLatency = fFalse;
    }

    apicall.LeaveAfterTerm( err );

    if ( fTermCompleted )
    {
        CallS( ErrTermAlloc( instance ) );
    }

    return apicall.ErrResult();
}

class CAutoSETSYSPARAMW
{
    public:
        CAutoSETSYSPARAMW():m_psetsysparam( NULL ),m_psetsysparamW( NULL ), m_rgwsz( NULL ), m_csetsysparam( 0 ) { }
        ~CAutoSETSYSPARAMW();

    public:
        ERR ErrSet( JET_SETSYSPARAM_A * psetsysparam, const ULONG csetsysparam );
        void Result( );
        operator JET_SETSYSPARAM_W*()       { return m_psetsysparamW; }
        ULONG Count() const         { return m_csetsysparam; }

    private:
        JET_SETSYSPARAM_A *     m_psetsysparam;
        JET_SETSYSPARAM_W *     m_psetsysparamW;
        CAutoWSZ *              m_rgwsz;
        ULONG           m_csetsysparam;
};


ERR CAutoSETSYSPARAMW::ErrSet( JET_SETSYSPARAM_A * psetsysparam, const ULONG csetsysparam )
{
    ERR                 err         = JET_errSuccess;

    C_ASSERT( sizeof(JET_SETSYSPARAM_W) == sizeof(JET_SETSYSPARAM_A) );

    delete[] m_psetsysparamW;
    delete[] m_rgwsz;

    m_psetsysparamW = NULL;
    m_rgwsz = NULL;
    m_csetsysparam = 0;
    m_psetsysparam = NULL;

    if ( ( psetsysparam == NULL && csetsysparam != 0 ) ||
        ( psetsysparam != NULL && csetsysparam == 0 ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( !csetsysparam )
    {
        return JET_errSuccess;
    }

    Alloc( m_psetsysparamW = (JET_SETSYSPARAM_W*)new JET_SETSYSPARAM_W[csetsysparam] );
    memset( m_psetsysparamW, 0, csetsysparam * sizeof(JET_SETSYSPARAM_W) );
    Alloc( m_rgwsz = (CAutoWSZ*)new CAutoWSZ[csetsysparam] );
    m_csetsysparam = csetsysparam;

    for( ULONG i = 0; i < csetsysparam; i++ )
    {

        Call( m_rgwsz[i].ErrSet( psetsysparam[i].sz ) );
        m_psetsysparamW[i].sz = m_rgwsz[i];

        m_psetsysparamW[i].paramid = psetsysparam[i].paramid;
        m_psetsysparamW[i].lParam = psetsysparam[i].lParam;
        m_psetsysparamW[i].err = psetsysparam[i].err;
    }

    m_psetsysparam = psetsysparam;


HandleError:
    return err;
}

void CAutoSETSYSPARAMW::Result( )
{
    if ( m_psetsysparamW && m_psetsysparam )
    {
        for( ULONG i = 0; i < m_csetsysparam; i++ )
        {
            m_psetsysparam[i].err = m_psetsysparamW[i].err;
        }
    }
}

CAutoSETSYSPARAMW::~CAutoSETSYSPARAMW()
{
    delete[] m_psetsysparamW;
    delete[] m_rgwsz;
}



LOCAL JET_ERR JetEnableMultiInstanceEx(
    __in_ecount_opt( csetsysparam ) JET_SETSYSPARAM_W * psetsysparam,
    __in ULONG                                  csetsysparam,
    __out_opt ULONG *                           pcsetsucceed )
{
    ERR             err         = JET_errSuccess;
    ULONG   csetsucceed = 0;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p,0x%x,0x%p)",
            __FUNCTION__,
            psetsysparam,
            csetsysparam,
            pcsetsucceed ) );

    csetsucceed = 0;

    INST::EnterCritInst();

    FixDefaultSystemParameters();

    if ( csetsysparam && !psetsysparam )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( RUNINSTGetMode() != runInstModeNoSet )
    {
        CFixedBitmap    fbm( PbCbStackBitmap( JET_paramMaxValueInvalid ) );

        if ( RUNINSTGetMode() != runInstModeMultiInst )
        {
            Assert( RUNINSTGetMode() == runInstModeOneInst );
            Error( ErrERRCheck( JET_errRunningInOneInstanceMode ) );
        }

        for ( ULONG iset = 0; iset < csetsysparam; iset++ )
        {
            BOOL fMatches = fFalse;

            if ( psetsysparam[iset].paramid >= _countof(g_rgparamRaw) )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            const ULONG iparamid = psetsysparam[iset].paramid;

            if ( g_rgparam[iparamid].FGlobal() &&
                    g_rgparam[iparamid].m_fMayNotWriteAfterGlobalInit &&
                    !g_rgparam[iparamid].FWritten() )
            {
                WCHAR wszParamName[100];
                OSStrCbFormatW( wszParamName, sizeof(wszParamName), L"%hs", g_rgparam[iparamid].m_szParamName );
                const WCHAR * rgwszT[] = { wszParamName };
                UtilReportEvent(    eventWarning,
                                    GENERAL_CATEGORY,
                                    GLOBAL_SYSTEM_PARAMETER_NOT_SET_PREVIOUSLY_MISMATCH_ID,
                                    _countof(rgwszT), rgwszT );
                psetsysparam[iset].err = ErrERRCheck( JET_errSystemParameterConflict );
                Error( psetsysparam[iset].err );
            }

            IBitmapAPI::ERR errBitmap = fbm.ErrSet( iparamid, fTrue );
            Assert( errBitmap == IBitmapAPI::ERR::errSuccess );


            switch( g_rgparam[iparamid].Type_() )
            {
            case JetParam::typeBlockSize:
            case JetParam::typeBoolean:
            case JetParam::typeGrbit:
            case JetParam::typeInteger:
            {
                ULONG_PTR ulValue = g_rgparam[iparamid].Value();
                if ( iparamid == JET_paramConfiguration )
                {
                    ulValue = ulValue & ~JET_configDefault;
                }
                fMatches = ( ulValue == psetsysparam[iset].lParam );
                break;
            }
            case JetParam::typeString:
            case JetParam::typeFolder:
            case JetParam::typePath:
                fMatches = ( 0 == wcscmp( (WCHAR*)(g_rgparam[iparamid].Value()), psetsysparam[iset].sz ) );
                break;
            case JetParam::typePointer:
            case JetParam::typeUserDefined:
                fMatches = ( g_rgparam[iparamid].Value() == psetsysparam[iset].lParam );
                Expected( fMatches );
                break;
            default:
                AssertSz( fFalse, "Type switch not updated for %d\n", g_rgparam[iparamid].Type_() );
            }
            if ( !fMatches )
            {
                WCHAR wszParamName[100];
                OSStrCbFormatW( wszParamName, sizeof(wszParamName), L"%hs", g_rgparam[iparamid].m_szParamName );
                const WCHAR * rgwszT[] = { wszParamName };
                UtilReportEvent(    eventWarning,
                                    GENERAL_CATEGORY,
                                    GLOBAL_SYSTEM_PARAMETER_MISMATCH_ID,
                                    _countof(rgwszT), rgwszT );
                psetsysparam[iset].err = ErrERRCheck( JET_errSystemParameterConflict );
                Error( psetsysparam[iset].err );
            }
            psetsysparam[iset].err = JET_errSuccess;
            csetsucceed++;
        }

        for ( size_t iparamid = 0; iparamid < _countof(g_rgparamRaw); iparamid++ )
        {
            Assert( g_rgparam[iparamid].m_paramid == iparamid );

            if ( g_rgparam[iparamid].FGlobal() &&
                    g_rgparam[iparamid].m_fMayNotWriteAfterGlobalInit )
            {
                const BOOL fOriginallySet = g_rgparam[iparamid].FWritten();
                BOOL fNewlySet = fFalse;
                IBitmapAPI::ERR errBitmap = fbm.ErrGet( iparamid, &fNewlySet );
                Assert( errBitmap == IBitmapAPI::ERR::errSuccess );
                if ( fOriginallySet != fNewlySet &&
                        !g_rgparam[iparamid].m_fRegDefault )
                {
                    Assert( fOriginallySet );
                    Assert( !fNewlySet );
                    WCHAR wszParamName[100];
                    OSStrCbFormatW( wszParamName, sizeof(wszParamName), L"%hs", g_rgparam[iparamid].m_szParamName );
                    const WCHAR * rgwszT[] = { wszParamName };
                    UtilReportEvent(    eventWarning,
                                        GENERAL_CATEGORY,
                                        GLOBAL_SYSTEM_PARAMETER_SET_PREVIOUSLY_MISMATCH_ID,
                                        _countof(rgwszT), rgwszT );
                    Error( ErrERRCheck( JET_errSystemParameterConflict ) );
                }
            }
        }

        Error( ErrERRCheck( JET_errSystemParamsAlreadySet ) );
    }

    for ( ULONG iset = 0; iset < csetsysparam; iset++ )
    {
        psetsysparam[ iset ].err = ErrSetSystemParameter( pinstNil, JET_sesidNil, psetsysparam[ iset ].paramid, psetsysparam[ iset ].lParam, psetsysparam[ iset ].sz, fFalse );
        Call( psetsysparam[ iset ].err );
        Assert( g_rgparam[ psetsysparam[ iset ].paramid ].m_fWritten );
        csetsucceed++;
    }

    Assert( err >= JET_errSuccess );
    RUNINSTSetModeMultiInst();

HandleError:

    INST::LeaveCritInst();

    if ( pcsetsucceed )
    {
        *pcsetsucceed = csetsucceed;
    }

    Assert ( err < JET_errSuccess || !pcsetsucceed || csetsysparam == *pcsetsucceed );
    return err;
}

LOCAL JET_ERR JetEnableMultiInstanceExA(
    __in_ecount_opt( csetsysparam ) JET_SETSYSPARAM_A * psetsysparam,
    __in ULONG                                  csetsysparam,
    __out_opt ULONG *                           pcsetsucceed )
{
    ERR                 err             = JET_errSuccess;
    CAutoSETSYSPARAMW   lsetsysparamW;

    CallR( lsetsysparamW.ErrSet( psetsysparam, csetsysparam ) );
    Assert( csetsysparam == lsetsysparamW.Count() );

    err = JetEnableMultiInstanceEx( lsetsysparamW, lsetsysparamW.Count(), pcsetsucceed );

    lsetsysparamW.Result();

    return err;
}

JET_ERR JET_API JetEnableMultiInstanceA(
    __in_ecount_opt( csetsysparam ) JET_SETSYSPARAM_A * psetsysparam,
    __in ULONG                                  csetsysparam,
    __out_opt ULONG *                           pcsetsucceed )
{
    JET_TRY( opEnableMultiInstance, JetEnableMultiInstanceExA( psetsysparam, csetsysparam, pcsetsucceed ) );
}

JET_ERR JET_API JetEnableMultiInstanceW(
    __in_ecount_opt( csetsysparam ) JET_SETSYSPARAM_W * psetsysparam,
    __in ULONG                                  csetsysparam,
    __out_opt ULONG *                           pcsetsucceed )
{
    JET_TRY( opEnableMultiInstance, JetEnableMultiInstanceEx( psetsysparam, csetsysparam, pcsetsucceed ) );
}

LOCAL JET_ERR JetInitEx(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_RSTINFO2_W *   prstInfo,
    __in JET_GRBIT              grbit )
{
    ERR             err                 = JET_errSuccess;
    BOOL            fAllocateInstance   = ( NULL == pinstance || FINSTInvalid( *pinstance ) );
    INST *          pinst               = NULL;
    BOOL            fInCritSec          = fFalse;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p,0x%p,0x%x)",
            __FUNCTION__,
            pinstance,
            prstInfo,
            grbit ) );

    COSTraceTrackErrors trackerrors( __FUNCTION__ );

    if ( fAllocateInstance && NULL != pinstance )
        *pinstance = JET_instanceNil;

    CallR( ErrFaultInjection( 60589 ) );
    Assert( JET_errSuccess == ErrFaultInjection( 50587 ) );
    if ( JET_errSuccess != ErrFaultInjection( 47515 ) )
    {
        *( char* )0 = 0;
    }

    INST::EnterCritInst();
    fInCritSec = fTrue;

    if ( RUNINSTGetMode() == runInstModeMultiInst && !pinstance )
    {
        Error( ErrERRCheck( JET_errRunningInMultiInstanceMode ) );
    }

    if ( RUNINSTGetMode() == runInstModeOneInst && 0 != g_cpinstInit )
    {
        Error( ErrERRCheck( JET_errAlreadyInitialized ) );
    }

    if ( prstInfo && prstInfo->cbStruct != sizeof( JET_RSTINFO2_W ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( RUNINSTGetMode() == runInstModeNoSet )
    {
        Assert ( 0 == g_cpinstInit);
        RUNINSTSetModeOneInst();
    }

    fAllocateInstance = fAllocateInstance | (RUNINSTGetMode() == runInstModeOneInst);

    if ( fAllocateInstance )
    {
        Call( ErrNewInst( &pinst, NULL, NULL, NULL ) );
        Assert( !pinst->m_fJetInitialized );
    }
    else
    {
        Call( ErrFindPinst( *pinstance, &pinst ) );
        Assert( pinst->m_isdlInit.FActiveSequence() );
        pinst->m_isdlInit.ReThreadSequence();
    }

    INST::LeaveCritInst();
    fInCritSec = fFalse;


    OSTrace( JET_tracetagInitTerm, OSFormat( "JetInit instance %p [grbit=0x%x]", pinst, grbit ) );

    CallJ( ErrInitComplete( JET_INSTANCE( pinst ),
                            prstInfo,
                            grbit ), TermAlloc );

    Assert( err >= 0 );

    if ( fAllocateInstance && pinstance )
        *pinstance = (JET_INSTANCE) pinst;

    pinst->m_isdlInit.Trigger( eInitDone );


    const ULONG cbAdditionalFixedData = pinst->m_isdlInit.CbSprintFixedData();
    WCHAR * wszAdditionalFixedData = (WCHAR *)_alloca( cbAdditionalFixedData );
    pinst->m_isdlInit.SprintFixedData( wszAdditionalFixedData, cbAdditionalFixedData );
    const ULONG cbTimingResourceDataSequence = pinst->m_isdlInit.CbSprintTimings();
    WCHAR * wszTimingResourceDataSequence = (WCHAR *)_alloca( cbTimingResourceDataSequence );
    pinst->m_isdlInit.SprintTimings( wszTimingResourceDataSequence, cbTimingResourceDataSequence );
    const __int64 secsInit = pinst->m_isdlInit.UsecTimer( eSequenceStart, eInitDone ) / 1000000;
    WCHAR wszSeconds[16];
    WCHAR wszInstId[16];
    OSStrCbFormatW( wszSeconds, sizeof(wszSeconds), L"%I64d", secsInit );
    OSStrCbFormatW( wszInstId, sizeof(wszInstId), L"%d", IpinstFromPinst( pinst ) );
    const WCHAR * rgszT[4] = { wszInstId, wszSeconds, wszTimingResourceDataSequence, wszAdditionalFixedData };

    OSTrace( JET_tracetagInitTerm, OSFormat( "JetInit() successful [instance=%p]. Details:\r\n\t\tTimings Resource Usage: %ws", pinst, wszTimingResourceDataSequence ) );

    pinst->m_isdlInit.TermSequence();

    UtilReportEvent(
        eventInformation,
        GENERAL_CATEGORY,
        START_INSTANCE_DONE_ID,
        _countof( rgszT ),
        rgszT,
        0,
        NULL,
        pinst );

    Assert( !Ptls()->fInSoftStart );
    Assert( !fInCritSec );

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlInitSucceed|sysosrtlContextInst, pinst, (PVOID)&(pinst->m_iInstance), sizeof(pinst->m_iInstance) );

    return err;

TermAlloc:

    ULONG rgul[4] = { (ULONG)pinst->m_iInstance, (ULONG)err, PefLastThrow()->UlLine(), UlLineLastCall() };
    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlInitFail|sysosrtlContextInst, pinst, rgul, sizeof(rgul) );

    Assert( err < JET_errSuccess );
    Expected( !fInCritSec );

    if ( err < JET_errSuccess && err != JET_errRecoveredWithoutUndo )
    {
        const WCHAR* wszInstDisplayName = ( pinst != NULL && pinst->m_wszDisplayName != NULL ? pinst->m_wszDisplayName : L"_unknown_" );
        OSDiagTrackInit( wszInstDisplayName, pinst->m_plog->QwSignLogHash(), err );
    }

    if ( fAllocateInstance || ( NULL != pinstance && !FINSTInvalid( *pinstance ) ) )
    {

        if ( JET_errAlreadyInitialized != err )
        {
            ErrTermAlloc( (JET_INSTANCE)pinst );
            if ( NULL != pinstance )
            {
                *pinstance = NULL;
            }
        }
    }

HandleError:
    Assert( err < JET_errSuccess );

    if ( fInCritSec )
    {
        INST::LeaveCritInst();
        fInCritSec = fFalse;
    }

    Assert( !Ptls()->fInSoftStart );

    return err;
}

class CAutoRSTMAP2W
{
    public:
        CAutoRSTMAP2W() :m_rgrstmap( NULL ), m_rgwszDatabaseName( NULL ), m_rgwszNewDatabaseName( NULL ), m_crstmap( 0 ) {}
        ~CAutoRSTMAP2W();

    public:
        ERR ErrSet( const JET_RSTMAP_A* prstmap, const LONG crstmap );
        ERR ErrSet( const JET_RSTMAP_W* prstmap, const LONG crstmap );
        ERR ErrSet( const JET_RSTMAP2_A* prstmap, const LONG crstmap );
        operator JET_RSTMAP2_W*( )      { return m_rgrstmap; }
        LONG Count()                    { return m_crstmap; }

    private:
        JET_RSTMAP2_W *     m_rgrstmap;
        CAutoWSZ*           m_rgwszDatabaseName;
        CAutoWSZ*           m_rgwszNewDatabaseName;
        LONG                m_crstmap;
};

ERR CAutoRSTMAP2W::ErrSet( const JET_RSTMAP_A * prstmap, const LONG crstmap )
{
    ERR                 err         = JET_errSuccess;

    delete[] m_rgrstmap;
    delete[] m_rgwszDatabaseName;
    delete[] m_rgwszNewDatabaseName;
    m_rgrstmap = NULL;
    m_rgwszDatabaseName = NULL;
    m_rgwszNewDatabaseName = NULL;
    m_crstmap = 0;

    C_ASSERT( sizeof(JET_RSTMAP_W) == sizeof(JET_RSTMAP_A) );

    if ( ( prstmap == NULL && crstmap != 0 ) ||
        ( prstmap != NULL && crstmap == 0 ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( crstmap )
    {
        AllocR( m_rgrstmap = new JET_RSTMAP2_W[crstmap] );
        memset( m_rgrstmap, 0, crstmap * sizeof(JET_RSTMAP2_W) );
        AllocR( m_rgwszDatabaseName = new CAutoWSZ[crstmap] );
        AllocR( m_rgwszNewDatabaseName = new CAutoWSZ[crstmap] );
        m_crstmap = crstmap;

        for( INT i = 0 ; i < crstmap; i++ )
        {
            Call( m_rgwszDatabaseName[i].ErrSet( prstmap[i].szDatabaseName ) );
            Call( m_rgwszNewDatabaseName[i].ErrSet( prstmap[i].szNewDatabaseName ) );
            m_rgrstmap[i].cbStruct = sizeof( JET_RSTMAP2_W );
            m_rgrstmap[i].szDatabaseName = (WCHAR*)m_rgwszDatabaseName[i];
            m_rgrstmap[i].szNewDatabaseName = (WCHAR*)m_rgwszNewDatabaseName[i];
            m_rgrstmap[i].rgsetdbparam = NULL;
            m_rgrstmap[i].csetdbparam = 0;
            m_rgrstmap[i].grbit = NO_GRBIT;
        }
    }

HandleError:
    return err;
}

ERR CAutoRSTMAP2W::ErrSet( const JET_RSTMAP_W * prstmap, const LONG crstmap )
{
    ERR                 err         = JET_errSuccess;

    delete[] m_rgrstmap;
    delete[] m_rgwszDatabaseName;
    delete[] m_rgwszNewDatabaseName;
    m_rgrstmap = NULL;
    m_rgwszDatabaseName = NULL;
    m_rgwszNewDatabaseName = NULL;
    m_crstmap = 0;

    if ( ( prstmap == NULL && crstmap != 0 ) ||
        ( prstmap != NULL && crstmap == 0 ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( crstmap )
    {
        AllocR( m_rgrstmap = new JET_RSTMAP2_W[crstmap] );
        memset( m_rgrstmap, 0, crstmap * sizeof(JET_RSTMAP2_W) );
        m_crstmap = crstmap;

        for( INT i = 0 ; i < crstmap; i++ )
        {
            m_rgrstmap[i].cbStruct = sizeof( JET_RSTMAP2_W );
            m_rgrstmap[i].szDatabaseName = prstmap[i].szDatabaseName;
            m_rgrstmap[i].szNewDatabaseName = prstmap[i].szNewDatabaseName;
            m_rgrstmap[i].rgsetdbparam = NULL;
            m_rgrstmap[i].csetdbparam = 0;
            m_rgrstmap[i].grbit = NO_GRBIT;
        }
    }

HandleError:
    return err;
}

ERR CAutoRSTMAP2W::ErrSet( const JET_RSTMAP2_A * prstmap, const LONG crstmap )
{
    ERR                 err = JET_errSuccess;

    delete[] m_rgrstmap;
    delete[] m_rgwszDatabaseName;
    delete[] m_rgwszNewDatabaseName;
    m_rgrstmap = NULL;
    m_rgwszDatabaseName = NULL;
    m_rgwszNewDatabaseName = NULL;
    m_crstmap = 0;

    C_ASSERT( sizeof( JET_RSTMAP2_W ) == sizeof( JET_RSTMAP2_A ) );

    if ( ( prstmap == NULL && crstmap != 0 ) ||
        ( prstmap != NULL && crstmap == 0 ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( crstmap )
    {
        AllocR( m_rgrstmap = new JET_RSTMAP2_W[crstmap] );
        memset( m_rgrstmap, 0, crstmap * sizeof( JET_RSTMAP2_W ) );
        AllocR( m_rgwszDatabaseName = new CAutoWSZ[crstmap] );
        AllocR( m_rgwszNewDatabaseName = new CAutoWSZ[crstmap] );
        m_crstmap = crstmap;

        for ( INT i = 0; i < crstmap; i++ )
        {
            Call( m_rgwszDatabaseName[i].ErrSet( prstmap[i].szDatabaseName ) );
            Call( m_rgwszNewDatabaseName[i].ErrSet( prstmap[i].szNewDatabaseName ) );
            m_rgrstmap[i].cbStruct = sizeof( JET_RSTMAP2_W );
            m_rgrstmap[i].szDatabaseName = (WCHAR*)m_rgwszDatabaseName[i];
            m_rgrstmap[i].szNewDatabaseName = (WCHAR*)m_rgwszNewDatabaseName[i];
            m_rgrstmap[i].rgsetdbparam = prstmap[i].rgsetdbparam;
            m_rgrstmap[i].csetdbparam = prstmap[i].csetdbparam;
            m_rgrstmap[i].grbit = prstmap[i].grbit;
        }
    }

HandleError:
    return err;
}

CAutoRSTMAP2W::~CAutoRSTMAP2W()
{
    delete[] m_rgrstmap;
    delete[] m_rgwszDatabaseName;
    delete[] m_rgwszNewDatabaseName;
}

class CAutoRSTINFOW
{
    public:
        CAutoRSTINFOW() : m_prstInfo( NULL ), m_pInitCallbackWrapper( NULL ) {}
        ~CAutoRSTINFOW();

    public:
        ERR ErrSet( const JET_RSTINFO_A * prstInfo );
        ERR ErrSet( const JET_RSTINFO_W * prstInfo );
        ERR ErrSet( const JET_RSTINFO2_A * prstInfo );
        operator JET_RSTINFO2_W*()      { return m_prstInfo; }

    private:
        JET_RSTINFO2_W *        m_prstInfo;
        CAutoRSTMAP2W           m_autorstmap;
        InitCallbackWrapper*    m_pInitCallbackWrapper;

};

ERR CAutoRSTINFOW::ErrSet( const JET_RSTINFO_A * prstInfo )
{
    ERR                 err         = JET_errSuccess;

    C_ASSERT( sizeof(JET_RSTINFO_W) == sizeof(JET_RSTINFO_A) );

    delete m_prstInfo;

    m_prstInfo = NULL;

    if ( NULL == prstInfo )
    {
        return JET_errSuccess;
    }
    else if ( sizeof(JET_RSTINFO_A) != prstInfo->cbStruct )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Alloc( m_prstInfo = new JET_RSTINFO2_W );
    memset( m_prstInfo, 0, sizeof(JET_RSTINFO2_W) );
    m_prstInfo->cbStruct = sizeof(JET_RSTINFO2_W);

    CallR( m_autorstmap.ErrSet( prstInfo->rgrstmap, prstInfo->crstmap ) );

    Alloc( m_pInitCallbackWrapper = new InitCallbackWrapper( prstInfo->pfnStatus ) );

    m_prstInfo->rgrstmap = m_autorstmap;
    Assert( prstInfo->crstmap == m_autorstmap.Count() );
    m_prstInfo->crstmap = m_autorstmap.Count();

    m_prstInfo->lgposStop = prstInfo->lgposStop;
    m_prstInfo->logtimeStop = prstInfo->logtimeStop;
    m_prstInfo->pfnCallback = InitCallbackWrapper::PfnWrapper;
    m_prstInfo->pvCallbackContext = m_pInitCallbackWrapper;

HandleError:
    return err;
}

ERR CAutoRSTINFOW::ErrSet( const JET_RSTINFO_W * prstInfo )
{
    ERR                 err         = JET_errSuccess;

    delete m_prstInfo;

    m_prstInfo = NULL;

    if ( NULL == prstInfo )
    {
        return JET_errSuccess;
    }
    else if ( sizeof(JET_RSTINFO_W) != prstInfo->cbStruct )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Alloc( m_prstInfo = new JET_RSTINFO2_W );
    memset( m_prstInfo, 0, sizeof(JET_RSTINFO2_W) );
    m_prstInfo->cbStruct = sizeof(JET_RSTINFO2_W);

    CallR( m_autorstmap.ErrSet( prstInfo->rgrstmap, prstInfo->crstmap ) );

    Alloc( m_pInitCallbackWrapper = new InitCallbackWrapper( prstInfo->pfnStatus ) );

    m_prstInfo->rgrstmap = m_autorstmap;
    Assert( prstInfo->crstmap == m_autorstmap.Count() );
    m_prstInfo->crstmap = m_autorstmap.Count();

    m_prstInfo->lgposStop = prstInfo->lgposStop;
    m_prstInfo->logtimeStop = prstInfo->logtimeStop;
    m_prstInfo->pfnCallback = InitCallbackWrapper::PfnWrapper;
    m_prstInfo->pvCallbackContext = m_pInitCallbackWrapper;

HandleError:
    return err;
}

ERR CAutoRSTINFOW::ErrSet( const JET_RSTINFO2_A * prstInfo )
{
    ERR                 err         = JET_errSuccess;

    C_ASSERT( sizeof(JET_RSTINFO2_W) == sizeof(JET_RSTINFO2_A) );

    delete m_prstInfo;

    m_prstInfo = NULL;

    if ( NULL == prstInfo )
    {
        return JET_errSuccess;
    }
    else if ( sizeof(JET_RSTINFO2_A) != prstInfo->cbStruct )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Alloc( m_prstInfo = new JET_RSTINFO2_W );
    memset( m_prstInfo, 0, sizeof(JET_RSTINFO2_W) );
    m_prstInfo->cbStruct = sizeof(JET_RSTINFO2_W);

    CallR( m_autorstmap.ErrSet( prstInfo->rgrstmap, prstInfo->crstmap ) );

    m_prstInfo->rgrstmap = m_autorstmap;
    Assert( prstInfo->crstmap == m_autorstmap.Count() );
    m_prstInfo->crstmap = m_autorstmap.Count();

    m_prstInfo->lgposStop = prstInfo->lgposStop;
    m_prstInfo->logtimeStop = prstInfo->logtimeStop;
    m_prstInfo->pfnCallback = prstInfo->pfnCallback;
    m_prstInfo->pvCallbackContext = prstInfo->pvCallbackContext;

HandleError:
    return err;
}

CAutoRSTINFOW::~CAutoRSTINFOW()
{
    delete m_prstInfo;
    delete m_pInitCallbackWrapper;
}

JET_ERR JET_API JetInit( __inout_opt JET_INSTANCE *pinstance )
{
    return JetInit2( pinstance, NO_GRBIT );
}

JET_ERR JET_API JetInit2( __inout_opt JET_INSTANCE *pinstance, __in JET_GRBIT grbit )
{
    return JetInit3A( pinstance, NULL, grbit | JET_bitReplayMissingMapEntryDB );
}

JET_ERR JET_API JetInit3A(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_RSTINFO_A *    prstInfo,
    __in JET_GRBIT              grbit )
{
    ERR             err;
    CAutoRSTINFOW   lrstInfoW;

    CallR( lrstInfoW.ErrSet( prstInfo ) );

    return JetInit4W( pinstance, lrstInfoW, grbit );
}

JET_ERR JET_API JetInit3W(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_RSTINFO_W *    prstInfo,
    __in JET_GRBIT              grbit )
{
    ERR             err;
    CAutoRSTINFOW   lrstInfoW;

    CallR( lrstInfoW.ErrSet( prstInfo ) );

    return JetInit4W( pinstance, lrstInfoW, grbit );
}

JET_ERR JET_API JetInit4A(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_RSTINFO2_A *   prstInfo,
    __in JET_GRBIT              grbit )
{
    ERR             err;
    CAutoRSTINFOW   lrstInfoW;

    CallR( lrstInfoW.ErrSet( prstInfo ) );

    return JetInit4W( pinstance, lrstInfoW, grbit );
}

JET_ERR JET_API JetInit4W(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_RSTINFO2_W *   prstInfo,
    __in JET_GRBIT              grbit )
{
    JET_TRY( opInit, JetInitEx( pinstance, prstInfo, grbit ) );
}


LOCAL JET_ERR JetCreateInstanceEx(
    __out JET_INSTANCE *    pinstance,
    __in_opt JET_PCWSTR     wszInstanceName,
    __in_opt JET_PCWSTR     wszDisplayName )
{
    ERR         err     = JET_errSuccess;
    INST *      pinst;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p,0x%p[%s],0x%p[%s])",
            __FUNCTION__,
            pinstance,
            wszInstanceName,
            SzOSFormatStringW( wszInstanceName ),
            wszDisplayName,
            SzOSFormatStringW( wszDisplayName ) ) );

    if ( !pinstance )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    Assert ( pinstance );
    *pinstance = JET_instanceNil;

    INST::EnterCritInst();

    if ( RUNINSTGetMode() == runInstModeOneInst )
    {
        INST::LeaveCritInst();
        return ErrERRCheck( JET_errRunningInOneInstanceMode );
    }

    if ( RUNINSTGetMode() == runInstModeNoSet )
    {
        RUNINSTSetModeMultiInst();
    }

    err = ErrNewInst( &pinst, wszInstanceName, wszDisplayName, NULL );

    INST::LeaveCritInst();


    if (err >= JET_errSuccess)
    {
        Assert( !pinst->m_fJetInitialized );
        Assert ( g_cpinstInit >= 1 );
        Assert ( RUNINSTGetMode() == runInstModeMultiInst );
        Assert ( pinstance );
        *pinstance = (JET_INSTANCE) pinst;
    }

    return err;
}

LOCAL JET_ERR JetCreateInstanceExA(
    __out JET_INSTANCE *    pinstance,
    __in_opt JET_PCSTR      szInstanceName,
    __in_opt JET_PCSTR      szDisplayName )
{
    ERR             err;
    CAutoWSZ        lwszInstanceName;
    CAutoWSZ        lwszDisplayName;

    CallR( lwszInstanceName.ErrSet( szInstanceName ) );
    CallR( lwszDisplayName.ErrSet( szDisplayName ) );

    return JetCreateInstanceEx( pinstance, lwszInstanceName, lwszDisplayName );
}

JET_ERR JET_API JetCreateInstanceA(
    __out JET_INSTANCE *    pinstance,
    __in_opt JET_PCSTR      szInstanceName )
{
    return JetCreateInstance2A( pinstance, szInstanceName, NULL, NO_GRBIT );
}

JET_ERR JET_API JetCreateInstanceW(
    __out JET_INSTANCE *    pinstance,
    __in_opt JET_PCWSTR     wszInstanceName )
{
    return JetCreateInstance2W( pinstance, wszInstanceName, NULL, NO_GRBIT );
}


JET_ERR JET_API JetCreateInstance2A(
    __out JET_INSTANCE *    pinstance,
    __in_opt JET_PCSTR      szInstanceName,
    __in_opt JET_PCSTR      szDisplayName,
    __in JET_GRBIT          grbit )
{
    JET_TRY( opCreateInstance, JetCreateInstanceExA( pinstance, szInstanceName, szDisplayName ) );
}

JET_ERR JET_API JetCreateInstance2W(
    __out JET_INSTANCE *    pinstance,
    __in_opt JET_PCWSTR     wszInstanceName,
    __in_opt JET_PCWSTR     wszDisplayName,
    __in JET_GRBIT          grbit )
{
    JET_TRY( opCreateInstance, JetCreateInstanceEx( pinstance, wszInstanceName, wszDisplayName ) );
}

LOCAL JET_ERR JET_API JetGetInstanceMiscInfoEx(
    __in JET_INSTANCE               instance,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    ERR             err;
    ULONG           cbMin;
    APICALL_INST    apicall( opGetInstanceMiscInfo );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p,0x%x,0x%x)",
            __FUNCTION__,
            instance,
            pvResult,
            cbMax,
            InfoLevel) );

    if ( !apicall.FEnter( instance ) )
        return apicall.ErrResult();

    ProbeClientBuffer( pvResult, cbMax );

    switch( InfoLevel )
    {
        case JET_InstanceMiscInfoLogSignature:
            cbMin = sizeof(JET_SIGNATURE);
            break;
        case JET_InstanceMiscInfoCheckpoint:
            cbMin = sizeof(JET_CHECKPOINTINFO);
            break;
        case JET_InstanceMiscInfoRBS:
            cbMin = sizeof(JET_RBSINFOMISC);
            break;
        default:
            Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( cbMax >= cbMin )
    {
        INST *  pinst;

        Call( ErrFindPinst( instance, &pinst ) );

        switch ( InfoLevel )
        {
            case JET_InstanceMiscInfoLogSignature:
                *(JET_SIGNATURE *)pvResult = *(JET_SIGNATURE *)&( pinst->m_plog->SignLog() );
                break;

            case JET_InstanceMiscInfoCheckpoint:
            {
                LOG * const                 plog                = pinst->m_plog;
                JET_CHECKPOINTINFO * const  pcheckpointinfo     = (JET_CHECKPOINTINFO *)pvResult;

                if ( !plog->FLogDisabled() )
                {
                    plog->LockCheckpoint();

                    pcheckpointinfo->genMin = plog->LgposGetCheckpoint().le_lGeneration;
                    pcheckpointinfo->genMax = plog->LGGetCurrentFileGenNoLock( (LOGTIME *)&pcheckpointinfo->logtimeGenMaxCreate );

                    plog->UnlockCheckpoint();
                }
                else
                {
                    memset( pcheckpointinfo, 0, sizeof(JET_CHECKPOINTINFO) );
                }

                break;
            }

            case JET_InstanceMiscInfoRBS:
            {
                if ( pinst->m_prbs != NULL &&
                    !pinst->m_prbs->FInvalid() &&
                    pinst->m_prbs->FInitialized() &&
                    pinst->m_prbs->RBSFileHdr() != NULL )
                {
                    UtilLoadRBSinfomiscFromRBSfilehdr( ( JET_RBSINFOMISC* )pvResult, cbMax, pinst->m_prbs->RBSFileHdr() );
                }
            }

            break;

            default:
                Assert( fFalse );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        err = JET_errSuccess;
    }
    else
    {
        err = ErrERRCheck( JET_errBufferTooSmall );
    }

HandleError:
    apicall.LeaveAfterCall( err );
    return apicall.ErrResult();
}

JET_ERR JET_API JetGetInstanceMiscInfo(
    __in JET_INSTANCE               instance,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opGetInstanceMiscInfo, JetGetInstanceMiscInfoEx( instance, pvResult, cbMax, InfoLevel ) );
}


LOCAL JET_ERR JetRestoreInstanceEx(
    __in JET_INSTANCE   instance,
    __in JET_PCWSTR     wsz,
    __in_opt JET_PCWSTR wszDest,
    __in_opt JET_PFNSTATUS  pfn )
{
    APICALL_INST    apicall( opInit );
    ERR             err;
    BOOL            fAllocateInstance   = FINSTInvalid( instance ) ;
    INST            *pinst;
    INT             ipinst;
    CCriticalSection *pcritInst;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p[%s],0x%p[%s],0x%p)",
            __FUNCTION__,
            instance,
            wsz,
            SzOSFormatStringW( wsz ),
            wszDest,
            SzOSFormatStringW( wszDest ),
            pfn ) );

    if ( fAllocateInstance )
    {
        INST::EnterCritInst();
        if ( RUNINSTGetMode() == runInstModeNoSet )
        {
            Assert ( 0 == g_cpinstInit);
            RUNINSTSetModeMultiInst();
        }

        err = ErrNewInst( &pinst, wszRestoreInstanceName, NULL, &ipinst );
        Assert( ( err < JET_errSuccess && pinst == NULL ) || !pinst->m_fJetInitialized );
        INST::LeaveCritInst();
        CallR( err );
    }
    else
    {
        CallR( ErrFindPinst( instance, &pinst, &ipinst ) );
    }

    pcritInst = &g_critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
    pcritInst->Enter();

    if ( !apicall.FEnterForInit( pinst ) )
    {
        pcritInst->Leave();
        if ( fAllocateInstance )
        {
            ErrTermAlloc( (JET_INSTANCE) pinst );
            apicall.LeaveAfterTerm( err );
        }
        return apicall.ErrResult();
    }

    pinst->APILock( pinst->fAPIRestoring );
    pcritInst->Leave();

    Assert( !pinst->m_fJetInitialized );
    Assert( !pinst->m_fTermInProgress );

    err = ErrInit(  pinst,
                    fTrue,
                    NO_GRBIT );

    Assert( JET_errAlreadyInitialized != err );

    Assert( !pinst->m_fJetInitialized );
    Assert( !pinst->m_fTermInProgress );

    if ( err >= 0 )
    {
        pinst->m_fJetInitialized = fTrue;

        InitCallbackWrapper initCallbackWrapper(pfn);
        err = ErrIsamRestore( (JET_INSTANCE) pinst, wsz, wszDest, InitCallbackWrapper::PfnWrapper, &initCallbackWrapper );



        pinst->m_fJetInitialized = fFalse;
    }

    pinst->APIUnlock( pinst->fAPIRestoring );

    apicall.LeaveAfterCall( err );
    err = apicall.ErrResult();

    if ( fAllocateInstance )
    {
        FreePinst( pinst );
    }

    Assert( !Ptls()->fInSoftStart );
    return err;
}

LOCAL JET_ERR JetRestoreInstanceExA(
    __in JET_INSTANCE   instance,
    __in JET_PCSTR      sz,
    __in_opt JET_PCSTR  szDest,
    __in_opt JET_PFNSTATUS  pfn )
{
    ERR             err;
    CAutoWSZPATH    lwsz;
    CAutoWSZPATH    lwszDest;

    CallR( lwsz.ErrSet( sz ) );
    CallR( lwszDest.ErrSet( szDest ) );
    return JetRestoreInstanceEx( instance, lwsz, lwszDest, pfn );
}

JET_ERR JET_API JetRestoreInstanceA(
    __in JET_INSTANCE   instance,
    __in JET_PCSTR      sz,
    __in_opt JET_PCSTR  szDest,
    __in_opt JET_PFNSTATUS  pfn )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opRestoreInstance, JetRestoreInstanceExA( instance, sz, szDest, pfn ) );
}

JET_ERR JET_API JetRestoreInstanceW(
    __in JET_INSTANCE   instance,
    __in JET_PCWSTR     wsz,
    __in_opt JET_PCWSTR wszDest,
    __in_opt JET_PFNSTATUS  pfn )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opRestoreInstance, JetRestoreInstanceEx( instance, wsz, wszDest, pfn ) );
}

LOCAL JET_ERR JetTermEx( __in JET_INSTANCE instance, __in JET_GRBIT grbit )
{
    OSTrace( JET_tracetagAPI, OSFormat( "Start %s(0x%Ix,0x%x)", __FUNCTION__, instance, grbit ) );
    COSTraceTrackErrors trackerrors( __FUNCTION__ );
    JET_ERR err = ErrTermComplete( instance, grbit );
    return err;
}

JET_ERR JET_API JetTerm( __in JET_INSTANCE instance )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opTerm, JetTermEx( instance, JET_bitTermAbrupt ) );
}
JET_ERR JET_API JetTerm2( __in JET_INSTANCE instance, __in JET_GRBIT grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opTerm, JetTermEx( instance, grbit ) );
}

JET_ERR JET_API JetStopServiceInstanceExOld( __in JET_INSTANCE instance )
{
    ERR     err;
    INST    *pinst;

    OSTrace( JET_tracetagAPI, OSFormat( "Start %s(0x%Ix)", __FUNCTION__, instance ) );

    CallR( ErrFindPinst( instance, &pinst ) );


    DBMScanStopAllScansForInst( pinst );
    OLDTermInst( pinst );
    OLD2TermInst( pinst );
    PagePatching::TermInst( pinst );

    pinst->m_pver->VERSignalCleanup();

    CallS( Param( pinst, JET_paramCheckpointDepthMax )->Set( pinst, ppibNil, 16384, NULL ) );

    pinst->m_fStopJetService = fTrue;
    return JET_errSuccess;
}


JET_ERR JET_API JetStopServiceInstanceEx( __in JET_INSTANCE instance, _In_ JET_GRBIT grbit )
{
    ERR     err = JET_errSuccess;
    INST *  pinst;

    OSTrace( JET_tracetagAPI, OSFormat( "Start %s(0x%Ix, 0x%x)", __FUNCTION__, instance, grbit ) );


    CallR( ErrFindPinst( instance, &pinst ) );

    const JET_GRBIT bitStopServiceAllInternal = 0x1;
    C_ASSERT( JET_bitStopServiceAll != bitStopServiceAllInternal );
    C_ASSERT( JET_bitStopServiceBackgroundUserTasks != bitStopServiceAllInternal );


    const JET_GRBIT mskValidIndependentResumableBits = ( JET_bitStopServiceBackgroundUserTasks |
                                                            JET_bitStopServiceQuiesceCaches );


    const JET_GRBIT mskValidIndependentUserBits = ( JET_bitStopServiceBackgroundUserTasks |
                                                    JET_bitStopServiceQuiesceCaches |
                                                    JET_bitStopServiceStopAndEmitLog |
                                                    JET_bitStopServiceResume );
    if ( 0 != ( grbit & ~mskValidIndependentUserBits ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    if ( grbit & JET_bitStopServiceResume )
    {

        if ( grbit & ~JET_bitStopServiceResume )
        {
            grbit = grbit & ~JET_bitStopServiceResume;
        }
        else
        {
            Assert( JET_bitStopServiceAll == ( grbit & ~JET_bitStopServiceResume ) );
            grbit = pinst->m_grbitStopped;
        }

        if ( ( grbit & ~mskValidIndependentResumableBits ) & mskValidIndependentUserBits )
        {
            Error( ErrERRCheck( JET_errInvalidGrbit ) );
        }


        if ( grbit & ~mskValidIndependentResumableBits )
        {
            Error( ErrERRCheck( JET_errClientRequestToStopJetService ) );
        }


        OnDebug( JET_GRBIT grbitCheck = grbit );


        if ( grbit & JET_bitStopServiceQuiesceCaches )
        {

            pinst->m_fCheckpointQuiesce = fFalse;




            pinst->m_grbitStopped &= (~JET_bitStopServiceQuiesceCaches);
            OnDebug( grbitCheck &= (~JET_bitStopServiceQuiesceCaches) );
        }


        if ( grbit & JET_bitStopServiceBackgroundUserTasks )
        {
            FMP::EnterFMPPoolAsWriter();
            FMP *   pfmpCurr = NULL;
            if ( pinst && pinst->m_fJetInitialized )
            {
                for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
                {
                    const IFMP ifmp = pinst->m_mpdbidifmp[ dbidT ];
                    if ( ifmp >= g_ifmpMax )
                    {
                        continue;
                    }

                    if ( g_rgfmp[ifmp].FReadOnlyAttach() )
                    {
                        continue;
                    }

                    pfmpCurr = &g_rgfmp[ifmp];
                    pfmpCurr->RwlDetaching().EnterAsReader();

                    pfmpCurr->ResetFDontRegisterOLD2Tasks();

                    pfmpCurr->RwlDetaching().LeaveAsReader();
                    pfmpCurr = NULL;
                }
            }
            FMP::LeaveFMPPoolAsWriter();

            pinst->m_grbitStopped &= (~JET_bitStopServiceBackgroundUserTasks);
            OnDebug( grbitCheck &= (~JET_bitStopServiceBackgroundUserTasks) );
        }

        pinst->m_tickStopCanceled = TickOSTimeCurrent();

        Assert( grbitCheck == 0 );
    }
    else if ( grbit & JET_bitStopServiceStopAndEmitLog )
    {
        if ( grbit != JET_bitStopServiceStopAndEmitLog )
        {
            Error( ErrERRCheck( JET_errInvalidGrbit ) );
        }

        err = pinst->m_plog->ErrLGStopAndEmitLog();
    }
    else
    {
        OnDebug( JET_GRBIT grbitCheck = 0 );

        pinst->m_tickStopped = TickOSTimeCurrent();

        if( grbit == JET_bitStopServiceAll )
        {
            grbit = bitStopServiceAllInternal;
        }

        if ( grbit & bitStopServiceAllInternal )
        {
            grbit |= ( JET_bitStopServiceBackgroundUserTasks | JET_bitStopServiceQuiesceCaches );
        }

        OnDebug( grbitCheck = ( grbit & ~JET_bitStopServiceAll ) );


        if ( grbit & bitStopServiceAllInternal )
        {

            DBMScanStopAllScansForInst( pinst );
            OLDTermInst( pinst );
            OLD2TermInst( pinst );
            PagePatching::TermInst( pinst );

        }

        if ( grbit & JET_bitStopServiceBackgroundUserTasks )
        {

            FMP::EnterFMPPoolAsWriter();
            FMP *   pfmpCurr = NULL;
            if ( pinst && pinst->m_fJetInitialized )
            {
                for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
                {
                    const IFMP ifmp = pinst->m_mpdbidifmp[ dbidT ];
                    if ( ifmp >= g_ifmpMax )
                    {
                        continue;
                    }

                    if ( g_rgfmp[ifmp].FReadOnlyAttach() )
                    {
                        continue;
                    }

                    pfmpCurr = &g_rgfmp[ifmp];
                    pfmpCurr->RwlDetaching().EnterAsReader();

                    pfmpCurr->SetFDontRegisterOLD2Tasks();

                    pfmpCurr->RwlDetaching().LeaveAsReader();
                    pfmpCurr = NULL;
                }
            }
            FMP::LeaveFMPPoolAsWriter();

            pinst->m_grbitStopped |= JET_bitStopServiceBackgroundUserTasks;
            OnDebug( grbitCheck &= ~JET_bitStopServiceBackgroundUserTasks );
        }

        if ( grbit & JET_bitStopServiceQuiesceCaches )
        {
            pinst->m_pver->VERSignalCleanup();

            PIB pibFake;
            pibFake.m_pinst = pinst;
            (void)ErrLGWaitForWrite( &pibFake, &lgposMax );



            pinst->m_fCheckpointQuiesce = fTrue;


            (void)ErrIOUpdateCheckpoints( pinst );

            pinst->m_grbitStopped |= JET_bitStopServiceQuiesceCaches;
            OnDebug( grbitCheck &= ~JET_bitStopServiceQuiesceCaches );
        }

        if ( grbit & bitStopServiceAllInternal )
        {

            pinst->m_fStopJetService = fTrue;

            pinst->m_grbitStopped |= bitStopServiceAllInternal;
            OnDebug( grbitCheck &= ~bitStopServiceAllInternal );
        }

        Assert( grbitCheck == 0 );
    }

HandleError:

    return err;
}

JET_ERR JET_API JetStopServiceInstance( __in JET_INSTANCE instance )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opStopServiceInstance, JetStopServiceInstanceExOld( instance ) );
}

JET_ERR JET_API JetStopServiceInstance2( _In_ JET_INSTANCE instance, _In_ const JET_GRBIT grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opStopServiceInstance, JetStopServiceInstanceEx( instance, grbit ) );
}

JET_ERR JET_API JetStopService()
{
    ERR     err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetStopServiceInstance( (JET_INSTANCE)g_rgpinst[0] );
}


LOCAL JET_ERR JetStopBackupInstanceEx( __in JET_INSTANCE instance )
{
    ERR     err;
    INST    *pinst;

    OSTrace( JET_tracetagAPI, OSFormat( "Start %s(0x%Ix)", __FUNCTION__, instance ) );

    CallR( ErrFindPinst( instance, &pinst ) );

    if ( pinst->m_plog )
    {
        pinst->m_pbackup->BKSetStopBackup( fTrue );
    }

    return JET_errSuccess;
}
JET_ERR JET_API JetStopBackupInstance( __in JET_INSTANCE instance )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opStopBackupInstance, JetStopBackupInstanceEx( instance ) );
}
JET_ERR JET_API JetStopBackup()
{
    ERR     err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetStopBackupInstance( (JET_INSTANCE)g_rgpinst[0] ) ;
}

LOCAL JET_ERR JetFreeBufferEx(
    _Pre_notnull_ char * pbBuf )
{
    OSTrace( JET_tracetagAPI, OSFormat( "Start %s(0x%p)", __FUNCTION__, pbBuf ) );

    OSMemoryHeapFree( (void* const)pbBuf );
    return JET_errSuccess;
};

JET_ERR JET_API JetFreeBuffer(
    _Pre_notnull_ char * pbBuf )
{
    JET_TRY( opFreeBuffer, JetFreeBufferEx( pbBuf ) );
}



LOCAL JET_ERR JetGetInstanceInfoEx(
    __out ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo )
{
    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p,0x%p)",
            __FUNCTION__,
            pcInstanceInfo,
            paInstanceInfo ) );
    return ErrIsamGetInstanceInfo( pcInstanceInfo, paInstanceInfo, NULL );
}


class CAutoINSTANCE_INFOW
{
public:
    CAutoINSTANCE_INFOW():m_aInstanceInfo( NULL ), m_cInstanceInfo( 0 ) {}
    ~CAutoINSTANCE_INFOW();

    ERR ErrSet( ULONG cInstanceInfo, JET_INSTANCE_INFO_W * paInstanceInfo );
    ERR ErrGet( ULONG *pcInstanceInfo, JET_INSTANCE_INFO_A ** paInstanceInfo );

private:
    JET_INSTANCE_INFO_W *   m_aInstanceInfo;
    ULONG           m_cInstanceInfo;
};

ERR CAutoINSTANCE_INFOW::ErrSet( ULONG cInstanceInfo, JET_INSTANCE_INFO_W * paInstanceInfo )
{

    if ( ( NULL == paInstanceInfo && 0 != cInstanceInfo ) ||
        ( NULL != paInstanceInfo && 0 == cInstanceInfo ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( m_aInstanceInfo )
    {
        JetFreeBufferEx( (char*)m_aInstanceInfo );
    }

    m_aInstanceInfo = paInstanceInfo;
    m_cInstanceInfo = cInstanceInfo;

    return JET_errSuccess;
}

ERR CAutoINSTANCE_INFOW::ErrGet( ULONG *pcInstanceInfo, JET_INSTANCE_INFO_A ** paInstanceInfo )
{
    ERR                     err             = JET_errSuccess;
    void *                  lpvNew          = NULL;
    UINT            iInst;
    UINT            iDb;
    size_t                  cDataPtrs;
    size_t                  cbTotal         = 0;
    size_t                  cchPartial      = 0;

    if ( NULL == pcInstanceInfo || NULL == paInstanceInfo )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }


    cbTotal = m_cInstanceInfo * sizeof(JET_INSTANCE_INFO_A);
    cDataPtrs = 0;

    for( iInst = 0 ; iInst < m_cInstanceInfo; iInst++ )
    {

        if ( m_aInstanceInfo[iInst].szInstanceName )
        {
            err = ErrOSSTRUnicodeToAscii( m_aInstanceInfo[iInst].szInstanceName, NULL, 0, &cchPartial );
            if ( err != JET_errBufferTooSmall )
            {
                Call( err );
            }
            cbTotal += sizeof( CHAR ) * cchPartial;
        }

        for( iDb = 0; iDb < m_aInstanceInfo[iInst].cDatabases; iDb++ )
        {

            Assert( m_aInstanceInfo[iInst].szDatabaseFileName[iDb] );
            err = ErrOSSTRUnicodeToAscii( m_aInstanceInfo[iInst].szDatabaseFileName[iDb], NULL, 0, &cchPartial );
            if ( err != JET_errBufferTooSmall )
            {
                Call( err );
            }
            cbTotal += sizeof( CHAR ) * cchPartial;

            if ( m_aInstanceInfo[iInst].szDatabaseDisplayName[iDb] )
            {
                err = ErrOSSTRUnicodeToAscii( m_aInstanceInfo[iInst].szDatabaseDisplayName[iDb], NULL, 0, &cchPartial );
                if ( err != JET_errBufferTooSmall )
                {
                    Call( err );
                }
                cbTotal += sizeof( CHAR ) * cchPartial;
            }
        }
        cDataPtrs += 3 * m_aInstanceInfo[iInst].cDatabases;
        cbTotal += 3 * m_aInstanceInfo[iInst].cDatabases * sizeof(CHAR*);
    }
    err = JET_errSuccess;

    Alloc( lpvNew = PvOSMemoryHeapAlloc( cbTotal ) );
    memset( lpvNew, 0, cbTotal );

    char * lpvCurrent = ((char*)lpvNew) + m_cInstanceInfo * sizeof(JET_INSTANCE_INFO_A);
    char * szCurrent = (char*)lpvCurrent + cDataPtrs * sizeof(char*);
    cbTotal -= m_cInstanceInfo * sizeof(JET_INSTANCE_INFO_A);

    for( iInst = 0; iInst < m_cInstanceInfo; iInst++ )
    {
        ((JET_INSTANCE_INFO_A*)lpvNew)[iInst].hInstanceId = m_aInstanceInfo[iInst].hInstanceId;
        ((JET_INSTANCE_INFO_A*)lpvNew)[iInst].cDatabases = m_aInstanceInfo[iInst].cDatabases;

        ((JET_INSTANCE_INFO_A*)lpvNew)[iInst].szDatabaseFileName = (CHAR**)lpvCurrent;
        lpvCurrent += m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * );
        if ( cbTotal < ( m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * ) ) )
        {
            AssertSz( fFalse,"CAutoINSTANCE_INFOW::ErrGet.  Buffer arithmetic validation failure." );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        cbTotal -= m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * );

        ((JET_INSTANCE_INFO_A*)lpvNew)[iInst].szDatabaseDisplayName = (CHAR**)lpvCurrent;
        lpvCurrent += m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * );
        if ( cbTotal < ( m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * ) ) )
        {
            AssertSz( fFalse,"CAutoINSTANCE_INFOW::ErrGet.  Buffer arithmetic validation failure." );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        cbTotal -= m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * );

        ((JET_INSTANCE_INFO_A*)lpvNew)[iInst].szDatabaseSLVFileName_Obsolete = NULL;
        lpvCurrent += m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * );
        if ( cbTotal < ( m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * ) ) )
        {
            AssertSz( fFalse,"CAutoINSTANCE_INFOW::ErrGet.  Buffer arithmetic validation failure." );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        cbTotal -= m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * );

        if ( m_aInstanceInfo[iInst].szInstanceName )
        {
            Call( ErrOSSTRUnicodeToAscii( m_aInstanceInfo[iInst].szInstanceName, szCurrent, cbTotal / sizeof( CHAR ), &cchPartial ) );
            if( cbTotal < ( cchPartial * sizeof( CHAR ) ) )
            {
                AssertSz( fFalse,"CAutoINSTANCE_INFOW::ErrGet.  Buffer arithmetic validation failure." );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            ((JET_INSTANCE_INFO_A*)lpvNew)[iInst].szInstanceName = szCurrent;
            szCurrent += cchPartial;
            cbTotal -= ( cchPartial * sizeof( CHAR ) );
        }

        for( iDb = 0; iDb < m_aInstanceInfo[iInst].cDatabases; iDb++ )
        {
            Call( ErrOSSTRUnicodeToAscii( m_aInstanceInfo[iInst].szDatabaseFileName[iDb], szCurrent, cbTotal / sizeof( CHAR ), &cchPartial ) );
            if( cbTotal < ( cchPartial * sizeof( CHAR ) ) )
            {
                AssertSz( fFalse,"CAutoINSTANCE_INFOW::ErrGet.  Buffer arithmetic validation failure." );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            ((JET_INSTANCE_INFO_A*)lpvNew)[iInst].szDatabaseFileName[iDb] = szCurrent;
            szCurrent += cchPartial;
            cbTotal -= ( cchPartial * sizeof( CHAR ) );

            if ( m_aInstanceInfo[iInst].szDatabaseDisplayName[iDb] )
            {
                Call( ErrOSSTRUnicodeToAscii( m_aInstanceInfo[iInst].szDatabaseDisplayName[iDb], szCurrent, cbTotal / sizeof( CHAR ), &cchPartial ) );
                if( cbTotal < ( cchPartial * sizeof( CHAR ) ) )
                {
                    AssertSz( fFalse,"CAutoINSTANCE_INFOW::ErrGet.  Buffer arithmetic validation failure." );
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }

                ((JET_INSTANCE_INFO_A*)lpvNew)[iInst].szDatabaseDisplayName[iDb] = szCurrent;
                szCurrent += cchPartial;
                cbTotal -= ( cchPartial * sizeof( CHAR ) );
            }
        }
    }

    Assert( cbTotal == 0 );

    *paInstanceInfo = (JET_INSTANCE_INFO_A*)lpvNew;
    *pcInstanceInfo = m_cInstanceInfo;
    lpvNew = NULL;

HandleError:
    OSMemoryHeapFree( lpvNew );
    return err;
}

CAutoINSTANCE_INFOW::~CAutoINSTANCE_INFOW()
{
    if ( m_aInstanceInfo )
    {
        JetFreeBufferEx( (char*)m_aInstanceInfo );
    }
}


LOCAL JET_ERR JetGetInstanceInfoExA(
    __out ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo )
{
    ERR                     err             = JET_errSuccess;
    JET_INSTANCE_INFO_W*    laInstanceInfoW = NULL;
    ULONG           lcInstanceInfo  = 0;
    CAutoINSTANCE_INFOW autoInstanceInfoW;

    Call( JetGetInstanceInfoEx( &lcInstanceInfo, &laInstanceInfoW ) );

    Call( autoInstanceInfoW.ErrSet( lcInstanceInfo, laInstanceInfoW ) );
    laInstanceInfoW = NULL;
    lcInstanceInfo = 0;

    Call( autoInstanceInfoW.ErrGet(pcInstanceInfo, paInstanceInfo ) );

HandleError:

    if ( laInstanceInfoW )
    {
        JetFreeBufferEx( (char*)laInstanceInfoW );
    }
    return err;
}

JET_ERR JET_API JetGetInstanceInfoA(
    __out ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo )
{
    JET_TRY( opGetInstanceInfo, JetGetInstanceInfoExA( pcInstanceInfo, paInstanceInfo ) );
}

JET_ERR JET_API JetGetInstanceInfoW(
    __out ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo )
{
    JET_TRY( opGetInstanceInfo, JetGetInstanceInfoEx( pcInstanceInfo, paInstanceInfo ) );
}

LOCAL JET_ERR JetOSSnapshotPrepareEx(
    __out JET_OSSNAPID *    psnapId,
    __in const JET_GRBIT    grbit )
{
    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p,0x%x)",
            __FUNCTION__,
            psnapId,
            grbit ) );
    return ErrIsamOSSnapshotPrepare( psnapId, grbit );
}

JET_ERR JET_API JetOSSnapshotPrepare(
    __out JET_OSSNAPID *    psnapId,
    __in const JET_GRBIT    grbit )
{
    JET_TRY( opOSSnapshotPrepare, JetOSSnapshotPrepareEx( psnapId, grbit ) );
}


LOCAL JET_ERR JetOSSnapshotFreezeEx(
    __in const JET_OSSNAPID                                         snapId,
    __out ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo,
    __in const JET_GRBIT                                            grbit )
{
    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p,0x%p,0x%x)",
            __FUNCTION__,
            snapId,
            pcInstanceInfo,
            paInstanceInfo,
            grbit ) );
    return ErrIsamOSSnapshotFreeze( snapId, pcInstanceInfo, paInstanceInfo, grbit );
}

LOCAL JET_ERR JetOSSnapshotFreezeExA(
    __in const JET_OSSNAPID                                         snapId,
    __out ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo,
    __in const JET_GRBIT                                            grbit )
{
    ERR                     err             = JET_errSuccess;
    JET_INSTANCE_INFO_W *   laInstanceInfoW     = NULL;
    ULONG           lcInstanceInfo  = 0;
    CAutoINSTANCE_INFOW     autoInstanceInfoW;

    if ( NULL == pcInstanceInfo || NULL == paInstanceInfo )
    {
        Call( ErrERRCheck(JET_errInvalidParameter ) );
    }

    Call( JetOSSnapshotFreezeEx( snapId, &lcInstanceInfo, &laInstanceInfoW, grbit ) );

    Call( autoInstanceInfoW.ErrSet( lcInstanceInfo, laInstanceInfoW ) );
    laInstanceInfoW = NULL;
    lcInstanceInfo = 0;

    Call( autoInstanceInfoW.ErrGet(pcInstanceInfo, paInstanceInfo ) );

HandleError:

    if ( laInstanceInfoW )
    {
        JetFreeBufferEx( (char*)laInstanceInfoW );
    }
    return err;
}

JET_ERR JET_API JetOSSnapshotFreezeA(
    __in const JET_OSSNAPID                                         snapId,
    __out ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo,
    __in const JET_GRBIT                                            grbit )
{
    JET_TRY( opOSSnapshotFreeze, JetOSSnapshotFreezeExA( snapId, pcInstanceInfo, paInstanceInfo, grbit ) );
}

JET_ERR JET_API JetOSSnapshotFreezeW(
    __in const JET_OSSNAPID                                         snapId,
    __out ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo,
    __in const JET_GRBIT                                            grbit )
{
    JET_TRY( opOSSnapshotFreeze, JetOSSnapshotFreezeEx( snapId, pcInstanceInfo, paInstanceInfo, grbit ) );
}

LOCAL JET_ERR JetOSSnapshotThawEx( __in const JET_OSSNAPID snapId, __in const JET_GRBIT grbit )
{
    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x)",
            __FUNCTION__,
            snapId,
            grbit ) );
    return ErrIsamOSSnapshotThaw( snapId, grbit );
}

JET_ERR JET_API JetOSSnapshotThaw( __in const JET_OSSNAPID snapId, __in const JET_GRBIT grbit )
{
    JET_TRY( opOSSnapshotThaw, JetOSSnapshotThawEx( snapId, grbit ) );
}


LOCAL JET_ERR JetOSSnapshotAbortEx( __in const JET_OSSNAPID snapId, __in const JET_GRBIT grbit )
{
    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x)",
            __FUNCTION__,
            snapId,
            grbit ) );
    return ErrIsamOSSnapshotAbort( snapId, grbit );
}

JET_ERR JET_API JetOSSnapshotAbort( __in const JET_OSSNAPID snapId, __in const JET_GRBIT grbit )
{
    JET_TRY( opOSSnapshotAbort, JetOSSnapshotAbortEx( snapId, grbit ) );
}

INLINE JET_ERR JetOSSnapshotPrepareInstanceEx( __in JET_OSSNAPID snapId, __in const JET_INSTANCE instance, __in const JET_GRBIT grbit )
{
    APICALL_INST    apicall( opOSSnapPrepareInstance );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x)",
            __FUNCTION__,
            snapId,
            instance,
            grbit ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamOSSnapshotPrepareInstance( snapId, apicall.Pinst(), grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetOSSnapshotPrepareInstance( __in JET_OSSNAPID snapId, __in const JET_INSTANCE instance, __in const JET_GRBIT grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opOSSnapPrepareInstance, JetOSSnapshotPrepareInstanceEx( snapId, instance, grbit ) );
}



INLINE JET_ERR JetOSSnapshotTruncateLogEx( __in const JET_OSSNAPID snapId, __in const JET_GRBIT grbit )
{
    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x)",
            __FUNCTION__,
            snapId,
            grbit ) );
    return ErrIsamOSSnapshotTruncateLog( snapId, NULL, grbit );
}

JET_ERR JET_API JetOSSnapshotTruncateLog( __in const JET_OSSNAPID snapId, __in const JET_GRBIT grbit )
{
    JET_TRY( opOSSnapshotTruncateLog, JetOSSnapshotTruncateLogEx( snapId, grbit ) );
}

INLINE JET_ERR JetOSSnapshotTruncateLogInstanceEx( __in const JET_OSSNAPID snapId, __in const JET_INSTANCE instance, __in const JET_GRBIT grbit )
{
    APICALL_INST    apicall( opOSSnapTruncateLogInstance );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,0x%x)",
            __FUNCTION__,
            snapId,
            instance,
            grbit ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamOSSnapshotTruncateLog( snapId, apicall.Pinst(), grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetOSSnapshotTruncateLogInstance( __in const JET_OSSNAPID snapId, __in const JET_INSTANCE instance, __in const JET_GRBIT grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opOSSnapTruncateLogInstance, JetOSSnapshotTruncateLogInstanceEx( snapId, instance, grbit ) );
}

INLINE JET_ERR JetOSSnapshotGetFreezeInfoEx(
    __in const JET_OSSNAPID                                         snapId,
    __out ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo,
    __in const JET_GRBIT                                            grbit )
{
    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p,0x%p,0x%x)",
            __FUNCTION__,
            snapId,
            pcInstanceInfo,
            paInstanceInfo,
            grbit ) );
    return ErrIsamOSSnapshotGetFreezeInfo( snapId, pcInstanceInfo, paInstanceInfo, grbit );
}

LOCAL JET_ERR JetOSSnapshotGetFreezeInfoExA(
    __in const JET_OSSNAPID                                         snapId,
    __out ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo,
    __in const JET_GRBIT                                            grbit )
{
    ERR                     err             = JET_errSuccess;
    JET_INSTANCE_INFO_W *   laInstanceInfoW     = NULL;
    ULONG           lcInstanceInfo  = 0;
    CAutoINSTANCE_INFOW     autoInstanceInfoW;

    Call( JetOSSnapshotGetFreezeInfoEx( snapId, &lcInstanceInfo, &laInstanceInfoW, grbit ) );

    Call( autoInstanceInfoW.ErrSet( lcInstanceInfo, laInstanceInfoW ) );
    laInstanceInfoW = NULL;
    lcInstanceInfo = 0;

    Call( autoInstanceInfoW.ErrGet(pcInstanceInfo, paInstanceInfo ) );

HandleError:

    if ( laInstanceInfoW )
    {
        JetFreeBufferEx( (char*)laInstanceInfoW );
    }
    return err;
}


JET_ERR JET_API JetOSSnapshotGetFreezeInfoA(
    __in const JET_OSSNAPID                                         snapId,
    __out ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo,
    __in const JET_GRBIT                                            grbit )
{
    JET_TRY( opOSSnapshotGetFreezeInfo, JetOSSnapshotGetFreezeInfoExA( snapId, pcInstanceInfo, paInstanceInfo, grbit ) );
}

JET_ERR JET_API JetOSSnapshotGetFreezeInfoW(
    __in const JET_OSSNAPID                                         snapId,
    __out ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo,
    __in const JET_GRBIT                                            grbit )
{
    JET_TRY( opOSSnapshotGetFreezeInfo, JetOSSnapshotGetFreezeInfoEx( snapId, pcInstanceInfo, paInstanceInfo, grbit ) );
}


INLINE JET_ERR JetOSSnapshotEndEx( __in const JET_OSSNAPID snapId, __in const JET_GRBIT grbit )
{
    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x)",
            __FUNCTION__,
            snapId,
            grbit ) );

    return ErrIsamOSSnapshotEnd( snapId, grbit );
}

JET_ERR JET_API JetOSSnapshotEnd( __in const JET_OSSNAPID snapId, __in const JET_GRBIT grbit )
{
    JET_TRY( opOSSnapshotEnd, JetOSSnapshotEndEx( snapId, grbit ) );
}


JET_ERR JetConfigureProcessForCrashDumpEx( __in const JET_GRBIT grbit )
{
    JET_ERR err = JET_errSuccess;


    if (    grbit == 0 ||
            ( grbit & ~(    JET_bitDumpMinimum |
                            JET_bitDumpMaximum |
                            JET_bitDumpCacheMinimum |
                            JET_bitDumpCacheMaximum |
                            JET_bitDumpCacheIncludeDirtyPages |
                            JET_bitDumpCacheIncludeCachedPages |
                            JET_bitDumpCacheIncludeCorruptedPages |
                            JET_bitDumpCacheNoDecommit |
                            JET_bitDumpUnitTest ) ) != 0 )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    if ( grbit & JET_bitDumpMaximum )
    {
        Call( ErrBFConfigureProcessForCrashDump( grbit | JET_bitDumpCacheMaximum ) );
    }
    else if ( grbit & JET_bitDumpMinimum )
    {
        Call( ErrBFConfigureProcessForCrashDump( grbit | JET_bitDumpCacheMinimum ) );
    }
    else
    {
        Call( ErrBFConfigureProcessForCrashDump( grbit ) );
    }

HandleError:
    return err;
}

JET_ERR JET_API JetConfigureProcessForCrashDump( __in const JET_GRBIT grbit )
{
    JET_ERR err = JET_errSuccess;

    TRY
    {
        err = JetConfigureProcessForCrashDumpEx( grbit );
    }
    EXCEPT( efaExecuteHandler )
    {
        err = ErrERRCheck( JET_errInternalError );
    }

    return err;
}


JET_ERR ErrTestHookCorruptOfflineFile( const JET_TESTHOOKCORRUPT * const pcorruptDatabaseFile )
{
    JET_ERR         err     = JET_errSuccess;
    ULONG           cbPageSize  = 0;
    BYTE *          pbPageImage = NULL;
    IFileSystemAPI *    pfsapi      = NULL;
    IFileAPI *      pfapi       = NULL;
    JET_TESTHOOKCORRUPT corruptPageImage= { sizeof(corruptPageImage), 0 };

    Assert( pcorruptDatabaseFile->grbit & JET_bitTestHookCorruptDatabaseFile );

    CallR( ErrOSUInit() );

    Assert( pcorruptDatabaseFile->cbStruct >= ( OffsetOf( JET_TESTHOOKCORRUPT, CorruptDatabaseFile.iSubTarget ) + sizeof( __int64 ) ) );
    AssertSz( pcorruptDatabaseFile->CorruptDatabaseFile.pgnoTarget != JET_pgnoTestHookCorruptRandom, "NYI" );

    Alloc( pbPageImage = (BYTE * )PvOSMemoryPageAlloc( g_cbPageMax, NULL ) );
    Call( ErrOSFSCreate( g_pfsconfigGlobal, &pfsapi ) );

    Call( pfsapi->ErrFileOpen(  pcorruptDatabaseFile->CorruptDatabaseFile.wszDatabaseFilePath,
                                ( BoolParam( JET_paramEnableFileCache ) ?
                                    IFileAPI::fmfCached :
                                    IFileAPI::fmfNone ),
                                &pfapi ) );

    Call( ErrUtilReadShadowedHeader( pinstNil, pfsapi, pfapi, pbPageImage, g_cbPageMax, OffsetOf( DBFILEHDR_FIX, le_cbPageSize ), urhfNoFailOnPageMismatch ) );
    cbPageSize = ((DBFILEHDR*)pbPageImage)->le_cbPageSize;

    Call( pfapi->ErrIORead( *TraceContextScope( iorpDirectAccessUtil ),
                OffsetOfPgno( (PGNO)pcorruptDatabaseFile->CorruptDatabaseFile.pgnoTarget ),
                cbPageSize,
                pbPageImage,
                qosIONormal ) );

    corruptPageImage.grbit = ( (~JET_bitTestHookCorruptDatabaseFile) & pcorruptDatabaseFile->grbit ) | JET_bitTestHookCorruptDatabasePageImage;
    corruptPageImage.CorruptDatabasePageImage.pbPageImageTarget = (JET_API_PTR)pbPageImage;
    corruptPageImage.CorruptDatabasePageImage.cbPageImage = cbPageSize;
    corruptPageImage.CorruptDatabasePageImage.pgnoTarget = pcorruptDatabaseFile->CorruptDatabaseFile.pgnoTarget;
    corruptPageImage.CorruptDatabasePageImage.iSubTarget = pcorruptDatabaseFile->CorruptDatabaseFile.iSubTarget;
    Call( JetTestHook( opTestHookCorrupt, &corruptPageImage ) );

    Call( pfapi->ErrIOWrite( *TraceContextScope( iorpCorruptionTestHook ), OffsetOfPgno( (PGNO)pcorruptDatabaseFile->CorruptDatabaseFile.pgnoTarget ), cbPageSize, pbPageImage, qosIONormal ) );

HandleError:

    if ( pbPageImage != NULL )
    {
        OSMemoryPageFree( pbPageImage );
    }

    (void)ErrUtilFlushFileBuffers( pfapi, iofrUtility );
    delete pfapi;
    delete pfsapi;

    OSUTerm();

    return err;
}

JET_ERR ErrTESTHOOKAlterDatabaseFileHeader( const JET_TESTHOOKALTERDBFILEHDR * const palterdbfilehdr )
{
    ERR err = JET_errSuccess;
    DBFILEHDR * pdbfilehdr = NULL;
    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapiDatabase = NULL;
    ShadowedHeaderStatus shs;
    DWORD cbPageSize = 1;
    CFlushMapForUnattachedDb* pfm           = NULL;

    Assert( palterdbfilehdr->szDatabase );

    AllocR( pdbfilehdr = (DBFILEHDR*)PvOSMemoryPageAlloc( g_cbPageMax, NULL ) );
    CallR( ErrOSUInit() );
    Call( ErrOSFSCreate( g_pfsconfigGlobal, &pfsapi ) );

    Call( pfsapi->ErrFileOpen( palterdbfilehdr->szDatabase, IFileAPI::fmfNone, &pfapiDatabase ) );
    Call( ErrUtilReadShadowedHeader( pinstNil, pfsapi, pfapiDatabase, (BYTE*)pdbfilehdr, (DWORD)g_cbPageMax, (LONG)OffsetOf( DBFILEHDR_FIX, le_cbPageSize ), urhfReadOnly|urhfNoFailOnPageMismatch, &cbPageSize, &shs ) );
    Call( CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( palterdbfilehdr->szDatabase, pdbfilehdr, pinstNil, &pfm ) );


    if ( ( palterdbfilehdr->ibField + palterdbfilehdr->cbField ) > cbPageSize )
    {
        AssertSz( fFalse, "Field off end of DB header!" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    C_ASSERT( OffsetOf( DBFILEHDR_FIX, le_ulVersion ) == JET_ibfieldDbFileHdrMajorVersion );
    C_ASSERT( OffsetOf( DBFILEHDR_FIX, le_ulDaeUpdateMajor ) == JET_ibfieldDbFileHdrUpdateMajor );
    C_ASSERT( OffsetOf( DBFILEHDR_FIX, le_ulDaeUpdateMinor ) == JET_ibfieldDbFileHdrUpdateMinor );

    BYTE * const pbHdrField = ((BYTE*)pdbfilehdr) + palterdbfilehdr->ibField;

    if ( palterdbfilehdr->grbit & JET_bitAlterDbFileHdrAddField )
    {
        switch( palterdbfilehdr->cbField )
        {
        case 4:
            *((UnalignedLittleEndian<LONG>*)pbHdrField) += *((UnalignedLittleEndian<LONG>*)palterdbfilehdr->pbField);
            break;
        case 8:
            *((UnalignedLittleEndian<__int64>*)pbHdrField) += *((UnalignedLittleEndian<__int64>*)palterdbfilehdr->pbField);
            break;
        default:
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }
    else
    {
        memcpy( pbHdrField, palterdbfilehdr->pbField, palterdbfilehdr->cbField );
    }

    DWORD dwPrev = FNegTestSet( fInvalidUsage | fCorruptingDbHeaders );
    Assert( ( dwPrev & ( fInvalidUsage | fCorruptingDbHeaders ) ) == 0 );
    Call( ErrUtilWriteUnattachedDatabaseHeaders( pinstNil, pfsapi, palterdbfilehdr->szDatabase, pdbfilehdr, pfapiDatabase, pfm ) );
    FNegTestUnset( fInvalidUsage | fCorruptingDbHeaders );
    if ( pfm )
    {
        pfm->TermFlushMap();
    }

HandleError:

    FNegTestUnset( fCorruptingDbHeaders );
    FNegTestUnset( fInvalidUsage );

    (void)ErrUtilFlushFileBuffers( pfapiDatabase, iofrUtility );
    OSMemoryPageFree( pdbfilehdr );
    delete pfm;
    delete pfapiDatabase;
    delete pfsapi;
    OSUTerm();

    return err;
}

JET_ERR JET_API JetTestHook(
    __in        const TESTHOOK_OP   opcode,
    __inout_opt void * const        pv )
{
    ERR err =   JET_errSuccess;

    switch( opcode )
    {

#ifdef ENABLE_JET_UNIT_TEST
        case opTestHookUnitTests:
        {
            Call( ErrSetSystemParameter( pinstNil, JET_sesidNil, JET_paramDisablePerfmon, fTrue, NULL ) );
            Call( ErrOSUInit() );
            const INT failures = JetUnitTest::RunTests( (const char * const)pv, ifmpNil );
            if( failures > 0 )
            {
                err = ErrERRCheck( JET_errInternalError );
            }
            OSUTerm();
        }
            break;

        case opTestHookUnitTests2:
        {
            const JET_TESTHOOKUNITTEST2* const pParams = reinterpret_cast<JET_TESTHOOKUNITTEST2*>( pv );

            const INT failures = JetUnitTest::RunTests( pParams->szTestName,
                            pParams->dbidTestOn == JET_dbidNil ? ifmpNil : (IFMP)pParams->dbidTestOn );
            if( failures > 0 )
            {
                err = ErrERRCheck( JET_errInternalError );
            }
        }
            break;

#else
        case opTestHookUnitTests:
        case opTestHookUnitTests2:
            err = ErrERRCheck( JET_errDisabledFunctionality );
            break;

#endif
        case opTestHookTestInjection:
        {
            const JET_TESTHOOKTESTINJECTION* const pParams = (JET_TESTHOOKTESTINJECTION*)pv;
            if ( pParams == NULL || pParams->cbStruct != sizeof(JET_TESTHOOKTESTINJECTION) )
            {
                Call( ErrERRCheck( JET_errInvalidParameter ) );
            }


            if ( pParams->ulID == 53984  )
            {
                if ( pParams->type != JET_TestInjectFault ||
                        pParams->grbit != JET_bitInjectionProbabilityPct ||
                        pParams->ulProbability != 5  )
                {
                    Call( ErrERRCheck( JET_errInvalidParameter ) );
                }
#ifdef DEBUG
                g_bflruk.SetFaultInjection( (DWORD)pParams->pv );
#endif
            }
            else
            {
                Call( ErrEnableTestInjection( pParams->ulID,
                                                pParams->pv,
                                                pParams->type,
                                                pParams->grbit == NO_GRBIT ? 100 : pParams->ulProbability,
                                                pParams->grbit == NO_GRBIT ? JET_bitInjectionProbabilityPct : pParams->grbit ) );
            }
        }
            break;

        case opTestHookHookNtQueryInformationProcess:
        {
            JET_TESTHOOKAPIHOOKING* const pParams = (JET_TESTHOOKAPIHOOKING*)pv;
            if ( pParams == NULL || pParams->cbStruct != sizeof(JET_TESTHOOKAPIHOOKING) )
            {
                Call( ErrERRCheck( JET_errInvalidParameter ) );
            }

            pParams->pfnOld = PvOSMemoryHookNtQueryInformationProcess( pParams->pfnNew );
        }
            break;

        case opTestHookHookNtQuerySystemInformation:
        {
            JET_TESTHOOKAPIHOOKING* const pParams = (JET_TESTHOOKAPIHOOKING*)pv;
            if ( pParams == NULL || pParams->cbStruct != sizeof(JET_TESTHOOKAPIHOOKING) )
            {
                Call( ErrERRCheck( JET_errInvalidParameter ) );
            }

            pParams->pfnOld = PvOSMemoryHookNtQuerySystemInformation( pParams->pfnNew );
        }
            break;

        case opTestHookHookGlobalMemoryStatus:
        {
            JET_TESTHOOKAPIHOOKING* const pParams = (JET_TESTHOOKAPIHOOKING*)pv;
            if ( pParams == NULL || pParams->cbStruct != sizeof(JET_TESTHOOKAPIHOOKING) )
            {
                Call( ErrERRCheck( JET_errInvalidParameter ) );
            }

            pParams->pfnOld = PvOSMemoryHookGlobalMemoryStatus( pParams->pfnNew );
        }
            break;

        case opTestHookSetNegativeTesting:
        {
            #ifndef RTM
                const DWORD dwNegativeTest  = *(DWORD * )pv;
                FNegTestSet( dwNegativeTest );
            #endif
        }
            break;
        case opTestHookResetNegativeTesting:
        {
            #ifndef RTM
                const DWORD dwNegativeTest  = *(DWORD * )pv;
                FNegTestUnset( dwNegativeTest );
            #endif
        }
            break;

        case opTestHookThrowError:
            ErrERRCheck( (JET_ERR)(LONG_PTR)pv );
            break;

        case opTestHookThrowAssert:
            AssertSz( fFalse, "This is not a real Assert(), if this had been a real Assert() you would have been given official text that would have scared you." );
            break;

        case opTestHookEnforceContextFail:
        {
            #ifndef RTM
                JET_TESTHOOKAPIHOOKING* const pParams = (JET_TESTHOOKAPIHOOKING*)pv;
                if ( pParams == NULL || pParams->cbStruct != sizeof(JET_TESTHOOKAPIHOOKING) )
                {
                    Call( ErrERRCheck( JET_errInvalidParameter ) );
                }

                pParams->pfnOld = g_pfnEnforceContextFail;
                g_pfnEnforceContextFail = ( decltype( g_pfnEnforceContextFail ) ) pParams->pfnNew;
            #endif
        }
            break;

        case opTestHookGetBFLowMemoryCallback:
            *(PfnMemNotification *)pv = BFIMaintLowMemoryCallback;
            break;

        case opTestHookTraceTestMarker:
        {
            Call( ErrOSUInit() );
            const JET_TESTHOOKTRACETESTMARKER* const pParams = (JET_TESTHOOKTRACETESTMARKER*)pv;

            QWORD qwMarker = (QWORD)-1;

            if ( pv < (void*)(64*1024) )
            {
                qwMarker = (QWORD)pv;
            }
            else
            {
                if ( pParams == NULL || pParams->cbStruct != sizeof(JET_TESTHOOKTRACETESTMARKER) )
                {
                    Call( ErrERRCheck( JET_errInvalidParameter ) );
                }
                qwMarker = pParams->qwMarkerID;
            }

            g_qwMarkerID = qwMarker;
            ETTestMarker( qwMarker, ( pv < (void*)(64*1024) ) ? NULL : pParams->szAnnotation );
            OSTrace( JET_tracetagInformation, OSFormat( "Mark[%I64d]: %hs", qwMarker, pParams->szAnnotation ) );

            OSUTerm();
        }
            break;

        case opTestHookGetCheckpointDepth:
        {
            const INST * const pinst = *(INST**)pv;


            LGPOS lgposCheckpoint;
            while ( fTrue )
            {
                lgposCheckpoint = pinst->m_plog->LgposLGCurrentCheckpointMayFail();
                if ( CmpLgpos( lgposCheckpoint, lgposMin ) != 0 )
                {
                    break;
                }
                UtilSleep( cmsecWaitGeneric );
            }
            const LGPOS lgposNewest = pinst->m_plog->LgposLGLogTipNoLock();
            const __int64 cbCheckpointDepth = (__int64)pinst->m_plog->CbLGOffsetLgposForOB0( lgposNewest, lgposCheckpoint );


            Assert( cbCheckpointDepth > -( 4096 * 64 * 1024 )  );
            *((__int64*)pv) = max( cbCheckpointDepth, 0 );
        }
            break;

        case opTestHookGetOLD2Status:
        {
            INST * pinst = *(INST**)pv;
            Assert( pinst );
            Assert( pinst->m_mpdbidifmp[dbidUserLeast] != ifmpNil );
            *((__int64*)pv) = (__int64)( g_rgfmp[pinst->m_mpdbidifmp[dbidUserLeast]].FDontRegisterOLD2Tasks() );
        }
            break;

        case opTestHookGetEngineTickNow:
            *((ULONG*)pv) = TickOSTimeCurrent();
            break;

        case opTestHookSetEngineTickTime:
        {
            JET_TESTHOOKTIMEINJECTION * pthtimeinj = (JET_TESTHOOKTIMEINJECTION*)pv;
            if ( pthtimeinj->tickNow )
            {
                if ( pthtimeinj->tickNow < 0x7fffffff )
                {
                    pthtimeinj->eTimeInjWrapMode = 1;
                    pthtimeinj->dtickTimeInjWrapOffset = 0x7fffffff - pthtimeinj->tickNow;
                }
                else
                {
                    pthtimeinj->eTimeInjWrapMode = 2;
                    pthtimeinj->dtickTimeInjWrapOffset = 0xffffffff - pthtimeinj->tickNow;
                }
            }
            OSTimeSetTimeInjection( pthtimeinj->eTimeInjWrapMode, pthtimeinj->dtickTimeInjWrapOffset, pthtimeinj->dtickTimeInjAccelerant );
            if ( pthtimeinj->tickNow )
            {
                g_bflruk.SetTimeBar( 90 * 60 * 1000 , pthtimeinj->tickNow + 10 * 60 * 60 * 1000  );
            }
        }
            break;

        case opTestHookCacheQuery:
        {
#ifdef DEBUGGER_EXTENSION
            JET_TESTHOOKCACHEQUERY * pThCacheQuery = (JET_TESTHOOKCACHEQUERY *)pv;

            Call( ErrEDBGCacheQueryLocal( pThCacheQuery->cCacheQuery, pThCacheQuery->rgszCacheQuery, pThCacheQuery->pvOut ) );
#else
            Call( ErrERRCheck( JET_errDisabledFunctionality ) );
#endif
        }
            break;

        case opTestHookEvictCache:
        {
            JET_TESTHOOKEVICTCACHE * pevicttarget = (JET_TESTHOOKEVICTCACHE *)pv;
            Assert( pevicttarget->cbStruct >= sizeof(JET_TESTHOOKEVICTCACHE) );
            Assert( pevicttarget->grbit == JET_bitTestHookEvictDataByPgno );
            Call( ErrBFTestEvictPage( (IFMP)pevicttarget->ulTargetContext, (PGNO)pevicttarget->ulTargetData ) );
        }
            break;

        case opTestHookCorrupt:
        {
            JET_TESTHOOKCORRUPT * pcorrupt = (JET_TESTHOOKCORRUPT *)pv;
            Assert( pcorrupt->grbit );
            Assert( pcorrupt->cbStruct >= ( OffsetOf( JET_TESTHOOKCORRUPT, grbit ) + sizeof( ULONG ) ) );

            C_ASSERT( 0 == ( JET_mskTestHookCorruptFileType & JET_mskTestHookCorruptDataType ) );
            C_ASSERT( 0 == ( JET_mskTestHookCorruptFileType & JET_mskTestHookCorruptSpecific ) );
            C_ASSERT( 0 == ( JET_mskTestHookCorruptDataType & JET_mskTestHookCorruptSpecific ) );
            C_ASSERT( 0 == ( ( JET_mskTestHookCorruptFileType |
                                JET_mskTestHookCorruptDataType |
                                JET_mskTestHookCorruptSpecific ) &
                                JET_bitTestHookCorruptLeaveChecksum ) );


            if ( pcorrupt->grbit & JET_bitTestHookCorruptDatabaseFile )
            {
                Call( ErrTestHookCorruptOfflineFile( pcorrupt ) );
            }
            else if ( pcorrupt->grbit & JET_bitTestHookCorruptDatabasePageImage )
            {
                AssertSz( pcorrupt->CorruptDatabasePageImage.pgnoTarget != JET_pgnoTestHookCorruptRandom, "NYI" );

                CPAGE cpage;
                cpage.LoadPage( ifmpNil,
                        (PGNO)pcorrupt->CorruptDatabasePageImage.pgnoTarget,
                        (BYTE *)pcorrupt->CorruptDatabasePageImage.pbPageImageTarget,
                        pcorrupt->CorruptDatabasePageImage.cbPageImage );
                if ( cpage.Clines() < 1 )
                {
                    AssertSz( fFalse, "Will not work out ..." );
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }


                if ( pcorrupt->grbit & JET_bitTestHookCorruptPageSingleFld )
                {
                    (void)FNegTestSet( fCorruptingPageLogically );

                    const INT iline = rand() % cpage.Clines();
                    LINE line;
                    cpage.GetPtr( iline, &line );
                    Assert( line.cb > 2 );
                    const INT ib = 2 + ( rand() % ( line.cb - 2 ) );
                    ((BYTE*)line.pv)[ib] = 0x42 ^ ((BYTE*)line.pv)[ib];
                }

                if ( pcorrupt->grbit & JET_bitTestHookCorruptPageRemoveNode )
                {
                    (void)FNegTestSet( fCorruptingPageLogically );

                    AssertSz( fFalse, "NYI - caused problems if there is only 1 line on the page." );

                    const INT iline = rand() % cpage.Clines();
                    cpage.Delete( iline );
                }

                if ( pcorrupt->grbit & JET_bitTestHookCorruptPageDbtimeDelta )
                {
                    Assert( pcorrupt->CorruptDatabasePageImage.iSubTarget );


                    (void)FNegTestSet( fCorruptingWithLostFlush );

                    const bool fPreviouslySet = FNegTestSet( fInvalidUsage );


                    cpage.SetDbtime( cpage.Dbtime() + pcorrupt->CorruptDatabasePageImage.iSubTarget );

                    if ( !fPreviouslySet )
                    {
                        FNegTestUnset( fInvalidUsage );
                    }
                }

                if ( pcorrupt->grbit & JET_bitTestHookCorruptNodePrefix )
                {
#ifdef DEBUG
                    NDCorruptNodePrefixSize( cpage, (ULONG)pcorrupt->CorruptDatabasePageImage.iSubTarget, pcorrupt->grbit & JET_mskTestHookCorruptSpecific );
#else
                    Error( ErrERRCheck( JET_errDisabledFunctionality ) );
#endif
                }

                if ( pcorrupt->grbit & JET_bitTestHookCorruptNodeSuffix )
                {
#ifdef DEBUG
                    NDCorruptNodeSuffixSize( cpage, (ULONG)pcorrupt->CorruptDatabasePageImage.iSubTarget, pcorrupt->grbit & JET_mskTestHookCorruptSpecific );
#else
                    Error( ErrERRCheck( JET_errDisabledFunctionality ) );
#endif
                }

                if ( !( pcorrupt->grbit & JET_bitTestHookCorruptLeaveChecksum ) )
                {
                    const bool fPreviouslySet = FNegTestSet( fInvalidUsage );

                    cpage.PreparePageForWrite( cpage.Pgft() );

                    if ( !fPreviouslySet )
                    {
                        FNegTestUnset( fInvalidUsage );
                    }
                }

                if ( pcorrupt->grbit & JET_bitTestHookCorruptPageChksumSafe )
                {
                    AssertSz( fFalse, "NYI" );
                }
                if ( pcorrupt->grbit & JET_bitTestHookCorruptPageChksumRand )
                {
                    AssertSz( fFalse, "NYI" );
                }
            }
            else
            {
                AssertSz( fFalse, "No main grbit for corruption guidance, targeting off ..." );
            }

        }
            break;

        case opTestHookEnableAutoIncDeprecated:
            AssertSz( fFalse, "Enabling auto-inc should be done through JET_paramEnableEngineVersion being set to a version greater than JET_efvExtHdrRootFieldAutoIncStorageReleased" );
            break;

        case opTestHookSetErrorTrap:
            (void)ErrERRSetErrTrap( *(ERR *)pv );
            break;

        case opTestHookGetTablePgnoFDP:
        {
            const FUCB * pfucb = *(FUCB**)pv;

            memset( pv, 0, sizeof(FUCB*) );
            *(DWORD * )pv = pfucb->u.pfcb->PgnoFDP();
        }
            break;

        case opTestHookAlterDatabaseFileHeader:
        {
            JET_TESTHOOKALTERDBFILEHDR * palterdbfilehdr = (JET_TESTHOOKALTERDBFILEHDR*)pv;
            err = ErrTESTHOOKAlterDatabaseFileHeader( palterdbfilehdr );
        }
            break;

        case opTestHookGetLogTip:
        {
            const INST * const pinst = *(INST**)pv;
            const LGPOS lgposNewest = pinst->m_plog->LgposLGLogTipNoLock();
            *((__int64*)pv) = lgposNewest.qw;
        }
            break;

        default:
            Call( ErrERRCheck( JET_errInvalidParameter ) );
            break;
    }

HandleError:
    return err;
}

LOCAL JET_ERR JetConsumeLogDataEx(
    __in    JET_INSTANCE        instance,
    __in    JET_EMITDATACTX *   pEmitLogDataCtx,
    __in    void *              pvLogData,
    __in    ULONG       cbLogData,
    __in    JET_GRBIT           grbits  )
{
    APICALL_INST    apicall( opConsumeLogData );

    Assert( instance );

    OSTrace( JET_tracetagAPI, OSFormat(
            "Start %s(0x%Ix,0x%p={%d,%d,%I64d,0x%x,%02d/%02d/%04d %02d:%02d:%02d.%3.3d,%08lX:%04hX:%04hX,%d},0x%p,%d,0x%x)",
            __FUNCTION__, instance, pEmitLogDataCtx,
            pEmitLogDataCtx ? pEmitLogDataCtx->cbStruct : 0,
            pEmitLogDataCtx ? pEmitLogDataCtx->dwVersion : 0,
            pEmitLogDataCtx ? pEmitLogDataCtx->qwSequenceNum : 0,
            pEmitLogDataCtx ? pEmitLogDataCtx->grbitOperationalFlags : 0,
            pEmitLogDataCtx ? pEmitLogDataCtx->logtimeEmit.bMonth : 0,
            pEmitLogDataCtx ? pEmitLogDataCtx->logtimeEmit.bDay : 0,
            pEmitLogDataCtx ? pEmitLogDataCtx->logtimeEmit.bYear : 0,
            pEmitLogDataCtx ? pEmitLogDataCtx->logtimeEmit.bHours : 0,
            pEmitLogDataCtx ? pEmitLogDataCtx->logtimeEmit.bMinutes : 0,
            pEmitLogDataCtx ? pEmitLogDataCtx->logtimeEmit.bSeconds : 0,
            pEmitLogDataCtx ? ((LOGTIME *)&pEmitLogDataCtx->logtimeEmit)->Milliseconds() : 0,
            pEmitLogDataCtx ? pEmitLogDataCtx->lgposLogData.lGeneration : 0,
            SHORT( pEmitLogDataCtx ? pEmitLogDataCtx->lgposLogData.isec : 0 ),
            SHORT( pEmitLogDataCtx ? pEmitLogDataCtx->lgposLogData.ib : 0 ),
            pEmitLogDataCtx ? pEmitLogDataCtx->cbLogData : 0,
            pvLogData, cbLogData, grbits ) );


    if ( pEmitLogDataCtx == NULL )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( 0 != grbits )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }


    if ( pEmitLogDataCtx->cbStruct != sizeof(JET_EMITDATACTX) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    BOOL fAPIEntered = fFalse;


    fAPIEntered = apicall.FEnterFromInitCallback( (INST*)instance ) ? fTrue :
                    apicall.FEnterFromTermCallback( (INST*)instance ) ? fTrue :
                    apicall.FEnter( instance );

    if ( fAPIEntered )
    {
        apicall.LeaveAfterCall( apicall.Pinst()->m_plog->ErrLGShadowLogAddData(
                                                                pEmitLogDataCtx,
                                                                pvLogData,
                                                                cbLogData ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetConsumeLogData(
    __in    JET_INSTANCE        instance,
    __in    JET_EMITDATACTX *   pEmitLogDataCtx,
    __in    void *              pvLogData,
    __in    ULONG       cbLogData,
    __in    JET_GRBIT           grbits  )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opConsumeLogData, JetConsumeLogDataEx( instance, pEmitLogDataCtx, pvLogData, cbLogData, grbits ) );
}



LOCAL JET_ERR JET_API JetGetErrorInfoExW(
    __in_opt void *                 pvContext,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel,
    __in JET_GRBIT                  grbit )
{
    ERR                 err;
    ULONG               cbMin;
    APICALL             apicall( opGetErrorInfo );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p,0x%p,0x%x,0x%x,0x%x)",
            __FUNCTION__,
            pvContext,
            pvResult,
            cbMax,
            InfoLevel,
            grbit ) );

    ProbeClientBuffer( pvResult, cbMax );

    switch( InfoLevel )
    {
        case JET_ErrorInfoSpecificErr:
            cbMin = sizeof( JET_ERRINFOBASIC_W );
            if ( !pvContext )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
            break;

        default:
            Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( grbit != JET_bitNil )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( cbMax >= cbMin )
    {
        switch ( InfoLevel )
        {
            case JET_ErrorInfoSpecificErr:
            {
                JET_ERRINFOBASIC_W* const perrinfobasic = (JET_ERRINFOBASIC_W*) pvResult;
                perrinfobasic->errValue = *(JET_ERR*)pvContext;

                perrinfobasic->cbStruct = cbMin;

                if ( perrinfobasic->cbStruct < sizeof( JET_ERRINFOBASIC_W ) )
                {
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }

                Call( ErrERRLookupErrorCategory( perrinfobasic->errValue, &perrinfobasic->errcatMostSpecific ) );
                Call( ErrERRLookupErrorHierarchy( perrinfobasic->errcatMostSpecific, &perrinfobasic->rgCategoricalHierarchy[ 0 ] ) );
                perrinfobasic->lSourceLine = 0;
                perrinfobasic->rgszSourceFile[ 0 ] = L'\0';
                break;
            }
            default:
                AssertSz( fFalse, "Invalid infolevels should have been caught above" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }
    else
    {
        Error( ErrERRCheck( JET_errInvalidBufferSize ) );
    }

HandleError:
    apicall.SetErr( err );
    return apicall.ErrResult();
}


#if 0
JET_ERR JET_API JetGetErrorInfoA(
    __in_opt void *                 pvContext,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              ulInfoLevel,
    __in JET_GRBIT                  grbit );
#endif

JET_ERR JET_API JetGetErrorInfoW(
    __in_opt void *                 pvContext,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel,
    __in JET_GRBIT                  grbit )
{
    JET_TRY( opGetErrorInfo, JetGetErrorInfoExW( pvContext, pvResult, cbMax, InfoLevel, grbit ) );
}

JET_ERR JET_API
JetSetSessionParameterEx(
    _In_opt_ JET_SESID                          sesid,
    _In_ ULONG                          sesparamid,
    _In_reads_bytes_opt_( cbParam ) void *      pvParam,
    _In_ ULONG                      cbParam )
{
    APICALL_SESID   apicall( opSetSessionParameter );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            sesparamid,
            pvParam,
            cbParam ) );

    if ( apicall.FEnter( sesid, fFalse ) )
    {
        apicall.LeaveAfterCall( ErrIsamSetSessionParameter( sesid, sesparamid, pvParam, cbParam ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API
JetSetSessionParameter(
    _In_opt_ JET_SESID                              sesid,
    _In_ ULONG                              sesparamid,
    _In_reads_bytes_opt_( cbParam ) void *          pvParam,
    _In_ ULONG                          cbParam )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opSetSessionParameter, JetSetSessionParameterEx( sesid, sesparamid, pvParam, cbParam ) );
}

JET_ERR JET_API
JetGetSessionParameterEx(
    _In_opt_ JET_SESID                                          sesid,
    _In_ ULONG                                          sesparamid,
    _Out_cap_post_count_(cbParamMax, *pcbParamActual) void *    pvParam,
    _In_ ULONG                                          cbParamMax,
    _Out_opt_ ULONG *                                   pcbParamActual )
{
    APICALL_INST    apicall( opGetSessionParameter );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p,0x%x,0x%p)",
            __FUNCTION__,
            sesid,
            sesparamid,
            pvParam,
            cbParamMax,
            pcbParamActual ) );

    if ( apicall.FEnter( (JET_INSTANCE)PinstFromSesid( sesid ) ) )
    {
        apicall.LeaveAfterCall( ErrIsamGetSessionParameter( sesid, sesparamid, pvParam, cbParamMax, pcbParamActual ) );
    }
    return apicall.ErrResult();
}

JET_ERR JET_API
JetGetSessionParameter(
    _In_opt_ JET_SESID                                          sesid,
    _In_ ULONG                                          sesparamid,
    _Out_cap_post_count_(cbParamMax, *pcbParamActual) void *    pvParam,
    _In_ ULONG                                          cbParamMax,
    _Out_opt_ ULONG *                                   pcbParamActual )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetSessionParameter, JetGetSessionParameterEx( sesid, sesparamid, pvParam, cbParamMax, pcbParamActual ) );
}

ERR JetPrereadTablesEx(
    __in JET_SESID                          sesid,
    __in JET_DBID                           dbid,
    __in_ecount( cwszTables ) PCWSTR *      rgwszTables,
    __in INT                                cwszTables,
    __in JET_GRBIT                          grbit )
{
    APICALL_SESID   apicall( opPrereadTables );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p,0x%x,0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            rgwszTables,
            cwszTables,
            grbit ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        apicall.LeaveAfterCall( ErrIsamPrereadTables( sesid, dbid, rgwszTables, cwszTables, grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetPrereadTablesW(
    __in JET_SESID                          sesid,
    __in JET_DBID                           dbid,
    __in_ecount( cwszTables ) JET_PCWSTR *  rgwszTables,
    __in LONG                               cwszTables,
    __in JET_GRBIT                          grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opPrereadTables, JetPrereadTablesEx( sesid, dbid, rgwszTables, cwszTables, grbit ) );
}

LOCAL JET_ERR JetPrereadIndexRangeEx(
    __in const JET_SESID                sesid,
    __in const JET_TABLEID              tableid,
    __in const JET_INDEX_RANGE * const  pIndexRange,
    __in const ULONG            cPageCacheMin,
    __in const ULONG            cPageCacheMax,
    __in const JET_GRBIT                grbit,
    __out_opt ULONG * const     pcPageCacheActual )
{
    APICALL_SESID   apicall( opPrereadIndexRange );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
        "Start %s(0x%Ix,0x%Ix,0x%p,%d,%d,0x%x)",
        __FUNCTION__,
        sesid,
        tableid,
        pIndexRange,
        cPageCacheMin,
        cPageCacheMax,
        grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispPrereadIndexRange( sesid, tableid, pIndexRange, cPageCacheMin, cPageCacheMax, grbit, pcPageCacheActual ) );
    }

    return apicall.ErrResult();
}

JET_ERR JetPrereadIndexRange(
    __in const JET_SESID                sesid,
    __in const JET_TABLEID              tableid,
    __in const JET_INDEX_RANGE * const  pIndexRange,
    __in const ULONG            cPageCacheMin,
    __in const ULONG            cPageCacheMax,
    __in const JET_GRBIT                grbit,
    __out_opt ULONG * const     pcPageCacheActual )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opPrereadIndexRange, JetPrereadIndexRangeEx( sesid, tableid, pIndexRange, cPageCacheMin, cPageCacheMax, grbit, pcPageCacheActual ) );
}

LOCAL JET_ERR JetRetrieveColumnByReferenceEx(
    _In_ const JET_SESID                                            sesid,
    _In_ const JET_TABLEID                                          tableid,
    _In_reads_bytes_( cbReference ) const void * const              pvReference,
    _In_ const ULONG                                        cbReference,
    _In_ const ULONG                                        ibData,
    _Out_writes_bytes_to_opt_( cbData, min( cbData, *pcbActual ) ) void * const pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit )
{
    APICALL_SESID   apicall( opRetrieveColumnByReference );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
        "Start %s(0x%Ix,0x%Ix,0x%p,%d,%d,0x%p,%d,0x%p,0x%x)",
        __FUNCTION__,
        sesid,
        tableid,
        pvReference,
        cbReference,
        ibData,
        pvData,
        cbData,
        pcbActual,
        grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispRetrieveColumnByReference( sesid, tableid, pvReference, cbReference, ibData, pvData, cbData, pcbActual, grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetRetrieveColumnByReference(
    _In_ const JET_SESID                                            sesid,
    _In_ const JET_TABLEID                                          tableid,
    _In_reads_bytes_( cbReference ) const void * const              pvReference,
    _In_ const ULONG                                        cbReference,
    _In_ const ULONG                                        ibData,
    _Out_writes_bytes_to_opt_( cbData, min( cbData, *pcbActual ) ) void * const pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opRetrieveColumnByReference, JetRetrieveColumnByReferenceEx( sesid, tableid, pvReference, cbReference, ibData, pvData, cbData, pcbActual, grbit ) );
}

LOCAL JET_ERR JetPrereadColumnsByReferenceEx(
    _In_ const JET_SESID                                    sesid,
    _In_ const JET_TABLEID                                  tableid,
    _In_reads_( cReferences ) const void * const * const    rgpvReferences,
    _In_reads_( cReferences ) const ULONG * const   rgcbReferences,
    _In_ const ULONG                                cReferences,
    _In_ const ULONG                                cPageCacheMin,
    _In_ const ULONG                                cPageCacheMax,
    _Out_opt_ ULONG * const                         pcReferencesPreread,
    _In_ const JET_GRBIT                                    grbit )
{
    APICALL_SESID   apicall( opPrereadColumnsByReference );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
        "Start %s(0x%Ix,0x%Ix,0x%p,0x%p,%d,%d,%d,0x%p,0x%x)",
        __FUNCTION__,
        sesid,
        tableid,
        rgpvReferences,
        rgcbReferences,
        cReferences,
        cPageCacheMin,
        cPageCacheMax,
        pcReferencesPreread,
        grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispPrereadColumnsByReference( sesid, tableid, rgpvReferences, rgcbReferences, cReferences, cPageCacheMin, cPageCacheMax, pcReferencesPreread, grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetPrereadColumnsByReference(
    _In_ const JET_SESID                                    sesid,
    _In_ const JET_TABLEID                                  tableid,
    _In_reads_( cReferences ) const void * const * const    rgpvReferences,
    _In_reads_( cReferences ) const ULONG * const   rgcbReferences,
    _In_ const ULONG                                cReferences,
    _In_ const ULONG                                cPageCacheMin,
    _In_ const ULONG                                cPageCacheMax,
    _Out_opt_ ULONG * const                         pcReferencesPreread,
    _In_ const JET_GRBIT                                    grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opPrereadColumnsByReference, JetPrereadColumnsByReferenceEx( sesid, tableid, rgpvReferences, rgcbReferences, cReferences, cPageCacheMin, cPageCacheMax, pcReferencesPreread, grbit ) );
}

LOCAL JET_ERR JetStreamRecordsEx(
    _In_ JET_SESID                                                  sesid,
    _In_ JET_TABLEID                                                tableid,
    _In_ const ULONG                                        ccolumnid,
    _In_reads_opt_( ccolumnid ) const JET_COLUMNID * const          rgcolumnid,
    _Out_writes_bytes_to_opt_( cbData, *pcbActual ) void * const    pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit )
{
    APICALL_SESID   apicall( opStreamRecords );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%Ix,%d,0x%p,0x%p,%d,0x%p,0x%x)",
            __FUNCTION__,
            sesid,
            tableid,
            ccolumnid,
            rgcolumnid,
            pvData,
            cbData,
            pcbActual,
            grbit ) );

    if ( apicall.FEnter( sesid ) )
    {
        apicall.LeaveAfterCall( ErrDispStreamRecords( sesid, tableid, ccolumnid, rgcolumnid, pvData, cbData, pcbActual, grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetStreamRecords(
    _In_ JET_SESID                                                  sesid,
    _In_ JET_TABLEID                                                tableid,
    _In_ const ULONG                                        ccolumnid,
    _In_reads_opt_( ccolumnid ) const JET_COLUMNID * const          rgcolumnid,
    _Out_writes_bytes_to_opt_( cbData, *pcbActual ) void * const    pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opStreamRecords, JetStreamRecordsEx( sesid, tableid, ccolumnid, rgcolumnid, pvData, cbData, pcbActual, grbit ) );
}

LOCAL JET_ERR JetRetrieveColumnFromRecordStreamEx(
    _Inout_updates_bytes_( cbData ) void * const    pvData,
    _In_ const ULONG                        cbData,
    _Out_ ULONG * const                     piRecord,
    _Out_ JET_COLUMNID * const                      pcolumnid,
    _Out_ ULONG * const                     pitagSequence,
    _Out_ ULONG * const                     pibValue,
    _Out_ ULONG * const                     pcbValue )
{
    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p,%d,0x%p,0x%p,0x%p,0x%p,0x%p)",
            __FUNCTION__,
            pvData,
            cbData,
            piRecord,
            pcolumnid,
            pitagSequence,
            pibValue,
            pcbValue ) );

    return ErrIsamRetrieveColumnFromRecordStream( pvData, cbData, piRecord, pcolumnid, pitagSequence, pibValue, pcbValue );
}

JET_ERR JET_API JetRetrieveColumnFromRecordStream(
    _Inout_updates_bytes_( cbData ) void * const    pvData,
    _In_ const ULONG                        cbData,
    _Out_ ULONG * const                     piRecord,
    _Out_ JET_COLUMNID * const              pcolumnid,
    _Out_ ULONG * const                     pitagSequence,
    _Out_ ULONG * const                     pibValue,
    _Out_ ULONG * const                     pcbValue )
{
    JET_TRY( opRetrieveColumnFromRecordStream, JetRetrieveColumnFromRecordStreamEx( pvData, cbData, piRecord, pcolumnid, pitagSequence, pibValue, pcbValue ) );
}

LOCAL JET_ERR JetGetRBSFileInfoEx(
    __in JET_PCWSTR                 wszRBSFileName,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG                      cbMax,
    __in ULONG                      InfoLevel )
{
    ERR             err                 = JET_errSuccess;
    RBSFILEHDR*     prbsfilehdr         = NULL;
    IFileSystemAPI* pfsapi              = NULL;
    IFileAPI*       pfapi               = NULL;

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p[%s],0x%p,0x%x,0x%x)",
            __FUNCTION__,
            wszRBSFileName,
            SzOSFormatStringW( wszRBSFileName ),
            pvResult,
            cbMax,
            InfoLevel ) );

    CallR( ErrOSUInit() );

    Call( ErrOSFSCreate( g_pfsconfigGlobal, &pfsapi ) );
    Call( pfsapi->ErrFileOpen( wszRBSFileName, ( BoolParam( JET_paramEnableFileCache ) ? IFileAPI::fmfCached : IFileAPI::fmfNone ) | IFileAPI::fmfReadOnly, &pfapi ) );

    switch ( InfoLevel )
    {
        case JET_RBSFileInfoMisc:
            if ( sizeof( JET_RBSINFOMISC ) != cbMax )
            {
                Error( ErrERRCheck( JET_errInvalidBufferSize ) );
            }
            ProbeClientBuffer( pvResult, cbMax );

            Alloc( prbsfilehdr = (RBSFILEHDR * )PvOSMemoryPageAlloc( sizeof( RBSFILEHDR ), NULL ) );

            Call( ErrUtilReadShadowedHeader( pinstNil, pfsapi, pfapi, (BYTE*) prbsfilehdr, sizeof( RBSFILEHDR ), -1, urhfNoAutoDetectPageSize | urhfReadOnly | urhfNoEventLogging ) );
            UtilLoadRBSinfomiscFromRBSfilehdr( ( JET_RBSINFOMISC* )pvResult, cbMax, ( RBSFILEHDR* )prbsfilehdr );
            break;

        default:
            Assert( fFalse );
    }

HandleError:
    OSMemoryPageFree( prbsfilehdr );

    delete pfapi;
    delete pfsapi;

    OSUTerm();

    return err;
}

}

LOCAL JET_ERR JetGetRBSFileInfoExA(
    __in JET_PCSTR                  szRBSFileName,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    ERR             err;
    CAutoWSZPATH    lszRBSFileName;

    CallR( lszRBSFileName.ErrSet( szRBSFileName ) );

    return JetGetRBSFileInfoEx( lszRBSFileName, pvResult, cbMax, InfoLevel );
}

JET_ERR JET_API JetGetRBSFileInfoA(
    __in JET_PCSTR                  szRBSFileName,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_TRY( opGetDatabaseFileInfo, JetGetRBSFileInfoExA( szRBSFileName, pvResult, cbMax, InfoLevel ) );
}

JET_ERR JET_API JetGetRBSFileInfoW(
    __in JET_PCWSTR                 wszRBSFileName,
    __out_bcount( cbMax ) void *    pvResult,
    __in ULONG              cbMax,
    __in ULONG              InfoLevel )
{
    JET_TRY( opGetDatabaseFileInfo, JetGetRBSFileInfoEx( wszRBSFileName, pvResult, cbMax, InfoLevel ) );
}

LOCAL
JET_ERR
JetRBSPrepareRevertEx(
    __in    JET_INSTANCE    instance,
    __in    JET_LOGTIME     jltRevertExpected,
    __in    CPG             cpgCache,
    __in    JET_GRBIT       grbit,
    _Out_   JET_LOGTIME*    pjltRevertActual )
{
    APICALL_INST    apicall( opRBSPrepareRevert );

    OSTrace( JET_tracetagAPI, OSFormat(
        "Start %s(0x%Ix,{%02d/%02d/%04d %02d:%02d:%02d.%3.3d},0x%x,0x%p)",
        __FUNCTION__,
        instance,
        jltRevertExpected.bMonth,
        jltRevertExpected.bDay,
        jltRevertExpected.bYear,
        jltRevertExpected.bHours,
        jltRevertExpected.bMinutes,
        jltRevertExpected.bSeconds,
        ((LOGTIME *)&jltRevertExpected)->Milliseconds(),
        grbit,
        pjltRevertActual) );

    if ( apicall.FEnterWithoutInit( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamRBSPrepareRevert( (JET_INSTANCE)apicall.Pinst(), jltRevertExpected, cpgCache, grbit, pjltRevertActual ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetRBSPrepareRevert(
    __in    JET_INSTANCE    instance,
    __in    JET_LOGTIME     jltRevertExpected,
    __in    CPG             cpgCache,
    __in    JET_GRBIT       grbit,
    _Out_   JET_LOGTIME*    pjltRevertActual )
{
    JET_TRY( opRBSPrepareRevert, JetRBSPrepareRevertEx( instance, jltRevertExpected, cpgCache, grbit, pjltRevertActual ) );
}

LOCAL
JET_ERR
JetRBSExecuteRevertEx(
    __in    JET_INSTANCE            instance,
    __in    JET_GRBIT               grbit,
    _Out_   JET_RBSREVERTINFOMISC*  prbsrevertinfomisc )
{
    APICALL_INST    apicall( opRBSExecuteRevert );

    OSTrace( JET_tracetagAPI, OSFormat(
        "Start %s(0x%Ix,0x%x)",
        __FUNCTION__,
        instance,
        grbit) );

    if ( apicall.FEnterWithoutInit( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamRBSExecuteRevert( (JET_INSTANCE)apicall.Pinst(), grbit, prbsrevertinfomisc ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetRBSExecuteRevert(
    __in    JET_INSTANCE            instance,
    __in    JET_GRBIT               grbit,
    _Out_   JET_RBSREVERTINFOMISC*  prbsrevertinfomisc )
{
    JET_TRY( opRBSExecuteRevert, JetRBSExecuteRevertEx( instance, grbit, prbsrevertinfomisc ) );
}

LOCAL
JET_ERR
JetRBSCancelRevertEx(
    __in    JET_INSTANCE    instance )
{
    APICALL_INST    apicall( opRBSCancelRevert );

    OSTrace( JET_tracetagAPI, OSFormat(
        "Start %s(0x%Ix)",
        __FUNCTION__,
        instance) );

    if ( apicall.FEnterWithoutInit( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamRBSCancelRevert( (JET_INSTANCE)apicall.Pinst() ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetRBSCancelRevert(
    __in    JET_INSTANCE    instance )
{
    JET_TRY( opRBSCancelRevert, JetRBSCancelRevertEx( instance ) );
}

VOID ThreadWaitBegin()
{
    TLS* const ptls = Ptls();

    AssertSz( ptls->hrtWaitBegin == 0, "Recursion of ThreadWaitBegin/ThreadWaitEnd is not supported." );

    ptls->hrtWaitBegin = HrtHRTCount() | 1;
}

COSLayerPreInit::PfnThreadWait g_pfnThreadWaitBegin = ThreadWaitBegin;

VOID ThreadWaitEnd()
{
    TLS* const ptls = Ptls();

    if ( ptls->hrtWaitBegin != 0 )
    {
        const HRT dhrtWait = ( HrtHRTCount() | 1 ) - ptls->hrtWaitBegin;
        const HRT dhrtSec = HrtHRTFreq();

        const HRT dhrtWaitT = 1000000 * dhrtWait;

        if ( dhrtWaitT >= dhrtSec )
        {
            ptls->threadstats.cusecWait += dhrtWaitT / dhrtSec;
            ptls->threadstats.cWait++;
        }

        ptls->hrtWaitBegin = 0;
    }
}

COSLayerPreInit::PfnThreadWait g_pfnThreadWaitEnd = ThreadWaitEnd;

#ifdef ENABLE_JET_UNIT_TEST
JETUNITTEST( JetApi, ErrGetLogInfoMiscFromLgfilehdr )
{
    LGFILEHDR lgfilehdr;
    memset( &lgfilehdr, 0, sizeof(lgfilehdr) );
    JET_LOGINFOMISC loginfomisc;
    memset( &loginfomisc, 0, sizeof(loginfomisc) );
    JET_LOGINFOMISC2 loginfomisc2;
    memset( &loginfomisc2, 0, sizeof(loginfomisc2) );

    lgfilehdr.lgfilehdr.le_lGeneration      = 2;
    lgfilehdr.lgfilehdr.le_ulMajor          = 3;
    lgfilehdr.lgfilehdr.le_ulMinor          = 4;
    lgfilehdr.lgfilehdr.le_ulUpdateMajor    = 5;
    lgfilehdr.lgfilehdr.le_ulUpdateMinor    = 51;
    lgfilehdr.lgfilehdr.le_csecLGFile       = 6;
    lgfilehdr.lgfilehdr.le_lgposCheckpoint.le_ib            = 7;
    lgfilehdr.lgfilehdr.le_lgposCheckpoint.le_isec          = 8;
    lgfilehdr.lgfilehdr.le_lgposCheckpoint.le_lGeneration   = 9;

    CHECK( JET_errInvalidBufferSize == ErrGetLogInfoMiscFromLgfilehdr( &lgfilehdr, &loginfomisc, sizeof(loginfomisc), JET_LogInfoMisc2 ) );
    CHECK( JET_errInvalidBufferSize == ErrGetLogInfoMiscFromLgfilehdr( &lgfilehdr, &loginfomisc2, sizeof(loginfomisc2), JET_LogInfoMisc ) );

    CHECK( JET_errSuccess == ErrGetLogInfoMiscFromLgfilehdr( &lgfilehdr, &loginfomisc, sizeof(loginfomisc), JET_LogInfoMisc ) );
    CHECK(  2 == loginfomisc.ulGeneration );
    CHECK(  3 == loginfomisc.ulVersionMajor );
    CHECK(  4 == loginfomisc.ulVersionMinor );
    CHECK(  5 == loginfomisc.ulVersionUpdate );
    CHECK(  6 == loginfomisc.cbFile );

    CHECK( JET_errSuccess == ErrGetLogInfoMiscFromLgfilehdr( &lgfilehdr, &loginfomisc2, sizeof(loginfomisc2), JET_LogInfoMisc2 ) );
    CHECK( 2 == loginfomisc2.ulGeneration );
    CHECK( 3 == loginfomisc2.ulVersionMajor );
    CHECK( 4 == loginfomisc2.ulVersionMinor );
    CHECK( 5 == loginfomisc2.ulVersionUpdate );
    CHECK( 6 == loginfomisc2.cbFile );
    CHECK( 7 == loginfomisc2.lgposCheckpoint.ib );
    CHECK( 8 == loginfomisc2.lgposCheckpoint.isec );
    CHECK( 9 == loginfomisc2.lgposCheckpoint.lGeneration );
}

JETUNITTEST( JetApi, CAutoINDEXCREATE1To2 )
{
    const INT cindexes = 3;
    JET_INDEXCREATEOLD_A rgindexcreateold[cindexes];
    char rgszIndexName[cindexes][JET_cbNameMost];
    char rgszIndexKey[cindexes][JET_cbNameMost];

    for (INT i = 0; i < cindexes; ++i)
    {
        rgindexcreateold[i].cbStruct = sizeof(rgindexcreateold[0]);
        rgindexcreateold[i].grbit = NO_GRBIT;
        rgindexcreateold[i].ulDensity = 100-i;
        rgindexcreateold[i].lcid = 0;
        rgindexcreateold[i].cbVarSegMac = 0;

        rgindexcreateold[i].szIndexName = rgszIndexName[i];
        rgindexcreateold[i].szKey = rgszIndexKey[i];
        OSStrCbFormatA( rgindexcreateold[i].szIndexName, JET_cbNameMost, "Index-%d", i);
        OSStrCbFormatA( rgindexcreateold[i].szKey, JET_cbNameMost, "+col%2.2d\0", i);
        rgindexcreateold[i].cbKey = 8;
    }

    CAutoINDEXCREATE1To2_T< JET_INDEXCREATE_A, JET_INDEXCREATE2_A > idx;

    ERR err;
    err = idx.ErrSet( (JET_INDEXCREATE_A *)rgindexcreateold, cindexes );
    CHECK( JET_errSuccess == err );

    JET_INDEXCREATE2_A * rgindexcreatenew = idx.GetPtr();
    CHECK( NULL != rgindexcreatenew );

    for (INT i = 0; i < cindexes; ++i)
    {
        CHECK( sizeof(JET_INDEXCREATE2_A) == rgindexcreatenew[i].cbStruct );
        CHECK( 0 == LOSStrCompareA( rgindexcreatenew[i].szIndexName, rgindexcreateold[i].szIndexName ) );
        CHECK( rgindexcreatenew[i].ulDensity == rgindexcreateold[i].ulDensity );
    }

    for (INT i = 0; i < cindexes; ++i)
    {
        rgindexcreatenew[i].err = ErrERRCheck( JET_errOutOfMemory );
    }

    idx.Result();

    for (INT i = 0; i < cindexes; ++i)
    {
        CHECK( JET_errOutOfMemory == rgindexcreateold[i].err );
    }
}

ERR ErrIDXCheckUnicodeFlagAndDefn(
    __in_ecount( cIndexCreate ) JET_INDEXCREATE3_A * pindexcreate,
    __in ULONG                               cIndexCreate );

JETUNITTEST( JetApi, CAutoINDEXCREATE2To3 )
{
    const INT cindexes = 3;
    JET_INDEXCREATE2_A rgindexcreateold[cindexes];
    char rgszIndexName[cindexes][JET_cbNameMost];
    char rgszIndexKey[cindexes][JET_cbNameMost];

    for (INT i = 0; i < cindexes; ++i)
    {
        rgindexcreateold[i].cbStruct = sizeof(rgindexcreateold[0]);
        rgindexcreateold[i].grbit = NO_GRBIT;
        rgindexcreateold[i].ulDensity = 100-i;
        rgindexcreateold[i].lcid = 0;
        rgindexcreateold[i].cbVarSegMac = 0;
        rgindexcreateold[i].pidxunicode = NULL;
        rgindexcreateold[i].ptuplelimits = NULL;

        rgindexcreateold[i].szIndexName = rgszIndexName[i];
        rgindexcreateold[i].szKey = rgszIndexKey[i];
        OSStrCbFormatA( rgindexcreateold[i].szIndexName, JET_cbNameMost, "Index-%d", i);
        OSStrCbFormatA( rgindexcreateold[i].szKey, JET_cbNameMost, "+col%2.2d\0", i);
        rgindexcreateold[i].cbKey = 8;
    }

    const LANGID    langidFrenchCanadian    = (0x03 << 10 ) | 0x0c;
    const LCID      lcidFrenchCanadian      = 0 | langidFrenchCanadian;

    JET_UNICODEINDEX unicodeindex;
    unicodeindex.dwMapFlags = 0x10004;
    unicodeindex.lcid = lcidFrenchCanadian;
    rgindexcreateold[1].grbit |= JET_bitIndexUnicode;
    rgindexcreateold[1].pidxunicode = &unicodeindex;

    rgindexcreateold[2].lcid = lcidFrenchCanadian;

    CAutoINDEXCREATE2To3_T< JET_INDEXCREATE2_A, JET_INDEXCREATE3_A > idx;

    ERR err;
    err = idx.ErrSet( (JET_INDEXCREATE2_A *)rgindexcreateold, cindexes );
    CHECK( JET_errSuccess == err );

    JET_INDEXCREATE3_A * rgindexcreatenew = idx.GetPtr();
    CHECK( NULL != rgindexcreatenew );

    for (INT i = 0; i < cindexes; ++i)
    {
        CHECK( sizeof(JET_INDEXCREATE3_A) == rgindexcreatenew[i].cbStruct );
        CHECK( 0 == LOSStrCompareA( rgindexcreatenew[i].szIndexName, rgindexcreateold[i].szIndexName ) );
        CHECK( rgindexcreatenew[i].ulDensity == rgindexcreateold[i].ulDensity );
    }

    for (INT i = 0; i < cindexes; ++i)
    {
        rgindexcreatenew[i].err = ErrERRCheck( JET_errOutOfMemory );
    }

    for (INT i = 0; i < cindexes; ++i)
    {
        printf( "rgindexcreatenew[%d].pidxunicode is (szLocaleName=%ws, dwMapFlags=%#x).\n",
                i,
                rgindexcreatenew[i].pidxunicode ? rgindexcreatenew[i].pidxunicode->szLocaleName : L"<null>",
                rgindexcreatenew[i].pidxunicode ? rgindexcreatenew[i].pidxunicode->dwMapFlags : 0);
    }

    CHECK( nullptr == rgindexcreatenew[0].pidxunicode );

    CHECK( 0 == wcscmp( rgindexcreatenew[1].pidxunicode->szLocaleName, L"fr-CA" ) );
    CHECK( 0x10004 == rgindexcreatenew[1].pidxunicode->dwMapFlags );

    CHECK( 0 == wcscmp( rgindexcreatenew[2].pidxunicode->szLocaleName, L"fr-CA" ) );
    CHECK( 0x0 == rgindexcreatenew[2].pidxunicode->dwMapFlags );

    CHECK( JET_errSuccess == ErrIDXCheckUnicodeFlagAndDefn( rgindexcreatenew, cindexes ) );

    idx.Result();

    for (INT i = 0; i < cindexes; ++i)
    {
        CHECK( JET_errOutOfMemory == rgindexcreateold[i].err );
    }
}

JETUNITTEST( JetApi, SetShrinkDatabaseParam )
{
    const JET_GRBIT grbitAll = JET_bitShrinkDatabaseOff | JET_bitShrinkDatabaseOn | JET_bitShrinkDatabaseRealtime | JET_bitShrinkDatabasePeriodically;
    const JET_GRBIT grbitInvalid = ~grbitAll;
    struct {
        JET_GRBIT grbitIn;
        JET_ERR errExpected;
} rgtestcases[] =
{
        { JET_bitShrinkDatabaseOff, JET_errSuccess },
        { JET_bitShrinkDatabaseRealtime, JET_errInvalidGrbit },
        { JET_bitShrinkDatabasePeriodically, JET_errInvalidGrbit },
        { JET_bitShrinkDatabaseRealtime | JET_bitShrinkDatabasePeriodically, JET_errInvalidGrbit },

        { JET_bitShrinkDatabaseOn, JET_errSuccess },
        { JET_bitShrinkDatabaseOn | JET_bitShrinkDatabaseRealtime, JET_errSuccess },
        { JET_bitShrinkDatabaseOn | JET_bitShrinkDatabasePeriodically, JET_errSuccess },
        { JET_bitShrinkDatabaseOn | JET_bitShrinkDatabaseRealtime | JET_bitShrinkDatabasePeriodically, JET_errSuccess },

        { grbitInvalid, JET_errInvalidGrbit },
        { JET_bitShrinkDatabaseOn | grbitInvalid, JET_errInvalidGrbit },
};

    CJetParam* pjetparam = new CJetParam();

    if ( pjetparam != NULL )
    {
        for ( size_t i = 0; i < _countof( rgtestcases ); ++i )
        {
            const ERR errActual = SetShrinkDatabaseParam( pjetparam, NULL, NULL, rgtestcases[ i ].grbitIn, NULL );
            CHECK( rgtestcases[ i ].errExpected == errActual );

            if ( errActual == JET_errSuccess )
            {
                ULONG_PTR ulActual;
                CHECKCALLS( pjetparam->GetInteger( pjetparam, NULL, NULL, &ulActual, NULL, 0 ) );

                JET_GRBIT grbitActual = (JET_GRBIT) ulActual;
                CHECK( grbitActual == rgtestcases[ i ].grbitIn );
            }
        }
    }
    delete pjetparam;
}

#endif
