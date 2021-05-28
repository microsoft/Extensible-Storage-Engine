// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef DIRAPI_H
#error DIR.HXX already included
#endif
#define DIRAPI_H

/**********************************************************
/************** DIR STRUCTURES and CONSTANTS **************
/**********************************************************
/**/
/************** DIR API defines and types ******************
/***********************************************************
/**/

//  struture for fractional positioning 
//
typedef struct {
    ULONG       ulLT;
    ULONG       ulTotal;
} FRAC;

//  possible positioning parameters
//
enum POS    { posFirst, posLast, posDown, posFrac };

//  no flags
//
const DIRFLAG fDIRNull                      = 0x00000000;

//  go back to root after a DIRInsert
//
const DIRFLAG fDIRBackToFather              = 0x00000001;

//  used by MoveNext/Prev for implementing MoveKeyNE
//  used at BT level
//
const DIRFLAG fDIRNeighborKey               = 0x00000002;

//  used by BTNext/Prev to walk over all nodes
//  may not be needed
//
const DIRFLAG fDIRAllNode                   = 0x00000004;
const DIRFLAG fDIRFavourPrev                = 0x00000008;
const DIRFLAG fDIRFavourNext                = 0x00000010;
const DIRFLAG fDIRExact                     = 0x00000020;


//  set by Space, Ver, OLC, auto-inc and many FDP-level operations
const DIRFLAG fDIRNoVersion                 = 0x00000040;

//  used to hint Split to append while creating index
//  change to Append -- as a hint to append
//  or create a new call BTAppend
//
const DIRFLAG fDIRAppend                    = 0x00000080;
const DIRFLAG fDIRReplace                   = 0x00000100;
const DIRFLAG fDIRInsert                    = 0x00000200;
const DIRFLAG fDIRFlagInsertAndReplaceData  = 0x00000400;

//  used by NDDeltaLogs
//  set at REC & LV
//
const DIRFLAG fDIRLogColumnDiffs            = 0x00000800;
const DIRFLAG fDIRLogChunkDiffs             = 0x00001000;

//  set by SPACE and VER for page operations w/o logging
//  used by node to turn off logging
//
const DIRFLAG fDIRNoLog                     = 0x00002000;

//  used by Undo to not re-dirty the page
//
const DIRFLAG fDIRNoDirty                   = 0x00004000;

//  used by LV to specify that a LV should be deleted if its refcount falls to zero
const DIRFLAG fDIRDeltaDeleteDereferencedLV = 0x00008000;

//  used by Isam to indicate that a finalizable column is being escrowed
const DIRFLAG fDIREscrowCallbackOnZero      = 0x00010000;

//  used by Isam to indicate that a finalizable column is being escrowed
const DIRFLAG fDIREscrowDeleteOnZero        = 0x00020000;

//  valid only with fDIRExact on a non-unique index,
//  return JET_wrnUniqueKey if the specified key value is unique
const DIRFLAG fDIRCheckUniqueness           = 0x00040000;

//  used by the LV layer to have DIRNext stop if it moves from one LID to another
const DIRFLAG fDIRSameLIDOnly               = 0x00080000;

// we don't want nodes which are deleted and don't have
// versions (hence might be scrubbed). Used by ErrRECIInitAutoInc only 
// and always OR'd with fDIRAllNode
const DIRFLAG fDIRAllNodesNoCommittedDeleted    = 0x00100000;

struct DIB
{
    POS             pos;
    const BOOKMARK *pbm;
    DIRFLAG         dirflag;
};

const CPG cpgDIRReserveConsumed  = (CPG)pgnoMax;

//  ************************************************
//  constructor/destructor
//
ERR ErrDIRCreateDirectory(
    FUCB    *pfucb,
    CPG     cpgMin,
    PGNO    *ppgnoFDP,
    OBJID   *pobjidFDP,
    UINT    fPageFlags,
    BOOL    fSPFlags = 0 );

//  ************************************************
//  open/close routines
//
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

//  ********************************************
//  retrieve/release operations
//
ERR ErrDIRGet( FUCB *pfucb );
ERR ErrDIRGetPosition( FUCB *pfucb, ULONG *pulLT, ULONG *pulTotal );
ERR ErrDIRGetBookmark( FUCB *pfucb, BOOKMARK *pbm );
ERR ErrDIRRelease( FUCB *pfucb );

//  ************************************************
//  positioning operations
//
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


//  ********************************************
//  index range operations
//
VOID DIRSetIndexRange( FUCB *pfucb, JET_GRBIT grbit );
VOID DIRResetIndexRange( FUCB *pfucb );
ERR ErrDIRCheckIndexRange( FUCB *pfucb );

//  ********************************************
//  update operations
//
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

// Explicitly instantiatiate the only allowed legal instances of this template
template ERR ErrDIRDelta<LONG>( FUCB *pfucb, INT cbOffset, const LONG delta, LONG *const pOldValue, DIRFLAG dirflag );
template ERR ErrDIRDelta<LONGLONG>( FUCB *pfucb, INT cbOffset, const LONGLONG delta, LONGLONG *const pOldValue, DIRFLAG dirflag );

//  *****************************************
//  statistical operations
//  
ERR ErrDIRIndexRecordCount( FUCB *pfucb, ULONG64 *pullCount, ULONG64 ullCountMost, BOOL fNext );
ERR ErrDIRComputeStats( FUCB *pfucb, INT *pcitem, INT *pckey, INT *pcpage );

//  *****************************************
//  transaction operations
//
ERR ErrDIRBeginTransaction( PIB *ppib, const TRXID trxid, const JET_GRBIT grbit );
ERR ErrDIRCommitTransaction( PIB *ppib, JET_GRBIT grbit, DWORD cmsecDurableCommit = 0, JET_COMMIT_ID *pCommitId = NULL );
ERR ErrDIRRollback( PIB *ppib, JET_GRBIT grbit = 0 );

//  *****************************************
//  external header operations
//
ERR ErrDIRGetRootField( _Inout_ FUCB* const pfucb, _In_ const NodeRootField noderf, _In_ const LATCH latch );
ERR ErrDIRSetRootField( _In_ FUCB* const pfucb, _In_ const NodeRootField noderf, _In_ const DATA& dataRootField );

//  ***********************************************
//  debug only routines
//
INLINE VOID AssertDIRNoLatch( PIB *ppib )
{
#ifdef DEBUG
    if ( ppib->FBatchIndexCreation() )
    {
        // Multiple threads are using this PIB, so it's possible to have random pages latched.
        return;
    }

    for ( FUCB * pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; pfucb = pfucb->pfucbNextOfSession )
    {
        Assert( !Pcsr( pfucb )->FLatched() );
    }
#endif  //  DEBUG
}

INLINE VOID AssertDIRMaybeNoLatch( PIB *ppib, FUCB *pfucb )
{
#ifdef DEBUG
    // This routine is only called from code paths that may validly be working on updating the Cpg cache.


    if ( ppib->FUpdatingExtentPageCountCache() )
    {
        // We're in the process of updating the Cpg cache.

        // In that case, we EXPECT various pages to be latched.  For example, the root page of the object
        // whose update is causing the update to the Cpg Cache must be latched, various other pages belonging
        // to that object may be latched, and there can be root pages latched in ancestor objects of that object
        // if we had to get new extents from parents.  Unfortunately, it's non-trivial to locate all
        // the objects and pages that may be latched in that case, making Assertions prohibitive.
        //
        // Obviously, we have to be very careful to only need latches in the Cache table at this point lest
        // we deadlock on attempting to acquire latches.
        //

        if ( pfucbNil == pfucb )
        {
            // We may be here from an error handler, in which case we might not have a pfucb.  In that case,
            // we can't verify anything about the objid.
            return;
        }


        Assert( pfucb->ppib == ppib );

        // We only expect to be here in limited conditions.
        if ( PfmpFromIfmp( pfucb->ifmp )->ObjidExtentPageCountCacheFDP() == pfucb->u.pfcb->ObjidFDP() )
        {
            // We're in the process of updating the Cpg cache AND we're actually working on the CpgCache table.
            return;
        }

        // defined in cat.hxx, which is generally not included until after this file.
        extern const OBJID  objidFDPMSO;
        if ( objidFDPMSO == pfucb->u.pfcb->ObjidFDP() )
        {
            // We're in the process of updating the Cpg cache AND we're working with the catalog.  This happens
            // normally when we we're opening the CpgCache table and we need to look up info about the table in
            // the catalog.
            return;
        }

        Assert( fFalse );

    }
    else
    {
        AssertDIRNoLatch( ppib );
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
#endif  //  DEBUG
}


//  ***********************************************
//  INLINE ROUTINES
//
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


//  gets bookmark of current node
//  returns NoCurrentRecord if none exists
//  bookmark is copied from cursor bookmark
//  and is valid till a DIR-level move occurs
//
INLINE ERR ErrDIRGetBookmark( FUCB *pfucb, BOOKMARK **ppbm )
{
    ERR     err;

    //  UNDONE: change this so this is done 
    //          only when bookmark is already not saved
    //
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


//  sets/resets index range
//
INLINE VOID DIRSetIndexRange( FUCB *pfucb, JET_GRBIT grbit )
{
    FUCBSetIndexRange( pfucb, grbit );
}
INLINE VOID DIRResetIndexRange( FUCB *pfucb )
{
    FUCBResetIndexRange( pfucb );
}

//  **************************************************
//  SPECIAL DIR level tests
//
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

