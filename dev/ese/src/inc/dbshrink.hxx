// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

struct ROOTMOVECHILD
    :   public CZeroInit
{
    // Ctr./Dtr.
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
        // Release latches.
        csrChildFDP.ReleasePage();

        ASSERT_VALID_OBJ( dataSphNew );
    }

    // Validation routine.
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

    // This dbtime/fFlags pair must be updated atomically so it can be reverted to
    // a consistent state if neeed.
    DBTIME          dbtimeBeforeChildFDP;   // Before dbtime of the root page of the child.
    ULONG           fFlagsBeforeChildFDP;   // Before flags on the root page of the child.

    CSR             csrChildFDP;            // Currency of the root page of the child.
    PGNO            pgnoChildFDP;           // Pgno of the root page of the child.
    OBJID           objidChild;             // OBJID of the child.
    SPACE_HEADER    sphNew;                 // Post-image of the space header.
    DATA            dataSphNew;             // Points to sphNew.

    ROOTMOVECHILD*  prootMoveChildNext;     // Next child tree.
};

struct ROOTMOVE
    :   public CZeroInit
{
    // Ctr./Dtr.
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

    // Validation routine.
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
            // If the page has latchRIW, it's either too new (no need to redo) or too old (added to dbtime mismatch map).
            // If the page has latchWrite, it must be ready with the new dbtime.
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

        // Catalog pages.
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
                // There is a clustered index object in a different catalog page.
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
                    // If the page has latchRIW, it's either too new (no need to redo) or too old (added to dbtime mismatch map).
                    // If the page has latchWrite, it must be ready with the new dbtime.
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
                // There isn't a clustered index object.
                Assert( pgnoCatClustIdx[iCat] == pgnoNull );
                Assert( dbtimeBeforeCatClustIdx[iCat] == dbtimeInvalid );
                Assert( dataBeforeCatClustIdx[iCat].Cb() == 0 );
                Assert( dataNewCatClustIdx[iCat].Cb() == 0 );
                Assert( !csrCatClustIdx[iCat].FLatched() );
            }
            else
            {
                // There is a clustered index object in the same catalog page.
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

        // Children objects.
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
                // If the page has latchRIW, it's either too new (no need to redo) or too old (added to dbtime mismatch map).
                // If the page has latchWrite, it must be ready with the new dbtime.
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

    DBTIME          dbtimeAfter;                // After dbtime for all pages involved: root, OE, AE, children roots,
                                                // affected catalog pages.

    // This dbtime/fFlags pair must be updated atomically so it can be reverted to
    // a consistent state if neeed.
    DBTIME          dbtimeBeforeFDP;            // Before dbtime of the root page.
    ULONG           fFlagsBeforeFDP;            // Before flags on the root page.

    CSR             csrFDP;                     // Currency of the root page.
    CSR             csrNewFDP;                  // Currency of the new page where the root page will be moved into.
    DATA            dataBeforeFDP;              // Pre-image of the root page.
    PGNO            pgnoFDP;                    // Original pgno of the root page.
    PGNO            pgnoNewFDP;                 // New pgno of the root page.
    OBJID           objid;                      // OBJID of the tree.

    // This dbtime/fFlags pair must be updated atomically so it can be reverted to
    // a consistent state if neeed.
    DBTIME          dbtimeBeforeOE;             // Before dbtime of the OE's root page.
    ULONG           fFlagsBeforeOE;             // Before flags on the OE's root page.

    CSR             csrOE;                      // Currency of the OE's root page.
    CSR             csrNewOE;                   // Currency of the new page where the OE's root page will be moved into.
    DATA            dataBeforeOE;               // Pre-image of the OE's root page.
    PGNO            pgnoOE;                     // Original pgno of the OE's root page.
    PGNO            pgnoNewOE;                  // New pgno of the OE's root page.

    // This dbtime/fFlags pair must be updated atomically so it can be reverted to
    // a consistent state if neeed.
    DBTIME          dbtimeBeforeAE;             // Before dbtime of the AE's root page.
    ULONG           fFlagsBeforeAE;             // Before flags on the AE's root page.

    CSR             csrAE;                      // Currency of the AE's root page.
    CSR             csrNewAE;                   // Currency of the new page where the AE's root page will be moved into.
    DATA            dataBeforeAE;               // Pre-image of the AE's root page.
    PGNO            pgnoAE;                     // Original pgno of the AE's root page.
    PGNO            pgnoNewAE;                  // New pgno of the AE's root page.

    // We need two catalog-related sets of members below because if this is a root object (table), there may
    // be two nodes describing its root in the catalog: a table object and an index object (clustered index).
    //
    // Also, each variable is an array of size 2 because we need to handle the shadow catalog as well.
    //
    // Therefore, we may see the two sets below in three different modes:
    //  1) Tree only has one node in the catalog: csrCatObj gets set, while csrCatClustIdx doesn't. All other "CatObj"
    //     variables are initialized, while their "CatClustIdx" counterparts aren't.
    //  2) Tree has two nodes in different pages of the catalog: both and csrCatClustIdx get set. All other "CatObj"
    //     and "CatClustIdx" variables are initialized.
    //  3) Tree has two nodes in the same page of the catalog: csrCatObj gets set, while csrCatClustIdx doesn't. All
    //     other "CatObj" and "CatClustIdx" variables are initialized. This is the crazy - despite common - case!
    //     Making two replaces to different records on same page is what makes it difficult to just use two CSRs
    //     (because both will want to hold a latch to the same page), and why we have to have ilines and manual
    //     replaces instead.

    // This dbtime/fFlags pair must be updated atomically so it can be reverted to
    // a consistent state if neeed.
    DBTIME          dbtimeBeforeCatObj[2];      // Before dbtime of the catalog page which contains the node describing this tree.
    ULONG           fFlagsBeforeCatObj[2];      // Before flags on the catalog page which contains the node describing this tree.

    CSR             csrCatObj[2];               // Currency of the catalog page which contains the node describing this tree.
    PGNO            pgnoCatObj[2];              // Catalog page which contains the node describing this tree.
    INT             ilineCatObj[2];             // Specific node within the catalog page which contains the node describing this tree.
    DATA            dataBeforeCatObj[2];        // Before-image of catalog node we need to update.
    DATA            dataNewCatObj[2];           // Post-image of catalog node we need to update.

    // This dbtime/fFlags pair must be updated atomically so it can be reverted to
    // a consistent state if neeed.
    DBTIME          dbtimeBeforeCatClustIdx[2]; // Before dbtime of the catalog page which contains the node describing the clustered index of this tree.
    ULONG           fFlagsBeforeCatClustIdx[2]; // Before flags on the catalog page which contains the node describing the clustered index of this tree.

    CSR             csrCatClustIdx[2];          // Currency of the catalog page which contains the node describing the clustered index of this tree.
    PGNO            pgnoCatClustIdx[2];         // Catalog page which contains the node describing the clustered index of this tree.
    INT             ilineCatClustIdx[2];        // Specific node within the catalog page which contains the node describing the clustered index of this tree.
    DATA            dataBeforeCatClustIdx[2];   // Before-image of catalog node we need to update.
    DATA            dataNewCatClustIdx[2];      // Post-image of catalog node we need to update.

    ROOTMOVECHILD*  prootMoveChildren;          // Linked list of children trees (e.g., secondary indexes, LV trees).

    // Helper to add a child object.
    VOID AddRootMoveChild( ROOTMOVECHILD* const prmc )
    {
        prmc->prootMoveChildNext = prootMoveChildren;
        prootMoveChildren = prmc;
    }

    // Helper to release all resources.
    VOID ReleaseResources()
    {
        // Release latches.
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

        // Free pages.
        ASSERT_VALID_OBJ( dataBeforeFDP );
        BFFree( dataBeforeFDP.Pv() );
        dataBeforeFDP.Nullify();
        ASSERT_VALID_OBJ( dataBeforeOE );
        BFFree( dataBeforeOE.Pv() );
        dataBeforeOE.Nullify();
        ASSERT_VALID_OBJ( dataBeforeAE );
        BFFree( dataBeforeAE.Pv() );
        dataBeforeAE.Nullify();

        // Free children objects.
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

// Iterates over the Avail Extents in the root of the given FMP, shrinking only fully available extents.
// Assumes exclusive access to root space trees and no concurrency with other threads trying to shrink or extend
// the database.
ERR ErrSHKShrinkDbFromEof(
    _In_ PIB *ppib,
    _In_ const IFMP ifmp );

// Moves the FDP, OE and AE roots of a given tree to a new location in the database.
ERR ErrSHKRootPageMove(
    _In_ PIB* ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgnoFDP );

// Performs a root page move based on state accumulated in the given ROOTMOVE struct.
VOID SHKPerformRootMove(
    _In_ ROOTMOVE* const prm,
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const BOOL fRecoveryRedo );

