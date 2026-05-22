// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

BBTBuff::~BBTBuff()
{
    // Caller is responsible for managing latches.
}

ERR BBTBuff::ErrEnsurePageLatched( SkipListLink link, LATCH latchType )
{
    PageOffsetTuple pgOffset = IpgOffsetFromLink( link );
    return ErrEnsurePageLatched( pgOffset.ipg, latchType );
}

ERR BBTBuff::ErrEnsurePageLatched( int ipgOffset, LATCH latchType )
{
    ERR err = JET_errSuccess;
    Assert( ipgOffset >= 0 && ipgOffset < Cpg() );

    CSR* pcsr = Pcsr( ipgOffset );
    if ( pcsr->Latch() != latchType )
    {
        switch ( pcsr->Latch() )
        {
        case latchNone:
        {
            if ( latchType != latchWrite )
            {
                CallR( pcsr->ErrGetPage( m_ppib, m_ifmp, m_pcsrBase->Pgno() + ipgOffset, latchType ) );
            }
            else
            {
                // ErrGetPage() doesn't support getting a write latch directly
                CallR( pcsr->ErrGetPage( m_ppib, m_ifmp, m_pcsrBase->Pgno() + ipgOffset, latchRIW ) );
                pcsr->UpgradeFromRIWLatch();
            }
            break;
        }

        case latchReadTouch:
            EnforceSz( false, "Upgrade from read isn't allowed." );
            err = ErrERRCheck( JET_errInternalError );
            break;

        case latchRIW:
            //Assert( m_latchType == latchRIW );
            EnforceSz( latchType == latchWrite, "Unsupported latch state transition from RIW latch" );
            pcsr->UpgradeFromRIWLatch();
            break;

        case latchWrite:
            // if the caller asks for RIW, and we have write, it is fine
            Assert( m_latchType == latchRIW );
            EnforceSz( latchType == latchRIW, "Unsupported latch state transition from write latch" );
            break;

        default:
            EnforceSz( false, "Unsupported current latch state" );
            err = ErrERRCheck( JET_errInternalError );
        }
    }

    Assert( pcsr->Cpage().FBBTBuffPage() );
    Assert( ( ipgOffset > 0 ) == ( !pcsr->Cpage().FBBTBuffRootPage() ) );    // only the first page is marked with the BBTBuff root flag
    Assert( m_pcsrBase->Dbtime() == pcsr->Dbtime() );
    return err;
}

void BBTBuff::DowngradeLatches()
{
    if ( m_latchType == latchRIW || m_latchType == latchReadTouch || m_latchType == latchReadNoTouch )
    {
        for ( int i = 0; i < Cpg(); i++ )
        {
            CSR* pcsrCurr = Pcsr( i );
            if ( latchWrite == pcsrCurr->Latch() )
            {
                pcsrCurr->Downgrade( m_latchType );
            }
        }
    }
    else
    {
        Assert( m_latchType == latchWrite );
    }
}

// Seeks to a particular key.
// Positions at node(s) based on SeekMode:
// - SeekMode::LEQ positions at LessThan Or Equal.
// - SeekMode::LT positions at LessThan.
// For duplicate nodes, level0 position is at the last (which is the latest) node in the sequence.
// If all nodes are greater, then positions at the first node, (which would be the oldest node in the sequence).
// Returns links and nodes at each level (which may belong to different nodes).
// SeekPos::Prev returns links/nodes to the node before the seeked node (at each level), needed for Delete.
// Note: SeekPos doesn't modify what key is seeked to, only changes whether returned links point to the seek key,
//       or its predecessors.
// Returns the comparison value of the seeked key to the nearest node in the order below (based on SeekMode, SeekPos has no effect):
// 1. If there is a key equal to seeked key, result is 0.
// 2. Else if LT keys exist, returns < 0.
// 3. Else if only GT keys exist, returns > 0.
// SeekMode::LT switches the evaluation order of 1 & 2. If LT exists returns < 0, Else if EQ exists, returns = 0.
// Note that rgNodes[], rgLinks[] only return what was asked. For example, they will be null if caller asked SeekMode::LT, and we only found GEQ nodes.
ERR BBTBuff::ErrSeek_(
        const KEY& key,
        SeekMode seekMode,
        SeekPos seekPos,
        _Out_ int* piResult,
        _Out_ SkipListLink rgLinks[ MAX_LEVELS ],
        _Out_ SkipListNode* rgNodes[ MAX_LEVELS ] )
{
    ERR             err = JET_errSuccess;
    int             cmp = -1;   // default value, if the list is empty
    int             cmpLEQ = 1;
    SkipListNode*   pnodePrev = NULL;
    SkipListNode*   pnodeCurr = NULL;
    SkipListNode*   pnodeNext = NULL;
    SkipListLink    linkNodePrev( 0 );
    SkipListLink    linkNodeCurr( 0 );
    SkipListLink    linkNodeNext( 0 );
    SkipListLinkArray rgLinksCurr = RgSkipListLinksHead( m_pHeader );

    const int       comparand = ( seekMode == SeekMode::LT ? 0  :
                                  seekMode == SeekMode::LEQ ? 1 :
                                  -1 );

    // Search the list for the given key
    for ( int i = MAX_LEVELS - 1; i >= 0; i-- )
    {
        Call( ErrPnodeFromLink_AcqLatch( rgLinksCurr[ i ], &pnodeNext ) );
        linkNodeNext = rgLinksCurr[ i ];
        while ( pnodeNext != NULL )
        {
            cmp = pnodeNext->CmpKey( key );
            if ( cmp < comparand )
            {
                // Next key is still behind the seeked position
                // MoveNext
                pnodePrev = pnodeCurr;
                pnodeCurr = pnodeNext;
                linkNodePrev = linkNodeCurr;
                linkNodeCurr = linkNodeNext;
                rgLinksCurr = pnodeCurr->RgLinksNext();
                Call( ErrPnodeFromLink_AcqLatch( rgLinksCurr[ i ], &pnodeNext ) );
                linkNodeNext = rgLinksCurr[ i ];
                cmpLEQ = cmp;
            }
            else
            {
                // Next key is after the seeked position
                // So move to lower level.
                break;
            }
        }

        // Next key is after the seeked position (or NULL).
        // Seek point found at the current level.
        // Now store pointers at the current level for caller.
        // Null link/pnode means there is no prev node (m_pHeader->rgSkipListLinksHead[i] should still point here)

        // Drill down levels; for all lower levels where
        // the next node is also the next node at the lower level.
        for ( i; i >= 1; i-- )
        {
            if ( seekPos == SeekPos::Prev )
            {
                // Caller gets the prev node at current level.
                rgLinks[ i ] = linkNodePrev;
                rgNodes[ i ] = pnodePrev;

                // If we found a match at the current level, we've moved to that node (to seek to the latest in the duplicate sequence).
                // But to return SeekPos::Prev, we need to find nodes sandwiched between pnodePrev and pnodeCurr at the lower level.
                // We would've skipped those nodes when we were moving next at a higher level.
                // To establist the correct prev node at the lower level we are drilling down to,
                // we have to walk the links at the lower level from the prev node until we land on the curr node.
                if ( !linkNodeCurr.FNull() )
                {
                    SkipListLinkArray rgLinksPrev = ( pnodePrev != NULL ? pnodePrev->RgLinksNext() : RgSkipListLinksHead( m_pHeader ) );
                    while ( rgLinksPrev[ i - 1 ] != linkNodeCurr )
                    {
                        // MoveNext (there is a node between prev and curr).
                        linkNodePrev = rgLinksPrev[ i - 1 ];
                        Assert( !linkNodePrev.FNull() );    // we can't walk off the end without hitting pnodeCurr, or the skiplist is ill-formed
                        Call( ErrPnodeFromLink_AcqLatch( linkNodePrev, &pnodePrev ) );
                        rgLinksPrev = pnodePrev->RgLinksNext();
                    }
                }
            }
            else
            {
                // Caller gets the current node.
                rgLinks[ i ] = linkNodeCurr;
                rgNodes[ i ] = pnodeCurr;
            }

            if ( linkNodeNext != rgLinksCurr[ i - 1 ] )
            {
                // Note that i will be decremented again once by the outer for loop.
                break;
            }
        }
    }

    // A quirk of the seek loop above, level0 remains to be set.
    if ( seekPos == SeekPos::Prev )
    {
        rgLinks[ 0 ] = linkNodePrev;
        rgNodes[ 0 ] = pnodePrev;
    }
    else
    {
        rgLinks[ 0 ] = linkNodeCurr;
        rgNodes[ 0 ] = pnodeCurr;
    }

    // if pnodeCurr was found, then it was LEQ to the seek key. Then we should return the LEQ cmp value (as the seek loop would break when a comparison is greater).
    // else, return cmp. If no node is found, we use cmp (the value of last comparison).
    *piResult = ( cmpLEQ <= 0 ? cmpLEQ : cmp );

HandleError:
    return err;
}

void BBTBuff::AssertLatchedAll( LATCH latchType )
{
#ifdef DEBUG
    for ( int i = 0; i < Cpg(); i++ )
    {
        CSR* pcsr = Pcsr( i );
        Assert( pcsr->Latch() == latchType );
    }
#endif
}

void BBTBuff::AssertReadyForWrite()
{
#ifdef DEBUG
    for ( int i = 0; i < Cpg(); i++ )
    {
        CSR* pcsr = Pcsr( i );
        Assert( pcsr->Latch() == latchWrite );
        Assert( pcsr->FDirty() );
    }
#endif
}

// Inserts a new node into the list.
// Duplicate nodes are inserted at the tail of a duplicate sequence.
SkipListNode* BBTBuff::PnodeInsert_(
    const BBTBuffChangeContext& changeCtx,
    SkipListNode* rgNodes[ MAX_LEVELS ],
    BBTBuffOpcode opcode,
    const KEY& key,
    const DATA& data,
    SkipListNodeFlags flags,
    bool fDuplicate )
{
    AssertReadyForWrite();

    int cbNodeInsert = SkipListNode::Cb( changeCtx.level, key.Cb(), data.Cb() );
    SkipListLinkArray rgLinksHead = RgSkipListLinksHead( m_pHeader );
    SkipListLink linkNext0 = ( rgNodes[ 0 ] ? rgNodes[ 0 ]->LinkNext0() : rgLinksHead[ 0 ] );
    SkipListLink linkNew = changeCtx.linkCurr;
    int level = changeCtx.level;

    Assert( cbNodeInsert <= CbMaxNodeSize() );

    // Start modifying the header
    int cbUsed = SkipListLink::Roundup( cbNodeInsert );
    m_pHeader->le_cbFree -= ( cbUsed + linkNew.ToInt() - m_pHeader->le_ibMicFree->ToInt() );    // count wasted space because of a page switch, too
    EnforceSz( m_pHeader->le_cbFree >= 0, "BBTBuff: InsertOverflow" );
    SkipListLink ibMicFree = linkNew;
    ibMicFree.Inc( cbUsed );
    m_pHeader->le_ibMicFree = ibMicFree;
    m_pHeader->le_cNodes++; // The node is now part of the list, officially

    PageOffsetTuple pgOffsetNew = IpgOffsetFromLink( linkNew );

    // In-place initialize the node on the page.
    SkipListNode* pnodeNew = SkipListNode::Create( PbPage( pgOffsetNew.ipg ) + pgOffsetNew.ibOnPage, level, opcode, key.Cb(), data.Cb() );
    pnodeNew->SetNodeKey( key );
    pnodeNew->SetNodeData( data );
    pnodeNew->SetNodeFlags( flags );

    // Modify the skip list
    Assert( pnodeNew->IsValid() );
    pnodeNew->SetLinkPrev0( changeCtx.rgLinksPrev[ 0 ] );
    SkipListLinkArray rgLinksNew = pnodeNew->RgLinksNext();

    // Adjust links for prev node(s) at each level
    for ( int i = level; i >= 0; i-- )
    {
        SkipListLinkArray rgLinksPrev = rgNodes[ i ] ? rgNodes[ i ]->RgLinksNext() : rgLinksHead;
        rgLinksNew.SetLink( i, rgLinksPrev[ i ] );
        rgLinksPrev.SetLink( i, linkNew );
    }

    // Adjust duplicate flag of the prev node
    if ( rgNodes[ 0 ] != NULL )
    {
        rgNodes[ 0 ]->SetDuplicateNext0( fDuplicate );
    }
    else
    {
        Assert( fDuplicate == false );
    }

    // Adjust m_linkPrev0 of the next node
    if ( !linkNext0.FNull() )
    {
        SkipListNode* pnodeNext = PnodeFromLink( linkNext0 );
        EnforceSz( changeCtx.rgLinksPrev[ 0 ] == pnodeNext->LinkPrev0(), "BBTBuffCorrupt" );
        pnodeNext->SetLinkPrev0( linkNew );

        // We should never be inserting in the middle of a duplicate sequence.
        Assert( pnodeNew->CmpKey( pnodeNext->Key() ) < 0 );
    }

#ifdef DEBUG
    // Check all the links in the inserted node are valid
    SkipListNode* pnode = PnodeFromLink( pnodeNew->LinkPrev0() );
    int cmpWithPrev = ( pnode != NULL ? pnodeNew->CmpKey( pnode->Key() ) : 1 );
    Assert( cmpWithPrev >= 0 );
    Assert( fDuplicate == ( cmpWithPrev == 0 ) );
    Assert( pnode == NULL || pnode->FDuplicateNext0() == fDuplicate );

    for ( int i = 0; i <= pnodeNew->Level(); i++ )
    {
        pnode = PnodeFromLink( rgLinksNew[ i ] );
        if ( pnode != NULL )
        {
            Assert( pnode->IsValid() );
            Assert( pnodeNew->CmpKey( pnode->Key() ) <= 0 );
        }
    }

    Assert( !rgLinksHead[ 0 ].FNull() );
#endif

    return pnodeNew;
}

// Note: this is a physical delete. Currently, only used to undo a versioned operation during rollback by verstore.
void BBTBuff::Delete_(
    BBTBuffChangeContext changeCtx,
    SkipListNode* rgNodes[ MAX_LEVELS ],
    const SkipListNode* pnodeToDelete )
{
    AssertReadyForWrite();
    Assert( pnodeToDelete != NULL );

    int level = changeCtx.level;
    SkipListLink linkDel = changeCtx.linkCurr;
    SkipListLink linkNext0 = pnodeToDelete->LinkNext0();
    SkipListNode* pnodeNext0 = PnodeFromLink( linkNext0 );

    // Start modifying the pages

    if ( pnodeNext0 )
    {
        pnodeNext0->SetLinkPrev0( pnodeToDelete->LinkPrev0() );
    }

    // Adjust links for prev node(s) at each level
    Assert( level == pnodeToDelete->Level() );
    SkipListLinkArray rgLinksNextToDel = pnodeToDelete->RgLinksNext();
    for ( int i = pnodeToDelete->Level(); i >= 0; i-- )
    {
        SkipListLinkArray rgLinksPrevToDel = ( rgNodes[ i ] != NULL ? rgNodes[ i ]->RgLinksNext() : RgSkipListLinksHead( m_pHeader ) );
        EnforceSz( rgLinksPrevToDel[ i ] == linkDel, "BBTBuffDelete_CorruptedLinks");    // the link being replaced must point to the node being deleted
        rgLinksPrevToDel.SetLink( i, rgLinksNextToDel[ i ] );
    }

    // Fix duplicate flag on the prev node, if the node being deleted is a duplicate of it.
    if ( rgNodes[ 0 ] != NULL && rgNodes[ 0 ]->FDuplicateNext0() )
    {
        // Transfer duplicate flag to the prev node:
        // - if the node being deleted has a next duplicate, then the prev node also has a next duplicate after deletion.
        // - if not, then the prev node doesn't either.
        rgNodes[ 0 ]->SetDuplicateNext0( pnodeToDelete->FDuplicateNext0() );
    }

    Assert( m_pHeader->le_cNodes > 0 );
    m_pHeader->le_cNodes--;
    m_pHeader->le_cbFree += pnodeToDelete->Cb();
    Assert( m_pHeader->le_cbFree <= CbMax() );
}

// Deletes a range of nodes, starting from the given link.
// The range delete operation must start on the oldest node of a duplicate sequence,
// and end at the latest node of a duplicate sequence.
// It is an n*log(n) operation, average case.
void BBTBuff::RangeDelete_( SkipListLink linkFirst, int cNodes )
{
    ERR err = JET_errSuccess;

    AssertReadyForWrite();
    ResetCurr();

    // Check/Prepare for deletion
    // All modifications are deferred until we know that delete can succeed unconditionally
    SkipListNode* pnodeFirst = PnodeFromLink( linkFirst );
    SkipListNode* pnodeCurr = pnodeFirst;
    SkipListLink linkCurr = linkFirst;
    KEY keyFirst = pnodeFirst->Key();
    int cbDeleted = 0;

    // Assert that deleted range doesn't start or end in the middle of a duplicate sequence
    // Range delete does't fix duplicate flags. It doesn't need to because range delete should
    // always span duplicate nodes.
    SkipListNode* pnodeBeforeFirst = PnodeFromLink( pnodeFirst->LinkPrev0() );
    EnforceSz( pnodeBeforeFirst == NULL || pnodeBeforeFirst->FDuplicateNext0() == false, "RangeDelete()_: PartiallyDeletingDuplicateSequence");
    Assert( cNodes > 0 );

    // We have to seek to find previous node at each level to remove the node from the skiplist.
    int result;
    SkipListLink rgLinksPrev[ MAX_LEVELS ] = {};    // braces zero-initialize the array
    SkipListNode* rgpNodes[ MAX_LEVELS ] = {};
    err = ErrSeek_( keyFirst, BBTBuff::SeekMode::LT, BBTBuff::SeekPos::Curr, &result, rgLinksPrev, rgpNodes );
    EnforceSz( result <= 0 && err == JET_errSuccess, "RangeDelete_(): SeekFailed" );

    // Store addresses of the link field of the prev node at each level.
    // This field needs to be modified to point to the after-last node at each level.
    //SkipListLink* rgpLinksPrev[ MAX_LEVELS ];
    //for ( int i = MAX_LEVELS; i >= 0; i-- )
    //{
    //    rgpLinksPrev[ i ] = ( rgNodes[ i ] != NULL ? rgNodes[ i ]->PrgLinks() + i : &m_pHeader->rgSkipListLinksHead[ i ] );
    //}

    // Walk all nodes in the range-delete, and move links of the prev node forward step-by-step to after-last,
    // at each level.
    SkipListLinkArray   rgLinksHead = RgSkipListLinksHead( m_pHeader );
    SkipListLink        rgLinksAfterLast[ MAX_LEVELS ];
    int                 iNode = 0;

    // Start from the current node, at each level.
    for ( int i = 0; i < MAX_LEVELS; i++ )
    {
        rgLinksAfterLast[ i ] = ( rgpNodes[ i ] != NULL ? rgpNodes[ i ]->RgLinksNext()[ i ] : rgLinksHead[ i ] );
    }

    while ( pnodeCurr != NULL )
    {
        SkipListLinkArray rgLinksCurr = pnodeCurr->RgLinksNext();
        for ( int level = pnodeCurr->Level(); level >= 0; level-- )
        {
            // Move link to the next node at the current level.
            Assert( rgLinksAfterLast[ level ] == linkCurr );
            rgLinksAfterLast[ level ] = rgLinksCurr[ level ];
        }

        cbDeleted += pnodeCurr->Cb();
        linkCurr = rgLinksCurr[ 0 ];
        pnodeCurr = PnodeFromLink( linkCurr );
        iNode++;
        if ( iNode >= cNodes )
        {
            break;
        }
    }

    // Fix the prev link of the after-last node, if it exists.
    if ( pnodeCurr != NULL )
    {
        Assert( pnodeFirst->LinkPrev0() == rgLinksPrev[ 0 ] );
        pnodeCurr->SetLinkPrev0( rgLinksPrev[ 0 ] );
    }

    // Fix the next links at all levels to point to the after last nodes.
    for ( int level = 0; level < MAX_LEVELS; level++ )
    {
        ( rgpNodes[ level ] ? rgpNodes[ level ]->RgLinksNext() : rgLinksHead ).SetLink( level, rgLinksAfterLast[ level ] );
    }

    m_pHeader->le_cNodes -= cNodes;
    m_pHeader->le_cbFree += cbDeleted;

    Assert( m_pHeader->le_cNodes >= 0 );
    Assert( m_pHeader->le_cbFree <= CbMax() );
}

// Seeks to the given key. Favors previous key if no match found.
// The cursor is placed at LessThanOrEqual, unless all nodes are greater than the key.
ERR BBTBuff::ErrSeekLEQ( const KEY& key )
{
    ERR err = JET_errSuccess;
    Assert( m_pcsrBase->FLatched() );
    ResetCurr();

    int result;
    SkipListLink rgLinks[ MAX_LEVELS ];
    SkipListNode* rgNodes[ MAX_LEVELS ];
    Call( ErrSeek_( key, SeekMode::LEQ, SeekPos::Curr, &result, rgLinks, rgNodes ) );

    if ( result == 0 )
    {
        Assert( rgNodes[ 0 ]->CmpKey( key ) == 0 );
        Call( ErrAssertIsLatestInDuplicateSequence( rgNodes[ 0 ] ) );

        PageOffsetTuple pgOffset = IpgOffsetFromLink( rgLinks[ 0 ] );
        ChangeCurr( pgOffset.ipg, rgNodes[ 0 ] );
        err = JET_errSuccess;
    }
    else if ( rgNodes[ 0 ] != NULL )
    {
        // If exact node not found, the Seek loop leaves currency immediately before the seek key
        Assert( result < 0 );
        Assert( rgNodes[ 0 ]->CmpKey( key ) < 0 );

        // We've landed on a node immediately before the seek key. The skip list guarantees
        // that this is the tail of a duplicate sequence.
        PageOffsetTuple pgOffset = IpgOffsetFromLink( rgLinks[ 0 ] );
        ChangeCurr( pgOffset.ipg, rgNodes[ 0 ] );
        err = ErrERRCheck( wrnNDFoundLess );
    }
    else if( !RgSkipListLinksHead( m_pHeader )[ 0 ].FNull() )
    {
        // If all nodes are greater than the seek key
        // and there is atleast one node in the list
        Assert( result > 0 );

        PageOffsetTuple pgOffset = IpgOffsetFromLink( RgSkipListLinksHead( m_pHeader )[ 0 ] );
        SkipListNode* pnodeCurr = PnodeFromIpgOffset( pgOffset );
        Assert( pnodeCurr->CmpKey( key ) > 0 );

        if ( !pnodeCurr->FDuplicateNext0() )
        {
            ChangeCurr( pgOffset.ipg, pnodeCurr );
        }
        else
        {
            // We have to walk the list at level0 to get to the tail of the current duplicate sequence.
            // It is wasted effort if the current node's data/flags are never accessed.
            // UA_TODO: For simplicity, lets see if this contract makes sense.
            SkipListLink linkLatest;
            Call( ErrGetLatestInDuplicateSequence( pnodeCurr, &linkLatest ) );

            if ( !linkLatest.FNull() )
            {
                pgOffset = IpgOffsetFromLink( linkLatest );
                SkipListNode* pnodeLatest = PnodeFromIpgOffset( pgOffset );
                ChangeCurr( pgOffset.ipg, pnodeLatest );
            }
            else
            {
                pgOffset = IpgOffsetFromLink( rgLinks[ 0 ] );
                ChangeCurr( pgOffset.ipg, rgNodes[ 0 ] );
            }
        }

        err = ErrERRCheck( wrnNDFoundGreater );
    }
    else
    {
        // If we aren't leaving currency at some node, BBTBuff must be empty
        Assert( m_pHeader->le_cNodes == 0 );
        err = ErrERRCheck( errBBTNodeNotFound );
    }

HandleError:
    return err;
}

// Seeks to a key that is equal to or greater than the given key.
// Returns errBBTNodeNotFound if all keys are less or the list is empty.
// Places cursor at the oldest node in the duplicate sequence.
// WARNING: This is needed to move BBTBuff nodes in an internal split.
//          Don't use for establishing currency.
ERR BBTBuff::ErrSeekGEQOldest( const KEY& key )
{
    ERR err = JET_errSuccess;
    Assert( m_pcsrBase->FLatched() );
    ResetCurr();

    int result;
    SkipListLink rgLinks[ MAX_LEVELS ];
    SkipListNode* rgNodes[ MAX_LEVELS ];
    Call( ErrSeek_( key, SeekMode::LT, SeekPos::Curr, &result, rgLinks, rgNodes ) );

    if ( result < 0 && rgNodes[ 0 ] != NULL )
    {
        // LT node found, next node (if exists) will be GEQ.
        Assert( rgNodes[ 0 ]->CmpKey( key ) < 0 );
        Call( ErrAssertIsLatestInDuplicateSequence( rgNodes[ 0 ] ) );

        if ( !rgNodes[ 0 ]->LinkNext0().FNull() )
        {
            PageOffsetTuple pgOffset = IpgOffsetFromLink( rgNodes[ 0 ]->LinkNext0() );
            SkipListNode* pnodeGEQ = PnodeFromIpgOffset( pgOffset );
            int cmp = pnodeGEQ->CmpKey( key );
            Call( ErrAssertIsOldestInDuplicateSequence( pnodeGEQ ) );

            Assert( cmp >= 0 );
            ChangeCurr( pgOffset.ipg, pnodeGEQ );
            err = ( cmp > 0 ? ErrERRCheck( wrnNDFoundGreater ) : JET_errSuccess );
        }
        else
        {
            err = ErrERRCheck( errBBTNodeNotFound );
        }
    }
    else if ( !RgSkipListLinksHead( m_pHeader )[ 0 ].FNull() )
    {
        // If all nodes are GEQ than the seek key
        // and there is atleast one node in the list
        Assert( result >= 0 );

        PageOffsetTuple pgOffset = IpgOffsetFromLink( RgSkipListLinksHead( m_pHeader )[ 0 ] );
        SkipListNode* pnodeGEQ = PnodeFromIpgOffset( pgOffset );
        Assert( result == pnodeGEQ->CmpKey( key ) );
        ChangeCurr( pgOffset.ipg, pnodeGEQ );
        err = ( result > 0 ? ErrERRCheck( wrnNDFoundGreater ) : JET_errSuccess );
    }
    else
    {
        // If we aren't leaving currency at some node, BBTBuff must be empty
        Assert( m_pHeader->le_cNodes == 0 );
        err = ErrERRCheck( errBBTNodeNotFound );
    }

HandleError:
    return err;
}

ERR BBTBuff::ErrMoveFirst( SeekFlags fFlags )
{
    ERR err = JET_errSuccess;
    ResetCurr();

    if ( !RgSkipListLinksHead( m_pHeader )[ 0 ].FNull() )
    {
        PageOffsetTuple pgOffset = IpgOffsetFromLink( RgSkipListLinksHead( m_pHeader )[ 0 ] );
        CallR( ErrEnsurePageLatched( pgOffset.ipg, m_latchType ) );

        SkipListNode* pnodeCurr = PnodeFromIpgOffset( pgOffset );
        if ( fFlags == sfSkipDuplicates )
        {
            SkipListLink link( 0 );
            CallR( ErrGetLatestInDuplicateSequence( pnodeCurr, &link ) );
            if ( !link.FNull() )
            {
                pgOffset = IpgOffsetFromLink( link );
                pnodeCurr = PnodeFromIpgOffset( pgOffset );
            }
        }

        ChangeCurr( pgOffset.ipg, pnodeCurr );
        return err;
    }
    else
    {
        return ErrERRCheck( errBBTNodeNotFound );
    }
}

// ErrMoveLast is a log(n) operation (average)
ERR BBTBuff::ErrMoveLast()
{
    ERR err = JET_errSuccess;
    Assert( m_pcsrBase->FLatched() );
    ResetCurr();

    if ( RgSkipListLinksHead( m_pHeader )[ 0 ].FNull() )
    {
        return ErrERRCheck( errBBTNodeNotFound );
    }

    SkipListNode*       pnodeCurr = NULL;
    SkipListLink        linkCurr( 0 );
    SkipListLinkArray   rgLinks = RgSkipListLinksHead( m_pHeader );

    for ( int i = MAX_LEVELS - 1; i >= 0; i-- )
    {
        SkipListNode* pnodeNext;
        Call( ErrPnodeFromLink_AcqLatch( rgLinks[ i ], &pnodeNext ) );
        while ( pnodeNext != NULL )
        {
            // MoveNext at the same level (until we reach the end node at the current level)
            pnodeCurr = pnodeNext;
            linkCurr = rgLinks[ i ];
            rgLinks = pnodeCurr->RgLinksNext();
            Call( ErrPnodeFromLink_AcqLatch( rgLinks[ i ], &pnodeNext ) );
        }
    }

    Assert( pnodeCurr != NULL );
    Assert( rgLinks[ 0 ].FNull() );    // should be the last node
    Assert( pnodeCurr->IsValid() );

    {
        PageOffsetTuple pgOffset = IpgOffsetFromLink( linkCurr );
        ChangeCurr( pgOffset.ipg, pnodeCurr );
    }

HandleError:
    return err;
}

ERR BBTBuff::ErrMoveNext( SeekFlags fFlags )
{
    ERR err = JET_errSuccess;
    SkipListNode* pnodeCurr = m_pnodeCurr;

    if ( pnodeCurr != NULL )
    {
        Assert( pnodeCurr->IsValid() );

        if ( fFlags == sfSkipDuplicates && pnodeCurr->FDuplicateNext0() )
        {
            SkipListLink linkLatest( 0 );
            Call( ErrGetLatestInDuplicateSequence( pnodeCurr, &linkLatest ) );

            Assert( !linkLatest.FNull() );
            Call( ErrPnodeFromLink_AcqLatch( linkLatest, &pnodeCurr ) );
        }

        // The node with FDuplicateNext0 == false is the tail of the duplicate sequence
        // Have to move 1 past it to get the next unique node.

        if ( !pnodeCurr->LinkNext0().FNull() )
        {
            PageOffsetTuple pgOffset = IpgOffsetFromLink( pnodeCurr->LinkNext0() );
            Call( ErrEnsurePageLatched( pgOffset.ipg, m_latchType ) );
            pnodeCurr = PnodeFromIpgOffset( pgOffset );
            ChangeCurr( pgOffset.ipg, pnodeCurr );

            if ( fFlags == sfSkipDuplicates )
            {
                Call( ErrMoveToLatestInDuplicateSequence() );
            }

            goto HandleError;
        }
    }

    // Currency is only reset if we move beyond the end of the skip list.
    // For other errors, e.g. IO errors while latching pages, currency stays
    // where it is.
    ResetCurr();
    err = ErrERRCheck( errBBTNodeNotFound );

HandleError:
    return err;
}

ERR BBTBuff::ErrMovePrev( SeekFlags fFlags )
{
    ERR err = JET_errSuccess;
    SkipListNode* pnodeCurr = m_pnodeCurr;

    if ( pnodeCurr != NULL )
    {
        Assert( pnodeCurr->IsValid() );

        if ( !pnodeCurr->LinkPrev0().FNull() )
        {
            SkipListNode* pnodePrev;
            Call( ErrPnodeFromLink_AcqLatch( pnodeCurr->LinkPrev0(), &pnodePrev ) );

            if ( fFlags == sfSkipDuplicates )
            {
                while ( pnodePrev != NULL )
                {
                    if ( !pnodePrev->FDuplicateNext0() )
                    {
                        ErrAssertIsLatestInDuplicateSequence( pnodePrev );
                        break;
                    }

                    pnodeCurr = pnodePrev;
                    Call( ErrPnodeFromLink_AcqLatch( pnodeCurr->LinkPrev0(), &pnodePrev ) );
                }
            }

            if ( !pnodeCurr->LinkPrev0().FNull() )
            {
                PageOffsetTuple pgOffset = IpgOffsetFromLink( pnodeCurr->LinkPrev0() );
                pnodeCurr = PnodeFromIpgOffset( pgOffset );

                Assert( pnodeCurr->IsValid() );
                ChangeCurr( pgOffset.ipg, pnodeCurr );
                goto HandleError;
            }
            else
            {
                // We can only get here if MovePrev() on a duplicate set of nodes, that don't have a prev.
                pnodeCurr->FDuplicateNext0();
            }
        }
    }

    // Currency is only reset if we move beyond the start of the skip list.
    // For other errors, e.g. IO errors while latching pages, currency stays
    // where it is.
    ResetCurr();
    err = ErrERRCheck( errBBTNodeNotFound );

HandleError:
    return err;
}

ERR BBTBuff::ErrGetDuplicate( _Out_ SkipListNode** ppnodeDup, _Inout_ int* piDup )
{
    ERR err = JET_errSuccess;
    SkipListNode* pnodeCurr = m_pnodeCurr;

    if ( pnodeCurr != NULL )
    {
        Assert( pnodeCurr->IsValid() );

        int i = 0;
        for ( i; i < *piDup; i++ )
        {
            if ( !pnodeCurr->LinkPrev0().FNull() )
            {
                SkipListNode* pnodePrev;
                Call( ErrPnodeFromLink_AcqLatch( pnodeCurr->LinkPrev0(), &pnodePrev ) );
                Assert( pnodePrev->IsValid() );

                if ( pnodePrev->FDuplicateNext0() )
                {
                    Assert( CmpKey( pnodePrev->Key(), pnodeCurr->Key() ) == 0 );
                    pnodeCurr = pnodePrev;
                    continue;
                }
            }

            break; // no more duplicates
        }

        Assert( 0 == m_pnodeCurr->CmpKey( pnodeCurr->Key() ) );
        *ppnodeDup = pnodeCurr;
        *piDup = i;
    }
    else
    {
        Assert( false );
        Error( ErrERRCheck( errBBTCurrencyLost ) );
    }

HandleError:
    return err;
}

ERR BBTBuff::ErrMoveToLatestInDuplicateSequence()
{
    ERR err = JET_errSuccess;
    SkipListLink linkLatest;

    Assert( m_pnodeCurr && m_pnodeCurr->IsValid() );
    CallR( ErrGetLatestInDuplicateSequence( m_pnodeCurr, &linkLatest ) );

    if ( !linkLatest.FNull() )
    {
        PageOffsetTuple pgOffset = IpgOffsetFromLink( linkLatest );
        SkipListNode* pnodeCurr = PnodeFromIpgOffset( pgOffset ); // should already be latched
        ChangeCurr( pgOffset.ipg, pnodeCurr );
    }

    return err;
}

// Moves to the most recent duplicate node of the given node,
// which is the last node of the current duplicate sequence.
// Moves forward through the sequence to find the tail of the duplicate sequence.
// Returns a null link if the current node is already the latest node in the duplicate sequence.
// NOTE: The caller is responsible for releasing latches !
ERR BBTBuff::ErrGetLatestInDuplicateSequence( SkipListNode* pnodeCurr, _Out_ SkipListLink* plinkLatestDup )
{
    ERR err = JET_errSuccess;
    SkipListLink linkCurr( 0 );
    SkipListNode* pnodeNext;

    while ( pnodeCurr->FDuplicateNext0() )
    {
        CallR( ErrPnodeFromLink_AcqLatch( pnodeCurr->LinkNext0(), &pnodeNext ) );
        Assert( pnodeNext != NULL );
        linkCurr = pnodeCurr->LinkNext0();
        pnodeCurr = pnodeNext;
    }

    *plinkLatestDup = linkCurr;
    return err;
}

ERR BBTBuff::ErrAssertIsLatestInDuplicateSequence( const SkipListNode* pnode )
{
    ERR err = JET_errSuccess;

#ifdef DEBUG
    // Assert that the given node is at the tail of a duplicate sequence.
    // That is, it's the latest node in the duplicate sequence.
    Assert( pnode->FDuplicateNext0() == false );

    SkipListNode* pnodeNext;
    CallR( ErrPnodeFromLink_AcqLatch( pnode->LinkNext0(), &pnodeNext ) );
    if ( pnodeNext != NULL )
    {
        Assert( pnode->CmpKey( pnodeNext->Key() ) < 0 );
    }
#endif

    return err;
}

ERR BBTBuff::ErrAssertIsOldestInDuplicateSequence( const SkipListNode* pnode )
{
    ERR err = JET_errSuccess;

#ifdef DEBUG
    SkipListNode* pnodePrev;
    CallR( ErrPnodeFromLink_AcqLatch( pnode->LinkPrev0(), &pnodePrev ) );
    if ( pnodePrev != NULL )
    {
        Assert( !pnodePrev->FDuplicateNext0() );
        Assert( pnodePrev->CmpKey( pnode->Key() ) < 0 );
    }
#endif

    return err;
}

#define ErrorIf( err, expr ) if ( expr ) { Error( ErrERRCheck( err ) ); }

ERR BBTBuff::ErrValidate()
{
    ERR                 err = JET_errSuccess;
    KEY                 rgKeyPrev[ MAX_LEVELS ];
    SkipListLink        rgLinksPrev[ MAX_LEVELS ];
    SkipListNode*       pnodeCurr;
    SkipListLink        linkPrev( 0 );
    int                 cNodes = 0;
    bool                fDuplicatePrev = false;

    // Check header.
    ErrorIf( JET_errBBTBuffCorrupted, m_pHeader->nVersion != bbtvInitial );
    ErrorIf( JET_errBBTBuffCorrupted, m_pHeader->cMaxPages != PFormat()->cpgInBBTBuff );

    Call( ErrLatchAll() );

    // Check DBTIMEs on each page of the BBTBuff. They should be the same.
    DBTIME dbtime = m_pcsrBase->Dbtime();
    for ( int i = 0; i < Cpg() - 1; i++ )
    {
        ErrorIf( JET_errBBTBuffCorrupted, dbtime != m_rgcsrLatched[ i ].Dbtime() );
    }

    // Start from header's skiplist links.
    {
        memset( rgKeyPrev, 0, sizeof( rgKeyPrev ) );
        SkipListLinkArray rgLinksHead = RgSkipListLinksHead( m_pHeader );
        for ( int i = 0; i < MAX_LEVELS; i++ )
        {
            rgLinksPrev[ i ] = rgLinksHead[ i ];
            ErrorIf( JET_errBBTBuffCorrupted, rgLinksPrev[ i ].ToInt() >= m_pHeader->le_ibMicFree->ToInt() );
        }
    }

    // Iterate over all nodes in the skiplist.
    pnodeCurr = PnodeFromLink( rgLinksPrev[ 0 ] );
    while ( pnodeCurr != NULL )
    {
        ErrorIf( JET_errBBTNodeCorrupted, !pnodeCurr->IsValid() );
        ErrorIf( JET_errBBTNodeCorrupted, pnodeCurr->Opcode() >= bbtOpUpsert ); // last valid opcode currently

        SkipListLink linkCurr = rgLinksPrev[ 0 ];
        KEY keyCurr = pnodeCurr->Key();

        // Level0 checks.
        if ( fDuplicatePrev )
        {
            ErrorIf( JET_errBBTBuffCorrupted, CmpKey( rgKeyPrev[ 0 ], keyCurr ) != 0 );
        }
        else
        {
            ErrorIf( JET_errBBTBuffCorrupted, CmpKey( rgKeyPrev[ 0 ], keyCurr ) >= 0 );
        }

        // If node is in order but the prev link of this node doesn't point to the prev node in sequence,
        // assume this node's prev link is corrupt (hence JET_errBBTNodeCorrupted).
        ErrorIf( JET_errBBTNodeCorrupted, linkPrev != pnodeCurr->LinkPrev0() );

        // Check that same node should occupy every level of the prev link array upto the curr node level.
        for ( int i = 1; i <= pnodeCurr->Level(); i++ )
        {
            ErrorIf( JET_errBBTBuffCorrupted, linkCurr != rgLinksPrev[ i ] );
        }
        // Since we've already checked keyCurr and link array, this means that rgKeyPrev[ level ] <= keyCurr for each level.

        cNodes++;
        ErrorIf( JET_errBBTBuffCorrupted, cNodes > m_pHeader->le_cNodes );

        // Check if current node protrudes into the unused region of the bbt buffer.
        int ibEndOfNode = SkipListLink::Roundup( linkCurr.ToInt() + pnodeCurr->Cb() );
        ErrorIf( JET_errBBTBuffCorrupted, ibEndOfNode > m_pHeader->le_ibMicFree->ToInt() );

        // Move state to next node.
        SkipListLinkArray rgLinks = pnodeCurr->RgLinksNext();
        for ( int i = 0; i <= pnodeCurr->Level(); i++ )
        {
            rgLinksPrev[ i ] = rgLinks[ i ];
            rgKeyPrev[ i ] = keyCurr;

            // Check links of the curr node.
            ErrorIf( JET_errBBTNodeCorrupted, rgLinksPrev[ i ].ToInt() >= m_pHeader->le_ibMicFree->ToInt() );
        }

        linkPrev = linkCurr;
        fDuplicatePrev = pnodeCurr->FDuplicateNext0();
        pnodeCurr = PnodeFromLink( rgLinksPrev[ 0 ] );
    }

    // All links should point to null at the end.
    for ( int i = 0; i < MAX_LEVELS; i++ )
    {
        ErrorIf( JET_errBBTBuffCorrupted, !rgLinksPrev[ i ].FNull() );
    }

HandleError:
    return err;
}

#undef ErrorIf

void BBTBuff::Load(
    _In_ PIB* ppib,
    IFMP ifmp,
    _In_ CSR* pcsr,
    _In_ CSRHeapArray& rgcsrBBT,
    BBTBuffHeader* pbbtbHeader,
    LATCH latchType )
{
    Assert( latchType == pcsr->Latch() );
    Assert( pbbtbHeader->nVersion == bbtvInitial );

    // Make sure that bbtbuff isn't already loaded
    Assert( m_ppib == NULL && m_ifmp == ifmpNil && m_pHeader == NULL && m_pcsrBase == NULL );

    EnforceSz( m_cMaxPages == 0 || m_cMaxPages == pbbtbHeader->cMaxPages, "BBTBuffInvalidMaxPages" );
    m_ifmt = (BYTE) IBBTBuffFormatForPage( g_cbPage );
    Assert( BBTBUFF_FORMAT_CONSTANTS[ m_ifmt ].cpgInBBTBuff < 256 );

    m_ppib = ppib;
    m_ifmp = ifmp;
    m_pHeader = pbbtbHeader;
    m_latchType = latchType;
    m_cMaxPages = m_pHeader->cMaxPages;

    Assert( rgcsrBBT != NULL );
    Assert( rgcsrBBT.CItems() == m_pHeader->cMaxPages - 1 );

    m_pcsrBase = pcsr;
    m_rgcsrLatched = rgcsrBBT;

    // BBTBuff doesn't insert tags/ilines. It uses the page data space as one big buffer.
    // But to keep CPAGE checks happy, we can't reclaim the reserved tag.
    m_cbPageDataMax = CPAGE::CbPageDataMaxNoInsert( g_cbPage );

    m_pnodeCurr = NULL;
    m_ipgCurr = -1;

    EnforceSz( FIsHeaderValid(), "BBTBuffInvalidHeader" );
}

void BBTBuff::Unload()
{
    ResetCurr();
    m_ppib = NULL;
    m_ifmp = ifmpNil;
    m_pHeader = NULL;
    m_pcsrBase = NULL;
    m_rgcsrLatched = NULL;
}

bool BBTBuff::FIsHeaderValid() const
{
    BYTE* pbHdr = (BYTE*) m_pHeader;
    return m_pHeader != NULL &&
            pbHdr > PbPage( 0 ) &&
            pbHdr + sizeof( BBTBuffHeader ) < PbPage( 0 ) + g_cbPage &&
            m_pHeader->nVersion == bbtvInitial &&
            m_pHeader->cMaxPages == PFormat()->cpgInBBTBuff &&
            m_pHeader->le_cbFree >= 0 && m_pHeader->le_cbFree <= CbMax() &&
            m_pHeader->le_cNodes >= 0 && m_pHeader->le_cNodes < CbMax() / sizeof( SkipListNode );
}

bool BBTBuff::FLoaded() const
{
    Assert( m_pHeader == NULL || FIsHeaderValid() );
    return ( m_pHeader != NULL );
}

ERR BBTBuff::ErrLatchAll()
{
    ERR err = JET_errSuccess;

    Assert( m_pcsrBase->FLatched() );
    for ( int i = 0; i < Cpg(); i++ )
    {
        CallR( ErrEnsurePageLatched( i, m_latchType ) );
    }

    return err;
}

ERR BBTBuff::ErrWriteLatchAll()
{
    ERR err = JET_errSuccess;

    if ( latchRIW == m_pcsrBase->Latch() )
    {
        m_pcsrBase->UpgradeFromRIWLatch();
    }
    else if ( latchWrite != m_pcsrBase->Latch() )
    {
        return ErrERRCheck( errBFLatchConflict );
    }

    for ( int i = 1; i < Cpg(); i++ )
    {
        CallR( ErrEnsurePageLatched( i, latchWrite ) );
    }

    m_latchType = latchWrite;
    return err;
}

void BBTBuff::Downgrade( LATCH latchType )
{
    Assert( m_latchType == latchWrite );
    Assert( latchType == latchReadTouch || latchType == latchRIW );
    m_latchType = latchType;

    DowngradeLatches();
}

ERR BBTBuff::ErrSetCurrNodeFromLink( SkipListLink link )
{
    ERR err = JET_errSuccess;
    Assert( m_pcsrBase->FLatched() );

    if ( link.FNull() )
    {
        ResetCurr();
        return ErrERRCheck( errBBTNodeNotFound );
    }

    PageOffsetTuple pgOffset = IpgOffsetFromLink( link );
    CallR( ErrEnsurePageLatched( pgOffset.ipg, m_latchType ) );

    SkipListNode* pnode = PnodeFromIpgOffset( pgOffset );
    Assert( pnode->IsValid() );
    ChangeCurr( pgOffset.ipg, pnode );
    return err;
}

bool BBTBuff::FCanInsert( const KEY& key, const DATA& data, _Out_ int* pcbReq )
{
    // Check for level0 node. Insertion code will first try with a random level, then fallback to level0.
    int cbNodeInsert = SkipListLink::Roundup( SkipListNode::Cb( 0, key.Cb(), data.Cb() ) );
    // UA_TODO: we can still hit this because BBTBuff nodes have a minimum 8-byte overhead vs 6-bytes for BTs.
    EnforceSz( cbNodeInsert <= CbMaxNodeSize(), "BBTBuff MaxNodeSizeExceeded" );

    if ( pcbReq )
    {
        *pcbReq = cbNodeInsert;
    }

    Assert( CbMax() - m_pHeader->le_ibMicFree->ToInt() >= 0 );
    if ( cbNodeInsert <= CbMax() - m_pHeader->le_ibMicFree->ToInt() )
    {
        return true;
    }
    else
    {
        return false;
    }
}

void BBTBuff::InitBBTBuffHeader( _Out_ BBTBuffHeader* pbbtbHeader, _In_ int cMaxPages )
{
    memset( pbbtbHeader, 0, sizeof( BBTBuffHeader ) );

    pbbtbHeader->nVersion = bbtvInitial;

    Assert( cMaxPages < 256 );
    pbbtbHeader->cMaxPages = (BYTE) cMaxPages;

    pbbtbHeader->le_ibMicFree = SkipListLink::FromInt( sizeof( BBTBuffHeader ) );  // leave space for the header
    pbbtbHeader->le_cbFree = CbMax() - pbbtbHeader->le_ibMicFree->ToInt();

    // Fields zero-ed out:
    // count
    // rgSkipListLinksHead[]
}

void BBTBuff::InitBBTBuffRoot( FUCB* pfucb, CSR* pcsr )
{
    const BBTBuffFormat*    pFormat = BBTBuff::PBBTBuffFormatForPage( g_cbPage );

    // Init BBTBuff root page (base page, not tree root)
    //
    Assert( latchWrite == pcsr->Latch() );
    Assert( pcsr->FDirty() );
    pcsr->SetILine( 0 );

    Assert( 0 == pcsr->Cpage().Clines() ); // must not have any data on the page
    Assert( pcsr->Cpage().CbPageFree() > pFormat->cbBBTRoot );

    // All of these flags should be set
    const ULONG fReqPageFlagsRoot = ( CPAGE::fPageBBTBuff | CPAGE::fPageBBTBuffRoot );
    Assert( fReqPageFlagsRoot == ( fReqPageFlagsRoot & pcsr->Cpage().FFlags() ) );

    int cb = sizeof( NodeResvTag ) + pFormat->cbBBTRoot;
    INT itag = INDAddReservedTag( pfucb, pcsr, rtidBBTBuff, cb, 0 );

    // The second reserved tag on the page must be the BBTBuff root tag.
    EnforceSz( itag == itagReservedBBTBuffRoot, "InitBBTBuffRoot: BadReservedTag" );

    LINE line;
    GetBBTBuffRoot( *pcsr, &line );
    BBTBuff::InitBBTBuffHeader( PBBTHeader( line ), pFormat->cpgInBBTBuff );
}

void BBTBuff::InitBBTBuffNonRoot( CSR* pcsr )
{
    const INT cbPageDataMax = (INT) pcsr->Cpage().CbPageDataMax();

    // Init non-root BBTBuff pages
    //
    Assert( latchWrite == pcsr->Latch() );
    Assert( pcsr->FDirty() );

    const ULONG fReqPageFlags = CPAGE::fPageBBTBuff;
    Assert( fReqPageFlags == ( fReqPageFlags & pcsr->Cpage().FFlags() ) );

    pcsr->Cpage().ResetReservedTag( 0, cbPageDataMax, 0 );  // take over external header tag
}

void BBTBuff::GetBBTBuffRoot( const CSR& csr, LINE* pline )
{
    Assert( csr.FLatched() );
    Assert( csr.Cpage().FBBTBuffRootPage() );  // can only be a root or internal page

    csr.Cpage().GetPtrReservedTag( itagReservedBBTBuffRoot, pline );
    NodeResvTag* pResvTag = (NodeResvTag*) pline->pv;

    Assert( rtidBBTBuff == pResvTag->resvTagId );
    Assert( pline->cb == sizeof( NodeResvTag ) + (ULONG) BBTBuff::PBBTBuffFormatForPage( g_cbPage )->cbBBTRoot );
}

BBTBuffHeader* BBTBuff::PBBTHeader( const LINE& line )
{
    NodeResvTag* pResvTag = (NodeResvTag*) line.pv;
    Assert( rtidBBTBuff == pResvTag->resvTagId );
    Assert( line.cb == sizeof( NodeResvTag ) + (ULONG) BBTBuff::PBBTBuffFormatForPage( g_cbPage )->cbBBTRoot );
    return (BBTBuffHeader*) pResvTag->rgb;
}


///////////////////////////////////////////////////////////////////////////////
// Debug helpers

std::string DumpBBTBuffHeader( const BBTBuffHeader* pHeader )
{
    std::string str = "";
    char buff[ 256 ];

    sprintf_s( buff, FORMAT_UINT( BBTBuffHeader, pHeader, nVersion, 0 ) );
    str.append( buff );

    sprintf_s( buff, FORMAT_UINT( BBTBuffHeader, pHeader, cMaxPages, 0 ) );
    str.append( buff );

    sprintf_s( buff, FORMAT_( BBTBuffHeader, pHeader, le_ibMicFree, 0 ) );
    str.append( buff );
    sprintf_s( buff, "%d (0x%X)\n", pHeader->le_ibMicFree->ToInt(), pHeader->le_ibMicFree->ToInt() );
    str.append( buff );

    sprintf_s( buff, FORMAT_INT( BBTBuffHeader, pHeader, le_cbFree, 0 ) );
    str.append( buff );

    sprintf_s( buff, FORMAT_UINT( BBTBuffHeader, pHeader, le_cNodes, 0 ) );
    str.append( buff );

    return str;
}

std::string DumpBBTBuff( const BBTBuff& bbtBuff, INT level, INT ib /* = 0 */ )
{
    std::string str;
    char buff[ 256 ];

    str.reserve( 4096 );    // just get some initial size going

    int pgnoFirst = bbtBuff.Pcsr( 0 )->Pgno();
    int pgnoLast = bbtBuff.Pcsr( bbtBuff.Cpg() - 1 )->Pgno();
    sprintf_s( buff, "BBT Buffer Superpage spanning pgnos [%d (0x%x), %d (0x%x)]\n\n", pgnoFirst, pgnoFirst, pgnoLast, pgnoLast );
    str.append( buff );
    str.append( DumpBBTBuffHeader( bbtBuff.m_pHeader ) );
    str += '\n';

    __try
    {
        SkipListLink linkCurr = ( ib == 0 ) ? RgSkipListLinksHead( bbtBuff.m_pHeader )[ 0 ] : SkipListLink::FromInt( ib );
        while ( !linkCurr.FNull() )
        {
            const SkipListNode* pnodeCurr = bbtBuff.PnodeFromLink( linkCurr );
            str.append( bbtBuff.PnodeCurr() == pnodeCurr ? "-->" : "   " );
            str.append( level == 0 && pnodeCurr->FDuplicateNext0() ? "D " : "  " );
            str.append( pnodeCurr->Key().ToString() );
            sprintf_s( buff, ", Op( %d ), Level( %d ), Cb( %d ), Data( %d ) ",
                pnodeCurr->Opcode(),
                pnodeCurr->Level(),
                pnodeCurr->Cb(),
                pnodeCurr->CbData() );
            str.append( buff );

            SkipListLink linkNext;
            if ( level == 0 )
            {
                linkNext = pnodeCurr->LinkNext0();
                sprintf_s( buff, "Link( %d ), Prev0( %d ), Next0( %d ): ", linkCurr.ToInt(), pnodeCurr->LinkPrev0().ToInt(), linkNext.ToInt() );
            }
            else
            {
                int maxLevel = min( level, pnodeCurr->Level() );
                linkNext = pnodeCurr->RgLinksNext()[ maxLevel ];
                sprintf_s( buff, "Link( %d ), Next%d( %d ): ", linkCurr.ToInt(), maxLevel, linkNext.ToInt() );
            }
            str.append( buff );

            DATA data = pnodeCurr->Data();
            bool fLargeData = data.Cb() > 16;
            data.SetCb( fLargeData ? 16 : data.Cb() );
            str.append( data.ToString() );
            str.append( fLargeData ? "...\n" : "\n" );

            linkCurr = linkNext;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        sprintf_s( buff, "\n ---- Encountered expcetion: 0x%x ----\n", GetExceptionCode() );
        str.append( buff );
    }

    return str;

// Force includes the function even if there are no calls to it.
// This allows the function to be available for debugging in VS.
#pragma comment(linker, "/include:" __FUNCDNAME__)
}

