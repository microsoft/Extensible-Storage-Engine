// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"


#ifdef DEBUG
BOOL FFUCBValidTableid( const JET_SESID sesid, const JET_TABLEID tableid )
{
    BOOL fValid = fFalse;

    TRY
    {
        fValid = CResource::FCallingProgramPassedValidJetHandle( JET_residFUCB, (void*)tableid );
    }
    EXCEPT( efaExecuteHandler )
    {
        //  nop
    }
    ENDEXCEPT;

    return fValid;
}
#endif


FUCB::FUCB( PIB* const ppibIn, const IFMP ifmpIn )
    :   CZeroInit( sizeof( FUCB ) ),
        pvtfndef( &vtfndefInvalidTableid ),
        ppib( ppibIn ),
        ifmp( ifmpIn ),
        ls( JET_LSNil )
{
    //  ensure bit array is aligned for LONG_PTR traversal
    Assert( (LONG_PTR)rgbitSet % sizeof(LONG_PTR) == 0 );

    kdfCurr.Nullify();
    bmCurr.Nullify();
    dataSearchKey.Nullify();
    dataWorkBuf.Nullify();
}

FUCB::~FUCB()
{
    RECReleaseKeySearchBuffer( this );
    RECRemoveCursorFilter( this );
    FUCBRemoveEncryptionKey( this );
    if ( JET_LSNil != ls )
    {
        INST*           pinst   = PinstFromIfmp( ifmp );
        JET_CALLBACK    pfn     = (JET_CALLBACK)PvParam( pinst, JET_paramRuntimeCallback );

        Assert( NULL != pfn );
        (*pfn)(
            JET_sesidNil,
            JET_dbidNil,
            JET_tableidNil,
            JET_cbtypFreeCursorLS,
            (VOID *)ls,
            NULL,
            NULL,
            NULL );
    }
}


//+api
//  ErrFUCBOpen
//  ------------------------------------------------------------------------
//  ERR ErrFUCBOpen( PIB *ppib, IFMP ifmp, FUCB **ppfucb );
//
//  Creates an open FUCB. At this point, no FCB is assigned yet.
//
//  PARAMETERS  ppib    PIB of this user
//              ifmp    Database Id
//              ppfucb  Address of pointer to FUCB.  If *ppfucb == NULL,
//                      an FUCB is allocated and **ppfucb is set to its
//                      address.  Otherwise, *ppfucb is assumed to be
//                      pointing at a closed FUCB, to be reused in the open.
//
//  RETURNS     JET_errSuccess if successful.
//                  JET_errOutOfCursors
//
//  SIDE EFFECTS    links the newly opened FUCB into the chain of open FUCBs
//                  for this session.
//
//  SEE ALSO        ErrFUCBClose
//-
ERR ErrFUCBOpen( PIB *ppib, IFMP ifmp, FUCB **ppfucb, const LEVEL level )
{
    ERR     err     = JET_errSuccess;
    FUCB*   pfucb   = pfucbNil;

    Assert( ppfucb );
    Assert( pfucbNil == *ppfucb );

    //  allocate a new FUCB
    //
    pfucb = new( PinstFromPpib( ppib ) ) FUCB( ppib, ifmp );
    if ( pfucbNil == pfucb )
    {
        err = ErrERRCheck( JET_errOutOfCursors );
        goto HandleError;
    }

    //  if this database is not read only then default the cursor to updatable
    //
    Assert( !FFUCBUpdatable( pfucb ) );
    if ( !g_rgfmp[ ifmp ].FReadOnlyAttach() )
    {
        FUCBSetUpdatable( pfucb );
    }

    // If level is non-zero, this indicates we're opening the FUCB via a proxy
    // (ie. concurrent CreateIndex).
    if ( level > 0 )
    {
        // If opening by proxy, then proxy should already have obtained rwlTrx.
        Assert( PinstFromPpib( ppib )->RwlTrx( ppib ).FWriter() );
        pfucb->levelOpen = level;

        // Must have these flags set BEFORE linking into session list to
        // ensure rollback doesn't close the FUCB prematurely.
        FUCBSetIndex( pfucb );
        FUCBSetSecondary( pfucb );
    }
    else
    {
        pfucb->levelOpen = ppib->Level();
    }

    //  link new FUCB into user chain, only when success is sure
    //  as unlinking NOT handled in error
    //
    *ppfucb = pfucb;

    //  link the fucb now
    //
    //  NOTE: The only concurrency involved is when concurrent create
    //  index must create an FUCB for the session.  This is always
    //  at the head of the FUCB list.  Note that the concurrent create
    //  index thread doesn't remove the FUCB from the session list,
    //  except on error.  The original session will normally close the
    //  FUCB created by proxy.
    //  So, only need a mutex wherever the head of the list is modified
    //  or if scanning and we want to look at secondary index FUCBs.
    ppib->critCursors.Enter();
    pfucb->pfucbNextOfSession = ( FUCB * )ppib->pfucbOfSession;
    ppib->pfucbOfSession = pfucb;
    ppib->cCursors++;
    ppib->critCursors.Leave();

HandleError:
    return err;
}


//+api
//  FUCBClose
//  ------------------------------------------------------------------------
//  FUCBClose( FUCB *pfucb )
//
//  Closes an active FUCB, optionally returning it to the free FUCB pool.
//  All the pfucb->pcsr are freed.
//
//  PARAMETERS      pfucb       FUCB to close.  Should be open. pfucb->csr should
//                                  hold no page.
//
//  SIDE EFFECTS    Unlinks the closed FUCB from the FUCB chain of its
//                     associated PIB and FCB.
//
//  SEE ALSO        ErrFUCBOpen
//-
VOID FUCBClose( FUCB * pfucb, FUCB * pfucbPrev )
{
    PIB *   ppib    = pfucb->ppib;

    //  our FCB/SCB should have been released
    //
    Assert( pfcbNil == pfucb->u.pfcb );
    Assert( pscbNil == pfucb->u.pscb );

    //  we shouldn't have any pages latched
    //
    Assert( !Pcsr( pfucb )->FLatched() );

    // Current secondary index should already have been closed.
    Assert( !FFUCBCurrentSecondary( pfucb ) );

    //  bookmark and RCE should have already been released
    //
    Assert( NULL == pfucb->pvBMBuffer );
    Assert( NULL == pfucb->pvRCEBuffer );

    //  locate the pfucb in this thread and take it out of the fucb list
    //
    ppib->critCursors.Enter();

    Assert( pfucbNil == pfucbPrev || pfucbPrev->pfucbNextOfSession == pfucb );
    if ( pfucbNil == pfucbPrev )
    {
        //  locate the pfucb in this thread and take it out of the fucb list
        //
        pfucbPrev = (FUCB *)( (BYTE *)&ppib->pfucbOfSession - (BYTE *)&( (FUCB *)0 )->pfucbNextOfSession );
        while ( pfucbPrev->pfucbNextOfSession != pfucb )
        {
            pfucbPrev = pfucbPrev->pfucbNextOfSession;
            Assert( pfucbPrev != pfucbNil );
        }
    }

    pfucbPrev->pfucbNextOfSession = pfucb->pfucbNextOfSession;

    Assert( ppib->cCursors > 0 );
    ppib->cCursors--;

    ppib->critCursors.Leave();

    //  release the fucb
    //
    delete pfucb;
}


VOID FUCBCloseAllCursorsOnFCB(
    PIB         * const ppib,   // pass ppibNil when closing cursors because we're terminating
    FCB         * const pfcb )
{
    Assert( pfcb->IsUnlocked() );

    forever
    {
        pfcb->Lock();
        FUCB *const pfucbT = pfcb->Pfucb();
        pfcb->Unlock();

        if ( pfucbNil == pfucbT )
        {
            // This is the way out of the loop.
            break;
        }

        // This function only called for temp. tables, for
        // table being rolled back, or when purging database (in
        // which case ppib is ppibNil), so no other session should
        // have open cursor on it.
        Assert( pfucbT->ppib == ppib || ppibNil == ppib );
        if ( ppibNil == ppib )
        {
            // If terminating, may have to manually clean up some FUCB resources.
            RECReleaseKeySearchBuffer( pfucbT );
            FILEReleaseCurrentSecondary( pfucbT );
            BTReleaseBM( pfucbT );
            RECIFreeCopyBuffer( pfucbT );
        }

        Assert( pfucbT->u.pfcb == pfcb );

        //  unlink the FCB without moving it to the avail LRU list because
        //      we will be synchronously purging the FCB shortly

        pfucbT->u.pfcb->Unlink( pfucbT, fTrue );

        //  close the FUCB

        FUCBClose( pfucbT );
    }
    Assert( pfcb->WRefCount() == 0 );
}


VOID FUCBSetIndexRange( FUCB *pfucb, JET_GRBIT grbit )
{
    //  set limstat
    //  also set the preread flags

    FUCBResetPreread( pfucb );
    FUCBSetLimstat( pfucb );
    if ( grbit & JET_bitRangeUpperLimit )
    {
        FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
        FUCBSetUpper( pfucb );
    }
    else
    {
        FUCBSetPrereadBackward( pfucb, cpgPrereadSequential );
        FUCBResetUpper( pfucb );
    }
    if ( grbit & JET_bitRangeInclusive )
    {
        FUCBSetInclusive( pfucb );
    }
    else
    {
        FUCBResetInclusive( pfucb );
    }

    return;
}


VOID FUCBResetIndexRange( FUCB *pfucb )
{
    if ( pfucb->pfucbCurIndex )
    {
        FUCBResetLimstat( pfucb->pfucbCurIndex );
        FUCBResetPreread( pfucb->pfucbCurIndex );
    }

    FUCBResetLimstat( pfucb );
    FUCBResetPreread( pfucb );
}


INLINE INT CmpPartialKeyKey( const KEY& key1, const KEY& key2 )
{
    INT     cmp;

    if ( key1.FNull() || key2.FNull() )
    {
        cmp = key1.Cb() - key2.Cb();
    }
    else
    {
        cmp = CmpKey( key1, key2 );
    }

    return cmp;
}

ERR ErrFUCBCheckIndexRange( FUCB *pfucb, const KEY& key )
{
    KEY     keyLimit;

    FUCBAssertValidSearchKey( pfucb );
    keyLimit.prefix.Nullify();
    keyLimit.suffix.SetPv( pfucb->dataSearchKey.Pv() );
    keyLimit.suffix.SetCb( pfucb->dataSearchKey.Cb() );

    const INT   cmp             = CmpPartialKeyKey( key, keyLimit );
    BOOL        fOutOfRange;

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
        fOutOfRange = !FFUCBInclusive( pfucb );
    }

    ERR     err;
    if ( fOutOfRange )
    {
        FUCBResetLimstat( pfucb );
        FUCBResetPreread( pfucb );
        err = ErrERRCheck( JET_errNoCurrentRecord );
    }
    else
    {
        err = JET_errSuccess;
    }

    return err;
}

