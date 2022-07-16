// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//
//
// Space Manager
//
//
//  Responsible for keeping track of who (what B+ Trees) own space, where free space
//  is left, policies on who gets what free space, etc.  Even the owned and free space
//  is tracked in B+ Trees, creating some interesting recursive challenges.
//
//  Space is managed hierarchically, with the DBROOT owning all space, and initially
//  owning any and all available (i.e. free space).  When the first table is created
//  some space is subtracted from the database available space pool and then tracked
//  as space that table owns.  Any space not immediately used by the created table
//  (and indices) would be in that tables available / free space list (and NOT in
//  the DBROOT's free space list).  And similarly for indices, they take free space
//  from the table, and "own it", and track any free space within the chunk of space
//  taken from the tables avail.
//
//  Space is represented in the space trees in the form of extents. An extent is a
//  (pgno,cpg) pair, where pgno is the last page of the extent and cpg is the number
//  of pages represented by the extent.
//
//  An FDP (Father Data Page) is a special page (although it has all the normal 
//  page characteristics, checksum, flags, nodes) in that it is the root / top page of
//  a B+ Tree managed by ESE.
//
//  There are 4 special (as in hard coded, and required for bootstrapping) pages 
//  in the database:
//      Page 1:  pgnoSystemRoot              - The System FDP (not a B+ Tree root page, doesn't really hold anything)
//      Page 2:  SPH(1).PgnoOE()             - DbRoot Owned Ext FDP
//      Page 3:  Previous PgnoOE+1           - DbRoot Avail Ext FDP 
//      Page 4:  pgnoFDPMSO                  - The ESE Catalog FDP
//
//
//  Additionally, these pages use hard coded values by cat.cxx in engine during init, but technically are not
//  required for bootstrapping:
//      Page 7:  pgnoFDPMSO_NameIndex        - The FDP of the ESE Catalog index off the object name
//      Page 10: pgnoFDPMSO_RootObjectIndex  - The FDP of the ESE Catalog index off the root flag
//      Page 24: pgnoFDPMSOShadow            - The ESE Shadow Catalog FDP
//

//
//
// Threading/Latching
//
//
//   Updates to space trees are not versioned and do not participate in transactions.
//   They cannot be rolled back.  They are done immediately and are visible to all threads.
//   When actually updating a space tree, calls must be made to:
//   * SPIWrappedNDInsert()
//   * ErrSPIWrappedNDSetExternalHeader()
//   * ErrSPIWrappedBTFlagDelete()
//   * ErrSPIWrappedBTReplace()
//   * ErrSPIWrappedBTInsert
//
//   A write or riw latch on the root page of the tree that owns a space tree is used to
//   make space tree operations thread safe, and such a latch must be held when calling
//   the *SPIWrapped* functions.  Also, when calling these functions, one must NOT
//   hold a similar latch on the parent of the tree being updated.  For example, for a table
//   whose parent is the DBRoot, before you update the AE tree for the table, you must have
//   a write latch on the root page of the table and must NOT have a write latch on the root
//   of the DB.  This restriction on the parent latch is most obvious when releasing pages 
//   from a tree to its parent.  You must hold the latch on the table as you remove the
//   extent from the space tree for the table, but must not hold the latch on the parent
//   until after you've removed the pages from the child.  Similarly, when taking pages from
//   a parent to give to a child, you must hold the latch on the parent when you take pages
//   from it (whether you hold the latch on the child at this point is unrestricted), but you
//   must release the latch on the parent and hold a latch on the child before you add the
//   extent to the child.  The same restrictions apply during split buffer and space tree
//   manipulations that need to get new extents from the parent for use in the space trees
//   themselves.  Such operations occur intermingled with the "outer" operation that is
//   causing the space tree change in the first place.
//
//   A design pattern is to never have pfucbParent latched when you pass both pfucb and
//   pfucbParent to a routine.  PfucbParent is passed largely to avoid conditions such
//   as repeatedly opening and closing a parent FUCB inside a loop, not as a carrier of
//   a latch that provides thread safety.  All routines that accept pfucbParent should
//   call AssertSPIPfucbNullOrUnlatched(pfucbParent) to validate this.  When a routine
//   does take a latch on pfucbParent, it should be very careful to release the latch
//   before returning.
//
//   The restriction to only update space trees via the *SPIWrapped* functions is to
//   guarantee that the Extent Page Count Cache is kept up to date.
//
//   The restriction to not hold a latch on the parent of a tree is to avoid deadlocks in
//   the Extent Page Count Cache.
//
//   Some exceptions to the locking requirements are made during repair, recovery, and 
//   when a database is open exclusively by one session (e.g. shrink makes some exceptions).
//   AssertSPIWrappedLatches() makes a unified and complicated set of assertions for all
//   *SPIWrapped* functions.

//
// Pools
//
//
//  The Space Manager also supports the concept of pools. More details about how they are
//  implemented and represented can be found under "Space Manager EXTENT data structures"
//  below in this file, but they are essentially a way to annotate page extents which are
//  used for special usage or representation purposes. Examples of such pools are the
//  the available continuous pool (used to reserve space which the client indicated performs
//  better if its pages are kept contiguous to one another) and the shelved page pool (space
//  which is beyond the EOF and potentially owned by a B+ Tree, but guaranteed not to contain
//  data). Since shelved pages are not just an optimization, we'll describe them a little
//  further below.
//
//  Shelved pages are created during a DB Shrink pass when the page categorization algorithm
//  cannot conclusively determine which object (B+ Tree) owns the page. In this situation,
//  we wouldn't normally be able to move the page and would have halted the pass, but because
//  the categorization algorithm guarantees that such a page cannot contain reachable data
//  (i.e., data that belongs to a consistent tree and can be reached via tree navigation),
//  it's possible to shrink the file below that page without the risk of losing data. These
//  pages are either 1) leaked at the DB root level (i.e., not owned by any B+ Tree and not
//  part of any B+ Tree); 2) leaked at the table level or below (i.e., owned by a B+ Tree,
//  but neither part of any B+ Tree, nor available); 3) available space at the table level
//  or below (owned by a B+ Tree and either part of split buffers or regular available space).
//  The DB root's split buffers and available space tree are always searched for by the
//  categorization algorithm, so type 3 above never belongs to the DB root. Because these
//  unused pages might be available to a B+ Tree, we must represent them in our space
//  management structures because if the file grows back past them, they must not be made
//  available as this would mean they could be given out to a different B+ Tree.
//
//  Example of lifecycle of a Type 1 shelved page (leaked from the root level):
//    1- Last pgno of the DB is 120. Pgno 100 is leaked at the root level.
//    2- DB Shrink adds pgno 100 as shelved and truncates the file down to pgno 80.
//    3- The shelved page is never expected to be touched or used because it's not
//       owned by any B+ Tree.
//    4- The DB re-grows back to pgno 110. Pgno 100 is then unshelved, but not made
//       available, i.e., will not be part of the newly added available extents upon
//       file re-growth.
//
//  Example of lifecycle of a Type 2 shelved page (leaked from the table level or below):
//    1- Last pgno of the DB is 120. Pgno 100 is owned by a table, but not part of any
//       B+ Tree and not available.
//    2- DB Shrink adds pgno 100 as shelved and truncates the file down to pgno 80.
//       Note that the table still has pgno 100 represented in one of its owned
//       extents.
//    3- The shelved page is never expected to be touched or used because it's not
//       part of any B+ Tree nor it is available.
//    4-
//        4.a- The DB re-grows back to pgno 110. Pgno 100 is then unshelved, but not made
//             available, i.e., will not be part of the newly added available extents upon
//             file re-growth. Though it's going to continue to be a leaked page from the
//             the table's perspective.
//
//        - OR -
//
//        4.b- The table gets deleted. Pgno 100 is then unshelved so it will be made
//             available by the DB root when the DB re-grows.
//
//  Example of lifecycle of a Type 3 shelved page (available at the table level or below):
//    1- Last pgno of the DB is 120. Pgno 100 is owned by a table, but not part of any
//       B+ Tree, though it is available as a split buffer page or as regular available
//       space.
//    2- DB Shrink adds pgno 100 as shelved and truncates the file down to pgno 80.
//       Note that the table still has pgno 100 represented in one of its owned
//       extents and in its split buffers or AE tree.
//    3- If space is needed for a space tree operation (which would consume pages from the
//       split buffer) or a regular tree split or move (which would consume pages from the
//       AE directly), we might come across the shelved pgno 100. If that happens, we will
//       immediately return errSPNoSpaceForYou, which signals upper layers to continue
//       looking for space. Ideally, we would also try to coalesce and release that space
//       so that we avoid bumping into it.
//       We'll leave that for a later iteration of the feature. We don't expect shelved
//       pages to be plentiful, so the performance penalty caused by the lookup retry is
//       expected to be negligible.
//    4-
//        4.a- The DB re-grows back to pgno 110. Pgno 100 is then unshelved, but not made
//             available, i.e., will not be part of the newly added available extents upon
//             file re-growth, but it is still part of the split buffrs or AE tree, so it
//             could be allocated by the table to be used in a B+ Tree.
//
//        - OR -
//
//        4.b- The table gets deleted. Pgno 100 is then unshelved so it will be made
//             available by the DB root when the DB re-grows.
//
//  An extent marked as shelved might exist below the EOF. This can happen if there is a crash
//  during DB Shrink, between the page being shelved an the subsequent reduction of owned
//  space (and file truncation). A shelved extent which is below the EOF is innocuous for
//  subsequent space operations in general, but must be removed before a new DB Shrink pass
//  is attempted, or we could end up with duplicate shelving during the new pass.
//


//
//
//
//
//  Overview of the big functions.
//
//      ErrSPGetExt/ErrSPIGetExt
//
//          This function gets an extent of space of a determined cpg.
//
//      ErrSPGetPage
//
//          This function gets a page from the specified pool.
//
//      ErrSPIGetSe
//
//          This function gets a secondary extent (i.e. more space) for itself from
//          its parent or DB root.
//

#include "std.hxx"
#include "_space.hxx"

#ifdef PERFMON_SUPPORT

PERFInstanceLiveTotal<> cSPPagesTrimmed;
LONG LSPPagesTrimmedCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cSPPagesTrimmed.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotal<> cSPPagesNotTrimmedUnalignedPage;
LONG LSPPagesNotTrimmedUnalignedPageCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cSPPagesNotTrimmedUnalignedPage.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotal<> cSPDeletedTrees;
LONG LSPDeletedTreesCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cSPDeletedTrees.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotal<> cSPDeletedTreeFreedPages;
LONG LSPDeletedTreeFreedPagesCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cSPDeletedTreeFreedPages.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotal<> cSPDeletedTreeSnapshottedPages;
LONG LSPDeletedTreeSnapshottedPagesCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cSPDeletedTreeSnapshottedPages.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotal<> cSPDeletedTreeFreedExtents;
LONG LSPDeletedTreeFreedExtentsCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cSPDeletedTreeFreedExtents.PassTo( iInstance, pvBuf );
    return 0;
}

#endif // PERFMON_SUPPORT


#include "_bt.hxx"

const CHAR * SzNameOfTable( const FUCB * const pfucb )
{
    if( pfucb->u.pfcb->FTypeTable() )
    {
        return pfucb->u.pfcb->Ptdb()->SzTableName();
    }
    else if( pfucb->u.pfcb->FTypeLV() )
    {
        return pfucb->u.pfcb->PfcbTable()->Ptdb()->SzTableName();
    }
    else if( pfucb->u.pfcb->FTypeSecondaryIndex() )
    {
        return pfucb->u.pfcb->PfcbTable()->Ptdb()->SzTableName();
///     return pfucb->u.pfcb->PfcbTable()->Ptdb()->SzIndexName(
///                 pfucb->u.pfcb->Pidb()->ItagIndexName(),
///                 pfucb->u.pfcb->FDerivedIndex() );
    }
    return "";
}

const CHAR * SzSpaceTreeType( const FUCB * const pfucb )
{
    if ( pfucb->fAvailExt )
    {
        return "AE";
    }
    else if ( pfucb->fOwnExt )
    {
        return "OE";
    }
    //else
    return "!!";
}

#ifdef DEBUG
// SPACECHECK is untested and unmaintained. Enable it at your own risk and
// have fun debugging issues that may or may not be real problems and may or may not
// have been introduced by your code.
//#define SPACECHECK

//  From the code it does not look like we need this anymore.  Even if we do
//  it is a difficulty because we now use cpg = 0 type nodes to represent
//  insertion points.  I am just leaving this here to mark where we used to
//  do this.
//#define DEPRECATED_ZERO_SIZED_AVAIL_EXTENT_CLEANUP
#endif

//  prototypes of internal functions and types
//
class CSPExtentInfo;

LOCAL ERR ErrSPIAddFreedExtent(
    FUCB *pfucb,
    FUCB *pfucbAE,
    const PGNO pgnoLast,
    const CPG cpgSize );

LOCAL ERR ErrSPISeekRootAE(
    _In_ FUCB* const           pfucbAE,
    _In_ const PGNO            pgno,
    _In_ const SpacePool       sppAvailPool,
    _Out_ CSPExtentInfo* const pspeiAE );

LOCAL ERR ErrSPIGetSparseInfoRange(
    _In_ FMP* const pfmp,
    _In_ const PGNO pgnoStart,
    _In_ const PGNO pgnoEnd,
    _Out_ CPG*      pcpgSparse );

LOCAL ERR FSPIParentIsFs( FUCB * const pfucb );

LOCAL ERR ErrSPIGetFsSe(
    FUCB * const pfucb,
    FUCB * const pfucbAE,
    const CPG    cpgReq,
    const CPG    cpgMin,
    const ULONG  fSPFlags,
    const BOOL   fExact = fFalse,
    const BOOL   fPermitAsyncExtension = fTrue,
    const BOOL   fMayViolateMaxSize = fFalse );

LOCAL ERR ErrSPIGetSe(
    FUCB * const    pfucb,
    FUCB * const    pfucbAE,
    const CPG       cpgReq,
    const CPG       cpgMin,
    const ULONG     fSPFlags,
    const SpacePool sppPool,
    const BOOL      MayViolateMaxSize = fFalse );

LOCAL ERR ErrSPIWasAlloc(
    FUCB *pfucb,
    PGNO pgnoFirst,
    CPG cpgSize );

LOCAL ERR ErrSPIValidFDP(
    PIB *ppib,
    IFMP ifmp,
    PGNO pgnoFDP );

LOCAL ERR ErrSPIReserveSPBufPages(
    FUCB* const pfucb,
    FUCB* const pfucbParent = pfucbNil,
    const CPG   cpgAddlReserveOE = 0,
    const CPG   cpgAddlReserveAE = 0,
    const PGNO  pgnoReplace = pgnoNull );

LOCAL ERR ErrSPIAddToAvailExt(
    _In_    FUCB *      pfucbAE,
    _In_    const PGNO  pgnoAELast,
    _In_    const CPG   cpgAESize,
    _In_    SpacePool   sppPool );


LOCAL ERR ErrSPIUnshelvePagesInRange(
    FUCB* const pfucbRoot,
    const PGNO pgnoFirst,
    const PGNO pgnoLast );

LOCAL ERR ErrSPIGetInfo(
    FUCB        *pfucb,
    CPG         *pcpgTotal,
    CPG         *pcpgReserved,
    CPG         *pcpgShelved,
    INT         *piext,
    INT         cext,
    EXTENTINFO  *rgext,
    INT         *pcextSentinelsRemaining,
    CPRINTF     * const pcprintf );

#ifdef EXPENSIVE_INLINE_EXTENT_PAGE_COUNT_CACHE_VALIDATION
// This function does very expensive validation of the value in the Extent Page
// Count Cache by counting space tree pages inline with other space operations.
// It's very good at finding incorrect values as soon as possible after they've
// gone bad but it is also VERY expensive, doing a lot of unnecessary IO in the
// middle of otherwise unrelated operations.
//
// It was instrumental during development of the cache, but not so useful after
// the feature was finished.  It's here to use if we need to debug an incorrect
// value later.

LOCAL VOID SPIValidateCpgOwnedAndAvail(
    FUCB * pfucb
    );
#else
#define SPIValidateCpgOwnedAndAvail( X )
#endif

INLINE VOID AssertSPIWrappedLatches( FUCB *pfucb, CSR *pcsr = pcsrNil )
{
#ifdef DEBUG
    // Single routine to verify the required latching before any update to a space tree.

    if ( PfmpFromIfmp( pfucb->ifmp)->FExclusiveBySession( pfucb->ppib ) )
    {
        // We take shortcuts when we have exclusive access and don't take some
        // latches.  Unfortunately, we don't have enough info in this case to
        // verify the latches we are NOT supposed to be holding.
        return;
    }

    if ( pcsrNil != pcsr )
    {
        // We don't do the same asserts for this as for the other SPIWrapped calls
        // because it uses the pfucb differently.
        Assert( pcsr->Latch() == latchWrite );

        LOG *plog = PinstFromIfmp( pfucb->ifmp )->m_plog;
        // We don't need the FDP latch during repair or redo.
        Assert( g_fRepair                                                             || 
                ( plog->FRecovering() && fRecoveringRedo == plog->FRecoveringMode() ) ||
                FBFWriteLatched( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP() )              ||
                FBFRDWLatched( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP() )                   );

        // Note we don't/can't assert on parent not being latched.  Or can we?  If
        // pcsr->Pgno() == pfucb->u.pfcb->PgnoFDP(), and the latch is there, we
        // could read the external header.  But is it accurate at this point?
    }
    else
    {
        FUCB *pfucbLatchHolder;

        // Sometimes we're here with the FUCB for the main tree, sometimes
        // we're here with a FUCB for the space tree inside the main tree.
        if ( FFUCBSpace( pfucb ) )
        {
            if ( pfucbNil == pfucb->pfucbLatchHolderForSpace )
            {
                AssertSz( fFalse, "Space FUCB has no latch holder recorded, no assertions possible." );
                return;
            }
            Assert( pfucb->u.pfcb == pfucb->pfucbLatchHolderForSpace->u.pfcb );
            pfucbLatchHolder = pfucb->pfucbLatchHolderForSpace;
        }
        else
        {
            pfucbLatchHolder = pfucb;
        }
        
        // If we don't have exclusive access by our session, we ensure we're the only
        // one writing to the space tree by holding a latch on the FDP (not just the
        // root of the space tree).
        if ( pcsrNil != pfucbLatchHolder->pcsrRoot )
        {
            switch ( pfucbLatchHolder->pcsrRoot->Latch() )
            {
                case latchRIW:
                case latchWrite:
                    break;
                default:
                    AssertSz( fFalse, "Unexpected latch type." );
                    break;
            }
            
            // We want to make sure we don't have a latch on the parent of whatever tree we're
            // updating.  The Extent Page Count Cache requires it (at least for the specific
            // case where the parent is the DB root), so validate.
            KEYDATAFLAGS kdf;
            const SPACE_HEADER *psph;
            PGNO pgnoParent;
            
            Assert( pfucbLatchHolder->pcsrRoot->Pgno() == pfucbLatchHolder->u.pfcb->PgnoFDP() );
            
            NDGetExternalHeader( &kdf, pfucbLatchHolder->pcsrRoot, noderfSpaceHeader );
            Assert( sizeof( SPACE_HEADER ) == kdf.data.Cb() );
            psph = reinterpret_cast <const SPACE_HEADER *> ( kdf.data.Pv() );
            pgnoParent = psph->PgnoParent();
            kdf.Nullify();

            Assert( pgnoNull == pgnoParent || FBFNotLatched( pfucbLatchHolder->ifmp, pgnoParent ) );
        }
        else
        {
            AssertSz( fFalse, "Latch Holder has no root page pointer, no assertions possible." );
        }
    }
#endif
}

//
// Wrappers around updates on space trees to force you to think about the
// ExtentPageCountCache.
//
VOID
SPIWrappedNDInsert(
    _Inout_ FUCB * const pfucb,
    _Inout_ CSR * const pcsr,
    _In_ const KEYDATAFLAGS * const pkdf,
    _In_ const CPG cpgDeltaOE = 0,
    _In_ const CPG cpgDeltaAE = 0
    )
{
    ERR err;
    PIB  *ppib = pfucb->ppib;
    BOOL fOpenedTransaction = fFalse;
    BOOL fUpdateCache = ( ( cpgDeltaOE != 0 ) || ( cpgDeltaAE != 0 ) );

    AssertSPIWrappedLatches( pfucb, pcsr );

    // This routine is in a code path that is not allowed to fail.  The actual
    // call to NDInsert can't fail, so that's good.  But the calls to begin
    // and end a transaction and the call to prepare the Extent Page Count Cache
    // for an update CAN fail.  Unfortunately if those calls fail, the best we
    // can do is log that we couldn't update the cache and we'll have to fix it
    // somehow later.
    
    if ( fUpdateCache && ( 0 == ppib->Level() ) )
    {
        // ErrCATAdjustExtentPageCountsPrepare and CATAdjustExtentPageCounts expect
        // to be in a transaction, and while we're normally in a transaction here,
        // we might not be (e.g. we may be under ErrIsamCompact() called by eseutil)
        // Begin and end a transaction here if we need to.  Note that the cache update
        // is unversioned, so we don't do much (or anything) with the transaction,
        // but at the very least we'll trigger all sorts of downstream Asserts() if
        // we don't have one.

        err =  ErrDIRBeginTransaction( ppib, 35734, NO_GRBIT );
        if ( JET_errSuccess > err )
        {
            OSTraceSuspendGC();
            const WCHAR * rgwsz[] = {
                PfmpFromIfmp( pfucb->ifmp )->WszDatabaseName(),
                OSFormatW( L"%d", ObjidFDP( pfucb ) ),
                OSFormatW( L"%d", PgnoFDP( pfucb ) ),
                OSFormatW( L"%d", err )
            };
            
            UtilReportEvent(
                eventError,
                SPACE_MANAGER_CATEGORY,
                BEGIN_TRANSACTION_FOR_EXTENT_PAGE_COUNT_CACHE_PREPARE_FAILED_ID,
                _countof( rgwsz ),
                rgwsz,
                0,
                PinstFromPfucb( pfucb ) );
            OSTraceResumeGC();
            
            // We can't trust any further calls to the cache here, since we don't have that transaction.
            fUpdateCache = fFalse;
        }
        else
        {
            fOpenedTransaction = fTrue;

            // Beginning this new transaction shouldn't have done anything.
            Assert( prceNil == ppib->prceNewest );
            Assert( 1 == ppib->Level() );
            Assert( !ppib->FBegin0Logged() );
            Assert( !FFUCBUpdatePrepared( pfucb ) );
        }
    }

    if ( fUpdateCache )
    {
        // We either successfully opened a transaction or we were already in one, a necessary
        // preliminary for ErrCATAdjustExtentPageCountsPrepare.
        Assert( 0 != ppib->Level() );

        err = ErrCATAdjustExtentPageCountsPrepare( pfucb );
        if ( JET_errSuccess > err )
        {
            OSTraceSuspendGC();
            const WCHAR * rgwsz[] = {
                PfmpFromIfmp( pfucb->ifmp )->WszDatabaseName(),
                OSFormatW( L"%d", ObjidFDP( pfucb ) ),
                OSFormatW( L"%d", PgnoFDP( pfucb ) ),
                OSFormatW( L"%d", err )
            };
            
            UtilReportEvent(
                eventError,
                SPACE_MANAGER_CATEGORY,
                EXTENT_PAGE_COUNT_CACHE_PREPARE_FAILED_ID,
                _countof( rgwsz ),
                rgwsz,
                0,
                PinstFromPfucb( pfucb ) );
            OSTraceResumeGC();
        }

        // Note that we don't unset fUpdateCache.  We might be able to make a successful
        // call to CATAdjustExtentPageCounts() later and end up with no problem.  No
        // guarantees, but it's possible.
    }
    
    NDInsert( pfucb, pcsr, pkdf );
    
    if ( fUpdateCache )
    {
        CATAdjustExtentPageCounts( pfucb, cpgDeltaOE, cpgDeltaAE );
    }
    
    
    if ( fOpenedTransaction )
    {
        // Operations on this new transaction shoulnd't have done anything, so we shouldn't
        // have logged a BeginTransaction0.
        Assert( prceNil == ppib->prceNewest );
        Assert( 1 == ppib->Level() );
        Assert( !ppib->FBegin0Logged() );
        Assert( !FFUCBUpdatePrepared( pfucb ) );

        // Since we didn't create any RCEs and didn't log anything, there should be no way for commit to fail.
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }
}
#define NDInsert _USE_SPI_NODE_INSERT_SPACE_TREE

ERR
ErrSPIWrappedNDSetExternalHeader(
    _Inout_ FUCB * const pfucb,
    _In_ CSR *pcsr,
    _In_ const DATA * const pData,
    _In_ const DIRFLAG fDIRFlag,
    _In_ NodeRootField noderf,
    _In_ const CPG cpgDeltaOE = 0,
    _In_ const CPG cpgDeltaAE = 0
    )
{
    ERR err;
    PIB  *ppib = pfucb->ppib;
    BOOL fOpenedTransaction = fFalse;
    BOOL fUpdateCache = ( ( cpgDeltaOE != 0 ) || ( cpgDeltaAE != 0 ) );

    AssertSPIWrappedLatches( pfucb, pcsr );

    if ( fUpdateCache )
    {
        if ( 0 == ppib->Level() )
        {
            // ErrCATAdjustExtentPageCountsPrepare and CATAdjustExtentPageCounts expect
            // to be in a transaction, and while we're normally in a transaction here,
            // we might not be (e.g. we may be under ErrIsamCompact() called by eseutil)
            // Begin and end a transaction here if we need to.  Note that the cache update
            // is unversioned, so we don't do much (or anything) with the transaction,
            // but at the very least we'll trigger all sorts of downstream Asserts() if
            // we don't have one.

            Call( ErrDIRBeginTransaction( ppib, 41878, NO_GRBIT ) );
            fOpenedTransaction = fTrue;

            // Beginning this new transaction shouldn't have done anything.
            Assert( prceNil == ppib->prceNewest );
            Assert( 1 == ppib->Level() );
            Assert( !ppib->FBegin0Logged() );
            Assert( !FFUCBUpdatePrepared( pfucb ) );
        }
        
        Call( ErrCATAdjustExtentPageCountsPrepare( pfucb ) );
    }

    Call( ErrNDSetExternalHeader(
               pfucb,
               pcsr,
               pData,
               fDIRFlag,
               noderf ) );

    if ( fUpdateCache )
    {
        CATAdjustExtentPageCounts( pfucb, cpgDeltaOE, cpgDeltaAE );
    }

HandleError:
    if ( fOpenedTransaction )
    {
        // Operations on this new transaction shoulnd't have done anything, so we shouldn't
        // have logged a BeginTransaction0.
        Assert( prceNil == ppib->prceNewest );
        Assert( 1 == ppib->Level() );
        Assert( !ppib->FBegin0Logged() );
        Assert( !FFUCBUpdatePrepared( pfucb ) );

        // Since we didn't create any RCEs and didn't log anything, there should be no way for commit to fail.
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }

    return err;
}
#define ErrNDSetExternalHeader _USE_ERR_SPI_NODE_SET_EXTERNAL_HEADER_SPACE_TREE_

ERR
ErrSPIWrappedBTFlagDelete(
    _Inout_ FUCB * const pfucb,
    _In_ const DIRFLAG fDIRFlag,
    _In_ const CPG cpgDeltaOE,
    _In_ const CPG cpgDeltaAE
    )
{
    ERR  err;
    PIB  *ppib = pfucb->ppib;
    BOOL fOpenedTransaction = fFalse;
    BOOL fUpdateCache = ( ( cpgDeltaOE != 0 ) || ( cpgDeltaAE != 0 ) );

    AssertSPIWrappedLatches( pfucb );

    Assert( fDIRNoVersion & fDIRFlag );

    if ( fUpdateCache )
    {
        if ( 0 == ppib->Level() )
        {
            // ErrCATAdjustExtentPageCountsPrepare and CATAdjustExtentPageCounts expect
            // to be in a transaction, and while we're normally in a transaction here,
            // we might not be (e.g. we may be under ErrIsamCompact() called by eseutil)
            // Begin and end a transaction here if we need to.  Note that the cache update
            // is unversioned, so we don't do much (or anything) with the transaction,
            // but at the very least we'll trigger all sorts of downstream Asserts() if
            // we don't have one.

            Call( ErrDIRBeginTransaction( ppib, 58262, NO_GRBIT ) );
            fOpenedTransaction = fTrue;

            // Beginning this new transaction shouldn't have done anything.
            Assert( prceNil == ppib->prceNewest );
            Assert( 1 == ppib->Level() );
            Assert( !ppib->FBegin0Logged() );
            Assert( !FFUCBUpdatePrepared( pfucb ) );
        }
        
        Call( ErrCATAdjustExtentPageCountsPrepare( pfucb ) );
    }

    Call( ErrBTFlagDelete(      // UNDONE: Synchronously remove the node
               pfucb,
               fDIRFlag ) );

    if ( fUpdateCache )
    {
        CATAdjustExtentPageCounts( pfucb, cpgDeltaOE, cpgDeltaAE );
    }

HandleError:
    if ( fOpenedTransaction )
    {
        // Operations on this new transaction shoulnd't have done anything, so we shouldn't
        // have logged a BeginTransaction0.
        Assert( prceNil == ppib->prceNewest );
        Assert( 1 == ppib->Level() );
        Assert( !ppib->FBegin0Logged() );
        Assert( !FFUCBUpdatePrepared( pfucb ) );

        // Since we didn't create any RCEs and didn't log anything, there should be no way for commit to fail.
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }

    return err;
}
#define ErrBTFlagDelete _USE_ERR_SPI_FLAG_DELETE_SPACE_TREE_

ERR
ErrSPIWrappedBTReplace(
    _Inout_ FUCB * const pfucb,
    _In_ const DATA &Data,
    _In_ const DIRFLAG fDIRFlag,
    _In_ const CPG cpgDeltaOE,
    _In_ const CPG cpgDeltaAE
    )
{
    ERR  err;
    PIB  *ppib = pfucb->ppib;
    BOOL fOpenedTransaction = fFalse;
    BOOL fUpdateCache = ( ( cpgDeltaOE != 0 ) || ( cpgDeltaAE != 0 ) );

    AssertSPIWrappedLatches( pfucb );

    Assert( fDIRNoVersion & fDIRFlag );

    if ( fUpdateCache )
    {
        if ( 0 == ppib->Level() )
        {
            // ErrCATAdjustExtentPageCountsPrepare and CATAdjustExtentPageCounts expect
            // to be in a transaction, and while we're normally in a transaction here,
            // we might not be (e.g. we may be under ErrIsamCompact() called by eseutil)
            // Begin and end a transaction here if we need to.  Note that the cache update
            // is unversioned, so we don't do much (or anything) with the transaction,
            // but at the very least we'll trigger all sorts of downstream Asserts() if
            // we don't have one.

            Call( ErrDIRBeginTransaction( ppib, 33686, NO_GRBIT ) );
            fOpenedTransaction = fTrue;

            // Beginning this new transaction shouldn't have done anything.
            Assert( prceNil == ppib->prceNewest );
            Assert( 1 == ppib->Level() );
            Assert( !ppib->FBegin0Logged() );
            Assert( !FFUCBUpdatePrepared( pfucb ) );
        }
        
        Call( ErrCATAdjustExtentPageCountsPrepare( pfucb ) );
    }

    Call( ErrBTReplace(
              pfucb,
              Data,
              fDIRFlag ) );

    if ( fUpdateCache )
    {
        CATAdjustExtentPageCounts( pfucb, cpgDeltaOE, cpgDeltaAE );
    }

HandleError:
    if ( fOpenedTransaction )
    {
        // Operations on this new transaction shoulnd't have done anything, so we shouldn't
        // have logged a BeginTransaction0.
        Assert( prceNil == ppib->prceNewest );
        Assert( 1 == ppib->Level() );
        Assert( !ppib->FBegin0Logged() );
        Assert( !FFUCBUpdatePrepared( pfucb ) );

        // Since we didn't create any RCEs and didn't log anything, there should be no way for commit to fail.
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }

    return err;
}
#define ErrBTReplace _USE_ERR_SPI_BT_REPLACE_SPACE_TREE_

ERR
ErrSPIWrappedBTInsert(
    _Inout_ FUCB * const pfucb,
    _In_ const KEY &Key,
    _In_ const DATA &Data,
    _In_ const DIRFLAG fDIRFlag,
    _In_ const CPG cpgDeltaOE,
    _In_ const CPG cpgDeltaAE
    )
{
    ERR  err;
    PIB  *ppib = pfucb->ppib;
    BOOL fOpenedTransaction = fFalse;
    BOOL fUpdateCache = ( ( cpgDeltaOE != 0 ) || ( cpgDeltaAE != 0 ) );

    AssertSPIWrappedLatches( pfucb );

    Assert( fDIRNoVersion & fDIRFlag );

    if ( fUpdateCache )
    {
        if ( 0 == ppib->Level() )
        {
            // ErrCATAdjustExtentPageCountsPrepare and CATAdjustExtentPageCounts expect
            // to be in a transaction, and while we're normally in a transaction here,
            // we might not be (e.g. we may be under ErrIsamCompact() called by eseutil)
            // Begin and end a transaction here if we need to.  Note that the cache update
            // is unversioned, so we don't do much (or anything) with the transaction,
            // but at the very least we'll trigger all sorts of downstream Asserts() if
            // we don't have one.

            Call( ErrDIRBeginTransaction( ppib, 50070, NO_GRBIT ) );
            fOpenedTransaction = fTrue;

            // Beginning this new transaction shouldn't have done anything.
            Assert( prceNil == ppib->prceNewest );
            Assert( 1 == ppib->Level() );
            Assert( !ppib->FBegin0Logged() );
            Assert( !FFUCBUpdatePrepared( pfucb ) );
        }
        
        Call( ErrCATAdjustExtentPageCountsPrepare( pfucb ) );
    }
    
    Call( ErrBTInsert(
              pfucb,
              Key,
              Data,
              fDIRFlag ) );

    if ( fUpdateCache )
    {
        CATAdjustExtentPageCounts( pfucb, cpgDeltaOE, cpgDeltaAE );
        
    }

HandleError:
    if ( fOpenedTransaction )
    {
        // Operations on this new transaction shoulnd't have done anything, so we shouldn't
        // have logged a BeginTransaction0.
        Assert( prceNil == ppib->prceNewest );
        Assert( 1 == ppib->Level() );
        Assert( !ppib->FBegin0Logged() );
        Assert( !FFUCBUpdatePrepared( pfucb ) );

        // Since we didn't create any RCEs and didn't log anything, there should be no way for commit to fail.
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }
    
    return err;
}
#define ErrBTInsert _USE_ERR_SPI_BT_INSERT_SPACE_TREE_


//  Init / Term

POSTIMERTASK g_posttSPITrimDBITask = NULL;
CSemaphore g_semSPTrimDBScheduleCancel( CSyncBasicInfo( "g_semSPTrimDBScheduleCancel" ) );
LOCAL VOID SPITrimDBITask( VOID*, VOID* );

ERR ErrSPInit()
{
    ERR err = JET_errSuccess;

    Assert( g_posttSPITrimDBITask == NULL );
    Call( ErrOSTimerTaskCreate( SPITrimDBITask, (void*)ErrSPInit, &g_posttSPITrimDBITask ) );

    Assert( g_semSPTrimDBScheduleCancel.CAvail() == 0 );
    g_semSPTrimDBScheduleCancel.Release();

HandleError:
    return err;
}

VOID SPTerm()
{
    if ( g_posttSPITrimDBITask )
    {
        OSTimerTaskCancelTask( g_posttSPITrimDBITask );
        OSTimerTaskDelete( g_posttSPITrimDBITask );
        g_posttSPITrimDBITask = NULL;
    }

    Assert( g_semSPTrimDBScheduleCancel.CAvail() <= 1 );
    g_semSPTrimDBScheduleCancel.Acquire();
    Assert( g_semSPTrimDBScheduleCancel.CAvail() == 0 );
}


#ifdef DEBUG
INLINE VOID AssertSPIPfucbOnRoot( const FUCB * const pfucb )
{
    Assert( pfucb->pcsrRoot != pcsrNil );
    Assert( pfucb->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );

    Assert( !FFUCBSpace( pfucb ) );

    switch ( pfucb->pcsrRoot->Latch() )
    {
    case latchRIW:
    case latchWrite:
        break;
    default:
        AssertSz( fFalse, "Unexpected latch type." );
        break;
    }
}

INLINE VOID AssertSPIPfucbNullOrUnlatched( const FUCB * const pfucb )
{
    if ( pfucb == pfucbNil )
    {
        return;
    }

    Assert( !FFUCBSpace( pfucb ) );

    Assert( pfucb->pcsrRoot == pcsrNil );
    if ( pfucb->pcsrRoot != pcsrNil )
    {
        Assert( pfucb->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
        Assert( latchNone == pfucb->pcsrRoot->Latch() );
        AssertSz( fFalse, "Does this happen?" );
    }
}

INLINE VOID AssertSPIPfucbOnSpaceTreeRoot( FUCB *pfucb, CSR *pcsr )
{
    Assert( FFUCBSpace( pfucb ) );
    Assert( pcsr->FLatched() );
    Assert( pcsr->Pgno() == PgnoRoot( pfucb ) );
    Assert( pcsr->Cpage().FRootPage() );
    Assert( pcsr->Cpage().FSpaceTree() );
}
#else
#define AssertSPIPfucbOnRoot( X )
#define AssertSPIPfucbNullOrUnlatched( X )
#define AssertSPIPfucbOnSpaceTreeRoot( X, Y )
#endif

INLINE BOOL FSPValidPGNO( _In_ const PGNO pgno )
{
    return pgnoNull < pgno && pgnoSysMax > pgno;
}

INLINE BOOL FSPValidAllocPGNO( _In_ const PGNO pgno )
{
    //  All valid pgnos we can allocate are higher than the first 3 bootstrap
    //  FDP pages (pgnoSystemRoot, 2 /* pgno DB OE "FDP" */, 3 /* pgno
    //  DB AE "FDP" and less than pgnoSysMax.
    //  Note: Don't have a pgnoDbRootAE, use pgnoFDPMSO /* catalog = pgno 4 */
    return pgnoFDPMSO-1 < pgno && pgnoSysMax > pgno;
}


#ifdef DEBUG
PGNO    g_pgnoAllocTrap = 0;
INLINE VOID SPCheckPgnoAllocTrap( _In_ const PGNO pgnoAlloc, _In_ const CPG cpgAlloc )
{
    if ( g_pgnoAllocTrap == pgnoAlloc ||
            ( g_pgnoAllocTrap > pgnoAlloc && g_pgnoAllocTrap <= pgnoAlloc + cpgAlloc -1 ) )
    {
        AssertSz( fFalse, "Pgno Alloc Trap" );
    }
}
#else
INLINE VOID SPCheckPgnoAllocTrap( _In_ const PGNO pgnoAlloc, _In_ const CPG cpgAlloc )
{
    ;   // do nothing
}
#endif

//  Space Manager EXTENT data structures
//

#ifdef DEBUG
//  Invalid(ate) data
//
const PGNO pgnoBadNews = 0xFFFFFFFF;
const PGNO cpgBadNews = 0xFFFFFFFF; // could do pgnoSysMax + 1 as well?  has different advantages use that.
#endif

//  persisted space tree / extent structures
//

//
//  5 major classes, here is how they work
//
//      These 2 are the persisted data for the key and data portion of a space
//      tree node.  They have limited abilities for validation.
//
//          SPEXTKEY - This is the key maker, and Morpheus says the key maker must be
//                      protected at all costs.  Generally, people shouldn't try to use 
//                      the key maker directly, he will be called when he is needed.
//                      (i.e. by CSPExtentKeyBM, CSPExtentNodeKDF, CSPExtentInfo)
//
//          SPEXTDATA - Similar to SPEXTKEY not for direct consumption, used by the 
//                      following classes and functions.
//
//      These 3 are handlers for the above persisted structures, making everything
//      you need to search / make key, or retrieve data in a consistent logical manner.
//
//          CSPExtentKeyBM - For searching.
//
//          CSPExtentNodeKDF - For setting.
//
//          CSPExtentInfo - Just for knowing.
//


//
//  now some low level information on the actual format.
//
//
//              --------------- key ---------------     -------- data ---------
//              00     01     02     03     04          00     01     02     04
//
//  OE          [-------- pgnoLast --------]            [------ cpgExt -------]
//  AE (orig)   [-------- pgnoLast --------]            [------ cpgExt -------]
//  AE (pool)   [flags][-------- pgnoLast --------]     [------ cpgExt -------]
//               flags = 0x80 | sppPool:3
//
//  The two AE formats are for general space (in the original / legacy format), and
//  the (pool) version for non-0 new pools of space.
//
//  The differentiator between AE(orig) and AE(pool) is, in addition to length, the
//  top bit of the flags byte.  The high bit in the first byte of OE and AE(orig) is
//  always unset.
//
//  At this time, we only store the bottom 3 bits of a SpacePool enum value, as those are the
//  only bits that have meaning in this context.
//
//  To be completely accurate, we really only use the bottom 2 bits of the SpacePool enum, since
//  only values 0, 1, and 2 reference actual pools.  The 3rd bit is always 0.
//
//  Note that the 0x80 flag is overlaid to separate searchable key space for pool-based space
//  and to indicate this is indeed the pool-based avail key format.
#include <pshpack1.h>

PERSISTED
class SPEXTKEY {

    private:
        union {
            BYTE                mbe_pgnoLast[sizeof(PGNO)]; // 4 bytes
            struct
            {
                BYTE            bExtFlags;
                BYTE            mbe_pgnoLastAfterFlags[sizeof(PGNO)];   // 4 bytes
            };

        };

        BOOL _FNewSpaceKey() const
        {
            return fSPAvailExtReservedPool == ( bExtFlags & fSPAvailExtReservedPool );
        }

        ULONG _Cb( ) const
        {

            if ( _FNewSpaceKey() )
            {
                C_ASSERT( 0x5 == sizeof(SPEXTKEY) );
                return sizeof(SPEXTKEY);
            }
            else
            {
                C_ASSERT( 0x4 == sizeof(PGNO) );    // legacy key was 4
                return sizeof(PGNO);
            }
        }

        PGNO _PgnoLast( void ) const
        {
            #ifdef DEBUG
            PGNO pgnoLast = pgnoBadNews;
            #else
            PGNO pgnoLast;
            #endif
            if ( _FNewSpaceKey() )
            {
                LongFromUnalignedKey( &pgnoLast, mbe_pgnoLastAfterFlags );
            }
            else
            {
                LongFromUnalignedKey( &pgnoLast, mbe_pgnoLast );
            }
            Assert( pgnoBadNews != pgnoLast );
            return pgnoLast;
        }

    public:

#ifdef DEBUG
        SPEXTKEY( )     { Invalidate(); }
        ~SPEXTKEY( )        { Invalidate(); }
        VOID Invalidate()   { memset( this, 0xFF, sizeof(*this) ); }
#else
        SPEXTKEY( )     { }
        ~SPEXTKEY( )        { }
#endif

    public:

        //
        //  Interface for making keys, getting at key and key data.
        //

        enum    E_SP_EXTENT_TYPE
                {
                    fSPExtentTypeUnknown    = 0x0,
                    fSPExtentTypeOE,
                    fSPExtentTypeAE,
                };


        enum    {   fSPAvailExtReservedPool = 0x80  };  // High bit used as a marker in a key (see above).
        enum    {   fSPReservedZero         = 0x40  };  // Defined for validation only.  If you need this bit
                                                        // for persisted storage, you must expand
                                                        //  the persisted SpacePool to 2 bytes so that you
                                                        //  are not left with 0 bits to express new format.
        enum    {   fSPReservedBitsMask     = 0x38  };  // Currently unused bits.  Available to be added to
                                                        //  fSPPoolMask if and when spp::Max gets bigger.
        enum    {   fSPPoolMask             = 0x07  };  // Bits of a SpacePool that can be stored.

        // static_assert such that at runtime we need only check that a SpacePool is valid to know that
        // it can be safely stored.

        static_assert( ( 0xFF == ( (BYTE)fSPAvailExtReservedPool |
                                   (BYTE)fSPReservedZero         |
                                   (BYTE)fSPReservedBitsMask     |
                                   (BYTE)fSPPoolMask               ) ), "All bits are used." );

        static_assert( ( ( 0 == ( (BYTE)fSPAvailExtReservedPool & (BYTE)fSPReservedZero     ) ) &&
                         ( 0 == ( (BYTE)fSPAvailExtReservedPool & (BYTE)fSPReservedBitsMask ) ) &&
                         ( 0 == ( (BYTE)fSPAvailExtReservedPool & (BYTE)fSPPoolMask         ) ) &&
                         ( 0 == ( (BYTE)fSPReservedZero         & (BYTE)fSPReservedBitsMask ) ) &&
                         ( 0 == ( (BYTE)fSPReservedZero         & (BYTE)fSPPoolMask         ) ) &&
                         ( 0 == ( (BYTE)fSPReservedBitsMask     & (BYTE)fSPPoolMask         ) )    ) , "All bits are used only once." );

        static_assert( ( (BYTE)spp::MaxPool == ( (BYTE)fSPPoolMask & (BYTE)spp::MaxPool ) ),
                       "We are using enough bits to safely mask all spp:: values." );
    
        VOID Make(
            _In_ const E_SP_EXTENT_TYPE     eExtType,
            _In_ const PGNO                 pgnoLast,
            _In_ const SpacePool            sppAvailPool )
        {
            //  Check there are no unintended bits.
            Assert( FSPIValidExplicitSpacePool( sppAvailPool ) );

            //  Setup the key.
            if ( eExtType == fSPExtentTypeOE )
            {
                Assert( spp::AvailExtLegacyGeneralPool == sppAvailPool );
                UnalignedKeyFromLong( mbe_pgnoLast, pgnoLast );
            }
            else if ( eExtType == fSPExtentTypeAE )
            {
                if ( spp::AvailExtLegacyGeneralPool != sppAvailPool )
                {
                    // And here's where we set the top bit.
                    bExtFlags = ( ( BYTE ) fSPAvailExtReservedPool | ( BYTE )sppAvailPool );
                    UnalignedKeyFromLong( mbe_pgnoLastAfterFlags, pgnoLast );
                    Assert( _FNewSpaceKey() );
                }
                else
                {
                    // We never explicitly store AvailLegacyGeneralPool with the top bit set.
                    UnalignedKeyFromLong( mbe_pgnoLast, pgnoLast );
                }
            }
            else
            {
                AssertSz( fFalse, "Unknown Ext type" );
            }
        }

        //  Accessors (for the key data)
        //
        ULONG Cb( ) const           { ASSERT_VALID( this ); return _Cb(); }
        const VOID * Pv( ) const
        {
            ASSERT_VALID( this );
            Assert( (BYTE*)this == mbe_pgnoLast );

            C_ASSERT( OffsetOf( SPEXTKEY, mbe_pgnoLast ) == OffsetOf( SPEXTKEY, bExtFlags ) );
            Assert( OffsetOf( SPEXTKEY, mbe_pgnoLast ) == OffsetOf( SPEXTKEY, bExtFlags ) );    // be extra sure
            C_ASSERT( OffsetOf( SPEXTKEY, mbe_pgnoLast ) != OffsetOf( SPEXTKEY, mbe_pgnoLastAfterFlags ) );
            Assert( OffsetOf( SPEXTKEY, mbe_pgnoLast ) != OffsetOf( SPEXTKEY, mbe_pgnoLastAfterFlags ) );

            return reinterpret_cast<const VOID *>(mbe_pgnoLast);
        }

        //  Logical accessors
        //
        PGNO PgnoLast() const           { ASSERT_VALID( this ); Assert( pgnoNull != _PgnoLast() ); return _PgnoLast(); }
        BOOL FNewAvailFormat() const    { ASSERT_VALID( this ); return _FNewSpaceKey(); }
        SpacePool SppPool() const
        {
            SpacePool spp;
            ASSERT_VALID( this );
            
            if( _FNewSpaceKey() )
            {
                spp = (SpacePool) (bExtFlags & fSPPoolMask);
                Assert( FSPIValidExplicitSpacePool( spp ) );
                // We don't explicitly store AvailExtLegacyGeneralPool.
                Assert( spp != spp::AvailExtLegacyGeneralPool );
            }
            else
            {
                spp = spp::AvailExtLegacyGeneralPool;
            }
            return spp;
        }

    public:
        //  Validation functions
        //

        enum    E_VALIDATE_TYPE
                {
                    fValidateData           = 0x01,
                    fValidateSearch         = 0x02
                };

        INLINE BOOL FValid( _In_ const E_SP_EXTENT_TYPE eExtType, _In_ const E_VALIDATE_TYPE fValidateType ) const
        {
            PGNO pgnoLast = _PgnoLast();

            Assert( pgnoBadNews != pgnoLast );
            if ( pgnoSysMax < pgnoLast )
            {
                return fFalse;
            }

            //  On non-search keys we can do stronger validations, i.e. such as 
            //  those that came from the a database itself.
            if ( fValidateData == fValidateType )
            {
                if ( eExtType == fSPExtentTypeOE &&
                    !FSPValidPGNO( pgnoLast ) )
                {
                    return fFalse;
                }

                //  Must be between reasonable values, first 4 pages or over pgnoSysMax, off limits.
                if ( eExtType == fSPExtentTypeAE &&
                    !FSPValidAllocPGNO( pgnoLast ) )
                {
                    return fFalse;
                }
            }
            return fTrue;   // let the worthy through ...
        }
        INLINE BOOL FValid( _In_ const E_SP_EXTENT_TYPE eExtType, _In_ const ULONG cb ) const
        {
            return Cb() == cb && FValid( eExtType, fValidateData );
        }
    private:
        INLINE BOOL FValid( ) const
        {
            //  The worst case we validate, but there are only so many possibilities.
            return FValid( fSPExtentTypeOE, fValidateData ) ||
                    FValid( fSPExtentTypeAE, fValidateData ) ||
                    FValid( fSPExtentTypeOE, fValidateSearch ) ||
                    FValid( fSPExtentTypeAE, fValidateSearch );
        }
    public:
#ifdef DEBUG
        VOID AssertValid() const
        {
            AssertSz( FValid( ), "Invalid Space Extent Key" );
        }
#endif

};

C_ASSERT( 0x5 == sizeof(SPEXTKEY ) );


PERSISTED
class SPEXTDATA {

    private:
        UnalignedLittleEndian< CPG >        le_cpgExtent;


    public:
        VOID Invalidate( )
        {
            #ifdef DEBUG
            le_cpgExtent = pgnoBadNews;
            #endif
        }
        SPEXTDATA( )
        {
            #ifdef DEBUG
            Invalidate();
            #endif
        }
        ~SPEXTDATA( )
        {
            #ifdef DEBUG
            Invalidate();
            #endif
        }

    public:
        SPEXTDATA( _In_ const CPG cpgExtent )
        {
            Set( cpgExtent );
            Assert( FValid( ) );
        }
        VOID Set( _In_ const CPG cpgExtent )
        {
            le_cpgExtent = cpgExtent;
            Assert( FValid( ) );
        }
        CPG         CpgExtent() const   { return le_cpgExtent; }
        ULONG       Cb() const          { C_ASSERT( 4 == sizeof(SPEXTDATA) ); return sizeof(SPEXTDATA); }
        const VOID* Pv() const          { return reinterpret_cast<const VOID*>(&le_cpgExtent); }

    public:
        BOOL FValid( ) const
        {
            return PGNO(le_cpgExtent) < pgnoSysMax;
        }
        BOOL FValid( ULONG cb ) const
        {
            return cb == Cb() && FValid( );
        }

};

C_ASSERT( 0x4 == sizeof(SPEXTDATA ) );

#include <poppack.h>

class CSPExtentInfo {

    private:

        //  (runtime) Fields to indicate where / how we came by this extent.
        //
        SPEXTKEY::E_SP_EXTENT_TYPE  m_eSpExtType:8;
        FLAG32                      m_fFromTempDB:1;

        //  Fields about the extent, pulled from persisted data.
        //
        PGNO                        m_pgnoLast;
        CPG                         m_cpgExtent;
        SpacePool                   m_sppPool;
        FLAG32                      m_fNewAvailFormat:1;

        //  Other
        FLAG32                      m_fCorruptData:1;   // set if the kdf seems wrong, or bad pgno's or cpg's.

        //  Internal accessors 
        //
        PGNO _PgnoFirst() const     { return m_pgnoLast - m_cpgExtent + 1; }
        BOOL _FOwnedExt() const     { return m_eSpExtType == SPEXTKEY::fSPExtentTypeOE; }
        BOOL _FAvailExt() const     { return m_eSpExtType == SPEXTKEY::fSPExtentTypeAE; }


        VOID _Set( _In_ const SPEXTKEY::E_SP_EXTENT_TYPE eSpExtType, _In_ const KEYDATAFLAGS& kdfCurr )
        {
            m_fCorruptData = fFalse;    // innocdent until proven guilty

            //  Track source of this data needed for later validation
            //
            m_eSpExtType = eSpExtType;
            if ( SPEXTKEY::fSPExtentTypeUnknown == eSpExtType )
            {
                if( kdfCurr.key.Cb() == 0x4 )
                {
                    m_eSpExtType = SPEXTKEY::fSPExtentTypeOE;
                }
                else if ( kdfCurr.key.Cb() == 0x5 )
                {
                    m_eSpExtType = SPEXTKEY::fSPExtentTypeAE;
                }
            }

            //  OE only currently supports 4-byte keys.
            //
            if ( kdfCurr.key.Cb() == 0x5 )
            {
                Assert( m_eSpExtType == SPEXTKEY::fSPExtentTypeAE );
            }

            //  Get ending offset of the extent out of key
            //

            BYTE rgExtKey[sizeof(SPEXTKEY)];
            kdfCurr.key.CopyIntoBuffer( rgExtKey, sizeof( rgExtKey ) );
            const SPEXTKEY * const pSPExtKey = reinterpret_cast<const SPEXTKEY *>( rgExtKey );

            m_fCorruptData |= !pSPExtKey->FValid( m_eSpExtType, kdfCurr.key.Cb() );
            Assert( !m_fCorruptData );

            m_pgnoLast = pSPExtKey->PgnoLast();

            m_sppPool = pSPExtKey->SppPool();

            if ( pSPExtKey->FNewAvailFormat() )
            {
                Assert( m_eSpExtType == SPEXTKEY::fSPExtentTypeAE );
                Assert( _FAvailExt() ); // today we only allow extended format on avail exts.
                Assert( 0x5 == kdfCurr.key.Cb() );
                m_fNewAvailFormat = fTrue;
            }
            else
            {
                Assert( 0x4 == kdfCurr.key.Cb() );
                Assert( spp::AvailExtLegacyGeneralPool == m_sppPool );
                m_fNewAvailFormat = fFalse;
            }

            //  Get length of extent out of data
            //

            const SPEXTDATA * const pSPExtData = reinterpret_cast<const SPEXTDATA *>( kdfCurr.data.Pv() );

            m_fCorruptData |= !pSPExtData->FValid( kdfCurr.data.Cb() );
            Assert( !m_fCorruptData );

            m_cpgExtent = pSPExtData->CpgExtent();
            Assert( m_cpgExtent >= 0 ); // not sure we can actually count on this.

        }

    public:

#ifdef DEBUG
        VOID Invalidate()
        {
            m_fCorruptData = fTrue;
            m_eSpExtType = SPEXTKEY::fSPExtentTypeUnknown;
            m_pgnoLast = pgnoBadNews;
            m_cpgExtent = cpgBadNews;
        }
        VOID AssertValid() const
        {
            Assert( SPEXTKEY::fSPExtentTypeUnknown != m_eSpExtType &&   // uninit
                    pgnoBadNews != m_pgnoLast &&
                    cpgBadNews != m_cpgExtent );
            Assert( SPEXTKEY::fSPExtentTypeAE == m_eSpExtType ||    // but be explicit too
                    SPEXTKEY::fSPExtentTypeOE == m_eSpExtType );
            Assert( m_fCorruptData || ErrCheckCorrupted( fTrue ) >= JET_errSuccess );
        }
#endif
        CSPExtentInfo(  )   { Unset(); }
        ~CSPExtentInfo(  )  { Unset(); }

        //  So people can create const versions.
        CSPExtentInfo( const FUCB * pfucb )
        {
            Set( pfucb );
            ASSERT_VALID( this );
            Assert( cpgBadNews != PgnoLast() );
            Assert( cpgBadNews != CpgExtent() );
        }

        CSPExtentInfo( const KEYDATAFLAGS * pkdf )
        {
            _Set( SPEXTKEY::fSPExtentTypeUnknown, *pkdf );
            ASSERT_VALID( this );
            Assert( cpgBadNews != PgnoLast() );
            Assert( cpgBadNews != CpgExtent() );
        }

        VOID Set( _In_ const FUCB * pfucb )
        {
            m_fCorruptData = fFalse;    // innocent until proven guilty

            Assert( FFUCBSpace( pfucb ) );
            Assert( Pcsr( pfucb )->FLatched() );

            //  Track source of this data needed for later validation
            //
            m_eSpExtType = pfucb->fAvailExt ? SPEXTKEY::fSPExtentTypeAE : SPEXTKEY::fSPExtentTypeOE;
            m_fFromTempDB = FFMPIsTempDB( pfucb->ifmp );

            //  Put data in
            //
            _Set( m_eSpExtType, pfucb->kdfCurr );

        }

        VOID Unset( )
        {
            m_eSpExtType = SPEXTKEY::fSPExtentTypeUnknown;
            #ifdef DEBUG
            Invalidate();
            #endif
        }

        BOOL FIsSet( ) const
        {
            return m_eSpExtType != SPEXTKEY::fSPExtentTypeUnknown;
        }

        //  Accessors (of data source)
        //
        BOOL FOwnedExt() const  { ASSERT_VALID( this ); return _FOwnedExt(); }
        BOOL FAvailExt() const  { ASSERT_VALID( this ); return _FAvailExt(); }

        //  Accessors (of actual data) - will insist on non-corrupted data.
        //
        BOOL FValidExtent( ) const      { ASSERT_VALID( this ); return ErrCheckCorrupted( fFalse ) >= JET_errSuccess; }
        PGNO PgnoLast() const           { ASSERT_VALID( this ); Assert( FValidExtent( ) ); return m_pgnoLast; }
        CPG  CpgExtent() const          { ASSERT_VALID( this ); Assert( FValidExtent( ) ); return m_cpgExtent; }
        PGNO PgnoFirst() const          { ASSERT_VALID( this ); Assert( FValidExtent( ) ); Assert( m_cpgExtent != 0 ); return _PgnoFirst(); }
        BOOL FContains( _In_ const PGNO pgnoIn ) const
        {
            ASSERT_VALID( this );
            #ifdef DEBUG
            BOOL fInExtent = ( ( pgnoIn >= _PgnoFirst() ) && ( pgnoIn <= m_pgnoLast ) );
            if ( 0x0 == m_cpgExtent )
            {
                Assert( !fInExtent );
            }
            else
            {
                Assert( FValidExtent( ) );
            }
            return fInExtent;
            #else
            return ( ( pgnoIn >= _PgnoFirst() ) && ( pgnoIn <= m_pgnoLast ) );
            #endif
        }

        //  Accessors (of insertion region data) - will insist on non-corrupted data.
        //
        //  These are separate, because there is a wider range of non-corrupted states, for
        //  this data.
        //
        BOOL FNewAvailFormat() const    { ASSERT_VALID( this ); Assert( FValidExtent( ) ); return m_fNewAvailFormat; }
        CPG  FEmptyExtent() const       { ASSERT_VALID( this ); return 0x0 == m_cpgExtent; }
        PGNO PgnoMarker() const     { ASSERT_VALID( this ); return m_pgnoLast; }
        SpacePool SppPool() const
        {
            //  difficult to assert valid, as this is a not a valid extent, in that the length is zero.
            ASSERT_VALID( this );

            Assert ( m_fNewAvailFormat || m_sppPool == spp::AvailExtLegacyGeneralPool );
            return m_sppPool;
        }


        enum    FValidationStrength { fStrong = 0x0, fZeroLengthOK = 0x1, fZeroFirstPgnoOK = 0x2 };
        INLINE static BOOL FValidExtent( _In_ const PGNO pgnoLast, _In_ const CPG cpg, _In_ const FValidationStrength fStrength = fStrong )
        {
            //  A variety of bad values and/or signs could be troublesome for future math.

            if( fStrength & fZeroLengthOK )
            {
                if( ( cpg < 0 ) ||
                    ( !( ( pgnoLast - cpg ) <= pgnoLast ) ) ||
                    ( (ULONG)cpg > pgnoLast ) ||    // might be overkill
                    //  Overflow checks of the effective pgno before the pgnoFirst.
                    ( ( (INT)pgnoLast - cpg ) <= ( fStrength & fZeroFirstPgnoOK ? -1 : 0 ) ) ||
                    //  Overflow checks for the effective pgno after pgnoLast.
                    ( ( pgnoLast + 1 ) < (ULONG)cpg ) )
                {
                    return fFalse;
                }
                //  If we got all our math right this should all hold trivially
                Assert( ( pgnoLast - cpg + 1 /* "pgnoFirst" */ ) <= pgnoLast );
            }
            else
            {
                if( ( cpg <= 0 ) ||
                    ( !( ( pgnoLast - cpg ) < pgnoLast ) ) ||
                    ( (ULONG)cpg > pgnoLast ) ||    // might be overkill
                    //  Overflow checks of the effective pgno before the  pgnoFirst.
                    ( ( (INT)pgnoLast - cpg ) <= ( fStrength & fZeroFirstPgnoOK ? -1 : 0 ) ) ||
                    //  Overflow checks for the effective pgno after pgnoLast.
                    ( ( pgnoLast + 1 ) < (ULONG)cpg ) )
                {
                    return fFalse;
                }
                //  If we got all our math right this should all hold trivially
                Assert( ( pgnoLast - cpg + 1 /* "pgnoFirst" */ ) <= pgnoLast );
            }
            return fTrue;
        }

        ERR ErrCheckCorrupted( BOOL fSilentOperation = fFalse ) const
        {
            //  We check that the data read from the FUCB / KDF made sense before 
            //  trying to interpret our member variables based on it (set in Set()).
            if( m_fCorruptData )
            {
                AssertSz( fSilentOperation, "Ext Node corrupted at TAG level" );
                goto HandleError;
            }

            //  OE does not currently use the new format.
            if ( m_fNewAvailFormat && _FOwnedExt() )
            {
                AssertSz( fSilentOperation, "OE does not currently use the new format" );
                goto HandleError;
            }

            //  We don't recognize pools in the OE.
            if ( ( spp::AvailExtLegacyGeneralPool != m_sppPool ) && _FOwnedExt() )
            {
                AssertSz( fSilentOperation, "OE does not currently support pools" );
                goto HandleError;
            }

            //  Make sure the pool is valid.
            if ( !FSPIValidExplicitSpacePool( m_sppPool ) )
            {
                AssertSz( fSilentOperation, "Invalid pool" );
                goto HandleError;
            }

            //  In spp::ShelvedPool we only allow single-page extents and we never
            //  merge extents.
            if ( spp::ShelvedPool == m_sppPool && 1 != m_cpgExtent )
            {
                AssertSz( fSilentOperation, "Shelved pool must only contain single-page extents." );
                goto HandleError;
            }

            //  In spp::ContinuousPool we allow zero sized extents as insertion 
            //  region markers, but if not in spp::ContinuousPool or the cpg is 
            //  other than zero, then this should be a valid extent.
            if( spp::ContinuousPool != m_sppPool || 0 != m_cpgExtent )
            {
                //  Validate we won't have math problems with the extent pgnoLast/cpg
                if( !FValidExtent( m_pgnoLast,
                                   m_cpgExtent,
                                   FValidationStrength( ( _FAvailExt() ? fZeroLengthOK : fStrong ) |
                                                        ( _FOwnedExt() ? fZeroFirstPgnoOK : fStrong ) ) ) )
                {
                    AssertSz( fSilentOperation, "Extent invalid range, could cause math overflow" );
                    goto HandleError;
                }
            }

            //  Check it is a PGNO we would give out.
            if( !FSPValidPGNO( _PgnoFirst() ) || !FSPValidPGNO( m_pgnoLast ) )
            {
                AssertSz( fSilentOperation, "Invalid pgno" );
                goto HandleError;
            }

            //  This wouldn't qualify as invalid, but I defy someone to get this 
            //  to go off ... it would be a single extent larger than 128 GBs,
            //  and on a chk build none-the-less, so if this happens, we probably
            //  have a bug.
            //  We divide by g_cbPage where we do in order to avoid LONG overflow
            //  problems.
            Assert( m_cpgExtent < 128 * 1024 / g_cbPage * 1024 * 1024 );

            return JET_errSuccess;

        HandleError:
                        //  Should we maybe log an event or something?
            if ( _FOwnedExt()  )
            {
                return ErrERRCheck( JET_errSPOwnExtCorrupted );
            }
            else if ( _FAvailExt() )
            {
                return ErrERRCheck( JET_errSPAvailExtCorrupted );
            }

            AssertSz( fFalse, "Unknown extent type." );
            return ErrERRCheck( JET_errInternalError );
        }

};


//  Useful helper function to encapsulate creating a key for a space ext.
class CSPExtentKeyBM {

    private:
        BOOKMARK                    m_bm;
        SPEXTKEY                    m_spextkey;
        SPEXTKEY::E_SP_EXTENT_TYPE  m_eSpExtType:8;
        BYTE                        m_fBookmarkSet:1;

    public:

        CSPExtentKeyBM()
        {
            m_fBookmarkSet = fFalse;
            #ifdef DEBUG
            m_eSpExtType = SPEXTKEY::fSPExtentTypeUnknown;
            m_spextkey.Invalidate();
            m_bm.Invalidate();
            #endif
        }

        //  A constructor so people can declare const versions
        CSPExtentKeyBM(
            _In_ const  SPEXTKEY::E_SP_EXTENT_TYPE  eExtType,
            _In_ const  PGNO                        pgno,
            _In_ const  SpacePool                   sppAvailPool = spp::AvailExtLegacyGeneralPool
            )
        {
            m_fBookmarkSet = fFalse;

            SPExtentMakeKeyBM( eExtType, pgno, sppAvailPool );

            Assert( m_spextkey.FValid( eExtType, SPEXTKEY::fValidateSearch  ) );
            Assert( m_spextkey.Pv() == m_bm.key.suffix.Pv() );
        }

        VOID SPExtentMakeKeyBM(
            _In_ const  SPEXTKEY::E_SP_EXTENT_TYPE  eExtType,
            _In_ const  PGNO                        pgno,
            _In_ const  SpacePool                   sppAvailPool = spp::AvailExtLegacyGeneralPool
            )
        {
            m_eSpExtType = eExtType;

            if( m_eSpExtType == SPEXTKEY::fSPExtentTypeAE )
            {
                Assert( FSPIValidExplicitSpacePool( sppAvailPool ) );
            }
            else
            {
                Assert( SPEXTKEY::fSPExtentTypeOE == eExtType );
                Assert( spp::AvailExtLegacyGeneralPool == sppAvailPool );
            }

            m_spextkey.Make( eExtType, pgno, sppAvailPool );
            Assert( m_spextkey.FValid( eExtType, SPEXTKEY::fValidateSearch ) );

            if ( !m_fBookmarkSet )
            {
                m_bm.key.prefix.Nullify();
                m_bm.key.suffix.SetPv( (VOID *) m_spextkey.Pv() );  // unconsting...
                m_bm.key.suffix.SetCb( m_spextkey.Cb() );
                m_bm.data.Nullify();
                Assert( m_bm.key.suffix.Pv() == m_spextkey.Pv() );  // validate constructor set this up.
                m_fBookmarkSet = fTrue;
            }

        }

        const BOOKMARK& GetBm( FUCB * pfucb ) const
        {
            Assert( FFUCBSpace( pfucb ) && FFUCBUnique( pfucb ) );  // we have the client pass the pfucb just for sanity checking
            Assert( m_spextkey.FValid( pfucb->fAvailExt ? SPEXTKEY::fSPExtentTypeAE : SPEXTKEY::fSPExtentTypeOE, SPEXTKEY::fValidateSearch ) );
            Assert( pgnoBadNews != m_spextkey.PgnoLast() );
            return m_bm;
        }

        const BOOKMARK* Pbm( FUCB * pfucb ) const
        {
            Assert( FFUCBSpace( pfucb ) && FFUCBUnique( pfucb ) );  // we have the client pass the pfucb just for sanity checking
            Assert( m_spextkey.FValid( pfucb->fAvailExt ? SPEXTKEY::fSPExtentTypeAE : SPEXTKEY::fSPExtentTypeOE, SPEXTKEY::fValidateSearch ) );
            return &m_bm;
        }

};

class CSPExtentNodeKDF {

    private:

        //  Helper structs for updates.
        //
        KEYDATAFLAGS                m_kdf;

        //  Persisted data.
        //
        SPEXTDATA                   m_spextdata;
        SPEXTKEY                    m_spextkey;

        //  Tracking info.
        //
        SPEXTKEY::E_SP_EXTENT_TYPE  m_eSpExtType:8;

        //  Update semantics.
        //
        BYTE                        m_fShouldDeleteNode:1;

    public:

#ifdef DEBUG
        VOID AssertValid( ) const
        {
            Assert( pgnoBadNews != m_spextkey.PgnoLast() ); // never use cpg w/o pgnoLast.
            Assert( pgnoBadNews != m_spextdata.CpgExtent() );
        }
#endif

        VOID Invalidate( void )
        {
            //  Invalidate, it make it yucky so we catch errors.
            #ifdef DEBUG
            m_kdf.Nullify();
            m_spextkey.Invalidate();
            m_spextdata.Invalidate();
            #endif
        }
        BOOL FValid( void ) const
        {
            Assert( pgnoBadNews != m_spextkey.PgnoLast() ); // never use cpg w/o pgnoLast.
            Assert( cpgBadNews != m_spextdata.CpgExtent() );
            if( m_spextkey.PgnoLast() > pgnoSysMax ||
                m_spextdata.CpgExtent() > pgnoSysMax ||
                pgnoNull == m_spextkey.PgnoLast() ||
                ( m_spextkey.SppPool() != spp::ContinuousPool &&
                    0x0 == m_spextdata.CpgExtent() )
                )
            {
                return fFalse;
            }
            return fTrue;
        }

    private:
        CSPExtentNodeKDF( )
        {
            Invalidate();
        }
    public:

        //  So we can have const versions.
        CSPExtentNodeKDF(
            _In_ const  SPEXTKEY::E_SP_EXTENT_TYPE  eExtType,
            _In_ const  PGNO                        pgnoLast,
            _In_ const  CPG                         cpgExtent,
            _In_ const  SpacePool                   sppAvailPool = spp::AvailExtLegacyGeneralPool )
        {
            Invalidate();

            m_eSpExtType = eExtType;

            m_spextkey.Make( m_eSpExtType, pgnoLast, sppAvailPool );

            m_spextdata.Set( cpgExtent );

            m_kdf.key.prefix.Nullify();
            m_kdf.key.suffix.SetCb( m_spextkey.Cb() );
            m_kdf.key.suffix.SetPv( (VOID*)m_spextkey.Pv() );
            m_kdf.data.SetCb( m_spextdata.Cb() );
            m_kdf.data.SetPv( (VOID*)m_spextdata.Pv() );
            m_kdf.fFlags = 0;

            m_fShouldDeleteNode = fFalse;

            ASSERT_VALID( this );
        }

        class CSPExtentInfo;

        ERR ErrConsumeSpace( _In_ const PGNO pgnoConsume, _In_ const CPG cpgConsume = 1 )
        {
            ASSERT_VALID( this );

            Assert( pgnoConsume == PgnoFirst() );
            Assert( pgnoConsume <= PgnoLast() );
            if ( cpgConsume < 0 )
            {
                AssertSz( fFalse, "Consume space has a negative cpgConsume." );
                return ErrERRCheck( JET_errInternalError );
            }
            else if ( cpgConsume != 1 )
            {
                Assert( pgnoConsume + cpgConsume - 1 <= PgnoLast() );
            }

            m_spextdata.Set( CpgExtent() - cpgConsume );

            Assert( m_spextkey.FValid( m_eSpExtType, SPEXTKEY::fValidateData ) );

            if ( m_spextkey.SppPool() != spp::ContinuousPool &&
                    CpgExtent() == 0 )
            {
                m_fShouldDeleteNode = fTrue;
            }

            if ( !m_fShouldDeleteNode && !FValid( ) )
            {
                AssertSz( fFalse, "Consume space has corrupted data." );
                return ErrERRCheck( JET_errInternalError );
            }

            ASSERT_VALID( this );
            return JET_errSuccess;
        }

        ERR ErrUnconsumeSpace( _In_ const CPG cpgConsume )
        {
            ASSERT_VALID( this );

            m_spextdata.Set( CpgExtent() + cpgConsume );

            if ( !FValid( ) )
            {
                AssertSz( fFalse, "Unconsume space has corrupted data." );
                return ErrERRCheck( JET_errInternalError );
            }

            ASSERT_VALID( this );
            return JET_errSuccess;
        }


        BOOL FDelete( void ) const
        {
            ASSERT_VALID( this );
            return m_fShouldDeleteNode;
        }

        PGNO PgnoLast( void ) const
        {
            ASSERT_VALID( this );
            Assert( m_kdf.key.Cb() == sizeof(PGNO) || m_kdf.key.Cb() == sizeof(SPEXTKEY) );
            return m_spextkey.PgnoLast();
        }
        PGNO PgnoFirst( void ) const
        {
            ASSERT_VALID( this );
            return PgnoLast() - CpgExtent() + 1;
        }

        CPG CpgExtent( void ) const
        {
            ASSERT_VALID( this );
            return m_spextdata.CpgExtent();
        }

        const KEYDATAFLAGS * Pkdf( void ) const
        {
            ASSERT_VALID( this );
            return &m_kdf;
        }

        const KEYDATAFLAGS& Kdf( void ) const
        {
            ASSERT_VALID( this );
            return m_kdf;
        }

        BOOKMARK GetBm( FUCB * pfucb ) const
        {
            // pfucb may or may not be a space tree.  We only use it to
            // pass along for the PPIB to read.
            BOOKMARK bm;
            Assert( pgnoBadNews != m_spextkey.PgnoLast() );
            // no check for CPG.
            NDGetBookmarkFromKDF( pfucb, Kdf(), &bm );

            ASSERT_VALID( this );
            return bm;
        }

        const KEY& GetKey( ) const
        {
            ASSERT_VALID( this );
            return Kdf().key;
        }

        DATA GetData( ) const
        {
            ASSERT_VALID( this );
            Assert( !m_fShouldDeleteNode );
            return Kdf().data;
        }

        SpacePool SppPool( ) const
        {
            ASSERT_VALID( this );
            return m_spextkey.SppPool();
        }

};


ERR ErrSPIExtentLastPgno( _In_ const FUCB * pfucb, _Out_ PGNO * ppgnoLast )
{
    const CSPExtentInfo spext( pfucb );
    CallS( spext.ErrCheckCorrupted() );
    *ppgnoLast = spext.PgnoLast();
    return JET_errSuccess;
}

ERR ErrSPIExtentFirstPgno( _In_ const FUCB * pfucb, _Out_ PGNO * ppgnoFirst )
{
    const CSPExtentInfo spext( pfucb );
    CallS( spext.ErrCheckCorrupted() );
    *ppgnoFirst = spext.PgnoFirst();
    return JET_errSuccess;
}

ERR ErrSPIExtentCpg( _In_ const FUCB * pfucb, _Out_ CPG * pcpgSize )
{
    const CSPExtentInfo spext( pfucb );
    CallS( spext.ErrCheckCorrupted() );
    *pcpgSize = spext.CpgExtent();
    return JET_errSuccess;
}

ERR ErrSPIGetExtentInfo( _In_ const FUCB * pfucb, _Out_ PGNO * ppgnoLast, _Out_ CPG * pcpgSize, _Out_ SpacePool * psppPool )
{
    const CSPExtentInfo spext( pfucb );
    CallS( spext.ErrCheckCorrupted() );
    *ppgnoLast = spext.PgnoLast();
    *pcpgSize = spext.CpgExtent();
    *psppPool = spext.SppPool();
    return JET_errSuccess;
}

ERR ErrSPIGetExtentInfo( _In_ const KEYDATAFLAGS * pkdf, _Out_ PGNO * ppgnoLast, _Out_ CPG * pcpgSize, _Out_ SpacePool * psppPool )
{
    const CSPExtentInfo spext( pkdf );
    CallS( spext.ErrCheckCorrupted() );
    *ppgnoLast = spext.PgnoLast();
    *pcpgSize = spext.CpgExtent();
    *psppPool = spext.SppPool();
    return JET_errSuccess;
}


ERR ErrSPREPAIRValidateSpaceNode(
    _In_ const  KEYDATAFLAGS * pkdf,
    _Out_       PGNO *          ppgnoLast,
    _Out_       CPG *           pcpgExtent,
    _Out_       PCWSTR *        pwszPoolName )
{
    SpacePool spp;

    ERR err = ErrSPIREPAIRValidateSpaceNode(
        pkdf,
        ppgnoLast,
        pcpgExtent,
        &spp
        );

    if ( JET_errSuccess <= err )
    {
        *pwszPoolName = WszPoolName( spp );
    }

    return err;
}

ERR ErrSPIREPAIRValidateSpaceNode(
    _In_ const  KEYDATAFLAGS * pkdf,
    _Out_       PGNO *          ppgnoLast,
    _Out_       CPG *           pcpgExtent,
    _Out_       SpacePool *     psppPool )
{
    ERR err = JET_errSuccess;
    // I've created a constructor for the rare case we don't have the pfucb to 
    //  validate against, only kdf.  But it would be better to get repair to
    //  pass it in.
    const CSPExtentInfo spext( pkdf );

    Assert( pkdf );
    Assert( ppgnoLast );
    Assert( pcpgExtent );

    Call( spext.ErrCheckCorrupted() );

    *ppgnoLast = spext.PgnoLast();
    *pcpgExtent = spext.CpgExtent();
    *psppPool = spext.SppPool();

HandleError:

    return err;
}

INLINE BOOL FSPIIsSmall( const FCB * const pfcb )
{
    #ifdef DEBUG
    Assert( pfcb->FSpaceInitialized() );
    if ( pfcb->PgnoOE() == pgnoNull )
    {
        Assert( pfcb->PgnoAE() == pgnoNull );
        Assert( FSPValidAllocPGNO( pfcb->PgnoFDP() ) );
        if ( !FFMPIsTempDB( pfcb->Ifmp() ) )
        {
            //  Temp DB doesn't seem to have catalog, so we can't test this.
            Assert( !FCATBaseSystemFDP( pfcb->PgnoFDP() ) );
        }
    }
    else
    {
        Assert( pfcb->PgnoAE() != pgnoNull );
        Assert( pfcb->PgnoOE() + 1 == pfcb->PgnoAE() );
    }
    #endif

    return pfcb->PgnoOE() == pgnoNull;
}

//  write latch page before update
//
LOCAL VOID SPIUpgradeToWriteLatch( FUCB *pfucb )
{
    CSR     *pcsrT;

    if ( Pcsr( pfucb )->Latch() == latchNone )
    {
        //  latch upgrades only work on root
        //
        Assert( pfucb->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );

        //  must want to upgrade latch in pcsrRoot
        //
        pcsrT = pfucb->pcsrRoot;
        Assert( latchRIW == pcsrT->Latch() || latchWrite == pcsrT->Latch() /* vertical split on 2-level tree while still in small space mode */ );
    }
    else
    {
        //  latch upgrades only work on root
        //
        Assert( Pcsr( pfucb ) == pfucb->pcsrRoot );
        Assert( Pcsr( pfucb )->Pgno() == PgnoFDP( pfucb ) );
        pcsrT = &pfucb->csr;
    }

    if ( pcsrT->Latch() != latchWrite )
    {
        Assert( pcsrT->Latch() == latchRIW );
        pcsrT->UpgradeFromRIWLatch();
    }
    else
    {
        //  vertical split on 2-level tree while still in small / single-extent space mode
#ifdef DEBUG
        LINE    lineSpaceHeader;
        NDGetPtrExternalHeader( pcsrT->Cpage(), &lineSpaceHeader, noderfSpaceHeader );
        Assert( sizeof( SPACE_HEADER ) == lineSpaceHeader.cb );
        SPACE_HEADER * psph = (SPACE_HEADER*)lineSpaceHeader.pv;
        Assert( psph->FSingleExtent() );            //  current small / single-extent space
        Assert( !psph->FMultipleExtent() );
        Assert( pcsrT->Cpage().FRootPage() );       //  currently on root of a 2-level or 3-level tree
        Assert( pcsrT->Cpage().FInvisibleSons() );
#endif
    }
}


//  opens a cursor on Avail/owned extent of given tree
//  subsequent BT operations on cursor will place
//  cursor on root page of available extent
//  this logic is embedded in ErrBTIGotoRootPage
//
ERR ErrSPIOpenAvailExt( PIB *ppib, FCB *pfcb, FUCB **ppfucbAE )
{
    ERR     err;
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = pfcb->TCE( fTrue );
    tcScope->SetDwEngineObjid( pfcb->ObjidFDP() );
    tcScope->iorReason.SetIort( iortSpace );

    //  open cursor on FCB
    //  label it as a space cursor
    //
    CallR( ErrBTOpen( ppib, pfcb, ppfucbAE, fFalse ) );
    FUCBSetAvailExt( *ppfucbAE );
    FUCBSetIndex( *ppfucbAE );
    (*ppfucbAE)->pfucbLatchHolderForSpace = pfucbNil;
    Assert( pfcb->FSpaceInitialized() );
    Assert( pfcb->PgnoAE() != pgnoNull );

    return err;
}


ERR ErrSPIOpenAvailExt( FUCB *pfucb, FUCB **ppfucbAE )
{
    ERR err = ErrSPIOpenAvailExt( pfucb->ppib, pfucb->u.pfcb, ppfucbAE );

    if ( err >= JET_errSuccess )
    {
        (*ppfucbAE)->pfucbLatchHolderForSpace = pfucb;
    }

    return err;
}


ERR ErrSPIOpenOwnExt( PIB *ppib, FCB *pfcb, FUCB **ppfucbOE )
{
    ERR err;
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = pfcb->TCE( fTrue );
    tcScope->SetDwEngineObjid( pfcb->ObjidFDP() );
    tcScope->iorReason.SetIort( iortSpace );

    //  open cursor on FCB
    //  label it as a space cursor
    //
    CallR( ErrBTOpen( ppib, pfcb, ppfucbOE, fFalse ) );
    FUCBSetOwnExt( *ppfucbOE );
    FUCBSetIndex( *ppfucbOE );
    (*ppfucbOE)->pfucbLatchHolderForSpace = pfucbNil;
    Assert( pfcb->FSpaceInitialized() );
    Assert( !FSPIIsSmall( pfcb ) );

    return err;
}


ERR ErrSPIOpenOwnExt(
    FUCB *pfucb,
    FUCB **ppfucbOE )
{
    ERR err = ErrSPIOpenOwnExt( pfucb->ppib, pfucb->u.pfcb, ppfucbOE );

    if ( err >= JET_errSuccess )
    {
        (*ppfucbOE)->pfucbLatchHolderForSpace = pfucb;
    }

    return err;
}


//  gets pgno of last page owned by database
//
ERR ErrSPGetLastPgno( _Inout_ PIB * ppib, _In_ const IFMP ifmp, _Out_ PGNO * ppgno )
{
    ERR err;
    EXTENTINFO extinfo;

    Call( ErrSPGetLastExtent( ppib, ifmp, &extinfo ) );
    *ppgno = extinfo.PgnoLast();

HandleError:
    return err;
}


//  gets last extent owned by database
//
ERR ErrSPGetLastExtent( _Inout_ PIB * ppib, _In_ const IFMP ifmp, _Out_ EXTENTINFO * pextinfo )
{
    ERR     err;
    FUCB    *pfucb = pfucbNil;
    FUCB    *pfucbOE = pfucbNil;
    DIB     dib;

    CallR( ErrBTOpen( ppib, pgnoSystemRoot, ifmp, &pfucb, openNormal, fTrue ) );
    Assert( pfucbNil != pfucb );

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortSpace );

    if ( PinstFromPpib( ppib )->FRecovering() && !pfucb->u.pfcb->FSpaceInitialized() )
    {
        Assert( PinstFromPpib( ppib )->FRecovering() );
        Assert( pfucb->u.pfcb->FInitedForRecovery() );

        //  pgnoOE and pgnoAE need to be obtained
        //
        Call( ErrSPInitFCB( pfucb ) );

        pfucb->u.pfcb->Lock();
        pfucb->u.pfcb->ResetInitedForRecovery();
        pfucb->u.pfcb->Unlock();
    }
    Assert( pfucb->u.pfcb->FSpaceInitialized() );
    Assert( pfucb->u.pfcb->PgnoOE() != pgnoNull );

    Call( ErrSPIOpenOwnExt( pfucb, &pfucbOE ) );

    dib.dirflag = fDIRNull;
    dib.pos     = posLast;
    dib.pbm     = NULL;
    err = ErrBTDown( pfucbOE, &dib, latchReadTouch );

    // We must have at least one owned extent.
    if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
    {
        FireWall( "GetDbLastExtNoOwned" );
        Error( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
    }

    Call( err );

    {
    const CSPExtentInfo spLastOE( pfucbOE );
    Assert( spLastOE.FIsSet() && ( spLastOE.CpgExtent() > 0 ) );
    pextinfo->pgnoLastInExtent = spLastOE.PgnoLast();
    pextinfo->cpgExtent = spLastOE.CpgExtent();
    OSTraceFMP(
        ifmp,
        JET_tracetagSpaceInternal,
        OSFormat(
            "%hs: ifmp=%d now has pgnoLast=%lu, cpgExtent=%ld",
            __FUNCTION__,
            ifmp,
            pextinfo->PgnoLast(),
            pextinfo->CpgExtent() ) );
    }

HandleError:
    if ( pfucbOE != pfucbNil )
    {
        BTClose( pfucbOE );
    }

    Assert ( pfucb != pfucbNil );
    BTClose( pfucb );

    return err;
}


//  Validate I have not unintentionally changed SPACE_HEADER size.
C_ASSERT( sizeof(SPACE_HEADER) == 16 );

LOCAL VOID SPIInitFCB( FUCB * pfucb, const BOOL fDeferredInit )
{
    CSR             * pcsr  = ( fDeferredInit ? pfucb->pcsrRoot : Pcsr( pfucb ) );
    FCB             * pfcb  = pfucb->u.pfcb;

    Assert( pcsr->FLatched() );

    //  need to acquire FCB lock because that's what protects the Flags
    pfcb->Lock();

    if ( !pfcb->FSpaceInitialized() )
    {
        //  get external header
        //
        NDGetExternalHeader ( pfucb, pcsr, noderfSpaceHeader );
        Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
        const SPACE_HEADER * const psph = reinterpret_cast <const SPACE_HEADER * const> ( pfucb->kdfCurr.data.Pv() );

        if ( psph->FSingleExtent() )
        {
            pfcb->SetPgnoOE( pgnoNull );
            pfcb->SetPgnoAE( pgnoNull );
        }
        else
        {
            pfcb->SetPgnoOE( psph->PgnoOE() );
            pfcb->SetPgnoAE( psph->PgnoAE() );
            Assert( pfcb->PgnoAE() == pfcb->PgnoOE() + 1 );
        }

        if ( !fDeferredInit )
        {
            Assert( pfcb->FUnique() );      // FCB always initialised as unique
            if ( psph->FNonUnique() )
                pfcb->SetNonUnique();
        }
        Assert( !!psph->FNonUnique() == !pfcb->FUnique() );

        pfcb->SetSpaceInitialized();

        Assert( !!psph->FSingleExtent() == !!FSPIIsSmall( pfcb ) );
    }

    pfcb->Unlock();

    return;
}


//  initializes FCB with pgnoAE and pgnoOE
//
ERR ErrSPInitFCB( _Inout_ FUCB * const pfucb )
{
    ERR             err;
    FCB             *pfcb   = pfucb->u.pfcb;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->iorReason.SetIort( iortSpace );
    tcScope->SetDwEngineObjid( pfcb->ObjidFDP() );

    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !FFUCBSpace( pfucb ) );

    //  goto root page of tree
    //
    err = ErrBTIGotoRoot( pfucb, latchReadTouch );
    if ( err < 0 )
    {
        if ( g_fRepair )
        {
            //  ignore the error
            pfcb->SetPgnoOE( pgnoNull );
            pfcb->SetPgnoAE( pgnoNull );

            Assert( objidNil == pfcb->ObjidFDP() );

            err = JET_errSuccess;
        }
    }
    else
    {
        //  get objidFDP from root page, FCB can only be set once

        Assert( objidNil == pfcb->ObjidFDP()
            || ( PinstFromIfmp( pfucb->ifmp )->FRecovering() && pfcb->ObjidFDP() == Pcsr( pfucb )->Cpage().ObjidFDP() ) );
        pfcb->SetObjidFDP( Pcsr( pfucb )->Cpage().ObjidFDP() );

        SPIInitFCB( pfucb, fFalse );

        BTUp( pfucb );
    }

    return err;
}


ERR ErrSPDeferredInitFCB( _Inout_ FUCB * const pfucb )
{
    ERR             err;
    FCB             * pfcb  = pfucb->u.pfcb;
    FUCB            * pfucbT    = pfucbNil;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortSpace );

    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !FFUCBSpace( pfucb ) );

    //  goto root page of tree
    //
    CallR( ErrBTIOpenAndGotoRoot(
                pfucb->ppib,
                pfcb->PgnoFDP(),
                pfucb->ifmp,
                &pfucbT ) );
    Assert( pfucbNil != pfucbT );
    Assert( pfucbT->u.pfcb == pfcb );
    Assert( pcsrNil != pfucbT->pcsrRoot );

    if ( !pfcb->FSpaceInitialized() )
    {
        SPIInitFCB( pfucbT, fTrue );
    }

    SPIValidateCpgOwnedAndAvail( pfucbT );

    pfucbT->pcsrRoot->ReleasePage();
    pfucbT->pcsrRoot = pcsrNil;

    Assert( pfucbNil != pfucbT );
    BTClose( pfucbT );

    return JET_errSuccess;
}

INLINE const SPACE_HEADER * PsphSPIRootPage( FUCB* pfucb )
{
    const SPACE_HEADER  *psph;

    AssertSPIPfucbOnRoot( pfucb );

    NDGetExternalHeader( pfucb, pfucb->pcsrRoot, noderfSpaceHeader );
    Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
    psph = reinterpret_cast <const SPACE_HEADER *> ( pfucb->kdfCurr.data.Pv() );

    return psph;
}

//  get pgnoFDP of parentFDP of this tree
//
PGNO PgnoSPIParentFDP( FUCB *pfucb )
{
    return PsphSPIRootPage( pfucb )->PgnoParent();
}

//  whether or not this is the root OE or AE
//
BOOL FSPIsRootSpaceTree( const FUCB * const pfucb )
{
    return ( ObjidFDP( pfucb ) == objidSystemRoot && FFUCBSpace( pfucb ) );
}

INLINE SPLIT_BUFFER *PspbufSPISpaceTreeRootPage( FUCB *pfucb, CSR *pcsr )
{
    SPLIT_BUFFER    *pspbuf;

    AssertSPIPfucbOnSpaceTreeRoot( pfucb, pcsr );

    NDGetExternalHeader( pfucb, pcsr, noderfWhole );
    Assert( sizeof( SPLIT_BUFFER ) == pfucb->kdfCurr.data.Cb() );
    pspbuf = reinterpret_cast <SPLIT_BUFFER *> ( pfucb->kdfCurr.data.Pv() );

    return pspbuf;
}

INLINE VOID SPITraceSplitBufferMsg( const FUCB * const pfucb, const CHAR * const szMsg )
{
    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagSpaceInternal,
        OSFormat(
            "%hs: Database '%ws': %s SplitBuffer for a Btree [0x%x:0x%x:%lu].",
            __FUNCTION__,
            PfmpFromIfmp( pfucb->ifmp )->WszDatabaseName(),
            szMsg,
            pfucb->ifmp,
            ObjidFDP( pfucb ),
            PgnoFDP( pfucb ) ) );
}

LOCAL ERR ErrSPIFixSpaceTreeRootPage( FUCB *pfucb, SPLIT_BUFFER **ppspbuf )
{
    ERR         err         = JET_errSuccess;
    const BOOL  fAvailExt   = FFUCBAvailExt( pfucb );

    AssertTrack( fFalse, "UnexpectedSpBufFixup" );

    AssertSPIPfucbOnSpaceTreeRoot( pfucb, Pcsr( pfucb ) );
    Assert( latchRIW == Pcsr( pfucb )->Latch() );

#ifdef DEBUG
    const BOOL  fNotEnoughPageSpace = ( Pcsr( pfucb )->Cpage().CbPageFree() < ( g_cbPage * 3 / 4 ) );
#else
    const BOOL  fNotEnoughPageSpace = ( Pcsr( pfucb )->Cpage().CbPageFree() < sizeof(SPLIT_BUFFER) );
#endif

    if ( fNotEnoughPageSpace )
    {
        if ( NULL == pfucb->u.pfcb->Psplitbuf( fAvailExt ) )
        {
            CallR( pfucb->u.pfcb->ErrEnableSplitbuf( fAvailExt ) );
            SPITraceSplitBufferMsg( pfucb, "Allocated" );
        }
        *ppspbuf = pfucb->u.pfcb->Psplitbuf( fAvailExt );
        Assert( NULL != *ppspbuf );
    }
    else
    {
        const BOOL      fSplitbufDangling   = ( NULL != pfucb->u.pfcb->Psplitbuf( fAvailExt ) );
        SPLIT_BUFFER    spbuf;
        DATA            data;
        Assert( 0 == pfucb->kdfCurr.data.Cb() );

        //  if in-memory copy of split buffer exists, move it to the page
        if ( fSplitbufDangling )
        {
            memcpy( &spbuf, pfucb->u.pfcb->Psplitbuf( fAvailExt ), sizeof(SPLIT_BUFFER) );
        }
        else
        {
            memset( &spbuf, 0, sizeof(SPLIT_BUFFER) );
        }

        data.SetPv( &spbuf );
        data.SetCb( sizeof(spbuf) );

        Pcsr( pfucb )->UpgradeFromRIWLatch();
        err = ErrSPIWrappedNDSetExternalHeader(
                    pfucb,
                    Pcsr( pfucb ),
                    &data,
                    ( pfucb->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    noderfWhole );
        Pcsr( pfucb )->Downgrade( latchRIW );
        CallR( err );

        if ( fSplitbufDangling )
        {
            //  split buffer successfully moved to page, destroy in-memory copy
            pfucb->u.pfcb->DisableSplitbuf( fAvailExt );
            SPITraceSplitBufferMsg( pfucb, "Persisted" );
        }

        //  re-cache external header
        NDGetExternalHeader( pfucb, Pcsr( pfucb ), noderfWhole );

        *ppspbuf = reinterpret_cast <SPLIT_BUFFER *> ( pfucb->kdfCurr.data.Pv() );
    }

    return err;
}

INLINE ERR ErrSPIGetSPBuf( FUCB *pfucb, SPLIT_BUFFER **ppspbuf )
{
    ERR err;

    AssertSPIPfucbOnSpaceTreeRoot( pfucb, Pcsr( pfucb ) );

    NDGetExternalHeader( pfucb, Pcsr( pfucb ), noderfWhole );

    if ( sizeof( SPLIT_BUFFER ) != pfucb->kdfCurr.data.Cb() )
    {
        AssertTrack( fFalse, "PersistedSpBufMissing" );
        err = ErrSPIFixSpaceTreeRootPage( pfucb, ppspbuf );
    }
    else
    {
        Assert( NULL == pfucb->u.pfcb->Psplitbuf( FFUCBAvailExt( pfucb ) ) );
        *ppspbuf = reinterpret_cast <SPLIT_BUFFER *> ( pfucb->kdfCurr.data.Pv() );
        err = JET_errSuccess;
    }

    return err;
}


ERR ErrSPIGetSPBufUnlatched( __inout FUCB * pfucbSpace, __out_bcount(sizeof(SPLIT_BUFFER)) SPLIT_BUFFER * pspbuf )
{
    ERR err = JET_errSuccess;
    LINE line;

    Assert( pspbuf );
    Assert( pfucbSpace );
    Assert( !Pcsr( pfucbSpace )->FLatched() );

    CallR( ErrBTIGotoRoot( pfucbSpace, latchReadNoTouch ) );
    Assert( Pcsr( pfucbSpace )->FLatched() );

    NDGetPtrExternalHeader( Pcsr( pfucbSpace )->Cpage(), &line, noderfWhole );
    if (sizeof( SPLIT_BUFFER ) == line.cb)
    {
        UtilMemCpy( (void*)pspbuf, line.pv, sizeof( SPLIT_BUFFER ) );
    }
    else
    {
        memset( (void*)pspbuf, 0, sizeof(SPLIT_BUFFER) );
        Assert( 0 == pspbuf->CpgBuffer1() );
        Assert( 0 == pspbuf->CpgBuffer2() );
    }

    BTUp( pfucbSpace );
    Assert( !Pcsr( pfucbSpace )->FLatched() );

    return err;
}


LOCAL ERR ErrSPISPBufContains(
    _In_ PIB* const ppib,
    _In_ FCB* const pfcbFDP,
    _In_ const PGNO pgno,
    _In_ const BOOL fOwnExt,
    _Out_ BOOL* const pfContains )
{
    ERR err = JET_errSuccess;
    FUCB* pfucbSpace = pfucbNil;
    SPLIT_BUFFER* pspbuf;

    if ( fOwnExt )
    {
        Call( ErrSPIOpenOwnExt( ppib, pfcbFDP, &pfucbSpace ) );
    }
    else
    {
        Call( ErrSPIOpenAvailExt( ppib, pfcbFDP, &pfucbSpace ) );
    }
    Call( ErrBTIGotoRoot( pfucbSpace, latchRIW ) );
    Call( ErrSPIGetSPBuf( pfucbSpace, &pspbuf ) );
    *pfContains = ( ( pgno >= ( pspbuf->PgnoLastBuffer1() - pspbuf->CpgBuffer1() + 1 ) ) &&
                        ( pgno <= pspbuf->PgnoLastBuffer1() ) ) ||
                  ( ( pgno >= ( pspbuf->PgnoLastBuffer2() - pspbuf->CpgBuffer2() + 1 ) ) &&
                        ( pgno <= pspbuf->PgnoLastBuffer2() ) );

HandleError:
    if ( pfucbSpace != pfucbNil )
    {
        BTUp( pfucbSpace );
        BTClose( pfucbSpace );
        pfucbSpace = pfucbNil;
    }

    return err;
}


INLINE VOID SPIDirtyAndSetMaxDbtime( CSR *pcsr1, CSR *pcsr2, CSR *pcsr3 )
{
#ifdef DEBUG
    Assert( pcsr1->Latch() == latchWrite );
    Assert( pcsr2->Latch() == latchWrite );
    Assert( pcsr3->Latch() == latchWrite );

    const IFMP ifmp = pcsr1->Cpage().Ifmp();
    Assert( pcsr2->Cpage().Ifmp() == ifmp );
    Assert( pcsr3->Cpage().Ifmp() == ifmp );

    // Implicitly created DBs start from a dbtime of 0 and pages start getting
    // 1 and above as CPAGE::ErrGetNewPreInitPage() is called to produce new pages
    // during an implicit create DB redo operation.
    const BOOL fRedoImplicitCreateDb = ( PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() == fRecoveringRedo ) &&
                                  g_rgfmp[ ifmp ].FCreatingDB() &&
                                  !FFMPIsTempDB( ifmp );
    const DBTIME dbtimeMaxBefore = max( max( pcsr1->Dbtime(), pcsr2->Dbtime() ), pcsr3->Dbtime() );
    if ( fRedoImplicitCreateDb )
    {
        Expected( pcsr1->Dbtime() < dbtimeStart );
        Expected( pcsr2->Dbtime() < dbtimeStart );
        Expected( pcsr3->Dbtime() < dbtimeStart );
        Expected( pcsr1->Dbtime() != pcsr2->Dbtime() );
        Expected( pcsr2->Dbtime() != pcsr3->Dbtime() );
        Expected( pcsr1->Dbtime() != pcsr3->Dbtime() );
        Expected( dbtimeMaxBefore -
                min( min( pcsr1->Dbtime(), pcsr2->Dbtime() ), pcsr3->Dbtime() ) // dbtimeMin
                == 2 );
    }
    else if ( !g_fRepair )
    {
        Expected( pcsr1->Dbtime() >= dbtimeStart );
        Expected( pcsr2->Dbtime() >= dbtimeStart );
        Expected( pcsr3->Dbtime() >= dbtimeStart );
    }
#endif

    pcsr1->Dirty();
    pcsr2->Dirty();
    pcsr3->Dirty();

    const DBTIME dbtimeMax = max( max( pcsr1->Dbtime(), pcsr2->Dbtime() ), pcsr3->Dbtime() );

#ifdef DEBUG
    if ( fRedoImplicitCreateDb )
    {
        Expected( dbtimeMax < dbtimeStart );
        Expected( ( dbtimeMax - dbtimeMaxBefore ) == 3 );
        Expected( pcsr1->Dbtime() != pcsr2->Dbtime() );
        Expected( pcsr2->Dbtime() != pcsr3->Dbtime() );
        Expected( pcsr1->Dbtime() != pcsr3->Dbtime() );
        Expected( dbtimeMax -
                min( min( pcsr1->Dbtime(), pcsr2->Dbtime() ), pcsr3->Dbtime() ) // dbtimeMin
                == 2 );
    }
    else if ( !g_fRepair )
    {
        Expected( dbtimeMax >= dbtimeStart );
    }
#endif

    //  set dbtime in the three pages
    //
    pcsr1->SetDbtime( dbtimeMax );
    pcsr2->SetDbtime( dbtimeMax );
    pcsr3->SetDbtime( dbtimeMax );
}


INLINE VOID SPIInitSplitBuffer( FUCB *pfucb, CSR *pcsr )
{
    //  copy dummy split buffer into external header
    //
    DATA            data;
    SPLIT_BUFFER    spbuf;

    memset( &spbuf, 0, sizeof(SPLIT_BUFFER) );

    Assert( FBTIUpdatablePage( *pcsr ) );       //  check is already performed by caller
    Assert( latchWrite == pcsr->Latch() );
    Assert( pcsr->FDirty() );
#ifdef DEBUG
    if ( g_fRepair )
    {
        // In repair, it's normal that we're initializing
        // a split buffer for a new space tree, but we're
        // being called with an FUCB for the parent of the
        // FCB being created or repaired, not the FUCB for
        // the space tree being created or the owner of the
        // space tree being created.  We only use the FUCB
        // to get at the PIB (all writing is done to the
        // CSR passed in), so this is OK, but untidy and
        // we avoid it EXCEPT during repair.
    }
    else
    {
        // The root pages of the space trees have been set on the FCB.
        Assert( pgnoNull != PgnoAE( pfucb ) );
        Assert( pgnoNull != PgnoOE( pfucb ) );

        // And the CSR passed in is for the root of one of the space trees.
        Assert( pcsr->Pgno() == PgnoAE( pfucb ) ||  pcsr->Pgno() == PgnoOE( pfucb ) );
    }
#endif

    data.SetPv( (VOID *)&spbuf );
    data.SetCb( sizeof(spbuf) );
    NDSetExternalHeader( pfucb, pcsr, noderfWhole, &data );
}

//  Creates extent tree for either owned or available extents.
//  Note that the pfucb passed in here is probably for the parent
//  of a new FCB being created.  pcsr is the page for the root of the
//  new extent tree and PgnoAE( pfcub ) and PgnoOE( pfucb ) are NOT
//  being written to.  We only use pfucb to pass on and in the end
//  only use it to get the ppib.
//

VOID SPICreateExtentTree( FUCB *pfucb, CSR *pcsr, PGNO pgnoLast, CPG cpgExtent, BOOL fAvail )
{
    if ( !FBTIUpdatablePage( *pcsr ) )
    {
        //  page does not redo
        //
        return;
    }

    //  cannot reuse a deferred closed cursor
    //
    Assert( !FFUCBVersioned( pfucb ) );
    Assert( !FFUCBSpace( pfucb ) );
    Assert( pgnoNull != PgnoAE( pfucb ) || g_fRepair );
    Assert( pgnoNull != PgnoOE( pfucb ) || g_fRepair );
    Assert( latchWrite == pcsr->Latch() );
    Assert( pcsr->FDirty() );

    Assert( pcsr->FDirty() );

    // Writing is done to pcsr, not to pfucb currency.
    SPIInitSplitBuffer( pfucb, pcsr );

    Assert( 0 == pcsr->Cpage().Clines() );
    pcsr->SetILine( 0 );

    //  goto Root before insert would place
    //  cursor on appropriate space tree
    //
    if ( cpgExtent != 0 )
    {

        //  We can use raw extent node, because we always want to insert
        //  a "legacy" format AE node here.
        //
        //  We use legacy here for two reasons, one this is issued during create
        //  DB so changing it makes the create DB versioning problem worse, and
        //  two the legacy nodes are also the general pool (at least today), and
        //  as such if we don't have a specific pool or purpose to associate these 
        //  with then we put them in the general (aka legacy) pool.
        //
        //  Note that we're inserting nodes into the root page of a new space tree,
        //  but not tracking that space in any FCB.  We'll do that later.
        //
        const CSPExtentNodeKDF      spextnode( fAvail ?
                                                    SPEXTKEY::fSPExtentTypeAE :
                                                    SPEXTKEY::fSPExtentTypeOE,
                                                pgnoLast, cpgExtent );
        //  Curio, this is the very first NDInsert() call.  During create DB, we 
        //  first put in an OE.
        //  Update: I think we may set an external header (SPACE_HEADER) on
        //  the pgnoSystemRoot before this, but its not an NDInsert(), but
        //  space owns that too.
        //  Space is so cool.
        //  Note that we don't update the page count cache here.  This is only called
        //  under a stack that is creating a new tree, so there IS no entry to update
        //  yet.  Up above, we'll create an appropriate initial entry.
        SPIWrappedNDInsert(
            pfucb,
            pcsr,
            spextnode.Pkdf() );
    }
    else
    {
        //  avail has already been set up as an empty tree
        //
        Assert( fAvail );
    }

    return;
}


VOID SPIInitPgnoFDP( FUCB *pfucb, CSR *pcsr, const SPACE_HEADER& sph )
{
    if ( !FBTIUpdatablePage( *pcsr ) )
    {
        //  page does not redo
        //
        return;
    }

    Assert( latchWrite == pcsr->Latch() );
    Assert( pcsr->FDirty() );
    Assert( pcsr->Pgno() == PgnoFDP( pfucb ) || g_fRepair );

    //  copy space information into external header
    //
    DATA    data;

    data.SetPv( (VOID *)&sph );
    data.SetCb( sizeof(sph) );
    NDSetExternalHeader( pfucb, pcsr, noderfSpaceHeader, &data );
}


ERR ErrSPIImproveLastAlloc( FUCB * pfucb, const SPACE_HEADER * const psph, CPG cpgLast )
{
    ERR             err = JET_errSuccess;

    AssertSPIPfucbOnRoot( pfucb );

    if ( psph->FSingleExtent() )
    {
        //  This tree is too young for this kind of thing, let it grow normally a little while.
        return JET_errSuccess;
    }

    Assert( psph->FMultipleExtent() );

    SPACE_HEADER sphSet;
    memcpy( &sphSet, psph, sizeof(sphSet) );

    sphSet.SetCpgLast( cpgLast );

    DATA data;
    data.SetPv( &sphSet );
    data.SetCb( sizeof(sphSet) );

    if ( latchWrite != pfucb->pcsrRoot->Latch() )
    {
        // as long as this is latch state, should be ok ... b/c someone 
        //  above us should own downgrade to latchNone.
        Assert( pfucb->pcsrRoot->Latch() == latchRIW );
        SPIUpgradeToWriteLatch( pfucb );
    }
    Assert( pfucb->pcsrRoot->Latch() == latchWrite );

    CallR( ErrSPIWrappedNDSetExternalHeader(
               pfucb,
               pfucb->pcsrRoot,
               &data,
               ( pfucb->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
               noderfSpaceHeader ) );

    return err;
}


//  performs creation of multiple extent FDP
//
VOID SPIPerformCreateMultiple( FUCB *pfucb, CSR *pcsrFDP, CSR *pcsrOE, CSR *pcsrAE, PGNO pgnoPrimary, const SPACE_HEADER& sph )
{
    const CPG   cpgPrimary = sph.CpgPrimary();
    const PGNO  pgnoLast = pgnoPrimary;
    Assert( g_fRepair || pgnoLast == PgnoFDP( pfucb ) + cpgPrimary - 1 );

    Assert( FBTIUpdatablePage( *pcsrFDP ) );
    Assert( FBTIUpdatablePage( *pcsrOE ) );
    Assert( FBTIUpdatablePage( *pcsrAE ) );

    //  insert space header into FDP
    //
    SPIInitPgnoFDP( pfucb, pcsrFDP, sph );

    //  add allocated extents to OwnExt
    //
    SPICreateExtentTree( pfucb, pcsrOE, pgnoLast, cpgPrimary, fFalse );

    //  add extent minus pages allocated to AvailExt
    //
    SPICreateExtentTree( pfucb, pcsrAE, pgnoLast, cpgPrimary - cpgMultipleExtentMin, fTrue );

    return;
}


//  ================================================================
ERR ErrSPCreateMultiple(
    FUCB        *pfucb,
    const PGNO  pgnoParent,
    const PGNO  pgnoFDP,
    const OBJID objidFDP,
    const PGNO  pgnoOE,
    const PGNO  pgnoAE,
    const PGNO  pgnoPrimary,
    const CPG   cpgPrimary,
    const BOOL  fUnique,
    const ULONG fPageFlags )
//  ================================================================
{
    ERR             err;
    CSR             csrOE;
    CSR             csrAE;
    CSR             csrFDP;
    SPACE_HEADER    sph;
    LGPOS           lgpos;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortSpace );

    Assert( objidFDP == ObjidFDP( pfucb ) || g_fRepair );
    Assert( objidFDP != objidNil );

    Assert( !( fPageFlags & CPAGE::fPageRoot ) );
    Assert( !( fPageFlags & CPAGE::fPageLeaf ) );
    Assert( !( fPageFlags & CPAGE::fPageParentOfLeaf ) );
    Assert( !( fPageFlags & CPAGE::fPageEmpty ) );
    Assert( !( fPageFlags & CPAGE::fPagePreInit ) );
    Assert( !( fPageFlags & CPAGE::fPageSpaceTree) );

    const BOOL fLogging = g_rgfmp[ pfucb->ifmp ].FLogOn();

    //  init space trees and root page
    //
    Call( csrOE.ErrGetNewPreInitPage( pfucb->ppib,
                                      pfucb->ifmp,
                                      pgnoOE,
                                      objidFDP,
                                      fLogging ) );
    csrOE.ConsumePreInitPage( ( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree ) & ~CPAGE::fPageRepair );
    Assert( csrOE.Latch() == latchWrite );

    Call( csrAE.ErrGetNewPreInitPage( pfucb->ppib,
                                      pfucb->ifmp,
                                      pgnoAE,
                                      objidFDP,
                                      fLogging ) );
    csrAE.ConsumePreInitPage( ( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree ) & ~CPAGE::fPageRepair );
    Assert( csrAE.Latch() == latchWrite );

    Call( csrFDP.ErrGetNewPreInitPage( pfucb->ppib,
                                       pfucb->ifmp,
                                       pgnoFDP,
                                       objidFDP,
                                       fLogging ) );
    csrFDP.ConsumePreInitPage( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf );
    Assert( csrFDP.Latch() == latchWrite );

    //  set max dbtime for the three pages in all the pages
    //
    SPIDirtyAndSetMaxDbtime( &csrFDP, &csrOE, &csrAE );

    sph.SetCpgPrimary( cpgPrimary );
    sph.SetPgnoParent( pgnoParent );

    Assert( sph.FSingleExtent() );      // initialised with these defaults
    Assert( sph.FUnique() );

    sph.SetMultipleExtent();

    if ( !fUnique )
    {
        sph.SetNonUnique();
    }

    sph.SetPgnoOE( pgnoOE );

    // This only runs during REDO during create DB.
    Assert( ( PinstFromPfucb( pfucb )->m_plog->FRecoveringMode() != fRecoveringRedo ) ||
            ( !g_rgfmp[ pfucb->ifmp ].FLogOn() && g_rgfmp[ pfucb->ifmp ].FCreatingDB() ) );

    //  log operation
    //
    Call( ErrLGCreateMultipleExtentFDP( pfucb, &csrFDP, &sph, fPageFlags, &lgpos ) );
    csrFDP.Cpage().SetLgposModify( lgpos );
    csrOE.Cpage().SetLgposModify( lgpos );
    csrAE.Cpage().SetLgposModify( lgpos );

    //  perform operation on all three pages
    //
    csrFDP.FinalizePreInitPage();
    csrOE.FinalizePreInitPage();
    csrAE.FinalizePreInitPage();
    SPIPerformCreateMultiple( pfucb, &csrFDP, &csrOE, &csrAE, pgnoPrimary, sph );

HandleError:
    csrAE.ReleasePage();
    csrOE.ReleasePage();
    csrFDP.ReleasePage();
    return err;
}


//  initializes a FDP page with external headers for space.
//  Also initializes external space trees appropriately.
//  This operation is logged as an aggregate.
//
INLINE ERR ErrSPICreateMultiple(
    FUCB        *pfucb,
    const PGNO  pgnoParent,
    const PGNO  pgnoFDP,
    const OBJID objidFDP,
    const CPG   cpgPrimary,
    const ULONG fPageFlags,
    const BOOL  fUnique )
{
    return ErrSPCreateMultiple(
            pfucb,
            pgnoParent,
            pgnoFDP,
            objidFDP,
            pgnoFDP + 1,
            pgnoFDP + 2,
            pgnoFDP + cpgPrimary - 1,
            cpgPrimary,
            fUnique,
            fPageFlags );
}

ERR ErrSPICreateSingle(
    FUCB            *pfucb,
    CSR             *pcsr,
    const PGNO      pgnoParent,
    const PGNO      pgnoFDP,
    const OBJID     objidFDP,
    CPG             cpgPrimary,
    const BOOL      fUnique,
    const ULONG     fPageFlags,
    const DBTIME    dbtime )
{
    ERR             err;
    SPACE_HEADER    sph;
    const BOOL      fRedo = ( PinstFromPfucb( pfucb )->m_plog->FRecoveringMode() == fRecoveringRedo );

    // We only expect a previously known dbtime during do-time.
    // This function might be invoked during redo with dbtimeNil during create DB. 
    Assert( ( !fRedo && ( dbtime == dbtimeNil ) ) ||
            ( fRedo && ( dbtime != dbtimeNil ) ) ||
            ( fRedo && ( dbtime == dbtimeNil ) && g_rgfmp[ pfucb->ifmp ].FCreatingDB() && !g_rgfmp[ pfucb->ifmp ].FLogOn() ) );

    //  copy space information into external header
    //
    sph.SetPgnoParent( pgnoParent );
    sph.SetCpgPrimary( cpgPrimary );

    Assert( sph.FSingleExtent() );      // always initialised to single-extent, unique
    Assert( sph.FUnique() );

    Assert( !( fPageFlags & CPAGE::fPageRoot ) );
    Assert( !( fPageFlags & CPAGE::fPageLeaf ) );
    Assert( !( fPageFlags & CPAGE::fPageParentOfLeaf ) );
    Assert( !( fPageFlags & CPAGE::fPageEmpty ) );
    Assert( !( fPageFlags & CPAGE::fPagePreInit ) );
    Assert( !( fPageFlags & CPAGE::fPageSpaceTree) );

    if ( !fUnique )
        sph.SetNonUnique();

    Assert( cpgPrimary > 0 );
    if ( cpgPrimary > cpgSmallSpaceAvailMost )
    {
        sph.SetRgbitAvail( 0xffffffff );
    }
    else
    {
        sph.SetRgbitAvail( 0 );
        while ( --cpgPrimary > 0 )
        {
            sph.SetRgbitAvail( ( sph.RgbitAvail() << 1 ) + 1 );
        }
    }

    Assert( objidFDP == ObjidFDP( pfucb ) );
    Assert( objidFDP != 0 );

    //  get pgnoFDP to initialize in current CSR pgno
    //
    const ULONG fInitPageFlags = fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf;
    if ( !fRedo || ( dbtime == dbtimeNil ) )
    {
        Call( pcsr->ErrGetNewPreInitPage(
            pfucb->ppib,
            pfucb->ifmp,
            pgnoFDP,
            objidFDP,
            !fRedo && g_rgfmp[ pfucb->ifmp ].FLogOn() ) );
        pcsr->ConsumePreInitPage( fInitPageFlags );

        if ( !fRedo )
        {
            LGPOS lgpos;
            Call( ErrLGCreateSingleExtentFDP( pfucb, pcsr, &sph, fPageFlags, &lgpos ) );
            pcsr->Cpage().SetLgposModify( lgpos );
        }
    }
    else
    {
        Call( pcsr->ErrGetNewPreInitPageForRedo(
                pfucb->ppib,
                pfucb->ifmp,
                pgnoFDP,
                objidFDP,
                dbtime ) );
        pcsr->ConsumePreInitPage( fInitPageFlags );
    }

    Assert( pcsr->FDirty() );
    SPIInitPgnoFDP( pfucb, pcsr, sph  );

    pcsr->FinalizePreInitPage();

    BTUp( pfucb );

HandleError:
    return err;
}


//  opens a cursor on tree to be created, and uses cursor to
//  initialize space data structures.
//
ERR ErrSPCreate(
    PIB             *ppib,
    const IFMP      ifmp,
    const PGNO      pgnoParent,
    const PGNO      pgnoFDP,
    const CPG       cpgPrimary,
    const ULONG     fSPFlags,
    const ULONG     fPageFlags,
    OBJID           *pobjidFDP,
    CPG             *pcpgOEFDP,
    CPG             *pcpgAEFDP )
{
    ERR             err;
    FUCB            *pfucb = pfucbNil;
    const BOOL      fUnique = !( fSPFlags & fSPNonUnique );

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortSpace );

    Assert( NULL != pobjidFDP );
    Assert( !( fPageFlags & CPAGE::fPageRoot ) );
    Assert( !( fPageFlags & CPAGE::fPageLeaf ) );
    Assert( !( fPageFlags & CPAGE::fPageParentOfLeaf ) );
    Assert( !( fPageFlags & CPAGE::fPageEmpty ) );
    Assert( !( fPageFlags & CPAGE::fPageSpaceTree ) );

    *pcpgOEFDP = 0;
    *pcpgAEFDP = 0;

    if ( pgnoFDP == pgnoSystemRoot &&
            pgnoParent == pgnoNull &&
            g_rgfmp[ ifmp ].FLogOn() &&
            //  This is a proxy for !( grbit & bitCreateDbImplicitly ), and once we
            //  definitely don't have to play such old logs, we could move this into
            //  the if body as Assert( !PinstFromIfmp( ifmp )->m_plogFRecovering() ).
            !PinstFromIfmp( ifmp )->m_plog->FRecovering() )
    {
        //  This means we're setting up the DB for the first time, and need to setup the dbroot
        //  space.
        //
        //  This LR corresponds to the ErrIONewSize() for the user specified cpgPrimary passed
        //  down to us from ErrDBCreateDatabase().  This allows us to extend the DB to the 
        //  correct user specified size at recovery\redo time.  We want this right before 
        //  we set the cpgPrimary into the DbRoot OE.  See ErrDBCreateDatabase() call in 
        //  logredo.cxx, for more info.
        //  Note: This also makes this lrtypExtendDB LR the first in the log stream after the 
        //  lrtypCreateDB.
#ifdef DEBUG
        QWORD cbSize = 0;
        if ( g_rgfmp[ ifmp ].Pfapi()->ErrSize( &cbSize, IFileAPI::filesizeLogical ) >= JET_errSuccess )
        {
            Assert( ( cbSize / g_cbPage - cpgDBReserved ) == cpgPrimary );
        }
#endif
        //  Technically we don't need to log this unless cpgPrimary > CpgDBDatabaseMinMin(), 
        //  but it is more resilient to always log, in case we lower MinMin in the future.
        //  Of course we should check the requested size is large enough to hold the basic
        //  DB structure (Space Info, Catalog, MSysLocales, etc).
        Assert( cpgPrimary >= CpgDBDatabaseMinMin() ||
                //  Temp is actually smaller, requires a lot less space.
                FFMPIsTempDB( ifmp ) );

        LGPOS lgposExtend;
        Call( ErrLGExtendDatabase( PinstFromIfmp( ifmp )->m_plog, ifmp, cpgPrimary, &lgposExtend ) );
        Call( ErrIOResizeUpdateDbHdrCount( ifmp, fTrue /* fExtend */ ) );
        if ( ( g_rgfmp[ ifmp ].ErrDBFormatFeatureEnabled( JET_efvLgposLastResize ) == JET_errSuccess ) )
        {
            // At this point, we don't expect the DB header to support JET_efvLgposLastResize
            // because the right version is only stamped later on in ErrDBCreateDBFinish().
            Expected( fFalse );
            Call( ErrIOResizeUpdateDbHdrLgposLast( ifmp, lgposExtend ) );
        }
    }

    //  UNDONE: an FCB will be allocated here, but it will be uninitialized,
    //  unless this is the special case where we are allocating the DB root,
    //  in which case it is initialized. Hence, the subsequent BTClose
    //  will free it.  It will have to be reallocated on a subsequent
    //  DIR/BTOpen, at which point it will get put into the FCB hash
    //  table.  Implement a fix to allow leaving the FCB in an uninitialized
    //  state, then have it initialized by the subsequent DIR/BTOpen.
    //
    CallR( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb, openNew ) );

    tcScope->nParentObjectClass = TceFromFUCB( pfucb );

    FCB *pfcb   = pfucb->u.pfcb;
    Assert( pfcbNil != pfcb );

    if ( pgnoSystemRoot == pgnoFDP )
    {
        Assert( pfcb->FInitialized() );
        Assert( pfcb->FTypeDatabase() );
    }
    else
    {
        Assert( !pfcb->FInitialized() );
        Assert( pfcb->FTypeNull() );
    }

    Assert( pfcb->FUnique() );      // FCB always initialised as unique
    if ( !fUnique )
    {
        pfcb->Lock();
        pfcb->SetNonUnique();
        pfcb->Unlock();
    }

    //  get objid for this new FDP. This objid is then stored in catalog table.

    if ( g_fRepair && pgnoSystemRoot == pgnoFDP )
    {
        *pobjidFDP = objidSystemRoot;
    }
    else
    {
        Call( g_rgfmp[ pfucb->ifmp ].ErrObjidLastIncrementAndGet( pobjidFDP ) );
    }
    Assert( pgnoSystemRoot != pgnoFDP || objidSystemRoot == *pobjidFDP );

    //  objidFDP should be initialised to NULL

    Assert( objidNil == pfcb->ObjidFDP() || g_fRepair );
    pfcb->SetObjidFDP( *pobjidFDP );
    tcScope->SetDwEngineObjid( *pobjidFDP );

    Assert( !pfcb->FSpaceInitialized() || g_fRepair );
    pfcb->Lock();
    pfcb->SetSpaceInitialized();
    pfcb->Unlock();

    if ( fSPFlags & fSPMultipleExtent )
    {
        Assert( PgnoFDP( pfucb ) == pgnoFDP );
        pfcb->SetPgnoOE( pgnoFDP + 1 );
        pfcb->SetPgnoAE( pgnoFDP + 2 );
        err = ErrSPICreateMultiple(
                    pfucb,
                    pgnoParent,
                    pgnoFDP,
                    *pobjidFDP,
                    cpgPrimary,
                    fPageFlags,
                    fUnique );

        Assert( pfcb->ObjidFDP() == *pobjidFDP );
        *pcpgOEFDP = cpgPrimary;
        *pcpgAEFDP = cpgPrimary - cpgMultipleExtentMin;
    }
    else
    {
        Assert( PgnoFDP( pfucb ) == pgnoFDP );
        pfcb->SetPgnoOE( pgnoNull );
        pfcb->SetPgnoAE( pgnoNull );
        err = ErrSPICreateSingle(
                    pfucb,
                    Pcsr( pfucb ),
                    pgnoParent,
                    pgnoFDP,
                    *pobjidFDP,
                    cpgPrimary,
                    fUnique,
                    fPageFlags,
                    dbtimeNil );

        Assert( pfcb->ObjidFDP() == *pobjidFDP );
        *pcpgOEFDP = cpgPrimary;
        *pcpgAEFDP = cpgPrimary - cpgSingleExtentMin;
    }

    Assert( !FFUCBSpace( pfucb ) );
    Assert( !FFUCBVersioned( pfucb ) );

HandleError:
    Assert( ( pfucb != pfucbNil ) || ( err < JET_errSuccess ) );

    if ( pfucb != pfucbNil )
    {
        BTClose( pfucb );
    }

    return err;
}


//  calculates extent info in rgext
//      number of extents in *pcextMac
VOID SPIConvertCalcExtents(
    const SPACE_HEADER& sph,
    const PGNO          pgnoFDP,
    EXTENTINFO          *rgext,
    INT                 *pcext )
{
    PGNO                pgno;
    UINT                rgbit       = 0x80000000;
    INT                 iextMac     = 0;

    //  if available extent for space after rgbitAvail, then
    //  set rgextT[0] to extent, otherwise set rgextT[0] to
    //  last available extent.
    //
    if ( sph.CpgPrimary() - 1 > cpgSmallSpaceAvailMost )
    {
        pgno = pgnoFDP + sph.CpgPrimary() - 1;
        rgext[iextMac].pgnoLastInExtent = pgno;
        rgext[iextMac].cpgExtent = sph.CpgPrimary() - cpgSmallSpaceAvailMost - 1;
        Assert( rgext[iextMac].FValid() );
        pgno -= rgext[iextMac].CpgExtent();
        iextMac++;

        Assert( pgnoFDP + cpgSmallSpaceAvailMost == pgno );
        Assert( 0x80000000 == rgbit );
    }
    else
    {
        pgno = pgnoFDP + cpgSmallSpaceAvailMost;
        while ( 0 != rgbit && 0 == iextMac )
        {
            Assert( pgno > pgnoFDP );
            if ( rgbit & sph.RgbitAvail() )
            {
                Assert( pgno <= pgnoFDP + sph.CpgPrimary() - 1 );
                rgext[iextMac].pgnoLastInExtent = pgno;
                rgext[iextMac].cpgExtent = 1;
                Assert( rgext[iextMac].FValid() );
                iextMac++;
            }

            //  even if we found an available extent, we still need
            //  to update the loop variables in preparation for the
            //  next loop below
            pgno--;
            rgbit >>= 1;
        }
    }

    //  continue through rgbitAvail finding all available extents.
    //  if iextMac == 0 then there was not even one available extent.
    //
    Assert( ( 0 == iextMac && 0 == rgbit )
        || 1 == iextMac );

    //  find additional available extents
    //
    for ( ; rgbit != 0; pgno--, rgbit >>= 1 )
    {
        Assert( pgno > pgnoFDP );
        if ( rgbit & sph.RgbitAvail() )
        {
            const INT   iextPrev    = iextMac - 1;
            Assert( rgext[iextPrev].CpgExtent() > 0 );
            Assert( (ULONG)rgext[iextPrev].CpgExtent() <= rgext[iextPrev].PgnoLast() );
            Assert( rgext[iextPrev].PgnoLast() <= pgnoFDP + sph.CpgPrimary() - 1 );

            const PGNO  pgnoFirst = rgext[iextPrev].PgnoLast() - rgext[iextPrev].CpgExtent() + 1;
            Assert( pgnoFirst > pgnoFDP );

            if ( pgnoFirst - 1 == pgno )
            {
                Assert( pgnoFirst - 1 > pgnoFDP );
                rgext[iextPrev].cpgExtent++;
                Assert( rgext[iextPrev].FValid() );
            }
            else
            {
                rgext[iextMac].pgnoLastInExtent = pgno;
                rgext[iextMac].cpgExtent = 1;
                Assert( rgext[iextMac].FValid() );
                iextMac++;
            }
        }
    }

    *pcext = iextMac;
}

//  update OwnExt root page for a convert
//
LOCAL VOID SPIConvertUpdateOE( FUCB *pfucb, CSR *pcsrOE, const SPACE_HEADER& sph, PGNO pgnoSecondaryFirst, CPG cpgSecondary )
{
    if ( !FBTIUpdatablePage( *pcsrOE ) )
    {
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() );
        return;
    }

    Assert( pcsrOE->Latch() == latchWrite );
    Assert( !FFUCBSpace( pfucb ) );

    // Writing is done to pcsr, not to pfucb currency.
    SPIInitSplitBuffer( pfucb, pcsrOE );

    Assert( 0 == pcsrOE->Cpage().Clines() );
    Assert( cpgSecondary != 0 );
    pcsrOE->SetILine( 0 );

    //  insert secondary and primary extents
    //
    //  We can use raw extent node, because we always want to insert
    //  a regular format ext nodes here, b/c its the OE.

    const CSPExtentNodeKDF      spextSecondary( SPEXTKEY::fSPExtentTypeOE, pgnoSecondaryFirst + cpgSecondary - 1, cpgSecondary );
    SPIWrappedNDInsert(
        pfucb,
        pcsrOE,
        spextSecondary.Pkdf(),
        cpgSecondary,
        0 );

    const CSPExtentNodeKDF      spextPrimary( SPEXTKEY::fSPExtentTypeOE, PgnoFDP( pfucb ) + sph.CpgPrimary() - 1, sph.CpgPrimary() );
    const BOOKMARK          bm = spextPrimary.GetBm( pfucb );
    const ERR               err = ErrNDSeek( pfucb, pcsrOE, bm );   // cannot fail, just did insert
    AssertRTL( wrnNDFoundLess == err || wrnNDFoundGreater == err );
    if ( wrnNDFoundLess == err )
    {
        Assert( pcsrOE->Cpage().Clines() - 1 == pcsrOE->ILine() );
        pcsrOE->IncrementILine();
    }
    SPIWrappedNDInsert(
        pfucb,
        pcsrOE,
        spextPrimary.Pkdf() );
}


LOCAL VOID SPIConvertUpdateAE(
    FUCB            *pfucb,
    CSR             *pcsrAE,
    EXTENTINFO      *rgext,
    INT             iextMac,
    PGNO            pgnoSecondaryFirst,
    CPG             cpgSecondary )
{
    if ( !FBTIUpdatablePage( *pcsrAE ) )
    {
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() );
        return;
    }

    //  we should never encounter an AE with > 2G extents

    Assert( iextMac >= 0 );
    if ( iextMac < 0 )
    {
        return;
    }

    //  create available extent tree and insert avail extents
    //
    Assert( pcsrAE->Latch() == latchWrite );
    Assert( !FFUCBSpace( pfucb ) );

    // Writing is done to pcsr, not to pfucb currency.
    SPIInitSplitBuffer( pfucb, pcsrAE );

    Assert( cpgSecondary >= 2 );
    Assert( 0 == pcsrAE->Cpage().Clines() );
    pcsrAE->SetILine( 0 );

    //
    // Adding a secondary extent, but using 1 page each for the new space trees
    //
    CPG     cpgExtent = cpgSecondary - 1 - 1;
    PGNO    pgnoLast = pgnoSecondaryFirst + cpgSecondary - 1;
    if ( cpgExtent != 0 )
    {
        //  We can use raw extent node, because we always want to insert
        //  a legacy format AE nodes here.
        const CSPExtentNodeKDF      spavailextSecondary( SPEXTKEY::fSPExtentTypeAE, pgnoLast, cpgExtent );
        SPIWrappedNDInsert(
            pfucb,
            pcsrAE,
            spavailextSecondary.Pkdf(),
            0,
            cpgExtent );
    }

    Assert( latchWrite == pcsrAE->Latch() );

    //  rgext contains list of available pages, with highest pages
    //  first, so traverse the list in reverse order to force append
    for ( INT iext = iextMac - 1; iext >= 0; iext-- )
    {

        //  extent may have been fully allocated for OE and AE trees
        //
//      if ( 0 == rgext[iext].CpgExtent() )
//          continue;

        //  NO!!  Above code is wrong!  OE and AE trees are allocated
        //  from a secondary extent when the FDP is converted from
        //  single to multiple. -- SOMEONE
        Assert( rgext[iext].CpgExtent() > 0 );

#ifdef DEBUG
        if ( iext > 0 )
        {
            Assert( rgext[iext].PgnoLast() < rgext[iext-1].PgnoLast() );
        }
#endif

        //  We can use raw extent node, because we always want to insert
        //  a legacy format AE nodes here.
        AssertTrack( rgext[iext].FValid(), "SPConvertInvalidExtInfo" );
        const CSPExtentNodeKDF      spavailextCurr( SPEXTKEY::fSPExtentTypeAE, rgext[iext].PgnoLast(), rgext[iext].CpgExtent() );

        //  seek to point of insert
        //
        if ( 0 < pcsrAE->Cpage().Clines() )
        {
            const BOOKMARK  bm = spavailextCurr.GetBm( pfucb );
            ERR         err;
            err = ErrNDSeek( pfucb, pcsrAE, bm );   // cannot fail, just did insert
            AssertRTL( wrnNDFoundLess == err || wrnNDFoundGreater == err );
            if ( wrnNDFoundLess == err )
                pcsrAE->IncrementILine();
        }

        SPIWrappedNDInsert(
            pfucb,
            pcsrAE,
            spavailextCurr.Pkdf() );
    }

    Assert( pcsrAE->Latch() == latchWrite );
}


//  update FDP page for convert
//
INLINE VOID SPIConvertUpdateFDP( FUCB *pfucb, CSR *pcsrRoot, SPACE_HEADER *psph )
{
    if ( FBTIUpdatablePage( *pcsrRoot ) )
    {
        DATA    data;

        Assert( latchWrite == pcsrRoot->Latch() );
        Assert( pcsrRoot->FDirty() );

        //  update external header to multiple extent space
        //
        data.SetPv( psph );
        data.SetCb( sizeof( *psph ) );
        NDSetExternalHeader( pfucb, pcsrRoot, noderfSpaceHeader, &data );
    }
    else
    {
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() );
    }
}


//  perform convert operation
//
VOID SPIPerformConvert( FUCB            *pfucb,
                        CSR             *pcsrRoot,
                        CSR             *pcsrAE,
                        CSR             *pcsrOE,
                        SPACE_HEADER    *psph,
                        PGNO            pgnoSecondaryFirst,
                        CPG             cpgSecondary,
                        EXTENTINFO      *rgext,
                        INT             iextMac )
{
    SPIConvertUpdateOE( pfucb, pcsrOE, *psph, pgnoSecondaryFirst, cpgSecondary );
    SPIConvertUpdateAE( pfucb, pcsrAE, rgext, iextMac, pgnoSecondaryFirst, cpgSecondary );
    SPIConvertUpdateFDP( pfucb, pcsrRoot, psph );
}


//  gets space header from root page
//  calcualtes extent info and places in rgext
//
VOID SPIConvertGetExtentinfo( FUCB          *pfucb,
                              CSR           *pcsrRoot,
                              SPACE_HEADER  *psph,
                              EXTENTINFO    *rgext,
                              INT           *piextMac )
{
    //  determine all available extents.  Maximum number of
    //  extents is (cpgSmallSpaceAvailMost+1)/2 + 1.
    //

    NDGetExternalHeader( pfucb, pcsrRoot, noderfSpaceHeader );
    Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );

    //  get single extent allocation information
    //
    UtilMemCpy( psph, pfucb->kdfCurr.data.Pv(), sizeof(SPACE_HEADER) );

    //  if available extent for space after rgbitAvail, then
    //  set rgextT[0] to extent, otherwise set rgextT[0] to
    //  last available extent.
    //
    SPIConvertCalcExtents( *psph, PgnoFDP( pfucb ), rgext, piextMac );
}


//  convert single extent root page external header to
//  multiple extent space tree.
//
LOCAL ERR ErrSPIConvertToMultipleExtent( FUCB *pfucb, CPG cpgReq, CPG cpgMin, BOOL fUseSpaceHints = fTrue )
{
    ERR             err;
    SPACE_HEADER    sph;
    PGNO            pgnoSecondaryFirst  = pgnoNull;
    INT             iextMac             = 0;
    UINT            rgbitAvail;
    FUCB            *pfucbT;
    CSR             csrOE;
    CSR             csrAE;
    CSR             *pcsrRoot           = pfucb->pcsrRoot;
    DBTIME          dbtimeBefore        = dbtimeNil;
    ULONG           fFlagsBefore        = 0;
    LGPOS           lgpos               = lgposMin;
    EXTENTINFO      rgextT[(cpgSmallSpaceAvailMost+1)/2 + 1];

    Assert( NULL != pcsrRoot );
    Assert( latchRIW == pcsrRoot->Latch() );
    AssertSPIPfucbOnRoot( pfucb );
    Assert( !FFUCBSpace( pfucb ) );
    Assert( ObjidFDP( pfucb ) != objidNil );
    Assert( pfucb->u.pfcb->FSpaceInitialized() );

    const ULONG fPageFlags = pcsrRoot->Cpage().FFlags()
                                & ~CPAGE::fPageRepair
                                & ~CPAGE::fPageRoot
                                & ~CPAGE::fPageLeaf
                                & ~CPAGE::fPageParentOfLeaf;

    //  get extent info from space header of root page
    //
    SPIConvertGetExtentinfo( pfucb, pcsrRoot, &sph, rgextT, &iextMac );
    Assert( sph.FSingleExtent() );

    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagSpaceInternal,
        OSFormat(
            "%hs: ConvertSMALLToMultiExt [0x%x:0x%x:%lu] %lu",
            __FUNCTION__,
            pfucb->ifmp,
            ObjidFDP( pfucb ),
            PgnoFDP( pfucb ),
            sph.CpgPrimary() ) );

    //  save off avail bitmap so we can log it below
    //
    rgbitAvail = sph.RgbitAvail();

    //  allocate secondary extent for space trees
    //
    CallR( ErrBTOpen( pfucb->ppib, pfucb->u.pfcb, &pfucbT, fFalse ) );

    //  allocate enough pages for conversion (OE/AE root pages), plus enough to satisfy
    //  the space request that caused us to burst to multiple extent in the first place
    cpgMin += cpgMultipleExtentConvert;
    cpgReq = max( cpgMin, cpgReq );

    if ( fUseSpaceHints )
    {
        //  factor in growth space hints.
        //
        CPG cpgNext = CpgSPIGetNextAlloc( pfucb->u.pfcb->Pfcbspacehints(), sph.CpgPrimary() );
        cpgReq = max( cpgNext, cpgReq );
    }

    //  database always uses multiple extent
    //
    Assert( sph.PgnoParent() != pgnoNull );
    Call( ErrSPGetExt(pfucbT, sph.PgnoParent(), &cpgReq, cpgMin, &pgnoSecondaryFirst ) );
    Assert( cpgReq >= cpgMin );
    Assert( pgnoSecondaryFirst != pgnoNull );

    //  set pgnoOE and pgnoAE in B-tree
    //
    Assert( pgnoSecondaryFirst != pgnoNull );
    sph.SetMultipleExtent();
    sph.SetPgnoOE( pgnoSecondaryFirst );

    //  represent all space in space trees.  Note that maximum
    //  space allowed to accumulate before conversion to
    //  large space format, should be representable in single
    //  page space trees.
    //
    //  create OwnExt and insert primary and secondary owned extents
    //
    Assert( !FFUCBVersioned( pfucbT ) );
    Assert( !FFUCBSpace( pfucbT ) );

    const BOOL fLogging = !pfucb->u.pfcb->FDontLogSpaceOps() && g_rgfmp[ pfucb->ifmp ].FLogOn();

    //  get pgnoOE and depend on pgnoRoot
    //
    Call( csrOE.ErrGetNewPreInitPage( pfucbT->ppib,
                                      pfucbT->ifmp,
                                      sph.PgnoOE(),
                                      ObjidFDP( pfucbT ),
                                      fLogging ) );
    csrOE.ConsumePreInitPage( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree );
    Assert( latchWrite == csrOE.Latch() );

    //  get pgnoAE and depend on pgnoRoot
    //
    Call( csrAE.ErrGetNewPreInitPage( pfucbT->ppib,
                                      pfucbT->ifmp,
                                      sph.PgnoAE(),
                                      ObjidFDP( pfucbT ),
                                      fLogging ) );
    csrAE.ConsumePreInitPage( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree );
    Assert( latchWrite == csrAE.Latch() );

    SPIUpgradeToWriteLatch( pfucb );

    dbtimeBefore = pfucb->pcsrRoot->Dbtime();
    fFlagsBefore = pfucb->pcsrRoot->Cpage().FFlags();
    Assert ( dbtimeNil != dbtimeBefore );

    //  set max dbtime for the three pages in all the pages
    //
    SPIDirtyAndSetMaxDbtime( pfucb->pcsrRoot, &csrOE, &csrAE );

    if ( pfucb->pcsrRoot->Dbtime() > dbtimeBefore )
    {
        //  log convert
        //  convert can not fail after this operation
        if ( !fLogging )
        {
            Assert( 0 == CmpLgpos( lgpos, lgposMin ) );
            err = JET_errSuccess;
        }
        else
        {
            err = ErrLGConvertFDP(
                        pfucb,
                        pcsrRoot,
                        &sph,
                        pgnoSecondaryFirst,
                        cpgReq,
                        dbtimeBefore,
                        rgbitAvail,
                        fPageFlags,
                        &lgpos );
        }
    }
    else
    {
        FireWall( "ConvertToMultiExtDbTimeTooLow" );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucb ), HaDbFailureTagCorruption, L"b4c2850e-04f0-4476-b967-2420fdcb6663" );
        err = ErrERRCheck( JET_errDbTimeCorrupted );
    }

    // see if we have to revert the time on the page
    if ( err < JET_errSuccess )
    {
        pfucb->pcsrRoot->RevertDbtime( dbtimeBefore, fFlagsBefore );
    }
    Call ( err );

    //  don't expose the multiple-extent trees until we've write-latched
    //  all three root pages and we're guaranteed to succeed
    //
    pfucb->u.pfcb->SetPgnoOE( sph.PgnoOE() );
    pfucb->u.pfcb->SetPgnoAE( sph.PgnoAE() );
    Assert( pgnoNull != PgnoAE( pfucb ) );
    Assert( pgnoNull != PgnoOE( pfucb ) );

    pcsrRoot->Cpage().SetLgposModify( lgpos );
    csrOE.Cpage().SetLgposModify( lgpos );
    csrAE.Cpage().SetLgposModify( lgpos );

    //  perform convert operation
    //
    SPIPerformConvert(
            pfucbT,
            pcsrRoot,
            &csrAE,
            &csrOE,
            &sph,
            pgnoSecondaryFirst,
            cpgReq,
            rgextT,
            iextMac );

    csrOE.FinalizePreInitPage();
    csrAE.FinalizePreInitPage();

    AssertSPIPfucbOnRoot( pfucb );

    Assert( pfucb->pcsrRoot->Latch() == latchWrite );

HandleError:
    Assert( err != JET_errKeyDuplicate );
    csrAE.ReleasePage();
    csrOE.ReleasePage();
    Assert( pfucbT != pfucbNil );
    BTClose( pfucbT );
    return err;
}


ERR ErrSPBurstSpaceTrees( FUCB *pfucb )
{
    ERR err = JET_errSuccess;

    Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
    pfucb->pcsrRoot = Pcsr( pfucb );

    Call( ErrSPIConvertToMultipleExtent(
            pfucb,
            0,          // cpgReq
            0,          // cpgMin
            fFalse ) ); // fUseSpaceHints

HandleError:
    BTUp( pfucb );
    pfucb->pcsrRoot = pcsrNil;

    return err;
}


INLINE BOOL FSPIAllocateAllAvail(
    const CSPExtentInfo *   pcspaei,
    const CPG               cpgReq,
    const LONG              lPageFragment,
    const PGNO              pgnoFDP,
    const CPG               cpgDb,
    const BOOL              fSPFlags )
{
    BOOL fAllocateAllAvail  = fTrue;

    // Use up all available pages if less than or equal to
    // requested amount.

    if ( pcspaei->CpgExtent() > cpgReq )
    {
        if ( BoolSetFlag( fSPFlags, fSPExactExtent ) )
        {
            fAllocateAllAvail = fFalse;
        }
        else if ( pgnoSystemRoot == pgnoFDP )
        {
            // If allocating extent from database, ensure we don't
            // leave behind an extent smaller than cpgSmallGrow,
            // because that's the smallest size a given table
            // will grow.
            // Exception: When creating tables in the first 128 pages
            // we want to keep tight allocation, because this might
            // be a small client.
            if ( BoolSetFlag( fSPFlags, fSPNewFDP ) && cpgDb < cpgSmallDbSpaceSize && pcspaei->PgnoFirst() < pgnoSmallDbSpaceStart )
            {
                fAllocateAllAvail = fFalse;
            }
            if ( pcspaei->CpgExtent() >= cpgReq + cpgSmallGrow )
            {
                fAllocateAllAvail = fFalse;
            }
        }
        else if ( cpgReq < lPageFragment || pcspaei->CpgExtent() >= cpgReq + lPageFragment )
        {
            // Don't use all if only requested a small amount
            // (ie. cpgReq < lPageFragment) or there is way more available
            // than we requested (ie. cpgAvailExt >= cpgReq+lPageFragment)
            fAllocateAllAvail = fFalse;
        }
    }

    return fAllocateAllAvail;
}


// Returns the extent information of the extent for which its pgnoLast is greater than or
// equal to the pgno parameter specified.
// IMPORTANT: note that the extent returned may or may not contain pgno. To determine that,
// callers need to call FContains() on the CSPExtentInfo object returned.
// It works on either the AE or OE (specified by eExtType).

LOCAL INLINE ERR ErrSPIFindExt(
    __inout     FUCB * pfucb,
    _In_ const  PGNO pgno,
    _In_ const  SpacePool sppAvailPool,
    _In_ const  SPEXTKEY::E_SP_EXTENT_TYPE eExtType,
    _Out_       CSPExtentInfo * pspei )
{
    Assert( ( eExtType == SPEXTKEY::fSPExtentTypeOE ) || ( eExtType == SPEXTKEY::fSPExtentTypeAE ) );

    ERR err;
    DIB dib;

    OnDebug( const BOOL fOwnExt = eExtType == SPEXTKEY::fSPExtentTypeOE );
    Assert( fOwnExt ? pfucb->fOwnExt : pfucb->fAvailExt );
    Assert( ( sppAvailPool == spp::AvailExtLegacyGeneralPool ) || !fOwnExt );

    //  find bounds of extent
    //
    CSPExtentKeyBM spextkeyFind( eExtType, pgno, sppAvailPool );
    dib.pos = posDown;
    dib.pbm = spextkeyFind.Pbm( pfucb ) ;
    dib.dirflag = fDIRFavourNext;
    Call( ErrBTDown( pfucb, &dib, latchReadTouch ) );
    if ( wrnNDFoundLess == err )
    {
        //  landed on node previous to one we want
        //  move next
        //
        Assert( Pcsr( pfucb )->Cpage().Clines() - 1 ==
                    Pcsr( pfucb )->ILine() );
        Call( ErrBTNext( pfucb, fDIRNull ) );
    }

    pspei->Set( pfucb );

    ASSERT_VALID( pspei );

    Call( pspei->ErrCheckCorrupted() );

    //  enforce the pool must match
    //
    if ( pspei->SppPool() != sppAvailPool )
    {
        Error( ErrERRCheck( JET_errRecordNotFound ) );
    }

    err = JET_errSuccess;

HandleError:
    if ( err < JET_errSuccess )
    {
        pspei->Unset();
    }

    return err;
}


LOCAL ERR ErrSPIFindExtOE(
    __inout     FUCB * pfucbOE,
    _In_ const  PGNO pgnoFirst,
    _Out_       CSPExtentInfo * pcspoext
    )
{
    return ErrSPIFindExt( pfucbOE, pgnoFirst, spp::AvailExtLegacyGeneralPool, SPEXTKEY::fSPExtentTypeOE, pcspoext );
}


LOCAL ERR ErrSPIFindExtAE(
    __inout     FUCB * pfucbAE,
    _In_ const  PGNO pgnoFirst,
    _In_ const  SpacePool sppAvailPool,
    _Out_       CSPExtentInfo * pcspaext
    )
{
    Expected( ( sppAvailPool != spp::ShelvedPool ) || ( pfucbAE->u.pfcb->PgnoFDP() == pgnoSystemRoot ) );
    return ErrSPIFindExt( pfucbAE, pgnoFirst, sppAvailPool, SPEXTKEY::fSPExtentTypeAE, pcspaext );
}


LOCAL INLINE ERR ErrSPIFindExtOE(
    __inout     PIB *           ppib,
    _In_        FCB *           pfcb,
    _In_ const  PGNO            pgnoFirst,
    _Out_       CSPExtentInfo * pcspoext )
{
    ERR         err = JET_errSuccess;
    FUCB        *pfucbOE;

    CallR( ErrSPIOpenOwnExt( ppib, pfcb, &pfucbOE ) );
    Assert( pfcb == pfucbOE->u.pfcb );

    Call( ErrSPIFindExtOE( pfucbOE, pgnoFirst, pcspoext ) );
    ASSERT_VALID( pcspoext );

HandleError:
    BTClose( pfucbOE );

    return err;
}


LOCAL INLINE ERR ErrSPIFindExtAE(
    __inout     PIB *           ppib,
    _In_        FCB *           pfcb,
    _In_ const  PGNO            pgnoFirst,
    _In_ const  SpacePool       sppAvailPool,
    _Out_       CSPExtentInfo * pcspaext )
{
    ERR         err = JET_errSuccess;
    FUCB        *pfucbAE;

    CallR( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbAE ) );
    Assert( pfcb == pfucbAE->u.pfcb );

    Call( ErrSPIFindExtAE( pfucbAE, pgnoFirst, sppAvailPool, pcspaext ) );
    ASSERT_VALID( pcspaext );

HandleError:
    BTClose( pfucbAE );

    return err;
}

template< class T >
inline SpaceCategoryFlags& operator|=( SpaceCategoryFlags& spcatf, const T& t )
{
    spcatf = SpaceCategoryFlags( spcatf | (SpaceCategoryFlags)t );
    return spcatf;
}

BOOL FSPSpaceCatStrictlyLeaf( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfStrictlyLeaf ) != 0;
}

BOOL FSPSpaceCatStrictlyInternal( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfStrictlyInternal ) != 0;
}

BOOL FSPSpaceCatRoot( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfRoot ) != 0;
}

BOOL FSPSpaceCatSplitBuffer( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfSplitBuffer ) != 0;
}

BOOL FSPSpaceCatSmallSpace( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfSmallSpace ) != 0;
}

BOOL FSPSpaceCatSpaceOE( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfSpaceOE ) != 0;
}

BOOL FSPSpaceCatSpaceAE( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfSpaceAE ) != 0;
}

BOOL FSPSpaceCatAnySpaceTree( const SpaceCategoryFlags spcatf )
{
    return FSPSpaceCatSpaceOE( spcatf ) || FSPSpaceCatSpaceAE( spcatf );
}

BOOL FSPSpaceCatAvailable( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfAvailable ) != 0;
}

BOOL FSPSpaceCatIndeterminate( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfIndeterminate ) != 0;
}

BOOL FSPSpaceCatInconsistent( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfInconsistent ) != 0;
}

BOOL FSPSpaceCatLeaked( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfLeaked ) != 0;
}

BOOL FSPSpaceCatNotOwned( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfNotOwned ) != 0;
}

BOOL FSPSpaceCatNotOwnedEof( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfNotOwnedEof ) != 0;
}

BOOL FSPSpaceCatUnknown( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfUnknown ) != 0;
}

// Validates whether a given combination of flags is valid.
BOOL FSPValidSpaceCategoryFlags( const SpaceCategoryFlags spcatf )
{
    // Must have at least one flag set.
    if ( spcatf == spcatfNone )
    {
        return fFalse;
    }

    // If it's unknown, no other flags are expected to be set.
    if ( FSPSpaceCatUnknown( spcatf ) && ( spcatf != spcatfUnknown ) )
    {
        return fFalse;
    }

    // If it's indeterminate, no other flags are expected to be set.
    if ( FSPSpaceCatIndeterminate( spcatf ) && ( spcatf != spcatfIndeterminate ) )
    {
        return fFalse;
    }

    // If it's inconsistent, no other flags are expected to be set.
    if ( FSPSpaceCatInconsistent( spcatf ) && ( spcatf != spcatfInconsistent ) )
    {
        return fFalse;
    }

    // If it's leaked, no other flags are expected to be set.
    if ( FSPSpaceCatLeaked( spcatf ) && ( spcatf != spcatfLeaked ) )
    {
        return fFalse;
    }

    // If it's not owned, no other flags are expected to be set.
    if ( FSPSpaceCatNotOwned( spcatf ) && ( spcatf != spcatfNotOwned ) )
    {
        return fFalse;
    }

    // If it's not owned in the end of file, no other flags are expected to be set.
    if ( FSPSpaceCatNotOwnedEof( spcatf ) && ( spcatf != spcatfNotOwnedEof ) )
    {
        return fFalse;
    }

    // If it's available, it can't be categorized as a valid tree page.
    if ( FSPSpaceCatAvailable( spcatf ) &&
            ( FSPSpaceCatStrictlyLeaf( spcatf ) || FSPSpaceCatStrictlyInternal( spcatf ) || FSPSpaceCatRoot( spcatf ) ) )
    {
        return fFalse;
    }

    // It can't be part of OA and AE at the same time.
    if ( FSPSpaceCatSpaceOE( spcatf ) && FSPSpaceCatSpaceAE( spcatf ) )
    {
        return fFalse;
    }

    // If it's part of a split buffer, it must be a space page and it must be available.
    if ( FSPSpaceCatSplitBuffer( spcatf ) &&
        ( !FSPSpaceCatAnySpaceTree( spcatf ) || !FSPSpaceCatAvailable( spcatf ) ) )
    {
        return fFalse;
    }

    // If the page is available but not part of a split buffer, it can't belong to a space tree.
    if ( FSPSpaceCatAvailable( spcatf ) && !FSPSpaceCatSplitBuffer( spcatf ) && FSPSpaceCatAnySpaceTree( spcatf ) )
    {
        return fFalse;
    }

    // An available page must be combined only with some other specific flags.
    if ( FSPSpaceCatAvailable( spcatf ) &&
            !FSPSpaceCatSplitBuffer( spcatf ) &&
            !FSPSpaceCatAnySpaceTree( spcatf ) &&
            !FSPSpaceCatSmallSpace( spcatf ) &&
            ( spcatf != spcatfAvailable ) )
    {
        return fFalse;
    }

    // If it's a root, it can't be any of the other two types.
    if ( FSPSpaceCatRoot( spcatf ) && ( FSPSpaceCatStrictlyLeaf( spcatf ) || FSPSpaceCatStrictlyInternal( spcatf ) ) )
    {
        return fFalse;
    }

    // If it's a leaf, it can't be any of the other two types.
    if ( FSPSpaceCatStrictlyLeaf( spcatf ) && ( FSPSpaceCatRoot( spcatf ) || FSPSpaceCatStrictlyInternal( spcatf ) ) )
    {
        return fFalse;
    }

    // If it's an internal page, it can't be any of the other two types.
    if ( FSPSpaceCatStrictlyInternal( spcatf ) && ( FSPSpaceCatRoot( spcatf ) || FSPSpaceCatStrictlyLeaf( spcatf ) ) )
    {
        return fFalse;
    }

    // If the page is part of a small-space layout, it can't possibly be a space page by definition.
    if ( FSPSpaceCatSmallSpace( spcatf ) && FSPSpaceCatAnySpaceTree( spcatf ) )
    {
        return fFalse;
    }

    return fTrue;
}

// Frees a SpaceCatCtx handle.
VOID SPFreeSpaceCatCtx( _Inout_ SpaceCatCtx** const ppSpCatCtx )
{
    Assert( ppSpCatCtx != NULL );
    SpaceCatCtx* const pSpCatCtx = *ppSpCatCtx;
    if ( pSpCatCtx == NULL )
    {
        return;
    }

    if ( pSpCatCtx->pfucbSpace != pfucbNil )
    {
        BTUp( pSpCatCtx->pfucbSpace );
        BTClose( pSpCatCtx->pfucbSpace );
        pSpCatCtx->pfucbSpace = pfucbNil;
    }

    Assert( ( pSpCatCtx->pfucb != pfucbNil ) || ( pSpCatCtx->pfucbParent == pfucbNil ) );
    CATFreeCursorsFromObjid( pSpCatCtx->pfucb, pSpCatCtx->pfucbParent );

    pSpCatCtx->pfucbParent = pfucbNil;
    pSpCatCtx->pfucb = pfucbNil;

    if ( pSpCatCtx->pbm != NULL )
    {
        delete pSpCatCtx->pbm;
        pSpCatCtx->pbm = NULL;
    }

    delete pSpCatCtx;

    *ppSpCatCtx = NULL;
}

// Gets the object ID from a page.
ERR ErrSPIGetObjidFromPage(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _Out_ OBJID* const pobjid )
{
    CPAGE cpage;
    ERR err = cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfUninitPageOk );

    if ( err < JET_errSuccess )
    {
        *pobjid = objidNil;
    }
    else
    {
        *pobjid = cpage.ObjidFDP();
    }
    Call( err );
    cpage.ReleaseReadLatch();

HandleError:
    return err;
}

// Gets the category of a page supposedly owned by object, though not searching its children objects.
// That means it still returns spcatfUnknown if the page is owned by a child object.
ERR ErrSPIGetSpaceCategoryObject(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _In_ const OBJID objid,
    _In_ const OBJID objidParent,
    _In_ const SYSOBJ sysobj,
    _Out_ SpaceCategoryFlags* const pspcatf,
    _Out_ BOOL* const pfOwned,
    _Out_ SpaceCatCtx** const ppSpCatCtx )
{
    ERR err = JET_errSuccess;
    BOOL fOwned = fFalse;
    PGNO pgnoFDPParent = pgnoNull;
    PGNO pgnoFDP = pgnoNull;
    FUCB* pfucbParent = pfucbNil;
    FUCB* pfucb = pfucbNil;
    FUCB* pfucbSpace = pfucbNil;
    FCB* pfcb = pfcbNil;
    SpaceCategoryFlags spcatf = spcatfNone;
    CPAGE cpage;
    BOOL fPageLatched = fFalse;
    KEYDATAFLAGS kdf;
    SpaceCatCtx* pSpCatCtx = NULL;
    BOOKMARK_COPY* pbm = NULL;

    Assert( objid != objidNil );
    Assert( objid != objidParent );
    Assert( ( sysobj == sysobjNil ) || ( sysobj == sysobjTable ) || ( sysobj == sysobjIndex ) || ( sysobj == sysobjLongValue ) );
    Assert( ( objidParent != objidNil ) != ( objid == objidSystemRoot ) );
    Assert( ( sysobj != sysobjNil ) != ( objid == objidSystemRoot ) );
    Assert( ( sysobj == sysobjTable ) == ( objidParent == objidSystemRoot ) );
    Assert( pspcatf != NULL );
    Assert( pfOwned != NULL );
    Assert( ppSpCatCtx != NULL );

    // Initialize out vars.
    *pspcatf = spcatfUnknown;
    *pfOwned = fFalse;
    *ppSpCatCtx = NULL;

    Alloc( pSpCatCtx = new SpaceCatCtx );
    Alloc( pbm = new BOOKMARK_COPY );

    // First, determine the pgnoFDP and initialize cursors.
    //


    Call( ErrCATGetCursorsFromObjid(
            ppib,
            ifmp,
            objid,
            objidParent,
            sysobj,
            &pgnoFDP,
            &pgnoFDPParent,
            &pfucb,
            &pfucbParent ) );

    pfcb = pfucb->u.pfcb;

#ifdef DEBUG
    if ( pfucbParent != pfucbNil )
    {
        Assert( objid != objidSystemRoot );
        FCB* const pfcbParent = pfucbParent->u.pfcb;
        Assert( pfcbParent != pfcbNil );
        Assert( pfcbParent->FInitialized() );
        Assert( PgnoFDP( pfucbParent ) == pgnoFDPParent );
    }
    else
    {
        Assert( objid == objidSystemRoot );
    }
#endif

    // Latch root.
    Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
    pfucb->pcsrRoot = Pcsr( pfucb );

    // Check if the page is owned by the indicated object and whether or not it is available.
    //

    // Handle either single or multi extent space format.
    if ( FSPIIsSmall( pfcb ) )
    {
        Expected( objid != objidSystemRoot );

        const SPACE_HEADER* const psph = PsphSPIRootPage( pfucb );
        Assert( psph->FSingleExtent() );

        // Check if the tree contains the page.
        const PGNO pgnoFirst = PgnoFDP( pfucb );
        const PGNO pgnoLast = pgnoFirst + psph->CpgPrimary() - 1;
        if ( ( pgno < pgnoFirst ) || ( pgno > pgnoLast ) )
        {
            spcatf = spcatfUnknown;
            goto HandleError;
        }

        fOwned = fTrue;
        spcatf |= spcatfSmallSpace;

        // Check if the page is available according to the bitmap.
        C_ASSERT( cpgSmallSpaceAvailMost <= ( 8 * sizeof( psph->RgbitAvail() ) ) );
        if (
             ( pgno > pgnoFirst ) &&
             (
               // This covers a corner case in which the extent is larger than the bitmap can represent
               // (i.e., psph->CpgPrimary() > cpgSmallSpaceAvailMost + 1). Any pages not represented
               // by the bitmap are always considered available (any allocation would up-convert to large
               // extent format).
               ( ( pgno - pgnoFirst ) > cpgSmallSpaceAvailMost ) ||

               // This is the normal case in which the page falls within the bitmap's reach.
               ( psph->RgbitAvail() & ( (UINT)0x1 << ( pgno - pgnoFirst - 1 ) ) )
             )
           )
        {
            spcatf |= spcatfAvailable;
            goto HandleError;
        }

        // Is it the root of this tree?
        if ( pgno == pgnoFDP )
        {
            spcatf |= spcatfRoot;
        }
    }
    else
    {
        // Check if OE contains the page.
        CSPExtentInfo speiOE;
        err = ErrSPIFindExtOE( ppib, pfcb, pgno, &speiOE );
        if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
        {
            err = JET_errSuccess;
        }
        Call( err );

        if ( !speiOE.FIsSet() || !speiOE.FContains( pgno ) )
        {
            // We set special flags for pages which are expected to belong to the root.
            if ( objid == objidSystemRoot )
            {
                if ( speiOE.FIsSet() && !speiOE.FContains( pgno ) )
                {
                    spcatf = spcatfNotOwned;
                }
                else
                {
                    Assert( !speiOE.FIsSet() );

                    // If the page is physically beyond EOF, fail the call.
                    QWORD cbSize = 0;
                    Call( g_rgfmp[ ifmp ].Pfapi()->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
                    const PGNO pgnoBeyondEOF = PgnoOfOffset( cbSize );
                    if ( pgno >= pgnoBeyondEOF )
                    {
                        Error( ErrERRCheck( JET_errFileIOBeyondEOF ) );
                    }
                    else
                    {
                        spcatf = spcatfNotOwnedEof;
                    }
                }

                goto HandleError;
            }
            else
            {
                Assert( spcatf == spcatfNone );
            }
        }
        else
        {
            fOwned = fTrue;
        }

        // Check if AE contains the page.
        for ( SpacePool sppAvailPool = spp::MinPool;
                sppAvailPool < spp::MaxPool;
                sppAvailPool++ )
        {
            if ( sppAvailPool == spp::ShelvedPool )
            {
                // Shelved pages aren't really available since they can't be handed out for utilization.
                continue;
            }

            CSPExtentInfo speiAE;
            err = ErrSPIFindExtAE( ppib, pfcb, pgno, sppAvailPool, &speiAE );
            if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
            {
                err = JET_errSuccess;
            }
            Call( err );

            if ( speiAE.FIsSet() && speiAE.FContains( pgno ) )
            {
                if ( fOwned )
                {
                    spcatf |= spcatfAvailable;
                }
                else
                {
                    AssertTrack( fFalse, "SpaceCatAvailNotOwned" );
                    spcatf = spcatfInconsistent;
                }

                goto HandleError;
            }
        }

        // Is it the root of this tree or any of its space trees?
        if ( ( pgno == pgnoFDP ) || ( pgno == pfcb->PgnoOE() ) || ( pgno == pfcb->PgnoAE() ) )
        {
            spcatf |= spcatfRoot;
            spcatf |= ( pgno == pfcb->PgnoOE() ) ? spcatfSpaceOE :
                        ( ( pgno == pfcb->PgnoAE() ) ? spcatfSpaceAE : spcatfNone );
        }
    }

    BTUp( pfucb );
    pfucb->pcsrRoot = pcsrNil;

    Assert( ( fOwned && !FSPSpaceCatAvailable( spcatf ) ) ||
            ( !fOwned && !FSPIIsSmall( pfcb ) && ( objid != objidSystemRoot ) && ( spcatf == spcatfNone ) ) );

    // Search split buffers.
    //

    Assert( !FSPSpaceCatSplitBuffer( spcatf ) );
    // If we've already detected that it is the root, no need to search more.
    // Also, small space layout does not hold pages in split buffers by definition.
    if ( !FSPSpaceCatRoot( spcatf ) && !FSPSpaceCatSmallSpace( spcatf ) )
    {
        // Check AE first.
        BOOL fSPBufAEContains = fFalse;
        BOOL fSPBufOEContains = fFalse;
        Call( ErrSPISPBufContains( ppib, pfcb, pgno, fFalse, &fSPBufAEContains ) );
        if ( !fSPBufAEContains )
        {
            // Check OE now.
            Call( ErrSPISPBufContains( ppib, pfcb, pgno, fTrue, &fSPBufOEContains ) );
        }
        if ( fSPBufAEContains || fSPBufOEContains )
        {
            Assert( !( fSPBufAEContains && fSPBufOEContains ) );
            spcatf |= ( spcatfSplitBuffer | spcatfAvailable | ( fSPBufAEContains ? spcatfSpaceAE : spcatfSpaceOE ) );
            goto HandleError;
        }
    }

    // Read the actual page and try to navigate to it.
    //

    Assert( !FSPSpaceCatStrictlyInternal( spcatf ) && !FSPSpaceCatStrictlyLeaf( spcatf ) );
    err = cpage.ErrGetReadPage( ppib, ifmp, pgno, BFLatchFlags( bflfNoTouch | bflfUninitPageOk ) );
    if ( err < JET_errSuccess )
    {
        if ( err == JET_errPageNotInitialized )
        {
            err = JET_errSuccess;
            if ( FSPSpaceCatRoot( spcatf ) )
            {
                AssertTrack( false, "SpaceCatBlankRoot" );
                spcatf = spcatfInconsistent;
            }
            else
            {
                spcatf = spcatfUnknown;
            }
            goto HandleError;
        }
        else
        {
            Error( err );
        }
    }

    // We've successfully latched the page.
    fPageLatched = fTrue;
    Assert( cpage.FPageIsInitialized() );

    // Proceed with navigation only if the OBJID on the page matches.
    if ( cpage.ObjidFDP() != objid )
    {
        spcatf = spcatfUnknown;
        goto HandleError;
    }

    // Root flag consistency.
    if ( FSPSpaceCatRoot( spcatf ) && !cpage.FRootPage() )
    {
        AssertTrack( false, "SpaceCatIncRootFlag" );
        spcatf = spcatfInconsistent;
        goto HandleError;
    }

    // Space flag consistency.
    if ( FSPSpaceCatAnySpaceTree( spcatf ) && !cpage.FSpaceTree() )
    {
        AssertTrack( false, "SpaceCatIncSpaceFlag" );
        spcatf = spcatfInconsistent;
        goto HandleError;
    }

    // Pre-init flag consistency.
    if ( cpage.FPreInitPage() && ( cpage.FRootPage() || cpage.FLeafPage() ) )
    {
        AssertTrack( false, "SpaceCatIncPreInitFlag" );
        spcatf = spcatfInconsistent;
        goto HandleError;
    }

    // At this point, we're guaranteed to have a consistent root. Return.
    if ( FSPSpaceCatRoot( spcatf ) )
    {
        goto HandleError;
    }

    // We can't navigate to an empty or pre-init page.
    if ( cpage.FEmptyPage() || cpage.FPreInitPage() || ( cpage.Clines() <= 0 ) )
    {
        spcatf = spcatfUnknown;
        goto HandleError;
    }

    // Get a reference line from the page.
#ifdef DEBUG
    const INT iline = ( ( cpage.Clines() - 1 ) * ( rand() % 5 ) ) / 4;
#else
    const INT iline = 0;
#endif  // DEBUG
    Assert( ( iline >= 0 ) && ( iline < cpage.Clines() ) );
    NDIGetKeydataflags( cpage, iline, &kdf );

    // Make a local copy of the key before releasing the latch.
    // For internal pages or unique indexes (internal or leaf), we
    // only need the copy of the key because the data portion of internal pages is the
    // page number of the child, which does not affect which direction to take when
    // navigating. For unique indexes, the key is sufficient for navigating. In fact,
    // the data portion of the line may not be reliable if the node has been scrubbed.
    // Note that we do not scrub the data portion of nodes on the leaf level of non-unique
    // indexes, since that would render the tree logically corrupted.
    const BOOL fLeafPage = cpage.FLeafPage();
    const BOOL fSpacePage = cpage.FSpaceTree();
    const BOOL fNonUniqueKeys = cpage.FNonUniqueKeys() && !fSpacePage;  // Space tree pages of non-unique indexes are flagged as non-unique,
                                                                        // even though the keys themselves (pgnoLast) are obviously unique, so
                                                                        // consider that page as containing unique keys for navigation purposes.

    if ( fLeafPage && fNonUniqueKeys )
    {
        if ( kdf.key.FNull() || kdf.data.FNull() )
        {
            AssertTrack( false, "SpaceCatIncKdfNonUnique" );
            spcatf = spcatfInconsistent;
            goto HandleError;
        }

        Call( pbm->ErrCopyKeyData( kdf.key, kdf.data ) );
    }
    else
    {
        if ( fLeafPage && kdf.key.FNull() )
        {
            AssertTrack( false, "SpaceCatIncKdfUniqueLeaf" );
            spcatf = spcatfInconsistent;
            goto HandleError;
        }
        else if ( !fLeafPage && kdf.data.FNull() )
        {
            AssertTrack( false, "SpaceCatIncKdfUniqueInternal" );
            spcatf = spcatfInconsistent;
            goto HandleError;
        }

        Call( pbm->ErrCopyKey( kdf.key ) );
    }

    // Release latch before navigating.
    cpage.ReleaseReadLatch();
    fPageLatched = fFalse;

    // At this point, the page is either an internal page or leaf page of the
    // actual tree or its space trees (but not a root for sure). Try to navigate
    // to it from one of these three trees.
    if ( !fSpacePage )
    {
        if ( !fNonUniqueKeys != !!FFUCBUnique( pfucb ) )
        {
            AssertTrack( false, "SpaceCatIncUniqueFdp" );
            spcatf = spcatfInconsistent;
            goto HandleError;
        }

        // Try the tree itself.
        err = ErrBTContainsPage( pfucb, *pbm, pgno, fLeafPage );
        if ( err >= JET_errSuccess )
        {
            spcatf |= ( fLeafPage ? spcatfStrictlyLeaf : spcatfStrictlyInternal );
            goto HandleError;
        }
        else if ( err == JET_errRecordNotFound )
        {
            err = JET_errSuccess;
        }
        Call( err );
    }
    else
    {
        // Try the AE.
        Assert( pfucbSpace == pfucbNil );
        Call( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbSpace ) );
        if ( !fNonUniqueKeys != !!FFUCBUnique( pfucbSpace ) )
        {
            AssertTrack( false, "SpaceCatIncUniqueAe" );
            spcatf = spcatfInconsistent;
            goto HandleError;
        }
        err = ErrBTContainsPage( pfucbSpace, *pbm, pgno, fLeafPage );
        if ( err >= JET_errSuccess )
        {
            spcatf |= ( spcatfSpaceAE | ( fLeafPage ? spcatfStrictlyLeaf : spcatfStrictlyInternal ) );
            goto HandleError;
        }
        else if ( err == JET_errRecordNotFound )
        {
            err = JET_errSuccess;
        }
        Call( err );
        BTUp( pfucbSpace );
        BTClose( pfucbSpace );
        pfucbSpace = pfucbNil;

        // Try the OE.
        Assert( pfucbSpace == pfucbNil );
        Call( ErrSPIOpenOwnExt( ppib, pfcb, &pfucbSpace ) );
        if ( !fNonUniqueKeys != !!FFUCBUnique( pfucbSpace ) )
        {
            AssertTrack( false, "SpaceCatIncUniqueOe" );
            spcatf = spcatfInconsistent;
            goto HandleError;
        }
        err = ErrBTContainsPage( pfucbSpace, *pbm, pgno, fLeafPage );
        if ( err >= JET_errSuccess )
        {
            spcatf |= ( spcatfSpaceOE | ( fLeafPage ? spcatfStrictlyLeaf : spcatfStrictlyInternal ) );
            goto HandleError;
        }
        else if ( err == JET_errRecordNotFound )
        {
            err = JET_errSuccess;
        }
        Call( err );
        BTUp( pfucbSpace );
        BTClose( pfucbSpace );
        pfucbSpace = pfucbNil;
    }

    // Navigation failed.
    //

    spcatf = spcatfUnknown;

HandleError:
    if ( fPageLatched )
    {
        cpage.ReleaseReadLatch();
        fPageLatched = fFalse;
    }

    if ( pfucbSpace != pfucbNil )
    {
        BTUp( pfucbSpace );
    }

    if ( pfucb != pfucbNil )
    {
        BTUp( pfucb );
        pfucb->pcsrRoot = pcsrNil;
    }

    if ( pfucbParent != pfucbNil )
    {
        BTUp( pfucbParent );
    }

    // Open cursors to space trees if applicable.
    if ( ( err >= JET_errSuccess ) &&
            FSPSpaceCatAnySpaceTree( spcatf ) &&
            ( pfucbSpace == pfucbNil ) )
    {
        if ( FSPSpaceCatSpaceOE( spcatf ) )
        {
            err = ErrSPIOpenOwnExt( ppib, pfcb, &pfucbSpace );
        }
        else
        {
            Assert( FSPSpaceCatSpaceAE( spcatf ) );
            err = ErrSPIOpenAvailExt( ppib, pfcb, &pfucbSpace );
        }
    }

    // Close cursors to space trees if applicable.
    if ( ( err >= JET_errSuccess ) &&
            !FSPSpaceCatAnySpaceTree( spcatf ) &&
            ( pfucbSpace != pfucbNil ))
    {
        BTClose( pfucbSpace );
        pfucbSpace = NULL;
    }

    // Only return bookmark if this is an internal or leaf page.
    if ( ( err >= JET_errSuccess ) &&
            !FSPSpaceCatStrictlyInternal( spcatf ) &&
            !FSPSpaceCatStrictlyLeaf( spcatf ) &&
            ( pbm != NULL ) )
    {
        delete pbm;
        pbm = NULL;
    }

    // Fill out the context struct, either to clean it up or return it.
    if ( pSpCatCtx != NULL )
    {
        pSpCatCtx->pfucbParent = pfucbParent;
        pSpCatCtx->pfucb = pfucb;
        pSpCatCtx->pfucbSpace = pfucbSpace;
        pSpCatCtx->pbm = pbm;
    }
    else
    {
        Assert( pfucbParent == pfucbNil );
        Assert( pfucb == pfucbNil );
        Assert( pfucbSpace == pfucbNil );
        Assert( pbm == NULL );
    }

    if ( err >= JET_errSuccess )
    {
        *pspcatf = spcatf;
        *pfOwned = fOwned;
    }
    else
    {
        *pspcatf = spcatfUnknown;
        *pfOwned = fFalse;
        SPFreeSpaceCatCtx( &pSpCatCtx );
    }

    *ppSpCatCtx = pSpCatCtx;

    return err;
}

// Gets the category of a page supposedly owned by a table, including searching its children objects.
ERR ErrSPIGetSpaceCategoryObjectAndChildren(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _In_ const OBJID objid,
    _In_ const OBJID objidChildExclude,
    _Out_ OBJID* const pobjid,
    _Out_ SpaceCategoryFlags* const pspcatf,
    _Out_ SpaceCatCtx** const ppSpCatCtx )
{
    ERR err = JET_errSuccess;
    FUCB* pfucbCatalog = pfucbNil;
    OBJID objidChild = objidNil;
    SYSOBJ sysobjChild = sysobjNil;
    SpaceCatCtx* pSpCatCtxParent = NULL;

    // Parent.
    BOOL fParentOwned = fFalse;
    Call( ErrSPIGetSpaceCategoryObject( ppib, ifmp, pgno, objid, objidSystemRoot, sysobjTable, pspcatf, &fParentOwned, ppSpCatCtx ) );
    if ( !FSPSpaceCatUnknown( *pspcatf ) )
    {
        *pobjid = objid;
        goto HandleError;
    }

    // We don't need to search the children if the root object doesn't own it.
    if ( !fParentOwned )
    {
        *pobjid = objidNil;
        goto HandleError;
    }

    pSpCatCtxParent = *ppSpCatCtx;
    *ppSpCatCtx = NULL;

    // Children.
    for ( err = ErrCATGetNextNonRootObject( ppib, ifmp, objid, &pfucbCatalog, &objidChild, &sysobjChild );
            ( err >= JET_errSuccess ) && ( objidChild != objidNil );
            err = ErrCATGetNextNonRootObject( ppib, ifmp, objid, &pfucbCatalog, &objidChild, &sysobjChild ) )
    {
        if ( objidChild == objidChildExclude )
        {
            continue;
        }

        BOOL fChildOwned = fFalse;
        Call( ErrSPIGetSpaceCategoryObject( ppib, ifmp, pgno, objidChild, objid, sysobjChild, pspcatf, &fChildOwned, ppSpCatCtx ) );
        if ( !FSPSpaceCatUnknown( *pspcatf ) )
        {
            *pobjid = objidChild;
            goto HandleError;
        }

        // Leaf object owns it but can't categorize it: leak!
        if ( fChildOwned )
        {
            *pobjid = objidChild;
            *pspcatf = spcatfLeaked;
            goto HandleError;
        }

        SPFreeSpaceCatCtx( ppSpCatCtx );
    }

    Call( err );

    // If we got this far, it means the root object owns the page but none of its children do.
    // That means the page is leaked.
    *pobjid = objid;
    *pspcatf = spcatfLeaked;
    *ppSpCatCtx = pSpCatCtxParent;
    pSpCatCtxParent = NULL;

HandleError:
    if ( pfucbCatalog != pfucbNil )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }

    SPFreeSpaceCatCtx( &pSpCatCtxParent );

    if ( err < JET_errSuccess )
    {
        SPFreeSpaceCatCtx( ppSpCatCtx );
    }

    return err;
}

// Gets the category of a page, searching all its root objects and their children, but not searching
// the DB root itself.
ERR ErrSPIGetSpaceCategoryNoHint(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _In_ const OBJID objidExclude,
    _Out_ OBJID* const pobjid,
    _Out_ SpaceCategoryFlags* const pspcatf,
    _Out_ SpaceCatCtx** const ppSpCatCtx )
{
    ERR err = JET_errSuccess;
    FUCB* pfucbCatalog = pfucbNil;
    OBJID objid = objidNil;

    // All root objects (tables) and their children.
    for ( err = ErrCATGetNextRootObject( ppib, ifmp, fFalse, &pfucbCatalog, &objid );
        ( err >= JET_errSuccess ) && ( objid != objidNil );
        err = ErrCATGetNextRootObject( ppib, ifmp, fFalse, &pfucbCatalog, &objid ) )
    {
        if ( objid == objidExclude )
        {
            continue;
        }

        Call( ErrSPIGetSpaceCategoryObjectAndChildren( ppib, ifmp, pgno, objid, objidNil, pobjid, pspcatf, ppSpCatCtx ) );
        if ( !FSPSpaceCatUnknown( *pspcatf ) )
        {
            goto HandleError;
        }

        SPFreeSpaceCatCtx( ppSpCatCtx );
    }

    Call( err );

HandleError:
    if ( pfucbCatalog != pfucbNil )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }

    if ( err < JET_errSuccess )
    {
        SPFreeSpaceCatCtx( ppSpCatCtx );
    }

    return err;
}

// Returns the space category of a given page and returns a context to be use for further operations on the page.
// The context must be freed by SPFreeSpaceCatCtx().
ERR ErrSPGetSpaceCategory(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _In_ const OBJID objidHint,
    _In_ const BOOL fRunFullSpaceCat,
    _Out_ OBJID* const pobjid,
    _Out_ SpaceCategoryFlags* const pspcatf,
    _Out_ SpaceCatCtx** const ppSpCatCtx )
{
    Assert( pobjid != NULL );
    Assert( pspcatf != NULL );
    Assert( ppSpCatCtx != NULL );

    ERR err = JET_errSuccess;
    OBJID objid = objidNil;
    SpaceCategoryFlags spcatf = spcatfUnknown;
    OBJID objidHintParent = objidNil;
    OBJID objidHintPage = objidNil;
    OBJID objidHintPageParent = objidNil;
    OBJID objidHintExclude = objidNil;
    BOOL fSearchedRoot = fFalse;
    BOOL fOwned = fFalse;
    SpaceCatCtx* pSpCatCtxRoot = NULL;

    // Try hinted object first, if any, unless it's the system root (will be done later).
    if ( ( objidHint != objidNil ) &&
            ( objidHint != objidSystemRoot ) )
    {
        SYSOBJ sysobjHint = sysobjNil;
        Call( ErrCATGetObjidMetadata( ppib, ifmp, objidHint, &objidHintParent, &sysobjHint ) );
        if ( sysobjHint != sysobjNil )
        {
            Assert( objidHintParent != objidNil );

            // If the hint is for a non-root object, try that first (parent will be done later).
            if ( objidHint != objidHintParent )
            {
                Call( ErrSPIGetSpaceCategoryObject( ppib,
                                                    ifmp,
                                                    pgno,
                                                    objidHint,
                                                    objidHintParent,
                                                    sysobjHint,
                                                    &spcatf,
                                                    &fOwned,
                                                    ppSpCatCtx ) );

                // Exit if it's already known.
                if ( !FSPSpaceCatUnknown( spcatf ) )
                {
                    objid = objidHint;
                    goto HandleError;
                }

                // It's a "leaf" object, so it means the space is leaked if it owns the page but it could not be categorized.
                if ( fOwned )
                {
                    objid = objidHint;
                    spcatf = spcatfLeaked;
                    goto HandleError;
                }

                SPFreeSpaceCatCtx( ppSpCatCtx );
            }
        }
        else
        {
            objidHintParent = objidNil;
            err = JET_errSuccess;
        }
    }

    Assert( FSPSpaceCatUnknown( spcatf ) );

    // Try parent object now.
    if ( ( objidHintParent != objidNil ) &&
            ( objidHintParent != objidSystemRoot ) )
    {
        Call( ErrSPIGetSpaceCategoryObjectAndChildren( ppib,
                                                        ifmp,
                                                        pgno,
                                                        objidHintParent,
                                                        ( objidHint != objidHintParent ) ? objidHint : objidNil,
                                                        &objid,
                                                        &spcatf,
                                                        ppSpCatCtx ) );

        // Exit if it's already known.
        if ( !FSPSpaceCatUnknown( spcatf ) )
        {
            goto HandleError;
        }

        SPFreeSpaceCatCtx( ppSpCatCtx );

        objidHintExclude = objidHintParent;
    }

    Assert( FSPSpaceCatUnknown( spcatf ) );

    // Try DB root.
    if ( objidHint == objidSystemRoot )
    {
        Call( ErrSPIGetSpaceCategoryObject( ppib, ifmp, pgno, objidSystemRoot, objidNil, sysobjNil, &spcatf, &fOwned, ppSpCatCtx ) );
        fSearchedRoot = fTrue;
        if ( !FSPSpaceCatUnknown( spcatf ) )
        {
            // The first three pages of the root DB object are known.
            Assert( !FSPSpaceCatRoot( spcatf ) ||
                    ( pgno == pgnoSystemRoot ) ||
                    ( FSPSpaceCatSpaceOE( spcatf ) && ( pgno == ( pgnoSystemRoot + 1 ) ) ) ||
                    ( FSPSpaceCatSpaceAE( spcatf ) && ( pgno == ( pgnoSystemRoot + 2 ) ) ) );

            objid = objidSystemRoot;
            goto HandleError;
        }

        Assert( pSpCatCtxRoot == NULL );
        pSpCatCtxRoot = *ppSpCatCtx;
        *ppSpCatCtx = NULL;
    }

    Assert( FSPSpaceCatUnknown( spcatf ) );

    // Nothing so far, now try to get a hint from the page itself.
    err = ErrSPIGetObjidFromPage( ppib, ifmp, pgno, &objidHintPage );
    if ( err >= JET_errSuccess )
    {
        if ( ( objidHintPage != objidNil ) &&
                ( objidHintPage != objidSystemRoot ) &&
                ( objidHintPage != objidHint ) &&
                ( objidHintPage != objidHintParent ) )
        {
            SYSOBJ sysobjHint = sysobjNil;
            Call( ErrCATGetObjidMetadata( ppib, ifmp, objidHintPage, &objidHintPageParent, &sysobjHint ) );
            if ( sysobjHint != sysobjNil )
            {
                Assert( objidHintPageParent != objidNil );

                // If the hint is for a non-root object, try that first (parent will be done later).
                if ( objidHintPage != objidHintPageParent )
                {
                    Call( ErrSPIGetSpaceCategoryObject( ppib,
                                                        ifmp,
                                                        pgno,
                                                        objidHintPage,
                                                        objidHintPageParent,
                                                        sysobjHint,
                                                        &spcatf,
                                                        &fOwned,
                                                        ppSpCatCtx ) );

                    // Exit if it's already known.
                    if ( !FSPSpaceCatUnknown( spcatf ) )
                    {
                        objid = objidHintPage;
                        goto HandleError;
                    }

                    // It's a "leaf" object, so it means the space is leaked if it owns the page but it could not be categorized.
                    if ( fOwned )
                    {
                        objid = objidHintPage;
                        spcatf = spcatfLeaked;
                        goto HandleError;
                    }

                    SPFreeSpaceCatCtx( ppSpCatCtx );
                }
            }
            else
            {
                objidHintPageParent = objidNil;
                err = JET_errSuccess;
            }

            // Try parent object now.
            if ( ( objidHintPageParent != objidNil ) &&
                    ( objidHintPageParent != objidSystemRoot ) &&
                    ( objidHintPageParent != objidHint ) &&
                    ( objidHintPageParent != objidHintParent ) )
            {
                Call( ErrSPIGetSpaceCategoryObjectAndChildren( ppib,
                                                                ifmp,
                                                                pgno,
                                                                objidHintPageParent,
                                                                ( objidHintPage != objidHintPageParent ) ? objidHintPage : objidNil,
                                                                &objid,
                                                                &spcatf,
                                                                ppSpCatCtx ) );

                // Exit if it's already known.
                if ( !FSPSpaceCatUnknown( spcatf ) )
                {
                    goto HandleError;
                }

                SPFreeSpaceCatCtx( ppSpCatCtx );

                objidHintExclude = objidHintPageParent;
            }
        }
    }
    else if ( err == JET_errPageNotInitialized )
    {
        objidHintPage = objidNil;
        err = JET_errSuccess;
    }
    else
    {
        Assert( err < JET_errSuccess );
        Error( err );
    }

    Assert( FSPSpaceCatUnknown( spcatf ) );

    // Try DB root again, if not already did.
    if ( !fSearchedRoot )
    {
        Call( ErrSPIGetSpaceCategoryObject( ppib, ifmp, pgno, objidSystemRoot, objidNil, sysobjNil, &spcatf, &fOwned, ppSpCatCtx ) );
        fSearchedRoot = fTrue;
        if ( !FSPSpaceCatUnknown( spcatf ) )
        {
            objid = objidSystemRoot;
            goto HandleError;
        }

        Assert( pSpCatCtxRoot == NULL );
        pSpCatCtxRoot = *ppSpCatCtx;
        *ppSpCatCtx = NULL;
    }

    Assert( FSPSpaceCatUnknown( spcatf ) );
    Assert( fSearchedRoot );

    // Expensive search (i.e., all tables in the catalog).
    if ( fRunFullSpaceCat )
    {
        Call( ErrSPIGetSpaceCategoryNoHint( ppib, ifmp, pgno, objidHintExclude, &objid, &spcatf, ppSpCatCtx ) );
        Assert( !FSPSpaceCatIndeterminate( spcatf ) );

        if ( FSPSpaceCatUnknown( spcatf ) )
        {
            // Nothing worked: leaked from the DB root.
            objid = objidSystemRoot;
            spcatf = spcatfLeaked;

            Assert( pSpCatCtxRoot != NULL );
            *ppSpCatCtx = pSpCatCtxRoot;
            pSpCatCtxRoot = NULL;
        }
    }
    else
    {
        spcatf = spcatfIndeterminate;
    }

    Assert( FSPValidSpaceCategoryFlags( spcatf ) );
    Assert( !FSPSpaceCatUnknown( spcatf ) );

HandleError:

    SPFreeSpaceCatCtx( &pSpCatCtxRoot );

    if ( err >= JET_errSuccess )
    {
#ifdef DEBUG
        // spcatfIndeterminate pages are only possible if not running full categorization and must
        // always be returned with objidNil.
        Assert( !FSPSpaceCatIndeterminate( spcatf ) || !fRunFullSpaceCat );
        Assert( ( objid != objidNil ) || FSPSpaceCatIndeterminate( spcatf ) );

        // The first three pages of the root DB object are always known and belong
        // to the system root.
        Assert( pgno != pgnoNull );
        if ( pgno <= ( pgnoSystemRoot + 2 ) )
        {
            Assert( objid == objidSystemRoot );
            Assert( ( pgno != pgnoSystemRoot ) ||
                    ( FSPSpaceCatRoot( spcatf ) && !FSPSpaceCatAnySpaceTree( spcatf ) ) );
            Assert( ( pgno != ( pgnoSystemRoot + 1 ) ) ||
                    ( FSPSpaceCatRoot( spcatf ) && FSPSpaceCatSpaceOE( spcatf ) ) );
            Assert( ( pgno != ( pgnoSystemRoot + 2 ) ) ||
                    ( FSPSpaceCatRoot( spcatf ) && FSPSpaceCatSpaceAE( spcatf ) ) );
        }
        if ( objid == objidSystemRoot )
        {
            // Root DB must not have internal of leaf pages if not from the space trees.
            Assert( FSPSpaceCatAnySpaceTree( spcatf ) ||
                    ( !FSPSpaceCatStrictlyLeaf( spcatf ) && !FSPSpaceCatStrictlyInternal( spcatf ) ) );
        }

        // The MSO table root is always known.
        if ( pgno == pgnoFDPMSO )
        {
            Assert( objid == objidFDPMSO );
            Assert( FSPSpaceCatRoot( spcatf ) && !FSPSpaceCatAnySpaceTree( spcatf ) );
        }

        // The shadow MSO table root is always known.
        if ( pgno == pgnoFDPMSOShadow )
        {
            Assert( objid == objidFDPMSOShadow );
            Assert( FSPSpaceCatRoot( spcatf ) && !FSPSpaceCatAnySpaceTree( spcatf ) );
        }

        // The MSO name index root is always known.
        if ( pgno == pgnoFDPMSO_NameIndex )
        {
            Assert( FSPSpaceCatRoot( spcatf ) && !FSPSpaceCatAnySpaceTree( spcatf ) );
        }

        // The MSO root object index root is always known.
        if ( pgno == pgnoFDPMSO_RootObjectIndex )
        {
            Assert( FSPSpaceCatRoot( spcatf ) && !FSPSpaceCatAnySpaceTree( spcatf ) );
        }

        // Various asserts around the contract of the context returned.
        const SpaceCatCtx* const pSpCatCtx = *ppSpCatCtx;
        if ( pSpCatCtx != NULL )
        {
            Assert( !FSPSpaceCatIndeterminate( spcatf ) );

            // Object FUCB must always be initialized.
            Assert( pSpCatCtx->pfucb != pfucbNil );
            Assert( pSpCatCtx->pfucb->u.pfcb->PgnoFDP() != pgnoNull );

            if ( pSpCatCtx->pfucbParent != pfucbNil )
            {
                // It is not the DB root if it has a parent.
                Assert( objid != objidSystemRoot );

                // The parent must not be itself.
                Assert( pSpCatCtx->pfucbParent->u.pfcb != pSpCatCtx->pfucb->u.pfcb );

                // The space FUCB must not be the same as the parent FUCB.
                if ( pSpCatCtx->pfucbSpace != pfucbNil )
                {
                    Assert( pSpCatCtx->pfucbParent->u.pfcb != pSpCatCtx->pfucbSpace->u.pfcb );
                }

                // If the parent is not the DB root, it means this is not a root object and
                // the FUCB must be the current index or the LV tree.
                if ( pSpCatCtx->pfucbParent->u.pfcb->PgnoFDP() != pgnoSystemRoot )
                {
                    Assert( ( pSpCatCtx->pfucb == pSpCatCtx->pfucbParent->pfucbCurIndex ) ||
                            ( pSpCatCtx->pfucb == pSpCatCtx->pfucbParent->pfucbLV ) );
                }
            }
            else
            {
                // The DB root does not have a parent.
                Assert( objid == objidSystemRoot );
                Assert( pSpCatCtx->pfucb->u.pfcb->PgnoFDP() == pgnoSystemRoot );
            }

            // If this is a space tree page, the space FUCB must be initialized and must
            // be different than the object FUCB, but the FCBs must eb the same.
            if ( pSpCatCtx->pfucbSpace != pfucbNil )
            {
                Assert( FSPSpaceCatAnySpaceTree( spcatf ) );
                Assert( ( FSPSpaceCatSpaceOE( spcatf ) && FFUCBOwnExt( pSpCatCtx->pfucbSpace ) ) ||
                        ( FSPSpaceCatSpaceAE( spcatf ) && FFUCBAvailExt( pSpCatCtx->pfucbSpace ) ) );
                Assert( pSpCatCtx->pfucbSpace != pSpCatCtx->pfucb );
                Assert( pSpCatCtx->pfucbSpace->u.pfcb == pSpCatCtx->pfucb->u.pfcb );

                // Root pgnos are consistent.
                if ( FSPSpaceCatRoot( spcatf ) )
                {
                    Assert( ( FSPSpaceCatSpaceOE( spcatf ) && ( pSpCatCtx->pfucb->u.pfcb->PgnoOE() == pgno ) ) ||
                            ( FSPSpaceCatSpaceAE( spcatf ) && ( pSpCatCtx->pfucb->u.pfcb->PgnoAE() == pgno ) ) );
                }
            }
            else
            {
                Assert( !FSPSpaceCatAnySpaceTree( spcatf ) );

                // Root pgno is consistent.
                if ( FSPSpaceCatRoot( spcatf ) )
                {
                    Assert( pSpCatCtx->pfucb->u.pfcb->PgnoFDP() == pgno );
                }
            }

            // We must have a bookmark if this is an internal or leaf page.
            Assert( !FSPSpaceCatStrictlyInternal( spcatf ) && !FSPSpaceCatStrictlyLeaf( spcatf ) || ( pSpCatCtx->pbm != NULL )  );
        }
        else
        {
            Assert( FSPSpaceCatIndeterminate( spcatf ) );
        }
#endif

        if ( ( !FSPValidSpaceCategoryFlags( spcatf ) ||
                    FSPSpaceCatUnknown( spcatf ) ||
                    FSPSpaceCatInconsistent( spcatf ) ) )
        {
            FireWall( OSFormat( "InconsistentCategoryFlags:0x%I32x", spcatf ) );
        }

        *pobjid = objid;
        *pspcatf = spcatf;
        err = JET_errSuccess;
    }
    else
    {
        SPFreeSpaceCatCtx( ppSpCatCtx );
        *pobjid = objidNil;
        *pspcatf = spcatfUnknown;
    }

    return err;
}

// Returns the space category of a given page.
ERR ErrSPGetSpaceCategory(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _In_ const OBJID objidHint,
    _In_ const BOOL fRunFullSpaceCat,
    _Out_ OBJID* const pobjid,
    _Out_ SpaceCategoryFlags* const pspcatf )
{
    SpaceCatCtx* pSpCatCtx = NULL;
    const ERR err = ErrSPGetSpaceCategory( ppib, ifmp, pgno, objidHint, fRunFullSpaceCat, pobjid, pspcatf, &pSpCatCtx );
    SPFreeSpaceCatCtx( &pSpCatCtx );
    return err;
}

// Returns the space category of page range, via callbacks.
ERR ErrSPGetSpaceCategoryRange(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgnoFirst,
    _In_ const PGNO pgnoLast,
    _In_ const BOOL fRunFullSpaceCat,
    _In_ const JET_SPCATCALLBACK pfnCallback,
    _In_opt_ VOID* const pvContext )
{
    ERR err = JET_errSuccess;
    BOOL fMSysObjidsReady = fFalse;

    if ( ( pgnoFirst < 1 ) || ( pgnoLast > pgnoSysMax ) || ( pgnoFirst > pgnoLast ) || ( pfnCallback == NULL ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Call( ErrCATCheckMSObjidsReady( ppib, ifmp, &fMSysObjidsReady ) );
    if ( !fMSysObjidsReady )
    {
        Error( ErrERRCheck( JET_errObjectNotFound ) );
    }

    const CPG cpgPrereadMax = LFunctionalMax( 1, (CPG)( UlParam( JET_paramMaxCoalesceReadSize ) / g_cbPage ) );
    PGNO pgnoPrereadWaypoint = pgnoFirst, pgnoPrereadNext = pgnoFirst;
    OBJID objidHint = objidNil;

    for ( PGNO pgnoCurrent = pgnoFirst; pgnoCurrent <= pgnoLast; pgnoCurrent++ )
    {
        // Preread pages if we crossed the preread waypoint.
        if ( ( pgnoCurrent >= pgnoPrereadWaypoint ) && ( pgnoPrereadNext <= pgnoLast ) )
        {
            PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
            tcScope->iorReason.SetIort( iortDbShrink );
            const CPG cpgPrereadCurrent = LFunctionalMin( cpgPrereadMax, (CPG)( pgnoLast - pgnoPrereadNext + 1 ) );
            Assert( cpgPrereadCurrent >= 1 );
            BFPrereadPageRange( ifmp, pgnoPrereadNext, cpgPrereadCurrent, bfprfDefault, ppib->BfpriPriority( ifmp ), *tcScope );
            pgnoPrereadNext += cpgPrereadCurrent;
            pgnoPrereadWaypoint = pgnoPrereadNext - ( cpgPrereadCurrent / 2 );
        }

        // Categorize page.
        OBJID objidCurrent = objidNil;
        SpaceCategoryFlags spcatfCurrent = spcatfNone;
        Call( ErrSPGetSpaceCategory(
                ppib,
                ifmp,
                pgnoCurrent,
                objidHint,
                fRunFullSpaceCat,
                &objidCurrent,
                &spcatfCurrent ) );
        objidHint = ( objidCurrent != objidNil ) ? objidCurrent : objidHint;
        Assert( FSPValidSpaceCategoryFlags( spcatfCurrent ) ); // The combination must be valid.
        Assert( !FSPSpaceCatUnknown( spcatfCurrent ) ); // We should not get this here.

        // Issue callback.
        pfnCallback( pgnoCurrent, objidCurrent, spcatfCurrent, pvContext );
    }

HandleError:
    return err;
}

ERR ErrSPICheckExtentIsAllocatable( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg )
{
    Assert( ifmp != ifmpNil );
    Assert( pgnoFirst != pgnoNull );
    Assert( cpg > 0 );

    if ( g_rgfmp[ ifmp ].FBeyondPgnoShrinkTarget( pgnoFirst, cpg ) )
    {
        AssertTrack( fFalse, "ShrinkExtentOverlap" );
        return ErrERRCheck( JET_errSPAvailExtCorrupted );
    }

    if ( ( pgnoFirst + cpg - 1 ) > g_rgfmp[ ifmp ].PgnoLast() )
    {
        AssertTrack( fFalse, "PageBeyondEofAlloc" );
        return ErrERRCheck( JET_errSPAvailExtCorrupted );
    }

    return JET_errSuccess;
}

ERR ErrSPISmallGetExt(
    FUCB        *pfucbParent,
    CPG         *pcpgReq,
    CPG         cpgMin,
    PGNO        *ppgnoFirst )
{
    FCB         * const pfcb    = pfucbParent->u.pfcb;
    ERR                 err     = errSPNoSpaceForYou;

    AssertSPIPfucbOnRoot( pfucbParent );
    Assert( pfcb->PgnoFDP() != pgnoSystemRoot );

    //  if any chance in satisfying request from extent
    //  then try to allcate requested extent, or minimum
    //  extent from first available extent.  Only try to
    //  allocate the minimum request, to facillitate
    //  efficient space usage.
    //
    if ( cpgMin <= cpgSmallSpaceAvailMost )
    {
        SPACE_HEADER    sph;
        UINT            rgbitT;
        PGNO            pgnoAvail;
        DATA            data;
        INT             iT;

        //  get external header
        //
        NDGetExternalHeader( pfucbParent, pfucbParent->pcsrRoot, noderfSpaceHeader );
        Assert( pfucbParent->pcsrRoot->Latch() == latchRIW
                    || pfucbParent->pcsrRoot->Latch() == latchWrite );
        Assert( sizeof( SPACE_HEADER ) == pfucbParent->kdfCurr.data.Cb() );

        //  get single extent allocation information
        //
        UtilMemCpy( &sph, pfucbParent->kdfCurr.data.Pv(), sizeof(sph) );

        const PGNO  pgnoFDP         = PgnoFDP( pfucbParent );
        const CPG   cpgSingleExtent = min( sph.CpgPrimary(), cpgSmallSpaceAvailMost+1 );    //  +1 because pgnoFDP is not represented in the single extent space map
        Assert( cpgSingleExtent > 0 );
        Assert( cpgSingleExtent <= cpgSmallSpaceAvailMost+1 );

        //  find first fit
        //
        //  make mask for minimum request
        //
        Assert( cpgMin > 0 );
        Assert( cpgMin <= 32 );
        for ( rgbitT = 1, iT = 1; iT < cpgMin; iT++ )
        {
            rgbitT = (rgbitT<<1) + 1;
        }

        for( pgnoAvail = pgnoFDP + 1;
            pgnoAvail + cpgMin <= pgnoFDP + cpgSingleExtent;
            pgnoAvail++, rgbitT <<= 1 )
        {
            Assert( rgbitT != 0 );
            if ( ( rgbitT & sph.RgbitAvail() ) == rgbitT )
            {
                SPIUpgradeToWriteLatch( pfucbParent );

                sph.SetRgbitAvail( sph.RgbitAvail() ^ rgbitT );
                data.SetPv( &sph );
                data.SetCb( sizeof(sph) );
                Assert( errSPNoSpaceForYou == err );

                Call( ErrSPIWrappedNDSetExternalHeader(
                          pfucbParent,
                          pfucbParent->pcsrRoot,
                          &data,
                          ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                          noderfSpaceHeader,
                          0,
                          -( cpgMin ) ) );

                //  set up allocated extent as FDP if requested
                //
                Assert( pgnoAvail != pgnoNull );

                //  at this point, we should have moved small extents out of the shrink range
                //  if we are shrinking.
                //
                Assert( !g_rgfmp[ pfucbParent->ifmp ].FBeyondPgnoShrinkTarget( pgnoAvail, cpgMin ) );

                //  we should also never have small-extent pages beyond EOF (and potentially shelved).
                //
                Assert( ( pgnoAvail + cpgMin - 1 ) <= g_rgfmp[ pfucbParent->ifmp ].PgnoLast() );

                *ppgnoFirst = pgnoAvail;
                *pcpgReq = cpgMin;
                err = JET_errSuccess;
                goto HandleError;
            }
        }
    }

HandleError:
    return err;
}

//  pfucb only passed for asserts and to maintain NextSE cache (via pfcb).
ERR ErrSPIAEFindExt(
    __inout FUCB * const    pfucbAE,
    _In_    const CPG       cpgMin,
    _In_    const SpacePool sppAvailPool,
    _Out_   CSPExtentInfo * pcspaei )
{
    ERR         err = JET_errSuccess;
    DIB         dib;
    BOOL        fFoundNextAvailSE       = fFalse;
    FCB* const  pfcb = pfucbAE->u.pfcb;

    Assert( FFUCBAvailExt( pfucbAE ) );
    Assert( !FSPIIsSmall( pfcb ) );

    //  begin search for first extent with size greater than request.
    //  Allocate secondary extent recursively until satisfactory extent found
    //

    //  most of the time, we simply start searching from the beginning of the AvailExt tree,
    //  but if this is the db root, optimise for the common case (request for an SE) by skipping
    //  small AvailExt nodes
    const PGNO  pgnoSeek = ( cpgMin >= cpageSEDefault ? pfcb->PgnoNextAvailSE() : pgnoNull );

    const CSPExtentKeyBM    spaeFindExt( SPEXTKEY::fSPExtentTypeAE, pgnoSeek, sppAvailPool );
    dib.pos = posDown;
    dib.pbm = spaeFindExt.Pbm( pfucbAE );

    //  begin search for first extent with size greater than request.
    //  Allocate secondary extent recursively until satisfactory extent found
    //

    dib.dirflag = fDIRFavourNext;

    if ( ( err = ErrBTDown( pfucbAE, &dib, latchReadTouch ) ) < 0 )
    {
        Assert( err != JET_errNoCurrentRecord );
        //  When AE tree empty, JET_errRecordNotFound is returned.
        if ( JET_errRecordNotFound == err )
        {
            //  no record in availExt tree
            //
            Error( ErrERRCheck( errSPNoSpaceForYou ) );
        }
        OSTraceFMP(
            pfucbAE->ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: could not down into available extent tree. [ifmp=0x%x].",
                __FUNCTION__,
                pfucbAE->ifmp ) );
        Call( err );
    }


    //  loop through extents looking for one large enough for allocation
    //
    do
    {
        pcspaei->Set( pfucbAE );

        //  We have ventured out of the desired pool.
        //
        if ( pcspaei->SppPool() != sppAvailPool )
        {
            Error( ErrERRCheck( errSPNoSpaceForYou ) );
        }

#ifdef DEPRECATED_ZERO_SIZED_AVAIL_EXTENT_CLEANUP
        if ( 0 == pcspaei->CpgExtent() )
        {
            //  We might have zero-sized extents if we crashed in ErrSPIAddToAvailExt().
            //  Simply delete such extents.
            Call( ErrBTFlagDelete(      // UNDONE: Synchronously remove the node
                        pfucbAE,
                        fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
            // Zero pages, so no need to call pfucbAE->u.pfcb->AddCpgAE()
            Pcsr( pfucbAE )->Downgrade( latchReadTouch );
        }
#endif

        if ( pcspaei->CpgExtent() > 0 )
        {
            //  Check if the current extent violates the shrink constraints.
            if ( g_rgfmp[ pfucbAE->ifmp ].FBeyondPgnoShrinkTarget( pcspaei->PgnoFirst(), pcspaei->CpgExtent() ) )
            {
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            //  Check if the current extent is beyond EOF (and potentially shelved).
            if ( pcspaei->PgnoLast() > g_rgfmp[ pfucbAE->ifmp ].PgnoLast() )
            {
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            if ( !fFoundNextAvailSE
                && pcspaei->CpgExtent() >= cpageSEDefault )
            {
                pfcb->SetPgnoNextAvailSE( pcspaei->PgnoLast() - pcspaei->CpgExtent() + 1 );
                fFoundNextAvailSE = fTrue;
            }

            if ( pcspaei->CpgExtent() >= cpgMin )
            {
                //  if no extent with cpg >= cpageSEDefault, then ensure NextAvailSE
                //  pointer is at least as far as we've scanned
                if ( !fFoundNextAvailSE
                    && pfcb->PgnoNextAvailSE() <= pcspaei->PgnoLast() )
                {
                    pfcb->SetPgnoNextAvailSE( pcspaei->PgnoLast() + 1 );
                }
                err = JET_errSuccess;
                goto HandleError;
            }
        }

        err = ErrBTNext( pfucbAE, fDIRNull );
    }
    while ( err >= 0 );

    if ( err != JET_errNoCurrentRecord )
    {
        OSTraceFMP(
            pfucbAE->ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Could not scan available extent tree. [ifmp=0x%x].",
                __FUNCTION__,
                pfucbAE->ifmp ) );
        Assert( err < 0 );
        goto HandleError;
    }

    if ( !fFoundNextAvailSE )
    {
        //  didn't find any extents with at least cpageSEDefault pages, so
        //  set pointer to beyond last extent
        pfcb->SetPgnoNextAvailSE( pcspaei->PgnoLast() + 1 );
    }

    err = ErrERRCheck( errSPNoSpaceForYou );

HandleError:
    if ( err >= JET_errSuccess )
    {
        Assert( pcspaei->FIsSet() );
        const ERR errT = ErrSPICheckExtentIsAllocatable( pfucbAE->ifmp, pcspaei->PgnoFirst(), pcspaei->CpgExtent() );
        if ( errT < JET_errSuccess )
        {
            err = errT;
        }
    }

    Assert( err != JET_errRecordNotFound );
    Assert( err != JET_errNoCurrentRecord );

    if ( err < JET_errSuccess )
    {
        pcspaei->Unset();
        BTUp( pfucbAE );
    }
    else
    {
        Assert( Pcsr( pfucbAE )->FLatched() );
    }

    return err;
}

//  gets an extent from tree
//  if space not available in given tree, get from its parent
//
//  pfcbDstTracingOnly is used for tracing only
//
//  pfucbSrc cursor placed on root of tree to get the extent from
//      root page should be RIW latched
//
//  *pcpgReq input is number of pages requested;
//           output is number of pages granted
//  cpgMin is minimum neeeded
//  *ppgnoFirst input gives locality of extent needed
//              output is first page of allocated extent
//  pobjidFDP, pcpgOEFDP, pcpgAEFDP returns data on newly created FDPs.
//            not optional if ( fSPFlags & fSPNewFDP )
LOCAL ERR ErrSPIGetExt(
    _In_ FCB        *pfcbDstTracingOnly,
    _Inout_ FUCB    *pfucbSrc,
    _Inout_ CPG     *pcpgReq,
    _In_ CPG        cpgMin,
    _Out_ PGNO      *ppgnoFirst,
    _In_ ULONG      fSPFlags = 0,
    _In_ UINT       fPageFlags = 0,
    _Out_opt_ OBJID *pobjidFDP = NULL,
    _Out_opt_ CPG   *pcpgOEFDP = NULL,
    _Out_opt_ CPG   *pcpgAEFDP = NULL,
    _In_ const BOOL fMayViolateMaxSize = fFalse )
{
    ERR         err;
    PIB         * const ppib            = pfucbSrc->ppib;
    FCB         * const pfcb            = pfucbSrc->u.pfcb;
    FUCB        *pfucbAE                = pfucbNil;
    const BOOL  fSelfReserving          = ( fSPFlags & fSPSelfReserveExtent ) != 0;

    CSPExtentInfo cspaei;

    //  check parameters.  If setting up new FDP, increment requested number of
    //  pages to account for consumption of first page to make FDP.
    //
    Assert( ( fSPFlags & fSPNewFDP ) ? ( NULL != pobjidFDP ) : fTrue );
    Assert( ( fSPFlags & fSPNewFDP ) ? ( NULL != pcpgOEFDP ) : fTrue );
    Assert( ( fSPFlags & fSPNewFDP ) ? ( NULL != pcpgAEFDP ) : fTrue );
    Assert( ( fSPFlags & fSPNewFDP ) ? ( 0 <= *pcpgReq ) : ( 0 < *pcpgReq ) );
    Assert( *pcpgReq >= cpgMin );
    Assert( !FFUCBSpace( pfucbSrc ) );
    AssertSPIPfucbOnRoot( pfucbSrc );

#ifdef SPACECHECK
    Assert( !( ErrSPIValidFDP( ppib, pfucbParent->ifmp, PgnoFDP( pfucbParent ) ) < 0 ) );
#endif

    OSTraceFMP(
        pfucbSrc->ifmp,
        JET_tracetagSpaceManagement,
        OSFormat(
            "%hs: Getting %hs Req(%d) Min(%d) Flags(0x%x) from [0x%x:0x%x:%lu] for [0x%x:0x%x:%lu].",
            __FUNCTION__,
            fSPFlags & fSPNewFDP ? "NewExtFDP" : "AddlExt",
            *pcpgReq,
            cpgMin,
            fSPFlags,
            pfucbSrc->ifmp,
            ObjidFDP( pfucbSrc ),
            PgnoFDP( pfucbSrc ),
            pfucbSrc->ifmp,
            pfcbDstTracingOnly->ObjidFDP(),
            pfcbDstTracingOnly->PgnoFDP()
            ) );

    if ( NULL != pobjidFDP )
    {
        *pobjidFDP = objidNil;
    }
    if ( NULL != pcpgOEFDP )
    {
        *pcpgOEFDP = 0;
    }
    if ( NULL != pcpgAEFDP )
    {
        *pcpgAEFDP = 0;
    }
    //  if a new FDP is requested, increment request count by FDP overhead,
    //  unless the count was smaller than the FDP overhead, in which case
    //  just allocate enough to satisfy the overhead (because the FDP will
    //  likely be small).
    //
    if ( fSPFlags & fSPNewFDP )
    {
        Assert( !fSelfReserving );
        const CPG   cpgFDPMin = ( fSPFlags & fSPMultipleExtent ?
                                        cpgMultipleExtentMin :
                                        cpgSingleExtentMin );
        cpgMin = max( cpgMin, cpgFDPMin );
        *pcpgReq = max( *pcpgReq, cpgMin );

    }

    Assert( cpgMin > 0 );

    if ( !pfcb->FSpaceInitialized() )
    {
        SPIInitFCB( pfucbSrc, fTrue );
    }

    //
    // Make sure the values are correct before we start
    //
    SPIValidateCpgOwnedAndAvail( pfucbSrc );

    //  if single extent optimization, then try to allocate from
    //  root page space map.  If cannot satisfy allocation, convert
    //  to multiple extent representation.
    if ( FSPIIsSmall( pfcb ) )
    {
        Expected( !fSelfReserving );
        Assert( !fMayViolateMaxSize );
        err = ErrSPISmallGetExt( pfucbSrc, pcpgReq, cpgMin, ppgnoFirst );
        if ( JET_errSuccess <= err )
        {
            Expected( err == JET_errSuccess );
            goto NewFDP;
        }

        if( errSPNoSpaceForYou == err )
        {
            CallR( ErrSPIConvertToMultipleExtent( pfucbSrc, *pcpgReq, cpgMin ) );
        }

        Assert( FSPExpectedError( err ) );
        Call( err );
    }

    //  for secondary extent allocation, only the normal Btree operations
    //  are logged. For allocating a new FDP, a special create FDP
    //  record is logged instead, since the new FDP and space pages need
    //  to be initialized as part of recovery.
    //
    //  move to available extents
    //
    CallR( ErrSPIOpenAvailExt( pfucbSrc, &pfucbAE ) );
    Assert( pfcb == pfucbAE->u.pfcb );

    SPIValidateCpgOwnedAndAvail( pfucbSrc );

    if ( pfcb->FUtilizeParentSpace() ||
        //  Make sure we search the first level if we are self-reserving.
        fSelfReserving ||
        //  We can not do this with system allocations, their placement is extremely carefully 
        //  controlled due to create DB being non-logged.
        FCATBaseSystemFDP( pfcb->PgnoFDP() ) )
    {
        //  If we are self-reserving, we do not want to end up adding or deleting nodes from our own
        //  space trees. Therefore, we are going to make sure we are guaranteed not to consume the entire
        //  extent returned, so we'll request/reserve one extra page, but only allocate/consume exactly
        //  cpgMin afterwards.
        const CPG cpgMinT = fSelfReserving ? ( cpgMin + 1 ) : cpgMin;

        err = ErrSPIAEFindExt( pfucbAE, cpgMinT, spp::AvailExtLegacyGeneralPool, &cspaei );
        Assert( err != JET_errRecordNotFound );
        Assert( err != JET_errNoCurrentRecord );

        if ( err >= JET_errSuccess )
        {
            Assert( cspaei.CpgExtent() >= cpgMinT );
            const CPG cpgMax = fSelfReserving ? ( cspaei.CpgExtent() - 1 ) : cspaei.CpgExtent();
            *pcpgReq = min( cpgMax, *pcpgReq );
        }
        else if ( err != errSPNoSpaceForYou )
        {
            //   other, unexpected failure, bail.
            Call( err );
        }

        //  If errSPNoSpaceForYou we fall through and get a secondary ext to satisfy 
        //  our hunger for space.
    }
    else
    {
        //  This is the default behavior ...
        //  There is no space for us here, we have to drop through and get from parent.
        err = ErrERRCheck( errSPNoSpaceForYou );
    }

    if ( err == errSPNoSpaceForYou )
    {
        BTUp( pfucbAE );

        //  We could not find any suitable space in our AE, ask daddy.

        if ( fSelfReserving )
        {
            Error( err );
        }

        if ( !FSPIParentIsFs( pfucbSrc ) )
        {
            Call( ErrSPIGetSe(
                        pfucbSrc,
                        pfucbAE,
                        *pcpgReq,
                        cpgMin,
                        fSPFlags & ( fSPSplitting | fSPExactExtent ),
                        spp::AvailExtLegacyGeneralPool,
                        fMayViolateMaxSize ) );
        }
        else
        {
            Call( ErrSPIGetFsSe(
                        pfucbSrc,
                        pfucbAE,
                        *pcpgReq,
                        cpgMin,
                        fSPFlags & ( fSPSplitting | fSPExactExtent ),
                        fFalse, // fExact
                        fTrue,  // fPermitAsyncExtension
                        fMayViolateMaxSize ) );
        }

        Assert( Pcsr( pfucbAE )->FLatched() );
        cspaei.Set( pfucbAE );
        Assert( cspaei.CpgExtent() > 0 );
        Assert( cspaei.CpgExtent() >= cpgMin );
    }

    //  validate this number for retail builds:
    // FUTURE (as of 9/07/2006): Add a retail check that (cpgAvailExt < cpgMin)
    err = cspaei.ErrCheckCorrupted();
    if ( err < JET_errSuccess )
    {
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"8aed90f2-6b6f-4bc4-902e-5ef1f328ebc3" );
    }
    Call( err );

    //  We believe we have a valid extent, lets consume some of it.
    //
    Assert( Pcsr( pfucbAE )->FLatched() );

    *ppgnoFirst = cspaei.PgnoFirst();

    const CPG cpgDb = (CPG)g_rgfmp[ pfucbAE->ifmp ].PgnoLast();

    if ( pfcb->PgnoFDP() == pgnoSystemRoot )
    {
        OSTraceFMP(
            pfucbSrc->ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: RootSpace E(%lu + %lu) PageFragment(%d) cpgDb(%d) R:%d.",
                __FUNCTION__,
                cspaei.PgnoFirst(),
                cspaei.CpgExtent(), 
                (LONG)UlParam( PinstFromPpib( ppib ), JET_paramPageFragment ),
                cpgDb,
                pgnoSystemRoot) );
    }

    Assert( *pcpgReq >= cpgMin );
    Assert( cspaei.CpgExtent() >= cpgMin );
    Assert( !fSelfReserving || ( cspaei.CpgExtent() > *pcpgReq ) );
    if ( FSPIAllocateAllAvail( &cspaei, *pcpgReq, (LONG)UlParam( PinstFromPpib( ppib ), JET_paramPageFragment ), pfcb->PgnoFDP(), cpgDb, fSPFlags )
            && !fSelfReserving )
    {
        *pcpgReq = cspaei.CpgExtent();
        SPCheckPgnoAllocTrap( *ppgnoFirst, *pcpgReq );

        // UNDONE: *pcpgReq may actually be less than cpgMin, because
        // some pages may have been used up if split occurred while
        // updating AvailExt.  However, as long as there's at least
        // one page, this is okay, because the only ones to call this
        // function are GetPage() (for split) and CreateDirectory()
        // (for CreateTable/Index).  The former only ever asks for
        // one page at a time, and the latter can deal with getting
        // only one page even though it asked for more.
        // Assert( *pcpgReq >= cpgMin );
        Assert( cspaei.CpgExtent() > 0 );

        Call( ErrSPIWrappedBTFlagDelete(      // UNDONE: Synchronously remove the node
                    pfucbAE,
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    0,
                    -( *pcpgReq ) ) );

    }
    else
    {
        //  *pcpgReq is already set to the return value
        //
        Assert( cspaei.CpgExtent() > *pcpgReq );
        SPCheckPgnoAllocTrap( *ppgnoFirst, *pcpgReq );

        CSPExtentNodeKDF spAdjustedSize( SPEXTKEY::fSPExtentTypeAE, cspaei.PgnoLast(), cspaei.CpgExtent(), spp::AvailExtLegacyGeneralPool );

        OnDebug( const PGNO pgnoLastBefore = cspaei.PgnoLast() );
        Call( spAdjustedSize.ErrConsumeSpace( *ppgnoFirst, *pcpgReq ) );
        Assert( spAdjustedSize.CpgExtent() > 0 );
        Assert( pgnoLastBefore == cspaei.PgnoLast() );

        Call( ErrSPIWrappedBTReplace(
                    pfucbAE,
                    spAdjustedSize.GetData( ),
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    0,
                    -( *pcpgReq ) ) );

        err = JET_errSuccess;
    }

    BTUp( pfucbAE );

    //
    // Make sure the values are correct after we're done.
    //
    SPIValidateCpgOwnedAndAvail( pfucbSrc );

NewFDP:
    //  initialize extent as new tree, including support for
    //  localized space allocation.
    //
    if ( fSPFlags & fSPNewFDP )
    {
        //  database root is allocated by DBInitDatabase
        //
        Assert( pgnoSystemRoot != *ppgnoFirst );
        Assert( PgnoFDP( pfucbSrc ) != *ppgnoFirst );
        Assert( PgnoFDP( pfucbSrc ) != pgnoNull );
        Assert( ( fSPFlags & fSPMultipleExtent ) ? ( *pcpgReq >= cpgMultipleExtentMin ) : ( *pcpgReq >= cpgSingleExtentMin ) );
        Assert( ( 0 != ppib->Level() ) || ( fSPFlags & fSPUnversionedExtent ) );
        Assert( ( 0 != ppib->Level() ) || g_rgfmp[ pfucbSrc->ifmp ].FIsTempDB() || g_rgfmp[ pfucbSrc->ifmp ].FCreatingDB() );

        VEREXT  verext;
        verext.pgnoFDP = PgnoFDP( pfucbSrc );
        verext.pgnoChildFDP = *ppgnoFirst;
        verext.pgnoFirst = *ppgnoFirst;
        verext.cpgSize = *pcpgReq;

        if ( !( fSPFlags & fSPUnversionedExtent ) )
        {
            VER *pver = PverFromIfmp( pfucbSrc->ifmp );
            Call( pver->ErrVERFlag( pfucbSrc, operAllocExt, &verext, sizeof(verext) ) );
        }

        Call( ErrSPCreate(
                    ppib,
                    pfucbSrc->ifmp,
                    PgnoFDP( pfucbSrc ),
                    *ppgnoFirst,
                    *pcpgReq,
                    fSPFlags,
                    fPageFlags,
                    pobjidFDP,
                    pcpgOEFDP,
                    pcpgAEFDP ) );

        Assert( *pobjidFDP > objidSystemRoot );

        //  reduce *pcpgReq by pages allocated for tree root
        //
        if ( fSPFlags & fSPMultipleExtent )
        {
            (*pcpgReq) -= cpgMultipleExtentMin;
        }
        else
        {
            (*pcpgReq) -= cpgSingleExtentMin;
        }

        SPIValidateCpgOwnedAndAvail( pfucbSrc );
    }


    //  assign error
    //
    err = JET_errSuccess;

    CPG cpgExt = *pcpgReq;

    if ( fSPFlags & fSPNewFDP)
    {
        if ( fSPFlags & fSPMultipleExtent )
        {
            cpgExt += cpgMultipleExtentMin;
        }
        else
        {
            cpgExt += cpgSingleExtentMin;
        }
    }

    SPIValidateCpgOwnedAndAvail( pfucbSrc );

    const PGNO pgnoParentFDP = PgnoFDP( pfucbSrc );
    const PGNO pgnoFDP = pfcbDstTracingOnly->PgnoFDP();
    ETSpaceAllocExt( pfucbSrc->ifmp, pgnoFDP, *ppgnoFirst, cpgExt, pfcbDstTracingOnly->ObjidFDP(), pfcbDstTracingOnly->TCE() );
    OSTraceFMP(
        pfucbSrc->ifmp,
        JET_tracetagSpaceManagement,
        OSFormat(
            "%hs: Got %hs E(%lu + %lu) Flags(0x%x) from [0x%x:0x%x:%lu] to [0x%x:0x%x:%lu].",
            __FUNCTION__,
            fSPFlags & fSPNewFDP ? "NewExtFDP" : "AddlExt",
            *ppgnoFirst,
            cpgExt,
            fSPFlags,
            pfucbSrc->ifmp,
            ObjidFDP( pfucbSrc ),
            PgnoFDP( pfucbSrc ),
            pfucbSrc->ifmp,
            pfcbDstTracingOnly->ObjidFDP(),
            pfcbDstTracingOnly->PgnoFDP()
            ) );

    //  Just putting this in for awhile, to ensure I did the above calc correctly to 
    //  match existing code here.
    Assert( cpgExt == ( (fSPFlags & fSPNewFDP) ?
                        *pcpgReq + ( (fSPFlags & fSPMultipleExtent) ? cpgMultipleExtentMin : cpgSingleExtentMin ) : *pcpgReq ) );

    //  count pages allocated by a table
    //
    if ( pgnoParentFDP == pgnoSystemRoot )
    {
        TLS* const ptls = Ptls();
        ptls->threadstats.cPageTableAllocated += cpgExt;
    }

HandleError:
    if ( pfucbAE != pfucbNil )
    {
        BTClose( pfucbAE );
    }

    return err;
}

ERR ErrSPGetExt(
    FUCB        *pfucb,
    PGNO        pgnoParentFDP,
    CPG         *pcpgReq,
    CPG         cpgMin,
    PGNO        *ppgnoFirst,
    ULONG       fSPFlags,
    UINT        fPageFlags,
    OBJID       *pobjidFDP )
{
    ERR     err;
    FUCB    *pfucbParent = pfucbNil;
    CPG     cpgOEFDP;
    CPG     cpgAEFDP;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortSpace );

    Assert( !g_rgfmp[ pfucb->ifmp ].FReadOnlyAttach() );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( 0 == ( fSPFlags & ~fMaskSPGetExt ) );

    //  open cursor on Parent and RIW latch root page
    //
    Call( ErrBTIOpenAndGotoRoot( pfucb->ppib, pgnoParentFDP, pfucb->ifmp, &pfucbParent ) );

    //  allocate an extent
    //  Note: We get back info on OE and AE so we can add the value to the cpg cache
    //  AFTER we've released the root.  This is because adding a value to the cpg cache
    //  may cause a split in the cpg cache table, and that means we'd need to get space
    //  from the DBRoot.  If pgnoParentFDP happens to be systemRoot, that results in
    //  trying to latch the page twice, one in ErrBTIOpenAndGotoRoot and one several
    //  levels lower in the callstack.

    err = ErrSPIGetExt(
              pfucb->u.pfcb,
              pfucbParent,
              pcpgReq,
              cpgMin,
              ppgnoFirst,
              fSPFlags,
              fPageFlags,
              pobjidFDP,
              &cpgOEFDP,
              &cpgAEFDP );

    Assert( Pcsr( pfucbParent ) == pfucbParent->pcsrRoot );

    //  latch may have been upgraded to write latch
    //  by single extent space allocation operation.
    //
    Assert( pfucbParent->pcsrRoot->Latch() == latchRIW
        || pfucbParent->pcsrRoot->Latch() == latchWrite );
    pfucbParent->pcsrRoot->ReleasePage();
    pfucbParent->pcsrRoot = pcsrNil;
    Call( err );

    if ( fSPFlags & fSPNewFDP )
    {
        Assert( NULL != pobjidFDP );
        Assert( objidNil != *pobjidFDP );

        // Track FCBs at creation.  We also have a mechanism to add them on-demand for
        // tables that predate the feature being on.
        CATSetExtentPageCounts(
            pfucb->ppib,
            pfucb->ifmp,
            *pobjidFDP,
            cpgOEFDP,
            cpgAEFDP
            );
    }
    else
    {
        Assert( ( cpgOEFDP == 0 ) && ( cpgAEFDP == 0 ) );
    }

HandleError:
    if ( pfucbParent != pfucbNil ) {
        BTClose( pfucbParent );
    }

    return err;
}


//  check FUCB work area for active extent and allocate first available
//  page of active extent
//
ERR ErrSPISPGetPage(
    __inout FUCB *          pfucb,
    _Out_   PGNO *          ppgnoAlloc )
{
    ERR         err = JET_errSuccess;
    const BOOL  fAvailExt   = FFUCBAvailExt( pfucb );

    Assert( FFUCBSpace( pfucb ) );
    Assert( ppgnoAlloc );
    Assert( pgnoNull == *ppgnoAlloc );


    // Pages in the split buffers are not counted in the Extent Page Count Cache.
    if ( NULL == pfucb->u.pfcb->Psplitbuf( fAvailExt ) )
    {
        CSR             *pcsrRoot   = pfucb->pcsrRoot;
        SPLIT_BUFFER    spbuf;
        DATA            data;

        UtilMemCpy( &spbuf, PspbufSPISpaceTreeRootPage( pfucb, pcsrRoot ), sizeof(SPLIT_BUFFER) );

        CallR( spbuf.ErrGetPage( pfucb->ifmp, ppgnoAlloc, fAvailExt ) );

        Assert( latchRIW == pcsrRoot->Latch() );
        pcsrRoot->UpgradeFromRIWLatch();

        data.SetPv( &spbuf );
        data.SetCb( sizeof(spbuf) );
        err = ErrSPIWrappedNDSetExternalHeader(
                    pfucb,
                    pcsrRoot,
                    &data,
                    ( pfucb->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    noderfWhole );

        //  reset to RIW latch
        pcsrRoot->Downgrade( latchRIW );

    }
    else
    {
        SPITraceSplitBufferMsg( pfucb, "Retrieved page from" );
        err = pfucb->u.pfcb->Psplitbuf( fAvailExt )->ErrGetPage( pfucb->ifmp, ppgnoAlloc, fAvailExt );
    }

    Assert( ( err < JET_errSuccess ) || !g_rgfmp[ pfucb->ifmp ].FBeyondPgnoShrinkTarget( *ppgnoAlloc, 1 ) );
    Assert( ( err < JET_errSuccess ) || ( *ppgnoAlloc <= g_rgfmp[ pfucb->ifmp ].PgnoLast() ) );

    return err;
}

//  if single extent optimization, then try to allocate from
//  root page space map.
//
ERR ErrSPISmallGetPage(
    __inout FUCB *      pfucb,
    _In_    PGNO        pgnoLast,
    _Out_   PGNO *      ppgnoAlloc )
{
    ERR             err = JET_errSuccess;
    SPACE_HEADER    sph;
    UINT            rgbitT;
    PGNO            pgnoAvail;
    DATA            data;
    const FCB * const   pfcb            = pfucb->u.pfcb;

    Assert( NULL != ppgnoAlloc );
    Assert( pgnoNull == *ppgnoAlloc );


    Assert( pfcb->FSpaceInitialized() );
    Assert( FSPIIsSmall( pfcb ) ); //   only should be called on single extent / small space representation.

    AssertSPIPfucbOnRoot( pfucb );

    SPIValidateCpgOwnedAndAvail( pfucb );

    //  if any chance in satisfying request from extent
    //  then try to allcate requested extent, or minimum
    //  extent from first available extent.  Only try to
    //  allocate the minimum request, to facillitate
    //  efficient space usage.
    //
    //  get external header
    //
    NDGetExternalHeader( pfucb, pfucb->pcsrRoot, noderfSpaceHeader );
    Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );

    //  get single extent allocation information
    //
    UtilMemCpy( &sph, pfucb->kdfCurr.data.Pv(), sizeof(sph) );

    const PGNO  pgnoFDP         = PgnoFDP( pfucb );
    const CPG   cpgSingleExtent = min( sph.CpgPrimary(), cpgSmallSpaceAvailMost+1 );    //  +1 because pgnoFDP is not represented in the single extent space map
    Assert( cpgSingleExtent > 0 );
    Assert( cpgSingleExtent <= cpgSmallSpaceAvailMost+1 );

    //  allocate first page
    //
    for( pgnoAvail = pgnoFDP + 1, rgbitT = 1;
        pgnoAvail <= pgnoFDP + cpgSingleExtent - 1;
        pgnoAvail++, rgbitT <<= 1 )
    {
        Assert( rgbitT != 0 );
        if ( rgbitT & sph.RgbitAvail() )
        {
            Assert( ( rgbitT & sph.RgbitAvail() ) == rgbitT );

            //  write latch page before update
            //
            SPIUpgradeToWriteLatch( pfucb );

            sph.SetRgbitAvail( sph.RgbitAvail() ^ rgbitT );
            data.SetPv( &sph );
            data.SetCb( sizeof(sph) );

            CallR( ErrSPIWrappedNDSetExternalHeader(
                        pfucb,
                        pfucb->pcsrRoot,
                        &data,
                        ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                        noderfSpaceHeader,
                        0,
                        -1 ) );

            //  at this point, we should have moved small extents out of the shrink range
            //  if we are shrinking.
            //
            Assert( !g_rgfmp[ pfucb->ifmp ].FBeyondPgnoShrinkTarget( pgnoAvail, 1 ) );

            //  we should also never have small-extent pages beyond EOF (and potentially shelved).
            //
            Assert( pgnoAvail <= g_rgfmp[ pfucb->ifmp ].PgnoLast() );

            //  set output parameter and done
            //
            *ppgnoAlloc = pgnoAvail;
            SPCheckPgnoAllocTrap( *ppgnoAlloc );

            ETSpaceAllocPage( pfucb->ifmp, pgnoFDP, *ppgnoAlloc, pfucb->u.pfcb->ObjidFDP(), TceFromFUCB( pfucb ) );
            OSTraceFMP(
                pfucb->ifmp,
                JET_tracetagSpace,
                OSFormat(
                    "%hs: get page 1 at %lu from [0x%x:0x%x:%lu].",
                    __FUNCTION__,
                    *ppgnoAlloc,
                    pfucb->ifmp,
                    ObjidFDP( pfucb ),
                    PgnoFDP( pfucb ) ) );

            return JET_errSuccess;
        }
    }

    return ErrERRCheck( errSPNoSpaceForYou );
}

//
//  This routine finds a page (or insertion region marker) from the AE tree.
//
//  The routine can match based upon parameters for AvailPool, Layout (findflags+pgnoLast).
//
//  Returns with pfucbAE currency set and pspaeAlloc with the extent's info or an err.
//
//  if pcpgFindInsertionRegionMarker is specified the function will also return success 
//  even if a page is not found, but an insertion region marker (that matches contiguity 
//  requirements) is found instead.  In this case the extentinfo will be blank, but the 
//  pfucbAE currency will be set.
//      Note: Cleanup of insertion markers is caller's responsibility, because I wanted the
//      ErrSPIAEFindPage() API to be read only.
//

enum SPFindFlags {
    fSPFindContinuousPage   = 0x1,
    fSPFindAnyGreaterPage   = 0x2
};

ERR ErrSPIAEFindPage(
    __inout FUCB * const        pfucbAE,
    _In_    const SPFindFlags   fSPFindFlags,
    _In_    const SpacePool     sppAvailPool,
    _In_    const PGNO          pgnoLast,
    _Out_   CSPExtentInfo *     pspaeiAlloc,
    _Out_   CPG *               pcpgFindInsertionRegionMarker
    )
{
    ERR             err             = JET_errSuccess;
    FCB * const     pfcb            = pfucbAE->u.pfcb;
    DIB             dib;

    Assert( pfucbAE );
    Assert( pspaeiAlloc );
    Assert( FFUCBAvailExt( pfucbAE ) );
    Assert( !FSPIIsSmall( pfcb ) );

    pspaeiAlloc->Unset();   // invalidate
    if ( pcpgFindInsertionRegionMarker )
    {
        *pcpgFindInsertionRegionMarker = 0;
    }

    //  Init search w/ current page ...
    CSPExtentKeyBM  spaeFindPage( SPEXTKEY::fSPExtentTypeAE, pgnoLast, sppAvailPool );
    dib.pos = posDown;
    dib.pbm = spaeFindPage.Pbm( pfucbAE );

    //  get node of next contiguous page
    //

    spaeFindPage.SPExtentMakeKeyBM( SPEXTKEY::fSPExtentTypeAE, pgnoLast, sppAvailPool );

    dib.dirflag = fDIRNull;

    err = ErrBTDown( pfucbAE, &dib, latchReadTouch );

    switch ( err )
    {

        default:
            Assert( err < JET_errSuccess );
            Assert( err != JET_errNoCurrentRecord );
            OSTraceFMP(
                pfucbAE->ifmp,
                JET_tracetagSpaceInternal,
                OSFormat(
                    "%hs: could not go down into available extent tree. [0x%x:0x%x:%lu].",
                    __FUNCTION__,
                    pfucbAE->ifmp,
                    ObjidFDP( pfucbAE ),
                    PgnoFDP( pfucbAE ) ) );
            Call( err );
            break;

        case JET_errRecordNotFound:     //  The available extent tree is complete empty.

            //  no node in available extent tree,
            //  get a secondary extent
            //
            Error( ErrERRCheck( errSPNoSpaceForYou ) );
            break;

        case JET_errSuccess:                //  found an exact matching node.

            pspaeiAlloc->Set( pfucbAE );    // fetch the space node/extent

            //  Since we leave markers as cpg = 0 extents in the spp::ContinuousPool
            //  pool, we have to handle this case.  In such cases, we should expect
            //  the extent to be empty and in the spp::ContinuousPool pool.

            //  Extra cautious, if anything looks out of place, lets claim corruption.
            if ( !( fSPFindContinuousPage & fSPFindFlags ) ||
                    ( spp::ContinuousPool != sppAvailPool ) ||
                    ( !pspaeiAlloc->FEmptyExtent() /* i.e. cpg != 0x0 */ ) ||
                    ( pspaeiAlloc->SppPool() != sppAvailPool ) )
            {
                //  page in use is also in AvailExt
                AssertSz( fFalse, "AvailExt corrupted." );
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"bc14a209-3a29-4f60-957b-3e847f0eefd0" );
                Call( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
            }

            Assert( sppAvailPool == spp::ContinuousPool );
            Assert( fSPFindContinuousPage & fSPFindFlags );

            //  We have hit an append or hotpoint insertion region
            //
            {
            CSPExtentInfo spoePreviousExtSize;
            Call( ErrSPIFindExtOE( pfucbAE->ppib, pfcb, pgnoLast, &spoePreviousExtSize ) );
            if ( !spoePreviousExtSize.FContains( pgnoLast ) )
            {
                //  page in not represented in OwnExt ... can not really be sure if 
                //  this is OE or AE corruption, but lets just pick OE.
                AssertSz( fFalse, "OwnExt corrupted." );
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"bfc12437-d215-4768-a6e9-3e534da76285" );
                Call( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
            }

            if ( NULL == pcpgFindInsertionRegionMarker )
            {
                spoePreviousExtSize.Unset();
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            //  Collect the previous extent's size, so caller will know how to size next extent.
            Assert( pcpgFindInsertionRegionMarker );

            *pcpgFindInsertionRegionMarker = spoePreviousExtSize.CpgExtent();
            spoePreviousExtSize.Unset();
            pspaeiAlloc->Unset();
            err = JET_errSuccess;
            goto HandleError;
            }
            break;

        case wrnNDFoundLess:        // space before desired point

            //  if we have a whole bunch of single pages available and use 
            //  space from this code path we actually allocate in backwards 
            //  pgno order here.  This is bad, as disks are generally 
            //  optimized to scan forward.
            //

            pspaeiAlloc->Set( pfucbAE );    // fetch the space node/extent

            if ( pgnoNull != pgnoLast &&
                pspaeiAlloc->SppPool() == sppAvailPool &&
                ( fSPFindFlags & fSPFindAnyGreaterPage ) )
            {
                //  So if this space is in the same pool, just before the current 
                //  space we wanted, and we wanted a page greater than the pgnoLast
                //  then we've failed to find space
                //  Note: Technically hard to know if we'll want all future pools
                //  (except the insertion region pool) to exhibit this behavior, 
                //  but we'll assume so for now.

                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }
            //  else fall through ...


        case wrnNDFoundGreater: // space after desired point
            //  keep locality of reference
            //  get page closest to pgnoLast
            //

            pspaeiAlloc->Set( pfucbAE );    // fetch the space node/extent

            //  Check that the extent found is acceptable in various ways.
            //

            if ( pspaeiAlloc->SppPool() != sppAvailPool )
            {
                //  Found space, but in a pool other than that requested, request more space
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            if ( pspaeiAlloc->FEmptyExtent() )
            {
                //  When asking for continous extents, we should always have an 
                //  exact match ... OR we're establishing a new insertion point! 
                //  When we establish a new insertion point, we insert a new extent 
                //  specifically for the insertion point, and marked the extent 
                //  with the fSPContinuous pool (all in ErrSPIGetSe()).
                Assert( fSPFindContinuousPage & fSPFindFlags ); // shouldn't have an empty extent unless we're on fSPContinuousPool / i.e. w/ fSPContinuous
                Assert( spp::ContinuousPool == pspaeiAlloc->SppPool() );

#ifdef DEPRECATED_ZERO_SIZED_AVAIL_EXTENT_CLEANUP
                if ( pspaeiAlloc->Pool() != spp::ContinuousPool &&
                    0 == pspaeiAlloc->CpgExtent() )
                {
                    //  We might have zero-sized extents if we crashed in ErrSPIAddToAvailExt().
                    //  Simply delete such extents and retry.
                    Call( ErrBTFlagDelete(      // UNDONE: Synchronously remove the node
                                pfucbAE,
                                fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
                    // Zero pages, so no need to call pfucbAE->u.pfcb->AddCpgAE()
                    BTUp( pfucbAE );
                    goto FindPage;
                }
#endif

                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            if ( ( fSPFindContinuousPage & fSPFindFlags ) &&
                    ( pspaeiAlloc->PgnoFirst() != ( pgnoLast + 1 ) ) )
            {
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            //  The current extent violates the shrink constraints.
            if ( g_rgfmp[ pfucbAE->ifmp ].FBeyondPgnoShrinkTarget( pspaeiAlloc->PgnoFirst(), pspaeiAlloc->CpgExtent() ) )
            {
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            //  Check if the current extent is beyond EOF (and potentially shelved).
            if ( pspaeiAlloc->PgnoLast() > g_rgfmp[ pfucbAE->ifmp ].PgnoLast() )
            {
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            break;
    }

    //  Success

    err = JET_errSuccess;

HandleError:
    if ( ( err >= JET_errSuccess ) && pspaeiAlloc->FIsSet() )
    {
        const ERR errT = ErrSPICheckExtentIsAllocatable( pfucbAE->ifmp, pspaeiAlloc->PgnoFirst(), pspaeiAlloc->CpgExtent() );
        if ( errT < JET_errSuccess )
        {
            err = errT;
        }
    }

    if ( err < JET_errSuccess )
    {
        //  Did not find any (appropriate) space.

        pspaeiAlloc->Unset();

        BTUp( pfucbAE );

        Assert( !Pcsr( pfucbAE )->FLatched() );

    }
    else
    {
        //  Success, found appropriate space (or an insertion marker).

        Assert( Pcsr( pfucbAE )->FLatched() );

        if ( NULL == pcpgFindInsertionRegionMarker || 0 == *pcpgFindInsertionRegionMarker )
        {
            //  We should have appropriate space.

            Assert( pspaeiAlloc->SppPool() == sppAvailPool );
            Assert( pspaeiAlloc->PgnoLast() && pspaeiAlloc->PgnoLast() != pgnoBadNews );
            Assert( !pspaeiAlloc->FEmptyExtent() );
        }
        else
        {
            //  We should have an insertion marker.

            Assert( *pcpgFindInsertionRegionMarker );
        }
    }

    return err;
}

ERR ErrSPIAEGetExtentAndPage(
    __inout FUCB * const        pfucb,
    __inout FUCB * const        pfucbAE,
    _In_    const SpacePool     sppAvailPool,
    _In_    const CPG           cpgRequest,
    _In_    const ULONG         fSPFlags,
    _Out_   CSPExtentInfo *     pspaeiAlloc
    )
{
    ERR             err             = JET_errSuccess;

    Assert( cpgRequest );
    Assert( fSPFlags );
    Assert( !Pcsr( pfucbAE )->FLatched() );
    BTUp( pfucbAE );    // protect ourselves just in case

    //  Get a secondary extent according to the paramters.

    Assert( !FSPIParentIsFs( pfucb ) );
    if ( !FSPIParentIsFs( pfucb ) )
    {
        Call( ErrSPIGetSe( pfucb, pfucbAE, cpgRequest, cpgRequest, fSPFlags, sppAvailPool ) );
    }
    else
    {
        Call( ErrSPIGetFsSe( pfucb, pfucbAE, cpgRequest, cpgRequest, fSPFlags ) );
    }

    Assert( Pcsr( pfucbAE )->FLatched() );

    //  Should be set to the extent our parent has given us.

    pspaeiAlloc->Set( pfucbAE );

    //  The added extent should be in the same pool we asked for.

    Assert( pspaeiAlloc->SppPool() == sppAvailPool );

HandleError:

    return err;
}

ERR ErrSPIAEGetContinuousPage(
    __inout FUCB * const        pfucb,  // needed for ErrSPIAEGetExtentAndPage()
    __inout FUCB * const        pfucbAE,
    _In_    const PGNO          pgnoLast,
    _In_    const CPG           cpgReserve,
    _In_    const BOOL          fHardReserve,   // ensure reserve, even if next contiguous page is available.
    _Out_   CSPExtentInfo *     pspaeiAlloc
    )
{
    ERR             err             = JET_errSuccess;
    FCB * const     pfcb            = pfucbAE->u.pfcb;
    CPG             cpgEscalatingRequest = 0;

    Assert( pfucb );
    Assert( pfucbAE );
    Assert( pspaeiAlloc );

    //  Search the continuous pool to find a contiguous page from an insertion region.

    err = ErrSPIAEFindPage( pfucbAE, fSPFindContinuousPage, spp::ContinuousPool, pgnoLast, pspaeiAlloc, &cpgEscalatingRequest );

    //  The cpgEscalatingRequest is filled out if a previous empty insertion region marker was
    //  found when looking for the next contigous page.  Remove it, and recalculate sizing for
    //  next insertion region extent.
    //
    if ( cpgEscalatingRequest )
    {

        //  We should have had success or cpgEscalatingRequest would not be set.
        CallS( err );
        Assert( Pcsr( pfucbAE )->FLatched() );

        //  Now apply growth rules to find size of next append or hotpoint extent.
        //
        cpgEscalatingRequest = CpgSPIGetNextAlloc( pfcb->Pfcbspacehints(), cpgEscalatingRequest );

        //  Delete the insertion region marker.
        //
        err = ErrSPIWrappedBTFlagDelete(      // UNDONE: Synchronously remove the node
                    pfucbAE,
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    0,      // This is just a region marker, not actual pages, so no need to adjust the cache.
                    0 );

        BTUp( pfucbAE );

        Assert( !Pcsr( pfucbAE )->FLatched() );

        Call( err );    // materialize the ErrBTFlagDelete() error ...

        err = ErrERRCheck( errSPNoSpaceForYou );

    }
    else
    {
        cpgEscalatingRequest = 1;
    }

    if ( fHardReserve &&
            err >= JET_errSuccess &&
            pspaeiAlloc->CpgExtent() < cpgReserve )
    {
        pspaeiAlloc->Unset();
        BTUp( pfucbAE );
        Assert( !Pcsr( pfucbAE )->FLatched() );
        err = ErrERRCheck( errSPNoSpaceForYou );
    }

    if ( errSPNoSpaceForYou == err )
    {

        if ( cpgEscalatingRequest == 1 )
        {
            if ( cpgReserve )
            {
                cpgEscalatingRequest += cpgReserve;
            }
            else
            {
                cpgEscalatingRequest = CpgSPIGetNextAlloc( pfcb->Pfcbspacehints(), cpgEscalatingRequest );
            }
        }

        if ( fHardReserve &&
                cpgReserve != 0 &&
                cpgReserve != cpgEscalatingRequest )
        {
            const CPG cpgFullReserveRequest = ( cpgReserve + 1 );
            //  Made this strict, but we could entertain that the request is only 10% wastage or something.
            if ( ( cpgEscalatingRequest % cpgFullReserveRequest ) != 0 )
            {
                cpgEscalatingRequest = cpgFullReserveRequest;
            }
        }

        //  no node (at least a satisfactory one) in available extent tree, get a
        //  secondary extent
        //

        //  Note: we don't want ErrSPIGetSe() to resize our request, we've already decided 
        //  on a good size, so don't pass fSPOriginatingRequest.
        Call( ErrSPIAEGetExtentAndPage(
                  pfucb,
                  pfucbAE,
                  spp::ContinuousPool,
                  cpgEscalatingRequest,
                  fSPSplitting,
                  pspaeiAlloc ) );

    }

    //  We should have succeeded or have latched some space
    //
    CallS( err );
    Call( err );

    Assert( Pcsr( pfucbAE )->FLatched() );
    Assert( pspaeiAlloc->SppPool() == spp::ContinuousPool );
    Assert( pspaeiAlloc->PgnoFirst() && pspaeiAlloc->PgnoFirst() != pgnoBadNews );
    Assert( !pspaeiAlloc->FEmptyExtent() );

HandleError:

    Assert( JET_errSuccess == err || !Pcsr( pfucbAE )->FLatched() );
    Assert( err < JET_errSuccess || Pcsr( pfucbAE )->FLatched() );

    return err;
}

//  This is an allocation for a regular page, which could be an append page for a regular table that does
//  not have appropriate "space hints" declared, or could be a random page split, or any other spurious 
//  usage.

ERR ErrSPIAEGetAnyPage(
    __inout FUCB * const        pfucb,  // needed for ErrSPIAEGetExtentAndPage()
    __inout FUCB * const        pfucbAE,
    _In_    const PGNO          pgnoLastHint,
    _In_    const BOOL          fSPAllocFlags,
    _Out_   CSPExtentInfo *     pspaeiAlloc
    )
{
    ERR             err             = JET_errSuccess;

    Assert( pfucb );
    Assert( pfucbAE );
    Assert( pspaeiAlloc );

    //  First we will at least try to find a contiguous page
    //
    err = ErrSPIAEFindPage( pfucbAE, fSPFindContinuousPage, spp::ContinuousPool, pgnoLastHint, pspaeiAlloc, NULL );

    if ( errSPNoSpaceForYou == err )
    {
        //  First we will at least try to find a contiguous or greater page anyway
        //
        err = ErrSPIAEFindPage( pfucbAE, fSPFindAnyGreaterPage, spp::AvailExtLegacyGeneralPool, pgnoLastHint, pspaeiAlloc, NULL );

        if ( errSPNoSpaceForYou == err )
        {
            //  If that fails, any random page will do (greater than 0)
            //
            err = ErrSPIAEFindPage( pfucbAE, fSPFindAnyGreaterPage, spp::AvailExtLegacyGeneralPool, pgnoNull, pspaeiAlloc, NULL );
        }
    }

    if ( errSPNoSpaceForYou == err )
    {
        //  no node (at least a satisfactory one) in available extent tree, get a
        //  secondary extent
        //

        Call( ErrSPIAEGetExtentAndPage( pfucb, pfucbAE,
                            spp::AvailExtLegacyGeneralPool,
                            1,
                            fSPAllocFlags | fSPSplitting | fSPOriginatingRequest,
                            pspaeiAlloc ) );

        Assert( Pcsr( pfucbAE )->FLatched() );
    }

    Call( err );

    Assert( Pcsr( pfucbAE )->FLatched() );
    Assert( pspaeiAlloc->PgnoFirst() && pspaeiAlloc->PgnoFirst() != pgnoBadNews );
    Assert( !pspaeiAlloc->FEmptyExtent() );

HandleError:

    Assert( JET_errSuccess == err || !Pcsr( pfucbAE )->FLatched() );
    Assert( err < JET_errSuccess || Pcsr( pfucbAE )->FLatched() );

    return err;
}


ERR ErrSPIAEGetPage(
    __inout FUCB *      pfucb,
    _In_    PGNO        pgnoLast,
    __inout PGNO *      ppgnoAlloc,
    _In_    const BOOL  fSPAllocFlags,
    _In_    const CPG   cpgReserve
    )
{
    ERR             err             = JET_errSuccess;
    FCB * const     pfcb            = pfucb->u.pfcb;
    FUCB *          pfucbAE         = pfucbNil;
    CSPExtentInfo   cspaeiAlloc;

    Assert( 0 == ( fSPAllocFlags & ~( fSPContinuous | fSPUseActiveReserve | fSPExactExtent ) ) );

    //  open cursor on available extent tree
    //
    AssertSPIPfucbOnRoot( pfucb );

    //
    // Make sure the values are correct before we start
    //
    SPIValidateCpgOwnedAndAvail( pfucb );

    CallR( ErrSPIOpenAvailExt( pfucb, &pfucbAE ) );
    Assert( pfcb == pfucbAE->u.pfcb );
    Assert( pfucb->ppib == pfucbAE->ppib );

    if ( fSPContinuous & fSPAllocFlags )
    {
        Call( ErrSPIAEGetContinuousPage( pfucb, pfucbAE, pgnoLast, cpgReserve, fSPAllocFlags & fSPUseActiveReserve, &cspaeiAlloc ) );
    }
    else
    {
        Call( ErrSPIAEGetAnyPage( pfucb, pfucbAE, pgnoLast, fSPAllocFlags & fSPExactExtent, &cspaeiAlloc ) );
    }

    Assert( err >= 0 );
    Assert( Pcsr( pfucbAE )->FLatched() );

    err = cspaeiAlloc.ErrCheckCorrupted();
    CallS( err );
    if ( err < JET_errSuccess )
    {
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"4fbc1735-8688-40ae-898f-40e86deb52c4" );
    }
    Call( err );
    Assert( cspaeiAlloc.PgnoFirst() && cspaeiAlloc.PgnoFirst() != pgnoBadNews );
    Assert( !cspaeiAlloc.FEmptyExtent() );
    Assert( cspaeiAlloc.CpgExtent() > 0 );

    //  allocate first page in node and return code
    //

    if ( cspaeiAlloc.FContains( pgnoLast ) )
    {
        //  page in use is also in AvailExt
        AssertSz( fFalse, "AvailExt corrupted." );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"ad23b08a-84d6-4d21-aefc-746560b0eceb" );
        Call( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
    }

    *ppgnoAlloc = cspaeiAlloc.PgnoFirst();
    SPCheckPgnoAllocTrap( *ppgnoAlloc );

    //  do not return the same page
    //
    Assert( *ppgnoAlloc != pgnoLast );

    //  should be from same / requested pool.

    {
    CSPExtentNodeKDF        spAdjustedAvail( SPEXTKEY::fSPExtentTypeAE,
                                                cspaeiAlloc.PgnoLast(),
                                                cspaeiAlloc.CpgExtent(),
                                                cspaeiAlloc.SppPool() );

    Call( spAdjustedAvail.ErrConsumeSpace( cspaeiAlloc.PgnoFirst() ) );

    if ( spAdjustedAvail.FDelete() )
    {
        Call( ErrSPIWrappedBTFlagDelete(      // UNDONE: Synchronously remove the node
                    pfucbAE,
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    0,
                    -1 ) );
    }
    else
    {
        Assert( FKeysEqual( pfucbAE->kdfCurr.key, spAdjustedAvail.GetKey() ) );

        Call( ErrSPIWrappedBTReplace(
                    pfucbAE,
                    spAdjustedAvail.GetData( ),
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    0,
                    -1 ) );
    }
    }

    BTUp( pfucbAE );
    err = JET_errSuccess;

    //
    // Make sure the values are correct after we're done
    //
    SPIValidateCpgOwnedAndAvail( pfucb );

HandleError:

    if ( pfucbAE != pfucbNil )
    {
        BTClose( pfucbAE );
    }

    return err;
}

CPG CpgSPIConsumeActiveSpaceRequestReserve( FUCB * const pfucb )
{
    Assert( !FFUCBSpace( pfucb ) );

    //  Get the space from FUCB, and set that we've "consumed" the space there.

    const CPG cpgAddlReserve = CpgDIRActiveSpaceRequestReserve( pfucb );
    DIRSetActiveSpaceRequestReserve( pfucb, cpgDIRReserveConsumed );
    Assert( CpgDIRActiveSpaceRequestReserve( pfucb ) == cpgDIRReserveConsumed );

    //  This Assert going off implies that ISAM managed to try to allocate more than one leaf page in
    //  some DIR/BT operation before properly either Re-setting a new active reserve or resetting the
    //  active reserve to zero.
    //  See additional DIRSetActiveSpaceRequestReserve() function comments.
    AssertSz( cpgAddlReserve != cpgDIRReserveConsumed, "The active space reserve should only be consumed one time." );

    return cpgAddlReserve;
}


//  ErrSPGetPage
//  ========================================================================
//  ERR ErrSPGetPage( FUCB *pfucb, PGNO *ppgnoLast )
//
//  Allocates page from FUCB cache. If cache is nil,
//  allocate from available extents. If available extent tree is empty,
//  a secondary extent is allocated from the parent FDP to
//  satisfy the page request. A page closest to
//  *ppgnoLast is allocated. If *ppgnoLast is pgnoNull,
//  first free page is allocated
//
//  PARAMETERS
//      pfucb       FUCB providing FDP page number and process identifier block
//                  cursor should be on root page RIW latched
//      ppgnoLast   may contain page number of last allocated page on
//                  input, on output contains the page number of the allocated page
//      fForDefrag  The page is being allocated for B-tree defragmentation. This mean
//                  that if the next page cannot be found a new secondary extent should
//                  be used.
//
//-
ERR ErrSPGetPage(
    __inout FUCB *          pfucb,
    _In_    const PGNO      pgnoLast,
    _In_    const ULONG     fSPAllocFlags,
    _Out_   PGNO *          ppgnoAlloc
    )
{
    ERR         err = errCodeInconsistency;
    FCB         * const pfcb            = pfucb->u.pfcb;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortSpace );

    //  check for valid input
    //
    Assert( ppgnoAlloc != NULL );
    Assert( 0 == ( fSPAllocFlags & ~fMaskSPGetPage ) );

    CPG         cpgAddlReserve = 0;
    if ( fSPAllocFlags & fSPNewExtent )
    {
        cpgAddlReserve = 15;
    }

    if ( ( fSPAllocFlags & fSPContinuous ) && ( fSPAllocFlags & fSPUseActiveReserve ) )
    {
        cpgAddlReserve = CpgSPIConsumeActiveSpaceRequestReserve( pfucb );
    }

    //  check FUCB work area for active extent and allocate first available
    //  page of active extent
    //
    if ( FFUCBSpace( pfucb ) )
    {
        err = ErrSPISPGetPage( pfucb, ppgnoAlloc );
    }
    else
    {
        //  check for valid input when alocating page from FDP
        //
#ifdef SPACECHECK
        Assert( !( ErrSPIValidFDP( pfucb->ppib, pfucb->ifmp, PgnoFDP( pfucb ) ) < 0 ) );
        if ( pgnoNull != pgnoLast )
        {
            CallS( ErrSPIWasAlloc( pfucb, pgnoLast, (CPG)1 ) );
        }
#endif

        if ( !pfcb->FSpaceInitialized() )
        {
            SPIInitFCB( pfucb, fTrue );
        }

        //
        // Make sure the values are correct before we start
        //
        SPIValidateCpgOwnedAndAvail( pfucb );

        //  if single extent optimization, then try to allocate from
        //  root page space map.  If cannot satisfy allocation, convert
        //  to multiple extent representation.
        //
        if ( FSPIIsSmall( pfcb ) )
        {
            err = ErrSPISmallGetPage( pfucb, pgnoLast, ppgnoAlloc );
            if ( errSPNoSpaceForYou != err )
            {
                Assert( FSPExpectedError( err ) );
                goto HandleError;
            }

            //  Cannot satisfy allocation, this tree will be converted 
            //  to multiple extent representation.
            Call( ErrSPIConvertToMultipleExtent( pfucb, 1, 1 ) );
        }

        //  Get page from AE Tree
        //
        Call( ErrSPIAEGetPage(
                    pfucb,
                    pgnoLast,
                    ppgnoAlloc,
                    fSPAllocFlags & ( fSPContinuous | fSPUseActiveReserve | fSPExactExtent ),
                    cpgAddlReserve ) );
    }

HandleError:

    //
    // Make sure the values are correct after we're done
    //
    SPIValidateCpgOwnedAndAvail( pfucb );

    if ( err < JET_errSuccess )
    {
        OSTraceFMP(
            pfucb->ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Failed to get page from [0x%x:0x%x:%lu].",
                __FUNCTION__,
                pfucb->ifmp,
                ObjidFDP( pfucb ),
                PgnoFDP( pfucb ) ) );

        if ( errSPNoSpaceForYou == err ||
                errCodeInconsistency == err )
        {
            AssertSz( fFalse, "Code malfunction." );
            err = ErrERRCheck( JET_errInternalError );
        }
    }
    else
    {
        Assert( FSPValidAllocPGNO( *ppgnoAlloc ) );

        const PGNO pgnoFDP = PgnoFDP( pfucb );
        ETSpaceAllocPage( pfucb->ifmp, pgnoFDP, *ppgnoAlloc, pfucb->u.pfcb->ObjidFDP(), TceFromFUCB( pfucb ) );
        OSTraceFMP(
            pfucb->ifmp,
            JET_tracetagSpace,
            OSFormat(
                "%hs: get page 1 at %lu from [0x%x:0x%x:%lu].",
                __FUNCTION__,
                *ppgnoAlloc,
                pfucb->ifmp,
                ObjidFDP( pfucb ),
                PgnoFDP( pfucb ) ) );
    }

    return err;
}


LOCAL ERR ErrSPIFreeSEToParent(
    FUCB        *pfucb,
    FUCB        *pfucbOE,
    FUCB        *pfucbAE,
    FUCB* const pfucbParent,
    const PGNO  pgnoLast,
    const CPG   cpgSize )
{
    ERR         err;
    FCB         *pfcb = pfucb->u.pfcb;
    FCB         *pfcbParent;
    BOOL        fState;
    DIB         dib;
    FUCB        *pfucbParentLocal = pfucbParent;
    const CSPExtentKeyBM    CSPOEKey( SPEXTKEY::fSPExtentTypeOE, pgnoLast );

    Assert( pfcbNil != pfcb );
    AssertSPIPfucbOnRoot( pfucb );
    AssertSPIPfucbNullOrUnlatched( pfucbParent );

    // Can't do this because pfucbAE and pfucbOE have their root pages locked, and
    // we would need to BTDown() in those trees deeper in the stack.
    // SPIValidateCpgOwnedAndAvail( pfucb );

    //  get parentFDP's root pgno
    //  cursor passed in should be at root of tree
    //  so we can access pgnoParentFDP from the external header
    const PGNO  pgnoParentFDP = PgnoSPIParentFDP( pfucb );
    if ( pgnoParentFDP == pgnoNull )
    {
        // This is the root DB and its parent is the file system, so there's nothing to release
        // to the parent in this case. We could in theory either sparsify/trim the extent or
        // shrink the file if it's the last extent, but those are done separately via other
        // mechanisms.
        Assert( pfucb->u.pfcb->PgnoFDP() == pgnoSystemRoot );
        return JET_errSuccess;
    }

    //  parent must always be in memory
    //
    pfcbParent = FCB::PfcbFCBGet( pfucb->ifmp, pgnoParentFDP, &fState, fTrue, fTrue );
    Assert( pfcbParent != pfcbNil );
    Assert( fFCBStateInitialized == fState );
    Assert( !pfcb->FTypeNull() );

    if ( !pfcb->FInitedForRecovery() )
    {
        if ( pfcb->FTypeSecondaryIndex() || pfcb->FTypeLV() )
        {
            Assert( pfcbParent->FTypeTable() );
        }
        else
        {
            Assert( pfcbParent->FTypeDatabase() );
            Assert( !pfcbParent->FDeletePending() );
            Assert( !pfcbParent->FDeleteCommitted() );
        }
    }

    // Callers may pass in a null pfucbAE in case it already took care of deleting the AE nodes.
    // Currently, that is ErrSPTryCoalesceAndFreeAvailExt().
    if ( pfucbAE != pfucbNil )
    {

        //  delete available extent node
        //
        Assert( pfucb->u.pfcb == pfucbAE->u.pfcb );
        Call( ErrSPIWrappedBTFlagDelete(      // UNDONE: Synchronously remove the node
                    pfucbAE,
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    0,
                    -( cpgSize ) ) );

        BTUp( pfucbAE );
    }

    //  seek to owned extent node and delete it
    //
    dib.pos = posDown;
    dib.pbm = CSPOEKey.Pbm( pfucbOE );
    dib.dirflag = fDIRNull;
    Call( ErrBTDown( pfucbOE, &dib, latchReadTouch ) );

    Assert( pfucb->u.pfcb == pfucbOE->u.pfcb );
    Call( ErrSPIWrappedBTFlagDelete(      // UNDONE: Synchronously remove the node
                pfucbOE,
                fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                -( cpgSize ),
                0 ) );

    BTUp( pfucbOE );

    //  free extent to parent FDP
    //
    if ( pfucbParentLocal == pfucbNil )
    {
        Call( ErrBTIOpenAndGotoRoot( pfucb->ppib, pgnoParentFDP, pfucb->ifmp, &pfucbParentLocal ) );
    }
    else
    {
        Call( ErrBTIGotoRoot( pfucbParentLocal, latchRIW ) );
        pfucbParentLocal->pcsrRoot = Pcsr( pfucbParentLocal );
    }

    err = ErrSPFreeExt( pfucbParentLocal, pgnoLast - cpgSize + 1, cpgSize, "FreeToParent" );

    pfucbParentLocal->pcsrRoot->ReleasePage();
    pfucbParentLocal->pcsrRoot = pcsrNil;
    Call( err );

    //  count pages freed by a table
    //
    if ( pgnoParentFDP == pgnoSystemRoot )
    {
        TLS* const ptls = Ptls();
        ptls->threadstats.cPageTableReleased += cpgSize;
    }

HandleError:
    AssertSPIPfucbOnRoot( pfucb );
    AssertSPIPfucbNullOrUnlatched( pfucbParent );

    if ( ( pfucbParentLocal != pfucbNil ) && ( pfucbParentLocal != pfucbParent ) )
    {
        Expected( pfucbParent == pfucbNil );
        if ( pcsrNil != pfucbParentLocal->pcsrRoot )
        {
            Expected( fFalse );
            pfucbParentLocal->pcsrRoot->ReleasePage();
            pfucbParentLocal->pcsrRoot = pcsrNil;
        }
        BTClose( pfucbParentLocal );
        pfucbParentLocal = pfucbNil;
    }

    pfcbParent->Release();

    return err;
}

LOCAL_BROKEN VOID SPIReportLostPages(
    const IFMP  ifmp,
    const OBJID objidFDP,
    const PGNO  pgnoLast,
    const CPG   cpgLost )
{
    WCHAR       wszStartPagesLost[16];
    WCHAR       wszEndPagesLost[16];
    WCHAR       wszObjidFDP[16];
    const WCHAR *rgcwszT[4];

    OSStrCbFormatW( wszStartPagesLost, sizeof(wszStartPagesLost), L"%d", pgnoLast - cpgLost + 1 );
    OSStrCbFormatW( wszEndPagesLost, sizeof(wszEndPagesLost), L"%d", pgnoLast );
    OSStrCbFormatW( wszObjidFDP, sizeof(wszObjidFDP), L"%d", objidFDP );

    rgcwszT[0] = g_rgfmp[ifmp].WszDatabaseName();
    rgcwszT[1] = wszStartPagesLost;
    rgcwszT[2] = wszEndPagesLost;
    rgcwszT[3] = wszObjidFDP;

    UtilReportEvent(
            eventWarning,
            SPACE_MANAGER_CATEGORY,
            SPACE_LOST_ON_FREE_ID,
            4,
            rgcwszT,
            0,
            NULL,
            PinstFromIfmp( ifmp ) );
}

ERR ErrSPISPFreeExt( __inout FUCB * pfucb, _In_ const PGNO pgnoFirst, _In_ const CPG cpgSize )
{
    ERR         err         = JET_errSuccess;
    const BOOL  fAvailExt   = FFUCBAvailExt( pfucb );

    //  must be returning space due to split failure
    Assert( 1 == cpgSize );

    if ( NULL == pfucb->u.pfcb->Psplitbuf( fAvailExt ) )
    {
        CSR             *pcsrRoot   = pfucb->pcsrRoot;
        SPLIT_BUFFER    spbuf;
        DATA            data;

        AssertSPIPfucbOnSpaceTreeRoot( pfucb, pcsrRoot );

        UtilMemCpy( &spbuf, PspbufSPISpaceTreeRootPage( pfucb, pcsrRoot ), sizeof(SPLIT_BUFFER) );

        spbuf.ReturnPage( pgnoFirst );

        Assert( latchRIW == pcsrRoot->Latch() );
        pcsrRoot->UpgradeFromRIWLatch();

        data.SetPv( &spbuf );
        data.SetCb( sizeof(spbuf) );
        AssertSz( fFalse, "SOMEONE: not validated for space update." );
        err = ErrSPIWrappedNDSetExternalHeader(
                    pfucb,
                    pcsrRoot,
                    &data,
                    ( pfucb->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    noderfWhole );

        //  reset to RIW latch
        pcsrRoot->Downgrade( latchRIW );
    }
    else
    {
        AssertSPIPfucbOnSpaceTreeRoot( pfucb, pfucb->pcsrRoot );
        pfucb->u.pfcb->Psplitbuf( fAvailExt )->ReturnPage( pgnoFirst );
        err = JET_errSuccess;
    }

    return err;
}

ERR ErrSPISmallFreeExt( __inout FUCB * pfucb, _In_ const PGNO pgnoFirst, _In_ const CPG cpgSize )
{
    ERR             err = JET_errSuccess;
    SPACE_HEADER    sph;
    UINT            rgbitT;
    INT             iT;
    DATA            data;

    AssertSPIPfucbOnRoot( pfucb );

    Assert( cpgSize <= cpgSmallSpaceAvailMost );
    Assert( FSPIIsSmall( pfucb->u.pfcb ) );

    Assert( pgnoFirst > PgnoFDP( pfucb ) );                             //  can't be equal, because then you'd be freeing root page to itself
    Assert( pgnoFirst - PgnoFDP( pfucb ) <= cpgSmallSpaceAvailMost );   //  extent must start and end within single-extent range
    Assert( ( pgnoFirst + cpgSize - 1 - PgnoFDP( pfucb ) ) <= cpgSmallSpaceAvailMost );
    Assert( ( pgnoFirst + cpgSize - 1 ) <= g_rgfmp[ pfucb->ifmp ].PgnoLast() );

    //  write latch page before update
    //
    if ( latchWrite != pfucb->pcsrRoot->Latch() )
    {
        SPIUpgradeToWriteLatch( pfucb );
    }

    //  get external header
    //
    NDGetExternalHeader( pfucb, pfucb->pcsrRoot, noderfSpaceHeader );
    Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );

    //  get single extent allocation information
    //
    UtilMemCpy( &sph, pfucb->kdfCurr.data.Pv(), sizeof(sph) );

    //  make mask for extent to free
    //
    for ( rgbitT = 1, iT = 1; iT < cpgSize; iT++ )
    {
        rgbitT = ( rgbitT << 1 ) + 1;
    }
    rgbitT <<= ( pgnoFirst - PgnoFDP( pfucb ) - 1 );
    sph.SetRgbitAvail( sph.RgbitAvail() | rgbitT );
    data.SetPv( &sph );
    data.SetCb( sizeof(sph) );

    Call( ErrSPIWrappedNDSetExternalHeader(
                pfucb,
                pfucb->pcsrRoot,
                &data,
                ( pfucb->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                noderfSpaceHeader,
                0,
                cpgSize ) );

HandleError:

    return err;
}


//
//  ErrSPIAERemoveInsertionRegion
//
//      This function takes a pgnoFirstToFree, *pcpgSizeToFree, and containing OE 
//      extent and determines if there are any insertion regions that need to be 
//      removed as a result of the freeing of this page.
//  
//  Notes / Side Effects:
//
//      Currency on pfucbAE is ruined.  Should come in unset as well.
//
//      Note: this function is pgnoFirst based ... meaning it takes in a pgnoFirst,
//      and since it can add space to the extent by consolidating a former insertion
//      region, it takes and in / out pcpgSizeToFree.
//
//  Return Values:
//
//      JET_errSuccess
//
//          No insertion regions found, nothing done.
//              or
//          Insertion region (marker only) found and deleted.  No additional action needed.
//
//      wrnSPReservedPages  // note canablizing this error, b/c couldn't happen here.
//
//          Non-empty insertion region found and deleted.  pcpgSizeToFree updated.  Re-coallesce required.
//
//      <Other>
//
//          Fatal error, out of log space, disk write failure, etc.
//
ERR ErrSPIAERemoveInsertionRegion(
    _In_    const FCB * const               pfcb,
    __inout FUCB * const                    pfucbAE,
    _In_    const CSPExtentInfo * const     pcspoeContaining,
    _In_    const PGNO                      pgnoFirstToFree,
    __inout CPG * const                     pcpgSizeToFree
    )
{
    ERR                 err = JET_errSuccess;
    DIB                 dibAE;
    CSPExtentKeyBM      cspextkeyAE;
    CSPExtentInfo       cspaei;

    PGNO                pgnoLast = pgnoFirstToFree + (*pcpgSizeToFree) - 1;

    //  should have no currency.
    Assert( !Pcsr( pfucbAE )->FLatched() );

    const SpacePool sppAvailContinuousPool = spp::ContinuousPool;

    cspextkeyAE.SPExtentMakeKeyBM( SPEXTKEY::fSPExtentTypeAE, pgnoFirstToFree - 1, sppAvailContinuousPool );
    Assert( cspextkeyAE.GetBm( pfucbAE ).data.Pv() == NULL );
    Assert( cspextkeyAE.GetBm( pfucbAE ).data.Cb() == 0 );
    dibAE.pos = posDown;
    dibAE.pbm = cspextkeyAE.Pbm( pfucbAE );
    dibAE.dirflag = fDIRFavourNext;

    //  Attempt to find an insertion region.
    //
    err = ErrBTDown( pfucbAE, &dibAE, latchReadTouch );
    Assert( err != JET_errNoCurrentRecord );

    if ( JET_errRecordNotFound == err )
    {
        //  Avail tree empty, no insertion regions to remove
        err = JET_errSuccess;
        goto HandleError;
    }
    if ( err < 0 )
    {
        OSTraceFMP(
            pfucbAE->ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: could not go down into nonempty available extent tree. [0x%x:0x%x:%lu].",
                __FUNCTION__,
                pfucbAE->ifmp,
                ObjidFDP( pfucbAE ),
                PgnoFDP( pfucbAE ) ) );
    }
    Call( err );

    BOOL    fOnNextExtent   = fFalse;

    //  found an available extent node ... what is it.
    //
    cspaei.Set( pfucbAE );

    //  Can not Call() b/c we consume err below ...
    ERR errT = cspaei.ErrCheckCorrupted();
    if( errT < JET_errSuccess )
    {
        CallS( errT );  // complain about it.
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"af6f9c2c-efa4-4c9e-bb1d-0cf605697457" );
        Call( errT );
    }

    if ( cspaei.SppPool() != sppAvailContinuousPool )
    {
        //  we've wandered into a different pool, which means we didn't find an 
        //  insertion region.
        //
        ExpectedSz( ( cspaei.SppPool() == spp::AvailExtLegacyGeneralPool ) ||
                    ( pfcb->PgnoFDP() == pgnoSystemRoot ), "We only have a next pool for the DB root today." );
        err = JET_errSuccess;
        goto HandleError;
    }

    if( !cspaei.FEmptyExtent() )
    {
        //  assert no page is common between available extent node
        //  and freed extent
        //
        Assert( pgnoFirstToFree > cspaei.PgnoLast() ||
                pgnoLast < cspaei.PgnoFirst() );
    }

    if ( wrnNDFoundGreater == err )
    {
        Assert( cspaei.PgnoLast() > pgnoFirstToFree - 1 );

        //  already on the next node, no need to move there
        fOnNextExtent = fTrue;
    }
    else
    {
        //  available extent nodes last page < pgnoFirstToFree - 1
        //  no possible coalescing on the left
        //  (this is the last node in available extent tree)
        //
        Assert( wrnNDFoundLess == err || JET_errSuccess == err );

        //Assert( cspaei.PgnoLast() < pgnoFirstToFree - 1 );    // wrnNDFoundLess
        //Assert( cspaei.PgnoLast() == pgnoFirstToFree - 1 );   // JET_errSuccess
        Assert( cspaei.PgnoLast() <= pgnoFirstToFree - 1 );

        cspaei.Unset();
        err = ErrBTNext( pfucbAE, fDIRNull );
        if ( err < 0 )
        {
            if ( JET_errNoCurrentRecord == err )
            {
                err = JET_errSuccess;
                goto HandleError;
            }
            Call( err );
            CallS( err );
        }
        else
        {
            //  successfully moved to next extent,
            cspaei.Set( pfucbAE );

            //  these 3 checks duplicated from above, better to consolidate...
            errT = cspaei.ErrCheckCorrupted();
            if( errT < JET_errSuccess )
            {
                CallS( errT );  // complain about it.
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"30d82277-2e77-4742-af4d-1d1808205ba6" );
                Call( errT );
            }
            if ( cspaei.SppPool() != sppAvailContinuousPool )
            {
                //  we've wandered into the next pool, which means we didn't find an 
                //  insertion region.
                //
                ExpectedSz( ( cspaei.SppPool() == spp::AvailExtLegacyGeneralPool ) ||
                            ( pfcb->PgnoFDP() == pgnoSystemRoot ), "We only have a next pool for the DB root today." );
                err = JET_errSuccess;
                goto HandleError;
            }

            if( !cspaei.FEmptyExtent() )
            {
                //  assert no page is common between available extent node
                //  and freed extent
                //
                Assert( pgnoFirstToFree > cspaei.PgnoLast() ||
                        pgnoLast < cspaei.PgnoFirst() );
            }
            fOnNextExtent = fTrue;
        }
    }

    //  now see if we can coalesce with next node, first moving there if necessary
    //
    Assert( cspaei.FValidExtent() );
    Assert( fOnNextExtent );
    Assert( cspaei.SppPool() == sppAvailContinuousPool );

    //  verify no page is common between available extent node
    //  and freed extent
    //
    if ( !cspaei.FEmptyExtent() &&
            pgnoLast >= cspaei.PgnoFirst() )
    {
        AssertSz( fFalse, "AvailExt corrupted." );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"d02a83ff-28b7-4523-8dee-c862ca33cf7e" );
        Call( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
    }

    //  There are two basic cases ...
    BOOL    fDeleteInsertionRegion = fFalse;
    if ( cspaei.FEmptyExtent() )
    {
        //  A pure insertion region "marker" (meaning no avail space).

        //
        if ( pgnoLast == cspaei.PgnoMarker() &&
             cspaei.PgnoLast() <= pcspoeContaining->PgnoLast() )
        {
            //  
            fDeleteInsertionRegion = fTrue;
        }
    }
    else
    {
        //  An insertion region free space / avail extent node.

        Assert( cspaei.FValidExtent() );
        Assert( cspaei.CpgExtent() != 0 );
        if ( pgnoLast >= cspaei.PgnoFirst() )
        {
            AssertSz( fFalse, "AvailExt corrupted." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"ba12baa3-cd1e-4726-9e9b-94bf34d99a14" );
            Call( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }

        if ( pgnoLast == cspaei.PgnoLast() - cspaei.CpgExtent() &&
             cspaei.PgnoLast() <= pcspoeContaining->PgnoLast() )
        {
            //  freed extent falls exactly in front of available extent node
            //          -- coalesce freed extent with current node
            //
            //  the second condition ensures that we do not coalesce
            //      two available extent nodes that are from different
            //      owned extent nodes
            fDeleteInsertionRegion = fTrue;
        }

    }

    if ( fDeleteInsertionRegion )
    {
        //  freed extent falls exactly in front of available extent node
        //          -- coalesce freed extent with current node
        //
        //  the second condition ensures that we do not coalesce
        //      two available extent nodes that are from different
        //      owned extent nodes
        //

        // We increase the size if this is more than a marker ...

        if ( !cspaei.FEmptyExtent() )
        {
            //  The extent is not empty ... add this to the free size.

            Assert( cspaei.PgnoLast() <= pcspoeContaining->PgnoLast() );

            *pcpgSizeToFree = *pcpgSizeToFree + cspaei.CpgExtent();

        }

        Call( ErrSPIWrappedBTFlagDelete(      // UNDONE: Synchronously remove the node
                    pfucbAE,
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    0,
                    -( cspaei.CpgExtent() ) ) );

        Assert( Pcsr( pfucbAE )->FLatched() );

        err = cspaei.FEmptyExtent() ? JET_errSuccess : wrnSPReservedPages;

    }
    else
    {
        //  Not matching criteria, no insertion region to remove, success.
        //
        err = JET_errSuccess;
    }

HandleError:

    BTUp( pfucbAE );

    return err;
}

LOCAL VOID SPIReportSpaceLeak( _In_ const FUCB* const pfucb, _In_ const ERR err, _In_ const PGNO pgnoFirst, _In_ const CPG cpg, __in_z const CHAR* const szTag )
{
    Assert( pfucb != NULL );
    Expected( err < JET_errSuccess );
    Assert( pgnoFirst != pgnoNull );
    Assert( cpg > 0 );

    const PGNO pgnoLast = pgnoFirst + cpg - 1;
    const OBJID objidFDP = pfucb->u.pfcb->ObjidFDP();
    const BOOL fDbRoot = ( pfucb->u.pfcb->PgnoFDP() == pgnoSystemRoot );

    OSTraceSuspendGC();
    const WCHAR* rgcwsz[] =
    {
        g_rgfmp[ pfucb->ifmp ].WszDatabaseName(),
        OSFormatW( L"%I32d", err ),
        OSFormatW( L"%I32u", objidFDP ),
        OSFormatW( L"%I32d", cpg ),
        OSFormatW( L"%I32u", pgnoFirst ),
        OSFormatW( L"%I32u", pgnoLast ),
        OSFormatW( L"%hs", szTag ),
    };
    UtilReportEvent(
        fDbRoot ? eventError : eventWarning,
        GENERAL_CATEGORY,
        fDbRoot ? DATABASE_LEAKED_ROOT_SPACE_ID : DATABASE_LEAKED_NON_ROOT_SPACE_ID,
        _countof( rgcwsz ),
        rgcwsz,
        0,
        NULL,
        g_rgfmp[ pfucb->ifmp ].Pinst() );
    OSTraceResumeGC();
}

LOCAL ERR ErrSPIAEFreeExt(
    __inout FUCB * pfucb,
    _In_ PGNO pgnoFirst,
    _In_ CPG cpgSize,
    _In_ FUCB * const pfucbParent = pfucbNil )
{
    ERR         err                 = errCodeInconsistency;
    FCB         * const pfcb        = pfucb->u.pfcb;
    PGNO        pgnoLast            = pgnoFirst + cpgSize - 1;
    BOOL        fCoalesced          = fFalse;
    CPG         cpgSizeAdj          = 0;

    Assert( !FSPIIsSmall( pfcb ) );
    Assert( cpgSize > 0 );

    // FDP available extent and owned extent operation variables
    //
    CSPExtentInfo           cspoeContaining;

    CSPExtentInfo           cspaei;
    CSPExtentKeyBM          cspextkeyAE;
    DIB         dibAE;

    // owned extent and avail extent variables
    //
    FUCB        *pfucbAE        = pfucbNil;
    FUCB        *pfucbOE        = pfucbNil;

    //  We always free to this pool
    //
    const SpacePool sppAvailGeneralPool = spp::AvailExtLegacyGeneralPool;

    #ifdef DEBUG
    BOOL    fWholeKitAndKabudle = fFalse;
    #endif

    AssertSPIPfucbOnRoot( pfucb );
    AssertSPIPfucbNullOrUnlatched( pfucbParent );

    //  If this is the DB root, we could be releasing shelved pages.
    //  This is the case in which we are freeing space which had been previously shelved.
    //  For example, we delete a table that had leaked or available space shelved and that space
    //  has now trickled up to the root so we unshelve that space to allow it to be reused when
    //  the database re-grows.
    //
    if ( ( pgnoLast > g_rgfmp[ pfcb->Ifmp() ].PgnoLast() ) && ( pfcb->PgnoFDP() == pgnoSystemRoot ) )
    {
        Assert( pfucbParent == pfucbNil );
        Assert( !g_rgfmp[ pfcb->Ifmp() ].FIsTempDB() );

        // Remove shelved pages from the shelf.
        const PGNO pgnoFirstUnshelve = max( pgnoFirst, g_rgfmp[ pfcb->Ifmp() ].PgnoLast() + 1 );
        Call( ErrSPIUnshelvePagesInRange( pfucb, pgnoFirstUnshelve, pgnoLast ) );

        if ( pgnoFirst > g_rgfmp[ pfcb->Ifmp() ].PgnoLast() )
        {
            goto HandleError;
        }

        pgnoLast = min( pgnoLast, g_rgfmp[ pfcb->Ifmp() ].PgnoLast() );
        Assert( pgnoLast >= pgnoFirst );
        cpgSize = pgnoLast - pgnoFirst + 1;
    }

    cpgSizeAdj = cpgSize;

    //  open owned extent tree
    //
    Call( ErrSPIOpenOwnExt( pfucb, &pfucbOE ) );
    Assert( pfcb == pfucbOE->u.pfcb );

    //  find bounds of owned extent which contains extent to be freed
    //
    Call( ErrSPIFindExtOE( pfucbOE, pgnoFirst, &cspoeContaining ) );

    // Check consistency.
    if ( ( pgnoFirst > cspoeContaining.PgnoLast() ) || ( pgnoLast < cspoeContaining.PgnoFirst() ) )
    {
        // Extent to be freed is completely outside of the matching OwnExt node.


        err = JET_errSuccess;
        goto HandleError;
    }
    else if ( ( pgnoFirst < cspoeContaining.PgnoFirst() ) || ( pgnoLast > cspoeContaining.PgnoLast() ) )
    {
        // Extent overlaps the matching OwnExt node. This is unexpected because we do not
        // merge AvailExt nodes across OwnExt boundaries.

        FireWall( "CorruptedOeTree" );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbOE ), HaDbFailureTagCorruption, L"9322391a-63fb-4718-948e-89425a75ed2e" );
        Call( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
    }

    BTUp( pfucbOE );

    //  if available extent empty, add extent to be freed.
    //  Otherwise, coalesce with left extents by deleting left extents
    //  and augmenting size.
    //  Coalesce right extent replacing size of right extent.
    //
    Call( ErrSPIOpenAvailExt( pfucb, &pfucbAE ) );
    Assert( pfcb == pfucbAE->u.pfcb );

    if ( pgnoLast == cspoeContaining.PgnoLast()
        && cpgSizeAdj == cspoeContaining.CpgExtent() )
    {
        // Reserve prior to insertion.
        Call( ErrSPIReserveSPBufPages( pfucb, pfucbParent ) );

        //  we're freeing an entire extent, so no point
        //  trying to coalesce
        OnDebug( fWholeKitAndKabudle = fTrue );
        goto InsertExtent;
    }

CoallesceWithNeighbors:

    // Reserve prior to deletion/insertion.
    Call( ErrSPIReserveSPBufPages( pfucb, pfucbParent ) );

    cspextkeyAE.SPExtentMakeKeyBM( SPEXTKEY::fSPExtentTypeAE, pgnoFirst - 1, sppAvailGeneralPool );
    Assert( cspextkeyAE.GetBm( pfucbAE ).data.Pv() == NULL );
    Assert( cspextkeyAE.GetBm( pfucbAE ).data.Cb() == 0 );
    dibAE.pos = posDown;
    dibAE.pbm = cspextkeyAE.Pbm( pfucbAE );
    dibAE.dirflag = fDIRFavourNext;

    err = ErrBTDown( pfucbAE, &dibAE, latchReadTouch );
    if ( JET_errRecordNotFound != err )
    {
        BOOL    fOnNextExtent   = fFalse;

        Assert( err != JET_errNoCurrentRecord );

        if ( err < 0 )
        {
            OSTraceFMP(
                pfucbAE->ifmp,
                JET_tracetagSpaceInternal,
                OSFormat(
                    "%hs: could not go down into nonempty available extent tree. [0x%x:0x%x:%lu].",
                    __FUNCTION__,
                    pfucbAE->ifmp,
                    ObjidFDP( pfucbAE ),
                    PgnoFDP( pfucbAE ) ) );
        }
        Call( err );

        //  found an available extent node
        //
        cspaei.Set( pfucbAE );

        //  Can not Call() b/c we consume err below ...
        ERR errT = cspaei.ErrCheckCorrupted();
        if( errT < JET_errSuccess )
        {
            AssertSz( fFalse, "Corruption bad." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"2bc482de-56bd-44ce-ae3d-d51b7a0343df" );
            Call( errT );
        }
        Assert( cspaei.CpgExtent() >= 0 );  // no longer needed.

#ifdef DEPRECATED_ZERO_SIZED_AVAIL_EXTENT_CLEANUP
        if ( 0 == cspaei.CpgExtent() )
        {
            //  We might have zero-sized extents if we crashed in ErrSPIAddToAvailExt().
            //  Simply delete such extents and retry.
            Call( ErrBTFlagDelete(      // UNDONE: Synchronously remove the node
                        pfucbAE,
                        fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
            // Zero pages, so no need to call pfucbAE->u.pfcb->AddCpgAE()
            BTUp( pfucbAE );
            goto CoallesceWithNeighbors;
        }
#endif

        if ( cspaei.SppPool() == sppAvailGeneralPool )
        {
            //  assert no page is common between available extent node
            //  and freed extent
            //
            Assert( pgnoFirst > cspaei.PgnoLast() ||
                    pgnoLast < cspaei.PgnoFirst() );

            if ( wrnNDFoundGreater == err )
            {
                Assert( cspaei.PgnoLast() > pgnoFirst - 1 );

                //  already on the next node, no need to move there
                fOnNextExtent = fTrue;
            }
            else if ( wrnNDFoundLess == err )
            {
                //  available extent nodes last page < pgnoFirst - 1
                //  no possible coalescing on the left
                //  (this is the last node in available extent tree)
                //
                Assert( cspaei.PgnoLast() < pgnoFirst - 1 );
            }
            else
            {
                Assert( cspaei.PgnoLast() == pgnoFirst - 1 );
                CallS( err );
            }

            if ( JET_errSuccess == err
                && pgnoFirst > cspoeContaining.PgnoFirst() )
            {
                //  found available extent node whose last page == pgnoFirst - 1
                //  can coalesce freed extent with this node after re-keying
                //
                //  the second condition is to ensure that we do not coalesce
                //      two available extent nodes that belong to different owned extent nodes
                //
                Assert( cspaei.PgnoFirst() >= cspoeContaining.PgnoFirst() );
                #ifdef DEBUG
                CPG cpgAECurrencyCheck;
                CallS( ErrSPIExtentCpg( pfucbAE, &cpgAECurrencyCheck ) );
                Assert( cspaei.CpgExtent() == cpgAECurrencyCheck );
                #endif
                Assert( cspaei.PgnoLast() == pgnoFirst - 1 );

                OSTraceFMP(
                    pfucb->ifmp,
                    JET_tracetagSpaceInternal,
                    OSFormat(
                        "%hs: Merging left E(%lu + %lu) with E(%lu + %lu) [0x%x:0x%x:%lu].",
                        __FUNCTION__,
                        cspaei.PgnoFirst(),
                        cspaei.CpgExtent(),
                        pgnoFirst,
                        cpgSizeAdj,
                        pfucb->ifmp,
                        ObjidFDP( pfucb ),
                        PgnoFDP( pfucb ) ) );

                cpgSizeAdj += cspaei.CpgExtent();
                pgnoFirst -= cspaei.CpgExtent();
                Assert( pgnoLast == pgnoFirst + cpgSizeAdj - 1 );

                Call( ErrSPIWrappedBTFlagDelete(      // UNDONE: Synchronously remove the node
                            pfucbAE,
                            fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                            0,
                            -(cspaei.CpgExtent() ) ) );

                //  Now, we can attempt coalescing on the right if we haven't formed a full extent
                Assert( !fOnNextExtent );
                if ( pgnoLast == cspoeContaining.PgnoLast()
                    && cpgSizeAdj == cspoeContaining.CpgExtent() )
                {
                    OnDebug( fWholeKitAndKabudle = fTrue );
                    goto InsertExtent;
                }

                Pcsr( pfucbAE )->Downgrade( latchReadTouch );

                //  verify we're still within the boundaries of OwnExt
                Assert( cspoeContaining.PgnoFirst() <= pgnoFirst );
                Assert( cspoeContaining.PgnoLast() >= pgnoLast );
            }

            //  now see if we can coalesce with next node, first moving there if necessary
            //
            if ( !fOnNextExtent )
            {
                err = ErrBTNext( pfucbAE, fDIRNull );
                if ( err < 0 )
                {
                    if ( JET_errNoCurrentRecord == err )
                    {
                        err = JET_errSuccess;
                    }
                    else
                    {
                        Call( err );
                    }
                }
                else
                {
                    //  successfully moved to next extent,
                    //  so we can try to coalesce
                    fOnNextExtent = fTrue;
                }
            }

            if ( fOnNextExtent )
            {
                cspaei.Set( pfucbAE );

                if ( cspaei.SppPool() == sppAvailGeneralPool )
                {
                    Assert( cspaei.CpgExtent() != 0 );
                    //  verify no page is common between available extent node
                    //  and freed extent
                    //
                    if ( pgnoLast >= cspaei.PgnoFirst() )
                    {
                        AssertSz( fFalse, "AvailExt corrupted." );
                        OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"5bc71f68-0ef3-4fb6-9c36-20bdbb1f5a70" );
                        Call( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
                    }

                    if ( ( pgnoLast == ( cspaei.PgnoFirst() - 1 ) ) &&
                         ( cspaei.PgnoLast() <= cspoeContaining.PgnoLast() ) )
                    {
                        //  freed extent falls exactly in front of available extent node
                        //          -- coalesce freed extent with current node
                        //
                        //  the second condition ensures that we do not coalesce
                        //      two available extent nodes that are from different
                        //      owned extent nodes
                        //
                        CSPExtentNodeKDF spAdjustedSize( SPEXTKEY::fSPExtentTypeAE, cspaei.PgnoLast(), cspaei.CpgExtent(), sppAvailGeneralPool );
                        // Remember the actual number of pages we're adding to the node we're coalescing with.
                        CPG cpgAdded = cpgSizeAdj;

                        // Put the pages we're freeing into the node we're coalescing with.
                        spAdjustedSize.ErrUnconsumeSpace( cpgSizeAdj );
                        cpgSizeAdj += cspaei.CpgExtent();

                        Call( ErrSPIWrappedBTReplace(
                                    pfucbAE,
                                    spAdjustedSize.GetData( ),
                                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                                    0,
                                    cpgAdded ) );
                        Assert( Pcsr( pfucbAE )->FLatched() );

                        OSTraceFMP(
                            pfucb->ifmp,
                            JET_tracetagSpaceInternal,
                            OSFormat(
                                "%hs: Merged E(%lu + %lu) with right E(%lu + %lu) [0x%x:0x%x:%lu].",
                                __FUNCTION__,
                                pgnoFirst,
                                cpgAdded,
                                cspaei.PgnoFirst(),
                                cspaei.CpgExtent(),
                                pfucb->ifmp,
                                ObjidFDP( pfucb ),
                                PgnoFDP( pfucb ) ) );

                        pgnoLast = cspaei.PgnoLast();
                        fCoalesced = fTrue;

                        if ( cpgSizeAdj >= cpageSEDefault
                            && pgnoNull != pfcb->PgnoNextAvailSE()
                            && pgnoFirst < pfcb->PgnoNextAvailSE() )
                        {
                            pfcb->SetPgnoNextAvailSE( pgnoFirst );
                        }
                    }
                }
            }
        }
    }

    //  add new node to available extent tree
    //
    if ( !fCoalesced )
    {
InsertExtent:

        //  Now, see if we've an insertion point right after us ...
        //
        BTUp( pfucbAE );

        const CPG cpgSave = cpgSizeAdj;
        Call( ErrSPIAERemoveInsertionRegion( pfcb, pfucbAE, &cspoeContaining, pgnoFirst, &cpgSizeAdj ) );

        Assert( !fWholeKitAndKabudle || wrnSPReservedPages !=  err );

        if ( wrnSPReservedPages == err ||
                cpgSave != cpgSizeAdj )
        {
            Assert( wrnSPReservedPages == err && cpgSave != cpgSizeAdj );   // both or neither should be true.

            //  We actually free'd some extra space from a collapsing insertion region
            //
            pgnoLast = pgnoFirst + cpgSizeAdj - 1;

            Assert( cspoeContaining.FContains( pgnoLast ) );    // shouldn't have offended this.
            Assert( cspoeContaining.FContains( pgnoFirst ) );

            goto CoallesceWithNeighbors;
        }

        BTUp( pfucbAE );

        Call( ErrSPIAddFreedExtent(
                    pfucb,
                    pfucbAE,
                    pgnoLast,
                    cpgSizeAdj ) );
    }
    Assert( Pcsr( pfucbAE )->FLatched() );

    //  if extent freed coalesced with available extents
    //  form a complete secondary extent, remove the secondary extent
    //  from the FDP and free it to the parent FDP.
    //  Since FDP is first page of primary extent,
    //  we do not have to guard against freeing
    //  primary extents.
    //  UNDONE: If parent FDP is NULL, FDP is device level and
    //          complete, free secondary extents to device.
    //
    Assert( pgnoLast != cspoeContaining.PgnoLast() || cpgSizeAdj <= cspoeContaining.CpgExtent() );
    if ( pgnoLast == cspoeContaining.PgnoLast() && cpgSizeAdj == cspoeContaining.CpgExtent() )
    {
        Assert( cpgSizeAdj > 0 );

#ifdef DEBUG
        Assert( Pcsr( pfucbAE )->FLatched() );
        cspaei.Set( pfucbAE );
        Assert( cspaei.PgnoLast() == pgnoLast );
        Assert( cspaei.CpgExtent() == cpgSizeAdj );
#endif

#ifdef DEBUG
        if ( ErrFaultInjection( 62250 ) >= JET_errSuccess )
#endif
        {
            //  owned extent node is same as available extent node
            Call( ErrSPIFreeSEToParent( pfucb, pfucbOE, pfucbAE, pfucbParent, cspoeContaining.PgnoLast(), cspoeContaining.CpgExtent() ) );
        }
    }

HandleError:

    Assert( FSPExpectedError( err ) );

    AssertSPIPfucbOnRoot( pfucb );
    AssertSPIPfucbNullOrUnlatched( pfucbParent );

    if ( pfucbAE != pfucbNil )
    {
        BTClose( pfucbAE );
    }
    if ( pfucbOE != pfucbNil )
    {
        BTClose( pfucbOE );
    }

    Assert( err != JET_errKeyDuplicate );

    return err;
}

// If FDP is a table, capture table root page and all the associated indices and LV tree root page else capture the FDP root page
// in RBS with special flags to indicate that the only action allowed on the table after revert is delete.
// Also, logs LREXTENTFREED
//
ERR ErrSPCaptureNonRevertableFDPRootPage( PIB *ppib, FCB* pfcbFDPToFree, const PGNO pgnoLVRoot, CPG* const pcpgCaptured )
{
    Assert( ppib );
    Assert( pfcbFDPToFree );
    Assert( pfcbFDPToFree->PgnoFDP() != pgnoNull );

    FUCB* pfucb     = pfucbNil;
    ERR err         = JET_errSuccess;
    CPG cpgCaptured = 0;
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();

    CallR( ErrBTOpen( ppib, pfcbFDPToFree, &pfucb ) );

    Assert( pfucbNil != pfucb );
    Assert( pfucb->u.pfcb->FInitialized() );
    Assert( !FFUCBSpace( pfucb ) );

    BOOL fEfvEnabled = ( g_rgfmp[pfucb->ifmp].FLogOn() && PinstFromPfucb( pfucb )->m_plog->ErrLGFormatFeatureEnabled( JET_efvRevertSnapshot ) >= JET_errSuccess );

    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortFreeExtSnapshot );

    // Capture the preimage of the table root and pass flag to indicate this is a delete table so that we special mark this table when reverted.
    // We should generally not be touching the table pages before table delete.
    // But in case we did due to some bug or some unexpected scenario, we will pass fRBSPreimageRevertAlways to make sure we always keep the table deleted.
    Call( ErrRBSRDWLatchAndCapturePreImage(
        pfucb->ifmp,
        PgnoRoot( pfucb ),
        dbtimeNil,
        fRBSDeletedTableRootPage,
        pfucb->ppib->BfpriPriority( pfucb->ifmp ),
        *tcScope ) );
    cpgCaptured++;

    if ( fEfvEnabled )
    {
        // Log extent being freed with special flag indicating rootpage of deleted table so available lag can capture the pre-image.
        Call( ErrLGExtentFreed( PinstFromPfucb( pfucb )->m_plog, pfucb->ifmp, PgnoRoot( pfucb ), 1, fTrue ) );
    }

    // Now capture all the associated indices and LV tree root page if the FDP being freed is a primary table.
    if ( pfcbFDPToFree->FTypeTable() )
    {
        for ( const FCB* pfcbT = pfcbFDPToFree->PfcbNextIndex(); pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
        {
            Call( ErrRBSRDWLatchAndCapturePreImage(
                pfucb->ifmp,
                pfcbT->PgnoFDP(),
                dbtimeNil,
                fRBSDeletedTableRootPage,
                pfucb->ppib->BfpriPriority( pfucb->ifmp ),
                *tcScope ) );
            cpgCaptured++;

            if ( fEfvEnabled )
            {
                // Log extent being freed with special flag indicating rootpage of deleted table so available lag can capture the pre-image.
                Call( ErrLGExtentFreed( PinstFromPfucb( pfucb )->m_plog, pfucb->ifmp, pfcbT->PgnoFDP(), 1, fTrue ) );
            }
        }

        if ( pgnoLVRoot != pgnoNull )
        {
            Call( ErrRBSRDWLatchAndCapturePreImage(
                pfucb->ifmp,
                pgnoLVRoot,
                dbtimeNil,
                fRBSDeletedTableRootPage,
                pfucb->ppib->BfpriPriority( pfucb->ifmp ),
                *tcScope ) );
            cpgCaptured++;

            if ( fEfvEnabled )
            {
                // Log extent being freed with special flag indicating rootpage of deleted table so available lag can capture the pre-image.
                Call( ErrLGExtentFreed( PinstFromPfucb( pfucb )->m_plog, pfucb->ifmp, pgnoLVRoot, 1, fTrue ) );
            }
        }
    }

    if ( pcpgCaptured != NULL )
    {
        *pcpgCaptured = cpgCaptured;
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        SPIReportSpaceLeak( pfucb, err, pfcbFDPToFree->PgnoFDP(), 1, "CaptureNonRevertableFDPRootPage" );
    }

    if ( pfucbNil != pfucb )
    {
        pfucb->pcsrRoot = pcsrNil;
        BTClose( pfucb );
    }

    return err;
}

ERR ErrSPCaptureSpaceTreePages( FUCB* const pfucbParent, FCB* pfcb, CPG* pcpgSnapshotted )
{
    FUCB* pfucbOE   = pfucbNil;
    ERR err         = JET_errSuccess;

    //  open owned extent tree of freed FDP
    CallR( ErrSPIOpenOwnExt( pfucbParent->ppib, pfcb, &pfucbOE ) );

    Assert( pfucbOE );
    Assert( pfucbOE->fOwnExt );

    // Set the start bookmark to 0 and end bookmark to last possible key so that we get all the space tree pages.
    CSPExtentKeyBM spoebmStart( SPEXTKEY::fSPExtentTypeOE, 0, SpacePool::MinPool );
    CSPExtentKeyBM spoebmEnd( SPEXTKEY::fSPExtentTypeOE, pgnoSysMax, SpacePool::AvailExtLegacyGeneralPool );
    LONG cbmPreread;
    PGNO* rgPgnos   = NULL;
    CPG   cpgno     = 0;
    PGNO pgnoFirst  = pgnoNull;
    LONG cpgExtent  = 0;

    PIBTraceContextScope tcScope = pfucbOE->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucbOE );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucbOE ) );
    tcScope->iorReason.SetIort( iortFreeExtSnapshot );

    PrereadContext context( pfucbOE->ppib, pfucbOE );
    Call( context.ErrPrereadBookmarkRanges(
        spoebmStart.Pbm( pfucbOE),
        spoebmEnd.Pbm( pfucbOE ),
        1,
        &cbmPreread,
        lMax,
        lMax,
        JET_bitPrereadForward | bitPrereadSkip | bitIncludeNonLeafRead,
        NULL ) );

    PGNO* rgLeafPgnos       = context.RgPgno( PrereadContext::PrereadPgType::LeafPages );
    PGNO* rgNonLeafPgnos    = context.RgPgno( PrereadContext::PrereadPgType::NonLeafPages );
    CPG   cpgLeafPgnos      = context.CPgnos( PrereadContext::PrereadPgType::LeafPages );
    CPG   cpgNonLeafPgnos   = context.CPgnos( PrereadContext::PrereadPgType::NonLeafPages );

    // Last element in array should be pgnoNull
    Assert( rgLeafPgnos[cpgLeafPgnos - 1] == pgnoNull );
    Assert( rgNonLeafPgnos[cpgNonLeafPgnos - 1] == pgnoNull );

    cpgno   = cpgLeafPgnos + cpgNonLeafPgnos - 2;
    rgPgnos = new PGNO[cpgno];

    // Ignore the pgnoNull
    memcpy( rgPgnos, rgNonLeafPgnos, ( cpgNonLeafPgnos - 1 ) * sizeof( PGNO ) );
    memcpy( rgPgnos + cpgNonLeafPgnos - 1, rgLeafPgnos, ( cpgLeafPgnos - 1 ) * sizeof( PGNO ) );

    std::sort( rgPgnos, rgPgnos + cpgno - 1, CmpPgno );

    // Find if there is any continuous extent in space tree. Also capture preimage if needed.
    for ( LONG ipg = 0; ipg < cpgno; ++ipg )
    {
        Assert( rgPgnos[ipg] != pgnoNull );

        if ( pgnoFirst == pgnoNull )
        {
            pgnoFirst = rgPgnos[ipg];
            cpgExtent = 1;
        }
        else if ( rgPgnos[ipg - 1] + 1 == rgPgnos[ipg] )
        {
            // Contiguous page. Consider it as part of the current extent being tracked.
            cpgExtent++;
        }
        else
        {
            Call( ErrSPCaptureSnapshot( pfucbOE, pgnoFirst, cpgExtent, fFalse ) );
            pgnoFirst = rgPgnos[ipg];
            cpgExtent = 1;
        }
    }

    // Capture any extents we haven't captured yet.
    Assert( cpgExtent > 0 );
    Call( ErrSPCaptureSnapshot( pfucbOE, pgnoFirst, cpgExtent, fFalse ) );

    *pcpgSnapshotted = cpgno;

HandleError:
    if ( err < JET_errSuccess )
    {
        SPIReportSpaceLeak( pfucbOE, err, pgnoFirst, cpgExtent, "CaptureSpaceTreePages" );
    }

    if ( pfucbOE != pfucbNil )
    {
        pfucbOE->pcsrRoot = pcsrNil;
        BTClose( pfucbOE );
    }

    if ( rgPgnos )
    {
        delete[] rgPgnos;
    }

    return err;
}

//  ErrSPCaptureSnapshot
//  ========================================================================
//  Capture snapshot of a set of pages, used to capture pages freed without
//  first merging/moving and hence dirtying them. Currently used for when a
//  whole Tree is freed (DeleteIndex/DeleteTable) but could be used in the
//  future to free pages without first emptying and dirtying them.
//
//  PARAMETERS  pfucb           tree to which the extent is freed,
//                              cursor should have currency on root page [RIW latched]
//              pgnoFirst       page number of first page in extent to be freed
//              cpgSize         number of pages in extent to be freed
//
//
//  SIDE EFFECTS
//  COMMENTS
//-
ERR ErrSPCaptureSnapshot( FUCB* const pfucb, const PGNO pgnoFirst, CPG cpgSize, const BOOL fMarkExtentEmptyFDPDeleted )
{
    ERR err = JET_errSuccess;
    BOOL fEfvEnabled = ( g_rgfmp[pfucb->ifmp].FLogOn() && PinstFromPfucb( pfucb )->m_plog->ErrLGFormatFeatureEnabled( JET_efvRevertSnapshot ) >= JET_errSuccess );
    const CPG cpgPrereadMax = LFunctionalMax( 1, (CPG)( UlParam( JET_paramMaxCoalesceReadSize ) / g_cbPage ) );
    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortFreeExtSnapshot );

    Assert( !PinstFromPfucb( pfucb )->FRecovering() );

    const PGNO pgnoLastFMP = g_rgfmp[ pfucb->ifmp ].PgnoLast();

    // The pages we want to capture are beyond the last page owned by this database.
    // This is possible if we have some shelved pages due to shrink and later the table being deleted.
    // Table deletion would try to log ExtentFreed during delete for the shelved pages which might be beyond EOF.
    // We should be able to safely ignore their pages as should have been captured when they were shelved.
    if ( pgnoFirst > pgnoLastFMP )
    {
        return JET_errSuccess;
    }
    else if ( pgnoFirst + cpgSize - 1 > pgnoLastFMP )
    {
        AssertTrack( fFalse, "CaptureSnapshotCrossEof" );
        cpgSize = pgnoLastFMP - pgnoFirst + 1;
    }

    // We don't have to break it down as per preread chunk since we are not reading the preimages instead just marking it to be reverted to a new page state with some special flags.
    if ( fMarkExtentEmptyFDPDeleted )
    {
        // If lag is the active here we will capture preimages of the freed extent but mark it as empty page since FDP being deleted is non-revertable.
        if ( g_rgfmp[ pfucb->ifmp ].Dbid() != dbidTemp && g_rgfmp[ pfucb->ifmp ].FRBSOn() )
        {
            // Capture all freed but non-revertable extent pages as if they need to be reverted to empty pages when RBS is applied with special flag fRBSFDPDeleted.
            // If we already captured a preimage for one of those pages in the extent, the revert to an empty page will be ignored for that page when we apply the snapshot.
            CallR( g_rgfmp[ pfucb->ifmp ].PRBS()->ErrCaptureEmptyPages( g_rgfmp[ pfucb->ifmp ].Dbid(), pgnoFirst, cpgSize, fRBSFDPNonRevertableDelete ) );
        }

        if ( fEfvEnabled )
        {
            // Log extent being freed so available lag can capture the pre-image.
            // Tight loop here can cause contention on log buffer.
            CallR( ErrLGExtentFreed( PinstFromPfucb( pfucb )->m_plog, pfucb->ifmp, pgnoFirst, cpgSize, fFalse, fTrue ) );
        }
    }
    else
    {
        // Break it up into I/O read size for reduce preread load on both passive and active
        for ( CPG cpgT = 0; cpgT < cpgSize; cpgT += cpgPrereadMax )
        {
            CPG cpgRead = LFunctionalMin( cpgSize - cpgT, cpgPrereadMax );

            // If lag is the active here we will capture preimages of the freed extent here before the extent is freed.
            if ( g_rgfmp[ pfucb->ifmp ].FRBSOn() )
            {
                BFPrereadPageRange( pfucb->ifmp, pgnoFirst + cpgT, cpgRead, bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->ifmp ), *tcScope );
                PinstFromPfucb( pfucb )->m_plog->LGAddFreePages( cpgRead );

                BFLatch bfl;

                for ( int i = 0; i < cpgRead; ++i )
                {
                    err = ErrBFWriteLatchPage( &bfl, pfucb->ifmp, pgnoFirst + cpgT + i, bflfUninitPageOk, pfucb->ppib->BfpriPriority( pfucb->ifmp ), *tcScope );

                    // It is fine if page is not initialized as there is no preimage to capture for it.
                    if ( err == JET_errPageNotInitialized )
                    {
                        BFMarkAsSuperCold( pfucb->ifmp, pgnoFirst + cpgT + i );
                        continue;
                    }
                    // Should we still allow operation to succeed and collect rest of snapshot?
                    CallR( err );
                    BFMarkAsSuperCold( &bfl );
                    BFWriteUnlatch( &bfl );
                }
            }

            if ( fEfvEnabled )
            {
                // Log extent being freed so available lag can capture the pre-image.
                // Tight loop here can cause contention on log buffer.
                CallR( ErrLGExtentFreed( PinstFromPfucb( pfucb )->m_plog, pfucb->ifmp, pgnoFirst + cpgT, cpgRead ) );
            }
        }
    }

    return JET_errSuccess;
}

//  ErrSPFreeExt
//  ========================================================================
//  Frees an extent to an FDP.  The extent, starting at page pgnoFirst
//  and cpgSize pages long, is added to available extent of the FDP.  If the
//  extent freed is a complete secondary extent of the FDP, or can be
//  coalesced with other available extents to form a complete secondary
//  extent, the complete secondary extent is freed to the parent FDP.
//
//  Besides, if the freed extent is contiguous with the FUCB space cache
//  in pspbuf, the freed extent is added to the FUCB cache. Also, when
//  an extent is freed recursively to the parentFDP, the FUCB on the parent
//  shares the same FUCB cache.
//
//  PARAMETERS  pfucb           tree to which the extent is freed,
//                              cursor should have currency on root page [RIW latched]
//              pgnoFirst       page number of first page in extent to be freed
//              cpgSize         number of pages in extent to be freed
//
//
//  SIDE EFFECTS
//  COMMENTS
//-
ERR ErrSPFreeExt( FUCB* const pfucb, const PGNO pgnoFirst, const CPG cpgSize, const CHAR* const szTag )
{
    ERR         err;
    FCB         * const pfcb    = pfucb->u.pfcb;
    BOOL        fRootLatched    = fFalse;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortSpace );

    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagSpaceInternal,
        OSFormat(
            "%hs: About to free E(%lu + %lu) to [0x%x:0x%x:%lu].",
            __FUNCTION__,
            pgnoFirst,
            cpgSize,
            pfucb->ifmp,
            ObjidFDP( pfucb ),
            PgnoFDP( pfucb ) ) );

    // check for valid input
    //
    Assert( cpgSize > 0 );

#ifdef SPACECHECK
    CallS( ErrSPIValidFDP( ppib, pfucb->ifmp, PgnoFDP( pfucb ) ) );
#endif

    Call( ErrFaultInjection( 41610 ) );

    //  if in space tree, return page back to split buffer
    if ( FFUCBSpace( pfucb ) )
    {
        Call( ErrSPISPFreeExt( pfucb, pgnoFirst, cpgSize ) );
    }
    else
    {
        //  if caller did not have root page latched, get root page
        //
        if ( pfucb->pcsrRoot == pcsrNil )
        {
            Assert( !Pcsr( pfucb )->FLatched() );
            Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
            pfucb->pcsrRoot = Pcsr( pfucb );
            fRootLatched = fTrue;
        }
        else
        {
            Assert( pfucb->pcsrRoot->Pgno() == PgnoRoot( pfucb ) );
            Assert( pfucb->pcsrRoot->Latch() == latchRIW
    //          || ( pfucb->pcsrRoot->Latch() == latchWrite && pgnoNull == pfcb->PgnoOE() ) ); // SINGLE EXTEND
    //          We can have a page that was converted from single to multiple extent but on the error handling
    //          further in the split will try to free that page: it will be latchWrite but it will have pfcb->PgnoOE()
    //          as it is after the conversion. We need to remove pgnoNull == pfcb->PgnoOE() condition from above
                || pfucb->pcsrRoot->Latch() == latchWrite ); // SINGLE EXTEND
        }

        if ( !pfcb->FSpaceInitialized() )
        {
            SPIInitFCB( pfucb, fTrue );
        }

        //
        // Make sure the values are correct before we start
        //
        SPIValidateCpgOwnedAndAvail( pfucb );

#ifdef SPACECHECK
        CallS( ErrSPIWasAlloc( pfucb, pgnoFirst, cpgSize ) );
#endif

        Assert( pfucb->pcsrRoot != pcsrNil );

        //  if single extent format, then free extent in external header
        //

        if ( FSPIIsSmall( pfcb ) )
        {
            Call( ErrSPISmallFreeExt( pfucb, pgnoFirst, cpgSize ) );
        }
        else
        {
            Call( ErrSPIAEFreeExt( pfucb, pgnoFirst, cpgSize ) );
        }

        //
        // Make sure the values are correct after we're done
        //
        SPIValidateCpgOwnedAndAvail( pfucb );
    }

    CallS( err );   // can warning happen?

HandleError:

    if( err < JET_errSuccess )
    {
        OSTraceFMP(
            pfucb->ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Failed to free E(%lu + %lu) to [0x%x:0x%x:%lu] err=(%d:0x%08X)",
                __FUNCTION__,
                pgnoFirst,
                cpgSize,
                pfucb->ifmp,
                ObjidFDP( pfucb ),
                PgnoFDP( pfucb ),
                err,
                err ) );

        SPIReportSpaceLeak( pfucb, err, pgnoFirst, cpgSize, szTag );
    }
    else
    {
        const PGNO pgnoFDP = PgnoFDP( pfucb );
        if ( cpgSize > 1 )
        {
            ETSpaceFreeExt( pfucb->ifmp, pgnoFDP, pgnoFirst, cpgSize, pfucb->u.pfcb->ObjidFDP(), TceFromFUCB( pfucb ) );
        }
        else
        {
            ETSpaceFreePage( pfucb->ifmp, pgnoFDP, pgnoFirst, pfucb->u.pfcb->ObjidFDP(), TceFromFUCB( pfucb ) );
        }

        OSTraceFMP(
            pfucb->ifmp,
            JET_tracetagSpaceManagement,
            OSFormat(
                "%hs: Freed E(%lu + %lu) to [0x%x:0x%x:%lu].",
                __FUNCTION__,
                pgnoFirst,
                cpgSize,
                pfucb->ifmp,
                ObjidFDP( pfucb ),
                PgnoFDP( pfucb ) ) );
    }

    if ( fRootLatched )
    {
        Assert( Pcsr( pfucb ) == pfucb->pcsrRoot );
        Assert( pfucb->pcsrRoot->FLatched() );
        pfucb->pcsrRoot->ReleasePage();
        pfucb->pcsrRoot = pcsrNil;
    }

    Assert( err != JET_errKeyDuplicate );

    return err;
}

LOCAL ERR ErrErrBitmapToJetErr( const IBitmapAPI::ERR err )
{
    switch ( err )
    {
        case IBitmapAPI::ERR::errSuccess:
            return JET_errSuccess;

        case IBitmapAPI::ERR::errInvalidParameter:
            return ErrERRCheck( JET_errInvalidParameter );

        case IBitmapAPI::ERR::errOutOfMemory:
            return ErrERRCheck( JET_errOutOfMemory );
    }

    AssertSz( fFalse, "UnknownIBitmapAPI::ERR: %s", (int)err );
    return ErrERRCheck( JET_errInternalError );
}


//  ErrSPTryCoalesceAndFreeAvailExt
//  ========================================================================
//  Tries to coalesce smaller available extents into one larger extent that
//  fully matches an owned extent so that it can be released up the hierarchy
//  chain.
//
//  This function will only either fully release the extent to its parent or
//  leave the current layout as is, which means it will only make modifications
//  to the existing extent layout if an extent that can be released can be produced.
//
//  PARAMETERS  pfucb           tree to coalesce space for
//              pgnoInExtent    any page in the extent to be assessed
//              pfCoalesced     whether or not coalescing/freeing happened
//
//
//  SIDE EFFECTS
//  COMMENTS
//-
ERR ErrSPTryCoalesceAndFreeAvailExt( FUCB* const pfucb, const PGNO pgnoInExtent, BOOL* const pfCoalesced )
{
    Assert( !FFUCBSpace( pfucb ) );

    ERR err = JET_errSuccess;
    FCB* const pfcb = pfucb->u.pfcb;
    FUCB* pfucbOE = pfucbNil;
    FUCB* pfucbAE = pfucbNil;
    CSPExtentInfo speiContaining;
    CSparseBitmap spbmAvail;

    Assert( !FSPIIsSmall( pfcb ) );
    Assert( pfcb->PgnoFDP() != pgnoSystemRoot );

    *pfCoalesced = fFalse;

    // "Lock" the root of the tree.
    Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
    pfucb->pcsrRoot = Pcsr( pfucb );

    SPIValidateCpgOwnedAndAvail( pfucb );

    // Open cursors to space trees.
    Call( ErrSPIOpenOwnExt( pfucb, &pfucbOE ) );
    Call( ErrSPIOpenAvailExt( pfucb, &pfucbAE ) );

    // Figure out the owning extent.
    err = ErrSPIFindExtOE( pfucbOE, pgnoInExtent, &speiContaining );
    if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
    {
        err = JET_errSuccess;
    }
    Call( err );
    if ( !speiContaining.FIsSet() || !speiContaining.FContains( pgnoInExtent ) )
    {
        goto HandleError;
    }
    BTUp( pfucbOE );

    const PGNO pgnoOeFirst = speiContaining.PgnoFirst();
    const PGNO pgnoOeLast = speiContaining.PgnoLast();
    const CPG cpgOwnExtent = speiContaining.CpgExtent();

    // Now, look up in the available extent collection.
    // Go through the pools and try to make up the full containing extent. If there are any pages in a pool
    // we do not recognize, bail out.

    Call( ErrErrBitmapToJetErr( spbmAvail.ErrInitBitmap( cpgOwnExtent ) ) );

    for ( SpacePool sppAvailPool = spp::MinPool;
            sppAvailPool < spp::MaxPool;
            sppAvailPool++ )
    {
        // We shouldn't find anything in the shelved pool anyways (this is not the DB root), but avoid
        // hitting asserts that warn us against trying to find shelved pages outside of the DB root.
        if ( sppAvailPool == spp::ShelvedPool )
        {
            continue;
        }

        // Start from the first page of the containing OE.
        PGNO pgno = pgnoOeFirst;
        while ( pgno <= pgnoOeLast )
        {
            CSPExtentInfo speiAE;
            err = ErrSPIFindExtAE( pfucbAE, pgno, sppAvailPool, &speiAE );
            if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
            {
                err = JET_errSuccess;
            }
            Call( err );
            err = JET_errSuccess;

            Assert( !speiAE.FIsSet() || ( speiAE.PgnoLast() >= pgno ) );

            // Invalid/unexpected cases.
            if ( speiAE.FIsSet() &&
                 ( speiAE.CpgExtent() > 0 ) &&
                 ( ( speiAE.PgnoFirst() < pgnoOeFirst ) ||
                   ( speiContaining.FContains( speiAE.PgnoFirst() ) && !speiContaining.FContains( speiAE.PgnoLast() ) ) ) )
            {
                AssertTrack( fFalse, "TryCoalesceAvailOverlapExt" );
                Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
            }

            // Unknown pool: do not fail, but bail
            if ( speiAE.FIsSet() &&
                 ( speiAE.SppPool() != spp::AvailExtLegacyGeneralPool ) &&
                 ( speiAE.SppPool() != spp::ContinuousPool ) )
            {
                FireWall( OSFormat( "TryCoalesceAvailUnkPool:0x%I32x", speiAE.SppPool() ) );
                goto HandleError;
            }

            // We're done with this pool.
            if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() > pgnoOeLast ) )
            {
                break;
            }

            // Set all bits.
            if ( speiAE.CpgExtent() > 0 )
            {
                for ( PGNO pgnoAvail = speiAE.PgnoFirst(); pgnoAvail <= speiAE.PgnoLast(); pgnoAvail++ )
                {
#ifdef DEBUG
                    BOOL fSet = fFalse;
                    if ( spbmAvail.ErrGet( pgnoAvail - pgnoOeFirst, &fSet ) == IBitmapAPI::ERR::errSuccess )
                    {
                        Assert( !fSet );
                    }
#endif

                    Call( ErrErrBitmapToJetErr( spbmAvail.ErrSet( pgnoAvail - pgnoOeFirst, fTrue ) ) );
                }
            }

            BTUp( pfucbAE );

            pgno = speiAE.PgnoLast() + 1;
        }
        BTUp( pfucbAE );
    }
    BTUp( pfucbAE );

    // Check the bitmap. We're only able to coalesce and free it if the entire extent is set as available.
    // We're more likely to short-circuit early if we check both ends in alternation under shrink, which is
    // currently the only consumer of this function. Consider a more optimal lookup method than the bitmap
    // we're using if this is used in the main code line.
    Expected( g_rgfmp[ pfucb->ifmp ].FShrinkIsRunning() );
    for ( PGNO ipgno = 0; ipgno < (PGNO)cpgOwnExtent; ipgno++ )
    {
        const size_t i = ( ( ipgno % 2 ) == 0 ) ? ( ipgno / 2 ) : ( (PGNO)cpgOwnExtent - 1 - ( ipgno / 2 ) );

        BOOL fSet = fFalse;
        Call( ErrErrBitmapToJetErr( spbmAvail.ErrGet( i, &fSet ) ) );
        if ( !fSet )
        {
            goto HandleError;
        }
    }

    // At this point, we know we can release the extent to its parent, so we need
    // to delete all AE nodes, then delete the OE node.
    for ( SpacePool sppAvailPool = spp::MinPool;
            sppAvailPool < spp::MaxPool;
            sppAvailPool++ )
    {
        // We shouldn't find anything in the shelved pool anyways (this is not the DB root), but avoid
        // hitting asserts that warn us against trying to find shelved pages outside of the DB root.
        if ( sppAvailPool == spp::ShelvedPool )
        {
            continue;
        }

        // Start from the first page of the containing OE.
        PGNO pgno = pgnoOeFirst;
        while ( pgno <= pgnoOeLast )
        {
            Call( ErrSPIReserveSPBufPages( pfucb ) );

            CSPExtentInfo speiAE;
            err = ErrSPIFindExtAE( pfucbAE, pgno, sppAvailPool, &speiAE );
            if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
            {
                err = JET_errSuccess;
            }
            Call( err );
            err = JET_errSuccess;

            Assert( !speiAE.FIsSet() || ( speiAE.PgnoLast() >= pgno ) );
            Assert( !( speiAE.FIsSet() &&
                       ( speiAE.CpgExtent() > 0 ) &&
                       ( ( speiAE.PgnoFirst() < pgnoOeFirst ) ||
                         ( speiContaining.FContains( speiAE.PgnoFirst() ) && !speiContaining.FContains( speiAE.PgnoLast() ) ) ) ) );
            Assert( !( speiAE.FIsSet() &&
                       ( speiAE.SppPool() != spp::AvailExtLegacyGeneralPool ) &&
                       ( speiAE.SppPool() != spp::ContinuousPool ) ) );

            // We're done with this pool.
            if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() > pgnoOeLast ) )
            {
                break;
            }

            pgno = speiAE.PgnoFirst();

            // Delete the AE node.
            Call( ErrSPIWrappedBTFlagDelete(      // UNDONE: Synchronously remove the node
                        pfucbAE,
                        fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                        0,
                        -( speiAE.CpgExtent() ) ) );

            BTUp( pfucbAE );

            SPIValidateCpgOwnedAndAvail( pfucb );

            // Just so we can check afterwards.
            while ( pgno <= speiAE.PgnoLast() )
            {
                Call( ErrErrBitmapToJetErr( spbmAvail.ErrSet( pgno - pgnoOeFirst, fFalse ) ) );
                pgno++;
            }
        }
        BTUp( pfucbAE );
    }
    BTUp( pfucbAE );

    // Make sure we actually deleted all AE nodes that make up this extent before deleting the OE node.
    for ( PGNO pgno = pgnoOeFirst; pgno <= pgnoOeLast; pgno++ )
    {
        BOOL fSet = fFalse;
        Call( ErrErrBitmapToJetErr( spbmAvail.ErrGet( pgno - pgnoOeFirst, &fSet ) ) );
        if ( fSet )
        {
            AssertTrack( fFalse, "TryCoalesceAvailAeChanged" );
            Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }
    }

    Call( ErrSPIReserveSPBufPages( pfucb ) );

    // Free it to the parent.
    Call( ErrSPIFreeSEToParent(
            pfucb,
            pfucbOE,
            pfucbNil,
            pfucbNil,
            pgnoOeLast,
            cpgOwnExtent ) );

    *pfCoalesced = fTrue;

    SPIValidateCpgOwnedAndAvail( pfucb );

HandleError:
    if ( pfucbAE != pfucbNil )
    {
        BTUp( pfucbAE );
        BTClose( pfucbAE );
        pfucbAE = pfucbNil;
    }

    if ( pfucbOE != pfucbNil )
    {
        BTUp( pfucbOE );
        BTClose( pfucbOE );
        pfucbOE = pfucbNil;
    }

    BTUp( pfucb );
    pfucb->pcsrRoot = pcsrNil;

    return err;
}


//  ErrSPShelvePage
//  ========================================================================
//  Adds a page as shelved, i.e., adds a single-page extent or appends to an
//  existing extent which is marked as sppShelvedPool in the AE.
//
//  PARAMETERS  ppib            session to be used by the operation.
//              ifmp            database to add a shelved page to.
//              pgno            page number to be added as shelved.
//
//-
ERR ErrSPShelvePage( PIB* const ppib, const IFMP  ifmp, const PGNO pgno )
{
    Assert( pgno != pgnoNull );

    ERR err = JET_errSuccess;
    FMP* const pfmp = g_rgfmp + ifmp;
    Assert( !pfmp->FIsTempDB() );
    Assert( pfmp->FBeyondPgnoShrinkTarget( pgno ) );
    Assert( pgno <= pfmp->PgnoLast() );

    FUCB* pfucbRoot = pfucbNil;
    FUCB* pfucbAE = pfucbNil;

    Call( ErrBTIOpenAndGotoRoot( ppib, pgnoSystemRoot, ifmp, &pfucbRoot ) );
    Call( ErrSPIOpenAvailExt( pfucbRoot, &pfucbAE ) );

    Call( ErrSPIReserveSPBufPages( pfucbRoot ) );

    Call( ErrSPIAddToAvailExt( pfucbAE, pgno, 1, spp::ShelvedPool ) );
    Assert( Pcsr( pfucbAE )->FLatched() );

HandleError:
    if ( pfucbAE != pfucbNil )
    {
        BTUp( pfucbAE );
        BTClose( pfucbAE );
        pfucbAE = pfucbNil;
    }

    if ( pfucbRoot != pfucbNil )
    {
        pfucbRoot->pcsrRoot->ReleasePage();
        pfucbRoot->pcsrRoot = pcsrNil;
        BTClose( pfucbRoot );
        pfucbRoot = pfucbNil;
    }

    return err;
}


//  ErrSPUnshelveShelvedPagesBelowEof
//  ========================================================================
//  Removes all shelved pages which are below EOF.
//
//  PARAMETERS  ppib            session to be used by the operation.
//              ifmp            database to remove shelved pages from.
//
//-
ERR ErrSPUnshelveShelvedPagesBelowEof( PIB* const ppib, const IFMP ifmp )
{
    ERR err = JET_errSuccess;
    FMP* const pfmp = g_rgfmp + ifmp;
    FUCB* pfucbRoot = pfucbNil;
    BOOL fInTransaction = fFalse;
    Assert( !pfmp->FIsTempDB() );
    Expected( pfmp->FShrinkIsRunning() );

    // This is a space operation, so transactions are not relevant because
    // space operations are not versioned. However, there are some valuable asserts
    // which require a transaction, so we'll silence those asserts.
    Call( ErrDIRBeginTransaction( ppib, 46018, NO_GRBIT ) );
    fInTransaction = fTrue;

    Call( ErrBTIOpenAndGotoRoot( ppib, pgnoSystemRoot, ifmp, &pfucbRoot ) );

    Call( ErrSPIUnshelvePagesInRange( pfucbRoot, 1, pfmp->PgnoLast() ) );

HandleError:
    if ( pfucbRoot != pfucbNil )
    {
        pfucbRoot->pcsrRoot->ReleasePage();
        pfucbRoot->pcsrRoot = pcsrNil;
        BTClose( pfucbRoot );
        pfucbRoot = pfucbNil;
    }

    if ( fInTransaction )
    {
        if ( err >= JET_errSuccess )
        {
            err = ErrDIRCommitTransaction( ppib, NO_GRBIT );
            fInTransaction = ( err < JET_errSuccess );
        }

        if ( fInTransaction )
        {
            CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
            fInTransaction = fFalse;
        }
    }

    return err;
}


//  ErrSPIUnshelvePagesInRange
//  ========================================================================
//  Removes a range of shelved pages expected to be beyond EOF.
//
//  PARAMETERS  pfucbRoot       FUCB of the DB root (expected to be latched).
//              pgnoFirst       first page to unshelve.
//              pgnoLast        last page to unshelve.
//
//-
ERR ErrSPIUnshelvePagesInRange( FUCB* const pfucbRoot, const PGNO pgnoFirst, const PGNO pgnoLast )
{
    ERR err = JET_errSuccess;
    FUCB* pfucbAE = pfucbNil;

    AssertSPIPfucbOnRoot( pfucbRoot );
    Assert( ( pgnoFirst != pgnoNull ) && ( pgnoLast != pgnoNull) );
    Assert( pgnoFirst <= pgnoLast );

    FMP* const pfmp = g_rgfmp + pfucbRoot->ifmp;
    Assert( !pfmp->FIsTempDB() );

    // In the case where we are in a DB growth codepath (either regular growth or split-buffer-driven growth),
    // pfmp->PgnoLast() has been already increased and the owned space has already been added to the OE.
    // Though removing these shelved extents comes prior to adding available space to the AE, obviously.
    const PGNO pgnoLastDbRootInitial = pfmp->PgnoLast();
    PGNO pgnoLastDbRootPrev = pgnoLastDbRootInitial;
    PGNO pgnoLastDbRoot = pgnoLastDbRootInitial;

    // We should be either unshelving pages which are completely below EOF or pages which are completely above EOF:
    //
    //   1- Completely below EOF:
    //     1.a- Cleanup of orphaned shelved pages.
    //     1.b- File growth (either regular growth or split-buffer-driven growth).
    //
    //   2- Completely above EOF:
    //     2.a- Freeing space which had been previously shelved. For example, we delete
    //          a table that had leaked or available space shelved and when the extent
    //          trickles up to the root, we unshelve the space so that it could be utilized
    //          when the file re-grows.
    //
    Expected( ( pgnoLast <= pgnoLastDbRootInitial ) || ( pgnoFirst > pgnoLastDbRootInitial ) );

    PGNO pgno = pgnoFirst;

    Call( ErrSPIOpenAvailExt( pfucbRoot, &pfucbAE ) );
    while ( pgno <= pgnoLast )
    {
        pgnoLastDbRootPrev = pgnoLastDbRoot;
        Call( ErrSPIReserveSPBufPages( pfucbRoot ) );
        pgnoLastDbRoot = pfmp->PgnoLast();
        Assert( pgnoLastDbRoot >= pgnoLastDbRootPrev );

        CSPExtentInfo speiAE;
        Call( ErrSPISeekRootAE( pfucbAE, pgno, spp::ShelvedPool, &speiAE ) );
        Assert( !speiAE.FIsSet() || ( speiAE.CpgExtent() == 1 ) );

        // Any growth should have taken care of pages in this range.
        AssertTrack( !speiAE.FIsSet() ||
                     ( pgnoLastDbRoot == pgnoLastDbRootPrev ) ||
                     ( speiAE.PgnoLast() <= pgnoLastDbRootPrev ) ||
                     ( speiAE.PgnoFirst() > pgnoLastDbRoot ),
                     "UnshelvePagesNotUnshelvedByGrowth" );

        // When this function is called to unshelve pages beyond the current EOF (case 2.a above),
        // it is expected that the entire range (pgnoFirst through pgnoLast) is shelved.
        AssertTrack( ( pgnoFirst <= pgnoLastDbRootInitial ) ||              // Not case 2.a is OK.
                     ( speiAE.FIsSet() && ( pgno == speiAE.PgnoFirst() ) || // Must be a contiguous range.
                     ( pgno <= pgnoLastDbRoot ) ),                          // It's OK if the DB grew while we unshelve the range.
                     "UnshelvePagesUnexpectedMissingShelves" );

        // Check if shelved extent/page found is in the right range.
        //
        // This function is generally expected to unshelve at least one page. Known cases in which
        // it may not happen are:
        //   1- Cleanup of orphaned shelved pages below EOF.
        //   2- If the DB grows as part of reserving space for the split buffers, the growth codepath itself
        //      will unshelve some of the pages.
        //
        if ( !speiAE.FIsSet() || ( speiAE.PgnoFirst() < pgnoFirst ) || ( speiAE.PgnoLast() > pgnoLast ) )
        {
            // Bail out early.
            break;
        }

        // Delete the AE node (i.e., actually unshelve the page).
        // These shelved pages aren't counted in pfucbRoot->u.pfcb->CpgAE(), so no need to adjust cache.
        Call( ErrSPIWrappedBTFlagDelete(
                  pfucbAE,
                  fDIRNoVersion | ( pfucbRoot->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                  0,
                  0 ) );

        BTUp( pfucbAE );

        pgno = speiAE.PgnoLast() + 1;
    }

HandleError:
    if ( pfucbAE != pfucbNil )
    {
        BTUp( pfucbAE );
        BTClose( pfucbAE );
        pfucbAE = pfucbNil;
    }

    return err;
}

// CPgnoProcessed represents a PGNO that can be flagged. The extra
// flag is the highest bit of the PGNO.

class CPgnoFlagged
{
    public:
        CPgnoFlagged()
        {
            m_pgnoflagged = pgnoNull;
        }

        CPgnoFlagged( const PGNO pgno )
        {
            Assert( ( pgno & 0x80000000 ) == 0 );
            m_pgnoflagged = pgno;
        }

        CPgnoFlagged( const CPgnoFlagged& pgnoflagged )
        {
            m_pgnoflagged = pgnoflagged.m_pgnoflagged;
        }

        PGNO Pgno() const
        {
            return ( m_pgnoflagged & 0x7FFFFFFF );
        }

        BOOL FFlagged() const
        {
            return ( ( m_pgnoflagged & 0x80000000 ) != 0 );
        }

        void SetFlagged()
        {
            Expected( !FFlagged() );
            m_pgnoflagged = m_pgnoflagged | 0x80000000;
        }

    private:
        PGNO m_pgnoflagged;
};

static_assert( sizeof( CPgnoFlagged ) == sizeof( PGNO ), "SizeOfCPgnoFlaggedMustBeEqualToSizeOfPgno." );

LOCAL ERR ErrErrArrayPgnoToJetErr( const CArray<CPgnoFlagged>::ERR err )
{
    switch ( err )
    {
        case CArray<CPgnoFlagged>::ERR::errSuccess:
            return JET_errSuccess;

        case CArray<CPgnoFlagged>::ERR::errOutOfMemory:
            return ErrERRCheck( JET_errOutOfMemory );
    }

    AssertSz( fFalse, "UnknownCArray<CPgnoFlagged>::ERR: %s", (int)err );
    return ErrERRCheck( JET_errInternalError );
}

INT __cdecl PgnoShvdListCmpFunction( const CPgnoFlagged* ppgnoflagged1, const CPgnoFlagged* ppgnoflagged2 )
{
    if ( ppgnoflagged1->Pgno() > ppgnoflagged2->Pgno() )
    {
        return 1;
    }

    if ( ppgnoflagged1->Pgno() < ppgnoflagged2->Pgno() )
    {
        return -1;
    }

    return 0;
}

LOCAL ERR ErrSPILRProcessObjectSpaceOwnershipSetPgnos(
    _Inout_ CSparseBitmap* const pspbmOwned,
    _Inout_ CArray<CPgnoFlagged>* const parrShelved,
    _In_ const PGNO pgno,
    _In_ const PGNO pgnoLast,
    _In_ const BOOL fDbRoot,
    _In_ const BOOL fAvailSpace,
    _In_ const BOOL fMayAlreadyBeSet )
{
    ERR err = JET_errSuccess;

    Assert( !( fDbRoot && fMayAlreadyBeSet ) );
    Assert( !( !fDbRoot && ( !fAvailSpace != !fMayAlreadyBeSet ) ) );

    if ( pgno < 1 )
    {
        AssertTrack( fFalse, "LeakReclaimPgnoTooLow" );
        Error( ErrERRCheck( fAvailSpace ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
    }

    if ( pgno <= pgnoLast )
    {
        const size_t iBit = pgno - 1;
        BOOL fSet = fFalse;

        if ( !fMayAlreadyBeSet )
        {
            Call( ErrErrBitmapToJetErr( pspbmOwned->ErrGet( iBit, &fSet ) ) );
            if ( fSet )
            {
                AssertTrack( fFalse, "LeakReclaimPgnoDoubleBelowEof" );
                Error( ErrERRCheck( fAvailSpace ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
            }
        }

        if ( !fSet )
        {
            Call( ErrErrBitmapToJetErr( pspbmOwned->ErrSet( iBit, fTrue ) ) );
        }
    }
    else
    {
        const size_t iEntry = parrShelved->SearchBinary( (CPgnoFlagged)pgno, PgnoShvdListCmpFunction );
        if ( iEntry == CArray<CPgnoFlagged>::iEntryNotFound )
        {
            // Entry was not found.
            if ( fDbRoot && !fAvailSpace )
            {
                // We always add shelved pages in ascending order.
                if ( ( parrShelved->Size() > 0 ) &&
                     ( ( parrShelved->Entry( parrShelved->Size() - 1 ).Pgno() > pgno ) ) )
                {
                    AssertTrack( fFalse, "LeakReclaimShelvedPagesOutOfOrder" );
                    Error( ErrERRCheck( fAvailSpace ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
                }
                
                // We are processing shelved extents. Add shelved space.
                if ( ( parrShelved->Size() + 1 ) > parrShelved->Capacity() )
                {
                    Call( ErrErrArrayPgnoToJetErr( parrShelved->ErrSetCapacity( 2 * ( parrShelved->Size() + 1 ) ) ) );
                }
                Call( ErrErrArrayPgnoToJetErr( parrShelved->ErrSetEntry( parrShelved->Size(), (CPgnoFlagged)pgno ) ) );
            }
            else
            {
                // If we are processing owned space (thus, non-root space), that means
                // space is owned beyond EOF without backing shelved pages.
                AssertTrack( fFalse, "LeakReclaimUntrackedPgnoBeyondEof" );
                Error( ErrERRCheck( fAvailSpace ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
            }
        }
        else
        {
            // Shelved page was found.
            if ( ( fDbRoot && fAvailSpace ) || ( !fDbRoot && ( !fAvailSpace || fMayAlreadyBeSet ) ) )
            {
                // We are either processing root split buffers or owned space by root objects (tables).
                CPgnoFlagged pgnoflaggedFound = parrShelved->Entry( iEntry );
                if ( !pgnoflaggedFound.FFlagged() )
                {
                    // Page has not been processed yet. Mark it as processed.
                    pgnoflaggedFound.SetFlagged();
                    Call( ErrErrArrayPgnoToJetErr( parrShelved->ErrSetEntry( iEntry, pgnoflaggedFound ) ) );
                }
                else if ( !fMayAlreadyBeSet )
                {
                    // Page has already been processed. This is illegal: page twice processed beyond EOF.
                    AssertTrack( fFalse, "LeakReclaimPgnoDoubleProcBeyondEof" );
                    Error( ErrERRCheck( fAvailSpace ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
                }
            }
            else
            {
                // We are processing shelved extents. This is illegal: page twice shelved beyond EOF.
                AssertTrack( fFalse, "LeakReclaimPgnoDoubleShelvedBeyondEof" );
                Error( ErrERRCheck( fAvailSpace ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
            }
        }
    }

HandleError:
    return err;
}

LOCAL ERR ErrSPILRProcessObjectSpaceOwnership(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const OBJID objid,
    _Inout_ CSparseBitmap* const pspbmOwned,
    _Inout_ CArray<CPgnoFlagged>* const parrShelved )
{
    ERR err = JET_errSuccess;
    PGNO pgnoFDP = pgnoNull;
    PGNO pgnoFDPParentUnused = pgnoNull;
    FCB* pfcb = pfcbNil;
    FUCB* pfucb = pfucbNil;
    FUCB* pfucbParentUnused = pfucbNil;
    BOOL fSmallSpace = fFalse;
    FUCB* pfucbSpace = pfucbNil;
    const BOOL fDbRoot = ( objid == objidSystemRoot );
    const OBJID objidParent = fDbRoot ? objidNil : objidSystemRoot;
    const SYSOBJ sysobj = fDbRoot ? sysobjNil : sysobjTable ;
    const PGNO pgnoLast = g_rgfmp[ifmp].PgnoLast();

    Call( ErrCATGetCursorsFromObjid(
        ppib,
        ifmp,
        objid,
        objidParent,
        sysobj,
        &pgnoFDP,
        &pgnoFDPParentUnused,
        &pfucb,
        &pfucbParentUnused ) );

    pfcb = pfucb->u.pfcb;
    fSmallSpace = FSPIIsSmall( pfcb );

    if ( fDbRoot )
    {
        // For the DB root, we need to process all available space.
        // Note that the space used by the root B+ tree and the root space trees
        // themselves are not tracked anywhere. So, when we traverse the leaked
        // space bitmap, we'll test for that via page categorization.

        Assert( !fSmallSpace );

        // Open AE for scanning.
        Call( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbSpace ) );
    }
    else
    {
        // If not the DB root, we need to process all owned space.

        if ( !fSmallSpace )
        {
            // Open OE for scanning.
            Call( ErrSPIOpenOwnExt( ppib, pfcb, &pfucbSpace ) );
        }
        else
        {
            // Latch root.
            Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
            pfucb->pcsrRoot = Pcsr( pfucb );

            const SPACE_HEADER* const psph = PsphSPIRootPage( pfucb );
            Assert( psph->FSingleExtent() );

            // Set all pages owned by this small-space tree.
            const PGNO pgnoFirstSmallExt = PgnoFDP( pfucb );
            const PGNO pgnoLastSmallExt = pgnoFirstSmallExt + psph->CpgPrimary() - 1;
            for ( PGNO pgno = pgnoFirstSmallExt; pgno <= pgnoLastSmallExt; pgno++ )
            {
                Call( ErrSPILRProcessObjectSpaceOwnershipSetPgnos(
                            pspbmOwned,
                            parrShelved,
                            pgno,
                            pgnoLast,
                            fFalse,     // fDbRoot
                            fFalse,     // fAvailSpace
                            fFalse ) ); // fMayAlreadyBeSet
            }

            BTUp( pfucb );
            pfucb->pcsrRoot = pcsrNil;
        }
    }

    Assert( !!fSmallSpace == ( pfucbSpace == pfucbNil ) );

    if ( fSmallSpace )
    {
        // Already processed above.
        goto HandleError;
    }

    Assert( !!pfucbSpace->fOwnExt ^ !!pfucbSpace->fAvailExt );
    FUCBSetSequential( pfucbSpace );
    FUCBSetPrereadForward( pfucbSpace, cpgPrereadSequential );

    // Go through all extents.
    DIB dib;
    dib.pos = posFirst;
    dib.dirflag = fDIRNull;
    err = ErrBTDown( pfucbSpace, &dib, latchReadTouch );
    if ( err == JET_errRecordNotFound )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    Call( err );

    while ( err >= JET_errSuccess )
    {
        const CSPExtentInfo spext( pfucbSpace );
        Call( spext.ErrCheckCorrupted() );
        Assert( spext.FIsSet() );

        if ( spext.CpgExtent() > 0 )
        {
            const SpacePool spp = spext.SppPool();
            if ( !( ( pfucbSpace->fOwnExt && ( spp == spp::AvailExtLegacyGeneralPool ) ) ||
                    ( pfucbSpace->fAvailExt && ( spp == spp::AvailExtLegacyGeneralPool ) ) ||
                    ( pfucbSpace->fAvailExt && ( spp == spp::ContinuousPool ) ) ||
                    ( pfucbSpace->fAvailExt && ( spp == spp::ShelvedPool ) ) ||
                    ( !fDbRoot && ( spp == spp::ShelvedPool ) ) ) )
            {
                AssertTrack( fFalse, OSFormat( "LeakReclaimUnknownPool:%d:%d:%d", (int)fDbRoot, (int)pfucbSpace->fAvailExt, (int)spp ) );
                Error( ErrERRCheck( pfucbSpace->fAvailExt ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
            }

            if ( ( spp == spp::ShelvedPool ) && ( spext.PgnoFirst() <= pgnoLast ) && ( spext.PgnoLast() > pgnoLast ) )
            {
                AssertTrack( fFalse, "LeakReclaimOverlappingShelvedExt" );
                Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
            }

            // Do not process leftover shelved pages below EOF.
            if ( ( spp != spp::ShelvedPool ) || ( spext.PgnoFirst() > pgnoLast ) )
            {
                for ( PGNO pgno = spext.PgnoFirst(); pgno <= spext.PgnoLast(); pgno++ )
                {
                    // Shelved extents are represented in the root's AE tree, but are technically owned space.
                    const BOOL fAvailSpace = pfucbSpace->fAvailExt && ( spp != spp::ShelvedPool );
                    Call( ErrSPILRProcessObjectSpaceOwnershipSetPgnos(
                                pspbmOwned,
                                parrShelved,
                                pgno,
                                pgnoLast,
                                fDbRoot,        // fDbRoot
                                fAvailSpace,    // fAvailSpace
                                fFalse ) );     // fMayAlreadyBeSet
                }
            }
        }

        err = ErrBTNext( pfucbSpace, dib.dirflag );
        if ( err == JET_errNoCurrentRecord )
        {
            err = JET_errSuccess;
            goto HandleError;
        }
    }
    Call( err );

    // Split buffers. We need to process split buffers separately because:
    //  1- If this is the DB root, space given to split buffers do not show up as available to the root and yet is not owned
    //     by any tables.
    //  2- If this is not the DB root, there was an old (already fixed) bug where space could be assigned to a non-root split buffer,
    //     but not owned by the object, due to failures during split buffer refill. That would normally be harmless, but now that we
    //     are unleaking space, we cannot allow that case anymore.
    if ( !fSmallSpace )
    {
        BTUp( pfucbSpace );
        for ( int iSpaceTree = 1; iSpaceTree <= 2; iSpaceTree++ )
        {
            SPLIT_BUFFER* pspbuf = NULL;
            Call( ErrBTIGotoRoot( pfucbSpace, latchRIW ) );
            Call( ErrSPIGetSPBuf( pfucbSpace, &pspbuf ) );
            if ( pspbuf->CpgBuffer1() > 0 )
            {
                for ( PGNO pgno = pspbuf->PgnoFirstBuffer1(); pgno <= pspbuf->PgnoLastBuffer1(); pgno++ )
                {
                    Call( ErrSPILRProcessObjectSpaceOwnershipSetPgnos(
                                pspbmOwned,
                                parrShelved,
                                pgno,
                                pgnoLast,
                                fDbRoot,        // fDbRoot
                                fTrue,          // fAvailSpace
                                !fDbRoot ) );   // fMayAlreadyBeSet
                }
            }
            if ( pspbuf->CpgBuffer2() > 0 )
            {
                for ( PGNO pgno = pspbuf->PgnoFirstBuffer2(); pgno <= pspbuf->PgnoLastBuffer2(); pgno++ )
                {
                    Call( ErrSPILRProcessObjectSpaceOwnershipSetPgnos(
                                pspbmOwned,
                                parrShelved,
                                pgno,
                                pgnoLast,
                                fDbRoot,        // fDbRoot
                                fTrue,          // fAvailSpace
                                !fDbRoot ) );   // fMayAlreadyBeSet
                }
            }

            const BOOL fAvailTree = pfucbSpace->fAvailExt;
            BTUp( pfucbSpace );
            BTClose( pfucbSpace );
            pfucbSpace = pfucbNil;

            if ( iSpaceTree == 1 )
            {
                if ( fAvailTree )
                {
                    Call( ErrSPIOpenOwnExt( ppib, pfcb, &pfucbSpace ) );
                }
                else
                {
                    Call( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbSpace ) );
                }                
            }
        }
    }

    err = JET_errSuccess;

HandleError:
    if ( pfucb != pfucbNil )
    {
        BTUp( pfucb );
        pfucb->pcsrRoot = pcsrNil;
    }

    if ( pfucbSpace != pfucbNil )
    {
        BTUp( pfucbSpace );
        BTClose( pfucbSpace );
        pfucbSpace = pfucbNil;
    }

    CATFreeCursorsFromObjid( pfucb, pfucbParentUnused );
    pfucb = pfucbNil;
    pfucbParentUnused = pfucbNil;

    return err;
}

LOCAL ERR ErrSPILRProcessPotentiallyLeakedPage(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _Inout_ CSparseBitmap* const pspbmOwned,
    _Out_ BOOL* const pfLeaked )
{
    ERR err = JET_errSuccess;
    OBJID objid = objidNil;
    SpaceCategoryFlags spcatf = spcatfNone;
    SpaceCatCtx* pSpCatCtx = NULL;
    BOOL fNotLeaked = fFalse;

    *pfLeaked = fFalse;

    Call( ErrErrBitmapToJetErr( pspbmOwned->ErrGet( pgno - 1, &fNotLeaked ) ) );
    if ( fNotLeaked )
    {
        goto HandleError;
    }

    Call( ErrSPGetSpaceCategory(
            ppib,
            ifmp,
            pgno,
            objidSystemRoot,
            fFalse, // fRunFullSpaceCat
            &objid,
            &spcatf,
            &pSpCatCtx ) );

    // To be safe, only proceed if we get one of the expected categories.
    const BOOL fRootDb = ( ( pSpCatCtx != NULL ) && ( pSpCatCtx->pfucb != pfucbNil ) ) ?
                             ( PgnoFDP( pSpCatCtx->pfucb ) == pgnoSystemRoot ) :
                             fFalse ;
    *pfLeaked = FSPSpaceCatIndeterminate( spcatf );

    if ( !(
             // This is the leaked case.
             *pfLeaked ||
             (
                 fRootDb &&
                 (
                     // This is the root space tree case.
                     FSPSpaceCatAnySpaceTree( spcatf ) ||
                     // This is the DB's root page (pgno 1).
                     ( !FSPSpaceCatAnySpaceTree( spcatf ) && FSPSpaceCatRoot( spcatf ) && ( pgno == pgnoSystemRoot ) )
                 )
             ) ||
             (
                 // This is the non-root space tree case. It is an uncommon case, but possible due to an old (already fixed) bug
                 // where space could be assigned to a non-root split buffer, but not owned by the object, due to failures during
                 // split buffer refill.
                 !fRootDb && FSPSpaceCatAnySpaceTree( spcatf )
             )
       ) )
    {
        AssertTrack( fFalse, OSFormat( "LeakReclaimUnexpectedCat:%d:%d", (int)fRootDb, (int)spcatf ) );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    SPFreeSpaceCatCtx( &pSpCatCtx );

    if ( !( *pfLeaked ) )
    {
        Call( ErrErrBitmapToJetErr( pspbmOwned->ErrSet( pgno - 1, fTrue ) ) );
    }

HandleError:
    SPFreeSpaceCatCtx( &pSpCatCtx );
    return err;
}

// Leak Reclaimer done reason.
typedef enum
{
    lrdrNone,
    lrdrCompleted,
    lrdrFailed,
    lrdrTimeout,
    lrdrMSysObjIdsNotReady,
} LeakReclaimerDoneReason;

ERR ErrSPReclaimSpaceLeaks( PIB* const ppib, const IFMP ifmp )
{
    ERR err = JET_errSuccess;
    ERR errVerClean = JET_errSuccess;
    LeakReclaimerDoneReason lrdr = lrdrNone;

    FMP* const pfmp = g_rgfmp + ifmp;
    pfmp->SetLeakReclaimerIsRunning();

    const HRT hrtStarted = HrtHRTCount();
    const LONG dtickQuota = pfmp->DtickLeakReclaimerTimeQuota();

    BOOL fDbOpen = fFalse;
    BOOL fInTransaction = fFalse;
    PGNO pgnoFirstReclaimed = pgnoNull, pgnoLastReclaimed = pgnoNull;
    CPG cpgReclaimed = 0, cpgShelved = -1;
    PGNO pgnoLastInitial = pfmp->PgnoLast();
    PGNO pgnoMaxToProcess = pgnoNull;
    PGNO pgnoFirstShelved = pgnoNull, pgnoLastShelved = pgnoNull;
    FUCB* pfucbCatalog = pfucbNil;
    FUCB* pfucbRoot = pfucbNil;
    CSparseBitmap spbmOwned;
    CArray<CPgnoFlagged> arrShelved;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortSpace );

    Assert( !pfmp->FIsTempDB() );

    // Setup and initial checks.
    //

    IFMP ifmpDummy;
    Call( ErrDBOpenDatabase( ppib, pfmp->WszDatabaseName(), &ifmpDummy, JET_bitDbExclusive ) );
    EnforceSz( pfmp->FExclusiveBySession( ppib ), "LeakReclaimDbNotExclusive" );
    Assert( ifmpDummy == ifmp );
    fDbOpen = fTrue;

    // Reclaiming leaked space requires a fully populated MSysObjids table because
    // that is how we enumerate all the tables efficiently.
    {
    BOOL fMSysObjidsReady = fFalse;
    Call( ErrCATCheckMSObjidsReady( ppib, ifmp, &fMSysObjidsReady ) );
    if ( !fMSysObjidsReady )
    {
        lrdr = lrdrMSysObjIdsNotReady;
        err = JET_errSuccess;
        goto HandleError;
    }
    }

    // Just in case, but we don't expect version store and DB tasks to be running at this point.
    errVerClean = PverFromPpib( ppib )->ErrVERRCEClean( ifmp );
    if ( errVerClean != JET_errSuccess )
    {
        AssertTrack( fFalse, OSFormat( "LeakReclaimBeginVerCleanErr:%d", errVerClean ) );
        Error( ErrERRCheck( JET_errDatabaseInUse ) );
    }
    pfmp->WaitForTasksToComplete();

    // Check timeout expiration.
    if ( ( dtickQuota >= 0 ) && ( CmsecHRTFromHrtStart( hrtStarted ) >= (QWORD)dtickQuota ) )
    {
        lrdr = lrdrTimeout;
        err = JET_errSuccess;
        goto HandleError;
    }

    // Get last owned extent info.
    Call( ErrSPGetLastPgno( ppib, ifmp, &pgnoLastInitial ) );
    EnforceSz( pgnoLastInitial == pfmp->PgnoLast(), "LeakReclaimPgnoLastMismatch" );
    pgnoMaxToProcess = pgnoLastInitial;

    // Initialize bitmap.
    Call( ErrErrBitmapToJetErr( spbmOwned.ErrInitBitmap( pgnoLastInitial ) ) );


    // Build space bitmap and shelved page list.
    //
    // Pages below EOF will be represented in a bitmap, while shelved pages will
    // be represented in a page array. The reason for the difference is that shelved pages
    // are sparse space owned beyond EOF, so in-between space is not expected to be owned
    // and therefore should not be considered leaked space when traversing the bitmap.
    //
    // The below-EOF bitmap will be initialized with zeroes and as we detect ownership, the
    // corresponding bits will be then flipped to 1. Any remaining zeroes in the bitmap will
    // then represent leaked pages.
    //
    // For beyond-EOF, pages will be first added to the list without their highest bit set and
    // as we detect ownership, their highest bits will be then flipped to 1. Any remaining pages
    // with their highest bits unset will then represent leaked pages.
    //

    // DB root: process available space (AE tree + split buffers) and shelved pages beyond EOF.
    Call( ErrSPILRProcessObjectSpaceOwnership( ppib, ifmp, objidSystemRoot, &spbmOwned, &arrShelved ) );
    cpgShelved = arrShelved.Size();
    if ( arrShelved.Size() > 0 )
    {
        pgnoFirstShelved = arrShelved.Entry( 0 ).Pgno();
        pgnoLastShelved = arrShelved.Entry( arrShelved.Size() - 1 ).Pgno();
        Assert( pgnoFirstShelved <= pgnoLastShelved );

        Assert( pgnoLastShelved > pgnoMaxToProcess );
        pgnoMaxToProcess = pgnoLastShelved;
    }

    // Check timeout expiration.
    if ( ( dtickQuota >= 0 ) && ( CmsecHRTFromHrtStart( hrtStarted ) >= (QWORD)dtickQuota ) )
    {
        lrdr = lrdrTimeout;
        err = JET_errSuccess;
        goto HandleError;
    }

    // All tables: process space owned by the first level of B+ trees right below the root.
    {
    OBJID objid = objidNil;
    for ( err = ErrCATGetNextRootObject( ppib, ifmp, fFalse, &pfucbCatalog, &objid );
        ( err >= JET_errSuccess ) && ( objid != objidNil );
        err = ErrCATGetNextRootObject( ppib, ifmp, fFalse, &pfucbCatalog, &objid ) )
    {
        Call( ErrSPILRProcessObjectSpaceOwnership( ppib, ifmp, objid, &spbmOwned, &arrShelved ) );

        // Check timeout expiration.
        if ( ( dtickQuota >= 0 ) && ( CmsecHRTFromHrtStart( hrtStarted ) >= (QWORD)dtickQuota ) )
        {
            lrdr = lrdrTimeout;
            err = JET_errSuccess;
            goto HandleError;
        }
    }
    Call( err );
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;
    }

    // Consistency checks before moving on.
    EnforceSz( cpgShelved == arrShelved.Size(), "LeakReclaimExtraShelvedPagesBefore" );
    EnforceSz( pgnoMaxToProcess < ulMax, "LeakReclaimPgnoMaxToProcessTooHigh" );

    // Search for contiguous chunks and reclaim any leaked space.
    //

    // Go through all pages, checking extent boundaries before reclaiming leaked space.
    // At this point, the only unprocessed pages that may not be leaked are the pages which are
    // part of the root space trees themselves. Scan all unprocessed pages as we go along to test
    // for those cases in the main reclaimer loop.
    {
    size_t iEntryShelvedFound = CArray<CPgnoFlagged>::iEntryNotFound;
    for ( PGNO pgnoCurrent = 1; pgnoCurrent <= pgnoMaxToProcess; )
    {
        // Get first page of the range to reclaim on this pass.
        PGNO pgnoFirstToReclaim = pgnoNull;
        while ( ( pgnoCurrent <= pgnoMaxToProcess ) && ( pgnoFirstToReclaim == pgnoNull ) )
        {
            BOOL fLeaked = fFalse;
            if ( pgnoCurrent <= pgnoLastInitial )
            {
                // Search the bitmap.
                Assert( iEntryShelvedFound == CArray<CPgnoFlagged>::iEntryNotFound );
                Call( ErrSPILRProcessPotentiallyLeakedPage( ppib, ifmp, pgnoCurrent, &spbmOwned, &fLeaked ) );
            }
            else
            {
                // Search the shelved page list, start from last seen.
                const BOOL fFirstShelvedSeen = ( iEntryShelvedFound == CArray<CPgnoFlagged>::iEntryNotFound );
                iEntryShelvedFound = fFirstShelvedSeen ? 0 : ( iEntryShelvedFound + 1 );
                EnforceSz( arrShelved.Size() > 0, "LeakReclaimShelvedListEmpty" );
                EnforceSz( arrShelved.Size() > iEntryShelvedFound, "LeakReclaimShelvedListTooLow" );

                const CPgnoFlagged pgnoFlagged = arrShelved.Entry( iEntryShelvedFound );
                EnforceSz( ( pgnoFlagged.Pgno() >= pgnoCurrent ) || fFirstShelvedSeen, "LeakReclaimPgnoCurrentGoesBack" );
                pgnoCurrent = pgnoFlagged.Pgno();
                fLeaked = !pgnoFlagged.FFlagged();
            }

            if ( fLeaked )
            {
                pgnoFirstToReclaim = pgnoCurrent;
            }

            pgnoCurrent++;
        }

        EnforceSz( cpgShelved == arrShelved.Size(), "LeakReclaimExtraShelvedPagesFirst" );

        // No more ranges to reclaim. Bail.
        if ( pgnoFirstToReclaim == pgnoNull )
        {
            err = JET_errSuccess;
            goto HandleError;
        }

        PGNO pgnoMaxToReclaim = pgnoMaxToProcess;

        // If below the current EOF, we need to limit the search to an owned extent.
        if ( pgnoFirstToReclaim <= pfmp->PgnoLast() )
        {
            Call( ErrDIRBeginTransaction( ppib, 37218, NO_GRBIT ) );
            fInTransaction = fTrue;

            Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucbRoot ) );

            CSPExtentInfo spext;
            err = ErrSPIFindExtOE( ppib, pfucbRoot->u.pfcb, pgnoFirstToReclaim, &spext );
            if ( ( err == JET_errNoCurrentRecord ) ||
                 ( err == JET_errRecordNotFound ) ||
                 ( ( err >= JET_errSuccess ) &&
                   ( !spext.FIsSet() || ( spext.PgnoFirst() > pgnoFirstToReclaim ) ) ) )
            {
                AssertTrack( fFalse, "LeakReclaimOeGap" );
                Error( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
            }
            Call( err );
            err = JET_errSuccess;

            pgnoMaxToReclaim = UlFunctionalMin( pgnoMaxToReclaim, spext.PgnoLast() );
            EnforceSz( pgnoMaxToReclaim >= pgnoFirstToReclaim, "LeakReclaimPgnoMaxToReclaimTooLow" );

            DIRClose( pfucbRoot );
            pfucbRoot = pfucbNil;
            Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
            fInTransaction = fFalse;
        }

        EnforceSz( pgnoCurrent == ( pgnoFirstToReclaim + 1 ), "LeakReclaimPgnoCurrentMisplacedFirst" );

        // Get last page of the range to reclaim on this pass.
        PGNO pgnoLastToReclaim = pgnoFirstToReclaim;
        while ( pgnoCurrent <= pgnoMaxToReclaim )
        {
            BOOL fLeaked = fFalse;
            if ( pgnoCurrent <= pgnoLastInitial )
            {
                // Search the bitmap.
                Assert( pgnoFirstToReclaim <= pgnoLastInitial );
                Assert( iEntryShelvedFound == CArray<CPgnoFlagged>::iEntryNotFound );
                Call( ErrSPILRProcessPotentiallyLeakedPage( ppib, ifmp, pgnoCurrent, &spbmOwned, &fLeaked ) );
            }
            else
            {
                // Search the shelved page list, start from last seen.
                Assert( pgnoFirstToReclaim > pgnoLastInitial );
                Assert( iEntryShelvedFound != CArray<CPgnoFlagged>::iEntryNotFound );
                const size_t iEntryShelvedFoundT = ( iEntryShelvedFound == CArray<CPgnoFlagged>::iEntryNotFound ) ? 0 : ( iEntryShelvedFound + 1 );
                if ( arrShelved.Size() > iEntryShelvedFoundT )
                {
                    const CPgnoFlagged pgnoFlagged = arrShelved.Entry( iEntryShelvedFoundT );
                    fLeaked = ( ( pgnoFlagged.Pgno() == ( pgnoLastToReclaim + 1 ) ) && !pgnoFlagged.FFlagged() );
                    if ( fLeaked )
                    {
                        Assert( pgnoCurrent == pgnoFlagged.Pgno() );
                        iEntryShelvedFound = iEntryShelvedFoundT;
                    }
                }
                else
                {
                    fLeaked = fFalse;
                }
            }

            if ( fLeaked )
            {
                Assert( pgnoLastToReclaim == ( pgnoCurrent - 1 ) );
                pgnoLastToReclaim = pgnoCurrent;
                pgnoCurrent++;
            }
            else
            {
                break;
            }
        }

        // Some consistency checks.
        //

        EnforceSz( cpgShelved == arrShelved.Size(), "LeakReclaimExtraShelvedPagesLast" );
        EnforceSz( pgnoCurrent == ( pgnoLastToReclaim + 1 ), "LeakReclaimPgnoCurrentMisplacedLast" );
        EnforceSz( pgnoFirstToReclaim >= 1, "LeakReclaimPgnoFirstToReclaimTooLow" );
        EnforceSz( pgnoLastToReclaim >= 1, "LeakReclaimPgnoLastToReclaimTooLow" );
        EnforceSz( pgnoFirstToReclaim <= pgnoLastToReclaim, "LeakReclaimInvalidReclaimRange" );
        EnforceSz( pgnoLastToReclaim <= pgnoMaxToProcess, "LeakReclaimPgnoLastToReclaimTooHighMaxToProcess" );
        EnforceSz( pgnoLastToReclaim <= pgnoMaxToReclaim, "LeakReclaimPgnoLastToReclaimTooHighMaxToReclaim" );
        EnforceSz( ( ( pgnoFirstToReclaim <= pgnoLastInitial ) && ( pgnoLastToReclaim <= pgnoLastInitial ) ) ||
                   ( ( pgnoFirstToReclaim > pgnoLastInitial ) && ( pgnoLastToReclaim > pgnoLastInitial ) ),
                   "LeakReclaimRangeOverlapsInitialPgnoLast" );

        // Enforce that the entire range must have been detected as leaked.
        for ( PGNO pgnoToReclaim = pgnoFirstToReclaim; pgnoToReclaim <= pgnoLastToReclaim; pgnoToReclaim++ )
        {
            BOOL fNotLeaked = fTrue;
            if ( pgnoToReclaim <= pgnoLastInitial )
            {
                Call( ErrErrBitmapToJetErr( spbmOwned.ErrGet( pgnoToReclaim - 1, &fNotLeaked ) ) );
            }
            else
            {
                const size_t iEntry = arrShelved.SearchBinary( (CPgnoFlagged)pgnoToReclaim, PgnoShvdListCmpFunction );
                if ( iEntry == CArray<CPgnoFlagged>::iEntryNotFound )
                {
                    fNotLeaked = fTrue;
                }
                else
                {
                    const CPgnoFlagged pgnoflaggedFound = arrShelved.Entry( iEntry );
                    fNotLeaked = pgnoflaggedFound.FFlagged();
                }
            }

            EnforceSz( !fNotLeaked, "LeakReclaimRangeNotFullyLeaked" );
        }


        // Go ahead and reclaim the space.
        //

        const CPG cpgToReclaim = pgnoLastToReclaim - pgnoFirstToReclaim + 1;

        Call( ErrDIRBeginTransaction( ppib, 51150, NO_GRBIT ) );
        fInTransaction = fTrue;
        Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucbRoot ) );
        const PGNO pgnoLastToReclaimBelowEof = UlFunctionalMin( pgnoLastToReclaim, pgnoLastInitial );
        if ( pgnoFirstToReclaim <= pgnoLastInitial )
        {
            Assert( pgnoLastToReclaimBelowEof >= pgnoFirstToReclaim );
            Call( ErrSPCaptureSnapshot( pfucbRoot, pgnoFirstToReclaim, pgnoLastToReclaimBelowEof - pgnoFirstToReclaim + 1, fFalse ) );
        }
        Call( ErrSPFreeExt( pfucbRoot, pgnoFirstToReclaim, cpgToReclaim, "LeakReclaimer" ) );
        cpgReclaimed += cpgToReclaim;
        if ( pgnoFirstReclaimed == pgnoNull )
        {
            pgnoFirstReclaimed = pgnoFirstToReclaim;
        }
        pgnoLastReclaimed = pgnoLastToReclaim;
        DIRClose( pfucbRoot );
        pfucbRoot = pfucbNil;
        Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
        fInTransaction = fFalse;

        // Mark reclaimed pages as supercold, just in case they were cached to determine ownership.
        for ( PGNO pgno = pgnoFirstToReclaim; pgno <= pgnoLastToReclaimBelowEof; pgno++ )
        {
            BFMarkAsSuperCold( ifmp, pgno );
        }

        // Check timeout expiration.
        if ( ( dtickQuota >= 0 ) && ( CmsecHRTFromHrtStart( hrtStarted ) >= (QWORD)dtickQuota ) )
        {
            lrdr = lrdrTimeout;
            err = JET_errSuccess;
            goto HandleError;
        }
    }
    }

    err = JET_errSuccess;

HandleError:

    // Determine exit reason.
    //

    if ( lrdr == lrdrNone )
    {
        lrdr = ( err >= JET_errSuccess ? lrdrCompleted : lrdrFailed );
    }


    // Cleanup.
    //

    if ( pfucbCatalog != pfucbNil )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }

    if ( pfucbRoot != pfucbNil )
    {
        DIRClose( pfucbRoot );
        pfucbRoot = pfucbNil;
    }

    // WARNING: most (if not all) of the above is done without versioning, so there
    // really isn't any rollback of the update.
    if ( fInTransaction )
    {
        if ( err >= JET_errSuccess )
        {
            fInTransaction = ( ErrDIRCommitTransaction( ppib, NO_GRBIT ) < JET_errSuccess );
        }

        if ( fInTransaction )
        {
            CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
            fInTransaction = fFalse;
        }
    }

    // Purge all FCBs to restore pristine attach state.
    // We don't expect any pending version store entries, but just in case, avoid purging the FCBs for the
    // database in that case. The potential issue is that clients that may try to change template tables
    // will get an error because the FCB has been opened (i.e., produced an FUCB) by the categorization code.
    errVerClean = PverFromPpib( ppib )->ErrVERRCEClean( ifmp );
    if ( errVerClean != JET_errSuccess )
    {
        AssertTrack( fFalse, OSFormat( "LeakReclaimEndVerCleanErr:%d", errVerClean ) );
        if ( errVerClean > JET_errSuccess )
        {
            errVerClean = ErrERRCheck( JET_errDatabaseInUse );
        }
    }
    else
    {
        pfmp->WaitForTasksToComplete();
        FCB::PurgeDatabase( ifmp, fFalse /* fTerminating */ );
    }

    Assert( errVerClean <= JET_errSuccess );
    if ( ( err >= JET_errSuccess ) && ( errVerClean < JET_errSuccess ) )
    {
        err = errVerClean;
    }

    if ( fDbOpen )
    {
        Assert( pfmp->FExclusiveBySession( ppib ) );
        CallS( ErrDBCloseDatabase( ppib, ifmp, NO_GRBIT ) );
        fDbOpen = fFalse;
    }

    pfmp->ResetLeakReclaimerIsRunning();


    // Issue event.
    //

    OSTraceSuspendGC();
    const HRT dhrtElapsed = DhrtHRTElapsedFromHrtStart( hrtStarted );
    const double dblSecTotalElapsed = DblHRTSecondsElapsed( dhrtElapsed );
    const DWORD dwMinElapsed = (DWORD)( dblSecTotalElapsed / 60.0 );
    const double dblSecElapsed = dblSecTotalElapsed - (double)dwMinElapsed * 60.0;
    const WCHAR* rgwsz[] =
    {
        pfmp->WszDatabaseName(),
        OSFormatW( L"%d", (int)lrdr ),
        OSFormatW( L"%I32u", dwMinElapsed ), OSFormatW( L"%.2f", dblSecElapsed ),
        OSFormatW( L"%d", err ),
        OSFormatW( L"%I64u", pfmp->CbOfCpg( (CPG)pgnoLastInitial ) ), OSFormatW( L"%I32d", (CPG)pgnoLastInitial ),
        OSFormatW( L"%I64u", pfmp->CbOfCpg( (CPG)pfmp->PgnoLast() ) ), OSFormatW( L"%I32d", (CPG)pfmp->PgnoLast() ),
        OSFormatW( L"%I32d", cpgReclaimed ),
        ( pgnoFirstReclaimed != pgnoNull ) ? OSFormatW( L"%I32u", pgnoFirstReclaimed ) : L"-",
        ( pgnoLastReclaimed != pgnoNull ) ? OSFormatW( L"%I32u", pgnoLastReclaimed ) : L"-",
        ( cpgShelved != -1 ) ? OSFormatW( L"%I32d", cpgShelved ) : L"-",
        ( pgnoFirstShelved != pgnoNull ) ? OSFormatW( L"%I32u", pgnoFirstShelved ) : L"-",
        ( pgnoLastShelved != pgnoNull ) ? OSFormatW( L"%I32u", pgnoLastShelved ) : L"-",
    };
    UtilReportEvent(
        ( err < JET_errSuccess ) ? eventError : eventInformation,
        GENERAL_CATEGORY,
        ( err < JET_errSuccess ) ? DB_LEAK_RECLAIMER_FAILED_ID : DB_LEAK_RECLAIMER_SUCCEEDED_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        pfmp->Pinst() );
    OSTraceResumeGC();

    return err;
}


const ULONG cOEListEntriesInit  = 32;
const ULONG cOEListEntriesMax   = 127;

class OWNEXT_LIST
{
    public:
        OWNEXT_LIST( OWNEXT_LIST **ppOEListHead );
        ~OWNEXT_LIST();

    public:
        EXTENTINFO      *RgExtentInfo()         { return m_extentinfo; }
        ULONG           CEntries() const;
        OWNEXT_LIST     *POEListNext() const    { return m_pOEListNext; }
        VOID            AddExtentInfoEntry( const PGNO pgnoLast, const CPG cpgSize );

    private:
        EXTENTINFO      m_extentinfo[cOEListEntriesMax];
        ULONG           m_centries;
        OWNEXT_LIST     *m_pOEListNext;
};

INLINE OWNEXT_LIST::OWNEXT_LIST( OWNEXT_LIST **ppOEListHead )
{
    m_centries = 0;
    m_pOEListNext = *ppOEListHead;
    *ppOEListHead = this;
}

INLINE ULONG OWNEXT_LIST::CEntries() const
{
    Assert( m_centries <= cOEListEntriesMax );
    return m_centries;
}

INLINE VOID OWNEXT_LIST::AddExtentInfoEntry(
    const PGNO  pgnoLast,
    const CPG   cpgSize )
{
    Assert( m_centries < cOEListEntriesMax );
    m_extentinfo[m_centries].pgnoLastInExtent = pgnoLast;
    m_extentinfo[m_centries].cpgExtent = cpgSize;
    m_centries++;
}

INLINE ERR ErrSPIFreeOwnedExtentsInList(
    FUCB        *pfucbParent,
    EXTENTINFO  *rgextinfo,
    const ULONG cExtents,
    const BOOL  fFDPRevertable )
{
    ERR         err;

    AssertSPIPfucbNullOrUnlatched( pfucbParent );

    for ( size_t i = 0; i < cExtents; i++ )
    {
        const CPG   cpgSize = rgextinfo[i].CpgExtent();
        const PGNO  pgnoFirst = rgextinfo[i].PgnoLast() - cpgSize + 1;

        Assert( !FFUCBSpace( pfucbParent ) );
        CallR( ErrSPCaptureSnapshot( pfucbParent, pgnoFirst, cpgSize, !fFDPRevertable ) );
        CallR( ErrSPFreeExt( pfucbParent, pgnoFirst, cpgSize, "FreeFdpLarge" ) );
    }

    AssertSPIPfucbNullOrUnlatched( pfucbParent );

    return JET_errSuccess;
}


LOCAL ERR ErrSPIFreeAllOwnedExtents( FUCB* pfucbParent, FCB* pfcb, const BOOL fPreservePrimaryExtent, const BOOL fRevertableFDP )
{
    ERR         err;
    FUCB        *pfucbOE;
    DIB         dib;
    PGNO        pgnoLastPrev;

    ULONG       cExtents    = 0;
    CPG         cpgOwned    = 0;

    Assert( pfcb != pfcbNil );

    AssertSPIPfucbNullOrUnlatched( pfucbParent );

    //  open owned extent tree of freed FDP
    //  free each extent in owned extent to parent FDP.
    //
    CallR( ErrSPIOpenOwnExt( pfucbParent->ppib, pfcb, &pfucbOE ) );

    dib.pos = posFirst;
    dib.dirflag = fDIRNull;
    if ( ( err = ErrBTDown( pfucbOE, &dib, latchReadTouch ) ) < 0 )
    {
        BTClose( pfucbOE );
        return err;
    }
    Assert( wrnNDFoundLess != err );
    Assert( wrnNDFoundGreater != err );

    EXTENTINFO  extinfo[ cOEListEntriesInit ];
    OWNEXT_LIST *pOEList = NULL;
    OWNEXT_LIST *pOEListCurr = NULL;
    ULONG       cOEListEntries = 0;

    //  Collect all Own extent and free them all at once.
    //  Note that the pages kept tracked by own extent tree contains own
    //  extend tree itself. We can not free it while scanning own extent
    //  tree since we could free the pages used by own extend let other
    //  thread to use the pages.

    //  UNDONE: Because we free it all at once, if we crash, we may lose space.
    //  UNDONE: we need logical logging to log the remove all extent is going on
    //  UNDONE: and remember its state so that during recovery it will be able
    //  UNDONE: to redo the clean up.

    pgnoLastPrev = 0;

    do
    {
        const CSPExtentInfo     spOwnedExt( pfucbOE );
        CallS( spOwnedExt.ErrCheckCorrupted() );    // we only asserted before.

        if ( pgnoLastPrev >  ( spOwnedExt.PgnoFirst() - 1 ) )
        {
            //  nodes not in ascending order, something is gravely wrong
            //
            AssertSzRTL( fFalse, "OwnExt nodes are not in monotonically-increasing key order." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbOE ), HaDbFailureTagCorruption, L"eb31e6ec-553f-4ac9-b2f6-2561ae325954" );
            Call( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
        }

        pgnoLastPrev = spOwnedExt.PgnoLast();

        cExtents++;
        cpgOwned += spOwnedExt.CpgExtent();

        if ( !fPreservePrimaryExtent
            || ( pfcb->PgnoFDP() != spOwnedExt.PgnoFirst() ) )
        {
            // Can't coalesce this OwnExt with previous OwnExt because
            // we may cross OwnExt boundaries in the parent.

            if ( cOEListEntries < cOEListEntriesInit )
            {
                // This entry can fit in the initial EXTENTINFO structure
                // (the one that was allocated on the stack).
                extinfo[cOEListEntries].pgnoLastInExtent = spOwnedExt.PgnoLast();
                extinfo[cOEListEntries].cpgExtent = spOwnedExt.CpgExtent();
            }
            else
            {
                Assert( ( NULL == pOEListCurr && NULL == pOEList )
                    || ( NULL != pOEListCurr && NULL != pOEList ) );
                if ( NULL == pOEListCurr || pOEListCurr->CEntries() == cOEListEntriesMax )
                {
                    pOEListCurr = (OWNEXT_LIST *)PvOSMemoryHeapAlloc( sizeof( OWNEXT_LIST ) );
                    if ( NULL == pOEListCurr )
                    {
                        Assert( pfucbNil != pfucbOE );
                        BTClose( pfucbOE );
                        err = ErrERRCheck( JET_errOutOfMemory );
                        goto HandleError;
                    }
                    new( pOEListCurr ) OWNEXT_LIST( &pOEList );

                    Assert( pOEList == pOEListCurr );
                }

                pOEListCurr->AddExtentInfoEntry( spOwnedExt.PgnoLast(), spOwnedExt.CpgExtent() );
            }

            cOEListEntries++;
        }

        err = ErrBTNext( pfucbOE, fDIRNull );
    }
    while ( err >= 0 );

    OSTraceFMP(
        pfucbOE->ifmp,
        JET_tracetagSpaceInternal,
        OSFormat(
            "%hs: freeing all space %sowned_pages=%08d owned_extents=%08d from [0x%x:0x%x:%lu] to [0x%x:0x%x:%lu].",
            __FUNCTION__,
            fPreservePrimaryExtent ? "but primary " : "",
            cpgOwned,
            cExtents,
            pfucbOE->ifmp,
            ObjidFDP( pfucbOE ),
            PgnoFDP( pfucbOE ),
            pfucbParent->ifmp,
            ObjidFDP( pfucbParent ),
            PgnoFDP( pfucbParent ) ) );

    //  Close the pfucbOE right away to release any latch on the pages that
    //  are going to be freed and used by others.

    Assert( pfucbNil != pfucbOE );
    BTClose( pfucbOE );

    //  Check the error code.

    if ( err != JET_errNoCurrentRecord )
    {
        Assert( err < 0 );
        goto HandleError;
    }

    Call( ErrSPIFreeOwnedExtentsInList(
            pfucbParent,
            extinfo,
            min( cOEListEntries, cOEListEntriesInit ),
            fRevertableFDP ) );

    for ( pOEListCurr = pOEList;
        pOEListCurr != NULL;
        pOEListCurr = pOEListCurr->POEListNext() )
    {
        Assert( cOEListEntries > cOEListEntriesInit );
        Call( ErrSPIFreeOwnedExtentsInList(
                pfucbParent,
                pOEListCurr->RgExtentInfo(),
                pOEListCurr->CEntries(),
                fRevertableFDP ) );
    }

    if ( fRevertableFDP )
    {
        PERFOpt( cSPDeletedTreeSnapshottedPages.Add( PinstFromPfucb( pfucbParent ), cpgOwned ) );

        if ( pfcb->FTypeTable() && cpgOwned > UlParam( PinstFromPfucb( pfucbParent ), JET_paramFlight_RBSLargeRevertableDeletePages ) )
        {
            OSTraceSuspendGC();
            WCHAR wszTableName[JET_cbNameMost+1] = L"";

            if ( pfcb->Ptdb() != NULL && pfcb->Ptdb()->SzTableName() != NULL )
            {
                OSStrCbFormatW( wszTableName, sizeof(wszTableName), L"%hs", pfcb->Ptdb()->SzTableName() );
            }

            const WCHAR* rgcwsz[] =
            {
                wszTableName,
                OSFormatW( L"%I32u", pfcb->PgnoFDP() ),
                OSFormatW( L"%I32u", pfcb->ObjidFDP() ),
                OSFormatW( L"%I32u", cpgOwned ),
                OSFormatW( L"%I32u", (ULONG)UlParam( PinstFromPfucb( pfucbParent ), JET_paramFlight_RBSLargeRevertableDeletePages ) ),
            };

            UtilReportEvent(
                eventInformation,
                GENERAL_CATEGORY,
                RBS_LARGE_REVERTABLE_DELETE_ID,
                _countof( rgcwsz ),
                rgcwsz,
                0,
                NULL,
                PinstFromPfucb( pfucbParent ) );

            OSTraceResumeGC();
        }
    }

    PERFOpt( cSPDeletedTreeFreedPages.Add( PinstFromPfucb( pfucbParent ), cpgOwned ) );
    PERFOpt( cSPDeletedTreeFreedExtents.Add( PinstFromPfucb( pfucbParent ), cExtents ) );

HandleError:
    AssertSPIPfucbNullOrUnlatched( pfucbParent );

    pOEListCurr = pOEList;
    while ( pOEListCurr != NULL )
    {
        OWNEXT_LIST *pOEListKill = pOEListCurr;

#ifdef DEBUG
        //  this variable is no longer used, so we can
        //  re-use it for DEBUG-only purposes
        Assert( cOEListEntries > cOEListEntriesInit );
        Assert( cOEListEntries > pOEListCurr->CEntries() );
        cOEListEntries -= pOEListCurr->CEntries();
#endif

        pOEListCurr = pOEListCurr->POEListNext();

        OSMemoryHeapFree( pOEListKill );
    }
    Assert( cOEListEntries <= cOEListEntriesInit );

    return err;
}


LOCAL ERR ErrSPIReportAEsFreedWithFDP( PIB * const ppib, FCB * const pfcb )
{
    ERR         err;
    FUCB        *pfucbAE;
    DIB         dib;
    ULONG       cExtents    = 0;
    CPG         cpgFree     = 0;

    Assert( pfcb != pfcbNil );

    //  open owned extent tree of freed FDP
    //  free each extent in owned extent to parent FDP.
    //
    CallR( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbAE ) );

    dib.pos = posFirst;
    dib.dirflag = fDIRNull;
    err = ErrBTDown( pfucbAE, &dib, latchReadTouch );
    Assert( wrnNDFoundLess != err );
    Assert( wrnNDFoundGreater != err );
    if ( err == JET_errRecordNotFound )
    {
        //  This is not a big deal, its possible to have an empty AE tree even for the temp DB.
        OSTraceFMP(
            pfucbAE->ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Free FDP with 0 avail pages [0x%x:0x%x:%lu].",
                __FUNCTION__,
                pfucbAE->ifmp,
                ObjidFDP( pfucbAE ),
                PgnoFDP( pfucbAE ) ) );
        err = JET_errSuccess;
        goto HandleError;
    }

    do
    {
        Call( err );

        const CSPExtentInfo     spavailext( pfucbAE );

        cExtents++;
        cpgFree += spavailext.CpgExtent();

        err = ErrBTNext( pfucbAE, fDIRNull );
    }
    while ( JET_errNoCurrentRecord != err );

    err = JET_errSuccess;

    OSTraceFMP(
        pfucbAE->ifmp,
        JET_tracetagSpaceInternal,
        OSFormat(
            "%hs: Free FDP with %08d avail pages and %08u avail extents [0x%x:0x%x:%lu].",
            __FUNCTION__,
            cpgFree,
            cExtents,
            pfucbAE->ifmp,
            ObjidFDP( pfucbAE ),
            PgnoFDP( pfucbAE ) ) );

HandleError:
    Assert( pfucbNil != pfucbAE );
    BTClose( pfucbAE );

    return err;
}


//  ErrSPFreeFDP
//  ========================================================================
//  ERR ErrSPFreeFDP( FUCB *pfucbParent, PGNO pgnoFDPFreed )
//
//  Frees all owned extents of an FDP to its parent FDP.  The FDP page is freed
//  with the owned extents to the parent FDP.
//
//  PARAMETERS  pfucbParent     cursor on tree space is freed to
//              pgnoFDPFreed    pgnoFDP of FDP to be freed
//
ERR ErrSPFreeFDP(
    PIB         *ppib,
    FCB         *pfcbFDPToFree,
    const PGNO  pgnoFDPParent,
    const BOOL  fPreservePrimaryExtent,
    const BOOL  fRevertableFDP,
    const PGNO  pgnoLVRoot )
{
    ERR         err;
    const IFMP  ifmp            = pfcbFDPToFree->Ifmp();
    const PGNO  pgnoFDPFree     = pfcbFDPToFree->PgnoFDP();
    FUCB        *pfucbParent    = pfucbNil;
    FUCB        *pfucb          = pfucbNil;
    CPG         cpgRootCaptured = 0;
    BOOL        fBeginTrx   = fFalse;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = pfcbFDPToFree->TCE( fTrue );
    tcScope->SetDwEngineObjid( pfcbFDPToFree->ObjidFDP() );
    tcScope->iorReason.SetIort( iortSpace );

    // It's possible we are redeleting a reverted deleted FDP. Skip throwing error when we try to read root page of such a table.
    pfcbFDPToFree->SetPpibAllowRBSFDPDeleteRead( ppib );

    // Capture table root page in RBS with special flags to indicate that the only action allowed on the table after revert is delete.
    if ( !fRevertableFDP )
    {
        Call( ErrSPCaptureNonRevertableFDPRootPage( ppib, pfcbFDPToFree, pgnoLVRoot, &cpgRootCaptured ) );
    }

    //  begin transaction if one is already not begun
    //
    if ( ppib->Level() == 0 )
    {
        CallR( ErrDIRBeginTransaction( ppib, 47141, NO_GRBIT ) );
        fBeginTrx   = fTrue;
    }

    Assert( pgnoNull != pgnoFDPParent );
    Assert( pgnoNull != pgnoFDPFree );

    Assert( !FFMPIsTempDB( ifmp ) || pgnoSystemRoot == pgnoFDPParent );

    Call( ErrBTOpen( ppib, pgnoFDPParent, ifmp, &pfucbParent ) );
    Assert( pfucbNil != pfucbParent );
    Assert( pfucbParent->u.pfcb->FInitialized() );

    //  check for valid parameters.
    //
#ifdef SPACECHECK
    CallS( ErrSPIValidFDP( ppib, pfucbParent->ifmp, pgnoFDPFree ) );
#endif

    OSTraceFMP(
        pfucbParent->ifmp,
        JET_tracetagSpaceManagement,
        OSFormat(
            "%hs: freeing all space %sfrom [0x%x:0x%x:%lu] to [0x%x:0x%x:%lu].",
            __FUNCTION__,
            fPreservePrimaryExtent ? "but primary " : "",
            pfucbParent->ifmp,
            pfcbFDPToFree->ObjidFDP(),
            pfcbFDPToFree->PgnoFDP(),
            pfucbParent->ifmp,
            ObjidFDP( pfucbParent ),
            PgnoFDP( pfucbParent ) ) );

    //  get temporary FUCB
    //
    Call( ErrBTOpen( ppib, pfcbFDPToFree, &pfucb ) );
    Assert( pfucbNil != pfucb );
    Assert( pfucb->u.pfcb->FInitialized() );
    Assert( pfucb->u.pfcb->FDeleteCommitted() );
    FUCBSetIndex( pfucb );

    Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
    pfucb->pcsrRoot = Pcsr( pfucb );

#ifdef SPACECHECK
    CallS( ErrSPIWasAlloc( pfucb, pgnoFDPFree, 1 ) );
#endif

    //  get parent FDP pgno
    //
    Assert( pgnoFDPParent == PgnoSPIParentFDP( pfucb ) );
    Assert( pgnoFDPParent == PgnoFDP( pfucbParent ) );

    if ( !pfucb->u.pfcb->FSpaceInitialized() )
    {
        SPIInitFCB( pfucb, fTrue );
    }

    // We expect this to fail to find the FCB in the cache if we're deleting it.
    Assert( fPreservePrimaryExtent || ( JET_errSuccess != ErrCATExtentPageCountsCached( pfucb ) ) );

    //  if single extent format, then free extent in external header
    //
    if ( FSPIIsSmall( pfucb->u.pfcb ) )
    {
        if ( !fPreservePrimaryExtent )
        {
            AssertSPIPfucbOnRoot( pfucb );

            //  get external header
            //
            NDGetExternalHeader( pfucb, pfucb->pcsrRoot, noderfSpaceHeader );
            Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
            const SPACE_HEADER * const psph = reinterpret_cast <SPACE_HEADER *> ( pfucb->kdfCurr.data.Pv() );

            ULONG cpgPrimary = psph->CpgPrimary();
            Assert( psph->CpgPrimary() != 0 );

            //  Close the cursor to make sure it latches no buffer whose page
            //  is going to be freed.

            pfucb->pcsrRoot = pcsrNil;
            BTClose( pfucb );
            pfucb = pfucbNil;

            Assert( !FFUCBSpace( pfucbParent ) );

            Call( ErrSPCaptureSnapshot( pfucbParent, pgnoFDPFree, cpgPrimary, !fRevertableFDP ) );

            if ( fRevertableFDP )
            {
                PERFOpt( cSPDeletedTreeSnapshottedPages.Add( PinstFromPfucb( pfucbParent ), cpgPrimary ) );
            }
            else
            {
                // Just snapshotted the FDP root page.
                PERFOpt( cSPDeletedTreeSnapshottedPages.Add( PinstFromPfucb( pfucbParent ), cpgRootCaptured ) );
            }

            Call( ErrSPFreeExt( pfucbParent, pgnoFDPFree, cpgPrimary, "FreeFdpSmall" ) );
            PERFOpt( cSPDeletedTreeFreedPages.Add( PinstFromPfucb( pfucbParent ), cpgPrimary ) );
            PERFOpt( cSPDeletedTreeFreedExtents.Inc( PinstFromPfucb( pfucbParent ) ) );
        }
    }
    else
    {
        //  Close the cursor to make sure it latches no buffer whose page
        //  is going to be freed.

        FCB *pfcb = pfucb->u.pfcb;
        pfucb->pcsrRoot = pcsrNil;
        BTClose( pfucb );
        pfucb = pfucbNil;

        CPG cpgSnapshotted = 0;

        //  This function call could be a non-trivial amount of effort just to report a 
        //  small piece of data ... so we will use the trace tag its traced under to 
        //  calling this function.
        if ( FOSTraceTagEnabled( JET_tracetagSpaceInternal ) &&
            FFMPIsTempDB( pfucbParent->ifmp ) )
        {
            Call( ErrSPIReportAEsFreedWithFDP( ppib, pfcb ) );
        }

        if ( !fRevertableFDP )
        {
            Call( ErrSPCaptureSpaceTreePages( pfucbParent, pfcb, &cpgSnapshotted ) );

            // Number of space tree pages snapshotted + FDP root page
            PERFOpt( cSPDeletedTreeSnapshottedPages.Add( PinstFromPfucb( pfucbParent ), cpgSnapshotted + cpgRootCaptured ) );
        }

        Call( ErrSPIFreeAllOwnedExtents( pfucbParent, pfcb, fPreservePrimaryExtent, fRevertableFDP ) );

        Assert( !Pcsr( pfucbParent )->FLatched() );
    }

    PERFOpt( cSPDeletedTrees.Inc( PinstFromPfucb( pfucbParent ) ) );

HandleError:
    pfcbFDPToFree->ResetPpibAllowRBSFDPDeleteRead();

    if ( pfucbNil != pfucb )
    {
        pfucb->pcsrRoot = pcsrNil;
        BTClose( pfucb );
    }

    if ( pfucbNil != pfucbParent )
    {
        BTClose( pfucbParent );
    }

    if ( fBeginTrx )
    {
        if ( err >= 0 )
        {
            err = ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush );
        }
        if ( err < 0 )
        {
            CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
        }
    }

#ifdef DEBUG
    if ( !FSPExpectedError( err ) )
    {
        CallS( err );
        AssertSz( fFalse, ( !FFMPIsTempDB( ifmp ) ?
                                "Space potentially lost permanently in user database." :
                                "Space potentially lost in temporary database." ) );
    }
#endif

    return err;
}

INLINE ERR ErrSPIAddExtent(
    __inout FUCB *pfucb,
    _In_ const CSPExtentNodeKDF * const pcspextnode )
{
    ERR         err;
    CPG         cpgOEDelta;
    CPG         cpgAEDelta;

    Assert( FFUCBSpace( pfucb ) );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( pcspextnode->CpgExtent() > 0 );

    //  Insist valid data before we insert it into the DB.
    Assert( pcspextnode->FValid() );

    const KEY   key     = pcspextnode->GetKey();
    const DATA  data    = pcspextnode->GetData();

    BTUp( pfucb );

    if ( pcspextnode->SppPool() == spp::ShelvedPool )
    {
        // We don't count shelved extents, although we could.
        cpgOEDelta = 0;
        cpgAEDelta = 0;
    }
    else if ( pfucb->fOwnExt )
    {
        cpgOEDelta = pcspextnode->CpgExtent();
        cpgAEDelta = 0;
    }
    else
    {
        Assert( pfucb->fAvailExt );
        cpgOEDelta = 0;
        cpgAEDelta = pcspextnode->CpgExtent();
    }

    Call( ErrSPIWrappedBTInsert(
                pfucb,
                key,
                data,
                fDIRNoVersion | ( pfucb->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                cpgOEDelta,
                cpgAEDelta ) );

    Assert( Pcsr( pfucb )->FLatched() );

    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagSpaceManagement,
        OSFormat(
            "%hs: Added %hs E(%lu + %lu) P(%ws) to [0x%x:0x%x:%lu].",
            __FUNCTION__,
            SzSpaceTreeType( pfucb ),
            pcspextnode->PgnoFirst(),
            pcspextnode->CpgExtent(),
            WszPoolName( pcspextnode->SppPool() ),
            pfucb->ifmp,
            ObjidFDP( pfucb ),
            PgnoFDP( pfucb ) ) );

HandleError:
    Assert( errSPOutOfOwnExtCacheSpace != err );
    Assert( errSPOutOfAvailExtCacheSpace != err );
    return err;
}

LOCAL ERR ErrSPIAddToAvailExt(
    _In_    FUCB *      pfucbAE,
    _In_    const PGNO  pgnoAELast,
    _In_    const CPG   cpgAESize,
    _In_    SpacePool   sppPool )
{
    ERR         err;
    FCB         * const pfcb    = pfucbAE->u.pfcb;

    const CSPExtentNodeKDF  cspaei( SPEXTKEY::fSPExtentTypeAE, pgnoAELast, cpgAESize, sppPool );

    Assert( FFUCBAvailExt( pfucbAE ) );
    err = ErrSPIAddExtent( pfucbAE, &cspaei );
    if ( err < 0 )
    {
        if ( JET_errKeyDuplicate == err )
        {
            AssertSz( fFalse, "This is a bad thing, please show to ESE developers - likely explanation is the code around the ErrBTReplace() in ErrSPIAEFreeExt() is not taking care of insertion regions." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"917f5355-a040-4919-9503-81a9bdbcaa16" );
            err = ErrERRCheck( JET_errSPAvailExtCorrupted );
        }
    }

    else if ( ( sppPool != spp::ShelvedPool ) &&
              ( cspaei.CpgExtent() >= cpageSEDefault ) &&
              ( pgnoNull != pfcb->PgnoNextAvailSE() ) &&
              ( cspaei.PgnoFirst() < pfcb->PgnoNextAvailSE() ) )
    {
        pfcb->SetPgnoNextAvailSE( cspaei.PgnoFirst() );
    }

    return err;
}

LOCAL ERR ErrSPIAddToOwnExt(
    FUCB        *pfucb,
    const PGNO  pgnoOELast,
    CPG         cpgOESize,
    CPG         *pcpgCoalesced )
{
    ERR         err;
    FUCB        *pfucbOE;

    //  open cursor on owned extent
    //
    CallR( ErrSPIOpenOwnExt( pfucb, &pfucbOE ) );
    Assert( FFUCBOwnExt( pfucbOE ) );

    //  coalescing OWNEXT is done only for temp database,
    //  otherwise we end up creating huge OwnExt's in which space never
    //  gets freed to the parent
    //
    //  Note 1: we can not coalesce for recoverable dbs , 
    //  not even at the DB level we use to do becuase of
    //  the following scenario
    //  - we delete the last / only node in the tree and log it
    //  - we crash before the insert is logged
    //  - recovery will replay the delete BUT becuase it is NOT versioned
    //  it will NOT undo the delete. 
    //  - the OwnExt tree will be left with no nodes
    //  (one attempt to fix this would during recovery to defer the delete
    //  until the insert of the new space is seen)
    // 
    //  Note 2: we could coalesce for non-recoverable DBS other then temp
    //  but we choose not to do it because we might get multiple kinds of
    //  space trees (like if a defrag database will have the first part 
    // 
    //  
    // 
    if ( NULL != pcpgCoalesced && FFMPIsTempDB( pfucb->ifmp ) )
    {
        DIB         dib;

        //  set up variables for coalescing
        //
        const CSPExtentKeyBM    spPgnoBeforeFirst( SPEXTKEY::fSPExtentTypeOE, pgnoOELast - cpgOESize );

        //  look for extent that ends at pgnoOELast - cpgOESize,
        //  the only extent we can coalesce with
        //
        dib.pos     = posDown;
        dib.pbm     = spPgnoBeforeFirst.Pbm( pfucbOE );
        dib.dirflag = fDIRExact;
        err = ErrBTDown( pfucbOE, &dib, latchReadTouch );
        if ( JET_errRecordNotFound == err )
        {
            err = JET_errSuccess;
        }
        else if ( JET_errSuccess == err )
        {
            //  we found a match, so get the old extent's size, delete the old extent,
            //  and add it's size to the new extent to insert
            //
            CSPExtentInfo       spextToCoallesce( pfucbOE );

            Assert( spextToCoallesce.PgnoLast() == pgnoOELast - cpgOESize );

            err = spextToCoallesce.ErrCheckCorrupted();
            if ( err < JET_errSuccess )
            {
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbOE ), HaDbFailureTagCorruption, L"4441dbd5-f6ff-4314-9315-efe96362f0a2" );
            }
            Call( err );

            Assert( spextToCoallesce.CpgExtent() > 0 );

            Call( ErrSPIWrappedBTFlagDelete(      // UNDONE: Synchronously remove the node
                        pfucbOE,
                        fDIRNoVersion | ( pfucbOE->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                        -( spextToCoallesce.CpgExtent() ),
                        0 ) );

            Assert( NULL != pcpgCoalesced );
            *pcpgCoalesced = spextToCoallesce.CpgExtent();

            cpgOESize += spextToCoallesce.CpgExtent();
        }
        else
        {
            Call( err );
        }

        BTUp( pfucbOE );
    }

    if( pgnoSystemRoot == pfucb->u.pfcb->PgnoFDP() )
    {
        QWORD cbFsFileSize = 0;
        if ( g_rgfmp[ pfucb->ifmp ].Pfapi()->ErrSize( &cbFsFileSize, IFileAPI::filesizeLogical ) >= JET_errSuccess )
        {
            AssertTrack( CbFileSizeOfPgnoLast( pgnoOELast ) <= cbFsFileSize, "RootPgnoOeLastBeyondEof" );
        }
    }

    {
    const CSPExtentNodeKDF  cspextnode( SPEXTKEY::fSPExtentTypeOE, pgnoOELast, cpgOESize );

    Call( ErrSPIAddExtent( pfucbOE, &cspextnode ) );
    }

HandleError:
    Assert( errSPOutOfOwnExtCacheSpace != err );
    Assert( errSPOutOfAvailExtCacheSpace != err );
    pfucbOE->pcsrRoot = pcsrNil;
    BTClose( pfucbOE );
    return err;
}

LOCAL ERR ErrSPICoalesceAvailExt(
    FUCB        *pfucbAE,
    const PGNO  pgnoLast,
    const CPG   cpgSize,
    CPG         *pcpgCoalesce )
{
    ERR         err;
    DIB         dib;

    //  coalescing AVAILEXT is done only for temp database
    //
    Assert( FFMPIsTempDB( pfucbAE->ifmp ) );

    *pcpgCoalesce = 0;

    //  Set up seek key to Avail Size
    //
    CSPExtentKeyBM spavailextSeek( SPEXTKEY::fSPExtentTypeAE, pgnoLast - cpgSize, spp::AvailExtLegacyGeneralPool );

    //  look for extent that ends at pgnoLast - cpgSize,
    //  the only extent we can coalesce with
    //
    dib.pos     = posDown;
    dib.pbm     = spavailextSeek.Pbm( pfucbAE );
    dib.dirflag = fDIRNull;
    err = ErrBTDown( pfucbAE, &dib, latchReadTouch );
    if ( JET_errRecordNotFound == err )
    {
        //  no record in available extent
        //  mask error
        err = JET_errSuccess;
    }
    else if ( JET_errSuccess == err )
    {
        //  we found a match, so get the old extent's size, delete the old extent,
        //  and add it's size to the new extent to insert
        //
        const CSPExtentInfo     spavailext( pfucbAE );
#ifdef DEBUG
        Assert( spavailext.PgnoLast() == pgnoLast - cpgSize );
#endif

        err = spavailext.ErrCheckCorrupted();
        if ( err < JET_errSuccess )
        {
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"9f63b812-97ed-46ce-b40d-3a05a844351b" );
        }
        Call( err );

        *pcpgCoalesce = spavailext.CpgExtent();

        Call( ErrSPIWrappedBTFlagDelete(      // UNDONE: Synchronously remove the node
                    pfucbAE,
                    fDIRNoVersion | ( pfucbAE->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    0,
                    -(*pcpgCoalesce) ) );
    }

HandleError:

    BTUp( pfucbAE );

    return err;
}


//  if Secondary extent, add given extent owned extent and available extent
//  if Freed extent, add extent to available extent
//  splits caused during insertion into owned extent and available extent will
//      use space from FUCB space cache, which is initialized here
//  pufcbAE is cursor on available extent tree, should be positioned on
//      added available extent node
//
LOCAL ERR ErrSPIAddSecondaryExtent(
    _In_ FUCB* const     pfucb,
    _In_ FUCB* const     pfucbAE,
    _In_ const PGNO      pgnoLast,
    _In_ CPG             cpgNewSpace,
    _In_ CPG             cpgAvailable,
    _In_ CArray<EXTENTINFO>* const parreiReleased,
    _In_ const SpacePool sppPool )
{
    ERR err;
    CPG cpgOECoalesced  = 0;
    const BOOL fRootDB = ( pfucb->u.pfcb->PgnoFDP() == pgnoSystemRoot );
    BOOL fAddedToOwnExt = fFalse;
    BOOL fAddedToAvailExt = fFalse;

    Assert( cpgNewSpace > 0 );
    Assert( cpgAvailable > 0 );

    Assert( ( cpgNewSpace == cpgAvailable ) ||
            ( fRootDB && ( cpgNewSpace > cpgAvailable ) ) );
    Assert( ( parreiReleased == NULL ) || ( parreiReleased->Size() == 0 ) ||
            ( fRootDB && ( cpgNewSpace > cpgAvailable ) ) );
    Assert( ( cpgNewSpace == cpgAvailable ) ||
            ( ( cpgNewSpace > cpgAvailable ) && !g_rgfmp[ pfucb->ifmp ].FIsTempDB() ) );

    Assert( sppPool != spp::ShelvedPool );

    Assert( !Pcsr( pfucbAE )->FLatched() );

    //  If this is a secondary extent, insert new extent into OWNEXT and
    //  AVAILEXT, coalescing with an existing extent to the left, if possible.
    //  Note we won't add to OWNEXT or coalesce if this is a shelved extent.
    //
    Call( ErrSPIAddToOwnExt(
                pfucb,
                pgnoLast,
                cpgNewSpace,
                &cpgOECoalesced ) );
    fAddedToOwnExt = fTrue;

    if ( fRootDB )
    {
        g_rgfmp[ pfucb->ifmp ].SetOwnedFileSize( CbFileSizeOfPgnoLast( pgnoLast ) );
    }

    //  We shouldn't even try coalescing AvailExt if no coalescing of OwnExt was done
    //  (since we cannot coalesce AvailExt across OwnExt boundaries)
    if ( cpgOECoalesced > 0 )
    {
        // Currently, we only coalesce OwnExt nodes for the temp DB, which always
        // uses the general pool. Coalescing of other pools is generally handled
        // as special cases, so if this assert goes off some day, you'll need to make
        // a decision about how coalescing is supposed to be handled for that specific
        // pool (and possibly pass the pool to ErrSPICoalesceAvailExt() below).
        Assert( g_rgfmp[ pfucb->ifmp ].Dbid() == dbidTemp );
        Expected( sppPool == spp::AvailExtLegacyGeneralPool );
        if ( sppPool == spp::AvailExtLegacyGeneralPool )
        {
            CPG cpgAECoalesced;

            Call( ErrSPICoalesceAvailExt( pfucbAE, pgnoLast, cpgAvailable, &cpgAECoalesced ) );

            // Ensure AvailExt wasn't coalesced across OwnExt boundaries.
            Assert( cpgAECoalesced <= cpgOECoalesced );
            cpgAvailable += cpgAECoalesced;
            Assert( cpgAvailable > 0 );
        }
    }

    Assert( !Pcsr( pfucbAE )->FLatched() );

    //  Release the extra chunks of available space.
    //
    if ( ( parreiReleased != NULL ) && ( parreiReleased->Size() > 0 ) )
    {
        Assert( !Pcsr( pfucbAE )->FLatched() );

        while ( parreiReleased->Size() > 0 )
        {
            const EXTENTINFO& extinfoReleased = parreiReleased->Entry( parreiReleased->Size() - 1 );
            Assert( extinfoReleased.FValid() && ( extinfoReleased.CpgExtent() > 0 ) );
            Call( ErrSPIAEFreeExt( pfucb, extinfoReleased.PgnoFirst(), extinfoReleased.CpgExtent() ) );
            CallS( ( parreiReleased->ErrSetSize( parreiReleased->Size() - 1 ) == CArray<EXTENTINFO>::ERR::errSuccess ) ?
                                                                                 JET_errSuccess :
                                                                                 ErrERRCheck( JET_errOutOfMemory ) );
        }
        Assert( !Pcsr( pfucbAE )->FLatched() );

        Call( ErrSPIReserveSPBufPages( pfucb ) );
        Assert( !Pcsr( pfucbAE )->FLatched() );
    }

    //  Unshelve any pages which are in the range of the new root extent.
    //
    if ( fRootDB && ( cpgNewSpace > cpgAvailable ) )
    {
        Assert( !g_rgfmp[ pfucb->ifmp ].FIsTempDB() );
        Assert( !Pcsr( pfucbAE )->FLatched() );

        Call( ErrSPIUnshelvePagesInRange(
                pfucb,
                pgnoLast - cpgNewSpace + 1,
                pgnoLast - cpgAvailable ) );
        Assert( !Pcsr( pfucbAE )->FLatched() );

        Call( ErrSPIReserveSPBufPages( pfucb ) );
        Assert( !Pcsr( pfucbAE )->FLatched() );
    }

    Call( ErrSPIAddToAvailExt( pfucbAE, pgnoLast, cpgAvailable, sppPool ) );
    fAddedToAvailExt = fTrue;
    Assert( Pcsr( pfucbAE )->FLatched() );

HandleError:
    Assert( ( err < JET_errSuccess ) || ( fAddedToOwnExt && fAddedToAvailExt ) );
    if ( fAddedToOwnExt && !fAddedToAvailExt )
    {
        SPIReportSpaceLeak( pfucb, err, pgnoLast - cpgAvailable + 1, cpgAvailable, "NewExt" );
    }

    return err;
}


INLINE ERR ErrSPICheckSmallFDP( FUCB *pfucb, BOOL *pfSmallFDP )
{
    ERR     err;
    FUCB    *pfucbOE    = pfucbNil;
    CPG     cpgOwned    = 0;
    DIB     dib;

    CallR( ErrSPIOpenOwnExt( pfucb, &pfucbOE ) );
    Assert( pfucbNil != pfucbOE );

    //  determine if this FDP owns a lot of space [> cpgSmallFDP]
    //
    dib.pos = posFirst;
    dib.dirflag = fDIRNull;
    err = ErrBTDown( pfucbOE, &dib, latchReadTouch );
    Assert( err != JET_errNoCurrentRecord );
    Assert( err != JET_errRecordNotFound );

    //  assume small FDP unless proven otherwise
    //
    *pfSmallFDP = fTrue;

    // Count pages until we reach the end or hit short-circuit value (cpgSmallFDP).
    do
    {
        //  process navigation error
        //
        Call( err );

        const CSPExtentInfo     spextOwnedCurr( pfucbOE );

        cpgOwned += spextOwnedCurr.CpgExtent();
        if ( cpgOwned > cpgSmallFDP )
        {
            *pfSmallFDP = fFalse;
            break;
        }

        err = ErrBTNext( pfucbOE, fDIRNull );
    }
    while ( JET_errNoCurrentRecord != err );

    err = JET_errSuccess;

HandleError:
    Assert( pfucbNil != pfucbOE );
    BTClose( pfucbOE );
    return err;
}


LOCAL ERR ErrSPINewSize(
    const IFMP  ifmp,
    const PGNO  pgnoLastCurr,
    const CPG   cpgReq, // cpgReq can be negative, which implies a shrink request.
    const CPG   cpgAsyncExtension )
{
    ERR err = JET_errSuccess;
    BOOL fUpdateLgposResizeHdr = fFalse;
    LGPOS lgposResize = lgposMin;
    TraceContextScope tcScope;

    OnDebug( g_rgfmp[ ifmp ].AssertSafeToChangeOwnedSize() );
    Assert( ( cpgReq <= 0 ) || !g_rgfmp[ ifmp ].FBeyondPgnoShrinkTarget( pgnoLastCurr + 1, cpgReq ) );
    Assert( ( cpgReq <= 0 ) || ( pgnoLastCurr <= g_rgfmp[ ifmp ].PgnoLast() ) );
    Expected( cpgAsyncExtension >= 0 );

    //  If this is a shrink operation, a few extra steps are necessary.

    if ( cpgReq < 0 )
    {
        Call( ErrBFPreparePageRangeForExternalZeroing( ifmp, pgnoLastCurr + cpgReq + 1, -1 * cpgReq, *tcScope ) );
    }

    //  Log the operation to indicate that resizing the database file is about to be attempted.

    if ( g_rgfmp[ ifmp ].FLogOn() )
    {
        LOG* const plog = PinstFromIfmp( ifmp )->m_plog;
        if ( cpgReq >= 0 )
        {
            Call( ErrLGExtendDatabase( plog, ifmp, pgnoLastCurr + cpgReq, &lgposResize ) );
        }
        else
        {
            // Write out any snapshot for pages about to be shrunk
            if ( g_rgfmp[ifmp].FRBSOn() )
            {
                // Capture all shrunk pages as if they need to be reverted to empty pages when RBS is applied.
                // If we already captured a preimage for one of those shrunk pages, the revert to an empty page will be ignored for that page when we apply the snapshot.
                Call( g_rgfmp[ ifmp ].PRBS()->ErrCaptureEmptyPages( g_rgfmp[ ifmp ].Dbid(), pgnoLastCurr + cpgReq + 1, -1 * cpgReq, 0 ) );
                Call( g_rgfmp[ifmp].PRBS()->ErrFlushAll() );
            }

            Call( ErrLGShrinkDatabase( plog, ifmp, pgnoLastCurr + cpgReq, -1 * cpgReq, &lgposResize ) );
        }

        fUpdateLgposResizeHdr = ( g_rgfmp[ ifmp ].ErrDBFormatFeatureEnabled( JET_efvLgposLastResize ) == JET_errSuccess );
    }

    Call( ErrIOResizeUpdateDbHdrCount( ifmp, ( cpgReq >= 0 ) /* fExtend */ ) );

    //  Resize the actual DB file.

    Call( ErrIONewSize(
            ifmp,
            *tcScope,
            pgnoLastCurr + cpgReq,
            cpgAsyncExtension,
            ( cpgReq >= 0 ) ? JET_bitResizeDatabaseOnlyGrow : JET_bitResizeDatabaseOnlyShrink ) );

    //  Ensure all FS metadata for the resized file is on disk (before we update our space trees - probably, right after this function).

    Call( ErrIOFlushDatabaseFileBuffers( ifmp, iofrDbResize ) );

    //  Update the header with lgpos of this resize.

    if ( fUpdateLgposResizeHdr )
    {
        Assert( CmpLgpos( lgposResize, lgposMin ) != 0 );
        Call( ErrIOResizeUpdateDbHdrLgposLast( ifmp, lgposResize ) );
    }

HandleError:
    OSTraceFMP(
        ifmp,
        JET_tracetagSpaceManagement,
        OSFormat(
            "%hs: Request to resize database=['%ws':0x%x] by %I64d bytes to %I64u bytes (and an additional %I64d bytes asynchronously) completed with error %d (0x%x)",
            __FUNCTION__,
            g_rgfmp[ ifmp ].WszDatabaseName(),
            ifmp,
            (__int64)cpgReq * g_cbPage,
            QWORD( pgnoLastCurr + cpgReq ) * g_cbPage,
            (__int64)cpgAsyncExtension * g_cbPage,
            err,
            err ) );

    return err;
}

// Unconditionally writes zeroes to the specified region of the database.
// Writes synchronously in blocks of cbZero
LOCAL ERR ErrSPIWriteZeroesDatabase(
    _In_ const IFMP ifmp,
    _In_ const QWORD ibOffsetStart,
    _In_ const QWORD cbZeroes,
    _In_ const TraceContext& tc )
{
    Assert( 0 != cbZeroes );

    ERR err = JET_errSuccess;
    DWORD_PTR dwRangeLockContext = NULL;
    const PGNO pgnoStart = PgnoOfOffset( ibOffsetStart );
    const PGNO pgnoEnd = PgnoOfOffset( ibOffsetStart + cbZeroes ) - 1;

    Assert( 0 == ( cbZeroes % ( g_cbPage ) ) );
    Assert( pgnoStart <= pgnoEnd );
    Assert( pgnoStart >= PgnoOfOffset( cpgDBReserved * g_cbPage ) );
    Assert( pgnoStart != pgnoNull );
    Assert( pgnoEnd != pgnoNull );
    Assert( ibOffsetStart == OffsetOfPgno( pgnoStart ) );
    Assert( ( ibOffsetStart + cbZeroes ) == OffsetOfPgno( pgnoEnd + 1 ) );

    FMP* const pfmp = &g_rgfmp[ ifmp ];
    const CPG cpg = ( pgnoEnd - pgnoStart ) + 1;
    TraceContextScope tcScope( iorpSPDatabaseInlineZero, iorsNone, iortSpace );

    OSTraceFMP(
        ifmp,
        JET_tracetagSpaceManagement,
        OSFormat(
            "%hs: Zeroing %I64d k at %#I64x (pages %lu through %lu).",
            __FUNCTION__,
            cbZeroes / 1024,
            ibOffsetStart,
            pgnoStart,
            pgnoEnd ) );


    CPG cpgZeroOptimal = CpgBFGetOptimalLockPageRangeSizeForExternalZeroing( ifmp );
    cpgZeroOptimal = LFunctionalMin( cpgZeroOptimal, pfmp->CpgOfCb( ::g_cbZero ) );
    PGNO pgnoZeroRangeThis = pgnoStart;
    while ( pgnoZeroRangeThis <= pgnoEnd )
    {
        const CPG cpgZeroRangeThis = LFunctionalMin( cpgZeroOptimal, (CPG)( pgnoEnd - pgnoZeroRangeThis + 1 ) );

        dwRangeLockContext = NULL;
        Call( ErrBFLockPageRangeForExternalZeroing( ifmp, pgnoZeroRangeThis, cpgZeroRangeThis, fTrue, *tcScope, &dwRangeLockContext ) );

        Call( pfmp->Pfapi()->ErrIOWrite(
            *tcScope,
            OffsetOfPgno( pgnoZeroRangeThis ),
            (DWORD)pfmp->CbOfCpg( cpgZeroRangeThis ),
            g_rgbZero,
            qosIONormal | ( ( UlParam( PinstFromIfmp( ifmp ), JET_paramFlight_NewQueueOptions ) & bitUseMetedQ ) ? qosIODispatchWriteMeted : 0x0 ) ) );

        BFPurgeLockedPageRangeForExternalZeroing( dwRangeLockContext, *tcScope );

        BFUnlockPageRangeForExternalZeroing( dwRangeLockContext, *tcScope );
        dwRangeLockContext = NULL;

        pgnoZeroRangeThis += cpgZeroRangeThis;
    }

HandleError:
    BFUnlockPageRangeForExternalZeroing( dwRangeLockContext, *tcScope );
    return err;
}

ERR ErrSPITrimUpdateDatabaseHeader( const IFMP ifmp )
{
    ERR err     = JET_errSuccess;
    FMP *pfmp   = &g_rgfmp[ ifmp ];

    Assert( pfmp->Pdbfilehdr() );

    if ( NULL != pfmp->Pdbfilehdr() )   // for insurance
    {
        BOOL fUpdateHeader = fTrue;

    { // .ctor acquires PdbfilehdrReadWrite
        PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();

        pdbfilehdr->le_ulTrimCount++;

#ifndef DEBUG
        // Force-update the header the first couple of times, because for
        // debugging purposes we usually care about whether the database
        // has ever been trimmed; the exact count is less important.
        // Besides, it will be written out when the log generation rolls.
        fUpdateHeader = ( pdbfilehdr->le_ulTrimCount <= 2 );
#endif
    } // .dtor releases PdbfilehdrReadWrite

        if ( fUpdateHeader )
        {
            Call( ErrUtilWriteAttachedDatabaseHeaders( PinstFromIfmp( ifmp ), PinstFromIfmp( ifmp )->m_pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ) );
        }
    }

HandleError:
    return err;
}

LOCAL VOID SPILogEventOversizedDb(
    const MessageId msgid,
    FUCB* const     pfucbRoot,
    const PGNO      pgnoLastInitial,
    const PGNO      pgnoLastFinal,
    const PGNO      pgnoMaxDbSize,
    const CPG       cpgMinReq,
    const CPG       cpgPrefReq,
    const CPG       cpgMinReqAdj,
    const CPG       cpgPrefReqAdj )
{
    ERR err = JET_errSuccess;
    FUCB *pfucbAE = pfucbNil;
    const FMP* const pfmp = &g_rgfmp[ pfucbRoot->ifmp ];
    const ULONGLONG cbInitialSize = CbFileSizeOfPgnoLast( pgnoLastInitial );
    const ULONGLONG cbSizeLimit = CbFileSizeOfPgnoLast( pgnoMaxDbSize );
    const ULONGLONG cbFinalSize = CbFileSizeOfPgnoLast( pgnoLastFinal );
    CSPExtentInfo speiAE;
    CPG cpgExtSmallest = lMax;
    CPG cpgExtLargest = 0;
    ULONG cext = 0;
    CPG cpgAvail = 0;

    // Collect available space stats.
    Assert( pfucbRoot->u.pfcb->PgnoFDP() == pgnoSystemRoot );
    AssertSPIPfucbOnRoot( pfucbRoot );
    Call( ErrSPIOpenAvailExt( pfucbRoot, &pfucbAE ) );
    Call( ErrSPISeekRootAE( pfucbAE, 1, spp::AvailExtLegacyGeneralPool, &speiAE ) );
    while ( speiAE.FIsSet() )
    {
        Assert( speiAE.SppPool() == spp::AvailExtLegacyGeneralPool );
        Assert( speiAE.PgnoLast() <= pgnoLastInitial );

        const CPG cpg = speiAE.CpgExtent();
        Assert( cpg > 0 );

        cpgExtSmallest = LFunctionalMin( cpgExtSmallest, cpg );
        cpgExtLargest = LFunctionalMax( cpgExtLargest, cpg );
        cext++;
        cpgAvail += cpg;

        speiAE.Unset();
        err = ErrBTNext( pfucbAE, fDIRNull );
        if ( err >= JET_errSuccess )
        {
            speiAE.Set( pfucbAE );
            if ( speiAE.SppPool() != spp::AvailExtLegacyGeneralPool )
            {
                speiAE.Unset();
            }
        }
        else if ( err == JET_errNoCurrentRecord )
        {
            err = JET_errSuccess;
        }
        Call( err );
    }

HandleError:
    if ( pfucbAE != pfucbNil )
    {
        BTClose( pfucbAE );
        pfucbAE = pfucbNil;
    }

    if ( ( err < JET_errSuccess ) || ( cext == 0 ) )
    {
        cpgExtSmallest = 0;
        cpgExtLargest = 0;
        cext = 0;
        cpgAvail = 0;
    }

    OSTraceSuspendGC();
    const WCHAR* rgwsz[] =
    {
        pfmp->WszDatabaseName(),
        OSFormatW( L"%I64u", cbInitialSize ), OSFormatW( L"%d", pfmp->CpgOfCb( cbInitialSize ) ),
        OSFormatW( L"%I64u", cbSizeLimit ), OSFormatW( L"%d", pfmp->CpgOfCb( cbSizeLimit ) ),
        OSFormatW( L"%I64u", cbFinalSize ), OSFormatW( L"%d", pfmp->CpgOfCb( cbFinalSize ) ),
        OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgMinReq ) ), OSFormatW( L"%d", cpgMinReq ),
        OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgPrefReq ) ), OSFormatW( L"%d", cpgPrefReq ),
        OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgMinReqAdj ) ), OSFormatW( L"%d", cpgMinReqAdj ),
        OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgPrefReqAdj ) ), OSFormatW( L"%d", cpgPrefReqAdj ),
        OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgExtSmallest ) ), OSFormatW( L"%d", cpgExtSmallest ),
        OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgExtLargest ) ), OSFormatW( L"%d", cpgExtLargest ),
        OSFormatW( L"%I32u", cext ),
        OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgAvail ) ), OSFormatW( L"%d", cpgAvail )
    };
    UtilReportEvent(
        eventWarning,
        SPACE_MANAGER_CATEGORY,
        msgid,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        pfmp->Pinst() );
    OSTraceResumeGC();
}

LOCAL ERR ErrSPIExtendDB(
    FUCB        *pfucbRoot,
    const CPG   cpgSEMin,
    CPG         *pcpgSEReq,
    PGNO        *ppgnoSELast,
    const BOOL  fPermitAsyncExtension,
    const BOOL  fMayViolateMaxSize,
    CArray<EXTENTINFO>* const parreiReleased,
    CPG         *pcpgSEAvail )
{
    ERR         err                 = JET_errSuccess;
    PGNO        pgnoSEMaxAdj        = pgnoNull;
    CPG         cpgSEMaxAdj         = 0;
    PGNO        pgnoSELast          = pgnoNull;
    PGNO        pgnoSELastAdj       = pgnoNull;
    CPG         cpgAdj              = 0;
    CPG         cpgSEReq            = *pcpgSEReq;
    CPG         cpgSEReqAdj         = 0;
    CPG         cpgSEMinAdj         = 0;
    FUCB        *pfucbOE            = pfucbNil;
    FUCB        *pfucbAE            = pfucbNil;
    CPG         cpgAsyncExtension   = 0;
    DIB         dib;

    CSPExtentInfo speiAEShelved;

    Assert( cpgSEMin > 0 );
    Assert( cpgSEReq > 0 );
    Assert( cpgSEReq >= cpgSEMin );
    Assert( parreiReleased != NULL );
    Assert( pgnoSystemRoot == pfucbRoot->u.pfcb->PgnoFDP() );
    Assert( !g_rgfmp[ pfucbRoot->ifmp ].FReadOnlyAttach() );

    // This effectively acts as a lock on this DB extending path..
    AssertSPIPfucbOnRoot( pfucbRoot );

    // If Shrink is running, signal it to bail and let the database grow IFF
    // if we can violate the max DB size constraint, which is a proxy for when
    // not doing so would lead to space leaks.
    if ( g_rgfmp[pfucbRoot->ifmp].FShrinkIsActive() )
    {
        if ( fMayViolateMaxSize )
        {
            g_rgfmp[pfucbRoot->ifmp].ResetPgnoShrinkTarget();
        }
        else
        {
            Error( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ) );
        }
    }

    Call( ErrSPIOpenOwnExt( pfucbRoot, &pfucbOE ) );

    dib.pos = posLast;
    dib.dirflag = fDIRNull;

    Call( ErrBTDown( pfucbOE, &dib, latchReadTouch ) );
    CallS( ErrSPIExtentLastPgno( pfucbOE, &pgnoSELast ) );
    pgnoSELastAdj = pgnoSELast;
    BTUp( pfucbOE );

    pgnoSEMaxAdj = pgnoSysMax;
    if ( g_rgfmp[ pfucbRoot->ifmp ].CpgDatabaseSizeMax() > 0 )
    {
        pgnoSEMaxAdj = min( pgnoSEMaxAdj, (PGNO)g_rgfmp[ pfucbRoot->ifmp ].CpgDatabaseSizeMax() );
    }

    if ( pgnoSEMaxAdj >= pgnoSELast )
    {
        cpgSEMaxAdj = pgnoSEMaxAdj - pgnoSELast;
    }

    //  Here, we need to be aware of shelved pages ahead. The contract of this function is 
    //  to return an extent (i.e., a pgnoLast:cpg pair) that is fully available for consumption.
    //  Therefore, we need to make sure that the range returned does not have any shelved pages.
    //  We are going to look for a range that contains at least cpgSEMin in-between shelved
    //  pages and we'll go from there. We'll try and keep the same cpgSEReq that was passed in
    //  to increase the chances of keeping it aligned with the DB extension size. Note that we
    //  also need to return any in-between segments which need to be made available so that the
    //  callers can release them as available space.
    //
    Call( ErrSPIOpenAvailExt( pfucbRoot, &pfucbAE ) );
    Call( ErrSPISeekRootAE( pfucbAE, pgnoSELastAdj + 1, spp::ShelvedPool, &speiAEShelved ) );
    Assert( !speiAEShelved.FIsSet() || !g_rgfmp[ pfucbRoot->ifmp ].FIsTempDB() );
    while ( ( ( pgnoSELastAdj + cpgSEMin ) <= pgnoSysMax ) && speiAEShelved.FIsSet() )
    {
        Assert( speiAEShelved.PgnoFirst() > pgnoSELastAdj );
        const CPG cpgAvail = (CPG)( speiAEShelved.PgnoFirst() - pgnoSELastAdj - 1 );
        if ( cpgAvail >= cpgSEMin )
        {
            cpgSEMaxAdj = min( cpgSEMaxAdj, cpgAdj + cpgAvail );
            break;
        }

        // We are skipping a chunk of pages because the extent is not sufficient to
        // fulfill the request. We need to add this to the release list so that the caller
        // can make these pages available for later consumption.
        if ( cpgAvail > 0 )
        {
            EXTENTINFO extinfoReleased;
            extinfoReleased.pgnoLastInExtent = pgnoSELastAdj + cpgAvail;
            extinfoReleased.cpgExtent = cpgAvail;
            Call( ( parreiReleased->ErrSetEntry( parreiReleased->Size(), extinfoReleased ) == CArray<EXTENTINFO>::ERR::errSuccess ) ?
                                                                                              JET_errSuccess :
                                                                                              ErrERRCheck( JET_errOutOfMemory ) );
        }

        pgnoSELastAdj = speiAEShelved.PgnoLast();
        cpgAdj = pgnoSELastAdj - pgnoSELast;

        speiAEShelved.Unset();
        err = ErrBTNext( pfucbAE, fDIRNull );
        if ( err >= JET_errSuccess )
        {
            speiAEShelved.Set( pfucbAE );
            if ( speiAEShelved.SppPool() != spp::ShelvedPool )
            {
                speiAEShelved.Unset();
            }
        }
        else if ( err == JET_errNoCurrentRecord )
        {
            err = JET_errSuccess;
        }
        Call( err );
    }
    BTUp( pfucbAE );
    BTClose( pfucbAE );
    pfucbAE = pfucbNil;

    // We must not have left a potentially relevant shelved extent behind.
    AssertTrack( !speiAEShelved.FIsSet() ||
                 ( ( speiAEShelved.PgnoFirst() > pgnoSELastAdj ) &&
                   ( (CPG)( speiAEShelved.PgnoFirst() - pgnoSELastAdj - 1 ) >= cpgSEMin ) ), "ExtendDbUnprocessedShelvedExt" );

    Assert( cpgSEMaxAdj >= 0 );
    Assert( cpgAdj >= 0 );

    cpgSEMinAdj = cpgSEMin + cpgAdj;
    cpgSEReqAdj = max( cpgSEMinAdj, min( cpgSEMinAdj + ( cpgSEReq - cpgSEMin ), cpgSEMaxAdj ) );
    Assert( cpgSEMinAdj >= cpgSEMin );
    Assert( cpgSEReqAdj >= cpgSEMinAdj );

    // Check for violation of max size.
    // Currently, there are two codepaths that can extend a database: split buffer refill and
    // extent allocation from space requests trickling up. The split buffer path must succeed,
    // even if it violates the max DB size, because not doing so may cause space to leak, as
    // replenished split buffers are a requirement to adding new space. If we are even violating
    // pgnoSysMax, we'll need to fail out and leak, as pgnos >= pgnoSysMax are not supported
    // in the engine.
    if ( ( ( pgnoSELastAdj + cpgSEMin ) > pgnoSEMaxAdj ) &&
         ( !fMayViolateMaxSize || ( pgnoSEMaxAdj >= pgnoSysMax ) ) )
    {
        AssertTrack( !fMayViolateMaxSize, "ExtendDbMaxSizeBeyondPgnoSysMax" );

        SPILogEventOversizedDb(
            SPACE_MAX_DB_SIZE_REACHED_ID,
            pfucbRoot,
            pgnoSELast,
            pgnoSELast,
            pgnoSEMaxAdj,
            cpgSEMin,
            cpgSEReq,
            cpgSEMinAdj,
            cpgSEReqAdj );

        Error( ErrERRCheck( JET_errOutOfDatabaseSpace ) );
    }

    if ( fPermitAsyncExtension && ( ( pgnoSELast + cpgSEReqAdj + cpgSEReq ) <= pgnoSEMaxAdj ) )
    {
        cpgAsyncExtension = cpgSEReq;
    }

    // Issue event if we violated the max DB size.
    if ( ( pgnoSELast + cpgSEReqAdj ) > pgnoSEMaxAdj )
    {
        Assert( pgnoSEMaxAdj < pgnoSysMax );
        SPILogEventOversizedDb(
            DB_EXTENSION_OVERSIZED_DB_ID,
            pfucbRoot,
            pgnoSELast,
            pgnoSELast + cpgSEReqAdj,
            pgnoSEMaxAdj,
            cpgSEMin,
            cpgSEReq,
            cpgSEMinAdj,
            cpgSEReqAdj );
    }

    // Resize the database. Try smaller extensions if extending by the preferred size fails.
    err = ErrSPINewSize( pfucbRoot->ifmp, pgnoSELast, cpgSEReqAdj, cpgAsyncExtension );

    if ( err < JET_errSuccess )
    {
        err = ErrSPINewSize( pfucbRoot->ifmp, pgnoSELast, cpgSEMinAdj, 0 );
        if ( err < JET_errSuccess )
        {
            //  we have failed to do a "big" allocation
            //  drop down to small allocations and see if we can succeed

            const CPG   cpgExtend   = 1;
            CPG         cpgT        = 0;

            do
            {
                cpgT += cpgExtend;
                Call( ErrSPINewSize( pfucbRoot->ifmp, pgnoSELast, cpgT, 0 ) );
            }
            while ( cpgT < cpgSEMinAdj );
        }

        cpgSEReqAdj = cpgSEMinAdj;
    }

    //  calculate last page of device level secondary extent
    //
    pgnoSELastAdj = pgnoSELast + cpgSEReqAdj;
    *pcpgSEAvail = cpgSEReqAdj - cpgAdj;
    *pcpgSEReq = cpgSEReqAdj;
    *ppgnoSELast = pgnoSELastAdj;
    Assert( ( *ppgnoSELast > pgnoSELast ) && ( ( *ppgnoSELast <= pgnoSEMaxAdj ) || fMayViolateMaxSize ) );
    Assert( ( *pcpgSEAvail >= cpgSEMin ) && ( *pcpgSEAvail <= *pcpgSEReq ) );
    Assert( (CPG)( *ppgnoSELast - pgnoSELast ) == ( cpgAdj + *pcpgSEAvail ) );
    Assert( *pcpgSEReq == ( cpgAdj + *pcpgSEAvail ) );

HandleError:

    if ( pfucbNil != pfucbOE )
    {
        BTClose( pfucbOE );
    }

    if ( pfucbNil != pfucbAE )
    {
        BTClose( pfucbAE );
    }

    return err;
}

ERR ErrSPExtendDB(
    PIB *       ppib,
    const IFMP  ifmp,
    const CPG   cpgSEMin,
    PGNO        *ppgnoSELast,
    const BOOL  fPermitAsyncExtension )
{
    ERR         err;
    FUCB        *pfucbDbRoot        = pfucbNil;
    FUCB        *pfucbAE            = pfucbNil;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortSpace );
    tcScope->SetDwEngineObjid( objidSystemRoot );

    //  open cursor on System / DB Root and RIW latch root page
    //
    CallR( ErrBTIOpenAndGotoRoot( ppib, pgnoSystemRoot, ifmp, &pfucbDbRoot ) );
    tcScope->nParentObjectClass = TceFromFUCB( pfucbDbRoot );
    Assert( objidSystemRoot == ObjidFDP( pfucbDbRoot ) );

    Call( ErrSPIOpenAvailExt( pfucbDbRoot, &pfucbAE ) );
    tcScope->nParentObjectClass = TceFromFUCB( pfucbAE );
    Assert( objidSystemRoot == ObjidFDP( pfucbAE ) );

    BTUp( pfucbAE );

    Assert( FSPIParentIsFs( pfucbDbRoot ) );
    Call( ErrSPIGetFsSe(
            pfucbDbRoot,
            pfucbAE,
            cpgSEMin,
            cpgSEMin,
            0,
            fTrue,
            fPermitAsyncExtension ) );

    Assert( Pcsr( pfucbAE )->FLatched() );

    BTUp( pfucbAE );

HandleError:

    if ( pfucbNil != pfucbAE )
    {
        BTClose( pfucbAE );
    }

    if ( pfucbNil != pfucbDbRoot )
    {
        BTClose( pfucbDbRoot );
    }

    return err;
}

// Seek to the last node of the root OE and returns the extent information.
LOCAL ERR ErrSPISeekRootOELast( _In_ FUCB* const pfucbOE, _Out_ CSPExtentInfo* const pspeiOE )
{
    ERR err = JET_errSuccess;
    DIB dib;

    Assert( !Pcsr( pfucbOE )->FLatched() );
    Assert( FFUCBOwnExt( pfucbOE ) );
    Assert( pfucbOE->u.pfcb->PgnoFDP() == pgnoSystemRoot );

    // Get the last page for the OE.
    dib.pos = posLast;
    dib.dirflag = fDIRNull;
    dib.pbm = NULL;
    err = ErrBTDown( pfucbOE, &dib, latchReadTouch );

    // We must have at least one owned extent.
    if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
    {
        FireWall( "SeekOeLastNoOwned" );
        Error( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
    }

    Call( err );

    pspeiOE->Set( pfucbOE );
    Call( pspeiOE->ErrCheckCorrupted() );

HandleError:
    if ( err < JET_errSuccess )
    {
        BTUp( pfucbOE );
    }

    return err;
}

// Seek to the node of the root AE whose pgnoLast is >= to the page being passed in and returns the extent information.
LOCAL ERR ErrSPISeekRootAE(
    _In_ FUCB* const pfucbAE,
    _In_ const PGNO pgno,
    _In_ const SpacePool sppAvailPool,
    _Out_ CSPExtentInfo* const pspeiAE )
{
    ERR err = JET_errSuccess;

    Assert( !Pcsr( pfucbAE )->FLatched() );
    Assert( FFUCBAvailExt( pfucbAE ) );
    Assert( pfucbAE->u.pfcb->PgnoFDP() == pgnoSystemRoot );

    // Get the last page for the AE.
    err = ErrSPIFindExtAE( pfucbAE, pgno, sppAvailPool, pspeiAE );

    // This is acceptable for the AE.
    if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
    {
        err = JET_errSuccess;
        BTUp( pfucbAE );
        goto HandleError;
    }

    Call( err );

    pspeiAE->Set( pfucbAE );
    Call( pspeiAE->ErrCheckCorrupted() );

HandleError:
    if ( err < JET_errSuccess )
    {
        BTUp( pfucbAE );
    }

    return err;
}

LOCAL ERR ErrSPICheckOEAELastConsistency( _In_ const CSPExtentInfo& speiLastOE, _In_ const CSPExtentInfo& speiLastAE )
{
    ERR err = JET_errSuccess;

    Call( speiLastOE.ErrCheckCorrupted() );
    Call( speiLastAE.ErrCheckCorrupted() );

    // Available space represented beyond owned space.
    if ( speiLastAE.PgnoLast() > speiLastOE.PgnoLast() )
    {
        AssertTrack( fFalse, "ShrinkConsistencyAePgnoLastBeyondOePgnoLast" );
        Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
    }

    // AE node crossing OE node.
    if ( !speiLastOE.FContains( speiLastAE.PgnoFirst() ) && speiLastOE.FContains( speiLastAE.PgnoLast() ) )
    {
        AssertTrack( fFalse, "ShrinkConsistencyAeCrossesOeBoundary" );
        Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
    }

HandleError:
    return err;
}

// Iterates over the available/owned extents in the root of the given FMP, makes the
// necessary changes to reduce the amount of available/owned space and finally truncates
// the file. It only processes the last owned extent.
ERR ErrSPShrinkTruncateLastExtent(
    _In_ PIB* ppib,
    _In_ const IFMP ifmp,
    _In_ CPRINTF* const pcprintfShrinkTraceRaw,
    _Inout_ HRT* const pdhrtExtMaint,
    _Inout_ HRT* const pdhrtFileTruncation,
    _Out_ PGNO* const ppgnoFirstFromLastExtentTruncated,
    _Out_ ShrinkDoneReason* const psdr )
{
    ERR err = JET_errSuccess;
    BOOL fInTransaction = fFalse;
    CSPExtentInfo speiLastOE, speiAE;
    CSPExtentInfo speiLastAfterOE, speiLastAfterAE;
    FMP* const pfmp = g_rgfmp + ifmp;
    FUCB* pfucbRoot = pfucbNil;
    FUCB* pfucbOE = pfucbNil;
    FUCB* pfucbAE = pfucbNil;
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope( );
    tcScope->iorReason.SetIort( iortDbShrink );
    tcScope->SetDwEngineObjid( objidSystemRoot );
    BOOL fExtMaint = fFalse;
    BOOL fFileTruncation = fFalse;
    HRT hrtExtMaintStart = 0;
    HRT hrtFileTruncationStart = 0;

    Assert( pfmp->FShrinkIsRunning() );
    Assert( pfmp->FExclusiveBySession( ppib ) );

    *psdr = sdrNone;
    *ppgnoFirstFromLastExtentTruncated = pgnoNull;

    // ErrLGSetExternalHeader, called in ErrNDSetExternalHeader, asks to be under
    // a transaction. However, note that space operations which are about to be performed
    // by this function are not versioned and as such do not really rollback upon
    // transaction rollback.
    Call( ErrDIRBeginTransaction( ppib, 48584, NO_GRBIT ) );
    fInTransaction = fTrue;

    // Open space trees.
    Call( ErrBTIOpenAndGotoRoot( ppib, pgnoSystemRoot, ifmp, &pfucbRoot ) );

    Call( ErrSPIOpenOwnExt( pfucbRoot, &pfucbOE ) );
    Call( ErrSPIOpenAvailExt( pfucbRoot, &pfucbAE ) );

    // OE.
    Call( ErrSPISeekRootOELast( pfucbOE, &speiLastOE ) );
    *ppgnoFirstFromLastExtentTruncated = speiLastOE.PgnoFirst();

    // The first extent is never supposed to be moved, it's the bootstrap of the database.
    if ( speiLastOE.PgnoFirst() == pgnoSystemRoot )
    {
        *psdr = sdrNoLowAvailSpace;
        goto HandleError;
    }

    // Set shrink target below the current extent to ensure we don't allocate from the extent itself,
    // though allocations are not expected in this function.
    pfmp->SetPgnoShrinkTarget( speiLastOE.PgnoFirst() - 1 );

    // AE (first, test that we don't have available space beyond the owned space).
    Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast() + 1, spp::AvailExtLegacyGeneralPool, &speiAE ) );
    if ( speiAE.FIsSet() )
    {
        AssertTrack( fFalse, "ShrinkExtAeBeyondOe" );
        Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
    }

    // Check if we've reached the desired size.
    if ( ( speiLastOE.PgnoLast() + (PGNO)cpgDBReserved ) <= (PGNO)pfmp->CpgShrinkDatabaseSizeLimit() )
    {
        *psdr = sdrReachedSizeLimit;
        goto HandleError;
    }

    // AE (now, seek to the right position).
    Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast(), spp::AvailExtLegacyGeneralPool, &speiAE ) );
    if ( !speiAE.FIsSet() )
    {
        Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast(), spp::ShelvedPool, &speiAE ) );
        if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() != speiLastOE.PgnoLast() ) )
        {
            Assert( !speiAE.FIsSet() || ( speiAE.PgnoLast() > speiLastOE.PgnoLast() ) );
            Error( ErrERRCheck( JET_wrnShrinkNotPossible ) );
        }
    }

    Call( ErrSPICheckOEAELastConsistency( speiLastOE, speiAE ) );

    fExtMaint = fTrue;
    hrtExtMaintStart = HrtHRTCount();

    // We'll now scan the last owned extent to see if it is fully available.
    // We can only proceed if it is either fully available or if all gaps
    // in availability are filled by shelved pages.
    PGNO pgnoLastAEExpected = speiLastOE.PgnoLast();
    while ( fTrue )
    {
        // We do not expect zero-sized AE nodes. Log a firewall and bail.
        if ( speiAE.CpgExtent() == 0 )
        {
            FireWall( "ShrinkExtZeroedAe" );
            *psdr = sdrUnexpected;
            goto HandleError;
        }

        Assert( speiAE.PgnoLast() == pgnoLastAEExpected );

        // AE crosses OE boundary. Not expected.
        if ( speiAE.PgnoFirst() < speiLastOE.PgnoFirst() )
        {
            AssertTrack( fFalse, "ShrinkExtAeCrossesOeBoundary" );
            Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }

        //  Did we get to the start of the OE?
        if ( speiAE.PgnoFirst() == speiLastOE.PgnoFirst() )
        {
            break;
        }

        // Refresh expected pgnoLast.
        pgnoLastAEExpected = speiAE.PgnoFirst() - 1;

        // Seek to extent with the expected page number.
        // Try to find a suitable extent in the shelved pool if the regular
        // pool indicates that the space is not readily available.
        BTUp( pfucbAE );
        Call( ErrSPISeekRootAE( pfucbAE, pgnoLastAEExpected, spp::AvailExtLegacyGeneralPool, &speiAE ) );
        if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() != pgnoLastAEExpected ) )
        {
            BTUp( pfucbAE );
            Call( ErrSPISeekRootAE( pfucbAE, pgnoLastAEExpected, spp::ShelvedPool, &speiAE ) );
            if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() != pgnoLastAEExpected ) )
            {
                Error( ErrERRCheck( JET_wrnShrinkNotPossible ) );
            }
        }
    }

    if ( !pfmp->FShrinkIsActive() )
    {
        AssertTrack( fFalse, "ShrinkExtShrinkInactiveBeforeTruncation" );
        Error( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ) );
    }

    // Just in case, but we don't expect version store and DB tasks to be running at this point.
    Call( PverFromPpib( ppib )->ErrVERRCEClean( ifmp ) );
    if ( err != JET_errSuccess )
    {
        AssertTrack( fFalse, OSFormat( "ShrinkExtVerCleanWrn:%d", err ) );
        *psdr = sdrUnexpected;
        goto HandleError;
    }
    pfmp->WaitForTasksToComplete();

    PGNO pgnoLastFileSystem = pgnoNull;
    Call( pfmp->ErrPgnoLastFileSystem( &pgnoLastFileSystem ) );

    const CPG cpgShrunk = pgnoLastFileSystem - ( speiLastOE.PgnoFirst() - 1 );
    Assert( cpgShrunk > 0 );

    // Now, we're going to actually remove nodes from the OE and AE trees, though we'll obviously
    // keep the nodes from the shelved page pool, if any, so that we won't consider that space
    // available the next time we grow the database.
    //
    // It is expected here that SPACE will not hand out pages from the extent we are operating on
    // in case manipulating the root AE or OE itself requires allocating new space (removing AE nodes
    // could, for example, trigger a split of the AE if we hit a merge situation when deleting the nodes).
    //
    // If space is needed to add new nodes to the AE, it is expected that the AE split buffer will be
    // able to handle the split. As for the OE, there could be a case where replenishing the OE or AE
    // split buffers could require extra space. In any case, if we try and allocate space from the extent
    // currently being targeted for shrink or beyond, FMP::m_pgnoShrinkTarget is supposed to make sure that
    // no space is returned in that region.
    //
    // Note that if we crash after having deleted AE nodes, but before deleting the OE node,
    // we'll leak that space represented by the deleted AE nodes forever.
    //
    // The most efficient way to perform the AE node deletions would be to position the cursor in the first
    // general pool node and move-next until the end of that pool. However, to add an extra layer of
    // corruption protection, we'll seek to each of the extents, including ones in the shelved pool, though
    // those won't be deleted. Note that the vast majority of cases consists of one single general pool AE
    // node, so performance is not a concern here.
    //
    BTUp( pfucbAE );
    Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast(), spp::AvailExtLegacyGeneralPool, &speiAE ) );
    if ( !speiAE.FIsSet() )
    {
        Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast(), spp::ShelvedPool, &speiAE ) );
        if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() != speiLastOE.PgnoLast() ) )
        {
            // We deemed the extent fully available, but something changed unexpectedly.
            AssertTrack( fFalse, "ShrinkExtNoLongerAvailableLast" );
            Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }
    }

    Call( ErrSPICheckOEAELastConsistency( speiLastOE, speiAE ) );

    ULONG cAeExtDeleted = 0, cAeExtShelved = 0;
    pgnoLastAEExpected = speiLastOE.PgnoLast();
    while ( fTrue )
    {
        // We do not expect zero-sized AE nodes, especially after having checked above.
        // This one is _really_ unexpected, so return a corruption error.
        if ( speiAE.CpgExtent() == 0 )
        {
            AssertTrack( fFalse, "ShrinkExtZeroedAeNew" );
            Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }

        Assert( speiAE.PgnoLast() == pgnoLastAEExpected );

        // AE crosses OE boundary. Not expected.
        if ( speiAE.PgnoFirst() < speiLastOE.PgnoFirst() )
        {
            AssertTrack( fFalse, "ShrinkExtAeCrossesOeBoundaryNew" );
            Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }

        // Delete node only if it's a general pool extent.
        if ( speiAE.SppPool() == spp::AvailExtLegacyGeneralPool )
        {
            if ( !pfmp->FShrinkIsActive() )
            {
                AssertTrack( fFalse, "ShrinkExtShrinkInactiveBeforeAeDeletion" );
                Error( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ) );
            }

            Expected( !pfucbAE->u.pfcb->FDontLogSpaceOps() );

            Assert( pfucbRoot->u.pfcb == pfucbAE->u.pfcb );
            Call( ErrSPIWrappedBTFlagDelete(
                    pfucbAE,
                    fDIRNoVersion | ( pfucbAE->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    0,
                    -( speiAE.CpgExtent() ) ) );

            Assert( latchWrite == Pcsr( pfucbAE )->Latch() );
            Pcsr( pfucbAE )->Downgrade( latchReadTouch );
            cAeExtDeleted++;

            Call( ErrFaultInjection( 49736 ) );
        }
        else
        {
            Assert( speiAE.SppPool() == spp::ShelvedPool );
            Call( ErrSPCaptureSnapshot( pfucbRoot, speiAE.PgnoFirst(), speiAE.CpgExtent(), fFalse ) );
            cAeExtShelved++;
        }

        //  Did we get to the start of the OE?
        if ( speiAE.PgnoFirst() == speiLastOE.PgnoFirst() )
        {
            err = JET_errSuccess;
            break;
        }

        // Refresh expected pgnoLast.
        pgnoLastAEExpected = speiAE.PgnoFirst() - 1;

        // Seek to extent with the expected page number.
        // Try to find a suitable extent in the shelved pool if the regular
        // pool indicates that the space is not readily available.
        BTUp( pfucbAE );
        Call( ErrSPISeekRootAE( pfucbAE, pgnoLastAEExpected, spp::AvailExtLegacyGeneralPool, &speiAE ) );
        if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() != pgnoLastAEExpected ) )
        {
            BTUp( pfucbAE );
            Call( ErrSPISeekRootAE( pfucbAE, pgnoLastAEExpected, spp::ShelvedPool, &speiAE ) );
            if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() != pgnoLastAEExpected ) )
            {
                // We deemed the extent fully available, but something changed unexpectedly.
                AssertTrack( fFalse, "ShrinkExtNoLongerAvailableNew" );
                Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
            }
        }
    }

    BTUp( pfucbAE );
    BTUp( pfucbOE );

    // Bail if we added more space in the process (unexpected).
    Call( ErrSPISeekRootOELast( pfucbOE, &speiLastAfterOE ) );
    if ( speiLastAfterOE.PgnoLast() != speiLastOE.PgnoLast() )
    {
        Assert( speiLastAfterOE.PgnoLast() > speiLastOE.PgnoLast() );
        FireWall( "ShrinkNewSpaceOwnedWhileShrinking" );
        *psdr = sdrUnexpected;
        goto HandleError;
    }

    // Check consistency of the AE.
    Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast(), spp::AvailExtLegacyGeneralPool, &speiLastAfterAE ) );
    BTUp( pfucbAE );
    if ( speiLastAfterAE.FIsSet() )
    {
        Call( ErrSPICheckOEAELastConsistency( speiLastAfterOE, speiLastAfterAE ) );
        if ( speiLastAfterAE.PgnoLast() >= speiLastOE.PgnoFirst() )
        {
            FireWall( "ShrinkNewSpaceAvailWhileShrinking" );
            *psdr = sdrUnexpected;
            goto HandleError;
        }
    }

    // Force this code to bail and leave a shelved extent below EOF behind.
    if ( ( cAeExtDeleted > 0 ) && ( cAeExtShelved > 0 ) )
    {
        Call( ErrFaultInjection( 50114 ) );
    }

    {
    // Set the reduced owned file size before deleting the owned space so we never get to overestimate
    // the owned size.
    const QWORD cbOwnedFileSizeBefore = pfmp->CbOwnedFileSize();
    pfmp->SetOwnedFileSize( CbFileSizeOfPgnoLast( speiLastAfterOE.PgnoFirst() - 1 ) );

    if ( !pfmp->FShrinkIsActive() )
    {
        AssertTrack( fFalse, "ShrinkExtShrinkInactiveBeforeOeDeletion" );
        Error( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ) );
    }

    // Now, delete the OE node.
    Expected( !pfucbOE->u.pfcb->FDontLogSpaceOps() );

    Assert( pfucbRoot->u.pfcb == pfucbOE->u.pfcb );
    err = ErrSPIWrappedBTFlagDelete(
            pfucbOE,
            fDIRNoVersion | ( pfucbOE->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
            -( speiLastAfterOE.CpgExtent() ),
            0 );
    if ( err < JET_errSuccess )
    {
        pfmp->SetOwnedFileSize( cbOwnedFileSizeBefore );
    }
    Call( err );


    Assert( latchWrite == Pcsr( pfucbOE )->Latch() );
    BTUp( pfucbOE );
    (*pcprintfShrinkTraceRaw)( "ShrinkTruncate[%I32u:%I32u]\r\n", speiLastAfterOE.PgnoFirst(), speiLastAfterOE.PgnoLast() );

    if ( !pfmp->FShrinkIsActive() )
    {
        AssertTrack( fFalse, "ShrinkExtShrinkInactiveAfterTruncation" );
        Error( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ) );
    }
    }

    // Refresh info on last OE node.
    Call( ErrSPISeekRootOELast( pfucbOE, &speiLastAfterOE ) );
    BTUp( pfucbOE );
    Assert( speiLastAfterOE.PgnoLast() == pfmp->PgnoLast() );

    // Refresh info on last AE node.
    Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast(), spp::AvailExtLegacyGeneralPool, &speiLastAfterAE ) );
    BTUp( pfucbAE );

    // Double-check that everything looks consistent after space operations.
    if ( speiLastAfterOE.PgnoLast() >= speiLastOE.PgnoFirst() )
    {
        AssertTrack( fFalse, "ShrinkNewSpaceOwnedPostOeDeleteWhileShrinking" );
        Error( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
    }
    if ( speiLastAfterAE.FIsSet() )
    {
        Call( ErrSPICheckOEAELastConsistency( speiLastAfterOE, speiLastAfterAE ) );

        if ( speiLastAfterAE.PgnoLast() >= speiLastOE.PgnoFirst() )
        {
            AssertTrack( fFalse, "ShrinkNewSpaceAvailPostOeDeleteWhileShrinking" );
            Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }
    }

    // Double-check if physical file size is smaller than size represented in the OE.
    if ( speiLastAfterOE.PgnoLast() > pgnoLastFileSystem )
    {
        AssertTrack( fFalse, "ShrinkExtOePgnoLastBeyondFsPgnoLast" );
        Error( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
    }

    // Close cursors.
    BTClose( pfucbAE );
    pfucbAE = pfucbNil;
    BTClose( pfucbOE );
    pfucbOE = pfucbNil;
    pfucbRoot->pcsrRoot->ReleasePage();
    pfucbRoot->pcsrRoot = pcsrNil;
    BTClose( pfucbRoot );
    pfucbRoot = pfucbNil;

    // Commit just for kicks. Space operations are unversioned anyways.
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTransaction = fFalse;

    *pdhrtExtMaint += DhrtHRTElapsedFromHrtStart( hrtExtMaintStart );
    fExtMaint = fFalse;

    fFileTruncation = fTrue;
    hrtFileTruncationStart = HrtHRTCount();

    // Finally, truncate the file.
    {
    PIBTraceContextScope tcScopeT( ppib, iorpDatabaseShrink );

    Call( ErrFaultInjection( 40200 ) );

    Call( ErrSPINewSize( ifmp, pgnoLastFileSystem, -1 * cpgShrunk, 0 ) );
    }

    pfmp->ResetPgnoMaxTracking( speiLastAfterOE.PgnoLast() );

    *pdhrtFileTruncation += DhrtHRTElapsedFromHrtStart( hrtFileTruncationStart );
    fFileTruncation = fFalse;

HandleError:
    Assert( !( fExtMaint && fFileTruncation ) );

    if ( fExtMaint )
    {
        *pdhrtExtMaint += DhrtHRTElapsedFromHrtStart( hrtExtMaintStart );
        fExtMaint = fFalse;
    }

    if ( fFileTruncation )
    {
        *pdhrtFileTruncation += DhrtHRTElapsedFromHrtStart( hrtFileTruncationStart );
        fFileTruncation = fFalse;
    }

    if ( pfucbAE != pfucbNil )
    {
        BTUp( pfucbAE );
        BTClose( pfucbAE );
        pfucbAE = pfucbNil;
    }

    if ( pfucbOE != pfucbNil )
    {
        BTUp( pfucbOE );
        BTClose( pfucbOE );
        pfucbOE = pfucbNil;
    }

    if ( pfucbRoot != pfucbNil )
    {
        BTClose( pfucbRoot );
        pfucbRoot = pfucbNil;
    }

    // WARNING: all the above is done without versioning, so there really isn't any rollback of the update.
    // This transaction is fake and we always will have done whatever we got done.
    if ( fInTransaction )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
        fInTransaction = fFalse;
    }

    return err;
}

// Returns the preferred number of pages to fill a space tree's split buffer with, given
// the required page count (cpgRequired) and the minimum number of pages required for
// a full vertical split (cpgMinForSplit). Note that cpgRequired is the minimum that can
// be returned, while cpgMinForSplit is passed only so we can estimate the scale of the
// tree.
LOCAL CPG CpgSPICpgPrefFromCpgRequired( const CPG cpgRequired, const CPG cpgMinForSplit )
{
    CPG cpgPref = 0;

    if ( cpgMinForSplit <= cpgMaxRootPageSplit )
    {
        cpgPref = cpgPrefRootPageSplit;
    }
    else if ( cpgMinForSplit <= cpgMaxParentOfLeafRootSplit )
    {
        cpgPref = cpgPrefParentOfLeafRootSplit;
    }
    else
    {
        cpgPref = cpgPrefSpaceTreeSplit;
    }

    if ( cpgRequired > cpgPref )
    {
        cpgPref = LNextPowerOf2( cpgRequired );
    }

    #ifndef ENABLE_JET_UNIT_TEST 
    Expected( ( cpgPref <= 16 ) || ( UlConfigOverrideInjection( 46030, 0 ) != 0 ) );
    #endif

    return cpgPref;
}

#ifdef ENABLE_JET_UNIT_TEST
JETUNITTEST( SPACE, CpgSPICpgPrefFromCpgRequired )
{
    // Nothing required to split, but split buffer still required.
    CHECK( 2 == CpgSPICpgPrefFromCpgRequired( 0, 0 ) );
    CHECK( 2 == CpgSPICpgPrefFromCpgRequired( 2, 0 ) );
    CHECK( 4 == CpgSPICpgPrefFromCpgRequired( 3, 0 ) );
    CHECK( 4 == CpgSPICpgPrefFromCpgRequired( 4, 0 ) );
    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 5, 0 ) );
    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 8, 0 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 9, 0 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 16, 0 ) );
    CHECK( 32 == CpgSPICpgPrefFromCpgRequired( 17, 0 ) );

    // Typical AE case where cpgRequired == cpgMinForSplit.
    CHECK( 2 == CpgSPICpgPrefFromCpgRequired( 2, 2 ) );
    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 3, 3 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 4, 4 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 5, 5 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 16, 16 ) );
    CHECK( 32 == CpgSPICpgPrefFromCpgRequired( 17, 17 ) );

    // Typical AE case where cpgRequired == cpgMinForSplit, with 1 additional page required.
    CHECK( 4 == CpgSPICpgPrefFromCpgRequired( 3, 2 ) );
    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 4, 3 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 5, 4 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 6, 5 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 16, 15 ) );
    CHECK( 32 == CpgSPICpgPrefFromCpgRequired( 17, 16 ) );

    // Typical OE case where cpgRequired == 2 * cpgMinForSplit.
    CHECK( 4 == CpgSPICpgPrefFromCpgRequired( 4, 2 ) );
    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 6, 3 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 8, 4 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 10, 5 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 16, 8 ) );
    CHECK( 32 == CpgSPICpgPrefFromCpgRequired( 18, 9 ) );

    // Typical OE case where cpgRequired == 2 * cpgMinForSplit, with 1 additional page required.
    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 5, 2 ) );
    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 7, 3 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 9, 4 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 11, 5 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 15, 7 ) );
    CHECK( 32 == CpgSPICpgPrefFromCpgRequired( 17, 8 ) );
}
#endif // ENABLE_JET_UNIT_TEST

LOCAL ERR ErrSPIReserveSPBufPagesForSpaceTree(
    FUCB* const pfucb,
    FUCB* const pfucbSpace,
    FUCB* const pfucbParent,
    CArray<EXTENTINFO>* const parreiReleased = NULL,
    const CPG cpgAddlReserve = 0,
    const PGNO pgnoReplace = pgnoNull )
{
    ERR             err  = JET_errSuccess;
    ERR             wrn  = JET_errSuccess;
    SPLIT_BUFFER    *pspbuf = NULL;
    CPG             cpgMinForSplit = 0;
    FMP*            pfmp = g_rgfmp + pfucb->ifmp;

    OnDebug( ULONG crepeat = 0 );
    Assert( pfmp != NULL );
    Assert( ( pfucb != pfucbNil ) && !FFUCBSpace( pfucb ) );
    Assert( ( pfucbSpace != pfucbNil ) && FFUCBSpace( pfucbSpace ) );
    Assert( ( pfucbParent == pfucbNil ) || !FFUCBSpace( pfucbParent ) );
    Assert( !Pcsr( pfucbSpace )->FLatched() );
    Assert( pfucb->u.pfcb == pfucbSpace->u.pfcb );
    Assert( pfucb->u.pfcb->FSpaceInitialized() );
    Assert( !FSPIIsSmall( pfucb->u.pfcb ) );
    Assert( ( pgnoReplace == pgnoNull ) || pfmp->FShrinkIsRunning() ); // Shrink is the only consumer of this facility so far.
    const BOOL fUpdatingDbRoot = ( pfucbParent == pfucbNil );
    const BOOL fMayViolateMaxSize = ( pgnoReplace == pgnoNull ); // An explicit replacement is considered optional and
                                                                 // therefore not prone to leaking space.
    const BOOL fAvailExt = FFUCBAvailExt( pfucbSpace );
    const BOOL fOwnExt = FFUCBOwnExt( pfucbSpace );
    Assert( !!fAvailExt ^ !!fOwnExt );

    rtlconst BOOL fForceRefillDbgOnly = fFalse;
    OnDebug( fForceRefillDbgOnly = ( ErrFaultInjection( 47594 ) < JET_errSuccess ) && !g_fRepair );

    rtlconst BOOL fForcePostAddToOwnExtDbgOnly = fFalse;
    rtlconst ERR errFaultAddToOe = JET_errSuccess;
#ifdef DEBUG
    errFaultAddToOe = fUpdatingDbRoot ? ErrFaultInjection( 60394 ) : ErrFaultInjection( 35818 );
    OnDebug( fForcePostAddToOwnExtDbgOnly = ( ( errFaultAddToOe == JET_errSuccess ) && ( rand() % 4 ) == 0 ) );
#endif  // DEBUG

    // Parent is not latched, we'll latch it when we need to.
    AssertSPIPfucbNullOrUnlatched( pfucbParent );

    forever
    {
        BOOL fSingleAndAvailableEnough = fFalse;
        SPLIT_BUFFER spbufBefore;

        Assert( crepeat < 3 );      //  shouldn't have to make more than 2 iterations in common case
                                    //  and 3 when we crash in the middle of operation. Consequential
                                    //  recovery will not reclaim the space.
        OnDebug( crepeat++ );

        AssertSPIPfucbOnRoot( pfucb );
        // Parent is still not latched, we'll latch it when we need to.
        AssertSPIPfucbNullOrUnlatched( pfucbParent );

        // Latch space FUCB and get split buffer pointer.
        Call( ErrBTIGotoRoot( pfucbSpace, latchRIW ) );
        AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );
        Call( ErrSPIGetSPBuf( pfucbSpace, &pspbuf ) );
        UtilMemCpy( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) );

        if ( pfucbSpace->csr.Cpage().FLeafPage() )
        {
            fSingleAndAvailableEnough = pfucbSpace->csr.Cpage().CbPageFree() >= 80;

            // If we're low on space, we must have already allocated a split buffer before
            // and not used it (it's still a single-level tree).
            Expected( fSingleAndAvailableEnough || ( ( pspbuf->CpgBuffer1() + pspbuf->CpgBuffer2() ) >= cpgMinForSplit ) );

            //  root page is also leaf page,
            //  see if we're close to splitting
            if ( pfucbSpace->csr.Cpage().CbPageFree() < 100 )
            {
                cpgMinForSplit = cpgMaxRootPageSplit;
            }
            else
            {
                cpgMinForSplit = 0;
            }
        }
        else if ( pfucbSpace->csr.Cpage().FParentOfLeaf() )
        {
            cpgMinForSplit = cpgMaxParentOfLeafRootSplit;
        }
        else
        {
            cpgMinForSplit = cpgMaxSpaceTreeSplit;
        }

        CPG cpgRequired;
        CPG cpgRequest;
        CPG cpgAvailable;
        CPG cpgNewSpace;
        BYTE ispbufReplace;
        BOOL fSpBuffersAvailableEnough;
        BOOL fSpBufRefilled;
        BOOL fAlreadyOwn;
        BOOL fAddedToOwnExt;
        PGNO pgnoLast;

        const ERR wrnT = wrn;

        // We need to prevent new extents from being added as available before they are owned. This could
        // potentially happen only during Shrink. Inactivate Shrink and retry if we hit that case.
        // If Shrink is not running the forever loop below is guaranteed to break out on the first iteration.
        forever
        {
            wrn = wrnT;
            cpgRequired = 0;
            cpgRequest = 0;
            cpgAvailable = 0;
            cpgNewSpace = 0;
            ispbufReplace = 0;
            fSpBuffersAvailableEnough = fFalse;
            fSpBufRefilled = fFalse;
            fAlreadyOwn = fFalse;
            fAddedToOwnExt = fFalse;
            pgnoLast = pgnoNull;

            // If this is the OE, then we'll always refill the split buffers with 2x the minimum required
            // such that we're guaranteed, from now on, to be able to add the next/new split buffer we need
            // to owned space of the OE itself (before we add it to the spbuf avail / External Header).
            cpgRequired = fOwnExt ? ( 2 * cpgMinForSplit ) : cpgMinForSplit;
            cpgRequired += ( cpgAddlReserve + (CPG)UlConfigOverrideInjection( 46030, 0 ) );

            OnDebug( fForceRefillDbgOnly = fForceRefillDbgOnly && ( cpgRequired > 0 ) );

            // Check if we have pages beyond the EOF which could be shelved.
            if ( pspbuf->PgnoLastBuffer1() > pfmp->PgnoLast() )
            {
                ispbufReplace = 1;
            }
            else if ( pspbuf->PgnoLastBuffer2() > pfmp->PgnoLast() )
            {
                ispbufReplace = 2;
            }

            // Check if we've explicitly requested to replace a buffer.
            if ( ( pgnoReplace != pgnoNull ) && ( ispbufReplace == 0 ) )
            {
                if ( pspbuf->FBuffer1ContainsPgno( pgnoReplace ) )
                {
                    Assert( !pspbuf->FBuffer2ContainsPgno( pgnoReplace ) );
                    ispbufReplace = 1;
                }
                else if ( pspbuf->FBuffer2ContainsPgno( pgnoReplace ) )
                {
                    ispbufReplace = 2;
                }
            }

            // If shrink is running, force-refill split buffers which are beyond the shrink target.
            // This is not necessary for correctness, but rather an optimization to prevent pages from
            // being allocated above the one we're currently processing during shrink should we need to
            // pull a page from a split buffer. Also, from a completeness standpoint, we should prevent
            // pages from being allocated in the range currently undergoing data move for shrink, like
            // we do for regular space allocation from AE trees.
            if ( pfmp->FShrinkIsActive() && ( ispbufReplace == 0 ) )
            {
                if ( pfmp->FBeyondPgnoShrinkTarget( pspbuf->PgnoFirstBuffer1(), pspbuf->CpgBuffer1() ) )
                {
                    ispbufReplace = 1;
                }
                else if ( pfmp->FBeyondPgnoShrinkTarget( pspbuf->PgnoFirstBuffer2(), pspbuf->CpgBuffer2() ) )
                {
                    ispbufReplace = 2;
                }
            }

            const CPG cpgSpBuffer1Available = ( pfmp->FBeyondPgnoShrinkTarget( pspbuf->PgnoFirstBuffer1(), pspbuf->CpgBuffer1() ) ||
                                                 ( pspbuf->PgnoLastBuffer1() > pfmp->PgnoLast() ) ) ? 0 : pspbuf->CpgBuffer1();
            const CPG cpgSpBuffer2Available = ( pfmp->FBeyondPgnoShrinkTarget( pspbuf->PgnoFirstBuffer2(), pspbuf->CpgBuffer2() ) ||
                                                 ( pspbuf->PgnoLastBuffer2() > pfmp->PgnoLast() ) ) ? 0 : pspbuf->CpgBuffer2();

            fSpBuffersAvailableEnough = ( ( cpgSpBuffer1Available + cpgSpBuffer2Available ) >= cpgMinForSplit );

            if ( ispbufReplace != 0 )
            {
                OnDebug( const PGNO pgnoLastSpBuf = ( ispbufReplace == 1 ) ? pspbuf->PgnoLastBuffer1() : pspbuf->PgnoLastBuffer2() );
                OnDebug( const PGNO cpgExt = ( ispbufReplace == 1 ) ? pspbuf->CpgBuffer1() : pspbuf->CpgBuffer2() );

                // Shrink and shelved page replacement are the only consumers of this facility so far.
                Expected( pfmp->FShrinkIsRunning() || ( pgnoLastSpBuf > pfmp->PgnoLast() ) );

                // Signal to upper layers that we need to rerun split buffer refill because
                // we forced an explicit refill so the other one may need it too.
                wrn = ErrERRCheck( wrnSPRequestSpBufRefill );

                // Allow for a smaller buffer, as long as it satisfies cpgRequired when added to the other
                // buffer. For DBs with small extension sizes, it may be hard to find an available AE
                // below the shrink range with suitable size.
                const CPG cpgNewSpBuf = cpgRequired - ( ( ispbufReplace == 1 ) ? cpgSpBuffer2Available : cpgSpBuffer1Available );
                cpgRequired = max( cpgNewSpBuf, cpgMaxRootPageSplit );
            }

            // Bail only if all of these are satisfied:
            //  o The total reserved buffers is >= cpgRequired.
            //  o We aren't replacing a specific buffer.
            //  o We are not forcing a refill (DEBUG only).
            if ( ( ( cpgSpBuffer1Available + cpgSpBuffer2Available ) >= cpgRequired ) &&
                    ( ispbufReplace == 0 ) &&
                    !fForceRefillDbgOnly )
            {
                fSpBufRefilled = fTrue;
                break;
            }

            Assert( cpgRequired > 0 );
            cpgRequest = CpgSPICpgPrefFromCpgRequired( cpgRequired, cpgMinForSplit );

            if ( fUpdatingDbRoot && ( pfmp->FShrinkIsRunning() || pfmp->FSelfAllocSpBufReservationEnabled() ) )
            {
                // First, we're going to try and get space from the AE tree itself.
                // To avoid infinite recursion and deadlocks, we'll prevent ErrSPIGetExt() from getting
                // space hierarchically, since we already handle that case ourselves below.

                BTUp( pfucbSpace );
                pspbuf = NULL;

                PGNO pgnoFirst = pgnoNull;
                cpgNewSpace = cpgRequest;
                err = ErrSPIGetExt(
                            pfucb->u.pfcb,
                            pfucb,
                            &cpgNewSpace,
                            cpgRequired,
                            &pgnoFirst,
                            fSPSelfReserveExtent );
                AssertSPIPfucbOnRoot( pfucb );

                if ( err >= JET_errSuccess )
                {
                    cpgAvailable = cpgNewSpace;
                    pgnoLast = pgnoFirst + cpgNewSpace - 1;
                    fAlreadyOwn = fTrue;
                }
                else
                {
                    // Failing to allocate space for root split buffers may lead to extensive space
                    // leakage. Ignore any errors so that we can try again by getting space from the file
                    // system (i.e., by growing the file).

                    AssertTrack( ( err == errSPNoSpaceForYou ) || FRFSAnyFailureDetected(), OSFormat( "SpBufSelfAllocError:%d", err ) );
                }

                err = JET_errSuccess;

                // Re-latch space FUCB and get split buffer pointer. We don't expect
                // the contents to have changed because we have the root FUCB latched
                // all the way through this function.
                Call( ErrBTIGotoRoot( pfucbSpace, latchRIW ) );
                AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );
                Call( ErrSPIGetSPBuf( pfucbSpace, &pspbuf ) );
                Assert( 0 == memcmp( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) ) );
                UtilMemCpy( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) );
            }

            Assert( !!fAlreadyOwn ^ !!( pgnoLast == pgnoNull ) );

            if ( fForcePostAddToOwnExtDbgOnly ||
                 fAlreadyOwn || !fOwnExt ||
                 !fUpdatingDbRoot ||
                 fSpBuffersAvailableEnough || fSingleAndAvailableEnough )
            {
                break;
            }

            AssertTrack( pfmp->FShrinkIsActive(), "SpBufUnexpectedShrinkInactive" );
            if ( !pfmp->FShrinkIsActive() )
            {
                // The retry is based on inactivating Shrink, so it's pointless
                // if Shrink isn't already active.
                break;
            }

            // Inactivate Shrink and retry.
            pfmp->ResetPgnoShrinkTarget();
            Assert( !pfmp->FShrinkIsActive() );
        }  // forever

        if ( fSpBufRefilled )
        {
            break;
        }

        if ( !fAlreadyOwn )
        {
            if ( !fUpdatingDbRoot )
            {
                PGNO pgnoFirst = pgnoNull;
                cpgNewSpace = cpgRequest;

                AssertSPIPfucbNullOrUnlatched( pfucbParent );

                Call( ErrBTIGotoRoot( pfucbParent, latchRIW ) );
                pfucbParent->pcsrRoot = Pcsr( pfucbParent );

                AssertSPIPfucbOnRoot( pfucbParent );

                err = ErrSPIGetExt(
                            pfucb->u.pfcb,
                            pfucbParent,
                            &cpgNewSpace,
                            cpgRequired,
                            &pgnoFirst,
                            0,
                            0,
                            NULL,
                            NULL,
                            NULL,
                            fMayViolateMaxSize );

                BTUp( pfucbParent );
                pfucbParent->pcsrRoot = pcsrNil;

                AssertSPIPfucbOnRoot( pfucb );
                AssertSPIPfucbNullOrUnlatched( pfucbParent );
                AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );
                Call( err );
                cpgAvailable = cpgNewSpace;

                pgnoLast = pgnoFirst + cpgNewSpace - 1;
            }
            else
            {
                Assert( pgnoSystemRoot == pfucb->u.pfcb->PgnoFDP() );

                //  don't want to grow database by only 1 or 2 pages at a time, so
                //  choose largest value to satisfy max. theoretical split requirements
                cpgRequired = max( cpgRequired, cpgMaxSpaceTreeSplit );
                cpgRequest = CpgSPICpgPrefFromCpgRequired( cpgRequired, cpgMinForSplit );


                AssertTrack( NULL == pfucbSpace->u.pfcb->Psplitbuf( fAvailExt ), "NonNullRootAvailSplitBuff" ); //  not really handled
                BTUp( pfucbSpace );
                pspbuf = NULL;

                cpgNewSpace = cpgRequest;
                Call( ErrSPIExtendDB(
                        pfucb,
                        cpgRequired,
                        &cpgNewSpace,
                        &pgnoLast,
                        fFalse, // fPermitAsyncExtension
                        fMayViolateMaxSize,
                        parreiReleased,
                        &cpgAvailable ) );

                // Re-latch space FUCB and get split buffer pointer. We don't expect
                // the contents to have changed because we have the root FUCB latched
                // all the way through this function.
                Call( ErrBTIGotoRoot( pfucbSpace, latchRIW ) );
                AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );
                Call( ErrSPIGetSPBuf( pfucbSpace, &pspbuf ) );
                Assert( 0 == memcmp( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) ) );
                UtilMemCpy( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) );
            }
        }

        AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );
        Assert( cpgRequest >= cpgRequired );
        Assert( cpgAvailable >= cpgRequired );
        Assert( cpgNewSpace >= cpgAvailable );

        // We only need to add space to the OE if we don't already own it.
        // Also, we can only add the node to the OE if we already have at least
        // the minimum required to split given the current height of the tree because
        // the insert itself might split the tree. This only applies to re-filling the OE's
        // split buffers, because it is assumed that OE split buffers are always refilling prior
        // to AE split buffers so they're guaranteed to able to handle the split.
        // Single-level trees are safe because we're guaranteed not to split upon adding the
        // very first split buffer.
        if ( !fAlreadyOwn && // Not already in the OE.
             ( !fOwnExt || // AE can always go first.
               ( !fForcePostAddToOwnExtDbgOnly && // DEBUG only, to force the less common post-add code to run.
                 // We must have enough space to guarantee the OE node insert first.
                 ( fSpBuffersAvailableEnough || fSingleAndAvailableEnough ) ) ) )
        {
            BTUp( pfucbSpace );
            pspbuf = NULL;

            Call( errFaultAddToOe );
            Call( ErrSPIAddToOwnExt( pfucb, pgnoLast, cpgNewSpace, NULL ) );
            fAddedToOwnExt = fTrue;

            if ( fUpdatingDbRoot )
            {
                pfmp->SetOwnedFileSize( CbFileSizeOfPgnoLast( pgnoLast ) );
            }

            // Signal to upper layers that we need to rerun split buffer refill.
            wrn = ErrERRCheck( wrnSPRequestSpBufRefill );

            // Re-latch space FUCB and get split buffer pointer. If we're refilling the OE split
            // buffer and we're also adding to the OE above, some of the current split buffer may
            // have been consumed, so we can only assert that the split buffer hasn't changed if
            // we're refilling the AE split buffer.
            Call( ErrBTIGotoRoot( pfucbSpace, latchRIW ) );
            AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );
            Call( ErrSPIGetSPBuf( pfucbSpace, &pspbuf ) );
            Assert( fOwnExt || ( 0 == memcmp( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) ) ) );
            UtilMemCpy( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) );
        }

        AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );

        // Refill split buffer.
        // Space in the split buffer is not counted in pfcb->CpgAE()
        BYTE ispbuf = 0;
        if ( NULL == pfucbSpace->u.pfcb->Psplitbuf( fAvailExt ) )
        {
            // CASE 1: we have a persisted external header in the page,
            // so set up the external header and store it in the page.

            pfucbSpace->csr.UpgradeFromRIWLatch();

            // Confirm that no one snuck in underneath us.
            Assert( 0 == memcmp( &spbufBefore,
                                    PspbufSPISpaceTreeRootPage( pfucbSpace, Pcsr( pfucbSpace ) ),
                                    sizeof(SPLIT_BUFFER) ) );
            Assert( sizeof( SPLIT_BUFFER ) == pfucbSpace->kdfCurr.data.Cb() );   //  WARNING: relies on NDGetExternalHeader() in the previous assert
            UtilMemCpy( &spbufBefore, PspbufSPISpaceTreeRootPage( pfucbSpace, Pcsr( pfucbSpace ) ), sizeof(SPLIT_BUFFER) );

            SPLIT_BUFFER spbuf;
            DATA data;
            UtilMemCpy( &spbuf, &spbufBefore, sizeof(SPLIT_BUFFER) );
            ispbuf = spbuf.AddPages( pgnoLast, cpgAvailable, ispbufReplace );
            Assert( ( ispbufReplace == 0 ) || ( ispbuf == ispbufReplace ) );
            data.SetPv( &spbuf );
            data.SetCb( sizeof(spbuf) );
            Call( ErrSPIWrappedNDSetExternalHeader(
                        pfucbSpace,
                        Pcsr( pfucbSpace ),
                        &data,
                        ( pfucbSpace->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                        noderfWhole ) );
        }
        else
        {
            // CASE 2: we have an in-memory dangling split buffer,
            // so replace the split buffer directly in memory.

            // Confirm that no one snuck in underneath us.
            Assert( pspbuf == pfucbSpace->u.pfcb->Psplitbuf( fAvailExt ) );
            Assert( 0 == memcmp( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) ) );
            UtilMemCpy( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) );

            ispbuf = pspbuf->AddPages( pgnoLast, cpgAvailable, ispbufReplace );
            Assert( ( ispbufReplace == 0 ) || ( ispbuf == ispbufReplace ) );
            SPITraceSplitBufferMsg( pfucbSpace, "Added pages to" );
        }

        BTUp( pfucbSpace );
        pspbuf = NULL;

        EXTENTINFO extinfoReleased;

        Assert( ( ispbuf == 1 ) || ( ispbuf == 2 ) );
        extinfoReleased.pgnoLastInExtent = ( ispbuf == 1 ) ? spbufBefore.PgnoLastBuffer1() : spbufBefore.PgnoLastBuffer2();
        extinfoReleased.cpgExtent = ( ispbuf == 1 ) ? spbufBefore.CpgBuffer1() : spbufBefore.CpgBuffer2();
        Assert( extinfoReleased.FValid() );

        // If we released a non-empty buffer, return it to caller so the space can be returned.
        if ( extinfoReleased.CpgExtent() > 0 )
        {
            // Make sure we have enough room in the array.
            if ( parreiReleased != NULL )
            {
                Call( ( parreiReleased->ErrSetEntry( parreiReleased->Size(), extinfoReleased ) == CArray<EXTENTINFO>::ERR::errSuccess ) ?
                                                                                                  JET_errSuccess :
                                                                                                  ErrERRCheck( JET_errOutOfMemory ) );
            }
            else
            {
                // We'll leak space if there is no more space in the array to return the buffer.
                // This is unexpected as callers should pass in enough space.
                FireWall( "SpBufDroppedReleased" );
            }
        }

        Assert( !fAddedToOwnExt || ( wrn == wrnSPRequestSpBufRefill ) );

        // We only need to add space to the OE if we didn't previously own it and did not add it before.
        if ( !fAlreadyOwn && !fAddedToOwnExt )
        {
            Assert( fOwnExt );

            // This codepath should not be common anymore because, to fix the corruption bug mentioned below,
            // we are now pre-filling the split buffers with twice the strictly required number of pages so,
            // in effect, we never drop below that level. Therefore, this codepath should be rare on
            // new databases and should phase out on existing (pre-bugfix) databases as the split buffers are
            // refilled with the extra room. The two exceptions are:
            //   1- Testing, i.e., fForcePostAddToOwnExtDbgOnly.
            //   2- When both split buffers are unavailable and we must get new space. In that case,
            //      we must potentially consume the new space before adding it to the OE tree. Note
            //      that this cannot happen for the DB root because shrink always searches the split
            //      buffers of the root, so we won't have both root split buffers shelved. Also,
            //      unavailability due to both split buffers being in the shrink range cannot lead
            //      here because, when shrink is running, we only refill the split buffers off the DB's
            //      already owned available space (since we can't grow to add a new extent by definition).
            AssertTrack( fForcePostAddToOwnExtDbgOnly || ( !fSpBuffersAvailableEnough && !fUpdatingDbRoot ), "SpBufPostAddToOe" );

            // Set DB owned space (which would be incorrect under normal circumstances) to avoid
            // asserts when splitting (latching pages beyond owned space).
            if ( fUpdatingDbRoot )
            {
                pfmp->SetOwnedFileSize( CbFileSizeOfPgnoLast( pgnoLast ) );
            }

            Call( ErrSPIAddToOwnExt( pfucb, pgnoLast, cpgNewSpace, NULL ) );
            fAddedToOwnExt = fTrue;

            // Signal to upper layers that we need to rerun split buffer refill.
            wrn = ErrERRCheck( wrnSPRequestSpBufRefill );
        }

        // Refill this split buffer again if it's the OE and we added nodes to it.
        Assert( !fAddedToOwnExt || ( wrn == wrnSPRequestSpBufRefill ) );
        if ( !fOwnExt || !fAddedToOwnExt )
        {
            break;
        }

        OnDebug( fForceRefillDbgOnly = fFalse );
    }  // forever

    CallS( err );
    err = wrn;

HandleError:
    BTUp( pfucbSpace );
    pspbuf = NULL;

    AssertSPIPfucbOnRoot( pfucb );
    AssertSPIPfucbNullOrUnlatched( pfucbParent );

    return err;
}

ERR ErrSPReserveSPBufPagesForSpaceTree( FUCB *pfucb, FUCB *pfucbSpace, FUCB *pfucbParent )
{
    ERR err = JET_errSuccess;
    PIBTraceContextScope tcScope = pfucbSpace->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucbSpace );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucbSpace ) );
    tcScope->iorReason.SetIort( iortSpace );

    Expected( g_fRepair );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( pcsrNil == pfucb->pcsrRoot );
    AssertSPIPfucbNullOrUnlatched( pfucbParent );

    Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
    pfucb->pcsrRoot = Pcsr( pfucb );

    Call( ErrSPIReserveSPBufPagesForSpaceTree( pfucb, pfucbSpace, pfucbParent ) );

HandleError:
    AssertSPIPfucbNullOrUnlatched( pfucbParent );
    
    BTUp( pfucb );
    pfucb->pcsrRoot = pcsrNil;
    
    return err;
}

ERR ErrSPReserveSPBufPages(
    FUCB* const pfucb,
    const BOOL fAddlReserveForPageMoveOE,
    const BOOL fAddlReserveForPageMoveAE )
{
    ERR err = JET_errSuccess;
    FUCB* pfucbOwningTree = pfucbNil;

    // Open FUCB of the owning tree, just in case this is a space FUCB.
    Call( ErrBTIOpenAndGotoRoot(
            pfucb->ppib,
            pfucb->u.pfcb->PgnoFDP(),
            pfucb->ifmp,
            &pfucbOwningTree ) );
    Assert( pfucbOwningTree->u.pfcb == pfucb->u.pfcb );

    Call( ErrSPIReserveSPBufPages(
            pfucbOwningTree,
            pfucbNil,
            fAddlReserveForPageMoveOE ? 1 : 0,
            fAddlReserveForPageMoveAE ? 1 : 0 ) );

HandleError:
    if ( pfucbOwningTree != pfucbNil )
    {
        pfucbOwningTree->pcsrRoot->ReleasePage();
        pfucbOwningTree->pcsrRoot = pcsrNil;
        BTClose( pfucbOwningTree );
        pfucbOwningTree = pfucbNil;
    }

    return err;
}

ERR ErrSPReplaceSPBuf(
    FUCB* const pfucb,
    FUCB* const pfucbParent,
    const PGNO pgnoReplace )
{
    ERR err = JET_errSuccess;

    Assert( pfucb != pfucbNil );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( pcsrNil == pfucb->pcsrRoot );

    AssertSPIPfucbNullOrUnlatched( pfucbParent );

    Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
    pfucb->pcsrRoot = Pcsr( pfucb );

    Call( ErrSPIReserveSPBufPages(
            pfucb,
            pfucbParent,
            0,
            0,
            pgnoReplace ) );

HandleError:
    AssertSPIPfucbNullOrUnlatched( pfucbParent );
    if ( pfucb->pcsrRoot != pcsrNil )
    {
        pfucb->pcsrRoot->ReleasePage();
        pfucb->pcsrRoot = pcsrNil;
    }

    return err;
}

LOCAL ERR ErrSPIReserveSPBufPages(
    FUCB* const pfucb,
    FUCB* const pfucbParent,
    const CPG cpgAddlReserveOE,
    const CPG cpgAddlReserveAE,
    const PGNO pgnoReplace )
{
    ERR err = JET_errSuccess;
    FMP* const pfmp = &g_rgfmp[ pfucb->ifmp ];
    FCB* const pfcb = pfucb->u.pfcb;
    FUCB* pfucbParentLocal = pfucbParent;
    FUCB* pfucbOE = pfucbNil;
    FUCB* pfucbAE = pfucbNil;
    BOOL fNeedRefill = fTrue;
    CArray<EXTENTINFO> arreiReleased( 10 );

    const PGNO pgnoParentFDP = PsphSPIRootPage( pfucb )->PgnoParent();
    const PGNO pgnoLastBefore = pfmp->PgnoLast();

    AssertSPIPfucbOnRoot( pfucb );
    AssertSPIPfucbNullOrUnlatched( pfucbParent );

    Assert( ( pgnoParentFDP != pgnoNull ) || ( pfucbParent == pfucbNil ) );
    if ( ( pfucbParentLocal == pfucbNil ) && ( pgnoParentFDP != pgnoNull ) )
    {
        // Open cursor on parent FDP to get space from.  Don't GotoRoot yet, we don't want to be latched
        // while calling ErrSPIReserveSPBufPages.
        //
        Call( ErrBTIOpen(
                  pfucb->ppib,
                  pfucb->ifmp,
                  pgnoParentFDP,
                  objidNil,
                  openNormal,
                  &pfucbParentLocal,
                  fFalse ) );
        Assert( pcsrNil == pfucbParentLocal->pcsrRoot );
    }

    Call( ErrSPIOpenOwnExt( pfucb, &pfucbOE ) );
    Call( ErrSPIOpenAvailExt( pfucb, &pfucbAE ) );

    while ( fNeedRefill )
    {
        fNeedRefill = fFalse;

        // Refill the OE.
        Call( ErrSPIReserveSPBufPagesForSpaceTree(
                pfucb,
                pfucbOE,
                pfucbParentLocal,
                &arreiReleased,
                cpgAddlReserveOE,
                pgnoReplace ) );
        fNeedRefill = ( err == wrnSPRequestSpBufRefill );

        // Refill the AE.
        Call( ErrSPIReserveSPBufPagesForSpaceTree(
                pfucb,
                pfucbAE,
                pfucbParentLocal,
                &arreiReleased,
                cpgAddlReserveAE,
                pgnoReplace ) );
        fNeedRefill = fNeedRefill || ( err == wrnSPRequestSpBufRefill );

        // Return one released extent to the AE, if any.
        // Note that we can only return one extent at a time here because returning it might cause a split
        // and consume a split buffer, which would break the contract of this function, which is to return
        // with all split buffers ready to handle any split. After returning, we set the fNeedRefill flag to
        // force another iteration.
        if ( arreiReleased.Size() > 0 )
        {
            const EXTENTINFO& extinfoReleased = arreiReleased[ arreiReleased.Size() - 1 ];
            Assert( extinfoReleased.FValid() && ( extinfoReleased.CpgExtent() > 0 ) );
            Call( ErrSPIAEFreeExt( pfucb, extinfoReleased.PgnoFirst(), extinfoReleased.CpgExtent(), pfucbParentLocal ) );
            CallS( ( arreiReleased.ErrSetSize( arreiReleased.Size() - 1 ) == CArray<EXTENTINFO>::ERR::errSuccess ) ?
                                                                             JET_errSuccess :
                                                                             ErrERRCheck( JET_errOutOfMemory ) );
            fNeedRefill = fTrue;
        }
    }

    const PGNO pgnoLastAfter = pfmp->PgnoLast();

    // Unshelving normally happens as part of growing the database to add secondary extents
    // to it. However, refilling the DB root's split buffers may also grow the database while
    // not adding a secondary extent, so we need to unshelve any pages in that range here.
    if ( !pfmp->FIsTempDB() && ( pfcb->PgnoFDP() == pgnoSystemRoot ) && ( pgnoLastAfter > pgnoLastBefore ) )
    {
        Call( ErrSPIUnshelvePagesInRange( pfucb, pgnoLastBefore + 1, pgnoLastAfter ) );
        Call( ErrSPIReserveSPBufPages( pfucb, pfucbParentLocal, cpgAddlReserveOE, cpgAddlReserveAE, pgnoReplace ) );
    }

HandleError:
    Assert( ( err < JET_errSuccess ) || ( arreiReleased.Size() == 0 ) );
    for ( size_t iext = 0; iext < arreiReleased.Size(); iext++ )
    {
        BOOL fExtentFreed = fFalse;
        const EXTENTINFO& extinfo = arreiReleased[ iext ];

        // If we're failing due to low space at lower offsets during Shrink, try to return
        // the pending space anyways (best effort) to avoid leakage.
        if ( err == errSPNoSpaceBelowShrinkTarget )
        {
            // De-activate Shrink before trying to free up extents to avoid leaks.
            Assert( pfmp->FShrinkIsRunning() );
            pfmp->ResetPgnoShrinkTarget();

            Assert( extinfo.FValid() && ( extinfo.CpgExtent() > 0 ) );
            fExtentFreed = ErrSPIAEFreeExt( pfucb, extinfo.PgnoFirst(), extinfo.CpgExtent(), pfucbParentLocal ) >= JET_errSuccess;
        }

        if ( !fExtentFreed )
        {
            SPIReportSpaceLeak( pfucb, err, extinfo.PgnoFirst(), (CPG)extinfo.CpgExtent(), "SpBuffer" );
        }
    }

    if ( pfucbNil != pfucbOE )
    {
        BTClose( pfucbOE );
    }

    if ( pfucbNil != pfucbAE )
    {
        BTClose( pfucbAE );
    }

    AssertSPIPfucbNullOrUnlatched( pfucbParent );
    
    if ( ( pfucbParentLocal != pfucbNil ) && ( pfucbParentLocal != pfucbParent ) )
    {
        Expected( pfucbParent == pfucbNil );
        AssertSPIPfucbNullOrUnlatched( pfucbParentLocal );
        if ( pcsrNil != pfucbParentLocal->pcsrRoot )
        {
            Expected( fFalse );
            pfucbParentLocal->pcsrRoot->ReleasePage();
            pfucbParentLocal->pcsrRoot = pcsrNil;
        }
        BTClose( pfucbParentLocal );
        pfucbParentLocal = pfucbNil;
    }

    return err;
}


// Checks if the region is backed by storage or not (is it sparse).
// If not, writes data to the file to force allocation.
//
// There are two purposes of doing this:
// -If we are low on disk space, this function will fail, rather than when
//  the page gets flushed to disk. This can be tricky if the flush happens at
//  an awkward time (e.g. Term), which would be a new failure mode.
// -Ideally reduce file fragmentation. (This isn't actually true).

// Legend:
//  |***| the 'sparse' regions.
//  |   | indicate a regular (allocated) region.
//
//
//  +---+---+---+---+---+---+---+---+
//  |   |***|***|***|***|***|   |   |
//  +---+---+---+---+---+---+---+---+
//  ^   ^       ^   ^       ^
//  |   |       |   |       |
//  |   |       |   |       \-- The end of the sparse region.
//  |   |       |   |
//  |   |       |   +-- pgnoLastImportant (some page in the middle of the sparse region)
//  |   |       |   |
//  |   |       |   \-- parrsparseseg[0].ibEnd
//  |   |       |
//  |   |       +-- pgnoFirstImportant (some page in the middle of the sparse region).
//  |   |       |
//  |   |       \-- parrsparseseg[0].ibBegin, even though it's not 64k-aligned.
//  |   |
//  |   \-- 64-k Aligned, but the write request doesn't start here. Ideally we would start
//  |       writing here, but there is no way to guarantee that these pages aren't used by a different extent.
//  |
//  \----- Some other allocated pages before.
//
//
// Contrast that to the following:
// Note that we won't overwrite the non-sparse segments in the middle. When there are multiple sparse
// regions, we must only overwrite the specific segments which are sparse, or we can end up with spurious
// JET_errPageNotInitialized (-1019) errors during recovery because the non-sparse segments may not have
// associated trim operations in the log stream, so zeroing those pages may introduce blank pages which aren't
// going to be cleared/resolved by the redo maps.
//
//  +---+---+---+---+---+---+---+---+
//  |   |***|   |***|   |***|   |***|
//  +---+---+---+---+---+---+---+---+
//      ^       ^   ^   ^   ^
//      |       |   |   |   |
//      |       |   |   |   +-- pgnoLastImportant
//      |       |   |   |   |
//      |       |   |   |   \-- parrsparseseg[1].ibLast
//      |       |   |   |
//      |       |   |   \-- parrsparseseg[1].ibFirst
//      |       |   |
//      |       |   \-- parrsparseseg[0].ibLast
//      |       |
//      |       +-- pgnoFirstImportant
//      |       |
//      |       \-- parrsparseseg[0].ibFirst
//      |
//      \-- Do NOT write here!
//
//
//

LOCAL ERR ErrSPIAllocatePagesSlowlyIfSparse(
    _In_ IFMP ifmp,
    _In_ const PGNO pgnoFirstImportant,
    _In_ const PGNO pgnoLastImportant,
    _In_ const CPG cpgReq,
    _In_ const TraceContext& tc )
{
    ERR err = JET_errSuccess;

    Expected( cpgReq == (CPG)( pgnoLastImportant - pgnoFirstImportant + 1 ) );
    Enforce( pgnoFirstImportant <= pgnoLastImportant );

    FMP* const pfmp = &g_rgfmp[ ifmp ];

    const QWORD ibFirstRangeImportant = OffsetOfPgno( pgnoFirstImportant );
    const QWORD ibLastRangeImportant = OffsetOfPgno( pgnoLastImportant + 1 ) - 1;

    CArray<SparseFileSegment> arrsparseseg;

    // We never want to trim/zero the the database header or the root and its space trees.
    Enforce( ibFirstRangeImportant >= OffsetOfPgno( pgnoSystemRoot + 3 ) );

    if ( !pfmp->FTrimSupported() )
    {
        // No sense doing any work if we aren't running Trim!
        goto HandleError;
    }

    Call( ErrIORetrieveSparseSegmentsInRegion(  pfmp->Pfapi(),
                                                ibFirstRangeImportant,
                                                ibLastRangeImportant,
                                                &arrsparseseg  ) );

    // Go through each segment and zero them out.
    for ( size_t isparseseg = 0; isparseseg < arrsparseseg.Size(); isparseseg++ )
    {
        const SparseFileSegment& sparseseg = arrsparseseg[isparseseg];
        Enforce( sparseseg.ibFirst <= sparseseg.ibLast );

        // We don't ever expect to trim/zero the database header or the root and its space trees.
        Enforce( sparseseg.ibFirst >= OffsetOfPgno( pgnoSystemRoot + 3 ) );

        // The write will be a proper subset of the extent that's being allocated.
        Enforce( sparseseg.ibFirst >= ibFirstRangeImportant );
        Enforce( sparseseg.ibLast <= ibLastRangeImportant );

        const QWORD cbToWrite = sparseseg.ibLast - sparseseg.ibFirst + 1;
        Enforce( ( cbToWrite % g_cbPage ) == 0 );

        Call( ErrSPIWriteZeroesDatabase( ifmp, sparseseg.ibFirst, cbToWrite, tc ) );
    }

HandleError:
    return err;
}

//  Returns true if the parent is the OS File System.
//
//  Used by a would be caller of ErrSPIGetSe( which gets a secondary
//  extent from his parent ) to determine that we need to call 
//  ErrSPIGetFsSe( which instead extends the DB to add a secondary
//  extent to the Root space tree - OE/AE pgno 2/3).

LOCAL ERR FSPIParentIsFs( FUCB * const pfucb )
{
    //  check validity of input parameters
    //
    AssertSPIPfucbOnRoot( pfucb );
    Assert( pfucb->u.pfcb->FSpaceInitialized() );

    //  get parent FDP page number
    //
    const SPACE_HEADER * const psph = PsphSPIRootPage( pfucb );
    const PGNO pgnoParentFDP = psph->PgnoParent();

    //  return if parent is FS (parent is pgnoNull)
    //
    return pgnoNull == pgnoParentFDP;
}

//  Gets secondary extent from parent of the pfucb passed in (parentFDP), and 
//  adds to available extent tree if pfucbAE.
//
//  Note: Currency of pfucbAE is left on the extent added.
//
//  INPUT:  pfucb       cursor on FDP root page with RIW latch
//          pfucbAE     cursor on available extent tree
//          cpgReq      requested number of pages
//          cpgMin      minimum number of pages required
//
//
LOCAL ERR ErrSPIGetSe(
    FUCB * const    pfucb,
    FUCB * const    pfucbAE,
    const CPG       cpgReq,
    const CPG       cpgMin,
    const ULONG     fSPFlags,
    const SpacePool sppPool,
    const BOOL      fMayViolateMaxSize )
{
    ERR             err;
    PIB             *ppib                   = pfucb->ppib;
    CPG             cpgSEReq;
    CPG             cpgSEMin;
    PGNO            pgnoSELast;
    FMP             *pfmp                   = &g_rgfmp[ pfucb->ifmp ];
    const BOOL      fSplitting              = BoolSetFlag( fSPFlags, fSPSplitting );

    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagSpaceInternal,
        OSFormat(
            "%hs: Getting Req(%d) Min(%d) Flags(0x%x) P(%ws) from [<parent>] for  [0x%x:0x%x:%lu].",
            __FUNCTION__,
            cpgReq,
            cpgMin,
            fSPFlags,
            WszPoolName( sppPool ),
            pfucb->ifmp,
            ObjidFDP( pfucb ),
            PgnoFDP( pfucb ) ) );

    //  check validity of input parameters
    //
    AssertSPIPfucbOnRoot( pfucb );
    Assert( !pfmp->FReadOnlyAttach() );
    Assert( pfucbNil != pfucbAE );
    Assert( !Pcsr( pfucbAE )->FLatched() );
    Assert( pfucb->u.pfcb->FSpaceInitialized() );
    Assert( !FSPIParentIsFs( pfucb ) );

    //  get parent FDP page number
    //  and primary extent size
    //
    const SPACE_HEADER * const psph = PsphSPIRootPage( pfucb );
    const PGNO pgnoParentFDP = psph->PgnoParent();
    Assert( pgnoNull != pgnoParentFDP );

    SPIValidateCpgOwnedAndAvail( pfucb );

    //  pages of allocated extent may be used to split Owned extents and
    //  AVAILEXT trees.  If this happens, then subsequent added
    //  extent will not have to split and will be able to satisfy
    //  requested allocation.
    //

    PGNO    pgnoSEFirst = pgnoNull;
    BOOL    fSmallFDP   = fFalse;

    cpgSEMin = cpgMin;

    //  Since we are moving to a model where we have flat space (though we still
    //  maintained our hierarchy of space for compatibility) and we do not utilize
    //  parent's space, such that each extent in OE, is wholely used by the table,
    //  or wholely used by the child (LV or secondary idx) we must only do 
    //  resizing on the original request for space.
    //
    if ( fSPFlags & fSPOriginatingRequest )
    {
        //  We only respond to originating requests (as opposed to requested trickled up
        //  to us by some lower B-Tree (such as an LV to a Table)) so that we do not get into
        //  a case where an LV requests 16, table space hints up it to 64 (per hints), 
        //  then inserts 64 pgs in table tree, pushes down only 16 to LV tree.

        if ( fSPFlags & fSPExactExtent )
        {
            cpgSEReq = cpgReq;
        }
        else
        {
            if ( psph->Fv1() &&
                    ( psph->CpgPrimary() < cpgSmallFDP ) )
            {
                const CPAGE&    cpage   = pfucb->pcsrRoot->Cpage();

                //  if root page is also a leaf page or a
                //  parent-of-leaf page with just a few nodes,
                //  then this is likely a small FDP, otherwise
                //  it's almost certainly not, so don't bother
                //  checking
                //
                if ( cpage.FLeafPage()
                    || ( cpage.FParentOfLeaf() && cpage.Clines() < cpgSmallFDP ) )
                {
                    Call( ErrSPICheckSmallFDP( pfucb, &fSmallFDP ) );
                }
            }

            if ( fSmallFDP )
            {
                //  if this FDP owns just a little space,
                //  add a very small constant amount of space,
                //  unless more is requested,
                //  in order to optimize space allocation
                //  for small tables, indexes, etc.
                //
                cpgSEReq = max( cpgReq, cpgSmallGrow );
            }
            else
            {
                const CPG   cpgSEReqDefault     = ( pfmp->FIsTempDB()
                                                    && ppib->FBatchIndexCreation() ?
                                                                cpageDbExtensionDefault :
                                                                cpageSEDefault );

                Assert( cpageDbExtensionDefault != cpgSEReqDefault
                    || pgnoSystemRoot == pgnoParentFDP );

                //  if this FDP owns a lot of space, allocate a fraction of the primary
                //  extent (or more if requested), but at least a given minimum amount
                //
                const CPG cpgNextLeast = psph->Fv1() ? ( psph->CpgPrimary() / cSecFrac ) : psph->CpgLastAlloc();
                cpgSEReq = max( cpgReq, max( cpgNextLeast, cpgSEReqDefault ) );
            }
        }

        //  Get the last allocation from header
        //
        Assert( psph->FMultipleExtent() );
        const CPG   cpgLastAlloc = psph->Fv1() ? psph->CpgPrimary() : psph->CpgLastAlloc();

        cpgSEReq = max( cpgSEReq, CpgSPIGetNextAlloc( pfucb->u.pfcb->Pfcbspacehints(), cpgLastAlloc ) );
        if ( cpgSEReq != cpgLastAlloc )
        {
            //  So ... 2 less than optimal, but completely acceptable things about this
            //  tracking of the last allocation.
            //  1. If we crash after this point, we will not have really performed
            //      the allocation, yet we will pretend we have when we calculate the
            //      next growth factor.  I.e. growth will be slightly faster than expected.
            //  2. If we fail to write the last growth, we ignore it, and this means that
            //      in such failure cases, we will perform the same growth again the 
            //      next time.  i.e. growth will be slightly slower than expected.
            //  We expect neither of these cases to occur regularly, so loosing track of one
            //  allocation seems unimportant.
            (void)ErrSPIImproveLastAlloc( pfucb, psph, cpgSEReq );
        }

        //  If we are using exact extents then request min = max.
        if ( fSPFlags & fSPExactExtent )
        {
            cpgSEMin = cpgSEReq;
        }
    }
    else
    {
        //  Pass the caller's requested size right through this layer to the 
        //  next level up in the space hierarchy.  This will ensure the entire
        //  set of allocated space will trickle down to the caller, and maintain
        //  our 1-to-1 OE to AE map situation.
        //
        cpgSEReq = cpgReq;
    }

    AssertSPIPfucbOnRoot( pfucb );
    {
        FUCB *pfucbParentLocal = pfucbNil;

        // Open cursor on parent FDP to get space from.  Don't GotoRoot yet, we don't want to be latched
        // while calling ErrSPIReserveSPBufPages, but it can be a time savings to already have an FUCB
        // that we can use for multiple calls.
        //
        Call( ErrBTIOpen(
                  pfucb->ppib,
                  pfucb->ifmp,
                  pgnoParentFDP,
                  objidNil,
                  openNormal,
                  &pfucbParentLocal,
                  fFalse ) );
        Assert( pcsrNil == pfucbParentLocal->pcsrRoot );

        CallJ( ErrSPIReserveSPBufPages( pfucb, pfucbParentLocal ), CloseParent );

        // Now GotoRoot to latch the page.
        CallJ( ErrBTIGotoRoot( pfucbParentLocal, latchRIW ), CloseParent );

        pfucbParentLocal->pcsrRoot = Pcsr( pfucbParentLocal );

        AssertSPIPfucbOnRoot( pfucbParentLocal );

        //  allocate extent
        //
        CallJ( ErrSPIGetExt(
                   pfucb->u.pfcb,
                   pfucbParentLocal,
                   &cpgSEReq,
                   cpgSEMin,
                   &pgnoSEFirst,
                   fSPFlags & ( fSplitting | fSPExactExtent ),
                   0,
                   NULL,
                   NULL,
                   NULL,
                   fMayViolateMaxSize ), CloseParent );

        AssertSPIPfucbOnRoot( pfucbParentLocal );

    CloseParent:
        if ( pfucbParentLocal->pcsrRoot != pcsrNil )
        {
            pfucbParentLocal->pcsrRoot->ReleasePage();
            pfucbParentLocal->pcsrRoot = pcsrNil;
            Assert( !Pcsr( pfucbParentLocal )->FLatched() );
        }
        BTClose( pfucbParentLocal );
        pfucbParentLocal = pfucbNil;

        Call( err );
    }
    AssertSPIPfucbOnRoot( pfucb );

    SPIValidateCpgOwnedAndAvail( pfucb );

    Assert( cpgSEReq >= cpgSEMin );
    pgnoSELast = pgnoSEFirst + cpgSEReq - 1;

    BTUp( pfucbAE );

    if ( pgnoSystemRoot == pgnoParentFDP )
    {
        // Even if JET_paramEnableShrinkDatabase is currently off, we should still check to
        // see if the file was made sparse in this region before.
        if ( pfmp->Pdbfilehdr()->le_ulTrimCount > 0 )
        {
            //  We have pulled an extent from the DB root, we want to "re-commit" it from the FS (in sparse mode).
            OSTraceFMP(
                pfucb->ifmp,
                JET_tracetagSpaceInternal,
                OSFormat(
                    "%hs: Allocating Root Extent( %lu - %lu, %lu ).",
                    __FUNCTION__,
                    pgnoSEFirst,
                    pgnoSELast,
                    cpgSEReq ) );

            // This is synchronous. Might be nice way to do it via dirty BFs async.
            PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
            Call( ErrSPIAllocatePagesSlowlyIfSparse( pfucb->ifmp, pgnoSEFirst, pgnoSELast, cpgSEReq, *tcScope ) );
        }
    }

    err = ErrSPIAddSecondaryExtent(
                    pfucb,
                    pfucbAE,
                    pgnoSELast,
                    cpgSEReq,
                    cpgSEReq,
                    NULL,
                    sppPool );
    Assert( errSPOutOfOwnExtCacheSpace != err );
    Assert( errSPOutOfAvailExtCacheSpace != err );
    Call( err );

    AssertSPIPfucbOnRoot( pfucb );
    Assert( Pcsr( pfucbAE )->FLatched() );
    Assert( cpgSEReq >= cpgSEMin );

HandleError:
    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagSpaceInternal,
        OSFormat(
            "%hs: %s from [<parent>] for [0x%x:0x%x:%lu].",
            __FUNCTION__,
            ( err >= JET_errSuccess) ? "Got" : "Failed to get",
            pfucb->ifmp,
            ObjidFDP( pfucb ),
            PgnoFDP( pfucb ) ) );

    return err;
}

//  Gets a new SE from the parent (the OS FS) by extending the database, and then
//  adding the secondary extent to DbRoot available extent tree.
//
//  Note: Currency of pfucbAE is left on the extent added.
//
//  INPUT:  pfucb       cursor on FDP root page with RIW latch
//          pfucbAE     cursor on available extent tree
//          cpgReq      requested number of pages
//          cpgMin      minimum number of pages required
//
//
LOCAL ERR ErrSPIGetFsSe(
    FUCB * const    pfucb,
    FUCB * const    pfucbAE,
    const CPG       cpgReq,
    const CPG       cpgMin,
    const ULONG     fSPFlags,
    const BOOL      fExact,
    const BOOL      fPermitAsyncExtension,
    const BOOL      fMayViolateMaxSize )
{
    ERR             err = JET_errSuccess;
    PIB             *ppib = pfucb->ppib;
    CPG             cpgSEReq = cpgReq;
    CPG             cpgSEMin = cpgMin;
    CPG             cpgSEAvail = 0;
    PGNO            pgnoSELast = pgnoNull;
    FMP             *pfmp = &g_rgfmp[ pfucb->ifmp ];
    PGNO            pgnoParentFDP = pgnoNull;
    CArray<EXTENTINFO> arreiReleased;

    //  check validity of input parameters
    //
    AssertSPIPfucbOnRoot( pfucb );
    Assert( !pfmp->FReadOnlyAttach() );
    Assert( pfucbNil != pfucbAE );
    Assert( !Pcsr( pfucbAE )->FLatched() );
    Assert( pfucb->u.pfcb->FSpaceInitialized() );
    Assert( FSPIParentIsFs( pfucb ) );
    Assert( cpgMin > 0 );
    Assert( cpgReq >= cpgMin );

    //  get parent FDP page number
    //  and primary extent size
    //
    const SPACE_HEADER * const psph = PsphSPIRootPage( pfucb );
    pgnoParentFDP = psph->PgnoParent();
    Assert( pgnoNull == pgnoParentFDP );

    //  pages of allocated extent may be used to split Owned extents and
    //  AVAILEXT trees.  If this happens, then subsequent added
    //  extent will not have to split and will be able to satisfy
    //  requested allocation.
    //

    //  allocate a secondary extent from the operating system
    //  by getting page number of last owned page, extending the
    //  file as possible and adding the sized secondary extent
    //  NOTE: only one user can do this at one time.
    //
    const CPG cpgDbExtensionSize = (CPG)UlParam( PinstFromPpib( ppib ), JET_paramDbExtensionSize );

    // In repair, the initial owned size is off the physical size because the space
    // trees can't be trusted, so we might come into this function and cache an innacurate
    // pgnoPreLast below, that's why the asserts are all bypassed for repair.
    PGNO pgnoPreLast = 0;

    if ( fExact )
    {
        Expected( cpgSEMin == cpgSEReq );
        Expected( 0 == cpgSEReq % cpgDbExtensionSize );
        cpgSEMin = cpgSEReq;
    }
    else
    {
        if ( pfmp->FIsTempDB() && !ppib->FBatchIndexCreation() )
        {
            cpgSEReq = max( cpgReq, cpageSEDefault );
        }
        else
        {
            cpgSEReq = max( cpgReq, cpgDbExtensionSize );
        }

        Assert( psph->Fv1() );

        cpgSEReq = max( cpgSEReq, psph->Fv1() ? ( psph->CpgPrimary() / cSecFrac ) : psph->CpgLastAlloc() );

        OSTraceFMP(
            pfucb->ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: [%d] - %d ? %d : %d,  %d -> %d",
                __FUNCTION__,
                (ULONG)pfucb->ifmp,
                psph->Fv1(),
                psph->Fv1() ? ( psph->CpgPrimary() / cSecFrac ) : -1,
                psph->Fv1() ? -1 : psph->CpgLastAlloc(),
                cpgSEMin,
                cpgSEReq ) );

        if ( !pfmp->FIsTempDB() )
        {
            //  Round up to multiple of system parameter.

            cpgSEReq = roundup( cpgSEReq, cpgDbExtensionSize );

            Assert( cpgSEMin >= cpgMin );
            Assert( cpgSEReq >= cpgReq );
            Assert( cpgSEReq >= cpgSEMin );

            //  Fetch the end 

            //  stunned this didn't hang / fail ... but from the BTUp() below we only have pfucbAE
            //  opened right now, and pfucbOE is not open (which is what we need for this call). I
            //  hope we have the DB root space locked.
            Expected( FBFLatched( pfucb->ifmp, pgnoSystemRoot ) );
            //  this is only for rounding out allocation, we could make this optional if needed.
            pgnoPreLast = g_rgfmp[ pfucb->ifmp ].PgnoLast();
            const CPG cpgLogicalFileSize = pgnoPreLast + cpgDBReserved;

            //  cpg of database should be reasonable
            Enforce( pgnoPreLast < 0x100000000 );

            //  this includes the DB headers in this cpg size, not cpg of database.
            Enforce( g_rgfmp[ pfucb->ifmp ].CbOwnedFileSize() / g_cbPage < 0x100000000 );

#ifdef DEBUG
            {
            PGNO pgnoPreLastCheck = 0;
            if ( ErrSPGetLastPgno( ppib, pfucb->ifmp, &pgnoPreLastCheck ) >= JET_errSuccess )
            {
                Assert( pgnoPreLastCheck == pgnoPreLast || g_fRepair );
            }

            const CPG cpgFullFileSize =  pfmp->CpgOfCb( g_rgfmp[ pfucb->ifmp ].CbOwnedFileSize() );
            //  I am asserting that our offset is rounded b/c our extension will be truly off the 
            //  pgnoPreLast ... so just making sure I understand this.
            //  Given the dicey situation of the DB size during recovery, it would not surprise me
            //  if this hits during recovery at some point.
            const CPG cpgDbExt = (CPG)UlParam( PinstFromPfucb( pfucb ), JET_paramDbExtensionSize );

            OSTraceFMP(
                pfucb->ifmp,
                JET_tracetagSpaceInternal,
                OSFormat(
                    "%hs: ( %d + 2 [ + %d ] == %d ).",
                    __FUNCTION__,
                    pgnoPreLast,
                    cpgDbExt,
                    cpgFullFileSize ) );


            //  I am leaving this for the hope in the future I can perfect it, as this is defending that
            //  our true physical file from NTFS is correctly being allocated and aligned by the DB extension
            //  size.  Note this is generally holding ... just exceptional conditions are inhibiting us.

            //Assert( (CPG)pgnoPreLast + cpgDBReserved == cpgFullFileSize || 
            //          //  So due to concurrency I think there is a chance that the file
            //          //  is extended out one (or ?more?) DB extension
            //          (CPG)pgnoPreLast + cpgDbExt + cpgDBReserved == cpgFullFileSize ||
            //          g_fRepair );
            }
#endif

            //  Now we will calculate the overrage of the last extent

            const CPG cpgOverage = cpgLogicalFileSize % cpgDbExtensionSize;
            if ( cpgOverage )
            {
                //  The current true DB size isn't DB extension size aligned, this is currently known to happen for these reasons:
                //   A) [In Legacy ESE] the DB headers shifted us off by 2, so we would start at 258 true
                //          pages and then extend by 256 pages at a time. Ewww. This was fixed in Oct 2013
                //          as part of the work to make small initial DB sizes for Win Phone.
                //   B) Also the initial DB size has the ESE base schema + the JET_paramDbExtensionSize at 
                //          the time of JetCreateDatabase() ... so this will create an overage related to
                //          the size of the ESE base schema (which is about 32 - 29 pages currently ... see
                //          cpgDatabaseMinMin* vars).
                //   C) We overgrew the database to accommodate shelved pages that were close to EOF in a past
                //          growth request.
                //   D) We failed to grow by the preferred size in the past and only grew up to the minimum
                //          required at that point and now we're trying to grow again.
                //   E) The client changed JET_paramDbExtensionSize at runtime or in-between database attachments.

                const CPG cpgReduction = cpgSEReq - cpgOverage;
                if ( cpgReduction >= cpgSEMin )
                {
                    //  We have enough room (between the client request) to re-align ourselves to the DB 
                    //  extension size, so trim this extension now.  This should only happen once on the
                    //  2nd allocation of the DB, so its ok to grow slightly smaller than normal.
                    cpgSEReq = cpgReduction;
                }
                else
                {
                    //  We do not have enough room to reduce, so instead grow ... but round up to the next
                    //  extension w/ the overage accounted for.
                    Assert( cpgDbExtensionSize - cpgOverage > 0 );
                    cpgSEReq += ( cpgDbExtensionSize - cpgOverage );
                }
            }
        }
    }

    Assert( cpgSEMin >= cpgMin );
    Assert( cpgSEReq >= cpgSEMin );

    Assert( !Ptls()->fNoExtendingDuringCreateDB );

    Call( ErrSPIReserveSPBufPages( pfucb ) );
    Assert( pgnoPreLast <= g_rgfmp[ pfucb->ifmp ].PgnoLast() || g_fRepair );

    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagSpaceInternal,
        OSFormat(
            "%hs: Pre-extend stats: %I64u bytes (pgnoLast %I32u) ... %d unaligned overage",
            __FUNCTION__,
            g_rgfmp[ pfucb->ifmp ].CbOwnedFileSize(),
            g_rgfmp[ pfucb->ifmp ].PgnoLast(),
            g_rgfmp[ pfucb->ifmp ].CpgOfCb( g_rgfmp[ pfucb->ifmp ].CbOwnedFileSize() ) % cpgDbExtensionSize  ) );

    Assert( cpgSEMin >= cpgMin );
    Assert( cpgSEReq >= cpgSEMin );

    Call( ErrSPIExtendDB(
            pfucb,
            cpgSEMin,
            &cpgSEReq,
            &pgnoSELast,
            fPermitAsyncExtension,
            fMayViolateMaxSize,
            &arreiReleased,
            &cpgSEAvail ) );
    Assert( pgnoPreLast <= g_rgfmp[ pfucb->ifmp ].PgnoLast() || g_fRepair );

    //  Check the ESE DB grows to even DB extension sizes, so that subsequent allocations
    //  are aligned nicely.

    BTUp( pfucbAE );
    err = ErrSPIAddSecondaryExtent(
            pfucb,
            pfucbAE,
            pgnoSELast,
            cpgSEReq,
            cpgSEAvail,
            &arreiReleased,
            spp::AvailExtLegacyGeneralPool );
    Assert( errSPOutOfOwnExtCacheSpace != err );
    Assert( errSPOutOfAvailExtCacheSpace != err );
    Call( err );
    Assert( pgnoPreLast < g_rgfmp[ pfucb->ifmp ].PgnoLast() || g_fRepair );
    Assert( arreiReleased.Size() == 0 );

    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagSpaceInternal,
        OSFormat(
            "%hs: Post-extend stats: %I64u bytes (pgnoLast %I32u) ... %d unaligned overage",
            __FUNCTION__,
            g_rgfmp[ pfucb->ifmp ].CbOwnedFileSize(),
            g_rgfmp[ pfucb->ifmp ].PgnoLast(),
            g_rgfmp[ pfucb->ifmp ].CpgOfCb( g_rgfmp[ pfucb->ifmp ].CbOwnedFileSize() ) % cpgDbExtensionSize  ) );

#ifdef DEBUG
    //  Check the ESE DB grows to even DB extension sizes, so that subsequent allocations
    //  are aligned nicely.
    Assert( FBFLatched( pfucb->ifmp, pgnoSystemRoot ) );

#endif

    AssertSPIPfucbOnRoot( pfucb );
    Assert( Pcsr( pfucbAE )->FLatched() );
    Assert( cpgSEAvail >= cpgSEMin );

HandleError:
    return err;
}



// For the specified page range, returns the number of sparse pages in that range.
// Note: the cpg of sparse pages returned may not be contiguous from pgnoStart,
// they could be scattered sparseness.
// Sparse pages have no disk storage backing them (sparse files in NTFS/ReFS).
// Undefined results if the query is beyond EOF.
LOCAL ERR ErrSPIGetSparseInfoRange(
    _In_ FMP* const pfmp,
    _In_ const PGNO pgnoStart,
    _In_ const PGNO pgnoEnd,
    _Out_ CPG* pcpgSparse
    )
{
    ERR err = JET_errSuccess;
    CPG cpgSparseTotal = 0;
    CPG cpgAllocatedTotal = 0;
    const QWORD ibEndRange = OffsetOfPgno( pgnoEnd + 1 );
    CPG cpgRegionPrevious = 0;
    *pcpgSparse = 0;

    Assert( pgnoStart <= pgnoEnd );

    for ( PGNO pgnoQuery = pgnoStart; pgnoQuery < pgnoEnd + 1; pgnoQuery += cpgRegionPrevious )
    {
        const QWORD ibOffsetToQuery = OffsetOfPgno( pgnoQuery );
        QWORD ibStartAllocatedRegion;
        QWORD cbAllocated;

        Assert( ibOffsetToQuery < ibEndRange );

        cpgRegionPrevious = 0;

        Call( pfmp->Pfapi()->ErrRetrieveAllocatedRegion( ibOffsetToQuery, &ibStartAllocatedRegion, &cbAllocated ) );

        Assert( ibStartAllocatedRegion >= ibOffsetToQuery || 0 == ibStartAllocatedRegion );

        if ( ibStartAllocatedRegion > ibOffsetToQuery )
        {
            // We queried in a sparse (allocated) region.
            const QWORD ibEndSparse = min( ibEndRange, ibStartAllocatedRegion );
            const CPG cpgSparseThisQuery = pfmp->CpgOfCb( ibEndSparse - ibOffsetToQuery );

            cpgSparseTotal += cpgSparseThisQuery;
            cpgRegionPrevious += cpgSparseThisQuery;
        }
        else if ( 0 == ibStartAllocatedRegion )
        {
            // The query was in a sparse region at the end of the file.
            const QWORD ibEndSparse = ibEndRange;
            const CPG cpgSparseThisQuery = pfmp->CpgOfCb( ibEndSparse - ibOffsetToQuery );

            cpgSparseTotal += cpgSparseThisQuery;
            cpgRegionPrevious += cpgSparseThisQuery;
        }

        // Regardless of where the range began, the OS will return the next allocated range next, which
        // may be beyond pgnoEnd.
        // (But if the last segment of the file happened to be sparse, then it shows up as 0.)
        const QWORD ibEndAllocated = min( ibEndRange, ibStartAllocatedRegion + cbAllocated );
        if ( ibEndAllocated > 0 && ibEndAllocated <= ibEndRange )
        {
            const QWORD ibStartAllocated = min( ibEndRange, ibStartAllocatedRegion );
            Assert( ibEndAllocated >= ibStartAllocated );
            const CPG cpgAllocatedThisQuery = pfmp->CpgOfCb( ibEndAllocated - ibStartAllocated );

            cpgAllocatedTotal += cpgAllocatedThisQuery;
            cpgRegionPrevious += cpgAllocatedThisQuery;
        }

        Assert( cpgRegionPrevious > 0 );

        // The query region we just calculated should take us no farther than the end of the region.
        Assert( pgnoQuery + cpgRegionPrevious <= pgnoEnd + 1 );
    }

    // Did we account for all of the pages in the range?
    Assert( ( (CPG) ( pgnoEnd - pgnoStart + 1 ) ) == ( cpgAllocatedTotal + cpgSparseTotal ) );

    *pcpgSparse = cpgSparseTotal;

HandleError:
    return err;
}

// Trims a database region, using Sparse Files (as provided by the Operating System).
LOCAL ERR ErrSPITrimRegion(
    _In_ const IFMP ifmp,
    _In_ PIB* const ppib,
    _In_ const PGNO pgnoLast,
    _In_ const CPG cpgRequestedToTrim,
    _Out_ CPG* pcpgSparseBeforeThisExtent,
    _Out_ CPG* pcpgSparseAfterThisExtent
    )
{
    ERR err;

    *pcpgSparseBeforeThisExtent = 0;
    *pcpgSparseAfterThisExtent = 0;

    //  we are freeing an extent to the DB root, so try to give the space back to the OS if in that mode
    const PGNO pgnoStartZeroes = pgnoLast - cpgRequestedToTrim + 1;
    PGNO pgnoStartZeroesAligned = 0;
    PGNO pgnoEndZeroesAligned = 0;
    CPG cpgZeroesAligned = 0;

    Assert( cpgRequestedToTrim > 0 );

    if ( ( UlParam(PinstFromIfmp ( ifmp ), JET_paramWaypointLatency ) > 0 ) ||
         g_rgfmp[ ifmp ].FDBMScanOn() )
    {
        // Either:
        // 1. LLR is on. With the current implementation, it is unsafe to trim the database because the TrimDB
        // record is not guaranteed to be written past the waypoint.
        //
        // 2. DBScan is on:
        // -Trim runs and trims the page.
        // -Page is still cached in memory.
        // -Dbscan scrubs the page, and writes it out.
        // -Crash!
        // -A 1019 when replaying the scrub record since page is trimmed on disk.
        // -RedoMaps won't save you since trim is in the past.

        OSTraceFMP(
            ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Skipping Trim for ifmp %d because either LLR and/or DBScan is on (cpgRequested=%d).",
                __FUNCTION__,
                ifmp,
                cpgRequestedToTrim ) );

        err = JET_errSuccess;
        goto HandleError;
    }

    QWORD ibStartZeroes = 0;
    QWORD cbZeroLength = 0;
    Call( ErrIOTrimNormalizeOffsetsAndPgnos(
        ifmp,
        pgnoStartZeroes,
        cpgRequestedToTrim,
        &ibStartZeroes,
        &cbZeroLength,
        &pgnoStartZeroesAligned,
        &cpgZeroesAligned ) );

    Assert( cpgZeroesAligned <= cpgRequestedToTrim );
    const CPG cpgNotZeroedDueToAlignment = cpgRequestedToTrim - cpgZeroesAligned;
    pgnoEndZeroesAligned = pgnoStartZeroesAligned + cpgZeroesAligned - 1;

    if ( cpgNotZeroedDueToAlignment > 0 )
    {
        PERFOpt( cSPPagesNotTrimmedUnalignedPage.Add( PinstFromIfmp( ifmp ), cpgNotZeroedDueToAlignment ) );
    }

    if ( cbZeroLength > 0 )
    {
        Assert( pgnoStartZeroesAligned > 0 );
        Assert( pgnoEndZeroesAligned >= pgnoStartZeroesAligned );

        const PGNO pgnoEndRegionAligned = pgnoStartZeroesAligned + cpgZeroesAligned - 1;
        Call( ErrSPIGetSparseInfoRange( &g_rgfmp[ ifmp ], pgnoStartZeroesAligned, pgnoEndRegionAligned, pcpgSparseBeforeThisExtent ) );
        *pcpgSparseAfterThisExtent = *pcpgSparseBeforeThisExtent;

        Assert( *pcpgSparseBeforeThisExtent <= cpgZeroesAligned );

        if ( *pcpgSparseBeforeThisExtent >= cpgZeroesAligned )
        {
            OSTraceFMP(
                ifmp,
                JET_tracetagSpaceInternal,
                OSFormat(
                    "%hs: Trimming ifmp %d for [start=%d,cpg=%d] is skipped because the region is already trimmed.",
                    __FUNCTION__,
                    ifmp,
                    pgnoStartZeroesAligned,
                    cpgZeroesAligned ) );
        }
        else
        {
            PIBTraceContextScope tcScope( ppib );
            if ( tcScope->nParentObjectClass == pocNone )
            {
                tcScope->nParentObjectClass = tceNone;
            }

            Call( ErrBFPreparePageRangeForExternalZeroing( ifmp, pgnoStartZeroesAligned, cpgZeroesAligned, *tcScope ) );

            if ( g_rgfmp[ ifmp ].FLogOn() )
            {
                Call( ErrLGTrimDatabase( PinstFromIfmp( ifmp )->m_plog, ifmp, ppib, pgnoStartZeroesAligned, cpgZeroesAligned ) );
            }

            PERFOpt( cSPPagesTrimmed.Add( PinstFromIfmp( ifmp ), cpgZeroesAligned ) );

            // Update the header prior to performing the Trim optimization; While it gives a False Positive
            // if the Trim operation fails (the Header will have already increased the Trim Count), this is
            // more desireable because there is logic at Redo time that depends on the Trim Count being non-zero
            Call( ErrSPITrimUpdateDatabaseHeader( ifmp ) );

            err = ErrIOTrim( ifmp, ibStartZeroes, cbZeroLength );
            if( JET_errUnloadableOSFunctionality == err ||  //  err for trying to trim with memory mapped files
                    JET_errFeatureNotAvailable == err )     //  err for trying to trim on FAT/FAT32 (i.e. not NTFS)
            {
                *pcpgSparseAfterThisExtent = *pcpgSparseBeforeThisExtent;
                err = JET_errSuccess;
                goto HandleError;
            }
            Call( err );

            CallS( err );

            const ERR errT = ErrSPIGetSparseInfoRange( &g_rgfmp[ ifmp ], pgnoStartZeroesAligned, pgnoEndRegionAligned, pcpgSparseAfterThisExtent );

            // What sorts of failure modes does it have?
            Assert( errT >= JET_errSuccess );

            if ( errT >= JET_errSuccess )
            {

                Assert( *pcpgSparseAfterThisExtent >= 0 );
                Assert( *pcpgSparseAfterThisExtent >= *pcpgSparseBeforeThisExtent );
            }
        }
    }

HandleError:
    Assert( *pcpgSparseAfterThisExtent >= *pcpgSparseBeforeThisExtent );

    return err;
}


LOCAL ERR ErrSPIAddFreedExtent(
    FUCB        *pfucb,
    FUCB        *pfucbAE,
    const PGNO  pgnoLast,
    const CPG   cpgSize )
{
    ERR         err;
    const IFMP ifmp = pfucbAE->ifmp;

    AssertSPIPfucbOnRoot( pfucb );
    Assert( !Pcsr( pfucbAE )->FLatched() );

    const PGNO pgnoParentFDP = PsphSPIRootPage( pfucb )->PgnoParent();


    Call( ErrSPIAddToAvailExt( pfucbAE, pgnoLast, cpgSize, spp::AvailExtLegacyGeneralPool ) );
    Assert( Pcsr( pfucbAE )->FLatched() );

    if ( ( pgnoParentFDP == pgnoSystemRoot ) &&
         g_rgfmp[ ifmp ].FTrimSupported() &&
         ( ( GrbitParam( g_rgfmp[ ifmp ].Pinst(), JET_paramEnableShrinkDatabase ) & ( JET_bitShrinkDatabaseOn | JET_bitShrinkDatabaseRealtime ) ) ==
         ( JET_bitShrinkDatabaseOn | JET_bitShrinkDatabaseRealtime ) ) )
    {
        // In-line version of database trim. Swallow the errors because at this point in the
        // code an error freeing up on-disk space should not block freeing up the extent.
        CPG cpgSparseBeforeThisExtent = 0;
        CPG cpgSparseAfterThisExtent = 0;
        (void) ErrSPITrimRegion( ifmp, pfucbAE->ppib, pgnoLast, cpgSize, &cpgSparseBeforeThisExtent, &cpgSparseAfterThisExtent );
    }

HandleError:
    AssertSPIPfucbOnRoot( pfucb );

    return err;
}


//  Check that the buffer passed to ErrSPGetInfo() is big enough to accommodate
//  the information requested.
//
INLINE ERR ErrSPCheckInfoBuf( const ULONG cbBufSize, const ULONG fSPExtents )
{
    ULONG   cbUnchecked     = cbBufSize;

    if ( FSPOwnedExtent( fSPExtents ) )
    {
        if ( cbUnchecked < sizeof(CPG) )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }
        cbUnchecked -= sizeof(CPG);

        //  if list needed, ensure enough space for list sentinel
        //
        if ( FSPExtentList( fSPExtents ) )
        {
            if ( cbUnchecked < sizeof(EXTENTINFO) )
            {
                return ErrERRCheck( JET_errBufferTooSmall );
            }
            cbUnchecked -= sizeof(EXTENTINFO);
        }
    }

    if ( FSPAvailExtent( fSPExtents ) )
    {
        if ( cbUnchecked < sizeof(CPG) )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }
        cbUnchecked -= sizeof(CPG);

        //  if list needed, ensure enough space for list sentinel
        //
        if ( FSPExtentList( fSPExtents ) )
        {
            if ( cbUnchecked < sizeof(EXTENTINFO) )
            {
                return ErrERRCheck( JET_errBufferTooSmall );
            }
            cbUnchecked -= sizeof(EXTENTINFO);
        }
    }

    if ( FSPReservedExtent( fSPExtents ) )
    {
        if ( cbUnchecked < sizeof(CPG) )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }
        cbUnchecked -= sizeof(CPG);
    }

    if ( FSPShelvedExtent( fSPExtents ) )
    {
        if ( cbUnchecked < sizeof(CPG) )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }
        cbUnchecked -= sizeof(CPG);
    }

    return JET_errSuccess;
}

_When_( (return >= 0), _Post_satisfies_( *pcextMac <= *pcextMax ))
LOCAL ERR ErrSPIAddExtentInfo(
    _Inout_ BTREE_SPACE_EXTENT_INFO **  pprgext,
    _Inout_ ULONG *                                                         pcextMax,
    _Inout_ ULONG *                                                         pcextMac,
    _In_ const SpacePool                                                    sppPool,
    _In_ const PGNO                                                         pgnoLast,
    _In_ const CPG                                                          cpgExtent,
    _In_ const PGNO                                                         pgnoSpaceNode )
{
    ERR err = JET_errSuccess;

    Expected( cpgExtent || sppPool == spp::ContinuousPool );        //  not empty, except for continuous markers ...

    if ( *pcextMac >= *pcextMax )
    {
        Assert( *pcextMac == *pcextMax );
        //  Reallocate ...
        BTREE_SPACE_EXTENT_INFO * prgextToFree = *pprgext;
        const ULONG cextNewSize = (*pcextMax) * 2;
        Alloc( *pprgext = new BTREE_SPACE_EXTENT_INFO[cextNewSize] );
        *pcextMax = cextNewSize;
        memset( *pprgext, 0, sizeof(BTREE_SPACE_EXTENT_INFO)*(*pcextMax) );
        C_ASSERT( sizeof(**pprgext) == 16 );    // just double checking got level of indirection correct
        memcpy( *pprgext, prgextToFree, *pcextMac * sizeof(**pprgext) );
        delete [] prgextToFree;
    }
    Assert( *pcextMac < *pcextMax );
    AssumePREFAST( *pcextMac < *pcextMax );

    (*pprgext)[*pcextMac].iPool = (ULONG)sppPool;
    (*pprgext)[*pcextMac].pgnoLast = pgnoLast;
    (*pprgext)[*pcextMac].cpgExtent = cpgExtent;
    (*pprgext)[*pcextMac].pgnoSpaceNode = pgnoSpaceNode;

    (*pcextMac)++;

HandleError:

    return err;
}

// Note the SAL for pprgext. We used to have _Out_cap_post_count_(*pcextMax, *pcextMac), but that
// applies to an array. pprgext is a pointer to an array, and the _At_(*pprgext) says that.
// Additionally, SOMEONE from winpft said that _Out_cap_post_count_() is being deprecated.
_Pre_satisfies_( *pcextMac <= *pcextMax )
_When_( (return >= 0), _Post_satisfies_( *pcextMac <= *pcextMax ))
LOCAL ERR ErrSPIExtGetExtentListInfo(
    _Inout_ FUCB *                      pfucb,
    _At_(*pprgext, _Out_writes_to_(*pcextMax, *pcextMac)) BTREE_SPACE_EXTENT_INFO **    pprgext,
    _Inout_ ULONG *                     pcextMax,
    _Inout_ ULONG *                     pcextMac )
{
    ERR         err;
    DIB         dib;

    // Consider returning this info as well, like we used to .... 
    ULONG       cRecords        = 0;
    ULONG       cRecordsDeleted = 0;

    //  we will be traversing the entire tree in order, preread all the pages
    FUCBSetSequential( pfucb );
    FUCBSetPrereadForward( pfucb, cpgPrereadSequential );

    dib.dirflag = fDIRNull;
    dib.pos = posFirst;
    Assert( FFUCBSpace( pfucb ) );
    err = ErrBTDown( pfucb, &dib, latchReadNoTouch );

    if ( err != JET_errRecordNotFound )
    {
        Call( err );

        forever
        {
            const CSPExtentInfo cspext( pfucb );

            ++cRecords;
            if( FNDDeleted( pfucb->kdfCurr ) )
            {
                ++cRecordsDeleted;
            }
            else
            {
                Assert( ( cspext.SppPool() != spp::ShelvedPool ) || FFUCBAvailExt( pfucb ) );
                Assert( ( cspext.SppPool() != spp::ShelvedPool ) || ( pfucb->u.pfcb->PgnoFDP() == pgnoSystemRoot ) );
                Expected( ( cspext.SppPool() != spp::ShelvedPool ) || ( cspext.CpgExtent() == 1 ) );
                Expected( FSPIValidExplicitSpacePool( cspext.SppPool() ) ); //  The "virtual" sppPrimaryExt, etc type pools are unexpected here.
                //  We keep 0 page continuous pool extents left around so that we can know the next allocation should
                //  follow the space hints growth guidance.  This may actually go off some day if we see a VERY 10+
                //  year old DB that missed deleting of 0'd extents, or if we have some unknown condition where we
                //  produce these - but we don't know of that, and that suggests a problem for the Continuous Pool
                //  Marker extents.
                Expected( cspext.CpgExtent() != 0 || cspext.SppPool() == spp::ContinuousPool );
                Call( ErrSPIAddExtentInfo( pprgext, pcextMax, pcextMac, (SpacePool)cspext.SppPool(), cspext.PgnoLast(), cspext.CpgExtent(), pfucb->csr.Pgno() ) );
            }

            err = ErrBTNext( pfucb, fDIRNull );
            if ( err < 0 )
            {
                if ( err == JET_errNoCurrentRecord )
                {
                    break;  //  finished
                }
                Call( err );
            }
        }
    }

    //  clobber all done error
    Assert( err == JET_errNoCurrentRecord || err == JET_errRecordNotFound );
    err = JET_errSuccess;

HandleError:
    return err;
}


LOCAL ERR ErrSPIGetInfo(
    FUCB        *pfucb,
    CPG         *pcpgTotal,
    CPG         *pcpgReserved,
    CPG         *pcpgShelved,
    INT         *piext,
    INT         cext,
    EXTENTINFO  *rgext,
    INT         *pcextSentinelsRemaining,
    CPRINTF     * const pcprintf )
{
    ERR         err;
    DIB         dib;
    INT         iext;
    const BOOL  fExtentList     = ( cext > 0 );

    PGNO        pgnoLastSeen    = pgnoNull;
    CPG         cpgSeen         = 0;
    ULONG       cRecords        = 0;
    ULONG       cRecordsDeleted = 0;

    Assert( FFUCBSpace( pfucb ) );

    if ( fExtentList )
    {
        Assert( NULL != pcextSentinelsRemaining );
        Assert( NULL != piext ) ;
        Assert( NULL != rgext );
        iext = *piext;
    }
    else
    {
        iext = 0;
    }

    *pcpgTotal = 0;
    if ( pcpgReserved )
    {
        *pcpgReserved = 0;
    }
    if ( pcpgShelved )
    {
        *pcpgShelved = 0;
    }

    //  we will be traversing the entire tree in order, preread all the pages
    FUCBSetSequential( pfucb );
    FUCBSetPrereadForward( pfucb, cpgPrereadSequential );

    dib.dirflag = fDIRNull;
    dib.pos = posFirst;

    err = ErrBTDown( pfucb, &dib, latchReadNoTouch );

    if ( err != JET_errRecordNotFound )
    {
        Call( err );

        forever
        {
            const CSPExtentInfo cspext( pfucb );

            if( pcprintf )
            {
                CPG cpgSparse = 0;

                if ( !cspext.FEmptyExtent() )
                {
                    (void) ErrSPIGetSparseInfoRange( &g_rgfmp[ pfucb->ifmp ], cspext.PgnoFirst(), cspext.PgnoLast(), &cpgSparse );
                }

                if( pgnoLastSeen != Pcsr( pfucb )->Pgno() )
                {
                    pgnoLastSeen = Pcsr( pfucb )->Pgno();

                    ++cpgSeen;
                }

                (*pcprintf)( "%30s: %s[%5d]:\t%6d-%6d (%3d) %s%s",
                                SzNameOfTable( pfucb ),
                                SzSpaceTreeType( pfucb ),
                                Pcsr( pfucb )->Pgno(),
                                cspext.FEmptyExtent() ? 0 : cspext.PgnoFirst(),
                                cspext.FEmptyExtent() ? cspext.PgnoMarker() : cspext.PgnoLast(),
                                cspext.CpgExtent(),
                                FNDDeleted( pfucb->kdfCurr ) ? " (DEL)" : "",
                                ( cspext.ErrCheckCorrupted( ) < JET_errSuccess ) ? " (COR)" : ""
                                );
                if ( cspext.FNewAvailFormat() )
                {
                    (*pcprintf)( "  Pool: %d %s",
                                    cspext.SppPool(),
                                    cspext.FNewAvailFormat() ? "(fNewAvailFormat)" : ""
                                    );
                }
                if ( cpgSparse > 0 )
                {
                    (*pcprintf)( " cpgSparse: %3d", cpgSparse );
                }
                (*pcprintf)( "\n" );

                ++cRecords;
                if( FNDDeleted( pfucb->kdfCurr ) )
                {
                    ++cRecordsDeleted;
                }
            }

            if ( cspext.SppPool() != spp::ShelvedPool )
            {
                *pcpgTotal += cspext.CpgExtent();
            }

            if ( pcpgReserved && cspext.FNewAvailFormat() &&
                ( cspext.SppPool() == spp::ContinuousPool ) )
            {
                *pcpgReserved += cspext.CpgExtent();
            }

            BOOL fSuppressExtent = fFalse;

            if ( pcpgShelved && cspext.FNewAvailFormat() &&
                ( cspext.SppPool() == spp::ShelvedPool ) )
            {
                if ( cspext.PgnoLast() > g_rgfmp[ pfucb->ifmp ].PgnoLast() )
                {
                    Assert( cspext.PgnoFirst() > g_rgfmp[ pfucb->ifmp ].PgnoLast() );
                    *pcpgShelved += cspext.CpgExtent();
                }
                else
                {
                    fSuppressExtent = fTrue;
                }
            }

            if ( fExtentList && !fSuppressExtent )
            {
                Assert( iext < cext );

                //  be sure to leave space for the sentinels
                //  (if no more room, we still want to keep
                //  calculating page count - we just can't
                //  keep track of individual extents anymore
                //
                Assert( iext + *pcextSentinelsRemaining <= cext );
                if ( iext + *pcextSentinelsRemaining < cext )
                {
                    rgext[iext].pgnoLastInExtent = cspext.PgnoLast();
                    rgext[iext].cpgExtent = cspext.CpgExtent();
                    iext++;
                }
            }

            err = ErrBTNext( pfucb, fDIRNull );
            if ( err < 0 )
            {
                if ( err != JET_errNoCurrentRecord )
                    goto HandleError;
                break;
            }
        }
    }

    if ( fExtentList )
    {
        Assert( iext < cext );

        rgext[iext].pgnoLastInExtent = pgnoNull;
        rgext[iext].cpgExtent = 0;
        iext++;

        Assert( NULL != pcextSentinelsRemaining );
        Assert( *pcextSentinelsRemaining > 0 );
        (*pcextSentinelsRemaining)--;

        *piext = iext;
        Assert( *piext + *pcextSentinelsRemaining <= cext );
    }

    if( pcprintf )
    {
        (*pcprintf)( "%s:\t%d pages, %d nodes, %d deleted nodes\n",
                        SzNameOfTable( pfucb ),
                        cpgSeen,
                        cRecords,
                        cRecordsDeleted );
    }

    err = JET_errSuccess;

HandleError:

    return err;
}


VOID SPDumpSplitBufExtent(
    CPRINTF * const             pcprintf,
    _In_ const CHAR *           szTable,
    _In_ const CHAR *           szSpaceTree,
    _In_ const PGNO             pgnoSpaceFDP,
    _In_ const SPLIT_BUFFER *   pspbuf
    )
{
    if ( pcprintf )
    {
        if ( pspbuf->CpgBuffer1() )
        {
            (*pcprintf)( "%30s:  AE[%5d]:\t%6d-%6d (%3d)   Pool: (%s Split Buffer)\n",
                                        szTable,
                                        pgnoSpaceFDP,
                                        ( pspbuf->CpgBuffer1() <= 0 ) ? 0 : pspbuf->PgnoLastBuffer1() - pspbuf->CpgBuffer1() + 1,
                                        pspbuf->PgnoLastBuffer1(),
                                        pspbuf->CpgBuffer1(),
                                        szSpaceTree
                                        );
        }
        if ( pspbuf->CpgBuffer2() )
        {
            (*pcprintf)( "%30s:  AE[%5d]:\t%6d-%6d (%3d)   Pool: (%s Split Buffer)\n",
                                        szTable,
                                        pgnoSpaceFDP,
                                        ( pspbuf->CpgBuffer2() <= 0 ) ? 0 : pspbuf->PgnoLastBuffer2() - pspbuf->CpgBuffer2() + 1,
                                        pspbuf->PgnoLastBuffer2(),
                                        pspbuf->CpgBuffer2(),
                                        szSpaceTree
                                        );
        }
    }
}

LOCAL VOID SPIReportAnyExtentCacheError(
    FUCB *pfucb,
    CPG cpgOECached,
    CPG cpgAECached,
    CPG cpgOECounted,
    CPG cpgAECounted
    )
{
    PSTR szNameOfObject;
    FCB *pfcbTable;
    FCB *pfcbIndex;

    //  WARNING! WARNING!  This code currently does not grab the DML latch,
    //  so there doesn't appear to be any guarantee that the table and index
    //  name won't be relocated from underneath us

    if( pfucb->u.pfcb->FTypeTable() )
    {
        pfcbTable = pfucb->u.pfcb;
        pfcbIndex = pfucb->u.pfcb;
        
        if ( pfcbIndex->FSequentialIndex() )
        {
            szNameOfObject = "<SEQUENTIAL>";
        }
        else if ( pfcbTable && pfcbTable->Ptdb() && pfcbIndex->Pidb() )
        {
            szNameOfObject = pfcbTable->Ptdb()->SzIndexName(
                pfcbIndex->Pidb()->ItagIndexName(),                                                                                                                   
                pfcbIndex->FDerivedIndex() );
        }
        else
        {
            // I don't think this is supposed to happen, but just in case I missed a
            // case where it does.
            szNameOfObject = "<PRIMARY>";
        }
    }
    else if( pfucb->u.pfcb->FTypeSecondaryIndex() )
    {
        pfcbTable = pfucb->u.pfcb->PfcbTable();
        pfcbIndex = pfucb->u.pfcb;
        
        if ( pfcbTable && pfcbTable->Ptdb() && pfcbIndex->Pidb() )
        {
            szNameOfObject = pfcbTable->Ptdb()->SzIndexName(
                pfcbIndex->Pidb()->ItagIndexName(),                                                                                                                   
                pfcbIndex->FDerivedIndex() );
        }
        else
        {
            // I don't think this is supposed to happen, but just in case I missed a
            // case where it does.
            szNameOfObject = "<SECONDARY>";
        }
    }
    else if( pfucb->u.pfcb->FTypeLV() )
    {
        szNameOfObject = "<LV>";
    }
    else
    {
        szNameOfObject = "<Unknown>";
    }

    // cpgNil on input is used to represent "Don't know"
    if ( ( ( cpgOECached == cpgOECounted ) || ( cpgOECached == cpgNil ) || ( cpgOECounted == cpgNil ) ) &&
         ( ( cpgAECached == cpgAECounted ) || ( cpgAECached == cpgNil ) || ( cpgAECounted == cpgNil ) )    )
    {
        // Sizes, where known, match.  No detected error to report.
        // N = Name of Table, NO = Name of object
        OSTraceFMP(
            pfucb->ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Validation succeeded N{%hs} NO{%hs} C{%d:%d} [0x%x:0x%x:%lu]",
                __FUNCTION__,
                SzNameOfTable( pfucb ),
                szNameOfObject,
                cpgOECached,
                cpgAECached,
                pfucb->ifmp,
                ObjidFDP( pfucb ),
                PgnoFDP( pfucb ) ) );
        return;
    }        // 
        
    //
    // We have values to compare, and they showed a discrepency.
    //
    ERR err;
    CPG cpgOECached2;
    CPG cpgAECached2;
    PSTR szReasonNotValidated = NULL;
    
    // There's a decent chance this is a false positive and that we're the victim of a race condition.
    // Lets try to decrease the number of false positives.  Sleep to yield to another thread.
    // In practice, this triggers less than 10ish times a day on production servers, so don't worry about
    // the brief delay.
    UtilSleep( 50 );
    err = ErrCATGetExtentPageCounts(
        pfucb->ppib,
        pfucb->ifmp,
        pfucb->u.pfcb->ObjidFDP(),
        &cpgOECached2,
        &cpgAECached2 );
    
    switch ( err )
    {
    case JET_errSuccess:
        if ( ( cpgOECached != cpgOECached2 ) || ( cpgAECached != cpgAECached2 ) )
        {
            // Cached values changed; things are in flux.  We can't trust anything enough
            // to positively identify an error.  The value we got from counting may have changed
            // also (although we're not going to take the time to read it again).  We could
            // validate the freshly read cached values against the potentially stale counted
            // values, but I don't suspect that would actually catch errors.
            szReasonNotValidated = "Updated";
        }
        break;
        
    case JET_errRecordNotFound:
        // Must now be marked as invalid, which is a form of a race.
        szReasonNotValidated = "Marked";
        break;
        
    case JET_errNotInitialized:
        // Hmm.  We found it in the cache, but now the cache says it can't be in the cache.
        // That's not supposed to happen.
        AssertSz( fFalse, "Found in cache, now can not be found in cache." );
        szReasonNotValidated = "Unexpected";
        break;
        
    default:
        AssertSz( fFalse, "Unexpected case in switch." );
        szReasonNotValidated = "Unknown";
        break;
    }

    if ( szReasonNotValidated )
    {
        // A race (or something).  Skip reporting.
        OSTraceFMP(
            pfucb->ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Validation skipped: N{%hs} NO{%hs} R{%hs} C{%d:%d} [0x%x:0x%x:%lu]",
                __FUNCTION__,
                SzNameOfTable( pfucb ),
                szNameOfObject,
                szReasonNotValidated,
                cpgOECached,
                cpgAECached,
                pfucb->ifmp,
                ObjidFDP( pfucb ),
                PgnoFDP( pfucb ) ) );
        return;
    }

    //
    // Apparantly no race (although not _absolutely_ ruled out), and yet a mismatch.
    //
    
    // TC == Tree count, CC == Cache count.
    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagSpaceInternal,
        OSFormat(
            "%hs: Validation Failed: N{%hs} NO{%hs} TC{%d:%d} CC{%d:%d} [0x%x:0x%x:%lu].",
            __FUNCTION__,
            SzNameOfTable( pfucb ),
            szNameOfObject,
            cpgOECounted,
            cpgAECounted,
            cpgOECached,
            cpgAECached,
            pfucb->ifmp,
            ObjidFDP( pfucb ),
            PgnoFDP( pfucb ) ) );

    OSTraceSuspendGC();
    const WCHAR * rgwsz[] = {
        PfmpFromIfmp( pfucb->ifmp )->WszDatabaseName(),
        OSFormatW( L"%d",  ObjidFDP( pfucb ) ),
        OSFormatW( L"%d",  PgnoFDP( pfucb ) ),
        OSFormatW( L"%d",  cpgOECounted ),
        OSFormatW( L"%d",  cpgAECounted ),
        OSFormatW( L"%d",  cpgOECached ),
        OSFormatW( L"%d",  cpgAECached ),
        OSFormatW( L"%hs", SzNameOfTable( pfucb ) ),
        OSFormatW( L"%hs", szNameOfObject ),
    };

    UtilReportEvent(
        eventError,
        SPACE_MANAGER_CATEGORY,
        EXTENT_PAGE_COUNT_CACHE_EXTENSIVE_VALIDATION_FAILED_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        PinstFromPfucb( pfucb ) );
    OSTraceResumeGC();

    // We used to reset the page count to the new value here, which helped fix up
    // some existing problematic database instances.  That doesn't seem to be happening
    // anymore, so don't reset.  That way we'll see if the reported error is repeatable,
    // or the result of a race condition.
}
    
//  Retrieves space info, like the owned # of pages, avail # of pages.
//  NOTE:  Guaranteed contract, if you ask for info on multiple extent types, the
//  returned info in this order (skipping any not requested):
//  1) OWNED
//  2) AVAILABLE
//  3) SPLITBUFFERS
//  4) RESERVED
//  5) SHELVED
//  6) LIST
//  7) REACHABLEPAGES (must not be combined with other options)
//
ERR ErrSPGetInfo(
    PIB                       *ppib,
    const IFMP                ifmp,
    FUCB                      *pfucb,
    __out_bcount(cbMax) BYTE  *pbResult,
    const ULONG               cbMax,
    const ULONG               fSPExtentsRequested,
    const GET_CACHED_INFO     gciType,
    CPRINTF * const           pcprintf )
{
    ERR           err;
    CPG           *pcpgOwnExtTotal;
    CPG           *pcpgAvailExtTotal;
    CPG           *pcpgReservedExtTotal;
    CPG           *pcpgShelvedExtTotal;
    CPG           *pcpgSplitBuffersTotal;
    CPG           *pcpgReachableTotal;
    EXTENTINFO    *rgext;
    FUCB          *pfucbT = pfucbNil;
    FUCB          *pfucbSpace = pfucbNil;
    INT           iext;
    ULONG         cbMaxReq = 0;
    BOOL          fReadCachedValue;
    BOOL          fSetCachedValue = fFalse;
    ULONG         fSPExtents = fSPExtentsRequested; // We might want to read an extent the caller didn't request.
    CPG           cpgOwnExtTotalDummy;
    CPG           cpgAvailExtTotalDummy;
    CPG           cpgOECached = cpgNil;
    CPG           cpgAECached = cpgNil;
    ULONG         fSPExtentsRequired; // One of these must be specified.
    ULONG         fSPExtentsAllowed;  // Any of these may be specified.

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortSpace );

    // Have to specify either owned, available, split buffers, or reachable pages when you call this routine.
    fSPExtentsRequired = fSPOwnedExtent | fSPAvailExtent | fSPSplitBuffers | fSPReachablePages;

    fSPExtentsAllowed = fSPExtentsRequired;
    if ( ( pfucbNil == pfucb ) || ( ObjidFDP( pfucb ) == pgnoSystemRoot ) )
    {
        // Can only additionally get Shelved extent info for the DBRoot, not reserved or extent list.
        fSPExtentsAllowed |= ( fSPShelvedExtent );
    }
    else
    {
        // Can only additionally get Reserved and ExtentList for any other tree.
        fSPExtentsAllowed |= ( fSPReservedExtent | fSPExtentList );
    }

    // At least one required extent type specified?
    if ( ( fSPExtents & fSPExtentsRequired ) == 0 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    // Only allowed extent types specified?
    if ( ( fSPExtents & ~fSPExtentsAllowed ) != 0 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( FSPReachablePages( fSPExtents ) )
    {
        // Don't allow fSPReachablePages combined with other options.
        if ( ( fSPExtents & ~fSPReachablePages ) != 0 )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        // We must have passed in a specific tree/FUCB.
        if ( ( pfucb == pfucbNil ) || ( pfucb->pcsrRoot == pcsrNil ) || !pfucb->pcsrRoot->FLatched() )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    if ( FSPExtentList( fSPExtents ) )
    {
        AssertSz( fFalse, "This is painfully limited, let's see if we can deprecate it.");

        //  ExtentList is used at least internally in comp.cxx (in ErrCMPCopyTable()), although
        //  not widely used elsewhere (at least, not hit in any normal testing).
        //
        //  It is triggered by someone asking for ErrIsamGetTableInfo(JET_TblInfoSpaceUsage) and
        //  giving us a buffer that's bigger than 2 * sizeof(CPG) (i.e. bigger than own + avail),
        //  so it's not like we can go looking for the use of some grbit or other.
        //
        //  Perhaps AssertTrack() to see if it's hitting outside of test, but in a monitored
        //  environment?  It would be nice to clean this up so you had to explicitly ask for it
        //  rather than piggy back off of someone simply supplying a buffer that's big enough.
    }

    if ( FSPReservedExtent( fSPExtents ) )
    {
        if ( !FSPAvailExtent( fSPExtents ) )
        {
            ExpectedSz( fFalse, "initially we won't support getting reserved w/o avail." );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    if ( FSPShelvedExtent( fSPExtents ) )
    {
        if ( !FSPAvailExtent( fSPExtents ) )
        {
            ExpectedSz( fFalse, "initially we won't support getting shelved w/o avail." );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    switch ( gciType )
    {
        case gci::Forbid:
            fReadCachedValue = fFalse;
            break;

        case gci::Allow:
            if ( FSPOnlyCachedExtents( fSPExtents ) )
            {
                // Only requesting cached info.
                fReadCachedValue = fTrue;
            }
            else
            {
                fReadCachedValue = fFalse;
            }
            break;

        case gci::Require:
            if ( FSPOnlyCachedExtents( fSPExtents ) )
            {
                // Only requesting cached info.
                fReadCachedValue = fTrue;
            }
            else
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
            break;

        default:
            AssertSz( fFalse, "Unexpected case in switch.");
            Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( FSPOwnedExtent( fSPExtents ) )
    {
        cbMaxReq += sizeof( CPG );
    }
    if ( FSPAvailExtent( fSPExtents ) )
    {
        cbMaxReq += sizeof( CPG );
    }
    if ( FSPReservedExtent( fSPExtents ) )
    {
        cbMaxReq += sizeof( CPG );
    }
    if ( FSPShelvedExtent( fSPExtents ) )
    {
        cbMaxReq += sizeof( CPG );
    }
    if ( FSPSplitBuffers( fSPExtents ) )
    {
        cbMaxReq += sizeof( CPG );
    }
    if ( FSPReachablePages( fSPExtents ) )
    {
        cbMaxReq += sizeof( CPG );
    }
    if ( cbMax < cbMaxReq )
    {
        AssertSz( fFalse, "Called without the necessary buffer allocated for extents." );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    CallR( ErrSPCheckInfoBuf( cbMax, fSPExtents ) );

    memset( pbResult, '\0', cbMax );

    //  setup up return information.  owned extent is followed by available extent.
    //  This is followed by extent list for both trees
    //
    CPG * pcpgT = (CPG *)pbResult;
    pcpgOwnExtTotal = NULL;
    pcpgAvailExtTotal = NULL;
    pcpgSplitBuffersTotal = NULL;
    pcpgReservedExtTotal = NULL;
    pcpgShelvedExtTotal = NULL;
    pcpgReachableTotal = NULL;
    if ( FSPOwnedExtent( fSPExtents ) )
    {
        pcpgOwnExtTotal = pcpgT;
        pcpgT++;
    }
    if ( FSPAvailExtent( fSPExtents ) )
    {
        pcpgAvailExtTotal = pcpgT;
        pcpgT++;
    }
    if ( FSPSplitBuffers( fSPExtents ) )
    {
        pcpgSplitBuffersTotal = pcpgT;
        pcpgT++;
    }
    if ( FSPReservedExtent( fSPExtents ) )
    {
        pcpgReservedExtTotal = pcpgT;
        pcpgT++;
    }
    if ( FSPShelvedExtent( fSPExtents ) )
    {
        pcpgShelvedExtTotal = pcpgT;
        pcpgT++;
    }
    if ( FSPReachablePages( fSPExtents ) )
    {
        pcpgReachableTotal = pcpgT;
        pcpgT++;
    }
    rgext = (EXTENTINFO *)pcpgT;

    if ( pcpgOwnExtTotal )
    {
        *pcpgOwnExtTotal = 0;
    }
    if ( pcpgAvailExtTotal )
    {
        *pcpgAvailExtTotal = 0;
    }
    if ( pcpgReservedExtTotal )
    {
        *pcpgReservedExtTotal = 0;
    }
    if ( pcpgShelvedExtTotal )
    {
        *pcpgShelvedExtTotal = 0;
    }
    if ( pcpgSplitBuffersTotal )
    {
        *pcpgSplitBuffersTotal = 0;
    }
    if ( pcpgReachableTotal )
    {
        *pcpgReachableTotal = 0;
    }

    const BOOL  fExtentList     = FSPExtentList( fSPExtents );
    const INT   cext            = fExtentList ? ( ( pbResult + cbMax ) - ( (BYTE *)rgext ) ) / sizeof(EXTENTINFO) : 0;
    INT         cextSentinelsRemaining;

    cextSentinelsRemaining = 0;
    if ( fExtentList )
    {
        cextSentinelsRemaining += FSPOwnedExtent( fSPExtents ) ? 1 : 0;
        cextSentinelsRemaining += FSPAvailExtent( fSPExtents ) ? 1 : 0;
        //  Today we won't support this, because it is a bad model.  Figure
        //  out the right model.
        //cextSentinelsRemaining += FSPReservedExtent( fSPExtents ) ? 1 : 0;
        //cextSentinelsRemaining += FSPShelvedExtent( fSPExtents ) ? 1 : 0;
        Assert( cext >= cextSentinelsRemaining );
    }

    if ( fReadCachedValue )
    {
        Assert( FSPOwnedExtent( fSPExtents ) || FSPAvailExtent( fSPExtents ) );

        err = ErrCATGetExtentPageCounts(
            ppib,
            ifmp,
            ( ( pfucbNil == pfucb ) ? objidSystemRoot : ObjidFDP( pfucb ) ),
            &cpgOECached,
            &cpgAECached );

        switch ( err )
        {
        case JET_errSuccess:
            if ( ( pfucbNil != pfucb ) &&
                 ( objidSystemRoot != ObjidFDP( pfucb ) ) &&
                 ( gci::Require != gciType ) &&
                 ( BoolParam( PinstFromIfmp( ifmp ), JET_paramFlight_ExtentPageCountCacheVerifyOnly ) ) )
            {
                // Don't return the values we just read.  Calculate the values the
                // long way and double check against what we just read.  Exception
                // made for system root and those cases where the caller explicitly
                // asked for the cached value.
                break;
            }

            if ( FSPOwnedExtent( fSPExtents ) )
            {
                *pcpgOwnExtTotal = cpgOECached;
            }
            if ( FSPAvailExtent( fSPExtents ) )
            {
                *pcpgAvailExtTotal = cpgAECached;
            }
            goto HandleError;

        case JET_errRecordNotFound:
            // This objid is a value that COULD be cached, but isn't.

            if ( gci::Require == gciType )
            {
                // Caller only wanted us to read the value from the cache, and it's not there.
                Error( ErrERRCheck( JET_errObjectNotFound ) );
            }

            // Make sure we read both owned and available (even if the caller only wanted one),
            // in order to initialize the cached value.
            if ( !FSPOwnedExtent( fSPExtents ) )
            {
                Assert( NULL == pcpgOwnExtTotal );
                pcpgOwnExtTotal = &cpgOwnExtTotalDummy;
                fSPExtents |= fSPOwnedExtent;
            }
            if ( !FSPAvailExtent( fSPExtents ) )
            {
                pcpgAvailExtTotal = &cpgAvailExtTotalDummy;
                fSPExtents |= fSPAvailExtent;
            }

            fSetCachedValue = !g_rgfmp[ ifmp ].FReadOnlyAttach();

            // Now go read the slow way.
            break;

        case JET_errNotInitialized:
            // This objid is a value that CAN NOT be cached at this time, perhaps not ever.

            if ( gci::Require == gciType )
            {
                // Caller only wanted us to read the value from the cache, and it's not there.
                Error( ErrERRCheck( JET_errObjectNotFound ) );
            }

            // Now go read the slow way.
            break;

        default:
            // If we got a real error, not a warning, return it.
            Call( err );

            ExpectedSz( fFalse, "Unexpected warning return from ErrCATGetExtentPageCounts()" );

            if ( gci::Require == gciType )
            {
                // Caller only wanted us to read the value from the cache, and we didn't
                // cleanly do that.
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            // Now go read the slow way.
            break;
        }
    }

    // If the FUCB we passed in its root already latched, we don't need to open a second one
    // and latch it.
    Assert( ( pfucb == pfucbNil ) || ( pfucb->pcsrRoot == pcsrNil ) || pfucb->pcsrRoot->FLatched() );
    if ( ( pfucb == pfucbNil ) || ( pfucb->pcsrRoot == pcsrNil ) || !pfucb->pcsrRoot->FLatched() )
    {
        Assert( !FSPReachablePages( fSPExtents ) );
        if ( pfucbNil == pfucb )
        {
            err = ErrBTOpen( ppib, pgnoSystemRoot, ifmp, &pfucbT );
        }
        else
        {
            Assert( !FFUCBSpace( pfucb ) );
            err = ErrBTOpen( ppib, pfucb->u.pfcb, &pfucbT );
        }
        CallR( err );
    }
    else
    {
        Assert( !fSetCachedValue || ( ObjidFDP( pfucb ) == objidSystemRoot ) ||  ( pfucb->pcsrRoot->Latch() == latchRIW ) );
        Assert( Pcsr( pfucb ) == pfucb->pcsrRoot );
        pfucbT = pfucb;
    }

    Assert( pfucbNil != pfucbT );
    tcScope->nParentObjectClass = TceFromFUCB( pfucbT );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucbT ) );

    if ( pfucbT != pfucb )
    {
        if ( fSetCachedValue )
        {
            // Since we're going to write a value to the cache if we can, we need to hold a write
            // or RIW latch on the root page in order to have the cache and the space trees be
            // consistent.
            Call( ErrBTIGotoRoot( pfucbT, latchRIW ) );
        }
        else
        {
            Call( ErrBTIGotoRoot( pfucbT, latchReadTouch ) );
        }
        Assert( pcsrNil == pfucbT->pcsrRoot );
        pfucbT->pcsrRoot = Pcsr( pfucbT );
    }
    else
    {
        Assert( Pcsr( pfucb ) == pfucb->pcsrRoot );
        Assert( pfucb->pcsrRoot != pcsrNil );
        Assert( pfucb->pcsrRoot->FLatched() );
    }

    if ( !pfucbT->u.pfcb->FSpaceInitialized() )
    {
        //  UNDONE: Are there concurrency issues with updating the FCB
        //  while we only have a read latch?
        SPIInitFCB( pfucbT, fTrue );
        if( !FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            BFPrereadPageRange( pfucbT->ifmp, pfucbT->u.pfcb->PgnoOE(), 2, NULL, NULL, bfprfDefault, ppib->BfpriPriority( pfucbT->ifmp ), *tcScope );
        }
    }

    //  initialize number of extent list entries
    //
    iext = 0;

    if ( FSPOwnedExtent( fSPExtents ) )
    {
        Assert( NULL != pcpgOwnExtTotal );

        //  if single extent format, then free extent in external header
        //
        if ( FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            SPACE_HEADER    *psph;

            if( pcprintf )
            {
                (*pcprintf)( "\n%s: OWNEXT: single extent\n", SzNameOfTable( pfucbT ) );
            }

            Assert( pfucbT->pcsrRoot != pcsrNil );
            Assert( pfucbT->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
            Assert( pfucbT->pcsrRoot->FLatched() );
            Assert( !FFUCBSpace( pfucbT ) );

            //  get external header
            //
            NDGetExternalHeader( pfucbT, pfucbT->pcsrRoot, noderfSpaceHeader );
            Assert( sizeof( SPACE_HEADER ) == pfucbT->kdfCurr.data.Cb() );
            psph = reinterpret_cast <SPACE_HEADER *> ( pfucbT->kdfCurr.data.Pv() );

            *pcpgOwnExtTotal = psph->CpgPrimary();
            if ( fExtentList )
            {
                Assert( iext + cextSentinelsRemaining <= cext );
                if ( iext + cextSentinelsRemaining < cext )
                {
                    rgext[iext].pgnoLastInExtent = PgnoFDP( pfucbT ) + psph->CpgPrimary() - 1;
                    rgext[iext].cpgExtent = psph->CpgPrimary();
                    iext++;
                }

                Assert( iext + cextSentinelsRemaining <= cext );
                rgext[iext].pgnoLastInExtent = pgnoNull;
                rgext[iext].cpgExtent = 0;
                iext++;

                Assert( cextSentinelsRemaining > 0 );
                cextSentinelsRemaining--;

                if ( iext == cext )
                {
                    Assert( !FSPAvailExtent( fSPExtents ) );
                    Assert( 0 == cextSentinelsRemaining );
                }
                else
                {
                    Assert( iext < cext );
                    Assert( iext + cextSentinelsRemaining <= cext );
                }
            }
        }

        else
        {
            //  open cursor on owned extent tree
            //
            Call( ErrSPIOpenOwnExt( pfucbT, &pfucbSpace ) );

            if( pcprintf )
            {
                (*pcprintf)( "\n%s: OWNEXT\n", SzNameOfTable( pfucbT ) );
            }

            Call( ErrSPIGetInfo(
                pfucbSpace,
                pcpgOwnExtTotal,
                NULL,
                NULL,
                &iext,
                cext,
                rgext,
                &cextSentinelsRemaining,
                pcprintf ) );

            BTClose( pfucbSpace );
            pfucbSpace = pfucbNil;
        }
    }

    if ( FSPAvailExtent( fSPExtents ) )
    {
        Assert( NULL != pcpgAvailExtTotal );
        Assert( !fExtentList || 1 == cextSentinelsRemaining );

        //  if single extent format, then free extent in external header
        //
        if ( FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            SPACE_HEADER    *psph;

            if( pcprintf )
            {
                (*pcprintf)( "\n%s: AVAILEXT: single extent\n", SzNameOfTable( pfucbT ) );
            }

            Assert( pfucbT->pcsrRoot != pcsrNil );
            Assert( pfucbT->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
            Assert( pfucbT->pcsrRoot->FLatched() );
            Assert( !FFUCBSpace( pfucbT ) );

            //  get external header
            //
            NDGetExternalHeader( pfucbT, pfucbT->pcsrRoot, noderfSpaceHeader );
            Assert( sizeof( SPACE_HEADER ) == pfucbT->kdfCurr.data.Cb() );
            psph = reinterpret_cast <SPACE_HEADER *> ( pfucbT->kdfCurr.data.Pv() );

            *pcpgAvailExtTotal = 0;

            //  continue through rgbitAvail finding all available extents
            //
            PGNO    pgnoT           = PgnoFDP( pfucbT ) + 1;
            CPG     cpgPrimarySeen  = 1;        //  account for pgnoFDP
            PGNO    pgnoPrevAvail   = pgnoNull;
            UINT    rgbitT;

            for ( rgbitT = 0x00000001;
                rgbitT != 0 && cpgPrimarySeen < psph->CpgPrimary();
                cpgPrimarySeen++, pgnoT++, rgbitT <<= 1 )
            {
                Assert( pgnoT <= PgnoFDP( pfucbT ) + cpgSmallSpaceAvailMost );

                if ( rgbitT & psph->RgbitAvail() )
                {
                    (*pcpgAvailExtTotal)++;

                    if ( fExtentList )
                    {
                        Assert( iext + cextSentinelsRemaining <= cext );
                        if ( pgnoT == pgnoPrevAvail + 1 )
                        {
                            Assert( iext > 0 );
                            const INT   iextPrev    = iext - 1;
                            Assert( pgnoNull != pgnoPrevAvail );
                            Assert( pgnoPrevAvail == rgext[iextPrev].PgnoLast() );
                            rgext[iextPrev].pgnoLastInExtent = pgnoT;
                            rgext[iextPrev].cpgExtent++;

                            Assert( rgext[iextPrev].PgnoLast() - rgext[iextPrev].CpgExtent()
                                    >= PgnoFDP( pfucbT ) );

                            pgnoPrevAvail = pgnoT;
                        }
                        else if ( iext + cextSentinelsRemaining < cext )
                        {
                            rgext[iext].pgnoLastInExtent = pgnoT;
                            rgext[iext].cpgExtent = 1;
                            iext++;

                            Assert( iext + cextSentinelsRemaining <= cext );

                            pgnoPrevAvail = pgnoT;
                        }
                    }
                }
            }


            //  must also account for any pages that were not present in
            //  the space bitmap
            if ( psph->CpgPrimary() > cpgSmallSpaceAvailMost + 1 )
            {
                Assert( cpgSmallSpaceAvailMost + 1 == cpgPrimarySeen );
                const CPG   cpgRemaining        = psph->CpgPrimary() - ( cpgSmallSpaceAvailMost + 1 );

                (*pcpgAvailExtTotal) += cpgRemaining;

                if ( fExtentList )
                {
                    Assert( iext + cextSentinelsRemaining <= cext );

                    const PGNO  pgnoLastOfRemaining = PgnoFDP( pfucbT ) + psph->CpgPrimary() - 1;
                    if ( pgnoLastOfRemaining - cpgRemaining == pgnoPrevAvail )
                    {
                        Assert( iext > 0 );
                        const INT   iextPrev    = iext - 1;
                        Assert( pgnoNull != pgnoPrevAvail );
                        Assert( pgnoPrevAvail == rgext[iextPrev].PgnoLast() );
                        rgext[iextPrev].pgnoLastInExtent = pgnoLastOfRemaining;
                        rgext[iextPrev].cpgExtent += cpgRemaining;

                        Assert( rgext[iextPrev].PgnoLast() - rgext[iextPrev].CpgExtent()
                                >= PgnoFDP( pfucbT ) );
                    }
                    else if ( iext + cextSentinelsRemaining < cext )
                    {
                        rgext[iext].pgnoLastInExtent = pgnoLastOfRemaining;
                        rgext[iext].cpgExtent = cpgRemaining;
                        iext++;

                        Assert( iext + cextSentinelsRemaining <= cext );
                    }
                }

            }

            if ( fExtentList )
            {
                Assert( iext < cext );  //  otherwise ErrSPCheckInfoBuf would fail
                rgext[iext].pgnoLastInExtent = pgnoNull;
                rgext[iext].cpgExtent = 0;
                iext++;

                Assert( cextSentinelsRemaining > 0 );
                cextSentinelsRemaining--;

                Assert( iext + cextSentinelsRemaining <= cext );
            }
        }

        else
        {
            //  open cursor on available extent tree
            //
            Call( ErrSPIOpenAvailExt( pfucbT, &pfucbSpace ) );

            if( pcprintf )
            {
                (*pcprintf)( "\n%s: AVAILEXT\n", SzNameOfTable( pfucbT ) );
            }

            Call( ErrSPIGetInfo(
                pfucbSpace,
                pcpgAvailExtTotal,
                pcpgReservedExtTotal,
                pcpgShelvedExtTotal,
                &iext,
                cext,
                rgext,
                &cextSentinelsRemaining,
                pcprintf ) );

            BTClose( pfucbSpace );
            pfucbSpace = pfucbNil;
        }

        //  if possible, verify AvailExt total against OwnExt total
        Assert( !FSPOwnedExtent( fSPExtents )
            || ( *pcpgAvailExtTotal <= *pcpgOwnExtTotal ) );
        //  also if possible, verify ReservedExt total against AvailExt total
        Assert( !FSPReservedExtent( fSPExtents )
            || ( *pcpgReservedExtTotal <= *pcpgAvailExtTotal ) );
    }

    if ( FSPSplitBuffers( fSPExtents ) )
    {
        Assert( NULL != pcpgSplitBuffersTotal );

        //  if single extent format, then there are no split buffers
        //
        if ( FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            *pcpgSplitBuffersTotal = 0;
        }
        else
        {
            //  Get the split buffers ...
            //
            SPLIT_BUFFER  spbufOnOE;
            SPLIT_BUFFER  spbufOnAE;
            memset( (void*)&spbufOnOE, 0, sizeof(spbufOnOE) );
            memset( (void*)&spbufOnAE, 0, sizeof(spbufOnAE) );

            Call( ErrSPIOpenOwnExt( pfucbT, &pfucbSpace ) );
            Call( ErrSPIGetSPBufUnlatched( pfucbSpace, &spbufOnOE ) );
            SPDumpSplitBufExtent( pcprintf, SzNameOfTable( pfucbSpace ), "OE", pfucbSpace->u.pfcb->PgnoOE(), &spbufOnOE );
            BTClose( pfucbSpace );
            pfucbSpace = pfucbNil;

            Call( ErrSPIOpenAvailExt( pfucbT, &pfucbSpace ) );
            Call( ErrSPIGetSPBufUnlatched( pfucbSpace, &spbufOnAE ) );
            SPDumpSplitBufExtent( pcprintf, SzNameOfTable( pfucbSpace ), "AE", pfucbSpace->u.pfcb->PgnoAE(), &spbufOnAE );
            BTClose( pfucbSpace );
            pfucbSpace = pfucbNil;

            *pcpgSplitBuffersTotal = spbufOnOE.CpgBuffer1() +
                spbufOnOE.CpgBuffer2() +
                spbufOnAE.CpgBuffer1() +
                spbufOnAE.CpgBuffer2();
        }
    }

    if ( FSPReachablePages( fSPExtents ) )
    {
        Assert( pcpgReachableTotal != NULL );
        Assert( pfucb != pfucbNil );
        Assert( pfucb == pfucbT );
        Call( ErrBTIGetReachablePageCount( pfucb, pcpgReachableTotal ) );
    }

    Assert( 0 == cextSentinelsRemaining );

    // cpgNil indicates no value was read.
    if ( ( cpgOECached != cpgNil ) || ( cpgAECached != cpgNil ) )
    {
        // Can't think of a reason why one would be -1 but not the other.
        Assert( ( cpgOECached != cpgNil ) && ( cpgAECached != cpgNil ) );

        // We have values from the cache and haven't already returned them.
        // Compare them against any counted values and report any errors.

        CPG cpgOECounted = cpgNil;
        CPG cpgAECounted = cpgNil;

        if ( pcpgOwnExtTotal )
        {
            cpgOECounted = *pcpgOwnExtTotal;
        }

        if ( pcpgAvailExtTotal )
        {
            cpgAECounted = *pcpgAvailExtTotal;
        }

        SPIReportAnyExtentCacheError(
            pfucb,
            cpgOECached,
            cpgAECached,
            cpgOECounted,
            cpgAECounted
            );

    }

    if ( fSetCachedValue )
    {
        Assert( FSPOwnedExtent( fSPExtents ) );
        Assert( FSPAvailExtent( fSPExtents ) );
        Assert( !FSPReservedExtent( fSPExtents ) );
        Assert( !FSPShelvedExtent( fSPExtents ) );
        Assert( !FSPExtentList( fSPExtents ) );
        Assert( pcpgOwnExtTotal );
        Assert( pcpgAvailExtTotal );
        BOOL fStartedTransaction = fFalse;

        // We may or may not already be in a transaction, but CATSetExtentPageCounts expects
        // to be in one.
        if ( ppib->Level() == 0 )
        {
            Call( ErrDIRBeginTransaction( ppib, 54166, NO_GRBIT ) );
            fStartedTransaction = fTrue;
        }

        // Note that we're removing any pages that were in the split buffers but were added
        // to Avail.  This means that if you get the page count from the cache, it will
        // NOT include those pages, which gives you a different answer from the cache than
        // what you get by calculating the answer from the tree.
        CATSetExtentPageCounts(
            ppib,
            ifmp,
            pfucbT->u.pfcb->ObjidFDP(),
            *pcpgOwnExtTotal,
            *pcpgAvailExtTotal );

        if ( fStartedTransaction )
        {
            Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
        }
    }

HandleError:
    if ( pfucbSpace != pfucbNil )
    {
        BTClose( pfucbSpace );
    }

    if ( ( pfucbT != pfucbNil ) && ( pfucbT != pfucb ) )
    {
        pfucbT->pcsrRoot = pcsrNil;
        BTClose( pfucbT );
    }

    return err;
}

#ifdef EXPENSIVE_INLINE_EXTENT_PAGE_COUNT_CACHE_VALIDATION
ERR ErrSPIGetCpgOwnedAndAvail(
    FUCB        *pfucb,
    CPG         *pcpgOwnExtTotal,
    CPG         *pcpgAvailExtTotal
    )
{
    // This duplicates the code in ErrSPGetInfo, although significantly simplified.
    Assert( NULL != pcpgOwnExtTotal );
    Assert( NULL != pcpgAvailExtTotal );

    Assert( !FFUCBSpace( pfucb ) );
    Assert( pfucb->u.pfcb->FSpaceInitialized() );

    ERR             err = JET_errSuccess;
    FUCB            *pfucbOE = pfucbNil;
    FUCB            *pfucbAE = pfucbNil;

    *pcpgOwnExtTotal = 0;
    *pcpgAvailExtTotal = 0;

    //  if single extent format, then free extent in external header
    //
    if ( FSPIIsSmall( pfucb->u.pfcb ) )
    {
        SPACE_HEADER    *psph;

        Assert( pfucb->pcsrRoot != pcsrNil );
        Assert( pfucb->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
        Assert( pfucb->pcsrRoot->FLatched() );

        //  get external header
        //
        NDGetExternalHeader( pfucb, pfucb->pcsrRoot, noderfSpaceHeader );
        Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
        psph = reinterpret_cast <SPACE_HEADER *> ( pfucb->kdfCurr.data.Pv() );

        // Get Owned Extent info
        *pcpgOwnExtTotal = psph->CpgPrimary();

        // Get Avail Extent info
        *pcpgAvailExtTotal = 0;

        //  continue through rgbitAvail finding all available extents
        //
        PGNO    pgnoT           = PgnoFDP( pfucb ) + 1;
        CPG     cpgPrimarySeen  = 1;        //  account for pgnoFDP
        UINT    rgbitT;

        for ( rgbitT = 0x00000001;
              rgbitT != 0 && cpgPrimarySeen < psph->CpgPrimary();
              cpgPrimarySeen++, pgnoT++, rgbitT <<= 1 )
        {
            Assert( pgnoT <= PgnoFDP( pfucb ) + cpgSmallSpaceAvailMost );

            if ( rgbitT & psph->RgbitAvail() )
            {
                (*pcpgAvailExtTotal)++;
            }
        }

        //  must also account for any pages that were not present in
        //  the space bitmap
        if ( psph->CpgPrimary() > cpgSmallSpaceAvailMost + 1 )
        {
            Assert( cpgSmallSpaceAvailMost + 1 == cpgPrimarySeen );
            const CPG   cpgRemaining        = psph->CpgPrimary() - ( cpgSmallSpaceAvailMost + 1 );

            (*pcpgAvailExtTotal) += cpgRemaining;
        }
    }
    else
    {
        Assert( pfucb->u.pfcb->FInitialized() );
        SPLIT_BUFFER    spbufOnOE;
        SPLIT_BUFFER    spbufOnAE;

        memset( (void*)&spbufOnOE, 0, sizeof(spbufOnOE) );
        memset( (void*)&spbufOnAE, 0, sizeof(spbufOnAE) );

        Call( ErrSPIOpenOwnExt( pfucb->ppib, pfucb->u.pfcb, &pfucbOE ) );

        Call( ErrSPIOpenAvailExt( pfucb->ppib, pfucb->u.pfcb, &pfucbAE ) );

        Call( ErrSPIGetInfo(
                  pfucbOE,
                  pcpgOwnExtTotal,
                  NULL,
                  NULL,
                  NULL,
                  0,
                  NULL,
                  NULL,
                  NULL ) );

        Call( ErrSPIGetInfo(
                  pfucbAE,
                  pcpgAvailExtTotal,
                  NULL,
                  NULL,
                  NULL,
                  0,
                  NULL,
                  NULL,
                  NULL ) );

    }

HandleError:
    if (pfucbNil != pfucbAE)
    {
        BTClose( pfucbAE );
    }

    if (pfucbNil != pfucbOE)
    {
        BTClose( pfucbOE );
    }

    return err;
}


LOCAL VOID SPIValidateCpgOwnedAndAvail(
    FUCB * pfucb
    )
{
    ERR err;
    CPG cpgOEFromTree;
    CPG cpgAEFromTree;
    CPG cpgOEFromCache;
    CPG cpgAEFromCache;

    if ( pfucb->ppib->FUpdatingExtentPageCountCache() || pfucb->ppib->FBatchIndexCreation() )
    {
        //
        // If we're currently updating the cache, we can't call back into the cache to read it
        // because we already latched some pages we would need to relatch, and we don't handle
        // that.
        //
        return;
    }

    if( FFUCBSpace( pfucb ) )
    {
        // This can happen when we're dealing with allocating pages to
        // a space tree during splits.  When we're doing this, the space
        // tree is not in a condition to walk to get an accurate countc
        // of pages, so we can't validate anything.
        return;
    }

    // We expect to be called from places that are modifying space trees.
    // This asserts that we're in a place that's safe to do so.  Part of this
    // is an assertion that we have the FDP of the FCB locked Write or RIW,
    // so we're single threaded.
    AssertSPIPfucbOnRoot( pfucb );

    if ( !pfucb->u.pfcb->FSpaceInitialized() )
    {
        // Valid at least in this call stack:
        // 0000003e`1edff2f0 00007ffe`6225ccb2 ESE!SPIValidateCpgOwnedAndAvail_Dbg+0x4f0
        // 0000003e`1edff3c0 00007ffe`622abfd3 ESE!ErrSPFreeFDP+0x572
        // 0000003e`1edff4b0 00007ffe`622ac960 ESE!VER::ErrVERICleanOneRCE+0xce3
        // 0000003e`1edff640 00007ffe`622ad475 ESE!RCE::ErrPrepareToDeallocate+0x1b0
        // 0000003e`1edff6e0 00007ffe`622a3104 ESE!VER::ErrVERIRCEClean+0x815
        // 0000003e`1edff790 00007ffe`6243cb7e ESE!VER::VERIRCECleanProc+0x34
        // 0000003e`1edff7d0 00007ffe`c3692de3 ESE!CGPTaskManager::TMIDispatchGP+0xee
        OSTraceFMP(
            pfucb->ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Can't validate if !FSpaceInitialized [0x%x:0x%x:%lu].",
                __FUNCTION__,
                pfucb->ifmp,
                ObjidFDP( pfucb ),
                PgnoFDP( pfucb ) ) );
        return;
    }

    err = ErrSPIGetCpgOwnedAndAvail(
               pfucb,
               &cpgOEFromTree,
               &cpgAEFromTree );

    if ( JET_errSuccess > err )
    {
        OSTraceFMP(
            pfucb->ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Can't read space tree err(%d) [0x%x:0x%x:%lu].",
                __FUNCTION__,
                err,
                pfucb->ifmp,
                ObjidFDP( pfucb ),
                PgnoFDP( pfucb ) ) );
        AssertSz( fFalse, "Failed to read space tree CPGs." );
        return;
    }

    Assert( cpgAEFromTree >= 0 );
    Assert( cpgOEFromTree > 0 );


    err = ErrCATGetExtentPageCounts(
        pfucb->ppib,
        pfucb->ifmp,
        pfucb->u.pfcb->ObjidFDP(),
        &cpgOEFromCache,
        &cpgAEFromCache );

    switch ( err )
    {
        case JET_errSuccess:
            Assert( cpgAEFromCache >= 0 );
            Assert( cpgOEFromCache > 0 );
            break;

        case JET_errRecordNotFound:
            // We don't have a stored value for this objid.
            OSTraceFMP(
                pfucb->ifmp,
                JET_tracetagSpaceInternal,
                OSFormat(
                    "%hs: Not found in cache [0x%x:0x%x:%lu].",
                    __FUNCTION__,
                    pfucb->ifmp,
                    ObjidFDP( pfucb ),
                    PgnoFDP( pfucb ) ) );
            if ( pfucb->ppib->Level() > 0 )
            {
                // We can store this value now, if we're not at transaction level 0.
                //
                // We can be here at transaction level 0 in this stack:
                //
                // 000000dc`1363e9c0 00007ff8`ce1733f9 ese!SPIValidateCpgOwnedAndAvail+0x652
                // 000000dc`1363ea80 00007ff8`ce0ee8aa ese!ErrSPDeferredInitFCB+0x289
                // 000000dc`1363eb00 00007ff8`ce0f0031 ese!ErrOLDIExplicitDefragOneTable+0x8ea
                // 000000dc`1363ecf0 00007ff8`ce0f27ed ese!ErrOLDIExplicitDefragTables+0x801
                // 000000dc`1363ee00 00007ff8`ce313a39 ese!OLDDefragDb+0xe1d
                // 000000dc`1363f750 00007ff9`43916fd4 ese!UtilThreadIThreadBase+0x49
                //
                // If we're at transaction level 0 it's not safe to write the
                // new value, since ErrCATSetExtentPageCounts makes use of ambient transaction.
                //

                // The value doesn't exist in the cache.  Go ahead and try to set it.
                CATSetExtentPageCounts(
                    pfucb->ppib,
                    pfucb->ifmp,
                    ObjidFDP( pfucb ),
                    cpgOEFromTree,
                    cpgAEFromTree );

            }
            return;

        case JET_errNotInitialized:
            // It's not (currently?) possible to store a value for this objid.
            OSTraceFMP(
                pfucb->ifmp,
                JET_tracetagSpaceInternal,
                OSFormat(
                    "%hs: Uninitialized in cache [0x%x:0x%x:%lu].",
                    __FUNCTION__,
                    pfucb->ifmp,
                    ObjidFDP( pfucb ),
                    PgnoFDP( pfucb ) ) );
            return;

        default:
            OSTraceFMP(
                pfucb->ifmp,
                JET_tracetagSpaceInternal,
                OSFormat(
                    "%hs: Can't read cache, unexpected err(%d) [0x%x:0x%x:%lu].",
                    __FUNCTION__,
                    err,
                    pfucb->ifmp,
                    ObjidFDP( pfucb ),
                    PgnoFDP( pfucb ) ) );
            AssertSz( fFalse, "Failed to find in cache in unexpected way." );
            return;
    }

    SPIReportAnyExtentCacheError(
        pfucb,
        cpgOEFromCache,
        cpgAEFromCache,
        cpgOEFromTree,
        cpgAEFromTree);

}
#endif

//  Retrieves 
//  The pprgExtentList is allocated with new[].

ERR ErrSPGetExtentInfo(
    _Inout_ PIB *                       ppib,
    _In_ const IFMP                     ifmp,
    _Inout_ FUCB *                      pfucb,
    _In_ const ULONG                    fSPExtents,
    _Out_ ULONG *                       pulExtentList,
    _Deref_post_cap_(*pulExtentList) BTREE_SPACE_EXTENT_INFO ** pprgExtentList )
{
    ERR                         err = JET_errSuccess;
    BTREE_SPACE_EXTENT_INFO *   prgext = NULL;
    FUCB *                      pfucbT              = pfucbNil;

    //  must specify either owned extent or available extent (or both) to retrieve
    //
    if ( !( FSPOwnedExtent( fSPExtents ) || FSPAvailExtent( fSPExtents ) ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    Assert( ( fSPExtents & ~( fSPOwnedExtent | fSPAvailExtent ) ) == 0 );   //   no other options allowed

    if ( pfucbNil == pfucb )
    {
        err = ErrBTOpen( ppib, pgnoSystemRoot, ifmp, &pfucbT );
    }
    else
    {
        err = ErrBTOpen( ppib, pfucb->u.pfcb, &pfucbT );
    }
    CallR( err );
    Assert( pfucbNil != pfucbT );

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope( );
    tcScope->nParentObjectClass = TceFromFUCB( pfucbT );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucbT ) );
    tcScope->iorReason.SetIort( iortSpace );

    Call( ErrBTIGotoRoot( pfucbT, latchReadTouch ) );
    Assert( pcsrNil == pfucbT->pcsrRoot );
    pfucbT->pcsrRoot = Pcsr( pfucbT );

    if ( !pfucbT->u.pfcb->FSpaceInitialized() )
    {
        //  UNDONE: Are there cuncurrency issues with updating the FCB
        //  while we only have a read latch?
        SPIInitFCB( pfucbT, fTrue );
        if( !FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            BFPrereadPageRange( pfucbT->ifmp, pfucbT->u.pfcb->PgnoOE(), 2, bfprfDefault, ppib->BfpriPriority( pfucbT->ifmp ), *tcScope );
        }
    }

    ULONG cextMax = 10;
    ULONG cextMac = 0;
    Alloc( prgext = new BTREE_SPACE_EXTENT_INFO[cextMax] );
    memset( prgext, 0, sizeof(BTREE_SPACE_EXTENT_INFO)*cextMax );

    if ( FSPOwnedExtent( fSPExtents ) )
    {
        Assert( !FSPAvailExtent( fSPExtents ) );

        //  if single extent format, then free extent in external header
        //
        if ( FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            SPACE_HEADER    *psph;

            Assert( pfucbT->pcsrRoot != pcsrNil );
            Assert( pfucbT->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
            Assert( pfucbT->pcsrRoot->FLatched() );
            Assert( !FFUCBSpace( pfucbT ) );

            //  get external header
            //
            NDGetExternalHeader( pfucbT, pfucbT->pcsrRoot, noderfSpaceHeader );
            Assert( sizeof( SPACE_HEADER ) == pfucbT->kdfCurr.data.Cb() );
            psph = reinterpret_cast <SPACE_HEADER *> ( pfucbT->kdfCurr.data.Pv() );

            Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::PrimaryExt, PgnoFDP( pfucb ) + psph->CpgPrimary() - 1, psph->CpgPrimary(), PgnoFDP( pfucb ) ) );
        }
        else
        {
            //  open cursor on owned extent tree
            //
            FUCB    *pfucbOE = pfucbNil;

            Call( ErrSPIOpenOwnExt( pfucbT, &pfucbOE ) );

            Call( ErrSPIExtGetExtentListInfo( pfucbOE, &prgext, &cextMax, &cextMac ) );

            Assert( pfucbOE != pfucbNil );
            BTClose( pfucbOE );
            Call( err );
        }

        // We've succeeded, set out params ...
        *pulExtentList = cextMac;
        *pprgExtentList = prgext;
    }

    if ( FSPAvailExtent( fSPExtents ) )
    {
        Assert( !FSPOwnedExtent( fSPExtents ) );

        SPLIT_BUFFER                spbufOnOE;
        SPLIT_BUFFER                spbufOnAE;

        memset( (void*)&spbufOnOE, 0, sizeof(spbufOnOE) );
        memset( (void*)&spbufOnAE, 0, sizeof(spbufOnAE) );

        //  if single extent format, then free extent in external header
        //
        if ( FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            SPACE_HEADER    *psph;

            Assert( pfucbT->pcsrRoot != pcsrNil );
            Assert( pfucbT->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
            Assert( pfucbT->pcsrRoot->FLatched() );
            Assert( !FFUCBSpace( pfucbT ) );

            //  get external header
            //
            NDGetExternalHeader( pfucbT, pfucbT->pcsrRoot, noderfSpaceHeader );
            Assert( sizeof( SPACE_HEADER ) == pfucbT->kdfCurr.data.Cb() );
            psph = reinterpret_cast <SPACE_HEADER *> ( pfucbT->kdfCurr.data.Pv() );

            //  continue through rgbitAvail finding all available extents
            //
            PGNO    pgnoT           = PgnoFDP( pfucbT ) + 1;
            CPG     cpgPrimarySeen  = 1;        //  account for pgnoFDP
            UINT    rgbitT;

            CPG     cpgAccum        = 0;
            ULONG   pgnoLastAccum   = 0;

            for ( rgbitT = 0x00000001;
                rgbitT != 0 && cpgPrimarySeen < psph->CpgPrimary();
                cpgPrimarySeen++, pgnoT++, rgbitT <<= 1 )
            {
                Assert( pgnoT <= PgnoFDP( pfucbT ) + cpgSmallSpaceAvailMost );

                if ( rgbitT & psph->RgbitAvail() )
                {
                    Assert( pgnoNull != pgnoT );
                    Assert( cextMac == 0 || prgext[cextMac-1].pgnoLast - prgext[cextMac-1].cpgExtent >= PgnoFDP( pfucbT ) );
                    Assert( cpgAccum < cpgPrimarySeen );
                    Assert( pgnoLastAccum == 0 || pgnoLastAccum + 1 == pgnoT );
                    cpgAccum++;
                    pgnoLastAccum = pgnoT;
                }
                else
                {
                    if ( cpgAccum )
                    {
                        Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::PrimaryExt, pgnoLastAccum, cpgAccum, PgnoFDP( pfucb ) ) );
                    }
                    cpgAccum = 0;
                    pgnoLastAccum = 0;
                }
            }

            //  must also account for any pages that were not present in
            //  the space bitmap
            if ( psph->CpgPrimary() > cpgSmallSpaceAvailMost + 1 )
            {
                Assert( cpgSmallSpaceAvailMost + 1 == cpgPrimarySeen );
                const CPG   cpgRemaining        = psph->CpgPrimary() - ( cpgSmallSpaceAvailMost + 1 );
                const PGNO  pgnoLastOfRemaining = PgnoFDP( pfucbT ) + psph->CpgPrimary() - 1;
                if ( cextMac > 0 )
                {
                    Assert( pgnoLastAccum == prgext[cextMac-1].pgnoLast );
                    Assert( prgext[cextMac-1].pgnoLast - prgext[cextMac-1].cpgExtent >= PgnoFDP( pfucbT ) );
                }
                Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::PrimaryExt, pgnoLastOfRemaining, cpgRemaining, PgnoFDP( pfucb ) ) );
            }
            else if ( cpgAccum )
            {
                //  We forgot to accumulate the last extent b/c we were checking for space beyond the bitmap ... 
                Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::PrimaryExt, pgnoLastAccum, cpgAccum, PgnoFDP( pfucb ) ) );
            }
        }
        else
        {
            //  open cursor on available extent tree
            //
            FUCB    *pfucbAE = pfucbNil;

            Call( ErrSPIOpenAvailExt( pfucbT, &pfucbAE ) );

            //  Get the split buffers ...
            //

            //  open cursor on owned extent tree, to get split buffer ...
            FUCB    *pfucbOE = pfucbNil;

            err = ErrSPIOpenOwnExt( pfucbT, &pfucbOE );
            if ( err >= JET_errSuccess )
            {
                err = ErrSPIGetSPBufUnlatched( pfucbOE, &spbufOnOE );
            }

            if ( pfucbOE != pfucbNil )
            {
                BTClose( pfucbOE );
            }

            if ( err >= JET_errSuccess )
            {
                err = ErrSPIGetSPBufUnlatched( pfucbAE, &spbufOnAE );
            }

            //  Now process the info from the space tree split buffers collected above ...

            if ( spbufOnOE.CpgBuffer1() )
            {
                Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::OwnedTreeAvail, spbufOnOE.PgnoLastBuffer1(), spbufOnOE.CpgBuffer1(), pfucbAE->u.pfcb->PgnoOE() ) );
            }
            if ( spbufOnOE.CpgBuffer2() )
            {
                Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::OwnedTreeAvail, spbufOnOE.PgnoLastBuffer2(), spbufOnOE.CpgBuffer2(), pfucbAE->u.pfcb->PgnoOE() ) );
            }
            if ( spbufOnAE.CpgBuffer1() )
            {
                Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::AvailTreeAvail, spbufOnAE.PgnoLastBuffer1(), spbufOnAE.CpgBuffer1(), pfucbAE->u.pfcb->PgnoAE() ) );
            }
            if ( spbufOnAE.CpgBuffer2() )
            {
                Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::AvailTreeAvail, spbufOnAE.PgnoLastBuffer2(), spbufOnAE.CpgBuffer2(), pfucbAE->u.pfcb->PgnoAE() ) );
            }

            //  Retrieve the list of regular extents
            //

            if ( err >= JET_errSuccess )
            {
                Call( ErrSPIExtGetExtentListInfo( pfucbAE, &prgext, &cextMax, &cextMac ) );
            }

            Assert( pfucbAE != pfucbNil );
            BTClose( pfucbAE );
            Call( err );

            //  Validate
            //
            for( ULONG iext = 0; iext < cextMac; iext++ )
            {
                BTREE_SPACE_EXTENT_INFO * pext = &prgext[iext];
                Assert( pext->iPool != (ULONG)spp::OwnedTreeAvail || iext <= 4 );    //   Must be first extents
                Assert( pext->iPool != (ULONG)spp::AvailTreeAvail || iext <= 4 );    //   Must be first extents
                if ( pext->iPool == (ULONG)spp::PrimaryExt )
                {
                    Assert( iext == 0 || prgext[iext-1].iPool != (ULONG)spp::AvailExtLegacyGeneralPool );
                    Assert( iext == 0 || prgext[iext-1].iPool != (ULONG)spp::ContinuousPool );
                    Assert( prgext[iext-1].iPool != (ULONG)spp::ShelvedPool );
                }
            }
        }

        // We've succeeded, set out params ...
        *pulExtentList = cextMac;
        *pprgExtentList = prgext;
    }

HandleError:

    Expected( pfucbNil != pfucbT ); //  codepaths up to (inclusive) opening the cursor return immediately (i.e., no HandleError cleanup).

    if ( pfucbT != pfucbNil )
    {
        pfucbT->pcsrRoot = pcsrNil;
        BTClose( pfucbT );
    }

    return err;
}



LOCAL ERR ErrSPITrimOneAvailableExtent(
    _In_ FUCB* pfucb,
    _Out_ CPG* pcpgTotalAvail,
    _Out_ CPG* pcpgTotalAvailSparseBefore,
    _Out_ CPG* pcpgTotalAvailSparseAfter,
    _In_ CPRINTF* const pcprintf )
{
    ERR         err;
    DIB         dib;

    Assert( pcprintf );

    CPG         cpgAvailTotal   = 0;
    CPG         cpgAvailSparseBefore    = 0;
    CPG         cpgAvailSparseAfter     = 0;

    *pcpgTotalAvail = 0;
    *pcpgTotalAvailSparseBefore = 0;
    *pcpgTotalAvailSparseAfter = 0;

    //  we will be traversing the entire tree in order, preread all the pages
    FUCBSetSequential( pfucb );
    FUCBSetPrereadForward( pfucb, cpgPrereadSequential );

    dib.dirflag = fDIRNull;
    dib.pos = posFirst;
    Assert( FFUCBAvailExt( pfucb ) );
    err = ErrBTDown( pfucb, &dib, latchReadNoTouch );

    if ( err != JET_errRecordNotFound )
    {
        Call( err );

        forever
        {
            const CSPExtentInfo cspext( pfucb );

            // Please handle new pools.
            Expected( ( cspext.SppPool() == spp::AvailExtLegacyGeneralPool ) || ( cspext.SppPool() == spp::ShelvedPool ) );

            // Do not trim shelved pages, obviously.
            if ( cspext.SppPool() != spp::ShelvedPool )
            {
                CPG cpgSparseBeforeThisExtent = 0;
                CPG cpgSparseAfterThisExtent = 0;
                QWORD ibStartZeroes = 0;
                QWORD cbZeroLength = 0;
                PGNO pgnoStartRegionAligned = 0;
                CPG cpgRegionAligned = 0;

                if ( !cspext.FEmptyExtent() )
                {
                    // We have a lock on the Available Extents, but the AE may not be 64k-aligned.
                    Call( ErrIOTrimNormalizeOffsetsAndPgnos(
                        pfucb->ifmp,
                        cspext.PgnoFirst(),
                        cspext.CpgExtent(),
                        &ibStartZeroes,
                        &cbZeroLength,
                        &pgnoStartRegionAligned,
                        &cpgRegionAligned ) );
                }

                // We can't trim a region that's smaller than a 64k block.
                if ( cpgRegionAligned != 0 )
                {
                    Assert( !cspext.FEmptyExtent() );

                    const PGNO pgnoEndRegionAligned = cspext.FEmptyExtent() ? 0 : pgnoStartRegionAligned + cpgRegionAligned - 1;

                    if ( !cspext.FEmptyExtent() )
                    {
                    }

                    (*pcprintf)( "%30s: %s[%5d]:\t%6d-%6d (%3d);\tA%6d-%6d (%3d) %s%s",
                                 SzNameOfTable( pfucb ),
                                 SzSpaceTreeType( pfucb ),
                                 Pcsr( pfucb )->Pgno(),
                                 // Unaligned:
                                 cspext.FEmptyExtent() ? 0 : cspext.PgnoFirst(),
                                 cspext.FEmptyExtent() ? cspext.PgnoMarker() : cspext.PgnoLast(),
                                 cspext.CpgExtent(),
                                 // Aligned region:
                                 pgnoStartRegionAligned,
                                 pgnoEndRegionAligned,
                                 cpgRegionAligned,
                                 FNDDeleted( pfucb->kdfCurr ) ? " (DEL)" : "",
                                 ( cspext.ErrCheckCorrupted( ) < JET_errSuccess ) ? " (COR)" : ""
                                 );

                    (*pcprintf)( " cpgSparseBefore: %3d", cpgSparseBeforeThisExtent );

                {
                    PIB pibFake;
                    pibFake.m_pinst = g_rgfmp[ pfucb->ifmp ].Pinst();
                    Call( ErrSPITrimRegion( pfucb->ifmp, &pibFake, pgnoEndRegionAligned, cpgRegionAligned, &cpgSparseBeforeThisExtent, &cpgSparseAfterThisExtent ) );

                    const CPG cpgSparseNewlyTrimmed = cpgSparseAfterThisExtent - cpgSparseBeforeThisExtent;
                    (*pcprintf)( " cpgSparseAfter: %3d cpgSparseNew: %3d", cpgSparseAfterThisExtent, cpgSparseNewlyTrimmed );

                    cpgAvailSparseBefore += cpgSparseBeforeThisExtent;
                    cpgAvailSparseAfter += cpgSparseAfterThisExtent;
                }

                    (*pcprintf)( "\n" );
                }

                cpgAvailTotal += cspext.CpgExtent();
            }

            err = ErrBTNext( pfucb, fDIRNull );
            if ( err < 0 )
            {
                if ( err != JET_errNoCurrentRecord )
                {
                    goto HandleError;
                }
                break;
            }
        }
    }

    *pcpgTotalAvail += cpgAvailTotal;
    *pcpgTotalAvailSparseBefore += cpgAvailSparseBefore;
    *pcpgTotalAvailSparseAfter += cpgAvailSparseAfter;

    if( pcprintf )
    {
        const CPG cpgSparseNewlyTrimmed = cpgAvailSparseAfter - cpgAvailSparseBefore;
        (*pcprintf)( "%s:\t%d trimmed (before), %d trimmed (after), %d pages newly trimmed.\n",
                     SzNameOfTable( pfucb ),
                     cpgAvailSparseBefore,
                     cpgAvailSparseAfter,
                     cpgSparseNewlyTrimmed );
    }

    err = JET_errSuccess;

HandleError:
    return err;
}

// Iterates over the Avail Extents in the given FMP, trimming what it can.
ERR ErrSPTrimRootAvail(
    _In_ PIB *ppib,
    _In_ const IFMP ifmp,
    _In_ CPRINTF * const pcprintf,
    _Out_opt_ CPG * const pcpgTrimmed )
{
    ERR err;
    CPG cpgAvailExtTotal = 0;
    CPG cpgAvailExtTotalSparseBefore = 0;
    CPG cpgAvailExtTotalSparseAfter = 0;
    FUCB *pfucbT = pfucbNil;
    FUCB *pfucbAE = pfucbNil;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortSpace );

    SPLIT_BUFFER    spbufOnAE;

    memset( (void*)&spbufOnAE, 0, sizeof(spbufOnAE) );

    Call( ErrBTIOpenAndGotoRoot( ppib, pgnoSystemRoot, ifmp, &pfucbT ) );
    AssertSPIPfucbOnRoot( pfucbT );

    if ( !pfucbT->u.pfcb->FSpaceInitialized() )
    {
        //  UNDONE: Are there cuncurrency issues with updating the FCB
        //  while we only have a read latch?
        SPIInitFCB( pfucbT, fTrue );
        if( !FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            BFPrereadPageRange( pfucbT->ifmp, pfucbT->u.pfcb->PgnoAE(), 2, bfprfDefault, ppib->BfpriPriority( pfucbT->ifmp ), *tcScope );
        }
    }

    AssertSz( !FSPIIsSmall( pfucbT->u.pfcb ), "Database root table should not be Small Space!" );

    //  open cursor on available extent tree
    //
    Call( ErrSPIOpenAvailExt( pfucbT, &pfucbAE ) );

    //  Get the split buffers ...
    //
    Call( ErrSPIGetSPBufUnlatched( pfucbAE, &spbufOnAE ) );

    Call( ErrSPITrimOneAvailableExtent(
        pfucbAE,
        &cpgAvailExtTotal,
        &cpgAvailExtTotalSparseBefore,
        &cpgAvailExtTotalSparseAfter,
        pcprintf ) );

    Assert( cpgAvailExtTotalSparseBefore <= cpgAvailExtTotal );
    Assert( cpgAvailExtTotalSparseBefore <= cpgAvailExtTotalSparseAfter );
    Assert( cpgAvailExtTotalSparseAfter <= cpgAvailExtTotal );

HandleError:
    if ( pfucbAE != pfucbNil )
    {
        BTClose( pfucbAE );
    }

    Assert( pfucbNil != pfucbT || err < JET_errSuccess );

    if ( pfucbT != pfucbNil )
    {
        pfucbT->pcsrRoot = pcsrNil;
        BTClose( pfucbT );
    }

    if ( pcpgTrimmed != NULL )
    {
        *pcpgTrimmed = 0;
        if ( ( err >= JET_errSuccess ) &&
            ( cpgAvailExtTotalSparseBefore > cpgAvailExtTotalSparseAfter ) )
        {
            *pcpgTrimmed = cpgAvailExtTotalSparseBefore - cpgAvailExtTotalSparseAfter;
        }
    }

    return err;
}


//  HACK: this is a hack function to force a dummy logged update
//  in the database to ensure that after recovery, the dbtime in the
//  db header is greater than any of the dbtimes in pages belonging
//  to non-logged indexes
//
ERR ErrSPDummyUpdate( FUCB * pfucb )
{
    ERR             err;
    SPACE_HEADER    sph;
    DATA            data;

    Assert( !Pcsr( pfucb )->FLatched() );

    CallR( ErrBTIGotoRoot( pfucb, latchRIW ) );

    //  get external header
    //
    NDGetExternalHeader( pfucb, noderfSpaceHeader );
    Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );

    //  make copy of header
    //
    UtilMemCpy( &sph, pfucb->kdfCurr.data.Pv(), sizeof(sph) );

    //  upgrade to write latch
    //
    Pcsr( pfucb )->UpgradeFromRIWLatch();

    //  perform dummy logged update to force the dbtime
    //  counter to be incremented over any non-logged
    //  updates
    //
    data.SetPv( &sph );
    data.SetCb( sizeof(sph) );
    err = ErrSPIWrappedNDSetExternalHeader(
        pfucb,
        Pcsr( pfucb ),
        &data,
        fDIRNull,
        noderfSpaceHeader );

    //  release latch regardless of error
    //
    BTUp( pfucb );

    return err;
}


#ifdef SPACECHECK

LOCAL ERR ErrSPIValidFDP( PIB *ppib, IFMP ifmp, PGNO pgnoFDP )
{
    ERR         err;
    FUCB        *pfucb = pfucbNil;
    FUCB        *pfucbOE = pfucbNil;
    DIB         dib;

    Assert( pgnoFDP != pgnoNull );

    //  get temporary FUCB
    //
    Call( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb ) );
    Assert( pfucbNil != pfucb );

    Assert( pfucb->u.pfcb->FInitialized() );

    if ( !FSPIIsSmall( pfucb->u.pfcb ) )
    {
        //  open cursor on owned extent
        //
        Call( ErrSPIOpenOwnExt( ppib, pfucb->u.pfcb, &pfucbOE ) );

        //  search for pgnoFDP in owned extent tree
        //
        const CSPExtentKeyBM    spFindOwnedExtOfFDP( SPEXTKEY::fSPExtentTypeOE, pgnoFDP );
        dib.pos = posDown;
        dib.pbm = spFindOwnedExtOfFDP.Pbm( pfucbOE ) ;
        dib.dirflag = fDIRNull;
        Call( ErrBTDown( pfucbOE, &dib, latchReadTouch ) );
        if ( err == wrnNDFoundGreater )
        {
            Call( ErrBTGet( pfucbOE ) );
        }

        const CSPExtentInfo     spOwnedExtOfFDP( pfucbOE );

        //  FDP page should be first page of primary extent
        //
        Assert( spOwnedExtOfFDP.FValidExtent() );
        Assert( pgnoFDP == spOwnedExtOfFDP.PgnoFirst() );
    }

HandleError:
    if ( pfucbOE != pfucbNil )
        BTClose( pfucbOE );
    if ( pfucb != pfucbNil )
        BTClose( pfucb );
    return err;
}


//  checks if an extent described by pgnoFirst, cpgSize was allocated
//
LOCAL ERR ErrSPIWasAlloc(
    FUCB    *pfucb,
    PGNO    pgnoFirst,
    CPG     cpgSize )
{
    ERR     err;
    FUCB    *pfucbOE = pfucbNil;
    FUCB    *pfucbAE = pfucbNil;
    DIB     dib;
    CSPExtentKeyBM      spFindExt;
    CSPExtentInfo       spExtInfo;
    const PGNO          pgnoLast = pgnoFirst + cpgSize - 1;

    if ( FSPIIsSmall( pfucb->u.pfcb ) )
    {
        SPACE_HEADER    *psph;
        UINT            rgbitT;
        INT             iT;

        AssertSPIPfucbOnRoot( pfucb );

        //  get external header
        //
        NDGetExternalHeader( pfucb, pfucb->pcsrRoot, noderfSpaceHeader );
        Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
        psph = reinterpret_cast <SPACE_HEADER *> ( pfucb->kdfCurr.data.Pv() );
        //  make mask for extent to check
        //
        for ( rgbitT = 1, iT = 1;
            iT < cpgSize && iT < cpgSmallSpaceAvailMost;
            iT++ )
            rgbitT = (rgbitT<<1) + 1;
        Assert( pgnoFirst - PgnoFDP( pfucb ) < cpgSmallSpaceAvailMost );
        if ( pgnoFirst != PgnoFDP( pfucb ) )
        {
            rgbitT <<= (pgnoFirst - PgnoFDP( pfucb ) - 1);
            Assert( ( psph->RgbitAvail() & rgbitT ) == 0 );
        }

        goto HandleError;
    }

    //  open cursor on owned extent
    //
    Call( ErrSPIOpenOwnExt( pfucb->ppib, pfucb->u.pfcb, &pfucbOE ) );

    //  check that the given extent is owned by the given FDP but not
    //  available in the FDP available extent.
    //
    spFindExt.SPExtentMakeKeyBM( SPEXTKEY::fSPExtentTypeOE, pgnoLast );
    dib.pos = posDown;
    dib.pbm = spFindExt.Pbm( pfucbOE );
    dib.dirflag = fDIRNull;
    Call( ErrBTDown( pfucbOE, &dib, latchReadNoTouch ) );
    if ( err == wrnNDFoundLess )
    {
        Assert( fFalse );
        Assert( Pcsr( pfucbOE )->Cpage().Clines() - 1 ==
                    Pcsr( pfucbOE )->ILine() );
        Assert( pgnoNull != Pcsr( pfucbOE )->Cpage().PgnoNext() );

        Call( ErrBTNext( pfucbOE, fDIRNull ) );
        err = ErrERRCheck( wrnNDFoundGreater );

        #ifdef DEBUG
        const CSPExtentInfo spExt( pfucbOE );
        Assert( spExt.PgnoLast() > pgnoLast );
        #endif
    }

    if ( err == wrnNDFoundGreater )
    {
        Call( ErrBTGet( pfucbOE ) );
    }

    spExtInfo.Set( pfucbOE );
    Assert( pgnoFirst >= spExtInfo.PgnoFirst() );
    BTUp( pfucbOE );

    //  check that the extent is not in available extent.  Since the BT search
    //  is keyed with the last page of the extent to be freed, it is sufficient
    //  to check that the last page of the extent to be freed is in the found
    //  extent to determine the full extent has not been allocated.
    //  If available extent is empty then the extent cannot be in available extent
    //  and has been allocated.
    //

    EnforceSz( fFalse, "SpaceCheckNeedsFixingForPools" );

    spFindExt.SPExtentMakeKeyBM( SPEXTKEY::fSPExtentTypeAE, pgnoLast );
    dib.pos = posDown;
    dib.pbm = spFindExt.Pbm( pfucbOE );
    dib.dirflag = fDIRNull;
    Call( ErrSPIOpenAvailExt( pfucb->ppib, pfucb->u.pfcb, &pfucbAE ) );
    if ( ( err = ErrBTDown( pfucbAE, &dib, latchReadNoTouch ) ) < 0 )
    {
        if ( err == JET_errNoCurrentRecord )
        {
            err = JET_errSuccess;
            goto HandleError;
        }
        goto HandleError;
    }

    //  extent should not be found in available extent tree
    //
    Assert( err != JET_errSuccess );

    if ( err == wrnNDFoundGreater )
    {
        Call( ErrBTNext( pfucbAE, fDIRNull ) );
        const CSPExtentInfo     spavailextNext( pfucbAE );
        Assert( spavailextNext.CpgExtent() != 0 );
        //  last page of extent should be < first page in available extent node
        //
        Assert( pgnoLast < spavailextNext.PgnoFirst() );
    }
    else
    {
        Assert( err == wrnNDFoundLess );
        Call( ErrBTPrev( pfucbAE, fDIRNull ) );
        const CSPExtentInfo     spavailextPrev( pfucbAE );

        //  first page of extent > last page in available extent node
        //
        Assert( pgnoFirst > spavailextPrev.PgnoLast() );
    }

HandleError:
    if ( pfucbOE != pfucbNil )
        BTClose( pfucbOE );
    if ( pfucbAE != pfucbNil )
        BTClose( pfucbAE );

    return JET_errSuccess;
}

#endif  //  SPACECHECK

const ULONG pctDoublingGrowth  = 200;

CPG CpgSPIGetNextAlloc(
    _In_ const FCB_SPACE_HINTS * const  pfcbsh,
    _In_ const CPG                      cpgPrevious
    )
{

    //  Has user has selected a constant new extent size.
    //
    if ( pfcbsh->m_cpgMaxExtent != 0 &&
            pfcbsh->m_cpgMinExtent == pfcbsh->m_cpgMaxExtent )
    {
        return pfcbsh->m_cpgMaxExtent;  // or should it be cpgMinExtent?
    }

    CPG cpgNextExtent = cpgPrevious;

    if ( pfcbsh->m_pctGrowth )
    {
        //  Apply growth factor. Make sure that the extent always grows. This is to deal
        //  With rounding errors where m_pctGrowth is less than 200%. For example:
        //      ( 1 * 150 ) / 100 = 1
        //  Even in that case we still want to grow.
        //
        cpgNextExtent = max( cpgPrevious + 1, ( cpgPrevious * pfcbsh->m_pctGrowth ) / 100 );
        Assert( cpgNextExtent != cpgPrevious );
    }

    //  Apply min/max limits (if non-zero)
    //
    cpgNextExtent = (CPG)UlBound( (ULONG)cpgNextExtent,
                                (ULONG)( pfcbsh->m_cpgMinExtent ? pfcbsh->m_cpgMinExtent : cpgNextExtent ),
                                (ULONG)( pfcbsh->m_cpgMaxExtent ? pfcbsh->m_cpgMaxExtent : cpgNextExtent ) );

    Assert( cpgNextExtent >= 0 );
    return cpgNextExtent;
}

#ifdef SPACECHECK
void SPCheckPrintMaxNumAllocs( const LONG cbPageSize )
{
    JET_SPACEHINTS jsph = {
        sizeof(JET_SPACEHINTS),
        80,
        0 /* set lower */,
        0x0, 0x0, 0x0, 0x0, 0x0,
        80,
        0 /* set lower */,
        0,
//      16 * 1024 * 1024    // 16 MB
        1 * 1024 * 1024 * 1024      // 1 GB
    };
    FCB_SPACE_HINTS fcbshCheck = { 0 };

    ULONG iLastAllocs = 0;

    ULONG pct = 200;
    for( ULONG cpg = 1; cpg < (ULONG) ( 16 * 1024 * 1024 / cbPageSize ); cpg++ )
    {

        jsph.cbInitial = cpg * cbPageSize;
        for( ; pct >= 100; pct-- )
        {
            ULONG cpgNext = ( ( jsph.cbInitial / cbPageSize ) * ( pct ) ) / 100;
            if ( cpgNext == ( jsph.cbInitial / cbPageSize ) )
            {
                pct++;
                break;
            }

        }
        jsph.ulGrowth = pct;

        fcbshCheck.SetSpaceHints( &jsph, cbPageSize );

        CPG cpgLast = fcbshCheck.m_cpgInitial;
        ULONG iAlloc = 1;
        //wprintf(L"Start (cpgMax->%d):[%d]=%d,", fcbshCheck.m_cpgMaxExtent, iAlloc, cpgLast );
        for( ; iAlloc < (1024 * 1024 * 1024 + 1) /* i _think_ max is SqrRoot( cpgMaxExtent ) */; )
        {
            iAlloc++;
            cpgLast = CpgSPIGetNextAlloc( &fcbshCheck, cpgLast );
            //wprintf(L"[%d]=%d,", iAlloc, cpgLast );
            if ( cpgLast >= fcbshCheck.m_cpgMaxExtent )
            {
                break;
            }
        }
        // hmmm.
        //      Assert( cpgLast == fcbshCheck.m_cpgMaxExtent );
        wprintf(L"\tCpgInit: %09d Min:%03d%% -> %d iAllocs\n", (ULONG)fcbshCheck.m_cpgInitial, (ULONG)fcbshCheck.m_pctGrowth, iAlloc );

        //  when cpgInitial = 100, pctGrowth will be 101%, and will be the worst case.
        if( fcbshCheck.m_cpgInitial > 104 )
        {
            wprintf( L" reached the turning point, bailing.\n" );
            break;
        }
        iLastAllocs = iAlloc;
    }

}

#endif


////////////////
//  Periodic Trim

#if defined( ESENT ) || !defined( DEBUG )
static const TICK               g_dtickTrimDBPeriod             = 1000 * 60 * 60 * 24; // 1 day
#else
static const TICK               g_dtickTrimDBPeriod             = 1 * 1000; // 1 second
#endif

static TICK             g_tickLastPeriodicTrimDB        = 0;    //  last time a Trim task was executed
static BOOL             g_fSPTrimDBTaskScheduled        = fFalse;

//  Init / Term

LOCAL ERR ErrSPITrimDBITaskPerIFMP( const IFMP ifmp )
{
    ERR err = JET_errSuccess;
    PIB* ppib                   = ppibNil;
    WCHAR wsz[32];
    const WCHAR* rgcwsz[]       = { wsz };
    CPG cpgTrimmed              = 0;
#ifdef DEBUG
    // On debug builds we start the task every second. This ends up spamming the event log.
    static BOOL s_fLoggedStartEvent = fFalse;
    static BOOL s_fLoggedStopNoActionEvent = fFalse;
#endif

    OSTraceFMP(
        ifmp,
        JET_tracetagSpaceInternal,
        OSFormat(
            "%hs: Running trim task for ifmp %d.",
            __FUNCTION__,
            ifmp ) );

    AssertSz( g_fPeriodicTrimEnabled, "Trim Task should never be run if the test hook wasn't enabled." );
    AssertSz( !g_fRepair, "Trim Task should never be run during Repair." );
    AssertSz( !g_rgfmp[ ifmp ].FReadOnlyAttach(), "Trim Task should not be run on a read-only database." );
    AssertSz( !FFMPIsTempDB( ifmp ), "Trim task should not be run on temp databases." );

    if ( PinstFromIfmp( ifmp )->FRecovering() )
    {
        OSTraceFMP(
            ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Periodic trim will not run an in-recovery database.",
                __FUNCTION__ ) );

        err = JET_errSuccess;
        goto HandleError;
    }

    const ULONG dbstate = g_rgfmp[ ifmp ].Pdbfilehdr()->Dbstate();
    if ( ( dbstate == JET_dbstateIncrementalReseedInProgress ) ||
        ( dbstate == JET_dbstateDirtyAndPatchedShutdown ) || 
        ( dbstate == JET_dbstateRevertInProgress ) )
    {
        AssertSz( fFalse, "Periodic trim should not take place during incremental reseed or revert." );
        OSTraceFMP(
            ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Periodic trim will not run for a database in inc reseed or revert or dirty and patched shutdown status.",
                __FUNCTION__ ) );

        err = JET_errSuccess;
        goto HandleError;
    }

#ifdef DEBUG
    if ( !s_fLoggedStartEvent )
#endif
    {
        UtilReportEvent(
            eventInformation,
            GENERAL_CATEGORY,
            DB_TRIM_TASK_STARTED,
            0,
            NULL,
            0,
            NULL,
            PinstFromIfmp( ifmp ) );

        OnDebug( s_fLoggedStartEvent = fTrue );
    }

    if ( PinstFromIfmp( ifmp )->m_pbackup->FBKBackupInProgress() )
    {
        OSTraceFMP(
            ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Periodic trim will not run when there is backup in progress.",
                __FUNCTION__ ) );

        Error( ErrERRCheck( JET_errBackupInProgress ) );
    }

    Call( ErrPIBBeginSession( PinstFromIfmp( ifmp ), &ppib, procidNil, fFalse ) );

    if ( ( ( GrbitParam( g_rgfmp[ ifmp ].Pinst(), JET_paramEnableShrinkDatabase ) & JET_bitShrinkDatabaseRealtime ) != 0 ) &&
         g_rgfmp[ ifmp ].FTrimSupported() )
    {
        Call( ErrSPTrimRootAvail( ppib, ifmp, CPRINTFNULL::PcprintfInstance(), &cpgTrimmed ) );
    }

HandleError:

    OSTraceFMP(
        ifmp,
        JET_tracetagSpaceInternal,
        OSFormat(
            "%hs: Trim task for ifmp %d finished with result %d (0x%x).",
            __FUNCTION__,
            ifmp,
            err,
            err ) );

    if ( ppibNil != ppib )
    {
        PIBEndSession( ppib );
    }

    if ( err < JET_errSuccess )
    {
        OSStrCbFormatW( wsz, sizeof(wsz), L"%d", err );

        UtilReportEvent(
            eventError,
            GENERAL_CATEGORY,
            DB_TRIM_TASK_FAILED,
            _countof(rgcwsz),
            rgcwsz,
            0,
            NULL,
            PinstFromIfmp( ifmp ) );
    }
    else
    {
        if ( cpgTrimmed > 0 )
        {
            OSStrCbFormatW( wsz, sizeof(wsz), L"%d", cpgTrimmed );

            UtilReportEvent(
                eventInformation,
                GENERAL_CATEGORY,
                DB_TRIM_TASK_SUCCEEDED,
                _countof(rgcwsz),
                rgcwsz,
                0,
                NULL,
                PinstFromIfmp( ifmp ) );
        }
        else
        {
#ifdef DEBUG
            if ( !s_fLoggedStopNoActionEvent )
#endif
            {
                UtilReportEvent(
                    eventInformation,
                    GENERAL_CATEGORY,
                    DB_TRIM_TASK_NO_TRIM,
                    0,
                    NULL,
                    0,
                    NULL,
                    PinstFromIfmp( ifmp ) );

                OnDebug( s_fLoggedStopNoActionEvent = fTrue );
            }
        }
    }

    return err;
}

LOCAL VOID SPITrimDBITask( VOID*, VOID* )
{
    BOOL fReschedule = fFalse;

    Assert( g_fSPTrimDBTaskScheduled );

    OSTrace( JET_tracetagSpaceInternal, __FUNCTION__ );

    g_tickLastPeriodicTrimDB = TickOSTimeCurrent();

    // Although we block running on repair, it is possible to get into repair "mode" by
    // running JetDBUtilities. So we should bail trimming if we encounter such case. We
    // shouldn't turn off trim altogether as at some point the repair should end and
    // "regular" usage should continue.
    if ( g_fRepair )
    {
        OSTrace(
            JET_tracetagSpaceInternal,
            OSFormat( "We're in repair - JetDBUtilities is being executed. Aborting trim." ) );

        goto FinishTask;
    }

    // taking this protects against FMPs coming and going; and it
    // protects against FMPs changing relevant state (FInUse, FLogOn, FAttached)
    FMP::EnterFMPPoolAsWriter();

    for ( IFMP ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
    {
        if ( FFMPIsTempDB( ifmp ) || !g_rgfmp[ ifmp ].FInUse() )
        {
            continue;
        }

        g_rgfmp[ ifmp ].GetPeriodicTrimmingDBLatch();

        FMP::LeaveFMPPoolAsWriter();

        if ( !g_rgfmp[ ifmp ].FScheduledPeriodicTrim() || g_rgfmp[ ifmp ].FDontStartTrimTask() )
        {
            OSTraceFMP(
                ifmp,
                JET_tracetagSpaceInternal,
                OSFormat(
                    "%hs: Periodic trimming is off for this IFMP. Skipping.",
                    __FUNCTION__ ) );

            g_rgfmp[ ifmp ].ReleasePeriodicTrimmingDBLatch();

            FMP::EnterFMPPoolAsWriter();

            continue;
        }

        fReschedule = fTrue;

        (void)ErrSPITrimDBITaskPerIFMP( ifmp );
        g_rgfmp[ ifmp ].ReleasePeriodicTrimmingDBLatch();

        FMP::EnterFMPPoolAsWriter();
    }

    FMP::LeaveFMPPoolAsWriter();

FinishTask:
    // Reschedule it if we need to.
    if ( fReschedule )
    {
        OSTimerTaskScheduleTask( g_posttSPITrimDBITask, NULL, g_dtickTrimDBPeriod, g_dtickTrimDBPeriod );
    }
    else
    {
        g_semSPTrimDBScheduleCancel.Acquire();
        g_fSPTrimDBTaskScheduled = fFalse;
        g_semSPTrimDBScheduleCancel.Release();
    }

    Assert( !FMP::FWriterFMPPool() );
}

VOID SPTrimDBTaskStop( INST * pinst, const WCHAR * cwszDatabaseFullName )
{
    Assert( g_fPeriodicTrimEnabled );

    //  terminate any periodic trimming ...
    FMP::EnterFMPPoolAsWriter();
    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        const IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax ||
            !g_rgfmp[ ifmp ].FInUse() )
        {
            continue;
        }

        if ( NULL != cwszDatabaseFullName &&
            ( g_rgfmp[ ifmp ].Pinst() != pinst || UtilCmpFileName( cwszDatabaseFullName, g_rgfmp[ ifmp ].WszDatabaseName() ) != 0 ) )
        {
            continue;
        }

        FMP::LeaveFMPPoolAsWriter();

        //  reset the periodic trimming under the trimming DB latch to
        //  ensure that any outstanding trims will be done for.
        g_rgfmp[ ifmp ].GetPeriodicTrimmingDBLatch();
        g_rgfmp[ ifmp ].SetFDontStartTrimTask();
        g_rgfmp[ ifmp ].ResetScheduledPeriodicTrim();
        g_rgfmp[ ifmp ].ReleasePeriodicTrimmingDBLatch();

        FMP::EnterFMPPoolAsWriter();
    }

    FMP::LeaveFMPPoolAsWriter();
}

ERR ErrSPTrimDBTaskInit( const IFMP ifmp )
{
    ERR err = JET_errSuccess;

    Assert( g_fPeriodicTrimEnabled );

    if ( g_rgfmp[ ifmp ].FReadOnlyAttach() )
    {
        AssertSz( fFalse, "Periodic trim should not be set up for read-only databases." );
        Error( ErrERRCheck( JET_errDatabaseFileReadOnly ) );
    }

    if ( g_fRepair || FFMPIsTempDB( ifmp ) )
    {
        AssertSz( fFalse, "Periodic trim should not be set up for a database in repair or for the temp db." );
        Error( ErrERRCheck( JET_errInvalidDatabaseId ) );
    }

    if ( ( GrbitParam( g_rgfmp[ ifmp ].Pinst(), JET_paramEnableShrinkDatabase ) & JET_bitShrinkDatabasePeriodically ) == 0 )
    {
        OSTraceFMP(
            ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Periodic trim is not enabled. Will not schedule the trim task.",
                __FUNCTION__ ) );

        err = JET_errSuccess;
        goto HandleError;
    }

    if ( BoolParam( JET_paramEnableViewCache ) )
    {
        OSTraceFMP(
            ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Periodic trimming cannot be enabled for view cache. Will not schedule the trimming task.",
                __FUNCTION__ ) );

        err = JET_errSuccess;
        goto HandleError;
    }

#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY

    g_rgfmp[ ifmp ].GetPeriodicTrimmingDBLatch();

    if ( g_rgfmp[ ifmp ].FDontStartTrimTask() )
    {
        OSTraceFMP(
            ifmp,
            JET_tracetagSpaceInternal,
            OSFormat(
                "%hs: Periodic trimming is set so it should not start. Will not schedule the trim task.",
                __FUNCTION__ ) );

        err = JET_errSuccess;
        g_rgfmp[ ifmp ].ReleasePeriodicTrimmingDBLatch();

        goto HandleError;
    }

    g_rgfmp[ ifmp ].SetScheduledPeriodicTrim();

    g_rgfmp[ ifmp ].ReleasePeriodicTrimmingDBLatch();

    g_semSPTrimDBScheduleCancel.Acquire();
    Assert( g_semSPTrimDBScheduleCancel.CAvail() == 0 );
    if ( !g_fSPTrimDBTaskScheduled )
    {
        g_fSPTrimDBTaskScheduled = fTrue;
        OSTimerTaskScheduleTask( g_posttSPITrimDBITask, NULL, g_dtickTrimDBPeriod, g_dtickTrimDBPeriod );
    }

    Assert( g_semSPTrimDBScheduleCancel.CAvail() == 0 );
    g_semSPTrimDBScheduleCancel.Release();
#endif  //  MINIMAL_FUNCTIONALITY

HandleError:

    return err;
}

