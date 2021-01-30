// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "errdata.hxx"
#include "_bt.hxx"

#include "PageSizeClean.hxx"


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

    Call( ErrSPGetLastExtent( ppib, ifmp, &eiLastOE ) );
    *pgnoFirstFromLastExtentMoved = eiLastOE.PgnoFirst();


    PGNO pgnoCurrent = eiLastOE.PgnoFirst();

    if ( pgnoCurrent == pgnoSystemRoot )
    {
        *psdr = sdrNoLowAvailSpace;
        *ppgnoLastProcessed = pgnoSystemRoot;
        *pspcatfLastProcessed = spcatfRoot;
        goto HandleError;
    }

    pfmp->SetPgnoShrinkTarget( pgnoCurrent - 1 );
    Assert( pgnoCurrent != pgnoNull );

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

            if ( FCATBaseSystemFDP( pgnoCurrent ) ||
                 ( pgnoCurrent == ( pgnoSystemRoot + 1 ) ) ||
                 ( pgnoCurrent == ( pgnoSystemRoot + 2 ) ) )
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

            if ( !pfmp->FShrinkIsActive() )
            {
                Error( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ) );
            }

            if ( !fInTransaction )
            {
                Assert( !fMovedPage );

                Call( ErrDIRBeginTransaction( ppib, 53898, NO_GRBIT ) );
                fInTransaction = fTrue;
            }

            Assert( fInTransaction && !fMovedPage );

            if ( ( ( dtickQuota >= 0 ) && ( CmsecHRTFromHrtStart( hrtStarted ) >= (QWORD)dtickQuota ) ) ||
                 ( pgnoCurrent == UlConfigOverrideInjection( 43982, pgnoNull ) ) )
            {
                *psdr = sdrTimeout;
                goto HandleError;
            }

            if ( ( pgnoCurrent >= pgnoPrereadWaypoint ) && ( pgnoPrereadNext <= pgnoLast ) )
            {
                const CPG cpgPrereadCurrent = LFunctionalMin( cpgPrereadMax, (CPG)( pgnoLast - pgnoPrereadNext + 1 ) );
                Assert( cpgPrereadCurrent >= 1 );
                BFPrereadPageRange( ifmp, pgnoPrereadNext, cpgPrereadCurrent, bfprfDefault, ppib->BfpriPriority( ifmp ), *tcScope );
                pgnoPrereadNext += cpgPrereadCurrent;
                pgnoPrereadWaypoint = pgnoPrereadNext - ( cpgPrereadCurrent / 2 );
            }

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
            Assert( !FSPSpaceCatUnknown( spcatfCurrent ) );
            Assert( !FSPSpaceCatNotOwnedEof( spcatfCurrent ) );
            *pdhrtPageCategorization += DhrtHRTElapsedFromHrtStart( hrtPageCategorizationStart );
            fPageCategorization = fFalse;

            if ( !FSPValidSpaceCategoryFlags( spcatfCurrent )   ||
                    FSPSpaceCatInconsistent( spcatfCurrent )    ||
                    FSPSpaceCatNotOwned( spcatfCurrent ) )
            {
                AssertTrack( fFalse, OSFormat( "ShrinkMoveUnmovable:0x%I32x", spcatfCurrent ) );
                *psdr = sdrPageNotMovable;
                goto HandleError;
            }

            if ( ( cpt == cptIndeterminateLookup ) && ( objidCurrent != objidNil ) )
            {
                Assert( pgnoIndeterminateFirst != pgnoNull );
                Assert( pgnoIndeterminateFirst < pgnoCurrent );
                Assert( pgnoPostIndeterminateRetry == pgnoNull );

                cpt = cptIndeterminateRetry;
                pgnoPostIndeterminateRetry = pgnoCurrent;
                pgnoCurrent = pgnoIndeterminateFirst;
                objidIndeterminateHint = objidCurrent;
                objidHint = objidIndeterminateHint;
                *ppgnoLastProcessed = pgnoNull;
                *pspcatfLastProcessed = spcatfNone;
                continue;
            }

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

            if ( FSPSpaceCatIndeterminate( spcatfCurrent ) || FSPSpaceCatLeaked( spcatfCurrent ) )
            {
                if ( FSPSpaceCatIndeterminate( spcatfCurrent ) )
                {
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

                    if ( pfmp->FRunShrinkDatabaseFullCatOnAttach() )
                    {
                        AssertTrack( fFalse, "ShrinkMoveUnexpectedIndeterminatePage" );
                        *psdr = sdrUnexpected;
                        goto HandleError;
                    }

                    if ( pfmp->FShrinkDatabaseDontTruncateIndeterminatePagesOnAttach() ||
                         !pfmp->FEfvSupported( JET_efvShelvedPages2 ) )
                    {
                        *psdr = sdrPageNotMovable;
                        goto HandleError;
                    }
                }

                if ( FSPSpaceCatLeaked( spcatfCurrent ) )
                {
                    if ( !pfmp->FRunShrinkDatabaseFullCatOnAttach() && ( PgnoFDP( pSpCatCtx->pfucb ) == pgnoSystemRoot ) )
                    {
                        AssertTrack( fFalse, "ShrinkMoveUnexpectedRootLeakedPage" );
                        *psdr = sdrUnexpected;
                        goto HandleError;
                    }

                    if ( pfmp->FShrinkDatabaseDontTruncateLeakedPagesOnAttach() )
                    {
                        *psdr = sdrPageNotMovable;
                        goto HandleError;
                    }
                }

                CPAGE cpage;


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

                cpage.UnloadPage();
                BFRDWUnlatch( &bfl );
                fPageLatched = fFalse;

                if ( ( errPageStatus != JET_errPageNotInitialized ) &&
                     ( ( dbtimeOnPage != dbtimeShrunk ) || ( clines != 0 ) ) &&
                     ( !fPreInitPage || ( clines > 0 ) ) &&
                     ( !fEmptyPage || ( clines != 0 ) ) )
                {
                    const OBJID objidPage = objidFDP;
                    Assert( objidPage != objidNil );

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
                        AssertTrack( fFalse, OSFormat( "%hs:0x%I32x",
                                                       FSPSpaceCatLeaked( spcatfCurrent ) ?
                                                           "ShrinkMoveInvalidLeakedPage" :
                                                           "ShrinkMoveInvalidIndPage",
                                                        fPageFlags ) );
                        *psdr = sdrUnexpected;
                        goto HandleError;
                    }
                }

                if ( pfmp->FLogOn() && BoolParam( pfmp->Pinst(), JET_paramEnableExternalAutoHealing ) )
                {
                    if ( !pfmp->FEfvSupported( JET_efvScanCheck2 ) )
                    {
                        *psdr = sdrPageNotMovable;
                        goto HandleError;
                    }

                    if ( !fRolledLogOnScanCheck )
                    {
                        Call( ErrLGForceLogRollover( pfmp->Pinst(), __FUNCTION__ ) );
                        fRolledLogOnScanCheck = fTrue;
                    }

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

                    cpage.UnloadPage();
                    BFRDWUnlatch( &bfl );
                    fPageLatched = fFalse;
                }

                if ( BoolParam( pfmp->Pinst(), JET_paramFlight_EnableShrinkArchiving ) )
                {
                    tcScope->iorReason.SetIorp( iorpDatabaseShrink );
                    Call( ErrIOArchiveShrunkPages( ifmp, *tcScope, pgnoCurrent, 1 ) );
                    tcScope->iorReason.SetIorp( iorpNone );
                }

                if ( FSPSpaceCatLeaked( spcatfCurrent ) )
                {
                    Call( ErrSPCaptureSnapshot( pSpCatCtx->pfucb, pgnoCurrent, 1 ) );
                    Call( ErrSPFreeExt( pSpCatCtx->pfucb, pgnoCurrent, 1, "PageUnleak" ) );
                    cpgUnleaked++;
                    fMovedPage = fTrue;
                    continue;
                }

                if ( FSPSpaceCatIndeterminate( spcatfCurrent ) )
                {
                    Call( ErrSPShelvePage( ppib, ifmp, pgnoCurrent ) );

                   cpgShelved++;

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
            }

            if ( FSPSpaceCatSplitBuffer( spcatfCurrent ) )
            {
                Assert( FSPSpaceCatAvailable( spcatfCurrent ) );
                Assert( FSPSpaceCatAnySpaceTree( spcatfCurrent ) );

                Call( ErrSPReplaceSPBuf( pSpCatCtx->pfucb, pSpCatCtx->pfucbParent, pgnoCurrent ) );
                fMovedPage = fTrue;
                (*pcprintfShrinkTraceRaw)( "ShrinkMove[%I32u:%I32u:%d]\r\n", objidCurrent, pgnoCurrent, (int)spcatfCurrent );
                continue;
            }

            if ( FSPSpaceCatSmallSpace( spcatfCurrent ) )
            {
                if ( !FSPSpaceCatRoot( spcatfCurrent ) )
                {
                    FireWall( OSFormat( "ShrinkMoveSmallSpNonRootFirst:0x%I32x", spcatfCurrent ) );
                }

                Assert( !FSPSpaceCatAnySpaceTree( spcatfCurrent ) );
                Expected( PgnoFDP( pSpCatCtx->pfucb ) == pgnoCurrent );

                if ( pgnoLast < (PGNO)( 4 * UlParam( pfmp->Pinst(), JET_paramDbExtensionSize ) ) )
                {
                    *psdr = sdrNoSmallSpaceBurst;
                    goto HandleError;
                }

                Call( ErrSPBurstSpaceTrees( pSpCatCtx->pfucb ) );

                fMovedPage = fTrue;
                continue;
            }

            if ( FSPSpaceCatAvailable( spcatfCurrent ) )
            {
                if ( FSPSpaceCatSplitBuffer( spcatfCurrent ) ||
                        FSPSpaceCatAnySpaceTree( spcatfCurrent ) ||
                        FSPSpaceCatSmallSpace( spcatfCurrent ) )
                {
                    AssertTrack( fFalse, OSFormat( "ShrinkMoveUnexpectedAvail:0x%I32x", spcatfCurrent ) );
                    *psdr = sdrUnexpected;
                    goto HandleError;
                }

                BOOL fMoveToNextPage = fTrue;

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

            if ( FSPSpaceCatRoot( spcatfCurrent ) )
            {
                Assert( !FSPSpaceCatSmallSpace( spcatfCurrent ) );

                if ( !pfmp->FShrinkDatabaseDontMoveRootsOnAttach() && pfmp->FEfvSupported( JET_efvRootPageMove ) )
                {
                    const PGNO pgnoFDP = pSpCatCtx->pfucb->u.pfcb->PgnoFDP();
                    Assert( !FSPSpaceCatAnySpaceTree( spcatfCurrent ) == ( pgnoFDP == pgnoCurrent ) );

                    SPFreeSpaceCatCtx( &pSpCatCtx );

                    Call( ErrSHKRootPageMove( ppib, ifmp, pgnoFDP ) );
                    fMovedPage = fTrue;
                    (*pcprintfShrinkTraceRaw)( "ShrinkMoveRoot[%I32u:%I32u:%I32u:%d]\r\n", objidCurrent, pgnoCurrent, pgnoFDP, (int)spcatfCurrent );

                    cpgMoved += 3;
                    continue;
                }
                else
                {
                    *psdr = sdrPageNotMovable;
                    goto HandleError;
                }
            }

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

                if ( fSpacePage && ( err == JET_errRecordNotFound ) )
                {
                    *ppgnoLastProcessed = pgnoNull;
                    *pspcatfLastProcessed = spcatfNone;
                    err = JET_errSuccess;
                    fMovedPage = fTrue;
                    continue;
                }

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

            AssertTrack( fFalse, OSFormat( "ShrinkMoveUnhandled:0x%I32x", spcatfCurrent ) );
            *psdr = sdrUnexpected;
            goto HandleError;
        }

        if ( fDataMove )
        {
            *pdhrtDataMove += DhrtHRTElapsedFromHrtStart( hrtDataMoveStart );
            fDataMove = fFalse;
        }

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
    }

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

    if ( fInTransaction )
    {
        if ( ( err >= JET_errSuccess ) && fMovedPage )
        {
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

    Call( ErrSPGetLastExtent( ppib, ifmp, &eiInitialOE ) );
    cbSizeOwnedInitial = OffsetOfPgno( eiInitialOE.PgnoLast() + 1 );
    Assert( cbSizeOwnedInitial <= cbSizeFileInitial );

    Call( ErrCATCheckMSObjidsReady( ppib, ifmp, &fMSysObjidsReady ) );
    if ( !fMSysObjidsReady )
    {
        sdr = sdrMSysObjIdsNotReady;
        goto DoneWithDataMove;
    }

    if ( OffsetOfPgno( eiInitialOE.PgnoLast() + 1 ) > cbSizeFileInitial )
    {
        AssertTrack( fFalse, "ShrinkEofOePgnoLastBeyondFsFileSize" );
        Error( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
    }

    OnDebug( fDbMayHaveChanged = fTrue );

    Assert( !pfmp->FShrinkIsRunning() );
    pfmp->SetShrinkIsRunning();

    errVerClean = PverFromPpib( ppib )->ErrVERRCEClean( ifmp );
    if ( errVerClean != JET_errSuccess )
    {
        AssertTrack( fFalse, OSFormat( "ShrinkEofBeginVerCleanErr:%d", errVerClean ) );
        Error( ErrERRCheck( JET_errDatabaseInUse ) );
    }
    pfmp->WaitForTasksToComplete();

    CallJ( ErrSPUnshelveShelvedPagesBelowEof( ppib, ifmp ), DoneWithDataMove );

    forever
    {
        PGNO pgnoFirstFromLastExtentShrunk = pgnoNull;
        fMayHaveShrunk = fTrue;
        Assert( sdr == sdrNone );

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

        if ( ( pgnoFirstFromLastExtentShrunkPrev != pgnoNull ) &&
             ( pgnoFirstFromLastExtentShrunk >= pgnoFirstFromLastExtentShrunkPrev ) )
        {
            AssertTrack( fFalse, "ShrinkEofStalled" );
            sdr = sdrUnexpected;
            goto DoneWithDataMove;
        }
        pgnoFirstFromLastExtentShrunkPrev = pgnoFirstFromLastExtentShrunk;

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

        if ( !pfmp->FShrinkIsActive() )
        {
            CallJ( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ), DoneWithDataMove );
        }

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

    Call( ErrSPGetLastExtent( ppib, ifmp, &eiFinalOE ) );
    Call( pfmp->Pfapi()->ErrSize( &cbSizeFileFinal, IFileAPI::filesizeLogical ) );
    cbSizeOwnedFinal = OffsetOfPgno( eiFinalOE.PgnoLast() + 1 );

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
#endif

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

    if ( ( cbSizeFileInitial != 0 ) && ( cbSizeFileFinal != 0 ) && ( cbSizeFileFinal > cbSizeFileInitial ) &&
         ( err != JET_wrnShrinkNotPossible ) && ( err >= JET_errSuccess ) )
    {
        FireWall( "ShrinkEofFileGrowth" );
    }
    if ( ( cbSizeOwnedInitial != 0 ) && ( cbSizeOwnedFinal != 0 ) && ( cbSizeOwnedFinal > cbSizeOwnedInitial ) &&
         ( err != JET_wrnShrinkNotPossible ) && ( err >= JET_errSuccess ) )
    {
        FireWall( "ShrinkEofOwnedSpaceGrowth" );
    }
    if ( ( cbSizeFileInitial != 0 ) && ( cbSizeOwnedInitial != 0 ) && ( cbSizeFileInitial < cbSizeOwnedInitial ) )
    {
        AssertTrack( false, "ShrinkEofOeFileLargerThanOwnedBefore" );
        err = ErrERRCheck( JET_errSPOwnExtCorrupted );
    }
    if ( ( cbSizeFileFinal != 0 ) && ( cbSizeOwnedFinal != 0 ) && ( cbSizeFileFinal < cbSizeOwnedFinal ) )
    {
        AssertTrack( fFalse, "ShrinkEofOeFileLargerThanOwnedAfter" );
        err = ErrERRCheck( JET_errSPOwnExtCorrupted );
    }

    Assert( ( sdr != sdrNone ) || ( err < JET_errSuccess ) );
    if ( ( sdr == sdrNone ) && ( err < JET_errSuccess ) )
    {
        sdr = sdrFailed;
    }

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

        errVerClean = PverFromPpib( ppib )->ErrVERRCEClean( ifmp );
        if ( errVerClean != JET_errSuccess )
        {
            AssertTrack( fFalse, OSFormat( "ShrinkEofEndVerCleanErr:%d", errVerClean ) );

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
            FCB::PurgeDatabase( ifmp, fFalse  );
        }

        (void)ErrLGSignalAttachDB( ifmp );
    }

    pfmp->ResetPgnoShrinkTarget();
    pfmp->ResetShrinkIsRunning();

    return err;
}



LOCAL VOID SHKIRootMoveRevertDbTime( ROOTMOVE* const prm )
{
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

    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        if ( prmc->csrChildFDP.Latch() == latchWrite )
        {
            prmc->csrChildFDP.RevertDbtime( prmc->dbtimeBeforeChildFDP, prmc->fFlagsBeforeChildFDP );
        }
    }

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

    prm->objid = objid;
    prm->pgnoFDP = pfcb->PgnoFDP();
    prm->pgnoOE = pfcb->PgnoOE();
    prm->pgnoAE = pfcb->PgnoAE();
    Assert( prm->pgnoFDP != pgnoNull );
    Assert( prm->pgnoOE != pgnoNull );
    Assert( prm->pgnoAE != pgnoNull );
    Assert( prm->pgnoAE == ( prm->pgnoOE + 1 ) );


    if ( fRootObject )
    {
        Assert( pfucbCatalog == pfucbNil );
        OBJID objidChild = objidNil;
        SYSOBJ sysobjChild = sysobjNil;
        for ( err = ErrCATGetNextNonRootObject( ppib, ifmp, objidTable, &pfucbCatalog, &objidChild, &sysobjChild );
                ( err >= JET_errSuccess ) && ( objidChild != objidNil );
                err = ErrCATGetNextNonRootObject( ppib, ifmp, objidTable, &pfucbCatalog, &objidChild, &sysobjChild ) )
        {
            PGNO pgnoFDPChild = pgnoNull;
            Call( ErrCATSeekObjectByObjid( ppib, ifmp, objidTable, sysobjChild, objidChild, NULL, 0, &pgnoFDPChild ) );

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

            CallS( ErrCATClose( ppib, pfucbCatalog ) );
            pfucbCatalog = pfucbNil;

            if ( prm->ilineCatObj[iCat] == -1 )
            {
                AssertTrack( fFalse, OSFormat( "RootMoveNoPreCatNodes:%d", (int)fShadowCat ) );
                Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }

            fFirstLatchAttempt = fFalse;

            Call( prm->csrCatObj[iCat].ErrGetRIWPage( ppib, ifmp, prm->pgnoCatObj[iCat] ) );
            if ( prm->csrCatObj[iCat].Dbtime() != prm->dbtimeBeforeCatObj[iCat] )
            {
                continue;
            }

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

    Assert( prm->pgnoCatObj[0] != prm->pgnoCatObj[1] );
    Assert( ( ( prm->pgnoCatClustIdx[0] == pgnoNull ) && ( prm->pgnoCatClustIdx[1] == pgnoNull ) ) ||
            ( prm->pgnoCatClustIdx[0] != prm->pgnoCatClustIdx[1] ) );


    CPG cpgReq = 3;
    PGNO pgnoNewFDP = pgnoNull;
    Call( ErrSPGetExt( pfucb, prm->pgnoFDP, &cpgReq, cpgReq, &pgnoNewFDP ) );
    Assert( cpgReq >= 3 );
    prm->pgnoNewFDP = pgnoNewFDP;
    prm->pgnoNewOE = pgnoNewFDP + 1;
    prm->pgnoNewAE = pgnoNewFDP + 2;


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

    Call( prm->csrFDP.ErrGetRIWPage( ppib, ifmp, prm->pgnoFDP ) );
    prm->dbtimeBeforeFDP = prm->csrFDP.Dbtime();
    prm->fFlagsBeforeFDP = prm->csrFDP.Cpage().FFlags();
    Call( prm->csrOE.ErrGetRIWPage( ppib, ifmp, prm->pgnoOE ) );
    prm->dbtimeBeforeOE = prm->csrOE.Dbtime();
    prm->fFlagsBeforeOE = prm->csrOE.Cpage().FFlags();
    Call( prm->csrAE.ErrGetRIWPage( ppib, ifmp, prm->pgnoAE ) );
    prm->dbtimeBeforeAE = prm->csrAE.Dbtime();
    prm->fFlagsBeforeAE = prm->csrAE.Cpage().FFlags();

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


    prm->csrFDP.UpgradeFromRIWLatch();
    prm->csrOE.UpgradeFromRIWLatch();
    prm->csrAE.UpgradeFromRIWLatch();
    Assert( prm->csrNewFDP.Latch() == latchWrite );
    Assert( prm->csrNewOE.Latch() == latchWrite );
    Assert( prm->csrNewAE.Latch() == latchWrite );

    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        prmc->csrChildFDP.UpgradeFromRIWLatch();
    }

    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        Assert( prm->csrCatObj[iCat].FLatched() );
        Assert( prm->pgnoCatObj[iCat] != pgnoNull );
        Assert( prm->ilineCatObj[iCat] != -1 );
        Assert( ( prm->csrCatClustIdx[iCat].FLatched() &&
                  ( prm->pgnoCatClustIdx[iCat] != prm->pgnoCatObj[iCat] ) &&
                  ( prm->ilineCatClustIdx[iCat] != -1 ) ) ||
                ( !prm->csrCatClustIdx[iCat].FLatched() &&
                  ( prm->pgnoCatClustIdx[iCat] == prm->pgnoCatObj[iCat] ) &&
                  ( prm->ilineCatClustIdx[iCat] != -1 ) ) ||
                ( !prm->csrCatClustIdx[iCat].FLatched() &&
                  ( prm->pgnoCatClustIdx[iCat] == pgnoNull ) &&
                  ( prm->ilineCatClustIdx[iCat] == -1 ) ) );

        prm->csrCatObj[iCat].UpgradeFromRIWLatch();
        if ( prm->csrCatClustIdx[iCat].FLatched() )
        {
            prm->csrCatClustIdx[iCat].UpgradeFromRIWLatch();
        }
    }


    SHKIRootMoveGetPagePreImage( ifmp, prm->pgnoFDP, &prm->csrFDP, &prm->dataBeforeFDP );
    SHKIRootMoveGetPagePreImage( ifmp, prm->pgnoOE, &prm->csrOE, &prm->dataBeforeOE );
    SHKIRootMoveGetPagePreImage( ifmp, prm->pgnoAE, &prm->csrAE, &prm->dataBeforeAE );

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

    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        prmc->sphNew.SetPgnoParent( prm->pgnoNewFDP );
    }

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


    Assert( prm->csrFDP.Dbtime() == prm->dbtimeBeforeFDP );
    Assert( prm->csrOE.Dbtime() == prm->dbtimeBeforeOE );
    Assert( prm->csrAE.Dbtime() == prm->dbtimeBeforeAE );
    prm->csrFDP.CoordinatedDirty( dbtimeAfter );
    prm->csrOE.CoordinatedDirty( dbtimeAfter );
    prm->csrAE.CoordinatedDirty( dbtimeAfter );
    prm->csrNewFDP.CoordinatedDirty( dbtimeAfter );
    prm->csrNewOE.CoordinatedDirty( dbtimeAfter );
    prm->csrNewAE.CoordinatedDirty( dbtimeAfter );

    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        Assert( prmc->csrChildFDP.Dbtime() == prmc->dbtimeBeforeChildFDP );
        prmc->csrChildFDP.CoordinatedDirty( dbtimeAfter );
    }

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
    prm->csrFDP.Cpage().SetLgposModify( lgpos );
    prm->csrOE.Cpage().SetLgposModify( lgpos );
    prm->csrAE.Cpage().SetLgposModify( lgpos );

    prm->csrNewFDP.Cpage().SetLgposModify( lgpos );
    prm->csrNewOE.Cpage().SetLgposModify( lgpos );
    prm->csrNewAE.Cpage().SetLgposModify( lgpos );

    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        prmc->csrChildFDP.Cpage().SetLgposModify( lgpos );
    }

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

    UtilMemCpy(
        pcsr->Cpage().PvBuffer(),
        dataOld.Pv(),
        dataOld.Cb() );

    pcsr->Cpage().SetPgno( pgnoNew );
    pcsr->Cpage().SetDbtime( dbtimeAfter );
}

LOCAL VOID SHKIRootMoveReleaseLatches( ROOTMOVE* const prm )
{
    prm->csrFDP.ReleasePage();
    prm->csrOE.ReleasePage();
    prm->csrAE.ReleasePage();
    prm->csrNewFDP.ReleasePage();
    prm->csrNewOE.ReleasePage();
    prm->csrNewAE.ReleasePage();

    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        prmc->csrChildFDP.ReleasePage();
    }

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

    if ( pfcb->PgnoFDP() != rm.pgnoNewFDP )
    {
        AssertTrack( fFalse, "RootMoveBadPgnoNewFdp" );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( pfcb->PgnoOE() != rm.pgnoNewOE )
    {
        AssertTrack( fFalse, "RootMoveBadPgnoNewOE" );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( pfcb->PgnoAE() != rm.pgnoNewAE )
    {
        AssertTrack( fFalse, "RootMoveBadPgnoNewAE" );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( fRootObject )
    {
        ROOTMOVECHILD* prmc = rm.prootMoveChildren;

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

            PGNO pgnoFDPChild = pgnoNull;
            Call( ErrCATSeekObjectByObjid( ppib, ifmp, objidTable, sysobjChild, objidChild, NULL, 0, &pgnoFDPChild ) );

            Call( ErrBTIOpen( ppib, ifmp, pgnoFDPChild, objidNil, openNormal, &pfucbChild, fFalse ) );
            Call( ErrBTIGotoRoot( pfucbChild, latchRIW ) );
            pfucbChild->pcsrRoot = Pcsr( pfucbChild );

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

    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        const BOOL fShadowCat = ( iCat == 1 );
        OBJID objidT = objidNil;
        SYSOBJ sysobjT = sysobjNil;

        Assert( pfucbCatalog == pfucbNil );
        err = ErrCATGetNextObjectByPgnoFDP( ppib, ifmp, objidTable, rm.pgnoFDP, fShadowCat, &pfucbCatalog, &objidT, &sysobjT );
        if ( ( err >= JET_errSuccess ) && ( objidT != objidNil ) )
        {
            AssertTrack( fFalse, OSFormat( "RootMoveUnexpectedOldPgnoFdpObject:%d", (int)fShadowCat ) );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        Call( err );

        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;

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

        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;

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

    const ERR errVerClean = PverFromPpib( ppib )->ErrVERRCEClean( ifmp );
    if ( errVerClean != JET_errSuccess )
    {
        AssertTrack( fFalse, OSFormat( "RootPageMoveVerCleanErr:%d", errVerClean ) );
        Error( ErrERRCheck( JET_errDatabaseInUse ) );
    }
    pfmp->WaitForTasksToComplete();


    Call( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb ) );
    pfcb = pfucb->u.pfcb;

    objid = pfcb->ObjidFDP();
    Call( ErrCATGetObjidMetadata( ppib, ifmp, objid, &objidTable, &sysobj ) );
    fRootObject = ( sysobj == sysobjTable );
    Assert( !!fRootObject == ( objid == objidTable ) );

    BTClose( pfucb );
    pfucb = pfucbNil;
    pfcb = pfcbNil;

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


    Call( ErrSHKICreateRootMove( &rm, pfucb, objidTable ) );

    Call( ErrSHKIRootMoveUpgradeLatches( &rm, ppib, ifmp ) );
    OnDebug( rm.AssertValid( fTrue , fFalse  ) );


    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        Assert( rm.pgnoCatObj[iCat] != pgnoNull );
        Assert( ( rm.pgnoCatClustIdx[iCat] == pgnoNull ) ||
                ( rm.pgnoCatObj[iCat] == rm.pgnoCatClustIdx[iCat] ) ||
                ( ( pgnoCatFDP[iCat] != rm.pgnoCatObj[iCat] ) &&
                  ( pgnoCatFDP[iCat] != rm.pgnoCatClustIdx[iCat] ) ) );

        if ( ( pgnoCatFDP[iCat] != rm.pgnoCatObj[iCat] ) &&
                ( pgnoCatFDP[iCat] != rm.pgnoCatClustIdx[iCat] ) )
        {
            Call( csrCatLock[iCat].ErrGetRIWPage( ppib, ifmp, pgnoCatFDP[iCat] ) );
            csrCatLock[iCat].UpgradeFromRIWLatch();
        }
    }

    CATFreeCursorsFromObjid( pfucb, pfucbParent );
    fFullyInitCursor = fFalse;
    pfucb = pfucbNil;
    pfucbParent = pfucbNil;
    pfcb = pfcbNil;
    FCB::PurgeObject( ifmp, fRootObject ? pgnoFDP : pgnoFDPParentBefore );

    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        FCB::PurgeObject( ifmp, pgnoCatFDP[iCat] );
    }


    if ( pfmp->FLogOn() )
    {
        LGPOS lgpos = lgposMin;
        Call( ErrLGRootPageMove( ppib, ifmp, &rm, &lgpos ) );
        OnDebug( rm.AssertValid( fTrue , fFalse  ) );

        SHKIRootMoveSetLgposModify( &rm, lgpos );
    }


    fMoveLogged = fTrue;


    SHKPerformRootMove( &rm, ppib, ifmp, fFalse  );
    OnDebug( rm.AssertValid( fFalse , fFalse  ) );

    fMovePerformed = fTrue;

    SHKIRootMoveReleaseLatches( &rm );

    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        csrCatLock[iCat].ReleasePage();
    }


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

    if ( pfucbParent->u.pfcb->PgnoFDP() != pgnoFDPParentBefore )
    {
        AssertTrack( fFalse, "RootMoveBadPgnoNewFdp" );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    Call( ErrSHKIRootMoveCheck( rm, pfucb, objidTable ) );


    Call( ErrFaultInjection( 45866 ) );
    Call( ErrSHKIRootMoveFreeEmptiedPages( rm, pfucb ) );


    CATFreeCursorsFromObjid( pfucb, pfucbParent );
    fFullyInitCursor = fFalse;
    pfucb = pfucbNil;
    pfucbParent = pfucbNil;
    pfcb = pfcbNil;

    rm.ReleaseResources();

    err = JET_errSuccess;

HandleError:
    Assert( fMoveLogged || ( err < JET_errSuccess ) );
    Assert( fMovePerformed || ( err < JET_errSuccess ) );
    Expected( !fFullyInitCursor || ( err < JET_errSuccess ) );

    if ( !fMoveLogged )
    {
        SHKIRootMoveRevertDbTime( &rm );
    }

    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        Assert( ( csrCatLock[iCat].Latch() == latchNone ) || ( err < JET_errSuccess ) );
        csrCatLock[iCat].ReleasePage();
    }

    rm.ReleaseResources();

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
    if ( FBTIUpdatablePage( prm->csrNewFDP ) )
    {
        SHKIRootMoveCopyToNewPage(
            &prm->csrNewFDP,
            prm->dataBeforeFDP,
            prm->pgnoNewFDP,
            prm->dbtimeAfter,
            fRecoveryRedo );

         prm->csrNewFDP.FinalizePreInitPage();

        LINE lineExtHdr;
        NDGetPtrExternalHeader( prm->csrNewFDP.Cpage(), &lineExtHdr, noderfSpaceHeader );
        SPACE_HEADER* const psph = (SPACE_HEADER*)( lineExtHdr.pv );
        psph->SetPgnoOE( prm->pgnoNewOE );
    }

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

    if ( FBTIUpdatablePage( prm->csrFDP ) )
    {
        NDEmptyPage( &prm->csrFDP );
    }

    if ( FBTIUpdatablePage( prm->csrOE ) )
    {
        NDEmptyPage( &prm->csrOE );
    }

    if ( FBTIUpdatablePage( prm->csrAE ) )
    {
        NDEmptyPage( &prm->csrAE );
    }
}

