// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_bt.hxx"

#include "PageSizeClean.hxx"

// Defining this helps to test the different recovery paths for page move.
// This is done by flagging some pages as filty so they are flushed agressively.
// A crash can then test all the different possibilities of flushed/unflushed pages.

///#define TEST_PAGEMOVE_RECOVERY

//  general directives
//  always correct bmCurr and csr.DBTime on loss of physical currency
//  unless not needed because of loc in which case, reset bmCurr

//  *****************************************************
//  internal function prototypes
//
ERR ErrBTIIRefresh( FUCB *pfucb, LATCH latch );
ERR ErrBTDelete( FUCB *pfucb, const BOOKMARK& bm );
INT CbBTIFreeDensity( const FUCB *pfucb );
VOID BTIComputePrefix( FUCB *pfucb, CSR *pcsr, const KEY& key, KEYDATAFLAGS *pkdf );
BOOL FBTIAppend( const FUCB *pfucb, CSR *pcsr, ULONG cbReq, const BOOL fUpdateUncFree = fTrue );
BOOL FBTISplit( const FUCB *pfucb, CSR *pcsr, const ULONG cbReq, const BOOL fUpdateUncFree = fTrue );
ERR ErrBTISplit(
                    FUCB * const pfucb,
                    KEYDATAFLAGS * const pkdf,
                    const DIRFLAG   dirflagT,
                    const RCEID rceid1,
                    const RCEID rceid2,
                    RCE * const prceReplace,
                    const INT cbDataOld,
                    const VERPROXY * const pverproxy );
VOID BTISplitSetCbAdjust(
                    SPLIT               *psplit,
                    FUCB                *pfucb,
                    const KEYDATAFLAGS& kdf,
                    const RCE           *prce1,
                    const RCE           *prce2 );
VOID BTISplitSetCursor( FUCB *pfucb, SPLITPATH *psplitPathLeaf );
VOID BTIPerformSplit( FUCB *pfucb, SPLITPATH *psplitPathLeaf, KEYDATAFLAGS *pkdf, DIRFLAG dirflag );
LOCAL VOID BTIComputePrefixAndInsert( FUCB *pfucb, CSR *pcsr, const KEYDATAFLAGS& kdf );
VOID BTICheckSplitLineinfo( FUCB *pfucb, SPLIT *psplit, const KEYDATAFLAGS& kdf );
VOID BTICheckSplits( FUCB *pfucb, SPLITPATH *psplitPathLeaf, KEYDATAFLAGS *pkdf, DIRFLAG dirflag );
LOCAL VOID BTICheckSplitFlags( const SPLIT *psplit );
VOID BTICheckOneSplit( FUCB *pfucb, SPLITPATH *psplitPathLeaf, KEYDATAFLAGS *pkdf, DIRFLAG dirflag );
LOCAL VOID BTIInsertPgnoNewAndSetPgnoSplit( FUCB *pfucb, SPLITPATH *psplitPath );
BOOL FBTIUpdatablePage( const CSR& csr );
LOCAL VOID BTISplitFixSiblings( SPLIT *psplit );
LOCAL VOID BTIInsertPgnoNew ( FUCB *pfucb, SPLITPATH *psplitPath );
VOID BTISplitMoveNodes( FUCB *pfucb, SPLIT *psplit, KEYDATAFLAGS *pkdf, DIRFLAG dirflag );
VOID BTISplitBulkCopy( FUCB *pfucb, SPLIT *psplit, INT ilineStart, INT clines );
VOID BTISplitBulkDelete( CSR * pcsr, INT clines );
const LINEINFO *PlineinfoFromIline( SPLIT *psplit, INT iline );
VOID BTISplitSetPrefixInSrcPage( FUCB *pfucb, SPLIT *psplit );
LOCAL ERR ErrBTIGetNewPages( FUCB *pfucb, SPLITPATH *psplitPathLeaf, DIRFLAG dirflag );
VOID BTISplitRevertDbtime( SPLITPATH *psplitPathLeaf );
VOID BTIMergeRevertDbtime( MERGEPATH *pmergePathTip );
LOCAL VOID BTISplitReleaseUnneededPages( INST *pinst, SPLITPATH **psplitPathLeaf );
LOCAL ERR ErrBTISplitUpgradeLatches( const IFMP ifmp, SPLITPATH * const psplitPathLeaf );
LOCAL VOID BTISplitSetLgpos( SPLITPATH *psplitPathLeaf, const LGPOS& lgpos );
VOID BTISplitShrinkPages( SPLITPATH *psplitPathLeaf );
VOID BTIReleaseSplitPaths( INST *pinst, SPLITPATH *psplitPath );
VOID BTIReleaseMergePaths( MERGEPATH *pmergePathTip );
LOCAL VOID BTISplitCheckPath( SPLITPATH *psplitPathLeaf );

ERR ErrBTINewMergePath( MERGEPATH **ppmergePath );
LOCAL ERR ErrBTICreateMergePath(
    FUCB *pfucb,
    const BOOKMARK& bmSearch,
    const PGNO pgnoSearch,
    const BOOL fLeafPage,
    MERGEPATH **ppmergePath,
    _Inout_opt_ PrereadInfo * const pPrereadInfo = NULL );

ERR ErrBTINewSplitPath( SPLITPATH **ppsplitPath );
LOCAL ERR ErrBTICreateSplitPath( FUCB               *pfucb,
                           const BOOKMARK&  bm,
                           SPLITPATH        **ppsplitPath );

ERR ErrBTICreateSplitPathAndRetryOper( FUCB             * const pfucb,
                                       const KEYDATAFLAGS * const pkdf,
                                       SPLITPATH        **ppsplitPath,
                                       DIRFLAG  * const pdirflag,
                                       const RCEID rceid1,
                                       const RCEID rceid2,
                                       RCE * const prceReplace,
                                       const VERPROXY * const pverproxy );
LOCAL ERR   ErrBTISelectSplit( FUCB *pfucb, SPLITPATH *psplitPath, KEYDATAFLAGS *pkdf, DIRFLAG dirflag );
ERR ErrBTISplitAllocAndCopyPrefix( const KEY &key, DATA *pdata );
ERR ErrBTISplitAllocAndCopyPrefixes( FUCB *pfucb, SPLIT *psplit );
ERR ErrBTISeekSeparatorKey( SPLIT *psplit, FUCB *pfucb );
ERR ErrBTISplitComputeSeparatorKey( SPLIT *psplit, FUCB *pfucb );
LOCAL VOID BTISelectPrefix( const LINEINFO  *rglineinfo,
                            INT             clines,
                            PREFIXINFO      *pprefixinfo );
LOCAL VOID BTISelectPrefixes( SPLIT *psplit, INT ilineSplit );
VOID BTISplitSetPrefixes( SPLIT *psplit );
LOCAL VOID BTISetPrefix( LINEINFO *rglineinfo, INT clines, const PREFIXINFO& prefixinfo );
VOID BTISplitCalcUncFree( SPLIT *psplit );
VOID BTISelectAppend( SPLIT *psplit, FUCB *pfucb );
VOID BTISelectVerticalSplit( SPLIT *psplit, FUCB *pfucb );
VOID BTISelectRightSplit( SPLIT *psplit, FUCB *pfucb );
BOOL FBTISplitCausesNoOverflow( SPLIT *psplit, INT cLineSplit, BOOL * pfSplitPageOverflow = NULL, BOOL * pfNewPageOverflow = NULL );
VOID BTIRecalcWeightsLE( SPLIT *psplit );
VOID BTISelectSplitWithOperNone( SPLIT *psplit, FUCB *pfucb );
ERR ErrBTINewSplit( FUCB *pfucb, SPLITPATH *psplitPath, KEYDATAFLAGS *pkdf, DIRFLAG dirflag );
ERR ErrBTINewMerge( MERGEPATH *pmergePathLeaf );
VOID BTIReleaseSplit( INST *pinst, SPLIT *psplit );
VOID BTIReleaseMergeLineinfo( MERGE *pmerge );
LOCAL ULONG CbBTIMaxSizeOfNode( const FUCB * const pfucb, const CSR * const pcsr );
LOCAL INT IlineBTIFrac( const CSR * const pcsr, double *pFrac );

LOCAL ERR ErrBTISelectMerge(
    FUCB            * const pfucb,
    MERGEPATH       * const pmergePathLeaf,
    const BOOKMARK&         bm,
    BOOKMARK        * const pbmNext,
    RECCHECK        * const preccheck,
    const BOOL              fRightMerges );
LOCAL ERR ErrBTIMergeCollectPageInfo(
    FUCB * const pfucb,
    MERGEPATH * const pmergePathLeaf,
    RECCHECK * const preccheck,
    const BOOL fRightMerges );
ERR ErrBTIMergeLatchSiblingPages( FUCB *pfucb, MERGEPATH * const pmergePathLeaf );
LOCAL BOOL FBTICheckMergeableOneNode(
    const FUCB * const pfucb,
    CSR * const pcsrDest,
    MERGE * const pmerge,
    const INT iline,
    const ULONG cbDensityFree,
    const BOOL fRightMerges );
LOCAL VOID BTICheckMergeable( FUCB * const pfucb, MERGEPATH * const pmergePath, const BOOL fRightMerges );
LOCAL BOOL FBTIOverflowOnReplacingKey( FUCB                     *pfucb,
                                 MERGEPATH              *pmergePath,
                                 const KEYDATAFLAGS&    kdfSep );
LOCAL BOOL FBTIOverflowOnReplacingKey(  FUCB                    *pfucb,
                                        CSR * const             pcsr,
                                        const INT               iLine,
                                        const KEYDATAFLAGS&     kdfSep );
ERR ErrBTIMergeCopySeparatorKey( MERGEPATH  *pmergePath,
                                 MERGE      *pmergeLeaf,
                                 FUCB       *pfucb );

LOCAL ERR ErrBTISelectMergeInternalPages( FUCB *pfucb, MERGEPATH * const pmergePathLeaf );
ERR  ErrBTIMergeOrEmptyPage( FUCB *pfucb, MERGEPATH *pmergePathLeaf );
VOID BTIPerformMerge( FUCB *pfucb, MERGEPATH *pmergePathLeaf );
VOID BTIPerformOneMerge( FUCB *pfucb,
                         MERGEPATH *pmergePath,
                         MERGE *pmergeLeaf );
VOID BTIChangeKeyOfPagePointer( FUCB *pfucb, CSR *pcsr, const KEY& key );
LOCAL ERR ErrBTIMergeUpgradeLatches( const IFMP ifmp, MERGEPATH * const pmergePathTip );
VOID BTIMergeReleaseUnneededPages( MERGEPATH *pmergePathTip );
VOID BTIMergeSetLgpos( MERGEPATH *pmergePathTip, const LGPOS& lgpos );
VOID BTIMergeShrinkPages( MERGEPATH *pmergePathLeaf );
VOID BTIMergeReleaseLatches( MERGEPATH *pmergePathTip );
VOID BTIReleaseEmptyPages( FUCB *pfucb, MERGEPATH *pmergePathTip );
LOCAL VOID BTIMergeDeleteFlagDeletedNodes( FUCB * const pfucb, MERGEPATH * const pmergePath );
VOID BTIMergeFixSiblings( INST *pinst, MERGEPATH *pmergePath );
LOCAL VOID BTIMergeMoveNodes( FUCB * const pfucb, MERGEPATH * const pmergePath );

VOID BTICheckMerge( FUCB *pfucb, MERGEPATH *pmergePathLeaf );
VOID BTICheckMergeLeaf( FUCB *pfucb, MERGEPATH *pmergePath );
VOID BTICheckMergeInternal( FUCB        *pfucb,
                            MERGEPATH   *pmergePath,
                            MERGE       *pmergeLeaf );

//  single page cleanup routines
//
LOCAL ERR ErrBTISinglePageCleanup( FUCB *pfucb, const BOOKMARK& bm );
LOCAL ERR ErrBTISPCCollectLeafPageInfo(
    FUCB        *pfucb,
    CSR         *pcsr,
    LINEINFO    **plineinfo,
    RECCHECK    * const preccheck,
    BOOL        *pfEmptyPage,
    BOOL        *pfExistsFlagDeletedNodeWithActiveVersion,
    INT         *ppctFull );
LOCAL ERR ErrBTISPCDeleteNodes( FUCB *pfucb, CSR *pcsr );
ERR ErrBTISPCSeek( FUCB *pfucb, const BOOKMARK& bm );
BOOL FBTISPCCheckMergeable( FUCB *pfucb, CSR *pcsrDest, LINEINFO *rglineinfo );

//  OLD2 routines
bool FBTICpgFragmentedRange(
    __in_ecount(cpg) const PGNO * const rgpgno,
    const CPG cpg,
    _In_ const CPG cpgFragmentedRangeMin,
    _Out_ INT * const pipgnoStart,
    _Out_ INT * const pipgnoEnd );
bool FBTICpgFragmentedRangeTrigger(
    __in_ecount(cpg) const PGNO * const rgpgno,
    const CPG cpg,
    _In_ const CPG cpgFragmentedRangeTrigger,
    _In_ const CPG cpgFragmentedRangeMin );
LOCAL BOOL FBTIEligibleForOLD2( FUCB * const pfucb );
LOCAL ERR ErrBTIRegisterForOLD2( FUCB * const pfucb );

//  debug routines
//
VOID AssertBTIVerifyPgnoSplit( FUCB *pfucb, SPLITPATH *psplitPath );
VOID AssertBTIBookmarkSaved( const FUCB *pfucb );

//  move to node
//
INT CbNDCommonPrefix( FUCB *pfucb, CSR *pcsr, const KEY& key );


//  number of non-visible nodes that are skipped or pages
//  containing such nodes that are visited during lateral node
//  navigation before we report a warning, and the frequency
//  (in hours) with which to report the warning
//
const ULONG g_cBTINonVisibleNodesSkippedWarningThreshold            = 10000;
const ULONG g_cBTIPagesVisitedWithNonVisibleNodesWarningThreshold   = 100;
const __int64 g_cBTIPagesCacheMissedForNonVisibleNodes              = 64;   //  this would be _at least_ 8 IOs (at current 256 KB max read, but likely much more) of useless work
const ULONG g_ulBTINonVisibleNodesSkippedWarningFrequency           = 1;    //  unit is in hours (so the event will be reported no more than once per hour per btree)
const double g_pctBTICacheMissLatencyToQualifyForIoLoad             = 0.80; //  this is 80%

//  latency limits for CPU load and IO load based event thresholds
//  respectively
//
const double g_cmsecBTINavigateProcessingLatencyThreshold           = 100.0;    //  at only 100 ms, we trim > 99% of pure CPU-based skipped nodes events.
//  Now 10 seconds may seem like an exceedingly long hang or 
//  IO time to insist before we log an event ... and it +is+, 
//  however from datacenter analysis of last 1000 BT skipped
//  nodes event ... at least at the moment, ~12.3% are > 2 secs,
//  and 16.7% > 1 second.  Even at 10 seconds, 3.9% of incidents
//  qualify for eventing about.
const __int64 g_cmsecBTINavigateCacheMissLatencyThreshold           = 10000; // 10 seconds - length of time cache misses must cumulative exceed to get an event.


//  Constants that control defragmentation behaviour

//  This constant controls ranges that will be defragmented when OLD2 runs
//  any sequence of non-contiguous pages this long will be defragmented
const CPG cpgFragmentedRangeDefrag = 16;

//  This constant controls when OLD2 will be started. There must be a range
//  of at least this many non-contiguous pages before OLD2 starts. We want the
//  trigger to be less sensitive than the cleanup so we make the fragmented
//  range shorter. That way this sequence:
//
//  101 | 102 | 103 | 104 | 106 | 107 | 108 | 109 
//
//  will be defragged by OLD2, but will not trigger OLD2
// 
//  (at the extreme, a range of 1 will consider all data contiguous)
//
const CPG cpgFragmentedRangeDefragTrigger = cpgFragmentedRangeDefrag / 2;

//  HACK:  reference to BF internal

extern TABLECLASS tableclassNameSetMax;

#ifdef PERFMON_SUPPORT

PERFInstanceDelayedTotalWithClass<> cBTSeekShortCircuit;
LONG LBTSeekShortCircuitCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTSeekShortCircuit.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTUnnecessarySiblingLatch;
LONG LBTUnnecessarySiblingLatchCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTUnnecessarySiblingLatch.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTNext;
LONG LBTNextCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTNext.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTNextNonVisibleNodesSkipped;
LONG LBTNextNonVisibleNodesSkippedCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTNextNonVisibleNodesSkipped.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTNextNodesFiltered;
LONG LBTNextNodesFilteredCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTNextNodesFiltered.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTPrev;
LONG LBTPrevCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cBTPrev.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTPrevNonVisibleNodesSkipped;
LONG LBTPrevNonVisibleNodesSkippedCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cBTPrevNonVisibleNodesSkipped.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTPrevNodesFiltered;
LONG LBTPrevNodesFilteredCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cBTPrevNodesFiltered.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTSeek;
LONG LBTSeekCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTSeek.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTAppend;
LONG LBTAppendCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTAppend.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTInsert;
LONG LBTInsertCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTInsert.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTReplace;
LONG LBTReplaceCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTReplace.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTFlagDelete;
LONG LBTFlagDeleteCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTFlagDelete.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTDelete;
LONG LBTDeleteCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTDelete.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTAppendSplit;
LONG LBTAppendSplitCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTAppendSplit.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTRightSplit;
LONG LBTRightSplitCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTRightSplit.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTRightHotpointSplit;
LONG LBTRightHotpointSplitCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTRightHotpointSplit.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTVerticalSplit;
LONG LBTVerticalSplitCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTVerticalSplit.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LBTSplitCEFLPv( LONG iInstance, VOID *pvBuf )
{
    if ( NULL != pvBuf )
    {
        *(LONG*)pvBuf = cBTAppendSplit.Get( iInstance ) + cBTRightSplit.Get( iInstance ) + cBTVerticalSplit.Get( iInstance );
    }
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTEmptyPageMerge;
LONG LBTEmptyPageMergeCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTEmptyPageMerge.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTRightMerge;
LONG LBTRightMergeCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTRightMerge.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTPartialMerge;
LONG LBTPartialMergeCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTPartialMerge.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTFullLeftMerge;
LONG LBTFullLeftMergeCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTFullLeftMerge.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTPartialLeftMerge;
LONG LBTPartialLeftMergeCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTPartialLeftMerge.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTPageMove;
LONG LBTPageMoveCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTPageMove.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LBTMergeCEFLPv( LONG iInstance, VOID *pvBuf )
{
    if ( NULL != pvBuf )
    {
        *(LONG*)pvBuf =
            cBTEmptyPageMerge.Get( iInstance )
            + cBTRightMerge.Get( iInstance )
            + cBTPartialMerge.Get( iInstance )
            + cBTFullLeftMerge.Get( iInstance )
            + cBTPartialLeftMerge.Get( iInstance )
            ;
    }
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTFailedSPCWriteLatch;
LONG LBTFailedSPCWriteLatchCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTFailedSPCWriteLatch.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cBTOpportuneReads;
LONG LBTOpportuneReadsCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cBTOpportuneReads.PassTo( iInstance, pvBuf );
    return 0;
}

#endif // PERFMON_SUPPORT

//  Creates a standard TC scope for BT functions to use. Sets iors and TCE in the scope
//  This function relies on being inlined and NRVO to avoid creating unwanted TraceContextScope copies
INLINE PIBTraceContextScope TcBTICreateCtxScope( FUCB* pfucb, IOREASONSECONDARY iors )
{
    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->iorReason.SetIors( iors );

    tcScope->SetDwEngineObjid( pfucb->u.pfcb->ObjidFDP() );
    return tcScope;
}


//  ******************************************************
//  BTREE API ROUTINES
//


//  ******************************************************
//  Open/Close operations
//

//  opens a cursor on given fcb [BTree]
//  uses defer-closed cursor, if possible
//  given FCB must already have conditions that will not allow it
//    to be closed while this operation is in progress.
//    lower levels of the stack Assert on this.
ERR ErrBTOpen( PIB *ppib, FCB *pfcb, FUCB **ppfucb, BOOL fAllowReuse )
{
    ERR     err;
    FUCB    *pfucb;

    Assert( pfcb != pfcbNil );
    Assert( pfcb->FInitialized() );

    // In most cases, we should reuse a deferred-closed FUCB.  The one
    // time we don't want to is if we're opening a space cursor.
    if ( fAllowReuse )
    {
        ENTERCRITICALSECTION critCursors( &ppib->critCursors, ppib->FBatchIndexCreation() );

        // cannabalize deferred closed cursor
        for ( pfucb = ppib->pfucbOfSession;
            pfucb != pfucbNil;
            pfucb = pfucb->pfucbNextOfSession )
        {
            if ( FFUCBDeferClosed( pfucb ) && !FFUCBNotReuse( pfucb ) )
            {
                Assert( !FFUCBSpace( pfucb ) );     // Space cursors are never defer-closed.

                // Secondary index FCB may have been deallocated by
                // rollback of CreateIndex or cleanup of DeleteIndex
                Assert( pfucb->u.pfcb != pfcbNil || FFUCBSecondary( pfucb ) );
                if ( pfucb->u.pfcb == pfcb )
                {
                    const BOOL  fVersioned  = FFUCBVersioned( pfucb );

                    Assert( pfcbNil != pfucb->u.pfcb );
                    Assert( ppib->Level() > 0 );
                    Assert( pfucb->levelOpen <= ppib->Level() );

                    // We are reusing the cursor at this level. This means that
                    // if we rollback we must then close it if the rollback happens
                    // at this level or lower. 
                    Assert( 0 == pfucb->levelReuse );
                    pfucb->levelReuse = ppib->Level();

                    //  Reset all used flags. Keep Updatable (fWrite) flag
                    //
                    FUCBResetFlags( pfucb );
                    Assert( !FFUCBDeferClosed( pfucb ) );

                    pfucb->pfucbTable = NULL;

                    FUCBResetPreread( pfucb );
                    FUCBResetOpportuneRead( pfucb );

                    //  must persist Versioned flag
                    Assert( !FFUCBVersioned( pfucb ) );     //  got reset by FUCBResetFlags()
                    if ( fVersioned )
                        FUCBSetVersioned( pfucb );

                    Assert( !FFUCBUpdatable( pfucb ) );
                    if ( !g_rgfmp[ pfcb->Ifmp() ].FReadOnlyAttach() )
                    {
                        FUCBSetUpdatable( pfucb );
                    }

                    *ppfucb = pfucb;

                    return JET_errSuccess;
                }
            }
        }
    }
    else
    {
        pfucb = pfucbNil;       // Initialise for FUCBOpen() below.
    }

    Assert( pfucbNil == pfucb );
    Call( ErrFUCBOpen(
              ppib,
              pfcb->Ifmp(),
              &pfucb ) );
    Call( pfcb->ErrLink( pfucb ) );

    *ppfucb = pfucb;

    return JET_errSuccess;

HandleError:
    Assert( err < 0 );
    if ( pfucbNil != pfucb )
    {
        if ( pfcbNil != pfucb->u.pfcb )
        {
            Assert( pfcb == pfucb->u.pfcb );
            // We managed to link the FUCB to the FCB before we errored.
            pfucb->u.pfcb->Unlink( pfucb );
        }
        //  close the FUCB
        FUCBClose( pfucb );
    }
    return err;
}

//  opens a cursor on given FCB on behalf of another thread.
//  uses defer-closed cursor, if possible
ERR ErrBTOpenByProxy( PIB *ppib, FCB *pfcb, FUCB **ppfucb, const LEVEL level )
{
    ERR     err;
    FUCB    *pfucb;

    Assert( PinstFromPpib( ppib )->RwlTrx( ppib ).FWriter() );

    Assert( level > 0 );
    Assert( pfcb != pfcbNil );
    Assert( pfcb->FInitialized() );

    // This routine only called by concurrent create index to obtain a cursor
    // on a secondary index tree.
    Assert( pfcb->FTypeSecondaryIndex() );
    Assert( pfcbNil == pfcb->PfcbTable() ); // FCB not yet linked into table's index list
    Assert( !pfcb->FInitialIndex() );

    // We want to make sure the FUCB doesn't disappear out from under us.
    pfcb->Lock();

    // We want to make sure no one modifies the list of FUCBs linked to this FCB while we're doing this.
    pfcb->FucbList().LockForEnumeration();

    for ( INT ifucbIndex = 0; ifucbIndex < pfcb->FucbList().Count(); ifucbIndex++ )
    {
        pfucb = pfcb->FucbList()[ifucbIndex];

        if ( pfucb->ppib == ppib )
        {
            const BOOL  fVersioned  = FFUCBVersioned( pfucb );

            // If there are any cursors on this tree at all for this user, then
            // it must be deferred closed, because we only use the cursor to
            // insert into the version store, then close it.
            Assert( FFUCBDeferClosed( pfucb ) );

            Assert( !FFUCBNotReuse( pfucb ) );
            Assert( !FFUCBSpace( pfucb ) );
            Assert( FFUCBIndex( pfucb ) );
            Assert( FFUCBSecondary( pfucb ) );
            Assert( pfucb->u.pfcb == pfcb );
            Assert( ppib->Level() > 0 );
            Assert( pfucb->levelOpen > 0 );

            // We expect the reuse level to be zero given that the secondary index is still 
            // being populated and no one other than the populating thread should have 
            // visibility on it (and thus can't open a cursor on it).
            Assert( 0 == pfucb->levelReuse );

            // Temporarily set levelOpen to 0 to ensure
            // that ErrIsamRollback() doesn't close the
            // FUCB on us.
            pfucb->levelOpen = 0;

            // Reset all used flags. Keep Updatable (fWrite) flag
            //
            FUCBResetFlags( pfucb );
            Assert( !FFUCBDeferClosed( pfucb ) );

            pfucb->pfucbTable = NULL;

            FUCBResetPreread( pfucb );
            FUCBResetOpportuneRead( pfucb );

            //  must persist Versioned flag
            Assert( !FFUCBVersioned( pfucb ) );     //  got reset by FUCBResetFlags()
            if ( fVersioned )
                FUCBSetVersioned( pfucb );

            // Set fIndex/fSecondary flags to ensure FUCB
            // doesn't get closed by ErrIsamRollback(), then
            // set proper levelOpen.
            FUCBSetIndex( pfucb );
            FUCBSetSecondary( pfucb );
            pfucb->levelOpen = level;

            Assert( !g_rgfmp[ pfcb->Ifmp() ].FReadOnlyAttach() );
            FUCBSetUpdatable( pfucb );

            pfcb->FucbList().UnlockForEnumeration();
            pfcb->Unlock();

            *ppfucb = pfucb;

            return JET_errSuccess;
        }
    }

    pfcb->FucbList().UnlockForEnumeration();
    pfcb->Unlock();

    pfucb = pfucbNil;

    Assert( level > 0 );
    Call( ErrFUCBOpen(
              ppib,
              pfcb->Ifmp(),
              &pfucb,
              level ) );

    // Must have these flags set BEFORE linking into session list to
    // ensure ErrIsamRollback() doesn't close the FUCB prematurely.
    Assert( FFUCBIndex( pfucb ) );
    Assert( FFUCBSecondary( pfucb ) );

    //  link FCB
    //
    Call( pfcb->ErrLink( pfucb ) );

    *ppfucb = pfucb;

    return JET_errSuccess;

HandleError:
    Assert( err < 0 );
    if ( pfucbNil != pfucb )
    {
        if ( pfcbNil != pfucb->u.pfcb )
        {
            Assert( pfcb == pfucb->u.pfcb );
            // We managed to link the FUCB to the FCB before we errored.
            pfucb->u.pfcb->Unlink( pfucb );
        }
        //  close the FUCB
        FUCBClose( pfucb );
    }
    return err;
}


//  closes cursor at BT level
//  releases BT level resources
//
VOID BTClose( FUCB *pfucb )
{
    INST * const    pinst   = PinstFromPfucb( pfucb );
    LOG * const     plog    = pinst->m_plog;

    FUCBAssertNoSearchKey( pfucb );

    // Current secondary index should already have been closed.
    Assert( !FFUCBCurrentSecondary( pfucb ) );
    // Should have been used and/or reset prior to close.
    Assert( pfucb->cpgSpaceRequestReserve == 0 );

    //  release memory used by bookmark buffer
    //
    BTReleaseBM( pfucb );

    if ( Pcsr( pfucb )->FLatched() )
    {
        if( pfucb->pvRCEBuffer )
        {
            OSMemoryHeapFree( pfucb->pvRCEBuffer );
            pfucb->pvRCEBuffer = NULL;
        }
        Pcsr( pfucb )->ReleasePage( pfucb->u.pfcb->FNoCache() );
    }

    Assert( pfucb->u.pfcb != pfcbNil );
    if ( pfucb->fInRecoveryTableHash )
    {
        Assert( plog->FRecovering() );
        //  delete reference to FUCB from hash table
        //
        plog->LGRRemoveFucb( pfucb );
    }

    //  if cursor created version,
    //      defer close until commit to transaction level 0
    //      since rollback needs cursor
    //
    if ( pfucb->ppib->Level() > 0 && FFUCBVersioned( pfucb ) )
    {
        Assert( !FFUCBSpace( pfucb ) );     // Space operations not versioned.
        RECRemoveCursorFilter( pfucb );
        FUCBRemoveEncryptionKey( pfucb );
        FUCBSetDeferClose( pfucb );
    }
    else
    {
        FCB *pfcb = pfucb->u.pfcb;

        //  reset FCB flags associated with this cursor
        //
        if ( FFUCBDenyRead( pfucb ) )
        {
            pfcb->ResetDomainDenyRead();
        }
        if ( FFUCBDenyWrite( pfucb ) )
        {
            pfcb->ResetDomainDenyWrite();
        }

        if ( !pfcb->FInitialized() )
        {

            //  we own the FCB (we're closing because the FCB was created during
            //      a DIROpen() of a DIRCreateDirectory() or because an error
            //      occurred during FILEOpenTable())

            //  unlink the FUCB from the FCB without moving the FCB to the
            //      avail LRU list (this prevents the FCB from being purged)

            pfucb->u.pfcb->Unlink( pfucb, fTrue );

            //  synchronously purge the FCB

            pfcb->PrepareForPurge();
            pfcb->Purge();
        }
        else if ( pfcb->FTypeTable() )
        {

            //  only table FCBs can be moved to the avail-LRU list

            //  unlink the FUCB from the FCB and place the FCB in the avail-LRU
            //      list so it can be used or purged later

            pfucb->u.pfcb->Unlink( pfucb );
        }
        else
        {

            //  all other types of FCBs will not be allowed in the avail-LRU list
            //
            //  NOTE: these FCBs must be purged manually!

            //  possible reasons why we are here:
            //      - we were called from ErrFILECloseTable() and have taken the
            //          special temp-table path
            //      - we are closing a sort FCB
            //      - ???
            //
            //  NOTE: database FCBs will never be purged because they are never
            //          available (PgnoFDP() == pgnoSystemRoot); these FCBs will
            //          be cleaned up when the database is detached or the instance
            //          is closed
            //  NOTE: sentinel FCBs will never be purged because they are never
            //          available either (FTypeSentinel()); these FCBs will be
            //          purged by version cleanup

            pfucb->u.pfcb->Unlink( pfucb, fTrue );
        }

        //  close the FUCB

        FUCBClose( pfucb );
    }
}


//  ******************************************************
//  retrieve/release operations
//

//  UNDONE: INLINE the following functions

//  gets node in pfucb->kdfCurr
//  refreshes currency to point to node
//  if node is versioned get correct version from the version store
//
ERR ErrBTGet( FUCB *pfucb )
{
    ERR         err;
    const BOOL  fBookmarkPreviouslySaved    = pfucb->fBookmarkPreviouslySaved;
    PIBTraceContextScope tcScope        = TcBTICreateCtxScope( pfucb, iorsBTGet );

    Call( ErrBTIRefresh( pfucb ) );
    CallS( err );
    Assert( Pcsr( pfucb )->FLatched() );

    BOOL    fVisible;
    err = ErrNDVisibleToCursor( pfucb, &fVisible, NULL );
    if ( err < 0 )
    {
        BTUp( pfucb );
    }
    else if ( !fVisible )
    {
        BTUp( pfucb );
        err = ErrERRCheck( JET_errRecordDeleted );
    }
    else
    {
        //  if this flag was FALSE when we first came into
        //  this function, it means the bookmark has yet
        //  to be saved (so leave it to FALSE so that
        //  BTRelease() will know to save it)
        //  if this flag was TRUE when we first came into
        //  this function, it means that we previously
        //  saved the bookmark already, so no need to
        //  re-save it (if NDGet() was called in the call
        //  to ErrBTIRefresh() above, it would have set
        //  the flag to FALSE, which is why we need to
        //  set it back to TRUE here)
        pfucb->fBookmarkPreviouslySaved = fBookmarkPreviouslySaved;
    }

HandleError:
    Assert( err >= 0 || !Pcsr( pfucb )->FLatched() );
    return err;
}


//  releases physical currency,
//  save logical currency
//  then, unlatch page
//
ERR ErrBTRelease( FUCB  *pfucb )
{
    ERR err = JET_errSuccess;

    if ( Pcsr( pfucb )->FLatched() )
    {
        if ( !pfucb->fBookmarkPreviouslySaved )
        {
            err = ErrBTISaveBookmark( pfucb );
        }
        if ( err >= JET_errSuccess )
        {
            AssertBTIBookmarkSaved( pfucb );
        }
        pfucb->ullLTCurr = pfucb->ullLTLast;
        pfucb->ullTotalCurr = pfucb->ullTotalLast;

        // release page anyway, return previous error
        Pcsr( pfucb )->ReleasePage( pfucb->u.pfcb->FNoCache() );
        if( NULL != pfucb->pvRCEBuffer )
        {
            OSMemoryHeapFree( pfucb->pvRCEBuffer );
            pfucb->pvRCEBuffer = NULL;
        }
    }

    //  We have touched, no longer need to touch again.
    //
    pfucb->fTouch = fFalse;

#ifdef DEBUG
    pfucb->kdfCurr.Invalidate();
#endif  //  DEBUG

    return err;
}

//  saves given bookmark in cursor
//  and resets physical currency
//
ERR ErrBTDeferGotoBookmark( FUCB *pfucb, const BOOKMARK& bm, BOOL fTouch )
{
    ERR     err;

    CallR( ErrBTISaveBookmark( pfucb, bm, fTouch ) );
    BTUp( pfucb );

    //  purge our cached key position

    pfucb->ullLTCurr = 0;
    pfucb->ullTotalCurr = 0;

    return err;
}


//  saves logical currency -- bookmark only
//  must be called before releasing physical currency
//  CONSIDER: change callers to not ask for bookmarks if not needed
//  CONSIDER: simplify by invalidating currency on resource allocation failure
//
//  tries to save primary key [or data] in local cache first
//  since it has higher chance of fitting
//  and in many cases we can refresh currency using just the primary key
//  bookmark save operation should be after all resources are allocated,
//  so resource failure would still leave previous bm valid
//
ERR ErrBTISaveBookmark( FUCB *pfucb, const BOOKMARK& bm, BOOL fTouch )
{
    ERR         err     = JET_errSuccess;
    const UINT  cbKey   = bm.key.Cb();
    const UINT  cbData  = bm.data.Cb();
    BOOL        fUnique = FFUCBUnique( pfucb );

    Assert( !fUnique || pfucb->bmCurr.data.FNull() );
    Assert( NULL == bm.key.suffix.Pv() || bm.key.suffix.Pv() != pfucb->bmCurr.key.suffix.Pv() );
    Assert( NULL == bm.key.prefix.Pv() || bm.key.prefix.Pv() != pfucb->bmCurr.key.prefix.Pv() );
    Assert( NULL == bm.data.Pv() || bm.data.Pv() != pfucb->bmCurr.data.Pv() );

    //  if we need more storage then is provided in our local buffer then we
    //  will use an external buffer
    //
    if ( !pfucb->pvBMBuffer && ( fUnique ? cbKey : cbKey + cbData ) > cbBMCache )
    {
        Alloc( pfucb->pvBMBuffer = RESBOOKMARK.PvRESAlloc() );
    }

    //  Record if we are going to touch the data page of this bookmark

    pfucb->fTouch = fTouch;

    //  copy key
    //
    pfucb->bmCurr.key.Nullify();
    if ( !pfucb->pvBMBuffer )
    {
        Assert( cbKey <= cbBMCache );
        pfucb->bmCurr.key.suffix.SetPv( pfucb->rgbBMCache );
    }
    else
    {
        Assert( cbKey <= cbBookmarkAlloc );
        pfucb->bmCurr.key.suffix.SetPv( pfucb->pvBMBuffer );
    }
    pfucb->bmCurr.key.suffix.SetCb( cbKey );
    bm.key.CopyIntoBuffer( pfucb->bmCurr.key.suffix.Pv() );

    //  copy data
    //
    if ( fUnique )
    {
        //  bookmark does not need data
        //
        pfucb->bmCurr.data.Nullify();
    }
    else
    {
        if ( !pfucb->pvBMBuffer )
        {
            Assert( cbKey + cbData <= cbBMCache );
            pfucb->bmCurr.data.SetPv( pfucb->rgbBMCache + cbKey );
        }
        else
        {
            Assert( cbKey + cbData <= cbBookmarkAlloc );
            pfucb->bmCurr.data.SetPv( (BYTE*)pfucb->pvBMBuffer + cbKey );
        }

        bm.data.CopyInto( pfucb->bmCurr.data );
    }

    //  Record that we now have the bookmark saved

    pfucb->fBookmarkPreviouslySaved = fTrue;

HandleError:
    return err;
}



//  ================================================================
LOCAL BOOL FKeysEqual( const KEY& key1, const BYTE* const pbKey2, const ULONG cbKey )
//  ================================================================
//
//  Compares two keys, using length as a tie-breaker
//
//-
{
    if( (ULONG)key1.Cb() == cbKey )
    {
        KEY     key2;
        key2.prefix.Nullify();
        key2.suffix.SetPv( ( VOID* )pbKey2 );
        key2.suffix.SetCb( cbKey );
        return ( 0 == CmpKeyShortest( key1, key2 ) );
    }
    return fFalse;
}


LOCAL ERR ErrBTIReportBadPageLink(
    FUCB        * const pfucb,
    const ERR   err,
    const PGNO  pgnoComingFrom,
    const PGNO  pgnoGoingTo,
    const PGNO  pgnoBadLink,    //  may actually be an OBJID if the bad link is due to mismatched objids
    const BOOL  fFatal,
    const CHAR * const szTag )
{
    //  only report the error if not repairing
    if ( !g_fRepair )
    {
        OSTraceSuspendGC();
        const WCHAR* rgwsz[] =
        {
            OSFormatW( L"%d", err ),
            OSFormatW( L"%I32u", pfucb->u.pfcb->ObjidFDP() ),
            OSFormatW( L"%I32u", pfucb->u.pfcb->PgnoFDP() ),
            g_rgfmp[pfucb->u.pfcb->Ifmp()].WszDatabaseName(),
            OSFormatW( L"%I32u", pgnoComingFrom ),
            OSFormatW( L"%I32u", pgnoGoingTo ),
            OSFormatW( L"%I32u", pgnoBadLink ),
            OSFormatW( L"%hs", szTag ),
            OSFormatW( L"%d", (int)fFatal ),
        };
        UtilReportEvent(
                eventError,
                DATABASE_CORRUPTION_CATEGORY,
                BAD_PAGE_LINKS_ID,
                _countof( rgwsz ),
                rgwsz,
                0,
                NULL,
                PinstFromPfucb( pfucb ) );
        if ( fFatal )
        {
            OSUHAPublishEvent(
                HaDbFailureTagCorruption, PinstFromPfucb( pfucb ), HA_DATABASE_CORRUPTION_CATEGORY,
                HaDbIoErrorNone, g_rgfmp[pfucb->u.pfcb->Ifmp()].WszDatabaseName(), 0, 0,
                HA_BAD_PAGE_LINKS_ID, _countof( rgwsz ), rgwsz );
        }
        OSTraceResumeGC();
    }

    Assert( JET_errBadPageLink == err || JET_errBadParentPageLink == err );
    return err;
}

#ifdef DEBUG
__int64 memcnt( const BYTE * const pb, const BYTE bTarget, const size_t cb )
{
    __int64 cnt = 0;
    for( size_t ib = 0; ib < cb; ib++ )
    {
        if( bTarget == pb[ib] )
        {
            cnt++;
        }
    }
    return cnt;
}
#endif // DEBUG

struct BTISKIPSTATS
{
private:
    //  Base State (used for defer init)
    ULONG       cNodesSkipped;

    //  Per Node Tracking
    //
    ULONG       cUnversionedDeletes;
    ULONG       cUncommittedDeletes;
    ULONG       cCommittedDeletes;
    ULONG       cNonVisibleInserts;
    ULONG       cFiltered;

    //  Per Page Tracking
    //
    ULONG       cPagesVisitedWithSkippedNodes;
    ULONG       cPagesCacheMissBaseline;
    __int64     cusecCacheMissBaseline;
    DBTIME      dbtimeSkippedPagesHigh;
    DBTIME      dbtimeSkippedPagesAve;  // initially a total, then divided by cPagesVisitedWithSkippedNodes at beginning of BTIReportManyNonVisibleNodesSkipped()
    DBTIME      dbtimeSkippedPagesLow;

    //  Total Time
    //
    HRT     hrtStart;   //  actually of deferred init - close enough

    //  Deferred init
    //
    OnDebug( BYTE   bFill );

public:
    void BTISkipStatsIDeferredInit()
    {
        //  <= b/c if bFill == 0, then cNodesSkipped matches the pattern fill too
        Assert( ( sizeof(*this) - sizeof(cNodesSkipped) ) <= memcnt( (BYTE*)this, bFill, sizeof(*this) ) );
        memset( this, 0, sizeof(*this) );
        hrtStart = HrtHRTCount();
    }

public:
    //  Init
    //

    BTISKIPSTATS()
    {
#ifdef DEBUG
        bFill = rand() % 255;
        memset( this, bFill, sizeof(*this) );
#endif
        //  In retail we will only initialize this piece and defer initialize the rest ...
        cNodesSkipped = 0;
    }

    //  Node Skipping Update routines
    //
    void BTISkipStatsUpdateSkippedNode( const NS ns )
    {
        if ( cNodesSkipped == 0 )
        {
            BTISkipStatsIDeferredInit();
        }
        cNodesSkipped++;
        switch ( ns )
        {
            case nsUncommittedVerInDB:
                cUncommittedDeletes++;
                break;
            case nsCommittedVerInDB:
                cCommittedDeletes++;
                break;
            case nsVersionedInsert:
                cNonVisibleInserts++;
                break;
            default:
                Assert( nsDatabase == ns );
                cUnversionedDeletes++;
                break;
        }
    }
    void BTISkipStatsUpdateSkippedDeleted()
    {
        if ( cNodesSkipped == 0 )
        {
            BTISkipStatsIDeferredInit();
        }
        cNodesSkipped++;
        cUnversionedDeletes++;
    }
    void BTISkipStatsUpdateFilteredNode()
    {
        if ( cNodesSkipped == 0 )
        {
            BTISkipStatsIDeferredInit();
        }
        cNodesSkipped++;
        cFiltered++;
    }

    //  Page Switching Update
    //
    void BTISkipStatsUpdateSkippedPage( const CPAGE& cpage )
    {
        Assert( cNodesSkipped != 0 );   //

        if ( cPagesVisitedWithSkippedNodes == 0 )
        {
            //  Skipping off a page for first time, get some initial state ...
            cPagesCacheMissBaseline = Ptls()->threadstats.cPageCacheMiss;
            cusecCacheMissBaseline = Ptls()->threadstats.cusecPageCacheMiss;
            dbtimeSkippedPagesLow = 0x7FFFFFFFFFFFFFFF;
        }

        cPagesVisitedWithSkippedNodes++;
        dbtimeSkippedPagesHigh = max( dbtimeSkippedPagesHigh, cpage.Dbtime() );
        dbtimeSkippedPagesLow  = min( dbtimeSkippedPagesLow, cpage.Dbtime() );
        dbtimeSkippedPagesAve += cpage.Dbtime();
    }

    //  Performance Counter Details
    //

    ULONG CNodesSkipped() const
    {
        return cNodesSkipped;
    }

    ULONG CFiltered() const
    {
        if ( cNodesSkipped == 0 )
        {
            return 0;
        }
        return cFiltered;
    }

    //  Report Check(s)
    //

    INLINE BOOL FBTIReportManyCacheMissesForSkippedNodes() const
    {
        const TLS * const ptls = Ptls();

        const ULONG cpgFaulted = ptls->threadstats.cPageCacheMiss - cPagesCacheMissBaseline;
        if ( cPagesCacheMissBaseline == 0 ||
                cpgFaulted <= g_cBTIPagesCacheMissedForNonVisibleNodes )
        {
            return fFalse;
        }

        const __int64 cmsecCacheMisses = ( ptls->threadstats.cusecPageCacheMiss - cusecCacheMissBaseline ) / 1000;
        if ( cusecCacheMissBaseline == 0 ||
                cmsecCacheMisses < g_cmsecBTINavigateCacheMissLatencyThreshold /* ~10 secs in ms, at the moment */ )
        {
            return fFalse;
        }

        const double    dblTimeElapsed = DblHRTElapsedTimeFromHrtStart( hrtStart );
        const double    dblFaultWait = double( Ptls()->threadstats.cusecPageCacheMiss - cusecCacheMissBaseline ) / 1000.0;
        if ( dblFaultWait > dblTimeElapsed ||
                ( dblFaultWait / dblTimeElapsed ) < g_pctBTICacheMissLatencyToQualifyForIoLoad /* 80% at the moment */ )
        {
            return fFalse;
        }

        return fTrue;
    }

    INLINE BOOL FBTIReportManyNonVisibleNodesSkipped() const
    {
        Assert( cNodesSkipped == 0 || cNodesSkipped == cUnversionedDeletes + cUncommittedDeletes + cCommittedDeletes + cNonVisibleInserts + cFiltered );

        // Note: The - 1, may cause a curio ... because in ErrBTNext/Prev() we call FBTIReportManyNonVisibleNodesSkipped() before 
        // we sum up the very last page with BTISkipStatsUpdateSkippedPage() it means we don't have a full page count, so we will
        // assume there are skipped nodes on this last page (as this is almost assuredly the case), and by subtracting -1 from the 
        // threshold we use to trigger - but this may lead to a curio of triggering with 1 less page than the threshold.
        // Note2: The filtered results sets is designed actually to skip a lot of nodes, so we don't want to print a result such
        // an event unless the skipping is the fault of our ESE non-visible nodes.
        C_ASSERT( g_cBTIPagesVisitedWithNonVisibleNodesWarningThreshold > 20 ); // let's ensure this curio is < 5% margin

        // because it/g_cmsecBTINavigateProcessingLatencyThreshold is checked first before we call into FBTIReportManyCacheMissesForSkippedNodes
        // to check g_cmsecBTINavigateCacheMissLatencyThreshold, then the Processing must be smaller than the CacheMiss threshold.
        Assert( (__int64)g_cmsecBTINavigateProcessingLatencyThreshold < g_cmsecBTINavigateCacheMissLatencyThreshold );

        return cNodesSkipped &&             //  some nodes skipped
                ( cNodesSkipped - cFiltered ) &&    //  and some non-filtered nodes skipped
                ( DblHRTElapsedTimeFromHrtStart( hrtStart ) >= g_cmsecBTINavigateProcessingLatencyThreshold ) &&    // took at least 100 ms at moment (for either CPU or Disk events)
                //  And one of the three cases is inefficient enough to care about eventing ...
                ( ( cNodesSkipped - cFiltered ) > g_cBTINonVisibleNodesSkippedWarningThreshold
                    || cPagesVisitedWithSkippedNodes > ( g_cBTIPagesVisitedWithNonVisibleNodesWarningThreshold - 1 )
                    || FBTIReportManyCacheMissesForSkippedNodes() );
    }

    //  Generate Report
    //

    VOID BTIReportManyNonVisibleNodesSkipped( FUCB * const pfucb )
    {
        Assert( cNodesSkipped == cUnversionedDeletes + cUncommittedDeletes + cCommittedDeletes + cNonVisibleInserts + cFiltered );
        FCB * const     pfcb                        = pfucb->u.pfcb;

        //  finalize some values
        //
        //  note: In the future we may want to use something like faulted pages or elapsed time to determine
        //  if we really should log an event, should be pretty easy to move upto FBTIReportManyNonVisibleNodesSkipped
        //  at that point.  Take care to do no more compute than necessary for the common case.
        const double    dblTimeElapsed = DblHRTElapsedTimeFromHrtStart( hrtStart );
        const ULONG     cpgFaulted = Ptls()->threadstats.cPageCacheMiss - cPagesCacheMissBaseline;
        const double    dblFaultWait = double( Ptls()->threadstats.cusecPageCacheMiss - cusecCacheMissBaseline ) / 1000.0;

        Assert( cPagesVisitedWithSkippedNodes );
        dbtimeSkippedPagesAve = dbtimeSkippedPagesAve / cPagesVisitedWithSkippedNodes;


        //  need to serialise updating of counters
        //  that are stored in the FCB
        //
        pfcb->Lock();

        const TICK      tickCurr                    = TickOSTimeCurrent();
        const TICK      tickLast                    = pfcb->TickMNVNLastReported();
        const TICK      tickDiff                    = ( 0 == tickLast ? 0 : tickCurr - tickLast );
        const ULONG     ulHoursSinceLastReported    = tickDiff / ( 1000 * 60 * 60 );    //  convert milliseconds to hours
        const ULONG     cPreviousIncidents          = pfcb->CMNVNIncidentsSinceLastReported();
        const ULONG     cPreviousNodesSkipped       = pfcb->CMNVNNodesSkippedSinceLastReported();
        const ULONG     cPreviousPagesVisited       = pfcb->CMNVNPagesVisitedSinceLastReported();
        const BOOL      fInitialReport              = ( 0 == tickLast );
        const BOOL      fIoLoadIssue                = FBTIReportManyCacheMissesForSkippedNodes(); // as opposed to all cached pages / mere CPU issue.
#ifdef ESENT
        //  disabling this eventlog message because it is too noisy and the
        //  suggested action (increase maintenance window) may not be possible
        //  for all uses of the database engine
        //
        //  CONSIDER:  move this event to ETW as an advanced perf diagnostic
        //
        const BOOL      fReportEvent                = fFalse;
#else
        //  this event is very useful for identifying potential perf issues, but
        //  ultimately there is very little actionable by the admin, so report
        //  this event at a higher logging level
        //
        //  thresholds have been tweaked such that this event
        //  is now only emitted for extremely egregious offenders, so it
        //  should be okay to emit this event at the lowest logging level
        //  without fear of excessive eventlog spam
        //
        const BOOL      lTargetLoggingLevel         = JET_EventLoggingLevelMin;
        const BOOL      fReportEvent                = ( UlParam( PinstFromPfucb( pfucb ), JET_paramEventLoggingLevel ) >= (ULONG_PTR)lTargetLoggingLevel )
                                                        && ( fInitialReport || ulHoursSinceLastReported >= g_ulBTINonVisibleNodesSkippedWarningFrequency )
                                                        && ( fIoLoadIssue ?
                                                                g_rgfmp[ pfcb->Ifmp() ].FCheckTreeSkippedNodesDisk( pfcb->ObjidFDP() ) :
                                                                g_rgfmp[ pfcb->Ifmp() ].FCheckTreeSkippedNodesCpu( pfcb->ObjidFDP() ) );
#endif  //  ESENT

        if ( fReportEvent )
        {
            //  event is being reported, so reset counters
            //
            pfcb->SetTickMNVNLastReported( tickCurr );
            pfcb->ResetCMNVNIncidentsSinceLastReported();
            pfcb->ResetCMNVNNodesSkippedSinceLastReported();
            pfcb->ResetCMNVNPagesVisitedSinceLastReported();
        }
        else
        {
            //  event is going to be suppressed, so increment counts
            //  (these counters are only ULONGs, so they may wrap,
            //  especially the nodes-skipped counter, but they are
            //  purely informational, so it's not a big deal)
            //
            if ( fInitialReport )
                pfcb->SetTickMNVNLastReported( tickCurr );
            pfcb->IncrementCMNVNIncidentsSinceLastReported();
            pfcb->IncrementCMNVNNodesSkippedSinceLastReported( cNodesSkipped );
            pfcb->IncrementCMNVNPagesVisitedSinceLastReported( cPagesVisitedWithSkippedNodes );
        }

        pfcb->Unlock();


        if ( fReportEvent )
        {
            ULONG           cbKey       = pfucb->bmCurr.key.Cb();
            BYTE *          pbKey       = NULL;
            const ULONG     csz         = 21;
            WCHAR           rgszDw[csz][16];
            WCHAR           szDbtimes[80];
            const WCHAR *   rgsz[csz];
            CAutoWSZDDL     szBtreeName;
            CAutoWSZDDL     szTableName;

            if ( pfcb->FTypeTable() )
            {
                pfcb->EnterDML();
                CallS( szBtreeName.ErrSet( pfcb->Ptdb()->SzTableName() ) );
                CallS( szTableName.ErrSet( pfcb->Ptdb()->SzTableName() ) );
                pfcb->LeaveDML();
            }
            else if ( pfcb->FTypeSecondaryIndex() )
            {
                if ( pfcbNil != pfcb->PfcbTable()
                    && pidbNil != pfcb->Pidb() )
                {
                    pfcb->PfcbTable()->EnterDML();
                    CallS( szBtreeName.ErrSet(
                                            pfcb->PfcbTable()->Ptdb()->SzIndexName(
                                                                            pfcb->Pidb()->ItagIndexName(),
                                                                            pfcb->FDerivedIndex() ) ) );
                    CallS( szTableName.ErrSet( pfcb->PfcbTable()->Ptdb()->SzTableName() ) );
                    pfcb->PfcbTable()->LeaveDML();
                }
                else
                {
                    CallS( szBtreeName.ErrSet( "???" ) );
                    CallS( szTableName.ErrSet( "???" ) );
                }
            }
            else if ( pfcb->FTypeLV() )
            {
                CallS( szBtreeName.ErrSet( "<LV>" ) );
                if ( pfcbNil != pfcb->PfcbTable() )
                {
                    pfcb->PfcbTable()->EnterDML();
                    CallS( szTableName.ErrSet( pfcb->PfcbTable()->Ptdb()->SzTableName() ) );
                    pfcb->PfcbTable()->LeaveDML();
                }
                else
                {
                    CallS( szTableName.ErrSet( "???" ) );
                }
            }
            else
            {
                //  another FCB type
                //
                CallS( szBtreeName.ErrSet( "<null>" ) );
                CallS( szTableName.ErrSet( "<null>" ) );
            }

            //  event args
            //
            //  Note: the isz++ at end; for clarity we attempt to use one line / string - unless it is really long
            //
            ULONG   cszSkippedSubsequentReportArgs = 0;
            ULONG   isz                     = 0;
            rgsz[ isz ] = g_rgfmp[ pfcb->Ifmp() ].WszDatabaseName(); isz++;
            rgsz[ isz ] = (WCHAR *)szBtreeName; isz++;
            rgsz[ isz ] = (WCHAR *)szTableName; isz++;
            OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", cNodesSkipped ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", cPagesVisitedWithSkippedNodes ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", pfcb->ObjidFDP() ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", pfcb->PgnoFDP() ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", pfcb->TCE( FFUCBSpace( pfucb ) ) ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", cUnversionedDeletes ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", cUncommittedDeletes ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", cCommittedDeletes ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", cNonVisibleInserts ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            if ( !fInitialReport )
            {
                OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", ulHoursSinceLastReported ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
                OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", cPreviousIncidents ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
                OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", cPreviousNodesSkipped ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
                OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", cPreviousPagesVisited ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            }
            else
            {
                cszSkippedSubsequentReportArgs = 4;
            }
            OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", cFiltered ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%0.3f", dblTimeElapsed * 1000.0 ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%0.3f", dblFaultWait ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            OSStrCbFormatW( rgszDw[ isz ], sizeof( rgszDw[ isz ] ), L"%u", cpgFaulted ); rgsz[ isz ] = rgszDw[ isz ]; isz++;
            OSStrCbFormatW( szDbtimes, sizeof( szDbtimes ), L"%I64d, %I64d, %I64d (%I64d)",
                        dbtimeSkippedPagesLow, dbtimeSkippedPagesAve, dbtimeSkippedPagesHigh,
                        g_rgfmp[ pfucb->ifmp ].DbtimeLast() );
                        rgsz[ isz ] = szDbtimes; isz++;

            Assert( csz == ( isz + cszSkippedSubsequentReportArgs ) );

            //  we might not have a bookmark if there was no currency before
            //  the BTNext/Prev (eg. the operation being performed may be
            //  something like a seek or a MoveFirst/Last)
            //
            if ( cbKey > 0 )
            {
                //  try to allocate a buffer so that we can report the
                //  key of the node we started the move from
                //
                pbKey = (BYTE *)RESKEY.PvRESAlloc();
                if ( NULL != pbKey )
                {
                    pfucb->bmCurr.key.CopyIntoBuffer( pbKey, cbKeyAlloc );
                }
                else
                {
                    //  failed allocating buffer for the key, so just don't
                    //  bother reporting it
                    //
                    cbKey = 0;
                }
            }

            if ( fInitialReport )
            {
                UtilReportEvent(
                        eventWarning,
                        PERFORMANCE_CATEGORY,
                        // note future comment above ... _AGAIN_ID logging is effectively disabled here.
                        ( fInitialReport ? BT_MOVE_SKIPPED_MANY_NODES_ID : BT_MOVE_SKIPPED_MANY_NODES_AGAIN_ID ),
                        isz,
                        rgsz,
                        cbKey,
                        pbKey,
                        PinstFromPfucb( pfucb ) );
            }

            RESKEY.Free( pbKey );
        }
    }

};  //  end struct BTISKIPSTATS


// the current node is deleted and committed and has no active versions
BOOL FBTINodeCommittedDelete( const FUCB * pfucb, const KEYDATAFLAGS& kdf )
{
    BOOL fDeleted = fFalse;

    Assert( pfucb );

    // if the node is deleted ...
    if ( FNDDeleted( kdf ) )
    {
        if ( !FNDVersion( kdf ) )
        {
            fDeleted = fTrue;
        }
        else
        {
            BOOKMARK bm;
            bm.key = kdf.key;

            // sequential indexes are unique and don't have an IDB set
            if ( FFUCBUnique( pfucb ) )
            {
                bm.data.Nullify();
            }
            else
            {
                bm.data = kdf.data;
            }

            // but we don't have any active versions
            if( !FVERActive( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bm, TrxOldest( PinstFromPfucb( pfucb ) ) ) )
            {
                fDeleted = fTrue;
            }
        }
    }

    return fDeleted;
}

// the current node is deleted and committed and has no active versions
BOOL FBTINodeCommittedDelete( const FUCB * pfucb )
{
    return FBTINodeCommittedDelete( pfucb, pfucb->kdfCurr );
}

//  returns true if the FUCB/database attached is in a logically navigable state
//
BOOL FBTLogicallyNavigableState( const FUCB * const pfucb )
{
    if ( !PinstFromPfucb( pfucb )->m_plog->FRecovering() ||                             //  during do-time we're fine
         fRecoveringRedo != PinstFromPfucb( pfucb )->m_plog->FRecoveringMode() ||   //  or during undo-time as well
         !g_rgfmp[ pfucb->ifmp ].FContainsDataFromFutureLogs() ||
         g_rgfmp[ pfucb->ifmp ].m_fCreatingDB )                                       //  and finally during create DB
    {
        return fTrue;
    }
    return fFalse;
}

//  goto the next line in tree
//
ERR ErrBTNext( FUCB *pfucb, DIRFLAG dirflag )
{
    // function called with dirflag == fDIRAllNode from eseutil - node dumper
    // Assert( ! ( dirflag & fDIRAllNode ) );

    ERR         err;
    CSR * const pcsr                        = Pcsr( pfucb );
    BYTE*       pbkeySave                   = NULL;
    ULONG       cbKeySave                   = 0;
    ULONG       cNeighbourKeysSkipped       = 0;
    BOOL        fNodesSkippedOnCurrentPage  = fFalse;
    BTISKIPSTATS    btskipstats;
    PIBTraceContextScope tcScope        = TcBTICreateCtxScope( pfucb, iorsBTMoveNext );

    // Prevent R/O transactions from proceeding if they are holding up version store cleanup.
    //
    if ( pfucb->ppib->FReadOnlyTrx() && pfucb->ppib->Level() > 0 )
    {
        CallR( PverFromIfmp( pfucb->ifmp )->ErrVERCheckTransactionSize( pfucb->ppib ) );
    }

    Assert( FBTLogicallyNavigableState( pfucb ) );

    //  if a filter is provided, then we must be navigating a table or secondary index
    //
    Assert( !pfucb->pmoveFilterContext || pfucb->u.pfcb->FTypeTable() || pfucb->u.pfcb->FTypeSecondaryIndex() );

    //  refresh currency
    //  places cursor on record to move from
    //
    CallR( ErrBTIRefresh( pfucb ) );

    //  lidMin is never used, the first lid should be lidMin+1

    LvId lid = lidMin;
    if( dirflag & fDIRSameLIDOnly )
    {
        //  we are on the LV tree, store the LID we currently have

        Assert( pfucb->u.pfcb->FTypeLV() );
        LidFromKey( &lid, pfucb->kdfCurr.key );
        Assert( lidMin != lid );
    }

Start:

    //  now we have a read latch on page
    //
    Assert( pcsr->Latch() == latchReadTouch ||
            pcsr->Latch() == latchReadNoTouch ||
            pcsr->Latch() == latchRIW );
    Assert( ( pcsr->Cpage().FLeafPage() ) != 0 );
    AssertNDGet( pfucb );

    //  get next node in page
    //  if it does not exist, go to next page
    //
    if ( pcsr->ILine() + 1 < pcsr->Cpage().Clines() )
    {
        //  get next node
        //
        pcsr->IncrementILine();
        NDGet( pfucb );

        //  adjust key position
        //
        if ( pfucb->ullTotalLast )
        {
            pfucb->ullLTLast++;
        }
    }
    else
    {
        Assert( pcsr->ILine() + 1 == pcsr->Cpage().Clines() );

        //  reset key position
        //
        pfucb->ullLTLast = 0;
        pfucb->ullTotalLast = 0;

        //  next node not in current page
        //  get next page and continue
        //
        if ( pcsr->Cpage().PgnoNext() == pgnoNull )
        {
            //  past the last node
            //  callee must set loc to AfterLast
            //
            Error( ErrERRCheck( JET_errNoCurrentRecord ) );
        }
        else
        {
            const PGNO  pgnoFrom    = pcsr->Pgno();

            //  before we move pages, track any stats from this page

            if ( fNodesSkippedOnCurrentPage )
            {
                btskipstats.BTISkipStatsUpdateSkippedPage( pcsr->Cpage() );
                fNodesSkippedOnCurrentPage = fFalse;
            }

            //  access next page [R latched]
            //
            Call( pcsr->ErrSwitchPage(
                                pfucb->ppib,
                                pfucb->ifmp,
                                pcsr->Cpage().PgnoNext(),
                                pfucb->u.pfcb->FNoCache() ) );

            const BOOL  fBadSiblingPointer  = ( pcsr->Cpage().PgnoPrev() != pgnoFrom );
            const BOOL  fBadTreeObjid       = pcsr->Cpage().ObjidFDP() != pfucb->u.pfcb->ObjidFDP();
            const BOOL  fClinesLow          = pcsr->Cpage().Clines() <= 0;
            if( fBadSiblingPointer || fBadTreeObjid || fClinesLow )
            {
                const PGNO  pgnoBadLink     = ( fBadSiblingPointer ?
                                                        pcsr->Cpage().PgnoPrev() :
                                                        pcsr->Cpage().ObjidFDP() );

                //  if not repair, assert, otherwise, suppress the assert and
                //  repair will just naturally err out
                AssertSz( g_fRepair, "Corrupt B-tree: bad leaf page links detected on MoveNext" );
                Call( ErrBTIReportBadPageLink(
                        pfucb,
                        ErrERRCheck( JET_errBadPageLink ),
                        pgnoFrom,
                        pcsr->Pgno(),
                        pgnoBadLink,
                        fTrue,
                        fBadSiblingPointer ?
                            "BtNextBadPgnoNextOrBacklink" :
                            ( fBadTreeObjid ?
                                "BtNextBadTreeObjid" :
                                ( pcsr->Cpage().FPreInitPage() ?
                                    "BtNextClinesLowPreinit" :
                                    ( pcsr->Cpage().FEmptyPage() ?
                                        "BtNextClinesLowEmpty" :
                                        "BtNextClinesLowInPlace" ) ) ) ) );
            }

            // get first node
            //
            NDMoveFirstSon( pfucb );

            if ( ( FFUCBPreread( pfucb ) &&
                   FFUCBPrereadForward( pfucb ) ) ||
                 btskipstats.CNodesSkipped() > 0 )
            {
                if ( !FFUCBPreread( pfucb ) )
                {
                    FUCBSetPrereadForward( pfucb, cpgPrereadPredictive );
                }

                if ( pfucb->cpgPrereadNotConsumed > 0 )
                {
                    //  read one more of preread pages
                    //
                    pfucb->cpgPrereadNotConsumed--;
                }

                // Long-value preread is handled in the LV code (because we know the size of the LV there)
                // All the BT code has to do is the cpgPrereadNotConsumed calculations
                if ( !FFUCBLongValue( pfucb ) )
                {
                    if ( 1 == pfucb->cpgPrereadNotConsumed
                         && pcsr->Cpage().PgnoNext() != pgnoNull )
                    {
                        PIBTraceContextScope tcScopeT = pfucb->ppib->InitTraceContextScope();
                        tcScopeT->iorReason.AddFlag( iorfOpportune );

                        //  preread the next page as it was not originally preread
                        //  Single page preread is kinda bad, but due to off by 1 error in cpgPrereadNotConsumed.
                        (void)ErrBFPrereadPage( pfucb->ifmp, pcsr->Cpage().PgnoNext(), bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->ifmp ), *tcScopeT );
                    }

                    if ( 0 == pfucb->cpgPrereadNotConsumed )
                    {
                        //  if we need to do neighbour-key check, must save off
                        //  bookmark
                        if ( ( dirflag & fDIRNeighborKey )
                            && NULL == pbkeySave )
                        {
                            Alloc( pbkeySave = ( BYTE *)RESKEY.PvRESAlloc() );
                            cbKeySave = pfucb->bmCurr.key.Cb();
                            pfucb->bmCurr.key.CopyIntoBuffer( pbkeySave, cbKeyAlloc );
                        }

                        //  UNDONE: this might cause a bug since there is an assumption
                        //          that ErrBTNext does not lose latch
                        //
                        //  preread more pages
                        //



                        const LATCH latch = pcsr->Latch();
                        BOOL    fVisible;
                        NS      ns;
                        Call( ErrNDVisibleToCursor( pfucb, &fVisible, &ns ) );
                        Assert( pcsr->FLatched() );
                        Call( ErrBTRelease( pfucb ) );
                        Assert( !pcsr->FLatched() );
                        //  Request exact position if key visible to cursor, otherwise, key can disappear
                        //  when we release the latch and it is ok to get adjacent key.
                        Call( ErrBTGotoBookmark( pfucb, pfucb->bmCurr, latch, fVisible ) );
                        Assert( !fVisible || ( err != wrnNDFoundGreater && err != wrnNDFoundLess ) );
                        if ( wrnNDFoundLess == err )
                        {
                            //  We went backwards, maybe to a node where we already were. Go to next node.
                            goto Start;
                        }
                    }
                }
            }

        }
    }

    AssertNDGet( pfucb );

    //  but node may not be valid due to dirFlag
    //

    //  move again if fDIRNeighborKey set and next node has same key
    //
    if ( dirflag & fDIRNeighborKey )
    {
        const BOOL  fSkip   = ( NULL == pbkeySave ?
                                    FKeysEqual( pfucb->kdfCurr.key, pfucb->bmCurr.key ) :
                                    FKeysEqual( pfucb->kdfCurr.key, pbkeySave, cbKeySave ) );
        if ( fSkip )
        {
            Assert( !FFUCBUnique( pfucb ) );
            cNeighbourKeysSkipped++;
            goto Start;
        }
    }

    //  check to see if we have moved to a different LID. if so, pretend we hit the end of the b-tree
    //  do this check before we check visibility to avoid walking long chains of deleted nodes

    if( dirflag & fDIRSameLIDOnly )
    {
        LvId lidCurr = lidMin;
        Assert( pfucb->u.pfcb->FTypeLV() );
        LidFromKey( &lidCurr, pfucb->kdfCurr.key );

        Assert( lidCurr >= lid );
        if( lidCurr != lid )
        {
            Error( ErrERRCheck( JET_errNoCurrentRecord ) );
        }
    }

    // function called with dirflag == fDIRAllNode from eseutil - node dumper
    // Assert( ! ( dirflag & fDIRAllNode ) );
    if ( !( dirflag & fDIRAllNode ) )
    {
        Assert( !( dirflag & fDIRAllNodesNoCommittedDeleted ) );

        //  fDIRAllNode not set
        //  check version store to see if node is visible to cursor
        //
        BOOL    fVisible;
        NS      ns;
        Call( ErrNDVisibleToCursor( pfucb, &fVisible, &ns ) );
        if ( fVisible )
        {
            if ( pfucb->pmoveFilterContext )
            {
                Call( pfucb->pmoveFilterContext->pfnMoveFilter( pfucb, pfucb->pmoveFilterContext ) );
                Assert( err == JET_errSuccess || err == wrnBTNotVisibleRejected || err == wrnBTNotVisibleAccumulated );
                if ( err > JET_errSuccess )
                {
                    //  current node isn't visible to us, but before going to
                    //  the trouble of finding a visible sibling (which may
                    //  be potentially extremely expensive and cause us to
                    //  navigate over tons of non-visible nodes), see if an
                    //  index range has been imposed on our navigation which
                    //  may allow us to short-circuit out.
                    //
                    if ( FFUCBLimstat( pfucb ) && FFUCBUpper( pfucb ) )
                    {
                        Call( ErrFUCBCheckIndexRange( pfucb, pfucb->kdfCurr.key ) );
                    }

                    if ( err == wrnBTNotVisibleRejected )
                    {
                        btskipstats.BTISkipStatsUpdateFilteredNode();
                    }
                    goto Start;
                }
            }
        }
        else
        {
            if ( dirflag & fDIRSameLIDOnly )
            {
                //  if this chunk is not visible to us, it should not
                //  be possible for subsequent chunks with the same
                //  lid to be visible
                //
                Error( ErrERRCheck( JET_errNoCurrentRecord ) );
            }

            //  current node isn't visible to us, but before going to
            //  the trouble of finding a visible sibling (which may
            //  be potentially extremely expensive and cause us to
            //  navigate over tons of non-visible nodes), see if an
            //  index range has been imposed on our navigation which
            //  may allow us to short-circuit out.
            //
            if ( FFUCBLimstat( pfucb ) && FFUCBUpper( pfucb ) )
            {
                Call( ErrFUCBCheckIndexRange( pfucb, pfucb->kdfCurr.key ) );
            }

            //  node not visible to cursor
            //  goto next node
            //
            btskipstats.BTISkipStatsUpdateSkippedNode( ns );
            fNodesSkippedOnCurrentPage = fTrue;
            goto Start;
        }
    }
    else
    {
        // we need to keep going if the node is deleted and 
        // doesn't have any versions
        if ( dirflag & fDIRAllNodesNoCommittedDeleted )
        {
            if ( FBTINodeCommittedDelete( pfucb ) )
            {
                btskipstats.BTISkipStatsUpdateSkippedDeleted();
                fNodesSkippedOnCurrentPage = fTrue;
                goto Start;
            }
        }
    }

    AssertNDGet( pfucb );
    err = JET_errSuccess;

HandleError:

    //  batch updates to our perf counters
    //
    PERFOpt( cBTNext.Add( PinstFromPfucb( pfucb )->m_iInstance, (TCE)tcScope->nParentObjectClass, 1 + cNeighbourKeysSkipped + btskipstats.CNodesSkipped() ) );
    if ( btskipstats.CFiltered() > 0 )
    {
        PERFOpt( cBTNextNodesFiltered.Add( PinstFromPfucb( pfucb )->m_iInstance, (TCE)tcScope->nParentObjectClass, btskipstats.CFiltered() ) );
    }
    const ULONG cNonVisibleSkipped = btskipstats.CNodesSkipped() - btskipstats.CFiltered();
    if ( cNonVisibleSkipped > 0 )
    {
        PERFOpt( cBTNextNonVisibleNodesSkipped.Add( PinstFromPfucb( pfucb )->m_iInstance, (TCE)tcScope->nParentObjectClass, cNonVisibleSkipped ) );
    }

    //  if we've skipped lots of non-visible nodes, report a warning
    //
    if ( btskipstats.FBTIReportManyNonVisibleNodesSkipped() )
    {
        //  We may have an outstanding page to process ...
        //  Note: Calling this after btskipstats.FBTIReportManyNonVisibleNodesSkipped means most of the time we require 1 extra page to trigger report.
        if ( fNodesSkippedOnCurrentPage && pcsr->Latch() > latchNone )
        {
            btskipstats.BTISkipStatsUpdateSkippedPage( pcsr->Cpage() );
        }

        btskipstats.BTIReportManyNonVisibleNodesSkipped( pfucb );
    }

    RESKEY.Free( pbkeySave );
    return err;
}


//  goes to previous line in tree
//
ERR ErrBTPrev( FUCB *pfucb, DIRFLAG dirflag )
{
    // the only way we navigate back on all the nodes
    // is to search for ErrRECIInitAutoInc
    Assert( !( dirflag & fDIRAllNode ) || (dirflag & fDIRAllNodesNoCommittedDeleted ) );

    ERR         err;
    CSR * const pcsr                        = Pcsr( pfucb );
    BYTE*       pbkeySave                   = NULL;
    ULONG       cbKeySave                   = 0;
    ULONG       cLatchConflicts             = 0;
    ULONG       cNeighbourKeysSkipped       = 0;
    BOOL        fNodesSkippedOnCurrentPage  = fFalse;
    BTISKIPSTATS    btskipstats;
    PIBTraceContextScope tcScope        = TcBTICreateCtxScope( pfucb, iorsBTMovePrev );

    Assert( FBTLogicallyNavigableState( pfucb ) );

    //  if a filter is provided, then we must be navigating a table or secondary index
    //
    Assert( !pfucb->pmoveFilterContext || pfucb->u.pfcb->FTypeTable() || pfucb->u.pfcb->FTypeSecondaryIndex() );

    //  this flag only works for ErrBTNext

    if( dirflag & fDIRSameLIDOnly )
    {
        AssertSz( fFalse, "ErrBTPrev doesn't support fDIRSameLIDOnly" );
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    // Prevent R/O transactions from proceeding if they are holding up version store cleanup.
    //
    if ( pfucb->ppib->FReadOnlyTrx() && pfucb->ppib->Level() > 0 )
    {
        CallR( PverFromIfmp( pfucb->ifmp )->ErrVERCheckTransactionSize( pfucb->ppib ) );
    }

    //  refresh currency
    //  places cursor on record to move from
    //

    CallR( ErrBTIRefresh( pfucb ) );

Start:

    //  now we have a read latch on page
    //
    Assert( latchReadTouch == pcsr->Latch() ||
            latchReadNoTouch == pcsr->Latch() ||
            latchRIW == pcsr->Latch() );
    Assert( ( pcsr->Cpage().FLeafPage() ) != 0 );
    AssertNDGet( pfucb );

    //  get prev node in page
    //  if it does not exist, go to prev page
    //
    if ( pcsr->ILine() > 0 )
    {
        //  get prev node
        //
        pcsr->DecrementILine();
        NDGet( pfucb );

        //  adjust key position
        //
        if ( pfucb->ullTotalLast )
        {
            pfucb->ullLTLast--;
        }
    }
    else
    {
        Assert( pcsr->ILine() == 0 );

        //  reset key position
        //
        pfucb->ullLTLast = 0;
        pfucb->ullTotalLast = 0;

        //  prev node not in current page
        //  get prev page and continue
        //
        if ( pcsr->Cpage().PgnoPrev() == pgnoNull )
        {
            //  past the first node
            //
            Error( ErrERRCheck( JET_errNoCurrentRecord ) );
        }
        else
        {
            const PGNO  pgnoFrom    = pcsr->Pgno();

            //  before we move pages, track any stats from this page

            if ( fNodesSkippedOnCurrentPage )
            {
                btskipstats.BTISkipStatsUpdateSkippedPage( pcsr->Cpage() );
                fNodesSkippedOnCurrentPage = fFalse;
            }

            //  access prev page [R latched] without wait
            //  if conflict, release latches and retry
            //  else proceed
            //  the release of current page is done atomically by CSR
            //
            err = pcsr->ErrSwitchPageNoWait(
                                    pfucb->ppib,
                                    pfucb->ifmp,
                                    pcsr->Cpage().PgnoPrev() );
            if ( err == errBFLatchConflict )
            {
                cLatchConflicts++;
                Assert( cLatchConflicts < 1000 );

                //  save bookmark
                //  release latches
                //  re-seek
                //
                const PGNO      pgnoWait    = pcsr->Cpage().PgnoPrev();
                const LATCH     latch       = pcsr->Latch();

                //  if we need to do neighbour-key check, must save off
                //  bookmark
                if ( ( dirflag & fDIRNeighborKey )
                    && NULL == pbkeySave )
                {
                    Alloc( pbkeySave = ( BYTE* )RESKEY.PvRESAlloc() );
                    cbKeySave = pfucb->bmCurr.key.Cb();
                    pfucb->bmCurr.key.CopyIntoBuffer( pbkeySave, cbKeyAlloc );
                }

                Assert( pcsr->FLatched() );
                Call( ErrBTRelease( pfucb ) );
                Assert( !pcsr->FLatched() );

                //  wait & refresh currency
                //
                //  NOTE:  we wait on the page that caused the conflict solely
                //  as a form of delay.  there is no guarantee that this will
                //  really be the page we want once we wake up
                //
                err = pcsr->ErrGetRIWPage(
                                    pfucb->ppib,
                                    pfucb->ifmp,
                                    pgnoWait );
                if ( err < JET_errSuccess )
                {
                    //  use sleep as a last resort
                    UtilSleep( cmsecWaitGeneric );
                }
                else
                {
                    pcsr->ReleasePage();
                }

                Call( ErrBTGotoBookmark( pfucb, pfucb->bmCurr, latch, fFalse ) );

                if ( wrnNDFoundGreater == err ||
                     JET_errSuccess == err )
                {
                    //  go to previous node
                    //
                    goto Start;
                }
            }
            else
            {
                Call( err );

                const BOOL  fBadSiblingPointer  = ( pcsr->Cpage().PgnoNext() != pgnoFrom );
                const BOOL  fBadTreeObjid       = pcsr->Cpage().ObjidFDP() != pfucb->u.pfcb->ObjidFDP();
                const BOOL  fClinesLow          = pcsr->Cpage().Clines() <= 0;
                if( fBadSiblingPointer || fBadTreeObjid || fClinesLow )
                {
                    const PGNO  pgnoBadLink     = ( fBadSiblingPointer ?
                                                            pcsr->Cpage().PgnoNext() :
                                                            pcsr->Cpage().ObjidFDP() );

                    //  if not repair, assert, otherwise, suppress the assert and
                    //  repair will just naturally err out
                    AssertSz( g_fRepair, "Corrupt B-tree: bad leaf page links detected on MovePrev" );
                    Call( ErrBTIReportBadPageLink(
                                pfucb,
                                ErrERRCheck( JET_errBadPageLink ),
                                pgnoFrom,
                                pcsr->Pgno(),
                                pgnoBadLink,
                                fTrue,
                                fBadSiblingPointer ?
                                    "BtPrevBadPgnoPrevOrFwdlink" :
                                    ( fBadTreeObjid ?
                                        "BtPrevBadTreeObjid" :
                                        ( pcsr->Cpage().FPreInitPage() ?
                                            "BtPrevClinesLowPreinit" :
                                            ( pcsr->Cpage().FEmptyPage() ?
                                                "BtPrevClinesLowEmpty" :
                                                "BtPrevClinesLowInPlace" ) ) ) ) );
                }

                //  get last node in page
                //
                NDMoveLastSon( pfucb );

                if( ( FFUCBPreread( pfucb ) &&
                      FFUCBPrereadBackward( pfucb ) ) ||
                    btskipstats.CNodesSkipped() > 0 )
                {
                    if ( !FFUCBPreread( pfucb ) )
                    {
                        FUCBSetPrereadBackward( pfucb, cpgPrereadPredictive );
                    }

                    if ( pfucb->cpgPrereadNotConsumed > 0 )
                    {
                        pfucb->cpgPrereadNotConsumed--;
                    }

                    // Long-value preread is handled in the LV code (because we know the size of the LV there)
                    // All the BT code has to do is the cpgPrereadNotConsumed calculations
                    if ( !FFUCBLongValue( pfucb ) )
                    {
                        if ( 1 == pfucb->cpgPrereadNotConsumed
                            && pcsr->Cpage().PgnoPrev() != pgnoNull )
                        {
                            PIBTraceContextScope tcScopeT = pfucb->ppib->InitTraceContextScope();
                            tcScopeT->iorReason.AddFlag( iorfOpportune );

                            //  preread the next page as it was not originally preread
                            //  Single page preread is kinda bad, but due to off by 1 error in cpgPrereadNotConsumed.
                            (void)ErrBFPrereadPage( pfucb->ifmp, pcsr->Cpage().PgnoPrev(), bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->ifmp ), *tcScopeT );
                        }

                        if ( 0 == pfucb->cpgPrereadNotConsumed )
                        {
                            //  if we need to do neighbour-key check, must save off
                            //  bookmark
                            if ( ( dirflag & fDIRNeighborKey )
                                && NULL == pbkeySave )
                            {
                                Alloc( pbkeySave = ( BYTE* )RESKEY.PvRESAlloc() );
                                cbKeySave = pfucb->bmCurr.key.Cb();
                                pfucb->bmCurr.key.CopyIntoBuffer( pbkeySave, cbKeyAlloc );
                            }

                            //  preread more pages
                            //
                            const LATCH latch = pcsr->Latch();
                            BOOL    fVisible;
                            NS      ns;
                            Call( ErrNDVisibleToCursor( pfucb, &fVisible, &ns ) );
                            Assert( pcsr->FLatched() );
                            Call( ErrBTRelease( pfucb ) );
                            Assert( !pcsr->FLatched() );
                            //  Request exact position if key visible to cursor, otherwise, key can disappear
                            //  when we release the latch and it is ok to get adjacent key.
                            Call( ErrBTGotoBookmark( pfucb, pfucb->bmCurr, latch, fVisible ) );
                            Assert( !fVisible || ( err != wrnNDFoundGreater && err != wrnNDFoundLess ) );
                            if ( wrnNDFoundGreater == err )
                            {
                                //  We went forward, maybe to a node where we already were. Go to prev node.
                                goto Start;
                            }
                        }
                    }
                }
            }
        }
    }

    //  get prev node
    //
    AssertNDGet( pfucb );

    //  but node may not be valid due to dirFlag
    //

    //  move again if fDIRNeighborKey set and prev node has same key
    //
    if ( dirflag & fDIRNeighborKey )
    {
        const BOOL  fSkip   = ( NULL == pbkeySave ?
                                    FKeysEqual( pfucb->kdfCurr.key, pfucb->bmCurr.key ) :
                                    FKeysEqual( pfucb->kdfCurr.key, pbkeySave, cbKeySave ) );
        if ( fSkip )
        {
            Assert( !FFUCBUnique( pfucb ) );
            cNeighbourKeysSkipped++;
            goto Start;
        }
    }

    if ( !( dirflag & fDIRAllNode ) )
    {
        Assert( !( dirflag & fDIRAllNodesNoCommittedDeleted ) );

        //  fDIRAllNode not set
        //  check version store to see if node is visible to cursor
        //
        BOOL    fVisible;
        NS      ns;
        Call( ErrNDVisibleToCursor( pfucb, &fVisible, &ns ) );
        if ( fVisible )
        {
            if ( pfucb->pmoveFilterContext )
            {
                Call( pfucb->pmoveFilterContext->pfnMoveFilter( pfucb, pfucb->pmoveFilterContext ) );
                Assert( err == JET_errSuccess || err == wrnBTNotVisibleRejected || err == wrnBTNotVisibleAccumulated );
                if ( err > JET_errSuccess )
                {
                    //  current node isn't visible to us, but before going to
                    //  the trouble of finding a visible sibling (which may
                    //  be potentially extremely expensive and cause us to
                    //  navigate over tons of non-visible nodes), see if an
                    //  index range has been imposed on our navigation which
                    //  may allow us to short-circuit out.
                    //
                    if ( FFUCBLimstat( pfucb ) && !FFUCBUpper( pfucb ) )
                    {
                        Call( ErrFUCBCheckIndexRange( pfucb, pfucb->kdfCurr.key ) );
                    }

                    if ( err == wrnBTNotVisibleRejected )
                    {
                        btskipstats.BTISkipStatsUpdateFilteredNode();
                    }
                    goto Start;
                }
            }
        }
        else
        {
            //  current node isn't visible to us, but before going to
            //  the trouble of finding a visible sibling (which may
            //  be potentially extremely expensive and cause us to
            //  navigate over tons of non-visible nodes), see if an
            //  index range has been imposed on our navigation which
            //  may allow us to short-circuit out.
            //
            if ( FFUCBLimstat( pfucb ) && !FFUCBUpper( pfucb ) )
            {
                Call( ErrFUCBCheckIndexRange( pfucb, pfucb->kdfCurr.key ) );
            }

            //  node not visible to cursor
            //  goto next node
            //
            btskipstats.BTISkipStatsUpdateSkippedNode( ns );
            fNodesSkippedOnCurrentPage = fTrue;
            goto Start;
        }
    }
    else
    {
        // the only way we navigate back on all the nodes
        // is to search for ErrRECIInitAutoInc
        Assert ( dirflag & fDIRAllNodesNoCommittedDeleted );

        // we need to go back if the node is deleted and 
        // doesn't have any versions
        if ( dirflag & fDIRAllNodesNoCommittedDeleted )
        {
            if ( FBTINodeCommittedDelete( pfucb ) )
            {
                btskipstats.BTISkipStatsUpdateSkippedDeleted();
                fNodesSkippedOnCurrentPage = fTrue;
                goto Start;
            }
        }
    }

    AssertNDGet( pfucb );
    err = JET_errSuccess;

HandleError:

    //  batch updates to our perf counters
    //
    PERFOpt( cBTPrev.Add( PinstFromPfucb( pfucb )->m_iInstance, (TCE)tcScope->nParentObjectClass, 1 + cNeighbourKeysSkipped + btskipstats.CNodesSkipped() ) );
    if ( btskipstats.CFiltered() > 0 )
    {
        PERFOpt( cBTPrevNodesFiltered.Add( PinstFromPfucb( pfucb )->m_iInstance, (TCE)tcScope->nParentObjectClass, btskipstats.CFiltered() ) );
    }
    const ULONG cNonVisibleSkipped = btskipstats.CNodesSkipped() - btskipstats.CFiltered();
    if ( cNonVisibleSkipped > 0 )
    {
        PERFOpt( cBTPrevNonVisibleNodesSkipped.Add( PinstFromPfucb( pfucb )->m_iInstance, (TCE)tcScope->nParentObjectClass, cNonVisibleSkipped ) );
    }

    //  if we've skipped lots of non-visible nodes, report a warning
    //
    if ( btskipstats.FBTIReportManyNonVisibleNodesSkipped() )
    {
        //  We may have an outstanding page to process ...
        if ( fNodesSkippedOnCurrentPage && pcsr->Latch() > latchNone )
        {
            btskipstats.BTISkipStatsUpdateSkippedPage( pcsr->Cpage() );
        }

        btskipstats.BTIReportManyNonVisibleNodesSkipped( pfucb );
    }

    RESKEY.Free( pbkeySave );
    return err;
}


//  ================================================================
VOID BTPrereadSpaceTree( const FUCB * const pfucb )
//  ================================================================
//
//  Most of the time we will only need the Avail-Extent. Only if it
//  is empty will we need the Owned-Extent. However, the two pages are together
//  on disk so they are very cheap to read together.
//
//-
{
    FCB* const pfcb = pfucb->u.pfcb;
    Assert( pfcbNil != pfcb );

    INT     ipgno = 0;
    PGNO    rgpgno[3];      // pgnoOE, pgnoAE, pgnoNull
    if( pgnoNull != pfcb->PgnoOE() )    //  may be single-extent
    {
        Assert( pgnoNull != pfcb->PgnoAE() );
        rgpgno[ipgno++] = pfcb->PgnoOE();
        rgpgno[ipgno++] = pfcb->PgnoAE();
        rgpgno[ipgno++] = pgnoNull;

        PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
        tcScope->nParentObjectClass = pfcb->TCE( fTrue );
        tcScope->SetDwEngineObjid ( ObjidFDP( pfucb ) );
        tcScope->iorReason.SetIort( iortSpace );
        tcScope->iorReason.AddFlag( iorfOpportune );

        BFPrereadPageList( pfcb->Ifmp(), rgpgno, bfprfDefault, pfucb->ppib->BfpriPriority( pfcb->Ifmp() ), *tcScope );
    }
}


//  ================================================================
VOID BTPrereadIndexesOfFCB( FUCB * const pfucb )
//  ================================================================
{
    FCB* const pfcb = pfucb->u.pfcb;
    Assert( pfcbNil != pfcb );
    Assert( pfcb->FTypeTable() );
    Assert( ptdbNil != pfcb->Ptdb() );

    const INT   cMaxIndexesToPreread    = 16;
    PGNO        rgpgno[cMaxIndexesToPreread + 1];   //  NULL-terminated
    INT         ipgno                   = 0;

    pfcb->EnterDML();

    for ( const FCB * pfcbT = pfcb->PfcbNextIndex();
        pfcbNil != pfcbT && ipgno < cMaxIndexesToPreread;
        pfcbT = pfcbT->PfcbNextIndex() )
    {
        const IDB * const   pidbT   = pfcbT->Pidb();
        if ( !pidbT->FSparseIndex()
            && !pidbT->FSparseConditionalIndex() )
        {
            //  this index isn't sparse, so odds are it
            //  will have to be updated, so go ahead
            //  and preread its pgnoFDP
            //
            rgpgno[ipgno++] = pfcbT->PgnoFDP();
        }
    }

    pfcb->LeaveDML();

    //  NULL-terminate the list of pages to preread,
    //  then issue the actual preread
    //
    rgpgno[ipgno] = pgnoNull;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = pfcb->TCE();
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );

    BFPrereadPageList( pfcb->Ifmp(), rgpgno, bfprfDefault, pfucb->ppib->BfpriPriority( pfcb->Ifmp() ), *tcScope );
}

//  ================================================================
PrereadContext::PrereadContext( PIB * const ppib, FUCB * const pfucb ) :
//  ================================================================
    m_ppib( ppib ),
    m_pfucb( pfucb ),
    m_ifmp( pfucb->ifmp ),
    m_tcScope( TcBTICreateCtxScope( pfucb, iorsBTPreread ) ),
    m_cpgPrereadMax( 0 )
{
    for ( int i = 0; i < 2; ++i )
    {
        m_cpgPreread[ i ]       = 0;
        m_cpgPrereadAlloc[ i ]  = 0;
        m_rgpgnoPreread[ i ]    = NULL;
    }
}

//  ================================================================
PrereadContext::~PrereadContext()
//  ================================================================
{
    for ( int i = 0; i < 2; ++i )
    {
        delete[] m_rgpgnoPreread[i];
    }
}

//  ================================================================
void PrereadContext::CheckSpaceFragmentation( const CSR& csr )
//  ================================================================
{
    if (    csr.Cpage().FParentOfLeaf() &&
            FBTIEligibleForOLD2( m_pfucb ) )
    {
        CheckSpaceFragmentation_( csr );
    }
}

//  ================================================================
void PrereadContext::CheckSpaceFragmentation_( const CSR& csr )
//  ================================================================
{
    ERR err;
    PGNO * rgpgno = NULL;

    const INT clines = csr.Cpage().Clines();
    Alloc( rgpgno = new PGNO[clines] );

    for( INT iline = 0; iline < clines; ++iline )
    {
        KEYDATAFLAGS kdf;
        NDIGetKeydataflags(csr.Cpage(), iline, &kdf);
        const PGNO pgnoChild = *(UnalignedLittleEndian< PGNO > *) kdf.data.Pv();
        rgpgno[iline] = pgnoChild;
    }

    // Sort the pages before checking. if the pages are slightly out of order
    // then preread will still read them efficiently so we don't want to
    // trigger OLD2.
    sort( rgpgno, rgpgno + clines );

    if( FBTICpgFragmentedRangeTrigger(
        rgpgno,
        clines,
        cpgFragmentedRangeDefragTrigger,
        cpgFragmentedRangeDefrag ) )
    {
        // remember not to overwrite this value if FBTICpgFragmentedRange return true
        m_fSawFragmentedSpace = true;
    }

HandleError:
    delete[] rgpgno;
}

//  ================================================================
ERR PrereadContext::ErrProcessSubtreeRangesForward(
    _In_ const BOOKMARK * const                     rgbmStart,
    _In_ const BOOKMARK * const                     rgbmEnd,
    _In_count_(csubtrees) const PGNO * const        rgpgnoSubtree,
    _In_count_(csubtrees) const LONG * const        rgibmMin,
    _In_count_(csubtrees) const LONG * const        rgibmMax,
    const LONG                                      csubtrees,
    _Out_ LONG * const                              pcbmRangesPreread,
    _In_ BOOL const                                 fIncludeNonLeafPg )
//  ================================================================
{
    ERR err = JET_errSuccess;

    // Optimization:
    // we could try to optimize this code by going through the loop twice.
    // the first time use bflfNoUncached to process pages that are already
    // in the cache and the second time reading pages that weren't processed
    // in the first loop. as per-page processing time should be small
    // compared to I/O latency doing that probably wouldn't help much

    CSR csrChild;
    for ( INT isubtree = 0; isubtree < csubtrees && m_cpgPreread[PrereadPgType::LeafPages] < m_cpgPrereadMax; ++isubtree )
    {
        const PGNO pgno = rgpgnoSubtree[isubtree];
        const INT ibmMin = rgibmMin[isubtree];
        const INT ibmMax = rgibmMax[isubtree];

        Assert( pgnoNull != pgno );
        Assert( ibmMax > ibmMin );

        Call( csrChild.ErrGetReadPage( m_ppib, m_ifmp, pgno, bflfDefault ) );
        err = ErrPrereadRangesForward(
                csrChild,
                rgbmStart+ibmMin,
                rgbmEnd+ibmMin,
                ibmMax-ibmMin,
                pcbmRangesPreread,
                fIncludeNonLeafPg );
        *pcbmRangesPreread += ibmMin;
        csrChild.ReleasePage();
        Call( err );
    }

HandleError:
    return err;
}

//  ================================================================
ERR PrereadContext::ErrProcessSubtreeRangesBackward(
    _In_ const BOOKMARK * const                     rgbmStart,
    _In_ const BOOKMARK * const                     rgbmEnd,
    _In_count_(csubtrees) const PGNO * const        rgpgnoSubtree,
    _In_count_(csubtrees) const LONG * const        rgibmMin,
    _In_count_(csubtrees) const LONG * const        rgibmMax,
    const LONG                                      csubtrees,
    _Out_ LONG * const                              pcbmRangesPreread,
    _In_ BOOL const                                 fIncludeNonLeafPg )
//  ================================================================
{
    ERR err = JET_errSuccess;

    // Optimization:
    // we could try to optimize this code by going through the loop twice.
    // the first time use bflfNoUncached to process pages that are already
    // in the cache and the second time reading pages that weren't processed
    // in the first loop. as per-page processing time should be small
    // compared to I/O latency doing that probably wouldn't help much

    CSR csrChild;
    for ( INT isubtree = 0; isubtree < csubtrees && m_cpgPreread[PrereadPgType::LeafPages] < m_cpgPrereadMax; ++isubtree )
    {
        const PGNO pgno = rgpgnoSubtree[isubtree];
        const INT ibmMin = rgibmMin[isubtree];
        const INT ibmMax = rgibmMax[isubtree];

        Assert( pgnoNull != pgno );
        Assert( ibmMax > ibmMin );

        Call( csrChild.ErrGetReadPage( m_ppib, m_ifmp, pgno, bflfDefault ) );
        err = ErrPrereadRangesBackward(
                csrChild,
                rgbmStart+ibmMin,
                rgbmEnd+ibmMin,
                ibmMax-ibmMin,
                pcbmRangesPreread,
                fIncludeNonLeafPg );
        *pcbmRangesPreread += ibmMin;
        csrChild.ReleasePage();
        Call( err );
    }

HandleError:
    return err;
}


//  ================================================================
ERR PrereadContext::ErrPrereadRangesForward(
    const CSR&                                      csr,
    __in_ecount(cbm) const BOOKMARK * const         rgbmStart,
    __in_ecount(cbm) const BOOKMARK * const         rgbmEnd,
    const LONG                                      cbm,
    _Out_ LONG * const                              pcbmRangesPreread,
    _In_ BOOL const                                 fIncludeNonLeafPg )
//  ================================================================
//
//  Preread all the leaf pages containing the given bookmarks
//
//-
{
    ERR err = JET_errSuccess;

    if( csr.Cpage().FLeafPage() )
    {
        // this can happen if the tree has a depth of 1
        Assert( csr.Cpage().FRootPage() );
        *pcbmRangesPreread = cbm;
        return JET_errSuccess;
    }

    Assert( csr.Cpage().FInvisibleSons() );

    CheckSpaceFragmentation( csr );

    // if this page isn't a parent-of-leaf page we will need to read the child pages
    // rather than wait for a synchronous read we issue prereads for those pages and
    // then go back to process them. these arrays remember which pages needs to be
    // processed and what arguments to pass to them
    //
    // the arguments are stored in this array and then used later

    // with 32kb pages the fanout is so high that a tree with more than 16 parent-of-leaf
    // pages is normally extremely large    
    const INT csubtreePrereadMax = 16;
    PGNO rgpgnoSubtree[csubtreePrereadMax+1];   // need space for a null terminator
    LONG rgibmMin[csubtreePrereadMax];
    LONG rgibmMax[csubtreePrereadMax];
    INT isubtree = 0;
    *pcbmRangesPreread = 0;

    INT ibm = 0;

    // loop until we have looked at all nodes on the page or all bookmarks
    //
    // we loop until we find a node whose bookmark is greater than the current bookmark
    // at that point we increment through the bookmarks until we find a bookmark greater
    // than the current node. at that point we have the set of bookmarks which belong
    // on the subtree. We can either recurse onto the subtree (if the child page
    // is an internal page) or add the page to the preread list (if the child page
    // is a leaf page)
    //
    // Optimization:
    // linear search is easiest, but we could improve performance by using
    // binary search here.
    //
    for(
        INT iline = 0;
        iline < csr.Cpage().Clines() && ibm < cbm && m_cpgPreread[PrereadPgType::LeafPages] < m_cpgPrereadMax;
        ++iline )
    {
        // if this iline points to a subtree we want to preread these variables will point
        // to the bookmark range contained by the subtree
        INT ibmMin = 0; // the first bookmark to process
        INT ibmMax = 0; // the first bookmark to ignore

        KEYDATAFLAGS kdf;

        //  binary search
        //      
        if( iline < ( csr.Cpage().Clines() - 1 ) )
        {
            NDIGetKeydataflags( csr.Cpage(), iline, &kdf );
            Assert( kdf.key.Cb() > 0 );
            //  find first iline with key data greater than rgbmStart[ibm]
            //
            if( CmpKeyWithKeyData( kdf.key, rgbmStart[ibm] ) <= 0 )
            {
                INT compareT = 0;
                INT ilineT = IlineNDISeekGEQInternal( csr.Cpage(), rgbmStart[ibm], &compareT );
                if ( 0 == compareT && ilineT < ( csr.Cpage().Clines() - 1 ) )
                {
                    //  found equal to so move to next line if one exists
                    //
                    ilineT++;
                }

#ifdef DEBUG
                //  assert correct position
                //
                KEYDATAFLAGS kdfDebug;
                //  if on non-last node of page then key should be greater than current start key
                //
                if ( ilineT < ( csr.Cpage().Clines() - 1 ) )
                {
                    NDIGetKeydataflags( csr.Cpage(), ilineT, &kdfDebug );
                    Assert( kdfDebug.key.Cb() > 0 );
                    Assert( CmpKeyWithKeyData( kdfDebug.key, rgbmStart[ibm] ) > 0 );
                }
                //  if a previous key then it should be less than or equal to current start key
                //
                if ( ilineT > 0 )
                {
                    NDIGetKeydataflags( csr.Cpage(), ilineT - 1, &kdfDebug );
                    Assert( kdfDebug.key.Cb() > 0 );
                    Assert( CmpKeyWithKeyData( kdfDebug.key, rgbmStart[ibm] ) <= 0 );
                }
#endif
                //  should advance
                //
                Assert( ilineT > iline );
                iline = ilineT;
            }
        }

        if( ( csr.Cpage().Clines() - 1 ) == iline )
        {
            // we are at the end of the page so all remaining nodes are found on this subtree
            ibmMin  = ibm;
            ibmMax  = cbm;
            ibm     = cbm;
        }
        else
        {
            NDIGetKeydataflags( csr.Cpage(), iline, &kdf );
            Assert( kdf.key.Cb() > 0 );

            // all nodes with a bookmark less than the page pointer are found on the child page
            if( CmpKeyWithKeyData( kdf.key, rgbmStart[ibm] ) > 0 )
            {
                // bookmarks are in ascending order so we are on the first bookmark which is less
                // than the current KDF. Find all bookmarks that are less than the current KDF
                // as they should all be processed for this sub-tree

                ibmMin = ibm;   // the first bookmark to process

                while( ibm < cbm )
                {
                    if( CmpKeyWithKeyData( kdf.key, rgbmEnd[ibm] ) <= 0 )
                    {
                        break;
                    }
                    // this bookmark is completely processed by this sub-tree
                    ibm++;
                }

                ibmMax = ibm;   // the first node to ignore
                while ( ibmMax < cbm )
                {
                    if ( CmpKeyWithKeyData( kdf.key, rgbmStart[ibmMax] ) <= 0 )
                    {
                        // this bookmark doesn't belong on the sub-tree
                        break;
                    }
                    ibmMax++;
                }
                Assert( ibmMax > ibmMin );
                Assert( ibmMax <= ibm + 1 );
            }
        }

        if( 0 == ibmMax )
        {
            // no match. this iline isn't needed
            Assert( 0 == ibmMin );
        }
        else
        {
            Assert( ibmMin < ibmMax );

            // retrieve the pgno of the subtree and process it
            NDIGetKeydataflags( csr.Cpage(), iline, &kdf );

            Assert( sizeof( PGNO ) == kdf.data.Cb() );
            const PGNO pgno = *(UnalignedLittleEndian< PGNO > *) kdf.data.Pv();
            Assert( pgnoNull != pgno );

            if( csr.Cpage().FParentOfLeaf() )
            {
                // this is a parent of leaf page just add the pgno to the preread list
                Call( ErrAddPrereadCandidate( pgno ) );
                *pcbmRangesPreread = ibm;
            }
            else
            {
                // otherwise we have to recurse. we have a child page and know which bookmarks are
                // found on that page. store the information in the subtree preread list

                DBEnforce( csr.Cpage().Ifmp(), isubtree < _countof( rgibmMin ) );
                DBEnforce( csr.Cpage().Ifmp(), isubtree < _countof( rgibmMax ) );
                rgibmMin[isubtree] = ibmMin;
                rgibmMax[isubtree] = ibmMax;
                rgpgnoSubtree[isubtree] = pgno;
                isubtree++;

                Assert( isubtree <= csubtreePrereadMax );
                // subtree buffer full, need to process now
                if( csubtreePrereadMax == isubtree )
                {
                    rgpgnoSubtree[isubtree] = pgnoNull;

                    // Even non-leaf pages are requested to be added to the m_rgpgnoPreread buffer.
                    // So lets add the array of subtree pages about to be read.
                    // Note: Currently this flag is used only to identify space tree pages needed for available lag during table deletes.
                    if ( fIncludeNonLeafPg )
                    {
                        Call( ErrAddPrereadCandidates( rgpgnoSubtree, isubtree, PrereadPgType::NonLeafPages ) );
                    }

                    BFPrereadPageList( m_ifmp, rgpgnoSubtree, bfprfDefault, m_ppib->BfpriPriority( m_ifmp ), TcCurr() );
                    Call( ErrProcessSubtreeRangesForward(
                            rgbmStart,
                            rgbmEnd,
                            rgpgnoSubtree,
                            rgibmMin,
                            rgibmMax,
                            isubtree,
                            pcbmRangesPreread,
                            fIncludeNonLeafPg ) );
                    isubtree = 0;
                }
            }
        }
    }

    // process any remaining subtrees
    AssumePREFAST( isubtree < csubtreePrereadMax );
    if( isubtree > 0 )
    {
        rgpgnoSubtree[isubtree] = pgnoNull;

        // Even non-leaf pages are requested to be added to the m_rgpgnoPreread buffer.
        // So lets add the array of subtree pages about to be read.
        // Note: Currently this flag is used only to identify space tree pages needed for available lag during table deletes.
        if ( fIncludeNonLeafPg )
        {
            Call( ErrAddPrereadCandidates( rgpgnoSubtree, isubtree, PrereadPgType::NonLeafPages ) );
        }

        BFPrereadPageList( m_ifmp, rgpgnoSubtree, bfprfDefault, m_ppib->BfpriPriority( m_ifmp ), TcCurr() );
        Call( ErrProcessSubtreeRangesForward(
                rgbmStart,
                rgbmEnd,
                rgpgnoSubtree,
                rgibmMin,
                rgibmMax,
                isubtree,
                pcbmRangesPreread,
                fIncludeNonLeafPg ) );
        isubtree = 0;
    }

HandleError:
    return err;
}

//  ================================================================
ERR PrereadContext::ErrPrereadRangesBackward(
    const CSR&                                      csr,
    __in_ecount(cbm) const BOOKMARK * const         rgbmStart,
    __in_ecount(cbm) const BOOKMARK * const         rgbmEnd,
    const LONG                                      cbm,
    _Out_ LONG * const                              pcbmRangesPreread,
    _In_ BOOL const                                 fIncludeNonLeafPg )
//  ================================================================
//
//  Preread all the leaf pages containing the given bookmarks
//
//-
{
    ERR err = JET_errSuccess;

    if( csr.Cpage().FLeafPage() )
    {
        // this can happen if the tree has a depth of 1
        Assert( csr.Cpage().FFDPPage() );
        *pcbmRangesPreread = cbm;
        return JET_errSuccess;
    }

    Assert( csr.Cpage().FInvisibleSons() );

    CheckSpaceFragmentation(csr);

    // if this page isn't a parent-of-leaf page we will need to read the child pages
    // rather than wait for a synchronous read we issue prereads for those pages and
    // then go back to process them. these arrays remember which pages needs to be
    // processed and what arguments to pass to them
    //
    // the arguments are stored in this array and then used later

    // with 32kb pages the fanout is so high that a tree with more than 16 parent-of-leaf
    // pages is normally extremely large
    const INT csubtreePrereadMax = 16;
    PGNO rgpgnoSubtree[csubtreePrereadMax+1];   // need space for a null terminator
    LONG rgibmMin[csubtreePrereadMax];
    LONG rgibmMax[csubtreePrereadMax];
    INT isubtree = 0;
    *pcbmRangesPreread = 0;

    INT ibm = 0;

    // loop until we have looked at all nodes on the page or all bookmarks
    //
    // We are working from the greatest bookmark to the least. That means we
    // loop backwards until the previous node has a bookmark which is less than
    // or equal to the bookmark we are processing. At that point the current node
    // points to the correct subtree. If we hit the end of the page then
    // all the remaining bookmarks belong to the first subtree.
    //
    // Optimization:
    // linear search is easiest, but we could improve performance by using
    // binary search here.
    //
    for(
        INT iline = csr.Cpage().Clines() - 1;
        iline >= 0 && ibm < cbm && m_cpgPreread[PrereadPgType::LeafPages] < m_cpgPrereadMax;
        --iline )
    {
        // if this iline points to a subtree we want to preread these variables will point
        // to the bookmark range contained by the subtree
        INT ibmMin = 0; // the first bookmark to process
        INT ibmMax = 0; // the first bookmark to ignore

        KEYDATAFLAGS kdfPrev;

        //  binary search
        //      
        if( iline > 0 )
        {
            NDIGetKeydataflags( csr.Cpage(), iline-1, &kdfPrev );
            Assert( kdfPrev.key.Cb() > 0 );

            //  find first iline with key data greater than rgbmStart[ibm]
            //
            if( CmpKeyWithKeyData( kdfPrev.key, rgbmStart[ibm] ) > 0 )
            {
                INT compareT = 0;
                INT ilineT = IlineNDISeekGEQInternal( csr.Cpage(), rgbmStart[ibm], &compareT );
                if ( 0 == compareT && ilineT < ( csr.Cpage().Clines() - 1 ) )
                {
                    //  found compare equal to so move to next so prev is equal.
                    //  If we had found greater then the previous node would be less than.
                    //
                    ilineT++;
                }

#ifdef DEBUG
                //  assert correct position
                //
                KEYDATAFLAGS kdfDebug;
                //  if on non-last node of page then key should be greater than current start key
                //
                if ( ilineT < ( csr.Cpage().Clines() - 1 ) )
                {
                    NDIGetKeydataflags( csr.Cpage(), ilineT, &kdfDebug );
                    Assert( kdfDebug.key.Cb() > 0 );
                    Assert( CmpKeyWithKeyData( kdfDebug.key, rgbmStart[ibm] ) > 0 );
                }
                //  if a previous key then it should be less than or equal to current start key
                //
                if ( ilineT > 0 )
                {
                    NDIGetKeydataflags( csr.Cpage(), ilineT - 1, &kdfDebug );
                    Assert( kdfDebug.key.Cb() > 0 );
                    Assert( CmpKeyWithKeyData( kdfDebug.key, rgbmStart[ibm] ) <= 0 );
                }
#endif
                //  should advance (in backward scan)
                //
                Assert( ilineT < iline );
                iline = ilineT;
            }
        }

        if( 0 == iline )
        {
            // we are at the start of the page so all remaining nodes are found on this subtree
            ibmMin  = ibm;
            ibmMax  = cbm;
            ibm     = cbm;
        }
        else
        {
            NDIGetKeydataflags( csr.Cpage(), iline-1, &kdfPrev );
            Assert( kdfPrev.key.Cb() > 0 );
            if( CmpKeyWithKeyData( kdfPrev.key, rgbmStart[ibm] ) <= 0 )
            {
                // the current iline points to the subtree for the current bookmark.
                // as we are prereading backwards the bookmarks are in descending order.
                // keep advancing through the bookmarks until we find one which doesn't
                // belong

                ibmMin = ibm;

                while( ibm < cbm )
                {
                    if( CmpKeyWithKeyData( kdfPrev.key, rgbmEnd[ibm] ) > 0 )
                    {
                        break;
                    }
                    // this bookmark is completely processed by this sub-tree
                    ibm++;
                }

                ibmMax = ibm;   // the first node to ignore
                while ( ibmMax < cbm )
                {
                    if ( CmpKeyWithKeyData( kdfPrev.key, rgbmStart[ibmMax] ) > 0 )
                    {
                        // this bookmark doesn't belong on the sub-tree
                        break;
                    }
                    ibmMax++;
                }
                Assert( ibmMax > ibmMin );
                Assert( ibmMax <= ibm + 1 );
            }
        }

        if( 0 == ibmMax )
        {
            // no match. this iline isn't needed
            Assert( 0 == ibmMin );
        }
        else
        {
            Assert( ibmMin < ibmMax );

            // retrieve the pgno of the subtree and process it
            KEYDATAFLAGS kdf;
            NDIGetKeydataflags( csr.Cpage(), iline, &kdf );

            Assert( sizeof( PGNO ) == kdf.data.Cb() );
            const PGNO pgno = *(UnalignedLittleEndian< PGNO > *) kdf.data.Pv();
            Assert( pgnoNull != pgno );

            if( csr.Cpage().FParentOfLeaf() )
            {
                // this is a parent of leaf page just add the pgno to the preread list
                Call( ErrAddPrereadCandidate( pgno ) );
                *pcbmRangesPreread = ibm;
            }
            else
            {
                // otherwise we have to recurse. we have a child page and know which bookmarks are
                // found on that page. store the information in the subtree preread list

                DBEnforce( csr.Cpage().Ifmp(), isubtree < _countof( rgibmMin ) );
                DBEnforce( csr.Cpage().Ifmp(), isubtree < _countof( rgibmMax ) );
                rgpgnoSubtree[isubtree] = pgno;
                rgibmMin[isubtree] = ibmMin;
                rgibmMax[isubtree] = ibmMax;
                isubtree++;

                Assert( isubtree <= csubtreePrereadMax );
                // subtree buffer full, need to process now
                if( csubtreePrereadMax == isubtree )
                {
                    rgpgnoSubtree[isubtree] = pgnoNull;

                    // Even non-leaf pages are requested to be added to the m_rgpgnoPreread buffer.
                    // So lets add the array of subtree pages about to be read.
                    // Note: Currently this flag is used only to identify space tree pages needed for available lag during table deletes.
                    if ( fIncludeNonLeafPg )
                    {
                        Call( ErrAddPrereadCandidates( rgpgnoSubtree, isubtree, PrereadPgType::NonLeafPages ) );
                    }

                    BFPrereadPageList( m_ifmp, rgpgnoSubtree, bfprfDefault, m_ppib->BfpriPriority( m_ifmp ), TcCurr() );
                    Call( ErrProcessSubtreeRangesBackward(
                            rgbmStart,
                            rgbmEnd,
                            rgpgnoSubtree,
                            rgibmMin,
                            rgibmMax,
                            isubtree,
                            pcbmRangesPreread,
                            fIncludeNonLeafPg ) );
                    isubtree = 0;
                }
            }
        }
    }

    // process any remaining subtrees
    AssumePREFAST( isubtree < csubtreePrereadMax );
    if( isubtree > 0 )
    {
        rgpgnoSubtree[isubtree] = pgnoNull;

        // Even non-leaf pages are requested to be added to the m_rgpgnoPreread buffer.
        // So lets add the array of subtree pages about to be read.
        // Note: Currently this flag is used only to identify space tree pages needed for available lag during table deletes.
        if ( fIncludeNonLeafPg )
        {
            Call( ErrAddPrereadCandidates( rgpgnoSubtree, isubtree, PrereadPgType::NonLeafPages ) );
        }

        BFPrereadPageList( m_ifmp, rgpgnoSubtree, bfprfDefault, m_ppib->BfpriPriority( m_ifmp ), TcCurr() );
        Call( ErrProcessSubtreeRangesBackward(
                rgbmStart,
                rgbmEnd,
                rgpgnoSubtree,
                rgibmMin,
                rgibmMax,
                isubtree,
                pcbmRangesPreread,
                fIncludeNonLeafPg ) );
        isubtree = 0;
    }

HandleError:
    return err;
}

//  ================================================================
ERR PrereadContext::ErrAddPrereadCandidate(
    const PGNO                  pgno,
    _In_ PrereadPgType const    prpgtyp )
//  ================================================================
{
    ERR err = JET_errSuccess;

    // realloc preread page buffer if it is too small to accept the new page
    const CPG cpgPrereadAllocMin = 128;
    const CPG cpgPrereadNew = m_cpgPreread[prpgtyp] + 1;
    if ( m_rgpgnoPreread[prpgtyp] == NULL || m_cpgPrereadAlloc[prpgtyp] < cpgPrereadNew )
    {
        CPG     cpgPrereadAllocNew  = max( cpgPrereadAllocMin, max( cpgPrereadNew, m_cpgPrereadAlloc[prpgtyp] * 2 ) );
        PGNO*   rgpgnoPrereadNew    = NULL;
        Alloc( rgpgnoPrereadNew = new PGNO[cpgPrereadAllocNew] );
        if ( m_rgpgnoPreread[prpgtyp] != NULL )
        {
            memcpy( rgpgnoPrereadNew, m_rgpgnoPreread[prpgtyp], sizeof( PGNO ) * m_cpgPrereadAlloc[prpgtyp] );
            delete[] m_rgpgnoPreread[prpgtyp];
        }
        m_cpgPrereadAlloc[prpgtyp] = cpgPrereadAllocNew;
        m_rgpgnoPreread[prpgtyp] = rgpgnoPrereadNew;
    }

    // add the new page ref
    m_rgpgnoPreread[prpgtyp][m_cpgPreread[prpgtyp]++] = pgno;

HandleError:
    return err;
}

//  ================================================================
ERR PrereadContext::ErrAddPrereadCandidates(
    _In_count_(cpgnos) const PGNO * const   rgpgno,
    const LONG                              cpgnos,
    _In_ PrereadPgType const                prpgtyp )
    //  ================================================================
{
    ERR err = JET_errSuccess;

    for ( CPG cpg = 0; cpg < cpgnos; cpg++ )
    {
        Call ( ErrAddPrereadCandidate( rgpgno[ cpg ], prpgtyp ) );
    }

HandleError:
    return err;
}

//  ================================================================
ERR PrereadContext::ErrPrereadKeys(
    __in_ecount(ckeys) const void * const * const   rgpvKeys,
    __in_ecount(ckeys) const ULONG * const  rgcbKeys,
    const LONG                                      ckeys,
    _Out_ LONG * const                              pckeysPreread,
    const JET_GRBIT                                 grbit )
//  ================================================================
//
//  Preread all the leaf pages containing the given keys
//
//-
{
    ERR err;

    BOOKMARK * rgbm = NULL;
    Alloc( rgbm = new BOOKMARK[ckeys] );

    for( INT ikey = 0; ikey < ckeys; ++ikey )
    {
        rgbm[ikey].key.prefix.Nullify();
        rgbm[ikey].key.suffix.SetPv( const_cast<VOID *>( rgpvKeys[ikey] ) );
        rgbm[ikey].key.suffix.SetCb( rgcbKeys[ikey] );
        rgbm[ikey].data.Nullify();
    }

    Call( ErrPrereadBookmarkRanges(
            rgbm,
            rgbm,
            ckeys,
            pckeysPreread,
            0,
            cpgPrereadSequential,
            grbit | bitPrereadSingletonRanges,
            NULL ) );

HandleError:
    delete[] rgbm;
    return err;
}

//  ================================================================
ERR ErrBTPrereadKeys(
    PIB * const                                     ppib,
    FUCB * const                                    pfucb,
    __in_ecount(ckeys) const void * const * const   rgpvKeys,
    __in_ecount(ckeys) const ULONG * const  rgcbKeys,
    const LONG                                      ckeys,
    _Out_ LONG * const                              pckeysPreread,
    const JET_GRBIT                                 grbit )
//  ================================================================
{
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTPreread );

    PrereadContext context( ppib, pfucb );
    return context.ErrPrereadKeys(
        rgpvKeys,
        rgcbKeys,
        ckeys,
        pckeysPreread,
        grbit );
}


//  ================================================================
ERR  ErrBTPrereadBookmarks(
    PIB * const                                     ppib,
    FUCB * const                                    pfucb,
    __in_ecount(cbm) const BOOKMARK * const         rgbm,
    const LONG                                      cbm,
    _Out_ LONG * const                              pcbmPreread,
    const JET_GRBIT                                 grbit )
//  ================================================================
{
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTPreread);

    PrereadContext context( ppib, pfucb );
    return context.ErrPrereadBookmarkRanges(
            rgbm,
            rgbm,
            cbm,
            pcbmPreread,
            0,
            cpgPrereadSequential,
            grbit | bitPrereadSingletonRanges,
            NULL );
}


//  ================================================================
ERR PrereadContext::ErrPrereadBookmarkRanges(
    __in_ecount(cbm) const BOOKMARK * const         rgbmStart,
    __in_ecount(cbm) const BOOKMARK * const         rgbmEnd,
    const LONG                                      cbm,
    _Out_ LONG * const                              pcbmRangesPreread,
    _In_ const ULONG                                cPageCacheMin,
    _In_ const ULONG                                cPageCacheMax,
    const JET_GRBIT                                 grbit,
    __out_opt ULONG * const                         pcPageCacheActual )
//  ================================================================
//
//  Preread all the leaf pages containing the given bookmarks
//
//-
{
    ERR     err             = JET_errSuccess;
    CSR     csrRoot;
    BOOL    fEligibleForOLD2        = !( grbit & bitPrereadDoNotDoOLD2 );
    BOOL    fSkipPreread            = !!( grbit & bitPrereadSkip );
    BOOL    fIncludeNonLeafPages    = !!( grbit & bitIncludeNonLeafRead );

    // bitIncludeNonLeafRead and fSkipPreread are only to capture space tree preimages for RBS
    Assert( !fSkipPreread || ( fSkipPreread && fIncludeNonLeafPages ) );

    *pcbmRangesPreread      = 0;
    m_fSawFragmentedSpace   = false;

    //  check parameters
    bool fForward = false;
    if ( grbit & JET_bitPrereadForward )
    {
        fForward = true;
    }
    else if ( grbit & JET_bitPrereadBackward )
    {
        fForward = false;
    }
    else
    {
        AssertSz( fFalse, "Bad grbit passed to PrereadContext::ErrPrereadBookmarkRanges" );
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }
    if ( cPageCacheMin > cPageCacheMax )
    {
        AssertSz( fFalse, "Bad cPageCacheMin/cPageCacheMax passed to PrereadContext::ErrPrereadBookmarkRanges" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( cPageCacheMax > lMax )
    {
        AssertSz( fFalse, "Bad cPageCacheMax passed to PrereadContext::ErrPrereadBookmarkRanges" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  compute how many pages we want to preread
    CPG cpgPrereadMax = (CPG)cPageCacheMax;
    if ( grbit & bitPrereadSingletonRanges )
    {
        cpgPrereadMax = min( cpgPrereadMax, (CPG)cbm );
    }
    const CPG cpgPrereadMin = min( (CPG)cPageCacheMin, cpgPrereadMax );

    //  complete empty preread requests
    if ( cpgPrereadMax == 0 )
    {
        *pcbmRangesPreread = 1;
        return JET_errSuccess;
    }

    // set our max preread
    m_cpgPrereadMax = cpgPrereadMax;

    //  open a CSR on the root of the index
    Call( csrRoot.ErrGetReadPage(
            m_ppib,
            m_ifmp,
            PgnoRoot( m_pfucb ),
            bflfDefault,
            PBFLatchHintPgnoRoot( m_pfucb )
            ) );

    // Should we just remove this condition and consider the rootpage as preread if its leaf page? The preread cache ignores if the page is already in BF cache
    // So doesn't really matter if we add to the preread array or not. For legacy reasons, lets do this only if non-leaf pages are requested to be included.
    if ( fIncludeNonLeafPages )
    {
        if ( csrRoot.Cpage().FLeafPage() )
        {
            Call( ErrAddPrereadCandidate( csrRoot.Pgno(), PrereadPgType::LeafPages ) );
        }
        else
        {
            Call( ErrAddPrereadCandidate( csrRoot.Pgno(), PrereadPgType::NonLeafPages ) );
        }
    }

    //  compute the list of pages to read
    if( fForward )
    {
        Call( ErrPrereadRangesForward(
                csrRoot,
                rgbmStart,
                rgbmEnd,
                cbm,
                pcbmRangesPreread,
                fIncludeNonLeafPages ) );
    }
    else
    {
        Call( ErrPrereadRangesBackward(
                csrRoot,
                rgbmStart,
                rgbmEnd,
                cbm,
                pcbmRangesPreread,
                fIncludeNonLeafPages ) );
    }

    // We shouldn't have added any non leaf page candidates unless requested
    Assert( m_cpgPreread[PrereadPgType::NonLeafPages] == 0 || fIncludeNonLeafPages );

    // add a null terminator to the candidate list
    Call( ErrAddPrereadCandidate( pgnoNull, PrereadPgType::NonLeafPages ) );
    Call( ErrAddPrereadCandidate( pgnoNull, PrereadPgType::LeafPages ) );

    if ( !fSkipPreread )
    {
        //  cooperatively throttle this preread request with other requests
        BFReserveAvailPages bfreserveavailpages( m_cpgPreread[PrereadPgType::LeafPages] - 1 );
        CPG cpgPrereadRequested = min( m_cpgPreread[PrereadPgType::LeafPages] - 1, max( cpgPrereadMin, bfreserveavailpages.CpgReserved() ) );

        // save the actual number of pages pre-read
        if ( pcPageCacheActual )
        {
            *pcPageCacheActual = cpgPrereadRequested;
        }

        //  try to preread the selected portion of the list
        PGNO pgnoPrereadSave = m_rgpgnoPreread[PrereadPgType::LeafPages][cpgPrereadRequested];
        m_rgpgnoPreread[PrereadPgType::LeafPages][cpgPrereadRequested] = pgnoNull;

        GetCurrUserTraceContext getutc;
        auto tc = TcCurr();
        BFPrereadPageList( m_ifmp, m_rgpgnoPreread[PrereadPgType::LeafPages], bfprfDefault, m_ppib->BfpriPriority( m_ifmp ), tc );

        m_rgpgnoPreread[PrereadPgType::LeafPages][cpgPrereadRequested] = pgnoPrereadSave;

        //  trace the preread requests and if they were ignored or not
        for ( CPG ipg = 0; ipg < m_cpgPreread[PrereadPgType::LeafPages] - 1; ipg++ )
        {
            //  use only the least significant bit in case we want to add more flags later
            const BYTE fOpFlags = ipg < cpgPrereadRequested && FBFInCache( m_ifmp, m_rgpgnoPreread[PrereadPgType::LeafPages][ipg] ) ? 0 : 1;

            ETBTreePrereadPageRequest(
                m_ifmp,
                m_rgpgnoPreread[PrereadPgType::LeafPages][ipg],
                getutc->context.dwUserID,
                getutc->context.nOperationID,
                getutc->context.nOperationType,
                getutc->context.nClientType,
                getutc->context.fFlags,
                getutc->dwCorrelationID,
                tc.iorReason.Iorp(),
                tc.iorReason.Iors(),
                tc.iorReason.Iort(),
                tc.iorReason.Ioru(),
                tc.iorReason.Iorf(),
                tc.nParentObjectClass,
                fOpFlags );
        }
    }

    //  if we saw fragmented space while computing the list of pages to preread then
    //  try to register this index for defragmentation for contiguity
    if( m_fSawFragmentedSpace && fEligibleForOLD2 )
    {
        (void)ErrBTIRegisterForOLD2( m_pfucb );
    }

    //  if the first range was only partially read, call it done
    if ( *pcbmRangesPreread == 0 )
    {
        *pcbmRangesPreread = 1;
    }

HandleError:
    csrRoot.ReleasePage();
    return err;
}

//  ================================================================
ERR PrereadContext::ErrPrereadKeyRanges(
    __in_ecount(cRanges) const void *   const * const   rgpvKeysStart,
    __in_ecount(cRanges) const ULONG * const            rgcbKeysStart,
    __in_ecount(cRanges) const void *   const * const   rgpvKeysEnd,
    __in_ecount(cRanges) const ULONG * const            rgcbKeysEnd,
    const LONG                                          cRanges,
    __deref_out_range( 0, cRanges ) LONG * const        pcRangesPreread,
    _In_ const ULONG                                    cPageCacheMin,
    _In_ const ULONG                                    cPageCacheMax,
    const JET_GRBIT                                     grbit,
    __out_opt ULONG * const                             pcPageCacheActual )
//  ================================================================
//
//  Preread all the leaf pages containing the given keys
//
//-
{
    ERR err;

    BOOKMARK * rgbmStart = NULL;
    BOOKMARK * rgbmEnd = NULL;
    Alloc( rgbmStart = new BOOKMARK[cRanges] );
    Alloc( rgbmEnd = new BOOKMARK[cRanges] );

    for( INT irange = 0; irange < cRanges; ++irange )
    {
        rgbmStart[irange].key.prefix.Nullify();
        rgbmStart[irange].key.suffix.SetPv( const_cast<VOID *>( rgpvKeysStart[irange] ) );
        rgbmStart[irange].key.suffix.SetCb( rgcbKeysStart[irange] );
        rgbmStart[irange].data.Nullify();

        rgbmEnd[irange].key.prefix.Nullify();
        rgbmEnd[irange].key.suffix.SetPv( const_cast<VOID *>( rgpvKeysEnd[irange] ) );
        rgbmEnd[irange].key.suffix.SetCb( rgcbKeysEnd[irange] );
        rgbmEnd[irange].data.Nullify();
    }

    Call( ErrPrereadBookmarkRanges(
            rgbmStart,
            rgbmEnd,
            cRanges,
            pcRangesPreread,
            cPageCacheMin,
            cPageCacheMax,
            grbit,
            pcPageCacheActual ) );

HandleError:
    delete[] rgbmStart;
    delete[] rgbmEnd;
    return err;
}

//  ================================================================
ERR ErrBTPrereadKeyRanges(
    PIB * const                                     ppib,
    FUCB * const                                    pfucb,
    __in_ecount(ckeys) const void * const * const   rgpvKeysStart,
    __in_ecount(ckeys) const ULONG * const          rgcbKeysStart,
    __in_ecount(ckeys) const void * const * const   rgpvKeysEnd,
    __in_ecount(ckeys) const ULONG * const          rgcbKeysEnd,
    const LONG                                      ckeys,
    __deref_out_range( 0, ckeys ) LONG * const      pcRangesPreread,
    _In_ const ULONG                                cPageCacheMin,
    _In_ const ULONG                                cPageCacheMax,
    const JET_GRBIT                                 grbit,
    __out_opt ULONG * const                         pcPageCacheActual )
//  ================================================================
{
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTPreread);

    PrereadContext context( ppib, pfucb );
    return context.ErrPrereadKeyRanges(
        rgpvKeysStart,
        rgcbKeysStart,
        rgpvKeysEnd,
        rgcbKeysEnd,
        ckeys,
        pcRangesPreread,
        cPageCacheMin,
        cPageCacheMax == ulMax ? cpgPrereadRangesMax : (CPG)cPageCacheMax,
        grbit,
        pcPageCacheActual );
}


//  ================================================================
VOID BTIPrereadContiguousPages(
    const IFMP ifmp,
    const CSR& csr,
    PrereadInfo * const pPrereadInfo,
    const BFPreReadFlags bfprf,
    const BFPriority bfpri,
    const TraceContext &tc )
//  ================================================================
//
//  Given a csr pointing to an internal page, preread all the contiguous
//  pages from the iline in the CSR on. Up to cpgMax pages are preread
//  and the number of pages preread is returned in pcpgPreread.
//
//  Pages are only preread if they are exactly in order, with no gaps.
//
//  We will pre-read even for a single page so that we can correctly fast-evict.
//
//-
{
    Assert( pPrereadInfo );
    Assert( pPrereadInfo->cpgToPreread > 0 );
    Assert( pPrereadInfo->cpgToPreread <= _countof( pPrereadInfo->rgfPageWasAlreadyCached ) );

    const CPAGE& cpage = csr.Cpage();
    Assert( cpage.FInvisibleSons() );

    BFReserveAvailPages bfreserveavailpages( cpage.Clines() - csr.ILine() );

    const INT ilineStart = csr.ILine();
    const INT ilineMax = csr.ILine() + bfreserveavailpages.CpgReserved();
    Assert( ilineStart <= ilineMax );

    KEYDATAFLAGS kdf;
    NDIGetKeydataflags( cpage, ilineStart, &kdf );

    Assert( sizeof( PGNO ) == kdf.data.Cb() );
    const PGNO pgnoFirst = *(UnalignedLittleEndian< PGNO > *) kdf.data.Pv();
    Assert( pgnoNull != pgnoFirst );

    CPG cpg = 1;

    // count the number of contiguous pages
    while ( ilineStart + cpg < ilineMax )
    {
        const INT iline = ilineStart + cpg;

        NDIGetKeydataflags( cpage, iline, &kdf );

        Assert( sizeof( PGNO ) == kdf.data.Cb() );
        const PGNO pgno = *(UnalignedLittleEndian< PGNO > *) kdf.data.Pv();
        Assert( pgnoNull != pgno );
        Assert( pgnoFirst != pgno );

        if ( pgno != pgnoFirst + cpg )
        {
            break;
        }

        ++cpg;

        if ( pPrereadInfo->cpgToPreread == cpg )
        {
            break;
        }
    }

    Assert( cpg >= 1 );
    Assert( cpg <= pPrereadInfo->cpgToPreread );

    pPrereadInfo->pgnoPrereadStart = pgnoFirst;

    // New scope not needed, all prereads will be issued under one context
    Assert( FParentObjectClassSet( tc.nParentObjectClass ) );
    BFPrereadPageRange( ifmp, pgnoFirst, cpg, &pPrereadInfo->cpgActuallyPreread, pPrereadInfo->rgfPageWasAlreadyCached, bfprf, bfpri, tc );
}

//  extracts the list of pages to preread and call BF to preread them
//
ERR ErrBTIPreread( FUCB *pfucb, CPG cpg, CPG * pcpgActual )
{
#ifdef DEBUG
    const INT   ilineOrig = Pcsr( pfucb )->ILine();

    Unused( ilineOrig );
#endif  //  DEBUG

    const CPG   cpgPrereadWanted = min( cpg,
                                  FFUCBPrereadForward( pfucb ) ?
                                    Pcsr( pfucb )->Cpage().Clines() - Pcsr( pfucb )->ILine() :
                                    Pcsr( pfucb )->ILine() + 1 );
    BFReserveAvailPages bfreserveavailpages( cpgPrereadWanted );
    const CPG   cpgPreread = bfreserveavailpages.CpgReserved();
    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->iorReason.AddFlag( iorfOpportune );
    Assert( FParentObjectClassSet( tcScope->nParentObjectClass ) );

    if( cpgPreread <= 1 )
    {
        // we couldn't reserve any pages to preread. no point going any further
        *pcpgActual = 0;
        return JET_errSuccess;
    }

    // Pre-reads on LVs should always have an index range set
    Assert( !FFUCBLongValue( pfucb ) || FFUCBLimstat( pfucb ) );

    //  this buffer is larger than any possible preread list
    Assert( ( cpgPreread + 1 ) * sizeof( PGNO ) <= (size_t)g_rgfmp[ pfucb->ifmp ].CbPage() );
    PGNO * rgpgnoPreread;
    BFAlloc( bfasTemporary, (void**)&rgpgnoPreread );

    KEY     keyLimit;
    BOOL    fOutOfRange = fFalse;
    if( FFUCBLimstat( pfucb ) )
    {
        //  we don't want to preread pages that aren't part of the index range
        //  the separator key is greater than any key on the page it points to
        //  so we always preread a page if the separator key is equal to our
        //  index limit

        if( !pfucb->u.pfcb->FTypeLV() )
        {
            FUCBAssertValidSearchKey( pfucb );
        }
        keyLimit.prefix.Nullify();
        keyLimit.suffix.SetPv( pfucb->dataSearchKey.Pv() );
        keyLimit.suffix.SetCb( pfucb->dataSearchKey.Cb() );
    }

    INT     ipgnoPreread;
    for( ipgnoPreread = 0; ipgnoPreread < cpgPreread; ++ipgnoPreread )
    {
        NDGet( pfucb );
        Assert( pfucb->kdfCurr.data.Cb() == sizeof( PGNO ) );

        if( FFUCBLimstat( pfucb ) )
        {

            //  we want to preread the first page that is out of range
            //  so don't break until the top of the loop

            if( fOutOfRange )
            {
                break;
            }

            //  the separator key is suffix compressed so just compare the shortest key

            const INT   cmp             = CmpKeyShortest( pfucb->kdfCurr.key, keyLimit );

            if ( cmp > 0 )
            {
                fOutOfRange = FFUCBUpper( pfucb );
            }
            else if ( cmp < 0 )
            {
                fOutOfRange = !FFUCBUpper( pfucb );
            }
            else
            {
                //  always preread the page if the separator key is equal
                fOutOfRange = fFalse;
            }
        }

        rgpgnoPreread[ipgnoPreread] = *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv();

        if( FFUCBPrereadBackward( pfucb ) )
        {
            Assert( Pcsr( pfucb )->ILine() >= 0 );
            Pcsr( pfucb )->DecrementILine();
        }
        else
        {
            Assert( Pcsr( pfucb )->ILine() < Pcsr( pfucb )->Cpage().Clines() );
            Pcsr( pfucb )->IncrementILine();
        }
    }

    // Sort the pages before checking. If we are doing a descending preread the
    // pages will be in the wrong order. Also, if the pages are slightly out
    // of order then preread will still read them efficiently so we don't want
    // to trigger OLD2.
    sort( rgpgnoPreread, rgpgnoPreread + ipgnoPreread );

    if ( pfucb->u.pfcb->FUseOLD2() &&
         g_rgfmp[pfucb->ifmp].FSeekPenalty() &&
         FBTICpgFragmentedRangeTrigger(
            rgpgnoPreread,
            ipgnoPreread,
            cpgFragmentedRangeDefragTrigger,
            cpgFragmentedRangeDefrag ) )
    {
        (void)ErrBTIRegisterForOLD2( pfucb );
    }

    rgpgnoPreread[ipgnoPreread] = pgnoNull;

    // New scope not needed, all prereads will be issued under one context
    auto tc = TcCurr();
    Assert( FParentObjectClassSet( tc.nParentObjectClass ) );
    BFPrereadPageList( pfucb->u.pfcb->Ifmp(), rgpgnoPreread, pcpgActual, bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->u.pfcb->Ifmp() ), tc );

    BFFree( rgpgnoPreread );
    return JET_errSuccess;
}

// atmost 8 pages for such opportunistic reading.
#define  dwPagesMaxOpportuneRead  8

// If the leaf page we are navigating to is not already in the cache then 
// attempt to preread adjacent pages that are also not in cache. i.e. if there
// is going to be an IO try to make it a larger IO to get contiguous pages on 
// either side.
// pfucb must be at ParentOfLeaf and ILine must be positioned to point to pgnoChild.
ERR ErrBTIOpportuneRead( FUCB *pfucb, PGNO pgnoChild )
{
    CSR * const pcsr            = Pcsr( pfucb );
    Assert( pcsr->Cpage().FParentOfLeaf() );

    // Do any Opportune read optimization only if target page is not already in cache. i.e. only if
    // there is going to be an IO in any case.
    if ( !FBFInCache( pfucb->ifmp, pgnoChild ) )
    {
        BYTE    rgbForward[dwPagesMaxOpportuneRead];
        BYTE    rgbBackward[dwPagesMaxOpportuneRead];
        memset(rgbForward, 0, sizeof(rgbForward));
        memset(rgbBackward, 0, sizeof(rgbBackward));

        // Scan the parent-of-leaf node and check for presence of pages
        // that are on either side of pgnoChild (upto dwPagesMaxOpportuneRead)
        const INT cline = pcsr->Cpage().Clines();
        for ( INT iline = 0; iline < cline; ++iline )
        {
            KEYDATAFLAGS    kdfCurr;
            NDIGetKeydataflags( pcsr->Cpage(), iline, &kdfCurr );
            Assert( sizeof(PGNO) == kdfCurr.data.Cb() );
            const PGNO  pgno    = *( UnalignedLittleEndian< PGNO > * )kdfCurr.data.Pv();

            if ( pgno > pgnoChild && ( ( pgno - pgnoChild ) < dwPagesMaxOpportuneRead ) )
            {
                rgbForward[pgno - pgnoChild] = 1;
            }
            else if (pgno < pgnoChild && ( ( pgnoChild - pgno ) < dwPagesMaxOpportuneRead ) )
            {
                rgbBackward[pgnoChild - pgno] = 1;
            }
        }

        // Look in pages on either side of pgnoChild that can be opportunely read
        PGNO    pgnoMin = pgnoChild;
        PGNO    pgnoMost = pgnoChild;
        DWORD   dwPagesTotal = 1;           //count pgnoChild itself as 1

        for (PGNO pgnoLoop = pgnoChild + 1;
                dwPagesTotal < dwPagesMaxOpportuneRead;
                    ++pgnoLoop)
        {
            // Stop if page is already in cache
            if ( FBFInCache( pfucb->ifmp, pgnoLoop) )
                break;

            // stop if page is not referenced anywhere in the current ParentOfLeaf
            if ( rgbForward[pgnoLoop - pgnoChild] == 0)
                break;

            pgnoMost = pgnoLoop;
            ++dwPagesTotal;
        }

        // Similar loop in the other direction to find the Min.
        for (PGNO pgnoLoop = pgnoChild - 1;
                dwPagesTotal < dwPagesMaxOpportuneRead;
                    --pgnoLoop)
        {
            // Stop if page is already in cache
            if ( FBFInCache( pfucb->ifmp, pgnoLoop) )
                break;

            // stop if page is not referenced anywhere in the current ParentOfLeaf
            if ( rgbBackward[pgnoChild - pgnoLoop] == 0)
                break;

            pgnoMin = pgnoLoop;
            ++dwPagesTotal;
        }

        // Do preread if we have atleast 2 contiguous pages. So if we come here with 1, it will
        // get sync-read in the regular read codepath.
        if ( dwPagesTotal >= 2 )
        {
            PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
            tcScope->iorReason.AddFlag( iorfOpportune );
            Assert( FParentObjectClassSet( tcScope->nParentObjectClass ) );

            // this preread includes pgnoChild, which we have to preread anyway so subtract one from the preread count
            // to determine to number of extra pages being read
            PERFOpt( cBTOpportuneReads.Add( PinstFromPfucb( pfucb )->m_iInstance, (TCE)tcScope->nParentObjectClass, dwPagesTotal - 1 ) );

            BFPrereadPageRange( pfucb->ifmp, pgnoMin, dwPagesTotal, bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->ifmp ), *tcScope );
        }
    }
    return JET_errSuccess;
}

// stack wrapper for RESKey object

class CAutoKey
{
public:
    enum AllocState { eDeferAlloc = 7 };

    CAutoKey( void ) : m_pvKey( RESKEY.PvRESAlloc() ) { }
    CAutoKey( const AllocState eAllocState ) : m_pvKey( NULL ) { Assert( eAllocState == eDeferAlloc ); }

    ERR ErrAlloc()
    {
        ERR err = JET_errSuccess;
        AllocR( m_pvKey = RESKEY.PvRESAlloc() );
        return err;
    }

    INT CbMax() const
    {
        if ( m_pvKey )
        {
            DWORD_PTR cbSize;
            CallS( RESKEY.ErrGetParam( JET_resoperSize, &cbSize ) );
            Assert( ((DWORD_PTR)((INT)cbSize)) == cbSize );
            return (INT)cbSize;
        }
        // not allocated yet ...
        return 0;
    }

    ~CAutoKey( void ) { RESKEY.Free( m_pvKey ); }

    operator void* ( void ) { return m_pvKey; }

private:
    void* m_pvKey;
};

JETUNITTEST( BTICAutoKey, SimpleAlloc )
{
    CAutoKey key;
    CHECK( key != NULL );   // potential OOM failure
    CHECK( key.CbMax() == cbKeyAlloc );
}

JETUNITTEST( BTICAutoKey, DeferredNeverAlloc )
{
    CAutoKey key( CAutoKey::eDeferAlloc );
    CHECK( key == NULL );
    CHECK( key.CbMax() == 0 );
}

JETUNITTEST( BTICAutoKey, DeferredAlloc )
{
    CAutoKey key( CAutoKey::eDeferAlloc );
    CHECK( key == NULL );
    CHECK( key.CbMax() == 0 );
    CHECKCALLS( key.ErrAlloc() );   // potential OOM failure
    CHECK( key != NULL );   // potential OOM failure
    CHECK( key.CbMax() == cbKeyAlloc );
}


//  seeks to key from root of tree
//
//      pdib->pos == posFirst --> seek to first node in tree
//                   posLast  --> seek to last node in tree
//                   posDown --> seek to pdib->key in tree
//                   posFrac --> fractional positioning
//
//      pdib->pbm   used for posDown and posFrac
//
//      pdib->dirflag == fDIRAllNode --> seek to deleted/versioned nodes too
//
//  positions cursor on node if one exists and is visible to cursor
//  else on next node visible to cursor
//  if no next node exists that is visible to cursor,
//      move previous to visible node
//
ERR ErrBTDown( FUCB *pfucb, DIB *pdib, LATCH latch )
{
    ERR         err                         = JET_errSuccess;
    ERR         wrn                         = JET_errSuccess;
    CSR * const pcsr                        = Pcsr( pfucb );
    PGNO        pgnoParent                  = pgnoNull;
    const BOOL  fSeekOnNonUniqueKeyOnly     = ( posDown == pdib->pos
                                                && !FFUCBUnique( pfucb )
                                                && 0 == pdib->pbm->data.Cb() );
    PIBTraceContextScope tcScope            = TcBTICreateCtxScope( pfucb, iorsBTSeek );
    double      dblCurrentFrac;
#ifdef CHECK_UNIQUE_KEY_ON_NONUNIQUE_INDEX
    //  unique-key check can only be done for an exact-match key-only seek on
    //  a non-unique index
    BOOL        fKeyIsolatedToCurrentPage   = fFalse;
    const BOOL  fCheckUniqueness            = ( ( pdib->dirflag & fDIRCheckUniqueness )
                                                && ( pdib->dirflag & fDIRExact )
                                                && fSeekOnNonUniqueKeyOnly );
#endif

#ifdef DEBUG
    ULONG       ulTrack                     = 0x00;

    //  Disable RFS and cleanup checking
    //  It's not a real failure path because it's debug only code
    const LONG cRFSCountdownOld = RFSThreadDisable( 0 );
    BOOL fNeedReEnableRFS = fTrue;
    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

    //  we must save off the key (and possibly data) to make sure we can properly validate our
    //  resulting position with the desired / seeked key.  We must save it off because if we
    //  call ErrBTNext() or ErrBTPrev() we may release / save our bookmark / bmCurr.  And this
    //  might affect pdib->pbm because at least one caller (ErrBTPerformOnSeekBM) passes bmCurr 
    //  as pdib->pbm.

    CAutoKey    keySeekKey( CAutoKey::eDeferAlloc );
    CAutoKey    keySeekData( CAutoKey::eDeferAlloc );

    BOOKMARK    bmSeek;
    bmSeek.Nullify();

    if ( pdib->pos == posDown )
    {
        Assert( bmSeek.data.Pv() == NULL );
        Assert( bmSeek.data.Cb() == 0 );

        Call( keySeekKey.ErrAlloc() );
        Assert( pdib->pbm->key.Cb() <= keySeekKey.CbMax() );
        pdib->pbm->key.CopyIntoBuffer( keySeekKey, pdib->pbm->key.Cb() );
        bmSeek.key.suffix.SetPv( keySeekKey );
        bmSeek.key.suffix.SetCb( pdib->pbm->key.Cb() );

        Assert( FKeysEqual( bmSeek.key, pdib->pbm->key ) );

        if ( !( FFUCBUnique( pfucb ) || fSeekOnNonUniqueKeyOnly ) )
        {
            //  key should be enough, because data should be a bookmark / primary key.
            Call( keySeekData.ErrAlloc() );
            Assert( pdib->pbm->data.Cb() <= keySeekData.CbMax() );
            memcpy( keySeekData, pdib->pbm->data.Pv(), pdib->pbm->data.Cb() );
            bmSeek.data.SetPv( keySeekData );
            bmSeek.data.SetCb( pdib->pbm->data.Cb() );
            Assert( FDataEqual( bmSeek.data, pdib->pbm->data ) );
        }
    }

    //  Restore RFS and cleanup checking
    //
    FOSSetCleanupState( fCleanUpStateSaved );
    RFSThreadReEnable( cRFSCountdownOld );
    fNeedReEnableRFS = fFalse;
#endif  //  DEBUG

    if ( pdib->pos == posFrac )
    {
        // calculate the fraction desired for the initial iteration of the loop.
        const FRAC *const pfrac = (FRAC *)(pdib->pbm);
        dblCurrentFrac = (double)pfrac->ullLT / (double)pfrac->ullTotal;
        if ( dblCurrentFrac > 1.0 )
        {
            // Somehow we were called with ulLT > ulTotal.  Bad form, but
            // we'll just take that to mean EndOfIndex.
            dblCurrentFrac = 1.0;
        }
        Assert( 0.0 <= dblCurrentFrac );
        Assert( dblCurrentFrac <= 1.0 );
    }

    // Prevent R/O transactions from proceeding if they are holding up version store cleanup.
    //
    if ( pfucb->ppib->FReadOnlyTrx() && pfucb->ppib->Level() > 0 )
    {
        Call( PverFromIfmp( pfucb->ifmp )->ErrVERCheckTransactionSize( pfucb->ppib ) );
    }

    Assert( FBTLogicallyNavigableState( pfucb ) ||
                // Special Case: There is one time during redo we allow walking down a B-tree,
                // during sizing the database which happens at "safe" points during recovery,
                // and only interacts with the root OE tree.
                ( FSPIsRootSpaceTree( pfucb ) &&
                    // except if we are patched, the OE tree may be invalid
                    g_rgfmp[ pfucb->ifmp ].Pdbfilehdr()->Dbstate() != JET_dbstateDirtyAndPatchedShutdown )
            );

    Assert( latchReadTouch == latch || latchReadNoTouch == latch ||
            latchRIW == latch );
    Assert( posDown == pdib->pos
            || 0 == ( pdib->dirflag & (fDIRFavourPrev|fDIRFavourNext|fDIRExact) ) );

    //  no latch should be held by cursor on tree
    //
    Assert( !pcsr->FLatched() );

    PERFOpt( PERFIncCounterTable( cBTSeek, PinstFromPfucb( pfucb ), (TCE)tcScope->nParentObjectClass ) );

    //  go to root
    //
    Call( ErrBTIGotoRoot( pfucb, latch ) );

    //  if no nodes in root, return
    //
    if ( 0 == pcsr->Cpage().Clines() )
    {
        BTUp( pfucb );
        return ErrERRCheck( JET_errRecordNotFound );
    }

    CallS( err );

#ifdef PREREAD_ON_INITIAL_SEEK
    //  if sequentially scanning, but preread hasn't
    //  yet been initiated, then initiate preread
    //  if we're moving to posfirst or posLast
    //
    if ( FFUCBSequential( pfucb ) && !FFUCBPreread( pfucb ) )
    {
        if ( posFirst == pdib->pos )
        {
            //  since we're sequentially scanning and moving
            //  to the first record, presumably we're going
            //  to be scanning forward
            //
            FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
        }
        else if ( posLast == pdib->pos )
        {
            //  since we're sequentially scanning and moving
            //  to the last record, presumably we're going
            //  to be scanning backward
            //
            FUCBSetPrereadBackward( pfucb, cpgPrereadSequential );
        }
    }
#endif  //  PREREAD_ON_INITIAL_SEEK

    //  setup to compute our record position
    //
    pfucb->ullLTLast = 0;
    pfucb->ullTotalLast = 1;

    //  seek to key
    //
    for ( ; ; )
    {
        //  verify page belongs to this btree
        //
        if ( pcsr->Cpage().ObjidFDP() != pfucb->u.pfcb->ObjidFDP() )
        {
            //  if not repair, assert, otherwise, suppress the assert and repair will
            //  just naturally err out
            AssertSz( g_fRepair, "Corrupt B-tree: page does not belong to btree" );
            Error( ErrBTIReportBadPageLink(
                        pfucb,
                        ErrERRCheck( JET_errBadParentPageLink ),
                        pgnoParent,
                        pcsr->Pgno(),
                        pcsr->Cpage().ObjidFDP(),
                        fTrue,
                        "BtDownObjid" ) );
        }

        //  verify page is not empty
        //
        if ( pcsr->Cpage().Clines() <= 0 )
        {
            //  if not repair, assert, otherwise, suppress the assert and repair will
            //  just naturally err out
            AssertSz( g_fRepair, "Corrupt B-tree: page found was empty" );
            Error( ErrBTIReportBadPageLink(
                      pfucb,
                      ErrERRCheck( JET_errBadParentPageLink ),
                      pgnoParent,
                      pcsr->Pgno(),
                      pcsr->Cpage().ObjidFDP(),
                      fTrue,
                      ( pcsr->Cpage().FPreInitPage() ?
                        "BtDownClinesLowPreinit" :
                        ( pcsr->Cpage().FEmptyPage() ?
                          "BtDownClinesLowEmpty" :
                          "BtDownClinesLowInPlace" ) ) ) );
        }
        Assert( pcsr->Cpage().Clines() > 0 );

        //  for every page level, set to correct line.
        //  if internal page,
        //      get child page
        //      move cursor to new page
        //      release parent page
        //
        switch ( pdib->pos )
        {
            case posDown:
                Call( ErrNDSeek( pfucb, *(pdib->pbm) ) );
                wrn = err;
                break;

            case posFirst:
                NDMoveFirstSon( pfucb );
                break;

            case posLast:
                NDMoveLastSon( pfucb );
                break;

            default:
                Assert( pdib->pos == posFrac );
                pcsr->SetILine( IlineBTIFrac( Pcsr(pfucb), &dblCurrentFrac ) );
                NDGet( pfucb );
                break;
        }


        const INT   iline   = pcsr->ILine();
        const INT   clines  = pcsr->Cpage().Clines();

        //  adjust number of records and key position
        //  for this tree level
        //  With 64 bit variables, we really don't expect to overflow.
        //
        Expected( pfucb->ullLTLast <= pfucb->ullLTLast * clines + iline ) ;
        Expected( pfucb->ullTotalLast <= pfucb->ullTotalLast * clines );
        pfucb->ullLTLast = pfucb->ullLTLast * clines + iline;
        pfucb->ullTotalLast = pfucb->ullTotalLast * clines;

        if ( pcsr->Cpage().FLeafPage() )
        {
            //  leaf node reached, exit loop
            //
            break;
        }
        else
        {
#ifdef DEBUG
            const PGNO  pgnoDBG             = pcsr->Pgno();

            Unused( pgnoDBG );

            ulTrack |= 0x01;
#endif

            const BOOL  fPageParentOfLeaf   = pcsr->Cpage().FParentOfLeaf();

            //  if performing a key-only seek on a non-unique tree,
            //  ErrNDSeek above GUARANTEES that this node's key+data
            //  is greater than the key being seeked, because of one
            //  of the following:
            //      - the node has a NULL key
            //      - the node's key itself is greater than the key being seeked
            //      - keys are equal, but the node's data portion forces it greater
            Assert( !fSeekOnNonUniqueKeyOnly
                || pfucb->kdfCurr.key.FNull()
                || CmpKeyShortest( pfucb->kdfCurr.key, pdib->pbm->key ) > 0
                || ( CmpKeyShortest( pfucb->kdfCurr.key, pdib->pbm->key ) == 0
                    && pfucb->kdfCurr.key.Cb() > pdib->pbm->key.Cb() ) );

#ifdef CHECK_UNIQUE_KEY_ON_NONUNIQUE_INDEX
            if ( fPageParentOfLeaf && fCheckUniqueness )
            {
                //  The key being seeked may have dupes if this
                //  node's key is not strictly greater than the
                //  key being seeked when we compare up to the
                //  length of the shortest of the two
                //
                fKeyIsolatedToCurrentPage = ( pfucb->kdfCurr.key.FNull()
                                            || CmpKeyShortest( pfucb->kdfCurr.key, pdib->pbm->key ) > 0 );
            }
#endif

            //  get pgno of child from node
            //  switch to that page
            //
            Assert( pfucb->kdfCurr.data.Cb() == sizeof( PGNO ) );
            const PGNO  pgnoChild           = *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv();

            // Do smart pre-reading
            if ( fPageParentOfLeaf && FFUCBOpportuneRead( pfucb ) )
            {
                // Attempt to Opportunely read data by looking for pages on either side of
                // the target page, i.e. if we are going to be doing an IO then try to do a larger IO.
                Call( ErrBTIOpportuneRead( pfucb, pgnoChild ) );
            }

            if ( FFUCBPreread( pfucb ) )
            {
                //  NOTE: the preread code below may not restore the kdfCurr

                if ( fPageParentOfLeaf )
                {
                    if ( 0 == pfucb->cpgPrereadNotConsumed && pfucb->cpgPreread > 1 )
                    {
                        //  if prereading and have reached the page above the leaf level,
                        //  extract next set of pages to be preread
                        Call( ErrBTIPreread( pfucb, pfucb->cpgPreread, &pfucb->cpgPrereadNotConsumed) );
                    }
                }
                else if ( ( FFUCBSequential( pfucb ) || FFUCBLimstat( pfucb ) )
                        && FFUCBPrereadForward( pfucb ) )
                {
                    //  if prereading a sequential table and on an internal page read the next
                    //  internal child page as well
                    CPG cpgUnused;
                    Call( ErrBTIPreread( pfucb, 2, &cpgUnused ) );
                }

                //  iline must be restored because ErrBTIPreread may have fiddled
                //  with it in order to look at other nodes on the page
                //
                pcsr->SetILine( iline );
            }

            pgnoParent = pcsr->Pgno();

            Call( pcsr->ErrSwitchPage(
                        pfucb->ppib,
                        pfucb->ifmp,
                        pgnoChild,
                        pfucb->u.pfcb->FNoCache()  ) );

            // 05/08/09, SOMEONE: this is a test hack which forces ESE to generate an ignored log
            // record. We need to make sure that we can skip over ignorable log records during recovery.
            if(JET_errSuccess != ErrFaultInjection(43973))
            {
                Call(ErrLGIgnoredRecord(
                        PinstFromPfucb(pfucb)->m_plog,
                        pfucb->ifmp,
                        rand() % 500));
            }

            Assert( FFUCBRepair( pfucb )
                    || pcsr->Cpage().FLeafPage() && fPageParentOfLeaf
                    || !pcsr->Cpage().FLeafPage() && !fPageParentOfLeaf );
        }
    }

    //  now, the cursor is on leaf node
    //
    Assert( pcsr->Cpage().FLeafPage() );
    AssertBTType( pfucb );
    AssertNDGet( pfucb );

    //  if we were going to the first/last page in the tree, check to see that it
    //  doesn't have a sibling
    if ( posFirst == pdib->pos && pgnoNull != pcsr->Cpage().PgnoPrev() )
    {
        //  if not repair, assert, otherwise, suppress the assert and repair will
        //  just naturally err out
        AssertSz( g_fRepair, "Corrupt B-tree: first page has pgnoPrev" );
        Call( ErrBTIReportBadPageLink(
                    pfucb,
                    ErrERRCheck( JET_errBadParentPageLink ),
                    pgnoParent,
                    pcsr->Pgno(),
                    pcsr->Cpage().PgnoPrev(),
                    fTrue,
                    "BtDownLeftMostPagePgnoPrevNonNull" ) );
    }
    else if ( posLast == pdib->pos && pgnoNull != pcsr->Cpage().PgnoNext() )
    {
        //  if not repair, assert, otherwise, suppress the assert and repair will
        //  just naturally err out
        AssertSz( g_fRepair, "Corrupt B-tree: last page has pgnoNext" );
        Call( ErrBTIReportBadPageLink(
                    pfucb,
                    ErrERRCheck( JET_errBadParentPageLink ),
                    pgnoParent,
                    pcsr->Pgno(),
                    pcsr->Cpage().PgnoNext(),
                    fTrue,
                    "BtDownRightMostPagePgnoNextNonNull" ) );
    }

    //  if node is not visible to cursor,
    //  move next till a node visible to cursor is found,
    //      if that leaves end of tree,
    //          move previous to first node visible to cursor
    //
    //  fDIRAllNode flag is used by ErrBTGotoBookmark
    //  to go to deleted records
    //
    Assert( !( ( pdib->dirflag & fDIRAllNode ) &&
            JET_errNoCurrentRecord == err ) );
    if ( !( pdib->dirflag & fDIRAllNode ) )
    {
        OnDebug( ulTrack |= 0x02 );

        Assert( !( pdib->dirflag & fDIRAllNodesNoCommittedDeleted ) );

        BOOL    fVisible;
        Call( ErrNDVisibleToCursor( pfucb, &fVisible, NULL ) );
        Assert( !fVisible || JET_errNoCurrentRecord != err );

        BOOL fIgnoreRecord = fFalse;
        if ( fVisible && pfucb->pmoveFilterContext )
        {
            Call( pfucb->pmoveFilterContext->pfnMoveFilter( pfucb, pfucb->pmoveFilterContext ) );
            Assert( err == JET_errSuccess || err == wrnBTNotVisibleRejected || err == wrnBTNotVisibleAccumulated );
            fIgnoreRecord = err > JET_errSuccess;
        }

        if ( !fVisible || fIgnoreRecord )
        {
            OnDebug( ulTrack |= !fVisible ? 0x04 : 0x4000 );

            if ( ( pdib->dirflag & fDIRFavourNext ) || posFirst == pdib->pos )
            {
                //  fDIRFavourNext is only set if we know we want RecordNotFound
                //  if there are no nodes greater than or equal to the one we
                //  want, in which case there's no point going ErrBTPrev().
                //
                OnDebug( ulTrack |= 0x08 );

                wrn = wrnNDFoundGreater;
                err = ErrBTNext( pfucb, fDIRNull );
            }
            else if ( ( pdib->dirflag & fDIRFavourPrev ) || posLast == pdib->pos )
            {
                OnDebug( ulTrack |= 0x10 );

                //  if this is a non-unique index then it is possible that
                //  subsequent nodes could have the same key, in which case
                //  we should land on them. if those nodes don't exist then
                //  we will move to the previous node

                bool fFoundExact = false;
                if ( fSeekOnNonUniqueKeyOnly )
                {
                    OnDebug( ulTrack |= 0x1000 );

                    err = ErrBTNext( pfucb, fDIRNull );
                    if ( JET_errSuccess == err && FKeysEqual( pfucb->kdfCurr.key, pdib->pbm->key ) )
                    {
                        OnDebug( ulTrack |= 0x2000 );

                        // Here we have moved next and landed on a record with the same key.
                        // This counts as a successful seek 
                        fFoundExact = true;
                        err = JET_errSuccess;
                    }
                    else if ( JET_errNoCurrentRecord != err )
                    {
                        // It is possible to move off the end of the tree, but we don't expect
                        // any other errors
                        Call( err );
                    }
                }

                if ( !fFoundExact )
                {
                    // We were unable to find an exact match. Move to the previous node
                    wrn = wrnNDFoundLess;
                    err = ErrBTPrev( pfucb, fDIRNull );
                }
            }
            else if ( ( pdib->dirflag & fDIRExact ) && !fSeekOnNonUniqueKeyOnly )
            {
                OnDebug( ulTrack |= 0x20 );
                err = ErrERRCheck( JET_errRecordNotFound );
            }
            else
            {
                OnDebug( ulTrack |= 0x40 );

                wrn = wrnNDFoundGreater;
                err = ErrBTNext( pfucb, fDIRNull );
                if( JET_errNoCurrentRecord == err )
                {
                    wrn = wrnNDFoundLess;
                    err = ErrBTPrev( pfucb, fDIRNull );
                }
            }
            Call( err );

            //  BTNext/Prev() shouldn't return these warnings
            Assert( wrnNDFoundGreater != err );
            Assert( wrnNDFoundLess != err );

            //  if on a non-unique index and no data portion of the
            //  bookmark was passed in, may need to do a "just the key"
            //  comparison
            if ( fSeekOnNonUniqueKeyOnly )
            {
                OnDebug( ulTrack |= 0x80 );
                const BOOL fKeysEqual = FKeysEqual( pfucb->kdfCurr.key, pdib->pbm->key );
                if ( fKeysEqual )
                {
                    wrn = JET_errSuccess;
                }
                else
                {
#ifdef DEBUG
                    const INT cmp = CmpKey( pfucb->kdfCurr.key, pdib->pbm->key );
                    Assert( ( cmp < 0 && wrnNDFoundLess == wrn )
                        || ( cmp > 0 && wrnNDFoundGreater == wrn ) );
#endif  //  DEBUG
                }
            }
        }
    }
    else
    {
        // we need to continue if the node is deleted and 
        // doesn't have any versions
        if ( pdib->dirflag & fDIRAllNodesNoCommittedDeleted )
        {
            OnDebug( ulTrack |= 0x100 );

            if ( FBTINodeCommittedDelete( pfucb ) )
            {
                if ( posFirst == pdib->pos )
                {
                    OnDebug( ulTrack |= 0x200 );
                    Call( ErrBTNext( pfucb, fDIRAllNodesNoCommittedDeleted | fDIRAllNode ) );
                }
                else if ( posLast == pdib->pos )
                {
                    OnDebug( ulTrack |= 0x400 );
                    Call( ErrBTPrev( pfucb, fDIRAllNodesNoCommittedDeleted | fDIRAllNode ) );
                }
                else
                {
                    AssertSz( fFalse, "fDIRAllNodesNoCommittedDeleted should be used with posFirst or posLast only" );
                }
            }
        }
    }


    if ( posDown == pdib->pos )
    {
        INT cmp;
#ifdef DEBUG
        cmp = ( ( FFUCBUnique( pfucb ) || fSeekOnNonUniqueKeyOnly ) ?
                                 CmpKey( pfucb->kdfCurr.key, bmSeek.key ) :
                                 CmpKeyData( pfucb->kdfCurr, bmSeek ) );
        if ( cmp < 0 )
            Assert( wrnNDFoundLess == wrn );
        else if ( cmp > 0 )
            Assert( wrnNDFoundGreater == wrn );
        else
            CallS( wrn );
#endif

        err = wrn;

#ifdef CHECK_UNIQUE_KEY_ON_NONUNIQUE_INDEX
        if ( fCheckUniqueness && JET_errSuccess == err )
        {
            Assert( fSeekOnNonUniqueKeyOnly );
            if ( pcsr->ILine() < pcsr->Cpage().Clines() - 1 )
            {
                //  we're not on the last node on this page
                //  so check the key of the next node
                //
                cmp     = CmpNDKeyOfNextNode( pcsr, pdib->pbm->key );
                Assert( cmp >= 0 );
                if ( cmp > 0 )
                {
                    //  these asserts must be commented out because they do not handle the case
                    //  of a down operation landing on a page with all flag deleted nodes
                    //  and moving laterally to a page containing the node of interest.
                    //
                    //  Assert( fKeyIsolatedToCurrentPage || pgnoNull == pcsr->Cpage().PgnoNext() );
                    //  Assert( fKeyIsolatedToCurrentPage || pcsr->Cpage().FRootPage() );
                    err = ErrERRCheck( JET_wrnUniqueKey );
                }
            }
            else
            {
                //  we're on the last node of the page, so
                //  best check we can do is to see if it's
                //  possible that the next page may contain
                //  the same key
                //
                //  if we are on the last page the key is unique
                //  (we are on the last record in the entire b-tree)
                //
                if ( fKeyIsolatedToCurrentPage || pgnoNull == pcsr->Cpage().PgnoNext() )
                {
                    err = ErrERRCheck( JET_wrnUniqueKey );
                }
            }
        }
#endif  //  CHECK_UNIQUE_KEY_ON_NONUNIQUE_INDEX
    }


    //  now, the cursor is on leaf node
    //
    Assert( pcsr->Cpage().FLeafPage() );
    AssertBTType( pfucb );
    AssertNDGet( pfucb );
    Assert( err >= 0 );

/// Assert( Pcsr( pfucb )->Latch() == latch );
    return err;

HandleError:

#ifdef  DEBUG
    // Restore RFS and cleanup checking
    FOSSetCleanupState( fCleanUpStateSaved );
    if ( fNeedReEnableRFS )
    {
        RFSThreadReEnable( cRFSCountdownOld );
    }
#endif  //  DEBUG

    Assert( err < 0 );
    BTUp( pfucb );
    if ( JET_errNoCurrentRecord == err )
    {
        err = ErrERRCheck( JET_errRecordNotFound );
    }
    return err;
}


ERR ErrBTPerformOnSeekBM( FUCB * const pfucb, const DIRFLAG dirflag )
{
    ERR     err;
    DIB     dib;

    Assert( locOnSeekBM == pfucb->locLogical );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( fDIRFavourPrev == dirflag
        || fDIRFavourNext == dirflag );

    dib.pbm     = &pfucb->bmCurr;
    dib.pos     = posDown;
    dib.dirflag = dirflag;
    err = ErrBTDown( pfucb, &dib, latchReadNoTouch );
    Assert( JET_errRecordDeleted != err );
    Assert( JET_errNoCurrentRecord != err );
    if ( JET_errRecordNotFound == err )
    {
        //  moved past last node
        //
        pfucb->locLogical = ( fDIRFavourPrev == dirflag ? locBeforeFirst : locAfterLast );
    }

    Assert( err < 0 || Pcsr( pfucb )->FLatched() );
    return err;
}


//  *********************************************
//  direct access routines
//


//  gets position of key by seeking down from root
//  ulTotal is estimated total number of records in tree
//  ulLT is estimated number of nodes lesser than given node
//
ERR ErrBTGetPosition( FUCB *pfucb, ULONGLONG *pullLT, ULONGLONG *pullTotal )
{
    ERR        err;
    ULONGLONG  ullLT = 0;
    ULONGLONG  ullTotal = 1;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTSeek );

    //  no latch should be held by cursor on tree
    //
    Assert( !Pcsr( pfucb )->FLatched() );

    //  if we are on locOnSeekBM then purge our cached key position because it
    //  is incorrect
    //
    if ( pfucb->locLogical == locOnSeekBM )
    {
        pfucb->ullLTCurr = 0;
        pfucb->ullTotalCurr = 0;
    }
    else
    {
        //  this function assumes we have a valid bookmark, and
        //  it is currently only called by ErrDIRGetPosition(),
        //  which guarantees logical currency is either
        //  locOnCurBM or locOnSeekBM (and therefore the
        //  current bookmark is valid)
        //
        Assert( locOnCurBM == pfucb->locLogical );
    }

    //  if we have a cached key position then return that
    //
    if ( pfucb->ullTotalCurr )
    {
        *pullLT = pfucb->ullLTCurr;
        *pullTotal = pfucb->ullTotalCurr;
        return JET_errSuccess;
    }

    PERFOpt( PERFIncCounterTable( cBTSeek, PinstFromPfucb( pfucb ), (TCE)tcScope->nParentObjectClass ) );

    //  go to root
    //
    CallR( ErrBTIGotoRoot( pfucb, latchReadTouch ) );

    //  seek to bookmark key
    //
    for ( ; ; )
    {
        INT     clines = Pcsr( pfucb )->Cpage().Clines();

        //  for every page level, seek to bookmark key
        //
        Call( ErrNDSeek( pfucb, pfucb->bmCurr ) );

        //  adjust number of records and key position
        //  for this tree level
        //  With 64 bit variables, we really don't expect to overflow.
        //
        Expected( ullLT <= ullLT * clines + Pcsr( pfucb )->ILine());
        Expected( ullTotal <= ullTotal * clines );
        ullLT = ullLT * clines + Pcsr( pfucb )->ILine();
        ullTotal = ullTotal * clines;

        if ( !Pcsr( pfucb )->Cpage().FInvisibleSons( ) )
        {
            //  leaf node reached, exit loop
            //
            break;
        }
        else
        {
            //  get pgno of child from node
            //  switch to that page
            //
            Assert( pfucb->kdfCurr.data.Cb() == sizeof( PGNO ) );
            Call( Pcsr( pfucb )->ErrSwitchPage(
                                pfucb->ppib,
                                pfucb->ifmp,
                                *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv(),
                                pfucb->u.pfcb->FNoCache()
                                ) );
        }
    }

    *pullLT = ullLT;
    *pullTotal = ullTotal;
    Assert( ullTotal >= ullLT );

    err = JET_errSuccess;

HandleError:
    //  unlatch the page
    //  do not save logical currency
    //
    BTUp( pfucb );
    return err;
}


//  goes to given bookmark on page [does not check version store]
//  bookmark must have been obtained on a node
//  if bookmark does not exist, returns JET_errRecordNotFound
//  if ExactPos is set, and we can not find node with bookmark == bm
//      we return error
//
ERR ErrBTGotoBookmark( FUCB *pfucb, const BOOKMARK& bm, LATCH latch, BOOL fExactPos )
{
    ERR         err;
    DIB         dib;
    BOOKMARK    bmT = bm;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTSeek );

    Assert( !bm.key.FNull() );
    Assert( !FFUCBUnique( pfucb ) || bm.data.Cb() == 0 );
    Assert( FFUCBUnique( pfucb ) || bm.data.Cb() != 0 );
    Assert( latchReadTouch == latch || latchReadNoTouch == latch ||
            latchRIW == latch );

    //  similar to BTDown
    //  goto Root and seek down using bookmark key and data
    //
    dib.pos     = posDown;
    dib.pbm     = &bmT;
    dib.dirflag = fDIRAllNode | ( fExactPos ? fDIRExact : fDIRNull );

    err = ErrBTDown( pfucb, &dib, latch );
    Assert( err < 0 || Pcsr( pfucb )->Latch() == latch );

    if ( fExactPos
        && ( JET_errRecordNotFound == err
            || wrnNDFoundLess == err
            || wrnNDFoundGreater == err ) )
    {
        //  bookmark does not exist anymore
        //
        BTUp( pfucb );
        Assert( !Pcsr( pfucb )->FLatched() );
        err = ErrERRCheck( JET_errRecordDeleted );
    }

    return err;
}


//  seeks for bookmark in page
//  functionality is similar to ErrBTGotoBookmark
//      looking at all nodes [fDIRAllNode]
//      and seeking for equal
//  returns wrnNDNotInPage if bookmark falls outside page boundary
//
ERR ErrBTISeekInPage( FUCB *pfucb, const BOOKMARK& bmSearch )
{
    ERR     err;

    Assert( Pcsr( pfucb )->FLatched() );

    if ( Pcsr( pfucb )->Cpage().FEmptyPage() ||
         Pcsr( pfucb )->Cpage().FPreInitPage() ||
         !Pcsr( pfucb )->Cpage().FLeafPage() ||
         Pcsr( pfucb )->Cpage().ObjidFDP() != ObjidFDP( pfucb ) ||
         0 == Pcsr( pfucb )->Cpage().Clines() )
    {
        return ErrERRCheck( wrnNDNotFoundInPage );
    }

    Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
    Assert( Pcsr( pfucb )->Cpage().ObjidFDP() == ObjidFDP( pfucb ) );
    Assert( !Pcsr( pfucb )->Cpage().FEmptyPage() );
    Assert( !Pcsr( pfucb )->Cpage().FPreInitPage() );

    err = ErrNDSeek( pfucb, bmSearch );
    AssertRTL( err >= JET_errSuccess ); //  even in corruption case, should be very hard to hit this for where it is used.
    CallR( err );

    if ( wrnNDFoundLess == err &&
             Pcsr( pfucb )->Cpage().Clines() - 1 == Pcsr( pfucb )->ILine() ||
         wrnNDFoundGreater == err &&
             0 == Pcsr( pfucb )->ILine() )
    {
        //  node may be elsewhere if it is not in the range of this page
        //
        err = ErrERRCheck( wrnNDNotFoundInPage );
    }

    return err;
}


//  checks if a given tree contains a specific page
//
ERR ErrBTContainsPage( FUCB* const pfucb, const BOOKMARK& bm, const PGNO pgno, const BOOL fLeafPage )
{
    ERR err = JET_errSuccess;
    CSR* const pcsr = Pcsr( pfucb );
    BOOL fNonUniqueKeys = !FFUCBUnique( pfucb );
    BOOL fLatched = fFalse;
    PGNO pgnoRoot = pgnoNull;
    PGNO pgnoParent = pgnoNull;
    PGNO pgnoCurrent = pgnoNull;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTSeek );

    Assert( !pcsr->FLatched() );
    Assert( !bm.key.FNull() || !fLeafPage );
    Assert( bm.data.FNull() || ( fLeafPage && fNonUniqueKeys ) );

    // Start from the root.
    Call( ErrBTIGotoRoot( pfucb, latchReadTouch ) );
    fLatched = fTrue;

    // The root itself is what we're looking for.
    pgnoRoot = pcsr->Pgno();
    pgnoCurrent = pgnoRoot;
    if ( pgnoCurrent == pgno )
    {
        Expected( fFalse ); // We currently do not have any cases where we look for a root.
        goto HandleError;
    }

    // No nodes in the root, done.
    if ( pcsr->Cpage().Clines() == 0 )
    {
        Error( ErrERRCheck( JET_errRecordNotFound ) );
    }

    // Search for references to pgno in the tree.
    // Because we are only testing for presence, we don't need to actually navigate
    // to the page or a specific node or even latch the page. Therefore, we only
    // need to search the nodes of the parent of the level we're looking for.
    while ( fTrue )
    {
        // If the current page is a leaf or a parent of leaf and we don't need to search the leaf level,
        // we're done.
        if ( pcsr->Cpage().FLeafPage() ||
            ( pcsr->Cpage().FParentOfLeaf() && !fLeafPage ) )
        {
            Error( ErrERRCheck( JET_errRecordNotFound ) );
        }

        // Make sure that the page belongs to the expected object.
        if ( pcsr->Cpage().ObjidFDP() != pfucb->u.pfcb->ObjidFDP() )
        {
            Error( ErrBTIReportBadPageLink(
                        pfucb,
                        ErrERRCheck( JET_errBadParentPageLink ),
                        pgnoParent,
                        pgnoCurrent,
                        pcsr->Cpage().ObjidFDP(),
                        fFalse,
                        "BtContainsObjid" ) );
        }

        // Make sure the page is not empty.
        if ( pcsr->Cpage().Clines() <= 0 )
        {
            Error( ErrBTIReportBadPageLink(
                        pfucb,
                        ErrERRCheck( JET_errBadParentPageLink ),
                        pgnoParent,
                        pgnoCurrent,
                        pcsr->Cpage().ObjidFDP(),
                        fFalse,
                        pcsr->Cpage().FPreInitPage() ?
                            "BtContainsClinesLowPreinit" :
                            ( pcsr->Cpage().FEmptyPage() ?
                                "BtContainsClinesLowEmpty" :
                                "BtContainsClinesLowInPlace" ) ) );
        }

        // Seek using the bookmark/hint provided.
        //
        // IMPORTANT: this lookup code differs from regular BTDown in a few significant ways, due
        // to the fact that the provided bookmark is a key/data pair which is actually present
        // in the page (thus it can be viewed as a hint), as opposed to a leaf-level key/data
        // search key which we want to navigate to. However, note that the provided bookmark may
        // be equivalent to a BTDown-like search key, in the case where we're looking for a leaf page.
        //
        //   1- We'll never get to the actual leaf level of the tree. Because we're looking for
        //      a reference to the page, we would have found a match as we come across its parent.
        //
        //   2- When looking for the next page to navigate to, the direction of navigation differs
        //      from a regular BTDown if we're looking for an internal page, w.r.t. exact matches.
        //      2.1- If we're looking for a leaf page, an exact match demands navigating to the
        //           next right child because the delimiter in an internal page is greater than all
        //           nodes in its subtree. In this case, we'll run the the regular ErrNDSeek() also
        //           used by BTDown.
        //      2.2- If we're looking for an internal page, an exact match demands navigating to the
        //           child pointed to by the node that was exactly matched. In this case, we'll use
        //           ErrNDSeekInternal().
        //
        if ( fLeafPage )
        {
            err = ErrNDSeek( pfucb, bm );
        }
        else
        {
            // If our hint is a null key, we must be going all the way to the right-most
            // branch of the tree.
            if ( bm.key.FNull() )
            {
                KEYDATAFLAGS kdf;
                NDIGetKeydataflags( pcsr->Cpage(), pcsr->Cpage().Clines() - 1, &kdf );
                if ( !kdf.key.FNull() )
                {
                    AssertTrack( false, "BtContainsPageNoNullKey" );
                    Error( ErrBTIReportBadPageLink(
                                pfucb,
                                ErrERRCheck( JET_errBadParentPageLink ),
                                pgnoParent,
                                pgnoCurrent,
                                pcsr->Cpage().ObjidFDP(),
                                fFalse,
                                "BtContainsNullKey" ) );
                }

                pcsr->SetILine( pcsr->Cpage().Clines() - 1 );
                NDGet( pfucb, pcsr );
            }
            else
            {
                err = ErrNDSeekInternal( pfucb, bm );
            }
        }
        Assert( err >= JET_errSuccess );
        Call( err );
        err = JET_errSuccess;

        // Make sure it points to what appears to be a page.
        if ( pfucb->kdfCurr.data.Cb() != sizeof( PGNO ) )
        {
            Error( ErrBTIReportBadPageLink(
                        pfucb,
                        ErrERRCheck( JET_errBadParentPageLink ),
                        pgnoParent,
                        pgnoCurrent,
                        pcsr->Cpage().ObjidFDP(),
                        fFalse,
                        "BtContainsBadPgnoSize" ) );
        }

        const PGNO pgnoChild = *(UnalignedLittleEndian<PGNO>*) pfucb->kdfCurr.data.Pv();

        // Make sure the page doesn't point to itself, back to its parent or the root, or
        // pgnoNull.
        if ( ( pgnoChild == pgnoRoot ) ||
            ( pgnoChild == pgnoParent ) ||
            ( pgnoChild == pgnoCurrent ) ||
            ( pgnoChild == pgnoNull ) )
        {
            Error( ErrBTIReportBadPageLink(
                        pfucb,
                        ErrERRCheck( JET_errBadParentPageLink ),
                        pgnoCurrent,
                        pgnoChild,
                        pcsr->Cpage().ObjidFDP(),
                        fFalse,
                        "BtContainsBadPgnoChild" ) );
        }

        // Check if we found the page.
        if ( pgnoChild == pgno )
        {
            goto HandleError;
        }

        Call( pcsr->ErrSwitchPage( pfucb->ppib, pfucb->ifmp, pgnoChild, fFalse ) );

        pgnoParent = pgnoCurrent;
        pgnoCurrent = pgnoChild;
    }

HandleError:
    if ( fLatched )
    {
        BTUp( pfucb );
        fLatched = fFalse;
    }

    if ( err >= JET_errSuccess )
    {
        err = JET_errSuccess;
    }

    return err;
}


//  counts how many pages are reachable in a given tree.
//
ERR ErrBTIIGetReachablePageCount( FUCB* const pfucb, CSR* pcsr, CPG* const pcpg )
{
    ERR err = JET_errSuccess;
    const INT clines = pcsr->Cpage().Clines();
    CSR csrChild;

    Expected( FFUCBSpace( pfucb ) );  // Calling this on a large tree might be too expensive.
    Assert( pcsr->FLatched() );
    Assert( !pcsr->Cpage().FEmptyPage() );
    Assert( !pcsr->Cpage().FLeafPage() );
    Assert( clines > 0 );

    // Accumulate the number of children.
    ( *pcpg ) += clines;

    if ( pcsr->Cpage().FParentOfLeaf() )
    {
        goto HandleError;
    }

    const CPG cpgSubtreePrereadMax = 16;
    PGNO rgpgnoSubtree[ cpgSubtreePrereadMax + 1 ] = { pgnoNull };

    // Preread and process each child.
    for ( INT ilineToProcess = 0, ilinePrereadFirst = -1, ilinePrereadLast = -1;
            ilineToProcess < clines;
            ilineToProcess++ )
    {
        // Check if we need to preread.
        if ( ilineToProcess > ilinePrereadLast )
        {
            Assert( ilineToProcess == ( ilinePrereadLast + 1 ) );

            ilinePrereadFirst = ilineToProcess;
            ilinePrereadLast = min( ilinePrereadFirst + cpgSubtreePrereadMax - 1, clines - 1 );
            Assert( ilinePrereadFirst <= ilinePrereadLast );
            Assert( ilinePrereadLast < clines );

            // Accumulate pgnos to preread.
            for ( INT ilineToPreread = ilinePrereadFirst; ilineToPreread <= ilinePrereadLast; ilineToPreread++ )
            {
                pcsr->SetILine( ilineToPreread );
                NDGet( pfucb, pcsr );
                const PGNO pgnoToPreread = *( (UnalignedLittleEndian<PGNO>*)pfucb->kdfCurr.data.Pv() );
                Assert( pgnoToPreread != pgnoNull );
                rgpgnoSubtree[ ilineToPreread - ilinePrereadFirst ] = pgnoToPreread;
            }
            rgpgnoSubtree[ ilinePrereadLast - ilinePrereadFirst + 1 ] = pgnoNull;   // Terminator.

            // Issue preread.
            BFPrereadPageList( pfucb->ifmp, rgpgnoSubtree, bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->ifmp ), TcCurr() );
        }

        Assert( ilinePrereadFirst >= 0 );
        Assert( ilineToProcess >= ilinePrereadFirst );
        Assert( ilineToProcess <= ilinePrereadLast );
        Assert( ( ilineToProcess - ilinePrereadFirst ) < cpgSubtreePrereadMax );

        // Latch and process the page.
        const PGNO pgnoToProcess = rgpgnoSubtree[ ilineToProcess - ilinePrereadFirst ];
        Assert( pgnoToProcess != pgnoNull );
        Call( csrChild.ErrGetReadPage( pfucb->ppib, pfucb->ifmp, pgnoToProcess, BFLatchFlags( bflfNoTouch ) ) );
        Call( ErrBTIIGetReachablePageCount( pfucb, &csrChild, pcpg ) );
        csrChild.ReleasePage();
        csrChild.Reset();
    }

HandleError:
    if ( csrChild.FLatched() )
    {
        csrChild.ReleasePage();
        csrChild.Reset();
    }

    if ( err >= JET_errSuccess )
    {
        err = JET_errSuccess;
    }

    Assert( pcsr->FLatched() );

    return err;
}


//  counts how many pages are reachable in a given tree.
//
ERR ErrBTIGetReachablePageCount( FUCB* const pfucb, CPG* const pcpg )
{
    ERR err = JET_errSuccess;
    CSR* pcsr = Pcsr( pfucb );
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTUtility );

    Expected( FFUCBSpace( pfucb ) || ( ObjidFDP( pfucb ) == objidSystemRoot ) );  // Calling this on a large tree might be too expensive.
    Assert( pcsr->FLatched() );
    Assert( pcsr == pfucb->pcsrRoot );
    Assert( pcsr->Cpage().FRootPage() );

    *pcpg = 1;

    // Only proceed if we haven't reached the leaf level yet.
    if ( pcsr->Cpage().FLeafPage() )
    {
        goto HandleError;
    }

    // We are already positioned at the root. Proceed with recursion.
    Call( ErrBTIIGetReachablePageCount( pfucb, pcsr, pcpg ) );

HandleError:
    if ( err >= JET_errSuccess )
    {
        err = JET_errSuccess;
    }

    Assert( pcsr->FLatched() );

    return err;
}


//  ******************************************************
//  UPDATE OPERATIONS
//

//  lock record. this does not lock anything, we do not set
//  the version bit and the page is not write latched
//
//  UNDONE: we don't need to latch the page at all. just create
//          the version using the bookmark in the FUCB
//
ERR ErrBTLock( FUCB *pfucb, DIRLOCK dirlock )
{
    Assert( dirlock == writeLock
            || dirlock == readLock );

    const BOOL  fVersion = !g_rgfmp[pfucb->ifmp].FVersioningOff() && !pfucb->u.pfcb->FVersioningOffForExtentPageCountCache();
    Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() );

    ERR     err = JET_errSuccess;

    // UNDONE: If versioning is disabled, so is locking.
    if ( fVersion )
    {
        OPER oper = 0;
        switch( dirlock )
        {
            case writeLock:
                oper = operWriteLock;
                break;
            case readLock:
                oper = operReadLock;
                break;
            default:
                Assert( fFalse );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
                break;
        }

        RCE *prce = prceNil;
        VER *pver = PverFromIfmp( pfucb->ifmp );
        Call( pver->ErrVERCheckTransactionSize( pfucb->ppib ) );
        Call( pver->ErrVERModify(
                pfucb,
                pfucb->bmCurr,
                oper,
                &prce,
                NULL ) );
        Assert( prceNil != prce );
        VERInsertRCEIntoLists( pfucb, pcsrNil, prce, NULL );
    }
    Assert( !Pcsr( pfucb )->FLatched() );

HandleError:
    return err;
}

//  if all prefix-compressed nodes have been removed set the prefix to NULL
//  if there is only one record left and it is compressed, then shrink the prefix to give
//  room in the page for that one record to grow to the maximum size.

LOCAL ERR ErrBTIResetPrefixIfUnused( FUCB * const pfucb, CSR * const pcsr )
{
    ERR err = JET_errSuccess;

    // root pages aren't prefix compressed
    if ( pcsr->Cpage().FRootPage() )
    {
        return JET_errSuccess;
    }

    KEYDATAFLAGS kdfPrefix;
    NDGetExternalHeader( &kdfPrefix, pcsr, noderfWhole );

    const bool fPageHasPrefix = ( 0 != kdfPrefix.data.Cb() );
    const bool fAnyNodeIsCompressed = FNDAnyNodeIsCompressed( pcsr->Cpage() );

    if ( fAnyNodeIsCompressed )
    {
        Assert( fPageHasPrefix );

        if ( pcsr->Cpage().Clines() == 1 )
        {
            KEYDATAFLAGS kdf;
            NDIGetKeydataflags( pcsr->Cpage(), 0, &kdf );
            Assert( FNDCompressed( kdf ) );
            Assert( kdf.key.prefix.Cb() <= kdfPrefix.data.Cb() );

            //  truncate the prefix of unneeded bytes
            if ( kdf.key.prefix.Cb() < kdfPrefix.data.Cb() )
            {
                DATA dataShrunkenPrefix;
                dataShrunkenPrefix.SetPv( kdfPrefix.data.Pv() );
                dataShrunkenPrefix.SetCb( kdf.key.prefix.Cb() );

                Call( ErrNDSetExternalHeader( pfucb, pcsr, &dataShrunkenPrefix, fDIRNull, noderfWhole ) );
            }
        }
    }
    else if ( fPageHasPrefix )
    {
        // There aren't any prefix-compressed nodes, but the page does have 
        // a prefix. Remove it.
        DATA dataNull;
        dataNull.Nullify();
        Call( ErrNDSetExternalHeader( pfucb, pcsr, &dataNull, fDIRNull, noderfWhole ) );
    }

HandleError:
    return err;
}

//  replace data of current node with given data
//      if necessary, split before replace
//  doesn't take a proxy because it isn't used by concurrent create index
//
ERR ErrBTReplace( FUCB * const pfucb, const DATA& data, const DIRFLAG dirflag )
{
    Assert( !FFUCBSpace( pfucb ) || fDIRNoVersion & dirflag );

    Assert( FBTLogicallyNavigableState( pfucb ) );

    ERR     err;
    LATCH   latch       = latchReadNoTouch;
    INT     cbDataOld   = 0;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTReplace );

    PERFOpt( PERFIncCounterTable( cBTReplace, PinstFromPfucb( pfucb ), (TCE)tcScope->nParentObjectClass ) );

    //  save bookmark
    //
    if ( Pcsr( pfucb )->FLatched() )
    {
        CallR( ErrBTISaveBookmark( pfucb ) );
    }

#ifdef DEBUG

    //  Disable RFS and cleanup checking
    //  It's not a real failure path because it's debug only code
    //
    const LONG cRFSCountdownOld = RFSThreadDisable( 0 );
    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

    CAutoKey Key, Data, Key2;

    //  Restore RFS and cleanup checking
    //
    FOSSetCleanupState( fCleanUpStateSaved );
    RFSThreadReEnable( cRFSCountdownOld );

    INT cbKey = 0;
    DATA dataBM;

    if ( Key && Data && Key2 )
    {
        const BOOKMARK* const pbmCur = &pfucb->bmCurr;
        cbKey = pbmCur->key.Cb();
        pbmCur->key.CopyIntoBuffer( Key, cbKey );

        dataBM.SetPv( Data );
        pbmCur->data.CopyInto( dataBM );
    }
#endif

    const BOOL      fVersion        = !( dirflag & fDIRNoVersion ) && !g_rgfmp[pfucb->ifmp].FVersioningOff();
    Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() );

    RCE * prceReplace = prceNil;
    RCEID rceidReplace = rceidNull;

Start:
    CallR( ErrBTIRefresh( pfucb, latch ) );
    AssertNDGet( pfucb );

    //  non-unique trees have nodes in key-data order
    //  so to replace data, we need to delete and re-insert
    //
    Assert( FFUCBUnique( pfucb ) );

#ifdef DEBUG
    if ( ( latchReadTouch == latch || latchReadNoTouch == latch )
        && ( latchReadTouch == Pcsr( pfucb )->Latch() || latchReadNoTouch == Pcsr( pfucb )->Latch() ) )
    {
        //  fine!
    }
    else if ( latch == Pcsr( pfucb )->Latch() )
    {
        //  fine!
    }
    else
    {
        Assert( latchWrite == Pcsr( pfucb )->Latch() );
        Assert( FFUCBSpace( pfucb ) );
    }

    if ( Key && Data && Key2 )
    {
        const BOOKMARK* const pbmCur = &pfucb->bmCurr;
        pbmCur->key.CopyIntoBuffer( Key2, pbmCur->key.Cb() );

        Assert( pbmCur->key.Cb() == cbKey );
        Assert( memcmp( Key, Key2, cbKey ) == 0 );
        Assert( pbmCur->data.Cb() == dataBM.Cb() );
        Assert( memcmp( pbmCur->data.Pv(), dataBM.Pv(), dataBM.Cb() ) == 0 );
    }
#endif

    if ( latchWrite != Pcsr( pfucb )->Latch() )
    {
        //  upgrade latch
        //

        err = Pcsr( pfucb )->ErrUpgrade();

        if ( err == errBFLatchConflict )
        {
            Assert( !Pcsr( pfucb )->FLatched() );
            latch = latchRIW;
            goto Start;
        }
        Call( err );
    }

    Assert( latchWrite == Pcsr( pfucb )->Latch() );

    if( fVersion )
    {
        Assert( prceNil == prceReplace );
        VER *pver = PverFromIfmp( pfucb->ifmp );
        Call( pver->ErrVERCheckTransactionSize( pfucb->ppib ) );
        Call( pver->ErrVERModify( pfucb, pfucb->bmCurr, operReplace, &prceReplace, NULL ) );
        Assert( prceNil != prceReplace );
        rceidReplace = Rceid( prceReplace );
    }

#ifdef DEBUG
    USHORT  cbUncFreeDBG;
    cbUncFreeDBG = Pcsr( pfucb )->Cpage().CbUncommittedFree();
#endif

    //  try to replace node data with new data
    //
    cbDataOld = pfucb->kdfCurr.data.Cb();
    err = ErrNDReplace( pfucb, &data, dirflag, rceidReplace, prceReplace );
    // someone left prefix behind even though there was only a single node
    // clean it up
    //
    if ( errPMOutOfPageSpace == err &&
         Pcsr( pfucb )->Cpage().Clines() == 1 )
    {
        Call( ErrBTIResetPrefixIfUnused( pfucb, Pcsr( pfucb ) ) );
        err = ErrNDReplace( pfucb, &data, dirflag, rceidReplace, prceReplace );
    }

    if ( errPMOutOfPageSpace == err )
    {
        const INT   cbData      = data.Cb();
        Assert( cbData >= 0 );
        const INT   cbReq       = data.Cb() - cbDataOld;
        Assert( cbReq > 0 );

        KEYDATAFLAGS    kdf;

        //  node replace causes page overflow
        //
        Assert( data.Cb() > pfucb->kdfCurr.data.Cb() );

        AssertNDGet( pfucb );

        kdf.Nullify();
        kdf.data = data;
        Assert( 0 == kdf.fFlags );

        //  call split repeatedly till replace succeeds
        //
        err = ErrBTISplit(
                    pfucb,
                    &kdf,
                    dirflag | fDIRReplace,
                    rceidReplace,
                    rceidNull,
                    prceReplace,
                    cbDataOld,
                    NULL );

        if ( errBTOperNone == err )
        {
            //  split was performed
            //  but replace did not succeed
            //  retry replace
            //
            Assert( !Pcsr( pfucb )->FLatched() );
            prceReplace = prceNil;  //  the version was nullified in ErrBTISplit
            latch = latchRIW;
            goto Start;
        }
    }

    //  this is the error either from ErrNDReplace() or from ErrBTISplit()
    Call( err );

    AssertBTGet( pfucb );

    if( prceNil != prceReplace )
    {
        Assert( fVersion );
        VERInsertRCEIntoLists( pfucb, Pcsr( pfucb ), prceReplace, NULL );
    }
    else
    {
        Assert( !fVersion );
    }

    return err;

HandleError:
    Assert( err < 0 );
    if( prceNil != prceReplace )
    {
        Assert( fVersion );
        VERNullifyFailedDMLRCE( prceReplace );
    }
    BTUp( pfucb );  //  UNDONE:  is this correct?

    return err;
}


//  performs a delta operation on current node at specified offset
//
template< typename TDelta >
ERR ErrBTDelta(
    FUCB*           pfucb,
    INT             cbOffset,
    const TDelta    tDelta,
    TDelta          *const ptOldValue,
    DIRFLAG         dirflag )
{
    ERR         err;
    LATCH       latch       = latchReadNoTouch;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTReplace );

    Assert( !FFUCBSpace( pfucb ) || fDIRNoVersion & dirflag );
    Assert( cbOffset >= 0 );

    //  can't normally come into this function with the page latched,
    //  because we may have to wait on a wait-lock when we go to create
    //  the Delta RCE, but the exception is a delta on the LV tree,
    //  which we know won't conflict with a wait-lock
    Assert( locOnCurBM == pfucb->locLogical );
    if ( Pcsr( pfucb )->FLatched() )
    {
        Assert( pfucb->u.pfcb->FTypeLV() );
        CallR( ErrBTISaveBookmark( pfucb ) );
    }

#ifdef DEBUG
    CAutoKey Key, Data, Key2;
    INT cbKey = 0;
    DATA dataBM;

    if ( Key && Data && Key2 )
    {
        const BOOKMARK* const pbmCur = &pfucb->bmCurr;
        cbKey = pbmCur->key.Cb();
        pbmCur->key.CopyIntoBuffer( Key, cbKey );

        dataBM.SetPv( Data );
        pbmCur->data.CopyInto( dataBM );
    }
#endif

    const BOOL      fVersion        = !( dirflag & fDIRNoVersion ) && !g_rgfmp[pfucb->ifmp].FVersioningOff();
    Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() );

    RCE*            prce            = prceNil;
    RCEID           rceid           = rceidNull;

    //  create delta RCE first but can't put in true delta yet
    //  because the page operation hasn't occurred yet
    //  (meaning other threads could still come in
    //  before we're able to obtain the write-latch and
    //  would calculate the wrong versioned delta
    //  because they would take into account this delta
    //  and erroneously compensate for it assuming it
    //  has already occurred on the page), so initially
    //  put in a zero-delta and then update RCE once
    //  we've done the node operation
    if( fVersion )
    {
        _VERDELTA< TDelta > verdelta;
        verdelta.tDelta             = 0;        //  this will be set properly after the node operation
        verdelta.cbOffset           = (USHORT)cbOffset;
        verdelta.fDeferredDelete    = fFalse;   //  this will be set properly after the node operation
        verdelta.fCallbackOnZero    = fFalse;   //  this will be set properly after the node operation
        verdelta.fDeleteOnZero      = fFalse;   //  this will be set properly after the node operation

        const KEYDATAFLAGS kdfSave = pfucb->kdfCurr;

        pfucb->kdfCurr.data.SetPv( &verdelta );
        pfucb->kdfCurr.data.SetCb( sizeof( _VERDELTA< TDelta > ) );

        VER *pver = PverFromIfmp( pfucb->ifmp );
        Call( pver->ErrVERCheckTransactionSize( pfucb->ppib ) );
        err = pver->ErrVERModify( pfucb, pfucb->bmCurr, _VERDELTA< TDelta >::TRAITS::oper, &prce, NULL );

        pfucb->kdfCurr = kdfSave;

        Call( err );

        Assert( prceNil != prce );
        rceid = Rceid( prce );
        Assert( rceidNull != rceid );
    }

Start:
    Call( ErrBTIRefresh( pfucb, latch ) );
    AssertNDGet( pfucb );

#ifdef DEBUG
    if ( Key && Data && Key2 )
    {
        const BOOKMARK* const pbmCur = &pfucb->bmCurr;
        pbmCur->key.CopyIntoBuffer( Key2, pbmCur->key.Cb() );

        Assert( pbmCur->key.Cb() == cbKey );
        Assert( memcmp( Key, Key2, cbKey ) == 0 );
        Assert( pbmCur->data.Cb() == dataBM.Cb() );
        Assert( memcmp( pbmCur->data.Pv(), dataBM.Pv(), dataBM.Cb() ) == 0 );
    }
#endif

    Assert( ( ( latchReadTouch == latch || latchReadNoTouch == latch ) &&
              ( latchReadTouch == Pcsr( pfucb )->Latch() ||
                latchReadNoTouch == Pcsr( pfucb )->Latch() ) ) ||
            latch == Pcsr( pfucb )->Latch() );

    err = Pcsr( pfucb )->ErrUpgrade();

    if ( err == errBFLatchConflict )
    {
        Assert( !Pcsr( pfucb )->FLatched() );
        latch = latchRIW;
        goto Start;
    }
    Call( err );

    //  try to replace node data with new data
    //  we need to store the old value
    TDelta tOldValue;
    Call( ErrNDDelta< TDelta >( pfucb, Pcsr( pfucb ), cbOffset, tDelta, &tOldValue, dirflag, rceid ) );
    if( ptOldValue )
    {
        *ptOldValue = tOldValue;
    }

    if( prceNil != prce )
    {
        _VERDELTA< TDelta >* const      pverdelta       = (_VERDELTA< TDelta > *)prce->PbData();

        Assert( fVersion );
        Assert( rceidNull != rceid );
        Assert( Pcsr( pfucb )->FLatched() );

        pverdelta->tDelta = tDelta;

        //  if the refcount went to zero we may need to set a flag in the RCE
        if ( 0 == ( tDelta + tOldValue ) )
        {
            if( dirflag & fDIRDeltaDeleteDereferencedLV )
            {
                pverdelta->fDeferredDelete = fTrue;
            }
            else
            {
                if( dirflag & fDIREscrowCallbackOnZero )
                {
                    pverdelta->fCallbackOnZero = fTrue;
                }
                if ( dirflag & fDIREscrowDeleteOnZero )
                {
                    pverdelta->fDeleteOnZero = fTrue;
                }
            }
        }

        VERInsertRCEIntoLists( pfucb, Pcsr( pfucb ), prce, NULL );
    }
    else
    {
        Assert( !fVersion );
    }

    return err;

HandleError:
    Assert( err < 0 );
    if( prceNil != prce )
    {
        Assert( fVersion );
        VERNullifyFailedDMLRCE( prce );
    }

    BTUp( pfucb );
    return err;
}

//  inserts key and data into tree
//  if inserted node does not fit into leaf page, split page and insert
//  if tree does not allow duplicates,
//      check that there are no duplicates in the tree
//      and also block out other inserts with the same key
//
ERR ErrBTInsert(
    FUCB            *pfucb,
    const KEY&      key,
    const DATA&     data,
    DIRFLAG         dirflag,
    RCE             *prcePrimary )
{
    ERR             err;
    BOOKMARK        bmSearch;
    ULONG           cbReq;
    BOOL            fInsert;
    const BOOL      fUnique         = FFUCBUnique( pfucb );
    const BOOL      fVersion        = !( dirflag & fDIRNoVersion ) && !g_rgfmp[pfucb->ifmp].FVersioningOff();
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTInsert );

    Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() );
    Assert( FBTLogicallyNavigableState( pfucb ) );

    INT             cbDataOld       = 0;
    BOOKMARK        bookmark;
    bookmark.key = key;
    if( fUnique )
    {
        bookmark.data.Nullify();
    }
    else
    {
        bookmark.data = data;
    }

    RCE             *prceInsert     = prceNil;
    RCE             *prceReplace    = prceNil;

    RCEID           rceidInsert     = rceidNull;
    RCEID           rceidReplace    = rceidNull;

    VERPROXY        verproxy;
    if ( prceNil != prcePrimary )
    {
        verproxy.prcePrimary    = prcePrimary;
        verproxy.proxy          = proxyCreateIndex;
    }

    const VERPROXY * const pverproxy = ( prceNil != prcePrimary ) ? &verproxy : NULL;

    KEYDATAFLAGS    kdf;
    LATCH           latch   = latchReadTouch;

    if ( fVersion )
    {
        VER *pver = PverFromIfmp( pfucb->ifmp );
        Call( pver->ErrVERCheckTransactionSize( pfucb->ppib ) );
        err = pver->ErrVERModify( pfucb, bookmark, operPreInsert, &prceInsert, pverproxy );
        if( JET_errWriteConflict == err )
        {
            //  insert never returns a writeConflict, turn it into a keyDuplicate
            err = ErrERRCheck( JET_errKeyDuplicate );
        }
        Call( err );
        Assert( prceInsert );
        rceidInsert = Rceid( prceInsert );
    }

    Assert( !Pcsr( pfucb )->FLatched( ) );
    Assert( !FFUCBSpace( pfucb ) || fDIRNoVersion & dirflag );

    PERFOpt( PERFIncCounterTable( cBTInsert, PinstFromPfucb( pfucb ), (TCE)tcScope->nParentObjectClass ) );

Seek:
    PERFOpt( PERFIncCounterTable( cBTSeek, PinstFromPfucb( pfucb ), (TCE)tcScope->nParentObjectClass ) );

    //  set kdf to insert
    //

    kdf.data    = data;
    kdf.key     = key;
    kdf.fFlags  = 0;
    ASSERT_VALID( &kdf );

    NDGetBookmarkFromKDF( pfucb, kdf, &bmSearch );

    //  no page should be latched
    //
    Assert( !Pcsr( pfucb )->FLatched( ) );

    //  go to root
    //
    Call( ErrBTIGotoRoot( pfucb, latch ) );
    Assert( 0 == Pcsr( pfucb )->ILine() );

    if ( Pcsr( pfucb )->Cpage().Clines() == 0 )
    {
        //  page is empty
        //  set error so we insert at current iline
        //
        Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
        pfucb->kdfCurr.Nullify();
        err = ErrERRCheck( wrnNDFoundGreater );
    }
    else
    {
        //  seek down tree for key alone
        //
        for ( ; ; )
        {
            //  for every page level, seek to bmSearch
            //  if internal page,
            //      get child page
            //      move cursor to new page
            //      release parent page
            //
            Call( ErrNDSeek( pfucb, bmSearch ) );

            if ( !Pcsr( pfucb )->Cpage().FInvisibleSons( ) )
            {
                //  leaf node reached, exit loop
                //
                break;
            }
            else
            {
                //  get pgno of child from node
                //  switch to that page
                //
                Assert( pfucb->kdfCurr.data.Cb() == sizeof( PGNO ) );
                Call( Pcsr( pfucb )->ErrSwitchPage(
                            pfucb->ppib,
                            pfucb->ifmp,
                            *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv(),
                            pfucb->u.pfcb->FNoCache() ) );
                Assert( Pcsr( pfucb )->Cpage().Clines() > 0 );
            }
        }
    }

Insert:
    kdf.data    = data;
    kdf.key     = key;
    kdf.fFlags  = 0;
    ASSERT_VALID( &kdf );

    //  now, the cursor is on leaf node
    //
    Assert( Pcsr( pfucb )->Cpage().FLeafPage() );

    if ( JET_errSuccess == err )
    {
        if ( fUnique )
        {
            Assert( FKeysEqual( kdf.key, pfucb->kdfCurr.key ) );

            if( !FNDDeleted( pfucb->kdfCurr ) )
            {
                //  this is either committed by another transaction that committed before we began
                //  or we inserted this key ourselves earlier
                Error( ErrERRCheck( JET_errKeyDuplicate ) );
            }
#ifdef DEBUG
            else
            {
                Assert( !FNDPotVisibleToCursor( pfucb ) );  //  should have gotten a JET_errWriteConflict when we created the RCE
            }
#endif  //  DEBUG


            cbReq = kdf.data.Cb() > pfucb->kdfCurr.data.Cb() ?
                        kdf.data.Cb() - pfucb->kdfCurr.data.Cb() :
                        0;
        }
        else
        {
            Assert( 0 == CmpKeyData( bmSearch, pfucb->kdfCurr ) );

            //  can not have two nodes with same key-data
            //
            if ( !FNDDeleted( pfucb->kdfCurr ) )
            {
                // Only way to get here is if a multi-valued index column
                // caused us to generate the same key for the record.
                // This would have happened if the multi-values are non-
                // unique, or if we ran out of keyspace before we could
                // evaluate the unique portion of the multi-values.
                Error( ErrERRCheck( JET_errMultiValuedIndexViolation ) );
            }
#ifdef DEBUG
            else
            {
                Assert( !FNDPotVisibleToCursor( pfucb ) );  //  should have gotten a JET_errWriteConflict when we created the RCE
            }
#endif  //  DEBUG

            //  flag insert node
            //
            cbReq   = 0;
        }

        //  flag insert node and replace data atomically
        //
        fInsert = fFalse;
    }
    else
    {
        //  error is from ErrNDSeek
        //
        if ( wrnNDFoundLess == err )
        {
            //  inserted key-data falls past last node on cursor
            //
            Assert( Pcsr( pfucb )->Cpage().Clines() - 1
                        == Pcsr( pfucb )->ILine() );
            Assert( Pcsr( pfucb )->Cpage().Clines() == 0 ||
                    !fUnique && CmpKeyData( pfucb->kdfCurr, bmSearch ) < 0 ||
                    fUnique && CmpKey( pfucb->kdfCurr.key, bmSearch.key ) < 0 );

            Pcsr( pfucb )->IncrementILine();
        }

        //  calculate key prefix
        //
        BTIComputePrefix( pfucb, Pcsr( pfucb ), key, &kdf );
        Assert( !FNDCompressed( kdf ) || kdf.key.prefix.Cb() > 0 );

        cbReq = CbNDNodeSizeCompressed( kdf );
        fInsert = fTrue;
    }

    if( !fInsert && fUnique && fVersion )
    {
        Assert( prceNil != prceInsert );
        Assert( prceNil == prceReplace );
        VER *pver = PverFromIfmp( pfucb->ifmp );
        Call( pver->ErrVERCheckTransactionSize( pfucb->ppib ) );
        Call( pver->ErrVERModify( pfucb, bookmark, operReplace, &prceReplace, pverproxy ) );
        Assert( prceNil != prceReplace );
        rceidReplace = Rceid( prceReplace );
        cbDataOld = pfucb->kdfCurr.data.Cb();
    }
    else
    {
        Assert( rceidNull == rceidReplace );
    }

#ifdef DEBUG
    USHORT  cbUncFreeDBG;
    cbUncFreeDBG = Pcsr( pfucb )->Cpage().CbUncommittedFree();
#endif

    //  cursor is at insertion point
    //
    //  check if node fits in page
    //
    const BOOL fShouldAppend = FBTIAppend( pfucb, Pcsr( pfucb ), cbReq );
    BOOL fSplitRequired = FBTISplit( pfucb, Pcsr( pfucb ), cbReq );
    if ( fSplitRequired && !fInsert && fUnique )
    {
        // This operation will be a FlagInsertAndReplaceData which means it can
        // reclaim uncommitted free space. If we don't check this then we can
        // end up trying to split a page with only one node because all the free
        // space on the page is reserved by the flag-deleted node.
        //
        // If we can reclaim the space then we won't split and instead will go to
        // the insert case where ErrNDFlagInsertAndReplaceData will call VERSetCbAdjust.
        const ULONG cbReserved = CbNDReservedSizeOfNode( pfucb, Pcsr( pfucb ) );
        fSplitRequired = (ULONG)cbReq >
            cbReserved + Pcsr( pfucb )->Cpage().CbPageFree() - Pcsr( pfucb )->Cpage().CbUncommittedFree();
    }

    if ( fShouldAppend || fSplitRequired )
    {
        Assert( fUnique || fInsert );

#ifdef PREREAD_SPACETREE_ON_SPLIT
        if( pfucb->u.pfcb->FPreread() )
        {
            //  PREREAD space tree while we determine what to split (we WILL need the AE and possibly the OE)
            BTPrereadSpaceTree( pfucb );
        }
#endif  //  PREREAD_SPACETREE_ON_SPLIT

        //  re-adjust kdf to not contain prefix info
        //
        kdf.key     = key;
        kdf.data    = data;
        kdf.fFlags  = 0;

        //  split and insert in tree
        //
        err = ErrBTISplit(
                    pfucb,
                    &kdf,
                    dirflag | ( fInsert ? fDIRInsert : fDIRFlagInsertAndReplaceData ),
                    rceidInsert,
                    rceidReplace,
                    prceReplace,
                    cbDataOld,
                    pverproxy
                    );

        if ( errBTOperNone == err )
        {
            //  insert was not performed
            //  retry insert
            //
            Assert( !Pcsr( pfucb )->FLatched() );

            //  the RCE was nullified in ErrBTISplit
            rceidReplace = rceidNull;
            prceReplace = prceNil;
            latch = latchRIW;
            goto Seek;
        }

        Call( err );
    }
    else
    {
        //  upgrade latch to write
        //
        PGNO    pgno = Pcsr( pfucb )->Pgno();

        err = Pcsr( pfucb )->ErrUpgrade();

        if ( err == errBFLatchConflict )
        {
            //  upgrade conflicted
            //  we lost our read latch
            //
            Assert( !Pcsr( pfucb )->FLatched( ) );

            Call( Pcsr( pfucb )->ErrGetPage(
                        pfucb->ppib,
                        pfucb->ifmp,
                        pgno,
                        latch
                        ) );

            Call( ErrBTISeekInPage( pfucb, bmSearch ) );

            if ( wrnNDNotFoundInPage != err )
            {
                //  we have re-seeked to the insert position in page
                //
                Assert( JET_errSuccess == err ||
                        wrnNDFoundLess == err ||
                        wrnNDFoundGreater == err );

                //  if necessary, we will recreate this so remove it now
                if( prceNil != prceReplace )
                {
                    Assert( prceNil != prceInsert );
                    VERNullifyFailedDMLRCE( prceReplace );
                    rceidReplace = rceidNull;
                    prceReplace = prceNil;
                }

                latch = latchRIW;
                goto Insert;
            }

            //  if necessary, we will recreate this so remove it now

            if( prceNil != prceReplace )
            {
                Assert( prceNil != prceInsert );
                VERNullifyFailedDMLRCE( prceReplace );
                rceidReplace = rceidNull;
                prceReplace = prceNil;
            }

            //  reseek from root for insert

            BTUp( pfucb );

            latch = latchRIW;
            goto Seek;
        }

        Call( err );
        Assert( !FBTIAppend( pfucb, Pcsr( pfucb ), cbReq, fFalse ) );

        if ( fInsert )
        {
            err = ErrNDInsert( pfucb, &kdf, dirflag, rceidInsert, pverproxy );
        }
        else if ( fUnique )
        {
            Assert( FKeysEqual( kdf.key, pfucb->kdfCurr.key ) );
            err = ErrNDFlagInsertAndReplaceData(
                        pfucb,
                        &kdf,
                        dirflag,
                        rceidInsert,
                        rceidReplace,
                        prceReplace,
                        pverproxy );
        }
        else
        {
            err = ErrNDFlagInsert( pfucb, dirflag, rceidInsert, pverproxy );
        }
        Assert ( errPMOutOfPageSpace != err );
        Call( err );
    }

    Assert( err >= 0 );
    Assert( Pcsr( pfucb )->FLatched( ) );
    if( prceNil != prceInsert )
    {
        Assert( fVersion );

        prceInsert->ChangeOper( operInsert );
        VERInsertRCEIntoLists( pfucb, Pcsr( pfucb ), prceInsert, pverproxy );
    }
#ifdef DEBUG
    else
    {
        Assert( !fVersion );
    }
#endif  //  DEBUG

    if( prceNil != prceReplace )
    {
        Assert( fVersion );
        Assert( prceNil != prceInsert );
        VERInsertRCEIntoLists( pfucb, Pcsr( pfucb ), prceReplace, pverproxy );
    }

HandleError:
    if ( err < 0 )
    {
        if( prceNil != prceReplace )
        {
            Assert( fVersion );
            Assert( prceNil != prceInsert );
            VERNullifyFailedDMLRCE( prceReplace );
        }
        if( prceNil != prceInsert )
        {
            Assert( fVersion );
            VERNullifyFailedDMLRCE( prceInsert );
        }
        BTUp( pfucb );
    }

    return err;
}


//  append a node to the end of the  latched page
//  if node insert violates density constraint
//      split and insert
//
ERR ErrBTAppend( FUCB           *pfucb,
                 const KEY&     key,
                 const DATA&    data,
                 DIRFLAG        dirflag )
{
    const BOOL      fUnique         = FFUCBUnique( pfucb );
    const BOOL      fVersion        = !( dirflag & fDIRNoVersion ) && !g_rgfmp[pfucb->ifmp].FVersioningOff();
    Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() );

    Assert( FBTLogicallyNavigableState( pfucb ) );

    ERR             err;
    KEYDATAFLAGS    kdf;
    ULONG           cbReq;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTAppend );

Retry:

    Assert( !FFUCBSpace( pfucb ) || fDIRNoVersion & dirflag );
    Assert( Pcsr( pfucb )->FLatched() &&
            latchWrite == Pcsr( pfucb )->Latch() );
    Assert( pgnoNull == Pcsr( pfucb )->Cpage().PgnoNext() );
    Assert( Pcsr( pfucb )->Cpage().FLeafPage( ) );

    PERFOpt( PERFIncCounterTable( cBTAppend, PinstFromPfucb( pfucb ), (TCE)tcScope->nParentObjectClass ) );

    BOOKMARK    bookmark;
    bookmark.key = key;
    if( fUnique )
    {
        bookmark.data.Nullify();
    }
    else
    {
        bookmark.data = data;
    }

    RCE * prceInsert = prceNil;
    if ( fVersion )
    {
        VER *pver = PverFromIfmp( pfucb->ifmp );
        Call( pver->ErrVERCheckTransactionSize( pfucb->ppib ) );
        Call( pver->ErrVERModify( pfucb, bookmark, operPreInsert, &prceInsert, NULL ) );
        Assert( prceInsert );
    }

    //  set kdf
    //
    kdf.key     = key;
    kdf.data    = data;
    kdf.fFlags  = 0;

#ifdef DEBUG
    Pcsr( pfucb )->SetILine( Pcsr( pfucb )->Cpage().Clines() - 1 );
    NDGet( pfucb );
    //  while repairing we may append a NULL key to the end of the page
    if( !( FFUCBRepair( pfucb ) && key.Cb() == 0 ) )
    {
        if ( FFUCBUnique( pfucb ) )
        {
            Assert( CmpKey( pfucb->kdfCurr.key, kdf.key ) < 0 );
        }
        else
        {
            Assert( CmpKeyData( pfucb->kdfCurr, kdf ) < 0 );
        }
    }
#endif

    //  set insertion point
    //
    Pcsr( pfucb )->SetILine( Pcsr( pfucb )->Cpage().Clines() );

    //  insert node at end of page
    //
    BTIComputePrefix( pfucb, Pcsr( pfucb ), key, &kdf );
    Assert( !FNDCompressed( kdf ) || kdf.key.prefix.Cb() > 0 );

    cbReq = CbNDNodeSizeCompressed( kdf );

#ifdef DEBUG
    USHORT  cbUncFreeDBG;
    cbUncFreeDBG = Pcsr( pfucb )->Cpage().CbUncommittedFree();
#endif

    //  cursor is at insertion point
    //
    //  check if node fits in page
    //
    if ( FBTIAppend( pfucb, Pcsr( pfucb ), cbReq ) )
    {
        //  readjust kdf to not contain prefix info
        //
        kdf.key     = key;
        kdf.data    = data;
        kdf.fFlags  = 0;

        //  split and insert in tree
        //
        err = ErrBTISplit( pfucb,
                           &kdf,
                           dirflag | fDIRInsert,
                           Rceid( prceInsert ),
                           rceidNull,
                           prceNil,
                           0,
                           NULL );

        if ( errBTOperNone == err && !FFUCBRepair( pfucb ) )
        {
            //  insert was not performed
            //  retry insert using normal insert operation
            //
            Assert( !Pcsr( pfucb )->FLatched() );
            Call( ErrBTInsert( pfucb, key, data, dirflag ) );
            return err;
        }
        else if ( errBTOperNone == err && FFUCBRepair( pfucb ) )
        {
            //  insert was not performed
            //  we may be inserting a NULL key so we can't go through
            //  the normal insert logic. move to the end of the tree
            //  and insert
            //
            Assert( !Pcsr( pfucb )->FLatched() );

            DIB dib;
            dib.pos = posLast;
            dib.pbm = NULL;
            dib.dirflag = fDIRNull;
            Call( ErrBTDown( pfucb, &dib, latchRIW ) );
            Call( Pcsr( pfucb )->ErrUpgrade() );
            goto Retry;
        }

    }
    else
    {
        Assert( !FBTISplit( pfucb, Pcsr( pfucb ), cbReq, fFalse ) );
        Assert( latchWrite == Pcsr( pfucb )->Latch() );

        //  insert node
        //
        err = ErrNDInsert( pfucb, &kdf, dirflag, Rceid( prceInsert ), NULL );
        Assert ( errPMOutOfPageSpace != err );
    }

    Call( err );

    AssertBTGet( pfucb );

    Assert( err >= 0 );
    Assert( Pcsr( pfucb )->FLatched( ) );
    if( prceNil != prceInsert )
    {
        Assert( fVersion );
        prceInsert->ChangeOper( operInsert );
        VERInsertRCEIntoLists( pfucb, Pcsr( pfucb ), prceInsert, NULL );
    }
#ifdef DEBUG
    else
    {
        Assert( !fVersion );
    }
#endif  //  DEBUG

    return err;

HandleError:
    Assert( err < 0 );
    if( prceNil != prceInsert )
    {
        Assert( fVersion );
        VERNullifyFailedDMLRCE( prceInsert );
    }
#ifdef DEBUG
    else
    {
        Assert( !fVersion );
    }
#endif  //  DEBUG

    BTUp( pfucb );
    return err;
}


LOCAL ERR ErrBTITryAvailExtMerge( FUCB * const pfucb )
{
    Assert( FFUCBAvailExt( pfucb ) );

    ERR                 err = JET_errSuccess;
    MERGEAVAILEXTTASK   * const ptask = new MERGEAVAILEXTTASK(
                                                pfucb->u.pfcb->PgnoFDP(),
                                                pfucb->u.pfcb,
                                                pfucb->ifmp,
                                                pfucb->bmCurr );

    if( NULL == ptask )
    {
        CallR ( ErrERRCheck( JET_errOutOfMemory ) );
    }

    if ( PinstFromIfmp( pfucb->ifmp )->m_pver->m_fSyncronousTasks
        || g_rgfmp[ pfucb->ifmp ].FDetachingDB() )
    {
        // don't start the task because the task manager is no longer running
        // and we can't run it synchronously because it will deadlock.
        delete ptask;
        return JET_errSuccess;
    }

    err = PinstFromIfmp( pfucb->ifmp )->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask );
    if( err < JET_errSuccess )
    {
        //  The task was not enqueued successfully.
        delete ptask;
    }

    return err;
}

//  flag-deletes a node
//
ERR ErrBTFlagDelete( FUCB *pfucb, DIRFLAG dirflag, RCE *prcePrimary )
{
    const BOOL      fVersion        = !( dirflag & fDIRNoVersion ) && !g_rgfmp[pfucb->ifmp].FVersioningOff();
    Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() );
    Assert( !FFUCBSpace( pfucb ) || fDIRNoVersion & dirflag );

    ERR     err;
    LATCH   latch = latchReadNoTouch;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTDelete );

    //  save bookmark
    //
    if ( Pcsr( pfucb )->FLatched() )
        CallR( ErrBTISaveBookmark( pfucb ) );

#ifdef DEBUG

    //  Disable RFS and cleanup checking
    //  It's not a real failure path because it's debug only code
    //
    const LONG cRFSCountdownOld = RFSThreadDisable( 0 );
    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

    CAutoKey Key, Data, Key2;
    INT cbKey = 0;
    DATA dataBM;

    if ( Key && Data && Key2  )
    {
        const BOOKMARK* const pbmCur = &pfucb->bmCurr;
        cbKey = pbmCur->key.Cb();
        pbmCur->key.CopyIntoBuffer( Key, cbKey );

        dataBM.SetPv( Data );
        pbmCur->data.CopyInto( dataBM );
    }

    //  Restore RFS and cleanup checking
    //
    FOSSetCleanupState( fCleanUpStateSaved );
    RFSThreadReEnable( cRFSCountdownOld );
#endif

    PERFOpt( PERFIncCounterTable( cBTFlagDelete, PinstFromPfucb( pfucb ), (TCE)tcScope->nParentObjectClass ) );

    VERPROXY        verproxy;
    if ( prceNil != prcePrimary )
    {
        verproxy.prcePrimary    = prcePrimary;
        verproxy.proxy          = proxyCreateIndex;
    }

    const VERPROXY * const pverproxy = ( prceNil != prcePrimary ) ? &verproxy : NULL;

    RCE * prce = prceNil;
    RCEID rceid = rceidNull;

    if( fVersion )
    {
        VER *pver = PverFromIfmp( pfucb->ifmp );
        Call( pver->ErrVERCheckTransactionSize( pfucb->ppib ) );
        Call( pver->ErrVERModify( pfucb, pfucb->bmCurr, operFlagDelete, &prce, pverproxy ) );
        Assert( prceNil != prce );
        rceid = Rceid( prce );
        Assert( rceidNull != rceid );
    }

Start:
    Call( ErrBTIRefresh( pfucb, latch ) );
    AssertNDGet( pfucb );

#ifdef DEBUG
    if ( ( latchReadTouch == latch || latchReadNoTouch == latch )
        && ( latchReadTouch == Pcsr( pfucb )->Latch() || latchReadNoTouch == Pcsr( pfucb )->Latch() ) )
    {
        //  fine!
    }
    else if ( latch == Pcsr( pfucb )->Latch() )
    {
        //  fine!
    }
    else
    {
        Assert( latchWrite == Pcsr( pfucb )->Latch() );
        Assert( FFUCBSpace( pfucb ) );
    }

    if ( Key && Data && Key2 )
    {
        const BOOKMARK* const pbmCur = &pfucb->bmCurr;
        pbmCur->key.CopyIntoBuffer( Key2, pbmCur->key.Cb() );

        Assert( pbmCur->key.Cb() == cbKey );
        Assert( memcmp( Key, Key2, cbKey ) == 0 );
        Assert( pbmCur->data.Cb() == dataBM.Cb() );
        Assert( memcmp( pbmCur->data.Pv(), dataBM.Pv(), dataBM.Cb() ) == 0 );
    }
#endif

    if ( latchWrite != Pcsr( pfucb )->Latch() )
    {
        //  upgrade latch
        //
        err = Pcsr( pfucb )->ErrUpgrade();

        if ( err == errBFLatchConflict )
        {
            Assert( !Pcsr( pfucb )->FLatched() );
            latch = latchRIW;
            goto Start;
        }
        Call( err );
    }

    Assert( latchWrite == Pcsr( pfucb )->Latch() );

    Assert( Pcsr( pfucb )->FLatched( ) );
    AssertNDGet( pfucb );

    //  if we are in the space tree and unversioned and we are
    //  not the first node in the page expunge the node
    //  UNDONE: if we do a BTPrev we will end up on the wrong node
    if ( dirflag & fDIRNoVersion
         && Pcsr( pfucb )->Cpage().FSpaceTree() )
    {
        Assert( FFUCBSpace( pfucb ) );
        if ( 0 != Pcsr( pfucb )->ILine() )
        {
            Assert( !FNDVersion( pfucb->kdfCurr ) );
            Assert( prceNil == prcePrimary );
            Call( ErrNDDelete( pfucb, dirflag ) );

            Assert( Pcsr( pfucb )->ILine() > 0 );
            Pcsr( pfucb )->DecrementILine();    //  correct the currency for a BTNext
            NDGet( pfucb );
        }
        else
        {
            Call( ErrNDFlagDelete( pfucb, dirflag, rceid, pverproxy ) );
        }

        if ( FFUCBAvailExt( pfucb )
            && pgnoNull != Pcsr( pfucb )->Cpage().PgnoNext()
            && Pcsr( pfucb )->Cpage().Clines() < 32
            && !FFMPIsTempDB( pfucb->ifmp )
            && !g_fRepair
            && !( dirflag & fDIRNoLog ))
        {
            Call( ErrBTITryAvailExtMerge( pfucb ) );
        }
    }
    else
    {
        Call( ErrNDFlagDelete( pfucb, dirflag, rceid, pverproxy ) );
    }

    if( prceNil != prce )
    {
        Assert( rceidNull != rceid );
        Assert( fVersion );
        Assert( Pcsr( pfucb )->FLatched() );
        VERInsertRCEIntoLists( pfucb, Pcsr( pfucb ), prce, pverproxy );
    }
#ifdef DEBUG
    else
    {
        Assert( !fVersion );
    }
#endif  //  DEBUG

    return err;

HandleError:
    //  the page may or may not be latched
    //  it won't be latched if RCE creation failed

    Assert( err < 0 );
    if( prceNil != prce )
    {
        Assert( fVersion );
        VERNullifyFailedDMLRCE( prce );
    }

    CallS( ErrBTRelease( pfucb ) );
    return err;
}



//  ================================================================
ERR ErrBTCopyTree( FUCB * pfucbSrc, FUCB * pfucbDest, DIRFLAG dirflag )
//  ================================================================
//
//  Used by repair. Copy all records in one tree to another tree.
//
{
    ERR err;

    FUCBSetPrereadForward( pfucbSrc, cpgPrereadSequential );

    VOID * pvWorkBuf;
    BFAlloc( bfasIndeterminate, &pvWorkBuf );

    DIB dib;
    dib.pos     = posFirst;
    dib.pbm     = NULL;
    dib.dirflag = fDIRNull;
    err = ErrBTDown( pfucbSrc, &dib, latchReadTouch );

    //  check for RecordNotFound, let the loop
    //  handle all other errors
    //
    Assert( JET_errNoCurrentRecord != err );
    if ( JET_errRecordNotFound == err )
    {
        err = JET_errSuccess;
        goto HandleError;
    }

    do
    {
        KEY key;
        DATA data;

        Call( err );

        key.Nullify();
        data.Nullify();

        BYTE * pb = (BYTE *)pvWorkBuf;

        Assert( g_rgfmp[ pfucbSrc->ifmp ].CbPage() == g_rgfmp[ pfucbDest->ifmp ].CbPage() ); // or we may have to be more clever
        if (    pfucbSrc->kdfCurr.key.Cb() < 0 || pfucbSrc->kdfCurr.key.Cb() > g_rgfmp[ pfucbSrc->ifmp ].CbPage() ||
                pfucbSrc->kdfCurr.data.Cb() < 0 || pfucbSrc->kdfCurr.data.Cb() > g_rgfmp[ pfucbSrc->ifmp ].CbPage() ||
                pfucbSrc->kdfCurr.key.Cb() + pfucbSrc->kdfCurr.data.Cb() > g_rgfmp[ pfucbSrc->ifmp ].CbPage() )
        {
            //  we should never hit this since we just repaired the table...
            Error( ErrERRCheck( JET_errInternalError ) );
        }

        pfucbSrc->kdfCurr.key.CopyIntoBuffer( pb );
        key.suffix.SetPv( pb );
        key.suffix.SetCb( pfucbSrc->kdfCurr.key.Cb() );
        pb += pfucbSrc->kdfCurr.key.Cb();

        UtilMemCpy( pb, pfucbSrc->kdfCurr.data.Pv(), pfucbSrc->kdfCurr.data.Cb() );
        data.SetPv( pb );
        data.SetCb( pfucbSrc->kdfCurr.data.Cb() );

        Call( ErrBTRelease( pfucbSrc ) );
        Call( ErrBTInsert( pfucbDest, key, data, dirflag, NULL ) );
        BTUp( pfucbDest );

        err = ErrBTNext( pfucbSrc, fDIRNull );
    }
    while( JET_errNoCurrentRecord != err );

    err = JET_errSuccess;

HandleError:
    BTUp( pfucbSrc );
    BTUp( pfucbDest );
    BFFree( pvWorkBuf );
    return err;
}


//  ******************************************************
//  STATISTICAL OPERATIONS
//

//  computes statistics on a given tree
//      calculates number of nodes, keys and leaf pages
//      in tree
ERR ErrBTComputeStats( FUCB *pfucb, INT *pcnode, INT *pckey, INT *pcpage )
{
    ERR     err;
    DIB     dib;
    PGNO    pgnoT;
    INT     cnode = 0;
    INT     ckey = 0;
    INT     cpage = 0;
    BYTE    *pbKey = NULL;
    KEY     key;


    const INST * const  pinst   = PinstFromPfucb( pfucb );

    Assert( !Pcsr( pfucb )->FLatched() );

    //  go to first node, this is one-time deal. No need to change buffer touch,
    //  set read latch as a ReadAgain latch.
    //
    FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
    dib.dirflag = fDIRNull;
    dib.pos     = posFirst;
    err = ErrBTDown( pfucb, &dib, latchReadNoTouch );
    if ( err < 0 )
    {
        //  if index empty then set err to success
        //
        if ( err == JET_errRecordNotFound )
        {
            err = JET_errSuccess;
            goto Done;
        }
        goto HandleError;
    }

    Assert( Pcsr( pfucb )->FLatched() );

    //  if there is at least one node, then there is a first page.
    //
    cpage = 1;

    if ( FFUCBUnique( pfucb ) )
    {
        forever
        {
            cnode++;

            //  this loop can take some time, so see if we need
            //  to terminate because of shutdown
            //
            Call( pinst->ErrCheckForTermination() );

            //  move to next node
            //
            pgnoT = Pcsr( pfucb )->Pgno();
            err = ErrBTNext( pfucb, fDIRNull );
            if ( err < JET_errSuccess )
            {
                ckey = cnode;
                goto Done;
            }

            if ( Pcsr( pfucb )->Pgno() != pgnoT )
            {
                //  increment page count, if page boundary is crossed
                //
                cpage++;
            }
        }
    }
    else
    {
        Assert( dib.dirflag == fDIRNull );
        key.Nullify();
        Alloc( pbKey = (BYTE *)RESKEY.PvRESAlloc() );
        key.suffix.SetPv( pbKey );

        Assert( pfucb->u.pfcb->FTypeSecondaryIndex() );
        Assert( pidbNil != pfucb->u.pfcb->Pidb() );
        Assert( !pfucb->u.pfcb->Pidb()->FUnique() );

        //  initialize key count to 1, to represent the
        //  node we're currently on
        ckey = 1;

        forever
        {
            //  copy key of current node
            //
            Assert( pfucb->kdfCurr.key.Cb() <= cbKeyMostMost );
            key.suffix.SetCb( pfucb->kdfCurr.key.Cb() );
            pfucb->kdfCurr.key.CopyIntoBuffer( key.suffix.Pv() );

            cnode++;

            //  this loop can take some time, so see if we need
            //  to terminate because of shutdown
            //
            Call( pinst->ErrCheckForTermination() );

            //  move to next node
            //
            pgnoT = Pcsr( pfucb )->Pgno();
            err = ErrBTNext( pfucb, fDIRNull );
            if ( err < JET_errSuccess )
            {
                goto Done;
            }

            if ( Pcsr( pfucb )->Pgno() != pgnoT )
            {
                //  increment page count, if page boundary is crossed
                //
                cpage++;
            }

            if ( !FKeysEqual( pfucb->kdfCurr.key, key ) )
            {
                //  increment key count, if key boundary is crossed
                //
                ckey++;
            }
        }
    }


Done:
    //  common exit loop processing
    //
    if ( err < 0 && err != JET_errNoCurrentRecord )
        goto HandleError;

    err     = JET_errSuccess;
    *pcnode = cnode;
    *pckey  = ckey;
    *pcpage = cpage;

HandleError:
    BTUp( pfucb );
    Assert( !Pcsr( pfucb )->FLatched() );
    RESKEY.Free( pbKey );
    return err;
}


//  ================================================================
bool FBTICpgFragmentedRange(
    __in_ecount(cpg) const PGNO * const rgpgno,
    const CPG cpg,
    _In_ const CPG cpgFragmentedRangeMin,
    _Out_ INT * const pipgnoStart,
    _Out_ INT * const pipgnoEnd )
//  ================================================================
//
// Given an array of page numbers, determine which of them should be included in
// the next fragmented range. The range [*pipgnoStart, *pipgnoEnd] is considered
// fragmented and should be defragmented.
// 
// cpgFragmentedRangeMin gives the minimum size of a non-contiguous range that
// is considered worth defragmenting
//
// Returns true if a fragmented range was found, false otherwise.
//
//-
{
    Assert(rgpgno);
    Assert(cpg);
    Assert(pipgnoStart);
    Assert(pipgnoEnd);
    Assert(pipgnoStart != pipgnoEnd);

    bool fFoundFragmentedRange = false;

    *pipgnoStart = -1;
    *pipgnoEnd = -1;

    if(cpg <= cpgFragmentedRangeMin)
    {
        // not enough pages to be worth defragmenting
        OSTrace( JET_tracetagOLD, OSFormat( __FUNCTION__ "( rgpgno = [%d, ...], cpg = %d ) did not satisfy minimum defrag range count (%d).",
                        rgpgno ? rgpgno[ 0 ] : 0, cpg, cpgFragmentedRangeMin ) );
        fFoundFragmentedRange = false;
    }
    else
    {
        INT ipgnoFragmentedRangeStart = -1;
        INT ipgno = 0;
        while( ipgno < cpg && !fFoundFragmentedRange )
        {
            INT ipgnoContiguousRunStart = ipgno;

            // Advance pgno until it points to a non-contiguous page
            for(    ipgno++; ipgno < cpg &&
                    ( rgpgno[ipgno - 1] + 1 == rgpgno[ipgno] || rgpgno[ipgno - 1] - 1 == rgpgno[ipgno] );
                    ++ipgno )
            {
            }

            // All of the pages in the range [ipgnoContiguousRunStart, ipgnoContiguousRunEnd]
            // are in ascending sequential order. Remember that the run can have a length of 1.
            const CPG cpgContiguous = ipgno - ipgnoContiguousRunStart;
            INT ipgnoContiguousRunEnd = ipgno-1;

            Assert( cpgContiguous > 0 );
            Assert( ipgnoContiguousRunEnd >= ipgnoContiguousRunStart );
            Assert( ipgnoContiguousRunEnd - ipgnoContiguousRunStart
                    == abs( (INT)( rgpgno[ipgnoContiguousRunEnd] - rgpgno[ipgnoContiguousRunStart] ) ) );

            // This run of pages is long enough that we don't want to defrag it.
            // See if we have a previous set of fragmented pages that should be defragged
            // We also need to do this if this is the last time through the loop.
            if( cpgContiguous >= cpgFragmentedRangeMin )
            {
                if( ipgnoFragmentedRangeStart != -1 )
                {
                    // prior to encountering this contiguous range we found a non-contiguous range
                    // in that case we want to defrag that range (if it is long enough)
                    const CPG cpgFragmented = ipgnoContiguousRunStart - ipgnoFragmentedRangeStart;
                    Assert( cpgFragmented >= 0 );
                    if (cpgFragmented > cpgFragmentedRangeMin )
                    {
                        fFoundFragmentedRange = true;
                        *pipgnoStart = ipgnoFragmentedRangeStart;
                        *pipgnoEnd = ipgnoContiguousRunStart - 1;
                    }
                    else
                    {
                        ipgnoFragmentedRangeStart = -1;
                    }
                }
            }
            else if ( cpg == ipgno )
            {
                // this is the last time through the loop so we will consider all the pages from
                // ipgnoFragmentedRangeStart through the end of the page to be fragmented
                if( ipgnoFragmentedRangeStart != -1 )
                {
                    // prior to encountering this contiguous range we found a non-contiguous range
                    // in that case we want to defrag that range (if it is long enough)
                    const CPG cpgFragmented = ipgno - ipgnoFragmentedRangeStart;
                    Assert( cpgFragmented >= 0 );
                    if (cpgFragmented > cpgFragmentedRangeMin )
                    {
                        fFoundFragmentedRange = true;
                        *pipgnoStart = ipgnoFragmentedRangeStart;
                        *pipgnoEnd = ipgno - 1;
                    }
                }
            }
            else if ( -1 == ipgnoFragmentedRangeStart )
            {
                // the run of pages is fragmented remember the start of the range and keep looking
                // for the end
                ipgnoFragmentedRangeStart = ipgnoContiguousRunStart;
            }
        }
    }

    if( fFoundFragmentedRange )
    {
        Assert( *pipgnoStart >= 0 );
        Assert( *pipgnoStart < cpg );
        Assert( *pipgnoEnd > *pipgnoStart );
        Assert( *pipgnoEnd < cpg );
        Assert( (*pipgnoEnd - *pipgnoStart) >= cpgFragmentedRangeMin );
    }

    return fFoundFragmentedRange;
}

#ifdef ENABLE_JET_UNIT_TEST

void Reverse( _In_ PGNO* const rgpgno, const INT cpgno )
{
    for ( INT ipgno = 0; ipgno < cpgno / 2; ipgno++ )
    {
        PGNO pgnoT = rgpgno[ipgno];
        rgpgno[ipgno] = rgpgno[cpgno - ipgno - 1];
        rgpgno[cpgno - ipgno - 1] = pgnoT;
    }
}

JETUNITTEST( FBTICpgFragmentedRange, RunTooShort )
{
    PGNO rgpgno[] = { 10, 20, 30, 40, 50, 60, 70, 80 };
    INT ignored1;
    INT ignored2;
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &ignored1, &ignored2 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof( rgpgno ), 8, &ignored1, &ignored2 ) );
}

JETUNITTEST( FBTICpgFragmentedRange, ContiguousRun )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    INT ignored1;
    INT ignored2;
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &ignored1, &ignored2 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof( rgpgno ), 8, &ignored1, &ignored2 ) );
}

JETUNITTEST( FBTICpgFragmentedRange, ContiguousRuns )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 101, 102, 103, 104, 105, 106, 107, 108, };
    INT ignored1;
    INT ignored2;
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &ignored1, &ignored2 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof( rgpgno ), 8, &ignored1, &ignored2 ) );
}

JETUNITTEST( FBTICpgFragmentedRange, MinRangeSizeIsRespected )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 101, 102, 103, 104, 105, 106, 107, 108, };
    INT ignored1;
    INT ignored2;
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 10, &ignored1, &ignored2 ) );
    CHECK( true == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 11, &ignored1, &ignored2 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof( rgpgno ), 10, &ignored1, &ignored2 ) );
    CHECK( true == FBTICpgFragmentedRange( rgpgno, _countof( rgpgno ), 11, &ignored1, &ignored2 ) );
}

// We can make FBTICpgFragmentedRange less sensitive by decreasing the range size
JETUNITTEST( FBTICpgFragmentedRange, SmallerRangeSizeTriggersLessOften )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, /*5*/ 6, 7, 8, 9, 10, };
    INT ignored1;
    INT ignored2;
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 4, &ignored1, &ignored2 ) );
    CHECK( true == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &ignored1, &ignored2 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof( rgpgno ), 4, &ignored1, &ignored2 ) );
    CHECK( true == FBTICpgFragmentedRange( rgpgno, _countof( rgpgno ), 8, &ignored1, &ignored2 ) );
}

JETUNITTEST( FBTICpgFragmentedRange, FragmentedRuns )
{
    PGNO rgpgno[] = { 1, 2, 3, /*4*/ 5, 6, 7, 8, 9, 20, 21, /*22*/ 23, 24, 25, 26, 27, 28 };
    INT start;
    INT end;
    CHECK( true == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &start, &end ) );
    CHECK( 0 == start );
    CHECK( _countof(rgpgno)-1 == end );
}

JETUNITTEST( FBTICpgFragmentedRange, FragmentedThenUnfragmented )
{
    PGNO rgpgno[] = { 1, 2, 3, /*4*/ 5, 6, 7, 8, 9, 10, 20, 21, 22, 23, 24, 25, 26, 27, 28 };
    INT start;
    INT end;
    CHECK( true == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &start, &end ) );
    CHECK( 0 == start );
    CHECK( 8 == end );
}

JETUNITTEST( FBTICpgFragmentedRange, UnfragmentedThenFragmented )
{
    PGNO rgpgno[] = { 20, 21, 22, 23, 24, 25, 26, 27, 28, 1, 2, 3, /*4*/ 5, 6, 7, 8, 9, 10,  };
    INT start;
    INT end;
    CHECK( true == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &start, &end ) );
    CHECK( 9 == start );
    CHECK( _countof(rgpgno)-1 == end );
}

JETUNITTEST( FBTICpgFragmentedRange, UnfragmentedThenFragmentedThenUnfragmented )
{
    PGNO rgpgno[] = {
        20, 21, 22, 23, 24, 25, 26, 27, 28,
        1, 2, 3, /*4*/ 5, 6, 7, 8, 9, 10,
        100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110
    };
    INT start;
    INT end;
    CHECK( true == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &start, &end ) );
    CHECK( 9 == start );
    CHECK( 17 == end );
}

JETUNITTEST( FBTICpgFragmentedRange, UnfragmentedThenFragmentedThenShortThenUnfragmented )
{
    PGNO rgpgno[] = {
        20, 21, 22, 23, 24, 25, 26, 27, 28,
        1, 2, 3, /*4*/ 5, 6, 7, 8, 9, 10,
        500, 501, 502, 503, 504, 505,
        100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110
    };
    INT start;
    INT end;
    CHECK( true == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &start, &end ) );
    CHECK( 9 == start );
    CHECK( 23 == end );
}

JETUNITTEST( FBTICpgFragmentedRange, SmallFragmentedRanges )
{
    PGNO rgpgno[] = {
        20, 22, 34, 44,
        50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
        70, 74, 77
    };
    INT start;
    INT end;
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &start, &end ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof( rgpgno ), 8, &start, &end ) );
}

#endif // ENABLE_JET_UNIT_TEST

//  ================================================================
bool FBTICpgFragmentedRangeTrigger(
    __in_ecount(cpg) const PGNO * const rgpgno,
    const CPG cpg,
    _In_ const CPG cpgFragmentedRangeTrigger,
    _In_ const CPG cpgFragmentedRangeMin )
//  ================================================================
//
// Uses FBTICpgFragmentedRange() to determine if the array of page
// numbers meets the criteria for triggering OLD2.
// 
// Returns true if OLD2 should be triggered, given the array of
// page numbers and the limits for triggering and performing actual
// defragmentation work (cpgFragmentedRangeTrigger and
// cpgFragmentedRangeMin, respectively).
//
// We do this because the trigger threshold can be lower than the
// defrag threshold. In that case we want to make sure that there
// are enough fragmented pages for OLD2 to work on.
//
//-
{
    INT ipgnoStart, ipgnoEnd;
    return FBTICpgFragmentedRange( rgpgno, cpg, cpgFragmentedRangeTrigger, &ipgnoStart, &ipgnoEnd ) &&
            FBTICpgFragmentedRange( rgpgno, cpg, cpgFragmentedRangeMin, &ipgnoStart, &ipgnoEnd );
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( FBTICpgFragmentedRangeTrigger, NotEnoughPagesToDefragAllFragmented )
{
    PGNO rgpgno[] = { 10, 20, 30, 40, 50, 60, 70, };
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof( rgpgno ), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, NotEnoughPagesToDefragUnfragmentedThenFragmented )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9,
                        100, 110, 120, 130, 140, };
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof( rgpgno ), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, NotEnoughPagesToDefragFragmentedThenUnfragmented )
{
    PGNO rgpgno[] = { 10, 20, 30, 40, 50,
                        60, 61, 62, 63, 64, 65, 66, 67, 68, };
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof( rgpgno ), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragButDoesNotMeetTrigger )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, /*6*/ 7, 8, 9, 10, 11, };
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof( rgpgno ), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragButContiguous )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9,
                        /*10*/
                        11, 12, 13, 14, 15, 16, 17, 18, 19, };
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof( rgpgno ), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragAllFragmented )
{
    PGNO rgpgno[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, };
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof( rgpgno ), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragUnfragmentedThenFragmented )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9,
                        100, 110, 120, 130, 140, 150, 160, 170, 180, };
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof( rgpgno ), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragFragmentedThenUnfragmented )
{
    PGNO rgpgno[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90,
                        100, 101, 102, 103, 104, 105, 106, 107, 108, 109, };
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof( rgpgno ), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragUnfragmentedThenFragmentedThenUnfragmented )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9,
                        100, 110, 120, 130, 140, 150, 160, 170, 180,
                        191, 192, 193, 194, 195, 196, 197, 198, 199, };
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof( rgpgno ), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragFragmentedThenUnfragmentedThenFragmented )
{
    PGNO rgpgno[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90,
                        100, 101, 102, 103, 104, 105, 106, 107, 108,
                        200, 210, 220, 230, 240, 250, 260, 270, 280 };
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
    Reverse( rgpgno, _countof( rgpgno ) );
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof( rgpgno ), 4, 8 ) );
}

#endif // ENABLE_JET_UNIT_TEST

//  ================================================================
ERR ErrBTIGetBookmarkFromPage(
    _In_ FUCB * const pfucb,
    _In_ CSR * const pcsr,
    const INT iline,
    _Out_ BOOKMARK * const pbm )
//  ================================================================
//
//  Given a CSR which points to a child page, get the first bookmark
//  on the child.
//
//-
{
    Assert( pfucb );
    Assert( pcsr );
    Assert( pbm );
    Assert( pcsr->FLatched() );
    Assert( pbm->key.FNull() );
    Assert( pcsr->Cpage().FInvisibleSons() );

    // this code only works for unique bookmarks
    Assert( FFUCBUnique( pfucb ) );

    ERR err;
    CSR csr;
    BYTE * pb = NULL;

    pcsr->SetILine( iline );
    NDGet( pfucb, pcsr );
    const PGNO pgno = *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv();

    Call( csr.ErrGetReadPage(   pfucb->ppib,
                                pfucb->ifmp,
                                pgno,
                                bflfDefault,
                                PBFLatchHintPgnoRoot( pfucb ) ) );
    csr.SetILine(0);
    NDGet( pfucb, &csr );

    const INT cbKey = pfucb->kdfCurr.key.Cb();
    Alloc( pb = new BYTE[cbKey] );
    pfucb->kdfCurr.key.CopyIntoBuffer(pb, cbKey);
    pbm->key.suffix.SetPv( pb );
    pbm->key.suffix.SetCb( cbKey );
    pb = NULL;

HandleError:
    csr.ReleasePage();
    delete [] pb;
    if( err < JET_errSuccess )
    {
        Assert( pbm->key.FNull() );
    }
    return err;
}

//  ================================================================
ERR ErrBTIFindFragmentedRangeInParentOfLeafPage(
    _In_ FUCB * const pfucb,
    _In_ CSR * const pcsr,
    _In_ const CPG cpgFragmentedRangeMin,
    _Out_ BOOKMARK * const pbmRangeStart,
    _Out_ BOOKMARK * const pbmRangeEnd)
//  ================================================================
//
//  Look for a fragmented range which occurs after the current iline.
//  If no range is found then return JET_errRecordNotFound.
//
//-
{
    Assert( pfucb );
    Assert( pcsr );
    Assert( pbmRangeStart );
    Assert( pbmRangeEnd );
    Assert( pcsr->FLatched() );
    Assert( pcsr->Cpage().FParentOfLeaf() );
    Assert( pbmRangeStart->key.FNull() );
    Assert( pbmRangeEnd->key.FNull() );

    ERR err;
    PGNO * rgpgno = NULL;

    const INT ilineStart    = pcsr->ILine();
    const INT clines        = pcsr->Cpage().Clines();
    const INT cpgno         = clines-ilineStart;
    Alloc( rgpgno = new PGNO[clines-ilineStart] );

    for( INT iline = ilineStart; iline < clines; ++iline )
    {
        pcsr->SetILine( iline );
        NDGet( pfucb, pcsr );
        const PGNO pgnoChild = *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv();
        INT ipgno = iline - ilineStart;
        Assert( ipgno >= 0 && ipgno < cpgno );
        rgpgno[ipgno] = pgnoChild;
    }

    INT ipgnoRangeStart;
    INT ipgnoRangeEnd;
    if( FBTICpgFragmentedRange( rgpgno, cpgno, cpgFragmentedRangeMin, &ipgnoRangeStart, &ipgnoRangeEnd ) )
    {
        OSTraceFMP( pfucb->ifmp, JET_tracetagOLD,
            OSFormat(   "ErrBTIFindFragmentedRangeInParentOfLeafPage(page = %d, ilineStart = %d) finds range [%d,%d]",
                        pcsr->Pgno(), ilineStart, ipgnoRangeStart, ipgnoRangeEnd));

        const INT ilineRangeStart = ilineStart + ipgnoRangeStart;
        const INT ilineRangeEnd = ilineStart + ipgnoRangeEnd;
        Assert( ilineRangeEnd > ilineRangeStart );
        Call( ErrBTIGetBookmarkFromPage( pfucb, pcsr, ilineRangeStart, pbmRangeStart ) );
        Call( ErrBTIGetBookmarkFromPage( pfucb, pcsr, ilineRangeEnd, pbmRangeEnd ) );
    }
    else
    {
        OSTraceFMP( pfucb->ifmp, JET_tracetagOLD,
            OSFormat(   "ErrBTIFindFragmentedRangeInParentOfLeafPage(page = %d, ilineStart = %d) finds (%d-%d)",
                        pcsr->Pgno(), ilineStart, ipgnoRangeStart, ipgnoRangeEnd));

        Call( ErrERRCheck( JET_errRecordNotFound ) );
    }

HandleError:
    delete [] rgpgno;

    Assert( err <= JET_errSuccess );    // no warnings are returned
    if( JET_errSuccess == err )
    {
        Assert( !pbmRangeStart->key.FNull() );
        // can't assert pbmRangeEnd because a null key is fine for the end of the tree
    }

    return err;
}

//  ================================================================
ERR ErrBTIFindFragmentedRange(
    _In_ FUCB * const pfucb,
    _In_ CSR * const pcsr,
    _In_ const BOOKMARK& bmStart,
    _Out_ BOOKMARK * const pbmRangeStart,
    _Out_ BOOKMARK * const pbmRangeEnd)
//  ================================================================
//
//  Recursively move down the tree looking for a fragmented range 
//  which occurs after bmStart. If no range is found then return
//  JET_errRecordNotFound.
//
//-
{
    Assert( pfucb );
    Assert( pcsr );
    Assert( pbmRangeStart );
    Assert( pbmRangeEnd );
    Assert( pcsr->FLatched() );
    Assert( pbmRangeStart->key.FNull() );
    Assert( pbmRangeEnd->key.FNull() );

    ERR err;
    CSR csrSubtree;

    Assert( !pcsr->Cpage().FLeafPage() );

    // Find the node for bmStart. This sets the iline of the csr.
    Call( ErrNDSeek( pfucb, pcsr, bmStart ) );

    // If the page is a parent-of-leaf then look for a fragmented range
    if( pcsr->Cpage().FParentOfLeaf() )
    {
        Call( ErrBTIFindFragmentedRangeInParentOfLeafPage(
            pfucb,
            pcsr,
            cpgFragmentedRangeDefrag,
            pbmRangeStart,
            pbmRangeEnd ) );
    }
    else
    {
        const INT ilineStart    = pcsr->ILine();
        const INT clines        = pcsr->Cpage().Clines();

        bool fFoundRange = false;

        // For each node until the end of the page:         
        for( INT iline = ilineStart; iline < clines; ++iline )
        {
            // Latch the child page
            pcsr->SetILine( iline );
            NDGet( pfucb, pcsr );
            const PGNO pgnoChild = *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv();

            // Recursively look for a fragmented range  
            Call( csrSubtree.ErrGetReadPage(pfucb->ppib,
                                            pfucb->ifmp,
                                            pgnoChild,
                                            bflfDefault,
                                            PBFLatchHintPgnoRoot( pfucb ) ) );

            err = ErrBTIFindFragmentedRange( pfucb, &csrSubtree, bmStart, pbmRangeStart, pbmRangeEnd );
            csrSubtree.ReleasePage();

            if( JET_errRecordNotFound == err )
            {
                // this is an expected error, we should continue to the next subtree
                err = JET_errSuccess;
            }
            else if( JET_errSuccess == err )
            {
                // a fragmented range was found in the subtree
                fFoundRange = true;
                break;
            }
            Call( err );
        }

        // Return an error if no range was found
        if( !fFoundRange )
        {
            Call( ErrERRCheck( JET_errRecordNotFound ) );
        }
    }

    // No subtree contained a fragmented range

HandleError:
    Assert( !csrSubtree.FLatched() );
    Assert( err <= JET_errSuccess );    // no warnings are returned
    if( JET_errSuccess == err )
    {
        Assert( !pbmRangeStart->key.FNull() );
        // can't assert pbmRangeEnd because a null key is fine for the end of the tree
    }

    return err;
}

//  ================================================================
ERR ErrBTFindFragmentedRange(
    _In_ FUCB * const pfucb,
    _In_ const BOOKMARK& bmStart,
    _Out_ BOOKMARK * const pbmRangeStart,
    _Out_ BOOKMARK * const pbmRangeEnd)
//  ================================================================
//
// Find the first fragmented range of the b-tree after the given bookmark
// The fragmented range is described by pbmStart and pbmEnd. This function
// allocates memory for the bookmarks in pbmRangeStart and pbmRangeEnd so
// they should be nullified when passed in. If no fragmented range is found
// then JET_errRecordNotFound is returned.
//
//-
{
    Assert( pfucb );
    Assert( pbmRangeStart );
    Assert( pbmRangeEnd );
    Assert( pbmRangeStart->key.FNull() );
    Assert( pbmRangeEnd->key.FNull() );

    ERR err;
    CSR csrRoot;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsNone );

    // Latch the root page
    Call( csrRoot.ErrGetReadPage( pfucb->ppib,
                                  pfucb->ifmp,
                                  PgnoRoot( pfucb ),
                                  bflfDefault,
                                  PBFLatchHintPgnoRoot( pfucb ) ) );

    // If the root page is a leaf page then the tree is too shallow
    if( csrRoot.Cpage().FLeafPage() )
    {
        Call( ErrERRCheck( JET_errRecordNotFound ) );
    }

    Call( ErrBTIFindFragmentedRange( pfucb, &csrRoot, bmStart, pbmRangeStart, pbmRangeEnd ) );

HandleError:
    csrRoot.ReleasePage();

    Assert( err <= JET_errSuccess );    // no warnings are returned
    if( JET_errSuccess == err )
    {
        Assert( !pbmRangeStart->key.FNull() );
        // can't assert pbmRangeEnd because a null key is fine for the end of the tree
    }

    return err;

}

ERR ErrBTDumpPageUsage( PIB * ppib, const IFMP ifmp, const PGNO pgnoFDP )
{
    ERR         err                     = JET_errSuccess;
    FUCB *      pfucb                   = pfucbNil;
    DIB         dib;
    CSR *       pcsr;
    LINE        line;
    ULONG       cTotalPages             = 0;
    QWORD       cTotalNodes             = 0;
    QWORD       cTotalDeleted           = 0;
    QWORD       cTotalVersioned         = 0;
    QWORD       cTotalCompressed        = 0;
    QWORD       cbTotalUsed             = 0;
    QWORD       cbTotalFree             = 0;
    QWORD       cbTotalPrefix           = 0;
    QWORD       cbTotalPrefixSavings    = 0;
    ULONG       cTotalBadPrefixes       = 0;
    ULONG       cbTotalNodeMax          = 0;
    ULONG       cbTotalNodeMin          = ulMax;

    printf( "\n" );
    printf( "Root Page: %d (0x%x)\n", pgnoFDP, pgnoFDP );
    printf( "\n" );
    printf( "Page\tcNodes\tcDeleted\tVersioned\tCompressed\tcbUsed\tcbFree\tcbPrefix\tcbPrefixSavings\tcBadPrefixes\tcbNodeMax\tcbNodeMin\n" );
    printf( "----\t------\t--------\t---------\t----------\t------\t------\t--------\t---------------\t------------\t---------\t---------\n" );

    if ( pgnoNull != pgnoFDP )
    {

        Call( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb ) );
        FUCBSetIndex( pfucb );

        //  we will be traversing the entire tree in order, preread all the pages
        //
        FUCBSetSequential( pfucb );
        FUCBSetPrereadForward( pfucb, cpgPrereadSequential );

        pcsr = Pcsr( pfucb );

        dib.pos = posFirst;
        dib.pbm = NULL;
        dib.dirflag = fDIRAllNode;

        err = ErrBTDown( pfucb, &dib, latchReadNoTouch );

        for ( err = ( JET_errRecordNotFound != err ? err : ErrERRCheck( JET_errNoCurrentRecord ) );
            JET_errNoCurrentRecord != err;
            err = ErrBTNext( pfucb, fDIRAllNode ) )
        {
            const ULONG     cNodes          = pcsr->Cpage().Clines();
            ULONG           cDeleted        = 0;
            ULONG           cVersioned      = 0;
            ULONG           cCompressed     = 0;
            ULONG           cbUsed          = 0;
            const ULONG     cbFree          = pcsr->Cpage().CbPageFree();
            ULONG           cbPrefix        = 0;
            ULONG           cbPrefixSavings = 0;
            ULONG           cBadPrefixes    = 0;
            ULONG           cbNodeMax       = 0;
            ULONG           cbNodeMin       = ulMax;

            //  process result of record navigation
            //
            Call( err );

            //  calculate size of prefix (external header)
            //
            NDGetPtrExternalHeader( pcsr->Cpage(), &line, noderfWhole );
            cbPrefix = CPAGE::cbInsertionOverhead + line.cb;
            cbUsed += cbPrefix;

            //  calculate stats for each node on the page
            //
            for ( ULONG iline = 0; iline < cNodes; iline++ )
            {
                ULONG   cbCurrNode;

                pcsr->Cpage().GetPtr( iline, &line );
                cbCurrNode = CPAGE::cbInsertionOverhead + line.cb;
                cbUsed += cbCurrNode;

                if ( cbCurrNode > cbNodeMax )
                {
                    cbNodeMax = cbCurrNode;
                }
                if ( cbCurrNode < cbNodeMin )
                {
                    cbNodeMin = cbCurrNode;
                }

                pcsr->SetILine( iline );
                NDGet( pfucb, pcsr );

                if ( FNDDeleted( pfucb->kdfCurr ) )
                {
                    cDeleted++;
                }
                if ( FNDVersion( pfucb->kdfCurr ) )
                {
                    cVersioned++;
                }
                if ( FNDCompressed( pfucb->kdfCurr ) )
                {
                    cCompressed++;

                    if ( pfucb->kdfCurr.key.prefix.Cb() > cbKeyCount )
                    {
                        cbPrefixSavings += ( pfucb->kdfCurr.key.prefix.Cb() - cbKeyCount );
                    }
                    else
                    {
                        cBadPrefixes++;
                    }
                }
            }

            //  output this node
            //
            printf(
                "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                pcsr->Pgno(),
                cNodes,
                cDeleted,
                cVersioned,
                cCompressed,
                cbUsed,
                cbFree,
                cbPrefix,
                cbPrefixSavings,
                cBadPrefixes,
                cbNodeMax,
                cbNodeMin );

            //  update running totals
            //
            cTotalPages++;
            cTotalNodes += cNodes;
            cTotalDeleted += cDeleted;
            cTotalVersioned += cVersioned;
            cTotalCompressed += cCompressed;
            cbTotalUsed += cbUsed;
            cbTotalFree += cbFree;
            cbTotalPrefix += cbPrefix;
            cbTotalPrefixSavings += cbPrefixSavings;
            cTotalBadPrefixes += cBadPrefixes;

            if ( cbNodeMax > cbTotalNodeMax )
                cbTotalNodeMax = cbNodeMax;
            if ( cbNodeMin < cbTotalNodeMin )
                cbTotalNodeMin = cbNodeMin;

            //  currency should be on the last node on the page
            //  (so moving to the next node will force us to
            //  the next page)
            //
            Assert( ULONG( pcsr->ILine() + 1 ) == cNodes );
        }

        //  reached end of tree
        //
        err = JET_errSuccess;
    }

    printf( "\n\n" );
    printf( "Total pages: %d\n", cTotalPages );
    printf( "Total nodes: %I64d\n", cTotalNodes );
    printf( "Total deleted nodes: %I64d\n", cTotalDeleted );
    printf( "Total versioned nodes: %I64d\n", cTotalVersioned );
    printf( "Total compressed nodes: %I64d\n", cTotalCompressed );
    printf( "Total used space: %I64d bytes\n", cbTotalUsed );
    printf( "Total free space: %I64d bytes\n", cbTotalFree );
    printf( "Total page overhead: %I64d bytes\n", QWORD( cTotalPages ) * sizeof(CPAGE::PGHDR) );
    printf( "Total prefix space: %I64d bytes\n", cbTotalPrefix );
    printf( "Total prefix savings: %I64d bytes\n", cbTotalPrefixSavings );
    printf( "Total bad prefixes: %d\n", cTotalBadPrefixes );
    printf( "Largest node: %d bytes\n", cbTotalNodeMax );
    printf( "Smallest node: %d bytes\n", ( cbTotalNodeMax > 0 ? cbTotalNodeMin : 0 ) );
    printf( "\n" );

HandleError:

    if ( pfucbNil != pfucb )
    {
        BTClose( pfucb );
    }
    return err;
}



//  ******************************************************
//  SPECIAL OPERATIONS
//


INLINE ERR ErrBTICreateFCB(
    PIB             *ppib,
    const IFMP      ifmp,
    const PGNO      pgnoFDP,
    const OBJID     objidFDP,
    const OPENTYPE  opentype,
    FUCB            **ppfucb )
{
    ERR             err;
    FCB             *pfcb       = pfcbNil;
    FUCB            *pfucb      = pfucbNil;

    //  create a new FCB

    CallR( FCB::ErrCreate( ppib, ifmp, pgnoFDP, &pfcb ) );

    //  the creation was successful

    Assert( pfcb->IsLocked() );
    Assert( pfcb->FTypeNull() );                // No fcbtype yet.
    Assert( pfcb->Ifmp() == ifmp );
    Assert( pfcb->PgnoFDP() == pgnoFDP );
    Assert( !pfcb->FInitialized() );
    Assert( pfcb->WRefCount() == 0 );
    pfcb->Unlock();

    Call( ErrFUCBOpen( ppib, ifmp, &pfucb ) );
    Call( pfcb->ErrLink( pfucb ) );

    Assert( !pfcb->FSpaceInitialized() );
    Assert( openNew != opentype || objidNil == objidFDP );
    if ( openNew != opentype )
    {
        if ( objidNil == objidFDP )
        {
            Assert( openNormal == opentype );

            //  read space info into FCB cache, including objid
            Call( ErrSPInitFCB( pfucb ) );
            Assert( g_fRepair || pfcb->FSpaceInitialized() );
        }
        else
        {
            pfcb->SetObjidFDP( objidFDP );
            if ( openNormalNonUnique == opentype )
            {
                pfcb->Lock();
                pfcb->SetNonUnique();
                pfcb->Unlock();
            }
            else
            {
                Assert( pfcb->FUnique() );          //  btree is initially assumed to be unique
                Assert( openNormalUnique == opentype );
            }
            Assert( !pfcb->FSpaceInitialized() );
        }
    }

    if ( pgnoFDP == pgnoSystemRoot )
    {
        // SPECIAL CASE: For database cursor, we've got all the
        // information we need.

        //  when opening db cursor, always force to check the root page
        Assert( objidNil == objidFDP );
        if ( openNew == opentype )
        {
            //  objid will be set when we return to ErrSPCreate()
            Assert( objidNil == pfcb->ObjidFDP() );
        }
        else
        {
            Assert( objidSystemRoot == pfcb->ObjidFDP() );
        }

        //  insert this FCB into the global list

        pfcb->InsertList();

        //  finish initializing this FCB

        pfcb->Lock();
        Assert( pfcb->FTypeNull() );
        pfcb->SetTypeDatabase();
        pfcb->CreateComplete();
        pfcb->Unlock();
    }

    *ppfucb = pfucb;
    Assert( !Pcsr( pfucb )->FLatched() );

    return err;

HandleError:
    Assert( pfcbNil != pfcb );
    Assert( !pfcb->FInitialized() );
    Assert( !pfcb->FInList() );
    Assert( !pfcb->FInLRU() );
    Assert( ptdbNil == pfcb->Ptdb() );
    Assert( pfcbNil == pfcb->PfcbNextIndex() );
    Assert( pidbNil == pfcb->Pidb() );

    if ( pfucbNil != pfucb )
    {
        if ( pfcbNil != pfucb->u.pfcb )
        {
            Assert( pfcb == pfucb->u.pfcb );
            // We managed to link the FUCB to the FCB before we errored.
            pfcb->Unlink( pfucb, fTrue );
        }

        //  close the FUCB
        FUCBClose( pfucb );
    }

    //  synchronously purge the FCB
    pfcb->PrepareForPurge( fFalse );
    pfcb->Purge( fFalse );

    return err;
}


//  *****************************************************
//  BTREE INTERNAL ROUTINES
//

//  opens a cursor on a tree rooted at pgnoFDP
//  open cursor on corresponding FCB if it is in cache [common case]
//  if FCB not in cache, create one, link with cursor
//              and initialize FCB space info
//  if fNew is set, this is a new tree,
//      so do not initialize FCB space info
//  fWillInitFCB: On a passive, is the caller planning to fully hydrate the placeholder FCB?
//
ERR ErrBTIOpen(
    PIB             *ppib,
    const IFMP      ifmp,
    const PGNO      pgnoFDP,
    const OBJID     objidFDP,
    const OPENTYPE  opentype,
    FUCB            **ppfucb,
    BOOL            fWillInitFCB )
{
    ERR             err;
    FCB             *pfcb;
    INT             fState;
    ULONG           cRetries = 0;
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope( );
    tcScope->iorReason.SetIors( iorsBTOpen );

RetrieveFCB:
    AssertTrack( cRetries != 100000, "TooManyFcbOpenRetries" );

    //  get the FCB for the given ifmp/pgnoFDP

    pfcb = FCB::PfcbFCBGet( ifmp, pgnoFDP, &fState, fTrue, !fWillInitFCB );
    if ( pfcb == pfcbNil )
    {

        //  the FCB does not exist

        Assert( fFCBStateNull == fState );

        //  try to create a new B-tree which will cause the creation of the new FCB

        err = ErrBTICreateFCB( ppib, ifmp, pgnoFDP, objidFDP, opentype, ppfucb );
        Assert( err <= JET_errSuccess );        // Shouldn't return warnings.

        if ( err == errFCBExists )
        {

            //  we failed because someone else was racing to create
            //      the same FCB that we want, but they beat us to it

            //  try to get the FCB again

            UtilSleep( 10 );
            cRetries++;
            goto RetrieveFCB;
        }
        Call( err );

        tcScope->nParentObjectClass = TceFromFUCB( *ppfucb );
    }
    else
    {
        tcScope->nParentObjectClass = pfcb->TCE();

        if ( fFCBStateInitialized == fState )
        {
            Assert( pfcb->WRefCount() >= 1);
            err = ErrBTOpen( ppib, pfcb, ppfucb );

            // Cursor has been opened on FCB, so refcount should be
            // at least 2 (one for cursor, one for call to PfcbFCBGet()).
            // (if ErrBTOpen returns w/o error)
            Assert( pfcb->WRefCount() > 1 || (1 == pfcb->WRefCount() && err < JET_errSuccess) );

            pfcb->Release();
        }
        else
        {
            FireWall( "DeprecatedSentinelFcbBtOpen" ); // Sentinel FCBs are believed deprecated
            Assert( !FFMPIsTempDB( ifmp ) );     // Sentinels not used by sort/temp. tables.

            // If we encounter a sentinel, it means the
            // table has been locked for subsequent deletion.
            err = ErrERRCheck( JET_errTableLocked );
        }
    }

HandleError:
    return err;
}


//  *************************************************
//  movement operations
//

//  positions cursor on root page of tree
//  this is root page of data/index for user cursors
//      and root of AvailExt or OwnExt trees for Space cursors
//  page is latched Read or RIW
//
ERR ErrBTIGotoRoot( FUCB *pfucb, LATCH latch )
{
    ERR     err;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTOpen );

    //  should have no page latched
    //
    Assert( !Pcsr( pfucb )->FLatched() );

    CallR( Pcsr( pfucb )->ErrGetPage( pfucb->ppib,
                                      pfucb->ifmp,
                                      PgnoRoot( pfucb ),
                                      latch,
                                      PBFLatchHintPgnoRoot( pfucb )
                                      ) );
    Pcsr( pfucb )->SetILine( 0 );

    // If fPageFDPDelete flag is set on the root page, we need to prevent any actions apart from delete on the table which was originally deleted using non-revertable flag
    // and the database was then reverted to a previous state using RBS causing the table's pages to not be reverted
    // but table root page and space tree pages were reverted to assist in catalog and space tree cleanup.
    //
    if ( Pcsr( pfucb )->Cpage().FPageFDPRootDelete() )
    {
        // TODO: Lots of space operations call this. Should those be handled separately or allowed to fail?
        if ( pfucb->u.pfcb->FPpibAllowRBSFDPDeleteReadByMe( pfucb->ppib ) )
        {
            pfucb->u.pfcb->SetRevertedFDPToDelete();
        }
        else if ( !pfucb->ppib->FSessionLeakReport() )
        {
            const OBJID objidFDP = pfucb->u.pfcb->ObjidFDP();
            FUCBIllegalOperationFDPToBeDeleted( pfucb, objidFDP == 0 ? Pcsr( pfucb )->Cpage().ObjidFDP() : objidFDP );

            // Unlatch the page so that closing FUCB doesn't leak the latch.
            Pcsr( pfucb )->ReleasePage( fTrue );
            return ErrERRCheck( JET_errRBSFDPToBeDeleted );
        }
    }

    return JET_errSuccess;
}

ERR ErrBTIOpenAndGotoRoot( PIB *ppib, const PGNO pgnoFDP, const IFMP ifmp, FUCB **ppfucb )
{
    ERR     err;
    FUCB    *pfucb;

    CallR( ErrBTIOpen( ppib, ifmp, pgnoFDP, objidNil, openNormal, &pfucb, fFalse ) );
    Assert( pfucbNil != pfucb );
    Assert( pfcbNil != pfucb->u.pfcb );
    Assert( pfucb->u.pfcb->FInitialized() );

    err = ErrBTIGotoRoot( pfucb, latchRIW );
    if ( err < JET_errSuccess )
    {
        BTClose( pfucb );
    }
    else
    {
        Assert( latchRIW == Pcsr( pfucb )->Latch() );
        Assert( pcsrNil == pfucb->pcsrRoot );
        pfucb->pcsrRoot = Pcsr( pfucb );

        *ppfucb = pfucb;
    }

    return err;
}

//  this is the uncommon case in the refresh logic
//  where we lost physical currency on page
//
ERR ErrBTIIRefresh( FUCB *pfucb, LATCH latch )
{
    ERR err = JET_errSuccess;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTRefresh );

    DBTIME  dbtimeLast = Pcsr( pfucb )->Dbtime();

    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !pfucb->bmCurr.key.FNull() );
    Assert( latchReadTouch == latch || latchReadNoTouch == latch ||
            latchRIW == latch );

    if ( pgnoNull != Pcsr( pfucb )->Pgno() )
    {
        //  get page latched as per request
        //
        err = Pcsr( pfucb )->ErrGetPage( pfucb->ppib,
                                         pfucb->ifmp,
                                         Pcsr( pfucb )->Pgno(),
                                         latch,
                                         NULL,
                                         fTrue );

        //  this may be a trimmed or shrunk page, so take the slow path to re-establish currency. If we have a real
        //  problem (unexpected uninitialized or beyond EOF page), the slow path will catch it.
        //
        if ( ( err == JET_errPageNotInitialized ) ||
                ( err == JET_errFileIOBeyondEOF ) ||
                ( ( err >= JET_errSuccess ) && ( Pcsr( pfucb )->Dbtime() == dbtimeShrunk ) ) )
        {
            // This is unexpected if trim and shrink are disabled. In that case, we'll fallback to
            // the slow path, just in case.
            if ( !g_rgfmp[ pfucb->ifmp ].FTrimSupported() && !g_rgfmp[ pfucb->ifmp ].FShrinkDatabaseEofOnAttach() )
            {
                FireWall( OSFormat( "BtRefreshUnexpectedErr:%d", err ) );
            }

            err = JET_errSuccess;
            BTUp( pfucb );
            goto LookupBookmark;
        }
        Call( err );

        //  check if DBTime of page is same as last seen by CSR
        //
        if ( Pcsr( pfucb )->Dbtime() == dbtimeLast )
        {
            //  page is same as last seen by cursor
            //
            Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
            Assert( Pcsr( pfucb )->Cpage().ObjidFDP() == ObjidFDP( pfucb ) );
            NDGet( pfucb );

            AssertNDGet( pfucb );
            return JET_errSuccess;
        }

        Assert( Pcsr( pfucb )->Dbtime() > dbtimeLast );

        //  check if node still belongs to latched page
        //
        err = ErrBTISeekInPage( pfucb, pfucb->bmCurr );

        if ( JET_errSuccess == err )
        {
            //  fast path success, return immediately
            goto HandleError;
        }

        //  smart refresh did not work
        //  use bookmark to seek to node
        //
        BTUp( pfucb );
        Call( err );

        if ( wrnNDFoundGreater == err || wrnNDFoundLess == err )
        {
            Error( ErrERRCheck( JET_errRecordDeleted ) );
        }

        Assert( wrnNDNotFoundInPage == err );
    }

LookupBookmark:
    //  Although the caller said no need to touch, but the buffer of the bookmark
    //  never touched before, refresh as touch.

    if ( latch == latchReadNoTouch && pfucb->fTouch )
    {
        latch = latchReadTouch;
    }

    Call( ErrBTGotoBookmark( pfucb, pfucb->bmCurr, latch ) );

    pfucb->fTouch = fFalse;

    AssertNDGet( pfucb );

HandleError:
    if ( err >= 0 )
    {
        CallS( err );
        Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
        Assert( Pcsr( pfucb )->Cpage().ObjidFDP() == ObjidFDP( pfucb ) );
        AssertNDGet( pfucb );
    }

    return err;
}

LOCAL BOOL FBTIEligibleForOLD2( FUCB * const pfucb )
{
    return BoolParam( PinstFromPfucb( pfucb ), JET_paramDefragmentSequentialBTrees )
        && !PinstFromPfucb(pfucb)->FRecovering()
        && pfucb->u.pfcb->FUseOLD2()
        && g_rgfmp[pfucb->ifmp].FSeekPenalty()
        && !pfucb->u.pfcb->FOLD2Running()
        && !g_rgfmp[pfucb->ifmp].FReadOnlyAttach()
        && !g_rgfmp[pfucb->ifmp].FShrinkIsRunning()
        && !g_fRepair;
}

// Register the current table for defragmentation by OLD2. This can be
// called with a page latched so we avoid triggering the deadlock-detection
// code by dispatching a task to register the table
LOCAL ERR ErrBTIRegisterForOLD2( FUCB * const pfucb )
{
    ERR err = JET_errSuccess;
    DBREGISTEROLD2TASK * ptask = NULL;

    OnDebug( BOOL fTaskPosted = fFalse );

    if( FBTIEligibleForOLD2( pfucb ) )
    {
        CHAR szTableName[JET_cbNameMost+1];

        if ( FFUCBPrimary( pfucb ) )
        {
            pfucb->u.pfcb->EnterDML();
            OSStrCbCopyA( szTableName, sizeof( szTableName ), pfucb->u.pfcb->Ptdb()->SzTableName() );
            pfucb->u.pfcb->LeaveDML();
            Alloc( ptask = new DBREGISTEROLD2TASK( pfucb->ifmp, szTableName, NULL, defragtypeTable ) );
        }

        if ( PinstFromIfmp( pfucb->ifmp )->m_pver->m_fSyncronousTasks || g_rgfmp[ pfucb->ifmp ].FDetachingDB() )
        {
            // don't start the task because the task manager is no longer running
            // and we can't run it synchronously because it will deadlock.
            Error( ErrERRCheck( JET_errTaskDropped ) );
        }

        Call( PinstFromIfmp( pfucb->ifmp )->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask ) );

        OnDebug( fTaskPosted = fTrue; );

        ptask = NULL;
        // WARNING: Please do not overwrite the err after this point. 
        // Otherwise successfully posted task may get deleted by its caller.
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        Assert ( !fTaskPosted );
        delete ptask;
    }
    return err;
}

//  deletes a node and blows it away
//  performs single-page cleanup or multipage cleanup, if possible
//  this is called from VER cleanup or other cleanup threads
//
INLINE ERR ErrBTIDelete( FUCB *pfucb, const BOOKMARK& bm )
{
    ERR     err;

    ASSERT_VALID( &bm );
    Assert( !Pcsr( pfucb )->FLatched( ) );
    Assert( !FFUCBSpace( pfucb ) );
    Assert( !FFMPIsTempDB( pfucb->ifmp ) );

    auto tc = TcCurr();
    PERFOpt( PERFIncCounterTable( cBTDelete, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );

    //  try to delete node, if multi-page operation is not needed
    //
    CallR( ErrBTISinglePageCleanup( pfucb, bm ) );
    Assert( !Pcsr( pfucb )->FLatched() );

    if ( wrnBTMultipageOLC == err )
    {
        //  multipage operations are needed to cleanup page
        //  here we try a right merge first, and then a left merge
        //

        MERGETYPE mergetype;
        err = ErrBTIMultipageCleanup( pfucb, bm, NULL, NULL, &mergetype, fTrue );
        if ( errBTMergeNotSynchronous == err )
        {
            //  ignore merge conflicts
            err = JET_errSuccess;
        }
        else if ( mergetypeNone == mergetype )
        {
            err = ErrBTIMultipageCleanup( pfucb, bm, NULL, NULL, &mergetype, fFalse );
            if ( errBTMergeNotSynchronous == err )
            {
                //  ignore merge conflicts
                err = JET_errSuccess;
            }
        }
    }

    return err;
}

ERR ErrBTDelete( FUCB *pfucb, const BOOKMARK& bm )
{
    ERR     err;
    PIB *   ppib            = pfucb->ppib;
    BOOL    fInTransaction  = fFalse;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTDelete );

    //  UNDONE: is a transaction really needed?  None of the operations
    //  performed by single/multi-page cleanup are versioned, so it's not
    //  for rollback reasons. Probably not visibility either, since I
    //  believe single/multi-page cleanup ignores version bits when
    //  looking at nodes. Are we worried about the visibility of other
    //  transactions? Or maybe are we worried that if we don't begin a
    //  transaction here, version cleanup might interfere?
    //
    //  RESOLVED: The reason we need a transaction is to ensure that
    //  the parent FCB is not purged out from underneath us.
    //  The scenario is:
    //      - DELETERECTASK on LV tree is dispatched
    //      - table is deleted
    //      - version cleanup purges table FCB, then waits for
    //        all tasks on LV to complete
    //      - DELETERECTASK is executed, space is freed to table
    //        btree, but FCB for the table has already been purged
    //  If we wrap the DELETERECTASK in a transaction, then check
    //  to make sure that the FCB is not flagged as DeletePending,
    //  we're guaranteed that any btree deletion (eg. table delete)
    //  will not be processed by version cleanup until after this
    //  transaction has committed.
    //
    Assert( !PinstFromPpib( ppib )->FRecovering() || fRecoveringUndo == PinstFromPpib( ppib )->m_plog->FRecoveringMode() );

    if ( ppib->Level() == 0 )
    {
        CallR( ErrDIRBeginTransaction( ppib, 52965, bitTransactionWritableDuringRecovery ) );
        fInTransaction = fTrue;
    }

    err = ErrBTIDelete( pfucb, bm );

    //  the node may have gotten expunged by someone else or may have
    //  gotten undeleted (because a new node was inserted with the same key)
    //
    switch ( err )
    {
        case JET_errSuccess:
        case JET_errNoCurrentRecord:
        case JET_errRecordNotDeleted:
            err = JET_errSuccess;
            break;
        default:
            Call( err );
    }

    //  no versioned updates performed, so just force success
    //
    Assert( prceNil == ppib->prceNewest || operWriteLock == ppib->prceNewest->Oper() );

    if ( fInTransaction )
    {
        Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
        fInTransaction = fFalse;
    }

HandleError:
    if ( err < JET_errSuccess && fInTransaction )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}


//  returns number of bytes to leave free in page
//  to satisfied density constraint
//
INLINE INT CbBTIFreeDensity( const FUCB *pfucb )
{
    Assert( pfucb->u.pfcb != pfcbNil );
    return ( (INT) pfucb->u.pfcb->CbDensityFree() );
}


//  returns required space for inserting a node into given page
//  used by split for estimating cbReq for internal page insertions
//
LOCAL ULONG CbBTICbReq( FUCB *pfucb, CSR *pcsr, const KEYDATAFLAGS& kdf )
{
    Assert( pcsr->Latch() == latchRIW );
    Assert( !pcsr->Cpage().FLeafPage() );
    Assert( sizeof( PGNO )== kdf.data.Cb() );

    //  temporary kdf to accommodate
    KEYDATAFLAGS    kdfT = kdf;

    //  get prefix from page
    //
    const INT   cbCommon = CbNDCommonPrefix( pfucb, pcsr, kdf.key );

    if ( cbCommon > cbPrefixOverhead )
    {
        kdfT.key.prefix.SetCb( cbCommon );
        #ifdef  DEBUG
        kdfT.key.prefix.SetPv( (VOID *)lBitsAllFlipped );
        #endif
        kdfT.key.suffix.SetCb( kdf.key.Cb() - cbCommon );
        kdfT.fFlags = fNDCompressed;
    }
    else
    {
        kdfT.key.prefix.SetCb( 0 );
        kdfT.key.suffix.SetCb( kdf.key.Cb() );
    }

    const   ULONG   cbReq = CbNDNodeSizeCompressed( kdfT );
    return cbReq;
}


//  returns cbCommon for given key
//  with respect to prefix in page
//
INT CbNDCommonPrefix( FUCB *pfucb, CSR *pcsr, const KEY& key )
{
    Assert( pcsr->FLatched() );

    //  get prefix from page
    //
    NDGetPrefix( pfucb, pcsr );
    Assert( pfucb->kdfCurr.key.suffix.Cb() == 0 );

    const ULONG cbCommon = CbCommonKey( key, pfucb->kdfCurr.key );

    return cbCommon;
}


//  computes prefix for a given key with respect to prefix key in page
//  reorganizes key in given kdf to reflect prefix
//
VOID BTIComputePrefix( FUCB         *pfucb,
                       CSR          *pcsr,
                       const KEY&   key,
                       KEYDATAFLAGS *pkdf )
{
    Assert( key.prefix.Cb() == 0 );
    Assert( pkdf->key.prefix.FNull() );
    Assert( pcsr->FLatched() );

    INT     cbCommon = CbNDCommonPrefix( pfucb, pcsr, key );

    if ( cbCommon > cbPrefixOverhead )
    {
        //  adjust inserted key to reflect prefix
        //
        pkdf->key.prefix.SetCb( cbCommon );
        pkdf->key.prefix.SetPv( key.suffix.Pv() );
        pkdf->key.suffix.SetCb( key.suffix.Cb() - cbCommon );
        pkdf->key.suffix.SetPv( (BYTE *)key.suffix.Pv() + cbCommon );

        pkdf->fFlags = fNDCompressed;
    }
    else
    {
    }

    return;
}


//  decides if a particular insert should be treated as an append
//
INLINE BOOL FBTIAppend( const FUCB *pfucb, CSR *pcsr, ULONG cbReq, const BOOL fUpdateUncFree )
{
    //  cbReq includes line TAG overhead and must be compared against CbNDPageAvailMostNoInsert
    //
    Assert( cbReq <= CbNDPageAvailMostNoInsert( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
    Assert( pcsr->FLatched() );

    //  adjust cbReq for density constraint
    //
    if ( pcsr->Cpage().FLeafPage()
        && !pcsr->Cpage().FSpaceTree() )        //  100% density on space trees
    {
        cbReq += CbBTIFreeDensity( pfucb );
    }

    //  last page in tree
    //  inserting past the last node in page
    //  and inserting a node of size cbReq violates density contraint
    //
    return ( pgnoNull == pcsr->Cpage().PgnoNext()
            && pcsr->ILine() == pcsr->Cpage().Clines()
            && !FNDFreePageSpace( pfucb, pcsr, cbReq, fUpdateUncFree ) );
}


INLINE BOOL FBTISplit( const FUCB *pfucb, CSR *pcsr, const ULONG cbReq, const BOOL fUpdateUncFree )
{
    return !FNDFreePageSpace( pfucb, pcsr, cbReq, fUpdateUncFree );
}


//  finds max size of node in pfucb->kdfCurr
//  checks version store to find reserved space for
//  uncommitted nodes
//
LOCAL ULONG CbBTIMaxSizeOfNode( const FUCB * const pfucb, const CSR * const pcsr )
{
    if ( FNDPossiblyVersioned( pfucb, pcsr ) )
    {
        BOOKMARK    bm;
        INT         cbData;

        NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
        cbData = CbVERGetNodeMax( pfucb, bm );

        if ( cbData >= pfucb->kdfCurr.data.Cb() )
        {
            return CbNDNodeSizeTotal( pfucb->kdfCurr ) - pfucb->kdfCurr.data.Cb() + cbData;
        }
    }

    return CbNDNodeSizeTotal( pfucb->kdfCurr );
}


//  split page and perform operation
//  update operation to be be performed can be
//      Insert:     node to insert is in kdf
//      Replace:    data to replace with is in kdf
//      FlagInsertAndReplaceData:   node to insert is in kdf
//                  [for unique trees that
//                   already have a flag-deleted node
//                   with the same key]
//
//  cursor is placed on node to replace, or insertion point
//
ERR ErrBTISplit( FUCB           * const pfucb,
                 KEYDATAFLAGS   * const pkdf,
                 const DIRFLAG  dirflagT,
                 const RCEID    rceid1,
                 const RCEID    rceid2,
                 RCE            * const prceReplace,
                 const INT      cbDataOld,
                 const VERPROXY * const pverproxy
                 )
{
    ERR         err;
    BOOL        fOperNone   = fFalse;
    SPLITPATH   *psplitPath = NULL;
    INST        *pinst = PinstFromIfmp( pfucb->ifmp );

    Assert( rceid2 == Rceid( prceReplace )
            || rceid1 == Rceid( prceReplace ) );

    //  copy flags into local, since it can be changed by SelectSplitPath
    //
    DIRFLAG     dirflag     = dirflagT;
    BOOL        fVersion    = !( dirflag & fDIRNoVersion ) && !g_rgfmp[ pfucb->ifmp ].FVersioningOff();
    const BOOL  fLogging    = !( dirflag & fDIRNoLog ) && g_rgfmp[pfucb->ifmp].FLogOn();

    ASSERT_VALID( pkdf );

#ifdef DEBUG
    Assert( !fVersion || rceidNull != rceid1 );
    Assert( pfucb->ppib->Level() > 0 || !fVersion );
    Assert( dirflag & fDIRInsert ||
            dirflag & fDIRReplace ||
            dirflag & fDIRFlagInsertAndReplaceData );
    Assert( !( dirflag & fDIRInsert && dirflag & fDIRReplace ) );
    if ( NULL != pverproxy )
    {
        Assert( !PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() );
        Assert( proxyCreateIndex == pverproxy->proxy );
    }
    Assert( pkdf->key.prefix.FNull() );
#endif  //  DEBUG

    //  seek from root
    //  RIW latching all intermediate pages
    //      and right sibling of leaf page
    //  also retry the operation
    //
    err = ErrBTICreateSplitPathAndRetryOper(
                pfucb,
                pkdf,
                &psplitPath,
                &dirflag,
                rceid1,
                rceid2,
                prceReplace,
                pverproxy );

    if ( JET_errSuccess == err )
    {
        Assert( psplitPath != NULL ) ;
        //  performed operation successfully
        //  set cursor to leaf page,
        //  release splitPath and return
        //
        const INT   ilineT      = psplitPath->csr.ILine();
        *Pcsr( pfucb ) = psplitPath->csr;
        Pcsr( pfucb )->SetILine( ilineT );
        Assert( Pcsr( pfucb )->FLatched() );
        goto HandleError;
    }
    else if ( err != errPMOutOfPageSpace )
    {
        Assert( err < 0 );
        Call( err );
    }

    Assert( psplitPath != NULL ) ;

    //  select split
    //      -- this selects split of parents too
    //
    Call( ErrBTISelectSplit( pfucb, psplitPath, pkdf, dirflag ) );
    BTISplitCheckPath( psplitPath );
    if ( NULL == psplitPath->psplit ||
         splitoperNone == psplitPath->psplit->splitoper )
    {
        //  save err if operNone
        //
        fOperNone = fTrue;
    }

    //  get new pages
    //
    Call( ErrBTIGetNewPages( pfucb, psplitPath, dirflag ) );

    //  release latch on unnecessary pages
    //
    BTISplitReleaseUnneededPages( pinst, &psplitPath );
    Assert( psplitPath->psplit != NULL );

    //  write latch remaining pages in order
    //  flag pages dirty and set each with max dbtime of the pages
    //
    Call( ErrBTISplitUpgradeLatches( pfucb->ifmp, psplitPath ) );

    //  The logging code will log the iline currently in the CSR of the
    //  psplitPath. The iline is ignored for recovery, but its nice to
    //  have it set to something sensible for debugging

    psplitPath->csr.SetILine( psplitPath->psplit->ilineOper );

    //  log split -- macro logging for whole split
    //
    if ( fLogging )
    {
        LGPOS   lgpos;

        err = ErrLGSplit( pfucb,
                      psplitPath,
                      *pkdf,
                      rceid1,
                      rceid2,
                      dirflag,
                      dirflagT,
                      &lgpos,
                      pverproxy );

        // on error, return to before dirty dbtime on all pages
        if ( err < JET_errSuccess )
        {
            BTISplitRevertDbtime( psplitPath );
            goto HandleError;
        }

        BTISplitSetLgpos( psplitPath, lgpos );
    }

    //  NOTE: after logging succeeds, nothing should fail...
    //
    if ( prceNil != prceReplace && !fOperNone )
    {
        const INT cbData    = pkdf->data.Cb();

        //  set uncommitted freed space for shrinking node
        VERSetCbAdjust( Pcsr( pfucb ), prceReplace, cbData, cbDataOld, fDoNotUpdatePage );
    }

    //  perform split
    //  insert parent page pointers
    //  fix sibling page pointers at leaf
    //  move nodes
    //  set dependencies [optional, depending upon page dependencies param]
    //
    BTIPerformSplit( pfucb, psplitPath, pkdf, dirflag );

    BTICheckSplits( pfucb, psplitPath, pkdf, dirflag );

    //  shrink / optimize page image
    //
    BTISplitShrinkPages( psplitPath );

    //  move cursor to leaf page [write-latched]
    //  and iLine to operNode
    //
    BTISplitSetCursor( pfucb, psplitPath );

    //  release all splitPaths
    //
    BTIReleaseSplitPaths( pinst, psplitPath );

    if ( fOperNone )
    {
        Assert( !Pcsr( pfucb )->FLatched() );
        err = ErrERRCheck( errBTOperNone );

        if( prceNil != prceReplace )
        {
            //  UNDONE: we don't have to nullify the RCE here. We could keep it and reuse it. No-one
            //  else can alter the node because we have a version on it.
            Assert( fVersion );
            VERNullifyFailedDMLRCE( prceReplace );
        }
    }
    else
    {
        Assert( Pcsr( pfucb )->FLatched() );
    }

    return err;

HandleError:
    //  release splitPath
    //
    if ( psplitPath != NULL )
    {
        BTIReleaseSplitPaths( pinst, psplitPath );
    }

    return err;
}


//  creates path of RIW-latched pages from root for split to work on
//  if DBTime or leaf pgno has changed after RIW latching path,
//      retry operation
//
ERR ErrBTICreateSplitPathAndRetryOper( FUCB             * const pfucb,
                                       const KEYDATAFLAGS * const pkdf,
                                       SPLITPATH        **ppsplitPath,
                                       DIRFLAG  * const pdirflag,
                                       const RCEID rceid1,
                                       const RCEID rceid2,
                                       RCE * const prceReplace,
                                       const VERPROXY * const pverproxy )
{
    ERR         err;
    BOOKMARK    bm;
    PGNO        pgnoSplit = Pcsr( pfucb )->Pgno();
    DBTIME      dbtimeLast = Pcsr( pfucb )->Dbtime();

    Assert( Pcsr( pfucb )->FLatched() );
    Assert( NULL == pfucb->pvRCEBuffer );
    Pcsr( pfucb )->ReleasePage();

    //  initialize bookmark to seek for
    //
    NDGetBookmarkFromKDF( pfucb, *pkdf, &bm );

    if ( *pdirflag & fDIRInsert )
    {
        Assert( bm.key.Cb() == pkdf->key.Cb() );
        //  [ldb 4/30/96]: assert below _may_ go off wrongly. possibly use CmpBm == 0
        Assert( bm.key.suffix.Pv() == pkdf->key.suffix.Pv() );
        Assert( bm.key.prefix.FNull() );
    }
    else if ( *pdirflag & fDIRFlagInsertAndReplaceData )
    {
        Call( ErrBTISaveBookmark( pfucb, bm, fFalse ) );
    }
    else
    {
        //  get key from cursor
        //
        Assert( *pdirflag & fDIRReplace );
        bm.key = pfucb->bmCurr.key;
    }

    Call( ErrBTICreateSplitPath( pfucb, bm, ppsplitPath ) );
    Assert( (*ppsplitPath)->csr.Cpage().FLeafPage() );

    //  set iline to point of insert
    //
    if ( wrnNDFoundLess == err )
    {
        Assert( (*ppsplitPath)->csr.Cpage().Clines() - 1 ==  (*ppsplitPath)->csr.ILine() );
        (*ppsplitPath)->csr.SetILine( (*ppsplitPath)->csr.Cpage().Clines() );
    }

    //  retry operation if timestamp / pgno has changed
    //
    if ( (*ppsplitPath)->csr.Pgno() != pgnoSplit ||
         (*ppsplitPath)->csr.Dbtime() != dbtimeLast )
    {
        CSR     *pcsr = &(*ppsplitPath)->csr;

        if ( fDIRReplace & *pdirflag )
        {
            //  check if replace fits in page
            //
            AssertNDGet( pfucb, pcsr );
            const INT  cbReq = pfucb->kdfCurr.data.Cb() >= pkdf->data.Cb() ?
                                  0 :
                                  pkdf->data.Cb() - pfucb->kdfCurr.data.Cb();

            if ( cbReq > 0 && FBTISplit( pfucb, pcsr, cbReq ) )
            {
                Error( ErrERRCheck( errPMOutOfPageSpace ) );
            }

            //  upgrade to write latch
            //
            pcsr->UpgradeFromRIWLatch();
            Assert( latchWrite == pcsr->Latch() );

            //  try to replace node data with new data
            //
            err = ErrNDReplace( pfucb, pcsr, &pkdf->data, *pdirflag, rceid1, prceReplace );
            Assert( errPMOutOfPageSpace != err );
            Call( err );
        }
        else
        {
            ULONG   cbReq;

            Assert( ( fDIRInsert & *pdirflag ) ||
                    ( fDIRFlagInsertAndReplaceData & *pdirflag ) );

            //  UNDONE: copy code from BTInsert
            //  set *pdirflag
            //
            //  if seek succeeded
            //      if unique
            //          flag insert and replace data
            //      else
            //          flag insert
            //  else
            //      insert whole node
            //
            if ( JET_errSuccess == err )
            {
                //  seek succeeded
                //
                //  can not have two nodes with same bookmark (attempts
                //  to do so should have been caught before split)
                //
#ifdef DEBUG
                FUCBResetUpdatable( pfucb );    //  don't reset the version bit in FNDPotVisibleToCursor
                Assert( FNDDeleted( pfucb->kdfCurr ) );
                Assert( !FNDPotVisibleToCursor( pfucb, pcsr ) );
                FUCBSetUpdatable( pfucb );
#endif  //  DEBUG

                if ( FFUCBUnique( pfucb ) )
                {
                    Assert( *pdirflag & fDIRFlagInsertAndReplaceData );

                    //  calcualte space requred
                    //  if new data fits, flag insert node and replace data
                    //
                    cbReq = pkdf->data.Cb() > pfucb->kdfCurr.data.Cb() ?
                                pkdf->data.Cb() - pfucb->kdfCurr.data.Cb() :
                                0;

                    if ( FBTISplit( pfucb, pcsr, cbReq ) )
                    {
                        Error( ErrERRCheck( errPMOutOfPageSpace ) );
                    }

                    //  upgrade to write latch
                    //
                    pcsr->UpgradeFromRIWLatch();
                    Assert( latchWrite == pcsr->Latch() );

                    //  flag insert node and replace data
                    //
                    Call( ErrNDFlagInsertAndReplaceData( pfucb,
                                                         pcsr,
                                                         pkdf,
                                                         *pdirflag,
                                                         rceid1,
                                                         rceid2,
                                                         prceReplace,
                                                         pverproxy ) );
                }
                else
                {
                    //  should never happen, because:
                    //      - if this is an insert, the dupe should
                    //        have been caught by BTInsert() before
                    //        the split
                    //      - if this is a flag-insert, it wouldn't
                    //        have caused a split
                    //      - FlagInsertAndReplaceData doesn't happen
                    //        on non-unique indexes
                    Assert( fFalse );
                    Assert( *pdirflag & fDIRInsert );
                    Assert( 0 == CmpKeyData( bm, pfucb->kdfCurr ) );

                    if ( !FNDDeleted( pfucb->kdfCurr )
                        || FNDPotVisibleToCursor( pfucb, pcsr ) )
                    {
                        Error( ErrERRCheck( JET_errMultiValuedIndexViolation ) );
                    }

                    //  upgrade to write latch
                    //
                    pcsr->UpgradeFromRIWLatch();
                    Assert( latchWrite == pcsr->Latch() );

                    //  no additional space required
                    //
                    Assert( fFalse );
                    Call( ErrNDFlagInsert( pfucb, pcsr, *pdirflag, rceid1, pverproxy ) );
                }
            }
            else
            {
                //  insert node if it fits
                //
                KEYDATAFLAGS kdfCompressed = *pkdf;

                //  if we were doing a flag insert and replace data and didn't find the node
                //  then it has been removed. change the operation to an ordinary insert
                *pdirflag &= ~fDIRFlagInsertAndReplaceData;
                *pdirflag |= fDIRInsert;

                BTIComputePrefix( pfucb, pcsr, pkdf->key, &kdfCompressed );
                Assert( !FNDCompressed( kdfCompressed ) ||
                        kdfCompressed.key.prefix.Cb() > 0 );

                cbReq = CbNDNodeSizeCompressed( kdfCompressed );

                if ( FBTISplit( pfucb, pcsr, cbReq ) || FBTIAppend( pfucb, pcsr, cbReq ) )
                {
                    Error( ErrERRCheck( errPMOutOfPageSpace ) );
                }

                //  upgrade to write latch
                //
                pcsr->UpgradeFromRIWLatch();
                Assert( latchWrite == pcsr->Latch() );

                Call( ErrNDInsert( pfucb, pcsr, &kdfCompressed, *pdirflag, rceid1, pverproxy ) );
            }
        }

        Assert( errPMOutOfPageSpace != err );
        CallS( err );
    }
    else
    {
        err = ErrERRCheck( errPMOutOfPageSpace );
    }

HandleError:
    Assert( errPMOutOfPageSpace != err ||
            latchRIW == (*ppsplitPath)->csr.Latch() );
    return err;
}


//  creates splitPath of RIW latched pages from root of tree
//  to seeked bookmark
//
LOCAL ERR   ErrBTICreateSplitPath( FUCB             *pfucb,
                                   const BOOKMARK&  bm,
                                   SPLITPATH        **ppsplitPath )
{
    ERR     err;
    BOOL    fLeftEdgeOfBtree    = fTrue;
    BOOL    fRightEdgeOfBtree   = fTrue;

    //  create splitPath structure
    //
    Assert( NULL == *ppsplitPath );
    CallR( ErrBTINewSplitPath( ppsplitPath ) );
    Assert( NULL != *ppsplitPath );

    //  RIW latch root
    //
    Call( (*ppsplitPath)->csr.ErrGetRIWPage( pfucb->ppib,
                                             pfucb->ifmp,
                                             PgnoRoot( pfucb ) ) );

    if ( 0 == (*ppsplitPath)->csr.Cpage().Clines() )
    {
        (*ppsplitPath)->csr.SetILine( -1 );
        err = ErrERRCheck( wrnNDFoundLess );
        goto HandleError;
    }

    for ( ; ; )
    {
        Assert( (*ppsplitPath)->csr.Cpage().Clines() > 0 );
        if( g_fRepair
            && FFUCBRepair( pfucb )
            && bm.key.Cb() == 0 )
        {
            //  when creating a repair tree we want NULL keys to go at the end, not the beginning
            NDMoveLastSon( pfucb, &(*ppsplitPath)->csr );
            err = ErrERRCheck( wrnNDFoundLess );
        }
        else
        {
            Call( ErrNDSeek( pfucb,
                         &(*ppsplitPath)->csr,
                         bm ) );
        }

        Assert( (*ppsplitPath)->csr.ILine() < (*ppsplitPath)->csr.Cpage().Clines() );

        if ( (*ppsplitPath)->csr.Cpage().FLeafPage() )
        {
            const SPLITPATH * const     psplitPathParent        = (*ppsplitPath)->psplitPathParent;

            if ( NULL != psplitPathParent )
            {
                Assert( !( (*ppsplitPath)->csr.Cpage().FRootPage() ) );

                const BOOL              fLeafPageIsFirstPage    = ( pgnoNull == (*ppsplitPath)->csr.Cpage().PgnoPrev() );
                const BOOL              fLeafPageIsLastPage     = ( pgnoNull == (*ppsplitPath)->csr.Cpage().PgnoNext() );

                if ( fLeftEdgeOfBtree ^ fLeafPageIsFirstPage )
                {
                    //  if not repair, assert, otherwise, suppress the assert
                    //  and repair will just naturally err out
                    AssertSz( g_fRepair, "Corrupt B-tree: first leaf page has mismatched parent" );
                    Call( ErrBTIReportBadPageLink(
                                pfucb,
                                ErrERRCheck( JET_errBadParentPageLink ),
                                psplitPathParent->csr.Pgno(),
                                (*ppsplitPath)->csr.Pgno(),
                                (*ppsplitPath)->csr.Cpage().PgnoPrev(),
                                fTrue,
                                "BtSplitParentMismatchFirst" ) );
                }
                if ( fRightEdgeOfBtree ^ fLeafPageIsLastPage )
                {
                    //  if not repair, assert, otherwise, suppress the assert
                    //  and repair will just naturally err out
                    AssertSz( g_fRepair, "Corrupt B-tree: last leaf page has mismatched parent" );
                    Call( ErrBTIReportBadPageLink(
                                pfucb,
                                ErrERRCheck( JET_errBadParentPageLink ),
                                psplitPathParent->csr.Pgno(),
                                (*ppsplitPath)->csr.Pgno(),
                                (*ppsplitPath)->csr.Cpage().PgnoNext(),
                                fTrue,
                                "BtSplitParentMismatchLast" ) );
                }
            }
            else
            {
                Assert( (*ppsplitPath)->csr.Cpage().FRootPage() );
            }

            break;
        }

        Assert( (*ppsplitPath)->csr.Cpage().FInvisibleSons() );
        Assert( !( fRightEdgeOfBtree ^ (*ppsplitPath)->csr.Cpage().FLastNodeHasNullKey() ) );

        fRightEdgeOfBtree = ( fRightEdgeOfBtree
                            && (*ppsplitPath)->csr.ILine() == (*ppsplitPath)->csr.Cpage().Clines() - 1 );
        fLeftEdgeOfBtree = ( fLeftEdgeOfBtree
                            && 0 == (*ppsplitPath)->csr.ILine() );

        //  allocate another splitPath structure for next level
        //
        Call( ErrBTINewSplitPath( ppsplitPath ) );

        //  access child page
        //
        Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
        Call( (*ppsplitPath)->csr.ErrGetRIWPage(
                                pfucb->ppib,
                                pfucb->ifmp,
                                *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv() ) );
    }

HandleError:
    return err;
}


//  creates a new SPLITPATH structure and initializes it
//      adds newly created splitPath structure to head of list
//      pointed to by *ppSplitPath passed in
//
ERR ErrBTINewSplitPath( SPLITPATH **ppsplitPath )
{
    SPLITPATH   *psplitPath;

    if ( !( psplitPath = new SPLITPATH ) )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    psplitPath->psplitPathParent = *ppsplitPath;

    if ( psplitPath->psplitPathParent != NULL )
    {
        Assert( NULL == psplitPath->psplitPathParent->psplitPathChild );
        psplitPath->psplitPathParent->psplitPathChild = psplitPath;
    }

    *ppsplitPath = psplitPath;
    return JET_errSuccess;
}


//  selects split at leaf level
//  recursively calls itself to select split at parent level
//      psplitPath is already created and all required pages RIW latched
//
LOCAL ERR ErrBTISelectSplit( FUCB           *pfucb,
                             SPLITPATH      *psplitPath,
                             KEYDATAFLAGS   *pkdf,
                             DIRFLAG        dirflag )
{
    ERR     err;

/// Assert( pkdf->key.prefix.cb == 0 );

    //  create and initialize split structure
    //  and link to psplitPath
    //
    CallR( ErrBTINewSplit( pfucb, psplitPath, pkdf, dirflag ) );
    Assert( psplitPath->psplit != NULL );
    Assert( psplitPath->psplit->psplitPath == psplitPath );

    SPLIT   *psplit = psplitPath->psplit;
    BTIRecalcWeightsLE( psplit );

    //  if root page
    //      select vertical split
    //
    if ( psplitPath->csr.Cpage().FRootPage() )
    {
        BTISelectVerticalSplit( psplit, pfucb );

        //  calculate uncommitted freed space
        //
        BTISplitCalcUncFree( psplit );

        Call( ErrBTISplitAllocAndCopyPrefixes( pfucb, psplit ) );
        return JET_errSuccess;
    }

    ULONG   cbReq;
    CSR     *pcsrParent;

    //  horizontal split
    //
    if  ( psplit->fAppend )
    {
        BTISelectAppend( psplit, pfucb );
    }
    else
    {
        //  find split point such that the
        //  two pages have almost equal weight
        //
        BTISelectRightSplit( psplit, pfucb );
        Assert( psplit->ilineSplit >= 0 );
    }

    //  calculate uncommitted freed space
    //
    BTISplitCalcUncFree( psplit );

    //  copy page flags
    //
    psplit->fNewPageFlags   =
    psplit->fSplitPageFlags = psplitPath->csr.Cpage().FFlags();

    //  allocate and copy prefixes
    //
    Call( ErrBTISplitAllocAndCopyPrefixes( pfucb, psplit ) );

    //  compute separator key to insert in parent
    //  allocate space for key and link to psplit
    //
    Call( ErrBTISplitComputeSeparatorKey( psplit, pfucb ) );
    Assert( sizeof( PGNO ) == psplit->kdfParent.data.Cb() );

    //  seek to separator key in parent
    //
    Call( ErrBTISeekSeparatorKey( psplit, pfucb ) );

    //  if insert in parent causes split
    //  call BTSelectSplit recursively
    //
    pcsrParent = &psplit->psplitPath->psplitPathParent->csr;
    cbReq = CbBTICbReq( pfucb, pcsrParent, psplit->kdfParent );

    if ( FBTISplit( pfucb, pcsrParent, cbReq ) )
    {
        Call( ErrBTISelectSplit( pfucb,
                                 psplitPath->psplitPathParent,
                                 &psplit->kdfParent,
                                 dirflag ) );

        if ( NULL == psplitPath->psplitPathParent->psplit ||
             splitoperNone == psplitPath->psplitPathParent->psplit->splitoper )
        {
            //  somewhere up the tree, split could not bepsplit->kdfParent performed
            //  along with the operation [insert]
            //  so reset psplit at this level
            //
            delete psplit;
            psplitPath->psplit = NULL;
            return err;
        }
    }

    Assert( psplit->ilineSplit < psplit->clines );
    Assert( splittypeAppend == psplit->splittype ||
            FBTISplitCausesNoOverflow( psplit, psplit->ilineSplit ) );
    Assert( psplitPath->psplit != NULL );
    Assert( psplitPath->psplitPathParent != NULL );

HandleError:
    return err;
}


//  allocates a new SPLIT structure
//  initalizes psplit and links it to psplitPath
//
ERR ErrBTINewSplit(
    FUCB *          pfucb,
    SPLITPATH *     psplitPath,
    KEYDATAFLAGS *  pkdf,
    DIRFLAG         dirflag )
{
    ERR             err;
    SPLIT *         psplit;
    INT             iLineTo;
    INT             iLineFrom;
    BOOL            fPossibleHotpoint       = fFalse;
    VOID *          pvHighest               = NULL;

    Assert( psplitPath != NULL );
    Assert( psplitPath->psplit == NULL );

    //  allocate split structure
    //
    if ( !( psplit = new SPLIT ) )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    psplit->pgnoSplit = psplitPath->csr.Pgno();

    //  initialize split structure
    //  and link to psplitPath
    //
    if ( psplitPath->csr.Cpage().FLeafPage( ) &&
         pgnoNull != psplitPath->csr.Cpage().PgnoNext() )
    {
        //  set right page
        //
        Assert( !psplitPath->csr.Cpage().FRootPage( ) );
        Call( psplit->csrRight.ErrGetRIWPage(
                                        pfucb->ppib,
                                        pfucb->ifmp,
                                        psplitPath->csr.Cpage().PgnoNext( ) ) );

        const BOOL  fBadSiblingPointer  = ( psplit->csrRight.Cpage().PgnoPrev() != psplit->pgnoSplit );
        if( fBadSiblingPointer
            || psplit->csrRight.Cpage().ObjidFDP() != pfucb->u.pfcb->ObjidFDP() )
        {
            const PGNO  pgnoBadLink     = ( fBadSiblingPointer ?
                                                    psplit->csrRight.Cpage().PgnoPrev() :
                                                    psplit->csrRight.Cpage().ObjidFDP() );

            //  if not repair, assert, otherwise, suppress the assert and
            //  repair will just naturally err out
            AssertSz( g_fRepair, "Corrupt B-tree: bad leaf page links detected on Split" );
            Call( ErrBTIReportBadPageLink(
                    pfucb,
                    ErrERRCheck( JET_errBadPageLink ),
                    psplit->pgnoSplit,
                    psplit->csrRight.Pgno(),
                    pgnoBadLink,
                    fTrue,
                    fBadSiblingPointer ?
                        "BtSplitBadLeafPgno" :
                        "BtSplitBadLeafObjid" ) );
        }
    }
    else
    {
        Assert( pgnoNull == psplit->csrRight.Pgno() );
    }

    psplit->psplitPath = psplitPath;

    //  get operation
    //  this will be corrected later to splitoperNone (for leaf pages only)
    //  if split can not still satisfy space requested for operation
    //
    if ( psplitPath->csr.Cpage().FInvisibleSons( ) )
    {
        //  internal pages have only insert operation
        //
        psplit->splitoper = splitoperInsert;
    }
    else if ( dirflag & fDIRInsert )
    {
        Assert( !( dirflag & fDIRReplace ) );
        Assert( !( dirflag & fDIRFlagInsertAndReplaceData ) );
        psplit->splitoper = splitoperInsert;

        //  must have at least two existing nodes to establish a hotpoint pattern
        fPossibleHotpoint = ( !psplitPath->csr.Cpage().FRootPage()
                            && psplitPath->csr.ILine() >= 2 );
    }
    else if ( dirflag & fDIRReplace )
    {
        Assert( !( dirflag & fDIRFlagInsertAndReplaceData ) );
        psplit->splitoper = splitoperReplace;
    }
    else
    {
        Assert( dirflag & fDIRFlagInsertAndReplaceData );
        psplit->splitoper = splitoperFlagInsertAndReplaceData;
    }

    //  allocate line info
    //
    psplit->clines = psplitPath->csr.Cpage().Clines();

    if ( splitoperInsert == psplit->splitoper )
    {
        //  insert needs one more line for inserted node
        //
        psplit->clines++;
    }

    //  allocate one more entry than we need so that BTISelectPrefix can use a sentinel value
    Alloc( psplit->rglineinfo = new LINEINFO[psplit->clines + 1] );

    //  psplitPath->csr is positioned at point of insert/replace
    //
    Assert( splitoperInsert == psplit->splitoper &&
            psplitPath->csr.Cpage().Clines() >= psplitPath->csr.ILine() ||
            psplitPath->csr.Cpage().Clines() > psplitPath->csr.ILine() );

    psplit->ilineOper = psplitPath->csr.ILine();

    for ( iLineFrom = 0, iLineTo = 0; iLineTo < psplit->clines; iLineTo++ )
    {
        if ( psplit->ilineOper == iLineTo &&
             splitoperInsert == psplit->splitoper )
        {
            //  place to be inserted node here
            //
            psplit->rglineinfo[iLineTo].kdf = *pkdf;
            psplit->rglineinfo[iLineTo].cbSizeMax =
            psplit->rglineinfo[iLineTo].cbSize =
                    CbNDNodeSizeTotal( *pkdf );

            if ( fPossibleHotpoint )
            {
                Assert( iLineTo >= 2 );

                //  verify last two nodes before insertion point are
                //  currently last two nodes physically in page
                if ( psplit->rglineinfo[iLineTo-2].kdf.key.suffix.Pv() > pvHighest
                    && psplit->rglineinfo[iLineTo-1].kdf.key.suffix.Pv() > psplit->rglineinfo[iLineTo-2].kdf.key.suffix.Pv() )
                {
                    //  need to guarantee that nodes after
                    //  the insert point are all physically
                    //  located before the nodes in the
                    //  the hotpoint area
                    pvHighest = psplit->rglineinfo[iLineTo-2].kdf.key.suffix.Pv();
                }
                else
                {
                    fPossibleHotpoint = fFalse;
                }
            }

            //  do not increment iLineFrom
            //
            continue;
        }

        //  get node from page
        //
        psplitPath->csr.SetILine( iLineFrom );

        NDGet( pfucb, &psplitPath->csr );

        if ( iLineTo == psplit->ilineOper )
        {
            //  get key from node
            //  and data from parameter
            //
            Assert( splitoperInsert != psplit->splitoper );
            Assert( splitoperNone != psplit->splitoper );

            //  hotpoint is dealt with above
            Assert( !fPossibleHotpoint );

            psplit->rglineinfo[iLineTo].kdf.key     = pfucb->kdfCurr.key;
            psplit->rglineinfo[iLineTo].kdf.data    = pkdf->data;
            psplit->rglineinfo[iLineTo].kdf.fFlags  = pfucb->kdfCurr.fFlags;

            ULONG   cbMax       = CbBTIMaxSizeOfNode( pfucb, &psplitPath->csr );
            ULONG   cbSize      = CbNDNodeSizeTotal( psplit->rglineinfo[iLineTo].kdf );

            psplit->rglineinfo[iLineTo].cbSizeMax = max( cbSize, cbMax );

            //  there should be no uncommitted version for node
            //
            Assert( cbSize != cbMax || CbNDReservedSizeOfNode( pfucb, &psplitPath->csr ) == 0 );
        }
        else
        {
            psplit->rglineinfo[iLineTo].kdf         = pfucb->kdfCurr;
            psplit->rglineinfo[iLineTo].cbSizeMax   = CbBTIMaxSizeOfNode( pfucb, &psplitPath->csr );

            if ( fPossibleHotpoint )
            {
                if ( iLineTo < psplit->ilineOper - 2 )
                {
                    //  for nodes before the hotpoint area, keep track
                    //  of highest physical location
                    pvHighest = max( pvHighest, pfucb->kdfCurr.key.suffix.Pv() );
                }
                else if ( iLineTo > psplit->ilineOper )
                {
                    //  for nodes after insertion point, ensure
                    //  all are physically located before nodes in hotpoint area
                    fPossibleHotpoint = pfucb->kdfCurr.key.suffix.Pv() < pvHighest;
                }
            }

#ifdef DEBUG
            const ULONG     cbMax       = CbBTIMaxSizeOfNode( pfucb, &psplitPath->csr );
            const ULONG     cbReserved  = CbNDReservedSizeOfNode( pfucb, &psplitPath->csr );
            const ULONG     cbSize      = CbNDNodeSizeTotal( pfucb->kdfCurr );

            Assert( cbMax >= cbSize + cbReserved );
#endif
        }

        psplit->rglineinfo[iLineTo].cbSize =
                    CbNDNodeSizeTotal( psplit->rglineinfo[iLineTo].kdf );

        Assert( iLineFrom <= iLineTo );
        Assert( iLineFrom + 1 >= iLineTo );

        iLineFrom++;
    }

    if ( fPossibleHotpoint )
    {
        Assert( psplitPath->csr.Cpage().FLeafPage( ) );
        Assert( splitoperInsert == psplit->splitoper );
        Assert( psplit->ilineOper >= 2 );
        Assert( psplit->clines >= 3 );
        Assert( !psplit->fAppend );
        Assert( !psplit->fHotpoint );
        psplit->fHotpoint = fTrue;
    }

    //  determine if this meets the criteria for an append which overrides hotpoint
    //
    if ( psplit->splitoper == splitoperInsert &&
         psplit->ilineOper == psplit->clines - 1 &&
         psplit->psplitPath->csr.Cpage().PgnoNext() == pgnoNull )
    {
        Assert( psplitPath->csr.Cpage().FLeafPage() );
        Assert( !psplit->fAppend );
        psplit->fAppend = fTrue;
        psplit->fHotpoint = fFalse;
    }

    psplitPath->psplit = psplit;
    return JET_errSuccess;

HandleError:
    delete psplit;
    Assert( psplitPath->psplit == NULL );
    return err;
}


//  calculates
//      size of all nodes to the left of a node
//          using size of nodes already collected
//      maximum size of nodes possible due to rollback
//      size of common key with previous node
//      cbUncFree in the source and dest pages
//          using info collected
//
VOID BTIRecalcWeightsLE( SPLIT *psplit )
{
    INT     iline;
    Assert( psplit->clines > 0 );

    psplit->rglineinfo[0].cbSizeLE = psplit->rglineinfo[0].cbSize;
    psplit->rglineinfo[0].cbSizeMaxLE = psplit->rglineinfo[0].cbSizeMax;
    psplit->rglineinfo[0].cbCommonPrev = 0;
    psplit->rglineinfo[0].cbPrefix = 0;
    for ( iline = 1; iline < psplit->clines; iline++ )
    {
        Assert( CbNDNodeSizeTotal( psplit->rglineinfo[iline].kdf ) ==
                psplit->rglineinfo[iline].cbSize );

        psplit->rglineinfo[iline].cbSizeLE =
            psplit->rglineinfo[iline-1].cbSizeLE +
            psplit->rglineinfo[iline].cbSize;

        psplit->rglineinfo[iline].cbSizeMaxLE =
            psplit->rglineinfo[iline-1].cbSizeMaxLE +
            psplit->rglineinfo[iline].cbSizeMax;

        const INT   cbCommonKey =
                        CbCommonKey( psplit->rglineinfo[iline].kdf.key,
                                     psplit->rglineinfo[iline - 1].kdf.key );

        psplit->rglineinfo[iline].cbCommonPrev =
                        cbCommonKey > cbPrefixOverhead ?
                            cbCommonKey - cbPrefixOverhead :
                            0;

        psplit->rglineinfo[0].cbPrefix = 0;
    }

    Assert( iline == psplit->clines );
}


//  calculates cbUncommitted for split and new pages
//
VOID BTISplitCalcUncFree( SPLIT *psplit )
{
    Assert( psplit->ilineSplit > 0 ||
            splittypeVertical == psplit->splittype );
    psplit->cbUncFreeDest = SHORT( ( psplit->rglineinfo[psplit->clines - 1].cbSizeMaxLE -
                              psplit->rglineinfo[psplit->clines - 1].cbSizeLE ) -
                            ( splittypeVertical == psplit->splittype ?
                                0 :
                                ( psplit->rglineinfo[psplit->ilineSplit - 1].cbSizeMaxLE -
                                  psplit->rglineinfo[psplit->ilineSplit - 1].cbSizeLE ) ) );

    psplit->cbUncFreeSrc = SHORT( ( splittypeVertical == psplit->splittype ?
                                0 :
                                ( psplit->rglineinfo[psplit->ilineSplit - 1].cbSizeMaxLE -
                                  psplit->rglineinfo[psplit->ilineSplit - 1].cbSizeLE ) ) );

    Assert( splittypeAppend != psplit->splittype || 0 == psplit->cbUncFreeDest );
    Assert( psplit->cbUncFreeSrc <= psplit->psplitPath->csr.Cpage().CbUncommittedFree() );

    Assert( psplit->psplitPath->csr.Cpage().FLeafPage() ||
            0 == psplit->cbUncFreeSrc && 0 == psplit->cbUncFreeDest );
    return;
}

//  selects vertical split
//  if oper can not be performed with split,
//      selects vertical split with operNone
//
VOID BTISelectVerticalSplit( SPLIT *psplit, FUCB *pfucb )
{
    Assert( psplit->psplitPath->csr.Pgno() == PgnoRoot( pfucb ) );

    psplit->splittype   = splittypeVertical;
    psplit->ilineSplit  = 0;

    //  select prefix
    //
    BTISelectPrefixes( psplit, psplit->ilineSplit );

    //  check if oper fits in new page
    //
    if ( !FBTISplitCausesNoOverflow( psplit, psplit->ilineSplit ) )
    {
        //  split without performing operation
        //
        BTISelectSplitWithOperNone( psplit, pfucb );
        BTISelectPrefixes( psplit, psplit->ilineSplit );
        Assert( FBTISplitCausesNoOverflow( psplit, psplit->ilineSplit ) );
    }
    else
    {
        //  split and oper would both succeed
        //
        Assert( psplit->splitoper != splitoperNone );
    }

    BTISplitSetPrefixes( psplit );
    Assert( FBTISplitCausesNoOverflow( psplit, psplit->ilineSplit ) );

    //  set page flags for split and new pages
    //
    Assert( !psplit->fNewPageFlags );
    Assert( !psplit->fSplitPageFlags );

    psplit->fSplitPageFlags =
    psplit->fNewPageFlags = psplit->psplitPath->csr.Cpage().FFlags();

    psplit->fNewPageFlags &= ~ CPAGE::fPageRoot;
    if ( psplit->psplitPath->csr.Cpage().FLeafPage() && !FFUCBRepair( pfucb ) )
    {
        psplit->fSplitPageFlags = psplit->fSplitPageFlags | CPAGE::fPageParentOfLeaf;
    }
    else
    {
        Assert( !( PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() && FFUCBRepair( pfucb ) ) );
        psplit->fSplitPageFlags &= ~ CPAGE::fPageParentOfLeaf;
    }

    psplit->fSplitPageFlags &= ~ CPAGE::fPageLeaf;
    Assert( FFUCBRepair( pfucb ) || !( psplit->fSplitPageFlags & CPAGE::fPageRepair ) );
    psplit->fSplitPageFlags &= ~ CPAGE::fPageRepair;

    return;
}


//  selects append split
//  if appended node would not cause an overflow,
//      set prefix in page to inserted key
//
VOID BTISelectAppend( SPLIT *psplit, FUCB *pfucb )
{
    Assert( psplit->clines - 1 == psplit->ilineOper );
    Assert( splitoperInsert == psplit->splitoper );
    Assert( psplit->psplitPath->csr.Cpage().FLeafPage() );

    psplit->splittype           = splittypeAppend;
    psplit->ilineSplit          = psplit->ilineOper;

    LINEINFO    *plineinfoOper  = &psplit->rglineinfo[psplit->ilineOper];
    //  CbNDNodeSizeTotal includes line TAG overhead and must be compared against CbNDPageAvailMostNoInsert
    //
    if ( CbNDNodeSizeTotal( plineinfoOper->kdf ) + cbPrefixOverhead <= CbNDPageAvailMostNoInsert( g_rgfmp[ pfucb->ifmp ].CbPage() ) &&
         plineinfoOper->kdf.key.Cb() > cbPrefixOverhead )
    {
        plineinfoOper->cbPrefix = plineinfoOper->kdf.key.Cb();
        psplit->prefixinfoNew.ilinePrefix   = 0;
        psplit->prefixinfoNew.ilineSegBegin = 0;
        psplit->prefixinfoNew.cbSavings     = 0;
    }
    else
    {
        psplit->prefixinfoNew.Nullify();
        plineinfoOper->cbPrefix = 0;
    }

    psplit->prefixinfoSplit.Nullify();
}


//  ================================================================
LOCAL BOOL FBTISplitAppendLeaf( _In_ const SPLIT * const psplit )
//  ================================================================
//
//  Determines if we are splitting an internal page whose (eventual)
//  child leaf page is doing an append split (if that is the case
//  the internal page should do an append split as well)
//
//-
{
    BOOL fAppendLeaf = fFalse;

    if ( !psplit->psplitPath->csr.Cpage().FLeafPage( ) )
    {
        const SPLITPATH *psplitPath = psplit->psplitPath;

        for ( ; psplitPath->psplitPathChild != NULL; psplitPath = psplitPath->psplitPathChild )
        {
        }

        Assert( psplitPath->psplitPathChild == NULL );
        Assert( psplitPath->csr.Cpage().FLeafPage() );
        if ( NULL != psplitPath->psplit &&
             splittypeAppend == psplitPath->psplit->splittype )
        {
            fAppendLeaf = fTrue;
        }
    }
    return fAppendLeaf;
}


//  ================================================================
LOCAL BOOL FBTIPrependSplit( _In_ const SPLIT * const psplit )
//  ================================================================
//
//  This function looks for the case where we are doing a prepend
//  split on a leaf page. This is a split where the split is caused
//  by inserting a node at the very beginning of the b-tree.
//
//  Prepend splits are right splits with a specially selected iline so
//  they are treated as right splits.
//
//-
{
    BOOL fPrependLeaf = fFalse;

    const SPLITPATH * const psplitPath = psplit->psplitPath;

    if ( psplitPath->csr.Cpage().FLeafPage() &&
         pgnoNull == psplitPath->csr.Cpage().PgnoPrev() &&
         splitoperInsert == psplit->splitoper &&
         0 == psplit->ilineOper )
    {
        fPrependLeaf = fTrue;
    }
    return fPrependLeaf;
}


//  ================================================================
LOCAL VOID BTISelectHotPointSplit( _In_ SPLIT * const psplit, _In_ FUCB * pfucb )
//  ================================================================
//
//  Called for a hotpoint split
//
//-
{
    Assert( psplit->prefixinfoSplit.FNull() );
    Assert( psplit->fHotpoint );
    Assert( psplit->psplitPath->csr.Cpage().FLeafPage( ) );
    Assert( splitoperInsert == psplit->splitoper );
    Assert( psplit->ilineOper < psplit->clines );
    Assert( psplit->ilineOper >= 2 );
    Assert( psplit->clines >= 3 );

    INT ilineCandidate = psplit->ilineOper;
    if ( ilineCandidate < psplit->clines - 1 )
    {
        //  there are nodes after the hotpoint, so what
        //  we do is force a split consisting of just
        //  the nodes beyond the hotpoint, then try
        //  the operation again (the retry will either
        //  result in the new node being inserted as
        //  the last node on the page or it will
        //  result in a hotpoint split)

        BTISelectSplitWithOperNone( psplit, pfucb );
        psplit->fHotpoint = fFalse;
    }
    else
    {
        Assert( ilineCandidate == psplit->clines - 1 );
    }

    psplit->ilineSplit = ilineCandidate;

    //  find optimal prefix key for split page
    BTISelectPrefix(
            psplit->rglineinfo,
            ilineCandidate,
            &psplit->prefixinfoSplit );

    //  find optimal prefix key for new pages
    BTISelectPrefix(
            &psplit->rglineinfo[ilineCandidate],
            psplit->clines - ilineCandidate,
            &psplit->prefixinfoNew );

    Assert( FBTISplitCausesNoOverflow( psplit, ilineCandidate ) );
    BTISplitSetPrefixes( psplit );
}


//  ================================================================
LOCAL VOID BTISelectPrependLeafSplit(
    _In_ SPLIT * const psplit,
    _Out_ INT * const pilineCandidate,
    _Out_ PREFIXINFO * const pprefixinfoSplitCandidate,
    _Out_ PREFIXINFO * const pprefixinfoNewCandidate )
//  ================================================================
{
    Assert( psplit->prefixinfoSplit.FNull() );
    Assert( psplit->prefixinfoNew.FNull() );
    Assert( FBTIPrependSplit( psplit ) );

    const INT iline = 1;

    //  find optimal prefix key for split page

    BTISelectPrefix(
            psplit->rglineinfo,
            iline,
            &psplit->prefixinfoSplit );

    //  find optimal prefix key for new page

    BTISelectPrefix(
            &psplit->rglineinfo[iline],
            psplit->clines - iline,
            &psplit->prefixinfoNew );

    // as we are moving all existing nodes and no new nodes they must fit 
    // on the new page
    Assert( 0 == psplit->psplitPath->psplit->ilineOper );
    Assert( FBTISplitCausesNoOverflow( psplit, iline ) );

    *pilineCandidate            = iline;
    *pprefixinfoNewCandidate    = psplit->prefixinfoNew;
    *pprefixinfoSplitCandidate  = psplit->prefixinfoSplit;
}


//  ================================================================
LOCAL VOID BTISelectAppendLeafSplit(
    _In_ SPLIT * const psplit,
    _Out_ INT * const pilineCandidate,
    _Out_ PREFIXINFO * const pprefixinfoSplitCandidate,
    _Out_ PREFIXINFO * const pprefixinfoNewCandidate )
//  ================================================================
//
//  Called if we are an internal page whose (eventual) child leaf page
//  is doing an append split. We actually want to do an append split as
//  well (or at least as far to the right as possible)
//
//-
{
    Assert( psplit->prefixinfoSplit.FNull() );
    Assert( psplit->prefixinfoNew.FNull() );
    Assert( FBTISplitAppendLeaf( psplit ) );

    *pilineCandidate = 0;

    for ( INT iline = psplit->clines - 1; iline > 0; iline-- )
    {
        BTISelectPrefixes( psplit, iline );

        if ( FBTISplitCausesNoOverflow( psplit, iline ) )
        {
            *pilineCandidate            = iline;
            *pprefixinfoNewCandidate    = psplit->prefixinfoNew;
            *pprefixinfoSplitCandidate  = psplit->prefixinfoSplit;
            break;
        }
        else
        {
            //  shouldn't get overflow if only two nodes on page (should end up getting
            //  one node on split page and one node on new page), but it appears that
            //  we may have a bug where this is not the case, so put in a firewall
            //  to trap the occurrence and allow us to debug it the next time it is hit.
            DBEnforce( psplit->psplitPath->csr.Cpage().Ifmp(), psplit->clines > 2 );
        }
    }

    //  if we're doing an append split at the leaf level, then
    //  if splits at internal levels are needed, they must be
    //  a right split of either the NULL-key node or of the
    //  second-last (new) node, because the rest of the nodes
    //  should be guaranteed to fit on the page because they
    //  fit on the page before the new node came in
    //
    Assert( psplit->clines - 1 == *pilineCandidate
        || psplit->clines - 2 == *pilineCandidate );
}


//  ================================================================
LOCAL VOID BTIRecalculatePrefix(
    _In_ SPLIT * const psplit,
    _In_ const INT ilinePrev,
    _In_ const INT ilineNew )
//  ================================================================
//
//  Called by BTISelectEvenSplit when the splitpoint has changed (by
//  1 line). Recalculate the prefix compression information, doing
//  as little work as needed
//
//-
{
    Assert( absdiff( ilinePrev, ilineNew ) == 1 );

    if( ilinePrev < ilineNew )
    {
        //  the split (left) page grew, the new (right) page shrank
        //      - a line was added to the end of the split (left) page
        //      - a line was removed from the start of the new (right) page

        //  find optimal prefix key for split page

        if ( psplit->rglineinfo[ilineNew-1].cbCommonPrev == 0 )
        {
            //  added line has nothing in common with the other nodes on the split page, so the prefix compression
            //  will not be affected
        }
        else
        {
            BTISelectPrefix(
                    psplit->rglineinfo,
                    ilineNew,
                    &psplit->prefixinfoSplit );
        }

        //  the new (right) page became smaller, see if we need to recalculate its prefix information
        //  ilineSegBegin in the prefixinfoNew is relative to the new page, so iline 0 is the node we just
        //  removed     

        if ( 0 == psplit->prefixinfoNew.ilineSegBegin )
        {
            //  the removed line contributed to prefix. recalculate the prefix for the new page

            BTISelectPrefix( &psplit->rglineinfo[ilineNew], psplit->clines - ilineNew, &psplit->prefixinfoNew );
        }
        else if ( !psplit->prefixinfoNew.FNull() )
        {
            //  adjust the ilines, as the new page now starts one iline later

            psplit->prefixinfoNew.ilinePrefix--;
            psplit->prefixinfoNew.ilineSegBegin--;
            psplit->prefixinfoNew.ilineSegEnd--;
        }

    }
    else
    {
        Assert( ilinePrev > ilineNew );

        //  the split (left) page shrank, the new (right) page grew
        //      - a line was removed from the end of the split (left) page
        //      - a line was added to the start of the new (right) page

        //  the split (left) page became smaller, see if we need to recalculate its prefix information
        //  the split looks like [0->iline-1][iline->clines-1], so we recalculate the prefix for the
        //  left page if the split point is now equal to ilineSegEnd

        if ( psplit->prefixinfoSplit.ilineSegEnd == ilineNew )
        {
            //  the removed line contributed to prefix. recalculate the prefix for the new page

            BTISelectPrefix( psplit->rglineinfo, ilineNew, &psplit->prefixinfoSplit );
        }

        //  find optimal prefix key for new page

        if ( psplit->rglineinfo[ilineNew+1].cbCommonPrev == 0 )
        {
            //  added line has nothing in common with the other nodes on the split page, so the prefix compression
            //  will not be affected

            //  the ilines in the prefixinfo are relative to the ilines in the new page. as we just added a line
            //  to the start of the page, we have to increment the ilines so that they point to the same places
            //  in the new page

            if ( !psplit->prefixinfoNew.FNull() )
            {
                psplit->prefixinfoNew.ilinePrefix++;
                psplit->prefixinfoNew.ilineSegBegin++;
                psplit->prefixinfoNew.ilineSegEnd++;
            }
        }
        else
        {
            BTISelectPrefix(
                &psplit->rglineinfo[ilineNew],
                psplit->clines - ilineNew,
                &psplit->prefixinfoNew );
        }
    }
}


//  ================================================================
LOCAL VOID BTISelectEvenSplitWithoutCheckingOverflows(
    _In_ const SPLIT * const psplit,
    _Out_ INT * const pilineCandidate )
//  ================================================================
//
//  Find the split point that has the "best" distribution of data
//  between the split and new pages. The best distribution is the one
//  where the data is distributed as evenly as possible
//
//-
{
    *pilineCandidate = 0;

    ULONG       cbSizeCandidateLE   = 0;

    //  in the best case, a split will leave 50% of the data on the split page and
    //  move 50% of the data to the new page. calculate the number of bytes that will
    //  be on each page in that case. this is used to determine if a candidate split point
    //  is better than the current split point

    const ULONG     cbSizeTotal     = psplit->rglineinfo[psplit->clines - 1].cbSizeLE;
    const ULONG     cbSizeBestSplit = cbSizeTotal / 2;

    //  pick the initial split point. we will find a split point that has the best distribution
    //  of data between the two pages (split and new). start by guessing the split point will
    //  be halfway through the page

    INT iline               = psplit->clines / 2;
    Assert( iline >= 1 );

    //  our initial split point will result in the split (left) page either being smaller
    //  in size than the new (right) page or GEQ to the new (right) page. we will move the
    //  split point so that the smaller page becomes larger. we only have to do this until
    //  the page which was previously the small page becomes larger.
    //
    //  i.e. if fFirstSplitPageWasSmaller is true, we loop below until fSplitPageIsSmaller
    //  is no longer true
    //
    //  if cbSizeSplitPage == cbSizeNewPage we consider it a "perfect" split and leave the
    //  loop immediately

    const ULONG cbSizeFirstSplitPage        = psplit->rglineinfo[iline - 1].cbSizeLE;
    const ULONG cbSizeFirstNewPage          = cbSizeTotal - cbSizeFirstSplitPage;
    const BOOL  fFirstSplitPageWasSmaller   = cbSizeFirstSplitPage < cbSizeFirstNewPage;

    //  this loop simply interates through the lines. we could use bsearch or interpolation
    //  but we will (optimistically?) assume that the records are about the same size (or
    //  the number of records is small) so that this loop won't take many steps

    while( true )
    {
        //  see if this split is better than our current candidate split, a split
        //  is "better" if the data is more equally distributed between the two
        //  (split/new or left/right) pages. again, this determination does NOT take
        //  prefix compression into account
        //          
        //  to start, determine how much data will be on the split (left) page and how
        //  much will be on the new (right) page. this calculation does not
        //  take prefix compression into account

        const ULONG cbSizeSplitPage     = psplit->rglineinfo[iline - 1].cbSizeLE;
        const ULONG cbSizeNewPage       = cbSizeTotal - cbSizeSplitPage;
        const BOOL  fSplitPageIsSmaller = cbSizeSplitPage < cbSizeNewPage;

        //  the first time through this loop (or anytime a candidate point hasn't been
        //  determined) cbSizeCandidateLE will be 0, so any split will be better

        if ( absdiff( cbSizeCandidateLE, cbSizeBestSplit ) >
             absdiff( cbSizeSplitPage, cbSizeBestSplit ) )
        {
            *pilineCandidate                = iline;
            cbSizeCandidateLE               = psplit->rglineinfo[iline - 1].cbSizeLE;
        }
        else if( (  absdiff( cbSizeCandidateLE, cbSizeBestSplit ) ==
                    absdiff( cbSizeSplitPage, cbSizeBestSplit ) )
                && iline > *pilineCandidate )
        {
            //  given two equally good split points, we want to choose the split point
            //  that moves the fewest nodes

            *pilineCandidate                = iline;
            cbSizeCandidateLE               = psplit->rglineinfo[iline - 1].cbSizeLE;

            //  if we have seen two split points with equal data distributions, we
            //  must have made the smaller page larger and vice versa

            Assert( fFirstSplitPageWasSmaller != fSplitPageIsSmaller );
        }
        else
        {
            //  if this split point is not better than our old split point, we must have
            //  gone past the point where the smaller page becomes larger. otherwise 
            //  moving data from the larger page to the smaller should have improved
            //  the split point. note that we are not guaranteed to end up in this clause
            //  if the smaller page has become larger -- doing so could produce a better
            //  split point

            Assert( fFirstSplitPageWasSmaller != fSplitPageIsSmaller );
        }

        if( fFirstSplitPageWasSmaller != fSplitPageIsSmaller )
        {
            break;
        }
        else if( fSplitPageIsSmaller )
        {
            //  make the split page larger

            ++iline;

            //  see if we have reached the end of the page

            if( psplit->clines == iline )
            {
                //  the optimum split point must be the previous line, otherwise
                //  we should have broken out of the loop earlier, when the smaller
                //  page became larger

                Assert( 0 != *pilineCandidate );
                Assert( psplit->clines - 1 == *pilineCandidate );
                break;
            }
        }
        else
        {
            Assert( !fSplitPageIsSmaller );

            //  make the split page smaller

            --iline;

            //  see if we have reached the end of the page

            if( 0 == iline )
            {
                //  the optimum split point must be the previous line, otherwise
                //  we should have broken out of the loop earlier, when the smaller
                //  page became larger

                Assert( 1 == *pilineCandidate );
                break;
            }
        }
    }

    Assert( 0 != *pilineCandidate );
}

#ifdef DEBUG
//  ================================================================
LOCAL VOID BTISelectEvenSplitDebug(
    _In_ SPLIT * const psplit,
    _Out_ INT * const pilineCandidate,
    _Out_ PREFIXINFO * const pprefixinfoSplitCandidate,
    _Out_ PREFIXINFO * const pprefixinfoNewCandidate )
//  ================================================================
//
//  A copy of the split code as it was prior to 06/2003, used to make
//  sure the new code is returning the correct answer (or at least the
//  same answer as the old code)
//
//-
{
        INT         iline;
        const PREFIXINFO    prefixinfoSplitSaved    = psplit->prefixinfoSplit;
        const PREFIXINFO    prefixinfoNewSaved      = psplit->prefixinfoNew;
        ULONG       cbSizeCandidateLE;
        ULONG       cbSizeTotal;

        psplit->prefixinfoNew.Nullify();
        psplit->prefixinfoSplit.Nullify();

        Assert( !psplit->fHotpoint );

        *pilineCandidate    = 0;
        cbSizeCandidateLE   = 0;
        cbSizeTotal         = psplit->rglineinfo[psplit->clines - 1].cbSizeLE;

        DBEnforce( psplit->psplitPath->csr.Cpage().Ifmp(), psplit->clines > 1 );

        //  starting from last node
        //  find candidate split points
        //
        for ( iline = psplit->clines - 1; iline > 0; iline-- )
        {
            //  UNDONE: optimize prefix selection using a prefix upgrade function
            //
            //  find optimal prefix key for both pages
            //
            BTISelectPrefixes( psplit, iline );

            if ( FBTISplitCausesNoOverflow( psplit, iline ) )
            {
                //  if this candidate is closer to cbSizeTotal / 2
                //  than last one, replace candidate
                //
                if ( absdiff( cbSizeCandidateLE, cbSizeTotal / 2 ) >
                     absdiff( psplit->rglineinfo[iline - 1].cbSizeLE, cbSizeTotal / 2 ) )
                {
                    *pilineCandidate = iline;
                    *pprefixinfoSplitCandidate  = psplit->prefixinfoSplit;
                    *pprefixinfoNewCandidate    = psplit->prefixinfoNew;
                    cbSizeCandidateLE = psplit->rglineinfo[iline - 1].cbSizeLE;
                }
            }
            else
            {
                //  shouldn't get overflow if only two nodes on page (should end up getting
                //  one node on split page and one node on new page), but it appears that
                //  we may have a bug where this is not the case, so put in a firewall
                //  to trap the occurrence and allow us to debug it the next time it is hit.
                DBEnforce( psplit->psplitPath->csr.Cpage().Ifmp(), psplit->clines > 2 );
            }
        }

        psplit->prefixinfoSplit =   prefixinfoSplitSaved;
        psplit->prefixinfoNew   =   prefixinfoNewSaved;

        return;
}
#endif // DEBUG

//  ================================================================
LOCAL VOID BTISelectEvenSplit(
    _In_ SPLIT * const psplit,
    _Out_ INT * const pilineCandidate,
    _Out_ PREFIXINFO * const pprefixinfoSplitCandidate,
    _Out_ PREFIXINFO * const pprefixinfoNewCandidate )
//  ================================================================
//
//  we want to find a split point that splits the data between the two new pages
//  as equally as possible. to do this we will pick a split point that distributes
//  the data as equally as possible and then move the split point to take overflow
//  into account
//
//-
{
    Assert( psplit->prefixinfoSplit.FNull() );

    *pilineCandidate = 0;

    INT iline;

    //  find the split point with the best distribution of data

    BTISelectEvenSplitWithoutCheckingOverflows( psplit, &iline );

    //  find optimal prefix key for split page

    BTISelectPrefix(
            psplit->rglineinfo,
            iline,
            &psplit->prefixinfoSplit );

    //  find optimal prefix key for new page

    BTISelectPrefix(
            &psplit->rglineinfo[iline],
            psplit->clines - iline,
            &psplit->prefixinfoNew );

    //  we have picked the best split point, but the split may not work as one or both pages may overflow
    //  if both pages overflow, there is nothing we can do. if only one page is overflowing we can move
    //  data from the overflowing page to the other page. we continue to do that until we find a split which
    //  works, or the non-overflowing page starts to overflow

    //  as we go through this loop we will make the split page either smaller or larger, terminate the loop
    //  if we attempt to reverse this decision (e.g. we were making the split page smaller and we now want
    //  to make it larger)

    enum { SPLIT_PAGE_SMALLER, SPLIT_PAGE_LARGER, UNKNOWN } splitpointChange, lastsplitpointChange;

    lastsplitpointChange = UNKNOWN;

    while( true )
    {
        Assert( iline > 0 );
        Assert( iline < psplit->clines );
        Assert( 0 == *pilineCandidate );

        //  when we improve our splitpoint, we will either make the split page smaller or larger
        //  track which we want to do in this variable

        splitpointChange = UNKNOWN;

        //  there is no guarantee that this split is possible -- the nodes for the
        //  new or old page may not fit on that page. if there is an overflow, ignore
        //  this as a split point. 

        BOOL fSplitPageOverflow;
        BOOL fNewPageOverflow;

        const BOOL fOverflow = !FBTISplitCausesNoOverflow( psplit, iline, &fSplitPageOverflow, &fNewPageOverflow );
        Assert( fOverflow == ( fSplitPageOverflow || fNewPageOverflow ) );

        //  UNDONE: we think you can't overflow both pages, but handle it
        //  anyway just in case (specifically, inserting a new node may
        //  cause the split page or new page to overflow, but shouldn't
        //  be able to cause both pages to overflow, because all nodes
        //  except the new node used to fit on one page)
        //
        Assert( !( fSplitPageOverflow && fNewPageOverflow ) );

        if( fOverflow )
        {
            if( fSplitPageOverflow && fNewPageOverflow )
            {
                //  if both pages are overflowing, we can leave this loop now as no
                //  split will be possible

                Assert( 0 == *pilineCandidate );
                break;
            }

            //  shouldn't get overflow if only two nodes on page (should end up getting
            //  one node on split page and one node on new page), but it appears that
            //  we may have a bug where this is not the case, so put in a firewall
            //  to trap the occurrence and allow us to debug it the next time it is hit.

            DBEnforce( psplit->psplitPath->csr.Cpage().Ifmp(), psplit->clines > 2 );

            //  this is not a valid split point, as one of the page overflows. we will
            //  improve our split point by moving one node from the overflowing page
            //  to the non overflowing page

            Assert( fSplitPageOverflow ^ fNewPageOverflow );
            splitpointChange = fSplitPageOverflow ? SPLIT_PAGE_SMALLER : SPLIT_PAGE_LARGER;
        }
        else
        {
            Assert( !fOverflow );
            Assert( !fSplitPageOverflow );
            Assert( !fNewPageOverflow );

            //  we have been moving away from the best (in terms of data distribution) split point
            //  the first split which works (doesn't overflow) will be the best

            Assert( 0 == *pilineCandidate );
            *pilineCandidate            = iline;
            *pprefixinfoSplitCandidate  = psplit->prefixinfoSplit;
            *pprefixinfoNewCandidate    = psplit->prefixinfoNew;
            break;
        }

        //  we have decided how to improve our split point. if we are about to change directions we are finished
        //  (we would re-evaluate the same nodes and end up in an infinite loop)

        Assert( UNKNOWN != splitpointChange );
        if( ( SPLIT_PAGE_LARGER == lastsplitpointChange && SPLIT_PAGE_SMALLER == splitpointChange )
            || ( SPLIT_PAGE_SMALLER == lastsplitpointChange && SPLIT_PAGE_LARGER == splitpointChange ) )
        {
            //  if we found a working split point we would have left the loop

            Assert( 0 == *pilineCandidate );
            break;
        }
        lastsplitpointChange = splitpointChange;
        Assert( UNKNOWN != lastsplitpointChange );

        //  move to the new split point

        if( SPLIT_PAGE_LARGER == splitpointChange )
        {
            //  we want to make the split page larger, increase the split point to leave more nodes
            //  on the split page

            iline++;

            //  see if we have reached the end of the page

            if( psplit->clines == iline )
            {
                //  if we found a working split point we would have left the loop

                Assert( 0 == *pilineCandidate );
                break;
            }

            //  recalculate the prefixes

            BTIRecalculatePrefix( psplit, iline-1, iline );
        }
        else if( SPLIT_PAGE_SMALLER == splitpointChange )
        {
            //  we want to make the split page smaller, descrease the split point to move more nodes
            //  onto the new page

            iline--;

            //  see if we have reached the beginning of the page

            if( 0 == iline )
            {
                //  if we found a working split point we would have left the loop

                Assert( 0 == *pilineCandidate );
                break;
            }

            //  recalculate the prefixes

            BTIRecalculatePrefix( psplit, iline+1, iline );
        }
        else
        {
            AssertRTL( fFalse );
        }
    }
}

//  selects split point such that
//      node weights are almost equal
//      and split nodes fit in both pages [with optimal prefix key]
//  if no such node exists,
//      select split with operNone
//
VOID BTISelectRightSplit( SPLIT *psplit, FUCB *pfucb )
{
    INT         ilineCandidate;
    PREFIXINFO  prefixinfoSplitCandidate;
    PREFIXINFO  prefixinfoNewCandidate;

    psplit->splittype = splittypeRight;

    //  if this is a hotpoint split, use the hotpoint split selection

    if ( psplit->fHotpoint )
    {
        BTISelectHotPointSplit( psplit, pfucb );
        return;
    }

    //  check if internal page and split at leaf level is append or prepend
    //
    //  append splits are a different type of split (no data is moved) while
    //  prepend splits are actually a special case of right splits (all existing
    //  nodes are moved to the new page and the newly inserted node is placed
    //  on the old page).

    const BOOL fAppendLeaf = FBTISplitAppendLeaf( psplit );
    const BOOL fPrependSplit = FBTIPrependSplit( psplit );

Start:
    ilineCandidate      = 0;

    //  when the following condition fails, we end up spinning in this loop forever;
    DBEnforce( psplit->psplitPath->csr.Cpage().Ifmp(), psplit->clines > 1 );

    if ( fAppendLeaf )
    {
        //  the leaf page is appending, do an append split on the parent

        BTISelectAppendLeafSplit( psplit, &ilineCandidate, &prefixinfoSplitCandidate, &prefixinfoNewCandidate );
    }
    else if ( fPrependSplit )
    {
        BTISelectPrependLeafSplit( psplit, &ilineCandidate, &prefixinfoSplitCandidate, &prefixinfoNewCandidate );
    }
    else
    {
        //  split the data as evenly as possible

        BTISelectEvenSplit( psplit, &ilineCandidate, &prefixinfoSplitCandidate, &prefixinfoNewCandidate );

#ifdef DEBUG
        if( FExpensiveDebugCodeEnabled( Debug_BT_Split ) )
        {
            INT         ilineCandidateT;
            PREFIXINFO  prefixinfoSplitCandidateT;
            PREFIXINFO  prefixinfoNewCandidateT;
            prefixinfoSplitCandidateT.Nullify();
            prefixinfoNewCandidateT.Nullify();
            BTISelectEvenSplitDebug(
                psplit,
                &ilineCandidateT,
                &prefixinfoSplitCandidateT,
                &prefixinfoNewCandidateT );

            Assert( ilineCandidateT == ilineCandidate );
            if( 0 != ilineCandidateT )
            {
                Assert( prefixinfoSplitCandidateT.cbSavings == prefixinfoSplitCandidate.cbSavings );
                Assert( prefixinfoNewCandidateT.cbSavings   == prefixinfoNewCandidate.cbSavings );
            }
        }
#endif
    }

    if ( ilineCandidate == 0 )
    {
        //  no candidate line fits the bill
        //  need to split without performing operation
        //
        Assert( psplit->psplitPath->csr.Cpage().FLeafPage() );
        Assert( psplit->splitoper != splitoperNone );
        BTISelectSplitWithOperNone( psplit, pfucb );
        goto Start;
    }

    Assert( ilineCandidate != 0 );

    psplit->ilineSplit      = ilineCandidate;
    psplit->prefixinfoNew   = prefixinfoNewCandidate;
    psplit->prefixinfoSplit = prefixinfoSplitCandidate;

    Assert( FBTISplitCausesNoOverflow( psplit, ilineCandidate ) );
    BTISplitSetPrefixes( psplit );
    return;
}


//  sets up psplit, so split is performed with no
//  user-requested operation
//
VOID BTISelectSplitWithOperNone( SPLIT *psplit, FUCB *pfucb )
{
    if ( splitoperInsert == psplit->splitoper )
    {
        //  move up all lines beyond ilineOper
        //
        Assert( psplit->clines >= 2 );
        if ( psplit->clines > 0 )
        {
            for ( INT iLine = psplit->ilineOper; iLine < psplit->clines - 1 ; iLine++ )
            {
#pragma prefast ( suppress: __WARNING_UNRELATED_LOOP_TERMINATION_NO_SIZEEXPR, "The problem is that the size of SPLIT::rglineinfo is equal to SPLIT::clines + 1 at do time and SPLIT::clines + 0 at redo time.  This extra slot is used for scratch space by an algorithm in BTISelectPrefix and is present based on the context in which the engine is being used.  Cleaning this up is easy but would cause too much churn to an escrow binary for this late in the ship cycle." )
                psplit->rglineinfo[iLine] = psplit->rglineinfo[iLine + 1];
            }

            psplit->clines--;
        }
    }
    else
    {
        Assert( psplit->psplitPath->csr.Cpage().FLeafPage( ) );

        //  adjust only rglineinfo[ilineOper]
        //
        //  get kdfCurr for ilineOper
        //
        psplit->psplitPath->csr.SetILine( psplit->ilineOper );
        NDGet( pfucb, &psplit->psplitPath->csr );

        psplit->rglineinfo[psplit->ilineOper].kdf = pfucb->kdfCurr;
        psplit->rglineinfo[psplit->ilineOper].cbSize =
            CbNDNodeSizeTotal( pfucb->kdfCurr );

        ULONG   cbMax   = CbBTIMaxSizeOfNode( pfucb, &psplit->psplitPath->csr );
        Assert( cbMax >= psplit->rglineinfo[psplit->ilineOper].cbSize );
        psplit->rglineinfo[psplit->ilineOper].cbSizeMax = cbMax;
    }

    //  UNDONE: optimize recalc for only nodes >= ilineOper
    //          optimize recalc for cbCommonPrev separately
    //
    psplit->ilineOper = 0;
    BTIRecalcWeightsLE( psplit );
    psplit->splitoper = splitoperNone;

    psplit->prefixinfoNew.Nullify();
    psplit->prefixinfoSplit.Nullify();
}


//  checks if splitting psplit->rglineinfo[] at cLineSplit
//      -- i.e., moving nodes cLineSplit and above to new page --
//      would cause an overflow in either page
//
BOOL FBTISplitCausesNoOverflow( SPLIT *psplit, INT ilineSplit, BOOL * pfSplitPageOverflow, BOOL * pfNewPageOverflow )
{
#ifdef DEBUG
    if( FExpensiveDebugCodeEnabled( Debug_BT_Split ) )
    {
        //  check that prefixes have been calculated correctly
        //
        PREFIXINFO  prefixinfo;

        if ( 0 == ilineSplit )
        {
            //  root page in vertical split has no prefix
            //
            Assert( splittypeVertical == psplit->splittype );
            prefixinfo.Nullify();
        }
        else
        {
            //  select prefix for split page
            //
            BTISelectPrefix( psplit->rglineinfo,
                             ilineSplit,
                             &prefixinfo );
        }

    /// Assert( prefixinfo.ilinePrefix  == psplit->prefixinfoSplit.ilinePrefix );
        Assert( prefixinfo.cbSavings    == psplit->prefixinfoSplit.cbSavings );
        Assert( prefixinfo.ilineSegBegin
                    == psplit->prefixinfoSplit.ilineSegBegin );
        Assert( prefixinfo.ilineSegEnd
                    == psplit->prefixinfoSplit.ilineSegEnd );

        //  select prefix for new page
        //
        Assert( psplit->clines > ilineSplit );
        BTISelectPrefix( &psplit->rglineinfo[ilineSplit],
                         psplit->clines - ilineSplit,
                         &prefixinfo );

    /// Assert( prefixinfo.ilinePrefix  == psplit->prefixinfoNew.ilinePrefix );
        Assert( prefixinfo.cbSavings    == psplit->prefixinfoNew.cbSavings );
        Assert( prefixinfo.ilineSegBegin
                    == psplit->prefixinfoNew.ilineSegBegin );
        Assert( prefixinfo.ilineSegEnd
                    == psplit->prefixinfoNew.ilineSegEnd );
    }
#endif

    //  ilineSplit == 0 <=> vertical split
    //  where every node is moved to new page
    //
    Assert( splittypeVertical != psplit->splittype || ilineSplit == 0 );
    Assert( splittypeVertical == psplit->splittype || ilineSplit > 0 );
    Assert( ilineSplit < psplit->clines );

    //  all nodes to left of ilineSplit should fit in page
    //  and all nodes >= ilineSplit should fit in page
    //
    const INT   cbSplitPage =
                    ilineSplit == 0 ?
                        0 :
                        ( psplit->rglineinfo[ilineSplit - 1].cbSizeMaxLE -
                          psplit->prefixinfoSplit.cbSavings );

    Assert( cbSplitPage >= 0 );
    const BOOL  fSplitPageFits = cbSplitPage <= (INT)CbNDPageAvailMostNoInsert( psplit->psplitPath->csr.Cpage().CbPage() );

    const INT   cbNewPage =
                    psplit->rglineinfo[psplit->clines - 1].cbSizeMaxLE -
                    ( ilineSplit == 0 ?
                        0 :
                        psplit->rglineinfo[ilineSplit - 1].cbSizeMaxLE ) -
                    psplit->prefixinfoNew.cbSavings;

    const BOOL  fNewPageFits = ( ilineSplit == psplit->clines ||
                                 cbNewPage <= (INT)CbNDPageAvailMostNoInsert( psplit->psplitPath->csr.Cpage().CbPage() ) );

    if( pfSplitPageOverflow )
    {
        *pfSplitPageOverflow = !fSplitPageFits;
    }

    if( pfNewPageOverflow )
    {
        *pfNewPageOverflow = !fNewPageFits;
    }

    return fSplitPageFits && fNewPageFits;
}


//  allocates space for key in data and copies entire key into data
//
ERR ErrBTISplitAllocAndCopyPrefix( const KEY &key, DATA *pdata )
{
    Assert( pdata->Pv() == NULL );
    Assert( !key.FNull() );

    pdata->SetPv( RESBOOKMARK.PvRESAlloc() );
    if ( pdata->Pv() == NULL )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    pdata->SetCb( key.Cb() );
    key.CopyIntoBuffer( pdata->Pv(), pdata->Cb() );

    return JET_errSuccess;
}


//  allocate space for new and old prefixes for split page
//  copy prefixes
//
ERR ErrBTISplitAllocAndCopyPrefixes( FUCB *pfucb, SPLIT *psplit )
{
    ERR     err = JET_errSuccess;

    Assert( psplit->prefixSplitOld.FNull() );
    Assert( psplit->prefixSplitNew.FNull() );

    NDGetPrefix( pfucb, &psplit->psplitPath->csr );
    if ( !pfucb->kdfCurr.key.prefix.FNull() )
    {
        CallR( ErrBTISplitAllocAndCopyPrefix( pfucb->kdfCurr.key,
                                              &psplit->prefixSplitOld ) );
    }

    if ( psplit->prefixinfoSplit.ilinePrefix != ilineInvalid )
    {
        const INT   ilinePrefix = psplit->prefixinfoSplit.ilinePrefix;

        Assert( psplit->splittype != splittypeAppend );
        CallR( ErrBTISplitAllocAndCopyPrefix(
                        psplit->rglineinfo[ilinePrefix].kdf.key,
                        &psplit->prefixSplitNew ) );
    }

    CallS( err );
    return err;
}


//  leave psplitPath->psplitPathParent->csr at insert point in parent
//
ERR ErrBTISeekSeparatorKey( SPLIT *psplit, FUCB *pfucb )
{
    ERR         err = JET_errSuccess;
    CSR         *pcsr = &psplit->psplitPath->psplitPathParent->csr;
    BOOKMARK    bm;


    Assert( !psplit->kdfParent.key.FNull() );
    Assert( sizeof( PGNO ) == psplit->kdfParent.data.Cb() );
    Assert( splittypeVertical != psplit->splittype );
    Assert( pcsr->Cpage().FInvisibleSons() );

    //  seeking in internal page should have NULL data
    //
    bm.key  = psplit->kdfParent.key;
    bm.data.Nullify();
    CallR( ErrNDSeek( pfucb, pcsr, bm ) );

    Assert( err == wrnNDFoundGreater );
    Assert( err != JET_errSuccess );
    if ( err == wrnNDFoundLess )
    {
        //  inserted node should never fall after last node in page
        //
        Assert( fFalse );
        Assert( pcsr->Cpage().Clines() - 1 == pcsr->ILine() );
        pcsr->IncrementILine();
    }
    err = JET_errSuccess;

    return err;
}


//  allocates and computes separator key between given lines
//
ERR ErrBTIComputeSeparatorKey( FUCB                 *pfucb,
                               const KEYDATAFLAGS   &kdfPrev,
                               const KEYDATAFLAGS   &kdfSplit,
                               KEY                  *pkey )
{
    INT     cbDataCommon    = 0;
    INT     cbKeyCommon     = CbCommonKey( kdfSplit.key, kdfPrev.key );
    BOOL    fKeysEqual      = fFalse;

    if ( cbKeyCommon == kdfSplit.key.Cb() &&
         cbKeyCommon == kdfPrev.key.Cb() )
    {
        //  split key is the same as the previous one
        //
        Assert( !FFUCBUnique( pfucb ) );
        Assert( FKeysEqual( kdfSplit.key, kdfPrev.key ) );
        Assert( CmpData( kdfSplit.data, kdfPrev.data ) > 0 );

        fKeysEqual = fTrue;
        cbDataCommon = CbCommonData( kdfSplit.data, kdfPrev.data );
    }
    else
    {
        Assert( CmpKey( kdfSplit.key, kdfPrev.key ) > 0 );
        Assert( cbKeyCommon < kdfSplit.key.Cb() );
    }

    //  allocate memory for separator key
    //
    Assert( pkey->FNull() );
    pkey->suffix.SetPv( RESBOOKMARK.PvRESAlloc() );
    if ( pkey->suffix.Pv() == NULL )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    pkey->suffix.SetCb( cbKeyCommon + cbDataCommon + 1 );

    //  copy separator key and data into alocated memory
    //
    Assert( cbKeyCommon <= pkey->Cb() );
    Assert( pkey->suffix.Pv() != NULL );
    kdfSplit.key.CopyIntoBuffer( pkey->suffix.Pv(),
                                 cbKeyCommon );

    if ( !fKeysEqual )
    {
        //  copy difference byte from split key
        //
        Assert( 0 == cbDataCommon );
        Assert( kdfSplit.key.Cb() > cbKeyCommon );

        if ( kdfSplit.key.prefix.Cb() > cbKeyCommon )
        {
            //  byte of difference is in prefix
            //
            ( (BYTE *)pkey->suffix.Pv() )[cbKeyCommon] =
                    ( (BYTE *) kdfSplit.key.prefix.Pv() )[cbKeyCommon];
        }
        else
        {
            //  get byte of difference from suffix
            //
            ( (BYTE *)pkey->suffix.Pv() )[cbKeyCommon] =
                    ( (BYTE *) kdfSplit.key.suffix.Pv() )[cbKeyCommon -
                                            kdfSplit.key.prefix.Cb() ];
        }
    }
    else
    {
        //  copy common data
        //  then copy difference byte from split data
        //
        UtilMemCpy( (BYTE *)pkey->suffix.Pv() + cbKeyCommon,
                kdfSplit.data.Pv(),
                cbDataCommon );

        Assert( kdfSplit.data.Cb() > cbDataCommon );

        ( (BYTE *)pkey->suffix.Pv() )[cbKeyCommon + cbDataCommon] =
                ( (BYTE *)kdfSplit.data.Pv() )[cbDataCommon];
    }

    return JET_errSuccess;
}


//  for leaf level,
//  computes shortest separator
//  between the keys of ilineSplit and ilineSplit - 1
//  allocates memory for node to be inserted
//      and the pointer to it in psplit->kdfParent
//  for internal pages, return last kdf in page
//
ERR ErrBTISplitComputeSeparatorKey( SPLIT *psplit, FUCB *pfucb )
{
    ERR             err;
    KEYDATAFLAGS    *pkdfSplit = &psplit->rglineinfo[psplit->ilineSplit].kdf;
    KEYDATAFLAGS    *pkdfPrev = &psplit->rglineinfo[psplit->ilineSplit - 1].kdf;

    Assert( psplit->kdfParent.key.FNull() );
    Assert( psplit->kdfParent.data.FNull() );
    Assert( psplit->psplitPath->psplitPathParent != NULL );

    //  data of node inserted at parent should point to split page
    //
    psplit->kdfParent.data.SetCb( sizeof( PGNO ) );
    psplit->kdfParent.data.SetPv( &psplit->pgnoSplit );

    if ( psplit->psplitPath->csr.Cpage().FInvisibleSons( ) || FFUCBRepair( pfucb ) )
    {
        //  not leaf page
        //  separator key should be key of ilineSplit - 1
        //
        Assert( !psplit->psplitPath->csr.Cpage().FLeafPage() && pkdfSplit->key.FNull()
                || CmpKey( pkdfPrev->key, pkdfSplit->key )  < 0
                || g_fRepair );

        psplit->kdfParent.key = pkdfPrev->key;
        return JET_errSuccess;
    }

    Assert( psplit->psplitPath->csr.Cpage().FLeafPage() );
    Assert( !psplit->fAllocParent );

    CallR( ErrBTIComputeSeparatorKey( pfucb, *pkdfPrev, *pkdfSplit, &psplit->kdfParent.key ) );
    psplit->fAllocParent = fTrue;

    return err;
}


#ifdef DEBUG
//  selects prefix for clines in rglineinfo
//  places result in *pprefixinfo
//
LOCAL VOID BTISelectPrefixCheck( const LINEINFO     *rglineinfo,
                                 INT                clines,
                                 PREFIXINFO         *pprefixinfo )
{
    pprefixinfo->Nullify();

    Assert( clines > 0 );
    if ( 1 == clines )
    {
        return;
    }

    //  set cbCommonPrev for first line to zero
    //
    const ULONG cbCommonPrevSav = rglineinfo[0].cbCommonPrev;
    ((LINEINFO *)rglineinfo)[0].cbCommonPrev = 0;

    INT         iline;

    //  UNDONE: optimize loop to use info from previous iteration
    //
    //  calculate prefixinfo for line
    //  if better than previous candidate
    //      choose as new candidate
    //
    for ( iline = 0; iline < clines; iline++ )
    {
        if ( iline != 0 )
        {
            ULONG cbCommonKey = CbCommonKey( rglineinfo[iline].kdf.key,
                                             rglineinfo[iline - 1].kdf.key );

            Assert( cbCommonKey <= cbPrefixOverhead &&
                        rglineinfo[iline].cbCommonPrev == 0 ||
                    cbCommonKey - cbPrefixOverhead ==
                        rglineinfo[iline].cbCommonPrev );
        }
        else
        {
            Assert( rglineinfo[iline].cbCommonPrev == 0 );
        }

        INT     ilineSegLeft;
        INT     ilineSegRight;
        INT     cbSavingsLeft = 0;
        INT     cbSavingsRight = 0;
        ULONG   cbCommonMin;

        //  calculate savings for previous lines
        //
        cbCommonMin = rglineinfo[iline].cbCommonPrev;
        for ( ilineSegLeft = iline;
              ilineSegLeft > 0 && rglineinfo[ilineSegLeft].cbCommonPrev > 0;
              ilineSegLeft-- )
        {
            Assert( cbCommonMin > 0 );
            if ( cbCommonMin > rglineinfo[ilineSegLeft].cbCommonPrev )
            {
                cbCommonMin = rglineinfo[ilineSegLeft].cbCommonPrev;
            }

            cbSavingsLeft += cbCommonMin;
        }

        //  calculate savings for following lines
        //
        for ( ilineSegRight = iline + 1;
              ilineSegRight < clines && rglineinfo[ilineSegRight].cbCommonPrev > 0;
              ilineSegRight++ )
        {
            if ( ilineSegRight == iline + 1 )
            {
                cbCommonMin = rglineinfo[ilineSegRight].cbCommonPrev;
            }
            else if ( cbCommonMin > rglineinfo[ilineSegRight].cbCommonPrev )
            {
                cbCommonMin = rglineinfo[ilineSegRight].cbCommonPrev;
            }
            Assert( cbCommonMin > 0 );
            cbSavingsRight += cbCommonMin;
        }
        ilineSegRight--;

        //  check if savings with iline as prefix
        //      compensate for prefix overhead
        //      and are better than previous prefix candidate
        //
        const INT       cbSavings = cbSavingsLeft + cbSavingsRight - cbPrefixOverhead;
        if ( cbSavings > pprefixinfo->cbSavings )
        {
            Assert( cbSavings > 0 );
            pprefixinfo->ilinePrefix    = iline;
            pprefixinfo->cbSavings      = cbSavings;
            pprefixinfo->ilineSegBegin  = ilineSegLeft;
            pprefixinfo->ilineSegEnd    = ilineSegRight;
        }
    }

    //  set cbCommonPrev for first line to original value
    //
    ((LINEINFO *)rglineinfo)[0].cbCommonPrev = cbCommonPrevSav;
}
#endif // DEBUG


//  selects prefix for clines in rglineinfo
//  places result in *pprefixinfo
//
LOCAL VOID BTISelectPrefix( const LINEINFO  *rglineinfo,
                            INT             clines,
                            PREFIXINFO      *pprefixinfo )
{
    pprefixinfo->Nullify();

    Assert( clines > 0 );
    if ( clines < 2 )
    {
        return;
    }

    if ( 2 == clines )
    {
        INT     cbSavings = rglineinfo[1].cbCommonPrev - cbPrefixOverhead;
        if ( cbSavings > 0 )
        {
            pprefixinfo->ilinePrefix    = 1;
            pprefixinfo->cbSavings      = cbSavings;
            pprefixinfo->ilineSegBegin  = 0;
            pprefixinfo->ilineSegEnd    = 1;
        }

        return;
    }

    //  set cbCommonPrev for first and last line to zero
    //  we exploit this to remove an extra check in the calculation loops
    //  WARNING:  the rglineinfo array must be allocated one entry too large!
    //
    const ULONG cbCommonPrevFirstSav = rglineinfo[0].cbCommonPrev;
    const ULONG cbCommonPrevLastSav  = rglineinfo[clines].cbCommonPrev;
    ((LINEINFO *)rglineinfo)[0].cbCommonPrev = 0;
    ((LINEINFO *)rglineinfo)[clines].cbCommonPrev = 0;

    INT         iline;

    //  UNDONE: optimize loop to use info from previous iteration
    //
    //  calculate prefixinfo for line
    //  if better than previous candidate
    //      choose as new candidate
    //
    for ( iline = 1; iline < clines - 1; iline++ )
    {
#ifdef DEBUG
        if ( iline != 0 )
        {
            ULONG cbCommonKey = CbCommonKey( rglineinfo[iline].kdf.key,
                                             rglineinfo[iline - 1].kdf.key );

            Assert( cbCommonKey <= cbPrefixOverhead &&
                        rglineinfo[iline].cbCommonPrev == 0 ||
                    cbCommonKey - cbPrefixOverhead ==
                        rglineinfo[iline].cbCommonPrev );
        }
        else
        {
            Assert( rglineinfo[iline].cbCommonPrev == 0 );
        }
#endif

        if ( iline != 1 &&
             iline != clines - 2 &&
             rglineinfo[iline + 1].cbCommonPrev >= rglineinfo[iline].cbCommonPrev )
        {
            //  next line would be at least as good a prefix as this one
            //
            continue;
        }

        if (    iline > 1
                && iline != clines - 2
                && rglineinfo[iline - 1].cbCommonPrev > rglineinfo[iline].cbCommonPrev
                && rglineinfo[iline + 1].cbCommonPrev < rglineinfo[iline].cbCommonPrev
             )
        {
            //  previous line would be at a better prefix than this one
            //
            continue;
        }

        INT     ilineSegLeft;
        INT     ilineSegRight;
        INT     cbSavingsLeft = 0;
        INT     cbSavingsRight = 0;
        ULONG   cbCommonMin;

        //  calculate savings for previous lines
        //
        cbCommonMin = rglineinfo[iline].cbCommonPrev;
        for ( ilineSegLeft = iline;
              rglineinfo[ilineSegLeft].cbCommonPrev > 0; //  rglineinfo[0].cbCommonPrev == 0
              ilineSegLeft-- )
        {
            Assert( ilineSegLeft > 0 );
            Assert( cbCommonMin > 0 );
            if ( cbCommonMin > rglineinfo[ilineSegLeft].cbCommonPrev )
            {
                cbCommonMin = rglineinfo[ilineSegLeft].cbCommonPrev;
            }

            cbSavingsLeft += cbCommonMin;
        }

        //  calculate savings for following lines
        //
        cbCommonMin = rglineinfo[iline+1].cbCommonPrev;
        for ( ilineSegRight = iline + 1;
              rglineinfo[ilineSegRight].cbCommonPrev > 0; //  rglineinfo[clines].cbCommonPrev == 0
              ilineSegRight++ )
        {
            Assert( ilineSegRight < clines );
            if ( cbCommonMin > rglineinfo[ilineSegRight].cbCommonPrev )
            {
                cbCommonMin = rglineinfo[ilineSegRight].cbCommonPrev;
            }
            Assert( cbCommonMin > 0 );
            cbSavingsRight += cbCommonMin;
        }
        ilineSegRight--;

        //  check if savings with iline as prefix
        //      compensate for prefix overhead
        //      and are better than previous prefix candidate
        //
        const INT       cbSavings = cbSavingsLeft + cbSavingsRight - cbPrefixOverhead;
        if ( cbSavings > pprefixinfo->cbSavings )
        {
            Assert( cbSavings > 0 );
            pprefixinfo->ilinePrefix    = iline;
            pprefixinfo->cbSavings      = cbSavings;
            pprefixinfo->ilineSegBegin  = ilineSegLeft;
            pprefixinfo->ilineSegEnd    = ilineSegRight;
        }
    }

    //  set cbCommonPrev for first line to original value
    //
    ((LINEINFO *)rglineinfo)[0].cbCommonPrev = cbCommonPrevFirstSav;
    ((LINEINFO *)rglineinfo)[clines].cbCommonPrev = cbCommonPrevLastSav;

    #ifdef DEBUG
    if( FExpensiveDebugCodeEnabled( Debug_BT_Split ) )
    {
        PREFIXINFO  prefixinfoT;
        BTISelectPrefixCheck( rglineinfo, clines, &prefixinfoT );
    //  Assert( prefixinfoT.ilinePrefix == pprefixinfo->ilinePrefix );
        Assert( prefixinfoT.cbSavings == pprefixinfo->cbSavings );
        Assert( prefixinfoT.ilineSegBegin == pprefixinfo->ilineSegBegin );
        Assert( prefixinfoT.ilineSegEnd == pprefixinfo->ilineSegEnd );
    }
    #endif
}


//  remove last line and re-calculate prefix
//
LOCAL VOID BTISelectPrefixDecrement( const LINEINFO *rglineinfo,
                                     INT            clines,
                                     PREFIXINFO     *pprefixinfo )
{
    Assert( pprefixinfo->ilinePrefix <= clines );
    Assert( pprefixinfo->ilineSegBegin <= clines );
    Assert( pprefixinfo->ilineSegEnd <= clines );
    Assert( pprefixinfo->ilineSegBegin <= pprefixinfo->ilineSegEnd );

    if ( pprefixinfo->ilineSegEnd == clines )
    {
        //  removed line contributed to prefix
        //
        Assert( !pprefixinfo->FNull() );
        BTISelectPrefix( rglineinfo, clines, pprefixinfo );
    }
    else
    {
        //  no need to change prefix
        //
///     Assert( fFalse );
    }

    return;
}


//  add line at beginning and re-calculate prefix
//
LOCAL VOID BTISelectPrefixIncrement( const LINEINFO *rglineinfo,
                                     INT            clines,
                                     PREFIXINFO     *pprefixinfo )
{
    Assert( pprefixinfo->ilinePrefix <= clines - 1 );
    Assert( pprefixinfo->ilineSegBegin <= clines - 1);
    Assert( pprefixinfo->ilineSegEnd <= clines - 1 );
    Assert( pprefixinfo->ilineSegBegin <= pprefixinfo->ilineSegEnd );

    if ( clines > 1 &&
         rglineinfo[1].cbCommonPrev == 0 )
    {
        //  added line does not contribute to prefix
        //
        if ( !pprefixinfo->FNull() )
        {
            pprefixinfo->ilinePrefix++;
            pprefixinfo->ilineSegBegin++;
            pprefixinfo->ilineSegEnd++;
        }
    }
    else if ( pprefixinfo->FNull() )
    {

        //  look for prefix only in first segment
        //
        INT     iline;
        for ( iline = 1; iline < clines; iline++ )
        {
            if ( rglineinfo[iline].cbCommonPrev == 0 )
            {
                break;
            }
        }

        BTISelectPrefix( rglineinfo, iline, pprefixinfo );
    }
    else
    {
        //  current prefix should be at or before earlier prefix
        //
        Assert( clines > 1 );
        Assert( pprefixinfo->ilineSegEnd + 1 + 1 <= clines );
        BTISelectPrefix( rglineinfo,
                         pprefixinfo->ilineSegEnd + 1 + 1,
                         pprefixinfo );
    }

    return;
}


//  selects optimal prefix for split page and new page
//  and places result in prefixinfoSplit and prefixinfoNew
//
LOCAL VOID BTISelectPrefixes( SPLIT *psplit, INT ilineSplit )
{
    if ( 0 == ilineSplit )
    {
        //  root page in vertical split has no prefix
        //
        Assert( splittypeVertical == psplit->splittype );
        Assert( psplit->prefixinfoSplit.FNull() );
        BTISelectPrefix( &psplit->rglineinfo[ilineSplit],
                         psplit->clines - ilineSplit,
                         &psplit->prefixinfoNew );
    }
    else
    {
        Assert( psplit->clines > ilineSplit );

        //  select prefix for split page
        //
        if ( psplit->clines - 1 == ilineSplit )
        {
            Assert( psplit->prefixinfoNew.FNull() );

            BTISelectPrefix( psplit->rglineinfo,
                             ilineSplit,
                             &psplit->prefixinfoSplit );
        }
        else
        {
            BTISelectPrefixDecrement( psplit->rglineinfo,
                                      ilineSplit,
                                      &psplit->prefixinfoSplit );
        }

        //  select prefix for new page
        //
        BTISelectPrefixIncrement( &psplit->rglineinfo[ilineSplit],
                                  psplit->clines - ilineSplit,
                                  &psplit->prefixinfoNew );
    }
}


//  sets cbPrefix for clines in rglineinfo
//  based on prefix selected in prefixinfo
//
LOCAL VOID BTISetPrefix( LINEINFO *rglineinfo, INT clines, const PREFIXINFO& prefixinfo )
{
    Assert( clines > 0 );

    INT         iline;

    //  set all cbPrefix to zero
    //
    for ( iline = 0; iline < clines; iline++ )
    {
        rglineinfo[iline].cbPrefix = 0;

#ifdef DEBUG
        if ( iline != 0 )
        {
            ULONG cbCommonKey = CbCommonKey( rglineinfo[iline].kdf.key,
                                             rglineinfo[iline - 1].kdf.key );

            Assert( cbCommonKey <= cbPrefixOverhead &&
                        rglineinfo[iline].cbCommonPrev == 0 ||
                    cbCommonKey - cbPrefixOverhead ==
                        rglineinfo[iline].cbCommonPrev );
        }
#endif
    }

    if ( ilineInvalid == prefixinfo.ilinePrefix )
    {
        return;
    }

    //  set cbPrefix to appropriate value for lines in prefix segment
    //
    const INT   ilineSegLeft    = prefixinfo.ilineSegBegin;
    const INT   ilineSegRight   = prefixinfo.ilineSegEnd;
    const INT   ilinePrefix     = prefixinfo.ilinePrefix;
    ULONG       cbCommonMin;

    #ifdef DEBUG
    INT     cbSavingsLeft = 0;
    INT     cbSavingsRight = 0;
    #endif

    Assert( ilineSegLeft != ilineSegRight );

    //  cbPrefix for ilinePrefix
    //
    rglineinfo[ilinePrefix].cbPrefix = rglineinfo[ilinePrefix].kdf.key.Cb();

    //  cbPrefix for previous lines
    //
    cbCommonMin = rglineinfo[ilinePrefix].cbCommonPrev;
    for ( iline = ilinePrefix; iline > ilineSegLeft; iline-- )
    {
        Assert( cbCommonMin > 0 );
        Assert( iline > 0 );
        Assert( rglineinfo[iline].cbCommonPrev > 0 );

        if ( cbCommonMin > rglineinfo[iline].cbCommonPrev )
        {
            cbCommonMin = rglineinfo[iline].cbCommonPrev;
        }

        rglineinfo[iline-1].cbPrefix = cbCommonMin + cbPrefixOverhead;

        #ifdef DEBUG
        cbSavingsLeft += cbCommonMin;
        #endif
    }

    //  calculate savings for following lines
    //
    for ( iline = ilinePrefix + 1; iline <= ilineSegRight; iline++ )
    {
        Assert( iline > 0 );
        if ( iline == ilinePrefix + 1 )
        {
            cbCommonMin = rglineinfo[iline].cbCommonPrev;
        }
        else if ( cbCommonMin > rglineinfo[iline].cbCommonPrev )
        {
            cbCommonMin = rglineinfo[iline].cbCommonPrev;
        }

        rglineinfo[iline].cbPrefix = cbCommonMin + cbPrefixOverhead;

        #ifdef DEBUG
        cbSavingsRight += cbCommonMin;
        #endif
    }

    #ifdef DEBUG
    //  check if savings are same as in prefixinfo
    const INT   cbSavings = cbSavingsLeft + cbSavingsRight - cbPrefixOverhead;

    Assert( cbSavings > 0 );
    Assert( prefixinfo.ilinePrefix == ilinePrefix );
    Assert( prefixinfo.cbSavings == cbSavings );
    #endif
}

//  sets cbPrefix in all lineinfo to correspond to chosen prefix
//
VOID BTISplitSetPrefixes( SPLIT *psplit )
{
    const INT   ilineSplit = psplit->ilineSplit;

    if ( 0 == ilineSplit )
    {
        //  root page in vertical split has no prefix
        //
        Assert( splittypeVertical == psplit->splittype );
        Assert( ilineInvalid == psplit->prefixinfoSplit.ilinePrefix );
    }
    else
    {
        //  select prefix for split page
        //
        BTISetPrefix( psplit->rglineinfo,
                      ilineSplit,
                      psplit->prefixinfoSplit );
    }

    //  select prefix for new page
    //
    Assert( psplit->clines > ilineSplit );
    BTISetPrefix( &psplit->rglineinfo[ilineSplit],
                  psplit->clines - ilineSplit,
                  psplit->prefixinfoNew );
}

//  get new pages for split
//
LOCAL ERR ErrBTIGetNewPages( FUCB *pfucb, SPLITPATH *psplitPathLeaf, DIRFLAG dirflag )
{
    ERR         err;
    SPLITPATH   *psplitPath;
    const BOOL fLogging = !( dirflag & fDIRNoLog ) && g_rgfmp[pfucb->ifmp].FLogOn();

    //  find pcsrRoot for pfucb
    //
    Assert( pfucb->pcsrRoot == pcsrNil );
    for ( psplitPath = psplitPathLeaf;
          psplitPath->psplitPathParent != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
        //  all logic in for loop
        //
    }
    pfucb->pcsrRoot = &psplitPath->csr;

    //  get a new page for every split
    //
    Assert( psplitPath->psplitPathParent == NULL );
    for ( ; psplitPath != NULL; psplitPath = psplitPath->psplitPathChild )
    {
        if ( psplitPath->psplit != NULL )
        {
            SPLIT * psplit      = psplitPath->psplit;

            Assert( psplit->psplitPath == psplitPath );

            BTICheckSplitFlags( psplit );

            //  choose the space pool based on the split characteristics
            //
            //  NOTE:  Special case:  presume append split if we are doing the first vertical split
            //  of an LV tree to compensate for the trick where we insert the LVCHUNK before the
            //  LVROOT to avoid placing them on separate pages.  This makes us think this isn't an
            //  append when it really is.
            //
            ULONG fSPAllocFlags = 0;
            if ( psplit->fAppend ||
                 (  psplit->splittype == splittypeVertical &&
                    psplitPath->csr.Cpage().FRootPage() &&
                    psplitPath->csr.Cpage().FLeafPage() &&
                    psplitPath->csr.Cpage().FLongValuePage() ) )
            {
                fSPAllocFlags |= pfucb->u.pfcb->FContiguousAppend() ? fSPContinuous : 0;
            }
            if ( psplit->fHotpoint )
            {
                fSPAllocFlags |= pfucb->u.pfcb->FContiguousHotpoint() ? fSPContinuous : 0;
            }

            const BOOL fVerticalToLeafSplit = ( psplit->splittype == splittypeVertical ) &&
                                                ( psplit->fSplitPageFlags & CPAGE::fPageRoot ) &&
                                                ( psplit->fSplitPageFlags & CPAGE::fPageParentOfLeaf );

            // If we are on a disk with no seek penalty, it doesn't
            // make sense to optimize for contiguous access
            if ( !g_rgfmp[pfucb->ifmp].FSeekPenalty() )
            {
                fSPAllocFlags &= ~fSPContinuous;
            }

            fSPAllocFlags |= pfucb->u.pfcb->FUtilizeExactExtents() ? fSPExactExtent : 0;

            Assert( !!psplitPath->csr.Cpage().FLeafPage() == !!( psplit->fNewPageFlags & CPAGE::fPageLeaf ) );  // should match, or I(SOMEONE) am lost.
            if ( CpgDIRActiveSpaceRequestReserve( pfucb ) != 0 &&
                    CpgDIRActiveSpaceRequestReserve( pfucb ) != cpgDIRReserveConsumed &&
                    ( ( psplit->fNewPageFlags & CPAGE::fPageLeaf ) || fVerticalToLeafSplit ) )
            {
                //  If there is an active reserve request outstanding and this is a leaf page allocation,
                //  indicate to space to consume the active reserve.
                fSPAllocFlags |= ( fSPContinuous | fSPUseActiveReserve );
                if ( fVerticalToLeafSplit )
                {
                    //  Because we need an additional page for whatever we're about to move off the root to replace
                    //  with pgno pointers, we adjust the active reserve up by one.
                    const CPG cpgReserveAdjust = CpgDIRActiveSpaceRequestReserve( pfucb ) + 1;
                    DIRSetActiveSpaceRequestReserve( pfucb, cpgReserveAdjust );
                }
            }

            //  pass in split page for getting locality
            //
            PGNO    pgnoNew     = pgnoNull;
            Call( ErrSPGetPage( pfucb, psplitPath->csr.Pgno(), fSPAllocFlags, &pgnoNew ) );
            psplit->pgnoNew = pgnoNew;

            Call( psplit->csrNew.ErrGetNewPreInitPage(
                    pfucb->ppib,
                    pfucb->ifmp,
                    psplit->pgnoNew,
                    ObjidFDP( pfucb ),
                    fLogging ) );
            psplit->csrNew.ConsumePreInitPage( psplit->fNewPageFlags );

            Assert( latchWrite == psplit->csrNew.Latch() );

            //  count pages allocated by an update
            //
            Ptls()->threadstats.cPageUpdateAllocated++;
        }
    }

    pfucb->pcsrRoot = pcsrNil;
    return JET_errSuccess;

HandleError:
    Assert( err < JET_errSuccess );
    //  release all latched pages
    //  free all allocated pages
    //
    for ( psplitPath = psplitPathLeaf;
          psplitPath != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
        if ( psplitPath->psplit != NULL &&
             pgnoNull != psplitPath->psplit->pgnoNew )
        {
            SPLIT   *psplit = psplitPath->psplit;

            if ( psplit->csrNew.Pgno() == psplit->pgnoNew )
            {
                Assert( latchWrite == psplit->csrNew.Latch() );

                psplit->csrNew.ReleasePage();
                psplit->csrNew.Reset();
            }

            //  UNDONE: we will leak the space if ErrSPFreeExt() fails
            //
            const ERR   errFreeExt  = ErrSPFreeExt( pfucb, psplit->pgnoNew, 1, "FailedNewPages" );
#ifdef DEBUG
            if ( !FSPExpectedError( errFreeExt ) )
            {
                CallS( errFreeExt );
            }
#endif

            psplit->pgnoNew = pgnoNull;

            //  count pages released by an update
            //
            Ptls()->threadstats.cPageUpdateReleased++;
        }
    }

    pfucb->pcsrRoot = pcsrNil;
    return err;
}


//  release latches on pages that are not in the split
//      this might cause psplitPathLeaf to change\
//
LOCAL VOID BTISplitReleaseUnneededPages( INST *pinst, SPLITPATH **ppsplitPathLeaf )
{
    SPLITPATH   *psplitPath;
    SPLITPATH   *psplitPathNewLeaf = NULL;

    //  go to root
    //  since we need to latch top-down
    //
    for ( psplitPath = *ppsplitPathLeaf;
          psplitPath->psplitPathParent != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
    }

    for ( ; NULL != psplitPath;  )
    {
        //  check if page is needed
        //      -- either there is a split at this level
        //         or there is a split one level below
        //          when we need write latch for inserting page pointer
        //
        SPLIT   *psplit = psplitPath->psplit;

        if ( psplit == NULL &&
             ( psplitPath->psplitPathChild == NULL ||
               psplitPath->psplitPathChild->psplit == NULL ) )
        {
            //  release latch and psplitPath at this level
            //
            SPLITPATH *psplitPathT = psplitPath;
            psplitPath = psplitPath->psplitPathChild;

            delete psplitPathT;
        }
        else
        {
            //  update new leaf
            //
            Assert( NULL == psplitPathNewLeaf ||
                    psplitPath == psplitPathNewLeaf->psplitPathChild );

            psplitPathNewLeaf = psplitPath;
            psplitPath = psplitPath->psplitPathChild;
        }
    }
    Assert( psplitPathNewLeaf != NULL );
    Assert( psplitPathNewLeaf->psplit != NULL );

    *ppsplitPathLeaf = psplitPathNewLeaf;

    return;
}


//  upgrade to write latch on all pages invloved in the split
//  new pages are already latched
//
LOCAL ERR ErrBTISplitUpgradeLatches( const IFMP ifmp, SPLITPATH * const psplitPathLeaf )
{
    ERR             err;
    SPLITPATH       * psplitPath;
    const DBTIME    dbtimeSplit     = g_rgfmp[ifmp].DbtimeIncrementAndGet();

    Assert( dbtimeSplit > 1 );
    Assert( PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() != fRecoveringRedo
        || g_rgfmp[ifmp].FCreatingDB() );         //  may hit this code path during recovery if explicitly redoing CreateDb

    //  go to root
    //  since we need to latch top-down
    //
    for ( psplitPath = psplitPathLeaf;
          psplitPath->psplitPathParent != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
    }

    Assert( NULL == psplitPath->psplitPathParent );
    for ( ; NULL != psplitPath;  psplitPath = psplitPath->psplitPathChild )
    {
        //  assert write latch is needed
        //      -- either there is a split at this level
        //         or there is a split one level below
        //          when we need write latch for inserting page pointer
        //
        SPLIT   *psplit = psplitPath->psplit;

        Assert( psplit != NULL ||
                ( psplitPath->psplitPathChild != NULL &&
                  psplitPath->psplitPathChild->psplit != NULL ) );
        Assert( psplitPath->csr.Latch() == latchWrite ||
                psplitPath->csr.Latch() == latchRIW );

        //  Setting dbtime befores to dbtimeNil, so cleanup logic works.
        //
        // If this doesn't hold, the BTISplitRevertDbtime() logic won't work out.
        Assert( dbtimeInvalid == psplitPath->dbtimeBefore || dbtimeNil == psplitPath->dbtimeBefore );

        psplitPath->dbtimeBefore = dbtimeNil;
        psplitPath->fFlagsBefore = 0;

        if ( psplitPath->psplitPathChild == NULL &&
            pgnoNull != psplit->csrRight.Pgno() )
        {
            Assert( psplit );
            Assert( dbtimeInvalid == psplit->dbtimeRightBefore || dbtimeNil == psplit->dbtimeRightBefore );
            psplit->dbtimeRightBefore = dbtimeNil;
            psplit->fFlagsRightBefore = 0;
        }

        //  Grab any write latches that may be required.
        //

        if ( psplitPath->csr.Latch() != latchWrite )
        {
            psplitPath->csr.UpgradeFromRIWLatch();
        }

        //  Touch the dbtimes appropriately.
        //

        Assert( psplitPath->csr.Latch() == latchWrite );
        if ( psplitPath->csr.Dbtime() < dbtimeSplit )
        {
            psplitPath->dbtimeBefore = psplitPath->csr.Dbtime();
            psplitPath->fFlagsBefore = psplitPath->csr.Cpage().FFlags();
            psplitPath->csr.CoordinatedDirty( dbtimeSplit );
        }
        else
        {
            FireWall( "SplitPathDbTimeTooHigh" );
            OSUHAEmitFailureTag( PinstFromIfmp( ifmp ), HaDbFailureTagCorruption, L"fa115e37-2234-4e2e-ba6c-dcb0cac38711" );
            Error( ErrERRCheck( JET_errDbTimeCorrupted ) );
        }

        //  new page will already be write latched
        //  dirty it and update max dbtime
        //
        if ( psplit != NULL )
        {
            Assert( psplit->csrNew.Latch() == latchWrite );
            psplit->csrNew.CoordinatedDirty( dbtimeSplit );
        }

        //  write latch right page at leaf-level
        //
        if ( psplitPath->psplitPathChild == NULL )
        {
            Assert( psplit != NULL );
            Assert( psplitPath->csr.Cpage().FLeafPage()
                || ( splitoperNone == psplit->splitoper
                    && splittypeVertical == psplit->splittype ) );

            if ( pgnoNull != psplit->csrRight.Pgno() )
            {
                Assert( psplit->splittype != splittypeAppend );
                Assert( psplit->csrRight.Cpage().FLeafPage() );

                psplit->csrRight.UpgradeFromRIWLatch();

                if ( psplit->csrRight.Dbtime() < dbtimeSplit )
                {
                    psplit->dbtimeRightBefore = psplit->csrRight.Dbtime();
                    psplit->fFlagsRightBefore = psplit->csrRight.Cpage().FFlags();
                    psplit->csrRight.CoordinatedDirty( dbtimeSplit );
                }
                else
                {
                    FireWall( "SplitRightDbTimeTooHigh" );
                    OSUHAEmitFailureTag( PinstFromIfmp( ifmp ), HaDbFailureTagCorruption, L"b6cd3237-b9a7-4839-b5a5-2973d981269a" );
                    Error( ErrERRCheck( JET_errDbTimeCorrupted ) );
                }
            }
            else
            {
                psplit->dbtimeRightBefore = dbtimeNil;
                psplit->fFlagsRightBefore = 0;
            }
        }
    }

    return JET_errSuccess;

HandleError:
    Assert( err < 0 );
    BTISplitRevertDbtime( psplitPathLeaf );
    return err;
}


//  sets dbtime of every (write) latched page to given dbtime
//
VOID BTISplitRevertDbtime( SPLITPATH *psplitPathLeaf )
{
    SPLITPATH *psplitPath = psplitPathLeaf;

    Assert( NULL == psplitPath->psplitPathChild );

    for ( ; NULL != psplitPath;
            psplitPath = psplitPath->psplitPathParent )
    {

        Assert( latchWrite == psplitPath->csr.Latch() );

        Assert( dbtimeInvalid != psplitPath->dbtimeBefore );
        Assert( psplitPath->dbtimeBefore < psplitPath->csr.Dbtime() );

        if ( latchWrite == psplitPath->csr.Latch() &&
            dbtimeNil != psplitPath->dbtimeBefore &&
            dbtimeInvalid != psplitPath->dbtimeBefore /* defense in depth */ )
        {
            //  set the dbtime for this page
            //
            psplitPath->csr.RevertDbtime( psplitPath->dbtimeBefore, psplitPath->fFlagsBefore );
        }

        SPLIT   *psplit = psplitPath->psplit;

        //  set dbtime for sibling and new pages
        //
        if ( psplit != NULL && pgnoNull != psplit->csrRight.Pgno() )
        {

            Assert( psplit->splittype != splittypeAppend );
            Assert( psplit->csrRight.Cpage().FLeafPage() );

            Assert( dbtimeInvalid != psplit->dbtimeRightBefore );
            Assert( psplit->dbtimeRightBefore < psplit->csrRight.Dbtime() );

            if ( dbtimeNil != psplit->dbtimeRightBefore &&
                dbtimeInvalid != psplit->dbtimeRightBefore && /* defense in depth */
                latchWrite == psplit->csrRight.Latch() /* defense in depth, should've been guaranteed write after dbtimeNil check */ )
            {
                psplit->csrRight.RevertDbtime( psplit->dbtimeRightBefore, psplit->fFlagsRightBefore );
            }
        }
    }
}


//  sets lgpos for all pages involved in split
//
LOCAL VOID BTISplitSetLgpos( SPLITPATH *psplitPathLeaf, const LGPOS& lgpos )
{
    SPLITPATH   *psplitPath = psplitPathLeaf;

    for ( ; psplitPath != NULL ; psplitPath = psplitPath->psplitPathParent )
    {
        Assert( psplitPath->csr.FDirty() );

        psplitPath->csr.Cpage().SetLgposModify( lgpos );

        SPLIT   *psplit = psplitPath->psplit;

        if ( psplit != NULL )
        {
            psplit->csrNew.Cpage().SetLgposModify( lgpos );

            if ( psplit->csrRight.Pgno() != pgnoNull )
            {
                Assert( psplit->csrRight.Cpage().FLeafPage() );
                psplit->csrRight.Cpage().SetLgposModify( lgpos );
            }
        }
    }
}


//  gets node to replace or flagInsertAndReplaceData at leaf level
//
VOID BTISplitGetReplacedNode( FUCB *pfucb, SPLIT *psplit )
{
    Assert( psplit != NULL );
    Assert( splitoperFlagInsertAndReplaceData == psplit->splitoper ||
            splitoperReplace == psplit->splitoper );

    CSR     *pcsr = &psplit->psplitPath->csr;
    Assert( pcsr->Cpage().FLeafPage() );
    Assert( pcsr->Cpage().Clines() > psplit->ilineOper );

    pcsr->SetILine( psplit->ilineOper );
    NDGet( pfucb, pcsr );
}


VOID BTISplitInsertIntoRCELists( FUCB               *pfucb,
                                 SPLITPATH          *psplitPath,
                                 const KEYDATAFLAGS *pkdf,
                                 RCE                *prce1,
                                 RCE                *prce2,
                                 VERPROXY           *pverproxy )
{
    Assert( splitoperNone != psplitPath->psplit->splitoper );

    SPLIT   *psplit = psplitPath->psplit;
    CSR     *pcsrOper = psplit->ilineOper < psplit->ilineSplit ?
                            &psplitPath->csr : &psplit->csrNew;

    if ( splitoperInsert != psplitPath->psplit->splitoper
        && !PinstFromIfmp( pfucb->ifmp )->FRecovering()
        && FBTIUpdatablePage( *pcsrOper ) )
    {
        Assert( latchWrite == psplitPath->csr.Latch() );
        Assert( pcsrOper->Cpage().FLeafPage() );
        BTISplitSetCbAdjust( psplitPath->psplit,
                             pfucb,
                             *pkdf,
                             prce1,
                             prce2 );
    }

    VERInsertRCEIntoLists( pfucb, pcsrOper, prce1, pverproxy );
    if ( prceNil != prce2 )
    {
        VERInsertRCEIntoLists( pfucb, pcsrOper, prce2, pverproxy );
    }

    return;
}


VOID BTISplitSetCbAdjust( SPLIT                 *psplit,
                          FUCB                  *pfucb,
                          const KEYDATAFLAGS&   kdf,
                          const RCE             *prce1,
                          const RCE             *prce2 )
{
    Assert( NULL != psplit );

    const SPLITOPER splitoper   = psplit->splitoper;
    const RCE       *prceReplace;

    Assert( splitoperReplace == splitoper ||
            splitoperFlagInsertAndReplaceData == splitoper );

    BTISplitGetReplacedNode( pfucb, psplit );
    const INT   cbDataOld = pfucb->kdfCurr.data.Cb();

    if ( splitoper == splitoperReplace )
    {
        Assert( NULL == prce2 );
        prceReplace = prce1;
    }
    else
    {
        Assert( NULL != prce2 );
        prceReplace = prce2;
    }

    Assert( cbDataOld < kdf.data.Cb() );
    Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() );
    VERSetCbAdjust( &psplit->psplitPath->csr,
                    prceReplace,
                    kdf.data.Cb(),
                    cbDataOld,
                    fDoNotUpdatePage );
}


//  sets cursor to point to ilineOper if requested
//  leaf-level operation was performed successfully
//
VOID BTISplitSetCursor( FUCB *pfucb, SPLITPATH *psplitPathLeaf )
{
    SPLIT   *psplit = psplitPathLeaf->psplit;

    Assert( NULL != psplit );
    Assert( !Pcsr( pfucb )->FLatched() );

    if ( splitoperNone == psplit->splitoper ||
         !psplit->csrNew.Cpage().FLeafPage() )
    {
        //  split was not performed
        //  set cursor to point to no valid node
        //
        BTUp( pfucb );
    }
    else
    {
        //  set Pcsr( pfucb ) to leaf page with oper
        //  reset CSR copied from to point to no page
        //
        Assert( psplit->csrNew.Cpage().FLeafPage() );
        Assert( splitoperNone != psplitPathLeaf->psplit->splitoper );
        Assert( !Pcsr( pfucb )->FLatched() );

        if ( psplit->ilineOper < psplit->ilineSplit )
        {
            //  ilineOper falls in split page
            //
            *Pcsr( pfucb )          = psplitPathLeaf->csr;
            Pcsr( pfucb )->SetILine( psplit->ilineOper );
        }
        else
        {
            *Pcsr( pfucb )          = psplit->csrNew;
            Pcsr( pfucb )->SetILine( psplit->ilineOper - psplit->ilineSplit );
        }

        Assert( Pcsr( pfucb )->ILine() < Pcsr( pfucb )->Cpage().Clines() );
        NDGet( pfucb );
    }
}


//  performs split
//  this code shared between do and redo phases
//      insert parent page pointers
//      fix sibling page pointers at leaf
//      move nodes
//      set dependencies
//
VOID BTIPerformSplit( FUCB          *pfucb,
                      SPLITPATH     *psplitPathLeaf,
                      KEYDATAFLAGS  *pkdf,
                      DIRFLAG       dirflag )
{
    SPLITPATH   *psplitPath;

    ASSERT_VALID( pkdf );

    auto tc = TcCurr();

    //  go to root
    //  since we need to latch top-down
    //
    for ( psplitPath = psplitPathLeaf;
          psplitPath->psplitPathParent != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
    }

    for ( ; psplitPath != NULL; psplitPath = psplitPath->psplitPathChild )
    {
        SPLIT   *psplit = psplitPath->psplit;

        if ( psplit == NULL )
        {
            Assert( psplitPath->psplitPathChild != NULL &&
                    psplitPath->psplitPathChild->psplit != NULL );

            //  insert parent page pointer for next level
            //
            BTIInsertPgnoNewAndSetPgnoSplit( pfucb, psplitPath );

            continue;
        }

        //  finish initializing new page before making changes to it
        //
        if ( FBTIUpdatablePage( psplit->csrNew ) )
        {
            psplit->csrNew.FinalizePreInitPage();
        }

        KEYDATAFLAGS    *pkdfOper;

        if ( splitoperNone == psplit->splitoper )
        {
            pkdfOper = NULL;
        }
        else if ( psplit->fNewPageFlags & CPAGE::fPageLeaf )
        {
            pkdfOper = pkdf;
        }
        else
        {
            Assert( psplitPath->psplitPathChild != NULL &&
                    psplitPath->psplitPathChild->psplit != NULL );
            pkdfOper = &psplitPath->psplitPathChild->psplit->kdfParent;
        }

        if ( psplit->splittype == splittypeVertical )
        {
            PERFOpt( PERFIncCounterTable( cBTVerticalSplit, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );

            //  move all nodes from root to new page
            //
            BTISplitMoveNodes( pfucb, psplit, pkdfOper, dirflag );

            CSR     *pcsrRoot = &psplit->psplitPath->csr;

            if ( FBTIUpdatablePage( *pcsrRoot ) )
            {
                Assert( latchWrite == pcsrRoot->Latch() );
                Assert( pcsrRoot->Cpage().FRootPage() );

                BOOL    fPageFDPDeleteBefore = pcsrRoot->Cpage().FPageFDPDelete();

                //  set parent page to non-leaf
                //
                pcsrRoot->Cpage().SetFlags( psplit->fSplitPageFlags );

                // If page had fPageFDPDelete flag set before, set it back.
                // It is possible this is the available lag and it was reverted back with a deleted table.
                if ( fPageFDPDeleteBefore )
                {
                    pcsrRoot->Cpage().SetPageFDPDelete( fTrue );
                }

                //  insert page pointer in root zero-sized key
                //
                Assert( NULL != psplit->psplitPath );
                Assert( 0 == pcsrRoot->Cpage().Clines() );

#ifdef DEBUG
                //  check prefix in root is null
                //
                NDGetPrefix( pfucb, pcsrRoot );
                Assert( pfucb->kdfCurr.key.prefix.FNull() );
#endif

                KEYDATAFLAGS        kdf;
                LittleEndian<PGNO>  le_pgnoNew = psplit->pgnoNew;

                kdf.key.Nullify();
                kdf.fFlags = 0;
                Assert( psplit->csrNew.Pgno() == psplit->pgnoNew );
                kdf.data.SetPv( &le_pgnoNew );
                kdf.data.SetCb( sizeof( PGNO ) );
                NDInsert( pfucb, pcsrRoot, &kdf );
            }

            if ( psplitPath->psplitPathChild != NULL &&
                 psplitPath->psplitPathChild->psplit != NULL )
            {
                //  replace data in ilineOper + 1 with pgnoNew
                //  assert data in ilineOper is pgnoSplit
                //
                BTIInsertPgnoNew( pfucb, psplitPath );
                AssertBTIVerifyPgnoSplit( pfucb, psplitPath );
            }
        }
        else
        {
            if ( splittypeAppend == psplit->splittype )
            {
                PERFOpt( PERFIncCounterTable( cBTAppendSplit, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );
            }
            else
            {
                Assert( splittypeRight == psplit->splittype );
                PERFOpt( PERFIncCounterTable( cBTRightSplit, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );
                if ( psplit->fHotpoint )
                {
                    PERFOpt( PERFIncCounterTable( cBTRightHotpointSplit, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );
                }
            }

            BTISplitMoveNodes( pfucb, psplit, pkdfOper, dirflag );

            if ( psplit->fNewPageFlags & CPAGE::fPageLeaf )
            {
                //  set sibling page pointers
                //
                BTISplitFixSiblings( psplit );
            }
            else
            {
                //  set page pointers
                //
                //  internal pages have no sibling pointers
                //
#ifdef DEBUG
                if ( FBTIUpdatablePage( psplit->csrNew ) )
                {
                    Assert( pgnoNull == psplit->csrNew.Cpage().PgnoPrev() );
                    Assert( pgnoNull == psplit->csrNew.Cpage().PgnoNext() );
                }
                if ( FBTIUpdatablePage( psplitPath->csr ) )
                {
                    Assert( pgnoNull == psplitPath->csr.Cpage().PgnoPrev() );
                    Assert( pgnoNull == psplitPath->csr.Cpage().PgnoNext() );
                }
                Assert( pgnoNull == psplit->csrRight.Pgno() );
#endif

                //  replace data in ilineOper + 1 with pgnoNew
                //  assert data in ilineOper is pgnoSplit
                //
                BTIInsertPgnoNew( pfucb, psplitPath );
                AssertBTIVerifyPgnoSplit( pfucb, psplitPath );
            }
        }
    }
}


//  inserts kdfParent of lower level with pgnoSplit as data
//  replace data of next node with pgnoNew
//
LOCAL VOID BTIInsertPgnoNewAndSetPgnoSplit( FUCB *pfucb, SPLITPATH *psplitPath )
{
    ERR         err;
    CSR         *pcsr = &psplitPath->csr;
    DATA        data;
    BOOKMARK    bmParent;

    if ( !FBTIUpdatablePage( *pcsr ) )
    {
        //  page does not need redo
        //
        return;
    }
    Assert( latchWrite == pcsr->Latch() );

    Assert( NULL == psplitPath->psplit );
    Assert( !psplitPath->csr.Cpage().FLeafPage() );
    Assert( psplitPath->psplitPathChild != NULL );
    Assert( psplitPath->psplitPathChild->psplit != NULL );

    SPLIT   *psplit = psplitPath->psplitPathChild->psplit;

    Assert( !psplitPath->csr.Cpage().FLeafPage() );
    Assert( sizeof(PGNO) == psplit->kdfParent.data.Cb() );
    Assert( psplit->pgnoSplit ==
            *( reinterpret_cast<UnalignedLittleEndian<PGNO> *>( psplit->kdfParent.data.Pv() ) ) );

    bmParent.key    = psplit->kdfParent.key;
    bmParent.data.Nullify();
    //  we will be in trouble if we get an error here ... luckily I think this should be
    //  impossible b/c I think we will have ErrNDSeek()d in there already and failed out
    //  from there.  Not 100% sure of that though.
    err = ErrNDSeek( pfucb, pcsr, bmParent );
    AssertRTL( err != JET_errBadEmptyPage );
    Assert( err != JET_errSuccess );
    Assert( err != wrnNDFoundLess );
    Assert( err == wrnNDFoundGreater );
    Assert( psplit->pgnoSplit ==
            *( reinterpret_cast<UnalignedLittleEndian< PGNO > *> ( pfucb->kdfCurr.data.Pv() ) ) );
    Assert( pcsr->FDirty() );

    BTIComputePrefixAndInsert( pfucb, pcsr, psplit->kdfParent );

    //  go to next node and update pgno to pgnoNew
    //
    Assert( pcsr->ILine() < pcsr->Cpage().Clines() );
    pcsr->IncrementILine();
#ifdef DEBUG
    //  current page pointer should point to pgnoSplit
    //
    NDGet( pfucb, pcsr );
    Assert( sizeof(PGNO) == pfucb->kdfCurr.data.Cb() );
    Assert( psplit->pgnoSplit ==
            *( reinterpret_cast<UnalignedLittleEndian< PGNO > *>
                        ( pfucb->kdfCurr.data.Pv() ) ) );
#endif

    LittleEndian<PGNO> le_pgnoNew = psplit->pgnoNew;
    data.SetCb( sizeof( PGNO ) );
    data.SetPv( reinterpret_cast<VOID *> (&le_pgnoNew) );
    NDReplace( pcsr, &data );
}


//  computes prefix for node with repect to given page
//  inserts with appropriate prefix
//
LOCAL VOID BTIComputePrefixAndInsert( FUCB *pfucb, CSR *pcsr, const KEYDATAFLAGS& kdf )
{
    Assert( latchWrite == pcsr->Latch() );

    INT     cbCommon = CbNDCommonPrefix( pfucb, pcsr, kdf.key );

    if ( cbCommon <= cbPrefixOverhead )
    {
        cbCommon = 0;
    }

    NDInsert( pfucb, pcsr, &kdf, cbCommon );
    return;
}


//  replace data in ilineOper + 1 with pgnoNew at lower level
//
LOCAL VOID BTIInsertPgnoNew( FUCB *pfucb, SPLITPATH *psplitPath )
{
    SPLIT   *psplit = psplitPath->psplit;
    CSR     *pcsr;
    DATA    data;

    Assert( psplit != NULL );
    Assert( splittypeRight == psplit->splittype
        || ( splittypeVertical == psplit->splittype
                && psplitPath->psplitPathChild != NULL
                && psplitPath->psplitPathChild->psplit != NULL ) );
    Assert( psplit->ilineOper < psplit->clines - 1 );
    Assert( psplitPath->psplitPathChild != NULL );
    Assert( psplitPath->psplitPathChild->psplit != NULL );
    Assert( !FBTIUpdatablePage( psplitPath->csr )
        || !psplitPath->csr.Cpage().FLeafPage() );
    Assert( !FBTIUpdatablePage( psplit->csrNew )
        || !psplit->csrNew.Cpage().FLeafPage() );

    LittleEndian<PGNO> le_pgnoNew = psplitPath->psplitPathChild->psplit->pgnoNew;
    data.SetCb( sizeof( PGNO ) );
    data.SetPv( reinterpret_cast<BYTE *>(&le_pgnoNew) );

    if ( psplit->ilineOper + 1 >= psplit->ilineSplit )
    {
        //  page pointer to new page falls in new page
        //
        pcsr            = &psplit->csrNew;
        pcsr->SetILine( psplit->ilineOper + 1 - psplit->ilineSplit );
    }
    else
    {
        //  page pointer falls in split page
        //
        Assert( splittypeVertical != psplit->splittype );
        pcsr            = &psplit->psplitPath->csr;
        pcsr->SetILine( psplit->ilineOper + 1 );
    }

    if ( !FBTIUpdatablePage( *pcsr ) )
    {
        //  page does not need redo
        //
        return;
    }
    Assert( latchWrite == pcsr->Latch() );

    //  check that we already dirtied these pages
    //
    Assert( pcsr->FDirty() );

#ifdef DEBUG
    //  current page pointer should point to pgnoSplit
    //
    NDGet( pfucb, pcsr );
    Assert( sizeof(PGNO) == pfucb->kdfCurr.data.Cb() );
    Assert( psplitPath->psplitPathChild->psplit->pgnoSplit ==
            *( reinterpret_cast<UnalignedLittleEndian< PGNO >  *>
                    (pfucb->kdfCurr.data.Pv() ) ) );
#endif

    NDReplace( pcsr, &data );
}


//  fixes sibling pages of split, new and right pages
//
LOCAL VOID BTISplitFixSiblings( SPLIT *psplit )
{
    SPLITPATH   *psplitPath = psplit->psplitPath;

    //  set sibling page pointers only if page is write-latched
    //
    if ( FBTIUpdatablePage( psplit->csrNew ) )
    {
        psplit->csrNew.Cpage().SetPgnoPrev( psplit->pgnoSplit );
    }

    if ( FBTIUpdatablePage( psplitPath->csr ) )
    {
        psplitPath->csr.Cpage().SetPgnoNext( psplit->pgnoNew );
    }

    if ( pgnoNull != psplit->csrRight.Pgno() )
    {
        Assert( psplit->splittype == splittypeRight );

        if ( FBTIUpdatablePage( psplit->csrRight ) )
        {
            Assert( psplit->csrRight.FDirty() );
            psplit->csrRight.Cpage().SetPgnoPrev( psplit->pgnoNew );
        }

        if ( FBTIUpdatablePage( psplit->csrNew ) )
        {
            psplit->csrNew.Cpage().SetPgnoNext( psplit->csrRight.Pgno() );
        }
    }
    else
    {
        Assert( !FBTIUpdatablePage( psplit->csrNew ) ||
                pgnoNull == psplit->csrNew.Cpage().PgnoNext() );
    }
    return;
}


//  move nodes from src to dest page
//  set prefix in destination page
//  move nodes >= psplit->ilineSplit
//  if oper is not operNone, perform oper on ilineOper
//  set prefix in src page [in-page]
//  set cbUncommittedFree in src and dest pages
//  move undoInfo of moved nodes to destination page
//
VOID BTISplitMoveNodes( FUCB            *pfucb,
                        SPLIT           *psplit,
                        KEYDATAFLAGS    *pkdf,
                        DIRFLAG         dirflag )
{
    CSR             *pcsrSrc        = &psplit->psplitPath->csr;
    CSR             *pcsrDest       = &psplit->csrNew;
    INT             cLineInsert     = psplit->splitoper == splitoperInsert ? 1 : 0;
    const LINEINFO  *plineinfoOper  = &psplit->rglineinfo[psplit->ilineOper];

    Assert( splittypeVertical != psplit->splittype
        || 0 == psplit->ilineSplit );
    Assert( splittypeVertical == psplit->splittype
        || 0 < psplit->ilineSplit );
    Assert( !( psplit->fSplitPageFlags & CPAGE::fPageRoot )
        || splittypeVertical == psplit->splittype );
    Assert( psplit->splitoper != splitoperNone
        || 0 == psplit->ilineOper );

    Assert( !FBTIUpdatablePage( *pcsrDest ) || pcsrDest->FDirty() );
    Assert( !FBTIUpdatablePage( *pcsrSrc ) || pcsrSrc->FDirty() );

    pcsrDest->SetILine( 0 );
    BTICheckSplitLineinfo( pfucb, psplit, *pkdf );

    //  set prefix in destination page
    //
    if ( psplit->prefixinfoNew.ilinePrefix != ilineInvalid
        && FBTIUpdatablePage( *pcsrDest ) )
    {
        const INT   ilinePrefix = psplit->prefixinfoNew.ilinePrefix + psplit->ilineSplit;

        Assert( ilinePrefix < psplit->clines );
        Assert( ilinePrefix >= psplit->ilineSplit );

        NDSetPrefix( pcsrDest, psplit->rglineinfo[ilinePrefix].kdf.key );
    }

    //  move every node from Src to Dest
    //
    if ( psplit->splitoper != splitoperNone
        && psplit->ilineOper >= psplit->ilineSplit )
    {
        //  ilineOper falls in Dest page
        //  copy lines from ilineSplit till ilineOper - 1 from Src to Dest
        //  perform oper
        //  copy remaining lines
        //  delete copied lines from Src
        //
        Assert( 0 == pcsrDest->ILine() );
        BTISplitBulkCopy( pfucb,
                          psplit,
                          psplit->ilineSplit,
                          psplit->ilineOper - psplit->ilineSplit );

        //  insert ilineOper
        //
        pcsrSrc->SetILine( psplit->ilineOper );

        if ( FBTIUpdatablePage( *pcsrDest ) )
        {
            //  if need to redo destination, must need to redo source page as well
            Assert( FBTIUpdatablePage( *pcsrSrc )
                || !FBTISplitPageBeforeImageRequired( psplit ) );
            Assert( psplit->ilineOper - psplit->ilineSplit == pcsrDest->ILine() );
            Assert( psplit->ilineOper - psplit->ilineSplit == pcsrDest->Cpage().Clines() );

            switch( psplit->splitoper )
            {
                case splitoperNone:
                    Assert( fFalse );
                    break;

                case splitoperInsert:
#ifdef DEBUG
                {
                    const INT   cbCommon = CbNDCommonPrefix( pfucb, pcsrDest, pkdf->key );
                    if ( cbCommon > cbPrefixOverhead )
                    {
                        Assert( cbCommon == plineinfoOper->cbPrefix  ) ;
                    }
                    else
                    {
                        Assert( 0 == plineinfoOper->cbPrefix );
                    }
                }
#endif
                    NDInsert( pfucb, pcsrDest, pkdf, plineinfoOper->cbPrefix );
                    if ( !( dirflag & fDIRNoVersion ) &&
                         !g_rgfmp[ pfucb->ifmp ].FVersioningOff() &&
                         ( psplit->fNewPageFlags & CPAGE::fPageLeaf ) )
                    {
                        //  UNDONE: assert version for this node exists
                        //
                        CallS( ErrNDFlagVersion( pcsrDest ) );
                    }
                    else
                    {
#ifdef  DEBUG
                        NDGet( pfucb, pcsrDest );
                        Assert( !FNDVersion( pfucb->kdfCurr ) );
#endif
                    }
                    break;

                default:
                    Assert( psplit->splitoper == splitoperFlagInsertAndReplaceData ||
                            psplit->splitoper == splitoperReplace );
                    Assert( psplit->ilineOper == pcsrSrc->ILine() );
                    Assert( pkdf->data == plineinfoOper->kdf.data );

                    NDInsert( pfucb,
                              pcsrDest,
                              &plineinfoOper->kdf,
                              plineinfoOper->cbPrefix );

                    if ( splitoperReplace != psplit->splitoper )
                    {
#ifdef DEBUG
                        NDGet( pfucb, pcsrDest );
                        Assert( FNDDeleted( pfucb->kdfCurr ) );
#endif
                        NDResetFlagDelete( pcsrDest );
                        NDGet( pfucb, pcsrDest );
                    }
                    else
                    {
#ifdef  DEBUG
                        NDGet( pfucb, pcsrDest );
                        Assert( !FNDDeleted( pfucb->kdfCurr ) );
#endif
                    }

                    if ( !( dirflag & fDIRNoVersion ) &&
                         !g_rgfmp[ pfucb->ifmp ].FVersioningOff( ) &&
                         ( psplit->fNewPageFlags & CPAGE::fPageLeaf ) )
                    {
                        //  UNDONE: assert version for this node exists
                        //
                        CallS( ErrNDFlagVersion( pcsrDest ) );
                    }
                    else
                    {
#ifdef DEBUG
                        NDGet( pfucb, pcsrDest );
                        Assert( !FNDVersion( pfucb->kdfCurr ) ||
                                FNDVersion( plineinfoOper->kdf ) );
                        Assert( FNDVersion( pfucb->kdfCurr ) ||
                                !FNDVersion( plineinfoOper->kdf ) );
#endif
                    }
            }
        }

        pcsrDest->IncrementILine();
        Assert( pagetrimTrimmed == pcsrDest->PagetrimState() ||
                pcsrDest->Cpage().Clines() == pcsrDest->ILine() ||
                latchRIW == pcsrDest->Latch() );

        BTISplitBulkCopy( pfucb,
                          psplit,
                          psplit->ilineOper + 1,
                          psplit->clines - psplit->ilineOper - 1 );

        pcsrSrc->SetILine( psplit->ilineSplit );
        BTISplitBulkDelete( pcsrSrc,
                psplit->clines - psplit->ilineSplit - cLineInsert );

        //  set prefix in source page
        //
        if ( splittypeAppend != psplit->splittype )
        {
            //  set new prefix in src page
            //  adjust nodes in src to correspond to new prefix
            //
            BTISplitSetPrefixInSrcPage( pfucb, psplit );
        }
    }
    else
    {
        //  oper node is in Src page
        //  move nodes to Dest page
        //  delete nodes that have been moved
        //  perform oper in Src page
        //
        Assert( psplit->ilineOper < psplit->ilineSplit ||
                splitoperNone == psplit->splitoper );

        pcsrSrc->SetILine( psplit->ilineSplit - cLineInsert );
        Assert( 0 == pcsrDest->ILine() );

        BTISplitBulkCopy( pfucb,
                          psplit,
                          psplit->ilineSplit,
                          psplit->clines - psplit->ilineSplit );

        Assert( psplit->ilineSplit - cLineInsert == pcsrSrc->ILine() );
        BTISplitBulkDelete( pcsrSrc,
                            psplit->clines - psplit->ilineSplit );

        //  set prefix
        //
        Assert( splittypeAppend != psplit->splittype );
        BTISplitSetPrefixInSrcPage( pfucb, psplit );

        //  can't use rglineinfo[].kdf anymore
        //  since page may have been reorged
        //

        pcsrSrc->SetILine( psplit->ilineOper );

        if ( FBTIUpdatablePage( *pcsrSrc ) )
        {
            switch ( psplit->splitoper )
            {
                case splitoperNone:
                    break;

                case splitoperInsert:
                    NDInsert( pfucb, pcsrSrc, pkdf, plineinfoOper->cbPrefix );
                    if ( !( dirflag & fDIRNoVersion ) &&
                         !g_rgfmp[ pfucb->ifmp ].FVersioningOff( ) &&
                         ( psplit->fNewPageFlags & CPAGE::fPageLeaf ) )
                    {
                        //  UNDONE: assert version for this node exists
                        //
                        CallS( ErrNDFlagVersion( pcsrSrc ) );
                    }
                    else
                    {
#ifdef DEBUG
                        NDGet( pfucb, pcsrSrc );
                        Assert( !FNDVersion( pfucb->kdfCurr ) );
#endif
                    }

                    break;

                default:
                    //  replace data
                    //  by deleting and re-inserting [to avoid page reorg problems]
                    //
                    Assert( psplit->splitoper == splitoperFlagInsertAndReplaceData ||
                            psplit->splitoper == splitoperReplace );

#ifdef DEBUG
                    //  assert that key of node is in pfucb->bmCurr
                    //
                    NDGet( pfucb, pcsrSrc );
                    Assert( FFUCBUnique( pfucb ) );
                    Assert( FKeysEqual( pfucb->bmCurr.key, pfucb->kdfCurr.key ) );
#endif

                    NDDelete( pcsrSrc );

                    Assert( !pkdf->data.FNull() );
                    Assert( pkdf->data.Cb() > pfucb->kdfCurr.data.Cb() );
                    Assert( pcsrSrc->ILine() == psplit->ilineOper );

                    KEYDATAFLAGS    kdfInsert;
                    kdfInsert.data      = pkdf->data;
                    kdfInsert.key       = pfucb->bmCurr.key;
                    kdfInsert.fFlags    = 0;

                    NDInsert( pfucb, pcsrSrc, &kdfInsert, plineinfoOper->cbPrefix );
                    if ( !( dirflag & fDIRNoVersion ) &&
                         !g_rgfmp[ pfucb->ifmp ].FVersioningOff( ) &&
                         ( psplit->fNewPageFlags & CPAGE::fPageLeaf ) )
                    {
                        //  UNDONE: assert version for this node exists
                        //
                        CallS( ErrNDFlagVersion( pcsrSrc ) );
                    }
                    else
                    {
#ifdef DEBUG
                        NDGet( pfucb, pcsrDest );
                        Assert( !FNDVersion( pfucb->kdfCurr ) );
#endif
                    }

                    break;
            }
        }
        else
        {
            //  if we didn't need to redo the source page, we shouldn't need to redo the
            //  destination page
            Assert( !FBTIUpdatablePage( *pcsrDest ) );
        }
    }

    if ( psplit->fNewPageFlags & CPAGE::fPageLeaf )
    {
        //  set cbUncommittedFreed in src and dest pages
        //
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering()
            || ( pcsrDest->Cpage().FLeafPage() && latchWrite == pcsrDest->Latch() ) );
        if ( FBTIUpdatablePage( *pcsrDest ) )
        {
            //  if need to redo destination, must need to redo source page as well
            Assert( FBTIUpdatablePage( *pcsrSrc )
                || !FBTISplitPageBeforeImageRequired( psplit ) );
            pcsrDest->Cpage().SetCbUncommittedFree( psplit->cbUncFreeDest );
            if ( pcsrDest->Cpage().FSpaceTree() )
            {
                Assert( 0 == psplit->cbUncFreeDest );
            }
            else
            {
                Assert( CbNDUncommittedFree( pfucb, pcsrDest ) <= psplit->cbUncFreeDest );
            }
        }

        if ( FBTIUpdatablePage( *pcsrSrc ) )
        {
            pcsrSrc->Cpage().SetCbUncommittedFree( psplit->cbUncFreeSrc );
            if ( pcsrSrc->Cpage().FSpaceTree() )
            {
                Assert( 0 == psplit->cbUncFreeSrc );
            }
            else
            {
                Assert( CbNDUncommittedFree( pfucb, pcsrSrc ) <= psplit->cbUncFreeSrc );
            }
        }
        else
        {
            //  if we didn't need to redo the source page, we shouldn't need to redo the
            //  destination page if not an append
            Assert( !FBTIUpdatablePage( *pcsrDest )
                || !FBTISplitPageBeforeImageRequired( psplit ) );
        }


        //  move UndoInfo of moved nodes to destination page
        //
        Assert( ( splittypeVertical == psplit->splittype && psplit->kdfParent.key.FNull() )
            || ( splittypeVertical != psplit->splittype && !psplit->kdfParent.key.FNull() ) );
        if ( !FBTIUpdatablePage( *pcsrSrc ) )
        {
            //  if we didn't need to redo the source page, we shouldn't need to redo the
            //  destination page if not an append
            Assert( !FBTIUpdatablePage( *pcsrDest )
                || !FBTISplitPageBeforeImageRequired( psplit ) );
        }
    }
    else
    {
        Assert( 0 == psplit->cbUncFreeSrc );
        Assert( 0 == psplit->cbUncFreeDest );

#ifdef DEBUG
        if ( !PinstFromIfmp( pfucb->ifmp )->FRecovering() )
        {
            Assert( !pcsrDest->Cpage().FLeafPage() );
            Assert( pcsrSrc->Cpage().CbUncommittedFree() == 0 );
            Assert( pcsrDest->Cpage().CbUncommittedFree() == 0 );
        }
#endif
    }

    return;
}


INLINE VOID BTISplitBulkDelete( CSR * pcsr, INT clines )
{
    if ( !FBTIUpdatablePage( *pcsr ) )
    {
        //  page does not need redo
        //
        return;
    }
    Assert( latchWrite == pcsr->Latch() );

    NDBulkDelete( pcsr, clines );
}


//  copy clines starting from rglineInfo[ilineStart] to csrDest
//  prefixes have been calculated in rglineInfo
//
VOID BTISplitBulkCopy( FUCB *pfucb, SPLIT *psplit, INT ilineStart, INT clines )
{
    INT         iline;
    const INT   ilineEnd    = ilineStart + clines;
    CSR         *pcsrDest   = &psplit->csrNew;

    if ( !FBTIUpdatablePage( *pcsrDest ) )
    {
        //  page does not need redo
        //
        return;
    }
    Assert( latchWrite == pcsrDest->Latch() );

    Assert( ilineEnd <= psplit->clines );
    for ( iline = ilineStart; iline < ilineEnd; iline++ )
    {
        Assert( iline != psplit->ilineOper ||
                splitoperNone == psplit->splitoper );
        Assert( pcsrDest->Cpage().Clines() == pcsrDest->ILine() );

        const LINEINFO  * plineinfo = &psplit->rglineinfo[iline];

#ifdef DEBUG
        const INT       cbCommon    = CbNDCommonPrefix( pfucb, pcsrDest, plineinfo->kdf.key );
        if ( cbCommon > cbPrefixOverhead )
        {
            Assert( cbCommon == plineinfo->cbPrefix  ) ;
        }
        else
        {
            Assert( 0 == plineinfo->cbPrefix );
        }
#endif

        NDInsert( pfucb, pcsrDest, &plineinfo->kdf, plineinfo->cbPrefix );

        pcsrDest->IncrementILine();
    }
}


//  returns reference to rglineInfo corresponding to iline
//
INLINE const LINEINFO *PlineinfoFromIline( SPLIT *psplit, INT iline )
{
    Assert( iline >= 0);
    Assert( iline < psplit->clines );
    if ( iline < 0 || iline >= psplit->clines )
    {
        return NULL;
    }

    if ( psplit->splitoper != splitoperInsert ||
         iline < psplit->ilineOper )
    {
        return &psplit->rglineinfo[iline];
    }

    Assert( psplit->splitoper == splitoperInsert );
    Assert( iline >= psplit->ilineOper );
    Assert( iline + 1 < psplit->clines );

    return &psplit->rglineinfo[iline+1];
}

//  set prefix in page to psplit->prefix and reorg nodes
//
VOID BTISplitSetPrefixInSrcPage( FUCB *pfucb, SPLIT *psplit )
{
    Assert( psplit->splittype != splittypeAppend );
    Assert( !psplit->prefixSplitNew.FNull()
        || psplit->prefixinfoSplit.ilinePrefix == ilineInvalid );

    CSR         *pcsr = &psplit->psplitPath->csr;

    if ( !FBTIUpdatablePage( *pcsr ) )
    {
        //  page does not need redo
        //
        return;
    }
    Assert( latchWrite == pcsr->Latch() );

    const DATA  *pprefixOld = &psplit->prefixSplitOld;
    const DATA  *pprefixNew = &psplit->prefixSplitNew;
    Assert( psplit->prefixinfoSplit.ilinePrefix != ilineInvalid ||
            pprefixNew->FNull() );
    Assert( psplit->prefixinfoSplit.ilinePrefix == ilineInvalid ||
            !pprefixNew->FNull() );

    INT         iline;
    const INT   clines = pcsr->Cpage().Clines();

    //  delete old prefix
    //
    if ( !pprefixOld->FNull() )
    {
        KEY keyNull;

        keyNull.Nullify();
        NDSetPrefix( pcsr, keyNull );
    }
    else
    {
#ifdef DEBUG
        //  check prefix was null before
        //
        NDGetPrefix( pfucb, pcsr );
        Assert( pfucb->kdfCurr.key.FNull() );
#endif
    }

    //  fix nodes that shrink because of prefix change
    //
    for ( iline = 0; iline < clines; iline++ )
    {
        pcsr->SetILine( iline );
        NDGet( pfucb, pcsr );

        const LINEINFO  *plineinfo = PlineinfoFromIline( psplit, iline );
        Assert( plineinfo->cbPrefix == 0 ||
                plineinfo->cbPrefix > cbPrefixOverhead );

        if ( plineinfo->cbPrefix > pfucb->kdfCurr.key.prefix.Cb() )
        {

            NDGrowCbPrefix( pfucb, pcsr, plineinfo->cbPrefix );

#ifdef DEBUG
            NDGet( pfucb, pcsr );
            Assert( pfucb->kdfCurr.key.prefix.Cb() == plineinfo->cbPrefix );
#endif
        }
    }

    //  fix nodes that grow because of prefix change
    //
    for ( iline = 0; iline < clines; iline++ )
    {
        pcsr->SetILine( iline );
        NDGet( pfucb, pcsr );

        const LINEINFO  *plineinfo = PlineinfoFromIline( psplit, iline );
        Assert( plineinfo->cbPrefix == 0 ||
                plineinfo->cbPrefix > cbPrefixOverhead );

        if ( plineinfo->cbPrefix < pfucb->kdfCurr.key.prefix.Cb() )
        {
            NDShrinkCbPrefix( pfucb, pcsr, pprefixOld, plineinfo->cbPrefix );

#ifdef DEBUG
            NDGet( pfucb, pcsr );
            Assert( pfucb->kdfCurr.key.prefix.Cb() == plineinfo->cbPrefix );
#endif
        }
    }

    //  set new prefix
    //
    KEY     keyPrefixNew;

    keyPrefixNew.Nullify();
    keyPrefixNew.prefix = *pprefixNew;
    NDSetPrefix( pcsr, keyPrefixNew );

    return;
}

//  optimizes internal fragmentation for whole 
//  chain of splitpath's from leaf to root
//
VOID BTISplitShrinkPages( SPLITPATH *psplitPathLeaf )
{
    SPLITPATH *psplitPath = psplitPathLeaf;

    for ( ; psplitPath != NULL; )
    {
        //  attempt to optimize internal frag if nesc and we can piggy back on 
        //  an existing DBTIME update
        //
        if( psplitPath->csr.Latch() == latchWrite )
        {
            AssertRTL( psplitPath->dbtimeBefore != psplitPath->csr.Cpage().Dbtime() );
            if ( psplitPath->dbtimeBefore != psplitPath->csr.Cpage().Dbtime() )
            {
                psplitPath->csr.Cpage().OptimizeInternalFragmentation();
            }
        }

        //  navigate to the parent
        //
        psplitPath = psplitPath->psplitPathParent;
    }
}


//  releases whole chain of splitpath's
//  from leaf to root
//
VOID BTIReleaseSplitPaths( INST *pinst, SPLITPATH *psplitPathLeaf )
{
    SPLITPATH *psplitPath = psplitPathLeaf;

    for ( ; psplitPath != NULL; )
    {
        //  save parent
        //
        SPLITPATH *psplitPathParent = psplitPath->psplitPathParent;

        delete psplitPath;
        psplitPath = psplitPathParent;
    }
}


//  checks to make sure there are no erroneous splits
//  if there is a operNone split at any level,
//      there should be no splits at lower levels
//
LOCAL VOID BTISplitCheckPath( SPLITPATH *psplitPathLeaf )
{
#ifdef DEBUG
    SPLITPATH   *psplitPath;
    BOOL        fOperNone = fFalse;

    //  goto root
    //
    for ( psplitPath = psplitPathLeaf;
          psplitPath->psplitPathParent != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
    }

    Assert( NULL == psplitPath->psplit ||
            splittypeVertical == psplitPath->psplit->splittype );

    for ( ; psplitPath != NULL; psplitPath = psplitPath->psplitPathChild )
    {
        SPLIT   *psplit = psplitPath->psplit;

        if ( fOperNone )
        {
            //  higher level has a split with no oper
            //
            Assert( NULL == psplit );
        }
        else
        {
            if ( psplit != NULL && splitoperNone == psplit->splitoper )
            {
                fOperNone = fTrue;
            }
        }
    }
#endif
}


//  checks lineinfo in split point to the right nodes
//
VOID BTICheckSplitLineinfo( FUCB *pfucb, SPLIT *psplit, const KEYDATAFLAGS& kdf )
{
#ifdef DEBUG
    if ( PinstFromIfmp( pfucb->ifmp )->FRecovering() && psplit->psplitPath->csr.Latch() != latchWrite )
    {
        return;
    }

    INT             iline;
    CSR             *pcsr       = &psplit->psplitPath->csr;
    const INT       clines      = pcsr->Cpage().Clines();
    const SPLITOPER splitoper   = psplit->splitoper;

    for ( iline = 0; iline < clines; iline++ )
    {
        const LINEINFO  *plineinfo = PlineinfoFromIline( psplit, iline );

        pcsr->SetILine( iline );
        NDGet( pfucb, pcsr );

        if ( splitoper == splitoperInsert ||
             iline != psplit->ilineOper )
        {
            Assert( FKeysEqual( plineinfo->kdf.key, pfucb->kdfCurr.key ) );
            Assert( FDataEqual( plineinfo->kdf.data, pfucb->kdfCurr.data ) );
///         Assert( plineinfo->kdf.fFlags == pfucb->kdfCurr.fFlags );
        }
        else if ( splitoper == splitoperNone )
        {
        }
        else
        {
            Assert( iline == psplit->ilineOper );
            if ( splitoper != splitoperReplace )
            {
                Assert( FKeysEqual( plineinfo->kdf.key, kdf.key ) );
            }
            else
            {
                Assert( kdf.key.FNull() );
            }

            Assert( FKeysEqual( pfucb->kdfCurr.key, plineinfo->kdf.key ) );
            Assert( FDataEqual( plineinfo->kdf.data, kdf.data ) );
        }
    }

    //  check ilineOper
    //
    LINEINFO    *plineinfo = &psplit->rglineinfo[psplit->ilineOper];
    if ( splitoperInsert == psplit->splitoper )
    {
        Assert( plineinfo->kdf == kdf );
    }
#endif
}


//  check if a split just performed is correct
//
VOID BTICheckSplits( FUCB           *pfucb,
                    SPLITPATH       *psplitPathLeaf,
                    KEYDATAFLAGS    *pkdf,
                    DIRFLAG         dirflag )
{
#ifdef DEBUG
    SPLITPATH   *psplitPath;
    for ( psplitPath = psplitPathLeaf;
          psplitPath != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
        BTICheckOneSplit( pfucb, psplitPath, pkdf, dirflag );
    }
#endif
}


LOCAL VOID BTICheckSplitFlags( const SPLIT *psplit )
{
#ifdef DEBUG
    const SPLITPATH *psplitPath = psplit->psplitPath;

    if ( psplit->splittype == splittypeVertical )
    {
        Assert( psplit->fSplitPageFlags & CPAGE::fPageRoot );
        Assert( !( psplit->fSplitPageFlags & CPAGE::fPageLeaf ) );

        if ( psplitPath->csr.Cpage().FLeafPage() )
        {
            Assert( ( psplit->fNewPageFlags & CPAGE::fPageLeaf ) );
            Assert( g_fRepair || ( psplit->fSplitPageFlags & CPAGE::fPageParentOfLeaf ) );
        }
        else
        {
            Assert( !( psplit->fSplitPageFlags & CPAGE::fPageParentOfLeaf ) );
        }
    }
    else
    {
        Assert( psplit->fNewPageFlags == psplit->fSplitPageFlags );
        Assert( psplitPath->csr.Cpage().FFlags() == psplit->fNewPageFlags );
        Assert( !( psplit->fSplitPageFlags & CPAGE::fPageRoot ) );
    }

    Assert( !( psplit->fNewPageFlags & CPAGE::fPageRoot ) );
    Assert( !( psplit->fNewPageFlags & CPAGE::fPageEmpty ) );
    Assert( !( psplit->fSplitPageFlags & CPAGE::fPageEmpty ) );
    Assert( !( psplit->fNewPageFlags & CPAGE::fPagePreInit ) );
    Assert( !( psplit->fSplitPageFlags & CPAGE::fPagePreInit ) );
#endif
}


//  check split at one level
//
VOID BTICheckOneSplit( FUCB             *pfucb,
                       SPLITPATH        *psplitPath,
                       KEYDATAFLAGS     *pkdf,
                       DIRFLAG          dirflag )
{
#ifdef DEBUG
    SPLIT           *psplit = psplitPath->psplit;
    const DBTIME    dbtime  = psplitPath->csr.Dbtime();

//  UNDONE: check lgpos of all pages is the same
//  const LGPOS     lgpos;

    //  check that nodes in every page are in order
    //  this will be done by node at NDGet
    //

    //  check that key at parent > all sons
    //
    if ( psplit == NULL )
    {
        return;
    }

    Assert( psplit->csrNew.Pgno() == psplit->pgnoNew );
    switch ( psplit->splittype )
    {
        case splittypeVertical:
        {
            CSR     *pcsrRoot = &psplitPath->csr;

            //  parent page has only one node
            //
            Assert( pcsrRoot->Cpage().FRootPage() );
            Assert( !pcsrRoot->Cpage().FLeafPage() );
            Assert( 1 == pcsrRoot->Cpage().Clines() );
            Assert( pcsrRoot->Dbtime() == dbtime );
            Assert( psplit->csrNew.Dbtime() == dbtime );

            NDGet( pfucb, pcsrRoot );
            Assert( pfucb->kdfCurr.key.FNull() );
            Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
            Assert( psplit->pgnoNew == *( (UnalignedLittleEndian< PGNO > *)pfucb->kdfCurr.data.Pv() ) );
        }
            break;

        case splittypeAppend:
            //  assert no node is moved
            //
            Assert( psplit->csrNew.Cpage().Clines() == 1 );
            Assert( psplit->csrRight.Pgno() == pgnoNull );
            Assert( psplit->csrNew.Cpage().PgnoNext() == pgnoNull );

        case splittypeRight:
            CSR             *pcsrParent     = &psplitPath->psplitPathParent->csr;
            CSR             *pcsrSplit      = &psplitPath->csr;
            CSR             *pcsrNew        = &psplit->csrNew;
            CSR             *pcsrRight      = &psplit->csrRight;

            KEYDATAFLAGS    kdfLess, kdfGreater;

            //  if parent is undergoing a vertical split, new page is parent
            //
            if ( psplitPath->psplitPathParent->psplit != NULL &&
                 splittypeVertical == psplitPath->psplitPathParent->psplit->splittype )
            {
                pcsrParent = &psplitPath->psplitPathParent->psplit->csrNew;
            }

            Assert( pcsrSplit->Dbtime() == dbtime );
            Assert( pcsrNew->Dbtime() == dbtime );
            Assert( pcsrParent->Dbtime() == dbtime );

            //  check split, new and right pages are in order
            //
            NDMoveLastSon( pfucb, pcsrSplit );
            kdfLess = pfucb->kdfCurr;

            NDMoveFirstSon( pfucb, pcsrNew );
            kdfGreater = pfucb->kdfCurr;

            if ( pcsrNew->Cpage().FLeafPage() )
            {
                Assert( CmpKeyData( kdfLess, kdfGreater ) < 0 || g_fRepair );
            }
            else
            {
                Assert( sizeof( PGNO ) == kdfGreater.data.Cb() );
                Assert( sizeof( PGNO ) == kdfLess.data.Cb() );
                Assert( !kdfLess.key.FNull() );
                Assert( kdfGreater.key.FNull() ||
                        CmpKey( kdfLess.key, kdfGreater.key ) < 0 );
            }

            if ( pcsrRight->Pgno() != pgnoNull )
            {
                Assert( pcsrRight->Dbtime() == dbtime );
                Assert( pcsrRight->Cpage().FLeafPage() );
                NDMoveLastSon( pfucb, pcsrNew );
                kdfLess = pfucb->kdfCurr;

                NDMoveFirstSon( pfucb, pcsrRight );
                kdfGreater = pfucb->kdfCurr;

                Assert( CmpKeyData( kdfLess, kdfGreater ) < 0 );

                //  right page should also be >= parent of new page
                //

            }

            //  check parent pointer key > all nodes in child page
            //
            NDMoveFirstSon( pfucb, pcsrParent );

            Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
            PGNO    pgnoChild = *( (UnalignedLittleEndian< PGNO > *)pfucb->kdfCurr.data.Pv() );
            for ( ; pgnoChild != psplit->pgnoSplit ; )
            {

                Assert( pgnoChild != psplit->pgnoNew );
                Assert( pgnoChild != psplit->csrRight.Pgno() );

                //  get next page-pointer node
                //
                if ( pcsrParent->ILine() + 1 == pcsrParent->Cpage().Clines() )
                {
                    Assert( psplitPath->psplitPathParent->psplit != NULL );
                    pcsrParent = &psplitPath->psplitPathParent->psplit->csrNew;
                    NDMoveFirstSon( pfucb, pcsrParent );
                }
                else
                {
                    pcsrParent->IncrementILine();
                }

                Assert( pcsrParent->ILine() < pcsrParent->Cpage().Clines() );
                NDGet( pfucb, pcsrParent );

                Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
                pgnoChild = *( (UnalignedLittleEndian< PGNO > *)pfucb->kdfCurr.data.Pv() );
            }

            Assert( pgnoChild == psplit->pgnoSplit );
            kdfGreater = pfucb->kdfCurr;

            NDMoveLastSon( pfucb, pcsrSplit );
            if ( pcsrSplit->Cpage().FLeafPage() && !FFUCBRepair( pfucb ) )
            {
                if ( !FFUCBUnique( pfucb ) )
                {
                    Assert( kdfGreater.key.FNull() ||
                            CmpKeyWithKeyData( kdfGreater.key, pfucb->kdfCurr ) > 0 );
                }
                else
                {
                    Assert( kdfGreater.key.FNull() ||
                            CmpKey( kdfGreater.key, pfucb->kdfCurr.key ) > 0 );
                }
            }
            else
            {
                //  no suffix compression at internal levels
                //
                Assert( FKeysEqual( kdfGreater.key, pfucb->kdfCurr.key ) );
            }

            //  next page pointer should point to new page
            //
            if ( pcsrParent->ILine() + 1 == pcsrParent->Cpage().Clines() )
            {
                Assert( psplitPath->psplitPathParent->psplit != NULL );
                pcsrParent = &psplitPath->psplitPathParent->psplit->csrNew;
                NDMoveFirstSon( pfucb, pcsrParent );
            }
            else
            {
                pcsrParent->IncrementILine();
                NDGet( pfucb, pcsrParent );
            }

            Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
            Assert( psplit->pgnoNew == *( (UnalignedLittleEndian< PGNO > *)pfucb->kdfCurr.data.Pv() ) );

            //  key at this node should be > last node in pgnoNew
            //
            kdfGreater = pfucb->kdfCurr;

            NDMoveLastSon( pfucb, pcsrNew );
            if ( pcsrNew->Cpage().FLeafPage() )
            {
                if ( !FFUCBUnique( pfucb ) )
                {
                    Assert( kdfGreater.key.FNull() ||
                            CmpKeyWithKeyData( kdfGreater.key, pfucb->kdfCurr ) > 0 );
                }
                else
                {
                    Assert( kdfGreater.key.FNull() ||
                            CmpKey( kdfGreater.key, pfucb->kdfCurr.key ) > 0 );
                }
            }
            else
            {
                //  no suffix compression at internal levels
                //
                Assert( FKeysEqual( kdfGreater.key, pfucb->kdfCurr.key ) );
            }

            //  key at this node should be < first node in right page, if any
            //
            if ( pcsrRight->Pgno() != pgnoNull )
            {
                Assert( pcsrRight->Cpage().FLeafPage() );

                kdfLess = kdfGreater;
                NDMoveFirstSon( pfucb, pcsrRight );

                Assert( !kdfLess.key.FNull() );
                Assert( CmpKeyWithKeyData( kdfLess.key, pfucb->kdfCurr ) <= 0 );
            }
    }
#endif  //  DEBUG
}


//  creates a new MERGEPATH structure and initializes it
//  adds newly created mergePath structure to head of list
//  pointed to by *ppMergePath passed in
//
ERR ErrBTINewMergePath( MERGEPATH **ppmergePath )
{
    MERGEPATH   *pmergePath;

    if ( !( pmergePath = new MERGEPATH ) )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    pmergePath->pmergePathParent = *ppmergePath;
    if ( pmergePath->pmergePathParent != NULL )
    {
        Assert( NULL == pmergePath->pmergePathParent->pmergePathChild );
        pmergePath->pmergePathParent->pmergePathChild = pmergePath;
    }

    *ppmergePath = pmergePath;
    return JET_errSuccess;
}


//  seeks to node for single page cleanup
//  returns error if node is not found
//
INLINE ERR ErrBTISPCSeek( FUCB *pfucb, const BOOKMARK& bm )
{
    ERR     err;

    //  no page should be latched
    //
    Assert( !Pcsr( pfucb )->FLatched( ) );

    //  go to root
    //
    Call( ErrBTIGotoRoot( pfucb, latchReadNoTouch ) );
    Assert( 0 == Pcsr( pfucb )->ILine() );

    if ( Pcsr( pfucb )->Cpage().Clines() == 0 )
    {
        //  page is empty
        //
        Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
        err = wrnNDFoundGreater;
    }
    else
    {
        //  seek down tree for bm
        //
        for ( ; ; )
        {
            Call( ErrNDSeek( pfucb, bm ) );

            if ( !Pcsr( pfucb )->Cpage().FInvisibleSons( ) )
            {
                //  leaf node reached, exit loop
                //
                break;
            }
            else
            {
                //  get pgno of child from node
                //  switch to that page
                //
                Assert( pfucb->kdfCurr.data.Cb() == sizeof( PGNO ) );
                Call( Pcsr( pfucb )->ErrSwitchPage(
                            pfucb->ppib,
                            pfucb->ifmp,
                            *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv(),
                            pfucb->u.pfcb->FNoCache() ) );
                Assert( Pcsr( pfucb )->Cpage().Clines() > 0 );
            }
        }
    }

    if ( wrnNDFoundGreater == err ||
         wrnNDFoundLess == err )
    {
        Error( ErrERRCheck( JET_errNoCurrentRecord ) );
    }
    else if ( !FNDDeleted( pfucb->kdfCurr ) )
    {
        Error( ErrERRCheck( JET_errRecordNotDeleted ) );
    }

HandleError:
    if ( err < 0 )
    {
        BTUp( pfucb );
    }

    return err;
}

//  deletes all nodes in page that are flagged-deleted
//      and have no active version
//  also nullifies versions on deleted nodes
//  WARNING: May re-organize the page
//
LOCAL ERR ErrBTISPCDeleteNodes( FUCB *pfucb, CSR *pcsr )
{
    ERR         err = JET_errSuccess;
    const INT   clines = pcsr->Cpage().Clines();
    BOOL        fUpdated = fFalse;

    Assert( latchWrite == pcsr->Latch() );
    Assert( pcsr->Cpage().Dbtime() == pcsr->Dbtime() );

    DBTIME dbtimePre = pcsr->Cpage().Dbtime();

    Assert( clines > 0 );
    for ( INT iline = clines - 1; iline >= 0 ; iline-- )
    {
        KEYDATAFLAGS kdf;
        NDIGetKeydataflags( pcsr->Cpage(), iline, &kdf );

        BOOKMARK    bm;
        NDGetBookmarkFromKDF( pfucb, kdf, &bm );

        if ( FNDDeleted( kdf )
            && !FVERActive( pfucb, bm ) )
        {
            pcsr->SetILine( iline );
            if ( pcsr->Cpage().Clines() > 1 )
            {
                Call( ErrNDDelete( pfucb, pcsr, fDIRNull ) );

                // WARNING: The assert below is wrong, because by this point, there may actually
                // be future active versions.  This is because versioning is now done before we
                // latch the page.
                // Assert( !FVERActive( pfucb, bm ) );
                VERNullifyInactiveVersionsOnBM( pfucb, bm );

                fUpdated = fTrue;
            }
            // Doing this for LV roots will cause problems for some parts of the code
            // (we've seen backup scrubbing complaining) because, even though the node is
            // deleted and not visible, it is expected to carry metadata about the LV
            // See ErrSCRUBIZeroLV().
            //
            else if ( FFUCBUnique( pfucb ) && !( pcsr->Cpage().FLongValuePage() && FIsLVRootKey( kdf.key ) ) )
            {
                // ErrNDReplace needs the bmCurr in the FUCB to be set properly. Here we will set it to
                // the node being replaced and then reset it after the replace
                BOOKMARK bmSaved = pfucb->bmCurr;
                pfucb->bmCurr = bm;
                NDGet( pfucb, pcsr );

                // We can't remove the last node on the page (b-tree pages can't be empty).
                // In this case we will have to keep the key and replace the data with null
                // This is only done for unique indexes.
                //
                // The record isn't visible to anyone so the replace isn't versioned
                //
                // Technically, we're scrubbing one byte with chSCRUBDBMaintEmptyPageLastNodeFill
                // below, which we should be skipping if JET_paramZeroDatabaseUnusedSpace is not set,
                // but this pattern is actually used by DBM to detect cases where we've already performed
                // single-node cleanup on an empty tree, so we'll keep the single-byte scrubbing below
                // always active. Note that the scrubbing of the rest of the node being replaced below
                // will honor the parameter and not get scrubbed if it is not set.
                //
                BYTE bNull = chSCRUBDBMaintEmptyPageLastNodeFill;
                DATA data;
                data.SetPv(&bNull);
                data.SetCb(sizeof(bNull));
                err = ErrNDReplace( pfucb, pcsr, &data, fDIRNoVersion, rceidNull, NULL );
                pfucb->bmCurr = bmSaved;
                Call( err );

                fUpdated = fTrue;
            }
        }
    }

    // Note that the following is not crash consistent since this whole
    // operation is not not undoable and not in a macro
    // Consider skipping it (at least for DEBUG builds) to catch any other
    // issues with uneeded prefixes left behind (cannot do that currently since
    // the test to check for the Replace bug with uneeded prefix depends on this
    Call( ErrBTIResetPrefixIfUnused( pfucb, pcsr ) );

    //  since the OptimizeInternalFragmentation() call piggy-backs on the DBTIME changes of
    //  others, we can only do it if we updated something (and thus the DBTIME).

    if ( fUpdated )
    {
        //  shrink / optimize page image
        //
        AssertRTL( dbtimePre != pcsr->Cpage().Dbtime() );
        if( dbtimePre != pcsr->Cpage().Dbtime() )
        {
            pcsr->Cpage().OptimizeInternalFragmentation();
        }
    }

HandleError:
    return err;
}


LOCAL ERR ErrBTIMergeEmptyTree(
    FUCB        * const pfucb,
    MERGEPATH   * const pmergePathLeaf )
{
    ERR         err                 = JET_errSuccess;
    MERGEPATH   * pmergePath;
    ULONG       cPagesToFree        = 0;
    EMPTYPAGE   rgemptypage[cBTMaxDepth];
    LGPOS       lgpos;

    //  go to root
    //  since we need to latch top-down
    for ( pmergePath = pmergePathLeaf;
          pmergePath->pmergePathParent != NULL;
          pmergePath = pmergePath->pmergePathParent )
    {
        Assert( latchRIW == pmergePath->csr.Latch() );
    }

    MERGEPATH   * const pmergePathRoot  = pmergePath;
    CSR         * const pcsrRoot        = &pmergePathRoot->csr;

    Assert( pcsrRoot->Cpage().FRootPage() );
    Assert( !pcsrRoot->Cpage().FLeafPage() );
    Assert( 1 == pcsrRoot->Cpage().Clines() );
    pcsrRoot->UpgradeFromRIWLatch();

    //  latch and dirty all pages
    Assert( NULL != pmergePathRoot->pmergePathChild );
    for ( pmergePath = pmergePathRoot->pmergePathChild;
        NULL != pmergePath;
        pmergePath = pmergePath->pmergePathChild )
    {
        pmergePath->csr.UpgradeFromRIWLatch();

        rgemptypage[cPagesToFree].dbtimeBefore = pmergePath->csr.Dbtime();
        rgemptypage[cPagesToFree].pgno = pmergePath->csr.Pgno();
        rgemptypage[cPagesToFree].ulFlags = pmergePath->csr.Cpage().FFlags();

        cPagesToFree++;
        Assert( cPagesToFree <= cBTMaxDepth );
    }
    Assert( cPagesToFree > 0 );

    err = ErrLGEmptyTree(
                pfucb,
                pcsrRoot,
                rgemptypage,
                cPagesToFree,
                &lgpos );

    if ( JET_errSuccess <= err )
    {
        NDSetEmptyTree( pcsrRoot );

        //  update lgpos
        pcsrRoot->Cpage().SetLgposModify( lgpos );

        //  update all child pages with dbtime of root, mark them as empty, and update lgpos
        const DBTIME    dbtime      = pcsrRoot->Dbtime();
        for ( pmergePath = pmergePathRoot->pmergePathChild;
            NULL != pmergePath;
            pmergePath = pmergePath->pmergePathChild )
        {
            pmergePath->csr.CoordinatedDirty( dbtime );
            NDEmptyPage( &(pmergePath->csr) );
            pmergePath->csr.Cpage().SetLgposModify( lgpos );
        }
    }

    //  shrink / optimize page image
    //
    BTIMergeShrinkPages( pmergePathLeaf );

    BTIReleaseMergePaths( pmergePathLeaf );
    CallR( err );

    //  WARNING: If we crash after this point, we will lose space

    const BOOL      fAvailExt   = FFUCBAvailExt( pfucb );
    const BOOL      fOwnExt     = FFUCBOwnExt( pfucb );

    //  fake out cursor to make it think it's not a space cursor
    if ( fAvailExt )
    {
        Assert( !fOwnExt );
        FUCBResetAvailExt( pfucb );
    }
    else if ( fOwnExt )
    {
        FUCBResetOwnExt( pfucb );
    }
    Assert( !FFUCBSpace( pfucb ) );

    //  return freed pages to AvailExt
    BTUp( pfucb );
    for ( ULONG i = 0; i < cPagesToFree; i++ )
    {
        //  UNDONE: track lost space because of inability
        //          to split availExt tree with the released space
        Assert( PgnoRoot( pfucb ) != rgemptypage[i].pgno );
        const ERR   errFreeExt  = ErrSPFreeExt( pfucb, rgemptypage[i].pgno, 1, "MergeEmptyTree" );
#ifdef DEBUG
        if ( !FSPExpectedError( errFreeExt ) )
        {
            CallS( errFreeExt );
        }
#endif

        //  count pages released by an update
        //
        Ptls()->threadstats.cPageUpdateReleased++;

        CallR( errFreeExt );
    }

    Assert( !FFUCBSpace( pfucb ) );
    if ( fAvailExt )
    {
        FUCBSetAvailExt( pfucb );
    }
    else if ( fOwnExt )
    {
        FUCBSetOwnExt( pfucb );
    }

    return JET_errSuccess;
}



//  performs multipage cleanup
//      seeks down to node
//      performs empty page or merge operation if possible
//      else expunges all deletable nodes in page
//

ERR ErrBTIMultipageCleanup(
    FUCB                    * const pfucb,
    const BOOKMARK&                 bm,
    BOOKMARK                * const pbmNext,
    RECCHECK                * const preccheck,
    MERGETYPE               * const pmergetype,
    const BOOL                      fRightMerges,
    __inout_opt PrereadInfo * const pPrereadInfo )
{
    ERR             err;
    MERGEPATH       *pmergePath     = NULL;
    PIBTraceContextScope tcScope       = TcBTICreateCtxScope( pfucb, iorsBTMerge );

    if ( pmergetype )
    {
        *pmergetype = mergetypeNone;
    }

    if ( pfucb->u.pfcb->FDeletePending() )
    {
        //  btree is scheduled for deletion - don't bother attempting cleanup
        //
        if ( NULL != pbmNext )
        {
            pbmNext->key.suffix.SetCb( 0 );
            pbmNext->data.SetCb( 0 );
        }

        return JET_errSuccess;
    }

    //  get path RIW latched
    //
    Call( ErrBTICreateMergePath( pfucb, bm, pgnoNull, fTrue, &pmergePath, pPrereadInfo ) );
    if ( wrnBTShallowTree == err )
    {
        if ( NULL != pbmNext )
        {
            pbmNext->key.suffix.SetCb( 0 );
            pbmNext->data.SetCb( 0 );
        }
        goto HandleError;
    }

    //  check if merge conditions hold
    //
    Call( ErrBTISelectMerge( pfucb, pmergePath, bm, pbmNext, preccheck, fRightMerges ) );
    Assert( pmergePath->pmerge != NULL );
    Assert( pmergePath->pmerge->rglineinfo != NULL );

    if ( mergetypeEmptyTree == pmergePath->pmerge->mergetype )
    {
        if ( NULL != pbmNext )
        {
            pbmNext->key.suffix.SetCb( 0 );
            pbmNext->data.SetCb( 0 );
        }

        err = ErrBTIMergeEmptyTree( pfucb, pmergePath );
        return err;
    }

    //  release pages not involved in merge
    //
    BTIMergeReleaseUnneededPages( pmergePath );

    //  if this is OLD, see if we want to do partial merges
    //
    if ( mergetypePartialRight == pmergePath->pmerge->mergetype )
    {
        if( pfucb->ppib->FSessionOLD())
        {
            const IFMP                  ifmp        = pfucb->ifmp;
            const INST * const          pinst       = PinstFromIfmp( ifmp );
            const OLDDB_STATUS * const  poldstatDB  = pinst->m_rgoldstatDB + g_rgfmp[ifmp].Dbid();
            if( poldstatDB->FNoPartialMerges() )
            {
                pmergePath->pmerge->mergetype = mergetypeNone;
            }
        }
        else
        {
            // we don't want to do any partial merges during
            // version store cleanup
            pmergePath->pmerge->mergetype = mergetypeNone;
        }
    }

    switch( pmergePath->pmerge->mergetype )
    {
        case mergetypeEmptyPage:
            Call( ErrBTIMergeOrEmptyPage( pfucb, pmergePath ) );
            break;

        case mergetypePartialLeft:
        case mergetypeFullLeft:
            //  sibling pages, if any, should be RIW latched
            //
            Assert( pgnoNull != pmergePath->csr.Cpage().PgnoPrev() );
            Assert( latchRIW == pmergePath->pmerge->csrLeft.Latch() );

            Assert( pgnoNull == pmergePath->csr.Cpage().PgnoNext()
                || latchRIW == pmergePath->pmerge->csrRight.Latch() );

            //  log merge, merge pages, release empty page
            //
            Call( ErrBTIMergeOrEmptyPage( pfucb, pmergePath ) );
            break;

        //  UNDONE: disable partial merges from RCE cleanup
        //
        case mergetypePartialRight:
        case mergetypeFullRight:
            //  sibling pages, if any, should be RIW latched
            //
            Assert( pgnoNull != pmergePath->csr.Cpage().PgnoNext() );
            Assert( latchRIW == pmergePath->pmerge->csrRight.Latch() );

            Assert( pgnoNull == pmergePath->csr.Cpage().PgnoPrev()
                || latchRIW == pmergePath->pmerge->csrLeft.Latch() );

            //  log merge, merge pages, release empty page
            //
            Call( ErrBTIMergeOrEmptyPage( pfucb, pmergePath ) );
            break;

        default:
            Assert( pmergePath->pmerge->mergetype == mergetypeNone );
            Assert( latchRIW == pmergePath->csr.Latch() );

            //  can not delete only node in page
            //
            if ( pmergePath->csr.Cpage().Clines() == 1 )
            {
                goto HandleError;
            }

            //  upgrade to write latch on leaf page
            //
            pmergePath->csr.UpgradeFromRIWLatch();

            //  delete all other flag-deleted nodes with no active version
            //  may re-org page
            //
            Call( ErrBTISPCDeleteNodes( pfucb, &pmergePath->csr ) );
            Assert( pmergePath->csr.Cpage().Clines() > 0 );
            break;
    }

    if( pmergetype )
    {
        *pmergetype = pmergePath->pmerge->mergetype;
    }

HandleError:
    BTIReleaseMergePaths( pmergePath );
    return err;
}

ERR ErrBTICanMergeWithSibling(FUCB * const pfucb, LINEINFO * const rglineinfo, const PGNO pgnoSibling, BOOL * const pfMergeable)
{
    ERR         err = JET_errSuccess;
    CSR         csrSibling;

    *pfMergeable = fFalse;

    // latch sibling
    err = csrSibling.ErrGetReadPage( pfucb->ppib, pfucb->ifmp, pgnoSibling, BFLatchFlags( bflfNoTouch | bflfNoWait ) );
    if ( errBFLatchConflict == err )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    Call( err );

    // check if page is mergeable with right page
    *pfMergeable = FBTISPCCheckMergeable( pfucb, &csrSibling, rglineinfo );
    if ( !*pfMergeable )
    {
        auto tc = TcCurr();
        PERFOpt( PERFIncCounterTable( cBTUnnecessarySiblingLatch, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );
    }
    else
    {
        // we are going to try to merge the current page into the sibling.
        // merge doesn't remove flag-deleted nodes from the destination page so
        // make a best-effort attempt to remove them now.
        // WARNING: the CSR won't be latched if the upgrade fails
        // Note may re-org page
        //
        // to avoid deadlock we should not wait for the page latch
        // (this only has to be done when latching the left page, but we will
        // do it for either sibling to keep the code simple)
        csrSibling.ReleasePage();
        if ( JET_errSuccess == csrSibling.ErrGetRIWPage(
                pfucb->ppib,
                pfucb->ifmp,
                pgnoSibling,
                BFLatchFlags( bflfNoTouch | bflfNoWait ) ) )
        {
            csrSibling.UpgradeFromRIWLatch();
            Call( ErrBTISPCDeleteNodes( pfucb, &csrSibling ) );
        }
    }

HandleError:
    csrSibling.ReleasePage();
    return err;
}

LOCAL BOOL FBTICheckSibling(const FUCB * const pfucb, const INT pctFull, const PGNO pgnoSibling)
{
    const INT pctFullUnconditionalCheck = 30 - CbBTIFreeDensity( pfucb );

    BOOL fCheckSibling = fFalse;
    if ( pctFull < pctFullUnconditionalCheck )
    {
        fCheckSibling = fTrue;
    }
    else
    {
        CSR csr;
        const ERR err = csr.ErrGetReadPage(
            pfucb->ppib,
            pfucb->ifmp,
            pgnoSibling,
            BFLatchFlags( bflfNoUncached | bflfNoWait | bflfNoTouch ) );
        if ( err >= JET_errSuccess )
        {
            fCheckSibling = fTrue;
        }
        csr.ReleasePage();
    }
    return fCheckSibling;
}

LOCAL ERR ErrBTICheckForMultipageOLC(
    _In_ FUCB * const pfucb,
    const INT pctFull,
    _In_ LINEINFO * const rglineinfo,
    _Out_ BOOL * const pfMultipageOLC)
{
    Assert( pfucb );
    Assert( rglineinfo );
    Assert( pfMultipageOLC );

    ERR err = JET_errSuccess;

    *pfMultipageOLC = fFalse;

    if ( pgnoNull != Pcsr( pfucb )->Cpage().PgnoNext() )
    {
        if ( FBTICheckSibling( pfucb, pctFull, Pcsr( pfucb )->Cpage().PgnoNext() ) )
        {
            BOOL fMergeable;
            Call( ErrBTICanMergeWithSibling( pfucb, rglineinfo, Pcsr( pfucb )->Cpage().PgnoNext(), &fMergeable ) );
            if ( fMergeable )
            {
                *pfMultipageOLC = fTrue;
                goto HandleError;
            }
        }
    }

    // when considering a left merge we only want to do so if this is not the last page in the tree
    // undoing an append merge by doing a left merge could cause a lot of churn
    if ( pgnoNull != Pcsr( pfucb )->Cpage().PgnoPrev() && pgnoNull != Pcsr( pfucb )->Cpage().PgnoNext() )
    {
        if ( FBTICheckSibling( pfucb, pctFull, Pcsr( pfucb )->Cpage().PgnoPrev() ) )
        {
            BOOL fMergeable;
            Call( ErrBTICanMergeWithSibling( pfucb, rglineinfo, Pcsr( pfucb )->Cpage().PgnoPrev(), &fMergeable ) );
            if ( fMergeable )
            {
                *pfMultipageOLC = fTrue;
                goto HandleError;
            }
        }
    }

HandleError:
    return err;
}

//  performs cleanup deleting bookmarked node from tree
//  seeks for node using single page read latches
//  if page is empty/mergeable
//      return NeedsMultipageOLC
//  else
//      expunge all flag deleted nodes without active version from page
//
LOCAL ERR ErrBTISinglePageCleanup( FUCB *pfucb, const BOOKMARK& bm )
{
    Assert( !Pcsr( pfucb )->FLatched() );

    ERR         err;

    if ( pfucb->u.pfcb->FDeletePending() )
    {
        //  btree is scheduled for deletion - don't bother attempting cleanup
        //
        return JET_errSuccess;
    }

    CallR( ErrBTISPCSeek( pfucb, bm ) );

    LINEINFO    *rglineinfo = NULL;
    BOOL        fEmptyPage;
    INT         pctFull;

    //  upgrade page to write latch
    //  if upgrade fails, return NeedsMultipageOLC
    //
    err = Pcsr( pfucb )->ErrUpgradeFromReadLatch( );
    if ( errBFLatchConflict == err )
    {
        auto tc = TcCurr();
        PERFOpt( PERFIncCounterTable( cBTFailedSPCWriteLatch, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );
        err = ErrERRCheck( wrnBTMultipageOLC );
        goto HandleError;
    }
    Call( err );

    //  delete all flag-deleted nodes that have no active versions
    //  this is done first to ensure that the page is scrubbed properly
    //  may re-org the page
    //
    Assert( latchWrite == Pcsr( pfucb )->Latch() );
    Call( ErrBTISPCDeleteNodes( pfucb, Pcsr( pfucb ) ) );

    //  collect page info for nodes
    //
    Call( ErrBTISPCCollectLeafPageInfo(
                pfucb,
                Pcsr( pfucb ),
                &rglineinfo,
                NULL,
                &fEmptyPage,
                NULL,
                &pctFull ) );

    //  if page is empty, needs MultipageOLC (note that
    //  EmptyPage and EmptyTree merges won't create
    //  dependencies, so no need to preclude these types
    //  of merges if a backup is in progress)
    //
    if ( fEmptyPage )
    {
        err = ErrERRCheck( wrnBTMultipageOLC );
        goto HandleError;
    }

    //  ErrBTICheckForMultipageOLC may write-latch the sibling pages.
    //  To avoid deadlocks we cannot have this page write-latched so
    //  the latch is downgraded. That avoids the case where thread A
    //  has the PgnoPrev read-latched and is trying to latch this page while
    //  we have this page write-latched and are trying to write-latch
    //  the previous page.
    Pcsr( pfucb )->Downgrade( latchRIW );

    BOOL fMultipageOLC;
    Call( ErrBTICheckForMultipageOLC( pfucb, pctFull, rglineinfo, &fMultipageOLC ) );
    if ( fMultipageOLC )
    {
        //  if page is mergeable
        //      needs MultipageOLC
        //
        err = ErrERRCheck( wrnBTMultipageOLC );
        goto HandleError;
    }

HandleError:
    if ( rglineinfo != NULL )
    {
        delete [] rglineinfo;
    }

    BTUp( pfucb );
    return err;
}


//  creates mergePath of RIW latched pages from root of tree
//  to seeked bookmark or found internal (as in non-leaf) page.
//
//  contiguous pages can be preread at the parent-of-leaf level
//  while descending the tree if a pPrereadInfo pointer is passed in.
//
//  pgnoSearch is only supposed to be passed in for an internal page search
//
LOCAL ERR ErrBTICreateMergePath( FUCB                       *pfucb,
                                 const BOOKMARK&             bmSearch,
                                 const PGNO                  pgnoSearch,
                                 const BOOL                  fLeafPage,
                                 MERGEPATH                 **ppmergePath,
                                 _Inout_opt_ PrereadInfo * const pPrereadInfo )
{
    ERR err = JET_errSuccess;

    Assert( pfucb );
    Assert( !bmSearch.key.FNull() || !fLeafPage );
    Assert( fLeafPage || ( pgnoSearch != pgnoNull ) );
    Assert( !Pcsr( pfucb )->FLatched() );

    //  create mergePath structure
    //
    CallR( ErrBTINewMergePath( ppmergePath ) );
    Assert( NULL != *ppmergePath );
    CSR* pcsr = &( (*ppmergePath)->csr );

    //  RIW latch root
    //
    Call( pcsr->ErrGetRIWPage( pfucb->ppib,
                                             pfucb->ifmp,
                                             PgnoRoot( pfucb ) ) );

    if ( pcsr->Cpage().FLeafPage() ||
        ( !fLeafPage && ( pgnoSearch == pcsr->Pgno() ) ) )
    {
        //  tree is too shallow to bother doing merges on
        //
        Error( ErrERRCheck( wrnBTShallowTree ) );
    }

    BOOL    fLeftEdgeOfBtree    = fTrue;
    BOOL    fRightEdgeOfBtree   = fTrue;

    Assert( pcsr->Cpage().Clines() > 0 );

    for ( ; ; )
    {
        //  Position currency in the bookmark.
        //
        if ( fLeafPage )
        {
            Call( ErrNDSeek( pfucb, pcsr, bmSearch ) );
        }
        else
        {
            // If our hint is a null key, we must be going all the way to the right-most
            // branch of the tree.
            if ( bmSearch.key.FNull() )
            {
                KEYDATAFLAGS kdf;
                NDIGetKeydataflags( pcsr->Cpage(), pcsr->Cpage().Clines() - 1, &kdf );
                if ( !kdf.key.FNull() )
                {
                    AssertTrack( false, "BtMergeNoNullKey" );
                    Error( ErrBTIReportBadPageLink(
                                pfucb,
                                ErrERRCheck( JET_errBadParentPageLink ),
                                (*ppmergePath)->pmergePathParent ?
                                    (*ppmergePath)->pmergePathParent->csr.Pgno() :
                                    pgnoNull,
                                pcsr->Pgno(),
                                pcsr->Cpage().ObjidFDP(),
                                fTrue,
                                "BtMergeNullKey" ) );
                }

                pcsr->SetILine( pcsr->Cpage().Clines() - 1 );
                NDGet( pfucb, pcsr );
            }
            else
            {
                Call( ErrNDSeekInternal( pfucb, pcsr, bmSearch ) );
            }
        }
        Assert( ( pcsr->ILine() >= 0 ) && ( pcsr->ILine() < pcsr->Cpage().Clines() ) );
        err = JET_errSuccess;

        //  Save iLine for later use
        //
        (*ppmergePath)->iLine = SHORT( pcsr->ILine() );
        Assert( (*ppmergePath)->iLine < pcsr->Cpage().Clines() );

        //  Success case.
        //
        if ( pcsr->Cpage().FLeafPage() ||
            ( !fLeafPage && ( pgnoSearch == pcsr->Pgno() ) ) )
        {
            Assert( !!pcsr->Cpage().FLeafPage() == !!fLeafPage );
            Assert( fLeafPage || ( pgnoSearch == pcsr->Pgno() ) );

            const MERGEPATH * const pmergePathParent        = (*ppmergePath)->pmergePathParent;

            //  if root page was also a leaf page or the internal page we're looking for, we would have
            //  err'd out above with wrnBTShallowTree
            Assert( NULL != pmergePathParent );
            Assert( !( pcsr->Cpage().FRootPage() ) );

            if ( pcsr->Cpage().FLeafPage() )
            {
                const BOOL              fLeafPageIsFirstPage    = ( pgnoNull == pcsr->Cpage().PgnoPrev() );
                const BOOL              fLeafPageIsLastPage     = ( pgnoNull == pcsr->Cpage().PgnoNext() );

                if ( fLeftEdgeOfBtree ^ fLeafPageIsFirstPage )
                {
                    //  if not repair, assert, otherwise, suppress the assert
                    //  and repair will just naturally err out
                    AssertSz( g_fRepair, "Corrupt B-tree: first leaf page has mismatched parent" );
                    Call( ErrBTIReportBadPageLink(
                                pfucb,
                                ErrERRCheck( JET_errBadParentPageLink ),
                                pmergePathParent->csr.Pgno(),
                                pcsr->Pgno(),
                                pcsr->Cpage().PgnoPrev(),
                                fTrue,
                                "BtMergeParentMismatchFirst" ) );
                }
                if ( fRightEdgeOfBtree ^ fLeafPageIsLastPage )
                {
                    //  if not repair, assert, otherwise, suppress the assert
                    //  and repair will just naturally err out
                    AssertSz( g_fRepair, "Corrupt B-tree: last leaf page has mismatched parent" );
                    Call( ErrBTIReportBadPageLink(
                                pfucb,
                                ErrERRCheck( JET_errBadParentPageLink ),
                                pmergePathParent->csr.Pgno(),
                                pcsr->Pgno(),
                                pcsr->Cpage().PgnoNext(),
                                fTrue,
                                "BtMergeParentMismatchLast" ) );
                }
            }

            break;
        }

        //  Preread only if we've requested preread and we are not looking for a specific page and we've requested preread.
        //
        if ( pPrereadInfo && pcsr->Cpage().FParentOfLeaf() && fLeafPage )
        {
            BTIPrereadContiguousPages( pfucb->ifmp, *pcsr, pPrereadInfo, bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->ifmp ), TcCurr() );
        }

        Assert( pcsr->Cpage().FInvisibleSons() );
        Assert( !( fRightEdgeOfBtree ^ pcsr->Cpage().FLastNodeHasNullKey() ) );

        fRightEdgeOfBtree = ( fRightEdgeOfBtree
                            && (*ppmergePath)->iLine == pcsr->Cpage().Clines() - 1 );
        fLeftEdgeOfBtree = ( fLeftEdgeOfBtree
                            && 0 == (*ppmergePath)->iLine );

        //  allocate another mergePath structure for next level
        //
        Call( ErrBTINewMergePath( ppmergePath ) );
        pcsr = &( (*ppmergePath)->csr );

        //  access child page
        //
        Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
        Call( pcsr->ErrGetRIWPage(
                                pfucb->ppib,
                                pfucb->ifmp,
                                *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv() ) );

        //  Search match failure cases:
        //    1- We want an internal page but we already got to the leaf level.
        //    2- We want a leaf page and we got to the leaf level, but the page is not the one we're expecting.
        //
        if ( ( pcsr->Cpage().FLeafPage() && !fLeafPage ) ||
                ( pcsr->Cpage().FLeafPage() && fLeafPage && ( pgnoSearch != pgnoNull ) && ( pgnoSearch != pcsr->Pgno() ) ) )
        {
            Error( ErrERRCheck( JET_errRecordNotFound ) );
        }
    }

HandleError:
#ifdef DEBUG
    if ( err >= JET_errSuccess )
    {
        if ( err != wrnBTShallowTree )
        {
            Assert( !!(*ppmergePath)->csr.Cpage().FLeafPage() == !!fLeafPage );
        }
        else
        {
            Assert( (*ppmergePath)->csr.Cpage().FRootPage() );
        }
    }
#endif

    return err;
}


//  ================================================================
LOCAL VOID BTIMergeCopyNextBookmark( FUCB       * const pfucb,
                                     MERGEPATH  * const pmergePathLeaf,
                                     BOOKMARK   * const pbmNext,
                                     const BOOL         fRightMerges )
//  ================================================================
//
//  Copies next bookmark to seek for online defrag.
//
//  This function can run in two modes, left merge mode and right
//  merge mode. This is necessary because while OLD processes a 
//  b-tree from right to left doing right merges, b-tree defragmentation
//  works from left to right doing left merges. The fRightMerges
//  parameter controls the behaviour of this function:
//
//  if fRightMerges is TRUE
//      - pbmNext is set to the bookmark of the first node on the page on the left
//
//  if fRightMerges is FALSE
//      - pbmNext is set to the bookmark of the last node on the page on the right
//
//-

{
    Assert( NULL != pmergePathLeaf );
    Assert( NULL != pbmNext );
    Assert( pmergePathLeaf->csr.FLatched() );
    Assert( pmergePathLeaf->pmerge != NULL );

    Assert( pbmNext->key.prefix.FNull() );

    CSR * const pcsr = fRightMerges ? &(pmergePathLeaf->pmerge->csrLeft) : &(pmergePathLeaf->pmerge->csrRight);

    //  if no sibling, nullify bookmark
    //
    if ( pcsr->Pgno() == pgnoNull )
    {
        pbmNext->key.suffix.SetCb( 0 );
        pbmNext->data.SetCb( 0 );
        return;
    }

    Assert( mergetypeEmptyTree != pmergePathLeaf->pmerge->mergetype );

    BOOKMARK    bm;

    Assert( pcsr->FLatched() );
    Assert( pcsr->Cpage().Clines() > 0 );

    //  get bm of first node from page
    //
    if( fRightMerges )
    {
        pcsr->SetILine( 0 );
    }
    else
    {
        pcsr->SetILine( pcsr->Cpage().Clines() - 1 );
    }
    NDGet( pfucb, pcsr );
    NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );

    //  copy bm into given buffer
    //
    Assert( NULL != pbmNext->key.suffix.Pv() );
    Assert( 0 == pbmNext->key.prefix.Cb() );
    bm.key.CopyIntoBuffer( pbmNext->key.suffix.Pv(), bm.key.Cb() );
    pbmNext->key.suffix.SetCb( bm.key.Cb() );

    pbmNext->data.SetPv( (BYTE *) pbmNext->key.suffix.Pv() + pbmNext->key.Cb() );
    bm.data.CopyInto( pbmNext->data );

    return;
}


//  ================================================================
LOCAL ERR ErrBTISelectMerge(
    FUCB            * const pfucb,
    MERGEPATH       * const pmergePathLeaf,
    const BOOKMARK&         bm,
    BOOKMARK        * const pbmNext,
    RECCHECK        * const preccheck,
    const BOOL              fRightMerges )
//  ================================================================
//
//  Select merge at leaf level and recursively at parent levels.
//  pmergePathLeaf should be already created and RIW latched.
//
//  This function can run in two modes, left merge mode and right
//  merge mode. This is necessary because while OLD processes a 
//  b-tree from right to left doing right merges, b-tree defragmentation
//  works from left to right doing left merges. The fRightMerges
//  parameter controls the behaviour of this function:
//
//  if fRightMerges is TRUE
//      - mergetypeFullRight, mergetypePartialRight are used
//      - pbmNext is set to the bookmark of the page on the left
//
//  if fRightMerges is FALSE
//      - mergetypeFullLeft, mergetypePartialLeft are used
//      - pbmNext is set to the bookmark of the page on the right
//
//- 
{
    ERR             err;

    Assert( pmergePathLeaf->csr.Cpage().FLeafPage() );

    //  allocate merge structure and initialize
    //
    Call( ErrBTINewMerge( pmergePathLeaf ) );
    Assert( NULL != pmergePathLeaf->pmerge );

    MERGE * const pmerge = pmergePathLeaf->pmerge;
    pmerge->mergetype = mergetypeNone;

    //  check if page is mergeable, without latching sibling pages,
    //  also collect info on all nodes in page
    //
    Call( ErrBTIMergeCollectPageInfo( pfucb, pmergePathLeaf, preccheck, fRightMerges ) );

    //  if we want the next bookmark, then we have to latch the sibling page to
    //  obtain it, even if no merge will occur with the current page
    if ( mergetypeNone == pmerge->mergetype && NULL == pbmNext )
    {
        return err;
    }
    else if ( mergetypeEmptyTree == pmerge->mergetype )
    {
        return err;
    }

    //  page could be merged
    //  acquire latches on sibling pages
    //  this might cause latch of merged page to be released
    //
    Call( ErrBTIMergeLatchSiblingPages( pfucb, pmergePathLeaf ) );

    //  copy next bookmark to seek for online defrag
    //
    if ( NULL != pbmNext )
    {
        BTIMergeCopyNextBookmark( pfucb, pmergePathLeaf, pbmNext, fRightMerges );
    }

    Assert( pmergePathLeaf->pmergePathParent != NULL || mergetypeNone == pmerge->mergetype );

    {
        //  page may have changed when we released and reacquired latch
        //  reseek to deleted node
        //  recompute if merge is possible
        //
        BTIReleaseMergeLineinfo( pmerge );

        Call( ErrNDSeek( pfucb, &pmergePathLeaf->csr, bm ) );

        pmergePathLeaf->iLine = SHORT( pmergePathLeaf->csr.ILine() );

        //  we don't want to check the same node multiple times so we don't bother with the reccheck
        Call( ErrBTIMergeCollectPageInfo( pfucb, pmergePathLeaf, NULL, fRightMerges ) );
    }

    // ErrBTIMergeCollectPageInfo only chooses these types of merges
    Assert( mergetypeNone == pmerge->mergetype
            || mergetypeEmptyPage == pmerge->mergetype
            || mergetypeEmptyTree == pmerge->mergetype
            || mergetypeFullRight == pmerge->mergetype
            || mergetypeFullLeft == pmerge->mergetype );

    switch ( pmerge->mergetype )
    {
        case mergetypeEmptyPage:
            Call( ErrBTISelectMergeInternalPages( pfucb, pmergePathLeaf ) );
            break;

        case mergetypeFullRight:
        case mergetypeFullLeft:
            if( fRightMerges )
            {
                Assert( mergetypeFullRight == pmerge->mergetype );
            }
            else
            {
                Assert( mergetypeFullLeft == pmerge->mergetype );
            }

            //  check if page can be merged to next page
            //  without violating density constraint
            //
            BTICheckMergeable( pfucb, pmergePathLeaf, fRightMerges );
            if ( mergetypeNone == pmerge->mergetype )
            {
                return err;
            }

            //  select merge at parent pages
            //
            Call( ErrBTISelectMergeInternalPages( pfucb, pmergePathLeaf ) );

            if ( mergetypeEmptyPage != pmerge->mergetype )
            {
                //  calculate uncommitted freed space in destination page
                //
                const CSR * const pcsrDest = fRightMerges ? &(pmerge->csrRight) : &(pmerge->csrLeft);
                pmerge->cbUncFreeDest   = pcsrDest->Cpage().CbUncommittedFree() +
                                            pmerge->cbSizeMaxTotal -
                                            pmerge->cbSizeTotal;
            }
            break;

        default:
            Assert( mergetypeNone == pmerge->mergetype
                || mergetypeEmptyTree == pmerge->mergetype );
    }

HandleError:
    return err;
}


//  allocate a new merge structure
//      and link it to mergePath
//
ERR ErrBTINewMerge( MERGEPATH *pmergePath )
{
    MERGE   *pmerge;

    Assert( pmergePath != NULL );
    Assert( pmergePath->pmerge == NULL );

    //  allocate split structure
    //
    if ( !( pmerge = new MERGE ) )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    //  link merge structure to pmergePath
    //
    pmerge->pmergePath = pmergePath;
    pmergePath->pmerge = pmerge;

    return JET_errSuccess;
}


INLINE VOID BTIReleaseMergeLineinfo( MERGE *pmerge )
{
    if ( pmerge->rglineinfo != NULL )
    {
        delete [] pmerge->rglineinfo;
        pmerge->rglineinfo = NULL;
    }
}


//  revert dbtime of every (write) latched page to the before dirty dbtime
//
VOID BTIMergeRevertDbtime( MERGEPATH *pmergePathTip )
{
    MERGEPATH *pmergePath = pmergePathTip;

    Assert( NULL == pmergePath->pmergePathChild );
    for ( ; NULL != pmergePath;
            pmergePath = pmergePath->pmergePathParent )
    {
        MERGE   *pmerge = pmergePath->pmerge;

        //  set dbtime for left sibling
        //
        if ( NULL != pmerge )
        {
            if ( pgnoNull != pmerge->csrLeft.Pgno() &&
                latchWrite == pmergePath->pmerge->csrLeft.Latch() &&
                dbtimeNil != pmerge->dbtimeLeftBefore )
            {
                Assert( pmerge->csrLeft.Cpage().FLeafPage() );

                Assert ( dbtimeInvalid != pmerge->dbtimeLeftBefore );
                Assert ( pmerge->dbtimeLeftBefore < pmerge->csrLeft.Dbtime() );
                if ( dbtimeInvalid != pmerge->dbtimeLeftBefore )    // defense in depth
                {
                    pmerge->csrLeft.RevertDbtime( pmerge->dbtimeLeftBefore, pmerge->fFlagsLeftBefore );
                }
            }
        }

        //  set dbtime for merge page
        //
        if ( latchWrite == pmergePath->csr.Latch() &&
            dbtimeNil != pmergePath->dbtimeBefore )
        {
            //  set the dbtime for this page
            //
            Assert ( latchWrite == pmergePath->csr.Latch() );

            Assert ( dbtimeInvalid != pmergePath->dbtimeBefore );
            Assert ( pmergePath->dbtimeBefore < pmergePath->csr.Dbtime() );
            if ( dbtimeInvalid != pmergePath->dbtimeBefore )    // defense in depth
            {
                pmergePath->csr.RevertDbtime( pmergePath->dbtimeBefore, pmergePath->fFlagsBefore );
            }
        }

        //  set dbtime for right sibling
        //
        if ( pmerge != NULL )
        {
            if ( pgnoNull != pmerge->csrRight.Pgno() &&
                latchWrite == pmerge->csrRight.Latch() &&
                dbtimeNil != pmerge->dbtimeRightBefore )
            {
                Assert( pmerge->csrRight.Cpage().FLeafPage() );

                Assert ( dbtimeInvalid != pmerge->dbtimeRightBefore );
                Assert ( pmerge->dbtimeRightBefore < pmerge->csrRight.Dbtime() );
                if ( dbtimeInvalid != pmerge->dbtimeRightBefore )    // defense in depth
                {
                    pmerge->csrRight.RevertDbtime( pmerge->dbtimeRightBefore, pmerge->fFlagsRightBefore );
                }
            }
        }
    }
}


// Calculate the iline needed to position cursor fractionally in the page.
// Overall accurate-ish if each level of the tree is balanced.
//
// *pdblFracPos is InOut.  On input, it holds the fraction of csr->Cpage()->Clines to
// skip.  If the current page is not a leaf, on output it holds the fraction of
// lines to skip on a child page.
LOCAL INT IlineBTIFrac( const CSR * const pcsr, double *pdblFracPos )
{
    const INT   clines = pcsr->Cpage().Clines( );
    INT         iLine;

    Assert( 0 <= *pdblFracPos );
    Assert( *pdblFracPos <= 1.0 );

    // What line should we be on?

    if ( 0.0 == *pdblFracPos )
    {
        // The absolute beginning of the page.
        return 0;
    }

    if ( 1.0 == *pdblFracPos )
    {
        // The absolute end of the page.
        return (clines - 1);
    }

    // OK, have to calculate it.  An example:
    // Assume a perfectly balanced tree with clines = 10 at the first level and
    // clines = 20 at every 2nd level leaf page, for 200 total rows in the leaves.
    // Given *pdblFracPos = .89, we want to land 89% of the way through the total
    // index, which is the 178th row.  To get there, we multiply .89 by 10
    // (ie. fracPos * clines ) and get 8.90.  That means we take the integral part
    // (i.e. 8) as the iline for the first level.  That gets us to a child page
    // holding rows 160-189.  We use the fractional portion of 8.90 as the fractional
    // position to use at the next level.  That results in us looking for fracPos .90
    // with cline = 20 on the child page, which is iLine 18.  That's the 178th
    // element of the index.  Calling repeatedly gets you to the desired location
    // in the tree, no matter how deep the tree is.
    // Of course, the more out of balance your tree is, the less likely you are to
    // land on the desired row.

    const double dblLine = (double)clines * (*pdblFracPos);
    Assert( dblLine <= clines );
    Assert( 0 <= dblLine );

    iLine = (INT)dblLine;
    if ( iLine >= clines )
    {
        // Special case of end-of-range, since we landed on a line that is actually
        // too great.  The check for '>' is to account for any oddness from multiplying
        // a double by an INT.  You might wonder how we got here when we already
        // apparantly dealt with *pdblFracPos = 1.0.  We did, but if *pdblFracPos
        // is .9999<repeated>, then multiplying it by clines could still result in iLine
        // being equal to clines.

        // Position on the last actual line.
        iLine = clines - 1;

        // And mark that in any child, we need to keep picking the last line.
        *pdblFracPos = 1.0;
    }
    else
    {
        // Normal case where iLine is in range and the decimal portion is the fraction
        // of line we couldn't skip over at this level.
        *pdblFracPos = dblLine - iLine;

        // Just in case there's any oddness from subtracting an INT from a double,
        // force the value to be in range.
        if ( *pdblFracPos < 0.0 )
        {
            *pdblFracPos = 0.0;
        }
        else if ( *pdblFracPos > 1.0 )
        {
            *pdblFracPos = 1.0;
        }
    }

    Assert( 0 <= *pdblFracPos );
    Assert( *pdblFracPos <= 1.0 );
    Assert( iLine < clines );
    return iLine;
}


//  collects lineinfo for page
//  if all nodes in page are flag-deleted without active version
//      set fEmptyPage
//  if there exists a flag deleted node with active version
//      set fExistsFlagDeletedNodeWithActiveVersion
//
ERR ErrBTISPCCollectLeafPageInfo(
    FUCB        *pfucb,
    CSR         *pcsr,
    LINEINFO    **pplineinfo,
    RECCHECK    * const preccheck,
    BOOL        *pfEmptyPage,
    BOOL        *pfExistsFlagDeletedNodeWithActiveVersion,
    INT         *ppctFull )
{
    const INT   clines                                  = pcsr->Cpage().Clines();
    BOOL        fExistsFlagDeletedNodeWithActiveVersion = fFalse;
    ULONG       cbSizeMaxTotal                          = 0;

    Assert( pcsr->Cpage().FLeafPage() );

    //  UNDONE: allocate rglineinfo on demand [only if not empty page]
    //
    //  allocate rglineinfo
    //
    Assert( NULL == *pplineinfo );
    *pplineinfo = new LINEINFO[clines];

    if ( NULL == *pplineinfo )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    LINEINFO    *rglineinfo = *pplineinfo;

    Assert( NULL != pfEmptyPage );
    *pfEmptyPage = fTrue;

    //  collect total size of movable nodes in page
    //      i.e, nodes that are not flag-deleted
    //           or flag-deleted with active versions
    //
    for ( INT iline = 0; iline < clines; iline++ )
    {
        pcsr->SetILine( iline );
        NDGet( pfucb, pcsr );

        rglineinfo[iline].kdf       = pfucb->kdfCurr;
        rglineinfo[iline].cbSize    = CbNDNodeSizeTotal( pfucb->kdfCurr );
        rglineinfo[iline].cbSizeMax = CbBTIMaxSizeOfNode( pfucb, pcsr );

        if ( !FNDDeleted( pfucb->kdfCurr ) )
        {
            if( NULL != preccheck )
            {
                (*preccheck)( pfucb->kdfCurr, Pcsr( pfucb )->Pgno() );
            }

            cbSizeMaxTotal += rglineinfo[iline].cbSizeMax;
            *pfEmptyPage = fFalse;
            continue;
        }

        rglineinfo[iline].fVerActive = fFalse;

        Assert( FNDDeleted( pfucb->kdfCurr ) );
        if ( FNDPossiblyVersioned( pfucb, pcsr ) )
        {
            BOOKMARK    bm;
            NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );

            if ( FVERActive( pfucb, bm ) )
            {
                //  version is still active
                //
                cbSizeMaxTotal += rglineinfo[iline].cbSizeMax;
                rglineinfo[iline].fVerActive = fTrue;
                *pfEmptyPage = fFalse;
                fExistsFlagDeletedNodeWithActiveVersion = fTrue;
            }
        }
    }

    if ( NULL != pfExistsFlagDeletedNodeWithActiveVersion )
    {
        *pfExistsFlagDeletedNodeWithActiveVersion = fExistsFlagDeletedNodeWithActiveVersion;
    }
    if ( NULL != ppctFull )
    {
        *ppctFull = ( cbSizeMaxTotal * 100 ) / CbNDPageAvailMostNoInsert( g_rgfmp[ pfucb->ifmp ].CbPage() );
    }

    return JET_errSuccess;
}


//  collects merge info for page
//  if page has flag-deleted node with an active version
//      return pmerge->mergetype = mergetypeNone
//
LOCAL ERR ErrBTIMergeCollectPageInfo(
    FUCB * const pfucb,
    MERGEPATH * const pmergePath,
    RECCHECK * const preccheck,
    const BOOL fRightMerges )
{
    ERR         err;
    const INT   clines = pmergePath->csr.Cpage().Clines();
    MERGE       *pmerge = pmergePath->pmerge;

    Assert( pmerge != NULL );
    Assert( pmergePath->csr.Cpage().FLeafPage() );

    BOOL    fEmptyPage;
    BOOL    fExistsFlagDeletedNodeWithActiveVersion;

    pmerge->clines = clines;
    Assert( NULL == pmerge->rglineinfo );

    Assert( 0 == pmerge->cbSavings );
    Assert( 0 == pmerge->cbSizeTotal );
    Assert( 0 == pmerge->cbSizeMaxTotal );

    CallR( ErrBTISPCCollectLeafPageInfo(
            pfucb,
            &pmergePath->csr,
            &pmerge->rglineinfo,
            preccheck,
            &fEmptyPage,
            &fExistsFlagDeletedNodeWithActiveVersion,
            NULL ) );
    Assert( NULL != pmerge->rglineinfo );

    Assert( pmergePath->pmergePathParent != NULL ||
            PgnoRoot( pfucb ) == pmergePath->csr.Pgno() &&
            pmergePath->csr.Cpage().FRootPage() );

    if ( NULL == pmergePath->pmergePathParent )
    {
        //  no merge/empty page possible if single-page tree    
        pmerge->mergetype = mergetypeNone;
    }
    else if (   pmergePath->csr.Cpage().PgnoPrev() != pgnoNull &&
                pmergePath->csr.Cpage().PgnoNext() == pgnoNull &&
                pmergePath->pmergePathParent->csr.Cpage().Clines() == 1 )
    {
        //  eliminate the case where right sibling does not exist
        //  and left sibling does not have the same parent
        //  since we can't fix page pointer to left sibling to be NULL-keyed
        //      [left sibling page's parent is not latched]
        //      
        pmerge->mergetype = mergetypeNone;
    }
    else if (   !fRightMerges
                && 0 == pmergePath->pmergePathParent->csr.ILine() )
    {
        //  When a left merge is done nodes are added to the end of the page
        //  to the left of the merged page. That means the separator key has
        //  to be updated. If the parent node of the merged page is the first
        //  node in the parent-of-leaf page then it won't be possible to
        //  update the separator key -- the page containing the pointer to 
        //  the left page is on a different parent-of-leaf page.
        //
        //  To work around this left merges are not done when the parent pointer
        //  is the first pointer on the page
        pmerge->mergetype = mergetypeNone;
    }
    else if ( fEmptyPage )
    {
        pmerge->mergetype   = ( pmergePath->csr.Cpage().PgnoPrev() == pgnoNull
                                && pmergePath->csr.Cpage().PgnoNext() == pgnoNull ?
                                    mergetypeEmptyTree :
                                    mergetypeEmptyPage );
    }
    else if ( fExistsFlagDeletedNodeWithActiveVersion )
    {
        //  next cleanup with clean this page
        //
        pmerge->mergetype = mergetypeNone;
    }
    else
    {
        pmerge->mergetype = fRightMerges ? mergetypeFullRight : mergetypeFullLeft;
    }

    return err;
}


//  latches sibling pages
//  release current page
//  RIW latch left, current and right pages in order
//
ERR ErrBTIMergeLatchSiblingPages( FUCB *pfucb, MERGEPATH * const pmergePathLeaf )
{
    ERR             err             = JET_errSuccess;
    CSR * const     pcsr            = &pmergePathLeaf->csr;
    MERGE * const   pmerge          = pmergePathLeaf->pmerge;
    const PGNO      pgnoCurr        = pcsr->Pgno();
    ULONG           cLatchFailures  = 0;

    Assert( NULL != pmerge );

Start:
    Assert( latchRIW == pcsr->Latch() );
    Assert( !pcsr->Cpage().FPreInitPage() );

    const DBTIME    dbtimeCurr      = pcsr->Dbtime();
    const PGNO      pgnoLeft        = pcsr->Cpage().PgnoPrev();
    PGNO            pgnoRight;

    if ( pgnoLeft != pgnoNull )
    {
        pcsr->ReleasePage();

        Assert( mergetypeEmptyTree != pmerge->mergetype );
        Call( pmerge->csrLeft.ErrGetRIWPage( pfucb->ppib,
                                             pfucb->ifmp,
                                             pgnoLeft ) );

        const BOOL  fBadSiblingPointer  = ( pmerge->csrLeft.Cpage().PgnoNext() != pgnoCurr );
        if( fBadSiblingPointer
            || pmerge->csrLeft.Cpage().ObjidFDP() != pfucb->u.pfcb->ObjidFDP() )
        {
            const PGNO  pgnoBadLink     = ( fBadSiblingPointer ?
                                                    pmerge->csrLeft.Cpage().PgnoNext() :
                                                    pmerge->csrLeft.Cpage().ObjidFDP() );

            //  left page has split after we released current page
            //  release left page
            //  relatch current page and retry
            //
            pmerge->csrLeft.ReleasePage();

            Call( pcsr->ErrGetRIWPage( pfucb->ppib,
                                       pfucb->ifmp,
                                       pgnoCurr ) );

            Assert( pcsr->Dbtime() >= dbtimeCurr );
            if ( pcsr->Dbtime() == dbtimeCurr )
            {
                //  dbtime didn't change, but pgnoNext of the left page doesn't
                //  match pgnoPrev of the current page - must be bad page link, so
                //  if not repair, assert, otherwise, suppress the assert and
                //  repair will just naturally err out
                AssertSz( g_fRepair, "Corrupt B-tree: bad leaf page links detected on Merge (left sibling)" );
                Call( ErrBTIReportBadPageLink(
                            pfucb,
                            ErrERRCheck( JET_errBadPageLink ),
                            pgnoCurr,
                            pgnoLeft,
                            pgnoBadLink,
                            fTrue,
                            fBadSiblingPointer ?
                                "BtMergeBadLeftPagePgnoNext" :
                                "BtMergeBadLeftPageObjid" ) );
            }
            else if ( cLatchFailures < 10
                && !pcsr->Cpage().FEmptyPage() )    //  someone else could have cleaned the page when we gave it up to latch the sibling
            {
                cLatchFailures++;
                goto Start;
            }
            else
            {
                Error( ErrERRCheck( errBTMergeNotSynchronous ) );
            }
        }

        //  relatch current page
        //
        Call( pcsr->ErrGetRIWPage( pfucb->ppib,
                                   pfucb->ifmp,
                                   pgnoCurr ) );
    }
    else
    {
        //  set pgnoLeft to pgnoNull
        //
        pmerge->csrLeft.Reset();
    }


    Assert( latchRIW == pcsr->Latch() );
    pgnoRight = pcsr->Cpage().PgnoNext();

    Assert( pmerge->csrRight.Pgno() == pgnoNull );
    if ( pgnoRight != pgnoNull )
    {
        Assert( mergetypeEmptyTree != pmerge->mergetype );
        Call( pmerge->csrRight.ErrGetRIWPage( pfucb->ppib,
                                              pfucb->ifmp,
                                              pgnoRight ) );

        const BOOL  fBadSiblingPointer  = ( pmerge->csrRight.Cpage().PgnoPrev() != pgnoCurr );
        if( fBadSiblingPointer
            || pmerge->csrRight.Cpage().ObjidFDP() != pfucb->u.pfcb->ObjidFDP() )
        {
            const PGNO  pgnoBadLink     = ( fBadSiblingPointer ?
                                                    pmerge->csrRight.Cpage().PgnoPrev() :
                                                    pmerge->csrRight.Cpage().ObjidFDP() );

            //  if not repair, assert, otherwise, suppress the assert and
            //  repair will just naturally err out
            AssertSz( g_fRepair, "Corrupt B-tree: bad leaf page links detected on Merge (right sibling)" );
            Call( ErrBTIReportBadPageLink(
                    pfucb,
                    ErrERRCheck( JET_errBadPageLink ),
                    pgnoCurr,
                    pmerge->csrRight.Pgno(),
                    pgnoBadLink,
                    fTrue,
                    fBadSiblingPointer ?
                        "BtMergeBadRightPagePgnoPrev" :
                        "BtMergeBadRightPageObjid" ) );
        }
    }

    Assert( pgnoRight == pcsr->Cpage().PgnoNext() );
    Assert( pgnoLeft == pcsr->Cpage().PgnoPrev() );
    Assert( pcsr->Cpage().FLeafPage() );
    Assert( pgnoLeft == pgnoNull || pmerge->csrLeft.Cpage().FLeafPage() );
    Assert( pgnoRight == pgnoNull || pmerge->csrRight.Cpage().FLeafPage() );

HandleError:
    return err;
}


//  ================================================================
LOCAL BOOL FBTICheckMergeableOneNode(
    const FUCB * const pfucb,
    CSR * const pcsrDest,
    MERGE * const pmerge,
    const INT iline,
    const ULONG cbDensityFree,
    const BOOL fRightMerges )
//  ================================================================
//
//  Used by BTICheckMergeable to determine if the given iline will
//  fit into the target page in addition to the other nodes already
//  selected. Returns true if it will fit, false if it won't.
//
//  This function can run in two modes, left merge mode and right
//  merge mode. This is necessary because while OLD processes a 
//  b-tree from right to left doing right merges, b-tree defragmentation
//  works from left to right doing left merges. The fRightMerges
//  parameter controls the behaviour of this function:
//
//  if fRightMerges is TRUE
//      - partialRightMerge is selected if not all nodes will fit
//
//  if fRightMerges is FALSE
//      - partialLeftMerge is selected if not all nodes will fit
//
//-
{
    if( fRightMerges )
    {
        Assert( mergetypeFullRight == pmerge->mergetype );
    }
    else
    {
        Assert( mergetypeFullLeft == pmerge->mergetype );
    }

    LINEINFO * const plineinfo = &pmerge->rglineinfo[iline];

    if ( FNDDeleted( plineinfo->kdf ) && !plineinfo->fVerActive )
    {
        //  this node will not be moved
        //
        return fTrue;
    }

    //  calculate cbPrefix for node
    //
    const INT   cbCommon = CbCommonKey( pfucb->kdfCurr.key,
                                        plineinfo->kdf.key );
    INT         cbSavings = pmerge->cbSavings;

    if ( cbCommon > cbPrefixOverhead )
    {
        plineinfo->cbPrefix = cbCommon;
        cbSavings += cbCommon - cbPrefixOverhead;
    }
    else
    {
        plineinfo->cbPrefix = 0;
    }

    //  moving nodes should not violate density constraint [assuming no rollbacks]
    //  and moving nodes should still allow rollbacks to succeed
    //
    Assert( pcsrDest->Pgno() != pgnoNull );
    const INT   cbSizeTotal     = pmerge->cbSizeTotal + plineinfo->cbSize;
    const INT   cbSizeMaxTotal  = pmerge->cbSizeMaxTotal + plineinfo->cbSizeMax;
    const INT   cbReq           = ( cbSizeMaxTotal - cbSavings ) + cbDensityFree;

    //  UNDONE: this may be expensive if we do not want a partial merge
    //          move this check to later [on all nodes in page]
    //
    Assert( cbReq >= 0 );
    if ( !FNDFreePageSpace( pfucb, pcsrDest, cbReq ) )
    {
        //  if no nodes are moved, there is no merge
        //
        if ( fRightMerges && iline == pmerge->clines - 1 )
        {
            pmerge->mergetype = mergetypeNone;
        }
        else if ( !fRightMerges && iline == 0 )
        {
            pmerge->mergetype = mergetypeNone;
        }
        else
        {
            if( fRightMerges )
            {
                Assert( iline + 1 == pmerge->ilineMerge );
            }
            else
            {
                Assert( iline - 1 == pmerge->ilineMerge );
            }
            Assert( pmerge->ilineMerge < pmerge->clines );
            Assert( 0 <= pmerge->ilineMerge );

            pmerge->mergetype = fRightMerges ? mergetypePartialRight : mergetypePartialLeft;
        }

        return fFalse;
    }

    //  update merge to include node
    //
    pmerge->cbSavings       = cbSavings;
    pmerge->cbSizeTotal     = cbSizeTotal;
    pmerge->cbSizeMaxTotal  = cbSizeMaxTotal;

    return fTrue;
}

//  ================================================================
LOCAL VOID BTICheckMergeable(
    FUCB * const pfucb,
    MERGEPATH * const pmergePath,
    const BOOL fRightMerges )
//  ================================================================
//
//  Calculates how many nodes in merged page fit in the right/left page
//  without violating density constraint.
//
//  This function can run in two modes, left merge mode and right
//  merge mode. This is necessary because while OLD processes a 
//  b-tree from right to left doing right merges, b-tree defragmentation
//  works from left to right doing left merges. The fRightMerges
//  parameter controls the behaviour of this function:
//
//  if fRightMerges is TRUE
//      - calculates how many nodes will fit onto the right page
//      - processes nodes from the highest iline to the lowest
//      - all nodes >= ilineMerge will fit into the right page
//      - partialRightMerge is selected if not all nodes will fit
//
//  if fRightMerges is FALSE
//      - calculates how many nodes will fit onto the left page
//      - processes nodes from the lowest (0) iline to the highest
//      - all nodes <= ilineMerge will fit into the left page
//      - partialLeftMerge is selected if not all nodes will fit
//
//-

{
    MERGE * const pmerge = pmergePath->pmerge;
    Assert( NULL != pmerge );

    if( fRightMerges )
    {
        Assert( mergetypeFullRight == pmerge->mergetype );
        if ( pmergePath->csr.Cpage().PgnoNext() == pgnoNull )
        {
            //  no right sibling -- can't merge
            //
            Assert( latchNone == pmerge->csrRight.Latch() );
            pmerge->mergetype = mergetypeNone;
            return;
        }
    }
    else
    {
        Assert( mergetypeFullLeft == pmerge->mergetype );
        if ( pmergePath->csr.Cpage().PgnoPrev() == pgnoNull )
        {
            //  no left sibling -- can't merge
            //
            Assert( latchNone == pmerge->csrLeft.Latch() );
            pmerge->mergetype = mergetypeNone;
            return;
        }
    }

    CSR * const pcsrDest = fRightMerges ? &(pmerge->csrRight) : &(pmerge->csrLeft);
    Assert( pcsrDest->FLatched() );

    //  calculate total size, total max size and prefixes of nodes to move
    //
    Assert( NULL != pmerge->rglineinfo );
    Assert( 0 == pmerge->cbSizeTotal );
    Assert( 0 == pmerge->cbSizeMaxTotal );

    const ULONG cbDensityFree = ( pcsrDest->Cpage().FLeafPage() ? CbBTIFreeDensity( pfucb ) : 0 );

    NDGetPrefix( pfucb, pcsrDest );

    Assert( pmerge->clines > 0 );
    if ( pmerge->clines <= 0 )
    {
        return;
    }

    if( fRightMerges )
    {
        for ( INT iline = pmerge->clines - 1; iline >= 0 ; iline-- )
        {
            if( !FBTICheckMergeableOneNode(
                    pfucb,
                    pcsrDest,
                    pmerge,
                    iline,
                    cbDensityFree,
                    fRightMerges ) )
            {
                break;
            }
            pmerge->ilineMerge = iline;
        }
        if( mergetypeFullRight == pmerge->mergetype )
        {
            Assert( 0 == pmerge->ilineMerge );
        }
        else if( mergetypePartialRight == pmerge->mergetype )
        {
            Assert( pmerge->ilineMerge > 0 );
        }
    }
    else
    {
        for ( INT iline = 0; iline < pmerge->clines; iline++ )
        {
            if( !FBTICheckMergeableOneNode(
                    pfucb,
                    pcsrDest,
                    pmerge,
                    iline,
                    cbDensityFree,
                    fRightMerges ) )
            {
                break;
            }
            pmerge->ilineMerge = iline;
        }
        if( mergetypeFullLeft == pmerge->mergetype )
        {
            Assert( pmerge->clines-1 == pmerge->ilineMerge );
        }
        else if( mergetypePartialLeft == pmerge->mergetype )
        {
            Assert( pmerge->ilineMerge < pmerge->clines-1 );
        }
    }
}


//  ================================================================
BOOL FBTIFreePageSpaceForMerge(
        const FUCB *pfucb,
        CSR * const pcsr,
        const ULONG cbReq )
//  ================================================================
//
//  Determine if the given page has at lest cbReq bytes free for a merge.
//  A merge will remove committed flag-deleted nodes on the page
//  so we should ignore their space usage.
//  
//-
{
    ULONG cbReqActual = cbReq;

    const INT clines = pcsr->Cpage().Clines();
    for ( INT iline = 0; iline < clines; iline++ )
    {
        KEYDATAFLAGS kdf;
        NDIGetKeydataflags( pcsr->Cpage(), iline, &kdf );

        if( FBTINodeCommittedDelete( pfucb, kdf ) )
        {
            const ULONG cbSavings = CbNDNodeSizeCompressed( kdf );
            if( cbSavings >= cbReqActual )
            {
                return fTrue;
            }
            cbReqActual -= cbSavings;
        }
    }

    return FNDFreePageSpace( pfucb, pcsr, cbReqActual );
}

//  check if remaining nodes in merged page fit in the destination page
//      without violating density constraint
//
BOOL FBTISPCCheckMergeable( FUCB *pfucb, CSR *pcsrDest, LINEINFO *rglineinfo )
{
    const INT   clines = Pcsr( pfucb )->Cpage().Clines();

    Assert( Pcsr( pfucb )->FLatched() );
    Assert( pcsrDest->FLatched() );
    Assert( pcsrDest->Cpage().FLeafPage() );
    Assert( Pcsr( pfucb )->Cpage().FLeafPage() );

    //  calculate total size, total max size and prefixes of nodes to move
    //
    INT     cbSizeTotal = 0;
    INT     cbSizeMaxTotal = 0;
    INT     cbSavings = 0;
    INT     iline;

    NDGetPrefix( pfucb, pcsrDest );

    for ( iline = 0; iline < clines; iline++ )
    {
        LINEINFO    *plineinfo = &rglineinfo[iline];

        if ( FNDDeleted( plineinfo->kdf ) && !plineinfo->fVerActive )
        {
            //  this node will not be moved
            //
            continue;
        }

        //  calculate cbPrefix for node
        //
        INT     cbCommon = CbCommonKey( pfucb->kdfCurr.key,
                                        plineinfo->kdf.key );
        if ( cbCommon > cbPrefixOverhead )
        {
            plineinfo->cbPrefix = cbCommon;
            cbSavings += cbCommon - cbPrefixOverhead;
        }
        else
        {
            plineinfo->cbPrefix = 0;
        }

        //  add cbSize and cbSizeMax
        //
        cbSizeTotal     += plineinfo->cbSize;
        cbSizeMaxTotal  += plineinfo->cbSizeMax;
    }

    //  moving nodes should not violate density constraint [assuming no rollbacks]
    //  and moving nodes should still allow rollbacks to succeed
    //
    Assert( pcsrDest->Pgno() != pgnoNull );
    const INT       cbReq       = ( cbSizeMaxTotal - cbSavings )
                                    + ( Pcsr( pfucb )->Cpage().FLeafPage() ? CbBTIFreeDensity( pfucb ) : 0 );
    const BOOL      fMergeable  = FBTIFreePageSpaceForMerge( pfucb, pcsrDest, cbReq );

    return fMergeable;
}

//  check if last node in internal page is null-keyed
//
INLINE BOOL FBTINullKeyedLastNode( FUCB *pfucb, MERGEPATH *pmergePath )
{
    CSR     *pcsr = &pmergePath->csr;

    Assert( !pcsr->Cpage().FLeafPage() );

    pcsr->SetILine( pmergePath->iLine );
    NDGet( pfucb, pcsr );
    Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );

    //  cannot be null if not last node
    Assert( pcsr->Cpage().Clines() - 1 == pmergePath->iLine
        || !pfucb->kdfCurr.key.FNull() );

    return pfucb->kdfCurr.key.FNull();
}

//  checks if internal pages are emptied because of page merge/deletion
//  also checks if internal page must and can adjust page-pointer key
//
LOCAL ERR ErrBTISelectMergeInternalPages( FUCB * const pfucb, MERGEPATH * const pmergePathLeaf )
{
    ERR         err;
    MERGEPATH   * pmergePath        = pmergePathLeaf->pmergePathParent;
    MERGE       * const pmergeLeaf  = pmergePathLeaf->pmerge;
    const BOOL  fLeftSibling        = pgnoNull != pmergeLeaf->csrLeft.Pgno();
    const BOOL  fRightSibling       = pgnoNull != pmergeLeaf->csrRight.Pgno();
    BOOL        fKeyChange          = fFalse;

    //  check input
    //
    Assert( mergetypeNone != pmergeLeaf->mergetype );
    Assert( pmergePath != NULL );
    Assert( pmergePath->pmergePathChild );
    Assert( !pmergePath->csr.Cpage().FLeafPage() );
    Assert( fRightSibling ||
            pmergePath->csr.Cpage().Clines() - 1 == pmergePath->csr.ILine() );
    Assert( fLeftSibling ||
            0 == pmergePath->csr.ILine() );
    Assert( fRightSibling ||
            !fLeftSibling ||
            pmergePath->csr.ILine() > 0 );

    //  set flag to empty leaf page
    //
    Assert( !pmergePathLeaf->fEmptyPage );
    if ( mergetypePartialRight != pmergeLeaf->mergetype && mergetypePartialLeft != pmergeLeaf->mergetype )
    {
        pmergePathLeaf->fEmptyPage = fTrue;
    }

    for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathParent )
    {
        Assert( pmergePath->pmerge == NULL );
        Assert( !pmergePath->csr.Cpage().FLeafPage() );
        Assert( !pmergePath->fKeyChange );
        Assert( !pmergePath->fEmptyPage );
        Assert( !pmergePath->fDeleteNode );
        Assert( pmergePath->iLine == pmergePath->csr.ILine() );

        if ( mergetypePartialRight == pmergeLeaf->mergetype )
        {
            MERGEPATH * const pmergePathChild = pmergePath->pmergePathChild;
            Assert( NULL != pmergePathChild );

            const BOOL fParentOfLeaf = ( NULL == pmergePathChild->pmergePathChild );
            if( fParentOfLeaf )
            {
                const INT       ilineMerge              = pmergeLeaf->ilineMerge;
                const KEYDATAFLAGS * const pkdfMerge    = &pmergeLeaf->rglineinfo[ilineMerge].kdf;
                const KEYDATAFLAGS * const pkdfPrev     = &pmergeLeaf->rglineinfo[ilineMerge - 1].kdf;

                //  allocate key separator
                //
                Assert( ilineMerge > 0 );
                Assert( !pmergeLeaf->fAllocParentSep );
                CallR( ErrBTIComputeSeparatorKey( pfucb,
                                                  *pkdfPrev,
                                                  *pkdfMerge,
                                                  &pmergeLeaf->kdfParentSep.key ) );

                pmergeLeaf->kdfParentSep.data.SetCb( sizeof( PGNO ) );
                pmergeLeaf->fAllocParentSep = fTrue;
            }

            //  if parent of leaf
            //    or last node in child is changing key
            //      change key at level
            //  else break
            if ( fParentOfLeaf || pmergePathChild->iLine == pmergePathChild->csr.Cpage().Clines() - 1  )
            {
                Assert( pmergeLeaf->fAllocParentSep );

                if ( FBTIOverflowOnReplacingKey( pfucb, pmergePath, pmergeLeaf->kdfParentSep ) )
                {
                    goto Overflow;
                }

                pmergePath->fKeyChange = fTrue;
                continue;
            }

            break;
        }

        if ( mergetypePartialLeft == pmergeLeaf->mergetype )
        {
            const MERGEPATH * const pmergePathChild = pmergePath->pmergePathChild;
            Assert( NULL != pmergePathChild );

            // this loop works from the bottom up so the parent-of-leaf level should
            // have caused the loop to exit at the 'break' below
            const BOOL fParentOfLeaf = ( NULL == pmergePathChild->pmergePathChild );
            Assert( fParentOfLeaf );

            // all nodes up to and including ilineMerge will move to the left page
            // nodes from ilineMerge+1 to the end will stay             
            const INT ilineMerge  = pmergeLeaf->ilineMerge;
            Assert( ilineMerge < ( pmergeLeaf->clines - 1 ) );

            const KEYDATAFLAGS * const pkdfMerge    = &pmergeLeaf->rglineinfo[ilineMerge].kdf;
            const KEYDATAFLAGS * const pkdfNext     = &pmergeLeaf->rglineinfo[ilineMerge + 1].kdf;

            //  allocate key separator
            Assert( ilineMerge >= 0 );
            Assert( !pmergeLeaf->fAllocParentSep );
            CallR( ErrBTIComputeSeparatorKey( pfucb,
                                              *pkdfMerge,
                                              *pkdfNext,
                                              &pmergeLeaf->kdfParentSep.key ) );
            pmergeLeaf->kdfParentSep.data.SetCb( sizeof( PGNO ) );
            pmergeLeaf->fAllocParentSep = fTrue;

            // Nodes are moving from the start of the merged page to the end
            // of the left page. That means the separator key for the merged
            // page doesn't change (if the node with the highest key moved
            // it would be a full left merge, not a partial left merge). 
            // On the other hand, the highest key on the left page is changing
            // so the separator key for the pointer to the left page has
            // to be updated.
            //
            // ErrBTIMergeCollectPageInfo doesn't allow a left merge to happen
            // if the parent pointer is the first in the page so the pointer to
            // the left page will be on the same parent-of-leaf page as the
            // pointer to the merged page.
            //
            // When the merge is performed the correct node will be updated by
            // BTIPerformOneMerge.
            //

            Assert( pmergePath->iLine > 0 );
            if ( FBTIOverflowOnReplacingKey( pfucb, &pmergePath->csr, pmergePath->iLine-1, pmergeLeaf->kdfParentSep ) )
            {
                goto Overflow;
            }

            pmergePath->fKeyChange = fTrue;

            // A partial left merge is only done if both parent-of-leaf nodes are in the same
            // page. The node before the pointer to the merged page will have its key changed.
            // As that node is guaranteed not to be the last key on the page the key change
            // will not propagate up the tree.
            break;
        }

        if ( mergetypeFullLeft == pmergeLeaf->mergetype )
        {
            const MERGEPATH * const pmergePathChild = pmergePath->pmergePathChild;
            Assert( NULL != pmergePathChild );

            // this loop works from the bottom up so the parent-of-leaf level should
            // have caused the loop to exit at the 'break' below
            const BOOL fParentOfLeaf = ( NULL == pmergePathChild->pmergePathChild );
            Assert( fParentOfLeaf );

            // Updating the parent-of-leaf level when a full left merge is done requires
            // two operations.
            //  - the node that points to the merged page has to be deleted
            //  - the previous node has to have its separator key changed
            //
            // All the nodes moved off the merged page are moved onto the end of the
            // left page so the separator key of the left page will be the separator
            // key of the page which is being deleted.
            //          
            // ErrBTIMergeCollectPageInfo doesn't allow a left merge to happen
            // if the parent pointer is the first in the page so the pointer to
            // the left page will be on the same parent-of-leaf page as the
            // pointer to the merged page.
            //
            // Set fKeyChange and fDeleteNode. BTIPerformOneMerge recognizes this case
            // and handles it correctly.

            Assert( pmergePath->iLine > 0 );

            pmergePath->fKeyChange = fTrue;
            pmergePath->fDeleteNode = fTrue;

            // A full left merge is only done if both parent-of-leaf nodes are in the same
            // page. After the merge there will still be a node with the same separator key
            // as the deleted node so these changes won't propagate up the tree.
            break;
        }

        if ( pmergePath->csr.Cpage().Clines() == 1 )
        {
            //  only node in page
            //  whole page will be deleted
            //
            Assert( pmergePath->csr.ILine() == 0 );

            if ( pmergePath->csr.Cpage().FRootPage() )
            {
                //  UNDONE: fix this by deleting page pointer in root,
                //          releasing all pages except root and
                //          setting root to be a leaf page
                //
                //  we can't free root page -- so punt empty page operation
                //
                Assert( fFalse );   //  should now be handled by mergetypeEmptyTree/
                Assert( mergetypeEmptyPage == pmergeLeaf->mergetype );

                goto Overflow;
            }

            //  can only delete this page if child page is deleted as well
            if ( pmergePath->pmergePathChild->fEmptyPage )
            {
                Assert( pmergePath->pmergePathChild->csr.Cpage().FLeafPage()
                        || ( 0 == pmergePath->pmergePathChild->csr.ILine()
                            && 1 == pmergePath->pmergePathChild->csr.Cpage().Clines() ) );
                pmergePath->fEmptyPage = fTrue;
            }
            else
            {
                //  this code path is a very specialised case -- there is only one page pointer
                //  in this page, and in a non-leaf child page, the page pointer with
                //  the largest key was deleted (and the key was non-null), necessitating a key
                //  change in the key of the sole page pointer in this page
#ifdef DEBUG
                Assert( fKeyChange );
                MERGEPATH *pMergePathChild = pmergePath->pmergePathChild;
                while ( pMergePathChild != NULL &&
                        !pMergePathChild->fDeleteNode )
                {
                    Assert( pMergePathChild->fKeyChange );
                    Assert( pMergePathChild->iLine == pMergePathChild->csr.Cpage().Clines() - 1 );
                    Assert( !pMergePathChild->csr.Cpage().FLeafPage() );

                    pMergePathChild = pMergePathChild->pmergePathChild;
                }
                Assert( pMergePathChild != NULL );
                Assert( pMergePathChild->fDeleteNode );
                Assert( pMergePathChild->iLine == pMergePathChild->csr.Cpage().Clines() - 1 );
                Assert( !pMergePathChild->csr.Cpage().FLeafPage() );
#endif
                if ( FBTIOverflowOnReplacingKey(
                            pfucb,
                            pmergePath,
                            pmergeLeaf->kdfParentSep ) )
                {
                    goto Overflow;
                }

                pmergePath->fKeyChange = fTrue;
            }

            continue;
        }

        Assert( pmergePath->csr.ILine() == pmergePath->iLine );
        if ( pmergePath->csr.Cpage().Clines() - 1 == pmergePath->iLine )
        {
            //  delete largest parent pointer node in page
            //      replace current last node in page with new separator key
            //
            Assert( fLeftSibling );

            if ( !fKeyChange )
            {
                //  allocate and compute separator key from leaf level
                //
                Assert( !pmergePathLeaf->pmerge->fAllocParentSep );

                fKeyChange = fTrue;
                CallR( ErrBTIMergeCopySeparatorKey( pmergePath,
                                                    pmergePathLeaf->pmerge,
                                                    pfucb ) );
                Assert( pmergePathLeaf->pmerge->fAllocParentSep );

                pmergePath->fDeleteNode = fTrue;

                Assert( pmergePath->csr.Cpage().Clines() > 1 );
                if ( FBTINullKeyedLastNode( pfucb, pmergePath ) )
                {
                    //  parent pointer is also null-keyed
                    //
                    Assert( NULL == pmergePath->pmergePathParent ||
                            pmergePath->pmergePathParent->csr.Cpage().Clines() - 1 ==
                                pmergePath->pmergePathParent->iLine &&
                            FBTINullKeyedLastNode( pfucb,
                                                   pmergePath->pmergePathParent ) );
                    break;
                }
            }
            else
            {
                if ( FBTIOverflowOnReplacingKey( pfucb,
                                                 pmergePath,
                                                 pmergeLeaf->kdfParentSep ) )
                {
                    goto Overflow;
                }

                pmergePath->fKeyChange = fTrue;
            }

            continue;
        }

        if ( pmergePath->pmergePathChild->fKeyChange ||
             ( pmergePath->pmergePathChild->fDeleteNode &&
               pmergePath->pmergePathChild->iLine ==
               pmergePath->pmergePathChild->csr.Cpage().Clines() - 1 ) )
        {
            //  change key of page pointer in this page
            //  since largest key in child page has changed
            //
            Assert( !pmergePath->pmergePathChild->csr.Cpage().FLeafPage() );
            Assert( pmergePath->pmergePathChild->iLine ==
                    pmergePath->pmergePathChild->csr.Cpage().Clines() - 1 );
            Assert( fKeyChange );

            if ( FBTIOverflowOnReplacingKey( pfucb,
                                             pmergePath,
                                             pmergeLeaf->kdfParentSep ) )
            {
                goto Overflow;
            }

            pmergePath->fKeyChange = fTrue;
        }
        else if ( pmergePath->pmergePathChild->fEmptyPage )
        {
            //  parent of merged or emptied page
            //
            Assert( pmergePath->iLine != pmergePath->csr.Cpage().Clines() - 1 );
            Assert( pmergePath->csr.Cpage().Clines() > 1 );

            pmergePath->fDeleteNode = fTrue;
        }

        break;
    }

    return JET_errSuccess;

Overflow:
    pmergePathLeaf->pmerge->mergetype = mergetypeNone;
    return JET_errSuccess;
}



//  does replacing the key in node pmergePath->iLine of page
//  cause a page overflow?
//
LOCAL BOOL FBTIOverflowOnReplacingKey(  FUCB                    *pfucb,
                                        MERGEPATH               *pmergePath,
                                        const KEYDATAFLAGS&     kdfSep )
{
    return FBTIOverflowOnReplacingKey( pfucb, &pmergePath->csr, pmergePath->iLine, kdfSep );
}

//  does replacing the key in node iline of the csr
//  cause a page overflow?
//
LOCAL BOOL FBTIOverflowOnReplacingKey(  FUCB                    *pfucb,
                                        CSR * const             pcsr,
                                        const INT               iLine,
                                        const KEYDATAFLAGS&     kdfSep )
{
    Assert( !kdfSep.key.FNull() );
    Assert( !pcsr->Cpage().FLeafPage() );

    //  calculate required space for separator with current prefix
    //
    ULONG   cbReq = CbBTICbReq( pfucb, pcsr, kdfSep );

    //  get last node in page
    //
    pcsr->SetILine( iLine );
    NDGet( pfucb, pcsr );
    Assert( !FNDVersion( pfucb->kdfCurr ) );

    ULONG   cbSizeCurr = CbNDNodeSizeCompressed( pfucb->kdfCurr );

    //  check if new separator key would cause overflow
    //
    if ( cbReq > cbSizeCurr )
    {
        const BOOL  fOverflow = FBTISplit( pfucb, pcsr, cbReq - cbSizeCurr );

        return fOverflow;
    }

    return fFalse;
}


//  UNDONE: we can do without the allocation and copy by
//          copying kdfCurr into kdfParentSep
//          and ordering merge/empty page operations top-down
//
//  copies new page separator key from last - 1 node in current page
//
ERR ErrBTIMergeCopySeparatorKey( MERGEPATH  *pmergePath,
                                 MERGE      *pmergeLeaf,
                                 FUCB       *pfucb )
{
    Assert( NULL != pmergeLeaf );
    Assert( mergetypePartialRight != pmergeLeaf->mergetype );
    Assert( pmergeLeaf->kdfParentSep.key.FNull() );
    Assert( pmergeLeaf->kdfParentSep.data.FNull() );

    Assert( !pmergePath->csr.Cpage().FLeafPage() );
    Assert( pmergePath->iLine == pmergePath->csr.Cpage().Clines() - 1 );
    Assert( pmergePath->iLine > 0 );

    pmergePath->csr.SetILine( pmergePath->iLine - 1 );
    NDGet( pfucb, &pmergePath->csr );
    Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );

    pmergeLeaf->kdfParentSep.key.Nullify();
    pmergeLeaf->kdfParentSep.key.suffix.SetPv( RESBOOKMARK.PvRESAlloc() );
    if ( pmergeLeaf->kdfParentSep.key.suffix.Pv() == NULL )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    //  copy separator key into alocated memory
    //
    pfucb->kdfCurr.key.CopyIntoBuffer( pmergeLeaf->kdfParentSep.key.suffix.Pv() );
    pmergeLeaf->kdfParentSep.key.suffix.SetCb( pfucb->kdfCurr.key.Cb() );

    Assert( !pmergeLeaf->fAllocParentSep );
    Assert( !pmergeLeaf->kdfParentSep.key.FNull() );
    pmergeLeaf->fAllocParentSep = fTrue;

    //  page pointer should have pgno as data
    //
    pmergeLeaf->kdfParentSep.data.SetCb( sizeof( PGNO ) );
    return JET_errSuccess;
}


//  from leaf to root
//
VOID BTIReleaseMergePaths( MERGEPATH *pmergePathTip )
{
    MERGEPATH *pmergePath = pmergePathTip;

    for ( ; pmergePath != NULL; )
    {
        //  save parent
        //
        MERGEPATH *pmergePathParent = pmergePath->pmergePathParent;

        delete pmergePath;
        pmergePath = pmergePathParent;
    }
}

//  performs merge by going through pages top-down
//
ERR ErrBTIMergeOrEmptyPage( FUCB *pfucb, MERGEPATH *pmergePathLeaf )
{
    ERR         err = JET_errSuccess;
    MERGE       *pmerge = pmergePathLeaf->pmerge;
    LGPOS       lgpos;

    Assert( NULL != pmerge );
    Assert( mergetypeNone != pmerge->mergetype );

    //  upgrade latches
    //
    CallR( ErrBTIMergeUpgradeLatches( pfucb->ifmp, pmergePathLeaf ) );

    //  log merge operation as a multi-page operation
    //  there can be no failures after this
    //      till space release operations
    //
    err = ErrLGMerge( pfucb, pmergePathLeaf, &lgpos );

    // on error, return to before dirty dbtime on all pages
    if ( JET_errSuccess > err )
    {
        BTIMergeRevertDbtime( pmergePathLeaf );
    }
    CallR ( err );

    BTIMergeSetLgpos( pmergePathLeaf, lgpos );

    BTIPerformMerge( pfucb, pmergePathLeaf );

    //  check if the merge performed is correct
    //
    BTICheckMerge( pfucb, pmergePathLeaf );

    //  shrink / optimize page image
    //
    BTIMergeShrinkPages( pmergePathLeaf );

    //  release all latches
    //  so space can latch root successfully
    //
    BTIMergeReleaseLatches( pmergePathLeaf );

    //  release empty pages -- ignores errors
    //
    BTIReleaseEmptyPages( pfucb, pmergePathLeaf );

    return err;
}


//  ================================================================
LOCAL VOID BTICheckPagePointer(
    _In_ const CSR& csr,
    _In_ const PGNO pgno )
//  ================================================================
//
// given an internal page make sure the node pointed to by the current
// iline has the given pgno
//
//-
{
    Assert( csr.FLatched() );
    Assert( csr.Cpage().FInvisibleSons() );

    KEYDATAFLAGS kdf;
    NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdf );

    Assert( sizeof( PGNO ) == kdf.data.Cb() );
    const PGNO pgnoCurr = *reinterpret_cast<const UnalignedLittleEndian<ULONG> *>( kdf.data.Pv() );
    Assert( pgno == pgnoCurr );
}

//  ================================================================
LOCAL VOID BTIUpdatePagePointer(
    _In_ CSR * const pcsr,
    _In_ const PGNO pgnoNew )
//  ================================================================
//
// given an internal page change the node pointed to by the current
// iline to have the given pgno
//
//-
{
    Assert( pcsr );
    Assert( pcsr->FLatched() );
    Assert( pcsr->Cpage().FInvisibleSons() );

    DATA data;
    data.SetPv( const_cast<PGNO*>( &pgnoNew ) );
    data.SetCb( sizeof( pgnoNew ) );

    NDReplace( pcsr, &data );
    BTICheckPagePointer( *pcsr, pgnoNew );
}

//  ================================================================
LOCAL VOID BTIMovePageCopyNextBookmark(
            _In_        const FUCB * const pfucb,
            _In_        const MERGEPATH * const pmergePath,
            __inout     BOOKMARK * const pbmNext )
//  ================================================================
//
// copies the bookmark from the right-hand page for a move
// the key.suffix.Pv() of the bookmark that is passed in must point to a buffer
// big enough to hold the largest bookmark possible
//
//-
{
    Assert( pfucb );
    Assert( pmergePath );
    Assert( NULL != pmergePath->pmerge );
    Assert( pbmNext );
    Assert( NULL != pbmNext->key.suffix.Pv() );

    if( pgnoNull == pmergePath->pmerge->csrRight.Pgno())
    {
        pbmNext->key.prefix.SetCb( 0 );
        pbmNext->key.suffix.SetCb( 0 );
        pbmNext->data.SetCb( 0 );
        Assert( pbmNext->key.FNull() );
    }
    else
    {
        Assert( pmergePath->pmerge->csrRight.FLatched() );
        Assert( pmergePath->pmerge->csrRight.Cpage().Clines() > 0 );

        BOOKMARK bm;
        KEYDATAFLAGS kdf;

        const INT iline = pmergePath->pmerge->csrRight.Cpage().Clines() - 1;
        NDIGetKeydataflags( pmergePath->pmerge->csrRight.Cpage(), iline, &kdf );
        NDGetBookmarkFromKDF( pfucb, kdf, &bm );

        //  copy bm into given buffer
        bm.key.CopyIntoBuffer( pbmNext->key.suffix.Pv(), bm.key.Cb() );
        pbmNext->key.suffix.SetCb( bm.key.Cb() );

        pbmNext->data.SetPv( reinterpret_cast<BYTE *>( pbmNext->key.suffix.Pv() ) + pbmNext->key.Cb() );
        bm.data.CopyInto( pbmNext->data );

        Assert( pbmNext->key.prefix.FNull() );
        Assert( !pbmNext->key.suffix.FNull() );
        Assert( !pbmNext->key.FNull() );
    }
}

//  ================================================================
LOCAL VOID BTIMergeSetPcsrRoot(
    _In_ FUCB * const pfucb,
    _In_ MERGEPATH * const pmergePathTip )
//  ================================================================
{
    Assert( pfucb );
    Assert( pcsrNil == pfucb->pcsrRoot );
    Assert( pmergePathTip );

    MERGEPATH * pmergePath;
    for ( pmergePath = pmergePathTip;
          pmergePath->pmergePathParent != NULL;
          pmergePath = pmergePath->pmergePathParent )
    {
    }
    pfucb->pcsrRoot = &(pmergePath->csr);

    Assert( pcsrNil != pfucb->pcsrRoot );
}

//  ================================================================
LOCAL ERR ErrBTIPageMoveAllocatePage(
    _In_        FUCB * const        pfucb,
    _In_        MERGEPATH * const   pmergePath,
    _In_        ULONG               fSPAllocFlags,
    _In_        const DIRFLAG       dirflag )
//  ================================================================
{
    Assert( pfucb );
    Assert( NULL == pfucb->pcsrRoot );
    Assert( pmergePath );
    Assert( NULL != pmergePath->pmerge );
    Assert( mergetypePageMove == pmergePath->pmerge->mergetype );
    Assert( NULL != pmergePath->pmergePathParent );

    Assert( pgnoNull == pmergePath->pmerge->pgnoNew );
    Assert( pgnoNull == pmergePath->pmerge->csrNew.Pgno() );
    Assert( !pmergePath->pmerge->csrNew.FLatched() );

    ERR err;
    const BOOL fLogging = !( dirflag & fDIRNoLog ) && g_rgfmp[pfucb->ifmp].FLogOn();

    BTIMergeSetPcsrRoot( pfucb, pmergePath );

    // if there is no left page and we are at the leaf level,
    // then pgnoNew will be pgnoNull and a new extent will be allocated
    // if we are moving a space page, we'll get it from the space tree's split buffer, so
    // we don't need that special flag
    if( pmergePath->csr.Cpage().FLeafPage() &&
        ( pgnoNull == pmergePath->pmerge->csrLeft.Pgno() ) &&
        !FFUCBSpace( pfucb ) )
    {
        fSPAllocFlags |= fSPNewExtent;
    }

    Call( ErrSPGetPage( pfucb,
            pmergePath->pmerge->csrLeft.Pgno(),
            fSPAllocFlags,
            &pmergePath->pmerge->pgnoNew ) );

    Call( pmergePath->pmerge->csrNew.ErrGetNewPreInitPage(
            pfucb->ppib,
            pfucb->ifmp,
            pmergePath->pmerge->pgnoNew,
            ObjidFDP( pfucb ),
            fLogging ) );

    Assert( pgnoNull != pmergePath->pmerge->pgnoNew );
    Assert( pmergePath->pmerge->pgnoNew == pmergePath->pmerge->csrNew.Pgno() );
    Assert( pmergePath->pmerge->csrNew.FLatched() );

    //  count pages allocated by an update
    //
    Ptls()->threadstats.cPageUpdateAllocated++;

HandleError:
    pfucb->pcsrRoot = NULL;
    return err;
}

//  ================================================================
LOCAL VOID BTICheckPageMove(
    const CSR& csrParent,
    const CSR& csrSrc,
    const CSR& csrDest,
    const CSR& csrLeft,
    const CSR& csrRight,
    const BOOL fBeforeMove )
//  ================================================================
{
    const BOOL fLeafPage    = csrSrc.Cpage().FLeafPage();
    const OBJID objidFDP    = csrSrc.Cpage().ObjidFDP();

    // source and destination pages must be consistent themselves and consistent
    // with each other
    Assert( !csrSrc.Cpage().FPreInitPage() );
    Assert( csrSrc.Cpage().FFlags() != 0 );
    Assert( csrSrc.Cpage().ObjidFDP() != objidNil ) ;
    Assert( csrSrc.Pgno() != pgnoNull );
    Assert( csrSrc.Pgno() == csrSrc.Cpage().PgnoThis() );
    Assert( csrDest.Cpage().ObjidFDP() == csrSrc.Cpage().ObjidFDP() ) ;
    Assert( csrDest.Pgno() != pgnoNull );
    Assert( csrDest.Pgno() == csrDest.Cpage().PgnoThis() );
    Assert( csrDest.Pgno() != csrSrc.Pgno() );
    if ( fBeforeMove )
    {
        Assert( csrDest.Cpage().FPreInitPage() );
        Assert( csrDest.Cpage().PgnoPrev() == pgnoNull );
        Assert( csrDest.Cpage().PgnoNext() == pgnoNull );
    }
    else
    {
        Assert( !csrDest.Cpage().FPreInitPage() );
        Assert( csrSrc.Cpage().FEmptyPage() );
        Assert( ( csrSrc.Cpage().FFlags() & ~CPAGE::fPageEmpty & ~CPAGE::maskFlushType & ~CPAGE::fPageNewChecksumFormat ) ==
                ( csrDest.Cpage().FFlags() & ~CPAGE::maskFlushType & ~CPAGE::fPageNewChecksumFormat ) );
    }

    const CSR* const pcsrCurrent = fBeforeMove ? &csrSrc : &csrDest;

    // the parent should point to the destination page
    Assert( !fLeafPage || csrParent.Cpage().FParentOfLeaf() );
    Assert( objidFDP == csrParent.Cpage().ObjidFDP() );
    BTICheckPagePointer( csrParent, pcsrCurrent->Pgno() );

    // left page
    if( pgnoNull != csrLeft.Pgno() )
    {
        Assert( fLeafPage );
        Assert( csrLeft.Cpage().FLeafPage() );
        Assert( objidFDP == csrLeft.Cpage().ObjidFDP() );
        Assert( csrLeft.Pgno() == pcsrCurrent->Cpage().PgnoPrev() );
        Assert( csrLeft.Cpage().PgnoNext() == pcsrCurrent->Pgno() );
    }
    else
    {
        Assert( pgnoNull == pcsrCurrent->Cpage().PgnoPrev() );
    }

    // right page
    if( pgnoNull != csrRight.Pgno() )
    {
        Assert( fLeafPage );
        Assert( csrRight.Cpage().FLeafPage() );
        Assert( objidFDP == csrRight.Cpage().ObjidFDP() );
        Assert( csrRight.Pgno() == pcsrCurrent->Cpage().PgnoNext() );
        Assert( csrRight.Cpage().PgnoPrev() == pcsrCurrent->Pgno() );
    }
    else
    {
        Assert( pgnoNull == pcsrCurrent->Cpage().PgnoNext() );
    }
}

//  ================================================================
VOID BTPerformPageMove( _In_ MERGEPATH * const pmergePath )
//  ================================================================
//
// the operation is logged and the pages are dirtied. all that remains is to do the move
// there are 4 things that have to happen:
//  - copy data from the source page to the destination
//  - update the page pointer in the parent page
//  - update the pgnoNext member of the left page
//  - update the pgnoPrev member of the right page
//
// this function is used at recovery time to redo page moves. in that case only the
// pages which need to be redone will be write latched
//
//-
{
    Assert( pmergePath );
    Assert( NULL != pmergePath->pmerge );
    Assert( mergetypePageMove == pmergePath->pmerge->mergetype );
    Assert( NULL != pmergePath->pmergePathParent );

    const PGNO pgnoOld = pmergePath->csr.Pgno();
    const PGNO pgnoNew = pmergePath->pmerge->csrNew.Pgno();
    const DBTIME dbtimeNew = pmergePath->pmerge->csrNew.Dbtime();

    // copy the data
    if( FBTIUpdatablePage( pmergePath->pmerge->csrNew ) )
    {
        Assert( pmergePath->csr.Cpage().CbPage() == pmergePath->pmerge->csrNew.Cpage().CbPage() );
        pmergePath->pmerge->csrNew.Cpage().CopyPage(
            pmergePath->csr.Cpage().PvBuffer(),             // source
            pmergePath->csr.Cpage().CbPage() );             // length

        pmergePath->pmerge->csrNew.FinalizePreInitPage();

        Assert( pmergePath->pmerge->csrNew.Cpage().PgnoThis() == pgnoNew );
        Assert( pmergePath->pmerge->csrNew.Dbtime() == dbtimeNew );
    }

    // set the source page as empty
    if( FBTIUpdatablePage( pmergePath->csr ) )
    {
        NDEmptyPage( &(pmergePath->csr) );
    }

    // page pointer in parent page
    if( FBTIUpdatablePage( pmergePath->pmergePathParent->csr ) )
    {
        BTICheckPagePointer( pmergePath->pmergePathParent->csr, pgnoOld );
        BTIUpdatePagePointer( &(pmergePath->pmergePathParent->csr), pgnoNew );
    }

    // left page
    if( pgnoNull != pmergePath->pmerge->csrLeft.Pgno() && FBTIUpdatablePage( pmergePath->pmerge->csrLeft ) )
    {
        Assert( pmergePath->pmerge->csrLeft.Cpage().PgnoNext() == pgnoOld );
        pmergePath->pmerge->csrLeft.Cpage().SetPgnoNext( pgnoNew );
    }

    // right page
    if( pgnoNull != pmergePath->pmerge->csrRight.Pgno() && FBTIUpdatablePage( pmergePath->pmerge->csrRight ) )
    {
        Assert( pmergePath->pmerge->csrRight.Cpage().PgnoPrev() == pgnoOld );
        pmergePath->pmerge->csrRight.Cpage().SetPgnoPrev( pgnoNew );
    }
}

//  ================================================================
LOCAL ERR ErrBTIPageMove(
    _In_ FUCB * const       pfucb,
    _In_ MERGEPATH * const  pmergePath,
    _In_ const DIRFLAG      dirflag,
    _In_ const ULONG        fSPAllocFlags )
//  ================================================================
//
// This function takes a mergePath and performs the merge.
//  - Latch the sibling pages
//  - Allocate the new page
//  - Upgrade the latches
//  - Log the move
//  - Perform the move
//
//-
{
    Assert( pfucb );
    Assert( pmergePath );
    Assert( NULL != pmergePath->pmerge );
    Assert( mergetypePageMove == pmergePath->pmerge->mergetype );
    Assert( NULL != pmergePath->pmergePathParent );

    ERR err = JET_errSuccess;

    if ( pmergePath->csr.Cpage().FLeafPage() )
    {
        Call( ErrBTIMergeLatchSiblingPages( pfucb, pmergePath ) );
    }
    Call( ErrBTIPageMoveAllocatePage( pfucb, pmergePath, fSPAllocFlags, dirflag ) );

    BTIMergeReleaseUnneededPages( pmergePath );
    Call( ErrBTIMergeUpgradeLatches( pfucb->ifmp, pmergePath ) );

    const BOOL fLogging = !( dirflag & fDIRNoLog ) && g_rgfmp[pfucb->ifmp].FLogOn();
    if( fLogging )
    {
        LGPOS lgpos;
        err = ErrLGPageMove( pfucb, pmergePath, &lgpos );
        if ( err < JET_errSuccess )
        {
            BTIMergeRevertDbtime( pmergePath );
            Call( err );
        }
        BTIMergeSetLgpos( pmergePath, lgpos );
    }

    // make sure the pages are correct before the move
    BTICheckPageMove(
        pmergePath->pmergePathParent->csr,  // parent
        pmergePath->csr,                    // old page
        pmergePath->pmerge->csrNew,         // new page
        pmergePath->pmerge->csrLeft,        // left page
        pmergePath->pmerge->csrRight,       // right page
        fTrue );                            // fBeforeMove

    BTPerformPageMove( pmergePath );

    // make sure the pages are correct after the move
    BTICheckPageMove(
        pmergePath->pmergePathParent->csr,  // parent
        pmergePath->csr,                    // old page
        pmergePath->pmerge->csrNew,         // new page
        pmergePath->pmerge->csrLeft,        // left page
        pmergePath->pmerge->csrRight,       // right page
        fFalse );                           // fBeforeMove

#ifdef TEST_PAGEMOVE_RECOVERY
    // five pages are involved. set some of them to filthy to change the flush pattern
    static INT testcase = 0;
    if( ( testcase % 500 < 0x20 ) )
    {
        const DBTIME dbtime = pmergePath->csr.Dbtime();
        if( testcase & 0x1 )
        {
            pmergePath->csr.CoordinatedDirty( dbtime, bfdfFilthy );
        }
        if( testcase & 0x2 )
        {
            pmergePath->pmergePathParent->csr.CoordinatedDirty( dbtime, bfdfFilthy );
        }
        if( testcase & 0x4 )
        {
            pmergePath->pmerge->csrNew.CoordinatedDirty( dbtime, bfdfFilthy );
        }
        if( ( testcase & 0x8 ) && ( pgnoNull != pmergePath->pmerge->csrLeft.Pgno() ) )
        {
            pmergePath->pmerge->csrLeft.CoordinatedDirty( dbtime, bfdfFilthy );
        }
        if( ( testcase & 0x10 ) && ( pgnoNull != pmergePath->pmerge->csrRight.Pgno() ) )
        {
            pmergePath->pmerge->csrRight.CoordinatedDirty( dbtime, bfdfFilthy );
        }
    }
    testcase++;
#endif

HandleError:
    return err;
}

//  ================================================================
ERR ErrBTPageMove(
    _In_ FUCB * const pfucb,
    _In_ const BOOKMARK& bm,
    _In_ const PGNO pgnoSource,
    _In_ const BOOL fLeafPage,
    _In_ const ULONG fSPAllocFlags,
    __inout BOOKMARK * const pbmNext )
//  ================================================================
{
    Assert( pfucb );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !bm.key.FNull() || !fLeafPage );
    Assert( fLeafPage || ( pgnoSource != pgnoNull ) );
    Assert( ( pbmNext == NULL ) || fLeafPage );

    ERR err = JET_errSuccess;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTMerge );
    MERGEPATH * pmergePath = NULL;

    // If it's a space page, we may need refill the split buffers first, including
    // reservation for the page we're moving data to.
    if ( FFUCBSpace( pfucb ) )
    {
        Call( ErrSPReserveSPBufPages( pfucb, FFUCBOwnExt( pfucb ), FFUCBAvailExt( pfucb ) ) );
    }

    Call( ErrBTICreateMergePath( pfucb, bm, pgnoSource, fLeafPage, &pmergePath ) );
    if ( wrnBTShallowTree == err )
    {
        if ( pbmNext )
        {
            pbmNext->key.prefix.SetCb( 0 );
            pbmNext->key.suffix.SetCb( 0 );
            pbmNext->data.SetCb( 0 );
        }

        goto HandleError;
    }

    Call( ErrBTINewMerge( pmergePath ) );
    pmergePath->pmerge->mergetype               = mergetypePageMove;
    pmergePath->fEmptyPage                      = fTrue;    // set this so the page will be freed
    pmergePath->pmergePathParent->fKeyChange    = fTrue;    // set this so the parent page will be write-latched

    Call( ErrBTIPageMove(
            pfucb,
            pmergePath,
            fDIRNull,
            fSPAllocFlags ) );

    if ( pbmNext )
    {
        BTIMovePageCopyNextBookmark( pfucb, pmergePath, pbmNext );
    }

    BTIMergeReleaseLatches( pmergePath );
    BTIReleaseEmptyPages( pfucb, pmergePath );

HandleError:
    BTIReleaseMergePaths( pmergePath );
    Assert( !Pcsr( pfucb )->FLatched( ) );
    return err;
}

//  performs merge or empty page operation
//      by calling one-level merge at each level top-down
//
VOID BTIPerformMerge( FUCB *pfucb, MERGEPATH *pmergePathLeaf )
{
    auto tc = TcCurr();

    MERGEPATH   *pmergePath;

    //  go to root
    //  since we need to process pages top-down
    //
    for ( pmergePath = pmergePathLeaf;
          pmergePath->pmergePathParent != NULL;
          pmergePath = pmergePath->pmergePathParent )
    {
    }

    //  process pages top-down
    //
    for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathChild )
    {
        // If this goes off, it means we've started consuming pgnoNew for "regular" (as opposed
        // to page-move) merges. Make sure pgnoNew gets flagged as initialized (do-time) in this function.
        Assert( ( pmergePath->pmerge == NULL ) || ( pmergePath->pmerge->pgnoNew == pgnoNull ) );

        if ( pmergePath->csr.Latch() == latchWrite ||
             PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() )
        {
            BTIPerformOneMerge( pfucb, pmergePath, pmergePathLeaf->pmerge );
            switch ( pmergePath->pmerge ? pmergePath->pmerge->mergetype : mergetypeNone )
            {
                case mergetypeEmptyPage:
                    PERFOpt( PERFIncCounterTable( cBTEmptyPageMerge, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );
                    break;

                case mergetypeFullRight:
                    PERFOpt( PERFIncCounterTable( cBTRightMerge, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );
                    break;

                case mergetypePartialRight:
                    PERFOpt( PERFIncCounterTable( cBTPartialMerge, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );
                    break;

                case mergetypeFullLeft:
                    PERFOpt( PERFIncCounterTable( cBTFullLeftMerge, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );
                    break;

                case mergetypePartialLeft:
                    PERFOpt( PERFIncCounterTable( cBTPartialLeftMerge, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );
                    break;

                default:
                    break;
            }
        }
        else
        {
//          Assert( fFalse );
        }
    }
}

//  processes one page for merge or empty page operation
//  depending on the operation selection in pmergePath->flags
//
VOID BTIPerformOneMerge( FUCB       *pfucb,
                         MERGEPATH  *pmergePath,
                         MERGE      *pmergeLeaf )
{
    CSR     *pcsr = &pmergePath->csr;

    Assert( !FBTIUpdatablePage( *pcsr ) || pcsr->FDirty() );

    Assert( !pmergePath->fKeyChange || !pmergePath->fDeleteNode || mergetypeFullLeft == pmergeLeaf->mergetype );
    Assert( !pmergePath->fEmptyPage ||
            !pmergePath->fKeyChange && !pmergePath->fDeleteNode );
    if( pmergePath->fEmptyPage )
    {
        Assert( mergetypePartialRight != pmergeLeaf->mergetype );
        Assert( mergetypePartialLeft != pmergeLeaf->mergetype );
    }

    //  if leaf page,
    //      fix all flag deleted versions of nodes in page to operNull
    //      if merge,
    //          move nodes to right page
    //      if not partial merge
    //          fix siblings
    //
    if ( NULL == pmergePath->pmergePathChild )
    {
        Assert( pmergePath->pmerge == pmergeLeaf );
        const MERGETYPE mergetype = pmergeLeaf->mergetype;

        Assert( mergetype != mergetypeNone );
        Assert( !FBTIUpdatablePage( *pcsr ) || pcsr->Cpage().FLeafPage() );

        if ( mergetypeEmptyPage != mergetype )
        {
            BTIMergeMoveNodes( pfucb, pmergePath );
        }

        //  delete flag deleted nodes that have no active version
        //  if there is a version nullify it
        //
        BTIMergeDeleteFlagDeletedNodes( pfucb, pmergePath );

        if ( mergetypePartialRight != mergetype && mergetypePartialLeft != mergetype )
        {
            //  fix siblings
            BTIMergeFixSiblings( PinstFromIfmp( pfucb->ifmp ), pmergePath );
        }
    }

    //  if page not write latched [no redo needed]
    //      do nothing
    else if ( !FBTIUpdatablePage( *pcsr ) )
    {
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() );
    }

    //  if a full left merge is being done then both a node deletion
    //  and a key change are required. handle those here.
    else if ( mergetypeFullLeft == pmergeLeaf->mergetype
            && pmergePath->fDeleteNode
            && pmergePath->fKeyChange )
    {
        // the parent-of-leaf page will look like this:
        // (page 2 is being merged onto page 1)
        //
        // NODE | SEPARATOR KEY | DATA
        // -----+---------------+-------
        // N-1  | KeyA          | 1
        // N    | KeyB          | 2
        //
        // After all nodes are moved from page 2 onto page 1
        // the parent-of-leaf page should look like this:
        //
        // NODE | SEPARATOR KEY | DATA
        // -----+---------------+-------
        // N-1  | KeyB          | 1
        //
        // There are two ways this can be done:
        //  1. Delete node N, change the key of node N-1
        //  2. Delete node N-1, change the data of node N
        //
        // The second approach will be taken.

        // These changes don't propagate up the tree so we
        // can assert that this is the parent-of-leaf
        // We can only assert that the child page is a leaf
        // during runtime. During redo time the page may have 
        // been reused.
        Assert( pmergePath->csr.Cpage().FParentOfLeaf() );
        Assert( pmergePath->pmergePathChild );
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() || pmergePath->pmergePathChild->csr.Cpage().FLeafPage() );
        Assert( NULL == pmergePath->pmergePathChild->pmergePathChild );
        Assert( pmergeLeaf == pmergePath->pmergePathChild->pmerge );

        // A full left merge isn't selected if the parent pointer is
        // the first node in the page
        Assert( pmergePath->iLine > 0 );
        pmergePath->csr.SetILine( pmergePath->iLine );

        // Change the data
        const PGNO pgnoChild = pmergePath->pmergePathChild->csr.Pgno();
        const PGNO pgnoLeft = pmergeLeaf->csrLeft.Pgno();

        Assert( pgnoNull != pgnoChild );
        Assert( pgnoNull != pgnoLeft );

        BTICheckPagePointer( pmergePath->csr, pgnoChild );
        BTIUpdatePagePointer( &(pmergePath->csr), pgnoLeft );
        BTICheckPagePointer( pmergePath->csr, pgnoLeft );

        // Delete the previous node
        pmergePath->csr.SetILine( pmergePath->iLine - 1 );
        NDDelete( &(pmergePath->csr) );
    }

    //  if fDeleteNode,
    //      delete node
    //      if page pointer is last node and
    //      there is no right sibling to leaf page,
    //          fix new last key to NULL
    //
    else if ( pmergePath->fDeleteNode )
    {
        Assert( !pmergePath->csr.Cpage().FLeafPage() );
        Assert( pmergePath->csr.Cpage().Clines() > 1 );
        Assert( mergetypePartialRight != pmergeLeaf->mergetype );
        Assert( mergetypePartialLeft != pmergeLeaf->mergetype );
        Assert( mergetypeFullLeft != pmergeLeaf->mergetype );

        const BOOL fFixLastNodeToNull = FBTINullKeyedLastNode( pfucb, pmergePath );

        NDDelete( pcsr );
        if ( fFixLastNodeToNull )
        {
            Assert( pmergeLeaf->mergetype == mergetypeEmptyPage );
            Assert( !FBTIUpdatablePage( pmergeLeaf->pmergePath->csr ) ||
                    pmergeLeaf->pmergePath->csr.Cpage().PgnoNext() == pgnoNull );
            Assert( pcsr->Cpage().Clines() == pmergePath->iLine );
            Assert( pcsr->ILine() > 0 );

            pcsr->DecrementILine();
            KEY keyNull;
            keyNull.Nullify();
            BTIChangeKeyOfPagePointer( pfucb, pcsr, keyNull );
        }
    }

    //  if fKeyChange
    //      change the key of seeked node to new separator
    //
    else if ( pmergePath->fKeyChange )
    {
        Assert( mergetypeFullLeft != pmergeLeaf->mergetype );
        Assert( !pmergeLeaf->kdfParentSep.key.FNull() );

        // Partial left merges require a different iline be updated.
        // The nodes are moved from the beginning of the merged page
        // to the end of the left page so the separator key of the
        // node before the pointer to the merged page has to be 
        // updated.

        if( mergetypePartialLeft == pmergeLeaf->mergetype )
        {
            Assert( pmergePath->iLine > 0 );
            pcsr->SetILine( pmergePath->iLine - 1 );
        }
        else
        {
            pcsr->SetILine( pmergePath->iLine );
        }

        BTIChangeKeyOfPagePointer( pfucb,
                                   pcsr,
                                   pmergeLeaf->kdfParentSep.key );
    }

    else
    {
        Assert( pmergePath->fEmptyPage );
    }

    if ( pmergePath->fEmptyPage &&
         FBTIUpdatablePage( pmergePath->csr ) )
    {
        //  set page flag to Empty
        //
        NDEmptyPage( &(pmergePath->csr) );
    }

    return;
}


//  changes the key of a page pointer node to given key
//
VOID BTIChangeKeyOfPagePointer( FUCB *pfucb, CSR *pcsr, const KEY& key )
{
    Assert( !pcsr->Cpage().FLeafPage() );

    NDGet( pfucb, pcsr );
    Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
    Assert( !pfucb->kdfCurr.key.FNull() );

    LittleEndian<PGNO>  le_pgno = *((UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv() );
    Assert( le_pgno != pgnoNull );

    //  delete node and re-insert with given key
    //
    NDDelete( pcsr );

    KEYDATAFLAGS    kdfInsert;

    kdfInsert.fFlags    = 0;
    kdfInsert.data.SetCb( sizeof( PGNO ) );
    kdfInsert.data.SetPv( &le_pgno );
    kdfInsert.key       = key;

    BTIComputePrefixAndInsert( pfucb, pcsr, kdfInsert );
}


//  release latches on pages that are not required
//
VOID BTIMergeReleaseUnneededPages( MERGEPATH *pmergePathTip )
{
    MERGEPATH   *pmergePath;

    //  go to root
    //  release latches top-down
    //
    for ( pmergePath = pmergePathTip;
          pmergePath->pmergePathParent != NULL;
          pmergePath = pmergePath->pmergePathParent )
    {
    }

    Assert( NULL == pmergePath->pmergePathParent );
    for ( ; NULL != pmergePath;  )
    {

        //  check if page is needed
        //      -- either there is a merge/empty page at this level
        //         or there is a merge/empty page one level below
        //              when we need write latch for deleting page pointer
        //         or there is a key change at this level
        //
        if ( !pmergePath->fKeyChange &&
             !pmergePath->fEmptyPage &&
             !pmergePath->fDeleteNode &&
             pmergePath->pmergePathChild != NULL )
        {
            Assert( NULL == pmergePath->pmergePathParent );

            //  release latch and pmergePath at this level
            //
            MERGEPATH *pmergePathT = pmergePath;

            Assert( !pmergePath->fDeleteNode );
            Assert( !pmergePath->fKeyChange );
            Assert( !pmergePath->fEmptyPage );
            Assert( !pmergePath->csr.Cpage().FLeafPage() );
            Assert( NULL != pmergePath->pmergePathChild );
            if ( mergetypeNone != pmergePathTip->pmerge->mergetype )
            {
                //  parent of merged or emptied page must not be released
                //
                Assert( pmergePath->pmergePathChild->pmergePathChild != NULL );
                Assert( !pmergePath->pmergePathChild->csr.Cpage().FLeafPage() );
            }

            //  pages normally get here with latchRIW, but there are a few known cases in which they
            //  have latchWrite: 1) when a root page's latch gets upgraded so that the tree
            //  can be up-converted from small to large extent format; 2) when allocating a new page
            //  for the merge, we may run ErrSPIImproveLastAlloc(), which also upgrades the root's latch.
            //
            Assert( pmergePath->csr.Latch() == latchRIW || pmergePath->csr.Latch() == latchWrite );
            Expected( pmergePath->csr.Latch() != latchWrite || pmergePath->csr.Cpage().FRootPage() );

            pmergePath = pmergePath->pmergePathChild;

            //  release latches on these pages
            //
            delete pmergePathT;
        }
        else
        {
            pmergePath = pmergePath->pmergePathChild;
        }
    }
}


//  upgrade to write latch on all pages invloved in the merge/emptypage
//
//  note that pmergePathTip is the MERGEPATH object which is closest to the leaf level,
//  but not necessarily at the leaf level, which could happen while moving an internal
//  page during shrink, for example.
//
LOCAL ERR ErrBTIMergeUpgradeLatches( const IFMP ifmp, MERGEPATH * const pmergePathTip )
{
    ERR             err;
    MERGEPATH       *pmergePath;
    const DBTIME    dbtimeMerge     = g_rgfmp[ifmp].DbtimeIncrementAndGet();

    Assert( dbtimeMerge > 1 );
    Assert( PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() != fRecoveringRedo );

    //  go to root
    //  since we need to latch top-down
    //
    for ( pmergePath = pmergePathTip;
          pmergePath->pmergePathParent != NULL;
          pmergePath = pmergePath->pmergePathParent )
    {
    }

    Assert( NULL == pmergePath->pmergePathParent );
    for ( ; NULL != pmergePath;  )
    {
        //  check if write latch is needed
        //      -- either there is a merge/empty page at this level
        //         or there is a merge/empty page one level below
        //              when we need write latch for deleting page pointer
        //         or there is a key change at this level
        //
        MERGE   *pmerge = pmergePath->pmerge;

        if ( pmergePath->fKeyChange ||
             pmergePath->fEmptyPage ||
             pmergePath->fDeleteNode ||
             pmergePath->pmergePathChild == NULL )
        {
            Assert( latchWrite == pmergePath->csr.Latch()
                    || latchRIW == pmergePath->csr.Latch() );

            if ( pmergePath->pmergePathChild == NULL )
            {
                //  tip-level
                //  write latch left, current and right pages in order
                //
                Assert( NULL != pmerge );
                Assert( mergetypeNone != pmerge->mergetype );

                //  Verify invalid/nil dbtimes, and then set to nil.
                //
                Assert( dbtimeInvalid == pmerge->dbtimeLeftBefore || dbtimeNil == pmerge->dbtimeLeftBefore );
                Assert( dbtimeInvalid == pmergePath->dbtimeBefore || dbtimeNil == pmergePath->dbtimeBefore );
                Assert( dbtimeInvalid == pmerge->dbtimeRightBefore || dbtimeNil == pmerge->dbtimeRightBefore );
                pmerge->dbtimeLeftBefore = dbtimeNil;
                pmerge->fFlagsLeftBefore = 0;
                if ( dbtimeInvalid == pmergePath->dbtimeBefore )
                {
                    pmergePath->dbtimeBefore = dbtimeNil;
                    pmergePath->fFlagsBefore = 0;
                }
                pmerge->dbtimeRightBefore = dbtimeNil;
                pmerge->fFlagsRightBefore = 0;

                //  Upgrade all latches involved to write latches.
                //

                if ( pgnoNull != pmerge->csrLeft.Pgno() )
                {
                    Assert( pmergePath->csr.Cpage().FLeafPage() );
                    Assert( pmerge->csrLeft.Cpage().FLeafPage() );

                    pmerge->csrLeft.UpgradeFromRIWLatch();

                    if ( pmerge->csrLeft.Dbtime() >= dbtimeMerge )
                    {
                        FireWall( "MergeLeftDbTimeTooHigh" );
                        OSUHAEmitFailureTag( PinstFromIfmp( ifmp ), HaDbFailureTagCorruption, L"a72ae55c-c7f6-4ac8-8cd7-3e1422612b84" );
                        Error( ErrERRCheck( JET_errDbTimeCorrupted ) );
                    }
                }

                if( latchRIW == pmergePath->csr.Latch() )
                {
                    pmergePath->csr.UpgradeFromRIWLatch();
                }

                if ( pmergePath->csr.Dbtime() >= dbtimeMerge )
                {
                    FireWall( "MergePathDbTimeTooHigh" );
                    OSUHAEmitFailureTag( PinstFromIfmp( ifmp ), HaDbFailureTagCorruption, L"ef81250c-d5a1-4ac6-b7f7-9e128c39ad29" );
                    Error( ErrERRCheck( JET_errDbTimeCorrupted ) );
                }

                if ( pgnoNull != pmerge->csrRight.Pgno() )
                {
                    Assert( pmergePath->csr.Cpage().FLeafPage() );
                    Assert( pmerge->csrRight.Cpage().FLeafPage() );

                    pmerge->csrRight.UpgradeFromRIWLatch();

                    if ( pmerge->csrRight.Dbtime() >= dbtimeMerge )
                    {
                        FireWall( "MergeRightDbTimeTooHigh" );
                        OSUHAEmitFailureTag( PinstFromIfmp( ifmp ), HaDbFailureTagCorruption, L"4feadb0c-eedf-4f8f-b3b2-0156d095f305" );
                        Error( ErrERRCheck( JET_errDbTimeCorrupted ) );
                    }
                }

                if ( FBTIMergePageBeforeImageRequired( pmerge ) )
                {
                    const BOOL fRightMerge = FRightMerge( pmerge->mergetype );
                    CSR * const pcsrDest = fRightMerge ? &(pmerge->csrRight) : &(pmerge->csrLeft);
                    Assert( pgnoNull != pcsrDest->Pgno() );
                }


                //  Set the dbtimes on all involved pages...
                //

                if ( pgnoNull != pmerge->csrLeft.Pgno() )
                {
                    Assert( pmergePath->csr.Cpage().FLeafPage() );
                    Assert( pmerge->csrLeft.Cpage().FLeafPage() );
                    Assert( pmerge->csrLeft.Dbtime() < dbtimeMerge );
                    pmerge->dbtimeLeftBefore = pmerge->csrLeft.Dbtime();
                    pmerge->fFlagsLeftBefore = pmerge->csrLeft.Cpage().FFlags();
                    pmerge->csrLeft.CoordinatedDirty( dbtimeMerge );
                }

                Assert( pmergePath->csr.Dbtime() < dbtimeMerge );
                pmergePath->dbtimeBefore = pmergePath->csr.Dbtime();
                pmergePath->fFlagsBefore = pmergePath->csr.Cpage().FFlags();
                pmergePath->csr.CoordinatedDirty( dbtimeMerge );

                if ( pgnoNull != pmerge->csrRight.Pgno() )
                {
                    Assert( pmergePath->csr.Cpage().FLeafPage() );
                    Assert( pmerge->csrRight.Cpage().FLeafPage() );
                    Assert( pmerge->csrRight.Dbtime() < dbtimeMerge );
                    pmerge->dbtimeRightBefore = pmerge->csrRight.Dbtime();
                    pmerge->fFlagsRightBefore= pmerge->csrRight.Cpage().FFlags();
                    pmerge->csrRight.CoordinatedDirty( dbtimeMerge );
                }

                if ( pgnoNull != pmerge->csrNew.Pgno() )
                {
                    Assert( pmerge->csrNew.Latch() == latchWrite );
                    pmerge->csrNew.CoordinatedDirty( dbtimeMerge );
                }
            }
            else
            {
                Assert( !pmergePath->csr.Cpage().FLeafPage() );

                Assert( dbtimeInvalid == pmergePath->dbtimeBefore || dbtimeNil == pmergePath->dbtimeBefore );
                pmergePath->dbtimeBefore = dbtimeNil;
                pmergePath->fFlagsBefore = 0;

                if( latchRIW == pmergePath->csr.Latch() )
                {
                    pmergePath->csr.UpgradeFromRIWLatch();
                }

                if ( pmergePath->csr.Dbtime() < dbtimeMerge )
                {
                    pmergePath->dbtimeBefore = pmergePath->csr.Dbtime();
                    pmergePath->fFlagsBefore = pmergePath->csr.Cpage().FFlags();
                    pmergePath->csr.CoordinatedDirty( dbtimeMerge );
                }
                else
                {
                    FireWall( "MergePathDbTimeTooHighMultiLevel" );
                    OSUHAEmitFailureTag( PinstFromIfmp( ifmp ), HaDbFailureTagCorruption, L"f59c596c-2a9e-436e-b6c6-f442e07ff052" );
                    Error( ErrERRCheck( JET_errDbTimeCorrupted ) );
                }
            }

            pmergePath = pmergePath->pmergePathChild;
        }
        else
        {
            //  release latch and pmergePath at this level
            //
            AssertTrack( fFalse, "MergePathInconsistent" );
            MERGEPATH *pmergePathT = pmergePath;

            Assert( !pmergePath->fDeleteNode );
            Assert( !pmergePath->fKeyChange );
            Assert( !pmergePath->fEmptyPage );
            Assert( !pmergePath->csr.Cpage().FLeafPage() );
            Assert( NULL != pmergePath->pmergePathChild );
            Assert( pmergePath->pmergePathChild->pmergePathChild != NULL );
            Assert( !pmergePath->pmergePathChild->csr.Cpage().FLeafPage() );
            Assert( pmergePath->csr.Latch() == latchRIW );

            pmergePath = pmergePath->pmergePathChild;

            delete pmergePathT;
        }
    }

    return JET_errSuccess;

HandleError:
    Assert( err < 0 );
    BTIMergeRevertDbtime( pmergePathTip );
    return err;
}


//  sets lgpos for every page involved in merge or empty page operation
//
VOID BTIMergeSetLgpos( MERGEPATH *pmergePathTip, const LGPOS& lgpos )
{
    MERGEPATH   *pmergePath = pmergePathTip;
    Assert( pmergePath != NULL );

    const BOOL fLeafPage = pmergePath->csr.Cpage().FLeafPage();
    for ( ; pmergePath != NULL && pmergePath->csr.Latch() == latchWrite;
          pmergePath = pmergePath->pmergePathParent )
    {
        Assert( pmergePath->csr.FDirty() );

        pmergePath->csr.Cpage().SetLgposModify( lgpos );

        MERGE   *pmerge = pmergePath->pmerge;

        if ( pmerge != NULL )
        {
            if ( pmerge->csrLeft.Pgno() != pgnoNull )
            {
                Assert( fLeafPage );
                Assert( pmerge->csrLeft.Cpage().FLeafPage() );
                pmerge->csrLeft.Cpage().SetLgposModify( lgpos );
            }

            if ( pmerge->csrRight.Pgno() != pgnoNull )
            {
                Assert( fLeafPage );
                Assert( pmerge->csrRight.Cpage().FLeafPage() );
                pmerge->csrRight.Cpage().SetLgposModify( lgpos );
            }

            if ( pmerge->csrNew.Pgno() != pgnoNull )
            {
                pmerge->csrNew.Cpage().SetLgposModify( lgpos );
            }
        }
    }

#ifdef DEBUG
    for ( ; NULL != pmergePath; pmergePath = pmergePath->pmergePathParent )
    {
        Assert( pmergePath->csr.Latch() == latchRIW );
    }
#endif
}


//  reduces internal page fragmentation for all pages in the merge
//  path that are write latched
//
VOID BTIMergeShrinkPages( MERGEPATH *pmergePathLeaf )
{
    MERGEPATH   *pmergePath = pmergePathLeaf;

    for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathParent )
    {
        Assert( pmergePath->csr.FLatched() );
        if ( pmergePath->csr.Latch() == latchWrite )
        {
            AssertRTL( pmergePath->dbtimeBefore != pmergePath->csr.Cpage().Dbtime() );
            if ( pmergePath->dbtimeBefore != pmergePath->csr.Cpage().Dbtime() )
            {
                pmergePath->csr.Cpage().OptimizeInternalFragmentation();
            }
        }

        if ( pmergePath->pmerge != NULL )
        {
            if ( pmergePath->pmerge->csrLeft.FLatched() &&
                    pmergePath->pmerge->csrLeft.Latch() == latchWrite )
            {
                AssertRTL( pmergePath->pmerge->dbtimeLeftBefore != pmergePath->pmerge->csrLeft.Cpage().Dbtime() );
                if ( pmergePath->pmerge->dbtimeLeftBefore != pmergePath->pmerge->csrLeft.Cpage().Dbtime() )
                {
                    pmergePath->pmerge->csrLeft.Cpage().OptimizeInternalFragmentation();
                }
            }

            if ( pmergePath->pmerge->csrRight.FLatched() &&
                    pmergePath->pmerge->csrRight.Latch() == latchWrite )
            {
                AssertRTL( pmergePath->pmerge->dbtimeRightBefore != pmergePath->pmerge->csrRight.Cpage().Dbtime() );
                if ( pmergePath->pmerge->dbtimeRightBefore != pmergePath->pmerge->csrRight.Cpage().Dbtime() )
                {
                    pmergePath->pmerge->csrRight.Cpage().OptimizeInternalFragmentation();
                }
            }

            if ( pmergePath->pmerge->csrNew.FLatched() &&
                    pmergePath->pmerge->csrNew.Latch() == latchWrite )
            {
                pmergePath->pmerge->csrNew.Cpage().OptimizeInternalFragmentation();
            }
        }
    }
}


//  releases all latches held by merge or empty page operation
//
VOID BTIMergeReleaseLatches( MERGEPATH *pmergePathTip )
{
    MERGEPATH   *pmergePath = pmergePathTip;

    for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathParent )
    {
        Assert( pmergePath->csr.FLatched() );
        pmergePath->csr.ReleasePage();

        if ( pmergePath->pmerge != NULL )
        {
            if ( pmergePath->pmerge->csrLeft.FLatched() )
            {
                pmergePath->pmerge->csrLeft.ReleasePage();
            }

            if ( pmergePath->pmerge->csrRight.FLatched() )
            {
                pmergePath->pmerge->csrRight.ReleasePage();
            }

            if ( pmergePath->pmerge->csrNew.FLatched() )
            {
                pmergePath->pmerge->csrNew.ReleasePage();
            }
        }
    }
}


//  release every page marked empty
//
VOID BTIReleaseEmptyPages( FUCB *pfucb, MERGEPATH *pmergePathTip )
{
    MERGEPATH   *pmergePath = pmergePathTip;
    const BOOL  fAvailExt   = FFUCBAvailExt( pfucb );
    const BOOL  fOwnExt     = FFUCBOwnExt( pfucb );

    //  fake out cursor to make it think it's not a space cursor
    if ( fAvailExt )
    {
        Assert( !fOwnExt );
        FUCBResetAvailExt( pfucb );
    }
    else if ( fOwnExt )
    {
        FUCBResetOwnExt( pfucb );
    }
    Assert( !FFUCBSpace( pfucb ) );

    Assert( pmergePathTip->fEmptyPage
            || mergetypePartialRight == pmergePathTip->pmerge->mergetype
            || mergetypePartialLeft == pmergePathTip->pmerge->mergetype );

    Assert( !PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() ||
            fRecoveringUndo == PinstFromIfmp( pfucb->ifmp )->m_plog->FRecoveringMode() );

    for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathParent )
    {
        //  if empty page
        //      release space to FDP
        //
        Assert( !pmergePath->csr.FLatched() );
        if ( pmergePath->fEmptyPage )
        {

            //  space flags reset at the top of this function in order to fake out SPFreeExtent()
            Assert( !FFUCBSpace( pfucb ) );

            //  UNDONE: track lost space because of inability
            //          to split availExt tree with the released space
            //
            //  UNDONE: we will leak the space if ErrSPFreeExt() fails
            //
            Assert( pmergePath->csr.Pgno() != PgnoRoot( pfucb ) );
            const ERR   errFreeExt  = ErrSPFreeExt( pfucb, pmergePath->csr.Pgno(), 1, "FreeEmptyPages" );
#ifdef DEBUG
            if ( !FSPExpectedError( errFreeExt ) )
            {
                CallS( errFreeExt );
            }
#endif

            //  count pages released by an update
            //
            Ptls()->threadstats.cPageUpdateReleased++;
        }
    }

    Assert( !FFUCBSpace( pfucb ) );
    if ( fAvailExt )
    {
        FUCBSetAvailExt( pfucb );
    }
    else if ( fOwnExt )
    {
        FUCBSetOwnExt( pfucb );
    }
}


//  nullify every inactive flag-deleted version in page
//  delete node if flag-deleted with no active version
//
LOCAL VOID BTIMergeDeleteFlagDeletedNodes( FUCB * const pfucb, MERGEPATH * const pmergePath )
{
    CSR * const pcsr = &pmergePath->csr;

    if ( !FBTIUpdatablePage( *pcsr ) )
    {
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() );
        return;
    }
    Assert( latchWrite == pcsr->Latch() );

    Assert( pcsr->Cpage().FLeafPage() );
    Assert( pmergePath->pmergePathChild == NULL );

    const MERGE * const pmerge = pmergePath->pmerge;

    for ( INT iline = pcsr->Cpage().Clines() - 1; iline >= 0; iline-- )
    {
        const LINEINFO * const plineinfo = &pmerge->rglineinfo[iline];

        pcsr->SetILine( iline );
        NDGet( pfucb, pcsr );

        if ( FNDDeleted( pfucb->kdfCurr ) )
        {
            if ( FNDVersion( pfucb->kdfCurr ) )
            {
                // Cannot have active move FDeleted nodes as recovery does not move them
                Assert( !plineinfo->fVerActive );
                if ( !plineinfo->fVerActive )
                {
                    BOOKMARK bm;

                    NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );

                    VERNullifyInactiveVersionsOnBM( pfucb, bm );
                }
            }
        }

        // delete the node if it has moved
        if ( mergetypePartialRight == pmerge->mergetype && iline >= pmerge->ilineMerge )
        {
            NDDelete( pcsr );
        }
        else if ( mergetypePartialLeft == pmerge->mergetype && iline <= pmerge->ilineMerge )
        {
            NDDelete( pcsr );
        }
    }
}


//  fix siblings of leaf page merged or emptied
//  to point to each other
//
VOID BTIMergeFixSiblings( INST *pinst, MERGEPATH *pmergePath )
{
    MERGE   *pmerge = pmergePath->pmerge;

    Assert( !FBTIUpdatablePage( pmergePath->csr ) ||
            pmergePath->csr.Cpage().FLeafPage() );
    Assert( pmergePath->pmergePathChild == NULL );
    Assert( pmerge != NULL );

    if ( pmerge->csrLeft.Pgno() != pgnoNull &&
         FBTIUpdatablePage( pmerge->csrLeft ) )
    {
        Assert( !FBTIUpdatablePage( pmergePath->csr ) ||
                pmergePath->csr.Cpage().PgnoPrev() != pgnoNull );
        Assert( pmerge->csrLeft.Latch() == latchWrite );
        Assert( pmerge->csrLeft.FDirty() );

        pmerge->csrLeft.Cpage().SetPgnoNext( pmerge->csrRight.Pgno() );
    }
    else if ( !pinst->FRecovering() )
    {
        Assert( pmergePath->csr.Cpage().PgnoPrev() == pgnoNull );
    }
    else
    {
        Assert( pgnoNull == pmerge->csrLeft.Pgno() &&
                    ( !FBTIUpdatablePage( pmergePath->csr ) ||
                      pgnoNull == pmergePath->csr.Cpage().PgnoPrev() ) ||
                !FBTIUpdatablePage( pmerge->csrLeft ) );
    }

    if ( pmerge->csrRight.Pgno() != pgnoNull &&
         FBTIUpdatablePage( pmerge->csrRight ) )
    {
        Assert( !FBTIUpdatablePage( pmergePath->csr ) ||
                pmergePath->csr.Cpage().PgnoNext() != pgnoNull );
        Assert( pmerge->csrRight.Latch() == latchWrite );
        Assert( pmerge->csrRight.FDirty() );

        pmerge->csrRight.Cpage().SetPgnoPrev( pmerge->csrLeft.Pgno() );
    }
    else  if (!pinst->m_plog->FRecovering() )
    {
        Assert( pmergePath->csr.Cpage().PgnoNext() == pgnoNull );
    }
    else
    {
        Assert( pgnoNull == pmerge->csrRight.Pgno() &&
                    ( !FBTIUpdatablePage( pmergePath->csr ) ||
                      pgnoNull == pmergePath->csr.Cpage().PgnoNext() ) ||
                !FBTIUpdatablePage( pmerge->csrRight ) );
    }
}


//  ================================================================
LOCAL VOID BTIMergeMoveNodes( FUCB * const pfucb, MERGEPATH * const pmergePath )
//  ================================================================
//
//  Move undeleted nodes from the source page to the destination page. 
//  For left merges nodes <= ilineMerge are moved to the left page.
//  For right merges nodes >= ilineMerge are moved to the right page.
//  Set cbUncommittedFree on destination page.
//
//-

{
    Assert( mergetypeFullRight == pmergePath->pmerge->mergetype
            || mergetypePartialRight == pmergePath->pmerge->mergetype
            || mergetypeFullLeft == pmergePath->pmerge->mergetype
            || mergetypePartialLeft == pmergePath->pmerge->mergetype );

    CSR     * const pcsrSrc = &pmergePath->csr;
    MERGE   * const pmerge  = pmergePath->pmerge;
    Assert( NULL != pmerge );

    const BOOL fRightMerges = FRightMerge( pmergePath->pmerge->mergetype );
    CSR * const pcsrDest    = fRightMerges ? &(pmergePath->pmerge->csrRight) : &(pmergePath->pmerge->csrLeft);
    if ( !FBTIUpdatablePage( *pcsrDest ) )
    {
        return;
    }
    Assert( FBTIUpdatablePage( *pcsrSrc ) );
    Assert( FBTIUpdatablePage( *pcsrDest ) );

    // ilineSrcMin is the first iline to copy
    INT ilineSrcMin;
    // ilineSrcMax is the first iline not to copy (i.e. it is invalid)
    INT ilineSrcMax;
    // the location on the destination page to insert into. right merges go at the beginning
    // while left merges go at the end
    INT ilineDest;

    if( fRightMerges )
    {
        ilineSrcMin = pmerge->ilineMerge;
        ilineSrcMax = pmerge->clines;
        ilineDest = 0;
    }
    else
    {
        ilineSrcMin = 0;
        ilineSrcMax = pmerge->ilineMerge+1;
        ilineDest = pcsrDest->Cpage().Clines();
    }

    Assert( latchWrite == pcsrDest->Latch() );
    Assert( ilineSrcMin < ilineSrcMax );

    pcsrDest->SetILine( ilineDest );

#ifdef DEBUG
    //  cbUncommittedFree will be set correctly at the bottom of the function.
    //  Set to 0 here to prevent assertions in CPage during merge.
    //  If there is any error in cbUncommittedFree it will be detected when set.
    //
    if ( PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() )
    {
        pcsrDest->Cpage().SetCbUncommittedFree( 0 );
    }
#endif

// disable this warning
//  warning C22019: 'ilineSrcMax - 1' may be greater than 'ilineSrcMax'. 
//  This can be caused by integer underflow. This could yield an incorrect loop index 'iline>=ilineSrcMin'
#pragma warning(disable:22019)
    for ( INT iline = ilineSrcMax - 1; iline >= ilineSrcMin; iline-- )
#pragma warning(default:22019)
    {
        const LINEINFO * const plineinfo = &pmerge->rglineinfo[iline];

#ifdef DEBUG
        pcsrSrc->SetILine( iline );
        NDGet( pfucb, pcsrSrc );
        Assert( FKeysEqual( pfucb->kdfCurr.key, plineinfo->kdf.key ) );
        Assert( FDataEqual( pfucb->kdfCurr.data, plineinfo->kdf.data ) );
#endif

        if ( FNDDeleted( plineinfo->kdf ) )
        {
            // Cannot have active move FDeleted nodes as recovery does not move them
            Assert( !plineinfo->fVerActive );
            if ( !plineinfo->fVerActive )
            {
                continue;
            }
        }

#ifdef DEBUG
        //  check cbPrefix
        //
        NDGetPrefix( pfucb, pcsrDest );
        const INT cbCommon = CbCommonKey( pfucb->kdfCurr.key,
                                            plineinfo->kdf.key );
        if ( cbCommon > cbPrefixOverhead )
        {
            Assert( cbCommon == plineinfo->cbPrefix );
        }
        else
        {
            Assert( 0 == plineinfo->cbPrefix );
        }
#endif

        //  copy node to start/end of destination page
        //
        Assert( !FNDDeleted( plineinfo->kdf ) );
        Assert( pcsrDest->ILine() == ilineDest );
        NDInsert( pfucb, pcsrDest, &plineinfo->kdf, plineinfo->cbPrefix );
    }

    //  add uncommitted freed space caused by move to destination page
    //
    Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() ||
            pcsrDest->Cpage().CbUncommittedFree() +
                pmerge->cbSizeMaxTotal -
                pmerge->cbSizeTotal  ==
                pmerge->cbUncFreeDest );
    pcsrDest->Cpage().SetCbUncommittedFree( pmerge->cbUncFreeDest );
}


//  checks merge/empty page operation
//
INLINE VOID BTICheckMerge( FUCB *pfucb, MERGEPATH *pmergePathLeaf )
{
#ifdef DEBUG
    MERGEPATH   *pmergePath;

    //  check leaf level merge/empty page
    //
    BTICheckMergeLeaf( pfucb, pmergePathLeaf );

    //  check operation at internal levels
    //
    for ( pmergePath = pmergePathLeaf->pmergePathParent;
          pmergePath != NULL;
          pmergePath = pmergePath->pmergePathParent )
    {
        BTICheckMergeInternal( pfucb, pmergePath, pmergePathLeaf->pmerge );
    }
#endif
}

//  checks merge/empty page operation at leaf-level
//
VOID BTICheckMergeLeaf( FUCB *pfucb, MERGEPATH *pmergePath )
{
#ifdef DEBUG
    MERGE   * const pmerge      = pmergePath->pmerge;
    CSR     * const pcsr        = &pmergePath->csr;
    CSR     * const pcsrRight   = &pmerge->csrRight;
    CSR     * const pcsrLeft    = &pmerge->csrLeft;

    Assert( pmerge != NULL );

    const MERGETYPE mergetype   = pmerge->mergetype;
    const DBTIME    dbtime      = pcsr->Dbtime();

    Assert( mergetypeFullRight == mergetype ||
            mergetypeEmptyPage == mergetype ||
            mergetypePartialRight == mergetype ||
            mergetypeFullLeft == mergetype ||
            mergetypePartialLeft == mergetype );
    Assert( pcsr->Cpage().FLeafPage() );
    Assert( pmergePath->pmergePathParent != NULL );
    Assert( pmergePath->pmergePathParent->csr.Dbtime() == dbtime );

    if ( mergetypeFullRight == mergetype
        || mergetypePartialRight == mergetype )
    {
        Assert( latchWrite == pcsrRight->Latch() );
        Assert( pcsrRight->Pgno() != pgnoNull );
    }
    else if ( mergetypeFullLeft == mergetype
        || mergetypePartialLeft == mergetype )
    {
        Assert( latchWrite == pcsrLeft->Latch() );
        Assert( pcsrLeft->Pgno() != pgnoNull );
    }

    //  check sibling pages point to each other
    //
    if ( pmergePath->fEmptyPage )
    {
        Assert( pcsrRight->Pgno() != pcsr->Pgno() );
        if ( pcsrRight->Pgno() != pgnoNull )
        {
            Assert( pcsrRight->Dbtime() == dbtime );
            Assert( latchWrite == pcsrRight->Latch() );
            Assert( pcsrRight->Cpage().PgnoPrev() == pcsrLeft->Pgno() );
        }

        Assert( pcsrLeft->Pgno() != pcsr->Pgno() );
        if ( pcsrLeft->Pgno() != pgnoNull )
        {
            Assert( pcsrLeft->Dbtime() == dbtime );
            Assert( latchWrite == pcsrLeft->Latch() );
            Assert( pcsrLeft->Cpage().PgnoNext() == pcsrRight->Pgno() );
        }
    }
#endif  //  DEBUG
}


//  checks internal page after a merge/empty page operation
//
VOID BTICheckMergeInternal( FUCB        *pfucb,
                            MERGEPATH   *pmergePath,
                            MERGE       *pmergeLeaf )
{
#ifdef DEBUG
    Assert( pmergePath->pmerge == NULL );

    CSR             *pcsr   = &pmergePath->csr;
    const SHORT     clines  = SHORT( pcsr->Cpage().Clines() );
    const DBTIME    dbtime  = pcsr->Dbtime();

    Assert( !pcsr->Cpage().FLeafPage() );
    Assert( pcsr->Latch() == latchRIW
        || pcsr->Dbtime() == dbtime );

    //  if empty page,
    //      return
    //
    if ( pmergePath->fEmptyPage )
    {
        Assert( pmergePath->csr.Cpage().Clines() == 0 );
        return;
    }

    Assert( pmergePath->pmergePathParent == NULL ||
            !pmergePath->pmergePathParent->fEmptyPage );

    if ( pmergePath->fKeyChange ||
         pmergePath->fDeleteNode )
    {
        Assert( pcsr->Latch() == latchWrite );
    }
    else
    {
        //  UNDONE: change this later to Assert( fFalse )
        //          since the page should be released
        //
///     Assert( pcsr->Latch() == latchRIW );
        Assert( fFalse );
    }

    //  for every node in page
    //  check that it does not point to an empty page
    //
    for ( SHORT iline = 0; iline < clines; iline++ )
    {
        pcsr->SetILine( iline );
        NDGet( pfucb, pcsr );

        Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
        PGNO pgnoChild = *( (UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv() );

        MERGEPATH   *pmergePathT = pmergeLeaf->pmergePath;
        Assert( pmergePathT->pmergePathChild == NULL );

        for ( ;
              pmergePathT != NULL;
              pmergePathT = pmergePathT->pmergePathParent )
        {
            if ( pmergePathT->fEmptyPage )
            {
                Assert( pgnoChild != pmergePathT->csr.Pgno() );
            }
        }
    }

    //  check last node in page has same key
    //  as parent pointer
    //
    if ( pmergePath->pmergePathParent == NULL )
    {
        //  if root page, check if last node is NULL-keyed
        //
        Assert( !pcsr->Cpage().FLeafPage() );

        pcsr->SetILine( pcsr->Cpage().Clines() - 1 );
        NDGet( pfucb, pcsr );

        Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
        Assert( !pcsr->Cpage().FRootPage() ||
                pfucb->kdfCurr.key.FNull() );

        return;
    }

    pcsr->SetILine( clines - 1 );
    NDGet( pfucb, pcsr );
    const KEYDATAFLAGS kdfLast = pfucb->kdfCurr;

    CSR     *pcsrParent = &pmergePath->pmergePathParent->csr;

    Assert( pcsrParent->Latch() == latchRIW
        || pcsrParent->Dbtime() == dbtime );
    Assert( !pmergePath->pmergePathParent->fDeleteNode );
    pcsrParent->SetILine( pmergePath->pmergePathParent->iLine );
    NDGet( pfucb, pcsrParent );

    Assert( FKeysEqual( kdfLast.key, pfucb->kdfCurr.key ) );
#endif  //  DEBUG
}

#ifndef MINIMAL_FUNCTIONALITY

// This is support for eseutil /ms space analysis.

class CBTAcrossQueue {

    const PGNO      m_pgnoFDP;

    CStupidQueue *  m_pQ;
    PGNO            m_rgpgnoLevelLeftStart[32];
    ULONG           m_iNextLevel;   //  0 is root.

#ifdef DEBUG
    PGNO            m_pgnoCurr;
#endif

    ULONG           m_cPages;

    CStupidQueue::ERR ErrEnqueuePage( _In_ const PGNO pgno )
    {
        Assert( m_pQ );
        if ( m_rgpgnoLevelLeftStart[m_iNextLevel] == 0x0 )
        {
            m_rgpgnoLevelLeftStart[m_iNextLevel] = pgno;
        }
        return m_pQ->ErrEnqueue( (void*)&pgno );
    }

    static ERR EnumChildPagePtrNodes(
        const CPAGE::PGHDR * const  ppghdr,
        INT                         itag,
        DWORD                       fNodeFlags,
        const KEYDATAFLAGS * const  pkdf,
        void *                      pvCtx
        )
    {
        ERR                     err = JET_errSuccess;
        CBTAcrossQueue *    pThisCtx = (CBTAcrossQueue *) pvCtx;

        Assert( !( ppghdr->fFlags & CPAGE::fPageLeaf ) );

        if ( pkdf == NULL )
        {
            // we don't process the external header as a regular node...
            Assert( itag == 0 );
            return JET_errSuccess;
        }

        if ( itag >= CPAGE::CTagReserved( ppghdr ) )
        {
            Assert( pkdf->key.Cb() == ( pkdf->key.prefix.Cb() + pkdf->key.suffix.Cb() ) );  // I assumed this...

            PGNO pgnoLeaf;
            Assert( pkdf->data.Cb() == sizeof( PGNO ) );
            pgnoLeaf = *( ( UnalignedLittleEndian<ULONG>* )pkdf->data.Pv() );
            CStupidQueue::ERR errQueue = pThisCtx->ErrEnqueuePage( pgnoLeaf );
            if ( errQueue == CStupidQueue::ERR::errSuccess )
            {
                err = JET_errSuccess;
            }
            else if ( errQueue == CStupidQueue::ERR::errOutOfMemory )
            {
                err = ErrERRCheck( JET_errOutOfMemory );
            }
            else
            {
                AssertSz( fFalse, "Unexpected error out of ErrEnqueuePage()" );
                err = ErrERRCheck( JET_errInvalidParameter );
            }
        }

        return err;
    }

public:

    CBTAcrossQueue( _In_ const PGNO pgnoFDP ) :
        m_pgnoFDP( pgnoFDP )
    {
        memset( m_rgpgnoLevelLeftStart, 0, sizeof(m_rgpgnoLevelLeftStart) );    // set all pgnoLevelLeftStarts to 0x0
        m_pQ = NULL;
        m_iNextLevel = 0x0;
        m_cPages = 0x0;
    }

    ~CBTAcrossQueue( )
    {
        if ( m_pQ )
        {
            delete m_pQ;
        }
    }

    CStupidQueue::ERR ErrDequeuePage( _Out_ PGNO * ppgnoNext, _Out_ ULONG * piLevel )
    {
        //  This is essentially a queue so we steal the require queue's error space.
        CStupidQueue::ERR errQueue = CStupidQueue::ERR::errSuccess;

        //
        //  We deferred init/allocation of the queue until the first dequeue.
        //
        if ( m_pQ == NULL )
        {
            m_pQ = new CStupidQueue( sizeof( PGNO ) );
            if ( m_pQ == NULL )
            {
                return CStupidQueue::ERR::errOutOfMemory;
            }
            m_iNextLevel = 0;
            errQueue = ErrEnqueuePage( m_pgnoFDP );
            if ( errQueue != CStupidQueue::ERR::errSuccess )
            {
                return errQueue;
            }
        }

        //
        //  Dequeue a page and set level.
        //
        Assert( ppgnoNext );
        Assert( piLevel );
        PGNO pgnoNext;
        errQueue = m_pQ->ErrDequeue( &pgnoNext );
        if ( errQueue == CStupidQueue::ERR::errSuccess )
        {
            if ( m_rgpgnoLevelLeftStart[m_iNextLevel] == pgnoNext )
            {
                //  We are dequeuing a page that is the left most of this level, which means
                //  that we should increment the next level, so that any new pages we 
                //  enqueue (which would be form this page) would logically belong to the 
                //  next level.
                m_iNextLevel++;
            }
            Assert( m_iNextLevel != 0 );

#ifdef DEBUG
            m_pgnoCurr = pgnoNext;
#endif
            *ppgnoNext = pgnoNext;
            *piLevel = m_iNextLevel - 1;
        }
        return errQueue;
    }

    ERR ErrEnqueueNextLevel( _In_ const PGNO pgno, _In_ const CPAGE * const pcpage )
    {
        ERR err;

        Assert( m_pgnoCurr == pgno );

        if ( pcpage->FLeafPage() )
        {
            return JET_errSuccess;
        }

        //  Enum the tags, EnumChildPagePtrNodes knows how to crack the page out of 
        //  internal page nodes, and call ErrEnqueuPage().

        err = pcpage->ErrEnumTags( CBTAcrossQueue::EnumChildPagePtrNodes, (void*)this );
        return err;
    }


};


//  This function takes an ifmp and pgno, and does a breadth first evaluation of
//  the B-Tree, by reading it directly from the file.  It taks a fVisitFlags which
//  should be CPAGE::fPageLeaf drive all the way to the leaf pages, otherwise the
//  function will visit all internal pages only.
//      fVisitFlags is a set of CPage flags that are interprited to determine
//      what pages we visit.  
//              CPAGE::fPageParentOfLeaf visits all internal pages, 
//              CPAGE::fPageLeaf visits all leaf pages, 
//              together they visit all pages.
//  ================================================================
ERR ErrBTUTLAcross(
    IFMP                    ifmp,
    const PGNO              pgnoFDP,
    const ULONG             fVisitFlags,
    PFNVISITPAGE            pfnErrVisitPage,
    void *                  pvVisitPageCtx,
    CPAGE::PFNVISITNODE *   rgpfnzErrVisitNode,
    void **                 rgpvzVisitNodeCtx
    )
//  ================================================================
{
    ERR                     err         = JET_errSuccess;
    IFileAPI *              pfapi = g_rgfmp[ifmp].Pfapi();

    CBTAcrossQueue *    pBreadthFirst = NULL;

    PGNO                pgnoCurr = 0x0;
    ULONG               iCurrLevel = 0;

    VOID *              pvPageBuffer = NULL;


    //  Setup the variables required to implement our breadth first search of the B+ Tree.
    //
    Alloc( pBreadthFirst = new CBTAcrossQueue( pgnoFDP ) );

    //  Get buffer to hold page.    
    //
    Assert( g_rgfmp[ ifmp ].CbPage() >= g_cbPageMin );
    Alloc( pvPageBuffer = PvOSMemoryPageAlloc( g_rgfmp[ ifmp ].CbPage(), NULL ) );
    memset( pvPageBuffer, 0x42, g_rgfmp[ ifmp ].CbPage() );

    //  Walk the queue of pages ...
    //
    CStupidQueue::ERR errQueue = CStupidQueue::ERR::errSuccess;
    while ( CStupidQueue::ERR::errSuccess == ( errQueue = pBreadthFirst->ErrDequeuePage( &pgnoCurr, &iCurrLevel ) ) )
    {
        Assert( pgnoCurr );

        //  Read the next page.
        //
        TraceContextScope tcUtil( iorpDirectAccessUtil );
        Call( pfapi->ErrIORead( *tcUtil, OffsetOfPgno( pgnoCurr ), g_rgfmp[ ifmp ].CbPage(), (BYTE* const)pvPageBuffer, qosIONormal ) );
        CPAGE   cpage;
        cpage.LoadPage( ifmp, pgnoCurr, pvPageBuffer, g_rgfmp[ifmp].CbPage() );

        //  Evaluate any visiting the caller wanted to do.
        //
        if ( ( (fVisitFlags & CPAGE::fPageLeaf) && cpage.FLeafPage() ) ||
                ( (fVisitFlags & CPAGE::fPageParentOfLeaf) && ( !cpage.FLeafPage() || cpage.FRootPage() ) ) )
        {
            if ( pfnErrVisitPage )
            {
                Call( pfnErrVisitPage( pgnoCurr, iCurrLevel, &cpage, pvVisitPageCtx ) );
            }
            if ( rgpfnzErrVisitNode )
            {
                for( ULONG iVisitFunc = 0; rgpfnzErrVisitNode[iVisitFunc] != NULL; iVisitFunc++ )
                {
                    Call( cpage.ErrEnumTags( rgpfnzErrVisitNode[iVisitFunc], rgpvzVisitNodeCtx[iVisitFunc] ) );
                }
            }
        }

        //  Enqueue the level of pages below this one (if appropriate).
        //
        if ( !cpage.FLeafPage() )
        {
            //  Well its not leaf, so it has pages to enqueue, now check if appropriate.

            if ( cpage.FParentOfLeaf() && !(fVisitFlags & CPAGE::fPageLeaf) )
            {
                //  caller did not ask for leaf evaluation, and we're at parent of leaf, so do not 
                //  enque next level.
                continue;
            }

            //  Appropriate, so enque pages...
            //
            Call( pBreadthFirst->ErrEnqueueNextLevel( pgnoCurr, &cpage ) );
        }

    }

    //  Validate that we eventually emptied the queue.
    if ( CStupidQueue::ERR::wrnEmptyQueue == errQueue )
    {
        err = JET_errSuccess;   // Done!
    }
    else if ( CStupidQueue::ERR::errOutOfMemory == errQueue )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    else
    {
        AssertSz( fFalse, "This should be impossible." );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

HandleError:    // cleanup.

    OSMemoryPageFree( pvPageBuffer );
    delete pBreadthFirst;

    return err;
}

#endif  // MINIMAL_FUNCTIONALITY

// external header size array for different type of data stored in external header.
extern USHORT g_rgcbExternalHeaderSize[noderfMax];

ERR ErrBTGetRootField( _Inout_ FUCB* const pfucb, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderf, _In_ const LATCH latch )
{
    ERR err = JET_errSuccess;

    // navigate to the root page
    CallR( ErrBTIGotoRoot( pfucb, latch ) );
    // retrieve the specified root field
    NDGetExternalHeader( pfucb, Pcsr( pfucb ), noderf );

#ifdef DEBUG
    if ( noderf != noderfWhole )
    {
        Assert( noderfMax > noderf );
        Assert( (INT)g_rgcbExternalHeaderSize[noderf] == pfucb->kdfCurr.data.Cb() ||
            0 == pfucb->kdfCurr.data.Cb() );
    }
#endif // DEBUG

    if ( 0 == pfucb->kdfCurr.data.Cb() )
    {
        err = ErrERRCheck( JET_wrnColumnNull );
        goto HandleError;
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        BTUp( pfucb );
    }
    return err;
}

ERR ErrBTSetRootField( _In_ FUCB* const pfucb, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderf, _In_ const DATA& dataRootField )
{
    ERR err = JET_errSuccess;
    ULONG cbReq;

#ifdef DEBUG
    if ( noderf != noderfWhole )
    {
        Assert( noderfMax > noderf );
        Assert( dataRootField.Cb() == (INT)g_rgcbExternalHeaderSize[noderf] );
    }
#endif // DEBUG

    Assert( Pcsr( pfucb )->FLatched() );
    if ( Pcsr( pfucb )->Latch() != latchWrite )
    {
        Call( Pcsr( pfucb )->ErrUpgrade() );
    }
    Assert( Pcsr( pfucb )->Latch() == latchWrite );

    // retrieve the specified root field and make sure we have enough space to update it
    NDGetExternalHeader( pfucb, Pcsr( pfucb ), noderf );

    // One extra byte required for the new header field
    cbReq = dataRootField.Cb() > pfucb->kdfCurr.data.Cb() ?  ( dataRootField.Cb() - pfucb->kdfCurr.data.Cb() + 1 ) : 0;
    if ( FBTISplit( pfucb, Pcsr( pfucb ), cbReq ) || ( ErrFaultInjection( 47278 ) < JET_errSuccess ) )
    {
        // Undone: implement split for this.
        return ErrERRCheck( JET_errRecordTooBig );
    }

    Call( ErrNDSetExternalHeader( pfucb, Pcsr( pfucb ), &dataRootField, fDIRNull, noderf ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        BTUp( pfucb );
    }
    return err;
}

