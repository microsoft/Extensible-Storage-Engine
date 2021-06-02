// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "errdata.hxx"
#include "_bt.hxx"

#include "PageSizeClean.hxx"

// Tracing.
//

LOCAL ERR ErrSHKIShrinkEofTracingBegin( _In_ IFileSystemAPI * pfsapi, _In_ JET_PCWSTR wszDatabase, _Out_ CPRINTF** ppcprintfShrinkTraceRaw )
{
    ERR err = JET_errSuccess;

    Call( ErrBeginDatabaseIncReseedTracing( pfsapi, wszDatabase, ppcprintfShrinkTraceRaw ) );

    (**ppcprintfShrinkTraceRaw)( "Beginning shrink pass.\r\n" );

HandleError:
    return err;
}

VOID SHKIShrinkEofTracingEnd( _Out_ CPRINTF** ppcprintfShrinkTraceRaw )
{
    if ( *ppcprintfShrinkTraceRaw )
    {
        (**ppcprintfShrinkTraceRaw)( "Ending shrink pass.\r\n" );
    }

    EndDatabaseIncReseedTracing( ppcprintfShrinkTraceRaw );
}


// Top-level shrink functions.
//

LOCAL ERR ErrSHKIMoveLastExtent(
    _In_ PIB* ppib,
    _In_ const IFMP ifmp,
    _In_ const HRT hrtStarted,
    _In_ CPRINTF* const pcprintfShrinkTraceRaw,
    _Inout_ CPG* const pcpgMoved,
    _Inout_ CPG* const pcpgShelved,
    _Inout_ CPG* const pcpgUnleaked,
    _Inout_ HRT* const pdhrtPageCategorization,
    _Inout_ HRT* const pdhrtDataMove,
    _Out_ ShrinkDoneReason* const psdr,
    _Out_ PGNO* const ppgnoLastProcessed,
    _Out_ PGNO* const pgnoFirstFromLastExtentMoved,
    _Out_ SpaceCategoryFlags* const pspcatfLastProcessed )
{
    // This is used in the OBJID lookup logic as follows:
    //  - cptNormal: regular categorization pass. The move pass starts in this mode.
    //  - cptIndeterminateLookup: turned on when we find a page which can't be categorized with full
    //    categorization on (i.e., an spcatfIndeterminate page). The goal is to look for a valid hint
    //    so that we can go back and re-evaluate indeterminate pages and potentially be able to categorize
    //    them definitively. It gets turned off and we switch to cptIndeterminateRetry if we can find a valid
    //    OBJID hint.
    //  - cptIndeterminateRetry: turned on when a valid OBJID hint has been found and we are going back to re-categorize
    //    pages. It gets turned off and we return to cptNormal once we reach the page at which we went back and started
    //    the retry pass.
    //
    //  This might sound like an optimization, but it is required to ensure that once we are done with this loop,
    //  ErrSPShrinkTruncateLastExtent() will then be able to do its job in finding a contiguous range of available
    //  or shelved extents matching the owned extent we're trying to remove and truncate.
    //
    //  Example: the extent we're trying to truncate is 100-110 and pgno 100 belongs to OBJID 43, but it's a blank page.
    //  We first categorize pgno 100 as spcatfIndeterminate, add it to the shelved page list and move on. Then we
    //  successfully categorize pgnos 101-110 as belonging to OBJID 43 and moved them all out of the way. ErrSPShrinkTruncateLastExtent()
    //  won't be able to truncate the extent because it wasn't migrated up to the DB root (pgno 100 is pinning it).
    //  With the lookup/retry solution described here, we would have gone back and categorized pgno 100 under OBJID 43.
    //
    //  Therefore, we predict that an spcatfIndeterminate page will be always either 1) leaked from the root, in
    //  which case the retry will fail and adding it to the shelved list will allow it to be part of the range to
    //  be truncated or 2) be available space in a given OBJID. In case 2), if the retry succeeds, the page will
    //  then be 2.a) categorized and move appropriately; if it fails it will be 2.b) part of a chain of
    //  spcatfIndeterminate pages which will match the entire space owned by the OBJID and will me sparsified at the
    //  root level.
    //
    enum CatPassType
    {
        cptNormal,
        cptIndeterminateLookup,
        cptIndeterminateRetry
    };

    ERR err = JET_errSuccess;
    BOOL fInTransaction = fFalse, fMovedPage = fFalse;
    FMP* const pfmp = g_rgfmp + ifmp;
    EXTENTINFO eiLastOE;
    const LONG dtickQuota = pfmp->DtickShrinkDatabaseTimeQuota();
    SpaceCatCtx* pSpCatCtx = NULL;
    BFLatch bfl;
    BOOL fPageLatched = fFalse;
    BOOL fPageCategorization = fFalse;
    BOOL fDataMove = fFalse;
    HRT hrtPageCategorizationStart = 0;
    HRT hrtDataMoveStart = 0;
    CPG cpgMoved = 0;
    CPG cpgShelved = 0;
    CPG cpgUnleaked = 0;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortDbShrink );

    Assert( pfmp->FShrinkIsActive() );
    Assert( pfmp->FExclusiveBySession( ppib ) );

    *psdr = sdrNone;
    *pgnoFirstFromLastExtentMoved = pgnoNull;

    // Get last owned extent info.
    Call( ErrSPGetLastExtent( ppib, ifmp, &eiLastOE ) );
    *pgnoFirstFromLastExtentMoved = eiLastOE.PgnoFirst();

    // Move pages one by one.
    //

    PGNO pgnoCurrent = eiLastOE.PgnoFirst();

    // The first extent is never supposed to be moved, it's the bootstrap of the database.
    if ( pgnoCurrent == pgnoSystemRoot )
    {
        *psdr = sdrNoLowAvailSpace;
        *ppgnoLastProcessed = pgnoSystemRoot;
        *pspcatfLastProcessed = spcatfRoot;
        goto HandleError;
    }

    // Set shrink threshold to signal SPACE to not allocate space in the region undergoing shrink.
    pfmp->SetPgnoShrinkTarget( pgnoCurrent - 1 );
    Assert( pgnoCurrent != pgnoNull );

    // Just in case, but we don't expect version store and DB tasks to be running at this point.
    Call( PverFromPpib( ppib )->ErrVERRCEClean( ifmp ) );
    if ( err != JET_errSuccess )
    {
        AssertTrack( fFalse, OSFormat( "ShrinkMoveVerCleanWrn:%d", err ) );
        *psdr = sdrUnexpected;
        goto HandleError;
    }
    pfmp->WaitForTasksToComplete();

    BOOL fRolledLogOnScanCheck = fFalse;
    const PGNO pgnoLast = eiLastOE.PgnoLast();
    const CPG cpgPrereadMax = LFunctionalMax( 1, (CPG)( UlParam( JET_paramMaxCoalesceReadSize ) / pfmp->CbPage() ) );
    PGNO pgnoPrereadWaypoint = pgnoCurrent, pgnoPrereadNext = pgnoCurrent;
    PGNO pgnoIndeterminateFirst = pgnoNull;
    PGNO pgnoPostIndeterminateRetry = pgnoNull;
    OBJID objidLast = objidNil;
    OBJID objidHint = objidNil;
    OBJID objidIndeterminateHint = objidNil;
    CatPassType cpt = cptNormal;
    *ppgnoLastProcessed = pgnoNull;
    *pspcatfLastProcessed = spcatfNone;

    while ( fTrue )
    {
        while ( pgnoCurrent <= pgnoLast )
        {
            if ( fDataMove )
            {
                *pdhrtDataMove += DhrtHRTElapsedFromHrtStart( hrtDataMoveStart );
                fDataMove = fFalse;
            }

            // Resume to normal pass type if we're past the proper page or if
            // the original hint for indeterminate pages has changed.
            if ( ( cpt == cptIndeterminateRetry ) &&
                 ( ( pgnoCurrent >= pgnoPostIndeterminateRetry ) || ( objidHint != objidIndeterminateHint ) ) )
            {
                Assert( pgnoIndeterminateFirst != pgnoNull );
                Assert( ( pgnoCurrent == pgnoPostIndeterminateRetry ) || ( objidHint != objidIndeterminateHint ) );
                cpt = cptNormal;
                pgnoIndeterminateFirst = pgnoNull;
                pgnoPostIndeterminateRetry = pgnoNull;
                objidIndeterminateHint = objidNil;
            }

            // Some pages cannot be moved by definition.
            // They are part of the first extent of the database, so we should have bailed earlier.
            if ( FCATBaseSystemFDP( pgnoCurrent ) ||            // Any base system page.
                 ( pgnoCurrent == ( pgnoSystemRoot + 1 ) ) ||   // Root OE.
                 ( pgnoCurrent == ( pgnoSystemRoot + 2 ) ) )    // Toot AE.
            {
                *psdr = sdrPageNotMovable;
                goto HandleError;
            }

            SPFreeSpaceCatCtx( &pSpCatCtx );

            if ( fInTransaction && fMovedPage )
            {
                Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
                fInTransaction = fFalse;
                fMovedPage = fFalse;
            }

            // We've got signaled to stop.
            if ( !pfmp->FShrinkIsActive() )
            {
                Error( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ) );
            }

            if ( !fInTransaction )
            {
                Assert( !fMovedPage );

                // ErrLGSetExternalHeader, called in ErrNDSetExternalHeader, asks to be under
                // a transaction. However, note that space operations which are about to be performed
                // by this function are not versioned and as such do not really rollback upon
                // transaction rollback.
                Call( ErrDIRBeginTransaction( ppib, 53898, NO_GRBIT ) );
                fInTransaction = fTrue;
            }

            Assert( fInTransaction && !fMovedPage );

            // Check timeout expiration.
            if ( ( ( dtickQuota >= 0 ) && ( CmsecHRTFromHrtStart( hrtStarted ) >= (QWORD)dtickQuota ) ) ||
                 ( pgnoCurrent == UlConfigOverrideInjection( 43982, pgnoNull ) ) )
            {
                *psdr = sdrTimeout;
                goto HandleError;
            }

            // Preread pages if we crossed the preread waypoint.
            if ( ( pgnoCurrent >= pgnoPrereadWaypoint ) && ( pgnoPrereadNext <= pgnoLast ) )
            {
                const CPG cpgPrereadCurrent = LFunctionalMin( cpgPrereadMax, (CPG)( pgnoLast - pgnoPrereadNext + 1 ) );
                Assert( cpgPrereadCurrent >= 1 );
                BFPrereadPageRange( ifmp, pgnoPrereadNext, cpgPrereadCurrent, bfprfDefault, ppib->BfpriPriority( ifmp ), *tcScope );
                pgnoPrereadNext += cpgPrereadCurrent;
                pgnoPrereadWaypoint = pgnoPrereadNext - ( cpgPrereadCurrent / 2 );
            }

            // Categorize page.
            fPageCategorization = fTrue;
            hrtPageCategorizationStart = HrtHRTCount();
            OBJID objidCurrent = objidNil;
            SpaceCategoryFlags spcatfCurrent = spcatfNone;
            Call( ErrSPGetSpaceCategory(
                    ppib,
                    ifmp,
                    pgnoCurrent,
                    objidHint,
                    pfmp->FRunShrinkDatabaseFullCatOnAttach(),
                    &objidCurrent,
                    &spcatfCurrent,
                    &pSpCatCtx ) );
            Assert( !FSPSpaceCatUnknown( spcatfCurrent ) ); // We should not get this here.
            Assert( !FSPSpaceCatNotOwnedEof( spcatfCurrent ) ); // We should not get this here because we're only processing known-owned pages.
            *pdhrtPageCategorization += DhrtHRTElapsedFromHrtStart( hrtPageCategorizationStart );
            fPageCategorization = fFalse;

            // It is not possible to handle these. The database is now effectively unshrinkable.
            if ( !FSPValidSpaceCategoryFlags( spcatfCurrent )   ||
                    FSPSpaceCatInconsistent( spcatfCurrent )    ||
                    FSPSpaceCatNotOwned( spcatfCurrent ) )
            {
                AssertTrack( fFalse, OSFormat( "ShrinkMoveUnmovable:0x%I32x", spcatfCurrent ) );
                *psdr = sdrPageNotMovable;
                goto HandleError;
            }

            // Check if we are in indeterminate page lookup mode and finally found a good hint.
            if ( ( cpt == cptIndeterminateLookup ) && ( objidCurrent != objidNil ) )
            {
                Assert( pgnoIndeterminateFirst != pgnoNull );
                Assert( pgnoIndeterminateFirst < pgnoCurrent );
                Assert( pgnoPostIndeterminateRetry == pgnoNull );

                // Transition to retry mode and go back to the previously indeterminate page.
                cpt = cptIndeterminateRetry;
                pgnoPostIndeterminateRetry = pgnoCurrent;
                pgnoCurrent = pgnoIndeterminateFirst;
                objidIndeterminateHint = objidCurrent;
                objidHint = objidIndeterminateHint;
                *ppgnoLastProcessed = pgnoNull;
                *pspcatfLastProcessed = spcatfNone;
                continue;
            }

            // Make sure we're not spinning in an infinite loop.
            if ( ( *ppgnoLastProcessed == pgnoCurrent ) &&
                    ( *pspcatfLastProcessed == spcatfCurrent ) &&
                    ( objidLast == objidCurrent ) )
            {
                AssertTrack( fFalse, OSFormat( "ShrinkMoveLoop:0x%I32x", spcatfCurrent ) );
                *psdr = sdrUnexpected;
                goto HandleError;
            }

            objidLast = objidCurrent;
            objidHint = ( objidCurrent != objidNil ) ? objidCurrent : objidHint;
            *ppgnoLastProcessed = pgnoCurrent;
            *pspcatfLastProcessed = spcatfCurrent;

            fDataMove = fTrue;
            hrtDataMoveStart = HrtHRTCount();

            // Indeterminate and leaked pages share some common handling.
            if ( FSPSpaceCatIndeterminate( spcatfCurrent ) || FSPSpaceCatLeaked( spcatfCurrent ) )
            {
                // Ineterminate-page pre-checks.
                if ( FSPSpaceCatIndeterminate( spcatfCurrent ) )
                {
                    // If the page could not be categorized, try to find a better hint or add it to the
                    // shelved page list and move on.
                    if ( cpt == cptNormal )
                    {
                        Assert( pgnoIndeterminateFirst == pgnoNull );
                        Assert( pgnoPostIndeterminateRetry == pgnoNull );
                        Assert( objidIndeterminateHint == objidNil );

                        cpt = cptIndeterminateLookup;
                        pgnoIndeterminateFirst = pgnoCurrent;
                        pgnoCurrent++;
                        continue;
                    }
                    else if ( cpt == cptIndeterminateLookup )
                    {
                        Assert( pgnoIndeterminateFirst != pgnoNull );
                        Assert( pgnoPostIndeterminateRetry == pgnoNull );
                        Assert( objidIndeterminateHint == objidNil );

                        pgnoCurrent++;
                        continue;
                    }

                    Assert( cpt == cptIndeterminateRetry );
                    Assert( pgnoIndeterminateFirst != pgnoNull );
                    Assert( pgnoPostIndeterminateRetry != pgnoNull );
                    Assert( pgnoIndeterminateFirst < pgnoPostIndeterminateRetry );

                    // We should never get an indeterminate page if full categorization is on.
                    if ( pfmp->FRunShrinkDatabaseFullCatOnAttach() )
                    {
                        AssertTrack( fFalse, "ShrinkMoveUnexpectedIndeterminatePage" );
                        *psdr = sdrUnexpected;
                        goto HandleError;
                    }

                    // Check if indeterminate-page handling is enabled and supported.
                    // Note that FMP::FEfvSupported() below checks for both DB and log versions, but JET_efvShelvedPages2
                    // only upgrades the DB version so technically, we wouldn't need to check for the log version.
                    if ( pfmp->FShrinkDatabaseDontTruncateIndeterminatePagesOnAttach() ||
                         !pfmp->FEfvSupported( JET_efvShelvedPages2 ) )
                    {
                        *psdr = sdrPageNotMovable;
                        goto HandleError;
                    }
                }

                // Leaked-page pre-checks.
                if ( FSPSpaceCatLeaked( spcatfCurrent ) )
                {
                    // We should never see a page leaked at the root level if full categorization is off.
                    if ( !pfmp->FRunShrinkDatabaseFullCatOnAttach() && ( PgnoFDP( pSpCatCtx->pfucb ) == pgnoSystemRoot ) )
                    {
                        AssertTrack( fFalse, "ShrinkMoveUnexpectedRootLeakedPage" );
                        *psdr = sdrUnexpected;
                        goto HandleError;
                    }

                    // Check if leaked-page handling is enabled.
                    if ( pfmp->FShrinkDatabaseDontTruncateLeakedPagesOnAttach() )
                    {
                        *psdr = sdrPageNotMovable;
                        goto HandleError;
                    }
                }

                CPAGE cpage;

                // From here on, we do some defense-in-depth common checks to make sure we aren't truncating
                // pages which might potentially contain useful data.

                Call( ErrBFRDWLatchPage(
                        &bfl,
                        ifmp,
                        pgnoCurrent,
                        BFLatchFlags( bflfNoTouch | bflfNoFaultFail | bflfUninitPageOk | bflfExtensiveChecks ),
                        ppib->BfpriPriority( ifmp ),
                        *tcScope ) );
                fPageLatched = fTrue;

                const ERR errPageStatus = ErrBFLatchStatus( &bfl );
                if ( ( errPageStatus < JET_errSuccess ) &&
                     ( errPageStatus != JET_errPageNotInitialized ) )
                {
                    Call( errPageStatus );
                }

                cpage.ReBufferPage(
                        bfl,
                        ifmp,
                        pgnoCurrent,
                        bfl.pv,
                        (ULONG)pfmp->CbPage() );

                const BOOL fPreInitPage = cpage.FPreInitPage();
                const BOOL fEmptyPage = cpage.FEmptyPage();
                const INT clines = cpage.Clines();
                const OBJID objidFDP = cpage.ObjidFDP();
                const ULONG fPageFlags = cpage.FFlags();
                const DBTIME dbtimeOnPage = cpage.Dbtime();

                // Unlatch page, as we may need to search the catalog below and the latches might collide,
                // though it is not expected, since we are looking at a leaked or indeterminate page, so
                // it shouldn't be latched as part of consulting the catalog.
                cpage.UnloadPage();
                BFRDWUnlatch( &bfl );
                fPageLatched = fFalse;

                // Make sure the page does not have any useful data.
                if ( ( errPageStatus != JET_errPageNotInitialized ) &&
                     ( ( dbtimeOnPage != dbtimeShrunk ) || ( clines != 0 ) ) &&
                     ( !fPreInitPage || ( clines > 0 ) ) &&
                     ( !fEmptyPage || ( clines != 0 ) ) )
                {
                    const OBJID objidPage = objidFDP;
                    Assert( objidPage != objidNil );

                    // Check if the object exists.
                    BOOL fObjidExists = ( objidPage == objidSystemRoot );
                    if ( !fObjidExists )
                    {
                        OBJID objidTable = objidNil;
                        SYSOBJ sysobj = sysobjNil;
                        Call( ErrCATGetObjidMetadata( ppib, ifmp, objidFDP, &objidTable, &sysobj ) );
                        fObjidExists = ( sysobj != sysobjNil );
                    }

                    if ( fObjidExists || ( objidPage == objidNil ) )
                    {
                        // We are rejecting this page because it might potentially contain data and it belongs
                        // to a valid object or it had objidNil stamped, which is unexpected, but we'll still
                        // fail for the sake of safety.
                        AssertTrack( fFalse, OSFormat( "%hs:0x%I32x",
                                                       FSPSpaceCatLeaked( spcatfCurrent ) ?
                                                           "ShrinkMoveInvalidLeakedPage" :
                                                           "ShrinkMoveInvalidIndPage",
                                                        fPageFlags ) );
                        *psdr = sdrUnexpected;
                        goto HandleError;
                    }
                }

                // Emit divergence check so replicas will stop redo if the check fails, avoiding
                // propagation of a bad truncation.
                // For testing purposes, we would like to enable this even if JET_paramEnableExternalAutoHealing
                // is not enabled (so that crash/recovery would check for divergences). However, unlogged index
                // creation produces pages in the database for which updates aren't logged, so we may end up with
                // leaked pages which have a dbtime that is greater than the running dbtime of the database if we
                // crashing during that index creation operation and end up leaking that space.
                if ( pfmp->FLogOn() && BoolParam( pfmp->Pinst(), JET_paramEnableExternalAutoHealing ) )
                {
                    if ( !pfmp->FEfvSupported( JET_efvScanCheck2 ) )
                    {
                        *psdr = sdrPageNotMovable;
                        goto HandleError;
                    }

                    if ( !fRolledLogOnScanCheck )
                    {
                        // Force a log rollover. We do this because we need to make sure that the ScanCheck log
                        // record we are about to issue is outside of the required range in a replicated system
                        // that runs with a non-zero LLR. If the ScanCheck is in the initial required range of
                        // a passive copy of a replicated system, we may skip certain checks due to it containing
                        // future data, therefore rendering useless issuing the ScanCheck in the first place. Note
                        // that we wouldn't have this problem if the required range and waypoint had full LGPOS
                        // precision, instead of log generation only.
                        // We could have limited this to LLR being set, but we're always doing it to avoid multiple
                        // modalities.
                        Call( ErrLGForceLogRollover( pfmp->Pinst(), __FUNCTION__ ) );
                        fRolledLogOnScanCheck = fTrue;
                    }

                    // Latch the page again.
                    Call( ErrBFRDWLatchPage(
                            &bfl,
                            ifmp,
                            pgnoCurrent,
                            BFLatchFlags( bflfNoTouch | bflfNoFaultFail | bflfUninitPageOk | bflfExtensiveChecks ),
                            ppib->BfpriPriority( ifmp ),
                            *tcScope ) );
                    fPageLatched = fTrue;

                    const ERR errPageStatusRelatch = ErrBFLatchStatus( &bfl );
                    if ( ( errPageStatusRelatch < JET_errSuccess ) &&
                         ( errPageStatusRelatch != JET_errPageNotInitialized ) )
                    {
                        Call( errPageStatusRelatch );
                    }

                    cpage.ReBufferPage(
                            bfl,
                            ifmp,
                            pgnoCurrent,
                            bfl.pv,
                            (ULONG)pfmp->CbPage() );

                    const BOOL fPreInitPageRelatch = cpage.FPreInitPage();
                    const BOOL fEmptyPageRelatch = cpage.FEmptyPage();
                    const INT clinesRelatch = cpage.Clines();
                    const OBJID objidFDPRelatch = cpage.ObjidFDP();
                    const ULONG fPageFlagsRelatch = cpage.FFlags();
                    const DBTIME dbtimeOnPageRelatch = cpage.Dbtime();

                    // Defense-in-depth only. We don't expect the page to change from underneath us.
                    if ( ( !!fPreInitPage != !!fPreInitPageRelatch ) ||
                         ( !!fEmptyPage != !!fEmptyPageRelatch ) ||
                         ( clines != clinesRelatch ) ||
                         ( objidFDP != objidFDPRelatch ) ||
                         ( fPageFlags != fPageFlagsRelatch ) ||
                         ( dbtimeOnPage != dbtimeOnPageRelatch ) )
                    {
                        AssertTrack( fFalse, "ShrinkMovePageChanged" );
                        *psdr = sdrUnexpected;
                        goto HandleError;
                    }

                    Call( ErrDBMEmitDivergenceCheck( ifmp, pgnoCurrent, scsDbShrink, &cpage ) );

                    // Unlatch.
                    cpage.UnloadPage();
                    BFRDWUnlatch( &bfl );
                    fPageLatched = fFalse;
                }

                // We'll archive the data if requested.
                if ( BoolParam( pfmp->Pinst(), JET_paramFlight_EnableShrinkArchiving ) )
                {
                    tcScope->iorReason.SetIorp( iorpDatabaseShrink );
                    Call( ErrIOArchiveShrunkPages( ifmp, *tcScope, pgnoCurrent, 1 ) );
                    tcScope->iorReason.SetIorp( iorpNone );
                }

                // Leaked-page handling.
                if ( FSPSpaceCatLeaked( spcatfCurrent ) )
                {
                    // Release page to its owner and re-evaluate the page.
                    Call( ErrSPCaptureSnapshot( pSpCatCtx->pfucb, pgnoCurrent, 1 ) );
                    Call( ErrSPFreeExt( pSpCatCtx->pfucb, pgnoCurrent, 1, "PageUnleak" ) );
                    cpgUnleaked++;
                    fMovedPage = fTrue;
                    continue;
                }

                // Indeterminate-page handling.
                if ( FSPSpaceCatIndeterminate( spcatfCurrent ) )
                {
                    // Add it to the shelved page list and move on.
                    Call( ErrSPShelvePage( ppib, ifmp, pgnoCurrent ) );

                   cpgShelved++;

                    // It should be impossible to change a tree's layout such that the process of shelving
                    // a page ends up consuming or changing the page itself. Still, re-check it.
                    Assert( pSpCatCtx == NULL );
                    Call( ErrSPGetSpaceCategory(
                            ppib,
                            ifmp,
                            pgnoCurrent,
                            objidHint,
                            pfmp->FRunShrinkDatabaseFullCatOnAttach(),
                            &objidCurrent,
                            &spcatfCurrent,
                            &pSpCatCtx ) );
                    if ( !FSPSpaceCatIndeterminate( spcatfCurrent ) )
                    {
                        AssertTrack( fFalse, "ShrinkMoveIndeterminateChanged" );
                        Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
                    }

                    Assert( fInTransaction && !fMovedPage );
                    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
                    fInTransaction = fFalse;
                    pgnoCurrent++;
                    continue;
                }
                Assert( fFalse );
            }  // end if ( FSPSpaceCatIndeterminate( spcatfCurrent ) || FSPSpaceCatLeaked( spcatfCurrent ) )

            // Handle split buffer pages (free split buffer and allocate a new one below the current shrink watermark).
            if ( FSPSpaceCatSplitBuffer( spcatfCurrent ) )
            {
                Assert( FSPSpaceCatAvailable( spcatfCurrent ) ); // Split buffer pages are available by definition.
                Assert( FSPSpaceCatAnySpaceTree( spcatfCurrent ) ); // Split buffer pages are space pages by definition.

                Call( ErrSPReplaceSPBuf( pSpCatCtx->pfucb, pSpCatCtx->pfucbParent, pgnoCurrent ) );
                fMovedPage = fTrue;
                (*pcprintfShrinkTraceRaw)( "ShrinkMove[%I32u:%I32u:%d]\r\n", objidCurrent, pgnoCurrent, (int)spcatfCurrent );
                continue;
            }

            // Handle small-space format: convert to large space format first, let the pages be moved,
            // by the next iterations as non-small-space pages.
            if ( FSPSpaceCatSmallSpace( spcatfCurrent ) )
            {
                if ( !FSPSpaceCatRoot( spcatfCurrent ) )
                {
                    // We should have found the root first as small space trees do not cross extents.
                    FireWall( OSFormat( "ShrinkMoveSmallSpNonRootFirst:0x%I32x", spcatfCurrent ) );
                }

                Assert( !FSPSpaceCatAnySpaceTree( spcatfCurrent ) );
                Expected( PgnoFDP( pSpCatCtx->pfucb ) == pgnoCurrent );

                // Avoid bursting to large space if this is a fairly small database, since
                // large space trees are more expensive to populate.
                if ( pgnoLast < (PGNO)( 4 * UlParam( pfmp->Pinst(), JET_paramDbExtensionSize ) ) )
                {
                    *psdr = sdrNoSmallSpaceBurst;
                    goto HandleError;
                }

                Call( ErrSPBurstSpaceTrees( pSpCatCtx->pfucb ) );

                // Next iteration will see the same page as large space and move it appropriately.
                fMovedPage = fTrue;
                continue;
            }

            // If the page is available, move on to the next page:
            //   o If it's available to the DB root, it is part of an available extent which is ready to be shrunk.
            //   o If it's available to an object, it is not ready to be shrunk yet, but in most cases no action is
            //     required because it will eventually be released to the root once neighboring pages are also made
            //     available and a full extent is then released. However, there two known cases in which we need to
            //     give it a little push:
            //       1- We crashed in-between releasing space which would make up for a fully available extent to
            //          a table and releasing that full extent up to its parent.
            //       2- We currently do not coalesce space released from different space pools even if they turn out
            //          to make up a full available extent.
            if ( FSPSpaceCatAvailable( spcatfCurrent ) )
            {
                // We should have found the root first, as small space trees do not cross extents.
                if ( FSPSpaceCatSplitBuffer( spcatfCurrent ) ||     // We should have handled it above.
                        FSPSpaceCatAnySpaceTree( spcatfCurrent ) || // We should have handled it above (split buffers).
                        FSPSpaceCatSmallSpace( spcatfCurrent ) )    // We should have moved/converted whole small space trees starting from their roots first.
                {
                    AssertTrack( fFalse, OSFormat( "ShrinkMoveUnexpectedAvail:0x%I32x", spcatfCurrent ) );
                    *psdr = sdrUnexpected;
                    goto HandleError;
                }

                BOOL fMoveToNextPage = fTrue;

                // Fix up any pending coalescing at the non-root level.
                if ( objidCurrent != objidSystemRoot )
                {
                    Assert( objidCurrent != objidNil );

                    BOOL fCoalesced = fFalse;
                    Call( ErrSPTryCoalesceAndFreeAvailExt( pSpCatCtx->pfucb, pgnoCurrent, &fCoalesced ) );
                    if ( fCoalesced )
                    {
                        fMoveToNextPage = fFalse;
                        fMovedPage = fTrue;
                    }
                }

                if ( fMoveToNextPage )
                {
                    pgnoCurrent++;
                }
                continue;
            }

            // Roots.
            if ( FSPSpaceCatRoot( spcatfCurrent ) )
            {
                Assert( !FSPSpaceCatSmallSpace( spcatfCurrent ) );

                if ( !pfmp->FShrinkDatabaseDontMoveRootsOnAttach() && pfmp->FEfvSupported( JET_efvRootPageMove ) )
                {
                    // Note that we currently only support moving all roots of a tree (root itself, OE and AE root)
                    // at the same time. So depending on what kind of root we are processing, we need to pass the
                    // actual root of the tree.
                    const PGNO pgnoFDP = pSpCatCtx->pfucb->u.pfcb->PgnoFDP();
                    Assert( !FSPSpaceCatAnySpaceTree( spcatfCurrent ) == ( pgnoFDP == pgnoCurrent ) );

                    // We have to close all cursors before moving a root.
                    SPFreeSpaceCatCtx( &pSpCatCtx );

                    Call( ErrSHKRootPageMove( ppib, ifmp, pgnoFDP ) );
                    fMovedPage = fTrue;
                    (*pcprintfShrinkTraceRaw)( "ShrinkMoveRoot[%I32u:%I32u:%I32u:%d]\r\n", objidCurrent, pgnoCurrent, pgnoFDP, (int)spcatfCurrent );

                    cpgMoved += 3;  // FDP + OE + AE.
                    continue;
                }
                else
                {
                    *psdr = sdrPageNotMovable;
                    goto HandleError;
                }
            }

            // Strictly internal or leaf pages.
            if ( FSPSpaceCatStrictlyInternal( spcatfCurrent ) || FSPSpaceCatStrictlyLeaf( spcatfCurrent ) )
            {
                Assert( !FSPSpaceCatSmallSpace( spcatfCurrent ) );

                const BOOL fSpacePage = FSPSpaceCatAnySpaceTree( spcatfCurrent );

                err = ErrBTPageMove(
                        fSpacePage ? pSpCatCtx->pfucbSpace : pSpCatCtx->pfucb,
                        *( pSpCatCtx->pbm ),
                        pgnoCurrent,
                        FSPSpaceCatStrictlyLeaf( spcatfCurrent ),
                        fSPNoFlags,
                        NULL );

                // If this is a space tree, we may need to retry because reserving split buffers
                // for the move might have changed the page we're trying to move itself, which
                // will surface as JET_errRecordNotFound when we try to latch the move path.
                if ( fSpacePage && ( err == JET_errRecordNotFound ) )
                {
                    *ppgnoLastProcessed = pgnoNull;
                    *pspcatfLastProcessed = spcatfNone;
                    err = JET_errSuccess;
                    fMovedPage = fTrue;
                    continue;
                }

                // We should never get this here because we passed a pgnoSource in and we have
                // exclusive access to the tree, so we must have been able to find the page
                // with the passed in bookmark in the expected conditions (i.e., strictly internal
                // or leaf).
                AssertTrack( ( err != wrnBTShallowTree ) && ( err != JET_errRecordNotFound ),
                    OSFormat( ( err == wrnBTShallowTree ) ?
                        "ShrinkMoveUnexpectedShallowBt:0x%I32x" : "ShrinkMoveUnexpectedNotFound:0x%I32x",
                        spcatfCurrent ) );
                Call( err );
                fMovedPage = fTrue;
                (*pcprintfShrinkTraceRaw)( "ShrinkMove[%I32u:%I32u:%d]\r\n", objidCurrent, pgnoCurrent, (int)spcatfCurrent );

                cpgMoved++;
                continue;
            }

            // WARNING: do not add anything at the end of this block! All individual cases are supposed to
            // be handled in their respective blocks and the flow continues from there to the next iteration
            // of the loop.
            AssertTrack( fFalse, OSFormat( "ShrinkMoveUnhandled:0x%I32x", spcatfCurrent ) );
            *psdr = sdrUnexpected;
            goto HandleError;
        }  // end while ( pgnoCurrent <= pgnoLast )

        if ( fDataMove )
        {
            *pdhrtDataMove += DhrtHRTElapsedFromHrtStart( hrtDataMoveStart );
            fDataMove = fFalse;
        }

        // If we made all the way with a lookup pending, go back and re-evaluate.
        if ( cpt == cptIndeterminateLookup )
        {
            Assert( pgnoIndeterminateFirst != pgnoNull );
            Assert( pgnoIndeterminateFirst <= pgnoLast );
            Assert( pgnoPostIndeterminateRetry == pgnoNull );
            Assert( pgnoCurrent > pgnoLast );
            Assert( objidIndeterminateHint == objidNil );

            cpt = cptIndeterminateRetry;
            pgnoPostIndeterminateRetry = pgnoCurrent;
            pgnoCurrent = pgnoIndeterminateFirst;
            objidIndeterminateHint = objidHint;
            objidHint = objidIndeterminateHint;
            *ppgnoLastProcessed = pgnoNull;
            *pspcatfLastProcessed = spcatfNone;
        }
        else
        {
            break;
        }
    }  // end while ( fTrue )

HandleError:
    Assert( !( fPageCategorization && fDataMove ) );

    if ( fPageCategorization )
    {
        *pdhrtPageCategorization += DhrtHRTElapsedFromHrtStart( hrtPageCategorizationStart );
        fPageCategorization = fFalse;
    }

    if ( fDataMove )
    {
        *pdhrtDataMove += DhrtHRTElapsedFromHrtStart( hrtDataMoveStart );
        fDataMove = fFalse;
    }

    if ( fPageLatched )
    {
        BFRDWUnlatch( &bfl );
        fPageLatched = fFalse;
    }

    SPFreeSpaceCatCtx( &pSpCatCtx );

    // WARNING: most (if not all) of the above is done without versioning, so there
    // really isn't any rollback of the update.
    if ( fInTransaction )
    {
        if ( ( err >= JET_errSuccess ) && fMovedPage )
        {
            // Just in case, but all begin/commit cycles are supposed to be handled by the main loop above.
            Assert( fFalse );
            fInTransaction = ( ErrDIRCommitTransaction( ppib, NO_GRBIT ) < JET_errSuccess );
        }

        if ( fInTransaction )
        {
            CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
            fInTransaction = fFalse;
        }
    }

    if ( eiLastOE.FValid() )
    {
        const CPG cpgLastOE = eiLastOE.CpgExtent();
        AssertTrack( cpgShelved <= cpgLastOE, "ShrinkMoveTooManyPagesShelved" );
        AssertTrack( cpgUnleaked <= cpgLastOE, "ShrinkMoveTooManyPagesUnleaked" );
    }

    *pcpgMoved += cpgMoved;
    *pcpgShelved += cpgShelved;
    *pcpgUnleaked += cpgUnleaked;

    return err;
}

// Iterates over the Avail Extents in the root of the given FMP, shrinking only fully available extents.
// Assumes exclusive access to root space trees and no concurrency with other threads trying to shrink or extend
// the database.
ERR ErrSHKShrinkDbFromEof(
    _In_ PIB *ppib,
    _In_ const IFMP ifmp )
{
    ERR err = JET_errSuccess;
    ERR errVerClean = JET_errSuccess;
    BOOL fDbOpen = fFalse;
    BOOL fMayHaveShrunk = fFalse;
    BOOL fMSysObjidsReady = fFalse;
    FMP* const pfmp = g_rgfmp + ifmp;
    INST* const pinst = pfmp->Pinst();
    EXTENTINFO eiInitialOE, eiFinalOE;
    QWORD cbSizeFileInitial = 0;
    QWORD cbSizeFileFinal = 0;
    QWORD cbSizeOwnedInitial = 0;
    QWORD cbSizeOwnedFinal = 0;
    CPRINTF* pcprintfShrinkTraceRaw = NULL;
    const HRT hrtStarted = HrtHRTCount();
    CPG cpgMoved = 0, cpgShelved = 0, cpgUnleaked = 0;
    ShrinkDoneReason sdr = sdrNone;
    PGNO pgnoFirstFromLastExtentShrunkPrev = pgnoNull;
    PGNO pgnoLastProcessed = pgnoNull;
    SpaceCategoryFlags spcatfLastProcessed = spcatfNone;
    HRT dhrtExtMaint = 0;
    HRT dhrtFileTruncation = 0;
    HRT dhrtPageCategorization = 0;
    HRT dhrtDataMove = 0;
    OnDebug( BOOL fDbMayHaveChanged = fFalse );

    Assert( !pfmp->FIsTempDB() );

    Call( ErrSHKIShrinkEofTracingBegin( pinst->m_pfsapi, g_rgfmp[ ifmp ].WszDatabaseName(), &pcprintfShrinkTraceRaw ) );

    Assert( !BoolParam( JET_paramEnableViewCache ) );

    // First, delete any previously saved shrink archive files.
    if ( !BoolParam( pinst, JET_paramFlight_EnableShrinkArchiving ) )
    {
        (void)ErrIODeleteShrinkArchiveFiles( ifmp );
    }

    IFMP ifmpDummy;
    Call( ErrDBOpenDatabase( ppib, pfmp->WszDatabaseName(), &ifmpDummy, JET_bitDbExclusive ) );
    Assert( pfmp->FExclusiveBySession( ppib ) );
    Assert( ifmpDummy == ifmp );
    fDbOpen = fTrue;

    Call( pfmp->Pfapi()->ErrSize( &cbSizeFileInitial, IFileAPI::filesizeLogical ) );

    // Open cursors to space trees.
    Call( ErrSPGetLastExtent( ppib, ifmp, &eiInitialOE ) );
    cbSizeOwnedInitial = OffsetOfPgno( eiInitialOE.PgnoLast() + 1 );
    Assert( cbSizeOwnedInitial <= cbSizeFileInitial );

    // Shrink requires a fully populated MSysObjids table.
    Call( ErrCATCheckMSObjidsReady( ppib, ifmp, &fMSysObjidsReady ) );
    if ( !fMSysObjidsReady )
    {
        sdr = sdrMSysObjIdsNotReady;
        goto DoneWithDataMove;
    }

    // Check if physical file size is smaller than size represented in the OE.
    if ( OffsetOfPgno( eiInitialOE.PgnoLast() + 1 ) > cbSizeFileInitial )
    {
        AssertTrack( fFalse, "ShrinkEofOePgnoLastBeyondFsFileSize" );
        Error( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
    }

    OnDebug( fDbMayHaveChanged = fTrue );

    Assert( !pfmp->FShrinkIsRunning() );
    pfmp->SetShrinkIsRunning();

    // Just in case, but we don't expect version store and DB tasks to be running at this point.
    errVerClean = PverFromPpib( ppib )->ErrVERRCEClean( ifmp );
    if ( errVerClean != JET_errSuccess )
    {
        AssertTrack( fFalse, OSFormat( "ShrinkEofBeginVerCleanErr:%d", errVerClean ) );
        Error( ErrERRCheck( JET_errDatabaseInUse ) );
    }
    pfmp->WaitForTasksToComplete();

    // Clean up orphaned shelved page extents, which may be present if a previous shrink operation
    // bailed for any reason without freeing up and truncating the extent after having added shelved
    // pages. Below-EOF shelved pages are innocuous for subsequent space operations in general,
    // but must be removed before a new shrink pass is attempted. Otherwise, we could either end up
    // with duplicate shelving during the new pass or, even worse, improperly truncate an extent based
    // on shelved pages which were available when they were shelved but then got allocated and used.
    CallJ( ErrSPUnshelveShelvedPagesBelowEof( ppib, ifmp ), DoneWithDataMove );

    // Shrink in steps.
    forever
    {
        PGNO pgnoFirstFromLastExtentShrunk = pgnoNull;
        fMayHaveShrunk = fTrue;
        Assert( sdr == sdrNone );

        // Truncate as many extents as possible. Do it one by one.
        PGNO pgnoFirstFromLastExtentTruncatedPrev = pgnoNull;
        PGNO pgnoFirstFromLastExtentTruncated = pgnoNull;
        do
        {
            CallJ( ErrSPShrinkTruncateLastExtent(
                    ppib,
                    ifmp,
                    pcprintfShrinkTraceRaw,
                    &dhrtExtMaint,
                    &dhrtFileTruncation,
                    &pgnoFirstFromLastExtentTruncated,
                    &sdr ),
                  DoneWithDataMove );

            Assert( pgnoFirstFromLastExtentTruncated != pgnoNull );
            Assert( ( pgnoFirstFromLastExtentTruncated < pgnoFirstFromLastExtentTruncatedPrev ) ||
                    ( pgnoFirstFromLastExtentTruncatedPrev == pgnoNull ) );
            pgnoFirstFromLastExtentTruncatedPrev = pgnoFirstFromLastExtentTruncated;
            pgnoFirstFromLastExtentShrunk = pgnoFirstFromLastExtentTruncated;

            // Check done conditions.
            if ( sdr != sdrNone )
            {
                if ( pgnoFirstFromLastExtentTruncated == pgnoSystemRoot )
                {
                    pgnoLastProcessed = pgnoSystemRoot;
                    spcatfLastProcessed = spcatfRoot;
                }
                if ( pgnoLastProcessed == pgnoNull )
                {
                    Assert( pgnoFirstFromLastExtentTruncated != pgnoNull );
                    pgnoLastProcessed = pgnoFirstFromLastExtentTruncated;
                    spcatfLastProcessed = spcatfNone;
                }
                goto DoneWithDataMove;
            }
        }
        while ( err != JET_wrnShrinkNotPossible );

        Assert( sdr == sdrNone );
        err = JET_errSuccess;

        // Make sure we are not bouncing back and forth between truncation and data move.
        if ( ( pgnoFirstFromLastExtentShrunkPrev != pgnoNull ) &&
             ( pgnoFirstFromLastExtentShrunk >= pgnoFirstFromLastExtentShrunkPrev ) )
        {
            AssertTrack( fFalse, "ShrinkEofStalled" );
            sdr = sdrUnexpected;
            goto DoneWithDataMove;
        }
        pgnoFirstFromLastExtentShrunkPrev = pgnoFirstFromLastExtentShrunk;

        // Now, move data from the last extent.
        PGNO pgnoFirstFromLastExtentMoved = pgnoNull;
        CallJ( ErrSHKIMoveLastExtent(
                ppib,
                ifmp,
                hrtStarted,
                pcprintfShrinkTraceRaw,
                &cpgMoved,
                &cpgShelved,
                &cpgUnleaked,
                &dhrtPageCategorization,
                &dhrtDataMove,
                &sdr,
                &pgnoLastProcessed,
                &pgnoFirstFromLastExtentMoved,
                &spcatfLastProcessed ), DoneWithDataMove );
        Assert( pgnoFirstFromLastExtentMoved == pgnoFirstFromLastExtentTruncated );

        // We've got signaled to stop.
        if ( !pfmp->FShrinkIsActive() )
        {
            CallJ( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ), DoneWithDataMove );
        }

        // Check done conditions.
        if ( sdr != sdrNone )
        {
            goto DoneWithDataMove;
        }
    }

DoneWithDataMove:
    if ( err == errSPNoSpaceBelowShrinkTarget )
    {
        Assert( sdr == sdrNone );
        sdr = sdrNoLowAvailSpace;
        err = JET_errSuccess;
    }
    Call( err );

    Assert( sdr != sdrNone );
    err = JET_errSuccess;

    // See how much we shrunk the database by.
    Call( ErrSPGetLastExtent( ppib, ifmp, &eiFinalOE ) );
    Call( pfmp->Pfapi()->ErrSize( &cbSizeFileFinal, IFileAPI::filesizeLogical ) );
    cbSizeOwnedFinal = OffsetOfPgno( eiFinalOE.PgnoLast() + 1 );

    // Check if we now own less space (i.e., if we shrank).
    if ( eiFinalOE.PgnoLast() >= eiInitialOE.PgnoLast() )
    {
        err = ErrERRCheck( JET_wrnShrinkNotPossible );
    }

HandleError:
#ifdef DEBUG
    Assert( cpgMoved >= 0 );
    Assert( cpgShelved >= 0 );
    Assert( cpgUnleaked >= 0 );
    Assert( err != errSPNoSpaceBelowShrinkTarget );
    if ( err == JET_wrnShrinkNotPossible )
    {
        if ( fDbMayHaveChanged )
        {
            Assert( ( cbSizeFileInitial > 0 ) && ( cbSizeFileFinal > 0 ) );
            Assert( cbSizeFileInitial <= cbSizeFileFinal );
            Assert( ( cbSizeOwnedInitial > 0 ) && ( cbSizeOwnedFinal > 0 ) );
            Assert( cbSizeOwnedInitial <= cbSizeOwnedFinal );
            Assert( cbSizeOwnedFinal <= cbSizeFileFinal );
        }
    }
    else if ( err >= JET_errSuccess )
    {
        Assert( ( cbSizeFileInitial > 0 ) && ( cbSizeFileFinal > 0 ) );
        Assert( cbSizeFileInitial > cbSizeFileFinal );
        Assert( ( cbSizeOwnedInitial > 0 ) && ( cbSizeOwnedFinal > 0 ) );
        Assert( cbSizeOwnedInitial > cbSizeOwnedFinal );
        Assert( cbSizeOwnedFinal <= cbSizeFileFinal );
    }
#endif  // DEBUG

    // For event purposes, try to collect information if it isn't already collected.
    if ( ( cbSizeFileInitial > 0 ) && ( cbSizeFileFinal == 0 ) )
    {
        (void)pfmp->Pfapi()->ErrSize( &cbSizeFileFinal, IFileAPI::filesizeLogical );
    }
    if ( ( cbSizeOwnedInitial > 0 ) &&
            ( cbSizeOwnedFinal == 0 ) &&
            ( ErrSPGetLastExtent( ppib, ifmp, &eiFinalOE ) >= JET_errSuccess ) )
    {
        cbSizeOwnedFinal = OffsetOfPgno( eiFinalOE.PgnoLast() + 1 );
    }

    // Some safety checks.
    if ( ( cbSizeFileInitial != 0 ) && ( cbSizeFileFinal != 0 ) && ( cbSizeFileFinal > cbSizeFileInitial ) &&
         ( err != JET_wrnShrinkNotPossible ) && ( err >= JET_errSuccess ) )
    {
        // We don't expect to have grown the file.
        FireWall( "ShrinkEofFileGrowth" );
    }
    if ( ( cbSizeOwnedInitial != 0 ) && ( cbSizeOwnedFinal != 0 ) && ( cbSizeOwnedFinal > cbSizeOwnedInitial ) &&
         ( err != JET_wrnShrinkNotPossible ) && ( err >= JET_errSuccess ) )
    {
        // We don't expect to own more space.
        FireWall( "ShrinkEofOwnedSpaceGrowth" );
    }
    if ( ( cbSizeFileInitial != 0 ) && ( cbSizeOwnedInitial != 0 ) && ( cbSizeFileInitial < cbSizeOwnedInitial ) )
    {
        // Owned space must never be larger than the actual file size (before).
        AssertTrack( false, "ShrinkEofOeFileLargerThanOwnedBefore" );
        err = ErrERRCheck( JET_errSPOwnExtCorrupted );
    }
    if ( ( cbSizeFileFinal != 0 ) && ( cbSizeOwnedFinal != 0 ) && ( cbSizeFileFinal < cbSizeOwnedFinal ) )
    {
        // Owned space must never be larger than the actual file size (after).
        AssertTrack( fFalse, "ShrinkEofOeFileLargerThanOwnedAfter" );
        err = ErrERRCheck( JET_errSPOwnExtCorrupted );
    }

    // If the reason is not set and we failed, set it.
    Assert( ( sdr != sdrNone ) || ( err < JET_errSuccess ) );
    if ( ( sdr == sdrNone ) && ( err < JET_errSuccess ) )
    {
        sdr = sdrFailed;
    }

    // Emit event.
    OSTraceSuspendGC();
    const HRT dhrtElapsed = DhrtHRTElapsedFromHrtStart( hrtStarted );
    const double dblSecTotalElapsed = DblHRTSecondsElapsed( dhrtElapsed );
    const DWORD dwMinElapsed = (DWORD)( dblSecTotalElapsed / 60.0 );
    const double dblSecElapsed = dblSecTotalElapsed - (double)dwMinElapsed * 60.0;
    dhrtExtMaint = min( dhrtExtMaint, dhrtElapsed );
    dhrtFileTruncation = min( dhrtFileTruncation, dhrtElapsed );
    dhrtPageCategorization = min( dhrtPageCategorization, dhrtElapsed );
    dhrtDataMove = min( dhrtDataMove, dhrtElapsed );
    HRT dhrtRemaining = dhrtElapsed - ( dhrtExtMaint + dhrtFileTruncation + dhrtPageCategorization + dhrtDataMove );
    dhrtRemaining = max( dhrtRemaining, 0 );
    const WCHAR* rgwsz[] =
    {
        pfmp->WszDatabaseName(),
        OSFormatW( L"%I32u", dwMinElapsed ), OSFormatW( L"%.2f", dblSecElapsed ),
        OSFormatW( L"%I64u", cbSizeFileInitial ), OSFormatW( L"%d", pfmp->CpgOfCb( cbSizeFileInitial ) ),
        OSFormatW( L"%I64u", cbSizeFileFinal ), OSFormatW( L"%d", pfmp->CpgOfCb( cbSizeFileFinal ) ),
        OSFormatW( L"%I64u", cbSizeOwnedInitial ), OSFormatW( L"%d", pfmp->CpgOfCb( cbSizeOwnedInitial ) ),
        OSFormatW( L"%I64u", cbSizeOwnedFinal ), OSFormatW( L"%d", pfmp->CpgOfCb( cbSizeOwnedFinal ) ),
        OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgMoved ) ), OSFormatW( L"%d", cpgMoved ),
        OSFormatW( L"%d", err ),
        OSFormatW( L"%I32u:%d:0x%08I32x", pgnoLastProcessed, (int)sdr, (DWORD)spcatfLastProcessed ),
        OSFormatW( L"%.2f", ( 100.0 * (double)dhrtExtMaint ) / (double)dhrtElapsed ),
        OSFormatW( L"%.2f", ( 100.0 * (double)dhrtFileTruncation ) / (double)dhrtElapsed ),
        OSFormatW( L"%.2f", ( 100.0 * (double)dhrtPageCategorization ) / (double)dhrtElapsed ),
        OSFormatW( L"%.2f", ( 100.0 * (double)dhrtDataMove ) / (double)dhrtElapsed ),
        OSFormatW( L"%.2f", ( 100.0 * (double)dhrtRemaining ) / (double)dhrtElapsed ),
        OSFormatW( L"%d", cpgShelved ),
        OSFormatW( L"%d", cpgUnleaked )
    };
    UtilReportEvent(
        ( err < JET_errSuccess ) ? eventError : eventInformation,
        GENERAL_CATEGORY,
        ( err < JET_errSuccess ) ? DB_SHRINK_FAILED_ID : DB_SHRINK_SUCCEEDED_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        pfmp->Pinst() );
    OSTraceResumeGC();

    if ( fDbOpen )
    {
        Assert( pfmp->FExclusiveBySession( ppib ) );
        CallS( ErrDBCloseDatabase( ppib, ifmp, NO_GRBIT ) );
        fDbOpen = fFalse;
    }

    if ( pcprintfShrinkTraceRaw != NULL )
    {
        (*pcprintfShrinkTraceRaw)( "ShrinkDone[%d:%d:%I32d:%I32d:%I32d]\r\n", err, (int)sdr, pfmp->CpgOfCb( cbSizeOwnedInitial ), pfmp->CpgOfCb( cbSizeOwnedFinal ), pfmp->CpgOfCb( cbSizeFileFinal ) );
    }
    SHKIShrinkEofTracingEnd( &pcprintfShrinkTraceRaw );

    if ( fMayHaveShrunk )
    {
        Assert( pfmp->FShrinkIsRunning() );

        // Purge all FCBs to restore pristine attach state.
        // We don't expect any pending version store entries, but just in case, avoid purging the FCBs for the
        // database in that case. The potential issue is that clients that may try to change template tables
        // will get an error because the FCB has been opened (i.e., produced an FUCB) by the categorization code.
        errVerClean = PverFromPpib( ppib )->ErrVERRCEClean( ifmp );
        if ( errVerClean != JET_errSuccess )
        {
            AssertTrack( fFalse, OSFormat( "ShrinkEofEndVerCleanErr:%d", errVerClean ) );

            // Only clobber the error if it's not a corruption or it's success.
            const ErrData* perrdata = NULL;
            if ( ( err >= JET_errSuccess ) ||
                 ( ( perrdata = PerrdataLookupErrValue( err ) ) == NULL ) ||
                 ( perrdata->errorCategory != JET_errcatCorruption ) )
            {
                err = ErrERRCheck( JET_errDatabaseInUse );
            }
        }
        else
        {
            pfmp->WaitForTasksToComplete();
            FCB::PurgeDatabase( ifmp, fFalse /* fTerminating */ );
        }

        // Log an attach signal so that any read-from-passive clients can re-attach the database once
        // shrink is done. The counterpart detach is triggered by LOG::ErrLGRIRedoRootPageMove(), in the
        // call to ErrLGDbDetachingCallback().
        (void)ErrLGSignalAttachDB( ifmp );
    }

    pfmp->ResetPgnoShrinkTarget();
    pfmp->ResetShrinkIsRunning();

    return err;
}


// Root page move functions.
//

LOCAL VOID SHKIRootMoveRevertDbTime( ROOTMOVE* const prm )
{
    // Root, OE, AE.
    if ( prm->csrFDP.Latch() == latchWrite )
    {
        prm->csrFDP.RevertDbtime( prm->dbtimeBeforeFDP, prm->fFlagsBeforeFDP );
    }
    if ( prm->csrOE.Latch() == latchWrite )
    {
        prm->csrOE.RevertDbtime( prm->dbtimeBeforeOE, prm->fFlagsBeforeOE );
    }
    if ( prm->csrAE.Latch() == latchWrite )
    {
        prm->csrAE.RevertDbtime( prm->dbtimeBeforeAE, prm->fFlagsBeforeAE );
    }

    // Children objects.
    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        if ( prmc->csrChildFDP.Latch() == latchWrite )
        {
            prmc->csrChildFDP.RevertDbtime( prmc->dbtimeBeforeChildFDP, prmc->fFlagsBeforeChildFDP );
        }
    }

    // Catalog pages.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        if ( prm->csrCatObj[iCat].Latch() == latchWrite )
        {
            prm->csrCatObj[iCat].RevertDbtime( prm->dbtimeBeforeCatObj[iCat], prm->fFlagsBeforeCatObj[iCat] );
        }

        if ( prm->csrCatClustIdx[iCat].Latch() == latchWrite )
        {
            prm->csrCatClustIdx[iCat].RevertDbtime( prm->dbtimeBeforeCatClustIdx[iCat], prm->fFlagsBeforeCatClustIdx[iCat] );
        }
    }
}

LOCAL VOID SHKIRootMoveGetPagePreImage( const IFMP ifmp, const PGNO pgno, CSR* const pcsr, DATA* const pdata )
{
    CPAGE cpage;
    void* pv = NULL;

    Assert( pcsr->Cpage().CbBuffer() == (ULONG)g_rgfmp[ ifmp ].CbPage() );
    Assert( pcsr->Cpage().CbPage() == (ULONG)g_rgfmp[ ifmp ].CbPage() );

    BFAlloc( bfasTemporary, &pv, g_rgfmp[ ifmp ].CbPage() );
    pdata->SetPv( pv );
    pdata->SetCb( g_rgfmp[ ifmp ].CbPage() );
    UtilMemCpy( pdata->Pv(), pcsr->Cpage().PvBuffer(), g_rgfmp[ ifmp ].CbPage() );
}

LOCAL VOID SHKIRootMoveGetCatNodePreImage( const CPAGE& cpage, const INT iline, DATA* const pdata )
{
    KEYDATAFLAGS kdf;

    NDIGetKeydataflags( cpage, iline, &kdf );
    pdata->SetPv( kdf.data.Pv() );
    pdata->SetCb( kdf.data.Cb() );
}

LOCAL ERR ErrSHKIRootMoveSetCatRecPostImage(
    FUCB* const pfucbCatalog,
    DATA* const pdataPreImage,
    DATA* const pdataPostImage,
    const PGNO pgnoNewFDP )
{
    VOID* pv = NULL;
    BFAlloc( bfasTemporary, &pv, pdataPreImage->Cb() );
    pdataPostImage->SetPv( pv );
    pdataPreImage->CopyInto( *pdataPostImage );

    DATA dataField;
    dataField.SetPv( (void*)( &pgnoNewFDP ) );
    dataField.SetCb( sizeof( pgnoNewFDP ) );

    return ErrRECISetFixedColumnInLoadedDataBuffer(
                pdataPostImage,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_PgnoFDP,
                &dataField );
}

LOCAL ERR ErrSHKICreateRootMove( ROOTMOVE* const prm, FUCB* const pfucb, const OBJID objidTable )
{
    ERR err = JET_errSuccess;
    FCB* const pfcb = pfucb->u.pfcb;
    FUCB* pfucbCatalog = pfucbNil;
    PIB* const ppib = pfucb->ppib;
    const IFMP ifmp = pfucb->ifmp;
    const OBJID objid = pfcb->ObjidFDP();
    const BOOL fRootObject = ( objid == objidTable );
    BOOL fCatalogLatched = fFalse;

    Assert( prm != NULL );

    // Initialize basic move struct members.
    prm->objid = objid;
    prm->pgnoFDP = pfcb->PgnoFDP();
    prm->pgnoOE = pfcb->PgnoOE();
    prm->pgnoAE = pfcb->PgnoAE();
    Assert( prm->pgnoFDP != pgnoNull );
    Assert( prm->pgnoOE != pgnoNull );
    Assert( prm->pgnoAE != pgnoNull );
    Assert( prm->pgnoAE == ( prm->pgnoOE + 1 ) );

    // If this is a root object, latch each of the children's root pages because
    // we'll need to update their external headers with the new pgnoParent.
    //

    if ( fRootObject )
    {
        // Go through each child object.
        Assert( pfucbCatalog == pfucbNil );
        OBJID objidChild = objidNil;
        SYSOBJ sysobjChild = sysobjNil;
        for ( err = ErrCATGetNextNonRootObject( ppib, ifmp, objidTable, &pfucbCatalog, &objidChild, &sysobjChild );
                ( err >= JET_errSuccess ) && ( objidChild != objidNil );
                err = ErrCATGetNextNonRootObject( ppib, ifmp, objidTable, &pfucbCatalog, &objidChild, &sysobjChild ) )
        {
            // Get child's pgnoFDP.
            PGNO pgnoFDPChild = pgnoNull;
            Call( ErrCATSeekObjectByObjid( ppib, ifmp, objidTable, sysobjChild, objidChild, NULL, 0, &pgnoFDPChild ) );

            // Allocate and set up child root move object.
            ROOTMOVECHILD* const prmc = new ROOTMOVECHILD;
            Alloc( prmc );
            prm->AddRootMoveChild( prmc );

            Call( prmc->csrChildFDP.ErrGetRIWPage( ppib, ifmp, pgnoFDPChild ) );

            prmc->dbtimeBeforeChildFDP = prmc->csrChildFDP.Cpage().Dbtime();
            prmc->fFlagsBeforeChildFDP = prmc->csrChildFDP.Cpage().FFlags();
            prmc->pgnoChildFDP = pgnoFDPChild;
            prmc->objidChild = objidChild;

            LINE lineExtHdr;
            NDGetPtrExternalHeader(
                prmc->csrChildFDP.Cpage(),
                &lineExtHdr,
                noderfSpaceHeader );
            UtilMemCpy( &prmc->sphNew, lineExtHdr.pv, sizeof( prmc->sphNew ) );
        }
        Call( err );

        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }

    // Latch catalog page(s) which contain nodes describing this object.
    // Note that if this is a root object, we may have two affected nodes: the
    // first one is the table node and the second one is the clustered index,
    // which points to the same tree/pgnoFDP. Tables which do not have a primary
    // index explicitly defined will have an internal unique auto-inc key and will
    // have only one node representing the table. Otherwise, they will have two nodes
    // (thus two pgnoFDP references) that need to be fixed up.
    //

    // Retry until we get a consistent view, since we release page latches for a moment.
    // Do it for both the main and shadow catalogs.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        const BOOL fShadowCat = ( iCat == 1 );
        BOOL fFirstLatchAttempt = fTrue;

        while ( fTrue )
        {
            AssertTrack( fFirstLatchAttempt, OSFormat( "RootMoveUnexpectedRelatch:%d", (int)fShadowCat ) );

            prm->pgnoCatObj[iCat] = pgnoNull;
            prm->ilineCatObj[iCat] = -1;
            prm->csrCatObj[iCat].ReleasePage();
            prm->csrCatObj[iCat].Reset();

            prm->pgnoCatClustIdx[iCat] = pgnoNull;
            prm->ilineCatClustIdx[iCat] = -1;
            prm->csrCatClustIdx[iCat].ReleasePage();
            prm->csrCatClustIdx[iCat].Reset();

            // Go through each object with this pgnoFDP.
            Assert( pfucbCatalog == pfucbNil );
            OBJID objidT = objidNil;
            SYSOBJ sysobjT = sysobjNil;
            for ( err = ErrCATGetNextObjectByPgnoFDP( ppib, ifmp, objidTable, prm->pgnoFDP, fShadowCat, &pfucbCatalog, &objidT, &sysobjT );
                    ( err >= JET_errSuccess ) && ( objidT != objidNil );
                    err = ErrCATGetNextObjectByPgnoFDP( ppib, ifmp, objidTable, prm->pgnoFDP, fShadowCat, &pfucbCatalog, &objidT, &sysobjT ) )
            {
                Assert( fRootObject || ( ( sysobjT == sysobjIndex ) || ( sysobjT == sysobjLongValue ) ) );
                Assert( !fRootObject || ( ( sysobjT == sysobjIndex ) || ( sysobjT == sysobjTable ) ) );
                Assert( ( sysobjT != sysobjTable ) || fRootObject );
                Assert( ( sysobjT != sysobjLongValue ) || !fRootObject );
                Assert( fRootObject ? ( objidTable == objidT ) : ( objidTable != objidT ) );

                Call( ErrDIRGet( pfucbCatalog ) );
                fCatalogLatched = fTrue;

                const DBTIME dbtimeCat = pfucbCatalog->csr.Dbtime();
                const ULONG fFlags = pfucbCatalog->csr.Cpage().FFlags();
                const PGNO pgnoCat = pfucbCatalog->csr.Pgno();
                const INT ilineCat = pfucbCatalog->csr.ILine();

                if ( prm->ilineCatObj[iCat] == -1 )
                {
                    prm->dbtimeBeforeCatObj[iCat] = dbtimeCat;
                    prm->fFlagsBeforeCatObj[iCat] = fFlags;
                    prm->pgnoCatObj[iCat] = pgnoCat;
                    prm->ilineCatObj[iCat] = ilineCat;
                }
                else if ( prm->ilineCatClustIdx[iCat] == -1 )
                {
                    if ( !fRootObject )
                    {
                        AssertTrack( fFalse, OSFormat( "RootMoveTooManyPreCatNodesNonRoot:%d", (int)fShadowCat ) );
                        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
                    }

                    Assert( ( ilineCat != prm->ilineCatObj[iCat] ) || ( pgnoCat != prm->pgnoCatObj[iCat] ) );
                    prm->dbtimeBeforeCatClustIdx[iCat] = dbtimeCat;
                    prm->fFlagsBeforeCatClustIdx[iCat] = fFlags;
                    prm->pgnoCatClustIdx[iCat] = pgnoCat;
                    prm->ilineCatClustIdx[iCat] = ilineCat;
                }
                else
                {
                    AssertTrack( fFalse, OSFormat( "RootMoveTooManyPreCatNodes:%d", (int)fShadowCat ) );
                    Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
                }

                CallS( ErrDIRRelease( pfucbCatalog ) );
                fCatalogLatched = fFalse;
            }
            Call( err );

            // Close catalog cursor.
            CallS( ErrCATClose( ppib, pfucbCatalog ) );
            pfucbCatalog = pfucbNil;

            if ( prm->ilineCatObj[iCat] == -1 )
            {
                AssertTrack( fFalse, OSFormat( "RootMoveNoPreCatNodes:%d", (int)fShadowCat ) );
                Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }

            fFirstLatchAttempt = fFalse;

            // Now, latch the pages and check if their DB times changed.
            Call( prm->csrCatObj[iCat].ErrGetRIWPage( ppib, ifmp, prm->pgnoCatObj[iCat] ) );
            if ( prm->csrCatObj[iCat].Dbtime() != prm->dbtimeBeforeCatObj[iCat] )
            {
                continue;
            }

            // Check if it's a valid and different page before latching.
            if ( ( prm->pgnoCatClustIdx[iCat] != pgnoNull ) && ( prm->pgnoCatClustIdx[iCat] != prm->pgnoCatObj[iCat] ) )
            {
                Call( prm->csrCatClustIdx[iCat].ErrGetRIWPage( ppib, ifmp, prm->pgnoCatClustIdx[iCat] ) );
                if ( prm->csrCatClustIdx[iCat].Dbtime() != prm->dbtimeBeforeCatClustIdx[iCat] )
                {
                    continue;
                }
            }

            Assert( ( prm->pgnoCatClustIdx[iCat] != prm->pgnoCatObj[iCat] ) || ( prm->dbtimeBeforeCatClustIdx[iCat] == prm->dbtimeBeforeCatObj[iCat] ) );
            Assert( !!prm->csrCatClustIdx[iCat].FLatched() ==
                    ( ( prm->ilineCatClustIdx[iCat] != -1 ) && ( prm->pgnoCatClustIdx[iCat] != prm->pgnoCatObj[iCat] ) ) );

            break;
        }
    }

    // Verify consistency between main and shadow catalogs.
    Assert( prm->pgnoCatObj[0] != prm->pgnoCatObj[1] );
    Assert( ( ( prm->pgnoCatClustIdx[0] == pgnoNull ) && ( prm->pgnoCatClustIdx[1] == pgnoNull ) ) ||
            ( prm->pgnoCatClustIdx[0] != prm->pgnoCatClustIdx[1] ) );

    // Allocate 3 new contiguous pages (root + OE + AE).
    //

    // Alocate pages.
    CPG cpgReq = 3;
    PGNO pgnoNewFDP = pgnoNull;
    Call( ErrSPGetExt( pfucb, prm->pgnoFDP, &cpgReq, cpgReq, &pgnoNewFDP ) );
    Assert( cpgReq >= 3 );
    prm->pgnoNewFDP = pgnoNewFDP;
    prm->pgnoNewOE = pgnoNewFDP + 1;
    prm->pgnoNewAE = pgnoNewFDP + 2;

    // Latch root, OA and AE pages (current and new).
    //

    // Latch new pages.
    const BOOL fLogging = g_rgfmp[ifmp].FLogOn();
    Call( prm->csrNewFDP.ErrGetNewPreInitPage(
                                ppib,
                                ifmp,
                                prm->pgnoNewFDP,
                                objid,
                                fLogging ) );
    Call( prm->csrNewOE.ErrGetNewPreInitPage(
                                ppib,
                                ifmp,
                                prm->pgnoNewOE,
                                objid,
                                fLogging ) );
    Call( prm->csrNewAE.ErrGetNewPreInitPage(
                                ppib,
                                ifmp,
                                prm->pgnoNewAE,
                                objid,
                                fLogging ) );

    // Latch old pages.
    Call( prm->csrFDP.ErrGetRIWPage( ppib, ifmp, prm->pgnoFDP ) );
    prm->dbtimeBeforeFDP = prm->csrFDP.Dbtime();
    prm->fFlagsBeforeFDP = prm->csrFDP.Cpage().FFlags();
    Call( prm->csrOE.ErrGetRIWPage( ppib, ifmp, prm->pgnoOE ) );
    prm->dbtimeBeforeOE = prm->csrOE.Dbtime();
    prm->fFlagsBeforeOE = prm->csrOE.Cpage().FFlags();
    Call( prm->csrAE.ErrGetRIWPage( ppib, ifmp, prm->pgnoAE ) );
    prm->dbtimeBeforeAE = prm->csrAE.Dbtime();
    prm->fFlagsBeforeAE = prm->csrAE.Cpage().FFlags();

    // Get the dbtime of the overall operation.
    prm->dbtimeAfter = g_rgfmp[ifmp].DbtimeIncrementAndGet();

HandleError:
    if ( fCatalogLatched )
    {
        Expected( err < JET_errSuccess );
        Assert( pfucbCatalog != pfucbNil );
        CallS( ErrDIRRelease( pfucbCatalog ) );
        fCatalogLatched = fFalse;
    }

    if ( pfucbCatalog != pfucbNil )
    {
        Expected( err < JET_errSuccess );
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }

    if ( err < JET_errSuccess )
    {
        SHKIRootMoveRevertDbTime( prm );
        prm->ReleaseResources();
    }

    return err;
}

LOCAL ERR ErrSHKIRootMoveUpgradeLatches( ROOTMOVE* const prm, PIB* const ppib, const IFMP ifmp )
{
    ERR err = JET_errSuccess;
    const DBTIME dbtimeAfter = prm->dbtimeAfter;
    FUCB* pfucbCatalog = pfucbNil;

    // Upgrade all latches to write.
    //

    // Root, OE, AE.
    prm->csrFDP.UpgradeFromRIWLatch();
    prm->csrOE.UpgradeFromRIWLatch();
    prm->csrAE.UpgradeFromRIWLatch();
    Assert( prm->csrNewFDP.Latch() == latchWrite );
    Assert( prm->csrNewOE.Latch() == latchWrite );
    Assert( prm->csrNewAE.Latch() == latchWrite );

    // Children objects.
    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        prmc->csrChildFDP.UpgradeFromRIWLatch();
    }

    // Catalog pages.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        Assert( prm->csrCatObj[iCat].FLatched() );
        Assert( prm->pgnoCatObj[iCat] != pgnoNull );
        Assert( prm->ilineCatObj[iCat] != -1 );
        Assert( ( prm->csrCatClustIdx[iCat].FLatched() &&                       // Clustered index node in a different page...
                  ( prm->pgnoCatClustIdx[iCat] != prm->pgnoCatObj[iCat] ) &&
                  ( prm->ilineCatClustIdx[iCat] != -1 ) ) ||
                ( !prm->csrCatClustIdx[iCat].FLatched() &&                       // ... or clustered index node in the same page...
                  ( prm->pgnoCatClustIdx[iCat] == prm->pgnoCatObj[iCat] ) &&
                  ( prm->ilineCatClustIdx[iCat] != -1 ) ) ||
                ( !prm->csrCatClustIdx[iCat].FLatched() &&                       // ... or no clustered index node.
                  ( prm->pgnoCatClustIdx[iCat] == pgnoNull ) &&
                  ( prm->ilineCatClustIdx[iCat] == -1 ) ) );

        prm->csrCatObj[iCat].UpgradeFromRIWLatch();
        if ( prm->csrCatClustIdx[iCat].FLatched() )
        {
            prm->csrCatClustIdx[iCat].UpgradeFromRIWLatch();
        }
    }

    // Prepare pre/post-images and data blobs.
    //

    // Root, OE, AE (pre-image).
    SHKIRootMoveGetPagePreImage( ifmp, prm->pgnoFDP, &prm->csrFDP, &prm->dataBeforeFDP );
    SHKIRootMoveGetPagePreImage( ifmp, prm->pgnoOE, &prm->csrOE, &prm->dataBeforeOE );
    SHKIRootMoveGetPagePreImage( ifmp, prm->pgnoAE, &prm->csrAE, &prm->dataBeforeAE );

    // Catalog pages (pre-images).
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        SHKIRootMoveGetCatNodePreImage(
            prm->csrCatObj[iCat].Cpage(),
            prm->ilineCatObj[iCat],
            &prm->dataBeforeCatObj[iCat] );

        if ( prm->ilineCatClustIdx[iCat] != -1 )
        {
            SHKIRootMoveGetCatNodePreImage(
                prm->csrCatClustIdx[iCat].FLatched() ?
                    prm->csrCatClustIdx[iCat].Cpage() :
                    prm->csrCatObj[iCat].Cpage(),
                prm->ilineCatClustIdx[iCat],
                &prm->dataBeforeCatClustIdx[iCat] );
        }
    }

    // Children objects (post-image).
    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        prmc->sphNew.SetPgnoParent( prm->pgnoNewFDP );
    }

    // Catalog pages (post-images).
    Assert( pfucbCatalog == pfucbNil );
    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        Call( ErrSHKIRootMoveSetCatRecPostImage(
                pfucbCatalog,
                &prm->dataBeforeCatObj[iCat],
                &prm->dataNewCatObj[iCat],
                prm->pgnoNewFDP ) );
        if ( prm->ilineCatClustIdx[iCat] != -1 )
        {
            Call( ErrSHKIRootMoveSetCatRecPostImage(
                    pfucbCatalog,
                    &prm->dataBeforeCatClustIdx[iCat],
                    &prm->dataNewCatClustIdx[iCat],
                    prm->pgnoNewFDP ) );
        }
    }
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    // Dirty pages.
    //

    // Root, OE, AE.
    Assert( prm->csrFDP.Dbtime() == prm->dbtimeBeforeFDP );
    Assert( prm->csrOE.Dbtime() == prm->dbtimeBeforeOE );
    Assert( prm->csrAE.Dbtime() == prm->dbtimeBeforeAE );
    prm->csrFDP.CoordinatedDirty( dbtimeAfter );
    prm->csrOE.CoordinatedDirty( dbtimeAfter );
    prm->csrAE.CoordinatedDirty( dbtimeAfter );
    prm->csrNewFDP.CoordinatedDirty( dbtimeAfter );
    prm->csrNewOE.CoordinatedDirty( dbtimeAfter );
    prm->csrNewAE.CoordinatedDirty( dbtimeAfter );

    // Children objects.
    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        Assert( prmc->csrChildFDP.Dbtime() == prmc->dbtimeBeforeChildFDP );
        prmc->csrChildFDP.CoordinatedDirty( dbtimeAfter );
    }

    // Catalog pages.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        Assert( prm->csrCatObj[iCat].Dbtime() == prm->dbtimeBeforeCatObj[iCat] );
        prm->csrCatObj[iCat].CoordinatedDirty( dbtimeAfter );
        if ( prm->csrCatClustIdx[iCat].FLatched() )
        {
            Assert( prm->csrCatClustIdx[iCat].Dbtime() == prm->dbtimeBeforeCatClustIdx[iCat] );
            prm->csrCatClustIdx[iCat].CoordinatedDirty( dbtimeAfter );
        }
    }

HandleError:
    if ( pfucbCatalog != pfucbNil )
    {
        Expected( err < JET_errSuccess );
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }

    return err;
}

LOCAL VOID SHKIRootMoveSetLgposModify( ROOTMOVE* const prm, const LGPOS &lgpos )
{
    // Root, OE, AE.
    prm->csrFDP.Cpage().SetLgposModify( lgpos );
    prm->csrOE.Cpage().SetLgposModify( lgpos );
    prm->csrAE.Cpage().SetLgposModify( lgpos );

    // New root, OE, AE.
    prm->csrNewFDP.Cpage().SetLgposModify( lgpos );
    prm->csrNewOE.Cpage().SetLgposModify( lgpos );
    prm->csrNewAE.Cpage().SetLgposModify( lgpos );

    // Children objects.
    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        prmc->csrChildFDP.Cpage().SetLgposModify( lgpos );
    }

    // Catalog pages.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        prm->csrCatObj[iCat].Cpage().SetLgposModify( lgpos );

        if ( prm->csrCatClustIdx[iCat].FLatched() )
        {
            Assert( prm->csrCatClustIdx[iCat].Latch() == latchWrite );
            prm->csrCatClustIdx[iCat].Cpage().SetLgposModify( lgpos );
        }
    }
}

LOCAL VOID SHKIRootMoveCopyToNewPage(
    CSR* const pcsr,
    const DATA& dataOld,
    const PGNO pgnoNew,
    const DBTIME dbtimeAfter,
    const BOOL fRecoveryRedo )
{
    Assert( pcsr->FDirty() );
    if ( fRecoveryRedo )
    {
        pcsr->SetDbtime( dbtimeAfter );
    }

    // Copy image to destination page.
    UtilMemCpy(
        pcsr->Cpage().PvBuffer(),
        dataOld.Pv(),
        dataOld.Cb() );

    // Fix up pgno and dbtime in the destination page.
    pcsr->Cpage().SetPgno( pgnoNew );
    pcsr->Cpage().SetDbtime( dbtimeAfter );
}

LOCAL VOID SHKIRootMoveReleaseLatches( ROOTMOVE* const prm )
{
    // Root, OE, AE.
    prm->csrFDP.ReleasePage();
    prm->csrOE.ReleasePage();
    prm->csrAE.ReleasePage();
    prm->csrNewFDP.ReleasePage();
    prm->csrNewOE.ReleasePage();
    prm->csrNewAE.ReleasePage();

    // Children objects.
    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        prmc->csrChildFDP.ReleasePage();
    }

    // Catalog pages.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        prm->csrCatObj[iCat].ReleasePage();
        prm->csrCatClustIdx[iCat].ReleasePage();
    }
}

LOCAL ERR ErrSHKIRootMoveCheck( const ROOTMOVE& rm, FUCB* const pfucb, const OBJID objidTable )
{
    ERR err = JET_errSuccess;
    FUCB* pfucbCatalog = pfucbNil;
    FUCB* pfucbChild = pfucbNil;
    PIB* const ppib = pfucb->ppib;
    const IFMP ifmp = pfucb->ifmp;
    const FCB* const pfcb = pfucb->u.pfcb;
    const BOOL fRootObject = ( rm.objid == objidTable );

    // Check root.
    if ( pfcb->PgnoFDP() != rm.pgnoNewFDP )
    {
        AssertTrack( fFalse, "RootMoveBadPgnoNewFdp" );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    // Check OE.
    if ( pfcb->PgnoOE() != rm.pgnoNewOE )
    {
        AssertTrack( fFalse, "RootMoveBadPgnoNewOE" );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    // Check AE.
    if ( pfcb->PgnoAE() != rm.pgnoNewAE )
    {
        AssertTrack( fFalse, "RootMoveBadPgnoNewAE" );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    // Check children objects.
    if ( fRootObject )
    {
        ROOTMOVECHILD* prmc = rm.prootMoveChildren;

        // Go through each child object (we expect them to show up in the same order they
        // have been inserted into the list earlier).
        Assert( pfucbCatalog == pfucbNil );
        OBJID objidChild = objidNil;
        SYSOBJ sysobjChild = sysobjNil;
        for ( err = ErrCATGetNextNonRootObject( ppib, ifmp, objidTable, &pfucbCatalog, &objidChild, &sysobjChild );
                ( err >= JET_errSuccess ) && ( objidChild != objidNil );
                err = ErrCATGetNextNonRootObject( ppib, ifmp, objidTable, &pfucbCatalog, &objidChild, &sysobjChild ) )
        {
            if ( prmc == NULL )
            {
                AssertTrack( fFalse, "RootMoveTooManyPostChildren" );
                Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }

            // Get child's pgnoFDP.
            PGNO pgnoFDPChild = pgnoNull;
            Call( ErrCATSeekObjectByObjid( ppib, ifmp, objidTable, sysobjChild, objidChild, NULL, 0, &pgnoFDPChild ) );

            Call( ErrBTIOpen( ppib, ifmp, pgnoFDPChild, objidNil, openNormal, &pfucbChild, fFalse ) );
            Call( ErrBTIGotoRoot( pfucbChild, latchRIW ) );
            pfucbChild->pcsrRoot = Pcsr( pfucbChild );

            // Check against the previously enumerated objected.
            if ( PgnoSPIParentFDP( pfucbChild ) != rm.pgnoNewFDP )
            {
                AssertTrack( fFalse, "RootMoveBadPgnoParentFdp" );
                Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }

            pfucbChild->pcsrRoot = pcsrNil;
            BTUp( pfucbChild );
            BTClose( pfucbChild );
            pfucbChild = pfucbNil;

            prmc = prmc->prootMoveChildNext;
        }
        Call( err );

        if ( prmc != NULL )
        {
            AssertTrack( fFalse, "RootMoveTooManyPreChildren" );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }
    else
    {
        Assert( rm.prootMoveChildren == NULL );
    }

    // Check catalogs.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        const BOOL fShadowCat = ( iCat == 1 );
        OBJID objidT = objidNil;
        SYSOBJ sysobjT = sysobjNil;

        // Check catalog (no records with the old pgnoFDP).
        Assert( pfucbCatalog == pfucbNil );
        err = ErrCATGetNextObjectByPgnoFDP( ppib, ifmp, objidTable, rm.pgnoFDP, fShadowCat, &pfucbCatalog, &objidT, &sysobjT );
        if ( ( err >= JET_errSuccess ) && ( objidT != objidNil ) )
        {
            AssertTrack( fFalse, OSFormat( "RootMoveUnexpectedOldPgnoFdpObject:%d", (int)fShadowCat ) );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        Call( err );

        // Close catalog cursor.
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;

        // Check catalog (records can be reached with the new pgnoFDP).
        objidT = objidNil;
        sysobjT = sysobjNil;
        ULONG cCatRecords = 0;
        for ( err = ErrCATGetNextObjectByPgnoFDP( ppib, ifmp, objidTable, rm.pgnoNewFDP, fShadowCat, &pfucbCatalog, &objidT, &sysobjT );
                ( err >= JET_errSuccess ) && ( objidT != objidNil );
                err = ErrCATGetNextObjectByPgnoFDP( ppib, ifmp, objidTable, rm.pgnoNewFDP, fShadowCat, &pfucbCatalog, &objidT, &sysobjT ) )
        {
            cCatRecords++;
        }
        Call( err );

        // Close catalog cursor.
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;

        // Verify consistency.
        if ( cCatRecords == 0 )
        {
            AssertTrack( fFalse, OSFormat( "RootMoveNoPostCatNodes:%d", (int)fShadowCat ) );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        else if ( cCatRecords == 1 )
        {
            if ( rm.ilineCatClustIdx[iCat] != -1 )
            {
                AssertTrack( fFalse, OSFormat( "RootMoveUnexpOnePostCatNodes:%d", (int)fShadowCat ) );
                Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
        }
        else if ( cCatRecords == 2 )
        {
            if ( rm.ilineCatClustIdx[iCat] == -1 )
            {
                AssertTrack( fFalse, OSFormat( "RootMoveUnexpTwoPostCatNodes:%d", (int)fShadowCat ) );
                Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
        }
        else
        {
            AssertTrack( fFalse, OSFormat( "RootMoveTooManyPostCatNodes:%d", (int)fShadowCat ) );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }

HandleError:
    if ( pfucbChild != pfucbNil )
    {
        Expected( err < JET_errSuccess );
        pfucbChild->pcsrRoot = pcsrNil;
        BTUp( pfucbChild );
        BTClose( pfucbChild );
        pfucbChild = pfucbNil;
    }

    if ( pfucbCatalog != pfucbNil )
    {
        Expected( err < JET_errSuccess );
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }

    return err;
}

LOCAL ERR ErrSHKIRootMoveFreeEmptiedPages( const ROOTMOVE& rm, FUCB* const pfucb )
{
    ERR err = JET_errSuccess;

    CallR( ErrSPFreeExt( pfucb, rm.pgnoFDP, 1, "RootMoveFreeSrcFdp" ) );
    CallR( ErrSPFreeExt( pfucb, rm.pgnoOE, 2, "RootMoveFreeSrcOeAe" ) );

    return err;
}

ERR ErrSHKRootPageMove(
    _In_ PIB* ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgnoFDP )
{
    ERR err = JET_errSuccess;
    FMP* const pfmp = g_rgfmp + ifmp;
    PGNO pgnoFDPParentBefore = pgnoNull;
    PGNO pgnoFDPParentAfter = pgnoNull;
    FCB* pfcb = pfcbNil;
    FUCB* pfucb = pfucbNil;
    FUCB* pfucbParent = pfucbNil;
    OBJID objid = objidNil;
    OBJID objidTable = objidNil;
    SYSOBJ sysobj = sysobjNil;
    CSR csrCatLock[2];
    PGNO pgnoCatFDP[2] = { pgnoFDPMSO, pgnoFDPMSOShadow };
    BOOL fRootObject = fFalse;
    BOOL fMoveLogged = fFalse;
    BOOL fMovePerformed = fFalse;
    BOOL fFullyInitCursor = fFalse;
    ROOTMOVE rm;

    Assert( pgnoFDP != pgnoNull );
    Assert( !FCATBaseSystemFDP( pgnoFDP ) );
    Assert( pfmp->FEfvSupported( JET_efvRootPageMove ) );
    Assert( pfmp->FExclusiveBySession( ppib ) );
    Expected( pfmp->FShrinkIsActive() );

    // Make sure version store is clean, just in case.
    const ERR errVerClean = PverFromPpib( ppib )->ErrVERRCEClean( ifmp );
    if ( errVerClean != JET_errSuccess )
    {
        AssertTrack( fFalse, OSFormat( "RootPageMoveVerCleanErr:%d", errVerClean ) );
        Error( ErrERRCheck( JET_errDatabaseInUse ) );
    }
    pfmp->WaitForTasksToComplete();

    // Open a cursor to the tree we want to move the root of and
    // initialize helper variables.
    //

    // Retrieve some metadata first by opening it at the BT level.
    Call( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb ) );
    pfcb = pfucb->u.pfcb;

    // Determine whether or not this is a root object.
    objid = pfcb->ObjidFDP();
    Call( ErrCATGetObjidMetadata( ppib, ifmp, objid, &objidTable, &sysobj ) );
    fRootObject = ( sysobj == sysobjTable );
    Assert( !!fRootObject == ( objid == objidTable ) );

    // Close primitive cursor.
    BTClose( pfucb );
    pfucb = pfucbNil;
    pfcb = pfcbNil;

    // Open/initialize full-fledged cursor.
    Assert( objid != objidSystemRoot );
    PGNO pgnoFDPT = pgnoNull;
    const OBJID objidParent = fRootObject ? objidSystemRoot : objidTable;
    Call( ErrCATGetCursorsFromObjid(
            ppib,
            ifmp,
            objid,
            objidParent,
            sysobj,
            &pgnoFDPT,
            &pgnoFDPParentBefore,
            &pfucb,
            &pfucbParent ) );
    fFullyInitCursor = fTrue;
    pfcb = pfucb->u.pfcb;
    Assert( pgnoFDP == pgnoFDPT );
    Assert( pgnoFDP == pfcb->PgnoFDP() );

    // Prepare latches and metadata.
    //

    // Build up ROOTMOVE struct with the metadata and latches necessary.
    Call( ErrSHKICreateRootMove( &rm, pfucb, objidTable ) );

    // Upgrade all latches to write latches (new pages are already write-latched).
    Call( ErrSHKIRootMoveUpgradeLatches( &rm, ppib, ifmp ) );
    OnDebug( rm.AssertValid( fTrue /* fBeforeMove */, fFalse /* fRedo */ ) );

    // Purge FCBs.
    //

    // Lock catalogs.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        Assert( rm.pgnoCatObj[iCat] != pgnoNull );
        Assert( ( rm.pgnoCatClustIdx[iCat] == pgnoNull ) ||             // Clustered index object does not exist.
                ( rm.pgnoCatObj[iCat] == rm.pgnoCatClustIdx[iCat] ) ||  // Main and clustered index objects represented in the same catalog page.
                ( ( pgnoCatFDP[iCat] != rm.pgnoCatObj[iCat] ) &&        // Main and clustered index objects represented in different catalog pages,
                  ( pgnoCatFDP[iCat] != rm.pgnoCatClustIdx[iCat] ) ) ); // which means the pages can't be the catalog root.

        if ( ( pgnoCatFDP[iCat] != rm.pgnoCatObj[iCat] ) &&
                ( pgnoCatFDP[iCat] != rm.pgnoCatClustIdx[iCat] ) )
        {
            // WARNING: We are latching the root of catalog after we have leaf catalog pages latched,
            // so this is a "page latching" sub-rank / order violation. We believe this to be OK because
            // DB Shrink operates in locked down / "single-threaded" environment, so there shouldn't be
            // anyone trying to lock the catalog tree from to bottom ("correct" order) for a split or merge,
            // for example.
            Call( csrCatLock[iCat].ErrGetRIWPage( ppib, ifmp, pgnoCatFDP[iCat] ) );
            csrCatLock[iCat].UpgradeFromRIWLatch();
        }
    }

    // Purge FCBs of the object and its children (done hierarchically under FCB::PurgeObject()).
    CATFreeCursorsFromObjid( pfucb, pfucbParent );
    fFullyInitCursor = fFalse;
    pfucb = pfucbNil;
    pfucbParent = pfucbNil;
    pfcb = pfcbNil;
    FCB::PurgeObject( ifmp, fRootObject ? pgnoFDP : pgnoFDPParentBefore );

    // Purge catalog FCBs. We do this so that we can make unversioned/unlogged
    // updates without worrying about pending version store updates.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        FCB::PurgeObject( ifmp, pgnoCatFDP[iCat] );
    }

    // Log the operation and set lgposModify for all affected pages.
    //

    if ( pfmp->FLogOn() )
    {
        LGPOS lgpos = lgposMin;
        Call( ErrLGRootPageMove( ppib, ifmp, &rm, &lgpos ) );
        OnDebug( rm.AssertValid( fTrue /* fBeforeMove */, fFalse /* fRedo */ ) );

        SHKIRootMoveSetLgposModify( &rm, lgpos );
    }

    // WARNING: we can't fail between fMoveLogged = fTrue and fMovePerformed = fTrue.

    fMoveLogged = fTrue;

    // Perform the actual operation and check consistency of the final state.
    //

    // Perform the operation.
    SHKPerformRootMove( &rm, ppib, ifmp, fFalse /* fRecoveryRedo */ );
    OnDebug( rm.AssertValid( fFalse /* fBeforeMove */, fFalse /* fRedo */ ) );

    fMovePerformed = fTrue;

    // Release latches
    SHKIRootMoveReleaseLatches( &rm );

    // Release catalog lock.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        csrCatLock[iCat].ReleasePage();
    }

    // Re-open cursors and verify that the move looks consistent.
    // It's OK to fail from this point on, but it might lead to leaked space.
    //

    Call( ErrCATGetCursorsFromObjid(
            ppib,
            ifmp,
            objid,
            objidParent,
            sysobj,
            &pgnoFDPT,
            &pgnoFDPParentAfter,
            &pfucb,
            &pfucbParent ) );
    fFullyInitCursor = fTrue;
    pfcb = pfucb->u.pfcb;
    Assert( pgnoFDPT == rm.pgnoNewFDP );
    Assert( pgnoFDPParentAfter == pgnoFDPParentBefore );

    // Parent's root must not have changed.
    if ( pfucbParent->u.pfcb->PgnoFDP() != pgnoFDPParentBefore )
    {
        AssertTrack( fFalse, "RootMoveBadPgnoNewFdp" );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    // Check overall consistency.
    Call( ErrSHKIRootMoveCheck( rm, pfucb, objidTable ) );

    // Free empty pages (space allocation).
    //

    Call( ErrFaultInjection( 45866 ) );
    Call( ErrSHKIRootMoveFreeEmptiedPages( rm, pfucb ) );

    // Close cursors.
    //

    CATFreeCursorsFromObjid( pfucb, pfucbParent );
    fFullyInitCursor = fFalse;
    pfucb = pfucbNil;
    pfucbParent = pfucbNil;
    pfcb = pfcbNil;

    // Release resources/latches.
    //
    rm.ReleaseResources();

    err = JET_errSuccess;

HandleError:
    Assert( fMoveLogged || ( err < JET_errSuccess ) );
    Assert( fMovePerformed || ( err < JET_errSuccess ) );
    Expected( !fFullyInitCursor || ( err < JET_errSuccess ) );

    // Revert DB times in case of failure.
    if ( !fMoveLogged )
    {
        SHKIRootMoveRevertDbTime( &rm );
    }

    // Release catalog lock.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        Assert( ( csrCatLock[iCat].Latch() == latchNone ) || ( err < JET_errSuccess ) );
        csrCatLock[iCat].ReleasePage();
    }

    // Release resources/latches. If we've succeeded, this should be a no-op because
    // we already called it above in the success path.
    rm.ReleaseResources();

    // We can't guarantee consistency if we fail after logging succeeds, but
    // before the move is completed,
    if ( ( err < JET_errSuccess ) && fMoveLogged && !fMovePerformed )
    {
        AssertTrack( fFalse, OSFormat( "RootMoveUnexpectedErr:%d", err ) );
        err = ErrERRCheck( JET_errDatabaseCorrupted );
    }

    Assert( pfmp->FExclusiveBySession( ppib ) );

    if ( pfucb != pfucbNil )
    {
        Expected( err < JET_errSuccess );
        if ( fFullyInitCursor )
        {
            Assert( pfucbParent != pfucbNil );
            CATFreeCursorsFromObjid( pfucb, pfucbParent );
        }
        else
        {
            Assert( pfucbParent == pfucbNil );
            BTUp( pfucb );
            BTClose( pfucb );
        }

        pfucb = pfucbNil;
        pfucbParent = pfucbNil;
        pfcb = pfcbNil;
    }

    return err;
}

VOID SHKPerformRootMove(
    _In_ ROOTMOVE* const prm,
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const BOOL fRecoveryRedo )
{
    // Copy root to destination.
    if ( FBTIUpdatablePage( prm->csrNewFDP ) )
    {
        SHKIRootMoveCopyToNewPage(
            &prm->csrNewFDP,
            prm->dataBeforeFDP,
            prm->pgnoNewFDP,
            prm->dbtimeAfter,
            fRecoveryRedo );

         prm->csrNewFDP.FinalizePreInitPage();

        // Fix up external header in the destination page.
        LINE lineExtHdr;
        NDGetPtrExternalHeader( prm->csrNewFDP.Cpage(), &lineExtHdr, noderfSpaceHeader );
        SPACE_HEADER* const psph = (SPACE_HEADER*)( lineExtHdr.pv );
        psph->SetPgnoOE( prm->pgnoNewOE );
    }

    // Copy OE to destination.
    if ( FBTIUpdatablePage( prm->csrNewOE ) )
    {
        SHKIRootMoveCopyToNewPage(
            &prm->csrNewOE,
            prm->dataBeforeOE,
            prm->pgnoNewOE,
            prm->dbtimeAfter,
            fRecoveryRedo );

        prm->csrNewOE.FinalizePreInitPage();
    }

    // Copy AE to destination.
    if ( FBTIUpdatablePage( prm->csrNewAE ) )
    {
        SHKIRootMoveCopyToNewPage(
            &prm->csrNewAE,
            prm->dataBeforeAE,
            prm->pgnoNewAE,
            prm->dbtimeAfter,
            fRecoveryRedo );

        prm->csrNewAE.FinalizePreInitPage();
    }

    // Change children objects to point to new root.
    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        if ( FBTIUpdatablePage( prmc->csrChildFDP ) )
        {
            if ( fRecoveryRedo )
            {
                prmc->csrChildFDP.Dirty();
                prmc->csrChildFDP.SetDbtime( prm->dbtimeAfter );
            }
            else
            {
                Assert( prmc->csrChildFDP.FDirty() );
            }

            NDSetExternalHeader(
                ifmp,
                &prmc->csrChildFDP,
                noderfSpaceHeader,
                &prmc->dataSphNew );
        }
    }

    // Change catalog pages to point to new root.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        if ( FBTIUpdatablePage( prm->csrCatObj[iCat] ) )
        {
            if ( fRecoveryRedo )
            {
                prm->csrCatObj[iCat].Dirty();
                prm->csrCatObj[iCat].SetDbtime( prm->dbtimeAfter );
            }
            else
            {
                Assert( prm->csrCatObj[iCat].FDirty() );
            }

            Assert( prm->ilineCatObj[iCat] >= 0 );
            prm->csrCatObj[iCat].SetILine( prm->ilineCatObj[iCat] );
            NDReplace( &prm->csrCatObj[iCat], &prm->dataNewCatObj[iCat] );
            prm->csrCatObj[iCat].SetILine( 0 );
        }

        if ( prm->ilineCatClustIdx[iCat] != -1 )
        {
            if ( ( prm->pgnoCatClustIdx[iCat] == prm->pgnoCatObj[iCat] ) &&
                 FBTIUpdatablePage( prm->csrCatObj[iCat] ) )
            {
                prm->csrCatObj[iCat].SetILine( prm->ilineCatClustIdx[iCat] );
                NDReplace( &prm->csrCatObj[iCat], &prm->dataNewCatClustIdx[iCat] );
                prm->csrCatObj[iCat].SetILine( 0 );
            }
            else if ( ( prm->pgnoCatClustIdx[iCat] != prm->pgnoCatObj[iCat] ) &&
                      FBTIUpdatablePage( prm->csrCatClustIdx[iCat] ) )
            {
                if ( fRecoveryRedo )
                {
                    prm->csrCatClustIdx[iCat].Dirty();
                    prm->csrCatClustIdx[iCat].SetDbtime( prm->dbtimeAfter );
                }
                else
                {
                    Assert( prm->csrCatClustIdx[iCat].FDirty() );
                }

                Assert( prm->ilineCatClustIdx[iCat] >= 0 );
                prm->csrCatClustIdx[iCat].SetILine( prm->ilineCatClustIdx[iCat] );
                NDReplace( &prm->csrCatClustIdx[iCat], &prm->dataNewCatClustIdx[iCat] );
                prm->csrCatClustIdx[iCat].SetILine( 0 );
            }
        }
    }

    // Empty old root.
    if ( FBTIUpdatablePage( prm->csrFDP ) )
    {
        // Empty Source page.
        NDEmptyPage( &prm->csrFDP );
    }

    // Empty old OE.
    if ( FBTIUpdatablePage( prm->csrOE ) )
    {
        // Empty Source page.
        NDEmptyPage( &prm->csrOE );
    }

    // Empty old AE.
    if ( FBTIUpdatablePage( prm->csrAE ) )
    {
        // Empty Source page.
        NDEmptyPage( &prm->csrAE );
    }
}

