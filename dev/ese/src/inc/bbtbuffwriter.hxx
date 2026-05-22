// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "bbtbuff.hxx"

/************************************************************************************************************
Provides a wrapper that allows transacted write operations on a BBTBuff.
IBBTBuffTrxLog defines the interface that can be implemented to capture the operations in a trx log.
The implementation of IBBTBuffTrxLog is responsible for correctly recording the details of the operation,
and ensuring durability/redo guarantees.

BBTBuffWriter only guarantees that each write operation is atomic, and in case of a failure,
there will be no side-effects and the operation will be cleanly undone.

Certain trx log concepts from the engine above have leaked into the BBTBuffWriter.
Following are managed here for the BBT buffer.
1. DBTIMEs and page dirtying.
************************************************************************************************************/

// Interface class for a transaction logger needed for write operations on a BBTBuff.
// Uses CRTP to do static polymorphism.
template <typename TImpl>
class IBBTBuffTrxLog
{
public:
    BBTBuff* const  m_pbbtbuff;
    DBTIME          m_dbtimeCoordinated = dbtimeNil;

    IBBTBuffTrxLog( BBTBuff * pbbtbuff ) : m_pbbtbuff( pbbtbuff )
    {}

    ERR ErrLogInsert(
        const BBTBuffChangeContext& changeCtx,
        BBTBuffOpcode               opcode,
        const KEY&                  key,
        const DATA&                 data,
        SkipListNodeFlags           flags )
    {
        return static_cast<TImpl*>( this )->ErrLogInsert( changeCtx, opcode, key, data, flags );
    }

    ERR ErrLogDelete(
        const BBTBuffChangeContext& changeCtx,
        const SkipListNode*         pnodeDel )
    {
        return static_cast<TImpl*>( this )->ErrLogDelete( changeCtx, pnodeDel );
    }

    ERR ErrLogRangeDelete(
        DBTIME          dbtimeBefore,
        DBTIME          dbtimeCurr,
        SkipListLink    linkFirst,
        int             cNodes )
    {
        return static_cast<TImpl*>( this )->ErrLogRangeDelete( dbtimeBefore, dbtimeCurr, linkFirst, cNodes );
    }

    ERR ErrLogMergeAndDel(
        DBTIME                                              dbtimeBefore,
        DBTIME                                              dbtimeCurr,
        _In_count_( cNodesMerged ) const SkipListNode**     rgNodesMerged,
        _In_count_( cNodesDel )  SkipListLink*              rgLinksDel,
        int                                                 cNodesMerged,
        int                                                 cNodesDel,
        SkipListLink                                        linkIbMergeStart )
    {
        return static_cast<TImpl*>( this )->ErrLogMergeAndDel( dbtimeBefore, dbtimeCurr, rgNodesMerged, rgLinksDel, cNodesMerged, cNodesDel, linkIbMergeStart );
    }

protected:
    // BBTBuff interface exposed to actual implementations of IBBTBuffTrxLog

    int Cpg() const                             { return m_pbbtbuff->Cpg(); }
    CPAGE& BBTBuffCpage( int ipg )              { return m_pbbtbuff->Pcsr( ipg )->Cpage(); }
};

// Helper class to manage CoordinatedDirty on a BBTBuff.
// If commit isn't called, the dbtimes will be reverted back by the destructor.
class DbTimeGuard
{
    BBTBuff*    m_pbbtBuff;
    DBTIME      m_dbtimeBefore;
    ULONG       m_fPageFlags;

public:
    DbTimeGuard( BBTBuff* pbbtBuff ) : 
        m_pbbtBuff( pbbtBuff ),
        m_dbtimeBefore( pbbtBuff->m_pcsrBase->Dbtime() )
    {}

    void CoordinatedDirty( DBTIME dbtime )
    {
        m_fPageFlags = m_pbbtBuff->m_pcsrBase->Cpage().FFlags();
        Assert( ( m_fPageFlags & CPAGE::fPageBBTBuffRoot ) && ( m_fPageFlags & CPAGE::fPageBBTBuff ) );
        m_fPageFlags &= ( ~CPAGE::fPageBBTBuffRoot );   // Remove flag because it should only be present on the base page

        // Requires pageset to be setup correctly
        dbtime == dbtimeNil ? m_pbbtBuff->m_pcsrBase->Dirty() : m_pbbtBuff->m_pcsrBase->CoordinatedDirty( dbtime );
        dbtime = m_pbbtBuff->m_pcsrBase->Dbtime();

        for ( int i = 0; i < m_pbbtBuff->Cpg() - 1; i++ )
        {
            Assert( m_fPageFlags == m_pbbtBuff->m_rgcsrLatched[ i ].Cpage().FFlags() );
            Assert( m_pbbtBuff->m_rgcsrLatched[ i ].Dbtime() == m_dbtimeBefore );
            m_pbbtBuff->m_rgcsrLatched[ i ].CoordinatedDirty( dbtime );
        }
    }

    void Commit()               { m_dbtimeBefore = dbtimeNil; }
    ~DbTimeGuard()
    { 
        if ( m_dbtimeBefore != dbtimeNil )
        {
            // DbTimeGuard doesn't protect against page flag modifications.
            // BBTBuff doesn't modify any page flags during its write operations.

            m_pbbtBuff->m_pcsrBase->RevertDbtime( m_dbtimeBefore, m_fPageFlags | CPAGE::fPageBBTBuffRoot );
            for ( int i = 0; i < m_pbbtBuff->Cpg() - 1; i++ )
            {
                m_pbbtBuff->m_rgcsrLatched[ i ].RevertDbtime( m_dbtimeBefore, m_fPageFlags );
            }
        }
    }

    DBTIME DbtimeBefore()
    {
        Assert( m_dbtimeBefore >= dbtimeStart );
        return m_dbtimeBefore;
    }

    DBTIME DbtimeNew()
    { 
        Assert( m_dbtimeBefore >= dbtimeStart );
        return m_pbbtBuff->m_pcsrBase->Dbtime();
    }
};

// A simple iterator over a sequence of SkipListNode*
// The iterator starts positioned before the first element (like c# IEnumerator). Call Next() before calling Curr().
// Provides a level of indirection to implement more complex iterators over a sequence of SkipListNode*
// (required by FT Split/Evict)
// can be generalized using standard c++ iterators and a type-erasing any_iterator
class INodeSequence
{
public:
    virtual const SkipListNode* Curr() = 0;
    virtual bool Next() = 0;
    virtual void Reset() = 0;
};

// A node sequence over a BBTBuff
class NodeSequenceBBTBuff : public INodeSequence
{
    BBTBuff&            m_bbtbuff;
    SkipListLink        m_linkFirst;
    BBTBuff::SeekFlags  m_fSeekFlags;
    bool                m_fBeforeFirst; // BBTBuff doesn't support BeforeFirst cursor position

public:
    NodeSequenceBBTBuff( BBTBuff& bbtbuff, BBTBuff::SeekFlags fFlags ) :
        m_bbtbuff( bbtbuff ),
        m_linkFirst( bbtbuff.GetLinkToCurrNode() ),
        m_fSeekFlags( fFlags ),
        m_fBeforeFirst( true )
    {}

    virtual const SkipListNode* Curr()  { return ( !m_fBeforeFirst ? m_bbtbuff.PnodeCurr() : NULL ); }
    virtual bool Next()
    {
        if ( !m_fBeforeFirst )
        {
            ERR err = m_bbtbuff.ErrMoveNext( m_fSeekFlags );
            if ( err != errBBTNodeNotFound )
            {
                EnforceSz( err >= JET_errSuccess, "NodeSequenceBBTBuff_MoveNext" );
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            m_fBeforeFirst = false;
            return true;
        }
    }

    virtual void Reset()
    { 
        ERR err = m_bbtbuff.ErrSetCurrNodeFromLink( m_linkFirst );
        EnforceSz( err >= JET_errSuccess, "NodeSequenceBBTBuff_Reset" );
        m_fBeforeFirst = true;
    }
};

// Provides transacted writes on a BBTBuff.
template <typename TLogger>
class BBTBuffWriter
{
private:
    BBTBuff*                    m_pbbtbuff;
    IBBTBuffTrxLog<TLogger>*    m_pTrxLogger;

public:
    BBTBuffWriter( BBTBuff* pbbtbuff, IBBTBuffTrxLog<TLogger>* pTrxLogger ) :
        m_pbbtbuff( pbbtbuff ),
        m_pTrxLogger( pTrxLogger )
    {
        Assert( pbbtbuff == pTrxLogger->m_pbbtbuff );
    }

private:
    void CoordinatedDirty();
    SkipListLink GetLinkNew( int cb );
    ERR ErrReorg_ProcessDeletes(
        _In_count_( cLinksDel ) SkipListLink*   rgLinksDel,
        const int                               cLinksDel,
        _Out_ SkipListLink*                     plinkIbMerge );

public:
    ERR ErrUpgradeToWriteMode();
    ERR ErrInsert( BBTBuffOpcode opcode, const KEY& key, const DATA& data, SkipListNodeFlags flags );
    ERR ErrDelete( const KEY& key );
    ERR ErrFlagDelete( SkipListLink link );
    ERR ErrReorganize();


    // APIs for Evict
 
    ERR ErrRangeDelete( SkipListLink linkFirst, int cNodes );   // deletes a range of nodes defined by link, count

    // Merges in the given sequence of nodes and deleting the nodes in the delete sequence, while potentially reorganizing the buffer if needed.
    ERR ErrMergeAndDelNodes(
        _In_count_( cNodesMerge ) const SkipListNode**  rgNodesMerge,
        _In_count_( cNodesToDel ) SkipListLink*         rgLinksDel,
        const int                                       cNodesMerge,
        const int                                       cNodesToDel,
        _Out_ int*                                      pcNodesMerged,
        _Out_ int*                                      pcNodesDel,
        int                                             cbReorgThreshold );
};

// Uses dbtime of the root to dirty the rest of the pages
template <typename TLogger>
void BBTBuffWriter<TLogger>::CoordinatedDirty()
{
    // All pages should be write-latched and root already dirtied
    Assert( m_pbbtbuff->m_pcsrBase->FDirty() );
    DBTIME dbtime = m_pbbtbuff->m_pcsrBase->Dbtime();

    for ( int i = 0; i < m_pbbtbuff->Cpg() - 1; i++ )
    {
        m_pbbtbuff->m_rgcsrLatched[ i ].CoordinatedDirty( dbtime, bfdfDirty );
    }
}

template <typename TLogger>
ERR BBTBuffWriter<TLogger>::ErrUpgradeToWriteMode()
{
    ERR err = JET_errSuccess;

    Assert( m_pbbtbuff->m_pcsrBase->FLatched() );
    m_pbbtbuff->m_latchType = m_pbbtbuff->m_pcsrBase->Latch();

    for ( int i = 0; i < m_pbbtbuff->Cpg(); i++ )
    {
        CSR* pcsr = m_pbbtbuff->Pcsr( i );
        LATCH latchCurr = pcsr->Latch();
        if ( latchCurr == latchReadTouch || latchCurr == latchReadNoTouch )
        {
            err = pcsr->ErrUpgradeFromReadLatch();
            if ( err < JET_errSuccess )
            {
                // we lose our latch if the upgrade fails
                // BBTBuff is unusable and we must release all latches.
                for ( int j = 0; j < m_pbbtbuff->Cpg(); j++ )
                {
                    m_pbbtbuff->Pcsr( j )->ReleasePage();
                }

                m_pbbtbuff->Unload();
                return err;
            }
        }
        else if ( latchCurr == latchRIW )
        {
            pcsr->UpgradeFromRIWLatch();
        }
        else
        {
            Assert( latchCurr == latchNone || latchCurr == latchWrite );
        }
    }

    m_pbbtbuff->m_latchType = latchWrite;
    return err;
}

template <typename TLogger>
SkipListLink BBTBuffWriter<TLogger>::GetLinkNew( int cb )
{
    Assert( latchWrite == m_pbbtbuff->m_pcsrBase->Latch() );

    BBTBuffHeader* pHeader = m_pbbtbuff->m_pHeader;
    SkipListLink ibMicFree = pHeader->le_ibMicFree;
    PageOffsetTuple pgOffset = m_pbbtbuff->IpgOffsetFromLink( pHeader->le_ibMicFree );
    int cbUsed = SkipListLink::Roundup( cb );   // count wasted space too
    Assert( cbUsed >= cb );

    if ( cbUsed > m_pbbtbuff->IbPageDataEnd( pgOffset.ipg ) - pgOffset.ibOnPage )
    {
        if ( pgOffset.ipg < m_pbbtbuff->Cpg() - 1 )
        {
            pgOffset.ipg++;
            ibMicFree = m_pbbtbuff->LinkFromIpgOffset( pgOffset.ipg, m_pbbtbuff->IbPageDataBegin( pgOffset.ipg ) );
        }
        else
        {
            // Caller should have checked for free space before calling this function
            EnforceSz( false, "BBTBuff: errBBTBuffFull" );
        }
    }

    // Caller should ensure that the page referenced by the returned link is latched
    return ibMicFree;
}

// Inserts a new node into the list.
// Duplicate nodes are inserted at the tail of a duplicate sequence.
template <typename TLogger>
ERR BBTBuffWriter<TLogger>::ErrInsert( BBTBuffOpcode opcode, const KEY& key, const DATA& data, SkipListNodeFlags flags )
{
    ERR err = JET_errSuccess;

    m_pbbtbuff->ResetCurr();
    Call( m_pbbtbuff->ErrWriteLatchAll() );

    // Check/prepare for the insertion
    // All modifications are deferred until we know that insert can succeed unconditionally
    {
        BBTBuffHeader* pHeader = m_pbbtbuff->m_pHeader;
        BBTBuffChangeContext changeCtx{};    // zero-initializes
        int level = m_pbbtbuff->GenLevel();
        int cbNode = SkipListNode::Cb( level, key.Cb(), data.Cb() );

        // Check for space and max supported node size.
        if ( cbNode > m_pbbtbuff->CbMax() - pHeader->le_ibMicFree->ToInt() ||
             cbNode > m_pbbtbuff->CbMaxNodeSize() )
        {
            // Check again with level 0 node.
            level = 0;
            cbNode = SkipListNode::Cb( level, key.Cb(), data.Cb() );
            EnforceSz( cbNode <= m_pbbtbuff->CbMaxNodeSize(), "BBTBuff MaxNodeSizeExceeded" );

            if ( cbNode > m_pbbtbuff->CbMax() - pHeader->le_ibMicFree->ToInt() )
            {
                Call( ErrERRCheck( errBBTBuffFull ) );
            }
        }

        int result;
        SkipListNode* rgNodes[ MAX_LEVELS ];
        Call( m_pbbtbuff->ErrSeek_( key, BBTBuff::SeekMode::LEQ, BBTBuff::SeekPos::Curr, &result, changeCtx.rgLinksPrev, rgNodes ) );
        bool fDuplicate = ( result == 0 );

        // ErrSeek_() gives us the links to each node at every level at the insertion point.
        // To insert, we only need to fix the links that are at the same level or below as the newly inserted node.

        // Get node next to the insertion point at level 0
        // This node's m_linkPrev0 pointer needs to be modified
        SkipListLink linkNext0 = ( rgNodes[ 0 ] ? rgNodes[ 0 ]->LinkNext0() : RgSkipListLinksHead( pHeader )[ 0 ] );

        SkipListLink linkNew = GetLinkNew( cbNode );
        changeCtx.linkCurr = linkNew;
        changeCtx.level = level;

        // All preparation/checks succeeded
        // LOG the insert operation
        {
            DbTimeGuard dbtimeGuard( m_pbbtbuff );
            dbtimeGuard.CoordinatedDirty( m_pTrxLogger->m_dbtimeCoordinated );
            changeCtx.dbtimeBefore = dbtimeGuard.DbtimeBefore();
            changeCtx.dbtimeCurr = dbtimeGuard.DbtimeNew();

            Call( m_pTrxLogger->ErrLogInsert( changeCtx, opcode, key, data, flags ) );
            dbtimeGuard.Commit();
        }

        // WARNING: Can't fail after this point !!!
        SkipListNode* pnodeNew = m_pbbtbuff->PnodeInsert_( changeCtx, rgNodes, opcode, key, data, flags, fDuplicate );

        PageOffsetTuple pgOffsetNew = m_pbbtbuff->IpgOffsetFromLink( linkNew );
        m_pbbtbuff->ChangeCurr( pgOffsetNew.ipg, pnodeNew );
        Assert( linkNew == m_pbbtbuff->GetLinkToCurrNode() );   // sanity check
    }

HandleError:
    m_pbbtbuff->DowngradeLatches();
    return err;
}

template <typename TLogger>
ERR BBTBuffWriter<TLogger>::ErrDelete( const KEY& key )
{
    ERR err = JET_errSuccess;

    m_pbbtbuff->ResetCurr();
    Call( m_pbbtbuff->ErrWriteLatchAll() );

    // Check/Prepare for deletion
    // All modifications are deferred until we know that delete can succeed unconditionally
    {
        int result;
        BBTBuffChangeContext changeCtx{};    // zero-initializes
        SkipListNode* rgNodes[ MAX_LEVELS ];
        BBTBuffHeader* pHeader = m_pbbtbuff->m_pHeader;
        Call( m_pbbtbuff->ErrSeek_( key, BBTBuff::SeekMode::LEQ, BBTBuff::SeekPos::Prev, &result, changeCtx.rgLinksPrev, rgNodes ) );

        // ErrSeek_() will position at the node prev to the node to be deleted.
        // result will tell us if the next node is an exact match or not.

        if ( result == 0 )
        {
            // Get link to the node to delete.
            SkipListLink linkDel = !changeCtx.rgLinksPrev[ 0 ].FNull() ? rgNodes[ 0 ]->LinkNext0() : RgSkipListLinksHead( pHeader )[ 0 ];
            SkipListNode* pnodeToDelete = m_pbbtbuff->PnodeFromLink( linkDel );
            Assert( pnodeToDelete != NULL );
            Assert( pnodeToDelete->CmpKey( key ) == 0 );

            changeCtx.linkCurr = linkDel;
            changeCtx.level = pnodeToDelete->Level();

            // The seek loop above guarantees that we always land at the latest node in a duplicate sequence.
            // Enforce that. Deleting a node that isn't the latest duplicate version will cause corruption !
            Call( m_pbbtbuff->ErrAssertIsLatestInDuplicateSequence( pnodeToDelete ) );
            EnforceSz( !pnodeToDelete->FDuplicateNext0(), "BBTBuff::Delete_BadDupFlag" );

            // All preparation/checks succeeded
            // LOG delete operation
            {
                DbTimeGuard dbtimeGuard( m_pbbtbuff );
                dbtimeGuard.CoordinatedDirty( m_pTrxLogger->m_dbtimeCoordinated );
                changeCtx.dbtimeBefore = dbtimeGuard.DbtimeBefore();
                changeCtx.dbtimeCurr = dbtimeGuard.DbtimeNew();

                Call( m_pTrxLogger->ErrLogDelete( changeCtx, pnodeToDelete ) );
                dbtimeGuard.Commit();
            }

            // WARNING: Can't fail after this point !!!
            m_pbbtbuff->Delete_( changeCtx, rgNodes, pnodeToDelete );
        }
        else
        {
            err = ErrERRCheck( errBBTNodeNotFound );
        }
    }

HandleError:
    m_pbbtbuff->DowngradeLatches();
    return err;
}

template <typename TLogger>
ERR BBTBuffWriter<TLogger>::ErrFlagDelete( SkipListLink link )
{
    ERR err = JET_errSuccess;
    Call( m_pbbtbuff->ErrWriteLatchAll() );
    m_pbbtbuff->m_pcsrBase->Dirty();
    CoordinatedDirty();

    SkipListNode* pnode = m_pbbtbuff->PnodeFromLink( link );
    pnode->SetDeleted( true );

HandleError:
    m_pbbtbuff->DowngradeLatches();
    return err;
}

template <typename TLogger>
ERR BBTBuffWriter<TLogger>::ErrReorganize()
{
    int cNodesMerged = 0;
    int cNodesDel = 0;
    ERR err = ErrMergeAndDelNodes( NULL, NULL, 0, 0, &cNodesMerged, &cNodesDel, -m_pbbtbuff->CbMax() );  // always reorg
    Assert( cNodesMerged == 0 );
    Assert( cNodesDel == 0 );
    return err;
}

template <typename TLogger>
ERR BBTBuffWriter<TLogger>::ErrRangeDelete( SkipListLink linkFirst, int cNodes )
{
    ERR err = JET_errSuccess;

    m_pbbtbuff->AssertLatchedAll( latchWrite ); // not dirty yet
    DbTimeGuard dbtimeGuard( m_pbbtbuff );
    dbtimeGuard.CoordinatedDirty( m_pTrxLogger->m_dbtimeCoordinated );
    DBTIME dbtimeBefore = dbtimeGuard.DbtimeBefore();
    DBTIME dbtimeCurr = dbtimeGuard.DbtimeNew();

    CallR( m_pTrxLogger->ErrLogRangeDelete( dbtimeBefore, dbtimeCurr, linkFirst, cNodes ) );
    dbtimeGuard.Commit();

    m_pbbtbuff->RangeDelete_( linkFirst, cNodes );
    return err;
}

// Returns the cost of doing a full reorg relative to doing inplace operations.
// where cNodesTotal is the total nodes in a BBTBuff,
// and the return value is the number of nodes to inplace delete/insert to match that cost.
// For example, for input -> out
//                  0    -> 0
//                  100  -> 15
//                  500  -> 55
//                  1000 -> 100
//                  2500 -> 221
//                  5000 -> 406
INLINE int CalcSkipListMergeHeuristic( int n )
{
    // Skiplist has avg insert/delete performance of log2(n)
    // Each reorg requires individually visiting every node and copying it (n operations).
    // Then copying back the reorg-ed buffers.
    // Assume that the nodes are small enough that cost of copying is negligible compared to visiting the node.
    // So if n = log2(n) * cOPs (where cOPs = the number of inplace ops).
    // return cOps = n / log2(n)
    const double log10Of2 = 0.30102999566398119521373889472449;
    return n > 0 ? static_cast<int>( n / ( log10( n ) / log10Of2 ) ) : 0;
}

// Reorganize and merge: Merges incoming nodes in pseqNodesMerge while deleting local nodes specified by pseqNodesDel.
// 1. Makes a logical copy of the list by allocating new page sized buffers on the heap
//    and copying over all the nodes.
//    - If copying is impossible or inefficient, does an inplace merge-delete.
//    - Caller can specify cbReorgthreshold to force a reorg if doing a reorg would return atleast that much empty space.
// 2. Merges nodes in pseqNodesMerge into the copied list. Merge sequence must be sorted.
// 3. Removes any local nodes that are in the delete sequence. Del sequence must be sorted.
// 4. The new pages are mem-copied back into the cpage buffers.
// 5. The BBTBuff header is adjusted to reflect the new reality, in the end.
template <typename TLogger>
ERR BBTBuffWriter<TLogger>::ErrMergeAndDelNodes(
    _In_count_( cNodesMerge ) const SkipListNode**  rgNodesMerge,
    _In_count_( cNodesToDel ) SkipListLink*         rgLinksDel,
    const int                                       cNodesMerge,
    const int                                       cNodesToDel,
    _Out_ int*                                      pcNodesMerged,
    _Out_ int*                                      pcNodesDel,
    int                                             cbReorgThreshold )
{
    Assert( pcNodesMerged != NULL );
    Assert( pcNodesDel != NULL );
    m_pbbtbuff->AssertLatchedAll( latchWrite ); // not dirty yet
    m_pbbtbuff->ResetCurr();

    ERR             err = JET_errSuccess;
    BYTE**          rgpbPage = NULL;
    SkipListLink    linkIbMergeStart( 0 );
    const int       cMaxPages = m_pbbtbuff->Cpg();

    Call( m_pbbtbuff->ErrWriteLatchAll() );

    err = ErrReorg_ProcessDeletes( rgLinksDel, cNodesToDel, &linkIbMergeStart );

    if ( err == errBBTBuffFull )
    {
        // Reorg can't be done because existing data expanded because of skiplist leveling
        // and there weren't enough deleted nodes to cover the difference.
        *pcNodesDel = 0;
        *pcNodesMerged = 0;
        goto HandleError;
    }

    const int cbReorgGained = m_pbbtbuff->m_pHeader->le_ibMicFree->ToInt() - linkIbMergeStart.ToInt();  // can be negative
    if ( cbReorgGained < cbReorgThreshold )
    {
        const int heuristic = CalcSkipListMergeHeuristic( m_pbbtbuff->CNodes() );
        if ( cNodesToDel < heuristic )
        {
            // We may be doing too less to do a full reorg.
            // Calculate approx count of merges too.
            int cbLeftReorg = m_pbbtbuff->CbMax() - linkIbMergeStart.ToInt();
            int cbLeftNoReorg = m_pbbtbuff->CbMax() - m_pbbtbuff->m_pHeader->le_ibMicFree->ToInt();
            int cOpsReorg = cNodesToDel;
            int cOpsNoReorg = cNodesToDel;

            for ( int i = 0; i < cNodesMerge; i++ )
            {
                // Merge count is calculated off of cbLeft after reorg.
                // It should be close to current cbFree (assuming we are doing few deletes, thats why we are here).
                // In which case it doesn't matter much, or if we did delete some large node, then we assume that
                // most nodes are small and cOpsExpected will climb higher than the heuristic, skipping in-place merge.
                const SkipListNode* pnodeMerge = rgNodesMerge[ i ];
                const int cb = SkipListNode::Cb( cOpsReorg, pnodeMerge->CbKey(), pnodeMerge->CbData() );
                cbLeftReorg -= cb;
                cbLeftNoReorg -= cb;

                if ( cbLeftNoReorg >= 0 )
                {
                    cOpsNoReorg++;
                }

                if ( cbLeftReorg < 0 || cOpsReorg > heuristic )   // count 1 over the heuristic
                {
                    break;
                }

                cOpsReorg++;
            }

            // UA_TODO: this works well for small sized nodes. Too much variance in node sizes, or large nodes will cause problems.
            // For example, lets say we delete 1 large node on a full bbtbuff, and try to merge in a few small nodes.
            // The merge might not be possible without reorg, but we will try to do an inplace merge.
            // This atleast makes forward progress (the large node will be deleted, but no merges will be done).
            // The next evict will have more forward progress.

            // UA_TODO: Enable code below. Needs logging support for in-place merge and del.
            //if ( cOpsReorg < heuristic && cOpsNoReorg == cOpsReorg )
            //{
            //    // If we are doing a small number of merge and deletes then use in-place merge,
            //    // and we can do the same number of operations with a reorg.
            //    Call( ErrInplaceMergeAndDelNodes( rgNodesMerge, rgNodesDel, cNodesMerge, cNodesDel, pcNodesMerged, pcNodesDel ) );
            //    goto HandleError;
            //}
        }
    }

    Alloc( rgpbPage= (BYTE**) _alloca( sizeof(BYTE*) * cMaxPages ) );
    for ( int i = 0; i < cMaxPages; i++ )
    {
        BFAlloc( bfasTemporary, (void**) &rgpbPage[ i ], m_pbbtbuff->m_pcsrBase->Cpage().CbPage() );
        Alloc( rgpbPage[ i ] );
    }

    int cNodesMerged = 0;
    int cNodesDeleted = 0;
    m_pbbtbuff->CopyMergeAndDelNodes_(
            rgpbPage,
            rgNodesMerge,
            rgNodesMerge + cNodesMerge,
            rgLinksDel,
            rgLinksDel + cNodesToDel,
            linkIbMergeStart,
            &cNodesMerged,
            &cNodesDeleted );

    // The skip list has been copied and reogranized
    // Dirty all pages to copy back to the original
    // WARNING: All local node pointers (e.g. in the merge, del sequences) are invalid after this point.
    // LOG the MergeAndDel operation
    {
        DbTimeGuard dbtimeGuard( m_pbbtbuff );
        dbtimeGuard.CoordinatedDirty( m_pTrxLogger->m_dbtimeCoordinated );
        DBTIME dbtimeBefore = dbtimeGuard.DbtimeBefore();
        DBTIME dbtimeCurr = dbtimeGuard.DbtimeNew();

        Call( m_pTrxLogger->ErrLogMergeAndDel(
            dbtimeBefore,
            dbtimeCurr,
            rgNodesMerge,
            rgLinksDel,
            cNodesMerged,
            cNodesDeleted,
            linkIbMergeStart ) );
        dbtimeGuard.Commit();
    }

    // WARNING: Can't fail after this point !!!
    *pcNodesMerged = cNodesMerged;
    *pcNodesDel = cNodesDeleted;

    // Copy root page (BBTBuffHeader + any data)
    memcpy(
            m_pbbtbuff->PbPage( 0 ) + m_pbbtbuff->IbHeader(),
            rgpbPage[ 0 ] + m_pbbtbuff->IbHeader(),
            m_pbbtbuff->PFormat()->cbBBTRoot );

    // if the last node fits perfectly at the end of the last page, ibMicFree can point to the next page
    int ipgLast = min( cMaxPages - 1, m_pbbtbuff->IpgOffsetFromLink( m_pbbtbuff->m_pHeader->le_ibMicFree ).ipg );
    for ( int i = 1; i <= ipgLast; i++ )
    {
        // Copy back the page at the appropriate offset
        int cbCopy = m_pbbtbuff->IbPageDataEnd( i ) - m_pbbtbuff->IbPageDataBegin( i );
        memcpy(
                m_pbbtbuff->PbPage( i ) + m_pbbtbuff->IbPageDataBegin( i ),
                rgpbPage[ i ] + m_pbbtbuff->IbPageDataBegin( i ),
                cbCopy );
    }

    // UA_TODO: pattern-fill leftover pages

    // Return a warning if we couldn't merge in all of the external nodes.
    if ( *pcNodesMerged < cNodesMerge )
    {
        err = ErrERRCheck( wrnBBTMergeTargetFull );
    }

HandleError:
    // Cleanup allocated memory
    if ( rgpbPage )
    {
        for ( int i = 0; i < cMaxPages; i++ )
        {
            BFFree( rgpbPage[ i ] );
        }
    }

    return err;
}

template <typename TLogger>
ERR BBTBuffWriter<TLogger>::ErrReorg_ProcessDeletes(
    _In_count_( cLinksDel ) SkipListLink*   rgLinksDel,
    const int                               cLinksDel,
    _Out_ SkipListLink*                     plinkIbMerge )
{
    ERR                 err = JET_errSuccess;
    BBTBuffHeader*       pHeader = m_pbbtbuff->m_pHeader;
    SkipListNode*       pnodeCurr;
    SkipListLink        linkDelCurr = ( cLinksDel > 0 ? rgLinksDel[ 0 ] : SkipListLink::Null() );
    SkipListLink        linkCurr = RgSkipListLinksHead( pHeader )[ 0 ];
    SkipListLink        ibCurr = SkipListLink::FromInt( sizeof( BBTBuffHeader ) );  // leave space for the header
    int                 cNodesLeft = 0;
    int                 iLinkDel = 0;

    Assert( latchWrite == m_pbbtbuff->m_pcsrBase->Latch() );

    while ( !linkCurr.FNull() )
    {
        pnodeCurr = m_pbbtbuff->PnodeFromLink( linkCurr );
        if ( linkCurr != linkDelCurr )
        {
            const int level = m_pbbtbuff->GenLevel( cNodesLeft ); // Re-level deterministically because we know the node count, and we are appending sequentially
            const int cb = SkipListNode::Cb( level, pnodeCurr->CbKey(), pnodeCurr->CbData() );
            const int cbUsed = SkipListLink::Roundup( cb );  // count wasted space too

            PageOffsetTuple pgOffsetCurr = m_pbbtbuff->IpgOffsetFromLink( ibCurr );
            const int cbLeft = ( pgOffsetCurr.ipg < m_pbbtbuff->Cpg() ? m_pbbtbuff->IbPageDataEnd( pgOffsetCurr.ipg ) - pgOffsetCurr.ibOnPage : 0 );
            if ( cbUsed > cbLeft )
            {
                pgOffsetCurr.ipg++;
                if ( pgOffsetCurr.ipg >= m_pbbtbuff->Cpg() )
                {
                    // We haven't added any extra nodes. But overflowed because of different skiplist leveling.
                    Call( ErrERRCheck( errBBTBuffFull ) );
                }

                pgOffsetCurr.ibOnPage = (USHORT) m_pbbtbuff->IbPageDataBegin( pgOffsetCurr.ipg );
                ibCurr = m_pbbtbuff->LinkFromIpgOffset( pgOffsetCurr.ipg, pgOffsetCurr.ibOnPage );
            }

            ibCurr.Inc( cbUsed );
            cNodesLeft++;
        }
        else
        {
            iLinkDel++;
            linkDelCurr = ( iLinkDel < cLinksDel ? rgLinksDel[ iLinkDel ] : SkipListLink::Null() );
        }

        linkCurr = pnodeCurr->LinkNext0();
    }

    Assert( linkDelCurr.FNull() ); // all deleted nodes should've been matched
    Assert( iLinkDel == cLinksDel );

    // When we reorg, node sizes may increase (because of different skiplist leveling).
    Assert( cNodesLeft == pHeader->le_cNodes - iLinkDel );
    *plinkIbMerge = ibCurr;

HandleError:
    return err;
}
