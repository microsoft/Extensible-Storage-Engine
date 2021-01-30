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


ERR ErrFUCBOpen( PIB *ppib, IFMP ifmp, FUCB **ppfucb, const LEVEL level )
{
    ERR     err     = JET_errSuccess;
    FUCB*   pfucb   = pfucbNil;

    Assert( ppfucb );
    Assert( pfucbNil == *ppfucb );

    pfucb = new( PinstFromPpib( ppib ) ) FUCB( ppib, ifmp );
    if ( pfucbNil == pfucb )
    {
        err = ErrERRCheck( JET_errOutOfCursors );
        goto HandleError;
    }

    Assert( !FFUCBUpdatable( pfucb ) );
    if ( !g_rgfmp[ ifmp ].FReadOnlyAttach() )
    {
        FUCBSetUpdatable( pfucb );
    }

    if ( level > 0 )
    {
        Assert( PinstFromPpib( ppib )->RwlTrx( ppib ).FWriter() );
        pfucb->levelOpen = level;

        FUCBSetIndex( pfucb );
        FUCBSetSecondary( pfucb );
    }
    else
    {
        pfucb->levelOpen = ppib->Level();
    }

    *ppfucb = pfucb;

    ppib->critCursors.Enter();
    pfucb->pfucbNextOfSession = ( FUCB * )ppib->pfucbOfSession;
    ppib->pfucbOfSession = pfucb;
    ppib->cCursors++;
    ppib->critCursors.Leave();

HandleError:
    return err;
}


VOID FUCBClose( FUCB * pfucb, FUCB * pfucbPrev )
{
    PIB *   ppib    = pfucb->ppib;

    Assert( pfcbNil == pfucb->u.pfcb );
    Assert( pscbNil == pfucb->u.pscb );

    Assert( !Pcsr( pfucb )->FLatched() );

    Assert( !FFUCBCurrentSecondary( pfucb ) );

    Assert( NULL == pfucb->pvBMBuffer );
    Assert( NULL == pfucb->pvRCEBuffer );

    ppib->critCursors.Enter();

    Assert( pfucbNil == pfucbPrev || pfucbPrev->pfucbNextOfSession == pfucb );
    if ( pfucbNil == pfucbPrev )
    {
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

    delete pfucb;
}


VOID FUCBCloseAllCursorsOnFCB(
    PIB         * const ppib,
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
            break;
        }

        Assert( pfucbT->ppib == ppib || ppibNil == ppib );
        if ( ppibNil == ppib )
        {
            RECReleaseKeySearchBuffer( pfucbT );
            FILEReleaseCurrentSecondary( pfucbT );
            BTReleaseBM( pfucbT );
            RECIFreeCopyBuffer( pfucbT );
        }

        Assert( pfucbT->u.pfcb == pfcb );


        pfucbT->u.pfcb->Unlink( pfucbT, fTrue );


        FUCBClose( pfucbT );
    }
    Assert( pfcb->WRefCount() == 0 );
}


VOID FUCBSetIndexRange( FUCB *pfucb, JET_GRBIT grbit )
{

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

