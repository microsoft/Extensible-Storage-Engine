// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_bt.hxx"

#include "PageSizeClean.hxx"




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
LOCAL INT IlineBTIFrac( FUCB *pfucb, DIB *pdib );

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

bool FBTICpgFragmentedRange(
    __in_ecount(cpg) const PGNO * const rgpgno,
    const CPG cpg,
    __in const CPG cpgFragmentedRangeMin,
    __out INT * const pipgnoStart,
    __out INT * const pipgnoEnd );
bool FBTICpgFragmentedRangeTrigger(
    __in_ecount(cpg) const PGNO * const rgpgno,
    const CPG cpg,
    __in const CPG cpgFragmentedRangeTrigger,
    __in const CPG cpgFragmentedRangeMin );
LOCAL BOOL FBTIEligibleForOLD2( FUCB * const pfucb );
LOCAL ERR ErrBTIRegisterForOLD2( FUCB * const pfucb );

VOID AssertBTIVerifyPgnoSplit( FUCB *pfucb, SPLITPATH *psplitPath );
VOID AssertBTIBookmarkSaved( const FUCB *pfucb );

INT CbNDCommonPrefix( FUCB *pfucb, CSR *pcsr, const KEY& key );


const ULONG g_cBTINonVisibleNodesSkippedWarningThreshold            = 10000;
const ULONG g_cBTIPagesVisitedWithNonVisibleNodesWarningThreshold   = 100;
const __int64 g_cBTIPagesCacheMissedForNonVisibleNodes              = 64;
const ULONG g_ulBTINonVisibleNodesSkippedWarningFrequency           = 1;
const double g_pctBTICacheMissLatencyToQualifyForIoLoad             = 0.80;

const double g_cmsecBTINavigateProcessingLatencyThreshold           = 100.0;
const __int64 g_cmsecBTINavigateCacheMissLatencyThreshold           = 10000;



const CPG cpgFragmentedRangeDefrag = 16;

const CPG cpgFragmentedRangeDefragTrigger = cpgFragmentedRangeDefrag / 2;


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

#endif

INLINE PIBTraceContextScope TcBTICreateCtxScope( FUCB* pfucb, IOREASONSECONDARY iors )
{
    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->iorReason.SetIors( iors );

    tcScope->SetDwEngineObjid( pfucb->u.pfcb->ObjidFDP() );
    return tcScope;
}





ERR ErrBTOpen( PIB *ppib, FCB *pfcb, FUCB **ppfucb, BOOL fAllowReuse )
{
    ERR     err;
    FUCB    *pfucb;

    Assert( pfcb != pfcbNil );
    Assert( pfcb->FInitialized() );

    if ( fAllowReuse )
    {
        ENTERCRITICALSECTION critCursors( &ppib->critCursors, ppib->FBatchIndexCreation() );

        for ( pfucb = ppib->pfucbOfSession;
            pfucb != pfucbNil;
            pfucb = pfucb->pfucbNextOfSession )
        {
            if ( FFUCBDeferClosed( pfucb ) && !FFUCBNotReuse( pfucb ) )
            {
                Assert( !FFUCBSpace( pfucb ) );

                Assert( pfucb->u.pfcb != pfcbNil || FFUCBSecondary( pfucb ) );
                if ( pfucb->u.pfcb == pfcb )
                {
                    const BOOL  fVersioned  = FFUCBVersioned( pfucb );

                    Assert( pfcbNil != pfucb->u.pfcb );
                    Assert( ppib->Level() > 0 );
                    Assert( pfucb->levelOpen <= ppib->Level() );

                    Assert( 0 == pfucb->levelReuse );
                    pfucb->levelReuse = ppib->Level();

                    FUCBResetFlags( pfucb );
                    Assert( !FFUCBDeferClosed( pfucb ) );

                    pfucb->pfucbTable = NULL;

                    FUCBResetPreread( pfucb );
                    FUCBResetOpportuneRead( pfucb );
                    
                    Assert( !FFUCBVersioned( pfucb ) );
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
        pfucb = pfucbNil;
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
            pfucb->u.pfcb->Unlink( pfucb );
        }
        FUCBClose( pfucb );
    }
    return err;
}

ERR ErrBTOpenByProxy( PIB *ppib, FCB *pfcb, FUCB **ppfucb, const LEVEL level )
{
    ERR     err;
    FUCB    *pfucb;

    Assert( PinstFromPpib( ppib )->RwlTrx( ppib ).FWriter() );

    Assert( level > 0 );
    Assert( pfcb != pfcbNil );
    Assert( pfcb->FInitialized() );

    Assert( pfcb->FTypeSecondaryIndex() );
    Assert( pfcbNil == pfcb->PfcbTable() );
    Assert( !pfcb->FInitialIndex() );

    pfcb->Lock();

    pfcb->FucbList().LockForEnumeration();

    for ( INT ifucbIndex = 0; ifucbIndex < pfcb->FucbList().Count(); ifucbIndex++ )
    {
        pfucb = pfcb->FucbList()[ifucbIndex];

        if ( pfucb->ppib == ppib )
        {
            const BOOL  fVersioned  = FFUCBVersioned( pfucb );

            Assert( FFUCBDeferClosed( pfucb ) );

            Assert( !FFUCBNotReuse( pfucb ) );
            Assert( !FFUCBSpace( pfucb ) );
            Assert( FFUCBIndex( pfucb ) );
            Assert( FFUCBSecondary( pfucb ) );
            Assert( pfucb->u.pfcb == pfcb );
            Assert( ppib->Level() > 0 );
            Assert( pfucb->levelOpen > 0 );

            Assert( 0 == pfucb->levelReuse );

            pfucb->levelOpen = 0;

            FUCBResetFlags( pfucb );
            Assert( !FFUCBDeferClosed( pfucb ) );

            pfucb->pfucbTable = NULL;

            FUCBResetPreread( pfucb );
            FUCBResetOpportuneRead( pfucb );

            Assert( !FFUCBVersioned( pfucb ) );
            if ( fVersioned )
                FUCBSetVersioned( pfucb );

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

    Assert( FFUCBIndex( pfucb ) );
    Assert( FFUCBSecondary( pfucb ) );

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
            pfucb->u.pfcb->Unlink( pfucb );
        }
        FUCBClose( pfucb );
    }
    return err;
}


VOID BTClose( FUCB *pfucb )
{
    INST * const    pinst   = PinstFromPfucb( pfucb );
    LOG * const     plog    = pinst->m_plog;

    FUCBAssertNoSearchKey( pfucb );

    Assert( !FFUCBCurrentSecondary( pfucb ) );
    Assert( pfucb->cpgSpaceRequestReserve == 0 );

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
        plog->LGRRemoveFucb( pfucb );
    }

    if ( pfucb->ppib->Level() > 0 && FFUCBVersioned( pfucb ) )
    {
        Assert( !FFUCBSpace( pfucb ) );
        RECRemoveCursorFilter( pfucb );
        FUCBRemoveEncryptionKey( pfucb );
        FUCBSetDeferClose( pfucb );
    }
    else
    {
        FCB *pfcb = pfucb->u.pfcb;

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



            pfucb->u.pfcb->Unlink( pfucb, fTrue );


            pfcb->PrepareForPurge();
            pfcb->Purge();
        }
        else if ( pfcb->FTypeTable() )
        {



            pfucb->u.pfcb->Unlink( pfucb );
        }
        else
        {



            pfucb->u.pfcb->Unlink( pfucb, fTrue );
        }


        FUCBClose( pfucb );
    }
}




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
        pfucb->fBookmarkPreviouslySaved = fBookmarkPreviouslySaved;
    }

HandleError:
    Assert( err >= 0 || !Pcsr( pfucb )->FLatched() );
    return err;
}


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
        pfucb->ulLTCurr = pfucb->ulLTLast;
        pfucb->ulTotalCurr = pfucb->ulTotalLast;

        Pcsr( pfucb )->ReleasePage( pfucb->u.pfcb->FNoCache() );
        if( NULL != pfucb->pvRCEBuffer )
        {
            OSMemoryHeapFree( pfucb->pvRCEBuffer );
            pfucb->pvRCEBuffer = NULL;
        }
    }

    pfucb->fTouch = fFalse;

#ifdef DEBUG
    pfucb->kdfCurr.Invalidate();
#endif

    return err;
}

ERR ErrBTDeferGotoBookmark( FUCB *pfucb, const BOOKMARK& bm, BOOL fTouch )
{
    ERR     err;

    CallR( ErrBTISaveBookmark( pfucb, bm, fTouch ) );
    BTUp( pfucb );


    pfucb->ulLTCurr = 0;
    pfucb->ulTotalCurr = 0;

    return err;
}


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

    if ( !pfucb->pvBMBuffer && ( fUnique ? cbKey : cbKey + cbData ) > cbBMCache )
    {
        Alloc( pfucb->pvBMBuffer = RESBOOKMARK.PvRESAlloc() );
    }


    pfucb->fTouch = fTouch;

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

    if ( fUnique )
    {
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


    pfucb->fBookmarkPreviouslySaved = fTrue;

HandleError:
    return err;
}



LOCAL BOOL FKeysEqual( const KEY& key1, const BYTE* const pbKey2, const ULONG cbKey )
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
    const PGNO  pgnoBadLink,
    const BOOL  fFatal,
    const CHAR * const szTag )
{
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
#endif

struct BTISKIPSTATS
{
private:
    ULONG       cNodesSkipped;

    ULONG       cUnversionedDeletes;
    ULONG       cUncommittedDeletes;
    ULONG       cCommittedDeletes;
    ULONG       cNonVisibleInserts;
    ULONG       cFiltered;

    ULONG       cPagesVisitedWithSkippedNodes;
    ULONG       cPagesCacheMissBaseline;
    __int64     cusecCacheMissBaseline;
    DBTIME      dbtimeSkippedPagesHigh;
    DBTIME      dbtimeSkippedPagesAve;
    DBTIME      dbtimeSkippedPagesLow;

    HRT     hrtStart;

    OnDebug( BYTE   bFill );

public:
    void BTISkipStatsIDeferredInit()
    {
        Assert( ( sizeof(*this) - sizeof(cNodesSkipped) ) <= memcnt( (BYTE*)this, bFill, sizeof(*this) ) );
        memset( this, 0, sizeof(*this) );
        hrtStart = HrtHRTCount();
    }

public:

    BTISKIPSTATS()
    {
#ifdef DEBUG
        bFill = rand() % 255;
        memset( this, bFill, sizeof(*this) );
#endif
        cNodesSkipped = 0;
    }

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

    void BTISkipStatsUpdateSkippedPage( const CPAGE& cpage )
    {
        Assert( cNodesSkipped != 0 );

        if ( cPagesVisitedWithSkippedNodes == 0 )
        {
            cPagesCacheMissBaseline = Ptls()->threadstats.cPageCacheMiss;
            cusecCacheMissBaseline = Ptls()->threadstats.cusecPageCacheMiss;
            dbtimeSkippedPagesLow = 0x7FFFFFFFFFFFFFFF;
        }

        cPagesVisitedWithSkippedNodes++;
        dbtimeSkippedPagesHigh = max( dbtimeSkippedPagesHigh, cpage.Dbtime() );
        dbtimeSkippedPagesLow  = min( dbtimeSkippedPagesLow, cpage.Dbtime() );
        dbtimeSkippedPagesAve += cpage.Dbtime();
    }


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
                cmsecCacheMisses < g_cmsecBTINavigateCacheMissLatencyThreshold  )
        {
            return fFalse;
        }

        const double    dblTimeElapsed = DblHRTElapsedTimeFromHrtStart( hrtStart );
        const double    dblFaultWait = double( Ptls()->threadstats.cusecPageCacheMiss - cusecCacheMissBaseline ) / 1000.0;
        if ( dblFaultWait > dblTimeElapsed ||
                ( dblFaultWait / dblTimeElapsed ) < g_pctBTICacheMissLatencyToQualifyForIoLoad  )
        {
            return fFalse;
        }

        return fTrue;
    }
    
    INLINE BOOL FBTIReportManyNonVisibleNodesSkipped() const
    {
        Assert( cNodesSkipped == 0 || cNodesSkipped == cUnversionedDeletes + cUncommittedDeletes + cCommittedDeletes + cNonVisibleInserts + cFiltered );

        C_ASSERT( g_cBTIPagesVisitedWithNonVisibleNodesWarningThreshold > 20 );

        Assert( (__int64)g_cmsecBTINavigateProcessingLatencyThreshold < g_cmsecBTINavigateCacheMissLatencyThreshold );

        return cNodesSkipped &&
                ( cNodesSkipped - cFiltered ) &&
                ( DblHRTElapsedTimeFromHrtStart( hrtStart ) >= g_cmsecBTINavigateProcessingLatencyThreshold ) &&
                ( ( cNodesSkipped - cFiltered ) > g_cBTINonVisibleNodesSkippedWarningThreshold
                    || cPagesVisitedWithSkippedNodes > ( g_cBTIPagesVisitedWithNonVisibleNodesWarningThreshold - 1 )
                    || FBTIReportManyCacheMissesForSkippedNodes() );
    }


    VOID BTIReportManyNonVisibleNodesSkipped( FUCB * const pfucb )
    {
        Assert( cNodesSkipped == cUnversionedDeletes + cUncommittedDeletes + cCommittedDeletes + cNonVisibleInserts + cFiltered );
        FCB * const     pfcb                        = pfucb->u.pfcb;

        const double    dblTimeElapsed = DblHRTElapsedTimeFromHrtStart( hrtStart );
        const ULONG     cpgFaulted = Ptls()->threadstats.cPageCacheMiss - cPagesCacheMissBaseline;
        const double    dblFaultWait = double( Ptls()->threadstats.cusecPageCacheMiss - cusecCacheMissBaseline ) / 1000.0;

        Assert( cPagesVisitedWithSkippedNodes );
        dbtimeSkippedPagesAve = dbtimeSkippedPagesAve / cPagesVisitedWithSkippedNodes;
        

        pfcb->Lock();

        const TICK      tickCurr                    = TickOSTimeCurrent();
        const TICK      tickLast                    = pfcb->TickMNVNLastReported();
        const TICK      tickDiff                    = ( 0 == tickLast ? 0 : tickCurr - tickLast );
        const ULONG     ulHoursSinceLastReported    = tickDiff / ( 1000 * 60 * 60 );
        const ULONG     cPreviousIncidents          = pfcb->CMNVNIncidentsSinceLastReported();
        const ULONG     cPreviousNodesSkipped       = pfcb->CMNVNNodesSkippedSinceLastReported();
        const ULONG     cPreviousPagesVisited       = pfcb->CMNVNPagesVisitedSinceLastReported();
        const BOOL      fInitialReport              = ( 0 == tickLast );
        const BOOL      fIoLoadIssue                = FBTIReportManyCacheMissesForSkippedNodes();
#ifdef ESENT
        const BOOL      fReportEvent                = fFalse;
#else
        const BOOL      lTargetLoggingLevel         = JET_EventLoggingLevelMin;
        const BOOL      fReportEvent                = ( UlParam( PinstFromPfucb( pfucb ), JET_paramEventLoggingLevel ) >= (ULONG_PTR)lTargetLoggingLevel )
                                                        && ( fInitialReport || ulHoursSinceLastReported >= g_ulBTINonVisibleNodesSkippedWarningFrequency )
                                                        && ( fIoLoadIssue ?
                                                                g_rgfmp[ pfcb->Ifmp() ].FCheckTreeSkippedNodesDisk( pfcb->ObjidFDP() ) :
                                                                g_rgfmp[ pfcb->Ifmp() ].FCheckTreeSkippedNodesCpu( pfcb->ObjidFDP() ) );
#endif

        if ( fReportEvent )
        {
            pfcb->SetTickMNVNLastReported( tickCurr );
            pfcb->ResetCMNVNIncidentsSinceLastReported();
            pfcb->ResetCMNVNNodesSkippedSinceLastReported();
            pfcb->ResetCMNVNPagesVisitedSinceLastReported();
        }
        else
        {
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
                CallS( szBtreeName.ErrSet( "<null>" ) );
                CallS( szTableName.ErrSet( "<null>" ) );
            }

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

            if ( cbKey > 0 )
            {
                pbKey = (BYTE *)RESKEY.PvRESAlloc();
                if ( NULL != pbKey )
                {
                    pfucb->bmCurr.key.CopyIntoBuffer( pbKey, cbKeyAlloc );
                }
                else
                {
                    cbKey = 0;
                }
            }

            if ( fInitialReport )
            {
                UtilReportEvent(
                        eventWarning,
                        PERFORMANCE_CATEGORY,
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

};


BOOL FBTINodeCommittedDelete( const FUCB * pfucb, const KEYDATAFLAGS& kdf )
{
    BOOL fDeleted = fFalse;

    Assert( pfucb );
    
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

            if ( FFUCBUnique( pfucb ) )
            {
                bm.data.Nullify();
            }
            else
            {
                bm.data = kdf.data;
            }
        
            if( !FVERActive( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bm, TrxOldest( PinstFromPfucb( pfucb ) ) ) )
            {
                fDeleted = fTrue;
            }
        }
    }

    return fDeleted;
}

BOOL FBTINodeCommittedDelete( const FUCB * pfucb )
{
    return FBTINodeCommittedDelete( pfucb, pfucb->kdfCurr );
}

BOOL FBTLogicallyNavigableState( const FUCB * const pfucb )
{
    if ( !PinstFromPfucb( pfucb )->m_plog->FRecovering() ||
         fRecoveringRedo != PinstFromPfucb( pfucb )->m_plog->FRecoveringMode() ||
         !g_rgfmp[ pfucb->ifmp ].FContainsDataFromFutureLogs() ||
         g_rgfmp[ pfucb->ifmp ].m_fCreatingDB )
    {
        return fTrue;
    }
    return fFalse;
}

ERR ErrBTNext( FUCB *pfucb, DIRFLAG dirflag )
{

    ERR         err;
    CSR * const pcsr                        = Pcsr( pfucb );
    BYTE*       pbkeySave                   = NULL;
    ULONG       cbKeySave                   = 0;
    ULONG       cNeighbourKeysSkipped       = 0;
    BOOL        fNodesSkippedOnCurrentPage  = fFalse;
    BTISKIPSTATS    btskipstats;
    PIBTraceContextScope tcScope        = TcBTICreateCtxScope( pfucb, iorsBTMoveNext );

    if ( pfucb->ppib->FReadOnlyTrx() && pfucb->ppib->Level() > 0 )
    {
        CallR( PverFromIfmp( pfucb->ifmp )->ErrVERCheckTransactionSize( pfucb->ppib ) );
    }

    Assert( FBTLogicallyNavigableState( pfucb ) );

    Assert( !pfucb->pmoveFilterContext || pfucb->u.pfcb->FTypeTable() || pfucb->u.pfcb->FTypeSecondaryIndex() );

    CallR( ErrBTIRefresh( pfucb ) );

    
    LvId lid = lidMin;
    if( dirflag & fDIRSameLIDOnly )
    {

        Assert( pfucb->u.pfcb->FTypeLV() );
        LidFromKey( &lid, pfucb->kdfCurr.key );
        Assert( lidMin != lid );
    }
    
Start:

    Assert( pcsr->Latch() == latchReadTouch ||
            pcsr->Latch() == latchReadNoTouch ||
            pcsr->Latch() == latchRIW );
    Assert( ( pcsr->Cpage().FLeafPage() ) != 0 );
    AssertNDGet( pfucb );

    if ( pcsr->ILine() + 1 < pcsr->Cpage().Clines() )
    {
        pcsr->IncrementILine();
        NDGet( pfucb );

        if ( pfucb->ulTotalLast )
        {
            pfucb->ulLTLast++;
        }
    }
    else
    {
        Assert( pcsr->ILine() + 1 == pcsr->Cpage().Clines() );

        pfucb->ulLTLast = 0;
        pfucb->ulTotalLast = 0;

        if ( pcsr->Cpage().PgnoNext() == pgnoNull )
        {
            Error( ErrERRCheck( JET_errNoCurrentRecord ) );
        }
        else
        {
            const PGNO  pgnoFrom    = pcsr->Pgno();


            if ( fNodesSkippedOnCurrentPage )
            {
                btskipstats.BTISkipStatsUpdateSkippedPage( pcsr->Cpage() );
                fNodesSkippedOnCurrentPage = fFalse;
            }

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
                    pfucb->cpgPrereadNotConsumed--;
                }

                if ( !FFUCBLongValue( pfucb ) )
                {
                    if ( 1 == pfucb->cpgPrereadNotConsumed
                         && pcsr->Cpage().PgnoNext() != pgnoNull )
                    {
                        PIBTraceContextScope tcScopeT = pfucb->ppib->InitTraceContextScope();
                        tcScopeT->iorReason.AddFlag( iorfOpportune );

                        (void)ErrBFPrereadPage( pfucb->ifmp, pcsr->Cpage().PgnoNext(), bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->ifmp ), *tcScopeT );
                    }

                    if ( 0 == pfucb->cpgPrereadNotConsumed )
                    {
                        if ( ( dirflag & fDIRNeighborKey )
                            && NULL == pbkeySave )
                        {
                            Alloc( pbkeySave = ( BYTE *)RESKEY.PvRESAlloc() );
                            cbKeySave = pfucb->bmCurr.key.Cb();
                            pfucb->bmCurr.key.CopyIntoBuffer( pbkeySave, cbKeyAlloc );
                        }

                        


                        const LATCH latch = pcsr->Latch();
                        BOOL    fVisible;
                        NS      ns;
                        Call( ErrNDVisibleToCursor( pfucb, &fVisible, &ns ) );
                        Assert( pcsr->FLatched() );
                        Call( ErrBTRelease( pfucb ) );
                        Assert( !pcsr->FLatched() );
                        Call( ErrBTGotoBookmark( pfucb, pfucb->bmCurr, latch, fVisible ) );
                        Assert( !fVisible || ( err != wrnNDFoundGreater && err != wrnNDFoundLess ) );
                        if ( wrnNDFoundLess == err )
                        {
                            goto Start;
                        }
                    }
                }
            }

        }
    }

    AssertNDGet( pfucb );


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

    if ( !( dirflag & fDIRAllNode ) )
    {
        Assert( !( dirflag & fDIRAllNodesNoCommittedDeleted ) );
        
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
                Error( ErrERRCheck( JET_errNoCurrentRecord ) );
            }

            if ( FFUCBLimstat( pfucb ) && FFUCBUpper( pfucb ) )
            {
                Call( ErrFUCBCheckIndexRange( pfucb, pfucb->kdfCurr.key ) );
            }

            btskipstats.BTISkipStatsUpdateSkippedNode( ns );
            fNodesSkippedOnCurrentPage = fTrue;
            goto Start;
        }
    }
    else
    {
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

    if ( btskipstats.FBTIReportManyNonVisibleNodesSkipped() )
    {
        if ( fNodesSkippedOnCurrentPage && pcsr->Latch() > latchNone )
        {
            btskipstats.BTISkipStatsUpdateSkippedPage( pcsr->Cpage() );
        }

        btskipstats.BTIReportManyNonVisibleNodesSkipped( pfucb );
    }

    RESKEY.Free( pbkeySave );
    return err;
}


ERR ErrBTPrev( FUCB *pfucb, DIRFLAG dirflag )
{
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

    Assert( !pfucb->pmoveFilterContext || pfucb->u.pfcb->FTypeTable() || pfucb->u.pfcb->FTypeSecondaryIndex() );

    
    if( dirflag & fDIRSameLIDOnly )
    {
        AssertSz( fFalse, "ErrBTPrev doesn't support fDIRSameLIDOnly" );
        return ErrERRCheck( JET_errInvalidGrbit );
    }
    
    if ( pfucb->ppib->FReadOnlyTrx() && pfucb->ppib->Level() > 0 )
    {
        CallR( PverFromIfmp( pfucb->ifmp )->ErrVERCheckTransactionSize( pfucb->ppib ) );
    }


    CallR( ErrBTIRefresh( pfucb ) );

Start:

    Assert( latchReadTouch == pcsr->Latch() ||
            latchReadNoTouch == pcsr->Latch() ||
            latchRIW == pcsr->Latch() );
    Assert( ( pcsr->Cpage().FLeafPage() ) != 0 );
    AssertNDGet( pfucb );

    if ( pcsr->ILine() > 0 )
    {
        pcsr->DecrementILine();
        NDGet( pfucb );

        if ( pfucb->ulTotalLast )
        {
            pfucb->ulLTLast--;
        }
    }
    else
    {
        Assert( pcsr->ILine() == 0 );

        pfucb->ulLTLast = 0;
        pfucb->ulTotalLast = 0;

        if ( pcsr->Cpage().PgnoPrev() == pgnoNull )
        {
            Error( ErrERRCheck( JET_errNoCurrentRecord ) );
        }
        else
        {
            const PGNO  pgnoFrom    = pcsr->Pgno();


            if ( fNodesSkippedOnCurrentPage )
            {
                btskipstats.BTISkipStatsUpdateSkippedPage( pcsr->Cpage() );
                fNodesSkippedOnCurrentPage = fFalse;
            }

            err = pcsr->ErrSwitchPageNoWait(
                                    pfucb->ppib,
                                    pfucb->ifmp,
                                    pcsr->Cpage().PgnoPrev() );
            if ( err == errBFLatchConflict )
            {
                cLatchConflicts++;
                Assert( cLatchConflicts < 1000 );

                const PGNO      pgnoWait    = pcsr->Cpage().PgnoPrev();
                const LATCH     latch       = pcsr->Latch();

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

                err = pcsr->ErrGetRIWPage(
                                    pfucb->ppib,
                                    pfucb->ifmp,
                                    pgnoWait );
                if ( err < JET_errSuccess )
                {
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

                    if ( !FFUCBLongValue( pfucb ) )
                    {
                        if ( 1 == pfucb->cpgPrereadNotConsumed
                            && pcsr->Cpage().PgnoPrev() != pgnoNull )
                        {
                            PIBTraceContextScope tcScopeT = pfucb->ppib->InitTraceContextScope();
                            tcScopeT->iorReason.AddFlag( iorfOpportune );

                            (void)ErrBFPrereadPage( pfucb->ifmp, pcsr->Cpage().PgnoPrev(), bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->ifmp ), *tcScopeT );
                        }

                        if ( 0 == pfucb->cpgPrereadNotConsumed )
                        {
                            if ( ( dirflag & fDIRNeighborKey )
                                && NULL == pbkeySave )
                            {
                                Alloc( pbkeySave = ( BYTE* )RESKEY.PvRESAlloc() );
                                cbKeySave = pfucb->bmCurr.key.Cb();
                                pfucb->bmCurr.key.CopyIntoBuffer( pbkeySave, cbKeyAlloc );
                            }

                            const LATCH latch = pcsr->Latch();
                            BOOL    fVisible;
                            NS      ns;
                            Call( ErrNDVisibleToCursor( pfucb, &fVisible, &ns ) );
                            Assert( pcsr->FLatched() );
                            Call( ErrBTRelease( pfucb ) );
                            Assert( !pcsr->FLatched() );
                            Call( ErrBTGotoBookmark( pfucb, pfucb->bmCurr, latch, fVisible ) );
                            Assert( !fVisible || ( err != wrnNDFoundGreater && err != wrnNDFoundLess ) );
                            if ( wrnNDFoundGreater == err )
                            {
                                goto Start;
                            }
                        }
                    }
                }
            }
        }
    }

    AssertNDGet( pfucb );


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
            if ( FFUCBLimstat( pfucb ) && !FFUCBUpper( pfucb ) )
            {
                Call( ErrFUCBCheckIndexRange( pfucb, pfucb->kdfCurr.key ) );
            }
            
            btskipstats.BTISkipStatsUpdateSkippedNode( ns );
            fNodesSkippedOnCurrentPage = fTrue;
            goto Start;
        }
    }
    else
    {
        Assert ( dirflag & fDIRAllNodesNoCommittedDeleted );
        
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

    if ( btskipstats.FBTIReportManyNonVisibleNodesSkipped() )
    {
        if ( fNodesSkippedOnCurrentPage && pcsr->Latch() > latchNone )
        {
            btskipstats.BTISkipStatsUpdateSkippedPage( pcsr->Cpage() );
        }

        btskipstats.BTIReportManyNonVisibleNodesSkipped( pfucb );
    }

    RESKEY.Free( pbkeySave );
    return err;
}


VOID BTPrereadSpaceTree( const FUCB * const pfucb )
{
    FCB* const pfcb = pfucb->u.pfcb;
    Assert( pfcbNil != pfcb );

    INT     ipgno = 0;
    PGNO    rgpgno[3];
    if( pgnoNull != pfcb->PgnoOE() )
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


VOID BTPrereadIndexesOfFCB( FUCB * const pfucb )
{
    FCB* const pfcb = pfucb->u.pfcb;
    Assert( pfcbNil != pfcb );
    Assert( pfcb->FTypeTable() );
    Assert( ptdbNil != pfcb->Ptdb() );

    const INT   cMaxIndexesToPreread    = 16;
    PGNO        rgpgno[cMaxIndexesToPreread + 1];
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
            rgpgno[ipgno++] = pfcbT->PgnoFDP();
        }
    }

    pfcb->LeaveDML();

    rgpgno[ipgno] = pgnoNull;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = pfcb->TCE();
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );

    BFPrereadPageList( pfcb->Ifmp(), rgpgno, bfprfDefault, pfucb->ppib->BfpriPriority( pfcb->Ifmp() ), *tcScope );
}

class PrereadContext
{
public:
    PrereadContext( PIB * const ppib, FUCB * const pfucb );
    ~PrereadContext();

    ERR ErrPrereadKeys(
        __in_ecount(ckeys) const void * const * const   rgpvKeys,
        __in_ecount(ckeys) const ULONG * const  rgcbKeys,
        const LONG                                      ckeys,
        __out LONG * const                              pckeysPreread,
        const JET_GRBIT                                 grbit );

    ERR ErrPrereadBookmarkRanges(
        __in_ecount(cbm) const BOOKMARK * const rgbmStart,
        __in_ecount(cbm) const BOOKMARK * const rgbmEnd,
        const LONG                              cbm,
        __out LONG * const                      pcbmRangesPreread,
        __in const ULONG                        cPageCacheMin,
        __in const ULONG                        cPageCacheMax,
        const JET_GRBIT                         grbit,
        __out_opt ULONG * const                 pcPageCacheActual );

    ERR ErrPrereadKeyRanges(
        __in_ecount(cRanges) const void *   const * const   rgpvKeysStart,
        __in_ecount(cRanges) const ULONG * const    rgcbKeysStart,
        __in_ecount(cRanges) const void *   const * const   rgpvKeysEnd,
        __in_ecount(cRanges) const ULONG * const    rgcbKeysEnd,
        const LONG                                          cRanges,
        __deref_out_range( 0, cRanges ) LONG * const        pcKeyRangesPreread,
        __in const ULONG                            cPageCacheMin,
        __in const ULONG                            cPageCacheMax,
        const JET_GRBIT                                     grbit,
        __out_opt ULONG * const                     pcPageCacheActual );

private:
    PIB * const m_ppib;
    FUCB * const m_pfucb;
    const IFMP m_ifmp;
    PIBTraceContextScope m_tcScope;

    PGNO * m_rgpgnoPreread;
    CPG m_cpgPrereadAlloc;
    CPG m_cpgPreread;
    CPG m_cpgPrereadMax;

    bool m_fSawFragmentedSpace;

    void CheckSpaceFragmentation( const CSR& csr );
    void CheckSpaceFragmentation_( const CSR& csr );

    ERR ErrProcessSubtreeRangesForward(
        _In_ const BOOKMARK * const                     rgbmStart,
        _In_ const BOOKMARK * const                     rgbmEnd,
        _In_count_(csubtrees) const PGNO * const        rgpgnoSubtree,
        _In_count_(csubtrees) const LONG * const        rgibmMin,
        _In_count_(csubtrees) const LONG * const        rgibmMax,
        const LONG                                      csubtrees,
        _Out_ LONG * const                              pcbmRangesPreread );

    ERR ErrProcessSubtreeRangesBackward(
        _In_ const BOOKMARK * const                     rgbmStart,
        _In_ const BOOKMARK * const                     rgbmEnd,
        _In_count_(csubtrees) const PGNO * const        rgpgnoSubtree,
        _In_count_(csubtrees) const LONG * const        rgibmMin,
        _In_count_(csubtrees) const LONG * const        rgibmMax,
        const LONG                                      csubtrees,
        _Out_ LONG * const                              pcbmRangesPreread );

    ERR ErrPrereadRangesForward(
        const CSR&                                      csr,
        __in_ecount(cbm) const BOOKMARK * const         rgbmStart,
        __in_ecount(cbm) const BOOKMARK * const         rgbmEnd,
        const LONG                                      cbm,
        __out LONG * const                              pcbmRangesPreread );

    ERR ErrPrereadRangesBackward(
        const CSR&                                      csr,
        __in_ecount(cbm) const BOOKMARK * const         rgbmStart,
        __in_ecount(cbm) const BOOKMARK * const         rgbmEnd,
        const LONG                                      cbm,
        __out LONG * const                              pcbmRangesPreread );

    ERR ErrAddPrereadCandidate( const PGNO pgno );

private:
    PrereadContext( const PrereadContext& );
    PrereadContext& operator=( const PrereadContext& );
};

PrereadContext::PrereadContext( PIB * const ppib, FUCB * const pfucb ) :
    m_ppib( ppib ),
    m_pfucb( pfucb ),
    m_ifmp( pfucb->ifmp ),
    m_tcScope( TcBTICreateCtxScope( pfucb, iorsBTPreread ) ),
    m_rgpgnoPreread( NULL ),
    m_cpgPrereadAlloc( 0 ),
    m_cpgPreread( 0 ),
    m_cpgPrereadMax( 0 )
{
}

PrereadContext::~PrereadContext()
{
    delete[] m_rgpgnoPreread;
}

void PrereadContext::CheckSpaceFragmentation( const CSR& csr )
{
    if (    csr.Cpage().FParentOfLeaf() &&
            FBTIEligibleForOLD2( m_pfucb ) )
    {
        CheckSpaceFragmentation_( csr );
    }
}

void PrereadContext::CheckSpaceFragmentation_( const CSR& csr )
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

    sort( rgpgno, rgpgno + clines );

    if( FBTICpgFragmentedRangeTrigger(
        rgpgno,
        clines,
        cpgFragmentedRangeDefragTrigger,
        cpgFragmentedRangeDefrag ) )
    {
        m_fSawFragmentedSpace = true;
    }

HandleError:
    delete[] rgpgno;
}

ERR PrereadContext::ErrProcessSubtreeRangesForward(
    _In_ const BOOKMARK * const                     rgbmStart,
    _In_ const BOOKMARK * const                     rgbmEnd,
    _In_count_(csubtrees) const PGNO * const        rgpgnoSubtree,
    _In_count_(csubtrees) const LONG * const        rgibmMin,
    _In_count_(csubtrees) const LONG * const        rgibmMax,
    const LONG                                      csubtrees,
    _Out_ LONG * const                              pcbmRangesPreread )
{
    ERR err = JET_errSuccess;

    
    CSR csrChild;
    for ( INT isubtree = 0; isubtree < csubtrees && m_cpgPreread < m_cpgPrereadMax; ++isubtree )
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
                pcbmRangesPreread );
        *pcbmRangesPreread += ibmMin;
        csrChild.ReleasePage();
        Call( err );
    }

HandleError:
    return err;
}

ERR PrereadContext::ErrProcessSubtreeRangesBackward(
    _In_ const BOOKMARK * const                     rgbmStart,
    _In_ const BOOKMARK * const                     rgbmEnd,
    _In_count_(csubtrees) const PGNO * const        rgpgnoSubtree,
    _In_count_(csubtrees) const LONG * const        rgibmMin,
    _In_count_(csubtrees) const LONG * const        rgibmMax,
    const LONG                                      csubtrees,
    _Out_ LONG * const                              pcbmRangesPreread )
{
    ERR err = JET_errSuccess;

    
    CSR csrChild;
    for ( INT isubtree = 0; isubtree < csubtrees && m_cpgPreread < m_cpgPrereadMax; ++isubtree )
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
                pcbmRangesPreread );
        *pcbmRangesPreread += ibmMin;
        csrChild.ReleasePage();
        Call( err );
    }

HandleError:
    return err;
}


ERR PrereadContext::ErrPrereadRangesForward(
    const CSR&                                      csr,
    __in_ecount(cbm) const BOOKMARK * const         rgbmStart,
    __in_ecount(cbm) const BOOKMARK * const         rgbmEnd,
    const LONG                                      cbm,
    __out LONG * const                              pcbmRangesPreread )
{
    ERR err = JET_errSuccess;

    if( csr.Cpage().FLeafPage() )
    {
        Assert( csr.Cpage().FFDPPage() );
        *pcbmRangesPreread = cbm;
        return JET_errSuccess;
    }

    Assert( csr.Cpage().FInvisibleSons() );

    CheckSpaceFragmentation( csr );


    const INT csubtreePrereadMax = 16;
    PGNO rgpgnoSubtree[csubtreePrereadMax+1];
    LONG rgibmMin[csubtreePrereadMax];
    LONG rgibmMax[csubtreePrereadMax];
    INT isubtree = 0;
    *pcbmRangesPreread = 0;
    
    INT ibm = 0;

    for(
        INT iline = 0;
        iline < csr.Cpage().Clines() && ibm < cbm && m_cpgPreread < m_cpgPrereadMax;
        ++iline )
    {
        INT ibmMin = 0;
        INT ibmMax = 0;

        KEYDATAFLAGS kdf;

        if( iline < ( csr.Cpage().Clines() - 1 ) )
        {
            NDIGetKeydataflags( csr.Cpage(), iline, &kdf );
            Assert( kdf.key.Cb() > 0 );
            if( CmpKeyWithKeyData( kdf.key, rgbmStart[ibm] ) <= 0 )
            {
                INT compareT = 0;
                INT ilineT = IlineNDISeekGEQInternal( csr.Cpage(), rgbmStart[ibm], &compareT );
                if ( 0 == compareT && ilineT < ( csr.Cpage().Clines() - 1 ) )
                {
                    ilineT++;
                }

#ifdef DEBUG
                KEYDATAFLAGS kdfDebug;
                if ( ilineT < ( csr.Cpage().Clines() - 1 ) )
                {
                    NDIGetKeydataflags( csr.Cpage(), ilineT, &kdfDebug );
                    Assert( kdfDebug.key.Cb() > 0 );
                    Assert( CmpKeyWithKeyData( kdfDebug.key, rgbmStart[ibm] ) > 0 );
                }
                if ( ilineT > 0 )
                {
                    NDIGetKeydataflags( csr.Cpage(), ilineT - 1, &kdfDebug );
                    Assert( kdfDebug.key.Cb() > 0 );
                    Assert( CmpKeyWithKeyData( kdfDebug.key, rgbmStart[ibm] ) <= 0 );
                }
#endif
                Assert( ilineT > iline );
                iline = ilineT;
            }
        }
        
        if( ( csr.Cpage().Clines() - 1 ) == iline )
        {
            ibmMin  = ibm;
            ibmMax  = cbm;
            ibm     = cbm;
        }
        else
        {
            NDIGetKeydataflags( csr.Cpage(), iline, &kdf );
            Assert( kdf.key.Cb() > 0 );

            if( CmpKeyWithKeyData( kdf.key, rgbmStart[ibm] ) > 0 )
            {
                
                ibmMin = ibm;
                
                while( ibm < cbm )
                {
                    if( CmpKeyWithKeyData( kdf.key, rgbmEnd[ibm] ) <= 0 )
                    {
                        break;
                    }
                    ibm++;
                }

                ibmMax = ibm;
                while ( ibmMax < cbm )
                {
                    if ( CmpKeyWithKeyData( kdf.key, rgbmStart[ibmMax] ) <= 0 )
                    {
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
            Assert( 0 == ibmMin );
        }
        else
        {
            Assert( ibmMin < ibmMax );
            
            NDIGetKeydataflags( csr.Cpage(), iline, &kdf );
            
            Assert( sizeof( PGNO ) == kdf.data.Cb() );
            const PGNO pgno = *(UnalignedLittleEndian< PGNO > *) kdf.data.Pv();
            Assert( pgnoNull != pgno );
            
            if( csr.Cpage().FParentOfLeaf() )
            {
                Call( ErrAddPrereadCandidate( pgno ) );
                *pcbmRangesPreread = ibm;
            }
            else
            {

                DBEnforce( csr.Cpage().Ifmp(), isubtree < _countof( rgibmMin ) );
                DBEnforce( csr.Cpage().Ifmp(), isubtree < _countof( rgibmMax ) );
                rgibmMin[isubtree] = ibmMin;
                rgibmMax[isubtree] = ibmMax;
                rgpgnoSubtree[isubtree] = pgno;
                isubtree++;

                Assert( isubtree <= csubtreePrereadMax );
                if( csubtreePrereadMax == isubtree )
                {
                    rgpgnoSubtree[isubtree] = pgnoNull;

                    BFPrereadPageList( m_ifmp, rgpgnoSubtree, bfprfDefault, m_ppib->BfpriPriority( m_ifmp ), TcCurr() );
                    Call( ErrProcessSubtreeRangesForward(
                            rgbmStart,
                            rgbmEnd,
                            rgpgnoSubtree,
                            rgibmMin,
                            rgibmMax,
                            isubtree,
                            pcbmRangesPreread ) );
                    isubtree = 0;
                }
            }
        }
    }

    AssumePREFAST( isubtree < csubtreePrereadMax );
    if( isubtree > 0 )
    {
        rgpgnoSubtree[isubtree] = pgnoNull;

        BFPrereadPageList( m_ifmp, rgpgnoSubtree, bfprfDefault, m_ppib->BfpriPriority( m_ifmp ), TcCurr() );
        Call( ErrProcessSubtreeRangesForward(
                rgbmStart,
                rgbmEnd,
                rgpgnoSubtree,
                rgibmMin,
                rgibmMax,
                isubtree,
                pcbmRangesPreread ) );
        isubtree = 0;
    }

HandleError:
    return err;
}

ERR PrereadContext::ErrPrereadRangesBackward(
    const CSR&                                      csr,
    __in_ecount(cbm) const BOOKMARK * const         rgbmStart,
    __in_ecount(cbm) const BOOKMARK * const         rgbmEnd,
    const LONG                                      cbm,
    __out LONG * const                              pcbmRangesPreread )
{
    ERR err = JET_errSuccess;
    
    if( csr.Cpage().FLeafPage() )
    {
        Assert( csr.Cpage().FFDPPage() );
        *pcbmRangesPreread = cbm;
        return JET_errSuccess;
    }

    Assert( csr.Cpage().FInvisibleSons() );

    CheckSpaceFragmentation(csr);


    const INT csubtreePrereadMax = 16;
    PGNO rgpgnoSubtree[csubtreePrereadMax+1];
    LONG rgibmMin[csubtreePrereadMax];
    LONG rgibmMax[csubtreePrereadMax];
    INT isubtree = 0;
    *pcbmRangesPreread = 0;
    
    INT ibm = 0;

    for(
        INT iline = csr.Cpage().Clines() - 1;
        iline >= 0 && ibm < cbm && m_cpgPreread < m_cpgPrereadMax;
        --iline )
    {
        INT ibmMin = 0;
        INT ibmMax = 0;

        KEYDATAFLAGS kdfPrev;

        if( iline > 0 )
        {
            NDIGetKeydataflags( csr.Cpage(), iline-1, &kdfPrev );
            Assert( kdfPrev.key.Cb() > 0 );
            
            if( CmpKeyWithKeyData( kdfPrev.key, rgbmStart[ibm] ) > 0 )
            {
                INT compareT = 0;
                INT ilineT = IlineNDISeekGEQInternal( csr.Cpage(), rgbmStart[ibm], &compareT );
                if ( 0 == compareT && ilineT < ( csr.Cpage().Clines() - 1 ) )
                {
                    ilineT++;
                }

#ifdef DEBUG
                KEYDATAFLAGS kdfDebug;
                if ( ilineT < ( csr.Cpage().Clines() - 1 ) )
                {
                    NDIGetKeydataflags( csr.Cpage(), ilineT, &kdfDebug );
                    Assert( kdfDebug.key.Cb() > 0 );
                    Assert( CmpKeyWithKeyData( kdfDebug.key, rgbmStart[ibm] ) > 0 );
                }
                if ( ilineT > 0 )
                {
                    NDIGetKeydataflags( csr.Cpage(), ilineT - 1, &kdfDebug );
                    Assert( kdfDebug.key.Cb() > 0 );
                    Assert( CmpKeyWithKeyData( kdfDebug.key, rgbmStart[ibm] ) <= 0 );
                }
#endif
                Assert( ilineT < iline );
                iline = ilineT;
            }
        }
        
        if( 0 == iline )
        {
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
                
                ibmMin = ibm;
                
                while( ibm < cbm )
                {
                    if( CmpKeyWithKeyData( kdfPrev.key, rgbmEnd[ibm] ) > 0 )
                    {
                        break;
                    }
                    ibm++;
                }

                ibmMax = ibm;
                while ( ibmMax < cbm )
                {
                    if ( CmpKeyWithKeyData( kdfPrev.key, rgbmStart[ibmMax] ) > 0 )
                    {
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
            Assert( 0 == ibmMin );
        }
        else
        {
            Assert( ibmMin < ibmMax );
            
            KEYDATAFLAGS kdf;
            NDIGetKeydataflags( csr.Cpage(), iline, &kdf );
            
            Assert( sizeof( PGNO ) == kdf.data.Cb() );
            const PGNO pgno = *(UnalignedLittleEndian< PGNO > *) kdf.data.Pv();
            Assert( pgnoNull != pgno );
            
            if( csr.Cpage().FParentOfLeaf() )
            {
                Call( ErrAddPrereadCandidate( pgno ) );
                *pcbmRangesPreread = ibm;
            }
            else
            {

                DBEnforce( csr.Cpage().Ifmp(), isubtree < _countof( rgibmMin ) );
                DBEnforce( csr.Cpage().Ifmp(), isubtree < _countof( rgibmMax ) );
                rgpgnoSubtree[isubtree] = pgno;
                rgibmMin[isubtree] = ibmMin;
                rgibmMax[isubtree] = ibmMax;
                isubtree++;

                Assert( isubtree <= csubtreePrereadMax );
                if( csubtreePrereadMax == isubtree )
                {
                    rgpgnoSubtree[isubtree] = pgnoNull;

                    BFPrereadPageList( m_ifmp, rgpgnoSubtree, bfprfDefault, m_ppib->BfpriPriority( m_ifmp ), TcCurr() );
                    Call( ErrProcessSubtreeRangesBackward(
                            rgbmStart,
                            rgbmEnd,
                            rgpgnoSubtree,
                            rgibmMin,
                            rgibmMax,
                            isubtree,
                            pcbmRangesPreread ) );
                    isubtree = 0;
                }
            }
        }
    }

    AssumePREFAST( isubtree < csubtreePrereadMax );
    if( isubtree > 0 )
    {
        rgpgnoSubtree[isubtree] = pgnoNull;

        BFPrereadPageList( m_ifmp, rgpgnoSubtree, bfprfDefault, m_ppib->BfpriPriority( m_ifmp ), TcCurr() );
        Call( ErrProcessSubtreeRangesBackward(
                rgbmStart,
                rgbmEnd,
                rgpgnoSubtree,
                rgibmMin,
                rgibmMax,
                isubtree,
                pcbmRangesPreread ) );
        isubtree = 0;
    }

HandleError:
    return err;
}

ERR PrereadContext::ErrAddPrereadCandidate( const PGNO pgno )
{
    ERR err = JET_errSuccess;

    const CPG cpgPrereadAllocMin = 128;
    const CPG cpgPrereadNew = m_cpgPreread + 1;
    if ( m_rgpgnoPreread == NULL || m_cpgPrereadAlloc < cpgPrereadNew )
    {
        CPG     cpgPrereadAllocNew  = max( cpgPrereadAllocMin, max( cpgPrereadNew, m_cpgPrereadAlloc * 2 ) );
        PGNO*   rgpgnoPrereadNew    = NULL;
        Alloc( rgpgnoPrereadNew = new PGNO[cpgPrereadAllocNew] );
        if ( m_rgpgnoPreread != NULL )
        {
            memcpy( rgpgnoPrereadNew, m_rgpgnoPreread, sizeof( PGNO ) * m_cpgPrereadAlloc );
            delete[] m_rgpgnoPreread;
        }
        m_cpgPrereadAlloc = cpgPrereadAllocNew;
        m_rgpgnoPreread = rgpgnoPrereadNew;
    }

    m_rgpgnoPreread[m_cpgPreread++] = pgno;

HandleError:
    return err;
}

ERR PrereadContext::ErrPrereadKeys(
    __in_ecount(ckeys) const void * const * const   rgpvKeys,
    __in_ecount(ckeys) const ULONG * const  rgcbKeys,
    const LONG                                      ckeys,
    __out LONG * const                              pckeysPreread,
    const JET_GRBIT                                 grbit )
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

ERR ErrBTPrereadKeys(
    PIB * const                                     ppib,
    FUCB * const                                    pfucb,
    __in_ecount(ckeys) const void * const * const   rgpvKeys,
    __in_ecount(ckeys) const ULONG * const  rgcbKeys,
    const LONG                                      ckeys,
    __out LONG * const                              pckeysPreread,
    const JET_GRBIT                                 grbit )
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


ERR  ErrBTPrereadBookmarks(
    PIB * const                                     ppib,
    FUCB * const                                    pfucb,
    __in_ecount(cbm) const BOOKMARK * const         rgbm,
    const LONG                                      cbm,
    __out LONG * const                              pcbmPreread,
    const JET_GRBIT                                 grbit )
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


ERR PrereadContext::ErrPrereadBookmarkRanges(
    __in_ecount(cbm) const BOOKMARK * const         rgbmStart,
    __in_ecount(cbm) const BOOKMARK * const         rgbmEnd,
    const LONG                                      cbm,
    __out LONG * const                              pcbmRangesPreread,
    __in const ULONG                        cPageCacheMin,
    __in const ULONG                        cPageCacheMax,
    const JET_GRBIT                                 grbit,
    __out_opt ULONG * const                 pcPageCacheActual )
{
    ERR     err             = JET_errSuccess;
    CSR     csrRoot;
    BOOL    fEligibleForOLD2 = !( grbit & bitPrereadDoNotDoOLD2 );

    *pcbmRangesPreread      = 0;
    m_fSawFragmentedSpace   = false;

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

    CPG cpgPrereadMax = (CPG)cPageCacheMax;
    if ( grbit & bitPrereadSingletonRanges )
    {
        cpgPrereadMax = min( cpgPrereadMax, (CPG)cbm );
    }
    const CPG cpgPrereadMin = min( (CPG)cPageCacheMin, cpgPrereadMax );

    if ( cpgPrereadMax == 0 )
    {
        *pcbmRangesPreread = 1;
        return JET_errSuccess;
    }

    m_cpgPrereadMax = cpgPrereadMax;

    Call( csrRoot.ErrGetReadPage(
            m_ppib,
            m_ifmp,
            PgnoRoot( m_pfucb ),
            bflfDefault,
            PBFLatchHintPgnoRoot( m_pfucb )
            ) );

    if( fForward )
    {
        Call( ErrPrereadRangesForward(
                csrRoot,
                rgbmStart,
                rgbmEnd,
                cbm,
                pcbmRangesPreread ) );
    }
    else
    {
        Call( ErrPrereadRangesBackward(
                csrRoot,
                rgbmStart,
                rgbmEnd,
                cbm,
                pcbmRangesPreread ) );
    }

    Call( ErrAddPrereadCandidate( pgnoNull ) );

{
    BFReserveAvailPages bfreserveavailpages( m_cpgPreread - 1 );
    CPG cpgPrereadRequested = min( m_cpgPreread - 1, max( cpgPrereadMin, bfreserveavailpages.CpgReserved() ) );
    
    if ( pcPageCacheActual )
    {
        *pcPageCacheActual = cpgPrereadRequested;
    }

    PGNO pgnoPrereadSave = m_rgpgnoPreread[cpgPrereadRequested];
    m_rgpgnoPreread[cpgPrereadRequested] = pgnoNull;

    GetCurrUserTraceContext getutc;
    auto tc = TcCurr();
    BFPrereadPageList( m_ifmp, m_rgpgnoPreread, bfprfDefault, m_ppib->BfpriPriority( m_ifmp ), tc );

    m_rgpgnoPreread[cpgPrereadRequested] = pgnoPrereadSave;

    for ( CPG ipg = 0; ipg < m_cpgPreread - 1; ipg++ )
    {
        const BYTE fOpFlags = ipg < cpgPrereadRequested && FBFInCache( m_ifmp, m_rgpgnoPreread[ipg] ) ? 0 : 1;

        ETBTreePrereadPageRequest(
            m_ifmp,
            m_rgpgnoPreread[ipg],
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

    if( m_fSawFragmentedSpace && fEligibleForOLD2 )
    {
        (void)ErrBTIRegisterForOLD2( m_pfucb );
    }

    if ( *pcbmRangesPreread == 0 )
    {
        *pcbmRangesPreread = 1;
    }

HandleError:
    csrRoot.ReleasePage();
    return err;
}

ERR PrereadContext::ErrPrereadKeyRanges(
    __in_ecount(cRanges) const void *   const * const   rgpvKeysStart,
    __in_ecount(cRanges) const ULONG * const    rgcbKeysStart,
    __in_ecount(cRanges) const void *   const * const   rgpvKeysEnd,
    __in_ecount(cRanges) const ULONG * const    rgcbKeysEnd,
    const LONG                                          cRanges,
    __deref_out_range( 0, cRanges ) LONG * const        pcRangesPreread,
    __in const ULONG                            cPageCacheMin,
    __in const ULONG                            cPageCacheMax,
    const JET_GRBIT                                     grbit,
    __out_opt ULONG * const                     pcPageCacheActual )
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

ERR ErrBTPrereadKeyRanges(
    PIB * const                                     ppib,
    FUCB * const                                    pfucb,
    __in_ecount(ckeys) const void * const * const   rgpvKeysStart,
    __in_ecount(ckeys) const ULONG * const  rgcbKeysStart,
    __in_ecount(ckeys) const void * const * const   rgpvKeysEnd,
    __in_ecount(ckeys) const ULONG * const  rgcbKeysEnd,
    const LONG                                      ckeys,
    __deref_out_range( 0, ckeys ) LONG * const      pcRangesPreread,
    __in const ULONG                        cPageCacheMin,
    __in const ULONG                        cPageCacheMax,
    const JET_GRBIT                                 grbit,
    __out_opt ULONG * const                 pcPageCacheActual )
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


VOID BTIPrereadContiguousPages(
    const IFMP ifmp,
    const CSR& csr,
    PrereadInfo * const pPrereadInfo,
    const BFPreReadFlags bfprf,
    const BFPriority bfpri,
    const TraceContext &tc )
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

    Assert( FParentObjectClassSet( tc.nParentObjectClass ) );
    BFPrereadPageRange( ifmp, pgnoFirst, cpg, &pPrereadInfo->cpgActuallyPreread, pPrereadInfo->rgfPageWasAlreadyCached, bfprf, bfpri, tc );
}

ERR ErrBTIPreread( FUCB *pfucb, CPG cpg, CPG * pcpgActual )
{
#ifdef DEBUG
    const INT   ilineOrig = Pcsr( pfucb )->ILine();

    Unused( ilineOrig );
#endif

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
        *pcpgActual = 0;
        return JET_errSuccess;
    }

    Assert( !FFUCBLongValue( pfucb ) || FFUCBLimstat( pfucb ) );

    Assert( ( cpgPreread + 1 ) * sizeof( PGNO ) <= (size_t)g_rgfmp[ pfucb->ifmp ].CbPage() );
    PGNO * rgpgnoPreread;
    BFAlloc( bfasTemporary, (void**)&rgpgnoPreread );

    KEY     keyLimit;
    BOOL    fOutOfRange = fFalse;
    if( FFUCBLimstat( pfucb ) )
    {

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


            if( fOutOfRange )
            {
                break;
            }


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

    auto tc = TcCurr();
    Assert( FParentObjectClassSet( tc.nParentObjectClass ) );
    BFPrereadPageList( pfucb->u.pfcb->Ifmp(), rgpgnoPreread, pcpgActual, bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->u.pfcb->Ifmp() ), tc );

    BFFree( rgpgnoPreread );
    return JET_errSuccess;
}

#define  dwPagesMaxOpportuneRead  8

ERR ErrBTIOpportuneRead( FUCB *pfucb, PGNO pgnoChild )
{
    CSR * const pcsr            = Pcsr( pfucb );
    Assert( pcsr->Cpage().FParentOfLeaf() );

    if ( !FBFInCache( pfucb->ifmp, pgnoChild ) )
    {
        BYTE    rgbForward[dwPagesMaxOpportuneRead];
        BYTE    rgbBackward[dwPagesMaxOpportuneRead];
        memset(rgbForward, 0, sizeof(rgbForward));
        memset(rgbBackward, 0, sizeof(rgbBackward));
        
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

        PGNO    pgnoMin = pgnoChild;
        PGNO    pgnoMost = pgnoChild;
        DWORD   dwPagesTotal = 1;
        
        for (PGNO pgnoLoop = pgnoChild + 1;
                dwPagesTotal < dwPagesMaxOpportuneRead;
                    ++pgnoLoop)
        {
            if ( FBFInCache( pfucb->ifmp, pgnoLoop) )
                break;

            if ( rgbForward[pgnoLoop - pgnoChild] == 0)
                break;

            pgnoMost = pgnoLoop;
            ++dwPagesTotal;
        }
        
        for (PGNO pgnoLoop = pgnoChild - 1;
                dwPagesTotal < dwPagesMaxOpportuneRead;
                    --pgnoLoop)
        {
            if ( FBFInCache( pfucb->ifmp, pgnoLoop) )
                break;

            if ( rgbBackward[pgnoChild - pgnoLoop] == 0)
                break;

            pgnoMin = pgnoLoop;
            ++dwPagesTotal;
        }

        if ( dwPagesTotal >= 2 )
        {
            PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
            tcScope->iorReason.AddFlag( iorfOpportune );
            Assert( FParentObjectClassSet( tcScope->nParentObjectClass ) );

            PERFOpt( cBTOpportuneReads.Add( PinstFromPfucb( pfucb )->m_iInstance, (TCE)tcScope->nParentObjectClass, dwPagesTotal - 1 ) );

            BFPrereadPageRange( pfucb->ifmp, pgnoMin, dwPagesTotal, bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->ifmp ), *tcScope );
        }
    }
    return JET_errSuccess;
}


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
    CHECK( key != NULL );
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
    CHECKCALLS( key.ErrAlloc() );
    CHECK( key != NULL );
    CHECK( key.CbMax() == cbKeyAlloc );
}


ERR ErrBTDown( FUCB *pfucb, DIB *pdib, LATCH latch )
{
    ERR         err                         = JET_errSuccess;
    ERR         wrn                         = JET_errSuccess;
    CSR * const pcsr                        = Pcsr( pfucb );
    PGNO        pgnoParent                  = pgnoNull;
    const BOOL  fSeekOnNonUniqueKeyOnly     = ( posDown == pdib->pos
                                                && !FFUCBUnique( pfucb )
                                                && 0 == pdib->pbm->data.Cb() );
    PIBTraceContextScope tcScope        = TcBTICreateCtxScope( pfucb, iorsBTSeek );

#ifdef CHECK_UNIQUE_KEY_ON_NONUNIQUE_INDEX
    BOOL        fKeyIsolatedToCurrentPage   = fFalse;
    const BOOL  fCheckUniqueness            = ( ( pdib->dirflag & fDIRCheckUniqueness )
                                                && ( pdib->dirflag & fDIRExact )
                                                && fSeekOnNonUniqueKeyOnly );
#endif

#ifdef DEBUG
    ULONG       ulTrack                     = 0x00;

    const LONG cRFSCountdownOld = RFSThreadDisable( 0 );
    BOOL fNeedReEnableRFS = fTrue;
    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );


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
            Call( keySeekData.ErrAlloc() );
            Assert( pdib->pbm->data.Cb() <= keySeekData.CbMax() );
            memcpy( keySeekData, pdib->pbm->data.Pv(), pdib->pbm->data.Cb() );
            bmSeek.data.SetPv( keySeekData );
            bmSeek.data.SetCb( pdib->pbm->data.Cb() );
            Assert( FDataEqual( bmSeek.data, pdib->pbm->data ) );
        }
    }
    
    FOSSetCleanupState( fCleanUpStateSaved );
    RFSThreadReEnable( cRFSCountdownOld );
    fNeedReEnableRFS = fFalse;
#endif

    if ( pfucb->ppib->FReadOnlyTrx() && pfucb->ppib->Level() > 0 )
    {
        Call( PverFromIfmp( pfucb->ifmp )->ErrVERCheckTransactionSize( pfucb->ppib ) );
    }

    Assert( FBTLogicallyNavigableState( pfucb ) ||
                ( FSPIsRootSpaceTree( pfucb ) &&
                    g_rgfmp[ pfucb->ifmp ].Pdbfilehdr()->Dbstate() != JET_dbstateDirtyAndPatchedShutdown )
            );

    Assert( latchReadTouch == latch || latchReadNoTouch == latch ||
            latchRIW == latch );
    Assert( posDown == pdib->pos
            || 0 == ( pdib->dirflag & (fDIRFavourPrev|fDIRFavourNext|fDIRExact) ) );

    Assert( !pcsr->FLatched() );

    PERFOpt( PERFIncCounterTable( cBTSeek, PinstFromPfucb( pfucb ), (TCE)tcScope->nParentObjectClass ) );

    Call( ErrBTIGotoRoot( pfucb, latch ) );

    if ( 0 == pcsr->Cpage().Clines() )
    {
        BTUp( pfucb );
        return ErrERRCheck( JET_errRecordNotFound );
    }

    CallS( err );

#ifdef PREREAD_ON_INITIAL_SEEK
    if ( FFUCBSequential( pfucb ) && !FFUCBPreread( pfucb ) )
    {
        if ( posFirst == pdib->pos )
        {
            FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
        }
        else if ( posLast == pdib->pos )
        {
            FUCBSetPrereadBackward( pfucb, cpgPrereadSequential );
        }
    }
#endif

    pfucb->ulLTLast = 0;
    pfucb->ulTotalLast = 1;

    for ( ; ; )
    {
        if ( pcsr->Cpage().ObjidFDP() != pfucb->u.pfcb->ObjidFDP() )
        {
            AssertSz( g_fRepair, "Corrupt B-tree: page does not belong to btree" );
            Call( ErrBTIReportBadPageLink(
                        pfucb,
                        ErrERRCheck( JET_errBadParentPageLink ),
                        pgnoParent,
                        pcsr->Pgno(),
                        pcsr->Cpage().ObjidFDP(),
                        fTrue,
                        "BtDownObjid" ) );
        }

        if ( pcsr->Cpage().Clines() <= 0 )
        {
            AssertSz( g_fRepair, "Corrupt B-tree: page found was empty" );
            Call( ErrBTIReportBadPageLink(
                        pfucb,
                        ErrERRCheck( JET_errBadParentPageLink ),
                        pgnoParent,
                        pcsr->Pgno(),
                        pcsr->Cpage().ObjidFDP(),
                        fTrue,
                        pcsr->Cpage().FPreInitPage() ?
                            "BtDownClinesLowPreinit" :
                            ( pcsr->Cpage().FEmptyPage() ?
                                "BtDownClinesLowEmpty" :
                                "BtDownClinesLowInPlace" ) ) );
        }

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
                pcsr->SetILine( IlineBTIFrac( pfucb, pdib ) );
                NDGet( pfucb );
                break;
        }


        const INT   iline   = pcsr->ILine();
        const INT   clines  = pcsr->Cpage().Clines();

        pfucb->ulLTLast = pfucb->ulLTLast * clines + iline;
        pfucb->ulTotalLast = pfucb->ulTotalLast * clines;

        if ( pcsr->Cpage().FLeafPage() )
        {
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

            Assert( !fSeekOnNonUniqueKeyOnly
                || pfucb->kdfCurr.key.FNull()
                || CmpKeyShortest( pfucb->kdfCurr.key, pdib->pbm->key ) > 0
                || ( CmpKeyShortest( pfucb->kdfCurr.key, pdib->pbm->key ) == 0
                    && pfucb->kdfCurr.key.Cb() > pdib->pbm->key.Cb() ) );

#ifdef CHECK_UNIQUE_KEY_ON_NONUNIQUE_INDEX
            if ( fPageParentOfLeaf && fCheckUniqueness )
            {
                fKeyIsolatedToCurrentPage = ( pfucb->kdfCurr.key.FNull()
                                            || CmpKeyShortest( pfucb->kdfCurr.key, pdib->pbm->key ) > 0 );
            }
#endif

            Assert( pfucb->kdfCurr.data.Cb() == sizeof( PGNO ) );
            const PGNO  pgnoChild           = *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv();

            if ( fPageParentOfLeaf && FFUCBOpportuneRead( pfucb ) )
            {
                Call( ErrBTIOpportuneRead( pfucb, pgnoChild ) );
            }

            if ( FFUCBPreread( pfucb ) )
            {

                if ( fPageParentOfLeaf )
                {
                    if ( 0 == pfucb->cpgPrereadNotConsumed && pfucb->cpgPreread > 1 )
                    {
                        Call( ErrBTIPreread( pfucb, pfucb->cpgPreread, &pfucb->cpgPrereadNotConsumed) );
                    }
                }
                else if ( ( FFUCBSequential( pfucb ) || FFUCBLimstat( pfucb ) )
                        && FFUCBPrereadForward( pfucb ) )
                {
                    CPG cpgUnused;
                    Call( ErrBTIPreread( pfucb, 2, &cpgUnused ) );
                }

                pcsr->SetILine( iline );
            }

            pgnoParent = pcsr->Pgno();

            Call( pcsr->ErrSwitchPage(
                        pfucb->ppib,
                        pfucb->ifmp,
                        pgnoChild,
                        pfucb->u.pfcb->FNoCache()  ) );

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

    Assert( pcsr->Cpage().FLeafPage() );
    AssertBTType( pfucb );
    AssertNDGet( pfucb );

    if ( posFirst == pdib->pos && pgnoNull != pcsr->Cpage().PgnoPrev() )
    {
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
                OnDebug( ulTrack |= 0x08 );

                wrn = wrnNDFoundGreater;
                err = ErrBTNext( pfucb, fDIRNull );
            }
            else if ( ( pdib->dirflag & fDIRFavourPrev ) || posLast == pdib->pos )
            {
                OnDebug( ulTrack |= 0x10 );


                bool fFoundExact = false;
                if ( fSeekOnNonUniqueKeyOnly )
                {
                    OnDebug( ulTrack |= 0x1000 );

                    err = ErrBTNext( pfucb, fDIRNull );
                    if ( JET_errSuccess == err && FKeysEqual( pfucb->kdfCurr.key, pdib->pbm->key ) )
                    {
                        OnDebug( ulTrack |= 0x2000 );

                        fFoundExact = true;
                        err = JET_errSuccess;
                    }
                    else if ( JET_errNoCurrentRecord != err )
                    {
                        Call( err );
                    }
                }

                if ( !fFoundExact )
                {
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

            Assert( wrnNDFoundGreater != err );
            Assert( wrnNDFoundLess != err );

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
#endif
                }
            }
        }
    }
    else
    {
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
                cmp     = CmpNDKeyOfNextNode( pcsr, pdib->pbm->key );
                Assert( cmp >= 0 );
                if ( cmp > 0 )
                {
                    err = ErrERRCheck( JET_wrnUniqueKey );
                }
            }
            else
            {
                if ( fKeyIsolatedToCurrentPage || pgnoNull == pcsr->Cpage().PgnoNext() )
                {
                    err = ErrERRCheck( JET_wrnUniqueKey );
                }
            }
        }
#endif
    }


    Assert( pcsr->Cpage().FLeafPage() );
    AssertBTType( pfucb );
    AssertNDGet( pfucb );
    Assert( err >= 0 );

    return err;

HandleError:

#ifdef  DEBUG
    FOSSetCleanupState( fCleanUpStateSaved );
    if ( fNeedReEnableRFS )
    {
        RFSThreadReEnable( cRFSCountdownOld );
    }
#endif

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
        pfucb->locLogical = ( fDIRFavourPrev == dirflag ? locBeforeFirst : locAfterLast );
    }

    Assert( err < 0 || Pcsr( pfucb )->FLatched() );
    return err;
}




ERR ErrBTGetPosition( FUCB *pfucb, ULONG *pulLT, ULONG *pulTotal )
{
    ERR     err;
    UINT    ulLT = 0;
    UINT    ulTotal = 1;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTSeek );

    Assert( !Pcsr( pfucb )->FLatched() );

    if ( pfucb->locLogical == locOnSeekBM )
    {
        pfucb->ulLTCurr = 0;
        pfucb->ulTotalCurr = 0;
    }
    else
    {
        Assert( locOnCurBM == pfucb->locLogical );
    }

    if ( pfucb->ulTotalCurr )
    {
        *pulLT = pfucb->ulLTCurr;
        *pulTotal = pfucb->ulTotalCurr;
        return JET_errSuccess;
    }

    PERFOpt( PERFIncCounterTable( cBTSeek, PinstFromPfucb( pfucb ), (TCE)tcScope->nParentObjectClass ) );

    CallR( ErrBTIGotoRoot( pfucb, latchReadTouch ) );

    for ( ; ; )
    {
        INT     clines = Pcsr( pfucb )->Cpage().Clines();

        Call( ErrNDSeek( pfucb, pfucb->bmCurr ) );

        ulLT = ulLT * clines + Pcsr( pfucb )->ILine();
        ulTotal = ulTotal * clines;

        if ( !Pcsr( pfucb )->Cpage().FInvisibleSons( ) )
        {
            break;
        }
        else
        {
            Assert( pfucb->kdfCurr.data.Cb() == sizeof( PGNO ) );
            Call( Pcsr( pfucb )->ErrSwitchPage(
                                pfucb->ppib,
                                pfucb->ifmp,
                                *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv(),
                                pfucb->u.pfcb->FNoCache()
                                ) );
        }
    }

    *pulLT = ulLT;
    *pulTotal = ulTotal;
    Assert( ulTotal >= ulLT );
    
    err = JET_errSuccess;

HandleError:
    BTUp( pfucb );
    return err;
}


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
        BTUp( pfucb );
        Assert( !Pcsr( pfucb )->FLatched() );
        err = ErrERRCheck( JET_errRecordDeleted );
    }

    return err;
}


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
    AssertRTL( err >= JET_errSuccess );
    CallR( err );

    if ( wrnNDFoundLess == err &&
             Pcsr( pfucb )->Cpage().Clines() - 1 == Pcsr( pfucb )->ILine() ||
         wrnNDFoundGreater == err &&
             0 == Pcsr( pfucb )->ILine() )
    {
        err = ErrERRCheck( wrnNDNotFoundInPage );
    }

    return err;
}


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

    Call( ErrBTIGotoRoot( pfucb, latchReadTouch ) );
    fLatched = fTrue;

    pgnoRoot = pcsr->Pgno();
    pgnoCurrent = pgnoRoot;
    if ( pgnoCurrent == pgno )
    {
        Expected( fFalse );
        goto HandleError;
    }

    if ( pcsr->Cpage().Clines() == 0 )
    {
        Error( ErrERRCheck( JET_errRecordNotFound ) );
    }

    while ( fTrue )
    {
        if ( pcsr->Cpage().FLeafPage() ||
            ( pcsr->Cpage().FParentOfLeaf() && !fLeafPage ) )
        {
            Error( ErrERRCheck( JET_errRecordNotFound ) );
        }

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

        if ( fLeafPage )
        {
            err = ErrNDSeek( pfucb, bm );
        }
        else
        {
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



ERR ErrBTLock( FUCB *pfucb, DIRLOCK dirlock )
{
    Assert( dirlock == writeLock
            || dirlock == readLock );

    const BOOL  fVersion = !g_rgfmp[pfucb->ifmp].FVersioningOff();
    Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() );

    ERR     err = JET_errSuccess;

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


LOCAL ERR ErrBTIResetPrefixIfUnused( FUCB * const pfucb, CSR * const pcsr )
{
    ERR err = JET_errSuccess;

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
        DATA dataNull;
        dataNull.Nullify();
        Call( ErrNDSetExternalHeader( pfucb, pcsr, &dataNull, fDIRNull, noderfWhole ) );
    }

HandleError:
    return err;
}

ERR ErrBTReplace( FUCB * const pfucb, const DATA& data, const DIRFLAG dirflag )
{
    Assert( !FFUCBSpace( pfucb ) || fDIRNoVersion & dirflag );

    Assert( FBTLogicallyNavigableState( pfucb ) );

    ERR     err;
    LATCH   latch       = latchReadNoTouch;
    INT     cbDataOld   = 0;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTReplace );

    PERFOpt( PERFIncCounterTable( cBTReplace, PinstFromPfucb( pfucb ), (TCE)tcScope->nParentObjectClass ) );

    if ( Pcsr( pfucb )->FLatched() )
    {
        CallR( ErrBTISaveBookmark( pfucb ) );
    }

#ifdef DEBUG

    const LONG cRFSCountdownOld = RFSThreadDisable( 0 );
    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

    CAutoKey Key, Data, Key2;
    
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

    Assert( FFUCBUnique( pfucb ) );

#ifdef DEBUG
    if ( ( latchReadTouch == latch || latchReadNoTouch == latch )
        && ( latchReadTouch == Pcsr( pfucb )->Latch() || latchReadNoTouch == Pcsr( pfucb )->Latch() ) )
    {
    }
    else if ( latch == Pcsr( pfucb )->Latch() )
    {
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

    cbDataOld = pfucb->kdfCurr.data.Cb();
    err = ErrNDReplace( pfucb, &data, dirflag, rceidReplace, prceReplace );
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

        Assert( data.Cb() > pfucb->kdfCurr.data.Cb() );

        AssertNDGet( pfucb );

        kdf.Nullify();
        kdf.data = data;
        Assert( 0 == kdf.fFlags );

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
            Assert( !Pcsr( pfucb )->FLatched() );
            prceReplace = prceNil;
            latch = latchRIW;
            goto Start;
        }
    }

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
    BTUp( pfucb );

    return err;
}


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

    if( fVersion )
    {
        _VERDELTA< TDelta > verdelta;
        verdelta.tDelta             = 0;
        verdelta.cbOffset           = (USHORT)cbOffset;
        verdelta.fDeferredDelete    = fFalse;
        verdelta.fCallbackOnZero    = fFalse;
        verdelta.fDeleteOnZero      = fFalse;

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


    kdf.data    = data;
    kdf.key     = key;
    kdf.fFlags  = 0;
    ASSERT_VALID( &kdf );

    NDGetBookmarkFromKDF( pfucb, kdf, &bmSearch );

    Assert( !Pcsr( pfucb )->FLatched( ) );

    Call( ErrBTIGotoRoot( pfucb, latch ) );
    Assert( 0 == Pcsr( pfucb )->ILine() );

    if ( Pcsr( pfucb )->Cpage().Clines() == 0 )
    {
        Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
        pfucb->kdfCurr.Nullify();
        err = ErrERRCheck( wrnNDFoundGreater );
    }
    else
    {
        for ( ; ; )
        {
            Call( ErrNDSeek( pfucb, bmSearch ) );

            if ( !Pcsr( pfucb )->Cpage().FInvisibleSons( ) )
            {
                break;
            }
            else
            {
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

    Assert( Pcsr( pfucb )->Cpage().FLeafPage() );

    if ( JET_errSuccess == err )
    {
        if ( fUnique )
        {
            Assert( FKeysEqual( kdf.key, pfucb->kdfCurr.key ) );

            if( !FNDDeleted( pfucb->kdfCurr ) )
            {
                Error( ErrERRCheck( JET_errKeyDuplicate ) );
            }
#ifdef DEBUG
            else
            {
                Assert( !FNDPotVisibleToCursor( pfucb ) );
            }
#endif


            cbReq = kdf.data.Cb() > pfucb->kdfCurr.data.Cb() ?
                        kdf.data.Cb() - pfucb->kdfCurr.data.Cb() :
                        0;
        }
        else
        {
            Assert( 0 == CmpKeyData( bmSearch, pfucb->kdfCurr ) );

            if ( !FNDDeleted( pfucb->kdfCurr ) )
            {
                Error( ErrERRCheck( JET_errMultiValuedIndexViolation ) );
            }
#ifdef DEBUG
            else
            {
                Assert( !FNDPotVisibleToCursor( pfucb ) );
            }
#endif

            cbReq   = 0;
        }

        fInsert = fFalse;
    }
    else
    {
        if ( wrnNDFoundLess == err )
        {
            Assert( Pcsr( pfucb )->Cpage().Clines() - 1
                        == Pcsr( pfucb )->ILine() );
            Assert( Pcsr( pfucb )->Cpage().Clines() == 0 ||
                    !fUnique && CmpKeyData( pfucb->kdfCurr, bmSearch ) < 0 ||
                    fUnique && CmpKey( pfucb->kdfCurr.key, bmSearch.key ) < 0 );

            Pcsr( pfucb )->IncrementILine();
        }

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

    const BOOL fShouldAppend = FBTIAppend( pfucb, Pcsr( pfucb ), cbReq );
    BOOL fSplitRequired = FBTISplit( pfucb, Pcsr( pfucb ), cbReq );
    if ( fSplitRequired && !fInsert && fUnique )
    {
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
            BTPrereadSpaceTree( pfucb );
        }
#endif

        kdf.key     = key;
        kdf.data    = data;
        kdf.fFlags  = 0;

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
            Assert( !Pcsr( pfucb )->FLatched() );

            rceidReplace = rceidNull;
            prceReplace = prceNil;
            latch = latchRIW;
            goto Seek;
        }

        Call( err );
    }
    else
    {
        PGNO    pgno = Pcsr( pfucb )->Pgno();

        err = Pcsr( pfucb )->ErrUpgrade();

        if ( err == errBFLatchConflict )
        {
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
                Assert( JET_errSuccess == err ||
                        wrnNDFoundLess == err ||
                        wrnNDFoundGreater == err );

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


            if( prceNil != prceReplace )
            {
                Assert( prceNil != prceInsert );
                VERNullifyFailedDMLRCE( prceReplace );
                rceidReplace = rceidNull;
                prceReplace = prceNil;
            }


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
#endif

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

    kdf.key     = key;
    kdf.data    = data;
    kdf.fFlags  = 0;

#ifdef DEBUG
    Pcsr( pfucb )->SetILine( Pcsr( pfucb )->Cpage().Clines() - 1 );
    NDGet( pfucb );
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

    Pcsr( pfucb )->SetILine( Pcsr( pfucb )->Cpage().Clines() );

    BTIComputePrefix( pfucb, Pcsr( pfucb ), key, &kdf );
    Assert( !FNDCompressed( kdf ) || kdf.key.prefix.Cb() > 0 );

    cbReq = CbNDNodeSizeCompressed( kdf );

#ifdef DEBUG
    USHORT  cbUncFreeDBG;
    cbUncFreeDBG = Pcsr( pfucb )->Cpage().CbUncommittedFree();
#endif

    if ( FBTIAppend( pfucb, Pcsr( pfucb ), cbReq ) )
    {
        kdf.key     = key;
        kdf.data    = data;
        kdf.fFlags  = 0;

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
            Assert( !Pcsr( pfucb )->FLatched() );
            Call( ErrBTInsert( pfucb, key, data, dirflag ) );
            return err;
        }
        else if ( errBTOperNone == err && FFUCBRepair( pfucb ) )
        {
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
#endif

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
#endif

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
        delete ptask;
        return JET_errSuccess;
    }

    err = PinstFromIfmp( pfucb->ifmp )->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask );
    if( err < JET_errSuccess )
    {
        delete ptask;
    }

    return err;
}

ERR ErrBTFlagDelete( FUCB *pfucb, DIRFLAG dirflag, RCE *prcePrimary )
{
    const BOOL      fVersion        = !( dirflag & fDIRNoVersion ) && !g_rgfmp[pfucb->ifmp].FVersioningOff();
    Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() );
    Assert( !FFUCBSpace( pfucb ) || fDIRNoVersion & dirflag );

    ERR     err;
    LATCH   latch = latchReadNoTouch;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTDelete );

    if ( Pcsr( pfucb )->FLatched() )
        CallR( ErrBTISaveBookmark( pfucb ) );

#ifdef DEBUG

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
    }
    else if ( latch == Pcsr( pfucb )->Latch() )
    {
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
            Pcsr( pfucb )->DecrementILine();
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
#endif

    return err;

HandleError:

    Assert( err < 0 );
    if( prceNil != prce )
    {
        Assert( fVersion );
        VERNullifyFailedDMLRCE( prce );
    }

    CallS( ErrBTRelease( pfucb ) );
    return err;
}



ERR ErrBTCopyTree( FUCB * pfucbSrc, FUCB * pfucbDest, DIRFLAG dirflag )
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

        Assert( g_rgfmp[ pfucbSrc->ifmp ].CbPage() == g_rgfmp[ pfucbDest->ifmp ].CbPage() );
        if (    pfucbSrc->kdfCurr.key.Cb() < 0 || pfucbSrc->kdfCurr.key.Cb() > g_rgfmp[ pfucbSrc->ifmp ].CbPage() ||
                pfucbSrc->kdfCurr.data.Cb() < 0 || pfucbSrc->kdfCurr.data.Cb() > g_rgfmp[ pfucbSrc->ifmp ].CbPage() ||
                pfucbSrc->kdfCurr.key.Cb() + pfucbSrc->kdfCurr.data.Cb() > g_rgfmp[ pfucbSrc->ifmp ].CbPage() )
        {
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

    FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
    dib.dirflag = fDIRNull;
    dib.pos     = posFirst;
    err = ErrBTDown( pfucb, &dib, latchReadNoTouch );
    if ( err < 0 )
    {
        if ( err == JET_errRecordNotFound )
        {
            err = JET_errSuccess;
            goto Done;
        }
        goto HandleError;
    }

    Assert( Pcsr( pfucb )->FLatched() );

    cpage = 1;

    if ( FFUCBUnique( pfucb ) )
    {
        forever
        {
            cnode++;

            Call( pinst->ErrCheckForTermination() );

            pgnoT = Pcsr( pfucb )->Pgno();
            err = ErrBTNext( pfucb, fDIRNull );
            if ( err < JET_errSuccess )
            {
                ckey = cnode;
                goto Done;
            }

            if ( Pcsr( pfucb )->Pgno() != pgnoT )
            {
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

        ckey = 1;

        forever
        {
            Assert( pfucb->kdfCurr.key.Cb() <= cbKeyMostMost );
            key.suffix.SetCb( pfucb->kdfCurr.key.Cb() );
            pfucb->kdfCurr.key.CopyIntoBuffer( key.suffix.Pv() );

            cnode++;

            Call( pinst->ErrCheckForTermination() );

            pgnoT = Pcsr( pfucb )->Pgno();
            err = ErrBTNext( pfucb, fDIRNull );
            if ( err < JET_errSuccess )
            {
                goto Done;
            }

            if ( Pcsr( pfucb )->Pgno() != pgnoT )
            {
                cpage++;
            }

            if ( !FKeysEqual( pfucb->kdfCurr.key, key ) )
            {
                ckey++;
            }
        }
    }


Done:
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


bool FBTICpgFragmentedRange(
    __in_ecount(cpg) const PGNO * const rgpgno,
    const CPG cpg,
    __in const CPG cpgFragmentedRangeMin,
    __out INT * const pipgnoStart,
    __out INT * const pipgnoEnd )
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

            for( ipgno++; ipgno < cpg && rgpgno[ipgno-1] + 1 == rgpgno[ipgno]; ++ipgno )
            {
            }

            const CPG cpgContiguous = ipgno - ipgnoContiguousRunStart;
            INT ipgnoContiguousRunEnd = ipgno-1;

            Assert( cpgContiguous > 0 );
            Assert( ipgnoContiguousRunEnd >= ipgnoContiguousRunStart );
            Assert( (PGNO)(ipgnoContiguousRunEnd - ipgnoContiguousRunStart)
                    == rgpgno[ipgnoContiguousRunEnd] - rgpgno[ipgnoContiguousRunStart] );
            
            if( cpgContiguous >= cpgFragmentedRangeMin )
            {
                if( ipgnoFragmentedRangeStart != -1 )
                {
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
                if( ipgnoFragmentedRangeStart != -1 )
                {
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

JETUNITTEST( FBTICpgFragmentedRange, RunTooShort )
{
    PGNO rgpgno[] = { 10, 20, 30, 40, 50, 60, 70, 80 };
    INT ignored1;
    INT ignored2;
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &ignored1, &ignored2 ) );
}

JETUNITTEST( FBTICpgFragmentedRange, ContiguousRun )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    INT ignored1;
    INT ignored2;
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &ignored1, &ignored2 ) );
}

JETUNITTEST( FBTICpgFragmentedRange, ContiguousRuns )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 101, 102, 103, 104, 105, 106, 107, 108, };
    INT ignored1;
    INT ignored2;
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &ignored1, &ignored2 ) );
}

JETUNITTEST( FBTICpgFragmentedRange, MinRangeSizeIsRespected )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 101, 102, 103, 104, 105, 106, 107, 108, };
    INT ignored1;
    INT ignored2;
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 10, &ignored1, &ignored2 ) );
    CHECK( true == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 11, &ignored1, &ignored2 ) );
}

JETUNITTEST( FBTICpgFragmentedRange, SmallerRangeSizeTriggersLessOften )
{
    PGNO rgpgno[] = { 1, 2, 3, 4,  6, 7, 8, 9, 10, };
    INT ignored1;
    INT ignored2;
    CHECK( false == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 4, &ignored1, &ignored2 ) );
    CHECK( true == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &ignored1, &ignored2 ) );
}

JETUNITTEST( FBTICpgFragmentedRange, FragmentedRuns )
{
    PGNO rgpgno[] = { 1, 2, 3,  5, 6, 7, 8, 9, 20, 21,  23, 24, 25, 26, 27, 28 };
    INT start;
    INT end;
    CHECK( true == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &start, &end ) );
    CHECK( 0 == start );
    CHECK( _countof(rgpgno)-1 == end );
}

JETUNITTEST( FBTICpgFragmentedRange, FragmentedThenUnfragmented )
{
    PGNO rgpgno[] = { 1, 2, 3,  5, 6, 7, 8, 9, 10, 20, 21, 22, 23, 24, 25, 26, 27, 28 };
    INT start;
    INT end;
    CHECK( true == FBTICpgFragmentedRange( rgpgno, _countof(rgpgno), 8, &start, &end ) );
    CHECK( 0 == start );
    CHECK( 8 == end );
}

JETUNITTEST( FBTICpgFragmentedRange, UnfragmentedThenFragmented )
{
    PGNO rgpgno[] = { 20, 21, 22, 23, 24, 25, 26, 27, 28, 1, 2, 3,  5, 6, 7, 8, 9, 10,  };
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
        1, 2, 3,  5, 6, 7, 8, 9, 10,
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
        1, 2, 3,  5, 6, 7, 8, 9, 10,
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
}

#endif

bool FBTICpgFragmentedRangeTrigger(
    __in_ecount(cpg) const PGNO * const rgpgno,
    const CPG cpg,
    __in const CPG cpgFragmentedRangeTrigger,
    __in const CPG cpgFragmentedRangeMin )
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
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, NotEnoughPagesToDefragUnfragmentedThenFragmented )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9,
                        100, 110, 120, 130, 140, };
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, NotEnoughPagesToDefragFragmentedThenUnfragmented )
{
    PGNO rgpgno[] = { 10, 20, 30, 40, 50,
                        60, 61, 62, 63, 64, 65, 66, 67, 68, };
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragButDoesNotMeetTrigger )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5,  7, 8, 9, 10, 11, };
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragButContiguous )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9,
                        
                        11, 12, 13, 14, 15, 16, 17, 18, 19, };
    CHECK( false == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragAllFragmented )
{
    PGNO rgpgno[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, };
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragUnfragmentedThenFragmented )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9,
                        100, 110, 120, 130, 140, 150, 160, 170, 180, };
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragFragmentedThenUnfragmented )
{
    PGNO rgpgno[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90,
                        100, 101, 102, 103, 104, 105, 106, 107, 108, 109, };
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragUnfragmentedThenFragmentedThenUnfragmented )
{
    PGNO rgpgno[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9,
                        100, 110, 120, 130, 140, 150, 160, 170, 180,
                        191, 192, 193, 194, 195, 196, 197, 198, 199, };
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
}

JETUNITTEST( FBTICpgFragmentedRangeTrigger, EnoughPagesToDefragFragmentedThenUnfragmentedThenFragmented )
{
    PGNO rgpgno[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90,
                        100, 101, 102, 103, 104, 105, 106, 107, 108,
                        200, 210, 220, 230, 240, 250, 260, 270, 280 };
    CHECK( true == FBTICpgFragmentedRangeTrigger( rgpgno, _countof(rgpgno), 4, 8 ) );
}

#endif

ERR ErrBTIGetBookmarkFromPage(
    __in FUCB * const pfucb,
    __in CSR * const pcsr,
    const INT iline,
    __out BOOKMARK * const pbm )
{
    Assert( pfucb );
    Assert( pcsr );
    Assert( pbm );
    Assert( pcsr->FLatched() );
    Assert( pbm->key.FNull() );
    Assert( pcsr->Cpage().FInvisibleSons() );

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

ERR ErrBTIFindFragmentedRangeInParentOfLeafPage(
    __in FUCB * const pfucb,
    __in CSR * const pcsr,
    __in const CPG cpgFragmentedRangeMin,
    __out BOOKMARK * const pbmRangeStart,
    __out BOOKMARK * const pbmRangeEnd)
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

    Assert( err <= JET_errSuccess );
    if( JET_errSuccess == err )
    {
        Assert( !pbmRangeStart->key.FNull() );
    }
    
    return err;
}

ERR ErrBTIFindFragmentedRange(
    __in FUCB * const pfucb,
    __in CSR * const pcsr,
    __in const BOOKMARK& bmStart,
    __out BOOKMARK * const pbmRangeStart,
    __out BOOKMARK * const pbmRangeEnd)
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
    
    Call( ErrNDSeek( pfucb, pcsr, bmStart ) );

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
        
        for( INT iline = ilineStart; iline < clines; ++iline )
        {
            pcsr->SetILine( iline );
            NDGet( pfucb, pcsr );
            const PGNO pgnoChild = *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv();
            
            Call( csrSubtree.ErrGetReadPage(pfucb->ppib,
                                            pfucb->ifmp,
                                            pgnoChild,
                                            bflfDefault,
                                            PBFLatchHintPgnoRoot( pfucb ) ) );

            err = ErrBTIFindFragmentedRange( pfucb, &csrSubtree, bmStart, pbmRangeStart, pbmRangeEnd );
            csrSubtree.ReleasePage();

            if( JET_errRecordNotFound == err )
            {
                err = JET_errSuccess;
            }
            else if( JET_errSuccess == err )
            {
                fFoundRange = true;
                break;
            }
            Call( err );
        }

        if( !fFoundRange )
        {
            Call( ErrERRCheck( JET_errRecordNotFound ) );
        }
    }

    
HandleError:
    Assert( !csrSubtree.FLatched() );
    Assert( err <= JET_errSuccess );
    if( JET_errSuccess == err )
    {
        Assert( !pbmRangeStart->key.FNull() );
    }
    
    return err;
}

ERR ErrBTFindFragmentedRange(
    __in FUCB * const pfucb,
    __in const BOOKMARK& bmStart,
    __out BOOKMARK * const pbmRangeStart,
    __out BOOKMARK * const pbmRangeEnd)
{
    Assert( pfucb );
    Assert( pbmRangeStart );
    Assert( pbmRangeEnd );
    Assert( pbmRangeStart->key.FNull() );
    Assert( pbmRangeEnd->key.FNull() );

    ERR err;
    CSR csrRoot;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsNone );

    Call( csrRoot.ErrGetReadPage( pfucb->ppib,
                                  pfucb->ifmp,
                                  PgnoRoot( pfucb ),
                                  bflfDefault,
                                  PBFLatchHintPgnoRoot( pfucb ) ) );
    
    if( csrRoot.Cpage().FLeafPage() )
    {
        Call( ErrERRCheck( JET_errRecordNotFound ) );
    }

    Call( ErrBTIFindFragmentedRange( pfucb, &csrRoot, bmStart, pbmRangeStart, pbmRangeEnd ) );
    
HandleError:
    csrRoot.ReleasePage();

    Assert( err <= JET_errSuccess );
    if( JET_errSuccess == err )
    {
        Assert( !pbmRangeStart->key.FNull() );
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

            Call( err );

            NDGetPtrExternalHeader( pcsr->Cpage(), &line, noderfWhole );
            cbPrefix = CPAGE::cbInsertionOverhead + line.cb;
            cbUsed += cbPrefix;

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

            Assert( ULONG( pcsr->ILine() + 1 ) == cNodes );
        }

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


    CallR( FCB::ErrCreate( ppib, ifmp, pgnoFDP, &pfcb ) );


    Assert( pfcb->IsLocked() );
    Assert( pfcb->FTypeNull() );
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
                Assert( pfcb->FUnique() );
                Assert( openNormalUnique == opentype );
            }
            Assert( !pfcb->FSpaceInitialized() );
        }
    }

    if ( pgnoFDP == pgnoSystemRoot )
    {

        Assert( objidNil == objidFDP );
        if ( openNew == opentype )
        {
            Assert( objidNil == pfcb->ObjidFDP() );
        }
        else
        {
            Assert( objidSystemRoot == pfcb->ObjidFDP() );
        }


        pfcb->InsertList();


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
            pfcb->Unlink( pfucb, fTrue );
        }
        
        FUCBClose( pfucb );
    }
    
    pfcb->PrepareForPurge( fFalse );
    pfcb->Purge( fFalse );

    return err;
}



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


    pfcb = FCB::PfcbFCBGet( ifmp, pgnoFDP, &fState, fTrue, !fWillInitFCB );
    if ( pfcb == pfcbNil )
    {


        Assert( fFCBStateNull == fState );


        err = ErrBTICreateFCB( ppib, ifmp, pgnoFDP, objidFDP, opentype, ppfucb );
        Assert( err <= JET_errSuccess );

        if ( err == errFCBExists )
        {



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

            Assert( pfcb->WRefCount() > 1 || (1 == pfcb->WRefCount() && err < JET_errSuccess) );

            pfcb->Release();
        }
        else
        {
            FireWall( "DeprecatedSentinelFcbBtOpen" );
            Assert( !FFMPIsTempDB( ifmp ) );

            err = ErrERRCheck( JET_errTableLocked );
        }
    }

HandleError:
    return err;
}



ERR ErrBTIGotoRoot( FUCB *pfucb, LATCH latch )
{
    ERR     err;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTOpen );

    Assert( !Pcsr( pfucb )->FLatched() );

    CallR( Pcsr( pfucb )->ErrGetPage( pfucb->ppib,
                                      pfucb->ifmp,
                                      PgnoRoot( pfucb ),
                                      latch,
                                      PBFLatchHintPgnoRoot( pfucb )
                                      ) );
    Pcsr( pfucb )->SetILine( 0 );

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
        err = Pcsr( pfucb )->ErrGetPage( pfucb->ppib,
                                         pfucb->ifmp,
                                         Pcsr( pfucb )->Pgno(),
                                         latch,
                                         NULL,
                                         fTrue );

        if ( ( err == JET_errPageNotInitialized ) ||
                ( err == JET_errFileIOBeyondEOF ) ||
                ( ( err >= JET_errSuccess ) && ( Pcsr( pfucb )->Dbtime() == dbtimeShrunk ) ) )
        {
            if ( !g_rgfmp[ pfucb->ifmp ].FTrimSupported() && !g_rgfmp[ pfucb->ifmp ].FShrinkDatabaseEofOnAttach() )
            {
                FireWall( OSFormat( "BtRefreshUnexpectedErr:%d", err ) );
            }

            err = JET_errSuccess;
            BTUp( pfucb );
            goto LookupBookmark;
        }
        Call( err );

        if ( Pcsr( pfucb )->Dbtime() == dbtimeLast )
        {
            Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
            Assert( Pcsr( pfucb )->Cpage().ObjidFDP() == ObjidFDP( pfucb ) );
            NDGet( pfucb );

            AssertNDGet( pfucb );
            return JET_errSuccess;
        }

        Assert( Pcsr( pfucb )->Dbtime() > dbtimeLast );

        err = ErrBTISeekInPage( pfucb, pfucb->bmCurr );

        if ( JET_errSuccess == err )
        {
            goto HandleError;
        }

        BTUp( pfucb );
        Call( err );

        if ( wrnNDFoundGreater == err || wrnNDFoundLess == err )
        {
            Error( ErrERRCheck( JET_errRecordDeleted ) );
        }

        Assert( wrnNDNotFoundInPage == err );
    }

LookupBookmark:

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
            Error( ErrERRCheck( JET_errTaskDropped ) );
        }

        Call( PinstFromIfmp( pfucb->ifmp )->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask ) );
        
        OnDebug( fTaskPosted = fTrue; );
        
        ptask = NULL;
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        Assert ( !fTaskPosted );
        delete ptask;
    }
    return err;
}
    
INLINE ERR ErrBTIDelete( FUCB *pfucb, const BOOKMARK& bm )
{
    ERR     err;

    ASSERT_VALID( &bm );
    Assert( !Pcsr( pfucb )->FLatched( ) );
    Assert( !FFUCBSpace( pfucb ) );
    Assert( !FFMPIsTempDB( pfucb->ifmp ) );

    auto tc = TcCurr();
    PERFOpt( PERFIncCounterTable( cBTDelete, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );

    CallR( ErrBTISinglePageCleanup( pfucb, bm ) );
    Assert( !Pcsr( pfucb )->FLatched() );

    if ( wrnBTMultipageOLC == err )
    {

        MERGETYPE mergetype;
        err = ErrBTIMultipageCleanup( pfucb, bm, NULL, NULL, &mergetype, fTrue );
        if ( errBTMergeNotSynchronous == err )
        {
            err = JET_errSuccess;
        }
        else if ( mergetypeNone == mergetype )
        {
            err = ErrBTIMultipageCleanup( pfucb, bm, NULL, NULL, &mergetype, fFalse );
            if ( errBTMergeNotSynchronous == err )
            {
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

    Assert( !PinstFromPpib( ppib )->FRecovering() || fRecoveringUndo == PinstFromPpib( ppib )->m_plog->FRecoveringMode() );

    if ( ppib->Level() == 0 )
    {
        CallR( ErrDIRBeginTransaction( ppib, 52965, bitTransactionWritableDuringRecovery ) );
        fInTransaction = fTrue;
    }

    err = ErrBTIDelete( pfucb, bm );

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


INLINE INT CbBTIFreeDensity( const FUCB *pfucb )
{
    Assert( pfucb->u.pfcb != pfcbNil );
    return ( (INT) pfucb->u.pfcb->CbDensityFree() );
}


LOCAL ULONG CbBTICbReq( FUCB *pfucb, CSR *pcsr, const KEYDATAFLAGS& kdf )
{
    Assert( pcsr->Latch() == latchRIW );
    Assert( !pcsr->Cpage().FLeafPage() );
    Assert( sizeof( PGNO )== kdf.data.Cb() );

    KEYDATAFLAGS    kdfT = kdf;

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


INT CbNDCommonPrefix( FUCB *pfucb, CSR *pcsr, const KEY& key )
{
    Assert( pcsr->FLatched() );

    NDGetPrefix( pfucb, pcsr );
    Assert( pfucb->kdfCurr.key.suffix.Cb() == 0 );

    const ULONG cbCommon = CbCommonKey( key, pfucb->kdfCurr.key );

    return cbCommon;
}


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


INLINE BOOL FBTIAppend( const FUCB *pfucb, CSR *pcsr, ULONG cbReq, const BOOL fUpdateUncFree )
{
    Assert( cbReq <= CbNDPageAvailMostNoInsert( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
    Assert( pcsr->FLatched() );

    if ( pcsr->Cpage().FLeafPage()
        && !pcsr->Cpage().FSpaceTree() )
    {
        cbReq += CbBTIFreeDensity( pfucb );
    }

    return ( pgnoNull == pcsr->Cpage().PgnoNext()
            && pcsr->ILine() == pcsr->Cpage().Clines()
            && !FNDFreePageSpace( pfucb, pcsr, cbReq, fUpdateUncFree ) );
}


INLINE BOOL FBTISplit( const FUCB *pfucb, CSR *pcsr, const ULONG cbReq, const BOOL fUpdateUncFree )
{
    return !FNDFreePageSpace( pfucb, pcsr, cbReq, fUpdateUncFree );
}


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
#endif

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

    Call( ErrBTISelectSplit( pfucb, psplitPath, pkdf, dirflag ) );
    BTISplitCheckPath( psplitPath );
    if ( NULL == psplitPath->psplit ||
         splitoperNone == psplitPath->psplit->splitoper )
    {
        fOperNone = fTrue;
    }

    Call( ErrBTIGetNewPages( pfucb, psplitPath, dirflag ) );

    BTISplitReleaseUnneededPages( pinst, &psplitPath );
    Assert( psplitPath->psplit != NULL );

    Call( ErrBTISplitUpgradeLatches( pfucb->ifmp, psplitPath ) );


    psplitPath->csr.SetILine( psplitPath->psplit->ilineOper );

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

        if ( err < JET_errSuccess )
        {
            BTISplitRevertDbtime( psplitPath );
            goto HandleError;
        }

        BTISplitSetLgpos( psplitPath, lgpos );
    }

    if ( prceNil != prceReplace && !fOperNone )
    {
        const INT cbData    = pkdf->data.Cb();

        VERSetCbAdjust( Pcsr( pfucb ), prceReplace, cbData, cbDataOld, fDoNotUpdatePage );
    }

    BTIPerformSplit( pfucb, psplitPath, pkdf, dirflag );

    BTICheckSplits( pfucb, psplitPath, pkdf, dirflag );

    BTISplitShrinkPages( psplitPath );

    BTISplitSetCursor( pfucb, psplitPath );

    BTIReleaseSplitPaths( pinst, psplitPath );

    if ( fOperNone )
    {
        Assert( !Pcsr( pfucb )->FLatched() );
        err = ErrERRCheck( errBTOperNone );

        if( prceNil != prceReplace )
        {
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
    if ( psplitPath != NULL )
    {
        BTIReleaseSplitPaths( pinst, psplitPath );
    }

    return err;
}


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

    NDGetBookmarkFromKDF( pfucb, *pkdf, &bm );

    if ( *pdirflag & fDIRInsert )
    {
        Assert( bm.key.Cb() == pkdf->key.Cb() );
        Assert( bm.key.suffix.Pv() == pkdf->key.suffix.Pv() );
        Assert( bm.key.prefix.FNull() );
    }
    else if ( *pdirflag & fDIRFlagInsertAndReplaceData )
    {
        Call( ErrBTISaveBookmark( pfucb, bm, fFalse ) );
    }
    else
    {
        Assert( *pdirflag & fDIRReplace );
        bm.key = pfucb->bmCurr.key;
    }

    Call( ErrBTICreateSplitPath( pfucb, bm, ppsplitPath ) );
    Assert( (*ppsplitPath)->csr.Cpage().FLeafPage() );

    if ( wrnNDFoundLess == err )
    {
        Assert( (*ppsplitPath)->csr.Cpage().Clines() - 1 ==  (*ppsplitPath)->csr.ILine() );
        (*ppsplitPath)->csr.SetILine( (*ppsplitPath)->csr.Cpage().Clines() );
    }

    if ( (*ppsplitPath)->csr.Pgno() != pgnoSplit ||
         (*ppsplitPath)->csr.Dbtime() != dbtimeLast )
    {
        CSR     *pcsr = &(*ppsplitPath)->csr;

        if ( fDIRReplace & *pdirflag )
        {
            AssertNDGet( pfucb, pcsr );
            const INT  cbReq = pfucb->kdfCurr.data.Cb() >= pkdf->data.Cb() ?
                                  0 :
                                  pkdf->data.Cb() - pfucb->kdfCurr.data.Cb();

            if ( cbReq > 0 && FBTISplit( pfucb, pcsr, cbReq ) )
            {
                Error( ErrERRCheck( errPMOutOfPageSpace ) );
            }

            pcsr->UpgradeFromRIWLatch();
            Assert( latchWrite == pcsr->Latch() );

            err = ErrNDReplace( pfucb, pcsr, &pkdf->data, *pdirflag, rceid1, prceReplace );
            Assert( errPMOutOfPageSpace != err );
            Call( err );
        }
        else
        {
            ULONG   cbReq;

            Assert( ( fDIRInsert & *pdirflag ) ||
                    ( fDIRFlagInsertAndReplaceData & *pdirflag ) );

            if ( JET_errSuccess == err )
            {
#ifdef DEBUG
                FUCBResetUpdatable( pfucb );
                Assert( FNDDeleted( pfucb->kdfCurr ) );
                Assert( !FNDPotVisibleToCursor( pfucb, pcsr ) );
                FUCBSetUpdatable( pfucb );
#endif

                if ( FFUCBUnique( pfucb ) )
                {
                    Assert( *pdirflag & fDIRFlagInsertAndReplaceData );

                    cbReq = pkdf->data.Cb() > pfucb->kdfCurr.data.Cb() ?
                                pkdf->data.Cb() - pfucb->kdfCurr.data.Cb() :
                                0;

                    if ( FBTISplit( pfucb, pcsr, cbReq ) )
                    {
                        Error( ErrERRCheck( errPMOutOfPageSpace ) );
                    }

                    pcsr->UpgradeFromRIWLatch();
                    Assert( latchWrite == pcsr->Latch() );

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
                    Assert( fFalse );
                    Assert( *pdirflag & fDIRInsert );
                    Assert( 0 == CmpKeyData( bm, pfucb->kdfCurr ) );

                    if ( !FNDDeleted( pfucb->kdfCurr )
                        || FNDPotVisibleToCursor( pfucb, pcsr ) )
                    {
                        Error( ErrERRCheck( JET_errMultiValuedIndexViolation ) );
                    }

                    pcsr->UpgradeFromRIWLatch();
                    Assert( latchWrite == pcsr->Latch() );

                    Assert( fFalse );
                    Call( ErrNDFlagInsert( pfucb, pcsr, *pdirflag, rceid1, pverproxy ) );
                }
            }
            else
            {
                KEYDATAFLAGS kdfCompressed = *pkdf;

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


LOCAL ERR   ErrBTICreateSplitPath( FUCB             *pfucb,
                                   const BOOKMARK&  bm,
                                   SPLITPATH        **ppsplitPath )
{
    ERR     err;
    BOOL    fLeftEdgeOfBtree    = fTrue;
    BOOL    fRightEdgeOfBtree   = fTrue;

    Assert( NULL == *ppsplitPath );
    CallR( ErrBTINewSplitPath( ppsplitPath ) );
    Assert( NULL != *ppsplitPath );

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

        Call( ErrBTINewSplitPath( ppsplitPath ) );

        Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
        Call( (*ppsplitPath)->csr.ErrGetRIWPage(
                                pfucb->ppib,
                                pfucb->ifmp,
                                *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv() ) );
    }

HandleError:
    return err;
}


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


LOCAL ERR ErrBTISelectSplit( FUCB           *pfucb,
                             SPLITPATH      *psplitPath,
                             KEYDATAFLAGS   *pkdf,
                             DIRFLAG        dirflag )
{
    ERR     err;


    CallR( ErrBTINewSplit( pfucb, psplitPath, pkdf, dirflag ) );
    Assert( psplitPath->psplit != NULL );
    Assert( psplitPath->psplit->psplitPath == psplitPath );

    SPLIT   *psplit = psplitPath->psplit;
    BTIRecalcWeightsLE( psplit );

    if ( psplitPath->csr.Cpage().FRootPage() )
    {
        BTISelectVerticalSplit( psplit, pfucb );

        BTISplitCalcUncFree( psplit );

        Call( ErrBTISplitAllocAndCopyPrefixes( pfucb, psplit ) );
        return JET_errSuccess;
    }

    ULONG   cbReq;
    CSR     *pcsrParent;

    if  ( psplit->fAppend )
    {
        BTISelectAppend( psplit, pfucb );
    }
    else
    {
        BTISelectRightSplit( psplit, pfucb );
        Assert( psplit->ilineSplit >= 0 );
    }

    BTISplitCalcUncFree( psplit );

    psplit->fNewPageFlags   =
    psplit->fSplitPageFlags = psplitPath->csr.Cpage().FFlags();

    Call( ErrBTISplitAllocAndCopyPrefixes( pfucb, psplit ) );

    Call( ErrBTISplitComputeSeparatorKey( psplit, pfucb ) );
    Assert( sizeof( PGNO ) == psplit->kdfParent.data.Cb() );

    Call( ErrBTISeekSeparatorKey( psplit, pfucb ) );

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

    if ( !( psplit = new SPLIT ) )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    psplit->pgnoSplit = psplitPath->csr.Pgno();

    if ( psplitPath->csr.Cpage().FLeafPage( ) &&
         pgnoNull != psplitPath->csr.Cpage().PgnoNext() )
    {
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

    if ( psplitPath->csr.Cpage().FInvisibleSons( ) )
    {
        psplit->splitoper = splitoperInsert;
    }
    else if ( dirflag & fDIRInsert )
    {
        Assert( !( dirflag & fDIRReplace ) );
        Assert( !( dirflag & fDIRFlagInsertAndReplaceData ) );
        psplit->splitoper = splitoperInsert;

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

    psplit->clines = psplitPath->csr.Cpage().Clines();

    if ( splitoperInsert == psplit->splitoper )
    {
        psplit->clines++;
    }

    Alloc( psplit->rglineinfo = new LINEINFO[psplit->clines + 1] );

    Assert( splitoperInsert == psplit->splitoper &&
            psplitPath->csr.Cpage().Clines() >= psplitPath->csr.ILine() ||
            psplitPath->csr.Cpage().Clines() > psplitPath->csr.ILine() );

    psplit->ilineOper = psplitPath->csr.ILine();

    for ( iLineFrom = 0, iLineTo = 0; iLineTo < psplit->clines; iLineTo++ )
    {
        if ( psplit->ilineOper == iLineTo &&
             splitoperInsert == psplit->splitoper )
        {
            psplit->rglineinfo[iLineTo].kdf = *pkdf;
            psplit->rglineinfo[iLineTo].cbSizeMax =
            psplit->rglineinfo[iLineTo].cbSize =
                    CbNDNodeSizeTotal( *pkdf );

            if ( fPossibleHotpoint )
            {
                Assert( iLineTo >= 2 );

                if ( psplit->rglineinfo[iLineTo-2].kdf.key.suffix.Pv() > pvHighest
                    && psplit->rglineinfo[iLineTo-1].kdf.key.suffix.Pv() > psplit->rglineinfo[iLineTo-2].kdf.key.suffix.Pv() )
                {
                    pvHighest = psplit->rglineinfo[iLineTo-2].kdf.key.suffix.Pv();
                }
                else
                {
                    fPossibleHotpoint = fFalse;
                }
            }

            continue;
        }

        psplitPath->csr.SetILine( iLineFrom );

        NDGet( pfucb, &psplitPath->csr );

        if ( iLineTo == psplit->ilineOper )
        {
            Assert( splitoperInsert != psplit->splitoper );
            Assert( splitoperNone != psplit->splitoper );

            Assert( !fPossibleHotpoint );

            psplit->rglineinfo[iLineTo].kdf.key     = pfucb->kdfCurr.key;
            psplit->rglineinfo[iLineTo].kdf.data    = pkdf->data;
            psplit->rglineinfo[iLineTo].kdf.fFlags  = pfucb->kdfCurr.fFlags;

            ULONG   cbMax       = CbBTIMaxSizeOfNode( pfucb, &psplitPath->csr );
            ULONG   cbSize      = CbNDNodeSizeTotal( psplit->rglineinfo[iLineTo].kdf );

            psplit->rglineinfo[iLineTo].cbSizeMax = max( cbSize, cbMax );

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
                    pvHighest = max( pvHighest, pfucb->kdfCurr.key.suffix.Pv() );
                }
                else if ( iLineTo > psplit->ilineOper )
                {
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

VOID BTISelectVerticalSplit( SPLIT *psplit, FUCB *pfucb )
{
    Assert( psplit->psplitPath->csr.Pgno() == PgnoRoot( pfucb ) );

    psplit->splittype   = splittypeVertical;
    psplit->ilineSplit  = 0;

    BTISelectPrefixes( psplit, psplit->ilineSplit );

    if ( !FBTISplitCausesNoOverflow( psplit, psplit->ilineSplit ) )
    {
        BTISelectSplitWithOperNone( psplit, pfucb );
        BTISelectPrefixes( psplit, psplit->ilineSplit );
        Assert( FBTISplitCausesNoOverflow( psplit, psplit->ilineSplit ) );
    }
    else
    {
        Assert( psplit->splitoper != splitoperNone );
    }

    BTISplitSetPrefixes( psplit );
    Assert( FBTISplitCausesNoOverflow( psplit, psplit->ilineSplit ) );

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


VOID BTISelectAppend( SPLIT *psplit, FUCB *pfucb )
{
    Assert( psplit->clines - 1 == psplit->ilineOper );
    Assert( splitoperInsert == psplit->splitoper );
    Assert( psplit->psplitPath->csr.Cpage().FLeafPage() );

    psplit->splittype           = splittypeAppend;
    psplit->ilineSplit          = psplit->ilineOper;

    LINEINFO    *plineinfoOper  = &psplit->rglineinfo[psplit->ilineOper];
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


LOCAL BOOL FBTISplitAppendLeaf( IN const SPLIT * const psplit )
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


LOCAL BOOL FBTIPrependSplit( IN const SPLIT * const psplit )
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


LOCAL VOID BTISelectHotPointSplit( IN SPLIT * const psplit, IN FUCB * pfucb )
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
    
        BTISelectSplitWithOperNone( psplit, pfucb );
        psplit->fHotpoint = fFalse;
    }
    else
    {
        Assert( ilineCandidate == psplit->clines - 1 );
    }
    
    psplit->ilineSplit = ilineCandidate;
    
    BTISelectPrefix(
            psplit->rglineinfo,
            ilineCandidate,
            &psplit->prefixinfoSplit );
    
    BTISelectPrefix(
            &psplit->rglineinfo[ilineCandidate],
            psplit->clines - ilineCandidate,
            &psplit->prefixinfoNew );
    
    Assert( FBTISplitCausesNoOverflow( psplit, ilineCandidate ) );
    BTISplitSetPrefixes( psplit );
}


LOCAL VOID BTISelectPrependLeafSplit(
    IN SPLIT * const psplit,
    OUT INT * const pilineCandidate,
    OUT PREFIXINFO * const pprefixinfoSplitCandidate,
    OUT PREFIXINFO * const pprefixinfoNewCandidate )
{
    Assert( psplit->prefixinfoSplit.FNull() );
    Assert( psplit->prefixinfoNew.FNull() );
    Assert( FBTIPrependSplit( psplit ) );

    const INT iline = 1;
    
    
    BTISelectPrefix(
            psplit->rglineinfo,
            iline,
            &psplit->prefixinfoSplit );
    
    
    BTISelectPrefix(
            &psplit->rglineinfo[iline],
            psplit->clines - iline,
            &psplit->prefixinfoNew );

    Assert( 0 == psplit->psplitPath->psplit->ilineOper );
    Assert( FBTISplitCausesNoOverflow( psplit, iline ) );

    *pilineCandidate            = iline;
    *pprefixinfoNewCandidate    = psplit->prefixinfoNew;
    *pprefixinfoSplitCandidate  = psplit->prefixinfoSplit;
}


LOCAL VOID BTISelectAppendLeafSplit(
    IN SPLIT * const psplit,
    OUT INT * const pilineCandidate,
    OUT PREFIXINFO * const pprefixinfoSplitCandidate,
    OUT PREFIXINFO * const pprefixinfoNewCandidate )
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
            DBEnforce( psplit->psplitPath->csr.Cpage().Ifmp(), psplit->clines > 2 );
        }
    }

    Assert( psplit->clines - 1 == *pilineCandidate
        || psplit->clines - 2 == *pilineCandidate );
}


LOCAL VOID BTIRecalculatePrefix(
    IN SPLIT * const psplit,
    IN const INT ilinePrev,
    IN const INT ilineNew )
{
    Assert( absdiff( ilinePrev, ilineNew ) == 1 );

    if( ilinePrev < ilineNew )
    {

        
        if ( psplit->rglineinfo[ilineNew-1].cbCommonPrev == 0 )
        {
        }
        else
        {
            BTISelectPrefix(
                    psplit->rglineinfo,
                    ilineNew,
                    &psplit->prefixinfoSplit );
        }
        
        
        if ( 0 == psplit->prefixinfoNew.ilineSegBegin )
        {
            
            BTISelectPrefix( &psplit->rglineinfo[ilineNew], psplit->clines - ilineNew, &psplit->prefixinfoNew );
        }
        else if ( !psplit->prefixinfoNew.FNull() )
        {
            
            psplit->prefixinfoNew.ilinePrefix--;
            psplit->prefixinfoNew.ilineSegBegin--;
            psplit->prefixinfoNew.ilineSegEnd--;
        }
        
    }
    else
    {
        Assert( ilinePrev > ilineNew );
        

        
        if ( psplit->prefixinfoSplit.ilineSegEnd == ilineNew )
        {
        
            BTISelectPrefix( psplit->rglineinfo, ilineNew, &psplit->prefixinfoSplit );
        }
        
        
        if ( psplit->rglineinfo[ilineNew+1].cbCommonPrev == 0 )
        {

            
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


LOCAL VOID BTISelectEvenSplitWithoutCheckingOverflows(
    IN const SPLIT * const psplit,
    OUT INT * const pilineCandidate )
{
    *pilineCandidate = 0;
    
    ULONG       cbSizeCandidateLE   = 0;


    const ULONG     cbSizeTotal     = psplit->rglineinfo[psplit->clines - 1].cbSizeLE;
    const ULONG     cbSizeBestSplit = cbSizeTotal / 2;

    
    INT iline               = psplit->clines / 2;
    Assert( iline >= 1 );

    
    const ULONG cbSizeFirstSplitPage        = psplit->rglineinfo[iline - 1].cbSizeLE;
    const ULONG cbSizeFirstNewPage          = cbSizeTotal - cbSizeFirstSplitPage;
    const BOOL  fFirstSplitPageWasSmaller   = cbSizeFirstSplitPage < cbSizeFirstNewPage;

    
    while( true )
    {
        
        const ULONG cbSizeSplitPage     = psplit->rglineinfo[iline - 1].cbSizeLE;
        const ULONG cbSizeNewPage       = cbSizeTotal - cbSizeSplitPage;
        const BOOL  fSplitPageIsSmaller = cbSizeSplitPage < cbSizeNewPage;
        
        
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

            *pilineCandidate                = iline;
            cbSizeCandidateLE               = psplit->rglineinfo[iline - 1].cbSizeLE;


            Assert( fFirstSplitPageWasSmaller != fSplitPageIsSmaller );
        }
        else
        {
            
            Assert( fFirstSplitPageWasSmaller != fSplitPageIsSmaller );
        }

        if( fFirstSplitPageWasSmaller != fSplitPageIsSmaller )
        {
            break;
        }
        else if( fSplitPageIsSmaller )
        {
            
            ++iline;

            
            if( psplit->clines == iline )
            {
                
                Assert( 0 != *pilineCandidate );
                Assert( psplit->clines - 1 == *pilineCandidate );
                break;
            }
        }
        else
        {
            Assert( !fSplitPageIsSmaller );


            --iline;


            if( 0 == iline )
            {
                
                Assert( 1 == *pilineCandidate );
                break;
            }
        }
    }
    
    Assert( 0 != *pilineCandidate );
}

#ifdef DEBUG
LOCAL VOID BTISelectEvenSplitDebug(
    IN SPLIT * const psplit,
    OUT INT * const pilineCandidate,
    OUT PREFIXINFO * const pprefixinfoSplitCandidate,
    OUT PREFIXINFO * const pprefixinfoNewCandidate )
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
    
        for ( iline = psplit->clines - 1; iline > 0; iline-- )
        {
            BTISelectPrefixes( psplit, iline );
    
            if ( FBTISplitCausesNoOverflow( psplit, iline ) )
            {
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
                DBEnforce( psplit->psplitPath->csr.Cpage().Ifmp(), psplit->clines > 2 );
            }
        }
        
        psplit->prefixinfoSplit =   prefixinfoSplitSaved;
        psplit->prefixinfoNew   =   prefixinfoNewSaved;
    
        return;
}
#endif

LOCAL VOID BTISelectEvenSplit(
    IN SPLIT * const psplit,
    OUT INT * const pilineCandidate,
    OUT PREFIXINFO * const pprefixinfoSplitCandidate,
    OUT PREFIXINFO * const pprefixinfoNewCandidate )
{
    Assert( psplit->prefixinfoSplit.FNull() );

    *pilineCandidate = 0;

    INT iline;
    
    
    BTISelectEvenSplitWithoutCheckingOverflows( psplit, &iline );

    
    BTISelectPrefix(
            psplit->rglineinfo,
            iline,
            &psplit->prefixinfoSplit );
    
    
    BTISelectPrefix(
            &psplit->rglineinfo[iline],
            psplit->clines - iline,
            &psplit->prefixinfoNew );

    

    enum { SPLIT_PAGE_SMALLER, SPLIT_PAGE_LARGER, UNKNOWN } splitpointChange, lastsplitpointChange;

    lastsplitpointChange = UNKNOWN;
    
    while( true )
    {
        Assert( iline > 0 );
        Assert( iline < psplit->clines );
        Assert( 0 == *pilineCandidate );
        

        splitpointChange = UNKNOWN;
        

        BOOL fSplitPageOverflow;
        BOOL fNewPageOverflow;
        
        const BOOL fOverflow = !FBTISplitCausesNoOverflow( psplit, iline, &fSplitPageOverflow, &fNewPageOverflow );
        Assert( fOverflow == ( fSplitPageOverflow || fNewPageOverflow ) );

        Assert( !( fSplitPageOverflow && fNewPageOverflow ) );

        if( fOverflow )
        {
            if( fSplitPageOverflow && fNewPageOverflow )
            {

                Assert( 0 == *pilineCandidate );
                break;
            }


            DBEnforce( psplit->psplitPath->csr.Cpage().Ifmp(), psplit->clines > 2 );


            Assert( fSplitPageOverflow ^ fNewPageOverflow );
            splitpointChange = fSplitPageOverflow ? SPLIT_PAGE_SMALLER : SPLIT_PAGE_LARGER;
        }
        else
        {
            Assert( !fOverflow );
            Assert( !fSplitPageOverflow );
            Assert( !fNewPageOverflow );


            Assert( 0 == *pilineCandidate );
            *pilineCandidate            = iline;
            *pprefixinfoSplitCandidate  = psplit->prefixinfoSplit;
            *pprefixinfoNewCandidate    = psplit->prefixinfoNew;
            break;
        }


        Assert( UNKNOWN != splitpointChange );
        if( ( SPLIT_PAGE_LARGER == lastsplitpointChange && SPLIT_PAGE_SMALLER == splitpointChange )
            || ( SPLIT_PAGE_SMALLER == lastsplitpointChange && SPLIT_PAGE_LARGER == splitpointChange ) )
        {

            Assert( 0 == *pilineCandidate );
            break;
        }
        lastsplitpointChange = splitpointChange;
        Assert( UNKNOWN != lastsplitpointChange );
        

        if( SPLIT_PAGE_LARGER == splitpointChange )
        {
            
            iline++;


            if( psplit->clines == iline )
            {

                Assert( 0 == *pilineCandidate );
                break;
            }

            
            BTIRecalculatePrefix( psplit, iline-1, iline );
        }
        else if( SPLIT_PAGE_SMALLER == splitpointChange )
        {

            iline--;


            if( 0 == iline )
            {

                Assert( 0 == *pilineCandidate );
                break;
            }

            
            BTIRecalculatePrefix( psplit, iline+1, iline );
        }
        else
        {
            AssertRTL( fFalse );
        }
    }
}

VOID BTISelectRightSplit( SPLIT *psplit, FUCB *pfucb )
{
    INT         ilineCandidate;
    PREFIXINFO  prefixinfoSplitCandidate;
    PREFIXINFO  prefixinfoNewCandidate;

    psplit->splittype = splittypeRight;

    
    if ( psplit->fHotpoint )
    {
        BTISelectHotPointSplit( psplit, pfucb );
        return;
    }


    const BOOL fAppendLeaf = FBTISplitAppendLeaf( psplit );
    const BOOL fPrependSplit = FBTIPrependSplit( psplit );

Start:
    ilineCandidate      = 0;

    DBEnforce( psplit->psplitPath->csr.Cpage().Ifmp(), psplit->clines > 1 );

    if ( fAppendLeaf )
    {
        
        BTISelectAppendLeafSplit( psplit, &ilineCandidate, &prefixinfoSplitCandidate, &prefixinfoNewCandidate );
    }
    else if ( fPrependSplit )
    {
        BTISelectPrependLeafSplit( psplit, &ilineCandidate, &prefixinfoSplitCandidate, &prefixinfoNewCandidate );
    }
    else
    {
        
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


VOID BTISelectSplitWithOperNone( SPLIT *psplit, FUCB *pfucb )
{
    if ( splitoperInsert == psplit->splitoper )
    {
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

        psplit->psplitPath->csr.SetILine( psplit->ilineOper );
        NDGet( pfucb, &psplit->psplitPath->csr );

        psplit->rglineinfo[psplit->ilineOper].kdf = pfucb->kdfCurr;
        psplit->rglineinfo[psplit->ilineOper].cbSize =
            CbNDNodeSizeTotal( pfucb->kdfCurr );

        ULONG   cbMax   = CbBTIMaxSizeOfNode( pfucb, &psplit->psplitPath->csr );
        Assert( cbMax >= psplit->rglineinfo[psplit->ilineOper].cbSize );
        psplit->rglineinfo[psplit->ilineOper].cbSizeMax = cbMax;
    }

    psplit->ilineOper = 0;
    BTIRecalcWeightsLE( psplit );
    psplit->splitoper = splitoperNone;

    psplit->prefixinfoNew.Nullify();
    psplit->prefixinfoSplit.Nullify();
}


BOOL FBTISplitCausesNoOverflow( SPLIT *psplit, INT ilineSplit, BOOL * pfSplitPageOverflow, BOOL * pfNewPageOverflow )
{
#ifdef DEBUG
    if( FExpensiveDebugCodeEnabled( Debug_BT_Split ) )
    {
        PREFIXINFO  prefixinfo;

        if ( 0 == ilineSplit )
        {
            Assert( splittypeVertical == psplit->splittype );
            prefixinfo.Nullify();
        }
        else
        {
            BTISelectPrefix( psplit->rglineinfo,
                             ilineSplit,
                             &prefixinfo );
        }

        Assert( prefixinfo.cbSavings    == psplit->prefixinfoSplit.cbSavings );
        Assert( prefixinfo.ilineSegBegin
                    == psplit->prefixinfoSplit.ilineSegBegin );
        Assert( prefixinfo.ilineSegEnd
                    == psplit->prefixinfoSplit.ilineSegEnd );

        Assert( psplit->clines > ilineSplit );
        BTISelectPrefix( &psplit->rglineinfo[ilineSplit],
                         psplit->clines - ilineSplit,
                         &prefixinfo );

        Assert( prefixinfo.cbSavings    == psplit->prefixinfoNew.cbSavings );
        Assert( prefixinfo.ilineSegBegin
                    == psplit->prefixinfoNew.ilineSegBegin );
        Assert( prefixinfo.ilineSegEnd
                    == psplit->prefixinfoNew.ilineSegEnd );
    }
#endif

    Assert( splittypeVertical != psplit->splittype || ilineSplit == 0 );
    Assert( splittypeVertical == psplit->splittype || ilineSplit > 0 );
    Assert( ilineSplit < psplit->clines );

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


ERR ErrBTISeekSeparatorKey( SPLIT *psplit, FUCB *pfucb )
{
    ERR         err = JET_errSuccess;
    CSR         *pcsr = &psplit->psplitPath->psplitPathParent->csr;
    BOOKMARK    bm;


    Assert( !psplit->kdfParent.key.FNull() );
    Assert( sizeof( PGNO ) == psplit->kdfParent.data.Cb() );
    Assert( splittypeVertical != psplit->splittype );
    Assert( pcsr->Cpage().FInvisibleSons() );

    bm.key  = psplit->kdfParent.key;
    bm.data.Nullify();
    CallR( ErrNDSeek( pfucb, pcsr, bm ) );

    Assert( err == wrnNDFoundGreater );
    Assert( err != JET_errSuccess );
    if ( err == wrnNDFoundLess )
    {
        Assert( fFalse );
        Assert( pcsr->Cpage().Clines() - 1 == pcsr->ILine() );
        pcsr->IncrementILine();
    }
    err = JET_errSuccess;

    return err;
}


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

    Assert( pkey->FNull() );
    pkey->suffix.SetPv( RESBOOKMARK.PvRESAlloc() );
    if ( pkey->suffix.Pv() == NULL )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    pkey->suffix.SetCb( cbKeyCommon + cbDataCommon + 1 );

    Assert( cbKeyCommon <= pkey->Cb() );
    Assert( pkey->suffix.Pv() != NULL );
    kdfSplit.key.CopyIntoBuffer( pkey->suffix.Pv(),
                                 cbKeyCommon );

    if ( !fKeysEqual )
    {
        Assert( 0 == cbDataCommon );
        Assert( kdfSplit.key.Cb() > cbKeyCommon );

        if ( kdfSplit.key.prefix.Cb() > cbKeyCommon )
        {
            ( (BYTE *)pkey->suffix.Pv() )[cbKeyCommon] =
                    ( (BYTE *) kdfSplit.key.prefix.Pv() )[cbKeyCommon];
        }
        else
        {
            ( (BYTE *)pkey->suffix.Pv() )[cbKeyCommon] =
                    ( (BYTE *) kdfSplit.key.suffix.Pv() )[cbKeyCommon -
                                            kdfSplit.key.prefix.Cb() ];
        }
    }
    else
    {
        UtilMemCpy( (BYTE *)pkey->suffix.Pv() + cbKeyCommon,
                kdfSplit.data.Pv(),
                cbDataCommon );

        Assert( kdfSplit.data.Cb() > cbDataCommon );

        ( (BYTE *)pkey->suffix.Pv() )[cbKeyCommon + cbDataCommon] =
                ( (BYTE *)kdfSplit.data.Pv() )[cbDataCommon];
    }

    return JET_errSuccess;
}


ERR ErrBTISplitComputeSeparatorKey( SPLIT *psplit, FUCB *pfucb )
{
    ERR             err;
    KEYDATAFLAGS    *pkdfSplit = &psplit->rglineinfo[psplit->ilineSplit].kdf;
    KEYDATAFLAGS    *pkdfPrev = &psplit->rglineinfo[psplit->ilineSplit - 1].kdf;

    Assert( psplit->kdfParent.key.FNull() );
    Assert( psplit->kdfParent.data.FNull() );
    Assert( psplit->psplitPath->psplitPathParent != NULL );

    psplit->kdfParent.data.SetCb( sizeof( PGNO ) );
    psplit->kdfParent.data.SetPv( &psplit->pgnoSplit );

    if ( psplit->psplitPath->csr.Cpage().FInvisibleSons( ) || FFUCBRepair( pfucb ) )
    {
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

    const ULONG cbCommonPrevSav = rglineinfo[0].cbCommonPrev;
    ((LINEINFO *)rglineinfo)[0].cbCommonPrev = 0;

    INT         iline;

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

    ((LINEINFO *)rglineinfo)[0].cbCommonPrev = cbCommonPrevSav;
}
#endif


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

    const ULONG cbCommonPrevFirstSav = rglineinfo[0].cbCommonPrev;
    const ULONG cbCommonPrevLastSav  = rglineinfo[clines].cbCommonPrev;
    ((LINEINFO *)rglineinfo)[0].cbCommonPrev = 0;
    ((LINEINFO *)rglineinfo)[clines].cbCommonPrev = 0;

    INT         iline;

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
            continue;
        }

        if (    iline > 1
                && iline != clines - 2
                && rglineinfo[iline - 1].cbCommonPrev > rglineinfo[iline].cbCommonPrev
                && rglineinfo[iline + 1].cbCommonPrev < rglineinfo[iline].cbCommonPrev
             )
        {
            continue;
        }

        INT     ilineSegLeft;
        INT     ilineSegRight;
        INT     cbSavingsLeft = 0;
        INT     cbSavingsRight = 0;
        ULONG   cbCommonMin;

        cbCommonMin = rglineinfo[iline].cbCommonPrev;
        for ( ilineSegLeft = iline;
              rglineinfo[ilineSegLeft].cbCommonPrev > 0;
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

        cbCommonMin = rglineinfo[iline+1].cbCommonPrev;
        for ( ilineSegRight = iline + 1;
              rglineinfo[ilineSegRight].cbCommonPrev > 0;
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

    ((LINEINFO *)rglineinfo)[0].cbCommonPrev = cbCommonPrevFirstSav;
    ((LINEINFO *)rglineinfo)[clines].cbCommonPrev = cbCommonPrevLastSav;

    #ifdef DEBUG
    if( FExpensiveDebugCodeEnabled( Debug_BT_Split ) )
    {
        PREFIXINFO  prefixinfoT;
        BTISelectPrefixCheck( rglineinfo, clines, &prefixinfoT );
        Assert( prefixinfoT.cbSavings == pprefixinfo->cbSavings );
        Assert( prefixinfoT.ilineSegBegin == pprefixinfo->ilineSegBegin );
        Assert( prefixinfoT.ilineSegEnd == pprefixinfo->ilineSegEnd );
    }
    #endif
}


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
        Assert( !pprefixinfo->FNull() );
        BTISelectPrefix( rglineinfo, clines, pprefixinfo );
    }
    else
    {
    }

    return;
}


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
        if ( !pprefixinfo->FNull() )
        {
            pprefixinfo->ilinePrefix++;
            pprefixinfo->ilineSegBegin++;
            pprefixinfo->ilineSegEnd++;
        }
    }
    else if ( pprefixinfo->FNull() )
    {

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
        Assert( clines > 1 );
        Assert( pprefixinfo->ilineSegEnd + 1 + 1 <= clines );
        BTISelectPrefix( rglineinfo,
                         pprefixinfo->ilineSegEnd + 1 + 1,
                         pprefixinfo );
    }

    return;
}


LOCAL VOID BTISelectPrefixes( SPLIT *psplit, INT ilineSplit )
{
    if ( 0 == ilineSplit )
    {
        Assert( splittypeVertical == psplit->splittype );
        Assert( psplit->prefixinfoSplit.FNull() );
        BTISelectPrefix( &psplit->rglineinfo[ilineSplit],
                         psplit->clines - ilineSplit,
                         &psplit->prefixinfoNew );
    }
    else
    {
        Assert( psplit->clines > ilineSplit );

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

        BTISelectPrefixIncrement( &psplit->rglineinfo[ilineSplit],
                                  psplit->clines - ilineSplit,
                                  &psplit->prefixinfoNew );
    }
}


LOCAL VOID BTISetPrefix( LINEINFO *rglineinfo, INT clines, const PREFIXINFO& prefixinfo )
{
    Assert( clines > 0 );

    INT         iline;

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

    const INT   ilineSegLeft    = prefixinfo.ilineSegBegin;
    const INT   ilineSegRight   = prefixinfo.ilineSegEnd;
    const INT   ilinePrefix     = prefixinfo.ilinePrefix;
    ULONG       cbCommonMin;

    #ifdef DEBUG
    INT     cbSavingsLeft = 0;
    INT     cbSavingsRight = 0;
    #endif

    Assert( ilineSegLeft != ilineSegRight );

    rglineinfo[ilinePrefix].cbPrefix = rglineinfo[ilinePrefix].kdf.key.Cb();

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
    const INT   cbSavings = cbSavingsLeft + cbSavingsRight - cbPrefixOverhead;

    Assert( cbSavings > 0 );
    Assert( prefixinfo.ilinePrefix == ilinePrefix );
    Assert( prefixinfo.cbSavings == cbSavings );
    #endif
}

VOID BTISplitSetPrefixes( SPLIT *psplit )
{
    const INT   ilineSplit = psplit->ilineSplit;

    if ( 0 == ilineSplit )
    {
        Assert( splittypeVertical == psplit->splittype );
        Assert( ilineInvalid == psplit->prefixinfoSplit.ilinePrefix );
    }
    else
    {
        BTISetPrefix( psplit->rglineinfo,
                      ilineSplit,
                      psplit->prefixinfoSplit );
    }

    Assert( psplit->clines > ilineSplit );
    BTISetPrefix( &psplit->rglineinfo[ilineSplit],
                  psplit->clines - ilineSplit,
                  psplit->prefixinfoNew );
}

LOCAL ERR ErrBTIGetNewPages( FUCB *pfucb, SPLITPATH *psplitPathLeaf, DIRFLAG dirflag )
{
    ERR         err;
    SPLITPATH   *psplitPath;
    const BOOL fLogging = !( dirflag & fDIRNoLog ) && g_rgfmp[pfucb->ifmp].FLogOn();

    Assert( pfucb->pcsrRoot == pcsrNil );
    for ( psplitPath = psplitPathLeaf;
          psplitPath->psplitPathParent != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
    }
    pfucb->pcsrRoot = &psplitPath->csr;

    Assert( psplitPath->psplitPathParent == NULL );
    for ( ; psplitPath != NULL; psplitPath = psplitPath->psplitPathChild )
    {
        if ( psplitPath->psplit != NULL )
        {
            SPLIT * psplit      = psplitPath->psplit;

            Assert( psplit->psplitPath == psplitPath );

            BTICheckSplitFlags( psplit );

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

            if ( !g_rgfmp[pfucb->ifmp].FSeekPenalty() )
            {
                fSPAllocFlags &= ~fSPContinuous;
            }

            fSPAllocFlags |= pfucb->u.pfcb->FUtilizeExactExtents() ? fSPExactExtent : 0;

            Assert( !!psplitPath->csr.Cpage().FLeafPage() == !!( psplit->fNewPageFlags & CPAGE::fPageLeaf ) );
            if ( CpgDIRActiveSpaceRequestReserve( pfucb ) != 0 &&
                    CpgDIRActiveSpaceRequestReserve( pfucb ) != cpgDIRReserveConsumed &&
                    ( ( psplit->fNewPageFlags & CPAGE::fPageLeaf ) || fVerticalToLeafSplit ) )
            {
                fSPAllocFlags |= ( fSPContinuous | fSPUseActiveReserve );
                if ( fVerticalToLeafSplit )
                {
                    const CPG cpgReserveAdjust = CpgDIRActiveSpaceRequestReserve( pfucb ) + 1;
                    DIRSetActiveSpaceRequestReserve( pfucb, cpgReserveAdjust );
                }
            }

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

            Ptls()->threadstats.cPageUpdateAllocated++;
        }
    }

    pfucb->pcsrRoot = pcsrNil;
    return JET_errSuccess;

HandleError:
    Assert( err < JET_errSuccess );
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

            const ERR   errFreeExt  = ErrSPFreeExt( pfucb, psplit->pgnoNew, 1, "FailedNewPages" );
#ifdef DEBUG
            if ( !FSPExpectedError( errFreeExt ) )
            {
                CallS( errFreeExt );
            }
#endif

            psplit->pgnoNew = pgnoNull;

            Ptls()->threadstats.cPageUpdateReleased++;
        }
    }

    pfucb->pcsrRoot = pcsrNil;
    return err;
}


LOCAL VOID BTISplitReleaseUnneededPages( INST *pinst, SPLITPATH **ppsplitPathLeaf )
{
    SPLITPATH   *psplitPath;
    SPLITPATH   *psplitPathNewLeaf = NULL;

    for ( psplitPath = *ppsplitPathLeaf;
          psplitPath->psplitPathParent != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
    }

    for ( ; NULL != psplitPath;  )
    {
        SPLIT   *psplit = psplitPath->psplit;

        if ( psplit == NULL &&
             ( psplitPath->psplitPathChild == NULL ||
               psplitPath->psplitPathChild->psplit == NULL ) )
        {
            SPLITPATH *psplitPathT = psplitPath;
            psplitPath = psplitPath->psplitPathChild;

            delete psplitPathT;
        }
        else
        {
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


LOCAL ERR ErrBTISplitUpgradeLatches( const IFMP ifmp, SPLITPATH * const psplitPathLeaf )
{
    ERR             err;
    SPLITPATH       * psplitPath;
    const DBTIME    dbtimeSplit     = g_rgfmp[ifmp].DbtimeIncrementAndGet();

    Assert( dbtimeSplit > 1 );
    Assert( PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() != fRecoveringRedo
        || g_rgfmp[ifmp].FCreatingDB() );

    for ( psplitPath = psplitPathLeaf;
          psplitPath->psplitPathParent != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
    }

    Assert( NULL == psplitPath->psplitPathParent );
    for ( ; NULL != psplitPath;  psplitPath = psplitPath->psplitPathChild )
    {
        SPLIT   *psplit = psplitPath->psplit;

        Assert( psplit != NULL ||
                ( psplitPath->psplitPathChild != NULL &&
                  psplitPath->psplitPathChild->psplit != NULL ) );
        Assert( psplitPath->csr.Latch() == latchWrite ||
                psplitPath->csr.Latch() == latchRIW );

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


        if ( psplitPath->csr.Latch() != latchWrite )
        {
            psplitPath->csr.UpgradeFromRIWLatch();
        }


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

        if ( psplit != NULL )
        {
            Assert( psplit->csrNew.Latch() == latchWrite );
            psplit->csrNew.CoordinatedDirty( dbtimeSplit );
        }

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
            dbtimeInvalid != psplitPath->dbtimeBefore  )
        {
            psplitPath->csr.RevertDbtime( psplitPath->dbtimeBefore, psplitPath->fFlagsBefore );
        }

        SPLIT   *psplit = psplitPath->psplit;

        if ( psplit != NULL && pgnoNull != psplit->csrRight.Pgno() )
        {

            Assert( psplit->splittype != splittypeAppend );
            Assert( psplit->csrRight.Cpage().FLeafPage() );

            Assert( dbtimeInvalid != psplit->dbtimeRightBefore );
            Assert( psplit->dbtimeRightBefore < psplit->csrRight.Dbtime() );

            if ( dbtimeNil != psplit->dbtimeRightBefore &&
                dbtimeInvalid != psplit->dbtimeRightBefore && 
                latchWrite == psplit->csrRight.Latch()  )
            {
                psplit->csrRight.RevertDbtime( psplit->dbtimeRightBefore, psplit->fFlagsRightBefore );
            }
        }
    }
}


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


VOID BTISplitSetCursor( FUCB *pfucb, SPLITPATH *psplitPathLeaf )
{
    SPLIT   *psplit = psplitPathLeaf->psplit;

    Assert( NULL != psplit );
    Assert( !Pcsr( pfucb )->FLatched() );

    if ( splitoperNone == psplit->splitoper ||
         !psplit->csrNew.Cpage().FLeafPage() )
    {
        BTUp( pfucb );
    }
    else
    {
        Assert( psplit->csrNew.Cpage().FLeafPage() );
        Assert( splitoperNone != psplitPathLeaf->psplit->splitoper );
        Assert( !Pcsr( pfucb )->FLatched() );

        if ( psplit->ilineOper < psplit->ilineSplit )
        {
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


VOID BTIPerformSplit( FUCB          *pfucb,
                      SPLITPATH     *psplitPathLeaf,
                      KEYDATAFLAGS  *pkdf,
                      DIRFLAG       dirflag )
{
    SPLITPATH   *psplitPath;

    ASSERT_VALID( pkdf );

    auto tc = TcCurr();

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

            BTIInsertPgnoNewAndSetPgnoSplit( pfucb, psplitPath );

            continue;
        }

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

            BTISplitMoveNodes( pfucb, psplit, pkdfOper, dirflag );

            CSR     *pcsrRoot = &psplit->psplitPath->csr;

            if ( FBTIUpdatablePage( *pcsrRoot ) )
            {
                Assert( latchWrite == pcsrRoot->Latch() );
                Assert( pcsrRoot->Cpage().FRootPage() );

                pcsrRoot->Cpage().SetFlags( psplit->fSplitPageFlags );

                Assert( NULL != psplit->psplitPath );
                Assert( 0 == pcsrRoot->Cpage().Clines() );

#ifdef DEBUG
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
                BTISplitFixSiblings( psplit );
            }
            else
            {
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

                BTIInsertPgnoNew( pfucb, psplitPath );
                AssertBTIVerifyPgnoSplit( pfucb, psplitPath );
            }
        }
    }
}


LOCAL VOID BTIInsertPgnoNewAndSetPgnoSplit( FUCB *pfucb, SPLITPATH *psplitPath )
{
    ERR         err;
    CSR         *pcsr = &psplitPath->csr;
    DATA        data;
    BOOKMARK    bmParent;

    if ( !FBTIUpdatablePage( *pcsr ) )
    {
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
    err = ErrNDSeek( pfucb, pcsr, bmParent );
    AssertRTL( err != JET_errBadEmptyPage );
    Assert( err != JET_errSuccess );
    Assert( err != wrnNDFoundLess );
    Assert( err == wrnNDFoundGreater );
    Assert( psplit->pgnoSplit ==
            *( reinterpret_cast<UnalignedLittleEndian< PGNO > *> ( pfucb->kdfCurr.data.Pv() ) ) );
    Assert( pcsr->FDirty() );

    BTIComputePrefixAndInsert( pfucb, pcsr, psplit->kdfParent );

    Assert( pcsr->ILine() < pcsr->Cpage().Clines() );
    pcsr->IncrementILine();
#ifdef DEBUG
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
        pcsr            = &psplit->csrNew;
        pcsr->SetILine( psplit->ilineOper + 1 - psplit->ilineSplit );
    }
    else
    {
        Assert( splittypeVertical != psplit->splittype );
        pcsr            = &psplit->psplitPath->csr;
        pcsr->SetILine( psplit->ilineOper + 1 );
    }

    if ( !FBTIUpdatablePage( *pcsr ) )
    {
        return;
    }
    Assert( latchWrite == pcsr->Latch() );

    Assert( pcsr->FDirty() );

#ifdef DEBUG
    NDGet( pfucb, pcsr );
    Assert( sizeof(PGNO) == pfucb->kdfCurr.data.Cb() );
    Assert( psplitPath->psplitPathChild->psplit->pgnoSplit ==
            *( reinterpret_cast<UnalignedLittleEndian< PGNO >  *>
                    (pfucb->kdfCurr.data.Pv() ) ) );
#endif

    NDReplace( pcsr, &data );
}


LOCAL VOID BTISplitFixSiblings( SPLIT *psplit )
{
    SPLITPATH   *psplitPath = psplit->psplitPath;

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

    if ( psplit->prefixinfoNew.ilinePrefix != ilineInvalid
        && FBTIUpdatablePage( *pcsrDest ) )
    {
        const INT   ilinePrefix = psplit->prefixinfoNew.ilinePrefix + psplit->ilineSplit;

        Assert( ilinePrefix < psplit->clines );
        Assert( ilinePrefix >= psplit->ilineSplit );

        NDSetPrefix( pcsrDest, psplit->rglineinfo[ilinePrefix].kdf.key );
    }

    if ( psplit->splitoper != splitoperNone
        && psplit->ilineOper >= psplit->ilineSplit )
    {
        Assert( 0 == pcsrDest->ILine() );
        BTISplitBulkCopy( pfucb,
                          psplit,
                          psplit->ilineSplit,
                          psplit->ilineOper - psplit->ilineSplit );

        pcsrSrc->SetILine( psplit->ilineOper );

        if ( FBTIUpdatablePage( *pcsrDest ) )
        {
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

        if ( splittypeAppend != psplit->splittype )
        {
            BTISplitSetPrefixInSrcPage( pfucb, psplit );
        }
    }
    else
    {
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

        Assert( splittypeAppend != psplit->splittype );
        BTISplitSetPrefixInSrcPage( pfucb, psplit );


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
                    Assert( psplit->splitoper == splitoperFlagInsertAndReplaceData ||
                            psplit->splitoper == splitoperReplace );

#ifdef DEBUG
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
            Assert( !FBTIUpdatablePage( *pcsrDest ) );
        }
    }

    if ( psplit->fNewPageFlags & CPAGE::fPageLeaf )
    {
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering()
            || ( pcsrDest->Cpage().FLeafPage() && latchWrite == pcsrDest->Latch() ) );
        if ( FBTIUpdatablePage( *pcsrDest ) )
        {
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
            Assert( !FBTIUpdatablePage( *pcsrDest )
                || !FBTISplitPageBeforeImageRequired( psplit ) );
        }


        Assert( ( splittypeVertical == psplit->splittype && psplit->kdfParent.key.FNull() )
            || ( splittypeVertical != psplit->splittype && !psplit->kdfParent.key.FNull() ) );
        if ( !FBTIUpdatablePage( *pcsrSrc ) )
        {
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
        return;
    }
    Assert( latchWrite == pcsr->Latch() );

    NDBulkDelete( pcsr, clines );
}


VOID BTISplitBulkCopy( FUCB *pfucb, SPLIT *psplit, INT ilineStart, INT clines )
{
    INT         iline;
    const INT   ilineEnd    = ilineStart + clines;
    CSR         *pcsrDest   = &psplit->csrNew;

    if ( !FBTIUpdatablePage( *pcsrDest ) )
    {
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

VOID BTISplitSetPrefixInSrcPage( FUCB *pfucb, SPLIT *psplit )
{
    Assert( psplit->splittype != splittypeAppend );
    Assert( !psplit->prefixSplitNew.FNull()
        || psplit->prefixinfoSplit.ilinePrefix == ilineInvalid );

    CSR         *pcsr = &psplit->psplitPath->csr;

    if ( !FBTIUpdatablePage( *pcsr ) )
    {
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

    if ( !pprefixOld->FNull() )
    {
        KEY keyNull;

        keyNull.Nullify();
        NDSetPrefix( pcsr, keyNull );
    }
    else
    {
#ifdef DEBUG
        NDGetPrefix( pfucb, pcsr );
        Assert( pfucb->kdfCurr.key.FNull() );
#endif
    }

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

    KEY     keyPrefixNew;

    keyPrefixNew.Nullify();
    keyPrefixNew.prefix = *pprefixNew;
    NDSetPrefix( pcsr, keyPrefixNew );

    return;
}

VOID BTISplitShrinkPages( SPLITPATH *psplitPathLeaf )
{
    SPLITPATH *psplitPath = psplitPathLeaf;

    for ( ; psplitPath != NULL; )
    {
        if( psplitPath->csr.Latch() == latchWrite )
        {
            AssertRTL( psplitPath->dbtimeBefore != psplitPath->csr.Cpage().Dbtime() );
            if ( psplitPath->dbtimeBefore != psplitPath->csr.Cpage().Dbtime() )
            {
                psplitPath->csr.Cpage().OptimizeInternalFragmentation();
            }
        }

        psplitPath = psplitPath->psplitPathParent;
    }
}


VOID BTIReleaseSplitPaths( INST *pinst, SPLITPATH *psplitPathLeaf )
{
    SPLITPATH *psplitPath = psplitPathLeaf;

    for ( ; psplitPath != NULL; )
    {
        SPLITPATH *psplitPathParent = psplitPath->psplitPathParent;

        delete psplitPath;
        psplitPath = psplitPathParent;
    }
}


LOCAL VOID BTISplitCheckPath( SPLITPATH *psplitPathLeaf )
{
#ifdef DEBUG
    SPLITPATH   *psplitPath;
    BOOL        fOperNone = fFalse;

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

    LINEINFO    *plineinfo = &psplit->rglineinfo[psplit->ilineOper];
    if ( splitoperInsert == psplit->splitoper )
    {
        Assert( plineinfo->kdf == kdf );
    }
#endif
}


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


VOID BTICheckOneSplit( FUCB             *pfucb,
                       SPLITPATH        *psplitPath,
                       KEYDATAFLAGS     *pkdf,
                       DIRFLAG          dirflag )
{
#ifdef DEBUG
    SPLIT           *psplit = psplitPath->psplit;
    const DBTIME    dbtime  = psplitPath->csr.Dbtime();



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
            Assert( psplit->csrNew.Cpage().Clines() == 1 );
            Assert( psplit->csrRight.Pgno() == pgnoNull );
            Assert( psplit->csrNew.Cpage().PgnoNext() == pgnoNull );

        case splittypeRight:
            CSR             *pcsrParent     = &psplitPath->psplitPathParent->csr;
            CSR             *pcsrSplit      = &psplitPath->csr;
            CSR             *pcsrNew        = &psplit->csrNew;
            CSR             *pcsrRight      = &psplit->csrRight;

            KEYDATAFLAGS    kdfLess, kdfGreater;

            if ( psplitPath->psplitPathParent->psplit != NULL &&
                 splittypeVertical == psplitPath->psplitPathParent->psplit->splittype )
            {
                pcsrParent = &psplitPath->psplitPathParent->psplit->csrNew;
            }

            Assert( pcsrSplit->Dbtime() == dbtime );
            Assert( pcsrNew->Dbtime() == dbtime );
            Assert( pcsrParent->Dbtime() == dbtime );

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


            }

            NDMoveFirstSon( pfucb, pcsrParent );

            Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
            PGNO    pgnoChild = *( (UnalignedLittleEndian< PGNO > *)pfucb->kdfCurr.data.Pv() );
            for ( ; pgnoChild != psplit->pgnoSplit ; )
            {

                Assert( pgnoChild != psplit->pgnoNew );
                Assert( pgnoChild != psplit->csrRight.Pgno() );

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
                Assert( FKeysEqual( kdfGreater.key, pfucb->kdfCurr.key ) );
            }

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
                Assert( FKeysEqual( kdfGreater.key, pfucb->kdfCurr.key ) );
            }

            if ( pcsrRight->Pgno() != pgnoNull )
            {
                Assert( pcsrRight->Cpage().FLeafPage() );

                kdfLess = kdfGreater;
                NDMoveFirstSon( pfucb, pcsrRight );

                Assert( !kdfLess.key.FNull() );
                Assert( CmpKeyWithKeyData( kdfLess.key, pfucb->kdfCurr ) <= 0 );
            }
    }
#endif
}


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


INLINE ERR ErrBTISPCSeek( FUCB *pfucb, const BOOKMARK& bm )
{
    ERR     err;

    Assert( !Pcsr( pfucb )->FLatched( ) );

    Call( ErrBTIGotoRoot( pfucb, latchReadNoTouch ) );
    Assert( 0 == Pcsr( pfucb )->ILine() );

    if ( Pcsr( pfucb )->Cpage().Clines() == 0 )
    {
        Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
        err = wrnNDFoundGreater;
    }
    else
    {
        for ( ; ; )
        {
            Call( ErrNDSeek( pfucb, bm ) );

            if ( !Pcsr( pfucb )->Cpage().FInvisibleSons( ) )
            {
                break;
            }
            else
            {
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

                VERNullifyInactiveVersionsOnBM( pfucb, bm );

                fUpdated = fTrue;
            }
            else if ( FFUCBUnique( pfucb ) && !( pcsr->Cpage().FLongValuePage() && FIsLVRootKey( kdf.key ) ) )
            {
                BOOKMARK bmSaved = pfucb->bmCurr;
                pfucb->bmCurr = bm;
                NDGet( pfucb, pcsr );
                
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

    Call( ErrBTIResetPrefixIfUnused( pfucb, pcsr ) );


    if ( fUpdated )
    {
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

        pcsrRoot->Cpage().SetLgposModify( lgpos );

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

    BTIMergeShrinkPages( pmergePathLeaf );

    BTIReleaseMergePaths( pmergePathLeaf );
    CallR( err );


    const BOOL      fAvailExt   = FFUCBAvailExt( pfucb );
    const BOOL      fOwnExt     = FFUCBOwnExt( pfucb );

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

    BTUp( pfucb );
    for ( ULONG i = 0; i < cPagesToFree; i++ )
    {
        Assert( PgnoRoot( pfucb ) != rgemptypage[i].pgno );
        const ERR   errFreeExt  = ErrSPFreeExt( pfucb, rgemptypage[i].pgno, 1, "MergeEmptyTree" );
#ifdef DEBUG
        if ( !FSPExpectedError( errFreeExt ) )
        {
            CallS( errFreeExt );
        }
#endif

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
        if ( NULL != pbmNext )
        {
            pbmNext->key.suffix.SetCb( 0 );
            pbmNext->data.SetCb( 0 );
        }

        return JET_errSuccess;
    }

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

    BTIMergeReleaseUnneededPages( pmergePath );

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
            Assert( pgnoNull != pmergePath->csr.Cpage().PgnoPrev() );
            Assert( latchRIW == pmergePath->pmerge->csrLeft.Latch() );

            Assert( pgnoNull == pmergePath->csr.Cpage().PgnoNext()
                || latchRIW == pmergePath->pmerge->csrRight.Latch() );

            Call( ErrBTIMergeOrEmptyPage( pfucb, pmergePath ) );
            break;

        case mergetypePartialRight:
        case mergetypeFullRight:
            Assert( pgnoNull != pmergePath->csr.Cpage().PgnoNext() );
            Assert( latchRIW == pmergePath->pmerge->csrRight.Latch() );

            Assert( pgnoNull == pmergePath->csr.Cpage().PgnoPrev()
                || latchRIW == pmergePath->pmerge->csrLeft.Latch() );

            Call( ErrBTIMergeOrEmptyPage( pfucb, pmergePath ) );
            break;

        default:
            Assert( pmergePath->pmerge->mergetype == mergetypeNone );
            Assert( latchRIW == pmergePath->csr.Latch() );

            if ( pmergePath->csr.Cpage().Clines() == 1 )
            {
                goto HandleError;
            }

            pmergePath->csr.UpgradeFromRIWLatch();

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

    err = csrSibling.ErrGetReadPage( pfucb->ppib, pfucb->ifmp, pgnoSibling, BFLatchFlags( bflfNoTouch | bflfNoWait ) );
    if ( errBFLatchConflict == err )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    Call( err );

    *pfMergeable = FBTISPCCheckMergeable( pfucb, &csrSibling, rglineinfo );
    if ( !*pfMergeable )
    {
        auto tc = TcCurr();
        PERFOpt( PERFIncCounterTable( cBTUnnecessarySiblingLatch, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );
    }
    else
    {
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
    __in FUCB * const pfucb,
    const INT pctFull,
    __in LINEINFO * const rglineinfo,
    __out BOOL * const pfMultipageOLC)
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

LOCAL ERR ErrBTISinglePageCleanup( FUCB *pfucb, const BOOKMARK& bm )
{
    Assert( !Pcsr( pfucb )->FLatched() );

    ERR         err;

    if ( pfucb->u.pfcb->FDeletePending() )
    {
        return JET_errSuccess;
    }

    CallR( ErrBTISPCSeek( pfucb, bm ) );

    LINEINFO    *rglineinfo = NULL;
    BOOL        fEmptyPage;
    INT         pctFull;

    err = Pcsr( pfucb )->ErrUpgradeFromReadLatch( );
    if ( errBFLatchConflict == err )
    {
        auto tc = TcCurr();
        PERFOpt( PERFIncCounterTable( cBTFailedSPCWriteLatch, PinstFromPfucb( pfucb ), (TCE)tc.nParentObjectClass ) );
        err = ErrERRCheck( wrnBTMultipageOLC );
        goto HandleError;
    }
    Call( err );

    Assert( latchWrite == Pcsr( pfucb )->Latch() );
    Call( ErrBTISPCDeleteNodes( pfucb, Pcsr( pfucb ) ) );

    Call( ErrBTISPCCollectLeafPageInfo(
                pfucb,
                Pcsr( pfucb ),
                &rglineinfo,
                NULL,
                &fEmptyPage,
                NULL,
                &pctFull ) );

    if ( fEmptyPage )
    {
        err = ErrERRCheck( wrnBTMultipageOLC );
        goto HandleError;
    }

    Pcsr( pfucb )->Downgrade( latchRIW );

    BOOL fMultipageOLC;
    Call( ErrBTICheckForMultipageOLC( pfucb, pctFull, rglineinfo, &fMultipageOLC ) );
    if ( fMultipageOLC )
    {
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

    CallR( ErrBTINewMergePath( ppmergePath ) );
    Assert( NULL != *ppmergePath );
    CSR* pcsr = &( (*ppmergePath)->csr );

    Call( pcsr->ErrGetRIWPage( pfucb->ppib,
                                             pfucb->ifmp,
                                             PgnoRoot( pfucb ) ) );

    if ( pcsr->Cpage().FLeafPage() ||
        ( !fLeafPage && ( pgnoSearch == pcsr->Pgno() ) ) )
    {
        Error( ErrERRCheck( wrnBTShallowTree ) );
    }

    BOOL    fLeftEdgeOfBtree    = fTrue;
    BOOL    fRightEdgeOfBtree   = fTrue;

    Assert( pcsr->Cpage().Clines() > 0 );

    for ( ; ; )
    {
        if ( fLeafPage )
        {
            Call( ErrNDSeek( pfucb, pcsr, bmSearch ) );
        }
        else
        {
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

        (*ppmergePath)->iLine = SHORT( pcsr->ILine() );
        Assert( (*ppmergePath)->iLine < pcsr->Cpage().Clines() );

        if ( pcsr->Cpage().FLeafPage() ||
            ( !fLeafPage && ( pgnoSearch == pcsr->Pgno() ) ) )
        {
            Assert( !!pcsr->Cpage().FLeafPage() == !!fLeafPage );
            Assert( fLeafPage || ( pgnoSearch == pcsr->Pgno() ) );

            const MERGEPATH * const pmergePathParent        = (*ppmergePath)->pmergePathParent;

            Assert( NULL != pmergePathParent );
            Assert( !( pcsr->Cpage().FRootPage() ) );

            if ( pcsr->Cpage().FLeafPage() )
            {
                const BOOL              fLeafPageIsFirstPage    = ( pgnoNull == pcsr->Cpage().PgnoPrev() );
                const BOOL              fLeafPageIsLastPage     = ( pgnoNull == pcsr->Cpage().PgnoNext() );

                if ( fLeftEdgeOfBtree ^ fLeafPageIsFirstPage )
                {
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

        Call( ErrBTINewMergePath( ppmergePath ) );
        pcsr = &( (*ppmergePath)->csr );

        Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
        Call( pcsr->ErrGetRIWPage(
                                pfucb->ppib,
                                pfucb->ifmp,
                                *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv() ) );

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


LOCAL VOID BTIMergeCopyNextBookmark( FUCB       * const pfucb,
                                     MERGEPATH  * const pmergePathLeaf,
                                     BOOKMARK   * const pbmNext,
                                     const BOOL         fRightMerges )
                                     
{
    Assert( NULL != pmergePathLeaf );
    Assert( NULL != pbmNext );
    Assert( pmergePathLeaf->csr.FLatched() );
    Assert( pmergePathLeaf->pmerge != NULL );

    Assert( pbmNext->key.prefix.FNull() );

    CSR * const pcsr = fRightMerges ? &(pmergePathLeaf->pmerge->csrLeft) : &(pmergePathLeaf->pmerge->csrRight);
    
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

    Assert( NULL != pbmNext->key.suffix.Pv() );
    Assert( 0 == pbmNext->key.prefix.Cb() );
    bm.key.CopyIntoBuffer( pbmNext->key.suffix.Pv(), bm.key.Cb() );
    pbmNext->key.suffix.SetCb( bm.key.Cb() );

    pbmNext->data.SetPv( (BYTE *) pbmNext->key.suffix.Pv() + pbmNext->key.Cb() );
    bm.data.CopyInto( pbmNext->data );

    return;
}


LOCAL ERR ErrBTISelectMerge(
    FUCB            * const pfucb,
    MERGEPATH       * const pmergePathLeaf,
    const BOOKMARK&         bm,
    BOOKMARK        * const pbmNext,
    RECCHECK        * const preccheck,
    const BOOL              fRightMerges )
{
    ERR             err;

    Assert( pmergePathLeaf->csr.Cpage().FLeafPage() );

    Call( ErrBTINewMerge( pmergePathLeaf ) );
    Assert( NULL != pmergePathLeaf->pmerge );

    MERGE * const pmerge = pmergePathLeaf->pmerge;
    pmerge->mergetype = mergetypeNone;

    Call( ErrBTIMergeCollectPageInfo( pfucb, pmergePathLeaf, preccheck, fRightMerges ) );

    if ( mergetypeNone == pmerge->mergetype && NULL == pbmNext )
    {
        return err;
    }
    else if ( mergetypeEmptyTree == pmerge->mergetype )
    {
        return err;
    }

    Call( ErrBTIMergeLatchSiblingPages( pfucb, pmergePathLeaf ) );

    if ( NULL != pbmNext )
    {
        BTIMergeCopyNextBookmark( pfucb, pmergePathLeaf, pbmNext, fRightMerges );
    }

    Assert( pmergePathLeaf->pmergePathParent != NULL || mergetypeNone == pmerge->mergetype );

    {
        BTIReleaseMergeLineinfo( pmerge );

        Call( ErrNDSeek( pfucb, &pmergePathLeaf->csr, bm ) );

        pmergePathLeaf->iLine = SHORT( pmergePathLeaf->csr.ILine() );

        Call( ErrBTIMergeCollectPageInfo( pfucb, pmergePathLeaf, NULL, fRightMerges ) );
    }

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
            
            BTICheckMergeable( pfucb, pmergePathLeaf, fRightMerges );
            if ( mergetypeNone == pmerge->mergetype )
            {
                return err;
            }

            Call( ErrBTISelectMergeInternalPages( pfucb, pmergePathLeaf ) );

            if ( mergetypeEmptyPage != pmerge->mergetype )
            {
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


ERR ErrBTINewMerge( MERGEPATH *pmergePath )
{
    MERGE   *pmerge;

    Assert( pmergePath != NULL );
    Assert( pmergePath->pmerge == NULL );

    if ( !( pmerge = new MERGE ) )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

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


VOID BTIMergeRevertDbtime( MERGEPATH *pmergePathTip )
{
    MERGEPATH *pmergePath = pmergePathTip;

    Assert( NULL == pmergePath->pmergePathChild );
    for ( ; NULL != pmergePath;
            pmergePath = pmergePath->pmergePathParent )
    {
        MERGE   *pmerge = pmergePath->pmerge;

        if ( NULL != pmerge )
        {
            if ( pgnoNull != pmerge->csrLeft.Pgno() &&
                latchWrite == pmergePath->pmerge->csrLeft.Latch() &&
                dbtimeNil != pmerge->dbtimeLeftBefore )
            {
                Assert( pmerge->csrLeft.Cpage().FLeafPage() );

                Assert ( dbtimeInvalid != pmerge->dbtimeLeftBefore );
                Assert ( pmerge->dbtimeLeftBefore < pmerge->csrLeft.Dbtime() );
                if ( dbtimeInvalid != pmerge->dbtimeLeftBefore )
                {
                    pmerge->csrLeft.RevertDbtime( pmerge->dbtimeLeftBefore, pmerge->fFlagsLeftBefore );
                }
            }
        }

        if ( latchWrite == pmergePath->csr.Latch() &&
            dbtimeNil != pmergePath->dbtimeBefore )
        {
            Assert ( latchWrite == pmergePath->csr.Latch() );

            Assert ( dbtimeInvalid != pmergePath->dbtimeBefore );
            Assert ( pmergePath->dbtimeBefore < pmergePath->csr.Dbtime() );
            if ( dbtimeInvalid != pmergePath->dbtimeBefore )
            {
                pmergePath->csr.RevertDbtime( pmergePath->dbtimeBefore, pmergePath->fFlagsBefore );
            }
        }

        if ( pmerge != NULL )
        {
            if ( pgnoNull != pmerge->csrRight.Pgno() &&
                latchWrite == pmerge->csrRight.Latch() &&
                dbtimeNil != pmerge->dbtimeRightBefore )
            {
                Assert( pmerge->csrRight.Cpage().FLeafPage() );

                Assert ( dbtimeInvalid != pmerge->dbtimeRightBefore );
                Assert ( pmerge->dbtimeRightBefore < pmerge->csrRight.Dbtime() );
                if ( dbtimeInvalid != pmerge->dbtimeRightBefore )
                {
                    pmerge->csrRight.RevertDbtime( pmerge->dbtimeRightBefore, pmerge->fFlagsRightBefore );
                }
            }
        }
    }
}


LOCAL INT IlineBTIFrac( FUCB *pfucb, DIB *pdib )
{
    INT         iLine;
    INT         clines      = Pcsr( pfucb )->Cpage().Clines( );
    FRAC        *pfrac      = (FRAC *)pdib->pbm;
    const INT   clinesMax   = 4096;

    Assert( pdib->pos == posFrac );
    Assert( pfrac->ulTotal >= pfrac->ulLT );

    iLine = INT( ( __int64( pfrac->ulLT ) * clines ) / pfrac->ulTotal );
    Assert( iLine <= clines );
    if ( iLine >= clines )
    {
        iLine = clines - 1;
    }

    if ( pfrac->ulTotal / clines == 0 )
    {
        pfrac->ulTotal *= clinesMax;
        pfrac->ulLT *= clinesMax;
    }

    Assert( pfrac->ulTotal > 0 );
    pfrac->ulLT = INT( ( __int64( pfrac->ulLT ) * clines - __int64( iLine ) * pfrac->ulTotal ) / clines );
    pfrac->ulTotal /= clines;

    Assert( pfrac->ulLT <= pfrac->ulTotal );
    return iLine;
}


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

    Assert( NULL == *pplineinfo );
    *pplineinfo = new LINEINFO[clines];

    if ( NULL == *pplineinfo )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    LINEINFO    *rglineinfo = *pplineinfo;

    Assert( NULL != pfEmptyPage );
    *pfEmptyPage = fTrue;

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
        pmerge->mergetype = mergetypeNone;
    }
    else if (   pmergePath->csr.Cpage().PgnoPrev() != pgnoNull &&
                pmergePath->csr.Cpage().PgnoNext() == pgnoNull &&
                pmergePath->pmergePathParent->csr.Cpage().Clines() == 1 )
    {
        pmerge->mergetype = mergetypeNone;
    }
    else if (   !fRightMerges
                && 0 == pmergePath->pmergePathParent->csr.ILine() )
    {
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
        pmerge->mergetype = mergetypeNone;
    }
    else
    {
        pmerge->mergetype = fRightMerges ? mergetypeFullRight : mergetypeFullLeft;
    }

    return err;
}


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

            pmerge->csrLeft.ReleasePage();

            Call( pcsr->ErrGetRIWPage( pfucb->ppib,
                                       pfucb->ifmp,
                                       pgnoCurr ) );

            Assert( pcsr->Dbtime() >= dbtimeCurr );
            if ( pcsr->Dbtime() == dbtimeCurr )
            {
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
                && !pcsr->Cpage().FEmptyPage() )
            {
                cLatchFailures++;
                goto Start;
            }
            else
            {
                Error( ErrERRCheck( errBTMergeNotSynchronous ) );
            }
        }

        Call( pcsr->ErrGetRIWPage( pfucb->ppib,
                                   pfucb->ifmp,
                                   pgnoCurr ) );
    }
    else
    {
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


LOCAL BOOL FBTICheckMergeableOneNode(
    const FUCB * const pfucb,
    CSR * const pcsrDest,
    MERGE * const pmerge,
    const INT iline,
    const ULONG cbDensityFree,
    const BOOL fRightMerges )
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
        return fTrue;
    }

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

    Assert( pcsrDest->Pgno() != pgnoNull );
    const INT   cbSizeTotal     = pmerge->cbSizeTotal + plineinfo->cbSize;
    const INT   cbSizeMaxTotal  = pmerge->cbSizeMaxTotal + plineinfo->cbSizeMax;
    const INT   cbReq           = ( cbSizeMaxTotal - cbSavings ) + cbDensityFree;

    Assert( cbReq >= 0 );
    if ( !FNDFreePageSpace( pfucb, pcsrDest, cbReq ) )
    {
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

    pmerge->cbSavings       = cbSavings;
    pmerge->cbSizeTotal     = cbSizeTotal;
    pmerge->cbSizeMaxTotal  = cbSizeMaxTotal;

    return fTrue;
}

LOCAL VOID BTICheckMergeable(
    FUCB * const pfucb,
    MERGEPATH * const pmergePath,
    const BOOL fRightMerges )

{
    MERGE * const pmerge = pmergePath->pmerge;
    Assert( NULL != pmerge );
    
    if( fRightMerges )
    {
        Assert( mergetypeFullRight == pmerge->mergetype );
        if ( pmergePath->csr.Cpage().PgnoNext() == pgnoNull )
        {
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
            Assert( latchNone == pmerge->csrLeft.Latch() );
            pmerge->mergetype = mergetypeNone;
            return;
        }
    }
    
    CSR * const pcsrDest = fRightMerges ? &(pmerge->csrRight) : &(pmerge->csrLeft);
    Assert( pcsrDest->FLatched() );

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


BOOL FBTIFreePageSpaceForMerge(
        const FUCB *pfucb,
        CSR * const pcsr,
        const ULONG cbReq )
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
    
BOOL FBTISPCCheckMergeable( FUCB *pfucb, CSR *pcsrDest, LINEINFO *rglineinfo )
{
    const INT   clines = Pcsr( pfucb )->Cpage().Clines();

    Assert( Pcsr( pfucb )->FLatched() );
    Assert( pcsrDest->FLatched() );
    Assert( pcsrDest->Cpage().FLeafPage() );
    Assert( Pcsr( pfucb )->Cpage().FLeafPage() );

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
            continue;
        }

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

        cbSizeTotal     += plineinfo->cbSize;
        cbSizeMaxTotal  += plineinfo->cbSizeMax;
    }

    Assert( pcsrDest->Pgno() != pgnoNull );
    const INT       cbReq       = ( cbSizeMaxTotal - cbSavings )
                                    + ( Pcsr( pfucb )->Cpage().FLeafPage() ? CbBTIFreeDensity( pfucb ) : 0 );
    const BOOL      fMergeable  = FBTIFreePageSpaceForMerge( pfucb, pcsrDest, cbReq );

    return fMergeable;
}

INLINE BOOL FBTINullKeyedLastNode( FUCB *pfucb, MERGEPATH *pmergePath )
{
    CSR     *pcsr = &pmergePath->csr;

    Assert( !pcsr->Cpage().FLeafPage() );

    pcsr->SetILine( pmergePath->iLine );
    NDGet( pfucb, pcsr );
    Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );

    Assert( pcsr->Cpage().Clines() - 1 == pmergePath->iLine
        || !pfucb->kdfCurr.key.FNull() );

    return pfucb->kdfCurr.key.FNull();
}

LOCAL ERR ErrBTISelectMergeInternalPages( FUCB * const pfucb, MERGEPATH * const pmergePathLeaf )
{
    ERR         err;
    MERGEPATH   * pmergePath        = pmergePathLeaf->pmergePathParent;
    MERGE       * const pmergeLeaf  = pmergePathLeaf->pmerge;
    const BOOL  fLeftSibling        = pgnoNull != pmergeLeaf->csrLeft.Pgno();
    const BOOL  fRightSibling       = pgnoNull != pmergeLeaf->csrRight.Pgno();
    BOOL        fKeyChange          = fFalse;

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

                Assert( ilineMerge > 0 );
                Assert( !pmergeLeaf->fAllocParentSep );
                CallR( ErrBTIComputeSeparatorKey( pfucb,
                                                  *pkdfPrev,
                                                  *pkdfMerge,
                                                  &pmergeLeaf->kdfParentSep.key ) );

                pmergeLeaf->kdfParentSep.data.SetCb( sizeof( PGNO ) );
                pmergeLeaf->fAllocParentSep = fTrue;
            }
            
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

            const BOOL fParentOfLeaf = ( NULL == pmergePathChild->pmergePathChild );
            Assert( fParentOfLeaf );
            
            const INT ilineMerge  = pmergeLeaf->ilineMerge;
            Assert( ilineMerge < ( pmergeLeaf->clines - 1 ) );
            
            const KEYDATAFLAGS * const pkdfMerge    = &pmergeLeaf->rglineinfo[ilineMerge].kdf;
            const KEYDATAFLAGS * const pkdfNext     = &pmergeLeaf->rglineinfo[ilineMerge + 1].kdf;

            Assert( ilineMerge >= 0 );
            Assert( !pmergeLeaf->fAllocParentSep );
            CallR( ErrBTIComputeSeparatorKey( pfucb,
                                              *pkdfMerge,
                                              *pkdfNext,
                                              &pmergeLeaf->kdfParentSep.key ) );
            pmergeLeaf->kdfParentSep.data.SetCb( sizeof( PGNO ) );
            pmergeLeaf->fAllocParentSep = fTrue;


            Assert( pmergePath->iLine > 0 );
            if ( FBTIOverflowOnReplacingKey( pfucb, &pmergePath->csr, pmergePath->iLine-1, pmergeLeaf->kdfParentSep ) )
            {
                goto Overflow;
            }

            pmergePath->fKeyChange = fTrue;

            break;
        }

        if ( mergetypeFullLeft == pmergeLeaf->mergetype )
        {
            const MERGEPATH * const pmergePathChild = pmergePath->pmergePathChild;
            Assert( NULL != pmergePathChild );

            const BOOL fParentOfLeaf = ( NULL == pmergePathChild->pmergePathChild );
            Assert( fParentOfLeaf );
            

            Assert( pmergePath->iLine > 0 );

            pmergePath->fKeyChange = fTrue;
            pmergePath->fDeleteNode = fTrue;

            break;
        }

        if ( pmergePath->csr.Cpage().Clines() == 1 )
        {
            Assert( pmergePath->csr.ILine() == 0 );

            if ( pmergePath->csr.Cpage().FRootPage() )
            {
                Assert( fFalse );
                Assert( mergetypeEmptyPage == pmergeLeaf->mergetype );

                goto Overflow;
            }

            if ( pmergePath->pmergePathChild->fEmptyPage )
            {
                Assert( pmergePath->pmergePathChild->csr.Cpage().FLeafPage()
                        || ( 0 == pmergePath->pmergePathChild->csr.ILine()
                            && 1 == pmergePath->pmergePathChild->csr.Cpage().Clines() ) );
                pmergePath->fEmptyPage = fTrue;
            }
            else
            {
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
            Assert( fLeftSibling );

            if ( !fKeyChange )
            {
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



LOCAL BOOL FBTIOverflowOnReplacingKey(  FUCB                    *pfucb,
                                        MERGEPATH               *pmergePath,
                                        const KEYDATAFLAGS&     kdfSep )
{
    return FBTIOverflowOnReplacingKey( pfucb, &pmergePath->csr, pmergePath->iLine, kdfSep );
}

LOCAL BOOL FBTIOverflowOnReplacingKey(  FUCB                    *pfucb,
                                        CSR * const             pcsr,
                                        const INT               iLine,
                                        const KEYDATAFLAGS&     kdfSep )
{
    Assert( !kdfSep.key.FNull() );
    Assert( !pcsr->Cpage().FLeafPage() );

    ULONG   cbReq = CbBTICbReq( pfucb, pcsr, kdfSep );

    pcsr->SetILine( iLine );
    NDGet( pfucb, pcsr );
    Assert( !FNDVersion( pfucb->kdfCurr ) );

    ULONG   cbSizeCurr = CbNDNodeSizeCompressed( pfucb->kdfCurr );

    if ( cbReq > cbSizeCurr )
    {
        const BOOL  fOverflow = FBTISplit( pfucb, pcsr, cbReq - cbSizeCurr );

        return fOverflow;
    }

    return fFalse;
}


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

    pfucb->kdfCurr.key.CopyIntoBuffer( pmergeLeaf->kdfParentSep.key.suffix.Pv() );
    pmergeLeaf->kdfParentSep.key.suffix.SetCb( pfucb->kdfCurr.key.Cb() );

    Assert( !pmergeLeaf->fAllocParentSep );
    Assert( !pmergeLeaf->kdfParentSep.key.FNull() );
    pmergeLeaf->fAllocParentSep = fTrue;

    pmergeLeaf->kdfParentSep.data.SetCb( sizeof( PGNO ) );
    return JET_errSuccess;
}


VOID BTIReleaseMergePaths( MERGEPATH *pmergePathTip )
{
    MERGEPATH *pmergePath = pmergePathTip;

    for ( ; pmergePath != NULL; )
    {
        MERGEPATH *pmergePathParent = pmergePath->pmergePathParent;

        delete pmergePath;
        pmergePath = pmergePathParent;
    }
}

ERR ErrBTIMergeOrEmptyPage( FUCB *pfucb, MERGEPATH *pmergePathLeaf )
{
    ERR         err = JET_errSuccess;
    MERGE       *pmerge = pmergePathLeaf->pmerge;
    LGPOS       lgpos;

    Assert( NULL != pmerge );
    Assert( mergetypeNone != pmerge->mergetype );

    CallR( ErrBTIMergeUpgradeLatches( pfucb->ifmp, pmergePathLeaf ) );

    err = ErrLGMerge( pfucb, pmergePathLeaf, &lgpos );

    if ( JET_errSuccess > err )
    {
        BTIMergeRevertDbtime( pmergePathLeaf );
    }
    CallR ( err );

    BTIMergeSetLgpos( pmergePathLeaf, lgpos );

    BTIPerformMerge( pfucb, pmergePathLeaf );

    BTICheckMerge( pfucb, pmergePathLeaf );

    BTIMergeShrinkPages( pmergePathLeaf );

    BTIMergeReleaseLatches( pmergePathLeaf );

    BTIReleaseEmptyPages( pfucb, pmergePathLeaf );

    return err;
}


LOCAL VOID BTICheckPagePointer(
    __in const CSR& csr,
    __in const PGNO pgno )
{
    Assert( csr.FLatched() );
    Assert( csr.Cpage().FInvisibleSons() );

    KEYDATAFLAGS kdf;
    NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdf );

    Assert( sizeof( PGNO ) == kdf.data.Cb() );
    const PGNO pgnoCurr = *reinterpret_cast<const UnalignedLittleEndian<ULONG> *>( kdf.data.Pv() );
    Assert( pgno == pgnoCurr );
}

LOCAL VOID BTIUpdatePagePointer(
    __in CSR * const pcsr,
    __in const PGNO pgnoNew )
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

LOCAL VOID BTIMovePageCopyNextBookmark(
            __in        const FUCB * const pfucb,
            __in        const MERGEPATH * const pmergePath,
            __inout     BOOKMARK * const pbmNext )
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

        bm.key.CopyIntoBuffer( pbmNext->key.suffix.Pv(), bm.key.Cb() );
        pbmNext->key.suffix.SetCb( bm.key.Cb() );

        pbmNext->data.SetPv( reinterpret_cast<BYTE *>( pbmNext->key.suffix.Pv() ) + pbmNext->key.Cb() );
        bm.data.CopyInto( pbmNext->data );

        Assert( pbmNext->key.prefix.FNull() );
        Assert( !pbmNext->key.suffix.FNull() );
        Assert( !pbmNext->key.FNull() );
    }
}

LOCAL VOID BTIMergeSetPcsrRoot(
    __in FUCB * const pfucb,
    __in MERGEPATH * const pmergePathTip )
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

LOCAL ERR ErrBTIPageMoveAllocatePage(
    __in        FUCB * const        pfucb,
    __in        MERGEPATH * const   pmergePath,
    __in        ULONG               fSPAllocFlags,
    __in        const DIRFLAG       dirflag )
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

    Ptls()->threadstats.cPageUpdateAllocated++;

HandleError:
    pfucb->pcsrRoot = NULL;
    return err;
}

LOCAL VOID BTICheckPageMove(
    const CSR& csrParent,
    const CSR& csrSrc,
    const CSR& csrDest,
    const CSR& csrLeft,
    const CSR& csrRight,
    const BOOL fBeforeMove )
{
    const BOOL fLeafPage    = csrSrc.Cpage().FLeafPage();
    const OBJID objidFDP    = csrSrc.Cpage().ObjidFDP();

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

    Assert( !fLeafPage || csrParent.Cpage().FParentOfLeaf() );
    Assert( objidFDP == csrParent.Cpage().ObjidFDP() );
    BTICheckPagePointer( csrParent, pcsrCurrent->Pgno() );

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

VOID BTPerformPageMove( __in MERGEPATH * const pmergePath )
{
    Assert( pmergePath );
    Assert( NULL != pmergePath->pmerge );
    Assert( mergetypePageMove == pmergePath->pmerge->mergetype );
    Assert( NULL != pmergePath->pmergePathParent );

    const PGNO pgnoOld = pmergePath->csr.Pgno();
    const PGNO pgnoNew = pmergePath->pmerge->csrNew.Pgno();

    if( FBTIUpdatablePage( pmergePath->pmerge->csrNew ) )
    {
        Assert( pmergePath->csr.Cpage().CbPage() == pmergePath->pmerge->csrNew.Cpage().CbPage() );
        UtilMemCpy(
            pmergePath->pmerge->csrNew.Cpage().PvBuffer(),
            pmergePath->csr.Cpage().PvBuffer(),
            pmergePath->csr.Cpage().CbPage() );
        pmergePath->pmerge->csrNew.Cpage().SetPgno( pgnoNew );
        pmergePath->pmerge->csrNew.FinalizePreInitPage();
    }

    if( FBTIUpdatablePage( pmergePath->csr ) )
    {
        NDEmptyPage( &(pmergePath->csr) );
    }

    if( FBTIUpdatablePage( pmergePath->pmergePathParent->csr ) )
    {
        BTICheckPagePointer( pmergePath->pmergePathParent->csr, pgnoOld );
        BTIUpdatePagePointer( &(pmergePath->pmergePathParent->csr), pgnoNew );
    }

    if( pgnoNull != pmergePath->pmerge->csrLeft.Pgno() && FBTIUpdatablePage( pmergePath->pmerge->csrLeft ) )
    {
        Assert( pmergePath->pmerge->csrLeft.Cpage().PgnoNext() == pgnoOld );
        pmergePath->pmerge->csrLeft.Cpage().SetPgnoNext( pgnoNew );
    }

    if( pgnoNull != pmergePath->pmerge->csrRight.Pgno() && FBTIUpdatablePage( pmergePath->pmerge->csrRight ) )
    {
        Assert( pmergePath->pmerge->csrRight.Cpage().PgnoPrev() == pgnoOld );
        pmergePath->pmerge->csrRight.Cpage().SetPgnoPrev( pgnoNew );
    }
}

LOCAL ERR ErrBTIPageMove(
    __in FUCB * const       pfucb,
    __in MERGEPATH * const  pmergePath,
    __in const DIRFLAG      dirflag,
    __in const ULONG        fSPAllocFlags )
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

    BTICheckPageMove(
        pmergePath->pmergePathParent->csr,
        pmergePath->csr,
        pmergePath->pmerge->csrNew,
        pmergePath->pmerge->csrLeft,
        pmergePath->pmerge->csrRight,
        fTrue );

    BTPerformPageMove( pmergePath );

    BTICheckPageMove(
        pmergePath->pmergePathParent->csr,
        pmergePath->csr,
        pmergePath->pmerge->csrNew,
        pmergePath->pmerge->csrLeft,
        pmergePath->pmerge->csrRight,
        fFalse );

#ifdef TEST_PAGEMOVE_RECOVERY
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

ERR ErrBTPageMove(
    __in FUCB * const pfucb,
    __in const BOOKMARK& bm,
    __in const PGNO pgnoSource,
    __in const BOOL fLeafPage,
    __in const ULONG fSPAllocFlags,
    __inout BOOKMARK * const pbmNext )
{
    Assert( pfucb );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !bm.key.FNull() || !fLeafPage );
    Assert( fLeafPage || ( pgnoSource != pgnoNull ) );
    Assert( ( pbmNext == NULL ) || fLeafPage );

    ERR err = JET_errSuccess;
    PIBTraceContextScope tcScope = TcBTICreateCtxScope( pfucb, iorsBTMerge );
    MERGEPATH * pmergePath = NULL;

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
    pmergePath->fEmptyPage                      = fTrue;
    pmergePath->pmergePathParent->fKeyChange    = fTrue;
    
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

VOID BTIPerformMerge( FUCB *pfucb, MERGEPATH *pmergePathLeaf )
{
    auto tc = TcCurr();

    MERGEPATH   *pmergePath;

    for ( pmergePath = pmergePathLeaf;
          pmergePath->pmergePathParent != NULL;
          pmergePath = pmergePath->pmergePathParent )
    {
    }

    for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathChild )
    {
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
        }
    }
}

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

        BTIMergeDeleteFlagDeletedNodes( pfucb, pmergePath );

        if ( mergetypePartialRight != mergetype && mergetypePartialLeft != mergetype )
        {
            BTIMergeFixSiblings( PinstFromIfmp( pfucb->ifmp ), pmergePath );
        }
    }

    else if ( !FBTIUpdatablePage( *pcsr ) )
    {
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() );
    }

    else if ( mergetypeFullLeft == pmergeLeaf->mergetype
            && pmergePath->fDeleteNode
            && pmergePath->fKeyChange )
    {

        Assert( pmergePath->csr.Cpage().FParentOfLeaf() );
        Assert( pmergePath->pmergePathChild );
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() || pmergePath->pmergePathChild->csr.Cpage().FLeafPage() );
        Assert( NULL == pmergePath->pmergePathChild->pmergePathChild );
        Assert( pmergeLeaf == pmergePath->pmergePathChild->pmerge );

        Assert( pmergePath->iLine > 0 );
        pmergePath->csr.SetILine( pmergePath->iLine );

        const PGNO pgnoChild = pmergePath->pmergePathChild->csr.Pgno();
        const PGNO pgnoLeft = pmergeLeaf->csrLeft.Pgno();

        Assert( pgnoNull != pgnoChild );
        Assert( pgnoNull != pgnoLeft );
        
        BTICheckPagePointer( pmergePath->csr, pgnoChild );
        BTIUpdatePagePointer( &(pmergePath->csr), pgnoLeft );
        BTICheckPagePointer( pmergePath->csr, pgnoLeft );

        pmergePath->csr.SetILine( pmergePath->iLine - 1 );
        NDDelete( &(pmergePath->csr) );
    }
    
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

    else if ( pmergePath->fKeyChange )
    {
        Assert( mergetypeFullLeft != pmergeLeaf->mergetype );
        Assert( !pmergeLeaf->kdfParentSep.key.FNull() );


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
        NDEmptyPage( &(pmergePath->csr) );
    }

    return;
}


VOID BTIChangeKeyOfPagePointer( FUCB *pfucb, CSR *pcsr, const KEY& key )
{
    Assert( !pcsr->Cpage().FLeafPage() );

    NDGet( pfucb, pcsr );
    Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
    Assert( !pfucb->kdfCurr.key.FNull() );

    LittleEndian<PGNO>  le_pgno = *((UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv() );
    Assert( le_pgno != pgnoNull );

    NDDelete( pcsr );

    KEYDATAFLAGS    kdfInsert;

    kdfInsert.fFlags    = 0;
    kdfInsert.data.SetCb( sizeof( PGNO ) );
    kdfInsert.data.SetPv( &le_pgno );
    kdfInsert.key       = key;

    BTIComputePrefixAndInsert( pfucb, pcsr, kdfInsert );
}


VOID BTIMergeReleaseUnneededPages( MERGEPATH *pmergePathTip )
{
    MERGEPATH   *pmergePath;

    for ( pmergePath = pmergePathTip;
          pmergePath->pmergePathParent != NULL;
          pmergePath = pmergePath->pmergePathParent )
    {
    }

    Assert( NULL == pmergePath->pmergePathParent );
    for ( ; NULL != pmergePath;  )
    {

        if ( !pmergePath->fKeyChange &&
             !pmergePath->fEmptyPage &&
             !pmergePath->fDeleteNode &&
             pmergePath->pmergePathChild != NULL )
        {
            Assert( NULL == pmergePath->pmergePathParent );

            MERGEPATH *pmergePathT = pmergePath;

            Assert( !pmergePath->fDeleteNode );
            Assert( !pmergePath->fKeyChange );
            Assert( !pmergePath->fEmptyPage );
            Assert( !pmergePath->csr.Cpage().FLeafPage() );
            Assert( NULL != pmergePath->pmergePathChild );
            if ( mergetypeNone != pmergePathTip->pmerge->mergetype )
            {
                Assert( pmergePath->pmergePathChild->pmergePathChild != NULL );
                Assert( !pmergePath->pmergePathChild->csr.Cpage().FLeafPage() );
            }

            Assert( pmergePath->csr.Latch() == latchRIW || pmergePath->csr.Latch() == latchWrite );
            Expected( pmergePath->csr.Latch() != latchWrite || pmergePath->csr.Cpage().FRootPage() );

            pmergePath = pmergePath->pmergePathChild;

            delete pmergePathT;
        }
        else
        {
            pmergePath = pmergePath->pmergePathChild;
        }
    }
}


LOCAL ERR ErrBTIMergeUpgradeLatches( const IFMP ifmp, MERGEPATH * const pmergePathTip )
{
    ERR             err;
    MERGEPATH       *pmergePath;
    const DBTIME    dbtimeMerge     = g_rgfmp[ifmp].DbtimeIncrementAndGet();

    Assert( dbtimeMerge > 1 );
    Assert( PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() != fRecoveringRedo );

    for ( pmergePath = pmergePathTip;
          pmergePath->pmergePathParent != NULL;
          pmergePath = pmergePath->pmergePathParent )
    {
    }

    Assert( NULL == pmergePath->pmergePathParent );
    for ( ; NULL != pmergePath;  )
    {
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
                Assert( NULL != pmerge );
                Assert( mergetypeNone != pmerge->mergetype );

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


VOID BTIReleaseEmptyPages( FUCB *pfucb, MERGEPATH *pmergePathTip )
{
    MERGEPATH   *pmergePath = pmergePathTip;
    const BOOL  fAvailExt   = FFUCBAvailExt( pfucb );
    const BOOL  fOwnExt     = FFUCBOwnExt( pfucb );

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
        Assert( !pmergePath->csr.FLatched() );
        if ( pmergePath->fEmptyPage )
        {

            Assert( !FFUCBSpace( pfucb ) );

            Assert( pmergePath->csr.Pgno() != PgnoRoot( pfucb ) );
            const ERR   errFreeExt  = ErrSPFreeExt( pfucb, pmergePath->csr.Pgno(), 1, "FreeEmptyPages" );
#ifdef DEBUG
            if ( !FSPExpectedError( errFreeExt ) )
            {
                CallS( errFreeExt );
            }
#endif

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
                Assert( !plineinfo->fVerActive );
                if ( !plineinfo->fVerActive )
                {
                    BOOKMARK bm;

                    NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );

                    VERNullifyInactiveVersionsOnBM( pfucb, bm );
                }
            }
        }

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


LOCAL VOID BTIMergeMoveNodes( FUCB * const pfucb, MERGEPATH * const pmergePath )
    
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

    INT ilineSrcMin;
    INT ilineSrcMax;
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
    if ( PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() )
    {
        pcsrDest->Cpage().SetCbUncommittedFree( 0 );
    }
#endif

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
            Assert( !plineinfo->fVerActive );
            if ( !plineinfo->fVerActive )
            {
                continue;
            }
        }

#ifdef DEBUG
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

        Assert( !FNDDeleted( plineinfo->kdf ) );
        Assert( pcsrDest->ILine() == ilineDest );
        NDInsert( pfucb, pcsrDest, &plineinfo->kdf, plineinfo->cbPrefix );
    }

    Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() ||
            pcsrDest->Cpage().CbUncommittedFree() +
                pmerge->cbSizeMaxTotal -
                pmerge->cbSizeTotal  ==
                pmerge->cbUncFreeDest );
    pcsrDest->Cpage().SetCbUncommittedFree( pmerge->cbUncFreeDest );
}


INLINE VOID BTICheckMerge( FUCB *pfucb, MERGEPATH *pmergePathLeaf )
{
#ifdef DEBUG
    MERGEPATH   *pmergePath;

    BTICheckMergeLeaf( pfucb, pmergePathLeaf );

    for ( pmergePath = pmergePathLeaf->pmergePathParent;
          pmergePath != NULL;
          pmergePath = pmergePath->pmergePathParent )
    {
        BTICheckMergeInternal( pfucb, pmergePath, pmergePathLeaf->pmerge );
    }
#endif
}

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
#endif
}


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
        Assert( fFalse );
    }

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

    if ( pmergePath->pmergePathParent == NULL )
    {
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
#endif
}

#ifndef MINIMAL_FUNCTIONALITY


class CBTAcrossQueue {

    const PGNO      m_pgnoFDP;

    CStupidQueue *  m_pQ;
    PGNO            m_rgpgnoLevelLeftStart[32];
    ULONG           m_iNextLevel;

#ifdef DEBUG
    PGNO            m_pgnoCurr;
#endif

    ULONG           m_cPages;

    CStupidQueue::ERR ErrEnqueuePage( __in const PGNO pgno )
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
            Assert( itag == 0 );
            return JET_errSuccess;
        }

        Assert( pkdf->key.Cb() == ( pkdf->key.prefix.Cb() + pkdf->key.suffix.Cb() ) );

        PGNO pgnoLeaf;
        Assert( pkdf->data.Cb() == sizeof(PGNO) );
        pgnoLeaf = *((UnalignedLittleEndian<ULONG>*)pkdf->data.Pv());
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

        return err;
    }

public:

    CBTAcrossQueue( __in const PGNO pgnoFDP ) :
        m_pgnoFDP( pgnoFDP )
    {
        memset( m_rgpgnoLevelLeftStart, 0, sizeof(m_rgpgnoLevelLeftStart) );
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

    CStupidQueue::ERR ErrDequeuePage( __out PGNO * ppgnoNext, __out ULONG * piLevel )
    {
        CStupidQueue::ERR errQueue = CStupidQueue::ERR::errSuccess;

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

        Assert( ppgnoNext );
        Assert( piLevel );
        PGNO pgnoNext;
        errQueue = m_pQ->ErrDequeue( &pgnoNext );
        if ( errQueue == CStupidQueue::ERR::errSuccess )
        {
            if ( m_rgpgnoLevelLeftStart[m_iNextLevel] == pgnoNext )
            {
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

    ERR ErrEnqueueNextLevel( __in const PGNO pgno, __in const CPAGE * const pcpage )
    {
        ERR err;

        Assert( m_pgnoCurr == pgno );

        if ( pcpage->FLeafPage() )
        {
            return JET_errSuccess;
        }
        

        err = pcpage->ErrEnumTags( CBTAcrossQueue::EnumChildPagePtrNodes, (void*)this );
        return err;
    }


};


ERR ErrBTUTLAcross(
    IFMP                    ifmp,
    const PGNO              pgnoFDP,
    const ULONG             fVisitFlags,
    PFNVISITPAGE            pfnErrVisitPage,
    void *                  pvVisitPageCtx,
    CPAGE::PFNVISITNODE *   rgpfnzErrVisitNode,
    void **                 rgpvzVisitNodeCtx
    )
{
    ERR                     err         = JET_errSuccess;
    IFileAPI *              pfapi = g_rgfmp[ifmp].Pfapi();

    CBTAcrossQueue *    pBreadthFirst = NULL;

    PGNO                pgnoCurr = 0x0;
    ULONG               iCurrLevel = 0;

    VOID *              pvPageBuffer = NULL;


    Alloc( pBreadthFirst = new CBTAcrossQueue( pgnoFDP ) );

    Assert( g_rgfmp[ ifmp ].CbPage() >= g_cbPageMin );
    Alloc( pvPageBuffer = PvOSMemoryPageAlloc( g_rgfmp[ ifmp ].CbPage(), NULL ) );
    memset( pvPageBuffer, 0x42, g_rgfmp[ ifmp ].CbPage() );

    CStupidQueue::ERR errQueue = CStupidQueue::ERR::errSuccess;
    while ( CStupidQueue::ERR::errSuccess == ( errQueue = pBreadthFirst->ErrDequeuePage( &pgnoCurr, &iCurrLevel ) ) )
    {
        Assert( pgnoCurr );

        TraceContextScope tcUtil( iorpDirectAccessUtil );
        Call( pfapi->ErrIORead( *tcUtil, OffsetOfPgno( pgnoCurr ), g_rgfmp[ ifmp ].CbPage(), (BYTE* const)pvPageBuffer, qosIONormal ) );
        CPAGE   cpage;
        cpage.LoadPage( ifmp, pgnoCurr, pvPageBuffer, g_rgfmp[ifmp].CbPage() );

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

        if ( !cpage.FLeafPage() )
        {

            if ( cpage.FParentOfLeaf() && !(fVisitFlags & CPAGE::fPageLeaf) )
            {
                continue;
            }

            Call( pBreadthFirst->ErrEnqueueNextLevel( pgnoCurr, &cpage ) );
        }

    }

    if ( CStupidQueue::ERR::wrnEmptyQueue == errQueue )
    {
        err = JET_errSuccess;
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

HandleError:

    OSMemoryPageFree( pvPageBuffer );
    delete pBreadthFirst;

    return err;
}

#endif

extern USHORT g_rgcbExternalHeaderSize[noderfMax];

ERR ErrBTGetRootField( _Inout_ FUCB* const pfucb, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderf, _In_ const LATCH latch )
{
    ERR err = JET_errSuccess;

    CallR( ErrBTIGotoRoot( pfucb, latch ) );
    NDGetExternalHeader( pfucb, Pcsr( pfucb ), noderf );

#ifdef DEBUG
    if ( noderf != noderfWhole )
    {
        Assert( noderfMax > noderf );
        Assert( (INT)g_rgcbExternalHeaderSize[noderf] == pfucb->kdfCurr.data.Cb() ||
            0 == pfucb->kdfCurr.data.Cb() );
    }
#endif

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
#endif

    Assert( Pcsr( pfucb )->FLatched() );
    if ( Pcsr( pfucb )->Latch() != latchWrite )
    {
        Call( Pcsr( pfucb )->ErrUpgrade() );
    }
    Assert( Pcsr( pfucb )->Latch() == latchWrite );

    NDGetExternalHeader( pfucb, Pcsr( pfucb ), noderf );

    cbReq = dataRootField.Cb() > pfucb->kdfCurr.data.Cb() ?  ( dataRootField.Cb() - pfucb->kdfCurr.data.Cb() + 1 ) : 0;
    if ( FBTISplit( pfucb, Pcsr( pfucb ), cbReq ) )
    {
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

