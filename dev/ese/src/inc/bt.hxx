// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef BT_H_INCLUDED
#error: Already included
#endif
#define BT_H_INCLUDED

//  **********************************************************
//  BTREE API
//


enum OPENTYPE
{
    openNormal,             //  normal open cursor (may be either unique or non-unique btree)
    openNormalUnique,       //  normal open cursor (unique btree only)
    openNormalNonUnique,    //  normal open cursor (non-unique btree only)
    openNew                 //  open cursor on newly-created FDP
};

//  **************************************
//  open/close opearations
//
ERR ErrBTOpen( PIB *ppib, FCB *pfcb, FUCB **ppfucb, BOOL fAllowReuse = fTrue );
ERR ErrBTOpenByProxy( PIB *ppib, FCB *pfcb, FUCB **ppfucb, const LEVEL level );
VOID BTClose( FUCB *pfucb );

ERR ErrBTIOpen(
    PIB             *ppib,
    const IFMP      ifmp,
    const PGNO      pgnoFDP,
    const OBJID     objidFDP,
    const OPENTYPE  opentype,
    FUCB            **ppfucb,
    BOOL            fWillInitFCB );

INLINE ERR ErrBTOpen(
    PIB             *ppib,
    const PGNO      pgnoFDP,
    const IFMP      ifmp,
    FUCB            **ppfucb,
    const OPENTYPE  opentype = openNormal,
    BOOL            fWillInitFCB = fFalse )
{
    Assert( openNormal == opentype || openNew == opentype );
    return ErrBTIOpen(
                ppib,
                ifmp,
                pgnoFDP,
                objidNil,
                opentype,
                ppfucb,
                fWillInitFCB );
}

//  open cursor, don't touch root page
INLINE ERR ErrBTOpenNoTouch(
    PIB             *ppib,
    const IFMP      ifmp,
    const PGNO      pgnoFDP,
    const OBJID     objidFDP,
    const BOOL      fUnique,
    FUCB            **ppfucb,
    BOOL            fWillInitFCB = fFalse )
{
    Assert( objidNil != objidFDP );
    return ErrBTIOpen(
                ppib,
                ifmp,
                pgnoFDP,
                objidFDP,
                fUnique ? openNormalUnique : openNormalNonUnique,
                ppfucb,
                fWillInitFCB );
}


//  **************************************
//  retrieve/release operations
//
ERR ErrBTGet( FUCB *pfucb );
ERR ErrBTRelease( FUCB *pfucb );
ERR ErrBTDeferGotoBookmark( FUCB *pfucb, const BOOKMARK& bm, BOOL fTouch );

//  **************************************
//  movement operations
//
ERR ErrBTNext( FUCB *pfucb, DIRFLAG dirflags );
ERR ErrBTPrev( FUCB *pfucb, DIRFLAG dirflags );
ERR ErrBTDown( FUCB *pfucb, DIB *pdib, LATCH latch );
ERR ErrBTIGotoRoot( FUCB *pfucb, LATCH latch );

//  reset currency
//  no need to save bookmark
//
INLINE VOID BTUp( FUCB *pfucb )
{
    CSR *pcsr = Pcsr( pfucb );

    if( pfucb->pvRCEBuffer )
    {
        OSMemoryHeapFree( pfucb->pvRCEBuffer );
        pfucb->pvRCEBuffer = NULL;
    }

    pcsr->ReleasePage( pfucb->u.pfcb->FNoCache() );
    pcsr->Reset();

#ifdef DEBUG
    //  reset kdfCurr of pfucb
    //
    pfucb->kdfCurr.Nullify();
#endif
}

INLINE VOID BTSetupOnSeekBM( FUCB * const pfucb )
{
    //  node was deleted from under the cursor
    //  reseek to logical bm and move to next node
    //
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !pfucb->bmCurr.key.FNull() );
    Assert( locOnCurBM == pfucb->locLogical );
    BTUp( pfucb );
    pfucb->locLogical = locOnSeekBM;
}
ERR ErrBTPerformOnSeekBM( FUCB * const pfucb, const DIRFLAG dirflag );

//  **************************************
//  direct access routines
//
ERR ErrBTGotoBookmark( FUCB *pfucb, const BOOKMARK& bm, LATCH latch, BOOL fExactPosition = fTrue );
ERR ErrBTGetPosition( FUCB *pfucb, ULONG *pulLT, ULONG *pulTotal );

//  release memory allocated for bookmark in cursor
//
INLINE VOID BTReleaseBM( FUCB *pfucb )
{
    RESBOOKMARK.Free( pfucb->pvBMBuffer );
    pfucb->pvBMBuffer = NULL;

#ifdef DEBUG
    pfucb->bmCurr.Nullify();
#endif
}

//  **************************************
//  probing routines
//

//  Determines whether or not the tree contains a given page.
//  If it does, it'll return JET_errSuccess, otherwise, it returns JET_errRecordNotFound.
//  Any other errors which are < JET_errSuccess should be treated as failures.
//
ERR ErrBTContainsPage( FUCB* const pfucb, const BOOKMARK& bm, const PGNO pgno, const BOOL fLeafPage );


//  **************************************
//  update operations
//
ERR ErrBTLock( FUCB *pfucb, DIRLOCK dirlock );
ERR ErrBTReplace( FUCB * const pfucb, const DATA& data, const DIRFLAG dirflags );

template< typename TDelta >
ERR ErrBTDelta(
        FUCB            *pfucb,
        INT             cbOffset,
        const TDelta    tDelta,
        TDelta          *const pOldValue,
        DIRFLAG         dirflag );

// Explicitly instantiatiate the only allowed legal instances of this template
template ERR ErrBTDelta<LONG>( FUCB *pfucb, INT cbOffset, const LONG delta, LONG *const pOldValue, DIRFLAG dirflag );
template ERR ErrBTDelta<LONGLONG>( FUCB *pfucb, INT cbOffset, const LONGLONG delta, LONGLONG *const pOldValue, DIRFLAG dirflag );

ERR ErrBTInsert( FUCB *pfucb, const KEY& key, const DATA& data, DIRFLAG dirflags, RCE *prcePrimary = prceNil );

ERR ErrBTAppend( FUCB *pfucb, const KEY& key, const DATA& data, DIRFLAG dirflags );

ERR ErrBTFlagDelete( FUCB *pfucb, DIRFLAG dirflags, RCE *prcePrimary = prceNil );
ERR ErrBTDelete( FUCB *pfucb, const BOOKMARK& bm );

ERR ErrBTCopyTree( FUCB * pfucbSrc, FUCB * pfucbDest, DIRFLAG dirflag );

//  **************************************
//  statistical functions
//
ERR ErrBTComputeStats( FUCB *pfucb, INT *pcnode, INT *pckey, INT *pcpage );
ERR ErrBTDumpPageUsage( PIB * ppib, const IFMP ifmp, const PGNO pgnoFDP );

typedef INT (*PFNVISITPAGE)( const PGNO pgno, const ULONG iLevel, const CPAGE * pcpage, void * pvCtx );
ERR ErrBTUTLAcross(
    IFMP                    ifmp,
    const PGNO              pgnoFDP,
    const ULONG             fVisitFlags,
    PFNVISITPAGE            pfnErrVisitPage,
    void *                  pvVisitPageCtx,
    CPAGE::PFNVISITNODE *   rgpfnzErrVisitNode,
    void **                 rgpvzVisitNodeCtx
    );
INLINE ERR ErrBTUTLAcross(
    IFMP                    ifmp,
    const PGNO              pgnoFDP,
    const ULONG             fVisitFlags,
    PFNVISITPAGE            pfnErrVisitPage,
    void *                  pvVisitPageCtx,
    CPAGE::PFNVISITNODE     pfnErrVisitNode,
    void *                  pvVisitNodeCtx
    )
{
    CPAGE::PFNVISITNODE     rgpfnzErrVisitNode[2] = { pfnErrVisitNode, NULL };
    void *                  rgpvzVisitNodeCtx[2] = { pvVisitNodeCtx, NULL };
    return ErrBTUTLAcross(
                ifmp,
                pgnoFDP,
                fVisitFlags,
                pfnErrVisitPage,
                pvVisitPageCtx,
                rgpfnzErrVisitNode,
                rgpvzVisitNodeCtx );
}

//  **************************************
//  special tests
//
ERR ErrBTGetLastPgno( PIB *ppib, IFMP ifmp, PGNO * ppgno );

BOOL FVERDeltaActiveNotByMe( const FUCB * pfucb, const BOOKMARK& bookmark );
BOOL FVERWriteConflict( FUCB * pfucb, const BOOKMARK& bookmark, const OPER oper );
ERR ErrSPGetLastPgno( _Inout_ PIB * ppib, _In_ const IFMP ifmp, _Out_ PGNO * ppgno );

INLINE BOOL FBTActiveVersion( FUCB *pfucb, const TRX trxSession )
{
    BOOKMARK    bm;

    Assert( Pcsr( pfucb )->FLatched() );
    NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
    return FVERActive( pfucb, bm, trxSession );
}
INLINE BOOL FBTDeltaActiveNotByMe( FUCB *pfucb, INT cbOffset )
{
    BOOKMARK    bm;

    Assert( Pcsr( pfucb )->FLatched() );
    NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
    return FVERDeltaActiveNotByMe( pfucb, bm, cbOffset );
}
INLINE BOOL FBTWriteConflict( FUCB *pfucb, const OPER oper )
{
    BOOKMARK    bm;

    Assert( Pcsr( pfucb )->FLatched() );
    NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
    return FVERWriteConflict( pfucb, bm, oper );
}




INLINE ERR ErrBTGetLastPgno( PIB *ppib, IFMP ifmp, PGNO * ppgno )
{
    if( g_fRepair )
    {
        //  this would happen if we were attaching a database with a corrupt global space tree
        //  the FMP is already initialized with the size of the database file
        *ppgno = g_rgfmp[ifmp].PgnoLast();
        return JET_errSuccess;
    }
    else
    {
        return ErrSPGetLastPgno( ppib, ifmp, ppgno );
    }
}

//  saves bookmark from current record in pfucb
//
ERR ErrBTISaveBookmark( FUCB *pfucb, const BOOKMARK& bm, BOOL fTouch );
INLINE ERR ErrBTISaveBookmark( FUCB *pfucb )
{
    BOOKMARK    bm;

    //  UNDONE: cast kdfCurr to BOOKMARK to avoid copies
    //
    Assert( Pcsr( pfucb )->FLatched() );
    Assert( !pfucb->kdfCurr.key.FNull() );
    bm.key = pfucb->kdfCurr.key;
    bm.data = pfucb->kdfCurr.data;

    return ErrBTISaveBookmark( pfucb, bm, fFalse/*NoTouch*/ );
}



//  Forward declaration because repair.hxx is included after bt.hxx
class RECCHECK;

struct PrereadInfo
{
    PrereadInfo(CPG cpg)
    {
        cpgToPreread = cpg;
        C_ASSERT( cbReadSizeMax % g_cbPageMin == 0 );
        Assert( cpg <= cbReadSizeMax / g_cbPageMin );
    }
    
    CPG  cpgToPreread;
    PGNO pgnoPrereadStart;
    CPG  cpgActuallyPreread;
    BYTE rgfPageWasAlreadyCached[ cbReadSizeMax / g_cbPageMin ];
};


ERR ErrBTIMultipageCleanup(
        FUCB * const pfucb,
        const BOOKMARK& bm,
        BOOKMARK * const pbmNext,
        RECCHECK * const preccheck,
        MERGETYPE * const pmergetype,
        const BOOL fRightMerges,
        __inout_opt PrereadInfo * const pPrereadInfo = NULL );

ERR ErrBTPageMove(
    __in FUCB * const pfucb,
    __in const BOOKMARK& bm,
    __in const PGNO pgnoSource,
    __in const BOOL fLeafPage,
    __in const ULONG fSPAllocFlags,
    __inout BOOKMARK * const pbmNext );
VOID BTPerformPageMove( __in MERGEPATH * const pmergePath );

ERR ErrBTFindFragmentedRange(
    __in FUCB * const pfucb,
    __in const BOOKMARK& bmStart,
    __out BOOKMARK * const pbmStart,
    __out BOOKMARK * const pbmEnd);

//  **************************************
//  PREREAD FUNCTIONS
//
#define PREREAD_SPACETREE_ON_SPLIT
#define PREREAD_INDEXES_ON_PREPINSERT
#define PREREAD_INDEXES_ON_REPLACE
#define PREREAD_INDEXES_ON_DELETE

VOID BTPrereadPage( IFMP ifmp, PGNO pgno );
VOID BTPrereadIndexesOfFCB( FUCB * const pfucb );
VOID BTPrereadSpaceTree( const FUCB * const pfucb );

ERR ErrBTPrereadKeys(
    PIB * const                                     ppib,
    FUCB * const                                    pfucb,
    __in_ecount(ckeys) const void * const * const   rgpvKeys,
    __in_ecount(ckeys) const ULONG * const  rgcbKeys,
    const LONG                                      ckeys,
    __out LONG * const                              pckeysPreread,
    const JET_GRBIT                                 grbit );

ERR ErrBTPrereadBookmarks(
    PIB * const                                     ppib,
    FUCB * const                                    pfucb,
    __in_ecount(cbm) const BOOKMARK * const         rgbm,
    const LONG                                      cbm,
    __out LONG * const                              pcbmPreread,
    const JET_GRBIT                                 grbit );

ERR ErrBTPrereadKeyRanges(
    PIB * const                                     ppib,
    FUCB * const                                    pfucb,
    __in_ecount(ckeys) const void * const * const   rgpvKeysStart,
    __in_ecount(ckeys) const ULONG * const  rgcbKeysStart,
    __in_ecount(ckeys) const void * const * const   rgpvKeysEnd,
    __in_ecount(ckeys) const ULONG * const  rgcbKeysEnd,
    const LONG                                      ckeys,
    __deref_out_range( 0, ckeys ) LONG * const      pcRangesPreread,
    __in const ULONG                        cPageCacheMin,
    __in const ULONG                        cPageCacheMax,
    const JET_GRBIT                                 grbit,
    __out_opt ULONG * const                 pcPageCacheActual );

//  **************************************
//  Tree Root Field Operations
//
ERR ErrBTGetRootField(
    _Inout_     FUCB* const                         pfucb,
    _In_range_( 0, noderfMax - 1 )  const NodeRootField     noderf,
    _In_        const LATCH                         latch );

ERR ErrBTSetRootField(
    _In_        FUCB* const                         pfucb,
    _In_range_( 0, noderfMax - 1 )  const NodeRootField     noderf,
    _In_        const DATA&                         dataRootField );

//  **************************************
//  DEBUG ONLY FUNCTIONS
//

//  ensures that status of fucb is valid before
//  call can return to higher level, outside BT
//  use this whenever BT returns a latched page
//  to a higher layer
//
INLINE VOID AssertBTIBookmarkSaved( const FUCB *pfucb )
{
#ifdef DEBUG
    Assert( pfucb->fBookmarkPreviouslySaved );
    Assert( !pfucb->bmCurr.key.FNull() );
    Assert( !pfucb->kdfCurr.key.FNull() );
    Assert( FKeysEqual( pfucb->kdfCurr.key,
                         pfucb->bmCurr.key ) );

    //  iff fucb is on unique tree
    //      data should be zero
    //
    if ( !pfucb->bmCurr.data.FNull() )
    {
        Assert( !FFUCBUnique( pfucb ) );
        Assert( FDataEqual( pfucb->kdfCurr.data,
                             pfucb->bmCurr.data ) );
    }
    else
    {
        Assert( FFUCBUnique( pfucb ) );
    }
#endif
}


INLINE VOID AssertNDCursorOnPage( const FUCB *pfucb, const CSR *pcsr )
{
#ifdef DEBUG
    //  page should be cached
    //
    Assert( pcsr->FLatched( ) );
    Assert( pcsr->Pgno() != pgnoNull );
    Assert( pcsr->Dbtime() != dbtimeNil );
    Assert( pcsr->Dbtime() == pcsr->Cpage().Dbtime() );

    //  node should be cached
    //
    Assert( pcsr->ILine() >= 0 &&
            pcsr->ILine() < pcsr->Cpage().Clines( ) );
    if ( pcsr->Cpage().FLeafPage() && !FFUCBRepair( pfucb ) )
    {
        Assert( !pfucb->kdfCurr.key.FNull() );
    }
    else
    {
        //  key may be NULL only for last node in page
        //
        Assert( !pfucb->kdfCurr.key.FNull() ||
                pcsr->Cpage().Clines() - 1 == pcsr->ILine() );
    }
#endif

    AssertSzRTL( pfucb->kdfCurr.key.Cb() >= 0, "Key portion of record with negative length detected" );
    AssertSzRTL( pfucb->kdfCurr.data.Cb() >= 0, "Data portion of record with negative length detected" );

}


INLINE VOID AssertNDGet( const FUCB *pfucb, const CSR *pcsr )
{
#ifdef DEBUG
    AssertNDCursorOnPage( pfucb, pcsr );

    KEY     keyT    = pfucb->kdfCurr.key;
    DATA    dataT   = pfucb->kdfCurr.data;

    NDGet( const_cast <FUCB *>(pfucb),
           const_cast <CSR *> (pcsr) );
    Assert( keyT.Cb() == pfucb->kdfCurr.key.Cb() );
    Assert( ( keyT.suffix.Pv() == pfucb->kdfCurr.key.suffix.Pv()
                && keyT.prefix.Pv() == pfucb->kdfCurr.key.prefix.Pv() )
            || FKeysEqual( keyT, pfucb->kdfCurr.key ) );

    //  if we're at level 0, the node may been versioned when we first
    //  retrieved it, but the RCE may subsequently have been cleaned up
    //  and the versioned bit reset from underneath us.  Thus, dataT.pv
    //  may now be pointing to something bogus, so don't compare it.
    if ( !FNDVersion( pfucb->kdfCurr ) && pfucb->ppib->Level() > 0 )
    {
        // Comparison of data only valid if node is not versioned
        Assert( dataT.Cb() == pfucb->kdfCurr.data.Cb() );
        Assert( dataT.Pv() == pfucb->kdfCurr.data.Pv()
            || FDataEqual( dataT, pfucb->kdfCurr.data ) );
    }

    const_cast <FUCB *>(pfucb)->kdfCurr.key = keyT;
    const_cast <FUCB *>(pfucb)->kdfCurr.data = dataT;
#endif
}


INLINE VOID AssertNDGet( const FUCB *pfucb )
{
#ifdef DEBUG
    AssertNDGet( pfucb, Pcsr( pfucb ) );
#endif  // DEBUG
}


INLINE VOID AssertBTGet( const FUCB *pfucb )
{
#ifdef DEBUG
    //  check node-pointer is cached in fucb
    //
    if ( FNDVersion( pfucb->kdfCurr ) &&
         !FPIBDirty( pfucb->ppib ) )
    {
        KEYDATAFLAGS    kdfT = pfucb->kdfCurr;

        //  page should be latched
        //  and cursor should be on correct version
        //
        AssertNDCursorOnPage( pfucb, Pcsr( pfucb ) );

        BOOKMARK    bm;

        NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );

        if( pfucb->ppib->Level() > 0 )  //  otherwise ErrVERAccessNode will save the bookmark
        {
            NS          ns;
            CallS( ErrVERAccessNode( (FUCB *) pfucb, bm, &ns ) );

            if ( ns == nsDatabase || ns == nsUncommittedVerInDB || ns == nsCommittedVerInDB )
            {
                //  should point to node in page
                //
                Assert( kdfT.fFlags == pfucb->kdfCurr.fFlags );
                AssertNDGet( pfucb );
            }
            else if ( ns == nsVersionedInsert )
            {
                //  node shouldn't be visible to us -- why are we on it?
                //
                Assert( fFalse );
            }
            else
            {
                //  node obtained from version store
                //
                Assert( kdfT == pfucb->kdfCurr );
            }
        }
        else
        {
            AssertNDGet( pfucb );
        }
    }
    else
    {
        AssertNDGet( pfucb );
    }
#endif
}


INLINE VOID AssertBTType( const FUCB *pfucb )
{
#ifdef DEBUG
    Assert( Pcsr( pfucb )->FLatched() );
    Assert( ObjidFDP( pfucb ) == Pcsr( pfucb )->Cpage().ObjidFDP() );

    if( FFUCBOwnExt( pfucb ) || FFUCBAvailExt( pfucb ) )
    {
        Assert( Pcsr( pfucb )->Cpage().FSpaceTree() );
    }
    else
    if( pfucb->u.pfcb->FTypeTable() )
    {
        //  during recovery, table type is overloaded
        //
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() || Pcsr( pfucb )->Cpage().FPrimaryPage() );
    }
    else if( pfucb->u.pfcb->FTypeSecondaryIndex() )
    {
        Assert( Pcsr( pfucb )->Cpage().FIndexPage() );
    }
    else if( pfucb->u.pfcb->FTypeLV() )
    {
        Assert( Pcsr( pfucb )->Cpage().FLongValuePage() );
    }
#endif  //  DEBUG
}

