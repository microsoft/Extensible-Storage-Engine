// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include <ctype.h>
#include "_logstream.hxx"

LRPAGEMOVE::LRPAGEMOVE() :
    LRPAGE_( sizeof( *this ) ),
    m_pgnoDest( pgnoNull ),
    m_pgnoLeft( pgnoNull ),
    m_pgnoRight( pgnoNull ),
    m_pgnoParent( pgnoNull ),
    m_dbtimeLeftBefore( dbtimeInvalid ),
    m_dbtimeRightBefore( dbtimeInvalid ),
    m_dbtimeParentBefore( dbtimeInvalid ),
    m_pgnoFDP( pgnoNull ),
    m_objidFDP( objidNil ),
    m_ilineParent( 0xFFFFFFFF ),
    m_fMoveFlags( 0 ),
    m_cbPageHeader( 0 ),
    m_cbPageTrailer( 0 )
{
}

BOOL LRPAGEMOVE::FUnitTest()
{
    LRPAGEMOVE lrpagemove;
    lrpagemove.lrtyp = lrtypPageMove;

    lrpagemove.SetPgnoSource( 2 );
    Assert( 2 == lrpagemove.PgnoSource() );
    lrpagemove.SetPgnoDest( 3 );
    Assert( 3 == lrpagemove.PgnoDest() );
    lrpagemove.SetPgnoRight( 4 );
    Assert( 4 == lrpagemove.PgnoRight() );
    lrpagemove.SetPgnoLeft( 5 );
    Assert( 5 == lrpagemove.PgnoLeft() );
    lrpagemove.SetPgnoParent( 6 );
    Assert( 6 == lrpagemove.PgnoParent() );
    lrpagemove.SetDbtimeSourceBefore( 0x1000 );
    Assert( 0x1000 == lrpagemove.DbtimeSourceBefore() );
    lrpagemove.SetDbtimeLeftBefore( 0x1001 );
    Assert( 0x1001 == lrpagemove.DbtimeLeftBefore() );
    lrpagemove.SetDbtimeRightBefore( 0x1002 );
    Assert( 0x1002 == lrpagemove.DbtimeRightBefore() );
    lrpagemove.SetDbtimeParentBefore( 0x1003 );
    Assert( 0x1003 == lrpagemove.DbtimeParentBefore() );
    lrpagemove.SetDbtimeAfter( 0x1004 );
    Assert( 0x1004 == lrpagemove.DbtimeAfter() );
    lrpagemove.SetPgnoFDP( 7 );
    Assert( 7 == lrpagemove.PgnoFDP() );
    lrpagemove.SetObjidFDP( 100 );
    Assert( 100 == lrpagemove.ObjidFDP() );
    lrpagemove.SetIlineParent( 9 );
    Assert( 9 == lrpagemove.IlineParent() );
    lrpagemove.SetCbPageHeader(1064 );
    lrpagemove.SetCbPageTrailer( 16 );
    Assert( 1064 == lrpagemove.CbPageHeader() );
    Assert( 16 == lrpagemove.CbPageTrailer() );
    Assert( 1080 == lrpagemove.CbTotal() );
    Assert( lrpagemove.PvPageTrailer() > lrpagemove.PvPageHeader() );
    ASSERT_VALID_OBJ( lrpagemove );
    return fTrue;
}


const LRPAGEMOVE * LRPAGEMOVE::PlrpagemoveFromLr( const LR * const plr )
{
    Assert( plr );
    const LRPAGEMOVE * const plrpagemove = reinterpret_cast<const LRPAGEMOVE *>( plr );
    ASSERT_VALID( plrpagemove );
    return plrpagemove;
}

VOID LRPAGEMOVE::AssertValid() const
{
    Assert( lrtypPageMove == lrtyp );
    Assert( pgnoNull != PgnoSource() );
    Assert( pgnoNull != PgnoDest() );
    Assert( PgnoSource() != PgnoDest() );
    Assert( dbtimeInvalid != DbtimeSourceBefore() );
    Assert( DbtimeAfter() > DbtimeSourceBefore() );
    Assert( pgnoNull != PgnoFDP() );
    Assert( PgnoFDP() != PgnoDest() );
    Assert( objidNil != ObjidFDP() );
    Assert( 0 != m_cbPageHeader );
    Assert( 0 != m_cbPageTrailer );

    if ( !FPageMoveRoot() )
    {
        Assert( pgnoNull != PgnoParent() );
        Assert( PgnoSource() != PgnoParent() );
        Assert( PgnoDest() != PgnoParent() );
        Assert( dbtimeInvalid != DbtimeParentBefore() );
        Assert( dbtimeInvalid != DbtimeAfter() );
        Assert( DbtimeAfter() > DbtimeParentBefore() );
        if( pgnoNull == PgnoLeft() )
        {
            Assert( dbtimeInvalid == DbtimeLeftBefore() );
        }
        else
        {
            Assert( dbtimeInvalid != DbtimeLeftBefore() );
            Assert( DbtimeAfter() > DbtimeLeftBefore() );
            Assert( PgnoLeft() != PgnoSource() );
            Assert( PgnoLeft() != PgnoDest() );
            Assert( PgnoLeft() != PgnoParent() );
            Assert( PgnoLeft() != PgnoRight() );
        }
        if( pgnoNull == PgnoRight() )
        {
            Assert( dbtimeInvalid == DbtimeRightBefore() );
        }
        else
        {
            Assert( dbtimeInvalid != DbtimeRightBefore() );
            Assert( DbtimeAfter() > DbtimeRightBefore() );
            Assert( PgnoRight() != PgnoSource() );
            Assert( PgnoRight() != PgnoDest() );
            Assert( PgnoRight() != PgnoParent() );
            Assert( PgnoRight() != PgnoLeft() );
        }
        Assert( PgnoFDP() != PgnoSource() );
        Assert( PgnoFDP() != PgnoLeft() );
        Assert( PgnoFDP() != PgnoRight() );
        Assert( 0xFFFFFFFF != IlineParent() );
        Assert( 0 == m_fMoveFlags );
    }
    else
    {
        Assert( pgnoNull == PgnoParent() );
        Assert( pgnoNull == PgnoLeft() );
        Assert( pgnoNull == PgnoRight() );
        Assert( dbtimeInvalid == DbtimeParentBefore() );
        Assert( dbtimeInvalid == DbtimeLeftBefore() );
        Assert( dbtimeInvalid == DbtimeRightBefore() );
        Assert( 0xFFFFFFFF == IlineParent() );
        Assert( !!FPageMoveRootFDP() == ( PgnoFDP() == PgnoSource() ) );
        Assert( !FPageMoveRootFDP() || ( !FPageMoveRootOE() && !FPageMoveRootAE() ) );
        Assert( !FPageMoveRootOE() || ( !FPageMoveRootFDP() && !FPageMoveRootAE() ) );
        Assert( !FPageMoveRootAE() || ( !FPageMoveRootOE() && !FPageMoveRootFDP() ) );
    }
}


ERR ErrLGIMacroBegin( PIB *ppib, DBTIME dbtime );
ERR ErrLGIMacroEnd( PIB *ppib, DBTIME dbtime, LRTYP lrtyp, const IFMP ifmp, const PGNO * const rgpgno, CPG cpgno, LGPOS *plgpos );


LOCAL ERR ErrLGIDeferBeginTransaction( PIB *ppib );

INLINE ERR ErrLGDeferBeginTransaction( PIB *ppib )
{
    Assert( ppib->Level() > 0 );
    const ERR   err     = ( 0 != ppib->clevelsDeferBegin ?
                                    ErrLGIDeferBeginTransaction( ppib ) :
                                    JET_errSuccess );
    return err;
}



INLINE VOID LGISetTrx( PIB *ppib, LRPAGE_ *plrpage, const VERPROXY * const pverproxy = NULL )
{
    Assert( ppibNil != ppib );
    
    if ( NULL == pverproxy )
    {
        plrpage->le_trxBegin0 = ppib->trxBegin0;
        plrpage->level = ppib->Level();
    }
    else
    {
        Assert( prceNil != pverproxy->prcePrimary );
        plrpage->le_trxBegin0 = pverproxy->prcePrimary->TrxBegin0();
        if ( trxMax == pverproxy->prcePrimary->TrxCommitted() )
        {
            Assert( pverproxy->prcePrimary->Level() > 0 );
            plrpage->level = pverproxy->prcePrimary->Level();
        }
        else
        {
            plrpage->level = 0;

            plrpage->ResetFVersioned();
        }
    }

    if ( ppib->FSessionSystemTask() )
    {
        plrpage->SetFSystemTask();
    }
}

INLINE ERR ErrLGSetDbtimeBeforeAndDirty(
    INST                                * const pinst,
    CSR                                 * const pcsr,
    UnalignedLittleEndian< DBTIME >     * ple_dbtimeBefore,
    UnalignedLittleEndian< DBTIME >     * ple_dbtime,
    ULONG                               * const pfFlagsBefore,
    const BOOL                          fDirty,
    const BOOL                          fForScrub)
{
    ERR                                 err         = JET_errSuccess;

    Assert ( NULL != pinst );
    Assert ( NULL != pcsr );
    Assert ( NULL != ple_dbtimeBefore );
    Assert ( NULL != ple_dbtime );

    if ( fDirty )
    {
        *ple_dbtimeBefore = pcsr->Dbtime();
        *pfFlagsBefore = pcsr->Cpage().FFlags();
        if( fForScrub )
        {
            pcsr->DirtyForScrub();
        }
        else
        {
            pcsr->Dirty();
        }

        if ( pcsr->Dbtime() <= *ple_dbtimeBefore )
        {
            pcsr->RevertDbtime( *ple_dbtimeBefore, *pfFlagsBefore );
            FireWall( "PageDbTimeNotSetByCsrDirty" );
            OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"92bd51ac-258e-44aa-b26f-9c829de354f1" );
            err = ErrERRCheck( JET_errDbTimeCorrupted );
        }
    }
    else
    {
        *ple_dbtimeBefore = dbtimeNil;
        *pfFlagsBefore = 0;
    }

    *ple_dbtime = pcsr->Dbtime();
    Assert( pcsr->Dbtime() == pcsr->Cpage().Dbtime() );

    return err;
}

ERR ErrLGInsert( const FUCB             * const pfucb,
                 CSR                    * const pcsr,
                 const KEYDATAFLAGS&    kdf,
                 const RCEID            rceid,
                 const DIRFLAG          dirflag,
                 LGPOS                  * const plgpos,
                 const VERPROXY         * const pverproxy,
                 const BOOL             fDirtyCSR )
{
    ERR             err;
    DATA            rgdata[4];
    LRINSERT        lrinsert;

    INST *pinst = PinstFromIfmp( pfucb->ifmp );
    LOG *plog = pinst->m_plog;

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );

    Assert( fDirtyCSR || pcsr->FDirty() );

    if ( plog->FLogDisabled() )
    {
        if ( fDirtyCSR )
        {
            pcsr->Dirty();
        }
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( !( dirflag & fDIRNoLog ) );
    Assert( g_rgfmp[pfucb->ifmp].FLogOn() );

    PIB * const ppib = pverproxy != NULL &&
                    trxMax == pverproxy->prcePrimary->TrxCommitted() ?
                        pverproxy->prcePrimary->Pfucb()->ppib :
                        pfucb->ppib;
    Assert( NULL == pverproxy ||
            prceNil != pverproxy->prcePrimary &&
            ( pverproxy->prcePrimary->TrxCommitted() != trxMax ||
              pfucbNil != pverproxy->prcePrimary->Pfucb() &&
              ppibNil != pverproxy->prcePrimary->Pfucb()->ppib ) );

    Assert( ppib->Level() > 0 );

    CallR( ErrLGDeferBeginTransaction( ppib ) );

    lrinsert.lrtyp      = lrtypInsert;

    Assert( pcsr->ILine() >= 0 );
    Assert( pcsr->ILine() < g_cbPage / 8 );
    lrinsert.SetILine( (USHORT)pcsr->ILine() );

    Assert( !lrinsert.FVersioned() );
    Assert( !lrinsert.FDeleted() );
    Assert( !lrinsert.FUnique() );
    Assert( !lrinsert.FSpace() );
    Assert( !lrinsert.FConcCI() );
    Assert( !lrinsert.FSystemTask() );

    if ( !( dirflag & fDIRNoVersion ) )
        lrinsert.SetFVersioned();
    if ( pfucb->u.pfcb->FUnique() )
        lrinsert.SetFUnique();
    if ( FFUCBSpace( pfucb ) )
        lrinsert.SetFSpace();
    if ( NULL != pverproxy )
        lrinsert.SetFConcCI();

    lrinsert.le_rceid       = rceid;
    lrinsert.le_pgnoFDP     = PgnoFDP( pfucb );
    lrinsert.le_objidFDP    = ObjidFDP( pfucb );

    lrinsert.le_procid  = ppib->procid;

    ULONG fFlagsBefore = 0;
    CallR( ErrLGSetDbtimeBeforeAndDirty( pinst, pcsr, &lrinsert.le_dbtimeBefore, &lrinsert.le_dbtime, &fFlagsBefore, fDirtyCSR, fFalse ) )

    lrinsert.dbid           = g_rgfmp[ pfucb->ifmp ].Dbid();
    lrinsert.le_pgno        = pcsr->Pgno();

    LGISetTrx( ppib, &lrinsert, pverproxy );

    lrinsert.SetCbSuffix( USHORT( kdf.key.suffix.Cb() ) );
    lrinsert.SetCbPrefix( USHORT( kdf.key.prefix.Cb() ) );
    lrinsert.SetCbData( USHORT( kdf.data.Cb() ) );

    rgdata[0].SetPv( (BYTE *)&lrinsert );
    rgdata[0].SetCb( sizeof(LRINSERT) );

    rgdata[1].SetPv( kdf.key.prefix.Pv() );
    rgdata[1].SetCb( kdf.key.prefix.Cb() );

    rgdata[2].SetPv( kdf.key.suffix.Pv() );
    rgdata[2].SetCb( kdf.key.suffix.Cb() );

    rgdata[3].SetPv( kdf.data.Pv() );
    rgdata[3].SetCb( kdf.data.Cb() );

    err = plog->ErrLGLogRec( rgdata, 4, fDirtyCSR ? 0 : fLGMacroGoing, ppib->lgposStart.lGeneration, plgpos );

    if ( JET_errSuccess > err && fDirtyCSR )
    {
        Assert ( dbtimeInvalid !=  lrinsert.le_dbtimeBefore && dbtimeNil != lrinsert.le_dbtimeBefore);
        Assert ( pcsr->Dbtime() > lrinsert.le_dbtimeBefore );
        pcsr->RevertDbtime( lrinsert.le_dbtimeBefore, fFlagsBefore );
    }

    return err;
}


ERR ErrLGFlagInsertAndReplaceData( const FUCB           * const pfucb,
                                   CSR          * const pcsr,
                                   const KEYDATAFLAGS&  kdf,
                                   const RCEID          rceidInsert,
                                   const RCEID          rceidReplace,
                                   const DIRFLAG        dirflag,
                                   LGPOS                * const plgpos,
                                   const VERPROXY       * const pverproxy,
                                 const BOOL             fDirtyCSR )

{
    ERR                         err;
    DATA                        rgdata[4];
    LRFLAGINSERTANDREPLACEDATA  lrfiard;

    INST *pinst = PinstFromIfmp( pfucb->ifmp );
    LOG *plog = pinst->m_plog;

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );

    Assert( fDirtyCSR || pcsr->FDirty() );

    if ( plog->FLogDisabled() )
    {
        if ( fDirtyCSR )
        {
            pcsr->Dirty();
        }
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( !( dirflag & fDIRNoLog ) );
    Assert( g_rgfmp[pfucb->ifmp].FLogOn() );

    PIB * const ppib = pverproxy != NULL &&
                    trxMax == pverproxy->prcePrimary->TrxCommitted() ?
                        pverproxy->prcePrimary->Pfucb()->ppib :
                        pfucb->ppib;
    Assert( NULL == pverproxy ||
            prceNil != pverproxy->prcePrimary &&
            ( pverproxy->prcePrimary->TrxCommitted() != trxMax ||
              pfucbNil != pverproxy->prcePrimary->Pfucb() &&
              ppibNil != pverproxy->prcePrimary->Pfucb()->ppib ) );

    Assert( ppib->Level() > 0 );

    CallR( ErrLGDeferBeginTransaction( ppib ) );

    lrfiard.lrtyp       = lrtypFlagInsertAndReplaceData;

    Assert( pcsr->ILine() >= 0 );
    Assert( pcsr->ILine() < g_cbPage / 8 );
    lrfiard.SetILine( (USHORT)pcsr->ILine() );

    Assert( !lrfiard.FVersioned() );
    Assert( !lrfiard.FDeleted() );
    Assert( !lrfiard.FUnique() );
    Assert( !lrfiard.FSpace() );
    Assert( !lrfiard.FConcCI() );
    Assert( !lrfiard.FSystemTask() );

    if ( !( dirflag & fDIRNoVersion ) )
        lrfiard.SetFVersioned();
    if ( pfucb->u.pfcb->FUnique() )
        lrfiard.SetFUnique();
    if ( FFUCBSpace( pfucb ) )
        lrfiard.SetFSpace();
    if ( NULL != pverproxy )
        lrfiard.SetFConcCI();

    lrfiard.le_rceid            = rceidInsert;
    lrfiard.le_rceidReplace     = rceidReplace;
    lrfiard.le_pgnoFDP          = PgnoFDP( pfucb );
    lrfiard.le_objidFDP         = ObjidFDP( pfucb );

    lrfiard.le_procid       = ppib->procid;

    ULONG fFlagsBefore = 0;
    CallR( ErrLGSetDbtimeBeforeAndDirty( pinst, pcsr, &lrfiard.le_dbtimeBefore, &lrfiard.le_dbtime, &fFlagsBefore, fDirtyCSR, fFalse ) );

    lrfiard.dbid        = g_rgfmp[ pfucb->ifmp ].Dbid();
    lrfiard.le_pgno     = pcsr->Pgno();

    LGISetTrx( ppib, &lrfiard, pverproxy );

    Assert( !kdf.key.FNull() );
    lrfiard.SetCbKey( USHORT( kdf.key.Cb() ) );
    lrfiard.SetCbData( USHORT( kdf.data.Cb() ) );

    rgdata[0].SetPv( (BYTE *)&lrfiard );
    rgdata[0].SetCb( sizeof(LRFLAGINSERTANDREPLACEDATA) );

    rgdata[1].SetPv( kdf.key.prefix.Pv() );
    rgdata[1].SetCb( kdf.key.prefix.Cb() );

    rgdata[2].SetPv( kdf.key.suffix.Pv() );
    rgdata[2].SetCb( kdf.key.suffix.Cb() );

    rgdata[3].SetPv( kdf.data.Pv() );
    rgdata[3].SetCb( kdf.data.Cb() );

    err = plog->ErrLGLogRec( rgdata, 4, fDirtyCSR ? 0 : fLGMacroGoing, ppib->lgposStart.lGeneration, plgpos );

    if ( JET_errSuccess > err && fDirtyCSR )
    {
        Assert ( dbtimeInvalid !=  lrfiard.le_dbtimeBefore && dbtimeNil != lrfiard.le_dbtimeBefore);
        Assert ( pcsr->Dbtime() > lrfiard.le_dbtimeBefore );
        pcsr->RevertDbtime( lrfiard.le_dbtimeBefore, fFlagsBefore );
    }

    return err;
}


ERR ErrLGFlagInsert( const FUCB             * const pfucb,
                     CSR                    * const pcsr,
                     const KEYDATAFLAGS&    kdf,
                     const RCEID            rceid,
                     const DIRFLAG          dirflag,
                     LGPOS                  * const plgpos,
                     const VERPROXY         * const pverproxy ,
                     const BOOL             fDirtyCSR )
{
    ERR             err;
    DATA            rgdata[4];
    LRFLAGINSERT    lrflaginsert;

    INST *pinst = PinstFromIfmp( pfucb->ifmp );
    LOG *plog = pinst->m_plog;

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );
    Assert( fDirtyCSR || pcsr->FDirty() );

    if ( plog->FLogDisabled() )
    {
        if ( fDirtyCSR )
        {
            pcsr->Dirty();
        }
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( !( dirflag & fDIRNoLog ) );
    Assert( g_rgfmp[pfucb->ifmp].FLogOn() );

    Assert( NULL == pverproxy ||
            prceNil != pverproxy->prcePrimary &&
            ( pverproxy->prcePrimary->TrxCommitted() != trxMax ||
              pfucbNil != pverproxy->prcePrimary->Pfucb() &&
              ppibNil != pverproxy->prcePrimary->Pfucb()->ppib ) );

    PIB * const ppib = pverproxy != NULL &&
                    trxMax == pverproxy->prcePrimary->TrxCommitted() ?
                        pverproxy->prcePrimary->Pfucb()->ppib :
                        pfucb->ppib;

    Assert( ppib->Level() > 0 );

    CallR( ErrLGDeferBeginTransaction( ppib ) );

    lrflaginsert.lrtyp      = lrtypFlagInsert;

    Assert( pcsr->ILine() >= 0 );
    Assert( pcsr->ILine() < g_cbPage / 8 );
    lrflaginsert.SetILine( (USHORT)pcsr->ILine() );

    Assert( !lrflaginsert.FVersioned() );
    Assert( !lrflaginsert.FDeleted() );
    Assert( !lrflaginsert.FUnique() );
    Assert( !lrflaginsert.FSpace() );
    Assert( !lrflaginsert.FConcCI() );
    Assert( !lrflaginsert.FSystemTask() );

    if ( !( dirflag & fDIRNoVersion ) )
        lrflaginsert.SetFVersioned();
    if ( pfucb->u.pfcb->FUnique() )
        lrflaginsert.SetFUnique();
    if ( FFUCBSpace( pfucb ) )
        lrflaginsert.SetFSpace();
    if ( NULL != pverproxy )
        lrflaginsert.SetFConcCI();

    lrflaginsert.le_rceid       = rceid;
    lrflaginsert.le_pgnoFDP     = PgnoFDP( pfucb );
    lrflaginsert.le_objidFDP    = ObjidFDP( pfucb );

    lrflaginsert.le_procid  = ppib->procid;

    ULONG fFlagsBefore = 0;
    CallR( ErrLGSetDbtimeBeforeAndDirty( pinst, pcsr, &lrflaginsert.le_dbtimeBefore, &lrflaginsert.le_dbtime, &fFlagsBefore, fDirtyCSR, fFalse ) );

    lrflaginsert.dbid           = g_rgfmp[ pfucb->ifmp ].Dbid();
    lrflaginsert.le_pgno        = pcsr->Pgno();

    LGISetTrx( ppib, &lrflaginsert, pverproxy );

    Assert( !kdf.key.FNull() );
    lrflaginsert.SetCbKey( USHORT( kdf.key.Cb() ) );
    lrflaginsert.SetCbData( USHORT( kdf.data.Cb() ) );

    rgdata[0].SetPv( (BYTE *)&lrflaginsert );
    rgdata[0].SetCb( sizeof(LRFLAGINSERT) );

    rgdata[1].SetPv( kdf.key.prefix.Pv() );
    rgdata[1].SetCb( kdf.key.prefix.Cb() );

    rgdata[2].SetPv( kdf.key.suffix.Pv() );
    rgdata[2].SetCb( kdf.key.suffix.Cb() );

    rgdata[3].SetPv( kdf.data.Pv() );
    rgdata[3].SetCb( kdf.data.Cb() );

    err = plog->ErrLGLogRec( rgdata, 4, fDirtyCSR ? 0 : fLGMacroGoing, ppib->lgposStart.lGeneration, plgpos );

    if ( JET_errSuccess > err && fDirtyCSR )
    {
        Assert ( dbtimeInvalid !=  lrflaginsert.le_dbtimeBefore && dbtimeNil != lrflaginsert.le_dbtimeBefore);
        Assert ( pcsr->Dbtime() > lrflaginsert.le_dbtimeBefore );
        pcsr->RevertDbtime( lrflaginsert.le_dbtimeBefore, fFlagsBefore );
    }

    return err;
}

LOCAL INLINE ERR ErrLGISetExternalHeader(
    PIB* const  ppib,
    const IFMP  ifmp,
    const PGNO  pgnoFDP,
    const OBJID objidFDP,
    const BOOL  fUnique,
    const BOOL  fSpace,
    CSR*        pcsr,
    const DATA& data,
    LGPOS*      plgpos,
    const BOOL  fDirtyCSR,
    const DBTIME dbtimeBefore = dbtimeNil )
{
    ERR                 err;
    LRSETEXTERNALHEADER lrsetextheader;
    DATA                rgdata[2];

    INST *pinst = PinstFromIfmp( ifmp );
    LOG *plog = pinst->m_plog;

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );
    Assert( fDirtyCSR || pcsr->FDirty() );
    Assert( !fDirtyCSR || ( dbtimeBefore == dbtimeNil ) );

    if ( plog->FLogDisabled() )
    {
        if ( fDirtyCSR )
        {
            pcsr->Dirty();
        }
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( g_rgfmp[ifmp].FLogOn() );
    Assert( ppib->Level() > 0 );


    CallR( ErrLGDeferBeginTransaction( ppib ) );

    lrsetextheader.lrtyp = lrtypSetExternalHeader;

    lrsetextheader.le_procid    = ppib->procid;
    lrsetextheader.dbid         = g_rgfmp[ ifmp ].Dbid();
    lrsetextheader.le_pgno      = pcsr->Pgno();
    lrsetextheader.le_pgnoFDP   = pgnoFDP;
    lrsetextheader.le_objidFDP  = objidFDP;

    Assert( !lrsetextheader.FVersioned() );
    Assert( !lrsetextheader.FDeleted() );
    Assert( !lrsetextheader.FUnique() );
    Assert( !lrsetextheader.FSpace() );
    Assert( !lrsetextheader.FConcCI() );
    Assert( !lrsetextheader.FSystemTask() );

    if ( fUnique )
        lrsetextheader.SetFUnique();
    if ( fSpace )
        lrsetextheader.SetFSpace();

    ULONG fFlagsBefore = 0;
    CallR( ErrLGSetDbtimeBeforeAndDirty( pinst, pcsr, &lrsetextheader.le_dbtimeBefore, &lrsetextheader.le_dbtime, &fFlagsBefore, fDirtyCSR, fFalse ) );
    Assert( fDirtyCSR || ( lrsetextheader.le_dbtimeBefore == dbtimeNil ) );
    if ( !fDirtyCSR && ( dbtimeBefore != dbtimeNil )  )
    {
        lrsetextheader.le_dbtimeBefore = dbtimeBefore;
    }

    LGISetTrx( ppib, &lrsetextheader );

    lrsetextheader.SetCbData( USHORT( data.Cb() ) );

    rgdata[0].SetPv( (BYTE *)&lrsetextheader );
    rgdata[0].SetCb( sizeof(LRSETEXTERNALHEADER) );

    rgdata[1].SetPv( data.Pv() );
    rgdata[1].SetCb( data.Cb() );

    err = plog->ErrLGLogRec( rgdata, 2, fDirtyCSR ? 0 : fLGMacroGoing, ppib->lgposStart.lGeneration, plgpos );

    if ( JET_errSuccess > err && fDirtyCSR )
    {
        Assert ( dbtimeInvalid !=  lrsetextheader.le_dbtimeBefore && dbtimeNil != lrsetextheader.le_dbtimeBefore);
        Assert ( pcsr->Dbtime() > lrsetextheader.le_dbtimeBefore );
        pcsr->RevertDbtime( lrsetextheader.le_dbtimeBefore, fFlagsBefore );
    }

    return err;
}

ERR ErrLGSetExternalHeader(
    const FUCB* pfucb,
    CSR*        pcsr,
    const DATA& data,
    LGPOS*      plgpos,
    const BOOL  fDirtyCSR,
    const DBTIME dbtimeBefore )
{
    return ErrLGISetExternalHeader(
            pfucb->ppib,
            pfucb->ifmp,
            PgnoFDP( pfucb ),
            ObjidFDP( pfucb ),
            pfucb->u.pfcb->FUnique(),
            FFUCBSpace( pfucb ),
            pcsr,
            data,
            plgpos,
            fDirtyCSR,
            dbtimeBefore );
}

static BYTE g_rgbPadding[1618];

LOCAL size_t CbPaddingForLrScrub( INST * const pinst )
{
    const size_t cbPaddingMin   = 0;
    const size_t cbPaddingMax   = sizeof(g_rgbPadding);

    const INT pctPinned = LBFICachePinnedPercentage();
    
    const size_t cbPage     = g_cbPage;
    const size_t cbLogFile  = (size_t)UlParam( pinst, JET_paramLogFileSize ) * 1024;

    size_t cbPagePerLogfile = ( 100000*1024 ) - ( pctPinned * ( 100000*1024 ) );
    cbPagePerLogfile = min( cbPagePerLogfile, 80000*1024 );
    cbPagePerLogfile = max( cbPagePerLogfile, 20000*1024 );

    const INT cpgPerLogfile = cbPagePerLogfile / cbPage;
    const size_t cbDesiredLrScrubSize = cbLogFile / cpgPerLogfile;

    size_t cbPadding = cbDesiredLrScrubSize - sizeof(LRSCRUB);
    cbPadding = min( cbPadding, cbPaddingMax );
    cbPadding = max( cbPadding, cbPaddingMin );
    
    return cbPadding;
}

ERR ErrLGScrub(
    __in                        PIB * const             ppib,
    __in                        const IFMP              ifmp,
    __in                        CSR * const             pcsr,
    __in                        const BOOL              fUnusedPage,
    __in_ecount_opt(cscrubOper) const SCRUBOPER * const rgscrubOper,
    __in                        const INT               cscrubOper,
    __out                       LGPOS * const           plgpos )
{
    ERR         err;
    LRSCRUB     lrscrub;
    DATA        rgdata[3];

    INST * const pinst = PinstFromIfmp( ifmp );
    LOG * const plog = pinst->m_plog;

    const size_t cbPadding = CbPaddingForLrScrub( pinst );

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );

    if ( plog->FLogDisabled() )
    {
        pcsr->DirtyForScrub();
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( g_rgfmp[ifmp].FLogOn() );
    Assert( ppib->Level() > 0 );

    CallR( ErrLGDeferBeginTransaction( ppib ) );

    lrscrub.lrtyp       = lrtypScrub;
    lrscrub.le_procid   = ppib->procid;
    lrscrub.dbid        = g_rgfmp[ ifmp ].Dbid();
    lrscrub.le_pgno     = pcsr->Pgno();
    lrscrub.le_pgnoFDP  = 0;
    lrscrub.le_objidFDP = pcsr->Cpage().ObjidFDP();

    Assert( !lrscrub.FVersioned() );
    Assert( !lrscrub.FDeleted() );
    Assert( !lrscrub.FUnique() );
    Assert( !lrscrub.FSpace() );
    Assert( !lrscrub.FConcCI() );
    Assert( !lrscrub.FSystemTask() );

    if( !pcsr->Cpage().FNonUniqueKeys() )
    {
        lrscrub.SetFUnique();
    }
    if( pcsr->Cpage().FSpaceTree() )
    {
        lrscrub.SetFSpace();
    }

    ULONG fFlagsBefore = 0;
    CallR( ErrLGSetDbtimeBeforeAndDirty( pinst, pcsr, &lrscrub.le_dbtimeBefore, &lrscrub.le_dbtime, &fFlagsBefore, fMustDirtyCSR, fTrue ) );
    Assert( pcsr->Cpage().FScrubbed() );

    LGISetTrx( ppib, &lrscrub );
    
    INT idata = 0;
    rgdata[idata].SetPv( (VOID *)&lrscrub );
    rgdata[idata].SetCb( sizeof(LRSCRUB) );
    idata++;

    if( fUnusedPage )
    {
        Assert( 0 == cscrubOper );
        Assert( NULL == rgscrubOper );
        lrscrub.SetFUnusedPage();
        lrscrub.SetCbData( (USHORT)cbPadding );
        lrscrub.SetCscrubOper( 0 );
        rgdata[idata].SetPv( NULL );
        rgdata[idata].SetCb( 0 );
        Assert( lrscrub.FUnusedPage() );
    }
    else
    {
        const USHORT cbScrubData = (USHORT)(cscrubOper * sizeof(SCRUBOPER));
        const USHORT cbData =  cbScrubData + (USHORT)cbPadding;
        lrscrub.SetCbData( cbData );
        lrscrub.SetCscrubOper( cscrubOper );
        rgdata[idata].SetPv( (VOID *)rgscrubOper );
        rgdata[idata].SetCb( cbScrubData );
        Assert( !lrscrub.FUnusedPage() );
    }
    idata++;

    rgdata[idata].SetPv( g_rgbPadding );
    rgdata[idata].SetCb( cbPadding );
    idata++;

    Assert( _countof( rgdata ) == idata );
    err = plog->ErrLGLogRec( rgdata, idata, 0, ppib->lgposStart.lGeneration, plgpos );

    if ( err < JET_errSuccess )
    {
        Assert ( dbtimeInvalid !=  lrscrub.le_dbtimeBefore && dbtimeNil != lrscrub.le_dbtimeBefore);
        Assert ( pcsr->Dbtime() > lrscrub.le_dbtimeBefore );
        pcsr->RevertDbtime( lrscrub.le_dbtimeBefore, fFlagsBefore );
    }

    return err;
}

ERR ErrLGNewPage(
    __in    PIB * const     ppib,
    __in    const IFMP      ifmp,
    __in    const PGNO      pgno,
    __in    const OBJID     objid,
    __in    const DBTIME    dbtime,
    __out   LGPOS * const   plgpos )
{

    ERR err = JET_errSuccess;
    FMP* const pfmp = g_rgfmp + ifmp;
    LOG* const plog = PinstFromIfmp( ifmp )->m_plog;

    if ( plog->FLogDisabled() || !pfmp->FLogOn() )
    {
        Expected( fFalse );
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( !pfmp->FIsTempDB() );
    Assert( ppib->Level() > 0 );

    CallR( ErrLGDeferBeginTransaction( ppib ) );

    LRNEWPAGE lrnewpage;
    lrnewpage.SetDbtimeAfter( dbtime );
    lrnewpage.SetProcid( ppib->procid );
    lrnewpage.SetDbid( pfmp->Dbid() );
    lrnewpage.SetPgno( pgno );
    lrnewpage.SetObjid( objid );

    DATA data;
    data.SetPv( &lrnewpage );
    data.SetCb( sizeof( lrnewpage ) );

    Call( plog->ErrLGLogRec( &data, 1, 0, ppib->lgposStart.lGeneration, plgpos ) );

HandleError:
    return err;
}

ERR ErrLGScanCheck(
    __in    const IFMP      ifmp,
    __in    const PGNO      pgno,
    __in    const BYTE      bSource,
    __in    const DBTIME    dbtimePage,
    __in    const DBTIME    dbtimeCurrent,
    __in    const ULONG     ulChecksum,
    __in    const BOOL      fScanCheck2Supported,
    __in    LGPOS* const    plgposLogRec )
{
    INST * const pinst = PinstFromIfmp( ifmp );
    LOG * const plog = pinst->m_plog;

    if ( plog->FLogDisabled() )
    {
        return JET_errSuccess;
    }
    Assert( g_rgfmp[ifmp].FLogOn() );


#ifdef DEBUG
    if ( pgno != pgnoScanLastSentinel )
    {
        Assert( pgno != pgnoNull );
        Assert( ( dbtimePage >= 0 ) || ( dbtimePage == dbtimeShrunk ) );
        Assert( dbtimePage != dbtimeInvalid );
        Assert( dbtimeCurrent > 0 );
        Assert( dbtimeCurrent != dbtimeShrunk );
        Assert( dbtimeCurrent != dbtimeInvalid );
        Expected( ( bSource == scsDbScan ) || ( bSource == scsDbShrink ) );
    }
    else
    {
        Expected( bSource == scsDbScan );

        Expected( dbtimePage == 0 );
        Expected( dbtimeCurrent == 0 );
        Expected( ulChecksum == 0 );
    }
#endif

    DATA data;
    LRSCANCHECK2 lrscancheck2;
    LRSCANCHECK lrscancheck;
    if ( fScanCheck2Supported )
    {
        lrscancheck2.InitScanCheck(
            (USHORT)g_rgfmp[ ifmp ].Dbid(),
            pgno,
            bSource,
            dbtimePage,
            dbtimeCurrent,
            ulChecksum );

        Assert( lrscancheck2.DbtimePage() <= lrscancheck2.DbtimeCurrent() );

        data.SetPv( (VOID *)&lrscancheck2 );
        data.SetCb( sizeof(LRSCANCHECK2) );
    }
    else
    {
        Assert( ( ulChecksum & 0xFFFF0000 ) == 0 );
        lrscancheck.InitLegacyScanCheck(
            (USHORT)g_rgfmp[ ifmp ].Dbid(),
            pgno,
            dbtimePage,
            dbtimeCurrent,
            (USHORT)ulChecksum );

        Assert( lrscancheck.DbtimeBefore() <= lrscancheck.DbtimeAfter() );

        data.SetPv( (VOID *)&lrscancheck );
        data.SetCb( sizeof(LRSCANCHECK) );
    }

    return plog->ErrLGLogRec( &data, 1, 0, 0, plgposLogRec );
}

LOCAL INLINE ERR ErrLGIPageMove(
    __in    PIB * const     ppib,
    __in    const IFMP      ifmp,
    __in    const PGNO      pgnoFDP,
    __in    const OBJID     objidFDP,
    __in    CSR* const      pcsrSource,
    __in    const PGNO      pgnoDest,
    __in    const PGNO      pgnoParent,
    __in    const INT       ilineParent,
    __in    const PGNO      pgnoLeft,
    __in    const PGNO      pgnoRight,
    __in    const DBTIME    dbtimeSourceBefore,
    __in    const DBTIME    dbtimeParentBefore,
    __in    const DBTIME    dbtimeLeftBefore,
    __in    const DBTIME    dbtimeRightBefore,
    __in    const DBTIME    dbtimeAfter,
    __in    const ULONG     fMoveFlags,
    __out   LGPOS * const   plgpos )
{
    Assert( pcsrSource );
    Assert( pcsrSource->Latch() == latchWrite );
    Assert( pcsrSource->Pgno() != pgnoNull );
    Assert( pgnoDest != pgnoNull );
    Assert( ( pgnoParent != pgnoNull ) || ( ( pgnoLeft == pgnoNull ) && ( pgnoRight == pgnoNull ) ) );
    Assert( plgpos );

    const INST * const  pinst   = PinstFromIfmp( ifmp );
    LOG * const         plog    = pinst->m_plog;

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );
    Assert( !g_rgfmp[ifmp].FLogOn() || !plog->FLogDisabled() );
    if ( !g_rgfmp[ifmp].FLogOn() )
    {
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    ERR err;
    CallR( ErrLGDeferBeginTransaction( ppib ) );

    LRPAGEMOVE  lrpagemove;

    lrpagemove.lrtyp        = lrtypPageMove;
    lrpagemove.le_procid    = ppib->procid;
    lrpagemove.dbid         = g_rgfmp[ifmp].Dbid();

    lrpagemove.SetPgnoFDP( pgnoFDP );
    lrpagemove.SetObjidFDP( objidFDP );

    lrpagemove.SetPgnoSource( pcsrSource->Pgno() );
    lrpagemove.SetDbtimeSourceBefore( dbtimeSourceBefore );

    lrpagemove.SetPgnoDest( pgnoDest );
    
    lrpagemove.SetPgnoParent( pgnoParent );
    if ( pgnoNull != pgnoParent )
    {
        lrpagemove.SetDbtimeParentBefore( dbtimeParentBefore );
        lrpagemove.SetIlineParent( ilineParent );
    }

    lrpagemove.SetPgnoLeft( pgnoLeft );
    if ( pgnoNull != pgnoLeft )
    {
        lrpagemove.SetDbtimeLeftBefore( dbtimeLeftBefore );
    }

    lrpagemove.SetPgnoRight( pgnoRight );
    if ( pgnoNull != pgnoRight )
    {
        lrpagemove.SetDbtimeRightBefore( dbtimeRightBefore );
    }

    lrpagemove.SetDbtimeAfter( dbtimeAfter );

    const VOID * pvHeader;
    const VOID * pvTrailer;
    size_t cbHeader;
    size_t cbTrailer;

    pcsrSource->Cpage().ReorganizePage( &pvHeader, &cbHeader, &pvTrailer, &cbTrailer );

    lrpagemove.SetCbPageHeader( cbHeader );
    lrpagemove.SetCbPageTrailer( cbTrailer );

    if ( ppib->FSessionSystemTask() )
    {
        lrpagemove.SetFSystemTask();
    }

    if ( fMoveFlags & LRPAGEMOVE::fRootFDP )
    {
        lrpagemove.SetPageMoveRootFDP();
    }
    if ( fMoveFlags & LRPAGEMOVE::fRootOE )
    {
        lrpagemove.SetPageMoveRootOE();
    }
    if ( fMoveFlags & LRPAGEMOVE::fRootAE )
    {
        lrpagemove.SetPageMoveRootAE();
    }

    ASSERT_VALID( &lrpagemove );

    DATA    rgdata[3];
    size_t  idata = 0;

    rgdata[idata].SetPv( &lrpagemove );
    rgdata[idata].SetCb( sizeof( lrpagemove ) );
    idata++;
    rgdata[idata].SetPv( const_cast<VOID*>(pvHeader) );
    rgdata[idata].SetCb( cbHeader );
    idata++;
    rgdata[idata].SetPv( const_cast<VOID*>(pvTrailer) );
    rgdata[idata].SetCb( cbTrailer );
    idata++;
    Assert( _countof(rgdata) == idata );

    Call( plog->ErrLGLogRec( rgdata, idata, lrpagemove.FPageMoveRoot() ? fLGMacroGoing : 0, ppib->lgposStart.lGeneration, plgpos ) );

HandleError:
    return err;
}

ERR ErrLGPageMove(
    __in    const FUCB * const  pfucb,
    __in    MERGEPATH * const   pmergePath,
    __out   LGPOS * const       plgpos )
{
    Assert( pfucb );
    Assert( pmergePath );
    Assert( pmergePath->pmerge );
    Assert( pmergePath->pmergePathParent );
    Assert( mergetypePageMove == pmergePath->pmerge->mergetype );

    return ErrLGIPageMove(
            pfucb->ppib,
            pfucb->ifmp,
            PgnoFDP( pfucb ),
            ObjidFDP( pfucb ),
            &pmergePath->csr,
            pmergePath->pmerge->csrNew.Pgno(),
            pmergePath->pmergePathParent->csr.Pgno(),
            pmergePath->pmergePathParent->csr.ILine(),
            pmergePath->pmerge->csrLeft.Pgno(),
            pmergePath->pmerge->csrRight.Pgno(),
            pmergePath->dbtimeBefore,
            pmergePath->pmergePathParent->dbtimeBefore,
            pmergePath->pmerge->dbtimeLeftBefore,
            pmergePath->pmerge->dbtimeRightBefore,
            pmergePath->csr.Dbtime(),
            LRPAGEMOVE::fRootNone,
            plgpos );
}


LOCAL ERR ErrLGIPageMoveNoTableInMacro(
    __in    PIB * const     ppib,
    __in    const IFMP      ifmp,
    __in    const PGNO      pgnoFDP,
    __in    const OBJID     objidFDP,
    __in    CSR* const      pcsrSource,
    __in    const PGNO      pgnoDest,
    __in    const PGNO      pgnoParent,
    __in    const INT       ilineParent,
    __in    const PGNO      pgnoLeft,
    __in    const PGNO      pgnoRight,
    __in    const DBTIME    dbtimeSourceBefore,
    __in    const DBTIME    dbtimeParentBefore,
    __in    const DBTIME    dbtimeLeftBefore,
    __in    const DBTIME    dbtimeRightBefore,
    __in    const DBTIME    dbtimeAfter,
    __in    const ULONG     fMoveFlags,
    __out   LGPOS * const   plgpos )
{
    return ErrLGIPageMove(
             ppib,
             ifmp,
             pgnoFDP,
             objidFDP,
             pcsrSource,
             pgnoDest,
             pgnoParent,
             ilineParent,
             pgnoLeft,
             pgnoRight,
             dbtimeSourceBefore,
             dbtimeParentBefore,
             dbtimeLeftBefore,
             dbtimeRightBefore,
             dbtimeAfter,
             fMoveFlags,
             plgpos );
}


LOCAL ERR ErrLGIPageMoveRootsInMacro(
    __in    PIB * const         ppib,
    __in    const IFMP          ifmp,
    __in    ROOTMOVE * const    prm,
    __out   LGPOS * const       plgpos )
{
    ERR err = JET_errSuccess;

    for ( CPG ipg = 1; ipg <= 3; ipg++ )
    {
        CSR* pcsrSource = pcsrNil;
        DATA* pdataBeforeSource = NULL;
        PGNO pgnoNew = pgnoNull;
        DBTIME dbtimeBefore = dbtimeNil;
        ULONG fMoveFlags = LRPAGEMOVE::fRootNone;
        if ( ipg == 1 )
        {
            pcsrSource = &prm->csrFDP;
            pdataBeforeSource = &prm->dataBeforeFDP;
            pgnoNew = prm->pgnoNewFDP;
            dbtimeBefore = prm->dbtimeBeforeFDP;
            fMoveFlags = LRPAGEMOVE::fRootFDP;
        }
        else if ( ipg == 2 )
        {
            pcsrSource = &prm->csrOE;
            pdataBeforeSource = &prm->dataBeforeOE;
            pgnoNew = prm->pgnoNewOE;
            dbtimeBefore = prm->dbtimeBeforeOE;
            fMoveFlags = LRPAGEMOVE::fRootOE;
        }
        else if ( ipg == 3 )
        {
            pcsrSource = &prm->csrAE;
            pdataBeforeSource = &prm->dataBeforeAE;
            pgnoNew = prm->pgnoNewAE;
            dbtimeBefore = prm->dbtimeBeforeAE;
            fMoveFlags = LRPAGEMOVE::fRootAE;
        }
        else
        {
            Assert( fFalse );
        }

        CallR( ErrLGIPageMoveNoTableInMacro(
                ppib,
                ifmp,
                prm->pgnoFDP,
                prm->objid,
                pcsrSource,
                pgnoNew,
                pgnoNull,
                -1,
                pgnoNull,
                pgnoNull,
                dbtimeBefore,
                dbtimeNil,
                dbtimeNil,
                dbtimeNil,
                prm->dbtimeAfter,
                fMoveFlags,
                plgpos ) );

        Assert( (ULONG)pdataBeforeSource->Cb() == pcsrSource->Cpage().CbBuffer() );
        Assert( pcsrSource->Cpage().CbPage() == (ULONG)g_cbPage );
        Assert( (ULONG)pdataBeforeSource->Cb() == (ULONG)g_cbPage );
        UtilMemCpy(
            pdataBeforeSource->Pv(),
            pcsrSource->Cpage().PvBuffer(),
            pdataBeforeSource->Cb() );
    }

    return err;
}


LOCAL ERR ErrLGISetExternalHeaderNoTableInMacro(
    PIB* const      ppib,
    const IFMP      ifmp,
    const PGNO      pgnoFDP,
    const OBJID     objidFDP,
    CSR*            pcsr,
    const DATA&     data,
    LGPOS*          plgpos,
    const BOOL      fDirtyCSR,
    const DBTIME    dbtimeBefore )
{
    return ErrLGISetExternalHeader(
            ppib,
            ifmp,
            pgnoFDP,
            objidFDP,
            fFalse,
            fFalse,
            pcsr,
            data,
            plgpos,
            fDirtyCSR,
            dbtimeBefore );
}

LOCAL INLINE ERR ErrLGIReplace(
    PIB* const          ppib,
    const IFMP          ifmp,
    const PGNO          pgnoFDP,
    const OBJID         objidFDP,
    const BOOL          fUnique,
    const BOOL          fSpace,
    CSR* const          pcsr,
    const DATA&         dataOld,
    const DATA&         dataNew,
    const DATA* const   pdataDiff,
    const RCEID         rceid,
    const DIRFLAG       dirflag,
    LGPOS* const        plgpos,
    const BOOL          fDirtyCSR,
    const DBTIME        dbtimeBefore )
{
    ERR         err;
    DATA        rgdata[2];
    LRREPLACE   lrreplace;

    INST *pinst = PinstFromIfmp( ifmp );
    LOG *plog = pinst->m_plog;

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );
    Assert( fDirtyCSR || pcsr->FDirty() );
    Assert( !fDirtyCSR || ( dbtimeBefore == dbtimeNil ) );

    if ( plog->FLogDisabled() )
    {
        if ( fDirtyCSR )
        {
            pcsr->Dirty();
        }
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( !( dirflag & fDIRNoLog ) );
    Assert( g_rgfmp[ifmp].FLogOn() );

    Assert( ppib->Level() > 0 );

    CallR( ErrLGDeferBeginTransaction( ppib ) );

    Assert( pcsr->ILine() >= 0 );
    Assert( pcsr->ILine() < g_cbPage / 8 );
    lrreplace.SetILine( (USHORT)pcsr->ILine() );

    Assert( !lrreplace.FVersioned() );
    Assert( !lrreplace.FDeleted() );
    Assert( !lrreplace.FUnique() );
    Assert( !lrreplace.FSpace() );
    Assert( !lrreplace.FConcCI() );
    Assert( !lrreplace.FSystemTask() );

    if ( !( dirflag & fDIRNoVersion ) )
    {
        Assert( rceid != rceidNull );
        lrreplace.SetFVersioned();
    }

    if ( fUnique )
    {
        lrreplace.SetFUnique();
    }

    if ( fSpace )
    {
        lrreplace.SetFSpace();
    }

    lrreplace.le_rceid      = rceid;
    lrreplace.le_pgnoFDP    = pgnoFDP;
    lrreplace.le_objidFDP   = objidFDP;

    lrreplace.le_procid = ppib->procid;

    ULONG fFlagsBefore = 0;
    CallR( ErrLGSetDbtimeBeforeAndDirty( pinst, pcsr, &lrreplace.le_dbtimeBefore, &lrreplace.le_dbtime, &fFlagsBefore, fDirtyCSR, fFalse ) );
    Assert( fDirtyCSR || ( lrreplace.le_dbtimeBefore == dbtimeNil ) );
    if ( !fDirtyCSR && ( dbtimeBefore != dbtimeNil )  )
    {
        lrreplace.le_dbtimeBefore = dbtimeBefore;
    }

    lrreplace.dbid          = g_rgfmp[ ifmp ].Dbid();
    lrreplace.le_pgno       = pcsr->Pgno();

    LGISetTrx( ppib, &lrreplace );

    lrreplace.SetCbOldData( USHORT( dataOld.Cb() ) );
    lrreplace.SetCbNewData( USHORT( dataNew.Cb() ) );

    rgdata[0].SetPv( (BYTE *)&lrreplace );
    rgdata[0].SetCb( sizeof(LRREPLACE) );

    if ( NULL != pdataDiff )
    {
        lrreplace.lrtyp     = lrtypReplaceD;
        lrreplace.SetCb( (USHORT)pdataDiff->Cb() );
        rgdata[1].SetCb( pdataDiff->Cb() );
        rgdata[1].SetPv( pdataDiff->Pv() );

        if ( !FIsSmallPage() )
        {
            lrreplace.SetFDiff2();
        }
    }
    else
    {
        lrreplace.lrtyp     = lrtypReplace;
        lrreplace.SetCb( (USHORT)dataNew.Cb() );
        rgdata[1].SetCb( dataNew.Cb() );
        rgdata[1].SetPv( dataNew.Pv() );
    }

    err = plog->ErrLGLogRec( rgdata, 2, fDirtyCSR ? 0 : fLGMacroGoing, ppib->lgposStart.lGeneration, plgpos );

    if ( JET_errSuccess > err && fDirtyCSR )
    {
        Assert ( dbtimeInvalid !=  lrreplace.le_dbtimeBefore && dbtimeNil != lrreplace.le_dbtimeBefore);
        Assert ( pcsr->Dbtime() > lrreplace.le_dbtimeBefore );
        pcsr->RevertDbtime( lrreplace.le_dbtimeBefore, fFlagsBefore );
    }

    Assert( lrreplace.Cb() <= lrreplace.CbNewData() );
    Assert( lrreplace.CbNewData() == lrreplace.Cb()
        || lrreplace.lrtyp == lrtypReplaceD );
    return err;
}


LOCAL ERR ErrLGIReplaceNoTableInMacro(
    PIB* const          ppib,
    const IFMP          ifmp,
    const PGNO          pgnoFDP,
    const OBJID         objidFDP,
    const BOOL          fUnique,
    const BOOL          fSpace,
    CSR* const          pcsr,
    const DATA&         dataOld,
    const DATA&         dataNew,
    const DATA* const   pdataDiff,
    const RCEID         rceid,
    const DIRFLAG       dirflag,
    LGPOS* const        plgpos,
    const BOOL          fDirtyCSR,
    const DBTIME        dbtimeBefore )
{
    return ErrLGIReplace(
            ppib,
            ifmp,
            pgnoFDP,
            objidFDP,
            fUnique,
            fSpace,
            pcsr,
            dataOld,
            dataNew,
            pdataDiff,
            rceid,
            dirflag,
            plgpos,
            fDirtyCSR,
            dbtimeBefore );
}

ERR ErrLGRootPageMove(
    __in    PIB * const         ppib,
    __in    const IFMP          ifmp,
    __in    ROOTMOVE * const    prm,
    __out   LGPOS * const       plgpos )
{
    ERR err = JET_errSuccess;
    FMP* const pfmp = g_rgfmp + ifmp;
    LOG* const plog = PinstFromIfmp( ifmp )->m_plog;
    PGNO *rgpgnoRef = NULL;
    DBTIME dbtimeMacro = dbtimeNil;
    BOOL fAbortMacro = fFalse;
    LGPOS lgposThrowAway = lgposMin;

    *plgpos = lgposMin;

    if ( plog->FLogDisabled() || !pfmp->FLogOn() )
    {
        Expected( fFalse );
        return JET_errSuccess;
    }

    Assert( !pfmp->FIsTempDB() );
    Assert( ppib->Level() > 0 );

    CallR( ErrLGDeferBeginTransaction( ppib ) );


    CPG cpgRef = 0;

    cpgRef += 6;

    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        cpgRef++;
    }

    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        cpgRef++;

        if ( ( prm->pgnoCatClustIdx[iCat] != pgnoNull ) &&
             ( prm->pgnoCatClustIdx[iCat] != prm->pgnoCatObj[iCat] ) )
        {
            cpgRef++;
        }
    }

    AssumePREFAST( cpgRef >= ( 6 + 2 ) );

    AllocR( rgpgnoRef = new PGNO[ cpgRef ] );
    CPG ipgRef = 0;

    dbtimeMacro = prm->dbtimeAfter;
    Call( ErrLGIMacroBegin( ppib, dbtimeMacro ) );
    fAbortMacro = fTrue;


    {
    LRROOTPAGEMOVE lrrpm;
    lrrpm.SetDbtimeAfter( prm->dbtimeAfter );
    lrrpm.SetProcid( ppib->procid );
    lrrpm.SetDbid( pfmp->Dbid() );
    lrrpm.SetObjid( prm->objid );
    ASSERT_VALID_OBJ( lrrpm );

    DATA data;
    data.SetPv( &lrrpm );
    data.SetCb( sizeof( lrrpm ) );

    Call( plog->ErrLGLogRec( &data, 1, fLGMacroGoing, ppib->lgposStart.lGeneration, &lgposThrowAway ) );
    }

    Call( ErrLGIPageMoveRootsInMacro( ppib, ifmp, prm, &lgposThrowAway ) );
    rgpgnoRef[ipgRef++] = prm->pgnoFDP;
    rgpgnoRef[ipgRef++] = prm->pgnoOE;
    rgpgnoRef[ipgRef++] = prm->pgnoAE;
    rgpgnoRef[ipgRef++] = prm->pgnoNewFDP;
    rgpgnoRef[ipgRef++] = prm->pgnoNewOE;
    rgpgnoRef[ipgRef++] = prm->pgnoNewAE;

    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        Call( ErrLGISetExternalHeaderNoTableInMacro(
                ppib,
                ifmp,
                prmc->pgnoChildFDP,
                prmc->objidChild,
                &prmc->csrChildFDP,
                prmc->dataSphNew,
                &lgposThrowAway,
                fDontDirtyCSR,
                prmc->dbtimeBeforeChildFDP ) );
        rgpgnoRef[ipgRef++] = prmc->pgnoChildFDP;
    }

    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        const PGNO pgnoFDP = ( iCat == 0 ) ? pgnoFDPMSO : pgnoFDPMSOShadow;
        const OBJID objidFDP = ( iCat == 0 ) ? objidFDPMSO : objidFDPMSOShadow;

        prm->csrCatObj[iCat].SetILine( prm->ilineCatObj[iCat] );
        Call( ErrLGIReplaceNoTableInMacro(
                ppib,
                ifmp,
                pgnoFDP,
                objidFDP,
                fTrue,
                fFalse,
                &prm->csrCatObj[iCat],
                prm->dataBeforeCatObj[iCat],
                prm->dataNewCatObj[iCat],
                NULL,
                rceidNull,
                fDIRNoVersion,
                &lgposThrowAway,
                fDontDirtyCSR,
                prm->dbtimeBeforeCatObj[iCat] ) );
        prm->csrCatObj[iCat].SetILine( 0 );
        rgpgnoRef[ipgRef++] = prm->pgnoCatObj[iCat];

        if ( prm->ilineCatClustIdx[iCat] != -1 )
        {
            Assert( prm->pgnoCatClustIdx[iCat] != pgnoNull );
            CSR* pcsr = pcsrNil;
            DATA* pdata = NULL;
            DBTIME dbtimeBeforeReplace = dbtimeNil;

            if ( prm->pgnoCatClustIdx[iCat] != prm->pgnoCatObj[iCat] )
            {
                pcsr = &prm->csrCatClustIdx[iCat];
                pdata = &prm->dataBeforeCatClustIdx[iCat];
                dbtimeBeforeReplace = prm->dbtimeBeforeCatClustIdx[iCat];
                rgpgnoRef[ipgRef++] = prm->pgnoCatClustIdx[iCat];
            }
            else
            {
                pcsr = &prm->csrCatObj[iCat];
                pdata = &prm->dataBeforeCatObj[iCat];
                dbtimeBeforeReplace = prm->dbtimeBeforeCatObj[iCat];
            }

            pcsr->SetILine( prm->ilineCatClustIdx[iCat] );
            Call( ErrLGIReplaceNoTableInMacro(
                    ppib,
                    ifmp,
                    pgnoFDP,
                    objidFDP,
                    fTrue,
                    fFalse,
                    pcsr,
                    *pdata,
                    prm->dataNewCatClustIdx[iCat],
                    NULL,
                    rceidNull,
                    fDIRNoVersion,
                    &lgposThrowAway,
                    fDontDirtyCSR,
                    dbtimeBeforeReplace ) );
            pcsr->SetILine( 0 );
        }
    }

    Call( ErrFaultInjection( 41962 ) );

    Assert( ipgRef == cpgRef );
    Assert( dbtimeMacro != dbtimeNil );
    Call( ErrLGIMacroEnd( ppib, dbtimeMacro, lrtypMacroCommit, ifmp, rgpgnoRef, ipgRef, plgpos ) );
    fAbortMacro = fFalse;

HandleError:
    delete[] rgpgnoRef;

    if ( fAbortMacro )
    {
        Assert( err < JET_errSuccess );
        Assert( dbtimeMacro != dbtimeNil );
        (void)ErrLGMacroAbort( ppib, dbtimeMacro, &lgposThrowAway );
    }

    if ( err < JET_errSuccess )
    {
        Assert( CmpLgpos( *plgpos, lgposMin ) == 0 );
    }
    else
    {
        Assert( CmpLgpos( lgposThrowAway, lgposMin ) > 0 );
        Assert( CmpLgpos( *plgpos, lgposThrowAway ) > 0 );
    }

    return err;
}

ERR ErrLGReplace(
    const FUCB* const   pfucb,
    CSR* const          pcsr,
    const DATA&         dataOld,
    const DATA&         dataNew,
    const DATA* const   pdataDiff,
    const RCEID         rceid,
    const DIRFLAG       dirflag,
    LGPOS* const        plgpos,
    const BOOL          fDirtyCSR,
    const DBTIME        dbtimeBefore )
{
    return ErrLGIReplace(
            pfucb->ppib,
            pfucb->ifmp,
            PgnoFDP( pfucb ),
            ObjidFDP( pfucb ),
            pfucb->u.pfcb->FUnique(),
            FFUCBSpace( pfucb ),
            pcsr,
            dataOld,
            dataNew,
            pdataDiff,
            rceid,
            dirflag,
            plgpos,
            fDirtyCSR,
            dbtimeBefore );
}


ERR ErrLGIUndoInfo( const RCE *prce, LGPOS *plgpos, const BOOL fRetry )
{
    ERR             err;
    FUCB            * pfucb         = prce->Pfucb();
    const PGNO      pgnoUndoInfo    = prce->PgnoUndoInfo();
    INST            * pinst         = PinstFromIfmp( pfucb->ifmp );
    LOG             * plog          = pinst->m_plog;
    LRUNDOINFO      lrundoinfo;
    DATA            rgdata[4];

    *plgpos = lgposMin;

    Assert( !g_rgfmp[pfucb->ifmp].FLogOn() || !plog->FLogDisabled() );
    if ( !g_rgfmp[pfucb->ifmp].FLogOn() )
    {
        return JET_errSuccess;
    }

    if ( pgnoNull == pgnoUndoInfo )
    {
        return JET_errSuccess;
    }

    if ( plog->FLogDisabledDueToRecoveryFailure() )
    {
        return ErrERRCheck( JET_errLogDisabledDueToRecoveryFailure );
    }

    if ( plog->FRecovering() &&
         ( fRecoveringRedo == plog->FRecoveringMode() ||
           ( fRecoveringUndo == plog->FRecoveringMode() &&
             !plog->FRecoveryUndoLogged() ) ) )
    {
        return ErrERRCheck( JET_errCannotLogDuringRecoveryRedo );
    }

    Assert( pfucb->ppib->Level() > 0 );

    lrundoinfo.lrtyp        = lrtypUndoInfo;
    lrundoinfo.le_procid    = pfucb->ppib->procid;
    lrundoinfo.dbid         = g_rgfmp[ pfucb->ifmp ].Dbid();

    Assert( !lrundoinfo.FVersioned() );
    Assert( !lrundoinfo.FDeleted() );
    Assert( !lrundoinfo.FUnique() );
    Assert( !lrundoinfo.FSpace() );
    Assert( !lrundoinfo.FConcCI() );
    Assert( !lrundoinfo.FSystemTask() );

    if ( pfucb->u.pfcb->FUnique() )
        lrundoinfo.SetFUnique();
    if ( pfucb->ppib->FSessionSystemTask() )
        lrundoinfo.SetFSystemTask();

    Assert( !FFUCBSpace( pfucb ) );

    lrundoinfo.le_dbtime        = dbtimeNil;
    lrundoinfo.le_dbtimeBefore  = dbtimeNil;

    Assert( rceidNull != prce->Rceid() );
    lrundoinfo.le_rceid     = prce->Rceid();
    lrundoinfo.le_pgnoFDP   = prce->PgnoFDP();
    lrundoinfo.le_objidFDP  = prce->ObjidFDP();

    lrundoinfo.le_pgno      = pgnoUndoInfo;
    lrundoinfo.level    = prce->Level();
    lrundoinfo.le_trxBegin0 = prce->TrxBegin0();
    lrundoinfo.le_oper      = USHORT( prce->Oper() );
    if ( prce->FOperReplace() )
    {
        Assert( prce->CbData() > cbReplaceRCEOverhead );
        lrundoinfo.le_cbData    = USHORT( prce->CbData() - cbReplaceRCEOverhead );

        VERREPLACE* const pverreplace = (VERREPLACE*)prce->PbData();
        lrundoinfo.le_cbMaxSize = (SHORT) pverreplace->cbMaxSize;
        lrundoinfo.le_cbDelta   = (SHORT) pverreplace->cbDelta;
    }
    else
    {
        Assert( prce->CbData() == 0 );
        lrundoinfo.le_cbData = 0;
    }

    BOOKMARK    bm;
    prce->GetBookmark( &bm );
    Assert( bm.key.prefix.FNull() );
    lrundoinfo.SetCbBookmarkData( USHORT( bm.data.Cb() ) );
    lrundoinfo.SetCbBookmarkKey( USHORT( bm.key.suffix.Cb() ) );

    rgdata[0].SetPv( (BYTE *)&lrundoinfo );
    rgdata[0].SetCb( sizeof(LRUNDOINFO) );

    Assert( 0 == bm.key.prefix.Cb() );
    rgdata[1].SetPv( bm.key.suffix.Pv() );
    rgdata[1].SetCb( lrundoinfo.CbBookmarkKey() );

    rgdata[2].SetPv( bm.data.Pv() );
    rgdata[2].SetCb( lrundoinfo.CbBookmarkData() );

    rgdata[3].SetPv( const_cast<BYTE *>( prce->PbData() ) + cbReplaceRCEOverhead );
    rgdata[3].SetCb( lrundoinfo.le_cbData );

    if ( !fRetry )
    {
        err = plog->ErrLGTryLogRec( rgdata, 4, 0, 0 , plgpos );
    }
    else
    {
        err = plog->ErrLGLogRec( rgdata, 4, 0, 0 , plgpos );
    }

    return err;
}

ERR ErrLGUndoInfo( const RCE *prce, LGPOS *plgpos )
{
    return ErrLGIUndoInfo( prce, plgpos, fTrue );
}

ERR ErrLGUndoInfoWithoutRetry( const RCE *prce, LGPOS *plgpos )
{
    return ErrLGIUndoInfo( prce, plgpos, fFalse );
}


ERR ErrLGFlagDelete( const FUCB * const pfucb,
                     CSR    * const pcsr,
                     const RCEID    rceid,
                     const DIRFLAG  dirflag,
                     LGPOS      * const plgpos,
                     const VERPROXY * const pverproxy,
                     const BOOL             fDirtyCSR )

{
    ERR             err;
    LRFLAGDELETE    lrflagdelete;
    DATA            rgdata[1];

    INST *pinst = PinstFromIfmp( pfucb->ifmp );
    LOG *plog = pinst->m_plog;

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );
    Assert( fDirtyCSR || pcsr->FDirty() );

    if ( plog->FLogDisabled() )
    {
        if ( fDirtyCSR )
        {
            pcsr->Dirty();
        }
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( !( dirflag & fDIRNoLog ) );
    Assert( g_rgfmp[pfucb->ifmp].FLogOn() );

    Assert( NULL == pverproxy ||
            prceNil != pverproxy->prcePrimary &&
            ( pverproxy->prcePrimary->TrxCommitted() != trxMax ||
              pfucbNil != pverproxy->prcePrimary->Pfucb() &&
              ppibNil != pverproxy->prcePrimary->Pfucb()->ppib ) );

    PIB * const ppib = pverproxy != NULL &&
                    trxMax == pverproxy->prcePrimary->TrxCommitted() ?
                        pverproxy->prcePrimary->Pfucb()->ppib :
                        pfucb->ppib;

    Assert( ppib->Level() > 0 );

    CallR( ErrLGDeferBeginTransaction( ppib ) );

    lrflagdelete.lrtyp      = lrtypFlagDelete;

    Assert( pcsr->ILine() >= 0 );
    Assert( pcsr->ILine() < g_cbPage / 8 );
    lrflagdelete.SetILine( (USHORT)pcsr->ILine() );
    lrflagdelete.le_procid      = ppib->procid;

    ULONG fFlagsBefore = 0;
    CallR( ErrLGSetDbtimeBeforeAndDirty( pinst, pcsr, &lrflagdelete.le_dbtimeBefore, &lrflagdelete.le_dbtime, &fFlagsBefore, fDirtyCSR, fFalse ) );

    lrflagdelete.dbid           = g_rgfmp[ pfucb->ifmp ].Dbid();
    lrflagdelete.le_pgno        = pcsr->Pgno();

    Assert( !lrflagdelete.FVersioned() );
    Assert( !lrflagdelete.FDeleted() );
    Assert( !lrflagdelete.FUnique() );
    Assert( !lrflagdelete.FSpace() );
    Assert( !lrflagdelete.FConcCI() );
    Assert( !lrflagdelete.FSystemTask() );

    if ( !( dirflag & fDIRNoVersion ) )
        lrflagdelete.SetFVersioned();
    if ( pfucb->u.pfcb->FUnique() )
        lrflagdelete.SetFUnique();
    if ( FFUCBSpace( pfucb ) )
        lrflagdelete.SetFSpace();
    if ( NULL != pverproxy )
        lrflagdelete.SetFConcCI();

    LGISetTrx( ppib, &lrflagdelete, pverproxy );

    lrflagdelete.le_rceid       = rceid;
    lrflagdelete.le_pgnoFDP = PgnoFDP( pfucb );
    lrflagdelete.le_objidFDP    = ObjidFDP( pfucb );

    rgdata[0].SetPv( (BYTE *)&lrflagdelete );
    rgdata[0].SetCb( sizeof(LRFLAGDELETE) );

    err = plog->ErrLGLogRec( rgdata, 1, fDirtyCSR ? 0 : fLGMacroGoing, ppib->lgposStart.lGeneration, plgpos );

    if ( JET_errSuccess > err && fDirtyCSR )
    {
        Assert ( dbtimeInvalid !=  lrflagdelete.le_dbtimeBefore && dbtimeNil != lrflagdelete.le_dbtimeBefore);
        Assert ( pcsr->Dbtime() > lrflagdelete.le_dbtimeBefore );
        pcsr->RevertDbtime( lrflagdelete.le_dbtimeBefore, fFlagsBefore );
    }

    return err;
}


ERR ErrLGDelete(    const FUCB      *pfucb,
                    CSR             *pcsr,
                    LGPOS           *plgpos ,
                    const BOOL      fDirtyCSR )
{
    ERR         err;
    LRDELETE    lrdelete;
    DATA        rgdata[1];

    INST *pinst = PinstFromIfmp( pfucb->ifmp );
    LOG *plog = pinst->m_plog;

    Assert( fDirtyCSR || pcsr->FDirty() );
    if ( plog->FLogDisabled()
        || ( plog->FRecovering()
            && ( plog->FRecoveringMode() != fRecoveringUndo ) ) )
    {
        if ( fDirtyCSR )
        {
            pcsr->Dirty();
        }
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( g_rgfmp[pfucb->ifmp].FLogOn() );

    PIB     *ppib   = pfucb->ppib;
    Assert( ppib->Level() > 0 );



    CallR( ErrLGDeferBeginTransaction( ppib ) );

    lrdelete.lrtyp      = lrtypDelete;

    Assert( pcsr->ILine() >= 0 );
    Assert( pcsr->ILine() < g_cbPage / 8 );
    lrdelete.SetILine( (USHORT)pcsr->ILine() );
    lrdelete.le_procid      = ppib->procid;

    ULONG fFlagsBefore = 0;
    CallR( ErrLGSetDbtimeBeforeAndDirty( pinst, pcsr, &lrdelete.le_dbtimeBefore, &lrdelete.le_dbtime, &fFlagsBefore, fDirtyCSR, fFalse ) );

    lrdelete.le_pgnoFDP = PgnoFDP( pfucb );
    lrdelete.le_objidFDP    = ObjidFDP( pfucb );

    lrdelete.le_pgno        = pcsr->Pgno();
    lrdelete.dbid           = g_rgfmp[ pfucb->ifmp ].Dbid();

    Assert( !lrdelete.FVersioned() );
    Assert( !lrdelete.FDeleted() );
    Assert( !lrdelete.FUnique() );
    Assert( !lrdelete.FSpace() );
    Assert( !lrdelete.FConcCI() );
    Assert( !lrdelete.FSystemTask() );

    if ( pfucb->u.pfcb->FUnique() )
        lrdelete.SetFUnique();
    if ( FFUCBSpace( pfucb ) )
        lrdelete.SetFSpace();

    LGISetTrx( ppib, &lrdelete );

    rgdata[0].SetPv( (BYTE *)&lrdelete );
    rgdata[0].SetCb( sizeof(LRDELETE) );

    err = plog->ErrLGLogRec( rgdata, 1, fDirtyCSR ? 0 : fLGMacroGoing, ppib->lgposStart.lGeneration, plgpos );

    if ( JET_errSuccess > err && fDirtyCSR )
    {
        Assert ( dbtimeInvalid !=  lrdelete.le_dbtimeBefore && dbtimeNil != lrdelete.le_dbtimeBefore);
        Assert ( pcsr->Dbtime() > lrdelete.le_dbtimeBefore );
        pcsr->RevertDbtime( lrdelete.le_dbtimeBefore, fFlagsBefore );
    }

    return err;
}


ERR ErrLGUndo(  RCE             *prce,
                CSR             *pcsr,
                const BOOL      fDirtyCSR )
{
    ERR             err;
    FUCB *          pfucb           = prce->Pfucb();
    LRUNDO          lrundo;
    DATA            rgdata[3];
    LGPOS           lgpos;
    ULONG           fFlagsBefore    = 0;

    INST *pinst = PinstFromIfmp( pfucb->ifmp );
    LOG *plog = pinst->m_plog;

    Assert( fDirtyCSR );

    if ( plog->FLogDisabled()
        || ( plog->FRecovering()
            && ( plog->FRecoveringMode() != fRecoveringUndo ) ) )
    {
        pcsr->Dirty();
        Assert( pcsr->Dbtime() == pcsr->Cpage().Dbtime() );
        return JET_errSuccess;
    }

    if ( !g_rgfmp[pfucb->ifmp].FLogOn() )
    {
        pcsr->Dirty();
        Assert( pcsr->Dbtime() == pcsr->Cpage().Dbtime() );
        return JET_errSuccess;
    }

    Assert ( Pcsr( pfucb ) == pcsr );


    PIB     *ppib   = pfucb->ppib;
    Assert( ppib->Level() > 0 );

    Assert( pcsr->FLatched() );
    Assert( fDirtyCSR || pcsr->FDirty() );

    CallR( ErrLGDeferBeginTransaction( ppib ) );

    lrundo.lrtyp        = lrtypUndo;

    lrundo.level        = prce->Level();
    lrundo.le_procid        = ppib->procid;

    CallR( ErrLGSetDbtimeBeforeAndDirty( pinst, pcsr, &lrundo.le_dbtimeBefore, &lrundo.le_dbtime, &fFlagsBefore, fDirtyCSR, fFalse ) );

    lrundo.dbid             = g_rgfmp[prce->Pfucb()->ifmp].Dbid();
    lrundo.le_oper          = USHORT( prce->Oper() );
    Assert( lrundo.le_oper == prce->Oper() );
    Assert( lrundo.le_oper != operMaskNull );

    Assert( pcsr->ILine() >= 0 );
    Assert( pcsr->ILine() < g_cbPage / 8 );
    lrundo.SetILine( (USHORT)pcsr->ILine() );
    lrundo.le_pgno          = pcsr->Pgno();

    Assert( !lrundo.FVersioned() );
    Assert( !lrundo.FDeleted() );
    Assert( !lrundo.FUnique() );
    Assert( !lrundo.FSpace() );
    Assert( !lrundo.FConcCI() );
    Assert( !lrundo.FSystemTask() );

    if ( pfucb->u.pfcb->FUnique() )
        lrundo.SetFUnique();

    Assert( !FFUCBSpace( pfucb ) );

    LGISetTrx( ppib, &lrundo );

    lrundo.le_rceid     = prce->Rceid();
    lrundo.le_pgnoFDP   = PgnoFDP( pfucb );
    lrundo.le_objidFDP  = ObjidFDP( pfucb );

    BOOKMARK    bm;
    prce->GetBookmark( &bm );

    Assert( bm.key.prefix.FNull() );
    lrundo.SetCbBookmarkKey( USHORT( bm.key.Cb() ) );
    lrundo.SetCbBookmarkData( USHORT( bm.data.Cb() ) );

    rgdata[0].SetPv( (BYTE *)&lrundo );
    rgdata[0].SetCb( sizeof(LRUNDO) );

    rgdata[1].SetPv( bm.key.suffix.Pv() );
    rgdata[1].SetCb( lrundo.CbBookmarkKey() );

    rgdata[2].SetPv( bm.data.Pv() );
    rgdata[2].SetCb( bm.data.Cb() );
    Assert( !lrundo.FUnique() || bm.data.FNull() );

    Call( plog->ErrLGLogRec( rgdata, 3, fDirtyCSR ? 0 : fLGMacroGoing, ppib->lgposStart.lGeneration, &lgpos ) );
    CallS( err );
    Assert( pcsr->Latch() == latchWrite );

    pcsr->Cpage().SetLgposModify( lgpos );

    Assert ( JET_errSuccess <= err );

HandleError:
    if ( JET_errSuccess > err && fDirtyCSR )
    {
        Assert ( dbtimeInvalid !=  lrundo.le_dbtimeBefore && dbtimeNil != lrundo.le_dbtimeBefore);
        Assert ( pcsr->Dbtime() > lrundo.le_dbtimeBefore );
        pcsr->RevertDbtime( lrundo.le_dbtimeBefore, fFlagsBefore );
    }

    return err;
}

template< typename TDelta >
ERR ErrLGDelta( const FUCB      *pfucb,
                CSR     *pcsr,
                const BOOKMARK& bm,
                INT             cbOffset,
                TDelta          tDelta,
                RCEID           rceid,
                DIRFLAG         dirflag,
                LGPOS           *plgpos,
                const BOOL      fDirtyCSR )

{
    DATA                rgdata[4];
    _LRDELTA<TDelta>    lrdelta;
    ERR                 err;

    INST *pinst = PinstFromIfmp( pfucb->ifmp );
    LOG *plog = pinst->m_plog;

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );
    Assert( fDirtyCSR || pcsr->FDirty() );

    if ( plog->FLogDisabled() )
    {
        if ( fDirtyCSR )
        {
            pcsr->Dirty();
        }
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( g_rgfmp[pfucb->ifmp].FLogOn() );

    PIB     * const ppib = pfucb->ppib;

    Assert( ppib->Level() > 0 );

    CallR( ErrLGDeferBeginTransaction( ppib ) );

    lrdelta.lrtyp       = _LRDELTA< TDelta >::TRAITS::lrtyp;

    Assert( pcsr->ILine() >= 0 );
    Assert( pcsr->ILine() < g_cbPage / 8 );
    lrdelta.SetILine( (USHORT)pcsr->ILine() );
    lrdelta.le_procid       = ppib->procid;

    ULONG fFlagsBefore = 0;
    CallR( ErrLGSetDbtimeBeforeAndDirty( pinst, pcsr, &lrdelta.le_dbtimeBefore, &lrdelta.le_dbtime, &fFlagsBefore, fDirtyCSR, fFalse ) );

    lrdelta.dbid        = g_rgfmp[ pfucb->ifmp ].Dbid();
    lrdelta.le_pgno     = pcsr->Pgno();

    Assert( !lrdelta.FVersioned() );
    Assert( !lrdelta.FDeleted() );
    Assert( !lrdelta.FUnique() );
    Assert( !lrdelta.FSpace() );
    Assert( !lrdelta.FConcCI() );
    Assert( !lrdelta.FSystemTask() );

    if ( !( dirflag & fDIRNoVersion ) )
        lrdelta.SetFVersioned();
    if ( pfucb->u.pfcb->FUnique() )
        lrdelta.SetFUnique();

    Assert( !FFUCBSpace( pfucb ) );

    LGISetTrx( ppib, &lrdelta );

    lrdelta.le_rceid        = rceid;
    lrdelta.le_pgnoFDP      = PgnoFDP( pfucb );
    lrdelta.le_objidFDP = ObjidFDP( pfucb );

    lrdelta.SetCbBookmarkKey( USHORT( bm.key.Cb() ) );
    lrdelta.SetCbBookmarkData( USHORT( bm.data.Cb() ) );
    Assert( 0 == bm.data.Cb() );

    lrdelta.SetDelta( tDelta );

    Assert( cbOffset < g_cbPage );
    lrdelta.SetCbOffset( USHORT( cbOffset ) );

    rgdata[0].SetPv( (BYTE *)&lrdelta );
    rgdata[0].SetCb( sizeof( lrdelta ) );

    rgdata[1].SetPv( bm.key.prefix.Pv() );
    rgdata[1].SetCb( bm.key.prefix.Cb() );

    rgdata[2].SetPv( bm.key.suffix.Pv() );
    rgdata[2].SetCb( bm.key.suffix.Cb() );

    rgdata[3].SetPv( bm.data.Pv() );
    rgdata[3].SetCb( bm.data.Cb() );

    err = plog->ErrLGLogRec( rgdata, 4, fDirtyCSR ? 0 : fLGMacroGoing, ppib->lgposStart.lGeneration, plgpos );

    if ( JET_errSuccess > err && fDirtyCSR )
    {
        Assert ( dbtimeInvalid !=  lrdelta.le_dbtimeBefore && dbtimeNil != lrdelta.le_dbtimeBefore);
        Assert ( pcsr->Dbtime() > lrdelta.le_dbtimeBefore );
        pcsr->RevertDbtime( lrdelta.le_dbtimeBefore, fFlagsBefore );
    }

    return err;
}



LOCAL ERR ErrLGIDeferBeginTransaction( PIB *ppib )
{
    ERR         err;
    DATA        rgdata[1];
    LRBEGINDT   lrbeginDT;

    INST *pinst = PinstFromPpib( ppib );
    LOG *plog = pinst->m_plog;

    ENTERCRITICALSECTION    enterCritConcurrentDDL( &ppib->critConcurrentDDL );

    if ( 0 == ppib->clevelsDeferBegin )
    {
        return JET_errSuccess;
    }

    Assert( !ppib->FReadOnlyTrx() );
    Assert( !plog->FLogDisabled() );
    Assert( ppib->clevelsDeferBegin > 0 );
    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );


    CallR( plog->ErrLGCheckState() );

    Assert( ppib->procid < 64000 || ppib->procid == procidRCEClean  || ppib->procid == procidRCECleanCallback );
    lrbeginDT.le_procid = (USHORT) ppib->procid;

    lrbeginDT.levelBeginFrom = ppib->levelBegin;
    Assert( lrbeginDT.levelBeginFrom >= 0 );
    Assert( lrbeginDT.levelBeginFrom <= levelMax );
    lrbeginDT.clevelsToBegin = (BYTE)ppib->clevelsDeferBegin;
    Assert( lrbeginDT.clevelsToBegin >= 0 );
    Assert( lrbeginDT.clevelsToBegin <= levelMax );

    rgdata[0].SetPv( (BYTE *) &lrbeginDT );
    if ( 0 == lrbeginDT.levelBeginFrom )
    {
        Assert( ppib->trxBegin0 != trxMax );
        lrbeginDT.le_trxBegin0 = ppib->trxBegin0;
        lrbeginDT.lrtyp = lrtypBegin0;
        rgdata[0].SetCb( sizeof(LRBEGIN0) );
    }
    else
    {
        lrbeginDT.lrtyp = lrtypBegin;
        rgdata[0].SetCb( sizeof(LRBEGIN) );
    }

    ppib->critLogBeginTrx.Enter();

    Assert( ppib->levelBegin != 0 || !ppib->FLoggedCheckpointGettingDeep() );

    LGPOS lgposLogRec;
    err = plog->ErrLGLogRec( rgdata, 1, 0, ppib->lgposStart.lGeneration, &lgposLogRec, &ppib->critLogBeginTrx );

    if ( err >= 0 )
    {
        ppib->clevelsDeferBegin = 0;
        if ( 0 == lrbeginDT.levelBeginFrom )
        {
            ppib->SetFBegin0Logged();
            ppib->lgposStart = lgposLogRec;
        }
    }

    ppib->critLogBeginTrx.Leave();

    return err;
}

#ifdef DEBUG
VOID LGJetOp( JET_SESID sesid, INT op )
{
    DATA        rgdata[1];
    PIB         *ppib = (PIB *) sesid;
    LRJETOP     lrjetop;

    if ( !sesid || sesid == JET_sesidNil )
        return;

    INST *pinst = PinstFromPpib( ppib );
    LOG *plog = pinst->m_plog;

    if ( !plog->FLogDisabled() && !plog->FRecovering() )
    {
        Assert( plog->FLogDisabled() == fFalse );
        Assert( plog->FRecovering() == fFalse );

        lrjetop.lrtyp = lrtypJetOp;
        Assert( ppib->procid < 64000 );
        lrjetop.le_procid = (USHORT) ppib->procid;
        lrjetop.op = (BYTE)op;
        rgdata[0].SetPv( (BYTE *) &lrjetop );
        rgdata[0].SetCb( sizeof(LRJETOP) );

        plog->ErrLGLogRec( rgdata, 1, 0, ppib->lgposStart.lGeneration, pNil );
    }
}
#endif

ERR ErrLGBeginTransaction( PIB * const ppib )
{
    const INST  * const pinst   = PinstFromPpib( ppib );
    const LOG   * const plog    = pinst->m_plog;

    if ( plog->FLogDisabled() )
        return JET_errSuccess;

    if ( 0 == ppib->clevelsDeferBegin )
    {
        ppib->levelBegin = ppib->Level();
    }

    ppib->clevelsDeferBegin++;
    Assert( ppib->clevelsDeferBegin < levelMax );

    return JET_errSuccess;
}


ERR ErrLGCommitTransaction( PIB *ppib, const LEVEL levelCommitTo, BOOL fFireRedoCallback, TRX *ptrxCommit0, LGPOS *plgposRec )
{
    ERR         err;
    DATA        rgdata[3];
    INT         cdata;
    LRCOMMIT0   lrcommit0;
    LRCOMMITCTX lrcommitC( ppib->CbClientCommitContextGeneric() );

    INST *pinst = PinstFromPpib( ppib );
    LOG *plog = pinst->m_plog;

    *plgposRec = lgposMax;

    if ( plog->FLogDisabled() )
        return JET_errSuccess;

    if ( ppib->clevelsDeferBegin > 0 )
    {
        ppib->clevelsDeferBegin--;
        if ( levelCommitTo == 0 )
        {
            Assert( 0 == ppib->clevelsDeferBegin );
            Assert( !ppib->FBegin0Logged() );
            *plgposRec = lgposMin;
        }
        return JET_errSuccess;
    }

    Assert( ppib->procid < 64000 || ppib->procid == procidRCEClean  || ppib->procid == procidRCECleanCallback );
    lrcommit0.le_procid = (USHORT) ppib->procid;
    lrcommit0.levelCommitTo = levelCommitTo;

    rgdata[0].SetPv( (BYTE *)&lrcommit0 );
    cdata = 1;

    if ( levelCommitTo == 0 )
    {
        lrcommit0.lrtyp = lrtypCommit0;

        rgdata[0].SetCb( sizeof(LRCOMMIT0) );


        if ( ppib->CbClientCommitContextGeneric() )
        {
            Assert( lrcommitC.CbCommitCtx() == ppib->CbClientCommitContextGeneric() );
            Expected( lrcommitC.CbCommitCtx() <= cbCommitCtxExpected );

            lrcommitC.SetProcID( ppib->procid );
            if ( fFireRedoCallback )
            {
                lrcommitC.SetFCallbackNeeded();
            }
            lrcommitC.SetContainsCustomerData( ppib->FCommitContextContainsCustomerData() );

            rgdata[1].SetPv( &lrcommitC );
            rgdata[1].SetCb( sizeof(LRCOMMITCTX) );
            rgdata[2].SetPv( const_cast<VOID*>( ppib->PvClientCommitContextGeneric() ) );
            rgdata[2].SetCb( ppib->CbClientCommitContextGeneric() );

            cdata += 2;
        }
    }
    else
    {
        lrcommit0.lrtyp = lrtypCommit;
        rgdata[0].SetCb( sizeof(LRCOMMIT) );
    }

    err = plog->ErrLGLogRec( rgdata, cdata, 0, ppib->lgposStart.lGeneration, plgposRec );

    if ( 0 == levelCommitTo
        && err >= 0 )
    {
        ppib->ResetFBegin0Logged();
        Assert( lrcommit0.le_trxCommit0 != trxMax );
        *ptrxCommit0 = lrcommit0.le_trxCommit0;
    }

    return err;
}

ERR ErrLGRollback( PIB *ppib, LEVEL levelsRollback, BOOL fRollbackToLevel0, BOOL fFireRedoCallback )
{
    ERR         err;
    DATA        rgdata[3];
    INT         cdata;
    LRROLLBACK  lrrollback;
    INST        * pinst     = PinstFromPpib( ppib );
    LOG         * plog      = pinst->m_plog;
    LRCOMMITCTX lrcommitC( ppib->CbClientCommitContextGeneric() );

    if ( plog->FLogDisabled() )
        return JET_errSuccess;

    if ( ppib->clevelsDeferBegin > 0 )
    {
        if ( ppib->clevelsDeferBegin >= levelsRollback )
        {
            ppib->clevelsDeferBegin = LEVEL( ppib->clevelsDeferBegin - levelsRollback );
            return JET_errSuccess;
        }
        levelsRollback = LEVEL( levelsRollback - ppib->clevelsDeferBegin );
        ppib->clevelsDeferBegin = 0;
    }

    Assert( levelsRollback > 0 );
    lrrollback.lrtyp = lrtypRollback;
    Assert( ppib->procid < 64000 || ppib->procid == procidRCEClean  || ppib->procid == procidRCECleanCallback );
    lrrollback.le_procid = (USHORT) ppib->procid;
    lrrollback.levelRollback = levelsRollback;

    rgdata[0].SetPv( (BYTE *)&lrrollback );
    rgdata[0].SetCb( sizeof(LRROLLBACK) );
    cdata = 1;


    if ( fRollbackToLevel0 && ppib->CbClientCommitContextGeneric() )
    {
        Assert( lrcommitC.CbCommitCtx() == ppib->CbClientCommitContextGeneric() );
        Expected( lrcommitC.CbCommitCtx() <= cbCommitCtxExpected );

        lrcommitC.SetProcID( ppib->procid );
        if ( fFireRedoCallback )
        {
            lrcommitC.SetFCallbackNeeded();
        }
        lrcommitC.SetContainsCustomerData( ppib->FCommitContextContainsCustomerData() );

        rgdata[1].SetPv( &lrcommitC );
        rgdata[1].SetCb( sizeof(LRCOMMITCTX) );
        rgdata[2].SetPv( const_cast<VOID*>( ppib->PvClientCommitContextGeneric() ) );
        rgdata[2].SetCb( ppib->CbClientCommitContextGeneric() );

        cdata += 2;
    }

    err = plog->ErrLGLogRec( rgdata, cdata, 0, ppib->lgposStart.lGeneration, pNil );

    if ( 0 == ppib->Level() )
    {
        ppib->ResetFBegin0Logged();
    }

    return err;
}



ERR ErrLGWaitForWrite( PIB* const ppib, const LGPOS* const plgposLogRec )
{
    ERR         err     = JET_errSuccess;
    INST* const pinst   = PinstFromPpib( ppib );
    LOG* const  plog    = pinst->m_plog;

    Call( plog->ErrLGWaitForWrite( ppib, plgposLogRec ) );

HandleError:
    Assert( err >= 0 || ( plog->FNoMoreLogWrite() && JET_errLogWriteFail == err ) );
    return err;
}

ERR ErrLGFlush( LOG* const plog, const IOFLUSHREASON iofr, const BOOL fMayOwnBFLatch )
{
    return plog->ErrLGFlush( iofr, fMayOwnBFLatch );
}

ERR ErrLGWrite( PIB* const ppib )
{
    ERR         err     = JET_errSuccess;
    INST* const pinst   = PinstFromPpib( ppib );
    LOG* const  plog    = pinst->m_plog;

    plog->FLGSignalWrite();

    return err;
}

ERR ErrLGScheduleWrite( PIB* const ppib, DWORD cmsecDurableCommit, LGPOS lgposCommit )
{
    LOG *plog = PinstFromPpib( ppib )->m_plog;
    return plog->ErrLGScheduleWrite( cmsecDurableCommit, lgposCommit );
}

ERR ErrLGCreateDB(
    _In_ PIB *                  ppib,
    _In_ const IFMP             ifmp,
    _In_ const JET_GRBIT        grbit,
    _In_ BOOL                   fSparseFile,
    _Out_ LGPOS * const         plgposRec
    )
{
    ERR                     err;
    FMP *                   pfmp            = &g_rgfmp[ifmp];
    const USHORT            cbDbName        = USHORT( sizeof( WCHAR ) * ( LOSStrLengthW( pfmp->WszDatabaseName() ) + 1 ) );
    DATA                    rgdata[3];
    ULONG                   idata           = 0;
    INST *                  pinst           = PinstFromPpib( ppib );
    LOG *                   plog            = pinst->m_plog;
    LRCREATEDB              lrcreatedb;
    LRCREATEDB::VersionInfo verinfo;

    Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposDetach() ) );
    Assert( NULL != pfmp->Pdbfilehdr() );
    Assert( !pfmp->FLogOn() || !plog->FLogDisabled() );
    if ( plog->FRecovering() || !pfmp->FLogOn() )
    {
        *plgposRec = lgposMin;
        return JET_errSuccess;
    }

    Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposAttach() ) );
    CallR( plog->ErrLGCheckState() );

    lrcreatedb.lrtyp = lrtypCreateDB;
    Assert( ppib->procid < 64000 );
    lrcreatedb.le_procid = (USHORT) ppib->procid;
    FMP::AssertVALIDIFMP( ifmp );
    lrcreatedb.dbid = pfmp->Dbid();

    lrcreatedb.le_grbit = grbit;
    lrcreatedb.signDb = pfmp->Pdbfilehdr()->signDb;
    lrcreatedb.le_cpgDatabaseSizeMax = pfmp->CpgDatabaseSizeMax();

    lrcreatedb.SetFVersionInfo();
    verinfo.mle_usVersion = usDAECreateDbVersion;
    verinfo.mle_usUpdateMajor = usDAECreateDbUpdateMajor;

    lrcreatedb.SetFUnicodeNames();

    if ( fSparseFile )
    {
        lrcreatedb.SetFSparseEnabledFile();
    }
    
    Assert( cbDbName > 1 );
    lrcreatedb.SetCbPath( USHORT( cbDbName ) );

    rgdata[idata].SetPv( (BYTE *)&lrcreatedb );
    rgdata[idata].SetCb( sizeof(LRCREATEDB) );
    idata++;

    rgdata[idata].SetPv( (BYTE *)&verinfo );
    rgdata[idata].SetCb( sizeof(LRCREATEDB::VersionInfo) );
    idata++;

    rgdata[idata].SetPv( (BYTE *)pfmp->WszDatabaseName() );
    rgdata[idata].SetCb( cbDbName );
    idata++;

    Assert( pfmp->RwlDetaching().FNotWriter() );
    Assert( pfmp->RwlDetaching().FNotReader() );

    pfmp->RwlDetaching().EnterAsWriter();
    while ( errLGNotSynchronous == ( err = plog->ErrLGTryLogRec( rgdata, idata, 0, ppib->lgposStart.lGeneration, plgposRec ) ) )
    {
        pfmp->RwlDetaching().LeaveAsWriter();
        UtilSleep( cmsecWaitLogWrite );
        pfmp->RwlDetaching().EnterAsWriter();
    }
    if ( err >= JET_errSuccess )
    {
        pfmp->SetLgposAttach( *plgposRec );
    }
    pfmp->RwlDetaching().LeaveAsWriter();

    CallR( err );

    LGPOS   lgposStartOfLastRec = *plgposRec;
    plog->LGAddLgpos( &lgposStartOfLastRec, rgdata[0].Cb() + rgdata[1].Cb() + rgdata[2].Cb() - 1 );
    err = ErrLGWaitForWrite( ppib, &lgposStartOfLastRec );
    return err;
}


ERR ErrLGCreateDBFinish(
    PIB *                   ppib,
    const IFMP              ifmp,
    LGPOS *                 plgposRec
    )
{
    ERR                     err;
    FMP *                   pfmp            = &g_rgfmp[ifmp];
    DATA                    rgdata;
    INST *                  pinst           = PinstFromPpib( ppib );
    LOG *                   plog            = pinst->m_plog;
    LRCREATEDBFINISH        lrCreateDBFinish;

    Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposDetach() ) );
    Assert( NULL != pfmp->Pdbfilehdr() );
    Assert( !pfmp->FLogOn() || !plog->FLogDisabled() );
    if ( plog->FRecovering() || !pfmp->FLogOn() )
    {
        *plgposRec = lgposMin;
        return JET_errSuccess;
    }

    lrCreateDBFinish.lrtyp = lrtypCreateDBFinish;
    Assert( ppib->procid < 64000 );
    lrCreateDBFinish.le_procid = (USHORT) ppib->procid;
    FMP::AssertVALIDIFMP( ifmp );
    lrCreateDBFinish.dbid = pfmp->Dbid();

    rgdata.SetPv( (BYTE *)&lrCreateDBFinish );
    rgdata.SetCb( sizeof(LRCREATEDBFINISH) );

    Assert( pfmp->RwlDetaching().FNotWriter() );
    Assert( pfmp->RwlDetaching().FNotReader() );

    CallR( plog->ErrLGLogRec( &rgdata, 1, 0, ppib->lgposStart.lGeneration, plgposRec ) );

    LGPOS   lgposStartOfLastRec = *plgposRec;
    plog->LGAddLgpos( &lgposStartOfLastRec, sizeof( LRCREATEDBFINISH ) - 1 );
    err = ErrLGWaitForWrite( ppib, &lgposStartOfLastRec );
    return err;
}


ERR ErrLGAttachDB(
    _In_ PIB                *ppib,
    _In_ const IFMP         ifmp,
    _In_ const BOOL         fSparseFile,
    _Out_ LGPOS *const      plgposRec )
{
    ERR             err;
    FMP             *pfmp           = &g_rgfmp[ifmp];
    Assert( NULL != pfmp->Pdbfilehdr() );
    const ULONG     cbDbName        = sizeof( WCHAR ) * ((ULONG)LOSStrLengthW( pfmp->WszDatabaseName() ) + 1);
    DATA            rgdata[4];
    ULONG           cdata           = 2;
    LRATTACHDB      lrattachdb;
    INST            *pinst          = PinstFromPpib( ppib );
    LOG             *plog           = pinst->m_plog;

    Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposDetach() ) );
    Assert( !pfmp->FLogOn() || !plog->FLogDisabled() );
    Assert( !plog->FRecovering() );
    if ( ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo )
        || !pfmp->FLogOn() )
    {
        *plgposRec = lgposMin;
        return JET_errSuccess;
    }

    Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposAttach() ) );
    CallR( plog->ErrLGCheckState() );

    lrattachdb.lrtyp = lrtypAttachDB;
    Assert( ppib->procid < 64000 );
    lrattachdb.le_procid = (USHORT) ppib->procid;
    lrattachdb.dbid = pfmp->Dbid();

    Assert( !g_rgfmp[ifmp].FReadOnlyAttach() );

    lrattachdb.signDb = pfmp->Pdbfilehdr()->signDb;
    lrattachdb.le_cpgDatabaseSizeMax = pfmp->CpgDatabaseSizeMax();
    lrattachdb.signLog = pfmp->Pdbfilehdr()->signLog;
    lrattachdb.lgposConsistent = pfmp->Pdbfilehdr()->le_lgposConsistent;

    lrattachdb.SetFUnicodeNames( );
    Assert( cbDbName > 1 );
    lrattachdb.SetCbPath( USHORT( cbDbName ) );

    if ( fSparseFile )
    {
        lrattachdb.SetFSparseEnabledFile();
    }

    rgdata[0].SetPv( (BYTE *)&lrattachdb );
    rgdata[0].SetCb( sizeof(LRATTACHDB) );
    rgdata[1].SetPv( (BYTE *)pfmp->WszDatabaseName() );
    rgdata[1].SetCb( cbDbName );

    Assert( pfmp->RwlDetaching().FNotWriter() );
    Assert( pfmp->RwlDetaching().FNotReader() );

    pfmp->RwlDetaching().EnterAsWriter();
    while ( errLGNotSynchronous == ( err = plog->ErrLGTryLogRec( rgdata, cdata, 0, ppib->lgposStart.lGeneration, plgposRec ) ) )
    {
        pfmp->RwlDetaching().LeaveAsWriter();
        UtilSleep( cmsecWaitLogWrite );
        pfmp->RwlDetaching().EnterAsWriter();
    }
    if ( err >= JET_errSuccess )
    {
        pfmp->SetLgposAttach( *plgposRec );
    }
    pfmp->RwlDetaching().LeaveAsWriter();
    CallR( err );

    LGPOS   lgposStartOfLastRec = *plgposRec;
    plog->LGAddLgpos( &lgposStartOfLastRec, rgdata[0].Cb() + rgdata[1].Cb() - 1 );
    err = ErrLGWaitForWrite( ppib, &lgposStartOfLastRec );
    return err;
}


ERR ErrLGReAttachDB(
    _In_ const IFMP         ifmp,
    _Out_ LGPOS *const      plgposRec )
{
    LRREATTACHDB lrreattachdb;
    DATA rgdata[1];
    FMP *pfmp = &g_rgfmp[ifmp];
    LOG *plog = PinstFromIfmp( ifmp )->m_plog;

    Assert( !pfmp->FLogOn() || !plog->FLogDisabled() );
    Assert( !plog->FRecovering() );
    if ( !pfmp->FLogOn() )
    {
        *plgposRec = lgposMin;
        return JET_errSuccess;
    }

    lrreattachdb.le_dbid = pfmp->Dbid();
    lrreattachdb.lgposAttach = pfmp->Pdbfilehdr()->le_lgposAttach;
    lrreattachdb.lgposConsistent = pfmp->Pdbfilehdr()->le_lgposConsistent;
    lrreattachdb.signDb = pfmp->Pdbfilehdr()->signDb;

    rgdata[0].SetPv( (BYTE *)&lrreattachdb );
    rgdata[0].SetCb( sizeof(lrreattachdb) );

    return plog->ErrLGLogRec( rgdata, 1, 0, 0, plgposRec );
}

ERR ErrLGSignalAttachDB( _In_ const IFMP ifmp )
{
    LRSIGNALATTACHDB lrsattachdb;
    FMP *pfmp = &g_rgfmp[ifmp];
    LOG *plog = PinstFromIfmp( ifmp )->m_plog;

    Assert( !pfmp->FLogOn() || !plog->FLogDisabled() );
    Assert( !plog->FRecovering() );
    if ( !pfmp->FLogOn() )
    {
        return JET_errSuccess;
    }

    lrsattachdb.SetDbid( pfmp->Dbid() );

    DATA data;
    LGPOS lgpos;
    data.SetPv( (BYTE *)&lrsattachdb );
    data.SetCb( sizeof(lrsattachdb) );
    return plog->ErrLGLogRec( &data, 1, 0, 0, &lgpos );
}

ERR ErrLGSetDbVersion(
    _In_ PIB*               ppib,
    _In_ const IFMP         ifmp,
    _In_ const DbVersion&   dbvUpdate,
    _Out_ LGPOS *           plgposRec )
{
    ERR             err;
    DATA            rgdata[1];
    LRSETDBVERSION  lrsetdbversion;
    INST            *pinst          = PinstFromPpib( ppib );
    LOG             *plog           = pinst->m_plog;
    FMP::AssertVALIDIFMP( ifmp );
    FMP             *pfmp           = &g_rgfmp[ifmp];

    Assert( !pinst->FRecovering() || pinst->m_plog->FRecoveringMode() != fRecoveringRedo );
    Assert( !pfmp->FReadOnlyAttach() );
    Assert( pfmp->FLogOn() );
    Expected( CmpDbVer( pfmp->Pdbfilehdr()->Dbv(), dbvUpdate ) <= 0 );

    Assert( lrsetdbversion.lrtyp == lrtypSetDbVersion );

    lrsetdbversion.SetDbid( pfmp->Dbid() );
    lrsetdbversion.SetDbVersion( dbvUpdate );

    rgdata[0].SetPv( (BYTE *)&lrsetdbversion );
    rgdata[0].SetCb( sizeof(lrsetdbversion) );

    Assert( _countof( rgdata ) == 1 );
    Call( plog->ErrLGLogRec( rgdata, 1, 0, ppib->lgposStart.lGeneration, plgposRec ) );

    OnDebug( pfmp->SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusUpdateLogged ) );

HandleError:
    return err;
}


ERR ErrLGForceWriteLog( PIB * ppib )
{
    ERR                     err;
    DATA                    rgdata[1];
    LRFORCEWRITELOG     lrForceWriteLog;
    LGPOS                   lgposStartOfLastRec;

    INST *                  pinst               = PinstFromPpib( ppib );
    LOG *                   plog                = pinst->m_plog;

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );
    if ( plog->FLogDisabled() || ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo ) )
    {
        return JET_errSuccess;
    }

    lrForceWriteLog.lrtyp = lrtypForceWriteLog;
    Assert( ppib->procid < 64000 || ppib->procid == procidRCEClean  || ppib->procid == procidRCECleanCallback );
    lrForceWriteLog.le_procid = (USHORT) ppib->procid;

    rgdata[0].SetPv( (BYTE *)&lrForceWriteLog );
    rgdata[0].SetCb( sizeof(lrForceWriteLog) );

    CallR( plog->ErrLGLogRec( rgdata, sizeof(rgdata) / sizeof(rgdata[0]), 0, ppib->lgposStart.lGeneration, &lgposStartOfLastRec ) );

    plog->LGAddLgpos( &lgposStartOfLastRec, rgdata[0].Cb() - 1 );
    return ErrLGWaitForWrite( ppib, &lgposStartOfLastRec );
}


ERR ErrLGForceLogRollover(
    PIB * const         ppib,
    __in PSTR           szTrace,
    LGPOS* const        plgposLogRec )
{
    INST * const        pinst       = PinstFromPpib( ppib );
    LOG * const         plog        = pinst->m_plog;
    const ULONG         cbTrace     = ( NULL != szTrace ? (ULONG)LOSStrLengthA( szTrace ) + 1 : 0 );
    ULONG               idata       = 0;
    const ULONG         cdata       = 3;
    DATA                rgdata[ cdata ];
    LRFORCELOGROLLOVER  lrForceLogRollover;

    if ( plog->FLogDisabled() || plog->FRecovering() )
    {
        return JET_errSuccess;
    }

    lrForceLogRollover.lrtyp = lrtypForceLogRollover;
    Assert( ppib->procid < 64000 );
    lrForceLogRollover.le_procid = (USHORT)ppib->procid;
    lrForceLogRollover.le_cb = (USHORT)cbTrace;

    rgdata[ idata ].SetPv( (BYTE *)&lrForceLogRollover );
    rgdata[ idata ].SetCb( sizeof( lrForceLogRollover ) );
    idata++;

    if ( cbTrace > 1 )
    {
        rgdata[ idata ].SetPv( (BYTE *)szTrace );
        rgdata[ idata ].SetCb( cbTrace );
        idata++;
    }

    Assert( idata <= cdata );
    return plog->ErrLGLogRec( rgdata, idata, fLGCreateNewGen, ppib->lgposStart.lGeneration, plgposLogRec );
}


ERR ErrLGDetachDB(
    PIB         *ppib,
    const IFMP  ifmp,
    BYTE        flags,
    LGPOS       *plgposRec )
{
    ERR         err;
    FMP         *pfmp       = &g_rgfmp[ifmp];
    INST        *pinst      = PinstFromPpib( ppib );
    LOG         *plog       = pinst->m_plog;

    WCHAR *     wszDatabaseName = pfmp->WszDatabaseName();
    Assert ( wszDatabaseName );

    if ( plog->FRecovering() )
    {
        WCHAR *wszRstmapDbName;
        INT irstmap = plog->IrstmapSearchNewName( wszDatabaseName, &wszRstmapDbName );

        if ( 0 <= irstmap )
        {
            wszDatabaseName = wszRstmapDbName;
        }
    }
    Assert ( wszDatabaseName );

    const ULONG cbDbName    = sizeof(WCHAR) * ((ULONG)LOSStrLengthW( wszDatabaseName ) + 1);
    DATA        rgdata[2];
    LRDETACHDB  lrdetachdb;

    Assert( !pfmp->FLogOn() || !plog->FLogDisabled() );
    if ( ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo )
        || !pfmp->FLogOn() )
    {
        if ( plog->FRecovering() )
        {
            Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposDetach() ) );
            pfmp->SetLgposDetach( plog->LgposLGLogTipNoLock() );
        }
        *plgposRec = lgposMin;
        return JET_errSuccess;
    }

    Assert( !pfmp->FReadOnlyAttach() );
    Assert( 0 != CmpLgpos( lgposMin, pfmp->LgposAttach() ) || NULL == pfmp->Pdbfilehdr() );

    lrdetachdb.lrtyp = lrtypDetachDB;
    Assert( ppib->procid < 64000 );
    lrdetachdb.le_procid = (USHORT) ppib->procid;
    lrdetachdb.dbid = pfmp->Dbid();
    lrdetachdb.SetCbPath( cbDbName );

    lrdetachdb.SetFUnicodeNames();
    
    rgdata[0].SetPv( (BYTE *)&lrdetachdb );
    rgdata[0].SetCb( sizeof(LRDETACHDB) );
    rgdata[1].SetPv( (BYTE *)wszDatabaseName );
    rgdata[1].SetCb( cbDbName );

    Assert( pfmp->RwlDetaching().FNotWriter() );
    Assert( pfmp->RwlDetaching().FNotReader() );

    if ( 0 != CmpLgpos( lgposMin, pfmp->LgposDetach() ) )
    {
        *plgposRec = pfmp->LgposDetach();
    }
    else
    {
        pfmp->RwlDetaching().EnterAsWriter();
        while ( errLGNotSynchronous == ( err = plog->ErrLGTryLogRec( rgdata, 2, 0, ppib->lgposStart.lGeneration, plgposRec ) ) )
        {
            pfmp->RwlDetaching().LeaveAsWriter();
            UtilSleep( cmsecWaitLogWrite );
            pfmp->RwlDetaching().EnterAsWriter();
        }
        if ( err >= JET_errSuccess )
        {
            Assert( 0 != CmpLgpos( lgposMin, pfmp->LgposAttach() ) );
            Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposDetach() ) );
            pfmp->SetLgposDetach( *plgposRec );
        }
        pfmp->RwlDetaching().LeaveAsWriter();
        CallR( err );
    }

    LGPOS   lgposStartOfLastRec = *plgposRec;
    plog->LGAddLgpos( &lgposStartOfLastRec, rgdata[0].Cb() + rgdata[1].Cb() - 1 );
    err = ErrLGWaitForWrite( ppib, &lgposStartOfLastRec );
    return err;
}


VOID LGICompressPreImage(
    INST *pinst,
    const CPAGE &cpage,
    const LONG cbPage,
    DATA &dataToSet,
    _Pre_bytecap_( cbPage ) BYTE *pbDataDehydrated,
    _Pre_bytecap_( cbPage ) BYTE *pbDataCompressed,
    ULONG *pcompressionPerformed )
{
    *pcompressionPerformed = 0;
    INT cbDataCompressedActual = 0;
    if ( cpage.FPageIsDehydratable( (ULONG *)&cbDataCompressedActual, fTrue ) &&
         cbDataCompressedActual <= (cbPage*9)/10 )
    {
        memcpy( pbDataDehydrated, cpage.PvBuffer(), cbPage );
        CPAGE cpageT;
        cpageT.LoadPage( cpage.Ifmp(), cpage.PgnoThis(), pbDataDehydrated, cbPage );
        cpageT.DehydratePage( cbDataCompressedActual, fTrue );
        dataToSet.SetPv( pbDataDehydrated );
        dataToSet.SetCb( cbDataCompressedActual );
        *pcompressionPerformed |= fPreimageDehydrated;
        cpageT.UnloadPage();
    }

    CompressFlags compressFlags = compressXpress;
    if ( BoolParam( pinst, JET_paramFlight_EnableXpress10Compression ) &&
         pinst->m_plog->ErrLGFormatFeatureEnabled( JET_efvXpress10Compression ) >= JET_errSuccess )
    {
        compressFlags = CompressFlags( compressFlags | compressXpress10 );
    }

    if ( ErrPKCompressData( dataToSet, compressFlags, pinst, pbDataCompressed, cbPage, &cbDataCompressedActual ) >= JET_errSuccess &&
             cbDataCompressedActual < dataToSet.Cb() )
    {
        dataToSet.SetPv( pbDataCompressed );
        dataToSet.SetCb( cbDataCompressedActual );
        *pcompressionPerformed |= fPreimageXpress;
    }
    else if ( *pcompressionPerformed & fPreimageDehydrated )
    {
        Enforce( dataToSet.Cb() <= cbPage );
        AssumePREFAST( dataToSet.Cb() <= cbPage );
        const size_t cbToCopy = min( cbPage, dataToSet.Cb() );

#pragma prefast(suppress:26015, "Prefast believes that there is a BO when dataToSet.Cb() returns a larger value, even though we min() it with the buffer size.")
        memcpy( pbDataCompressed, pbDataDehydrated, cbToCopy );
        dataToSet.SetPv( pbDataCompressed );
    }
}

ERR ErrLGIDecompressPreimage(
    DATA &data,
    const LONG cbPage,
    BYTE *pbDataDecompressed,
    IFMP ifmp,
    PGNO pgno,
    BOOL fDehydrated,
    BOOL fXpressed );

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTESTDB( PreImageCompression, Dehydration, dwOpenDatabase )
{
    const ULONG cbPage = 32 * 1024;
    CPAGE cpage, cpageT;
    FMP * pfmp = g_rgfmp + IfmpTest();
    LINE line, lineT;
    INT iline = 0;
    DATA data, dataRec;
    BYTE rgbData[5];
    BYTE *pbDehydrationBuffer = NULL, *pbCompressionBuffer = NULL;
    ULONG compressionPerformed;

    cpage.LoadNewTestPage( cbPage, IfmpTest() );
    memset( rgbData, '0', sizeof(rgbData) );
    dataRec.SetPv( rgbData );
    dataRec.SetCb( sizeof(rgbData) );
    cpage.Insert( 0, &dataRec, 1, 0 );

    data.SetPv( cpage.PvBuffer() );
    data.SetCb( cbPage );
    pbDehydrationBuffer = new BYTE[cbPage];
    pbCompressionBuffer = new BYTE[cbPage];

    PKTermCompression();
    CHECK( JET_errSuccess == ErrPKInitCompression( cbPage, 1024, cbPage ) );

    LGICompressPreImage( pfmp->Pinst(), cpage, cbPage, data, pbDehydrationBuffer, pbCompressionBuffer, &compressionPerformed );
    CHECK( compressionPerformed == fPreimageDehydrated );
    CHECK( data.Pv() == pbCompressionBuffer );
    CHECK( data.Cb() < cbPage );

    CHECK( JET_errSuccess == ErrLGIDecompressPreimage( data, cbPage, pbDehydrationBuffer, pfmp->Ifmp(), cpage.PgnoThis(), fTrue, fFalse ) );
    CHECK( data.Pv() == pbDehydrationBuffer );
    CHECK( data.Cb() == cbPage );

    cpageT.LoadPage( data.Pv(), data.Cb() );
    CHECK( cpage.Clines() == cpageT.Clines() );
    cpage.GetPtrExternalHeader( &line );
    cpageT.GetPtrExternalHeader( &lineT );
    iline = 0;
    while ( fTrue )
    {
        CHECK( line.cb == lineT.cb );
        CHECK( memcmp( line.pv, lineT.pv, line.cb ) == 0 );
        CHECK( line.fFlags == lineT.fFlags );

        if ( iline >= cpage.Clines() )
            break;

        cpage.GetPtr( iline, &line );
        cpageT.GetPtr( iline, &lineT );
        iline++;
    }

    delete[] pbCompressionBuffer;
    delete[] pbDehydrationBuffer;
}


JETUNITTESTDB( PreImageCompression, DehydrationAndXpress, dwOpenDatabase )
{
    const ULONG cbPage = 32 * 1024;
    CPAGE cpage, cpageT;
    FMP * pfmp = g_rgfmp + IfmpTest();
    LINE line, lineT;
    INT iline = 0;
    DATA data, dataRec;
    BYTE rgbData[100];
    BYTE *pbDehydrationBuffer = NULL, *pbCompressionBuffer = NULL;
    ULONG compressionPerformed;

    cpage.LoadNewTestPage( cbPage, IfmpTest() );
    memset( rgbData, '0', sizeof(rgbData) );
    dataRec.SetPv( rgbData );
    dataRec.SetCb( sizeof(rgbData) );
    for ( iline=0; iline<100; iline++ )
    {
        cpage.Insert( iline, &dataRec, 1, 0 );
    }

    data.SetPv( cpage.PvBuffer() );
    data.SetCb( cbPage );
    pbDehydrationBuffer = new BYTE[cbPage];
    pbCompressionBuffer = new BYTE[cbPage];
    
    PKTermCompression();
    CHECK( JET_errSuccess == ErrPKInitCompression( cbPage, 1024, cbPage ) );

    LGICompressPreImage( pfmp->Pinst(), cpage, cbPage, data, pbDehydrationBuffer, pbCompressionBuffer, &compressionPerformed );
    CHECK( compressionPerformed == (fPreimageDehydrated | fPreimageXpress) );
    CHECK( data.Pv() == pbCompressionBuffer );
    CHECK( data.Cb() < cbPage );

    CHECK( JET_errSuccess == ErrLGIDecompressPreimage( data, cbPage, pbDehydrationBuffer, pfmp->Ifmp(), cpage.PgnoThis(), fTrue, fTrue ) );
    CHECK( data.Pv() == pbDehydrationBuffer );
    CHECK( data.Cb() == cbPage );

    cpageT.LoadPage( data.Pv(), data.Cb() );
    CHECK( cpage.Clines() == cpageT.Clines() );
    cpage.GetPtrExternalHeader( &line );
    cpageT.GetPtrExternalHeader( &lineT );
    iline = 0;
    while ( fTrue )
    {
        CHECK( line.cb == lineT.cb );
        CHECK( memcmp( line.pv, lineT.pv, line.cb ) == 0 );
        CHECK( line.fFlags == lineT.fFlags );

        if ( iline >= cpage.Clines() )
            break;

        cpage.GetPtr( iline, &line );
        cpageT.GetPtr( iline, &lineT );
        iline++;
    }

    delete[] pbCompressionBuffer;
    delete[] pbDehydrationBuffer;
}

JETUNITTESTDB( PreImageCompression, Xpress, dwOpenDatabase )
{
    const ULONG cbPage = 32 * 1024;
    CPAGE cpage;
    FMP * pfmp = g_rgfmp + IfmpTest();
    DATA data, dataRec;
    BYTE rgbData[100];
    BYTE *pbDehydrationBuffer = NULL, *pbCompressionBuffer = NULL;
    ULONG compressionPerformed;

    cpage.LoadNewTestPage( cbPage, IfmpTest() );
    memset( rgbData, '0', sizeof(rgbData) );
    dataRec.SetPv( rgbData );
    dataRec.SetCb( sizeof(rgbData) );
    for ( INT i=0; i<310; i++ )
    {
        cpage.Insert( i, &dataRec, 1, 0 );
    }

    data.SetPv( cpage.PvBuffer() );
    data.SetCb( cbPage );
    pbDehydrationBuffer = new BYTE[cbPage];
    pbCompressionBuffer = new BYTE[cbPage];
    
    PKTermCompression();
    CHECK( JET_errSuccess == ErrPKInitCompression( cbPage, 1024, cbPage ) );

    LGICompressPreImage( pfmp->Pinst(), cpage, cbPage, data, pbDehydrationBuffer, pbCompressionBuffer, &compressionPerformed );
    CHECK( compressionPerformed == fPreimageXpress );
    CHECK( data.Pv() == pbCompressionBuffer );
    CHECK( data.Cb() < cbPage );

    CHECK( JET_errSuccess == ErrLGIDecompressPreimage( data, cbPage, pbDehydrationBuffer, pfmp->Ifmp(), cpage.PgnoThis(), fFalse, fTrue ) );
    CHECK( data.Pv() == pbDehydrationBuffer );
    CHECK( data.Cb() == cbPage );
    CHECK( memcmp( cpage.PvBuffer(), data.Pv(), cbPage ) == 0 );

    delete[] pbCompressionBuffer;
    delete[] pbDehydrationBuffer;
}

#endif


ERR ErrLGSplit( const FUCB          * const pfucb,
                _In_ SPLITPATH      * const psplitPathLeaf,
                const KEYDATAFLAGS& kdfOper,
                const RCEID         rceid1,
                const RCEID         rceid2,
                const DIRFLAG       dirflag,
                const DIRFLAG       dirflagPrevAssertOnly,
                LGPOS               * const plgpos,
                const VERPROXY      * const pverproxy )
{
    ERR         err = JET_errSuccess;
    BYTE *pbDataCompressed = NULL, *pbDataDehydrated = NULL;

    INST *pinst = PinstFromIfmp( pfucb->ifmp );
    LOG *plog = pinst->m_plog;
    PGNO *rgpgno = NULL;
    BOOL fAbortMacro = fFalse;

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );
    if ( plog->FLogDisabled() )
    {
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( g_rgfmp[pfucb->ifmp].FLogOn() );
    Assert( NULL == pverproxy ||
            prceNil != pverproxy->prcePrimary &&
            ( pverproxy->prcePrimary->TrxCommitted() != trxMax ||
              pfucbNil != pverproxy->prcePrimary->Pfucb() &&
              ppibNil != pverproxy->prcePrimary->Pfucb()->ppib ) );

    PIB * const ppib = pverproxy != NULL &&
                    trxMax == pverproxy->prcePrimary->TrxCommitted() ?
                        pverproxy->prcePrimary->Pfucb()->ppib :
                        pfucb->ppib;

    Assert( !FFMPIsTempDB( pfucb->ifmp ) );
    Assert( ppib->Level() > 0 );


    CallR( ErrLGDeferBeginTransaction( ppib ) );


    const SPLITPATH *psplitPath = psplitPathLeaf;


    CPG cpgno = 3;
    CPG ipgno = 0;
    
    for ( ; psplitPath->psplitPathParent != NULL; psplitPath = psplitPath->psplitPathParent )
    {
        cpgno += 3;
    }


    AllocR( rgpgno = new PGNO[ cpgno ] );

    DBTIME  dbtime = psplitPathLeaf->csr.Dbtime();
    Call( ErrLGIMacroBegin( ppib, dbtime ) );
    fAbortMacro = fTrue;


    for ( ; psplitPath != NULL; psplitPath = psplitPath->psplitPathChild )
    {
        DATA        rgdata[6];
        USHORT      idata = 0;
        LRSPLITNEW  lrsplit;

        lrsplit.lrtyp                   = lrtypSplit2;

        Assert( dbtime > psplitPath->dbtimeBefore );

        lrsplit.le_dbtime               = dbtime;
        Assert ( dbtimeNil != psplitPath->dbtimeBefore);
        lrsplit.le_dbtimeBefore         = psplitPath->dbtimeBefore;
        Assert( dbtime == psplitPath->csr.Dbtime() );
        Assert( psplitPath->csr.FDirty() );

        lrsplit.dbid                    = (BYTE) g_rgfmp[pfucb->ifmp].Dbid();
        lrsplit.le_procid               = ppib->procid;

        lrsplit.le_pgno             = psplitPath->csr.Pgno();
        lrsplit.le_pgnoParent       = psplitPath->psplitPathParent != NULL ? psplitPath->psplitPathParent->csr.Pgno() : pgnoNull;
        lrsplit.le_dbtimeParentBefore      = psplitPath->psplitPathParent != NULL ? psplitPath->psplitPathParent->dbtimeBefore : dbtimeNil;
        lrsplit.le_pgnoFDP          = PgnoFDP( pfucb );
        lrsplit.le_objidFDP         = ObjidFDP( pfucb );

        lrsplit.SetILine( 0 );

        Assert( !lrsplit.FVersioned() );
        Assert( !lrsplit.FDeleted() );
        Assert( !lrsplit.FUnique() );
        Assert( !lrsplit.FSpace() );
        Assert( !lrsplit.FConcCI() );
        Assert( !lrsplit.FSystemTask() );

        if ( pfucb->u.pfcb->FUnique() )
            lrsplit.SetFUnique();
        if ( FFUCBSpace( pfucb ) )
            lrsplit.SetFSpace();
        if ( NULL != pverproxy )
            lrsplit.SetFConcCI();

        LGISetTrx( ppib, &lrsplit, pverproxy );

        rgdata[idata].SetPv( (BYTE *) &lrsplit );
        rgdata[idata++].SetCb( sizeof (LRSPLITNEW) );

        if ( psplitPath->psplit == NULL )
        {
            lrsplit.le_pgnoNew      = pgnoNull;

            lrsplit.le_ilineOper    = USHORT( psplitPath->csr.ILine() );

            lrsplit.le_cbKeyParent      = 0;
            lrsplit.le_cbPrefixSplitOld = 0;
            lrsplit.le_cbPrefixSplitNew = 0;
            lrsplit.le_dbtimeRightBefore       = dbtimeNil;
            lrsplit.SetCbPageBeforeImage( 0 );
        }
        else
        {
            const SPLIT *psplit = psplitPath->psplit;
            Assert( psplit->csrNew.Dbtime() == lrsplit.le_dbtime );
            Assert( psplit->csrNew.FDirty() );

            Assert( pgnoNull == psplit->csrRight.Pgno()
                || dbtime > psplit->dbtimeRightBefore );

            lrsplit.le_dbtimeRightBefore    = psplit->dbtimeRightBefore;

            lrsplit.le_pgnoNew          = psplit->csrNew.Pgno();
            lrsplit.le_pgnoRight        = psplit->csrRight.Pgno();
            Assert( lrsplit.le_pgnoNew != pgnoNull );

            lrsplit.splittype           = BYTE( psplit->splittype );
            lrsplit.splitoper           = BYTE( psplit->splitoper );

            lrsplit.le_ilineOper        = USHORT( psplit->ilineOper );
            lrsplit.le_ilineSplit       = USHORT( psplit->ilineSplit );
            lrsplit.le_clines           = USHORT( psplit->clines );
            Assert( lrsplit.le_clines < g_cbPage );

            lrsplit.le_fNewPageFlags    = psplit->fNewPageFlags;
            lrsplit.le_fSplitPageFlags  = psplit->fSplitPageFlags;

            lrsplit.le_cbUncFreeSrc     = psplit->cbUncFreeSrc;
            lrsplit.le_cbUncFreeDest    = psplit->cbUncFreeDest;

            lrsplit.le_ilinePrefixSplit = psplit->prefixinfoSplit.ilinePrefix;
            lrsplit.le_ilinePrefixNew   = psplit->prefixinfoNew.ilinePrefix;

            lrsplit.le_cbKeyParent      = (USHORT) psplit->kdfParent.key.Cb();
            rgdata[idata].SetPv( psplit->kdfParent.key.prefix.Pv() );
            rgdata[idata++].SetCb( psplit->kdfParent.key.prefix.Cb() );

            rgdata[idata].SetPv( psplit->kdfParent.key.suffix.Pv() );
            rgdata[idata++].SetCb( psplit->kdfParent.key.suffix.Cb() );

            lrsplit.le_cbPrefixSplitOld = USHORT( psplit->prefixSplitOld.Cb() );
            rgdata[idata].SetPv( psplit->prefixSplitOld.Pv() );
            rgdata[idata++].SetCb( lrsplit.le_cbPrefixSplitOld );

            lrsplit.le_cbPrefixSplitNew = USHORT( psplit->prefixSplitNew.Cb() );
            rgdata[idata].SetPv( psplit->prefixSplitNew.Pv() );
            rgdata[idata++].SetCb( lrsplit.le_cbPrefixSplitNew );

            if( FBTISplitPageBeforeImageRequired( psplit ) )
            {
                DATA dataToSet;
                dataToSet.SetPv( psplitPath->csr.Cpage().PvBuffer() );
                dataToSet.SetCb( g_cbPage );
                if ( ( pbDataCompressed != NULL ||
                       ( pbDataCompressed = PbPKAllocCompressionBuffer() ) != NULL ) &&
                     ( pbDataDehydrated != NULL ||
                       ( pbDataDehydrated = PbPKAllocCompressionBuffer() ) != NULL ) )
                {
                    Assert( CbPKCompressionBuffer() == g_cbPage );

                    ULONG compressionPerformed = 0;
                    LGICompressPreImage( pinst, psplitPath->csr.Cpage(), g_cbPage, dataToSet, pbDataDehydrated, pbDataCompressed, &compressionPerformed );
                    if ( compressionPerformed & fPreimageDehydrated )
                    {
                        lrsplit.SetPreimageDehydrated();
                    }
                    if ( compressionPerformed & fPreimageXpress )
                    {
                        lrsplit.SetPreimageXpress();
                    }
                }
                lrsplit.SetCbPageBeforeImage( dataToSet.Cb() );
                rgdata[idata].SetPv( dataToSet.Pv() );
                rgdata[idata++].SetCb( dataToSet.Cb() );
            }
            else
            {
                lrsplit.SetCbPageBeforeImage( 0 );
            }
        }

        Call( plog->ErrLGLogRec( rgdata, idata, fLGMacroGoing, ppib->lgposStart.lGeneration, plgpos ) );


        Assert( ipgno + 3 <= cpgno );

        if ( lrsplit.le_pgno != pgnoNull )
            rgpgno[ ipgno++ ] = lrsplit.le_pgno;
        if ( lrsplit.le_pgnoNew != pgnoNull )
            rgpgno[ ipgno++ ] = lrsplit.le_pgnoNew;
        if ( lrsplit.le_pgnoRight != pgnoNull )
            rgpgno[ ipgno++ ] = lrsplit.le_pgnoRight;

            
    }

    AssertTrack( ( dirflagPrevAssertOnly == dirflag ) ||
                 ( ( dirflagPrevAssertOnly & fDIRFlagInsertAndReplaceData ) &&
                   ( dirflag & fDIRInsert ) ),
                 OSFormat( "LgSplitUnexpectedDirFlagChange:%d:%d", (int)dirflagPrevAssertOnly, (int)dirflag ) );
    
    
    if ( psplitPathLeaf->psplit != NULL &&
         psplitPathLeaf->psplit->splitoper != splitoperNone )
    {
        const SPLIT *psplit = psplitPathLeaf->psplit;
        Assert( psplitPathLeaf->csr.Cpage().FLeafPage() );

        switch ( psplit->splitoper )
        {
            case splitoperInsert:
                AssertTrack( ( ( rceidNull == rceid2 ) && ( dirflagPrevAssertOnly == dirflag ) && ( dirflag & fDIRInsert ) ) ||
                             ( ( rceidNull != rceid2 ) && ( dirflagPrevAssertOnly != dirflag ) && ( dirflag & fDIRInsert ) && ( dirflagPrevAssertOnly & fDIRFlagInsertAndReplaceData ) ),
                             OSFormat( "LgSplitInconsistentOperDirFlag:%d:%d", (int)dirflagPrevAssertOnly, (int)dirflag ) );
                Call( ErrLGInsert( pfucb,
                                   &psplitPathLeaf->csr,
                                   kdfOper,
                                   rceid1,
                                   dirflag,
                                   plgpos,
                                   pverproxy,
                                   fDontDirtyCSR ) );
                break;

            case splitoperFlagInsertAndReplaceData:
                Call( ErrLGFlagInsertAndReplaceData( pfucb,
                                                     &psplitPathLeaf->csr,
                                                     kdfOper,
                                                     rceid1,
                                                     rceid2,
                                                     dirflag,
                                                     plgpos,
                                                     pverproxy,
                                                     fDontDirtyCSR ) );
                break;

            case splitoperReplace:
                AssertTrack( rceidNull == rceid2, "OpReplaceNonNullRce2" );
                Assert( NULL == pverproxy );
                Call( ErrLGReplace( pfucb,
                                    &psplitPathLeaf->csr,
                                    psplit->rglineinfo[psplit->ilineOper].kdf.data,
                                    kdfOper.data,
                                    NULL,
                                    rceid1,
                                    dirflag,
                                    plgpos,
                                    fDontDirtyCSR ) );
                break;

            default:
                Assert( fFalse );
        }
    }

    Call( ErrFaultInjection( 38586 ) );

    Assert( ipgno <= cpgno );
    Call( ErrLGIMacroEnd( ppib, dbtime, lrtypMacroCommit, pfucb->ifmp, rgpgno, ipgno, plgpos ) );
    fAbortMacro = fFalse;

HandleError:
    delete[] rgpgno;
    PKFreeCompressionBuffer( pbDataCompressed );
    PKFreeCompressionBuffer( pbDataDehydrated );

    if ( fAbortMacro )
    {
        Assert( err < JET_errSuccess );
        (void)ErrLGMacroAbort( ppib, dbtime, plgpos );
    }

    return err;
}


ERR ErrLGMerge( const FUCB *pfucb, MERGEPATH *pmergePathLeaf, LGPOS *plgpos )
{
    ERR     err = JET_errSuccess;
    BYTE *pbDataCompressed = NULL, *pbDataDehydrated = NULL;

    INST *pinst = PinstFromIfmp( pfucb->ifmp );
    LOG *plog = pinst->m_plog;
    PGNO *rgpgno = NULL;
    BOOL fAbortMacro = fFalse;

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );
    Assert( !g_rgfmp[pfucb->ifmp].FLogOn() || !plog->FLogDisabled() );

    if ( !g_rgfmp[pfucb->ifmp].FLogOn() )
    {
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( pfucb->ppib->Level() == 1 );
    Assert( !FFMPIsTempDB( pfucb->ifmp ) );


    CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );

    const MERGEPATH *pmergePath = pmergePathLeaf;

    Assert( pmergePathLeaf->pmerge != NULL );



    CPG cpgno = 3;
    CPG ipgno = 0;
    
    for ( ; pmergePath->pmergePathParent != NULL && latchWrite == pmergePath->pmergePathParent->csr.Latch();
            pmergePath = pmergePath->pmergePathParent )
    {
        cpgno += 3;
    }


    AllocR( rgpgno = new PGNO[ cpgno ] );

    const DBTIME    dbtime = pmergePathLeaf->csr.Dbtime();
    Call( ErrLGIMacroBegin( pfucb->ppib, dbtime ) );
    fAbortMacro = fTrue;


    for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathChild )
    {
        Assert( latchWrite == pmergePath->csr.Latch() );

        const MERGE *pmerge = pmergePath->pmerge;
        DATA        rgdata[3];
        USHORT      idata = 0;
        LRMERGENEW  lrmerge;

        lrmerge.lrtyp           = lrtypMerge2;
        lrmerge.le_dbtime       = dbtime;
        lrmerge.le_dbtimeBefore = pmergePath->dbtimeBefore;
        Assert( pmergePath->csr.FDirty() );

        lrmerge.le_procid       = pfucb->ppib->procid;
        lrmerge.dbid            = (BYTE) g_rgfmp[ pfucb->ifmp ].Dbid();

        lrmerge.le_pgno         = pmergePath->csr.Pgno();
        lrmerge.SetILine( pmergePath->iLine );

        Assert( !lrmerge.FVersioned() );
        Assert( !lrmerge.FDeleted() );
        Assert( !lrmerge.FUnique() );
        Assert( !lrmerge.FSpace() );
        Assert( !lrmerge.FConcCI() );
        Assert( !lrmerge.FSystemTask() );
        Assert( !lrmerge.FKeyChange() );
        Assert( !lrmerge.FDeleteNode() );
        Assert( !lrmerge.FEmptyPage() );

        if ( pfucb->u.pfcb->FUnique() )
            lrmerge.SetFUnique();
        if ( FFUCBSpace( pfucb ) )
            lrmerge.SetFSpace();
        if ( pmergePath->fKeyChange )
            lrmerge.SetFKeyChange();
        if ( pmergePath->fDeleteNode )
            lrmerge.SetFDeleteNode();
        if ( pmergePath->fEmptyPage )
            lrmerge.SetFEmptyPage();

        LGISetTrx( pfucb->ppib, &lrmerge );

        lrmerge.le_pgnoParent   = pmergePath->pmergePathParent != NULL ? pmergePath->pmergePathParent->csr.Pgno() : pgnoNull;
        lrmerge.le_dbtimeParentBefore = pmergePath->pmergePathParent != NULL ? pmergePath->pmergePathParent->dbtimeBefore : dbtimeNil;

        lrmerge.le_pgnoFDP      = PgnoFDP( pfucb );
        lrmerge.le_objidFDP     = ObjidFDP( pfucb );

        rgdata[idata].SetPv( (BYTE *) &lrmerge );
        rgdata[idata++].SetCb( sizeof (LRMERGENEW) );

        lrmerge.SetCbPageBeforeImage( 0 );
        if ( pmerge != NULL )
        {
            Assert( pgnoNull == pmerge->csrLeft.Pgno() ||
                    pmerge->csrLeft.FDirty() );
            Assert( pgnoNull == pmerge->csrRight.Pgno() ||
                    pmerge->csrRight.FDirty() );
            Assert( 0 == pmerge->ilineMerge
                    || mergetypeFullLeft == pmerge->mergetype
                    || mergetypePartialRight == pmerge->mergetype
                    || mergetypePartialLeft == pmerge->mergetype );

            lrmerge.SetILineMerge( USHORT( pmerge->ilineMerge ) );

            lrmerge.le_pgnoLeft     = pmerge->csrLeft.Pgno();
            lrmerge.le_pgnoRight    = pmerge->csrRight.Pgno();

            lrmerge.le_dbtimeRightBefore = pmerge->dbtimeRightBefore;
            lrmerge.le_dbtimeLeftBefore = pmerge->dbtimeLeftBefore;

            Assert( mergetypeNone != pmerge->mergetype );
            Assert( mergetypePageMove != pmerge->mergetype );
            lrmerge.mergetype           = BYTE( pmerge->mergetype );
            lrmerge.le_cbSizeTotal      = USHORT( pmerge->cbSizeTotal );
            lrmerge.le_cbSizeMaxTotal   = USHORT( pmerge->cbSizeMaxTotal );
            lrmerge.le_cbUncFreeDest    = USHORT( pmerge->cbUncFreeDest );

            lrmerge.le_cbKeyParentSep   = (USHORT) pmerge->kdfParentSep.key.suffix.Cb();
            Assert( pmerge->kdfParentSep.key.prefix.FNull() );

            rgdata[idata].SetPv( pmerge->kdfParentSep.key.suffix.Pv() );
            rgdata[idata++].SetCb( pmerge->kdfParentSep.key.suffix.Cb() );

            if( FBTIMergePageBeforeImageRequired( pmerge ) )
            {
                DATA dataToSet;
                dataToSet.SetPv( pmergePath->csr.Cpage().PvBuffer() );
                dataToSet.SetCb( g_cbPage );
                if ( ( pbDataCompressed != NULL ||
                       ( pbDataCompressed = PbPKAllocCompressionBuffer() ) != NULL ) &&
                     ( pbDataDehydrated != NULL ||
                       ( pbDataDehydrated = PbPKAllocCompressionBuffer() ) != NULL ) )
                {
                    Assert( CbPKCompressionBuffer() == g_cbPage );

                    ULONG compressionPerformed = 0;
                    LGICompressPreImage( pinst, pmergePath->csr.Cpage(), g_cbPage, dataToSet, pbDataDehydrated, pbDataCompressed, &compressionPerformed );
                    if ( compressionPerformed & fPreimageDehydrated )
                    {
                        lrmerge.SetPreimageDehydrated();
                    }
                    if ( compressionPerformed & fPreimageXpress )
                    {
                        lrmerge.SetPreimageXpress();
                    }
                }
                lrmerge.SetCbPageBeforeImage( dataToSet.Cb() );
                rgdata[idata].SetPv( dataToSet.Pv() );
                rgdata[idata++].SetCb( dataToSet.Cb() );
            }
            else
            {
                Assert( lrmerge.CbPageBeforeImage() == 0 );
            }
        }
        else
        {
            lrmerge.le_dbtimeRightBefore = dbtimeNil;
            lrmerge.le_dbtimeLeftBefore = dbtimeNil;
            Assert( lrmerge.CbPageBeforeImage() == 0 );
        }

        Call( plog->ErrLGLogRec( rgdata, idata, fLGMacroGoing, pfucb->ppib->lgposStart.lGeneration, plgpos ) );


        Assert( ipgno + 3 <= cpgno );

        if ( lrmerge.le_pgno != pgnoNull )
            rgpgno[ ipgno++ ] = lrmerge.le_pgno;
        if ( lrmerge.le_pgnoLeft != pgnoNull )
            rgpgno[ ipgno++ ] = lrmerge.le_pgnoLeft;
        if ( lrmerge.le_pgnoRight != pgnoNull )
            rgpgno[ ipgno++ ] = lrmerge.le_pgnoRight;
    
    }

    Assert( ipgno <= cpgno );

    Call( ErrFaultInjection( 38586 ) );

    Call( ErrLGIMacroEnd( pfucb->ppib, dbtime, lrtypMacroCommit, pfucb->ifmp, rgpgno, ipgno, plgpos ) );
    fAbortMacro = fFalse;

HandleError:
    delete[] rgpgno;
    PKFreeCompressionBuffer( pbDataCompressed );
    PKFreeCompressionBuffer( pbDataDehydrated );

    if ( fAbortMacro )
    {
        Assert( err < JET_errSuccess );
        (void)ErrLGMacroAbort( pfucb->ppib, dbtime, plgpos );
    }

    return err;
}


ERR ErrLGEmptyTree(
    FUCB            * const pfucb,
    CSR             * const pcsrRoot,
    EMPTYPAGE       * const rgemptypage,
    const CPG       cpgToFree,
    LGPOS           * const plgpos )
{
    ERR             err;
    INST            * const pinst   = PinstFromIfmp( pfucb->ifmp );
    LOG             * const plog    = pinst->m_plog;
    DATA            rgdata[2];
    LREMPTYTREE     lremptytree;

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );
    Assert( pcsrRoot->Pgno() == PgnoRoot( pfucb ) );

    if ( plog->FLogDisabled()
        || !g_rgfmp[pfucb->ifmp].FLogOn() )
    {
        pcsrRoot->Dirty();
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( pfucb->ppib->Level() > 0 );
    CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );

    lremptytree.lrtyp               = lrtypEmptyTree;
    lremptytree.le_procid           = pfucb->ppib->procid;
    lremptytree.dbid                = g_rgfmp[ pfucb->ifmp ].Dbid();
    lremptytree.le_pgno             = pcsrRoot->Pgno();
    lremptytree.SetILine( 0 );

    Assert( !lremptytree.FVersioned() );
    Assert( !lremptytree.FDeleted() );
    Assert( !lremptytree.FUnique() );
    Assert( !lremptytree.FSpace() );
    Assert( !lremptytree.FConcCI() );
    Assert( !lremptytree.FSystemTask() );
    if ( pfucb->u.pfcb->FUnique() )
        lremptytree.SetFUnique();
    if ( FFUCBSpace( pfucb ) )
        lremptytree.SetFSpace();

    ULONG fFlagsBefore = 0;
    CallR( ErrLGSetDbtimeBeforeAndDirty( pinst, pcsrRoot, &lremptytree.le_dbtimeBefore, &lremptytree.le_dbtime, &fFlagsBefore, fTrue, fFalse ) );
    LGISetTrx( pfucb->ppib, &lremptytree );

    Assert( cpgToFree > 0 );
    Assert( cpgToFree <= cBTMaxDepth );
    const USHORT    cbEmptyPageList     = USHORT( sizeof(EMPTYPAGE) * cpgToFree );

    lremptytree.le_rceid = rceidNull;
    lremptytree.le_pgnoFDP = PgnoFDP( pfucb );
    lremptytree.le_objidFDP = ObjidFDP( pfucb );

    lremptytree.SetCbEmptyPageList( cbEmptyPageList );

    rgdata[0].SetPv( (BYTE *)&lremptytree );
    rgdata[0].SetCb( sizeof(lremptytree) );

    rgdata[1].SetPv( (BYTE *)rgemptypage );
    rgdata[1].SetCb( cbEmptyPageList );

    err = plog->ErrLGLogRec( rgdata, 2, 0, pfucb->ppib->lgposStart.lGeneration, plgpos );

    if ( err < 0 )
    {
        Assert( dbtimeInvalid !=  lremptytree.le_dbtimeBefore );
        Assert( dbtimeNil != lremptytree.le_dbtimeBefore );
        Assert( pcsrRoot->Dbtime() > lremptytree.le_dbtimeBefore );
        pcsrRoot->RevertDbtime( lremptytree.le_dbtimeBefore, fFlagsBefore );
    }

    return err;
}




ERR ErrLGCreateMultipleExtentFDP(
    const FUCB          *pfucb,
    const CSR           *pcsr,
    const SPACE_HEADER  *psph,
    const ULONG         fPageFlags,
    LGPOS               *plgpos )
{
    ERR                 err;
    DATA                rgdata[1];
    LRCREATEMEFDP       lrcreatemefdp;

    INST *pinst = PinstFromIfmp( pfucb->ifmp );
    LOG *plog = pinst->m_plog;

    Assert( pcsr->FDirty() );
    Assert( !g_rgfmp[pfucb->ifmp].FLogOn() || !plog->FLogDisabled() );

    if ( !g_rgfmp[pfucb->ifmp].FLogOn() )
    {
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( !plog->FRecovering() );
    Assert( 0 < pfucb->ppib->Level() );

    CallR( plog->ErrLGCheckState() );


    CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );

    Assert( psph->FMultipleExtent() );

    lrcreatemefdp.lrtyp             = lrtypCreateMultipleExtentFDP;
    lrcreatemefdp.le_procid             = pfucb->ppib->procid;
    lrcreatemefdp.le_pgno               = PgnoFDP( pfucb );
    lrcreatemefdp.le_objidFDP           = ObjidFDP( pfucb );
    lrcreatemefdp.le_pgnoFDPParent  = psph->PgnoParent();
    lrcreatemefdp.le_pgnoOE         = PgnoOE( pfucb );
    lrcreatemefdp.le_pgnoAE         = PgnoAE( pfucb );
    lrcreatemefdp.le_fPageFlags     = fPageFlags;

    lrcreatemefdp.dbid              = g_rgfmp[ pfucb->ifmp ].Dbid();
    lrcreatemefdp.le_dbtime         = pcsr->Dbtime();
    lrcreatemefdp.le_dbtimeBefore = dbtimeNil;
    lrcreatemefdp.le_cpgPrimary     = psph->CpgPrimary();

    Assert( !lrcreatemefdp.FVersioned() );
    Assert( !lrcreatemefdp.FDeleted() );
    Assert( !lrcreatemefdp.FUnique() );
    Assert( !lrcreatemefdp.FSpace() );
    Assert( !lrcreatemefdp.FConcCI() );
    Assert( !lrcreatemefdp.FSystemTask() );

    if ( psph->FUnique() )
        lrcreatemefdp.SetFUnique();

    LGISetTrx( pfucb->ppib, &lrcreatemefdp );

    rgdata[0].SetPv( (BYTE *)&lrcreatemefdp );
    rgdata[0].SetCb( sizeof(lrcreatemefdp) );

    err = plog->ErrLGLogRec( rgdata, 1, 0, pfucb->ppib->lgposStart.lGeneration, plgpos );

    return err;
}


ERR ErrLGCreateSingleExtentFDP(
    const FUCB          *pfucb,
    const CSR           *pcsr,
    const SPACE_HEADER  *psph,
    const ULONG         fPageFlags,
    LGPOS               *plgpos )
{
    ERR                 err;
    DATA                rgdata[1];
    LRCREATESEFDP       lrcreatesefdp;

    INST *pinst = PinstFromIfmp( pfucb->ifmp );
    LOG *plog = pinst->m_plog;

    Assert( !plog->FRecovering() );
    Assert( pcsr->FDirty() );
    Assert( !g_rgfmp[pfucb->ifmp].FLogOn() || !plog->FLogDisabled() );
    if ( !g_rgfmp[pfucb->ifmp].FLogOn() )
    {
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( 0 < pfucb->ppib->Level() );

    CallR( plog->ErrLGCheckState() );


    CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );

    Assert( psph->FSingleExtent() );

    lrcreatesefdp.lrtyp             = lrtypCreateSingleExtentFDP;
    lrcreatesefdp.le_procid             = pfucb->ppib->procid;
    lrcreatesefdp.le_pgnoFDPParent  = psph->PgnoParent();

    Assert( pcsr->Pgno() == PgnoFDP( pfucb ) );
    lrcreatesefdp.le_pgno               = pcsr->Pgno();
    lrcreatesefdp.le_objidFDP           = ObjidFDP( pfucb );
    lrcreatesefdp.le_fPageFlags     = fPageFlags;

    lrcreatesefdp.dbid              = g_rgfmp[ pfucb->ifmp ].Dbid();
    lrcreatesefdp.le_dbtime         = pcsr->Dbtime();
    lrcreatesefdp.le_dbtimeBefore = dbtimeNil;
    lrcreatesefdp.le_cpgPrimary     = psph->CpgPrimary();

    Assert( !lrcreatesefdp.FVersioned() );
    Assert( !lrcreatesefdp.FDeleted() );
    Assert( !lrcreatesefdp.FUnique() );
    Assert( !lrcreatesefdp.FSpace() );
    Assert( !lrcreatesefdp.FConcCI() );
    Assert( !lrcreatesefdp.FSystemTask() );

    if ( psph->FUnique() )
        lrcreatesefdp.SetFUnique();

    LGISetTrx( pfucb->ppib, &lrcreatesefdp );

    rgdata[0].SetPv( (BYTE *)&lrcreatesefdp );
    rgdata[0].SetCb( sizeof(lrcreatesefdp) );

    err = plog->ErrLGLogRec( rgdata, 1, 0, pfucb->ppib->lgposStart.lGeneration, plgpos );

    return err;
}


ERR ErrLGConvertFDP(
    const FUCB          *pfucb,
    const CSR           *pcsr,
    const SPACE_HEADER  *psph,
    const PGNO          pgnoSecondaryFirst,
    const CPG           cpgSecondary,
    const DBTIME        dbtimeBefore,
    const UINT          rgbitAvail,
    const ULONG         fCpageFlags,
    LGPOS               *plgpos )
{
    ERR                 err;
    DATA                rgdata[1];
    LRCONVERTFDP        lrconvertfdp;

    INST *pinst = PinstFromIfmp( pfucb->ifmp );
    LOG *plog = pinst->m_plog;

    Assert( !plog->FRecovering() );
    Assert( pcsr->FDirty() );
    Assert( !FFUCBSpace( pfucb ) );
    Assert( psph->PgnoOE() == pgnoSecondaryFirst );
    Assert( !g_rgfmp[pfucb->ifmp].FLogOn() || !plog->FLogDisabled() );
    if ( !g_rgfmp[pfucb->ifmp].FLogOn() )
    {
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( 0 < pfucb->ppib->Level() );

    CallR( plog->ErrLGCheckState() );


    CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );

    lrconvertfdp.lrtyp                  = lrtypConvertFDP2;
    lrconvertfdp.le_procid              = pfucb->ppib->procid;
    lrconvertfdp.le_pgnoFDPParent       = psph->PgnoParent();
    lrconvertfdp.le_pgno                = PgnoFDP( pfucb );
    lrconvertfdp.le_objidFDP            = ObjidFDP( pfucb );
    lrconvertfdp.le_pgnoOE              = pgnoSecondaryFirst;
    lrconvertfdp.le_pgnoAE              = pgnoSecondaryFirst + 1;

    lrconvertfdp.dbid                   = g_rgfmp[ pfucb->ifmp ].Dbid();
    lrconvertfdp.le_dbtime              = pcsr->Dbtime();
    lrconvertfdp.le_dbtimeBefore        = dbtimeBefore;
    lrconvertfdp.le_cpgPrimary          = psph->CpgPrimary();
    lrconvertfdp.le_cpgSecondary        = cpgSecondary;
    lrconvertfdp.le_pgnoSecondaryFirst  = pgnoSecondaryFirst;

    lrconvertfdp.le_rgbitAvail          = rgbitAvail;
    lrconvertfdp.le_fCpageFlags         = fCpageFlags;

    Assert( !lrconvertfdp.FVersioned() );
    Assert( !lrconvertfdp.FDeleted() );
    Assert( !lrconvertfdp.FUnique() );
    Assert( !lrconvertfdp.FSpace() );
    Assert( !lrconvertfdp.FConcCI() );
    Assert( !lrconvertfdp.FSystemTask() );

    if ( psph->FUnique() )
        lrconvertfdp.SetFUnique();

    LGISetTrx( pfucb->ppib, &lrconvertfdp );

    rgdata[0].SetPv( (BYTE *)&lrconvertfdp );
    rgdata[0].SetCb( sizeof(lrconvertfdp) );

    err = plog->ErrLGLogRec( rgdata, 1, 0, pfucb->ppib->lgposStart.lGeneration, plgpos );

    return err;
}

ERR ErrLGFreeFDP( const FCB *pfcb, const TRX trxCommitted )
{
    IFMP ifmp = pfcb->Ifmp();
    INST *pinst = PinstFromIfmp( ifmp );
    LOG *plog = pinst->m_plog;
    LRFREEFDP lrfreefdp;
    DATA data;
    LGPOS lgpos;

    if ( !g_rgfmp[ifmp].FLogOn() || plog->FLogDisabled() || plog->FRecovering() )
        return JET_errSuccess;

    if ( !pfcb->FTypeTable() && !pfcb->FTypeSecondaryIndex() )
        return JET_errSuccess;

    lrfreefdp.le_dbid = g_rgfmp[ ifmp ].Dbid();
    lrfreefdp.le_pgnoFDP = pfcb->PgnoFDP();
    lrfreefdp.le_trxCommitted = trxCommitted;
    lrfreefdp.bFDPType = (BYTE)(pfcb->FTypeTable() ? fFDPTypeTable : fFDPTypeIndex);

    data.SetPv( (BYTE*)&lrfreefdp );
    data.SetCb( sizeof(lrfreefdp) );

    return plog->ErrLGLogRec( &data, 1, 0, 0, &lgpos );
}

ERR ErrLGStart( INST *pinst )
{
    ERR     err;
    DATA    rgdata[1];
    LRINIT2 lr;
    LOG     *plog = pinst->m_plog;
    
    if ( plog->FLogDisabled() || ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo ) )
        return JET_errSuccess;

    CallR( plog->ErrLGCheckState() );

    lr.lrtyp = lrtypInit2;
    LGIGetDateTime( &lr.logtime );
    pinst->SaveDBMSParams( &lr.dbms_param );

    rgdata[0].SetPv( (BYTE *)&lr );
    rgdata[0].SetCb( sizeof(lr) );

    err = plog->ErrLGLogRec( rgdata, 1, 0, 0, pNil );

    plog->LGSetLgposStart();

    return err;
}


ERR ErrLGIMacroBegin( PIB *ppib, DBTIME dbtime )
{
    ERR             err;
    DATA            rgdata[1];
    LRMACROBEGIN    lrMacroBegin;

    INST *pinst = PinstFromPpib( ppib );
    LOG *plog = pinst->m_plog;

    Assert( !plog->FRecovering() || fRecoveringUndo == plog->FRecoveringMode() );

    if ( plog->FLogDisabled() )
        return JET_errSuccess;

    lrMacroBegin.lrtyp  = lrtypMacroBegin;
    lrMacroBegin.le_procid = ppib->procid;
    lrMacroBegin.le_dbtime = dbtime;

    rgdata[0].SetPv( (BYTE *)&lrMacroBegin );
    rgdata[0].SetCb( sizeof(lrMacroBegin) );

    err = plog->ErrLGLogRec( rgdata, 1, fLGMacroGoing, ppib->lgposStart.lGeneration, pNil );

    return err;
}


ERR ErrLGIMacroEnd( PIB *ppib, DBTIME dbtime, LRTYP lrtyp, const IFMP ifmp, const PGNO * const rgpgno, CPG cpgno, LGPOS *plgpos )
{
    ERR         err;
    DATA        rgdata[5];
    ULONG       cdata = 0;
    LRMACROEND  lrMacroEnd;
    LRMACROINFO lrMacroInfo;
    LRMACROINFO2 lrMacroInfo2;

    INST *pinst = PinstFromPpib( ppib );
    LOG *plog = pinst->m_plog;

    Assert( !plog->FRecovering() || plog->FRecoveringMode() == fRecoveringUndo );

    if ( plog->FLogDisabled() )
    {
        *plgpos = lgposMin;
        return JET_errSuccess;
    }

    Assert( lrtyp == lrtypMacroCommit || lrtyp == lrtypMacroAbort );
    lrMacroEnd.lrtyp        = lrtyp;
    lrMacroEnd.le_procid    = ppib->procid;
    lrMacroEnd.le_dbtime    = dbtime;

    rgdata[cdata].SetPv( (BYTE *)&lrMacroEnd );
    rgdata[cdata].SetCb( sizeof(lrMacroEnd) );
    cdata++;

    if ( cpgno > 0 )
    {
        Assert( rgpgno );

        if ( g_rgfmp[ifmp].Dbid() == 1 )
        {
            lrMacroInfo.SetCountOfPgno( cpgno );

            rgdata[cdata].SetPv( (BYTE *)&lrMacroInfo );
            rgdata[cdata].SetCb( sizeof( lrMacroInfo ) );
            cdata++;

            rgdata[cdata].SetPv( (BYTE *)rgpgno );
            rgdata[cdata].SetCb( cpgno * sizeof( PGNO ) );
            cdata++;
        }

        else
        {
            lrMacroInfo2.SetDbid( g_rgfmp[ifmp].Dbid() );
            lrMacroInfo2.SetCountOfPgno( cpgno );

            rgdata[cdata].SetPv( (BYTE *)&lrMacroInfo2 );
            rgdata[cdata].SetCb( sizeof( lrMacroInfo2 ) );
            cdata++;

            rgdata[cdata].SetPv( (BYTE *)rgpgno );
            rgdata[cdata].SetCb( cpgno * sizeof( PGNO ) );
            cdata++;
        }
    }
    




    

    Assert( cdata <= _countof( rgdata ) );
    err = plog->ErrLGLogRec( rgdata,
                             cdata,
                             fLGMacroGoing,
                             ppib->lgposStart.lGeneration,
                             plgpos );

    return err;
}


ERR ErrLGMacroAbort( PIB *ppib, DBTIME dbtime, LGPOS *plgpos )
{

    
    return ErrLGIMacroEnd( ppib, dbtime, lrtypMacroAbort, ifmpNil, NULL, 0, plgpos );
}

ERR ErrLGShutDownMark( INST *pinst, LGPOS *plgposRec )
{
    ERR             err;
    DATA            rgdata[1];
    LRSHUTDOWNMARK  lr;
    LOG *           plog = pinst->m_plog;

    if ( plog->FLogDisabled() || ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo ) )
    {
        *plgposRec = lgposMin;
        return JET_errSuccess;
    }

    lr.lrtyp = lrtypShutDownMark;

    rgdata[0].SetPv( (BYTE *)&lr );
    rgdata[0].SetCb( sizeof(lr) );

    CallR( plog->ErrLGLogRec( rgdata, 1, 0, 0, plgposRec ) );

    PIB pibFake;
    pibFake.m_pinst = pinst;
    err = ErrLGWaitForWrite( &pibFake, plgposRec );
    return err;
}


ERR ErrLGRecoveryUndo( LOG *plog, BOOL fAggressiveRollover )
{
    ERR             err;
    DATA            rgdata[1];
    LRRECOVERYUNDO2 lr;

    Assert( plog->FRecovering() );
    Assert( fRecoveringUndo == plog->FRecoveringMode() );

    if ( plog->FLogDisabled() || ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo ) )
    {
        return JET_errSuccess;
    }

    lr.lrtyp = lrtypRecoveryUndo2;
    LGIGetDateTime( &lr.logtime );

    rgdata[0].SetPv( (BYTE *)&lr );
    rgdata[0].SetCb( sizeof(lr) );

    err = plog->ErrLGLogRec(    rgdata,
                                1,
                                fAggressiveRollover ? fLGCreateNewGen : 0 ,
                                0,
                                pNil );

    return err;
}


ERR ErrLGQuitRec( LOG *plog, LRTYP lrtyp, const LE_LGPOS *ple_lgpos, const LE_LGPOS *ple_lgposRedoFrom, BOOL fHard, BOOL fAggressiveRollover, LGPOS * plgposQuit )
{
    ERR         err;
    DATA        rgdata[1];
    LRTERMREC2  lr;

    if ( plog->FLogDisabled() || ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo ) )
        return JET_errSuccess;

    lr.lrtyp = lrtyp;
    LGIGetDateTime( &lr.logtime );
    lr.lgpos = *ple_lgpos;
    if ( ple_lgposRedoFrom )
    {
        Assert( lrtyp == lrtypRecoveryQuit2 || lrtyp == lrtypRecoveryQuitKeepAttachments );
        lr.lgposRedoFrom = *ple_lgposRedoFrom;
        lr.fHard = BYTE( fHard );
    }
    rgdata[0].SetPv( (BYTE *)&lr );
    rgdata[0].SetCb( sizeof(lr) );

    err = plog->ErrLGLogRec(
                        rgdata,
                        1,
                        fAggressiveRollover ? fLGCreateNewGen : 0,
                        0,
                        plgposQuit );
    return err;
}


ERR ErrLGLogBackup(
    LOG *                               plog,
    LRLOGBACKUP::LRLOGBACKUP_PHASE      ePhase,
    DBFILEHDR::BKINFOTYPE               eType,
    LRLOGBACKUP::LRLOGBACKUP_SCOPE      eScope,
    const INT                           lgenLow,
    const INT                           lgenHigh,
    const LGPOS *                       plgpos,
    const LOGTIME *                     plogtime,
    const DBID                          dbid,
    const BOOL                          fLGFlags,
    LGPOS *                             plgposLogRec )
{
    DATA            rgdata[2];
    LRLOGBACKUP lr;
    LOGTIME         logtimeStamp;
    LGPOS           lgposLocal;

    if ( plgposLogRec == NULL )
    {
        plgposLogRec = &lgposLocal;
    }

    if ( plog->FLogDisabled() || ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo ) )
    {
        AssertSz( fFalse, "Would be surprised to see this fire" );
        *plgposLogRec = lgposMin;
        return JET_errSuccess;
    }

    lr.lrtyp = lrtypBackup2;

    Assert( lgenLow == 0 || lgenHigh == 0 || lgenLow <= lgenHigh );

    lr.eBackupPhase = BYTE( ePhase );

    lr.eBackupType = eType;
    lr.eBackupScope = BYTE( eScope );

    lr.phaseDetails.le_genLow = lgenLow;
    lr.phaseDetails.le_genHigh = lgenHigh;
    Assert( plgpos || ePhase != LRLOGBACKUP::fLGBackupPhaseUpdate );
    if ( plgpos )
    {
        Assert( plogtime );
        lr.phaseDetails.le_lgposMark = *plgpos;
    }
    if ( plogtime == NULL )
    {
        Assert( ePhase == LRLOGBACKUP::fLGBackupPhaseBegin ||
                ePhase == LRLOGBACKUP::fLGBackupPhaseTruncate ||
                ePhase == LRLOGBACKUP::fLGBackupPhaseAbort ||
                ePhase == LRLOGBACKUP::fLGBackupPhaseStop );
        LGIGetDateTime( &logtimeStamp );
        plogtime = &logtimeStamp;
    }
    lr.phaseDetails.logtimeMark = *plogtime;

    lr.dbid = dbid;

    lr.le_cbPath = 0;

    rgdata[0].SetPv( (BYTE *)&lr );
    rgdata[0].SetCb( sizeof(lr) );
    rgdata[1].SetPv( reinterpret_cast<BYTE *>( L"" ) );
    rgdata[1].SetCb( lr.le_cbPath );

    return plog->ErrLGLogRec( rgdata, 2, fLGFlags, 0, plgposLogRec );
}


ERR ErrLGPagePatchRequest( LOG * const plog, const IFMP ifmp, const PGNO pgno, const DBTIME dbtime )
{
    ERR                 err;
    DATA                rgdata[1];
    LRPAGEPATCHREQUEST  lr;

    C_ASSERT(46 == sizeof(LRPAGEPATCHREQUEST));

    if ( plog->FLogDisabled() || ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo ) )
    {
        return JET_errSuccess;
    }

    Assert( g_rgfmp[ifmp].FLogOn() );

    lr.SetDbid( g_rgfmp[ifmp].Dbid() );
    lr.SetPgno( pgno );
    lr.SetDbtime( dbtime );

    rgdata[0].SetPv( (BYTE *)&lr );
    rgdata[0].SetCb( sizeof(lr) );

    err = plog->ErrLGLogRec( rgdata, 1, 0, 0, pNil );

    return err;
}

ERR ErrLGExtendDatabase( LOG * const plog, const IFMP ifmp, PGNO pgnoLast, LGPOS* const plgposExtend )
{
    ERR             err;
    DATA            rgdata[1];
    LREXTENDDB      lr;
    DBID            dbid = g_rgfmp[ifmp].Dbid();

    if ( plog->FLogDisabled() || ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo ) )
    {
        return JET_errSuccess;
    }

    Assert( g_rgfmp[ifmp].FLogOn() );

    lr.SetDbid( dbid );
    lr.SetPgnoLast( pgnoLast );

    rgdata[0].SetPv( &lr );
    rgdata[0].SetCb( sizeof(lr) );

    Call( plog->ErrLGLogRec( rgdata, _countof(rgdata), 0, 0, plgposExtend ) );

    {
    PIB pibFake;
    pibFake.m_pinst = PinstFromIfmp( ifmp );
    Call( ErrLGWaitForWrite( &pibFake, &lgposMax ) );
    Call( ErrLGFlush( plog, iofrLogFlushAll, fTrue ) );
    }

HandleError:
    return err;
}

ERR ErrLGShrinkDatabase( LOG * const plog, const IFMP ifmp, const PGNO pgnoLast, const CPG cpgShrunk, LGPOS* const plgposShrink )
{
    if ( plog->FLogDisabled() )
    {
        return JET_errSuccess;
    }

    ERR err = JET_errSuccess;
    DATA rgdata[1];
    DBID dbid = g_rgfmp[ifmp].Dbid();
    const BOOL fDbTimeSupported = g_rgfmp[ifmp].FEfvSupported( JET_efvDbTimeShrink );
    LRSHRINKDB* const plr = fDbTimeSupported ? ( new LRSHRINKDB3 ) : ( new LRSHRINKDB( lrtypShrinkDB2 ) );
    Alloc( plr );

    Assert( !plog->FRecovering() || ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo ) );
    
    Assert( g_rgfmp[ifmp].FLogOn() );

    plr->SetDbid( dbid );
    plr->SetPgnoLast( pgnoLast );
    plr->SetCpgShrunk( cpgShrunk );
    rgdata[0].SetPv( plr );
    rgdata[0].SetCb( fDbTimeSupported ? sizeof( LRSHRINKDB3 ) : sizeof( LRSHRINKDB ) );
    if ( fDbTimeSupported )
    {
        ( (LRSHRINKDB3*)plr )->SetDbtime( g_rgfmp[ifmp].DbtimeIncrementAndGet() );
    }
    Call( plog->ErrLGLogRec( rgdata, _countof(rgdata), 0, 0, plgposShrink ) );

    {
    PIB pibFake;
    pibFake.m_pinst = PinstFromIfmp( ifmp );
    Call( ErrLGWaitForWrite( &pibFake, &lgposMax ) );
    Call( ErrLGFlush( plog, iofrLogFlushAll, fTrue ) );
    }

HandleError:
    delete plr;
    return err;
}

ERR ErrLGTrimDatabase(
    _In_ LOG * const plog,
    _In_ const IFMP ifmp,
    _In_ PIB* const ppib,
    _In_ const PGNO pgnoStartZeroes,
    _In_ const CPG cpgZeroLength )
{
    ERR             err;
    DATA            rgdata[1];
    LRTRIMDB        lr;
    DBID            dbid = g_rgfmp[ifmp].Dbid();
    LGPOS           lgposStartOfLastRec;

    if ( plog->FLogDisabled() )
    {
        return JET_errSuccess;
    }

    Assert( !plog->FRecovering() || ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo ) );

    Assert( g_rgfmp[ifmp].FLogOn() );

    lr.SetDbid( dbid );
    lr.SetPgnoStartZeroes( pgnoStartZeroes );
    lr.SetCpgZeroLength( cpgZeroLength );

    rgdata[0].SetPv( &lr );
    rgdata[0].SetCb( sizeof(lr) );

    Call( plog->ErrLGLogRec( rgdata, _countof(rgdata), 0, ppib->lgposStart.lGeneration, &lgposStartOfLastRec ) );

    plog->LGAddLgpos( &lgposStartOfLastRec, rgdata[0].Cb() - 1 );
    Call( ErrLGWaitForWrite( ppib, &lgposStartOfLastRec ) );

HandleError:
    return err;
}

ERR ErrLGIgnoredRecord( LOG * const plog, const IFMP ifmp, const INT cb )
{
    ERR             err;
    BYTE*           pb = NULL;
    DATA            rgdata[2];
    LRIGNORED       lr( lrtypIgnored19 );

    C_ASSERT( 5 == sizeof(lr) );

    if ( plog->FLogDisabled() || ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo ) )
    {
        return JET_errSuccess;
    }

    Assert( g_rgfmp[ifmp].FLogOn() );

    Alloc( pb = new BYTE[cb] );

    lr.SetCb( cb );

    rgdata[0].SetPv( &lr );
    rgdata[0].SetCb( sizeof(lr) );
    rgdata[1].SetPv( pb );
    rgdata[1].SetCb( cb );

    Call( plog->ErrLGLogRec( rgdata, _countof(rgdata), 0, 0, pNil ) );

HandleError:
    delete[] pb;
    return err;
}

ERR ErrLGExtentFreed( LOG * const plog, const IFMP ifmp, const PGNO pgnoFirst, const CPG cpgExtent )
{
    DATA                rgdata[1];
    LREXTENTFREED  lr;

    C_ASSERT( 11 == sizeof(LREXTENTFREED) );

    if ( plog->FLogDisabled() || ( plog->FRecovering() && plog->FRecoveringMode() != fRecoveringUndo ) )
    {
        return JET_errSuccess;
    }

    Assert( g_rgfmp[ifmp].FLogOn() );

    lr.SetDbid( g_rgfmp[ifmp].Dbid() );
    lr.SetPgnoFirst( pgnoFirst );
    lr.SetCpgExtent( cpgExtent );

    rgdata[0].SetPv( (BYTE *)&lr );
    rgdata[0].SetCb( sizeof(lr) );

    return plog->ErrLGLogRec( rgdata, 1, 0, 0, pNil );
}


const char * const szNOP                        = "NOP      ";
const char * const szNOPEndOfList               = "NOPEnd   ";
const char * const szInit                       = "Init     ";
const char * const szTerm                       = "Term     ";
const char * const szMS                         = "MS       ";
const char * const szEnd                        = "End      ";

const char * const szBegin                      = "Begin    ";
const char * const szCommit                     = "Commit   ";
const char * const szRollback                   = "Rollback ";

const char * const szCreateDB                   = "CreateDB ";
const char * const szCreateDBFinish             = "CrDBFinsh";
const char * const szAttachDB                   = "AttachDB ";
const char * const szDetachDB                   = "DetachDB ";
const char * const szDbList                     = "DbList   ";

const char * const szCreateMultipleExtentFDP    = "Create M ";
const char * const szCreateSingleExtentFDP      = "Create S ";
const char * const szConvertFDP                 = "Convert  ";
const char * const szFreeFDP                    = "FreeFDP  ";

const char * const szSplit                      = "Split    ";
const char * const szSplit2                     = "Split2   ";
const char * const szMerge                      = "Merge    ";
const char * const szMerge2                     = "Merge2   ";
const char * const szEmptyTree                  = "EmptyTree";
const char * const szPageMove                   = "PageMove ";
const char * const szRootPageMove               = "PageMoveR";

const char * const szInsert                     = "Insert   ";
const char * const szFlagInsert                 = "FInsert  ";
const char * const szFlagInsertAndReplaceData   = "FInsertRD";
const char * const szFlagDelete                 = "FDelete  ";
const char * const szReplace                    = "Replace  ";
const char * const szReplaceD                   = "ReplaceD ";

const char * const szLock                       = "Lock     ";
const char * const szUndoInfo                   = "UndoInfo ";

const char * const szDelta                      = "Delta    ";
const char * const szDelta64                    = "Delta64  ";
const char * const szDelete                     = "Delete   ";

const char * const szUndo                       = "Undo     ";

const char * const szScrub                      = "Scrub    ";

const char * const szBegin0                     = "Begin0   ";
const char * const szBeginDT                    = "BeginDT  ";
const char * const szPrepCommit                 = "PreCommit";
const char * const szPrepRollback               = "PreRollbk";
const char * const szCommit0                    = "Commit0  ";
const char * const szRefresh                    = "Refresh  ";

const char * const szRecoveryUndo               = "RcvUndo  ";
const char * const szRecoveryQuit               = "RcvQuit  ";
const char * const szRecoveryQuitKeepAttachments = "RcvQKA   ";

const char * const szFullBackup                 = "FullBkUp ";
const char * const szIncBackup                  = "IncBkUp  ";
const char * const szBackup                     = "Backup   ";
const char * const szBackup2                    = "Backup2  ";

const char * const szJetOp                      = "JetOp    ";
const char * const szTrace                      = "Trace    ";

const char * const szShutDownMark               = "ShutDown ";

const char * const szSetExternalHeader          = "SetExtHdr";

const char * const szMacroBegin                 = "McroBegin";
const char * const szMacroCommit                = "McroComit";
const char * const szMacroAbort                 = "McroAbort";

const char * const szChecksum                   = "Checksum ";

const char * const szExtRestore                 = "ExtRest  ";
const char * const szForceWriteLog              = "FWriteLog";
const char * const szExtRestore2                = "ExtRest2 ";
const char * const szForceLogRollover           = "FRollLog ";

const char * const szPagePatchRequest           = "PagePatch";

const char * const szMacroInfo                  = "McroInfo ";
const char * const szExtendDB                   = "ExtendDB ";
const char * const szCommitCtx                  = "CommitCtx";
const char * const szScanCheck                  = "ScanCheck";
const char * const szScanCheck2                 = "ScanChk2 ";
const char * const szNOP2                       = "NOP2     ";
const char * const szReAttach                   = "ReAttach ";
const char * const szMacroInfo2                 = "McroInfo2";
const char * const szIgnored                    = "Ignored  ";

const char * const szFragmentBegin              = "FragBegin";
const char * const szFragment                   = "FragContd";

const char * const szShrinkDB                   = "ShrinkDB ";
const char * const szShrinkDB2                  = "ShrinkDB2";
const char * const szShrinkDB3                  = "ShrinkDB3";
const char * const szTrimDB                     = "TrimDB   ";

const char * const szSetDbHdrVersion            = "SetDbVer ";

const char * const szNewPage                    = "NewPage  ";

const char * const szSignalAttachDb             = "SAttachDB";

const char * const szExtentFreed                = "ExtentFreed";

const char * szUnknown                          = "*UNKNOWN*";

const char * SzLrtyp( LRTYP lrtyp )
{
    switch ( lrtyp )
    {
        case lrtypNOP:          return szNOP;
        case lrtypInit:         return szInit;
        case lrtypInit2:        return szInit;
        case lrtypTerm:         return szTerm;
        case lrtypTerm2:        return szTerm;
        case lrtypMS:           return szMS;
        case lrtypEnd:          return szEnd;

        case lrtypBegin:        return szBegin;
        case lrtypCommit:       return szCommit;
        case lrtypRollback:     return szRollback;
        case lrtypBegin0:       return szBegin0;
        case lrtypCommit0:      return szCommit0;
        case lrtypBeginDT:      return szBeginDT;
        case lrtypPrepCommit:   return szPrepCommit;
        case lrtypPrepRollback: return szPrepRollback;
        case lrtypRefresh:      return szRefresh;
        case lrtypMacroBegin:   return szMacroBegin;
        case lrtypMacroCommit:  return szMacroCommit;
        case lrtypMacroAbort:   return szMacroAbort;

        case lrtypCreateDB:     return szCreateDB;
        case lrtypCreateDBFinish:   return szCreateDBFinish;
        case lrtypAttachDB:     return szAttachDB;
        case lrtypDetachDB:     return szDetachDB;

        case lrtypRecoveryUndo: return szRecoveryUndo;
        case lrtypRecoveryUndo2:    return szRecoveryUndo;
        case lrtypRecoveryQuit: return szRecoveryQuit;
        case lrtypRecoveryQuit2: return szRecoveryQuit;
        case lrtypRecoveryQuitKeepAttachments: return szRecoveryQuitKeepAttachments;

        case lrtypFullBackup:   return szFullBackup;
        case lrtypIncBackup:    return szIncBackup;
        case lrtypBackup:       return szBackup;
        case lrtypBackup2:      return szBackup2;

        case lrtypJetOp:        return szJetOp;
        case lrtypTrace:        return szTrace;

        case lrtypShutDownMark: return szShutDownMark;

        case lrtypCreateMultipleExtentFDP:  return szCreateMultipleExtentFDP;
        case lrtypCreateSingleExtentFDP:    return szCreateSingleExtentFDP;
        case lrtypConvertFDP2:              return szConvertFDP;
        case lrtypConvertFDP:       return szConvertFDP;

        case lrtypSplit:        return szSplit;
        case lrtypSplit2:       return szSplit2;
        case lrtypMerge:        return szMerge;
        case lrtypMerge2:       return szMerge2;
        case lrtypEmptyTree:    return szEmptyTree;
        case lrtypPageMove:     return szPageMove;
        case lrtypRootPageMove: return szRootPageMove;

        case lrtypInsert:       return szInsert;
        case lrtypFlagInsert:   return szFlagInsert;
        case lrtypFlagInsertAndReplaceData:
                                return szFlagInsertAndReplaceData;
        case lrtypFlagDelete:   return szFlagDelete;
        case lrtypReplace:      return szReplace;
        case lrtypReplaceD:     return szReplaceD;
        case lrtypDelete:       return szDelete;

        case lrtypUndoInfo:     return szUndoInfo;

        case lrtypDelta:        return szDelta;

        case lrtypDelta64:      return szDelta64;

        case lrtypSetExternalHeader:
                                return szSetExternalHeader;

        case lrtypUndo:         return szUndo;
        case lrtypScrub:        return szScrub;
        case lrtypChecksum:     return szChecksum;
        case lrtypExtRestore:   return szExtRestore;
        case lrtypExtRestore2:  return szExtRestore2;
        case lrtypForceWriteLog:    return szForceWriteLog;
        case lrtypForceLogRollover: return szForceLogRollover;
        case lrtypPagePatchRequest: return szPagePatchRequest;

        case lrtypMacroInfo:        return szMacroInfo;
        case lrtypExtendDB:         return szExtendDB;
        case lrtypCommitCtx:        return szCommitCtx;
        case lrtypScanCheck:        return szScanCheck;
        case lrtypScanCheck2:       return szScanCheck2;
        case lrtypNOP2:             return szNOP2;
        case lrtypReAttach:         return szReAttach;
        case lrtypMacroInfo2:       return szMacroInfo2;
        case lrtypFreeFDP:          return szFreeFDP;

        case lrtypIgnored9:
        case lrtypIgnored10:
        case lrtypIgnored11:
        case lrtypIgnored12:
        case lrtypIgnored13:
        case lrtypIgnored14:
        case lrtypIgnored15:
        case lrtypIgnored16:
        case lrtypIgnored17:
        case lrtypIgnored18:
        case lrtypIgnored19:
                                    return szIgnored;

        case lrtypFragmentBegin:    return szFragmentBegin;
        case lrtypFragment:         return szFragment;
        case lrtypShrinkDB:         return szShrinkDB;
        case lrtypShrinkDB2:        return szShrinkDB2;
        case lrtypShrinkDB3:        return szShrinkDB3;
        case lrtypTrimDB:           return szTrimDB;
        case lrtypSetDbVersion:     return szSetDbHdrVersion;
        case lrtypNewPage:          return szNewPage;
        case lrtypSignalAttachDb:   return szSignalAttachDb;
        case lrtypExtentFreed:      return szExtentFreed;

        default:
            AssertSz( fFalse, "Unknown lrtyp: %d\n", lrtyp );

            __fallthrough;
        case lrtypSLVPageAppendOBSOLETE:
        case lrtypSLVSpaceOBSOLETE:
        case lrtypSLVPageMoveOBSOLETE:
        case lrtypForceDetachDBOBSOLETE:
        case 47:
        case 56:
            return szUnknown;
    }
}


ERR ErrLrToLogCsvSimple(
    CWPRINTFFILE * pcwpfCsvOut,
    LGPOS lgpos,
    const LR *plr,
    LOG * plog )
{
    ERR     err = JET_errSuccess;
    enum { eUnprocessed, eConsumed, eNopIgnored, eMax } eProcessed = eUnprocessed;

    ULONG ulCkSumSeed = ( (lgpos.lGeneration) ^ (lgpos.isec << 16 | lgpos.ib) );

    WCHAR const * szLogRecordChecksumInfo           = L"LRCI";
    WCHAR const * szLogRecordDatabaseInfo           = L"LRDI";
    WCHAR const * szLogRecordPgChangeInfo           = L"LRPI";
    WCHAR const * szLogRecordMiscelLrInfo           = L"LRMI";
    WCHAR const * szLogRecordResizeDatabaseInfo     = L"LRRI";
    WCHAR const * szLogRecordTrimDatabaseInfo       = L"LRTI";

    typedef struct
    {
        ULONG       ulChecksum1;
        ULONG       ulChecksum2;
        PGNO        pgno;
        OBJID       objid;
        DBID        dbid;
        DBTIME      dbtimePre;
        DBTIME      dbtimePost;
        WCHAR*      szDbPath;
        SIGNATURE   signDb;
    } SIMPLE_CSV_CHG_INFO;

    #define LR_STACK_CSV_LINES   6
    const WCHAR *               rgszLogRecordsCsvFormats[LR_STACK_CSV_LINES] = { 0 };
    SIMPLE_CSV_CHG_INFO         rgChangeInfo[LR_STACK_CSV_LINES] = { 0 };

    WCHAR const **              pLogRecordsCsvFormats = rgszLogRecordsCsvFormats;
    SIMPLE_CSV_CHG_INFO *       pChangeInfo = rgChangeInfo;
    ULONG                       cCsvLines = LR_STACK_CSV_LINES;
    ULONG                       cLogRecordsCsvFormats = 0;

    WCHAR                       rgTempDbPath[IFileSystemAPI::cchPathMax];
    CAutoWSZPATH                    wszTempDbAlignedPath;


#ifdef DEBUG_CODE_NOT_READY
    static ULONG cbLR;
    static LGPOS lgposPrev;


    

    Assert( cbLR < 33 * 1024 );

    if ( lgpos.isec != 0 &&
            lgpos.ib != 0 )
    {
        const __int64 cbDelta = plog->CbOffsetLgpos( lgpos, lgposPrev );
        Assert( cbDelta == cbLR );
    }

    lgposPrev = lgpos;

#else
    ULONG cbLR;
#endif
    cbLR = 34 * 1024;

    const static ULONG cchFormatLgposSz = 30;
    char szLgposLR[cchFormatLgposSz];
    szLgposLR[0] = L'\0';
    OSStrCbFormatA( szLgposLR, sizeof(szLgposLR), ", %08X:%04X:%04X", lgpos.lGeneration, lgpos.isec, lgpos.ib);

    cbLR = CbLGSizeOfRec( plr );


#define SetLogCsvTypeSz(szCsvType) \
    if ( cLogRecordsCsvFormats >= (cCsvLines-1) ) \
    { \
        const WCHAR ** pCsvFormatsTemp = pLogRecordsCsvFormats; \
        SIMPLE_CSV_CHG_INFO * pCsvInfosTemp = pChangeInfo; \
        \
        pLogRecordsCsvFormats = (const WCHAR**) malloc( (sizeof(WCHAR*) * cCsvLines*2 ) ); \
        if ( pLogRecordsCsvFormats == NULL ) \
        { \
            pLogRecordsCsvFormats = pCsvFormatsTemp;  \
            err = ErrERRCheck( JET_errOutOfMemory ); \
            goto HandleError; \
        } \
        memcpy( (void*)pLogRecordsCsvFormats, (void*)pCsvFormatsTemp, sizeof(WCHAR*)*cCsvLines ); \
        if ( pCsvFormatsTemp != rgszLogRecordsCsvFormats ) \
        { \
            free ( pCsvFormatsTemp ); \
        } \
        \
        pChangeInfo = (SIMPLE_CSV_CHG_INFO*) malloc( sizeof(SIMPLE_CSV_CHG_INFO) * cCsvLines*2 ); \
        if ( pChangeInfo == NULL ) \
        { \
            pChangeInfo = pCsvInfosTemp;   \
            err = ErrERRCheck( JET_errOutOfMemory ); \
            goto HandleError; \
        } \
        memcpy( (void*)pChangeInfo, (void*)pCsvInfosTemp, sizeof(SIMPLE_CSV_CHG_INFO)*cCsvLines ); \
        if ( pCsvInfosTemp != rgChangeInfo ) \
        { \
            free ( pCsvInfosTemp ); \
        } \
        \
        cCsvLines = cCsvLines*2; \
        \
    } \
    pLogRecordsCsvFormats[cLogRecordsCsvFormats] = szCsvType; \
    ;

#define SetLogCsvChangeInfo(chk1, chk2, pgnoIn, objidIn, dbidIn, dbtimePreIn, dbtimePostIn) \
    Assert( cLogRecordsCsvFormats < (cCsvLines-1) ); \
    memset( &(pChangeInfo[cLogRecordsCsvFormats]), 0, sizeof(SIMPLE_CSV_CHG_INFO) ); \
    pChangeInfo[cLogRecordsCsvFormats].ulChecksum1  = chk1;         \
    pChangeInfo[cLogRecordsCsvFormats].ulChecksum2  = chk2;         \
    pChangeInfo[cLogRecordsCsvFormats].pgno         = pgnoIn;       \
    pChangeInfo[cLogRecordsCsvFormats].objid        = objidIn;      \
    pChangeInfo[cLogRecordsCsvFormats].dbid         = dbidIn;       \
    pChangeInfo[cLogRecordsCsvFormats].dbtimePre    = dbtimePreIn;  \
    pChangeInfo[cLogRecordsCsvFormats].dbtimePost   = dbtimePostIn;

#define SetLogCsvChecksumInfo(chk1, chk2) \
    Assert( cLogRecordsCsvFormats < (cCsvLines-1) ); \
    memset( &(pChangeInfo[cLogRecordsCsvFormats]), 0, sizeof(SIMPLE_CSV_CHG_INFO) ); \
    pChangeInfo[cLogRecordsCsvFormats].ulChecksum1  = chk1;     \
    pChangeInfo[cLogRecordsCsvFormats].ulChecksum2  = chk2;

#define SetLogCsvDatabaseInfo(chk1, chk2, dbidIn, szDb, signDbIn ) \
    Assert( cLogRecordsCsvFormats < (cCsvLines-1) ); \
    memset( &(pChangeInfo[cLogRecordsCsvFormats]), 0, sizeof(SIMPLE_CSV_CHG_INFO) ); \
    pChangeInfo[cLogRecordsCsvFormats].ulChecksum1  = chk1;     \
    pChangeInfo[cLogRecordsCsvFormats].ulChecksum2  = chk2;     \
    pChangeInfo[cLogRecordsCsvFormats].dbid         = dbidIn;   \
    pChangeInfo[cLogRecordsCsvFormats].szDbPath     = szDb;     \
    pChangeInfo[cLogRecordsCsvFormats].signDb       = signDbIn;

#define SetLogCsvResizeInfo(chk1, chk2, dbidIn ) \
    Assert( cLogRecordsCsvFormats < (cCsvLines-1) ); \
    memset( &(pChangeInfo[cLogRecordsCsvFormats]), 0, sizeof(SIMPLE_CSV_CHG_INFO) ); \
    pChangeInfo[cLogRecordsCsvFormats].ulChecksum1  = chk1;     \
    pChangeInfo[cLogRecordsCsvFormats].ulChecksum2  = chk2;     \
    pChangeInfo[cLogRecordsCsvFormats].dbid         = dbidIn;

    Assert( lrtypMax > plr->lrtyp );
    const CHAR * szLRTyp = SzLrtyp( (plr->lrtyp >= lrtypMax) ? lrtypMax : plr->lrtyp );

#define UlChecksumSimpleLR( plrVar )   LGChecksum::UlChecksumBytes( (BYTE*) (plrVar), ((BYTE*)(plrVar))+sizeof(*(plrVar)), ulCkSumSeed )

#define UlChecksumDataLR( plrVar, cbVar ) LGChecksum::UlChecksumBytes( (BYTE*) (plrVar), ((BYTE*)(plrVar))+sizeof(*(plrVar))+(cbVar), ulCkSumSeed )

    switch ( plr->lrtyp )
    {
        case lrtypNOP:
            if( plog )
            {
                plog->IncNOP();
            }
#ifdef TEST_IGNORED_LRS_TO_GET_REAL_LAST_LGPOS
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plr ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
#else
            eProcessed = eNopIgnored;
#endif
            break;

        case lrtypMS:
        {
            const LRMS * const plrms = (LRMS *)plr;

            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrms ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;


        case lrtypInsert:
        {
            const LRINSERT * const  plrinsert = (LRINSERT *)plr;
            const BYTE *        pb;
            ULONG           cb;
            pb = (const BYTE *) plr + sizeof( LRINSERT );
            cb = plrinsert->CbSuffix() + plrinsert->CbPrefix() + plrinsert->CbData();
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, UlChecksumDataLR( plrinsert, cb ), (PGNO) plrinsert->le_pgno,
                (OBJID) plrinsert->le_objidFDP,
                (DBID) plrinsert->dbid, (DBTIME) plrinsert->le_dbtimeBefore, (DBTIME) plrinsert->le_dbtime );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypFlagInsert:
        {
            const LRFLAGINSERT * const plrflaginsert = (LRFLAGINSERT *)plr;
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, UlChecksumSimpleLR( plrflaginsert ), (PGNO) plrflaginsert->le_pgno, (OBJID) plrflaginsert->le_objidFDP,
                (DBID) plrflaginsert->dbid, (DBTIME) plrflaginsert->le_dbtimeBefore, (DBTIME) plrflaginsert->le_dbtime );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypReplace:
        case lrtypReplaceD:
        {
            const LRREPLACE * const plrreplace = (LRREPLACE *)plr;
            const BYTE *    pb;
            USHORT  cb;
            pb = (const BYTE *) plrreplace + sizeof( LRREPLACE );
            cb = plrreplace->Cb();
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, UlChecksumDataLR( plrreplace, cb ), (PGNO) plrreplace->le_pgno, (OBJID) plrreplace->le_objidFDP,
                (DBID) plrreplace->dbid, (DBTIME) plrreplace->le_dbtimeBefore, (DBTIME) plrreplace->le_dbtime );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypFlagInsertAndReplaceData:
        {
            const LRFLAGINSERTANDREPLACEDATA * const plrfiard = (LRFLAGINSERTANDREPLACEDATA *)plr;
            const BYTE *    pb;
            ULONG       cb;
            pb = (const BYTE *) plrfiard + sizeof( LRFLAGINSERTANDREPLACEDATA );
            cb = plrfiard->CbKey() + plrfiard->CbData();
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, UlChecksumDataLR( plrfiard, cb ), (PGNO) plrfiard->le_pgno, (OBJID) plrfiard->le_objidFDP,
                (DBID) plrfiard->dbid, (DBTIME) plrfiard->le_dbtimeBefore, (DBTIME) plrfiard->le_dbtime );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypFlagDelete:
        {
            const LRFLAGDELETE * const plrflagdelete = (LRFLAGDELETE *)plr;
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, UlChecksumSimpleLR( plrflagdelete ), (PGNO) plrflagdelete->le_pgno, (OBJID) plrflagdelete->le_objidFDP,
                (DBID) plrflagdelete->dbid, (DBTIME) plrflagdelete->le_dbtimeBefore, (DBTIME) plrflagdelete->le_dbtime );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypDelete:
        {
            const LRDELETE * const plrdelete = (LRDELETE *)plr;
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, UlChecksumSimpleLR( plrdelete ), (PGNO) plrdelete->le_pgno, (OBJID) plrdelete->le_objidFDP,
                (DBID) plrdelete->dbid, (DBTIME) plrdelete->le_dbtimeBefore, (DBTIME) plrdelete->le_dbtime );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypUndoInfo:
        {
            const LRUNDOINFO * const plrundoinfo = (LRUNDOINFO *)plr;
            const BYTE * pb = plrundoinfo->rgbData;
            ULONG cb = plrundoinfo->le_cbData;
            Assert( pb == (((BYTE*)plrundoinfo)+sizeof(*plrundoinfo)) );
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, UlChecksumDataLR( plrundoinfo, cb ), (PGNO) plrundoinfo->le_pgno, (OBJID) plrundoinfo->le_objidFDP,
                (DBID) plrundoinfo->dbid, (DBTIME) plrundoinfo->le_dbtimeBefore, (DBTIME) plrundoinfo->le_dbtime );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypDelta:
        {
            const LRDELTA32 * const plrdelta = (LRDELTA32 *)plr;
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, UlChecksumSimpleLR( plrdelta ), (PGNO) plrdelta->le_pgno, (OBJID) plrdelta->le_objidFDP,
                (DBID) plrdelta->dbid, (DBTIME) plrdelta->le_dbtimeBefore, (DBTIME) plrdelta->le_dbtime );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypDelta64:
        {
            const LRDELTA64 * const plrdelta = (LRDELTA64 *) plr;
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, UlChecksumSimpleLR( plrdelta ), (PGNO) plrdelta->le_pgno, (OBJID) plrdelta->le_objidFDP,
                (DBID) plrdelta->dbid, (DBTIME) plrdelta->le_dbtimeBefore, (DBTIME) plrdelta->le_dbtime );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypBegin:
        case lrtypBegin0:
        {
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            if ( plr->lrtyp == lrtypBegin )
            {
                const LRBEGIN       * const plrbegin    = (LRBEGINDT *)plr;
                SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrbegin ) );
            }
            else
            {
                const LRBEGINDT     * const plrbeginDT  = (LRBEGINDT *)plr;
                SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrbeginDT ) );
            }
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypMacroBegin:
        {
            const LRMACROBEGIN * const plrmbegin = (LRMACROBEGIN *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrmbegin ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypMacroCommit:
        case lrtypMacroAbort:
        {
            const LRMACROEND * const plrmend = (LRMACROEND *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrmend ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypRefresh:
        {
            const LRREFRESH * const plrrefresh = (LRREFRESH *) plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrrefresh ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypCommit:
        case lrtypCommit0:
        {
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            if ( plr->lrtyp == lrtypCommit )
            {
                const LRCOMMIT * const plrcommit = (LRCOMMIT0 *)plr;
                SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrcommit ) );
            }
            else
            {
                const LRCOMMIT0 * const plrcommit0 = (LRCOMMIT0 *)plr;
                SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrcommit0 ) );
            }
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypRollback:
        {
            const LRROLLBACK * const plrrollback = (LRROLLBACK *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrrollback ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypForceWriteLog:
        {
            const LRFORCEWRITELOG   * const plrForceWritelog    = (LRFORCEWRITELOG *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrForceWritelog ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypCreateDB:
        {
            const LRCREATEDB * const plrcreatedb = (LRCREATEDB *) plr;
            WCHAR * wszDbPath = NULL;
            BYTE* pb;
            ULONG cb = ( plrcreatedb->FVersionInfo() ? sizeof(LRCREATEDB::VersionInfo) : 0 );

            pb =  ((BYTE*)( plr ))
                + sizeof( LRCREATEDB )
                + cb;
            if ( plrcreatedb->FUnicodeNames() )
            {
                CallS( wszTempDbAlignedPath.ErrSet( (UnalignedLittleEndian< WCHAR > *)pb ) );
                wszDbPath = (WCHAR*) wszTempDbAlignedPath;
                cb += sizeof(WCHAR) * (LOSStrLengthW( wszDbPath ) + 1);
            }
            else
            {
                Call( ErrOSSTRAsciiToUnicode( (CHAR*)pb, rgTempDbPath, sizeof(rgTempDbPath)/sizeof(WCHAR)) );
                wszDbPath = rgTempDbPath;
                cb += strlen( (CHAR*) pb ) + 1;
            }

            SetLogCsvTypeSz( szLogRecordDatabaseInfo );
            SetLogCsvDatabaseInfo( 0, UlChecksumDataLR( plrcreatedb, cb ), (DBID) plrcreatedb->dbid, wszDbPath, plrcreatedb->signDb );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypCreateDBFinish:
        {
            const LRCREATEDBFINISH * const plrCreateDBFinish = (LRCREATEDBFINISH *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrCreateDBFinish ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypAttachDB:
        {
            const LRATTACHDB * const plrattachdb = (LRATTACHDB *) plr;
            WCHAR * wszDbPath = NULL;
            const BYTE * pb = reinterpret_cast<const BYTE *>( plrattachdb ) + sizeof(LRATTACHDB);
            ULONG cb = 0;

            if ( plrattachdb->FUnicodeNames() )
            {
                CallS( wszTempDbAlignedPath.ErrSet( (UnalignedLittleEndian< WCHAR > *)pb ) );
                wszDbPath = (WCHAR*) wszTempDbAlignedPath;
                cb = sizeof(WCHAR) * (LOSStrLengthW(wszDbPath)+1);
            }
            else
            {
                Call( ErrOSSTRAsciiToUnicode( (CHAR*)pb, rgTempDbPath, sizeof(rgTempDbPath)/sizeof(WCHAR)) );
                wszDbPath = rgTempDbPath;
                cb = strlen((CHAR*)pb)+1;
            }


            SetLogCsvTypeSz( szLogRecordDatabaseInfo );
            SetLogCsvDatabaseInfo( 0, UlChecksumDataLR( plrattachdb, cb ), (DBID)plrattachdb->dbid, wszDbPath, plrattachdb->signDb );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypDetachDB:
        {
            const LRDETACHDB * const plrdetachdb = (LRDETACHDB *) plr;
            WCHAR * wszDbPath = NULL;
            const BYTE * pb = reinterpret_cast<const BYTE*>( plrdetachdb ) + sizeof(LRDETACHDB);
            ULONG cb = 0;
            if ( plrdetachdb->FUnicodeNames() )
            {
                CallS( wszTempDbAlignedPath.ErrSet( (UnalignedLittleEndian< WCHAR > *)pb ) );
                wszDbPath = (WCHAR*) wszTempDbAlignedPath;
                cb = sizeof(WCHAR) * (LOSStrLengthW(wszDbPath)+1);
            }
            else
            {
                Call( ErrOSSTRAsciiToUnicode( (CHAR*)pb, rgTempDbPath, sizeof(rgTempDbPath)/sizeof(WCHAR)) );
                wszDbPath = rgTempDbPath;
                cb = strlen((CHAR*)pb)+1;
            }
            SetLogCsvTypeSz( szLogRecordDatabaseInfo );
            SIGNATURE signatureNil;
            memset( &signatureNil, 0, sizeof( signatureNil ) );
            SetLogCsvDatabaseInfo( 0, UlChecksumDataLR( plrdetachdb, cb ), (DBID)plrdetachdb->dbid, wszDbPath, signatureNil );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypCreateMultipleExtentFDP:
        {
            const LRCREATEMEFDP * const plrcreatemefdp = (LRCREATEMEFDP *)plr;

            

            const ULONG ulCkSum = UlChecksumSimpleLR( plrcreatemefdp );

            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrcreatemefdp->le_pgno, (OBJID) plrcreatemefdp->le_objidFDP,
                (DBID) plrcreatemefdp->dbid, (DBTIME) plrcreatemefdp->le_dbtimeBefore, (DBTIME) plrcreatemefdp->le_dbtime );
            cLogRecordsCsvFormats++;

            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrcreatemefdp->le_pgnoOE, (OBJID) plrcreatemefdp->le_objidFDP,
                (DBID) plrcreatemefdp->dbid, dbtimeNil, (DBTIME) plrcreatemefdp->le_dbtime );
            cLogRecordsCsvFormats++;

            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrcreatemefdp->le_pgnoAE, (OBJID) plrcreatemefdp->le_objidFDP,
                (DBID) plrcreatemefdp->dbid, dbtimeNil, (DBTIME) plrcreatemefdp->le_dbtime );
            cLogRecordsCsvFormats++;

            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrcreatemefdp->le_pgnoFDPParent, (OBJID) plrcreatemefdp->le_objidFDP,
                (DBID) plrcreatemefdp->dbid, dbtimeNil, (DBTIME) plrcreatemefdp->le_dbtime );
            cLogRecordsCsvFormats++;

            eProcessed = eConsumed;

        }
            break;

        case lrtypCreateSingleExtentFDP:
        {
            const LRCREATESEFDP * const plrcreatesefdp = (LRCREATESEFDP *)plr;

            ULONG ulCkSum = UlChecksumSimpleLR( plrcreatesefdp );

            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrcreatesefdp->le_pgno, (OBJID) plrcreatesefdp->le_objidFDP,
                (DBID) plrcreatesefdp->dbid, (DBTIME) plrcreatesefdp->le_dbtimeBefore, (DBTIME) plrcreatesefdp->le_dbtime );
            cLogRecordsCsvFormats++;

            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrcreatesefdp->le_pgnoFDPParent, (OBJID) plrcreatesefdp->le_objidFDP,
                (DBID) plrcreatesefdp->dbid, dbtimeNil, (DBTIME) plrcreatesefdp->le_dbtime );
            cLogRecordsCsvFormats++;
            
            eProcessed = eConsumed;

        }
            break;

        case lrtypConvertFDP2:
        case lrtypConvertFDP:
        {
            const LRCONVERTFDP * const plrconvertfdp = (LRCONVERTFDP *)plr;

            ULONG ulCkSum;
            if ( plr->lrtyp == lrtypConvertFDP )
            {
                LRCONVERTFDP_OBSOLETE *plrconvertfdpObsolete = (LRCONVERTFDP *)plr;
                ulCkSum = UlChecksumSimpleLR( plrconvertfdpObsolete );
            }
            else
            {
                ulCkSum = UlChecksumSimpleLR( plrconvertfdp );
            }

            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrconvertfdp->le_pgno, (OBJID) plrconvertfdp->le_objidFDP,
                (DBID) plrconvertfdp->dbid, (DBTIME) plrconvertfdp->le_dbtimeBefore, (DBTIME) plrconvertfdp->le_dbtime );
            cLogRecordsCsvFormats++;

            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrconvertfdp->le_pgnoFDPParent, (OBJID) plrconvertfdp->le_objidFDP,
                (DBID) plrconvertfdp->dbid, dbtimeNil, (DBTIME) plrconvertfdp->le_dbtime );
            cLogRecordsCsvFormats++;

            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrconvertfdp->le_pgnoOE, (OBJID) plrconvertfdp->le_objidFDP,
                (DBID) plrconvertfdp->dbid, dbtimeNil, (DBTIME) plrconvertfdp->le_dbtime );
            cLogRecordsCsvFormats++;

            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrconvertfdp->le_pgnoAE, (OBJID) plrconvertfdp->le_objidFDP,
                (DBID) plrconvertfdp->dbid, dbtimeNil, (DBTIME) plrconvertfdp->le_dbtime );
            cLogRecordsCsvFormats++;

            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrconvertfdp->le_pgnoSecondaryFirst, (OBJID) plrconvertfdp->le_objidFDP,
                (DBID) plrconvertfdp->dbid, dbtimeNil, (DBTIME) plrconvertfdp->le_dbtime );
            cLogRecordsCsvFormats++;

            eProcessed = eConsumed;
        }
            break;

        case lrtypSplit:
        case lrtypSplit2:
        {
            const LRSPLIT_ * const plrsplit = (LRSPLIT_ *)plr;

            ULONG ulCkSum = UlChecksumSimpleLR( plrsplit );

            if ( pgnoNull == plrsplit->le_pgnoNew )
            {
                SetLogCsvTypeSz( szLogRecordPgChangeInfo );
                SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrsplit->le_pgno, (OBJID) plrsplit->le_objidFDP,
                    (DBID) plrsplit->dbid, (DBTIME) plrsplit->le_dbtimeBefore, (DBTIME) plrsplit->le_dbtime );
                cLogRecordsCsvFormats++;

                eProcessed = eConsumed;
            }
            else
            {
                SetLogCsvTypeSz( szLogRecordPgChangeInfo );
                SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrsplit->le_pgno, (OBJID) plrsplit->le_objidFDP,
                    (DBID) plrsplit->dbid, (DBTIME) plrsplit->le_dbtimeBefore, (DBTIME) plrsplit->le_dbtime );
                cLogRecordsCsvFormats++;

                SetLogCsvTypeSz( szLogRecordPgChangeInfo );
                SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrsplit->le_pgnoNew, (OBJID) plrsplit->le_objidFDP,
                    (DBID) plrsplit->dbid, (DBTIME) dbtimeNil, (DBTIME) plrsplit->le_dbtime );
                cLogRecordsCsvFormats++;

                if ( pgnoNull != (PGNO) plrsplit->le_pgnoParent )
                {
                    SetLogCsvTypeSz( szLogRecordPgChangeInfo );
                    SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrsplit->le_pgnoParent, (OBJID) plrsplit->le_objidFDP,
                        (DBID) plrsplit->dbid, (DBTIME) plrsplit->le_dbtimeParentBefore, (DBTIME) plrsplit->le_dbtime );
                    cLogRecordsCsvFormats++;
                }

                if ( pgnoNull != (PGNO) plrsplit->le_pgnoRight )
                {
                    SetLogCsvTypeSz( szLogRecordPgChangeInfo );
                    SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrsplit->le_pgnoRight, (OBJID) plrsplit->le_objidFDP,
                        (DBID) plrsplit->dbid, (DBTIME) plrsplit->le_dbtimeRightBefore, (DBTIME) plrsplit->le_dbtime );
                    cLogRecordsCsvFormats++;
                }
                
                eProcessed = eConsumed;
            }

        }
            break;
        
        case lrtypMerge:
        case lrtypMerge2:
        {
            const LRMERGE_ * const plrmerge = (LRMERGE_ *)plr;
            INT     cb = plrmerge->le_cbKeyParentSep;

            ULONG ulCkSum = UlChecksumDataLR( plrmerge, cb );

            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrmerge->le_pgno, (OBJID) plrmerge->le_objidFDP,
                (DBID) plrmerge->dbid, (DBTIME) plrmerge->le_dbtimeBefore, (DBTIME) plrmerge->le_dbtime );
            cLogRecordsCsvFormats++;

            if ( pgnoNull != (PGNO) plrmerge->le_pgnoParent )
            {
                SetLogCsvTypeSz( szLogRecordPgChangeInfo );
                SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrmerge->le_pgnoParent, (OBJID) plrmerge->le_objidFDP,
                    (DBID) plrmerge->dbid, (DBTIME) plrmerge->le_dbtimeParentBefore, (DBTIME) plrmerge->le_dbtime );
                cLogRecordsCsvFormats++;
            }

            if ( pgnoNull != (PGNO) plrmerge->le_pgnoRight )
            {
                SetLogCsvTypeSz( szLogRecordPgChangeInfo );
                SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrmerge->le_pgnoRight, (OBJID) plrmerge->le_objidFDP,
                    (DBID) plrmerge->dbid, (DBTIME) plrmerge->le_dbtimeRightBefore, (DBTIME) plrmerge->le_dbtime );
                cLogRecordsCsvFormats++;
            }

            if ( pgnoNull != (PGNO) plrmerge->le_pgnoLeft )
            {
                SetLogCsvTypeSz( szLogRecordPgChangeInfo );
                SetLogCsvChangeInfo( 0, ulCkSum, (PGNO) plrmerge->le_pgnoLeft, (OBJID) plrmerge->le_objidFDP,
                    (DBID) plrmerge->dbid, (DBTIME) plrmerge->le_dbtimeLeftBefore, (DBTIME) plrmerge->le_dbtime );
                cLogRecordsCsvFormats++;
            }

            eProcessed = eConsumed;
        }
            break;

        case lrtypPageMove:
        {
            const LRPAGEMOVE * const plrpagemove = LRPAGEMOVE::PlrpagemoveFromLr( plr );
            
            const size_t cb = plrpagemove->CbTotal();

            const ULONG ulCkSum = UlChecksumDataLR( plrpagemove, cb );

            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo(
                0,
                ulCkSum,
                plrpagemove->PgnoSource(),
                plrpagemove->ObjidFDP(),
                plrpagemove->Dbid(),
                plrpagemove->DbtimeSourceBefore(),
                plrpagemove->DbtimeAfter() );
            cLogRecordsCsvFormats++;

            Assert( pgnoNull != plrpagemove->PgnoDest() );
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo(
                0,
                ulCkSum,
                plrpagemove->PgnoDest(),
                plrpagemove->ObjidFDP(),
                plrpagemove->Dbid(),
                dbtimeNil,
                plrpagemove->DbtimeAfter());
            cLogRecordsCsvFormats++;

            if ( pgnoNull != plrpagemove->PgnoParent() )
            {
                SetLogCsvTypeSz( szLogRecordPgChangeInfo );
                SetLogCsvChangeInfo(
                    0,
                    ulCkSum,
                    plrpagemove->PgnoParent(),
                    plrpagemove->ObjidFDP(),
                    plrpagemove->Dbid(),
                    plrpagemove->DbtimeParentBefore(),
                    plrpagemove->DbtimeAfter());
                cLogRecordsCsvFormats++;
            }

            if( pgnoNull != plrpagemove->PgnoLeft() )
            {
                SetLogCsvTypeSz( szLogRecordPgChangeInfo );
                SetLogCsvChangeInfo(
                    0,
                    ulCkSum,
                    plrpagemove->PgnoLeft(),
                    plrpagemove->ObjidFDP(),
                    plrpagemove->Dbid(),
                    plrpagemove->DbtimeLeftBefore(),
                    plrpagemove->DbtimeAfter());
                cLogRecordsCsvFormats++;
            }

            if( pgnoNull != plrpagemove->PgnoRight() )
            {
                SetLogCsvTypeSz( szLogRecordPgChangeInfo );
                SetLogCsvChangeInfo(
                    0,
                    ulCkSum,
                    plrpagemove->PgnoRight(),
                    plrpagemove->ObjidFDP(),
                    plrpagemove->Dbid(),
                    plrpagemove->DbtimeRightBefore(),
                    plrpagemove->DbtimeAfter());
                cLogRecordsCsvFormats++;
            }
            
            eProcessed = eConsumed;
        }
            break;

        case lrtypRootPageMove:
        {
            const LRROOTPAGEMOVE * const plrrpm = (LRROOTPAGEMOVE *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrrpm ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypPagePatchRequest:
        {
            const LRPAGEPATCHREQUEST * const plrpagepatchrequest = (LRPAGEPATCHREQUEST *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrpagepatchrequest ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypEmptyTree:
        {
            const LREMPTYTREE * const plremptytree  = (LREMPTYTREE *)plr;
            const BYTE * pb = plremptytree->rgb;
            ULONG  cb = plremptytree->CbEmptyPageList();
            Assert( pb == ((BYTE*)plremptytree)+sizeof(*plremptytree) );

            ULONG ulChkSum = UlChecksumDataLR( plremptytree, cb );
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, ulChkSum, (PGNO) plremptytree->le_pgno, (OBJID) plremptytree->le_objidFDP,
                (DBID) plremptytree->dbid, (DBTIME) plremptytree->le_dbtimeBefore, (DBTIME) plremptytree->le_dbtime);
                cLogRecordsCsvFormats++;

            const EMPTYPAGE * const pEmptyPageEntries = (EMPTYPAGE *) plremptytree->rgb;
            const CPG           cpgToFree               = plremptytree->CbEmptyPageList() / sizeof(EMPTYPAGE);
            Assert( plremptytree->CbEmptyPageList() % sizeof(EMPTYPAGE) == 0 );
            
            for ( INT i = 0; i < cpgToFree; i++ )
            {
                SetLogCsvTypeSz( szLogRecordPgChangeInfo );
                SetLogCsvChangeInfo( 0, ulChkSum, (PGNO) pEmptyPageEntries[i].pgno, (OBJID) plremptytree->le_objidFDP,
                    (DBID) plremptytree->dbid, (DBTIME) pEmptyPageEntries[i].dbtimeBefore, (DBTIME) plremptytree->le_dbtime);
                cLogRecordsCsvFormats++;
                
            }
            eProcessed = eConsumed;
        }
            break;

        case lrtypSetExternalHeader:
        {
            const LRSETEXTERNALHEADER * const   plrsetexternalheader = (LRSETEXTERNALHEADER *)plr;
            const INT                           cb  = plrsetexternalheader->CbData();
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, UlChecksumDataLR( plrsetexternalheader, cb ), (PGNO) plrsetexternalheader->le_pgno,
                (OBJID) plrsetexternalheader->le_objidFDP, (DBID) plrsetexternalheader->dbid,
                (DBTIME) plrsetexternalheader->le_dbtimeBefore, (DBTIME) plrsetexternalheader->le_dbtime );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypUndo:
        {
            const LRUNDO * const plrundo = (LRUNDO *)plr;
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo( 0, UlChecksumSimpleLR( plrundo ), (PGNO) plrundo->le_pgno, (OBJID) plrundo->le_objidFDP,
                (DBID) plrundo->dbid, (DBTIME) plrundo->le_dbtimeBefore, (DBTIME) plrundo->le_dbtime );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypScrub:
        {
            const LRSCRUB * const plrscrub = (LRSCRUB *)plr;
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo(
                0,
                UlChecksumDataLR( plrscrub, plrscrub->CbData() ),
                (PGNO) plrscrub->le_pgno,
                (OBJID) plrscrub->le_objidFDP,
                (DBID) plrscrub->dbid,
                (DBTIME) plrscrub->le_dbtimeBefore,
                (DBTIME) plrscrub->le_dbtime );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypInit:
        case lrtypInit2:
        {
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            if ( lrtypInit == plr->lrtyp )
            {
                const LRINIT * const plrinit = (LRINIT*)plr;
                SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrinit ) );
            }
            else
            {
                const LRINIT2 * const plrinit = (LRINIT2 *)plr;
                SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrinit ) );
            }
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypTerm:
        case lrtypShutDownMark:
        case lrtypRecoveryUndo:
        {
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plr ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypTerm2:
        {
            const LRTERMREC2 * const plrtermrec2 = (LRTERMREC2*)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrtermrec2 ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypRecoveryUndo2:
        {
            const LRRECOVERYUNDO2 * const plrrcvundo2 = (LRRECOVERYUNDO2 *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrrcvundo2 ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypRecoveryQuit:
        case lrtypRecoveryQuit2:
        case lrtypRecoveryQuitKeepAttachments:
        {
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            if ( lrtypRecoveryQuit == plr->lrtyp )
            {
                const LRTERMREC * const plrquit = (LRTERMREC *) plr;
                SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrquit ) );
            }
            else
            {
                const LRTERMREC2 * const plrquit = (LRTERMREC2 *) plr;
                SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrquit ) );
            }
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;


        case lrtypJetOp:
        {
            const LRJETOP * const plrjetop = (LRJETOP *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrjetop ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypFullBackup:
        case lrtypIncBackup:
        {
            const LRLOGRESTORE * const plrlr = (LRLOGRESTORE *) plr;

            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );

            if ( plrlr->le_cbPath )
            {
                SetLogCsvChecksumInfo( 0, UlChecksumDataLR( plrlr, plrlr->le_cbPath ) );
            }
            else
            {
                SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrlr ) );
            }
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;


        case lrtypTrace:
        case lrtypForceLogRollover:
        {
            const LRTRACE * const plrtrace = (LRTRACE *)plr;
            const ULONG cb = strlen( plrtrace->sz );
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumDataLR( plrtrace, cb ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypBackup:
        {
            const LRLOGBACKUP_OBSOLETE * const plrbackup = (LRLOGBACKUP_OBSOLETE *) plr;

            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );

            if ( plrbackup->le_cbPath )
            {
                SetLogCsvChecksumInfo( 0, UlChecksumDataLR( plrbackup, plrbackup->le_cbPath ) );
            }
            else
            {
                SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrbackup ) );
            }
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypBackup2:
        {
            const LRLOGBACKUP * const plrbackup2 = (LRLOGBACKUP *) plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            if ( plrbackup2->le_cbPath )
            {
                SetLogCsvChecksumInfo( 0, UlChecksumDataLR( plrbackup2, plrbackup2->le_cbPath ) );
            }
            else
            {
                SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrbackup2 ) );
            }
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
        case lrtypChecksum:
        {
            const LRCHECKSUM * const plrck = static_cast< const LRCHECKSUM* const >( plr );

            Assert( plrck->bUseShortChecksum == bShortChecksumOn ||
                    plrck->bUseShortChecksum == bShortChecksumOff );

            SetLogCsvTypeSz( szLogRecordChecksumInfo );
            
            if ( plrck->bUseShortChecksum == bShortChecksumOn )
            {
                SetLogCsvChecksumInfo( plrck->le_ulShortChecksum, plrck->le_ulChecksum );
            }
            else if ( plrck->bUseShortChecksum == bShortChecksumOff )
            {
                SetLogCsvChecksumInfo( 0, plrck->le_ulChecksum );
            }
            else
            {
                SetLogCsvChecksumInfo( 0, 0 );
                Call( ErrERRCheck( JET_errLogFileCorrupt ) );
            }
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;

        }
            break;
#endif

        case lrtypExtRestore:
        {
            const LREXTRESTORE * const plrextrestore = (LREXTRESTORE *) plr;
            const CHAR * sz = reinterpret_cast<const CHAR *>( plrextrestore ) + sizeof(LREXTRESTORE);
            ULONG cb = 0;
            cb += strlen(sz) +1;
            sz = sz + strlen( sz ) + 1;
            cb += strlen(sz) +1;
            sz = sz + strlen( sz ) + 1;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumDataLR( plrextrestore, cb ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypExtRestore2:
        {
            const LREXTRESTORE * const plrextrestore = (LREXTRESTORE *) plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            if ( plrextrestore->CbInfo() )
            {
                SetLogCsvChecksumInfo( 0, UlChecksumDataLR( plrextrestore, plrextrestore->CbInfo() ) );
            }
            else
            {
                SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrextrestore ) );
            }
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypMacroInfo:
        {
            const LRIGNORED * const plrIgnored = (LRIGNORED *) plr;
            const LRMACROINFO * const plrmacroinfo = (LRMACROINFO *) plr;
            Assert( plrmacroinfo->CountOfPgno() >= 2 );

            for ( CPG ipgno = 0; ipgno < plrmacroinfo->CountOfPgno(); ipgno++ )
            {
                Assert( plrmacroinfo->GetPgno( ipgno ) != 0 );
                SetLogCsvTypeSz( szLogRecordPgChangeInfo );

                ULONG ulChkSum = UlChecksumDataLR( plrIgnored, plrIgnored->Cb() );

                SetLogCsvChangeInfo(
                    0,
                    ulChkSum,
                    plrmacroinfo->GetPgno( ipgno ),
                    0,
                    1,
                    0,
                    0 );
                cLogRecordsCsvFormats++;
            }
            
            eProcessed = eConsumed;
        }
            break;

        case lrtypExtendDB:
        {
            const LREXTENDDB * const plrextenddb = (LREXTENDDB *)plr;
            SetLogCsvTypeSz( szLogRecordResizeDatabaseInfo );
            SetLogCsvResizeInfo( 0, UlChecksumSimpleLR( plrextenddb ), plrextenddb->Dbid() );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypCommitCtx:
        {
            const LRCOMMITCTX * const plrcommitctx = (LRCOMMITCTX *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumDataLR( plrcommitctx, plrcommitctx->CbCommitCtx() ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypScanCheck:
        {
            const LRSCANCHECK * const plrscancheck = (LRSCANCHECK *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvResizeInfo( 0, UlChecksumSimpleLR( plrscancheck ), plrscancheck->Dbid() );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypScanCheck2:
        {
            const LRSCANCHECK2 * const plrscancheck = (LRSCANCHECK2 *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvResizeInfo( 0, UlChecksumSimpleLR( plrscancheck ), plrscancheck->Dbid() );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypReAttach:
        {
            const LRREATTACHDB * const plrreattach = (LRREATTACHDB *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvResizeInfo( 0, UlChecksumSimpleLR( plrreattach ), plrreattach->le_dbid );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypMacroInfo2:
        {
            const LRIGNORED * const plrIgnored = (LRIGNORED *)plr;
            const LRMACROINFO2 * const plrmacroinfo2 = (LRMACROINFO2 *)plr;
            Assert( plrmacroinfo2->CountOfPgno() >= 2 );

            for ( CPG ipgno = 0; ipgno < plrmacroinfo2->CountOfPgno(); ipgno++ )
            {
                Assert( plrmacroinfo2->GetPgno( ipgno ) != 0 );
                SetLogCsvTypeSz( szLogRecordPgChangeInfo );

                ULONG ulChkSum = UlChecksumDataLR( plrIgnored, plrIgnored->Cb() );

                SetLogCsvChangeInfo(
                    0,
                    ulChkSum,
                    plrmacroinfo2->GetPgno( ipgno ),
                    0,
                    plrmacroinfo2->Dbid(),
                    0,
                    0 );
                cLogRecordsCsvFormats++;
            }

            eProcessed = eConsumed;
        }
            break;

        case lrtypFreeFDP:
        {
            const LRFREEFDP * const plrfreefdp = (LRFREEFDP *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvResizeInfo( 0, UlChecksumSimpleLR( plrfreefdp ), plrfreefdp->le_dbid );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypNOP2:
        case lrtypIgnored9:
        case lrtypIgnored10:
        case lrtypIgnored11:
        case lrtypIgnored12:
        case lrtypIgnored13:
        case lrtypIgnored14:
        case lrtypIgnored15:
        case lrtypIgnored16:
        case lrtypIgnored17:
        case lrtypIgnored18:
        case lrtypIgnored19:
        {
            const LRIGNORED * const plrignoreddata = (LRIGNORED *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumDataLR( plrignoreddata, plrignoreddata->Cb() ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypShrinkDB:
        {
            const LRSHRINKDB * const plrshrinkdb = (LRSHRINKDB *)plr;
            SetLogCsvTypeSz( szLogRecordResizeDatabaseInfo );
            SetLogCsvResizeInfo( 0, UlChecksumSimpleLR( plrshrinkdb ), plrshrinkdb->Dbid() );

            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypShrinkDB2:
        case lrtypShrinkDB3:
        {
            const LRSHRINKDB3 lrshrinkdb( (LRSHRINKDB *)plr );
            const ULONG ulCkSum = UlChecksumSimpleLR( &lrshrinkdb );
            const PGNO pgnoShrunkFirst = lrshrinkdb.PgnoLast() + 1;
            const PGNO pgnoShrunkLast = lrshrinkdb.PgnoLast() + lrshrinkdb.CpgShrunk();

            for ( PGNO pgno = pgnoShrunkFirst; pgno <= pgnoShrunkLast; pgno++ )
            {
                SetLogCsvTypeSz( szLogRecordPgChangeInfo );
                SetLogCsvChangeInfo(
                    0,
                    ulCkSum,
                    pgno,
                    objidNil,
                    lrshrinkdb.Dbid(),
                    dbtimeNil,
                    lrshrinkdb.Dbtime() );
                cLogRecordsCsvFormats++;
            }

            eProcessed = eConsumed;
        }
            break;

        case lrtypTrimDB:
        {
            const LRTRIMDB * const plrtrimdb = (LRTRIMDB *)plr;
            SetLogCsvTypeSz( szLogRecordTrimDatabaseInfo );
            SetLogCsvResizeInfo( 0, UlChecksumSimpleLR( plrtrimdb ), plrtrimdb->Dbid() );

            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypSetDbVersion:
        {
            const LRSETDBVERSION * const plrSetDbVersion = (LRSETDBVERSION *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvResizeInfo( 0, UlChecksumSimpleLR( plrSetDbVersion ), plrSetDbVersion->Dbid() );

            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypNewPage:
        {
            const LRNEWPAGE * const plrnewpage = (LRNEWPAGE *)plr;
            SetLogCsvTypeSz( szLogRecordPgChangeInfo );
            SetLogCsvChangeInfo(
                0,
                UlChecksumSimpleLR( plrnewpage ),
                plrnewpage->Pgno(),
                plrnewpage->Objid(),
                plrnewpage->Dbid(),
                dbtimeNil,
                plrnewpage->DbtimeAfter() );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypSignalAttachDb:
        {
            const LRSIGNALATTACHDB * const plrsattachdb = (LRSIGNALATTACHDB *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvChecksumInfo( 0, UlChecksumSimpleLR( plrsattachdb ) );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        case lrtypExtentFreed:
        {
            const LREXTENTFREED * const plrextentfreed = (LREXTENTFREED *)plr;
            SetLogCsvTypeSz( szLogRecordMiscelLrInfo );
            SetLogCsvResizeInfo( 0, UlChecksumSimpleLR( plrextentfreed ), plrextentfreed->Dbid() );
            cLogRecordsCsvFormats++;
            eProcessed = eConsumed;
        }
            break;

        default:
            AssertSzRTL( fFalse, "Unknown LR = %d, lgpos = %s.", (ULONG)plr->lrtyp, szLgposLR );
            Call( ErrERRCheck( JET_errLogFileCorrupt ) );
            break;
    }


    if ( eProcessed == eConsumed )
    {
        Assert( cLogRecordsCsvFormats > 0 );

        Expected( cbLR < g_cbPageMax + 1024 );

        for (ULONG i = 0; i < cLogRecordsCsvFormats; i++)
        {
            WCHAR   szLR[ 512 ];
            WCHAR   rgwchBuf[ 512 ];

            Assert( pLogRecordsCsvFormats[i] != NULL );

            szLR[0] = L'\0';
            OSStrCbAppendW( szLR, sizeof(szLR), pLogRecordsCsvFormats[i] );

            
            if ( pLogRecordsCsvFormats[i] == szLogRecordChecksumInfo )
            {
                OSStrCbFormatW( rgwchBuf, sizeof(rgwchBuf),
                                L"%hs, %4.4X, %8.8X%8.8X",
                                szLgposLR, cbLR,
                                pChangeInfo[i].ulChecksum1, pChangeInfo[i].ulChecksum2 );
                OSStrCbAppendW( szLR, sizeof(szLR), rgwchBuf );
                (*pcwpfCsvOut)( L"%s", szLR );
            }
            else if ( pLogRecordsCsvFormats[i] == szLogRecordDatabaseInfo )
            {
                Assert( i == 0 );
                WCHAR rgwchSignBuf[3 * sizeof( pChangeInfo[i].signDb ) + 1];
                for ( INT j = 0; j < sizeof( pChangeInfo[i].signDb ); j++ )
                {
                    OSStrCbFormatW( rgwchSignBuf + j * 3, sizeof( rgwchSignBuf ) - j * 3 * sizeof(WCHAR), L"%02X ", ( (BYTE*)&pChangeInfo[i].signDb )[j] );
                }
                rgwchSignBuf[3 * sizeof( pChangeInfo[i].signDb )] = L'\0';
                OSStrCbFormatW( rgwchBuf, sizeof( rgwchBuf ),
                                L"%hs, %4.4X, %8.8X%8.8X, %hs, %d, \"%s\", %s",
                                szLgposLR, cbLR,
                                pChangeInfo[i].ulChecksum1, pChangeInfo[i].ulChecksum2,
                                szLRTyp, pChangeInfo[i].dbid, pChangeInfo[i].szDbPath, rgwchSignBuf );
                OSStrCbAppendW( szLR, sizeof(szLR), rgwchBuf );
                (*pcwpfCsvOut)( L"%s", szLR );
            }
            else if ( pLogRecordsCsvFormats[i] == szLogRecordPgChangeInfo )
            {
                OSStrCbFormatW( rgwchBuf, sizeof(rgwchBuf),
                                L"%hs, %4.4X, %8.8X%8.8X, %hs, %8.8X, %8.8X, %8.8X, %16.16I64X, %16.16I64X",
                                szLgposLR, cbLR,
                                pChangeInfo[i].ulChecksum1, pChangeInfo[i].ulChecksum2,
                                szLRTyp, pChangeInfo[i].pgno, pChangeInfo[i].objid, pChangeInfo[i].dbid,
                                pChangeInfo[i].dbtimePre, pChangeInfo[i].dbtimePost );
                OSStrCbAppendW( szLR, sizeof(szLR), rgwchBuf );
                (*pcwpfCsvOut)( L"%s", szLR );

            }
            else if ( pLogRecordsCsvFormats[i] == szLogRecordMiscelLrInfo )
            {
                OSStrCbFormatW( rgwchBuf, sizeof(rgwchBuf),
                                L"%hs, %4.4X, %8.8X%8.8X, %hs",
                                szLgposLR, cbLR,
                                pChangeInfo[i].ulChecksum1, pChangeInfo[i].ulChecksum2,
                                szLRTyp );
                OSStrCbAppendW( szLR, sizeof(szLR), rgwchBuf );
                (*pcwpfCsvOut)( L"%s", szLR );
            }
            else if ( pLogRecordsCsvFormats[i] == szLogRecordResizeDatabaseInfo ||
                      pLogRecordsCsvFormats[i] == szLogRecordTrimDatabaseInfo )
            {
                OSStrCbFormatW( rgwchBuf, sizeof(rgwchBuf),
                                L"%hs, %4.4X, %8.8X%8.8X, %hs, %d",
                                szLgposLR, cbLR,
                                pChangeInfo[i].ulChecksum1, pChangeInfo[i].ulChecksum2,
                                szLRTyp, pChangeInfo[i].dbid );
                OSStrCbAppendW( szLR, sizeof(szLR), rgwchBuf );
                (*pcwpfCsvOut)( L"%s", szLR );
            }
            else
            {
                AssertSz( fFalse, "Unknown CSV type!!!" );
            }
        
            (*pcwpfCsvOut)( L"\r\n" );
            cbLR = 0;
        }

    }
    else
    {
        Assert( eProcessed == eNopIgnored );
        Assert( plr->lrtyp == lrtypNOP );
        Assert( cLogRecordsCsvFormats == 0 );
    }

HandleError:

    if ( pLogRecordsCsvFormats != rgszLogRecordsCsvFormats )
    {
        free ( pLogRecordsCsvFormats );
        pLogRecordsCsvFormats = NULL;
    }
    if ( pChangeInfo != rgChangeInfo )
    {
        free( pChangeInfo );
        pChangeInfo = NULL;
    }

    return(err);
}

#ifdef DEBUG
extern const CHAR * const mpopsz[];
#endif

VOID SPrintSign( const SIGNATURE * const psign, UINT cbSz, __out_bcount(cbSz) PSTR  sz )
{
    LOGTIME tm = psign->logtimeCreate;
    OSStrCbFormatA( sz, cbSz, "Create time:%02d/%02d/%04d %02d:%02d:%02d.%3.3d Rand:%lu Computer:%s",
                        (SHORT) tm.bMonth,
                        (SHORT) tm.bDay,
                        (SHORT) tm.bYear + 1900,
                        (SHORT) tm.bHours,
                        (SHORT) tm.bMinutes,
                        (SHORT) tm.bSeconds,
                        (SHORT) tm.Milliseconds(),
                        (ULONG) psign->le_ulRandom,
                        0 != psign->szComputerName[0] ? psign->szComputerName : "<None>" );
}

const BYTE mpbb[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8',
                     '9', 'A', 'B', 'C', 'D', 'E', 'F' };

LOCAL VOID FullDataToSz(
    _In_reads_( cbData ) const BYTE *pbData,
    UINT cbData,
    UINT cbDataMax,
    UINT iVerbosityLevel,
    _Out_writes_z_( cbBuf ) PSTR sz,
    UINT cbBuf )
{
    Assert( cbBuf > 0 );
    Assert( iVerbosityLevel != LOG::ldvlBasic );

    if ( iVerbosityLevel >= LOG::ldvlData )
    {
        const BYTE  *pb;
        const BYTE  *pbMax;
        BYTE    *pbPrint = reinterpret_cast<BYTE *>( sz );
        const UINT cbPrintMax = min( cbData, min( cbDataMax, ( cbBuf - 1 ) / 3 ) );

        pbMax = pbData + cbPrintMax;

        for( pb = pbData; pb < pbMax; pb++ )
        {
            BYTE b = *pb;

            *pbPrint++ = mpbb[b >> 4];
            *pbPrint++ = mpbb[b & 0x0f];
            *pbPrint++ = ' ';

        }

        *pbPrint = '\0';
    }
    else if ( iVerbosityLevel >= LOG::ldvlStructure )
    {
        const CHAR szObfuscation[] = "<Data>";
        if ( cbBuf >= sizeof( szObfuscation ) )
        {
            OSStrCbCopyA( sz, cbBuf, szObfuscation );
        }
        else
        {
            sz[0] = '\0';
        }
    }
}

LOCAL VOID DataToSz(
    _In_reads_( cbData ) const BYTE *pbData,
    INT cbData,
    UINT iVerbosityLevel,
    _Out_writes_z_( cbBuf ) PSTR sz,
    UINT cbBuf )
{
    FullDataToSz( pbData, cbData, cbRawDataMax, iVerbosityLevel, sz, cbBuf );
}


VOID ShowData(
    _In_reads_( cbData ) const BYTE *pbData,
    _In_range_( 0, ( cbFormattedDataMax - 1 ) / 3 ) INT cbData,
    UINT iVerbosityLevel,
    CPRINTF * const pcprintf )
{
    CHAR    rgchPrint[cbFormattedDataMax];
    DataToSz( pbData, cbData, iVerbosityLevel, rgchPrint, cbFormattedDataMax );
    (*pcprintf)( "%s", rgchPrint );
}




const INT   cbLRBuf = 1024 + cbFormattedDataMax;

VOID ShowLR( const LR *plr, LOG * plog )
{
    CHAR    rgchBuf[cbLRBuf];

    LrToSz( plr, rgchBuf, sizeof(rgchBuf), plog );
    (*CPRINTFSTDOUT::PcprintfInstance())( "%s\n", rgchBuf );

    if ( lrtypReplaceD == plr->lrtyp )
    {
        LGDumpDiff( plog, plr, CPRINTFSTDOUT::PcprintfInstance(), 20 );
    }
}

VOID CheckEndOfNOPList( const LR *plr, LOG *plog )
{
    const ULONG cNOP    = plog->GetNOP();

    Assert( cNOP > 0 );
    if ( NULL == plr || lrtypNOP != plr->lrtyp )
    {
        if ( cNOP > 1 )
        {
            (*CPRINTFSTDOUT::PcprintfInstance())( ">                 %s (Total NOPs: %d)\n", szNOPEndOfList, cNOP );;
        }
        plog->SetNOP(0);
    }
    else
    {
        plog->IncNOP();
    }
}

ERR ErrDownConvertName(
    __in_z const WCHAR * const wszName,
    __out_bcount_z( cbName ) CHAR *     szName,
    ULONG   cbName )
{
    const ULONG cchName = cbName / sizeof(CHAR);
    ERR err = ErrOSSTRUnicodeToAscii( wszName, szName, cchName );
    if ( JET_errUnicodeTranslationFail == err )
    {
        err = ErrOSSTRUnicodeToAscii( wszName, szName, cchName, NULL, OSSTR_ALLOW_LOSSY );
        Assert( err == JET_errSuccess );
        CallR( err );
        err = wrnLossy;
    }

    return err;
}


BOOL FLGVersionNewCommitCtx( const LGFILEHDR_FIXED* const plgfilehdr )
{
    if ( plgfilehdr->le_ulUpdateMajor > ulLGVersionUpdateMajor_NewCommitCtx1 ) return fTrue;
    if ( plgfilehdr->le_ulUpdateMajor == ulLGVersionUpdateMajor_NewCommitCtx1 )
    {
        if ( plgfilehdr->le_ulUpdateMinor >= ulLGVersionUpdateMinor_NewCommitCtx1 ) return fTrue;
        return fFalse;
    }
    if ( plgfilehdr->le_ulUpdateMajor == ulLGVersionUpdateMajor_NewCommitCtx2 )
    {
        if ( plgfilehdr->le_ulUpdateMinor >= ulLGVersionUpdateMinor_NewCommitCtx2 ) return fTrue;
        return fFalse;
    }

    return fFalse;
}

VOID LrToSz(
    const LR *plr,
    __out_bcount(cbLR) PSTR szLR,
    ULONG cbLR,
    LOG * plog )
{
    LRTYP   lrtyp;
    CHAR    rgchBuf[cbLRBuf];
    const UINT iVerbosityLevel = ( plog != NULL ) ? plog->IDumpVerbosityLevel() : LOG::ldvlMax;
#ifndef DEBUGGER_EXTENSION
    Assert( plog != NULL );
#endif

    char const *szUnique        = "U";
    char const *szNotUnique     = "NU";


    char const *szVersion       = "V";
    char const *szNoVersion     = "NV";

    char const *szDeleted       = "D";
    char const *szNotDeleted    = "ND";

    char const *szEmptyPage     = "EP";
    char const *szNotEmptyPage  = "NEP";

    char const *szKeyChange     = "KC";
    char const *szNoKeyChange   = "NKC";

    char const *szConcCI        = "CI";
    char const *szNotConcCI     = "NCI";

    char const *szSpaceTree     = "SP";
    char const *szNotSpaceTree  = "NSP";

    char const *szSystemTask    = "SYS";
    char const *szNotSystemTask = "USR";

    char const *rgszMergeType[] = { "None", "EmptyPage", "FullRight", "PartialRight", "EmptyTree", "FullLeft", "PartialLeft", "PageMove" };

    char const *szLossyUnicodePath  = " - LOST UNICODE CHARACTERS UNPRINTED";

    if ( plr->lrtyp >= lrtypMax )
        lrtyp = lrtypMax;
    else
        lrtyp = plr->lrtyp;

    if ( !plog || plog->GetNOP() == 0 || lrtyp != lrtypNOP )
    {
        OSStrCbFormatA( szLR, cbLR, " %s", SzLrtyp( lrtyp ) );
    }

    switch ( plr->lrtyp )
    {
        case lrtypNOP:
            if( plog )
            {
                Assert( 0 == plog->GetNOP() );
                plog->IncNOP();
            }
            break;

        case lrtypMS:
        {
            LRMS *plrms = (LRMS *)plr;

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%X,%X checksum %u)",
                (USHORT) plrms->le_isecForwardLink,
                (USHORT) plrms->le_ibForwardLink,
                (ULONG) plrms->le_ulCheckSum );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypInsert:
        {
            LRINSERT    *plrinsert = (LRINSERT *)plr;
            BYTE        *pb;
            ULONG       cb;

            pb = (BYTE *) plr + sizeof( LRINSERT );
            cb = plrinsert->CbSuffix() + plrinsert->CbPrefix() + plrinsert->CbData();

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s:%s:%s:%s:%s,%u,%u,%u,rceid:%lu,objid:[%u:%lu])",
                    (DBTIME) plrinsert->le_dbtime,
                    (DBTIME) plrinsert->le_dbtimeBefore,
                    (TRX) plrinsert->le_trxBegin0,
                    (USHORT) plrinsert->level,
                    (PROCID) plrinsert->le_procid,
                    (USHORT) plrinsert->dbid,
                    (PGNO) plrinsert->le_pgno,
                    plrinsert->ILine(),
                    plrinsert->FUnique() ? szUnique : szNotUnique,
                    plrinsert->FVersioned() ? szVersion : szNoVersion,
                    plrinsert->FDeleted() ? szDeleted : szNotDeleted,
                    plrinsert->FSpace() ? szSpaceTree : szNotSpaceTree,
                    plrinsert->FSystemTask() ? szSystemTask : szNotSystemTask,
                    plrinsert->FConcCI() ? szConcCI : szNotConcCI,
                    plrinsert->CbPrefix(),
                    plrinsert->CbSuffix(),
                    plrinsert->CbData(),
                    (RCEID) plrinsert->le_rceid,
                    (USHORT) plrinsert->dbid,
                    (OBJID) plrinsert->le_objidFDP );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            DataToSz( pb, cb, plog->IDumpVerbosityLevel(), rgchBuf, cbLRBuf );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypFlagInsert:
        {
            LRFLAGINSERT    *plrflaginsert = (LRFLAGINSERT *)plr;

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s:%s:%s:%s:%s,rceid:%lu,objid:[%u:%lu])",
                    (DBTIME) plrflaginsert->le_dbtime,
                    (DBTIME) plrflaginsert->le_dbtimeBefore,
                    (TRX) plrflaginsert->le_trxBegin0,
                    (USHORT) plrflaginsert->level,
                    (PROCID) plrflaginsert->le_procid,
                    (USHORT) plrflaginsert->dbid,
                    (PGNO) plrflaginsert->le_pgno,
                    plrflaginsert->ILine(),
                    plrflaginsert->FUnique() ? szUnique : szNotUnique,
                    plrflaginsert->FVersioned() ? szVersion : szNoVersion,
                    plrflaginsert->FDeleted() ? szDeleted : szNotDeleted,
                    plrflaginsert->FSpace() ? szSpaceTree : szNotSpaceTree,
                    plrflaginsert->FSystemTask() ? szSystemTask : szNotSystemTask,
                    plrflaginsert->FConcCI() ? szConcCI : szNotConcCI,
                    (RCEID) plrflaginsert->le_rceid,
                    (USHORT) plrflaginsert->dbid,
                    (OBJID) plrflaginsert->le_objidFDP );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            break;
        }

        case lrtypReplace:
        case lrtypReplaceD:
        {
            LRREPLACE *plrreplace = (LRREPLACE *)plr;
            BYTE    *pb;
            USHORT  cb;

            pb = (BYTE *) plrreplace + sizeof( LRREPLACE );
            cb = plrreplace->Cb();

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s:%s:%s:%s,%5u,%5u,%5u,rceid:%lu,objid:[%u:%lu])",
                (DBTIME) plrreplace->le_dbtime,
                (DBTIME) plrreplace->le_dbtimeBefore,
                (TRX) plrreplace->le_trxBegin0,
                (USHORT) plrreplace->level,
                (PROCID) plrreplace->le_procid,
                (USHORT) plrreplace->dbid,
                (PGNO) plrreplace->le_pgno,
                plrreplace->ILine(),
                plrreplace->FUnique() ? szUnique : szNotUnique,
                plrreplace->FVersioned() ? szVersion : szNoVersion,
                plrreplace->FSpace() ? szSpaceTree : szNotSpaceTree,
                plrreplace->FSystemTask() ? szSystemTask : szNotSystemTask,
                plrreplace->FConcCI() ? szConcCI : szNotConcCI,
                cb,
                plrreplace->CbNewData(),
                plrreplace->CbOldData(),
                (RCEID) plrreplace->le_rceid,
                (USHORT) plrreplace->dbid,
                (OBJID) plrreplace->le_objidFDP );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            if ( lrtypReplace == plr->lrtyp )
            {
                DataToSz( pb, cb, plog->IDumpVerbosityLevel(), rgchBuf, cbLRBuf );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            else
            {
                NULL;
            }
            break;
        }

        case lrtypFlagInsertAndReplaceData:
        {
            LRFLAGINSERTANDREPLACEDATA *plrfiard = (LRFLAGINSERTANDREPLACEDATA *)plr;
            BYTE    *pb;
            ULONG   cb;

            pb = (BYTE *) plrfiard + sizeof( LRFLAGINSERTANDREPLACEDATA );
            cb = plrfiard->CbKey() + plrfiard->CbData();

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s:%s:%s:%s,%5u,rceidInsert:%lu,rceidReplace:%lu,objid:[%u:%lu])",
                (DBTIME) plrfiard->le_dbtime,
                (DBTIME) plrfiard->le_dbtimeBefore,
                (TRX) plrfiard->le_trxBegin0,
                (USHORT) plrfiard->level,
                (PROCID) plrfiard->le_procid,
                (USHORT) plrfiard->dbid,
                (PGNO) plrfiard->le_pgno,
                plrfiard->ILine(),
                plrfiard->FUnique() ? szUnique : szNotUnique,
                plrfiard->FVersioned() ? szVersion : szNoVersion,
                plrfiard->FSpace() ? szSpaceTree : szNotSpaceTree,
                plrfiard->FSystemTask() ? szSystemTask : szNotSystemTask,
                plrfiard->FConcCI() ? szConcCI : szNotConcCI,
                cb,
                (RCEID) plrfiard->le_rceid,
                (RCEID) plrfiard->le_rceidReplace,
                (USHORT) plrfiard->dbid,
                (OBJID) plrfiard->le_objidFDP );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            DataToSz( pb, cb, plog->IDumpVerbosityLevel(), rgchBuf, cbLRBuf );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypFlagDelete:
        {
            LRFLAGDELETE *plrflagdelete = (LRFLAGDELETE *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s:%s:%s:%s),rceid:%lu,objid:[%u:%lu]",
                (DBTIME) plrflagdelete->le_dbtime,
                (DBTIME) plrflagdelete->le_dbtimeBefore,
                (TRX) plrflagdelete->le_trxBegin0,
                (USHORT) plrflagdelete->level,
                (PROCID) plrflagdelete->le_procid,
                (USHORT) plrflagdelete->dbid,
                (PGNO) plrflagdelete->le_pgno,
                plrflagdelete->ILine(),
                plrflagdelete->FUnique() ? szUnique : szNotUnique,
                plrflagdelete->FVersioned() ? szVersion : szNoVersion,
                plrflagdelete->FSpace() ? szSpaceTree : szNotSpaceTree,
                plrflagdelete->FSystemTask() ? szSystemTask : szNotSystemTask,
                plrflagdelete->FConcCI() ? szConcCI : szNotConcCI,
                (RCEID) plrflagdelete->le_rceid,
                (USHORT) plrflagdelete->dbid,
                (OBJID) plrflagdelete->le_objidFDP );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypDelete:
        {
            LRDELETE *plrdelete = (LRDELETE *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s:%s,objid:[%u:%lu])",
                (DBTIME) plrdelete->le_dbtime,
                (DBTIME) plrdelete->le_dbtimeBefore,
                (TRX) plrdelete->le_trxBegin0,
                (USHORT) plrdelete->level,
                (PROCID) plrdelete->le_procid,
                (USHORT) plrdelete->dbid,
                (PGNO) plrdelete->le_pgno,
                plrdelete->ILine(),
                plrdelete->FUnique() ? szUnique : szNotUnique,
                plrdelete->FSpace() ? szSpaceTree : szNotSpaceTree,
                plrdelete->FSystemTask() ? szSystemTask : szNotSystemTask,
                (USHORT) plrdelete->dbid,
                (OBJID) plrdelete->le_objidFDP );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypUndoInfo:
        {
            LRUNDOINFO *plrundoinfo = (LRUNDOINFO *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %lx:%u(%x,[%u:%lu],%s:%s,%u,%u,%u,rceid:%lu,objid:[%u:%lu])",
                (TRX) plrundoinfo->le_trxBegin0,
                (USHORT) plrundoinfo->level,
                (PROCID) plrundoinfo->le_procid,
                (USHORT) plrundoinfo->dbid,
                (PGNO) plrundoinfo->le_pgno,
                plrundoinfo->FUnique() ? szUnique : szNotUnique,
                plrundoinfo->FSystemTask() ? szSystemTask : szNotSystemTask,
                plrundoinfo->CbBookmarkKey(),
                plrundoinfo->CbBookmarkData(),
                (USHORT) plrundoinfo->le_cbData,
                (RCEID) plrundoinfo->le_rceid,
                (USHORT) plrundoinfo->dbid,
                (OBJID) plrundoinfo->le_objidFDP
                );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            DataToSz( plrundoinfo->rgbData, plrundoinfo->CbBookmarkKey() + plrundoinfo->CbBookmarkData() + plrundoinfo->le_cbData, plog->IDumpVerbosityLevel(), rgchBuf, cbLRBuf );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypDelta:
        {
            LRDELTA32 *plrdelta = (LRDELTA32 *)plr;

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s:%s:%s,%ld,offset:%d,rceid:%lu,objid:[%u:%lu])",
                (DBTIME) plrdelta->le_dbtime,
                (DBTIME) plrdelta->le_dbtimeBefore,
                (TRX) plrdelta->le_trxBegin0,
                (USHORT) plrdelta->level,
                (PROCID) plrdelta->le_procid,
                (USHORT) plrdelta->dbid,
                (PGNO) plrdelta->le_pgno,
                plrdelta->ILine(),
                plrdelta->FUnique() ? szUnique : szNotUnique,
                plrdelta->FVersioned() ? szVersion : szNoVersion,
                plrdelta->FSystemTask() ? szSystemTask : szNotSystemTask,
                plrdelta->FConcCI() ? szConcCI : szNotConcCI,
                plrdelta->Delta(),
                plrdelta->CbOffset(),
                (RCEID) plrdelta->le_rceid,
                (USHORT) plrdelta->dbid,
                (OBJID) plrdelta->le_objidFDP );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypDelta64:
        {
            LRDELTA64 *plrdelta = (LRDELTA64 *) plr;

            OSStrCbFormatA( rgchBuf, sizeof( rgchBuf ), " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s:%s:%s,%I64d,offset:%d,rceid:%lu,objid:[%u:%lu])",
                (DBTIME) plrdelta->le_dbtime,
                (DBTIME) plrdelta->le_dbtimeBefore,
                (TRX) plrdelta->le_trxBegin0,
                (USHORT) plrdelta->level,
                (PROCID) plrdelta->le_procid,
                (USHORT) plrdelta->dbid,
                (PGNO) plrdelta->le_pgno,
                plrdelta->ILine(),
                plrdelta->FUnique() ? szUnique : szNotUnique,
                plrdelta->FVersioned() ? szVersion : szNoVersion,
                plrdelta->FSystemTask() ? szSystemTask : szNotSystemTask,
                plrdelta->FConcCI() ? szConcCI : szNotConcCI,
                plrdelta->Delta(),
                plrdelta->CbOffset(),
                (RCEID) plrdelta->le_rceid,
                (USHORT) plrdelta->dbid,
                (OBJID) plrdelta->le_objidFDP );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypJetOp:
        {
            LRJETOP *plrjetop = (LRJETOP *)plr;
#ifdef DEBUG
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x) -- %s",
            (PROCID) plrjetop->le_procid,
            mpopsz[ plrjetop->op ] );
#else
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x) -- %d",
            (PROCID) plrjetop->le_procid,
            (ULONG)( plrjetop->op ) );
#endif
            break;
        }

        case lrtypBegin:
        case lrtypBegin0:
        {
            const LRBEGINDT     * const plrbeginDT  = (LRBEGINDT *)plr;
            Assert( plrbeginDT->levelBeginFrom >= 0 );
            Assert( plrbeginDT->clevelsToBegin <= levelMax );
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x,from:%d,to:%d)",
                (PROCID) plrbeginDT->le_procid,
                (SHORT) plrbeginDT->levelBeginFrom,
                (SHORT) ( plrbeginDT->levelBeginFrom + plrbeginDT->clevelsToBegin ) );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            if ( lrtypBegin != plr->lrtyp )
            {
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %lx", (TRX) plrbeginDT->le_trxBegin0 );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            break;
        }

        case lrtypMacroBegin:
        case lrtypMacroCommit:
        case lrtypMacroAbort:
        {
            LRMACROBEGIN *plrmbegin = (LRMACROBEGIN *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x) %I64x", (PROCID) plrmbegin->le_procid, (DBTIME) plrmbegin->le_dbtime );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypRefresh:
        {
            LRREFRESH *plrrefresh = (LRREFRESH *) plr;

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x,%lx)",
                (PROCID) plrrefresh->le_procid,
                (TRX) plrrefresh->le_trxBegin0 );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            break;
        }

        case lrtypCommit:
        case lrtypCommit0:
        {
            LRCOMMIT0 *plrcommit0 = (LRCOMMIT0 *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x,to:%d)",
                (PROCID) plrcommit0->le_procid,
                (USHORT) plrcommit0->levelCommitTo );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            if ( plr->lrtyp == lrtypCommit0 )
            {
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %lx", (TRX) plrcommit0->le_trxCommit0 );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            break;
        }

        case lrtypRollback:
        {
            LRROLLBACK *plrrollback = (LRROLLBACK *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x,%d)",
                (PROCID) plrrollback->le_procid,
                (USHORT) plrrollback->levelRollback );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypForceWriteLog:
        {
            const LRFORCEWRITELOG   * const plrForceWritelog    = (LRFORCEWRITELOG *)plr;

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x)", (PROCID)plrForceWritelog->le_procid );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypCreateDB:
        {
            LRCREATEDB *plrcreatedb = (LRCREATEDB *) plr;

            if ( plrcreatedb->FUnicodeNames() )
            {
                CAutoWSZPATH wszName;

                CallS( wszName.ErrSet( plrcreatedb->WszUnalignedNames() ) );
                
                CHAR szName[IFileSystemAPI::cchPathMax+1];

                ERR errT = ErrDownConvertName( (WCHAR*)wszName, szName, sizeof(szName) );
                Assert( errT >= JET_errSuccess );

                CHAR * szLossy = ( wrnLossy == errT ) ? szLossyUnicodePath : "";

                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x,%hs%hs,%u)", (PROCID)plrcreatedb->le_procid, szName, szLossy, (USHORT)plrcreatedb->dbid );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );

                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " Version: 0x%x,0x%x", plrcreatedb->UsVersion(), plrcreatedb->UsUpdateMajor() );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            else
            {
                CHAR *sz;

                sz = (CHAR *)plrcreatedb->SzNames();

                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x,%s,%u)", (PROCID)plrcreatedb->le_procid, sz, (USHORT)plrcreatedb->dbid );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );

                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " Version: 0x%x,0x%x", plrcreatedb->UsVersion(), plrcreatedb->UsUpdateMajor() );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " cpgMax: %lu Sig: ", (ULONG) plrcreatedb->le_cpgDatabaseSizeMax );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            SPrintSign( &plrcreatedb->signDb, sizeof(rgchBuf), rgchBuf );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " sparse: %d", plrcreatedb->FSparseEnabledFile() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypCreateDBFinish:
        {
            LRCREATEDBFINISH *plrCreateDBFinish = (LRCREATEDBFINISH *)plr;

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x,%u)", (PROCID)plrCreateDBFinish->le_procid, (USHORT)plrCreateDBFinish->dbid );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypAttachDB:
        {
            LRATTACHDB *plrattachdb = (LRATTACHDB *) plr;

            if ( plrattachdb->FUnicodeNames() )
            {
                CAutoWSZPATH wszName;

                CallS( wszName.ErrSet( plrattachdb->WszUnalignedNames() ) );

                CHAR szName[IFileSystemAPI::cchPathMax+1];

                ERR errT = ErrDownConvertName( (WCHAR*)wszName, szName, sizeof(szName) );
                Assert( errT >= JET_errSuccess );

                CHAR * szLossy = ( wrnLossy == errT ) ? szLossyUnicodePath : "";

                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x,%hs%hs,%u)", (PROCID)plrattachdb->le_procid, szName, szLossy, (USHORT) plrattachdb->dbid );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            else
            {
                CHAR *sz;

                sz = reinterpret_cast<CHAR *>( plrattachdb ) + sizeof(LRATTACHDB);
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x,%s,%u)", (PROCID)plrattachdb->le_procid, sz, (USHORT) plrattachdb->dbid );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " cpgMax: %u", (ULONG) plrattachdb->le_cpgDatabaseSizeMax );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " consistent:(%X,%X,%X)   SigDb: ",
                (LONG) plrattachdb->lgposConsistent.le_lGeneration,
                (SHORT) plrattachdb->lgposConsistent.le_isec,
                (SHORT) plrattachdb->lgposConsistent.le_ib );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            SPrintSign( &plrattachdb->signDb, sizeof(rgchBuf), rgchBuf );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), "   SigLog: " );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            SPrintSign( &plrattachdb->signLog, sizeof(rgchBuf), rgchBuf );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " sparse: %d", plrattachdb->FSparseEnabledFile() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypDetachDB:
        {
            LRDETACHDB *plrdetachdb = (LRDETACHDB *) plr;

            if ( plrdetachdb->FUnicodeNames() )
            {
                CAutoWSZPATH wszName;

                CallS( wszName.ErrSet( plrdetachdb->WszUnalignedNames() ) );

                CHAR szName[IFileSystemAPI::cchPathMax+1];

                ERR errT = ErrDownConvertName( (WCHAR*)wszName, szName, sizeof(szName) );
                Assert( errT >= JET_errSuccess );

                CHAR * szLossy = ( wrnLossy == errT ) ? szLossyUnicodePath : "";

                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x,%hs%hs,%u)", (PROCID)plrdetachdb->le_procid, szName, szLossy, (USHORT)plrdetachdb->dbid );
            }
            else
            {
                CHAR *sz;

                sz = reinterpret_cast<CHAR *>( plrdetachdb ) + sizeof(LRDETACHDB);
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x,%s,%u)", (PROCID)plrdetachdb->le_procid, sz, (USHORT) plrdetachdb->dbid );
            }
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypCreateMultipleExtentFDP:
        {
            LRCREATEMEFDP *plrcreatemefdp = (LRCREATEMEFDP *)plr;

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                " %I64x,%lx:%u(%x,FDP:[%u:%lu],OE:[%u:%lu],AE:[%u:%lu],FDPParent:[%u:%lu],objid:[%u:%lu],PageFlags:[0x%lx],%lu,%s:%s)",
                (DBTIME) plrcreatemefdp->le_dbtime,
                (TRX) plrcreatemefdp->le_trxBegin0,
                (USHORT) plrcreatemefdp->level,
                (PROCID) plrcreatemefdp->le_procid,
                (USHORT) plrcreatemefdp->dbid,
                (PGNO) plrcreatemefdp->le_pgno,
                (USHORT) plrcreatemefdp->dbid,
                (PGNO) plrcreatemefdp->le_pgnoOE,
                (USHORT) plrcreatemefdp->dbid,
                (PGNO) plrcreatemefdp->le_pgnoAE,
                (USHORT) plrcreatemefdp->dbid,
                (PGNO) plrcreatemefdp->le_pgnoFDPParent,
                (USHORT) plrcreatemefdp->dbid,
                (OBJID) plrcreatemefdp->le_objidFDP,
                (ULONG) plrcreatemefdp->le_fPageFlags,
                (ULONG) plrcreatemefdp->le_cpgPrimary,
                plrcreatemefdp->FUnique() ? szUnique : szNotUnique,
                plrcreatemefdp->FSystemTask() ? szSystemTask : szNotSystemTask );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            break;
        }

        case lrtypCreateSingleExtentFDP:
        {
            LRCREATESEFDP *plrcreatesefdp = (LRCREATESEFDP *)plr;

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                " %I64x,%lx:%u(%x,[%u:%lu],[%u:%lu],[%lu],PageFlags:[0x%lx]%lu,%s:%s)",
                (DBTIME) plrcreatesefdp->le_dbtime,
                (TRX) plrcreatesefdp->le_trxBegin0,
                (USHORT) plrcreatesefdp->level,
                (PROCID) plrcreatesefdp->le_procid,
                (USHORT) plrcreatesefdp->dbid,
                (PGNO) plrcreatesefdp->le_pgno,
                (USHORT) plrcreatesefdp->dbid,
                (PGNO) plrcreatesefdp->le_pgnoFDPParent,
                (OBJID) plrcreatesefdp->le_objidFDP,
                (ULONG) plrcreatesefdp->le_fPageFlags,
                (ULONG) plrcreatesefdp->le_cpgPrimary,
                plrcreatesefdp->FUnique() ? szUnique : szNotUnique,
                plrcreatesefdp->FSystemTask() ? szSystemTask : szNotSystemTask );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            break;
        }

        case lrtypConvertFDP2:
        case lrtypConvertFDP:
        {
            LRCONVERTFDP *plrconvertfdp = (LRCONVERTFDP *)plr;

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                " %I64x,%I64x,%lx:%u(%x,[%u:%lu],[%u:%lu],[%lu],+%lu,[%u:%lu],+%lu,%s:%s)",
                (DBTIME) plrconvertfdp->le_dbtime,
                (DBTIME) plrconvertfdp->le_dbtimeBefore,
                (TRX) plrconvertfdp->le_trxBegin0,
                (USHORT) plrconvertfdp->level,
                (PROCID) plrconvertfdp->le_procid,
                (USHORT) plrconvertfdp->dbid,
                (PGNO) plrconvertfdp->le_pgno,
                (USHORT) plrconvertfdp->dbid,
                (PGNO) plrconvertfdp->le_pgnoFDPParent,
                (OBJID) plrconvertfdp->le_objidFDP,
                (ULONG) plrconvertfdp->le_cpgPrimary,
                (USHORT) plrconvertfdp->dbid,
                (PGNO) plrconvertfdp->le_pgnoSecondaryFirst,
                (ULONG) plrconvertfdp->le_cpgSecondary,
                plrconvertfdp->FUnique() ? szUnique : szNotUnique,
                plrconvertfdp->FSystemTask() ? szSystemTask : szNotSystemTask );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            break;
        }

        case lrtypSplit:
        case lrtypSplit2:
        {
            LRSPLIT_ *plrsplit = (LRSPLIT_ *)plr;

            if ( pgnoNull == plrsplit->le_pgnoNew )
            {
                Assert( 0 == plrsplit->le_cbKeyParent );
                Assert( 0 == plrsplit->le_cbPrefixSplitOld );
                Assert( 0 == plrsplit->le_cbPrefixSplitNew );
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                    "%s %I64x,%I64x,%lx:%u(%x,parent:%I64x,[%u:%lu])",
                        " ParentOfSplitPage",
                        (DBTIME) plrsplit->le_dbtime,
                        (DBTIME) plrsplit->le_dbtimeBefore,
                        (TRX) plrsplit->le_trxBegin0,
                        (USHORT) plrsplit->level,
                        (PROCID) plrsplit->le_procid,
                        (DBTIME) plrsplit->le_dbtimeParentBefore,
                        (USHORT) plrsplit->dbid,
                        (PGNO) plrsplit->le_pgno );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            else
            {
                switch( plrsplit->splittype )
                {
                    case splittypeRight:
                        if ( splitoperInsert == plrsplit->splitoper
                            && plrsplit->le_ilineSplit == plrsplit->le_ilineOper
                            && plrsplit->le_ilineSplit == plrsplit->le_clines - 1 )
                        {
                            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " SplitHotpoint" );
                        }
                        else
                        {
                            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " SplitRight" );
                        }
                        break;
                    case splittypeVertical:
                        OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " SplitVertical" );
                        break;
                    case splittypeAppend:
                        OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " Append" );
                        break;
                    default:
                        Assert( fFalse );
                }
                OSStrCbAppendA( szLR, cbLR, rgchBuf );

                const CHAR *    szSplitoper[4]  = { "None",
                                                    "Insert",
                                                    "Replace",
                                                    "FlagInsertAndReplaceData" };

                switch( plrsplit->splitoper )
                {
                    case splitoperNone:
                    case splitoperInsert:
                    case splitoperReplace:
                    case splitoperFlagInsertAndReplaceData:
                        break;
                    default:
                        Assert( fFalse );
                }

                CHAR *szDehydrate = "", *szXpress = "";
                if ( plr->lrtyp == lrtypSplit2 )
                {
                    LRSPLITNEW *plrSplit2 = (LRSPLITNEW *)plrsplit;
                    if ( plrSplit2->FPreimageDehydrated() ) szDehydrate = "D";
                    if ( plrSplit2->FPreimageXpress() ) szXpress = "X";
                }

                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                    " %I64x,%I64x,%lx:%u(%x,split:[%u:%lu:%u],oper/last:[%u/%u],new:[%u:%lu],parent:%I64x,[%u:%lu],right:%I64x,[%u:%lu], objid:[%u:%lu], %s:%s:%s:%s, splitoper:%s, beforeImage:%d[%s%s])",
                        (DBTIME) plrsplit->le_dbtime,
                        (DBTIME) plrsplit->le_dbtimeBefore,
                        (TRX) plrsplit->le_trxBegin0,
                        (USHORT) plrsplit->level,
                        (PROCID) plrsplit->le_procid,
                        (USHORT) plrsplit->dbid,
                        (PGNO) plrsplit->le_pgno,
                        (USHORT) plrsplit->le_ilineSplit,
                        (USHORT) plrsplit->le_ilineOper,
                        (USHORT) plrsplit->le_clines-1,
                        (USHORT) plrsplit->dbid,
                        (PGNO) plrsplit->le_pgnoNew,
                        (DBTIME) (pgnoNull != (PGNO) plrsplit->le_pgnoParent ? (DBTIME) plrsplit->le_dbtimeParentBefore: 0),
                        (USHORT) plrsplit->dbid,
                        (PGNO) plrsplit->le_pgnoParent,
                        (DBTIME) (pgnoNull != (PGNO) plrsplit->le_pgnoRight ? (DBTIME) plrsplit->le_dbtimeRightBefore: 0),
                        (USHORT) plrsplit->dbid,
                        (PGNO) plrsplit->le_pgnoRight,
                        (USHORT) plrsplit->dbid,
                        (OBJID) plrsplit->le_objidFDP,
                        plrsplit->FUnique() ? szUnique : szNotUnique,
                        plrsplit->FSpace() ? szSpaceTree : szNotSpaceTree,
                        plrsplit->FSystemTask() ? szSystemTask : szNotSystemTask,
                        plrsplit->FConcCI() ? szConcCI : szNotConcCI,
                        szSplitoper[plrsplit->splitoper],
                        CbPageBeforeImage(plrsplit),
                        szDehydrate,
                        szXpress);
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }

            break;
        }

        case lrtypMerge:
        case lrtypMerge2:
        {
            LRMERGE_ *plrmerge = (LRMERGE_ *)plr;
            const BYTE  *pb = PbData(plrmerge);
            INT     cb = plrmerge->le_cbKeyParentSep;
            const INT mergetype = plrmerge->mergetype;

            CHAR *szDehydrate = "", *szXpress = "";
            if ( plr->lrtyp == lrtypMerge2 )
            {
                LRMERGENEW *plrMerge2 = (LRMERGENEW *)plrmerge;
                if ( plrMerge2->FPreimageDehydrated() ) szDehydrate = "D";
                if ( plrMerge2->FPreimageXpress() ) szXpress = "X";
            }

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                " %I64x,%I64x,%lx:%u(%x,merge:[%u:%lu:%u],right:%I64x,[%u:%lu],left:%I64x,[%u:%lu],parent:%I64x,[%u:%lu],%s:%s:%s:%s:%s:%s:%s,size:%d,maxsize:%d,uncfree:%d,beforeImage:%d[%s%s])",
                (DBTIME) plrmerge->le_dbtime,
                (DBTIME) plrmerge->le_dbtimeBefore,
                (TRX) plrmerge->le_trxBegin0,
                (USHORT) plrmerge->level,
                (PROCID) plrmerge->le_procid,
                (USHORT) plrmerge->dbid,
                (PGNO) plrmerge->le_pgno,
                plrmerge->ILineMerge(),
                (DBTIME)  (pgnoNull != (PGNO) plrmerge->le_pgnoRight ? (DBTIME) plrmerge->le_dbtimeRightBefore: 0),
                (USHORT) plrmerge->dbid,
                (PGNO) plrmerge->le_pgnoRight,
                (DBTIME)  (pgnoNull != (PGNO) plrmerge->le_pgnoLeft ? (DBTIME) plrmerge->le_dbtimeLeftBefore: 0),
                (USHORT) plrmerge->dbid,
                (PGNO) plrmerge->le_pgnoLeft,
                (DBTIME)  (pgnoNull != (PGNO) plrmerge->le_pgnoParent ? (DBTIME) plrmerge->le_dbtimeParentBefore: 0),
                (USHORT) plrmerge->dbid,
                (PGNO) plrmerge->le_pgnoParent,
                plrmerge->FUnique() ? szUnique : szNotUnique,
                plrmerge->FSpace() ? szSpaceTree : szNotSpaceTree,
                plrmerge->FSystemTask() ? szSystemTask : szNotSystemTask,
                plrmerge->FKeyChange() ? szKeyChange : szNoKeyChange,
                plrmerge->FEmptyPage() ? szEmptyPage : szNotEmptyPage,
                plrmerge->FDeleteNode() ? szDeleted : szNotDeleted,
                rgszMergeType[mergetype],
                (SHORT) plrmerge->le_cbSizeTotal,
                (SHORT) plrmerge->le_cbSizeMaxTotal,
                (SHORT) plrmerge->le_cbUncFreeDest,
                CbPageBeforeImage(plrmerge),
                szDehydrate,
                szXpress);
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            DataToSz( pb, cb, plog->IDumpVerbosityLevel(), rgchBuf, cbLRBuf );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            break;
        }

        case lrtypPageMove:
        {
            const LRPAGEMOVE * const plrpagemove = LRPAGEMOVE::PlrpagemoveFromLr( plr );

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                " %I64x,%I64x,%lx:%u(%x,source:[%u:%lu],destination:[%u:%lu],right:%I64x,[%u:%lu],left:%I64x,[%u:%lu],parent:%I64x,[%u:%lu:%u],header:%d,trailer:%d,objid:[%u:%lu],flags:%lx, %s)",

                plrpagemove->DbtimeAfter(),
                plrpagemove->DbtimeSourceBefore(),
                (TRX) plrpagemove->le_trxBegin0,
                (USHORT) plrpagemove->level,
                (PROCID) plrpagemove->le_procid,
                
                (USHORT) plrpagemove->dbid,
                plrpagemove->PgnoSource(),

                (USHORT) plrpagemove->dbid,
                plrpagemove->PgnoDest(),
                
                (pgnoNull != (PGNO) plrpagemove->PgnoRight() ? (DBTIME) plrpagemove->DbtimeRightBefore() : 0),
                (USHORT) plrpagemove->Dbid(),
                plrpagemove->PgnoRight(),

                (pgnoNull != (PGNO) plrpagemove->PgnoLeft() ? (DBTIME) plrpagemove->DbtimeLeftBefore() : 0),
                (USHORT) plrpagemove->Dbid(),
                plrpagemove->PgnoLeft(),

                plrpagemove->DbtimeParentBefore(),
                (USHORT) plrpagemove->Dbid(),
                plrpagemove->PgnoParent(),
                plrpagemove->IlineParent(),
                
                plrpagemove->CbPageHeader(),
                plrpagemove->CbPageTrailer(),

                (USHORT) plrpagemove->Dbid(),
                plrpagemove->ObjidFDP(),

                plrpagemove->FMoveFlags(),
                plrpagemove->FSystemTask() ? szSystemTask : szNotSystemTask );
                
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            
            break;
        }

        case lrtypRootPageMove:
        {
            const LRROOTPAGEMOVE * const plrrpm = (LRROOTPAGEMOVE *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                " %I64x,(%x,objid:[%u:%lu])",
                plrrpm->DbtimeAfter(),
                plrrpm->Procid(),
                plrrpm->Dbid(),
                plrrpm->Objid() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypPagePatchRequest:
        {
            const LRPAGEPATCHREQUEST * const plrpagepatchrequest = (LRPAGEPATCHREQUEST *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                " %I64x,[%d:%d]",
                plrpagepatchrequest->Dbtime(),
                plrpagepatchrequest->Dbid(),
                plrpagepatchrequest->Pgno());
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
        }
            break;
        
        case lrtypEmptyTree:
        {
            LREMPTYTREE *plremptytree   = (LREMPTYTREE *)plr;

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                " %I64x,%I64x,%lx:%u(%x,[%u:%lu],objid:[%u:%lu],%s:%s:%s,%lu)",
                (DBTIME) plremptytree->le_dbtime,
                (DBTIME) plremptytree->le_dbtimeBefore,
                (TRX) plremptytree->le_trxBegin0,
                (USHORT) plremptytree->level,
                (PROCID) plremptytree->le_procid,
                (USHORT) plremptytree->dbid,
                (PGNO) plremptytree->le_pgno,
                (USHORT) plremptytree->dbid,
                (ULONG) plremptytree->le_objidFDP,
                plremptytree->FUnique() ? szUnique : szNotUnique,
                plremptytree->FSpace() ? szSpaceTree : szNotSpaceTree,
                plremptytree->FSystemTask() ? szSystemTask : szNotSystemTask,
                plremptytree->CbEmptyPageList() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            DataToSz( plremptytree->rgb, plremptytree->CbEmptyPageList(), plog->IDumpVerbosityLevel(), rgchBuf, cbLRBuf );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypSetExternalHeader:
        {
            const LRSETEXTERNALHEADER * const   plrsetexternalheader = (LRSETEXTERNALHEADER *)plr;
            const BYTE * const                  pb  = plrsetexternalheader->rgbData;
            const INT                           cb  = plrsetexternalheader->CbData();
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                " %I64x,%I64x,%lx:%u(%x,[%u:%lu],%s:%s:%s,%5u)",
                        (DBTIME) plrsetexternalheader->le_dbtime,
                        (DBTIME) plrsetexternalheader->le_dbtimeBefore,
                        (TRX) plrsetexternalheader->le_trxBegin0,
                        (USHORT) plrsetexternalheader->level,
                        (PROCID) plrsetexternalheader->le_procid,
                        (USHORT) plrsetexternalheader->dbid,
                        (PGNO) plrsetexternalheader->le_pgno,
                        plrsetexternalheader->FUnique() ? szUnique : szNotUnique,
                        plrsetexternalheader->FSpace() ? szSpaceTree : szNotSpaceTree,
                        plrsetexternalheader->FSystemTask() ? szSystemTask : szNotSystemTask,
                        plrsetexternalheader->CbData() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            DataToSz( pb, cb, plog->IDumpVerbosityLevel(), rgchBuf, cbLRBuf );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypScrub:
        {
            const LRSCRUB * const plrscrub = (LRSCRUB *)plr;
            const BYTE * const                  pb  = plrscrub->PbData();
            const INT                           cb  = plrscrub->CbData();
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                " %I64x,%I64x,%lx:%u(%x,[%u:%lu],%s:%s:%s,%s,%d,%5u)",
                        (DBTIME) plrscrub->le_dbtime,
                        (DBTIME) plrscrub->le_dbtimeBefore,
                        (TRX) plrscrub->le_trxBegin0,
                        (USHORT) plrscrub->level,
                        (PROCID) plrscrub->le_procid,
                        (USHORT) plrscrub->dbid,
                        (PGNO) plrscrub->le_pgno,
                        plrscrub->FUnique() ? szUnique : szNotUnique,
                        plrscrub->FSpace() ? szSpaceTree : szNotSpaceTree,
                        plrscrub->FSystemTask() ? szSystemTask : szNotSystemTask,
                        plrscrub->FUnusedPage() ? "unused page" : "used page",
                        plrscrub->CscrubOper(),
                        plrscrub->CbData() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            DataToSz( pb, cb, plog->IDumpVerbosityLevel(), rgchBuf, cbLRBuf );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypUndo:
        {
            LRUNDO *plrundo = (LRUNDO *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s,%u,%u,rceid:%u)",
                (DBTIME) plrundo->le_dbtime,
                (DBTIME) plrundo->le_dbtimeBefore,
                (TRX) plrundo->le_trxBegin0,
                (USHORT) plrundo->level,
                (PROCID) plrundo->le_procid,
                (USHORT) plrundo->dbid,
                (PGNO) plrundo->le_pgno,
                plrundo->ILine(),
                plrundo->FUnique() ? szUnique : szNotUnique,
                plrundo->FSystemTask() ? szSystemTask : szNotSystemTask,
                (USHORT) plrundo->level,
                (USHORT) plrundo->le_oper,
                (RCEID) plrundo->le_rceid );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypInit:
        case lrtypInit2:
        {
            if ( lrtypInit2 == plr->lrtyp )
            {
                const LOGTIME &logtime = ((LRINIT2 *)plr)->logtime;
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                    " (%u/%u/%u %u:%02u:%02u.%3.3u)",
                    (INT) (logtime.bMonth),
                    (INT) (logtime.bDay),
                    (INT) (logtime.bYear+1900),
                    (INT) (logtime.bHours),
                    (INT) (logtime.bMinutes),
                    (INT) (logtime.bSeconds),
                    (INT) (logtime.Milliseconds()) );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            DBMS_PARAM *pdbms_param = &((LRINIT2 *)plr)->dbms_param;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), "\n      Env SystemPath:%s\n",    pdbms_param->szSystemPathDebugOnly);
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), "      Env LogFilePath:%s\n", pdbms_param->szLogFilePathDebugOnly);
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), "      Env (CircLog,Session,Opentbl,VerPage,Cursors,LogBufs,LogFile,Buffers)\n");
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), "          (%s,%7lu,%7lu,%7lu,%7lu,%7lu,%7lu,%7lu)\n",
                ( pdbms_param->fDBMSPARAMFlags & fDBMSPARAMCircularLogging ? "     on" : "    off" ),
                (ULONG) pdbms_param->le_lSessionsMax,
                (ULONG) pdbms_param->le_lOpenTablesMax,
                (ULONG) pdbms_param->le_lVerPagesMax,
                (ULONG) pdbms_param->le_lCursorsMax,
                (ULONG) pdbms_param->le_lLogBuffers,
                (ULONG) pdbms_param->le_lcsecLGFile,
                (ULONG) pdbms_param->le_ulCacheSizeMax );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
        }
            break;

        case lrtypTerm:
        case lrtypShutDownMark:
        case lrtypRecoveryUndo:
            break;

        case lrtypTerm2:
        {
            const LOGTIME &logtime = ((LRTERMREC2 *)plr)->logtime;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%u/%u/%u %u:%02u:%02u.%3.3u)",
                (INT) (logtime.bMonth),
                (INT) (logtime.bDay),
                (INT) (logtime.bYear+1900),
                (INT) (logtime.bHours),
                (INT) (logtime.bMinutes),
                (INT) (logtime.bSeconds),
                (INT) (logtime.Milliseconds()) );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
        }
            break;

        case lrtypRecoveryUndo2:
        {
            const LOGTIME &logtime = ((LRRECOVERYUNDO2 *)plr)->logtime;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%u/%u/%u %u:%02u:%02u.%3.3u)",
                (INT) (logtime.bMonth),
                (INT) (logtime.bDay),
                (INT) (logtime.bYear+1900),
                (INT) (logtime.bHours),
                (INT) (logtime.bMinutes),
                (INT) (logtime.bSeconds),
                (INT) (logtime.Milliseconds()) );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
        }
            break;

        case lrtypRecoveryQuit:
        case lrtypRecoveryQuit2:
        case lrtypRecoveryQuitKeepAttachments:
        {
            LRTERMREC2 *plrquit = (LRTERMREC2 *) plr;

            if ( lrtypRecoveryQuit2 == plr->lrtyp ||
                 lrtypRecoveryQuitKeepAttachments == plr->lrtyp )
            {
                const LOGTIME &logtime = plrquit->logtime;
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%u/%u/%u %u:%02u:%02u.%3.3u)",
                    (INT) (logtime.bMonth),
                    (INT) (logtime.bDay),
                    (INT) (logtime.bYear+1900),
                    (INT) (logtime.bHours),
                    (INT) (logtime.bMinutes),
                    (INT) (logtime.bSeconds),
                    (INT) (logtime.Milliseconds()) );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            if ( plrquit->fHard )
            {
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), "\n      Quit on Hard Restore." );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            else
            {
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), "\n      Quit on Soft Restore." );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), "\n      RedoFrom:(%X,%X,%X)\n",
                (LONG) plrquit->lgposRedoFrom.le_lGeneration,
                (SHORT) plrquit->lgposRedoFrom.le_isec,
                (SHORT) plrquit->lgposRedoFrom.le_ib );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), "      UndoRecordFrom:(%X,%X,%X)\n",
                (LONG) plrquit->lgpos.le_lGeneration,
                (SHORT) plrquit->lgpos.le_isec,
                (SHORT) plrquit->lgpos.le_ib );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
        }
            break;

        case lrtypFullBackup:
        case lrtypIncBackup:
        {
            LRLOGRESTORE *plrlr = (LRLOGRESTORE *) plr;

            if ( plrlr->le_cbPath )
            {
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), "%*s", (USHORT) plrlr->le_cbPath, plrlr->szData );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            break;
        }

        case lrtypBackup:
        {
            LRLOGBACKUP_OBSOLETE *plrlb = (LRLOGBACKUP_OBSOLETE *) plr;

            if ( plrlb->FFull() )
            {
                OSStrCbAppendA( szLR, cbLR, " FullBackup" );
            }
            else if ( plrlb->FIncremental() )
            {
                OSStrCbAppendA( szLR, cbLR, " IncrementalBackup" );
            }
            else if ( plrlb->FSnapshotStart() )
            {
                OSStrCbAppendA( szLR, cbLR, " StartSnapshot" );
            }
            else if ( plrlb->FSnapshotStop() )
            {
                OSStrCbAppendA( szLR, cbLR, " StopSnapshot" );
            }
            else if ( plrlb->FTruncateLogs() )
            {
                OSStrCbAppendA( szLR, cbLR, " TruncateLogs" );
            }
            else if ( plrlb->FOSSnapshot() )
            {
                OSStrCbAppendA( szLR, cbLR, " OSSnapshot" );
            }
            else if ( plrlb->FOSSnapshotIncremental() )
            {
                OSStrCbAppendA( szLR, cbLR, " OSIncrementalSnapshot" );
            }
            else
            {
                Assert ( fFalse );
            }

            if ( plrlb->le_cbPath )
            {
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %*s", (USHORT) plrlb->le_cbPath, plrlb->szData );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            break;
        }

        case lrtypBackup2:
        {
            LRLOGBACKUP *plrlb = (LRLOGBACKUP *) plr;
            CHAR * szBackupPhase = NULL;
            CHAR * szBackupType = NULL;
            CHAR * szBackupScope = NULL;

            switch ( plrlb->eBackupPhase )
            {
                case LRLOGBACKUP::fLGBackupPhaseBegin:
                    szBackupPhase = "Begin";
                    break;
                case LRLOGBACKUP::fLGBackupPhasePrepareLogs:
                    szBackupPhase = "PrepLogs";
                    break;
                case LRLOGBACKUP::fLGBackupPhaseTruncate:
                    szBackupPhase = "Truncate";
                    break;
                case LRLOGBACKUP::fLGBackupPhaseUpdate:
                    szBackupPhase = "Update";
                    break;
                case LRLOGBACKUP::fLGBackupPhaseAbort:
                    szBackupPhase = "Torpedos Away!";
                    break;
                case LRLOGBACKUP::fLGBackupPhaseStop:
                    szBackupPhase = "Stop";
                    break;
                default:
                    AssertSz( fFalse, "Unknown eBackupPhase!" );
                    szBackupPhase = "*Unknown*";
                    break;
            }

            switch ( plrlb->eBackupType )
            {
                case DBFILEHDR::backupNormal:
                    szBackupType = "External";
                    break;
                case DBFILEHDR::backupOSSnapshot:
                    szBackupType = "OSSnapshot";
                    break;
                
                case DBFILEHDR::backupSurrogate:
                    szBackupType = "Surrogate";
                    break;
                default:
                    AssertSz( fFalse, "Unknown eBackupType!" );
                    szBackupType = "*Unknown*";
            }

            switch ( plrlb->eBackupScope )
            {
                case LRLOGBACKUP::fLGBackupScopeFull:
                    szBackupScope = "Full";
                    break;
                case LRLOGBACKUP::fLGBackupScopeIncremental:
                    szBackupScope = "Incremental";
                    break;
                case LRLOGBACKUP::fLGBackupScopeCopy:
                    szBackupScope = "Copy";
                    break;
                case LRLOGBACKUP::fLGBackupScopeDifferential:
                    szBackupScope = "Differential";
                    break;
                default:
                    AssertSz( fFalse, "Unknown eBackupScope!" );
                    szBackupScope = "*Unknown*";
            }

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                " %s( %s:%s, dbid=%d, phaseDetails[(%X,%X,%X),(%u/%u/%u %u:%02u:%02u.%3.3d),(%d,%d)] )",
                szBackupPhase, szBackupType, szBackupScope,
                (INT) (plrlb->dbid),
                (LONG) (plrlb->phaseDetails.le_lgposMark.le_lGeneration),
                (SHORT) (plrlb->phaseDetails.le_lgposMark.le_isec),
                (SHORT) (plrlb->phaseDetails.le_lgposMark.le_ib),
                (INT) (plrlb->phaseDetails.logtimeMark.bMonth),
                (INT) (plrlb->phaseDetails.logtimeMark.bDay),
                (INT) (plrlb->phaseDetails.logtimeMark.bYear+1900),
                (INT) (plrlb->phaseDetails.logtimeMark.bHours),
                (INT) (plrlb->phaseDetails.logtimeMark.bMinutes),
                (INT) (plrlb->phaseDetails.logtimeMark.bSeconds),
                (INT) (plrlb->phaseDetails.logtimeMark.Milliseconds()),
                (ULONG) (plrlb->phaseDetails.le_genLow),
                (ULONG) (plrlb->phaseDetails.le_genHigh)
                );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            if ( plrlb->le_cbPath )
            {
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %*s", (USHORT) plrlb->le_cbPath, plrlb->szData );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            break;
        }

        case lrtypTrace:
        case lrtypForceLogRollover:
        {
            LRTRACE *plrtrace = (LRTRACE *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x) %s", (PROCID)plrtrace->le_procid, plrtrace->sz );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
        case lrtypChecksum:
        {
            const LRCHECKSUM * const plrck = static_cast< const LRCHECKSUM* const >( plr );

            Assert( plrck->bUseShortChecksum == bShortChecksumOn ||
                    plrck->bUseShortChecksum == bShortChecksumOff );

            if ( plrck->bUseShortChecksum == bShortChecksumOn )
            {
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                    " (0x%X,0x%X,0x%X checksum [0x%X,0x%X])",
                    (ULONG32) plrck->le_cbBackwards,
                    (ULONG32) plrck->le_cbForwards,
                    (ULONG32) plrck->le_cbNext,
                    (ULONG32) plrck->le_ulChecksum,
                    (ULONG32) plrck->le_ulShortChecksum );
            }
            else if ( plrck->bUseShortChecksum == bShortChecksumOff )
            {
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                    " (0x%X,0x%X,0x%X checksum [0x%X])",
                    (ULONG32) plrck->le_cbBackwards,
                    (ULONG32) plrck->le_cbForwards,
                    (ULONG32) plrck->le_cbNext,
                    (ULONG32) plrck->le_ulChecksum );
            }
            else
            {
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                    " CORRUPT (0x%X,0x%X,0x%X checksum 0x%X short checksum 0x%X use short checksum 0x%X)",
                    (ULONG32) plrck->le_cbBackwards,
                    (ULONG32) plrck->le_cbForwards,
                    (ULONG32) plrck->le_cbNext,
                    (ULONG32) plrck->le_ulChecksum,
                    (ULONG32) plrck->le_ulShortChecksum,
                    (BYTE) plrck->bUseShortChecksum );
            }
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }
#endif

        case lrtypExtRestore:
        {
            LREXTRESTORE *plrextrestore = (LREXTRESTORE *) plr;
            CHAR *sz;

            sz = reinterpret_cast<CHAR *>( plrextrestore ) + sizeof(LREXTRESTORE);
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%s,%s)", sz, sz + strlen(sz) + 1 );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypExtRestore2:
        {
            LREXTRESTORE2 * plrextrestore   = (LREXTRESTORE2 *) plr;
            CHAR *          szNames         = reinterpret_cast<CHAR *>( plrextrestore ) + sizeof(LREXTRESTORE);
            CAutoWSZPATH    wszName;

            CallS( wszName.ErrSet( (UnalignedLittleEndian< WCHAR > *)szNames ) );
            
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%ws,", (WCHAR*)wszName );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            szNames += ( sizeof(WCHAR) * ( LOSStrLengthW( wszName ) + 1 ) );
            CallS( wszName.ErrSet( (UnalignedLittleEndian< WCHAR > *)szNames ) );
            
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), "%ws)", (WCHAR*)wszName );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypMacroInfo:
        {
            const LRMACROINFO * const plrmacroinfo = (LRMACROINFO *) plr;
            OSStrCbAppendA( szLR, cbLR, " " );
            for ( CPG ipgno = 0; ipgno < plrmacroinfo->CountOfPgno(); ipgno++ )
            {
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), "[%u:%lu],", 1, plrmacroinfo->GetPgno( ipgno ) );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            
            break;
        }
        
        case lrtypExtendDB:
        {
            const LREXTENDDB * const plrextendb = (LREXTENDDB *) plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " [%u:%lu]", plrextendb->Dbid(), plrextendb->PgnoLast() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypCommitCtx:
        {
            if ( !plog || FLGVersionNewCommitCtx( &plog->m_pLogStream->GetCurrentFileHdr()->lgfilehdr ) )
            {
                const LRCOMMITCTX * const plrcommitctx = (LRCOMMITCTX *) plr;
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x) [%s%s]", plrcommitctx->ProcID(), plrcommitctx->FCallbackNeeded() ? "C" : "", plrcommitctx->FContainsCustomerData() ? "P" : "" );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
                Assert( plrcommitctx->CbCommitCtx() <= cbCommitCtxExpected );
                FullDataToSz( plrcommitctx->PbCommitCtx(), plrcommitctx->CbCommitCtx(), cbCommitCtxExpected, plrcommitctx->FContainsCustomerData() ? plog->IDumpVerbosityLevel() : LOG::ldvlData, rgchBuf, cbLRBuf );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            else
            {
                const LRCOMMITCTXOLD * const plrcommitctx = (LRCOMMITCTXOLD *) plr;
                OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%x) ", plrcommitctx->ProcID() );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
                Assert( plrcommitctx->CbCommitCtx() <= cbCommitCtxExpected );
                FullDataToSz( plrcommitctx->PbCommitCtx(), plrcommitctx->CbCommitCtx(), cbCommitCtxExpected, plog->IDumpVerbosityLevel(), rgchBuf, cbLRBuf );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }
            break;
        }

        case lrtypScanCheck:
        {
            const LRSCANCHECK * const plrscancheck = (LRSCANCHECK*)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %I64x,%I64x[%u:%lu]",
                plrscancheck->DbtimeAfter(),
                plrscancheck->DbtimeBefore(),
                plrscancheck->Dbid(),
                plrscancheck->Pgno() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypScanCheck2:
        {
            const LRSCANCHECK2 * const plrscancheck = (LRSCANCHECK2*)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %I64x,%I64x[%u:%lu],source:%hhu",
                plrscancheck->DbtimeCurrent(),
                plrscancheck->DbtimePage(),
                plrscancheck->Dbid(),
                plrscancheck->Pgno(),
                plrscancheck->BSource() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypReAttach:
        {
            const LRREATTACHDB * const plrreattach = (LRREATTACHDB*)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " [%d:],", (DBID)plrreattach->le_dbid );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " consistent:(%X,%X,%X) attach:(%X,%X,%X) SigDb: ",
                (LONG) plrreattach->lgposConsistent.le_lGeneration,
                (SHORT) plrreattach->lgposConsistent.le_isec,
                (SHORT) plrreattach->lgposConsistent.le_ib,
                (LONG) plrreattach->lgposAttach.le_lGeneration,
                (SHORT) plrreattach->lgposAttach.le_isec,
                (SHORT) plrreattach->lgposAttach.le_ib );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            SPrintSign( &plrreattach->signDb, sizeof(rgchBuf), rgchBuf );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            break;
        }

        case lrtypMacroInfo2:
        {
            const LRMACROINFO2 * const plrmacroinfo2 = (LRMACROINFO2 *)plr;
            OSStrCbAppendA( szLR, cbLR, " " );
            for ( CPG ipgno = 0; ipgno < plrmacroinfo2->CountOfPgno(); ipgno++ )
            {
                OSStrCbFormatA( rgchBuf, sizeof( rgchBuf ), "[%u:%lu],", plrmacroinfo2->Dbid(), plrmacroinfo2->GetPgno( ipgno ) );
                OSStrCbAppendA( szLR, cbLR, rgchBuf );
            }

            break;
        }

        case lrtypFreeFDP:
        {
            const LRFREEFDP * const plrfreefdp = (LRFREEFDP *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %lx:([%u:%lu] (%c))",
                            (TRX)plrfreefdp->le_trxCommitted,
                            (USHORT)plrfreefdp->le_dbid,
                            (PGNO)plrfreefdp->le_pgnoFDP,
                            (plrfreefdp->bFDPType == fFDPTypeTable) ? 'T' : 'I' );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );

            break;
        }

        case lrtypNOP2:
        case lrtypIgnored9:
        case lrtypIgnored10:
        case lrtypIgnored11:
        case lrtypIgnored12:
        case lrtypIgnored13:
        case lrtypIgnored14:
        case lrtypIgnored15:
        case lrtypIgnored16:
        case lrtypIgnored17:
        case lrtypIgnored18:
        case lrtypIgnored19:
        {
            const LRIGNORED * const plrignored = (LRIGNORED *) plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (lrtyp = %d, %d bytes)", (BYTE) plrignored->lrtyp, plrignored->Cb() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
        }
            break;

        case lrtypShrinkDB:
        case lrtypShrinkDB2:
        {
            const LRSHRINKDB * const plrshrinkdb = (LRSHRINKDB *) plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " [%u:%lu:%ld]", plrshrinkdb->Dbid(), plrshrinkdb->PgnoLast(), plrshrinkdb->CpgShrunk() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }
        case lrtypShrinkDB3:
        {
            const LRSHRINKDB3 * const plrshrinkdb = (LRSHRINKDB3 *) plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " %I64x[%u:%lu:%ld]", plrshrinkdb->Dbtime(), plrshrinkdb->Dbid(), plrshrinkdb->PgnoLast(), plrshrinkdb->CpgShrunk() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }
        case lrtypTrimDB:
        {
            const LRTRIMDB * const plrtrimdb = (LRTRIMDB *) plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " [%u:%lu+%ld] (", plrtrimdb->Dbid(), plrtrimdb->PgnoStartZeroes(), plrtrimdb->CpgZeroLength() );

            for ( INT i = 1; i < plrtrimdb->CpgZeroLength() && i < 32; ++i )
            {
                CHAR rgchPage[ 16 ];
                OSStrCbFormatA( rgchPage, sizeof( rgchPage ), "[%u:%lu] ", plrtrimdb->Dbid(), plrtrimdb->PgnoStartZeroes() + i );
                OSStrCbAppendA( rgchBuf, sizeof(rgchBuf), rgchPage );
            }
            OSStrCbAppendA( rgchBuf, sizeof(rgchBuf), ")" );

            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }
        case lrtypFragmentBegin:
        {
            const LRFRAGMENTBEGIN * const plrFragBegin = (LRFRAGMENTBEGIN *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (Total %u bytes, current %hu bytes, lrtyp %s)", (ULONG) plrFragBegin->le_cbTotalLRSize, (USHORT) plrFragBegin->le_cbFragmentSize, SzLrtyp( plrFragBegin->le_rgbData[0] ) );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
        }
            break;

        case lrtypFragment:
        {
            const LRFRAGMENT * const plrFrag = (LRFRAGMENT *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (%hu bytes)", (USHORT) plrFrag->le_cbFragmentSize );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
        }
            break;

        case lrtypSetDbVersion:
        {
            const LRSETDBVERSION * const plrsetdbversion = (LRSETDBVERSION *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (dbid:%d, DB ver:%u.%u.%u)",
                            (DBID)plrsetdbversion->Dbid(),
                            plrsetdbversion->Dbv().ulDbMajorVersion, plrsetdbversion->Dbv().ulDbUpdateMajor, plrsetdbversion->Dbv().ulDbUpdateMinor );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
        }
            break;

        case lrtypNewPage:
        {
            const LRNEWPAGE * const plrnewpage = (LRNEWPAGE *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf),
                " %I64x,(%x,page:[%u:%lu],objid:[%u:%lu])",
                plrnewpage->DbtimeAfter(),
                plrnewpage->Procid(),
                plrnewpage->Dbid(),
                plrnewpage->Pgno(),
                plrnewpage->Dbid(),
                plrnewpage->Objid() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypSignalAttachDb:
        {
            const LRSIGNALATTACHDB * const plrsattachdb = (LRSIGNALATTACHDB *)plr;
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " (dbid:%u)", plrsattachdb->Dbid() );
            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        case lrtypExtentFreed:
        {
            const LREXTENTFREED * const plrextentfreed = (LREXTENTFREED*)plr;

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " [%u:%lu+%ld] (", plrextentfreed->Dbid(), plrextentfreed->PgnoFirst(), plrextentfreed->CpgExtent() );

            for ( INT i = 1; i < plrextentfreed->CpgExtent() && i < 32; ++i )
            {
                CHAR rgchPage[ 16 ];
                OSStrCbFormatA( rgchPage, sizeof( rgchPage ), "[%u:%lu] ", plrextentfreed->Dbid(), plrextentfreed->PgnoFirst() + i );
                OSStrCbAppendA( rgchBuf, sizeof(rgchBuf), rgchPage );
            }
            OSStrCbAppendA( rgchBuf, sizeof(rgchBuf), ")" );

            OSStrCbAppendA( szLR, cbLR, rgchBuf );
            break;
        }

        default:
        {
            EnforceSz( fFalse, OSFormat( "LrToSzUnknownLr:%d", (INT)plr->lrtyp ) );
            break;
        }
    }
}

ERR  ErrAddPageRef( _In_ const DBID                                                                         dbid,
                    _In_ const PGNO                                                                         pgno,
                    _Inout_ ULONG* const                                                                    pcPageRef,
                    _Inout_ ULONG* const                                                                    pcPageRefAlloc,
                    _At_(*_Curr_, _Inout_updates_to_opt_( *pcPageRefAlloc, *pcPageRef )) _Inout_ PageRef**  prgPageRef,
                    _In_ const BOOL                                                                         fWrite = fTrue,
                    _In_ const BOOL                                                                         fRead = fTrue )

{
    ERR err = JET_errSuccess;

    const ULONG cPageRefMin = 128;
    const ULONG cPageRefNew = *pcPageRef + 1;
    if ( *prgPageRef == NULL || *pcPageRefAlloc < cPageRefNew )
    {
        ULONG       cPageRefAllocNew    = max(cPageRefMin, max( cPageRefNew, *pcPageRefAlloc * 2 ) );
        PageRef*    rgPageRefNew        = NULL;
        Alloc( rgPageRefNew = new PageRef[cPageRefAllocNew] );
        if ( *prgPageRef != NULL )
        {
            memcpy( rgPageRefNew, *prgPageRef, sizeof( PageRef ) * *pcPageRefAlloc );
            delete[] * prgPageRef;
        }
        *pcPageRefAlloc = cPageRefAllocNew;
        *prgPageRef = rgPageRefNew;
    }

    ( *prgPageRef )[( *pcPageRef )++] = PageRef( dbid, pgno, fWrite, fRead );

HandleError:
    return err;
}

ERR ErrLrToPageRef( _In_ INST* const                                                                        pinst,
                    _In_ const LR* const                                                                    plr,
                    _Inout_ ULONG* const                                                                    pcPageRef,
                    _Inout_ ULONG* const                                                                    pcPageRefAlloc,
                    _At_(*_Curr_, _Inout_updates_to_opt_( *pcPageRefAlloc, *pcPageRef )) _Inout_ PageRef**  prgPageRef )
{
    ERR err = JET_errSuccess;

    switch( plr->lrtyp )
    {
        case lrtypInsert:
        case lrtypFlagInsert:
        case lrtypFlagInsertAndReplaceData:
        case lrtypFlagDelete:
        case lrtypReplace:
        case lrtypReplaceD:
        case lrtypDelete:
        case lrtypDelta:
        case lrtypDelta64:
        case lrtypSetExternalHeader:
        case lrtypUndo:
        case lrtypScrub:
        {
            const LRPAGE_ * const   plrpage         = (LRPAGE_*)plr;
            const DBID              dbid            = plrpage->dbid;

            Call( ErrAddPageRef( dbid, plrpage->le_pgno, pcPageRef, pcPageRefAlloc, prgPageRef ) );
            break;
        }

        case lrtypSplit:
        case lrtypSplit2:
        {
            const LRSPLIT_ * const  plrsplit        = (LRSPLIT_*)plr;
            const DBID              dbid            = plrsplit->dbid;

            if ( plrsplit->le_pgno != pgnoNull )
            {
                Call( ErrAddPageRef( dbid, plrsplit->le_pgno, pcPageRef, pcPageRefAlloc, prgPageRef ) );
            }
            if ( plrsplit->le_pgnoNew != pgnoNull )
            {
                Call( ErrAddPageRef( dbid, plrsplit->le_pgnoNew, pcPageRef, pcPageRefAlloc, prgPageRef, fTrue, fFalse ) );
            }
            if ( plrsplit->le_pgnoParent != pgnoNull )
            {
                Call( ErrAddPageRef( dbid, plrsplit->le_pgnoParent, pcPageRef, pcPageRefAlloc, prgPageRef ) );
            }
            if ( plrsplit->le_pgnoRight != pgnoNull )
            {
                Call( ErrAddPageRef( dbid, plrsplit->le_pgnoRight, pcPageRef, pcPageRefAlloc, prgPageRef ) );
            }
            break;
        }

        case lrtypMerge:
        case lrtypMerge2:
        {
            const LRMERGE_ * const  plrmerge        = (LRMERGE_*)plr;
            const DBID              dbid            = plrmerge->dbid;

            if ( plrmerge->le_pgno != pgnoNull )
            {
                Call( ErrAddPageRef( dbid, plrmerge->le_pgno, pcPageRef, pcPageRefAlloc, prgPageRef ) );
            }
            if ( plrmerge->le_pgnoRight != pgnoNull )
            {
                Call( ErrAddPageRef( dbid, plrmerge->le_pgnoRight, pcPageRef, pcPageRefAlloc, prgPageRef ) );
            }
            if ( plrmerge->le_pgnoLeft != pgnoNull )
            {
                Call( ErrAddPageRef( dbid, plrmerge->le_pgnoLeft, pcPageRef, pcPageRefAlloc, prgPageRef ) );
            }
            if ( plrmerge->le_pgnoParent != pgnoNull )
            {
                Call( ErrAddPageRef( dbid, plrmerge->le_pgnoParent, pcPageRef, pcPageRefAlloc, prgPageRef ) );
            }
            break;
        }

        case lrtypCreateMultipleExtentFDP:
        {
            const LRCREATEMEFDP *   plrcreatemefdp  = (LRCREATEMEFDP*)plr;
            const DBID              dbid            = plrcreatemefdp->dbid;

            Call( ErrAddPageRef( dbid, plrcreatemefdp->le_pgno, pcPageRef, pcPageRefAlloc, prgPageRef ) );
            Call( ErrAddPageRef( dbid, plrcreatemefdp->le_pgnoOE, pcPageRef, pcPageRefAlloc, prgPageRef, fTrue, fFalse ) );
            Call( ErrAddPageRef( dbid, plrcreatemefdp->le_pgnoAE, pcPageRef, pcPageRefAlloc, prgPageRef, fTrue, fFalse ) );
            break;
        }

        case lrtypCreateSingleExtentFDP:
        {
            const LRCREATESEFDP *   plrcreatesefdp  = (LRCREATESEFDP*)plr;
            const DBID              dbid            = plrcreatesefdp->dbid;

            Call( ErrAddPageRef( dbid, plrcreatesefdp->le_pgno, pcPageRef, pcPageRefAlloc, prgPageRef, fTrue, fFalse ) );
            break;
        }

        case lrtypConvertFDP:
        case lrtypConvertFDP2:
        {
            const LRCONVERTFDP *    plrconvertfdp   = (LRCONVERTFDP*)plr;
            const DBID              dbid            = plrconvertfdp->dbid;

            Call( ErrAddPageRef( dbid, plrconvertfdp->le_pgno, pcPageRef, pcPageRefAlloc, prgPageRef ) );
            Call( ErrAddPageRef( dbid, plrconvertfdp->le_pgnoOE, pcPageRef, pcPageRefAlloc, prgPageRef, fTrue, fFalse ) );
            Call( ErrAddPageRef( dbid, plrconvertfdp->le_pgnoAE, pcPageRef, pcPageRefAlloc, prgPageRef, fTrue, fFalse ) );
            break;
        }

        case lrtypEmptyTree:
        {
            const LREMPTYTREE *     plremptytree    = (LREMPTYTREE *)plr;
            const DBID              dbid            = plremptytree->dbid;
            const CPG               cpgToFree       = plremptytree->CbEmptyPageList() / sizeof( EMPTYPAGE );
            const EMPTYPAGE*        rgemptypage     = (EMPTYPAGE*)plremptytree->rgb;

            for ( CPG ipgToFree = 0; ipgToFree < cpgToFree; ipgToFree++ )
            {
                Call( ErrAddPageRef( dbid, rgemptypage[ipgToFree].pgno, pcPageRef, pcPageRefAlloc, prgPageRef ) );
            }
            break;
        }

        case lrtypPageMove:
        {
            const LRPAGEMOVE * const    plrpagemove     = LRPAGEMOVE::PlrpagemoveFromLr( plr );
            const DBID                  dbid            = plrpagemove->Dbid();
    
            if ( plrpagemove->PgnoSource() != pgnoNull )
            {
                Call( ErrAddPageRef( dbid, plrpagemove->PgnoSource(), pcPageRef, pcPageRefAlloc, prgPageRef ) );
            }
            if ( plrpagemove->PgnoDest() != pgnoNull )
            {
                Call( ErrAddPageRef( dbid, plrpagemove->PgnoDest(), pcPageRef, pcPageRefAlloc, prgPageRef, fTrue, fFalse ) );
            }
            if ( plrpagemove->PgnoParent() != pgnoNull )
            {
                Call( ErrAddPageRef( dbid, plrpagemove->PgnoParent(), pcPageRef, pcPageRefAlloc, prgPageRef ) );
            }
            if ( plrpagemove->PgnoLeft() != pgnoNull )
            {
                Call( ErrAddPageRef( dbid, plrpagemove->PgnoLeft(), pcPageRef, pcPageRefAlloc, prgPageRef ) );
            }
            if ( plrpagemove->PgnoRight() != pgnoNull )
            {
                Call( ErrAddPageRef( dbid, plrpagemove->PgnoRight(), pcPageRef, pcPageRefAlloc, prgPageRef ) );
            }
            break;
        }

        case lrtypRootPageMove:
        {
            break;
        }

        case lrtypMacroInfo:
            break;

        case lrtypScanCheck:
        {
            const LRSCANCHECK * const   plrscancheck    = (LRSCANCHECK *)plr;
            const DBID                  dbid            = plrscancheck->Dbid();

            Call( ErrAddPageRef( dbid, plrscancheck->Pgno(), pcPageRef, pcPageRefAlloc, prgPageRef, fFalse, fTrue ) );
            break;
        }

        case lrtypScanCheck2:
        {
            const LRSCANCHECK2 * const  plrscancheck   = (LRSCANCHECK2 *)plr;
            const DBID                  dbid            = plrscancheck->Dbid();

            Call( ErrAddPageRef( dbid, plrscancheck->Pgno(), pcPageRef, pcPageRefAlloc, prgPageRef, fFalse, fTrue ) );
            break;
        }

        case lrtypNewPage:
        {
            const LRNEWPAGE * const plrnewpage = (LRNEWPAGE *)plr;
            Call( ErrAddPageRef( plrnewpage->Dbid(), plrnewpage->Pgno(), pcPageRef, pcPageRefAlloc, prgPageRef, fTrue, fFalse ) );
            break;
        }

        case lrtypMacroInfo2:
            break;

        case lrtypExtendDB:
        case lrtypCommitCtx:
        case lrtypNOP2:
        case lrtypReAttach:
        case lrtypFreeFDP:
        case lrtypIgnored9:
        case lrtypIgnored10:
        case lrtypIgnored11:
        case lrtypIgnored12:
        case lrtypIgnored13:
        case lrtypIgnored14:
        case lrtypIgnored15:
        case lrtypIgnored16:
        case lrtypIgnored17:
        case lrtypIgnored18:
        case lrtypIgnored19:
            break;

        case lrtypShrinkDB:
        case lrtypShrinkDB2:
        case lrtypShrinkDB3:
        case lrtypTrimDB:
        case lrtypSignalAttachDb:
            break;

        case lrtypExtentFreed:
        {
            const LREXTENTFREED * const   plrextentfreed    = (LREXTENTFREED *)plr;
            const DBID                  dbid                = plrextentfreed->Dbid();

            PGNO pgnoFirst = plrextentfreed->PgnoFirst();
            CPG  cpgExtent = plrextentfreed->CpgExtent();

            for( INT i = 0; i < cpgExtent; ++i )
            {
                Call( ErrAddPageRef( dbid, pgnoFirst + i, pcPageRef, pcPageRefAlloc, prgPageRef, fFalse, fTrue ) );
            }

            break;
        }

        default:
            break;
    }

HandleError:
    return err;
}

#if defined( DEBUG ) && defined( ENABLE_JET_UNIT_TEST )

JETUNITTEST( LRPAGEPATCHREQUEST, ConstructorSetsLrtyp )
{
    LRPAGEPATCHREQUEST lr;
    CHECK(lrtypPagePatchRequest == lr.lrtyp);
}

JETUNITTEST( LRPAGEPATCHREQUEST, ConstructorZeroesMembers )
{
    LRPAGEPATCHREQUEST lr;
    CHECK(0 == lr.Dbid());
    CHECK(0 == lr.Pgno());
    CHECK(0 == lr.Dbtime());
}

class TESTLRPAGEPATCHREQUEST : public LRPAGEPATCHREQUEST
{
public:
    const BYTE * PbReserved() const { return m_rgbReserved; }
    const INT CbReserved() const { return sizeof(m_rgbReserved); }
};

JETUNITTEST( LRPAGEPATCHREQUEST, HasReservedData )
{
    const TESTLRPAGEPATCHREQUEST lr;
    CHECK(lr.CbReserved() > 0);
}

JETUNITTEST( LRPAGEPATCHREQUEST, ConstructorZeroesReservedData )
{
    const TESTLRPAGEPATCHREQUEST lr;
    const BYTE * const pb = lr.PbReserved();
    const INT cb = lr.CbReserved();
    for(INT i = 0; i < cb; ++i)
    {
        CHECK(0 == pb[i]);
    }
}

JETUNITTEST( LRPAGEPATCHREQUEST, SetDbid )
{
    const DBID dbid = 1;
    LRPAGEPATCHREQUEST lr;
    lr.SetDbid(dbid);
    CHECK(dbid == lr.Dbid());
}

JETUNITTEST( LRPAGEPATCHREQUEST, SetPgno )
{
    const PGNO pgno = 2;
    LRPAGEPATCHREQUEST lr;
    lr.SetPgno(pgno);
    CHECK(pgno == lr.Pgno());
}

JETUNITTEST( LRPAGEPATCHREQUEST, SetDbtime )
{
    const DBTIME dbtime = 0xABCDABCDF;
    LRPAGEPATCHREQUEST lr;
    lr.SetDbtime(dbtime);
    CHECK(dbtime == lr.Dbtime());
}

JETUNITTEST( LRPAGEPATCHREQUEST, LrToSz )
{
    LRPAGEPATCHREQUEST lr;
    lr.SetDbtime(0x1234a);
    lr.SetDbid(0x1);
    lr.SetPgno(0x2);

    char szLR[1024];
    LrToSz(&lr, szLR, sizeof(szLR), NULL);
    CHECK(0 == strcmp(" PagePatch 1234a,[1:2]", szLR));
}

JETUNITTEST( LREXTENDDB, ConstructorSetsLrtyp )
{
    LREXTENDDB lr;
    LRIGNORED *plrT = (LRIGNORED *)&lr;
    
    CHECK( lrtypExtendDB == plrT->lrtyp );
}

JETUNITTEST( LREXTENDDB, TestForwardCompatibility )
{

    LREXTENDDB lr;
    LRIGNORED *plrT = (LRIGNORED *)&lr;
    
    CHECK( plrT->lrtyp >= lrtypMacroInfo );
    CHECK( plrT->lrtyp <= lrtypIgnored19 );
    CHECK( plrT->Cb() + sizeof(LRIGNORED) == sizeof(LREXTENDDB) );
}

JETUNITTEST( LREXTENDDB, SetDbid )
{
    LREXTENDDB lr;

    const DBID dbid = dbidMax;
    lr.SetDbid( dbid );
    CHECK( dbid == lr.Dbid() );
}

JETUNITTEST( LREXTENDDB, SetPgnoLast )
{
    LREXTENDDB lr;

    const PGNO pgno = 4321;
    lr.SetPgnoLast( pgno );
    CHECK( pgno == lr.PgnoLast() );
}

JETUNITTEST( LREXTENDDB, LrToSz )
{
    LREXTENDDB lr;

    lr.SetDbid( 7 );
    lr.SetPgnoLast( 1234 );

    char szLR[1024];
    LrToSz( (LR *)&lr, szLR, sizeof(szLR), NULL );
    CHECK( 0 == strcmp(" ExtendDB  [7:1234]", szLR ) );
}

JETUNITTEST( LRSHRINKDB, ConstructorSetsLrtypShrink1 )
{
    LRSHRINKDB lr( lrtypShrinkDB );
    
    CHECK( lrtypShrinkDB == lr.lrtyp );
}

JETUNITTEST( LRSHRINKDB, ConstructorSetsLrtypShrink2 )
{
    LRSHRINKDB lr( lrtypShrinkDB2 );
    
    CHECK( lrtypShrinkDB2 == lr.lrtyp );
}

JETUNITTEST( LRSHRINKDB3, ConstructorSetsLrtypShrink3 )
{
    LRSHRINKDB3 lr;

    CHECK( lrtypShrinkDB3 == lr.lrtyp );
    CHECK( dbtimeNil == lr.Dbtime() );
}

JETUNITTEST( LRSHRINKDB3, CopyConstructorLrtypShrink3SetsLrtypShrink1 )
{
    LRSHRINKDB lr( lrtypShrinkDB );
    lr.SetDbid( 7 );
    lr.SetPgnoLast( 1234 );
    lr.SetCpgShrunk( 567 );

    LRSHRINKDB3 lr3( &lr );

    CHECK( lrtypShrinkDB == lr3.lrtyp );
    CHECK( 7 == lr3.Dbid() );
    CHECK( 1234 == lr3.PgnoLast() );
    CHECK( 567 == lr3.CpgShrunk() );
    CHECK( dbtimeNil == lr3.Dbtime() );
}

JETUNITTEST( LRSHRINKDB3, CopyConstructorLrtypShrink3SetsLrtypShrink2 )
{
    LRSHRINKDB lr( lrtypShrinkDB2 );
    lr.SetDbid( 7 );
    lr.SetPgnoLast( 1234 );
    lr.SetCpgShrunk( 567 );

    LRSHRINKDB3 lr3( &lr );

    CHECK( lrtypShrinkDB2 == lr3.lrtyp );
    CHECK( 7 == lr3.Dbid() );
    CHECK( 1234 == lr3.PgnoLast() );
    CHECK( 567 == lr3.CpgShrunk() );
    CHECK( dbtimeNil == lr3.Dbtime() );
}

JETUNITTEST( LRSHRINKDB3, CopyConstructorLrtypShrink3SetsLrtypShrink3 )
{
    LRSHRINKDB3 lr3a;
    lr3a.SetDbid( 7 );
    lr3a.SetPgnoLast( 1234 );
    lr3a.SetCpgShrunk( 567 );
    lr3a.SetDbtime( 8910 );

    LRSHRINKDB3 lr3b( &lr3a );

    CHECK( lrtypShrinkDB3 == lr3b.lrtyp );
    CHECK( 7 == lr3b.Dbid() );
    CHECK( 1234 == lr3b.PgnoLast() );
    CHECK( 567 == lr3b.CpgShrunk() );
    CHECK( 8910 == lr3b.Dbtime() );
}

JETUNITTEST( LRSHRINKDB, SetDbid )
{
    LRSHRINKDB lr( lrtypShrinkDB2 );

    const DBID dbid = dbidMax;
    lr.SetDbid( dbid );
    CHECK( dbid == lr.Dbid() );
}

JETUNITTEST( LRSHRINKDB, SetPgnoLast )
{
    LRSHRINKDB lr( lrtypShrinkDB2 );

    const PGNO pgno = 4321;
    lr.SetPgnoLast( pgno );
    CHECK( pgno == lr.PgnoLast() );
}

JETUNITTEST( LRSHRINKDB, SetCpgShrunk )
{
    LRSHRINKDB lr( lrtypShrinkDB2 );

    const CPG cpg = 567;
    lr.SetCpgShrunk( cpg );
    CHECK( cpg == lr.CpgShrunk() );
}

JETUNITTEST( LRSHRINKDB3, SetDbtime )
{
    LRSHRINKDB3 lr;

    const DBTIME dbtime = 8910;
    lr.SetDbtime( dbtime );
    CHECK( dbtime == lr.Dbtime() );
}

JETUNITTEST( LRSHRINKDB, LrToSzShrink1 )
{
    LRSHRINKDB lr( lrtypShrinkDB );

    lr.SetDbid( 7 );
    lr.SetPgnoLast( 1234 );
    lr.SetCpgShrunk( 567 );

    char szLR[1024];
    LrToSz( (LR *)&lr, szLR, sizeof(szLR), NULL );
    CHECK( 0 == strcmp(" ShrinkDB  [7:1234:567]", szLR ) );
}

JETUNITTEST( LRSHRINKDB, LrToSzShrink2 )
{
    LRSHRINKDB lr( lrtypShrinkDB2 );

    lr.SetDbid( 7 );
    lr.SetPgnoLast( 1234 );
    lr.SetCpgShrunk( 567 );

    char szLR[1024];
    LrToSz( (LR *)&lr, szLR, sizeof(szLR), NULL );
    CHECK( 0 == strcmp(" ShrinkDB2 [7:1234:567]", szLR ) );
}

JETUNITTEST( LRSHRINKDB3, LrToSzShrink3 )
{
    LRSHRINKDB3 lr;

    lr.SetDbid( 7 );
    lr.SetPgnoLast( 1234 );
    lr.SetCpgShrunk( 567 );
    lr.SetDbtime( 8910 );

    char szLR[1024];
    LrToSz( (LR *)&lr, szLR, sizeof(szLR), NULL );
    CHECK( 0 == strcmp(" ShrinkDB3 22ce[7:1234:567]", szLR ) );
}

JETUNITTEST( LRTRIMDB, ConstructorSetsLrtyp )
{
    LRTRIMDB lr;

    CHECK( lrtypTrimDB == lr.lrtyp );
}

JETUNITTEST( LRTRIMDB, SetDbid )
{
    LRTRIMDB lr;

    const DBID dbid = dbidMax;
    lr.SetDbid( dbid );
    CHECK( dbid == lr.Dbid() );
}

JETUNITTEST( LRTRIMDB, SetPgnoStartZeroes )
{
    LRTRIMDB lr;

    const PGNO pgno = 4321;
    lr.SetPgnoStartZeroes( pgno );
    CHECK( pgno == lr.PgnoStartZeroes() );
}

JETUNITTEST( LRTRIMDB, SetCpgZeroLength )
{
    LRTRIMDB lr;

    const PGNO pgno = 43210;
    lr.SetCpgZeroLength( pgno );
    CHECK( pgno == lr.CpgZeroLength() );
}

JETUNITTEST( LRTRIMDB, LrToSz )
{
    LRTRIMDB lr;

    lr.SetDbid( 7 );
    lr.SetPgnoStartZeroes( 1234 );
    lr.SetCpgZeroLength( 5 );

    char szLR[1024];
    LrToSz( (LR *)&lr, szLR, sizeof(szLR), NULL );
    printf( "((%s))\n", szLR );
    CHECK( 0 == strcmp(" TrimDB    [7:1234+5] ([7:1235] [7:1236] [7:1237] [7:1238] )", szLR ) );
}

#endif
