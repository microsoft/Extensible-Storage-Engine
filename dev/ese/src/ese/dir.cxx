// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

//  prototypes of DIR internal routines
//
LOCAL ERR ErrDIRICheckIndexRange( FUCB *pfucb, const KEY& key );
LOCAL ERR ErrDIRIIRefresh( FUCB * const pfucb );
INLINE ERR ErrDIRIRefresh( FUCB * const pfucb )
{
    return ( locDeferMoveFirst != pfucb->locLogical ?
                JET_errSuccess :
                ErrDIRIIRefresh( pfucb ) );
}


#ifdef PERFMON_SUPPORT

//  perf stats

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

#endif // PERFMON_SUPPORT


//  *****************************************
//  DIR API functions
//

// *********************************************************
// ******************** DIR API Routines ********************
//

//  ***********************************************
//  constructor/destructor routines
//

//  creates a directory
//  get a new extent from space that is initialized to a directory
//
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

    //  check currency is on "parent" FDP
    //
    Assert( !Pcsr( pfucb )->FLatched( ) );
    Assert( locOnFDPRoot == pfucb->locLogical );

    fSPFlags |= fSPNewFDP;

    // WARNING: Should only create an unversioned extent if its parent was
    // previously created and versioned in the same transaction as the
    // creation of this extent.
    Assert( ( fSPFlags & fSPUnversionedExtent )
            || !FFMPIsTempDB( pfucb->ifmp ) );       // Don't version creation of temp. table.

    //  create FDP
    //
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


//  ***********************************************
//  Open/Close routines
//

//  opens a cursor on given ifmp, pgnoFDP
//
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

    //  set return pfucb
    //
    *ppfucb = pfucb;
    return JET_errSuccess;
}

//  open cursor, don't touch root page
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

    //  set return pfucb
    //
    *ppfucb = pfucb;
    return JET_errSuccess;
}

//  open cursor on given FCB
//
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

    //  set return pfucb
    //
    *ppfucb = pfucb;
    return JET_errSuccess;
}

//  open cursor on given FCB on behalf of another session
//
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

    //  set return pfucb
    //
    *ppfucb = pfucb;
    return JET_errSuccess;
}


//  closes cursor
//  frees space allocated for logical currency
//
VOID DIRClose( FUCB *pfucb )
{
    //  this cursor should not be already defer closed
    //
    Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() || !FFUCBDeferClosed(pfucb) );

    RECReleaseKeySearchBuffer( pfucb );
    FILEReleaseCurrentSecondary( pfucb );

    BTClose( pfucb );
}


//  ***************************************************
//  RETRIEVE OPERATIONS
//

//  currency is implemented at BT level,
//  so DIRGet just does a BTRefresh()
//  returns a latched page iff the node access is successful
//
ERR ErrDIRGet( FUCB *pfucb )
{
    ERR     err;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !FFUCBSpace( pfucb ) );

    //  UNDONE: special case
    //      if on current BM and page cached and
    //      timestamp not changed and
    //      node has not been versioned
    //
    if ( locOnCurBM == pfucb->locLogical )
    {
        Call( ErrBTGet( pfucb ) );
        Assert( Pcsr( pfucb )->FLatched() );
        return err;
    }

    CallR( ErrDIRIRefresh( pfucb ) );

    //  check logical currency status
    //
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

///     case locOnSeekBM:
///         return ErrERRCheck( JET_errRecordDeleted );
///         break;

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

//  gets fractional postion of current node in directory
//
ERR ErrDIRGetPosition( FUCB *pfucb, ULONG *pulLT, ULONG *pulTotal )
{
    ERR     err;
    ULONG   ulLT;
    ULONG   ulTotal;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !FFUCBSpace( pfucb ) );

    //  refresh logical currency
    //
    Call( ErrDIRIRefresh( pfucb ) );

    //  return error if not on a record
    //
    switch ( pfucb->locLogical )
    {
        case locOnCurBM:
        case locOnSeekBM:
            break;
        default:
            return ErrERRCheck( JET_errNoCurrentRecord );
    }

    //  get approximate position of node.
    //
    Call( ErrBTGetPosition( pfucb, &ulLT, &ulTotal ) );
    CallS( err );

    Assert( ulLT <= ulTotal );
    *pulLT = ulLT;
    *pulTotal = ulTotal;

HandleError:
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


//  reverses a DIRGet() operation
//  releases latch on page after recording bm of node or seek in pfucb
//
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
///         AssertDIRGet( pfucb );
            Call( ErrBTRelease( pfucb ) );
            break;

        case locAfterSeekBM:
        case locBeforeSeekBM:
        {
            BOOKMARK    bm;

            FUCBAssertValidSearchKey( pfucb );
            bm.Nullify();

            //  first byte is segment counter
            //
            bm.key.suffix.SetPv( pfucb->dataSearchKey.Pv() );
            bm.key.suffix.SetCb( pfucb->dataSearchKey.Cb() );
            Call( ErrBTDeferGotoBookmark( pfucb, bm, fFalse/*no touch*/ ) );
            pfucb->locLogical = locOnSeekBM;
            break;
        }

        default:
            //  should be impossible, but return success just in case
            //  (okay to return success because we shouldn't have a latch
            //  even if the locLogical is an unexpected value)
            AssertSz( fFalse, "Unexpected locLogical" );
            err = JET_errSuccess;
            break;
    }

    Assert( !Pcsr( pfucb )->FLatched() );
    return err;

HandleError:
    Assert( err < 0 );

    //  bookmark could not be saved
    //  move up
    //
    Assert( JET_errOutOfMemory == err );
    DIRUp( pfucb );

    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}

//  ***************************************************
//  POSITIONING OPERATIONS
//

//  called from JetGotoBookmark
//      bookmark may not be valid
//      therefore, logical currency of cursor can be changed
//      only after we move to bookmark successfully
//      also, save bookmark and release latch
//
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


//  go to given bookmark
//  saves bookmark in cursor
//  next DIRGet() call will return an error if bookmark is not valid
//      this is an internal bookmark [possibly on/from a secondary index]
//      so bookmark should be valid
//
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

    //  copy given bookmark to cursor, we will need to touch the data page buffer
    //
    CallR( ErrBTDeferGotoBookmark( pfucb, bm, fTrue/*Touch*/ ) );
    pfucb->locLogical = locOnCurBM;

#ifdef DEBUG
    //  check that bookmark is valid
    //
    if ( !(BOOL)UlConfigOverrideInjection( 44228, fFalse ) )
    {
        err = ErrBTGotoBookmark( pfucb, pfucb->bmCurr, latchReadNoTouch );
        switch ( err )
        {
                case JET_errSuccess:
                    //  if we suddenly lost visibility on the node, it must mean
                    //  that we're at level 0 and the node deletion was suddenly
                    //  committed underneath us
                    if ( 0 != pfucb->ppib->Level() )
                    {
                        BOOL fVisible;
                        CallS( ErrNDVisibleToCursor( pfucb, &fVisible, NULL ) );
                        Assert( fVisible );
                    }
                    break;

                case JET_errRecordDeleted:
                    //  node must have gotten expunged by RCEClean (only possible
                    //  if we're at level 0)
                    Assert( 0 == pfucb->ppib->Level() );
                    break;

                case JET_errDiskIO:                     //  (#48313 -- RFS testing causes JET_errDiskIO)
                case_AllDatabaseStorageCorruptionErrs : //  (#86323 -- Repair testing causes JET_errReadVerifyFailure )
                case JET_errOutOfMemory:                //  (#146720 -- RFS testing causes JET_errOutOfMemory)
                case JET_errDiskFull:                   //  (W7#456103 -- Testing on small disks causes JET_errDiskFull)
                case JET_errOutOfBuffers:
                    break;

                default:
                    //  force an assert
                    CallS( err );
        }

        BTUp( pfucb );
    }
#endif

    Assert( !Pcsr( pfucb )->FLatched() );
    return JET_errSuccess;
}


//  goes to fractional position in directory
//
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

    //  set cursor navigation level for rollback support
    //
    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );

    //  UNDONE: do we need to refresh here???

    //  refresh logical currency
    //
    Call( ErrDIRIRefresh( pfucb ) );

    dib.dirflag = fDIRNull;
    dib.pos     = posFrac;
    dib.pbm     = reinterpret_cast<BOOKMARK *>( &frac );

    Assert( ulLT <= ulTotal );

    frac.ulLT       = ulLT;
    frac.ulTotal    = ulTotal;

    //  position fractionally on node.  Move up preserving currency
    //  in case down fails.
    //
    Call( ErrBTDown( pfucb, &dib, latchReadNoTouch ) );

    pfucb->locLogical = locOnCurBM;
    AssertDIRGet( pfucb );
    return err;

HandleError:
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


//  seek down to a key or position in the directory
//      returns errSuccess, wrnNDFoundGreater or wrnNDFoundLess
//
ERR ErrDIRDown( FUCB *pfucb, DIB *pdib )
{
    ERR     err;

    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( pdib->pos != posFrac );
    Assert( !FFUCBSpace( pfucb ) );

    CheckFUCB( pfucb->ppib, pfucb );

    //  set preread flags if we are not navigating sequentially
    if ( !FFUCBSequential( pfucb ) && !pfucb->u.pfcb->FTypeLV() )
    {
        FUCBResetPreread( pfucb );
    }

    //  set cursor navigation level for rollback support
    //
    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );

    err = ErrBTDown( pfucb, pdib, latchReadTouch );
    if ( err < 0 )
    {
        if( ( JET_errRecordNotFound == err )
            && ( posDown == pdib->pos ) )
        {
            //  we didn't find the record we were looking for. save the bookmark
            //  so that a DIRNext/DIRPrev will work properly
            Call( ErrBTDeferGotoBookmark( pfucb, *(pdib->pbm), fFalse ) );
            pfucb->locLogical = locOnSeekBM;

            err = ErrERRCheck( JET_errRecordNotFound );
        }

        goto HandleError;
    }

    Assert( Pcsr( pfucb )->FLatched() );

    //  bookmark in pfucb has not been set to bm of current node
    //  but that will be done at the time of latch release
    //
    //  save physical position of cursor with respect to node
    //
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
            //  FALL THROUGH to set locLogical
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


//  seeks secondary index record corresponding to NC key + primary key
//
ERR ErrDIRDownKeyData(
    FUCB            *pfucb,
    const KEY&      key,
    const DATA&     data )
{
    ERR             err;
    BOOKMARK        bm;

    //  this routine should only be called with secondary indexes.
    //
    ASSERT_VALID( &key );
    ASSERT_VALID( &data );
    Assert( FFUCBSecondary( pfucb ) );
    Assert( locOnFDPRoot == pfucb->locLogical );
    Assert( !FFUCBSpace( pfucb ) );

    if( !FFUCBSequential( pfucb ) )
    {
        FUCBResetPreread( pfucb );
    }

    //  set cursor navigation level for rollback support
    //
    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );

    //  goto bookmark pointed by NC key + primary key
    //
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


//  CONSIDER: not needed for one-level trees?
//
//  moves cursor to root -- releases latch, if any
//
VOID DIRUp( FUCB *pfucb )
{
    BTUp( pfucb );
    Assert( !Pcsr( pfucb )->FLatched() );

    pfucb->locLogical = locOnFDPRoot;
}

//
//  releases latch, if any, but not to change locLogical
//
VOID DIRReleaseLatch( _In_ FUCB* const pfucb )
{
    BTUp( pfucb );
}

//  moves cursor to node after current bookmark
//  if cursor is after current bookmark, gets current record
//
ERR ErrDIRNext( FUCB *pfucb, DIRFLAG dirflag )
{
    ERR     err;
    ERR     wrn = JET_errSuccess;
    LOC     locLogicalInitial;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !FFUCBSpace( pfucb ) );

    //  set preread flags if we are navigating sequentially
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
                && locOnCurBM == pfucb->locLogical )    //  can't call DIRRelease() if locBefore/AfterSeekBM because there may not be a bookmark available
            {
                FUCBSetPrereadForward( pfucb, cpgPrereadPredictive );
                //  release to save bookmark and nullify csr to force a BTDown
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

    //  set cursor navigation level for rollback support
    //
    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );

Refresh:
    //  check currency and refresh if necessary
    //
    Call( ErrDIRIRefresh( pfucb ) );

    locLogicalInitial = pfucb->locLogical;

    //  switch action based on logical cursor location
    //
    switch( pfucb->locLogical )
    {
        case locOnCurBM:
        case locBeforeSeekBM:
            break;

        case locOnSeekBM:
        {
            //  re-seek to key and if foundLess, fall through to MoveNext
            //
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

            //  set currency on current
            //
            pfucb->locLogical = locOnCurBM;
            Assert( Pcsr( pfucb )->FLatched( ) );

            Call( err );
            goto CheckRange;

        default:
        {
            DIB dib;
            Assert( locBeforeFirst == pfucb->locLogical );

            //  move to root.
            //
            DIRGotoRoot( pfucb );
            dib.dirflag = fDIRNull;
            dib.pos     = posFirst;
            err = ErrDIRDown( pfucb, &dib );
            if ( err < 0 )
            {
                //  restore currency.
                //
                pfucb->locLogical = locBeforeFirst;
                Assert( !Pcsr( pfucb )->FLatched() );

                //  polymorph error code.
                //
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
            //  moved past last node
            //
            pfucb->locLogical = locAfterLast;
        }
        else if ( JET_errRecordDeleted == err )
        {
            //  node was deleted from under the cursor
            //  reseek to logical bm and move to next node
            //
            BTSetupOnSeekBM( pfucb );
            goto Refresh;
        }
        goto HandleError;
    }

    pfucb->locLogical = locOnCurBM;

CheckRange:
    wrn = err;

    //  check index range
    //
    if ( FFUCBLimstat( pfucb ) &&
         FFUCBUpper( pfucb ) &&
         JET_errSuccess == err )
    {
        Call( ErrDIRICheckIndexRange( pfucb, pfucb->kdfCurr.key ) );
    }

    // if we just started, we won't have the preread counting set yet
    if ( locBeforeFirst != locLogicalInitial )
    {
        Assert( FFUCBSequential( pfucb ) || FFUCBPrereadForward( pfucb ) );
        // we want to count the entire row (key + data) as the amount of
        // data the FUCB read in sequence already. 
        // Counting only the data (which used to be the case) wasn't accurate
        // especialy on a secondary index with big index key and small 
        // primary key (ie small data part in the record). 
        //
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


//  moves cursor to node before current bookmark
//
ERR ErrDIRPrev( FUCB *pfucb, DIRFLAG dirflag )
{
    ERR     err;
    ERR     wrn = JET_errSuccess;
    LOC     locLogicalInital;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !FFUCBSpace( pfucb ) );

    //  set preread flags if we are navigating sequentially
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
                && locOnCurBM == pfucb->locLogical )    //  can't call DIRRelease() if locBefore/AfterSeekBM because there may not be a bookmark available
            {
                FUCBSetPrereadBackward( pfucb, cpgPrereadPredictive );
                //  release to save bookmark and nullify csr to force a BTDown
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

    //  set cursor navigation level for rollback support
    //
    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );

Refresh:
    //  check currency and refresh if necessary
    //
    Call( ErrDIRIRefresh( pfucb ) );

    locLogicalInital = pfucb->locLogical;

    //  switch action based on logical cursor location
    //
    switch( pfucb->locLogical )
    {
        case locOnCurBM:
        case locAfterSeekBM:
            break;

        case locOnSeekBM:
        {
            //  re-seek to key and if foundGreater, fall through to MovePrev
            //
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

            //  set currency on current
            //
            pfucb->locLogical = locOnCurBM;
            Assert( Pcsr( pfucb )->FLatched( ) );

            Call( err );
            goto CheckRange;

        default:
        {
            DIB dib;
            Assert( locAfterLast == pfucb->locLogical );

            //  move to root.
            //
            DIRGotoRoot( pfucb );
            dib.dirflag = fDIRNull;
            dib.pos     = posLast;
            err = ErrDIRDown( pfucb, &dib );
            if ( err < 0 )
            {
                //  retore currency.
                //
                pfucb->locLogical = locAfterLast;
                Assert( !Pcsr( pfucb )->FLatched() );

                //  polymorph error code.
                //
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
            //  moved past first node
            //
            pfucb->locLogical = locBeforeFirst;
        }
        else if ( JET_errRecordDeleted == err )
        {
            //  node was deleted from under the cursor
            //  reseek to logical bm and move to next node
            //
            BTSetupOnSeekBM( pfucb );
            goto Refresh;
        }

        goto HandleError;
    }

    pfucb->locLogical = locOnCurBM;

CheckRange:
    wrn = err;

    //  check index range
    //
    if ( FFUCBLimstat( pfucb ) &&
         !FFUCBUpper( pfucb ) &&
         JET_errSuccess == err )
    {
        Call( ErrDIRICheckIndexRange( pfucb, pfucb->kdfCurr.key ) );
    }

    // if we were past the end, we won't have the preread counting set yet
    if ( locAfterLast != locLogicalInital )
    {
        Assert( FFUCBSequential( pfucb ) || FFUCBPrereadBackward( pfucb ) );
        // we want to count the entire row (key + data) as the amount of
        // data the FUCB read in sequence already. 
        // Counting only the data (which used to be the case) wasn't accurate
        // especialy on a secondary index with big index key and small 
        // primary key (ie small data part in the record). 
        //
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


//  ************************************************
//  INDEX RANGE OPERATIONS
//
//  could become part of BT
//
ERR ErrDIRCheckIndexRange( FUCB *pfucb )
{
    ERR     err;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !FFUCBSpace( pfucb ) );

    //  check currency and refresh if necessary
    //
    CallR( ErrDIRIRefresh( pfucb ) );

    //  get key of node for check index range
    //  we don't need to latch the page, we just need the logical bookmark
    //  if we were locDeferMoveFirst the ErrDIRIRefresh above should have
    //  straightened us out
    //
    if( locOnCurBM == pfucb->locLogical
        || locOnSeekBM == pfucb->locLogical )
    {
        Call( ErrDIRICheckIndexRange( pfucb, pfucb->bmCurr.key ) );
    }
    else
    {
        //  UNDONE: deal with locBeforeSeekBM, locAfterSeekBM, locBeforeFirst etc.
        Call( ErrERRCheck( JET_errNoCurrentRecord ) );
    }

HandleError:
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}



//  ************************************************
//  UPDATE OPERATIONS
//

//  Updates DIR / Space to reserve an additional set of reserve pages on the very next leaf
//  page allocation.  This facility only allows the usage of DIRSetActiveSpaceRequestReserve() 
//  across a single leaf page allocation.
//
//  Client (such as lv.cxx) is expected to manage it so that if the reserve is consumed (can 
//  check with CpgDIRActiveSpaceRequestReserve() == cpgDIRReserveConsumed), then the client
//  calls DIRResetActiveSpaceRequestReserve() to avoid any additional leaf page allocations
//  getting the additional reserve (or the reserve allocations being mis-applied to the wrong 
//  earlier than intended page allocation operation).
//
VOID DIRSetActiveSpaceRequestReserve( FUCB * const pfucb, const CPG cpgRequired )
{
    pfucb->cpgSpaceRequestReserve = cpgRequired;
}
VOID DIRResetActiveSpaceRequestReserve( FUCB * const pfucb )
{
    DIRSetActiveSpaceRequestReserve( pfucb, 0 );
}

//  insert key-data-flags into tree
//  cursor should be at FDP root
//      and have no physical currency
//
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

    //  set cursor navigation level for rollback support
    //
    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );

    Assert( !Pcsr( pfucb )->FLatched() );

    Call( ErrBTInsert( pfucb, key, data, dirflag, prcePrimary ) );

    if ( dirflag & fDIRBackToFather )
    {
        //  no need to save bookmark
        //
        DIRUp( pfucb );
        Assert( locOnFDPRoot == pfucb->locLogical );
    }
    else
    {
        //  save bookmark
        //
        Call( ErrBTRelease( pfucb ) );
        pfucb->locLogical = locOnCurBM;
    }

HandleError:
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


//  the following three calls provide an API for inserting secondary keys
//  at Index creation time
//  DIRAppend is used instead of DIRInit for performance reasons
//
ERR ErrDIRInitAppend( FUCB *pfucb )
{
    Assert( !FFUCBSpace( pfucb ) );

    //  set cursor navigation level for rollback support
    //
    FUCBSetLevelNavigate( pfucb, pfucb->ppib->Level() );

    return JET_errSuccess;
}


//  appends key-data at the end of page
//  leaves page latched
//
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

    //  if key-data same as current node, must be
    //  trying to insert duplicate keys into a
    //  multi-valued index.
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
        //  must have been record with multi-value column
        //  with sufficiently similar values (ie. the
        //  indexed portion of the multi-values were
        //  identical) to produce redundant index
        //  entries -- this is okay, we simply have
        //  one index key pointing to multiple multi-values
        err = JET_errSuccess;
        return err;
    }

    Assert( FFUCBRepair( pfucb ) && key.FNull()
            || CmpKey( pfucb->kdfCurr.key, key ) < 0
            || CmpData( pfucb->kdfCurr.data, data ) < 0 );

    Call( ErrBTAppend( pfucb, key, data, dirflag ) );

    //  set logical currency on inserted node.
    //
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


//  flag delete node
//  release latch
//
ERR ErrDIRDelete( FUCB *pfucb, DIRFLAG dirflag, RCE *prcePrimary )
{
    ERR     err;

    Assert( !FFUCBSpace( pfucb ) );
    CheckFUCB( pfucb->ppib, pfucb );

    //  refresh logical currency
    //
    //  UNDONE: should this be a call to ErrDIRGet() instead??
    //  Currently, we're relying on whatever precedes a call
    //  to ErrDIRDelete() to perform visibility checks
    //  (eg. if the same session performs two consecutive
    //  JetDelete() calls on the same record, the only thing
    //  saving us is the call in ErrIsamDelete() to
    //  ErrRECDereferenceLongFieldsInRecord() just before the
    //  call to ErrDIRDelete())
    //
    Call( ErrDIRIRefresh( pfucb ) );

    if ( locOnCurBM == pfucb->locLogical )
    {
        err = ErrBTFlagDelete( pfucb, dirflag, prcePrimary );
///     if( err >= JET_errSuccess )
///         {
///         pfucb->locLogical = locOnSeekBM;
///         }
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


//  replaces data portion of current node
//  releases latch after recording bm of node in pfucb
//
ERR ErrDIRReplace( FUCB *pfucb, const DATA& data, DIRFLAG dirflag )
{
    ERR     err;

    ASSERT_VALID( &data );
    Assert( !FFUCBSpace( pfucb ) );
    CheckFUCB( pfucb->ppib, pfucb );

    //  check currency and refresh if necessary.
    //
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


//  lock record by calling BTLockRecord
//
ERR ErrDIRGetLock( FUCB *pfucb, DIRLOCK dirlock )
{
    ERR     err = JET_errSuccess;

    Assert( !FFUCBSpace( pfucb ) );
    Assert( pfucb->ppib->Level() > 0 );
    Assert( !Pcsr( pfucb )->FLatched() );

    //  check currency and refresh if necessary.
    //
    Call( ErrDIRIRefresh( pfucb ) );

    //  check cursor location
    //
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


//  called by Lv.c
//  calls ErrBTDelta
//
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

    //  check currency and refresh if necessary.
    //
    Call( ErrDIRIRefresh( pfucb ) );
    Assert( locOnCurBM == pfucb->locLogical );

    Call( ErrBTDelta( pfucb, cbOffset, tDelta, pOldValue, dirflag ) );

HandleError:
    CallS( ErrBTRelease( pfucb ) );
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}


//  ****************************************************************
//  STATISTICAL ROUTINES
//

//  counts nodes from current to limit or end of table
//
ERR ErrDIRIndexRecordCount( FUCB *pfucb, ULONG64 *pullCount, ULONG64 ullCountMost, BOOL fNext )
{
    ERR     err;
    INST *  pinst   = PinstFromPfucb( pfucb );
    CPG     cpgPreread;
    BOOKMARK bm;
    BYTE    *pb = NULL;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !Pcsr( pfucb )->FLatched() );

    //  intialize count variable
    //
    *pullCount = 0;

    //  there is no record count limit or the record count limit is larger than
    //  the maximum fanout of the maximum preread

    if ( 0 == ullCountMost )
    {
        //  turn on maximum preread

        cpgPreread = cpgPrereadSequential;

        ullCountMost = ullMax;
    }
    else if ( ullCountMost > ( (ULONG64)cpgPrereadSequential * (ULONG64)cpgPrereadSequential ) )
    {
        //  turn on maximum preread

        cpgPreread = cpgPrereadSequential;
    }

    //  we have a small record count limit

    else
    {
        //  preread two pages so that we can use the first page to estimate
        //  the true number of pages to preread and use the second page to
        //  trigger the preread without taking another cache miss

        cpgPreread = 2;
    }

    //  turn on preread

    if ( fNext )
    {
        FUCBSetPrereadForward( pfucb, cpgPreread );
    }
    else
    {
        FUCBSetPrereadBackward( pfucb, cpgPreread );
    }

    //  refresh logical currency
    //
    err = ErrDIRIRefresh( pfucb );
    if ( JET_errNoCurrentRecord == err && pfucb->locLogical == locDeferMoveFirst )
    {
        //  special-case: empty table
        //
        err = JET_errSuccess;
        goto HandleError;
    }
    else
    {
        Call( err );
    }

    //  return error if not on a record
    //
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

    //  if we have a small record count limit, use the current page to make an
    //  estimate at how much we should preread and set our preread limit to that
    //  amount
    //
    //  CONSIDER:  sample and account for percentage of visible nodes

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

    //  count nodes from current to limit or end of table
    //
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

        //  check index range
        //
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

    //  common exit loop processing
    //
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
            // Keep the existing err code, even if hit the error. At least we did our best
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


//  computes statistics on directory by calling BT level function
//
ERR ErrDIRComputeStats( FUCB *pfucb, INT *pcnode, INT *pckey, INT *pcpage )
{
    ERR     err;

    CheckFUCB( pfucb->ppib, pfucb );
    Assert( !FFUCBLimstat( pfucb ) );
    Assert( locOnFDPRoot == pfucb->locLogical );

    CallR( ErrBTComputeStats( pfucb, pcnode, pckey, pcpage ) );
    return err;
}


//  ********************************************************
//  ************** DIR Transaction Routines ******************
//

//  begins a transaction
//  gets transaction id
//  logs trqansaction
//  records lgpos of begin in ppib [VERBeginTransaction]
//
ERR ErrDIRBeginTransaction( PIB *ppib, const TRXID trxid, const JET_GRBIT grbit )
{
    ERR         err         = JET_errSuccess;
    BOOL        fInCritTrx  = fFalse;
    INST        * pinst     = PinstFromPpib( ppib );

    //  The critical section ensure that the deferred begin trx level
    //  issued by indexer for a updater will not be changed till the indexer
    //  finish changing for the updater's RCEs that need be updated by indexer.

    if ( JET_errSuccess != ppib->ErrRollbackFailure() )
    {
        //  Previous rollback error encountered, no further begin-transactions permitted on this session.
        Error( ErrERRCheck( JET_errRollbackError ) );
    }

    //  log begin transaction.
    //  Must be called first so that lgpos and trx used in ver are consistent.
    //
    if ( ppib->Level() == 0 )
    {
        // Do not allow transactions during recovery until we know what trxBegin0 to assign them
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

        //  Set a trx ctx so we can catch misbehaved clients that move our sessions
        //  unknowingly between threads ...
        ppib->PIBSetTrxContext();

        // Special read during recovery transaction share a common procid and so should not be writable
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
        ppib->CritTrx().Enter();
        Assert( !( ppib->FReadOnlyTrx() ) );
        fInCritTrx = fTrue;
    }
    else
    {
        Assert( ppib->FPIBCheckTrxContext() );
    }

    //  Check the trx ctx so we can catch misbehaved clients that move our sessions
    //  unknowingly between threads ...
    if ( ppib->Level() != 0 && !ppib->FPIBCheckTrxContext() )
    {
        PIBReportSessionSharingViolation( ppib );
        FireWall( "SessionSharingViolationBeginTrx" );
        Call( ErrERRCheck( JET_errSessionSharingViolation ) );
    }
    else
    {
        //  should have been filtered out at ISAM level
        Assert( !( grbit & JET_bitDistributedTransaction ) );

        CallS( ErrLGBeginTransaction( ppib ) );
        VERBeginTransaction( ppib, trxid );

        OSTrace(
            JET_tracetagTransactions,
            OSFormat( "Session=[0x%p:0x%x] begin trx to level %d", ppib, ppib->trxBegin0, ppib->Level() ) );

        ETTransactionBegin( ppib, (void*)(ULONG_PTR)ppib->trxBegin0, ppib->Level() );
    }

HandleError:
    if ( fInCritTrx )
    {
        Assert( prceNil != ppib->prceNewest );
        Assert( !( ppib->FReadOnlyTrx() ) );
        ppib->CritTrx().Leave();
    }

    return err;
}

//  commits transaction
//
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

    //  Check the trx ctx so we can catch misbehaved clients that move our sessions
    //  unknowingly between threads ...
    if ( !ppib->FPIBCheckTrxContext() )
    {
        PIBReportSessionSharingViolation( ppib );
        FireWall( "SessionSharingViolationCommitTrx" );
        return ErrERRCheck( JET_errSessionSharingViolation );
    }

    // This critical section ensures that we don't commit RCE's from underneath CreateIndex.
    if( fSessionHasRCE )
    {
        ppib->CritTrx().Enter();
    }

    //  perf stats

    if ( fCommit0 )
    {
        //  UNDONE: transactions that perform non-logged write operations
        //  will currently get counted as a read-only transaction
        //
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

    // commit trxid's cannot go backwards in the log, otherwise a read transaction during recovery can get
    // its snapshot isolation violated.  So have to allow log to set trxid in sequentially increasing sequence
    //
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

        //  if no RCE's (likely cause they got rolled back),
        //  then don't need durable commit
        //
        if ( fSessionHasRCE
            && fLoggedBegin0 )  //  PrepareToCommit is always force-flushed, so no need to force-flush Commit0
        {

            if ( !fLazyCommit )
            {
                //  remember the minimum requirement to flush.
                err = ErrLGWaitForWrite( ppib, &lgposCommitRec );
                Assert( err >= 0 ||
                        ( plog->FNoMoreLogWrite() && JET_errLogWriteFail == err ) );


                //  CONSIDER: if ErrLGWaitForFlush fails, are we sure our commit record wasn't logged?
                Call( err );
            }
            else if ( cmsecDurableCommit > 0 )
            {
                ErrLGScheduleWrite( ppib, cmsecDurableCommit, lgposCommitRec );
            }
        }

        Assert( trxMax == ppib->trxCommit0 );

        //  if this transaction actually versioned something, then
        //  must use accurate Commit0 to ensure correct visibility
        //  by other sessions, otherwise the hint is fine because
        //  no other session will care
        //
        if ( fSessionHasRCE )
        {
            //  must set ppib->trxCommit0 to sentinel "precommit" value
            //  because there's an infinitessimally small window where we
            //  might increment trxNewest, but before we can assign it to
            //  ppib->trxCommit0, another session begins a transaction and
            //  starts looking at our RCE's (in which case he'll see them
            //  as uncommitted but in actuality, they should be committed
            //  for him since his Begin0 will be greater than our Commit0)
            //
            ppib->trxCommit0 = trxPrecommit;

            //  Get the real trxid now, it is ok to be different than the one logged, as recovery does not
            //  need to do snapshot isolation for logged transactions
            //
            const TRX   trxCommit0  = TRX( AtomicExchangeAdd( (LONG *)&pinst->m_trxNewest, TRXID_INCR ) ) + TRXID_INCR;

#ifdef DEBUG
            if ( ( trxCommit0 % 4096 ) == 0 )
            {
                //  every 512 transactions (or 1k trx's, since a trx is always a multiple of 4),
                //  simulate losing our quantum after we've obtained a trxCommit0 but before
                //  we could assign it to ppib->trxCommit0 to make it visible to all our RCE's,
                //  meaning that if another session begins a transaction in this time, then
                //  without the loop in RCE::TrxCommitted(), they would think that the RCE's
                //  of this session are uncommitted even though they should be treated as
                //  committed
                //
                UtilSleep( TickOSTimeCurrent() % 100 ); //  sleep a random interval up to 100ms
            }
#endif

            // Note that the following will be different than the one we already logged
            //
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
        //  if committing to level 0, ppib should have no more
        //  outstanding versions
        Assert( 0 == ppib->Level() );
        Assert( prceNil == ppib->prceNewest );
    }

    //  set all open cursor transaction levels to new level
    //
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
            // If update was prepared during this transaction, migrate the
            // update to the next lower level.
            Assert( pfucb->levelPrep <= ppib->Level() + 1 );
            if ( ppib->Level() + 1 == pfucb->levelPrep )
            {
                pfucb->levelPrep = ppib->Level();
            }

            //  LV updates no longer allowed at level 0.  This assert detects
            //  any of the following illegal client actions:
            //      --  calling PrepareUpdate() at level 1, doing some
            //          LV operations, then calling CommitTransaction()
            //          before calling Update(), effectively causing the
            //          update to migrate to level 0.
            //      --  called Update(), but the Update() failed
            //          (eg. errWriteConflict) and the client continued
            //          to commit the transaction anyway with the update
            //          still pending
            //      --  beginning a transaction at level 0 AFTER calling
            //          PrepareUpdate(), then committing the transaction
            //          BEFORE calling Update().
            AssertSz(
                ppib->Level() > 0 || !FFUCBInsertReadOnlyCopyPrepared( pfucb ),
                "Illegal attempt to migrate read-only copy to level 0." );

            //  Illegal attempt to migrate outstanding LV update(s) to level 0.
            EnforceSz(
                ppib->Level() > 0 || !FFUCBUpdateSeparateLV( pfucb ),
                "IllegalLvUpdMigrationToLevel0" );
        }

        //  set cursor navigation level for rollback support
        //
        //  Note: This assert used to check for ppib->Level + 2 during batch
        //  index creation and MSU updates (when we had an MSU).
        //
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() ||
                LevelFUCBNavigate( pfucb ) <= ppib->Level() + 1 );
        if ( LevelFUCBNavigate( pfucb ) > ppib->Level() )
        {
            FUCBSetLevelNavigate( pfucb, ppib->Level() );
        }

        if ( fCommit0 )
        {
            //  if committing to level 0, ppib should have no more
            //  outstanding versions
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
        ppib->CritTrx().Leave();
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


//  rolls back a transaction
//
ERR ErrDIRRollback( PIB *ppib, JET_GRBIT grbit )
{
    ERR         err = JET_errSuccess;
    FUCB        *pfucb;
    const BOOL  fRollbackToLevel0   = ( 1 == ppib->Level() );
    const BOOL  fSessionHasRCE      = ( prceNil != ppib->prceNewest );

    //  Start detecting mem alloc during rollback
    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fTrue );

    CheckPIB( ppib );
    Assert( ppib->Level() > 0 );
    INST        *pinst = PinstFromPpib( ppib );

    //  UNDONE: if we can guarantee that our clients are well-behaved, we don't
    //  need this check
    if ( !ppib->FPIBCheckTrxContext() )
    {
        PIBReportSessionSharingViolation( ppib );
        FireWall( "SessionSharingViolationRollbackTrx" );
        //  Stop detecting mem alloc
        FOSSetCleanupState( fCleanUpStateSaved );
        return ErrERRCheck( JET_errSessionSharingViolation );
    }

    if ( JET_errSuccess != ppib->ErrRollbackFailure() )
    {
        //  previous rollback error encountered, no further rollbacks permitted
        //  Stop detecting mem alloc
        FOSSetCleanupState( fCleanUpStateSaved );
        return ErrERRCheck( JET_errRollbackError );
    }

    Assert( trxMax == ppib->trxCommit0 );

    //  perf stats

    if ( fRollbackToLevel0 )
    {
        //  UNDONE: transactions that perform non-logged write operations
        //  will currently get counted as a read-only transaction
        //
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

        //  reset copy buffer if prepared at transaction level
        //  which is being rolled back.
        //
        Assert( !FFUCBUpdatePrepared( pfucb ) ||
            pfucb->levelPrep <= ppib->Level() );
        if ( FFUCBUpdatePreparedLevel( pfucb, ppib->Level() ) )
        {
            //  reset update separate LV and copy buffer status
            //  all long value resources will be freed as a result of
            //  rollback and copy buffer status is reset
            //
            //  UNDONE: all updates should have already been cancelled
            //  in ErrIsamRollback(), but better to be safe than sorry,
            //  so just leave this code here
            RECIFreeCopyBuffer( pfucb );
            FUCBResetUpdateFlags( pfucb );
        }

        //  if LV cursor was opened at this level, close it
        //  UNDONE: identical logic exists at the IsamRollback
        //  level, but that didn't catch the case where we're
        //  in ErrRECSetLongField(), we create the LV tree,
        //  but the subsequent LV operation rolls back.  For
        //  now, it was just safest to duplicate the check in
        //  IsamRollback, but since the check is now done
        //  here, can I simply remove it from IsamRollback??
        if ( pfucbNil != pfucb->pfucbLV
            && ( pfucb->pfucbLV->levelOpen >= ppib->Level() ||
                 pfucb->pfucbLV->levelReuse >= ppib->Level() ) )
        {
            DIRClose( pfucb->pfucbLV );
            pfucb->pfucbLV = pfucbNil;
        }
    }

    //  UNDONE: rollback may fail from resource failure so
    //          we must retry in order to assure success

    //  rollback changes made in transaction
    //
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

    //  log rollback. Must be called after VERRollback to record
    //  the UNDO operations.  Do not handle error.
    err = ErrLGRollback( ppib, 1, fRollbackToLevel0, !!(grbit & JET_bitRollbackRedoCallback) );
    if ( err == JET_errLogWriteFail || err == JET_errDiskFull )
    {
        //  these error codes will lead to crash recovery which will
        //  rollback transaction.
        //
        err = JET_errSuccess;
    }
    CallS( err );

#ifdef DEBUG
    if ( fRollbackToLevel0 )
    {
        Assert( 0 == ppib->Level() );

        //  if rolling back to level 0, ppib should have no more
        //  outstanding versions
        Assert( prceNil == ppib->prceNewest );
    }
    else
    {
        Assert( ppib->Level() > 0 );
    }
#endif

    //  if recovering then we are done. No need to close fucb since they are faked and
    //  not the same behavior as regular fucb which could be deferred.
    //
    if ( !pinst->FRecovering() )
    {
        //  if rollback to level 0 then close deferred closed cursors
        //  if cursor was opened at this level, close it
        //
        ENTERCRITICALSECTION enterCritTrx( &ppib->CritTrx(), fSessionHasRCE );
        for ( pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; )
        {
            FUCB * const    pfucbNext                   = pfucb->pfucbNextOfSession;
            const BOOL      fOpenedInThisTransaction    = ( pfucb->levelOpen > ppib->Level() );
            const BOOL      fDeferClosed                = FFUCBDeferClosed( pfucb );

            if ( fRollbackToLevel0 )
            {
                //  if rolling back to level 0, ppib should have no more
                //  outstanding versions
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
                // A bookmark may still be outstanding if:
                //   1) Cursor opened in this transaction was
                //      not closed before rolling back.
                //   2) Bookmark was allocated by ErrVERIUndoLoggedOper()
                //      on a deferred-closed cursor.
                BTReleaseBM( pfucb );

                if ( fOpenedInThisTransaction || fRollbackToLevel0 )
                {
                    //  pfucbTable is only set for LV and index cursors,
                    //  and it should be impossible to get here with
                    //  non-deferred-closed index cursors because they
                    //  would have been closed in IsamRollback (it's
                    //  possible to get here with open LV cursors
                    //  because an internal transaction in which the
                    //  LV cursor was opened may have failed)
                    if ( !fDeferClosed
                        && FFUCBLongValue( pfucb )
                        && pfucbNil != pfucb->pfucbTable )
                    {
                        //  unlink table from LV cursor
                        //
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

                    // Force-close cursor if it was open at this level (or higher).
                    // or if we're dropping to level 0 and there are still deferred-
                    // closed cursors.
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

    //  Stop detecting mem alloc
    FOSSetCleanupState( fCleanUpStateSaved );
    return err;
}

//  *****************************************
//  external header operations
//

//  get external header
//
//  Get specified root field. JET_wrnColumnNull is returned if the specified root field hasn't been set.
//  Latch is held as long as success or warning is returned.
ERR ErrDIRGetRootField( _Inout_ FUCB* const pfucb, _In_ const NodeRootField noderf, _In_ const LATCH latch )
{
    Expected( noderfSpaceHeader != noderf );
    return ErrBTGetRootField( pfucb, noderf, latch );
}

//  set external header
//
ERR ErrDIRSetRootField( _In_ FUCB* const pfucb, _In_ const NodeRootField noderf, _In_ const DATA& dataRootField )
{
    Expected( noderfSpaceHeader != noderf );
    return ErrBTSetRootField( pfucb, noderf, dataRootField );
}


//  **********************************************************
//  ******************* DIR Internal *************************
//  **********************************************************
//
//  *******************************************
//  DIR internal routines
//

//  checks index range
//  and sets currency if cursor is beyond boundaries
//
LOCAL ERR ErrDIRICheckIndexRange( FUCB *pfucb, const KEY& key )
{
    ERR     err;

    err = ErrFUCBCheckIndexRange( pfucb, key );

    Assert( err >= 0 || err == JET_errNoCurrentRecord );
    if ( err == JET_errNoCurrentRecord )
    {
        if( Pcsr( pfucb )->FLatched() )
        {
            //  if we are called from ErrDIRCheckIndexRange the page may not be latched
            //  (we use the logical bookmark instead)
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


//  cursor is refreshed
//  if cursor status is DeferMoveFirst,
//      go to first node in the CurrentIdx
//  refresh for other cases is handled at BT-level
//
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

    //  go to root of index
    //
    DIRGotoRoot( pfucbIdx );

    //  move to first child
    //
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
        //  go to primary key on primary index
        //
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

