// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

LOCAL ERR ErrDIRICheckIndexRange( FUCB *pfucb, const KEY& key );
LOCAL ERR ErrDIRIIRefresh( FUCB * const pfucb );
INLINE ERR ErrDIRIRefresh( FUCB * const pfucb )
{
    return ( locDeferMoveFirst != pfucb->locLogical ?
                JET_errSuccess :
                ErrDIRIIRefresh( pfucb ) );
}


#ifdef PERFMON_SUPPORT


PERFInstanceDelayedTotal<> cDIRUserROTrxCommit0;
LONG LDIRUserROTrxCommit0CEFLPv(LONG iInstance,void *pvBuf)
{
    cDIRUserROTrxCommit0.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cDIRUserRWDurableTrxCommit0;
LONG LDIRUserRWDurableTrxCommit0CEFLPv(LONG iInstance,void *pvBuf)
{
    cDIRUserRWDurableTrxCommit0.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cDIRUserRWLazyTrxCommit0;
LONG LDIRUserRWLazyTrxCommit0CEFLPv(LONG iInstance,void *pvBuf)
{
    cDIRUserRWLazyTrxCommit0.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDIRUserTrxCommit0CEFLPv(LONG iInstance,void *pvBuf)
{
    if ( NULL != pvBuf )
    {
        *((LONG *)pvBuf) = cDIRUserROTrxCommit0.Get( iInstance ) + cDIRUserRWDurableTrxCommit0.Get( iInstance ) + cDIRUserRWLazyTrxCommit0.Get( iInstance );
    }
    return 0;
}

PERFInstanceDelayedTotal<> cDIRUserROTrxRollback0;
LONG LDIRUserROTrxRollback0CEFLPv(LONG iInstance,void *pvBuf)
{
    cDIRUserROTrxRollback0.PassTo( iInstance, pvBuf );
    return 0;
}


PERFInstanceDelayedTotal<> cDIRUserRWTrxRollback0;
LONG LDIRUserRWTrxRollback0CEFLPv(LONG iInstance,void *pvBuf)
{
    cDIRUserRWTrxRollback0.PassTo( iInstance, pvBuf );
    return 0;
}


LONG LDIRUserTrxRollback0CEFLPv(LONG iInstance,void *pvBuf)
{
    if ( NULL != pvBuf )
    {
        *((LONG *)pvBuf) = cDIRUserROTrxRollback0.Get( iInstance ) + cDIRUserRWTrxRollback0.Get( iInstance );
    }
    return 0;
}


PERFInstanceDelayedTotal<> cDIRSystemROTrxCommit0;
LONG LDIRSystemROTrxCommit0CEFLPv(LONG iInstance,void *pvBuf)
{
    cDIRSystemROTrxCommit0.PassTo( iInstance, pvBuf );
    return 0;
}


PERFInstanceDelayedTotal<> cDIRSystemRWDurableTrxCommit0;
LONG LDIRSystemRWDurableTrxCommit0CEFLPv(LONG iInstance,void *pvBuf)
{
    cDIRSystemRWDurableTrxCommit0.PassTo( iInstance, pvBuf );

    return 0;
}


PERFInstanceDelayedTotal<> cDIRSystemRWLazyTrxCommit0;
LONG LDIRSystemRWLazyTrxCommit0CEFLPv(LONG iInstance,void *pvBuf)
{
    cDIRSystemRWLazyTrxCommit0.PassTo( iInstance, pvBuf );

    return 0;
}


LONG LDIRSystemTrxCommit0CEFLPv(LONG iInstance,void *pvBuf)
{
    if ( NULL != pvBuf )
    {
        *((LONG *)pvBuf) = cDIRSystemROTrxCommit0.Get( iInstance ) + cDIRSystemRWDurableTrxCommit0.Get( iInstance ) + cDIRSystemRWLazyTrxCommit0.Get( iInstance );
    }
    return 0;
}


PERFInstanceDelayedTotal<> cDIRSystemROTrxRollback0;
LONG LDIRSystemROTrxRollback0CEFLPv(LONG iInstance,void *pvBuf)
{
    cDIRSystemROTrxRollback0.PassTo( iInstance, pvBuf );
    return 0;
}


PERFInstanceDelayedTotal<> cDIRSystemRWTrxRollback0;
LONG LDIRSystemRWTrxRollback0CEFLPv(LONG iInstance,void *pvBuf)
{
    cDIRSystemRWTrxRollback0.PassTo( iInstance, pvBuf );
    return 0;
}


LONG LDIRSystemTrxRollback0CEFLPv(LONG iInstance,void *pvBuf)
{
    if ( NULL != pvBuf )
    {
        *((LONG *)pvBuf) = cDIRSystemROTrxRollback0.Get( iInstance ) + cDIRSystemRWTrxRollback0.Get( iInstance );
    }
    return 0;
}

#endif





ERR ErrDIRCreateDirectory(
    FUCB    *pfucb,
    CPG     cpgMin,
    PGNO    *ppgnoFDP,
    OBJID   *pobjidFDP,
    UINT    fPageFlags,
    BOOL    fSPFlags )
{
    ERR     err;
    CPG     cpgRequest = cpgMin;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( NULL != ppgnoFDP );
    Assert( NULL != pobjidFDP );

    Assert( !Pcsr( pfucb )->FLatched( ) );
    Assert( locOnFDPRoot == pfucb->locLogical );

    fSPFlags |= fSPNewFDP;

    Assert( ( fSPFlags & fSPUnversionedExtent )
            || !FFMPIsTempDB( pfucb->ifmp ) );

    *ppgnoFDP = pgnoNull;
    Call( ErrSPGetExt(
        pfucb,
        PgnoFDP( pfucb ),
        &cpgRequest,
        cpgMin,
        ppgnoFDP,
        fSPFlags,
        fPageFlags,
        pobjidFDP ) );
    Assert( *ppgnoFDP > pgnoSystemRoot );
    Assert( *ppgnoFDP <= pgnoSysMax );
    Assert( *pobjidFDP > objidSystemRoot );

HandleError:
    return err;
}



ERR ErrDIROpen( PIB *ppib, PGNO pgnoFDP, IFMP ifmp, FUCB **ppfucb, BOOL fWillInitFCB )
{
    ERR     err;
    FUCB    *pfucb;

    CheckPIB( ppib );

#ifdef DEBUG
    INST *pinst = PinstFromPpib( ppib );
    if ( !pinst->FRecovering()
         && pinst->m_fSTInit == fSTInitDone
         && !Ptls()->FIsTaskThread()
         && !Ptls()->fIsRCECleanup )
    {
        CheckDBID( ppib, ifmp );
    }
#endif

    CallR( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb, openNormal, fWillInitFCB ) );
    DIRInitOpenedCursor( pfucb, pfucb->ppib->Level() );

    *ppfucb = pfucb;
    return JET_errSuccess;
}

ERR ErrDIROpenNoTouch( PIB *ppib, IFMP ifmp, PGNO pgnoFDP, OBJID objidFDP, BOOL fUnique, FUCB **ppfucb, BOOL fWillInitFCB )
{
    ERR     err;
    FUCB    *pfucb;

    CheckPIB( ppib );

#ifdef DEBUG
    INST *pinst = PinstFromPpib( ppib );
    if ( !pinst->FRecovering()
         && pinst->m_fSTInit == fSTInitDone
         && !Ptls()->FIsTaskThread()
         && !Ptls()->fIsRCECleanup )
    {
        CheckDBID( ppib, ifmp );
    }
#endif

    CallR( ErrBTOpenNoTouch( ppib, ifmp, pgnoFDP, objidFDP, fUnique, &pfucb, fWillInitFCB ) );
    DIRInitOpenedCursor( pfucb, pfucb->ppib->Level() );

    *ppfucb = pfucb;
    return JET_errSuccess;
}

ERR ErrDIROpen( PIB *ppib, FCB *pfcb, FUCB **ppfucb )
{
    ERR     err;
    FUCB    *pfucb;

    CheckPIB( ppib );

#ifdef DEBUG
    INST *pinst = PinstFromPpib( ppib );
    if ( !pinst->FRecovering()
        && pinst->m_fSTInit == fSTInitDone
        && !Ptls()->FIsTaskThread()
        && !Ptls()->fIsRCECleanup )
    {
        CheckDBID( ppib, pfcb->Ifmp() );
    }
#endif

    Assert( pfcbNil != pfcb );
    CallR( ErrBTOpen( ppib, pfcb, &pfucb ) );
    DIRInitOpenedCursor( pfucb, pfucb->ppib->Level() );

    *ppfucb = pfucb;
    return JET_errSuccess;
}

ERR ErrDIROpenByProxy( PIB *ppib, FCB *pfcb, FUCB **ppfucb, LEVEL level )
{
    ERR     err;
    FUCB    *pfucb;

    CheckPIB( ppib );

#ifdef DEBUG
    INST *pinst = PinstFromPpib( ppib );
    if ( !pinst->FRecovering()
        && pinst->m_fSTInit == fSTInitDone
        && !Ptls()->FIsTaskThread()
        && !Ptls()->fIsRCECleanup )
    {
        CheckDBID( ppib, pfcb->Ifmp() );
    }
#endif

    Assert( pfcbNil != pfcb );
    Assert( level > 0 );
    CallR( ErrBTOpenByProxy( ppib, pfcb, &pfucb, level ) );
    DIRInitOpenedCursor( pfucb, level );

    *ppfucb = pfucb;
    return JET_errSuccess;
}


VOID DIRClose( FUCB *pfucb )
{
    Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() || !FFUCBDeferClosed(pfucb) );

    RECReleaseKeySearchBuffer( pfucb );
    FILEReleaseCurrentSecondary( pfucb );

    BTClose( pfucb );
}



ERR ErrDIRGet( FUCB *pfucb )
{
    ERR     err;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !FFUCBSpace( pfucb ) );

    if ( locOnCurBM == pfucb->locLogical )
    {
        Call( ErrBTGet( pfucb ) );
        Assert( Pcsr( pfucb )->FLatched() );
        return err;
    }

    CallR( ErrDIRIRefresh( pfucb ) );

    switch ( pfucb->locLogical )
    {
        case locOnCurBM:
            Call( ErrBTGet( pfucb ) );
            return err;

        case locAfterSeekBM:
        case locBeforeSeekBM:
        case locOnFDPRoot:
        case locDeferMoveFirst:
            Assert( fFalse );
            break;


        default:
            Assert( pfucb->locLogical == locAfterLast
                    || pfucb->locLogical == locOnSeekBM
                    || pfucb->locLogical == locBeforeFirst );
            return ErrERRCheck( JET_errNoCurrentRecord );
    }

    Assert( fFalse );
    return err;

HandleError:
    BTUp( pfucb );
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}

ERR ErrDIRGetPosition( FUCB *pfucb, ULONG *pulLT, ULONG *pulTotal )
{
    ERR     err;
    ULONG   ulLT;
    ULONG   ulTotal;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !FFUCBSpace( pfucb ) );

    Call( ErrDIRIRefresh( pfucb ) );

    switch ( pfucb->locLogical )
    {
        case locOnCurBM:
        case locOnSeekBM:
            break;
        default:
            return ErrERRCheck( JET_errNoCurrentRecord );
    }

    Call( ErrBTGetPosition( pfucb, &ulLT, &ulTotal ) );
    CallS( err );

    Assert( ulLT <= ulTotal );
    *pulLT = ulLT;
    *pulTotal = ulTotal;

HandleError:
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


ERR ErrDIRRelease( FUCB *pfucb )
{
    ERR     err;

    Assert( pfucb->locLogical == locOnCurBM ||
            pfucb->locLogical == locAfterSeekBM ||
            pfucb->locLogical == locBeforeSeekBM );
    Assert( !FFUCBSpace( pfucb ) );

    switch ( pfucb->locLogical )
    {
        case locOnCurBM:
            Call( ErrBTRelease( pfucb ) );
            break;

        case locAfterSeekBM:
        case locBeforeSeekBM:
        {
            BOOKMARK    bm;

            FUCBAssertValidSearchKey( pfucb );
            bm.Nullify();

            bm.key.suffix.SetPv( pfucb->dataSearchKey.Pv() );
            bm.key.suffix.SetCb( pfucb->dataSearchKey.Cb() );
            Call( ErrBTDeferGotoBookmark( pfucb, bm, fFalse ) );
            pfucb->locLogical = locOnSeekBM;
            break;
        }

        default:
            AssertSz( fFalse, "Unexpected locLogical" );
            err = JET_errSuccess;
            break;
    }

    Assert( !Pcsr( pfucb )->FLatched() );
    return err;

HandleError:
    Assert( err < 0 );

    Assert( JET_errOutOfMemory == err );
    DIRUp( pfucb );

    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


ERR ErrDIRGotoJetBookmark( FUCB *pfucb, const BOOKMARK& bm, const BOOL fRetainLatch )
{
    ERR     err;

    Assert( !Pcsr( pfucb )->FLatched() );
    ASSERT_VALID( &bm );
    Assert( !FFUCBSpace( pfucb ) );

    if( !FFUCBSequential( pfucb ) )
    {
        FUCBResetPreread( pfucb );
    }

    Call( ErrBTGotoBookmark( pfucb, bm, latchReadNoTouch, fTrue ) );

    Assert( Pcsr( pfucb )->FLatched() );
    Assert( Pcsr( pfucb )->Cpage().FLeafPage() );

    Call( ErrBTGet( pfucb ) );
    pfucb->locLogical = locOnCurBM;

    if ( !fRetainLatch )
    {
        Call( ErrBTRelease( pfucb ) );
        Assert( 0 == CmpBM( pfucb->bmCurr, bm ) );
    }

    return err;

HandleError:
    BTUp( pfucb );
    return err;
}


ERR ErrDIRGotoBookmark( FUCB *pfucb, const BOOKMARK& bm )
{
    ERR     err;

    ASSERT_VALID( &bm );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !FFUCBSpace( pfucb ) );

    if( !FFUCBSequential( pfucb ) )
    {
        FUCBResetPreread( pfucb );
    }

    CallR( ErrBTDeferGotoBookmark( pfucb, bm, fTrue ) );
    pfucb->locLogical = locOnCurBM;

#ifdef DEBUG
    if ( !(BOOL)UlConfigOverrideInjection( 44228, fFalse ) )
    {
        err = ErrBTGotoBookmark( pfucb, pfucb->bmCurr, latchReadNoTouch );
        switch ( err )
        {
                case JET_errSuccess:
                    if ( 0 != pfucb->ppib->Level() )
                    {
                        BOOL fVisible;
                        CallS( ErrNDVisibleToCursor( pfucb, &fVisible, NULL ) );
                        Assert( fVisible );
                    }
                    break;

                case JET_errRecordDeleted:
                    Assert( 0 == pfucb->ppib->Level() );
                    break;

                case JET_errDiskIO:
                case_AllDatabaseStorageCorruptionErrs :
                case JET_errOutOfMemory:
                case JET_errDiskFull:
                case JET_errOutOfBuffers:
                    break;

                default:
                    CallS( err );
        }

        BTUp( pfucb );
    }
#endif

    Assert( !Pcsr( pfucb )->FLatched() );
    return JET_errSuccess;
}


ERR ErrDIRGotoPosition( FUCB *pfucb, ULONG ulLT, ULONG ulTotal )
{
    ERR     err;
    DIB     dib;
    FRAC    frac;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !FFUCBSpace( pfucb ) );

    if( !FFUCBSequential( pfucb ) )
    {
        FUCBResetPreread( pfucb );
    }

    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );


    Call( ErrDIRIRefresh( pfucb ) );

    dib.dirflag = fDIRNull;
    dib.pos     = posFrac;
    dib.pbm     = reinterpret_cast<BOOKMARK *>( &frac );

    Assert( ulLT <= ulTotal );

    frac.ulLT       = ulLT;
    frac.ulTotal    = ulTotal;

    Call( ErrBTDown( pfucb, &dib, latchReadNoTouch ) );

    pfucb->locLogical = locOnCurBM;
    AssertDIRGet( pfucb );
    return err;

HandleError:
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


ERR ErrDIRDown( FUCB *pfucb, DIB *pdib )
{
    ERR     err;

    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( pdib->pos != posFrac );
    Assert( !FFUCBSpace( pfucb ) );

    CheckFUCB( pfucb->ppib, pfucb );

    if ( !FFUCBSequential( pfucb ) && !pfucb->u.pfcb->FTypeLV() )
    {
        FUCBResetPreread( pfucb );
    }

    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );

    err = ErrBTDown( pfucb, pdib, latchReadTouch );
    if ( err < 0 )
    {
        if( ( JET_errRecordNotFound == err )
            && ( posDown == pdib->pos ) )
        {
            Call( ErrBTDeferGotoBookmark( pfucb, *(pdib->pbm), fFalse ) );
            pfucb->locLogical = locOnSeekBM;

            err = ErrERRCheck( JET_errRecordNotFound );
        }

        goto HandleError;
    }

    Assert( Pcsr( pfucb )->FLatched() );

    switch ( err )
    {
        case wrnNDFoundGreater:
            pfucb->locLogical = locAfterSeekBM;
            break;

        case wrnNDFoundLess:
            pfucb->locLogical = locBeforeSeekBM;
            break;

        default:
            err = JET_errSuccess;
        case JET_wrnUniqueKey:
            pfucb->locLogical = locOnCurBM;
            break;
    }

    return err;

HandleError:
    Assert( err < 0 );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( err != JET_errNoCurrentRecord );
    return err;
}


ERR ErrDIRDownKeyData(
    FUCB            *pfucb,
    const KEY&      key,
    const DATA&     data )
{
    ERR             err;
    BOOKMARK        bm;

    ASSERT_VALID( &key );
    ASSERT_VALID( &data );
    Assert( FFUCBSecondary( pfucb ) );
    Assert( locOnFDPRoot == pfucb->locLogical );
    Assert( !FFUCBSpace( pfucb ) );

    if( !FFUCBSequential( pfucb ) )
    {
        FUCBResetPreread( pfucb );
    }

    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );

    bm.key = key;
    if ( FFUCBUnique( pfucb ) )
    {
        bm.data.Nullify();
    }
    else
    {
        bm.data = data;
    }
    Call( ErrDIRGotoBookmark( pfucb, bm ) );
    CallS( err );

    return JET_errSuccess;

HandleError:
    Assert( JET_errNoCurrentRecord != err );
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


VOID DIRUp( FUCB *pfucb )
{
    BTUp( pfucb );
    Assert( !Pcsr( pfucb )->FLatched() );

    pfucb->locLogical = locOnFDPRoot;
}

VOID DIRReleaseLatch( _In_ FUCB* const pfucb )
{
    BTUp( pfucb );
}

ERR ErrDIRNext( FUCB *pfucb, DIRFLAG dirflag )
{
    ERR     err;
    ERR     wrn = JET_errSuccess;
    LOC     locLogicalInitial;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !FFUCBSpace( pfucb ) );

    if( FFUCBSequential( pfucb ) )
    {
        FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
    }
    else
    {
        if( FFUCBPrereadForward( pfucb ) )
        {
            if( !FFUCBPreread( pfucb )
                && FFUCBIndex( pfucb )
                && pfucb->cbSequentialDataRead >= cbSequentialDataPrereadThreshold
                && locOnCurBM == pfucb->locLogical )
            {
                FUCBSetPrereadForward( pfucb, cpgPrereadPredictive );
                Call( ErrDIRRelease( pfucb ) );
                BTUp( pfucb );
            }
        }
        else
        {
            FUCBResetPreread( pfucb );
            pfucb->fPrereadForward = fTrue;
        }
    }

    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );

Refresh:
    Call( ErrDIRIRefresh( pfucb ) );

    locLogicalInitial = pfucb->locLogical;

    switch( pfucb->locLogical )
    {
        case locOnCurBM:
        case locBeforeSeekBM:
            break;

        case locOnSeekBM:
        {
            Call( ErrBTPerformOnSeekBM( pfucb, fDIRFavourNext ) );
            Assert( Pcsr( pfucb )->FLatched() );

            if ( wrnNDFoundGreater == err )
            {
                pfucb->locLogical = locOnCurBM;
                err = JET_errSuccess;
                goto CheckRange;
            }
            else
            {
                Assert( wrnNDFoundLess == err );
            }
            break;
        }

        case locOnFDPRoot:
            return( ErrERRCheck( JET_errNoCurrentRecord ) );

        case locAfterLast:
            return( ErrERRCheck( JET_errNoCurrentRecord ) );

        case locAfterSeekBM:
            Assert( Pcsr( pfucb )->FLatched() );

            pfucb->locLogical = locOnCurBM;
            Assert( Pcsr( pfucb )->FLatched( ) );

            Call( err );
            goto CheckRange;

        default:
        {
            DIB dib;
            Assert( locBeforeFirst == pfucb->locLogical );

            DIRGotoRoot( pfucb );
            dib.dirflag = fDIRNull;
            dib.pos     = posFirst;
            err = ErrDIRDown( pfucb, &dib );
            if ( err < 0 )
            {
                pfucb->locLogical = locBeforeFirst;
                Assert( !Pcsr( pfucb )->FLatched() );

                if ( err == JET_errRecordNotFound )
                {
                    err = ErrERRCheck( JET_errNoCurrentRecord );
                }
            }

            Call( err );
            goto CheckRange;
        }
    }

    Assert( locOnCurBM == pfucb->locLogical ||
            locBeforeSeekBM == pfucb->locLogical ||
            locOnSeekBM == pfucb->locLogical && err != wrnNDFoundGreater );

    err = ErrBTNext( pfucb, dirflag );

    if ( err < 0 )
    {
        if ( JET_errNoCurrentRecord == err )
        {
            pfucb->locLogical = locAfterLast;
        }
        else if ( JET_errRecordDeleted == err )
        {
            BTSetupOnSeekBM( pfucb );
            goto Refresh;
        }
        goto HandleError;
    }

    pfucb->locLogical = locOnCurBM;

CheckRange:
    wrn = err;

    if ( FFUCBLimstat( pfucb ) &&
         FFUCBUpper( pfucb ) &&
         JET_errSuccess == err )
    {
        Call( ErrDIRICheckIndexRange( pfucb, pfucb->kdfCurr.key ) );
    }

    if ( locBeforeFirst != locLogicalInitial )
    {
        Assert( FFUCBSequential( pfucb ) || FFUCBPrereadForward( pfucb ) );
        pfucb->cbSequentialDataRead += pfucb->kdfCurr.key.Cb();
        pfucb->cbSequentialDataRead += pfucb->kdfCurr.data.Cb();
    }

    return wrn != JET_errSuccess ? wrn : err;

HandleError:
    Assert( err < 0 );
    BTUp( pfucb );
    if ( JET_errRecordNotFound == err )
    {
        err = ErrERRCheck( JET_errNoCurrentRecord );
    }
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


ERR ErrDIRPrev( FUCB *pfucb, DIRFLAG dirflag )
{
    ERR     err;
    ERR     wrn = JET_errSuccess;
    LOC     locLogicalInital;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !FFUCBSpace( pfucb ) );

    if( FFUCBSequential( pfucb ) )
    {
        FUCBSetPrereadBackward( pfucb, cpgPrereadSequential );
    }
    else
    {
        if( FFUCBPrereadBackward( pfucb ) )
        {
            if( !FFUCBPreread( pfucb )
                && FFUCBIndex( pfucb )
                && pfucb->cbSequentialDataRead >= cbSequentialDataPrereadThreshold
                && locOnCurBM == pfucb->locLogical )
            {
                FUCBSetPrereadBackward( pfucb, cpgPrereadPredictive );
                Call( ErrDIRRelease( pfucb ) );
                BTUp( pfucb );
            }
        }
        else
        {
            FUCBResetPreread( pfucb );
            pfucb->fPrereadBackward = fTrue;
        }
    }

    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );

Refresh:
    Call( ErrDIRIRefresh( pfucb ) );

    locLogicalInital = pfucb->locLogical;

    switch( pfucb->locLogical )
    {
        case locOnCurBM:
        case locAfterSeekBM:
            break;

        case locOnSeekBM:
        {
            Call( ErrBTPerformOnSeekBM( pfucb, fDIRFavourPrev ) );
            Assert( Pcsr( pfucb )->FLatched() );

            if ( wrnNDFoundLess == err )
            {
                pfucb->locLogical = locOnCurBM;
                err = JET_errSuccess;
                goto CheckRange;
            }
            else
            {
                Assert( wrnNDFoundGreater == err );
            }
            break;
        }

        case locOnFDPRoot:
            return( ErrERRCheck( JET_errNoCurrentRecord ) );

        case locBeforeFirst:
            return( ErrERRCheck( JET_errNoCurrentRecord ) );

        case locBeforeSeekBM:
            Assert( Pcsr( pfucb )->FLatched() );

            pfucb->locLogical = locOnCurBM;
            Assert( Pcsr( pfucb )->FLatched( ) );

            Call( err );
            goto CheckRange;

        default:
        {
            DIB dib;
            Assert( locAfterLast == pfucb->locLogical );

            DIRGotoRoot( pfucb );
            dib.dirflag = fDIRNull;
            dib.pos     = posLast;
            err = ErrDIRDown( pfucb, &dib );
            if ( err < 0 )
            {
                pfucb->locLogical = locAfterLast;
                Assert( !Pcsr( pfucb )->FLatched() );

                if ( err == JET_errRecordNotFound )
                {
                    err = ErrERRCheck( JET_errNoCurrentRecord );
                }
            }

            Call( err );
            goto CheckRange;
        }
    }

    Assert( locOnCurBM == pfucb->locLogical ||
            locAfterSeekBM == pfucb->locLogical ||
            locOnSeekBM == pfucb->locLogical && wrnNDFoundGreater == err );

    err = ErrBTPrev( pfucb, dirflag );

    if ( err < 0 )
    {
        if ( JET_errNoCurrentRecord == err )
        {
            pfucb->locLogical = locBeforeFirst;
        }
        else if ( JET_errRecordDeleted == err )
        {
            BTSetupOnSeekBM( pfucb );
            goto Refresh;
        }

        goto HandleError;
    }

    pfucb->locLogical = locOnCurBM;

CheckRange:
    wrn = err;

    if ( FFUCBLimstat( pfucb ) &&
         !FFUCBUpper( pfucb ) &&
         JET_errSuccess == err )
    {
        Call( ErrDIRICheckIndexRange( pfucb, pfucb->kdfCurr.key ) );
    }

    if ( locAfterLast != locLogicalInital )
    {
        Assert( FFUCBSequential( pfucb ) || FFUCBPrereadBackward( pfucb ) );
        pfucb->cbSequentialDataRead += pfucb->kdfCurr.key.Cb();
        pfucb->cbSequentialDataRead += pfucb->kdfCurr.data.Cb();
    }

    return wrn != JET_errSuccess ? wrn : err;

HandleError:
    Assert( err < 0 );
    BTUp( pfucb );
    if ( JET_errRecordNotFound == err )
    {
        err = ErrERRCheck( JET_errNoCurrentRecord );
    }
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


ERR ErrDIRCheckIndexRange( FUCB *pfucb )
{
    ERR     err;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !FFUCBSpace( pfucb ) );

    CallR( ErrDIRIRefresh( pfucb ) );

    if( locOnCurBM == pfucb->locLogical
        || locOnSeekBM == pfucb->locLogical )
    {
        Call( ErrDIRICheckIndexRange( pfucb, pfucb->bmCurr.key ) );
    }
    else
    {
        Call( ErrERRCheck( JET_errNoCurrentRecord ) );
    }

HandleError:
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}




VOID DIRSetActiveSpaceRequestReserve( FUCB * const pfucb, const CPG cpgRequired )
{
    pfucb->cpgSpaceRequestReserve = cpgRequired;
}
VOID DIRResetActiveSpaceRequestReserve( FUCB * const pfucb )
{
    DIRSetActiveSpaceRequestReserve( pfucb, 0 );
}

ERR ErrDIRInsert( FUCB          *pfucb,
                  const KEY&    key,
                  const DATA&   data,
                  DIRFLAG       dirflag,
                  RCE           *prcePrimary )
{
    ERR     err;

    ASSERT_VALID( &key );
    ASSERT_VALID( &data );
    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !FFUCBSpace( pfucb ) );

    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );

    Assert( !Pcsr( pfucb )->FLatched() );

    Call( ErrBTInsert( pfucb, key, data, dirflag, prcePrimary ) );

    if ( dirflag & fDIRBackToFather )
    {
        DIRUp( pfucb );
        Assert( locOnFDPRoot == pfucb->locLogical );
    }
    else
    {
        Call( ErrBTRelease( pfucb ) );
        pfucb->locLogical = locOnCurBM;
    }

HandleError:
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


ERR ErrDIRInitAppend( FUCB *pfucb )
{
    Assert( !FFUCBSpace( pfucb ) );

    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );

    return JET_errSuccess;
}


ERR ErrDIRAppend( FUCB          *pfucb,
                  const KEY&    key,
                  const DATA&   data,
                  DIRFLAG       dirflag )
{
    ERR     err;

    ASSERT_VALID( &key );
    ASSERT_VALID( &data );
    Assert( !FFUCBSpace( pfucb ) );
    Assert( !data.FNull() );
    Assert( !key.FNull() || FFUCBRepair( pfucb ) );
    Assert( LevelFUCBNavigate( pfucb ) == pfucb->ppib->Level() );

    if ( locOnFDPRoot == pfucb->locLogical )
    {
        CallR( ErrBTInsert( pfucb, key, data, dirflag ) );
        Assert( Pcsr( pfucb )->FLatched() );
        pfucb->locLogical = locOnCurBM;
        return err;
    }

    Assert( Pcsr( pfucb )->FLatched() );
    Assert( locOnCurBM == pfucb->locLogical );
    AssertDIRGet( pfucb );

    if ( FKeysEqual( pfucb->kdfCurr.key, key )
        && FDataEqual( pfucb->kdfCurr.data, data ) )
    {
        Assert( FFUCBSecondary( pfucb ) );
        Assert( pfcbNil != pfucb->u.pfcb );
        Assert( !FFMPIsTempDB( pfucb->u.pfcb->Ifmp() ) );
        Assert( pfucb->u.pfcb->FTypeSecondaryIndex() );
        Assert( pfucb->u.pfcb->Pidb() != pidbNil );
        Assert( !pfucb->u.pfcb->Pidb()->FPrimary() );
        Assert( !pfucb->u.pfcb->Pidb()->FUnique() );
        Assert( pfucb->u.pfcb->Pidb()->FMultivalued() );
        err = JET_errSuccess;
        return err;
    }

    Assert( FFUCBRepair( pfucb ) && key.FNull()
            || CmpKey( pfucb->kdfCurr.key, key ) < 0
            || CmpData( pfucb->kdfCurr.data, data ) < 0 );

    Call( ErrBTAppend( pfucb, key, data, dirflag ) );

    pfucb->locLogical = locOnCurBM;
    Assert( Pcsr( pfucb )->FLatched() );
    return err;

HandleError:
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


ERR ErrDIRTermAppend( FUCB *pfucb )
{
    ERR     err = JET_errSuccess;

    DIRUp( pfucb );
    return err;
}


ERR ErrDIRDelete( FUCB *pfucb, DIRFLAG dirflag, RCE *prcePrimary )
{
    ERR     err;

    Assert( !FFUCBSpace( pfucb ) );
    CheckFUCB( pfucb->ppib, pfucb );

    Call( ErrDIRIRefresh( pfucb ) );

    if ( locOnCurBM == pfucb->locLogical )
    {
        err = ErrBTFlagDelete( pfucb, dirflag, prcePrimary );
    }
    else
    {
        Assert( !Pcsr( pfucb )->FLatched() );
        err = ErrERRCheck( JET_errNoCurrentRecord );
        return err;
    }

HandleError:
    CallS( ErrBTRelease( pfucb ) );
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


ERR ErrDIRReplace( FUCB *pfucb, const DATA& data, DIRFLAG dirflag )
{
    ERR     err;

    ASSERT_VALID( &data );
    Assert( !FFUCBSpace( pfucb ) );
    CheckFUCB( pfucb->ppib, pfucb );

    Call( ErrDIRIRefresh( pfucb ) );

    if ( locOnCurBM != pfucb->locLogical )
    {
        Assert( !Pcsr( pfucb )->FLatched( ) );
        err = ErrERRCheck( JET_errNoCurrentRecord );
        return err;
    }

    Call( ErrBTReplace( pfucb, data, dirflag ) );
    Assert( locOnCurBM == pfucb->locLogical );

HandleError:
    CallS( ErrBTRelease( pfucb ) );
    Assert( !Pcsr( pfucb )->FLatched( ) );
    return err;
}


ERR ErrDIRGetLock( FUCB *pfucb, DIRLOCK dirlock )
{
    ERR     err = JET_errSuccess;

    Assert( !FFUCBSpace( pfucb ) );
    Assert( pfucb->ppib->Level() > 0 );
    Assert( !Pcsr( pfucb )->FLatched() );

    Call( ErrDIRIRefresh( pfucb ) );

    switch ( pfucb->locLogical )
    {
        case locOnCurBM:
            break;

        case locOnFDPRoot:
        case locDeferMoveFirst:
            Assert( fFalse );

        default:
            Assert( pfucb->locLogical == locBeforeSeekBM ||
                    pfucb->locLogical == locAfterSeekBM ||
                    pfucb->locLogical == locAfterLast ||
                    pfucb->locLogical == locBeforeFirst ||
                    pfucb->locLogical == locOnSeekBM );
            return( ErrERRCheck( JET_errNoCurrentRecord ) );
    }

    Call( ErrBTLock( pfucb, dirlock ) );
    CallS( err );

    return err;

HandleError:
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


template< typename TDelta >
ERR ErrDIRDelta(
    FUCB            *pfucb,
    INT             cbOffset,
    const TDelta    tDelta,
    TDelta          *const pOldValue,
    DIRFLAG         dirflag )
{
    ERR     err;

    Assert( !FFUCBSpace( pfucb ) );
    CheckFUCB( pfucb->ppib, pfucb );

    Call( ErrDIRIRefresh( pfucb ) );
    Assert( locOnCurBM == pfucb->locLogical );

    Call( ErrBTDelta( pfucb, cbOffset, tDelta, pOldValue, dirflag ) );

HandleError:
    CallS( ErrBTRelease( pfucb ) );
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}



ERR ErrDIRIndexRecordCount( FUCB *pfucb, ULONG64 *pullCount, ULONG64 ullCountMost, BOOL fNext )
{
    ERR     err;
    INST *  pinst   = PinstFromPfucb( pfucb );
    CPG     cpgPreread;
    BOOKMARK bm;
    BYTE    *pb = NULL;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !Pcsr( pfucb )->FLatched() );

    *pullCount = 0;


    if ( 0 == ullCountMost )
    {

        cpgPreread = cpgPrereadSequential;

        ullCountMost = ullMax;
    }
    else if ( ullCountMost > ( (ULONG64)cpgPrereadSequential * (ULONG64)cpgPrereadSequential ) )
    {

        cpgPreread = cpgPrereadSequential;
    }


    else
    {

        cpgPreread = 2;
    }


    if ( fNext )
    {
        FUCBSetPrereadForward( pfucb, cpgPreread );
    }
    else
    {
        FUCBSetPrereadBackward( pfucb, cpgPreread );
    }

    err = ErrDIRIRefresh( pfucb );
    if ( JET_errNoCurrentRecord == err && pfucb->locLogical == locDeferMoveFirst )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    else
    {
        Call( err );
    }

    if ( locOnCurBM != pfucb->locLogical )
    {
        Call( ErrERRCheck( JET_errNoCurrentRecord ) );
    }

    Assert( pfucb->bmCurr.key.Cb() + pfucb->bmCurr.data.Cb() > 0 );
    pb = new BYTE[ pfucb->bmCurr.key.Cb() + pfucb->bmCurr.data.Cb() ];
    if ( pb == NULL )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }
    bm.Nullify();
    if ( pfucb->bmCurr.key.Cb() > 0 )
    {
        bm.key.suffix.SetPv( pb );
        pfucb->bmCurr.key.CopyIntoBuffer( pb );
        bm.key.suffix.SetCb( pfucb->bmCurr.key.Cb() );
    }
    if ( pfucb->bmCurr.data.Cb() > 0 )
    {
        bm.data.SetPv( pb + bm.key.Cb() );
        pfucb->bmCurr.data.CopyInto( bm.data );
    }

    Call( ErrBTGet( pfucb ) );


    if ( cpgPreread == 2 )
    {
        Assert( ullCountMost <= ( cpgPrereadSequential * cpgPrereadSequential ) );
        cpgPreread = LFunctionalMin(    CPG( ( ( ullCountMost + Pcsr( pfucb )->ILine() ) /
                                            LFunctionalMax( Pcsr( pfucb )->Cpage().Clines(), 1 ) ) -
                                            cpgPreread ),
                                        cpgPrereadSequential );

        if ( fNext )
        {
            FUCBSetPrereadForward( pfucb, cpgPreread );
        }
        else
        {
            FUCBSetPrereadBackward( pfucb, cpgPreread );
        }
    }

    forever
    {
        err = pinst->ErrCheckForTermination();
        if ( err < JET_errSuccess )
        {
            BTUp( pfucb );
            break;
        }

        if ( ++(*pullCount) == ullCountMost )
        {
            BTUp( pfucb );
            break;
        }

        if ( fNext )
        {
            err = ErrBTNext( pfucb, fDIRNull );
        }
        else
        {
            err = ErrBTPrev( pfucb, fDIRNull );
        }

        if ( err < 0 )
        {
            BTUp( pfucb );
            break;
        }

        if ( FFUCBLimstat( pfucb ) )
        {
            err = ErrDIRICheckIndexRange( pfucb, pfucb->kdfCurr.key );
            if ( err < 0 )
            {
                Assert( !Pcsr( pfucb )->FLatched() );
                break;
            }
        }
    }

    if ( err >= 0 || err == JET_errNoCurrentRecord )
    {
        err = JET_errSuccess;
    }

HandleError:
    if ( pb != NULL )
    {
        if ( err == JET_errSuccess )
        {
            FUCBResetLimstat( pfucb );
            err = ErrDIRGotoBookmark( pfucb, bm );
        }
        else
        {
            (VOID)ErrDIRGotoBookmark( pfucb, bm );
        }
        bm.Nullify();
        delete[] pb;
        pb = NULL;
    }
    Assert( pb == NULL );
    Assert( !Pcsr( pfucb )->FLatched() );
    FUCBResetPreread( pfucb );
    return err;
}


ERR ErrDIRComputeStats( FUCB *pfucb, INT *pcnode, INT *pckey, INT *pcpage )
{
    ERR     err;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !FFUCBLimstat( pfucb ) );
    Assert( locOnFDPRoot == pfucb->locLogical );

    CallR( ErrBTComputeStats( pfucb, pcnode, pckey, pcpage ) );
    return err;
}



ERR ErrDIRBeginTransaction( PIB *ppib, const TRXID trxid, const JET_GRBIT grbit )
{
    ERR         err         = JET_errSuccess;
    BOOL        fInRwlTrx   = fFalse;
    INST        * pinst     = PinstFromPpib( ppib );


    if ( JET_errSuccess != ppib->ErrRollbackFailure() )
    {
        Error( ErrERRCheck( JET_errRollbackError ) );
    }

    if ( ppib->Level() == 0 )
    {
        if ( pinst->FRecovering() &&
             !pinst->m_fTrxNewestSetByRecovery &&
             !( grbit & bitTransactionWritableDuringRecovery ) )
        {
            if ( Ptls()->fInCallback )
            {
                Assert( pinst->m_pver->m_pbucketGlobalHead == NULL );
                Assert( trxMax == TrxOldest( pinst ) );
            }
            else
            {
                Call( ErrERRCheck( JET_errTransactionsNotReadyDuringRecovery ) );
            }
        }

        ppib->PIBSetTrxContext();

        Assert( !( grbit & bitTransactionWritableDuringRecovery ) || ( ppib->procid != procidReadDuringRecovery ) );

        Assert( prceNil == ppib->prceNewest );
        if ( ( grbit & JET_bitTransactionReadOnly ) ||
             ( pinst->FRecovering() && !( grbit & bitTransactionWritableDuringRecovery ) ) )
            ppib->SetFReadOnlyTrx();
        else
            ppib->ResetFReadOnlyTrx();

        PIBSetTrxBegin0( ppib );
    }
    else if( prceNil != ppib->prceNewest )
    {
        pinst->RwlTrx( ppib ).EnterAsReader();
        Assert( !( ppib->FReadOnlyTrx() ) );
        fInRwlTrx = fTrue;
    }
    else
    {
        Assert( ppib->FPIBCheckTrxContext() );
    }

    if ( ppib->Level() != 0 && !ppib->FPIBCheckTrxContext() )
    {
        PIBReportSessionSharingViolation( ppib );
        FireWall( "SessionSharingViolationBeginTrx" );
        Call( ErrERRCheck( JET_errSessionSharingViolation ) );
    }
    else
    {
        Assert( !( grbit & JET_bitDistributedTransaction ) );

        CallS( ErrLGBeginTransaction( ppib ) );
        VERBeginTransaction( ppib, trxid );

        OSTrace(
            JET_tracetagTransactions,
            OSFormat( "Session=[0x%p:0x%x] begin trx to level %d", ppib, ppib->trxBegin0, ppib->Level() ) );

        ETTransactionBegin( ppib, (void*)(ULONG_PTR)ppib->trxBegin0, ppib->Level() );
    }

HandleError:
    if ( fInRwlTrx )
    {
        Assert( prceNil != ppib->prceNewest );
        Assert( !( ppib->FReadOnlyTrx() ) );
        pinst->RwlTrx( ppib ).LeaveAsReader();
    }

    return err;
}

ERR ErrDIRCommitTransaction( PIB *ppib, JET_GRBIT grbit, DWORD cmsecDurableCommit, JET_COMMIT_ID *pCommitId )
{
    ERR     err = JET_errSuccess;
    FUCB    *pfucb;

    const BOOL  fCommit0        = ( 1 == ppib->Level() );
    const BOOL  fLoggedBegin0   = ppib->FBegin0Logged();
    const BOOL  fSessionHasRCE  = ( prceNil != ppib->prceNewest );
    const BOOL  fLazyCommit     = ( grbit | ppib->grbitCommitDefault ) & JET_bitCommitLazyFlush;
    TRX         trxCommit0Hint  = trxMax;

    CheckPIB( ppib );
    Assert( ppib->Level() > 0 );
    if ( !fLazyCommit && cmsecDurableCommit != 0 )
    {
        AssertSz( fFalse, "non-lazy transactions should not have durable commit timeout" );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    INST * const pinst = PinstFromPpib( ppib );
    LOG  * const plog = pinst->m_plog;

    if ( !ppib->FPIBCheckTrxContext() )
    {
        PIBReportSessionSharingViolation( ppib );
        FireWall( "SessionSharingViolationCommitTrx" );
        return ErrERRCheck( JET_errSessionSharingViolation );
    }

    if( fSessionHasRCE )
    {
        pinst->RwlTrx( ppib ).EnterAsReader();
    }


    if ( fCommit0 )
    {
        if ( fLoggedBegin0 )
        {
            if ( fLazyCommit )
            {
                PERFOpt( ppib->FUserSession() ? cDIRUserRWLazyTrxCommit0.Inc( pinst ) : cDIRSystemRWLazyTrxCommit0.Inc( pinst ) );
            }
            else
            {
                PERFOpt( ppib->FUserSession() ? cDIRUserRWDurableTrxCommit0.Inc( pinst ) : cDIRSystemRWDurableTrxCommit0.Inc( pinst ) );
            }
        }
        else
        {
            PERFOpt( ppib->FUserSession() ? cDIRUserROTrxCommit0.Inc( pinst ) : cDIRSystemROTrxCommit0.Inc( pinst ) );
        }
    }

    LGPOS   lgposCommitRec;
    Call( ErrLGCommitTransaction(
                ppib,
                LEVEL( ppib->Level() - 1 ),
                !!(grbit & JET_bitCommitRedoCallback),
                &trxCommit0Hint,
                &lgposCommitRec ) );

    if ( fCommit0 )
    {
        if ( fLoggedBegin0 )
        {
            Assert( trxMax != trxCommit0Hint );
            Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );

            Assert( CmpLgpos( lgposCommitRec, lgposMin ) != 0 &&
                    CmpLgpos( lgposCommitRec, lgposMax ) != 0 );

            plog->LGAddLgpos( &lgposCommitRec, sizeof( LRCOMMIT0 ) - 1 );

            if ( pCommitId != NULL )
            {
                pCommitId->signLog = *(JET_SIGNATURE *)&PinstFromPpib( ppib )->m_plog->SignLog();
                pCommitId->commitId = (__int64)lgposCommitRec.qw;
            }

            ppib->lgposCommit0 = lgposCommitRec;
        }
        else
        {
            Assert( CmpLgpos( lgposCommitRec, lgposMin ) == 0 ||
                    CmpLgpos( lgposCommitRec, lgposMax ) == 0 );
        }

        if ( fSessionHasRCE
            && fLoggedBegin0 )
        {

            if ( !fLazyCommit )
            {
                err = ErrLGWaitForWrite( ppib, &lgposCommitRec );
                Assert( err >= 0 ||
                        ( plog->FNoMoreLogWrite() && JET_errLogWriteFail == err ) );


                Call( err );
            }
            else if ( cmsecDurableCommit > 0 )
            {
                ErrLGScheduleWrite( ppib, cmsecDurableCommit, lgposCommitRec );
            }
        }

        Assert( trxMax == ppib->trxCommit0 );

        if ( fSessionHasRCE )
        {
            ppib->trxCommit0 = trxPrecommit;

            const TRX   trxCommit0  = TRX( AtomicExchangeAdd( (LONG *)&pinst->m_trxNewest, TRXID_INCR ) ) + TRXID_INCR;

#ifdef DEBUG
            if ( ( trxCommit0 % 4096 ) == 0 )
            {
                UtilSleep( TickOSTimeCurrent() % 100 );
            }
#endif

            ppib->trxCommit0 = trxCommit0;

            Assert( ( ppib->trxCommit0 % 4 ) == 0 );
        }
        else
        {
            ppib->trxCommit0 = trxCommit0Hint;
        }
    }

    VERCommitTransaction( ppib );

    OSTrace(
        JET_tracetagTransactions,
        OSFormat( "Session=[0x%p:0x%x] commit trx to level %d", ppib, ppib->trxBegin0, ppib->Level() ) );

    ETTransactionCommit( ppib, (void*)(ULONG_PTR)ppib->trxBegin0, ppib->Level() );

    if ( fCommit0 && fSessionHasRCE )
    {
        Assert( 0 == ppib->Level() );
        Assert( prceNil == ppib->prceNewest );
    }

    FUCB    *pfucbNext;
    for ( pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; pfucb = pfucbNext )
    {
        pfucbNext = pfucb->pfucbNextOfSession;

        Assert( 0 == pfucb->levelReuse || pfucb->levelReuse >= pfucb->levelOpen );

        if ( pfucb->levelOpen > ppib->Level() )
        {
            pfucb->levelOpen = ppib->Level();
        }
        
        if ( pfucb->levelReuse > ppib->Level() )
        {
            pfucb->levelReuse = ppib->Level();
        }
        
        if ( FFUCBUpdatePrepared( pfucb ) )
        {
            Assert( pfucb->levelPrep <= ppib->Level() + 1 );
            if ( ppib->Level() + 1 == pfucb->levelPrep )
            {
                pfucb->levelPrep = ppib->Level();
            }

            AssertSz(
                ppib->Level() > 0 || !FFUCBInsertReadOnlyCopyPrepared( pfucb ),
                "Illegal attempt to migrate read-only copy to level 0." );

            EnforceSz(
                ppib->Level() > 0 || !FFUCBUpdateSeparateLV( pfucb ),
                "IllegalLvUpdMigrationToLevel0" );
        }

        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() ||
                LevelFUCBNavigate( pfucb ) <= ppib->Level() + 1 );
        if ( LevelFUCBNavigate( pfucb ) > ppib->Level() )
        {
            FUCBSetLevelNavigate( pfucb, ppib->Level() );
        }

        if ( fCommit0 )
        {
            Assert( 0 == ppib->Level() );
            Assert( prceNil == ppib->prceNewest );
            FUCBResetVersioned( pfucb );

            if ( FFUCBDeferClosed( pfucb ) )
                FUCBCloseDeferredClosed( pfucb );
        }
        else
        {
            Assert( ppib->Level() > 0 );
        }
    }

    if ( fCommit0 )
    {
        PIBResetTrxBegin0( ppib );
        ppib->PIBResetTrxContext();

        if ( pinst->FRecovering() )
        {
            pinst->m_sigTrxOldestDuringRecovery.Set();
        }
    }

HandleError:
    if( fSessionHasRCE )
    {
        pinst->RwlTrx( ppib ).LeaveAsReader();
    }

#ifdef DEBUG
    if ( fCommit0 )
    {
        Assert( err < 0 || 0 == ppib->Level() );
    }
    else
    {
        Assert( ppib->Level() > 0 );
    }
#endif

    return err;
}


ERR ErrDIRRollback( PIB *ppib, JET_GRBIT grbit )
{
    ERR         err = JET_errSuccess;
    FUCB        *pfucb;
    const BOOL  fRollbackToLevel0   = ( 1 == ppib->Level() );
    const BOOL  fSessionHasRCE      = ( prceNil != ppib->prceNewest );

    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fTrue );

    CheckPIB( ppib );
    Assert( ppib->Level() > 0 );
    INST        *pinst = PinstFromPpib( ppib );

    if ( !ppib->FPIBCheckTrxContext() )
    {
        PIBReportSessionSharingViolation( ppib );
        FireWall( "SessionSharingViolationRollbackTrx" );
        FOSSetCleanupState( fCleanUpStateSaved );
        return ErrERRCheck( JET_errSessionSharingViolation );
    }

    if ( JET_errSuccess != ppib->ErrRollbackFailure() )
    {
        FOSSetCleanupState( fCleanUpStateSaved );
        return ErrERRCheck( JET_errRollbackError );
    }

    Assert( trxMax == ppib->trxCommit0 );


    if ( fRollbackToLevel0 )
    {
        if ( ppib->FBegin0Logged() )
        {
            PERFOpt( ppib->FUserSession() ? cDIRUserRWTrxRollback0.Inc( pinst ) : cDIRSystemRWTrxRollback0.Inc( pinst ) );
        }
        else
        {
            PERFOpt( ppib->FUserSession() ? cDIRUserROTrxRollback0.Inc( pinst ) : cDIRSystemROTrxRollback0.Inc( pinst ) );
        }
    }

    for ( pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; pfucb = pfucb->pfucbNextOfSession )
    {
        Assert( pinst->FRecovering() || LevelFUCBNavigate( pfucb ) <= ppib->Level() );
        if ( LevelFUCBNavigate( pfucb ) == ppib->Level() )
        {
            FUCBSetLevelNavigate( pfucb, LEVEL( ppib->Level() - 1 ) );
            DIRBeforeFirst( pfucb );

            Assert( !FFUCBUpdatePrepared( pfucb ) ||
                    pfucb->levelPrep == ppib->Level() );
        }

        Assert( !FFUCBUpdatePrepared( pfucb ) ||
            pfucb->levelPrep <= ppib->Level() );
        if ( FFUCBUpdatePreparedLevel( pfucb, ppib->Level() ) )
        {
            RECIFreeCopyBuffer( pfucb );
            FUCBResetUpdateFlags( pfucb );
        }

        if ( pfucbNil != pfucb->pfucbLV
            && ( pfucb->pfucbLV->levelOpen >= ppib->Level() ||
                 pfucb->pfucbLV->levelReuse >= ppib->Level() ) )
        {
            DIRClose( pfucb->pfucbLV );
            pfucb->pfucbLV = pfucbNil;
        }
    }


    err = ErrVERRollback( ppib );
    CallSx( err, JET_errRollbackError );

    OSTrace(
        JET_tracetagTransactions,
        OSFormat(
            "Session=[0x%p:0x%x] rollback trx to level %d with error %d (0x%x)",
            ppib,
            ppib->trxBegin0,
            ppib->Level(),
            err,
            err ) );

    ETTransactionRollback( ppib, (void*)(ULONG_PTR)ppib->trxBegin0, ppib->Level() );

    if ( err < JET_errSuccess )
    {
        FOSSetCleanupState( fCleanUpStateSaved );
        return err;
    }

    err = ErrLGRollback( ppib, 1, fRollbackToLevel0, !!(grbit & JET_bitRollbackRedoCallback) );
    if ( err == JET_errLogWriteFail || err == JET_errDiskFull )
    {
        err = JET_errSuccess;
    }
    CallS( err );

#ifdef DEBUG
    if ( fRollbackToLevel0 )
    {
        Assert( 0 == ppib->Level() );

        Assert( prceNil == ppib->prceNewest );
    }
    else
    {
        Assert( ppib->Level() > 0 );
    }
#endif

    if ( !pinst->FRecovering() )
    {
        ENTERREADERWRITERLOCK enterRwlTrx( &pinst->RwlTrx( ppib ), fTrue, fSessionHasRCE );
        for ( pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; )
        {
            FUCB * const    pfucbNext                   = pfucb->pfucbNextOfSession;
            const BOOL      fOpenedInThisTransaction    = ( pfucb->levelOpen > ppib->Level() );
            const BOOL      fDeferClosed                = FFUCBDeferClosed( pfucb );

            if ( fRollbackToLevel0 )
            {
                Assert( 0 == ppib->Level() );
                Assert( prceNil == ppib->prceNewest );
                FUCBResetVersioned( pfucb );
            }
            else
            {
                Assert( ppib->Level() > 0 );
            }

            if ( fOpenedInThisTransaction || fDeferClosed )
            {
                BTReleaseBM( pfucb );

                if ( fOpenedInThisTransaction || fRollbackToLevel0 )
                {
                    if ( !fDeferClosed
                        && FFUCBLongValue( pfucb )
                        && pfucbNil != pfucb->pfucbTable )
                    {
                        Assert( pfcbNil != pfucb->u.pfcb );
                        Assert( pfucb->u.pfcb->FTypeLV() );
                        Assert( !Pcsr( pfucb )->FLatched() );
                        Enforce( pfucb->pfucbTable->pfucbLV == pfucb );
                        pfucb->pfucbTable->pfucbLV = pfucbNil;
                    }
                    else if ( pfucbNil != pfucb->pfucbTable )
                    {
                        Assert( fDeferClosed );
                        Assert( FFUCBSecondary( pfucb )
                            || FFUCBLongValue( pfucb ) );
                    }

                    FUCBCloseDeferredClosed( pfucb );
                }
            }

            pfucb = pfucbNext;
        }
    }

    if ( fRollbackToLevel0 )
    {
        if ( pinst->FRecovering() )
        {
            for ( pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; pfucb = pfucb->pfucbNextOfSession )
            {
                FUCBResetVersioned( pfucb );
            }
        }

        PIBResetTrxBegin0( ppib );
        ppib->PIBResetTrxContext();

        if ( pinst->FRecovering() )
        {
            pinst->m_sigTrxOldestDuringRecovery.Set();
        }
    }

    FOSSetCleanupState( fCleanUpStateSaved );
    return err;
}


ERR ErrDIRGetRootField( _Inout_ FUCB* const pfucb, _In_ const NodeRootField noderf, _In_ const LATCH latch )
{
    Expected( noderfSpaceHeader != noderf );
    return ErrBTGetRootField( pfucb, noderf, latch );
}

ERR ErrDIRSetRootField( _In_ FUCB* const pfucb, _In_ const NodeRootField noderf, _In_ const DATA& dataRootField )
{
    Expected( noderfSpaceHeader != noderf );
    return ErrBTSetRootField( pfucb, noderf, dataRootField );
}



LOCAL ERR ErrDIRICheckIndexRange( FUCB *pfucb, const KEY& key )
{
    ERR     err;

    err = ErrFUCBCheckIndexRange( pfucb, key );

    Assert( err >= 0 || err == JET_errNoCurrentRecord );
    if ( err == JET_errNoCurrentRecord )
    {
        if( Pcsr( pfucb )->FLatched() )
        {
            DIRUp( pfucb );
        }

        if ( FFUCBUpper( pfucb ) )
        {
            DIRAfterLast( pfucb );
        }
        else
        {
            DIRBeforeFirst( pfucb );
        }
    }

    return err;
}


LOCAL ERR ErrDIRIIRefresh( FUCB * const pfucb )
{
    ERR     err;
    DIB     dib;
    FUCB    * pfucbIdx;

    Assert( locDeferMoveFirst == pfucb->locLogical );
    Assert( !Pcsr( pfucb )->FLatched() );

    if ( pfucb->pfucbCurIndex )
    {
        pfucbIdx = pfucb->pfucbCurIndex;
    }
    else
    {
        pfucbIdx = pfucb;
    }

    DIRGotoRoot( pfucbIdx );

    dib.dirflag = fDIRNull;
    dib.pos     = posFirst;
    err = ErrBTDown( pfucbIdx, &dib, latchReadTouch );
    if ( err < 0 )
    {
        Assert( err != JET_errNoCurrentRecord );
        if ( JET_errRecordNotFound == err )
        {
            err = ErrERRCheck( JET_errNoCurrentRecord );
        }

        Assert( !Pcsr( pfucb )->FLatched() );
        pfucb->locLogical = locDeferMoveFirst;

        goto HandleError;
    }

    Assert( JET_errSuccess == err ||
            wrnBFPageFault == err ||
            wrnBFPageFlushPending == err ||
            wrnBFBadLatchHint == err );

    pfucbIdx->locLogical = locOnCurBM;

    if ( pfucbNil != pfucb->pfucbCurIndex )
    {
        BOOKMARK    bm;

        bm.data.Nullify();
        bm.key.prefix.Nullify();
        bm.key.suffix = pfucbIdx->kdfCurr.data;

        Call( ErrDIRGotoBookmark( pfucb, bm ) );
    }

    Call( ErrBTRelease( pfucbIdx ) );
    CallS( err );

HandleError:
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( JET_errRecordDeleted != err );
    Assert( JET_errRecordNotFound != err );
    return err;
}

