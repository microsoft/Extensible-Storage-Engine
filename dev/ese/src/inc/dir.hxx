// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef DIRAPI_H
#error DIR.HXX already included
#endif
#define DIRAPI_H




typedef struct {
    ULONG       ulLT;
    ULONG       ulTotal;
} FRAC;

enum POS    { posFirst, posLast, posDown, posFrac };

const DIRFLAG fDIRNull                      = 0x00000000;

const DIRFLAG fDIRBackToFather              = 0x00000001;

const DIRFLAG fDIRNeighborKey               = 0x00000002;

const DIRFLAG fDIRAllNode                   = 0x00000004;
const DIRFLAG fDIRFavourPrev                = 0x00000008;
const DIRFLAG fDIRFavourNext                = 0x00000010;
const DIRFLAG fDIRExact                     = 0x00000020;


const DIRFLAG fDIRNoVersion                 = 0x00000040;

const DIRFLAG fDIRAppend                    = 0x00000080;
const DIRFLAG fDIRReplace                   = 0x00000100;
const DIRFLAG fDIRInsert                    = 0x00000200;
const DIRFLAG fDIRFlagInsertAndReplaceData  = 0x00000400;

const DIRFLAG fDIRLogColumnDiffs            = 0x00000800;
const DIRFLAG fDIRLogChunkDiffs             = 0x00001000;

const DIRFLAG fDIRNoLog                     = 0x00002000;

const DIRFLAG fDIRNoDirty                   = 0x00004000;

const DIRFLAG fDIRDeltaDeleteDereferencedLV = 0x00008000;

const DIRFLAG fDIREscrowCallbackOnZero      = 0x00010000;

const DIRFLAG fDIREscrowDeleteOnZero        = 0x00020000;

const DIRFLAG fDIRCheckUniqueness           = 0x00040000;

const DIRFLAG fDIRSameLIDOnly               = 0x00080000;

const DIRFLAG fDIRAllNodesNoCommittedDeleted    = 0x00100000;

struct DIB
{
    POS             pos;
    const BOOKMARK *pbm;
    DIRFLAG         dirflag;
};

const CPG cpgDIRReserveConsumed  = (CPG)pgnoMax;

ERR ErrDIRCreateDirectory(
    FUCB    *pfucb,
    CPG     cpgMin,
    PGNO    *ppgnoFDP,
    OBJID   *pobjidFDP,
    UINT    fPageFlags,
    BOOL    fSPFlags = 0 );

ERR ErrDIROpen( PIB *ppib, FCB *pfcb, FUCB **ppfucb );
ERR ErrDIROpenByProxy( PIB *ppib, FCB *pfcb, FUCB **ppfucb, LEVEL level );
ERR ErrDIROpen( PIB *ppib, PGNO pgnoFDP, IFMP ifmp, FUCB **ppfucb, BOOL fWillInitFCB = fFalse );
ERR ErrDIROpenNoTouch( PIB *ppib, IFMP ifmp, PGNO pgnoFDP, OBJID objidFDP, BOOL fUnique, FUCB **ppfucb, BOOL fWillInitFCB = fFalse );
INLINE VOID DIRInitOpenedCursor( FUCB * const pfucb, const LEVEL level )
{
    FUCBSetLevelNavigate( pfucb, level );
    pfucb->locLogical = locOnFDPRoot;
}
VOID DIRClose( FUCB *pfucb );
INLINE VOID DIRCloseIfExists( FUCB ** ppfucb )
{
    Assert( NULL != ppfucb );
    if ( pfucbNil != *ppfucb )
    {
        DIRClose( *ppfucb );
        *ppfucb = pfucbNil;
    }
}

ERR ErrDIRGet( FUCB *pfucb );
ERR ErrDIRGetPosition( FUCB *pfucb, ULONG *pulLT, ULONG *pulTotal );
ERR ErrDIRGetBookmark( FUCB *pfucb, BOOKMARK *pbm );
ERR ErrDIRRelease( FUCB *pfucb );

ERR ErrDIRGotoBookmark( FUCB *pfucb, const BOOKMARK& bm );
ERR ErrDIRGotoJetBookmark( FUCB *pfucb, const BOOKMARK& bm, const BOOL fRetainLatch );
ERR ErrDIRGotoPosition( FUCB *pfucb, ULONG ulLT, ULONG ulTotal );

ERR ErrDIRDown( FUCB *pfucb, DIB *pdib );
ERR ErrDIRDownKeyData( FUCB *pfucb, const KEY& key, const DATA& data );
VOID DIRUp( FUCB *pfucb );
VOID DIRReleaseLatch( _In_ FUCB* const pfucb );
ERR ErrDIRNext( FUCB *pfucb, DIRFLAG dirFlags );
ERR ErrDIRPrev( FUCB *pfucb, DIRFLAG dirFlags );

VOID DIRGotoRoot( FUCB *pfucb );
VOID DIRDeferMoveFirst( FUCB *pfucb );

INLINE VOID DIRBeforeFirst( FUCB *pfucb )   { pfucb->locLogical = locBeforeFirst; }
INLINE VOID DIRAfterLast( FUCB *pfucb )     { pfucb->locLogical = locAfterLast; }


VOID DIRSetIndexRange( FUCB *pfucb, JET_GRBIT grbit );
VOID DIRResetIndexRange( FUCB *pfucb );
ERR ErrDIRCheckIndexRange( FUCB *pfucb );

VOID DIRSetActiveSpaceRequestReserve( FUCB * const pfucb, const CPG cpgRequired );
VOID DIRResetActiveSpaceRequestReserve( FUCB * const pfucb );

ERR ErrDIRInsert( FUCB *pfucb, const KEY& key, const DATA& data, DIRFLAG dirflag, RCE *prcePrimary = prceNil );

ERR ErrDIRInitAppend( FUCB *pfucb );
ERR ErrDIRAppend( FUCB *pfucb, const KEY& key, const DATA& data, DIRFLAG dirflag );
ERR ErrDIRTermAppend( FUCB *pfucb );

ERR ErrDIRDelete( FUCB *pfucb, DIRFLAG dirflag, RCE *prcePrimary = prceNil );

ERR ErrDIRReplace( FUCB *pfucb, const DATA& data, DIRFLAG dirflag );
ERR ErrDIRGetLock( FUCB *pfucb, DIRLOCK dirlock );

template< typename TDelta >
ERR ErrDIRDelta(
        FUCB            *pfucb,
        INT             cbOffset,
        const TDelta    tDelta,
        TDelta          *const pOldValue,
        DIRFLAG         dirflag );

template ERR ErrDIRDelta<LONG>( FUCB *pfucb, INT cbOffset, const LONG delta, LONG *const pOldValue, DIRFLAG dirflag );
template ERR ErrDIRDelta<LONGLONG>( FUCB *pfucb, INT cbOffset, const LONGLONG delta, LONGLONG *const pOldValue, DIRFLAG dirflag );

ERR ErrDIRIndexRecordCount( FUCB *pfucb, ULONG64 *pullCount, ULONG64 ullCountMost, BOOL fNext );
ERR ErrDIRComputeStats( FUCB *pfucb, INT *pcitem, INT *pckey, INT *pcpage );

ERR ErrDIRBeginTransaction( PIB *ppib, const TRXID trxid, const JET_GRBIT grbit );
ERR ErrDIRCommitTransaction( PIB *ppib, JET_GRBIT grbit, DWORD cmsecDurableCommit = 0, JET_COMMIT_ID *pCommitId = NULL );
ERR ErrDIRRollback( PIB *ppib, JET_GRBIT grbit = 0 );

ERR ErrDIRGetRootField( _Inout_ FUCB* const pfucb, _In_ const NodeRootField noderf, _In_ const LATCH latch );
ERR ErrDIRSetRootField( _In_ FUCB* const pfucb, _In_ const NodeRootField noderf, _In_ const DATA& dataRootField );

INLINE VOID AssertDIRNoLatch( PIB *ppib )
{
#ifdef DEBUG
    if ( !ppib->FBatchIndexCreation() )
    {
        for ( FUCB * pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; pfucb = pfucb->pfucbNextOfSession )
        {
            Assert( !Pcsr( pfucb )->FLatched() );
        }
    }
#endif
}

INLINE VOID AssertDIRGet( FUCB *pfucb )
{
#ifdef DEBUG
    Assert( locOnCurBM == pfucb->locLogical );
    Assert( Pcsr( pfucb )->FLatched() );
    Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
    AssertBTGet( pfucb );
#endif
}
    

INLINE VOID DIRGotoRoot( FUCB *pfucb )
{
    Assert( !Pcsr( pfucb )->FLatched() );
    pfucb->locLogical = locOnFDPRoot;
    return;
}

    
INLINE VOID DIRDeferMoveFirst( FUCB *pfucb )
{
    Assert( !Pcsr( pfucb )->FLatched() );
    pfucb->locLogical = locDeferMoveFirst;
}


INLINE ERR ErrDIRGetBookmark( FUCB *pfucb, BOOKMARK **ppbm )
{
    ERR     err;

    if ( Pcsr( pfucb )->FLatched( ) )
    {
        Assert( pfucb->locLogical == locOnCurBM );
        CallR( ErrBTRelease( pfucb ) );
        CallR( ErrBTGet( pfucb ) );
    }
    if ( locOnCurBM == pfucb->locLogical )
    {
        Assert( !pfucb->bmCurr.key.FNull() );
        Assert( ( pfucb->u.pfcb->FUnique() && pfucb->bmCurr.data.FNull() )
            || ( !pfucb->u.pfcb->FUnique() && !pfucb->bmCurr.data.FNull() ) );

        *ppbm = &pfucb->bmCurr;
        return JET_errSuccess;
    }
    else
    {
        return ErrERRCheck( JET_errNoCurrentRecord );
    }
}


INLINE VOID DIRSetIndexRange( FUCB *pfucb, JET_GRBIT grbit )
{
    FUCBSetIndexRange( pfucb, grbit );
}
INLINE VOID DIRResetIndexRange( FUCB *pfucb )
{
    FUCBResetIndexRange( pfucb );
}

INLINE BOOL FDIRActiveVersion( FUCB *pfucb, const TRX trxSession )
{
    Assert( Pcsr( pfucb )->FLatched() );
    Assert( locOnCurBM == pfucb->locLogical );

    return FBTActiveVersion( pfucb, trxSession );
}
INLINE BOOL FDIRDeltaActiveNotByMe( FUCB *pfucb, INT cbOffset )
{
    Assert( Pcsr( pfucb )->FLatched() );
    Assert( locOnCurBM == pfucb->locLogical );

    return FBTDeltaActiveNotByMe( pfucb, cbOffset );
}
INLINE BOOL FDIRWriteConflict( FUCB *pfucb, const OPER oper )
{
    Assert( Pcsr( pfucb )->FLatched() );
    Assert( locOnCurBM == pfucb->locLogical );

    return FBTWriteConflict( pfucb, oper );
}
INLINE CPG CpgDIRActiveSpaceRequestReserve( const FUCB * const pfucb )
{
    return pfucb->cpgSpaceRequestReserve;
}

