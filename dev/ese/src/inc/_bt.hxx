// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

ERR ErrBTIOpenAndGotoRoot( PIB *ppib, const PGNO pgnoFDP, const IFMP ifmp, FUCB **ppfucb );

ERR ErrBTIIRefresh( FUCB *pfucb, LATCH latch );

ERR ErrBTINewSplitPath( SPLITPATH **ppsplitPath );
VOID BTIReleaseSplitPaths( INST *pinst, SPLITPATH *psplitPathLeaf );

VOID BTISplitGetReplacedNode( FUCB *pfucb, SPLIT *psplit );
VOID BTISplitInsertIntoRCELists( FUCB               *pfucb,
                                 SPLITPATH          *psplitPath,
                                 const KEYDATAFLAGS *pkdf,
                                 RCE                *prce1,
                                 RCE                *prce2,
                                 VERPROXY           *pverproxy );

VOID BTIPerformSplit( FUCB *pfucb, SPLITPATH *psplitPathLeaf, KEYDATAFLAGS  *pkdf, DIRFLAG dirflag );

ERR ErrBTINewMergePath( MERGEPATH **ppmergePath );
ERR ErrBTINewMerge( MERGEPATH *pmergePath );
VOID BTIReleaseMergePaths( MERGEPATH *pmergePathLeaf );
VOID BTIPerformMerge( FUCB *pfucb, MERGEPATH *pmergePathLeaf );

INLINE
ERR ErrBTIRefresh( FUCB *pfucb, LATCH latch = latchReadNoTouch )
{
    if ( Pcsr( pfucb )->FLatched() )
    {
        AssertNDCursorOnPage( pfucb, Pcsr( pfucb ) );
        AssertNDGet( pfucb );
        return JET_errSuccess;
    }

    return ErrBTIIRefresh( pfucb, latch );
}


INLINE BOOL FBTIUpdatablePage( const CSR& csr )
{
    Assert( ( pagetrimTrimmed == csr.PagetrimState() )
            || ( PinstFromIfmp( csr.Cpage().Ifmp() )->FRecovering() && latchRIW == csr.Latch() )
            || latchWrite == csr.Latch() );
    return ( latchWrite == csr.Latch() );
}


INLINE VOID AssertBTIVerifyPgnoSplit( FUCB *pfucb, SPLITPATH *psplitPath )
{
#ifdef DEBUG
    SPLIT   *psplit = psplitPath->psplit;
    CSR     *pcsr;

    Assert( psplit != NULL );
    Assert( !FBTIUpdatablePage( psplitPath->csr )
        || !psplitPath->csr.Cpage().FLeafPage() );
    Assert( !FBTIUpdatablePage( psplit->csrNew )
        || !psplit->csrNew.Cpage().FLeafPage() );
    Assert( psplitPath->psplitPathChild != NULL );
    Assert( psplitPath->psplitPathChild->psplit != NULL );
    Assert( splittypeRight == psplit->splittype
        || splittypeVertical == psplit->splittype );
    Assert( psplit->ilineOper < psplit->clines - 1 );

    if ( psplit->ilineOper >= psplit->ilineSplit )
    {
        pcsr = &psplit->csrNew;
        pcsr->SetILine( psplit->ilineOper - psplit->ilineSplit );
    }
    else
    {
        Assert( splittypeVertical != psplit->splittype );
        pcsr = &psplit->psplitPath->csr;
        pcsr->SetILine( psplit->ilineOper );
    }

    if ( FBTIUpdatablePage( *pcsr ) )
    {
        NDGet( pfucb, pcsr );
        Assert( sizeof(PGNO) == pfucb->kdfCurr.data.Cb() );
        Assert( psplitPath->psplitPathChild->psplit->pgnoSplit ==
                    *( (UnalignedLittleEndian< PGNO > *) (pfucb->kdfCurr.data.Pv()) ) );
    }
#endif
}

