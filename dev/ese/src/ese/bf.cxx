// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_bf.hxx"




#ifdef g_cbPage
#undef g_cbPage
#endif
#define g_cbPage g_cbPage_BF_NOT_ALLOWED_TO_USE_THIS_VARIABLE

#include <malloc.h>


CSmallLookasideCache* g_pBFAllocLookasideList;






ERR ErrBFInit( __in const LONG cbPageSizeMax )
{
    ERR err = JET_errSuccess;


    Assert( !g_fBFInitialized );
    Assert( !g_fBFCacheInitialized );


    g_dblBFSpeedSizeTradeoff  = 0.0;
    g_dblBFHashLoadFactor     = 5.0;
    g_dblBFHashUniformity     = 1.0;
    g_csecBFLRUKUncertainty   = 1.0;
 

    CallJ( ErrBFIFTLInit(), Validate );


    cBFOpportuneWriteIssued = 0;

    PERFOpt( g_cBFVersioned = 0 );


    Assert( g_rgbBFTemp == NULL );
    Assert( g_pBFAllocLookasideList == NULL );

    g_rgbBFTemp               = NULL;
    g_pBFAllocLookasideList = NULL;


    if ( BoolParam( JET_paramEnableViewCache ) )
    {
        g_rgbBFTemp = (BYTE*)PvOSMemoryPageAlloc( cbPageSizeMax, NULL );
        if ( NULL == g_rgbBFTemp )
        {
            err = ErrERRCheck( JET_errOutOfMemory );
            goto TermMemoryAlloc;
        }
    }

    g_pBFAllocLookasideList = new CSmallLookasideCache();
    if ( g_pBFAllocLookasideList == NULL )
    {
        err = ErrERRCheck( JET_errOutOfMemory );
        goto TermMemoryAlloc;
    }


    switch ( g_bfhash.ErrInit(    g_dblBFHashLoadFactor,
                                g_dblBFHashUniformity ) )
    {
        default:
            AssertSz( fFalse, "Unexpected error initializing BF Hash Table" );
        case BFHash::ERR::errOutOfMemory:
            CallJ( ErrERRCheck( JET_errOutOfMemory ), TermLRUK );
        case BFHash::ERR::errSuccess:
            break;
    }


    g_pBFAllocLookasideList->Init( cbPageSizeMax );

    switch ( g_bfavail.ErrInit( g_dblBFSpeedSizeTradeoff ) )
    {
        default:
            AssertSz( fFalse, "Unexpected error initializing BF Avail Pool" );
        case BFAvail::ERR::errOutOfMemory:
            CallJ( ErrERRCheck( JET_errOutOfMemory ), TermLRUK );
        case BFAvail::ERR::errSuccess:
            break;
    }

    Assert( g_bfquiesced.FEmpty() );

    CallJ( ErrBFIMaintScavengePreInit( max( 1, min( Kmax, INT( UlParam( JET_paramLRUKPolicy ) ) ) ),
                                        double( UlParam( JET_paramLRUKCorrInterval ) ) / 1000000,
                                        double( UlParam( JET_paramLRUKTimeout ) ),
                                        g_csecBFLRUKUncertainty,
                                        g_dblBFHashLoadFactor,
                                        g_dblBFHashUniformity,
                                        g_dblBFSpeedSizeTradeoff ), TermLRUK );

    CallJ( ErrBFICacheInit( cbPageSizeMax ), TermLRUK );

    g_fBFCacheInitialized = fTrue;

    if ( !g_critpoolBFDUI.FInit( OSSyncGetProcessorCount(), rankBFDUI, szBFDUI ) )
    {
        CallJ( ErrERRCheck( JET_errOutOfMemory ), TermCache );
    }

    CallJ( ErrBFIMaintInit(), TermDUI );


    g_fBFInitialized = fTrue;

    goto Validate;


TermDUI:
    g_critpoolBFDUI.Term();
TermCache:
    g_fBFCacheInitialized = fFalse;
    BFICacheTerm();
TermLRUK:
    g_bflruk.Term();
    g_bfavail.Term();
    g_bfquiesced.Empty();
    g_bfhash.Term();
TermMemoryAlloc:
    OSMemoryPageFree( g_rgbBFTemp );
    g_rgbBFTemp = NULL;

    delete g_pBFAllocLookasideList;
    g_pBFAllocLookasideList = NULL;

    BFIFTLTerm();

Validate:
    Assert( err == JET_errOutOfMemory ||
            err == JET_errOutOfThreads ||
            err == JET_errSuccess );
    Assert( ( err != JET_errSuccess && !g_fBFInitialized ) ||
            ( err == JET_errSuccess && g_fBFInitialized ) );

    return err;
}


void BFTerm()
{

    Assert( g_fBFInitialized );
    Assert( g_fBFCacheInitialized );

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlBfTerm, NULL );

    if ( g_rgfmp )
    {
        for ( IFMP ifmp = cfmpReserved; ifmp < g_ifmpMax && FMP::FAllocatedFmp( ifmp ); ifmp++ )
        {
            if ( g_rgfmp[ ifmp ].FBFContext() )
            {
                BFPurge( ifmp );
            }
        }
    }

    g_fBFInitialized = fFalse;


    BFIMaintTerm();

    g_critpoolBFDUI.Term();
    g_fBFCacheInitialized = fFalse;
    BFICacheTerm();
    g_bflruk.Term();
    BFITraceResMgrTerm();
    g_bfavail.Term();
    g_bfquiesced.Empty();
    
    OSMemoryPageFree( g_rgbBFTemp );
    g_rgbBFTemp = NULL;

#ifdef DEBUG
    OSMemoryPageFree( g_pvIoThreadImageCheckCache );
    g_pvIoThreadImageCheckCache = NULL;
#endif

    g_pBFAllocLookasideList->Term();
    delete g_pBFAllocLookasideList;
    g_pBFAllocLookasideList = NULL;

    g_bfhash.Term();
    BFIFTLTerm();
}





ERR ErrBFGetCacheSize( ULONG_PTR* const pcpg )
{

    if ( pcpg == NULL )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    const LONG_PTR cbfCacheSizeT = cbfCacheSize;
    const LONG_PTR cbfBFIAveCredit = CbfBFIAveCredit();

    *pcpg = ( cbfCacheSizeT > cbfBFIAveCredit ) ? ( cbfCacheSizeT - cbfBFIAveCredit ) : 0;
    return JET_errSuccess;
}



ERR ErrBFIStartCacheTasks()
{
    ERR err = JET_errSuccess;
    
    if ( g_fBFInitialized )
    {
        BFICacheSetTarget( OnDebug( -1 ) );
        Call( ErrBFIMaintCacheSizeRequest() );
        Call( ErrBFIMaintCacheStatsRequest( bfmcsrtForce ) );
    }

HandleError:
    return err;
}

ERR ErrBFSetCacheSize( const ULONG_PTR cpg )
{
    ERR err = JET_errSuccess;
    

    g_critCacheSizeSetTarget.Enter();

    g_cbfCacheUserOverride = cpg;

    g_critCacheSizeSetTarget.Leave();

    Call( ErrBFIStartCacheTasks() );

HandleError:
    return err;
}

ERR ErrBFConsumeSettings( BFConsumeSetting bfcs, const IFMP ifmp )
{
    ERR err = JET_errSuccess;

    if ( bfcs & bfcsCacheSize )
    {
        Assert( ifmp == ifmpNil);
        Call( ErrBFIStartCacheTasks() );
        bfcs = BFConsumeSetting( bfcs & ~bfcsCacheSize );
    }

    if ( bfcs & bfcsCheckpoint )
    {
        BFIMaintCheckpointDepthRequest( &g_rgfmp[ifmp], bfcpdmrRequestConsumeSettings );
        bfcs = BFConsumeSetting( bfcs & ~bfcsCheckpoint );
    }

    AssertSz( bfcs == 0, "Unknown settings (%d / 0x%x) bits left unhandled", bfcs, bfcs );

HandleError:
    return err;
}

ERR ErrBFCheckMaintAvailPoolStatus()
{
    const LONG_PTR cbfAvail = g_bfavail.Cobject();
    if ( cbfAvail < cbfAvailPoolLow )
    {
        return ErrERRCheck( JET_wrnIdleFull );
    }
    return JET_errSuccess;
}

ERR ErrBFICapturePagePreimage( BF *pbf, RBS_POS *prbsposSnapshot )
{
    if ( g_rgfmp[ pbf->ifmp ].Dbid() == dbidTemp ||
         !g_rgfmp[ pbf->ifmp ].FRBSOn() )
    {
        return JET_errSuccess;
    }
    if ( ((CPAGE::PGHDR *)pbf->pv)->dbtimeDirtied == 0 ||
         ((CPAGE::PGHDR *)pbf->pv)->dbtimeDirtied > g_rgfmp[ pbf->ifmp ].DbtimeBeginRBS() ||
         pbf->rbsposSnapshot.lGeneration == g_rgfmp[ pbf->ifmp ].PRBS()->RbsposFlushPoint().lGeneration )
    {
        *prbsposSnapshot = pbf->rbsposSnapshot;
        return JET_errSuccess;
    }

    ERR err = g_rgfmp[ pbf->ifmp ].PRBS()->ErrCapturePreimage( g_rgfmp[ pbf->ifmp ].Dbid(), pbf->pgno, (const BYTE *)pbf->pv, CbBFIBufferSize( pbf ), prbsposSnapshot );
    OSTrace( JET_tracetagRBS, OSFormat(
             "Collecting pre-image dbid:%u,pgno:%lu,dbtime:0x%I64x,dbtimeBegin:0x%I64x,rbspos:%u,%u\n",
             g_rgfmp[ pbf->ifmp ].Dbid(),
             pbf->pgno,
             (DBTIME)((CPAGE::PGHDR *)pbf->pv)->dbtimeDirtied,
             g_rgfmp[ pbf->ifmp ].DbtimeBeginRBS(),
             prbsposSnapshot->lGeneration,
             prbsposSnapshot->iSegment ) );
    return err;
}

ERR ErrBFICaptureNewPage( BF *pbf, RBS_POS *prbsposSnapshot )
{
    if ( g_rgfmp[ pbf->ifmp ].Dbid() == dbidTemp ||
         !g_rgfmp[ pbf->ifmp ].FRBSOn() )
    {
        return JET_errSuccess;
    }
    if ( pbf->rbsposSnapshot.lGeneration == g_rgfmp[ pbf->ifmp ].PRBS()->RbsposFlushPoint().lGeneration )
    {
        *prbsposSnapshot = pbf->rbsposSnapshot;
        return JET_errSuccess;
    }

    ERR err = g_rgfmp[ pbf->ifmp ].PRBS()->ErrCaptureNewPage( g_rgfmp[ pbf->ifmp ].Dbid(), pbf->pgno, prbsposSnapshot );
    OSTrace( JET_tracetagRBS, OSFormat(
             "Collecting new-page dbid:%u,pgno:%lu,rbspos:%u,%u\n",
             g_rgfmp[ pbf->ifmp ].Dbid(),
             pbf->pgno,
             prbsposSnapshot->lGeneration,
             prbsposSnapshot->iSegment ) );
    return err;
}


ERR ErrBFReadLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf, const BFPriority bfpri, const TraceContext& tc )
{
    ERR err;

    AssertRTL( g_fBFInitialized );


    Assert( FBFNotLatched( ifmp, pgno ) );
    Assert( !( bflf & ( bflfNew | bflfNewIfUncached ) ) );
    Assert( tc.iorReason.Iorp() == iorpNone );


    const BFLatchFlags bflfMask     = BFLatchFlags( bflfNoCached | bflfHint );
    const BFLatchFlags bflfPattern  = BFLatchFlags( bflfHint );

    if ( ( bflf & bflfMask ) == bflfPattern )
    {

        Assert( FBFILatchValidContext( pbfl->dwContext ) || !pbfl->dwContext );

        PBF pbfHint;

        if ( pbfl->dwContext & 1 )
        {
            pbfHint = ((BFHashedLatch*)( pbfl->dwContext ^ 1 ))->pbf;
        }
        else
        {
            pbfHint = PBF( pbfl->dwContext );
        }


        if ( pbfHint != pbfNil )
        {

            PLS*            ppls;
            CSXWLatch*      psxwl;
            const size_t    iHashedLatch    = pbfHint->iHashedLatch;

            if ( iHashedLatch < cBFHashedLatch )
            {
                ppls    = Ppls();
                psxwl   = &ppls->rgBFHashedLatch[ iHashedLatch ].sxwl;
            }
            else
            {
                ppls    = NULL;
                psxwl   = &pbfHint->sxwl;
            }


            CLockDeadlockDetectionInfo::DisableOwnershipTracking();
            if ( psxwl->ErrTryAcquireSharedLatch() == CSXWLatch::ERR::errSuccess )
            {

                PBF pbfLatch;
                if ( iHashedLatch < cBFHashedLatch )
                {
                    pbfLatch = ppls->rgBFHashedLatch[ iHashedLatch ].pbf;
                }
                else
                {
                    pbfLatch = pbfHint;
                }

                if (    pbfLatch == pbfHint &&
                        FBFICurrentPage( pbfHint, ifmp, pgno ) &&
                        !pbfHint->fAbandoned &&
                        !FBFIChance( 25 ) &&
                        ( err = pbfHint->err ) >= JET_errSuccess &&
                        ( pbfHint->bfat != bfatViewMapped || FBFICacheViewFresh( pbfHint ) ) &&
                        pbfHint->bfrs == bfrsResident )
                {

                    CLockDeadlockDetectionInfo::EnableOwnershipTracking();
                    Assert( psxwl->FNotOwner() );
                    psxwl->ClaimOwnership( bfltShared );



                    const BOOL fTouchPage = ( !( bflf & bflfNoTouch ) && !BoolParam( JET_paramEnableFileCache ) );

                    BFITouchResource( pbfHint, bfltShared, bflf, fTouchPage, PctBFCachePri( bfpri ), tc );


                    PERFOpt( cBFCacheReq.Inc( PinstFromIfmp( pbfHint->ifmp ), pbfHint->tce ) );

                    pbfl->pv        = pbfHint->pv;
                    pbfl->dwContext = DWORD_PTR( pbfHint );

                    if ( iHashedLatch < cBFHashedLatch )
                    {
                        ppls->rgBFHashedLatch[ iHashedLatch ].cCacheReq++;
                        pbfl->dwContext = DWORD_PTR( &ppls->rgBFHashedLatch[ iHashedLatch ] ) | 1;
                    }
                    else if ( pbfHint->bfls == bflsElect )
                    {
                        Ppls()->rgBFNominee[ 0 ].cCacheReq++;
                    }

                    Assert( FParentObjectClassSet( tc.nParentObjectClass ) );
                    Assert( FEngineObjidSet( tc.dwEngineObjid ) );

                    goto HandleError;
                }
                psxwl->ReleaseSharedLatch();
            }
            CLockDeadlockDetectionInfo::EnableOwnershipTracking();
        }
    }


    Call( ErrBFILatchPage( pbfl, ifmp, pgno, bflf, bfltShared, bfpri, tc ) );

HandleError:


#ifdef DEBUG
    Assert( err != wrnBFPageFault || !( bflf & bflfNoUncached ) );
    Assert( err != errBFPageCached || ( bflf & bflfNoCached ) );
    Assert( err != errBFPageNotCached || ( bflf & bflfNoUncached ) );
    Assert( err != errBFLatchConflict || ( bflf & bflfNoWait ) );

    if ( err >= JET_errSuccess )
    {
        Assert( FBFReadLatched( pbfl ) );
        Assert( FBFICurrentPage( PbfBFILatchContext( pbfl->dwContext ), ifmp, pgno ) );
    }
    else
    {
        Assert( FBFNotLatched( ifmp, pgno ) );
    }
#endif

    return err;
}

ERR ErrBFRDWLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf, const BFPriority bfpri, const TraceContext& tc )
{
    ERR err;

    AssertRTL( g_fBFInitialized );


    Assert( FBFNotLatched( ifmp, pgno ) );
    Assert( !( bflf & ( bflfNew | bflfNewIfUncached ) ) );
    Assert( tc.iorReason.Iorp( ) == iorpNone );


    const BFLatchFlags bflfMask     = BFLatchFlags( bflfNoCached | bflfHint );
    const BFLatchFlags bflfPattern  = BFLatchFlags( bflfHint );

    if ( ( bflf & bflfMask ) == bflfPattern )
    {

        Assert( FBFILatchValidContext( pbfl->dwContext ) || !pbfl->dwContext );

        PBF pbfHint;

        if ( pbfl->dwContext & 1 )
        {
            pbfHint = ((BFHashedLatch*)( pbfl->dwContext ^ 1 ))->pbf;
        }
        else
        {
            pbfHint = PBF( pbfl->dwContext );
        }


        if ( pbfHint != pbfNil )
        {

            CLockDeadlockDetectionInfo::DisableOwnershipTracking();
            if ( pbfHint->sxwl.ErrTryAcquireExclusiveLatch() == CSXWLatch::ERR::errSuccess )
            {

                if (    FBFICurrentPage( pbfHint, ifmp, pgno ) &&
                        !pbfHint->fAbandoned &&
                        FBFIUpdatablePage( pbfHint ) &&
                        ( err = pbfHint->err ) >= JET_errSuccess &&
                        ( pbfHint->bfat != bfatViewMapped || FBFICacheViewFresh( pbfHint ) ) &&
                        pbfHint->bfrs == bfrsResident )
                {

                    CLockDeadlockDetectionInfo::EnableOwnershipTracking();
                    Assert( pbfHint->sxwl.FNotOwner() );
                    pbfHint->sxwl.ClaimOwnership( bfltExclusive );



                    const BOOL fTouchPage = ( !( bflf & bflfNoTouch ) && !BoolParam( JET_paramEnableFileCache ) );

                    BFITouchResource( pbfHint, bfltExclusive, bflf, fTouchPage, PctBFCachePri( bfpri ), tc );


                    PERFOpt( cBFCacheReq.Inc( PinstFromIfmp( pbfHint->ifmp ), pbfHint->tce ) );

                    pbfl->pv        = pbfHint->pv;
                    pbfl->dwContext = DWORD_PTR( pbfHint );

                    goto HandleError;
                }
                pbfHint->sxwl.ReleaseExclusiveLatch();
            }
            CLockDeadlockDetectionInfo::EnableOwnershipTracking();
        }
    }


    Call( ErrBFILatchPage( pbfl, ifmp, pgno, bflf, bfltExclusive, bfpri, tc ) );

HandleError:


#ifdef DEBUG
    Assert( err != wrnBFPageFault || !( bflf & bflfNoUncached ) );
    Assert( err != errBFPageCached || ( bflf & bflfNoCached ) );
    Assert( err != errBFPageNotCached || ( bflf & bflfNoUncached ) );
    Assert( err != errBFLatchConflict || ( bflf & bflfNoWait ) );

    if ( err >= JET_errSuccess )
    {
        Assert( FBFRDWLatched( pbfl ) );
        Assert( FBFICurrentPage( PBF( pbfl->dwContext ), ifmp, pgno ) );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageNotVerified );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageRemapNotReVerified );
        Expected( FBFUpdatableLatch( pbfl ) );
    }
    else
    {
        Assert( FBFNotLatched( ifmp, pgno ) );
    }
#endif

    return err;
}

ERR ErrBFWARLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf, const BFPriority bfpri, const TraceContext& tc )
{
    ERR err;


    Assert( FBFNotLatched( ifmp, pgno ) );
    Assert( !( bflf & ( bflfNew | bflfNewIfUncached ) ) );
    Assert( tc.iorReason.Iorp( ) == iorpNone );


    if ( BoolParam( JET_paramEnableViewCache ) )
    {
        CallR( ErrERRCheck( errBFLatchConflict ) );
    }


    Call( ErrBFRDWLatchPage( pbfl, ifmp, pgno, bflf, bfpri, tc ) );


    PBF( pbfl->dwContext )->fWARLatch = fTrue;

HandleError:


#ifdef DEBUG
    Assert( err != wrnBFPageFault || !( bflf & bflfNoUncached ) );
    Assert( err != errBFPageCached || ( bflf & bflfNoCached ) );
    Assert( err != errBFPageNotCached || ( bflf & bflfNoUncached ) );
    Assert( err != errBFLatchConflict || ( bflf & bflfNoWait ) );

    if ( err >= JET_errSuccess )
    {
        Assert( FBFWARLatched( pbfl ) );
        Assert( FBFICurrentPage( PBF( pbfl->dwContext ), ifmp, pgno ) );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageNotVerified );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageRemapNotReVerified );
        Assert( FBFUpdatableLatch( pbfl ) );
        Assert( PBF( pbfl->dwContext )->icbBuffer == PBF( pbfl->dwContext )->icbPage );
    }
    else
    {
        Assert( FBFNotLatched( ifmp, pgno ) );
    }
#endif

    return err;
}

ERR ErrBFWriteLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf, const BFPriority bfpri, const TraceContext& tc, BOOL* const pfCachedNewPage )
{
    ERR err;
    BOOL fCachedNewPage = fFalse;
    RBS_POS rbsposSnapshot = rbsposMin;

    AssertRTL( g_fBFInitialized );


    Assert( FBFNotLatched( ifmp, pgno ) );
    Assert( !( bflf & ( bflfNew | bflfNewIfUncached ) ) || !( bflf & bflfNoTouch ) );
    Assert( tc.iorReason.Iorp( ) == iorpNone );


    Call( ErrBFILatchPage( pbfl, ifmp, pgno, bflf, bfltWrite, bfpri, tc, &fCachedNewPage ) );
    Assert( !fCachedNewPage || ( bflf & ( bflfNew | bflfNewIfUncached ) ) );
    Assert( PBF( pbfl->dwContext )->ifmp == ifmp );

    Assert( FBFUpdatableLatch( pbfl ) );

    if ( bflf & ( bflfNew | bflfNewIfUncached ) )
    {
        err = ErrBFICaptureNewPage( PBF( pbfl->dwContext ), &rbsposSnapshot );
    }
    else
    {
        err = ErrBFICapturePagePreimage( PBF( pbfl->dwContext ), &rbsposSnapshot );
    }
    if ( err < JET_errSuccess )
    {
        BFWriteUnlatch( pbfl );
        goto HandleError;
    }

#ifdef DEBUG
    const PBF pbfPreMaint = PBF( pbfl->dwContext );
#endif


    err = ErrBFIMaintImpedingPageLatch( PBF( pbfl->dwContext ), fTrue, pbfl );
    Assert( err >= JET_errSuccess );
    if ( wrnBFLatchMaintConflict == err )
    {
        Assert( ( 0x1 & (DWORD_PTR)PBF( pbfl->dwContext ) ) == 0x0 );
        Assert( pbfPreMaint != PBF( pbfl->dwContext ) );
        Assert( PBF( pbfl->dwContext )->sxwl.FOwnWriteLatch() );

        err = ErrERRCheck( wrnBFBadLatchHint );
    }
    else
    {
        Assert( JET_errSuccess == err );
        Assert( pbfPreMaint == PBF( pbfl->dwContext ) );
    }

    PBF( pbfl->dwContext )->rbsposSnapshot = rbsposSnapshot;


    Assert( !fCachedNewPage ||
            ( PBF( pbfl->dwContext )->icbPage == PBF( pbfl->dwContext )->icbBuffer ) ||
            ( PBF( pbfl->dwContext )->bfdf == bfdfUntidy ) );

    BFIRehydratePage( PBF( pbfl->dwContext ) );

    Assert( FBFCurrentLatch( pbfl ) );
    Assert( FBFUpdatableLatch( pbfl ) );
    Assert( PBF( pbfl->dwContext )->icbBuffer == PBF( pbfl->dwContext )->icbPage );

HandleError:


#ifdef DEBUG
    Assert( err != wrnBFPageFault || !( bflf & bflfNoUncached ) );
    Assert( err != errBFPageCached || ( bflf & bflfNoCached ) );
    Assert( err != errBFPageNotCached || ( bflf & bflfNoUncached ) );
    Assert( err != errBFLatchConflict || ( bflf & bflfNoWait ) );

    if ( err >= JET_errSuccess )
    {
        Assert( FBFWriteLatched( pbfl ) );
        Assert( FBFICurrentPage( PBF( pbfl->dwContext ), ifmp, pgno ) );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageNotVerified );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageRemapNotReVerified );
        Assert( FBFUpdatableLatch( pbfl ) );
        Assert( PBF( pbfl->dwContext )->icbBuffer == PBF( pbfl->dwContext )->icbPage );
    }
    else
    {
        Assert( FBFNotLatched( ifmp, pgno ) );
    }
#endif

    if ( pfCachedNewPage != NULL )
    {
        *pfCachedNewPage = fCachedNewPage;
    }

    return err;
}


ERR ErrBFUpgradeReadLatchToRDWLatch( BFLatch* pbfl )
{

    Assert( FBFReadLatched( pbfl ) );

    ERR             err;
    PBF             pbf;
    CSXWLatch*      psxwl;
    CSXWLatch::ERR  errSXWL;


    if ( pbfl->dwContext & 1 )
    {
        pbf     = ((BFHashedLatch*)( pbfl->dwContext ^ 1 ))->pbf;
        psxwl   = &((BFHashedLatch*)( pbfl->dwContext ^ 1 ))->sxwl;
    }
    else
    {
        pbf     = PBF( pbfl->dwContext );
        psxwl   = &pbf->sxwl;
    }


    if ( psxwl == &pbf->sxwl )
    {
        errSXWL = pbf->sxwl.ErrUpgradeSharedLatchToExclusiveLatch();
    }
    else
    {
        errSXWL = pbf->sxwl.ErrTryAcquireExclusiveLatch();
        if ( errSXWL == CSXWLatch::ERR::errSuccess )
        {
            psxwl->ReleaseSharedLatch();
            pbfl->dwContext = DWORD_PTR( pbf );
        }
    }


    if ( errSXWL == CSXWLatch::ERR::errLatchConflict )
    {

        PERFOpt( cBFLatchConflict.Inc( perfinstGlobal ) );
        Error( ErrERRCheck( errBFLatchConflict ) );
    }


    Assert( errSXWL == CSXWLatch::ERR::errSuccess );


    (void)ErrBFIValidatePage( pbf, bfltExclusive, CPageValidationLogEvent::LOG_ALL, *TraceContextScope() );

    Assert( PBF( pbfl->dwContext )->err != errBFIPageNotVerified );
    Assert( PBF( pbfl->dwContext )->err != errBFIPageRemapNotReVerified );


    Assert( FBFICurrentPage( pbf, pbf->ifmp, pbf->pgno ) );


    err = ErrBFIMaintImpedingPageLatch( pbf, fFalse, pbfl );
    if ( err < JET_errSuccess )
    {

        Expected( JET_errOutOfMemory == err || errBFLatchConflict == err );
        Assert( pbf == PBF( pbfl->dwContext ) );
        PBF( pbfl->dwContext )->sxwl.DowngradeExclusiveLatchToSharedLatch();
        Call( err );
    }
    if ( wrnBFLatchMaintConflict == err )
    {
        Assert( ( 0x1 & (DWORD_PTR)PBF( pbfl->dwContext ) ) == 0x0 );
        Assert( pbf != PBF( pbfl->dwContext ) );
        Assert( pbf->sxwl.FNotOwner() );


        PBF( pbfl->dwContext )->sxwl.DowngradeExclusiveLatchToSharedLatch();
        Call( ErrERRCheck( errBFLatchConflict ) );
    }
    else
    {
        Assert( JET_errSuccess == err );
        Assert( pbf == PBF( pbfl->dwContext ) );
    }
    Assert( FBFCurrentLatch( pbfl ) );
    Assert( FBFUpdatableLatch( pbfl ) );

HandleError:


#ifdef DEBUG
    Assert( FBFCurrentLatch( pbfl ) );

    if ( err >= JET_errSuccess )
    {
        Assert( FBFRDWLatched( pbfl ) );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageNotVerified );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageRemapNotReVerified );
        Expected( FBFUpdatableLatch( pbfl ) );
    }
    else
    {
        Assert( FBFReadLatched( pbfl ) );
    }
#endif

    return err;
}


ERR ErrBFUpgradeReadLatchToWARLatch( BFLatch* pbfl )
{

    Assert( FBFReadLatched( pbfl ) );

    ERR             err;
    PBF             pbf;
    CSXWLatch*      psxwl;
    CSXWLatch::ERR  errSXWL;


    if ( BoolParam( JET_paramEnableViewCache ) )
    {
        CallR( ErrERRCheck( errBFLatchConflict ) );
    }


    if ( pbfl->dwContext & 1 )
    {
        pbf     = ((BFHashedLatch*)( pbfl->dwContext ^ 1 ))->pbf;
        psxwl   = &((BFHashedLatch*)( pbfl->dwContext ^ 1 ))->sxwl;
    }
    else
    {
        pbf     = PBF( pbfl->dwContext );
        psxwl   = &pbf->sxwl;
    }


    if ( psxwl == &pbf->sxwl )
    {
        errSXWL = pbf->sxwl.ErrUpgradeSharedLatchToExclusiveLatch();
    }
    else
    {
        errSXWL = pbf->sxwl.ErrTryAcquireExclusiveLatch();
        if ( errSXWL == CSXWLatch::ERR::errSuccess )
        {
            psxwl->ReleaseSharedLatch();
            pbfl->dwContext = DWORD_PTR( pbf );
        }
    }


    if ( errSXWL == CSXWLatch::ERR::errLatchConflict )
    {

        PERFOpt( cBFLatchConflict.Inc( perfinstGlobal ) );
        Error( ErrERRCheck( errBFLatchConflict ) );
    }


    Assert( errSXWL == CSXWLatch::ERR::errSuccess );


    if ( !FBFIUpdatablePage( pbf ) )
    {
        Assert( pbf->err == wrnBFPageFlushPending );
        if ( FBFICompleteFlushPage( pbf, bfltExclusive ) )
        {

            Assert( FBFIUpdatablePage( pbf ) );
        }

        Assert( pbf->err != errBFIPageRemapNotReVerified );

        if ( !FBFIUpdatablePage( pbf )  ||
                pbf->err < JET_errSuccess  )
        {

            pbf->sxwl.DowngradeExclusiveLatchToSharedLatch();
            PERFOpt( cBFLatchConflict.Inc( perfinstGlobal ) );
            Error( ErrERRCheck( errBFLatchConflict ) );
        }
    }


    (void)ErrBFIValidatePage( pbf, bfltExclusive, CPageValidationLogEvent::LOG_ALL, *TraceContextScope() );

    Assert( PBF( pbfl->dwContext )->err != errBFIPageNotVerified );
    Assert( PBF( pbfl->dwContext )->err != errBFIPageRemapNotReVerified );


    pbf->fWARLatch = fTrue;

    Assert( FBFICurrentPage( pbf, pbf->ifmp, pbf->pgno ) );

    err = JET_errSuccess;

HandleError:


#ifdef DEBUG
    Assert( FBFCurrentLatch( pbfl ) );

    if ( err >= JET_errSuccess )
    {
        Assert( FBFWARLatched( pbfl ) );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageNotVerified );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageRemapNotReVerified );
        Assert( FBFUpdatableLatch( pbfl ) );
    }
    else
    {
        Assert( FBFReadLatched( pbfl ) );
    }
#endif

    return err;
}

ERR ErrBFUpgradeReadLatchToWriteLatch( BFLatch* pbfl, const BOOL fCOWAllowed )
{

    Assert( FBFReadLatched( pbfl ) );

    ERR             err;
    PBF             pbf;
    CSXWLatch*      psxwl;
    CSXWLatch::ERR  errSXWL;
    RBS_POS         rbsposSnapshot = rbsposMin;


    if ( pbfl->dwContext & 1 )
    {
        pbf     = ((BFHashedLatch*)( pbfl->dwContext ^ 1 ))->pbf;
        psxwl   = &((BFHashedLatch*)( pbfl->dwContext ^ 1 ))->sxwl;
    }
    else
    {
        pbf     = PBF( pbfl->dwContext );
        psxwl   = &pbf->sxwl;
    }


    if ( psxwl == &pbf->sxwl )
    {
        errSXWL = pbf->sxwl.ErrUpgradeSharedLatchToWriteLatch();
    }
    else
    {
        errSXWL = pbf->sxwl.ErrTryAcquireExclusiveLatch();
        if ( errSXWL == CSXWLatch::ERR::errSuccess )
        {
            psxwl->ReleaseSharedLatch();
            errSXWL = pbf->sxwl.ErrUpgradeExclusiveLatchToWriteLatch();
            pbfl->dwContext = DWORD_PTR( pbf );
        }
    }


    if ( errSXWL == CSXWLatch::ERR::errLatchConflict )
    {

        PERFOpt( cBFLatchConflict.Inc( perfinstGlobal ) );
        Error( ErrERRCheck( errBFLatchConflict ) );
    }


    Assert( errSXWL == CSXWLatch::ERR::errSuccess ||
            errSXWL == CSXWLatch::ERR::errWaitForWriteLatch );


    if ( errSXWL == CSXWLatch::ERR::errWaitForWriteLatch )
    {
        PERFOpt( cBFLatchStall.Inc( perfinstGlobal ) );
        pbf->sxwl.WaitForWriteLatch();
    }

#ifdef MINIMAL_FUNCTIONALITY
#else


    if ( pbf->bfls == bflsHashed )
    {
        const size_t    cProcs  = (size_t)OSSyncGetProcessorCountMax();
        for ( size_t iProc = 0; iProc < cProcs; iProc++ )
        {
            CSXWLatch* const psxwlProc = &Ppls( iProc )->rgBFHashedLatch[ pbf->iHashedLatch ].sxwl;
            if ( psxwlProc->ErrAcquireExclusiveLatch() == CSXWLatch::ERR::errWaitForExclusiveLatch )
            {
                PERFOpt( cBFLatchStall.Inc( perfinstGlobal ) );
                psxwlProc->WaitForExclusiveLatch();
            }
            if ( psxwlProc->ErrUpgradeExclusiveLatchToWriteLatch() == CSXWLatch::ERR::errWaitForWriteLatch )
            {
                PERFOpt( cBFLatchStall.Inc( perfinstGlobal ) );
                psxwlProc->WaitForWriteLatch();
            }
        }
    }

#endif


    (void)ErrBFIValidatePage( pbf, bfltWrite, CPageValidationLogEvent::LOG_ALL, *TraceContextScope() );

    Assert( PBF( pbfl->dwContext )->err != errBFIPageNotVerified );
    Assert( PBF( pbfl->dwContext )->err != errBFIPageRemapNotReVerified );

    if ( !fCOWAllowed && !FBFIUpdatablePage( PBF( pbfl->dwContext ) ) )
    {

        PBF( pbfl->dwContext )->sxwl.DowngradeWriteLatchToSharedLatch();
        Call( ErrERRCheck( errBFLatchConflict ) );
    }

    err = ErrBFICapturePagePreimage( PBF( pbfl->dwContext ), &rbsposSnapshot );
    if ( err < JET_errSuccess )
    {
        PBF( pbfl->dwContext )->sxwl.DowngradeWriteLatchToSharedLatch();
        goto HandleError;
    }


    err = ErrBFIMaintImpedingPageLatch( pbf, fTrue, pbfl );
    if ( err < JET_errSuccess )
    {

        Expected( JET_errOutOfMemory == err || JET_errOutOfBuffers == err || errBFLatchConflict == err );
        Assert( pbf == PBF( pbfl->dwContext ) );
        PBF( pbfl->dwContext )->sxwl.DowngradeWriteLatchToSharedLatch();
        Call( err );
    }
    if ( wrnBFLatchMaintConflict == err )
    {
        Assert( ( 0x1 & (DWORD_PTR)PBF( pbfl->dwContext ) ) == 0x0 );
        Assert( pbf != PBF( pbfl->dwContext ) );
        Assert( pbf->sxwl.FNotOwner() );


        PBF( pbfl->dwContext )->sxwl.DowngradeWriteLatchToSharedLatch();
        Call( ErrERRCheck( errBFLatchConflict ) );
    }
    else
    {
        Assert( JET_errSuccess == err );
        Assert( pbf == PBF( pbfl->dwContext ) );
    }

    PBF( pbfl->dwContext )->rbsposSnapshot = rbsposSnapshot;


    BFIRehydratePage( PBF( pbfl->dwContext ) );

    Assert( FBFCurrentLatch( pbfl ) );
    Assert( FBFUpdatableLatch( pbfl ) );
    Assert( PBF( pbfl->dwContext )->icbBuffer == PBF( pbfl->dwContext )->icbPage );

HandleError:


#ifdef DEBUG
    Assert( FBFCurrentLatch( pbfl ) );

    if ( err >= JET_errSuccess )
    {
        Assert( FBFWriteLatched( pbfl ) );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageNotVerified );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageRemapNotReVerified );
        Assert( FBFUpdatableLatch( pbfl ) );
        Assert( PBF( pbfl->dwContext )->icbBuffer == PBF( pbfl->dwContext )->icbPage );
    }
    else
    {
        Assert( FBFReadLatched( pbfl ) );
    }
#endif

    return err;
}

ERR ErrBFUpgradeRDWLatchToWARLatch( BFLatch* pbfl )
{

    Assert( FBFRDWLatched( pbfl ) );

    PBF pbf = PBF( pbfl->dwContext );




    pbf->fWARLatch = fTrue;


    Assert( FBFWARLatched( pbfl ) );
    Assert( FBFCurrentLatch( pbfl ) );
    Assert( PBF( pbfl->dwContext )->err != errBFIPageNotVerified );
    Assert( PBF( pbfl->dwContext )->err != errBFIPageRemapNotReVerified );
    Assert( FBFUpdatableLatch( pbfl ) );

    return JET_errSuccess;
}

ERR ErrBFUpgradeRDWLatchToWriteLatch( BFLatch* pbfl )
{

    Assert( FBFRDWLatched( pbfl ) );

    ERR             err = JET_errSuccess;
    PBF             pbf = PBF( pbfl->dwContext );
    CSXWLatch::ERR  errSXWL;
    RBS_POS         rbsposSnapshot = rbsposMin;


    errSXWL = pbf->sxwl.ErrUpgradeExclusiveLatchToWriteLatch();

    Assert( errSXWL == CSXWLatch::ERR::errSuccess ||
            errSXWL == CSXWLatch::ERR::errWaitForWriteLatch );


    if ( errSXWL == CSXWLatch::ERR::errWaitForWriteLatch )
    {
        PERFOpt( cBFLatchStall.Inc( perfinstGlobal ) );
        pbf->sxwl.WaitForWriteLatch();
    }

    Assert( FBFUpdatableLatch( pbfl ) );

#ifdef MINIMAL_FUNCTIONALITY
#else


    if ( pbf->bfls == bflsHashed )
    {
        const size_t    cProcs  = (size_t)OSSyncGetProcessorCountMax();
        for ( size_t iProc = 0; iProc < cProcs; iProc++ )
        {
            CSXWLatch* const psxwlProc = &Ppls( iProc )->rgBFHashedLatch[ pbf->iHashedLatch ].sxwl;
            if ( psxwlProc->ErrAcquireExclusiveLatch() == CSXWLatch::ERR::errWaitForExclusiveLatch )
            {
                PERFOpt( cBFLatchStall.Inc( perfinstGlobal ) );
                psxwlProc->WaitForExclusiveLatch();
            }
            if ( psxwlProc->ErrUpgradeExclusiveLatchToWriteLatch() == CSXWLatch::ERR::errWaitForWriteLatch )
            {
                PERFOpt( cBFLatchStall.Inc( perfinstGlobal ) );
                psxwlProc->WaitForWriteLatch();
            }
        }
    }

#endif

    Assert( FBFICurrentPage( PBF( pbfl->dwContext ), PBF( pbfl->dwContext )->ifmp, PBF( pbfl->dwContext )->pgno ) );

    err = ErrBFICapturePagePreimage( PBF( pbfl->dwContext ), &rbsposSnapshot );
    if ( err < JET_errSuccess )
    {
        PBF( pbfl->dwContext )->sxwl.DowngradeWriteLatchToExclusiveLatch();
        goto HandleError;
    }


    BFIMaintImpedingPage( PBF( pbfl->dwContext ) );

    PBF( pbfl->dwContext )->rbsposSnapshot = rbsposSnapshot;


    BFIRehydratePage( PBF( pbfl->dwContext ) );

HandleError:


#ifdef DEBUG
    Assert( FBFCurrentLatch( pbfl ) );

    if ( err >= JET_errSuccess )
    {
        Assert( FBFWriteLatched( pbfl ) );
        Assert( FBFCurrentLatch( pbfl ) );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageNotVerified );
        Assert( PBF( pbfl->dwContext )->err != errBFIPageRemapNotReVerified );
        Assert( FBFUpdatableLatch( pbfl ) );
        Assert( PBF( pbfl->dwContext )->icbBuffer == PBF( pbfl->dwContext )->icbPage );
    }
    else
    {
        Assert( FBFRDWLatched( pbfl ) );
    }
#endif

    return err;
}

void BFDowngradeWriteLatchToRDWLatch( BFLatch* pbfl )
{

    Assert( FBFWriteLatched( pbfl ) );

    PBF pbf = PBF( pbfl->dwContext );

#ifdef MINIMAL_FUNCTIONALITY
#else


    if ( pbf->bfls == bflsHashed )
    {
        const size_t    cProcs  = (size_t)OSSyncGetProcessorCountMax();
        for ( size_t iProc = 0; iProc < cProcs; iProc++ )
        {
            CSXWLatch* const psxwlProc = &Ppls( iProc )->rgBFHashedLatch[ pbf->iHashedLatch ].sxwl;
            psxwlProc->ReleaseWriteLatch();
        }
    }

#endif

    BFIValidatePagePgno( pbf );


    BFIValidatePageUsed( pbf );


    BFIDehydratePage( pbf, fFalse );


    pbf->sxwl.DowngradeWriteLatchToExclusiveLatch();


    Assert( FBFRDWLatched( pbfl ) );
}

void BFDowngradeWARLatchToRDWLatch( BFLatch* pbfl )
{

    Assert( FBFWARLatched( pbfl ) );

    PBF pbf = PBF( pbfl->dwContext );


    pbf->fWARLatch = fFalse;


    Assert( FBFRDWLatched( pbfl ) );
}

void BFDowngradeWriteLatchToReadLatch( BFLatch* pbfl )
{

    Assert( FBFWriteLatched( pbfl ) );

    PBF pbf = PBF( pbfl->dwContext );

#ifdef MINIMAL_FUNCTIONALITY
#else


    if ( pbf->bfls == bflsHashed )
    {
        const size_t    cProcs  = (size_t)OSSyncGetProcessorCountMax();
        for ( size_t iProc = 0; iProc < cProcs; iProc++ )
        {
            CSXWLatch* const psxwlProc = &Ppls( iProc )->rgBFHashedLatch[ pbf->iHashedLatch ].sxwl;
            psxwlProc->ReleaseWriteLatch();
        }
    }

#endif

    BFIValidatePagePgno( pbf );


    BFIValidatePageUsed( pbf );


    PBF     pbfOpportunisticCheckpointAdv = NULL;
    BFIOpportunisticallyVersionPage( pbf, &pbfOpportunisticCheckpointAdv );


    Assert( pbf->bfdf != bfdfClean );


    BFIDehydratePage( pbf, fFalse );


    pbf->sxwl.DowngradeWriteLatchToSharedLatch();


    if ( pbfOpportunisticCheckpointAdv )
    {
        BFIOpportunisticallyFlushPage( pbfOpportunisticCheckpointAdv, iorpBFCheckpointAdv );
    }


    Assert( FBFReadLatched( pbfl ) );
}

void BFDowngradeWARLatchToReadLatch( BFLatch* pbfl )
{

    Assert( FBFWARLatched( pbfl ) );

    PBF pbf = PBF( pbfl->dwContext );


    BFIValidatePageUsed( pbf );


    pbf->fWARLatch = fFalse;


    PBF     pbfOpportunisticCheckpointAdv = NULL;
    BFIOpportunisticallyVersionPage( pbf, &pbfOpportunisticCheckpointAdv );


    pbf->sxwl.DowngradeExclusiveLatchToSharedLatch();


    if ( pbfOpportunisticCheckpointAdv )
    {
        BFIOpportunisticallyFlushPage( pbfOpportunisticCheckpointAdv, iorpBFCheckpointAdv );
    }


    Assert( FBFReadLatched( pbfl ) );
}

void BFDowngradeRDWLatchToReadLatch( BFLatch* pbfl )
{

    Assert( FBFRDWLatched( pbfl ) );

    PBF pbf = PBF( pbfl->dwContext );


    BFIValidatePageUsed( pbf );


    PBF     pbfOpportunisticCheckpointAdv = NULL;
    BFIOpportunisticallyVersionPage( pbf, &pbfOpportunisticCheckpointAdv );


    pbf->sxwl.DowngradeExclusiveLatchToSharedLatch();


    if ( pbfOpportunisticCheckpointAdv )
    {
        BFIOpportunisticallyFlushPage( pbfOpportunisticCheckpointAdv, iorpBFCheckpointAdv );
    }


    Assert( FBFReadLatched( pbfl ) );
}

void BFWriteUnlatch( BFLatch* pbfl )
{

    Assert( FBFWriteLatched( pbfl ) );

    const PBF pbf = PBF( pbfl->dwContext );

#ifdef MINIMAL_FUNCTIONALITY
#else


    if ( pbf->bfls == bflsHashed )
    {
        const size_t    cProcs  = (size_t)OSSyncGetProcessorCountMax();
        for ( size_t iProc = 0; iProc < cProcs; iProc++ )
        {
            CSXWLatch* const psxwlProc = &Ppls( iProc )->rgBFHashedLatch[ pbf->iHashedLatch ].sxwl;
            psxwlProc->ReleaseWriteLatch();
        }
    }

#endif


    if ( pbf->bfdf > bfdfClean )
    {
        BFIValidatePagePgno( pbf );
        BFIValidatePageUsed( pbf );
    }


    const IFMP ifmp = pbf->ifmp;
    const PGNO pgno = pbf->pgno;

    Assert( pgno <= g_rgfmp[ ifmp ].PgnoLast() ||
            g_rgfmp[ ifmp ].FBeyondPgnoShrinkTarget( pgno ) ||
            g_rgfmp[ ifmp ].FOlderDemandExtendDb() );

    BFIDehydratePage( pbf, fTrue );


    if ( pbf->bfdf == bfdfClean )
    {
        pbf->sxwl.ReleaseWriteLatch();
    }
    else
    {

        BFIUnlatchMaintPage( pbf, bfltWrite );
    }


    Assert( FBFNotLatched( ifmp, pgno ) );
}

void BFWARUnlatch( BFLatch* pbfl )
{

    Assert( FBFWARLatched( pbfl ) );


    PBF( pbfl->dwContext )->fWARLatch = fFalse;


    BFRDWUnlatch( pbfl );
}

void BFRDWUnlatch( BFLatch* pbfl )
{

    Assert( FBFRDWLatched( pbfl ) );


    const PBF pbf = PBF( pbfl->dwContext );


    BFIValidatePageUsed( pbf );


    const IFMP ifmp = pbf->ifmp;
    const PGNO pgno = pbf->pgno;

    if ( pbf->bfdf == bfdfClean )
    {
        pbf->sxwl.ReleaseExclusiveLatch();
    }
    else
    {

        BFIUnlatchMaintPage( pbf, bfltExclusive );
    }


    Assert( FBFNotLatched( ifmp, pgno ) );
}

void BFReadUnlatch( BFLatch* pbfl )
{

    Assert( FBFReadLatched( pbfl ) );


    PBF             pbf;
    CSXWLatch*      psxwl;

    if ( pbfl->dwContext & 1 )
    {
        pbf     = ((BFHashedLatch*)( pbfl->dwContext ^ 1 ))->pbf;
        psxwl   = &((BFHashedLatch*)( pbfl->dwContext ^ 1 ))->sxwl;
    }
    else
    {
        pbf     = PBF( pbfl->dwContext );
        psxwl   = &pbf->sxwl;
    }


    const IFMP ifmp = pbf->ifmp;
    const PGNO pgno = pbf->pgno;

    if ( pbf->bfdf != bfdfFilthy )
    {

        psxwl->ReleaseSharedLatch();
    }
    else
    {

        BFIUnlatchMaintPage( pbf, bfltShared );
    }


    Assert( FBFNotLatched( ifmp, pgno ) );
}

void BFMarkAsSuperCold( IFMP ifmp, PGNO pgno, const BFLatchFlags bflf )
{
    
    TraceContextScope tcScope;
    tcScope->nParentObjectClass = tceNone;

    BFLatch bfl;
    ERR err = ErrBFRDWLatchPage( 
                    &bfl,
                    ifmp,
                    pgno,
                    BFLatchFlags( bflfNoTouch | bflfNoFaultFail | bflfUninitPageOk | bflfNoWait | bflfNoEventLogging | bflfNoUncached | bflf ),
                    BfpriBFMake( g_pctCachePriorityNeutral, (BFTEMPOSFILEQOS)0  ),
                    *tcScope );

    if ( err >= JET_errSuccess )
    {
        const ERR errBFLatchStatus = ErrBFLatchStatus( &bfl );
        
        if( ( errBFLatchStatus >= JET_errSuccess ) || ( errBFLatchStatus == JET_errPageNotInitialized ) )
        {

            Expected( !PagePatching::FIsPatchableError( errBFLatchStatus ) );
            
            BFMarkAsSuperCold( &bfl );
        }

        BFRDWUnlatch( &bfl );
    }
}

void BFMarkAsSuperCold( BFLatch *pbfl )
{
    Assert( FBFRDWLatched( pbfl ) || FBFWriteLatched( pbfl ) );


    const PBF pbf = PbfBFILatchContext( pbfl->dwContext );
    
    BFIMarkAsSuperCold( pbf, fTrue );
}

void BFCacheStatus( const IFMP ifmp, const PGNO pgno, BOOL* const pfInCache, ERR* const perrBF, BFDirtyFlags* const pbfdf )
{
    *pfInCache = fFalse;
    ( perrBF != NULL ) ? ( *perrBF = JET_errSuccess ) : 0;
    ( pbfdf != NULL ) ? ( *pbfdf = bfdfMin ) : 0;

    if ( g_fBFCacheInitialized )
    {
        PGNOPBF         pgnopbf;
        BFHash::ERR     errHash;
        BFHash::CLock   lock;

        g_bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
        errHash = g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf );
        if ( errHash == BFHash::ERR::errSuccess )
        {
            *pfInCache = fTrue;
            ( perrBF != NULL ) ? ( *perrBF = pgnopbf.pbf->err ) : 0;
            ( pbfdf != NULL ) ? ( *pbfdf = (BFDirtyFlags)pgnopbf.pbf->bfdf ) : 0;
        }
        g_bfhash.ReadUnlockKey( &lock );
    }
}

BOOL FBFInCache( IFMP ifmp, PGNO pgno )
{
    BOOL fInCache = fFalse;
    BFCacheStatus( ifmp, pgno, &fInCache );
    return fInCache;
}


BOOL FBFPreviouslyCached( IFMP ifmp, PGNO pgno )
{
    BOOL fInCache       = fFalse;
    ERR errBF           = JET_errSuccess;

    BFCacheStatus( ifmp, pgno, &fInCache, &errBF );

    return ( fInCache &&
                ( errBF != errBFIPageFaultPending ) &&
                ( errBF != errBFIPageNotVerified ) );
}

#ifdef DEBUG

BOOL FBFReadLatched( const BFLatch* pbfl )
{
    return  FBFICacheValidPv( pbfl->pv ) &&
            FBFILatchValidContext( pbfl->dwContext ) &&
            pbfl->pv == PbfBFILatchContext( pbfl->dwContext )->pv &&
            PsxwlBFILatchContext( pbfl->dwContext )->FOwnSharedLatch() &&
            FBFCurrentLatch( pbfl );
}

BOOL FBFNotReadLatched( const BFLatch* pbfl )
{
    return  !FBFICacheValidPv( pbfl->pv ) ||
            !FBFILatchValidContext( pbfl->dwContext ) ||
            pbfl->pv != PbfBFILatchContext( pbfl->dwContext )->pv ||
            PsxwlBFILatchContext( pbfl->dwContext )->FNotOwnSharedLatch();
}

BOOL FBFReadLatched( IFMP ifmp, PGNO pgno )
{
    PGNOPBF         pgnopbf;
    BFHash::ERR     errHash;
    BFHash::CLock   lock;
    BOOL            fReadLatched;


    g_bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
    errHash = g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf );


    fReadLatched = errHash == BFHash::ERR::errSuccess;

    if ( fReadLatched )
    {
        fReadLatched = ( pgnopbf.pbf->sxwl.FOwnSharedLatch() &&
                            FBFICurrentPage( pgnopbf.pbf, ifmp, pgno ) );

        const size_t iHashedLatch = pgnopbf.pbf->iHashedLatch;
        if ( iHashedLatch < cBFHashedLatch )
        {
            const size_t    cProcs  = (size_t)OSSyncGetProcessorCountMax();
            for ( size_t iProc = 0; iProc < cProcs; iProc++ )
            {
                BFHashedLatch* const pbfhl = &Ppls( iProc )->rgBFHashedLatch[ iHashedLatch ];
                fReadLatched = fReadLatched ||
                                    ( pbfhl->sxwl.FOwnSharedLatch() &&
                                        pbfhl->pbf == pgnopbf.pbf &&
                                        FBFICurrentPage( pgnopbf.pbf, ifmp, pgno ) );
            }
        }
    }


    g_bfhash.ReadUnlockKey( &lock );


    return fReadLatched;
}

BOOL FBFNotReadLatched( IFMP ifmp, PGNO pgno )
{
    PGNOPBF         pgnopbf;
    BFHash::ERR     errHash;
    BFHash::CLock   lock;
    BOOL            fNotReadLatched;


    g_bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
    errHash = g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf );


    fNotReadLatched = errHash == BFHash::ERR::errEntryNotFound;

    if ( !fNotReadLatched )
    {
        fNotReadLatched = pgnopbf.pbf->sxwl.FNotOwnSharedLatch();

        const size_t iHashedLatch = pgnopbf.pbf->iHashedLatch;
        if ( iHashedLatch < cBFHashedLatch )
        {
            const size_t    cProcs  = (size_t)OSSyncGetProcessorCountMax();
            for ( size_t iProc = 0; iProc < cProcs; iProc++ )
            {
                BFHashedLatch* const pbfhl = &Ppls( iProc )->rgBFHashedLatch[ iHashedLatch ];
                fNotReadLatched = fNotReadLatched && ( pbfhl->sxwl.FNotOwnSharedLatch() || pbfhl->pbf != pgnopbf.pbf );
            }
        }
    }


    g_bfhash.ReadUnlockKey( &lock );


    return fNotReadLatched;
}

BOOL FBFRDWLatched( const BFLatch* pbfl )
{
    return  FBFICacheValidPv( pbfl->pv ) &&
            FBFILatchValidContext( pbfl->dwContext ) &&
            FBFICacheValidPbf( PBF( pbfl->dwContext ) ) &&
            pbfl->pv == PBF( pbfl->dwContext )->pv &&
            !PBF( pbfl->dwContext )->fWARLatch &&
            PBF( pbfl->dwContext )->sxwl.FOwnExclusiveLatch() &&
            FBFCurrentLatch( pbfl ) &&
            FBFUpdatableLatch( pbfl );
}

BOOL FBFNotRDWLatched( const BFLatch* pbfl )
{
    return  !FBFICacheValidPv( pbfl->pv ) ||
            !FBFILatchValidContext( pbfl->dwContext ) ||
            !FBFICacheValidPbf( PBF( pbfl->dwContext ) ) ||
            pbfl->pv != PBF( pbfl->dwContext )->pv ||
            PBF( pbfl->dwContext )->fWARLatch ||
            PBF( pbfl->dwContext )->sxwl.FNotOwnExclusiveLatch();
}

BOOL FBFRDWLatched( IFMP ifmp, PGNO pgno )
{
    PGNOPBF         pgnopbf;
    BFHash::ERR     errHash;
    BFHash::CLock   lock;
    BOOL            fRDWLatched;


    g_bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
    errHash = g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf );


    fRDWLatched =   errHash == BFHash::ERR::errSuccess &&
                    !pgnopbf.pbf->fWARLatch &&
                    pgnopbf.pbf->sxwl.FOwnExclusiveLatch() &&
                    FBFICurrentPage( pgnopbf.pbf, ifmp, pgno ) &&
                    FBFIUpdatablePage( pgnopbf.pbf );


    g_bfhash.ReadUnlockKey( &lock );


    return fRDWLatched;
}

BOOL FBFNotRDWLatched( IFMP ifmp, PGNO pgno )
{
    PGNOPBF         pgnopbf;
    BFHash::ERR     errHash;
    BFHash::CLock   lock;
    BOOL            fNotRDWLatched;


    g_bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
    errHash = g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf );


    fNotRDWLatched =    errHash == BFHash::ERR::errEntryNotFound ||
                        pgnopbf.pbf->fWARLatch ||
                        pgnopbf.pbf->sxwl.FNotOwnExclusiveLatch();


    g_bfhash.ReadUnlockKey( &lock );


    return fNotRDWLatched;
}

BOOL FBFWARLatched( const BFLatch* pbfl )
{
    return  FBFICacheValidPv( pbfl->pv ) &&
            FBFILatchValidContext( pbfl->dwContext ) &&
            FBFICacheValidPbf( PBF( pbfl->dwContext ) ) &&
            pbfl->pv == PBF( pbfl->dwContext )->pv &&
            PBF( pbfl->dwContext )->fWARLatch &&
            PBF( pbfl->dwContext )->sxwl.FOwnExclusiveLatch() &&
            FBFCurrentLatch( pbfl ) &&
            FBFUpdatableLatch( pbfl );
}

BOOL FBFNotWARLatched( const BFLatch* pbfl )
{
    return  !FBFICacheValidPv( pbfl->pv ) ||
            !FBFILatchValidContext( pbfl->dwContext ) ||
            !FBFICacheValidPbf( PBF( pbfl->dwContext ) ) ||
            pbfl->pv != PBF( pbfl->dwContext )->pv ||
            !PBF( pbfl->dwContext )->fWARLatch ||
            PBF( pbfl->dwContext )->sxwl.FNotOwnExclusiveLatch();
}

BOOL FBFWARLatched( IFMP ifmp, PGNO pgno )
{
    PGNOPBF         pgnopbf;
    BFHash::ERR     errHash;
    BFHash::CLock   lock;
    BOOL            fWARLatched;


    g_bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
    errHash = g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf );


    fWARLatched =   errHash == BFHash::ERR::errSuccess &&
                    pgnopbf.pbf->fWARLatch &&
                    pgnopbf.pbf->sxwl.FOwnExclusiveLatch() &&
                    FBFICurrentPage( pgnopbf.pbf, ifmp, pgno ) &&
                    FBFIUpdatablePage( pgnopbf.pbf );


    g_bfhash.ReadUnlockKey( &lock );


    return fWARLatched;
}

BOOL FBFNotWARLatched( IFMP ifmp, PGNO pgno )
{
    PGNOPBF         pgnopbf;
    BFHash::ERR     errHash;
    BFHash::CLock   lock;
    BOOL            fNotWARLatched;


    g_bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
    errHash = g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf );


    fNotWARLatched =    errHash == BFHash::ERR::errEntryNotFound ||
                        !pgnopbf.pbf->fWARLatch ||
                        pgnopbf.pbf->sxwl.FNotOwnExclusiveLatch();


    g_bfhash.ReadUnlockKey( &lock );


    return fNotWARLatched;
}

BOOL FBFWriteLatched( const BFLatch* pbfl )
{
    BOOL fWriteLatched;

    fWriteLatched = FBFICacheValidPv( pbfl->pv ) &&
                    FBFILatchValidContext( pbfl->dwContext ) &&
                    FBFICacheValidPbf( PBF( pbfl->dwContext ) ) &&
                    pbfl->pv == PBF( pbfl->dwContext )->pv &&
                    PBF( pbfl->dwContext )->sxwl.FOwnWriteLatch() &&
                    FBFCurrentLatch( pbfl ) &&
                    FBFUpdatableLatch( pbfl );

#ifdef MINIMAL_FUNCTIONALITY
#else

    if ( fWriteLatched && PBF( pbfl->dwContext )->bfls == bflsHashed )
    {
        const size_t    iHashedLatch    = PBF( pbfl->dwContext )->iHashedLatch;
        const size_t    cProcs          = (size_t)OSSyncGetProcessorCountMax();
        for ( size_t iProc = 0; iProc < cProcs; iProc++ )
        {
            BFHashedLatch* const pbfhl = &Ppls( iProc )->rgBFHashedLatch[ iHashedLatch ];
            fWriteLatched = fWriteLatched && pbfhl->sxwl.FOwnWriteLatch();
        }
    }

#endif

    return fWriteLatched;
}

BOOL FBFNotWriteLatched( const BFLatch* pbfl )
{
    BOOL fNotWriteLatched;

    fNotWriteLatched =  !FBFICacheValidPv( pbfl->pv ) ||
                        !FBFILatchValidContext( pbfl->dwContext ) ||
                        !FBFICacheValidPbf( PBF( pbfl->dwContext ) ) ||
                        pbfl->pv != PBF( pbfl->dwContext )->pv ||
                        PBF( pbfl->dwContext )->sxwl.FNotOwnWriteLatch();

#ifdef MINIMAL_FUNCTIONALITY
#else

    if ( !fNotWriteLatched && PBF( pbfl->dwContext )->bfls == bflsHashed )
    {
        const size_t    iHashedLatch    = PBF( pbfl->dwContext )->iHashedLatch;
        const size_t    cProcs          = (size_t)OSSyncGetProcessorCountMax();
        for ( size_t iProc = 0; iProc < cProcs; iProc++ )
        {
            BFHashedLatch* const pbfhl = &Ppls( iProc )->rgBFHashedLatch[ iHashedLatch ];
            fNotWriteLatched = fNotWriteLatched || pbfhl->sxwl.FNotOwnWriteLatch();
        }
    }

#endif

    return fNotWriteLatched;
}

BOOL FBFWriteLatched( IFMP ifmp, PGNO pgno )
{
    PGNOPBF         pgnopbf;
    BFHash::ERR     errHash;
    BFHash::CLock   lock;
    BOOL            fWriteLatched;


    g_bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
    errHash = g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf );


    fWriteLatched = errHash == BFHash::ERR::errSuccess &&
                    pgnopbf.pbf->sxwl.FOwnWriteLatch() &&
                    FBFICurrentPage( pgnopbf.pbf, ifmp, pgno ) &&
                    FBFIUpdatablePage( pgnopbf.pbf );

#ifdef MINIMAL_FUNCTIONALITY
#else

    if ( fWriteLatched && pgnopbf.pbf->bfls == bflsHashed )
    {
        const size_t    iHashedLatch    = pgnopbf.pbf->iHashedLatch;
        const size_t    cProcs          = (size_t)OSSyncGetProcessorCountMax();
        for ( size_t iProc = 0; iProc < cProcs; iProc++ )
        {
            BFHashedLatch* const pbfhl = &Ppls( iProc )->rgBFHashedLatch[ iHashedLatch ];
            fWriteLatched = fWriteLatched && pbfhl->sxwl.FOwnWriteLatch();
        }
    }

#endif


    g_bfhash.ReadUnlockKey( &lock );


    return fWriteLatched;
}

BOOL FBFNotWriteLatched( IFMP ifmp, PGNO pgno )
{
    PGNOPBF         pgnopbf;
    BFHash::ERR     errHash;
    BFHash::CLock   lock;
    BOOL            fNotWriteLatched;


    g_bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
    errHash = g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf );


    fNotWriteLatched =  errHash == BFHash::ERR::errEntryNotFound ||
                        pgnopbf.pbf->sxwl.FNotOwnWriteLatch();

#ifdef MINIMAL_FUNCTIONALITY
#else

    if ( !fNotWriteLatched && pgnopbf.pbf->bfls == bflsHashed )
    {
        const size_t    iHashedLatch    = pgnopbf.pbf->iHashedLatch;
        const size_t    cProcs          = (size_t)OSSyncGetProcessorCountMax();
        for ( size_t iProc = 0; iProc < cProcs; iProc++ )
        {
            BFHashedLatch* const pbfhl = &Ppls( iProc )->rgBFHashedLatch[ iHashedLatch ];
            fNotWriteLatched = fNotWriteLatched || pbfhl->sxwl.FNotOwnWriteLatch();
        }
    }

#endif


    g_bfhash.ReadUnlockKey( &lock );


    return fNotWriteLatched;
}

BOOL FBFLatched( const BFLatch* pbfl )
{
    return  FBFReadLatched( pbfl ) ||
            FBFRDWLatched( pbfl ) ||
            FBFWARLatched( pbfl ) ||
            FBFWriteLatched( pbfl );
}

BOOL FBFNotLatched( const BFLatch* pbfl )
{
    return  FBFNotReadLatched( pbfl ) &&
            FBFNotRDWLatched( pbfl ) &&
            FBFNotWARLatched( pbfl ) &&
            FBFNotWriteLatched( pbfl );
}

BOOL FBFLatched( IFMP ifmp, PGNO pgno )
{
    return  FBFReadLatched( ifmp, pgno ) ||
            FBFRDWLatched( ifmp, pgno ) ||
            FBFWARLatched( ifmp, pgno ) ||
            FBFWriteLatched( ifmp, pgno );
}

BOOL FBFNotLatched( IFMP ifmp, PGNO pgno )
{
    return  FBFNotReadLatched( ifmp, pgno ) &&
            FBFNotRDWLatched( ifmp, pgno ) &&
            FBFNotWARLatched( ifmp, pgno ) &&
            FBFNotWriteLatched( ifmp, pgno );
}


BOOL FBFCurrentLatch( const BFLatch* pbfl, IFMP ifmp, PGNO pgno )
{
    return FBFICurrentPage( PbfBFILatchContext( pbfl->dwContext ), ifmp, pgno );
}

BOOL FBFCurrentLatch( const BFLatch* pbfl )
{
    const PBF pbf = PbfBFILatchContext( pbfl->dwContext );

    return FBFCurrentLatch( pbfl, pbf->ifmp, pbf->pgno );
}


BOOL FBFUpdatableLatch( const BFLatch* pbfl )
{
    Assert( FBFCurrentLatch( pbfl ) );

    return FBFIUpdatablePage( PbfBFILatchContext( pbfl->dwContext ) );
}

#endif

DWORD_PTR BFGetLatchHint( const BFLatch* pbfl )
{

    Assert( FBFLatched( pbfl ) );


    return (DWORD_PTR)PbfBFILatchContext( pbfl->dwContext );
}





ERR ErrBFLatchStatus( const BFLatch * pbfl )
{
    Assert( FBFRDWLatched( pbfl ) || FBFWARLatched( pbfl ) || FBFWriteLatched( pbfl ) );
    Expected( FBFRDWLatched( pbfl ) || FBFWriteLatched( pbfl ) );

    Expected( PBF( pbfl->dwContext )->err != JET_errOutOfMemory &&
                PBF( pbfl->dwContext )->err != JET_errOutOfBuffers );

    return PBF( pbfl->dwContext )->err;
}


void BFInitialize( BFLatch* pbfl, const TraceContext& tc )
{
    Assert( FBFWriteLatched( pbfl ) );
    const PBF pbf = PbfBFILatchContext( pbfl->dwContext );
    BFIInitialize( pbf, tc );
}


void BFDirty( const BFLatch* pbfl, BFDirtyFlags bfdf, const TraceContext& tc )
{

    Assert( FBFWARLatched( pbfl ) || FBFWriteLatched( pbfl ) );
    Assert( FBFUpdatableLatch( pbfl ) );

    if ( bfdf >= bfdfDirty )
    {
        Assert( FBFWriteLatched( pbfl ) );
        Assert( FBFNotWARLatched( pbfl ) );
    }

    const PBF pbf = PBF( pbfl->dwContext );


    if ( BoolParam( PinstFromIfmp( pbf->ifmp ), JET_paramEnableExternalAutoHealing ) )
    {
        Assert( FBFWARLatched( pbf->ifmp, pbf->pgno ) || FBFWriteLatched( pbf->ifmp, pbf->pgno ) );
        PagePatching::CancelPatchRequest( pbf->ifmp, pbf->pgno );
    }


    BFIDirtyPage( pbf, bfdf, tc );
}


BFDirtyFlags FBFDirty( const BFLatch* pbfl )
{

    Assert( FBFLatched( pbfl ) );


    return BFDirtyFlags( PBF( pbfl->dwContext )->bfdf );
}


LONG CbBFGetBufferSize( const LONG cbReqSize )
{
    ICBPage icbNewSize = max( icbPage4KB, IcbBFIBufferSize( cbReqSize ) );
    Assert( CbBFISize( icbNewSize ) >= cbReqSize );
    return CbBFISize( icbNewSize );
}



LONG CbBFBufferSize( const BFLatch* pbfl )
{



    return CbBFIBufferSize( PBF( pbfl->dwContext ) );
}


LONG CbBFPageSize( const BFLatch* pbfl )
{



    return CbBFIPageSize( PBF( pbfl->dwContext ) );
}


void BFSetBufferSize( __inout BFLatch* pbfl, __in const INT cbNewSize )
{
    Assert( FBFWriteLatched( pbfl ) );
    Assert( CbBFISize( IcbBFIBufferSize( cbNewSize ) ) == cbNewSize );

    CallS( ErrBFISetBufferSize( (PBF)pbfl->dwContext, IcbBFIBufferSize( cbNewSize ), fTrue ) );
}








void BFGetBestPossibleWaypoint(
    __in    IFMP        ifmp,
    __in    const LONG  lgenCommitted,
    __out   LGPOS *     plgposBestWaypoint )
{
    FMP* pfmp = &g_rgfmp[ ifmp  ];
    LOG * plog = PinstFromIfmp( ifmp )->m_plog;

    LGPOS lgposCurrentWaypoint;

    Assert( plgposBestWaypoint );
    Assert( plog && pfmp );



    lgposCurrentWaypoint = pfmp->LgposWaypoint();


    if ( plog->FLogDisabled() || !pfmp->FLogOn() )
    {
        Assert( CmpLgpos( lgposCurrentWaypoint, lgposMin ) == 0 );
        *plgposBestWaypoint = lgposMin;
        return;
    }


    LGPOS lgposPreferredWaypoint;

    if ( plog->FRecovering() && fRecoveringRedo == plog->FRecoveringMode() )
    {

        lgposPreferredWaypoint.lGeneration = plog->LgposLGLogTipNoLock().lGeneration;
    }
    else if ( 0 != lgenCommitted )
    {
        Assert( lgenCommitted == plog->LGGetCurrentFileGenNoLock()
                || lgenCommitted == plog->LGGetCurrentFileGenNoLock() + 1
                || lgenCommitted == plog->LGGetCurrentFileGenNoLock() - 1 );

        lgposPreferredWaypoint.lGeneration = lgenCommitted;
    }
    else
    {
        lgposPreferredWaypoint.lGeneration = plog->LGGetCurrentFileGenWithLock();
    }

    
    lgposPreferredWaypoint.isec = lgposMax.isec;
    lgposPreferredWaypoint.ib = lgposMax.ib;


    if ( pfmp->FNoWaypointLatency() ||
         PinstFromIfmp( ifmp )->m_fNoWaypointLatency )
    {

        ;

    }
    else
    {


        const LONG lWaypointDepth = (LONG)UlParam( PinstFromIfmp( ifmp ), JET_paramWaypointLatency );

        if ( 0 < ( lgposPreferredWaypoint.lGeneration - lWaypointDepth ) )
        {
            lgposPreferredWaypoint.lGeneration = lgposPreferredWaypoint.lGeneration - lWaypointDepth;
        }
        else if ( CmpLgpos( &lgposCurrentWaypoint, &lgposMin ) != 0 )
        {
            lgposPreferredWaypoint = lgposCurrentWaypoint;
        }
    }

    LGPOS lgposFlushTip;
    plog->LGFlushTip( &lgposFlushTip );
    lgposPreferredWaypoint.lGeneration = min( lgposPreferredWaypoint.lGeneration, lgposFlushTip.lGeneration );

    *plgposBestWaypoint = MaxLgpos( lgposCurrentWaypoint, lgposPreferredWaypoint );

}

void BFIMaintWaypointRequest(
    __in    IFMP    ifmp )
{
    FMP* pfmp = &g_rgfmp[ ifmp ];
    LGPOS lgposCurrentWaypoint;
    LGPOS lgposBestWaypoint;


    lgposCurrentWaypoint = pfmp->LgposWaypoint();

    BFGetBestPossibleWaypoint( ifmp, 0, &lgposBestWaypoint );

    Assert( CmpLgpos( lgposBestWaypoint, lgposCurrentWaypoint) >= 0 );


    if ( CmpLgpos( lgposBestWaypoint, lgposCurrentWaypoint ) > 0 )
    {


        Assert( lgposCurrentWaypoint.lGeneration < lgposBestWaypoint.lGeneration );

        BFIMaintCheckpointRequest();
        return;
    }

    return;
}



void BFIGetLgposOldestBegin0( const IFMP ifmp, LGPOS * const plgpos, const BOOL fExternalLocking, LGPOS lgposOldestTrx = lgposMin )
{
    FMP* pfmp = &g_rgfmp[ ifmp ];

    BFFMPContext* pbffmp = NULL;
    if ( fExternalLocking )
    {

        Assert( pfmp->FBFContextReader() );
        pbffmp = (BFFMPContext*)pfmp->DwBFContext();
        Assert( pbffmp );
        Assert( pbffmp->fCurrentlyAttached );
    }
    else
    {
        pfmp->EnterBFContextAsReader();
        pbffmp = (BFFMPContext*)pfmp->DwBFContext();
    }


    if ( !pbffmp || !pbffmp->fCurrentlyAttached )
    {
        *plgpos = lgposMax;
        if ( !fExternalLocking )
        {
            pfmp->LeaveBFContextAsReader();
        }
        return;
    }



    BFOB0::ERR      errOB0;
    BFOB0::CLock    lockOB0;

    pbffmp->bfob0.MoveBeforeFirst( &lockOB0 );
    errOB0 = pbffmp->bfob0.ErrMoveNext( &lockOB0 );


    if ( errOB0 != BFOB0::ERR::errNoCurrentEntry )
    {

        PBF pbf;
        errOB0 = pbffmp->bfob0.ErrRetrieveEntry( &lockOB0, &pbf );
        Assert( errOB0 == BFOB0::ERR::errSuccess );

        *plgpos = BFIOB0Lgpos( ifmp, pbf->lgposOldestBegin0 );


        if ( pbf->bfdf == bfdfClean )
        {
            BFIMaintCheckpointDepthRequest( &g_rgfmp[ pbf->ifmp ], bfcpdmrRequestRemoveCleanEntries );
        }
    }


    else
    {

        *plgpos = lgposMax;
    }


    pbffmp->bfob0.UnlockKeyPtr( &lockOB0 );


    pbffmp->critbfob0ol.Enter();

    for ( PBF pbf = pbffmp->bfob0ol.PrevMost(); pbf != pbfNil; pbf = pbffmp->bfob0ol.Next( pbf ) )
    {
        if ( CmpLgpos( plgpos, &pbf->lgposOldestBegin0 ) > 0 )
        {
            *plgpos = pbf->lgposOldestBegin0;
        }
        if ( pbf->bfdf == bfdfClean )
        {
            BFIMaintCheckpointDepthRequest( &g_rgfmp[ pbf->ifmp ], bfcpdmrRequestRemoveCleanEntries );
        }
    }

    pbffmp->lgposOldestBegin0Last = *plgpos;

    pbffmp->critbfob0ol.Leave();

    if ( !fExternalLocking )
    {
        pfmp->LeaveBFContextAsReader();
    }

#pragma prefast( suppress:6237, "When CONFIG OVERRIDE_INJECTION is off, UlConfigOverrideInjection() is a no-op." )
    if ( (BOOL)UlConfigOverrideInjection( 50764, fFalse ) && CmpLgpos( lgposOldestTrx, lgposMin ) != 0 )
    {

        LGPOS lgposOldestBegin0Scan;
        lgposOldestBegin0Scan = lgposMax;

        for ( IBF ibf = 0; ibf < cbfInit; ibf++ )
        {
            PBF pbf = PbfBFICacheIbf( ibf );



            if ( pbf->ifmp == ifmp )
            {

                if ( CmpLgpos( &lgposOldestBegin0Scan, &pbf->lgposOldestBegin0 ) > 0 )
                {
                    lgposOldestBegin0Scan = pbf->lgposOldestBegin0;
                }
            }
        }


        LGPOS lgposOldestBegin0Index = ( CmpLgpos( plgpos, &lgposOldestTrx ) < 0 ) ? *plgpos : lgposOldestTrx;


        Enforce( CmpLgpos( &lgposOldestBegin0Index, &lgposOldestBegin0Scan ) <= 0 );
    }
}


void BFGetLgposOldestBegin0( IFMP ifmp, LGPOS * const plgpos, LGPOS lgposOldestTrx )
{
    BFIGetLgposOldestBegin0( ifmp, plgpos, fFalse, lgposOldestTrx );
}


void BFSetLgposModify( const BFLatch* pbfl, LGPOS lgpos )
{

    Assert( FBFWARLatched( pbfl ) || FBFWriteLatched( pbfl ) );
    Assert( FBFUpdatableLatch( pbfl ) );

    Assert( FBFWriteLatched( pbfl ) );
    Assert( FBFNotWARLatched( pbfl ) );

    FMP* pfmp = &g_rgfmp[ PBF( pbfl->dwContext )->ifmp ];
    Assert( ( !pfmp->Pinst()->m_plog->FLogDisabled() && pfmp->FLogOn() ) ||
            !CmpLgpos( &lgpos, &lgposMin ) );


    BFISetLgposModify( PBF( pbfl->dwContext ), lgpos );
}


void BFSetLgposBegin0( const BFLatch* pbfl, LGPOS lgpos, const TraceContext& tc )
{

    Assert( FBFWARLatched( pbfl ) || FBFWriteLatched( pbfl ) );
    Assert( FBFUpdatableLatch( pbfl ) );
    Assert( tc.iorReason.Iorp( ) == iorpNone );

    if ( PBF( pbfl->dwContext )->bfdf >= bfdfDirty )
    {
        Assert( FBFWriteLatched( pbfl ) );
        Assert( FBFNotWARLatched( pbfl ) );
    }

    FMP* pfmp = &g_rgfmp[ PBF( pbfl->dwContext )->ifmp ];
    Assert( ( !pfmp->Pinst()->m_plog->FLogDisabled() && pfmp->FLogOn() ) ||
            !CmpLgpos( &lgpos, &lgposMax ) );


    BFISetLgposOldestBegin0( PBF( pbfl->dwContext ), lgpos, tc );
}





void BFPrereadPageRange( IFMP ifmp, const PGNO pgnoFirst, CPG cpg, CPG* pcpgActual, BYTE *rgfPageWasCached, const BFPreReadFlags bfprf, const BFPriority bfpri, const TraceContext& tc )
{
    LONG cbfPreread = 0;


    Expected( cpg >= 0 || rgfPageWasCached == NULL );
    Assert( tc.iorReason.Iorp( ) == iorpNone );

#ifdef DEBUG
    if ( cpg >= 0 )
    {
        Expected( pgnoFirst + cpg >= pgnoFirst );
    }
    else
    {
        Expected( pgnoFirst + cpg < pgnoFirst );
    }
#endif


    const LONG lDir = cpg > 0 ? 1 : -1;

    if ( lDir >= 0 )
    {
        AssertPREFIX( pgnoFirst - lDir < pgnoFirst );
    }
    else
    {
        AssertPREFIX( pgnoFirst - lDir > pgnoFirst );
    }
    
    if ( rgfPageWasCached )
    {
        for ( INT ipg = 0; ipg < cpg; ipg++ )
        {
            rgfPageWasCached[ ipg ] = fTrue;
        }
    }


#ifdef PREREAD_COMBINABLE_DEBUG
    LONG cbfCached = 0;
    BFPreReadFlags bfprfCombinablePass = bfprfCombinableOnly;
#else
    BFPreReadFlags bfprfCombinablePass = bfprfDefault;
#endif

    ULONG pgno;
    for ( pgno = pgnoFirst; pgno != pgnoFirst + cpg; pgno += lDir )
    {
        const ERR err = ErrBFIPrereadPage( ifmp, pgno, BFPreReadFlags( bfprfCombinablePass | bfprf ), bfpri, tc );

        if ( err == JET_errSuccess )
        {
            cbfPreread++;
            if ( rgfPageWasCached != NULL )
            {
                rgfPageWasCached[ abs( (INT)( pgno - pgnoFirst ) ) ] = fFalse;
            }

#ifdef PREREAD_COMBINABLE_DEBUG
            if ( bfprfCombinablePass == bfprfDefault )
            {
                bfprfCombinablePass = bfprfCombinableOnly;
            }
#endif
        }
#ifdef PREREAD_COMBINABLE_DEBUG
        else if ( err == errBFPageCached )
        {
            cbfCached++;
        }
        else if ( err == errDiskTilt )
        {
            AssertRTL( bfprfCombinablePass == bfprfCombinableOnly );

            pgno -= lDir;
            bfprfCombinablePass = bfprfDefault;
        }
#endif
        else if ( err != errBFPageCached )
        {
            Assert( err < 0 );
            break;
        }
    }

    
    if ( cbfPreread )
    {
        CallS( g_rgfmp[ ifmp ].Pfapi()->ErrIOIssue() );
        Ptls()->cbfAsyncReadIOs = 0;
    }


    if ( pcpgActual )
    {
        *pcpgActual = abs( (INT)( pgno - pgnoFirst ) );
    }


    if ( rgfPageWasCached != NULL )
    {
        for ( ; pgno != pgnoFirst + cpg; pgno += lDir )
        {
            rgfPageWasCached[ abs( (INT)( pgno - pgnoFirst ) ) ] = (BYTE)FBFInCache( ifmp, pgno );
        }
    }

    Assert( FBFApiClean() );
}


void BFPrereadPageList( IFMP ifmp, PGNO* prgpgno, CPG* pcpgActual, const BFPreReadFlags bfprf, const BFPriority bfpri, const TraceContext& tc )
{
    PGNO* prgpgnoSorted = NULL;

    Assert( tc.iorReason.Iorp( ) == iorpNone );

    BOOL fAlreadySorted = fTrue;
    size_t cpgno;
    for ( cpgno = 1; prgpgno[cpgno - 1] != pgnoNull; cpgno++ )
    {
        fAlreadySorted = fAlreadySorted && ( cpgno < 2 || prgpgno[cpgno - 2] < prgpgno[cpgno - 1] );
    }
    if ( !fAlreadySorted )
    {
        prgpgnoSorted = new PGNO[cpgno];
        if ( prgpgnoSorted )
        {
            memcpy( prgpgnoSorted, prgpgno, cpgno * sizeof( PGNO ) );
            Assert( prgpgnoSorted[cpgno - 1] == pgnoNull );
            std::sort( prgpgnoSorted, prgpgnoSorted + cpgno - 1, CmpPgno );
            prgpgno = prgpgnoSorted;
        }
    }
    for ( size_t ipgno = 0; prgpgno[ipgno] != pgnoNull; ipgno++ )
    {
        Assert( !prgpgnoSorted || ipgno < 1 || prgpgno[ipgno - 1] <= prgpgno[ipgno] );
    }

#ifdef PREREAD_COMBINABLE_DEBUG
    BFPreReadFlags bfprfCombinablePass = bfprfCombinableOnly;
#else
    BFPreReadFlags bfprfCombinablePass = bfprfDefault;
#endif


    LONG cbfPreread = 0;
    size_t ipgno;

    for ( ipgno = 0; prgpgno[ ipgno ] != pgnoNull; ipgno++ )
    {
        const ERR err = ErrBFIPrereadPage( ifmp, prgpgno[ ipgno ], BFPreReadFlags( bfprfCombinablePass | bfprf ), bfpri, tc );

        if ( err == JET_errSuccess )
        {
            cbfPreread++;
#ifdef PREREAD_COMBINABLE_DEBUG
            if ( bfprfCombinablePass == bfprfDefault )
            {
                bfprfCombinablePass = bfprfCombinableOnly;
            }
#endif
        }
#ifdef PREREAD_COMBINABLE_DEBUG
        else if ( err == errDiskTilt )
        {
            AssertRTL( bfprfCombinablePass == bfprfCombinableOnly );

            ipgno--;
            bfprfCombinablePass = bfprfDefault;
        }
#endif
        else if ( err != errBFPageCached )
        {
            Assert( err < 0 );
            break;
        }
    }
    
    
    if ( cbfPreread )
    {
        CallS( g_rgfmp[ ifmp ].Pfapi()->ErrIOIssue() );
        Ptls()->cbfAsyncReadIOs = 0;
    }


    if ( pcpgActual )
    {
        *pcpgActual = ipgno;
    }

    Assert( FBFApiClean() );

    delete[] prgpgnoSorted;
}




ERR ErrBFPrereadPage( const IFMP ifmp, const PGNO pgno, const BFPreReadFlags bfprf, const BFPriority bfpri, const TraceContext& tc )
{
    Assert( tc.iorReason.Iorp( ) == iorpNone );

    const ERR err = ErrBFIPrereadPage( ifmp, pgno, BFPreReadFlags( bfprf & ~bfprfNoIssue ), bfpri, tc );

    if ( err >= JET_errSuccess &&
            ( 0 == ( bfprf & bfprfNoIssue ) ) )
    {
        CallS( g_rgfmp[ ifmp ].Pfapi()->ErrIOIssue() );
        Ptls()->cbfAsyncReadIOs = 0;
    }

    Assert( Ptls()->cbfAsyncReadIOs == 0 || bfprf & bfprfNoIssue );

    return err;
}






CSmallLookasideCache::CSmallLookasideCache()
{
    m_cbBufferSize = 0;
    memset( m_rgpvLocalLookasideBuffers, 0, sizeof(m_rgpvLocalLookasideBuffers) );
}

CSmallLookasideCache::~CSmallLookasideCache()
{
}

void CSmallLookasideCache::Init( const INT cbBufferSize )
{
    m_cbBufferSize = cbBufferSize;

#ifdef MEMORY_STATS_TRACKING
    m_cHits = 0;
    m_cAllocs = 0;
    m_cFrees = 0;
    m_cFailures = 0;
#endif
}

void CSmallLookasideCache::Term()
{
    ULONG cSlotsFilled = 0;
    C_ASSERT( m_cLocalLookasideBuffers == _countof( m_rgpvLocalLookasideBuffers ) );
    for( INT ipb = 0; ipb < m_cLocalLookasideBuffers; ++ipb )
    {
        if ( m_rgpvLocalLookasideBuffers[ipb] )
        {
            OSMemoryPageFree( m_rgpvLocalLookasideBuffers[ipb] );
            m_rgpvLocalLookasideBuffers[ipb] = NULL;
            cSlotsFilled++;
        }
    }
#ifdef MEMORY_STATS_TRACKING
    
#endif
}

void * CSmallLookasideCache::PvAlloc()
{

    Assert( m_cbBufferSize );
    if ( 0 == m_cbBufferSize )
    {
        return NULL;
    }


    void * pb = GetCachedPtr<void *>( m_rgpvLocalLookasideBuffers, m_cLocalLookasideBuffers );


    if( NULL == pb )
    {
        pb = PvOSMemoryPageAlloc( m_cbBufferSize, NULL );
#ifdef MEMORY_STATS_TRACKING
        m_cAllocs++;
#endif
    }
#ifdef MEMORY_STATS_TRACKING
    else
    {
        m_cHits++;
    }

    if ( NULL == pb )
    {
        m_cFailures++;
    }
#endif
    return pb;
}

void CSmallLookasideCache::Free( void * const pv )
{
    if( pv )
    {
        if ( !FCachePtr<void *>( pv, m_rgpvLocalLookasideBuffers, m_cLocalLookasideBuffers ) )
        {
            OSMemoryPageFree( pv );
#ifdef MEMORY_STATS_TRACKING
            m_cFrees++;
#endif
        }
    }
}

void BFIAlloc( __in_range( bfasMin, bfasMax - 1 ) const BFAllocState bfas, void** ppv, ICBPage icbBufferSize )
{

    Assert( g_fBFInitialized );
    Assert( icbPageInvalid != icbBufferSize );


    if ( (DWORD)g_rgcbPageSize[g_icbCacheMax] >= OSMemoryPageCommitGranularity() &&
        (DWORD)g_rgcbPageSize[icbBufferSize] < OSMemoryPageCommitGranularity() )
    {
        Assert( icbBufferSize < IcbBFIBufferSize( OSMemoryPageCommitGranularity() ) );
        icbBufferSize = IcbBFIBufferSize( OSMemoryPageCommitGranularity() );
        Assert( icbPageInvalid != icbBufferSize );
    }


    *ppv = NULL;

    const LONG cRFSCountdownOld = RFSThreadDisable( 10 );
    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );
    for ( BOOL fWait = fFalse; !(*ppv); fWait = fTrue )
    {


        *ppv = g_pBFAllocLookasideList->PvAlloc();


        if ( !(*ppv) && fWait )
        {
            UtilSleep( dtickFastRetry );
        }
    }
    RFSThreadReEnable( cRFSCountdownOld );
    FOSSetCleanupState( fCleanUpStateSaved );
}


void BFAlloc( __in_range( bfasMin, bfasMax - 1 ) const BFAllocState bfas, void** ppv, INT cbBufferSize )
{

    if ( cbBufferSize )
    {
        BFIAlloc( bfas, ppv, IcbBFIBufferSize( cbBufferSize ) );
    }
    else
    {
        
        BFIAlloc( bfas, ppv, g_icbCacheMax );
    }
}


void BFFree( void* pv )
{

    Assert( g_fBFInitialized );


    if ( !pv )
    {

        return;
    }


    g_pBFAllocLookasideList->Free( pv );
}




void BFRenouncePage( BFLatch* const pbfl, const BOOL fRenounceDirty )
{

    Assert( FBFReadLatched( pbfl ) );


    if ( ErrBFUpgradeReadLatchToWriteLatch( pbfl, fFalse ) == JET_errSuccess )
    {

        BFIRenouncePage( PBF( pbfl->dwContext ), fRenounceDirty );


        BFWriteUnlatch( pbfl );
    }


    else
    {

        BFReadUnlatch( pbfl );
    }
}


void BFAbandonNewPage( BFLatch* const pbfl, const TraceContext& tc )
{
    Assert( FBFWriteLatched( pbfl ) );

    Expected( FBFDirty( pbfl ) == bfdfClean );

    const PBF pbf = PbfBFILatchContext( pbfl->dwContext );
    pbf->fAbandoned = fTrue;
    BFIPurgeNewPage( pbf, tc );

    pbfl->pv = NULL;
    pbfl->dwContext = NULL;
}


PBF PbfBFIGetFlushOrderLeafWithUndoInfoForExtZeroing( const PBF pbf )
{
    PBF pbfOldestWithUndoInfo = pbfNil;

    Assert( g_critBFDepend.FOwner() );

    for ( PBF pbfVersion = PbfBFIGetFlushOrderLeaf( pbf, fFalse ); pbfVersion != pbfNil; pbfVersion = pbfVersion->pbfTimeDepChainPrev )
    {
        if ( pbfVersion->prceUndoInfoNext != prceNil )
        {
            pbfOldestWithUndoInfo = pbfVersion;
            break;
        }
    }

    return pbfOldestWithUndoInfo;
}

ERR ErrBFPreparePageRangeForExternalZeroing( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg, const TraceContext& tc )
{
    BFLatch bfl = { NULL, 0 };
    PBF pbf = pbfNil;
    BOOL fLatched = fFalse;
    PBF pbfVersion = pbfNil;
    BOOL fVersionLatched = fFalse;
    BOOL fInCritDepend = fFalse;
    ERR err = JET_errSuccess;

    if ( !g_rgfmp[ ifmp ].FLogOn() )
    {
        goto HandleError;
    }

    const PGNO pgnoLast = pgnoFirst + cpg - 1;
    for ( PGNO pgno = pgnoFirst; pgno <= pgnoLast; pgno++ )
    {
        const ERR errLatch = ErrBFILatchPage(
                                &bfl,
                                ifmp,
                                pgno,
                                BFLatchFlags( bflfNoTouch | bflfNoUncached | bflfNoFaultFail | bflfUninitPageOk | bflfLatchAbandoned ),
                                bfltWrite,
                                BfpriBFMake( g_pctCachePriorityNeutral, (BFTEMPOSFILEQOS)0  ),
                                tc );

        if ( errLatch == errBFPageNotCached )
        {
            continue;
        }

        Call( errLatch );
        fLatched = fTrue;
        pbf = PbfBFILatchContext( bfl.dwContext );

        g_critBFDepend.Enter();
        fInCritDepend = fTrue;
        while ( ( pbfVersion = PbfBFIGetFlushOrderLeafWithUndoInfoForExtZeroing( pbf ) ) != pbfNil )
        {
            PBF pbfUndoInfo = pbfNil;

            if ( pbfVersion != pbf )
            {
                if ( pbfVersion->sxwl.ErrTryAcquireWriteLatch() == CSXWLatch::ERR::errSuccess )
                {
                    fVersionLatched = fTrue;
                    pbfUndoInfo = pbfVersion;
                }
                else
                {
                    g_critBFDepend.Leave();
                    fInCritDepend = fFalse;
                    UtilSleep( dtickFastRetry );
                    g_critBFDepend.Enter();
                    fInCritDepend = fTrue;
                    continue;
                }
            }
            else
            {
                pbfUndoInfo = pbf;
            }
            Assert( ( pbfVersion != pbf ) == !!fVersionLatched );
            Assert( pbfUndoInfo != pbfNil );
            Assert( ( pbfUndoInfo == pbf ) || ( pbfUndoInfo == pbfVersion ) );

            g_critBFDepend.Leave();
            fInCritDepend = fFalse;

            if ( pbfUndoInfo->prceUndoInfoNext != prceNil )
            {
                ENTERCRITICALSECTION ecs( &g_critpoolBFDUI.Crit( pbfUndoInfo ) );

                while ( pbfUndoInfo->prceUndoInfoNext != prceNil )
                {
                    Expected( pbfUndoInfo->err == JET_errSuccess );
                    if ( pbfUndoInfo->err < JET_errSuccess )
                    {
                        Error( pbfUndoInfo->err );
                    }

                    Assert( pbfUndoInfo->err == JET_errSuccess );
                    Assert( pbfUndoInfo->bfdf >= bfdfDirty );

                    LGPOS lgpos;
                    Call( ErrLGUndoInfo( pbfUndoInfo->prceUndoInfoNext, &lgpos ) );

                    BFIRemoveUndoInfo( pbfUndoInfo, pbfUndoInfo->prceUndoInfoNext, lgpos );
                }
            }

            if ( pbfVersion != pbf )
            {
                pbfVersion->sxwl.ReleaseWriteLatch();
                fVersionLatched = fFalse;
            }

            g_critBFDepend.Enter();
            fInCritDepend = fTrue;
        }
        g_critBFDepend.Leave();
        fInCritDepend = fFalse;

        BFWriteUnlatch( &bfl );
        fLatched = fFalse;
    }

HandleError:
    Assert( !( ( fVersionLatched || fInCritDepend ) && !fLatched ) );
    Assert( !( fVersionLatched && fLatched ) || ( pbf != pbfVersion ) );

    if ( fInCritDepend )
    {
        g_critBFDepend.Leave();
        fInCritDepend = fFalse;
    }

    if ( fVersionLatched )
    {
        Assert( pbfVersion != pbfNil );
        pbfVersion->sxwl.ReleaseWriteLatch();
        fVersionLatched = fFalse;
    }

    if ( fLatched )
    {
        Assert( pbf != pbfNil );
        Assert( PbfBFILatchContext( bfl.dwContext ) != pbf );
        BFWriteUnlatch( &bfl );
        fLatched = fFalse;
    }

    return err;
}

CPG CpgBFGetOptimalLockPageRangeSizeForExternalZeroing( const IFMP ifmp )
{
    ULONG_PTR cpgCacheSize = 0;
    CallS( ErrBFGetCacheSize( &cpgCacheSize ) );
    CPG cpgOptimalSize = (CPG)UlpFunctionalMin( UlParam( PinstFromIfmp( ifmp ), JET_paramDbExtensionSize ), cpgCacheSize / 10 );
    cpgOptimalSize = LFunctionalMax( cpgOptimalSize, g_rgfmp[ifmp].CpgOfCb( UlParam( JET_paramMaxCoalesceWriteSize ) ) );
    cpgOptimalSize = LFunctionalMax( cpgOptimalSize, 1 );

    return cpgOptimalSize;
}

ERR ErrBFLockPageRangeForExternalZeroing( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg, const BOOL fTrimming, const TraceContext& tc, _Out_ DWORD_PTR* const pdwContext )
{
    ERR err = JET_errSuccess;
    BFIPageRangeLock* pbfprl = NULL;
    FMP* const pfmp = &g_rgfmp[ ifmp ];

    Assert( pdwContext != NULL );
    Assert( pgnoFirst >= PgnoOfOffset( cpgDBReserved * g_rgcbPageSize[ g_icbCacheMax ] ) );
    Assert( cpg > 0 );

    if ( pfmp->FLogOn() )
    {
        const BOOL fWaypointLatency = UlParam( pfmp->Pinst(), JET_paramWaypointLatency ) > 0;
        if ( fTrimming && fWaypointLatency )
        {
            Expected( fFalse );
            Error( ErrERRCheck( JET_errFeatureNotAvailable ) );
        }

        Call( ErrBFIWriteLog( ifmp, fTrue ) );
        Call( ErrBFIFlushLog( ifmp, iofrLogFlushAll, fTrue ) );

        if ( g_rgfmp[ ifmp ].FRBSOn() )
        {
            Call( pfmp->PRBS()->ErrFlushAll() );
        }

        if ( !fTrimming )
        {
            Call( pfmp->FShrinkIsRunning() ? ErrFaultInjection( 50954 ) : JET_errSuccess );
            Call( pfmp->Pinst()->m_plog->ErrLGQuiesceWaypointLatencyIFMP( ifmp ) );
            Call( pfmp->FShrinkIsRunning() ? ErrFaultInjection( 47882 ) : JET_errSuccess );
        }
    }

    Alloc( pbfprl = new BFIPageRangeLock );
    pbfprl->ifmp = ifmp;
    pbfprl->pgnoFirst = pgnoFirst;
    pbfprl->pgnoLast = pgnoFirst + cpg - 1;
    pbfprl->pgnoDbLast = pfmp->PgnoLast();
    pbfprl->cpg = cpg;
    Alloc( pbfprl->rgbfl = new BFLatch[ cpg ] );
    memset( pbfprl->rgbfl, 0, cpg * sizeof(BFLatch) );
    Alloc( pbfprl->rgfLatched = new BOOL[ cpg ] );
    memset( pbfprl->rgfLatched, 0, cpg * sizeof(BOOL) );
    Alloc( pbfprl->rgfUncached = new BOOL[ cpg ] );
    memset( pbfprl->rgfUncached, 0, cpg * sizeof(BOOL) );

    Expected( ( pbfprl->pgnoLast <= pbfprl->pgnoDbLast ) || ( pbfprl->pgnoFirst > pbfprl->pgnoDbLast ) );

    for ( PGNO pgno = pbfprl->pgnoFirst; pgno <= pbfprl->pgnoLast; pgno++ )
    {
        const size_t ipgno = pgno - pbfprl->pgnoFirst;
        BOOL fCachedNewPage = fFalse;
        const BOOL fWithinOwnedDbSize = ( pgno <= pbfprl->pgnoDbLast );
        const BFLatchFlags bflfUncachedBehavior = fWithinOwnedDbSize ? bflfNewIfUncached : bflfNoUncached;
        Expected( !!fWithinOwnedDbSize == !!fTrimming );

        err = ErrBFILatchPage(
                &pbfprl->rgbfl[ ipgno ],
                pbfprl->ifmp,
                pgno,
                BFLatchFlags( bflfUncachedBehavior | bflfNoTouch | bflfNoFaultFail | bflfUninitPageOk | bflfLatchAbandoned ),
                bfltWrite,
                BfpriBFMake( g_pctCachePriorityNeutral, (BFTEMPOSFILEQOS)0  ),
                tc,
                &fCachedNewPage );

        if ( err == errBFPageNotCached )
        {
            Assert( bflfUncachedBehavior == bflfNoUncached );
            pbfprl->rgfLatched[ ipgno ] = fFalse;
            pbfprl->rgfUncached[ ipgno ] = fTrue;
            err = JET_errSuccess;
        }
        else
        {
            Call( err );

            Assert( !fCachedNewPage || ( bflfUncachedBehavior == bflfNewIfUncached ) );
            pbfprl->rgfLatched[ ipgno ] = fTrue;
            pbfprl->rgfUncached[ ipgno ] = fCachedNewPage;
            err = JET_errSuccess;

            BF* const pbf = PBF( pbfprl->rgbfl[ ipgno ].dwContext );
            Assert( pbf->sxwl.FOwnWriteLatch() );

            pbf->fAbandoned = fTrue;

            if ( pbfprl->rgfUncached[ ipgno ] )
            {
                Assert( pbf->bfdf == bfdfClean );
                memset( pbf->pv, 0, g_rgcbPageSize[ pbf->icbBuffer ] );
                pbf->err = SHORT( ErrERRCheck( JET_errPageNotInitialized ) );
            }
        }
    }

    OnDebug( const PGNO pgnoDbLastPostLatches = pfmp->PgnoLast() );
    Expected( pgnoDbLastPostLatches >= pbfprl->pgnoDbLast );

    Assert( ( pgnoDbLastPostLatches <= pbfprl->pgnoDbLast ) || ( pbfprl->pgnoLast <= pbfprl->pgnoDbLast ) );

    while ( !pbfprl->fRangeLocked )
    {
        err = pfmp->ErrRangeLockAndEnter( pbfprl->pgnoFirst, pbfprl->pgnoLast, &pbfprl->irangelock );

        if ( err == JET_errTooManyActiveUsers )
        {
            UtilSleep( dtickFastRetry );
        }
        else
        {
            Call( err );
            pbfprl->fRangeLocked = fTrue;
        }
    }

    Call( pfmp->PFlushMap()->ErrSyncRangeInvalidateFlushType( pbfprl->pgnoFirst, pbfprl->cpg ) );

    OnDebug( const PGNO pgnoDbLastPostRangeLock = pfmp->PgnoLast() );
    Expected( pgnoDbLastPostRangeLock >= pbfprl->pgnoDbLast );
    Assert( ( pgnoDbLastPostRangeLock <= pbfprl->pgnoDbLast ) || ( pbfprl->pgnoLast <= pbfprl->pgnoDbLast ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        BFUnlockPageRangeForExternalZeroing( (DWORD_PTR)pbfprl, tc );
        *pdwContext = (DWORD_PTR)NULL;
    }
    else
    {
        *pdwContext = (DWORD_PTR)pbfprl;
    }

    return err;
}

void BFPurgeLockedPageRangeForExternalZeroing( const DWORD_PTR dwContext, const TraceContext& tc )
{
    BFIPageRangeLock* const pbfprl = (BFIPageRangeLock*)dwContext;
    Assert( ( pbfprl != NULL ) && ( pbfprl->rgbfl != NULL ) && ( pbfprl->rgfLatched != NULL ) && ( pbfprl->rgfUncached != NULL ) );
    Assert( pbfprl->fRangeLocked );

    OnDebug( const PGNO pgnoDbLastPrePurges = g_rgfmp[ pbfprl->ifmp ].PgnoLast() );
    Expected( pgnoDbLastPrePurges >= pbfprl->pgnoDbLast );

    Assert( ( pgnoDbLastPrePurges <= pbfprl->pgnoDbLast ) || ( pbfprl->pgnoLast <= pbfprl->pgnoDbLast ) );

    for ( PGNO pgno = pbfprl->pgnoFirst; pgno <= pbfprl->pgnoLast; pgno++ )
    {
        const size_t ipgno = pgno - pbfprl->pgnoFirst;

        Assert( !!FBFInCache( pbfprl->ifmp, pgno ) == !!pbfprl->rgfLatched[ ipgno ] );

        if ( !pbfprl->rgfLatched[ ipgno ] )
        {
            Assert( pgno > g_rgfmp[ pbfprl->ifmp ].PgnoLast() );
            continue;
        }

        BF* const pbf = PBF( pbfprl->rgbfl[ ipgno ].dwContext );
        const IFMP ifmp = pbf->ifmp;

        Assert( pbf->sxwl.FOwnWriteLatch() );

        if ( pbf->bfdf == bfdfClean )
        {
            memset( pbf->pv, 0, g_rgcbPageSize[ pbf->icbBuffer ] );
            pbf->err = SHORT( ErrERRCheck( JET_errPageNotInitialized ) );
        }

        BFIPurgeAllPageVersions( &pbfprl->rgbfl[ ipgno ], tc );
        pbfprl->rgfLatched[ ipgno ] = fFalse;
    }

    OnDebug( const PGNO pgnoDbLastPostPurges = g_rgfmp[ pbfprl->ifmp ].PgnoLast() );
    Expected( pgnoDbLastPostPurges >= pbfprl->pgnoDbLast );
    Assert( ( pgnoDbLastPostPurges <= pbfprl->pgnoDbLast ) || ( pbfprl->pgnoLast <= pbfprl->pgnoDbLast ) );
}

void BFUnlockPageRangeForExternalZeroing( const DWORD_PTR dwContext, const TraceContext& tc )
{
    BFIPageRangeLock* const pbfprl = (BFIPageRangeLock*)dwContext;

    if ( ( pbfprl == NULL ) || ( pbfprl->rgbfl == NULL ) || ( pbfprl->rgfLatched == NULL ) || ( pbfprl->rgfUncached == NULL ) )
    {
        goto HandleError;
    }

    FMP* const pfmp = &g_rgfmp[ pbfprl->ifmp ];

    if ( pbfprl->fRangeLocked )
    {
        pfmp->RangeUnlockAndLeave( pbfprl->pgnoFirst, pbfprl->pgnoLast, pbfprl->irangelock );
        pbfprl->fRangeLocked = fFalse;
    }

    for ( PGNO pgno = pbfprl->pgnoFirst; pgno <= pbfprl->pgnoLast; pgno++ )
    {
        const size_t ipgno = pgno - pbfprl->pgnoFirst;

        if ( !pbfprl->rgfLatched[ ipgno ] )
        {
            continue;
        }

        BF* const pbf = PBF( pbfprl->rgbfl[ ipgno ].dwContext );
        Assert( pbf->sxwl.FOwnWriteLatch() );

        if ( pbfprl->rgfUncached[ ipgno ] )
        {
            BFIPurgeAllPageVersions( &pbfprl->rgbfl[ ipgno ], tc );
        }
        else
        {
            BFWriteUnlatch( &pbfprl->rgbfl[ ipgno ] );
        }

        pbfprl->rgfLatched[ ipgno ] = fFalse;
    }

HandleError:
    delete pbfprl;
}

void BFPurge( IFMP ifmp, PGNO pgno, CPG cpg )
{
    FMP*    pfmp        = &g_rgfmp[ ifmp ];
    PGNO    pgnoFirst   = ( pgnoNull == pgno ) ? PgnoOfOffset( cpgDBReserved * g_rgcbPageSize[g_icbCacheMax] ) : pgno;
    PGNO    pgnoLast    = ( pgnoNull == pgno ) ? PgnoOfOffset( 0 ) : ( pgno + cpg - 1 );
    BOOL    fUnlock     = fFalse;
    
    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlBfPurge|sysosrtlContextFmp, pfmp, &ifmp, sizeof(ifmp) );


    if ( pfmp->ErrRangeLock( pgnoFirst, pgnoLast ) >= JET_errSuccess )
    {
        fUnlock = fTrue;
    }

    const TICK tickStartPurge = TickOSTimeCurrent();
    


    Expected( DtickDelta( tickStartPurge, TickOSTimeCurrent() ) <= 5 * 60 * 1000 );


    for ( IBF ibf = 0; ibf < cbfInit; ibf++ )
    {
        const PBF pbf = PbfBFICacheIbf( ibf );
        volatile BF* const pbfT = pbf;
        const IFMP ifmpT = pbfT->ifmp;
        const PGNO pgnoT = pbfT->pgno;


        if ( ifmpT != ifmp || pgnoT < pgnoFirst || pgnoT > pgnoLast )
        {
            continue;
        }

        BFIPurgePage( pbf, ifmpT, pgnoT, bfltMax, BFEvictFlags( bfefReasonPurgeContext | bfefEvictDirty | bfefAllowTearDownClean ) );
    }


    if ( pgnoNull == pgno )
    {

        pfmp->EnterBFContextAsWriter();
        BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext();
        if ( pbffmp )
        {

            BOOL fEmpty = fTrue;

            BFOB0::CLock    lockOB0;
            pbffmp->bfob0.MoveBeforeFirst( &lockOB0 );
            fEmpty = fEmpty && pbffmp->bfob0.ErrMoveNext( &lockOB0 ) == BFOB0::ERR::errNoCurrentEntry;
            pbffmp->bfob0.UnlockKeyPtr( &lockOB0 );

            pbffmp->critbfob0ol.Enter();
            fEmpty = fEmpty && pbffmp->bfob0ol.FEmpty();
            pbffmp->critbfob0ol.Leave();


            Enforce( fEmpty );


            pbffmp->bfob0.Term();
            pbffmp->BFFMPContext::~BFFMPContext();
            OSMemoryHeapFreeAlign( pbffmp );
            pfmp->SetDwBFContext( NULL );
        }
        pfmp->LeaveBFContextAsWriter();
    }


    if ( fUnlock )
    {
        pfmp->RangeUnlock( pgnoFirst, pgnoLast );
    }


    (void)CBFIssueList::ErrSync();

    

    pfmp->WaitForAsyncIOForViewCache();
    
}

BOOL CmpPgno( __in const PGNO& pgno1, __in const PGNO& pgno2 )
{
    return pgno1 < pgno2;
}

LOCAL BOOL CmpPgnoPbf( const PGNOPBF& pgnopbf1, const PGNOPBF& pgnopbf2 )
{
    return CmpPgno( pgnopbf1.pgno, pgnopbf2.pgno );
}

CAutoResetSignal g_asigBFFlush( CSyncBasicInfo( "g_asigBFFlush" ) );

void BFIPreFlushOrdered( __in const IFMP ifmp, __in const PGNO pgnoFirst, __in const PGNO pgnoLast )
{
    LOG * plog = PinstFromIfmp( ifmp )->m_plog;

    ULONG cFlushCycles = 0;
    DWORD_PTR   cbfFalseHits = 0;
    DWORD_PTR   cbfRemainingDependencies = 0;
    DWORD_PTR   cbfPageTouchTooRecent = 0;
    DWORD_PTR   cbfOutOfMemory = 0;
    DWORD_PTR   cbfLatchConflict = 0;
    DWORD_PTR   cbfAbandoned = 0;
    DWORD_PTR   cbfOtherErr = 0;


    PGNOPBF *       rgpgnopbf = NULL;
    LONG_PTR        cbfOrderedFlushMax = cbfInit;
    LONG_PTR        cbfOrderedFlushMac = 0;
    while( NULL == ( rgpgnopbf = new PGNOPBF[ cbfOrderedFlushMax ] ) )
    {
        cbfOrderedFlushMax = cbfOrderedFlushMax / 2;
        if ( cbfOrderedFlushMax < 10 )
        {
            return;
        }
    }



    if ( NULL != plog && !plog->FLogDisabled() )
    {
        (void)plog->ErrLGUpdateWaypointIFMP( PinstFromIfmp( ifmp )->m_pfsapi, ifmp );
    }

    if ( g_rgfmp[ ifmp ].FRBSOn() )
    {
        (void)g_rgfmp[ ifmp ].PRBS()->ErrFlushAll();
    }

    (void)ErrBFIWriteLog( ifmp, fTrue );


    CBFIssueList    bfil;

    for ( IBF ibf = 0; ibf < cbfInit; ibf++ )
    {

        PBF pbf = PbfBFICacheIbf( ibf );


        if ( pbf->ifmp != ifmp )
        {
            continue;
        }


        if ( ( pgnoFirst != pgnoNull ) && ( pbf->pgno < pgnoFirst ) )
        {
            continue;
        }
        if ( ( pgnoLast != pgnoNull ) && ( pbf->pgno > pgnoLast ) )
        {
            continue;
        }



        if ( cbfOrderedFlushMac >= cbfOrderedFlushMax )
        {
            break;
        }

        rgpgnopbf[cbfOrderedFlushMac].pgno = pbf->pgno;
        rgpgnopbf[cbfOrderedFlushMac].pbf = pbf;
        cbfOrderedFlushMac++;

    }


    std::sort( rgpgnopbf, rgpgnopbf + cbfOrderedFlushMac, CmpPgnoPbf );


    INT ipgnopbf = 0;
    while( ipgnopbf < cbfOrderedFlushMac )
    {


        for ( ; ipgnopbf < cbfOrderedFlushMac; ipgnopbf++ )
        {


            if ( rgpgnopbf[ipgnopbf].pbf->ifmp != ifmp )
            {
                cbfFalseHits++;
                continue;
            }
            if ( ( pgnoFirst != pgnoNull ) && ( rgpgnopbf[ipgnopbf].pbf->pgno < pgnoFirst ) )
            {
                cbfFalseHits++;
                continue;
            }
            if ( ( pgnoLast != pgnoNull ) && ( rgpgnopbf[ipgnopbf].pbf->pgno > pgnoLast ) )
            {
                cbfFalseHits++;
                continue;
            }


            const ERR errFlush = ErrBFIFlushPage( rgpgnopbf[ipgnopbf].pbf, IOR( iorpBFDatabaseFlush ), QosOSFileFromUrgentLevel( qosIODispatchUrgentBackgroundLevelMax / 2 ) );

            OSTrace( JET_tracetagBufferManager, OSFormat( "\t[%d]OrderedFlush( %d:%d ) -> %d\n", ipgnopbf, (ULONG)rgpgnopbf[ipgnopbf].pbf->ifmp, (UINT)rgpgnopbf[ipgnopbf].pbf->pgno, errFlush ) );

            if ( JET_errSuccess == errFlush ||
                errBFIPageFlushed == errFlush ||
                errBFIPageFlushPending == errFlush ||
                errBFIPageFlushPendingSlowIO == errFlush ||
                errBFIPageFlushPendingHungIO == errFlush )
            {
            }
            else if ( errDiskTilt == errFlush )
            {
                break;
            }
            else if ( errBFIRemainingDependencies == errFlush ) cbfRemainingDependencies++;
            else if ( errBFIPageTouchTooRecent == errFlush )    cbfPageTouchTooRecent++;
            else if ( JET_errOutOfMemory == errFlush )      cbfOutOfMemory++;
            else if ( errBFLatchConflict == errFlush )      cbfLatchConflict++;
            else if ( errBFIPageAbandoned == errFlush )     cbfAbandoned++;
            else                            cbfOtherErr++;

            Assert( errBFIPageFlushDisallowedOnIOThread != errFlush );
            Assert( wrnBFPageFlushPending != errFlush );



        }


        CallS( bfil.ErrIssue( fTrue ) );
        (VOID)g_asigBFFlush.FWait( dtickFastRetry );
        cFlushCycles++;
        OSTrace( JET_tracetagBufferManager, OSFormat( "   OrderlyFlush( %d ) taking a breather!\n", (ULONG)ifmp ) );

    }


    OSTrace( JET_tracetagBufferManager, OSFormat( "BF: Orderly flush complete for IFMP = %I32u: cbf = %I32u, cFlushCycles = %I32u, cbfFalseHits = %I32u, errors { %I32u, %I32u, %I32u, %I32u, %I32u, %I32u }",
            (ULONG)ifmp, (ULONG)cbfOrderedFlushMac, (ULONG)cFlushCycles, (ULONG)cbfFalseHits,
            (ULONG)cbfRemainingDependencies, (ULONG)cbfPageTouchTooRecent, (ULONG)cbfOutOfMemory, (ULONG)cbfLatchConflict, (ULONG)cbfAbandoned, (ULONG)cbfOtherErr ) );


    delete[] rgpgnopbf;
}


ERR ErrBFFlush( IFMP ifmp, const OBJID objidFDP, const PGNO pgnoFirst, const PGNO pgnoLast )
{

    ERR         err                     = JET_errSuccess;
    BOOL        fRetryFlush             = fFalse;
    LONG        cRetryFlush             = 0;

    Expected( ( pgnoFirst == pgnoNull ) == ( pgnoLast == pgnoNull ) );
    Expected( ( objidFDP == objidNil ) || ( ( pgnoFirst == pgnoNull ) && ( pgnoLast == pgnoNull ) ) );


    DWORD_PTR   cTooManyOutstandingIOs  = 0;
    DWORD_PTR   cRemainingDependencies  = 0;
    DWORD_PTR   cPagesBeingFlushed      = 0;
    DWORD_PTR   cPageFlushesPending     = 0;
    DWORD_PTR   cLatchConflicts         = 0;
    DWORD_PTR   cPageTouchesTooRecent   = 0;
    DWORD_PTR   cPageFlushesDisallowed  = 0;
    DWORD_PTR   cPagesAbandoned         = 0;

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlBfFlushBegin|sysosrtlContextFmp, &g_rgfmp[ifmp], &ifmp, sizeof(ifmp) );

    OSTraceFMP(
        ifmp,
        JET_tracetagBufferManager,
        OSFormat(   "cBFPagesFlushedContextFlush before force-flush: %d [ifmp=0x%x]",
                    PERFZeroDisabledAndDiscouraged( cBFPagesFlushedContextFlush.GetInstance( PinstFromIfmp( ifmp ) ) ),
                    ifmp ) );


    if ( objidNil == objidFDP && ( g_rgfmp[ ifmp ].Pfapi() == NULL || g_rgfmp[ ifmp ].FSeekPenalty() ) )
    {
        BFIPreFlushOrdered( ifmp, pgnoFirst, pgnoLast );
    }

    do  {
        CBFIssueList    bfil;
        DWORD_PTR       cPageTouchesTooRecentThisRetry  = 0;

        fRetryFlush = fFalse;


        for ( IBF ibf = 0; ibf < cbfInit; ibf++ )
        {
            PBF pbf = PbfBFICacheIbf( ibf );


            if ( pbf->ifmp != ifmp )
            {
                continue;
            }


            if ( ( pgnoFirst != pgnoNull ) && ( pbf->pgno < pgnoFirst ) )
            {
                continue;
            }
            if ( ( pgnoLast != pgnoNull ) && ( pbf->pgno > pgnoLast ) )
            {
                continue;
            }


            if ( objidNil != objidFDP )
            {
                CSXWLatch::ERR  errSXWL = pbf->sxwl.ErrTryAcquireSharedLatch();

                if ( errSXWL == CSXWLatch::ERR::errSuccess )
                {
                    const BOOL  fSkip   = ( objidFDP != ( (CPAGE::PGHDR *)( pbf->pv ) )->objidFDP );

                    pbf->sxwl.ReleaseSharedLatch();

                    if ( fSkip )
                    {
                        continue;
                    }
                }
                else
                {
                }
            }


            const ERR errFlush = ErrBFIFlushPage( pbf, IOR( iorpBFDatabaseFlush ), QosOSFileFromUrgentLevel( qosIODispatchUrgentBackgroundLevelMax / 2 ) );

            OSTrace( JET_tracetagBufferManager, OSFormat( "\t[%d]CleanupFlush( %d:%d ) -> %d\n", (ULONG)ibf, (ULONG)pbf->ifmp, pbf->pgno, errFlush ) );


            if ( errFlush < JET_errSuccess )
            {

                if ( errFlush == errBFIRemainingDependencies )
                {

                    cRemainingDependencies++;
                    fRetryFlush = fTrue;
                }


                else if ( errFlush == errBFIPageFlushed )
                {

                    cPagesBeingFlushed++;
                    fRetryFlush = fTrue;
                }


                else if ( ( errFlush == errBFIPageFlushPending ) || ( errFlush == errBFIPageFlushPendingSlowIO ) || ( errFlush == errBFIPageFlushPendingHungIO ) )
                {

                    cPageFlushesPending++;
                    fRetryFlush = fTrue;
                }


                else if ( errFlush == errDiskTilt )
                {

                    cTooManyOutstandingIOs++;
                    fRetryFlush = fTrue;
                    cRetryFlush = 0;
                    break;
                }


                else if ( errFlush == errBFLatchConflict )
                {

                    cLatchConflicts++;
                    fRetryFlush = fTrue;
                }

                else if ( errFlush == errBFIPageTouchTooRecent )
                {

                    cPageTouchesTooRecent++;
                    cPageTouchesTooRecentThisRetry++;
                    fRetryFlush = fTrue;
                }

                else if ( errFlush == errBFIPageFlushDisallowedOnIOThread )
                {

                    AssertSz( fFalse, "Shouldn't see errBFIPageFlushDisallowedOnIOThread in " __FUNCTION__ );

                    cPageFlushesDisallowed++;
                    fRetryFlush = fTrue;
                }

                else if ( errFlush == errBFIPageAbandoned )
                {

                    cPagesAbandoned++;
                    fRetryFlush = fTrue;
                }


                else
                {

                    err = err < JET_errSuccess ? err : errFlush;
                }
            }
        }


        if ( fRetryFlush )
        {
            LOG * plog = PinstFromIfmp( ifmp )->m_plog;
            
            OSTraceFMP(
                ifmp,
                JET_tracetagBufferManager,
                OSFormat(   "cBFPagesFlushedContextFlush before retry force-flush: %d [ifmp=0x%x]\r\n"
                            "    cRetryFlush                    = %d\r\n"
                            "    cTooManyOutstandingIOs         = %Iu\r\n"
                            "    cRemainingDependencies         = %Iu\r\n"
                            "    cPagesBeingFlushed             = %Iu\r\n"
                            "    cPageFlushesPending            = %Iu\r\n"
                            "    cLatchConflicts                = %Iu\r\n"
                            "    cPageFlushesDisallowed         = %Iu\r\n"
                            "    cPageTouchesTooRecent          = %Iu\r\n"
                            "    cPageTouchesTooRecentThisRetry = %Iu\r\n"
                            "    cPagesAbandoned                = %Iu",
                            PERFZeroDisabledAndDiscouraged( cBFPagesFlushedContextFlush.GetInstance( PinstFromIfmp( ifmp ) ) ),
                            ifmp,
                            cRetryFlush,
                            cTooManyOutstandingIOs,
                            cRemainingDependencies,
                            cPagesBeingFlushed,
                            cPageFlushesPending,
                            cLatchConflicts,
                            cPageFlushesDisallowed,
                            cPageTouchesTooRecent,
                            cPageTouchesTooRecentThisRetry,
                            cPagesAbandoned ) );


            CallS( bfil.ErrIssue( fTrue ) );


            if ( 0 != cPageTouchesTooRecentThisRetry )
            {
                if ( NULL != plog && !plog->FLogDisabled() )
                {
                    (void)plog->ErrLGUpdateWaypointIFMP( PinstFromIfmp( ifmp )->m_pfsapi, ifmp );
                }

                if ( g_rgfmp[ ifmp ].FRBSOn() )
                {
                    (void)g_rgfmp[ ifmp ].PRBS()->ErrFlushAll();
                }
            }


            (VOID)g_asigBFFlush.FWait( dtickFastRetry );
            cRetryFlush++;
        }

        Assert( bfil.FEmpty() );
    }
    while ( fRetryFlush );

    const ERR errBfFlushLoop = err;
    
    OSTraceFMP(
        ifmp,
        JET_tracetagBufferManager,
        OSFormat(   "cBFPagesFlushedContextFlush after force-flush: %d [ifmp=0x%x]\r\n"
                    "    cRetryFlush             = %d\r\n"
                    "    cTooManyOutstandingIOs  = %Iu\r\n"
                    "    cRemainingDependencies  = %Iu\r\n"
                    "    cPagesBeingFlushed      = %Iu\r\n"
                    "    cPageFlushesPending     = %Iu\r\n"
                    "    cLatchConflicts         = %Iu\r\n"
                    "    cPageFlushesDisallowed  = %Iu\r\n"
                    "    cPageTouchesTooRecent   = %Iu\r\n"
                    "    cPagesAbandoned         = %Iu",
                    PERFZeroDisabledAndDiscouraged( cBFPagesFlushedContextFlush.GetInstance( PinstFromIfmp( ifmp ) ) ),
                    ifmp,
                    cRetryFlush,
                    cTooManyOutstandingIOs,
                    cRemainingDependencies,
                    cPagesBeingFlushed,
                    cPageFlushesPending,
                    cLatchConflicts,
                    cPageFlushesDisallowed,
                    cPageTouchesTooRecent,
                    cPagesAbandoned ) );


    FMP* pfmp = &g_rgfmp[ ifmp ];
    pfmp->EnterBFContextAsWriter();
    BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext();
    if ( pbffmp )
    {
        Assert( pbffmp->fCurrentlyAttached );


        BFOB0::ERR      errOB0;
        BFOB0::CLock    lockOB0;

        pbffmp->bfob0.MoveBeforeFirst( &lockOB0 );
        while ( pbffmp->bfob0.ErrMoveNext( &lockOB0 ) != BFOB0::ERR::errNoCurrentEntry )
        {
            PBF pbf;
            errOB0 = pbffmp->bfob0.ErrRetrieveEntry( &lockOB0, &pbf );
            Assert( errOB0 == BFOB0::ERR::errSuccess );

            AssertTrack( !pbf->fAvailable && !pbf->fQuiesced, "EvictedBufferInOB0" );
            AssertTrack( pbf->icbBuffer != icbPage0, "FullyDehydratedBufferInOB0" );


            if ( ( ( objidNil != objidFDP ) &&
                   ( objidFDP != ( (CPAGE::PGHDR *)( pbf->pv ) )->objidFDP ) ) ||
                 ( ( pgnoFirst != pgnoNull ) && ( pbf->pgno < pgnoFirst ) ) ||
                 ( ( pgnoLast != pgnoNull ) && ( pbf->pgno > pgnoLast ) ) )
            {
                continue;
            }

            Enforce( err < JET_errSuccess || pbf->bfdf == bfdfClean );
        }
        pbffmp->bfob0.UnlockKeyPtr( &lockOB0 );

        pbffmp->critbfob0ol.Enter();
        PBF pbfNext;
        for ( PBF pbf = pbffmp->bfob0ol.PrevMost(); pbf != pbfNil; pbf = pbfNext )
        {
            pbfNext = pbffmp->bfob0ol.Next( pbf );


            if ( ( ( objidNil != objidFDP ) &&
                   ( objidFDP != ( (CPAGE::PGHDR *)( pbf->pv ) )->objidFDP ) ) ||
                 ( ( pgnoFirst != pgnoNull ) && ( pbf->pgno < pgnoFirst ) ) ||
                 ( ( pgnoLast != pgnoNull ) && ( pbf->pgno > pgnoLast ) ) )
            {
                continue;
            }

            Enforce( err < JET_errSuccess || pbf->bfdf == bfdfClean );
        }
        pbffmp->critbfob0ol.Leave();

    }
    pfmp->LeaveBFContextAsWriter();

    if ( ( JET_errSuccess <= err ) &&
         ( objidNil == objidFDP ) &&
         ( pgnoNull == pgnoFirst ) && ( pgnoNull == pgnoLast ) )
    {
        BOOL fRetry = fFalse;
        do  {
            fRetry  = fFalse;

            for ( IBF ibf = 0; ibf < cbfInit; ibf++ )
            {
                PBF pbf = PbfBFICacheIbf( ibf );
                if ( pbf->ifmp != ifmp )
                {
                    continue;
                }

                CSXWLatch::ERR errSXWL = pbf->sxwl.ErrTryAcquireExclusiveLatch();
                if ( errSXWL == CSXWLatch::ERR::errSuccess )
                {
                    if ( pbf->ifmp == ifmp )
                    {
                        BFIResetLgposOldestBegin0( pbf );
                    }

                    pbf->sxwl.ReleaseExclusiveLatch();
                }
                else
                {
                    Assert( errSXWL == CSXWLatch::ERR::errLatchConflict );
                    fRetry = fTrue;
                }
            }

            if ( fRetry )
            {
                UtilSleep( dtickFastRetry );
            }
        }
        while ( fRetry );
    }


    (void)CBFIssueList::ErrSync();

    if ( err >= JET_errSuccess )
    {
        OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlBfFlushSucceed|sysosrtlContextFmp, pfmp, &ifmp, sizeof(ifmp) );
    }
    else
    {
        ULONG rgul[4] = { (ULONG)ifmp, (ULONG)err, PefLastThrow()->UlLine(), UlLineLastCall() };
        OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlBfFlushFail|sysosrtlContextFmp, pfmp, rgul, sizeof(rgul) );
    }

    Assert( g_rgfmp[ifmp].Pfapi() ||
                ( PinstFromIfmp( ifmp )->FRecovering() &&
                    ( !g_rgfmp[ifmp].FAttached() ||
                        g_rgfmp[ifmp].FDeferredAttach() ||
                        g_rgfmp[ifmp].FSkippedAttach() ) ) ||
                ( _wcsicmp ( SzParam( PinstFromIfmp( ifmp ), JET_paramRecovery ), wszOn ) != 0 ) ||
                ( !PinstFromIfmp( ifmp )->m_fJetInitialized ) );
    if ( err >= JET_errSuccess && g_rgfmp[ifmp].Pfapi() )
    {
        err = ErrIOFlushDatabaseFileBuffers( ifmp, iofrFlushIfmpContext );
    }


    AssertTrack( errBfFlushLoop >= JET_errSuccess || err == errBfFlushLoop, "ErrBFFlushLostFlushLoopErr" );

    return err;
}


ERR ErrBFFlushSync( IFMP ifmp )
{
    ERR err = JET_errSuccess;


    CBFIssueList bfil;

    for ( IBF ibf = 0; ibf < cbfInit; ibf++ )
    {
        PBF pbf = PbfBFICacheIbf( ibf );


        if ( pbf->ifmp != ifmp )
        {
            continue;
        }


        if ( pbf->sxwl.ErrTryAcquireExclusiveLatch() != CSXWLatch::ERR::errSuccess )
        {
            continue;
        }


        if ( !FBFIUpdatablePage( pbf ) )
        {
            pbf->sxwl.ReleaseExclusiveLatch();
            continue;
        }


        if ( pbf->bfdf == bfdfClean )
        {
            pbf->sxwl.ReleaseExclusiveLatch();
            continue;
        }


        const ERR errPrepareFlush = ErrBFIPrepareFlushPage( pbf, bfltExclusive, IOR( iorpBFDatabaseFlush ), qosIODispatchImmediate, fFalse );

        if ( errPrepareFlush < JET_errSuccess )
        {
            pbf->sxwl.ReleaseExclusiveLatch();

            OSTrace(
                JET_tracetagBufferManager,
                OSFormat( "ErrBFIPrepareFlushPage: pgno=%u:%u errPrepareFlush=%d", (ULONG)pbf->ifmp, pbf->pgno, errPrepareFlush ) );
            
            if ( errPrepareFlush == JET_errOutOfMemory || errPrepareFlush == JET_errOutOfBuffers )
            {
                Error( errPrepareFlush );
            }
            else
            {
                Error( ErrERRCheck( JET_errDiskIO ) );
            }
        }
        Assert( !bfil.FEmpty() );

        
        TraceContextScope tcScope( iorpBFDatabaseFlush );
        err = ErrBFISyncWrite( pbf, bfltExclusive, qosIODispatchImmediate, *tcScope );
        pbf->sxwl.ReleaseExclusiveLatch();

        if ( err < JET_errSuccess )
        {
            Call( err );
        }
    }

HandleError:


    CallS( bfil.ErrIssue( fTrue ) );

    return err;
}



void BFAddUndoInfo( const BFLatch* pbfl, RCE* prce )
{

    Assert( FBFWARLatched( pbfl ) || FBFWriteLatched( pbfl ) );
    Assert( prce->PgnoUndoInfo() == pgnoNull );
    Assert( prce->PrceUndoInfoNext() == prceInvalid );


    PBF pbf = PBF( pbfl->dwContext );

    ENTERCRITICALSECTION ecs( &g_critpoolBFDUI.Crit( pbf ) );

    BFIAddUndoInfo( pbf, prce );
}

void BFRemoveUndoInfo( RCE* const prce, const LGPOS lgposModify )
{

    Assert( prce != prceNil );


    while ( prce->PgnoUndoInfo() != pgnoNull )
    {

        BFHash::CLock lockHash;
        g_bfhash.ReadLockKey( IFMPPGNO( prce->Ifmp(), prce->PgnoUndoInfo() ), &lockHash );

        PGNOPBF pgnopbf;
        if ( g_bfhash.ErrRetrieveEntry( &lockHash, &pgnopbf ) == BFHash::ERR::errSuccess )
        {

            CCriticalSection* const pcrit = &g_critpoolBFDUI.Crit( pgnopbf.pbf );
            pcrit->Enter();


            if (    prce->PgnoUndoInfo() == pgnopbf.pbf->pgno &&
                    prce->Ifmp() == pgnopbf.pbf->ifmp )
            {

                if ( pgnopbf.pbf->pbfTimeDepChainNext == pbfNil )
                {
#ifdef DEBUG

                    RCE* prceT;
                    for (   prceT = pgnopbf.pbf->prceUndoInfoNext;
                            prceT != prceNil && prceT != prce;
                            prceT = prceT->PrceUndoInfoNext() )
                    {
                    }

                    Assert( prceT == prce );

#endif


                    FMP*    pfmp    = &g_rgfmp[ prce->Ifmp() ];
                    PIB*    ppib    = prce->Pfucb()->ppib;

                    if (    ppib->Level() == 1 &&
                            pfmp->FLogOn() &&
                            CmpLgpos( &ppib->lgposCommit0, &lgposMax ) != 0 )
                    {
                        Assert( !pfmp->Pinst()->m_plog->FLogDisabled() );
                        BFISetLgposModify( pgnopbf.pbf, ppib->lgposCommit0 );
                    }


                    BFIRemoveUndoInfo( pgnopbf.pbf, prce, lgposModify );


                    pcrit->Leave();
                }


                else
                {

                    pcrit->Leave();


                    ENTERCRITICALSECTION ecsDepend( &g_critBFDepend );


                    for (   PBF pbfVer = pgnopbf.pbf;
                            pbfVer != pbfNil;
                            pbfVer = pbfVer->pbfTimeDepChainNext )
                    {

                        ENTERCRITICALSECTION ecs( &g_critpoolBFDUI.Crit( pbfVer ) );

                        Expected( prce->Ifmp() == pbfVer->ifmp );
                        Expected( prce->PgnoUndoInfo() == pbfVer->pgno || prce->PgnoUndoInfo() == pgnoNull );


                        if (    prce->PgnoUndoInfo() == pbfVer->pgno &&
                                prce->Ifmp() == pbfVer->ifmp )
                        {
                            RCE* prceT;
                            for (   prceT = pbfVer->prceUndoInfoNext;
                                    prceT != prceNil && prceT != prce;
                                    prceT = prceT->PrceUndoInfoNext() )
                            {
                            }

                            if ( prceT != prceNil )
                            {

                                FMP*    pfmp    = &g_rgfmp[ prce->Ifmp() ];
                                PIB*    ppib    = prce->Pfucb()->ppib;

                                if (    ppib->Level() == 1 &&
                                        pfmp->FLogOn() &&
                                        CmpLgpos( &ppib->lgposCommit0, &lgposMax ) != 0 )
                                {
                                    Assert( !pfmp->Pinst()->m_plog->FLogDisabled() );

                                    BFISetLgposModify( pbfVer, ppib->lgposCommit0 );
                                }


                                BFIRemoveUndoInfo( pbfVer, prce, lgposModify );


                                break;
                            }
                        }


                        else
                        {

                            break;
                        }
                    }
                }
            }


            else
            {

                pcrit->Leave();
            }
        }

        g_bfhash.ReadUnlockKey( &lockHash );
    }


    Assert( prce->PgnoUndoInfo() == pgnoNull );
}




typedef CTable< DWORD_PTR, CPagePointer >   CReferencedPages;

inline INT CReferencedPages::CKeyEntry:: Cmp( const DWORD_PTR& dw ) const
{
    return (INT)( DwPage() - dw );
}

inline INT CReferencedPages::CKeyEntry:: Cmp( const CReferencedPages::CKeyEntry& keyentry ) const
{
    return Cmp( keyentry.DwPage() );
}

#pragma warning( disable : 4509 )

void BFIBuildReferencedPageListForCrashDump( CReferencedPages * ptableReferencedPages )
{
    
    CArray< CPagePointer >  arrayReferencedPages;

    TRY
    {


        for ( size_t ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
        {
            const INST * const pinst    = g_rgpinst[ ipinst ];
        
            if ( pinstNil != pinst )
            {


                for ( PIB * ppib = pinst->m_ppibGlobal; ppibNil != ppib; ppib = ppib->ppibNext )
                {

                    
                    for ( FUCB * pfucb = ppib->pfucbOfSession; pfucbNil != pfucb; pfucb = pfucb->pfucbNextOfSession )
                    {
                        void * const    pvPage  = pfucb->csr.PvBufferForCrashDump();

                        if ( NULL != pvPage )
                        {
                            CPagePointer    pagepointer( (DWORD_PTR)pvPage );


                            (void) arrayReferencedPages.ErrSetEntry( arrayReferencedPages.Size(), pagepointer );
                        }
                    }
                }
            }
        }
    }
    EXCEPT( efaExecuteHandler )
    {

        g_fBFErrorBuildingReferencedPageListForCrashDump = fTrue;
    }


    (void) ptableReferencedPages->ErrCloneArray( arrayReferencedPages );
}

INLINE BOOL FBFIMismatchedVMPageInCrashDump(
    const BYTE * const  pbPage,
    const size_t        cbPage,
    const BYTE * const  pbVMPage,
    const size_t        cbVMPage )
{
    BOOL                fMismatch   = fFalse;

    if ( cbPage > cbVMPage )
    {

        if ( pbVMPage < pbPage || pbVMPage + cbVMPage > pbPage + cbPage )
        {
            fMismatch = fTrue;
        }
    }
    else if ( cbPage < cbVMPage )
    {

        if ( pbPage < pbVMPage || pbPage + cbPage > pbVMPage + cbVMPage )
        {
            fMismatch = fTrue;
        }
    }
    else
    {

        if ( pbPage != pbVMPage )
        {
            fMismatch = fTrue;
        }
    }

    return fMismatch;
}

ERR
ErrBFIInspectForInclusionInCrashDump(
        const IBF ibf,
        const JET_GRBIT grbit,
        const VOID *pvVMPage,
        const size_t cbVMPage,
        CReferencedPages &tableReferencedPages,
        BOOL *pfIncludeVMPage )
{
    ERR err = JET_errSuccess;

    TRY
    {
        const PBF   pbf     = PbfBFICacheIbf( ibf );

        cBFInspectedForInclusionInCrashDump++;

        if ( NULL == pbf->pv
            || pbf->fQuiesced
            || pbf->fAvailable
            || JET_errPageNotInitialized == pbf->err
            || errBFIPageFaultPending == pbf->err )
        {

            cBFMayBeRemovedFromCrashDump++;
        }


        else if ( pvVMPage != NULL &&
                  FBFIMismatchedVMPageInCrashDump( (BYTE *)pbf->pv, g_rgcbPageSize[pbf->icbBuffer], (BYTE *)pvVMPage, cbVMPage ) )
        {
            *pfIncludeVMPage = fTrue;
            cBFMismatchedVMPageIncludedInCrashDump++;
        }


        else if ( ( grbit & JET_bitDumpCacheIncludeDirtyPages ) != 0 &&
                      pbf->bfdf > bfdfUntidy ||
                  ( grbit & JET_bitDumpCacheMaximum ) != 0 )
        {
            *pfIncludeVMPage = fTrue;
            cBFDirtiedPageIncludedInCrashDump++;
        }


        else if ( ( grbit & JET_bitDumpCacheIncludeCachedPages ) != 0 &&
                    ( pbf->ifmp != ifmpNil || pbf->pgno != pgnoNull ) ||
                  ( grbit & JET_bitDumpCacheMaximum ) != 0 )
        {
            *pfIncludeVMPage = fTrue;
            cBFCachedPageIncludedInCrashDump++;
        }


        else if ( pbf->sxwl.FLatched() )
        {
            *pfIncludeVMPage = fTrue;
            cBFLatchedPageIncludedInCrashDump++;
        }


        else if ( tableReferencedPages.SeekEQ( DWORD_PTR( pbf->pv ) ) )
        {
            *pfIncludeVMPage = fTrue;
            cBFReferencedPageIncludedInCrashDump++;
        }


        else if ( g_bflruk.FRecentlyTouched( pbf, 2000 ) )
        {
            *pfIncludeVMPage = fTrue;
            cBFRecentlyTouchedPageIncludedInCrashDump++;
        }


        else if (   JET_errSuccess != pbf->err &&
                    errBFIPageNotVerified != pbf->err )
        {
            *pfIncludeVMPage = fTrue;
            cBFErrorIncludedInCrashDump++;
        }


        else if (   NULL != pbf->pWriteSignalComplete )
        {
            *pfIncludeVMPage = fTrue;
            cBFIOIncludedInCrashDump++;
        }


        else if ( ( grbit & JET_bitDumpCacheIncludeCorruptedPages ) != 0 &&
                    ( errBFIPageNotVerified == pbf->err ||
                      errBFIPageRemapNotReVerified == pbf->err ) ||
                  ( grbit & JET_bitDumpCacheMaximum ) != 0 )
        {

            *pfIncludeVMPage = fTrue;
            cBFUnverifiedIncludedInCrashDump++;
        }


        else
        {
            cBFMayBeRemovedFromCrashDump++;
        }
    }
    EXCEPT( efaExecuteHandler )
    {
        err = ErrERRCheck( JET_errInternalError );
    }

    return err;
}

ERR ErrBFConfigureProcessForCrashDump( const JET_GRBIT grbit )
{
    JET_ERR             err             = JET_errSuccess;
    CReferencedPages    tableReferencedPages;


    
    UINT aaOriginal = COSLayerPreInit::SetAssertAction( JET_AssertSkipAll );

    if ( grbit & JET_bitDumpUnitTest )
    {
        
        Assert( 0 );
    }
    

    g_tickBFPreparingCrashDump = TickOSTimeCurrent();

    
    if ( !( grbit & JET_bitDumpUnitTest ) && !BoolParam( JET_paramEnableViewCache ) )
    {

        if ( grbit & JET_bitDumpCacheMaximum )
        {
            Error( JET_errSuccess );
        }


        if ( OSMemoryPageWorkingSetPeak() < 100 * 1024 * 1024 )
        {
            Error( JET_errSuccess );
        }

        
        if ( cbfInit < ( 100 * 1024 * 1024 ) / g_cbPageMax )
        {
            Error( JET_errSuccess );
        }
    
    }


    if ( !g_fBFInitialized )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }


    if ( OSMemoryPageCommitGranularity() == 0 ||
         g_icbCacheMax == icbPageInvalid ||
         cbfInit == 0 ||
         g_rgpbfChunk == NULL ||
         g_cbfChunk == 0 ||
         ( !BoolParam( JET_paramEnableViewCache ) &&
           ( g_rgpvChunk == NULL ||
             g_cpgChunk == 0 ) ) )
    {
        Error( ErrERRCheck( JET_errIllegalOperation ) );
    }


    BFIBuildReferencedPageListForCrashDump( &tableReferencedPages );

    if ( !BoolParam( JET_paramEnableViewCache ) )
    {
        if ( grbit & JET_bitDumpCacheNoDecommit )
        {
            Error( JET_errSuccess );
        }


        const size_t cbVMPage = OSMemoryPageCommitGranularity();
        size_t cbitVMPage;
        for ( cbitVMPage = 0; (size_t)1 << cbitVMPage != cbVMPage; cbitVMPage++ );

        const size_t    cbfVMPage   = max( 1, cbVMPage / g_rgcbPageSize[g_icbCacheMax] );
        const size_t    cpgBF       = max( 1, g_rgcbPageSize[g_icbCacheMax] / cbVMPage );


        for ( size_t iCacheChunk = 0, ibf = 0;
            ibf < (size_t)cbfInit && iCacheChunk < cCacheChunkMax && NULL != g_rgpvChunk[ iCacheChunk ];
            iCacheChunk++ )
        {
            const size_t    iVMPageMin      = (size_t)g_rgpvChunk[ iCacheChunk ] >> cbitVMPage;
            const size_t    iVMPageMax      = iVMPageMin + ( g_cpgChunk * g_rgcbPageSize[g_icbCacheMax] >> cbitVMPage );
            BOOL            fIncludeVMPage  = fFalse;
            size_t          ipgBF           = 0;


            for ( size_t iVMPage = iVMPageMin; ibf < (size_t)cbfInit && iVMPage < iVMPageMax; iVMPage++ )
            {
                void* const pvVMPage = (void*)( iVMPage << cbitVMPage );

                if ( 0 != ipgBF )
                {

                    NULL;
                }
                else
                {

                    for ( IBF ibfT = ibf; ibfT < cbfCacheAddressable && ibfT < IBF( ibf + cbfVMPage ); ibfT++ )
                    {
                        err = ErrBFIInspectForInclusionInCrashDump( ibfT, grbit, pvVMPage, cbVMPage, tableReferencedPages, &fIncludeVMPage );
                    }
                }


                if ( fIncludeVMPage )
                {
                    cBFVMPagesIncludedInCrashDump++;
                }
                else
                {
                    OSMemoryPageDecommit( pvVMPage, cbVMPage );
                    cBFVMPagesRemovedFromCrashDump++;

                }


                if ( ++ipgBF >= cpgBF )
                {
                    fIncludeVMPage = fFalse;
                    ipgBF = 0;
                    ibf += cbfVMPage;
                }
            }
        }
    }
    else
    {
        BOOL fIncludePage;
        for ( IBF ibf = 0; ibf < cbfCacheAddressable; ibf++ )
        {
            fIncludePage = fFalse;
            err = ErrBFIInspectForInclusionInCrashDump( ibf, grbit, NULL, 0, tableReferencedPages, &fIncludePage );
            if ( fIncludePage )
            {
                const PBF pbf = PbfBFICacheIbf( ibf );
                OSErrorRegisterForWer( pbf->pv, g_rgcbPageSize[pbf->icbBuffer] );
                cBFVMPagesIncludedInCrashDump++;
            }
        }
    }

HandleError:


    g_tickBFCrashDumpPrepared = TickOSTimeCurrent();
    g_errBFCrashDumpResult    = err;


    COSLayerPreInit::SetAssertAction( aaOriginal );

    return JET_errSuccess;
}

#pragma warning( default : 4509 )



void BFSetBFFMPContextAttached( IFMP ifmp )
{
    FMP* pfmp = &g_rgfmp[ ifmp ];

    if ( pfmp->FBFContext() )
    {
        pfmp->EnterBFContextAsWriter();

        BFFMPContext* pbffmp = ( BFFMPContext* )pfmp->DwBFContext();
        if ( pbffmp )
        {
            pbffmp->fCurrentlyAttached = fTrue;
        }

        pfmp->LeaveBFContextAsWriter();
    }

#ifdef PERFMON_SUPPORT
    for ( IBF ibf = 0; ibf < cbfInit; ibf++ )
    {
        PBF pbf = PbfBFICacheIbf( ibf );
    
        if ( pbf->ifmp != ifmp )
        {
            continue;
        }

        PERFOpt( cBFCache.Inc( PinstFromIfmp( ifmp ), pbf->tce, ifmp ) );
        if ( pbf->err == errBFIPageNotVerified )
        {
            PERFOpt( cBFCacheUnused.Inc( PinstFromIfmp( ifmp ), pbf->tce ) );
        }

        PERFOpt( g_cbCacheUnattached -= g_rgcbPageSize[pbf->icbBuffer] );
    }
#endif
}

void BFResetBFFMPContextAttached( IFMP ifmp )
{
    FMP* pfmp = &g_rgfmp[ ifmp ];

#ifdef PERFMON_SUPPORT
    for ( IBF ibf = 0; ibf < cbfInit; ibf++ )
    {
        PBF pbf = PbfBFICacheIbf( ibf );
    
        if ( pbf->ifmp != ifmp )
        {
            continue;
        }

        PERFOpt( cBFCache.Dec( PinstFromIfmp( ifmp ), pbf->tce, ifmp ) );
        if ( pbf->err == errBFIPageNotVerified )
        {
            PERFOpt( cBFCacheUnused.Dec( PinstFromIfmp( ifmp ), pbf->tce ) );
        }

        PERFOpt( g_cbCacheUnattached += g_rgcbPageSize[pbf->icbBuffer] );
    }
#endif
    
    pfmp->EnterBFContextAsWriter();

    BFFMPContext* pbffmp = ( BFFMPContext* )pfmp->DwBFContext();
    if ( pbffmp )
    {
        Assert( pbffmp );
        pbffmp->fCurrentlyAttached = fFalse;
    }

    pfmp->LeaveBFContextAsWriter();
}


ERR ErrBFPatchPage(
    __in                        const IFMP      ifmp,
    __in                        const PGNO      pgno,
    __in_bcount(cbToken)        const void *    pvToken,
    __in                        const INT       cbToken,
    __in_bcount(cbPageImage)    const void *    pvPageImage,
    __in                        const INT       cbPageImage )
{
    ERR             err = JET_errSuccess;
    BFLatch         bfl;
    BFLRUK::CLock   lockLRUK;
    bool            fLockedLRUK = false;
    PBF             pbf = NULL;
    CPAGE           cpage;
    TraceContextScope   tcPatchPage( iorpPatchFix );

    tcPatchPage->nParentObjectClass = tceNone;

    OSTrace(
        JET_tracetagBufferManager,
        OSFormat( "Patching ifmp:pgno %d:%d", (ULONG)ifmp, pgno ) );

    Call( ErrBFILatchPage( &bfl, ifmp, pgno, BFLatchFlags( bflfNoFaultFail | bflfNoEventLogging ), bfltWrite, BfpriBFMake( PctFMPCachePriority( ifmp ), (BFTEMPOSFILEQOS)qosIODispatchImmediate ), *tcPatchPage ) );

    CallSx( err, wrnBFPageFault );

    pbf = (PBF) bfl.dwContext;


    if ( pbf->bfdf > bfdfUntidy )
    {
        Error( ErrERRCheck( JET_errDatabaseInUse ) );
    }
    else if ( pbf->bfdf == bfdfUntidy )
    {

        SetPageChecksum( pbf->pv, CbBFIBufferSize(pbf), databasePage, pbf->pgno );

        pbf->bfdf = bfdfClean;
    }
    Assert( bfdfClean == pbf->bfdf );


    BFIRehydratePage( pbf );


    Enforce( pbf->ifmp == ifmp );
    Enforce( pbf->pgno == pgno );
    Enforce( FBFICurrentPage( pbf, ifmp, pgno ) );
    Enforce( FBFIUpdatablePage( pbf ) );


    BFIResetLgposOldestBegin0( pbf );


    BOOL fPatched = fFalse;
    Call( PagePatching::ErrDoPatch( ifmp, pgno, &bfl, pvToken, cbToken, pvPageImage, cbPageImage, &fPatched ) );
    if ( !fPatched )
    {
        goto HandleError;
    }



    err = ErrBFITryPrepareFlushPage( pbf, bfltWrite, IOR( iorpPatchFix ), qosIODispatchImmediate, fFalse );
    if ( err < JET_errSuccess )
    {
        Error( ErrERRCheck( JET_errDatabaseInUse ) );
    }

    BFIAssertReadyForWrite( pbf );

    Assert( 0 == CmpLgpos( pbf->lgposModify, lgposMin ) );

    Assert( 0 == CmpRbspos( pbf->rbsposSnapshot, rbsposMin ) );

    Assert( 0 == CmpLgpos( pbf->lgposOldestBegin0, lgposMax ) );

    Call( ErrBFISyncWrite( pbf, bfltWrite, qosIODispatchImmediate, *tcPatchPage ) );

    Call( ErrIOFlushDatabaseFileBuffers( ifmp, iofrPagePatching ) );



    Expected( pbf->err >= JET_errSuccess );


    pbf->fNewlyEvicted = fTrue;


    AssertRTL( FBFICurrentPage( pbf, ifmp, pgno ) );
    

    g_bflruk.LockResourceForEvict( pbf, &lockLRUK );
    fLockedLRUK = true;


    pbf->sxwl.ReleaseWriteLatch();


    err = ErrBFIEvictPage( pbf, &lockLRUK, bfefReasonPatch );


    pbf = NULL;

    if ( err < JET_errSuccess )
    {
        Error( ErrERRCheck( JET_errDatabaseInUse ) );
    }

HandleError:

    if ( err < JET_errSuccess )
    {
        OSTrace(
            JET_tracetagBufferManager,
            OSFormat( "Patching ifmp:pgno %d:%d fails with error %d", (ULONG)ifmp, pgno, err ) );
    }

    if ( pbf )
    {

        pbf->sxwl.ReleaseWriteLatch();
    }

    if ( fLockedLRUK )
    {

        g_bflruk.UnlockResourceForEvict( &lockLRUK );
    }

    return err;
}



ERR ErrBFTestEvictPage( _In_ const IFMP ifmp, _In_ const PGNO pgno )
{
    ERR             err = errCodeInconsistency;

    OSTrace( JET_tracetagBufferManager, OSFormat( "Test Evicting ifmp:pgno %d:%d", (ULONG)ifmp, pgno ) );


    ULONG iIter = 0;
    TICK tickStart = TickOSTimeCurrent();
    do
    {

        BFHash::CLock   lockHash;
        PGNOPBF         pgnopbf;
        g_bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lockHash );
        BFHash::ERR errHash = g_bfhash.ErrRetrieveEntry( &lockHash, &pgnopbf );
        g_bfhash.ReadUnlockKey( &lockHash );
        Assert( errHash == BFHash::ERR::errSuccess || errHash == BFHash::ERR::errEntryNotFound );

        if( errHash == BFHash::ERR::errEntryNotFound )
        {

            err = JET_errSuccess;
            break;
        }

        if( errHash == BFHash::ERR::errSuccess )
        {


            Assert( pgnopbf.pbf->ifmp == ifmp &&
                        pgnopbf.pbf->pgno == pgno &&
                        pgnopbf.pbf->fCurrentVersion );


            BFLRUK::CLock   lockLRUK;
            g_bflruk.LockResourceForEvict( pgnopbf.pbf, &lockLRUK );


            g_bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lockHash );
            errHash = g_bfhash.ErrRetrieveEntry( &lockHash, &pgnopbf );
            g_bfhash.ReadUnlockKey( &lockHash );

            if( errHash != BFHash::ERR::errSuccess )
            {

                Assert( errHash == BFHash::ERR::errEntryNotFound );

                err = JET_errSuccess;
            }
            else
            {


                err = ErrBFIEvictPage( pgnopbf.pbf, &lockLRUK, bfefReasonTest );

                if ( err < JET_errSuccess )
                {
                    if ( err == errBFIPageDirty )
                    {
                        CBFIssueList    bfil;


                        (void)ErrBFIFlushPage( pgnopbf.pbf, IOR( iorpDirectAccessUtil, iorfForeground ), qosIODispatchImmediate, bfdfDirty, fFalse , NULL );

                        CallS( bfil.ErrIssue() );


                        UtilSleep( cmsecWaitIOComplete );
                    }

                }
                else
                {
                    iIter--;
                }

                
                err = ErrERRCheck( JET_errDatabaseInUse );
            }


            g_bflruk.UnlockResourceForEvict( &lockLRUK );
        }

        iIter++;
    }
    while ( err == JET_errDatabaseInUse && ( iIter < 10 || DtickDelta( tickStart, TickOSTimeCurrent() ) < 300 ) );

    if ( err < JET_errSuccess )
    {
        OSTrace( JET_tracetagBufferManager, OSFormat( "Evicting ifmp:pgno %d:%d fails with error %d", (ULONG)ifmp, pgno, err ) );
    }

    return err;
}





BOOL    g_fBFInitialized = fFalse;

BYTE*   g_rgbBFTemp = NULL;

TICK    g_tickBFPreparingCrashDump;
size_t  cBFInspectedForInclusionInCrashDump;
size_t  cBFMismatchedVMPageIncludedInCrashDump;
size_t  cBFDirtiedPageIncludedInCrashDump;
size_t  cBFCachedPageIncludedInCrashDump;
size_t  cBFLatchedPageIncludedInCrashDump;
size_t  cBFReferencedPageIncludedInCrashDump;
size_t  cBFRecentlyTouchedPageIncludedInCrashDump;
size_t  cBFErrorIncludedInCrashDump;
size_t  cBFIOIncludedInCrashDump;
size_t  cBFUnverifiedIncludedInCrashDump;
size_t  cBFMayBeRemovedFromCrashDump;
size_t  cBFVMPagesIncludedInCrashDump;
size_t  cBFVMPagesRemovedFromCrashDump;
TICK    g_tickBFCrashDumpPrepared;
ERR     g_errBFCrashDumpResult;
BOOL    g_fBFErrorBuildingReferencedPageListForCrashDump  = fFalse;



double  g_dblBFSpeedSizeTradeoff;



ULONG cBFOpportuneWriteIssued;


#ifdef MINIMAL_FUNCTIONALITY
#else
#pragma data_seg( "cacheline_aware_data" )
#endif
BFHash g_bfhash( rankBFHash );
#ifdef MINIMAL_FUNCTIONALITY
#else
#pragma data_seg()
#endif

double g_dblBFHashLoadFactor;
double g_dblBFHashUniformity;



#ifdef MINIMAL_FUNCTIONALITY
#else
#pragma bss_seg( "cacheline_aware_data" )
#endif
BFAvail g_bfavail;
#ifdef MINIMAL_FUNCTIONALITY
#else
#pragma bss_seg()
#endif


#ifdef MINIMAL_FUNCTIONALITY
#else
#pragma bss_seg( "cacheline_aware_data" )
#endif
BFQuiesced g_bfquiesced;
#ifdef MINIMAL_FUNCTIONALITY
#else
#pragma bss_seg()
#endif


#ifdef MINIMAL_FUNCTIONALITY
#else
#pragma data_seg( "cacheline_aware_data" )
#endif
BFLRUK g_bflruk( rankBFLRUK );
#ifdef MINIMAL_FUNCTIONALITY
#else
#pragma data_seg()
#endif

double g_csecBFLRUKUncertainty;

#ifdef MINIMAL_FUNCTIONALITY
#else
CFastTraceLog* g_pbfftl = NULL;
#endif
IOREASON g_iorBFTraceFile( iorpOsLayerTracing );

class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
{
    public:
        CFileSystemConfiguration()
        {
            m_dtickAccessDeniedRetryPeriod = 500;
        }
} g_fsconfigBFIFTL;

ERR ErrBFIFTLInit()
{
    ERR     err = JET_errSuccess;

#ifdef MINIMAL_FUNCTIONALITY
#else
    WCHAR   wszPath[IFileSystemAPI::cchPathMax];

    Assert( g_pbfftl == NULL );
    Alloc( g_pbfftl = new CFastTraceLog( NULL, &g_fsconfigBFIFTL ) );

#ifdef BFFTL_TRACE_ALWAYS_ON
    OSStrCbCopyW( wszPath, sizeof(wszPath), L".\\bftracer.ftl" );
    if ( fTrue )
#else
    if ( FOSConfigGet_( L"BF", L"FTL Trace File", wszPath, sizeof(wszPath) )
            && wszPath[0] )
#endif
    {
        err = g_pbfftl->ErrFTLInitWriter( wszPath, &g_iorBFTraceFile, CFastTraceLog::ftlifReOpenExisting );

        if ( err == JET_errFileAccessDenied )
        {
            WCHAR wszDiffTrace[33];
            OSStrCbFormatW( wszDiffTrace, sizeof(wszDiffTrace), L".\\bftracePID%d.ftl", DwUtilProcessId() );
            err = g_pbfftl->ErrFTLInitWriter( wszDiffTrace, &g_iorBFTraceFile, CFastTraceLog::ftlifReOpenExisting );
        }

        if ( err == JET_errFileAccessDenied )
        {
            g_pbfftl->SetFTLDisabled();
            err = JET_errSuccess;
        }

        Call( err );
    }
    else
    {
        g_pbfftl->SetFTLDisabled();
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        delete g_pbfftl;
        g_pbfftl = NULL;
    }
#endif

    return err;
}

void BFIFTLTerm()
{
#ifdef MINIMAL_FUNCTIONALITY
    return;
#else
    Assert( g_pbfftl != NULL );
    g_pbfftl->FTLTerm();

    delete g_pbfftl;
    g_pbfftl = NULL;
#endif
}



#ifdef DEBUG
#define ENABLE_BFFTL_TRACING
#endif

INLINE void BFITraceResMgrInit(
    const INT       K,
    const double    csecCorrelatedTouch,
    const double    csecTimeout,
    const double    csecUncertainty,
    const double    dblHashLoadFactor,
    const double    dblHashUniformity,
    const double    dblSpeedSizeTradeoff )
{
#ifdef ENABLE_BFFTL_TRACING
    (void)ErrBFIFTLSysResMgrInit(
        K,
        csecCorrelatedTouch,
        csecTimeout,
        csecUncertainty,
        dblHashLoadFactor,
        dblHashUniformity,
        dblSpeedSizeTradeoff );
#endif

    ETResMgrInit(
        TickOSTimeCurrent(),
        K,
        csecCorrelatedTouch,
        csecTimeout,
        csecUncertainty,
        dblHashLoadFactor,
        dblHashUniformity,
        dblSpeedSizeTradeoff );
}

INLINE void BFITraceResMgrTerm()
{
#ifdef ENABLE_BFFTL_TRACING
    (void)ErrBFIFTLSysResMgrTerm();
#endif

    ETResMgrTerm( TickOSTimeCurrent() );
}

INLINE void BFITraceCachePage(
    const TICK                  tickCache,
    const PBF                   pbf,
    const BFLatchType           bflt,
    const ULONG                 pctPriority,
    const BFLatchFlags          bflf,
    const BFRequestTraceFlags   bfrtf,
    const TraceContext&         tc )
{
    GetCurrUserTraceContext getutc;
    const BYTE bClientType = getutc->context.nClientType;

#ifdef ENABLE_BFFTL_TRACING
#endif

    ETCacheCachePage(
        tickCache,
        pbf->ifmp,
        pbf->pgno,
        bflf,
        bflt,
        pctPriority,
        bfrtf,
        bClientType );
}

INLINE void BFITraceRequestPage(
    const TICK                  tickTouch,
    const PBF                   pbf,
    const ULONG                 pctPriority,
    const BFLatchType           bflt,
    const BFLatchFlags          bflf,
    const BFRequestTraceFlags   bfrtf,
    const TraceContext&         tc )
{
#ifdef ENABLE_BFFTL_TRACING
    GetCurrUserTraceContext getutc;
    const BYTE bClientType = getutc->context.nClientType;

    (void)ErrBFIFTLTouch(
        tickTouch,
        pbf->ifmp,
        pbf->pgno,
        bflt,
        bClientType,
        pctPriority,
        !!( bfrtf & bfrtfUseHistory ),
        !!( bfrtf & bfrtfNewPage ),
        !!( bfrtf & bfrtfNoTouch ),
        !!( bfrtf & bfrtfDBScan ) );
#endif

    if ( FOSEventTraceEnabled< _etguidCacheRequestPage >() )
    {
#ifndef ENABLE_BFFTL_TRACING
        GetCurrUserTraceContext getutc;
        const BYTE bClientType = getutc->context.nClientType;
#endif

        OSEventTrace_(
            _etguidCacheRequestPage,
            10,
            &tickTouch,
            &(pbf->ifmp),
            &(pbf->pgno),
            &bflf,
            &( ( (CPAGE::PGHDR *)( pbf->pv ) )->objidFDP ),
            &( ( (CPAGE::PGHDR *)( pbf->pv ) )->fFlags ),
            &bflt,
            &pctPriority,
            &bfrtf,
            &bClientType );
    }
}

INLINE void BFITraceMarkPageAsSuperCold(
    const IFMP  ifmp,
    const PGNO  pgno )
{
#ifdef ENABLE_BFFTL_TRACING
    (void)ErrBFIFTLMarkAsSuperCold( ifmp, pgno );
#endif

    ETMarkPageAsSuperCold( TickOSTimeCurrent(), ifmp, pgno );
}

INLINE void BFITraceEvictPage(
    const IFMP  ifmp,
    const PGNO  pgno,
    const BOOL  fCurrentVersion,
    const ERR   errBF,
    const ULONG bfef )
{
    const ULONG pctPriority = 0;
    
#ifdef ENABLE_BFFTL_TRACING
    (void)ErrBFIFTLEvict( ifmp, pgno, fCurrentVersion, errBF, bfef, pctPriority );
#endif

    const TICK tickEvictPage = TickOSTimeCurrent();

    ETCacheEvictPage( tickEvictPage, ifmp, pgno, fCurrentVersion, errBF, bfef, pctPriority );
}

INLINE void BFITraceDirtyPage(
    const PBF               pbf,
    const BFDirtyFlags      bfdf,
    const TraceContext&     tc )
{
    auto tick = TickOSTimeCurrent();
    static_assert( sizeof(tick) == sizeof(DWORD), "Compiler magic failing." );



    const LGPOS lgposModifyRead = pbf->lgposModify.LgposAtomicRead();
    const ULONG lgposModifyLGen = (ULONG)lgposModifyRead.lGeneration;
    const USHORT lgposModifyISec = lgposModifyRead.isec;
    const USHORT lgposModifyIb = lgposModifyRead.ib;

    Assert( (LONG)lgposModifyLGen == lgposModifyRead.lGeneration );

#ifdef ENABLE_BFFTL_TRACING
    (void)ErrBFIFTLDirty( pbf->ifmp, pbf->pgno, bfdf, lgposModifyLGen, lgposModifyISec, lgposModifyIb );
#endif

    Assert( CmpLgpos( pbf->lgposModify.LgposAtomicRead(), lgposModifyRead ) >= 0 );

    const CPAGE::PGHDR * ppghdr = (const CPAGE::PGHDR *)pbf->pv;
    GetCurrUserTraceContext getutc;

    if ( pbf->bfdf < bfdfDirty  )
    {
        ETCacheFirstDirtyPage(
                tick,
                pbf->ifmp,
                pbf->pgno,
                ppghdr->objidFDP,
                ppghdr->fFlags,
                bfdf,
                lgposModifyRead.qw,
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
                tc.nParentObjectClass );
    }

    ETCacheDirtyPage(
            tick,
            pbf->ifmp,
            pbf->pgno,
            ppghdr->objidFDP,
            ppghdr->fFlags,
            bfdf,
            lgposModifyRead.qw,
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
            tc.nParentObjectClass );
}

INLINE void BFITraceSetLgposModify(
    const PBF       pbf,
    const LGPOS&    lgposModify )
{
    auto tick = TickOSTimeCurrent();
    static_assert( sizeof(tick) == sizeof(DWORD), "Compiler magic failing." );

#ifdef ENABLE_BFFTL_TRACING
    const ULONG lgposModifyLGen = (ULONG)lgposModify.lGeneration;
    const USHORT lgposModifyISec = lgposModify.isec;
    const USHORT lgposModifyIb = lgposModify.ib;

    Assert( (LONG)lgposModifyLGen == lgposModify.lGeneration );

    (void)ErrBFIFTLSetLgposModify( pbf->ifmp, pbf->pgno, lgposModifyLGen, lgposModifyISec, lgposModifyIb );
#endif

    ETCacheSetLgposModify(
            tick,
            pbf->ifmp,
            pbf->pgno,
            lgposModify.qw );
}

INLINE void BFITraceWritePage(
    const PBF               pbf,
    const FullTraceContext&     tc )
{
    const ULONG bfdfTrace = (ULONG)pbf->bfdf;
    auto tick = TickOSTimeCurrent();

    Assert( tc.etc.iorReason.Iorp() != iorpNone );

#ifdef ENABLE_BFFTL_TRACING
#endif

    ETCacheWritePage(
        tick,
        pbf->ifmp,
        pbf->pgno,
        (((CPAGE::PGHDR *)(pbf->pv))->objidFDP),
        (((CPAGE::PGHDR *)(pbf->pv))->fFlags),
        bfdfTrace,
        tc.utc.context.dwUserID,
        tc.utc.context.nOperationID,
        tc.utc.context.nOperationType,
        tc.utc.context.nClientType,
        tc.utc.context.fFlags,
        tc.utc.dwCorrelationID,
        tc.etc.iorReason.Iorp(),
        tc.etc.iorReason.Iors(),
        tc.etc.iorReason.Iort(),
        tc.etc.iorReason.Ioru(),
        tc.etc.iorReason.Iorf(),
        tc.etc.nParentObjectClass );
}



#ifdef ENABLE_JET_UNIT_TEST


void BFIOB0UnitTest( const ULONG cSec, const ULONG cbSec )
{
    LGPOS lgposPrecision = { 0, 0, 2 * lgenCheckpointTooDeepMax };
    Assert( ( cbCheckpointTooDeepUncertainty % cbSec ) == 0 );
    LGPOS lgposUncertainty = { 0, (USHORT)( cbCheckpointTooDeepUncertainty / cbSec ), 0 };

    BFOB0 bfob0( rankBFOB0 );
    BFOB0::ERR errOB0Init = bfob0.ErrInit(  lgposPrecision.IbOffset( cSec, cbSec ),
                                            lgposUncertainty.IbOffset( cSec, cbSec ),
                                            99.9 );
    Assert( errOB0Init == BFOB0::ERR::errSuccess ||
            errOB0Init == BFOB0::ERR::errOutOfMemory );
    if ( errOB0Init == BFOB0::ERR::errSuccess )
    {
        LGPOS lgpos1;
        lgpos1.SetByIbOffset( lgposMin.IbOffset( cSec, cbSec ), cSec, cbSec );
        BF bf1;
        BFOB0::CLock lock;
        bfob0.LockKeyPtr( lgpos1.IbOffset( cSec, cbSec ), &bf1, &lock );
        BFOB0::ERR errOB0Insert1 = bfob0.ErrInsertEntry( &lock, &bf1 );
        bfob0.UnlockKeyPtr( &lock );
        Assert( errOB0Insert1 == BFOB0::ERR::errSuccess ||
                errOB0Insert1 == BFOB0::ERR::errOutOfMemory );
        if ( errOB0Insert1 == BFOB0::ERR::errSuccess )
        {
            LGPOS lgpos2;
            lgpos2.SetByIbOffset( lgpos1.IbOffset( cSec, cbSec ) + 1 * lgposPrecision.IbOffset( cSec, cbSec ) / 4, cSec, cbSec );
            BF bf2;
            bfob0.LockKeyPtr( lgpos2.IbOffset( cSec, cbSec ), &bf2, &lock );
            BFOB0::ERR errOB0Insert2 = bfob0.ErrInsertEntry( &lock, &bf2 );
            bfob0.UnlockKeyPtr( &lock );
            Assert( errOB0Insert2 == BFOB0::ERR::errSuccess ||
                    errOB0Insert2 == BFOB0::ERR::errOutOfMemory );
            if ( errOB0Insert2 == BFOB0::ERR::errSuccess )
            {
                LGPOS lgpos3;
                lgpos3.SetByIbOffset( lgpos1.IbOffset( cSec, cbSec ) + 2 * lgposPrecision.IbOffset( cSec, cbSec ) / 4 - lgposUncertainty.IbOffset( cSec, cbSec ), cSec, cbSec );
                BF bf3;
                bfob0.LockKeyPtr( lgpos3.IbOffset( cSec, cbSec ), &bf3, &lock );
                BFOB0::ERR errOB0Insert3 = bfob0.ErrInsertEntry( &lock, &bf3 );
                bfob0.UnlockKeyPtr( &lock );
                Assert( errOB0Insert3 == BFOB0::ERR::errSuccess ||
                        errOB0Insert3 == BFOB0::ERR::errOutOfMemory );
                if ( errOB0Insert3 == BFOB0::ERR::errSuccess )
                {
                    LGPOS lgpos4;
                    lgpos4.SetByIbOffset( lgpos1.IbOffset( cSec, cbSec ) + 2 * lgposPrecision.IbOffset( cSec, cbSec ) / 4 + 2 * lgposUncertainty.IbOffset( cSec, cbSec ), cSec, cbSec );
                    BF bf4;
                    bfob0.LockKeyPtr( lgpos4.IbOffset( cSec, cbSec ), &bf4, &lock );
                    BFOB0::ERR errOB0Insert4 = bfob0.ErrInsertEntry( &lock, &bf4 );
                    bfob0.UnlockKeyPtr( &lock );
                    Assert( errOB0Insert4 == BFOB0::ERR::errKeyRangeExceeded ||
                            errOB0Insert4 == BFOB0::ERR::errOutOfMemory );
                    
                    LGPOS lgpos5;
                    lgpos5.SetByIbOffset( lgpos1.IbOffset( cSec, cbSec ) + 3 * lgposPrecision.IbOffset( cSec, cbSec ) / 4, cSec, cbSec );
                    BF bf5;
                    bfob0.LockKeyPtr( lgpos5.IbOffset( cSec, cbSec ), &bf5, &lock );
                    BFOB0::ERR errOB0Insert5 = bfob0.ErrInsertEntry( &lock, &bf5 );
                    bfob0.UnlockKeyPtr( &lock );
                    Assert( errOB0Insert5 == BFOB0::ERR::errKeyRangeExceeded ||
                            errOB0Insert5 == BFOB0::ERR::errOutOfMemory );
                    
                    LGPOS lgpos6;
                    lgpos6.SetByIbOffset( lgpos1.IbOffset( cSec, cbSec ) + 4 * lgposPrecision.IbOffset( cSec, cbSec ) / 4 - 2 * lgposUncertainty.IbOffset( cSec, cbSec ), cSec, cbSec );
                    BF bf6;
                    bfob0.LockKeyPtr( lgpos6.IbOffset( cSec, cbSec ), &bf6, &lock );
                    BFOB0::ERR errOB0Insert6 = bfob0.ErrInsertEntry( &lock, &bf6 );
                    bfob0.UnlockKeyPtr( &lock );
                    Assert( errOB0Insert6 == BFOB0::ERR::errKeyRangeExceeded ||
                            errOB0Insert6 == BFOB0::ERR::errOutOfMemory );
                    
                    bfob0.LockKeyPtr( lgpos3.IbOffset( cSec, cbSec ), &bf3, &lock );
                    BFOB0::ERR errOB0Delete3 = bfob0.ErrDeleteEntry( &lock );
                    Assert( errOB0Delete3 == BFOB0::ERR::errSuccess );
                    bfob0.UnlockKeyPtr( &lock );
                }
                
                bfob0.LockKeyPtr( lgpos2.IbOffset( cSec, cbSec ), &bf2, &lock );
                BFOB0::ERR errOB0Delete2 = bfob0.ErrDeleteEntry( &lock );
                Assert( errOB0Delete2 == BFOB0::ERR::errSuccess );
                bfob0.UnlockKeyPtr( &lock );
            }
            bfob0.LockKeyPtr( lgpos1.IbOffset( cSec, cbSec ), &bf1, &lock );
            BFOB0::ERR errOB0Delete1 = bfob0.ErrDeleteEntry( &lock );
            Assert( errOB0Delete1 == BFOB0::ERR::errSuccess );
            bfob0.UnlockKeyPtr( &lock );
        }
        bfob0.Term();
    }
}

JETUNITTEST( BF, BFIOB0SmallLogsSmallSectors )
{
    BFIOB0UnitTest( ( 128 * 1024 ) / 512, 512 );
}

JETUNITTEST( BF, BFIOB0SmallLogsLargeSectors )
{
    BFIOB0UnitTest( ( 128 * 1024 ) / ( 4 * 1024 ), 4 * 1024 );
}

JETUNITTEST( BF, BFIOB0LargeLogsSmallSectors )
{
    BFIOB0UnitTest( ( 1 * 1024 * 1024 ) / 512, 512 );
}

JETUNITTEST( BF, BFIOB0LargeLogsLargeSectors )
{
    BFIOB0UnitTest( ( 1 * 1024 * 1024 ) / ( 4 * 1024 ), 4 * 1024 );
}

#endif


QWORD BFIOB0Offset( const IFMP ifmp, const LGPOS* const plgpos )
{
    INST* const pinst   = PinstFromIfmp( ifmp );
    LOG* const  plog    = pinst->m_plog;

    OnDebug( plog->CbLGOffsetLgposForOB0( *plgpos, lgposMin ) );

    LGPOS lgposFile = { 0, 0, 1 };
    LGPOS lgposIb = { plgpos->ib, plgpos->isec, 0 };
    QWORD offsetFile = plog->CbLGOffsetLgposForOB0( lgposFile, lgposMin );
    QWORD p2offsetFile = LNextPowerOf2( (LONG)offsetFile );
    return p2offsetFile * plgpos->lGeneration + plog->CbLGOffsetLgposForOB0( lgposIb, lgposMin );
}

INLINE LGPOS BFIOB0Lgpos( const IFMP ifmp, LGPOS lgpos, const BOOL fNextBucket )
{
    INST* const pinst   = PinstFromIfmp( ifmp );
    LOG* const  plog    = pinst->m_plog;

    lgpos = plog->LgposLGFromIbForOB0(
            ( plog->CbLGOffsetLgposForOB0( lgpos, lgposMin ) /
                cbCheckpointTooDeepUncertainty +
                ( fNextBucket ? 1 : 0 ) ) *
            cbCheckpointTooDeepUncertainty );


    if ( lgpos.lGeneration < 1 )
    {
        Assert( lgpos.lGeneration == 0 );
        lgpos.lGeneration = 1;
        lgpos.isec = 0;
        lgpos.ib = 0;
    }
    
    return lgpos;
}



CRITPOOL< BF > g_critpoolBFDUI;



BOOL g_fBFCacheInitialized = fFalse;


CCriticalSection g_critCacheSizeSetTarget( CLockBasicInfo( CSyncBasicInfo( "g_critCacheSizeSetTarget" ), rankBFCacheSizeSet, 0 ) );
CCriticalSection g_critCacheSizeResize( CLockBasicInfo( CSyncBasicInfo( "g_critCacheSizeResize" ), rankBFCacheSizeResize, CLockDeadlockDetectionInfo::subrankNoDeadlock ) );

volatile LONG_PTR   cbfCacheTarget;
volatile LONG_PTR   g_cbfCacheTargetOptimal;
LONG_PTR            g_cbfCacheUserOverride;

LONG                g_cbfCacheResident;
LONG                g_cbfCacheClean;


LONG                g_rgcbfCachePages[icbPageMax] = { 0 };
ULONG_PTR           g_cbCacheReservedSize;
ULONG_PTR           g_cbCacheCommittedSize;
volatile LONG_PTR   cbfCacheAddressable;
volatile LONG_PTR   cbfCacheSize;

const LONG g_rgcbPageSize[icbPageMax] =
{
      0,
      0,
      64,
      128,
      256,
      512,
      1024,
      2*1024,
      4*1024,
      8*1024,
      12*1024,
      16*1024,
      20*1024,
      24*1024,
      28*1024,
      32*1024,
};


DWORD           g_cbfCommitted;
DWORD           g_cbfNewlyCommitted;
DWORD           g_cbfNewlyEvictedUsed;
DWORD           g_cbNewlyEvictedUsed;
DWORD           g_cpgReclaim;
DWORD           g_cResidenceCalc;

CMovingAverage< LONG_PTR, cMaintCacheSamplesAvg >   g_avgCbfCredit( 0 );

BFCacheStatsChanges g_statsBFCacheResidency;


LONG_PTR        g_cpgChunk;
void**          g_rgpvChunk;
ICBPage         g_icbCacheMax;



LONG_PTR        cbfInit;
LONG_PTR        g_cbfChunk;
BF**            g_rgpbfChunk;



ERR ErrBFICacheInit( __in const LONG cbPageSizeMax )
{
    ERR     err = JET_errSuccess;


    Assert( g_critCacheSizeResize.FNotOwner() );
    BFICacheIResetTarget();
    g_cbfCacheUserOverride    = 0;
    cbfCacheAddressable     = 0;
    cbfCacheSize            = 0;
    g_cbfCacheResident        = 0;
    g_cbfCacheClean           = 0;

    memset( g_rgcbfCachePages, 0, sizeof( g_rgcbfCachePages ) );
    g_cbCacheReservedSize   = 0;
    g_cbCacheCommittedSize  = 0;

    g_cbfCommitted        = 0;
    g_cbfNewlyCommitted   = 0;
    g_cbfNewlyEvictedUsed = 0;
    g_cbNewlyEvictedUsed  = 0;
    g_cpgReclaim          = 0;
    g_cResidenceCalc      = 0;

    PERFOpt( g_tickBFUniqueReqLast = TickOSTimeCurrent() );

    memset( &g_statsBFCacheResidency, 0, sizeof(g_statsBFCacheResidency) );

    g_cacheram.Reset();
    g_cacheram.ResetStatistics();
    g_avgCbfCredit.Reset( 0 );

    g_cpgChunk            = 0;
    g_rgpvChunk           = NULL;

    cbfInit             = 0;
    g_cbfChunk            = 0;
    g_rgpbfChunk          = NULL;

    Assert( g_rgcbPageSize[icbPage128] == 128 );
    Assert( g_rgcbPageSize[icbPage1KB] == 1024 );
    Assert( g_rgcbPageSize[icbPage12KB] == 12*1024 );
    Assert( g_rgcbPageSize[icbPage32KB] == 32*1024 );

    g_icbCacheMax = IcbBFIBufferSize( cbPageSizeMax );

    if ( icbPageInvalid == g_icbCacheMax ||
        icbPageInvalid == IcbBFIPageSize( cbPageSizeMax ) )
    {
        Error( ErrERRCheck( JET_errInvalidSettings ) );
    }


    const QWORD cbCacheReserveMost = min(   size_t( (double)OSMemoryPageReserveTotal() * fracVAMax ),
                                            max(    pctOverReserve * OSMemoryTotal() / 100,
                                                    UlParam( JET_paramCacheSizeMin ) * g_rgcbPageSize[g_icbCacheMax] ) );

    const LONG_PTR cpgChunkMin = (LONG_PTR)( cbCacheReserveMost / cCacheChunkMax / g_rgcbPageSize[g_icbCacheMax] );
    for ( g_cpgChunk = 1; g_cpgChunk < cpgChunkMin; g_cpgChunk <<= 1 );
    Assert( FPowerOf2( g_cpgChunk ) );


    Alloc( g_rgpvChunk = new void*[ cCacheChunkMax ] );
    memset( g_rgpvChunk, 0, sizeof( void* ) * cCacheChunkMax );


    g_cbfChunk = g_cpgChunk * g_rgcbPageSize[g_icbCacheMax] / sizeof( BF );


    Alloc( g_rgpbfChunk = new PBF[ cCacheChunkMax ] );
    memset( g_rgpbfChunk, 0, sizeof( PBF ) * cCacheChunkMax );


    OnDebug( const LONG_PTR cbfInitialCacheSize = max( cbfCacheMinMin, min( UlParam( JET_paramCacheSizeMin ), UlParam( JET_paramCacheSizeMax ) ) ) );
    BFICacheSetTarget( OnDebug( cbfInitialCacheSize ) );


    RFSSuppressFaultInjection( 33032 );
    RFSSuppressFaultInjection( 49416 );
    RFSSuppressFaultInjection( 48904 );
    RFSSuppressFaultInjection( 65288 );
    RFSSuppressFaultInjection( 40712 );
    RFSSuppressFaultInjection( 57096 );
    RFSSuppressFaultInjection( 44808 );
    RFSSuppressFaultInjection( 61192 );

    Call( ErrBFICacheGrow() );

    RFSUnsuppressFaultInjection( 33032 );
    RFSUnsuppressFaultInjection( 49416 );
    RFSUnsuppressFaultInjection( 48904 );
    RFSUnsuppressFaultInjection( 65288 );
    RFSUnsuppressFaultInjection( 40712 );
    RFSUnsuppressFaultInjection( 57096 );
    RFSUnsuppressFaultInjection( 44808 );
    RFSUnsuppressFaultInjection( 61192 );

    return JET_errSuccess;

HandleError:
    BFICacheTerm();
    return err;
}


void BFICacheTerm()
{

    C_ASSERT( icbPageMax == _countof(g_rgcbfCachePages) );
#ifndef RTM
    for ( LONG icb = 0; icb < _countof(g_rgcbfCachePages); icb++ )
    {
        AssertRTL( g_rgcbfCachePages[icb] == 0 );
    }
#endif


#ifdef DEBUG
    for( LONG_PTR ibf = 0; ibf < cbfInit; ibf++ )
    {
        const BF * pbf = PbfBFICacheIbf( ibf );
        Assert( pbf->fQuiesced || pbf->fAvailable );
    }
#endif


    Assert( !g_fBFCacheInitialized );
    BFICacheIResetTarget();


    Assert( g_critCacheSizeResize.FNotOwner() );
    g_critCacheSizeResize.Enter();
    BFICacheIFree();
    g_critCacheSizeResize.Leave();

    g_cacheram.Reset();

#ifndef RTM

    AssertRTL( g_cbCacheReservedSize == 0 );
    AssertRTL( g_cbCacheCommittedSize == 0 );
    for( INT icbPage = icbPageSmallest; icbPage < icbPageMax; icbPage++ )
    {
        AssertRTL( g_rgcbfCachePages[icbPage] == 0 );
    }
    AssertRTL( g_cbfCommitted == 0 );
    AssertRTL( g_cbfNewlyCommitted == 0 );
    AssertRTL( g_cbfCacheResident == 0 );
    AssertRTL( cbfCacheAddressable == 0 );
    AssertRTL( cbfCacheSize == 0 );
    AssertRTL( g_bfquiesced.FEmpty() );
#endif


    if ( g_rgpbfChunk )
    {
        delete [] g_rgpbfChunk;
        g_rgpbfChunk = NULL;
    }


    if ( g_rgpvChunk )
    {
        delete [] g_rgpvChunk;
        g_rgpvChunk = NULL;
    }
}

INLINE INT CbBFISize( ICBPage icb )
{
    return g_rgcbPageSize[icb];
}


#ifndef RTM
TICK g_tickCacheSetTargetLast = 0;
#endif

void BFICacheISetTarget( const LONG_PTR cbfCacheNew )
{
    Assert( g_critCacheSizeSetTarget.FOwner() );

    Assert( cbfCacheNew > 0 );

    OnNonRTM( g_tickCacheSetTargetLast = TickOSTimeCurrent() );

    const __int64 cbfCacheTargetInitial = (__int64)cbfCacheTarget;
    cbfCacheTarget = cbfCacheNew;

    ETCacheLimitResize( cbfCacheTargetInitial, (__int64)cbfCacheTarget );
}


void BFICacheIResetTarget()
{
    g_critCacheSizeSetTarget.Enter();

    OnNonRTM( g_tickCacheSetTargetLast = TickOSTimeCurrent() );

    cbfCacheTarget = 0;
    g_cbfCacheTargetOptimal = 0;

    g_critCacheSizeSetTarget.Leave();
}



void BFICacheSetTarget( OnDebug( const LONG_PTR cbfCacheOverrideCheck ) )
{
    g_critCacheSizeSetTarget.Enter();


    Assert( cbfCacheOverrideCheck == -1 || cbfCacheOverrideCheck > 0 );



    g_cacheram.SetOptimalResourcePoolSize();


    Assert( ( cbfCacheOverrideCheck == -1 ) || ( cbfCacheTarget >= cbfCacheOverrideCheck ) || BoolParam( JET_paramEnableViewCache ) );

    Assert( cbfCacheTarget >= cbfCacheMinMin );

    g_critCacheSizeSetTarget.Leave();
}


#ifndef RTM
TICK g_tickCacheGrowLast = 0;
#endif

ERR ErrBFICacheGrow()
{
    if ( !g_critCacheSizeResize.FTryEnter() )
    {
        return ErrERRCheck( JET_errTaskDropped );
    }

    ERR err = JET_errSuccess;
    const LONG_PTR cbfCacheAddressableInitial = cbfCacheAddressable;
    const LONG_PTR cbfCacheSizeInitial = cbfCacheSize;

    OnNonRTM( g_tickCacheGrowLast = TickOSTimeCurrent() );

    BOOL fGrowToAvoidDeadlock = fFalse;
    LONG_PTR cbfCacheTargetNew = cbfCacheTarget;

    do
    {
        if ( g_fBFCacheInitialized )
        {
            Assert( cbfCacheTargetNew >= cbfCacheMinMin );
            Enforce( cbfCacheTargetNew > 0 );
        }
        else
        {
            Assert( cbfCacheAddressable == 0 );
            Assert( cbfCacheSize == 0 );
        }

        if ( cbfCacheSize >= cbfCacheTargetNew )
        {
            break;
        }

        PBF pbf = NULL;
        while ( ( cbfCacheSize < cbfCacheTargetNew ) && ( ( pbf = g_bfquiesced.NextMost() ) != NULL ) )
        {
            Assert( pbf->fQuiesced );
            pbf->sxwl.ClaimOwnership( bfltWrite );
            BFIFreePage( pbf, fFalse );
        }

        const LONG_PTR dcbfCacheSize = cbfCacheTargetNew - cbfCacheSize;
        if ( dcbfCacheSize <= 0 )
        {
            break;
        }

        const LONG_PTR cbfCacheAddressableNew = cbfCacheAddressable + dcbfCacheSize;
        Assert( !g_fBFCacheInitialized || ( cbfCacheAddressableNew <= LONG_PTR( (ULONG_PTR)cCacheChunkMax * (ULONG_PTR)min( g_cbfChunk, g_cpgChunk ) ) ) );

        const ERR errT = ErrBFICacheISetSize( cbfCacheAddressableNew );
        Assert( ( cbfCacheAddressable == cbfCacheAddressableNew ) || ( errT < JET_errSuccess ) );

        if ( !fGrowToAvoidDeadlock )
        {
            if ( errT == JET_errOutOfMemory )
            {
                const LONG_PTR cbfCacheDeadlockT = (LONG_PTR)AtomicReadPointer( (void**)&cbfCacheDeadlock );

                if ( ( cbfCacheDeadlockT > cbfCacheSize ) && ( cbfCacheDeadlockT < cbfCacheTargetNew ) )
                {
                    fGrowToAvoidDeadlock = fTrue;
                    cbfCacheTargetNew = cbfCacheDeadlockT;
                }
            }

            err = errT;
        }
        else
        {
            fGrowToAvoidDeadlock = fFalse;
        }
    } while ( fGrowToAvoidDeadlock );

    BFICacheINotifyCacheSizeChanges( cbfCacheAddressableInitial, cbfCacheSizeInitial, cbfCacheAddressable, cbfCacheSize );
    g_critCacheSizeResize.Leave();

    return err;
}


void BFICacheIShrinkAddressable()
{
    Assert( g_critCacheSizeResize.FOwner() );

    const LONG_PTR cbfCacheAddressableInitial = cbfCacheAddressable;
    const LONG_PTR cbfCacheSizeInitial = cbfCacheSize;


    IBF ibf;
    for ( ibf = cbfCacheAddressable - 1; ibf >= 0; ibf-- )
    {
        PBF pbf = PbfBFICacheIbf( ibf );

        if ( !pbf->fQuiesced )
        {
            break;
        }

        Assert( !pbf->fAvailable );
        Assert( !pbf->fInOB0OL && pbf->ob0ic.FUninitialized() );
        g_bfquiesced.Remove( pbf );
    }

    const LONG_PTR cbfCacheAddressableNew = ibf + 1;
    Assert( cbfCacheAddressableNew >= cbfCacheMinMin );

    CallS( ErrBFICacheISetSize( cbfCacheAddressableNew ) );
    Assert( cbfCacheAddressable == cbfCacheAddressableNew );


    BFICacheINotifyCacheSizeChanges( cbfCacheAddressableInitial, cbfCacheSizeInitial, cbfCacheAddressable, cbfCacheSize );
}


void BFICacheIFree()
{
    Assert( g_critCacheSizeResize.FOwner() );
    Assert( cbfCacheTarget == 0 );

    const LONG_PTR cbfCacheAddressableInitial = cbfCacheAddressable;
    const LONG_PTR cbfCacheSizeInitial = cbfCacheSize;

#ifdef DEBUG
    IBF ibf;


    for ( ibf = cbfInit - 1; ibf >= cbfCacheAddressable; ibf-- )
    {
        PBF pbf = PbfBFICacheIbf( ibf );
        Assert( pbf->fQuiesced );
        Assert( !pbf->fAvailable );
    }


    for ( ; ibf >= 0; ibf-- )
    {
        PBF pbf = PbfBFICacheIbf( ibf );

        if ( pbf->fQuiesced )
        {
            Assert( !pbf->fAvailable );
            Assert( !pbf->fInOB0OL && pbf->ob0ic.FUninitialized() );
            g_bfquiesced.Remove( pbf );
        }
        else
        {
            Assert( pbf->fAvailable );
        }
    }

    Assert( g_bfquiesced.FEmpty() );
#endif


    CallS( ErrBFICacheISetSize( 0 ) );
    Assert( cbfCacheAddressable == 0 );
    cbfCacheSize = 0;
    g_bfquiesced.Empty();


    BFICacheINotifyCacheSizeChanges( cbfCacheAddressableInitial, cbfCacheSizeInitial, cbfCacheAddressable, cbfCacheSize );
}


void BFICacheINotifyCacheSizeChanges(
    const LONG_PTR cbfCacheAddressableInitial,
    const LONG_PTR cbfCacheSizeInitial,
    const LONG_PTR cbfCacheAddressableFinal,
    const LONG_PTR cbfCacheSizeFinal )
{
    Assert( g_critCacheSizeResize.FOwner() );

    if ( cbfCacheAddressableFinal != cbfCacheAddressableInitial )
    {

        CallS( CPAGE::ErrSetPageHintCacheSize( cbfCacheAddressableFinal * sizeof( DWORD_PTR ) ) );
    }

    if ( ( cbfCacheSizeFinal != cbfCacheSizeInitial ) && ( cbfCacheSizeFinal != 0 ) )
    {
        (void)ErrBFIMaintCacheStatsRequest( bfmcsrtForce );
    }

    ETCacheResize( (__int64)cbfCacheAddressableInitial,
            (__int64)cbfCacheSizeInitial,
            (__int64)cbfCacheAddressableFinal,
            (__int64)cbfCacheSizeFinal );
}

void BFIReportCacheStatisticsChanges(
    __inout BFCacheStatsChanges* const pstatsBFCacheResidency,
    __in const __int64 ftNow,
    __in const INT cbfCacheResidentCurrent,
    __in const INT cbfCacheCurrent,
    __in const __int64 cbCommittedCacheSize = 0,
    __in const __int64 cbCommitCacheTarget = 0,
    __in const __int64 cbTotalPhysicalMemory = 0 );

void BFIReportCacheStatisticsChanges(
    __inout BFCacheStatsChanges* const pstatsBFCacheResidency,
    __in const __int64 ftNow,
    __in const INT cbfCacheResidentCurrent,
    __in const INT cbfCacheCurrent,
    __in const __int64 cbCommittedCacheSize,
    __in const __int64 cbCommitCacheTarget,
    __in const __int64 cbTotalPhysicalMemory )
{
    Assert ( cbfCacheCurrent >= 0 );

    if ( cbfCacheCurrent < 3840 )
    {
        return;
    }

    if ( pstatsBFCacheResidency->ftResidentLastEvent == 0 )
    {
        pstatsBFCacheResidency->ftResidentLastEvent = ftNow;
        pstatsBFCacheResidency->cbfResidentLast = cbfCacheCurrent;
        pstatsBFCacheResidency->cbfCacheLast = cbfCacheCurrent;
    }

    pstatsBFCacheResidency->eResidentCurrentEventType = eResidentCacheStatusNoChange;
    pstatsBFCacheResidency->csecLastEventDelta = -1;

    eResidentCacheStatusChange eStatus = eResidentCacheStatusNoChange;

    const INT pctCacheResidentLowThreshold = 80;
    const INT pctCacheResidentDropThreshold = 30;
    
    const INT pctCacheResident = (INT)( ( (__int64)cbfCacheResidentCurrent * 100 ) / cbfCacheCurrent );
    const INT pctCacheResidentLast = (INT)( ( (__int64)pstatsBFCacheResidency->cbfResidentLast * 100 ) / pstatsBFCacheResidency->cbfCacheLast );
    const INT pctCacheResidentDelta = pctCacheResident - pctCacheResidentLast;


    if ( ( pctCacheResident < pctCacheResidentLowThreshold ) &&
            ( ( pctCacheResidentLast >= pctCacheResidentLowThreshold ) || ( -pctCacheResidentDelta > pctCacheResidentDropThreshold ) ) )
    {
        eStatus = eResidentCacheStatusDrop;
    }
    else
    {
        const INT pctCacheResidentNormalThreshold = 85;
        C_ASSERT ( pctCacheResidentNormalThreshold > pctCacheResidentLowThreshold );

        if ( ( pctCacheResidentLast < pctCacheResidentNormalThreshold ) && ( pctCacheResident >= pctCacheResidentNormalThreshold ) )
        {
            eStatus = eResidentCacheStatusRestore;
        }
    }

    pstatsBFCacheResidency->eResidentCurrentEventType = eStatus;

    if ( eStatus != eResidentCacheStatusNoChange )
    {
        WCHAR wszPercentResidentLast[16];
        WCHAR wszResidentBuffersLast[16];
        WCHAR wszTotalBuffersLast[16];
        OSStrCbFormatW( wszPercentResidentLast, sizeof(wszPercentResidentLast), L"%d", pctCacheResidentLast );
        OSStrCbFormatW( wszResidentBuffersLast, sizeof(wszResidentBuffersLast), L"%d", pstatsBFCacheResidency->cbfResidentLast );
        OSStrCbFormatW( wszTotalBuffersLast, sizeof(wszTotalBuffersLast), L"%d", pstatsBFCacheResidency->cbfCacheLast );

        WCHAR wszSecsDelta[24];
        pstatsBFCacheResidency->csecLastEventDelta = UtilConvertFileTimeToSeconds( ftNow - pstatsBFCacheResidency->ftResidentLastEvent );
        OSStrCbFormatW( wszSecsDelta, sizeof(wszSecsDelta), L"%I64d", pstatsBFCacheResidency->csecLastEventDelta );

        WCHAR wszPercentResident[16];
        WCHAR wszResidentBuffers[16];
        WCHAR wszTotalBuffers[16];
        OSStrCbFormatW( wszPercentResident, sizeof(wszPercentResident), L"%d", pctCacheResident );
        OSStrCbFormatW( wszResidentBuffers, sizeof(wszResidentBuffers), L"%d", cbfCacheResidentCurrent );
        OSStrCbFormatW( wszTotalBuffers, sizeof(wszTotalBuffers), L"%d", cbfCacheCurrent );

        WCHAR wszCachePctOfTarget[70]; wszCachePctOfTarget[0] = L'\0';
        WCHAR wszCacheSizeVsTarget[70]; wszCacheSizeVsTarget[0] = L'\0';
        WCHAR wszRamSize[70]; wszRamSize[0] = L'\0';
        if ( cbCommitCacheTarget && cbTotalPhysicalMemory )
        {
            const INT pctCacheTarget = (INT)( ( (__int64)cbCommittedCacheSize * 100 ) / cbCommitCacheTarget );
            OSStrCbFormatW( wszCachePctOfTarget, sizeof(wszCachePctOfTarget), L"%d", pctCacheTarget );
            OSStrCbFormatW( wszCacheSizeVsTarget, sizeof(wszCacheSizeVsTarget),
                                L"%0.03f / %0.03f",
                                DblMBs( cbCommittedCacheSize ),
                                DblMBs( cbCommitCacheTarget ) );

            OSStrCbFormatW( wszRamSize, sizeof(wszRamSize),
                                L"%0.03f",
                                DblMBs( cbTotalPhysicalMemory ) );
        }

        const WCHAR* rgwsz [] =
        {
            wszPercentResidentLast,
            wszResidentBuffersLast,
            wszTotalBuffersLast,
            wszSecsDelta,
            wszPercentResident,
            wszResidentBuffers,
            wszTotalBuffers,
            wszCachePctOfTarget,
            wszCacheSizeVsTarget,
            wszRamSize
        };

        switch ( eStatus )
        {
            case eResidentCacheStatusDrop:
                UtilReportEvent(
                        eventWarning,
                        PERFORMANCE_CATEGORY,
                        RESIDENT_CACHE_HAS_FALLEN_TOO_FAR_ID,
                        _countof( rgwsz ), rgwsz );
                break;
            case eResidentCacheStatusRestore:
                UtilReportEvent(
                    eventInformation,
                    PERFORMANCE_CATEGORY,
                    RESIDENT_CACHE_IS_RESTORED_ID,
                    _countof( rgwsz ), rgwsz );
                break;
        }

        pstatsBFCacheResidency->ftResidentLastEvent = ftNow;
        pstatsBFCacheResidency->eResidentLastEventType = eStatus;
        pstatsBFCacheResidency->cbfResidentLast = cbfCacheResidentCurrent;
        pstatsBFCacheResidency->cbfCacheLast = cbfCacheCurrent;
    }
}

JETUNITTEST( BF, BFICacheUpdateStatisticsIReportStatsSimple )
{
    BFCacheStatsChanges statsBFCacheResidency;

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 20, 20 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 16, 20 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 100000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 94000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 88000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 100000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 79000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 83000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 84000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 83000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 70000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 49000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
    CHECK( statsBFCacheResidency.cbfResidentLast == 79000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 300000000LL, 48999, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );
}

JETUNITTEST( BF, BFICacheUpdateStatisticsIReportGreenStats )
{
    BFCacheStatsChanges statsBFCacheResidency;

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 81000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 80000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 79000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 0 );

    CHECK( statsBFCacheResidency.cbfResidentLast == 79000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 85000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusRestore );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );
    
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 95000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 85000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 300000000LL, 79000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );
    
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 300000000LL, 82000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
    
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 300000000LL, 84000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
    
    CHECK( statsBFCacheResidency.cbfResidentLast == 79000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 400000000LL, 85000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusRestore );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );
    
    CHECK( statsBFCacheResidency.cbfResidentLast == 85000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 500000000LL, 74000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );
}

JETUNITTEST( BF, BFICacheUpdateStatisticsIReportStatsShrinkingOk )
{
    BFCacheStatsChanges statsBFCacheResidency;

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 100000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 91000, 91000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 86000, 86200 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 50000, 50000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 100000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 35000, 50000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );
}

JETUNITTEST( BF, BFICacheUpdateStatisticsIReportStatsHittingZeroLogsOnce )
{
    BFCacheStatsChanges statsBFCacheResidency;

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 100000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 50000, 50000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 100000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 0, 50000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 0, 50000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
}

JETUNITTEST( BF, BFICacheUpdateStatisticsIReportStatsSevereResidencyLossEvents )
{
    BFCacheStatsChanges statsBFCacheResidency;

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 2300000, 2300000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 2300000, 2300000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 2300000, 2300000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 2300000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 2300000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 1100000 , 2300000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );
}

JETUNITTEST( BF, BFICacheUpdateStatisticsIReportStatsSevereResidencyLossCoincidentWithCacheShrink )
{
    BFCacheStatsChanges statsBFCacheResidency;

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 2300000, 2300000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 2300000, 2300000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 2300000, 2300000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 2300000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 2300000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 1100000 , 1500000  );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );
}

JETUNITTEST( BF, BFICacheUpdateStatisticsIReportStatsResidencyLossByGrowth )
{
    BFCacheStatsChanges statsBFCacheResidency;

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 100000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 100000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 100000, 150000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 100000, 250000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 100000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 150000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 300000000LL, 100000, 285714 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );
}

JETUNITTEST( BF, BFICacheUpdateStatisticsIReportStatsResidencyIncreaseByShrinkage )
{
    BFCacheStatsChanges statsBFCacheResidency;

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 100000, 200000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 0 );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 100000, 181000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 100000, 119047 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 100000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 200000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 100000, 117000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusRestore );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 100000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
}

JETUNITTEST( BF, BFICacheUpdateStatisticsIReportStatsResidencyLossAndRestoredWithShrink )
{
    BFCacheStatsChanges statsBFCacheResidency;

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 100000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 100000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 50000, 75000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );

    CHECK( statsBFCacheResidency.cbfResidentLast == 50000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 75000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 300000000LL, 45000, 50000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusRestore );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );
}

JETUNITTEST( BF, BFICacheUpdateStatisticsIReportStatsResidencyIncreaseMultiple )
{
    BFCacheStatsChanges statsBFCacheResidency;

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 100000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 100000000LL, 80000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 100000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 79000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 70000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 68000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 200000000LL, 49000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 79000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 300000000LL, 48000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 300000000LL, 38000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 300000000LL, 28000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 300000000LL, 18000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 48000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 400000000LL, 17000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 400000000LL, 0, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 400000000LL, 60000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 400000000LL, 80000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 400000000LL, 84000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 17000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 500000000LL, 85000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusRestore );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 500000000LL, 100000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 500000000LL, 80000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 85000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 600000000LL, 79000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 600000000LL, 84000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 600000000LL, 49000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 79000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 700000000LL, 48000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );

    CHECK( statsBFCacheResidency.cbfResidentLast == 48000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 800000000LL, 85000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusRestore );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 800000000LL, 84000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 800000000LL, 100000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 800000000LL, 80000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 800000000LL, 100000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    CHECK( statsBFCacheResidency.cbfResidentLast == 85000 );
    CHECK( statsBFCacheResidency.cbfCacheLast == 100000 );
    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 900000000LL, 79000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusDrop );
    CHECK( statsBFCacheResidency.csecLastEventDelta == 10 );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 900000000LL, 84000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 900000000LL, 79000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );

    BFIReportCacheStatisticsChanges( &statsBFCacheResidency, 900000000LL, 84000, 100000 );
    CHECK( statsBFCacheResidency.eResidentCurrentEventType == eResidentCacheStatusNoChange );
}

static QWORD g_qwUnintendedResidentPagesLastPass = 0;
static QWORD g_cbfUnintendedResidentPagesLastPass = 0;
static QWORD g_cbfHighUnintendedResidentPagesLastPass = 0;
static QWORD g_qwUnintendedResidentPagesEver = 0;
static TICK g_tickLastUpdateStatistics = 0;


BOOL FBFITriggerCacheOverMemoryInDepthProtection(
    const __int64 cbTotalPhysicalMemory,
    const __int64 cbCommittedCacheSize )
{

    Expected( cbTotalPhysicalMemory > 0 );
    Expected( cbCommittedCacheSize >= 0 );

    const BOOL fOverCommitRAM = ( cbTotalPhysicalMemory == 0 ) || ( 100 * cbCommittedCacheSize / cbTotalPhysicalMemory >= pctCommitDefenseRAM );
    
    return fOverCommitRAM;
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( BF, TestFBFITriggerCacheOverMemoryInDepthProtection )
{
    const __int64 cbTotalPhysicalMemory = 32000000000i64;
    const __int64 cbCommittedCacheSizeOver = cbTotalPhysicalMemory * ( pctCommitDefenseRAM ) / 100;
    const __int64 cbCommittedCacheSizeOk = cbCommittedCacheSizeOver - 1;


    CHECK( cbTotalPhysicalMemory <= cbCommittedCacheSizeOver );
    CHECK( cbCommittedCacheSizeOk < cbCommittedCacheSizeOver );


    CHECK( !FBFITriggerCacheOverMemoryInDepthProtection( cbTotalPhysicalMemory, cbCommittedCacheSizeOk ) );
    CHECK( FBFITriggerCacheOverMemoryInDepthProtection( cbTotalPhysicalMemory, cbCommittedCacheSizeOver ) );
}

#endif


BOOL FBFICommittedCacheSizeOverTargetDefense(
    const __int64 cbCommittedCacheSize,
    const __int64 cbCommitCacheTarget )
{
    Expected( cbCommittedCacheSize >= 0 );
    Expected( cbCommitCacheTarget >= 0 );
    C_ASSERT( cbCommitDefenseMin > 0 );


    if ( cbCommitCacheTarget <= 0 )
    {
        Assert( cbCommitCacheTarget == 0 );
        return fFalse;
    }


    const __int64 cbCommitCacheTargetT = max( cbCommitCacheTarget, cbCommitDefenseMin );
    const BOOL fOverCommitTarget = ( 100 * cbCommittedCacheSize / cbCommitCacheTargetT >= pctCommitDefenseTarget );

    return fOverCommitTarget;
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( BF, TestFBFICommittedCacheSizeOverTargetDefense )
{
    const __int64 cbCommitCacheTargetNominal = cbCommitDefenseMin;
    const __int64 cbCommitCacheTargetSmall = cbCommitCacheTargetNominal / 2;
    const __int64 cbCommittedCacheSizeOver = cbCommitCacheTargetNominal * pctCommitDefenseTarget / 100;
    const __int64 cbCommittedCacheSizeOk = cbCommittedCacheSizeOver - 1;


    CHECK( cbCommitCacheTargetSmall < cbCommitCacheTargetNominal );
    CHECK( cbCommitCacheTargetNominal < cbCommittedCacheSizeOver );
    CHECK( cbCommittedCacheSizeOk < cbCommittedCacheSizeOver );
    CHECK( ( 100 * cbCommittedCacheSizeOk / cbCommitCacheTargetSmall ) > pctCommitDefenseTarget );


    CHECK( !FBFICommittedCacheSizeOverTargetDefense( cbCommittedCacheSizeOk, cbCommitCacheTargetNominal ) );
    CHECK( FBFICommittedCacheSizeOverTargetDefense( cbCommittedCacheSizeOver, cbCommitCacheTargetNominal ) );


    CHECK( !FBFICommittedCacheSizeOverTargetDefense( cbCommittedCacheSizeOk, cbCommitCacheTargetSmall ) );
    CHECK( FBFICommittedCacheSizeOverTargetDefense( cbCommittedCacheSizeOver, cbCommitCacheTargetSmall ) );


    CHECK( !FBFICommittedCacheSizeOverTargetDefense( cbCommittedCacheSizeOk, 0 ) );
    CHECK( !FBFICommittedCacheSizeOverTargetDefense( cbCommittedCacheSizeOver, 0 ) );


    CHECK( !FBFICommittedCacheSizeOverTargetDefense( 0, cbCommitCacheTargetNominal ) );
    CHECK( !FBFICommittedCacheSizeOverTargetDefense( 0, cbCommitCacheTargetSmall ) );
    CHECK( !FBFICommittedCacheSizeOverTargetDefense( 0, 0 ) );
}

#endif


BOOL FBFITriggerCacheOverTargetInDepthProtection(
    const __int64 cbCommittedCacheSize,
    const __int64 cbCommitCacheTarget,
    const TICK dtickCacheSizeDuration )
{

    Expected( dtickCacheSizeDuration >= 0 );

    const BOOL fOverCommitTarget = FBFICommittedCacheSizeOverTargetDefense( cbCommittedCacheSize, cbCommitCacheTarget );

    if ( !fOverCommitTarget )
    {
        return fFalse;
    }

    const BOOL fTriggerOverCommmitDefense = ( dtickCacheSizeDuration >= dtickCommitDefenseTarget );

    return fTriggerOverCommmitDefense;
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( BF, TestFBFITriggerCacheOverTargetInDepthProtection )
{
    const __int64 cbCommitCacheTargetNominal = cbCommitDefenseMin;
    const __int64 cbCommitCacheTargetSmall = cbCommitCacheTargetNominal / 2;
    const __int64 cbCommittedCacheSizeOver = cbCommitCacheTargetNominal * pctCommitDefenseTarget / 100;
    const __int64 cbCommittedCacheSizeOk = cbCommittedCacheSizeOver - 1;
    const TICK dtickCacheSizeDurationOver = dtickCommitDefenseTarget;
    const TICK dtickCacheSizeDurationOk = dtickCacheSizeDurationOver - 1;


    CHECK( cbCommitCacheTargetSmall < cbCommitCacheTargetNominal );
    CHECK( cbCommitCacheTargetNominal < cbCommittedCacheSizeOver );
    CHECK( cbCommittedCacheSizeOk < cbCommittedCacheSizeOver );
    CHECK( dtickCacheSizeDurationOk < dtickCacheSizeDurationOver );
    CHECK( ( 100 * cbCommittedCacheSizeOk / cbCommitCacheTargetSmall ) > pctCommitDefenseTarget );


    CHECK( !FBFITriggerCacheOverTargetInDepthProtection( cbCommittedCacheSizeOk, cbCommitCacheTargetNominal, dtickCacheSizeDurationOk ) );
    CHECK( !FBFITriggerCacheOverTargetInDepthProtection( cbCommittedCacheSizeOver, cbCommitCacheTargetNominal, dtickCacheSizeDurationOk ) );
    CHECK( !FBFITriggerCacheOverTargetInDepthProtection( cbCommittedCacheSizeOk, cbCommitCacheTargetNominal, dtickCacheSizeDurationOver ) );
    CHECK( FBFITriggerCacheOverTargetInDepthProtection( cbCommittedCacheSizeOver, cbCommitCacheTargetNominal, dtickCacheSizeDurationOver ) );


    CHECK( !FBFITriggerCacheOverTargetInDepthProtection( cbCommittedCacheSizeOk, cbCommitCacheTargetSmall, dtickCacheSizeDurationOk ) );
    CHECK( !FBFITriggerCacheOverTargetInDepthProtection( cbCommittedCacheSizeOver, cbCommitCacheTargetSmall, dtickCacheSizeDurationOk ) );
    CHECK( !FBFITriggerCacheOverTargetInDepthProtection( cbCommittedCacheSizeOk, cbCommitCacheTargetSmall, dtickCacheSizeDurationOver ) );
    CHECK( FBFITriggerCacheOverTargetInDepthProtection( cbCommittedCacheSizeOver, cbCommitCacheTargetSmall, dtickCacheSizeDurationOver ) );


    CHECK( !FBFITriggerCacheOverTargetInDepthProtection( cbCommittedCacheSizeOk, 0, dtickCacheSizeDurationOk ) );
    CHECK( !FBFITriggerCacheOverTargetInDepthProtection( cbCommittedCacheSizeOver, 0, dtickCacheSizeDurationOk ) );
    CHECK( !FBFITriggerCacheOverTargetInDepthProtection( cbCommittedCacheSizeOk, 0, dtickCacheSizeDurationOver ) );
    CHECK( !FBFITriggerCacheOverTargetInDepthProtection( cbCommittedCacheSizeOver, 0, dtickCacheSizeDurationOver ) );


    CHECK( !FBFITriggerCacheOverTargetInDepthProtection( 0, cbCommitCacheTargetNominal, dtickCacheSizeDurationOver ) );
    CHECK( !FBFITriggerCacheOverTargetInDepthProtection( 0, cbCommitCacheTargetSmall, dtickCacheSizeDurationOver ) );
    CHECK( !FBFITriggerCacheOverTargetInDepthProtection( 0, 0, dtickCacheSizeDurationOver ) );
}

#endif


ERR ErrBFICacheUpdateStatistics()
{
    ERR         err                     = JET_errSuccess;
    DWORD       cUpdate                 = 0;
    IBitmapAPI* pbmapi                  = NULL;
    size_t      cmmpgResident           = 0;
    size_t      cmmpgNonResident        = 0;
    size_t      cmmpgNewNonResident     = 0;
    LONG        ibfMax                  = 0;
    LONG        ibfLastTrimmed          = -1;
    LONG        cbfCacheAddressableAfter        = -1;
    LONG        cbfInitAfter            = -1;
    LONG        cbfNewlyCommittedAfter  = -1;

    const TICK tickStartUpdateStats = TickOSTimeCurrent();
    Unused( tickStartUpdateStats );


    const __int64 cbTotalPhysicalMemory = (__int64)OSMemoryQuotaTotal();
    const __int64 cbCommittedCacheSize = CbBFICacheIMemoryCommitted();
    const __int64 cbCommitCacheTarget = g_cacheram.GetOptimalResourcePoolSize();
    const TICK dtickCacheSizeDuration = DtickBFIMaintCacheSizeDuration();


    if ( FBFICommittedCacheSizeOverTargetDefense( cbCommittedCacheSize, cbCommitCacheTarget ) )
    {
        EnforceSz( !FBFITriggerCacheOverMemoryInDepthProtection( cbTotalPhysicalMemory, cbCommittedCacheSize ), "CacheWayOverPhysMem" );
    }

#ifndef ESENT


    if ( FBFITriggerCacheOverTargetInDepthProtection( cbCommittedCacheSize, cbCommitCacheTarget, dtickCacheSizeDuration ) )
    {
        FireWall( "CacheOverTargetTooLong" );
        AssertSz( fFalse, "Defense-in-depth protection for committed cache (%I64d) vs. target size (%I64d).", cbCommittedCacheSize, cbCommitCacheTarget );
    }
#endif


    g_qwUnintendedResidentPagesLastPass = 0;
    g_cbfUnintendedResidentPagesLastPass = 0;
    g_cbfHighUnintendedResidentPagesLastPass = 0;

    const LONG cbfCacheAddressableBefore = (LONG)cbfCacheAddressable;
    const LONG cbfNewlyCommittedBefore = (LONG)g_cbfNewlyCommitted;
    const LONG cbfInitBefore = *(volatile LONG *)(&cbfInit);

    OnDebug( CStats * phistoIcbBuffer = new CPerfectHistogramStats() );


    const size_t cbVMPage   = OSMemoryPageCommitGranularity();
    const size_t cbfVMPage  = max( 1, cbVMPage / g_rgcbPageSize[g_icbCacheMax] );
    const size_t cmmpgBF    = max( 1, g_rgcbPageSize[g_icbCacheMax] / cbVMPage );

    const LONG_PTR cpgMax = min( cbfInitBefore, g_cpgChunk );


    size_t cbMax = roundup( cpgMax * g_rgcbPageSize[g_icbCacheMax], OSMemoryPageCommitGranularity() );

    Call( ErrOSMemoryPageResidenceMapScanStart( cbMax, &cUpdate ) );


    if ( cUpdate != g_cResidenceCalc )
    {

        g_cResidenceCalc = cUpdate;

        size_t cbitVMPage;
        for ( cbitVMPage = 0; (size_t)1 << cbitVMPage != cbVMPage; cbitVMPage++ );
        Expected( cbitVMPage == 12 || cbitVMPage == 13 );

        IBF ibfLastUnintendedResident = 0;


        for ( size_t iCacheChunk = 0, ibf = 0; ibf < (size_t)cbfInitBefore && iCacheChunk < cCacheChunkMax; iCacheChunk++ )
        {

            Assert( cpgMax == g_cpgChunk || iCacheChunk == 0 );

            cbMax = roundup( cpgMax * g_rgcbPageSize[g_icbCacheMax], OSMemoryPageCommitGranularity() );

            Call( ErrOSMemoryPageResidenceMapRetrieve( g_rgpvChunk[ iCacheChunk ], cbMax, &pbmapi ) );


            size_t immpgBF = 0;
            for ( size_t iVMPage = 0; ibf < (size_t)cbfInitBefore && iVMPage < ( cbMax / OSMemoryPageCommitGranularity() ); iVMPage++ )
            {
                BOOL            fResident   = fTrue;
                IBitmapAPI::ERR errBM       = IBitmapAPI::ERR::errSuccess;


                errBM = pbmapi->ErrGet( iVMPage, &fResident );
                if ( errBM != IBitmapAPI::ERR::errSuccess )
                {
                    fResident = fTrue;
                }


                if ( !fResident )
                {
                    cmmpgNonResident++;


                    for ( IBF ibfT = ibf; ibfT < cbfInitBefore && ibfT < IBF( ibf + cbfVMPage ); ibfT++ )
                    {
                        const PBF pbf = PbfBFICacheIbf( ibfT );
                        if ( cmmpgBF >= 2 )
                        {
                            Assert( ibfT == (LONG_PTR)ibf );


                            if ( ( cbVMPage * ( immpgBF + 1 ) ) <= (size_t)g_rgcbPageSize[pbf->icbBuffer] )
                            {
                                
                                (void)BfrsBFIUpdateResidentState( pbf, bfrsNotResident, bfrsResident );
                            }
                        }
                        else
                        {
                            if ( bfrsResident == BfrsBFIUpdateResidentState( pbf, bfrsNotResident, bfrsResident ) )
                            {
                                cmmpgNewNonResident++;
                            }
                        }
                    }


                    if ( (IBF)ibf != ibfLastTrimmed )
                    {
                        PERFOpt( AtomicIncrement( (LONG*)&g_cbfTrimmed ) );
                    }
                    ibfLastTrimmed = ibf;
                }
                else
                {
                    cmmpgResident++;

#ifndef RTM
                    if ( cmmpgBF >= 2 )
                    {
                        const PBF pbf = PbfBFICacheIbf( ibf );
                        const ICBPage icbBuffer = (ICBPage)pbf->icbBuffer;
#ifdef DEBUG
                        if ( phistoIcbBuffer )
                        {
                            phistoIcbBuffer->ErrAddSample( (SAMPLE) icbBuffer );
                        }
#endif
                        if ( ( cbVMPage * ( immpgBF + 1 ) ) > (size_t)g_rgcbPageSize[icbBuffer] )
                        {

                        
                            if ( ibf > (size_t)cbfCacheAddressable )
                            {
                                g_cbfHighUnintendedResidentPagesLastPass++;
                            }
                            else
                            {
                                g_qwUnintendedResidentPagesLastPass++;
                                g_qwUnintendedResidentPagesEver++;
                                if ( (IBF)ibf != ibfLastUnintendedResident )
                                {
                                    g_cbfUnintendedResidentPagesLastPass++;
                                    ibfLastUnintendedResident = (IBF)ibf;
                                }
                            }
                        }
                    }
#endif
                }


                if ( ++immpgBF >= cmmpgBF )
                {
                    immpgBF = 0;
                    ibf += cbfVMPage;
                    ibfMax = ibf;
                }
            }
        }

    }

    g_tickLastUpdateStatistics = TickOSTimeCurrent();

    cbfCacheAddressableAfter = (LONG)cbfCacheAddressable;
    cbfNewlyCommittedAfter = (LONG)g_cbfNewlyCommitted;
    cbfInitAfter = (LONG)cbfInit;

HandleError:

    OSMemoryPageResidenceMapScanStop();

    const TICK tickFinishUpdateStats = TickOSTimeCurrent();

    Unused( tickFinishUpdateStats );

    Assert( g_qwUnintendedResidentPagesLastPass < 10000 );

    OSTrace( JET_tracetagBufferManagerMaintTasks, OSFormat( "ErrBFICacheUpdateStatistics() -> %d completed in %d ms, g_qwUnintendedResidentPagesLastPass  = %d, b: %d, %d, %d, a: %d, %d, %d ... res-stats: %d, %d, %d ... %d",
            err,
            DtickDelta( tickStartUpdateStats, g_tickLastUpdateStatistics ),
            (ULONG)g_qwUnintendedResidentPagesLastPass,
            cbfCacheAddressableBefore, cbfNewlyCommittedBefore, cbfInitBefore,
            cbfCacheAddressableAfter, cbfNewlyCommittedAfter, cbfInitAfter,
            (ULONG)cmmpgResident, (ULONG)cmmpgNonResident, (ULONG)cmmpgNewNonResident,
            ibfMax ) );

    BFIReportCacheStatisticsChanges(
        &g_statsBFCacheResidency,
        UtilGetCurrentFileTime(),
        (INT)g_cbfCacheResident,
        (INT)CbfBFICacheCommitted(),
        cbCommittedCacheSize,
        cbCommitCacheTarget,
        cbTotalPhysicalMemory );

    OnDebug( delete phistoIcbBuffer );

    return err;
}


LONG LBFICacheCleanPercentage( void )
{
    return ( LONG )( 100 * g_cbfCacheClean / max( 1, cbfCacheSize ) );
}


LONG LBFICacheSizePercentage( void )
{
    return ( LONG )( 100 * cbfCacheSize / max( 1, cbfCacheTarget ) );
}


LONG LBFICachePinnedPercentage( void )
{
    BFSTAT bfStat = BFLogHistogram::Read();
    Assert( bfStat.m_cBFPin <= bfStat.m_cBFMod );

    return ( LONG )( 100 * bfStat.m_cBFPin / max( 1, cbfCacheSize ) );
}



INLINE LONG CbBFIBufferSize( const PBF pbf )
{
    Assert( pbf->sxwl.FLatched() );

    return g_rgcbPageSize[pbf->icbBuffer];
}


INLINE LONG CbBFIPageSize( const PBF pbf )
{
    Assert( pbf->sxwl.FLatched() );

    return g_rgcbPageSize[pbf->icbPage];
}

#ifdef DEBUG


INLINE BOOL FBFICacheValidPv( const void* const pv )
{
    if ( BoolParam( JET_paramEnableViewCache ) )
    {
        return pv != NULL;
    }
    else
    {
        return IpgBFICachePv( pv ) != ipgNil;
    }
}


INLINE BOOL FBFICacheValidPbf( const PBF pbf )
{
    return IbfBFICachePbf( pbf ) != ibfNil;
}

#endif


INLINE PBF PbfBFICacheIbf( const IBF ibf )
{
    return (    ibf == ibfNil ?
                    pbfNil :
                    g_rgpbfChunk[ ibf / g_cbfChunk ] + ibf % g_cbfChunk );
}


INLINE void* PvBFICacheIpg( const IPG ipg )
{
    if ( BoolParam( JET_paramEnableViewCache ) )
    {
        return NULL;
    }
    else
    {

        return (    ipg == ipgNil ?
                        NULL :
                        (BYTE*)g_rgpvChunk[ ipg / g_cpgChunk ] + ( ipg % g_cpgChunk ) * g_rgcbPageSize[g_icbCacheMax] );
    }
}


IBF IbfBFICachePbf( const PBF pbf )
{

    LONG_PTR ibfChunk;
    for ( ibfChunk = 0; ibfChunk < cCacheChunkMax; ibfChunk++ )
    {

        if (    g_rgpbfChunk[ ibfChunk ] &&
                g_rgpbfChunk[ ibfChunk ] <= pbf && pbf < g_rgpbfChunk[ ibfChunk ] + g_cbfChunk &&
                ( DWORD_PTR( pbf ) - DWORD_PTR( g_rgpbfChunk[ ibfChunk ] ) ) % sizeof( BF ) == 0 )
        {

            const IBF ibf = ibfChunk * g_cbfChunk + pbf - g_rgpbfChunk[ ibfChunk ];

            Assert( PbfBFICacheIbf( ibf ) == pbf );
            return ibf;
        }
    }


    return ibfNil;
}


IPG IpgBFICachePv( const void* const pv )
{
    Expected( !UlParam( JET_paramEnableViewCache ) );


    LONG_PTR ipgChunk;
    for ( ipgChunk = 0; ipgChunk < cCacheChunkMax; ipgChunk++ )
    {

        if (    g_rgpvChunk[ ipgChunk ] &&
                g_rgpvChunk[ ipgChunk ] <= pv &&
                pv < (BYTE*)g_rgpvChunk[ ipgChunk ] + g_cpgChunk * g_rgcbPageSize[g_icbCacheMax] &&
                ( DWORD_PTR( pv ) - DWORD_PTR( g_rgpvChunk[ ipgChunk ] ) ) % g_rgcbPageSize[g_icbCacheMax] == 0 )
        {

            const IPG ipg = ipgChunk * g_cpgChunk + ( (BYTE*)pv - (BYTE*)g_rgpvChunk[ ipgChunk ] ) / g_rgcbPageSize[g_icbCacheMax];

            Assert( PvBFICacheIpg( ipg ) == pv );
            return ipg;
        }
    }


    return ipgNil;
}


BOOL FBFIValidPvAllocType( const BF * const pbf )
{
    return ( pbf->bfat == bfatNone && pbf->pv == NULL ) ||
            ( pbf->bfat != bfatNone && pbf->pv != NULL );
}


#define FOpFI( ulID )           ( ( ErrFaultInjection( ulID ) < JET_errSuccess ) ? fFalse : fTrue )
#define AllocFI( ulID, func )       \
{                                                       \
    if ( ErrFaultInjection( ulID ) < JET_errSuccess )   \
    {                                                   \
        Alloc( NULL );                                  \
    }                                                   \
    else                                                \
    {                                                   \
        Alloc( func );                                  \
    }                                                   \
}

ERR ErrBFICacheISetDataSize( const LONG_PTR cpgCacheStart, const LONG_PTR cpgCacheNew )
{
    ERR err = JET_errSuccess;

    Assert( g_critCacheSizeResize.FOwner() );


    if ( BoolParam( JET_paramEnableViewCache ) )
    {
        return JET_errSuccess;
    }


    LONG_PTR cpgCacheCur = cpgCacheStart;


    const LONG_PTR ipgChunkStart    = cpgCacheStart ? ( cpgCacheStart - 1 ) / g_cpgChunk : -1;
    const LONG_PTR ipgChunkNew      = cpgCacheNew ? ( cpgCacheNew - 1 ) / g_cpgChunk : -1;


    if ( ipgChunkNew > ipgChunkStart )
    {

        if ( cpgCacheStart % g_cpgChunk )
        {

            const size_t ib = ( cpgCacheStart % g_cpgChunk ) * g_rgcbPageSize[g_icbCacheMax];
            const size_t cb = ( ( ipgChunkStart + 1 ) * g_cpgChunk - cpgCacheStart ) * g_rgcbPageSize[g_icbCacheMax];
            void* const pvStart = (BYTE*)g_rgpvChunk[ ipgChunkStart ] + ib;

            if ( !FOpFI( 33032 ) || !FOSMemoryPageCommit( pvStart, cb ) )
            {
                Call( ErrERRCheck( JET_errOutOfMemory ) );
            }
        }


        for ( LONG_PTR ipgChunkAlloc = ipgChunkStart + 1; ipgChunkAlloc <= ipgChunkNew; ipgChunkAlloc++ )
        {

            const size_t cbChunkAlloc = g_cpgChunk * g_rgcbPageSize[g_icbCacheMax];
            AllocFI( 49416, g_rgpvChunk[ ipgChunkAlloc ] = PvOSMemoryPageReserve( cbChunkAlloc, NULL ) );
            g_cbCacheReservedSize += (ULONG_PTR)cbChunkAlloc;
            Assert( (LONG_PTR)g_cbCacheReservedSize >= (LONG_PTR)cbChunkAlloc );


            cpgCacheCur = min( cpgCacheNew, ( ipgChunkAlloc + 1 ) * g_cpgChunk );


            const size_t ib = 0;
            const size_t cb = min( g_cpgChunk, cpgCacheNew - ipgChunkAlloc * g_cpgChunk ) * g_rgcbPageSize[g_icbCacheMax];
            void* const pvStart = (BYTE*)g_rgpvChunk[ ipgChunkAlloc ] + ib;

            if ( !FOpFI( 48904 ) || !FOSMemoryPageCommit( pvStart, cb ) )
            {
                Call( ErrERRCheck( JET_errOutOfMemory ) );
            }
        }
    }


    else if ( ipgChunkNew < ipgChunkStart )
    {

        for ( LONG_PTR ipgChunkFree = ipgChunkNew + 1; ipgChunkFree <= ipgChunkStart; ipgChunkFree++ )
        {
            void* const pvChunkFree = g_rgpvChunk[ ipgChunkFree ];
            
            g_rgpvChunk[ ipgChunkFree ] = NULL;
            const size_t cbChunkFree = g_cpgChunk * g_rgcbPageSize[g_icbCacheMax];
            OSMemoryPageDecommit( pvChunkFree, cbChunkFree );
            OSMemoryPageFree( pvChunkFree );

            g_cbCacheReservedSize -= (ULONG_PTR)cbChunkFree;
            Assert( (LONG_PTR)g_cbCacheReservedSize >= 0 );
        }


        const LONG_PTR cpgPerPage = max( 1, OSMemoryPageCommitGranularity() / g_rgcbPageSize[g_icbCacheMax] );
        Expected( cpgPerPage == 1 );

        LONG_PTR cpgCommit = cpgCacheNew - ipgChunkNew * g_cpgChunk + cpgPerPage - 1;
        cpgCommit -= cpgCommit % cpgPerPage;

        LONG_PTR cpgCommitMax = g_cpgChunk + cpgPerPage - 1;
        cpgCommitMax -= cpgCommitMax % cpgPerPage;

        const LONG_PTR cpgReset = cpgCommitMax - cpgCommit;
        if ( cpgReset )
        {
            OSMemoryPageReset(  (BYTE*)g_rgpvChunk[ ipgChunkNew ] + cpgCommit * g_rgcbPageSize[g_icbCacheMax],
                                cpgReset * g_rgcbPageSize[g_icbCacheMax],
                                fTrue );
        }
    }


    else
    {

        if ( cpgCacheNew > cpgCacheStart )
        {

            const size_t ib = ( cpgCacheStart % g_cpgChunk ) * g_rgcbPageSize[g_icbCacheMax];
            const size_t cb = ( cpgCacheNew - cpgCacheStart ) * g_rgcbPageSize[g_icbCacheMax];
            void* const pvStart = (BYTE*)g_rgpvChunk[ ipgChunkStart ] + ib;

            if ( !FOpFI( 65288 ) || !FOSMemoryPageCommit( pvStart, cb ) )
            {
                Call( ErrERRCheck( JET_errOutOfMemory ) );
            }
        }


        else if ( cpgCacheNew < cpgCacheStart )
        {

            const LONG_PTR cpgPerPage = max( 1, OSMemoryPageCommitGranularity() / g_rgcbPageSize[g_icbCacheMax] );

            LONG_PTR cpgCommit = cpgCacheNew - ipgChunkNew * g_cpgChunk + cpgPerPage - 1;
            cpgCommit -= cpgCommit % cpgPerPage;

            LONG_PTR cpgCommitMax = g_cpgChunk + cpgPerPage - 1;
            cpgCommitMax -= cpgCommitMax % cpgPerPage;

            const LONG_PTR cpgReset = cpgCommitMax - cpgCommit;
            const size_t ib = cpgCommit * g_rgcbPageSize[g_icbCacheMax];
            const size_t cb = cpgReset * g_rgcbPageSize[g_icbCacheMax];

            if ( cpgReset )
            {
                OSMemoryPageReset(  (BYTE*)g_rgpvChunk[ ipgChunkNew ] + ib,
                                    cb,
                                    fTrue );
            }
        }
    }

    return JET_errSuccess;


HandleError:
    Assert( cpgCacheCur >= cpgCacheStart );
    CallS( ErrBFICacheISetDataSize( cpgCacheCur, cpgCacheStart ) );
    return err;
}

ERR ErrBFICacheISetStatusSize( const LONG_PTR cbfCacheStart, const LONG_PTR cbfCacheNew )
{
    ERR err = JET_errSuccess;

    Assert( g_critCacheSizeResize.FOwner() );


    LONG_PTR cbfCacheCur = cbfCacheStart;


    const LONG_PTR ibfChunkStart    = cbfCacheStart ? ( cbfCacheStart - 1 ) / g_cbfChunk : -1;
    const LONG_PTR ibfChunkNew      = cbfCacheNew ? ( cbfCacheNew - 1 ) / g_cbfChunk : -1;


    if ( ibfChunkNew > ibfChunkStart )
    {

        if ( cbfCacheStart % g_cbfChunk )
        {

            const size_t ib = ( cbfCacheStart % g_cbfChunk ) * sizeof( BF );
            const size_t cb = ( ( ibfChunkStart + 1 ) * g_cbfChunk - cbfCacheStart ) * sizeof( BF );

            if ( !FOpFI( 40712 ) || !FOSMemoryPageCommit( (BYTE*)g_rgpbfChunk[ ibfChunkStart ] + ib, cb ) )
            {
                Call( ErrERRCheck( JET_errOutOfMemory ) );
            }
        }


        for ( LONG_PTR ibfChunkAlloc = ibfChunkStart + 1; ibfChunkAlloc <= ibfChunkNew; ibfChunkAlloc++ )
        {

            AllocFI( 57096, g_rgpbfChunk[ ibfChunkAlloc ] = (PBF)PvOSMemoryPageReserve( g_cbfChunk * sizeof( BF ), NULL ) );


            cbfCacheCur = min( cbfCacheNew, ( ibfChunkAlloc + 1 ) * g_cbfChunk );


            const size_t ib = 0;
            const size_t cb = min( g_cbfChunk, cbfCacheNew - ibfChunkAlloc * g_cbfChunk ) * sizeof( BF );

            if ( !FOpFI( 44808 ) || !FOSMemoryPageCommit( (BYTE*)g_rgpbfChunk[ ibfChunkAlloc ] + ib, cb ) )
            {
                Call( ErrERRCheck( JET_errOutOfMemory ) );
            }
        }
    }


    else if ( ibfChunkNew < ibfChunkStart )
    {

        for ( LONG_PTR ibfChunkFree = ibfChunkNew + 1; ibfChunkFree <= ibfChunkStart; ibfChunkFree++ )
        {
            const PBF pbfChunkFree = g_rgpbfChunk[ ibfChunkFree ];
            g_rgpbfChunk[ ibfChunkFree ] = NULL;
            OSMemoryPageDecommit( pbfChunkFree, g_cbfChunk * sizeof( BF ) );
            OSMemoryPageFree( pbfChunkFree );
        }


        const LONG_PTR cbfPerPage = max( 1, OSMemoryPageCommitGranularity() / sizeof( BF ) );

        LONG_PTR cbfCommit = cbfCacheNew - ibfChunkNew * g_cbfChunk + cbfPerPage - 1;
        cbfCommit -= cbfCommit % cbfPerPage;

        LONG_PTR cbfCommitMax = g_cbfChunk + cbfPerPage - 1;
        cbfCommitMax -= cbfCommitMax % cbfPerPage;

        const LONG_PTR cbfReset = cbfCommitMax - cbfCommit;
        if ( cbfReset )
        {
            OSMemoryPageReset(  g_rgpbfChunk[ ibfChunkNew ] + cbfCommit,
                                cbfReset * sizeof( BF ),
                                fTrue );
        }
    }


    else
    {

        if ( cbfCacheNew > cbfCacheStart )
        {

            const size_t ib = ( cbfCacheStart % g_cbfChunk ) * sizeof( BF );
            const size_t cb = ( cbfCacheNew - cbfCacheStart ) * sizeof( BF );

            if ( !FOpFI( 61192 ) || !FOSMemoryPageCommit( (BYTE*)g_rgpbfChunk[ ibfChunkStart ] + ib, cb ) )
            {
                Call( ErrERRCheck( JET_errOutOfMemory ) );
            }
        }


        else if ( cbfCacheNew < cbfCacheStart )
        {

            const LONG_PTR cbfPerPage = max( 1, OSMemoryPageCommitGranularity() / sizeof( BF ) );

            LONG_PTR cbfCommit = cbfCacheNew - ibfChunkNew * g_cbfChunk + cbfPerPage - 1;
            cbfCommit -= cbfCommit % cbfPerPage;

            LONG_PTR cbfCommitMax = g_cbfChunk + cbfPerPage - 1;
            cbfCommitMax -= cbfCommitMax % cbfPerPage;

            const LONG_PTR cbfReset = cbfCommitMax - cbfCommit;
            if ( cbfReset )
            {
                OSMemoryPageReset(  g_rgpbfChunk[ ibfChunkNew ] + cbfCommit,
                                    cbfReset * sizeof( BF ),
                                    fTrue );
            }
        }
    }

    return JET_errSuccess;


HandleError:
    CallS( ErrBFICacheISetStatusSize( cbfCacheCur, cbfCacheStart ) );
    return err;
}


ERR ErrBFICacheISetSize( const LONG_PTR cbfCacheAddressableNew )
{
    ERR err = JET_errSuccess;

    Assert( g_critCacheSizeResize.FOwner() );


    const LONG_PTR cbfCacheAddressableStart = cbfCacheAddressable;

    
    if ( cbfCacheAddressableStart < 0 )
    {
        AssertSz( fFalse, "At some point we had a negative cache size!!!?" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( cbfCacheAddressableNew < 0 )
    {
        AssertSz( fFalse, "Someone is trying to set a negative cache size!!!?" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    AssertRTL( cbfCacheAddressableNew != 0 || !g_fBFInitialized  );


    Call( ErrBFICacheISetDataSize( cbfCacheAddressableStart, cbfCacheAddressableNew ) );


    if ( cbfCacheAddressableNew > cbfCacheAddressableStart )
    {
#ifdef DEBUG

        for ( LONG_PTR ibfInit = cbfCacheAddressableStart; ibfInit < cbfInit; ibfInit++ )
        {
            PBF pbf = PbfBFICacheIbf( ibfInit );
        
            Assert( pbf->fQuiesced );
        }
#endif


        if ( cbfCacheAddressableNew > cbfInit )
        {

            if ( ( err = ErrBFICacheISetStatusSize( cbfInit, cbfCacheAddressableNew ) ) < JET_errSuccess )
            {
                CallS( ErrBFICacheISetDataSize( cbfCacheAddressableNew, cbfCacheAddressableStart ) );
                return err;
            }


            for ( LONG_PTR ibfInit = cbfInit; ibfInit < cbfCacheAddressableNew; ibfInit++ )
            {
                PBF pbf = PbfBFICacheIbf( ibfInit );


                new( pbf ) BF;

                CSXWLatch::ERR errSXWL = pbf->sxwl.ErrTryAcquireWriteLatch();
                Assert( errSXWL == CSXWLatch::ERR::errSuccess );
                pbf->sxwl.ReleaseOwnership( bfltWrite );


                cbfInit = ibfInit + 1;
            }
        }


        for ( LONG_PTR ibfInit = cbfCacheAddressableStart; ibfInit < cbfCacheAddressableNew; ibfInit++ )
        {
            PBF pbf = PbfBFICacheIbf( ibfInit );


            pbf->pv = PvBFICacheIpg( ibfInit );


            pbf->fNewlyEvicted = fFalse;
            pbf->icbBuffer = g_icbCacheMax;
            Assert( icbPageInvalid == pbf->icbPage );
            const BFResidenceState bfrsOld = BfrsBFIUpdateResidentState( pbf, bfrsNewlyCommitted );
            Expected( bfrsOld == bfrsNotCommitted );


            if ( !UlParam( JET_paramEnableViewCache ) )
            {
                pbf->bfat = bfatFracCommit;
                Assert( g_icbCacheMax == pbf->icbBuffer );
                OnDebug( const LONG_PTR cbCacheCommittedSizeInitial = (LONG_PTR)) AtomicExchangeAddPointer( (void**)&g_cbCacheCommittedSize, (void*)(ULONG_PTR)g_rgcbPageSize[pbf->icbBuffer] );
                Assert( cbCacheCommittedSizeInitial >= 0 );
            }
            Assert( ( pbf->bfat == bfatFracCommit && pbf->pv != NULL ) ||
                    ( pbf->bfat == bfatNone && pbf->pv == NULL ) );
            Assert( FBFIValidPvAllocType( pbf ) );


            pbf->sxwl.ClaimOwnership( bfltWrite );
            BFIFreePage( pbf, fFalse );


            Assert( cbfCacheAddressable == ibfInit );

            const ULONG_PTR cbfCacheAddressableNext = ibfInit + 1;

            Enforce( ( cbfCacheAddressableNext > 0 ) || ( cbfCacheAddressableNext == 0 && !g_fBFInitialized ) );
            cbfCacheAddressable = cbfCacheAddressableNext;
        }
    }


    else if ( cbfCacheAddressableNew < cbfCacheAddressableStart )
    {

        for ( LONG_PTR ibfTerm = cbfCacheAddressableStart - 1; ibfTerm >= cbfCacheAddressableNew; ibfTerm-- )
        {
            PBF pbf = PbfBFICacheIbf( ibfTerm );


            Assert( cbfCacheAddressable == ibfTerm + 1 );

            Enforce( ( ibfTerm > 0 ) || ( ibfTerm == 0 && !g_fBFInitialized ) );
            cbfCacheAddressable = ibfTerm;


            const BFResidenceState bfrsOld = BfrsBFIUpdateResidentState( pbf, bfrsNotCommitted );
            Expected( bfrsOld != bfrsNotCommitted || ( pbf->fQuiesced || pbf->fAvailable ) );

            if ( pbf->bfat == bfatFracCommit )
            {
                OnDebug( const LONG_PTR cbCacheCommittedSizeInitial = (LONG_PTR)) AtomicExchangeAddPointer( (void**)&g_cbCacheCommittedSize, (void*)( -( (LONG_PTR)g_rgcbPageSize[pbf->icbBuffer] ) ) );
                Assert( cbCacheCommittedSizeInitial >= g_rgcbPageSize[pbf->icbBuffer] );
            }

            pbf->fNewlyEvicted = fFalse;


            pbf->pv = NULL;
            pbf->bfat = bfatNone;

            Assert( FBFIValidPvAllocType( pbf ) );
        }


        if ( 0 == cbfCacheAddressableNew )
        {

            const LONG_PTR cbfTerm = cbfInit;
            for ( LONG_PTR ibfTerm = cbfTerm - 1; ibfTerm >= 0; ibfTerm-- )
            {
                PBF pbf = PbfBFICacheIbf( ibfTerm );


                cbfInit = ibfTerm;


                Expected( pbf->fQuiesced || pbf->fAvailable );


                if ( pbf->fQuiesced || pbf->fAvailable )
                {
                    pbf->sxwl.ClaimOwnership( bfltWrite );
                    pbf->sxwl.ReleaseWriteLatch();
                }

                pbf->~BF();
            }


            CallS( ErrBFICacheISetStatusSize( cbfTerm, 0 ) );
        };
    }

HandleError:
    
    return err;
}


LOCAL BOOL FBFICacheSizeFixed()
{
    return ( UlParam( JET_paramCacheSizeMin ) == UlParam( JET_paramCacheSizeMax ) ||
             g_cbfCacheUserOverride != 0 );
}

LOCAL BOOL FBFICacheApproximatelyEqual( const ULONG_PTR cbfTarget, const ULONG_PTR cbfCurrent )
{
    if ( cbfCurrent > 0 )
    {
        
        if ( absdiff( cbfTarget, cbfCurrent ) == 1 )
        {
            return fTrue;
        }

        
        const double fracCbfCache = (double)absdiff( cbfTarget, cbfCurrent ) / (double)cbfCurrent;

        if ( fracCbfCache <= fracMaintCacheSensitivity )
        {
            return fTrue;
        }

        return fFalse;
    }
    else
    {
        return ( cbfTarget == cbfCurrent );
    }
}


#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( BF, TestCbfSameWithinSensitivity )
{
    const ULONG_PTR cbfCurrent = 1000;
    const ULONG_PTR dcbfWithinRange = (ULONG_PTR)( cbfCurrent * fracMaintCacheSensitivity );
    const ULONG_PTR cbfCurrentSmall = 10;
    const ULONG_PTR dcbfWithinRangeSmall = (ULONG_PTR)( cbfCurrentSmall * fracMaintCacheSensitivity );
    const ULONG_PTR dcbfOutsideRangeSmall = 1;

    
    CHECK( dcbfWithinRange < cbfCurrent );
    CHECK( cbfCurrent != 0 );
    CHECK( dcbfWithinRange != 0 );
    CHECK( dcbfWithinRangeSmall < cbfCurrentSmall );
    CHECK( dcbfOutsideRangeSmall < cbfCurrentSmall );
    CHECK( dcbfWithinRangeSmall < dcbfOutsideRangeSmall );
    CHECK( cbfCurrentSmall != 0 );
    CHECK( dcbfOutsideRangeSmall != 0 );

    
    CHECK( FBFICacheApproximatelyEqual( 0, 0 ) );
    CHECK( !FBFICacheApproximatelyEqual( 1, 0 ) );
    CHECK( !FBFICacheApproximatelyEqual( 1000, 0 ) );

    
    CHECK( FBFICacheApproximatelyEqual( cbfCurrentSmall, cbfCurrentSmall ) );
    CHECK( FBFICacheApproximatelyEqual( cbfCurrentSmall - dcbfWithinRangeSmall, cbfCurrentSmall ) );
    CHECK( FBFICacheApproximatelyEqual( cbfCurrentSmall + dcbfWithinRangeSmall, cbfCurrentSmall ) );
    CHECK( FBFICacheApproximatelyEqual( cbfCurrentSmall - dcbfOutsideRangeSmall, cbfCurrentSmall ) );
    CHECK( FBFICacheApproximatelyEqual( cbfCurrentSmall + dcbfOutsideRangeSmall, cbfCurrentSmall ) );
    CHECK( !FBFICacheApproximatelyEqual( cbfCurrentSmall - dcbfOutsideRangeSmall - 1, cbfCurrentSmall ) );
    CHECK( !FBFICacheApproximatelyEqual( cbfCurrentSmall + dcbfOutsideRangeSmall + 1, cbfCurrentSmall ) );

    
    CHECK( FBFICacheApproximatelyEqual( cbfCurrent, cbfCurrent ) );
    CHECK( FBFICacheApproximatelyEqual( cbfCurrent - dcbfWithinRange, cbfCurrent ) );
    CHECK( FBFICacheApproximatelyEqual( cbfCurrent + dcbfWithinRange, cbfCurrent ) );
    CHECK( !FBFICacheApproximatelyEqual( cbfCurrent - dcbfWithinRange - 1, cbfCurrent ) );
    CHECK( !FBFICacheApproximatelyEqual( cbfCurrent + dcbfWithinRange + 1, cbfCurrent ) );
}

#endif


CCacheRAM g_cacheram;

inline CCacheRAM::CCacheRAM()
    :   CDBAResourceAllocationManager< cMaintCacheSamplesAvg >()
{
    Reset();
}

inline CCacheRAM::~CCacheRAM()
{
}

inline void CCacheRAM::Reset()
{
    m_cpgReclaimCurr = 0;
    m_cpgReclaimLast = 0;
    m_cpgReclaimNorm = 0;
    m_cpgEvictCurr = 0;
    m_cpgEvictLast = 0;
    m_cpgEvictNorm = 0;
    m_cpgPhysicalMemoryEvictedLast = 0;
    m_cbTotalPhysicalMemoryEvicted = 0;
    m_cbTotalResourcesEvictedLast = 0;
    m_cbTotalResourcesEvicted = 0;
    m_cbfCacheNewDiscrete = 0;
    m_cbOptimalResourcePoolSizeUsedLast = 0;
    m_dcbAdjustmentOverride = 0;
}

inline size_t CCacheRAM::TotalPhysicalMemory()
{

    return OSMemoryQuotaTotal();
}

inline size_t CCacheRAM::AvailablePhysicalMemory()
{

    const QWORD     cbAvail = OSMemoryAvailable();
    const size_t    cbTotal = OSMemoryQuotaTotal();
    const QWORD     cbPool  = this->TotalResources();

    size_t cbTemp = cbTotal > cbPool ? (size_t)( cbTotal - cbPool ) : 0;
    Assert( min( cbAvail, (QWORD)cbTemp ) == (size_t)min( cbAvail, cbTemp ) );
    return (size_t)min( cbAvail, cbTemp );
}

inline size_t CCacheRAM::TotalPhysicalMemoryEvicted()
{
    const size_t    cbQuotaTotal    = OSMemoryQuotaTotal();
    const QWORD     cbTotal         = OSMemoryTotal();


    const double fracCache = (double)CbBFICacheBufferSize() / (double)cbQuotaTotal;

    m_cpgReclaimCurr    = g_cpgReclaim;
    m_cpgReclaimNorm    += (DWORD)( ( m_cpgReclaimCurr - m_cpgReclaimLast ) / fracCache + 0.5 );
    m_cpgReclaimLast    = m_cpgReclaimCurr;


    const double fracQuota = (double)cbQuotaTotal / (double)cbTotal;

    m_cpgEvictCurr      = OSMemoryPageEvictionCount();
    m_cpgEvictNorm      += (DWORD)( ( m_cpgEvictCurr - m_cpgEvictLast ) * fracQuota + 0.5 );
    m_cpgEvictLast      = m_cpgEvictCurr;

    
    const DWORD cpgPhysicalMemoryEvicted = m_cpgEvictNorm + m_cpgReclaimNorm;
    m_cbTotalPhysicalMemoryEvicted += ( cpgPhysicalMemoryEvicted - m_cpgPhysicalMemoryEvictedLast ) * (size_t)OSMemoryPageCommitGranularity();
    m_cpgPhysicalMemoryEvictedLast = cpgPhysicalMemoryEvicted;


    return m_cbTotalPhysicalMemoryEvicted;
}

inline QWORD CCacheRAM::TotalResources()
{
    return (QWORD)CbBFICacheBufferSize();
}

inline QWORD CCacheRAM::TotalResourcesEvicted()
{
    const DWORD cbTotalResourcesEvicted = g_cbNewlyEvictedUsed;
    m_cbTotalResourcesEvicted += ( cbTotalResourcesEvicted - m_cbTotalResourcesEvictedLast );
    m_cbTotalResourcesEvictedLast = cbTotalResourcesEvicted;

    return m_cbTotalResourcesEvicted;
}

QWORD CbBFIMaintCacheSizeIAdjust_( const QWORD cbCurrrent, const double cbDelta )
{
    if ( cbDelta < 0.0 && (QWORD)-cbDelta > cbCurrrent )
    {
        return 0;
    }
    else if ( cbDelta > 0.0 && (QWORD)cbDelta > ( qwMax - cbCurrrent ) )
    {
        return qwMax;
    }
    else
    {
        return ( cbDelta > 0.0 ? cbCurrrent + (QWORD)cbDelta : cbCurrrent - (QWORD)-cbDelta ) ;
    }
}

inline QWORD CCacheRAM::GetOptimalResourcePoolSize()
{
    return m_cbOptimalResourcePoolSizeUsedLast;
}

void CCacheRAM::UpdateStatistics()
{
    Assert( g_critCacheSizeSetTarget.FOwner() );
    __super::UpdateStatistics();
}

void CCacheRAM::ConsumeResourceAdjustments( __out double * const pdcbTotalResource, __in const double cbResourceSize )
{
    Assert( g_critCacheSizeSetTarget.FOwner() );

    if ( m_dcbAdjustmentOverride != 0 )
    {
        const __int64 dcbAdjustmentOverride = (__int64)m_dcbAdjustmentOverride;
        *pdcbTotalResource = (double)roundup( dcbAdjustmentOverride, (__int64)cbResourceSize );
        m_dcbAdjustmentOverride = 0;
    }
    else
    {
        __super::ConsumeResourceAdjustments( pdcbTotalResource, cbResourceSize );
    }
}

void CCacheRAM::OverrideResourceAdjustments( double const dcbRource )
{
    Assert( g_critCacheSizeSetTarget.FOwner() );
    m_dcbAdjustmentOverride = dcbRource;
}

inline void CCacheRAM::SetOptimalResourcePoolSize()
{
    Assert( g_critCacheSizeSetTarget.FOwner() );


    double dcbTotalResource = 0.0;

    g_cacheram.ConsumeResourceAdjustments( &dcbTotalResource, (double)g_rgcbPageSize[g_icbCacheMax] );


    QWORD cbOptimalResources = CbBFIMaintCacheSizeIAdjust_( m_cbOptimalResourcePoolSizeUsedLast, dcbTotalResource );
    cbOptimalResources = min( cbOptimalResources, OSMemoryQuotaTotal() );
    cbOptimalResources = max( cbOptimalResources, (QWORD)( cbfCacheMinMin * g_rgcbPageSize[g_icbCacheMax] ) );

    Assert( cbOptimalResources > 0 );


    LONG_PTR cbfCacheNew = (LONG_PTR)( cbOptimalResources / g_rgcbPageSize[g_icbCacheMax] );
    Enforce( cbfCacheNew >= 0 );


    cbfCacheNew = g_cbfCacheUserOverride ? g_cbfCacheUserOverride : cbfCacheNew;


    const ULONG_PTR cbfCacheSizeMin = UlParam( JET_paramCacheSizeMin );
    const ULONG_PTR cbfCacheSizeMax = UlParam( JET_paramCacheSizeMax );

    cbfCacheNew = UlpBound( (ULONG_PTR)cbfCacheNew, (ULONG_PTR)cbfCacheSizeMin, (ULONG_PTR)cbfCacheSizeMax );
    Enforce( cbfCacheNew > 0 );


    m_cbOptimalResourcePoolSizeUsedLast = (QWORD)cbfCacheNew * (QWORD)g_rgcbPageSize[g_icbCacheMax];


    const LONG_PTR cbfCredit = CbfBFICredit();
    g_avgCbfCredit.AddSample( cbfCredit );
    cbfCacheNew += CbfBFIAveCredit();
    Enforce( cbfCacheNew > 0 );


    if ( !FBFICacheSizeFixed() )
    {
        if ( !FBFICacheApproximatelyEqual( (ULONG_PTR)cbfCacheNew, (ULONG_PTR)m_cbfCacheNewDiscrete ) )
        {
            
            m_cbfCacheNewDiscrete = cbfCacheNew;
        }
        else
        {
            
            cbfCacheNew = m_cbfCacheNewDiscrete;
        }

        Enforce( m_cbfCacheNewDiscrete > 0 );
        Enforce( cbfCacheNew > 0 );
    }


    const size_t    cbVATotal       = size_t( OSMemoryPageReserveTotal() * fracVAMax );
    const size_t    iViewCacheDilutionFactor = BoolParam( JET_paramEnableViewCache ) ? OSMemoryPageReserveGranularity() / g_rgcbPageSize[g_icbCacheMax] : 1;
    Enforce( iViewCacheDilutionFactor >= 1 );
    Enforce( iViewCacheDilutionFactor <= 32 );
    const size_t    cbVAAvailMin    = size_t( cbVATotal * fracVAAvailMin );
    const size_t    cbVAAvail       = OSMemoryPageReserveAvailable();
    const size_t    cbVACache       = ( ( cbfCacheAddressable + g_cpgChunk - 1 ) / g_cpgChunk ) * g_cpgChunk * g_rgcbPageSize[g_icbCacheMax];
    const size_t    cbVACacheMax    = ( min( cbVATotal, max( cbVAAvailMin, cbVACache + cbVAAvail ) ) - cbVAAvailMin ) / iViewCacheDilutionFactor;
    const size_t    cbfVACacheMax   = ( cbVACacheMax / g_rgcbPageSize[g_icbCacheMax] / g_cpgChunk ) * g_cpgChunk;

    cbfCacheNew = min( (size_t)cbfCacheNew, cbfVACacheMax );
    cbfCacheNew = max( cbfCacheNew, 1 );
    Enforce( cbfCacheNew > 0 );


    g_cbfCacheTargetOptimal = cbfCacheNew;
    BFIMaintAvailPoolUpdateThresholds( g_cbfCacheTargetOptimal );


    cbfCacheNew = UlpFunctionalMax( cbfCacheNew, cbfCacheDeadlock );
    Enforce( cbfCacheNew > 0 );


    cbfCacheNew = min( (ULONG_PTR)cbfCacheNew, (ULONG_PTR)cCacheChunkMax * (ULONG_PTR)min( g_cbfChunk, g_cpgChunk ) );
    Enforce( cbfCacheNew > 0 );


    if ( cbfCacheNew < cbfCacheMinMin )
    {
        cbfCacheNew = cbfCacheMinMin;
    }
    Enforce( cbfCacheNew >= cbfCacheMinMin );


    BFICacheISetTarget( cbfCacheNew );
}



CBFIssueList::
CBFIssueList()
    :   m_pbfilPrev( Ptls()->pbfil ),
        m_group( CMeteredSection::groupInvalidNil )
{
    Ptls()->pbfil = this;
    Expected( m_pbfilPrev > this || m_pbfilPrev == NULL );
}

CBFIssueList::
~CBFIssueList()
{
    Assert( FEmpty() );
    while ( CEntry* pentry = m_il.PrevMost() )
    {
        m_il.Remove( pentry );
        delete pentry;
    }
    if ( m_group >= 0 )
    {
        s_msSync.Leave( m_group );
    }
    Assert( Ptls()->pbfil == this );
    Ptls()->pbfil = m_pbfilPrev;
}

ERR CBFIssueList::
ErrPrepareWrite( const IFMP ifmp )
{
    return ErrPrepareOper( ifmp, CEntry::operWrite );
}

ERR CBFIssueList::
ErrPrepareLogWrite( const IFMP ifmp )
{
    return ErrPrepareOper( ifmp, CEntry::operLogWrite );
}

ERR CBFIssueList::
ErrPrepareRBSWrite( const IFMP ifmp )
{
    return ErrPrepareOper( ifmp, CEntry::operRBSWrite );
}

ERR CBFIssueList::
ErrIssue( const BOOL fSync )
{
    ERR     err         = JET_errSuccess;
    CEntry* pentry      = NULL;

    while ( pentry = m_il.PrevMost() )
    {
        switch ( pentry->Oper() )
        {
            case CEntry::operWrite:
                Call( g_rgfmp[ pentry->Ifmp() ].Pfapi()->ErrIOIssue() );
                break;

            case CEntry::operLogWrite:
                (void)ErrBFIWriteLog( pentry->Ifmp(), fSync );
                break;

            case CEntry::operRBSWrite:
                Assert( g_rgfmp[ pentry->Ifmp() ].FRBSOn() );
                (void)g_rgfmp[ pentry->Ifmp() ].PRBS()->ErrFlushAll();
                break;

            default:
                AssertSz( fFalse, "Unknown CBFIssueList pentry->Oper() = %d, unissued!\n", pentry->Oper() );
        }
        m_il.Remove( pentry );
        delete pentry;
    }

    if ( m_group >= 0 )
    {
        s_msSync.Leave( m_group );
        m_group = CMeteredSection::groupInvalidNil;
    }

    Assert( FEmpty() );


HandleError:
    return err;
}

VOID CBFIssueList::
AbandonLogOps()
{
    CEntry* pentry      = NULL;

    while ( pentry = m_il.PrevMost() )
    {
        switch ( pentry->Oper() )
        {
            case CEntry::operLogWrite:
            case CEntry::operRBSWrite:
                break;

            default:
                AssertSz( fFalse, "Unexpected CBFIssueList pentry->Oper() = %d, unissued!\n", pentry->Oper() );
        }
        m_il.Remove( pentry );
        delete pentry;
    }
    if ( m_group >= 0 )
    {
        s_msSync.Leave( m_group );
        m_group = CMeteredSection::groupInvalidNil;
    }
    Expected( FEmpty() );
}

ERR CBFIssueList::
ErrSync()
{
#ifdef DEBUG
    CBFIssueList * pbfil = Ptls()->pbfil;
    while( pbfil )
    {
        Assert( pbfil->FEmpty() );
        Assert( pbfil->m_group == CMeteredSection::groupInvalidNil );

        pbfil = pbfil->m_pbfilPrev;
    }
#endif
    s_critSync.Enter();
    s_msSync.Partition();
    s_critSync.Leave();
    return JET_errSuccess;
}

ERR CBFIssueList::
ErrPrepareOper( const IFMP ifmp, const CEntry::eOper oper )
{
    ERR     err         = JET_errSuccess;
    CEntry* pentry      = NULL;
    CEntry* pentryNew   = NULL;

    if ( m_group < 0 )
    {
        m_group = s_msSync.Enter();
    }

    for ( pentry = m_il.PrevMost(); pentry; pentry = m_il.Next( pentry ) )
    {
        if ( pentry->Ifmp() == ifmp && pentry->Oper() == oper )
        {
            pentry->IncRequests();
            Error( JET_errSuccess );
        }
    }

    Alloc( pentryNew = new CEntry( ifmp, oper ) );
    m_il.InsertAsNextMost( pentryNew );
    pentryNew = NULL;

HandleError:
    delete pentryNew;
    return err;
}

BOOL CBFIssueList::FEmpty() const
{
    return m_il.PrevMost() == NULL;
}

VOID CBFIssueList::NullifyDiskTiltFake( const IFMP ifmp )
{
    CEntry * pentry = m_il.PrevMost();
    if ( pentry )
    {
        if ( m_il.Next( pentry ) == NULL &&
                pentry->Oper() == CEntry::operWrite &&
                pentry->Ifmp() == ifmp &&
                pentry->CRequests() == 1 )
        {
            AssertRTL( PefLastThrow() && PefLastThrow()->Err() == errDiskTilt );
            Assert( pentry == m_il.PrevMost() );
            m_il.Remove( pentry );
            delete pentry;
            Assert( FEmpty() );
        }
        else
        {
            AssertSz( fFalse, "Client shouldn't be calling with these args / state" );
        }
    }
}


CCriticalSection            CBFIssueList::s_critSync( CLockBasicInfo( CSyncBasicInfo( _T( "CBFIssueList::s_critSync" ) ), rankBFIssueListSync, 0 ) );
CMeteredSection             CBFIssueList::s_msSync;



LOCAL volatile TICK g_tickMaintCacheSizeStartedLast = 0;
LOCAL volatile LONG_PTR g_cbfMaintCacheSizeStartedLast  = cbfMainCacheSizeStartedLastInactive;

LOCAL TICK g_tickLastMaintCacheStats        = 0;
LOCAL TICK g_tickLastCacheStatsRequest      = 0;
LOCAL TICK g_tickLastMaintCacheSize         = 0;

HMEMORY_NOTIFICATION g_pMemoryNotification = NULL;


LOCAL CBinaryLock g_blBFMaintScheduleCancel( CLockBasicInfo( CSyncBasicInfo( _T( "BFMaint Schedule/Cancel" ) ), rankBFMaintScheduleCancel, 0 ) );
LOCAL volatile BOOL g_fBFMaintInitialized = fFalse;

ERR ErrBFIMaintInit()
{
    ERR err = JET_errSuccess;

    Assert( !g_fBFMaintInitialized );


    g_tickLastCacheStatsRequest = TickOSTimeCurrent();


    cbfCacheDeadlock    = 0;
    g_cbfCacheDeadlockMax = 0;
    Call( ErrBFIMaintScavengeInit() );


    Call( ErrOSTimerTaskCreate( BFIMaintIdleDatabaseITask,  &ErrBFIMaintInit, &g_posttBFIMaintIdleDatabaseITask ) );
    Call( ErrOSTimerTaskCreate( BFIMaintCacheResidencyITask,  &ErrBFIMaintInit, &g_posttBFIMaintCacheResidencyITask ) );
    Call( ErrOSTimerTaskCreate( BFIMaintIdleCacheStatsITask,  &ErrBFIMaintInit, &g_posttBFIMaintIdleCacheStatsITask ) );
    Call( ErrOSTimerTaskCreate( BFIMaintCacheStatsITask,  &ErrBFIMaintInit, &g_posttBFIMaintCacheStatsITask ) );
    Call( ErrOSTimerTaskCreate( BFIMaintTelemetryITask,  &ErrBFIMaintInit, &g_posttBFIMaintTelemetryITask ) );
    Call( ErrOSTimerTaskCreate( BFIMaintCacheSizeITask,  &ErrBFIMaintInit, &g_posttBFIMaintCacheSizeITask ) );
    Call( ErrOSTimerTaskCreate( BFIMaintCheckpointITask,  &ErrBFIMaintInit, &g_posttBFIMaintCheckpointITask ) );
    Call( ErrOSTimerTaskCreate( BFIMaintCheckpointDepthITask,  &ErrBFIMaintInit, &g_posttBFIMaintCheckpointDepthITask ) );
    Call( ErrOSTimerTaskCreate( BFIMaintAvailPoolIUrgentTask,  &ErrBFIMaintInit, &g_posttBFIMaintAvailPoolIUrgentTask ) );
    Call( ErrOSTimerTaskCreate( BFIMaintAvailPoolITask,  &ErrBFIMaintInit, &g_posttBFIMaintAvailPoolITask ) );

    Call( ErrOSCreateLowMemoryNotification(
                BFIMaintLowMemoryCallback,
                NULL,
                &g_pMemoryNotification ) );

    g_semMaintAvailPoolRequestUrgent.Release();
    g_semMaintAvailPoolRequest.Release();
    g_semMaintScavenge.Release();


    g_ifmpMaintCheckpointDepthStart   = 0;

    g_semMaintCheckpointDepthRequest.Release();


    g_tickMaintCheckpointLast = TickOSTimeCurrent();

    g_semMaintCheckpointRequest.Release();


    g_tickLastMaintCacheStats = TickOSTimeCurrent() - dtickMaintCacheStatsTooOld;

    BFIMaintCacheStatsRelease();

    g_tickMaintCacheSizeStartedLast = TickOSTimeCurrent();
    g_cbfMaintCacheSizeStartedLast = cbfMainCacheSizeStartedLastInactive;
    g_cMaintCacheSizePending = 0;
    Assert( g_semMaintCacheSize.CAvail() == 0 );
    g_semMaintCacheSize.Release();

    BFIMaintCacheResidencyInit();


    g_tickMaintIdleDatabaseLast   = TickOSTimeCurrent();

    g_semMaintIdleDatabaseRequest.Release();


    g_fBFMaintInitialized = fTrue;


    CallS( ErrBFIMaintCacheStatsRequest( bfmcsrtForce ) );

    BFIMaintTelemetryRequest();

#if 0
#ifdef MINIMAL_FUNCTIONALITY
#else

    g_fBFMaintHashedLatches = fFalse;

#ifdef DEBUG
    const INT cMinProcsForHashedLatches = 1;
#else
    const INT cMinProcsForHashedLatches = 2;
#endif
#pragma prefast( suppress:6237, "The rest of the conditions do not have any side effects." )
    if (    fFalse &&
            !FJetConfigLowMemory() &&
            !FJetConfigMedMemory() &&
            !FJetConfigLowPower() &&
            OSSyncGetProcessorCountMax() >= cMinProcsForHashedLatches )
    {
        g_fBFMaintHashedLatches = fTrue;
        Call( ErrBFIMaintScheduleTask(  BFIMaintHashedLatchesITask,
                                        NULL,
                                        dtickMaintHashedLatchesPeriod,
                                        dtickMaintHashedLatchesPeriod ) );
    }

#endif
#endif

HandleError:

    if ( err < JET_errSuccess )
    {
        Assert( !g_fBFMaintInitialized );
        BFIMaintTerm();
    }
    else
    {
        Assert( g_fBFMaintInitialized );
    }

    return err;
}


void BFIMaintTerm()
{

    g_blBFMaintScheduleCancel.Enter2();


    if ( g_posttBFIMaintIdleDatabaseITask )
    {
        OSTimerTaskCancelTask( g_posttBFIMaintIdleDatabaseITask );
    }
    (void)g_semMaintIdleDatabaseRequest.FTryAcquire();


    BFIMaintCacheResidencyTerm();
    if ( g_posttBFIMaintIdleCacheStatsITask )
    {
        OSTimerTaskCancelTask( g_posttBFIMaintIdleCacheStatsITask );
    }
    if ( g_posttBFIMaintCacheStatsITask )
    {
        OSTimerTaskCancelTask( g_posttBFIMaintCacheStatsITask );
    }
    if ( g_posttBFIMaintTelemetryITask )
    {
        OSTimerTaskCancelTask( g_posttBFIMaintTelemetryITask );
    }

    if ( g_pMemoryNotification != NULL )
    {
        OSUnregisterAndDestroyMemoryNotification( g_pMemoryNotification );
        g_pMemoryNotification = NULL;
    }
    Assert( g_semMaintCacheSize.CAvail() <= 1 );
    if ( g_posttBFIMaintCacheSizeITask )
    {
        OSTimerTaskCancelTask( g_posttBFIMaintCacheSizeITask );
    }
    Assert( g_semMaintCacheSize.CAvail() <= 1 );
    (void)FBFIMaintCacheStatsTryAcquire();
    Assert( g_semMaintCacheSize.CAvail() <= 1 );
    (void)g_semMaintCacheSize.FTryAcquire();
    Assert( g_semMaintCacheSize.CAvail() == 0 );

#ifdef MINIMAL_FUNCTIONALITY
#else

#if 0

    (void)ErrBFIMaintCancelTask( BFIMaintHashedLatchesITask );
    g_fBFMaintHashedLatches = fFalse;
#endif

#endif


    if ( g_posttBFIMaintCheckpointITask )
    {
        OSTimerTaskCancelTask( g_posttBFIMaintCheckpointITask );
    }
    (void)g_semMaintCheckpointRequest.FTryAcquire();


    if ( g_posttBFIMaintCheckpointDepthITask )
    {
        OSTimerTaskCancelTask( g_posttBFIMaintCheckpointDepthITask );
    }
    (void)g_semMaintCheckpointDepthRequest.FTryAcquire();


    if ( g_posttBFIMaintAvailPoolIUrgentTask )
    {
        OSTimerTaskCancelTask( g_posttBFIMaintAvailPoolIUrgentTask );
    }
    if ( g_posttBFIMaintAvailPoolITask )
    {
        OSTimerTaskCancelTask( g_posttBFIMaintAvailPoolITask );
    }
    (void)g_semMaintAvailPoolRequestUrgent.FTryAcquire();
    (void)g_semMaintAvailPoolRequest.FTryAcquire();
    (void)g_semMaintScavenge.FTryAcquire();
    BFIMaintScavengeTerm();


    if ( g_posttBFIMaintIdleDatabaseITask )
    {
        OSTimerTaskDelete( g_posttBFIMaintIdleDatabaseITask );
        g_posttBFIMaintIdleDatabaseITask = NULL;
    }
    if ( g_posttBFIMaintCacheResidencyITask )
    {
        OSTimerTaskDelete( g_posttBFIMaintCacheResidencyITask );
        g_posttBFIMaintCacheResidencyITask = NULL;
    }
    if ( g_posttBFIMaintIdleCacheStatsITask )
    {
        OSTimerTaskDelete( g_posttBFIMaintIdleCacheStatsITask );
        g_posttBFIMaintIdleCacheStatsITask = NULL;
    }
    if ( g_posttBFIMaintCacheStatsITask )
    {
        OSTimerTaskDelete( g_posttBFIMaintCacheStatsITask );
        g_posttBFIMaintCacheStatsITask = NULL;
    }
    if ( g_posttBFIMaintTelemetryITask )
    {
        OSTimerTaskDelete( g_posttBFIMaintTelemetryITask );
        g_posttBFIMaintTelemetryITask = NULL;
    }
    if ( g_posttBFIMaintCacheSizeITask )
    {
        OSTimerTaskDelete( g_posttBFIMaintCacheSizeITask );
        g_posttBFIMaintCacheSizeITask = NULL;
    }
    if ( g_posttBFIMaintCheckpointITask )
    {
        OSTimerTaskDelete( g_posttBFIMaintCheckpointITask );
        g_posttBFIMaintCheckpointITask = NULL;
    }
    if ( g_posttBFIMaintCheckpointDepthITask )
    {
        OSTimerTaskDelete( g_posttBFIMaintCheckpointDepthITask );
        g_posttBFIMaintCheckpointDepthITask = NULL;
    }
    if ( g_posttBFIMaintAvailPoolIUrgentTask )
    {
        OSTimerTaskDelete( g_posttBFIMaintAvailPoolIUrgentTask );
        g_posttBFIMaintAvailPoolIUrgentTask = NULL;
    }
    if ( g_posttBFIMaintAvailPoolITask )
    {
        OSTimerTaskDelete( g_posttBFIMaintAvailPoolITask );
        g_posttBFIMaintAvailPoolITask = NULL;
    }


    g_fBFMaintInitialized = fFalse;
    g_blBFMaintScheduleCancel.Leave2();
}


ERR ErrBFIMaintScheduleTask(    POSTIMERTASK        postt,
                                const VOID * const  pvContext,
                                const TICK          dtickDelay,
                                const TICK          dtickSlop )
{
    ERR     err     = JET_errSuccess;
    BOOL    fLeave1 = fFalse;


    if ( !g_blBFMaintScheduleCancel.FTryEnter1() )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }
    fLeave1 = fTrue;

    if ( !g_fBFMaintInitialized )
    {
        FireWall( "ScheduleMaintUninitTimerTask" );
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    OSTimerTaskScheduleTask( postt, pvContext, dtickDelay, dtickSlop );

HandleError:
    if ( fLeave1 )
    {
        g_blBFMaintScheduleCancel.Leave1();
    }
    return err;
}



INLINE BOOL FBFIChance( INT pctChance )
{
#ifdef BF_CC_SIM
    return ( rand() % 100 ) < pctChance;
#else
    return fFalse;
#endif
}


INLINE void BFISynchronicity( void )
{
#ifdef BF_CC_SIM
    if ( FBFIChance( 10 ) )
    {
        UtilSleep( 0 );
    }
    else if ( FBFIChance( 10 ) )
    {
        UtilSleep( 1 );
    }
    else if ( FBFIChance( 10 ) )
    {
        if ( FBFIChance( 25 ) )
        {
            while ( FBFIChance( 50 ) )
            {
                UtilSleep( 16 );
            }
        }
        else
        {
            UtilSleep( rand() % 4 + 2 );
        }
    }

#endif
}



CSemaphore      g_semMaintAvailPoolRequestUrgent( CSyncBasicInfo( _T( "g_semMaintAvailPoolRequestUrgent" ) ) );
CSemaphore      g_semMaintAvailPoolRequest( CSyncBasicInfo( _T( "g_semMaintAvailPoolRequest" ) ) );

LONG_PTR        cbfAvailPoolLow;
LONG_PTR        cbfAvailPoolHigh;
LONG_PTR        cbfAvailPoolTarget;

LONG_PTR        cbfCacheDeadlock;
LONG_PTR        g_cbfCacheDeadlockMax;

POSTIMERTASK    g_posttBFIMaintAvailPoolIUrgentTask = NULL;
POSTIMERTASK    g_posttBFIMaintAvailPoolITask = NULL;


ERR ErrBFIMaintAvailPoolRequest( const BFIMaintAvailPoolRequestType bfmaprt )
{
    const BOOL  fForceSync          = ( bfmaprtSync == bfmaprt );
    const BOOL  fAllowSync          = ( bfmaprtAsync != bfmaprt );
    BOOL        fRequestAsync       = !fForceSync;
    ERR         err                 = JET_errSuccess;
    OnDebug( BOOL fExecutedSync     = fFalse );


    Assert( ( bfmaprtUnspecific == bfmaprt ) || ( bfmaprtSync == bfmaprt ) || ( bfmaprtAsync == bfmaprt ) );
    Assert( !fForceSync || fAllowSync );

    const DWORD cbfAvail = g_bfavail.Cobject();
    if ( ( cbfAvail >= (DWORD)cbfAvailPoolHigh ) && ( cbfCacheDeadlock <= g_cbfCacheTargetOptimal ) )
    {
        goto HandleError;
    }

    const BOOL fSmallPool = ( ( cbfAvailPoolHigh - cbfAvailPoolLow ) < 10 ) || ( cbfAvailPoolLow < 5 );
    const BOOL fLowPool = ( cbfAvail <= (DWORD)cbfAvailPoolLow );

    if ( fForceSync || ( fAllowSync && fSmallPool && fLowPool ) )
    {

        if ( g_semMaintScavenge.FTryAcquire() )
        {


            CLockDeadlockDetectionInfo::DisableOwnershipTracking();
            CLockDeadlockDetectionInfo::DisableDeadlockDetection();
            if ( fForceSync )
            {
                err = ErrBFIMaintScavengeIScavengePages( __FUNCTION__, fTrue );
            }
            else
            {
                (void)ErrBFIMaintScavengeIScavengePages( __FUNCTION__, fFalse );
            }
            OnDebug( fExecutedSync = fTrue );
            CLockDeadlockDetectionInfo::EnableDeadlockDetection();
            CLockDeadlockDetectionInfo::EnableOwnershipTracking();
            g_semMaintScavenge.Release();


            fRequestAsync = fFalse;
        }
    }


    if ( fRequestAsync )
    {
        const BOOL fUrgent = fLowPool;
        CSemaphore* const psemMaintAvailPoolRequest = fUrgent ? &g_semMaintAvailPoolRequestUrgent : &g_semMaintAvailPoolRequest;

        if ( psemMaintAvailPoolRequest->FTryAcquire() )
        {
            const ERR errSchedule = fUrgent ?
                ErrBFIMaintScheduleTask( g_posttBFIMaintAvailPoolIUrgentTask, NULL, dtickMaintAvailPoolRequestUrgent, 0 ) :
                ErrBFIMaintScheduleTask( g_posttBFIMaintAvailPoolITask, NULL, dtickMaintAvailPoolRequest, 0 );

            if ( errSchedule < JET_errSuccess )
            {
                psemMaintAvailPoolRequest->Release();
            }
        }
    }

HandleError:

#ifdef DEBUG
    if ( err != JET_errSuccess )
    {
        Expected( fForceSync && fExecutedSync );
        Expected( ( err == wrnSlow ) || ( err == JET_errOutOfBuffers ) || ( err == JET_errOutOfMemory ) );
    }
#endif

    return err;
}


void BFIMaintAvailPoolITask_( CSemaphore* const psemMaintAvailPoolRequest )
{

    Assert( psemMaintAvailPoolRequest->CAvail() == 0 );


    if ( g_semMaintScavenge.FTryAcquire() )
    {

        (void)ErrBFIMaintScavengeIScavengePages( __FUNCTION__, fFalse );
        g_semMaintScavenge.Release();
    }

    Assert( psemMaintAvailPoolRequest->CAvail() == 0 );
    psemMaintAvailPoolRequest->Release();
}


void BFIMaintAvailPoolIUrgentTask( void*, void* )
{
    OSTrace( JET_tracetagBufferManagerMaintTasks, __FUNCTION__ );

    BFIMaintAvailPoolITask_( &g_semMaintAvailPoolRequestUrgent );
}


void BFIMaintAvailPoolITask( void*, void* )
{
    OSTrace( JET_tracetagBufferManagerMaintTasks, __FUNCTION__ );

    BFIMaintAvailPoolITask_( &g_semMaintAvailPoolRequest );
}



CSemaphore g_semMaintScavenge( CSyncBasicInfo( _T( "g_semMaintScavenge" ) ) );


TICK                g_dtickMaintScavengeTimeout = 0;
size_t              g_cScavengeTimeSeq = 0;

ULONG               g_iScavengeLastRun = 0;
BFScavengeStats*    g_rgScavengeLastRuns = NULL; // [ OnDebugOrRetailOrRtm( 150, 30, 4 ) ];


ULONG               g_iScavengeTimeSeqLast = 0;
BFScavengeStats*    g_rgScavengeTimeSeq = NULL;

#ifndef RTM
BFScavengeStats*    g_rgScavengeTimeSeqCumulative = NULL;
#endif

#ifdef DEBUG
BOOL                g_modeExtraScavengingRuns = 0;
#endif

LONG                g_cCacheBoosts = 0;
__int64             g_cbCacheBoosted = 0;


INLINE ULONG UlBFIMaintScavengeAvailPoolSev( const BFScavengeStats& stats )
{
    Assert( stats.cbfAvailPoolTarget >= stats.cbfAvailPoolLow );
    
    if ( stats.cbfAvail < stats.cbfAvailPoolLow )
    {
        return ulMaintScavengeSevMax;
    }

    if ( stats.cbfAvail >= stats.cbfAvailPoolTarget )
    {
        return ulMaintScavengeSevMin;
    }

    if ( ( stats.cbfAvailPoolTarget - 1 ) <= stats.cbfAvailPoolLow )
    {
        return ulMaintScavengeSevMax - 1;
    }

    return ( ulMaintScavengeSevMin + 1 ) +
            (ULONG)( ( ( (__int64)( ( ulMaintScavengeSevMax - 1 ) - ( ulMaintScavengeSevMin + 1 ) ) ) *
            ( (__int64)( ( stats.cbfAvailPoolTarget - 1 ) - stats.cbfAvail ) ) ) /
            ( (__int64)( ( stats.cbfAvailPoolTarget - 1 ) - stats.cbfAvailPoolLow ) ) );
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( BF, BufferScavengingAvailPoolSev )
{
    BFScavengeStats stats = { 0 };
    static_assert( ulMaintScavengeSevMin == 0, "ulMaintScavengeSevMin is assumed 0." );
    static_assert( ulMaintScavengeSevMax == 1000, "ulMaintScavengeSevMax is assumed 1000." );

    stats.cbfAvailPoolLow = 20000;
    stats.cbfAvailPoolTarget = 30000;

    stats.cbfAvail = 1;
    CHECK( 1000 == UlBFIMaintScavengeAvailPoolSev( stats ) );

    stats.cbfAvail = 19999;
    CHECK( 1000 == UlBFIMaintScavengeAvailPoolSev( stats ) );

    stats.cbfAvail = 20000;
    CHECK( 999 == UlBFIMaintScavengeAvailPoolSev( stats ) );

    stats.cbfAvail = 22500;
    CHECK( 749 == UlBFIMaintScavengeAvailPoolSev( stats ) );

    stats.cbfAvail = 25000;
    CHECK( 499 == UlBFIMaintScavengeAvailPoolSev( stats ) );

    stats.cbfAvail = 27500;
    CHECK( 250 == UlBFIMaintScavengeAvailPoolSev( stats ) );

    stats.cbfAvail = 29999;
    CHECK( 1 == UlBFIMaintScavengeAvailPoolSev( stats ) );

    stats.cbfAvail = 30000;
    CHECK( 0 == UlBFIMaintScavengeAvailPoolSev( stats ) );

    stats.cbfAvail = 30001;
    CHECK( 0 == UlBFIMaintScavengeAvailPoolSev( stats ) );

    stats.cbfAvail = 100000;
    CHECK( 0 == UlBFIMaintScavengeAvailPoolSev( stats ) );

    stats.cbfAvailPoolLow = 20000;
    stats.cbfAvailPoolTarget = 20001;
    stats.cbfAvail = 19999;
    CHECK( 1000 == UlBFIMaintScavengeAvailPoolSev( stats ) );
    stats.cbfAvail = 20000;
    CHECK( 999 == UlBFIMaintScavengeAvailPoolSev( stats ) );
    stats.cbfAvail = 20001;
    CHECK( 0 == UlBFIMaintScavengeAvailPoolSev( stats ) );
}

#endif


INLINE ULONG UlBFIMaintScavengeShrinkSev( const BFScavengeStats& stats )
{
    if ( stats.dtickShrinkDuration == 0 )
    {
        return ulMaintScavengeSevMin;
    }

    const LONG dcbfShrinkDeficit = stats.cbfCacheSize - stats.cbfCacheTarget;

    if ( dcbfShrinkDeficit <= 0 )
    {
        return ulMaintScavengeSevMin;
    }

    const LONG cbfCacheSizeStartShrink = LFunctionalMax( stats.cbfCacheSizeStartShrink, stats.cbfCacheTarget );
    const LONG dcbfShrinkRange = cbfCacheSizeStartShrink - stats.cbfCacheTarget;

    const __int64 cbfCacheSizeExpected = (__int64)cbfCacheSizeStartShrink -
                                        ( ( (__int64)dcbfShrinkRange ) * ( (__int64)stats.dtickShrinkDuration ) ) / ( (__int64)dtickMaintScavengeShrinkMax );
    const LONG dcbfCacheSizeDiscrepancy = LFunctionalMin( stats.cbfCacheSize - (LONG)max( cbfCacheSizeExpected, 0 ), dcbfShrinkRange + 1 );

    if ( dcbfCacheSizeDiscrepancy <= 0 )
    {
        return ulMaintScavengeSevMin;
    }

    if ( dcbfCacheSizeDiscrepancy > dcbfShrinkRange )
    {
        return ulMaintScavengeSevMax;
    }

    return ( ulMaintScavengeSevMin + 1 ) +
            (ULONG)( ( ( (__int64)( ( ulMaintScavengeSevMax - 1 ) - ( ulMaintScavengeSevMin + 1 ) ) ) *
            ( (__int64)dcbfCacheSizeDiscrepancy ) ) / ( (__int64)dcbfShrinkRange ) );
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( BF, BufferScavengingShrinkSev )
{
    BFScavengeStats stats = { 0 };
    static_assert( ulMaintScavengeSevMin == 0, "ulMaintScavengeSevMin is assumed 0." );
    static_assert( ulMaintScavengeSevMax == 1000, "ulMaintScavengeSevMax is assumed 1000." );
    static_assert( dtickMaintScavengeShrinkMax == 30 * 1000, "ulMaintScavengeSevMax is assumed 30 * 1000." );

    stats.cbfCacheSizeStartShrink = 30000;
    stats.cbfCacheTarget = 20000;

    stats.dtickShrinkDuration = 0;
    stats.cbfCacheSize = 15000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );
    stats.cbfCacheSize = 20000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );
    stats.cbfCacheSize = 25000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );
    stats.cbfCacheSize = 30000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );
    stats.cbfCacheSize = 35000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 20000;
    stats.dtickShrinkDuration = 15 * 1000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );
    stats.dtickShrinkDuration = 45 * 1000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );
    stats.cbfCacheSize = 10000;
    stats.dtickShrinkDuration = 15 * 1000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );
    stats.dtickShrinkDuration = 45 * 1000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.dtickShrinkDuration = 7500;

    stats.cbfCacheSize = 30000;
    CHECK( 250 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 28750;
    CHECK( 125 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 27750;
    CHECK( 25 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 27500;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 25000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.dtickShrinkDuration = 15 * 1000;

    stats.cbfCacheSize = 30000;
    CHECK( 500 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 27500;
    CHECK( 250 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 25500;
    CHECK( 50 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 25000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 22500;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.dtickShrinkDuration = 22500;

    stats.cbfCacheSize = 30000;
    CHECK( 749 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 26250;
    CHECK( 375 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 23250;
    CHECK( 75 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 22500;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 21000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.dtickShrinkDuration = 30 * 1000;

    stats.cbfCacheSize = 30000;
    CHECK( 999 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 25000;
    CHECK( 500 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 21000;
    CHECK( 100 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 20000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 15000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.dtickShrinkDuration = 45 * 1000;

    stats.cbfCacheSize = 30000;
    CHECK( 1000 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 25000;
    CHECK( 999 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 21000;
    CHECK( 599 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 20000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 15000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.dtickShrinkDuration = 60 * 1000;

    stats.cbfCacheSize = 30000;
    CHECK( 1000 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 25000;
    CHECK( 1000 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 20100;
    CHECK( 1000 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 20000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 15000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.dtickShrinkDuration = 240 * 1000;

    stats.cbfCacheSize = 30000;
    CHECK( 1000 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 25000;
    CHECK( 1000 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 21000;
    CHECK( 1000 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 20000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );

    stats.cbfCacheSize = 15000;
    CHECK( 0 == UlBFIMaintScavengeShrinkSev( stats ) );
}

#endif


INLINE OSFILEQOS QosBFIMaintScavengePages( const ULONG_PTR ulIoPriority, const ULONG ulScavengeSev )
{
    OSFILEQOS qosIoOsPriority = qosIOOSNormalPriority;
    OSFILEQOS qosIoDispatch = qosIODispatchImmediate;

    Assert( ( ulScavengeSev >= ulMaintScavengeSevMin ) && ( ulScavengeSev <= ulMaintScavengeSevMax ) );

    if ( ulScavengeSev == ulMaintScavengeSevMax )
    {
        goto HandleError;
    }

    if ( ( ulIoPriority & JET_IOPriorityLowForScavenge ) != 0 )
    {
        qosIoOsPriority = qosIOOSLowPriority;
    }


    if ( ulScavengeSev == ulMaintScavengeSevMin )
    {
        qosIoDispatch = qosIODispatchBackground;
        goto HandleError;
    }

    const INT iUrgentLevel =
            qosIODispatchUrgentBackgroundLevelMin +
            (INT)( ( ( (__int64)( qosIODispatchUrgentBackgroundLevelMax - qosIODispatchUrgentBackgroundLevelMin ) ) *
            ( (__int64)( ulScavengeSev - ( ulMaintScavengeSevMin + 1 ) ) ) ) / ( (__int64)( ( ulMaintScavengeSevMax - 1 ) - ( ulMaintScavengeSevMin + 1 ) ) ) );
    qosIoDispatch = QosOSFileFromUrgentLevel( iUrgentLevel );

HandleError:
    return ( qosIoDispatch | qosIoOsPriority );
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( BF, BufferScavengingSverityToQos )
{
    ULONG_PTR ulIoPriority = 0;
    ULONG ulScavengeSev = 0;
    static_assert( ulMaintScavengeSevMin == 0, "ulMaintScavengeSevMin is assumed 0." );
    static_assert( ulMaintScavengeSevMax == 1000, "ulMaintScavengeSevMax is assumed 1000." );
    static_assert( qosIODispatchUrgentBackgroundLevelMin == 1, "qosIODispatchUrgentBackgroundLevelMin is assumed 1." );
    static_assert( qosIODispatchUrgentBackgroundLevelMax == 127, "qosIODispatchUrgentBackgroundLevelMax is assumed 127." );

    ulIoPriority = JET_IOPriorityLowForScavenge;

    ulScavengeSev = 0;
    CHECK( ( qosIOOSLowPriority | qosIODispatchBackground ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );

    ulScavengeSev = 1;
    CHECK( ( qosIOOSLowPriority | QosOSFileFromUrgentLevel( 1 ) ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );

    ulScavengeSev = 250;
    CHECK( ( qosIOOSLowPriority | QosOSFileFromUrgentLevel( 32 ) ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );

    ulScavengeSev = 500;
    CHECK( ( qosIOOSLowPriority | QosOSFileFromUrgentLevel( 64 ) ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );

    ulScavengeSev = 750;
    CHECK( ( qosIOOSLowPriority | QosOSFileFromUrgentLevel( 95 ) ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );

    ulScavengeSev = 999;
    CHECK( ( qosIOOSLowPriority | QosOSFileFromUrgentLevel( 127 ) ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );

    ulScavengeSev = 1000;
    CHECK( ( qosIOOSNormalPriority | qosIODispatchImmediate ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );

    ulIoPriority = JET_IOPriorityNormal;

    ulScavengeSev = 0;
    CHECK( ( qosIOOSNormalPriority | qosIODispatchBackground ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );

    ulScavengeSev = 1;
    CHECK( ( qosIOOSNormalPriority | QosOSFileFromUrgentLevel( 1 ) ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );

    ulScavengeSev = 250;
    CHECK( ( qosIOOSNormalPriority | QosOSFileFromUrgentLevel( 32 ) ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );

    ulScavengeSev = 500;
    CHECK( ( qosIOOSNormalPriority | QosOSFileFromUrgentLevel( 64 ) ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );

    ulScavengeSev = 750;
    CHECK( ( qosIOOSNormalPriority | QosOSFileFromUrgentLevel( 95 ) ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );

    ulScavengeSev = 999;
    CHECK( ( qosIOOSNormalPriority | QosOSFileFromUrgentLevel( 127 ) ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );

    ulScavengeSev = 1000;
    CHECK( ( qosIOOSNormalPriority | qosIODispatchImmediate ) == QosBFIMaintScavengePages( ulIoPriority, ulScavengeSev ) );
}

#endif


ERR ErrBFIMaintScavengeIScavengePages( const char* const szContextTraceOnly, const BOOL fSync )
{
    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

    Assert( g_semMaintScavenge.CAvail() == 0  );



    BFScavengeStats statsCurrRun = { 0 };
    BOOL fOwnCacheResizeCritSec = fFalse;
    LONG_PTR cbfCacheAddressableInitial = -1;
    LONG_PTR cbfCacheSizeInitial = -1;
    PBF pbf = NULL;


    Assert( g_iScavengeLastRun < g_cScavengeLastRuns );
    C_ASSERT( g_cScavengeLastRuns >= 1 );
    AssumePREFAST( g_iScavengeLastRun < g_cScavengeLastRuns );
    statsCurrRun.iRun = g_rgScavengeLastRuns[g_iScavengeLastRun].iRun + 1;
    statsCurrRun.fSync = fSync;
    statsCurrRun.tickStart = TickOSTimeCurrent();


    if ( ( cbfCacheSize > cbfCacheTarget ) && ( g_bfavail.Cobject() > (DWORD)cbfAvailPoolHigh ) )
    {
        Assert( !fOwnCacheResizeCritSec );
        fOwnCacheResizeCritSec = g_critCacheSizeResize.FTryEnter();
        if ( fOwnCacheResizeCritSec )
        {
            cbfCacheAddressableInitial = cbfCacheAddressable;
            cbfCacheSizeInitial = cbfCacheSize;

            BFAvail::CLock lockAvail;
            g_bfavail.BeginPoolScan( &lockAvail );
            pbf = NULL;
            while ( ( cbfCacheSize > cbfCacheTarget ) &&
                    ( g_bfavail.Cobject() > (DWORD)cbfAvailPoolHigh ) &&
                    ( g_bfavail.ErrGetNextObject( &lockAvail, &pbf ) == BFAvail::ERR::errSuccess ) )
            {
                Assert( !pbf->fInOB0OL && pbf->ob0ic.FUninitialized() );
                if ( g_bfavail.ErrRemoveCurrentObject( &lockAvail ) == BFAvail::ERR::errSuccess )
                {
                    pbf->sxwl.ClaimOwnership( bfltWrite );
                    BFIReleaseBuffer( pbf );
                    statsCurrRun.cbfShrinkFromAvailPool++;
                }
            }
            g_bfavail.EndPoolScan( &lockAvail );
        }
    }

    OnDebug( CBF cbfCacheUsed = CbfBFICacheUsed() );
    OnDebug( CBF cbfCacheUsedMin = cbfCacheUsed );
    OnDebug( CBF cbfCacheUsedMax = cbfCacheUsed );


    CBFIssueList bfil;
    BFLRUK::ERR errLRUK = BFLRUK::ERR::errSuccess;
    BFLRUK::CLock lockLRUK;
    g_bflruk.BeginResourceScan( &lockLRUK );
    OnDebug( const LONG cbfSuperColdedInitial = g_bflruk.CSuperColdSuccesses() );
    pbf = NULL;
    statsCurrRun.eStopReason = eScavengeInvalid;
    while ( fTrue )
    {
        BFEvictFlags bfefReason = bfefReasonMax;
        ULONG ulScavengeWriteSev = ulMax;


        statsCurrRun.cbfCacheSize = (LONG)cbfCacheSize;
        Assert( statsCurrRun.cbfCacheSize >= 0 );

        statsCurrRun.cbfCacheTarget = (LONG)cbfCacheTarget;
        Assert( statsCurrRun.cbfCacheTarget >= 0 );

        statsCurrRun.cCacheBoosts = g_cCacheBoosts;
        statsCurrRun.cbCacheBoosted = g_cbCacheBoosted;

        const LONG_PTR cbfMaintCacheSizeStartedLast = g_cbfMaintCacheSizeStartedLast;
        if ( cbfMaintCacheSizeStartedLast >= 0 )
        {
            statsCurrRun.cbfCacheSizeStartShrink = (LONG)cbfMaintCacheSizeStartedLast;
            Assert( statsCurrRun.cbfCacheSizeStartShrink >= 0 );
            statsCurrRun.dtickShrinkDuration = (TICK)LFunctionalMax( DtickDelta( g_tickMaintCacheSizeStartedLast, TickOSTimeCurrent() ), 0 );
        }
        else
        {
            statsCurrRun.cbfCacheSizeStartShrink = 0;
            statsCurrRun.dtickShrinkDuration = 0;
        }

        statsCurrRun.cbfAvailPoolLow = (LONG)cbfAvailPoolLow;
        Assert( statsCurrRun.cbfAvailPoolLow >= 0 );

        statsCurrRun.cbfAvailPoolHigh = LFunctionalMax( statsCurrRun.cbfAvailPoolLow, (LONG)cbfAvailPoolHigh );
        Assert( statsCurrRun.cbfAvailPoolHigh >= 0 );
        Assert( statsCurrRun.cbfAvailPoolLow <= statsCurrRun.cbfAvailPoolHigh );

        statsCurrRun.cbfAvailPoolTarget = LBound( (LONG)cbfAvailPoolTarget, statsCurrRun.cbfAvailPoolLow, statsCurrRun.cbfAvailPoolHigh );
        Assert( statsCurrRun.cbfAvailPoolTarget >= 0 );
        Assert( statsCurrRun.cbfAvailPoolLow <= statsCurrRun.cbfAvailPoolTarget );
        Assert( statsCurrRun.cbfAvailPoolTarget <= statsCurrRun.cbfAvailPoolHigh );

        statsCurrRun.cbfAvail = (LONG)g_bfavail.Cobject();
        Assert( statsCurrRun.cbfAvail >= 0 );

        const LONG_PTR dcbfAvailPoolDeficit = LpFunctionalMax( statsCurrRun.cbfAvailPoolHigh - statsCurrRun.cbfAvail, 0 );
        const LONG_PTR dcbfShrinkDeficit = LpFunctionalMax( statsCurrRun.cbfCacheSize - statsCurrRun.cbfCacheTarget, 0 );


        if ( ( dcbfAvailPoolDeficit == 0 ) && ( dcbfShrinkDeficit == 0 ) )
        {
            statsCurrRun.eStopReason = eScavengeCompleted;
            break;
        }

        Assert( statsCurrRun.cbfFlushPending >= ( statsCurrRun.cbfFlushPendingSlow + statsCurrRun.cbfFlushPendingHung ) );
        if ( ( dcbfAvailPoolDeficit + dcbfShrinkDeficit ) <= ( statsCurrRun.cbfFlushPending - statsCurrRun.cbfFlushPendingSlow - statsCurrRun.cbfFlushPendingHung ) )
        {
            statsCurrRun.eStopReason = eScavengeCompletedPendingWrites;
            break;
        }

#ifdef DEBUG
        if ( g_modeExtraScavengingRuns == 0 )
        {
            if ( ( g_modeExtraScavengingRuns = (BOOL)UlConfigOverrideInjection( 37600, 0 ) ) == 0 )
            {
                g_modeExtraScavengingRuns = ( ( rand() % 4 ) == 3 ) ? 2  : 1 ;
            }
            
        }
        if ( ( statsCurrRun.cbfAvail > 0 ) &&
            ( statsCurrRun.dtickShrinkDuration <= ( dtickMaintScavengeShrinkMax / 10 ) ) &&
            ( ( rand() % g_modeExtraScavengingRuns ) != 0 ) &&
            !FNegTest( fStrictIoPerfTesting ) )
        {
            statsCurrRun.eStopReason = eScavengeBailedRandomlyDebug;
            break;
        }
#endif


        errLRUK = g_bflruk.ErrGetNextResource( &lockLRUK, &pbf );

        if ( errLRUK != BFLRUK::ERR::errSuccess )
        {
            Assert( errLRUK == BFLRUK::ERR::errNoCurrentResource );
            statsCurrRun.eStopReason = eScavengeVisitedAllLrukEntries;
            break;
        }

        statsCurrRun.cbfVisited++;


        if ( ( dcbfShrinkDeficit > 0 ) && ( dcbfAvailPoolDeficit == 0 ) )
        {
            fOwnCacheResizeCritSec = fOwnCacheResizeCritSec || g_critCacheSizeResize.FTryEnter();

            if ( !fOwnCacheResizeCritSec )
            {
                statsCurrRun.eStopReason = eScavengeBailedExternalResize;
                break;
            }

            bfefReason = bfefReasonShrink;
            ulScavengeWriteSev = UlBFIMaintScavengeShrinkSev( statsCurrRun );
        }

        else if ( ( dcbfAvailPoolDeficit > 0 ) && ( dcbfShrinkDeficit == 0 ) )
        {
            bfefReason = bfefReasonAvailPool;
            ulScavengeWriteSev = UlBFIMaintScavengeAvailPoolSev( statsCurrRun );
        }

        else
        {
            const ULONG ulScavengeWriteAvailPoolSev = UlBFIMaintScavengeAvailPoolSev( statsCurrRun );
            const ULONG ulScavengeWriteShrinkSev = UlBFIMaintScavengeShrinkSev( statsCurrRun );
            if ( ulScavengeWriteAvailPoolSev > ulScavengeWriteShrinkSev )
            {
                bfefReason = bfefReasonAvailPool;
            }
            else if ( ulScavengeWriteShrinkSev > ulScavengeWriteAvailPoolSev )
            {
                bfefReason = bfefReasonShrink;
            }
            else
            {
                if ( ulScavengeWriteAvailPoolSev <= ( ( ulMaintScavengeSevMin + ulMaintScavengeSevMax ) / 2 ) )
                {
                    bfefReason = bfefReasonShrink;
                }
                else
                {
                    bfefReason = bfefReasonAvailPool;
                }
            }

            if ( ( bfefReason == bfefReasonShrink ) &&
                !( fOwnCacheResizeCritSec = ( fOwnCacheResizeCritSec || g_critCacheSizeResize.FTryEnter() ) ) )
            {
                bfefReason = bfefReasonAvailPool;
            }

            ulScavengeWriteSev = ( bfefReason == bfefReasonShrink ) ? ulScavengeWriteShrinkSev : ulScavengeWriteAvailPoolSev;
        }

        if ( ( bfefReason == bfefReasonShrink ) && ( cbfCacheAddressableInitial == -1 ) )
        {
            Assert( cbfCacheSizeInitial == -1 );
            cbfCacheAddressableInitial = cbfCacheAddressable;
            cbfCacheSizeInitial = cbfCacheSize;
        }


        BOOL fPermanentErr = fFalse;
        BOOL fHungIO = fFalse;
        Assert( ( bfefReason == bfefReasonAvailPool ) || ( bfefReason == bfefReasonShrink ) );
        Assert( ( bfefReason != bfefReasonShrink ) || ( fOwnCacheResizeCritSec && ( cbfCacheAddressableInitial != -1 ) && ( cbfCacheSizeInitial != -1 ) ) );
        const BFEvictFlags bfef = BFEvictFlags( bfefReason | ( ( bfefReason == bfefReasonShrink ) ? bfefQuiesce : bfefNone ) );
        const ERR errEvict = ErrBFIEvictPage( pbf, &lockLRUK, bfef );
        if ( errEvict >= JET_errSuccess )
        {
            if ( bfefReason == bfefReasonAvailPool )
            {
                statsCurrRun.cbfEvictedAvailPool++;
            }
            else
            {
                statsCurrRun.cbfEvictedShrink++;
            }
        }
        else if ( errEvict == errBFIPageDirty )
        {

            if ( pbf->sxwl.ErrTryAcquireSharedLatch() == CSXWLatch::ERR::errSuccess )
            {
                g_bflruk.PuntResource( pbf, (TICK)( g_csecBFLRUKUncertainty * 1000 ) );
                pbf->sxwl.ReleaseSharedLatch();
            }

            const IOREASON ior = IOR( ( ( bfefReason == bfefReasonShrink ) ? iorpBFShrink : iorpBFAvailPool ), fSync ? iorfForeground : iorfNone );
            const OSFILEQOS qos = QosBFIMaintScavengePages( UlParam( PinstFromIfmp( pbf->ifmp ), JET_paramIOPriority ), ulScavengeWriteSev );
            const ERR errFlush = ErrBFIFlushPage( pbf, ior, qos, bfdfDirty, fFalse , &fPermanentErr );

            if ( errFlush == errBFLatchConflict )
            {
                statsCurrRun.cbfLatched++;
            }

            else if (   errFlush == errBFIRemainingDependencies ||
                        errFlush == errBFIPageTouchTooRecent ||
                        errFlush == errBFIDependentPurged )
            {
                statsCurrRun.cbfDependent++;
                statsCurrRun.cbfTouchTooRecent += ( errFlush == errBFIPageTouchTooRecent );
                fPermanentErr = fPermanentErr || ( errFlush == errBFIDependentPurged );
            }

            else if ( errFlush == errBFIPageFlushed )
            {
                statsCurrRun.cbfFlushed++;
                statsCurrRun.cbfFlushPending++;
            }

            else if ( errFlush == errBFIPageFlushPending )
            {
                statsCurrRun.cbfFlushPending++;
            }

            else if ( errFlush == errBFIPageFlushPendingSlowIO )
            {
                statsCurrRun.cbfFlushPending++;
                statsCurrRun.cbfFlushPendingSlow++;
            }

            else if ( errFlush == errBFIPageFlushPendingHungIO )
            {
                statsCurrRun.cbfFlushPending++;
                statsCurrRun.cbfFlushPendingHung++;
                fHungIO = fTrue;
            }

            else if ( errFlush == errBFIPageFlushDisallowedOnIOThread )
            {
                Assert( !fPermanentErr );
                Assert( FIOThread() );
                FireWall( "ScavengeFromIoThread" );
            }

            else if ( errFlush == errDiskTilt )
            {
                statsCurrRun.cbfDiskTilt++;
                Assert( !fPermanentErr );
                (void)bfil.ErrIssue( fFalse );
                statsCurrRun.eStopReason = eScavengeBailedDiskTilt;
                break;
            }
            else if ( errFlush == JET_errOutOfMemory )
            {
                statsCurrRun.cbfOutOfMemory++;
            }
            else if ( errFlush == errBFIPageAbandoned )
            {
                statsCurrRun.cbfAbandoned++;
            }
            else
            {
                fHungIO = FBFIIsIOHung( pbf );
                AssertSz( !fHungIO, "Hung I/Os should have been captured by other errors, so it's not expected here!" );
            }
        }
        else if ( errEvict == errBFIPageFlushPending )
        {
            statsCurrRun.cbfFlushPending++;
        }
        else if ( errEvict == errBFIPageFlushPendingSlowIO )
        {
            statsCurrRun.cbfFlushPending++;
            statsCurrRun.cbfFlushPendingSlow++;
        }
        else if ( errEvict == errBFIPageFlushPendingHungIO )
        {
            statsCurrRun.cbfFlushPending++;
            statsCurrRun.cbfFlushPendingHung++;
            fHungIO = fTrue;
        }
        else if ( errEvict == errBFLatchConflict )
        {
            statsCurrRun.cbfLatched++;
        }
        else if ( errEvict == errBFIPageFaultPending )
        {
            statsCurrRun.cbfFaultPending++;
        }
        else if ( errEvict == errBFIPageFaultPendingHungIO )
        {
            statsCurrRun.cbfFaultPending++;
            statsCurrRun.cbfFaultPendingHung++;
            fHungIO = fTrue;
        }
        else
        {
            FireWall( OSFormat( "ScavengeUnknownErr:%d", errEvict ) );
        }

        if ( fHungIO )
        {
            fPermanentErr = fTrue;
            statsCurrRun.cbfHungIOs++;
        }

        if ( fPermanentErr )
        {
            if ( pbf->sxwl.ErrTryAcquireSharedLatch() == CSXWLatch::ERR::errSuccess )
            {
                g_bflruk.PuntResource( pbf, 60 * 1000 );
                pbf->sxwl.ReleaseSharedLatch();
            }

            statsCurrRun.cbfPermanentErrs++;
        }

#ifdef DEBUG
        Assert( statsCurrRun.cbfFlushPending >= ( statsCurrRun.cbfFlushPendingSlow + statsCurrRun.cbfFlushPendingHung ) );
        Assert( statsCurrRun.cbfFaultPending >= statsCurrRun.cbfFaultPendingHung );

        cbfCacheUsed = CbfBFICacheUsed();
        cbfCacheUsedMin = UlpFunctionalMin( cbfCacheUsedMin, cbfCacheUsed );
        cbfCacheUsedMax = UlpFunctionalMax( cbfCacheUsedMax, cbfCacheUsed );
#endif

    }// end while ( fTrue ) - for each target resource loop.

#ifdef DEBUG
    Assert( statsCurrRun.eStopReason != eScavengeInvalid );
    Assert( ( errLRUK == BFLRUK::ERR::errNoCurrentResource ) || ( statsCurrRun.eStopReason != eScavengeVisitedAllLrukEntries ) );
    Assert( ( errLRUK != BFLRUK::ERR::errNoCurrentResource ) || ( statsCurrRun.eStopReason == eScavengeVisitedAllLrukEntries ) );

    cbfCacheUsed = CbfBFICacheUsed();
    cbfCacheUsedMin = UlpFunctionalMin( cbfCacheUsedMin, cbfCacheUsed );
    cbfCacheUsedMax = UlpFunctionalMax( cbfCacheUsedMax, cbfCacheUsed );

    const LONG cbfSuperColdedFinal = g_bflruk.CSuperColdSuccesses();

    if ( cbfCacheUsedMin >= 1000 )
    {
        if ( statsCurrRun.eStopReason == eScavengeVisitedAllLrukEntries )
        {
            const LONG cbfSuperColded = cbfSuperColdedFinal - cbfSuperColdedInitial;
            Assert( cbfSuperColded >= 0 );
            Expected( ( statsCurrRun.cbfVisited + cbfSuperColded ) >= ( cbfCacheUsedMin - cbfCacheUsedMin / 4 ) );
        }

        Expected( statsCurrRun.cbfVisited <= ( cbfCacheUsedMax + cbfCacheUsedMax / 4 ) );
    }
#endif


    g_bflruk.EndResourceScan( &lockLRUK );

    (void)bfil.ErrIssue( fFalse );

    Assert( statsCurrRun.cbfFlushPending >= statsCurrRun.cbfFlushPendingHung );
    Assert( statsCurrRun.cbfFaultPending >= statsCurrRun.cbfFaultPendingHung );
    cbfCacheDeadlock = (    statsCurrRun.cbfPermanentErrs +
                            statsCurrRun.cbfDependent +
                            statsCurrRun.cbfLatched +
                            ( statsCurrRun.cbfFaultPending - statsCurrRun.cbfFaultPendingHung ) +
                            ( statsCurrRun.cbfFlushPending - statsCurrRun.cbfFlushPendingHung ) +
                            g_bfavail.CWaiter() + ( fSync ? 1 : 0 ) );
    const LONG_PTR cbfCacheDeadlockMargin = max( cbfCacheDeadlock / 10, 1 );
    cbfCacheDeadlock = cbfCacheDeadlock + cbfCacheDeadlockMargin;
    g_cbfCacheDeadlockMax = max( g_cbfCacheDeadlockMax, cbfCacheDeadlock );
    Assert( cbfCacheDeadlock < INT_MAX );
    statsCurrRun.cbfCacheDeadlock = (LONG)cbfCacheDeadlock;


    ERR errHang = JET_errSuccess;
    if ( ( statsCurrRun.eStopReason == eScavengeVisitedAllLrukEntries ) &&
        ( ( statsCurrRun.cbfEvictedAvailPool + statsCurrRun.cbfEvictedShrink ) == 0 ) &&
        ( statsCurrRun.cbfVisited > 0 ) )
    {
        const LONG cbfUnflushable = statsCurrRun.cbfPermanentErrs + statsCurrRun.cbfOutOfMemory;
        if ( cbfUnflushable >= statsCurrRun.cbfVisited )
        {
            Assert( cbfUnflushable == statsCurrRun.cbfVisited );
            if ( statsCurrRun.cbfVisited == statsCurrRun.cbfOutOfMemory )
            {
                errHang = ErrERRCheck( JET_errOutOfMemory );
            }
            else
            {
                errHang = ErrERRCheck( JET_errOutOfBuffers );
            }
        }
        else
        {
            errHang = ErrERRCheck( wrnSlow );
        }
    }

    statsCurrRun.errRun = errHang;
    statsCurrRun.tickEnd = TickOSTimeCurrent();

    g_iScavengeLastRun = IrrNext( g_iScavengeLastRun, g_cScavengeLastRuns );
    Assert( g_iScavengeLastRun >= 0 && g_iScavengeLastRun < g_cScavengeLastRuns );
    AssumePREFAST( g_iScavengeLastRun < g_cScavengeLastRuns );
    memcpy( &(g_rgScavengeLastRuns[g_iScavengeLastRun]), &statsCurrRun, sizeof(statsCurrRun) );

    if ( g_rgScavengeTimeSeq[g_iScavengeTimeSeqLast].tickEnd == 0 ||
            DtickDelta( g_rgScavengeTimeSeq[g_iScavengeTimeSeqLast].tickEnd, statsCurrRun.tickEnd ) > dtickMaintScavengeTimeSeqDelta )
    {
        g_iScavengeTimeSeqLast = IrrNext( g_iScavengeTimeSeqLast, g_cScavengeTimeSeq );
        Assert( g_iScavengeTimeSeqLast >= 0 && g_iScavengeTimeSeqLast < g_cScavengeTimeSeq );
        AssumePREFAST( g_iScavengeTimeSeqLast < g_cScavengeTimeSeq );
        memcpy( &(g_rgScavengeTimeSeq[g_iScavengeTimeSeqLast]), &statsCurrRun, sizeof(statsCurrRun) );
#ifndef RTM
        memcpy( &(g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast]), &statsCurrRun, sizeof(statsCurrRun) );
#endif
    }
    else
    {
        Assert( g_iScavengeTimeSeqLast < g_cScavengeTimeSeq );
        AssumePREFAST( g_iScavengeTimeSeqLast < g_cScavengeTimeSeq );

#ifndef RTM
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].iRun = statsCurrRun.iRun;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].tickEnd = statsCurrRun.tickEnd;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfCacheSize = max( g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfCacheSize, statsCurrRun.cbfCacheSize );
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfCacheTarget = max( g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfCacheTarget, statsCurrRun.cbfCacheTarget );
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfCacheSizeStartShrink = max( g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfCacheSizeStartShrink, statsCurrRun.cbfCacheSizeStartShrink );
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].dtickShrinkDuration = max( g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].dtickShrinkDuration, statsCurrRun.dtickShrinkDuration );
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfCacheDeadlock = max( g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfCacheDeadlock, statsCurrRun.cbfCacheDeadlock );
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfAvail = max( g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfAvail, statsCurrRun.cbfAvail );
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfAvailPoolLow = max( g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfAvailPoolLow, statsCurrRun.cbfAvailPoolLow );
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfAvailPoolHigh = max( g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfAvailPoolHigh, statsCurrRun.cbfAvailPoolHigh );
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfAvailPoolTarget = max( g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfAvailPoolTarget, statsCurrRun.cbfAvailPoolTarget );
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfVisited += statsCurrRun.cbfVisited;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfFlushed += statsCurrRun.cbfFlushed;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfEvictedAvailPool += statsCurrRun.cbfEvictedAvailPool;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfEvictedShrink += statsCurrRun.cbfEvictedShrink;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfShrinkFromAvailPool += statsCurrRun.cbfShrinkFromAvailPool;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfFlushPending += statsCurrRun.cbfFlushPending;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfFlushPendingSlow += statsCurrRun.cbfFlushPendingSlow;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfFlushPendingHung += statsCurrRun.cbfFlushPendingHung;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfFaultPending += statsCurrRun.cbfFaultPending;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfFaultPendingHung += statsCurrRun.cbfFaultPendingHung;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfOutOfMemory += statsCurrRun.cbfOutOfMemory;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfLatched += statsCurrRun.cbfLatched;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfDiskTilt += statsCurrRun.cbfDiskTilt;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfAbandoned += statsCurrRun.cbfAbandoned;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfDependent += statsCurrRun.cbfDependent;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfTouchTooRecent += statsCurrRun.cbfTouchTooRecent;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfPermanentErrs += statsCurrRun.cbfPermanentErrs;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].cbfHungIOs += statsCurrRun.cbfHungIOs;
        g_rgScavengeTimeSeqCumulative[g_iScavengeTimeSeqLast].errRun = ( statsCurrRun.errRun < JET_errSuccess ) ? statsCurrRun.errRun : 0;
#endif
    }

    ETCacheScavengeProgress(
            statsCurrRun.iRun,
            statsCurrRun.cbfVisited,
            statsCurrRun.cbfCacheSize,
            statsCurrRun.cbfCacheTarget,
            statsCurrRun.cbfCacheSizeStartShrink,
            statsCurrRun.dtickShrinkDuration,
            statsCurrRun.cbfAvail,
            statsCurrRun.cbfAvailPoolLow,
            statsCurrRun.cbfAvailPoolHigh,
            statsCurrRun.cbfFlushPending,
            statsCurrRun.cbfFlushPendingSlow,
            statsCurrRun.cbfFlushPendingHung,
            statsCurrRun.cbfOutOfMemory,
            statsCurrRun.cbfPermanentErrs,
            (INT)statsCurrRun.eStopReason,
            statsCurrRun.errRun );

    if ( errHang != JET_errSuccess )
    {
        OSTrace(
            JET_tracetagBufferManagerMaintTasks,
            OSFormat(   "BF:      ===== scan efficacy =========\r\n"
                        "BF:      iRun                     = %I64d\r\n"
                        "BF:      cbfVisited               = %I32d\r\n"
                        "BF:      cbfCacheSize             = %I32d\r\n"
                        "BF:      cbfCacheTarget           = %I32d\r\n"
                        "BF:      cbfCacheSizeStartShrink  = %I32d\r\n"
                        "BF:      dtickShrinkDuration      = %I32u\r\n"
                        "BF:      cbfAvail                 = %I32d\r\n"
                        "BF:      cbfAvailPoolLow          = %I32d\r\n"
                        "BF:      cbfAvailPoolHigh         = %I32d\r\n"
                        "BF:      cbfFlushPending          = %I32d\r\n"
                        "BF:      cbfFlushPendingSlow      = %I32d\r\n"
                        "BF:      cbfFlushPendingHung      = %I32d\r\n"
                        "BF:      cbfOutOfMemory           = %I32d\r\n"
                        "BF:      cbfPermanentErrs         = %I32d\r\n"
                        "BF:      eStopReason              = %d\r\n"
                        "BF:      errRun                   = %d\r\n",
                        statsCurrRun.iRun,
                        statsCurrRun.cbfVisited,
                        statsCurrRun.cbfCacheSize,
                        statsCurrRun.cbfCacheTarget,
                        statsCurrRun.cbfCacheSizeStartShrink,
                        statsCurrRun.dtickShrinkDuration,
                        statsCurrRun.cbfAvail,
                        statsCurrRun.cbfAvailPoolLow,
                        statsCurrRun.cbfAvailPoolHigh,
                        statsCurrRun.cbfFlushPending,
                        statsCurrRun.cbfFlushPendingSlow,
                        statsCurrRun.cbfFlushPendingHung,
                        statsCurrRun.cbfOutOfMemory,
                        statsCurrRun.cbfPermanentErrs,
                        (INT)statsCurrRun.eStopReason,
                        statsCurrRun.errRun ) );
    }


    if ( fOwnCacheResizeCritSec )
    {
        Assert( ( cbfCacheAddressableInitial != -1 ) && ( cbfCacheSizeInitial != -1 ) );

        BFICacheINotifyCacheSizeChanges( cbfCacheAddressableInitial, cbfCacheSizeInitial, cbfCacheAddressable, cbfCacheSize );

        BFICacheIShrinkAddressable();

        g_critCacheSizeResize.Leave();
        fOwnCacheResizeCritSec = fFalse;
    }

    BFICacheSetTarget( OnDebug( cbfCacheDeadlock ) );
    (void)ErrBFIMaintCacheSizeRequest();

    if ( cbfCacheSize >= cbfCacheDeadlock )
    {
        cbfCacheDeadlock = 0;
    }

    FOSSetCleanupState( fCleanUpStateSaved );

    return errHang;
}


void BFIMaintAvailPoolUpdateThresholds( const LONG_PTR cbfCacheTargetOptimalNew )
{

    Assert( g_critCacheSizeSetTarget.FOwner() );

    if (    !FDefaultParam( JET_paramCacheSizeMax ) &&
            FDefaultParam( JET_paramStartFlushThreshold ) &&
            FDefaultParam( JET_paramStopFlushThreshold ) )
    {
        cbfAvailPoolLow = LONG_PTR( QWORD( UlParam( JET_paramStartFlushThreshold ) ) * cbfCacheTargetOptimalNew / UlParamDefault( JET_paramCacheSizeMax ) );
        cbfAvailPoolHigh = LONG_PTR( QWORD( UlParam( JET_paramStopFlushThreshold ) ) * cbfCacheTargetOptimalNew / UlParamDefault( JET_paramCacheSizeMax ) );
    }
    else if ( !(    (   !FDefaultParam( JET_paramCacheSizeMax ) &&
                        !FDefaultParam( JET_paramStartFlushThreshold ) &&
                        !FDefaultParam( JET_paramStopFlushThreshold ) ) ||
                    (   FDefaultParam( JET_paramCacheSizeMax ) &&
                        FDefaultParam( JET_paramStartFlushThreshold ) &&
                        FDefaultParam( JET_paramStopFlushThreshold ) ) ) )
    {
        cbfAvailPoolLow = cbfCacheTargetOptimalNew / 100;
        cbfAvailPoolHigh = cbfCacheTargetOptimalNew / 50;
    }
    else
    {
        cbfAvailPoolLow = LONG_PTR( QWORD( UlParam( JET_paramStartFlushThreshold ) ) * cbfCacheTargetOptimalNew / UlParam( JET_paramCacheSizeMax ) );
        cbfAvailPoolHigh = LONG_PTR( QWORD( UlParam( JET_paramStopFlushThreshold ) ) * cbfCacheTargetOptimalNew / UlParam( JET_paramCacheSizeMax ) );
    }


    if ( cbfAvailPoolLow > cbfCacheTargetOptimalNew / 5 )
    {
        cbfAvailPoolLow = cbfCacheTargetOptimalNew / 5;
    }
    if ( cbfAvailPoolHigh > cbfCacheTargetOptimalNew / 4 )
    {
        cbfAvailPoolHigh = cbfCacheTargetOptimalNew / 4;
    }

    if ( cbfCacheTargetOptimalNew > 0 )
    {

        if ( cbfCacheTargetOptimalNew > 10000 )
        {
            if ( cbfAvailPoolLow < ( cbfCacheTargetOptimalNew / 1000 ) )
            {
                cbfAvailPoolLow = ( cbfCacheTargetOptimalNew / 1000 );
            }
            if ( cbfAvailPoolHigh < ( cbfCacheTargetOptimalNew / 500 ) )
            {
                cbfAvailPoolHigh = ( cbfCacheTargetOptimalNew / 500 );
            }
        }


        if ( cbfAvailPoolLow == cbfAvailPoolHigh )
        {
            cbfAvailPoolHigh += cbfCacheTargetOptimalNew / 100;
        }
        if ( cbfAvailPoolHigh == cbfCacheTargetOptimalNew )
        {
            cbfAvailPoolHigh = cbfCacheTargetOptimalNew - 1;
        }
        if ( cbfAvailPoolHigh - cbfAvailPoolLow < 1 )
        {
            cbfAvailPoolHigh = min( cbfCacheTargetOptimalNew - 1, cbfAvailPoolHigh + 1 );
        }
        if ( cbfAvailPoolHigh - cbfAvailPoolLow < 1 )
        {
            cbfAvailPoolLow = max( 1, cbfAvailPoolLow - 1 );
        }
        if ( cbfAvailPoolHigh - cbfAvailPoolLow < 1 )
        {
            cbfAvailPoolHigh = cbfAvailPoolLow;
        }
    }
    else
    {
        Assert( cbfCacheTargetOptimalNew == 0 );
        Expected( fFalse );
        cbfAvailPoolLow = 0;
        cbfAvailPoolHigh = 0;
    }

    Assert( cbfAvailPoolLow >= 0 );
    Assert( cbfAvailPoolHigh >= 0 );
    Assert( cbfAvailPoolLow <= cbfAvailPoolHigh );

    cbfAvailPoolTarget = ( cbfAvailPoolLow + cbfAvailPoolHigh + 1 ) / 2;
    Assert( cbfAvailPoolTarget >= cbfAvailPoolLow );
    Assert( cbfAvailPoolTarget <= cbfAvailPoolHigh );
}

void BFIMaintScavengeIReset()
{
    g_iScavengeLastRun = 0;
    memset( g_rgScavengeLastRuns, 0, g_cScavengeLastRuns * sizeof(g_rgScavengeLastRuns[ 0 ]) );

    g_iScavengeTimeSeqLast = 0;
    memset( g_rgScavengeTimeSeq, 0, g_cScavengeTimeSeq * sizeof(g_rgScavengeTimeSeq[ 0 ]) );
#ifndef RTM
    memset( g_rgScavengeTimeSeqCumulative, 0, g_cScavengeTimeSeq * sizeof(g_rgScavengeTimeSeqCumulative[ 0 ]) );
#endif
}

ERR ErrBFIMaintScavengePreInit(
    INT     K,
    double  csecCorrelatedTouch,
    double  csecTimeout,
    double  csecUncertainty,
    double  dblHashLoadFactor,
    double  dblHashUniformity,
    double  dblSpeedSizeTradeoff )
{
    Expected( K == 1 || K == 2 );

    switch ( g_bflruk.ErrInit(    K,
                                csecCorrelatedTouch,
                                csecTimeout,
                                csecUncertainty,
                                dblHashLoadFactor,
                                dblHashUniformity,
                                dblSpeedSizeTradeoff ) )
    {
        default:
            AssertSz( fFalse, "Unexpected error initializing BF LRUK Manager" );
        case BFLRUK::ERR::errOutOfMemory:
            return ErrERRCheck( JET_errOutOfMemory );
        case BFLRUK::ERR::errSuccess:
            break;
    }

    BFITraceResMgrInit( K,
                            csecCorrelatedTouch,
                            csecTimeout,
                            csecUncertainty,
                            dblHashLoadFactor,
                            dblHashUniformity,
                            dblSpeedSizeTradeoff );

    return JET_errSuccess;
}

ERR ErrBFIMaintScavengeInit( void )
{
    ERR err = JET_errSuccess;

    Assert( g_rgScavengeTimeSeq == NULL );
    Assert( g_rgScavengeLastRuns == NULL );
    Assert( g_rgScavengeTimeSeqCumulative == NULL );


    g_dtickMaintScavengeTimeout = (TICK)UlParam( JET_paramHungIOThreshold );
#ifdef DEBUG
    g_cScavengeTimeSeq = ( 2 * g_dtickMaintScavengeTimeout ) / dtickMaintScavengeTimeSeqDelta + 3 ;
#else
    g_cScavengeTimeSeq = g_dtickMaintScavengeTimeout / dtickMaintScavengeTimeSeqDelta + 2 ;
#endif


    g_cScavengeTimeSeq = UlpFunctionalMax( g_cScavengeTimeSeq, 10 );
    g_cScavengeTimeSeq = UlpFunctionalMin( g_cScavengeTimeSeq, 100 );

    Alloc( g_rgScavengeTimeSeq = new BFScavengeStats[ g_cScavengeTimeSeq ] );
    Alloc( g_rgScavengeLastRuns = new BFScavengeStats[ g_cScavengeLastRuns ] );
#ifndef RTM
    Alloc( g_rgScavengeTimeSeqCumulative = new BFScavengeStats[ g_cScavengeTimeSeq ] );
#endif

    BFIMaintScavengeIReset();

HandleError:

    if ( err < JET_errSuccess )
    {
        BFIMaintScavengeTerm();
    }

    return err;
}

void BFIMaintScavengeTerm( void )
{
#ifndef RTM
    delete[] g_rgScavengeTimeSeqCumulative;
    g_rgScavengeTimeSeqCumulative = NULL;
#endif

    delete[] g_rgScavengeLastRuns;
    g_rgScavengeLastRuns = NULL;

    delete[] g_rgScavengeTimeSeq;
    g_rgScavengeTimeSeq = NULL;
}



CSemaphore      g_semMaintCheckpointDepthRequest( CSyncBasicInfo( _T( "g_semMaintCheckpointDepthRequest" ) ) );

IFMP            g_ifmpMaintCheckpointDepthStart;
ERR             errLastCheckpointMaint = JET_errSuccess;

POSTIMERTASK    g_posttBFIMaintCheckpointDepthITask = NULL;

#define dtickBFMaintNever   0xFFFFFFFF



void BFIMaintCheckpointDepthRequest( FMP * pfmp, const BFCheckpointDepthMainReason eRequestReason )
{

    if ( eRequestReason != bfcpdmrRequestRemoveCleanEntries )
    {


        if ( Ptls()->fCheckpoint )
        {
            return;
        }


        pfmp->EnterBFContextAsReader();

        if ( eRequestReason == bfcpdmrRequestOB0Movement )
        {
            BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext();
            if ( !pbffmp || !pbffmp->fCurrentlyAttached )
            {
                pfmp->LeaveBFContextAsReader();
                return;
            }

            if ( TickCmp( TickOSTimeCurrent(), pbffmp->tickMaintCheckpointDepthNext ) < 0 )
            {
                pfmp->LeaveBFContextAsReader();
                return;
            }
        }
        else if ( eRequestReason == bfcpdmrRequestIOThreshold )
        {


            if ( errLastCheckpointMaint != errDiskTilt )
            {
                pfmp->LeaveBFContextAsReader();
                return;
            }


            BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext();
            if ( pbffmp && pbffmp->fCurrentlyAttached )
            {
                pbffmp->tickMaintCheckpointDepthNext = TickOSTimeCurrent();
            }
        }
        else if ( eRequestReason == bfcpdmrRequestConsumeSettings )
        {

            BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext();
            if ( pbffmp && pbffmp->fCurrentlyAttached )
            {
                pbffmp->tickMaintCheckpointDepthNext = TickOSTimeCurrent();
            }
        }
        else
        {
            AssertSz( fFalse, "Unknown reason (%d) for CP depth maintenance.", eRequestReason );
        }

        pfmp->LeaveBFContextAsReader();
    }


    BOOL fAcquiredAsync = g_semMaintCheckpointDepthRequest.FTryAcquire();


    if ( fAcquiredAsync )
    {

        OSTrace( JET_tracetagBufferManagerMaintTasks, OSFormat( "Scheduling BFIMaintCheckpointDepthITask immediately (%d).", eRequestReason ) );

        if ( ErrBFIMaintScheduleTask( g_posttBFIMaintCheckpointDepthITask, NULL, 0, 0 ) >= JET_errSuccess )
        {

            fAcquiredAsync = fFalse;
        }
    }


    if ( fAcquiredAsync )
    {
        Assert( g_semMaintCheckpointDepthRequest.CAvail() == 0 );
        g_semMaintCheckpointDepthRequest.Release();
    }

}


void BFIMaintCheckpointDepthITask( void*, void* )
{
    OSTrace( JET_tracetagBufferManagerMaintTasks, __FUNCTION__ );


    BOOL fAcquiredAsync = fTrue;


    TICK dtickNextSchedule = 0;
    BFIMaintCheckpointDepthIFlushPages( &dtickNextSchedule );



    if ( dtickNextSchedule != dtickBFMaintNever )
    {
        Assert( dtickNextSchedule <= 60000 );


#ifdef BF_REALTIME_CHECKPOINT_RESCHEDULING
        const TICK dtickNextScheduleActual = dtickNextSchedule;
#else
        const TICK dtickNextScheduleActual = dtickMaintCheckpointDepthRetry;
#endif

        OSTrace( JET_tracetagBufferManagerMaintTasks,
                OSFormat( "Scheduling BFIMaintCheckpointDepthITask for %u ticks in future (next estimated is in %u ticks).", dtickNextScheduleActual, dtickNextSchedule ) );

        if ( ErrBFIMaintScheduleTask(   g_posttBFIMaintCheckpointDepthITask,
                                        NULL,
                                        dtickNextScheduleActual,
                                        0 ) >= JET_errSuccess )
        {

            fAcquiredAsync = fFalse;
        }
    }


    if ( fAcquiredAsync )
    {
        Assert( g_semMaintCheckpointDepthRequest.CAvail() == 0 );
        g_semMaintCheckpointDepthRequest.Release();
    }
}


void BFIMaintCheckpointDepthIFlushPages( TICK * pdtickNextSchedule )
{
    ERR             err         = JET_errSuccess;
    CBFIssueList    bfil;
    const IFMP      ifmpMin     = FMP::IfmpMinInUse();
    const IFMP      ifmpMac     = FMP::IfmpMacInUse();
    IFMP            ifmpStart   = g_ifmpMaintCheckpointDepthStart;
    IFMP            ifmp;

    OSTrace(    JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "Beginning Checkpoint Depth Maint at ifmpStart=0x%x (ifmpMin=0x%x, g_ifmpMax=0x%x)",
                            (ULONG)ifmpStart, (ULONG)ifmpMin, (ULONG)ifmpMac ) );

    Assert( pdtickNextSchedule );
    *pdtickNextSchedule = dtickBFMaintNever;

    if ( ifmpMin > ifmpMac )
    {
        *pdtickNextSchedule = dtickBFMaintNever;
        return;
    }


    if ( ifmpStart < ifmpMin || ifmpStart > ifmpMac )
    {
        ifmpStart = ifmpMin;
    }

    Assert( !Ptls()->fCheckpoint );
    Ptls()->fCheckpoint = fTrue;

    ifmp = ifmpStart;
    do
    {
        Assert( ifmp >= ifmpMin );
        Assert( ifmp <= ifmpMac );

        FMP * pfmp = &g_rgfmp[ifmp];
        if ( !pfmp->FBFContext() )
        {
            FMP::EnterFMPPoolAsWriter();
            pfmp->RwlDetaching().EnterAsReader();
            if ( pfmp->FAttached() && !pfmp->FDetachingDB() && !pfmp->FBFContext() )
            {
                (VOID)ErrBFISetupBFFMPContext( ifmp );
            }
            pfmp->RwlDetaching().LeaveAsReader();
            FMP::LeaveFMPPoolAsWriter();
        }


        pfmp->EnterBFContextAsReader();
        BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext();
        err = JET_errSuccess;
        BOOL fUpdateCheckpoint = fFalse;
        size_t ipinstCheckpointUpdate = INT_MAX;

        if ( pbffmp && pbffmp->fCurrentlyAttached )
        {
            const TICK tickNow = TickOSTimeCurrent();

            const TICK tickNextMaint = pbffmp->tickMaintCheckpointDepthNext;
            const LONG dtickNextMaint = DtickDelta( tickNow, tickNextMaint );

            if ( dtickNextMaint <= 0 )
            {


                if ( g_rgfmp[ ifmp ].DwBFContext() )
                {
                    err = ErrBFIMaintCheckpointDepthIFlushPagesByIFMP( ifmp, &fUpdateCheckpoint );
                    if ( err != JET_errSuccess )
                    {
                        *pdtickNextSchedule = dtickMaintCheckpointDepthRetry;
                        pbffmp->tickMaintCheckpointDepthNext = TickOSTimeCurrent() + dtickMaintCheckpointDepthRetry;
                    }
                    else
                    {
                        pbffmp->tickMaintCheckpointDepthNext = TickOSTimeCurrent() + dtickMaintCheckpointDepthDelay;
                    }
                }

                pbffmp->tickMaintCheckpointDepthLast = TickOSTimeCurrent();
            }
            else
            {

                OSTrace( JET_tracetagBufferManagerMaintTasks, OSFormat( "CP: Skipping due to time not yet elapsed (%u >= %u).",
                            tickNextMaint, tickNow ) );


#ifdef BF_REALTIME_CHECKPOINT_RESCHEDULING

                if ( ( *pdtickNextSchedule == dtickBFMaintNever ) || ( dtickNextMaint < (LONG)( *pdtickNextSchedule ) ) )
                {
                    const TICK dtickNextScheduleOld = *pdtickNextSchedule;
                    *pdtickNextSchedule = (TICK)dtickNextMaint;

                    OSTrace( JET_tracetagBufferManagerMaintTasks, OSFormat( "CP: Changing dtickNextSchedule schedule from %u to %u.",
                                dtickNextScheduleOld, *pdtickNextSchedule ) );
                }
#endif
            }

            
            ipinstCheckpointUpdate = IpinstFromPinst( PinstFromIfmp( ifmp ) );
            Assert( PinstFromIfmp( ifmp ) == g_rgpinst[ipinstCheckpointUpdate] );
        }

        pfmp->LeaveBFContextAsReader();

        Assert( !fUpdateCheckpoint || ipinstCheckpointUpdate != INT_MAX );
        if ( fUpdateCheckpoint )
        {
            OSTrace( JET_tracetagBufferManagerMaintTasks, "CPUPD: Triggering checkpoint update from checkpoint maintenance." );
            BFIMaintCheckpointIUpdateInst( ipinstCheckpointUpdate );
        }

        if ( err == errDiskTilt )
        {
            
            errLastCheckpointMaint = err;
            break;
        }

        ifmp++;
        if ( ifmp > ifmpMac )
        {
            ifmp = ifmpMin;
        }

    }
    while ( ifmp != ifmpStart );


    if ( err == errDiskTilt )
    {
        g_ifmpMaintCheckpointDepthStart = ifmp + 1;
    }
    else
    {
        g_ifmpMaintCheckpointDepthStart = 0;
    }

    OSTrace(    JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "Next Checkpoint Depth Maint will begin at g_ifmpMaintCheckpointDepthStart=0x%x in %u ms",
                            (ULONG)g_ifmpMaintCheckpointDepthStart,
                            *pdtickNextSchedule ) );


    (void)bfil.ErrIssue();


    Ptls()->fCheckpoint = fFalse;

    Assert( *pdtickNextSchedule == dtickBFMaintNever || *pdtickNextSchedule < INT_MAX );
}



const static ULONG g_cbCheckpointInterval = 512 * 1024;
C_ASSERT( g_cbCheckpointInterval >= cbCheckpointTooDeepUncertainty );

INT IUrgentBFIMaintCheckpointPriority(
    __in const LOG * const      plog,
    __in const LGPOS&           lgposNewest,
    __in const QWORD            cbCheckpointDepth,
    __in const LGPOS            lgposOldestBegin0 )
{
    INT     iUrgentLevel;

    Assert( 0 != CmpLgpos( lgposMin, lgposNewest ) );
    Assert( 0 != CmpLgpos( lgposMin, lgposOldestBegin0 ) );


    const QWORD     cbCurrentCheckpoint     = plog->CbLGOffsetLgposForOB0( lgposNewest, lgposOldestBegin0 );



    const QWORD     cbUrgentCheckpoint  = cbCheckpointDepth > 0 ?
                                            cbCheckpointDepth + cbCheckpointDepth / 4 :
                                            plog->CbLGSec() * plog->CSecLGFile();

    Assert( cbCheckpointDepth <= cbUrgentCheckpoint );


    if ( cbCurrentCheckpoint < cbUrgentCheckpoint )
    {


        iUrgentLevel = 0;
    }
    else
    {


        const QWORD cbCheckpointOverdue = cbCurrentCheckpoint - cbUrgentCheckpoint;

        iUrgentLevel = (INT)( cbCheckpointOverdue / g_cbCheckpointInterval + 1 );

        iUrgentLevel = UlBound( iUrgentLevel, qosIODispatchUrgentBackgroundLevelMin, qosIODispatchUrgentBackgroundLevelMax );
    }

    return iUrgentLevel;
}


OSFILEQOS QosBFIMaintCheckpointPriority(
    __in const INST * const     pinst,
    __in const LGPOS&           lgposNewest,
    __in const QWORD            cbCheckpointDepth,
    __in const LGPOS            lgposOldestBegin0 )
{
    OSFILEQOS       qosIO;

    Assert( 0 != CmpLgpos( lgposMin, lgposNewest ) );
    Assert( 0 != CmpLgpos( lgposMin, lgposOldestBegin0 ) );

    const INT iUrgentLevel = IUrgentBFIMaintCheckpointPriority( pinst->m_plog, lgposNewest, cbCheckpointDepth, lgposOldestBegin0 );

    if ( 0 == iUrgentLevel )
    {

        qosIO = qosIODispatchBackground;
    }
    else
    {

        qosIO = QosOSFileFromUrgentLevel( iUrgentLevel );
    }

    if ( JET_IOPriorityLowForCheckpoint & UlParam( pinst, JET_paramIOPriority ) )
    {
        qosIO |= qosIOOSLowPriority;
    }

    return qosIO;
}

OSFILEQOS QosBFIMaintCheckpointQuiescePriority()
{
    OSFILEQOS       qosIO;

    const ULONG_PTR cioMax = UlParam( JET_paramOutstandingIOMax );

    if ( cioMax > 256 )
    {
        qosIO = QosOSFileFromUrgentLevel( 25 );
    }
    else if ( cioMax <= 32 )
    {
        qosIO = QosOSFileFromUrgentLevel( 64 );
    }
    else if ( cioMax <= 64 )
    {
        qosIO = QosOSFileFromUrgentLevel( 50 );
    }
    else if ( cioMax <= 128 )
    {
        qosIO = QosOSFileFromUrgentLevel( 38 );
    }
    else if ( cioMax <= 256 )
    {
        qosIO = QosOSFileFromUrgentLevel( 33 );
    }
    else
    {
        AssertSz( fFalse, "Somehow we didn't cover this cioMax = %d ??", cioMax );
    }


    qosIO = ( qosIO | qosIOOSLowPriority );

    return qosIO;
}





ERR ErrBFIOB0MaintEntry(
    IFMP                        ifmp,
    BFFMPContext *              pbffmp,
    BF *                        pbf,
    BFOB0::CLock *              plockOnOB0,
    __in const LGPOS&           lgposNewest,
    __in const QWORD            cbCheckpointDepth,
    const BFOB0MaintOperations  fOperations
    )
{
    ERR err     = JET_errSuccess;

    Assert( plockOnOB0 || pbffmp->critbfob0ol.FOwner() );

    pbffmp->ChkAdvData.cEntriesVisited++;

    if ( fOperations & bfob0moCleaning )
    {
        if (    pbf->bfdf == bfdfClean &&
                pbf->sxwl.ErrTryAcquireExclusiveLatch() == CSXWLatch::ERR::errSuccess )
        {


            if ( pbf->bfdf == bfdfClean )
            {
                if ( plockOnOB0 )
                {

                    BFOB0::ERR      errOB0;
                    errOB0 = pbffmp->bfob0.ErrDeleteEntry( plockOnOB0 );
                    Assert( errOB0 == BFOB0::ERR::errSuccess );

                    pbf->lgposOldestBegin0  = lgposMax;
                }
                else
                {

                    pbf->fInOB0OL = fFalse;

                    pbffmp->bfob0ol.Remove( pbf );

                    pbf->lgposOldestBegin0  = lgposMax;
                }
                pbffmp->ChkAdvData.cCleanRemoved++;
                pbf->sxwl.ReleaseExclusiveLatch();

                goto HandleError;
            }
            pbf->sxwl.ReleaseExclusiveLatch();
        }
    }

    if ( fOperations & bfob0moFlushing )
    {

        Assert( fOperations & bfob0moCleaning );

        Assert( fOperations & bfob0moVersioning );


        if ( CmpLgpos( &pbf->lgposOldestBegin0, &lgposMax ) )
        {


            const OSFILEQOS qosIO = ( fOperations & bfob0moQuiescing ) ?
                                        QosBFIMaintCheckpointQuiescePriority() :
                                        QosBFIMaintCheckpointPriority( PinstFromIfmp( ifmp ), lgposNewest, cbCheckpointDepth, pbf->lgposOldestBegin0 );

            if ( qosIO != qosIODispatchBackground )
            {
                OSTrace( JET_tracetagBufferManager, OSFormat( "Checkpoint advancement falling behind, escalating BF page=[0x%x:0x%x] flush to 0x%I64x", (ULONG)pbf->ifmp, pbf->pgno, qosIO ) );
            }


            err = ErrBFIFlushPage( pbf, IOR( iorpBFCheckpointAdv ), qosIO );

            switch( err )
            {
                case errBFIPageFlushed:
                    pbffmp->ChkAdvData.cFlushErrPageFlushed++;
                    break;
                case errBFIPageFlushPending:
                case errBFIPageFlushPendingSlowIO:
                case errBFIPageFlushPendingHungIO:
                    pbffmp->ChkAdvData.cFlushErrPageFlushPending++;
                    break;
                case errBFIRemainingDependencies:
                    pbffmp->ChkAdvData.cFlushErrRemainingDependencies++;
                    break;
                case errBFIDependentPurged:
                    pbffmp->ChkAdvData.cFlushErrDependentPurged++;
                    break;
                case errBFLatchConflict:
                    BFIFlagDependenciesImpeding( pbf );
                    pbffmp->ChkAdvData.cFlushErrLatchConflict++;
                    break;
                case errBFIPageTouchTooRecent:
                    pbffmp->ChkAdvData.cFlushErrPageTouchTooRecent++;
                    break;
                case errBFIPageFlushDisallowedOnIOThread:
                    AssertSz( fFalse, "Shouldn't see errBFIPageFlushDisallowedOnIOThread in " __FUNCTION__ );
                    break;
                case JET_errSuccess:
                    pbffmp->ChkAdvData.cFlushErrSuccess++;
                    break;
                default:
                    pbffmp->ChkAdvData.cFlushErrOther++;
                    break;
            }
            Call( err );
        }
    }
    else if ( ( fOperations & bfob0moVersioning ) && !( fOperations & bfob0moFlushing ) )
    {
        if ( CmpLgpos( &pbf->lgposOldestBegin0, &lgposMax ) )
        {
            BFIFlagDependenciesImpeding( pbf );
        }
    }

HandleError:

    return err;
}


ERR ErrBFIOB0MaintScan(
    const IFMP                  ifmp,
    BFFMPContext * const        pbffmp,
    __in const LGPOS&           lgposNewest,
    __in const QWORD            cbCheckpointDepth,
    BFOB0::CLock * const        plockOB0,
    LGPOS                       lgposStartBM,
    __inout LGPOS * const       plgposStopBM,
    __out LGPOS * const         plgposForwardFlushProgressBM,
    enum BFOB0MaintOperations   fOperations
    )
{
    ERR         err             = JET_errSuccess;
    BOOL        fTracedInitBM   = fFalse;
    BOOL        fHadFlushErr    = fFalse;
    BOOL        fSetUrgentCtr   = 0 != CmpLgpos( lgposMin, lgposStartBM );

    Assert( fOperations );
    Assert( plgposStopBM );
    Assert( lgposStartBM.lGeneration >= 0 );
    Assert( plgposStopBM->lGeneration >= 0 );
    Assert( CmpLgpos( *plgposStopBM, lgposMax ) != 0 );

    if ( plgposForwardFlushProgressBM )
    {
        *plgposForwardFlushProgressBM = lgposMin;
    }

    OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks,
        OSFormat(   "%s ( ifmp=0x%x, lgposStartBM=%08x:%04x:%04x, *plgposStopBM=%08x:%04x:%04x, %s%s%s )",
                    __FUNCTION__,
                    (ULONG)ifmp,
                    lgposStartBM.lGeneration, lgposStartBM.isec, lgposStartBM.ib,
                    plgposStopBM->lGeneration, plgposStopBM->isec, plgposStopBM->ib,
                    fOperations & bfob0moCleaning   ? "Clean" : "",
                    fOperations & bfob0moFlushing   ? "Flush" : "",
                    fOperations & bfob0moVersioning ? "Versn" : ""
                    ) );

    *plgposStopBM = BFIOB0Lgpos( ifmp, *plgposStopBM, fTrue );

    if ( CmpLgpos( lgposStartBM, *plgposStopBM ) >= 0 )
    {
        OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks,
            OSFormat(   "%s start/end BMs same, nothing to do. [ifmp=0x%x]", __FUNCTION__, (ULONG)ifmp ) );
        if ( plgposForwardFlushProgressBM )
        {
            *plgposForwardFlushProgressBM = *plgposStopBM;
        }
        return( JET_errSuccess );
    }

    if ( 0 == CmpLgpos( lgposStartBM, lgposMin ) )
    {
        pbffmp->bfob0.MoveBeforeFirst( plockOB0 );
    }
    else
    {
        pbffmp->bfob0.MoveBeforeKeyPtr( BFIOB0Offset( ifmp, &lgposStartBM ), NULL, plockOB0 );
    }

    while ( pbffmp->bfob0.ErrMoveNext( plockOB0 ) != BFOB0::ERR::errNoCurrentEntry )
    {
        PBF pbf;
        BFOB0::ERR      errOB0;
        errOB0 = pbffmp->bfob0.ErrRetrieveEntry( plockOB0, &pbf );
        Assert( errOB0 == BFOB0::ERR::errSuccess );

        LGPOS lgposOldestBegin0 = BFIOB0Lgpos( ifmp, pbf->lgposOldestBegin0 );

        if ( !fTracedInitBM )
        {
            OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "%s actual starting lgposStartBM=%08x:%04x:%04x [ifmp=0x%x]",
                            __FUNCTION__,
                            lgposOldestBegin0.lGeneration,
                            lgposOldestBegin0.isec,
                            lgposOldestBegin0.ib,
                            (ULONG)ifmp ) );
            fTracedInitBM = fTrue;
        }

        if ( !fSetUrgentCtr )
        {
            if ( fOperations & bfob0moFlushing )
            {
                const INT iUrgentLevelWorst = IUrgentBFIMaintCheckpointPriority( PinstFromIfmp( pbf->ifmp )->m_plog, lgposNewest, cbCheckpointDepth, lgposOldestBegin0 );
                if ( iUrgentLevelWorst )
                {
                    PERFOptDeclare( const INT cioOutstandingMax = CioOSDiskPerfCounterIOMaxFromUrgentQOS( QosOSFileFromUrgentLevel( iUrgentLevelWorst ) ) );
                    PERFOpt( cBFCheckpointMaintOutstandingIOMax.Set( PinstFromIfmp( pbf->ifmp ), cioOutstandingMax ) );
                }
                else
                {
                    PERFOpt( cBFCheckpointMaintOutstandingIOMax.Set( PinstFromIfmp( pbf->ifmp ), 1 ) );
                }
            }
            fSetUrgentCtr = fTrue;
        }

        if ( CmpLgpos( &lgposOldestBegin0, plgposStopBM ) >= 0 )
        {
            break;
        }


        err = ErrBFIOB0MaintEntry( ifmp, pbffmp, pbf, plockOB0, lgposNewest, cbCheckpointDepth, fOperations );


        if ( plgposForwardFlushProgressBM )
        {
            Expected( fOperations & bfob0moFlushing || fOperations & bfob0moCleaning  );

            if ( !fHadFlushErr )
            {
                *plgposForwardFlushProgressBM = lgposOldestBegin0;
                if ( err != JET_errSuccess &&
                      err != errBFIPageTouchTooRecent )
                {
                    fHadFlushErr = fTrue;
                }
            }
        }

        if ( err == errDiskTilt )
        {
            break;
        }

    }

    pbffmp->bfob0.UnlockKeyPtr( plockOB0 );

    pbffmp->critbfob0ol.Enter();
    PBF pbfNext;
    ULONG cIterations = 0;
    for ( PBF pbf = pbffmp->bfob0ol.PrevMost(); pbf != pbfNil; pbf = pbfNext )
    {
        pbfNext = pbffmp->bfob0ol.Next( pbf );

        if ( CmpLgpos( &pbf->lgposOldestBegin0, plgposStopBM ) < 0 )
        {


            err = ErrBFIOB0MaintEntry( ifmp, pbffmp, pbf, NULL, lgposNewest, cbCheckpointDepth, fOperations );

            if ( err == errDiskTilt )
            {
                break;
            }
        }

        Assert( cIterations++ < 50 );
    }

    pbffmp->critbfob0ol.Leave();

    if ( plgposForwardFlushProgressBM &&
        0 == CmpLgpos( plgposForwardFlushProgressBM, &lgposMin ) )
    {
        OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks,
            OSFormat(   "%s Visited no entries. [ifmp=0x%x]", __FUNCTION__, (ULONG)ifmp ) );
        *plgposForwardFlushProgressBM = *plgposStopBM;
    }

    OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks,
        OSFormat(   "%s actual stopping plgposStopBM=%08x:%04x:%04x [ifmp=0x%x]",
                    __FUNCTION__,
                    plgposStopBM->lGeneration,
                    plgposStopBM->isec,
                    plgposStopBM->ib,
                    (ULONG)ifmp ) );

    return err;
}









BOOL g_fBFOverscanCheckpoint = fTrue;
BOOL g_fBFEnableForegroundCheckpointMaint = fTrue;


ERR ErrBFIMaintCheckpointDepthIFlushPagesByIFMP( const IFMP ifmp, BOOL * const pfUpdateCheckpoint )
{
    ERR  err  = JET_errSuccess;
    FMP* pfmp = &g_rgfmp[ ifmp ];

    Assert( ifmp != ifmpNil );
    Assert( pfUpdateCheckpoint );


    BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext();
    if ( !pbffmp || !pbffmp->fCurrentlyAttached )
    {
        AssertSz( fFalse, "Caller should've protected us from being called when there is no pbffmp (%p), or it is not attached", pbffmp );
        return JET_errSuccess;
    }


    memset( &(pbffmp->ChkAdvData), 0, sizeof(pbffmp->ChkAdvData) );
    

    LOG* const  plog        = pfmp->Pinst()->m_plog;
    const LGPOS lgposNewest = plog->LgposLGLogTipNoLock();
    if( 0 == CmpLgpos( lgposMin, lgposNewest ) )
    {
        OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks,
            OSFormat(   "CP [ifmp=0x%x]: Skipping checkpoint maintenance. fRecoveringMode = %d", (ULONG)ifmp, plog->FRecoveringMode() ) );
        return err;
    }


    const BOOL fActivateTheReFlusher = ( pbffmp->lgposLastLogTip.lGeneration < lgposNewest.lGeneration );
    pbffmp->lgposLastLogTip = lgposNewest;


    LGPOS lgposCPD = lgposMin;
    const BOOL fQuiesceCheckpoint = pfmp->Pinst()->m_fCheckpointQuiesce;

    const BOOL fUrgentCheckpointUpdate = (BOOL)UlConfigOverrideInjection( 46336, fFalse ) || fQuiesceCheckpoint;

    const QWORD cbCheckpointDepth = fQuiesceCheckpoint ? 0 : (QWORD)pfmp->Pinst()->m_plog->CbLGDesiredCheckpointDepth();
    lgposCPD.lGeneration = (LONG)( cbCheckpointDepth / ( 1024 * (QWORD)UlParam( pfmp->Pinst(), JET_paramLogFileSize ) ) );
    LGPOS lgposPreferredCheckpoint = lgposNewest;
    if ( lgposNewest.lGeneration - lgposCPD.lGeneration > 0 )
    {
        lgposPreferredCheckpoint.lGeneration = lgposNewest.lGeneration - lgposCPD.lGeneration;
    }
    else
    {
        lgposPreferredCheckpoint = lgposMin;
    }



    LGPOS lgposCheckpointOverscan       = lgposPreferredCheckpoint;
    LGPOS lgposWaypointLatency          = lgposMin;
    lgposWaypointLatency.lGeneration    = (LONG)UlParam( pfmp->Pinst(), JET_paramWaypointLatency );
    if ( lgposWaypointLatency.lGeneration > 0 )
    {
        if ( lgposPreferredCheckpoint.lGeneration + lgposWaypointLatency.lGeneration < ( lgposNewest.lGeneration - ( 1 + lgposCPD.lGeneration / 5 ) ) &&
             lgposPreferredCheckpoint.lGeneration + lgposWaypointLatency.lGeneration > 0 )
        {
            lgposCheckpointOverscan.lGeneration = lgposPreferredCheckpoint.lGeneration + lgposWaypointLatency.lGeneration;
        }
    }
    Assert(lgposCheckpointOverscan.lGeneration >= 0 );

    OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks,
        OSFormat(   "CP: Checkpoint Advancement begins [ifmp=0x%x], plan:\r\n"
                    "CP:    lgposPreferredCheckpoint  =%08x:%04x:%04x (fQuiesce = %d, fUrgent = %d)\r\n"
                    "CP:    lgposFlusherBM            =%08x:%04x:%04x\r\n"
                    "CP:    lgposVersionerBM          =%08x:%04x:%04x\r\n"
                    "CP:    lgposCheckpointOverscan   =%08x:%04x:%04x\r\n"
                    "CP:    fActivateTheReFlusher     =%s",
                    (ULONG)ifmp,
                    lgposPreferredCheckpoint.lGeneration, lgposPreferredCheckpoint.isec, lgposPreferredCheckpoint.ib, fQuiesceCheckpoint, fUrgentCheckpointUpdate,
                    pbffmp->lgposFlusherBM.lGeneration, pbffmp->lgposFlusherBM.isec, pbffmp->lgposFlusherBM.ib,
                    pbffmp->lgposVersionerBM.lGeneration, pbffmp->lgposVersionerBM.isec, pbffmp->lgposVersionerBM.ib,
                    lgposCheckpointOverscan.lGeneration, lgposCheckpointOverscan.isec, lgposCheckpointOverscan.ib,
                    fActivateTheReFlusher ? "yes" : "no"
                    ) );


    CFlushMap* const pfm = pfmp->PFlushMap();
    if ( pfm != NULL )
    {
        const QWORD ibLogTip = ( (QWORD)lgposNewest.lGeneration * pfmp->Pinst()->m_plog->CSecLGFile() + lgposNewest.isec ) * pfmp->Pinst()->m_plog->CbLGSec() + lgposNewest.ib;
        const QWORD cbPreferredChktpDepth = (QWORD)UlParam( pfmp->Pinst(), JET_paramCheckpointDepthMax );
        if ( pfm->FRequestFmSectionWrite( ibLogTip, cbPreferredChktpDepth ) )
        {
            pfm->FlushOneSection( ibLogTip );
        }
    }





    if ( g_fBFOverscanCheckpoint &&
        CmpLgpos( lgposCheckpointOverscan, lgposPreferredCheckpoint ) &&
        CmpLgpos( BFIOB0Lgpos( ifmp, pbffmp->lgposVersionerBM ), BFIOB0Lgpos( ifmp, lgposCheckpointOverscan ) )
        )
    {
        BFOB0::CLock    lockOB0Pass1;

        Assert( CmpLgpos( lgposCheckpointOverscan, lgposPreferredCheckpoint) > 0 );

        CallS( ErrBFIOB0MaintScan( ifmp, pbffmp,
                            lgposMin, 0,
                            &lockOB0Pass1,
                            pbffmp->lgposVersionerBM,
                            &lgposCheckpointOverscan,
                            NULL,
                            BFOB0MaintOperations( bfob0moVersioning | bfob0moCleaning ) ) );

        OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks,
            OSFormat(   "CP [ifmp=0x%x]: Pass 1: Versioner: start=%08x,%04x,%04x - stopBM=%08x,%04x,%04x",
                    (ULONG)ifmp,
                    pbffmp->lgposVersionerBM.lGeneration, pbffmp->lgposVersionerBM.isec, pbffmp->lgposVersionerBM.ib,
                    lgposCheckpointOverscan.lGeneration, lgposCheckpointOverscan.isec, lgposCheckpointOverscan.ib
                    ) );

        pbffmp->lgposVersionerBM = lgposCheckpointOverscan;

        if ( FOSTraceTagEnabled( JET_tracetagBufferManagerMaintTasks ) )
        {
            char rgOB0Stats[lockOB0Pass1.cchStatsString];
            lockOB0Pass1.SPrintStats( rgOB0Stats );
            OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "CP [ifmp=0x%x]: Pass 1: Versioner - OB0 Index stats: %s", (ULONG)ifmp, rgOB0Stats ) );
        }
    }


    BFOB0::CLock    lockOB0Pass2N;

    LGPOS lgposFlushStart;
    if ( !fActivateTheReFlusher )
    {
        lgposFlushStart = pbffmp->lgposFlusherBM;
    }
    else
    {
        lgposFlushStart = lgposMin;
    }


    LGPOS lgposFlushStop = lgposPreferredCheckpoint;
    LGPOS lgposFlushForwardProgress = lgposMin;
    err = ErrBFIOB0MaintScan( ifmp, pbffmp,
                        lgposNewest,
                        cbCheckpointDepth,
                        &lockOB0Pass2N,
                        lgposFlushStart,
                        &lgposFlushStop ,
                        &lgposFlushForwardProgress,
                        BFOB0MaintOperations( bfob0moFlushing | bfob0moVersioning | bfob0moCleaning | ( fQuiesceCheckpoint ? bfob0moQuiescing : 0 ) ) );
    Assert( CmpLgpos( lgposFlushForwardProgress, lgposMin ) );
    pbffmp->lgposFlusherBM = lgposFlushForwardProgress;

    OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks,
        OSFormat(   "CP [ifmp=0x%x]: Pass %c: Flusher: start=%08x,%04x,%04x - stopBM=%08x,%04x,%04x",
                (ULONG)ifmp, fActivateTheReFlusher ? 'N' : '2',
                lgposFlushStart.lGeneration, lgposFlushStart.isec, lgposFlushStart.ib,
                lgposFlushStop.lGeneration, lgposFlushStop.isec, lgposFlushStop.ib
                ) );

    if ( FOSTraceTagEnabled( JET_tracetagBufferManagerMaintTasks ) )
    {
        char rgOB0Stats[lockOB0Pass2N.cchStatsString];
        lockOB0Pass2N.SPrintStats( rgOB0Stats );
        OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks,
            OSFormat(   "CP [ifmp=0x%x]: Pass %c: Flusher - OB0 Index stats: %s", (ULONG)ifmp, fActivateTheReFlusher ? 'N' : '2', rgOB0Stats ) );
    }

    if ( err != JET_errSuccess &&
        err != errDiskTilt )
    {
        if ( pbffmp->ChkAdvData.cFlushErrPageFlushed ||
                pbffmp->ChkAdvData.cFlushErrPageFlushPending ||
                pbffmp->ChkAdvData.cFlushErrRemainingDependencies ||
                pbffmp->ChkAdvData.cFlushErrDependentPurged ||
                pbffmp->ChkAdvData.cFlushErrLatchConflict )
        {
            OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks, OSFormat( "CP [ifmp=0x%x]: work remaining b/c flushes pending, dependencies, etc", (ULONG)ifmp ) );
            err = ErrERRCheck( errBFICheckpointWorkRemaining );
        }
        else if ( err == errBFIPageTouchTooRecent )
        {
            OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks, OSFormat( "CP [ifmp=0x%x]: success b/c errBFIPageTouchTooRecent", (ULONG)ifmp ) );
            err = JET_errSuccess;
        }
        else
        {
            OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks, OSFormat( "CP [ifmp=0x%x]: work remaining b/c unknown errors", (ULONG)ifmp ) );
            err = ErrERRCheck( errBFICheckpointWorkRemaining );
        }
    }

    if ( err == JET_errSuccess && !fQuiesceCheckpoint && pfmp->Pinst()->m_fCheckpointQuiesce )
    {
        OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks, OSFormat( "CP [ifmp=0x%x]: work remaining b/c we changed our mind on quiescing", (ULONG)ifmp ) );
        err = ErrERRCheck( errBFICheckpointWorkRemaining );
    }


    LGPOS lgposCheckpointOB0 = lgposMin;
    BFIGetLgposOldestBegin0( ifmp, &lgposCheckpointOB0, fTrue );
    if ( 0 == CmpLgpos( lgposCheckpointOB0, lgposMax ) )
    {
        lgposCheckpointOB0 = lgposMin;
    }
#ifdef DEBUG
    else
    {
        const LGPOS lgposCheckpointOB0Rounded = BFIOB0Lgpos( ifmp, lgposCheckpointOB0, fFalse );
        Assert( 0 == CmpLgpos( &lgposCheckpointOB0, &lgposCheckpointOB0Rounded ) );
    }
#endif
    const LGPOS lgposCheckpoint = BFIOB0Lgpos( ifmp, plog->LgposLGCurrentCheckpointMayFail(), fFalse );

    Assert( fUrgentCheckpointUpdate || !fQuiesceCheckpoint );

    if ( fUrgentCheckpointUpdate )
    {
        LGPOS lgposStopT = { 0 };

        ERR errT = ErrBFIOB0MaintScan( ifmp, pbffmp,
                            lgposNewest,
                            cbCheckpointDepth,
                            &lockOB0Pass2N,
                            lgposMin,
                            &lgposPreferredCheckpoint,
                            &lgposStopT,
                            bfob0moCleaning );


        lgposCheckpointOB0 = lgposMin;
        BFIGetLgposOldestBegin0( ifmp, &lgposCheckpointOB0, fTrue );
        if ( 0 == CmpLgpos( lgposCheckpointOB0, lgposMax ) )
        {
            lgposCheckpointOB0 = lgposMin;
        }
#ifdef DEBUG
        else
        {
            const LGPOS lgposCheckpointOB0Rounded = BFIOB0Lgpos( ifmp, lgposCheckpointOB0, fFalse );
            Assert( 0 == CmpLgpos( &lgposCheckpointOB0, &lgposCheckpointOB0Rounded ) );
        }
#endif

        OSTrace( JET_tracetagBufferManagerMaintTasks,
                    OSFormat(   "CPUPD: bfob0moCleaning ifmp 0x%x lgposCheckpointOB0 %s to lgposStopT %s errT = %d",
                                (ULONG)ifmp,
                                OSFormatLgpos( LGPOS( lgposCheckpointOB0 ) ),
                                OSFormatLgpos( LGPOS( lgposStopT ) ),
                                errT ) );



        const BOOL fEmptyOB0 = CmpLgpos( lgposMin, lgposCheckpointOB0 ) == 0;
        
        if ( fEmptyOB0 ||
             ( CmpLgpos( lgposCheckpoint, lgposCheckpointOB0 ) < 0 ) )
        {

            OSTrace( JET_tracetagBufferManagerMaintTasks,
                    OSFormat(   "CPUPD: Pushing out checkpoint for fmp 0x%x because lgposCheckpoint %s could be improved to lgposOBO %s",
                                (ULONG)ifmp,
                                OSFormatLgpos( LGPOS( lgposCheckpoint ) ),
                                OSFormatLgpos( LGPOS( lgposCheckpointOB0 ) ) ) );

            *pfUpdateCheckpoint = fTrue;
        }

        if ( err == JET_errSuccess &&
                lgposWaypointLatency.lGeneration == 0 &&
                !fEmptyOB0 &&
                lgposCheckpointOB0.lGeneration != lgposStopT.lGeneration )
        {
            OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks, OSFormat( "CP [ifmp=0x%x]: work remaining b/c lgposCheckpointOB0 does not match lgposStopT (%d != %d)", (ULONG)ifmp, lgposCheckpointOB0.lGeneration, lgposStopT.lGeneration ) );
            err = ErrERRCheck( errBFICheckpointWorkRemaining );
        }
    }

    OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks,
        OSFormat(   "CP: Checkpoint Adv error stats:   err = %d\r\n"
                    "CP:    ifmp                           = 0x%x\r\n"
                    "CP:    cEntriesVisited                = %d\r\n"
                    "CP:    cCleanRemoved                  = %d\r\n"
                    "CP:    cFlushErrSuccess               = %d\r\n"
                    "CP:    cFlushErrPageFlushed           = %d\r\n"
                    "CP:    cFlushErrPageFlushPending      = %d\r\n"
                    "CP:    cFlushErrRemainingDependencies = %d\r\n"
                    "CP:    cFlushErrDependentPurged       = %d\r\n"
                    "CP:    cFlushErrLatchConflict         = %d\r\n"
                    "CP:    cFlushErrPageTouchTooRecent    = %d\r\n"
                    "CP:    cFlushErrOther                 = %d\r\n"
                    "CP:    lgposCheckpoint                = %08x:%04x:%04x\r\n"
                    "CP:    lgposCheckpointOB0             = %08x:%04x:%04x\r\n",
                    err,
                    (ULONG)ifmp,
                    pbffmp->ChkAdvData.cEntriesVisited,
                    pbffmp->ChkAdvData.cCleanRemoved,
                    pbffmp->ChkAdvData.cFlushErrSuccess,
                    pbffmp->ChkAdvData.cFlushErrPageFlushed,
                    pbffmp->ChkAdvData.cFlushErrPageFlushPending,
                    pbffmp->ChkAdvData.cFlushErrRemainingDependencies,
                    pbffmp->ChkAdvData.cFlushErrDependentPurged,
                    pbffmp->ChkAdvData.cFlushErrLatchConflict,
                    pbffmp->ChkAdvData.cFlushErrPageTouchTooRecent,
                    pbffmp->ChkAdvData.cFlushErrOther,
                    lgposCheckpoint.lGeneration, lgposCheckpoint.isec, lgposCheckpoint.ib,
                    lgposCheckpointOB0.lGeneration, lgposCheckpointOB0.isec, lgposCheckpointOB0.ib ) );

    return err;
}


CSemaphore      g_semMaintCheckpointRequest( CSyncBasicInfo( _T( "g_semMaintCheckpointRequest" ) ) );

TICK            g_tickMaintCheckpointLast;

POSTIMERTASK    g_posttBFIMaintCheckpointITask = NULL;
POSTIMERTASK    g_posttBFIMaintCacheStatsITask = NULL;
POSTIMERTASK    g_posttBFIMaintIdleCacheStatsITask = NULL;



void BFIMaintCheckpointRequest()
{

    BOOL fAcquiredAsync = g_semMaintCheckpointRequest.FTryAcquire();


    if ( fAcquiredAsync )
    {

        if ( ErrBFIMaintScheduleTask( g_posttBFIMaintCheckpointITask, NULL, dtickMaintCheckpointDelay, dtickMaintCheckpointFuzz ) >= JET_errSuccess )
        {

            fAcquiredAsync = fFalse;
        }
    }


    if ( fAcquiredAsync )
    {
        Assert( g_semMaintCheckpointRequest.CAvail() == 0 );
        g_semMaintCheckpointRequest.Release();
    }
}

#ifdef DEBUG
LONG g_cSingleThreadedCheckpointTaskCheck = 0;
#endif


void BFIMaintCheckpointITask( VOID * pvGroupContext, VOID * pvRuntimeContext )
{
    OSTrace( JET_tracetagBufferManagerMaintTasks, __FUNCTION__ );

    OnDebug( LONG cBegin = AtomicIncrement( &g_cSingleThreadedCheckpointTaskCheck ) );
    Expected( cBegin == 1 );


    BOOL fAcquiredAsync = fTrue;


    BFIMaintCheckpointIUpdate();


    g_tickMaintCheckpointLast = TickOSTimeCurrent();


    if ( fAcquiredAsync )
    {
        Assert( g_semMaintCheckpointRequest.CAvail() == 0 );
        g_semMaintCheckpointRequest.Release();
    }

    OnDebug( LONG cEnd = AtomicDecrement( &g_cSingleThreadedCheckpointTaskCheck ) );
    Assert( cEnd == 0 );
}


void BFIMaintCheckpointIUpdateInst( const size_t ipinst )
{
    extern CRITPOOL< INST* > g_critpoolPinstAPI;
    CCriticalSection *pcritInst = &g_critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
    pcritInst->Enter();

    INST *pinst = g_rgpinst[ ipinst ];

    if ( pinstNil == pinst )
    {
        pcritInst->Leave();
        return;
    }


    const BOOL fAPILocked = pinst->APILock( pinst->fAPICheckpointing );
    pcritInst->Leave();

    if ( fAPILocked )
    {
        if ( pinst->m_fJetInitialized )
        {
            (void)pinst->m_plog->ErrLGUpdateCheckpointFile( fFalse );
        }

        pinst->APIUnlock( pinst->fAPICheckpointing );
    }

}


void BFIMaintCheckpointIUpdate()
{
    for ( size_t ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
    {
        BFIMaintCheckpointIUpdateInst( ipinst );
    }
}

#ifdef MINIMAL_FUNCTIONALITY
#else


const BOOL      g_fBFMaintHashedLatches = fFalse;
size_t          g_icReqOld = 0;
size_t          g_icReqNew = 1;
ULONG           g_rgcReqSystem[ 2 ];
ULONG           g_dcReqSystem;


TICK TickBFIHashedLatchTime( const TICK tickIn )
{
    return ( tickIn < (ULONG)cBFHashedLatch ) ? ( cBFHashedLatch + 1 ) : tickIn;
}


void BFIMaintHashedLatchesITask( DWORD_PTR )
{
    OSTrace( JET_tracetagBufferManagerHashedLatches, __FUNCTION__ );
    Enforce( g_fBFMaintHashedLatches );


    WCHAR   wszBuf[ 16 ]            = { 0 };
    BOOL    fEnableHashedLatches    = fFalse;
    if (    FOSConfigGet( L"BF", L"Enable Hashed Latches", wszBuf, sizeof( wszBuf ) ) &&
            wszBuf[ 0 ] )
    {
        fEnableHashedLatches = !!_wtol( wszBuf );
    }


    if ( fEnableHashedLatches )
    {
        BFIMaintHashedLatchesIRedistribute();
    }
}


void BFIMaintHashedLatchesIRedistribute()
{
    const size_t    cProc = OSSyncGetProcessorCountMax();
    size_t          iProc;
    size_t          iNominee;
    size_t          iHashedLatch;
    ULONG           rgdcReqNomineeSum[ cBFNominee ];
    ULONG           rgdcReqNomineeMax[ cBFNominee ];
    ULONG           rgdcReqHashedLatchSum[ cBFHashedLatch ];
    ULONG           dcReqHashedLatchTotal;
    size_t          iNomineeWinner;
    ULONG           dcReqNomineeSumWinner;
    size_t          iHashedLatchLoser;
    ULONG           dcReqHashedLatchSumLoser;
    PBF             pbfElect;
    PBF             pbfLoser;
    PBF             pbfWinner;


    PERFOpt( g_rgcReqSystem[ g_icReqNew ] = cBFCacheReq.Get( perfinstGlobal ) );

    for ( iProc = 0; iProc < cProc; iProc++ )
    {
        PLS* const ppls = Ppls( iProc );

        for ( iNominee = 0; iNominee < cBFNominee; iNominee++ )
        {
            ppls->rgcreqBFNominee[ g_icReqNew ][ iNominee ] = ppls->rgBFNominee[ iNominee ].cCacheReq;
        }
        for ( iHashedLatch = 0; iHashedLatch < cBFHashedLatch; iHashedLatch++ )
        {
            ppls->rgcreqBFHashedLatch[ g_icReqNew ][ iHashedLatch ] = ppls->rgBFHashedLatch[ iHashedLatch ].cCacheReq;
        }
    }


    g_dcReqSystem = g_rgcReqSystem[ g_icReqNew ] - g_rgcReqSystem[ g_icReqOld ];

    for ( iProc = 0; iProc < cProc; iProc++ )
    {
        PLS* const ppls = Ppls( iProc );

        for ( iNominee = 0; iNominee < cBFNominee; iNominee++ )
        {
            ppls->rgdcreqBFNominee[ iNominee ] = ppls->rgcreqBFNominee[ g_icReqNew ][ iNominee ] - ppls->rgcreqBFNominee[ g_icReqOld ][ iNominee ];
        }
        for ( iHashedLatch = 0; iHashedLatch < cBFHashedLatch; iHashedLatch++ )
        {
            ppls->rgdcreqBFHashedLatch[ iHashedLatch ] = ppls->rgcreqBFHashedLatch[ g_icReqNew ][ iHashedLatch ] - ppls->rgcreqBFHashedLatch[ g_icReqOld ][ iHashedLatch ];
        }
    }


    g_icReqOld    = g_icReqOld ^ 1;
    g_icReqNew    = g_icReqNew ^ 1;


    for ( iNominee = 0; iNominee < cBFNominee; iNominee++ )
    {
        rgdcReqNomineeSum[ iNominee ] = 0;
        rgdcReqNomineeMax[ iNominee ] = 0;
    }
    for ( iHashedLatch = 0; iHashedLatch < cBFHashedLatch; iHashedLatch++ )
    {
        rgdcReqHashedLatchSum[ iHashedLatch ] = 0;
    }

    for ( iProc = 0; iProc < cProc; iProc++ )
    {
        PLS* const ppls = Ppls( iProc );

        for ( iNominee = 0; iNominee < cBFNominee; iNominee++ )
        {
            rgdcReqNomineeSum[ iNominee ] += ppls->rgdcreqBFNominee[ iNominee ];
            rgdcReqNomineeMax[ iNominee ] = max( rgdcReqNomineeMax[ iNominee ], ppls->rgdcreqBFNominee[ iNominee ] );
        }
        for ( iHashedLatch = 0; iHashedLatch < cBFHashedLatch; iHashedLatch++ )
        {
            rgdcReqHashedLatchSum[ iHashedLatch ] += ppls->rgdcreqBFHashedLatch[ iHashedLatch ];
        }
    }

    dcReqHashedLatchTotal = 0;
    for ( iHashedLatch = 0; iHashedLatch < cBFHashedLatch; iHashedLatch++ )
    {
        dcReqHashedLatchTotal += rgdcReqHashedLatchSum[ iHashedLatch ];
    }


    iNomineeWinner          = 0;
    dcReqNomineeSumWinner   = 0;

    for ( iNominee = 1; iNominee < cBFNominee; iNominee++ )
    {
        const PBF pbfNominee = Ppls( 0 )->rgBFNominee[ iNominee ].pbf;

        if (    pbfNominee != pbfNil &&
                rgdcReqNomineeMax[ iNominee ] < pctProcAffined * rgdcReqNomineeSum[ iNominee ] &&
                dcReqNomineeSumWinner < rgdcReqNomineeSum[ iNominee ] )
        {
            iNomineeWinner          = iNominee;
            dcReqNomineeSumWinner   = rgdcReqNomineeSum[ iNominee ];
        }
    }


    iHashedLatchLoser           = 0;
    dcReqHashedLatchSumLoser    = ULONG( ~0 );

    for ( iHashedLatch = 0; iHashedLatch < cBFHashedLatch; iHashedLatch++ )
    {
        if ( dcReqHashedLatchSumLoser > rgdcReqHashedLatchSum[ iHashedLatch ] )
        {
            iHashedLatchLoser           = iHashedLatch;
            dcReqHashedLatchSumLoser    = rgdcReqHashedLatchSum[ iHashedLatch ];
        }
    }


    pbfElect    = Ppls( 0 )->rgBFNominee[ 0 ].pbf;
    pbfLoser    = Ppls( 0 )->rgBFHashedLatch[ iHashedLatchLoser ].pbf;

    if (    pbfElect != pbfNil &&
            rgdcReqNomineeMax[ 0 ] < pctProcAffined * rgdcReqNomineeSum[ 0 ] &&
            rgdcReqNomineeSum[ 0 ] > dcReqHashedLatchSumLoser )
    {
        if ( pbfElect->sxwl.ErrTryAcquireExclusiveLatch() == CSXWLatch::ERR::errSuccess )
        {
            if (    pbfLoser == pbfNil ||
                    pbfLoser->sxwl.ErrTryAcquireExclusiveLatch() == CSXWLatch::ERR::errSuccess )
            {
                if ( pbfLoser == pbfNil || FBFILatchDemote( pbfLoser ) )
                {
                    for ( iProc = 0; iProc < cProc; iProc++ )
                    {
                        PLS* const ppls = Ppls( iProc );
                        ppls->rgBFNominee[ 0 ].pbf = pbfNil;
                        ppls->rgBFHashedLatch[ iHashedLatchLoser ].pbf = pbfElect;
                    }
                    pbfElect->iHashedLatch = iHashedLatchLoser;
                    pbfElect->bfls = bflsHashed;
                    if ( pbfLoser != pbfNil )
                    {
                        OSTrace(
                            JET_tracetagBufferManagerHashedLatches,
                            OSFormat(   "BF %s in slot %d demoted (%.2f percent %d)",
                                        OSFormatPointer( pbfLoser ),
                                        (ULONG)iHashedLatchLoser,
                                        100.0 * dcReqHashedLatchSumLoser / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                        dcReqHashedLatchSumLoser ) );
                    }
                    OSTrace(
                        JET_tracetagBufferManagerHashedLatches,
                        OSFormat(   "BF %s promoted to slot %d (%.2f percent %d %.2f percent)",
                                    OSFormatPointer( pbfElect ),
                                    (ULONG)iHashedLatchLoser,
                                    100.0 * rgdcReqNomineeSum[ 0 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    rgdcReqNomineeSum[ 0 ],
                                    100.0 * rgdcReqNomineeMax[ 0 ] / rgdcReqNomineeSum[ 0 ] ) );
                    OSTrace(
                        JET_tracetagBufferManagerHashedLatches,
                        OSFormat(   "Hashed Latch Summary:\r\n"
                                    "---------------------\r\n"
                                    " 0    %.2f percent\r\n"
                                    " 1    %.2f percent\r\n"
                                    " 2    %.2f percent\r\n"
                                    " 3    %.2f percent\r\n"
                                    " 4    %.2f percent\r\n"
                                    " 5    %.2f percent\r\n"
                                    " 6    %.2f percent\r\n"
                                    " 7    %.2f percent\r\n"
                                    " 8    %.2f percent\r\n"
                                    " 9    %.2f percent\r\n"
                                    "10    %.2f percent\r\n"
                                    "11    %.2f percent\r\n"
                                    "12    %.2f percent\r\n"
                                    "13    %.2f percent\r\n"
                                    "14    %.2f percent\r\n"
                                    "15    %.2f percent\r\n"
                                    "Total %.2f percent",
                                    100.0 * rgdcReqHashedLatchSum[ 0 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 1 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 2 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 3 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 4 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 5 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 6 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 7 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 8 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 9 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 10 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 11 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 12 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 13 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 14 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * rgdcReqHashedLatchSum[ 15 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                                    100.0 * dcReqHashedLatchTotal / ( g_dcReqSystem ? g_dcReqSystem : 100 ) ) );
                }
                if ( pbfLoser != pbfNil )
                {
                    pbfLoser->sxwl.ReleaseExclusiveLatch();
                }
            }
            pbfElect->sxwl.ReleaseExclusiveLatch();
        }
    }


    else if ( pbfElect != pbfNil )
    {
        Assert( pbfElect->bfat == bfatFracCommit );

        if ( pbfElect->sxwl.ErrTryAcquireExclusiveLatch() == CSXWLatch::ERR::errSuccess )
        {
            for ( iProc = 0; iProc < cProc; iProc++ )
            {
                BFNominee* const pbfn = &Ppls( iProc )->rgBFNominee[ 0 ];
                pbfn->pbf = pbfNil;
            }
            pbfElect->tickEligibleForNomination = TickBFIHashedLatchTime( TickOSTimeCurrent() + dtickPromotionDenied );
            pbfElect->bfls = bflsNormal;
            OSTrace(
                JET_tracetagBufferManagerHashedLatches,
                OSFormat(   "BF %s denied promotion (%.2f percent %d %.2f percent %d)",
                            OSFormatPointer( pbfElect ),
                            100.0 * rgdcReqNomineeSum[ 0 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            rgdcReqNomineeSum[ 0 ],
                            100.0 * rgdcReqNomineeMax[ 0 ] / rgdcReqNomineeSum[ 0 ],
                            dcReqHashedLatchSumLoser ) );
            OSTrace(
                JET_tracetagBufferManagerHashedLatches,
                OSFormat(   "Hashed Latch Summary:\r\n"
                            "---------------------\r\n"
                            " 0    %.2f percent\r\n"
                            " 1    %.2f percent\r\n"
                            " 2    %.2f percent\r\n"
                            " 3    %.2f percent\r\n"
                            " 4    %.2f percent\r\n"
                            " 5    %.2f percent\r\n"
                            " 6    %.2f percent\r\n"
                            " 7    %.2f percent\r\n"
                            " 8    %.2f percent\r\n"
                            " 9    %.2f percent\r\n"
                            "10    %.2f percent\r\n"
                            "11    %.2f percent\r\n"
                            "12    %.2f percent\r\n"
                            "13    %.2f percent\r\n"
                            "14    %.2f percent\r\n"
                            "15    %.2f percent\r\n"
                            "Total %.2f percent",
                            100.0 * rgdcReqHashedLatchSum[ 0 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 1 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 2 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 3 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 4 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 5 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 6 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 7 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 8 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 9 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 10 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 11 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 12 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 13 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 14 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * rgdcReqHashedLatchSum[ 15 ] / ( g_dcReqSystem ? g_dcReqSystem : 100 ),
                            100.0 * dcReqHashedLatchTotal / ( g_dcReqSystem ? g_dcReqSystem : 100 ) ) );
            pbfElect->sxwl.ReleaseExclusiveLatch();
        }
    }


    pbfWinner = Ppls( 0 )->rgBFNominee[ iNomineeWinner ].pbf;

    if ( Ppls( 0 )->rgBFNominee[ 0 ].pbf == pbfNil && pbfWinner != pbfNil )
    {
        if ( pbfWinner->sxwl.ErrTryAcquireExclusiveLatch() == CSXWLatch::ERR::errSuccess )
        {
            for ( iProc = 0; iProc < cProc; iProc++ )
            {
                PLS* const ppls = Ppls( cProc - 1 - iProc );
                ppls->rgBFNominee[ iNomineeWinner ].pbf = pbfNil;
                ppls->rgBFNominee[ 0 ].pbf = pbfWinner;
            }
            pbfWinner->bfls = bflsElect;
            OSTrace(
                JET_tracetagBufferManagerHashedLatches,
                OSFormat(   "BF %s in slot %d elected (%d %.2f percent)",
                            OSFormatPointer( pbfWinner ),
                            (ULONG)iNomineeWinner,
                            rgdcReqNomineeSum[ iNomineeWinner ],
                            100.0 * rgdcReqNomineeMax[ iNomineeWinner ] / rgdcReqNomineeSum[ iNomineeWinner ] ) );
            pbfWinner->sxwl.ReleaseExclusiveLatch();
        }
    }


    for ( iNominee = 1; iNominee < cBFNominee; iNominee++ )
    {
        if ( iNominee != iNomineeWinner )
        {
            const PBF pbfNominee = Ppls( 0 )->rgBFNominee[ iNominee ].pbf;

            if (    pbfNominee != pbfNil &&
                    pbfNominee->sxwl.ErrTryAcquireExclusiveLatch() == CSXWLatch::ERR::errSuccess )
            {
                for ( iProc = 0; iProc < cProc; iProc++ )
                {
                    PLS* const ppls = Ppls( cProc - 1 - iProc );
                    ppls->rgBFNominee[ iNominee ].pbf = pbfNil;
                }
                pbfNominee->tickEligibleForNomination = TickBFIHashedLatchTime( TickOSTimeCurrent() + dtickLosingNominee );
                pbfNominee->bfls = bflsNormal;
                OSTrace(
                    JET_tracetagBufferManagerHashedLatches,
                    OSFormat(   "BF %s in slot %d purged (%d %.2f percent)",
                                OSFormatPointer( pbfNominee ),
                                (ULONG)iNominee,
                                rgdcReqNomineeSum[ iNominee ],
                                100.0 * rgdcReqNomineeMax[ iNominee ] / rgdcReqNomineeSum[ iNominee ] ) );
                pbfNominee->sxwl.ReleaseExclusiveLatch();
            }
        }
    }
}

#endif


CSemaphore      g_semMaintCacheStatsRequest( CSyncBasicInfo( _T( "g_semMaintCacheStatsRequest" ) ) );

CSemaphore      g_semMaintCacheSize( CSyncBasicInfo( _T( "g_semMaintCacheSize" ) ) );
LONG            g_cMaintCacheSizePending = 0;

inline ICBPage IcbBFIBufferSize( __in const INT cbSize )
{
    for( ICBPage icbBuffer = icbPageSmallest; icbBuffer < icbPageMax; icbBuffer++ )
    {
        if ( cbSize <= g_rgcbPageSize[icbBuffer]  )
        {
            return icbBuffer;
        }
    }
    return icbPageInvalid;
}

inline _Ret_range_( icbPageInvalid, icbPageBiggest ) ICBPage IcbBFIPageSize( __in const INT cbSize )
{
    switch( cbSize )
    {
        case (4*1024):
            return icbPage4KB;
        case (8*1024):
            return icbPage8KB;
        case (16*1024):
            return icbPage16KB;
        case (32*1024):
            return icbPage32KB;
    }
    AssertSz( fFalse, "Invalid page size request (cb = %d)", cbSize );
    return icbPageInvalid;
}

INLINE __int64 CbBFICacheUsed( const BOOL fFullyHydrated )
{
    __int64 cb = 0;
    INT icbPage = icbPageSmallest;
    INT icbCacheMaxT = g_icbCacheMax;

    
    INT& icbPageT = fFullyHydrated ? icbCacheMaxT : icbPage;

    for( ;icbPage <= g_icbCacheMax; icbPage++ )
    {
        cb += ( (__int64)g_rgcbfCachePages[icbPage] * (__int64)g_rgcbPageSize[icbPageT] );
    }

    Assert( g_rgcbfCachePages[icbPageInvalid] == 0 );

    return cb;
}



INLINE CBF CbfBFICacheUsed()
{
    CBF cbf = 0;

    for( INT icbPage = icbPageSmallest; icbPage <= g_icbCacheMax; icbPage++ )
    {
        cbf += g_rgcbfCachePages[icbPage];
    }

    Assert( g_rgcbfCachePages[icbPageInvalid] == 0 );

    return cbf;
}


__int64 CbBFICacheBufferSize()
{
    if ( g_fBFInitialized )
    {
        return CbBFICacheIMemoryCommitted();
    }

    return 0;
}


INLINE CBF CbfBFICacheCommitted()
{
    const CBF cbfCommittedT = (CBF)g_cbfCommitted - (CBF)g_cbfNewlyCommitted;

    if ( cbfCommittedT < 0 )
    {
        return 0;
    }
    
    return cbfCommittedT;
}


__int64 CbBFICacheSizeUsedDehydrated()
{
    return CbBFICacheUsed( fFalse );
}


__int64 CbBFICacheISizeUsedHydrated()
{
    return CbBFICacheUsed( fTrue );
}

__int64 CbBFICacheIMemoryReserved()
{
    return g_cbCacheReservedSize;
}

__int64 CbBFICacheIMemoryCommitted()
{
    return g_cbCacheCommittedSize;
}


__int64 CbBFIAveResourceSize()
{
    if ( g_fBFInitialized )
    {
        const __int64 cbCacheSizeUsedDehydrated = CbBFICacheSizeUsedDehydrated();
        const CBF cbfCacheSizeUsed = CbfBFICacheUsed();
        

        if ( cbCacheSizeUsedDehydrated > 0 && cbfCacheSizeUsed > 0 && cbCacheSizeUsedDehydrated >= cbfCacheSizeUsed )
        {
            return cbCacheSizeUsedDehydrated / cbfCacheSizeUsed;
        }
        else
        {
            return g_rgcbPageSize[g_icbCacheMax];
        }
    }

    return 0;
}


LONG_PTR CbfBFICredit()
{
    if ( g_fBFInitialized )
    {
        const __int64 cbCacheSizeUsedHydrated = CbBFICacheISizeUsedHydrated();
        const __int64 cbCacheSizeUsedDehydrated = CbBFICacheSizeUsedDehydrated();

        
        if ( cbCacheSizeUsedHydrated > cbCacheSizeUsedDehydrated )
        {
            return (LONG_PTR)( ( cbCacheSizeUsedHydrated - cbCacheSizeUsedDehydrated ) / g_rgcbPageSize[g_icbCacheMax] );
        }
    }

    return 0;
}


LONG_PTR CbfBFIAveCredit()
{
    return g_avgCbfCredit.GetAverage();
}

BOOL FBFIMaintCacheSizeQuiescedInSensitivityRange()
{
    if ( cbfCacheTarget > g_cbfCacheTargetOptimal )
    {
        return fFalse;
    }

    ULONG_PTR cbfCacheStable = 0;
    ULONG_PTR cbfCacheStableNew = 0;
    const ULONG_PTR cbfBFIAveCredit = (ULONG_PTR)CbfBFIAveCredit();
    const ULONG_PTR cbfBFICredit = (ULONG_PTR)CbfBFICredit();
    
    if ( FBFICacheSizeFixed() )
    {
        const ULONG_PTR cbfCacheTargetEffective = g_cbfCacheUserOverride ? g_cbfCacheUserOverride : UlParam( JET_paramCacheSizeMin );
        cbfCacheStable = cbfCacheTargetEffective + cbfBFIAveCredit;
        cbfCacheStableNew = cbfCacheTargetEffective + cbfBFICredit;
    }
    else
    {
        cbfCacheStable = cbfCacheTarget;
        cbfCacheStableNew = cbfCacheStable + cbfBFICredit;
        cbfCacheStableNew -= min( cbfCacheStableNew, cbfBFIAveCredit );
    }


    if ( (ULONG_PTR)cbfCacheSize == cbfCacheStable )
    {
        return FBFICacheApproximatelyEqual( cbfCacheStableNew, cbfCacheStable );
    }

    return fFalse;
}

LOCAL ULONG g_cMaintCacheSizeReqAcquireFailures = 0;

#ifndef RTM
TICK        g_tickMaintCacheStatsResizeLastAttempt = 0;
LONG_PTR    g_cbfMaintCacheStatsResizeLastAttempt = 0;
TICK        g_tickMaintCacheStatsResizeLast = 0;
LONG_PTR    g_cbfMaintCacheStatsResizeLast = 0;

#endif
TICK        g_tickLastLowMemoryCallback = 0;

#define     fIdleCacheStatTask      (VOID *)1

void BFIMaintLowMemoryCallback( DWORD_PTR pvUnused )
{
    g_tickLastLowMemoryCallback = TickOSTimeCurrent();
    ErrBFIMaintCacheStatsRequest( bfmcsrtForce );
}

void BFICacheSizeBoost()
{
    if ( FBFICacheSizeFixed() )
    {
        return;
    }

    const size_t cbAvailOsMem = g_cacheram.AvailablePhysicalMemory();
    const size_t cbRam = g_cacheram.TotalPhysicalMemory();


    if ( ( cbAvailOsMem < ( cbRam / 10 ) ) &&
         ( ( (size_t)cbfCacheSize * (size_t)g_rgcbPageSize[g_icbCacheMax] ) > ( cbRam / 10 ) ) )
    {
        return;
    }

    if ( ( cbAvailOsMem < ( cbRam / 20 ) ) )
    {
        return;
    }

    g_critCacheSizeSetTarget.Enter();
    ULONG_PTR cbfRealCacheSize;
    LONG_PTR cbBoost;
    CallS( ErrBFGetCacheSize( &cbfRealCacheSize ) );
    cbBoost = min( ( cbfAvailPoolHigh - (LONG_PTR)g_bfavail.Cobject() ) * g_rgcbPageSize[g_icbCacheMax],
                   (LONG_PTR)cbAvailOsMem / 2 );
    if ( cbfRealCacheSize >= UlParam( JET_paramCacheSizeMax ) ||
         cbBoost <= 0 )
    {
        g_critCacheSizeSetTarget.Leave();
        return;
    }
    g_cCacheBoosts++;
    g_cbCacheBoosted += cbBoost;
    g_cacheram.OverrideResourceAdjustments( (double)cbBoost );
    g_cacheram.SetOptimalResourcePoolSize();
    g_critCacheSizeSetTarget.Leave();

    OnDebug( BOOL fAcquiredSemaphore = fFalse );


    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );
    
    (void)ErrBFIMaintCacheSizeRequest( OnDebug( &fAcquiredSemaphore ) );


    FOSSetCleanupState( fCleanUpStateSaved );
}

LOCAL void BFIMaintIdleCacheStatsSchedule()
{

    Expected( ( g_posttBFIMaintIdleCacheStatsITask != NULL ) || !g_fBFInitialized );

    if ( g_posttBFIMaintIdleCacheStatsITask != NULL )
    {
        (void)ErrBFIMaintScheduleTask( g_posttBFIMaintIdleCacheStatsITask, NULL, dtickIdleCacheStatsPeriod, dtickIdleCacheStatsSlop );
    }
}

#ifndef ENABLE_JET_UNIT_TEST

LOCAL INLINE BOOL FBFICleanBuffer( const IBF ibf )
{
    if ( ( ibf < cbfInit ) && !FBFIIsCleanEnoughForEvict( PbfBFICacheIbf( ibf ) ) )
    {
        return fFalse;
    }

    return fTrue;
}

#else

LOCAL BOOL g_fSimulateDirtyCache = fFalse;

LOCAL BOOL FBFICleanBuffer( const IBF ibf )
{
    if ( g_fSimulateDirtyCache && ( ( ibf % 2 ) == 0 ) )
    {
        return fFalse;
    }

    return fTrue;
}

#endif

LOCAL LONG_PTR CbfBFIMaintIdleCacheStatsWithdrawal(
    const LONG_PTR cbfTargetCacheSizeCurrent,
    const LONG_PTR cbfTargetCacheSizeMin,
    const LONG_PTR dcbfLastWithdrawalThreshold,
    BOOL* const pfTooManyUncleanPages )
{
    Expected( cbfTargetCacheSizeCurrent >= 0 );
    Expected( cbfTargetCacheSizeMin >= 0 );
    Expected( dcbfLastWithdrawalThreshold > 0 );
    Assert( pfTooManyUncleanPages != NULL );


    LONG_PTR cbfTargetCacheSizeNew = cbfTargetCacheSizeMin + ( cbfTargetCacheSizeCurrent - cbfTargetCacheSizeMin ) / 2;

    if ( cbfTargetCacheSizeNew <= ( cbfTargetCacheSizeMin + dcbfLastWithdrawalThreshold ) )
    {
        cbfTargetCacheSizeNew = cbfTargetCacheSizeMin;
    }

    *pfTooManyUncleanPages = fFalse;
    CPG cbfUnclean = 0;

    for ( IBF ibf = cbfTargetCacheSizeCurrent - 1; ibf >= cbfTargetCacheSizeNew; ibf-- )
    {
        if ( FBFICleanBuffer( ibf ) )
        {
            continue;
        }

        if ( ++cbfUnclean > cbfIdleCacheUncleanThreshold )
        {
            *pfTooManyUncleanPages = fTrue;
            cbfTargetCacheSizeNew = ibf + 1;
            break;
        }
    }
    
    return cbfTargetCacheSizeNew;
}

void BFIMaintIdleCacheStatsITask( VOID *pvGroupContext, VOID * )
{
    OSTrace( JET_tracetagBufferManagerMaintTasks, __FUNCTION__ );
    

    if ( FBFIMaintCacheSizeQuiescedInSensitivityRange() && FBFICacheSizeFixed() )
    {
        OSTrace( JET_tracetagBufferManagerMaintTasks, OSFormat( "%s: quiesced due to fixed cache size within sensitivity range.", __FUNCTION__ ) );
        return;
    }

    
    if ( !FJetConfigLowMemory() && !FJetConfigLowPower() )
    {
        if ( FBFIMaintCacheStatsTryAcquire() )
        {
            BFIMaintCacheStatsITask( pvGroupContext, fIdleCacheStatTask );
            BFIMaintCacheStatsRelease();
        }

        goto Reschedule;
    }


    g_critCacheSizeSetTarget.Enter();


    BOOL fTooManyUncleanPages = fFalse;
    const LONG cbPageSize = g_rgcbPageSize[g_icbCacheMax];
    const LONG_PTR cbfTargetCacheSizeCurrent = (LONG_PTR)( g_cacheram.GetOptimalResourcePoolSize() / cbPageSize );
    const LONG_PTR cbfTargetCacheSizeMin = (LONG_PTR)( max( max( UlParam( JET_paramCacheSizeMin ), (ULONG_PTR)cbfCacheMinMin ), (ULONG_PTR)cbfCacheDeadlock ) );
    const LONG_PTR dcbfLastWithdrawalThreshold = dcbIdleCacheLastWithdrawalThreshold / cbPageSize;
    const LONG_PTR cbfTargetCacheSizeNew = CbfBFIMaintIdleCacheStatsWithdrawal( cbfTargetCacheSizeCurrent, cbfTargetCacheSizeMin, dcbfLastWithdrawalThreshold, &fTooManyUncleanPages );
    const LONG_PTR dcbfCacheSize = cbfTargetCacheSizeNew - cbfTargetCacheSizeCurrent;

    OSTrace(
        JET_tracetagBufferManagerMaintTasks,
        OSFormat(
            "%s: cache sizing from %Id to %Id buffers (min. size is %Id buffers, too-many-unclean is %d).",
            __FUNCTION__,
            cbfTargetCacheSizeCurrent,
            cbfTargetCacheSizeNew,
            cbfTargetCacheSizeMin,
            fTooManyUncleanPages ) );

    AssertSz( !fTooManyUncleanPages || ( dcbfCacheSize <= -cbfIdleCacheUncleanThreshold ),
        "We must only have limited shrinkage if we are shrinking by more than the unclean threshold." );


    if ( dcbfCacheSize >= 0 )
    {
        Assert( !fTooManyUncleanPages );
        g_critCacheSizeSetTarget.Leave();
        OSTrace( JET_tracetagBufferManagerMaintTasks, OSFormat( "%s: quiesced due to non-negative cache size change.", __FUNCTION__ ) );
        return;
    }


    g_cacheram.OverrideResourceAdjustments( (double)( dcbfCacheSize * g_rgcbPageSize[g_icbCacheMax] ) );
    g_cacheram.SetOptimalResourcePoolSize();


    if ( FBFIMaintCacheSizeQuiescedInSensitivityRange() && !fTooManyUncleanPages )
    {
        g_critCacheSizeSetTarget.Leave();
        OSTrace( JET_tracetagBufferManagerMaintTasks, OSFormat( "%s: quiesced due to variable cache size within sensitivity range.", __FUNCTION__ ) );
        return;
    }

    g_critCacheSizeSetTarget.Leave();


#ifdef DEBUG
    if ( dtickIdleCacheStatsPeriod >= 60 * 1000 )
    {
        const TICK dtickLastCacheStatsRequest = DtickDelta( g_tickLastCacheStatsRequest, TickOSTimeCurrent() );
        AssertSz(
            ( dtickLastCacheStatsRequest >= ( dtickIdleCacheStatsPeriod / 2 ) ) ||
            ( dtickLastCacheStatsRequest <= 1 * 1000 ),
            "Last active cache request was too recent (%u msec ago).", dtickLastCacheStatsRequest );
    }
#endif

    OnDebug( BOOL fAcquiredSemaphore = fFalse );


    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );
    
    (void)ErrBFIMaintCacheSizeRequest( OnDebug( &fAcquiredSemaphore ) );


    FOSSetCleanupState( fCleanUpStateSaved );

Reschedule:

    OSTrace( JET_tracetagBufferManagerMaintTasks, OSFormat( "%s: rescheduled.", __FUNCTION__ ) );

    BFIMaintIdleCacheStatsSchedule();
    return;
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( BF, BFIMaintIdleCacheStatsWithdrawalBasic )
{

    g_fSimulateDirtyCache = fFalse;
    BOOL fTooManyUncleanPages = fTrue;
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 200, 10, 12, &fTooManyUncleanPages ) == 105 );
    CHECK( !fTooManyUncleanPages );
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 105, 10, 12, &fTooManyUncleanPages ) == 57 );
    CHECK( !fTooManyUncleanPages );
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 57, 10, 12, &fTooManyUncleanPages ) == 33 );
    CHECK( !fTooManyUncleanPages );
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 33, 10, 12, &fTooManyUncleanPages ) == 10 );
    CHECK( !fTooManyUncleanPages );
}

JETUNITTEST( BF, BFIMaintIdleCacheStatsWithdrawalBasicDirty )
{

    g_fSimulateDirtyCache = fTrue;
    BOOL fTooManyUncleanPages = fFalse;
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 200, 10, 12, &fTooManyUncleanPages ) == 159 );
    CHECK( fTooManyUncleanPages );
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 159, 10, 12, &fTooManyUncleanPages ) == 119 );
    CHECK( fTooManyUncleanPages );
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 119, 10, 12, &fTooManyUncleanPages ) == 79 );
    CHECK( fTooManyUncleanPages );
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 79, 10, 12, &fTooManyUncleanPages ) == 44 );
    CHECK( !fTooManyUncleanPages );
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 44, 10, 12, &fTooManyUncleanPages ) == 27 );
    CHECK( !fTooManyUncleanPages );
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 27, 10, 12, &fTooManyUncleanPages ) == 10 );
    CHECK( !fTooManyUncleanPages );
}

JETUNITTEST( BF, BFIMaintIdleCacheStatsWithdrawalAlreadyQuiesced )
{

    g_fSimulateDirtyCache = fFalse;
    BOOL fTooManyUncleanPages = fTrue;
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 100, 100, 1, &fTooManyUncleanPages ) == 100 );
    CHECK( !fTooManyUncleanPages );
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 100, 100, 50, &fTooManyUncleanPages ) == 100 );
    CHECK( !fTooManyUncleanPages );
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 100, 100, 100, &fTooManyUncleanPages ) == 100 );
    CHECK( !fTooManyUncleanPages );
}

JETUNITTEST( BF, BFIMaintIdleCacheStatsWithdrawalCurrentBelowMin )
{

    g_fSimulateDirtyCache = fFalse;
    BOOL fTooManyUncleanPages = fTrue;
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 100, 110, 1, &fTooManyUncleanPages ) == 110 );
    CHECK( !fTooManyUncleanPages );
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 100, 110, 10, &fTooManyUncleanPages ) == 110 );
    CHECK( !fTooManyUncleanPages );
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 100, 110, 50, &fTooManyUncleanPages ) == 110 );
    CHECK( !fTooManyUncleanPages );
}

JETUNITTEST( BF, BFIMaintIdleCacheStatsWithdrawalStartBelowThreshold )
{

    g_fSimulateDirtyCache = fFalse;
    BOOL fTooManyUncleanPages = fTrue;
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 100, 10, 100, &fTooManyUncleanPages ) == 10 );
    CHECK( !fTooManyUncleanPages );
}

JETUNITTEST( BF, BFIMaintIdleCacheStatsWithdrawalQuiesceOnFirst )
{

    g_fSimulateDirtyCache = fFalse;
    BOOL fTooManyUncleanPages = fTrue;
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 100, 50, 25, &fTooManyUncleanPages ) == 50 );
    CHECK( !fTooManyUncleanPages );
}

JETUNITTEST( BF, BFIMaintIdleCacheStatsWithdrawalQuiesceOnSecond )
{

    g_fSimulateDirtyCache = fFalse;
    BOOL fTooManyUncleanPages = fTrue;
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 100, 50, 24, &fTooManyUncleanPages ) == 75 );
    CHECK( !fTooManyUncleanPages );
    CHECK( CbfBFIMaintIdleCacheStatsWithdrawal( 75, 50, 24, &fTooManyUncleanPages ) == 50 );
    CHECK( !fTooManyUncleanPages );
}

#endif


void BFIMaintCacheStatsITask( VOID *, VOID * pvContext )
{
    OSTrace( JET_tracetagBufferManagerMaintTasks, __FUNCTION__ );
    const TICK tickNow = TickOSTimeCurrent();

    AtomicExchange( (LONG *)&g_tickLastMaintCacheStats, tickNow );

    Assert( g_semMaintCacheStatsRequest.CAvail() == 0  );

    g_critCacheSizeSetTarget.Enter();


    g_cacheram.UpdateStatistics();

    g_cacheram.SetOptimalResourcePoolSize();


    OnNonRTM( g_tickMaintCacheStatsResizeLastAttempt = TickOSTimeCurrent() );
    OnNonRTM( g_cbfMaintCacheStatsResizeLastAttempt = cbfCacheTarget );

    g_critCacheSizeSetTarget.Leave();

    OnDebug( BOOL fAcquiredSemaphore = fFalse );
    const ERR errMaintCacheSize = ErrBFIMaintCacheSizeRequest( OnDebug( &fAcquiredSemaphore ) );
#ifndef RTM
    if ( errMaintCacheSize >= JET_errSuccess )
    {
        OnNonRTM( g_tickMaintCacheStatsResizeLast = TickOSTimeCurrent() );
        OnNonRTM( g_cbfMaintCacheStatsResizeLast = cbfCacheTarget );
    }
#endif

#ifdef DEBUG
    if ( fAcquiredSemaphore )
    {
        g_cMaintCacheSizeReqAcquireFailures = 0;
    }
    else
    {

        S_ASSERT( dtickMaintCacheSizeRequest <= ( dtickMaintCacheStatsPeriod / 2 ) );

        g_cMaintCacheSizeReqAcquireFailures++;

        const TICK dtickOrphanedTaskThreshold = (TICK)UlConfigOverrideInjection( 57528, cmsecDeadlock );

        Expected( dtickOrphanedTaskThreshold >= dtickMaintCacheStatsPeriod );
        Expected( g_cMaintCacheSizeReqAcquireFailures <= ( dtickOrphanedTaskThreshold / dtickMaintCacheStatsPeriod ) );
    }
#endif

    if ( pvContext == fIdleCacheStatTask )
    {
        return;
    }

    BFIMaintIdleCacheStatsSchedule();


    BOOL fQuiesceTask = fFalse;
    if ( ( ( DtickDelta( g_tickLastCacheStatsRequest, tickNow ) > dtickMaintCacheStatsQuiesce ) || FBFIMaintCacheSizeQuiescedInSensitivityRange() ) &&
            ( ( cbfCacheDeadlock <= g_cbfCacheTargetOptimal ) || FUtilSystemRestrictIdleActivity() ) )
    {
        BOOL fLowMemory;
        if ( ErrOSQueryMemoryNotification( g_pMemoryNotification, &fLowMemory ) >= JET_errSuccess &&
             !fLowMemory &&
             ErrOSRegisterMemoryNotification( g_pMemoryNotification ) >= JET_errSuccess )
        {
            fQuiesceTask = fTrue;
        }
    }

    if ( !fQuiesceTask &&
         ErrBFIMaintScheduleTask( g_posttBFIMaintCacheStatsITask, NULL, dtickMaintCacheStatsPeriod, dtickMaintCacheStatsSlop ) >= JET_errSuccess )
    {
        return;
    }

    BFIMaintCacheStatsRelease();
}

TICK g_tickLastMaintCacheSizeRequestSuccess = 0;


INLINE ERR ErrBFIMaintCacheStatsRequest( const BFIMaintCacheStatsRequestType bfmcsrt )
{
    ERR err = JET_errSuccess;
    BOOL fReleaseSemaphore = fFalse;
    const BOOL fForce = ( bfmcsrt == bfmcsrtForce );
    const TICK tickNow = TickOSTimeCurrent();


    if ( DtickDelta( g_tickLastCacheStatsRequest, tickNow ) > 0 )
    {
        g_tickLastCacheStatsRequest = tickNow;
    }


    if ( !fForce && ( DtickDelta( g_tickLastMaintCacheStats, tickNow ) < dtickMaintCacheStatsTooOld ) )
    {
        goto HandleError;
    }


    if ( FBFIMaintCacheSizeQuiescedInSensitivityRange() && FBFICacheSizeFixed() )
    {
        goto HandleError;
    }


    if ( FBFIMaintCacheStatsTryAcquire() )
    {
        fReleaseSemaphore = fTrue;
        
        g_tickLastMaintCacheSizeRequestSuccess = TickOSTimeCurrent();


        BFIMaintIdleCacheStatsSchedule();


        Call( ErrBFIMaintScheduleTask( g_posttBFIMaintCacheStatsITask, NULL, dtickMaintCacheStatsPeriod, dtickMaintCacheStatsSlop ) );

        fReleaseSemaphore = fFalse;
        OSTrace( JET_tracetagBufferManagerMaintTasks, OSFormat( "%s:  started Cache Stats Collection", __FUNCTION__ ) );
    }

HandleError:

    if ( fReleaseSemaphore )
    {
        BFIMaintCacheStatsRelease();
    }

    return err;
}


INLINE BOOL FBFIMaintCacheStatsTryAcquire()
{
    return g_semMaintCacheStatsRequest.FTryAcquire();
}

INLINE void BFIMaintCacheStatsRelease()
{
    Assert( g_semMaintCacheStatsRequest.CAvail() == 0 );
    g_semMaintCacheStatsRequest.Release();
}

POSTIMERTASK    g_posttBFIMaintCacheSizeITask = NULL;

#ifndef RTM
TICK g_tickMaintCacheSizeRequestLast = 0;
TICK g_tickMaintCacheSizeRequestSyncLastAttempt = 0;
TICK g_tickMaintCacheSizeRequestSyncLastSuccess = 0;
TICK g_tickMaintCacheSizeRequestAsyncLastAttempt = 0;
TICK g_tickMaintCacheSizeRequestAsyncLastSuccess = 0;
#endif


INLINE ERR ErrBFIMaintCacheSizeRequest( OnDebug( BOOL* const pfAcquiredSemaphoreCheck ) )
{
    ERR err = JET_errSuccess;
    BOOL fReleaseSemaphore = fFalse;
    BOOL fNeedsAsyncSizing = fFalse;

    OnNonRTM( g_tickMaintCacheSizeRequestLast = TickOSTimeCurrent() );

    if ( !FBFIMaintCacheSizeAcquire() )
    {
        goto HandleError;
    }

    fReleaseSemaphore = fTrue;

    AtomicExchange( &g_cMaintCacheSizePending, 0 );

#ifdef DEBUG
    if ( pfAcquiredSemaphoreCheck != NULL )
    {
        *pfAcquiredSemaphoreCheck = fTrue;
    }
#endif


    const LONG_PTR cbfCacheTargetT = cbfCacheTarget;

    if ( cbfCacheTargetT > cbfCacheSize )
    {
        OnNonRTM( g_tickMaintCacheSizeRequestSyncLastAttempt = TickOSTimeCurrent() );
        

        if ( ErrBFICacheGrow() < JET_errSuccess )
        {
            fNeedsAsyncSizing = fTrue;
        }
        else
        {
            OnNonRTM( g_tickMaintCacheSizeRequestSyncLastSuccess = TickOSTimeCurrent() );
            goto HandleError;
        }
    }
    else if ( cbfCacheTargetT < cbfCacheSize )
    {
        fNeedsAsyncSizing = fTrue;
    }
    else if ( g_cbfCacheTargetOptimal < cbfCacheTargetT )
    {
        Assert( cbfCacheTargetT == cbfCacheSize );
        fNeedsAsyncSizing = fTrue;
    }


    if ( fNeedsAsyncSizing )
    {
        OnNonRTM( g_tickMaintCacheSizeRequestAsyncLastAttempt = TickOSTimeCurrent() );
        Call( ErrBFIMaintScheduleTask(  g_posttBFIMaintCacheSizeITask,
                                        NULL,
                                        dtickMaintCacheSizeRequest,
                                        0 ) );
        OnNonRTM( g_tickMaintCacheSizeRequestAsyncLastSuccess = TickOSTimeCurrent() );
        fReleaseSemaphore = fFalse;
        OSTrace( JET_tracetagBufferManagerMaintTasks, OSFormat( "%s:  started Cache Size Maintenance", __FUNCTION__ ) );
    }

HandleError:

    if ( fReleaseSemaphore )
    {
        const ERR errT = ErrBFIMaintCacheSizeReleaseAndRescheduleIfPending();

        if ( err >= JET_errSuccess )
        {
            err = errT;
        }
    }

    return err;
}

#ifndef RTM
TICK        g_tickMaintCacheSizeCacheTargetLastAttempt = 0;
LONG_PTR    g_cbfMaintCacheSizeCacheTargetLastAttempt = 0;
TICK        g_tickMaintCacheSizeCacheTargetLast = 0;
LONG_PTR    g_cbfMaintCacheSizeCacheTargetLast = 0;
TICK        g_tickMaintCacheSizeLastSuccess = 0;
TICK        g_tickMaintCacheSizeLastContinue = 0;
TICK        g_tickMaintCacheSizeLastFailedReschedule = 0;
#endif


void BFIMaintCacheSizeITask( void*, void* )
{
    OSTrace( JET_tracetagBufferManagerMaintTasks, __FUNCTION__ );

    Assert( g_fBFMaintInitialized );
    AtomicExchange( &g_cMaintCacheSizePending, 0 );
    Assert( g_semMaintCacheSize.CAvail() == 0  );
    OnDebug( g_cMaintCacheSizeReqAcquireFailures = 0 );

    g_tickLastMaintCacheSize = TickOSTimeCurrent();


    const LONG_PTR cbfCacheSizeStart = cbfCacheSize;
    const __int64 cbCacheSizeStart = CbBFICacheBufferSize();

    const LONG_PTR cbfCacheTargetT = cbfCacheTarget;

    OnNonRTM( g_tickMaintCacheSizeCacheTargetLastAttempt = TickOSTimeCurrent() );
    OnNonRTM( g_cbfMaintCacheSizeCacheTargetLastAttempt = cbfCacheTargetT );

    if ( cbfCacheTargetT > cbfCacheSize )
    {
        (void)ErrBFICacheGrow();
    }
    else if ( ( cbfCacheTargetT < cbfCacheSize ) ||
            ( g_cbfCacheTargetOptimal < cbfCacheTarget ) )
    {
        BFIMaintCacheSizeIShrink();
    }


    const BOOL fDone = ( cbfCacheSize == cbfCacheTarget ) && ( ( g_cbfCacheTargetOptimal >= cbfCacheTarget ) || FUtilSystemRestrictIdleActivity() );

    OnNonRTM( g_tickMaintCacheSizeCacheTargetLast = TickOSTimeCurrent() );
    OnNonRTM( g_cbfMaintCacheSizeCacheTargetLast = cbfCacheTarget );

    if ( fDone )
    {
        OnNonRTM( g_tickMaintCacheSizeLastSuccess = TickOSTimeCurrent() );
    }
    else
    {
        OnNonRTM( g_tickMaintCacheSizeLastContinue = TickOSTimeCurrent() );
    }

    const LONG_PTR cbfCacheSizeStop = cbfCacheSize;
    const __int64 cbCacheSizeStop = CbBFICacheBufferSize();

    Unused( cbfCacheSizeStart );
    Unused( cbCacheSizeStart );
    Unused( cbfCacheSizeStop );
    Unused( cbCacheSizeStop );

    OSTrace(
        JET_tracetagBufferManagerMaintTasks,
        OSFormat(
            "%s:  changed cache size by %I64d buffers (from %I64d to %I64d), %I64d bytes (from %I64d to %I64d)",
            __FUNCTION__,
            (__int64)( cbfCacheSizeStop - cbfCacheSizeStart ),
            (__int64)cbfCacheSizeStart,
            (__int64)cbfCacheSizeStop,
            cbCacheSizeStop - cbCacheSizeStart,
            cbCacheSizeStart,
            cbCacheSizeStop
             ) );


    if ( !fDone )
    {
        if ( ErrBFIMaintScheduleTask(   g_posttBFIMaintCacheSizeITask,
                                        NULL,
                                        dtickMaintCacheSizeRetry,
                                        0 ) < JET_errSuccess )
        {

            (void)ErrBFIMaintCacheSizeReleaseAndRescheduleIfPending();
            OnNonRTM( g_tickMaintCacheSizeLastFailedReschedule = TickOSTimeCurrent() );
        }
        else
        {
            OSTrace( JET_tracetagBufferManagerMaintTasks, OSFormat( "%s:  started Cache Size Maintenance", __FUNCTION__ ) );
        }
    }
    else
    {
        (void)ErrBFIMaintCacheSizeReleaseAndRescheduleIfPending();
    }

    Assert( g_fBFMaintInitialized );
}


void BFIReleaseBuffer( PBF pbf )
{
    Assert( g_critCacheSizeResize.FOwner() );


    Assert( IbfBFICachePbf( pbf ) < cbfCacheAddressable );
    Assert( !pbf->fQuiesced );

    AtomicDecrement( (LONG*)&g_cbfCacheClean );
    cbfCacheSize--;
    Assert( cbfCacheSize >= 0 );
    Assert( !pbf->fInOB0OL && pbf->ob0ic.FUninitialized() );

    OnDebug( const BOOL fWasAvailable = pbf->fAvailable );
    pbf->fAvailable = fFalse;
    pbf->fQuiesced = fTrue;
    g_bfquiesced.InsertAsNextMost( pbf );
    


    Assert( ( pbf->icbBuffer == icbPage0 ) || ( (DWORD)g_rgcbPageSize[pbf->icbBuffer] >= OSMemoryPageCommitGranularity() ) );
    if ( pbf->bfat == bfatFracCommit )
    {
        const BFResidenceState bfrsOld = BfrsBFIUpdateResidentState( pbf, bfrsNotCommitted );
        Expected( bfrsOld != bfrsNotCommitted || fWasAvailable );
        Assert( pbf->icbBuffer != icbPage0 || bfrsOld == bfrsNotCommitted );

        if ( pbf->icbBuffer != icbPage0 )
        {

            CallS( ErrBFISetBufferSize( pbf, icbPage0, fFalse ) );
        }
    }

    pbf->sxwl.ReleaseOwnership( bfltWrite );

    BFIAssertNewlyAllocatedPage( pbf, fTrue );
}


#ifndef RTM
TICK g_tickCacheShrinkLast = 0;
#endif

void BFIMaintCacheSizeIShrink()
{
    Assert( g_semMaintCacheSize.CAvail() == 0 );

    if ( !g_semMaintScavenge.FTryAcquire() )
    {
        return;
    }

    OnNonRTM( g_tickCacheShrinkLast = TickOSTimeCurrent() );
    (void)ErrBFIMaintScavengeIScavengePages( __FUNCTION__, fFalse );
    g_semMaintScavenge.Release();
}

#ifndef RTM
TICK g_tickMaintCacheSizeAcquireLast = 0;
TICK g_tickMaintCacheSizeReleaseLast = 0;
TICK g_tickMaintCacheSizeReleaseAndRescheduleLast = 0;
TICK g_tickMaintCacheSizeRescheduleLastAttempt = 0;
TICK g_tickMaintCacheSizeRescheduleLastSuccess = 0;
#endif

INLINE BOOL FBFIMaintCacheSizeAcquire()
{
    AtomicIncrement( &g_cMaintCacheSizePending );

    if ( g_semMaintCacheSize.FTryAcquire() )
    {
        g_tickMaintCacheSizeStartedLast = TickOSTimeCurrent();
        g_cbfMaintCacheSizeStartedLast = cbfCacheSize;
        OnNonRTM( g_tickMaintCacheSizeAcquireLast = g_tickMaintCacheSizeStartedLast );
        return fTrue;
    }

    return fFalse;
}

INLINE ERR ErrBFIMaintCacheSizeReleaseAndRescheduleIfPending()
{
    ERR err = JET_errSuccess;

    OnNonRTM( g_tickMaintCacheSizeReleaseAndRescheduleLast = TickOSTimeCurrent() );

    Assert( g_fBFMaintInitialized );
    Assert( g_semMaintCacheSize.CAvail() == 0 );
    g_tickMaintCacheSizeStartedLast = TickOSTimeCurrent();
    g_cbfMaintCacheSizeStartedLast = cbfMainCacheSizeStartedLastInactive;
    g_semMaintCacheSize.Release();
    OnNonRTM( g_tickMaintCacheSizeReleaseLast = TickOSTimeCurrent() );

    if ( g_cMaintCacheSizePending > 0 &&
            g_semMaintCacheSize.FTryAcquire() )
    {
        g_tickMaintCacheSizeStartedLast = TickOSTimeCurrent();
        g_cbfMaintCacheSizeStartedLast = cbfCacheSize;
        OnNonRTM( g_tickMaintCacheSizeRescheduleLastAttempt = g_tickMaintCacheSizeStartedLast );
        

        err = ErrBFIMaintScheduleTask(  g_posttBFIMaintCacheSizeITask,
                                        NULL,
                                        dtickMaintCacheSizeRequest,
                                        0 );


        if ( err < JET_errSuccess )
        {
            Assert( g_semMaintCacheSize.CAvail() == 0 );
            g_tickMaintCacheSizeStartedLast = TickOSTimeCurrent();
            g_cbfMaintCacheSizeStartedLast = cbfMainCacheSizeStartedLastInactive;
            g_semMaintCacheSize.Release();
            OnNonRTM( g_tickMaintCacheSizeReleaseLast = TickOSTimeCurrent() );
        }
        else
        {
            OnNonRTM( g_tickMaintCacheSizeRescheduleLastSuccess = TickOSTimeCurrent() );
            OSTrace( JET_tracetagBufferManagerMaintTasks, OSFormat( "%s:  started Cache Size Maintenance", __FUNCTION__ ) );
        }
    }

    Assert( g_fBFMaintInitialized );

    return err;
}

TICK DtickBFIMaintCacheSizeDuration()
{
    if ( g_fBFInitialized && g_semMaintCacheSize.CAvail() == 0 )
    {
        return DtickDelta( g_tickMaintCacheSizeStartedLast, TickOSTimeCurrent() );
    }
    else
    {
        return 0;
    }
}


CSemaphore      g_semMaintIdleDatabaseRequest( CSyncBasicInfo( _T( "g_semMaintIdleDatabaseRequest" ) ) );

TICK            g_tickMaintIdleDatabaseLast;

POSTIMERTASK    g_posttBFIMaintIdleDatabaseITask = NULL;

inline const char* BFFormatLGPOS( const LGPOS* const plgpos )
{
    return OSFormat(    "%05lX,%04hX,%04hX",
                        INT( plgpos->lGeneration ),
                        SHORT( plgpos->isec ),
                        SHORT( plgpos->ib ) );
}


void BFIMaintIdleDatabaseRequest( PBF pbf )
{

    FMP* const  pfmp            = &g_rgfmp[ pbf->ifmp ];
    const LGPOS lgposWaypoint   = pfmp->LgposWaypoint();
    
    if (    CmpLgpos( &pbf->lgposModify, &lgposMin ) != 0 &&
            CmpLgpos( &lgposWaypoint, &lgposMin ) != 0 &&
            CmpLgpos( &pbf->lgposModify, &lgposWaypoint ) >= 0 )
    {
        

        Assert( pfmp->FNotBFContextWriter() );
        CLockDeadlockDetectionInfo::DisableOwnershipTracking();
        CLockDeadlockDetectionInfo::DisableDeadlockDetection();

        BOOL fAcquiredBook = pfmp->FTryEnterBFContextAsReader();
        if ( fAcquiredBook )
        {
            BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext();
            Assert( pbffmp );

            if ( CmpLgpos( &pbf->lgposModify, &pbffmp->lgposNewestModify ) > 0 )
            {
                if ( pbf->lgposModify.lGeneration > pbffmp->lgposNewestModify.lGeneration )
                {
                    OSTrace(    JET_tracetagBufferManagerMaintTasks,
                                OSFormat(   "%s:  ipinst %s, ifmp %s has new pinned pages (lgposNewestModify = %s; lgposWaypoint = %s)",
                                            __FUNCTION__,
                                            OSFormatUnsigned( IpinstFromPinst( PinstFromIfmp( pbf->ifmp ) ) ),
                                            OSFormatUnsigned( pbf->ifmp ),
                                            BFFormatLGPOS( &pbf->lgposModify ),
                                            BFFormatLGPOS( &lgposWaypoint ) ) );
                }

                pbffmp->lgposNewestModify = pbf->lgposModify;
            }

            pfmp->LeaveBFContextAsReader();
        }

        CLockDeadlockDetectionInfo::EnableDeadlockDetection();
        CLockDeadlockDetectionInfo::EnableOwnershipTracking();
    }
    

    BOOL fAcquiredAsync = g_semMaintIdleDatabaseRequest.FTryAcquire();


    if ( fAcquiredAsync )
    {

        if ( ErrBFIMaintScheduleTask( g_posttBFIMaintIdleDatabaseITask, NULL, dtickMaintIdleDatabaseDelay, 0 ) >= JET_errSuccess )
        {
            OSTrace(    JET_tracetagBufferManagerMaintTasks,
                        OSFormat(   "%s:  Idle Database Maintenance Scheduled",
                                    __FUNCTION__ ) );


            fAcquiredAsync = fFalse;
        }
    }


    if ( fAcquiredAsync )
    {
        Assert( g_semMaintIdleDatabaseRequest.CAvail() == 0 );
        g_semMaintIdleDatabaseRequest.Release();
    }
}

void BFIMaintIdleDatabaseITask( void*, void* )
{

    BOOL fAcquiredAsync = fTrue;


    BFIMaintIdleDatabaseIRollLogs();


    g_tickMaintIdleDatabaseLast = TickOSTimeCurrent();


    if ( fAcquiredAsync )
    {
        Assert( g_semMaintIdleDatabaseRequest.CAvail() == 0 );
        g_semMaintIdleDatabaseRequest.Release();
    }
}

BOOL FBFIMaintIdleDatabaseIDatabaseHasPinnedPages( const INST * const pinst, const DBID dbid )
{
    const IFMP  ifmp    = pinst->m_mpdbidifmp[ dbid ];
    FMP* const  pfmp    = &g_rgfmp[ ifmp ];

    BOOL fPinnedPages = fFalse;
    
    if ( ifmp >= g_ifmpMax || !pfmp->FAttached() )
    {
        fPinnedPages = fFalse;
    }
    else
    {

        pfmp->EnterBFContextAsReader();
        
        const BFFMPContext* const pbffmp = (BFFMPContext*)pfmp->DwBFContext();
        
        const LGPOS lgposNewestModify = ( pbffmp && pbffmp->fCurrentlyAttached ) ? pbffmp->lgposNewestModify : lgposMin;
        
        pfmp->LeaveBFContextAsReader();


        const LGPOS lgposWaypoint   = pfmp->LgposWaypoint();


        if (    CmpLgpos( &lgposNewestModify, &lgposMin ) != 0 &&
                CmpLgpos( &lgposWaypoint, &lgposMin ) != 0 &&
                CmpLgpos( &lgposNewestModify, &lgposWaypoint ) >= 0 )
        {
            OSTrace(    JET_tracetagBufferManagerMaintTasks,
                        OSFormat(   "%s:  pinst %s, ifmp %s has pinned pages (lgposNewestModify = %s; lgposWaypoint = %s)",
                                    __FUNCTION__,
                                    OSFormatPointer( pinst ),
                                    OSFormatUnsigned( ifmp ),
                                    BFFormatLGPOS( &lgposNewestModify ),
                                    BFFormatLGPOS( &lgposWaypoint ) ) );
            fPinnedPages = fTrue;
        }
    }
    return fPinnedPages;
}

void BFIMaintIdleDatabaseIRollLogs( INST * const pinst )
{
    
    BOOL fPinnedPages = fFalse;
    for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
    {
        fPinnedPages = fPinnedPages || FBFIMaintIdleDatabaseIDatabaseHasPinnedPages( pinst, dbid );
    }


    TICK dtickLogRollMin = 24 * 60 * 60 * 1000;
    if ( FDefaultParam( pinst, JET_paramPeriodicLogRolloverLLR ) )
    {
        if ( UlParam( pinst, JET_paramWaypointLatency ) )
        {
            dtickLogRollMin = dtickMaintIdleDatabaseClearLLR / (ULONG)UlParam( pinst, JET_paramWaypointLatency );
            dtickLogRollMin = max( 30 * 1000, dtickLogRollMin );
        }
    }
    else
    {
        dtickLogRollMin = (ULONG)UlParam( pinst, JET_paramPeriodicLogRolloverLLR ) * 1000;
    }

    if ( fPinnedPages &&
         !pinst->m_plog->FLGRolloverInDuration( dtickLogRollMin ) )
    {
        OSTrace(    JET_tracetagBufferManagerMaintTasks,
                    OSFormat(   "%s:  pinst %s has pinned pages and hasn't rolled a log for %s ms (tickNow = %s)",
                                __FUNCTION__,
                                OSFormatPointer( pinst ),
                                OSFormatUnsigned( dtickLogRollMin ),
                                OSFormatUnsigned( TickOSTimeCurrent() ) ) );

        const ERR err = ErrLGForceLogRollover( pinst, __FUNCTION__ );
        if ( err < JET_errSuccess )
        {
            OSTrace(    JET_tracetagBufferManagerMaintTasks,
                        OSFormat(   "%s:  pinst %s failed to roll a new log with error %s",
                                    __FUNCTION__,
                                    OSFormatPointer( pinst ),
                                    OSFormatSigned( err ) ) );
        }
        else
        {
            OSTrace(    JET_tracetagBufferManagerMaintTasks,
                        OSFormat(   "%s:  pinst %s rolled a new log",
                                    __FUNCTION__,
                                    OSFormatPointer( pinst ) ) );
        }
    }
}
    
void BFIMaintIdleDatabaseIRollLogs()
{
    OSTrace(    JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "%s:  Beginning Idle Database Maintenance",
                            __FUNCTION__ ) );


    for ( size_t ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
    {
        extern CRITPOOL< INST* > g_critpoolPinstAPI;
        CCriticalSection * const pcritInst = &g_critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
        pcritInst->Enter();

        INST * const pinst = g_rgpinst[ ipinst ];
        
        if ( pinstNil == pinst )
        {
            pcritInst->Leave();
        }
        else
        {

            const BOOL fAPILocked = pinst->APILock( pinst->fAPICheckpointing );
            pcritInst->Leave();

            if ( fAPILocked )
            {
                if ( pinst->m_fJetInitialized )
                {
                    BFIMaintIdleDatabaseIRollLogs( pinst );
                }
                pinst->APIUnlock( pinst->fAPICheckpointing );
            }
        }
    }
    
    OSTrace(    JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "%s:  Ending Idle Database Maintenance",
                            __FUNCTION__ ) );
}



TICK g_tickLastUpdateStatisticsRequest = 0;
LONG g_fUpdateStatisticsMayRequest = fFalse;
BOOL g_fMaintCacheResidencyInit = fFalse;
BOOL g_fEnabledCacheResidencyTask = fTrue;

POSTIMERTASK g_posttBFIMaintCacheResidencyITask = NULL;

void BFIMaintCacheResidencyInit()
{
    Assert( !g_fUpdateStatisticsMayRequest );
    Assert( !g_fMaintCacheResidencyInit );

    const TICK tickNow = TickOSTimeCurrent();
    g_tickLastUpdateStatistics = tickNow - dtickMaintCacheResidencyTooOld;
    g_tickLastUpdateStatisticsRequest = tickNow - dtickMaintCacheResidencyQuiesce;
    g_fMaintCacheResidencyInit = fTrue;
    AtomicExchange( &g_fUpdateStatisticsMayRequest, fTrue );
}

void BFIMaintCacheResidencyTerm()
{

    g_fMaintCacheResidencyInit = fFalse;
    if ( g_posttBFIMaintCacheResidencyITask )
    {
        OSTimerTaskCancelTask( g_posttBFIMaintCacheResidencyITask );
    }
    AtomicExchange( &g_fUpdateStatisticsMayRequest, fFalse );

    Assert( !g_fMaintCacheResidencyInit );
    Assert( !g_fUpdateStatisticsMayRequest );
}

INLINE void BFIMaintCacheResidencyRequest()
{

    if ( BoolParam( JET_paramEnableViewCache ) )
    {
        return;
    }


    if ( g_fEnabledCacheResidencyTask &&
            ( g_fEnabledCacheResidencyTask = !FUtilSystemRestrictIdleActivity() ) )
    {
        const TICK tickNow = TickOSTimeCurrent();


        if ( DtickDelta( g_tickLastUpdateStatisticsRequest, tickNow ) >= dtickMaintCacheResidencyPeriod / 10 )
        {
            g_tickLastUpdateStatisticsRequest = tickNow;
        }


        if ( DtickDelta( g_tickLastUpdateStatistics, tickNow ) >= dtickMaintCacheResidencyTooOld &&
                AtomicCompareExchange( &g_fUpdateStatisticsMayRequest, fTrue, fFalse ) == fTrue )
        {
            if ( ErrBFIMaintScheduleTask(   g_posttBFIMaintCacheResidencyITask,
                                            NULL,
                                            dtickMaintCacheResidencyPeriod,
                                            0 ) < JET_errSuccess )
            {


                if ( g_fMaintCacheResidencyInit )
                {
                    AtomicExchange( &g_fUpdateStatisticsMayRequest, fTrue );
                }
            }
        }
    }
}


void BFIMaintCacheResidencyITask( void*, void* )
{
    OSTrace( JET_tracetagBufferManagerMaintTasks, __FUNCTION__ );
    
    Assert( !g_fUpdateStatisticsMayRequest );

    (void)ErrBFICacheUpdateStatistics();


    const ERR errForceQuiesce = ErrFaultInjection( 54947 );

    if ( DtickDelta( g_tickLastUpdateStatisticsRequest, TickOSTimeCurrent() ) >= dtickMaintCacheResidencyQuiesce ||
            errForceQuiesce != JET_errSuccess ||
            !( g_fEnabledCacheResidencyTask = !FUtilSystemRestrictIdleActivity() )
            )
    {
        AtomicExchange( &g_fUpdateStatisticsMayRequest, fTrue );
    }
    else if ( ErrBFIMaintScheduleTask( g_posttBFIMaintCacheResidencyITask,
                                       NULL,
                                       dtickMaintCacheResidencyPeriod,
                                       0 ) < JET_errSuccess )
    {
        AtomicExchange( &g_fUpdateStatisticsMayRequest, fTrue );
    }
}

INLINE void BFIUpdateResidencyStatsAfterResidencyFlag( const BFResidenceState bfrsOld, const BFResidenceState bfrsNew )
{

    Expected( bfrsOld != bfrsMax );
    Expected( bfrsNew != bfrsMax );
    

    Expected( !( bfrsOld == bfrsNotCommitted && bfrsNew == bfrsNotResident ) );
    Expected( !( bfrsOld == bfrsNewlyCommitted && bfrsNew == bfrsNewlyCommitted ) );
    Expected( !( bfrsOld == bfrsNewlyCommitted && bfrsNew == bfrsNotResident ) );
    Expected( !( bfrsOld == bfrsNotResident && bfrsNew == bfrsNewlyCommitted ) );
    Expected( !( bfrsOld == bfrsNotResident && bfrsNew == bfrsNotResident ) );
    Expected( !( bfrsOld == bfrsResident && bfrsNew == bfrsNewlyCommitted ) );


    if ( bfrsOld == bfrsNew )
    {
        return;
    }


    if ( bfrsNew == bfrsNotCommitted )
    {

        AtomicDecrement( (LONG*)&g_cbfCommitted );
    }
    else if ( bfrsOld == bfrsNotCommitted )
    {
 
        AtomicIncrement( (LONG*)&g_cbfCommitted );
    }


    if ( bfrsNew == bfrsNewlyCommitted )
    {

        AtomicIncrement( (LONG*)&g_cbfNewlyCommitted );
    }
    else if ( bfrsOld == bfrsNewlyCommitted )
    {

        AtomicDecrement( (LONG*)&g_cbfNewlyCommitted );
    }


    if ( bfrsNew == bfrsResident )
    {

        AtomicIncrement( &g_cbfCacheResident );
    }
    else if ( bfrsOld == bfrsResident )
    {

        AtomicDecrement( &g_cbfCacheResident );
    }
}

INLINE BFResidenceState BfrsBFIUpdateResidentState( PBF const pbf, const BFResidenceState bfrsNew )
{
    const BFResidenceState bfrsOld = (BFResidenceState)AtomicExchange( (LONG*)&pbf->bfrs, (LONG)bfrsNew );

    BFIUpdateResidencyStatsAfterResidencyFlag( bfrsOld, bfrsNew );
    
    return bfrsOld;
}

INLINE BFResidenceState BfrsBFIUpdateResidentState( PBF const pbf, const BFResidenceState bfrsNew, const BFResidenceState bfrsIfOld )
{
    const BFResidenceState bfrsOld = (BFResidenceState)AtomicCompareExchange( (LONG*)&pbf->bfrs, (LONG)bfrsIfOld, (LONG)bfrsNew );

    if ( bfrsOld == bfrsIfOld )
    {
        BFIUpdateResidencyStatsAfterResidencyFlag( bfrsOld, bfrsNew );
    }
    
    return bfrsOld;
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( BF, BFIUpdateResidencyStatsAfterResidencyFlag )
{
    const DWORD cbfCommittedInit = 50;
    const DWORD cbfNewlyCommittedInit = 10;
    const LONG cbfCacheResidentInit = 20;
    

    g_cbfCommitted = cbfCommittedInit;
    g_cbfNewlyCommitted = cbfNewlyCommittedInit;
    g_cbfCacheResident = cbfCacheResidentInit;
    BFIUpdateResidencyStatsAfterResidencyFlag( bfrsNotCommitted, bfrsNotCommitted );
    CHECK( g_cbfCommitted == cbfCommittedInit );
    CHECK( g_cbfNewlyCommitted == cbfNewlyCommittedInit );
    CHECK( g_cbfCacheResident == cbfCacheResidentInit );


    g_cbfCommitted = cbfCommittedInit;
    g_cbfNewlyCommitted = cbfNewlyCommittedInit;
    g_cbfCacheResident = cbfCacheResidentInit;
    BFIUpdateResidencyStatsAfterResidencyFlag( bfrsNotCommitted, bfrsNewlyCommitted );
    CHECK( g_cbfCommitted == cbfCommittedInit + 1 );
    CHECK( g_cbfNewlyCommitted == cbfNewlyCommittedInit + 1 );
    CHECK( g_cbfCacheResident == cbfCacheResidentInit );


    g_cbfCommitted = cbfCommittedInit;
    g_cbfNewlyCommitted = cbfNewlyCommittedInit;
    g_cbfCacheResident = cbfCacheResidentInit;
    BFIUpdateResidencyStatsAfterResidencyFlag( bfrsNotCommitted, bfrsResident );
    CHECK( g_cbfCommitted == cbfCommittedInit + 1 );
    CHECK( g_cbfNewlyCommitted == cbfNewlyCommittedInit );
    CHECK( g_cbfCacheResident == cbfCacheResidentInit + 1 );


    g_cbfCommitted = cbfCommittedInit;
    g_cbfNewlyCommitted = cbfNewlyCommittedInit;
    g_cbfCacheResident = cbfCacheResidentInit;
    BFIUpdateResidencyStatsAfterResidencyFlag( bfrsNewlyCommitted, bfrsNotCommitted );
    CHECK( g_cbfCommitted == cbfCommittedInit - 1 );
    CHECK( g_cbfNewlyCommitted == cbfNewlyCommittedInit - 1 );
    CHECK( g_cbfCacheResident == cbfCacheResidentInit );


    g_cbfCommitted = cbfCommittedInit;
    g_cbfNewlyCommitted = cbfNewlyCommittedInit;
    g_cbfCacheResident = cbfCacheResidentInit;
    BFIUpdateResidencyStatsAfterResidencyFlag( bfrsNewlyCommitted, bfrsResident );
    CHECK( g_cbfCommitted == cbfCommittedInit );
    CHECK( g_cbfNewlyCommitted == cbfNewlyCommittedInit - 1 );
    CHECK( g_cbfCacheResident == cbfCacheResidentInit + 1 );


    g_cbfCommitted = cbfCommittedInit;
    g_cbfNewlyCommitted = cbfNewlyCommittedInit;
    g_cbfCacheResident = cbfCacheResidentInit;
    BFIUpdateResidencyStatsAfterResidencyFlag( bfrsNotResident, bfrsNotCommitted );
    CHECK( g_cbfCommitted == cbfCommittedInit - 1 );
    CHECK( g_cbfNewlyCommitted == cbfNewlyCommittedInit );
    CHECK( g_cbfCacheResident == cbfCacheResidentInit );


    g_cbfCommitted = cbfCommittedInit;
    g_cbfNewlyCommitted = cbfNewlyCommittedInit;
    g_cbfCacheResident = cbfCacheResidentInit;
    BFIUpdateResidencyStatsAfterResidencyFlag( bfrsNotResident, bfrsResident );
    CHECK( g_cbfCommitted == cbfCommittedInit );
    CHECK( g_cbfNewlyCommitted == cbfNewlyCommittedInit );
    CHECK( g_cbfCacheResident == cbfCacheResidentInit + 1 );


    g_cbfCommitted = cbfCommittedInit;
    g_cbfNewlyCommitted = cbfNewlyCommittedInit;
    g_cbfCacheResident = cbfCacheResidentInit;
    BFIUpdateResidencyStatsAfterResidencyFlag( bfrsResident, bfrsNotCommitted );
    CHECK( g_cbfCommitted == cbfCommittedInit - 1 );
    CHECK( g_cbfNewlyCommitted == cbfNewlyCommittedInit );
    CHECK( g_cbfCacheResident == cbfCacheResidentInit - 1 );


    g_cbfCommitted = cbfCommittedInit;
    g_cbfNewlyCommitted = cbfNewlyCommittedInit;
    g_cbfCacheResident = cbfCacheResidentInit;
    BFIUpdateResidencyStatsAfterResidencyFlag( bfrsResident, bfrsNotResident );
    CHECK( g_cbfCommitted == cbfCommittedInit );
    CHECK( g_cbfNewlyCommitted == cbfNewlyCommittedInit );
    CHECK( g_cbfCacheResident == cbfCacheResidentInit - 1 );


    g_cbfCommitted = cbfCommittedInit;
    g_cbfNewlyCommitted = cbfNewlyCommittedInit;
    g_cbfCacheResident = cbfCacheResidentInit;
    BFIUpdateResidencyStatsAfterResidencyFlag( bfrsResident, bfrsResident );
    CHECK( g_cbfCommitted == cbfCommittedInit );
    CHECK( g_cbfNewlyCommitted == cbfNewlyCommittedInit );
    CHECK( g_cbfCacheResident == cbfCacheResidentInit );
}

JETUNITTEST( BF, BfrsBFISwitchBFResidencyFlagForced )
{
    BF bf;
    const DWORD cbfCommittedInit = 50;
    const DWORD cbfNewlyCommittedInit = 10;
    const LONG cbfCacheResidentInit = 20;
    
    bf.bfrs = bfrsNotCommitted;
    g_cbfCommitted = cbfCommittedInit;
    g_cbfNewlyCommitted = cbfNewlyCommittedInit;
    g_cbfCacheResident = cbfCacheResidentInit;

    const BFResidenceState bfrsOld = BfrsBFIUpdateResidentState( &bf, bfrsNewlyCommitted );

    CHECK( bfrsOld == bfrsNotCommitted );
    CHECK( bf.bfrs == bfrsNewlyCommitted );
    CHECK( g_cbfCommitted == cbfCommittedInit + 1 );
    CHECK( g_cbfNewlyCommitted == cbfNewlyCommittedInit + 1 );
    CHECK( g_cbfCacheResident == cbfCacheResidentInit );
}

JETUNITTEST( BF, BfrsBFISwitchBFResidencyFlagConditional )
{
    BF bf;
    const DWORD cbfCommittedInit = 50;
    const DWORD cbfNewlyCommittedInit = 10;
    const LONG cbfCacheResidentInit = 20;

    
    bf.bfrs = bfrsNewlyCommitted;
    g_cbfCommitted = cbfCommittedInit;
    g_cbfNewlyCommitted = cbfNewlyCommittedInit;
    g_cbfCacheResident = cbfCacheResidentInit;

    const BFResidenceState bfrsOldUnchanged = BfrsBFIUpdateResidentState( &bf, bfrsNotCommitted, bfrsResident );

    CHECK( bfrsOldUnchanged == bfrsNewlyCommitted );
    CHECK( bf.bfrs == bfrsNewlyCommitted );
    CHECK( g_cbfCommitted == cbfCommittedInit );
    CHECK( g_cbfNewlyCommitted == cbfNewlyCommittedInit );
    CHECK( g_cbfCacheResident == cbfCacheResidentInit );


    bf.bfrs = bfrsResident;
    g_cbfNewlyCommitted = cbfNewlyCommittedInit;
    g_cbfCacheResident = cbfCacheResidentInit;

    const BFResidenceState bfrsOldChanged = BfrsBFIUpdateResidentState( &bf, bfrsNotCommitted, bfrsResident );

    CHECK( bfrsOldChanged == bfrsResident );
    CHECK( bf.bfrs == bfrsNotCommitted );
    CHECK( g_cbfCommitted == cbfCommittedInit - 1 );
    CHECK( g_cbfNewlyCommitted == cbfNewlyCommittedInit );
    CHECK( g_cbfCacheResident == cbfCacheResidentInit - 1 );
}
#endif


POSTIMERTASK    g_posttBFIMaintTelemetryITask = NULL;

void BFIMaintTelemetryRequest()
{
    if ( !( FOSEventTraceEnabled< _etguidCacheMemoryUsage >() ||
            (BOOL)UlConfigOverrideInjection( 56322, fFalse ) ) )
    {
        return;
    }

    (VOID)ErrBFIMaintScheduleTask(
        g_posttBFIMaintTelemetryITask,
        NULL,
        dtickTelemetryPeriod,
        0);
}

void BFIMaintTelemetryITask( VOID *, VOID * pvContext )
{

    LONGLONG** rgrgcbCacheByIfmpTce = new LONGLONG*[ g_ifmpMax ];
    TICK** rgrgtickMinByIfmpTce = new TICK*[ g_ifmpMax ];
    for ( IFMP ifmpT = 0; ifmpT < g_ifmpMax; ifmpT++ )
    {
        rgrgcbCacheByIfmpTce[ ifmpT ] = NULL;
        rgrgtickMinByIfmpTce[ ifmpT ] = NULL;
    }

    const TICK tickNow = TickOSTimeCurrent();

    for ( IBF ibf = 0; ibf < cbfInit; ibf++ )
    {
        PBF pbf = PbfBFICacheIbf( ibf );

        
        if ( pbf->sxwl.ErrTryAcquireSharedLatch() != CSXWLatch::ERR::errSuccess )
        {
            continue;
        }

        IFMP ifmp = pbf->ifmp;


        if ( ifmp == ifmpNil || pbf->pgno == pgnoNull || pbf->bfrs != bfrsResident )
        {
            pbf->sxwl.ReleaseSharedLatch();
            continue;
        }


        if ( rgrgcbCacheByIfmpTce[ ifmp ] == NULL )
        {
            rgrgcbCacheByIfmpTce[ ifmp ] = new LONGLONG[ tceMax ];
            rgrgtickMinByIfmpTce[ ifmp ] = new TICK[ tceMax ];
            for ( int tceT = 0; tceT < tceMax; tceT++ )
            {
                rgrgcbCacheByIfmpTce[ ifmp ][ tceT ] = 0;
                rgrgtickMinByIfmpTce[ ifmp ][ tceT ] = tickNow;
            }
        }


        rgrgcbCacheByIfmpTce[ ifmp ][ pbf->tce ] += g_rgcbPageSize[ pbf->icbBuffer ];


        if ( pbf->lrukic.FSuperColded() == fFalse && pbf->bfdf <= bfdfUntidy )
        {
            rgrgtickMinByIfmpTce[ ifmp ][ pbf->tce ] = TickMin( rgrgtickMinByIfmpTce[ ifmp ][ pbf->tce ],
                                                                pbf->lrukic.TickLastTouchTime() );
        }

        pbf->sxwl.ReleaseSharedLatch();
    }


    for ( IFMP ifmp = 0; ifmp < g_ifmpMax; ifmp++ )
    {
        if ( !rgrgcbCacheByIfmpTce[ ifmp ] )
        {
            continue;
        }

        FMP* pfmp = &g_rgfmp[ ifmp ];

        pfmp->EnterBFContextAsReader();
        BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext();

        if ( pbffmp && pbffmp->fCurrentlyAttached )
        {
            for ( TCE tce = 0; tce < tceMax; tce++ )
            {
                if ( rgrgcbCacheByIfmpTce[ ifmp ][ tce ] == 0 )
                {
                    continue;
                }

                ETCacheMemoryUsage(
                    pfmp->WszDatabaseName(),
                    tce,
                    PinstFromIfmp( ifmp )->FRecovering() ? iofileDbRecovery : iofileDbAttached,
                    ifmp,
                    rgrgcbCacheByIfmpTce[ ifmp ][ tce ],
                    TickCmp( tickNow, rgrgtickMinByIfmpTce[ ifmp ][ tce ] ) >= 0 ?
                        DtickDelta( rgrgtickMinByIfmpTce[ ifmp ][ tce ], tickNow ) :
                        0 );
            }
        }

        pfmp->LeaveBFContextAsReader();
    }


    for ( IFMP ifmpT = 0; ifmpT < g_ifmpMax; ifmpT++ )
    {
        delete[] rgrgcbCacheByIfmpTce[ ifmpT ];
        delete[] rgrgtickMinByIfmpTce[ ifmpT ];
    }

    delete[] rgrgcbCacheByIfmpTce;
    delete[] rgrgtickMinByIfmpTce;

    BFIMaintTelemetryRequest();
}




#ifdef DEBUG

INLINE BOOL FBFILatchValidContext( const DWORD_PTR dwContext )
{

    if ( dwContext & 1 )
    {
        BFHashedLatch* const    pbfhl   = (BFHashedLatch*)( dwContext ^ 1 );
        const size_t            cProcs  = (size_t)OSSyncGetProcessorCountMax();

        for ( size_t iProc = 0; iProc < cProcs; iProc++ )
        {
            PLS* const ppls = Ppls( iProc );

            if (    ppls->rgBFHashedLatch <= pbfhl && pbfhl < ppls->rgBFHashedLatch + cBFHashedLatch &&
                    ( DWORD_PTR( pbfhl ) - DWORD_PTR( ppls->rgBFHashedLatch ) ) % sizeof( BFHashedLatch ) == 0 )
            {
                return fTrue;
            }
        }
        return fFalse;
    }


    else
    {
        return FBFICacheValidPbf( PBF( dwContext ) );
    }
}

#endif

INLINE PBF PbfBFILatchContext( const DWORD_PTR dwContext )
{

    if ( dwContext & 1 )
    {
        return ((BFHashedLatch*)( dwContext ^ 1 ))->pbf;
    }


    else
    {
        return PBF( dwContext );
    }
}

INLINE CSXWLatch* PsxwlBFILatchContext( const DWORD_PTR dwContext )
{

    if ( dwContext & 1 )
    {
        return &((BFHashedLatch*)( dwContext ^ 1 ))->sxwl;
    }


    else
    {
        return &PBF( dwContext )->sxwl;
    }
}

void BFILatchNominate( const PBF pbf )
{

    Assert( pbf->sxwl.FNotOwnExclusiveLatch() && pbf->sxwl.FNotOwnWriteLatch() );
    Assert( pbf->bfat == bfatFracCommit );

#ifdef MINIMAL_FUNCTIONALITY
#else


    CLockDeadlockDetectionInfo::DisableOwnershipTracking();


    if (    pbf->bfls == bflsNormal &&
            TickCmp( TickOSTimeCurrent(), pbf->tickEligibleForNomination ) > 0 &&
            g_bflruk.FSuperHotResource( pbf ) &&
            pbf->sxwl.ErrTryAcquireExclusiveLatch() == CSXWLatch::ERR::errSuccess )
    {

        if ( pbf->bfls == bflsNormal )
        {

            PLS* const ppls = Ppls();
            size_t iNominee;
            for ( iNominee = 1; iNominee < cBFNominee; iNominee++ )
            {
                if ( ppls->rgBFNominee[ iNominee ].pbf == pbfNil )
                {
                    if ( AtomicCompareExchangePointer(  (void**)&Ppls( 0 )->rgBFNominee[ iNominee ].pbf,
                                                        pbfNil,
                                                        pbf ) == pbfNil )
                    {
                        break;
                    }
                }
            }


            if ( iNominee < cBFNominee )
            {

                const size_t    cProcs  = (size_t)OSSyncGetProcessorCountMax();
                for ( size_t iProc = 0; iProc < cProcs; iProc++ )
                {
                    BFNominee* const pbfn = &Ppls( iProc )->rgBFNominee[ iNominee ];
                    pbfn->pbf = pbf;
                }


                Assert( bflsNominee1 <= iNominee && iNominee <= bflsNominee3 );
                pbf->bfls = iNominee;
                OSTrace(
                    JET_tracetagBufferManagerHashedLatches,
                    OSFormat( "BF %s nominated and placed in slot %d", OSFormatPointer( pbf ), (ULONG)iNominee ) );
            }


            else
            {

                pbf->tickEligibleForNomination = TickBFIHashedLatchTime( TickOSTimeCurrent() + dtickMaintHashedLatchesPeriod );
            }
        }


        pbf->sxwl.ReleaseExclusiveLatch();
    }


    CLockDeadlockDetectionInfo::EnableOwnershipTracking();


    const BFLatchState bfls = BFLatchState( pbf->bfls );
    if ( bflsNominee1 <= bfls && bfls <= bflsNominee3 )
    {
        Ppls()->rgBFNominee[ bfls ].cCacheReq++;
    }

#endif

}

BOOL FBFILatchDemote( const PBF pbf )
{

    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );

#ifdef MINIMAL_FUNCTIONALITY
#else


    if ( pbf->bfls == bflsHashed )
    {
        Assert( pbf->bfat == bfatFracCommit );


        const ULONG     iHashedLatch    = pbf->iHashedLatch;
        const size_t    cProcs          = (size_t)OSSyncGetProcessorCountMax();

        size_t iProc;
        for ( iProc = 0; iProc < cProcs; iProc++ )
        {
            CSXWLatch* const psxwlProc = &Ppls( iProc )->rgBFHashedLatch[ iHashedLatch ].sxwl;
            if ( psxwlProc->ErrTryAcquireWriteLatch() == CSXWLatch::ERR::errLatchConflict )
            {
                break;
            }
        }


        if ( iProc == cProcs )
        {

            pbf->bfls = bflsNormal;


            pbf->tickEligibleForNomination = TickBFIHashedLatchTime( TickOSTimeCurrent() );


            for ( size_t iProc3 = 0; iProc3 < cProcs; iProc3++ )
            {
                BFHashedLatch* const pbfhl = &Ppls( iProc3 )->rgBFHashedLatch[ iHashedLatch ];
                pbfhl->pbf = pbfNil;
            }
        }


        for ( size_t iProc2 = 0; iProc2 < iProc; iProc2++ )
        {
            CSXWLatch* const psxwlProc = &Ppls( iProc2 )->rgBFHashedLatch[ iHashedLatch ].sxwl;
            psxwlProc->ReleaseWriteLatch();
        }
    }

#endif


    return pbf->bfls != bflsHashed;
}


ERR ErrBFISetupBFFMPContext( IFMP ifmp )
{
    ERR     err = JET_errSuccess;
    FMP*    pfmp = &g_rgfmp[ifmp];

    if ( !pfmp->FBFContext() )
    {
        pfmp->EnterBFContextAsWriter();
        if ( !pfmp->FBFContext() )
        {
            BYTE* rgbBFFMPContext = (BYTE*)PvOSMemoryHeapAllocAlign( sizeof( BFFMPContext ), cbCacheLine );
            if ( !rgbBFFMPContext )
            {
                pfmp->LeaveBFContextAsWriter();
                Call( ErrERRCheck( JET_errOutOfMemory ) );
            }
            BFFMPContext* pbffmp = new(rgbBFFMPContext)BFFMPContext();

            Assert( UlParam( PinstFromIfmp( ifmp ), JET_paramCheckpointTooDeep ) >= lgenCheckpointTooDeepMin );
            Assert( UlParam( PinstFromIfmp( ifmp ), JET_paramCheckpointTooDeep ) <= lgenCheckpointTooDeepMax );

            LGPOS lgposFileSize = { 0, 0, 1 };

            BFOB0::ERR errOB0 = pbffmp->bfob0.ErrInit(
                BFIOB0Offset( ifmp, &lgposFileSize ) * 2 * (LONG)UlParam( PinstFromIfmp( ifmp ), JET_paramCheckpointTooDeep ),
                cbCheckpointTooDeepUncertainty,
                g_dblBFSpeedSizeTradeoff );
            if ( errOB0 != BFOB0::ERR::errSuccess )
            {
                Assert( errOB0 == BFOB0::ERR::errOutOfMemory );

                pbffmp->BFFMPContext::~BFFMPContext();
                OSMemoryHeapFreeAlign( pbffmp );

                pfmp->LeaveBFContextAsWriter();
                Call( ErrERRCheck( JET_errOutOfMemory ) );
            }

            DWORD_PTR rgp[2] = { ifmp, (DWORD_PTR)pbffmp };
            OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlBfCreateContext | sysosrtlContextFmp, pfmp, rgp, sizeof( rgp ) );

            pbffmp->fCurrentlyAttached = fTrue;
            pfmp->SetDwBFContext( DWORD_PTR( pbffmp ) );
        }
        pfmp->LeaveBFContextAsWriter();
    }

HandleError:
    return err;
}


ERR ErrBFISetBufferSize( __inout PBF pbf, __in const ICBPage icbNewSize, __in const BOOL fWait )
{
    ERR err = JET_errSuccess;
    
    Enforce( icbPageInvalid != icbNewSize );

    Assert( pbf->sxwl.FOwnWriteLatch() );

    if ( BoolParam( JET_paramEnableViewCache ) )
    {
        Assert( pbf->icbBuffer == icbNewSize );
    }

    const size_t cbBufferOld = g_rgcbPageSize[pbf->icbBuffer];
    const size_t cbBufferNew = g_rgcbPageSize[icbNewSize];

    if ( cbBufferNew < cbBufferOld )
    {


        if ( pbf->bfat == bfatFracCommit )
        {
            Expected( !BoolParam( JET_paramEnableViewCache ) );

            Assert( 0 == ( cbBufferOld % OSMemoryPageCommitGranularity() ) );
            Assert( 0 == ( cbBufferNew % OSMemoryPageCommitGranularity() ) );


            Assert( (INT)cbBufferOld - (INT)cbBufferNew > 0 );
            Assert( 0 == ( ( (INT)cbBufferOld - (INT)cbBufferNew ) % OSMemoryPageCommitGranularity() ) );


            OSMemoryPageDecommit( ((BYTE*)((pbf)->pv))+cbBufferNew, cbBufferOld - cbBufferNew );

            OnDebug( const LONG_PTR cbCacheCommittedSizeInitial = (LONG_PTR)) AtomicExchangeAddPointer( (void**)&g_cbCacheCommittedSize, (void*)( -( (LONG_PTR)( cbBufferOld - cbBufferNew ) ) ) );
            Assert( cbCacheCommittedSizeInitial >= (LONG_PTR)( cbBufferOld - cbBufferNew ) );


            if ( !pbf->fAvailable && !pbf->fQuiesced )
            {
                Assert( ( cbBufferOld != 0 ) && ( cbBufferNew != 0 ) );
                AtomicDecrement( &g_rgcbfCachePages[pbf->icbBuffer] );
                Assert( g_rgcbfCachePages[pbf->icbBuffer] >= 0 );
                AtomicIncrement( &g_rgcbfCachePages[icbNewSize] );
                Assert( g_rgcbfCachePages[icbNewSize] > 0 );
            }

            Assert( !pbf->fQuiesced || ( icbNewSize == icbPage0 ) );
            Expected( ( icbNewSize != icbPage0 ) || pbf->fQuiesced || pbf->fAvailable );

            pbf->icbBuffer = icbNewSize;
        }
    }
    else if ( cbBufferNew > cbBufferOld )
    {

        const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );
        if ( fWait )
        {
            const LONG cRFSCountdownOld = RFSThreadDisable( 10 );
            while( !FOSMemoryPageCommit( ((BYTE*)((pbf)->pv))+cbBufferOld, cbBufferNew - cbBufferOld ) )
            {
            }
            RFSThreadReEnable( cRFSCountdownOld );
        }
        else if ( !FOSMemoryPageCommit( ((BYTE*)((pbf)->pv))+cbBufferOld, cbBufferNew - cbBufferOld ) )
        {
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }
        FOSSetCleanupState( fCleanUpStateSaved );

        OnDebug( const LONG_PTR cbCacheCommittedSizeInitial = (LONG_PTR)) AtomicExchangeAddPointer( (void**)&g_cbCacheCommittedSize, (void*)( cbBufferNew - cbBufferOld ) );
        Assert( cbCacheCommittedSizeInitial >= 0 );


        if ( !pbf->fAvailable && !pbf->fQuiesced )
        {
            Assert( ( cbBufferOld != 0 ) && ( cbBufferNew != 0 ) );
            AtomicDecrement( &g_rgcbfCachePages[pbf->icbBuffer] );
            Assert( g_rgcbfCachePages[pbf->icbBuffer] >= 0 );
            AtomicIncrement( &g_rgcbfCachePages[icbNewSize] );
            Assert( g_rgcbfCachePages[icbNewSize] > 0 );
        }


        Expected( ( pbf->icbBuffer != icbPage0 ) || pbf->fAvailable );

        pbf->icbBuffer = icbNewSize;

        BFIFaultInBuffer( pbf->pv, g_rgcbPageSize[icbNewSize] );
    }
    else
    {

        FireWall( "UnnecessaryBufferSizeChange" );
    }

    Assert( pbf->icbBuffer == icbNewSize );

HandleError:

    if ( err != JET_errSuccess )
    {
        Assert( ( err == JET_errOutOfMemory ) && ( cbBufferNew > cbBufferOld ) && !fWait );
    }

    return err;
}

#ifdef DEBUG
void BFIAssertNewlyAllocatedPage( const PBF pbfNew, const BOOL fAvailPoolAdd )
{
    Assert( pbfNew );


    Assert( !pbfNew->fAvailable );

    Assert( FBFIValidPvAllocType( pbfNew ) );
    Assert( pbfNew->bfat == bfatFracCommit || ( UlParam( JET_paramEnableViewCache ) && pbfNew->bfat == bfatNone ) );
    Assert( UlParam( JET_paramEnableViewCache ) || ( pbfNew->pv != NULL ) );

    Assert( pbfNew->ifmp == ifmpNil );

    Assert( pbfNew->err == JET_errSuccess );

    Assert( !pbfNew->bfbitfield.FRangeLocked() );
    Assert( pbfNew->pWriteSignalComplete == NULL );
    Assert( PvBFIAcquireIOContext( pbfNew ) == NULL );

    Assert( pbfNew->prceUndoInfoNext == prceNil );

    Assert( !pbfNew->fNewlyEvicted || fAvailPoolAdd );

    Assert( !pbfNew->fCurrentVersion );
    Assert( !pbfNew->fOlderVersion );
    Assert( !pbfNew->bfbitfield.FDependentPurged() );
    Assert( pbfNew->pbfTimeDepChainPrev == pbfNil );
    Assert( pbfNew->pbfTimeDepChainNext == pbfNil );

    Assert( !pbfNew->fAbandoned );

    Assert( !pbfNew->fWARLatch );

    Assert( pbfNew->bfdf == bfdfClean );
    Assert( 0 == CmpLgpos( &(pbfNew->lgposModify), &lgposMin ) );
    Assert( 0 == CmpRbspos( pbfNew->rbsposSnapshot, rbsposMin ) );
    Assert( 0 == CmpLgpos( &(pbfNew->lgposOldestBegin0), &lgposMax ) );
    Assert( !pbfNew->fInOB0OL && pbfNew->ob0ic.FUninitialized() );

    Assert( !pbfNew->bfbitfield.FImpedingCheckpoint() );
    Assert( !pbfNew->fSuspiciouslySlowRead );

}
#else
INLINE void BFIAssertNewlyAllocatedPage( const PBF pbfNew, const BOOL fAvailPoolAdd ) { return; }
#endif


ERR ErrBFIAllocPage( PBF* const ppbf, __in const ICBPage icbBufferSize, const BOOL fWait, const BOOL fMRU )
{
    Assert( icbBufferSize > icbPageInvalid );
    Assert( icbBufferSize < icbPageMax );
    Assert( icbBufferSize <= g_icbCacheMax );
    Assert( ppbf );


    static_assert( dtickMaintAvailPoolOomRetry >= 16, "Can't be smaller than the default Windows timer resolution." );
    ULONG cOutOfMemoryRetriesMax = roundupdiv( g_dtickMaintScavengeTimeout, dtickMaintAvailPoolOomRetry );
    cOutOfMemoryRetriesMax = UlFunctionalMax( cOutOfMemoryRetriesMax, 10 );
    ULONG cOutOfMemoryRetries = 0;
    while ( fTrue )
    {
        *ppbf = pbfNil;
        BFAvail::ERR errAvail;
        ERR errAvailPoolRequest = JET_errSuccess;
        if ( ( errAvail = g_bfavail.ErrRemove( ppbf, cmsecTest, fMRU ) ) != BFAvail::ERR::errSuccess )
        {
            Assert( errAvail == BFAvail::ERR::errOutOfObjects );

            do
            {
                BFICacheSizeBoost();
                errAvail = g_bfavail.ErrRemove( ppbf, cmsecTest, fMRU );

                if ( errAvail == BFAvail::ERR::errSuccess )
                {
                    Assert( *ppbf != pbfNil );
                    break;
                }

                Assert( errAvail == BFAvail::ERR::errOutOfObjects );
                errAvailPoolRequest = ErrBFIMaintAvailPoolRequest( bfmaprtSync );


                if ( ( errAvailPoolRequest != JET_errSuccess ) && fWait )
                {
                    g_critCacheSizeSetTarget.Enter();
                    g_cacheram.UpdateStatistics();
                    g_cacheram.SetOptimalResourcePoolSize();
                    g_critCacheSizeSetTarget.Leave();

                    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );
                    const ERR errCacheResize = ErrBFICacheGrow();
                    FOSSetCleanupState( fCleanUpStateSaved );
                    if ( ( errAvailPoolRequest == JET_errOutOfMemory ) ||
                        ( ( errAvailPoolRequest > JET_errSuccess ) && ( errCacheResize == JET_errOutOfMemory ) ) )
                    {
                        cOutOfMemoryRetries++;
                        if ( FRFSAnyFailureDetected() )
                        {
                            cOutOfMemoryRetriesMax = 10;
                        }
                        if ( cOutOfMemoryRetries > cOutOfMemoryRetriesMax )
                        {
                            errAvailPoolRequest = ErrERRCheck( JET_errOutOfMemory );
                        }
                        else
                        {
                            UtilSleep( dtickMaintAvailPoolOomRetry );
                        }
                    }
                    else
                    {
                        cOutOfMemoryRetries = 0;
                    }
                }

                errAvail = g_bfavail.ErrRemove( ppbf, fWait ? dtickFastRetry : cmsecTest, fMRU );
            } while ( ( errAvail != BFAvail::ERR::errSuccess ) && ( errAvailPoolRequest >= JET_errSuccess ) && fWait );
        }

        if ( *ppbf == pbfNil )
        {
            Assert( errAvail == BFAvail::ERR::errOutOfObjects );

            if ( fWait )
            {
                Assert( errAvailPoolRequest < JET_errSuccess );
                if ( !FRFSAnyFailureDetected() )
                {
                    FireWall( "AvailPoolMaintStalled" );
                }
                
                return errAvailPoolRequest;
            }
            else
            {
                return ErrERRCheck( errBFINoBufferAvailable );
            }
        }
        else
        {
            Assert( errAvail == BFAvail::ERR::errSuccess );
        }

        Assert( !(*ppbf)->fInOB0OL && (*ppbf)->ob0ic.FUninitialized() );


        (*ppbf)->sxwl.ClaimOwnership( bfltWrite );
        Assert( (*ppbf)->sxwl.FOwnWriteLatch() );
        Enforce( (*ppbf)->fAvailable );


        AssertTrack( !(*ppbf)->bfbitfield.FRangeLocked(), "BFAllocRangeAlreadyLocked" );
        Enforce( (*ppbf)->err != errBFIPageFaultPending );
        Enforce( (*ppbf)->err != wrnBFPageFlushPending );
        Enforce( (*ppbf)->pWriteSignalComplete == NULL );
        Enforce( PvBFIAcquireIOContext( *ppbf ) == NULL );

        Enforce( (*ppbf)->pbfTimeDepChainNext == NULL );
        Enforce( (*ppbf)->pbfTimeDepChainPrev == NULL );
        Assert( (*ppbf)->bfdf == bfdfClean );
        Assert( (*ppbf)->fOlderVersion == fFalse );
        Assert( (*ppbf)->fSuspiciouslySlowRead == fFalse );

        if ( (*ppbf)->bfat == bfatNone )
        {
            Expected( BoolParam( JET_paramEnableViewCache ) );
            Assert( (*ppbf)->pv == NULL );
        }
        else
        {
            Assert( (*ppbf)->bfat == bfatFracCommit );
            Assert( (*ppbf)->pv != NULL );
            Expected( (*ppbf)->icbBuffer == icbPage0 || FOSMemoryPageAllocated( (*ppbf)->pv, g_rgcbPageSize[(*ppbf)->icbBuffer] ) );
        }


        if ( (*ppbf)->icbBuffer == icbBufferSize )
        {
            break;
        }

        const ERR errSetBufferSize = ErrBFISetBufferSize( *ppbf, icbBufferSize, fFalse );


        if ( errSetBufferSize >= JET_errSuccess )
        {
            break;
        }


        Assert( errSetBufferSize == JET_errOutOfMemory );
        Assert( icbBufferSize > (*ppbf)->icbBuffer );


        Assert( (*ppbf)->sxwl.FOwnWriteLatch() );
        if ( (*ppbf)->icbBuffer != icbPage0 )
        {
            CallS( ErrBFISetBufferSize( *ppbf, icbPage0, fFalse ) );
            const BFResidenceState bfrsOld = BfrsBFIUpdateResidentState( *ppbf, bfrsNotCommitted );
            Assert( bfrsOld != bfrsNotCommitted );
        }
        (*ppbf)->sxwl.ReleaseOwnership( bfltWrite );
        Assert( !(*ppbf)->fInOB0OL && (*ppbf)->ob0ic.FUninitialized() );

        g_bfavail.Insert( *ppbf, !fMRU );
        *ppbf = pbfNil;


        if ( FRFSAnyFailureDetected() )
        {
            cOutOfMemoryRetriesMax = 10;
        }
        if ( cOutOfMemoryRetries > cOutOfMemoryRetriesMax )
        {
            return errSetBufferSize;
        }

        cOutOfMemoryRetries++;
    }


    if ( ( g_bfavail.Cobject() <= (ULONG_PTR)cbfAvailPoolLow ) &&
         ( FJetConfigMedMemory() || FJetConfigLowMemory() ) &&
         g_cacheram.AvailablePhysicalMemory() >= OSMemoryQuotaTotal() / 10 )
    {
        BFICacheSizeBoost();
    }


    CallS( ErrBFIMaintAvailPoolRequest( bfmaprtUnspecific ) );



    const ICBPage icbBufferEvicted = (ICBPage)(*ppbf)->icbBuffer;


    const BFResidenceState bfrsOld = BfrsBFIUpdateResidentState( *ppbf, bfrsResident );

    if ( bfrsOld != bfrsResident )
    {

        BOOL    fTryReclaim     = ( (DWORD)g_rgcbPageSize[(*ppbf)->icbBuffer] >= OSMemoryPageCommitGranularity() &&
                                    bfrsOld == bfrsNotResident );


        if ( fTryReclaim )
        {
            if ( (*ppbf)->bfat == bfatFracCommit )
            {
                Expected( !BoolParam( JET_paramEnableViewCache ) );
                Assert( FOSMemoryPageAllocated( (*ppbf)->pv, g_rgcbPageSize[(*ppbf)->icbBuffer] ) );
                OSMemoryPageReset( (*ppbf)->pv, g_rgcbPageSize[(*ppbf)->icbBuffer] );
            }
        }


        Assert( (*ppbf)->err != wrnBFPageFlushPending );

        const size_t cbChunk = min( (size_t)g_rgcbPageSize[(*ppbf)->icbBuffer], OSMemoryPageCommitGranularity() );
        for ( size_t ib = 0; ib < (size_t)g_rgcbPageSize[(*ppbf)->icbBuffer] && !BoolParam( JET_paramEnableViewCache ); ib += cbChunk )
        {
            if (    AtomicExchangeAdd( &((LONG*)((BYTE*)(*ppbf)->pv + ib))[0], 0 ) ||
                    AtomicExchangeAdd( &((LONG*)((BYTE*)(*ppbf)->pv + ib + cbChunk))[-1], 0 ) )
            {
                if ( bfrsOld == bfrsNotResident )
                {
                    AtomicIncrement( (LONG*)&g_cpgReclaim );
                }
            }
        }

    }

    Assert( (*ppbf)->icbBuffer != icbPageInvalid );

    if ( (*ppbf)->fNewlyEvicted )
    {
        (*ppbf)->fNewlyEvicted = fFalse;
        AtomicIncrement( (LONG*)&g_cbfNewlyEvictedUsed );
        AtomicExchangeAdd( (LONG*)&g_cbNewlyEvictedUsed, g_rgcbPageSize[icbBufferEvicted] );
    }

    (*ppbf)->fAvailable = fFalse;


    BFIAssertNewlyAllocatedPage( *ppbf );

    return JET_errSuccess;
}

BOOL FBFICacheViewCacheDerefIo( const BF * const pbf )
{
    Assert( pbf->bfat != bfatNone );

    
    return ( pbf->bfat == bfatViewMapped || pbf->bfat == bfatPageAlloc );
}

ERR ErrBFICacheIMapPage( BF * const pbf, const BOOL fNewPage )
{
    ERR err = JET_errSuccess;

    Assert( pbf );
    Assert( pbf->ifmp != 0 );
    Assert( pbf->ifmp != ifmpNil );
    Assert( pbf->pgno != pgnoNull );
    Assert( pbf->icbBuffer != icbPageInvalid && pbf->icbBuffer != icbPage0 );
    Assert( pbf->sxwl.FOwnWriteLatch() );

    Expected( BoolParam( JET_paramEnableViewCache ) );

    const size_t cb = g_rgcbPageSize[pbf->icbBuffer];

    if ( !fNewPage )
    {

        IFileAPI *const pfapi = g_rgfmp[pbf->ifmp].Pfapi();


        const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

        err = pfapi->ErrMMCopy( OffsetOfPgno( pbf->pgno ), cb, &pbf->pv );
        AssertTrack( err != JET_errFileIOBeyondEOF, "BFICacheMapPageEof" );

        FOSSetCleanupState( fCleanUpStateSaved );

        if ( err == JET_errFileIOBeyondEOF )
        {
            err = JET_errSuccess;
        }
        else
        {
            Assert( err <= JET_errSuccess );
            Assert( pbf->bfat == bfatNone );
            if ( err == JET_errSuccess )
            {

                Assert( pbf->pv );
                Assert( FOSMemoryFileMapped( pbf->pv, cb ) );
                Assert( !FOSMemoryFileMappedCowed( pbf->pv, cb ) );
                Assert( !FOSMemoryPageAllocated( pbf->pv, cb ) );

                pbf->bfat = bfatViewMapped;
            }
        }

        if ( err )
        {
            AssertRTL( JET_errOutOfMemory == err );
            Assert( pbf->pv == NULL );
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }
    }
    else
    {
        pbf->pv = NULL;
    }


    if ( pbf->pv == NULL )
    {
        Assert( pbf->bfat != bfatViewMapped );
        Assert( pbf->bfat == bfatNone );
    
        if ( !( pbf->pv = PvOSMemoryPageAlloc( cb, NULL ) ) )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        Assert( FOSMemoryPageAllocated( pbf->pv, cb ) );
        Assert( !FOSMemoryFileMapped( pbf->pv, cb ) );

        pbf->bfat = bfatPageAlloc;
    }

HandleError:

    if ( err < JET_errSuccess )
    {
        Assert( pbf->bfat == bfatNone );
        Assert( pbf->pv == NULL );
    }
    else
    {
        Assert( pbf->bfat != bfatNone );
        Assert( pbf->pv != NULL );
    }

    return err;
}

void BFICacheIUnmapPage( BF * const pbf )
{
    Assert( pbf );

    if ( FBFICacheViewCacheDerefIo( pbf ) )
    {
        Assert( pbf->ifmp != 0 );
        Assert( pbf->ifmp != ifmpNil );
        Assert( pbf->pgno != pgnoNull );

        Expected( BoolParam( JET_paramEnableViewCache ) );
        Assert( pbf->bfat != bfatNone );
        Assert( pbf->bfat != bfatFracCommit );
        Assert( pbf->bfat == bfatViewMapped || pbf->bfat == bfatPageAlloc );
        Assert( pbf->icbPage == pbf->icbBuffer );
        IFileAPI *const pfapi = g_rgfmp[pbf->ifmp].Pfapi();
        if ( pbf->bfat == bfatViewMapped )
        {
            Assert( FOSMemoryFileMapped( pbf->pv, g_rgcbPageSize[pbf->icbBuffer] ) );
            Assert( !FOSMemoryPageAllocated( pbf->pv, g_rgcbPageSize[pbf->icbBuffer] ) );
            if ( pfapi->ErrMMFree( pbf->pv ) >= JET_errSuccess )
            {
                pbf->pv = NULL;
                pbf->bfat = bfatNone;
            }
            else
            {
                AssertSz( fFalse, "This shouldn't fail (anymore), as we know when to call OSMemoryPageFree() vs. unmap a view." );
            }
        }
        else
        {
            Assert( pbf->bfat == bfatPageAlloc );
            Assert( FOSMemoryPageAllocated( pbf->pv, g_rgcbPageSize[pbf->icbBuffer] ) );
            Assert( !FOSMemoryFileMapped( pbf->pv, g_rgcbPageSize[pbf->icbBuffer] ) );
            OSMemoryPageFree( pbf->pv );
            pbf->pv = NULL;
            pbf->bfat = bfatNone;
        }
    }
}

void BFIFreePage( PBF pbf, const BOOL fMRU, const BFFreePageFlags bffpfDangerousOptions )
{
    Assert( pbf->sxwl.FOwnWriteLatch() );

    Assert( pbf->bfdf == bfdfClean );

    Enforce( pbf->err != errBFIPageFaultPending );
    Enforce( pbf->err != wrnBFPageFlushPending );
    Enforce( pbf->pWriteSignalComplete == NULL );
    Enforce( PvBFIAcquireIOContext( pbf ) == NULL );
    AssertTrack( !pbf->bfbitfield.FRangeLocked(), "BFFreeRangeStillLocked" );

    Enforce( pbf->pbfTimeDepChainNext == NULL );
    Enforce( pbf->pbfTimeDepChainPrev == NULL );

    Assert( !pbf->fOlderVersion );

    Assert( FBFIValidPvAllocType( pbf ) );
    Assert( pbf->bfat == bfatFracCommit || ( UlParam( JET_paramEnableViewCache ) && pbf->bfat == bfatNone ) );
    Assert( icbPageInvalid != pbf->icbBuffer );


    if (    pbf->ifmp != ifmpNil &&
            PinstFromIfmp( pbf->ifmp ) )
    {
        PERFOpt( cBFCache.Dec( PinstFromIfmp( pbf->ifmp ), pbf->tce, pbf->ifmp ) );
        if ( pbf->err == errBFIPageNotVerified )
        {
            PERFOpt( cBFCacheUnused.Dec( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
        }

        Assert( icbPageInvalid != pbf->icbPage );
    }


    if ( icbPageInvalid != pbf->icbPage )
    {
        AtomicDecrement( &g_rgcbfCachePages[pbf->icbBuffer] );
        Assert( g_rgcbfCachePages[pbf->icbBuffer] >= 0 );
    }

    pbf->ifmp = ifmpNil;
    pbf->pgno = pgnoNull;
    pbf->tce = tceNone;
    pbf->fAbandoned = fFalse;

    pbf->bfbitfield.SetFRangeLocked( fFalse );

    pbf->fSyncRead = fFalse;


    pbf->icbPage = icbPageInvalid;


    if ( bffpfDangerousOptions & bffpfQuiesce )
    {
        BFIReleaseBuffer( pbf );
    }


    else
    {

        pbf->sxwl.ReleaseOwnership( bfltWrite );

        BFIAssertNewlyAllocatedPage( pbf, fTrue );

#ifdef DEBUG

        if ( !UlParam( JET_paramEnableViewCache ) &&
            !( ( pbf->icbBuffer == g_icbCacheMax ) && ( IbfBFICachePbf( pbf ) >= cbfCacheAddressable ) ) )
        {
            Assert( !pbf->fQuiesced || ( pbf->icbBuffer == icbPage0 ) );
            Expected( ( pbf->icbBuffer != icbPage0 ) || pbf->fQuiesced );
        }
#endif

        if ( pbf->fQuiesced )
        {
            Assert( g_critCacheSizeResize.FOwner() );

            if ( IbfBFICachePbf( pbf ) < cbfCacheAddressable )
            {

                Assert( !pbf->fInOB0OL && pbf->ob0ic.FUninitialized() );
                g_bfquiesced.Remove( pbf );
            }
            
            AtomicIncrement( (LONG*)&g_cbfCacheClean );
            cbfCacheSize++;
            Assert( cbfCacheSize > 0 );
        }


        pbf->fQuiesced = fFalse;
        pbf->fAvailable = fTrue;


        Assert( !pbf->fInOB0OL && pbf->ob0ic.FUninitialized() );
        g_bfavail.Insert( pbf, fMRU );
    }
}

ERR ErrBFICachePage(    PBF* const ppbf,
                        const IFMP ifmp,
                        const PGNO pgno,
                        const BOOL fNewPage,
                        const BOOL fWait,
                        const BOOL fMRU,
                        const ULONG_PTR pctCachePriority,
                        const TraceContext& tc,
                        const BFLatchType bfltTraceOnly,
                        const BFLatchFlags bflfTraceOnly )
{
    ERR             err;
    PGNOPBF         pgnopbf;
    BFHash::ERR     errHash;
    BFHash::CLock   lock;
    BFLRUK::ERR     errLRUK;
    const TCE       tce = (TCE)tc.nParentObjectClass;

    Assert( pgno >= 1 );

    pgnopbf.pbf = *ppbf;

    FMP* pfmp = &g_rgfmp[ ifmp ];


    if ( !fNewPage )
    {
        PGNO pgnoLast = pgnoNull;
        err = pfmp->ErrPgnoLastFileSystem( &pgnoLast );

        if ( ( ( err >= JET_errSuccess ) && ( pgno > pgnoLast ) ) ||
             ( err == JET_errDatabaseCorrupted ) )
        {
            Error( ErrERRCheck( JET_errFileIOBeyondEOF ) );
        }
        Call( err );
    }


    if ( !pfmp->FBFContext() )
    {
        Call( ErrBFISetupBFFMPContext( ifmp ) );
    }


    const INT icbPageSize = IcbBFIPageSize( pfmp->CbPage() );

    Assert( icbPageSize == IcbBFIBufferSize( pfmp->CbPage() ) );


    if ( pgnopbf.pbf == NULL )
    {
        const ERR errBufferAlloc = ErrBFIAllocPage( &pgnopbf.pbf, IcbBFIBufferSize( pfmp->CbPage() ), fWait, fMRU );

        if ( errBufferAlloc < JET_errSuccess )
        {
            Expected( ( errBufferAlloc == JET_errOutOfMemory ) || ( errBufferAlloc == JET_errOutOfBuffers ) );

            OSUHAEmitFailureTag( PinstFromIfmp( ifmp ), HaDbFailureTagHard, L"9ee69aa5-53f9-43e6-9b6c-b817db21b9c2" );
        }

        Call( errBufferAlloc );
    }
    Assert( pgnopbf.pbf->sxwl.FOwnWriteLatch() );
    BFIAssertNewlyAllocatedPage( pgnopbf.pbf );
    Assert( pgnopbf.pbf->icbBuffer == IcbBFIBufferSize( pfmp->CbPage() ));


    Assert( ifmpNil != ifmp );
    pgnopbf.pbf->ifmp = ifmp;
    pgnopbf.pbf->pgno = pgno;
    pgnopbf.pbf->icbPage = icbPageSize;
    pgnopbf.pbf->tce = tce;

    AssertTrack( !pgnopbf.pbf->bfbitfield.FRangeLocked(), "BFCacheRangeAlreadyLocked" );
    pgnopbf.pbf->bfbitfield.SetFRangeLocked( fFalse );

    PERFOpt( cBFCache.Inc( PinstFromIfmp( ifmp ), tce, ifmp ) );

    Assert( icbPageInvalid != pgnopbf.pbf->icbBuffer );
    AtomicIncrement( &g_rgcbfCachePages[pgnopbf.pbf->icbBuffer] );
    Assert( g_rgcbfCachePages[pgnopbf.pbf->icbBuffer] > 0 );

    pgnopbf.pgno = pgno;


    if ( BoolParam( JET_paramEnableViewCache ) )
    {
        Call( ErrBFICacheIMapPage( pgnopbf.pbf, fNewPage ) );
    }
    else
    {
        Assert( FOSMemoryPageAllocated( pgnopbf.pbf->pv, g_rgcbPageSize[pgnopbf.pbf->icbBuffer] ) );
        Assert( !FOSMemoryFileMapped( pgnopbf.pbf->pv, g_rgcbPageSize[pgnopbf.pbf->icbBuffer] ) );

        Assert( pgnopbf.pbf->bfat == bfatFracCommit );
        pgnopbf.pbf->bfat = bfatFracCommit;
    }
    Assert( FBFIValidPvAllocType( pgnopbf.pbf ) );
    Assert( pgnopbf.pbf->bfat != bfatNone );


    BOOL fRepeatedlyRead = fFalse;
    const TICK tickCache = TickOSTimeCurrent();

    errLRUK = g_bflruk.ErrCacheResource( IFMPPGNO( ifmp, pgno ),
                                       pgnopbf.pbf,
                                       tickCache,
                                       pctCachePriority,
                                       !fNewPage,
                                       &fRepeatedlyRead );


    if ( errLRUK != BFLRUK::ERR::errSuccess )
    {
        Assert( errLRUK == BFLRUK::ERR::errOutOfMemory );


        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }


    g_bfhash.WriteLockKey( IFMPPGNO( ifmp, pgno ), &lock );
    errHash = g_bfhash.ErrInsertEntry( &lock, pgnopbf );
    g_bfhash.WriteUnlockKey( &lock );


    if ( errHash != BFHash::ERR::errSuccess )
    {

        TICK cmsecTotal = 0;
        while ( g_bflruk.ErrEvictResource( IFMPPGNO( pgnopbf.pbf->ifmp, pgnopbf.pbf->pgno ),
                                         pgnopbf.pbf,
                                         fFalse ) != BFLRUK::ERR::errSuccess )
        {
            UtilSleep( dtickFastRetry );
            cmsecTotal += dtickFastRetry;
            if ( cmsecTotal > (TICK)cmsecDeadlock )
            {
                AssertSz( fFalse, "BF LRU-K eviction appears to be hung." );
                cmsecTotal = 0;
            }
        }


        if ( errHash == BFHash::ERR::errKeyDuplicate )
        {

            Error( ErrERRCheck( errBFPageCached ) );
        }


        else
        {
            Assert( errHash == BFHash::ERR::errOutOfMemory );


            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }
    }


    pgnopbf.pbf->fCurrentVersion = fTrue;


    *ppbf = pgnopbf.pbf;

    Enforce( FBFICurrentPage( *ppbf, ifmp, pgno ) );

    Enforce( pgnopbf.pbf->pbfTimeDepChainNext == NULL );
    Enforce( pgnopbf.pbf->pbfTimeDepChainPrev == NULL );

    Enforce( wrnBFPageFlushPending != pgnopbf.pbf->err );
    Enforce( JET_errSuccess == pgnopbf.pbf->err );
    Enforce( NULL == pgnopbf.pbf->pWriteSignalComplete );
    Enforce( NULL == PvBFIAcquireIOContext( pgnopbf.pbf ) );

    Assert( FBFIValidPvAllocType( pgnopbf.pbf ) );
    Assert( pgnopbf.pbf->bfat != bfatNone );

    if ( fRepeatedlyRead )
    {
        PERFOpt( cBFPagesRepeatedlyRead.Inc( PinstFromIfmp( pgnopbf.pbf->ifmp ), pgnopbf.pbf->tce ) );
    }

    const BOOL fDBScan = ( bflfTraceOnly & bflfDBScan );


    if ( !fNewPage && !fDBScan )
    {
        PERFOpt( cBFCacheUniqueReq.Inc( PinstFromIfmp( pgnopbf.pbf->ifmp ), pgnopbf.pbf->tce, pgnopbf.pbf->ifmp ) );
        Ptls()->threadstats.cPageUniqueCacheRequests++;
    }


    const BFRequestTraceFlags bfrtfTraceOnly = BFRequestTraceFlags(
        ( fNewPage ? bfrtfNewPage : bfrtfUseHistory ) |
        ( BoolParam( JET_paramEnableFileCache ) ? bfrtfFileCacheEnabled : bfrtfNone ) |
        ( fDBScan ? bfrtfDBScan : bfrtfNone ) );

    BFITraceCachePage( tickCache, pgnopbf.pbf, bfltTraceOnly, (ULONG)pctCachePriority, bflfTraceOnly, bfrtfTraceOnly, tc );

    OSTraceFMP( pgnopbf.pbf->ifmp, JET_tracetagBufferManagerBufferCacheState,
                OSFormat(   "Cached page=[0x%x:0x%x], pbf=%p, pv=%p tce=%d",
                            (ULONG)pgnopbf.pbf->ifmp,
                            pgnopbf.pbf->pgno,
                            pgnopbf.pbf,
                            pgnopbf.pbf->pv,
                            pgnopbf.pbf->tce ) );

    return JET_errSuccess;

HandleError:
    AssertRTL( err > -65536 && err < 65536 );

    Assert( err < JET_errSuccess );
    if ( pgnopbf.pbf != NULL )
    {

        BFICacheIUnmapPage( pgnopbf.pbf );

        Assert( pgnopbf.pbf->bfat == bfatNone || !UlParam( JET_paramEnableViewCache ) );


        BFIFreePage( pgnopbf.pbf, fMRU );
    }
    *ppbf = pbfNil;

    return err;
}


INLINE BOOL FBFICacheViewFresh( const PBF pbf )
{
    Assert( pbf->bfat == bfatViewMapped );
    return DtickDelta( pbf->tickViewLastRefreshed, TickOSTimeCurrent() ) < dtickMaintRefreshViewMappedThreshold;
}


ERR ErrBFICacheViewFreshen( PBF pbf, const OSFILEQOS qosIoPriorities, const TraceContext& tcBase )
{
    ERR err = JET_errSuccess;

    Assert( pbf->sxwl.FOwner() );
    Assert( pbf->bfat == bfatViewMapped );
    Assert( pbf->bfls != bflsHashed );

    Assert( g_rgcbPageSize[g_icbCacheMax] == CbBFIPageSize( pbf ) );

    TraceContextScope tcReclaimFromOS;
    tcReclaimFromOS->iorReason.AddFlag( iorfReclaimPageFromOS );

    HRT hrtStart = HrtHRTCount();
    Call( g_rgfmp[pbf->ifmp].Pfapi()->ErrMMIORead( OffsetOfPgno( pbf->pgno ),
                                                    (BYTE*)pbf->pv,
                                                    CbBFIPageSize( pbf ),
                                                    IFileAPI::FileMmIoReadFlag( IFileAPI::fmmiorfKeepCleanMapped | IFileAPI::fmmiorfPessimisticReRead ) ) );
    const QWORD usecs = ( 1000000 * ( HrtHRTCount() - hrtStart ) ) / HrtHRTFreq();
    if ( usecs > 100 )
    {
        BFITrackCacheMissLatency( pbf, hrtStart, bftcmrReasonMapViewRefresh, qosIoPriorities, *tcReclaimFromOS, err );
    }

    pbf->tickViewLastRefreshed = TickBFIHashedLatchTime( TickOSTimeCurrent() );

HandleError:

    return err;
}


void BFIOpportunisticallyFlushPage( PBF pbf, IOREASONPRIMARY iorp )
{
    CBFIssueList bfil;

    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

     
    const ERR errT = ErrBFIFlushPage( pbf,
                                        IOR( iorp, iorfForeground ),
                                        qosIODispatchWriteMeted | qosIODispatchImmediate,
                                        ( iorp == iorpBFFilthyFlush ) ? bfdfFilthy : bfdfDirty );

    FOSSetCleanupState( fCleanUpStateSaved );

    if ( errT == errDiskTilt )
    {
#ifdef DEBUG
        AssertSz( ChitsFaultInj( 39452 ) > 0, "DiskTiltOnMetedWriteIoReqWithoutFi" );
#else
        AssertTrack( fFalse, "DiskTiltOnMetedWriteIoReq" );
#endif
        bfil.NullifyDiskTiltFake( pbf->ifmp );
        Assert( bfil.FEmpty() );
    }
    if ( errT == errBFIPageFlushed )
    {
        CallS( bfil.ErrIssue() );
    }
    else if ( errT == errBFIRemainingDependencies )
    {
        bfil.AbandonLogOps();
    }

}


BOOL FBFIMaintIsImpedingCheckpointMaintenance( __in const PBF pbf, __out BOOL * const pfUrgentMaintReq )
{
    const IFMP  ifmp        = pbf->ifmp;
    INST *      pinst       = PinstFromIfmp( ifmp );
    LOG *       plog        = pinst->m_plog;

    Assert( pbf );
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );

    Assert( pfUrgentMaintReq );
    *pfUrgentMaintReq = fFalse;

    *pfUrgentMaintReq = FBFIChance( 40 );


    LGPOS       lgposWaypoint = g_rgfmp[ ifmp ].LgposWaypoint();

    LGPOS       lgposLogTip = plog->LgposLGLogTipNoLock();


    if (
            ( FBFIChance( 20 ) || ( CmpLgpos( lgposLogTip, lgposWaypoint ) > 0 ) ) &&
            pbf->bfdf > bfdfClean &&
            CmpLgpos( &pbf->lgposOldestBegin0, &lgposMax ) )
    {
        __int64     cbEffCheckpointDepth = 0;


        ULONG_PTR cbCheckpointDepth = pinst->m_plog->CbLGDesiredCheckpointDepth();
        cbEffCheckpointDepth = (__int64) cbCheckpointDepth;
        cbEffCheckpointDepth -= (__int64) UlParam( pinst, JET_paramWaypointLatency ) * 1024 * (__int64) UlParam( pinst, JET_paramLogFileSize );
        if ( cbEffCheckpointDepth < (__int64) ( cbCheckpointDepth / 5 ) )
        {
            cbEffCheckpointDepth = cbCheckpointDepth / 5;
        }
        const BOOL  fWaypoint   = g_fBFEnableForegroundCheckpointMaint &&
                        (LONGLONG) plog->CbLGOffsetLgposForOB0( lgposLogTip, pbf->lgposOldestBegin0 ) > cbEffCheckpointDepth;

        const BOOL  fDependedPage = pbf->bfbitfield.FImpedingCheckpoint();

        const BOOL  fRandomChance = FBFIChance( 20 );

        if ( fWaypoint || fDependedPage || fRandomChance )
        {

            const PBF pbfOlder = pbf->pbfTimeDepChainNext;
            if (    NULL == pbfOlder ||
                    pbf->lgposModify.lGeneration != pbfOlder->lgposModify.lGeneration )
            {
                return fTrue;
            }
        }
    }

    return fFalse;
}

void BFIMaintImpedingPage( PBF pbf )
{
    PBF         pbfVer          = NULL;
    BOOL    fUrgentMaintReq     = fFalse;

    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );

    if ( FBFIMaintIsImpedingCheckpointMaintenance( pbf, &fUrgentMaintReq ) )
    {
        const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

        if ( ErrBFIVersionPage( pbf, &pbfVer, fUrgentMaintReq ) >= JET_errSuccess )
        {
            Assert( pbf->pbfTimeDepChainNext != pbfNil );

            pbfVer->sxwl.ReleaseWriteLatch();

            if ( g_fBFEnableForegroundCheckpointMaint )
            {
                BFIOpportunisticallyFlushPage( pbfVer, iorpBFCheckpointAdv );
            }
        }

        FOSSetCleanupState( fCleanUpStateSaved );
    }
}


ERR ErrBFIMaintImpedingPageLatch( PBF pbf, __in const BOOL fOwnsWrite, BFLatch* pbfl )
{
    ERR     err                 = JET_errSuccess;
    PBF     pbfVer              = NULL;
    BOOL    fUrgentMaintReq     = fFalse;

    Assert( PBF( pbfl->dwContext ) == pbf );
    Assert( fOwnsWrite ?
                    pbf->sxwl.FOwnWriteLatch() :
                    pbf->sxwl.FOwnExclusiveLatch() );


    if ( !FBFIUpdatablePage( pbf ) )
    {
        Assert( wrnBFPageFlushPending == pbf->err );
        if ( FBFICompleteFlushPage( pbf, fOwnsWrite ? bfltWrite : bfltExclusive ) )
        {
            Assert( pbf->err < JET_errSuccess || pbf->bfdf == bfdfClean );
            Assert( FBFIUpdatablePage( pbf ) );
        }


        if ( pbf->err < JET_errSuccess )
        {
            Error( ErrERRCheck( errBFLatchConflict ) );
        }
    }

    const BOOL fInIo = !FBFIUpdatablePage( pbf );

    if ( fInIo ||
            FBFIMaintIsImpedingCheckpointMaintenance( pbf, &fUrgentMaintReq ) )
    {

        fUrgentMaintReq = fUrgentMaintReq || fInIo;

        Call( ErrBFIVersionCopyPage( pbf, &pbfVer, fUrgentMaintReq, fOwnsWrite ) );

        Assert( pbfVer );
        if ( pbf->bfdf > bfdfClean )
        {
            Assert( pbf->pbfTimeDepChainPrev == pbfVer );
            Assert( pbfVer->pbfTimeDepChainNext == pbf );
        }

        Assert( !FBFICurrentPage( pbf, pbfVer->ifmp, pbfVer->pgno ) );
        Assert( FBFICurrentPage( pbfVer, pbf->ifmp, pbf->pgno ) );

        if ( fOwnsWrite )
        {
            pbf->sxwl.ReleaseWriteLatch();
        }
        else
        {
            pbf->sxwl.ReleaseExclusiveLatch();
        }

        if ( g_fBFEnableForegroundCheckpointMaint )
        {

            BFIOpportunisticallyFlushPage( pbf, fInIo ? iorpBFImpedingWriteCleanDoubleIo : iorpBFCheckpointAdv );
        }

        Assert( PBF( pbfl->dwContext ) != pbfVer );
        pbfl->dwContext = (DWORD_PTR)pbfVer;
        pbfl->pv = pbfVer->pv;

        PERFOpt( cBFLatchConflict.Inc( perfinstGlobal ) );

        err = ErrERRCheck( wrnBFLatchMaintConflict );

    }

HandleError:

    if( err < JET_errSuccess && FBFIUpdatablePage( pbf ) )
    {
        err = JET_errSuccess;
    }


    if ( JET_errSuccess == err || err < JET_errSuccess )
    {
        Assert( JET_errSuccess == err || JET_errOutOfMemory == err || JET_errOutOfBuffers == err );
        Assert( pbf == PBF( pbfl->dwContext ) );
        Assert( fOwnsWrite ?
                        pbf->sxwl.FOwnWriteLatch() :
                        pbf->sxwl.FOwnExclusiveLatch() );
    }
    else
    {
        Assert( wrnBFLatchMaintConflict == err );
        Assert( pbf != PBF( pbfl->dwContext ) );
        Assert( pbf->sxwl.FNotOwner() );
        Assert( pbfVer->sxwl.FOwnWriteLatch() );
    }
    return err;
}



BOOL FBFIMaintNeedsOpportunisticFlushing( PBF pbf )
{
    const IFMP  ifmp        = pbf->ifmp;
    INST *      pinst       = PinstFromIfmp( ifmp );
    LOG *       plog        = pinst->m_plog;

    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );

    const ULONG_PTR cbCheckpointDepthPreferred = pinst->m_plog->CbLGDesiredCheckpointDepth();

    if ( pinst->m_fCheckpointQuiesce )
    {
        return fFalse;
    }

    if ( pbf->bfdf > bfdfClean &&
         CmpLgpos( &pbf->lgposOldestBegin0, &lgposMax ) )
    {
        

        const BOOL  fHotPage    = plog->CbLGOffsetLgposForOB0( plog->LgposLGLogTipNoLock(), pbf->lgposOldestBegin0 ) > cbCheckpointDepthPreferred &&
                                    ( pbf->sxwl.CWaitExclusiveLatch() || pbf->sxwl.CWaitWriteLatch() ) ;

        const BOOL  fForegroundCheckpointMaint = g_fBFEnableForegroundCheckpointMaint &&
                            plog->CbLGOffsetLgposForOB0( plog->LgposLGLogTipNoLock(), pbf->lgposOldestBegin0 ) > cbCheckpointDepthPreferred;

        const BOOL  fRandomChance = FBFIChance( 10 );

        if (    pbf->pbfTimeDepChainNext == pbfNil &&
                ( fHotPage || fForegroundCheckpointMaint || fRandomChance ) )
        {
            return fTrue;
        }
    }

    return fFalse;
}


void BFIOpportunisticallyVersionPage( PBF pbf, PBF * ppbfOpportunisticCheckpointAdv )
{
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );

    Assert( ppbfOpportunisticCheckpointAdv );
    *ppbfOpportunisticCheckpointAdv = NULL;

    if ( FBFIMaintNeedsOpportunisticFlushing( pbf ) )
    {
        if ( ErrBFIVersionPage( pbf, ppbfOpportunisticCheckpointAdv, fFalse ) >= JET_errSuccess )
        {
            Assert( pbf->pbfTimeDepChainNext != pbfNil );
            (*ppbfOpportunisticCheckpointAdv)->sxwl.ReleaseWriteLatch();
        }
    }
}


void BFIOpportunisticallyVersionCopyPage( PBF pbf, PBF * ppbfNew, __in const BOOL fOwnsWrite )
{
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );
    Assert( pbf->bfdf >= bfdfClean );

    Assert( ppbfNew );
    *ppbfNew = NULL;

    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

    if ( FBFIMaintNeedsOpportunisticFlushing( pbf ) )
    {
        if ( ErrBFIVersionCopyPage( pbf, ppbfNew, fFalse, fOwnsWrite ) >= JET_errSuccess )
        {
            Assert( (*ppbfNew)->pbfTimeDepChainNext != pbfNil );
            Assert( (*ppbfNew)->pbfTimeDepChainNext == pbf );
            Assert( pbf->pbfTimeDepChainPrev == (*ppbfNew) );
        }
    }

    FOSSetCleanupState( fCleanUpStateSaved );

}


ERR ErrBFIVersionPage( PBF pbf, PBF* ppbfOld, const BOOL fWait )
{
    ERR         err     = JET_errSuccess;
    BFLRUK::ERR errLRUK = BFLRUK::ERR::errSuccess;
    TraceContextScope tcVerPage;

    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );

    Assert( !!pbf->fCurrentVersion == !pbf->fOlderVersion );

    Assert( FBFIUpdatablePage( pbf ) );

    if ( pbf->fAbandoned )
    {
        Error( ErrERRCheck( errBFIPageAbandoned ) );
    }

    Assert( ( pbf->err >= JET_errSuccess ) || ( pbf->bfdf > bfdfClean ) );

    
    const ICBPage icbNewOrigBuffer = (ICBPage)pbf->icbBuffer;

    
    Expected( pbf->icbBuffer == pbf->icbPage );


    Call( ErrBFIAllocPage( ppbfOld, icbNewOrigBuffer, fWait ) );
    Assert( (*ppbfOld)->sxwl.FOwnWriteLatch() );
    BFIAssertNewlyAllocatedPage( *ppbfOld );


    if ( BoolParam( JET_paramEnableViewCache ) )
    {
        const size_t cbAlloc = g_rgcbPageSize[icbNewOrigBuffer];
        
        if ( !( (*ppbfOld)->pv = PvOSMemoryPageAlloc( cbAlloc, NULL ) ) )
        {

            BFIFreePage( *ppbfOld );


            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        (*ppbfOld)->bfat = bfatPageAlloc;
    }
    Assert( FBFIValidPvAllocType( (*ppbfOld) ) );
    Assert( (*ppbfOld)->bfat != bfatNone );


    Assert( ifmpNil != pbf->ifmp );
    (*ppbfOld)->ifmp = pbf->ifmp;
    (*ppbfOld)->pgno = pbf->pgno;
    (*ppbfOld)->fAbandoned = pbf->fAbandoned;
    (*ppbfOld)->icbPage = pbf->icbPage;
    (*ppbfOld)->tce = pbf->tce;

    PERFOpt( cBFCache.Inc( PinstFromIfmp( (*ppbfOld)->ifmp ), (*ppbfOld)->tce, (*ppbfOld)->ifmp ) );

    Assert( icbPageInvalid != (*ppbfOld)->icbBuffer );
    AtomicIncrement( &g_rgcbfCachePages[(*ppbfOld)->icbBuffer] );
    Assert( g_rgcbfCachePages[(*ppbfOld)->icbBuffer] > 0 );


    errLRUK = g_bflruk.ErrCacheResource( IFMPPGNO( pbf->ifmp, pbf->pgno ),
                                        *ppbfOld,
                                        TickOSTimeCurrent(),
                                        g_pctCachePriorityMin,
                                        fFalse );


    if ( errLRUK != BFLRUK::ERR::errSuccess )
    {
        Assert( errLRUK == BFLRUK::ERR::errOutOfMemory );


        if ( BoolParam( JET_paramEnableViewCache ) )
        {
            Assert( (*ppbfOld)->bfat == bfatPageAlloc );
            OSMemoryPageFree( (*ppbfOld)->pv );
            (*ppbfOld)->pv = NULL;
            (*ppbfOld)->bfat = bfatNone;
        }


        BFIFreePage( *ppbfOld );


        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }




    Assert( icbNewOrigBuffer == pbf->icbBuffer );
    Assert( pbf->icbBuffer == (*ppbfOld)->icbBuffer );

    UtilMemCpy( (*ppbfOld)->pv, pbf->pv, g_rgcbPageSize[icbNewOrigBuffer] );


    (*ppbfOld)->err = pbf->err;



    BFIDirtyPage( pbf, bfdfDirty, *tcVerPage );
    BFIDirtyPage( *ppbfOld, BFDirtyFlags( pbf->bfdf ), *tcVerPage );
    Expected( bfdfDirty <= pbf->bfdf );
    pbf->bfdf = bfdfDirty;


    BFISetLgposOldestBegin0( *ppbfOld, pbf->lgposOldestBegin0, *tcVerPage );
    BFIResetLgposOldestBegin0( pbf );

    g_critBFDepend.Enter();


    (*ppbfOld)->pbfTimeDepChainNext = pbf->pbfTimeDepChainNext;
    (*ppbfOld)->pbfTimeDepChainPrev = pbf;
    pbf->pbfTimeDepChainNext        = *ppbfOld;
    if ( (*ppbfOld)->pbfTimeDepChainNext != pbfNil )
    {
        (*ppbfOld)->pbfTimeDepChainNext->pbfTimeDepChainPrev = *ppbfOld;
    }

    (*ppbfOld)->fOlderVersion = fTrue;


    if ( pbf->bfbitfield.FDependentPurged() )
    {
        (*ppbfOld)->bfbitfield.SetFDependentPurged( fTrue );
    }


    (*ppbfOld)->bfbitfield.SetFImpedingCheckpoint( pbf->bfbitfield.FImpedingCheckpoint() );
    pbf->bfbitfield.SetFImpedingCheckpoint( fFalse );

    g_critBFDepend.Leave();

    if ( pbf->prceUndoInfoNext == prceNil )
    {
        BFISetLgposModify( *ppbfOld, pbf->lgposModify );
        BFIResetLgposModify( pbf );
    }
    else
    {

        CCriticalSection* const pcrit       = &g_critpoolBFDUI.Crit( pbf );
        CCriticalSection* const pcritOld    = &g_critpoolBFDUI.Crit( *ppbfOld );

        CCriticalSection* const pcritMax    = max( pcrit, pcritOld );
        CCriticalSection* const pcritMin    = min( pcrit, pcritOld );

        ENTERCRITICALSECTION ecsMax( pcritMax );
        ENTERCRITICALSECTION ecsMin( pcritMin, pcritMin != pcritMax );

        (*ppbfOld)->prceUndoInfoNext    = pbf->prceUndoInfoNext;
        pbf->prceUndoInfoNext           = prceNil;

        BFISetLgposModify( *ppbfOld, pbf->lgposModify );
        BFIResetLgposModify( pbf );
    }

    (*ppbfOld)->rbsposSnapshot = pbf->rbsposSnapshot;
    pbf->rbsposSnapshot = rbsposMin;


    PERFOpt( cBFPagesVersioned.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
    PERFOpt( AtomicIncrement( (LONG*)&g_cBFVersioned ) );
    ETCacheVersionPage( pbf->ifmp, pbf->pgno );


    if ( !pbf->fFlushed )
    {
        pbf->fFlushed = fTrue;
    }
    else
    {
        (*ppbfOld)->fFlushed = fTrue;
    }

    return JET_errSuccess;

HandleError:
    Assert( err < JET_errSuccess );

    *ppbfOld = pbfNil;
    return err;
}



ERR ErrBFIVersionCopyPage( PBF pbfOrigOld, PBF* ppbfNewCurr, const BOOL fWait, __in const BOOL fOwnsWrite )
{
    ERR             err     = JET_errSuccess;
    BFLRUK::CLock   lockLRUK;
    BFLRUK::ERR     errLRUK = BFLRUK::ERR::errSuccess;
    BFHash::CLock   lockHash;
    PGNOPBF         pgnopbf;

    Assert( pbfOrigOld->sxwl.FOwnExclusiveLatch() || pbfOrigOld->sxwl.FOwnWriteLatch() );
    Assert( pbfOrigOld->fCurrentVersion );
    Assert( !pbfOrigOld->fOlderVersion );

    Assert( fOwnsWrite ? pbfOrigOld->sxwl.FOwnWriteLatch() : pbfOrigOld->sxwl.FNotOwnWriteLatch() );
    if ( !fOwnsWrite )
    {
        CSXWLatch::ERR errWL = pbfOrigOld->sxwl.ErrUpgradeExclusiveLatchToWriteLatch();
        if ( errWL != CSXWLatch::ERR::errSuccess )
        {
            Assert( CSXWLatch::ERR::errWaitForWriteLatch == errWL );
            pbfOrigOld->sxwl.WaitForWriteLatch();
        }
    }
    Assert( pbfOrigOld->sxwl.FOwnWriteLatch() );

    if ( pbfOrigOld->fAbandoned )
    {
        Error( ErrERRCheck( errBFIPageAbandoned ) );
    }

    Assert( ( pbfOrigOld->err >= JET_errSuccess ) || ( pbfOrigOld->bfdf > bfdfClean ) );



    const ICBPage icbNewCurrBuffer = (ICBPage)pbfOrigOld->icbBuffer;

    Call( ErrBFIAllocPage( ppbfNewCurr, icbNewCurrBuffer, fWait ) );
    Assert( (*ppbfNewCurr)->sxwl.FOwnWriteLatch() );
    BFIAssertNewlyAllocatedPage( *ppbfNewCurr );


    if ( BoolParam( JET_paramEnableViewCache ) )
    {
        const size_t cbAlloc = g_rgcbPageSize[icbNewCurrBuffer];
    

        const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

        if ( !( (*ppbfNewCurr)->pv = PvOSMemoryPageAlloc( cbAlloc, NULL ) ) )
        {


            FOSSetCleanupState( fCleanUpStateSaved );


            BFIFreePage( *ppbfNewCurr );


            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        (*ppbfNewCurr)->bfat = bfatPageAlloc;


        FOSSetCleanupState( fCleanUpStateSaved );
    }
    Assert( FBFIValidPvAllocType( (*ppbfNewCurr) ) );
    Assert( (*ppbfNewCurr)->bfat != bfatNone );


    Assert( ifmpNil != pbfOrigOld->ifmp );
    (*ppbfNewCurr)->ifmp = pbfOrigOld->ifmp;
    (*ppbfNewCurr)->pgno = pbfOrigOld->pgno;
    (*ppbfNewCurr)->fAbandoned = pbfOrigOld->fAbandoned;
    (*ppbfNewCurr)->icbPage = pbfOrigOld->icbPage;
    (*ppbfNewCurr)->tce = pbfOrigOld->tce;

    PERFOpt( cBFCache.Inc( PinstFromIfmp( (*ppbfNewCurr)->ifmp ), (*ppbfNewCurr)->tce, (*ppbfNewCurr)->ifmp ) );

    Assert( icbPageInvalid != (*ppbfNewCurr)->icbBuffer );
    AtomicIncrement( &g_rgcbfCachePages[(*ppbfNewCurr)->icbBuffer] );
    Assert( g_rgcbfCachePages[(*ppbfNewCurr)->icbBuffer] > 0 );


    BFISynchronicity();
    errLRUK = g_bflruk.ErrCacheResource( IFMPPGNO( pbfOrigOld->ifmp, pbfOrigOld->pgno ),
                                        *ppbfNewCurr,
                                        TickOSTimeCurrent(),
                                        g_pctCachePriorityNeutral,
                                        fFalse,
                                        NULL,
                                        pbfOrigOld );
    BFISynchronicity();


    if ( errLRUK != BFLRUK::ERR::errSuccess )
    {
        Assert( errLRUK == BFLRUK::ERR::errOutOfMemory );


        if ( BoolParam( JET_paramEnableViewCache ) )
        {
            Assert( (*ppbfNewCurr)->bfat == bfatPageAlloc );
            OSMemoryPageFree( (*ppbfNewCurr)->pv );
            (*ppbfNewCurr)->pv = NULL;
            (*ppbfNewCurr)->bfat = bfatNone;
        }


        BFIFreePage( *ppbfNewCurr );


        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }




    BFISynchronicity();
    g_bfhash.WriteLockKey( IFMPPGNO( pbfOrigOld->ifmp, pbfOrigOld->pgno ), &lockHash );
    BFISynchronicity();


    pgnopbf.pgno = pbfOrigOld->pgno;
    pgnopbf.pbf = *ppbfNewCurr;

    const BFHash::ERR errHash = g_bfhash.ErrReplaceEntry( &lockHash, pgnopbf );
    Assert( BFHash::ERR::errSuccess == errHash );



    UtilMemCpy( (*ppbfNewCurr)->pv, pbfOrigOld->pv, g_rgcbPageSize[icbNewCurrBuffer] );


    if ( wrnBFPageFlushPending != pbfOrigOld->err )
    {
        (*ppbfNewCurr)->err = pbfOrigOld->err;
    }


    if ( pbfOrigOld->bfdf != bfdfClean )
    {
        BFIDirtyPage( *ppbfNewCurr, bfdfDirty, *TraceContextScope() );
        BFIDirtyPage( pbfOrigOld, BFDirtyFlags( (*ppbfNewCurr)->bfdf ), *TraceContextScope() );
    }
    else
    {
        Assert( (*ppbfNewCurr)->bfdf == bfdfClean );
    }

    BFISynchronicity();
    g_critBFDepend.Enter();
    BFISynchronicity();


    (*ppbfNewCurr)->pbfTimeDepChainNext = pbfOrigOld;
    (*ppbfNewCurr)->pbfTimeDepChainPrev = pbfNil;

    pbfOrigOld->pbfTimeDepChainPrev     = *ppbfNewCurr;


    pbfOrigOld->fOlderVersion = fTrue;
    pbfOrigOld->fCurrentVersion = fFalse;

    (*ppbfNewCurr)->fOlderVersion = fFalse;
    (*ppbfNewCurr)->fCurrentVersion = fTrue;


    if ( pbfOrigOld->bfbitfield.FDependentPurged() )
    {
        (*ppbfNewCurr)->bfbitfield.SetFDependentPurged( fTrue );
    }


    Assert( (*ppbfNewCurr)->bfbitfield.FImpedingCheckpoint() == fFalse );
    (*ppbfNewCurr)->bfbitfield.SetFImpedingCheckpoint( fFalse );


    BFISynchronicity();
    g_critBFDepend.Leave();
    BFISynchronicity();


    if ( pbfOrigOld->bfdf == bfdfClean )
    {
        Assert( pbfOrigOld->prceUndoInfoNext == prceNil );
    }
    Assert( (*ppbfNewCurr)->prceUndoInfoNext == prceNil );
    (*ppbfNewCurr)->prceUndoInfoNext    = prceNil;

    BFISynchronicity();
    g_bfhash.WriteUnlockKey( &lockHash );
    BFISynchronicity();



    
    Assert( 0 == CmpLgpos( &((*ppbfNewCurr)->lgposOldestBegin0), &lgposMax ) );
    BFIResetLgposOldestBegin0( *ppbfNewCurr );
    if ( pbfOrigOld->bfdf == bfdfClean )
    {
        BFIResetLgposOldestBegin0( pbfOrigOld );
    }

    
    Assert( 0 == CmpLgpos( &((*ppbfNewCurr)->lgposModify), &lgposMin ) );
    BFIResetLgposModify( *ppbfNewCurr );
    if ( pbfOrigOld->bfdf == bfdfClean )
    {
        Assert( 0 == CmpLgpos( &(pbfOrigOld->lgposModify), &lgposMin ) );
    }
    (*ppbfNewCurr)->rbsposSnapshot = rbsposMin;


    if ( !pbfOrigOld->fFlushed )
    {
        pbfOrigOld->fFlushed = fTrue;
    }
    else
    {
        (*ppbfNewCurr)->fFlushed = fTrue;
    }


    Assert( pbfOrigOld->sxwl.FOwnWriteLatch() );

    if ( !fOwnsWrite )
    {
        pbfOrigOld->sxwl.DowngradeWriteLatchToExclusiveLatch();
    }

    Assert( pbfOrigOld->sxwl.FOwnExclusiveLatch() || pbfOrigOld->sxwl.FOwnWriteLatch() );
    Assert( (*ppbfNewCurr)->sxwl.FOwnWriteLatch() );


    PERFOpt( cBFPagesVersionCopied.Inc( PinstFromIfmp( pbfOrigOld->ifmp ), pbfOrigOld->tce ) );
    PERFOpt( AtomicIncrement( (LONG*)&g_cBFVersioned ) );
    OSTrace( JET_tracetagBufferManager, OSFormat( "Version Copied: %p (0x%d:%d.%d) to %p (0x%d,%d)\n",
                pbfOrigOld,
                    (ULONG)pbfOrigOld->ifmp, pbfOrigOld->pgno,
                    (ULONG)(*((CPAGE::PGHDR*)pbfOrigOld->pv)).objidFDP,
                (*ppbfNewCurr),
                    (ULONG)(*ppbfNewCurr)->ifmp, (*ppbfNewCurr)->pgno
                ) );
    ETCacheVersionCopyPage( pbfOrigOld->ifmp, pbfOrigOld->pgno );

    return JET_errSuccess;

HandleError:
    Assert( err < JET_errSuccess );
    AssertRTL( err > -65536 && err < 65536 );

    if ( !fOwnsWrite )
    {
        pbfOrigOld->sxwl.DowngradeWriteLatchToExclusiveLatch();
    }

    *ppbfNewCurr = pbfNil;
    return err;
}


OSFILEQOS QosBFIMergeInstUserDispPri( const INST * const pinst, const BFTEMPOSFILEQOS qosIoUserBfTemp )
{
    OSFILEQOS qosIoUser = (OSFILEQOS)qosIoUserBfTemp;

    const OSFILEQOS qosIoDispatch = qosIoUser & qosIODispatchMask;
    if ( qosIoDispatch == 0 )
    {
        qosIoUser |= qosIODispatchImmediate;
    }
    Expected( ( ( qosIoUser & qosIODispatchMask ) == qosIODispatchImmediate ) || ( ( qosIoUser & qosIODispatchMask ) == qosIODispatchBackground ) );

    const OSFILEQOS qosIoUserDispatch = qosIoUser | 
                                        ( ( qosIoUser & qosIODispatchMask ) ?
                                            ( ~qosIODispatchMask & QosSyncDefault( pinst ) ) :
                                            QosSyncDefault( pinst ) );
    return qosIoUserDispatch;
}


ERR ErrBFIPrereadPage( IFMP ifmp, PGNO pgno, const BFPreReadFlags bfprf, const BFPriority bfpri, const TraceContext& tc )
{
    ERR             err     = errBFPageCached;
    BFHash::CLock   lock;
    VOID *          pioreqReserved = NULL;
    BOOL            fBFOwned = fFalse;
    PBF             pbf = NULL;

    if ( !FParentObjectClassSet( tc.nParentObjectClass ) )
    {
        FireWall( "TcPrereadObjectClassTcNotSet" );
    }

    Expected( pgno <= pgnoSysMax );

    Assert( 0 == ( bfprf & bfprfNoIssue ) );


    if ( !FBFInCache( ifmp, pgno ) )
    {
        const OSFILEQOS qosIoUserDispatch = QosBFIMergeInstUserDispPri( PinstFromIfmp( ifmp ), QosBFUserAndIoPri( bfpri ) );
        const OSFILEQOS qos = qosIoUserDispatch | ( ( bfprf & bfprfCombinableOnly ) ? qosIOOptimizeCombinable : 0 );

        Call( ErrBFIAllocPage( &pbf, IcbBFIBufferSize( g_rgfmp[ifmp].CbPage() ), fFalse, fFalse ) );
        fBFOwned = fTrue;

        err = ErrBFIAsyncPreReserveIOREQ( ifmp, pgno, qos, &pioreqReserved );
        CallSx( err, errDiskTilt );
        Call( err );


        err = ErrBFICachePage( &pbf,
                                ifmp,
                                pgno,
                                fFalse,
                                fFalse,
                                fFalse,
                                PctBFCachePri( bfpri ),
                                tc,
                                bfltMax,
                                ( bfprf & bfprfDBScan ) ? bflfDBScan : bflfNone );
        fBFOwned = fFalse;
        Call( err );



        TraceContextScope tcPreread( iorpBFPreread );
        CallS( ErrBFIAsyncRead( pbf, qos, pioreqReserved, *tcPreread ) );
        pioreqReserved = NULL;


        PERFOpt( cBFPagesPreread.Inc( PinstFromIfmp( ifmp ), (TCE) tc.nParentObjectClass ) );
        Ptls()->threadstats.cPagePreread++;
        OSTraceFMP( ifmp, JET_tracetagBufferManager, OSFormat( "Preread page=[0x%x:0x%x]", (ULONG)ifmp, pgno ) );
    }
    else
    {
        PERFOpt( cBFPagesPrereadUnnecessary.Inc( PinstFromIfmp( ifmp ), (TCE) tc.nParentObjectClass ) );
    }

HandleError:

    if ( pioreqReserved != NULL )
    {
        BFIAsyncReleaseUnusedIOREQ( ifmp, pioreqReserved );
    }

    if ( fBFOwned )
    {
        BFIFreePage( pbf, fFalse );
    }

    return err;
}

INLINE ERR ErrBFIValidatePage( const PBF pbf, const BFLatchType bflt, const CPageEvents cpe, const TraceContext& tc )
{

    Assert( bflt == bfltShared || bflt == bfltExclusive || bflt == bfltWrite );


    ERR errBF;
    if ( ( errBF = pbf->err ) >= JET_errSuccess )
    {
        AssertRTL( errBF > -65536 && errBF < 65536 );

        return errBF;
    }

    if ( bflt != bfltShared )
    {
        Assert( FBFIUpdatablePage( pbf ) );
    }

    errBF = ErrBFIValidatePageSlowly( pbf, bflt, cpe, tc );

    if ( BoolParam( PinstFromIfmp( pbf->ifmp ), JET_paramEnableExternalAutoHealing )
        && PagePatching::FIsPatchableError( errBF )
        && CPageValidationLogEvent::LOG_NONE != cpe )
    {
        BFIPatchRequestIORange( pbf, cpe, tc );
    }

    Assert( errBF != errBFIPageNotVerified );
    Assert( errBF != errBFIPageRemapNotReVerified );

    return errBF;
}

INLINE BOOL FBFIDatabasePage( const PBF pbf )
{


    return ( !FFMPIsTempDB( pbf->ifmp ) );
}


LOCAL BOOL FBFIBufferIsZeroed( const PBF pbf );

void BFIValidatePagePgno_( const PBF pbf, PCSTR szFunction )
{
    if ( FBFIDatabasePage( pbf ) && !FIsSmallPage() )
    {
        CPAGE cpage;
        cpage.LoadPage( pbf->ifmp, pbf->pgno, pbf->pv, CbBFIBufferSize( pbf ) );
        const PGNO pgnoOnPage = g_fRepair ? cpage.PgnoREPAIRThis() : cpage.PgnoThis();
        const PGNO pgnoInBF = pbf->pgno;
        if ( pgnoOnPage == 0 || pgnoOnPage != pgnoInBF )
        {
            if ( g_fRepair )
            {
                Enforce( pgnoOnPage == 0 || pgnoOnPage == pgnoInBF );
                Enforce( pgnoOnPage != 0 || FBFIBufferIsZeroed( pbf ) );
            }
            else
            {
                WCHAR wszEnforceMessage[128];
                OSStrCbFormatW( wszEnforceMessage, sizeof(wszEnforceMessage), L"pgnoOnPage(%d) != pgnoInBF(%d) @ %hs", pgnoOnPage, pgnoInBF, szFunction );
                PageEnforceSz( cpage, fFalse, L"CachePgnoMismatch", wszEnforceMessage );
            }
        }
    }
}

void BFIValidatePageUsed( const PBF pbf )
{
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );
}

const ULONG shfUserPriorityTag = 24;
BYTE BUserPriTagOnly( const OSFILEQOS qos )
{
    QWORD qw = ( qos >> shfUserPriorityTag );
    Assert( ( qw & (QWORD)~0xFF ) == 0 );
    Assert( ( ( qosIOUserPriorityTraceMask & qos ) >> shfUserPriorityTag ) == qw );
    return (BYTE)qw;
}

JETUNITTEST( BF, BFICheckUserPriorityTagFitsInHighDwordByte )
{
    CHECK( UsBits( qosIOUserPriorityTraceMask ) == UsBits( ( qosIOUserPriorityTraceMask >> shfUserPriorityTag ) ) );
}

void BFITrackCacheMissLatency( const PBF pbf, HRT hrtStartWait, const BFTraceCacheMissReason bftcmr, const OSFILEQOS qosIoPriorities, const TraceContext& tc, ERR errTrueIo  )
{
    const HRT dhrt = HrtHRTCount() - hrtStartWait;
    const QWORD usecsWait = ( 1000000 * dhrt ) / HrtHRTFreq();

    Assert( bftcmr != bftcmrInvalid );

    if ( !PinstFromIfmp( pbf->ifmp )->FRecovering() )
    {
        PERFOpt( cBFCacheMissLatencyTotalTicksAttached.Add( PinstFromIfmp( pbf->ifmp )->m_iInstance, pbf->tce, CmsecHRTFromDhrt( dhrt ) ) );
        PERFOpt( cBFCacheMissLatencyTotalOperationsAttached.Inc( PinstFromIfmp( pbf->ifmp )->m_iInstance, pbf->tce ) );
    }

    TLS* ptls = Ptls();
    ptls->threadstats.cusecPageCacheMiss += usecsWait;
    ptls->threadstats.cPageCacheMiss++;
    if ( errTrueIo >= JET_errSuccess && FBFIDatabasePage( pbf ) && ( ( CPAGE::PGHDR * )( pbf->pv ) )->fFlags & CPAGE::fPageLongValue )
    {
        ptls->threadstats.cusecLongValuePageCacheMiss += usecsWait;
        ptls->threadstats.cLongValuePageCacheMiss++;
    }


    GetCurrUserTraceContext getutc;
    static_assert( sizeof(bftcmr) == sizeof(BYTE), "Because trace type is as byte" );

    ETCacheMissLatency(
        pbf->ifmp,
        pbf->pgno,
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
        pbf->tce,
        usecsWait,
        bftcmr,
        BUserPriTagOnly( qosIoPriorities ) );
}

ERR ErrBFIValidatePageSlowly( PBF pbf, const BFLatchType bflt, const CPageEvents cpe, const TraceContext& tc )
{
    ERR err = errCodeInconsistency;
    BOOL fRetryingLatch = fFalse;
    TICK tickStartRetryLatch = 0;
    BOOL fPageValidated = fFalse;

    while ( !fPageValidated )
    {
        ERR errFault = JET_errSuccess;

        Assert( err == errCodeInconsistency );
        Assert( ( !fRetryingLatch && ( tickStartRetryLatch == 0 ) ) || ( fRetryingLatch && ( tickStartRetryLatch != 0 ) ) );
        Assert( FBFIOwnsLatchType( pbf, bflt ) );


        ERR errBF = pbf->err;
        if ( ( errBF != errBFIPageNotVerified ) &&
             ( errBF != errBFIPageRemapNotReVerified ) )
        {
            err = ( pbf->bfdf == bfdfClean || errBF >= JET_errSuccess ?
                        errBF :
                        JET_errSuccess );
            AssertRTL( err > -65536 && err < 65536 );
            Assert( errBF != JET_errFileIOBeyondEOF );

            Assert( !fPageValidated );
            fPageValidated = fTrue;
        }


        else if (   bflt != bfltShared ||
                    pbf->sxwl.ErrUpgradeSharedLatchToExclusiveLatch() == CSXWLatch::ERR::errSuccess )
        {
            const BOOL fReverifyingRemap = ( pbf->err == errBFIPageRemapNotReVerified );
            const BOOL fPatchableCodePath =
                            CPageValidationLogEvent::LOG_NONE != cpe &&
                            !fReverifyingRemap;


            if ( pbf->err == errBFIPageNotVerified ||
                    pbf->err == errBFIPageRemapNotReVerified )
            {
                ERR errValidate = JET_errSuccess;
                Assert( FBFIUpdatablePage( pbf ) );
                Assert( pbf->icbPage == pbf->icbBuffer );


                if ( pbf->bfat == bfatViewMapped )
                {
                    Expected( BoolParam( JET_paramEnableViewCache ) );

                    HRT hrtStart = HrtHRTCount();
                    errValidate = g_rgfmp[pbf->ifmp].Pfapi()->ErrMMIORead( OffsetOfPgno( pbf->pgno ), (BYTE*)pbf->pv, CbBFIPageSize( pbf ), IFileAPI::fmmiorfKeepCleanMapped );
                    BFITrackCacheMissLatency( pbf, hrtStart, bftcmrReasonMapViewRead, 0, tc, errValidate );
                    Ptls()->threadstats.cPageRead++;
                }


                if ( errValidate >= JET_errSuccess )
                {
                    errValidate = ErrBFIVerifyPage( pbf, cpe, fTrue );
                }
                if ( errValidate < JET_errSuccess )
                {
                    (void)ErrERRCheck( errValidate );
                }

                pbf->err = SHORT( errValidate );
                Assert( pbf->err == errValidate );

                if ( FBFIDatabasePage( pbf ) && pbf->err >= JET_errSuccess && fPatchableCodePath && !FNegTest( fStrictIoPerfTesting ) )
                {
                    errFault = ErrFaultInjection( 34788 );
                    if ( errFault < JET_errSuccess )
                    {
                        pbf->err = SHORT( errFault );
                        Expected( PagePatching::FIsPatchableError( pbf->err ) );
                    }
                }

                PERFOpt( cBFCacheUnused.Dec( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );


                if ( pbf->bfat == bfatFracCommit )
                {
                    if ( pbf->err >= JET_errSuccess && FBFIDatabasePage( pbf ) )
                    {
                        CPAGE   cpage;
                        Assert( CbBFIBufferSize( pbf ) == CbBFIPageSize( pbf ) );
                        cpage.LoadPage( pbf->ifmp, pbf->pgno, pbf->pv, CbBFIBufferSize( pbf ) );

                        if ( !g_rgfmp[ pbf->ifmp ].m_fReadOnlyAttach &&
                             cpage.Dbtime() < g_rgfmp[ pbf->ifmp ].DbtimeOldestGuaranteed() )
                        {
                            if ( FNDAnyNodeIsVersioned( cpage ) )
                            {
                                NDResetVersionInfo( &cpage );
                                BFIDirtyPage( pbf, bfdfUntidy, *TraceContextScope() );
                            }
                        }

                        cpage.UnloadPage();
                    }
                }

                Assert( FBFIUpdatablePage( pbf ) );

#ifdef ENABLE_CLEAN_PAGE_OVERWRITE

                if ( pbf->fSuspiciouslySlowRead &&
                    pbf->err >= JET_errSuccess &&
                    !g_rgfmp[ pbf->ifmp ].m_fReadOnlyAttach &&
                    !PinstFromIfmp( pbf->ifmp )->m_fTermInProgress )
                {

                    BFIDirtyPage( pbf, bfdfDirty, *TraceContextScope() );
                }
#endif


                if ( pbf->err >= JET_errSuccess &&
                        pbf->icbBuffer == pbf->icbPage )
                {


                    CSXWLatch::ERR errTryW = CSXWLatch::ERR::errSuccess;
                    if ( bflt != bfltWrite )
                    {
                        Assert( pbf->sxwl.FOwnExclusiveLatch() );
                        errTryW = pbf->sxwl.ErrTryUpgradeExclusiveLatchToWriteLatch();
                    }
        

                    if ( CSXWLatch::ERR::errSuccess == errTryW )
                    {
                        Assert( pbf->sxwl.FOwnWriteLatch() );


                        BFIDehydratePage( pbf, fTrue );

                        if ( bflt != bfltWrite )
                        {
                            pbf->sxwl.DowngradeWriteLatchToExclusiveLatch();
                        }
                    }
                }
            }

            if ( PagePatching::FIsPatchableError( pbf->err ) &&
                 fPatchableCodePath &&
                 ( errFault < JET_errSuccess || g_rgfmp[pbf->ifmp].Pfapi()->CLogicalCopies() > 1 ) )
            {


                if ( bflt != bfltWrite &&
                     pbf->sxwl.ErrUpgradeExclusiveLatchToWriteLatch() == CSXWLatch::ERR::errWaitForWriteLatch )
                {
                    PERFOpt( cBFLatchStall.Inc( perfinstGlobal ) );
                    pbf->sxwl.WaitForWriteLatch();
                }
                Assert( pbf->sxwl.FOwnWriteLatch() );

                Assert( pbf->icbBuffer == pbf->icbPage );


                PagePatching::TryPatchFromCopy( pbf->ifmp, pbf->pgno, pbf->pv, &pbf->err );
                if ( pbf->err >= JET_errSuccess &&
                     !g_rgfmp[ pbf->ifmp ].m_fReadOnlyAttach )
                {
                    BFIDirtyPage( pbf, bfdfFilthy, *TraceContextScope() );
                }

                if ( bflt != bfltWrite )
                {
                    pbf->sxwl.DowngradeWriteLatchToExclusiveLatch();
                }
            }


            err = ( pbf->bfdf == bfdfClean || pbf->err >= JET_errSuccess ?
                        pbf->err :
                        JET_errSuccess );
            AssertRTL( err > -65536 && err < 65536 );

            Assert( !fPageValidated );
            fPageValidated = fTrue;


            if ( bflt == bfltShared )
            {
                pbf->sxwl.DowngradeExclusiveLatchToSharedLatch();
            }

            Assert( FBFIOwnsLatchType( pbf, bflt ) );
        }


        else
        {
            Assert( !fRetryingLatch || ( tickStartRetryLatch != 0 ) );
            if ( fRetryingLatch && ( DtickDelta( tickStartRetryLatch, TickOSTimeCurrent() ) < 1000 ) )
            {

                UtilSleep( dtickFastRetry );
                continue;
            }
            
            ERR errValidate = JET_errSuccess;

            Assert( pbf->icbPage == pbf->icbBuffer );
            

            if ( pbf->bfat == bfatViewMapped )
            {
                Expected( BoolParam( JET_paramEnableViewCache ) );
                errValidate = g_rgfmp[pbf->ifmp].Pfapi()->ErrMMIORead( OffsetOfPgno( pbf->pgno ), (BYTE*)pbf->pv, CbBFIPageSize( pbf ), IFileAPI::fmmiorfKeepCleanMapped );
            }


            if ( errValidate >= JET_errSuccess )
            {

                errValidate = ErrBFIVerifyPage( pbf, CPageValidationLogEvent::LOG_NONE, fFalse );
            }


            if ( errValidate < JET_errSuccess )
            {
                if ( ( errValidate == JET_errReadVerifyFailure ) && !fRetryingLatch )
                {
                    fRetryingLatch = fTrue;
                    tickStartRetryLatch = TickOSTimeCurrent();
                    OnDebug( tickStartRetryLatch = ( tickStartRetryLatch == 0 ) ? 1 : tickStartRetryLatch );
                    continue;
                }

                errBF = pbf->err;

                if ( ( errBF != errBFIPageNotVerified ) &&
                     ( errBF != errBFIPageRemapNotReVerified ) )
                {
                    err = ( pbf->bfdf == bfdfClean || errBF >= JET_errSuccess ?
                                errBF :
                                JET_errSuccess );
                }
                else
                {
                    err = errValidate;
                }

                AssertRTL( err > -65536 && err < 65536 );
            }
            else
            {
                err = errValidate;
            }

            Assert( !fPageValidated );
            fPageValidated = fTrue;
        }
    }

    Assert( fPageValidated );
    Assert( err != errCodeInconsistency );


    Assert( FBFIOwnsLatchType( pbf, bflt ) );

    if ( BoolParam( PinstFromIfmp( pbf->ifmp ), JET_paramEnableExternalAutoHealing )
         && CPageValidationLogEvent::LOG_NONE != cpe
         && PagePatching::FIsPatchableError( err ) )
    {
        PagePatching::TryToRequestPatch( pbf->ifmp, pbf->pgno );
    }


    return err;
}


void BFIPatchRequestIORange( PBF pbf, const CPageEvents cpe, const TraceContext& tc )
{
    AssertSz( !g_fRepair, "Page patching should never happen during repair/integrity check." );
    Assert( pbf->sxwl.FOwnSharedLatch() || pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );

    const IFMP ifmp = pbf->ifmp;
    const PGNO pgnoTarget = pbf->pgno;
    const JET_ERR errTarget = pbf->err;

    INT cpgIOSpan = (INT) ( max( UlParam( JET_paramMaxCoalesceReadSize ), UlParam( JET_paramMaxCoalesceWriteSize ) ) / g_rgfmp[ifmp].CbPage() );
    Assert( cpgIOSpan > 0 );
    if ( cpgIOSpan <= 1 )
    {
        return;
    }

    const BOOL fPassive = PinstFromIfmp( ifmp )->m_plog->FRecovering() &&
                            PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() == fRecoveringRedo;

    PGNO pgnoBegin = max( pgnoNull + 1, pgnoTarget - cpgIOSpan + 1 );
    PGNO pgnoEnd = min( pgnoMax, pgnoTarget + cpgIOSpan );


    Assert( g_rgfmp[ifmp].PgnoLast() >= pgnoTarget );
    pgnoEnd = min( pgnoEnd, g_rgfmp[ifmp].PgnoLast() + 1 );

    for ( PGNO pgnoCurr = pgnoBegin; pgnoCurr < pgnoEnd; pgnoCurr++ )
    {
        PGNOPBF         pgnopbf;
        BFHash::ERR     errHash;
        BFHash::CLock   lock;

        if ( pgnoCurr == pgnoTarget )
        {
            continue;
        }
        

        g_bfhash.ReadLockKey( IFMPPGNO( ifmp, pgnoCurr ), &lock );
        errHash = g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf );
        g_bfhash.ReadUnlockKey( &lock );

        if ( errHash == BFHash::ERR::errSuccess )
        {
            if ( FBFITryAcquireExclusiveLatchForMaint( pgnopbf.pbf ) )
            {
                Assert( pgnopbf.pbf->sxwl.FOwnExclusiveLatch() );
                if( FBFICurrentPage( pgnopbf.pbf, ifmp, pgnoCurr ) )
                {
                    (void)ErrBFIValidatePageSlowly( pgnopbf.pbf, bfltExclusive, cpe, tc );
                }

                pgnopbf.pbf->sxwl.ReleaseExclusiveLatch();
            }
        }

        else
        {
            Assert( errHash == BFHash::ERR::errEntryNotFound );



            if ( BoolParam( PinstFromIfmp( ifmp ), JET_paramEnableExternalAutoHealing )
                 && CPageValidationLogEvent::LOG_NONE != cpe
                 && fPassive
                 && PagePatching::FIsPatchableError( errTarget ) )
            {
                PagePatching::TryToRequestPatch( ifmp, pgnoCurr );
            }

        }
    }
}

ERR ErrBFIVerifyPageSimplyWork( const PBF pbf )
{
    BYTE rgBFLocal[sizeof(BF)];
    CPAGE::PGHDR2 pghdr2;
    void * pvPage = NULL;

    if ( !FBFIDatabasePage( pbf ) )
    {
        return JET_errSuccess;
    }
    memcpy( rgBFLocal, pbf, sizeof(BF) );
    CPageValidationNullAction nullaction;
    CPAGE cpage;
    Assert( CbBFIBufferSize( pbf ) == CbBFIPageSize( pbf ) );
    cpage.LoadPage( pbf->ifmp, pbf->pgno, pbf->pv, CbBFIBufferSize( pbf ) );

    const ERR err = cpage.ErrValidatePage( pgvfDoNotCheckForLostFlush, &nullaction );
    if ( JET_errSuccess != err &&
            JET_errPageNotInitialized != err )
    {
        if ( FIsSmallPage() )
        {
            memcpy( &(pghdr2.pghdr), pbf->pv, sizeof(pghdr2.pghdr) );
        }
        else
        {
            memcpy( &(pghdr2), pbf->pv, sizeof(pghdr2) );
        }
        Assert( pbf->icbPage == pbf->icbBuffer );
        pvPage = _alloca( CbBFIPageSize( pbf ) );
        memcpy( pvPage, pbf->pv, CbBFIPageSize( pbf ) );

        EnforceSz( fFalse, OSFormat( "UnexpectedPageValidationFailure:%d", err ) );

        Assert( pghdr2.pghdr.objidFDP <= 0x7FFFFFFF );
    }
    Enforce( ((BF*)rgBFLocal)->ifmp == pbf->ifmp && ((BF*)rgBFLocal)->pgno == pbf->pgno && ((BF*)rgBFLocal)->pv == pbf->pv );
    cpage.UnloadPage();
    return err;
}

ERR ErrBFIVerifyPage( const PBF pbf, const CPageEvents cpe, const BOOL fFixErrors )
{
    ERR err = JET_errSuccess;
    CPageEvents cpeActual = cpe;

    Assert( pbf->icbPage == pbf->icbBuffer );


    if ( !FBFIDatabasePage( pbf ) )
    {

        return JET_errSuccess;
    }

    const PGNO pgno = pbf->pgno;

    if ( fFixErrors || CPageValidationLogEvent::LOG_NONE != cpe )
    {
        Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );
    }
    else
    {
        Assert( pbf->sxwl.FOwner() );
    }

    const LOG * const plog = PinstFromIfmp( pbf->ifmp )->m_plog;
    FMP* const pfmp = &g_rgfmp[ pbf->ifmp ];
    const BOOL fInRecoveryRedo = ( !plog->FLogDisabled() && ( fRecoveringRedo == plog->FRecoveringMode() ) );
    const BOOL fReplayingRequiredRange = fInRecoveryRedo && pfmp->FContainsDataFromFutureLogs();


    if( CPageValidationLogEvent::LOG_NONE != cpeActual )
    {
        if( g_fRepair || fInRecoveryRedo )
        {
            cpeActual &= ~CPageValidationLogEvent::LOG_UNINIT_PAGE;
        }
    }

    CPageValidationLogEvent validationaction(
        pbf->ifmp,
        cpeActual,
        BUFFER_MANAGER_CATEGORY );

    CPAGE cpage;
    Assert( CbBFIBufferSize( pbf ) == CbBFIPageSize( pbf ) );
    cpage.LoadPage( pbf->ifmp, pgno, pbf->pv, CbBFIBufferSize( pbf ) );


    const BOOL fFailOnRuntimeLostFlushOnly = ( ( UlParam( PinstFromIfmp( pbf->ifmp ), JET_paramPersistedLostFlushDetection ) & JET_bitPersistedLostFlushDetectionFailOnRuntimeOnly ) != 0 );
    const PAGEValidationFlags pgvf =
        ( fFixErrors ? pgvfFixErrors : pgvfDefault ) |
        ( ( cpe & CPageValidationLogEvent::LOG_EXTENSIVE_CHECKS ) ? pgvfExtensiveChecks : pgvfDefault ) |
        ( pbf->err != errBFIPageNotVerified ? pgvfDoNotCheckForLostFlush : pgvfDefault ) |
        ( fFailOnRuntimeLostFlushOnly ? pgvfFailOnRuntimeLostFlushOnly : pgvfDefault ) |
        ( fReplayingRequiredRange ? pgvfDoNotCheckForLostFlushIfNotRuntime : pgvfDefault );

    err = cpage.ErrValidatePage( pgvf, &validationaction );


    if ( pbf->err == errBFIPageNotVerified )
    {
        if ( ( err >= JET_errSuccess ) &&
            ( fReplayingRequiredRange || ( pfmp->PFlushMap()->PgftGetPgnoFlushType( pgno ) == CPAGE::pgftUnknown ) ) )
        {
            pfmp->PFlushMap()->SetPgnoFlushType( pgno, cpage.Pgft(), cpage.Dbtime() );
        }
        else if ( ( err == JET_errPageNotInitialized ) && fReplayingRequiredRange )
        {
            pfmp->PFlushMap()->SetPgnoFlushType( pgno, CPAGE::pgftUnknown );
        }
    }

    cpage.UnloadPage();
    
    return err;
}

LOCAL BOOL FBFIBufferIsZeroed( const PBF pbf )
{
    return FUtilZeroed( (BYTE*)pbf->pv, CbBFIBufferSize( pbf ) );
}


bool FBFICurrentPage(
    __in const PBF          pbf,
    __in const IFMP     ifmp,
    __in const PGNO     pgno
    )
{
    Assert( pbf->sxwl.FOwner() );

    return ( pbf->ifmp == ifmp &&
                pbf->pgno == pgno &&
                pbf->fCurrentVersion );
}

bool FBFIUpdatablePage( __in const PBF pbf )
{
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );

    if ( wrnBFPageFlushPending == pbf->err )
    {
        return fFalse;
    }
    return fTrue;
}

#ifdef DEBUG


BOOL FBFIOwnsLatchType( const PBF pbf, const BFLatchType bfltHave )
{
    switch( bfltHave )
    {
        case bfltShared:
            return pbf->sxwl.FOwnSharedLatch();

        case bfltExclusive:
            return pbf->sxwl.FOwnExclusiveLatch();

        case bfltWrite:
            return pbf->sxwl.FOwnWriteLatch();

        default:
            return false;
    }
}
#endif


void BFIReleaseSXWL( __inout PBF const pbf, const BFLatchType bfltHave )
{
    Assert( FBFIOwnsLatchType( pbf, bfltHave ) );
    switch( bfltHave )
    {
        case bfltShared:
            Assert( pbf->sxwl.FOwnSharedLatch() );
            pbf->sxwl.ReleaseSharedLatch();
            break;
        case bfltExclusive:
            Assert( pbf->sxwl.FOwnExclusiveLatch() );
            pbf->sxwl.ReleaseExclusiveLatch();
            break;
        case bfltWrite:
            Assert( pbf->sxwl.FOwnWriteLatch() );
#ifdef MINIMAL_FUNCTIONALITY
#else
            Expected( pbf->bfls != bflsHashed );
            if ( pbf->bfls == bflsHashed )
            {
                const size_t    cProcs  = (size_t)OSSyncGetProcessorCountMax();
                size_t iProc;
                for ( iProc = 0; iProc < cProcs; iProc++ )
                {
                    CSXWLatch* const psxwlProc = &Ppls( iProc )->rgBFHashedLatch[ pbf->iHashedLatch ].sxwl;
                    psxwlProc->ReleaseWriteLatch();
                }
            }
#endif
            pbf->sxwl.ReleaseWriteLatch();
            break;
        default:
            Assert( fFalse );
            break;
    }
}

#ifdef DEBUG
#define EXTRA_LATCHLESS_IO_CHECKS 1
#endif



ERR ErrBFIFaultInBufferIO( __inout BF * const pbf )
{
    ERR err = JET_errSuccess;

    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );

    TRY
    {
        const size_t cbChunk = min( (size_t)CbBFIPageSize( pbf ), OSMemoryPageCommitGranularity() );
        for ( size_t ib = 0; ib < (size_t)CbBFIPageSize( pbf ); ib += cbChunk )
        {
            AtomicExchangeAdd( &((LONG*)((BYTE*)pbf->pv + ib))[0], 0 );
        }
    }
    EXCEPT( efaExecuteHandler )
    {
        err = ErrERRCheck( JET_errDiskIO );
    }
    if ( err < JET_errSuccess )
    {
        OSUHAEmitFailureTag( PinstFromIfmp( pbf->ifmp ), HaDbFailureTagIoHard, L"9bb34106-505c-49b7-a67e-9aedb60756ca" );
    }

    return err;
}

#ifdef DEBUG


BOOL FBFICorruptedNodeSizes( const BF * const pbf )
{
    CPAGE cpageToCorrupt;
    const BFLatch bfl = { pbf->pv, (DWORD_PTR)pbf };
    cpageToCorrupt.ReBufferPage( bfl, pbf->ifmp, pbf->pgno, pbf->pv, g_rgcbPageSize[pbf->icbPage] );
    (void)FNegTestSet( fCorruptingPageLogically );
    if ( FNDCorruptRandomNodeElement( &cpageToCorrupt ) )
    {
        OSTrace( JET_tracetagBufferManager, OSFormat( "FaultInjection: injecting corrupted OS MM page on ifmp:pgno %d:%d", (ULONG)pbf->ifmp, pbf->pgno ) );
        return fTrue;
    }
    return fFalse;
}

#endif


void BFIFaultInBuffer( __inout void * pv, __in LONG cb )
{
    Assert( 0 != cb );

    Assert( !BoolParam( JET_paramEnableViewCache ) );


    const size_t cbChunk = min( (size_t)cb, OSMemoryPageCommitGranularity() );
    size_t ib;
    for ( ib = 0; ib < (size_t)cb; ib += cbChunk )
    {
        (void)AtomicExchangeAdd( &((LONG*)((BYTE*)pv + ib))[0], 0 );
    }
}


void BFIFaultInBuffer( const PBF pbf )
{
#ifdef EXTRA_LATCHLESS_IO_CHECKS
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );
    Assert( FBFIUpdatablePage( pbf ) );
#else
    Assert( pbf->sxwl.FOwner() );
#endif

    Assert( !FBFICacheViewCacheDerefIo( pbf ) );
    if ( FBFICacheViewCacheDerefIo( pbf ) )
    {
        return;
    }


    BFIFaultInBuffer( pbf->pv, CbBFIBufferSize( pbf ) );

}



bool FBFIHasEmptyOSPages( const PBF pbf, LONG * pcmmpgReclaimed )
{
    Assert( pbf->sxwl.FOwnWriteLatch() );
    Assert( FBFIUpdatablePage( pbf ) );

    bool fHasEmptyOSPages = fFalse;


    const size_t cbChunk = min( (size_t)CbBFIBufferSize( pbf ), OSMemoryPageCommitGranularity() );
    size_t ib;
    for ( ib = 0; ib < (size_t)CbBFIBufferSize( pbf ); ib += cbChunk )
    {

        if ( 0 == ((LONG*)((BYTE*)pbf->pv + ib))[0] &&
                0 == ((LONG*)((BYTE*)pbf->pv + ib + cbChunk))[-1] &&
                FUtilZeroed( (BYTE*)pbf->pv + ib, cbChunk ) )
        {
            fHasEmptyOSPages = fTrue;
        }
        else
        {
            if ( pcmmpgReclaimed )
            {
                AtomicIncrement( pcmmpgReclaimed );
            }
        }

    }

    return fHasEmptyOSPages;
}



bool FBFIDestructiveSoftFaultPage( PBF pbf, __in const BOOL fNewPage )
{
    Assert( pbf->sxwl.FOwnWriteLatch() );
    Assert( FBFICurrentPage( pbf, pbf->ifmp, pbf->pgno ) );
    Assert( FBFIUpdatablePage( pbf ) );
    Assert( (DWORD)CbBFIBufferSize( pbf ) >= OSMemoryPageCommitGranularity() );
    Assert( pbf->bfdf < bfdfDirty || fNewPage );
    Assert( !BoolParam( JET_paramEnableViewCache ) );



    OSMemoryPageReset( pbf->pv, CbBFIBufferSize( pbf ) );


    BFIFaultInBuffer( pbf );


    const BOOL fLosePageToOsPageFile = ( !FNegTest( fStrictIoPerfTesting ) && ErrFaultInjection( 17396 ) < JET_errSuccess );
    if ( fLosePageToOsPageFile )
    {
        const BFLatch bfl = { pbf->pv, (DWORD_PTR)pbf };
        CPAGE cpageRuined;
        cpageRuined.ReBufferPage( bfl, pbf->ifmp, pbf->pgno, pbf->pv, g_rgcbPageSize[pbf->icbPage] );
        const DBTIME dbtimeBeforePageOut = FBFIDatabasePage( pbf ) ? cpageRuined.Dbtime() : 0x4242000000004242 ;
        OSTrace( JET_tracetagBufferManager, OSFormat( "FaultInjection: injecting failed OS MM reclaim on ifmp:pgno %d:%d @ %I64d (0x%I64x)", (ULONG)pbf->ifmp, pbf->pgno, dbtimeBeforePageOut, dbtimeBeforePageOut ) );

        Assert( (ULONG)CbBFIBufferSize( pbf ) >= OSMemoryPageCommitGranularity() );
        const size_t cbChunk =  OSMemoryPageCommitGranularity();
        const size_t cMaxChunk = (size_t)CbBFIBufferSize( pbf ) / cbChunk;
        const size_t iZeroedChunk = rand() % cMaxChunk;
        const BYTE * pb = (BYTE*)pbf->pv + ( iZeroedChunk * cbChunk );
        memset( (void*)pb, 0, cbChunk );
    }

    BOOL fPageTrashed = FBFIHasEmptyOSPages( pbf, (LONG*)&g_cpgReclaim );

    Assert( !fLosePageToOsPageFile || fPageTrashed );


    if ( FBFIDatabasePage( pbf ) && !fPageTrashed && pbf->err >= JET_errSuccess )
    {
        
        CPAGE cpageCheck;
        const BFLatch bfl = { pbf->pv, (DWORD_PTR)pbf };
        cpageCheck.ReBufferPage( bfl, pbf->ifmp, pbf->pgno, pbf->pv, g_rgcbPageSize[pbf->icbPage] );

        if ( cpageCheck.FPageIsInitialized() )
        {

            OnDebug( const BOOL fCorruptOnPageIn = ( ErrFaultInjection( 36380 ) < JET_errSuccess ) &&
                                                        FBFICorruptedNodeSizes( pbf ) );
#ifdef DEBUG
            if ( fCorruptOnPageIn )
            {
                OSTrace( JET_tracetagBufferManager, OSFormat( "FaultInjection: injecting corruption on OS MM reclaim on ifmp:pgno %d:%d", (ULONG)pbf->ifmp, pbf->pgno ) );
            }
#endif

            const ERR errCheckPage = cpageCheck.ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(),
                            OnDebugOrRetail( ( fCorruptOnPageIn ? CPAGE::OnErrorReturnError : CPAGE::OnErrorFireWall ), CPAGE::OnErrorFireWall ),
                            CPAGE::CheckLineBoundedByTag );

            Assert( errCheckPage == JET_errSuccess || fCorruptOnPageIn );
            fPageTrashed = ( errCheckPage != JET_errSuccess );

            AssertSz( errCheckPage == JET_errSuccess || fCorruptOnPageIn, "We hit a corruption on a page in event." );
        }
    }

    if ( fPageTrashed )
    {
        OSTrace( JET_tracetagBufferManager, OSFormat( "BF failed reclaim (OS soft fault), note page partially zero'd ifmp:pgno %d:%d", (ULONG)pbf->ifmp, pbf->pgno ) );
        PERFOpt( AtomicIncrement( (LONG*)&g_cbfNonResidentReclaimedFailed ) );
    }
    else
    {
        OSTrace( JET_tracetagBufferManager, OSFormat( "BF succeeded reclaim (OS soft fault) from the OS ifmp:pgno %d:%d", (ULONG)pbf->ifmp, pbf->pgno ) );
        PERFOpt( AtomicIncrement( (LONG*)&g_cbfNonResidentReclaimedSuccess ) );
    }

    return !fPageTrashed;
}




void BFIReclaimPageFromOS(
    __inout PBF                 pbf,
    __in    const BFLatchType   bfltHave,
    __in    const BOOL          fNewPage,
    __in    const BOOL          fWait,
    __in    const OSFILEQOS     qos,
    __in    const CPageEvents   cpe,
    __in    const TraceContext& tc
    )
{
    CSXWLatch::ERR  errSXWL         = CSXWLatch::ERR::errSuccess;
    BFLatchType     bfltAchieved    = bfltHave;

    Assert( !BoolParam( JET_paramEnableViewCache ) );
    Assert( bfltHave == bfltShared || bfltHave == bfltExclusive || bfltHave == bfltWrite );
    Assert( FBFIOwnsLatchType( pbf, bfltHave ) );


    const BOOL  fAttemptDestructiveSoftFault = ( !FNegTest( fStrictIoPerfTesting ) &&
                                                 (DWORD)CbBFIBufferSize( pbf ) >= OSMemoryPageCommitGranularity() &&
                                                 ( pbf->bfdf < bfdfDirty ) );


    if ( fAttemptDestructiveSoftFault )
    {
        switch( bfltHave )
        {
            case bfltShared:
                errSXWL = pbf->sxwl.ErrTryUpgradeSharedLatchToWriteLatch();
                Assert( errSXWL == CSXWLatch::ERR::errSuccess || errSXWL == CSXWLatch::ERR::errLatchConflict );
                if ( errSXWL == CSXWLatch::ERR::errSuccess )
                {
                    bfltAchieved = bfltWrite;
#ifdef MINIMAL_FUNCTIONALITY
#else
                    if ( pbf->bfls == bflsHashed )
                    {
                        pbf->sxwl.DowngradeWriteLatchToSharedLatch();
                        bfltAchieved = bfltShared;
                    }
#endif
                }
                break;

            case bfltExclusive:
                Assert( pbf->sxwl.FOwnExclusiveLatch() );
                if ( fWait )
                {
                    Assert( pbf->sxwl.FOwnExclusiveLatch() );
                    errSXWL = pbf->sxwl.ErrUpgradeExclusiveLatchToWriteLatch();
                    if ( errSXWL == CSXWLatch::ERR::errSuccess ||
                        errSXWL == CSXWLatch::ERR::errWaitForWriteLatch )
                    {
                        if ( errSXWL == CSXWLatch::ERR::errWaitForWriteLatch )
                        {
                            PERFOpt( cBFLatchStall.Inc( perfinstGlobal ) );
                            pbf->sxwl.WaitForWriteLatch();
                        }
                        bfltAchieved = bfltWrite;
                    }
#ifdef MINIMAL_FUNCTIONALITY
#else
                    if ( pbf->bfls == bflsHashed )
                    {
                        pbf->sxwl.DowngradeWriteLatchToExclusiveLatch();
                        bfltAchieved = bfltExclusive;
                    }
#endif
                }
                break;
                
            case bfltWrite:
                break;

            default:
                AssertSz( fFalse, "Unknown latch type (%d), bad things will probably happen!", bfltHave );
        }
    }

#ifdef EXTRA_LATCHLESS_IO_CHECKS
    if ( bfltAchieved == bfltShared )
    {
        errSXWL = pbf->sxwl.ErrUpgradeSharedLatchToExclusiveLatch();
        Assert( errSXWL == CSXWLatch::ERR::errSuccess || errSXWL == CSXWLatch::ERR::errLatchConflict );
        if ( errSXWL == CSXWLatch::ERR::errSuccess )
        {
            bfltAchieved = bfltExclusive;
        }
    }
#endif

    Assert( FBFIOwnsLatchType( pbf, bfltAchieved ) );


    const BFResidenceState bfrsOld = BfrsBFIUpdateResidentState( pbf, bfrsResident );

    if ( bfrsResident == bfrsOld )
    {
        goto HandleError;
    }


#ifdef EXTRA_LATCHLESS_IO_CHECKS
    if ( bfltAchieved >= bfltExclusive &&
#else
    if ( bfltAchieved == bfltWrite &&
#endif
            !FBFIUpdatablePage( pbf ) )
    {
        goto HandleError;
    }


    if ( fAttemptDestructiveSoftFault && bfltAchieved == bfltWrite )
    {


        if ( !FBFIDestructiveSoftFaultPage( pbf, fNewPage ) && !fNewPage )
        {


            if ( pbf->icbBuffer != pbf->icbPage )
            {

                CallS( ErrBFISetBufferSize( pbf, (ICBPage)pbf->icbPage, fTrue ) );
            }

            TraceContextScope tcReclaimFromOS;
            tcReclaimFromOS->iorReason.AddFlag( iorfReclaimPageFromOS );
            BFISyncRead( pbf, qos, *tcReclaimFromOS );
            PERFOpt( AtomicIncrement( (LONG*)&g_cbfNonResidentRedirectedToDatabase ) );
            PERFOpt( cBFPagesRepeatedlyRead.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
            OSTraceFMP(
                pbf->ifmp,
                JET_tracetagBufferManager,
                OSFormat( "Sync-read page=[0x%x:0x%x] (due to OS reclaim fail)", (ULONG)pbf->ifmp, pbf->pgno ) );
            (void)ErrBFIValidatePage( pbf, bfltWrite, cpe, *tcReclaimFromOS );
            Assert( !fNewPage );
            goto HandleError;
        }

        if ( fNewPage )
        {
            Assert( bfltHave == bfltWrite );

            if ( pbf->icbBuffer != pbf->icbPage )
            {

                CallS( ErrBFISetBufferSize( pbf, (ICBPage)pbf->icbPage, fTrue ) );
            }

            memset( pbf->pv, 0, sizeof(CPAGE::PGHDR2) );
        }

    }
    else
#ifdef EXTRA_LATCHLESS_IO_CHECKS
        if ( bfltAchieved >= bfltExclusive )
#endif
    {


        const HRT hrtFaultStart = HrtHRTCount();

        BFIFaultInBuffer( pbf );

        const HRT dhrtFault = HrtHRTCount() - hrtFaultStart;
        const QWORD cusecFaultTime = CusecHRTFromDhrt( dhrtFault );
        Assert( ( ( 1000 * dhrtFault ) / HrtHRTFreq() ) < (HRT)lMax );

        if ( cusecFaultTime > 100 )
        {

            PERFOpt( AtomicAdd( (QWORD*)&g_cusecNonResidentFaultedInLatencyTotal, cusecFaultTime ) );
            PERFOpt( AtomicIncrement( (LONG*)&g_cbfNonResidentReclaimedHardSuccess ) );

            BFITrackCacheMissLatency( pbf, hrtFaultStart, bftcmrReasonPagingFaultOs, qos, tc, JET_errSuccess );
        }

        OSTrace( JET_tracetagBufferManager, OSFormat( "BF forced fault (soft or hard fault) from the OS ifmp:pgno %d:%d in %I64d ms", (ULONG)pbf->ifmp, pbf->pgno, cusecFaultTime / 1000 ) );
    }


    if ( bfltAchieved >= bfltExclusive && FBFIDatabasePage( pbf ) && pbf->err >= JET_errSuccess && !fNewPage )
    {
        
        CPAGE cpageCheck;
        const BFLatch bfl = { pbf->pv, (DWORD_PTR)pbf };
        cpageCheck.ReBufferPage( bfl, pbf->ifmp, pbf->pgno, pbf->pv, g_rgcbPageSize[pbf->icbPage] );

        if ( cpageCheck.FPageIsInitialized() )
        {
#ifdef DEBUG
            if ( ErrFaultInjection( 52764 ) < JET_errSuccess )
            {
                (void)FBFICorruptedNodeSizes( pbf );
            }
#endif

            (void)cpageCheck.ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(),
                            CPAGE::OnErrorFireWall,
                            CPAGE::CheckLineBoundedByTag );
        }
    }

HandleError:


    Assert( bfltHave <= bfltAchieved );
    switch( bfltAchieved )
    {
        case bfltWrite:
            if ( bfltHave == bfltShared )
            {
                pbf->sxwl.DowngradeWriteLatchToSharedLatch();
            }
            else if ( bfltHave == bfltExclusive )
            {
                pbf->sxwl.DowngradeWriteLatchToExclusiveLatch();
            }
            else
            {
                Assert( bfltAchieved == bfltHave );
            }
            break;
        case bfltExclusive:
            if ( bfltHave == bfltShared )
            {
                pbf->sxwl.DowngradeExclusiveLatchToSharedLatch();
            }
            else
            {
                Assert( bfltAchieved == bfltHave );
            }
            break;
        default:
            Assert( bfltAchieved == bfltHave );
    }

    Assert( FBFIOwnsLatchType( pbf, bfltHave ) );

}


CPageEvents CpeBFICPageEventsFromBflf( const BFLatchFlags bflf )
{
    INT flags;
    if ( bflf & bflfNoEventLogging )
    {
        flags = CPageValidationLogEvent::LOG_NONE;
    }
    else
    {
        flags = CPageValidationLogEvent::LOG_ALL;

        if ( bflf & bflfUninitPageOk )
        {
            flags = flags & ~CPageValidationLogEvent::LOG_UNINIT_PAGE;
        }

        if ( bflf & bflfExtensiveChecks )
        {
            flags = flags | CPageValidationLogEvent::LOG_EXTENSIVE_CHECKS;
        }
    }
    return flags;
}

NOINLINE void BFIAsyncReadWait( __in PBF pbf, __in const BFLatchType bfltWaiting, const BFPriority bfpri, const TraceContext& tc )
{

    PERFOpt( cBFPrereadStall.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );


    HRT hrtStart = HrtHRTCount();
    switch( bfltWaiting )
    {
        case bfltShared:
            pbf->sxwl.WaitForSharedLatch();
            break;
        case bfltExclusive:
            pbf->sxwl.WaitForExclusiveLatch();
            break;
        default:
            AssertSz( fFalse, "Waiting for unexpected latchtype in BFIAsyncReadWait!" );
    }

    TraceContextScope tcAsyncReadWait( iorpBFLatch );
    if ( !FBFICacheViewCacheDerefIo( pbf ) )
    {
        BFITrackCacheMissLatency(   pbf,
                                hrtStart,
                                bftcmrReasonPrereadTooSlow,
                                QosBFUserAndIoPri( bfpri ),
                                *tcAsyncReadWait,
                                pbf->err == errBFIPageNotVerified ? JET_errSuccess : pbf->err );
    }

    Assert( FBFIOwnsLatchType( pbf, bfltWaiting ) );
}

C_ASSERT( bfltShared == CSXWLatch::iSharedGroup );
C_ASSERT( bfltExclusive == CSXWLatch::iExclusiveGroup );
C_ASSERT( bfltWrite == CSXWLatch::iWriteGroup );

const BFLatchFlags bflfDefaultValue = bflfDefault;

void BFIInitialize( __in PBF pbf, const TraceContext& tc )
{
    Assert( pbf->sxwl.FOwnWriteLatch() );

    const IFMP ifmp = pbf->ifmp;

    AssertRTL( pbf->err > -65536 && pbf->err < 65536 );
    if ( pbf->err == errBFIPageNotVerified )
    {
        PERFOpt( cBFCacheUnused.Dec( PinstFromIfmp( ifmp ), pbf->tce ) );
    }


    PERFOpt( cBFCache.Dec( PinstFromIfmp( ifmp ), pbf->tce, ifmp ) );
    PERFOpt( cBFCache.Inc( PinstFromIfmp( ifmp ), (TCE) tc.nParentObjectClass, ifmp ) );

    pbf->tce = (TCE) tc.nParentObjectClass;

    pbf->err = JET_errSuccess;


    pbf->fAbandoned = fFalse;


    if (  pbf->icbBuffer != pbf->icbPage &&
            pbf->bfdf == bfdfClean )
    {

        CallS( ErrBFISetBufferSize( pbf, (ICBPage)pbf->icbPage, fTrue ) );
    }
}

ERR ErrBFILatchPage(    _Out_ BFLatch* const    pbfl,
                        const IFMP              ifmp,
                        const PGNO              pgno,
                        const BFLatchFlags      bflf,
                        const BFLatchType       bfltReq,
                        const BFPriority        bfpri,
                        const TraceContext&     tc,
                        _Out_ BOOL* const       pfCachedNewPage )
{
    ERR             err         = JET_errSuccess;
    BFLatchFlags    bflfT       = bflf;
    BFLatchType     bfltHave    = bfltNone;
    BOOL            fCacheMiss  = fFalse;
    IFMPPGNO        ifmppgno    = IFMPPGNO( ifmp, pgno );
    ULONG           cRelatches  = 0;
    PGNOPBF         pgnopbf;
    BFHash::ERR     errHash;
    BFHash::CLock   lock;
    CSXWLatch::ERR  errSXWL     = CSXWLatch::ERR::errSuccess;

    Enforce( pgno != pgnoNull );
    Assert( bfltReq != bfltNone );

    if ( !FParentObjectClassSet( tc.nParentObjectClass ) )
    {
        FireWall( "TcLatchObjectClassNotSet" );
    }

    if ( !FEngineObjidSet( tc.dwEngineObjid ) )
    {
        FireWall( "TcLatchEngineObjidNotSet" );
    }

    const CPageEvents   cpe     = CpeBFICPageEventsFromBflf( bflf );

#ifdef DEBUG
    const ULONG cRelatchesLimit = 11;
    struct _BFIRelatchTrackingInfo {
        TICK        tickStart;
        TICK        tickHashLock;
        TICK        tickLatch;
        ULONG       ulLineContinue;
        PBF     pbf;
    } relatchinfo [cRelatchesLimit];
#endif

    Expected( pgno <= pgnoSysMax );
    Expected( ( pgno < g_rgfmp[ifmp].PgnoLast() + 262144 )  || ( bflfT & bflfLatchAbandoned ) );

    Assert( !( bflf & ( bflfNew | bflfNewIfUncached ) ) || ( bfltReq == bfltWrite ) );


    forever
    {
        Assert( bfltHave == bfltNone );

        if ( pfCachedNewPage != NULL )
        {
            *pfCachedNewPage = fFalse;
        }

        Assert( cRelatches < cRelatchesLimit );
        Assert( cRelatches == 0 || relatchinfo[cRelatches-1].ulLineContinue );
        OnDebug( memset( &(relatchinfo[cRelatches]), 0, sizeof(relatchinfo[cRelatches]) ) );
        OnDebug( relatchinfo[cRelatches].tickStart = TickOSTimeCurrent() );


        g_bfhash.ReadLockKey( ifmppgno, &lock );
        errHash = g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf );

        OnDebug( relatchinfo[cRelatches].tickHashLock = TickOSTimeCurrent() );
        OnDebug( relatchinfo[cRelatches].pbf = pgnopbf.pbf );


        if ( errHash == BFHash::ERR::errSuccess )
        {
            Assert( pgnopbf.pbf->ifmp == ifmp );
            Assert( pgnopbf.pbf->pgno == pgno );
            Assert( !pgnopbf.pbf->fAvailable );
            Assert( !pgnopbf.pbf->fQuiesced );


            if ( bflfT & bflfNoCached )
            {
                g_bfhash.ReadUnlockKey( &lock );
                return ErrERRCheck( errBFPageCached );
            }


            fCacheMiss = fCacheMiss || pgnopbf.pbf->err == errBFIPageFaultPending;


            switch ( bfltReq )
            {
                case bfltShared:
                    if ( bflfT & bflfNoWait )
                    {
                        errSXWL = pgnopbf.pbf->sxwl.ErrTryAcquireSharedLatch();
                    }
                    else
                    {
                        errSXWL = pgnopbf.pbf->sxwl.ErrAcquireSharedLatch();
                    }
                    break;

                case bfltExclusive:
                    if ( bflfT & bflfNoWait )
                    {
                        errSXWL = pgnopbf.pbf->sxwl.ErrTryAcquireExclusiveLatch();
                    }
                    else
                    {
                        errSXWL = pgnopbf.pbf->sxwl.ErrAcquireExclusiveLatch();
                    }
                    break;

                case bfltWrite:
                    if ( bflfT & bflfNoWait )
                    {
                        errSXWL = pgnopbf.pbf->sxwl.ErrTryAcquireWriteLatch();
#ifdef MINIMAL_FUNCTIONALITY
#else
                        if ( errSXWL == CSXWLatch::ERR::errSuccess && pgnopbf.pbf->bfls == bflsHashed )
                        {
                            const size_t    cProcs  = (size_t)OSSyncGetProcessorCountMax();
                            size_t          iProc   = 0;
                            for ( iProc = 0; iProc < cProcs; iProc++ )
                            {
                                CSXWLatch* const psxwlProc = &Ppls( iProc )->rgBFHashedLatch[ pgnopbf.pbf->iHashedLatch ].sxwl;
                                errSXWL = psxwlProc->ErrTryAcquireWriteLatch();
                                if ( errSXWL != CSXWLatch::ERR::errSuccess )
                                {
                                    break;
                                }
                            }
                            if ( errSXWL != CSXWLatch::ERR::errSuccess )
                            {
                                for ( size_t iProc2 = 0; iProc2 < iProc; iProc2++ )
                                {
                                    CSXWLatch* const psxwlProc = &Ppls( iProc2 )->rgBFHashedLatch[ pgnopbf.pbf->iHashedLatch ].sxwl;
                                    psxwlProc->ReleaseWriteLatch();
                                }
                                pgnopbf.pbf->sxwl.ReleaseWriteLatch();
                            }
                        }
#endif
                    }
                    else
                    {
                        errSXWL = pgnopbf.pbf->sxwl.ErrAcquireExclusiveLatch();
                    }
                    break;

                default:
                    Assert( fFalse );
                    errSXWL = CSXWLatch::ERR::errLatchConflict;
                    break;
            }


            g_bfhash.ReadUnlockKey( &lock );


            if ( errSXWL == CSXWLatch::ERR::errLatchConflict )
            {
                PERFOpt( cBFLatchConflict.Inc( perfinstGlobal ) );
                return ErrERRCheck( errBFLatchConflict );
            }


            else if ( errSXWL == CSXWLatch::ERR::errWaitForSharedLatch )
            {
                if ( pgnopbf.pbf->err == errBFIPageFaultPending )
                {
                    BFIAsyncReadWait( pgnopbf.pbf, bfltShared, bfpri, tc );
                }
                else
                {
                    PERFOpt( cBFLatchStall.Inc( perfinstGlobal ) );
                    pgnopbf.pbf->sxwl.WaitForSharedLatch();
                }
            }

            else if ( errSXWL == CSXWLatch::ERR::errWaitForExclusiveLatch )
            {
                if ( pgnopbf.pbf->err == errBFIPageFaultPending )
                {
                    BFIAsyncReadWait( pgnopbf.pbf, bfltExclusive, bfpri, tc );
                }
                else
                {
                    PERFOpt( cBFLatchStall.Inc( perfinstGlobal ) );
                    pgnopbf.pbf->sxwl.WaitForExclusiveLatch();
                }
            }
            else
            {
                Assert( errSXWL == CSXWLatch::ERR::errSuccess );
            }

            if ( bfltReq == bfltWrite && !( bflfT & bflfNoWait ) )
            {
                if ( pgnopbf.pbf->sxwl.ErrUpgradeExclusiveLatchToWriteLatch() == CSXWLatch::ERR::errWaitForWriteLatch )
                {
                    PERFOpt( cBFLatchStall.Inc( perfinstGlobal ) );
                    pgnopbf.pbf->sxwl.WaitForWriteLatch();
                }
#ifdef MINIMAL_FUNCTIONALITY
#else
                if ( pgnopbf.pbf->bfls == bflsHashed )
                {
                    const size_t    cProcs  = (size_t)OSSyncGetProcessorCountMax();
                    for ( size_t iProc = 0; iProc < cProcs; iProc++ )
                    {
                        CSXWLatch* const psxwlProc = &Ppls( iProc )->rgBFHashedLatch[ pgnopbf.pbf->iHashedLatch ].sxwl;
                        if ( psxwlProc->ErrAcquireExclusiveLatch() == CSXWLatch::ERR::errWaitForExclusiveLatch )
                        {
                            PERFOpt( cBFLatchStall.Inc( perfinstGlobal ) );
                            psxwlProc->WaitForExclusiveLatch();
                        }
                        if ( psxwlProc->ErrUpgradeExclusiveLatchToWriteLatch() == CSXWLatch::ERR::errWaitForWriteLatch )
                        {
                            PERFOpt( cBFLatchStall.Inc( perfinstGlobal ) );
                            psxwlProc->WaitForWriteLatch();
                        }
                    }
                }
#endif
            }

            Assert( FBFIOwnsLatchType( pgnopbf.pbf, bfltReq ) );
            bfltHave = bfltReq;

            OnDebug( relatchinfo[cRelatches].tickLatch = TickOSTimeCurrent() );

            Assert( pgnopbf.pbf->ifmp == ifmp );
            Assert( pgnopbf.pbf->pgno == pgno );
            Assert( !pgnopbf.pbf->fAvailable );
            Assert( !pgnopbf.pbf->fQuiesced );

            if ( bfltReq == bfltWrite )
            {
#ifdef DEBUG
                Assert( pgnopbf.pbf->pgno <= g_rgfmp[ifmp].PgnoLast() || g_rgfmp[ifmp].FOlderDemandExtendDb() || ( bflfT & bflfLatchAbandoned ) );
#else
                AssertTrack( pgnopbf.pbf->pgno <= g_rgfmp[ifmp].PgnoLast() || ( bflfT & bflfLatchAbandoned ), "WriteLatchPgnoLastTooLow" );
#endif

                g_rgfmp[ifmp].UpdatePgnoHighestWriteLatched( pgno );
                if ( !( bflf & bflfDBScan ) )
                {
                    g_rgfmp[ifmp].UpdatePgnoWriteLatchedNonScanMax( pgno );
                }
            }

            if ( bflf & bflfDBScan )
            {
                g_rgfmp[ifmp].UpdatePgnoLatchedScanMax( pgno );
            }


            if ( !FBFICurrentPage( pgnopbf.pbf, ifmp, pgno ) )
            {

                BFIReleaseSXWL( pgnopbf.pbf, bfltReq );
                bfltHave = bfltNone;
                pgnopbf.pbf = NULL;
                pgnopbf.pgno = pgnoNull;

                OnDebug( relatchinfo[cRelatches].ulLineContinue = __LINE__ );
                cRelatches++;
                continue;
            }

            if ( ( bfltExclusive == bfltReq || bfltWrite == bfltReq ) &&
                    !FBFIUpdatablePage( pgnopbf.pbf ) )
            {
                PBF pbfNew = NULL;

                AssertRTL( pgnopbf.pbf->err > -65536 && pgnopbf.pbf->err < 65536 );

                if ( pgnopbf.pbf->err == wrnBFPageFlushPending &&
                        FBFICompleteFlushPage( pgnopbf.pbf, bfltReq ) )
                {

                    Enforce( pgnopbf.pbf->err < JET_errSuccess || pgnopbf.pbf->bfdf == bfdfClean );
                    Enforce( FBFIUpdatablePage( pgnopbf.pbf ) );
                    Assert( pgnopbf.pbf->err != errBFIPageRemapNotReVerified );

                }
                else
                {

                    PERFOpt( cBFLatchConflict.Inc( perfinstGlobal ) );

                    const ERR errVersion = ErrBFIVersionCopyPage( pgnopbf.pbf, &pbfNew, fTrue, ( bfltWrite == bfltReq ) );

                    AssertRTL( errVersion > -65536 && errVersion < 65536 );

                    if ( errVersion >= JET_errSuccess )
                    {
                        CallS( errVersion );

                        Assert( pbfNew );
                        if ( pgnopbf.pbf->bfdf > bfdfClean )
                        {
                            Assert( pgnopbf.pbf->pbfTimeDepChainPrev == pbfNew );
                            Assert( pbfNew->pbfTimeDepChainNext == pgnopbf.pbf );
                        }
                        Assert( !pgnopbf.pbf->fCurrentVersion );
                        Assert( pgnopbf.pbf->fOlderVersion );
                        Assert( !pbfNew->fOlderVersion );
                        Assert( pbfNew->fCurrentVersion );

                        Assert( FBFWriteLatched( ifmp, pgno ) );

                        BFIReleaseSXWL( pgnopbf.pbf, bfltReq );
                        bfltHave = bfltNone;
                        pgnopbf.pbf = NULL;
                        pgnopbf.pgno = pgnoNull;

                        Assert( FBFWriteLatched( ifmp, pgno ) );

                        pbfNew->sxwl.ReleaseWriteLatch();


                        OnDebug( relatchinfo[cRelatches].ulLineContinue = __LINE__ );
                        cRelatches++;
                        continue;

                    }
                    else
                    {
                        Assert( JET_errOutOfMemory == errVersion );
                        Assert( NULL == pbfNew );

                        BFIReleaseSXWL( pgnopbf.pbf, bfltReq );
                        bfltHave = bfltNone;
                        pgnopbf.pbf = NULL;
                        pgnopbf.pgno = pgnoNull;

                        return errVersion;
                    }
                }
            }


            BFIMaintCacheResidencyRequest();


            (void)ErrBFIMaintCacheStatsRequest( bfmcsrtNormal );

            const OSFILEQOS qosIoPriorities = QosBFIMergeInstUserDispPri( PinstFromIfmp( pgnopbf.pbf->ifmp ), QosBFUserAndIoPri( bfpri ) );

            if ( bfrsResident != pgnopbf.pbf->bfrs &&
                    pgnopbf.pbf->bfat == bfatFracCommit )
            {
                TraceContextScope tcBFLatch( iorpBFLatch );

                BFIReclaimPageFromOS( pgnopbf.pbf, bfltReq, ( bflfT & bflfNew ), !( bflfT & bflfNoWait ), qosIoPriorities, cpe, *tcBFLatch );
            }


            if ( pgnopbf.pbf->bfat == bfatViewMapped && !FBFICacheViewFresh( pgnopbf.pbf ) )
            {
                TraceContextScope tcBFLatch( iorpBFLatch );
                err = ErrBFICacheViewFreshen( pgnopbf.pbf, qosIoPriorities, *tcBFLatch );

                if ( err < JET_errSuccess )
                {
                    
                    BFIReleaseSXWL( pgnopbf.pbf, bfltReq );
                    bfltHave = bfltNone;

                    return err;
                }
            }


            if ( pgnopbf.pbf->fAbandoned && !( bflfT & bflfLatchAbandoned ) )
            {
                BFIReleaseSXWL( pgnopbf.pbf, bfltReq );
                bfltHave = bfltNone;
                pgnopbf.pbf = NULL;
                pgnopbf.pgno = pgnoNull;

                OnDebug( relatchinfo[cRelatches].ulLineContinue = __LINE__ );
                cRelatches++;
                UtilSleep( ( cRelatches * cRelatches / 2 ) * dtickFastRetry );
                continue;
            }

            Assert( !( bflfT & ( bflfNew | bflfNewIfUncached ) ) || ( bfltReq == bfltWrite ) );


            if ( bflfT & bflfNew )
            {
                Assert( pgnopbf.pbf->ifmp == ifmp );
                BFIInitialize( pgnopbf.pbf, tc );

                if ( pfCachedNewPage != NULL )
                {
                    *pfCachedNewPage = fTrue;
                }
                

                err = JET_errSuccess;
            }


            else
            {


                if ( pgnopbf.pbf->err == errBFIPageNotVerified )
                {
                    bflfT = BFLatchFlags( bflfT | bflfNoTouch );
                }


                TraceContextScope tcBFLatch( iorpBFLatch );
                err = ErrBFIValidatePage( pgnopbf.pbf, bfltReq, cpe, *tcBFLatch );
                AssertRTL( err > -65536 && err < 65536 );

                Assert( err != JET_errOutOfMemory &&
                        err != JET_errOutOfBuffers );


                AssertTrack( err != JET_errFileIOBeyondEOF, "BFILatchEofCachedValidate" );


                if ( ( ( err < JET_errSuccess ) && !( bflfT & bflfNoFaultFail ) ) ||
                     ( err == JET_errFileIOBeyondEOF )  )
                {

                    BFIReleaseSXWL( pgnopbf.pbf, bfltReq );
                    bfltHave = bfltNone;

                    return err;
                }


                Assert( err >= JET_errSuccess || ( bflfT & bflfNoFaultFail ) );

                if ( err < JET_errSuccess )
                {
                    if ( bflfT & bflfNoFaultFail )
                    {
                        err = JET_errSuccess;
                    }
                }
            }


            if ( FBFICacheViewCacheDerefIo( pgnopbf.pbf ) &&
                    ( bfltReq == bfltExclusive || bfltReq == bfltWrite ) &&
                    pgnopbf.pbf->bfdf == bfdfClean )
            {
                err = ErrBFIFaultInBufferIO( pgnopbf.pbf );
                AssertTrack( err != JET_errFileIOBeyondEOF, "BFILatchEofCachedMM" );


                if ( ( ( err < JET_errSuccess ) && !( bflfT & bflfNoFaultFail ) ) ||
                     ( err == JET_errFileIOBeyondEOF )  )
                {

                    BFIReleaseSXWL( pgnopbf.pbf, bfltReq );
                    bfltHave = bfltNone;

                    return err;
                }


                Assert( err >= JET_errSuccess || ( bflfT & bflfNoFaultFail ) );

                if ( err < JET_errSuccess )
                {
                    if ( bflfT & bflfNoFaultFail )
                    {
                        err = JET_errSuccess;
                    }
                }
            }



            const BOOL fTouchPage = ( !( bflfT & bflfNoTouch ) && !BoolParam( JET_paramEnableFileCache ) );

#ifndef MINIMAL_FUNCTIONALITY
            if ( fTouchPage &&
                g_fBFMaintHashedLatches &&
                pgnopbf.pbf->bfat == bfatFracCommit &&
                g_bflruk.FSuperHotResource( pgnopbf.pbf ) )
            {
                BFILatchNominate( pgnopbf.pbf );
            }
#endif

            BFITouchResource( pgnopbf.pbf, bfltReq, bflfT, fTouchPage, PctBFCachePri( bfpri ), tc );


            break;
        }


        else
        {
            Assert( bfltHave == bfltNone );


            g_bfhash.ReadUnlockKey( &lock );


            if ( bflfT & bflfNoUncached )
            {
                return ErrERRCheck( errBFPageNotCached );
            }


            fCacheMiss = fTrue;

            const BOOL fNewPage = ( bflfT & bflfNew ) || ( bflfT & bflfNewIfUncached );


            pgnopbf.pbf = NULL;
            err = ErrBFICachePage(  &pgnopbf.pbf,
                                    ifmp,
                                    pgno,
                                    fNewPage,
                                    fTrue,
                                    fNewPage,
                                    PctBFCachePri( bfpri ),
                                    tc,
                                    bfltReq,
                                    bflfT );
            AssertRTL( err > -65536 && err < 65536 );
            AssertTrack( ( err != JET_errFileIOBeyondEOF ) || !fNewPage, "BFILatchEofUncached" );


            if ( err == JET_errSuccess )
            {
                bfltHave = bfltWrite;

                TraceContextScope tcBFLatch( iorpBFLatch );

                Assert( pgnopbf.pbf->sxwl.FOwnWriteLatch() );
                Assert( pgnopbf.pbf->icbPage == pgnopbf.pbf->icbBuffer );


                if ( fNewPage )
                {

                    pgnopbf.pbf->fAbandoned = fFalse;

                    if ( pfCachedNewPage != NULL )
                    {
                        *pfCachedNewPage = fTrue;
                    }
                }


                if ( !fNewPage )
                {

                    pgnopbf.pbf->fSyncRead = fTrue;
                    BFISyncRead( pgnopbf.pbf, QosBFIMergeInstUserDispPri( PinstFromIfmp( ifmp ), QosBFUserAndIoPri( bfpri ) ), *tcBFLatch );
                    OSTraceFMP(
                        ifmp,
                        JET_tracetagBufferManager,
                        OSFormat( "Sync-read page=[0x%x:0x%x]", (ULONG)ifmp, pgno ) );


                    err = ErrBFIValidatePage( pgnopbf.pbf, bfltWrite, cpe, *tcBFLatch );
                    AssertRTL( err > -65536 && err < 65536 );
                    AssertTrack( err != JET_errFileIOBeyondEOF, "BFILatchEofUncachedValidate" );

                    Expected( err != JET_errOutOfMemory &&
                                err != JET_errOutOfBuffers );


                    if ( ( ( err < JET_errSuccess ) && !( bflfT & bflfNoFaultFail ) ) ||
                         ( err == JET_errFileIOBeyondEOF )  )
                    {

                        pgnopbf.pbf->sxwl.ReleaseWriteLatch();
                        bfltHave = bfltNone;
                        return err;
                    }

                    
                    Assert( err >= JET_errSuccess || ( bflfT & bflfNoFaultFail ) );

                    if ( err < JET_errSuccess )
                    {
                        if ( bflfT & bflfNoFaultFail )
                        {
                            err = JET_errSuccess;
                        }
                    }
                }


                if ( fNewPage )
                {
                }
                else
                {
                    if ( FBFIDatabasePage( pgnopbf.pbf ) )
                    {
                        
                        GetCurrUserTraceContext getutc;

                        CPAGE::PGHDR * ppghdr = (CPAGE::PGHDR *)pgnopbf.pbf->pv;
                        ETCacheReadPage(
                            ifmp,
                            pgno,
                            bflf,
                            ppghdr->objidFDP,
                            ppghdr->fFlags,
                            getutc->context.dwUserID,
                            getutc->context.nOperationID,
                            getutc->context.nOperationType,
                            getutc->context.nClientType,
                            getutc->context.fFlags,
                            getutc->dwCorrelationID,
                            tcBFLatch->iorReason.Iorp(),
                            tcBFLatch->iorReason.Iors(),
                            tcBFLatch->iorReason.Iort(),
                            tcBFLatch->iorReason.Ioru(),
                            tcBFLatch->iorReason.Iorf(),
                            tcBFLatch->nParentObjectClass,
                            ppghdr->dbtimeDirtied,
                            ppghdr->itagMicFree,
                            ppghdr->cbFree );
                    }
                }


                Assert( pgnopbf.pbf->bfls != bflsHashed );
                pgnopbf.pbf->tickViewLastRefreshed = TickBFIHashedLatchTime( TickOSTimeCurrent() );
                pgnopbf.pbf->tickEligibleForNomination = pgnopbf.pbf->tickViewLastRefreshed;


                Assert( bfltHave == bfltWrite );

                break;
            }


            else if ( err == errBFPageCached )
            {

                OnDebug( relatchinfo[cRelatches].ulLineContinue = __LINE__ );
                cRelatches++;
                continue;
            }


            else
            {
                Assert( err == JET_errOutOfMemory ||
                        err == JET_errOutOfBuffers ||
                        err == JET_errFileIOBeyondEOF );

                AssertRTL( err > -65536 && err < 65536 );

                return err;
            }
        }
    }


    if ( ( bfltHave != bfltReq ) && ( bfltHave != bfltNone ) )
    {
        EnforceSz( bfltHave == bfltWrite, OSFormat( "BadLatchDowngrade:%d", (int)bfltHave ) );
        Expected( fCacheMiss );
        switch ( bfltReq )
        {
            case bfltShared:
                pgnopbf.pbf->sxwl.DowngradeWriteLatchToSharedLatch();
                bfltHave = bfltShared;
                break;

            case bfltExclusive:
                pgnopbf.pbf->sxwl.DowngradeWriteLatchToExclusiveLatch();
                bfltHave = bfltExclusive;
                break;

            case bfltWrite:
                break;

            default:
                Assert( fFalse );
                break;
        }
    }


    if ( (TCE)tc.nParentObjectClass != tceNone && pgnopbf.pbf->tce == tceNone )
    {
        PERFOpt( cBFCache.Dec( PinstFromIfmp( ifmp ), pgnopbf.pbf->tce, ifmp ) );
        PERFOpt( cBFCache.Inc( PinstFromIfmp( ifmp ), (TCE)tc.nParentObjectClass, ifmp ) );

        pgnopbf.pbf->tce = (TCE)tc.nParentObjectClass;
    }

    AssertRTL( err > -65536 && err < 65536 );

    if ( fCacheMiss )
    {
        PERFOpt( cBFCacheMiss.Inc( PinstFromIfmp( pgnopbf.pbf->ifmp ), pgnopbf.pbf->tce ) );
    }
    PERFOpt( cBFSlowLatch.Inc( perfinstGlobal ) );
    if ( bflf & bflfHint )
    {
        PERFOpt( cBFBadLatchHint.Inc( perfinstGlobal ) );
        err = err < JET_errSuccess ? err : ErrERRCheck( wrnBFBadLatchHint );
    }
    PERFOpt( cBFCacheReq.Inc( PinstFromIfmp( pgnopbf.pbf->ifmp ), pgnopbf.pbf->tce ) );

    pbfl->pv        = pgnopbf.pbf->pv;
    pbfl->dwContext = DWORD_PTR( pgnopbf.pbf );


    Assert( ( err < JET_errSuccess &&
                pgnopbf.pbf->sxwl.FNotOwner() &&
                bfltHave == bfltNone ) ||
            ( err >= JET_errSuccess &&
                FBFIOwnsLatchType( pgnopbf.pbf, bfltReq ) &&
                bfltHave == bfltReq ) );

#ifdef DEBUG
    if ( bflf & bflfNoFaultFail )
    {

        if ( pgnopbf.pbf->err == JET_errDiskIO ||
                pgnopbf.pbf->err == JET_errReadVerifyFailure )
        {

            Assert( err != pgnopbf.pbf->err );
        }
    }
#endif

    AssertRTL( err > -65536 && err < 65536 );

    return  (   err != JET_errSuccess ?
                    err :
                    (   fCacheMiss ?
                            ErrERRCheck( wrnBFPageFault ) :
                            JET_errSuccess ) );
}


#ifdef NUKE_OLD_VERSIONS_EXPENSIVE_DEBUG

DWORD g_cbfFlushPurgeNukeAttempts = 0;
DWORD g_cbfFlushPurgeNukeSuccesses = 0;

BOOL FBFIFlushPurgeNukeRelease( PBF pbf, IOREASONPRIMARY iorp )
{
    BOOL fRet = fFalse;

    Assert( pbf->sxwl.FOwnWriteLatch() );

    g_cbfFlushPurgeNukeAttempts++;

    Assert( ( g_cbfFlushPurgeNukeAttempts / 16 ) <= g_cbfFlushPurgeNukeSuccesses );

    if ( pbf->bfdf != bfdfClean )
    {


        if ( ErrBFITryPrepareFlushPage( pbf, bfltWrite, IOR( iorp ), qosIODispatchUrgentBackgroundMax, fTrue ) < JET_errSuccess )
        {
            return fFalse;
        }

        TraceContext tcContext = BFIGetTraceContext( pbf );
        tcContext.iorReason.SetIorp( iorp );
        tcContext.iorReason.AddFlag( iorfForeground );
        if ( ErrBFISyncWrite( pbf, bfltWrite, qosIODispatchImmediate, &tcContext ) < JET_errSuccess )
        {
            return fFalse;
        }
    }


    BFLRUK::CLock   lockLRUK;
    g_bflruk.LockResourceForEvict( pbf, &lockLRUK );


    pbf->sxwl.ReleaseWriteLatch();

    ERR errEvict = ErrBFIEvictPage( pbf, &lockLRUK, BFEvictFlags( bfefReasonTest | bfefNukePageImage ) );
    fRet = fTrue;

    g_cbfFlushPurgeNukeSuccesses += ( JET_errSuccess == errEvict );

    g_bflruk.UnlockResourceForEvict( &lockLRUK );

    return fRet;
}

#endif

void BFIUnlatchMaintPage( __inout PBF const pbf, __in const BFLatchType bfltHave )
{
    PBF     pbfNew = NULL;

    Assert( bfltMax != bfltHave );
    Assert( pbf->sxwl.FOwner() );
    Assert( FBFIOwnsLatchType( pbf, bfltHave ) );


    Assert( FBFICurrentPage( pbf, pbf->ifmp, pbf->pgno ) );


    const BFDirtyFlags  bfdf    = BFDirtyFlags( pbf->bfdf );


    if ( bfltWrite == bfltHave )
    {

        Assert( pbf->bfdf >= bfdfUntidy );



        BFIOpportunisticallyVersionCopyPage( pbf, &pbfNew, fTrue  );

        if ( pbfNew )
        {

            Assert( FBFIOwnsLatchType( pbf, bfltHave ) );
            Assert( pbfNew->sxwl.FOwnWriteLatch() );

            pbfNew->sxwl.ReleaseWriteLatch();

#ifdef NUKE_OLD_VERSIONS_EXPENSIVE_DEBUG

            if ( bfltExclusive == bfltHave )
            {
                CSXWLatch::ERR errEUWL = pbf->sxwl.ErrUpgradeExclusiveLatchToWriteLatch();
                if ( errEUWL != CSXWLatch::errSuccess )
                {
                    Assert( errEUWL == CSXWLatch::errWaitForWriteLatch );
                    pbf->sxwl.WaitForWriteLatch();
                }
                bfltHave = bfltWrite;
            }

            if ( FBFIFlushPurgeNukeRelease( pbf, iorpBFCheckpointAdv ) )
            {
                bfltHave = bfltMax;
            }
#endif
        }

    }


    if ( bfltMax != bfltHave )
    {

        BFIReleaseSXWL( pbf, bfltHave );
    }


    if ( bfdf == bfdfFilthy )
    {

        BFIOpportunisticallyFlushPage( pbf, iorpBFFilthyFlush );
    }
    else if ( pbfNew )
    {

        BFIOpportunisticallyFlushPage( pbf, iorpBFCheckpointAdv );
    }


    Assert( pbf->sxwl.FNotOwner() );

    if ( pbfNew )
    {
        Assert( pbfNew->sxwl.FNotOwner() );
    }

}

PBF PbfBFIGetFlushOrderLeaf( const PBF pbf, const BOOL fFlagCheckpointImpeders )
{
    PBF pbfT = NULL;


    Assert( g_critBFDepend.FOwner() );

    pbfT = pbf;

    while ( pbfT->pbfTimeDepChainNext != pbfNil )
    {
        pbfT = pbfT->pbfTimeDepChainNext;
        Assert( pbfT->ifmp == pbf->ifmp );
        if ( fFlagCheckpointImpeders )
        {
            pbfT->bfbitfield.SetFImpedingCheckpoint( fTrue );
        }
    }

    Assert( pbfT->pbfTimeDepChainNext == pbfNil );

    return pbfT;
}

void BFIAssertReadyForWrite( __in const PBF pbf )
{


    Assert( FBFIUpdatablePage( pbf ) );
    Assert( pbf->bfbitfield.FRangeLocked() );
    Assert( pbf->err != errBFIPageFaultPending );
    Assert( pbf->err != wrnBFPageFlushPending );
    Assert( pbf->pWriteSignalComplete == NULL );
    Assert( PvBFIAcquireIOContext( pbf ) == NULL );


    Assert( JET_errSuccess == pbf->err );


    Assert( !pbf->bfbitfield.FDependentPurged() );

    Assert( pbf->pbfTimeDepChainNext == pbfNil );

    Assert( pbf->prceUndoInfoNext == prceNil );

    Assert( ( CmpLgpos( &pbf->lgposModify, &lgposMin ) == 0 ) ||
                ( CmpLgpos( pbf->lgposModify, g_rgfmp[ pbf->ifmp ].LgposWaypoint() ) < 0 ) );

#ifdef DEBUG
    const INST * const pinst = PinstFromIfmp( pbf->ifmp );
    LOG * const plog = pinst->m_plog;

    if ( !plog->FLogDisabled() )
    {
        const BOOL fJetInitialized = pinst->m_fJetInitialized;
        const BOOL fRecovering = plog->FRecovering();
        LGPOS lgposFlushTip, lgposWriteTip;
        plog->LGWriteAndFlushTip( &lgposWriteTip, &lgposFlushTip );
        const INT icmpFlush = CmpLgpos( pbf->lgposModify.LgposAtomicRead(), lgposFlushTip );
        const INT icmpWrite = CmpLgpos( pbf->lgposModify.LgposAtomicRead(), lgposWriteTip );
        Assert( ( icmpWrite < 0 ) || !fJetInitialized );
        Assert( icmpFlush < 0 );
        Assert( ( CmpLgpos( &lgposFlushTip, &lgposWriteTip ) <= 0 ) || fRecovering || !fJetInitialized );
    }

    const BOOL fSnapshotFlushed =  !g_rgfmp[ pbf->ifmp ].FRBSOn() || CmpRbspos( g_rgfmp[ pbf->ifmp ].PRBS()->RbsposFlushPoint(), pbf->rbsposSnapshot ) >= 0;
    Assert( fSnapshotFlushed );
#endif
}


ERR ErrBFITryPrepareFlushPage(
    _Inout_ const PBF       pbf,
    _In_ const BFLatchType  bfltHave,
    _In_       IOREASON     ior,
    _In_ const OSFILEQOS    qos,
    _In_ const BOOL         fRemoveDependencies )
{
    Assert( pbf->sxwl.FOwnWriteLatch() && bfltWrite == bfltHave );

    ERR err = JET_errSuccess;

    CBFIssueList bfil;
    ULONG cIter = 0;


    while( ( err = ErrBFIPrepareFlushPage( pbf, bfltHave, ior, qos, fRemoveDependencies ) ) < JET_errSuccess )
    {
        cIter++;


        CallS( bfil.ErrIssue( fTrue ) );

        if ( ( cIter >= 10 ) || ( err == errBFIPageAbandoned ) )
        {
            return err;
        }

        UtilSleep( dtickFastRetry );
    }
    CallS( bfil.ErrIssue( fTrue ) );

    Expected( err >= JET_errSuccess );

    return err;
}

#ifdef DEBUG
VOID BFIAssertReqRangeConsistentWithLgpos( FMP* const pfmp, const LGPOS& lgposOB0, const LGPOS& lgposModify, const CHAR* const szTag )
{
    const BOOL fIsOB0Set = CmpLgpos( lgposOB0, lgposMax ) != 0;
    const BOOL fIsLgposModifySet = CmpLgpos( lgposModify, lgposMin ) != 0;

    if ( !fIsOB0Set && !fIsLgposModifySet )
    {
        return;
    }

    LONG lGenMinRequired = 0, lGenMaxRequired = 0;
    LONG lGenMinConsistent = 0;
    LONG lGenPreRedoMinConsistent = 0, lGenPreRedoMinRequired = 0;
    ULONG ulTrimCount = 0, ulShrinkCount = 0;
    {
    PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr();
    lGenMinRequired = pdbfilehdr->le_lGenMinRequired;
    lGenMaxRequired = pdbfilehdr->le_lGenMaxRequired;
    lGenMinConsistent = pdbfilehdr->le_lGenMinConsistent;
    lGenPreRedoMinConsistent = pdbfilehdr->le_lGenPreRedoMinConsistent;
    lGenPreRedoMinRequired = pdbfilehdr->le_lGenPreRedoMinRequired;
    ulTrimCount = pdbfilehdr->le_ulTrimCount;
    ulShrinkCount=  pdbfilehdr->le_ulShrinkCount;
    }
    Assert( lGenMinRequired > 0 );
    Assert( lGenMinConsistent > 0 );
    Assert( lGenMaxRequired > 0 );
    Assert( lGenMinRequired <= lGenMinConsistent );
    Assert( lGenMinConsistent <= lGenMaxRequired );

    const BOOL fLowerMinReqLogGenOnRedo = ( pfmp->ErrDBFormatFeatureEnabled( JET_efvLowerMinReqLogGenOnRedo ) == JET_errSuccess );
    Assert( fLowerMinReqLogGenOnRedo || ( ( lGenPreRedoMinConsistent == 0 ) && ( lGenPreRedoMinRequired == 0 ) ) );
    Assert( ( lGenPreRedoMinConsistent == 0 ) ||
            ( ( lGenPreRedoMinConsistent > lGenMinConsistent ) &&
              ( lGenPreRedoMinConsistent <= lGenMaxRequired ) ) );
    Assert( ( lGenPreRedoMinRequired == 0 ) ||
            ( ( lGenPreRedoMinRequired > lGenMinRequired ) &&
              ( lGenPreRedoMinRequired <= lGenMaxRequired ) ) );

    if ( FNegTest( fDisableAssertReqRangeConsistentLgpos ) )
    {
        return;
    }

    if ( fIsOB0Set )
    {
        AssertSz( lgposOB0.lGeneration >= lGenMinConsistent, "%hsLgposOB0TooOld", szTag );
    }

    if ( fIsLgposModifySet )
    {
        AssertSz( lgposModify.lGeneration >= lGenMinConsistent, "%hsLgposModifyTooOld", szTag );
    }


    if ( !FNegTest( fCorruptingWithLostFlushWithinReqRange ) && ( ulTrimCount == 0 ) && ( ulShrinkCount == 0 ) )
    {
        if ( fIsOB0Set && ( lGenPreRedoMinConsistent != 0 ) )
        {
            AssertSz( lgposOB0.lGeneration >= lGenPreRedoMinConsistent, "%hsLgposOB0TooOldPreRedo", szTag );
        }

        if ( fIsLgposModifySet && ( lGenPreRedoMinConsistent != 0 ) )
        {
            AssertSz( lgposModify.lGeneration >= lGenPreRedoMinConsistent, "%hsLgposModifyTooOldPreRedo", szTag );
        }
    }
}
#endif

TICK g_tickLastRBSWrite = 0;

ERR ErrBFIPrepareFlushPage(
                        _In_ const PBF          pbf,
                        _In_ const BFLatchType  bfltHave,
                        _In_       IOREASON     ior,
                        _In_ const OSFILEQOS    qos,
                        _In_ const BOOL         fRemoveDependencies,
                        _Out_opt_ BOOL * const  pfPermanentErr )
{
    ERR err = JET_errSuccess;
    FMP* pfmp = NULL;
    BOOL fRangeLocked = fFalse;
    TLS * ptls = Ptls();

    Assert( FBFIUpdatablePage( pbf ) );
    Enforce( pbf->err != errBFIPageFaultPending );
    Enforce( pbf->err != wrnBFPageFlushPending );
    Enforce( pbf->pWriteSignalComplete == NULL );
    Enforce( PvBFIAcquireIOContext( pbf ) == NULL );

    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );
    Assert( FBFIOwnsLatchType( pbf, bfltHave ) );

    Assert( NULL == pfPermanentErr || *pfPermanentErr == fFalse  );


    if ( pfPermanentErr &&
            pbf->err < JET_errSuccess )
    {
        *pfPermanentErr = fTrue;
    }
    Call( pbf->err );

    Assert( NULL == pfPermanentErr || *pfPermanentErr == fFalse  );


    Expected( !pbf->fAbandoned );
    if ( pbf->fAbandoned )
    {
        Error( ErrERRCheck( errBFIPageAbandoned ) );
    }


    if ( pbf->bfbitfield.FDependentPurged() )
    {
        Call( ErrERRCheck( errBFIDependentPurged ) );
    }


    if ( FIOThread() )
    {
        AssertSz( fFalse, "We have changed the Buffer Manager so this should never happen.  Please tell SOMEONE if it does." );
        Call( ErrERRCheck( errBFIPageFlushDisallowedOnIOThread ) );
    }


    while ( pbf->pbfTimeDepChainNext != pbfNil )
    {

        g_critBFDepend.Enter();
        PBF pbfT = PbfBFIGetFlushOrderLeaf( pbf, ptls->fCheckpoint );

        AssertTrack( pbf->pgno == pbfT->pgno && pbf->ifmp == pbfT->ifmp, "InterPageDependencyDeprecated" );

        g_critBFDepend.Leave();



        CLockDeadlockDetectionInfo::DisableOwnershipTracking();
        ior.AddFlag( iorfDependantOrVersion );
        err = ErrBFIFlushPage( pbfT, ior, qos, bfdfDirty, !fRemoveDependencies );
        CLockDeadlockDetectionInfo::EnableOwnershipTracking();
        Call( err );
    }


    if ( pbf->prceUndoInfoNext != prceNil )
    {
        if ( !fRemoveDependencies )
        {
            Call( ErrERRCheck( errBFIRemainingDependencies ) );
        }

        ENTERCRITICALSECTION ecs( &g_critpoolBFDUI.Crit( pbf ) );

        while ( pbf->prceUndoInfoNext != prceNil )
        {

            LGPOS lgpos;
            err = ErrLGUndoInfoWithoutRetry( pbf->prceUndoInfoNext, &lgpos );


            if ( err >= JET_errSuccess )
            {

                BFIRemoveUndoInfo( pbf, pbf->prceUndoInfoNext, lgpos );
            }


            else
            {

                if (    err == JET_errLogWriteFail ||
                        err == JET_errLogDisabledDueToRecoveryFailure )
                {

                    Call( err );
                }


                else if ( err == JET_errCannotLogDuringRecoveryRedo )
                {

                    Call( ErrERRCheck( errBFLatchConflict ) );
                }


                else
                {
                    Assert( err == errLGNotSynchronous );


                    Call( ptls->pbfil->ErrPrepareLogWrite( pbf->ifmp ) );


                    Call( ErrERRCheck( errBFIRemainingDependencies ) );
                }
            }
        }
    }


    LOG * const plog = PinstFromIfmp( pbf->ifmp )->m_plog;

    LGPOS lgposWaypoint;
    lgposWaypoint = g_rgfmp[ pbf->ifmp ].LgposWaypoint();

    if ( ( CmpLgpos( &pbf->lgposModify, &lgposMin ) != 0 ) &&
         ( CmpLgpos( &pbf->lgposModify, &lgposWaypoint ) >= 0 ) )
    {


        if ( plog->FNoMoreLogWrite() )
        {

            Call( ErrERRCheck( JET_errLogWriteFail ) );
        }


        LGPOS lgposTip;
        plog->LGLogTipWithLock( &lgposTip );
        if ( plog->LGGetCurrentFileGenWithLock() < lgposTip.lGeneration &&
                !plog->FLGProbablyWriting() )
        {
            Expected( lgposWaypoint.lGeneration < lgposTip.lGeneration );
            Call( ptls->pbfil->ErrPrepareLogWrite( pbf->ifmp ) );
            Call( ErrERRCheck( errBFIRemainingDependencies ) );
        }


        BFIMaintWaypointRequest( pbf->ifmp );


        if ( UlParam( PinstFromIfmp( pbf->ifmp ), JET_paramWaypointLatency ) > 0 )
        {
            BFIMaintIdleDatabaseRequest( pbf );
        }

        Call( ErrERRCheck( errBFIPageTouchTooRecent ) );
    }


    LGPOS lgposWriteTip, lgposFlushTip;
    plog->LGWriteAndFlushTip( &lgposWriteTip, &lgposFlushTip );
    const INT icmpWrite = CmpLgpos( &pbf->lgposModify, &lgposWriteTip );
    const INT icmpFlush = CmpLgpos( &pbf->lgposModify, &lgposFlushTip );
    const BOOL fSnapshotFlushed =  !g_rgfmp[ pbf->ifmp ].FRBSOn() || CmpRbspos( g_rgfmp[ pbf->ifmp ].PRBS()->RbsposFlushPoint(), pbf->rbsposSnapshot ) >= 0;

    if ( !plog->FLogDisabled() &&
         icmpWrite >= 0 )
    {
        if ( plog->FRecoveringMode() == fRecoveringRedo || !fRemoveDependencies )
        {
            Call( ErrERRCheck( errBFIRemainingDependencies ) );
        }


        if ( plog->FNoMoreLogWrite() )
        {

            Call( ErrERRCheck( JET_errLogWriteFail ) );
        }


        else
        {

            Call( ptls->pbfil->ErrPrepareLogWrite( pbf->ifmp ) );


            Call( ErrERRCheck( errBFIRemainingDependencies ) );
        }
    }
    else if ( !plog->FLogDisabled() &&
              icmpFlush >= 0 )
    {

        if ( plog->FNoMoreLogWrite() )
        {

            Call( ErrERRCheck( JET_errLogWriteFail ) );
        }


        Call( ErrERRCheck( errBFIPageTouchTooRecent ) );
    }
    else if ( !fSnapshotFlushed )
    {
        Assert( g_rgfmp[ pbf->ifmp ].FRBSOn() );

        if ( fRemoveDependencies && g_rgfmp[ pbf->ifmp ].PRBS()->FTooLongSinceLastFlush() )
        {
            Call( ptls->pbfil->ErrPrepareRBSWrite( pbf->ifmp ) );

            Call( ErrERRCheck( errBFIRemainingDependencies ) );
        }

        Call( ErrERRCheck( errBFIPageTouchTooRecent ) );
    }



    AssertTrack( !pbf->bfbitfield.FRangeLocked(), "BFPrepRangeAlreadyLocked" );

    CMeteredSection::Group irangelock = CMeteredSection::groupTooManyActiveErr;
    pfmp = &g_rgfmp[ pbf->ifmp ];
    fRangeLocked = pfmp->FEnterRangeLock( pbf->pgno, &irangelock );

    if ( fRangeLocked )
    {

        pbf->bfbitfield.SetFRangeLocked( fTrue );
        Assert( irangelock == 0 || irangelock == 1 );
        pbf->irangelock = BYTE( irangelock );
        Assert( pbf->irangelock == irangelock );
    }
    else
    {
        if ( irangelock == CMeteredSection::groupTooManyActiveErr )
        {

            Error( ErrERRCheck( errDiskTilt ) );
        }
        else
        {

            Error( ErrERRCheck( errBFIRemainingDependencies ) );
        }
    }

    Assert( fRangeLocked );
    Assert( pbf->bfbitfield.FRangeLocked() );
    Assert( irangelock != CMeteredSection::groupTooManyActiveErr );
    Assert( pbf->irangelock != CMeteredSection::groupTooManyActiveErr );


    if ( pbf->icbBuffer != pbf->icbPage )
    {

        Assert( bfdfClean == pbf->bfdf || bfdfUntidy == pbf->bfdf );

        if ( bfltExclusive  == bfltHave )
        {
            const CSXWLatch::ERR errTryW = pbf->sxwl.ErrTryUpgradeExclusiveLatchToWriteLatch();

            if ( CSXWLatch::ERR::errLatchConflict == errTryW )
            {

                Call( ErrERRCheck( errBFLatchConflict ) );
            }
            
            Assert( CSXWLatch::ERR::errSuccess == errTryW );
        }

        BFIRehydratePage( pbf );

        Assert( pbf->icbBuffer == pbf->icbPage );

        if ( bfltExclusive  == bfltHave )
        {
            pbf->sxwl.DowngradeWriteLatchToExclusiveLatch();
        }

    }


    if ( FBFIDatabasePage( pbf ) )
    {
        CFlushMap* const pfm = pfmp->PFlushMap();
        const BOOL fIsDirty = ( pbf->bfdf >= bfdfDirty );
        const BOOL fIsPagePatching = ( ior.Iorp() == IOREASONPRIMARY( iorpPatchFix ) );
        const BOOL fIsFmRecoverable = pfm->FRecoverable();
        const BOOL fIsOB0Set = CmpLgpos( &pbf->lgposOldestBegin0, &lgposMax ) != 0;
        Assert( CmpLgpos( &pbf->lgposOldestBegin0, &lgposMin ) != 0 );
        const BOOL fIsLgposModifySet = CmpLgpos( &pbf->lgposModify, &lgposMin ) != 0;
        Assert( CmpLgpos( &pbf->lgposModify, &lgposMax ) != 0 );
        const CPAGE::PageFlushType pgftMapCurrent = pfm->PgftGetPgnoFlushType( pbf->pgno );
        CPAGE::PageFlushType pgftPageCurrent;
        CPAGE::PageFlushType pgftPageNext = pgftMapCurrent;

        AssertSz( !( pbf->fOlderVersion || !pbf->fCurrentVersion ) || fIsDirty, "Must not write non-dirty older versions." );

        CPAGE cpage;
        Assert( CbBFIBufferSize( pbf ) == CbBFIPageSize( pbf ) );
        cpage.LoadPage( pbf->ifmp, pbf->pgno, pbf->pv, CbBFIBufferSize( pbf ) );
        if ( !FBFIBufferIsZeroed( pbf ) && ( cpage.Dbtime() != dbtimeShrunk ) && ( cpage.Dbtime() != dbtimeRevert ) )
        {
            pgftPageCurrent = cpage.Pgft();

            if ( pbf->bfdf > bfdfClean )
            {

                if ( fIsPagePatching )
                {
                    pgftPageNext = CPAGE::pgftUnknown;
                }
                else if ( ( pgftMapCurrent == CPAGE::pgftUnknown ) && ( pgftPageCurrent == CPAGE::pgftUnknown ) )
                {
                    pgftPageNext = CPAGE::PgftGetNextFlushType( CPAGE::pgftUnknown );
                }
                else
                {
                    const BOOL fChangeFlushType = !fIsFmRecoverable || ( fIsOB0Set && fIsLgposModifySet && fIsDirty );
                    const CPAGE::PageFlushType pgftPageNextSeed = ( pgftMapCurrent != CPAGE::pgftUnknown ) ? pgftMapCurrent : pgftPageCurrent;
                    Assert( pgftPageNextSeed != CPAGE::pgftUnknown );

                    if ( fChangeFlushType )
                    {
                        pgftPageNext = CPAGE::PgftGetNextFlushType( pgftPageNextSeed );
                    }
                    else
                    {
                        pgftPageNext = pgftPageNextSeed;
                    }
                }

                Assert( ( pgftPageNext != CPAGE::pgftUnknown ) || fIsPagePatching );


                cpage.PreparePageForWrite( pgftPageNext );
            }


            BFIValidatePagePgno( pbf );
        }
        else
        {
            pgftPageCurrent = CPAGE::pgftUnknown;
            pgftPageNext = CPAGE::pgftUnknown;
        }
        cpage.UnloadPage();

        if ( !fIsDirty && fIsFmRecoverable &&
            ( pgftMapCurrent != CPAGE::pgftUnknown ) &&
            ( pgftPageCurrent != CPAGE::pgftUnknown ) )
        {
            AssertSz( ( pgftMapCurrent == pgftPageCurrent ) && ( pgftPageCurrent == pgftPageNext ), "Non-dirty page must match in-memory flush state." );
        }

        OnDebug( BFIAssertReqRangeConsistentWithLgpos( pfmp, pbf->lgposOldestBegin0, pbf->lgposModify, "PrepareFlushPage" ) );


        if ( fIsPagePatching && fIsFmRecoverable )
        {
            if ( pfm->ErrSetPgnoFlushTypeAndWait( pbf->pgno, CPAGE::pgftUnknown, dbtimeNil ) < JET_errSuccess )
            {
                Error( ErrERRCheck( errBFIReqSyncFlushMapWriteFailed ) );
            }
        }
    }


    if ( ( err = ptls->pbfil->ErrPrepareWrite( pbf->ifmp ) ) < JET_errSuccess )
    {
        Call( err );
    }

    BFIAssertReadyForWrite( pbf );

HandleError:

    Assert( FBFIOwnsLatchType( pbf, bfltHave ) );

    if ( ( err < JET_errSuccess ) && fRangeLocked )
    {
        AssertTrack( pbf->bfbitfield.FRangeLocked(), "BFPrepFlushRangeNotLocked" );
        pfmp->LeaveRangeLock( pbf->pgno, pbf->irangelock );
        pbf->bfbitfield.SetFRangeLocked( fFalse );
        fRangeLocked = fFalse;
    }


    if (    err < JET_errSuccess &&
            err != JET_errOutOfMemory &&
            err != errDiskTilt &&
            err != errBFIRemainingDependencies &&
            err != errBFIPageTouchTooRecent  &&
            err != errBFIPageFlushed &&
            err != errBFIPageFlushPending &&
            err != errBFIPageFlushPendingSlowIO &&
            err != errBFIPageFlushPendingHungIO &&
            err != errBFLatchConflict &&
            err != errBFIPageFlushDisallowedOnIOThread &&
            err != errBFIReqSyncFlushMapWriteFailed )
    {

        pbf->err = SHORT( err );
        Assert( pbf->err == err );
        Assert( pbf->err != JET_errFileIOBeyondEOF );
    }


    return err;
}

BOOL FBFITryAcquireExclusiveLatchForMaint( const PBF pbf )
{
    CLockDeadlockDetectionInfo::DisableOwnershipTracking();
    const CSXWLatch::ERR errSXWL = pbf->sxwl.ErrTryAcquireExclusiveLatch();
    CLockDeadlockDetectionInfo::EnableOwnershipTracking();

    if ( errSXWL != CSXWLatch::ERR::errSuccess )
    {
        Assert( errSXWL == CSXWLatch::ERR::errLatchConflict );
        return fFalse;
    }
    pbf->sxwl.ClaimOwnership( bfltExclusive );
    return fTrue;
}


ERR ErrBFIAcquireExclusiveLatchForFlush( PBF pbf, __in const BOOL fUnencumberedPath )
{
    ERR err = JET_errSuccess;


    CSXWLatch::ERR errSXWL = pbf->sxwl.ErrTryAcquireExclusiveLatch();

    if ( errSXWL == CSXWLatch::ERR::errSuccess )
    {
        if ( pbf->fAbandoned )
        {
            pbf->sxwl.ReleaseExclusiveLatch();
            Error( ErrERRCheck( errBFIPageAbandoned ) );
        }

        if ( pbf->err == wrnBFPageFlushPending )
        {

            if ( !FBFICompleteFlushPage( pbf, bfltExclusive, fUnencumberedPath ) )
            {
                err = ErrBFIFlushPendingStatus( pbf );
                Assert( err < JET_errSuccess );
                

                pbf->sxwl.ReleaseExclusiveLatch();

                Error( err );
            }


            Assert( pbf->err < JET_errSuccess || pbf->bfdf == bfdfClean );
            Assert( FBFIUpdatablePage( pbf ) );

        }


        if ( pbf->pbfTimeDepChainNext )
        {

            (void)ErrBFIEvictRemoveCleanVersions( pbf );

        }

        return JET_errSuccess;
    }
    else
    {

        Call( ErrERRCheck( errBFLatchConflict ) );
    }

HandleError:

    Assert( JET_errSuccess == err || pbf->sxwl.FNotOwner() );
    Assert( JET_errSuccess != err || pbf->sxwl.FOwnExclusiveLatch() );
    
    return err;
}

class CBFOpportuneWriter
{
public:
    CBFOpportuneWriter( ULONG cbfMax, ULONG cbfMaxCleanRun, IOREASON iorBase, OSFILEQOS qos ) :
        m_cbfMax( cbfMax ), m_cbfMaxCleanRun( cbfMaxCleanRun ), m_iorBase( iorBase ), m_qos( qos ), m_cbfUseful( 0 ), m_cbfLatched( 0 )
    {
        Assert( m_iorBase.Iorp() != iorpNone );
        memset( m_rgpbf, 0, sizeof( m_rgpbf[ 0 ] ) * m_cbfMax );
    }
    ~CBFOpportuneWriter();

    void PerformOpportuneWrites( const IFMP ifmp, const PGNO pgno );

private:
    void AssertValid_() const;

    bool FFull_() const { return m_cbfMax <= m_cbfLatched; }
    ULONG CBFLatched() const { return m_cbfLatched; }
    ERR ErrCanAddBF_( const PBF pbf );
    ERR ErrAddBF_( const PBF pbf );
    void RevertBFs_( ULONG ibfStart = 0);
    void FlushUsefulBFs_();
    bool FVerifyCleanBFs_();

    ERR ErrPrepareBFForOpportuneWrite_( const PBF pbf );
    void GetFlushableNeighboringBFs_( const IFMP ifmp, const PGNO pgno, const INT iDelta );

    const IOREASON m_iorBase;
    const OSFILEQOS m_qos;

    const ULONG m_cbfMax;
    const ULONG m_cbfMaxCleanRun;

    ULONG m_cbfUseful;
    ULONG m_cbfLatched;

    PBF m_rgpbf[];
};

void CBFOpportuneWriter::AssertValid_() const
{
    Assert( m_cbfMaxCleanRun <= m_cbfMax );
    Assert( m_cbfUseful <= m_cbfLatched );
    Assert( m_cbfLatched <= m_cbfMax );
    Assert( qosIOOptimizeCombinable & m_qos );
}

ERR CBFOpportuneWriter::ErrCanAddBF_( const PBF pbf )
{
    ERR err = JET_errSuccess;
    AssertValid_();
    Assert( pbf->sxwl.FOwnExclusiveLatch() );

    if ( FFull_() )
    {
        Call( ErrERRCheck( errBFIOutOfBatchIOBuffers ) );
    }

    if ( bfdfClean == pbf->bfdf && m_cbfUseful + m_cbfMaxCleanRun <= m_cbfLatched )
    {
        Call( ErrERRCheck( errBFIOutOfBatchIOBuffers ) );
    }

HandleError:
    AssertValid_();
    return err;
}

ERR CBFOpportuneWriter::ErrAddBF_( const PBF pbf )
{
    ERR err = JET_errSuccess;
    AssertValid_();
    Assert( pbf->sxwl.FOwnExclusiveLatch() );

    Call( ErrCanAddBF_( pbf ) );

    if ( ( m_cbfLatched + 1 < m_cbfMax ) && m_rgpbf[ m_cbfLatched + 1 ] )
    {
        Assert( m_rgpbf[ m_cbfLatched + 1 ]->sxwl.FNotOwner() );
    }


    m_rgpbf[ m_cbfLatched++ ] = pbf;
    if ( bfdfClean != pbf->bfdf )
    {
        m_cbfUseful = m_cbfLatched;
    }

HandleError:
    AssertValid_();
    return err;
}

void CBFOpportuneWriter::RevertBFs_( ULONG ibfStart )
{
    for ( ULONG ibf = ibfStart; ibf < m_cbfLatched; ibf++ )
    {
        PBF pbf = m_rgpbf[ ibf ];
        if ( pbf )
        {
            Assert( pbf->sxwl.FOwnExclusiveLatch() );

            AssertTrack( pbf->bfbitfield.FRangeLocked(), "BFRevertRangeNotLocked" );
            g_rgfmp[ pbf->ifmp ].LeaveRangeLock( pbf->pgno, pbf->irangelock );
            pbf->bfbitfield.SetFRangeLocked( fFalse );
            pbf->sxwl.ReleaseExclusiveLatch();
        }
    }

    m_cbfLatched = min( m_cbfLatched, ibfStart );
    m_cbfUseful = min( m_cbfUseful, ibfStart );
    AssertValid_();
}

CBFOpportuneWriter::~CBFOpportuneWriter()
{
    AssertValid_();

    for ( ULONG ibf = 0; ibf < m_cbfMax; ibf++ )
    {
        PBF pbf = m_rgpbf[ ibf ];
        if ( pbf )
        {
            Assert( pbf->sxwl.FNotOwnExclusiveLatch() );
        }
    }
}

void CBFOpportuneWriter::FlushUsefulBFs_()
{
    ERR errLastFlush = errCodeInconsistency;
    ULONG cbfUseful = m_cbfUseful;

    if ( cbfUseful )
    {
        ULONG ibf;
        for ( ibf = 0; ibf < cbfUseful; ++ibf )
        {
            Assert( m_qos & qosIOOptimizeCombinable );

            const OSFILEQOS qosT = m_qos | ( ( m_rgpbf[ ibf ]->bfdf == bfdfClean ) ? qosIOOptimizeOverwriteTracing : 0 );

            errLastFlush = ErrBFIFlushExclusiveLatchedAndPreparedBF( m_rgpbf[ ibf ], m_iorBase, qosT, fTrue );

            Assert( errBFIPageFlushed == errLastFlush || errDiskTilt == errLastFlush );
            if( errDiskTilt == errLastFlush )
            {
                Assert( m_rgpbf[ ibf ]->sxwl.FOwnExclusiveLatch() );
                break;
            }
            Assert( errBFIPageFlushed == errLastFlush );
        }

        if ( ibf < cbfUseful )
        {
            Assert( errDiskTilt == errLastFlush );
            RevertBFs_( ibf );
        }
        if ( ibf )
        {
            AtomicIncrement( ( LONG* )&cBFOpportuneWriteIssued );
        }
    }
}

bool CBFOpportuneWriter::FVerifyCleanBFs_()
{
    for ( ULONG ibf = 0; ibf < m_cbfUseful; ++ibf )
    {
        const PBF pbf = m_rgpbf[ ibf ];
        Assert( pbf->sxwl.FOwnExclusiveLatch() );

        ERR errPreVerify = pbf->err;

        if ( bfdfClean == pbf->bfdf )
        {

            if ( !FOSMemoryPageResident( pbf->pv, CbBFIPageSize( pbf ) ) )
            {
                return false;
            }

            if ( pbf->bfat == bfatViewMapped && !FBFICacheViewFresh( pbf ) )
            {
                if ( ErrBFICacheViewFreshen( pbf, 0, *TraceContextScope( m_iorBase.Iorp() ) ) < JET_errSuccess )
                {

                    return false;
                }
            }

            const ERR errReVerify = ErrBFIVerifyPage( pbf, CPageValidationLogEvent::LOG_NONE, fFalse );
            if ( errPreVerify >= JET_errSuccess && errReVerify == JET_errPageNotInitialized )
            {
                return false;
            }
            if ( errReVerify != JET_errSuccess )
            {
                if ( errPreVerify >= JET_errSuccess )
                {
                    UtilReportEvent( eventError,
                            BUFFER_MANAGER_CATEGORY,
                            TRANSIENT_IN_MEMORY_CORRUPTION_DETECTED_ID,
                            0, NULL );
                    EnforceSz( fFalse, "TransientMemoryCorruption" );
                }
                else
                {
                    AssertSz( fFalse, "Unexpected error here, this should have been clean by now as ErrBFIPrepareFlushPage() rejects BFs with errors." );
                }

                AssertTrack( pbf->bfbitfield.FRangeLocked(), "BFVerCleanRangeNotLocked" );
                g_rgfmp[ pbf->ifmp ].LeaveRangeLock( pbf->pgno, pbf->irangelock );
                pbf->bfbitfield.SetFRangeLocked( fFalse );

                Assert( FBFIUpdatablePage( pbf ) );

                pbf->sxwl.ReleaseExclusiveLatch();

                m_rgpbf[ ibf ] = NULL;

                return false;
            }
        }
    }

    return true;
}

ERR CBFOpportuneWriter::ErrPrepareBFForOpportuneWrite_( const PBF pbf  )
{
    Assert( pbf->sxwl.FOwnExclusiveLatch() );

    ERR err = JET_errSuccess;

    Call( ErrCanAddBF_( pbf ) );

    Call( ErrBFIPrepareFlushPage( pbf, bfltExclusive, m_iorBase, m_qos, fFalse ) );

    const ERR errAddBF = ErrAddBF_( pbf );

    AssertTrack( err == JET_errSuccess, OSFormat( "BFPrepOpWriteErr:%d", err ) );
    AssertTrack( errAddBF == JET_errSuccess, OSFormat( "BFPrepOpWriteErrAddBf:%d", errAddBF ) );

    return err;

HandleError:
    return err;
}

void CBFOpportuneWriter::GetFlushableNeighboringBFs_( const IFMP ifmp, const PGNO pgno, const INT iDelta )
{
    Assert( 1 == iDelta || -1 == iDelta );
    PBF pbf = NULL;
    BOOL fMustReleaseLatch = fFalse;

    PGNO pgnoOpp = pgno + iDelta;
    while ( fTrue )
    {

        BFHash::CLock lock;
        g_bfhash.ReadLockKey( IFMPPGNO( ifmp, pgnoOpp ), &lock );
        PGNOPBF pgnopbf;
        BFHash::ERR errHash = g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf );
        g_bfhash.ReadUnlockKey( &lock );

        if ( errHash != BFHash::ERR::errSuccess )
        {
            OSTrace( JET_tracetagBufferManager, OSFormat( "Failed opportune write due to cache miss on page=[0x%x:0x%x]", (ULONG)ifmp, pgnoOpp ) );
            break;
        }

        pbf = pgnopbf.pbf;


        if ( ErrBFIAcquireExclusiveLatchForFlush( pbf, fFalse ) < JET_errSuccess )
        {
            OSTrace( JET_tracetagBufferManager, OSFormat( "Failed opportune write due to un-latcheable page=[0x%x:0x%x]", (ULONG)ifmp, pgnoOpp ) );
            break;
        }

        Assert( pbf->sxwl.FOwnExclusiveLatch() );
        fMustReleaseLatch = fTrue;


        if ( ( pbf->ifmp != ifmp ) || ( pbf->pgno != pgnoOpp ) ||
            ( ( pbf->bfdf < bfdfDirty ) && ( !pbf->fCurrentVersion || pbf->fOlderVersion ) ) )
        {
            OSTrace( JET_tracetagBufferManager, OSFormat( "Failed opportune write due to current buffer state page=[0x%x:0x%x]", (ULONG)ifmp, pgnoOpp ) );
            break;
        }


        if ( JET_errSuccess != ErrPrepareBFForOpportuneWrite_( pbf ) )
        {
            OSTrace( JET_tracetagBufferManager, OSFormat( "Failed opportune write due to un-preparable page=[0x%x:0x%x]", (ULONG)ifmp, pgnoOpp ) );
            break;
        }

        OSTrace( JET_tracetagBufferManager, OSFormat( "Opportune write on page=[0x%x:0x%x]", (ULONG)ifmp, pgnoOpp ) );

        Assert( pbf->sxwl.FOwnExclusiveLatch() );
        fMustReleaseLatch = fFalse;
        pgnoOpp += iDelta;
    }

    if ( fMustReleaseLatch )
    {
        Assert( pbf != NULL );
        Assert( pbf->sxwl.FOwnExclusiveLatch() );
        pbf->sxwl.ReleaseExclusiveLatch();
    }

    RevertBFs_( m_cbfUseful );
}

void CBFOpportuneWriter::PerformOpportuneWrites( const IFMP ifmp, const PGNO pgno )
{
    CLockDeadlockDetectionInfo::DisableOwnershipTracking();

    GetFlushableNeighboringBFs_( ifmp, pgno, 1 );
    GetFlushableNeighboringBFs_( ifmp, pgno, -1 );


    if ( FVerifyCleanBFs_() )
    {
        FlushUsefulBFs_();
    }
    else
    {
        RevertBFs_();
    }

    CLockDeadlockDetectionInfo::EnableOwnershipTracking();
}

void BFIPerformOpportuneWrites( const IFMP ifmp, const PGNO pgno, IOREASON iorBase, OSFILEQOS qos )
{
    const LONG cbPage = g_rgfmp[ifmp].CbPage();
    const LONG cbfMax = ( LONG )UlParam( JET_paramMaxCoalesceWriteSize ) / cbPage - 1;
    if ( 0 < cbfMax )
    {
        const LONG cbfMaxCleanRunRaw = ( LONG )UlParam( JET_paramMaxCoalesceWriteGapSize ) / cbPage;
        const LONG cbfMaxCleanRun = max( 0, min( cbfMax - 2, cbfMaxCleanRunRaw ) );

        void* pv = _alloca( sizeof( CBFOpportuneWriter ) + sizeof( PBF ) * cbfMax );

        if ( pv )
        {
            CBFOpportuneWriter* pPool = new ( pv ) CBFOpportuneWriter( cbfMax, cbfMaxCleanRun, iorBase, qos );
            pPool->PerformOpportuneWrites( ifmp, pgno );

            pPool->~CBFOpportuneWriter();
        }
    }
}

ERR ErrBFIFlushExclusiveLatchedAndPreparedBF(   __inout const PBF       pbf,
                                    __in const IOREASON     iorBase,
                                    __in const OSFILEQOS    qos,
                                    __in const BOOL         fOpportune )
{
    ERR err = JET_errSuccess;

    Assert( pbf->sxwl.FOwnExclusiveLatch() );


    TraceContextScope tcFlush( iorBase.Iorp() );
    tcFlush->iorReason.AddFlag( iorBase.Iorf() );

    if ( pbf->fSuspiciouslySlowRead )
    {
    }
    if ( fOpportune )
    {
        tcFlush->iorReason.AddFlag( iorfOpportune );
    }

    if ( FBFIDatabasePage( pbf ) )
    {
        tcFlush->SetDwEngineObjid( ( (CPAGE::PGHDR *)( pbf->pv ) )->objidFDP );
    }

    err = ErrBFIAsyncWrite( pbf, qos, *tcFlush );

    if ( err < JET_errSuccess )
    {

        Assert( errDiskTilt == err );

        Assert( pbf->sxwl.FOwnExclusiveLatch() );
    }
    else
    {

        err = ErrERRCheck( errBFIPageFlushed );

        Assert( pbf->sxwl.FNotOwnExclusiveLatch() );
    }

    return err;
}

ERR ErrBFIFlushPage(    __inout const PBF       pbf,
                        __in const IOREASON     iorBase,
                        __in const OSFILEQOS    qos,
                        __in const BFDirtyFlags bfdfFlushMin,
                        __in const BOOL         fOpportune,
                        __out_opt BOOL * const  pfPermanentErr )
{
    ERR err = JET_errSuccess;

    if ( pfPermanentErr )
    {
        *pfPermanentErr = fFalse;
    }

    const BOOL fUnencumberedPath = ( ( 0 == ( iorBase.Iorf() & iorfForeground ) ) &&
                                     ( iorBase.Iorp() == iorpBFCheckpointAdv || iorBase.Iorp() == iorpBFAvailPool || iorBase.Iorp() == iorpBFShrink ) );

    Call( ErrBFIAcquireExclusiveLatchForFlush( pbf, fUnencumberedPath ) );

    Assert( !pbf->fAbandoned );

    if ( pbf->bfdf < max( bfdfFlushMin, bfdfUntidy ) )
    {
        pbf->sxwl.ReleaseExclusiveLatch();
    }
    else
    {
        Expected( iorBase.Iorp() != iorpBFImpedingWriteCleanDoubleIo || 
                     ( pbf->bfdf >= bfdfUntidy && pbf->err < JET_errSuccess ) ); 


        if( ( err = ErrBFIPrepareFlushPage( pbf, bfltExclusive, iorBase, qos, !fOpportune, pfPermanentErr ) ) < JET_errSuccess )
        {
            pbf->sxwl.ReleaseExclusiveLatch();
            Call( err );
        }

        Enforce( pbf->pbfTimeDepChainNext == NULL );
#ifdef DEBUG
        const INT ibfPage = pbf->icbPage;
#endif

        Assert( ( ( qos & qosIOOptimizeCombinable ) == 0 ) || ( iorBase.Iorf() & iorfDependantOrVersion ) );

        err = ErrBFIFlushExclusiveLatchedAndPreparedBF( pbf, iorBase, qos, fOpportune );

        if ( errBFIPageFlushed == err )
        {
            if ( !fOpportune )
            {
                Assert( g_rgfmp[pbf->ifmp].CbPage() == g_rgcbPageSize[ibfPage] );
                BFIPerformOpportuneWrites( pbf->ifmp, pbf->pgno, iorBase, OSFILEQOS( qos | qosIOOptimizeCombinable ) );
            }
        }
        else if ( errDiskTilt == err )
        {
            AssertTrack( pbf->bfbitfield.FRangeLocked(), "BFFlushRangeNotLocked" );
            g_rgfmp[ pbf->ifmp ].LeaveRangeLock( pbf->pgno, pbf->irangelock );
            pbf->bfbitfield.SetFRangeLocked( fFalse );
            pbf->sxwl.ReleaseExclusiveLatch();
            Call( err );
        }
        else
        {
            EnforceSz( fFalse, OSFormat( "UnexpectedFlushFailure:%d", err ) );
        }

    }

HandleError:

    Assert( pbf->sxwl.FNotOwner() );

    return err;
}

bool FBFICompleteFlushPage( _Inout_ PBF pbf, _In_ const BFLatchType bflt, _In_ const BOOL fUnencumberedPath, _In_ const BOOL fCompleteRemapReVerify, _In_ const BOOL fAllowTearDownClean )
{
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );

    if ( wrnBFPageFlushPending != pbf->err )
    {
        return fFalse;
    }

    if ( ErrBFIWriteSignalState( pbf ) != wrnBFPageFlushPending )
    {
        Assert( wrnBFPageFlushPending == pbf->err );


        BFIFlushComplete( pbf, bflt, fUnencumberedPath, fCompleteRemapReVerify, fAllowTearDownClean );
        Assert( wrnBFPageFlushPending != pbf->err );
        Assert( FBFIUpdatablePage( pbf ) );
        Assert( errBFIPageRemapNotReVerified != pbf->err || !fCompleteRemapReVerify );

    }

    return wrnBFPageFlushPending != pbf->err;
}

INLINE BOOL FBFIIsCleanEnoughForEvict( const PBF pbf )
{
    return ( pbf->bfdf < bfdfDirty );
}

void BFIFlagDependenciesImpeding( PBF pbf )
{
    g_critBFDepend.Enter();


    pbf->bfbitfield.SetFImpedingCheckpoint( fTrue );

    (void)PbfBFIGetFlushOrderLeaf( pbf, fTrue );

    g_critBFDepend.Leave();
}




ERR ErrBFIEvictRemoveCleanVersions( PBF pbf )
{
    ERR err = JET_errSuccess;

    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );

    g_critBFDepend.Enter();

    PBF pbfLast = pbfNil;

    while ( ( pbfLast = PbfBFIGetFlushOrderLeaf( pbf, fFalse ) ) &&
            pbfLast != pbf &&
            FBFIIsCleanEnoughForEvict( pbfLast ) )
    {
        Assert( pbfLast->pbfTimeDepChainNext == NULL );


        CSXWLatch::ERR errSXWL = pbfLast->sxwl.ErrTryAcquireExclusiveLatch();

        if ( CSXWLatch::ERR::errSuccess == errSXWL )
        {

            if ( !FBFIUpdatablePage( pbfLast ) )
            {

                pbfLast->sxwl.ReleaseExclusiveLatch();

                Error( ErrERRCheck( errBFIRemainingDependencies ) );
            }

#ifndef RTM
            const PBF pbfPrev = pbfLast->pbfTimeDepChainPrev;
            AssertRTL( pbfPrev && pbfPrev->pbfTimeDepChainNext );
#endif

            BFICleanVersion( pbfLast, fFalse );


            AssertRTL( pbfLast->pbfTimeDepChainPrev == NULL );
            AssertRTL( pbfPrev && pbfPrev->pbfTimeDepChainNext == NULL );


            pbfLast->sxwl.ReleaseExclusiveLatch();
        }
        else
        {

            Assert( CSXWLatch::ERR::errLatchConflict == errSXWL );

            Error( ErrERRCheck( errBFLatchConflict ) );
        }


        g_critBFDepend.Leave();
        g_critBFDepend.Enter();
    }

    if ( pbfLast != pbf || pbf->pbfTimeDepChainNext )
    {

        Assert( !FBFIIsCleanEnoughForEvict( pbf ) );

        Error( ErrERRCheck( errBFIRemainingDependencies ) );
    }

HandleError:

    g_critBFDepend.Leave();

    return err;
}


ERR ErrBFIEvictPage( PBF pbf, BFLRUK::CLock* plockLRUK, const BFEvictFlags bfefDangerousOptions )
{
    ERR err = JET_errSuccess;

    BOOL fOwnsHash = fFalse;
    BOOL fOwnsLatch = fFalse;
    BFEvictFlags bfefTraceOnly = bfefNone;
    const BOOL fEvictDirty = ( 0 != ( bfefDangerousOptions & bfefEvictDirty ) );
    const BOOL fAllowTearDownClean = ( 0 != ( bfefDangerousOptions & bfefAllowTearDownClean ) );

    Assert( bfefDangerousOptions & bfefReasonMask );
    Expected( ( bfefDangerousOptions & bfefTraceMask ) == 0 );

    const INT iEvictReason = ( bfefDangerousOptions & bfefReasonMask );
    Assert( iEvictReason );
    Assert( iEvictReason >= bfefReasonMin && iEvictReason < bfefReasonMax );



    BFHash::CLock lockHash;
    g_bfhash.WriteLockKey( IFMPPGNO( pbf->ifmp, pbf->pgno ), &lockHash );
    fOwnsHash = fTrue;


    if ( pbf->sxwl.ErrTryAcquireWriteLatch() != CSXWLatch::ERR::errSuccess )
    {
        

        if ( pbf->err == errBFIPageFaultPending )
        {
            Error( FBFIIsIOHung( pbf ) ? ErrERRCheck( errBFIPageFaultPendingHungIO ) : ErrERRCheck( errBFIPageFaultPending ) );
        }
        else
        {
            Error( ErrERRCheck( errBFLatchConflict ) );
        }
    }

    Assert( pbf->sxwl.FOwnWriteLatch() );
    fOwnsLatch = fTrue;
    AssertTrack( pbf->err != JET_errFileIOBeyondEOF, "BFIEvictPageEof" );


    OnDebug( INT cCompleteFlushAttempts = 0 );
    while ( !FBFIUpdatablePage( pbf ) )
    {
        OnDebug( cCompleteFlushAttempts++ );
        Expected( cCompleteFlushAttempts <= 10 );
        

        Assert( !fEvictDirty );


        g_bfhash.WriteUnlockKey( &lockHash );
        fOwnsHash = fFalse;


        if ( FBFICompleteFlushPage( pbf, bfltWrite, fFalse, fTrue, fAllowTearDownClean ) )
        {
            Assert( FBFIUpdatablePage( pbf ) );

            bfefTraceOnly |= bfefTraceFlushComplete;

            pbf->sxwl.ReleaseWriteLatch();
            fOwnsLatch = fFalse;

            g_bfhash.WriteLockKey( IFMPPGNO( pbf->ifmp, pbf->pgno ), &lockHash );
            fOwnsHash = fTrue;

            if ( pbf->sxwl.ErrTryAcquireWriteLatch() != CSXWLatch::ERR::errSuccess )
            {
                Error( ErrERRCheck( errBFLatchConflict ) );
            }

            fOwnsLatch = fTrue;
        }
        else
        {

            Error( ErrBFIFlushPendingStatus( pbf ) );
        }
    }


    if ( pbf->pbfTimeDepChainNext != NULL )
    {

        (void)ErrBFIEvictRemoveCleanVersions( pbf );

    }


    if ( !FBFILatchDemote( pbf ) )
    {
        Error( ErrERRCheck( errBFLatchConflict ) );
    }


    if ( ( !FBFIIsCleanEnoughForEvict( pbf ) || ( pbf->pbfTimeDepChainNext != NULL ) ) && !fEvictDirty )
    {
        Error( ErrERRCheck( errBFIPageDirty ) );
    }


    Enforce( pbf->pbfTimeDepChainNext == NULL || fEvictDirty );

    PBF pbfLocked;
    if ( g_bflruk.ErrGetCurrentResource( plockLRUK, &pbfLocked ) != BFLRUK::ERR::errSuccess )
    {
        Error( ErrERRCheck( errBFLatchConflict ) );
    }
    Enforce( pbf == pbfLocked );



    const BOOL fKeepHistory     =   !fEvictDirty &&
                                    pbf->fCurrentVersion &&
                                    pbf->err != errBFIPageNotVerified &&
                                    !BoolParam( JET_paramEnableViewCache );
    Enforce( pbf->err != errBFIPageFaultPending );
    Enforce( pbf->err != wrnBFPageFlushPending );
    Enforce( pbf->pWriteSignalComplete == NULL );
    Enforce( PvBFIAcquireIOContext( pbf ) == NULL );

    if ( fKeepHistory )
    {
        bfefTraceOnly |= bfefKeepHistory;
    }


    Assert( pbf->fCurrentVersion != pbf->fOlderVersion );


    const BOOL fCurrentVersion = pbf->fCurrentVersion;

    if ( pbf->fCurrentVersion )
    {
        Assert( !( bfefNukePageImage == ( bfefDangerousOptions & bfefNukePageImage ) ) );

        pbf->fCurrentVersion = fFalse;

        BFHash::ERR errHash = g_bfhash.ErrDeleteEntry( &lockHash );
        Assert( errHash == BFHash::ERR::errSuccess );
    }


    if ( pbf->err == errBFIPageNotVerified )
    {
        bfefTraceOnly |= bfefTraceUntouched;
    }

    if ( pbf->lrukic.kLrukPool() == 2 )
    {
        bfefTraceOnly |= bfefTraceK2;
    }

    if ( pbf->bfrs == bfrsNotResident )
    {
        bfefTraceOnly |= bfefTraceNonResident;
    }

    if ( pbf->lrukic.FSuperColded() )
    {
        bfefTraceOnly |= bfefTraceSuperColded;
    }


    Assert( !fCurrentVersion || !pbf->fOlderVersion );
    Assert( ( bfefDangerousOptions & bfefTraceOnly ) == 0 );
    BFITraceEvictPage( pbf->ifmp, pbf->pgno, fCurrentVersion, pbf->err, bfefDangerousOptions | bfefTraceOnly );


    g_bfhash.WriteUnlockKey( &lockHash );
    fOwnsHash = fFalse;



    FMP* pfmp = &g_rgfmp[ pbf->ifmp ];
    pfmp->EnterBFContextAsReader();
    BFFMPContext* pbffmp = ( BFFMPContext* )pfmp->DwBFContext();
    const BOOL fCurrentlyAttached = ( pbffmp && pbffmp->fCurrentlyAttached );

    if ( fCurrentlyAttached )
    {

        if ( pbf->err == errBFIPageNotVerified )
        {
            PERFOpt( cBFCacheEvictUntouched.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
            OSTraceFMP(
                pbf->ifmp,
                JET_tracetagBufferManager,
                OSFormat( "Evicted untouched preread page=[0x%x:0x%x]", (ULONG)pbf->ifmp, pbf->pgno ) );
        }


#ifndef MINIMAL_FUNCTIONALITY
        const ULONG k = pbf->lrukic.kLrukPool();
        switch ( k )
        {
            case 1:
                PERFOpt( cBFCacheEvictk1.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
                break;
            case 2:
                PERFOpt( cBFCacheEvictk2.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
                break;
            default:
                AssertSz( fFalse, "k(%d) Touch not 1 or 2, BF confused.", k );
        }
#endif


        if ( pbf->bfrs == bfrsNotResident )
        {
            
            PERFOpt( AtomicIncrement( (LONG*)&g_cbfNonResidentEvicted ) );
        }


        Assert( iEvictReason >= 0 && iEvictReason < bfefReasonMax );
        if ( iEvictReason >= 0 &&
            iEvictReason < ( bfefReasonMax ) )
        {
            PERFOpt( rgcBFCacheEvictReasons[iEvictReason].Inc( PinstFromIfmp( pbf->ifmp )->m_iInstance, pbf->tce ) );
        }
        if ( pbf->lrukic.FSuperColded() )
        {
            if ( pbf->fOlderVersion )
            {
                PERFOpt( cBFCacheEvictScavengeSuperColdInternal.Inc( PinstFromIfmp( pbf->ifmp )->m_iInstance, pbf->tce ) );
            }
            else
            {
                PERFOpt( cBFCacheEvictScavengeSuperColdUser.Inc( PinstFromIfmp( pbf->ifmp )->m_iInstance, pbf->tce ) );
            }
        }
    }
    else
    {
        PERFOpt( g_cbCacheUnattached -= CbBFIBufferSize( pbf ) );
    }

    pfmp->LeaveBFContextAsReader();


    BFLRUK::ERR errLRUK = g_bflruk.ErrEvictCurrentResource( plockLRUK, IFMPPGNO( pbf->ifmp, pbf->pgno ), fKeepHistory );
    Assert( errLRUK == BFLRUK::ERR::errSuccess );


    BFIResetLgposOldestBegin0( pbf );


    BFICleanPage( pbf, bfltWrite, fAllowTearDownClean ? bfcfAllowTearDownClean : bfcfNone );

    Enforce( pbf->pbfTimeDepChainNext == NULL );
    Enforce( pbf->pbfTimeDepChainPrev == NULL );


    if ( pbf->fOlderVersion )
    {
        Assert( !fCurrentVersion );

#ifdef DEBUG
        PGNOPBF         pgnopbf;
        BFHash::CLock   lock;
        g_bfhash.ReadLockKey( IFMPPGNO( pbf->ifmp, pbf->pgno ), &lock );
        if ( g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf ) == BFHash::ERR::errSuccess )
        {
            Assert( pbf != pgnopbf.pbf );
        }
        g_bfhash.ReadUnlockKey( &lock );
#endif

        pbf->fOlderVersion = fFalse;
        PERFOpt( AtomicDecrement( (LONG*)&g_cBFVersioned ) );
    }


    OSTraceFMP(
        pbf->ifmp,
        JET_tracetagBufferManagerBufferCacheState,
        OSFormat( "Evicted page=[0x%x:0x%x], tce=%d", (ULONG)pbf->ifmp, pbf->pgno, pbf->tce ) );


    pbf->fNewlyEvicted = !pbf->fNewlyEvicted;

#ifdef DEBUG
    if ( bfefNukePageImage == ( bfefDangerousOptions & bfefNukePageImage ) )
    {

        memset( pbf->pv, 0xFF, CbBFIBufferSize( pbf ) );
    }
#endif

    
    BFICacheIUnmapPage( pbf );
    
    Assert( pbf->bfat == bfatNone || !UlParam( JET_paramEnableViewCache ) );


    const BOOL fQuiesce = ( bfefDangerousOptions & bfefQuiesce ) != 0;
    Expected( !fQuiesce || ( bfefDangerousOptions & bfefReasonShrink ) );
    BFIFreePage( pbf, fFalse, fQuiesce ? bffpfQuiesce : bffpfNone );
    fOwnsLatch = fFalse;

    Assert( JET_errSuccess == err );


HandleError:

    if ( fOwnsHash )
    {
        
        g_bfhash.WriteUnlockKey( &lockHash );
        fOwnsHash = fFalse;
    }

    if ( fOwnsLatch )
    {

        pbf->sxwl.ReleaseWriteLatch();
        fOwnsLatch = fFalse;
    }

    return err;
}


void BFIPurgeAllPageVersions( _Inout_ BFLatch* const pbfl, const TraceContext& tc )
{
    PBF pbf = PbfBFILatchContext( pbfl->dwContext );
    Assert( FBFIOwnsLatchType( pbf, bfltWrite ) );
    OnDebug( const IFMP ifmp = pbf->ifmp );
    OnDebug( const PGNO pgno = pbf->pgno );

    EnforceSz( pbf->fAbandoned, "PurgeAllInitNotAbandoned" );
    EnforceSz( pbf->fCurrentVersion, "PurgeAllInitNotCurrent" );
    EnforceSz( !pbf->fOlderVersion, "PurgeAllInitOlder" );

    g_critBFDepend.Enter();
    for ( PBF pbfAbandon = pbf->pbfTimeDepChainNext; pbfAbandon != pbfNil; )
    {
        if ( pbfAbandon->sxwl.ErrTryAcquireExclusiveLatch() == CSXWLatch::ERR::errSuccess )
        {
            if ( !pbfAbandon->fAbandoned )
            {
                pbfAbandon->sxwl.UpgradeExclusiveLatchToWriteLatch();
                pbfAbandon->fAbandoned = fTrue;
                pbfAbandon->sxwl.ReleaseWriteLatch();
            }
            else
            {
                pbfAbandon->sxwl.ReleaseExclusiveLatch();
            }

            pbfAbandon = pbfAbandon->pbfTimeDepChainNext;
        }
        else
        {
            g_critBFDepend.Leave();
            UtilSleep( dtickFastRetry );
            g_critBFDepend.Enter();

            pbfAbandon = pbf->pbfTimeDepChainNext;
        }
    }
    g_critBFDepend.Leave();

    while ( fTrue )
    {
        EnforceSz( pbf->fAbandoned, "PurgeAllCurNotAbandoned" );
        EnforceSz( pbf->fCurrentVersion, "PurgeAllCurNotCurrent" );
        EnforceSz( !pbf->fOlderVersion, "PurgeAllCurOlder" );

        g_critBFDepend.Enter();
        if ( pbf->pbfTimeDepChainNext != pbfNil )
        {
            BF* pbfVersion = PbfBFIGetFlushOrderLeaf( pbf, fFalse );
            Assert( ( pbfVersion != pbfNil ) && ( pbfVersion != pbf ) );
            const BOOL fVersionLatched = pbfVersion->sxwl.ErrTryAcquireWriteLatch() == CSXWLatch::ERR::errSuccess;

            g_critBFDepend.Leave();

            if ( fVersionLatched )
            {
                EnforceSz( pbfVersion->fAbandoned, "PurgeAllVerNotAbandoned" );
                EnforceSz( !pbfVersion->fCurrentVersion, "PurgeAllVerCurrent" );
                EnforceSz( pbfVersion->fOlderVersion, "PurgeAllVerNotOlder" );
                BFIPurgePage( pbfVersion, pbfVersion->ifmp, pbfVersion->pgno, bfltWrite, BFEvictFlags( bfefReasonPurgePage | bfefEvictDirty ) );
                pbfVersion = pbfNil;
            }
        }
        else
        {
            g_critBFDepend.Leave();
        }

        g_critBFDepend.Enter();
        if ( pbf->pbfTimeDepChainNext == pbfNil )
        {
            g_critBFDepend.Leave();
            BFIPurgePage( pbf, pbf->ifmp, pbf->pgno, bfltWrite, BFEvictFlags( bfefReasonPurgePage | bfefEvictDirty ) );
            pbf = pbfNil;
            break;
        }
        else
        {
            g_critBFDepend.Leave();
        }
    }

    pbfl->pv = NULL;
    pbfl->dwContext = NULL;
}


void BFIPurgeNewPage( _Inout_ const PBF pbf, const TraceContext& tc )
{
    Assert( pbf->sxwl.FOwnWriteLatch() );

    EnforceSz( pbf->fAbandoned, "PurgeAbandonedVerNotAbandoned" );
    EnforceSz( pbf->fCurrentVersion, "PurgeAbandonedVerNotCurrent" );
    EnforceSz( !pbf->fOlderVersion, "PurgeAbandonedVerOlder" );

    g_critBFDepend.Enter();
    EnforceSz( ( pbf->pbfTimeDepChainNext == pbfNil ) &&
               ( pbf->pbfTimeDepChainPrev == pbfNil ), "PurgeAbandonedVerTooManyVersions" );
    g_critBFDepend.Leave();

    OnDebug( const IFMP ifmp = pbf->ifmp );
    OnDebug( const PGNO pgno = pbf->pgno );
    BFIPurgePage( pbf, pbf->ifmp, pbf->pgno, bfltWrite, BFEvictFlags( bfefReasonPurgePage | bfefEvictDirty ) );
    Assert( !FBFInCache( ifmp, pgno ) );
}


void BFIPurgePage(
    _Inout_ const PBF pbf,
    _In_ const IFMP ifmpCheck,
    _In_ const PGNO pgnoCheck,
    _In_ const BFLatchType bfltHave,
    _In_ const BFEvictFlags bfefDangerousOptions )
{
    Assert( ( bfltHave == bfltMax ) || ( bfltHave == bfltWrite ) );
    Assert( ( bfltHave == bfltMax ) || pbf->sxwl.FOwnWriteLatch() );
    Assert( ( bfltHave == bfltMax ) || ( ( pbf->ifmp == ifmpCheck ) && ( pbf->pgno == pgnoCheck ) ) );
    Assert( ( bfefDangerousOptions & bfefEvictDirty ) != 0 );

    BOOL fLatched = ( bfltHave != bfltMax );
    const BOOL fAllowTearDownClean = ( 0 != ( bfefDangerousOptions & bfefAllowTearDownClean ) );

    while ( fTrue )
    {
        if ( ( pbf->ifmp != ifmpCheck ) || ( pbf->pgno != pgnoCheck ) )
        {
            break;
        }

        if ( !fLatched )
        {
            const CSXWLatch::ERR errSXWL = pbf->sxwl.ErrTryAcquireWriteLatch();
            if ( errSXWL == CSXWLatch::ERR::errSuccess )
            {
                fLatched = fTrue;
            }
            else
            {
                Assert( errSXWL == CSXWLatch::ERR::errLatchConflict );
                UtilSleep( dtickFastRetry );
                continue;
            }
        }

        if ( ( pbf->ifmp != ifmpCheck ) || ( pbf->pgno != pgnoCheck ) )
        {
            break;
        }

        if ( pbf->err == wrnBFPageFlushPending &&
                ErrBFIWriteSignalState( pbf ) == wrnBFPageFlushPending )
        {
            BFIReleaseSXWL( pbf, bfltWrite );
            fLatched = fFalse;
            UtilSleep( dtickFastRetry );
            continue;
        }
        Assert( pbf->err != wrnBFPageFlushPending ||
                ErrBFIWriteSignalState( pbf ) != wrnBFPageFlushPending );

        if ( pbf->err == wrnBFPageFlushPending &&
            !FBFICompleteFlushPage( pbf, bfltWrite, fFalse, fFalse, fAllowTearDownClean ) )
        {
            AssertSz( fFalse, "In BFPurge() couldn't, complete write IO!" );
        }
        Assert( FBFIUpdatablePage( pbf ) );

        if ( BoolParam( PinstFromIfmp( pbf->ifmp ), JET_paramEnableExternalAutoHealing ) )
        {
            Assert( FBFIOwnsLatchType( pbf, bfltExclusive ) || FBFIOwnsLatchType( pbf, bfltWrite ) );
            PagePatching::CancelPatchRequest( pbf->ifmp, pbf->pgno );
        }


        BFLRUK::CLock   lockLRUK;
        g_bflruk.LockResourceForEvict( pbf, &lockLRUK );


        pbf->fNewlyEvicted = fTrue;


        BFIReleaseSXWL( pbf, bfltWrite );
        fLatched = fFalse;

        const ERR errEvict = ErrBFIEvictPage( pbf, &lockLRUK, bfefDangerousOptions );


        g_bflruk.UnlockResourceForEvict( &lockLRUK );


        if ( errEvict < JET_errSuccess )
        {
            ExpectedSz( errEvict == errBFIPageDirty ||
                    errEvict == errBFIPageFlushPending ||
                    errEvict == errBFIPageFlushPendingSlowIO ||
                    errEvict == errBFIPageFlushPendingHungIO ||
                    errEvict == errBFIPageFaultPending ||
                    errEvict == errBFIPageFaultPendingHungIO ||
                    errEvict == errBFLatchConflict, "Unknown errEvict=%d.", errEvict );
            UtilSleep( dtickFastRetry );
            continue;
        }
        else
        {
            break;
        }
    }

    if ( fLatched )
    {
        BFIReleaseSXWL( pbf, bfltWrite );
        fLatched = fFalse;
    }
}

void BFIRenouncePage( _Inout_ PBF pbf, _In_ const BOOL fRenounceDirty )
{

    Assert( pbf->sxwl.FOwnWriteLatch() );


    Enforce( pbf->err != errBFIPageFaultPending );
    Enforce( pbf->err != wrnBFPageFlushPending );
    Enforce( pbf->pWriteSignalComplete == NULL );


    if ( ( pbf->bfdf == bfdfClean ) ||
        ( fRenounceDirty && pbfNil == pbf->pbfTimeDepChainNext ) )
    {

        if ( fRenounceDirty )
        {
            Assert( !FBFIDatabasePage( pbf ) );
        }

        if ( JET_errSuccess <= ErrBFIEvictRemoveCleanVersions( pbf ) )
        {

            Enforce( pbf->pbfTimeDepChainPrev == NULL );
            Enforce( pbf->pbfTimeDepChainNext == NULL );

            BFICleanPage( pbf, bfltWrite );
        }
    }


    pbf->fNewlyEvicted = fTrue;
}


const CHAR mpbfdfsz[ bfdfMax - bfdfMin ][ 16 ] =
{
    "bfdfClean",
    "bfdfUntidy",
    "bfdfDirty",
    "bfdfFilthy",
};

C_ASSERT( _countof( mpbfdfsz ) == bfdfFilthy + 1 );
C_ASSERT( _countof( mpbfdfsz ) == bfdfMax );



void BFIDirtyPage( PBF pbf, BFDirtyFlags bfdf, const TraceContext& tc )
{
    Assert( bfdfClean < bfdf );
    Assert( !g_rgfmp[ pbf->ifmp ].m_fReadOnlyAttach );

    if ( pbf->bfdf == bfdfClean )
    {

        BFIResetLgposOldestBegin0( pbf );


        pbf->err = JET_errSuccess;
    }


    if ( bfdf <= bfdfUntidy )
    {
        if ( pbf->bfdf < bfdfUntidy )
        {
            OSTraceFMP(
                pbf->ifmp,
                JET_tracetagBufferManager,
                OSFormat( "Untidied page=[0x%x:0x%x], tcd=%d", (ULONG)pbf->ifmp, pbf->pgno, pbf->tce ) );
        }
    }
    else if ( bfdf >= bfdfDirty )
    {


        BFITraceDirtyPage( pbf, bfdf, tc );

        Assert( pbf->pgno <= g_rgfmp[pbf->ifmp].PgnoLast() ||
                g_rgfmp[pbf->ifmp].FBeyondPgnoShrinkTarget( pbf->pgno ) ||
                g_rgfmp[pbf->ifmp].FOlderDemandExtendDb() );
        g_rgfmp[pbf->ifmp].UpdatePgnoDirtiedMax( pbf->pgno );

        TLS* const ptls = Ptls();

        if ( pbf->bfdf < bfdfDirty )
        {

            ptls->threadstats.cPageDirtied++;
            OSTraceFMP(
                pbf->ifmp,
                JET_tracetagBufferManagerBufferDirtyState,
                OSFormat( "Dirtied page=[0x%x:0x%x] from %hs, tce=%d", (ULONG)pbf->ifmp, pbf->pgno, mpbfdfsz[ pbf->bfdf ], pbf->tce ) );
            PERFOpt( cBFDirtied.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
        }
        else
        {
            ptls->threadstats.cPageRedirtied++;
            OSTraceFMP(
                pbf->ifmp,
                JET_tracetagBufferManager,
                OSFormat( "%hs page=[0x%x:0x%x], tce=%d", ( bfdf == bfdfDirty && pbf->bfdf == bfdfDirty ) ? "Re-dirtied" : "Filthied", (ULONG)pbf->ifmp, pbf->pgno, pbf->tce ) );
            PERFOpt( cBFDirtiedRepeatedly.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
        }

        const TICK tickLastDirtiedBefore = pbf->tickLastDirtied;
        const TICK tickLastDirtiedAfter = pbf->tickLastDirtied = TickOSTimeCurrent();

        Assert( TickCmp( tickLastDirtiedAfter, tickLastDirtiedBefore ) >= 0 );

        const BOOL fUpdateThreadStats = ( TickCmp( tickLastDirtiedBefore, ptls->TickThreadStatsLast() ) <= 0 );

        if ( fUpdateThreadStats )
        {
            ptls->threadstats.cPageUniqueModified++;
        }
    }

    Enforce( pbf->pbfTimeDepChainPrev == NULL );


    if ( pbf->bfdf < bfdfDirty && bfdf >= bfdfDirty )
    {
        AtomicDecrement( (LONG*)&g_cbfCacheClean );
    }



    if ( ( bfdf >= bfdfDirty ) && BoolParam( JET_paramEnableFileCache ) && pbf->fCurrentVersion )
    {
        Expected( !pbf->fOlderVersion );
        BFITouchResource( pbf, bfltExclusive, bflfDefault, fTrue, g_pctCachePriorityNeutral, tc );
    }

    pbf->bfdf = BYTE( max( pbf->bfdf, bfdf ) );
    Assert( pbf->bfdf == max( pbf->bfdf, bfdf ) );

}


void BFICleanVersion( PBF pbf, BOOL fTearDownFMP )
{
    Assert( g_critBFDepend.FOwner() );
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );

    
    Assert( !pbf->fOlderVersion || !pbf->fCurrentVersion );

    if (    pbf->pbfTimeDepChainPrev != pbfNil ||
            pbf->pbfTimeDepChainNext != pbfNil ||
            pbf->bfbitfield.FDependentPurged() )
    {

        Enforce( pbf->pbfTimeDepChainNext == NULL || fTearDownFMP );

        while ( pbf->pbfTimeDepChainPrev != pbfNil ||
                pbf->pbfTimeDepChainNext != pbfNil )
        {

            PBF pbfT = pbf;
            while ( pbfT->pbfTimeDepChainNext != pbfNil )
            {
                pbfT = pbfT->pbfTimeDepChainNext;
                Assert( pbfT->ifmp == pbf->ifmp );
            }


            if ( pbfT->pbfTimeDepChainPrev != pbfNil )
            {
                BF * const  pbfDepT = pbfT->pbfTimeDepChainPrev;

                pbfDepT->bfbitfield.SetFDependentPurged( pbfDepT->bfbitfield.FDependentPurged() || ( wrnBFPageFlushPending != pbf->err ) );
                pbfDepT->pbfTimeDepChainNext    = pbfNil;
                pbfT->pbfTimeDepChainPrev       = pbfNil;
                Enforce( pbfT->pbfTimeDepChainNext == NULL );
            }
        }

        pbf->bfbitfield.SetFDependentPurged( fFalse );

        pbf->bfbitfield.SetFImpedingCheckpoint( fFalse );

        Enforce( pbf->pbfTimeDepChainNext == NULL );
        Enforce( pbf->pbfTimeDepChainPrev == NULL );
    }
}


void BFICleanPage( __inout PBF pbf, __in const BFLatchType bfltHave, __in const BFCleanFlags bfcf )
{
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );
    Assert( FBFIOwnsLatchType( pbf, bfltHave ) );


    const BOOL fTearDown = bfcfAllowTearDownClean & bfcf;

    Enforce( pbf->err != errBFIPageFaultPending );
    Enforce( pbf->pWriteSignalComplete == NULL );
    Enforce( PvBFIAcquireIOContext( pbf ) == NULL );


    if (    pbf->pbfTimeDepChainPrev != pbfNil ||
            pbf->pbfTimeDepChainNext != pbfNil ||
            pbf->bfbitfield.FDependentPurged() )
    {
        g_critBFDepend.Enter();

        Enforce( pbf->pbfTimeDepChainNext == NULL || fTearDown );

        BFICleanVersion( pbf, fTearDown );

        Enforce( pbf->pbfTimeDepChainNext == NULL );
        Enforce( pbf->pbfTimeDepChainPrev == NULL );

        g_critBFDepend.Leave();
    }


    if ( pbf->bfbitfield.FImpedingCheckpoint() )
    {
        g_critBFDepend.Enter();
        pbf->bfbitfield.SetFImpedingCheckpoint( fFalse );
        g_critBFDepend.Leave();
    }

    Enforce( pbf->pbfTimeDepChainNext == NULL );
    Enforce( pbf->pbfTimeDepChainPrev == NULL );


    if ( pbf->prceUndoInfoNext != prceNil )
    {
        ENTERCRITICALSECTION ecs( &g_critpoolBFDUI.Crit( pbf ) );

        Assert( fTearDown );

        while ( pbf->prceUndoInfoNext != prceNil )
        {
            BFIRemoveUndoInfo( pbf, pbf->prceUndoInfoNext );
        }
    }


    BFIResetLgposModify( pbf );
    pbf->rbsposSnapshot = rbsposMin;


    FMP* pfmp = &g_rgfmp[ pbf->ifmp ];
    if ( CmpLgpos( pbf->lgposOldestBegin0, lgposMax ) )
    {
        BFIMaintCheckpointDepthRequest( pfmp, bfcpdmrRequestRemoveCleanEntries );
        BFIMaintCheckpointRequest();
    }


    if ( pbf->ifmp != ifmpNil &&
        PinstFromIfmp( pbf->ifmp ) )
    {
        if ( pbf->err == errBFIPageNotVerified )
        {
            PERFOpt( cBFCacheUnused.Dec( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
        }
    }

    pbf->err = JET_errSuccess;


    if ( pbf->fFlushed )
    {
        pbf->fFlushed = fFalse;
    }

    pbf->fLazyIO = fFalse;

    pbf->fSuspiciouslySlowRead = fFalse;


    if ( pbf->bfdf >= bfdfDirty )
    {
        AtomicIncrement( (LONG*)&g_cbfCacheClean );
    }

    OSTraceFMP( pbf->ifmp, JET_tracetagBufferManagerBufferDirtyState,
                OSFormat(   "Cleaned page=[0x%x:0x%x] from %hs, tce=%d",
                            (ULONG)pbf->ifmp,
                            pbf->pgno,
                            mpbfdfsz[ pbf->bfdf ],
                            pbf->tce ) );

    pbf->bfdf = bfdfClean;


    if ( !fTearDown &&
            pbf->fCurrentVersion
            )
    {


        CSXWLatch::ERR errSXWL = CSXWLatch::ERR::errSuccess;
        if ( bfltExclusive == bfltHave )
        {
            errSXWL = pbf->sxwl.ErrTryUpgradeExclusiveLatchToWriteLatch();
        }

        if ( CSXWLatch::ERR::errSuccess == errSXWL )
        {

            BFIDehydratePage( pbf, bfcf & bfcfAllowReorganization );

            if ( bfltExclusive == bfltHave )
            {

                pbf->sxwl.DowngradeWriteLatchToExclusiveLatch();
            }
        }
    }

    Assert( FBFIOwnsLatchType( pbf, bfltHave ) );
}




void BFIDehydratePage( PBF pbf, __in const BOOL fAllowReorg )
{
    Assert( pbf->sxwl.FOwnWriteLatch() );

    if ( pbf->err < JET_errSuccess )
    {

        return;
    }

    if ( FBFICacheViewCacheDerefIo( pbf ) )
    {

        return;
    }

    if ( !FBFIDatabasePage( pbf ) )
    {

        return;
    }

    if ( pbf->bfdf > bfdfUntidy )
    {

        return;
    }

    if ( pbf->icbBuffer < icbPage4KB )
    {

        Assert( pbf->icbPage == icbPage2KB );
        Assert( pbf->icbBuffer == icbPage2KB );
        return;
    }


    const BFLatch bfl = { pbf->pv, (DWORD_PTR)pbf };

    CPAGE cpage;

    cpage.ReBufferPage( bfl, pbf->ifmp, pbf->pgno, pbf->pv, g_rgcbPageSize[pbf->icbPage] );


    ULONG cbMinReqSize;

    if ( cpage.FPageIsDehydratable( &cbMinReqSize, fAllowReorg ) )
    {


        ICBPage icbNewSize = max( icbPage4KB, IcbBFIBufferSize( cbMinReqSize ) );
        Assert( cbMinReqSize <= (ULONG)g_rgcbPageSize[icbNewSize] );

        if ( icbNewSize != pbf->icbBuffer )
        {

            OSTrace( JET_tracetagBufferManager, OSFormat( "Dehydrating Page %d:%d to %d bytes.\n", (ULONG)pbf->ifmp, pbf->pgno, g_rgcbPageSize[icbNewSize] ) );

            cpage.DehydratePage( g_rgcbPageSize[icbNewSize], fAllowReorg );

            PERFOpt( cBFPagesDehydrated.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );


            Assert( pbf->icbBuffer == icbNewSize );
        }
    }
}


void BFIRehydratePage( PBF pbf )
{
    Assert( pbf->sxwl.FOwnWriteLatch() );
    Assert( pbf->icbBuffer != icbPage0 );

    if ( pbf->icbBuffer == pbf->icbPage )
    {
        
        return;
    }


    Assert( !BoolParam( JET_paramEnableViewCache ) );


    Assert( FBFIDatabasePage( pbf ) );

    
    const BFLatch bfl = { pbf->pv, (DWORD_PTR)pbf };

    CPAGE cpage;

    cpage.ReBufferPage( bfl, pbf->ifmp, pbf->pgno, pbf->pv, g_rgcbPageSize[pbf->icbPage] );


    OSTrace( JET_tracetagBufferManager, OSFormat( "Rehydrating Page %d:%d from %d bytes.\n", (ULONG)pbf->ifmp, pbf->pgno, g_rgcbPageSize[pbf->icbBuffer] ) );

    cpage.RehydratePage();

    PERFOpt( cBFPagesRehydrated.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );



    Assert( pbf->icbBuffer == pbf->icbPage );
}




ERR ErrBFIWriteLog( __in const IFMP ifmp, __in const BOOL fSync )
{
    PIB pibFake;
    pibFake.m_pinst = PinstFromIfmp( ifmp );
    if ( fSync )
    {
        return ErrLGWaitForWrite( &pibFake, &lgposMax );
    }

    return ErrLGWrite( &pibFake );
}

ERR ErrBFIFlushLog( __in const IFMP ifmp, __in const IOFLUSHREASON iofr, const BOOL fMayOwnBFLatch )
{
    return ErrLGFlush( PinstFromIfmp( ifmp )->m_plog, iofr, fMayOwnBFLatch );
}


const void* const PV_IO_CTX_LOCK = (void*)upMax;
void* PvBFIAcquireIOContext( PBF pbf )
{
    void* pvIOContextOld = AtomicReadPointer( &pbf->pvIOContext );
    OSSYNC_FOREVER
    {
        if ( pvIOContextOld == NULL )
        {
            return NULL;
        }

        if ( pvIOContextOld == PV_IO_CTX_LOCK )
        {
            UtilSleep( 1 );
            pvIOContextOld = AtomicReadPointer( &pbf->pvIOContext );
            continue;
        }

        void* const pvIOContextOldReplaced = AtomicCompareExchangePointer( &pbf->pvIOContext, pvIOContextOld, (void*)PV_IO_CTX_LOCK );

        if ( pvIOContextOldReplaced != pvIOContextOld )
        {
            pvIOContextOld = pvIOContextOldReplaced;
            continue;
        }

        Assert( AtomicReadPointer( &pbf->pvIOContext ) == PV_IO_CTX_LOCK );
        Assert( ( pvIOContextOldReplaced != NULL ) && ( pvIOContextOldReplaced != PV_IO_CTX_LOCK ) );
        return pvIOContextOldReplaced;
    }
}

void BFIReleaseIOContext( PBF pbf, void* const pvIOContext )
{
    Assert( pvIOContext != PV_IO_CTX_LOCK );
    Expected( pvIOContext != NULL );
    OnDebug( void* const pvIOContextOld = ) AtomicExchangePointer( &pbf->pvIOContext, pvIOContext );
    Assert( pvIOContextOld == PV_IO_CTX_LOCK );
}

void BFISetIOContext( PBF pbf, void* const pvIOContextNew )
{
    Assert( pvIOContextNew != PV_IO_CTX_LOCK );
    Expected( pvIOContextNew != NULL );
    OnDebug( void* const pvIOContextOld = ) AtomicExchangePointer( &pbf->pvIOContext, pvIOContextNew );
    Assert( pvIOContextOld == NULL );
}

void BFIResetIOContext( PBF pbf )
{
    void* pvIOContextOld = AtomicReadPointer( &pbf->pvIOContext );
    OSSYNC_FOREVER
    {
        Expected( pvIOContextOld != NULL );

        if ( pvIOContextOld == NULL )
        {
            return;
        }

        if ( pvIOContextOld == PV_IO_CTX_LOCK )
        {
            UtilSleep( 1 );
            pvIOContextOld = AtomicReadPointer( &pbf->pvIOContext );
            continue;
        }

        void* const pvIOContextOldReplaced = AtomicCompareExchangePointer( &pbf->pvIOContext, pvIOContextOld, NULL );

        if ( pvIOContextOldReplaced != pvIOContextOld )
        {
            pvIOContextOld = pvIOContextOldReplaced;
            continue;
        }

        Assert( ( pvIOContextOldReplaced != NULL ) && ( pvIOContextOldReplaced != PV_IO_CTX_LOCK ) );
        return;
    }
}

BOOL FBFIIsIOHung( PBF pbf )
{
    void* const pvIOContext = PvBFIAcquireIOContext( pbf );
    if ( pvIOContext == NULL )
    {
        return fFalse;
    }

    const BOOL fHung = ( PctBFIIsIOHung( pbf, pvIOContext ) >= 100 );

    BFIReleaseIOContext( pbf, pvIOContext );

    return fHung;
}

BYTE PctBFIIsIOHung( PBF pbf, void* const pvIOContext )
{
    IFileAPI* const pfapi = g_rgfmp[ pbf->ifmp ].Pfapi();

    const TICK dtickIOElapsed = pfapi->DtickIOElapsed( pvIOContext );
    const TICK dtickHungIO = (TICK)UlParam( JET_paramHungIOThreshold );

    if ( dtickIOElapsed >= dtickHungIO )
    {
        return 100;
    }
    else
    {
        return (BYTE)( ( (QWORD)dtickIOElapsed * 100 ) / dtickHungIO );
    }
}

ERR ErrBFIFlushPendingStatus( PBF pbf )
{
    ERR err = JET_errSuccess;


    void* const pvIOContext = PvBFIAcquireIOContext( pbf );
    if ( pvIOContext == NULL )
    {
        Error( ErrERRCheck( errBFIPageFlushPending ) );
    }

    const BYTE pctIOLatencyToHung = PctBFIIsIOHung( pbf, pvIOContext );
    Expected( pctIOLatencyToHung <= 100 );
    BFIReleaseIOContext( pbf, pvIOContext );


    if ( pctIOLatencyToHung >= 100 )
    {
        Error( ErrERRCheck( errBFIPageFlushPendingHungIO ) );
    }


    if ( pctIOLatencyToHung >= 2 )
    {
        Error( ErrERRCheck( errBFIPageFlushPendingSlowIO ) );
    }

    err = ErrERRCheck( errBFIPageFlushPending );

HandleError:
    Assert( ( err == errBFIPageFlushPending ) ||
            ( err == errBFIPageFlushPendingSlowIO ) ||
            ( err == errBFIPageFlushPendingHungIO ) );
    return err;
}

void BFIPrepareReadPage( PBF pbf )
{

    Enforce( pbf->err != errBFIPageFaultPending );
    Enforce( pbf->err != wrnBFPageFlushPending );
    Enforce( pbf->pWriteSignalComplete == NULL );
    Enforce( PvBFIAcquireIOContext( pbf ) == NULL );

    ERR errT = ErrERRCheck( errBFIPageFaultPending );
    pbf->err = SHORT( errT );
    Assert( pbf->err == errT );



    g_rgfmp[ pbf->ifmp ].TraceStationId( tsidrPulseInfo );
}

void BFIPrepareWritePage( PBF pbf )
{
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );

    EnforceSz( !pbf->fAbandoned, "PrepWriteAbandonedBf" );
    

    Enforce( pbf->err != errBFIPageFaultPending );
    Enforce( pbf->err != wrnBFPageFlushPending );
    Enforce( pbf->pWriteSignalComplete == NULL );

    ERR errT = ErrERRCheck( wrnBFPageFlushPending );
    pbf->err = SHORT( errT );
    Assert( pbf->err == errT );

    Enforce( CmpLgpos( pbf->lgposModify, g_rgfmp[ pbf->ifmp ].LgposWaypoint() ) <= 0 );
    Enforce( pbf->icbBuffer == pbf->icbPage );

    IFMPPGNO        ifmppgno    = IFMPPGNO( pbf->ifmp, pbf->pgno );
    PGNOPBF         pgnopbf;
    BFHash::ERR     errHash;
    BFHash::CLock   lock;
    PBF             pbfT        = pbfNil;


    g_bfhash.ReadLockKey( ifmppgno, &lock );
    errHash = g_bfhash.ErrRetrieveEntry( &lock, &pgnopbf );
    Enforce( errHash == BFHash::ERR::errSuccess );

    for ( pbfT = pgnopbf.pbf; pbfT != pbfNil && pbfT != pbf; pbfT = pbfT->pbfTimeDepChainNext );
    Enforce( pbfT == pbf );
    Enforce( pbf->pbfTimeDepChainNext == pbfNil );

    g_bfhash.ReadUnlockKey( &lock );
}


void BFISyncRead( PBF pbf, const OSFILEQOS qosIoPriorities, const TraceContext& tc )
{
    Assert( pbf->sxwl.FOwnWriteLatch() );


    BFIPrepareReadPage( pbf );

    pbf->fLazyIO = fFalse;

    AssertRTL( CbBFIBufferSize( pbf ) == CbBFIPageSize( pbf ) );

    IFileAPI *const pfapi       =   g_rgfmp[pbf->ifmp].Pfapi();
    const QWORD     ibOffset    =   OffsetOfPgno( pbf->pgno );
    const DWORD     cbData      =   CbBFIPageSize( pbf );
    BYTE* const     pbData      =   (BYTE*)pbf->pv;
    ERR             err         =   JET_errSuccess;

    const OSFILEQOS qosIoUserDispatch = QosBFIMergeInstUserDispPri( PinstFromIfmp( pbf->ifmp ), (BFTEMPOSFILEQOS)qosIoPriorities );



    if ( !FBFICacheViewCacheDerefIo( pbf ) )
    {
        HRT hrtStart = HrtHRTCount();
        err = pfapi->ErrIORead( tc,
                                ibOffset,
                                cbData,
                                pbData,
                                qosIoUserDispatch | qosIOSignalSlowSyncIO,
                                NULL,
                                DWORD_PTR( pbf ),
                                IFileAPI::PfnIOHandoff( BFISyncReadHandoff )  );
        BFITrackCacheMissLatency( pbf, hrtStart, ( tc.iorReason.Iorf() & iorfReclaimPageFromOS ) ? bftcmrReasonPagingFaultDb : bftcmrReasonSyncRead, qosIoPriorities, tc, err );
        Ptls()->threadstats.cPageRead++;
    }


    BFISyncReadComplete( err, pfapi, err == wrnIOSlow ? qosIOCompleteIoSlow : 0, ibOffset, cbData, pbData, pbf );


}

void BFISyncReadHandoff(    const ERR           err,
                            IFileAPI* const     pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS     grbitQOS,
                            const QWORD         ibOffset,
                            const DWORD         cbData,
                            const BYTE* const   pbData,
                            const PBF           pbf,
                            void* const         pvIOContext )
{
    Assert( JET_errSuccess == err );

    BFISetIOContext( pbf, pvIOContext );
}

void BFISyncReadComplete(   const ERR           err,
                            IFileAPI* const     pfapi,
                            const OSFILEQOS     grbitQOS,
                            const QWORD         ibOffset,
                            const DWORD         cbData,
                            const BYTE* const   pbData,
                            const PBF           pbf )

{
    Assert( pbf->sxwl.FOwnWriteLatch() );


    if ( AtomicReadPointer( &pbf->pvIOContext ) != NULL )
    {
        BFIResetIOContext( pbf );
    }
    else
    {
        Assert( FBFICacheViewCacheDerefIo( pbf ) );
    }


    if ( err >= 0 )
    {

        ERR errT = ErrERRCheck( errBFIPageNotVerified );
        pbf->err = SHORT( errT );
        Assert( pbf->err == errT );

        PERFOpt( cBFCacheUnused.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );


        if ( grbitQOS & qosIOCompleteIoSlow )
        {
            pbf->fSuspiciouslySlowRead = fTrue;
        }
    }


    else
    {

        pbf->err = SHORT( err );
        Assert( pbf->err == err );
        Assert( pbf->err != JET_errFileIOBeyondEOF );
    }

    pbf->fLazyIO = fFalse;

    PERFOpt( cBFPagesReadSync.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
}

ERR ErrBFIAsyncPreReserveIOREQ( IFMP ifmp, PGNO pgno, OSFILEQOS qos, VOID ** ppioreq )
{
    IFileAPI *const pfapi       =   g_rgfmp[ifmp].Pfapi();
    const QWORD     ibOffset    =   OffsetOfPgno( pgno );
    const DWORD     cbData      =   g_rgfmp[ifmp].CbPage();

    return pfapi->ErrReserveIOREQ( ibOffset, cbData, qos, ppioreq );
}

VOID BFIAsyncReleaseUnusedIOREQ( IFMP ifmp, VOID * pioreq )
{
    IFileAPI *const pfapi       =   g_rgfmp[ifmp].Pfapi();
    pfapi->ReleaseUnusedIOREQ( pioreq );
}


ERR ErrBFIAsyncRead( PBF pbf, OSFILEQOS qos, VOID * pioreq, const TraceContext& tc )
{
    ERR err = JET_errSuccess;

    Assert( pbf->sxwl.FOwnWriteLatch() );
    Assert( pioreq );
    Assert( tc.iorReason.Iorp() != iorpNone );


    BFIPrepareReadPage( pbf );

    pbf->fLazyIO = fFalse;

    AssertRTL( CbBFIBufferSize( pbf ) == CbBFIPageSize( pbf ) );

    IFileAPI *const pfapi       =   g_rgfmp[pbf->ifmp].Pfapi();
    const QWORD     ibOffset    =   OffsetOfPgno( pbf->pgno );
    const DWORD     cbData      =   CbBFIPageSize( pbf );
    BYTE* const     pbData      =   (BYTE*)pbf->pv;
    
    Assert( qosIODispatchImmediate == ( qosIODispatchMask & qos ) ||
            qosIODispatchBackground == ( qosIODispatchMask & qos ) );

    Expected( pbf->bfat != bfatPageAlloc || tc.iorReason.Iort() == iortDbScan );

    const BOOL fMapped = FBFICacheViewCacheDerefIo( pbf );
    if ( fMapped )
    {
        Assert( BoolParam( JET_paramEnableViewCache ) );
        g_rgfmp[ pbf->ifmp ].IncrementAsyncIOForViewCache();
    }

    if ( pbf->bfat == bfatViewMapped && FOSMemoryPageResident( pbData, cbData ) )
    {
        
        Assert( BoolParam( JET_paramEnableViewCache ) );
        BFIAsyncReleaseUnusedIOREQ( pbf->ifmp, pioreq );
        pioreq = NULL;
        OSTraceFMP( pbf->ifmp, JET_tracetagBufferManager, OSFormat( "OS File Cache preread skipped for page=[0x%x:0x%x]", (ULONG)pbf->ifmp, pbf->pgno ) );
        
        FullTraceContext ftc;
        ftc.DeepCopy( GetCurrUserTraceContext().Utc(), tc );
        BFIAsyncReadHandoff( JET_errSuccess, pfapi, ftc, qos, ibOffset, cbData, pbData, pbf, NULL );
        BFIAsyncReadComplete( JET_errSuccess, pfapi, ftc, qos, ibOffset, cbData, g_rgbBFTemp, pbf );
        Ptls()->cbfAsyncReadIOs++;
        CallS( err );
        goto HandleError;
    }


    err = pfapi->ErrIORead( tc,
                                ibOffset,
                                cbData,
                                fMapped ? g_rgbBFTemp : pbData,
                                qos,
                                IFileAPI::PfnIOComplete( BFIAsyncReadComplete ),
                                DWORD_PTR( pbf ),
                                IFileAPI::PfnIOHandoff( BFIAsyncReadHandoff ),
                                pioreq );

    CallS( err );
    Assert( ( qos & qosIOOptimizeCombinable ) || err == JET_errSuccess );

    if ( err < JET_errSuccess )
    {
        if ( fMapped )
        {
            g_rgfmp[ pbf->ifmp ].DecrementAsyncIOForViewCache();
        }

        goto HandleError;
    }
    

    if ( err >= JET_errSuccess )
    {
        Ptls()->cbfAsyncReadIOs++;
    }

HandleError:

    return err;
}

void BFIAsyncReadHandoff(   const ERR           err,
                            IFileAPI* const     pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS     grbitQOS,
                            const QWORD         ibOffset,
                            const DWORD         cbData,
                            const BYTE* const   pbData,
                            const PBF           pbf,
                            void* const         pvIOContext )
{
    Assert( JET_errSuccess == err );

    if ( pvIOContext != NULL )
    {
        BFISetIOContext( pbf, pvIOContext );
    }
    else
    {
        Assert( FBFICacheViewCacheDerefIo( pbf ) );
    }

    pbf->sxwl.ReleaseOwnership( bfltWrite );
}

void BFIAsyncReadComplete(  const ERR           err,
                            IFileAPI* const     pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS     grbitQOS,
                            const QWORD         ibOffset,
                            const DWORD         cbData,
                            const BYTE* const   pbData,
                            const PBF           pbf )
{

    pbf->sxwl.ClaimOwnership( bfltWrite );


    if ( AtomicReadPointer( &pbf->pvIOContext ) != NULL )
    {
        BFIResetIOContext( pbf );
    }
    else
    {
        Assert( FBFICacheViewCacheDerefIo( pbf ) );
    }


    Assert( !FBFICacheViewCacheDerefIo( pbf ) || pbData == g_rgbBFTemp );
    Assert( FBFICacheViewCacheDerefIo( pbf ) || pbData != g_rgbBFTemp );


    if ( err >= 0 )
    {

        ERR errT = ErrERRCheck( errBFIPageNotVerified );
        pbf->err = SHORT( errT );
        Assert( pbf->err == errT );

        PERFOpt( cBFCacheUnused.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );


        if ( grbitQOS & qosIOCompleteIoSlow )
        {
            pbf->fSuspiciouslySlowRead = fTrue;
        }
    }


    else
    {

        pbf->err = SHORT( err );
        Assert( pbf->err == err );
        Assert( pbf->err != JET_errFileIOBeyondEOF );
    }

    pbf->fLazyIO = fFalse;

    PERFOpt( cBFPagesReadAsync.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );

    if ( grbitQOS & qosIOOptimizeCombinable )
    {
        PERFOpt( cBFPagesCoalescedRead.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
    }


    ETCachePrereadPage(
        pbf->ifmp,
        pbf->pgno,
        tc.utc.context.dwUserID,
        tc.utc.context.nOperationID,
        tc.utc.context.nOperationType,
        tc.utc.context.nClientType,
        tc.utc.context.fFlags,
        tc.utc.dwCorrelationID,
        tc.etc.iorReason.Iorp(),
        tc.etc.iorReason.Iors(),
        tc.etc.iorReason.Iort(),
        tc.etc.iorReason.Ioru(),
        tc.etc.iorReason.Iorf(),
        tc.etc.nParentObjectClass );


    if ( g_rgbBFTemp == pbData )
    {
        g_rgfmp[ pbf->ifmp ].DecrementAsyncIOForViewCache();
    }


    pbf->sxwl.ReleaseWriteLatch();
}


ERR ErrBFISyncWrite( PBF pbf, const BFLatchType bfltHave, OSFILEQOS qos, const TraceContext& tc )
{
    Assert( FBFIUpdatablePage( pbf ) );


    BFIPrepareWritePage( pbf );

    Assert( !FBFIUpdatablePage( pbf ) );

    Assert( qosIODispatchImmediate == ( qos & qosIODispatchMask ) );
    pbf->fLazyIO = ( qosIODispatchImmediate != ( qos & qosIODispatchMask ) );

    AssertRTL( CbBFIBufferSize( pbf ) == CbBFIPageSize( pbf ) );

    IFileAPI *const pfapi       =   g_rgfmp[pbf->ifmp].Pfapi();
    const QWORD     ibOffset    =   OffsetOfPgno( pbf->pgno );
    const DWORD     cbData      =   CbBFIPageSize( pbf );
    BYTE* const     pbData      =   (BYTE*)pbf->pv;
    ERR             err         =   JET_errSuccess;


    err = pfapi->ErrIOWrite( tc,
                                ibOffset,
                                cbData,
                                pbData,
                                qos,
                                NULL,
                                DWORD_PTR( pbf ),
                                IFileAPI::PfnIOHandoff( BFISyncWriteHandoff ) );


    FullTraceContext fullTc;
    fullTc.DeepCopy( GetCurrUserTraceContext().Utc(), tc );
    BFISyncWriteComplete( err, pfapi, fullTc, qos, ibOffset, cbData, pbData, pbf, bfltHave );

    return err;
}

void BFISyncWriteHandoff(   const ERR           err,
                            IFileAPI* const     pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS     grbitQOS,
                            const QWORD         ibOffset,
                            const DWORD         cbData,
                            const BYTE* const   pbData,
                            const PBF           pbf,
                            void* const         pvIOContext )
{
    Assert( JET_errSuccess == err );

    BFISetIOContext( pbf, pvIOContext );
}

void BFISyncWriteComplete(  const ERR           err,
                            IFileAPI* const     pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS     grbitQOS,
                            const QWORD         ibOffset,
                            const DWORD         cbData,
                            const BYTE* const   pbData,
                            const PBF           pbf,
                            const BFLatchType   bfltHave )
{

    BFIResetIOContext( pbf );
    

    BFITraceWritePage( pbf, tc );

    Enforce( CmpLgpos( pbf->lgposModify, g_rgfmp[ pbf->ifmp ].LgposWaypoint() ) <= 0 );


    if ( FBFIDatabasePage( pbf ) )
    {
        if ( err >= JET_errSuccess )
        {
            CPAGE::PageFlushType pgft = CPAGE::pgftUnknown;
            DBTIME dbtime = dbtimeNil;
            const LONG cbBuffer = g_rgcbPageSize[ pbf->icbBuffer ];
            CFlushMap* const pfm = g_rgfmp[ pbf->ifmp ].PFlushMap();
            const BOOL fIsPagePatching = ( tc.etc.iorReason.Iorp() == IOREASONPRIMARY( iorpPatchFix ) );
            const BOOL fIsFmRecoverable = pfm->FRecoverable();

            CPAGE cpage;
            const LONG cbPage = g_rgcbPageSize[ pbf->icbPage ];
            Assert( cbBuffer == cbPage );
            cpage.LoadPage( pbf->ifmp, pbf->pgno, pbf->pv, cbBuffer );
            dbtime = cpage.Dbtime();
            if ( !FUtilZeroed( (BYTE*)pbf->pv, cbBuffer ) && ( dbtime != dbtimeShrunk ) && ( dbtime != dbtimeRevert ) )
            {
                pgft = cpage.Pgft();
            }
            cpage.UnloadPage();

            if ( fIsPagePatching && fIsFmRecoverable )
            {
                (void)pfm->ErrSetPgnoFlushTypeAndWait( pbf->pgno, pgft, dbtime );
            }
            else
            {
                pfm->SetPgnoFlushType( pbf->pgno, pgft, dbtime );
            }
        }
    }


    AssertTrack( pbf->bfbitfield.FRangeLocked(), "BFSyncCompleteRangeNotLocked" );
    g_rgfmp[ pbf->ifmp ].LeaveRangeLock( pbf->pgno, pbf->irangelock );
    pbf->bfbitfield.SetFRangeLocked( fFalse );


    if ( err >= JET_errSuccess )
    {

        OSTrace(    JET_tracetagBufferManagerMaintTasks,
                    OSFormat(   "%s:  [%s:%s] written to disk (dbtime: %s)",
                                __FUNCTION__,
                                OSFormatUnsigned( pbf->ifmp ),
                                OSFormatUnsigned( pbf->pgno ),
                                OSFormatUnsigned( (ULONG_PTR)((DBTIME*)pbf->pv)[ 1 ] ) ) );


        if ( pbf->fOlderVersion )
        {
            BFIMarkAsSuperCold( pbf, fFalse );
        }

        const BOOL  fFlushed        = pbf->fFlushed;
        PERFOptDeclare( TCE         tce             = pbf->tce );

        Assert( pbf->err != errBFIPageNotVerified );
        Assert( pbf->err != errBFIPageRemapNotReVerified );
        BFICleanPage( pbf, bfltHave );

        pbf->fLazyIO = fFalse;


        if ( fFlushed )
        {
            PERFOpt( cBFPagesRepeatedlyWritten.Inc( PinstFromIfmp( pbf->ifmp ), tce ) );
        }
        pbf->fFlushed = fTrue;

        PERFOpt( cBFPagesWritten.Inc( PinstFromIfmp( pbf->ifmp ), tce ) );

    }


    else
    {

        pbf->err = SHORT( err );
        Assert( pbf->err == err );
        Assert( pbf->err != JET_errFileIOBeyondEOF );

        pbf->fLazyIO = fFalse;


        if ( pbf->fFlushed )
        {
            PERFOpt( cBFPagesRepeatedlyWritten.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
        }
        else
        {
            pbf->fFlushed = fTrue;
        }

        PERFOpt( cBFPagesWritten.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );

    }

    if ( tc.etc.iorReason.Iorp() == iorpBFDatabaseFlush )
    {
        g_asigBFFlush.Set();
    }

    Expected( tc.etc.iorReason.Iorp() != iorpBFCheckpointAdv );
    if ( tc.etc.iorReason.Iorp() == iorpBFCheckpointAdv )
    {
        BFIMaintCheckpointDepthRequest( &g_rgfmp[pbf->ifmp], bfcpdmrRequestIOThreshold );
    }

}


ERR ErrBFIAsyncWrite( PBF pbf, OSFILEQOS qos, const TraceContext& tc )
{
    Assert( FBFIUpdatablePage( pbf ) );


    BFIPrepareWritePage( pbf );

    pbf->fLazyIO = ( qosIODispatchImmediate != ( qos & qosIODispatchMask ) );

    Assert( !FBFIUpdatablePage( pbf ) );

    AssertRTL( CbBFIBufferSize( pbf ) == CbBFIPageSize( pbf ) );

    qos |= ( UlParam( PinstFromIfmp( pbf->ifmp ), JET_paramFlight_NewQueueOptions ) & bitUseMetedQ ) ? qosIODispatchWriteMeted : 0x0;


    IFileAPI * const pfapi = g_rgfmp[pbf->ifmp].Pfapi();

    const ERR err = pfapi->ErrIOWrite(  tc,
                                    OffsetOfPgno( pbf->pgno ),
                                    CbBFIPageSize( pbf ),
                                    (BYTE*)pbf->pv,
                                    qos,
                                    IFileAPI::PfnIOComplete( BFIAsyncWriteComplete ),
                                    DWORD_PTR( pbf ),
                                    IFileAPI::PfnIOHandoff( BFIAsyncWriteHandoff ) );
    CallSx( err, errDiskTilt );

    
    if ( errDiskTilt == err )
    {
        Assert( SHORT( wrnBFPageFlushPending ) == pbf->err );
        Assert( pbf->sxwl.FOwnExclusiveLatch() );

        pbf->err = JET_errSuccess;
    }

    return err;
}

void BFIAsyncWriteHandoff(  const ERR           err,
                            IFileAPI* const     pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS     grbitQOS,
                            const QWORD         ibOffset,
                            const DWORD         cbData,
                            const BYTE* const   pbData,
                            const PBF           pbf,
                            void* const         pvIOContext )
{
    Assert( JET_errSuccess == err );

    BFISetIOContext( pbf, pvIOContext );

    Enforce( CmpLgpos( pbf->lgposModify, g_rgfmp[ pbf->ifmp ].LgposWaypoint() ) <= 0 );


    Enforce( wrnBFPageFlushPending == pbf->err );

    AssertRTL( CbBFIBufferSize( pbf ) == CbBFIPageSize( pbf ) );

#ifdef EXTRA_LATCHLESS_IO_CHECKS
    if (    !FBFICacheViewCacheDerefIo( pbf ) &&
            (DWORD)CbBFIBufferSize( pbf ) >= OSMemoryPageCommitGranularity() )
    {
        OSMemoryPageProtect( pbf->pv, CbBFIBufferSize( pbf ) );
    }
#endif


    if( grbitQOS & qosIOOptimizeCombinable )
    {
        PERFOpt( cBFPagesFlushedOpportunely.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );

        if ( bfdfClean == pbf->bfdf )
        {
            PERFOpt( cBFPagesFlushedOpportunelyClean.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
        }
    }
    else
    {
        Expected( bfdfClean != pbf->bfdf );

        switch( tc.etc.iorReason.Iorp() )
        {
            case iorpBFAvailPool:
            case iorpBFShrink:
                if ( pbf->lrukic.FSuperColded() )
                {
                    if ( pbf->fOlderVersion )
                    {
                        PERFOpt( cBFPagesFlushedScavengeSuperColdInternal.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
                    }
                    else
                    {
                        PERFOpt( cBFPagesFlushedScavengeSuperColdUser.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
                    }
                }

                if ( tc.etc.iorReason.Iorp() == iorpBFAvailPool )
                {
                    PERFOpt( cBFPagesFlushedAvailPool.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
                }
                else if ( tc.etc.iorReason.Iorp() == iorpBFShrink )
                {
                    PERFOpt( cBFPagesFlushedCacheShrink.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
                }
                break;

            case iorpBFCheckpointAdv:
                if ( tc.etc.iorReason.Iorf() & iorfForeground )
                {
                    PERFOpt( cBFPagesFlushedCheckpointForeground.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
                }
                else
                {
                    PERFOpt( cBFPagesFlushedCheckpoint.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
                }
                break;

            case iorpBFDatabaseFlush:
                Expected( !( tc.etc.iorReason.Iorf() & iorfForeground ) );

                PERFZeroDisabledAndDiscouraged( cBFPagesFlushedContextFlush.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
                break;

            case iorpBFFilthyFlush:
                Expected( tc.etc.iorReason.Iorf() & iorfForeground );
                PERFOpt( cBFPagesFlushedFilthyForeground.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
                break;

            default:
                AssertSz( fFalse, "Unknown iorp unknown" );
        }

    }

    Assert( !FBFIUpdatablePage( pbf ) );
    Assert( ErrBFIWriteSignalState( pbf ) == wrnBFPageFlushPending );
    Enforce( NULL == pbf->pWriteSignalComplete );

    pbf->sxwl.ReleaseExclusiveLatch();
}


ERR ErrBFIWriteSignalIError( ULONG_PTR pWriteSignal )
{
    Assert( pWriteSignal <= 0xFFFF );
    SHORT errS = (SHORT)( pWriteSignal & 0xFFFF );
    if ( wrnBFIWriteIOComplete == errS )
    {
        return JET_errSuccess;
    }
    return errS;
}

ERR ErrBFIWriteSignalState( const PBF pbf )
{
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );
    Assert( pbf->err == wrnBFPageFlushPending );

    const ULONG_PTR pWriteSignal = pbf->pWriteSignalComplete;
    if ( pbf->err == wrnBFPageFlushPending && pWriteSignal )
    {
        const ERR err = ErrBFIWriteSignalIError( pWriteSignal );
        Assert( wrnBFPageFlushPending != err );
        Assert( wrnBFIWriteIOComplete != err );
        return err;
    }
    return wrnBFPageFlushPending;
}

void BFIWriteSignalSetComplete( PBF pbf, const ERR err )
{

    Assert( pbf->err == wrnBFPageFlushPending );

    Assert( err != wrnBFPageFlushPending );

    ULONG_PTR pSignal;
    if ( err == JET_errSuccess )
    {
        pSignal = 0xFFFF & wrnBFIWriteIOComplete;
    }
    else
    {
        pSignal = 0xFFFF & err;
    }

    const ULONG_PTR pInitial = pbf->pWriteSignalComplete;
    Enforce( NULL == pInitial );

    Assert( ErrBFIWriteSignalIError( pSignal ) == err );

    const ULONG_PTR pBefore = (ULONG_PTR)AtomicCompareExchangePointer( (void**)&(pbf->pWriteSignalComplete), (void*)pInitial, (void*)pSignal );

    Enforce( pBefore == pInitial );
}

void BFIWriteSignalReset( const PBF pbf )
{
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );
    Assert( pbf->err == wrnBFPageFlushPending );

    const ULONG_PTR pInitial = pbf->pWriteSignalComplete;
    const ULONG_PTR pBefore = (ULONG_PTR)AtomicCompareExchangePointer( (void**)&(pbf->pWriteSignalComplete), (void*)pInitial, NULL );
    Enforce( pInitial == pBefore );
}

JETUNITTEST( BF, LocklessWriteCompleteSignaling )
{
    BF bfLocal;
    PBF pbf = &bfLocal;
    pbf->sxwl.ClaimOwnership( bfltWrite );

    CHECK( 0 == CmpLgpos( &(bfLocal.lgposOldestBegin0), &lgposMax ) );

    ERR err;

    CHECK( JET_errSuccess == pbf->err );
    CHECK( 0 == pbf->pWriteSignalComplete );

    err = wrnBFPageFlushPending;
    pbf->err = SHORT( err );

    CHECK( wrnBFPageFlushPending == ErrBFIWriteSignalState( pbf ) );

    BFIWriteSignalSetComplete( pbf, JET_errSuccess );
    CHECK( JET_errSuccess == ErrBFIWriteSignalState( pbf ) );
    BFIWriteSignalReset( pbf );

    BFIWriteSignalSetComplete( pbf, 4007  );
    CHECK( 4007 == ErrBFIWriteSignalState( pbf ) );
    BFIWriteSignalReset( pbf );

    BFIWriteSignalSetComplete( pbf, JET_errDiskIO );
    CHECK( JET_errDiskIO == ErrBFIWriteSignalState( pbf ) );
    BFIWriteSignalReset( pbf );
    pbf->sxwl.ReleaseOwnership( bfltWrite );
}

#ifdef DEBUG
void * g_pvIoThreadImageCheckCache = NULL;
#endif

#ifdef DEBUG

QWORD g_cRemapsConsidered = 0;
QWORD g_cRemapsSkippedByLatchContention = 0;

QWORD g_cRemapsSuccessful = 0;
QWORD g_cRemapsOfViewMapped = 0;
QWORD g_cRemapsOfPageAlloc = 0;

QWORD g_cRemapsFailed = 0;

QWORD g_cRemapsNonResident = 0;
#endif

BOOL FBFICacheRemapPage( __inout PBF pbf, IFileAPI* const pfapi )
{
    ERR errReRead = JET_errSuccess;
    ERR errCheckPage = JET_errSuccess;

    VOID * pvFreshMapPage = NULL;

    Assert( UlParam( JET_paramEnableViewCache ) );
    Assert( pbf->sxwl.FOwnWriteLatch() );
    Assert( pbf->bfat == bfatViewMapped || pbf->bfat == bfatPageAlloc );
    Assert( pfapi == g_rgfmp[pbf->ifmp].Pfapi() );


    if ( !pbf->fCurrentVersion )
    {
        return fFalse;
    }


    if ( pbf->bfat == bfatViewMapped )
    {
        Assert( FOSMemoryFileMapped( pbf->pv, g_rgcbPageSize[pbf->icbPage] ) );
        Assert( pbf->bfdf == bfdfClean || FOSMemoryFileMappedCowed( pbf->pv, g_rgcbPageSize[pbf->icbPage] ) );
    }
    else
    {
        Assert( pbf->bfat == bfatPageAlloc );
        Assert( FOSMemoryPageAllocated( pbf->pv, g_rgcbPageSize[pbf->icbPage] ) );
        Assert( !FOSMemoryFileMapped( pbf->pv, g_rgcbPageSize[pbf->icbPage] ) );
    }


    INT rgiulpOsMmPageMarkers[8];
    ULONG_PTR rgulpOsMmPageMarkers[8];
    const CPAGE::PGHDR * const ppghdrPre    = (CPAGE::PGHDR*)pbf->pv;
    const XECHECKSUM xechkCheckPre          = ppghdrPre->checksum;
    const DBTIME dbtimeCheckPre             = ppghdrPre->dbtimeDirtied;
    const PGNO pgnoCheckPre                 = ( pbf->icbPage <= icbPage8KB ) ? pbf->pgno : ( ((CPAGE::PGHDR2 *)ppghdrPre)->pgno );
    Assert( pgnoCheckPre == pbf->pgno );

    Assert( g_rgcbPageSize[pbf->icbPage] % OSMemoryPageCommitGranularity() == 0 );
    Assert( OSMemoryPageCommitGranularity() % sizeof( rgulpOsMmPageMarkers[0] ) == 0 );
    const INT cosmmpg = g_rgcbPageSize[pbf->icbPage] / OSMemoryPageCommitGranularity();
    const INT culpPerBlock = OSMemoryPageCommitGranularity() / sizeof( rgulpOsMmPageMarkers[0] );
    AssertPREFIX( _countof(rgiulpOsMmPageMarkers) >= cosmmpg );
    AssertPREFIX( _countof(rgulpOsMmPageMarkers) >= cosmmpg );
    for ( INT iosmmpage = 0; iosmmpage < cosmmpg; iosmmpage++ )
    {
        ULONG_PTR * pulpBlock = (ULONG_PTR*)( (BYTE*)pbf->pv + ( iosmmpage * OSMemoryPageCommitGranularity() ) );
        INT iulp;
        for ( iulp = 0; iulp < culpPerBlock; iulp++ )
        {
            if ( pulpBlock[iulp] != 0 )
            {
                rgiulpOsMmPageMarkers[iosmmpage] = iulp;
                rgulpOsMmPageMarkers[iosmmpage] = pulpBlock[iulp];
                break;
            }
        }
        if( iulp == culpPerBlock )
        {
            rgiulpOsMmPageMarkers[iosmmpage] = 0;
            Expected( pulpBlock[rgiulpOsMmPageMarkers[iosmmpage]] == 0 );
            rgulpOsMmPageMarkers[iosmmpage] = pulpBlock[rgiulpOsMmPageMarkers[iosmmpage]];
        }
        AssertSz( pulpBlock[rgiulpOsMmPageMarkers[iosmmpage]] == rgulpOsMmPageMarkers[iosmmpage],
                    "These should be equal, we _just_ set them: %I64x == %I64x",
                    pulpBlock[rgiulpOsMmPageMarkers[iosmmpage]], rgulpOsMmPageMarkers[iosmmpage] );
    }
    
#ifdef DEBUG
    void * pvPageImageCheckPre = NULL;
    if ( FIOThread() )
    {
        if ( g_pvIoThreadImageCheckCache == NULL )
        {
            Expected( pbf->icbPage == g_icbCacheMax );
            Assert( pbf->icbPage <= icbPageBiggest );
            g_pvIoThreadImageCheckCache = PvOSMemoryPageAlloc( g_rgcbPageSize[icbPageBiggest], NULL );
        }
        pvPageImageCheckPre = g_pvIoThreadImageCheckCache;
    }
    if ( pvPageImageCheckPre == NULL )
    {
        pvPageImageCheckPre = PvOSMemoryPageAlloc( g_rgcbPageSize[icbPageBiggest], NULL );
    }
    if ( pvPageImageCheckPre )
    {
        memcpy( pvPageImageCheckPre, pbf->pv, g_rgcbPageSize[pbf->icbPage] );
    }
#endif

    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );



    Assert( pbf->bfat == bfatViewMapped || pbf->bfat == bfatPageAlloc );
    const ERR errMapOp = ( pbf->bfat == bfatPageAlloc ) ?
                pfapi->ErrMMCopy( OffsetOfPgno( pbf->pgno ), g_rgcbPageSize[pbf->icbPage], &pvFreshMapPage ) :
                pfapi->ErrMMRevert( OffsetOfPgno( pbf->pgno ), pbf->pv, g_rgcbPageSize[pbf->icbPage] );
    const BOOL fRemappedRet = ( errMapOp >= JET_errSuccess );

    FOSSetCleanupState( fCleanUpStateSaved );

    if ( pbf->bfat == bfatPageAlloc )
    {
        if ( errMapOp >= JET_errSuccess )
        {
            Enforce( fRemappedRet );
            Enforce( pvFreshMapPage );
            Assert( pbf->pv != pvFreshMapPage );


            OSMemoryPageFree( pbf->pv );
            pbf->pv = pvFreshMapPage;
            pbf->bfat = bfatViewMapped;
        }
        else
        {
            Assert( !fRemappedRet );
            Assert( pvFreshMapPage == NULL );
            Expected( ppghdrPre == (CPAGE::PGHDR*)pbf->pv );
        }
    }
    else
    {
        Expected( ppghdrPre == (CPAGE::PGHDR*)pbf->pv );
    }

    if ( fRemappedRet )
    {
        Assert( FOSMemoryFileMapped( pbf->pv, g_rgcbPageSize[pbf->icbPage] ) );
        Assert( !FOSMemoryFileMappedCowed( pbf->pv, g_rgcbPageSize[pbf->icbPage] ) );


#ifdef DEBUG

        AtomicAdd( &g_cRemapsSuccessful, 1 );
        if ( pvFreshMapPage )
        {
            AtomicAdd( &g_cRemapsOfPageAlloc, 1 );
        }
        else
        {
            AtomicAdd( &g_cRemapsOfViewMapped, 1 );
        }

        if ( !FOSMemoryPageResident( pbf->pv, CbBFIBufferSize( pbf ) ) )
        {
            AtomicAdd( &g_cRemapsNonResident, 1 );
        }
#endif
        Expected( CbBFIBufferSize( pbf ) == CbBFIPageSize( pbf ) );
        errReRead = g_rgfmp[pbf->ifmp].Pfapi()->ErrMMIORead( OffsetOfPgno( pbf->pgno ),
                                    (BYTE*)pbf->pv,
                                    CbBFIBufferSize( pbf ),
                                    IFileAPI::FileMmIoReadFlag( IFileAPI::fmmiorfKeepCleanMapped | IFileAPI::fmmiorfPessimisticReRead ) );
    }
    else
    {
        Assert( FOSMemoryFileMapped( pbf->pv, g_rgcbPageSize[pbf->icbPage] ) );
        Assert( pbf->bfdf == bfdfClean || FOSMemoryFileMappedCowed( pbf->pv, g_rgcbPageSize[pbf->icbPage] ) );
        OnDebug( AtomicAdd( &g_cRemapsFailed, 1 ) );
    }



    if ( errReRead < JET_errSuccess )
    {
        Enforce( fRemappedRet );
    }
    else
    {
        const CPAGE::PGHDR * const ppghdrPost = (CPAGE::PGHDR*)pbf->pv;
        const PGNO pgnoCheckPost = ( pbf->icbPage <= icbPage8KB ) ? pbf->pgno : ( ((CPAGE::PGHDR2 *)ppghdrPost)->pgno );

        BOOL fMarkersSame = fTrue;
        for ( INT iosmmpage2 = 0; iosmmpage2 < cosmmpg; iosmmpage2++ )
        {
            ULONG_PTR * pulpBlock = (ULONG_PTR*)( (BYTE*)pbf->pv + ( iosmmpage2 * OSMemoryPageCommitGranularity() ) );
            fMarkersSame = fMarkersSame && ( pulpBlock[rgiulpOsMmPageMarkers[iosmmpage2]] == rgulpOsMmPageMarkers[iosmmpage2] );
            Assert( fMarkersSame );
        }


        if ( FBFIDatabasePage( pbf ) ||
                xechkCheckPre != ppghdrPost->checksum ||
                pgnoCheckPre != pgnoCheckPost ||
                xechkCheckPre != ppghdrPost->checksum ||
                dbtimeCheckPre != ppghdrPost->dbtimeDirtied ||
                !fMarkersSame )
        {
            const BFLatch bfl = { pbf->pv, (DWORD_PTR)pbf };
            CPAGE cpage;
            Assert( CbBFIBufferSize( pbf ) == CbBFIPageSize( pbf ) );
            cpage.ReBufferPage( bfl, pbf->ifmp, pbf->pgno, pbf->pv, g_rgcbPageSize[pbf->icbPage] );

            CPageValidationLogEvent validationaction( pbf->ifmp, CPageValidationLogEvent::LOG_NONE, BUFFER_MANAGER_CATEGORY );

            errCheckPage = cpage.ErrValidatePage( pgvfExtensiveChecks | pgvfDoNotCheckForLostFlush, &validationaction );
        }

        if ( errCheckPage >= JET_errSuccess || !fRemappedRet )
        {

            
            Enforce( xechkCheckPre == ppghdrPost->checksum );
            Enforce( dbtimeCheckPre == ppghdrPost->dbtimeDirtied );
            Enforce( pgnoCheckPre == pgnoCheckPost );
            Enforce( fMarkersSame );
        }
    }

#ifdef DEBUG


    if ( errReRead >= JET_errSuccess &&
        pvPageImageCheckPre )
    {
        const CPAGE::PGHDR * const ppghdrPreCopy = (CPAGE::PGHDR*)pvPageImageCheckPre;
        AssertSz( xechkCheckPre == ppghdrPreCopy->checksum, "Remap checksum mismatch: 0x%I64x != 0x%I64x, errs = %d / %d",
                    xechkCheckPre, (XECHECKSUM)ppghdrPreCopy->checksum, errReRead, errCheckPage );
        AssertSz( dbtimeCheckPre == ppghdrPreCopy->dbtimeDirtied, "Remap dbtime mismatch: %I64d != %I64d, errs = %d / %d",
                    dbtimeCheckPre, (DBTIME)ppghdrPreCopy->dbtimeDirtied, errReRead, errCheckPage );
        AssertSz( 0 == memcmp( pvPageImageCheckPre, pbf->pv, g_rgcbPageSize[pbf->icbPage] ), "Remap page image mismatch: %I64x != %I64x, errs = %d / %d",
                    pvPageImageCheckPre, pbf->pv, errReRead, errCheckPage );
        if ( pvPageImageCheckPre != g_pvIoThreadImageCheckCache )
        {
            OSMemoryPageFree( pvPageImageCheckPre );
        }
    }

    if ( errReRead >= JET_errSuccess &&
        FBFIDatabasePage( pbf ) )
    {
        CPAGE cpage;
        CPageValidationNullAction nullaction;
        cpage.LoadPage( pbf->ifmp, pbf->pgno, pbf->pv, g_rgcbPageSize[pbf->icbPage] );
        CallS( cpage.ErrValidatePage( pgvfDoNotCheckForLostFlush, &nullaction ) );
    }
#endif

    return fRemappedRet;
}

void BFIAsyncWriteComplete( const ERR           err,
                            IFileAPI* const     pfapi,
                            const FullTraceContext& tc,
                            const OSFILEQOS     grbitQOS,
                            const QWORD         ibOffset,
                            const DWORD         cbData,
                            const BYTE* const   pbData,
                            const PBF           pbf )
{
    const IFMP  ifmp    = pbf->ifmp;
    FMP * const pfmp    = &g_rgfmp[ ifmp ];

    Assert( pbf->err == wrnBFPageFlushPending );
    Expected( err <= JET_errSuccess );


    BFIResetIOContext( pbf );


    BFITraceWritePage( pbf, tc );

    Assert( CmpLgpos( pbf->lgposModify, pfmp->LgposWaypoint() ) <= 0 );
    Enforce( CmpLgpos( pbf->lgposModify, pfmp->LgposWaypoint() ) <= 0 );


    if ( FBFIDatabasePage( pbf ) )
    {
        if ( err >= JET_errSuccess )
        {
            CPAGE::PageFlushType pgft = CPAGE::pgftUnknown;
            DBTIME dbtime = dbtimeNil;
            const LONG cbBuffer = g_rgcbPageSize[ pbf->icbBuffer ];
            CFlushMap* const pfm = g_rgfmp[ pbf->ifmp ].PFlushMap();
            OnDebug( const BOOL fIsPagePatching = ( tc.etc.iorReason.Iorp() == IOREASONPRIMARY( iorpPatchFix ) ) );
            OnDebug( const BOOL fIsFmRecoverable = pfm->FRecoverable() );

            CPAGE cpage;
            const LONG cbPage = g_rgcbPageSize[ pbf->icbPage ];
            Assert( cbBuffer == cbPage );
            cpage.LoadPage( pbf->ifmp, pbf->pgno, pbf->pv, cbBuffer );
            dbtime = cpage.Dbtime();
            if ( !FUtilZeroed( (BYTE*)pbf->pv, cbBuffer ) && ( dbtime != dbtimeShrunk ) && ( dbtime != dbtimeRevert ) )
            {
                pgft = cpage.Pgft();
            }
            cpage.UnloadPage();

            Assert( !( fIsPagePatching && fIsFmRecoverable ) );
            pfm->SetPgnoFlushType( pbf->pgno, pgft, dbtime );
        }
    }


    BOOL fRemappedWriteLatched = fFalse;
    if ( err >= JET_errSuccess &&
            ( pbf->bfat == bfatViewMapped || pbf->bfat == bfatPageAlloc ) )
    {
        
        OnDebug( AtomicAdd( &g_cRemapsConsidered, 1 ) );

        if ( pbf->sxwl.ErrTryAcquireWriteLatch() == CSXWLatch::ERR::errSuccess )
        {
            if ( FBFICacheRemapPage( pbf, pfapi ) )
            {
                fRemappedWriteLatched = fTrue;
            }
            else
            {
                pbf->sxwl.ReleaseWriteLatch();
                Assert( !fRemappedWriteLatched );
            }
        }
        else
        {
            OnDebug( AtomicAdd( &g_cRemapsSkippedByLatchContention, 1 ) );
        }
    }
    Assert( fRemappedWriteLatched || pbf->sxwl.FNotOwner() );
    Assert( !fRemappedWriteLatched || pbf->sxwl.FOwnWriteLatch() );


    AssertTrack( pbf->bfbitfield.FRangeLocked(), "BFAsyncCompleteRangeNotLocked" );
    pfmp->LeaveRangeLock( pbf->pgno, pbf->irangelock );
    pbf->bfbitfield.SetFRangeLocked( fFalse );


    if ( err >= JET_errSuccess )
    {
        OSTrace(    JET_tracetagBufferManager,
                    OSFormat(   "%s:  [%s:%s] written to disk (dbtime: %s)",
                                __FUNCTION__,
                                OSFormatUnsigned( pbf->ifmp ),
                                OSFormatUnsigned( pbf->pgno ),
                                OSFormatUnsigned( (ULONG_PTR)((DBTIME*)pbf->pv)[ 1 ] ) ) );


        if ( pbf->fOlderVersion )
        {
            BFIMarkAsSuperCold( pbf, fFalse );
        }
    }


    if ( pbf->fFlushed )
    {
        PERFOpt( cBFPagesRepeatedlyWritten.Inc( PinstFromIfmp( ifmp ), pbf->tce ) );
    }

    if ( grbitQOS & qosIOOptimizeCombinable )
    {
        PERFOpt( cBFPagesCoalescedWritten.Inc( PinstFromIfmp( ifmp ), pbf->tce ) );
    }

    PERFOpt( cBFPagesWritten.Inc( PinstFromIfmp( ifmp ), pbf->tce ) );

#ifdef ENABLE_CLEAN_PAGE_OVERWRITE
    if ( pbf->fSuspiciouslySlowRead )
    {

        WCHAR           wszPgno[ 64 ];
        OSStrCbFormatW( wszPgno, sizeof(wszPgno), L"%d", pbf->pgno );


        const WCHAR * rgwsz [2] = { wszPgno, g_rgfmp[ifmp].WszDatabaseName() };

        UtilReportEvent(
                eventWarning,
                BUFFER_MANAGER_CATEGORY,
                SUSPECTED_BAD_BLOCK_OVERWRITE_ID,
                _countof( rgwsz ), rgwsz );
    }
#endif

    Assert( !fRemappedWriteLatched || err >= JET_errSuccess );


    ERR errSignal = ( err < JET_errSuccess ) ?
                        ( err ) :
                        ( fRemappedWriteLatched ? errBFIPageRemapNotReVerified : err );

    BFIWriteSignalSetComplete( pbf, errSignal );


    if ( fRemappedWriteLatched )
    {
        Assert( ErrBFIWriteSignalState( pbf ) == errBFIPageRemapNotReVerified );

        pbf->sxwl.ReleaseWriteLatch();
    }



    if ( tc.etc.iorReason.Iorp() == iorpBFCheckpointAdv &&
         ( ( err < JET_errSuccess ) || ( grbitQOS & qosIOCompleteWriteGameOn ) ) )
    {
        BFIMaintCheckpointDepthRequest( &g_rgfmp[ifmp], bfcpdmrRequestIOThreshold );
    }


    if ( tc.etc.iorReason.Iorp() == iorpBFAvailPool )
    {
        CallS( ErrBFIMaintAvailPoolRequest( bfmaprtAsync ) );
    }

    if ( tc.etc.iorReason.Iorp() == iorpBFDatabaseFlush )
    {
        g_asigBFFlush.Set();
    }
}


void BFIFlushComplete( _Inout_ const PBF pbf, _In_ const BFLatchType bfltHave, _In_ const BOOL fUnencumberedPath, _In_ const BOOL fCompleteRemapReVerify, _In_ const BOOL fAllowTearDownClean )
{
    const IFMP  ifmp    = pbf->ifmp;
    FMP * const pfmp    = &g_rgfmp[ ifmp ];

    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );


    Assert( wrnBFPageFlushPending == pbf->err );
    Assert( !FBFIUpdatablePage( pbf ) );
    Assert( pbf->pWriteSignalComplete <= 0xFFFF );

    ERR err = ErrBFIWriteSignalState( pbf );
    BFIWriteSignalReset( pbf );

    Assert( wrnBFPageFlushPending != err );

#ifdef EXTRA_LATCHLESS_IO_CHECKS


    if ( !FBFIBufferIsZeroed( pbf ) )
    {
        CallS( ErrBFIVerifyPageSimplyWork( pbf ) );
    }

    AssertRTL( CbBFIBufferSize( pbf ) == CbBFIPageSize( pbf ) );

    if (    !FBFICacheViewCacheDerefIo( pbf ) &&
            (DWORD)CbBFIBufferSize( pbf ) >= OSMemoryPageCommitGranularity() )
    {
        OSMemoryPageUnprotect( pbf->pv, CbBFIBufferSize( pbf ) );
    }
#endif

    Assert( CmpLgpos( pbf->lgposModify, pfmp->LgposWaypoint() ) <= 0 );


    if ( err >= JET_errSuccess || err == errBFIPageRemapNotReVerified )
    {

        Assert( err == JET_errSuccess || err == errBFIPageRemapNotReVerified || err == wrnBFPageFlushPending );
        Expected( pbf->err >= JET_errSuccess );
        Expected( pbf->err == JET_errSuccess || pbf->err == wrnBFPageFlushPending );


        const BFCleanFlags bfcf = BFCleanFlags( ( fUnencumberedPath ? bfcfAllowReorganization : bfcfNone ) | ( fAllowTearDownClean ? bfcfAllowTearDownClean : bfcfNone ) );
        BFICleanPage( pbf, bfltHave, bfcf );


        if ( err == errBFIPageRemapNotReVerified )
        {
            pbf->err = SHORT( errBFIPageRemapNotReVerified );
        }

        if ( fCompleteRemapReVerify  &&
                pbf->err == errBFIPageRemapNotReVerified )
        {
            TraceContextScope tcScope( iorpBFRemapReVerify );
            err = ErrBFIValidatePage( pbf, bfltHave, CPageValidationLogEvent::LOG_ALL & ~CPageValidationLogEvent::LOG_UNINIT_PAGE, *tcScope );
            Assert( pbf->err != errBFIPageRemapNotReVerified );
        }
    }


    else
    {

        pbf->err = SHORT( err );
        Assert( pbf->err == err );
        Assert( pbf->err != JET_errFileIOBeyondEOF );
    }


    pbf->fFlushed = fTrue;

}




CCriticalSection    g_critBFDepend( CLockBasicInfo( CSyncBasicInfo( szBFDepend ), rankBFDepend, 0 ) );


void BFISetLgposOldestBegin0( PBF pbf, LGPOS lgpos, const TraceContext& tc )
{
    LGPOS lgposOldestBegin0Last = lgposMax;
    
    Assert( pbf->sxwl.FOwnWriteLatch() ||
            ( pbf->sxwl.FOwnExclusiveLatch() && pbf->fWARLatch ) );
    Assert( pbf->bfdf >= bfdfDirty );
    Assert( FBFIUpdatablePage( pbf ) );


    BFIDirtyPage( pbf, bfdfDirty, tc );


    LGPOS lgposOldestBegin0 = pbf->lgposOldestBegin0;


    if ( CmpLgpos( &lgposOldestBegin0, &lgpos ) > 0 )
    {
        BFIResetLgposOldestBegin0( pbf, fTrue );
    }

    FMP* pfmp = &g_rgfmp[ pbf->ifmp ];


    if ( CmpLgpos( &lgposOldestBegin0, &lgpos ) > 0 )
    {
        pfmp->EnterBFContextAsReader();

        OnDebug( pfmp->Pinst()->m_plog->CbLGOffsetLgposForOB0( lgpos, lgpos ) );

        BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext();
        Assert( pbffmp && pbffmp->fCurrentlyAttached );

#ifdef DEBUG
        if ( pfmp->Pinst()->m_plog->FRecovering() )
        {
            BFIAssertReqRangeConsistentWithLgpos( pfmp, lgpos, lgposMin, "SetLgposOB0" );
        }
#endif


        pbf->lgposOldestBegin0 = lgpos;


        BFOB0::CLock lock;
        pbffmp->bfob0.LockKeyPtr( BFIOB0Offset( pbf->ifmp, &lgpos ), pbf, &lock );

        BFOB0::ERR errOB0 = pbffmp->bfob0.ErrInsertEntry( &lock, pbf );

        pbffmp->bfob0.UnlockKeyPtr( &lock );


        if ( errOB0 != BFOB0::ERR::errSuccess )
        {
            Assert( errOB0 == BFOB0::ERR::errOutOfMemory ||
                    errOB0 == BFOB0::ERR::errKeyRangeExceeded );


            pbf->fInOB0OL = fTrue;

            pbffmp->critbfob0ol.Enter();
            pbffmp->bfob0ol.InsertAsNextMost( pbf );
            pbffmp->critbfob0ol.Leave();
        }


        lgposOldestBegin0Last = pbffmp->lgposOldestBegin0Last;

        pfmp->LeaveBFContextAsReader();
    }


    if ( CmpLgpos( lgposOldestBegin0Last, lgposMax ) )
    {
        
        LOG* const      plog                    = pfmp->Pinst()->m_plog;
        const LGPOS     lgposNewest             = plog->LgposLGLogTipNoLock();

        const ULONG_PTR cbCheckpointDepthMax    = pfmp->Pinst()->m_fCheckpointQuiesce ? 0 : pfmp->Pinst()->m_plog->CbLGDesiredCheckpointDepth();
        const ULONG_PTR cbCheckpointDepth       = (ULONG_PTR)plog->CbLGOffsetLgposForOB0( lgposNewest, lgposOldestBegin0Last );

        if ( cbCheckpointDepth > cbCheckpointDepthMax )
        {
            BFIMaintCheckpointDepthRequest( pfmp, bfcpdmrRequestOB0Movement );
        }
    }

    OSTrace(    JET_tracetagBufferManager,
                OSFormat(   "%s:  [%s:%s] lgposOldestBegin0 %s -> %s",
                            __FUNCTION__,
                            OSFormatUnsigned( pbf->ifmp ),
                            OSFormatUnsigned( pbf->pgno ),
                            OSFormatLgpos( lgposOldestBegin0 ),
                            OSFormatLgpos( lgpos ) ) );
}

void BFIResetLgposOldestBegin0( PBF pbf, BOOL fCalledFromSet )
{
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );


    LGPOS lgposOldestBegin0 = pbf->lgposOldestBegin0;


    if ( CmpLgpos( &lgposOldestBegin0, &lgposMax ) )
    {
        FMP* pfmp = &g_rgfmp[ pbf->ifmp ];
        pfmp->EnterBFContextAsReader();

        BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext();
        Assert( pbffmp );

        if ( pbf->fInOB0OL )
        {
            pbf->fInOB0OL = fFalse;

            pbffmp->critbfob0ol.Enter();
            pbffmp->bfob0ol.Remove( pbf );
            pbffmp->critbfob0ol.Leave();
        }
        else
        {
            BFOB0::CLock lock;
            pbffmp->bfob0.LockKeyPtr( BFIOB0Offset( pbf->ifmp, &lgposOldestBegin0 ), pbf, &lock );

            BFOB0::ERR errOB0 = pbffmp->bfob0.ErrDeleteEntry( &lock );
            Assert( errOB0 == BFOB0::ERR::errSuccess );

            pbffmp->bfob0.UnlockKeyPtr( &lock );
        }

        pfmp->LeaveBFContextAsReader();

        pbf->lgposOldestBegin0 = lgposMax;
    }

    if ( !fCalledFromSet && CmpLgpos( &lgposOldestBegin0, &lgposMax ) )
    {
        OSTrace(    JET_tracetagBufferManager,
                    OSFormat(   "%s:  [%s:%s] lgposOldestBegin0 %s -> %s",
                                __FUNCTION__,
                                OSFormatUnsigned( pbf->ifmp ),
                                OSFormatUnsigned( pbf->pgno ),
                                OSFormatLgpos( lgposOldestBegin0 ),
                                OSFormatLgpos( lgposMax ) ) );
    }
}

void BFISetLgposModifyEx( PBF pbf, const LGPOS lgpos )
{

    FMP* pfmp = &g_rgfmp[ pbf->ifmp ];
    Assert( pfmp );

#ifdef DEBUG
    if ( ( pfmp->Pinst() != pinstNil ) && pfmp->Pinst()->m_plog->FRecovering() )
    {
        BFIAssertReqRangeConsistentWithLgpos( pfmp, lgposMax, lgpos, "SetLgposModify" );
    }
#endif

    LGPOS lgposOld;
    if ( CmpLgpos( lgpos, lgposMin ) == 0 )
    {
        Assert( pbf->prceUndoInfoNext == prceNil );
        lgposOld = pbf->lgposModify.LgposAtomicExchange( lgpos );
    }
    else
    {
        lgposOld = pbf->lgposModify.LgposAtomicExchangeMax( lgpos );
        Assert( CmpLgpos( pbf->lgposModify.LgposAtomicRead(), lgpos ) >= 0 );
    }

    if ( lgposOld.lGeneration != lgpos.lGeneration )
    {
        const TLS* const pTLS = Ptls();
        bool fLocalLock = false;
        if ( !pTLS->PFMP() )
        {
            CLockDeadlockDetectionInfo::DisableOwnershipTracking();
            CLockDeadlockDetectionInfo::DisableDeadlockDetection();
            pfmp->EnterBFContextAsReader();
            CLockDeadlockDetectionInfo::EnableDeadlockDetection();
            CLockDeadlockDetectionInfo::EnableOwnershipTracking();
            fLocalLock = true;
        }
        else
        {
            Assert( pTLS->PFMP() == pfmp );
        }

        BFFMPContext* pbffmp = ( BFFMPContext* )pfmp->DwBFContext();
        Assert( pbffmp );
        pbffmp->m_logHistModify.Update( lgposOld, lgpos, pbf->ifmp );

        if ( fLocalLock )
        {
            CLockDeadlockDetectionInfo::DisableOwnershipTracking();
            CLockDeadlockDetectionInfo::DisableDeadlockDetection();
            pfmp->LeaveBFContextAsReader();
            CLockDeadlockDetectionInfo::EnableDeadlockDetection();
            CLockDeadlockDetectionInfo::EnableOwnershipTracking();
        }
    }
}

void BFISetLgposModify( PBF pbf, LGPOS lgpos )
{


    if ( CmpLgpos( &pbf->lgposModify, &lgpos ) < 0 )
    {
        Assert( ( pbf->bfdf >= bfdfDirty ) && ( pbf->bfdf < bfdfMax ) );


        BFISetLgposModifyEx( pbf, lgpos );

        BFITraceSetLgposModify( pbf, lgpos );
    }
}

void BFIResetLgposModify( PBF pbf )
{
    BFISetLgposModifyEx( pbf, lgposMin );
}

void BFIAddUndoInfo( PBF pbf, RCE* prce, BOOL fMove )
{
    Assert( pbf->sxwl.FOwnExclusiveLatch() || pbf->sxwl.FOwnWriteLatch() );


    prce->AddUndoInfo( pbf->pgno, pbf->prceUndoInfoNext, fMove );


    pbf->prceUndoInfoNext = prce;
}

void BFIRemoveUndoInfo( PBF pbf, RCE* prce, LGPOS lgposModify, BOOL fMove )
{

    BFISetLgposModify( pbf, lgposModify );


    if ( pbf->prceUndoInfoNext == prce )
    {
        pbf->prceUndoInfoNext = prce->PrceUndoInfoNext();
    }


    prce->RemoveUndoInfo( fMove );
}


BFReserveAvailPages::BFReserveAvailPages( const CPG cpgWanted )
{
    const LONG cbfAvail = (LONG)g_bfavail.Cobject();
    const LONG cpgReserved = s_cpgReservedTotal;
    
    m_cpgReserved = min( max( ( cbfAvail - cpgReserved ) / 2 , 0 ), cpgWanted );
    Assert( m_cpgReserved <= cpgWanted );
    Assert( m_cpgReserved >= 0 );
    AtomicExchangeAdd( &s_cpgReservedTotal, m_cpgReserved );
}


BFReserveAvailPages::~BFReserveAvailPages()
{
    AtomicExchangeAdd( &s_cpgReservedTotal, -m_cpgReserved );
}


CPG BFReserveAvailPages::CpgReserved() const
{
    Assert( m_cpgReserved >= 0 );
    return m_cpgReserved;
}

LONG BFReserveAvailPages::s_cpgReservedTotal = 0;

VOID BFLogHistogram::Update( const LGPOS lgposOld, const LGPOS lgposNew, IFMP ifmp )
{
    Assert( lgposOld.lGeneration != lgposNew.lGeneration );

    INT iGroup = m_ms.Enter();
    Assert( 0 == iGroup || 1 == iGroup );

    LogHistData* pData = &m_rgdata[ iGroup ];

    if ( CmpLgpos( &lgposMin, &lgposOld ) )
    {
        Assert( lgposOld.lGeneration < pData->m_lgenBase + pData->m_cgen );

        const LONG iOld = lgposOld.lGeneration - pData->m_lgenBase;
        if ( pData->m_rgc && 0 <= iOld )
        {
            AtomicDecrement( &pData->m_rgc[ iOld ] );
        }
        else
        {
            AtomicDecrement( &pData->m_cOverflow );
        }
    }

    if ( CmpLgpos( &lgposMin, &lgposNew ) )
    {
        while ( pData->m_lgenBase + pData->m_cgen <= lgposNew.lGeneration )
        {
            m_ms.Leave( iGroup );

            if ( m_crit.FTryEnter() )
            {
                const INT iGroupActive = m_ms.GroupActive();
                LogHistData* const pDataActive = &m_rgdata[ iGroupActive ];

                if ( pDataActive->m_lgenBase + pDataActive->m_cgen <= lgposNew.lGeneration )
                {

                    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

                    ReBase( ifmp, lgposNew.lGeneration );

                    FOSSetCleanupState( fCleanUpStateSaved );
                }

                m_crit.Leave();
            }
            else
            {
                UtilSleep( 1 );
            }

            iGroup = m_ms.Enter();
            pData = &m_rgdata[ iGroup ];
        }

        LONG idxData = lgposNew.lGeneration - pData->m_lgenBase;
        if ( pData->m_rgc && 0 <= idxData )
        {
            AtomicIncrement( &pData->m_rgc[ idxData ] );
        }
        else
        {
            AtomicIncrement( &pData->m_cOverflow );
        }
    }

    m_ms.Leave( iGroup );
}

VOID BFLogHistogram::ReBase( IFMP ifmp, LONG lgenLatest )
{
    Assert( m_crit.FOwner() );

    LOG* const plog = PinstFromIfmp( ifmp )->m_plog;
    const LONG igen = plog->LgposGetCheckpoint().le_lGeneration;
    const LONG cgen = plog->LgposLGLogTipNoLock().lGeneration - igen + 1;
    const LONG cgenActual = LNextPowerOf2( cgen + cgenNewMin );

    Assert( cgenActual > 0 );

    INT iGroup = m_ms.GroupActive();
    Assert( 0 == iGroup || 1 == iGroup );

    LogHistData* const pDataOld = &m_rgdata[ iGroup ];
    LogHistData* const pDataNew = &m_rgdata[ 1 - iGroup ];

    if ( pDataNew->m_rgc && cgenActual <= pDataNew->m_cgen )
    {
    }
    else
    {
        delete[] pDataNew->m_rgc;
        if ( pDataNew->m_rgc = new LONG[ cgenActual ] )
        {
            pDataNew->m_cgen = cgenActual;
        }
        else
        {
            pDataNew->m_cgen = 0;
        }
    }
    memset( pDataNew->m_rgc, 0, sizeof( pDataNew->m_rgc[ 0 ] ) * pDataNew->m_cgen );
    pDataNew->m_cOverflow = 0;

    if ( pDataOld->m_rgc && pDataNew->m_rgc )
    {
        pDataNew->m_lgenBase = max( igen, pDataOld->m_lgenBase );
    }
    else
    {
        Assert( pDataOld->m_lgenBase + pDataOld->m_cgen <= lgenLatest );
        pDataNew->m_lgenBase = lgenLatest + ( pDataNew->m_rgc ? 0 : 1 );
    }

    m_ms.Partition();

    if ( pDataOld->m_rgc )
    {
        Assert( pDataOld->m_lgenBase <= pDataNew->m_lgenBase );

        LONG cgenMax = min( pDataOld->m_cgen, pDataNew->m_lgenBase - pDataOld->m_lgenBase );
        for ( LONG i = 0; i < cgenMax; ++i )
        {
            LONG c = pDataOld->m_rgc[ i ];
            pDataOld->m_rgc[ i ] = 0;
            pDataOld->m_cOverflow += c;
        }

        for ( LONG i = pDataNew->m_lgenBase; i < pDataOld->m_lgenBase + pDataOld->m_cgen; ++i )
        {
            LONG c = pDataOld->m_rgc[ i - pDataOld->m_lgenBase ];
            pDataOld->m_rgc[ i - pDataOld->m_lgenBase ] = 0;


            if ( pDataNew->m_rgc )
            {
                AtomicExchangeAdd( &pDataNew->m_rgc[ i - pDataNew->m_lgenBase ], c );
            }
            else
            {
                AtomicExchangeAdd( &pDataNew->m_cOverflow, c );
            }
        }

        for ( LONG i = 0; i < pDataOld->m_cgen; ++i )
        {
            Assert( 0 == pDataOld->m_rgc[ i ] );
        }
    }

    AtomicExchangeAdd( &pDataNew->m_cOverflow, pDataOld->m_cOverflow );
}

BFSTAT BFLogHistogram::Read( void )
{
    static BFSTAT bfstatCache( 0, 0 );
    volatile static TICK tickCacheLoad = 0;
    const TICK tickCacheLife = 200;

    TICK tickNow = TickOSTimeCurrent();
    if ( tickNow - tickCacheLoad < tickCacheLife )
    {
        return bfstatCache;
    }

    tickCacheLoad = tickNow;

    LONG cBFMod = 0;
    LONG cBFPin = 0;

    FMP::EnterFMPPoolAsReader();
    if ( g_rgfmp )
    {
        for ( IFMP ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
        {
            FMP* const pfmp = &g_rgfmp[ ifmp ];
            if ( pfmp )
            {
                pfmp->EnterBFContextAsReader();
                BFFMPContext* const pbffmp = ( BFFMPContext* )pfmp->DwBFContext();

                if ( pfmp->FInUse() && !pfmp->FIsTempDB() && pbffmp )
                {
                    BFLogHistogram* const pHist = &pbffmp->m_logHistModify;

                    INT iGroup = pHist->m_ms.Enter();
                    LogHistData* pData = &pHist->m_rgdata[ iGroup ];
                    LONG lgenWaypoint = pfmp->LgposWaypoint().lGeneration;

                    cBFMod += pData->m_cOverflow;
                    cBFPin += ( !pData->m_rgc || lgenWaypoint < pData->m_lgenBase ) ? pData->m_cOverflow : 0;

                    if ( pData->m_rgc )
                    {
                        for ( LONG i = 0; i < pData->m_cgen; ++i )
                        {
                            LONG c = pData->m_rgc[ i ];
                            cBFMod += c;
                            cBFPin += ( pData->m_lgenBase + i < lgenWaypoint ) ? 0 : c;
                        }
                    }

                    pHist->m_ms.Leave( iGroup );
                }

                pfmp->LeaveBFContextAsReader();
            }
        }
    }
    FMP::LeaveFMPPoolAsReader();

    BFSTAT bfstatRet( cBFMod, cBFPin );
    bfstatCache = bfstatRet;
    return bfstatRet;
}

INLINE void BFIMarkAsSuperCold( PBF pbf, const BOOL fUser )
{

    Assert( pbf->sxwl.FOwnExclusiveLatch()  ||
            pbf->sxwl.FOwnWriteLatch()      ||
            ( pbf->err == wrnBFPageFlushPending && NULL == pbf->pWriteSignalComplete ) );

    g_bflruk.MarkAsSuperCold( pbf );


    if ( !pbf->fOlderVersion )
    {
        Assert( pbf->fCurrentVersion );
        BFITraceMarkPageAsSuperCold( pbf->ifmp, pbf->pgno );
    }


    pbf->fNewlyEvicted = fTrue;


    if ( fUser )
    {
        PERFOpt( cBFSuperColdsUser.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
    }
    else
    {
        PERFOpt( cBFSuperColdsInternal.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce ) );
    }
}

C_ASSERT( sizeof(CPAGE::PGHDR) == 40 );

INLINE void BFITouchResource(
    __in const PBF                  pbf,
    __in const BFLatchType          bfltTraceOnly,
    __in const BFLatchFlags         bflfTraceOnly,
    __in const BOOL                 fTouchPage,
    __in const ULONG_PTR            pctCachePriority,
    __in const TraceContext&        tc )
{
    const TICK tickTouch = TickOSTimeCurrent();
    const BOOL fNewPage = ( bflfTraceOnly & ( bflfNew | bflfNewIfUncached ) );
    const BOOL fDBScan = ( bflfTraceOnly & bflfDBScan );
    const BFRequestTraceFlags bfrtf = BFRequestTraceFlags(
        ( fTouchPage ? bfrtfNone : bfrtfNoTouch ) |
        ( fNewPage ? bfrtfNewPage : bfrtfUseHistory ) |
        ( BoolParam( JET_paramEnableFileCache ) ? bfrtfFileCacheEnabled : bfrtfNone ) |
        ( fDBScan ? bfrtfDBScan : bfrtfNone ) );

    if ( fTouchPage )
    {
        AssertSz( pbf->fCurrentVersion && !pbf->fOlderVersion, "We should not be touching an older version of a page." );

        const TICK tickLastBefore = pbf->lrukic.TickLastTouchTime();
        const BFLRUK::ResMgrTouchFlags rmtf = g_bflruk.RmtfTouchResource( pbf, pctCachePriority, tickTouch );

        if ( ( rmtf != BFLRUK::kNoTouch ) && !fDBScan )
        {
            const BOOL fUpdatePerfCounter = ( TickCmp( tickLastBefore, g_tickBFUniqueReqLast ) <= 0 );
            const BOOL fUpdateThreadStats = ( TickCmp( tickLastBefore, Ptls()->TickThreadStatsLast() ) <= 0 );

            if ( fUpdatePerfCounter || fUpdateThreadStats )
            {
                OnDebug( const TICK tickLastAfter = pbf->lrukic.TickLastTouchTime() );
                Assert( TickCmp( tickLastAfter, tickLastBefore ) >= 0 );

                if ( fUpdatePerfCounter )
                {
                    PERFOpt( cBFCacheUniqueReq.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce, pbf->ifmp ) );
                    PERFOpt( cBFCacheUniqueHit.Inc( PinstFromIfmp( pbf->ifmp ), pbf->tce, pbf->ifmp ) );
                }

                if ( fUpdateThreadStats )
                {
                    Ptls()->threadstats.cPageUniqueCacheRequests++;
                    Ptls()->threadstats.cPageUniqueCacheHits++;
                }
            }
        }
    }

    BFITraceRequestPage( tickTouch, pbf, (ULONG)pctCachePriority, bfltTraceOnly, bflfTraceOnly, bfrtf, tc );
}

#ifdef PERFMON_SUPPORT


PERFInstanceLiveTotalWithClass<ULONG> cBFCacheMiss;

LONG LBFCacheMissesCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFCacheMiss.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LBFCacheHitsCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = cBFCacheReq.Get( iInstance ) - cBFCacheMiss.Get( iInstance );
    }

    return 0;
}

PERFInstanceLiveTotalWithClass<ULONG> cBFCacheReq;

LONG LBFCacheReqsCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFCacheReq.PassTo( iInstance, pvBuf );
    return 0;
}

TICK g_tickBFUniqueReqLast = 0;
PERFInstanceLiveTotalWithClass<ULONG, INST, 2> cBFCacheUniqueHit;
PERFInstanceLiveTotalWithClass<ULONG, INST, 2> cBFCacheUniqueReq;

LONG LBFCacheUniqueHitsCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFCacheUniqueHit.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LBFCacheUniqueReqsCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFCacheUniqueReq.PassTo( iInstance, pvBuf );


    const TICK tickNow = TickOSTimeCurrent();
    if ( DtickDelta( g_tickBFUniqueReqLast, tickNow ) >= 1000 )
    {
        g_tickBFUniqueReqLast = tickNow;
    }

    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFSuperColdsUser;
LONG LBFSuperColdsUserCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFSuperColdsUser.PassTo( iInstance, pvBuf );
    return 0;
}
PERFInstanceLiveTotalWithClass<> cBFSuperColdsInternal;
LONG LBFSuperColdsInternalCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFSuperColdsInternal.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LBFCleanBuffersCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = g_cbfCacheClean;
    }

    return 0;
}

LONG LBFPinnedBuffersCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {

        *( ( LONG* ) pvBuf ) = 0;
    }

    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesReadAsync;
LONG LBFPagesReadAsyncCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesReadAsync.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesReadSync;
LONG LBFPagesReadSyncCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesReadSync.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFDirtied;
LONG LBFPagesDirtiedCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFDirtied.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFDirtiedRepeatedly;
LONG LBFPagesDirtiedRepeatedlyCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFDirtiedRepeatedly.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesWritten;
LONG LBFPagesWrittenCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesWritten.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LBFPagesTransferredCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( NULL != pvBuf )
    {
        *( (LONG *) pvBuf ) = cBFPagesReadAsync.Get( iInstance )
                                + cBFPagesReadSync.Get( iInstance )
                                + cBFPagesWritten.Get( iInstance );
    }
    return 0;
}


LONG g_cbfTrimmed;
LONG LBFPagesNonResidentTrimmedByOsCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = g_cbfTrimmed;
    }
    return 0;
}

LONG g_cbfNonResidentReclaimedSuccess;
LONG LBFPagesNonResidentReclaimedSuccessCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = g_cbfNonResidentReclaimedSuccess;
    }
    return 0;
}

LONG g_cbfNonResidentReclaimedFailed;
LONG LBFPagesNonResidentReclaimedFailedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = g_cbfNonResidentReclaimedFailed;
    }
    return 0;
}

LONG g_cbfNonResidentRedirectedToDatabase;
LONG LBFPagesNonResidentRedirectedToDatabaseCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = g_cbfNonResidentRedirectedToDatabase;
    }
    return 0;
}

LONG g_cbfNonResidentEvicted;
LONG LBFPagesNonResidentEvictedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = g_cbfNonResidentEvicted;
    }
    return 0;
}

LONG g_cbfNonResidentReclaimedHardSuccess;
LONG LPagesNonResidentReclaimedHardSuccessCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = g_cbfNonResidentReclaimedHardSuccess;
    }
    return 0;
}

unsigned __int64 g_cusecNonResidentFaultedInLatencyTotal;
LONG LBFPagesNonResidentFaultedInLatencyUsCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf )
    {
        *( (unsigned __int64*) pvBuf ) = g_cusecNonResidentFaultedInLatencyTotal;
    }
    return 0;
}


LONG LBFLatchCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFCacheReq.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstance<> cBFSlowLatch;

LONG LBFFastLatchCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = cBFCacheReq.Get( iInstance ) - cBFSlowLatch.Get( iInstance );
    }

    return 0;
}

PERFInstance<> cBFBadLatchHint;

LONG LBFBadLatchHintCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        cBFBadLatchHint.PassTo( iInstance, pvBuf );
    }

    return 0;
}

PERFInstance<> cBFLatchConflict;

LONG LBFLatchConflictCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        cBFLatchConflict.PassTo( iInstance, pvBuf );
    }

    return 0;
}

PERFInstance<> cBFLatchStall;

LONG LBFLatchStallCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        cBFLatchStall.PassTo( iInstance, pvBuf );
    }

    return 0;
}

LONG LBFAvailBuffersCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bfavail.Cobject() : 0;

    return 0;
}

LONG LBFCacheFaultCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bfavail.CRemove() : 0;

    return 0;
}

LONG LBFCacheEvictCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_cbfNewlyEvictedUsed;

    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFCacheEvictUntouched;
LONG LBFCacheEvictUntouchedCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFCacheEvictUntouched.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFCacheEvictk1;
LONG LBFCacheEvictk1CEFLPv( LONG iInstance, void* pvBuf )
{
    cBFCacheEvictk1.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFCacheEvictk2;
LONG LBFCacheEvictk2CEFLPv( LONG iInstance, void* pvBuf )
{
    cBFCacheEvictk2.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> rgcBFCacheEvictReasons[bfefReasonMax];
LONG LBFCacheEvictScavengeAvailPoolCEFLPv( LONG iInstance, void* pvBuf )
{
    C_ASSERT( bfefReasonAvailPool < _countof(rgcBFCacheEvictReasons) );
    rgcBFCacheEvictReasons[bfefReasonAvailPool].PassTo( iInstance, pvBuf );
    return 0;
}
PERFInstanceLiveTotalWithClass<> cBFCacheEvictScavengeSuperColdInternal;
LONG LBFCacheEvictScavengeSuperColdInternalCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFCacheEvictScavengeSuperColdInternal.PassTo( iInstance, pvBuf );
    return 0;
}
PERFInstanceLiveTotalWithClass<> cBFCacheEvictScavengeSuperColdUser;
LONG LBFCacheEvictScavengeSuperColdUserCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFCacheEvictScavengeSuperColdUser.PassTo( iInstance, pvBuf );
    return 0;
}
LONG LBFCacheEvictScavengeShrinkCEFLPv( LONG iInstance, void* pvBuf )
{
    C_ASSERT( bfefReasonShrink < _countof(rgcBFCacheEvictReasons) );
    rgcBFCacheEvictReasons[bfefReasonShrink].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LBFCacheEvictOtherCEFLPv( LONG iInstance, void* pvBuf )
{
    C_ASSERT( bfefReasonPurgeContext < _countof(rgcBFCacheEvictReasons) );
    C_ASSERT( bfefReasonPurgePage < _countof(rgcBFCacheEvictReasons) );
    C_ASSERT( bfefReasonPatch < _countof(rgcBFCacheEvictReasons) );

    if ( pvBuf != NULL )
    {
        LONG lBufPurgeContext, lBufPurgePage, lBufPatch;
        rgcBFCacheEvictReasons[bfefReasonPurgeContext].PassTo( iInstance, &lBufPurgeContext );
        rgcBFCacheEvictReasons[bfefReasonPurgePage].PassTo( iInstance, &lBufPurgePage );
        rgcBFCacheEvictReasons[bfefReasonPatch].PassTo( iInstance, &lBufPatch );

        *( (LONG*)pvBuf ) = lBufPurgeContext + lBufPurgePage + lBufPatch;
    }

    return 0;
}

LONG LBFAvailStallsCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bfavail.CRemoveWait() : 0;

    return 0;
}

LONG LBFTotalBuffersCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = (ULONG)( g_fBFInitialized ? cbfCacheSize : 1 );

    return 0;
}

LONG LBFTotalBuffersUsedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = (ULONG)( g_fBFInitialized ? CbfBFICacheUsed() : 1 );

    return 0;
}

LONG LBFTotalBuffersCommittedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = (ULONG)( g_fBFInitialized ? CbfBFICacheCommitted() : 1 );

    return 0;
}

LONG LBFCacheSizeCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = CbBFICacheSizeUsedDehydrated();

    return 0;
}

LONG LBFCacheSizeMBCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = ( CbBFICacheSizeUsedDehydrated() / ( 1024 * 1024 ) );

    return 0;
}

LONG LBFCacheSizeEffectiveCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = CbBFICacheISizeUsedHydrated();

    return 0;
}

LONG LBFCacheSizeEffectiveMBCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = ( CbBFICacheISizeUsedHydrated() / ( 1024 * 1024 ) );

    return 0;
}

LONG LBFCacheMemoryReservedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = CbBFICacheIMemoryReserved();

    return 0;
}

LONG LBFCacheMemoryReservedMBCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = ( CbBFICacheIMemoryReserved() / ( 1024 * 1024 ) );

    return 0;
}

LONG LBFCacheMemoryCommittedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = CbBFICacheIMemoryCommitted();

    return 0;
}

LONG LBFCacheMemoryCommittedMBCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = ( CbBFICacheIMemoryCommitted() / ( 1024 * 1024 ) );

    return 0;
}

LONG LBFDehydratedBuffersCEFLPv( LONG iInstance, void* pvBuf )
{
    LONG cbf = 0;
    for( INT icb = icbPageSmallest; icb < g_icbCacheMax; icb++ )
    {
        cbf += g_rgcbfCachePages[icb];
    }

    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = cbf;
    }

    return 0;
}


LONG LBFCacheSizeTargetCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *( (unsigned __int64*) pvBuf ) = g_cacheram.GetOptimalResourcePoolSize();
    }

    return 0;
}

LONG LBFCacheSizeTargetMBCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *( (unsigned __int64*) pvBuf ) = g_cacheram.GetOptimalResourcePoolSize() / ( 1024  * 1024 );
    }

    return 0;
}

PERFInstanceDelayedTotalWithClass< LONG, INST, 2 > cBFCache;
LONG LBFCacheSizeMBCategorizedCEFLPv( LONG iInstance, void* pvBuf )
{
    const __int64 cbAveBufferSize = CbBFIAveResourceSize();
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = ( ( (ULONG_PTR)cBFCache.Get( iInstance ) * cbAveBufferSize ) / ( 1024 * 1024 ) );

    return 0;
}

LONG LBFCacheSizeCategorizedCEFLPv( LONG iInstance, void* pvBuf )
{
    const __int64 cbAveBufferSize = CbBFIAveResourceSize();
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = ( (ULONG_PTR)cBFCache.Get( iInstance ) * cbAveBufferSize ) ;

    return 0;
}
    
LONG LBFCacheSizeMinCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = (ULONG_PTR)UlParam( JET_paramCacheSizeMin ) * g_rgcbPageSize[g_icbCacheMax];

    return 0;
}

LONG LBFCacheSizeMaxCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = (ULONG_PTR)UlParam( JET_paramCacheSizeMax ) * g_rgcbPageSize[g_icbCacheMax];

    return 0;
}

LONG LBFCacheSizeResidentCEFLPv( LONG iInstance, void* pvBuf )
{
    const __int64 cbAveBufferSize = CbBFIAveResourceSize();
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = (ULONG_PTR)g_cbfCacheResident * cbAveBufferSize;

    return 0;
}

LONG LBFCacheSizeResidentMBCEFLPv( LONG iInstance, void* pvBuf )
{
    const __int64 cbAveBufferSize = CbBFIAveResourceSize();
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = ( ( (ULONG_PTR)g_cbfCacheResident * cbAveBufferSize ) / ( 1024 * 1024 ) );

    return 0;
}

LONG LBFCacheSizingDurationCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *( (unsigned __int64*) pvBuf ) = DtickBFIMaintCacheSizeDuration() / 1000;
    }

    return 0;
}

__int64 g_cbCacheUnattached = 0;
LONG LBFCacheUnattachedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (unsigned __int64*) pvBuf ) = g_cbCacheUnattached;

    return 0;
}

LONG LBFStartFlushThresholdCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = (ULONG)cbfAvailPoolLow;
    }

    return 0;
}

LONG LBFStopFlushThresholdCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = (ULONG)cbfAvailPoolHigh;
    }

    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesPreread;
LONG LBFPagesPrereadCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesPreread.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPrereadStall;
LONG LBFPagePrereadStallsCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPrereadStall.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesPrereadUnnecessary;
LONG LBFPagesPrereadUnnecessaryCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesPrereadUnnecessary.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesDehydrated;
LONG LBFPagesDehydratedCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesDehydrated.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesRehydrated;
LONG LBFPagesRehydratedCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesRehydrated.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesVersioned;
LONG LBFPagesVersionedCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesVersioned.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesVersionCopied;
LONG LBFPagesVersionCopiedCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesVersionCopied.PassTo( iInstance, pvBuf );
    return 0;
}

LONG g_cBFVersioned;
LONG LBFVersionedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *( (ULONG*) pvBuf ) = g_cBFVersioned;
    }

    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesRepeatedlyWritten;
LONG LBFPagesRepeatedlyWrittenCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesRepeatedlyWritten.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesFlushedCacheShrink;
LONG LBFPagesFlushedCacheShrinkCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesFlushedCacheShrink.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesFlushedCheckpoint;
LONG LBFPagesFlushedCheckpointCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesFlushedCheckpoint.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesFlushedCheckpointForeground;
LONG LBFPagesFlushedCheckpointForegroundCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesFlushedCheckpointForeground.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesFlushedContextFlush;
LONG LBFPagesFlushedContextFlushCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesFlushedContextFlush.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesFlushedFilthyForeground;
LONG LBFPagesFlushedFilthyForegroundCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesFlushedFilthyForeground.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesFlushedAvailPool;
LONG LBFPagesFlushedAvailPoolCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesFlushedAvailPool.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesFlushedScavengeSuperColdInternal;
LONG LBFPagesFlushedScavengeSuperColdInternalCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesFlushedScavengeSuperColdInternal.PassTo( iInstance, pvBuf );
    return 0;
}
PERFInstanceLiveTotalWithClass<> cBFPagesFlushedScavengeSuperColdUser;
LONG LBFPagesFlushedScavengeSuperColdUserCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesFlushedScavengeSuperColdUser.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<>  cBFPagesFlushedOpportunely;
LONG LBFPagesFlushedOpportunelyCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesFlushedOpportunely.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesFlushedOpportunelyClean;
LONG LBFPagesFlushedOpportunelyCleanCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesFlushedOpportunelyClean.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesCoalescedWritten;
LONG LBFPagesCoalescedWrittenCEFLPv( LONG iInstance, void * pvBuf )
{
    cBFPagesCoalescedWritten.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesCoalescedRead;
LONG LBFPagesCoalescedReadCEFLPv( LONG iInstance, void * pvBuf )
{
    cBFPagesCoalescedRead.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LBFPageHistoryCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bflruk.CHistoryRecord() : 0;

    return 0;
}

LONG LBFPageHistoryHitsCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bflruk.CHistoryHit() : 0;

    return 0;
}

LONG LBFPageHistoryReqsCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bflruk.CHistoryRequest() : 1;

    return 0;
}

LONG LBFPageScannedOutOfOrderCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bflruk.CResourceScannedOutOfOrder() : 0;

    return 0;
}

LONG LBFPageScannedMovesCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bflruk.CResourceScannedMoves() : 0;

    return 0;
}

LONG LBFPageScannedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bflruk.CResourceScanned() : 1;

    return 0;
}

LONG LRESMGRScanFoundEntriesCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bflruk.CLastScanEnumeratedEntries() : 1;

    return 0;
}

LONG LRESMGRScanBucketsScannedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bflruk.CLastScanBucketsScanned() : 1;

    return 0;
}

LONG LRESMGRScanEmptyBucketsScannedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bflruk.CLastScanEmptyBucketsScanned() : 1;

    return 0;
}

LONG LRESMGRScanIdRangeCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bflruk.CLastScanEnumeratedIDRange() : 1;

    return 0;
}

LONG LRESMGRScanTimeRangeCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_bflruk.DtickLastScanEnumeratedRange() : 1;

    return 0;
}

LONG LRESMGRCacheLifetimeCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *((LONG*)pvBuf) = g_fBFInitialized ? ( ( LFunctionalMax( g_bflruk.DtickScanFirstEvictedIndexTarget(), 0 ) + 500 ) / 1000 ) : 0;
    }
    return 0;
}

LONG LRESMGRCacheLifetimeHWCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *((LONG*)pvBuf) = g_fBFInitialized ? ( ( LFunctionalMax( g_bflruk.DtickScanFirstEvictedIndexTargetHW(), 0 ) + 500 ) / 1000 ) : 0;
    }
    return 0;
}

LONG LRESMGRCacheLifetimeMaxCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *((LONG*)pvBuf) = g_fBFInitialized ? ( ( LFunctionalMax( g_bflruk.DtickScanFirstFoundNormal(), 0 ) + 500 ) / 1000 ) : 0;
    }
    return 0;
}

LONG LRESMGRCacheLifetimeEstVarCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *((LONG*)pvBuf) = g_fBFInitialized ? LFunctionalMax( g_bflruk.DtickScanFirstEvictedIndexTargetVar(), 0 ) : 0;
    }
    return 0;
}

LONG LRESMGRCacheLifetimeK1CEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *((LONG*)pvBuf) = g_fBFInitialized ? ( ( LFunctionalMax( g_bflruk.DtickScanFirstEvictedTouchK1(), 0 ) + 500 ) / 1000 ) : 0;
    }
    return 0;
}

LONG LRESMGRCacheLifetimeK2CEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *((LONG*)pvBuf) = g_fBFInitialized ? ( ( LFunctionalMax( g_bflruk.DtickScanFirstEvictedTouchK2(), 0 ) + 500 ) / 1000 ) : 0;
    }
    return 0;
}

LONG LRESMGRScanFoundToEvictRangeCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *((LONG*)pvBuf) = g_fBFInitialized ? g_bflruk.DtickFoundToEvictDelta() : 0;
    }
    return 0;
}

LONG LRESMGRSuperColdedResourcesCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *((LONG*)pvBuf) = g_fBFInitialized ? g_bflruk.CSuperColdedResources() : 0;
    }
    return 0;
}

LONG LRESMGRSuperColdAttemptsCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *((LONG*)pvBuf) = g_fBFInitialized ? g_bflruk.CSuperColdAttempts() : 0;
    }
    return 0;
}

LONG LRESMGRSuperColdSuccessesCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        *((LONG*)pvBuf) = g_fBFInitialized ? g_bflruk.CSuperColdSuccesses() : 0;
    }
    return 0;
}

LONG LBFResidentBuffersCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_cbfCacheResident : 0;

    return 0;
}

LONG LBFMemoryEvictCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_fBFInitialized ? g_cacheram.CpgReclaim() + g_cacheram.CpgEvict() : 0;

    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFPagesRepeatedlyRead;
LONG LBFPagesRepeatedlyReadCEFLPv( LONG iInstance, void* pvBuf )
{
    cBFPagesRepeatedlyRead.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LBFOpportuneWriteIssuedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( ( ULONG* )pvBuf ) = cBFOpportuneWriteIssued;
    return 0;
}

PERFInstanceDelayedTotal< LONG, INST, fFalse > cBFCheckpointMaintOutstandingIOMax;
LONG LCheckpointMaintOutstandingIOMaxCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
    {
        cBFCheckpointMaintOutstandingIOMax.PassTo( iInstance, pvBuf );
    }

    return 0;
}

PERFInstanceLiveTotalWithClass<QWORD> cBFCacheMissLatencyTotalTicksAttached;
LONG LBFCacheMissLatencyTotalTicksAttachedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cBFCacheMissLatencyTotalTicksAttached.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFCacheMissLatencyTotalOperationsAttached;
LONG LBFCacheMissTotalAttachedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cBFCacheMissLatencyTotalOperationsAttached.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cBFCacheUnused;
LONG LBFCacheSizeUnusedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (unsigned __int64*)pvBuf ) = ( (ULONG_PTR)cBFCacheUnused.Get( iInstance ) * g_rgcbPageSize[g_icbCacheMax] );

    return 0;
}

#endif


#ifdef ENABLE_JET_UNIT_TEST


JETUNITTEST( BF, BFPriorityBasicResourcePriority )
{
    CHECK( 0    == PctBFCachePri( BfpriBFMake( 0, (BFTEMPOSFILEQOS)0x0 ) ) );
    CHECK( 1    == PctBFCachePri( BfpriBFMake( 1, (BFTEMPOSFILEQOS)0x0 ) ) );
    CHECK( 10   == PctBFCachePri( BfpriBFMake( 10, (BFTEMPOSFILEQOS)0x0 ) ) );
    CHECK( 100  == PctBFCachePri( BfpriBFMake( 100, (BFTEMPOSFILEQOS)0x0 ) ) );
    CHECK( 1000 == PctBFCachePri( BfpriBFMake( 1000, (BFTEMPOSFILEQOS)0x0 ) ) );

    CHECK( 1001 == PctBFCachePri( BfpriBFMake( 1001, (BFTEMPOSFILEQOS)0x0 ) ) );
    CHECK( 1023 == PctBFCachePri( BfpriBFMake( 1023, (BFTEMPOSFILEQOS)0x0 ) ) );
    CHECK( 0    == PctBFCachePri( BfpriBFMake( 1024, (BFTEMPOSFILEQOS)0x0 ) ) );
    CHECK( 1023 == PctBFCachePri( BfpriBFMake( 0xFFFFFFFF, (BFTEMPOSFILEQOS)0x0 ) ) );
    CHECK( 0    == QosBFUserAndIoPri( BfpriBFMake( 0xFFFFFFFF, (BFTEMPOSFILEQOS)0x0 ) ) );
}

JETUNITTEST( BF, BFPriorityBasicIoDispatchPriority )
{
    CHECK( 0                       == QosBFUserAndIoPri( BfpriBFMake( 0, (BFTEMPOSFILEQOS)0 ) ) );
    CHECK( qosIODispatchBackground == QosBFUserAndIoPri( BfpriBFMake( 0, (BFTEMPOSFILEQOS)qosIODispatchBackground ) ) );
    CHECK( qosIODispatchImmediate  == QosBFUserAndIoPri( BfpriBFMake( 0, (BFTEMPOSFILEQOS)qosIODispatchImmediate ) ) );

    CHECK( 0                       == QosBFUserAndIoPri( BfpriBFMake( 100, (BFTEMPOSFILEQOS)0 ) ) );
    CHECK( qosIODispatchBackground == QosBFUserAndIoPri( BfpriBFMake( 100, (BFTEMPOSFILEQOS)qosIODispatchBackground ) ) );
    CHECK( qosIODispatchImmediate  == QosBFUserAndIoPri( BfpriBFMake( 100, (BFTEMPOSFILEQOS)qosIODispatchImmediate ) ) );
}

JETUNITTEST( BF, BFPriorityBasicUserTagPriority )
{
    CHECK( 0x40000000 == QosBFUserAndIoPri( BfpriBFMake( 0, (BFTEMPOSFILEQOS)0x40000000 ) ) );
    CHECK( 0x01000000 == QosBFUserAndIoPri( BfpriBFMake( 0, (BFTEMPOSFILEQOS)0x01000000 ) ) );
    CHECK( 0x0F000000 == QosBFUserAndIoPri( BfpriBFMake( 0, (BFTEMPOSFILEQOS)0x0F000000 ) ) );
    CHECK( 0x4F000000 == QosBFUserAndIoPri( BfpriBFMake( 0, (BFTEMPOSFILEQOS)0x4F000000 ) ) );
    CHECK( 0x0F000000 == QosBFUserAndIoPri( BfpriBFMake( 0, (BFTEMPOSFILEQOS)0x1F000000 ) ) );
    CHECK( 0x0F000000 == QosBFUserAndIoPri( BfpriBFMake( 0, (BFTEMPOSFILEQOS)0x2F000000 ) ) );
    CHECK( 0x4F007F72 == QosBFUserAndIoPri( BfpriBFMake( 0, (BFTEMPOSFILEQOS)0xFFFFFFFF ) ) );

    CHECK( 0x40000000 == QosBFUserAndIoPri( BfpriBFMake( 100, (BFTEMPOSFILEQOS)0x40000000 ) ) );
    CHECK( 0x01000000 == QosBFUserAndIoPri( BfpriBFMake( 100, (BFTEMPOSFILEQOS)0x01000000 ) ) );
    CHECK( 0x0F000000 == QosBFUserAndIoPri( BfpriBFMake( 100, (BFTEMPOSFILEQOS)0x0F000000 ) ) );
    CHECK( 0x4F000000 == QosBFUserAndIoPri( BfpriBFMake( 100, (BFTEMPOSFILEQOS)0x4F000000 ) ) );
}

JETUNITTEST( BF, BFPriorityMaxEdgeCases )
{
    BFTEMPOSFILEQOS qosWorst = (BFTEMPOSFILEQOS)( 0x4F000000 | qosIODispatchMask | qosIOOSLowPriority );
    wprintf( L"\n\t\t BfpriBFMake( 1000%%, qosWorst = 0x%I64x ) -> bfpri = 0x%x ( bfpriFaultIoPriorityMask = 0x%x ).\n", 
                (QWORD)qosWorst, BfpriBFMake( 1000, qosWorst ), bfpriFaultIoPriorityMask );
    CHECK( 1000     == PctBFCachePri(     BfpriBFMake( 1000, qosWorst ) ) );
    CHECK( qosWorst == QosBFUserAndIoPri( BfpriBFMake( 1000, qosWorst ) ) );
}



JETUNITTEST( BF, AtomicBitFieldSet )
{
    BF bf;

    CHECK( !bf.bfbitfield.FDependentPurged() );
    CHECK( !bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( !bf.bfbitfield.FRangeLocked() );

    bf.bfbitfield.SetFDependentPurged( fTrue );
    CHECK( bf.bfbitfield.FDependentPurged() );
    CHECK( !bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( !bf.bfbitfield.FRangeLocked() );
    bf.bfbitfield.SetFDependentPurged( fFalse );
    CHECK( !bf.bfbitfield.FDependentPurged() );
    CHECK( !bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( !bf.bfbitfield.FRangeLocked() );

    bf.bfbitfield.SetFImpedingCheckpoint( fTrue );
    CHECK( !bf.bfbitfield.FDependentPurged() );
    CHECK( bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( !bf.bfbitfield.FRangeLocked() );
    bf.bfbitfield.SetFImpedingCheckpoint( fFalse );
    CHECK( !bf.bfbitfield.FDependentPurged() );
    CHECK( !bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( !bf.bfbitfield.FRangeLocked() );

    bf.bfbitfield.SetFRangeLocked( fTrue );
    CHECK( !bf.bfbitfield.FDependentPurged() );
    CHECK( !bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( bf.bfbitfield.FRangeLocked() );
    bf.bfbitfield.SetFRangeLocked( fFalse );
    CHECK( !bf.bfbitfield.FDependentPurged() );
    CHECK( !bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( !bf.bfbitfield.FRangeLocked() );
}

JETUNITTEST( BF, AtomicBitFieldReset )
{
    BF bf;

    CHECK( !bf.bfbitfield.FDependentPurged() );
    CHECK( !bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( !bf.bfbitfield.FRangeLocked() );

    bf.bfbitfield.SetFDependentPurged( fTrue );
    bf.bfbitfield.SetFImpedingCheckpoint( fTrue );
    bf.bfbitfield.SetFRangeLocked( fTrue );
    CHECK( bf.bfbitfield.FDependentPurged() );
    CHECK( bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( bf.bfbitfield.FRangeLocked() );

    bf.bfbitfield.SetFDependentPurged( fFalse );
    CHECK( !bf.bfbitfield.FDependentPurged() );
    CHECK( bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( bf.bfbitfield.FRangeLocked() );
    bf.bfbitfield.SetFDependentPurged( fTrue );
    CHECK( bf.bfbitfield.FDependentPurged() );
    CHECK( bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( bf.bfbitfield.FRangeLocked() );

    bf.bfbitfield.SetFImpedingCheckpoint( fFalse );
    CHECK( bf.bfbitfield.FDependentPurged() );
    CHECK( !bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( bf.bfbitfield.FRangeLocked() );
    bf.bfbitfield.SetFImpedingCheckpoint( fTrue );
    CHECK( bf.bfbitfield.FDependentPurged() );
    CHECK( bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( bf.bfbitfield.FRangeLocked() );

    bf.bfbitfield.SetFRangeLocked( fFalse );
    CHECK( bf.bfbitfield.FDependentPurged() );
    CHECK( bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( !bf.bfbitfield.FRangeLocked() );
    bf.bfbitfield.SetFRangeLocked( fTrue );
    CHECK( bf.bfbitfield.FDependentPurged() );
    CHECK( bf.bfbitfield.FImpedingCheckpoint() );
    CHECK( bf.bfbitfield.FRangeLocked() );
}

#endif
