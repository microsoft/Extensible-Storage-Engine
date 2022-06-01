// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "errdata.hxx"
#include "_bf.hxx"  // for JetTestHook

#ifdef ESENT
#include "slpolicylist.h"
#else
const LOCAL WCHAR g_wszLP_PARAMCONFIGURATION[] = L"ExtensibleStorageEngine-ISAM-ParamConfiguration";
#endif

/*

 Instructions for adding a JET API:

  -- ------ Native Engine Layer -------
  1) Declare the JET API header in published\inc\jethdr.w
        a) Don't forget to define the API as in the public header or private.
        b) Don't forget to surround the new API w/ the appropriate JET_VERSION ifdef
  2) Implement the JET API here (ese[nt]\src\ese\jetapi.cxx).
  3) Add an op for the operation in ese[nt]\src\inc\_jet.hxx
        Don't forget to increase opMax by one.
  4) Add an ALIAS1() and ALIAS2() entry to esent.def,
        Ensure to matching the total size (on x86) of the args for the # as the 2nd param.
  -- ------ ManagedEsent Layer -------
  A) Add the equivalent thunk to eseinterop\ese\UnpublishedNativeMethods.cs
  B) Add the equivalent API to the IJetApi interface in eseinterop\ese\UnpublishedIJetApi.cs
  C) Add the equivalent API to the JetApi implementation in eseinterop\ese\UnpublishedJetApi.cs
        Note: above the first private routine.
  D) Add the equivalent API to the outer layer eseinterop\ese\UnpublishedApi.cs
  E) [Optional] Add the helper APIs to take byte[] instead of IntPtr to
        eseinterop\ese\UnpublishedInternalApi.cs and pin / "fix" memory on call through.
  F) Other: You may have to add any enum files or new grbits used to the appropriate
        ManagedEsent files as well.
  Note: It's possible the API should be added to the (published) NativeMethods.cs, IJetApi.cs,
  JetApi.cs, Api.cs, etc files but unlikely b/c the API couldn't be published if you're
  implementing it right now.
  -- ------ Testing the API -------
  X) Add FULL API coverage of many scenarios and edge cases to some native or
        managed test.
        Note: If the testing is short it should be in accept.bat's default run.
  Y) Add test coverage to the ManagedEsent API unit test such that will excercise
        all the marshalling code in the ManagedEsent layer.
        See eseinterop/ese/making_eseinterop_changes.txt for more details.

*/

/*

Instructions for adding a JET param
  -- ------ Native Engine Layer -------
  1) Declare the JET param in published\inc\jethdr.w
  2) Modify sysparam.xml to create a defintion for the JET param
  3) Run gengen.bat to update the source files.
  -- ------ ManagedEsent Layer -------
  A) Update manually. // [2014/08/09 - SOMEONE]: Support for auto-generated ManagedEsent params will be added soon.
  */

JET_ERR ErrERRLookupErrorCategory(
    _In_ const JET_ERR errLookup,
    _Out_ JET_ERRCAT* perrortype
    )
{
    if ( !FERRLookupErrorCategory( errLookup, perrortype ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    return JET_errSuccess;
}

JET_ERR ErrERRLookupErrorHierarchy(
    _In_ JET_ERRCAT             errortype,
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
LGPOS   g_lgposRedoTrap     = lgposMax;         // Default to no redo trap.

#define SYSTEM_PIB_RFS

#endif


#ifdef SYSTEM_PIB_RFS
INT g_cSystemPibAlloc       = 0;
INT g_cSystemPibAllocFail   = 0;
#endif

/*  Jet VTFN dispatch supports.
 */
#pragma warning(disable: 4100)           /* Suppress Unreferenced parameter */


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
    const char  *szIndexName )
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
#endif  /* DEBUG */

extern const VTFNDEF vtfndefInvalidTableid =
{
    sizeof(VTFNDEF),
    0,
#ifndef DEBUG
    NULL,
#else   /* DEBUG */
    &vtdbgdefInvalidTableid,
#endif  /* !DEBUG */
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
#endif // PERFMON_SUPPORT

VOID INST::SetInstanceStatusReporting( const INT iInstance, const INT perfStatus )
{
    Assert( 0 != perfStatus );
    // Except perfStatusError, track status for events - ignoring Error state b/c it covers up what 
    // state we died in.  You'll have to forgive me consistency gods.
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
    //  Called (with tsidrPulseInfo) for every DB cache read/write.

    if ( tsidr == tsidrPulseInfo )
    {
        //  vector off to global system / process to give it a chance to announce as well   

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
    
//  All the counters are cleared during instance creation or deletion
//  So only thing we might need is InitPERFCounters function to
//  set initial values for specific counters
LOCAL VOID InitPERFCounters( INT iInstance )
{
    //  Empty so far
}

// The starting offset in the Processor-Local-Storage.
LONG g_cbPlsMemRequiredPerPerfInstance = 0;

LOCAL LONG g_lRefreshPerfInstanceList = 0;  // "Instance" here refers to performance counter instances.

// "Database ==> Instances" performance counter support.
WCHAR*      g_wszInstanceNames              = NULL;
WCHAR*      g_wszInstanceNamesOut           = NULL;
__range( 0, ( g_cpinstMax + 1 ) * ( cchPerfmonInstanceNameMax + 1 ) + 1 ) ULONG g_cchInstanceNames = 0;
BYTE*       g_rgbInstanceAggregationIDs     = NULL;
LOCAL const BYTE bInstanceNameAggregationID = 1;

// "Database ==> Databases" performance counter support.
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
    return cUsedInstances + 1; //  +1 to account for the "_Total" instance.
}

ULONG CPERFDatabaseObjects( const ULONG cIfmpsInUse )
{
    return max( 1, cIfmpsInUse );   // return at least 1 because when no IFMPs are in use, FMP::IfmpMacInUse() is zero, which
                                    // is indistinguishable from when IFMP 0 is in use, so g_cDatabases is at least 1.
}

ULONG CPERFObjectsMinReq()
{
    //  For ESE instances, we'll adjust the actual number of used perf instances at runtime, but we
    //  need at least one for the _Total counter.

    const ULONG cperfinstESEInstanceMinReq = (ULONG)CPERFESEInstanceObjects( 0 );

    //  For databases (IFMP), we'll adjust the actual number of used perf instances at runtime.

    const ULONG cperfinstIFMPMinReq = CPERFDatabaseObjects( 0 );

    //  For table classes (TCEs), we always display some internal ones, plus all the ones set by the user,
    //  even if no tables have been opened yet.

    const ULONG cperfinstTCEMinReq = (ULONG)CPERFTCEObjects();

    //  The default number of perf instances is the max of all three.

    return max( max( cperfinstESEInstanceMinReq, cperfinstIFMPMinReq ), cperfinstTCEMinReq );
}

//   The "_Total" instance is always returned.

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
    g_rgbInstanceAggregationIDs[ 0 ] = bInstanceNameAggregationID;  //  Only the "_Total" instance needs aggregation.

    if ( sizeof( WCHAR ) * ( LOSStrLengthW( szT ) + 1 ) > g_cchInstanceNames * sizeof( WCHAR ) )
    {
        Assert( fFalse );
        g_critInstanceNames.Leave();
        return;
    }
    cbT -= sizeof( WCHAR ) * ( LOSStrLengthW( szT ) + 1 );
    szT += LOSStrLengthW( szT ) + 1;

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
        cbT -= sizeof( WCHAR ) * ( LOSStrLengthW( szT ) + 1 );
        szT += LOSStrLengthW( szT ) + 1;
    }

    //  set new current
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

        cbT -= sizeof( WCHAR ) * ( LOSStrLengthW( szT ) + 1 );
        szT += LOSStrLengthW( szT ) + 1;
    }

    //  set new current
    g_cDatabases = FMP::IfmpMacInUse() + 1;

    AtomicExchange( &g_lRefreshPerfInstanceList, 1 );
    
    g_critDatabaseNames.Leave();
}


//
//      CIsamSequenceDiagLog
//

INT CIsamSequenceDiagLog::s_iDumpVersion = 2;

CIsamSequenceDiagLog::SEQFIXEDDATA CIsamSequenceDiagLog::s_uDummyFixedData = { 0 };

void CIsamSequenceDiagLog::InitSequence( _In_ const IsamSequenceDiagLogType isdltype, _In_range_( 3, 128 ) const BYTE cseqMax )
{
    ERR err = JET_errSuccess;

    Expected( cseqMax < 128 );

    //  Initialize the configuration ...

    Assert( m_isdltype == isdltype );
    m_cseqMax = cseqMax;

    OnDebug( m_tidOwner = DwUtilThreadId() );

    m_fInitialized = fTrue;

    //  Allocate (and test initial NULL) required resources ...

    Assert( !FAllocatedFixedData_() );
    if ( !m_fPreAllocatedSeqDiagInfo )
    {
        Assert( m_rgDiagInfo == NULL );     //  missing free on reuse?  This won't leak, but would be weird ...
        if( m_rgDiagInfo )                  //  but just in case, better to not leak ...
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
    Assert( m_pFixedData ); // whether allocated or not (dummy var), it is always present.

    //  Trigger the first sequence to provide the baseline stats / timings ... 

    Trigger( eSequenceStart );

    return; //  success ...

HandleError:

    Expected( m_rgDiagInfo == NULL );   // assert b/c only thing that can actually fail InitSequence() function.
    Expected( !FAllocatedFixedData_() );

    //  Technically not needed, becuase single alloc of m_rgDiagInfo  is the only thing that can fail 
    //  above, so m_rgDiagInfo must be NULL. But cleaner.
    TermSequence();

    //  Now setup for degraded / alloc failed operation ...
    OnDebug( m_tidOwner = DwUtilThreadId() );   // if we're operating in faulted mode, we still want the TID checks
    m_fAllocFailure = fTrue;
    m_fInitialized = fTrue;     // we're still initialized / active, but functions like Trigger() and FixedData() just invisibly do nothing.

    //  We leave m_cseqMax, to indicate we're configured, even if unusable, so we can act as a sufficient seed
    //  sequence to the 2nd InitSequence() initializer (below).
    m_cseqMax = cseqMax;
    Assert( m_cseqMac == 0 );
}

//  Note: We destroy / .TermSequence the seed after we suck the data.

void CIsamSequenceDiagLog::InitConsumeSequence( _In_ const IsamSequenceDiagLogType isdltype, _In_ CIsamSequenceDiagLog * const pisdlSeed, _In_range_( 3, 128 ) const BYTE cseqMax )
{
    //  Seed should have been configured with some number of pre-steps.
    Assert( pisdlSeed->m_cseqMax > 1 || pisdlSeed->m_rgDiagInfo == NULL );
    Expected( pisdlSeed->m_cseqMac < 7 );   //  Usually only a few steps (make sure we don't clobber/seed too much)
    Assert( pisdlSeed->m_cseqMac < ( cseqMax - 1 ) );

    //  First do base initialization

    InitSequence( isdltype, cseqMax );

    //  Next seed (clobbering the implicit Trigger( eSequenceStart ) in InitSequence BTW) from the pre-sequenced seed data ...

    if( m_rgDiagInfo && pisdlSeed->m_rgDiagInfo )
    {
        memcpy( m_rgDiagInfo, pisdlSeed->m_rgDiagInfo, pisdlSeed->m_cseqMac * sizeof(m_rgDiagInfo[0]) );
    }
    else
    {
        //  Perhaps we failed the alloc, then simply trigger what would have happened.
        Trigger( pisdlSeed->m_cseqMax - 1 );
    }

    //  Release memory either way.
    pisdlSeed->TermSequence();
}

void CIsamSequenceDiagLog::Trigger( _In_ const BYTE seqTrigger )
{
    //  Note this Assert() can go off if you call a "mismatching path" ... that is to say for
    //  example we call term paths from JetInit() paths ... so the init timing sequence is 
    //  initialized but the term one is not, so it's m_tidOwner == 0.  The correct answer is 
    //  to not trigger Term sequences from the Init path.  There is also a similar case for
    //  detach in attach paths.  I have solved all known cases of this.
    Assert( m_tidOwner == DwUtilThreadId() );
    //  We want this class to be able to be used in-light of alloc/memory failures.
    if( m_rgDiagInfo == NULL )
    {
        //  This actually happens regularly is well, it is for a Term(), here is the interesting stack ...
        //      ese!CIsamSequenceDiagLog::Trigger   ese\src\inc\daedef.hxx @ 3559
        //      ese!LOG::ErrLGTerm                  ese\src\ese\_log\log.cxx @ 534
        //      ese!LOG::ErrLGRSTExternalRestore    ese\src\ese\_log\logutil.cxx @ 1658
        //      ese!ErrIsamExternalRestore          ese\src\ese\_log\logutil.cxx @ 1353
        //      ese!JetExternalRestoreEx            ese\src\ese\jetapi.cxx @ 16486
        Assert( m_fAllocFailure );
        return;
    }

    if ( m_cseqMac > 0 && m_cseqMac - 1 != seqTrigger )
    {
        //  We used to have an Assert( m_cseqMac - 1 != seqTrigger ), but we removed it at one point -
        //  it was admittedly a bit hard to maintain ... however if we've been accumulating any of 
        //  the time adjustments - it is sort of a little ambigious, does the waits we accumulated 
        //  at m_cseqMac - 1 belong there, or at the new seqTrigger array element?  So we'll assert 
        //  if jumping by more than one sequence value, that there are no time adjustments lingering.
        Assert( FTriggeredStep( m_cseqMac - 1 ) || m_rgDiagInfo[ m_cseqMac - 1 ].cCallbacks == 0 );
        Assert( FTriggeredStep( m_cseqMac - 1 ) || m_rgDiagInfo[ m_cseqMac - 1 ].cThrottled == 0 );
    }

    Assert( FValidSequence_( seqTrigger ) );
    if ( FValidSequence_( seqTrigger ) )    // just in case
    {
        m_rgDiagInfo[seqTrigger].hrtEnd = HrtHRTCount();
        UtilGetCurrentDateTime( &(m_rgDiagInfo[seqTrigger].dtm) );
        m_rgDiagInfo[seqTrigger].seq = seqTrigger;
        C_ASSERT( sizeof(m_rgDiagInfo[seqTrigger].thstat) == sizeof(Ptls()->threadstats) );
        memcpy( &(m_rgDiagInfo[seqTrigger].thstat), &(Ptls()->threadstats), sizeof(m_rgDiagInfo[seqTrigger].thstat) );
        OSMemoryGetProcessMemStats( &(m_rgDiagInfo[seqTrigger].memstat) );
        extern ULONG_PTR        g_cbCacheCommittedSize; // nefariously reaching into buffer manager
        m_rgDiagInfo[seqTrigger].cbCacheMem = g_cbCacheCommittedSize;

        AssertSz( (INT)m_cseqMac <= seqTrigger + 1, "Timing sequence seems to be jumping back in time!?!?" );
        m_cseqMac = max( m_cseqMac, seqTrigger + 1 );

        //  The basic contract we use elsewhere ...
        Assert( FTriggeredSequence_( seqTrigger ) );
    }
}

void CIsamSequenceDiagLog::AddCallbackTime( const double secsCallback, const __int64 cCallbacks )
{
    Assert( m_tidOwner == DwUtilThreadId() );
    Assert( FValidSequence_( m_cseqMac ) ); // just in case
    if( m_rgDiagInfo == NULL || m_cseqMac <= 0 )
    {
        Assert( m_fAllocFailure );
        return; // see comments in Trigger() about this.
    }

    m_rgDiagInfo[m_cseqMac].cCallbacks += cCallbacks;
    m_rgDiagInfo[m_cseqMac].secInCallback += secsCallback;
}

double CIsamSequenceDiagLog::GetCallbackTime( __int64 *pcCallbacks )
{
    Assert( m_tidOwner == DwUtilThreadId() );
    Assert( FValidSequence_( m_cseqMac ) ); // just in case
    if( m_rgDiagInfo == NULL || m_cseqMac <= 0 )
    {
        Assert( m_fAllocFailure );
        *pcCallbacks = 0;
        return 0;   // see comments in Trigger() about this.
    }

    *pcCallbacks = m_rgDiagInfo[m_cseqMac].cCallbacks;
    return m_rgDiagInfo[m_cseqMac].secInCallback;
}

void CIsamSequenceDiagLog::AddThrottleTime( const double secsThrottle, const __int64 cThrottled )
{
    Assert( m_tidOwner == DwUtilThreadId() );
    Assert( FValidSequence_( m_cseqMac ) ); // just in case
    if( m_rgDiagInfo == NULL || m_cseqMac <= 0 )
    {
        Assert( m_fAllocFailure );
        return; // see comments in Trigger() about this.
    }

    m_rgDiagInfo[m_cseqMac].cThrottled += cThrottled;
    m_rgDiagInfo[m_cseqMac].secThrottled += secsThrottle;
}

double CIsamSequenceDiagLog::GetThrottleTime( __int64 *pcThrottled )
{
    Assert( m_tidOwner == DwUtilThreadId() );
    Assert( FValidSequence_( m_cseqMac ) ); // just in case
    if( m_rgDiagInfo == NULL || m_cseqMac <= 0 )
    {
        Assert( m_fAllocFailure );
        *pcThrottled = 0;
        return 0.0; // see comments in Trigger() about this.
    }

    *pcThrottled = m_rgDiagInfo[m_cseqMac].cThrottled;
    return m_rgDiagInfo[m_cseqMac].secThrottled;
}

__int64 CIsamSequenceDiagLog::UsecTimer( _In_ INT seqBegin, _In_ const INT seqEnd ) const
{
    Assert( m_tidOwner == DwUtilThreadId() );
    Expected( ( ( seqBegin + 1 ) == seqEnd ) || // today we only measure one step, or measure the whole sequence
              ( ( seqBegin == eSequenceStart ) && ( seqEnd == ( m_cseqMax - 1 ) ) ) );

    //  We want this class to be able to be used in-light of alloc/memory failures.
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
    
    // If there are some empty sequences in between, get timing from the
    // last filled sequence
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
    Assert( cbFixedData > 2 ); // NUL out
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
            cchUsed = LOSStrLengthW( pwszCurr );
            pwszCurr += cchUsed;
            cbCurrLeft -= ( cchUsed * 2 );
            //  worst case: ~ 24 + 5 [lgposes] * 22 char = 134 update

            if ( FixedData().sInitData.hrtRecoveryForwardLogs )
            {
                //  If this goes off, then I didn't quite understand the lifecycle, and the sprinted
                //  stats in the end of Init Event / 105 will be wrong.
                Assert( FTriggeredStep( eForwardLogBaselineStep ) );

                //  We append number of log gens for convenience that it took to get to forward log replay (i.e. through max committed).
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
                cchUsed = LOSStrLengthW( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );

                //  worst case: 34 + 12+6 + 12 = 70
            }

            if ( FixedData().sInitData.cReInits )
            {
                OSStrCbFormatW( pwszCurr, cbCurrLeft, L"cReInits = %d\n", FixedData().sInitData.cReInits );

                cchUsed = LOSStrLengthW( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );
            }

            OSStrCbFormatW( pwszCurr, cbCurrLeft, L"RBSOn = %d\n", (int) FixedData().sInitData.fRBSOn );

            cchUsed = LOSStrLengthW( pwszCurr );
            pwszCurr += cchUsed;
            cbCurrLeft -= ( cchUsed * 2 );
            //  worst case: +25 chars

            break;

        // total: worst case: ~230 chars, 460 bytes 

        #define ArgSplitLgposToUl( lgpos )      (ULONG)((lgpos).lGeneration), (ULONG)((lgpos).isec), (ULONG)((lgpos).ib)

        case isdltypeTerm:
            //  While by far most clients have logging on, a few don't and the lgpos will be Min, so don't sprint it then.
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
            //  No other operations have additional fixed data ... yet! ;)
            AssertSz( fFalse, "Unknown additional FixedData operation for %hc, or calling on non-active sequence.", m_isdltype );
            break;
        }
    }
    // else alloc failure, additional fixed data is lost.
}

void CIsamSequenceDiagLog::SprintTimings( _Out_writes_bytes_(cbTimeSeq) WCHAR * wszTimeSeq, _In_range_( 4, 4000 ) ULONG cbTimeSeq ) const
{
    Assert( m_tidOwner == DwUtilThreadId() );
    const BOOL fWithJetstat = fTrue;
    const BOOL fWithMemstat = fTrue;

    const BOOL fWithLineReturns = fTrue;    // leaving in, useful for debugging ...
    //  We want this class to be able to be used in-light of alloc/memory failures.
    if( m_rgDiagInfo == NULL )
    {
        OSStrCbCopyW( wszTimeSeq, cbTimeSeq, L"[*] = *." );
        return;
    }
    //  Consider doing something where we only sprint full stats if total time was > 30 or 50 or 200 ms or?  Or memory if > 300 KB?

    WCHAR * pwszCurr = wszTimeSeq;
    SIZE_T cbCurrLeft = cbTimeSeq;
    SIZE_T cchUsed;
    
    Assert( pwszCurr );
    Assert( cbTimeSeq > 2 );
    wszTimeSeq[0] = L'\0';

#ifdef DEBUG
    OSStrCbFormatW( pwszCurr, cbCurrLeft, L"%dV%d", m_isdltype, s_iDumpVersion );
    cchUsed = LOSStrLengthW( pwszCurr );
    pwszCurr += cchUsed;
    cbCurrLeft -= ( cchUsed * 2 );
    // Worst case: 4
#endif

    //  We should have at least seen the last sequence right before the SprintTimings() call.
    Assert( FTriggeredSequence_( m_cseqMax - 1 ) );

    for( ULONG seq = 1; seq < m_cseqMax; seq++ )
    {
        if ( seq > 1 || fWithLineReturns /* nice to have an initial line return too in this mode */ )
        {
            const INT errT = fWithLineReturns ?
                                ErrOSStrCbAppendW( pwszCurr, cbCurrLeft, L"\n" ) :
                                ErrOSStrCbAppendW( pwszCurr, cbCurrLeft, L", " );
            Assert( 0 == errT );
            cchUsed = LOSStrLengthW( pwszCurr );
            pwszCurr += cchUsed;
            cbCurrLeft -= ( cchUsed * 2 );
            // Worst case: 2
        }

        const __int64 usecStepTime = UsecTimer( seq - 1, seq );
        ULONG secs = ULONG( usecStepTime / 1000000 );
        ULONG usecs = ULONG( usecStepTime % 1000000 );
        //  We'll just assert if we can't fill... but we won't fail, just leaving a truncated string.
        if ( !FTriggeredSequence_( seq ) )
        {
            //  This means we actually skipped this sequence, indicate as such
            OSStrCbFormatW( pwszCurr, cbCurrLeft, L"[%d] -", seq );
        }
        else if ( usecStepTime == 0 )
        {
            //  No need for the extra decimal places
            OSStrCbFormatW( pwszCurr, cbCurrLeft, L"[%d] 0.0", seq );
        }
        else
        {
            // Because someone added the ability to skip sequences, you have to watch
            // for triggers that were never triggered, and have to walk back to the
            // sequence before now that WAS actually triggered or you get a set of
            // stats like:
            //  [3] 0.015 +(-342232, 0, 0, -3, 0, etc)  <-- where the neg counts are because of 0s in the data
            //  [4] 0.015 +(342232, 0, 0, 3, 0, etc)
            Assert( FTriggeredSequence_( seq ) );
            ULONG seqBefore = seq - 1;
            while ( !FTriggeredSequence_( seqBefore ) && seqBefore > 0 )
            {
                seqBefore--;
            }

            OSStrCbFormatW( pwszCurr, cbCurrLeft, L"[%d] %d.%06d", seq, secs, usecs );

            if ( m_rgDiagInfo[seq].thstat.cusecPageCacheMiss - m_rgDiagInfo[seqBefore].thstat.cusecPageCacheMiss )
            {
                cchUsed = LOSStrLengthW( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );

                OSStrCbFormatW( pwszCurr, cbCurrLeft, L" -%0.06I64f (%d) CM",
                        double( m_rgDiagInfo[seq].thstat.cusecPageCacheMiss - m_rgDiagInfo[seqBefore].thstat.cusecPageCacheMiss ) / 1000000.0,
                        m_rgDiagInfo[seq].thstat.cPageCacheMiss - m_rgDiagInfo[seqBefore].thstat.cPageCacheMiss );
            }
            if ( m_rgDiagInfo[seq].secInCallback != 0.0 )
            {
                cchUsed = LOSStrLengthW( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );

                OSStrCbFormatW( pwszCurr, cbCurrLeft, L" -%0.06f (%I64d) CB", m_rgDiagInfo[seq].secInCallback, m_rgDiagInfo[seq].cCallbacks );
            }
            if ( m_rgDiagInfo[seq].secThrottled != 0.0 )
            {
                cchUsed = LOSStrLengthW( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );

                OSStrCbFormatW( pwszCurr, cbCurrLeft, L" -%0.06f (%I64d) TT", m_rgDiagInfo[seq].secThrottled, m_rgDiagInfo[seq].cThrottled );
            }
            if ( m_rgDiagInfo[seq].thstat.cusecWait - m_rgDiagInfo[seqBefore].thstat.cusecWait )
            {
                cchUsed = LOSStrLengthW( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );

                OSStrCbFormatW( pwszCurr, cbCurrLeft, L" -%0.06I64f (%d) WT",
                        double( m_rgDiagInfo[seq].thstat.cusecWait - m_rgDiagInfo[seqBefore].thstat.cusecWait ) / 1000000.0,
                        m_rgDiagInfo[seq].thstat.cWait - m_rgDiagInfo[seqBefore].thstat.cWait );
            }
        }
        //  from above formats ...
        cchUsed = LOSStrLengthW( pwszCurr );
        pwszCurr += cchUsed;
        cbCurrLeft -= ( cchUsed * 2 );
        // Worst case: ~26 chars (assuming 2-digit seq num)

        if ( FTriggeredSequence_( seq ) )
        {
            // Because someone added the ability to skip sequences, you have to watch
            // for triggers that were never triggered, and have to walk back to the
            // sequence before now that WAS actually triggered or you get a set of
            // stats like:
            //  [3] 0.015 +(-342232, 0, 0, -3, 0, etc)  <-- where the neg counts are because of 0s in the data
            //  [4] 0.015 +(342232, 0, 0, 3, 0, etc)
            ULONG seqBefore = seq - 1;
            while ( !FTriggeredSequence_( seqBefore ) && seqBefore > 0 )
            {
                seqBefore--;
            }

            //  Consider moving this to trigger and only track deltas from last sequence, and accumulate the string at 
            //  Trigger!  Very cool, would save mem but complicated by the ability to initialize off an already done 
            //  sequence of course such as how we handle OS Init before we have an instance to initialize.

            if ( fWithJetstat &&
                    //  make sure that there is a stat delta worth printing
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
                //  could minimize this by only printing each tuple if non-zero ... 
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
                cchUsed = LOSStrLengthW( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );
                // Worst case: ~130
            }
            else
            {
                // indicate nothing
                OSStrCbFormatW( pwszCurr, cbCurrLeft, L" +J(0)" );
                cchUsed = LOSStrLengthW( pwszCurr );
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
                    //  make sure that there is a stat delta worth printing
                    ( dckbCacheMem ||
                        m_rgDiagInfo[seq].memstat.cPageFaultCount - m_rgDiagInfo[seqBefore].memstat.cPageFaultCount ||
                        dckbWorkingSetSize ||
                        dckbWorkingSetSizePeak ||
                        dckbPagefileUsage ||
                        dckbPagefileUsagePeak ||
                        dckbPrivateUsage ) )
            {
                OSStrCbFormatW( pwszCurr, cbCurrLeft, L" +M(C:%I64dK, Fs:%d, WS:%IdK # %IdK, PF:%IdK # %IdK, P:%I64dK)",
                                    dckbCacheMem,
                                    m_rgDiagInfo[seq].memstat.cPageFaultCount - m_rgDiagInfo[seqBefore].memstat.cPageFaultCount,
                                    dckbWorkingSetSize,
                                    dckbWorkingSetSizePeak,
                                    dckbPagefileUsage,
                                    dckbPagefileUsagePeak,
                                    dckbPrivateUsage );
                cchUsed = LOSStrLengthW( pwszCurr );
                pwszCurr += cchUsed;
                cbCurrLeft -= ( cchUsed * 2 );
                // Worst case: ~75
            }

            if ( m_isdltype == isdltypeInit )
            {
                //  We append a few special extra pieces of info to some steps of init
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
                    cchUsed = LOSStrLengthW( pwszCurr );
                    pwszCurr += cchUsed;
                    cbCurrLeft -= ( cchUsed * 2 );
                    // Worst case: ~21
                }
            }
        }   //  if ( FTriggeredSequence_( seq ) ) / i.e. if useful stats / sequence ...

    }   //  for

    OSStrCbAppendW( pwszCurr, cbCurrLeft, L"." );
}

void CIsamSequenceDiagLog::TermSequence()
{
    //  We do extra TermSequences, so can't Assert( m_tidOwner == DwUtilThreadId() ) here ...
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
        m_trxOldestCached( trxMax ),
        m_critTrxOldestCached( CLockBasicInfo( CSyncBasicInfo( szTrxOldestCached ), rankTrxOldestCached, 0 ) ),
        m_fTrxOldestCachedMayBeStale( fTrue ),
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

    C_ASSERT( sizeof( m_rgfStaticBetaFeatures[0] ) == sizeof( LONG ) ); // atomic ops used in os layer
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
    // close system sessions
    m_critLNPPIB.Enter();
    if ( NULL != m_plnppibEnd )
    {
        Assert( 0 == m_cUsedSystemPibs );   // otherwise there is a thread still using a system PIB
        Assert( NULL != m_plnppibBegin );
        //  all the system session must be returned to the session pool
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
        //  all the sessions should be terminated already
        //  so just free nodes
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

    RBSResourcesCleanUpFromInst( this );

    delete m_pfsapi;
    delete m_pfsconfig;

    Assert( m_cIOReserved >= 0 );
    while ( m_cIOReserved > 0 )
    {
        UnreserveIOREQ();
    }

    m_isdlInit.TermSequence();
    m_isdlTerm.TermSequence();

    //  Clear the instance's set of perfmon counters.
    //  BUG: this doesn't accomplish what it thinks it does. This will zero out all memory that stores any iInstance equal to m_iInstance.
    //  That means any ifmp and tce which are numerically equal to m_iInstance. With the current code, it is not possible to zero out
    //  memory only for a specific ESE instance (as opposed to a perf counter instance, which is what this is doing).
    //
    //  So... we have to choose between:
    //      1- Leave garbage behind.
    //      2- Zero out data from tces and ifmps which could be completely unrelated to this ESE instance.
    //
    //  Conceptually, I would go with #1, but as of the date of this comment (2017/06/21), this code already had this problem
    //  so I'd rather keep the current behavior.
    //
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

    // CAUTION: we are using WideCharToMultiByte using WC_DEFAULTCHAR which is with
    // information lost (using ? for non-ascii chars).
    // As a result, the szLogFilePathDebugOnly and szSystemPathDebugOnly MUST be used
    // for display information / debug only.
    //
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
    ULONG newSize = wszInstanceName ? ULONG( LOSStrLengthW(wszInstanceName) ) : 0;

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
    ULONG newSize = wszDisplayName ? ULONG( LOSStrLengthW(wszDisplayName) ) : 0;

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
    //  there are some available sessions

#ifdef SYSTEM_PIB_RFS

    //  debugging support: generate synthetic system PIB failures

    if( g_cSystemPibAllocFail
        && 0 == ( ++g_cSystemPibAlloc % g_cSystemPibAllocFail ) )
    {
        Error( ErrERRCheck( JET_errOutOfSessions ) );
    }

#endif

    // don't obtain a new session if in the instance-unavailable state,
    // because we might end up getting a stale session that previously
    // could not be rolled back
    //
    if ( FInstanceUnavailable( ) )
    {
        Call( ErrInstanceUnavailableErrorCode( ) );
    }

    if ( m_plnppibEnd != m_plnppibBegin )
    {
        *pppib = m_plnppibBegin->ppib;
#ifdef DEBUG
        m_plnppibBegin->ppib = NULL;
#endif // DEBUG
        m_plnppibBegin = m_plnppibBegin->pNext;
        goto HandleError;
    }
    else if ( m_cOpenedSystemPibs >= cpibSystemFudge )
    {
        Error( ErrERRCheck( JET_errOutOfSessions ) );
    }
    //  else try to allocate new session
    else
    {
        //  allocate new node
        Alloc( plnppib = new ListNodePPIB );

        //  allocate new session and initialize it
        Call( ErrPIBBeginSession( this, pppib, procidNil, fFalse ) );
        Call( (*pppib)->ErrSetOperationContext( &opContext, sizeof( opContext ) ) );

        m_cOpenedSystemPibs++;
        (*pppib)->grbitCommitDefault = JET_bitCommitLazyFlush;
        (*pppib)->SetFSystemCallback();
        //  add the node to the list
        Assert( NULL != m_plnppibEnd );
        plnppib->pNext  = m_plnppibEnd->pNext;
#ifdef DEBUG
        plnppib->ppib   = NULL;
#endif // DEBUG
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
        //  not expecting any warnings
        //
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
ULONG   g_cpinstMax                     = 0;    //  The total size of the usable (but not necessarily IN USE) array g_rgpinst (set before allocated in system init)

//  Tracking in-use g_rgpinst info.
ULONG   g_cpinstInit                    = 0;    //  Current number of instances in use - REMEMBER there can be holes so g_rgpinst[g_cpinstInit+1] is not guaranteed to be a free slot.
CRITPOOL< INST* > g_critpoolPinstAPI;

ULONG   g_cTermsInProgress              = 0;

POSTRACEREFLOG g_pJetApiTraceLog        = NULL;

//  PIF for the current process information

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

//  ICF for the current process' name

PM_ICF_PROC LProcFriendlyNameICFLPwszPpb;

LOCAL WCHAR g_wszProcName[cchPerfmonInstanceNameMax + 1];
LOCAL const BYTE bProcFriendlyNameAggregationID = 1;

LONG LProcFriendlyNameICFLPwszPpb( _In_ LONG icf, _Inout_opt_ void* const pvParam1, _Inout_opt_ void* const pvParam2 )
{
    switch ( icf )
    {
        case ICFInit:
        {
            // Even though we'll truncate the string at cchPerfmonInstanceNameMax,
            // the OSStrCbFormatW() will assert if it truncates the string.
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

// Max characters in any table class name's suffix that we add automatically. These
// strings are defined below.
#define cchTCESuffixMax     (10)
const WCHAR * const g_wszUnknown    = L"_Unknown";
const WCHAR * const g_wszCatalog    = L"_Catalog";
const WCHAR * const g_wszShadowCatalog  = L"_ShadowCatalog";
const WCHAR * const g_wszExtentPageCountCache = L"_ExtentPageCountCache";
const WCHAR * const g_wszPrimary    = L" (Primary)";
const WCHAR * const g_wszLV     = L" (LV)";
const WCHAR * const g_wszIndex  = L" (Indices)";
const WCHAR * const g_wszSpace = L" (Space)";

// Max characters in any table class name. JET_cbNameMost is the limit on the base
// name that callers can set using JetSetSystemParam. cchTCESuffixMax is the maximum
// our own suffixes add to that.
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
    //  Determine the highest user-defined tableclass that is in use.
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

// To reduce "noise" in Perfmon we only expose the table classes for which
// names have been set using JetSetSystemParameter on
// JET_paramTableClass*Name. The "_Unknown" instance is always returned.
//
// InitTableClassNames() is called by LTableClassNamesICFLPwszPpb. stored in rgpicfPERFICF,
// which is called by ErrUtilPerfInit(). This is called very early in the process as
// part of ErrOSInit(), which is a layering violation (see also the comment in violated.cxx).

VOID InitTableClassNames()
{
    const size_t cchTableClassNames = tceMax * (cchTCENameMax + 1) + 1;

    const JET_ERR errAllocation = g_tableclassnames.ErrAllocateIfNeeded( cchTableClassNames );

    if ( errAllocation != JET_errSuccess )
    {
        // Couldn't allocate memory, so let's bail and pretend there's nothing present.
        return;
    }

    const LONG cTCEObjects = CPERFTCEObjects();

    //  early-out if this count already matches the stored count.
    if ( cTCEObjects == g_cTableClassNames )
    {
        return;
    }

    WCHAR* const wszTableClassNames = g_tableclassnames.WszTableClassNames();

    AssertSz( wszTableClassNames != NULL, "wszTableClassNames should have been initialized in ErrUtilPerfInit." );

    WCHAR* wszT = wszTableClassNames;
    const WCHAR* const wszTEnd = wszTableClassNames + cchTableClassNames;

    //  something has changed.

    AtomicExchange( &g_lRefreshPerfInstanceList, 1 );

    //  Always return the "_Unknown" Instance
    OSStrCbFormatW( wszT, ( wszTEnd - wszT ) * sizeof( WCHAR ), L"%ws\0" , g_wszUnknown );

    wszT += LOSStrLengthW( wszT ) + 1;

    for ( TCE tce = tceMin; tce < cTCEObjects; ++tce )
    {
        //  get tableclass from TCE
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

            case tableclassExtentPageCountCache:
                wszParam = g_wszExtentPageCountCache;
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

        wszT += LOSStrLengthW( wszT ) + 1;
    }
    
    //  Remember the count, for future early-outs
    g_cTableClassNames = cTCEObjects;

    //  We are always interested in aggregating all these instances.
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
                    // If the wszTableClassNames allocation failed, return an empty string.
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

// set running mode
INLINE VOID RUNINSTSetMode( RUNINSTMODE newMode )
{
    // must be in critical section to set
    Assert ( g_critInst.FOwner() );
    // must be before any instance is started
    Assert ( 0 == g_cpinstInit );

    if ( newMode != runInstModeNoSet )
    {
        // Fix default system parameters so that we set JET_paramConfiguration
        // (off licensing policy for mobilecore) before we go into multi-inst
        // mode, and cannot set globals
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

// set running mode to one instance
INLINE VOID RUNINSTSetModeOneInst()
{
    Assert ( runInstModeNoSet == g_runInstMode );
    RUNINSTSetMode(runInstModeOneInst);
    //  force g_cpinstMax to 1
    g_cpinstMax = 1;
    g_ifmpMax = g_cpinstMax * dbidMax + cfmpReserved;
}

// set running mode to multi instance
INLINE VOID RUNINSTSetModeMultiInst()
{
    Assert ( runInstModeNoSet == g_runInstMode );
    RUNINSTSetMode(runInstModeMultiInst);
    //  NOTE: we have separate g_cpinstMax and
    //  JET_paramMaxInstances variables in case
    //  the user wants to switch back and forth between
    //  single- and multi-instance mode (g_cpinstMax keeps
    //  track of the max instances depending on what mode
    //  we're in, while JET_paramMaxInstances
    //  only keeps track of the max instances in multi-
    //  instance mode)
    g_cpinstMax = (ULONG)UlParam( JET_paramMaxInstances );
    g_ifmpMax = g_cpinstMax * dbidMax + cfmpReserved;
}

// get running mode
INLINE RUNINSTMODE RUNINSTGetMode()
{
    // can be no set mode only if no instance is active
    Assert ( runInstModeNoSet != g_runInstMode || 0 == g_cpinstInit );
    return g_runInstMode;
}

//  This checks we're in single instance mode and sets it if not.
//
//  Not every function should be able to automatically configure one inst mode, in fact
//  most should not (only implicit-inst JetRestoreA() uses this function) as they expect
//  there to exist an instance to work off of (such as implicit-inst JetBackupA()).

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

//  Check that we're in single / implicit inst mode.

LOCAL ERR ErrRUNINSTCheckOneInstMode()
{
    ERR     err     = JET_errSuccess;

    INST::EnterCritInst();
    if ( RUNINSTGetMode() == runInstModeNoSet )
    {
        Assert( g_cpinstInit == 0 );
        //  SOMEONE here: I checked and I could only find 4 instances (all variants of 
        //  JetRestore) of this called where it would expect / be OK with no mode being
        //  set.  So the callers expect no mode to be a failure (which they will fail
        //  with once they call ErrFindPinst()).
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
                m_cioOutstandingMax( dwMax ),
                m_permillageSmoothIo( dwMax )
        {
        }

    public:

        //ULONG CbZeroExtend() override
        //{
        //    return (ULONG)UlParam( m_pinst, JET_paramMaxCoalesceWriteSize );
        //}

        ULONG DtickAccessDeniedRetryPeriod() override
        {
            return (TICK)UlParam( m_pinst, JET_paramAccessDeniedRetryPeriod );
        }

        ULONG CIOMaxOutstanding() override
        {
            //  initialize this setting
            if ( m_cioOutstandingMax == dwMax )
            {
                m_cioOutstandingMax = (ULONG)UlParam( m_pinst, JET_paramOutstandingIOMax );
                    
#ifdef DEBUG
                //  if default param, mix it up a bit ... though 256 isn't really mixing it up that much ... AND
                //  the last problem we had with this param was setting it up, NOT down, so adding that ...

                if ( FDefaultParam( JET_paramOutstandingIOMax ) )
                {
                    DWORD cioT = 0;
                    switch( rand() % 5 )
                    {
                        case 0:     cioT = 324;     break;
                        case 1:     cioT = 1024;    break;
                        case 2:     cioT = 3072;    break;
                        case 3:     cioT = 10000;   break;
                        case 4:     cioT = 32764;   break;
                    }
                    m_cioOutstandingMax = min( (ULONG)UlParam( m_pinst, JET_paramOutstandingIOMax ), cioT );
                }
#endif // DEBUG

                //  the IO stack uses CMeteredSection so limit our IO max concurrency to avoid overflow

                m_cioOutstandingMax = min( m_cioOutstandingMax, CMeteredSection::cMaxActive - 1 );
            }

            return m_cioOutstandingMax;
        }

        ULONG CIOMaxOutstandingBackground() override
        {
            ULONG cioCheckpointMax = (ULONG)UlParam( m_pinst, JET_paramCheckpointIOMax );

            //  the IO stack uses CMeteredSection so limit our IO max concurrency to avoid overflow

            cioCheckpointMax = min( cioCheckpointMax, CMeteredSection::cMaxActive - 1 );

            return cioCheckpointMax;
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
            //  initialize this setting
            if ( m_permillageSmoothIo == dwMax )
            {
                // Exs: 999‰ = 99.9% Smooth, 990‰ = 99.0% Smooth, 900‰ = 90.0% Smooth.  Debug default = 0.2%
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
            BOOL fBlockCacheEnabled = fFalse;

            fBlockCacheEnabled = fBlockCacheEnabled || BoolParam( JET_paramEnableBlockCache );

            fBlockCacheEnabled = fBlockCacheEnabled || m_pinst != pinstNil && PvParam( m_pinst, JET_paramBlockCacheConfiguration );

            fBlockCacheEnabled = fBlockCacheEnabled || FBlockCacheTestEnabled();

            return fBlockCacheEnabled;
        }

        ERR ErrGetBlockCacheConfiguration( _Out_ IBlockCacheConfiguration** const ppbcconfig ) override
        {
            ERR err = JET_errSuccess;

            if ( !FBlockCacheEnabled() || m_pinst == pinstNil || !PvParam( m_pinst, JET_paramBlockCacheConfiguration ) )
            {
                Alloc( *ppbcconfig = new CInstanceBlockCacheConfiguration( m_pinst, FBlockCacheTestEnabled() ) );
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

        static BOOL FBlockCacheTestEnabled()
        {
            WCHAR wszBuf[16] = { 0 };
#pragma prefast( suppress:6237, "The rest of the conditions do not have any side effects." )
            if (    FOSConfigGet( L"DEBUG", L"BlockCacheEnabled", wszBuf, sizeof( wszBuf ) ) &&
                    wszBuf[0] &&
                    !!_wtol( wszBuf ) )
            {
                return fTrue;
            }

            return fFalse;
        }

        class CInstanceBlockCacheConfiguration : public IBlockCacheConfiguration
        {
            public:

                CInstanceBlockCacheConfiguration( _In_ INST* const pinst, _In_ const BOOL fBlockCacheTestEnabled )
                    :   m_pinst( pinst ),
                        m_fBlockCacheTestEnabled( fBlockCacheTestEnabled )
                {
                }

            public:

                ERR ErrGetCachedFileConfiguration(  _In_z_  const WCHAR* const                  wszKeyPathCachedFile, 
                                                    _Out_   ICachedFileConfiguration** const    ppcfconfig  ) override
                {
                    ERR err = JET_errSuccess;

                    Alloc( *ppcfconfig = new CInstanceCachedFileConfiguration( m_pinst, wszKeyPathCachedFile, m_fBlockCacheTestEnabled ) );

                HandleError:
                    return err;
                }

                ERR ErrGetCacheConfiguration(   _In_z_  const WCHAR* const          wszKeyPathCachingFile,
                                                _Out_   ICacheConfiguration** const ppcconfig ) override
                {
                    ERR err = JET_errSuccess;

                    Alloc( *ppcconfig = new CInstanceCacheConfiguration( m_pinst, m_fBlockCacheTestEnabled ) );

                HandleError:
                    return err;
                }

            private:

                INST* const m_pinst;
                const BOOL  m_fBlockCacheTestEnabled;
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

                CInstanceCachedFileConfiguration(   _In_    INST* const         pinst,
                                                    _In_z_  const WCHAR* const  wszKeyPathCachedFile,
                                                    _In_    const BOOL          fBlockCacheTestEnabled )
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

                    m_fCachingEnabled = fBlockCacheTestEnabled && m_wszAbsPathCachingFile[ 0 ] != 0;

                    if ( m_fCachingEnabled )
                    {
                        if (    UtilCmpFileName(    rgwszExt,
                                                    ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ?
                                                        wszOldLogExt : 
                                                        wszNewLogExt ) == 0 ||
                                UtilCmpFileName( rgwszExt, wszResLogExt ) == 0 ||
                                UtilCmpFileName( rgwszExt, wszSecLogExt ) == 0 ||
                                UtilCmpFileName( rgwszExt, wszRBSExt ) == 0 )
                        {
                            m_cbBlockSize = cbLogFileHeader;
                            m_ulPinnedHeaderSizeInBytes = 1 * m_cbBlockSize;
                        }
                        else if ( UtilCmpFileName(  rgwszExt,
                                                    ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ?
                                                        wszOldChkExt : 
                                                        wszNewChkExt ) == 0 )
                        {
                            m_cbBlockSize = cbCheckpoint;
                            m_ulPinnedHeaderSizeInBytes = 2 * m_cbBlockSize;
                        }
                        else if ( UtilCmpFileName( rgwszExt, L".jfm" ) == 0 )  //  CFlushMap::s_wszFmFileExtension
                        {
                            m_cbBlockSize = 8192;  //  CFlushMap::s_cbFlushMapPageOnDisk
                            m_ulPinnedHeaderSizeInBytes = 1 * m_cbBlockSize;
                        }
                        else
                        {
                            m_cbBlockSize = (ULONG)UlParam( JET_paramDatabasePageSize );
                            m_ulPinnedHeaderSizeInBytes = 2 * m_cbBlockSize;
                        }
                    }
                }
        };

        class CInstanceCacheConfiguration : public CDefaultCacheConfiguration
        {
            public:

                CInstanceCacheConfiguration( _In_ INST* const pinst, _In_ const BOOL fBlockCacheTestEnabled )
                {
                    GetCachingFileFromInst( pinst, m_wszAbsPathCachingFile );

                    m_fCacheEnabled = fBlockCacheTestEnabled && m_wszAbsPathCachingFile[ 0 ] != 0;

                    m_cbCachedFilePerSlab = (ULONG)UlParam( JET_paramDatabasePageSize );
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
        ULONG       m_cioOutstandingMax;
        ULONG       m_permillageSmoothIo;
};

CInstanceFileSystemConfiguration g_fsconfigGlobal( pinstNil );
IFileSystemConfiguration* const g_pfsconfigGlobal = &g_fsconfigGlobal;
CReaderWriterLock g_rwlOSUInit( CLockBasicInfo( CSyncBasicInfo( szOSUInit ), rankOSUInit, 0 ) );

//  Does a lot of stuff:
//      Does OSU Init
//      Does System Init
//      Finds a free instance slot in g_rgpinst
//      Allocates a new INST
//      Fills out most of the INST:: members.
//
//  This is called from four places:
//      JetCreateInstance
//      JetInit
//      JetRestoreInstanceEx/JetExternalRestoreInstance
//
//  Some notes (that probably no one should rely on) ...
//      On JetCreateInstance() the wszDisplayName is non-NULL, for all other calls it is NULL.
//      On JetRestoreInstanceEx()/JetExternalRestoreInstance() pipinst is non-NULL
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

    //  Could even pass this isdlSysTimings into NewInst, but that would only really cover the 
    //  grabbing of ::CritInst.
    isdlSysTimings.InitSequence( isdltypeOsinit, eInitSelectAllocInstDone + 1 );

    //  initialize the system if we are creating the first instance
    //

    if ( 0 == g_cpinstInit )
    {
        // OSUInit's done my misc APIs may not have been done with the correct global params,
        // so make sure they are done and have ErrNewInst do its own OSU init.
        g_rwlOSUInit.EnterAsWriter();
        err = ErrOSUInit();
        g_rwlOSUInit.LeaveAsWriter();
        Call( err );
        fOSUInit = fTrue;
        isdlSysTimings.Trigger( eInitOslayerDone );

        Call( ErrIsamSystemInit() );
        fSysInit = fTrue;
        isdlSysTimings.Trigger( eInitIsamSystemDone );
    }

    //  See if g_rgpinst still have space to hold the pinst.
    //
    if ( g_cpinstInit >= g_cpinstMax )
    {
        Error( ErrERRCheck( JET_errTooManyInstances ) );
    }

    //  check that this instance has a unique name
    //

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
             0 == LOSStrCompareW( wszInstanceName, g_rgpinst[ ipinst ]->m_wszInstanceName ) )
        {
            Error( ErrERRCheck( JET_errInstanceNameInUse ) );
        }
    }

    //  find an empty slot in the global table for this new INST
    //
    for ( ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
    {
        if ( pinstNil == g_rgpinst[ ipinst ] )
        {
            break;
        }
    }
    Assert( g_cpinstMax > ipinst );

    //  make sure we have enough memory to accumulate perf counters for this instance.
    //
    if ( !g_fDisablePerfmon && !PLS::FEnsurePerfCounterBuffer( CPERFESEInstanceObjects( ipinst + 1 ) * g_cbPlsMemRequiredPerPerfInstance ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    //  allocate a new INST
    //
    Alloc( pinst = new INST( ipinst ) );

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlInitBegin|sysosrtlContextInst, pinst, &ipinst, sizeof(ipinst) );
    isdlSysTimings.Trigger( eInitSelectAllocInstDone );

    pinst->m_isdlInit.InitConsumeSequence( isdltypeInit, &isdlSysTimings, eInitSeqMax );

    //  clone all parameters for the new INST
    //
    //  NOTE:  if we are in single instance mode then we will use the global
    //  parameter array because we will have exclusive access to it
    //

    // Sanity check for the system params
    //
#ifdef DEBUG
    for ( size_t iparam = 0; iparam < g_cparam; iparam++ )
    {
        Assert( iparam == g_rgparam[ iparam ].m_paramid );

        if ( g_rgparam[ iparam ].Type_() == JetParam::typeBoolean ||
             g_rgparam[ iparam ].Type_() == JetParam::typeGrbit ||
             g_rgparam[ iparam ].Type_() == JetParam::typeInteger ||
             g_rgparam[ iparam ].Type_() == JetParam::typeBlockSize )
        {
            // Weird if default(s) or current value did not fit in the valid range of values.
            Expected( g_rgparam[ iparam ].m_valueDefault[0] >= g_rgparam[ iparam ].m_rangeLow );
            Expected( g_rgparam[ iparam ].m_valueDefault[0] <= g_rgparam[ iparam ].m_rangeHigh );
            Expected( g_rgparam[ iparam ].m_valueDefault[1] >= g_rgparam[ iparam ].m_rangeLow );
            Expected( g_rgparam[ iparam ].m_valueDefault[1] <= g_rgparam[ iparam ].m_rangeHigh );
            Expected( g_rgparam[ iparam ].m_valueCurrent >= g_rgparam[ iparam ].m_rangeLow );
            Expected( g_rgparam[ iparam ].m_valueCurrent <= g_rgparam[ iparam ].m_rangeHigh );
        }
        
        // Check param consistency: if the param can't be changed after engine-init, it can't possibly be
        // instance-wide because there's no way to have an instance created without the engine being initialized.
        AssertSz( g_rgparam[ iparam ].FGlobal() || !g_rgparam[ iparam ].m_fMayNotWriteAfterGlobalInit,
        "Inconsistent param (%d). Why do you have a param that claims to be instance wide, but can't be changed after engine system init?  That effectively makes it global, mark it as such.\n", iparam);
    }
#endif // DEBUG

    Alloc( pinst->m_rgparam = new CJetParam[ g_cparam ] );
    // Clone parameters.
    for ( size_t iparam = 0; iparam < g_cparam; iparam++ )
    {
        Call( g_rgparam[ iparam ].Clone( pinstNil, ppibNil, &pinst->m_rgparam[ iparam ], pinst, ppibNil ) );
    }

    //  if user did not set global param and we are out of downgrade window, 
    //      then allow us to use JET_efvUseEngineDefault, 
    //      otherwise stick to JET_efvExchange2016Cu1Rtm to be safe (allow all possible future downgrades).
    //
    if ( !g_rgparam[ JET_paramEngineFormatVersion ].FWritten() )
    {
        BOOL fExpired = fFalse;
        if ( ErrUtilOsDowngradeWindowExpired( &fExpired ) >= JET_errSuccess && // to be safe, failure == not expired
             fExpired )
        {
            //  Ridiculously confusing but the downgrade window is expired, meaning downgrade can no longer happen, so allow
            //  ESE to use the engine default version.
            Expected( JET_efvUseEngineDefault == UlParam( pinst, JET_paramEngineFormatVersion ) ); // but JIC, we Reset to value.
            pinst->m_rgparam[ JET_paramEngineFormatVersion ].Reset( pinst, JET_efvUseEngineDefault );
        }
        else
        {
            //  It would be nice to use this,
            //     pinst->m_rgparam[ JET_paramEngineFormatVersion ].Reset( pinst, JET_efvUsePersistedFormat );
            //  but if we're creating a _new_ DB, and we happen to be in the downgrade window the new DB would use engine 
            //  default (i.e. upgradeed) and then if downgrade happens, we couldn't attach it, so we actually sort of need 
            //  to set this back to an old / safe version.
            pinst->m_rgparam[ JET_paramEngineFormatVersion ].Reset( pinst, JET_efvExchange2016Cu1Rtm | JET_efvAllowHigherPersistedFormat );
        }
    }

    //  init the members of the new INST that can fail
    //
    if ( !pinst->FSetInstanceName( wszInstanceName ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    if ( !pinst->FSetDisplayName( wszDisplayName ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    //  this allocates 64 rwlTrx per processor
    //  pass PIB size rounded up to allocation alignment used by cresmgr
    if ( !pinst->m_rwlpoolPIBTrx.FInit( 64 * OSSyncGetProcessorCount(), roundup( sizeof(PIB), 32 ), rankPIBTrx, szPIBTrx ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    Alloc( pinst->m_pfsconfig = new CInstanceFileSystemConfiguration( pinst ) );
    Call( ErrOSFSCreate( pinst->m_pfsconfig, &pinst->m_pfsapi ) );

    Alloc( pinst->m_plnppibEnd = new INST::ListNodePPIB );
#ifdef DEBUG
    pinst->m_plnppibEnd->ppib = NULL;
#endif // DEBUG
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

    //  insert the new INST into the global table
    //
    AtomicExchangePointer( (void **)&g_rgpinst[ ipinst ], pinst );
    g_cpinstInit++;
    EnforceSz( ipinst < g_cpinstMax, "CorruptedInstArrayInNewInst" );

    //  setup our instance name for perfmon
    //
    PERFSetInstanceNames();

    //  return the new INST
    //
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

            //  on failure, restore our situation to before.
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

    // fSTInitFailed means restore failed. Since now we clean up the instance resources
    // also on error, it should be fine to release the instance
    // If we don't do that, we can reach the situation when all the g_rgpinst array is full
    // with such left "deactivated" instances and we can not start any new one
/*
    //  if it did not shut down properly, do not free this instance.
    //  keep it as used so that no one will be able to reuse it.

    if ( pinst->m_fSTInit == fSTInitFailed )
        return;
*/
    //  find the pinst.

    INST::EnterCritInst();

    for ( size_t ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
    {
        if ( g_rgpinst[ ipinst ] == pinst )
        {
            //  enter per-inst crit to make sure no one can read the pinst.

            CCriticalSection *pcritInst = &g_critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
            pcritInst->Enter();

            //  ensure no one beat us to it
            //
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
        //  setting for one instance? NULL if for global default

        if ( pinstance && g_cpinstInit && *pinstance && *pinstance != JET_instanceNil )
            *ppinst = *(INST **) pinstance;
        else
            *ppinst = pinstNil;
    }
    else
    {
        //  sesid is given, assuming the client knows what they are doing
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
            //  find the only one instance, ignore the given instance
            //  since the given one may be bogus
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
                    // instance found
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

    //  a bogus instance
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


//  ================================================================
VOID INST::SetInstanceUnavailable( const ERR err )
//  ================================================================
{
    // right now, we only set instance unavailable under 4 conditions:
    // partial db create, attach, detach and ver oom
    // when adding new cases, call OSUHAPublishEvent() before call this
    Assert( err < JET_errSuccess );

    //  in case this gets called multiple times,
    //  only record the initial error
    //  UNDONE: there's a concurrency hole
    //  where multiple threads may try to set
    //  this for the first time, but it's not
    //  a big deal
    //
    if ( JET_errSuccess == m_errInstanceUnavailable )
    {
        m_errInstanceUnavailable = err;

        //  on LogWriteFail, see if we can't determine
        //  a more precise error
        //
        ERR errLog;
        if ( JET_errLogWriteFail == err
            && m_plog->FNoMoreLogWrite( &errLog ) )
        {
            if ( JET_errSuccess != errLog )
            {
                m_errInstanceUnavailable = errLog;
            }
        }

        //  a remount of this instance could fix the instance unavailable error
        //
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

#endif // PROFILE_JET_API



class APICALL
{
    protected:
        ERR         m_err;
        INT         m_op;       // 2014/11/03-SOMEONE - To make the change easier, we cache the op so that we don't have to modify every call site.
        INT         m_opOuter;  // This is the saved op of the "outer" JET API when we come into another JET API from a JET callback.
        IOREASONTERTIARY m_iortOuter;

#ifdef PROFILE_JET_API
        INT         m_profile_opCurrentOp;
        CHAR *      m_profile_szCurrentOp;
        HRT         m_profile_hrtStart;
#endif

        void SetJetOpInTraceContext( const INT op )
        {
            // Special case, we are going to modify the the JetOp stored in the trace context
            // No one else is supposed to modify it, so we can work without establishing a TraceContextScope
            TraceContext* ptc = const_cast<TraceContext*>( PetcTLSGetEngineContext() );

            // ioru must not be set, if we are not in a callback.
            // ioru might not be set even if we are in a callback (callbacks may come from threadpool threads that don't have the original op set).
            Assert( ptc->iorReason.Ioru() == ioruNone || Ptls()->fInCallback );
            Assert( op <= ioruMax );
            m_opOuter = ptc->iorReason.Ioru();
            ptc->iorReason.SetIoru( (IOREASONUSER) op );
            m_iortOuter = ptc->iorReason.Iort();
            ptc->iorReason.SetIort( iortISAM );
        }

        void ClearJetOpInTraceContext()
        {
            // Special case, we are going to modify the the JetOp stored in the trace context
            // No one else is supposed to modify it, so we can work without establishing a TraceContextScope
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
            //  UNDONE: need to specify the
            //  name of the op as the second
            //  param to the CProfileCounter
            //  constructor
            if ( opMax != op
                && ( profile_detailLevel & PROFILE_LOG_JET_OP )
                && NULL != profile_pFile )
            {
                m_profile_opCurrentOp = op;
//              m_profile_szCurrentOp = #op;    //  UNDONE: record API name
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
                    m_profile_opCurrentOp,      //  UNDONE: print m_profile_szCurrentOp instead
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
};  //  APICALL

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

        // Used in ErrTermComplete() to ensure that m_pinst isn't accessed after we call Term on it.
        // Can't make any guarantees about the instance state after Term has been called.
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
        const UserTraceContext* const   m_putcOuter;    // user context of the outer session, if we re-entered the JET API from a callback on a different session

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
            //  UNDONE: this check is a hack for single-
            //  instance mode to attempt to minimise the
            //  concurrency holes that exists when API
            //  calls are made while the instance is
            //  terminating
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

                //  if someone else is already in the Jet API with
                //  this session and this is not a callback, then
                //  report a session-sharing violation
                //
                //  (UNDONE: this means that we don't detect
                //  session-sharing violations within callbacks)
                //
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
                    if ( !FNegTest( fInvalidAPIUsage ) )
                    {
                        FireWall( "SessionSharingViolationEnterApi" );
                    }
                    AtomicDecrement( &m_ppib->m_cInJetAPI );
                    SetErr( ErrERRCheck( JET_errSessionSharingViolation ) );
                }

                //  Must check fTermInProgress again in case
                //  it got set after we incremented m_cInJetAPI
                //
                else if ( pinst->m_fTermInProgress )
                {
                    AtomicDecrement( &m_ppib->m_cInJetAPI );
                    SetErr( ErrERRCheck( JET_errTermInProgress ) );
                }
                else
                {
                    // Do not cache TLS in PIB if this is a callback, either it is already cached
                    // or we do not want to cache it because we are in parallel index rebuild and 
                    // the same PIB will be reused by multiple threads concurrently.
                    //
                    if ( !ptls->fInCallback )
                    {
                        m_ppib->ptlsApi = ptls;
                    }
                    else
                    {
                        Assert( m_ppib->ptlsApi == ptls || m_ppib->ptlsApi == NULL );
                    }

                    // The current user context in the TLS is saved in m_putcOuter above in the constructor
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
                Assert( PutcTLSGetUserContext() == NULL );   // PIBEndSession() should've cleaned up the TLS
            }
            else
            {
                //  if EndSession succeeded, the session is no longer
                //  alive (the PIB is freed). m_ppib can't be dereferenced.
                //  But if it failed the session is still alive, so we must still
                //  maintain the tc and API refcount properly.
                //
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
        //  note that init might have gotten in and succeeded by
        //  now, but return NotInit anyways
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

//  can enter API even if it's not initialised (but not if it's in progress)
ERR INST::ErrAPIEnterWithoutInit( const BOOL fAllowInitInProgress )
{
    ERR     err;
    LONG    lOld    = AtomicExchangeAdd( &m_cSessionInJetAPI, 1 );

    if ( ( lOld & maskAPILocked ) && // API can't be locked, unless ...
            //  ... unless it's checkpoint holding the lock or ...
            !( lOld & fAPICheckpointing ) &&
            //  ... or it's initializing but we're allowed in init in progress.
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

//  this function assumes we're in g_critpoolPinstAPI(&g_rgpinst[ipinst]).Crit()
BOOL INST::APILock( const LONG fAPIAction )
{
    Assert( fAPIAction & maskAPILocked );
    Assert( !(fAPIAction & maskAPISessionCount) );

    ULONG lOld = AtomicExchangeSet( (ULONG*) &m_cSessionInJetAPI, fAPIAction );
    
//  //  no one else could have the lock because we're in g_critpoolPinstAPI(&g_rgpinst[ipinst]).Crit()
//  Assert( !( lOld & maskAPILocked ) );

    if( fAPICheckpointing == fAPIAction )
    {
        if( lOld & fAPICheckpointing )
        {
            // the API was already locked for checkpointing. We didn't get get the lock
            return fFalse;
        }
        else if( lOld & maskAPILocked )
        {
            // the API was locked for a different reason. Reset the checkpoint lock
            Assert( !(lOld & fAPICheckpointing ) );
            AtomicExchangeReset( (ULONG*) &m_cSessionInJetAPI, fAPIAction );
            return fFalse;
        }
        // at this point the API is locked for checkpointing
    }
    else
    {
#ifdef DEBUG
        ULONG   cWaitForAllSessionsToLeaveJet = 0;
#endif
        Assert( !(fAPIAction & fAPICheckpointing) );
        while ( ( lOld & maskAPISessionCount ) > 1 || ( lOld & fAPICheckpointing ) )
        {
            //  session still active, wait then retry
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
    Assert(     !( cSessionInJetAPI & maskAPILocked )       // No one is holding except
            ||  ( cSessionInJetAPI & fAPICheckpointing )    // checkpointing, or
            ||  ( fAPIAction & fAPICheckpointing ) );       // Checkpointing itself is leaving
}
#endif
}

VOID INST::EnterCritInst()  { g_critInst.Enter(); }
VOID INST::LeaveCritInst()
{
    // We should have consistent g_runInstMode and g_cpinstInit parameters at this point
    Assert( ( runInstModeNoSet == g_runInstMode && 2 > g_cpinstInit ) ||
            ( runInstModeOneInst == g_runInstMode && 2 > g_cpinstInit ) ||
            ( runInstModeMultiInst == g_runInstMode ) );
    g_critInst.Leave();
}

#ifdef DEBUG
BOOL INST::FOwnerCritInst() { return g_critInst.FOwner(); }
#endif  //  DEBUG

//  ErrIsamSystemInit loops back and calls this very early on (after some config init)
ERR INST::ErrINSTSystemInit()
{
    ERR err = JET_errSuccess;

#ifdef PROFILE_JET_API
    Assert( NULL == profile_pFile );

    //  no file name specified
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
#endif // PROFILE_JET_API

    //  figure out our mode and allocate the array of pinsts appropriately ...

    Assert( INST::FOwnerCritInst() );
    Assert( 0 == g_cpinstInit );
    Assert( g_rgpinst == NULL );    // or we'll be leaking memory.


    Alloc( g_rgpinst = new INST*[g_cpinstMax] );
    memset( g_rgpinst, 0, sizeof(INST*) * g_cpinstMax );
    Assert( g_rgpinst[0] == pinstNil ); //  this is what most code assumes is NULL insts (it's not -1 BTW).

#if ENABLE_API_TRACE
    Call( ErrOSTraceCreateRefLog( 1000, 0, &g_pJetApiTraceLog ) );
#endif

    //  allocate CS storage, but not as CSs (no default initializer)

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


//  IsamSystemTerm loops back and calls this as the last thing.
VOID INST::INSTSystemTerm()
{
    Assert( 0 == g_cpinstInit );

    //  terminate CS storage
    g_critpoolPinstAPI.Term();

    //  terminate the array of pinsts ...

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

INLINE BOOL INST::FComputeRBSOn() const
{
    if ( !BoolParam( this, JET_paramEnableRBS ) )
    {
        return fFalse;
    }

    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        if ( m_mpdbidifmp[ dbid ] >= g_ifmpMax )
            continue;

        if ( g_rgfmp[ m_mpdbidifmp[ dbid ] ].FRBSOn() )
        {
            return fTrue;
        }
    }

    return fFalse;
}

//  ICF for our JET instance names

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

                    //  It is only safe to return this here because this is a constant array.
                    //  The contents are always zeroed, except for the first element, which has
                    //  always the same value. Ideally, we would like to initialize this only once
                    //  in this function, during the ICFInit phase, but the array is not allocated
                    //  until the first ESE instance gets allocated, which happens after ICFInit.
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

//  ICF for our database names

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

                    //  It is only safe to return this here because this is a constant array.
                    //  The contents are always zeroed. Ideally, we would like to initialize this only once
                    //  in this function, during the ICFInit phase, but the array is not allocated
                    //  until the first ESE instance gets allocated, which happens after ICFInit.
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

VOID JET_API DBGFPrintF( _In_ PCSTR sz )
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

        //  check for file/path collisions against all other instances
        //
        //      check for temp database collisions
        //      check for log-path collisions
        //      check for system-path collisions

        //  if temp. database will be created,
        //  check for temp database collisions

        const BOOL  fTempDbForCurrInst  = ( UlParam( g_rgpinst[ ipinst ], JET_paramMaxTemporaryTables ) > 0
                                            && !g_rgpinst[ ipinst ]->FRecovering() );
        if ( fTempDbForCurrInst && fTempDbForThisInst )
        {
            Call( pfsapi->ErrPathComplete( SzParam( g_rgpinst[ ipinst ], JET_paramTempPath ), rgwchFullNameExist ) );
            CallS( err );   // if this goes off, we have a downlevel (win7 or before) bug of leaked crit section / INST::EnterCritInst
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
                Assert( JET_errInvalidPath == errFullPath );    //  our instance has a bad temp db name/path
            }
        }

        //  don't require checkpoint file or logfiles if log is disabled
        if ( !g_rgpinst[ ipinst ]->FComputeLogDisabled()
            && fRecoveryForThisInst )
        {

            //  check for system path collisions

            Call( pfsapi->ErrPathComplete( SzParam( g_rgpinst[ ipinst ], JET_paramSystemPath ), rgwchFullNameExist ) );
            CallS( err );   // if this goes off, we have a downlevel (win7 or before) bug of leaked crit section / INST::EnterCritInst
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
                Assert( JET_errInvalidPath == errFullPath );    //  our instance has a bad system path
            }

            Call( pfsapi->ErrPathComplete( SzParam( g_rgpinst[ ipinst ], JET_paramLogFilePath ), rgwchFullNameExist ) );
            CallS( err );   // if this goes off, we have a downlevel (win7 or before) bug of leaked crit section / INST::EnterCritInst
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
                Assert( JET_errInvalidPath == errFullPath );    //  our instance has a bad log path
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




/*=================================================================
ErrInit

Description:
  This function initializes Jet and the built-in ISAM.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

=================================================================*/

LOCAL JET_ERR ErrInit(  INST        *pinst,
                        BOOL        fSkipIsamInit,
                        JET_GRBIT   grbit )
{
    JET_ERR     err     = JET_errSuccess;
    WCHAR       wszT[IFileSystemAPI::cchPathMax];

    Assert( !pinst->m_fJetInitialized );
    Assert( !pinst->m_fTermInProgress );

    pinst->m_ftInit = UtilGetCurrentFileTime();

    //  resolve our system path, which may be relative to the CWD at JetInit time.
    //  leave it as relative on an error and hope that the CWD doesn't change
    //
    Param( pinst, JET_paramSystemPath )->Set( pinst, ppibNil, 0, SzParam( pinst, JET_paramSystemPath ) );

    //  resolve our temp path, which may be relative to the CWD at JetInit time.
    //  leave it as relative on an error and hope that the CWD doesn't change.
    //  if it is just a file name then place it relative to the system path.
    //  if it is a folder path then append the default temp database file name
    //
    if ( !pinst->m_pfsapi->FPathIsRelative( SzParam( pinst, JET_paramTempPath ) ) )
    {
        Param( pinst, JET_paramTempPath )->Set( pinst, ppibNil, 0, SzParam( pinst, JET_paramTempPath ) );
    }
    else
    {
        //  a failure here used to return only JET_errOutOfMemory but can now also return JET_errInvalidParameter
        Call( pinst->m_pfsapi->ErrPathBuild( SzParam( pinst, JET_paramSystemPath ), SzParam( pinst, JET_paramTempPath ), NULL, wszT ) );
        Call( Param( pinst, JET_paramTempPath )->Set( pinst, ppibNil, 0, wszT ) );
    }

    if ( FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramTempPath ) ) )
    {
        //  this is also a new source of JET_errInvalidParameter
        Call( pinst->m_pfsapi->ErrPathBuild( SzParam( pinst, JET_paramTempPath ), wszDefaultTempDbFileName, wszDefaultTempDbExt, wszT ) );
        Call( Param( pinst, JET_paramTempPath )->Set( pinst, ppibNil, 0, wszT ) );
    }

    //  resolve our log file path, which may be relative to the CWD at JetInit time.
    //  leave it as relative on an error and hope that the CWD doesn't change
    //
    Param( pinst, JET_paramLogFilePath )->Set( pinst, ppibNil, 0, SzParam( pinst, JET_paramLogFilePath ) );

    //  check for name/path collisions
    //
    Call( ErrCheckUniquePath( pinst ) );

    //  set our resource quotas for this instance
    //
    DWORD_PTR cPIBQuota;
    cPIBQuota = UlParam( pinst, JET_paramMaxSessions ) + cpibSystem;
    if ( cPIBQuota < CQuota::QUOTA_MAX )
    {
        Call( ErrRESSetResourceParam( pinst, JET_residPIB, JET_resoperMaxUse, cPIBQuota ) );
    }

    //  the internal quota of an FCB limit is mismatched with external quota setting. we 
    //  bridge the gap by noting that FCBs get gobbled up by open tables, temporary tables and
    //  closed tables that were cached. we use some fudge factors because there isn't a one-to-one
    //  mapping between FCBs and an open table.

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

    // allocate the version store hash table
    //
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

    //  set our preferred resource thresholds
    //
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

    //  make a dummy callback to the runtime callback to verify its existence.
    //  we do this so that if it is misconfigured then we will always crash
    //
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

    // init HA publish api
    if ( UlParam( pinst, JET_paramEnableHaPublish ) )
    {
        Call( FUtilHaPublishInit() ? JET_errSuccess : ErrERRCheck( JET_errCallbackNotResolved ) );

        if ( 2 == UlParam( pinst, JET_paramEnableHaPublish ) )
        {
#ifdef USE_HAPUBLISH_API
            const WCHAR* rgwsz[] = { L"Ha Publishing started for this database.", };
#endif

            // system sanity check
            OSUHAPublishEvent(
                HaDbFailureTagNoOp, pinst, HA_GENERAL_CATEGORY,
                HaDbIoErrorNone, NULL, 0, 0,
                HA_PLAIN_TEXT_ID, _countof( rgwsz ), rgwsz );
        }
    }

    pinst->m_isdlInit.Trigger( eInitVariableInitDone );

    //  initialize the integrated ISAM
    //
    if ( !fSkipIsamInit )
    {
        Call( ErrIsamInit( JET_INSTANCE( pinst ), grbit ) );
        //  we will return a warning from ErrIsamInit() if any logs were lost ...
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

// JET_VALIDATE is disabled for retail builds of ESE because Exchange is using
// the managed layer to access our API, which makes handle mangling a lot harder.
#if defined( DEBUG ) || defined( ESENT )
#define JET_VALIDATE
#else  //  !DEBUG && !ESENT
//  Disable validation for retail builds of ESE because it is expensive
#endif  //


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
    /*  don't fail on NULL or JET_instanceNil due to single instance mode  */       \
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

#else  //  !JET_VALIDATE

#define JET_VALIDATE_INSTANCE( instance )
#define JET_VALIDATE_SESID                  JET_VALIDATE_SESID_FAST
#define JET_VALIDATE_TABLEID                JET_VALIDATE_TABLEID_FAST
#define JET_VALIDATE_SESID_TABLEID          JET_VALIDATE_SESID_TABLEID_FAST

#endif  //  JET_VALIDATE

// here are the minimal verifiers consistent with our API; to be used for particularly hot paths

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

//  ================================================================
//      JET PARAMETERS
//  ================================================================


JetParam::
~JetParam()
{
    if ( m_fFreeValue )
    {
        delete [] (void*)m_valueCurrent;
    }
}

// Set this value to a paramid value, and then anytime the value is
// Set(), Clone()d, or Reset() the program will Assert().  You can
// also set it via reg value ESE[NT]\DEBUG\"Param Trap" as well.
ULONG   g_paramidTrapParamSet = 0xFFFFFFFF;

#ifdef DEBUG
#define CheckParamTrap( paramid, szExtraneousFriendlyString )  \
    if ( ( g_paramidTrapParamSet != 0xFFFFFFFF ) &&     \
         ( g_paramidTrapParamSet == paramid ) ) \
    {   \
        Assert( !__FUNCTION__ " hit user specified g_paramidTrapParamSet." szExtraneousFriendlyString );    \
    }
#else
// this is cheap and infrequently excercised, so we could consider making it break in free as well.
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

    // UNICODE_COMPATIBILITY:
    // This is tricky, b/c JET used to just truncate the value, not return error if not enough buffer.
    // However, I (SOMEONE) think that we should change the contract, because anyone who is
    // getting a truncated string, is proably unknowingly failing in some logical way.
    //
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
    // If value is not changing, do not do any more validation, simple memory comparison for strings
    const ULONG_PTR newParam = fString ? (ULONG_PTR)wszParam : ulParam;
    if ( newParam == pjetparam->m_valueCurrent )
    {
        if ( pjetparam->Type_() == JetParam::typeBoolean ||
             pjetparam->Type_() == JetParam::typeGrbit ||
             pjetparam->Type_() == JetParam::typeInteger ||
             pjetparam->Type_() == JetParam::typeBlockSize )
        {
            // Ummm, it would be weird to provide a default value outside the allowed range of param 
            // values, but this clause skips the range check below ... however nothing really goes 
            // wrong other than a few Asserts though, and ESE will use the out of range default value, 
            // and allow you to set it to that default value, as long as it has not be set to any 
            // other value before.  This is wonky inconsistent behavior, so we will at least assert
            // the value set / when matching current is within range.
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
        const size_t cchParam = wszParam ? LOSStrLengthW( wszParam ) : 0;
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

    cchParam = wszParam ? LOSStrLengthW( wszParam ) : 0;
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

    // don't overwrite the destination if we encounter an error in the allocation
    
    if ( pjetparamSrc->m_fFreeValue )
    {
        // This assert is trying to ensure that we're only talking about strings here.
        Assert( pjetparamSrc->m_type == typeString || pjetparamSrc->m_type == typeFolder || pjetparamSrc->m_type == typePath );
        cchValue = LOSStrLengthW( (WCHAR*)pjetparamSrc->m_valueCurrent );

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

// init page size dependent static variables in various classes
//
// If a separated LV is being compressed then we don't actually
// reduce the number of pages being used by the LV unless we
// increase the number of chunks that fit on one page.
//
// For large pages the LV chunk sizes are selected so that four
// (uncompressed) chunks fit on a page. This means that if we
// compress the chunks by about 20% then 5 chunks will fit on
// a page and the number of pages used by the LV will be reduced.
//
// The table below calculates node sizes with key prefix compression,
// with following assumptions:
// 1. We assume all nodes on the page belong to the same LV. The LID portion of the key will be
//    prefix-compressed. Other cases are not interesting in the context of optimal space usage,
//    because it is highly unlikely to have LVs that are integral multiples of the chunk size
//    (we will always waste some space on the page if the page has nodes belonging to different LIDs).
// 2. We assume worst-case prefix-compression. That is, the offset part of the key can't be
//    prefix-compressed. But on the average, the lower 2 bytes will change. The rest should compress.
//
// The calculations are:
//                            Prefix-compressed
//                LID32 LID64    LID32 LID64
//  tag             4     4        4     4
//  cbKey           2     2        4     4
//  key             4     8        0     0
//  data            8     8        8     8
//  ========================================
//  LVROOT          18    22       16    16
//
//                            Prefix-compressed
//                LID32 LID64    LID32 LID64
//  tag             4     4        4     4
//  cbKey           2     2        4     4
//  key             8     12       4     4   (worst-case, offset has no common prefix)
//  ========================================
//  Chunk overhead  14    18       12    12
//
//  With an LVROOT and 4 chunks on a page:
//                                                                           Prefix-compressed
//                          ---- LID32 ----     ---- LID64 ----     ---- LID32 ----     ---- LID64 ----
//  Page Size               32,768   16,384     32,768   16,384     32,768   16,384     32,768   16,384
//  Page Header             80       80         80       80         80       80         80       80
//  External Header Tag     4        4          4        4          4+4      4+4        4+8      4+8      (+ key prefix)
//  LVROOT                  18       18         22       22         16       16         16       16
//  Chunk overhead          56       56         72       72         48       48         48       48       ( 4 * chunk overhead)
//  Data/page               32,610   16,226     32,590   16,206     32,616   16,232     32,612   16,228
//  ===================================================================================================
//  Data/chunk              8,152    4,056      8,147    4,051      8,154    4,058      8,153    4,057    ( data / 4 )
//  
//  We will base the LV chunk size off those calculations.
//  The numbers will be rounded down to:
//
//      32kb pages: 8,150
//      16kb pages: 4,050
//
// 2017/11/15-SOMEONE - With prefix-compression on LID64, current chunk size still allows us to store 4 chunks + LVROOT on 1 page.
//                     We can avoid changing the chunk size and still get optimal storage characteristics.
//
//  (The chunk size on 8kb and 4kb pages stays the same to preserve
//  backwards compatability)
//
//  With large pages the header/tag overhead is tiny. We can calculate
//  what percentage of the page that is user data (assuming the page
//  contains full chunks).
//
//  Page Size   Chunk size  Chunks/page User data
//  ----------------------------------------------
//  32,768      8,150       4           99.49%
//  16,384      4,050       4           98.88%
//  8,192       8,110       1           99.00%
//  4,096       4,014       1           98.00%
//  
//  The chunk size defines the *logical* size of an LV chunk. Now we can
//  calculate how much compression is needed to fit N+1 chunks
//  on a page. To get an LVROOT and 5 chunks on a page: 
//
//  Page Size               32,768  16,384 
//  Page Header             80      80 
//  External Header Tag     4       4 
//  LVROOT                  18      18 
//  Chunk overhead          70      70      ( 5 * chunk overhead)
//  Data/page               32,596  16,212 
//  Data/chunk              6,519   3,242   ( data / 5 )
//  ======================================
//  % Reduction Required    20.01%  19.92%
//
//  If there is no LVROOT on the page then the compression ratio
//  required drops to 19.98% for 32kb pages and 19.85% for 16kb pages.
//
//  Even without compression, reducing the chunk size allows a
//  small LV (or the final partial chunk of a large LV) to share
//  space with the first chunks of a new LV. This decreases space
//  usage, but may increase I/O as an LV will be split across two
//  pages.
//

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
            // We've subtly changed this contract ... if someone sent in a true ULONG_PTR * which is 
            // a 64-bit value then the top DWORD is no longer set to 0 by int to int64 extension.  This
            // would only changing a client if they are passing 
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

    // Note: While we accept any grbit, the ONLY grbit that is processed correctly from these
    // settings is JET_bitCommitLazyFlush.  So if the other bits are set here, they'll be ignored.
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
        // It may look like g_grbitCommitDefault isn't actually used anywhere, but
        // if it is set globally then any new instances will clone (below) the param
        // into the INST variable, where it will be used when new sessions are created.
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

//  fwd decl, but is below b/c needs access to g_rgparamRaw
ERR
ErrSetConfigStoreSpec(  CJetParam* const    pjetparam,
                INST* const             pinst,
                PIB* const              ppib,
                const ULONG_PTR         ulParam,
                PCWSTR                  wszParam );

//  List of published engine format versions and the associated last format of that release
C_ASSERT( JET_efvWindows10Rtm == ( JET_efvRollbackInsertSpaceStagedToTest + 60 /* scratch in case I missed something */ ) );
C_ASSERT( JET_efvExchange2013Rtm == ( JET_efvScrubLastNodeOnEmptyPage + 60 /* scratch in case I missed something */ ) );
C_ASSERT( JET_efvWindows10v2Rtm == JET_efvMsysLocalesGuidRefCountFixup );
C_ASSERT( JET_efvExchange2016Rtm == ( JET_efvLostFlushMapCrashResiliency + 60 /* scratch in case I missed something */ ) );
C_ASSERT( JET_efvExchange2016Cu1Rtm == 8920 /* not known last format */ );

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
        //  Probably combined JET_efvAllowHigherPersistedFormat with either JET_efvUsePersistedFormat or JET_efvUseEngineDefault
        AssertSz( fFalse, "Invalid set of flags for 0x%x", ulParam );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( ulParam != JET_efvUsePersistedFormat )
    {
        //  If not this wild card, then try to lookup desired version to ensure we can maintain
        //  the version desired / specified by the user or client.
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
                //  Stolen from normal CJetParam::ValidateSet()
                AssertSz( fFalse, "Someone is trying to set the EFV after a regular attach has been performed!  This is not allowed." );
                err = ErrERRCheck( JET_errAlreadyInitialized );
            }
        }
        FMP::LeaveFMPPoolAsWriter();

        // We could try to roll a log here for log upgrades. Ugh, I think we need a session ... not
        // guaranteed on JetSetSystemParameter() ... I just left it to attach.
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

    Assert( pinst != (INST*)JET_instanceNil );  // should be impossible, but handled just in case ... note if this does go off, there are other issues in like GetUnicodeIndexDefault.
    // pinst == pinstNil is allowed to retrieve the global and/or default value.

    JET_API_PTR efvT;
    CallS( CJetParam::GetInteger( pjetparam, pinst, ppib, (ULONG_PTR*)&efvT, NULL, sizeof(efvT) ) );
    Enforce( efvT <= (ULONGLONG)ulMax /* check truncation */ );
    JET_ENGINEFORMATVERSION efv = (JET_ENGINEFORMATVERSION)efvT;

    // If the setting is the special value to use the default setting, return the real default value.
    if ( efv == JET_efvUseEngineDefault )
    {
        efv = EfvDefault();
    }

    Assert( efv != JET_efvUseEngineDefault );   // should always return the discrete default value, not the flag.

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
    CAutoWSZ    lwszErrorValueString;   // 30 is optimization
    CAutoWSZ    lwszErrorDescString;    // 300 is optimization
    const CHAR *        szError     = NULL;
    const CHAR *        szErrorText = NULL;

    JetErrorToString( *((JET_ERR *)pulParam), &szError, &szErrorText );

    CallR( lwszErrorValueString.ErrSet( szError ) );
    CallR( lwszErrorDescString.ErrSet( szErrorText ) );

    return( ErrOSStrCbFormatW( wszParam, cbParamMax, L"%ws, %ws", (WCHAR*)lwszErrorValueString, (WCHAR*)lwszErrorDescString ) );
#endif
}

// g_fUnicodeIndexDefaultSetByUser determines if we should use the
// hardcoded default (dwLCMapFlagsDefault and wszLocaleNameDefault) if it's false.
//
// If g_fUnicodeIndexDefaultSetByUser is true then we should use the
// variables below (g_dwLCMapFlagsDefaultSetByUser and g_wszLocaleNameDefaultSetByUser). Those are
// set by JET_paramUnicodeIndexDefault.

static BOOL                 g_fUnicodeIndexDefaultSetByUser = fFalse;

static ULONG                g_dwLCMapFlagsDefaultSetByUser;             //  LC map flags
static WCHAR                g_wszLocaleNameDefaultSetByUser[NORM_LOCALE_NAME_MAX_LENGTH];   // Locale name

ERR
GetUnicodeIndexDefault( const CJetParam* const  pjetparam,
                        INST* const             pinst,
                        PIB* const              ppib,
                        ULONG_PTR* const        pulParam,
                        __out_bcount(cbParamMax) PWSTR const    wszParam,   //  ignored
                        const size_t            cbParamMax )                //  this is the size of the pulParam buffer
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

    // REVIEW: This is new behaviour. The old implementation rejected this under
    // ErrFILEICheckUserDefinedUnicode() as JET_errIndexInvalidDef. But isn't it
    // useful to be able to set the parameter to NULL and revert to the hard-coded
    // default?
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

    // We will now go through all databases attached to this instance and re-resolve the cache priority
    // for all sessions. We do this so that retrieving the priority is fast and doesn't hurt hot codepaths.
    // Otherwise, we would have to run the cache priority conflict resolution algorithm every time.

    // First, lock the PIB list.
    pinst->m_critPIB.Enter();

    // Go through all DBs attached to the instance.
    for ( DBID dbid = 0; dbid < _countof( pinst->m_mpdbidifmp ); dbid++ )
    {
        const IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
        if ( !FMP::FAllocatedFmp( ifmp ) )
        {
            continue;
        }

        // We only care if it's currently attached.
        const FMP* const pfmp = &g_rgfmp[ ifmp ];
        if ( !pfmp->FInUse() )
        {
            continue;
        }

        // Go through all sessions and resolve the priority for that database.
        for ( PIB* ppibCurr = pinst->m_ppibGlobal; ppibCurr != NULL; ppibCurr = ppibCurr->ppibNext )
        {
            ppibCurr->ResolveCachePriorityForDb( dbid );
        }
    }

    // Unlock the PIB list.
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

    //
    // -JET_bitShrinkDatabaseOff.
    // Self-explanatory. JetResizeDatabase() can't trim or shrink the file.
    // It is invalid to pass any modality bits (e.g., JET_bitShrinkDatabaseRealtime) without passing
    // JET_bitShrinkDatabaseOn combined with it.
    //
    // -JET_bitShrinkDatabaseOn.
    // Enables trim and shrink in the engine.
    // Passing this bit is required in order to enable any modality of shrink or trim.
    // 
    // -JET_bitShrinkDatabaseRealtime.
    // Trims at Space-Free.
    //
    // -JET_bitShrinkDatabasePeriodically.
    // Background task runs and trims.
    //

    const JET_GRBIT grbitValidMask = ( JET_bitShrinkDatabaseOff | JET_bitShrinkDatabaseOn | JET_bitShrinkDatabaseRealtime | JET_bitShrinkDatabasePeriodically );

    // Unknown grbit specified!
    if ( ( grbitShrink & ~grbitValidMask ) != 0 )
    {
        Error( ErrERRCheck ( JET_errInvalidGrbit ) );
    }

    // Can't pass anything if it's disabled.
    if ( ( ( grbitShrink & JET_bitShrinkDatabaseOn ) == 0 ) && ( grbitShrink != JET_bitShrinkDatabaseOff ) )
    {
        Error( ErrERRCheck ( JET_errInvalidGrbit ) );
    }

    // Anything else is a valid combination at this time, proceed.

    Call( pjetparam->SetGrbit( pjetparam, pinst, ppib, ulParam, wszParam ) );

HandleError:

    return err;
}


//
//  Configuration Sets
//

CJetParam::Config g_config = JET_configDefault;

//  Configuration Set Control

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


//  It is important (cough, SOMEONE, cough, SOMEONE, cough SOMEONE) that we do not give into our desire to
//  easily control our clients params with our binary and define only configuration sets that can be
//  abstracted to a logically sensible configuration for our engine that is not a layering violation.

//  note: This was formerly part of smallConfig
const ConfigSetOverrideValue    g_rgJetConfigRemoveQuotas[] = {
    CO( JET_paramMaxOpenTables,                 2147483647, 0x0 ),
    CO( JET_paramMaxCursors,                    2147483647, 0x0 ),
    CO( JET_paramMaxVerPages,                   2147483647, 0x0 ),
    CO( JET_paramMaxTemporaryTables,            2147483647, 0x0 ),
    CO( JET_paramPreferredVerPages,             2147483647, 0x0 ),
    CO( JET_paramMaxSessions,                   30000,      0x0 ),      //  I do not know if this is truly unlimited or not.  I _think_ it might be due to a USHORT.
    // Do not set anything here that increases memory usage necessarily, this is only for quota like
    // parameters.  Ignoring the fact that many of these arguments did FORMERLY increase memory usage.
};

/*

    NORMAL_PARAM(   JET_paramDefragmentSequentialBTrees,                        CJetParam::typeBoolean,     0,  0,  0,  0,  0,      -1,         1 ),
    NORMAL_PARAM(   JET_paramDefragmentSequentialBTreesDensityCheckFrequency,   CJetParam::typeInteger,     0,  0,  0,  1,  0,      -1,         10 ),
    NORMAL_PARAM(   JET_paramIOThrottlingTimeQuanta,                            CJetParam::typeInteger,     0,  1,  1,  1,  1,      10000,      100 ),

    NORMAL_PARAM(   JET_paramMaxCoalesceReadSize,           CJetParam::typeInteger,     1,  1,  1,  1,  0,  10 * 1024 * 1024,   cbReadSizeMax ), // max = 10MB
    NORMAL_PARAM(   JET_paramMaxCoalesceWriteSize,          CJetParam::typeInteger,     1,  1,  1,  1,  0,  10 * 1024 * 1024,   cbWriteSizeMax ), // max = 10MB
    NORMAL_PARAM(   JET_paramMaxCoalesceReadGapSize,        CJetParam::typeInteger,     1,  1,  1,  1,  0,  10 * 1024 * 1024,   cbReadSizeMax ), // max = 10MB
    NORMAL_PARAM(   JET_paramMaxCoalesceWriteGapSize,       CJetParam::typeInteger,     1,  1,  1,  1,  0,  10 * 1024 * 1024,   cbWriteSizeMax ), // max = 10MB

    NORMAL_PARAM(   JET_paramDbScanThrottle,                CJetParam::typeInteger,     1,  0,  0,  0,  0,  1000 * 60,  20 ),
    NORMAL_PARAM(   JET_paramDbScanIntervalMinSec,          CJetParam::typeInteger,     1,  0,  0,  1,  0,  60 * 60 * 24 * 21, 60 * 60 * 24 * 1 ),          // 0 sec, 3 weeks, 1 day
    NORMAL_PARAM(   JET_paramDbScanIntervalMaxSec,          CJetParam::typeInteger,     1,  0,  0,  1,  1,  60 * 60 * 24 * 365 * 50, 60 * 60 * 24 * 7 ),    // 1 sec, 50 years, 1 week

    NORMAL_PARAM(   JET_paramCachePriority,                 CJetParam::typeInteger,     1,  0,  0,  0,  g_pctCachePriorityMin,  g_pctCachePriorityMax,  g_pctCachePriorityDefault ),    // min = 0, max = 1000, default = 100
    NORMAL_PARAM(   JET_paramMaxTransactionSize,            CJetParam::typeInteger,     1,  0,  0,  0,  1,  100,    100 ),  // min = 0, max = 100, default = 100

    NORMAL_PARAM(   JET_paramPrereadIOMax,                  CJetParam::typeInteger,     1,  0,  0,  0,  0,  256,    8 ),    // min = 0, max = 256, default = 8

    NORMAL_PARAM(   JET_paramMinDataForXpress,              CJetParam::typeInteger,     1,  1,  1,  1,  0, 2147483647,  1024 ),

    NORMAL_PARAM(   JET_paramEnablePeriodicShrinkDatabase,                  CJetParam::typeBoolean,     0,  0,  0,  1,  0,       1,         0 ),    // default: false
*/


//  note: This was formerly part of smallConfig
const ConfigSetOverrideValue    g_rgJetConfigLowMemory[] = {
    //CO( JET_paramEnableAdvanced,              fFalse,     0x0 ),              // pre-win8 we used to disable this on smallConfig, but it turned out to be a terrible idea


    //      Global Component Control
    //
    CO( JET_paramMaxInstances,                  1,          0x0 ),              //  note: some of the restore APIs need an excess instance, so may want to up it to 2.

    //      Log Size, Log "Cache" Size, and Log thread control
    //
    CO( JET_paramLogBuffers,                    40,         0x0 ),              //  slightly larger ~32+2*page/size

    //      Database Cache Settings
    //
    CO( JET_paramPageHintCacheSize,             32,         0x0 ),              //
    CO( JET_paramEnableFileCache,               fTrue,      0x0 ),              //  use FileCache/ViewCache to minimize our visible cache, and defend against the super small cache size
    CO( JET_paramEnableViewCache,               fTrue,      0x0 ),
    CO( JET_paramCacheSizeMin,                  64*1024,    coBytesToPages ),   //  dynamic: min is 64k
    CO( JET_paramCacheSizeMax,                  256*1024,   coBytesToPages ),   //           max is 256k
    CO( JET_paramStartFlushThreshold,           1,          0x0 ),
    CO( JET_paramStopFlushThreshold,            2,          0x0 ),
    CO( JET_paramLRUKHistoryMax,                10,         0x0 ),

    //      Misc Memory Reduction
    //
    CO( JET_paramGlobalMinVerPages,             1,          0x0 ),
    CO( JET_paramMaxVerPages,                   64,         0x0 ),
    CO( JET_paramVerPageSize,                   8192,       0x0 ),              //  this cuts verstore across board by 2x on x86 and 4x on x64
    CO( JET_paramHungIOActions,                 JET_bitNil, 0x0 ),              //  Prevent the IO watchdog thread from starting up (49k).
    //  ALSO
    //      Forces cresmgr to heap over internal faster more concurrent, but larger slab allocator

    //      IO Control
    //
    CO( JET_paramOutstandingIOMax,              16,         0x0 ),
    //CO( JET_paramCheckpointIOMax,             1,          0x0 ),              //  not used right now (got disabled with smooth checkpoint IO)

    //  Formerly in small config, moved to RunSilently (which does more!) ...
    //CO( JET_paramNoInformationEvent,      fTrue, 0x0 ),
    //CO( JET_paramDisablePerfmon,              fTrue,      0x0 ),              //  normally I would not put this here, but it controls allocation of a large chunk of memory

    //  Formerly in small config, everything in RemoveQuotas 
};

//  note: This was formerly part of smallConfig
const ConfigSetOverrideValue    g_rgJetConfigLowDiskFootprint[] = {
    //      DB (and temp DB) Size Management
    //
    CO( JET_paramDbExtensionSize,               16,         0x0 ),              //  should we make this "64*1024, coBytesToPages"?
    CO( JET_paramPageTempDBMin,                 14,         0x0 ),              //  min size of a temp DB

    //      Log Config (Size, Checkpoint Depth, etc)
    //
    CO( JET_paramCircularLog,                   fTrue,      0x0 ),              //  most clients want this
    CO( JET_paramLogFileSize,                   64,         0x0 ),
    CO( JET_paramCheckpointDepthMax,            512*1024,   0x0 ),              //  512k checkpoint
};

const ConfigSetOverrideValue    g_rgJetConfigDynamicMediumMemory[] = {

    //      Global Component Control
    //
    CO( JET_paramMaxInstances,                  1,          0x0 ),              //  Controls a fair amount of memory

    //      Log Format, Log "Cache" Size, and Thread Control
    //
    CO( JET_paramLogBuffers,                    512,        0x0 ),              //  half the log file size for now, when we implement dynamic log buffers we'll let this vary

    //      Database Cache Settings
    //
    CO( JET_paramPageHintCacheSize,             64,         0x0 ),              //  64B to minimize DHT/g_bfhash bypass lookup cache (guestimate)
    CO( JET_paramEnableFileCache,               fTrue,      0x0 ),              //  use FileCache/ViewCache to improve our performance with a 2nd chance NTFS cache
    CO( JET_paramEnableViewCache,               fTrue,      0x0 ),
    CO( JET_paramCacheSizeMin,                  128*1024,   coBytesToPages ),   //  128k (4 pages @ 32 KB page, 32 pages @ 4 KB page)
    CO( JET_paramCacheSizeMax,                  5*1024*1024,coBytesToPages ),   //  5M
    CO( JET_paramStartFlushThreshold,           2,          coPctCacheMaxToPages ), //  a bit small (guestimate)
    CO( JET_paramStopFlushThreshold,            3,          coPctCacheMaxToPages ), //   (guestimate)
    CO( JET_paramLRUKHistoryMax,                1000,       0x0 ),              //   (guestimate, might be low, might be high, 1/10th of default)

    //      Misc Memory Reduction
    //
    CO( JET_paramGlobalMinVerPages,             1,          0x0 ),
    CO( JET_paramVerPageSize,                   8192,       0x0 ),              // this cuts verstore across board by 2x on x86 and 4x on x64
    //  ALSO
    //      Forces cresmgr to heap over internal faster more concurrent, but larger slab allocator

    //      IO Control
    //
    CO( JET_paramOutstandingIOMax,              48,         0x0 ),              //   (guestimate)
    //CO( JET_paramCheckpointIOMax,             1,          0x0 ),              // not used right now
};

const ConfigSetOverrideValue    g_rgJetConfigMediumDiskFootprint[] = {
    //      DB (and temp DB) Size Management
    //
    CO( JET_paramDbExtensionSize,               512*1024,   coBytesToPages ),   //  .5 MB (reasonably big and at least 16 page standard extent @ 32 KB page size)
    CO( JET_paramPageTempDBMin,                 30,         0x0 ),

    //      Log Config (Size, Checkpoint Depth, etc)
    //
    CO( JET_paramCircularLog,                   fTrue,      0x0 ),              //  most clients want this
    CO( JET_paramLogFileSize,                   512,        0x0 ),              //  512K
    CO( JET_paramCheckpointDepthMax,            4*1024*1024,0x0 ),              //  4 MB (8 OB0 buckets, 5 MB = aggressive checkpoint maint)
};

const ConfigSetOverrideValue    g_rgJetConfigSSDProfileIO[] = {
    CO( JET_paramMaxCoalesceReadSize,           256*1024,   0x0 ),              //  according to Appolo team, 256K looks like the breakeven point for both read and write
    CO( JET_paramMaxCoalesceWriteSize,          256*1024,   0x0 ),              //
    //CO( JET_paramMaxCoalesceReadGapSize,      256*1024,   0x0 ),              //  we actually want read gapping (according to NT perf team) ... though in theory we should NOT need it with SSDs.  Leaving it at default 256 KB, so we don't get carried away.  Have not verified ourselves.
    CO( JET_paramMaxCoalesceWriteGapSize,       0,          0x0 ),              //  avoid overwriting pieces of the disk that do not need to be updated
    CO( JET_paramOutstandingIOMax,              16,         0x0 ),              //  do not need to be as aggressive for read IO max
    CO( JET_paramPrereadIOMax,                  5,          0x0 ),              //  should not need as much prereading for SSDs
};

const ConfigSetOverrideValue    g_rgJetConfigRunSilent[] = {
    CO( JET_paramNoInformationEvent,            1,          0x0 ),              //  turn off all information events
    CO( JET_paramEventLoggingLevel,             JET_EventLoggingDisable, 0x0 ), //  turn off ALL other events as well
    CO( JET_paramDisablePerfmon,                fTrue,      0x0 ),              //  turn off perfmon
    //  ALSO
    //      disables OSTrace() tracing
    //      disables ETW tracing - which looks especially expensive now.
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
    Assert( (ULONG_PTR)configUser == ulpConfigUser );   // check no data loss
    Assert( configInternal == configUser ||
                configUser == 0 /* legacy small config */ );
    *pulParam = configUser;
    return JET_errSuccess;
}

VOID SetJetConfigSet( INST * const pinst, _In_reads_(cConfigOverrides) const ConfigSetOverrideValue * const prgConfigOverrides, ULONG cConfigOverrides )
{
    const ULONG_PTR cbPage = g_cbPage;

    for ( size_t iiparam = 0; iiparam < cConfigOverrides; iiparam++ )
    {
        //  get the paramid of the JET_param we want to change
        size_t iparam = prgConfigOverrides[iiparam].paramid;

        //  get the storage (global or instance) of which we want to change 
        CJetParam* pjetparamT = NULL;
        if ( pinst == pinstNil || g_rgparam[ iparam ].FGlobal() )
        {
            pjetparamT = g_rgparam + iparam;
        }
        else
        {
            pjetparamT = pinst->m_rgparam + iparam;
        }

        Assert( JET_paramConfiguration != prgConfigOverrides[iiparam].paramid );    // is handled outside of here in SetConfiguration()

        ULONG_PTR ulpFinal = prgConfigOverrides[iiparam].ulpValue;
        switch( prgConfigOverrides[iiparam].grbit & coUnitConversionsMsk )
        {
            case 0:
                // no conversion
                break;
            case coBytesToPages:
                ulpFinal = ulpFinal / cbPage;
                break;
            case coPctRamToPages:
                Assert( (QWORD)OSMemoryQuotaTotal() * (QWORD)ulpFinal > OSMemoryQuotaTotal() );     // overflow check
                Assert( (QWORD)OSMemoryQuotaTotal() * (QWORD)ulpFinal > ulpFinal );                 // overflow check
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
            //  we know these settings to not work together, double log file size (to 128 KB) ...
            ulpFinal *= 2;
        }


        //  if pinst == NULL, then we're resetting parameters globally (not on a per-instance
        //  basis), so we can reset this parameter now.
        //  or if the pinst != NULL then it has to be !global, so that we can reset this parameter.
        if ( pinst == pinstNil || !pjetparamT->FGlobal() )
        {
            //  set actual value
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

    //  basic arg validation
    //

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

    // convert legacy config argument to modern config argument
    //
    const CJetParam::Config     configLegacySmall   = 0;
    const CJetParam::Config     configLegacyLegacy  = 1;

    CJetParam::Config   configSet;      //  the settings we will enact / override
    CJetParam::Config   configFinal;    //  the config we will store in the INST or global
    CJetParam::Config   configLegacy;   //  the config we will store in the param table value

    C_ASSERT( 1 /* the old/legacy JetParam::configLegacy value */ == configLegacyLegacy );
    C_ASSERT( 1 /* the old/legacy JetParam::configLegacy value */ == JET_configDefault );

    if ( ulParam == configLegacySmall /* the legacy small config value */ )
    {
        //  we store the legacy config parameters exactly as the consumer set it in the config parameter

        configSet = ( JET_configDefault | JET_configLowDiskFootprint | JET_configLowMemory | JET_configRemoveQuotas );
        configFinal = configSet;
        configLegacy = configLegacySmall /* 0 */;
    }
    else if ( ulParam == configLegacyLegacy /* the legacy legacy value, AND no additional configs */ )
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
            //  the default value, resets the ALL system parameters

            configFinal = ( JET_configDefault );
        }
        else
        {
            configFinal = ( pinst == pinstNil ) ? g_config : pinst->m_config;
        }

        //  because the configuration is cumulative w/ regards to the other bits, we | all bits in together ...
        
        configFinal |= ( configSet & ~JET_configDefault );
        configLegacy = configFinal;
    }

    //  any new set bits should be in the final bits

    Assert( ( configSet & ~( ( pinst == pinstNil ) ? g_config : pinst->m_config ) ) ==
                ( configFinal & ~( ( pinst == pinstNil ) ? g_config : pinst->m_config ) ) );

    //  except for legacy small config, the configLegacy / param setting should match the configFinal / INST or global settings

    Assert( configLegacy == configLegacySmall ||
                configLegacy == configFinal );

    // the config must be properly set or checks for a specific config
    // (e.g.: FJetConfigSmall()) will always fail.
    //
    CheckParamTrap( JET_paramConfiguration, " (value to be set configLegacy | configFinal depending upon how you look at it)." );
    if ( pinst == pinstNil )
    {
        //g_rgparam[JET_paramConfiguration].Set( pinst, ppibNil, ULONG_PTR( configLegacy ), NULL );
        g_rgparam[JET_paramConfiguration].m_valueCurrent    = configLegacy;
        g_rgparam[JET_paramConfiguration].m_fWritten        = fTrue;
        g_config = configFinal;
    }
    else
    {
        //pinst->m_rgparam[JET_paramConfiguration].Set( pinst, ppibNil, ULONG_PTR( configLegacy ), NULL );
        pinst->m_rgparam[JET_paramConfiguration].m_valueCurrent = configLegacy;
        pinst->m_rgparam[JET_paramConfiguration].m_fWritten     = fTrue;
        pinst->m_config = configFinal;
    }

    //  install the base default settings
    //

    if ( configSet & JET_configDefault || configLegacy == configLegacySmall )
    {
        //  reset all parameters to their defaults for the chosen config
        //
        for ( size_t iparam = 0; iparam < g_cparam; iparam++ )
        {
            CJetParam* pjetparamT = NULL;

            //  lookup the storage location for this paramid.  if no instance was
            //  specified or if the parameter is intrinsically global then we will
            //  always go to the global array
            //
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
                continue;   // handled above
            }

            //  reset this parameter, ignoring any errors
            //
            //  NOTE:  we only reset an intrinsically global parameter if we are
            //  resetting parameters globally
            //
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

    //  install the config set overrides
    //

    //  note: Since some of these have over-lapping configurations, think carefully which is first/last

    if ( configSet & JET_configLowMemory && configLegacy != configLegacySmall )
    {
        //  this one operates a little differently than the below, because I wasn't ready to break it out into a table

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
    //  after memory configs b/c relies on paramCacheSizeMax and most likely battery is the top-most concern
    if ( configSet & JET_configLowPower )
    {
        configSet &= ~JET_configLowPower;
    }
    //  since these are specifically pools that don't affect memory size, it's fine to have near end
    if ( configSet & JET_configRemoveQuotas )
    {
        SetJetConfigSet( pinst, g_rgJetConfigRemoveQuotas, _countof(g_rgJetConfigRemoveQuotas) );
        configSet &= ~JET_configRemoveQuotas;
    }
    //  after all as it squashes all signaling
    if ( configSet & JET_configRunSilent )
    {
        SetJetConfigSet( pinst, g_rgJetConfigRunSilent, _countof(g_rgJetConfigRunSilent) );
        configSet &= ~JET_configRunSilent;
    }
    if ( configSet & JET_configHighConcurrencyScaling )
    {
        configSet &= ~JET_configHighConcurrencyScaling;
    }

    //  fail if there was a bit we didn't understand ...

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
        Assert( pjetparamDst->m_valueCurrent == ulParam );  // should be true from memcpy() above
    }
#endif

    //  pull the internal config variable from the other internal config variables
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

    // This is the allowed maximum record size (including overhead) as reported by JetGetRecordSize().
    // Since JetGetRecordSize() counts overhead (including tag array overhead),
    // we use CbPageDataMaxNoInsert() to keep compatibility.
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
    /* else */                              CJetParam::IllegalGet   ) ) ) ) ) ) ) ) )
#define PFNSET_OF_TYPE( type )                                      ( \
    type == CJetParam::typeBoolean ?        CJetParam::SetBoolean   : ( \
    type == CJetParam::typeGrbit ?          CJetParam::SetGrbit     : ( \
    type == CJetParam::typeInteger ?        CJetParam::SetInteger   : ( \
    type == CJetParam::typeString ?         CJetParam::SetString    : ( \
    type == CJetParam::typePointer ?        CJetParam::SetPointer   : ( \
    type == CJetParam::typeFolder ?         CJetParam::SetFolder    : ( \
    type == CJetParam::typePath ?           CJetParam::SetPath      : ( \
    type == CJetParam::typeBlockSize ?      CJetParam::SetBlockSize : ( \
    /* else */                              CJetParam::IllegalSet   ) ) ) ) ) ) ) ) )
#define PFNCLONE_OF_TYPE( type )                                    ( \
    type == CJetParam::typeBoolean ?        CJetParam::CloneDefault : ( \
    type == CJetParam::typeGrbit ?          CJetParam::CloneDefault : ( \
    type == CJetParam::typeInteger ?        CJetParam::CloneDefault : ( \
    type == CJetParam::typeString ?         CJetParam::CloneString  : ( \
    type == CJetParam::typePointer ?        CJetParam::CloneDefault : ( \
    type == CJetParam::typeFolder ?         CJetParam::CloneString  : ( \
    type == CJetParam::typePath ?           CJetParam::CloneString  : ( \
    type == CJetParam::typeBlockSize ?      CJetParam::CloneDefault : ( \
    /* else */                              CJetParam::IllegalClone ) ) ) ) ) ) ) ) )

#define ILLEGAL_PARAM(  paramid )                       \
    { #paramid, paramid, CJetParam::typeNull, fFalse, fFalse, fTrue, fTrue, fTrue, fTrue, fFalse, fFalse, fFalse, fFalse, 0, ULONG_PTR( ~0 ), CJetParam::IllegalGet, CJetParam::IllegalSet, CJetParam::CloneDefault }

// CUSTOM_PARAM() allows customized get/set/clone functions.
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

// CUSTOM_PARAM2() allows explicit rangeLow and rangeHigh.
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

// CUSTOM_PARAM3() sets m_fCustomStorage to fFalse.
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
//  there is no difference between an IGNORED and a NORMAL parameter other than
//  the fact that they are or are not actually referenced somewhere in the code
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



// UNICODE_UNDONE_DEFERRED: Technically don't need to do this, but I noticed that if you set a default string value to ASCII string, there is no compile error, that means it is ripe for a bug.

#ifdef _PREFAST_
const
#endif

// Conditional default values for parameters are defined below and then referenced in sysparam.xml (and thus in the generated sysparamtable.g.cxx).
//

#ifdef ESENT
#define JET_paramLegacyFileNames_DEFAULT            (JET_bitESE98FileNames | JET_bitEightDotThreeSoftCompat)
#define JET_paramEnablePersistedCallbacks_DEFAULT   0

#else
#define JET_paramLegacyFileNames_DEFAULT            JET_bitESE98FileNames

//  UNDONE:  change this default to FALSE and change Store.EXE to set it to TRUE
#define JET_paramEnablePersistedCallbacks_DEFAULT   1
#endif

#ifdef PERFMON_SUPPORT
#define JET_paramDisablePerfmon_DEFAULT     fFalse
#else
#define JET_paramDisablePerfmon_DEFAULT     fTrue
#endif

#if defined( DEBUG ) && !defined( ESENT )
// Default to 1 (Enabled) on Exchange-Debug only.
#define JET_paramEnableShrinkDatabase_DEFAULT       ( JET_bitShrinkDatabaseOn | JET_bitShrinkDatabaseRealtime | JET_bitShrinkDatabasePeriodically )
#else
// Default to 0 (Disabled) on Retail Exchange, and all ESENT.
#define JET_paramEnableShrinkDatabase_DEFAULT       JET_bitShrinkDatabaseOff
#endif

#if defined(ESENT) && !defined(DEBUG)
// Enabled in event-only mode for Windows RETAIL.
#define JET_paramPersistedLostFlushDetection_DEFAULT        ( JET_bitPersistedLostFlushDetectionEnabled | JET_bitPersistedLostFlushDetectionFailOnRuntimeOnly )
#else
// Fully enabled for DEBUG or Exchange.
#define JET_paramPersistedLostFlushDetection_DEFAULT        JET_bitPersistedLostFlushDetectionEnabled
#endif

// Elastic waypoint latency enabled by default on debug, retail enabled selectively via flighting
#define JET_paramElasticWaypointLatency_DEFAULT     OnDebugOrRetail( 1, 0 )

// Allow flighting of some features to windows independent of exchange flighting
#ifdef ESENT
#define JET_paramStageFlighting_DEFAULT 0x800000
#else
#define JET_paramStageFlighting_DEFAULT 0
#endif

// Note: This can be used to turn on smooth IO / meted Q globally for reads, but we generally speaking moved this control 
// to client (aka Store) so that it can be controlled a per task basis.  Except for controlling background ESE engine tasks.
#ifdef ESENT
#define grbitEseSkuQueueOptionDefault 0x0
#else
#define grbitEseSkuQueueOptionDefault ( bitUseMetedQEseTasks )
#endif

#define JET_paramFlight_RBSCleanupEnabledDEFAULT                OnDebugOrRetail( fTrue, fFalse )

//  ================================================================
// The following file is auto-generated from sysparam.xml.
// To modify or add parameters, edit sysparam.xml and run gengen.bat.
//  ================================================================
#include "sysparamtable.g.cxx"

// Ensure that the parameter array is the correct size.
C_ASSERT( _countof( g_rgparamRaw ) == JET_paramMaxValueInvalid + 1 );

const size_t    g_cparam    = _countof( g_rgparamRaw );

// CJetParam inherits from JetParam, adding some helper functions. They
// should be the same size.
C_ASSERT( sizeof( JetParam ) == sizeof( CJetParam ) );

// g_grparamRaw is an array of JetParam, and g_rgparam is an array of the child class CJetParam.
CJetParam* const        g_rgparam   = (CJetParam*) &g_rgparamRaw[ 0 ];

LOCAL CCriticalSection  g_critSysParamFixup( CLockBasicInfo( CSyncBasicInfo( "g_critSysParamFixup" ), rankSysParamFixup, 0 ) );
 
// Some System Parameters need a dynamic default that can't be established at compile time.
// The prime example of these are the default paths when running in an app container (packaged process?)
VOID FixDefaultSystemParameters()
{
    static BOOL fDefaultSystemParametersFixedUp = fFalse;

    ERR err = JET_errSuccess;
    IFileSystemAPI* pfsapi = NULL;

    if ( fDefaultSystemParametersFixedUp )
    {
        //  Fast path, someone has performed this already.
        return;
    }

    g_critSysParamFixup.Enter();

    //  Configure Default Config Variables
    //

    if ( !fDefaultSystemParametersFixedUp )
    {
        //  We won the race to set the initial default system parameters, so time to do our victory 
        //  lap and rub it other's faces ...

        // If the sku wants something other than normal configuration, use it
        // Must be first, as it has the potential to reset all other params (like ?maybe? the paths below)
        DWORD cConfiguration = JET_configDefault;

        if ( ErrUtilSystemSlConfiguration( g_wszLP_PARAMCONFIGURATION, &cConfiguration ) >= JET_errSuccess )
        {
            SetConfiguration( &(g_rgparam[JET_paramConfiguration]), pinstNil, ppibNil, cConfiguration, NULL );
        }

    /*      Testing with this random config settings ...
        srand( TickOSTimeCurrent() );
        CJetParam::Config config = rand() % ( JET_configMask + 1 );
        wprintf( L"Setting config to 0x%x\n", config );
        SetConfiguration( &(g_rgparam[JET_paramConfiguration]), pinstNil, ppibNil, config, NULL );
    //*/

        WCHAR rgwchDefaultPath[ IFileSystemAPI::cchPathMax ];
        BOOL fIsDefaultDirectory;

        //  Configure Default Path Variables
        //

        Call( ErrOSFSCreate( g_pfsconfigGlobal, &pfsapi ) );
        Call( pfsapi->ErrPathFolderDefault( rgwchDefaultPath, _countof( rgwchDefaultPath ), &fIsDefaultDirectory ) );

        if ( !fIsDefaultDirectory )
        {
            // Fix the default paths.
            const ULONG rgjpPathFixup[] =
            {
                JET_paramSystemPath,
                JET_paramLogFilePath,
            };

            for ( size_t i = 0; i < _countof( rgjpPathFixup ); ++i )
            {
                Call( CJetParam::SetPath( &g_rgparam[ rgjpPathFixup[ i ] ], NULL, NULL, 0, rgwchDefaultPath ) );
            }

            // Do not fix the temporary database; it will be resolved in ErrInit().
        }

        //  yikes!  Means we're not concurrent safe here ...
        Assert( fDefaultSystemParametersFixedUp == fFalse );
    }

HandleError:

    //  I debated leaving this set as fFalse if we were failing, but I decided that wasn't
    //  responsible as it could allow someone to later in the middle of ESE being configured 
    //  try to call SetConfiguration() resetting everything dynamically and wreaking havoc.
    //
    //  This has to remain best effort setup, _because_ we're not returning an error here and
    //  blocking clients from continuing.  If we change that, then we can allow retry.
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
        //  Even though it's a boolean, I don't want to let it through ... yet.
        return fFalse;
    }
    if ( pjetparam->m_paramid == JET_paramConfigStoreSpec )
    {
        //  8 out of 10 programmers consider endless recursion bad, the other two write in LISP.
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
        //  else this is not a param the config store can handle yet
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
    _In_ PCWSTR     wszParam,
    const BOOL      fEnterCritInst  = fTrue )
{
    ERR                 err         = JET_errSuccess;
    PIB*                ppib        = ( JET_sesidNil != sesid ? (PIB*)sesid : ppibNil );
    CJetParam*          pjetparam   = NULL;
    BOOL                fEnableAdvanced = fFalse;
    BOOL                fOriginalSet = fFalse;
    ULONG_PTR           ulpOriginal = ulParam;
    BOOL                fLoadedOverride = fFalse;

    //  set parameter values in g_critInst to protect the cloning process

    if ( fEnterCritInst )
    {
        INST::EnterCritInst();
    }

    //  if this parameter is not defined then it must be invalid

    if ( paramid >= g_cparam )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  lookup the storage location for this paramid.  if no instance was
    //  specified or if the parameter is intrinsically global then we will
    //  always go to the global array
    //
    if ( pinst == pinstNil || g_rgparam[ paramid ].FGlobal() )
    {
        pjetparam = g_rgparam + paramid;
        fEnableAdvanced = BOOL( g_rgparam[ JET_paramEnableAdvanced ].Value( ) );
        if ( g_rgparam[JET_paramConfigStoreSpec].Value() )
        {
            //  We aren't going to kill this if we fail to get access to overload config store
            if ( ErrSysParamLoadOverride( (PCWSTR)g_rgparam[JET_paramConfigStoreSpec].Value(), pjetparam, &ulParam ) == JET_errSuccess )
            {
                fLoadedOverride = fTrue;
                fOriginalSet = g_rgparam[paramid].FWritten();
                ulpOriginal = g_rgparam[paramid].Value();
            }
            //  We will be more forgiving about the errors from the overrides, ignoring ...
        }
    }
    else
    {
        pjetparam = pinst->m_rgparam + paramid;
        fEnableAdvanced = BoolParam( pinst, JET_paramEnableAdvanced );
        if ( pinst->m_rgparam[JET_paramConfigStoreSpec].Value() )
        {
            //  We aren't going to kill this if we fail to get access to overload config store
            if ( ErrSysParamLoadOverride( (PCWSTR)pinst->m_rgparam[JET_paramConfigStoreSpec].Value(), pjetparam, &ulParam ) == JET_errSuccess )
            {
                fLoadedOverride = fTrue;
                fOriginalSet = pinst->m_rgparam[paramid].FWritten();
                ulpOriginal = pinst->m_rgparam[paramid].Value();
            }
            //  We will be more forgiving about the errors from the overrides, ignoring ...
        }
    }

    //  if this parameter is flagged as advanced and the advanced counters are
    //  not enabled then IGNORE the requested parameter setting
    //
    if ( pjetparam->FAdvanced() && !fEnableAdvanced )
    {
        Error( JET_errSuccess );
    }

    //  set the value for the parameter

    Call( pjetparam->Set( pinst, ppib, ulParam, wszParam ) );

    if ( fLoadedOverride && !pjetparam->m_fRegOverride )
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
            //  not the most elegant, but don't want to use a localization breaking "N/A" or do a whole 2nd event.
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

    //  if this parameter is not defined then it must be invalid

    if ( paramid >= g_cparam )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  lookup the storage location for this paramid.  if no instance was
    //  specified or if the parameter is intrinsically global then we will
    //  always go to the global array
    //
    if ( pinst == pinstNil || g_rgparam[ paramid ].FGlobal() )
    {
        pjetparam = g_rgparam + paramid;
    }
    else
    {
        pjetparam = pinst->m_rgparam + paramid;
    }

    //  get the value for the parameter

    Call( pjetparam->Get( pinst, ppib, pulParam, wszParam, cbMax ) );

HandleError:
    return err;
}


//
//  Global Config Store
//

ERR ErrSysParamLoadDefaults( const BOOL fHasCritInst, INST * pinst, CConfigStore * const pcs, _In_reads_(cparam) CJetParam * const prgparam, const ULONG cparam )
{
    ERR err = JET_errSuccess;

    Expected( cparam == _countof(g_rgparamRaw) );

    const BOOL fLoadingGlobal = ( prgparam == g_rgparamRaw );

    Assert( !fHasCritInst || INST::FOwnerCritInst() );

    for ( size_t iparamid = 0; iparamid < (size_t)cparam; iparamid++ )
    {
        Assert( prgparam[iparamid].m_paramid == iparamid ); //  sanity check

        if ( !fLoadingGlobal &&                     // i.e. we ARE inst specific params
                prgparam[iparamid].FGlobal() &&     // and this is a global param
                //  these two are special in that they're marked global, but used both in a global and inst contexts
                iparamid != JET_paramConfiguration &&
                iparamid != JET_paramConfigStoreSpec )
        {
            // No point in loading a global into the inst param array, no one will read it.
            continue;
        }

        if ( FConfigStoreLoadableParam( &(prgparam[iparamid]) ) )
        {
            ULONG_PTR ulpValue;
            BOOL fLoadedDefault = fFalse;

            //  First we attempt to load the override value.
            //
            //  We have the ErrSysParamLoadOverride() call in ErrSetSystemParameter() first, but
            //  those funcs only get called if either we have a param explicitly listed in the 
            //  registry in SysParamDefaults section (so the 2nd ErrConfigReadValue() will fetch 
            //  a value and call into ErrSetSystemParameter()) or if the client explicitly calls 
            //  JetSetSystemParameter() on that param.  We could have the case that it is neither 
            //  going to be set in reg default section explicitly or the client will try to set 
            //  it, so we also need to load the override here for all params.
            err = ErrConfigReadValue( pcs, csspSysParamOverride, prgparam[ iparamid ].m_szParamName, &ulpValue );
            //  Note we ignore errors from trying read from the overrides.
            if ( err != JET_errSuccess )
            {
                //  If there is no override, we pull in the default value (if there is one).
                Call( ErrConfigReadValue( pcs, csspSysParamDefault, prgparam[ iparamid ].m_szParamName, &ulpValue ) );
                // Should we log event if ErrConfigReadValue() fails for a default load?
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
                    Error( err );   //  fail
                }
                else
                {
                    prgparam[iparamid].m_fRegDefault = fLoadedDefault;
                }
            }
        }
        else if ( iparamid != JET_paramConfigStoreSpec )
        {
            // These are the types of params that end up here:
            //  Params that are deprecated and have illegal get/set types, come back as typeNull, see ILLEGAL_PARAM()
            //case JetParam::typeNull:
            //  We can't handle these types of parameters yet:
            //case JetParam::typeString:
            //case JetParam::typeFolder:
            //case JetParam::typePath:
            //case JetParam::typePointer:
            //case JetParam::typeUserDefined:
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
            // Do nothing for these types currently ... though it would be nice to handle String, Folder, 
            // and Path ... the others Pointer and UserDefined don't make much sense.
        }

    }   // for each iparamid

    // err is probably JET_wrnColumnNull, so reset to success.
    Assert( err >= JET_errSuccess );    // errors should've Call()'d to HandleError
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
        //  There is a bit of a split brain on what is a NULL string is for this custom parameter table:
        //   1. if you just load the DLL and set a few variables, but do not set paramConfigStoreSpec 
        //      you will get a NULL for this value
        //   2. if you just set JET_paramConfiguration, then this gets set via Reset()/Clone() to NULL,
        //      but the below SetString() turns a NULL into a L"".
        //  So to avoid this, we're going to just set it to real NULL here, so we have a consistent 
        //  experience elsewhere.
        if ( pjetparam->m_fFreeValue )
        {
            delete [] (void*)pjetparam->m_valueCurrent;
        }
        pjetparam->m_valueCurrent   = NULL;
        pjetparam->m_fWritten       = fTrue;
        pjetparam->m_fFreeValue     = fFalse;
        //  Note SetConfiguration() --> pjetparamT->Reset() does this, and expects success.
        return JET_errSuccess;
    }

    //  First we call this to save a copy to storage.
    CallR( CJetParam::SetString( pjetparam, pinst, ppib, ulParam, wszParam ) );

    //  Only try to read from the config store if they're actually setting a value ...
    //      (keep in mind that when JET_paramConfiguration is set, we'll get a NULL from reset here)

    if ( wszParam && wszParam[0] != L'\0' )
    {

        Call( ErrOSConfigStoreInit( wszParam, &pcs ) );
        Assert( pcs );

        // a little silly, but makes me feel I understand my world  ...
        Assert( FBounded( (QWORD)pjetparam, (QWORD)( pjetparam - pjetparam->m_paramid ), sizeof(JetParam) * _countof(g_rgparamRaw) ) );

        if ( pinst == NULL || pinst == (INST*)JET_instanceNil /* just in case */ )
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


// Provide a JET_PFNINITCALLBACK function which wraps a JET_PFNSTATUS.
// This provides backwards compatability for JetInit3, which takes the
// old-style callback. When wrapping a JET_PFNSTATUS the context pointer
// for the JET_PFNINITCALLBACK should be a pointer to this object.
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
        // NOTE: m_pfnStatus may be null. If we are wrapping a null callback then
        // nothing should happen.
    }

    static JET_ERR PfnWrapper( JET_SNT snt, JET_SNP snp, void * pv, void * pvContext )
    {
        const InitCallbackWrapper * const pInitCallbackWrapper = (InitCallbackWrapper*)pvContext;
        Assert( NULL != pInitCallbackWrapper );
        return pInitCallbackWrapper->ErrCallback( snt, snp, pv );
    }
};

//
//  In what can only be described as a new-wave fusion of Romanian SOMEONE-escu-esc CAuto 
//  class model sprinkled with SOMEONE-like char/wchar agnostic _T-esk templating gloss, to 
//  create a not quite break the debugger SOMEONEian templated auto class for converting 
//  V1 index create structures to type V2 index create structures.
//
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
            //  There is a count but no pointer array, this is invalid.
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        //  If we get only a pindexcreate, this is allowed store apparently will pass an 
        //  array of indices, but 0 for count.
    }
    else
    {
        //  We have some indices to convert.
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

            //  Using the fact that the first n - 1 items are identical.
            Assert( sizeof(*m_rgindexcreateEngine) > sizeof(*pindexcreate) );
            Enforce( sizeof(*m_rgindexcreateEngine) > pidxcreateOld->cbStruct );
            memcpy( &(m_rgindexcreateEngine[iIdx]), pidxcreateOld, pidxcreateOld->cbStruct );

            //  Use space hints to propagate the density.
            memset( &(m_rgspacehintsEngine[iIdx]), 0, sizeof(m_rgspacehintsEngine[iIdx]) );
            m_rgspacehintsEngine[iIdx].cbStruct = sizeof(m_rgspacehintsEngine[iIdx]);
            m_rgspacehintsEngine[iIdx].ulInitialDensity = pidxcreateOld->ulDensity;
            m_rgindexcreateEngine[iIdx].pSpacehints = &(m_rgspacehintsEngine[iIdx]);

            //  Finally update the internal "version" of the struct...
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

//=====================================================
//
//  In what can only be described as a new-wave fusion of Romanian SOMEONE-escu-esc CAuto 
//  class model sprinkled with SOMEONE-like char/wchar agnostic _T-esk templating gloss, to 
//  create a not quite break the debugger SOMEONEian templated auto class for converting 
//  V2 index create structures to type V3 index create structures.
//
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
            //  There is a count but no pointer array, this is invalid.
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        //  If we get only a pindexcreate, this is allowed store apparently will pass an 
        //  array of indices, but 0 for count.
    }
    else
    {
        //  We have some indices to convert.
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

            //  Using the fact that most items are identical.
            C_ASSERT( sizeof(*m_rgindexcreateEngine) == sizeof(*pindexcreate) );
            Assert( pidxcreate2->cbStruct <= sizeof(*m_rgindexcreateEngine) );
            memcpy( &(m_rgindexcreateEngine[iIdx]), pidxcreate2, pidxcreate2->cbStruct );

            m_rgindexcreateEngine[iIdx].cbStruct = sizeof(*m_rgindexcreateEngine);

            LCID lcidT;
            ULONG dwLCMapFlagsT;

            // Populate the unicode index structure regardless of JET_bitIndexUnicode.
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

            // Fill out szLocaleName, but only if the LCID is specified. If it's zero, then
            // ErrFILEIValidateCreateIndex will set the default (pinst->m_wszLocaleNameDefault).
            if ( lcidNone != lcidT )
            {
                CallR( ErrNORMLcidToLocale( lcidT,
                                            m_rgunicodeindexEngine[iIdx].szLocaleName,
                                            NORM_LOCALE_NAME_MAX_LENGTH ) );
            }

            m_rgunicodeindexEngine[iIdx].dwMapFlags = dwLCMapFlagsT;

            // If the original structure didn't have JET_bitIndexUnicode, then there's
            // no way that the LC Map Flags should be set.
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

            // When converting from the v2 structure to the v3, we may need to create the
            // JET_INDEXUNICODE structure. This is because the union with the lcid is no
            // longer a union in the v3 structure. If the lcid is specified then the
            // JET_INDEXUNICODE2 structure must be used. It also means we need to inject
            // JET_bitIndexUnicode to know to look at the structure.

            // When checking for null-ness, only look at the bottom 32 bits.
            // When a structure is created on the stack, then the upper 32-bits may be
            // garbage, and we did a raw memcpy() up above.
            AssertSz( ( ( m_rgindexcreateEngine[iIdx].grbit & JET_bitIndexUnicode ) == 0 )
                    == ( ( DWORD_PTR( m_rgindexcreateEngine[iIdx].pidxunicode ) & 0xffffffff ) == NULL ),
                      "When JET_bitIndexUnicode is set, pidxunicode must be non-NULL." );

            //  Finally update the internal "version" of the struct...
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

//==============================================================================

// Similar as v2-to-v3, but useful for making a mutable copy of v3 structures.
// This does not do a deep-clone of the JET_UNICODEINDEX2 array, but instead points
// to the original version.
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
            //  There is a count but no pointer array, this is invalid.
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        //  If we get only a pindexcreate, this is allowed store apparently will pass an 
        //  array of indices, but 0 for count.
    }
    else
    {
        //  We have some indices to convert.
        Assert( pindexcreate );
        Assert( cindexcreate );

        Alloc( m_rgindexcreateEngine = new JET_INDEXCREATE3_T_TO[cindexcreate] );

        // Unlike the v2-to-v3 conversion, we won't clone the JET_UNICODEINDEX2 structure,
        // because JET_UNICODEINDEX2 is not a mutable structure.

        JET_INDEXCREATE3_T_FROM * pidxcreate3 = pindexcreate;
        JET_INDEXCREATE3_T_FROM * pidxcreate3Next;

        for ( ULONG iIdx = 0; iIdx < cindexcreate; iIdx++, pidxcreate3 = pidxcreate3Next )
        {
            pidxcreate3Next = (JET_INDEXCREATE3_T_FROM *)( ((BYTE*)pidxcreate3) + pidxcreate3->cbStruct );

            if ( pidxcreate3->cbStruct != sizeof(*pindexcreate) )
            {
                return ErrERRCheck( JET_errInvalidParameter );
            }

            //  Using the fact that most items are identical.
            C_ASSERT( sizeof(*m_rgindexcreateEngine) == sizeof(*pindexcreate) );
            Assert( pidxcreate3->cbStruct <= sizeof(*m_rgindexcreateEngine) );
            memcpy( &(m_rgindexcreateEngine[iIdx]), pidxcreate3, pidxcreate3->cbStruct );

            m_rgindexcreateEngine[iIdx].cbStruct = sizeof(*m_rgindexcreateEngine);

            // Unlike the v2-to-v3 conversion, we won't clone the JET_UNICODEINDEX2 structure,
            // because JET_UNICODEINDEX2 is not a mutable structure.

            // Spot check a few of the fields.
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


//==============================================================================

//
//  Same thing as Index V1 to V2 auto class above, but this one converts V2 table defn
//  to V3 table definition.  Also note the crazy usage of CAutoIndexT (go see its usage).
//
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

    //  Using the fact that copying the rgidx ptr doesn't matter, and the first n - 3 items are identical.
    Assert( sizeof(m_tablecreateEngine) > sizeof(*ptablecreate) );
    Assert( sizeof(m_tablecreateEngine) > ptablecreate->cbStruct );

    memcpy( &(m_tablecreateEngine), ptablecreate, sizeof(*ptablecreate) );

    //  Upgrade the index definitions.
    Alloc( m_pautoindices = new CAutoIndex_T() );
    Call( m_pautoindices->ErrSet( ptablecreate->rgindexcreate, ptablecreate->cIndexes ) );
    m_tablecreateEngine.rgindexcreate = m_pautoindices->GetPtr();

    Assert( m_tablecreateEngine.cIndexes == ptablecreate->cIndexes );
    Assert( m_tablecreateEngine.cIndexes > 0 || NULL == m_tablecreateEngine.rgindexcreate );
    
    //  Null the last 3 parameters (giving the caller the default of course)
    m_tablecreateEngine.pSeqSpacehints = NULL;
    m_tablecreateEngine.pLVSpacehints = NULL;
    m_tablecreateEngine.cbSeparateLV = 0;

    //  Finally update the internal "version" of the struct...
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

        //  Note: Do not need to copy over .err values of columns, because we cheaply
        //  copied the pointer to the array to the new structure.

        //  Copy index .err values.
        Assert( m_pautoindices );
        if ( m_pautoindices )
        {
            m_pautoindices->Result();
        }

        //  Copy the final out params.
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

//==============================================================================

//
//  Same thing as Index V1 to V2 auto class above, but this one converts V3 table defn
//  to V4 table definition.  Also note the crazy usage of CAutoIndexT (go see its usage).
//
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

    //  Using the fact that copying the rgidx ptr doesn't matter, and the other items are identical.
    C_ASSERT( sizeof(m_tablecreateEngine) == sizeof(*ptablecreate) );
    Assert( sizeof(m_tablecreateEngine) == ptablecreate->cbStruct );

    memcpy( &(m_tablecreateEngine), ptablecreate, sizeof(*ptablecreate) );

    //  Upgrade the index definitions.
    Alloc( m_pautoindices = new CAutoIndex_T() );
    Call( m_pautoindices->ErrSet( ptablecreate->rgindexcreate, ptablecreate->cIndexes ) );
    m_tablecreateEngine.rgindexcreate = m_pautoindices->GetPtr();

    Assert( m_tablecreateEngine.cIndexes == ptablecreate->cIndexes );
    Assert( m_tablecreateEngine.cIndexes > 0 || NULL == m_tablecreateEngine.rgindexcreate );

    //  Finally update the internal "version" of the struct...
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

        //  Note: Do not need to copy over .err values of columns, because we cheaply
        //  copied the pointer to the array to the new structure.

        if ( ( m_ptablecreateFromAPI->grbit & JET_bitTableCreateImmutableStructure ) == 0 )
        {
            //  Copy index .err values.
            Assert( m_pautoindices );
            if ( m_pautoindices )
            {
                m_pautoindices->Result();
            }

        //  Copy the final out params.
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


//==============================================================================

//
//  Same thing as Index V3 to V4 auto class above, but this one converts V4 table defn
//  to V5 table definition.
//
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

    //  Using the fact that the first n - 3 items are identical.
    C_ASSERT( sizeof(m_tablecreateEngine) >= sizeof(*ptablecreate) );
    Assert( sizeof(m_tablecreateEngine) >= ptablecreate->cbStruct );

    memcpy( &(m_tablecreateEngine), ptablecreate, sizeof(*ptablecreate) );

    Assert( m_tablecreateEngine.cIndexes == ptablecreate->cIndexes );

    //  Null cbLVChunkMax (giving the caller the default value)
    m_tablecreateEngine.cbLVChunkMax = 0;

    //  Finally update the internal "version" of the struct...
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

        //  Note: Do not need to copy over .err values of columns, because we cheaply
        //  copied the pointer to the array to the new structure.

        if ( ( m_ptablecreateFromAPI->grbit & JET_bitTableCreateImmutableStructure ) == 0 )
        {
            //  Copy the final out params.
            m_ptablecreateFromAPI->tableid  = m_tablecreateEngine.tableid;
            m_ptablecreateFromAPI->cCreated     = m_tablecreateEngine.cCreated;
        }
    }
}

template< class JET_TABLECREATE4_T, class JET_TABLECREATE5_T >
CAutoTABLECREATE4To5_T< JET_TABLECREATE4_T, JET_TABLECREATE5_T >::~CAutoTABLECREATE4To5_T()
{
}


//==============================================================================

//
// Same thing as Index V4 to V5 auto class above, but this one simply makes a copy
// of the structure. It's useful for making a mutable copy of the structure.
// Also note the crazy usage of CAutoIndexT (go see its usage).
// This version also needs to copy rgcolumncreate, although I took a shortcut and
// did not make that generic for JET_COLUMNCREATE_W.
//
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

    //  Using the fact that copying the rgidx ptr doesn't matter, and the other items are identical.
    C_ASSERT( sizeof(m_tablecreateEngine) == sizeof(*ptablecreate) );
    Assert( sizeof(m_tablecreateEngine) == ptablecreate->cbStruct );

    memcpy( &(m_tablecreateEngine), ptablecreate, sizeof(*ptablecreate) );

    // Copy the column definitions to mutable memory.
    if ( ptablecreate->cColumns != 0 )
    {
        Assert( NULL != ptablecreate->rgcolumncreate );
        Alloc( m_rgcolumncreate = new JET_COLUMNCREATE_A[ ptablecreate->cColumns ] );
        memcpy( m_rgcolumncreate, ptablecreate->rgcolumncreate, sizeof( ptablecreate->rgcolumncreate[0] ) * ptablecreate->cColumns );

        m_tablecreateEngine.rgcolumncreate = m_rgcolumncreate;
    }

    //  Upgrade the index definitions.
    Alloc( m_pautoindices = new CAutoIndex_T() );
    Call( m_pautoindices->ErrSet( ptablecreate->rgindexcreate, ptablecreate->cIndexes ) );
    m_tablecreateEngine.rgindexcreate = m_pautoindices->GetPtr();

    Assert( m_tablecreateEngine.cIndexes == ptablecreate->cIndexes );
    Assert( m_tablecreateEngine.cIndexes > 0 || NULL == m_tablecreateEngine.rgindexcreate );

    C_ASSERT( sizeof( *ptablecreate ) == sizeof( m_tablecreateEngine ) );

    // The cbStruct should already be correct. But just in case it is not...
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

        //  Note: Do not need to copy over .err values of columns, because we cheaply
        //  copied the pointer to the array to the new structure.

        if ( ( m_ptablecreateFromAPI->grbit & JET_bitTableCreateImmutableStructure ) == 0 )
        {
            // Copy columnids and err values.
            for ( size_t i = 0; i < m_ptablecreateFromAPI->cColumns; ++i )
            {
                m_ptablecreateFromAPI->rgcolumncreate[ i ].columnid = m_tablecreateEngine.rgcolumncreate[ i ].columnid;
                m_ptablecreateFromAPI->rgcolumncreate[ i ].err = m_tablecreateEngine.rgcolumncreate[ i ].err;
            }

            //  Copy index .err values.
            Assert( m_pautoindices );
            if ( m_pautoindices )
            {
                m_pautoindices->Result();
            }

            //  Copy the final out params.
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



// Sets the members that all JET_LOGINFOMISC structures have
template<class LOGINFOMISC_T> void SetCommonLogInfoMiscMembers( const LGFILEHDR * const plgfilehdr, LOGINFOMISC_T * const ploginfomisc )
{
    ploginfomisc->ulGeneration = (ULONG)plgfilehdr->lgfilehdr.le_lGeneration;
    ploginfomisc->signLog = *(JET_SIGNATURE *)&plgfilehdr->lgfilehdr.signLog;
    ploginfomisc->logtimeCreate = *(JET_LOGTIME *)&plgfilehdr->lgfilehdr.tmCreate;
    ploginfomisc->logtimePreviousGeneration = *(JET_LOGTIME *)&plgfilehdr->lgfilehdr.tmPrevGen;
    ploginfomisc->ulFlags = plgfilehdr->lgfilehdr.fLGFlags;
    ploginfomisc->ulVersionMajor = plgfilehdr->lgfilehdr.le_ulMajor;
    //  Minor, and Update are moved to ErrGetLogInfoMiscFromLgfilehdr().
    ploginfomisc->cbSectorSize = plgfilehdr->lgfilehdr.le_cbSec;
    ploginfomisc->cbHeader = plgfilehdr->lgfilehdr.le_csecHeader;
    ploginfomisc->cbFile = plgfilehdr->lgfilehdr.le_csecLGFile;
    ploginfomisc->cbDatabasePageSize = plgfilehdr->lgfilehdr.le_cbPageSize;
}

// Checks the size of the input buffer, casts the input buffer to the JET_LOGINFOMISC* structure and then calls
// a function to set the members
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

// Sets the JET_LOGINFOMISC* structure from the given LGFILEHDR
LOCAL ERR ErrGetLogInfoMiscFromLgfilehdr( const LGFILEHDR * const plgfilehdr, void * const pvResult, const ULONG cbMax, const ULONG infoLevel )
{
    ERR err = JET_errSuccess;
    
    if( JET_LogInfoMisc == infoLevel )
    {
        Call( ErrSetCommonLogInfoMiscMembers<JET_LOGINFOMISC>( plgfilehdr, pvResult, cbMax ) );

        //  Set deprecated names ... 
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

/***********************************************************************/
/***********************  JET API FUNCTIONS  ***************************/
/***********************************************************************/

/*  APICORE.CPP
 */


/*=================================================================
JetIdle

Description:
  Performs idle time processing.

Parameters:
  sesid         uniquely identifies session
  grbit         processing options

Return Value:
  Error code

Errors/Warnings:
  JET_errSuccess        some idle processing occurred
  JET_wrnNoIdleActivity no idle processing occurred
=================================================================*/

LOCAL JET_ERR JetIdleEx( _In_ JET_SESID sesid, _In_ JET_GRBIT grbit )
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
JET_ERR JET_API JetIdle( _In_ JET_SESID sesid, _In_ JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opIdle, JetIdleEx( sesid, grbit ) );
}


LOCAL JET_ERR JetGetTableIndexInfoEx(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    __in_opt JET_PCSTR              szIndexName,
    __out_bcount( cbResult ) void * pvResult,
    _In_ ULONG              cbResult,
    _In_ ULONG              InfoLevel,
    _In_ const BOOL                 fUnicodeNames )
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
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    __in_opt JET_PCWSTR             wszIndexName,
    __out_bcount( cbResult ) void * pvResult,
    _In_ ULONG              cbResult,
    _In_ ULONG              InfoLevel )
{
    ERR         err             = JET_errSuccess;
    CAutoSZDDL  lszIndexName;

    CallR( lszIndexName.ErrSet( wszIndexName ) );

    return JetGetTableIndexInfoEx( sesid, tableid, lszIndexName, pvResult, cbResult, InfoLevel, fTrue );
}

JET_ERR JET_API JetGetTableIndexInfoA(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    __in_opt JET_PCSTR              szIndexName,
    __out_bcount( cbResult ) void * pvResult,
    _In_ ULONG              cbResult,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetTableIndexInfo, JetGetTableIndexInfoEx( sesid, tableid, szIndexName, pvResult, cbResult, InfoLevel, fFalse ) );
}

JET_ERR JET_API JetGetTableIndexInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    __in_opt JET_PCWSTR             wszIndexName,
    __out_bcount( cbResult ) void * pvResult,
    _In_ ULONG              cbResult,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetTableIndexInfo, JetGetTableIndexInfoExW( sesid, tableid, wszIndexName, pvResult, cbResult, InfoLevel ) );
}


LOCAL JET_ERR JetGetIndexInfoEx(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_PCSTR                  szTableName,
    __in_opt JET_PCSTR              szIndexName,
    __out_bcount( cbResult ) void * pvResult,
    _In_ ULONG              cbResult,
    _In_ ULONG              InfoLevel,
    _In_ const BOOL                 fUnicodeNames )
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
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_PCWSTR                 wszTableName,
    __in_opt JET_PCWSTR             wszIndexName,
    __out_bcount( cbResult ) void * pvResult,
    _In_ ULONG              cbResult,
    _In_ ULONG              InfoLevel )
{
    ERR         err         = JET_errSuccess;
    CAutoSZDDL  lszTableName;
    CAutoSZDDL  lszIndexName;

    CallR( lszTableName.ErrSet( wszTableName ) );
    CallR( lszIndexName.ErrSet( wszIndexName ) );

    return JetGetIndexInfoEx( sesid, dbid, lszTableName, lszIndexName, pvResult, cbResult, InfoLevel, fTrue );
}

JET_ERR JET_API JetGetIndexInfo(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_PCSTR                  szTableName,
    __in_opt JET_PCSTR              szIndexName,
    __out_bcount( cbResult ) void * pvResult,
    _In_ ULONG              cbResult,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetIndexInfo, JetGetIndexInfoEx( sesid, dbid, szTableName, szIndexName, pvResult, cbResult, InfoLevel, fFalse ) );
}

JET_ERR JET_API JetGetIndexInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_PCWSTR                 wszTableName,
    __in_opt JET_PCWSTR             wszIndexName,
    __out_bcount( cbResult ) void * pvResult,
    _In_ ULONG              cbResult,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetIndexInfo, JetGetIndexInfoExW( sesid, dbid, wszTableName, wszIndexName, pvResult, cbResult, InfoLevel ) );
}

LOCAL JET_ERR JetGetObjectInfoEx(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_OBJTYP                 objtyp,
    _In_ JET_PCSTR                  szContainerName,
    __in_opt JET_PCSTR              szObjectName,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel,
    _In_ const BOOL                 fUnicodeNames )
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
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_OBJTYP                 objtyp,
    __in_opt JET_PCWSTR             wszContainerName,
    __in_opt JET_PCWSTR             wszObjectName,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    ERR         err                 = JET_errSuccess;
    CAutoSZDDL  lszContainerName;
    CAutoSZDDL  lszObjectName;

    CallR( lszContainerName.ErrSet( wszContainerName ) );
    CallR( lszObjectName.ErrSet( wszObjectName ) );

    return JetGetObjectInfoEx( sesid, dbid, objtyp, lszContainerName, lszObjectName, pvResult, cbMax, InfoLevel, fTrue );
}

JET_ERR JET_API JetGetObjectInfoA(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_OBJTYP                 objtyp,
    __in_opt JET_PCSTR              szContainerName,
    __in_opt JET_PCSTR              szObjectName,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetObjectInfo, JetGetObjectInfoEx( sesid, dbid, objtyp, szContainerName, szObjectName, (void*) pvResult, cbMax, InfoLevel, fFalse ) );
}

JET_ERR JET_API JetGetObjectInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_OBJTYP                 objtyp,
    __in_opt JET_PCWSTR             wszContainerName,
    __in_opt JET_PCWSTR             wszObjectName,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
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
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
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
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetTableInfo, JetGetTableInfoEx( sesid, tableid, pvResult, cbMax, InfoLevel ) );
}

JET_ERR JET_API JetGetTableInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
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

// Identical implementation for A and W version currently
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

LOCAL JET_ERR JetBeginTransactionEx( _In_ JET_SESID sesid, _In_ TRXID trxid, _In_ JET_GRBIT grbit )
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
JET_ERR JET_API JetBeginTransaction( _In_ JET_SESID sesid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opBeginTransaction, JetBeginTransactionEx( sesid, 57573, NO_GRBIT ) );
}
JET_ERR JET_API JetBeginTransaction2( _In_ JET_SESID sesid, _In_ JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opBeginTransaction, JetBeginTransactionEx( sesid, 32997, grbit ) );
}
JET_ERR JET_API JetBeginTransaction3( _In_ JET_SESID sesid, _In_ __int64 trxid, _In_ JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opBeginTransaction, JetBeginTransactionEx( sesid, trxid, grbit ) );
}


LOCAL JET_ERR JetPrepareToCommitTransactionEx(
    _In_ JET_SESID                      sesid,
    __in_bcount( cbData ) const void *  pvData,
    _In_ ULONG                  cbData,
    _In_ JET_GRBIT                      grbit )
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
    _In_ JET_SESID                      sesid,
    __in_bcount( cbData ) const void *  pvData,
    _In_ ULONG                  cbData,
    _In_ JET_GRBIT                      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opPrepareToCommitTransaction, JetPrepareToCommitTransactionEx( sesid, pvData, cbData, grbit ) );
}


LOCAL JET_ERR JetCommitTransactionEx( _In_ JET_SESID sesid, _In_ JET_GRBIT grbit, _In_ DWORD cmsecDurableCommit, __out_opt JET_COMMIT_ID *pCommitId )
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

JET_ERR JET_API JetCommitTransaction( _In_ JET_SESID sesid, _In_ JET_GRBIT grbit )
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


LOCAL JET_ERR JetRollbackEx( _In_ JET_SESID sesid, _In_ JET_GRBIT grbit )
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
JET_ERR JET_API JetRollback( _In_ JET_SESID sesid, _In_ JET_GRBIT grbit )
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

    if ( apicall.FEnter( sesid, fTrue ) )   //  ignore StopService (may need to know trx level for rollback purposes)
    {
        apicall.LeaveAfterCall( ErrIsamGetSessionInfo( sesid, pvResult, cbMax, ulInfoLevel ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetGetSessionInfo(
    _In_ JET_SESID                  sesid,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ const ULONG        cbMax,
    _In_ const ULONG        ulInfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetSessionInfo, JetGetSessionInfoEx( sesid, (void*) pvResult, cbMax, ulInfoLevel ) );
}


LOCAL JET_ERR JetOpenTableEx(
    _In_ JET_SESID                                  sesid,
    _In_ JET_DBID                                   dbid,
    _In_ JET_PCSTR                                  szTableName,
    __in_bcount_opt( cbParameters ) const void *    pvParameters,
    _In_ ULONG                              cbParameters,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid )
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
    _In_ JET_SESID                                  sesid,
    _In_ JET_DBID                                   dbid,
    _In_ JET_PCWSTR                                 wszTableName,
    __in_bcount_opt( cbParameters ) const void *    pvParameters,
    _In_ ULONG                              cbParameters,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid )
{
    ERR     err;
    CAutoSZDDL lszTableName;

    CallR( lszTableName.ErrSet( wszTableName ) );

    return JetOpenTableEx( sesid, dbid, lszTableName, pvParameters, cbParameters, grbit, ptableid );
}

JET_ERR JET_API JetOpenTableA(
    _In_ JET_SESID                                  sesid,
    _In_ JET_DBID                                   dbid,
    _In_ JET_PCSTR                                  szTableName,
    __in_bcount_opt( cbParameters ) const void *    pvParameters,
    _In_ ULONG                              cbParameters,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOpenTable, JetOpenTableEx( sesid, dbid, szTableName, pvParameters, cbParameters, grbit, ptableid ) );
}


JET_ERR JET_API JetOpenTableW(
    _In_ JET_SESID                                  sesid,
    _In_ JET_DBID                                   dbid,
    _In_ JET_PCWSTR                                 wszTableName,
    __in_bcount_opt( cbParameters ) const void *    pvParameters,
    _In_ ULONG                              cbParameters,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOpenTable, JetOpenTableExW( sesid, dbid, wszTableName, pvParameters, cbParameters, grbit, ptableid ) );
}

//  ================================================================
LOCAL JET_ERR JetSetTableSequentialEx(
    _In_ const JET_SESID    sesid,
    _In_ const JET_TABLEID  tableid,
    _In_ const JET_GRBIT    grbit )
//  ================================================================
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
//  ================================================================
JET_ERR JET_API JetSetTableSequential(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_GRBIT      grbit )
//  ================================================================
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetTableSequential, JetSetTableSequentialEx( sesid, tableid, grbit ) );
}


//  ================================================================
LOCAL JET_ERR JetResetTableSequentialEx(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID        tableid,
    _In_ JET_GRBIT      grbit )
//  ================================================================
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
//  ================================================================
JET_ERR JET_API JetResetTableSequential(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_GRBIT      grbit )
//  ================================================================
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opResetTableSequential, JetResetTableSequentialEx( sesid, tableid, grbit ) );
}

LOCAL JET_ERR JetRegisterCallbackEx(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_CBTYP      cbtyp,
    _In_ JET_CALLBACK   pCallback,
    __in_opt void *     pvContext,
    _In_ JET_HANDLE *   phCallbackId )
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
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_CBTYP      cbtyp,
    _In_ JET_CALLBACK   pCallback,
    __in_opt void *     pvContext,
    _In_ JET_HANDLE *   phCallbackId )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opRegisterCallback, JetRegisterCallbackEx( sesid, tableid, cbtyp, pCallback, pvContext, phCallbackId ) );
}


LOCAL JET_ERR JetUnregisterCallbackEx(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_CBTYP      cbtyp,
    _In_ JET_HANDLE     hCallbackId )
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
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_CBTYP      cbtyp,
    _In_ JET_HANDLE     hCallbackId )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opUnregisterCallback, JetUnregisterCallbackEx( sesid, tableid, cbtyp, hCallbackId ) );
}


LOCAL JET_ERR JetSetLSEx(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_LS         ls,
    _In_ JET_GRBIT      grbit )
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
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_LS         ls,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetLS, JetSetLSEx( sesid, tableid, ls, grbit ) );
}


LOCAL JET_ERR JetGetLSEx(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _Out_ JET_LS *      pls,
    _In_ JET_GRBIT      grbit )
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
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _Out_ JET_LS *      pls,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetLS, JetGetLSEx( sesid, tableid, pls, grbit ) );
}

LOCAL JET_ERR JetTracingEx(
    _In_ const JET_TRACEOP  traceop,
    _In_ const JET_TRACETAG tracetag,
    _In_ const JET_API_PTR  ul )
{
    JET_ERR             err     = JET_errSuccess;

    //  UNDONE: will we get into weird self-referencing
    //  problems trying to trace the tracing API?
    //
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
                //  enable/disable specified tag
                //
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
            //  UNDONE: currently unsupported, but don't err out, just let
            //  the application call this, even though it does nothing
            //
            break;

        case JET_traceopRegisterTag:
            if ( 0 != ul && tracetag > JET_tracetagNull && tracetag < JET_tracetagMax )
            {
                ULONG_PTR   ulT;

                //  callback to register the tag
                //
                CAutoISZ<JET_tracetagDescCbMax> szTraceDesc;

                err = szTraceDesc.ErrSet( g_rgwszTraceDesc[tracetag] );
                CallS( err );
                Call( err );

                (*(JET_PFNTRACEREGISTER)ul)( tracetag, szTraceDesc, &ulT );

                //  store the returned value with the info for this tag
                //
                OSTraceSetTagData( tracetag, ulT );
            }
            else
            {
                //  no callback or invalid tag specified
                //
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

                    //  callback to register the tag
                    //
                    (*(JET_PFNTRACEREGISTER)ul)( (JET_TRACETAG)itag, szTraceDesc, &ulT );

                    //  store the returned value with the info for this tag
                    //
                    OSTraceSetTagData( itag, ulT );
                }
            }
            else
            {
                //  no callback specified
                //
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

#endif  // !MINIMAL_FUNCTIONALITY

HandleError:
    return err;
}
JET_ERR JET_API JetTracing(
    _In_ const JET_TRACEOP  traceop,
    _In_ const JET_TRACETAG tracetag,
    _In_ const JET_API_PTR  ul )
{
    JET_TRY( opTracing, JetTracingEx( traceop, tracetag, ul ) );
}


LOCAL JET_ERR JetDupCursorEx(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _Out_ JET_TABLEID * ptableid,
    _In_ JET_GRBIT      grbit )
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
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _Out_ JET_TABLEID * ptableid,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDupCursor, JetDupCursorEx( sesid, tableid, ptableid, grbit ) );
}


LOCAL JET_ERR JetCloseTableEx( _In_ JET_SESID sesid, _In_ JET_TABLEID tableid )
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
JET_ERR JET_API JetCloseTable( _In_ JET_SESID sesid, _In_ JET_TABLEID tableid )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCloseTable, JetCloseTableEx( sesid, tableid ) );
}

const ULONG     maskGetColumInfoGrbits      = ( JET_ColInfoGrbitNonDerivedColumnsOnly | JET_ColInfoGrbitMinimalInfo | JET_ColInfoGrbitSortByColumnid | JET_ColInfoGrbitCompacting );
const ULONG     maskGetColumInfoLevelBase   = ~maskGetColumInfoGrbits;


LOCAL JET_ERR JetGetTableColumnInfoEx(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    __in_opt JET_PCSTR              pColumnNameOrId,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel,
    _In_ const BOOL                 fUnicodeNames )
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
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    __in_opt JET_PCWSTR             pColumnNameOrId,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
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

        // preserve the original error if we have success
        //
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
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    __in_opt JET_PCSTR              szColumnName,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetTableColumnInfo, JetGetTableColumnInfoEx( sesid, tableid, szColumnName, (void*) pvResult, cbMax, InfoLevel, fFalse ) );
}

JET_ERR JET_API JetGetTableColumnInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    __in_opt JET_PCWSTR             wszColumnName,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetTableColumnInfo, JetGetTableColumnInfoExW( sesid, tableid, wszColumnName, (void*) pvResult, cbMax, InfoLevel ) );
}

LOCAL JET_ERR JetGetColumnInfoEx(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_PCSTR                  szTableName,
    __in_opt JET_PCSTR              pColumnNameOrId,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel,
    _In_ const BOOL                 fUnicodeNames )
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
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_PCWSTR                 wszTableName,
    __in_opt JET_PCWSTR             pwColumnNameOrId,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
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

        // preserve the original error if we have success
        //
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
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_PCSTR                  szTableName,
    __in_opt JET_PCSTR              pColumnNameOrId,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetColumnInfo, JetGetColumnInfoEx( sesid, dbid, szTableName, pColumnNameOrId, pvResult, cbMax, InfoLevel, fFalse ) );
}

JET_ERR JET_API JetGetColumnInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_PCWSTR                 wszTableName,
    __in_opt JET_PCWSTR             pwColumnNameOrId,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetColumnInfo, JetGetColumnInfoExW( sesid, dbid, wszTableName, pwColumnNameOrId, pvResult, cbMax, InfoLevel ) );
}

LOCAL JET_ERR JetRetrieveColumnEx(
    _In_ JET_SESID                                      sesid,
    _In_ JET_TABLEID                                    tableid,
    _In_ JET_COLUMNID                                   columnid,
    __out_bcount_part_opt( cbData, min( cbData, *pcbActual ) ) void *   pvData,
    _In_ ULONG                                  cbData,
    __out_opt ULONG *                           pcbActual,
    _In_ JET_GRBIT                                      grbit,
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
    _In_ JET_SESID                                              sesid,
    _In_ JET_TABLEID                                            tableid,
    __inout_ecount_opt( cretrievecolumn ) JET_RETRIEVECOLUMN *  pretrievecolumn,
    _In_ ULONG                                          cretrievecolumn )
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
    _In_ JET_SESID                                                                              sesid,
    _In_ JET_TABLEID                                                                            tableid,
    _Out_ ULONG *                                                                       pcColumns,
    __out_xcount_part_opt( cbData, *pcColumns * sizeof( JET_RETRIEVEMULTIVALUECOUNT ) ) void *  pvData,
    _In_ ULONG                                                                          cbData,
    _In_ JET_COLUMNID                                                                           columnidStart,
    _In_ JET_GRBIT                                                                              grbit )
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
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    __inout JET_RECSIZE3 *  precsize,
    _In_ const JET_GRBIT    grbit )
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
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    __inout JET_RECSIZE3 *  precsize,
    _In_ const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetRecordSize, JetGetRecordSize3Ex( sesid, tableid, precsize, grbit ) );
}

LOCAL JET_ERR JetGetRecordSize2Ex(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    __inout JET_RECSIZE2 *  precsize,
    _In_ const JET_GRBIT    grbit )
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
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    __inout JET_RECSIZE2 *  precsize,
    _In_ const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetRecordSize, JetGetRecordSize2Ex( sesid, tableid, precsize, grbit ) );
}

LOCAL JET_ERR JetGetRecordSizeEx(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    __inout JET_RECSIZE *   precsize,
    _In_ const JET_GRBIT    grbit )
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
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    __inout JET_RECSIZE *   precsize,
    _In_ const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetRecordSize, JetGetRecordSizeEx( sesid, tableid, precsize, grbit ) );
}


LOCAL JET_ERR JetSetColumnEx(
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    _In_ JET_COLUMNID                       columnid,
    __in_bcount_opt( cbData ) const void *  pvData,
    _In_ ULONG                      cbData,
    _In_ JET_GRBIT                          grbit,
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
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    _In_ JET_COLUMNID                       columnid,
    __in_bcount_opt( cbData ) const void *  pvData,
    _In_ ULONG                      cbData,
    _In_ JET_GRBIT                          grbit,
    __in_opt JET_SETINFO *                  psetinfo )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetColumn, JetSetColumnEx( sesid, tableid, columnid, pvData, cbData, grbit, psetinfo ) );
}


LOCAL JET_ERR JetSetColumnsEx(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    __in_ecount_opt( csetcolumn ) JET_SETCOLUMN *   psetcolumn,
    _In_ ULONG                              csetcolumn )
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
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    __in_ecount_opt( csetcolumn ) JET_SETCOLUMN *   psetcolumn,
    _In_ ULONG                              csetcolumn )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetColumns, JetSetColumnsEx( sesid, tableid, psetcolumn, csetcolumn ) );
}


LOCAL JET_ERR JetPrepareUpdateEx(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ ULONG  prep )
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
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ ULONG  prep )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opPrepareUpdate, JetPrepareUpdateEx( sesid, tableid, prep ) );
}


LOCAL JET_ERR JetUpdateEx(
    _In_ JET_SESID                                          sesid,
    _In_ JET_TABLEID                                        tableid,
    __out_bcount_part_opt( cbBookmark, *pcbActual ) void *  pvBookmark,
    _In_ ULONG                                      cbBookmark,
    __out_opt ULONG *                               pcbActual,
    _In_ const JET_GRBIT                                    grbit )
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
    _In_ JET_SESID                                          sesid,
    _In_ JET_TABLEID                                        tableid,
    _In_ JET_COLUMNID                                       columnid,
    __in_bcount( cbMax ) void *                             pv,
    _In_ ULONG                                      cbMax,
    __out_bcount_part_opt( cbOldMax, *pcbOldActual ) void * pvOld,
    _In_ ULONG                                      cbOldMax,
    __out_opt ULONG *                               pcbOldActual,
    _In_ JET_GRBIT                                          grbit )
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


LOCAL JET_ERR JetDeleteEx( _In_ JET_SESID sesid, _In_ JET_TABLEID tableid )
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
JET_ERR JET_API JetDelete( _In_ JET_SESID sesid, _In_ JET_TABLEID tableid )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDelete, JetDeleteEx( sesid, tableid ) );
}


LOCAL JET_ERR JetGetCursorInfoEx(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
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
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetCursorInfo, JetGetCursorInfoEx( sesid, tableid, pvResult, cbMax, InfoLevel ) );
}


LOCAL JET_ERR JetGetCurrentIndexEx(
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    _Out_z_bytecap_( cbIndexName ) JET_PSTR szIndexName,
    _In_ ULONG                      cbIndexName )
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
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    _Out_z_bytecap_( cbIndexName ) JET_PWSTR    szIndexName,
    _In_ ULONG                      cbIndexName )
{
    ERR         err     = JET_errSuccess;
    CAutoSZDDL  lszIndexName;

    CallR( lszIndexName.ErrAlloc( cbIndexName ) );

    CallR( JetGetCurrentIndexEx( sesid, tableid, lszIndexName.Pv(), lszIndexName.Cb() ) );

    return lszIndexName.ErrGet( szIndexName, cbIndexName );
}

JET_ERR JET_API JetGetCurrentIndexA(
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    __out_bcount( cbIndexName ) JET_PSTR    szIndexName,
    _In_ ULONG                      cbIndexName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetCurrentIndex, JetGetCurrentIndexEx( sesid, tableid, szIndexName, cbIndexName ) );
}

JET_ERR JET_API JetGetCurrentIndexW(
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    __out_bcount( cbIndexName ) JET_PWSTR   szIndexName,
    _In_ ULONG                      cbIndexName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetCurrentIndex, JetGetCurrentIndexExW( sesid, tableid, szIndexName, cbIndexName ) );
}

LOCAL JET_ERR JetSetCurrentIndexEx(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    __in_opt JET_PCSTR      szIndexName,
    __in_opt JET_INDEXID *  pindexid,
    _In_ JET_GRBIT          grbit,
    _In_ ULONG      itagSequence )
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
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    __in_opt JET_PCWSTR     wszIndexName,
    __in_opt JET_INDEXID *  pindexid,
    _In_ JET_GRBIT          grbit,
    _In_ ULONG      itagSequence )
{
    ERR         err;
    CAutoSZDDL  lszIndexName;

    CallR( lszIndexName.ErrSet( wszIndexName ) );

    return JetSetCurrentIndexEx( sesid, tableid, lszIndexName, pindexid, grbit, itagSequence );
}

JET_ERR JET_API JetSetCurrentIndexA(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    __in_opt JET_PCSTR  szIndexName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexEx( sesid, tableid, szIndexName, NULL, JET_bitMoveFirst, 1 ) );
}

JET_ERR JET_API JetSetCurrentIndexW(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    __in_opt JET_PCWSTR wszIndexName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexExW( sesid, tableid, wszIndexName, NULL, JET_bitMoveFirst, 1 ) );
}
JET_ERR JET_API JetSetCurrentIndex2A(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    __in_opt JET_PCSTR  szIndexName,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexEx( sesid, tableid, szIndexName, NULL, grbit, 1 ) );
}
JET_ERR JET_API JetSetCurrentIndex2W(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    __in_opt JET_PCWSTR wszIndexName,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexExW( sesid, tableid, wszIndexName, NULL, grbit, 1 ) );
}

JET_ERR JET_API JetSetCurrentIndex3A(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    __in_opt JET_PCSTR  szIndexName,
    _In_ JET_GRBIT      grbit,
    _In_ ULONG  itagSequence )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexEx( sesid, tableid, szIndexName, NULL, grbit, itagSequence ) );
}
JET_ERR JET_API JetSetCurrentIndex3W(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    __in_opt JET_PCWSTR wszIndexName,
    _In_ JET_GRBIT      grbit,
    _In_ ULONG  itagSequence )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexExW( sesid, tableid, wszIndexName, NULL, grbit, itagSequence ) );
}
JET_ERR JET_API JetSetCurrentIndex4A(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    __in_opt JET_PCSTR      szIndexName,
    __in_opt JET_INDEXID *  pindexid,
    _In_ JET_GRBIT          grbit,
    _In_ ULONG      itagSequence )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexEx( sesid, tableid, szIndexName, pindexid, grbit, itagSequence ) );
}
JET_ERR JET_API JetSetCurrentIndex4W(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    __in_opt JET_PCWSTR     wszIndexName,
    __in_opt JET_INDEXID *  pindexid,
    _In_ JET_GRBIT          grbit,
    _In_ ULONG      itagSequence )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCurrentIndex, JetSetCurrentIndexExW( sesid, tableid, wszIndexName, pindexid, grbit, itagSequence ) );
}


LOCAL JET_ERR JetMoveEx(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _In_ LONG               cRow,
    _In_ JET_GRBIT          grbit )
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
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ LONG           cRow,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opMove, JetMoveEx( sesid, tableid, cRow, grbit ) );
}

LOCAL JET_ERR JetSetCursorFilterEx(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    __in_ecount(cFilters) JET_INDEX_COLUMN *rgFilters,
    _In_ DWORD              cFilters,
    _In_ JET_GRBIT          grbit )
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
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    __in_ecount(cFilters) JET_INDEX_COLUMN *rgFilters,
    _In_ DWORD              cFilters,
    _In_ JET_GRBIT          grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetCursorFilter, JetSetCursorFilterEx( sesid, tableid, rgFilters, cFilters, grbit ) );
}

LOCAL JET_ERR JetMakeKeyEx(
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    __in_bcount_opt( cbData ) const void *  pvData,
    _In_ ULONG                      cbData,
    _In_ JET_GRBIT                          grbit )
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
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    __in_bcount_opt( cbData ) const void *  pvData,
    _In_ ULONG                      cbData,
    _In_ JET_GRBIT                          grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opMakeKey, JetMakeKeyEx( sesid, tableid, pvData, cbData, grbit ) );
}


LOCAL JET_ERR JetSeekEx( _In_ JET_SESID sesid, _In_ JET_TABLEID tableid, _In_ JET_GRBIT grbit )
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
JET_ERR JET_API JetSeek( _In_ JET_SESID sesid, _In_ JET_TABLEID tableid, _In_ JET_GRBIT grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSeek, JetSeekEx( sesid, tableid, grbit ) );
}


LOCAL JET_ERR JetPrereadKeysEx(
    _In_ const JET_SESID                                sesid,
    _In_ const JET_TABLEID                              tableid,
    __in_ecount(ckeys) const void * const * const       rgpvKeys,
    __in_ecount(ckeys) const ULONG * const      rgcbKeys,
    _In_ const LONG                                     ckeys,
    __out_opt LONG * const                              pckeysPreread,
    _In_ const JET_GRBIT                                grbit )
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
    _In_ JET_SESID                                      sesid,
    _In_ JET_TABLEID                                    tableid,
    __in_ecount(ckeys) const void **                    rgpvKeys,
    __in_ecount(ckeys) const ULONG *            rgcbKeys,
    _In_ LONG                                           ckeys,
    __out_opt LONG *                                    pckeysPreread,
    _In_ JET_GRBIT                                      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opPrereadKeys, JetPrereadKeysEx( sesid, tableid, rgpvKeys, rgcbKeys, ckeys, pckeysPreread, grbit ) );
}

LOCAL JET_ERR JetPrereadIndexRangesEx(
    _In_ const JET_SESID                                sesid,
    _In_ const JET_TABLEID                              tableid,
    __in_ecount(cIndexRanges) const JET_INDEX_RANGE * const rgIndexRanges,
    _In_ const ULONG                            cIndexRanges,
    __out_opt ULONG * const                     pcRangesPreread,
    __in_ecount(ccolumnidPreread) const JET_COLUMNID * const rgcolumnidPreread,
    _In_ const ULONG                            ccolumnidPreread,
    _In_ const JET_GRBIT                                grbit )
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
    _In_ const JET_SESID                                sesid,
    _In_ const JET_TABLEID                              tableid,
    __in_ecount(cIndexRanges) const JET_INDEX_RANGE * const rgIndexRanges,
    _In_ const ULONG                            cIndexRanges,
    __out_opt ULONG * const                     pcRangesPreread,
    __in_ecount(ccolumnidPreread) const JET_COLUMNID * const rgcolumnidPreread,
    _In_ const ULONG                            ccolumnidPreread,
    _In_ const JET_GRBIT                                grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opPrereadIndexRanges, JetPrereadIndexRangesEx( sesid, tableid, rgIndexRanges, cIndexRanges, pcRangesPreread, rgcolumnidPreread, ccolumnidPreread, grbit ) );
}

LOCAL JET_ERR JetGetBookmarkEx(
    _In_ JET_SESID                                      sesid,
    _In_ JET_TABLEID                                    tableid,
    __out_bcount_part_opt( cbMax, *pcbActual ) void *   pvBookmark,
    _In_ ULONG                                  cbMax,
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
    _In_ JET_SESID                                                                  sesid,
    _In_ JET_TABLEID                                                                tableid,
    __out_bcount_part_opt( cbSecondaryKeyMax, *pcbSecondaryKeyActual ) void *       pvSecondaryKey,
    _In_ ULONG                                                              cbSecondaryKeyMax,
    __out_opt ULONG *                                                       pcbSecondaryKeyActual,
    __out_bcount_part_opt( cbPrimaryBookmarkMax, *pcbPrimaryBookmarkActual ) void * pvPrimaryBookmark,
    _In_ ULONG                                                              cbPrimaryBookmarkMax,
    __out_opt ULONG *                                                       pcbPrimaryBookmarkActual )
{
    APICALL_SESID   apicall( opGetSecondaryIndexBookmark );

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
    JET_TRY( opGetSecondaryIndexBookmark,
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
    _In_ JET_SESID                      sesid,
    _In_ JET_TABLEID                    tableid,
    __in_bcount( cbBookmark ) void *    pvBookmark,
    _In_ ULONG                  cbBookmark )
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
    _In_ JET_SESID                      sesid,
    _In_ JET_TABLEID                    tableid,
    __in_bcount( cbBookmark ) void *    pvBookmark,
    _In_ ULONG                  cbBookmark )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGotoBookmark, JetGotoBookmarkEx( sesid, tableid, pvBookmark, cbBookmark ) );
}

JET_ERR JET_API JetGotoSecondaryIndexBookmarkEx(
    _In_ JET_SESID                              sesid,
    _In_ JET_TABLEID                            tableid,
    __in_bcount( cbSecondaryKey ) void *        pvSecondaryKey,
    _In_ ULONG                          cbSecondaryKey,
    __in_bcount_opt( cbPrimaryBookmark ) void * pvPrimaryBookmark,
    _In_ ULONG                          cbPrimaryBookmark,
    _In_ const JET_GRBIT                        grbit )
{
    APICALL_SESID   apicall( opGotoSecondaryIndexBookmark );

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
    _In_ JET_SESID                              sesid,
    _In_ JET_TABLEID                            tableid,
    __in_bcount( cbSecondaryKey ) void *        pvSecondaryKey,
    _In_ ULONG                          cbSecondaryKey,
    __in_bcount_opt( cbPrimaryBookmark ) void * pvPrimaryBookmark,
    _In_ ULONG                          cbPrimaryBookmark,
    _In_ const JET_GRBIT                        grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetSecondaryIndexBookmark,
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
    _In_ JET_SESID                              sesid,
    __in_ecount( cindexrange ) JET_INDEXRANGE * rgindexrange,
    _In_ ULONG                          cindexrange,
    __inout JET_RECORDLIST *                    precordlist,
    _In_ JET_GRBIT                              grbit )
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
        //  not dispatched because we don't have a tableid to dispatch on
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
    _In_ JET_SESID                              sesid,
    __in_ecount( cindexrange ) JET_INDEXRANGE * rgindexrange,
    _In_ ULONG                          cindexrange,
    __inout JET_RECORDLIST *                    precordlist,
    _In_ JET_GRBIT                              grbit )
{
    JET_VALIDATE_SESID( sesid );
    //  UNDONE:  validate tableids inside JET_INDEXRANGE array
    JET_TRY( opIntersectIndexes, JetIntersectIndexesEx( sesid, rgindexrange, cindexrange, precordlist, grbit ) );
}


LOCAL JET_ERR JetGetRecordPositionEx(
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    __out_bcount( cbRecpos ) JET_RECPOS *   precpos,
    _In_ ULONG                      cbRecpos )
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
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    __out_bcount( cbRecpos ) JET_RECPOS *   precpos,
    _In_ ULONG                      cbRecpos )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetRecordPosition, JetGetRecordPositionEx( sesid, tableid, precpos, cbRecpos ) );
}


LOCAL JET_ERR JetGotoPositionEx(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_RECPOS *   precpos )
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
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_RECPOS *   precpos )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGotoPosition, JetGotoPositionEx( sesid, tableid, precpos ) );
}


LOCAL JET_ERR JetRetrieveKeyEx(
    _In_ JET_SESID                                      sesid,
    _In_ JET_TABLEID                                    tableid,
    __out_bcount_part_opt( cbMax, *pcbActual ) void *   pvKey,
    _In_ ULONG                                  cbMax,
    __out_opt ULONG *                           pcbActual,
    _In_ JET_GRBIT                                      grbit )
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


LOCAL JET_ERR JetGetLockEx( _In_ JET_SESID sesid, _In_ JET_TABLEID tableid, _In_ JET_GRBIT grbit )
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
JET_ERR JET_API JetGetLock( _In_ JET_SESID sesid, _In_ JET_TABLEID tableid, _In_ JET_GRBIT grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opGetLock, JetGetLockEx( sesid, tableid, grbit ) );
}

LOCAL JET_ERR JetGetVersionEx( _In_ JET_SESID sesid, _Out_ ULONG  *pVersion )
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
        //  Our scheme for ESENT is as follows:
        //      bits        ESENT
        //      ----        -----
        //      0-7         ServicePack
        //      8-23        Build Major
        //      24-27       Image Minor
        //      28-31       Image Major
        //
        //  for ESE it is as follows:
        //
        //      bits        ESE
        //      ----        ---
        //      0-7         Build Minor
        //      8-21        Build Major
        //      22-26       Image Minor
        //      27-31       Image Major
        //
        //
        
#ifdef ESENT
        //  assert no aliasing (i.e. overlap) of version information
        Assert( DwUtilImageVersionMajor()     < 1 <<  4 );
        Assert( DwUtilImageVersionMinor()     < 1 <<  4 );
        Assert( DwUtilImageBuildNumberMajor() < 1 << 16 );

        //  Windows is now using Build Number Minor values that are too big to
        //  fit into our version scheme, so we will use the Service Pack number
        //  instead.

        Assert( DwUtilSystemServicePackNumber() < 1 << 8 );


        const ULONG ulVersion   = ( ( DwUtilImageVersionMajor()       & 0xF )    << 28 ) +
                                  ( ( DwUtilImageVersionMinor()       & 0xF )    << 24 ) +
                                  ( ( DwUtilImageBuildNumberMajor()   & 0xFFFF ) <<  8 ) +
                                    ( DwUtilSystemServicePackNumber() & 0xFF );

#else // !ESENT
        //  assert no aliasing (i.e. overlap) of version information
        Assert( DwUtilImageVersionMajor()     < 1 <<  5 );
        Assert( DwUtilImageVersionMinor()     < 1 <<  5 );
        Assert( DwUtilImageBuildNumberMajor() < 1 << 14 );
        Assert( DwUtilImageBuildNumberMinor() < 1 <<  8 );

        const ULONG ulVersion   = ( ( DwUtilImageVersionMajor()       & 0x1F )   << 27 ) +
                                  ( ( DwUtilImageVersionMinor()       & 0x1F )   << 22 ) +
                                  ( ( DwUtilImageBuildNumberMajor()   & 0x3FFF ) <<  8 ) +
                                    ( DwUtilImageBuildNumberMinor()   & 0xFF );

#endif // !ESENT

        *pVersion = ulVersion;

        apicall.LeaveAfterCall( JET_errSuccess );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetGetVersion( _In_ JET_SESID sesid, _Out_ ULONG  *pVersion )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetVersion, JetGetVersionEx( sesid, pVersion ) );
}

#pragma warning( disable : 4509 )

/*=================================================================
JetGetSystemParameter

Description:
  This function returns the current settings of the system parameters.

Parameters:
  sesid         is the optional session identifier for dynamic parameters.
  paramid       is the system parameter code identifying the parameter.
  plParam       is the returned parameter value.
  sz            is the zero terminated string parameter buffer.
  cbMax         is the size of the string parameter buffer.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidParameter:
    Invalid parameter code.
  JET_errInvalidSesid:
    Dynamic parameters require a valid session id.

Side Effects:
  None.
=================================================================*/
LOCAL JET_ERR JetGetSystemParameterEx(
    _In_ JET_INSTANCE                   instance,
    _In_ JET_SESID                      sesid,
    _In_ ULONG                  paramid,
    __out_opt JET_API_PTR *             plParam,
    _Out_opt_z_bytecap_( cbMax ) JET_PWSTR  wszParam,
    _In_ ULONG                  cbMax )
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

    //  try to divine what instance we are addressing (if any) based on our
    //  args.  this is complicated due to backwards compatability
    //
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

    //  if no instance was found then we can not lock the API.  also, ignore
    //  any provided sesid because it is meaningless
    //
    if ( !pinst )
    {
        ERR err = ErrGetSystemParameter( NULL, JET_sesidNil, paramid, plParam, wszParam, cbMax );
        INST::LeaveCritInst();
        return err;
    }
    else
    {
        //  get the parameter
        //
        //  while we can't allow actions to run concurrent with JetInit, we can allow an exemption
        //  for if we're in a callback while JetInit (b/c JetInit can't tear down the instance
        //  until the callback returns).
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
    _In_ JET_INSTANCE                   instance,
    _In_ JET_SESID                      sesid,
    _In_ ULONG                  paramid,
    __out_opt JET_API_PTR *             plParam,
    _Out_opt_z_bytecap_( cbMax ) JET_PSTR   szParam,
    _In_ ULONG                  cbMax )
{
    ERR         err     = JET_errSuccess;
    ULONG       cbMaxT  = cbMax;
    CAutoWSZ    lwszParam; // 90 is optimization

    if ( szParam && cbMax )
    {
        CallR( lwszParam.ErrAlloc( cbMax * sizeof( WCHAR ) ) );
        cbMaxT = lwszParam.Cb();
    }

    CallR( JetGetSystemParameterEx( instance, sesid, paramid, plParam, lwszParam.Pv(), cbMaxT ) );

    // if no input buffer, get out right here
    if ( !szParam || !cbMax )
    {
        return err;
    }

    return lwszParam.ErrGet( (CHAR *)szParam, cbMax );
}


JET_ERR JET_API JetGetSystemParameterA(
    _In_ JET_INSTANCE                   instance,
    __in_opt JET_SESID                  sesid,
    _In_ ULONG                  paramid,
    __out_opt JET_API_PTR *             plParam,
    __out_bcount_opt( cbMax ) JET_PSTR  szParam,
    _In_ ULONG                  cbMax )
{
    JET_TRY( opGetSystemParameter, JetGetSystemParameterExA( instance, sesid, paramid, plParam, szParam, cbMax ) );
}

JET_ERR JET_API JetGetSystemParameterW(
    _In_ JET_INSTANCE                   instance,
    __in_opt JET_SESID                  sesid,
    _In_ ULONG                  paramid,
    __out_opt JET_API_PTR *             plParam,
    __out_bcount_opt( cbMax ) JET_PWSTR szParam,
    _In_ ULONG                  cbMax )
{
    JET_TRY( opGetSystemParameter, JetGetSystemParameterEx( instance, sesid, paramid, plParam, szParam, cbMax ) );
}

/*=================================================================
JetBeginSession

Description:
  This function signals the start of a session for a given user.  It must
  be the first function called by the application on behalf of that user.

  The username and password supplied must correctly identify a user account
  in the security accounts subsystem of the engine for which this session
  is being started.  Upon proper identification and authentication, a SESID
  is allocated for the session, a user token is created for the security
  subject, and that user token is specifically associated with the SESID
  of this new session for the life of that SESID (until JetEndSession is
  called).

Parameters:
  psesid        is the unique session identifier returned by the system.
  szUsername    is the username of the user account for logon purposes.
  szPassword    is the password of the user account for logon purposes.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:

Side Effects:
  * Allocates resources which must be freed by JetEndSession().
=================================================================*/

LOCAL JET_ERR JetBeginSessionEx(
    _In_ JET_INSTANCE   instance,
    _Out_ JET_SESID *   psesid,
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
        //  tell the ISAM to start a new session
        apicall.LeaveAfterCall( ErrIsamBeginSession(
                                        (JET_INSTANCE)apicall.Pinst(),
                                        psesid ) );
    }

    return apicall.ErrResult();
}


LOCAL JET_ERR JetBeginSessionExW(
    _In_ JET_INSTANCE   instance,
    _Out_ JET_SESID *   psesid,
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
    _In_ JET_INSTANCE   instance,
    _Out_ JET_SESID *   psesid,
    __in_opt JET_PCSTR  szUserName,
    __in_opt JET_PCSTR  szPassword )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBeginSession, JetBeginSessionEx( instance, psesid, szUserName, szPassword ) );
}

JET_ERR JET_API JetBeginSessionW(
    _In_ JET_INSTANCE   instance,
    _Out_ JET_SESID *   psesid,
    __in_opt JET_PCWSTR wszUserName,
    __in_opt JET_PCWSTR wszPassword )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBeginSession, JetBeginSessionExW( instance, psesid, wszUserName, wszPassword ) );
}

LOCAL JET_ERR JetDupSessionEx( _In_ JET_SESID sesid, _Out_ JET_SESID *psesid )
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
JET_ERR JET_API JetDupSession( _In_ JET_SESID sesid, _Out_ JET_SESID *psesid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDupSession, JetDupSessionEx( sesid, psesid ) );
}

/*=================================================================
JetEndSession

Description:
  This routine ends a session with a Jet engine.

Parameters:
  sesid         identifies the session uniquely

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidSesid:
    The SESID supplied is invalid.

Side Effects:
=================================================================*/
LOCAL JET_ERR JetEndSessionEx( _In_ JET_SESID sesid, _In_ JET_GRBIT grbit )
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
        //  must call special routine LeaveAfterEndSession()
        //  because we can't reference the PIB anymore if
        //  ErrIsamEndSession() returns success
        //
        apicall.LeaveAfterEndSession( ErrIsamEndSession( sesid, grbit ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetEndSession( _In_ JET_SESID sesid, _In_ JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opEndSession, JetEndSessionEx( sesid, grbit ) );
}

LOCAL JET_ERR JetCreateDatabaseEx(
    _In_ JET_SESID                                              sesid,
    _In_ JET_PCWSTR                                             wszDbFileName,
    _Out_ JET_DBID *                                            pdbid,
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
    _In_ JET_SESID                                              sesid,
    _In_ JET_PCSTR                                              szDbFileName,
    _Out_ JET_DBID *                                            pdbid,
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
    _In_ JET_SESID      sesid,
    _In_ JET_PCSTR      szFilename,
    __in_opt JET_PCSTR  szConnect,
    _Out_ JET_DBID *    pdbid,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateDatabase, JetCreateDatabaseExA( sesid, szFilename, pdbid, NULL, 0, grbit ) );
}

JET_ERR JET_API JetCreateDatabaseW(
    _In_ JET_SESID      sesid,
    _In_ JET_PCWSTR     wszFilename,
    __in_opt JET_PCWSTR wszConnect,
    _Out_ JET_DBID *    pdbid,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateDatabase, JetCreateDatabaseEx( sesid, wszFilename, pdbid, NULL, 0, grbit ) );
}

JET_ERR JET_API JetCreateDatabase2A(
    _In_ JET_SESID              sesid,
    _In_ JET_PCSTR              szFilename,
    _In_ ULONG          cpgDatabaseSizeMax,
    _Out_ JET_DBID *            pdbid,
    _In_ JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    const JET_SETDBPARAM rgsetdbparam[] =
    {
            { JET_dbparamDbSizeMaxPages, &cpgDatabaseSizeMax, sizeof( cpgDatabaseSizeMax ) }
    };
    JET_TRY( opCreateDatabase, JetCreateDatabaseExA( sesid, szFilename, pdbid, rgsetdbparam, _countof( rgsetdbparam ), grbit ) );
}

JET_ERR JET_API JetCreateDatabase2W(
    _In_ JET_SESID              sesid,
    _In_ JET_PCWSTR             wszFilename,
    _In_ ULONG          cpgDatabaseSizeMax,
    _Out_ JET_DBID *            pdbid,
    _In_ JET_GRBIT              grbit )
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
    _In_ JET_SESID              sesid,
    _In_ JET_PCSTR              szDbFileName,
    _Reserved_ JET_PCSTR        szSLVFileName,
    _Reserved_ JET_PCSTR        szSLVRootName,
    _In_ const ULONG    cpgDatabaseSizeMax,
    _Out_ JET_DBID *            pdbid,
    _In_ JET_GRBIT              grbit )
{
    return ErrERRCheck( JET_errFeatureNotAvailable );
}

JET_ERR JET_API JetCreateDatabaseWithStreamingW(
    _In_ JET_SESID              sesid,
    _In_ JET_PCWSTR             wszDbFileName,
    _Reserved_ JET_PCWSTR           wszSLVFileName,
    _Reserved_ JET_PCWSTR           wszSLVRootName,
    _In_ const ULONG    cpgDatabaseSizeMax,
    _Out_ JET_DBID *            pdbid,
    _In_ JET_GRBIT              grbit )
{
    return ErrERRCheck( JET_errFeatureNotAvailable );
}

LOCAL JET_ERR JetOpenDatabaseEx(
    JET_SESID       sesid,
    _In_ PCWSTR     wszDatabase,
    _In_ PCWSTR     wszConnect,
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
    _In_ JET_SESID      sesid,
    _In_ JET_PCSTR      szFilename,
    __in_opt JET_PCSTR  szConnect,
    _Out_ JET_DBID*     pdbid,
    _In_ JET_GRBIT      grbit )
{
    ERR             err;
    CAutoWSZPATH    lwszDatabase;
    CAutoWSZ        lwszConnect;

    CallR( lwszDatabase.ErrSet( szFilename ) );
    CallR( lwszConnect.ErrSet( szConnect ) );

    return JetOpenDatabaseEx( sesid, lwszDatabase, lwszConnect, pdbid, grbit );
}

JET_ERR JET_API JetOpenDatabaseA(
    _In_ JET_SESID      sesid,
    _In_ JET_PCSTR      szFilename,
    __in_opt JET_PCSTR  szConnect,
    _Out_ JET_DBID*     pdbid,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOpenDatabase, JetOpenDatabaseExA( sesid, szFilename, szConnect, pdbid, grbit ) );
}

JET_ERR JET_API JetOpenDatabaseW(
    _In_ JET_SESID      sesid,
    _In_ JET_PCWSTR     wszFilename,
    __in_opt JET_PCWSTR wszConnect,
    _Out_ JET_DBID*     pdbid,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOpenDatabase, JetOpenDatabaseEx( sesid, wszFilename, wszConnect, pdbid, grbit ) );
}

LOCAL JET_ERR JetCloseDatabaseEx(  _In_ JET_SESID sesid,  _In_ JET_DBID dbid, _In_ JET_GRBIT grbit )
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
JET_ERR JET_API JetCloseDatabase( _In_ JET_SESID sesid, _In_ JET_DBID dbid, _In_ JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCloseDatabase, JetCloseDatabaseEx( sesid, dbid, grbit ) );
}


LOCAL JET_ERR JetGetDatabaseInfoEx(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
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
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    ERR         err     = JET_errSuccess;
    CAutoWSZFixedConversion lwsz;

    if ( JET_DbInfoFilename != InfoLevel )
    {
        return JetGetDatabaseInfoEx( sesid, dbid, pvResult, cbMax, InfoLevel );
    }

    // buffer is too large
    if ( (~(ULONG)0) / sizeof(WCHAR) < cbMax )
    {
        CallR( JET_errOutOfMemory );
    }

    CallR( lwsz.ErrAlloc( cbMax * sizeof( WCHAR ) ) );

    CallR( JetGetDatabaseInfoEx( sesid, dbid, lwsz.Pv(), lwsz.Cb(), InfoLevel ) );

    return lwsz.ErrGet( (CHAR *)pvResult, cbMax );
}

JET_ERR JET_API JetGetDatabaseInfoA(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetDatabaseInfo, JetGetDatabaseInfoExA( sesid, dbid, pvResult, cbMax, InfoLevel ) );
}

JET_ERR JET_API JetGetDatabaseInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetDatabaseInfo, JetGetDatabaseInfoEx( sesid, dbid, pvResult, cbMax, InfoLevel ) );
}

LOCAL JET_ERR JetGetDatabaseFileInfoEx(
    _In_ JET_PCWSTR                 wszDatabaseName,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    ERR             err                 = JET_errSuccess;
    IFileSystemAPI* pfsapi              = NULL;
    IFileFindAPI*   pffapi              = NULL;
    WCHAR           wszFullDbName[ IFileSystemAPI::cchPathMax ];
    QWORD           cbFileSize          = 0;
    QWORD           cbFileSizeOnDisk    = 0;
    DBFILEHDR*      pdbfilehdr          = NULL;
    BOOL            fOSUInitCalled      = fFalse;

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

    g_rwlOSUInit.EnterAsReader();
    Call( ErrOSUInit() );
    fOSUInitCalled = fTrue;

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

            //  UNDONE: need more accurate estimates of space and time requirements
            pdbinfoupgd->csecToUpgrade = ULONG( ( cbFileSize * 3600 ) >> 30 );  // shr 30 == divide by 1Gb
            cbFileSize = ( cbFileSize * 10 ) >> 6;                              // shr 6 == divide by 64; 10/64 is roughly 15%
            pdbinfoupgd->cbFreeSpaceRequiredLow     = DWORD( cbFileSize );
            pdbinfoupgd->cbFreeSpaceRequiredHigh    = DWORD( cbFileSize >> 32 );

            //  bring in the database and check its header
            Alloc( pdbfilehdr = (DBFILEHDR_FIX * )PvOSMemoryPageAlloc( g_cbPage, NULL ) );

            //  need to zero out header because we try to read it
            //  later on even on failure
            memset( pdbfilehdr, 0, g_cbPage );

            //  verify flags initialised
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
            Assert( err <= JET_errSuccess );    // shouldn't get warnings

            //  Checksumming may have changed, which is probably
            //  why we got errors, so just force success.
            //  If there truly was an error, then the fUpgradable flag
            //  will be fFalse to indicate the database is not upgradable.
            //  If the user later tries to upgrade anyways, the error will
            //  be detected when the database is opened.
            err = JET_errSuccess;

            //  If able to read header, ignore any errors and check version
            //  Note that the magic number stays the same since 500.
            if ( ulDAEMagic == pdbfilehdr->le_ulMagic )
            {
                if ( pdbfilehdr->le_ulVersion == ulDAEVersionMax )
                {
                    //  Regular default / non-legacy case
                    Assert( !pdbinfoupgd->fUpgradable );
                    pdbinfoupgd->fAlreadyUpgraded = fTrue;
                    if ( pdbfilehdr->Dbstate() == JET_dbstateBeingConverted )
                        err = ErrERRCheck( JET_errDatabaseIncompleteUpgrade );
                }
                else
                {
                    //  Legacy case
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

                            //  HACK! HACK! HACK!
                            //  there was a bug where the Exchange 4.0 skeleton DIR.EDB
                            //  was mistakenly stamped with ulDAEVersion400.
                            pdbinfoupgd->fUpgradable = ( 0 == _wcsicmp( wszDbFileName, L"dir.edb" ) ? fTrue : fFalse );
                            Assert( !pdbinfoupgd->fAlreadyUpgraded );
                            break;
                        }
                        default:
                            //  unsupported upgrade format
                            Assert( !pdbinfoupgd->fUpgradable );
                            Assert( !pdbinfoupgd->fAlreadyUpgraded );
                            break;
                    }
                } // end else
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
                    if ( pdbfilehdr->le_filetype != JET_filetypeDatabase )
                    {
                        Error( ErrERRCheck( JET_errFileInvalidType ) );
                    }
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

            // no fmp table
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

    if ( fOSUInitCalled )
    {
        OSUTerm();
    }
    g_rwlOSUInit.LeaveAsReader();

    return err;
}

LOCAL JET_ERR JetGetDatabaseFileInfoExA(
    _In_ JET_PCSTR                  szDatabaseName,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
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

    // Initialize necessary subsystems

    Assert( fFalse == fOSUInitCalled );

    g_rwlOSUInit.EnterAsReader();
    Call( ErrOSUInit() );
    fOSUInitCalled = fTrue;
    Call( ErrOSFSCreate( g_pfsconfigGlobal, &pfsapi ) );

    // Fix the DB files to remove these log files if possible

    Call( ErrIsamRemoveLogfile( pfsapi, wszDatabase, wszLogfile, grbit ) );

HandleError:
    delete pfsapi;
    if( fOSUInitCalled )
    {
        OSUTerm();
    }
    g_rwlOSUInit.LeaveAsReader();
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
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCWSTR     wszDatabase,
    _In_ unsigned long  genFirstDivergedLog,
    _In_ JET_GRBIT      grbit )
{
    APICALL_INST    apicall( opBeginDatabaseIncrementalReseed );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%p[%s],0x%x, 0x%x)",
            __FUNCTION__,
            instance,
            wszDatabase,
            SzOSFormatStringW( wszDatabase ),
            genFirstDivergedLog,
            grbit ) );

    if ( apicall.FEnterWithoutInit( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamBeginDatabaseIncrementalReseed( (JET_INSTANCE)apicall.Pinst(), wszDatabase, genFirstDivergedLog, grbit ) );
    }

    return apicall.ErrResult();
}

JET_ERR
JET_API
JetBeginDatabaseIncrementalReseedW(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCWSTR     wszDatabase,
    _In_ unsigned long  genFirstDivergedLog,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBeginDatabaseIncrementalReseed, JetBeginDatabaseIncrementalReseedEx( instance, wszDatabase, genFirstDivergedLog, grbit ) );
}

LOCAL
JET_ERR
JetBeginDatabaseIncrementalReseedExA(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCSTR      szDatabase,
    _In_ unsigned long  genFirstDivergedLog,
    _In_ JET_GRBIT      grbit )
{
    ERR             err             = JET_errSuccess;
    CAutoWSZPATH    lwszDatabase;

    CallR( lwszDatabase.ErrSet( szDatabase ) );

    return JetBeginDatabaseIncrementalReseedEx( instance, lwszDatabase, genFirstDivergedLog, grbit );
}

JET_ERR
JET_API
JetBeginDatabaseIncrementalReseedA(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCSTR      szDatabase,
    _In_ unsigned long  genFirstDivergedLog,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBeginDatabaseIncrementalReseed, JetBeginDatabaseIncrementalReseedExA( instance, szDatabase, genFirstDivergedLog, grbit ) );
}

LOCAL
JET_ERR
JetEndDatabaseIncrementalReseedEx(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCWSTR     wszDatabase,
    _In_ ULONG  genMinRequired,
    _In_ ULONG  genFirstDivergedLog,
    _In_ ULONG  genMaxRequired,
    _In_ JET_GRBIT      grbit )
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
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCWSTR     wszDatabase,
    _In_ ULONG  genMinRequired,
    _In_ ULONG  genFirstDivergedLog,
    _In_ ULONG  genMaxRequired,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opEndDatabaseIncrementalReseed, JetEndDatabaseIncrementalReseedEx( instance, wszDatabase, genMinRequired, genFirstDivergedLog, genMaxRequired, grbit ) );
}

LOCAL
JET_ERR
JetEndDatabaseIncrementalReseedExA(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCSTR      szDatabase,
    _In_ ULONG  genMinRequired,
    _In_ ULONG  genFirstDivergedLog,
    _In_ ULONG  genMaxRequired,
    _In_ JET_GRBIT      grbit )
{
    ERR             err             = JET_errSuccess;
    CAutoWSZPATH    lwszDatabase;

    CallR( lwszDatabase.ErrSet( szDatabase ) );

    return JetEndDatabaseIncrementalReseedEx( instance, lwszDatabase, genMinRequired, genFirstDivergedLog, genMaxRequired, grbit );
}

JET_ERR
JET_API
JetEndDatabaseIncrementalReseedA(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCSTR      szDatabase,
    _In_ ULONG  genMinRequired,
    _In_ ULONG  genFirstDivergedLog,
    _In_ ULONG  genMaxRequired,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opEndDatabaseIncrementalReseed, JetEndDatabaseIncrementalReseedExA( instance, szDatabase, genMinRequired, genFirstDivergedLog, genMaxRequired, grbit ) );
}


LOCAL
JET_ERR
JetPatchDatabasePagesEx(
    _In_ JET_INSTANCE               instance,
    _In_ JET_PCWSTR                 wszDatabase,
    _In_ ULONG              pgnoStart,
    _In_ ULONG              cpg,
    __in_bcount( cb ) const void *  pv,
    _In_ ULONG              cb,
    _In_ JET_GRBIT                  grbit )
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
    _In_ JET_INSTANCE               instance,
    _In_ JET_PCWSTR                 wszDatabase,
    _In_ ULONG              pgnoStart,
    _In_ ULONG              cpg,
    __in_bcount( cb ) const void *  pv,
    _In_ ULONG              cb,
    _In_ JET_GRBIT                  grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opPatchDatabasePages, JetPatchDatabasePagesEx( instance, wszDatabase, pgnoStart, cpg, pv, cb, grbit ) );
}

LOCAL
JET_ERR
JetPatchDatabasePagesExA(
    _In_ JET_INSTANCE               instance,
    _In_ JET_PCSTR                  szDatabase,
    _In_ ULONG              pgnoStart,
    _In_ ULONG              cpg,
    __in_bcount( cb ) const void *  pv,
    _In_ ULONG              cb,
    _In_ JET_GRBIT                  grbit )
{
    ERR             err             = JET_errSuccess;
    CAutoWSZPATH    lwszDatabase;

    CallR( lwszDatabase.ErrSet( szDatabase ) );

    return JetPatchDatabasePagesEx( instance, lwszDatabase, pgnoStart, cpg, pv, cb, grbit );
}

JET_ERR
JET_API
JetPatchDatabasePagesA(
    _In_ JET_INSTANCE               instance,
    _In_ JET_PCSTR                  szDatabase,
    _In_ ULONG              pgnoStart,
    _In_ ULONG              cpg,
    __in_bcount( cb ) const void *  pv,
    _In_ ULONG              cb,
    _In_ JET_GRBIT                  grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opPatchDatabasePages, JetPatchDatabasePagesExA( instance, szDatabase, pgnoStart, cpg, pv, cb, grbit ) );
}


JET_ERR JET_API JetGetDatabaseFileInfoA(
    _In_ JET_PCSTR                  szDatabaseName,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_TRY( opGetDatabaseFileInfo, JetGetDatabaseFileInfoExA( szDatabaseName, pvResult, cbMax, InfoLevel ) );
}

JET_ERR JET_API JetGetDatabaseFileInfoW(
    _In_ JET_PCWSTR                 wszDatabaseName,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_TRY( opGetDatabaseFileInfo, JetGetDatabaseFileInfoEx( wszDatabaseName, pvResult, cbMax, InfoLevel ) );
}

LOCAL JET_ERR JetGetLogFileInfoEx(
    _In_ JET_PCWSTR                 wszLog,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ const ULONG        cbMax,
    _In_ const ULONG        InfoLevel )
{
    ERR                 err                 = JET_errSuccess;
    LGFILEHDR *         plgfilehdr          = NULL;
    IFileSystemAPI *    pfsapi              = NULL;
    IFileAPI *          pfapi               = NULL;
    BOOL                fOSUInitCalled      = fFalse;

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

    //  initialize the OSU layer if necessary
    //
    g_rwlOSUInit.EnterAsReader();
    Call( ErrOSUInit() );
    fOSUInitCalled = fTrue;

    //  open the file
    //
    Call( ErrOSFSCreate( g_pfsconfigGlobal, &pfsapi ) );
    //  Generally fmfReadOnlyPermissive is not very safe as it allows concurrent writing, but for log file headers it
    //  is safe because a few special factors:
    //    A) log header is only written once generally, so unlikely that we either get 0, or get a valid copy.
    //    B) We read only the header, and read it once ... so there is no in-consitent versions we could get.
    //    C) And of course after reading we check the header checksum to validate it we didn't get a partial view.
    //  This should allow a very highly consistent API experience, in spite of the permissive flag.
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

            //  allocate buffer
            //
            Alloc( plgfilehdr = (LGFILEHDR * )PvOSMemoryPageAlloc( sizeof(LGFILEHDR), NULL ) );

            //  open the specified file
            //
            Call( ErrLGIReadFileHeader( pfapi, *TraceContextScope( iorpDirectAccessUtil ), qosIONormal, plgfilehdr ) );

            //  return info to caller
            //
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

    if ( fOSUInitCalled )
    {
        OSUTerm();
    }
    g_rwlOSUInit.LeaveAsReader();

    return err;
}

JET_ERR JET_API JetGetLogFileInfoExA(
    _In_ JET_PCSTR                  szLog,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ const ULONG        cbMax,
    _In_ const ULONG        InfoLevel )
{
        ERR err = JET_errSuccess;
    CAutoWSZPATH    lwszLogFileName;

    CallR( lwszLogFileName.ErrSet( szLog ) );

    return JetGetLogFileInfoEx( lwszLogFileName, pvResult, cbMax, InfoLevel );
}

JET_ERR JET_API JetGetLogFileInfoA(
    _In_ JET_PCSTR                  szLog,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ const ULONG        cbMax,
    _In_ const ULONG        InfoLevel )
{
    JET_TRY( opGetLogFileInfo, JetGetLogFileInfoExA( szLog, pvResult, cbMax, InfoLevel ) );
}

JET_ERR JET_API JetGetLogFileInfoW(
    _In_ JET_PCWSTR                 wszLog,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ const ULONG        cbMax,
    _In_ const ULONG        InfoLevel )
{
    JET_TRY( opGetLogFileInfo, JetGetLogFileInfoEx( wszLog, pvResult, cbMax, InfoLevel ) );
}


LOCAL JET_ERR JetGetPageInfoEx(
    __in_bcount(cbData)         void * const            pvPages,        //  raw page data
                                const ULONG cbData,         //  size of raw page data
    __inout_bcount(cbPageInfo)  void* const         rgPageInfo,     //  array of pageinfo structures
                                const ULONG cbPageInfo,     //  length of buffer for pageinfo array
                                const JET_GRBIT     grbit,          //  options
                                ULONG       ulInfoLevel )   //  info level
{
    JET_ERR err;
    ULONG cbStruct = 0;

    //  validate the parameters
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
        Error( ErrERRCheck( JET_errInvalidParameter ) );    //  array sizes or page size mismatched
    }

    if( ( cbStruct * ( cbData / g_cbPage ) ) > cbPageInfo )
    {
        // Technically, we could say "> cbPageInfo" instead of "== cbPageInfo", but
        // showing that the client knows the exact size, is proof of probably correctness.
        Error( ErrERRCheck( JET_errInvalidParameter ) );    //  page size may be wrong
    }

    // further it would be odd, to not have an exactly sized buffer.
    Assert( cbStruct  * ( cbData / g_cbPage ) == cbPageInfo );

    if( 0 != ( (unsigned __int64)pvPages % g_cbPage ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  initialize pageinfo structures and check for invalid page numbers
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

    //  fill in the page information

    err = JET_errSuccess;   //  assume success, fill in error code otherwise

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

        // if page is initialized, fill out rest of fields.
        if( pInfo->fPageIsInitialized )
        {
            const DWORD * const     rgdwPage        = (DWORD *)pvPage;
            PGNO                    pgno            = pInfo->pgno;
            PAGETYPE                pagetype        = databasePage;
            PAGECHECKSUM            checksumExpected;
            PAGECHECKSUM            checksumActual;
            BOOL                    fCorrectableError;
            INT                     ibitCorrupted;

            //  see if this actually looks more like the
            //  database trailer page than an actual
            //  database page. pgno 0 and pgno -1 are also
            //  treated as header pages
            //
            if ( ( 0 == rgdwPage[1]     //  checksum (high dword)
                && 0 == rgdwPage[2]     //  dbtimeDirtied (low dword)
                && 0 == rgdwPage[3]     //  dbtimeDirtied (high dword)
                && 0 == rgdwPage[6] )   //  objidFDP
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
                //pInfo->flags = cpage.FFlags();
            }
            else if ( databaseHeader == pagetype )
            {
                pInfo->dbtime = ((DBFILEHDR_FIX*)pvPage)->le_dbtimeDirtied;
                if ( fComputeStructureChecksum )
                {
                    pInfo->structureChecksum = pInfo->checksumActual;
                }
                //pInfo->flags = cpage.FFlags();
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
    __in_bcount( cbData ) void * const          pvPages,        //  raw page data
    _In_ ULONG                          cbData,         //  size of raw page data
    __inout_bcount( cbPageInfo ) JET_PAGEINFO * rgPageInfo,     //  array of pageinfo structures
    _In_ ULONG                          cbPageInfo,     //  length of buffer for pageinfo array
    _In_ JET_GRBIT                              grbit,          //  options
    _In_ ULONG                          ulInfoLevel )   //  info level
{
    // only support JET_PageInfo, use JetGetPageInfo2() for more levels
    if ( JET_PageInfo != ulInfoLevel )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    JET_TRY( opGetPageInfo, JetGetPageInfoEx( pvPages, cbData, rgPageInfo, cbPageInfo, grbit, JET_PageInfo ) );
}

JET_ERR JET_API JetGetPageInfo2(
    __in_bcount( cbData ) void * const          pvPages,        //  raw page data
    _In_ ULONG                          cbData,         //  size of raw page data
    __inout_bcount( cbPageInfo ) void * const   rgPageInfo,     //  array of pageinfo structures
    _In_ ULONG                          cbPageInfo,     //  length of buffer for pageinfo array
    _In_ JET_GRBIT                              grbit,          //  options
    _In_ ULONG                          ulInfoLevel )   //  info level
{
    JET_TRY( opGetPageInfo, JetGetPageInfoEx( pvPages, cbData, rgPageInfo, cbPageInfo, grbit, ulInfoLevel ) );
}

LOCAL
JET_ERR
JetGetDatabasePagesEx(
    _In_ JET_SESID                              sesid,
    _In_ JET_DBID                               dbid,
    _In_ ULONG                          pgnoStart,
    _In_ ULONG                          cpg,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    _In_ ULONG                          cb,
    _Out_ ULONG *                       pcbActual,
    _In_ JET_GRBIT                              grbit )
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
    _In_ JET_SESID                              sesid,
    _In_ JET_DBID                               dbid,
    _In_ ULONG                          pgnoStart,
    _In_ ULONG                          cpg,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    _In_ ULONG                          cb,
    _Out_ ULONG *                       pcbActual,
    _In_ JET_GRBIT                              grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetDatabasePages, JetGetDatabasePagesEx( sesid, dbid, pgnoStart, cpg, pv, cb, pcbActual, grbit ) );
}

JET_ERR
JET_API
JetOnlinePatchDatabasePageEx(
    _In_ JET_SESID                              sesid,
    _In_ JET_DBID                               dbid,
    _In_ ULONG                          pgno,
    __in_bcount(cbToken)    const void *        pvToken,
    _In_ ULONG                          cbToken,
    __in_bcount(cbData) const void *            pvData,
    _In_ ULONG                          cbData,
    _In_ JET_GRBIT                              grbit )
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
    _In_ JET_SESID                              sesid,
    _In_ JET_DBID                               dbid,
    _In_ ULONG                          pgno,
    __in_bcount(cbToken)    const void *        pvToken,
    _In_ ULONG                          cbToken,
    __in_bcount(cbData) const void *            pvData,
    _In_ ULONG                          cbData,
    _In_ JET_GRBIT                              grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOnlinePatchDatabasePage, JetOnlinePatchDatabasePageEx( sesid, dbid, pgno, pvToken, cbToken, pvData, cbData, grbit ) );
}


LOCAL JET_ERR JetCreateTableEx(
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    _In_ JET_PCSTR      szTableName,
    _In_ ULONG  lPages,
    _In_ ULONG  lDensity,
    _Out_ JET_TABLEID * ptableid )
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
                                0,  // No columns/indexes
                                NULL,
                                0,  // No callbacks
                                0,  // grbit
                                NULL,
                                NULL,
                                0,
                                0,
                                JET_tableidNil, // returned tableid
                                0   // returned count of objects created
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

        //  the following statement automatically set to tableid Nil on error
        *ptableid = tablecreate.tableid;

        //  either the table was created or it was not
        Assert( 0 == tablecreate.cCreated || 1 == tablecreate.cCreated );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetCreateTableExW(
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    _In_ JET_PCWSTR     wszTableName,
    _In_ ULONG  lPages,
    _In_ ULONG  lDensity,
    _Out_ JET_TABLEID * ptableid )
{
    ERR             err             = JET_errSuccess;
    CAutoSZDDL      lszTableName;

    CallR( lszTableName.ErrSet( wszTableName ) );

    return JetCreateTableEx( sesid, dbid, lszTableName, lPages, lDensity, ptableid );
}

JET_ERR JET_API JetCreateTableA(
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    _In_ JET_PCSTR      szTableName,
    _In_ ULONG  lPages,
    _In_ ULONG  lDensity,
    _Out_ JET_TABLEID * ptableid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTable, JetCreateTableEx( sesid, dbid, szTableName, lPages, lDensity, ptableid ) );
}

JET_ERR JET_API JetCreateTableW(
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    _In_ JET_PCWSTR     wszTableName,
    _In_ ULONG  lPages,
    _In_ ULONG  lDensity,
    _Out_ JET_TABLEID * ptableid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTable, JetCreateTableExW( sesid, dbid, wszTableName, lPages, lDensity, ptableid ) );
}


class CAutoIDXCREATE2
{
    public:
        CAutoIDXCREATE2():m_pindexcreate( NULL ),m_pcondcol( NULL ), m_rgconditionalcolumn( NULL ),
            //  Hold pointers for 2 potential versions.
            m_pindexcreate1W( NULL ), m_pindexcreate2W( NULL ) { }
        ~CAutoIDXCREATE2();

    public:
        //  This CAutoIDXCREATE2 can take both current level and downleve index defintions, in
        //  both cases it converts to current level (2) index definitions.
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
        JET_INDEXCREATE_W       * m_pindexcreate1W;             // kept to know where to put the err back
        JET_INDEXCREATE2_W       * m_pindexcreate2W;            // kept to know where to put the err back
        JET_INDEXCREATE3_W       * m_pindexcreate3W;            // kept to know where to put the err back
        JET_INDEXCREATE2_A       * m_pindexcreate; // engine version
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
    // we reserve space for the longest key possible including the 2 zeros as the end
    // plus the potential LANGID and CbVarSegMac at the end
    const ULONG cbAllocateMax = sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 + ( sizeof(LANGID) + 1 + 1 + sizeof( BYTE ) + 1 + 1 ) );
    Call( m_szKey.ErrAlloc( cbAllocateMax ) );
    Call( ErrOSSTRUnicodeToAsciiM( pindexcreate->szKey, m_szKey.Pv(),
                sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 ), &cchActual, OSSTR_FIXED_CONVERSION ) );
    m_pindexcreate->szKey = m_szKey;
    m_pindexcreate->cbKey = cchActual * sizeof( char );

    // we may have a LANGID or LANGID+CbVarSegMac at the end
    //
    Assert ( pindexcreate->cbKey >= sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate->szKey ) + 1 ) );

    // we are in the limits of allocation
    //
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
    // we are in the limits of allocation
    //
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

    //  No space hints from V1.
    m_pindexcreate->pSpacehints = NULL;

    // maintain the original
    m_pindexcreate1W = pindexcreate1;

    return err;
}

ERR CAutoIDXCREATE2::ErrSet( JET_INDEXCREATE2_W * pindexcreate2 )
{
    C_ASSERT( sizeof(JET_INDEXCREATE2_W) == sizeof(JET_INDEXCREATE2_A) );

    //  This is legal b/c we only added something to the end
    ERR err;
    CallR( _ErrSet( reinterpret_cast<JET_INDEXCREATE_W*>(pindexcreate2) ) );

    //  now dress it up w/ the space hints we know we have.
    m_pindexcreate->pSpacehints = pindexcreate2->pSpacehints;

    // maintain the original
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
    // we reserve space for the longest key possible including the 2 zeros as the end
    // plus the potential LANGID and CbVarSegMac at the end
    const ULONG cbAllocateMax = sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 + ( sizeof(LANGID) + 1 + 1 + sizeof( BYTE ) + 1 + 1 ) );
    Call( m_szKey.ErrAlloc( cbAllocateMax ) );
    Call( ErrOSSTRUnicodeToAsciiM( pindexcreate3->szKey, m_szKey.Pv(),
                sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 ), &cchActual, OSSTR_FIXED_CONVERSION ) );
    m_pindexcreate->szKey = m_szKey;
    m_pindexcreate->cbKey = cchActual * sizeof( char );

    // we may have a LANGID or LANGID+CbVarSegMac at the end
    //
    Assert ( pindexcreate3->cbKey >= sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate3->szKey ) + 1 ) );

    // we are in the limits of allocation
    //
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
    // we are in the limits of allocation
    //
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

    //  now dress it up w/ the space hints we know we have.
    m_pindexcreate->pSpacehints = pindexcreate3->pSpacehints;

    // maintain the original
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
        //  This CAutoIDXCREATE3 can take JET_INDEXCREATE2_W and
        //  converts to current level (3) index definitions.
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
        JET_INDEXCREATE2_W       * m_pindexcreate2W;            // kept to know where to put the err back
        JET_INDEXCREATE3_W       * m_pindexcreate3W;            // kept to know where to put the err back
        JET_INDEXCREATE3_A       * m_pindexcreate; // engine version
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
    // we reserve space for the longest key possible including the 2 zeros as the end
    // plus the potential LANGID and CbVarSegMac at the end
    const ULONG cbAllocateMax = sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 + ( sizeof(LANGID) + 1 + 1 + sizeof( BYTE ) + 1 + 1 ) );
    Call( m_szKey.ErrAlloc( cbAllocateMax ) );
    Call( ErrOSSTRUnicodeToAsciiM( pindexcreate2->szKey, m_szKey.Pv(),
                sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 ), &cchActual, OSSTR_FIXED_CONVERSION ) );
    m_pindexcreate->szKey = m_szKey;
    m_pindexcreate->cbKey = cchActual * sizeof( char );

    // we may have a LANGID or LANGID+CbVarSegMac at the end
    //
    Assert ( pindexcreate2->cbKey >= sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate2->szKey ) + 1 ) );

    // we are in the limits of allocation
    //
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
    // we are in the limits of allocation
    //
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

    //  now dress it up w/ the space hints we know we have.
    m_pindexcreate->pSpacehints = pindexcreate2->pSpacehints;

    // maintain the original
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
    // we reserve space for the longest key possible including the 2 zeros as the end
    // plus the potential LANGID and CbVarSegMac at the end
    const ULONG cbAllocateMax = sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 + ( sizeof(LANGID) + 1 + 1 + sizeof( BYTE ) + 1 + 1 ) );
    Call( m_szKey.ErrAlloc( cbAllocateMax ) );
    Call( ErrOSSTRUnicodeToAsciiM( pindexcreate3->szKey, m_szKey.Pv(),
                sizeof(char) * ( (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 ), &cchActual, OSSTR_FIXED_CONVERSION ) );
    m_pindexcreate->szKey = m_szKey;
    m_pindexcreate->cbKey = cchActual * sizeof( char );

    // we may have a LANGID or LANGID+CbVarSegMac at the end
    //
    Assert ( pindexcreate3->cbKey >= sizeof( WCHAR ) * ( LOSStrLengthMW( pindexcreate3->szKey ) + 1 ) );

    // we are in the limits of allocation
    //
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
    // we are in the limits of allocation
    //
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
            // This is the Invariant Locale ("").
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

    //  now dress it up w/ the space hints we know we have.
    m_pindexcreate->pSpacehints = pindexcreate3->pSpacehints;

    // maintain the original
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
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
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
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
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
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
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
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
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
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
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
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    __inout JET_TABLECREATE2_W *    ptablecreate );


LOCAL JET_ERR JetCreateTableColumnIndexExW(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
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
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    __inout JET_TABLECREATE_A * ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexExOLD( sesid, dbid, ptablecreate ) );
}

JET_ERR JET_API JetCreateTableColumnIndexW(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
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

    // maintain the original
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

    // maintain the original
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

    // maintain the original
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
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    __inout JET_TABLECREATE3_W *    ptablecreate );


LOCAL JET_ERR JetCreateTableColumnIndexEx2W(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
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
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    __inout JET_TABLECREATE2_A *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx2( sesid, dbid, ptablecreate ) );
}

JET_ERR JET_API JetCreateTableColumnIndex2W(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    __inout JET_TABLECREATE2_W *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx2W( sesid, dbid, ptablecreate ) );
}

JET_ERR JET_API JetCreateTableColumnIndex3A(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    __inout JET_TABLECREATE3_A *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx3( sesid, dbid, ptablecreate ) );
}

JET_ERR JET_API JetCreateTableColumnIndexEx3W(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
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
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    __inout JET_TABLECREATE3_W *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx3W( sesid, dbid, ptablecreate ) );
}

JET_ERR JET_API JetCreateTableColumnIndex4A(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    __inout JET_TABLECREATE4_A *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx4( sesid, dbid, ptablecreate ) );
}

LOCAL JET_ERR JetCreateTableColumnIndexEx4W(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
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
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    __inout JET_TABLECREATE4_W *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx4W( sesid, dbid, ptablecreate ) );
}

JET_ERR JET_API JetCreateTableColumnIndex5A(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    __inout JET_TABLECREATE5_A *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx( sesid, dbid, ptablecreate ) );
}

LOCAL JET_ERR JetCreateTableColumnIndexEx5W(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
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
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    __inout JET_TABLECREATE5_W *    ptablecreate )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCreateTableColumnIndex, JetCreateTableColumnIndexEx5W( sesid, dbid, ptablecreate ) );
}

LOCAL JET_ERR JetDeleteTableEx( _In_ JET_SESID sesid, _In_ JET_DBID dbid, _In_ JET_PCSTR szName, _In_ const JET_GRBIT grbit )
{
    APICALL_SESID   apicall( opDeleteTable );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%Ix,0x%x,0x%p[%s],0x%x)",
            __FUNCTION__,
            sesid,
            dbid,
            szName,
            OSFormatString( szName ),
            grbit ) );

    if ( apicall.FEnter( sesid, dbid ) )
    {
        apicall.LeaveAfterCall( ErrIsamDeleteTable(
                                        sesid,
                                        dbid,
                                        (CHAR *)szName,
                                        fFalse,
                                        grbit ) );
    }

    return apicall.ErrResult();
}

LOCAL JET_ERR JetDeleteTableExW( _In_ JET_SESID sesid, _In_ JET_DBID dbid, _In_ JET_PCWSTR wszName, _In_ const JET_GRBIT grbit )
{
    ERR         err             = JET_errSuccess;
    CAutoSZDDL  lszName;

    CallR( lszName.ErrSet( wszName ) );

    return JetDeleteTableEx( sesid, dbid, lszName, grbit );
}

JET_ERR JET_API JetDeleteTableA( _In_ JET_SESID sesid, _In_ JET_DBID dbid, _In_ JET_PCSTR szName )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDeleteTable, JetDeleteTableEx( sesid, dbid, szName, NO_GRBIT ) );
}

JET_ERR JET_API JetDeleteTableW( _In_ JET_SESID sesid, _In_ JET_DBID dbid, _In_ JET_PCWSTR wszName )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDeleteTable, JetDeleteTableExW( sesid, dbid, wszName, NO_GRBIT ) );
}

JET_ERR JET_API JetDeleteTable2A( _In_ JET_SESID sesid, _In_ JET_DBID dbid, _In_ JET_PCSTR szName, _In_ const JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDeleteTable, JetDeleteTableEx( sesid, dbid, szName, grbit ) );
}

JET_ERR JET_API JetDeleteTable2W( _In_ JET_SESID sesid, _In_ JET_DBID dbid, _In_ JET_PCWSTR wszName, _In_ const JET_GRBIT grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDeleteTable, JetDeleteTableExW( sesid, dbid, wszName, grbit ) );
}

LOCAL JET_ERR JetRenameTableEx(
    _In_ JET_SESID  sesid,
    _In_ JET_DBID   dbid,
    _In_ JET_PCSTR  szName,
    _In_ JET_PCSTR  szNameNew )
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
    _In_ JET_SESID  sesid,
    _In_ JET_DBID   dbid,
    _In_ JET_PCWSTR wszName,
    _In_ JET_PCWSTR wszNameNew )
{
    ERR         err;
    CAutoSZDDL  lszName;
    CAutoSZDDL  lszNameNew;

    CallR( lszName.ErrSet( wszName ) );
    CallR( lszNameNew.ErrSet( wszNameNew ) );

    return JetRenameTableEx( sesid, dbid, lszName, lszNameNew );
}

JET_ERR JET_API JetRenameTableA(
    _In_ JET_SESID  sesid,
    _In_ JET_DBID   dbid,
    _In_ JET_PCSTR  szName,
    _In_ JET_PCSTR  szNameNew )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opRenameTable, JetRenameTableEx( sesid, dbid, szName, szNameNew ) );
}

JET_ERR JET_API JetRenameTableW(
    _In_ JET_SESID  sesid,
    _In_ JET_DBID   dbid,
    _In_ JET_PCWSTR wszName,
    _In_ JET_PCWSTR wszNameNew )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opRenameTable, JetRenameTableExW( sesid, dbid, wszName, wszNameNew ) );
}


LOCAL JET_ERR JetRenameColumnEx(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCSTR      szName,
    _In_ JET_PCSTR      szNameNew,
    _In_ JET_GRBIT      grbit )
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
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCWSTR     wszName,
    _In_ JET_PCWSTR     wszNameNew,
    _In_ JET_GRBIT      grbit )
{

    ERR         err;
    CAutoSZDDL  lszName;
    CAutoSZDDL  lszNameNew;

    CallR( lszName.ErrSet( wszName ) );
    CallR( lszNameNew.ErrSet( wszNameNew ) );

    return JetRenameColumnEx( sesid, tableid, lszName, lszNameNew, grbit );
}

JET_ERR JET_API JetRenameColumnA(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCSTR      szName,
    _In_ JET_PCSTR      szNameNew,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opRenameColumn, JetRenameColumnEx( sesid, tableid, szName, szNameNew, grbit ) );
}

JET_ERR JET_API JetRenameColumnW(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCWSTR     wszName,
    _In_ JET_PCWSTR     wszNameNew,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opRenameColumn, JetRenameColumnExW( sesid, tableid, wszName, wszNameNew, grbit ) );
}


LOCAL JET_ERR JetAddColumnEx(
    _In_ JET_SESID                              sesid,
    _In_ JET_TABLEID                            tableid,
    _In_ JET_PCSTR                              szColumnName,
    _In_ const JET_COLUMNDEF *                  pcolumndef,
    __in_bcount_opt( cbDefault ) const void *   pvDefault,
    _In_ ULONG                          cbDefault,
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
    _In_ JET_SESID                              sesid,
    _In_ JET_TABLEID                            tableid,
    _In_ JET_PCWSTR                             wszColumnName,
    _In_ const JET_COLUMNDEF *                  pcolumndef,
    __in_bcount_opt( cbDefault ) const void *   pvDefault,
    _In_ ULONG                          cbDefault,
    __out_opt JET_COLUMNID *                    pcolumnid )
{
    ERR         err;
    CAutoSZDDL  lszColumnName;

    CallR( lszColumnName.ErrSet( wszColumnName ) );

    return JetAddColumnEx( sesid, tableid, lszColumnName, pcolumndef, pvDefault, cbDefault, pcolumnid );
}

JET_ERR JET_API JetAddColumnA(
    _In_ JET_SESID                              sesid,
    _In_ JET_TABLEID                            tableid,
    _In_ JET_PCSTR                              szColumnName,
    _In_ const JET_COLUMNDEF *                  pcolumndef,
    __in_bcount_opt( cbDefault ) const void *   pvDefault,
    _In_ ULONG                          cbDefault,
    __out_opt JET_COLUMNID *                    pcolumnid )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opAddColumn, JetAddColumnEx( sesid, tableid, szColumnName, pcolumndef, pvDefault, cbDefault, pcolumnid ) );
}

JET_ERR JET_API JetAddColumnW(
    _In_ JET_SESID                              sesid,
    _In_ JET_TABLEID                            tableid,
    _In_ JET_PCWSTR                             wszColumnName,
    _In_ const JET_COLUMNDEF *                  pcolumndef,
    __in_bcount_opt( cbDefault ) const void *   pvDefault,
    _In_ ULONG                          cbDefault,
    __out_opt JET_COLUMNID *                    pcolumnid )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opAddColumn, JetAddColumnExW( sesid, tableid, wszColumnName, pcolumndef, pvDefault, cbDefault, pcolumnid ) );
}


LOCAL JET_ERR JetDeleteColumnEx(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _In_ JET_PCSTR          szColumnName,
    _In_ const JET_GRBIT    grbit )
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
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _In_ JET_PCWSTR         wszColumnName,
    _In_ const JET_GRBIT    grbit )
{
    ERR         err             = JET_errSuccess;
    CAutoSZDDL  lszColumnName;

    CallR( lszColumnName.ErrSet( wszColumnName ) );

    return JetDeleteColumnEx( sesid, tableid, lszColumnName, grbit );
}

JET_ERR JET_API JetDeleteColumnA(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCSTR      szColumnName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDeleteColumn, JetDeleteColumnEx( sesid, tableid, szColumnName, NO_GRBIT ) );
}
JET_ERR JET_API JetDeleteColumnW(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCWSTR     wszColumnName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDeleteColumn, JetDeleteColumnExW( sesid, tableid, wszColumnName, NO_GRBIT ) );
}
JET_ERR JET_API JetDeleteColumn2A(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _In_ JET_PCSTR          szColumnName,
    _In_ const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDeleteColumn, JetDeleteColumnEx( sesid, tableid, szColumnName, grbit ) );
}
JET_ERR JET_API JetDeleteColumn2W(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _In_ JET_PCWSTR         wszColumnName,
    _In_ const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDeleteColumn, JetDeleteColumnExW( sesid, tableid, wszColumnName, grbit ) );
}

LOCAL JET_ERR JetSetColumnDefaultValueEx(
    _In_ JET_SESID                      sesid,
    _In_ JET_DBID                       dbid,
    _In_ JET_PCSTR                      szTableName,
    _In_ JET_PCSTR                      szColumnName,
    __in_bcount( cbData ) const void *  pvData,
    _In_ const ULONG            cbData,
    _In_ const JET_GRBIT                grbit )
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
    _In_ JET_SESID                      sesid,
    _In_ JET_DBID                       dbid,
    _In_ JET_PCWSTR                     wszTableName,
    _In_ JET_PCWSTR                     wszColumnName,
    __in_bcount( cbData ) const void *  pvData,
    _In_ const ULONG            cbData,
    _In_ const JET_GRBIT                grbit )
{
    ERR         err             = JET_errSuccess;
    CAutoSZDDL  lszTableName;
    CAutoSZDDL  lszColumnName;

    CallR( lszTableName.ErrSet( wszTableName ) );
    CallR( lszColumnName.ErrSet( wszColumnName ) );

    return JetSetColumnDefaultValueEx(sesid, dbid, lszTableName, lszColumnName, pvData, cbData, grbit);
}

JET_ERR JET_API JetSetColumnDefaultValueA(
    _In_ JET_SESID                      sesid,
    _In_ JET_DBID                       dbid,
    _In_ JET_PCSTR                      szTableName,
    _In_ JET_PCSTR                      szColumnName,
    __in_bcount( cbData ) const void *  pvData,
    _In_ const ULONG            cbData,
    _In_ const JET_GRBIT                grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opSetColumnDefaultValue, JetSetColumnDefaultValueEx( sesid, dbid, szTableName, szColumnName, pvData, cbData, grbit ) );
}

JET_ERR JET_API JetSetColumnDefaultValueW(
    _In_ JET_SESID                      sesid,
    _In_ JET_DBID                       dbid,
    _In_ JET_PCWSTR                     wszTableName,
    _In_ JET_PCWSTR                     wszColumnName,
    __in_bcount( cbData ) const void *  pvData,
    _In_ const ULONG            cbData,
    _In_ const JET_GRBIT                grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opSetColumnDefaultValue, JetSetColumnDefaultValueExW( sesid, dbid, wszTableName, wszColumnName, pvData, cbData, grbit ) );
}

LOCAL JET_ERR JetCreateIndexEx(
    _In_ JET_SESID                                   sesid,
    _In_ JET_TABLEID                                 tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE3_A * pindexcreate,
    _In_ ULONG                               cIndexCreate )
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
    _In_ JET_SESID                                   sesid,
    _In_ JET_TABLEID                                 tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE2_A * pindexcreate,
    _In_ ULONG                               cIndexCreate )
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
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE2_W *    pindexcreate,
    _In_ ULONG                              cIndexCreate )
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

        //  This is not technically guaranteed, but we check anyway.
        Assert( pidxCurr == ( pindexcreate + iIndexCreate ) );

        Call( rgindexcreateauto[iIndexCreate].ErrSet( pidxCurr ) );
        memcpy( rgindexcreateEngine + iIndexCreate, (JET_INDEXCREATE2_A*)(rgindexcreateauto[iIndexCreate]), sizeof( JET_INDEXCREATE2_A ) );
    }

    err = JetCreateIndexEx2( sesid, tableid, rgindexcreateEngine, cIndexCreate );

    // we need to fill back the err from each member of pindexcreate
    //
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
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE_A * pindexcreate,
    _In_ ULONG                              cIndexCreate )
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
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE_W * pindexcreate,
    _In_ ULONG                              cIndexCreate )
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

        //  This is not technically guaranteed, but we check anyway.
        Assert( pidxCurr == ( pindexcreate + iIndexCreate ) );

        Call( rgindexcreateauto[iIndexCreate].ErrSet( pidxCurr ) );
        memcpy( rgindexcreateEngine + iIndexCreate, (JET_INDEXCREATE2_A*)(rgindexcreateauto[iIndexCreate]), sizeof( JET_INDEXCREATE2_A ) );
    }

    err = JetCreateIndexEx2( sesid, tableid, rgindexcreateEngine, cIndexCreate );

    // we need to fill back the err from each member of pindexcreate
    //
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
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE3_W *pindexcreate,
    _In_ ULONG                              cIndexCreate )
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

        //  This is not technically guaranteed, but we check anyway.
        Assert( pidxCurr == ( pindexcreate + iIndexCreate ) );

        Assert( pidxCurr->cbStruct == sizeof( JET_INDEXCREATE3_W ) );

        Call( rgindexcreateauto[iIndexCreate].ErrSet( pidxCurr ) );
        memcpy( rgindexcreateEngine + iIndexCreate, (JET_INDEXCREATE3_A*)(rgindexcreateauto[iIndexCreate]), sizeof( JET_INDEXCREATE3_A ) );
    }

    err = JetCreateIndexEx( sesid, tableid, rgindexcreateEngine, cIndexCreate );

    // we need to fill back the err from each member of pindexcreate
    //
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
    _In_ JET_SESID                      sesid,
    _In_ JET_TABLEID                    tableid,
    _In_ JET_PCSTR                      szIndexName,
    _In_ JET_GRBIT                      grbit,
    __in_bcount( cbKey ) const char *   szKey,
    _In_ ULONG                  cbKey,
    _In_ ULONG                  lDensity )
{
    JET_INDEXCREATE2_A  idxcreate;
    JET_SPACEHINTS      idxjsh;

    memset( &idxcreate, 0, sizeof(idxcreate) );
    idxcreate.cbStruct      = sizeof( idxcreate );
    idxcreate.szIndexName   = (CHAR *)szIndexName;
    idxcreate.szKey         = (CHAR *)szKey;
    idxcreate.cbKey         = cbKey;
    idxcreate.grbit         = grbit;
    idxcreate.ulDensity     = 0;    // moved to space hints for better coverage.
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
    _In_ JET_SESID                      sesid,
    _In_ JET_TABLEID                    tableid,
    _In_ JET_PCWSTR                     wszIndexName,
    _In_ JET_GRBIT                      grbit,
    __in_bcount( cbKey ) const WCHAR *  wszKey,
    _In_ ULONG                  cbKey,
    _In_ ULONG                  lDensity )
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
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE_A * pindexcreate,
    _In_ ULONG                              cIndexCreate )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx1( sesid, tableid, pindexcreate, cIndexCreate ) );
}

JET_ERR JET_API JetCreateIndex2W(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE_W * pindexcreate,
    _In_ ULONG                              cIndexCreate )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx1W( sesid, tableid, pindexcreate, cIndexCreate ) );
}

JET_ERR JET_API JetCreateIndex3A(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE2_A *pindexcreate,
    _In_ ULONG                              cIndexCreate )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx2( sesid, tableid, pindexcreate, cIndexCreate ) );
}

JET_ERR JET_API JetCreateIndex3W(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE2_W *pindexcreate,
    _In_ ULONG                              cIndexCreate )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx2W( sesid, tableid, pindexcreate, cIndexCreate ) );
}


JET_ERR JET_API JetCreateIndex4A(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE3_A *pindexcreate,
    _In_ ULONG                              cIndexCreate )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx( sesid, tableid, pindexcreate, cIndexCreate ) );
}

JET_ERR JET_API JetCreateIndex4W(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    __in_ecount( cIndexCreate ) JET_INDEXCREATE3_W *pindexcreate,
    _In_ ULONG                              cIndexCreate )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opCreateIndex, JetCreateIndexEx3W( sesid, tableid, pindexcreate, cIndexCreate ) );
}


LOCAL JET_ERR JetDeleteIndexEx(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _In_ JET_PCSTR          szIndexName )
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
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _In_ JET_PCWSTR         wszIndexName )
{
    ERR         err;
    CAutoSZDDL  lszIndexName;

    CallR( lszIndexName.ErrSet( wszIndexName ) );

    return JetDeleteIndexEx( sesid, tableid, lszIndexName );
}

JET_ERR JET_API JetDeleteIndexA(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCSTR      szIndexName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDeleteIndex, JetDeleteIndexEx( sesid, tableid, szIndexName ) );
}


JET_ERR JET_API JetDeleteIndexW(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCWSTR     wszIndexName )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opDeleteIndex, JetDeleteIndexExW( sesid, tableid, wszIndexName ) );
}

LOCAL JET_ERR JetComputeStatsEx( _In_ JET_SESID sesid, _In_ JET_TABLEID tableid )
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

JET_ERR JET_API JetComputeStats( _In_ JET_SESID sesid, _In_ JET_TABLEID tableid )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opComputeStats, JetComputeStatsEx( sesid, tableid ) );
}


LOCAL JET_ERR JetAttachDatabaseEx(
    _In_ JET_SESID                                              sesid,
    _In_ JET_PCWSTR                                             wszDbFileName,
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
    _In_ JET_SESID                                              sesid,
    _In_ JET_PCSTR                                              szDbFileName,
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
    _In_ JET_SESID  sesid,
    _In_ JET_PCSTR  szFilename,
    _In_ JET_GRBIT  grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opAttachDatabase, JetAttachDatabaseExA( sesid, szFilename, NULL, 0, grbit ) );
}

JET_ERR JET_API JetAttachDatabaseW(
    _In_ JET_SESID  sesid,
    _In_ JET_PCWSTR wszFilename,
    _In_ JET_GRBIT  grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opAttachDatabase, JetAttachDatabaseEx( sesid, wszFilename, NULL, 0, grbit ) );
}

JET_ERR JET_API JetAttachDatabase2A(
    _In_ JET_SESID              sesid,
    _In_ JET_PCSTR              szFilename,
    _In_ const ULONG    cpgDatabaseSizeMax,
    _In_ JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    const JET_SETDBPARAM rgsetdbparam[] =
    {
            { JET_dbparamDbSizeMaxPages, (void*)&cpgDatabaseSizeMax, sizeof( cpgDatabaseSizeMax ) }
    };
    JET_TRY( opAttachDatabase, JetAttachDatabaseExA( sesid, szFilename, rgsetdbparam, _countof( rgsetdbparam ), grbit ) );
}

JET_ERR JET_API JetAttachDatabase2W(
    _In_ JET_SESID              sesid,
    _In_ JET_PCWSTR             wszFilename,
    _In_ const ULONG    cpgDatabaseSizeMax,
    _In_ JET_GRBIT              grbit )
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
    _In_ JET_SESID              sesid,
    _In_ JET_PCSTR              szDbFileName,
    _Reserved_ JET_PCSTR        szSLVFileName,
    _Reserved_ JET_PCSTR        szSLVRootName,
    _In_ const ULONG    cpgDatabaseSizeMax,
    _In_ JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    return ErrERRCheck( JET_errFeatureNotAvailable );
}

JET_ERR JET_API JetAttachDatabaseWithStreamingW(
    _In_ JET_SESID              sesid,
    _In_ JET_PCWSTR             wszDbFileName,
    _Reserved_ JET_PCWSTR       wszSLVFileName,
    _Reserved_ JET_PCWSTR       wszSLVRootName,
    _In_ const ULONG    cpgDatabaseSizeMax,
    _In_ JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    return ErrERRCheck( JET_errFeatureNotAvailable );
}

LOCAL JET_ERR JetDetachDatabaseEx(
    _In_ JET_SESID  sesid,
    __in_opt JET_PCWSTR wszFilename,
    _In_ JET_GRBIT  grbit)
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
    _In_ JET_SESID  sesid,
    __in_opt JET_PCSTR  szFilename,
    _In_ JET_GRBIT  grbit)
{
    ERR             err;
    CAutoWSZPATH    lwszFilename;

    CallR( lwszFilename.ErrSet( szFilename ) );

    return JetDetachDatabaseEx( sesid, lwszFilename, grbit );
}

JET_ERR JET_API JetDetachDatabaseA(
    _In_ JET_SESID  sesid,
    __in_opt JET_PCSTR  szFilename )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDetachDatabase, JetDetachDatabaseExA( sesid, szFilename, NO_GRBIT ) );
}

JET_ERR JET_API JetDetachDatabaseW(
    _In_ JET_SESID  sesid,
    __in_opt JET_PCWSTR wszFilename )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDetachDatabase, JetDetachDatabaseEx( sesid, wszFilename, NO_GRBIT ) );
}

JET_ERR JET_API JetDetachDatabase2A(
    _In_ JET_SESID  sesid,
    __in_opt JET_PCSTR  szFilename,
    _In_ JET_GRBIT  grbit)
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDetachDatabase, JetDetachDatabaseExA( sesid, szFilename, grbit ) );
}

JET_ERR JET_API JetDetachDatabase2W(
    _In_ JET_SESID  sesid,
    __in_opt JET_PCWSTR wszFilename,
    _In_ JET_GRBIT  grbit)
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDetachDatabase, JetDetachDatabaseEx( sesid, wszFilename, grbit ) );
}


LOCAL JET_ERR JetBackupInstanceEx(
    JET_INSTANCE    instance,
    _In_ PCWSTR     wszBackupPath,
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
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCSTR      szBackupPath,
    _In_ JET_GRBIT      grbit,
    __in_opt JET_PFNSTATUS  pfnStatus )
{
    ERR             err;
    CAutoWSZPATH    lwszBackupPath;

    CallR( lwszBackupPath.ErrSet( szBackupPath ) );

    return JetBackupInstanceEx( instance, lwszBackupPath, grbit, pfnStatus );
}

JET_ERR JET_API JetBackupInstanceA(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCSTR      szBackupPath,
    _In_ JET_GRBIT      grbit,
    __in_opt JET_PFNSTATUS  pfnStatus )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBackupInstance, JetBackupInstanceExA( instance, szBackupPath, grbit, pfnStatus ) );
}

JET_ERR JET_API JetBackupInstanceW(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCWSTR     wszBackupPath,
    _In_ JET_GRBIT      grbit,
    __in_opt JET_PFNSTATUS  pfnStatus )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBackupInstance, JetBackupInstanceEx( instance, wszBackupPath, grbit, pfnStatus ) );
}

JET_ERR JET_API JetBackupA(
    _In_ JET_PCSTR      szBackupPath,
    _In_ JET_GRBIT      grbit,
    __in_opt JET_PFNSTATUS  pfnStatus )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetBackupInstanceA( (JET_INSTANCE)g_rgpinst[0], szBackupPath, grbit, pfnStatus );
}

JET_ERR JET_API JetBackupW(
    _In_ JET_PCWSTR     wszBackupPath,
    _In_ JET_GRBIT      grbit,
    __in_opt JET_PFNSTATUS  pfnStatus )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetBackupInstanceW( (JET_INSTANCE)g_rgpinst[0], wszBackupPath, grbit, pfnStatus );
}

JET_ERR JET_API JetRestoreA(    _In_ JET_PCSTR szSource, __in_opt JET_PFNSTATUS pfn )
{
    ERR     err;

    OnDebug( const BOOL fInitd = ( RUNINSTGetMode() != runInstModeNoSet ) );

    CallR( ErrRUNINSTCheckAndSetOneInstMode() );

    Assert( fInitd == ( g_rgpinst != NULL ) );

    err = JetRestoreInstanceA( g_rgpinst ? (JET_INSTANCE)g_rgpinst[0] : NULL, szSource, NULL, pfn );

    //  I am not sure this holds ...
    Assert( fInitd == ( RUNINSTGetMode() != runInstModeNoSet ) );
    //  Definitely Either we were previously init'd/mode'd, or we should have left ourselves in a pre-mode state.
    Assert( fInitd || ( RUNINSTGetMode() == runInstModeNoSet ) );

    return err;
}
JET_ERR JET_API JetRestoreW(    _In_ JET_PCWSTR wszSource, __in_opt JET_PFNSTATUS pfn )
{
    ERR     err;

    OnDebug( const BOOL fInitd = ( RUNINSTGetMode() != runInstModeNoSet ) );

    CallR( ErrRUNINSTCheckAndSetOneInstMode() );

    Assert( fInitd == ( g_rgpinst != NULL ) );

    err = JetRestoreInstanceW( g_rgpinst ? (JET_INSTANCE)g_rgpinst[0] : NULL, wszSource, NULL, pfn );

    //  I am not sure this holds ...
    Assert( fInitd == ( RUNINSTGetMode() != runInstModeNoSet ) );
    //  Definitely Either we were previously init'd/mode'd, or we should have left ourselves in a pre-mode state.
    Assert( fInitd || ( RUNINSTGetMode() == runInstModeNoSet ) );

    return err;
}

JET_ERR JET_API JetRestore2A( _In_ JET_PCSTR sz, __in_opt JET_PCSTR szDest, __in_opt JET_PFNSTATUS pfn )
{
    ERR     err;

    OnDebug( const BOOL fInitd = ( RUNINSTGetMode() != runInstModeNoSet ) );

    CallR( ErrRUNINSTCheckAndSetOneInstMode() );

    Assert( fInitd == ( g_rgpinst != NULL ) );

    err = JetRestoreInstanceA( g_rgpinst ? (JET_INSTANCE)g_rgpinst[0] : NULL, sz, szDest, pfn );

    //  I am not sure this holds ...
    Assert( fInitd == ( RUNINSTGetMode() != runInstModeNoSet ) );
    //  Definitely Either we were previously init'd/mode'd, or we should have left ourselves in a pre-mode state.
    Assert( fInitd || ( RUNINSTGetMode() == runInstModeNoSet ) );

    return err;
}

JET_ERR JET_API JetRestore2W(_In_ JET_PCWSTR wsz, __in_opt JET_PCWSTR wszDest, __in_opt JET_PFNSTATUS pfn )
{
    ERR     err;

    OnDebug( const BOOL fInitd = ( RUNINSTGetMode() != runInstModeNoSet ) );

    CallR( ErrRUNINSTCheckAndSetOneInstMode() );

    Assert( fInitd == ( g_rgpinst != NULL ) );

    err = JetRestoreInstanceW( g_rgpinst ? (JET_INSTANCE)g_rgpinst[0] : NULL, wsz, wszDest, pfn );

    //  I am not sure this holds ...
    Assert( fInitd == ( RUNINSTGetMode() != runInstModeNoSet ) );
    //  Definitely Either we were previously init'd/mode'd, or we should have left ourselves in a pre-mode state.
    Assert( fInitd || ( RUNINSTGetMode() == runInstModeNoSet ) );

    return err;
}


LOCAL JET_ERR JetOpenTempTableEx(
    _In_ JET_SESID                                  sesid,
    __in_ecount( ccolumn ) const JET_COLUMNDEF *    prgcolumndef,
    _In_ ULONG                              ccolumn,
    __in_opt JET_UNICODEINDEX2 *                    pidxunicode,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid,
    __out_ecount( ccolumn ) JET_COLUMNID *          prgcolumnid,
    _In_ ULONG                              cbKeyMost       = 0,
    _In_ ULONG                              cbVarSegMax     = 0 )
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
    _In_ JET_SESID                                  sesid,
    __in_ecount( ccolumn ) const JET_COLUMNDEF *    prgcolumndef,
    _In_ ULONG                              ccolumn,
    __in_opt JET_UNICODEINDEX *                     pidxunicode,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid,
    __out_ecount( ccolumn ) JET_COLUMNID *          prgcolumnid,
    _In_ ULONG                              cbKeyMost       = 0,
    _In_ ULONG                              cbVarSegMax     = 0 )
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
    _In_ JET_SESID                                  sesid,
    __in_ecount( ccolumn ) const JET_COLUMNDEF *    prgcolumndef,
    _In_ ULONG                              ccolumn,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid,
    __out_ecount( ccolumn ) JET_COLUMNID *          prgcolumnid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOpenTempTable, JetOpenTempTableEx1( sesid, prgcolumndef, ccolumn, NULL, grbit, ptableid, prgcolumnid ) );
}
JET_ERR JET_API JetOpenTempTable2(
    _In_ JET_SESID                                  sesid,
    __in_ecount( ccolumn ) const JET_COLUMNDEF *    prgcolumndef,
    _In_ ULONG                              ccolumn,
    _In_ ULONG                              lcid,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid,
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
    _In_ JET_SESID                                  sesid,
    __in_ecount( ccolumn ) const JET_COLUMNDEF *    prgcolumndef,
    _In_ ULONG                              ccolumn,
    __in_opt JET_UNICODEINDEX *                     pidxunicode,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid,
    __out_ecount( ccolumn ) JET_COLUMNID *          prgcolumnid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opOpenTempTable, JetOpenTempTableEx1( sesid, prgcolumndef, ccolumn, pidxunicode, grbit, ptableid, prgcolumnid ) );
}

JET_ERR JET_API JetOpenTemporaryTable(
    _In_ JET_SESID                  sesid,
    _In_ JET_OPENTEMPORARYTABLE *   popentemporarytable )
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
    _In_ JET_SESID                  sesid,
    _In_ JET_OPENTEMPORARYTABLE2 *   popentemporarytable )
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
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_GRBIT      grbit )
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
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opSetIndexRange, JetSetIndexRangeEx( sesid, tableid, grbit ) );
}

LOCAL JET_ERR JetIndexRecordCountEx(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _Out_ ULONG64 *         pcrec,
    _In_ ULONG64            crecMax )
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
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _Out_ ULONG *           pcrec,
    _In_ ULONG              crecMax )
{
    ERR err = JET_errSuccess;
    ULONG64 crec = 0;

    ProbeClientBuffer( pcrec, sizeof( ULONG ) );
    *pcrec = 0;

    err = JetIndexRecordCountEx( sesid, tableid, &crec, (ULONG64) crecMax );

    if ( crec > ulMax )
    {
        // There are too many records, user needs
        // to switch to JetIndexRecordCount2
        err = ErrERRCheck( JET_errTooManyRecords );
    }
    else
    {
        *pcrec = (ULONG) crec;
    }
    
    return err;
}

JET_ERR JET_API JetIndexRecordCount(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _Out_ ULONG *   pcrec,
    _In_ ULONG      crecMax )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opIndexRecordCount, JetIndexRecordCountEx32Bit( sesid, tableid, pcrec, crecMax ) );
}

JET_ERR JET_API JetIndexRecordCount2(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _Out_ ULONG64 *         pcrec,
    _In_ ULONG64            crecMax )
{
    JET_VALIDATE_SESID_TABLEID( sesid, tableid );
    JET_TRY( opIndexRecordCount, JetIndexRecordCountEx( sesid, tableid, pcrec, crecMax ) );
}

/***********************************************************
/************* EXTERNAL BACKUP JET API *********************
/*****/
LOCAL JET_ERR JetBeginExternalBackupInstanceEx( _In_ JET_INSTANCE instance, _In_ JET_GRBIT grbit )
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
        // if this backup is opened by an internal client, it's allowed to backup from a recovering copy (aka seed from passive).
        // 
        const BOOL fInternalCopyBackup = ( JET_bitInternalCopy & grbit ) != 0;
        apicall.LeaveAfterCall( apicall.Pinst()->m_fBackupAllowed || ( apicall.Pinst()->FRecovering() && fInternalCopyBackup ) ?
                                        ErrIsamBeginExternalBackup( (JET_INSTANCE)apicall.Pinst() , grbit ) :
                                        ErrERRCheck( JET_errBackupNotAllowedYet ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetBeginExternalBackupInstance( _In_ JET_INSTANCE instance, _In_ JET_GRBIT grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBeginExternalBackupInstance, JetBeginExternalBackupInstanceEx( instance, grbit ) );
}
JET_ERR JET_API JetBeginExternalBackup( _In_ JET_GRBIT grbit )
{
    ERR     err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetBeginExternalBackupInstance( (JET_INSTANCE)g_rgpinst[0], grbit );
}


LOCAL JET_ERR JetGetAttachInfoInstanceEx(
    _In_ JET_INSTANCE                                       instance,
    __out_bcount_part_opt( cbMax, *pcbActual ) JET_PWSTR    wszzDatabases,
    _In_ ULONG                                      cbMax,
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
    _In_ JET_INSTANCE                                   instance,
    __out_bcount_part_opt( cbMax, *pcbActual ) JET_PSTR szzDatabases,
    _In_ ULONG                                  cbMax,
    __out_opt ULONG *                           pcbActual )
{
    ERR             err         = JET_errSuccess;
    ULONG   cbActual;
    CAutoWSZ        lwszz;
    size_t  cchActual;

    Call( JetGetAttachInfoInstanceEx( instance, 0, NULL, &cbActual ) );

    Call( lwszz.ErrAlloc( cbActual ));

    Call( JetGetAttachInfoInstanceEx( instance, lwszz.Pv(), lwszz.Cb(), &cbActual ) );

    // at this point we have the ascii version, we need to convert to Unicode
    // using the existing provided buffer and maybe just to tell the real size needed
    //
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
    _In_ JET_INSTANCE       instance,
    _In_ JET_PCWSTR         wszFileName,
    _Out_ JET_HANDLE *      phfFile,
    _In_ ULONG64            ibRead,
    _Out_ ULONG *   pulFileSizeLow,
    _Out_ ULONG *   pulFileSizeHigh )
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
    _In_ JET_INSTANCE       instance,
    _In_ JET_PCSTR          szFileName,
    _Out_ JET_HANDLE *      phfFile,
    _In_ ULONG64            ibRead,
    _Out_ ULONG *   pulFileSizeLow,
    _Out_ ULONG *   pulFileSizeHigh )
{
    ERR             err         = JET_errSuccess;
    CAutoWSZPATH    lwszFileName;

    CallR( lwszFileName.ErrSet( szFileName ) );

    return JetOpenFileInstanceEx( instance, lwszFileName, phfFile, ibRead, pulFileSizeLow, pulFileSizeHigh );
}

JET_ERR JET_API JetOpenFileInstanceA(
    _In_ JET_INSTANCE       instance,
    _In_ JET_PCSTR          szFileName,
    _Out_ JET_HANDLE *      phfFile,
    _Out_ ULONG *   pulFileSizeLow,
    _Out_ ULONG *   pulFileSizeHigh )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opOpenFileInstance, JetOpenFileInstanceExA( instance, szFileName, phfFile, 0, pulFileSizeLow, pulFileSizeHigh ) );
}

JET_ERR JET_API JetOpenFileInstanceW(
    _In_ JET_INSTANCE       instance,
    _In_ JET_PCWSTR         wszFileName,
    _Out_ JET_HANDLE *      phfFile,
    _Out_ ULONG *   pulFileSizeLow,
    _Out_ ULONG *   pulFileSizeHigh )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opOpenFileInstance, JetOpenFileInstanceEx( instance, wszFileName, phfFile, 0, pulFileSizeLow, pulFileSizeHigh ) );
}

LOCAL JET_ERR JET_API JetOpenFileSectionInstanceExA(
    _In_ JET_INSTANCE       instance,
    _In_ JET_PCSTR          szFileName,
    _Out_ JET_HANDLE *      phfFile,
    _In_ ULONG64            ibRead,
    _Out_ ULONG *   pulFileSizeLow,
    _Out_ ULONG *   pulFileSizeHigh )
{
    ERR             err = JET_errSuccess;
    CAutoWSZPATH    lwszFileName;

    CallR( lwszFileName.ErrSet( szFileName ) );

    return JetOpenFileInstanceEx( instance, lwszFileName, phfFile, ibRead, pulFileSizeLow, pulFileSizeHigh );
}

JET_ERR JET_API JetOpenFileSectionInstanceA(
    _In_ JET_INSTANCE       instance,
    _In_ JET_PSTR           szFileName,
    _Out_ JET_HANDLE *      phFile,
    _In_ LONG               iSection,
    _In_ LONG               cSections,
    _In_ ULONG64            ibRead,
    _Out_ ULONG *   pulSectionSizeLow,
    _Out_ LONG *            plSectionSizeHigh )
{
    if ( cSections > 1 )
    {
        return ErrERRCheck( JET_wrnNyi );
    }

    JET_VALIDATE_INSTANCE( instance );

    JET_TRY( opFileSectionInstance, JetOpenFileSectionInstanceExA( instance, szFileName, phFile, ibRead, pulSectionSizeLow, (ULONG *)plSectionSizeHigh ) );
};

JET_ERR JET_API JetOpenFileSectionInstanceW(
    _In_ JET_INSTANCE       instance,
    _In_ JET_PWSTR          wszFileName,
    _Out_ JET_HANDLE *      phFile,
    _In_ LONG               iSection,
    _In_ LONG               cSections,
    _In_ ULONG64            ibRead,
    _Out_ ULONG *   pulSectionSizeLow,
    _Out_ LONG *            plSectionSizeHigh )
{

    if ( cSections > 1 )
    {
        return ErrERRCheck( JET_wrnNyi );
    }
    
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opFileSectionInstance, JetOpenFileInstanceEx( instance, wszFileName, phFile, ibRead, pulSectionSizeLow, (ULONG *)plSectionSizeHigh ) );
};


JET_ERR JET_API JetOpenFileA(
    _In_ JET_PCSTR          szFileName,
    _Out_ JET_HANDLE *      phfFile,
    _Out_ ULONG *   pulFileSizeLow,
    _Out_ ULONG *   pulFileSizeHigh )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetOpenFileInstanceA( (JET_INSTANCE)g_rgpinst[0], szFileName, phfFile, pulFileSizeLow, pulFileSizeHigh );
}

JET_ERR JET_API JetOpenFileW(
    _In_ JET_PCWSTR         wszFileName,
    _Out_ JET_HANDLE *      phfFile,
    _Out_ ULONG *   pulFileSizeLow,
    _Out_ ULONG *   pulFileSizeHigh )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetOpenFileInstanceW( (JET_INSTANCE)g_rgpinst[0], wszFileName, phfFile, pulFileSizeLow, pulFileSizeHigh );
}


LOCAL JET_ERR JetReadFileInstanceEx(
    _In_ JET_INSTANCE                           instance,
    _In_ JET_HANDLE                             hfFile,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    _In_ ULONG                          cb,
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
    _In_ JET_INSTANCE                           instance,
    _In_ JET_HANDLE                             hfFile,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    _In_ ULONG                          cb,
    __out_opt ULONG *                   pcbActual )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opReadFileInstance, JetReadFileInstanceEx( instance, hfFile, pv, cb, pcbActual ) );
}
JET_ERR JET_API JetReadFile(
    _In_ JET_HANDLE                             hfFile,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    _In_ ULONG                          cb,
    __out_opt ULONG *                   pcbActual )
{
    ERR             err;

    CallR( ErrRUNINSTCheckOneInstMode() );

    return JetReadFileInstance( (JET_INSTANCE)g_rgpinst[0], hfFile, pv, cb, pcbActual );
}

LOCAL JET_ERR JetCloseFileInstanceEx( _In_ JET_INSTANCE instance, _In_ JET_HANDLE hfFile )
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
JET_ERR JET_API JetCloseFileInstance( _In_ JET_INSTANCE instance, _In_ JET_HANDLE hfFile )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opCloseFileInstance, JetCloseFileInstanceEx( instance, hfFile ) );
}
JET_ERR JET_API JetCloseFile( _In_ JET_HANDLE hfFile )
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
    _In_ JET_INSTANCE                                       instance,
    __out_bcount_part_opt( cbMax, *pcbActual ) JET_PWSTR    wszzLogs,
    _In_ ULONG                                      cbMax,
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
    _In_ JET_INSTANCE                                   instance,
    __out_bcount_part_opt( cbMax, *pcbActual ) JET_PSTR szzLogs,
    _In_ ULONG                                  cbMax,
    __out_opt ULONG *                           pcbActual,
    __inout_opt JET_LOGINFO_A *                         pLogInfo )
{
    ERR             err         = JET_errSuccess;
    CAutoWSZ        lwsz;
    CAutoLOGINFOW   lLogInfoW; // by default acts as NULL, w/o calling ErrSet().
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

    // at this point we have the ascii version, we need to convert to Unicode
    // using the existing provided buffer and maybe just to tell the real size needed
    //
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
    _In_ JET_INSTANCE                                       instance,
    __out_bcount_part_opt( cbMax, *pcbActual ) JET_PWSTR    wszzLogs,
    _In_ ULONG                                      cbMax,
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
    _In_ JET_INSTANCE                                   instance,
    __out_bcount_part_opt( cbMax, *pcbActual ) JET_PSTR szzLogs,
    _In_ ULONG                                  cbMax,
    __out_opt ULONG *                           pcbActual )
{
    ERR             err         = JET_errSuccess;
    CAutoWSZ        lwszz;
    ULONG   cbActual;
    size_t      cchActual;

    Call( JetGetTruncateLogInfoInstanceEx( instance, 0, NULL, &cbActual ) );

    Call( lwszz.ErrAlloc( cbActual ) );

    Call( JetGetTruncateLogInfoInstanceEx( instance, lwszz.Pv(), lwszz.Cb(), &cbActual ) );

    // at this point we have the ascii version, we need to convert to Unicode
    // using the existing provided buffer and maybe just to tell the real size needed
    //
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

LOCAL JET_ERR JetTruncateLogInstanceEx( _In_ JET_INSTANCE instance )
{
    APICALL_INST    apicall( opTruncateLogInstance );

    OSTrace( JET_tracetagAPI, OSFormat( "Start %s(0x%Ix)", __FUNCTION__, instance ) );

    if ( apicall.FEnter( instance ) )
    {
        apicall.LeaveAfterCall( ErrIsamTruncateLog( (JET_INSTANCE)apicall.Pinst() ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetTruncateLogInstance( _In_ JET_INSTANCE instance )
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


LOCAL JET_ERR JetEndExternalBackupInstanceEx( _In_ JET_INSTANCE instance, _In_ JET_GRBIT grbit )
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

JET_ERR JET_API JetEndExternalBackupInstance2( _In_ JET_INSTANCE instance, _In_ JET_GRBIT grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opEndExternalBackupInstance, JetEndExternalBackupInstanceEx( instance, grbit ) );
}


JET_ERR JET_API JetEndExternalBackupInstance( _In_ JET_INSTANCE instance )
{
    // we had no flag to specify from the client side how the backup ended
    //and we assumed success until now.
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
    _In_    JET_INSTANCE    instance,
    _In_    ULONG   lgenFirst,
    _In_    ULONG   lgenLast,
    _In_    JET_GRBIT   grbit )
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
    _In_        JET_INSTANCE    instance,
    _In_        ULONG       lgenFirst,
    _In_        ULONG       lgenLast,
    _In_        JET_GRBIT       grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opBeginSurrogateBackup, JetBeginSurrogateBackupEx( instance, lgenFirst, lgenLast, grbit ) );
}

LOCAL JET_ERR JetEndSurrogateBackupEx(
    _In_    JET_INSTANCE    instance,
    _In_        JET_GRBIT       grbit )
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
    _In_    JET_INSTANCE    instance,
    _In_        JET_GRBIT       grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opEndSurrogateBackup, JetEndSurrogateBackupEx( instance, grbit ) );
}


volatile DWORD g_cRestoreInstance = 0;


ERR ErrINSTPrepareTargetInstance(
    INST * pinstTarget,
    __out_bcount(cbTargetInstanceLogPath) PWSTR wszTargetInstanceLogPath,
    __in_range(sizeof(WCHAR), cbOSFSAPI_MAX_PATHW) ULONG    cbTargetInstanceLogPath,
    _In_ PCWSTR wszRestoreLogBaseName,
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

    // if we found an instance, call APIEnter so that it can't get away until we are done with it
    // also this will check if the found instance is initalized and if it is restoring

    // if the target instance is restoring, error out at this point it will be a problem to force a new
    // generation in order to don't conflict on log files. They have to just try later

    CallR ( pinstTarget->ErrAPIEnter( fFalse ) );

    Assert ( pinstTarget );
    Assert ( pinstTarget->m_fJetInitialized );
    Assert ( pinstTarget->m_wszInstanceName );


    // if not base name is specified for the restore instance (wszRestoreLogBaseName)
    // then the instance will use the default global one (szBaseName)
    WCHAR wszDefaultBaseName[ 16 ];
    Call( ErrGetSystemParameter( NULL, JET_sesidNil, JET_paramBaseName, NULL, wszDefaultBaseName, sizeof( wszDefaultBaseName ) ) );
    const WCHAR * wszCurrentLogBaseName = wszRestoreLogBaseName?wszRestoreLogBaseName:wszDefaultBaseName;
    if ( 0 != _wcsnicmp( SzParam( pinstTarget, JET_paramBaseName ), wszCurrentLogBaseName, JET_BASE_NAME_LENGTH) )
    {
        Error( ErrERRCheck( JET_errBadRestoreTargetInstance ) );
    }

    // with circular logging we don't try to play forward logs, they are probably missing anyway
    // and we don't want to error out because of that
    if ( BoolParam( pinstTarget, JET_paramCircularLog ) )
    {
        // set as "no target instance"
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
    // can't call APILeave() because we are in a critical section
{
    const LONG  lOld    = AtomicExchangeAdd( &(pinstTarget->m_cSessionInJetAPI), -1 );
    Assert( lOld >= 1 );
}

    return err;
}

// this cristical section is entered then initalizing a restore instance
// We need it because we have 2 steps: check the target instance,
// then based on that complete the init step (after setting the log+system path)
// If we are doing it in a critical section, we may end with 2 restore instances
// that are finding no running instance and trying to start in the instance directory
// One will error out with LogPath in use instead of "Restore in Progress"
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
    _In_ JET_PWSTR                                  wszCheckpointFilePath,
    _In_ JET_PWSTR                                  wszLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_W *   rgrstmap,
    _In_ LONG                                       crstfilemap,
    _In_ JET_PWSTR                                  wszBackupLogPath,
    _In_ LONG                                       genLow,
    _In_ LONG                                       genHigh,
    _In_ JET_PWSTR                                  wszLogBaseName,
    __in_opt JET_PCWSTR                             wszTargetInstanceName,
    __in_opt JET_PCWSTR                             wszTargetInstanceCheckpointPath,
    __in_opt JET_PCWSTR                             wszTargetInstanceLogPath,
    _In_ JET_PFNSTATUS                              pfn )
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

    // used for unique TemDatabase
    WCHAR           wszTempDatabase[IFileSystemAPI::cchPathMax];

    WCHAR * wszNewInstanceName = NULL;
    BOOL fInCritInst = fFalse;
    BOOL fInCritRestoreInst = fFalse;

    CCriticalSection *pcritInst = NULL;

    WCHAR * wszTargetDisplayName = NULL;
    ULONG cchTargetDisplayName = 0;
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

    // Target Dirs may be both present or both NULL
    if ( (wszTargetInstanceLogPath && !wszTargetInstanceCheckpointPath ) ||
        ( !wszTargetInstanceLogPath && wszTargetInstanceCheckpointPath ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    // can have TarrgetName AND TargetDirs
    if ( wszTargetInstanceName && wszTargetInstanceLogPath )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    //  Get a new instance

    g_critRestoreInst.Enter();
    fInCritRestoreInst = fTrue;

    INST::EnterCritInst();
    fInCritInst = fTrue;

    // not allowed in one instance mode
    if ( runInstModeOneInst == RUNINSTGetMode() )
    {
        Error( ErrERRCheck( JET_errRunningInOneInstanceMode ) );
    }
    // force to multi instance mode as we want multiple restore
    else if ( runInstModeNoSet == RUNINSTGetMode() )
    {
        RUNINSTSetModeMultiInst();
    }

    Assert ( runInstModeMultiInst == RUNINSTGetMode() );

    // we want to run the restore instance using wszCheckpointFilePath/wszLogPath
    // the Target Instance is running or not provided. If provided but not running,
    // we want to run in the Target instance place (wszTargetInstanceCheckpointPath/wszTargetInstanceLogPath)


    Assert ( !fTargetDirs || !fTargetName );

    if ( !fTargetDirs && !fTargetName )
    {
        wszRestoreSystemPath = wszCheckpointFilePath;
        wszRestoreLogPath = wszLogPath;
    }
    else
    {
        // Target specified by Name XOR Dirs
        Assert ( fTargetDirs ^ fTargetName );

        if ( fTargetName )
        {
            pinstTarget = INST::GetInstanceByName( wszTargetInstanceName );
        }
        else
        {
            pinstTarget = INST::GetInstanceByFullLogPath( wszTargetInstanceLogPath );
        }

        Assert( pinstTarget == NULL || g_rgpinst != NULL ); //  if we got a pinst ... then we should have an inst array.

        if ( pinstTarget )
        {
            // Target Instance is Running
            wszRestoreSystemPath = wszCheckpointFilePath;
            wszRestoreLogPath = wszLogPath;

            Assert ( pinstTarget->m_wszInstanceName );
        }
        else
        {
            // Target Not Running
            if ( fTargetName )
            {
                // the instance is provided not running and we have just the Instance Name
                // so we can't find out where the log files are. Error out
                Assert ( !fTargetDirs );
                Error( ErrERRCheck( JET_errBadRestoreTargetInstance ) );
            }

            Assert ( fTargetDirs );
            // instance found and not running, use TargetDirs
            wszRestoreSystemPath = wszTargetInstanceCheckpointPath;
            wszRestoreLogPath = wszTargetInstanceLogPath;
        }
    }

    // we have to set the system params first for the new instance
    Assert ( RUNINSTGetMode() == runInstModeMultiInst );

    if ( pinstTarget )
    {
        Call ( ErrINSTPrepareTargetInstance( pinstTarget, wszTargetLogPath, sizeof(wszTargetLogPath), wszLogBaseName, &lGenHighTarget ) );

        wszTargetDisplayName = pinstTarget->m_wszDisplayName ? pinstTarget->m_wszDisplayName : pinstTarget->m_wszInstanceName;

        if ( wszTargetDisplayName )
        {
            cchTargetDisplayName = ( LOSStrLengthW(wszTargetDisplayName) + 1 );
            wszRestoreInstanceNameUsed = wszRestoreInstanceNamePrefix;
        }
        else
        {
            Assert( 0 == cchTargetDisplayName );
            Assert( wszRestoreInstanceNameUsed == wszRestoreInstanceName );
        }
    }

    ULONG cchNewInstanceName = cchTargetDisplayName + LOSStrLengthW(wszRestoreInstanceNameUsed) + 4 + 1;
    Alloc( wszNewInstanceName = new WCHAR[ cchNewInstanceName ] );
    OSStrCbFormatW( wszNewInstanceName, cchNewInstanceName*sizeof(WCHAR), L"%-*s%s%04lu", (ULONG)cchTargetDisplayName, wszTargetDisplayName ? wszTargetDisplayName : L"",
            wszRestoreInstanceNameUsed, AtomicIncrement( (LONG*)&g_cRestoreInstance ) % 10000L );

    Call ( ErrNewInst( &pinst, wszNewInstanceName, NULL, &ipinst ) );
    Assert( !pinst->m_fJetInitialized );

    INST::LeaveCritInst();
    fInCritInst = fFalse;

    // we have to set the system params first
    if ( NULL != wszLogBaseName)
    {
        Call ( ErrSetSystemParameter( pinst, JET_sesidNil, JET_paramBaseName, 0, wszLogBaseName ) );
    }

    // set restore path, we have to do this before ErrIsamExternalRestore (like it was before)
    // because the check against the running instances is in ErrInit

    if ( wszRestoreLogPath )
    {
        Call ( ErrSetSystemParameter( pinst, JET_sesidNil, JET_paramLogFilePath, 0, wszRestoreLogPath ) );
    }

    if ( wszRestoreSystemPath )
    {
        Call ( ErrSetSystemParameter( pinst, JET_sesidNil, JET_paramSystemPath, 0, wszRestoreSystemPath ) );

        // put the temp db in the checkpoint directory
        OSStrCbCopyW( wszTempDatabase, sizeof(wszTempDatabase), wszRestoreSystemPath );
        if ( !FOSSTRTrailingPathDelimiterW(wszTempDatabase) )
        {
            OSStrCbAppendW( wszTempDatabase, sizeof(wszTempDatabase), wszPathDelimiter );
        }
        Call ( ErrSetSystemParameter( pinst, JET_sesidNil, JET_paramTempPath, 0, wszTempDatabase ) );
    }

    //  Start to do JetRestore on this instance

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
                    fTrue,      //  fSkipIsamInit
                    NO_GRBIT );
    Assert( JET_errAlreadyInitialized != err );

    Assert( !pinst->m_fJetInitialized );
    Assert( !pinst->m_fTermInProgress );

    if ( err >= 0 )
    {
        pinst->m_fJetInitialized = fTrue;

        // now, other restore what will try to do restore on the same instance
        // will error out with "restore in progress" from ErrINSTPrepareTargetInstance (actually ErrAPIEnter)
        // because we set fAPIRestoring AND m_fJetInitialized
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

        // We can't assert() this b/c the instance we just created, it's log file size is just complete ignored
        // by external restore, nor set when it figures out the size of the log files.  I'd like to assert this
        // after the log re-write.
        //Assert( pinst->m_plog->CSecLGFile() == __int64( ( UlParam( pinst, JET_paramLogFileSize ) ) * 1024 ) / pinst->m_plog->m_cbSec );

//      OSUTerm();

    }
    else
    {
        g_critRestoreInst.Leave();
        fInCritRestoreInst = fFalse;
    }

    pinst->m_fJetInitialized = fFalse;

    pinst->APIUnlock( pinst->fAPIRestoring );

//  pcritInst->Leave();

    //  Return and delete the instance
    Assert ( !fInCritInst );

    apicall.LeaveAfterCall( err );
    err = apicall.ErrResult();  // can't be called after freeing pinst

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
    _In_ JET_PSTR                                   szCheckpointFilePath,
    _In_ JET_PSTR                                   szLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP *     rgrstmap,
    _In_ LONG                                       crstfilemap,
    _In_ JET_PSTR                                   szBackupLogPath,
    _In_ LONG                                       genLow,
    _In_ LONG                                       genHigh,
    _In_ JET_PSTR                                   szLogBaseName,
    __in_opt JET_PCSTR                              szTargetInstanceName,
    __in_opt JET_PCSTR                              szTargetInstanceCheckpointPath,
    __in_opt JET_PCSTR                              szTargetInstanceLogPath,
    _In_ JET_PFNSTATUS                              pfn )
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
    _In_ JET_PSTR                                   szCheckpointFilePath,
    _In_ JET_PSTR                                   szLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_A *   rgrstmap,
    _In_ LONG                                       crstfilemap,
    _In_ JET_PSTR                                   szBackupLogPath,
    __inout JET_LOGINFO_A *                         pLogInfo,
    __in_opt JET_PCSTR                              szTargetInstanceName,
    __in_opt JET_PCSTR                              szTargetInstanceLogPath,
    __in_opt JET_PCSTR                              szTargetInstanceCheckpointPath,
    _In_ JET_PFNSTATUS                              pfn )
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
    _In_ JET_PWSTR                                  wszCheckpointFilePath,
    _In_ JET_PWSTR                                  wszLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_W *   rgrstmap,
    _In_ LONG                                       crstfilemap,
    _In_ JET_PWSTR                                  wszBackupLogPath,
    __inout JET_LOGINFO_W *                         pLogInfo,
    __in_opt JET_PCWSTR                             wszTargetInstanceName,
    __in_opt JET_PCWSTR                             wszTargetInstanceLogPath,
    __in_opt JET_PCWSTR                             wszTargetInstanceCheckpointPath,
    _In_ JET_PFNSTATUS                              pfn )
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
    _In_ JET_PSTR                                   szCheckpointFilePath,
    _In_ JET_PSTR                                   szLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_A *   rgrstmap,
    _In_ LONG                                       crstfilemap,
    _In_ JET_PSTR                                   szBackupLogPath,
    _In_ LONG                                       genLow,
    _In_ LONG                                       genHigh,
    _In_ JET_PFNSTATUS                              pfn )
{
    JET_TRY( opInit, JetExternalRestoreExA( szCheckpointFilePath, szLogPath, rgrstmap, crstfilemap, szBackupLogPath, genLow, genHigh, NULL, NULL, NULL, NULL, pfn ) );
}

JET_ERR JET_API JetExternalRestoreW(
    _In_ JET_PWSTR                                  wszCheckpointFilePath,
    _In_ JET_PWSTR                                  wszLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_W *   rgrstmap,
    _In_ LONG                                       crstfilemap,
    _In_ JET_PWSTR                                  wszBackupLogPath,
    _In_ LONG                                       genLow,
    _In_ LONG                                       genHigh,
    _In_ JET_PFNSTATUS                              pfn )
{
    JET_TRY( opInit, JetExternalRestoreEx( wszCheckpointFilePath, wszLogPath, rgrstmap, crstfilemap, wszBackupLogPath, genLow, genHigh, NULL, NULL, NULL, NULL, pfn ) );
}

JET_ERR JET_API JetExternalRestore2A(
    _In_ JET_PSTR                                   szCheckpointFilePath,
    _In_ JET_PSTR                                   szLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_A *   rgrstmap,
    _In_ LONG                                       crstfilemap,
    _In_ JET_PSTR                                   szBackupLogPath,
    __inout JET_LOGINFO_A *                         pLogInfo,
    __in_opt JET_PSTR                               szTargetInstanceName,
    __in_opt JET_PSTR                               szTargetInstanceLogPath,
    __in_opt JET_PSTR                               szTargetInstanceCheckpointPath,
    _In_ JET_PFNSTATUS                              pfn )
{
    JET_TRY( opInit, JetExternalRestore2ExA( szCheckpointFilePath, szLogPath, rgrstmap, crstfilemap, szBackupLogPath, pLogInfo, szTargetInstanceName, szTargetInstanceCheckpointPath, szTargetInstanceLogPath, pfn ) );
}

JET_ERR JET_API JetExternalRestore2W(
    _In_ JET_PWSTR                                  wszCheckpointFilePath,
    _In_ JET_PWSTR                                  wszLogPath,
    __in_ecount_opt( crstfilemap ) JET_RSTMAP_W *   rgrstmap,
    _In_ LONG                                       crstfilemap,
    _In_ JET_PWSTR                                  wszBackupLogPath,
    __inout JET_LOGINFO_W *                         pLogInfo,
    __in_opt JET_PWSTR                              wszTargetInstanceName,
    __in_opt JET_PWSTR                              wszTargetInstanceLogPath,
    __in_opt JET_PWSTR                              wszTargetInstanceCheckpointPath,
    _In_ JET_PFNSTATUS                              pfn )
{
    JET_TRY( opInit, JetExternalRestore2Ex( wszCheckpointFilePath, wszLogPath, rgrstmap, crstfilemap, wszBackupLogPath, pLogInfo, wszTargetInstanceName, wszTargetInstanceCheckpointPath, wszTargetInstanceLogPath, pfn ) );
}

JET_ERR JET_API JetSnapshotStartA(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PSTR       szDatabases,
    _In_ JET_GRBIT      grbit )
{
    //  OBSOLETE: never finished, VSS used instead
    //
    return ErrERRCheck( JET_wrnNyi );
}

JET_ERR JET_API JetSnapshotStartW(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PWSTR      szDatabases,
    _In_ JET_GRBIT      grbit )
{
    //  OBSOLETE: never finished, VSS used instead
    //
    return ErrERRCheck( JET_wrnNyi );
}

JET_ERR JET_API JetSnapshotStop(
    _In_ JET_INSTANCE   instance,
    _In_ JET_GRBIT      grbit)
{
    //  OBSOLETE: never finished, VSS used instead
    //
    return ErrERRCheck( JET_wrnNyi );
}


LOCAL JET_ERR JetResetCounterEx( _In_ JET_SESID sesid, _In_ LONG CounterType )
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
JET_ERR JET_API JetResetCounter( _In_ JET_SESID sesid, _In_ LONG CounterType )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opResetCounter, JetResetCounterEx( sesid, CounterType ) );
}


LOCAL JET_ERR JetGetCounterEx( _In_ JET_SESID sesid, _In_ LONG CounterType, _Out_ LONG *plValue )
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
JET_ERR JET_API JetGetCounter( _In_ JET_SESID sesid, _In_ LONG CounterType, _Out_ LONG *plValue )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetCounter, JetGetCounterEx( sesid, CounterType, plValue ) );
}

LOCAL JET_ERR JetGetThreadStatsEx(
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax )
{
    ProbeClientBuffer( pvResult, cbMax );

    OSTrace(
        JET_tracetagAPI,
        OSFormat(
            "Start %s(0x%p,0x%x)",
            __FUNCTION__,
            pvResult,
            cbMax ) );

    // this reflects the min size of the JET_THREADSTATS struct.  keep for app compat
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
    _In_ ULONG              cbMax )
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
    _In_ JET_SESID              sesid,
    _In_ JET_PCWSTR             wszDatabaseSrc,
    _In_ JET_PCWSTR             wszDatabaseDest,
    _In_ JET_PFNSTATUS          pfnStatus,
    __in_opt JET_CONVERT_W *    pconvert,
    _In_ JET_GRBIT              grbit )
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
    _In_ JET_SESID              sesid,
    _In_ JET_PCSTR              szDatabaseSrc,
    _In_ JET_PCSTR              szDatabaseDest,
    _In_ JET_PFNSTATUS          pfnStatus,
    __in_opt JET_CONVERT_A *    pconvert,
    _In_ JET_GRBIT              grbit )
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
    _In_ JET_SESID              sesid,
    _In_ JET_PCSTR              szDatabaseSrc,
    _In_ JET_PCSTR              szDatabaseDest,
    _In_ JET_PFNSTATUS          pfnStatus,
    __in_opt JET_CONVERT_A *    pconvert,
    _In_ JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCompact, JetCompactExA( sesid, szDatabaseSrc, szDatabaseDest, pfnStatus, pconvert, grbit ) );
}

JET_ERR JET_API JetCompactW(
    _In_ JET_SESID              sesid,
    _In_ JET_PCWSTR             wszDatabaseSrc,
    _In_ JET_PCWSTR             wszDatabaseDest,
    _In_ JET_PFNSTATUS          pfnStatus,
    __in_opt JET_CONVERT_W *    pconvert,
    _In_ JET_GRBIT              grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opCompact, JetCompactEx( sesid, wszDatabaseSrc, wszDatabaseDest, pfnStatus, pconvert, grbit ) );
}

LOCAL JET_ERR JetConvertDDLEx(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_OPDDLCONV              convtyp,
    __out_bcount( cbData ) void *   pvData,
    _In_ ULONG              cbData )
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
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_OPDDLCONV              convtyp,
    __out_bcount( cbData ) void *   pvData,
    _In_ ULONG              cbData )
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
            // the below will be returned anyway
            // err = ErrERRCheck( JET_errInvalidParameter );
            break;
    }

    err = JetConvertDDLEx( sesid, dbid, convtyp, lpvData, lcbData );

HandleError:
    delete[] pcondcolumn;
    delete[] pcondcolumnName;

    return err;
}

JET_ERR JET_API JetConvertDDLA(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_OPDDLCONV              convtyp,
    __out_bcount( cbData ) void *   pvData,
    _In_ ULONG              cbData )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opConvertDDL, JetConvertDDLEx( sesid, dbid, convtyp, pvData, cbData ) );
}

JET_ERR JET_API JetConvertDDLW(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_OPDDLCONV              convtyp,
    __out_bcount( cbData ) void *   pvData,
    _In_ ULONG              cbData )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opConvertDDL, JetConvertDDLExW( sesid, dbid, convtyp, pvData, cbData ) );
}

LOCAL JET_ERR JetUpgradeDatabaseEx(
    _In_ JET_SESID          sesid,
    _In_ JET_PCWSTR         wszDbFileName,
    _In_ const JET_GRBIT    grbit )
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
    _In_ JET_SESID          sesid,
    _In_ JET_PCSTR          szDbFileName,
    _In_ const JET_GRBIT    grbit )
{
    ERR             err             = JET_errSuccess;
    CAutoWSZPATH    lwszDbFileName;

    CallR( lwszDbFileName.ErrSet( szDbFileName ) );

    return JetUpgradeDatabaseEx( sesid, lwszDbFileName, grbit );
}

JET_ERR JET_API JetUpgradeDatabaseA(
    _In_ JET_SESID          sesid,
    _In_ JET_PCSTR          szDbFileName,
    _Reserved_ JET_PCSTR    szSLVFileName,
    _In_ const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opUpgradeDatabase, JetUpgradeDatabaseExA( sesid, szDbFileName, grbit ) );
}

JET_ERR JET_API JetUpgradeDatabaseW(
    _In_ JET_SESID          sesid,
    _In_ JET_PCWSTR         wszDbFileName,
    _Reserved_ JET_PCWSTR   wszSLVFileName,
    _In_ const JET_GRBIT    grbit )
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
                                    _In_ JET_GRBIT      grbit );
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

        // new code should follow the discriminated union model
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
            // default code for legacy parameters
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
    BOOL    fOSUInitCalled  = fFalse;

    // Initialize necessary subsystems

    g_rwlOSUInit.EnterAsReader();
    Call( ErrOSUInit() );
    fOSUInitCalled = fTrue;

    Call( ErrDBUpdateHeaderFromTrailer( szDatabase, fSkipMinLogChecks ) );

HandleError:

    if ( fOSUInitCalled )
    {
        OSUTerm();
    }
    g_rwlOSUInit.LeaveAsReader();

    return err;
}

LOCAL JET_ERR JetDBUtilitiesEx( JET_DBUTIL_W *pdbutilW )
{
    AssertSzRTL( (JET_DBUTIL_W*)0 != pdbutilW, "Invalid (NULL) pdbutil. Call JET dev." );
    AssertSzRTL( (JET_DBUTIL_W*)(-1) != pdbutilW, "Invalid (-1) pdbutil. Call JET dev." );

    // We validate here that JET_DBUTIL_W does not change in size because that
    // would break forward and backward compatibility for a number of tools. In
    // particular, we regularly use perf tests across versions in order to narrow
    // down regressions.
    
#ifdef _WIN64
    C_ASSERT( sizeof(JET_DBUTIL_W) == 136 );
#else  //  !_WIN64
    C_ASSERT( sizeof(JET_DBUTIL_W) == 84 );
#endif // !_WIN64
    
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

    //  don't init if we're only dumping the logfile/checkpoint/DB header.
    switch( pdbutilW->op )
    {
        case opDBUTILUpdateDBHeaderFromTrailer:
            // Clients are normally not supposed to patch the database header themselves following a backup, since that will be
            // done when the backup up database is restored and is being brought back to a consistent state prior to a restore.
            // The only client that is known to do this is Exchange's HA replication (in addition to ESE-level test code).
            // With the introduction of restartable seeds, some long-held assumptions and checks about the backup set and its
            // required range may not hold anymore. Therefore, this JET_bitDBUtilOptionSkipMinLogChecksUpdateHeader option was
            // created to handle that case.
            // I (SOMEONE) think we shouldn't even have the bit and just always pass fTrue to the function below to signal
            // that the backup set is being handled externally, regardless of whether or not it was restarted. Perhaps we
            // should do that in the future and deprecate exposing the bit in the first place.
            //
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
                //  choose a unique instance name to allow multi-threaded access
                char szInstanceName[ 64 ];
                
                OSStrCbFormatA( szInstanceName, sizeof( szInstanceName ),
                                "JetDBUtilities - %lu",
                                DwUtilThreadId() );
                
                Call( JetCreateInstance( &instance, szInstanceName ) );
                fCreatedInst = fTrue;
                
                //  this is stuff that eseutil does; and required of anyone
                //  who sets up the instance before calling these functions
                Call( JetSetSystemParameterW( &instance, 0, JET_paramRecovery,           0, L"off" ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableOnlineDefrag, 0, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxTemporaryTables, 0, NULL ) );

                //  disable information events for internal Instances
                Call( JetSetSystemParameterW( &instance, 0, JET_paramNoInformationEvent, 1, NULL ) );

                //  set the base name
                if ( pdbutilW->op == opDBUTILChecksumLogFromMemory )
                {
                    Call( JetSetSystemParameterW( &instance, 0, JET_paramBaseName, 0, pdbutilW->checksumlogfrommemory.szBase ) );
                }

                //  a failure in JetInit auto-cleans the inst
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
    //  OBSOLETE: only used by SFS
    //
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
    //  OBSOLETE: only used by SFS
    //
    return ErrERRCheck( JET_errInvalidParameter );
}

LOCAL JET_ERR JetDatabaseScanEx(
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    __inout_opt ULONG * const pcSeconds,
    _In_ ULONG  cmsecSleep,
    _In_ JET_CALLBACK   pfnCallback,
    _In_ JET_GRBIT      grbit )
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
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    __inout_opt ULONG * pcSeconds,
    _In_ ULONG  cmsecSleep,
    _In_ JET_CALLBACK   pfnCallback,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opDatabaseScan, JetDatabaseScanEx( sesid, dbid, pcSeconds, cmsecSleep, pfnCallback, grbit ) );
}

LOCAL JET_ERR JetSetMaxDatabaseSizeEx(
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    _In_ ULONG  cpg,
    _In_ JET_GRBIT      grbit )
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
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    _In_ ULONG  cpg,
    _In_ JET_GRBIT      grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opSetDatabaseSize, JetSetMaxDatabaseSizeEx( sesid, dbid, cpg, grbit ) );
}


LOCAL JET_ERR JetGetMaxDatabaseSizeEx(
    _In_ JET_SESID          sesid,
    _In_ JET_DBID           dbid,
    _Out_ ULONG *   pcpg,
    _In_ JET_GRBIT          grbit )
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
    _In_ JET_SESID          sesid,
    _In_ JET_DBID           dbid,
    _Out_ ULONG *   pcpg,
    _In_ JET_GRBIT          grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGetDatabaseSize, JetGetMaxDatabaseSizeEx( sesid, dbid, pcpg, grbit ) );
}


LOCAL JET_ERR JetSetDatabaseSizeEx(
    _In_ JET_SESID          sesid,
    _In_ JET_PCWSTR         wszDatabaseName,
    _In_ ULONG      cpg,
    _Out_ ULONG *   pcpgReal )
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
    _In_ JET_SESID          sesid,
    _In_ JET_PCSTR          szDatabaseName,
    _In_ ULONG      cpg,
    _Out_ ULONG *   pcpgReal )
{
    ERR             err             = JET_errSuccess;
    CAutoWSZPATH    lwszDatabaseName;

    CallR( lwszDatabaseName.ErrSet( szDatabaseName ) );


    return JetSetDatabaseSizeEx( sesid, lwszDatabaseName, cpg, pcpgReal );
}

JET_ERR JET_API JetSetDatabaseSizeA(
    _In_ JET_SESID          sesid,
    _In_ JET_PCSTR          szDatabaseName,
    _In_ ULONG      cpg,
    _Out_ ULONG *   pcpgReal )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opSetDatabaseSize, JetSetDatabaseSizeExA( sesid, szDatabaseName, cpg, pcpgReal ) );
}

JET_ERR JET_API JetSetDatabaseSizeW(
    _In_ JET_SESID          sesid,
    _In_ JET_PCWSTR         wszDatabaseName,
    _In_ ULONG      cpg,
    _Out_ ULONG *   pcpgReal )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opSetDatabaseSize, JetSetDatabaseSizeEx( sesid, wszDatabaseName, cpg, pcpgReal ) );
}

LOCAL JET_ERR JetGrowDatabaseEx(
    _In_ JET_SESID          sesid,
    _In_ JET_DBID           dbid,
    _In_ ULONG      cpg,
    _In_ ULONG *    pcpgReal )
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
    _In_ JET_SESID          sesid,
    _In_ JET_DBID           dbid,
    _In_ ULONG      cpg,
    _In_ ULONG *    pcpgReal )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opGrowDatabase, JetGrowDatabaseEx( sesid, dbid, cpg, pcpgReal ) );
}

LOCAL JET_ERR JetResizeDatabaseEx(
    _In_ JET_SESID          sesid,
    _In_ JET_DBID           dbid,
    _In_ ULONG      cpg,
    _Out_ ULONG *   pcpgActual,
    _In_ const JET_GRBIT    grbit )
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
    _In_ JET_SESID          sesid,
    _In_ JET_DBID           dbid,
    _In_ ULONG      cpg,
    _Out_ ULONG *   pcpgActual,
    _In_ const JET_GRBIT    grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opResizeDatabase, JetResizeDatabaseEx( sesid, dbid, cpg, pcpgActual, grbit ) );
}

LOCAL JET_ERR JetSetSessionContextEx( _In_ JET_SESID sesid, _In_ ULONG_PTR ulContext )
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
JET_ERR JET_API JetSetSessionContext( _In_ JET_SESID vsesid, _In_ ULONG_PTR ulContext )
{
    JET_VALIDATE_SESID( vsesid );
    JET_TRY( opSetSessionContext, JetSetSessionContextEx( vsesid, ulContext ) );
}

LOCAL JET_ERR JetResetSessionContextEx( _In_ JET_SESID sesid )
{
    APICALL_SESID   apicall( opResetSessionContext );

    OSTrace( JET_tracetagAPI, OSFormat( "Start %s(0x%Ix)", __FUNCTION__, sesid ) );

    if ( apicall.FEnter( sesid, fTrue ) )
    {
        apicall.LeaveAfterCall( ErrIsamResetSessionContext( sesid ) );
    }

    return apicall.ErrResult();
}
JET_ERR JET_API JetResetSessionContext( _In_ JET_SESID sesid )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opResetSessionContext, JetResetSessionContextEx( sesid ) );
}

#pragma warning( disable : 4509 )

/*=================================================================
JetSetSystemParameter

Description:
  This function sets system parameter values.  It calls ErrSetSystemParameter
  to actually set the parameter values.

Parameters:
  sesid     is the optional session identifier for dynamic parameters.
  paramid   is the system parameter code identifying the parameter.
  lParam    is the parameter value.
  sz        is the zero terminated string parameter.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidParameter:
      Invalid parameter code.
  JET_errAlreadyInitialized:
      Initialization parameter cannot be set after the system is initialized.
  JET_errInvalidSesid:
      Dynamic parameters require a valid session id.

Side Effects:
  * May allocate memory
=================================================================*/

LOCAL JET_ERR JetSetSystemParameterEx(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_SESID          sesid,
    _In_ ULONG          paramid,
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

    //  try to divine what instance we are addressing (if any) based on our
    //  args.  this is complicated due to backwards compatability
    //
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

    //  if no instance was found then we can not lock the API.  also, ignore
    //  any provided sesid because it is meaningless
    //
    if ( !pinst )
    {
        const ERR err = ErrSetSystemParameter( pinstNil, JET_sesidNil, paramid, lParam, wszParam, fFalse );
        INST::LeaveCritInst();
        return err;
    }

    //  set the parameter
    //

    //  while we can't allow actions to run concurrent with JetInit, we can allow an exemption
    //  for if we're in a callback while JetInit (b/c JetInit can't tear down the instance
    //  until the callback returns).
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
    _In_ ULONG          paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCSTR          szParam )
{
    ERR         err         = JET_errSuccess;
    CAutoWSZ    lwszParam; // 90 is optimization

    CallR( lwszParam.ErrSet( szParam ) );

    return JetSetSystemParameterEx( pinstance, sesid, paramid, lParam, lwszParam );
}

JET_ERR JET_API JetSetSystemParameterA(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_SESID          sesid,
    _In_ ULONG          paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCSTR          szParam )
{
    JET_TRY( opSetSystemParameter, JetSetSystemParameterExA( pinstance, sesid, paramid, lParam, szParam ) );
}

JET_ERR JET_API JetSetSystemParameterW(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_SESID          sesid,
    _In_ ULONG          paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCWSTR         wszParam )
{
    JET_TRY( opSetSystemParameter, JetSetSystemParameterEx( pinstance, sesid, paramid, lParam, wszParam ) );
}

JET_ERR JetGetResourceParamEx(
    _In_ JET_INSTANCE   instance,
    _In_ JET_RESOPER    resoper,
    _In_ JET_RESID      resid,
    _Out_ JET_API_PTR*  pulParam )
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
        //  setting for global default
        return ErrRESGetResourceParam( pinstNil, resid, resoper, pulParam );
    }

    if ( apicall.FEnterWithoutInit( pinst ) )
    {
        apicall.LeaveAfterCall( ErrRESGetResourceParam( pinst, resid, resoper, pulParam ) );
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetGetResourceParam(
    _In_ JET_INSTANCE   instance,
    _In_ JET_RESOPER    resoper,
    _In_ JET_RESID      resid,
    _Out_ JET_API_PTR*  pulParam )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opGetResourceParam, JetGetResourceParamEx( instance, resoper, resid, pulParam ) );
}


JET_ERR JetSetResourceParamEx(
    _In_ JET_INSTANCE   instance,
    _In_ JET_RESOPER    resoper,
    _In_ JET_RESID      resid,
    _In_ JET_API_PTR    ulParam )
{
    //  atomically apply to all resources via a recursive call
    //
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
                //  This resid is deprecated, so skip it ...
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

    //  apply to one resource
    //
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

    //  if we aren't in advanced mode then don't allow these to be set
    //
    if ( !BoolParam( pinst, JET_paramEnableAdvanced ) )
    {
        return JET_errSuccess;
    }

    if ( pinstNil == pinst || JET_resoperMaxUse != resoper )
    {
        //  some of them should still go through SetSystemParameter
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

        //  setting for global default
        else
        {
            return ErrRESSetResourceParam( pinstNil, resid, resoper, ulParam );
        }
    }

    if ( apicall.FEnterWithoutInit( pinst ) )
    {
        switch ( resid )
        {
            //  Supported through SetSystemParameter
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

            //  everything else
            default:
                apicall.LeaveAfterCall( ErrRESSetResourceParam( pinst, resid, resoper, ulParam ) );
        }
    }

    return apicall.ErrResult();
}

JET_ERR JET_API JetSetResourceParam(
    _In_ JET_INSTANCE   instance,
    _In_ JET_RESOPER    resoper,
    _In_ JET_RESID      resid,
    _In_ JET_API_PTR    ulParam )
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

    //  Enter API for this instance

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
                    fFalse,         //  fSkipIsamInit
                    grbit );

    Assert( JET_errAlreadyInitialized != err );

    Assert( !pinst->m_fJetInitialized );
    Assert( !pinst->m_fTermInProgress );

    if ( err >= 0 )
    {
        pinst->m_fJetInitialized = fTrue;

        //  backup allowed only after Jet is properly initialized.

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

    //  verify no one beat us to it
    //
    if ( g_rgpinst[ipinst] != pinst )
    {
        pcritInst->Leave();
        return ErrERRCheck( JET_errNotInitialized );
    }
    else if ( !pinst->m_fJetInitialized
        && ( ( pinst->m_fSTInit == fSTInitNotDone )
            || ( pinst->m_fSTInit == fSTInitFailed ) ) )
    {
        //  instance was allocated, but never initialized
        //  (or possibly the instance has been terminated
        //  but not yet cleaned up, in which case we end
        //  up racing with the thread who terminated the
        //  instance, but hasn't yet cleaned up the INST
        //  resource - it doesn't really matter who wins)
        //
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
            //  skip PIB's on the global list which are actually free
            //  (their m_cInJetAPI may actually be non-zero, because
            //  we leave them as non-zero to try to trap anyone
            //  using a session that's been closed)
            //
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

    // we used to do not stop the backup on JET_bitTermAbrupt alone
    // (just on JET_bitTermStopBackup) but it makes sense to do it:
    // after all they called with JET_bitTermAbrupt ...
    //
    if ( grbit & JET_bitTermStopBackup || grbit & JET_bitTermAbrupt )
    {
        // we are going to wait for a potential freeze to
        // finish by just entering critBackupInProgress which
        // is held during snapshot
        // also, because we will use SnapshotCritEnter, there is
        // no Snapshot API in progress
        //
        CESESnapshotSession::SnapshotCritEnter();
        if ( pinst->m_pOSSnapshotSession )
        {

            pinst->m_pbackup->BKLockBackup();
            pinst->m_pbackup->BKUnlockBackup();

            // Lazy way to wait until the snapshot terminates / aborts and comes back to us, letting
            // the term thread go on normally. 
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

            // BUG: (also in jet.h, JET_errBackupAbortByServer added)
            // will close the backup resources only if needed. Will return JET_errNoBackup if no backup.
            CallSx( pinst->m_pbackup->ErrBKExternalBackupCleanUp( JET_errBackupAbortByServer ), JET_errNoBackup );
            // thats' all for this bug

            pinst->m_pbackup->BKAssertNoBackupInProgress();
        }
    }

    pinst->m_isdlTerm.Trigger( eTermWaitedForUserWorkerThreads );

    err = ErrIsamTerm( (JET_INSTANCE)pinst, grbit );
    Assert( pinst->m_fJetInitialized );

    //  g_cpinstInit is about to be decremented, so must
    //  decrement cTermsInProgress accordingly
    //
    AtomicDecrement( (LONG *)&g_cTermsInProgress );

    const BOOL  fTermCompleted  = ( fSTInitNotDone == pinst->m_fSTInit );   //  is ISAM initialised?

    if ( fTermCompleted )
    {
//      OSUTerm();
        pinst->m_fJetInitialized = fFalse;
    }

    OSTrace( JET_tracetagInitTerm, OSFormat( "Term[%p] finished err=%d, fTermCompleted=%d", pinst, err, fTermCompleted ) );

    pinst->APIUnlock( pinst->fAPITerminating );

    // we don't need the m_fTermInProgress flag anymore
    // and we should reset it as they might retry termination if we error out
    // (at which point we will expect m_fTermInProgress to NOT be set
    //
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

    // maintain the original
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


/*

The running mode must be "NoSet" to set the mode to multi-instance.
Used to set the global param.

In:
    psetsysparam - array of parameters to set
    csetsysparam - items count in array

Out:
    pcsetsucceed - count of params set w/o error (pointer can de NULL)
    psetsysparam[i].err - set to error code from setting  psetsysparam[i].param

Return:
    JET_errSuccess or first error in array

Obs:
    it stops at the first error
*/
LOCAL JET_ERR JetEnableMultiInstanceEx(
    __in_ecount_opt( csetsysparam ) JET_SETSYSPARAM_W * psetsysparam,
    _In_ ULONG                                  csetsysparam,
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

        //  function can't be called twice (JET_errSystemParamsAlreadySet) or in One Instance Mode (JET_errRunningInOneInstanceMode)
        if ( RUNINSTGetMode() != runInstModeMultiInst )
        {
            Assert( RUNINSTGetMode() == runInstModeOneInst );
            Error( ErrERRCheck( JET_errRunningInOneInstanceMode ) );
        }

        //  We now compare the JET params the client specified to the ones actually set ...
        for ( ULONG iset = 0; iset < csetsysparam; iset++ )
        {
            BOOL fMatches = fFalse;

            if ( psetsysparam[iset].paramid >= _countof(g_rgparamRaw) )
            {
                //  client is trying to make us crash!
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

            //  Note this is set.
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
                //  We're using strict string comparison to start with, as it should be plausible for two 
                //  apps to set this to something they specifically agree on.
                fMatches = ( 0 == LOSStrCompareW( (WCHAR*)(g_rgparam[iparamid].Value()), psetsysparam[iset].sz ) );
                break;
            case JetParam::typePointer:
            case JetParam::typeUserDefined:
                fMatches = ( g_rgparam[iparamid].Value() == psetsysparam[iset].lParam );
                Expected( fMatches );   //  if we get a not match, we should probably check it out
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
        }   //  for each pjetsetparam arg

        //  Walking all the static (that aren't allowed to be changed after global init) global 
        //  JET params, and track which ones are set, b/c the pjetsetparams this client uses 
        //  should be the same ... which we check lower.
        for ( size_t iparamid = 0; iparamid < _countof(g_rgparamRaw); iparamid++ )
        {
            Assert( g_rgparam[iparamid].m_paramid == iparamid );    //  sanity check

            if ( g_rgparam[iparamid].FGlobal() &&
                    g_rgparam[iparamid].m_fMayNotWriteAfterGlobalInit )
            {
                const BOOL fOriginallySet = g_rgparam[iparamid].FWritten();
                BOOL fNewlySet = fFalse;
                IBitmapAPI::ERR errBitmap = fbm.ErrGet( iparamid, &fNewlySet );
                Assert( errBitmap == IBitmapAPI::ERR::errSuccess );
                if ( fOriginallySet != fNewlySet &&
                        //  Since the registry defaults _appear written_, it can seem like the
                        //  two sets disagree.  We will assume that if they set the same value
                        //  that they got the same set of params.  This is not the safest bet.
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

        //  All parameters check out to be identical / equivalent, so return "success" of a form ... 
        //  This error returned here, should've been a warning, but too late now. Right? Sigh.
        Error( ErrERRCheck( JET_errSystemParamsAlreadySet ) );
    }

    for ( ULONG iset = 0; iset < csetsysparam; iset++ )
    {
        psetsysparam[ iset ].err = ErrSetSystemParameter( pinstNil, JET_sesidNil, psetsysparam[ iset ].paramid, psetsysparam[ iset ].lParam, psetsysparam[ iset ].sz, fFalse );
        Call( psetsysparam[ iset ].err );
        Assert( g_rgparam[ psetsysparam[ iset ].paramid ].m_fWritten );
        csetsucceed++;
    }

    //  if setting was successful, set the multi-instance mode
    //
    Assert( err >= JET_errSuccess );
    RUNINSTSetModeMultiInst();

HandleError:
    // UNDONE: if error: restore the params changed before the error
    //

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
    _In_ ULONG                                  csetsysparam,
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
    _In_ ULONG                                  csetsysparam,
    __out_opt ULONG *                           pcsetsucceed )
{
    JET_TRY( opEnableMultiInstance, JetEnableMultiInstanceExA( psetsysparam, csetsysparam, pcsetsucceed ) );
}

JET_ERR JET_API JetEnableMultiInstanceW(
    __in_ecount_opt( csetsysparam ) JET_SETSYSPARAM_W * psetsysparam,
    _In_ ULONG                                  csetsysparam,
    __out_opt ULONG *                           pcsetsucceed )
{
    JET_TRY( opEnableMultiInstance, JetEnableMultiInstanceEx( psetsysparam, csetsysparam, pcsetsucceed ) );
}

LOCAL JET_ERR JetInitEx(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_RSTINFO2_W *   prstInfo,
    _In_ JET_GRBIT              grbit )
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

    CallR( ErrFaultInjection( 60589 ) );    // Used by the fault injection unit-test
    Assert( JET_errSuccess == ErrFaultInjection( 50587 ) ); // Used by the assert handling unit-test
    if ( JET_errSuccess != ErrFaultInjection( 47515 ) ) // Used by the exception handling unit-test
    {
        *( char* )0 = 0;
    }
    
    INST::EnterCritInst();
    fInCritSec = fTrue;

    // instance value to be returned must be provided in multi-instance
    if ( RUNINSTGetMode() == runInstModeMultiInst && !pinstance )
    {
        Error( ErrERRCheck( JET_errRunningInMultiInstanceMode ) );
    }

    // just one instance accepted in one instance mode
    if ( RUNINSTGetMode() == runInstModeOneInst && 0 != g_cpinstInit )
    {
        Error( ErrERRCheck( JET_errAlreadyInitialized ) ); // one instance already started
    }

    if ( prstInfo && prstInfo->cbStruct != sizeof( JET_RSTINFO2_W ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  A singular implicit JetInit() without a JetCreateInstance() or JetEnableMultiInstance()
    //  call will forever live as single instance mode.
    // no previous call to JetSetSystemParam, JetEnableMultiInstance, JetCreateInstance, JetInit
    // or all the previous started instances are no longer active
    if ( RUNINSTGetMode() == runInstModeNoSet )
    {
        Assert ( 0 == g_cpinstInit);
        RUNINSTSetModeOneInst();
    }

    // in one instance mode, allocate instance even if pinstance (and *pinstance) not null
    // because this values can be bogus
    fAllocateInstance = fAllocateInstance | (RUNINSTGetMode() == runInstModeOneInst);

    // alocate a new instance or find the one provided (previously allocate with JetCreateInstance)
    if ( fAllocateInstance )
    {
        Call( ErrNewInst( &pinst, NULL, NULL, NULL ) );
        Assert( !pinst->m_fJetInitialized );
    }
    else
    {
        Call( ErrFindPinst( *pinstance, &pinst ) );
        //  Some applications call JetCreateInstance( which initialized the pinst->m_isdlInit to a given TID ) on a different
        //  thread than they call JetInit() - so we need to inform the IsamSequence to expect this new thread in this case.
        Assert( pinst->m_isdlInit.FActiveSequence() );  // should, we hope, be active from the JetCreateInstance
        pinst->m_isdlInit.ReThreadSequence();
    }

    INST::LeaveCritInst();
    fInCritSec = fFalse;

    // make the initialization

    OSTrace( JET_tracetagInitTerm, OSFormat( "JetInit instance %p [grbit=0x%x]", pinst, grbit ) );

    CallJ( ErrInitComplete( JET_INSTANCE( pinst ),
                            prstInfo,
                            grbit ), TermAlloc );

    pinst->m_isdlInit.FixedData().sInitData.fRBSOn = pinst->FComputeRBSOn();

    Assert( err >= 0 );

    if ( fAllocateInstance && pinstance )
        *pinstance = (JET_INSTANCE) pinst;

    pinst->m_isdlInit.Trigger( eInitDone );

    // report our successful init and timings

    const ULONG cbAdditionalFixedData = pinst->m_isdlInit.CbSprintFixedData();
    WCHAR * wszAdditionalFixedData = (WCHAR *)_alloca( cbAdditionalFixedData );
    pinst->m_isdlInit.SprintFixedData( wszAdditionalFixedData, cbAdditionalFixedData );
    const ULONG cbTimingResourceDataSequence = pinst->m_isdlInit.CbSprintTimings();
    WCHAR * wszTimingResourceDataSequence = (WCHAR *)_alloca( cbTimingResourceDataSequence );
    pinst->m_isdlInit.SprintTimings( wszTimingResourceDataSequence, cbTimingResourceDataSequence );
    const __int64 secsInit = pinst->m_isdlInit.UsecTimer( eSequenceStart, eInitDone ) / 1000000;    // convert to seconds
    WCHAR wszSeconds[16];
    WCHAR wszInstId[16];
    OSStrCbFormatW( wszSeconds, sizeof(wszSeconds), L"%I64d", secsInit );
    OSStrCbFormatW( wszInstId, sizeof(wszInstId), L"%d", IpinstFromPinst( pinst ) );
    const WCHAR * rgszT[4] = { wszInstId, wszSeconds, wszTimingResourceDataSequence, wszAdditionalFixedData };

    OSTrace( JET_tracetagInitTerm, OSFormat( "JetInit() successful [instance=%p]. Details:\r\n\t\tTimings Resource Usage: %ws", pinst, wszTimingResourceDataSequence ) );

    pinst->m_isdlInit.TermSequence();   //  optimistic release of memory

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

    // if instance allocated in this function call
    // or created by create instance, clean it
    if ( fAllocateInstance || ( NULL != pinstance && !FINSTInvalid( *pinstance ) ) )
    {
        // still, if the error is because the instance is already initialized
        // we don't want to Term it

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

JET_ERR JET_API JetInit2( __inout_opt JET_INSTANCE *pinstance, _In_ JET_GRBIT grbit )
{
    return JetInit3A( pinstance, NULL, grbit | JET_bitReplayMissingMapEntryDB );
}

JET_ERR JET_API JetInit3A(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_RSTINFO_A *    prstInfo,
    _In_ JET_GRBIT              grbit )
{
    ERR             err;
    CAutoRSTINFOW   lrstInfoW;

    CallR( lrstInfoW.ErrSet( prstInfo ) );

    return JetInit4W( pinstance, lrstInfoW, grbit );
}

JET_ERR JET_API JetInit3W(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_RSTINFO_W *    prstInfo,
    _In_ JET_GRBIT              grbit )
{
    ERR             err;
    CAutoRSTINFOW   lrstInfoW;

    CallR( lrstInfoW.ErrSet( prstInfo ) );

    return JetInit4W( pinstance, lrstInfoW, grbit );
}

JET_ERR JET_API JetInit4A(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_RSTINFO2_A *   prstInfo,
    _In_ JET_GRBIT              grbit )
{
    ERR             err;
    CAutoRSTINFOW   lrstInfoW;

    CallR( lrstInfoW.ErrSet( prstInfo ) );

    return JetInit4W( pinstance, lrstInfoW, grbit );
}

JET_ERR JET_API JetInit4W(
    __inout_opt JET_INSTANCE *  pinstance,
    __in_opt JET_RSTINFO2_W *   prstInfo,
    _In_ JET_GRBIT              grbit )
{
    //  UNDONE:  validate instance
    JET_TRY( opInit, JetInitEx( pinstance, prstInfo, grbit ) );
}

/*
Used to allocate an instance. This allowes to change
the instance parameters before calling JetInit for that instance.
The system params for this instance are set to the global ones
*/
LOCAL JET_ERR JetCreateInstanceEx(
    _Out_ JET_INSTANCE *    pinstance,
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

    // the allocated instance must be returned
    if ( !pinstance )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    Assert ( pinstance );
    *pinstance = JET_instanceNil;

    INST::EnterCritInst();

    // not allowed in one instance mode
    if ( RUNINSTGetMode() == runInstModeOneInst )
    {
        INST::LeaveCritInst();
        return ErrERRCheck( JET_errRunningInOneInstanceMode );
    }

    // set to multi-instance mode if not set
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
    _Out_ JET_INSTANCE *    pinstance,
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
    _Out_ JET_INSTANCE *    pinstance,
    __in_opt JET_PCSTR      szInstanceName )
{
    return JetCreateInstance2A( pinstance, szInstanceName, NULL, NO_GRBIT );
}

JET_ERR JET_API JetCreateInstanceW(
    _Out_ JET_INSTANCE *    pinstance,
    __in_opt JET_PCWSTR     wszInstanceName )
{
    return JetCreateInstance2W( pinstance, wszInstanceName, NULL, NO_GRBIT );
}


// Note: grbit is currently unused
JET_ERR JET_API JetCreateInstance2A(
    _Out_ JET_INSTANCE *    pinstance,
    __in_opt JET_PCSTR      szInstanceName,
    __in_opt JET_PCSTR      szDisplayName,
    _In_ JET_GRBIT          grbit )
{
    JET_TRY( opCreateInstance, JetCreateInstanceExA( pinstance, szInstanceName, szDisplayName ) );
}

JET_ERR JET_API JetCreateInstance2W(
    _Out_ JET_INSTANCE *    pinstance,
    __in_opt JET_PCWSTR     wszInstanceName,
    __in_opt JET_PCWSTR     wszDisplayName,
    _In_ JET_GRBIT          grbit )
{
    JET_TRY( opCreateInstance, JetCreateInstanceEx( pinstance, wszInstanceName, wszDisplayName ) );
}

LOCAL JET_ERR JET_API JetGetInstanceMiscInfoEx(
    _In_ JET_INSTANCE               instance,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
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
                //  should be impossible, since the info level should already
                //  have been validated above
                //
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
    _In_ JET_INSTANCE               instance,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opGetInstanceMiscInfo, JetGetInstanceMiscInfoEx( instance, pvResult, cbMax, InfoLevel ) );
}


LOCAL JET_ERR JetRestoreInstanceEx(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCWSTR     wsz,
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
                    fTrue,      //  fSkipIsamInit
                    NO_GRBIT );

    Assert( JET_errAlreadyInitialized != err );

    Assert( !pinst->m_fJetInitialized );
    Assert( !pinst->m_fTermInProgress );

    if ( err >= 0 )
    {
        pinst->m_fJetInitialized = fTrue;

        InitCallbackWrapper initCallbackWrapper(pfn);
        err = ErrIsamRestore( (JET_INSTANCE) pinst, wsz, wszDest, InitCallbackWrapper::PfnWrapper, &initCallbackWrapper );

        // log is termed by ErrIsamRestore, so this assert does not make sense
        // Assert( err < JET_errSuccess || pinst->m_plog->CSecLGFile() == __int64( ( UlParam( pinst, JET_paramLogFileSize ) ) * 1024 ) / pinst->m_plog->CbSec() || fAllocateInstance );

//      OSUTerm();

        pinst->m_fJetInitialized = fFalse;
    }

    pinst->APIUnlock( pinst->fAPIRestoring );

    apicall.LeaveAfterCall( err );
    err = apicall.ErrResult();  // can't be called after freeing pinst

    if ( fAllocateInstance )
    {
        FreePinst( pinst );
    }

    Assert( !Ptls()->fInSoftStart );
    return err;
}

LOCAL JET_ERR JetRestoreInstanceExA(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCSTR      sz,
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
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCSTR      sz,
    __in_opt JET_PCSTR  szDest,
    __in_opt JET_PFNSTATUS  pfn )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opRestoreInstance, JetRestoreInstanceExA( instance, sz, szDest, pfn ) );
}

JET_ERR JET_API JetRestoreInstanceW(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCWSTR     wsz,
    __in_opt JET_PCWSTR wszDest,
    __in_opt JET_PFNSTATUS  pfn )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opRestoreInstance, JetRestoreInstanceEx( instance, wsz, wszDest, pfn ) );
}

LOCAL JET_ERR JetTermEx( _In_ JET_INSTANCE instance, _In_ JET_GRBIT grbit )
{
    OSTrace( JET_tracetagAPI, OSFormat( "Start %s(0x%Ix,0x%x)", __FUNCTION__, instance, grbit ) );
    COSTraceTrackErrors trackerrors( __FUNCTION__ );
    JET_ERR err = ErrTermComplete( instance, grbit );
    return err;
}

JET_ERR JET_API JetTerm( _In_ JET_INSTANCE instance )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opTerm, JetTermEx( instance, JET_bitTermAbrupt ) );
}
JET_ERR JET_API JetTerm2( _In_ JET_INSTANCE instance, _In_ JET_GRBIT grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opTerm, JetTermEx( instance, grbit ) );
}

JET_ERR JET_API JetStopServiceInstanceExOld( _In_ JET_INSTANCE instance )
{
    ERR     err;
    INST    *pinst;

    OSTrace( JET_tracetagAPI, OSFormat( "Start %s(0x%Ix)", __FUNCTION__, instance ) );

    CallR( ErrFindPinst( instance, &pinst ) );

    //  Halt OLD for this instance

    DBMScanStopAllScansForInst( pinst );
    OLDTermInst( pinst );
    OLD2TermInst( pinst );
    PagePatching::TermInst( pinst );

    //  signal version cleanup to minimise
    //  the amount of work version cleanup
    //  has to do on term
    //
    pinst->m_pver->VERSignalCleanup();

    //  Set a very small checkpoint depth to cause the database cache
    //  to start flushing all important dirty pages to the database.
    //  Note that we do not set the depth to zero because that would
    //  make any remaining updates that need to be done very slow.
    //
    //  Ignore any errors, since this is just an optimistic perf
    //  optimisation.
    //
    CallS( Param( pinst, JET_paramCheckpointDepthMax )->Set( pinst, ppibNil, 16384, NULL ) );

    pinst->m_fStopJetService = fTrue;
    return JET_errSuccess;
}


JET_ERR JET_API JetStopServiceInstanceEx( _In_ JET_INSTANCE instance, _In_ JET_GRBIT grbit )
{
    ERR     err = JET_errSuccess;
    INST *  pinst;

    OSTrace( JET_tracetagAPI, OSFormat( "Start %s(0x%Ix, 0x%x)", __FUNCTION__, instance, grbit ) );

    //  Validate and retrieve args
    //

    CallR( ErrFindPinst( instance, &pinst ) );

    const JET_GRBIT bitStopServiceAllInternal = 0x1;
    C_ASSERT( JET_bitStopServiceAll != bitStopServiceAllInternal );
    C_ASSERT( JET_bitStopServiceBackgroundUserTasks != bitStopServiceAllInternal );

    //  Currently the only grbit we support canceling is this one ...
    
    const JET_GRBIT mskValidIndependentResumableBits = ( JET_bitStopServiceBackgroundUserTasks |
                                                            JET_bitStopServiceQuiesceCaches );

    //  All the user bits that are supported ...
    
    const JET_GRBIT mskValidIndependentUserBits = ( JET_bitStopServiceBackgroundUserTasks |
                                                    JET_bitStopServiceQuiesceCaches |
                                                    JET_bitStopServiceStopAndEmitLog |
                                                    JET_bitStopServiceResume );
    if ( 0 != ( grbit & ~mskValidIndependentUserBits ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  Process basic state (cancel-stop else stop)
    //

    if ( grbit & JET_bitStopServiceResume )
    {
        //  The client can cancel a specific set of stop service tasks, or everything they
        //  previously stopped.
        //

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

        //  We can't unstop this bit

        if ( grbit & ~mskValidIndependentResumableBits )
        {
            Error( ErrERRCheck( JET_errClientRequestToStopJetService ) );
        }

        //  To check we undo everything we committed to ...

        OnDebug( JET_GRBIT grbitCheck = grbit );

        //  Unquiesce caches if appropriate.

        if ( grbit & JET_bitStopServiceQuiesceCaches )
        {
            //  Stop checkpoint quiescing

            pinst->m_fCheckpointQuiesce = fFalse;


            //  We don't need to call ErrIOUpdateCheckpoints() like we do for quiesce, because it
            //  wouldn't do anything ... so we'll let the user drive the checkpoint back up from
            //  whatever real user load there is.  If the checkpoint is actively trying to quiesce
            //  right now, it will pick it up on the next checkpoint depth maint / which is not
            //  really a problem (the task is quick).

            //  Mark that we've unquiesced caches

            pinst->m_grbitStopped &= (~JET_bitStopServiceQuiesceCaches);
            OnDebug( grbitCheck &= (~JET_bitStopServiceQuiesceCaches) );
        }

        //  Unquiesce background tasks if appropriate.

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

                    pfmpCurr->UnpauseOLD2Tasks();

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
        //  Start stopping the various services the client specified.
        //
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

        //  Process the background services that don't yet have any specific bit tied to them.

        if ( grbit & bitStopServiceAllInternal )
        {
            //  Halt OLD for this instance

            DBMScanStopAllScansForInst( pinst );
            OLDTermInst( pinst );
            OLD2TermInst( pinst );
            PagePatching::TermInst( pinst );

            //  Both done later ...
            //pinst->m_grbitStopped |= bitStopServiceAllInternal;
            //OnDebug( grbitCheck &= ~bitStopServiceAllInternal );
        }

        if ( grbit & JET_bitStopServiceBackgroundUserTasks )
        {
            //  Halt OLDv2/B+ Tree defrag for this instance

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

                    // Pauses execution of OLD tasks but allows
                    // tasks to be registered for future processing
                    pfmpCurr->PauseOLD2Tasks();

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
            //  signal version cleanup to minimise
            //  the amount of work version cleanup
            //  has to do on term
            //
            pinst->m_pver->VERSignalCleanup();

            PIB pibFake;
            pibFake.m_pinst = pinst;
            (void)ErrLGWaitForWrite( &pibFake, &lgposMax );

            //  Set a very small checkpoint depth to cause the database cache
            //  to start flushing all important dirty pages to the database.
            //  Note that we do not set the depth to zero because that would
            //  make any remaining updates that need to be done very slow.
            //
            //  Ignore any errors, since this is just an optimistic perf
            //  optimisation.
            //

            //  We could've implemented this as saving the params, like so:
            //      pinst->fCheckpointParamSetSaved = Param( pinst, JET_paramCheckpointDepthMax )->FWritten();
            //      pinst->ulCheckpointParamSaved = UlParam( pinst, JET_paramCheckpointDepthMax );
            //  Setting the checkpoint
            //      CallS( Param( pinst, JET_paramCheckpointDepthMax )->Set( pinst, ppibNil, JET_cbCheckpointQuiesce, NULL ) );
            //  And then resetting like this
            //      if ( pinst->fCheckpointParamSetSaved )
            //          CallS( Param( pinst, JET_paramCheckpointDepthMax )->Set( pinst, ppibNil, pinst->ulCheckpointParamSaved, NULL ) );
            //      else
            //          CallS( Param( pinst, JET_paramCheckpointDepthMax )->Reset( pinst, UlParamDefault( JET_paramCheckpointDepthMax ) ) );
            //  BUT I was too chicken that I'd get the reset of the params 
            //  right, and it's a little wonky to play with the user's params
            //  like that.  Though it is more truthful, b/c when we set
            //  m_fCheckpointQuiesce we are pretending we have a 0 checkpoint.

            pinst->m_fCheckpointQuiesce = fTrue;

            //  Signal checkpoint maintenance is needed on all attached DBs
            //

            //  Best Effort
            (void)ErrIOUpdateCheckpoints( pinst );

            pinst->m_grbitStopped |= JET_bitStopServiceQuiesceCaches;
            OnDebug( grbitCheck &= ~JET_bitStopServiceQuiesceCaches );
        }

        if ( grbit & bitStopServiceAllInternal )
        {
            //  Never support Sync stop here ... because clients could assume they could 
            //  cleanup the DLL or call JetTerm(), and that will never be true until all
            //  their threads come back b/c the thread could be in the DLL on the last
            //  ret instruction of a function in the DLL.  Clients must always wait for
            //  all their threads.
            
            pinst->m_fStopJetService = fTrue;

            pinst->m_grbitStopped |= bitStopServiceAllInternal;
            OnDebug( grbitCheck &= ~bitStopServiceAllInternal );
        }

        Assert( grbitCheck == 0 );
    }

HandleError:

    return err;
}

JET_ERR JET_API JetStopServiceInstance( _In_ JET_INSTANCE instance )
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


LOCAL JET_ERR JetStopBackupInstanceEx( _In_ JET_INSTANCE instance )
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
JET_ERR JET_API JetStopBackupInstance( _In_ JET_INSTANCE instance )
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

    // use to free buffers returned by JetGetInstanceInfo
    OSMemoryHeapFree( (void* const)pbBuf );
    return JET_errSuccess;
};

JET_ERR JET_API JetFreeBuffer(
    _Pre_notnull_ char * pbBuf )
{
    JET_TRY( opFreeBuffer, JetFreeBufferEx( pbBuf ) );
}



LOCAL JET_ERR JetGetInstanceInfoEx(
    _Out_ ULONG *                                           pcInstanceInfo,
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
        // Replace Assert with a retail check.  Use an existing error path:
        if ( cbTotal < ( m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * ) ) )
        {
            AssertSz( fFalse,"CAutoINSTANCE_INFOW::ErrGet.  Buffer arithmetic validation failure." );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        cbTotal -= m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * );

        ((JET_INSTANCE_INFO_A*)lpvNew)[iInst].szDatabaseDisplayName = (CHAR**)lpvCurrent;
        lpvCurrent += m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * );
        // Replace Assert with a retail check.  Use an existing error path:
        if ( cbTotal < ( m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * ) ) )
        {
            AssertSz( fFalse,"CAutoINSTANCE_INFOW::ErrGet.  Buffer arithmetic validation failure." );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        cbTotal -= m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * );

        ((JET_INSTANCE_INFO_A*)lpvNew)[iInst].szDatabaseSLVFileName_Obsolete = NULL;
        lpvCurrent += m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * );
        // Replace Assert with a retail check.  Use an existing error path:
        if ( cbTotal < ( m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * ) ) )
        {
            AssertSz( fFalse,"CAutoINSTANCE_INFOW::ErrGet.  Buffer arithmetic validation failure." );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        cbTotal -= m_aInstanceInfo[iInst].cDatabases * sizeof( CHAR * );

        if ( m_aInstanceInfo[iInst].szInstanceName )
        {
            Call( ErrOSSTRUnicodeToAscii( m_aInstanceInfo[iInst].szInstanceName, szCurrent, cbTotal / sizeof( CHAR ), &cchPartial ) );
            // Replace Assert with a retail check.  Use an existing error path:
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
            // Replace Assert with a retail check.  Use an existing error path:
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
                // Replace Assert with a retail check.  Use an existing error path:
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
    _Out_ ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo )
{
    ERR                     err             = JET_errSuccess;
    JET_INSTANCE_INFO_W*    laInstanceInfoW = NULL;
    ULONG           lcInstanceInfo  = 0;
    CAutoINSTANCE_INFOW autoInstanceInfoW;

    Call( JetGetInstanceInfoEx( &lcInstanceInfo, &laInstanceInfoW ) );

    Call( autoInstanceInfoW.ErrSet( lcInstanceInfo, laInstanceInfoW ) );
    // we moved the "ownership" of the memory to the auto object
    // so we need to count for this
    //
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
    _Out_ ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo )
{
    JET_TRY( opGetInstanceInfo, JetGetInstanceInfoExA( pcInstanceInfo, paInstanceInfo ) );
}

JET_ERR JET_API JetGetInstanceInfoW(
    _Out_ ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo )
{
    JET_TRY( opGetInstanceInfo, JetGetInstanceInfoEx( pcInstanceInfo, paInstanceInfo ) );
}

LOCAL JET_ERR JetOSSnapshotPrepareEx(
    _Out_ JET_OSSNAPID *    psnapId,
    _In_ const JET_GRBIT    grbit )
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
    _Out_ JET_OSSNAPID *    psnapId,
    _In_ const JET_GRBIT    grbit )
{
    JET_TRY( opOSSnapshotPrepare, JetOSSnapshotPrepareEx( psnapId, grbit ) );
}


LOCAL JET_ERR JetOSSnapshotFreezeEx(
    _In_ const JET_OSSNAPID                                         snapId,
    _Out_ ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo,
    _In_ const JET_GRBIT                                            grbit )
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
    _In_ const JET_OSSNAPID                                         snapId,
    _Out_ ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo,
    _In_ const JET_GRBIT                                            grbit )
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
    // we moved the "ownership" of the memory to the auto object
    // so we need to count for this
    //
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
    _In_ const JET_OSSNAPID                                         snapId,
    _Out_ ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo,
    _In_ const JET_GRBIT                                            grbit )
{
    JET_TRY( opOSSnapshotFreeze, JetOSSnapshotFreezeExA( snapId, pcInstanceInfo, paInstanceInfo, grbit ) );
}

JET_ERR JET_API JetOSSnapshotFreezeW(
    _In_ const JET_OSSNAPID                                         snapId,
    _Out_ ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo,
    _In_ const JET_GRBIT                                            grbit )
{
    JET_TRY( opOSSnapshotFreeze, JetOSSnapshotFreezeEx( snapId, pcInstanceInfo, paInstanceInfo, grbit ) );
}

LOCAL JET_ERR JetOSSnapshotThawEx( _In_ const JET_OSSNAPID snapId, _In_ const JET_GRBIT grbit )
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

JET_ERR JET_API JetOSSnapshotThaw( _In_ const JET_OSSNAPID snapId, _In_ const JET_GRBIT grbit )
{
    JET_TRY( opOSSnapshotThaw, JetOSSnapshotThawEx( snapId, grbit ) );
}


LOCAL JET_ERR JetOSSnapshotAbortEx( _In_ const JET_OSSNAPID snapId, _In_ const JET_GRBIT grbit )
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

JET_ERR JET_API JetOSSnapshotAbort( _In_ const JET_OSSNAPID snapId, _In_ const JET_GRBIT grbit )
{
    JET_TRY( opOSSnapshotAbort, JetOSSnapshotAbortEx( snapId, grbit ) );
}

INLINE JET_ERR JetOSSnapshotPrepareInstanceEx( _In_ JET_OSSNAPID snapId, _In_ const JET_INSTANCE instance, _In_ const JET_GRBIT grbit )
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

JET_ERR JET_API JetOSSnapshotPrepareInstance( _In_ JET_OSSNAPID snapId, _In_ const JET_INSTANCE instance, _In_ const JET_GRBIT grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opOSSnapPrepareInstance, JetOSSnapshotPrepareInstanceEx( snapId, instance, grbit ) );
}



INLINE JET_ERR JetOSSnapshotTruncateLogEx( _In_ const JET_OSSNAPID snapId, _In_ const JET_GRBIT grbit )
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

JET_ERR JET_API JetOSSnapshotTruncateLog( _In_ const JET_OSSNAPID snapId, _In_ const JET_GRBIT grbit )
{
    JET_TRY( opOSSnapshotTruncateLog, JetOSSnapshotTruncateLogEx( snapId, grbit ) );
}

INLINE JET_ERR JetOSSnapshotTruncateLogInstanceEx( _In_ const JET_OSSNAPID snapId, _In_ const JET_INSTANCE instance, _In_ const JET_GRBIT grbit )
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

JET_ERR JET_API JetOSSnapshotTruncateLogInstance( _In_ const JET_OSSNAPID snapId, _In_ const JET_INSTANCE instance, _In_ const JET_GRBIT grbit )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opOSSnapTruncateLogInstance, JetOSSnapshotTruncateLogInstanceEx( snapId, instance, grbit ) );
}

INLINE JET_ERR JetOSSnapshotGetFreezeInfoEx(
    _In_ const JET_OSSNAPID                                         snapId,
    _Out_ ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo,
    _In_ const JET_GRBIT                                            grbit )
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
    _In_ const JET_OSSNAPID                                         snapId,
    _Out_ ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo,
    _In_ const JET_GRBIT                                            grbit )
{
    ERR                     err             = JET_errSuccess;
    JET_INSTANCE_INFO_W *   laInstanceInfoW     = NULL;
    ULONG           lcInstanceInfo  = 0;
    CAutoINSTANCE_INFOW     autoInstanceInfoW;

    Call( JetOSSnapshotGetFreezeInfoEx( snapId, &lcInstanceInfo, &laInstanceInfoW, grbit ) );

    Call( autoInstanceInfoW.ErrSet( lcInstanceInfo, laInstanceInfoW ) );
    // we moved the "ownership" of the memory to the auto object
    // so we need to count for this
    //
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
    _In_ const JET_OSSNAPID                                         snapId,
    _Out_ ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo,
    _In_ const JET_GRBIT                                            grbit )
{
    JET_TRY( opOSSnapshotGetFreezeInfo, JetOSSnapshotGetFreezeInfoExA( snapId, pcInstanceInfo, paInstanceInfo, grbit ) );
}

JET_ERR JET_API JetOSSnapshotGetFreezeInfoW(
    _In_ const JET_OSSNAPID                                         snapId,
    _Out_ ULONG *                                           pcInstanceInfo,
    __deref_out_ecount( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo,
    _In_ const JET_GRBIT                                            grbit )
{
    JET_TRY( opOSSnapshotGetFreezeInfo, JetOSSnapshotGetFreezeInfoEx( snapId, pcInstanceInfo, paInstanceInfo, grbit ) );
}


INLINE JET_ERR JetOSSnapshotEndEx( _In_ const JET_OSSNAPID snapId, _In_ const JET_GRBIT grbit )
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

JET_ERR JET_API JetOSSnapshotEnd( _In_ const JET_OSSNAPID snapId, _In_ const JET_GRBIT grbit )
{
    JET_TRY( opOSSnapshotEnd, JetOSSnapshotEndEx( snapId, grbit ) );
}


JET_ERR JetConfigureProcessForCrashDumpEx( _In_ const JET_GRBIT grbit )
{
    JET_ERR err = JET_errSuccess;

    //  validate grbit

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

    //  propagate the selection of DumpMinimum or DumpMaximum to each sub function
    //
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

JET_ERR JET_API JetConfigureProcessForCrashDump( _In_ const JET_GRBIT grbit )
{
    JET_ERR err = JET_errSuccess;

    //  eat any exception that occurs to avoid possible infinite recursion
    //  with the unhandled exception filter while configuring the process for a
    //  crash dump possibly resulting from a crash
    //
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

    //  Actually corrupt the page, by recursing into corrupt by page image (now that we have one)
    corruptPageImage.grbit = ( (~JET_bitTestHookCorruptDatabaseFile) & pcorruptDatabaseFile->grbit ) | JET_bitTestHookCorruptDatabasePageImage;
    corruptPageImage.CorruptDatabasePageImage.pbPageImageTarget = (JET_API_PTR)pbPageImage;
    corruptPageImage.CorruptDatabasePageImage.cbPageImage = cbPageSize;
    corruptPageImage.CorruptDatabasePageImage.pgnoTarget = pcorruptDatabaseFile->CorruptDatabaseFile.pgnoTarget;
    corruptPageImage.CorruptDatabasePageImage.iSubTarget = pcorruptDatabaseFile->CorruptDatabaseFile.iSubTarget;
    Call( JetTestHook( opTestHookCorrupt, &corruptPageImage ) );

    //  Write the page out
    Call( pfapi->ErrIOWrite( *TraceContextScope( iorpCorruptionTestHook ), OffsetOfPgno( (PGNO)pcorruptDatabaseFile->CorruptDatabaseFile.pgnoTarget ), cbPageSize, pbPageImage, qosIONormal ) );

HandleError:

    //  Cleanup
    if ( pbPageImage != NULL )
    {
        OSMemoryPageFree( pbPageImage );
    }

    if ( pfapi )
    {
        (void)ErrUtilFlushFileBuffers( pfapi, iofrUtility );
        delete pfapi;
    }
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

    CallR( ErrOSUInit() );
    Alloc( pdbfilehdr = (DBFILEHDR*)PvOSMemoryPageAlloc( g_cbPageMax, NULL ) );
    Call( ErrOSFSCreate( g_pfsconfigGlobal, &pfsapi ) );

    Call( pfsapi->ErrFileOpen( palterdbfilehdr->szDatabase, IFileAPI::fmfNone, &pfapiDatabase ) );
    Call( ErrUtilReadShadowedHeader( pinstNil, pfsapi, pfapiDatabase, (BYTE*)pdbfilehdr, (DWORD)g_cbPageMax, (LONG)OffsetOf( DBFILEHDR_FIX, le_cbPageSize ), urhfReadOnly|urhfNoFailOnPageMismatch, &cbPageSize, &shs ) );
    Call( CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( palterdbfilehdr->szDatabase, pdbfilehdr, pinstNil, &pfm ) );


    //  Must be after ErrUtilReadShadowedHeader() so we have page size.
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

    DWORD dwPrev = FNegTestSet( fInvalidUsage | fCorruptingDbHeaders );  // checks both for extra checking
    Assert( ( dwPrev & ( fInvalidUsage | fCorruptingDbHeaders ) ) == 0 ); // otherwise unset below will kill clients intended overrides
    Call( ErrUtilWriteUnattachedDatabaseHeaders( pinstNil, pfsapi, palterdbfilehdr->szDatabase, pdbfilehdr, pfapiDatabase, pfm ) );
    FNegTestUnset( fInvalidUsage | fCorruptingDbHeaders );
    if ( pfm )
    {
        pfm->TermFlushMap();
    }

HandleError:

    FNegTestUnset( fCorruptingDbHeaders );
    FNegTestUnset( fInvalidUsage );

    if ( pfapiDatabase )
    {
        (void)ErrUtilFlushFileBuffers( pfapiDatabase, iofrUtility ); // test only, no error handling necessary
    }
    OSMemoryPageFree( pdbfilehdr );
    delete pfm;
    delete pfapiDatabase;
    delete pfsapi;
    OSUTerm();

    return err;
}

JET_ERR JET_API JetTestHook(
    _In_        const TESTHOOK_OP   opcode,
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

#else // ! ENABLE_JET_UNIT_TEST
        case opTestHookUnitTests:
        case opTestHookUnitTests2:
            err = ErrERRCheck( JET_errDisabledFunctionality );
            break;

#endif // ! ENABLE_JET_UNIT_TEST
        case opTestHookTestInjection:
        {
            const JET_TESTHOOKTESTINJECTION* const pParams = (JET_TESTHOOKTESTINJECTION*)pv;
            if ( pParams == NULL || pParams->cbStruct != sizeof(JET_TESTHOOKTESTINJECTION) )
            {
                Call( ErrERRCheck( JET_errInvalidParameter ) );
            }

            //  first we process line IDs for any test hooks that aren't built on the OS layer

            if ( pParams->ulID == 53984 /* for g_bflruk / ResMgr */ )
            {
                if ( pParams->type != JET_TestInjectFault ||
                        pParams->grbit != JET_bitInjectionProbabilityPct ||
                        pParams->ulProbability != 5 /* b/c that's what g_bflruk is using, move along SOMEONE */ )
                {
                    //
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
            #endif // RTM
        }
            break;
        case opTestHookResetNegativeTesting:
        {
            #ifndef RTM
                const DWORD dwNegativeTest  = *(DWORD * )pv;
                FNegTestUnset( dwNegativeTest );
            #endif // RTM
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

            //  Retrieving this needs a critical section and we only try-acquiring it.
            //  This is just a test hook, so retry forever while lgposMin is returned.
            
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


            Assert( cbCheckpointDepth > -( 4096 * 64 * 1024 ) /* new 4k-segment-based max log file size */ );
            *((__int64*)pv) = max( cbCheckpointDepth, 0 );
        }
            break;

        case opTestHookGetOLD2Status:
        {
            INST * pinst = *(INST**)pv;
            Assert( pinst );
            Assert( pinst->m_mpdbidifmp[dbidUserLeast] != ifmpNil );
            *((__int64*)pv) = (__int64)( g_rgfmp[pinst->m_mpdbidifmp[dbidUserLeast]].FOLD2TasksPaused() );
        }
            break;

        case opTestHookGetEngineTickNow:
            //  consider making 0, 1, and 2 to get separately the LRT, MRT, HRT, and maybe datetime.
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
                //  This is only used for our resmgrenginetest.exe, and it at worst case loads 100k buffers, at
                //  a 20 unique touches / sec rate ... this is 5 M ticks cache lifetime ... anything beyond that
                //  is a break of our target ...
                g_bflruk.SetTimeBar( 90 * 60 * 1000 /* lifetime = 90 min | 5,400,000 */, pthtimeinj->tickNow + 10 * 60 * 60 * 1000 /* +10 hrs */ );
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
            Assert( pcorrupt->grbit );  //  must have some corruption options
            Assert( pcorrupt->cbStruct >= ( OffsetOf( JET_TESTHOOKCORRUPT, grbit ) + sizeof( ULONG ) ) );

            // validation of API constraints (_not_ args processing)
            C_ASSERT( 0 == ( JET_mskTestHookCorruptFileType & JET_mskTestHookCorruptDataType ) );
            C_ASSERT( 0 == ( JET_mskTestHookCorruptFileType & JET_mskTestHookCorruptSpecific ) );
            C_ASSERT( 0 == ( JET_mskTestHookCorruptDataType & JET_mskTestHookCorruptSpecific ) );
            C_ASSERT( 0 == ( ( JET_mskTestHookCorruptFileType |
                                JET_mskTestHookCorruptDataType |
                                JET_mskTestHookCorruptSpecific ) &
                                JET_bitTestHookCorruptLeaveChecksum ) );


            if ( pcorrupt->grbit & JET_bitTestHookCorruptDatabaseFile )
            {
                //  Note this recurses back into JetTestHook( opTestHookCorrupt w/ the JET-bitTestHookCorruptDatabasePageImage option )
                Call( ErrTestHookCorruptOfflineFile( pcorrupt ) );
            }
            else if ( pcorrupt->grbit & JET_bitTestHookCorruptDatabasePageImage )
            {
                AssertSz( pcorrupt->CorruptDatabasePageImage.pgnoTarget != JET_pgnoTestHookCorruptRandom, "NYI" );

                //  Load the Page.
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

                //  Corrupt the Page
                //

                if ( pcorrupt->grbit & JET_bitTestHookCorruptPageSingleFld )
                {
                    //  Help the client out ...
                    (void)FNegTestSet( fCorruptingPageLogically );

                    //  Corrupt the Page, by tweaking a byte randomly in the page.
                    const INT iline = rand() % cpage.Clines();
                    LINE line;
                    cpage.GetPtr( iline, &line );
                    // note: that CPAGE::ErrCheckPage() is too smart in dbscan and notices if we set
                    // the prefix larger than the pages' prefix node.  So avoid corrupting the prefix
                    // cb (which is the first 2 bytes of the page).
                    Assert( line.cb > 2 );  // what would this be?
                    const INT ib = 2 + ( rand() % ( line.cb - 2 ) );
                    ((BYTE*)line.pv)[ib] = 0x42 ^ ((BYTE*)line.pv)[ib];
                }

                if ( pcorrupt->grbit & JET_bitTestHookCorruptPageRemoveNode )
                {
                    //  Help the client out ...
                    (void)FNegTestSet( fCorruptingPageLogically );

                    AssertSz( fFalse, "NYI - caused problems if there is only 1 line on the page." );

                    //  Corrupt the Page, by removing a random line
                    const INT iline = rand() % cpage.Clines();
                    cpage.Delete( iline );
                }

                if ( pcorrupt->grbit & JET_bitTestHookCorruptPageDbtimeDelta )
                {
                    Assert( pcorrupt->CorruptDatabasePageImage.iSubTarget );    // or else this would do nothing.

                    //  Help the client out ...

                    (void)FNegTestSet( fCorruptingWithLostFlush );

                    //  we are about to use the SetDbtime() in an invalid way, indicate that.
                    const bool fPreviouslySet = FNegTestSet( fInvalidUsage );

                    //  Corrupt the Page, by shifting the DBTIME by the specified value

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
                    //  we are about to use the SetDbtime() in an invalid way, indicate that.
                    const bool fPreviouslySet = FNegTestSet( fInvalidUsage );

                    //  Fix the checksum up for writing.  Evil.  Buwahahaha.
                    cpage.PreparePageForWrite( cpage.Pgft() );

                    if ( !fPreviouslySet )
                    {
                        FNegTestUnset( fInvalidUsage );
                    }
                }

                //  Specifically these grbits processed at the end (so it doesn't foul previous 
                //  grbit processing which may require the page to be intact) ...
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

            // Sanitize the in param so it can be reused
            // as out param
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
    _In_    JET_INSTANCE        instance,
    _In_    JET_EMITDATACTX *   pEmitLogDataCtx,
    _In_    void *              pvLogData,
    _In_    ULONG       cbLogData,
    _In_    JET_GRBIT           grbits  )
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

    //  Validate the arguments to the function

    if ( pEmitLogDataCtx == NULL )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( 0 != grbits )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    //  Validate the emit data context ...

    if ( pEmitLogDataCtx->cbStruct != sizeof(JET_EMITDATACTX) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    BOOL fAPIEntered = fFalse;

    //  We can log data during both init and term, so we must be prepared to enter the API in 
    //  any state.

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
    _In_    JET_INSTANCE        instance,
    _In_    JET_EMITDATACTX *   pEmitLogDataCtx,
    _In_    void *              pvLogData,
    _In_    ULONG       cbLogData,
    _In_    JET_GRBIT           grbits  )
{
    JET_VALIDATE_INSTANCE( instance );
    JET_TRY( opConsumeLogData, JetConsumeLogDataEx( instance, pEmitLogDataCtx, pvLogData, cbLogData, grbits ) );
}



LOCAL JET_ERR JET_API JetGetErrorInfoExW(
    __in_opt void *                 pvContext,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel,
    _In_ JET_GRBIT                  grbit )
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

    // Parameter validation.
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

    // No supported grbits yet.
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

                // In debug this is overwritten by ProbeClientBuffer.
                perrinfobasic->cbStruct = cbMin;

                // Check for the size of the structure.
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
    

// This does not exist.
#if 0
JET_ERR JET_API JetGetErrorInfoA(
    __in_opt void *                 pvContext,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              ulInfoLevel,
    _In_ JET_GRBIT                  grbit );
#endif

JET_ERR JET_API JetGetErrorInfoW(
    __in_opt void *                 pvContext,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel,
    _In_ JET_GRBIT                  grbit )
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
    // specifically not using APICALL_SESID/FEnter( sesid ) in order to facilitate a system control 
    // thread being able to do session pool monitoring, even though it doesn't own the sesssion.
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
    _In_ JET_SESID                          sesid,
    _In_ JET_DBID                           dbid,
    __in_ecount( cwszTables ) PCWSTR *      rgwszTables,
    _In_ INT                                cwszTables,
    _In_ JET_GRBIT                          grbit )
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
    _In_ JET_SESID                          sesid,
    _In_ JET_DBID                           dbid,
    __in_ecount( cwszTables ) JET_PCWSTR *  rgwszTables,
    _In_ LONG                               cwszTables,
    _In_ JET_GRBIT                          grbit )
{
    JET_VALIDATE_SESID( sesid );
    JET_TRY( opPrereadTables, JetPrereadTablesEx( sesid, dbid, rgwszTables, cwszTables, grbit ) );
}

LOCAL JET_ERR JetPrereadIndexRangeEx(
    _In_ const JET_SESID                sesid,
    _In_ const JET_TABLEID              tableid,
    _In_ const JET_INDEX_RANGE * const  pIndexRange,
    _In_ const ULONG            cPageCacheMin,
    _In_ const ULONG            cPageCacheMax,
    _In_ const JET_GRBIT                grbit,
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
    _In_ const JET_SESID                sesid,
    _In_ const JET_TABLEID              tableid,
    _In_ const JET_INDEX_RANGE * const  pIndexRange,
    _In_ const ULONG            cPageCacheMin,
    _In_ const ULONG            cPageCacheMax,
    _In_ const JET_GRBIT                grbit,
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
    _In_ JET_PCWSTR                 wszRBSFileName,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG                      cbMax,
    _In_ ULONG                      InfoLevel )
{
    ERR             err                 = JET_errSuccess;
    RBSFILEHDR*     prbsfilehdr         = NULL;
    IFileSystemAPI* pfsapi              = NULL;
    IFileAPI*       pfapi               = NULL;
    BOOL            fOSUInitCalled      = fFalse;

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

    g_rwlOSUInit.EnterAsReader();
    Call( ErrOSUInit() );
    fOSUInitCalled = fTrue;

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

    if ( fOSUInitCalled )
    {
        OSUTerm();
    }
    g_rwlOSUInit.LeaveAsReader();

    return err;
}

} // extern "C"

LOCAL JET_ERR JetGetRBSFileInfoExA(
    _In_ JET_PCSTR                  szRBSFileName,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    ERR             err;
    CAutoWSZPATH    lszRBSFileName;

    CallR( lszRBSFileName.ErrSet( szRBSFileName ) );

    return JetGetRBSFileInfoEx( lszRBSFileName, pvResult, cbMax, InfoLevel );
}

JET_ERR JET_API JetGetRBSFileInfoA(
    _In_ JET_PCSTR                  szRBSFileName,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_TRY( opGetDatabaseFileInfo, JetGetRBSFileInfoExA( szRBSFileName, pvResult, cbMax, InfoLevel ) );
}

JET_ERR JET_API JetGetRBSFileInfoW(
    _In_ JET_PCWSTR                 wszRBSFileName,
    __out_bcount( cbMax ) void *    pvResult,
    _In_ ULONG              cbMax,
    _In_ ULONG              InfoLevel )
{
    JET_TRY( opGetDatabaseFileInfo, JetGetRBSFileInfoEx( wszRBSFileName, pvResult, cbMax, InfoLevel ) );
}

LOCAL
JET_ERR
JetRBSPrepareRevertEx(
    _In_    JET_INSTANCE    instance,
    _In_    JET_LOGTIME     jltRevertExpected,
    _In_    CPG             cpgCache,
    _In_    JET_GRBIT       grbit,
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
    _In_    JET_INSTANCE    instance,
    _In_    JET_LOGTIME     jltRevertExpected,
    _In_    CPG             cpgCache,
    _In_    JET_GRBIT       grbit,
    _Out_   JET_LOGTIME*    pjltRevertActual )
{
    JET_TRY( opRBSPrepareRevert, JetRBSPrepareRevertEx( instance, jltRevertExpected, cpgCache, grbit, pjltRevertActual ) );
}

LOCAL
JET_ERR
JetRBSExecuteRevertEx(
    _In_    JET_INSTANCE            instance,
    _In_    JET_GRBIT               grbit,
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
    _In_    JET_INSTANCE            instance,
    _In_    JET_GRBIT               grbit,
    _Out_   JET_RBSREVERTINFOMISC*  prbsrevertinfomisc )
{
    JET_TRY( opRBSExecuteRevert, JetRBSExecuteRevertEx( instance, grbit, prbsrevertinfomisc ) );
}

LOCAL
JET_ERR
JetRBSCancelRevertEx(
    _In_    JET_INSTANCE    instance )
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
    _In_    JET_INSTANCE    instance )
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

    //  ignore torn ThreadWaitBegin/ThreadWaitEnd pairs caused by init of callback while holding locks
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

// Defined in fcreate.cxx
ERR ErrIDXCheckUnicodeFlagAndDefn(
    __in_ecount( cIndexCreate ) JET_INDEXCREATE3_A * pindexcreate,
    _In_ ULONG                               cIndexCreate );

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
        // These union members are larger, so we need to ensure that the upper 32bits are clear.
        rgindexcreateold[i].pidxunicode = NULL;
        rgindexcreateold[i].ptuplelimits = NULL;

        rgindexcreateold[i].szIndexName = rgszIndexName[i];
        rgindexcreateold[i].szKey = rgszIndexKey[i];
        OSStrCbFormatA( rgindexcreateold[i].szIndexName, JET_cbNameMost, "Index-%d", i);
        OSStrCbFormatA( rgindexcreateold[i].szKey, JET_cbNameMost, "+col%2.2d\0", i);
        rgindexcreateold[i].cbKey = 8;
    }

    const LANGID    langidFrenchCanadian    = (0x03 << 10 ) | 0x0c; // MAKELANGID( LANG_FRENCH, SUBLANG_FRENCH_CANADIAN );
    const LCID      lcidFrenchCanadian      = 0 | langidFrenchCanadian; // MAKELCID( langidFrenchCanadian, SORT_DEFAULT );

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

    CHECK( 0 == LOSStrCompareW( rgindexcreatenew[1].pidxunicode->szLocaleName, L"fr-CA" ) );
    CHECK( 0x10004 == rgindexcreatenew[1].pidxunicode->dwMapFlags );

    CHECK( 0 == LOSStrCompareW( rgindexcreatenew[2].pidxunicode->szLocaleName, L"fr-CA" ) );
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

        // New grbit values:
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

#endif // ENABLE_JET_UNIT_TEST
