// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.




PERSISTED
const enum NodeRootField {
        noderfWhole         = 0,
        noderfSpaceHeader   = 1,
        noderfIsamAutoInc   = 2,
        noderfMax,
    };

C_ASSERT( noderfMax <= 5 );


ERR     ErrNDGet                    ( FUCB *pfucb, const CSR *pcsr );
VOID    NDGet                       ( FUCB *pfucb, const CSR *pcsr );
template< PageNodeBoundsChecking pgnbc = pgnbcNoChecks >
VOID    NDIGetKeydataflags          ( const CPAGE& cpage, INT iline, KEYDATAFLAGS * pkdf, _Out_opt_ ERR * perrNoEnforce = NULL );
ERR     ErrNDIGetKeydataflags       ( const CPAGE& cpage, INT iline, KEYDATAFLAGS * pkdf );


VOID    NDGetExternalHeader         ( _In_ KEYDATAFLAGS * const pkdf, _In_ const CSR * pcsr, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderf );
VOID    NDGetExternalHeader         ( _In_ FUCB *pfucb, _In_ const CSR *pcsr, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderf );
VOID    NDGetPtrExternalHeader      ( _In_ const CPAGE& cpage, _Out_ LINE * pline, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderf );
VOID    NDGetPrefix                 ( FUCB *pfucb, const CSR *pcsr );

VOID    NDGetBookmarkFromKDF    ( const FUCB *pfucb, const KEYDATAFLAGS& kdf, BOOKMARK *pbm );

ERR     ErrNDSeek               ( FUCB *pfucb, CSR *pcsr, const BOOKMARK& bookmark );
ERR     ErrNDSeekInternal       ( FUCB *pfucb, CSR *pcsr, const BOOKMARK& bookmark );

INT     IlineNDISeekGEQ(
            const CPAGE& cpage,
            const BOOKMARK& bm,
            const BOOL fUnique,
            INT * plastCompare );
INT     IlineNDISeekGEQInternal(
            const CPAGE& cpage,
            const BOOKMARK& bm,
            INT * plastCompare );

ERR     ErrNDInsert(
            FUCB * const pfucb,
            CSR * const pcsr,
            const KEYDATAFLAGS * const pkdf,
            const DIRFLAG fFlags,
            const RCEID rceid,
            const VERPROXY * const pverproxy);

ERR     ErrNDFlagInsert(
            FUCB * const pfucb,
            CSR * const pcsr,
            const DIRFLAG dirflag,
            const RCEID rceid,
            const VERPROXY * const pverproxy );

ERR     ErrNDFlagInsertAndReplaceData(
            FUCB * const pfucb,
            CSR * const pcsr,
            const KEYDATAFLAGS * const pkdf,
            const DIRFLAG dirflag,
            const RCEID rceidInsert,
            const RCEID rceidReplace,
            const RCE * const prceReplace,
            const VERPROXY * const pverproxy );

ERR     ErrNDReplace(
            FUCB * const pfucb,
            CSR * const pcsr,
            const DATA * const pdata,
            const DIRFLAG fFlags,
            const RCEID rceid,
            RCE * const prce );

template< typename TDelta >
ERR ErrNDDelta(
        FUCB            * const pfucb,
        CSR             * const pcsr,
        const INT       cbOffset,
        const TDelta    tDelta,
        TDelta          * const pOldValue,
        const DIRFLAG   dirflag,
        const RCEID     rceid );

template ERR ErrNDDelta<LONG>( FUCB * const pfucb, CSR * const pcsr, const INT cbOffset, const LONG tDelta, LONG * const pOldValue, const DIRFLAG   dirflag, const RCEID rceid );
template ERR ErrNDDelta<LONGLONG>( FUCB * const pfucb, CSR * const pcsr, const INT cbOffset, const LONGLONG tDelta, LONGLONG * const pOldValue, const DIRFLAG   dirflag, const RCEID rceid );

ERR     ErrNDFlagDelete         (
            FUCB * const pfucb,
            CSR * const pcsr,
            const DIRFLAG fFlags,
            const RCEID rceid,
            const VERPROXY * const pverproxy );
VOID    NDResetFlagDelete       ( CSR *pcsr );
ERR     ErrNDFlagVersion        ( CSR *pcsr );
VOID    NDDeferResetNodeVersion ( CSR *pcsr );


VOID    NDResetVersionInfo  ( CPAGE * const pcpage );
bool    FNDAnyNodeIsVersioned( const CPAGE& cpage );
bool    FNDAnyNodeIsCompressed( const CPAGE& cpage );

ERR     ErrNDDelete             ( FUCB *pfucb, CSR *pcsr, const DIRFLAG dirflag);
ERR     ErrNDSetExternalHeader  ( _In_ FUCB* const pfucb, _In_ CSR* const pcsr, _In_ const DATA *pdata, _In_ const DIRFLAG fFlags, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderf );
VOID    NDSetPrefix             ( CSR *pcsr, const KEY& key );
VOID    NDSetExternalHeader     ( _In_ FUCB* const pfucb, _In_ CSR* const pcsr, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderf, _In_ const DATA * pdata );
VOID    NDSetExternalHeader     ( _In_ const IFMP ifmp, _In_ CSR* const pcsr, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderf, _In_ const DATA * pdata );

const SCRUBOPER scrubOperNone               = 0x0;
const SCRUBOPER scrubOperScrubData          = 0x1;
const SCRUBOPER scrubOperScrubLVData        = 0x2;

ERR ErrNDScrubOneUsedPage(
        __in PIB * const ppib,
        const IFMP ifmp,
        __in CSR * const pcsr,
        __in_ecount(cscrubOper) const SCRUBOPER * const rgscrubOper,
        const INT cscrubOper,
        const DIRFLAG dirflag);
ERR ErrNDScrubOneUnusedPage(
        __in PIB * const pfucb,
        const IFMP ifmp,
        __in CSR * const pcsr,
        const DIRFLAG dirflag);

VOID    NDMoveFirstSon          ( FUCB *pfucb, CSR *pcsr );
VOID    NDMoveLastSon           ( FUCB *pfucb, CSR *pcsr );
ERR     ErrNDVisibleToCursor    ( FUCB *pfucb, BOOL * pfVisible, NS * pns );
BOOL    FNDPotVisibleToCursor   ( const FUCB *pfucb, CSR *pcsr );

VOID    NDBulkDelete            ( CSR *pcsr, INT clines );

VOID    NDInsert                ( FUCB *pfucb, CSR *pcsr, const KEYDATAFLAGS *pkdf );
VOID    NDInsert                ( FUCB *pfucb, CSR *pcsr, const KEYDATAFLAGS *pkdf, INT cbPrefix );
VOID    NDReplace               ( CSR *pcsr, const DATA *pdata );
VOID    NDDelete                ( CSR *pcsr );
VOID    NDGrowCbPrefix          ( const FUCB *pfucb, CSR *pcsr, INT cbPrefixNew );
VOID    NDShrinkCbPrefix        ( FUCB *pfucb, CSR *pcsr, const DATA *pdataOldPrefix, INT cbPrefixNew );

VOID NDReplaceForUpgrade(
    CPAGE * const pcpage,
    const INT iline,
    const DATA * const pdata,
    const KEYDATAFLAGS& kdfOld );

INT     CbNDUncommittedFree     ( const FUCB * const pfucb, const CSR * const pcsr );



#ifdef DEBUG
VOID NDAssertNDInOrder          ( const FUCB * pfucb, const CSR * pcsr );
BOOL FNDCorruptRandomNodeElement( CPAGE * const pcpage );
void NDCorruptNodePrefixSize( const CPAGE& cpage, const INT iline, const JET_GRBIT grbitNodeCorruption, const USHORT usOffsetFixed = 0 );
void NDCorruptNodeSuffixSize( const CPAGE& cpage, const INT iline, const JET_GRBIT grbitNodeCorruption, const USHORT usOffsetFixed = 0 );
#else
#define NDAssertNDInOrder(foo,bar) ((void)0)
#endif



const USHORT usNDCbKeyMask = 0x1fff;

const INT fNDVersion    = 0x01;
const INT fNDDeleted    = 0x02;
const INT fNDCompressed = 0x04;

const ULONG cbNDNullKeyData                   = cbKeyCount + CPAGE::cbInsertionOverhead;
#define CbNDNodeMost( cbPage )                ( CPAGE::CbPageDataMax( cbPage )  )
#define CbNDPageAvailMostNoInsert( cbPage )   ( CPAGE::CbPageDataMaxNoInsert( cbPage ) )





#ifdef CHECK_UNIQUE_KEY_ON_NONUNIQUE_INDEX
INLINE INT CmpNDKeyOfNextNode( const CSR * const pcsr, const KEY& key )
{
    Assert( pcsr->ILine() < pcsr->Cpage().Clines() - 1 );

    KEYDATAFLAGS    kdf;
    NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine() + 1, &kdf );
    return CmpKey( kdf.key, key );
}
#endif

INLINE BOOL FNDVersion( const KEYDATAFLAGS& keydataflags )
{
    return keydataflags.fFlags & fNDVersion;
}

INLINE BOOL FNDPossiblyVersioned( const FUCB * const pfucb, const CSR * const pcsr )
{
    return ( pcsr->FPageRecentlyDirtied( pfucb->ifmp )
            && FNDVersion( pfucb->kdfCurr ) );
}

INLINE BOOL FNDCompressed( const KEYDATAFLAGS& keydataflags )
{
    return keydataflags.fFlags & fNDCompressed;
}


INLINE BOOL FNDDeleted( const KEYDATAFLAGS& keydataflags )
{
    return keydataflags.fFlags & fNDDeleted;
}

INLINE VOID NDGetBookmarkFromKDF( const KEYDATAFLAGS& kdf, BOOKMARK *pbm, const BOOL fUnique )
{
    pbm->key = kdf.key;
    if ( fUnique )
    {
        pbm->data.Nullify();
    }
    else
    {
        pbm->data = kdf.data;
    }
}

INLINE VOID NDGetBookmarkFromKDF( const FUCB *pfucb, const KEYDATAFLAGS& kdf, BOOKMARK *pbm )
{
    NDGetBookmarkFromKDF( kdf, pbm, FFUCBUnique( pfucb ) );
}


INLINE BOOL FNDFreePageSpace( const FUCB *pfucb, CSR *pcsr, const ULONG cbReq, const BOOL fPermitUpdateUncFree = fTrue )
{
    return ( pcsr->Cpage().CbPageFree() < cbReq ?
                    fFalse :
                    FVERCheckUncommittedFreedSpace( pfucb, pcsr, cbReq, fPermitUpdateUncFree ) );
}

INLINE ULONG CbNDReservedSizeOfNode( FUCB *pfucb, const CSR * const pcsr )
{

    if ( FNDPossiblyVersioned( pfucb, pcsr )
        || ( PinstFromIfmp( pfucb->ifmp )->FRecovering() && !FFUCBSpace( pfucb ) ) )
    {
        BOOKMARK    bm;

        NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
        return CbVERGetNodeReserve( pfucb->ppib,
                                    pfucb,
                                    bm,
                                    pfucb->kdfCurr.data.Cb() );
    }
    else
    {
        return 0;
    }
}


INLINE ULONG CbNDNodeSizeTotal( const KEYDATAFLAGS &kdf )
{

    const ULONG cbSize = kdf.key.Cb() + kdf.data.Cb() + cbNDNullKeyData;
    return cbSize;
}


INLINE ULONG CbNDNodeSizeCompressed( const KEYDATAFLAGS &kdf )
{
    ULONG   cbSize = kdf.key.suffix.Cb() + kdf.data.Cb() + cbNDNullKeyData;
    
    if ( kdf.key.prefix.Cb() > 0 )
    {
        Assert( kdf.key.prefix.Cb() > cbPrefixOverhead );
        Assert( FNDCompressed( kdf ) );

        cbSize += cbPrefixOverhead;
    }

    Assert( CbNDNodeSizeTotal( kdf ) >= cbSize );
    
    return cbSize;
}





INLINE VOID NDGet( FUCB *pfucb )
{
    NDGet( pfucb, Pcsr( pfucb ) );
}


INLINE VOID NDGetExternalHeader( _In_ FUCB* const pfucb, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderf )
{
    NDGetExternalHeader( pfucb, Pcsr( pfucb ), noderf );
}


INLINE VOID NDGetPrefix( FUCB *pfucb )
{
    NDGetPrefix( pfucb, Pcsr( pfucb ) );
}


INLINE ERR ErrNDSeek( FUCB *pfucb, const BOOKMARK& bookmark )
{
    return ErrNDSeek( pfucb, Pcsr( pfucb ), bookmark );
}


INLINE ERR ErrNDSeekInternal( FUCB *pfucb, const BOOKMARK& bookmark )
{
    return ErrNDSeekInternal( pfucb, Pcsr( pfucb ), bookmark );
}


INLINE ERR ErrNDInsert(
                FUCB * const pfucb,
                const KEYDATAFLAGS * const pkdf,
                const DIRFLAG fFlags,
                const RCEID rceid,
                const VERPROXY *pverproxy )
{
    return ErrNDInsert( pfucb, Pcsr( pfucb ), pkdf, fFlags, rceid, pverproxy );
}


INLINE ERR ErrNDFlagInsert(
            FUCB * const pfucb,
            const DIRFLAG dirflag,
            const RCEID rceid,
            const VERPROXY * const pverproxy )
{
    return ErrNDFlagInsert( pfucb, Pcsr( pfucb ), dirflag, rceid, pverproxy );
}


INLINE ERR ErrNDFlagInsertAndReplaceData(
            FUCB * const pfucb,
            const KEYDATAFLAGS * const pkdf,
            const DIRFLAG dirflag,
            const RCEID rceidInsert,
            const RCEID rceidReplace,
            const RCE * const prceReplace,
            const VERPROXY * const pverproxy )
{
    return ErrNDFlagInsertAndReplaceData(
                pfucb,
                Pcsr( pfucb ),
                pkdf,
                dirflag,
                rceidInsert,
                rceidReplace,
                prceReplace,
                pverproxy );
}


INLINE ERR ErrNDReplace(
            FUCB * const pfucb,
            const DATA * const pdata,
            const DIRFLAG fFlags,
            const RCEID rceid,
            RCE * const prce )
{
    return ErrNDReplace( pfucb, Pcsr( pfucb ), pdata, fFlags, rceid, prce );
}


INLINE ERR ErrNDFlagDelete(
        FUCB * const pfucb,
        const DIRFLAG fFlags,
        const RCEID rceid,
        const VERPROXY * const pverproxy )
{
    return ErrNDFlagDelete( pfucb, Pcsr( pfucb ), fFlags, rceid, pverproxy );
}
    

INLINE ERR ErrNDDelete( FUCB * const pfucb, const DIRFLAG dirflag )
{
    return ErrNDDelete( pfucb, Pcsr( pfucb ), dirflag );
}


INLINE ERR ErrNDSetExternalHeader( _In_ FUCB * const pfucb, _In_ const DATA * const pdata, _In_ const DIRFLAG fFlags, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderf )
{
    return ErrNDSetExternalHeader( pfucb, Pcsr( pfucb ), pdata, fFlags, noderf );
}

#ifdef DEBUG
INLINE BOOL FNDIIsExternalHeaderUpgradable( _In_ const CPAGE& page )
{
    if ( !page.FRootPage() || page.FSpaceTree() )
    {
        return fFalse;
    }
    return fTrue;
}
#endif

INLINE VOID NDMoveFirstSon( FUCB * const pfucb )
{
    NDMoveFirstSon( pfucb, Pcsr( pfucb ) );
}


INLINE VOID NDMoveLastSon( FUCB * const pfucb )
{
    NDMoveLastSon( pfucb, Pcsr( pfucb ) );
}


INLINE BOOL FNDPotVisibleToCursor( const FUCB * const pfucb )
{
    return FNDPotVisibleToCursor( pfucb, Pcsr( const_cast<FUCB *>( pfucb ) ) );
}


INLINE VOID NDSetEmptyTree( CSR * const pcsrRoot )
{
    Assert( latchWrite == pcsrRoot->Latch() );
    Assert( pcsrRoot->Cpage().FRootPage() );
    Assert( !pcsrRoot->Cpage().FLeafPage() );
    Assert( 1 == pcsrRoot->Cpage().Clines() );

    pcsrRoot->SetILine( 0 );
    NDDelete( pcsrRoot );

    const ULONG     ulFlags     = ( pcsrRoot->Cpage().FFlags() & ~( CPAGE::fPageParentOfLeaf ) );
    pcsrRoot->Cpage().SetFlags( ulFlags | CPAGE::fPageLeaf );
}

INLINE VOID NDEmptyPage( CSR * const pcsr )
{
    KEY keyNull;
    keyNull.Nullify();
    NDSetPrefix( pcsr, keyNull );
    
    pcsr->SetILine( 0 );
    NDBulkDelete( pcsr, pcsr->Cpage().Clines() );
    
    pcsr->Cpage().SetFEmpty();

    
}

