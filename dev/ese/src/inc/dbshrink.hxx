// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

struct ROOTMOVECHILD
    :   public CZeroInit
{
    ROOTMOVECHILD()
        :   CZeroInit( sizeof( ROOTMOVECHILD ) )
    {
        dbtimeBeforeChildFDP = dbtimeInvalid;
        pgnoChildFDP = pgnoNull;
        objidChild = objidNil;
        dataSphNew.SetPv( &sphNew );
        dataSphNew.SetCb( sizeof( sphNew ) );
        prootMoveChildNext = NULL;
    }
    ~ROOTMOVECHILD()
    {
        csrChildFDP.ReleasePage();

        ASSERT_VALID_OBJ( dataSphNew );
    }

    VOID AssertValid() const
    {
        Assert( ( dbtimeBeforeChildFDP != dbtimeNil ) && ( dbtimeBeforeChildFDP != dbtimeInvalid ) );
        Assert( pgnoChildFDP != pgnoNull );
        Assert( objidChild != objidNil );
        ASSERT_VALID_OBJ( dataSphNew );
        Assert( dataSphNew.Cb() == sizeof( sphNew ) );
        if ( csrChildFDP.Latch() == latchWrite )
        {
            Assert( pgnoChildFDP == csrChildFDP.Pgno() );
            Assert( objidChild == csrChildFDP.Cpage().ObjidFDP() );
        }
    }

    DBTIME          dbtimeBeforeChildFDP;
    ULONG           fFlagsBeforeChildFDP;

    CSR             csrChildFDP;
    PGNO            pgnoChildFDP;
    OBJID           objidChild;
    SPACE_HEADER    sphNew;
    DATA            dataSphNew;

    ROOTMOVECHILD*  prootMoveChildNext;
};

struct ROOTMOVE
    :   public CZeroInit
{
    ROOTMOVE()
        :   CZeroInit( sizeof( ROOTMOVE ) )
    {
        dbtimeAfter = dbtimeInvalid;

        dbtimeBeforeFDP = dbtimeInvalid;
        dbtimeBeforeOE = dbtimeInvalid;
        dbtimeBeforeAE = dbtimeInvalid;

        pgnoFDP = pgnoNull;
        pgnoOE = pgnoNull;
        pgnoAE = pgnoNull;
        pgnoNewFDP = pgnoNull;
        pgnoNewOE = pgnoNull;
        pgnoNewAE = pgnoNull;

        for ( int iCat = 0; iCat < 2; iCat++ )
        {
            dbtimeBeforeCatObj[iCat] = dbtimeInvalid;
            pgnoCatObj[iCat] = pgnoNull;
            ilineCatObj[iCat] = -1;
            dbtimeBeforeCatClustIdx[iCat] = dbtimeInvalid;
            pgnoCatObj[iCat] = pgnoNull;
            ilineCatClustIdx[iCat] = -1;
            dataBeforeCatObj[iCat].Nullify();
            dataBeforeCatClustIdx[iCat].Nullify();
            dataNewCatObj[iCat].Nullify();
            dataNewCatClustIdx[iCat].Nullify();
        }

        dataBeforeFDP.Nullify();
        dataBeforeOE.Nullify();
        dataBeforeAE.Nullify();

        prootMoveChildren = NULL;
    }
    ~ROOTMOVE()
    {
        ReleaseResources();
    }

    VOID AssertValid( const BOOL fBeforeMove, const BOOL fRedo ) const
    {
        Assert( ( dbtimeAfter != dbtimeNil ) && ( dbtimeAfter != dbtimeInvalid ) );
        Assert( ( dbtimeBeforeFDP != dbtimeNil ) && ( dbtimeBeforeFDP != dbtimeInvalid ) );
        Assert( ( dbtimeBeforeOE != dbtimeNil ) && ( dbtimeBeforeOE != dbtimeInvalid ) );
        Assert( ( dbtimeBeforeAE != dbtimeNil ) && ( dbtimeBeforeAE != dbtimeInvalid ) );

        Assert( pgnoFDP != pgnoNull );
        Assert( pgnoOE != pgnoNull );
        Assert( pgnoAE != pgnoNull );
        Assert( ( pgnoNewFDP != pgnoNull ) && ( pgnoNewFDP != pgnoFDP ) );
        Assert( ( pgnoNewOE != pgnoNull ) && ( pgnoNewOE != pgnoOE ) );
        Assert( ( pgnoNewAE != pgnoNull ) && ( pgnoNewAE != pgnoAE ) );
        Assert( pgnoAE == ( pgnoOE + 1 ) );
        Assert( pgnoNewAE == ( pgnoNewOE + 1 ) );
        Expected( pgnoNewOE == ( pgnoNewFDP + 1 ) );

        Assert( objid != objidNil );

        ASSERT_VALID_OBJ( dataBeforeFDP );
        ASSERT_VALID_OBJ( dataBeforeOE );
        ASSERT_VALID_OBJ( dataBeforeAE );

        Assert( ( ( ( csrFDP.PagetrimState() == pagetrimTrimmed ) || ( csrFDP.Latch() == latchRIW ) ) && fRedo ) ||
                ( ( csrFDP.Latch() == latchWrite ) && csrFDP.FDirty() ) );
        if ( csrFDP.PagetrimState() != pagetrimTrimmed )
        {
            Assert( ( ( csrFDP.Latch() == latchRIW ) &&
                        ( ( csrFDP.Dbtime() >= dbtimeAfter ) || ( csrFDP.Dbtime() < dbtimeBeforeFDP ) ) ) ||
                    ( ( csrFDP.Latch() == latchWrite ) &&
                        ( csrFDP.Dbtime() == dbtimeAfter ) &&
                        ( pgnoFDP == csrFDP.Pgno() ) &&
                        ( objid == csrFDP.Cpage().ObjidFDP() ) ) );
        }

        Assert( ( ( ( csrOE.PagetrimState() == pagetrimTrimmed ) || ( csrOE.Latch() == latchRIW ) ) && fRedo ) ||
                ( ( csrOE.Latch() == latchWrite ) && csrOE.FDirty() ) );
        if ( csrOE.PagetrimState() != pagetrimTrimmed )
        {
            Assert( ( ( csrOE.Latch() == latchRIW ) &&
                        ( ( csrOE.Dbtime() >= dbtimeAfter ) || ( csrOE.Dbtime() < dbtimeBeforeOE ) ) ) ||
                    ( ( csrOE.Latch() == latchWrite ) &&
                        ( csrOE.Dbtime() == dbtimeAfter ) &&
                        ( pgnoOE == csrOE.Pgno() ) &&
                        ( objid == csrOE.Cpage().ObjidFDP() ) ) );
        }

        Assert( ( ( ( csrAE.PagetrimState() == pagetrimTrimmed ) || ( csrAE.Latch() == latchRIW ) ) && fRedo ) ||
                ( ( csrAE.Latch() == latchWrite ) && csrAE.FDirty() ) );
        if ( csrAE.PagetrimState() != pagetrimTrimmed )
        {
            Assert( ( ( csrAE.Latch() == latchRIW ) &&
                        ( ( csrAE.Dbtime() >= dbtimeAfter ) || ( csrAE.Dbtime() < dbtimeBeforeAE ) ) ) ||
                    ( ( csrAE.Latch() == latchWrite ) &&
                        ( csrAE.Dbtime() == dbtimeAfter ) &&
                        ( pgnoAE == csrAE.Pgno() ) &&
                        ( objid == csrAE.Cpage().ObjidFDP() ) ) );
        }

        if ( csrNewFDP.PagetrimState() != pagetrimTrimmed )
        {
            Assert( ( ( csrNewFDP.Latch() == latchRIW ) && ( dataBeforeFDP.Cb() == 0 ) && fRedo ) ||
                    ( ( csrNewFDP.Latch() == latchWrite ) && csrNewFDP.FDirty() && ( csrNewFDP.Dbtime() == dbtimeAfter ) && ( dataBeforeFDP.Cb() == g_cbPage ) ) );
        }
        else
        {
            Assert( fRedo );
            Assert( dataBeforeFDP.Cb() == 0 );
        }

        if ( csrNewOE.PagetrimState() != pagetrimTrimmed )
        {
            Assert( ( ( csrNewOE.Latch() == latchRIW ) && ( dataBeforeOE.Cb() == 0 ) && fRedo ) ||
                    ( ( csrNewOE.Latch() == latchWrite ) && csrNewOE.FDirty() && ( csrNewOE.Dbtime() == dbtimeAfter ) && ( dataBeforeOE.Cb() == g_cbPage ) ) );
        }
        else
        {
            Assert( fRedo );
            Assert( dataBeforeOE.Cb() == 0 );
        }

        if ( csrNewAE.PagetrimState() != pagetrimTrimmed )
        {
            Assert( ( ( csrNewAE.Latch() == latchRIW ) && ( dataBeforeAE.Cb() == 0 ) && fRedo ) ||
                    ( ( csrNewAE.Latch() == latchWrite ) && csrNewAE.FDirty() && ( csrNewAE.Dbtime() == dbtimeAfter ) && ( dataBeforeAE.Cb() == g_cbPage ) ) );
        }
        else
        {
            Assert( fRedo );
            Assert( dataBeforeAE.Cb() == 0 );
        }

        if ( !fRedo || !fBeforeMove )
        {
            if ( csrFDP.Latch() == latchWrite )
            {
                Assert( !csrFDP.Cpage().FPreInitPage() );
                Assert( csrFDP.Cpage().FRootPage() );
                Assert( csrFDP.Cpage().PgnoNext() == pgnoNull );
                Assert( csrFDP.Cpage().PgnoNext() == pgnoNull );
                Assert( fBeforeMove || csrFDP.Cpage().FEmptyPage() );
            }
            if ( csrNewFDP.Latch() == latchWrite )
            {
                Assert( !!fBeforeMove == !!csrNewFDP.Cpage().FPreInitPage() );
                Assert( !!fBeforeMove == !csrNewFDP.Cpage().FRootPage() );
                Assert( csrNewFDP.Cpage().PgnoNext() == pgnoNull );
                Assert( csrNewFDP.Cpage().PgnoNext() == pgnoNull );
            }
            if ( ( csrFDP.Latch() == latchWrite ) && ( csrNewFDP.Latch() == latchWrite ) )
            {
                Assert( fBeforeMove ||
                       ( csrFDP.Cpage().FFlags() & ~CPAGE::fPageEmpty & ~CPAGE::maskFlushType & ~CPAGE::fPageNewChecksumFormat ) ==
                       ( csrNewFDP.Cpage().FFlags() & ~CPAGE::maskFlushType & ~CPAGE::fPageNewChecksumFormat ) );
            }

            if ( csrOE.Latch() == latchWrite )
            {
                Assert( !csrOE.Cpage().FPreInitPage() );
                Assert( csrOE.Cpage().FRootPage() );
                Assert( csrOE.Cpage().FSpaceTree() );
                Assert( csrOE.Cpage().PgnoNext() == pgnoNull );
                Assert( csrOE.Cpage().PgnoNext() == pgnoNull );
                Assert( fBeforeMove || csrOE.Cpage().FEmptyPage() );
            }
            if ( csrNewOE.Latch() == latchWrite )
            {
                Assert( !!fBeforeMove == !!csrNewOE.Cpage().FPreInitPage() );
                Assert( !!fBeforeMove == !csrNewOE.Cpage().FRootPage() );
                Assert( !!fBeforeMove == !csrNewOE.Cpage().FSpaceTree() );
                Assert( csrNewOE.Cpage().PgnoNext() == pgnoNull );
                Assert( csrNewOE.Cpage().PgnoNext() == pgnoNull );
            }
            if ( ( csrOE.Latch() == latchWrite ) && ( csrNewOE.Latch() == latchWrite ) )
            {
                Assert( fBeforeMove ||
                       ( csrOE.Cpage().FFlags() & ~CPAGE::fPageEmpty & ~CPAGE::maskFlushType & ~CPAGE::fPageNewChecksumFormat ) ==
                       ( csrNewOE.Cpage().FFlags() & ~CPAGE::maskFlushType & ~CPAGE::fPageNewChecksumFormat ) );
            }

            if ( csrAE.Latch() == latchWrite )
            {
                Assert( !csrAE.Cpage().FPreInitPage() );
                Assert( csrAE.Cpage().FRootPage() );
                Assert( csrAE.Cpage().FSpaceTree() );
                Assert( csrAE.Cpage().PgnoNext() == pgnoNull );
                Assert( csrAE.Cpage().PgnoNext() == pgnoNull );
                Assert( fBeforeMove || csrAE.Cpage().FEmptyPage() );
            }
            if ( csrNewAE.Latch() == latchWrite )
            {
                Assert( !!fBeforeMove == !!csrNewAE.Cpage().FPreInitPage() );
                Assert( !!fBeforeMove == !csrNewAE.Cpage().FRootPage() );
                Assert( !!fBeforeMove == !csrNewAE.Cpage().FSpaceTree() );
                Assert( csrNewAE.Cpage().PgnoNext() == pgnoNull );
                Assert( csrNewAE.Cpage().PgnoNext() == pgnoNull );
            }
            if ( ( csrAE.Latch() == latchWrite ) && ( csrNewAE.Latch() == latchWrite ) )
            {
                Assert( fBeforeMove ||
                       ( csrAE.Cpage().FFlags() & ~CPAGE::fPageEmpty & ~CPAGE::maskFlushType & ~CPAGE::fPageNewChecksumFormat ) ==
                       ( csrNewAE.Cpage().FFlags() & ~CPAGE::maskFlushType & ~CPAGE::fPageNewChecksumFormat ) );
            }
        }

        for ( int iCat = 0; iCat < 2; iCat++ )
        {
            Assert( ( dbtimeBeforeCatObj[iCat] != dbtimeNil ) && ( dbtimeBeforeCatObj[iCat] != dbtimeInvalid ) );

            Assert( pgnoCatObj[iCat] != pgnoNull );
            Assert( ilineCatObj[iCat] >= 0 );

            ASSERT_VALID_OBJ( dataBeforeCatObj[iCat] );
            Assert( ( dataBeforeCatObj[iCat].Cb() == 0 ) || ( dataBeforeCatObj[iCat].Cb() < g_cbPage ) );
            ASSERT_VALID_OBJ( dataNewCatObj[iCat] );
            Assert( ( ( ( csrCatObj[iCat].PagetrimState() == pagetrimTrimmed ) || ( csrCatObj[iCat].Latch() == latchRIW ) ) &&
                      ( dataNewCatObj[iCat].Cb() == 0 ) && fRedo ) ||
                    ( ( csrCatObj[iCat].Latch() == latchWrite ) &&
                      csrCatObj[iCat].FDirty() &&
                      ( dataNewCatObj[iCat].Cb() > 0 ) &&
                      ( dataNewCatObj[iCat].Cb() < g_cbPage ) ) );
            if ( csrCatObj[iCat].PagetrimState() != pagetrimTrimmed )
            {
                Assert( ( ( csrCatObj[iCat].Latch() == latchRIW ) &&
                            ( ( csrCatObj[iCat].Dbtime() >= dbtimeAfter ) || ( csrCatObj[iCat].Dbtime() < dbtimeBeforeCatObj[iCat] ) ) ) ||
                        ( ( csrCatObj[iCat].Latch() == latchWrite ) &&
                            ( csrCatObj[iCat].Dbtime() == dbtimeAfter ) &&
                            ( pgnoCatObj[iCat] == csrCatObj[iCat].Pgno() ) ) );
            }
            else
            {
                Assert( fRedo );
            }

            Assert( ilineCatClustIdx[iCat] >= -1 );
            ASSERT_VALID_OBJ( dataBeforeCatClustIdx[iCat] );
            ASSERT_VALID_OBJ( dataNewCatClustIdx[iCat] );
            if ( ( ilineCatClustIdx[iCat] != -1 ) && ( pgnoCatClustIdx[iCat] != pgnoCatObj[iCat] ) )
            {
                Assert( ( dbtimeBeforeCatClustIdx[iCat] != dbtimeNil ) && ( dbtimeBeforeCatClustIdx[iCat] != dbtimeInvalid ) );

                Assert( pgnoCatClustIdx[iCat] != pgnoNull );

                Assert( ( dataBeforeCatClustIdx[iCat].Cb() == 0 ) || ( dataBeforeCatClustIdx[iCat].Cb() < g_cbPage ) );
                Assert( ( ( ( csrCatClustIdx[iCat].PagetrimState() == pagetrimTrimmed ) || ( csrCatClustIdx[iCat].Latch() == latchRIW ) ) &&
                          ( dataNewCatClustIdx[iCat].Cb() == 0 ) && fRedo ) ||
                        ( ( csrCatClustIdx[iCat].Latch() == latchWrite ) &&
                          csrCatClustIdx[iCat].FDirty() &&
                          ( dataNewCatClustIdx[iCat].Cb() > 0 ) &&
                          ( dataNewCatClustIdx[iCat].Cb() < g_cbPage ) ) );

                if ( csrCatClustIdx[iCat].PagetrimState() != pagetrimTrimmed )
                {
                    Assert( ( ( csrCatClustIdx[iCat].Latch() == latchRIW ) &&
                                ( ( csrCatClustIdx[iCat].Dbtime() >= dbtimeAfter ) || ( csrCatClustIdx[iCat].Dbtime() < dbtimeBeforeCatClustIdx[iCat] ) ) ) ||
                            ( ( csrCatClustIdx[iCat].Latch() == latchWrite ) &&
                                ( csrCatClustIdx[iCat].Dbtime() == dbtimeAfter ) &&
                                ( pgnoCatClustIdx[iCat] == csrCatClustIdx[iCat].Pgno() ) ) );
                }
                else
                {
                    Assert( fRedo );
                }
            }
            else if ( ilineCatClustIdx[iCat] == -1 )
            {
                Assert( pgnoCatClustIdx[iCat] == pgnoNull );
                Assert( dbtimeBeforeCatClustIdx[iCat] == dbtimeInvalid );
                Assert( dataBeforeCatClustIdx[iCat].Cb() == 0 );
                Assert( dataNewCatClustIdx[iCat].Cb() == 0 );
                Assert( !csrCatClustIdx[iCat].FLatched() );
            }
            else
            {
                Assert( pgnoCatClustIdx[iCat] == pgnoCatObj[iCat] );
                Assert( dbtimeBeforeCatClustIdx[iCat] == dbtimeBeforeCatObj[iCat] );

                Assert( !csrCatClustIdx[iCat].FLatched() );
                Assert( ( dataBeforeCatClustIdx[iCat].Cb() == 0 ) || ( dataBeforeCatClustIdx[iCat].Cb() < g_cbPage ) );
                Assert( ( ( ( csrCatObj[iCat].PagetrimState() == pagetrimTrimmed ) || ( csrCatObj[iCat].Latch() == latchRIW ) ) &&
                          ( dataNewCatClustIdx[iCat].Cb() == 0 ) && fRedo ) ||
                        ( ( csrCatObj[iCat].Latch() == latchWrite ) &&
                          csrCatObj[iCat].FDirty() &&
                          ( dataNewCatClustIdx[iCat].Cb() > 0 ) &&
                          ( dataNewCatClustIdx[iCat].Cb() < g_cbPage ) ) );
            }

            if ( iCat == 1 )
            {
                Assert( pgnoCatObj[0] != pgnoCatObj[1] );
                Assert( ( ilineCatClustIdx[1] != -1 ) == ( pgnoCatClustIdx[0] != pgnoCatClustIdx[1] ) );
            }
        }

        for ( ROOTMOVECHILD* prmc = prootMoveChildren;
                prmc != NULL;
                prmc = prmc->prootMoveChildNext )
        {
            ASSERT_VALID( prmc );
            Assert( dbtimeAfter > prmc->dbtimeBeforeChildFDP );
            Assert( prmc->pgnoChildFDP != pgnoFDP );
            Assert( prmc->objidChild != objid );

            if ( prmc->csrChildFDP.PagetrimState() != pagetrimTrimmed )
            {
                Assert( ( ( prmc->csrChildFDP.Latch() == latchRIW ) && fRedo &&
                            ( ( prmc->csrChildFDP.Dbtime() >= dbtimeAfter ) || ( prmc->csrChildFDP.Dbtime() < prmc->dbtimeBeforeChildFDP ) ) ) ||
                        ( ( prmc->csrChildFDP.Latch() == latchWrite ) && ( prmc->csrChildFDP.Dbtime() == dbtimeAfter ) ) );
            }
            else
            {
                Assert( fRedo );
            }
        }
    }

    DBTIME          dbtimeAfter;

    DBTIME          dbtimeBeforeFDP;
    ULONG           fFlagsBeforeFDP;

    CSR             csrFDP;
    CSR             csrNewFDP;
    DATA            dataBeforeFDP;
    PGNO            pgnoFDP;
    PGNO            pgnoNewFDP;
    OBJID           objid;

    DBTIME          dbtimeBeforeOE;
    ULONG           fFlagsBeforeOE;

    CSR             csrOE;
    CSR             csrNewOE;
    DATA            dataBeforeOE;
    PGNO            pgnoOE;
    PGNO            pgnoNewOE;

    DBTIME          dbtimeBeforeAE;
    ULONG           fFlagsBeforeAE;

    CSR             csrAE;
    CSR             csrNewAE;
    DATA            dataBeforeAE;
    PGNO            pgnoAE;
    PGNO            pgnoNewAE;


    DBTIME          dbtimeBeforeCatObj[2];
    ULONG           fFlagsBeforeCatObj[2];

    CSR             csrCatObj[2];
    PGNO            pgnoCatObj[2];
    INT             ilineCatObj[2];
    DATA            dataBeforeCatObj[2];
    DATA            dataNewCatObj[2];

    DBTIME          dbtimeBeforeCatClustIdx[2];
    ULONG           fFlagsBeforeCatClustIdx[2];

    CSR             csrCatClustIdx[2];
    PGNO            pgnoCatClustIdx[2];
    INT             ilineCatClustIdx[2];
    DATA            dataBeforeCatClustIdx[2];
    DATA            dataNewCatClustIdx[2];

    ROOTMOVECHILD*  prootMoveChildren;

    VOID AddRootMoveChild( ROOTMOVECHILD* const prmc )
    {
        prmc->prootMoveChildNext = prootMoveChildren;
        prootMoveChildren = prmc;
    }

    VOID ReleaseResources()
    {
        csrFDP.ReleasePage();
        csrNewFDP.ReleasePage();
        csrOE.ReleasePage();
        csrNewOE.ReleasePage();
        csrAE.ReleasePage();
        csrNewAE.ReleasePage();
        for ( int iCat = 0; iCat < 2; iCat++ )
        {
            csrCatObj[iCat].ReleasePage();
            csrCatClustIdx[iCat].ReleasePage();

            ASSERT_VALID_OBJ( dataBeforeCatObj[iCat] );
            dataBeforeCatObj[iCat].Nullify();
            ASSERT_VALID_OBJ( dataBeforeCatClustIdx[iCat] );
            dataBeforeCatClustIdx[iCat].Nullify();
            ASSERT_VALID_OBJ( dataNewCatObj[iCat] );
            BFFree( dataNewCatObj[iCat].Pv() );
            dataNewCatObj[iCat].Nullify();
            ASSERT_VALID_OBJ( dataNewCatClustIdx[iCat] );
            BFFree( dataNewCatClustIdx[iCat].Pv() );
            dataNewCatClustIdx[iCat].Nullify();
        }

        ASSERT_VALID_OBJ( dataBeforeFDP );
        BFFree( dataBeforeFDP.Pv() );
        dataBeforeFDP.Nullify();
        ASSERT_VALID_OBJ( dataBeforeOE );
        BFFree( dataBeforeOE.Pv() );
        dataBeforeOE.Nullify();
        ASSERT_VALID_OBJ( dataBeforeAE );
        BFFree( dataBeforeAE.Pv() );
        dataBeforeAE.Nullify();

        ROOTMOVECHILD* prmc = prootMoveChildren;
        while ( prmc != NULL )
        {
            ROOTMOVECHILD* const prootMoveChildNext = prmc->prootMoveChildNext;
            delete prmc;
            prmc = prootMoveChildNext;
        }
        prootMoveChildren = NULL;
    }
};

ERR ErrSHKShrinkDbFromEof(
    _In_ PIB *ppib,
    _In_ const IFMP ifmp );

ERR ErrSHKRootPageMove(
    _In_ PIB* ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgnoFDP );

VOID SHKPerformRootMove(
    _In_ ROOTMOVE* const prm,
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const BOOL fRecoveryRedo );

