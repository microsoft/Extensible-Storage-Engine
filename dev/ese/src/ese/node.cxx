// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

/*******************************************************************

Operations that modify a node are done in three stages:

1. Create a 'tentative' version (before image) for the record
1. Dirty the page -- this will increase the timestamp
2. Log the operation.
3. If the log operation succeeded:
    3.1 Update the page. Set its lgpos to the position we got
    from the log operation. As the page is write latched
    we do not need to worry about the page being flushed yet

The item that a currency references must be there

*******************************************************************/


#include "std.hxx"

#include "PageSizeClean.hxx"


//  ****************************************************************
//  MACROS
//  ****************************************************************

#ifdef DEBUG

//  check that the node is in order at each entry point
///#define DEBUG_NODE

#endif  // DEBUG


//  ****************************************************************
//  CONSTANTS
//  ****************************************************************

//  the number of DATA structs that a KEYDATAFLAGS is converted to
const ULONG cdataKDF    = 4;

//  the number of DATA structs that a prefix becomes
const ULONG cdataPrefix = 3;

//  external header size array for different type of data stored in external header.
USHORT g_rgcbExternalHeaderSize[] = {
                    0,                      // Real NodeRootField value in NodeRootField enum starts from 1, put a dummy here to make the offset consistent
                    sizeof(SPACE_HEADER),   // space header
                    sizeof(QWORD)           // auto inc
                };

C_ASSERT( _countof( g_rgcbExternalHeaderSize ) == noderfMax );

//  ****************************************************************
//  PROTOTYPES
//  ****************************************************************

LOCAL_BROKEN INT CdataNDIPrefixAndKeydataflagsToDataflags (
    KEYDATAFLAGS    * const pkdf,
    DATA            * const rgdata,
    INT             * const pfFlags,
    LE_KEYLEN       * const ple_keylen );
LOCAL_BROKEN INT CdataNDIKeydataflagsToDataflags (
    const KEYDATAFLAGS  * const pkdf,
    DATA                * const rgdata,
    INT                 * const pfFlags,
    LE_KEYLEN           * const ple_keylen );
INLINE VOID NDISetCompressed            ( KEYDATAFLAGS& kdf );
INLINE VOID NDIResetCompressed          ( KEYDATAFLAGS& kdf );
INLINE VOID NDISetFlag                  ( CSR * pcsr, INT fFlag );
INLINE VOID NDIResetFlag                ( CSR * pcsr, INT fFlag );

INLINE VOID NDIGetBookmark              ( const CPAGE& cpage, INT iline, BOOKMARK * pbookmark, const BOOL fUnique );
INLINE ERR ErrNDISeekInternalPage       ( const FUCB * pfucb, CSR * pcsr, const BOOKMARK& bm );
INLINE ERR ErrNDISeekLeafPage           ( const FUCB * pfucb, CSR * pcsr, const BOOKMARK& bm );

#ifdef DEBUG
INLINE VOID NDIAssertCurrency           ( const CSR * pcsr );
INLINE VOID NDIAssertCurrency           ( const FUCB * pfucb, const CSR * pcsr );
INLINE VOID NDIAssertCurrencyExists     ( const FUCB * pfucb, const CSR * pcsr );
INLINE VOID NDIAssertCurrencyExists     ( const CSR * pcsr );

LOCAL_BROKEN INT IlineNDISeekGEQDebug           ( const CPAGE& cpage, const BOOKMARK& bm, const BOOL fUnique, INT * plastCompare );
LOCAL_BROKEN INT IlineNDISeekGEQInternalDebug   ( const CPAGE& cpage, const BOOKMARK& bm, INT * plastCompare );

#else
INLINE VOID NDIAssertCurrency           ( const CSR * pcsr ) {}
INLINE VOID NDIAssertCurrency           ( const FUCB * pfucb, const CSR * pcsr ) {}
INLINE VOID NDIAssertCurrencyExists     ( const FUCB * pfucb, const CSR * pcsr ) {}
INLINE VOID NDIAssertCurrencyExists     ( const CSR * pcsr ) {}
#endif  //  DEBUG


//  ****************************************************************
//  INTERNAL ROUTINES
//  ****************************************************************

#ifndef DEBUG
BOOL g_fNodeMiscMemoryTrashedDefenseInDepthTemp = fTrue;    // covers bsearch key usage on leaf pages (done off computed post-results - so less "knowably correct")
#endif

LOCAL ERR ErrNDIReportBadLineCount(
    FUCB        * const pfucb,
    const ERR   err,
    const CSR *pcsr )
{
    const WCHAR * rgsz[6];
    WCHAR       rgszDw[5][16];

    OSStrCbFormatW( rgszDw[0], sizeof(rgszDw[0]), L"%u", pcsr->Pgno() );
    OSStrCbFormatW( rgszDw[1], sizeof(rgszDw[1]), L"%u", ObjidFDP( pfucb ) );
    OSStrCbFormatW( rgszDw[2], sizeof(rgszDw[2]), L"%u", PgnoFDP( pfucb ) );
    OSStrCbFormatW( rgszDw[3], sizeof(rgszDw[3]), L"%d", pcsr->ILine() );
    OSStrCbFormatW( rgszDw[4], sizeof(rgszDw[4]), L"%u", pcsr->Cpage().Clines() );

    rgsz[0] = g_rgfmp[pfucb->u.pfcb->Ifmp()].WszDatabaseName();
    rgsz[1] = rgszDw[0];
    rgsz[2] = rgszDw[1];
    rgsz[3] = rgszDw[2];
    rgsz[4] = rgszDw[3];
    rgsz[5] = rgszDw[4];

    UtilReportEvent(
            eventError,
            DATABASE_CORRUPTION_CATEGORY,
            BAD_LINE_COUNT_ID,
            _countof( rgsz ),
            rgsz,
            0,
            NULL,
            PinstFromPfucb( pfucb ) );

    FireWall( "InconsistentLineRequestedVsLineCount" );

    return err;
}

//  ================================================================
template< PageNodeBoundsChecking pgnbc >
LOCAL VOID NDILineToKeydataflags( const CPAGE& cpage, const LINE * pline, KEYDATAFLAGS * pkdf, _Out_opt_ ERR * perrNoEnforce )
//  ================================================================
{
    const BYTE  *pb             = (BYTE*)pline->pv;
    INT         cbKeyCountTotal = cbKeyCount;

    Expected( ( pgnbc == pgnbcChecked ) == ( perrNoEnforce != NULL ) );
    Assert( ( pgnbc == pgnbcChecked ) || ( perrNoEnforce == NULL ) ); // this wouldn't make sense.

    Assert( perrNoEnforce == NULL || *perrNoEnforce == JET_errSuccess || pgnbc == pgnbcNoChecks ); // only updated on errors

    typedef UnalignedLittleEndian< USHORT >* PULEUS;

    if ( pline->fFlags & fNDCompressed )
    {
        //  prefix key compressed
        pkdf->key.prefix.SetCb( *( PULEUS )pb & usNDCbKeyMask );
        pb += cbPrefixOverhead;
        cbKeyCountTotal += cbPrefixOverhead;
    }
    else
    {
        //  no prefix compression
        pkdf->key.prefix.SetCb( 0 );
    }

    const USHORT cbSuffix = *( PULEUS )pb & usNDCbKeyMask;

    pkdf->key.suffix.SetCb( cbSuffix );
    pkdf->key.suffix.SetPv( const_cast<BYTE *>( pb ) + cbKeyCount );

    //  In theory we would never create such a situation, so if this happens it is either
    //  an in-memory corruption, or ESE bug.  Either way we're hosed if we continue.


    PageAssertTrack( cpage, pline->cb > (ULONG)cbKeyCountTotal || FNegTest( fCorruptingPageLogically ), "LineNotLongEnoughForKeyCounts" );
#ifdef DEBUG
    if ( pline->cb <= (ULONG)cbKeyCountTotal )
    {
        if ( perrNoEnforce == NULL )
        {
            //  client doesn't know / have path to handle error yet.
            PageEnforce( cpage, cbSuffix < pline->cb );
        }
        else
        {
            *perrNoEnforce = ErrERRCheck( JET_errNodeCorrupted );
        }
    }
#endif

    if constexpr( pgnbc == pgnbcChecked ) 
    {
        PageAssertTrack( cpage, cbSuffix <= ( pline->cb - cbKeyCountTotal ) || FNegTest( fCorruptingPageLogically ), "SuffixNotContainedInLine" );
        PageAssertTrack( cpage, cbSuffix < ( pline->cb - cbKeyCountTotal ) || FNegTest( fCorruptingPageLogically ), "SuffixLeavesRoomForZeroData" );
#ifdef DEBUG
        if ( cbSuffix >= ( pline->cb - cbKeyCountTotal ) ) //  Maybe > someday, but all nodes have some data for now (so suffix must be < line->cb).
#else
        if ( cbSuffix >= pline->cb ) //  Maybe > someday, but all nodes have some data for now (so suffix must be < line->cb).
#endif
        {
            if ( perrNoEnforce == NULL )
            {
                //  client doesn't know / have path to handle error yet.
                PageEnforce( cpage, cbSuffix < pline->cb );
#ifdef DEBUG
                PageEnforce( cpage, cbSuffix < ( pline->cb - cbKeyCountTotal ) );
#endif
            }
            else
            {
                *perrNoEnforce = ErrERRCheck( JET_errNodeCorrupted );
            }
        }
    }

#ifdef DEBUG
    // Should have solved the negative data.Cb() or Enforced (above) or have an err to show for it.
    PageAssertTrack( cpage, ( ( pline->cb - pkdf->key.suffix.Cb() - cbKeyCountTotal ) > 0 ) || ( perrNoEnforce && *perrNoEnforce < JET_errSuccess ), "KeySizesCheckVsErrStateInconsistent" );
#endif

    pkdf->data.SetCb( pline->cb - pkdf->key.suffix.Cb() - cbKeyCountTotal );
    pkdf->data.SetPv( const_cast<BYTE *>( pb ) + cbKeyCount + pkdf->key.suffix.Cb() );

    pkdf->fFlags    = pline->fFlags;

    Assert( ULONG( pkdf->key.suffix.Cb() + pkdf->data.Cb() + cbKeyCountTotal ) == pline->cb );

    //  kdf can not yet be validated since prefix.pv is not yet set
}


//  ================================================================
LOCAL_BROKEN INT CdataNDIPrefixAndKeydataflagsToDataflags(
    KEYDATAFLAGS    * const pkdf,
    DATA            * const rgdata,
    INT             * const pfFlags,
    LE_KEYLEN       * const ple_keylen )
//  ================================================================
//
//  sets up rgdata to insert kdf with *pcbPrefix as prefix.cb
//  adjust kdf.suffix.cb to new suffix.cb
//  rgdata must have at least cdataKDF + 1 elements
//
//-
{
    ASSERT_VALID( pkdf );

    INT idata = 0;

    Assert( ple_keylen->le_cbPrefix >= 0 );
    if ( ple_keylen->le_cbPrefix > 0 )
    {
        //  prefix compresses
        Assert( ple_keylen->le_cbPrefix > cbPrefixOverhead );
        Assert( sizeof(ple_keylen->le_cbPrefix) == cbPrefixOverhead );

        rgdata[idata].SetPv( &ple_keylen->le_cbPrefix );
        rgdata[idata].SetCb( cbPrefixOverhead );
        ++idata;
    }

    //  suffix count will be updated correctly below
    rgdata[idata].SetPv( &ple_keylen->le_cbSuffix );
    rgdata[idata].SetCb( cbKeyCount );
    ++idata;

    //  leave ple_keylen->le_cbPrefix out of given key to get suffix to insert
    Assert( pkdf->key.Cb() >= ple_keylen->le_cbPrefix );
    if ( pkdf->key.prefix.Cb() <= ple_keylen->le_cbPrefix )
    {
        //  get suffix to insert from pkdf->suffix
        const INT   cbPrefixInSuffix = ple_keylen->le_cbPrefix - pkdf->key.prefix.Cb();

        rgdata[idata].SetPv( (BYTE *) pkdf->key.suffix.Pv() + cbPrefixInSuffix );
        rgdata[idata].SetCb( pkdf->key.suffix.Cb() - cbPrefixInSuffix );
        idata++;

        //  decrease size of suffix
        pkdf->key.suffix.DeltaCb( - cbPrefixInSuffix );
    }
    else
    {
        //  get suffix to insert from pkdf->prefix and suffix
        rgdata[idata].SetPv( (BYTE *)pkdf->key.prefix.Pv() + ple_keylen->le_cbPrefix );
        rgdata[idata].SetCb( pkdf->key.prefix.Cb() - ple_keylen->le_cbPrefix );
        ++idata;

        rgdata[idata] = pkdf->key.suffix;
        ++idata;

        //  decrease size of suffix
        pkdf->key.suffix.DeltaCb( pkdf->key.prefix.Cb() - ple_keylen->le_cbPrefix );
    }

    //  update suffix count in output buffer
    ple_keylen->le_cbSuffix = (USHORT)pkdf->key.suffix.Cb();

    //  get data of kdf
    rgdata[idata] = pkdf->data;
    ++idata;

    *pfFlags = pkdf->fFlags;
    if ( ple_keylen->le_cbPrefix > 0 )
    {
        *pfFlags |= fNDCompressed;
    }
    else
    {
        *pfFlags &= ~fNDCompressed;
    }

    return idata;
}


//  ================================================================
LOCAL_BROKEN INT CdataNDIKeydataflagsToDataflags(
    const KEYDATAFLAGS  * const pkdf,
    DATA                * const rgdata,
    INT                 * const pfFlags,
    LE_KEYLEN           * const ple_keylen )
//  ================================================================
//
//  rgdata must have at least cdataKDF elements
//
//-
{
    ASSERT_VALID( pkdf );

    INT idata = 0;

    if ( !pkdf->key.prefix.FNull() )
    {
        //  prefix compresses
        Assert( pkdf->key.prefix.Cb() > cbPrefixOverhead );
        Assert( FNDCompressed( *pkdf ) );

        ple_keylen->le_cbPrefix = USHORT( pkdf->key.prefix.Cb() );
        rgdata[idata].SetPv( &ple_keylen->le_cbPrefix );
        rgdata[idata].SetCb( cbPrefixOverhead );
        ++idata;
    }
    else
    {
        Assert( !FNDCompressed( *pkdf ) );
    }

    ple_keylen->le_cbSuffix = USHORT( pkdf->key.suffix.Cb() );
    rgdata[idata].SetPv( &ple_keylen->le_cbSuffix );
    rgdata[idata].SetCb( cbKeyCount );
    ++idata;

    rgdata[idata] = pkdf->key.suffix;
    ++idata;

    rgdata[idata] = pkdf->data;
    ++idata;

    *pfFlags = pkdf->fFlags;

    return idata;
}

CCondition g_condGetKdfBadGetPtrNullPv;
CCondition g_condGetKdfBadGetPtrNonNullPv;
CCondition g_condGetKdfBadGetPtrOffPage;
CCondition g_condGetKdfBadGetPtrExtHdrOffPage;

#pragma warning(push)
#pragma warning(disable: 4101)     // The compiler is smart enough to recognize errThrowAway is unused variable, but not smart enough to realize it is NOT always unused (pending upon the template)
//  ================================================================
template< PageNodeBoundsChecking pgnbc >
VOID NDIGetKeydataflags( const CPAGE& cpage, INT iline, KEYDATAFLAGS * pkdf, _Out_opt_ ERR * perrNoEnforce )
//  ================================================================
{
    LINE line;
    ERR errThrowAway;  // for subsequent errors past first
    OnDebug( const BOOL fNoEnforceCheck = ( perrNoEnforce != NULL ) );

    Expected( ( pgnbc == pgnbcChecked ) == ( perrNoEnforce != NULL ) );
    Assert( ( pgnbc == pgnbcChecked ) || ( perrNoEnforce == NULL ) ); // this wouldn't make sense.

    Assert( perrNoEnforce == NULL || *perrNoEnforce == JET_errSuccess ); // only updated on errors
    cpage.GetPtr< pgnbc >( iline, &line, perrNoEnforce );
    Assert( line.pv != NULL || ( perrNoEnforce && *perrNoEnforce < JET_errSuccess ) );
    if constexpr( pgnbc == pgnbcChecked ) 
    {
    if ( perrNoEnforce && *perrNoEnforce < JET_errSuccess )
        {
            OnDebug( g_condGetKdfBadGetPtrNullPv.Hit( line.pv == NULL ) );
            OnDebug( g_condGetKdfBadGetPtrNonNullPv.Hit( line.pv != NULL ) );

            if ( line.pv == NULL )
            {
                AssertTrack( perrNoEnforce == NULL || *perrNoEnforce < JET_errSuccess, "GetPtrLineNullPvErrInconsistent" );
                pkdf->Nullify();
                return;
            }

            //  Check bare minimum can be defererenced, to continue KDF computations.

            const BOOL fKdfCbOnPage = cpage.FOnPage( line.pv, cbKeyCount + ( ( line.fFlags & fNDCompressed ) ? cbPrefixOverhead : 0 ) );
            if ( !fKdfCbOnPage )
            {
                OnDebug( g_condGetKdfBadGetPtrOffPage.Hit( line.pv != NULL ) );
                PageAssertTrack( cpage, FNegTest( fCorruptingPageLogically ), "NodeKeyCbPrefixOrSuffixOffPage" );

                pkdf->Nullify();
                return;
            }

            //  We already have an error and we want to return the first error, so we preserve it by mapping
            //  a throw away error var.
            errThrowAway = JET_errSuccess;
            perrNoEnforce = &errThrowAway;
        }
    }
    Assert( line.pv != NULL );

    NDILineToKeydataflags< pgnbc >( cpage, &line, pkdf, perrNoEnforce );
    if constexpr( pgnbc == pgnbcChecked ) 
    {
    if ( perrNoEnforce && *perrNoEnforce < JET_errSuccess )
        {
            errThrowAway = JET_errSuccess;
            perrNoEnforce = &errThrowAway;
        }
    }

    if( FNDCompressed( *pkdf ) )
    {
        cpage.GetPtrExternalHeader< pgnbc >( &line, perrNoEnforce );
        Assert( line.pv != NULL || ( perrNoEnforce && *perrNoEnforce < JET_errSuccess ) );
        if constexpr( pgnbc == pgnbcChecked )
        {
            if ( perrNoEnforce && *perrNoEnforce < JET_errSuccess )
            {
                OnDebug( g_condGetKdfBadGetPtrExtHdrOffPage.Hit( line.pv == NULL ) );

                if ( line.pv == NULL )
                {
                    AssertTrack( perrNoEnforce == NULL || *perrNoEnforce < JET_errSuccess, "GetPtrExtHdrLineNullPvErrInconsistent" );
                    pkdf->key.prefix.Nullify();
                    return;
                }

                errThrowAway = JET_errSuccess;
                perrNoEnforce = &errThrowAway;
            }
        }

        Assert( line.pv != NULL );

        if ( !cpage.FRootPage() )
        {
            pkdf->key.prefix.SetPv( (BYTE *)line.pv );
        }
        else
        {
            if ( cpage.FSpaceTree() )
            {
                pkdf->key.prefix.SetPv( (BYTE *)line.pv + sizeof( SPLIT_BUFFER ) );
            }
            else
            {
                pkdf->key.prefix.SetPv( NULL );
                pkdf->key.prefix.SetCb( 0 );
            }
        }

        //  The plain key.prefix.Cb() > line.cb goes off during BT split where the line.cb is 0 before the
        //  code fixes up the splitted page.  So we check that line.cb is not zero, to detect the page being
        //  in this state.
        if constexpr( pgnbc == pgnbcChecked ) 
        {
            if ( (ULONG)pkdf->key.prefix.Cb() > line.cb && line.cb != 0 )
            {
                if ( perrNoEnforce != NULL )
                {
                    if ( *perrNoEnforce >= JET_errSuccess )
                    {
                        *perrNoEnforce = ErrERRCheck( JET_errNodeCorrupted );

                        errThrowAway = JET_errSuccess;
                        perrNoEnforce = &errThrowAway;
                    }
                }
                else
                {
                    //  client doesn't know / have path to handle error yet.
                    PageEnforce( cpage, (ULONG)pkdf->key.prefix.Cb() <= line.cb || line.cb == 0 );
                }
            }
        }
    }
    else
    {
        Assert( 0 == pkdf->key.prefix.Cb() );
        pkdf->key.prefix.SetPv( NULL );
    }

    //  If we started with no enforce, we should end in same mode or someone updated perrNoEnforce = &errThrowAway without 
    //  checking perrNoEnforce wasn't NULL from before / i.e. passed into the function.  This would hide / disable PageEnforce()s
    //  when the caller does not handle errors.
    Assert( fNoEnforceCheck == ( perrNoEnforce != NULL ) );

    ASSERT_VALID( pkdf );
}
#pragma warning(pop)


ERR ErrNDIGetKeydataflags( const CPAGE& cpage, INT iline, KEYDATAFLAGS * pkdf )
{
    ERR errRet = JET_errSuccess;
    NDIGetKeydataflags< pgnbcChecked >( cpage, iline, pkdf, &errRet );
    return errRet;
}


//  ================================================================
INLINE VOID NDIGetBookmark( const CPAGE& cpage, INT iline, BOOKMARK * pbookmark, const BOOL fUnique )
//  ================================================================
//
//  returns the correct bookmark for the node. this depends on wether the index is
//  unique or not. may need the FUCB to determine if the index is unique
//
//-
{
    KEYDATAFLAGS keydataflags;
    NDIGetKeydataflags( cpage, iline, &keydataflags );

    NDGetBookmarkFromKDF( keydataflags, pbookmark, fUnique );

    ASSERT_VALID( pbookmark );
}


//  ================================================================
INLINE VOID NDISetCompressed( KEYDATAFLAGS& kdf )
//  ================================================================
{
    kdf.fFlags |= fNDCompressed;
#ifdef DEBUG_NODE
    Assert( FNDCompressed( kdf ) );
#endif  //  DEBUG_NODE
}


//  ================================================================
INLINE VOID NDIResetCompressed( KEYDATAFLAGS& kdf )
//  ================================================================
{
    kdf.fFlags &= ~fNDCompressed;
#ifdef DEBUG_NODE
    Assert( !FNDCompressed( kdf ) );
#endif  //  DEBUG_NODE
}


//  ================================================================
INLINE VOID NDISetFlag( CSR * pcsr, INT fFlag )
//  ================================================================
{
    KEYDATAFLAGS kdf;
    NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdf );
    pcsr->Cpage().ReplaceFlags( pcsr->ILine(), kdf.fFlags | fFlag );

#ifdef DEBUG_NODE
    NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdf );
    Assert( kdf.fFlags & fFlag );   //  assert the flag is set
#endif  //  DEBUG_NODE
}


//  ================================================================
INLINE VOID NDIResetFlag( CSR * pcsr, INT fFlag )
//  ================================================================
{
    Assert( pcsr );

    KEYDATAFLAGS kdf;
    NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdf );
/// Assert( kdf.fFlags & fFlag );   //  unsetting a flag that is not set?!
    pcsr->Cpage().ReplaceFlags( pcsr->ILine(), kdf.fFlags & ~fFlag );

#ifdef DEBUG_NODE
    NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdf );
    Assert( !( kdf.fFlags & fFlag ) );
#endif  //  DEBUG_NODE
}


//  ================================================================
INT IlineNDISeekGEQ( const CPAGE& cpage, const BOOKMARK& bm, const BOOL fUnique, INT * plastCompare )
//  ================================================================
//
//  finds the first item greater or equal to the
//  given bookmark, returning the result of the last comparison
//
//-
{
    Assert( cpage.FLeafPage() );

    const INT       clines      = cpage.Clines( );
    Assert( clines > 0 );   // will not work on an empty page
    INT             ilineFirst  = 0;
    INT             ilineLast   = clines - 1;
    INT             ilineMid    = 0;
    INT             iline       = -1;
    INT             compare     = 0;
    BOOKMARK        bmNode;

    while( ilineFirst <= ilineLast )
    {
        ilineMid = (ilineFirst + ilineLast)/2;

        //  We used to call:
        //    NDIGetBookmark( cpage, ilineMid, &bmNode, fUnique );
        //  But we need the kdf (or better would be line) as well to do some basic data validation
        //  checks, so we inlined it so we could get at the kdf.  This code is a copy of 
        //  NDIGetBookmark() for efficiency...
        KEYDATAFLAGS kdfT;
        NDIGetKeydataflags( cpage, ilineMid, &kdfT );
        NDGetBookmarkFromKDF( kdfT, &bmNode, fUnique ); //  this stamps out data.Cb() on unique trees.
        ASSERT_VALID( &bmNode );

        //  Unfortunately we don't have the pure line from the cpage.GetPtr( iline, &line ) call,
        //  which gives us the true tag size ... HOWEVER, we do have a calculation off that from
        //  what NDILineToKeydataflags(nested deep under NDIGetBookmark()) does to create the 
        //  kdf/bm data ... essentially this catches things b/c if the cbPrefix or cbSuffix is
        //  super big, we'll underflow (and thus make very BIG) the data.cb in that func.
#ifdef DEBUG
        PageEnforce( cpage, kdfT.data.Cb() > 0 );    // don't believe we have 0 cbs, >= if not.
#else
        PageEnforce( cpage, kdfT.data.Cb() > 0 || !g_fNodeMiscMemoryTrashedDefenseInDepthTemp );    // don't believe we have 0 cbs, >= if not.
#endif
    
        compare = CmpKeyData( bmNode, bm );

        if ( compare < 0 )
        {
            //  the midpoint item is less than what we are looking for. look in top half
            ilineFirst = ilineMid + 1;
        }
        else if ( 0 == compare )
        {
            //  we have found the item
            iline = ilineMid;
            break;
        }
        else    // ( compare > 0 )
        {
            //  the midpoint item is greater than what we are looking for. look in bottom half
            ilineLast = ilineMid;
            if ( ilineLast == ilineFirst )
            {
                iline = ilineMid;
                break;
            }
        }
    }

    if( 0 != compare && 0 == bm.data.Cb() )
    {
        *plastCompare = CmpKey( bmNode.key, bm.key );   //lint !e772
    }
    else
    {
        *plastCompare = compare;
    }

#ifdef DEBUG_NODE_SEEK
    INT compareT;
    const INT   ilineT = IlineNDISeekGEQDebug( cpage, bm, fUnique, &compareT );
    if ( ilineT >= 0 )
    {
        if ( compareT > 0 )
        {
            Assert( *plastCompare > 0 );
        }
        else if ( compareT < 0 )
        {
            Assert( *plastCompare < 0 );
        }
        else
        {
            Assert( 0 == *plastCompare );
        }
    }
    Assert( ilineT == iline );
#endif
    return iline;
}


//  ================================================================
INT IlineNDISeekGEQInternal(
    const CPAGE& cpage,
    const BOOKMARK& bm,
    INT * plastCompare )
//  ================================================================
//
//  seeks for bookmark in an internal page. We use binary search. At any
//  time the iline ilineLast is >= bm. To preserve this we include the
//  midpoint in the new range when the item we are seeking for is in the
//  lower half of the range.
//
//-
{
    Assert( !cpage.FLeafPage() || g_fRepair );

    const INT       clines      = cpage.Clines( );
    Assert( clines > 0 );   // will not work on an empty page
    KEYDATAFLAGS    kdfNode;
    INT             ilineFirst  = 0;
    INT             ilineLast   = clines - 1;
    INT             ilineMid    = 0;
    INT             compare     = 0;

    LINE            lineExternalHeader;
    LINE            line;
    cpage.GetPtrExternalHeader( &lineExternalHeader );
    kdfNode.key.prefix.SetPv( lineExternalHeader.pv );

    PageEnforce( cpage, lineExternalHeader.cb < g_cbPageMax );

    while( ilineFirst <= ilineLast )
    {
        ilineMid = (ilineFirst + ilineLast)/2;

        cpage.GetPtr( ilineMid, &line );
        NDILineToKeydataflags< pgnbcNoChecks >( cpage, &line, &kdfNode, NULL );
        Assert( kdfNode.key.prefix.Pv() == lineExternalHeader.pv );

        //  Note we validated the checking of this in read IO ErrCheckPage(), so the
        //  Enforce() doesn't continuously halt again and again on already stored 
        //  on-disk corruptions.

        PageEnforce( cpage, (ULONG)kdfNode.key.prefix.Cb() <= lineExternalHeader.cb );
        PageEnforce( cpage, (ULONG)kdfNode.key.suffix.Cb() < (ULONG)line.cb );     //  don't think it can be all key today so < vs. <=
        PageEnforce( cpage, ( ULONG( (BYTE*)line.pv - (BYTE*)cpage.PvBuffer() ) + line.cb ) < g_cbPageMax );   //  b/c it could be the tag array that is messed up.

        Assert( sizeof( PGNO ) == kdfNode.data.Cb() );
        compare = CmpKeyWithKeyData( kdfNode.key, bm );

        if ( compare < 0 )
        {
            //  the midpoint item is less than what we are looking for. look in top half
            ilineFirst = ilineMid + 1;
        }
        else if ( 0 == compare )
        {
            //  we have found the item
            break;
        }
        else    // ( compare > 0 )
        {
            //  the midpoint item is greater than what we are looking for. look in bottom half
            ilineLast = ilineMid;
            if ( ilineLast == ilineFirst )
            {
                break;
            }
        }
    }

    Assert( ilineMid >= 0 );
    Assert( ilineMid < clines );

    // if we are seeking for an NULL key and there is 
    // just one node in the internal page,
    // the above code will end with compare == 0
    // because the last internal node has a NULL key
    // Still, in fact we haven't found an item 
    // (because we are looking for NULL for some reason)
    //
    if ( 0 == compare && !bm.key.FNull() )
    {
        //  we found the item
        *plastCompare = 0;
    }
    else
    {
        *plastCompare   = 1;
    }

#ifdef DEBUG
    if( FExpensiveDebugCodeEnabled( Debug_Node_Seek ) )
    {
        INT compareT;
        const INT   ilineT = IlineNDISeekGEQInternalDebug( cpage, bm, &compareT );
        if ( compareT > 0 )
        {
            Assert( *plastCompare > 0 );
        }
        else if ( compareT < 0 )
        {
            Assert( *plastCompare < 0 );
        }
        else
        {
            Assert( 0 == *plastCompare );
        }
        Assert( ilineT == ilineMid );
    }
#endif

    return ilineMid;
}


//  ================================================================
LOCAL_BROKEN INT IlineNDISeekGEQDebug( const CPAGE& cpage, const BOOKMARK& bm, const BOOL fUnique, INT * plastCompare )
//  ================================================================
//
//  finds the first item greater or equal to the
//  given bookmark, returning the result of the last comparison
//
//-
{
    Assert( cpage.FLeafPage() );

    const INT       clines  = cpage.Clines( );
    BOOKMARK        bmNode;

    INT             iline   = 0;
    for ( ; iline < clines; iline++ )
    {
        NDIGetBookmark( cpage, iline, &bmNode, fUnique );
        INT cmp = CmpKey( bmNode.key, bm.key );

        if ( cmp < 0 )
        {
            //  keep on looking
        }
        else if ( cmp > 0 || bm.data.Cb() == 0 )
        {
            Assert( cmp > 0 ||
                    bm.data.Cb() == 0 && cmp == 0 );
            *plastCompare = cmp;
            return iline;
        }
        else
        {
            //  key is same
            //  check data -- only if we are seeking for a key-data
            Assert( cmp == 0 );
            Assert( bm.data.Cb() != 0 );

            cmp = CmpData( bmNode.data, bm.data );
            if ( cmp >= 0 )
            {
                *plastCompare = cmp;
                return iline;
            }
        }
    }

    return -1;
}


//  ================================================================
LOCAL_BROKEN INT IlineNDISeekGEQInternalDebug(
    const CPAGE& cpage,
    const BOOKMARK& bm,
    INT * plastCompare )
//  ================================================================
//
//  seeks for bookmark in an internal page
//
//-
{
    Assert( !cpage.FLeafPage() );

    const INT       clines = cpage.Clines( ) - 1;
    KEYDATAFLAGS    kdfNode;
    INT             iline;

    for ( iline = 0; iline < clines; iline++ )
    {
        NDIGetKeydataflags ( cpage, iline, &kdfNode );
        Assert( sizeof( PGNO ) == kdfNode.data.Cb() );

        const INT compare = CmpKeyWithKeyData( kdfNode.key, bm );
        if ( compare < 0 )
        {
            //  keep on looking
        }
        else
        {
            Assert( compare >= 0 );
            *plastCompare = compare;
            return iline;
        }
    }

    //  for internal pages
    //  last key or NULL is greater than everything else
#ifdef DEBUG
    Assert( cpage.Clines() - 1 == iline );
    NDIGetKeydataflags( cpage, iline, &kdfNode );
    Assert( kdfNode.key.FNull() ||
            CmpKeyWithKeyData( kdfNode.key, bm ) > 0 );
#endif

    *plastCompare = 1;
    return iline;
}


//  ================================================================
INLINE ERR ErrNDISeekInternalPage( const FUCB *pfucb, CSR * pcsr, const BOOKMARK& bm )
//  ================================================================
//
//  Seek on an internal page never returns wrnNDFoundLess
//  if bookmark.data is not null, never returns wrnNDEqual
//
//-
{
    ASSERT_VALID( &bm );

    ERR         err;
    INT         compare;
    const INT   iline   = IlineNDISeekGEQInternal( pcsr->Cpage(), bm, &compare );
    Assert( iline >= 0 );
    Assert( iline < pcsr->Cpage().Clines( ) );

    if ( 0 == compare )
    {
        //  we found an exact match
        //  page delimiter == bookmark of search
        //  the cursor is placed on node next to S
        //  because no node in the subtree rooted at S
        //  can have a key == XY -- such cursor placement obviates a moveNext at BT level
        //  Also, S can not be the last node in page because if it were,
        //  we would not have seeked down to this page!
        if ( iline >= pcsr->Cpage().Clines() - 1 )
        {
            FireWall( "SeekBadLastNode" );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucb ), HaDbFailureTagCorruption, L"35e8ae2a-c994-415a-8fdf-780d5e180274" );
            err = ErrERRCheck( JET_errDatabaseCorrupted );
        }
        else
        {
            pcsr->SetILine( iline + 1 );
            err = JET_errSuccess;
        }
    }
    else
    {
        //  we are on the first node greater than the BOOKMARK
        pcsr->SetILine( iline );
        err = ErrERRCheck( wrnNDFoundGreater );
    }

    return err;
}


//  ================================================================
INLINE ERR ErrNDISeekLeafPage( const FUCB * pfucb, CSR * pcsr, const BOOKMARK& bm )
//  ================================================================
//
//  Seek on leaf page returns wrnNDFoundGreater/Less/Equal
//
//-
{
    ASSERT_VALID( &bm );

    ERR         err;
    INT         compare;
    const INT   iline   = IlineNDISeekGEQ(
                                            pcsr->Cpage(),
                                            bm,
                                            FFUCBUnique( pfucb ),   //  UNDONE: can't uniqueness be determined by the cpage flags?
                                            &compare );
    Assert( iline < pcsr->Cpage().Clines( ) );

    if ( iline >= 0 && 0 == compare )
    {
        //  great! we found the node
        pcsr->SetILine( iline );
        err = JET_errSuccess;
    }
    else if ( iline < 0 )
    {
        //  all nodes in page are less than XY
        //  place cursor on last node in page;
        pcsr->SetILine( pcsr->Cpage().Clines( ) - 1 );
        err = ErrERRCheck( wrnNDFoundLess );
    }
    else
    {
        //  node S exists && key-data(S) > XY
        pcsr->SetILine( iline );
        err = ErrERRCheck( wrnNDFoundGreater );
    }

    return err;
}


//  ****************************************************************
//  EXTERNAL ROUTINES
//  ****************************************************************


//  ================================================================
VOID NDMoveFirstSon( FUCB * pfucb, CSR * pcsr )
//  ================================================================
{
    NDIAssertCurrency( pfucb, pcsr );

    pcsr->SetILine( 0 );
    NDGet( pfucb, pcsr );
}


//  ================================================================
VOID NDMoveLastSon( FUCB * pfucb, CSR * pcsr )
//  ================================================================
{
    NDIAssertCurrency( pfucb, pcsr );

    pcsr->SetILine( pcsr->Cpage().Clines() - 1 );
    NDGet( pfucb, pcsr );
}

//  ================================================================
ERR ErrNDVisibleToCursor( FUCB * pfucb, BOOL * pfVisibleToCursor, NS * pns )
//  ================================================================
//
//  returns true if the node, as seen by cursor, exists
//  and is not deleted
//
//-
{
    ERR             err         = JET_errSuccess;
    NS              ns          = nsDatabase;
    BOOL            fVisible;

    ASSERT_VALID( pfucb );
    AssertBTType( pfucb );
    AssertNDGet( pfucb );

    //  if session cursor isolation model is not dirty and node
    //  has version, then call version store for appropriate version.
    if ( FNDPossiblyVersioned( pfucb, Pcsr( pfucb ) )
        && !FPIBDirty( pfucb->ppib ) )
    {
        //  get bookmark from node in page
        BOOKMARK        bm;

        NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );

        Call( ErrVERAccessNode( pfucb, bm, &ns ) );
        switch( ns )
        {
            case nsVersionedUpdate:
                fVisible = fTrue;
                break;
            case nsDatabase:
            case nsUncommittedVerInDB:
            case nsCommittedVerInDB:
                fVisible = !FNDDeleted( pfucb->kdfCurr );
                break;
            case nsVersionedInsert:
            case nsNone:
            default:
                Assert( nsVersionedInsert == ns );
                fVisible = fFalse;
                break;
        }
    }
    else
    {
        fVisible = !FNDDeleted( pfucb->kdfCurr );
    }

    *pfVisibleToCursor = fVisible;
    if ( NULL != pns )
        *pns = ns;

HandleError:
    Assert( JET_errSuccess == err || 0 == pfucb->ppib->Level() );
    return err;
}


//  ================================================================
BOOL FNDPotVisibleToCursor( const FUCB * pfucb, CSR * pcsr )
//  ================================================================
//
//  returns true if node, as seen by cursor,
//  potentially exists
//  i.e., node is uncommitted by other or not deleted in page
//
//-
{
    ASSERT_VALID( pfucb );

    BOOL    fVisible = !( FNDDeleted( pfucb->kdfCurr ) );

    //  if session cursor isolation model is not dirty and node
    //  has version, then call version store for appropriate version.

    //  UNDONE: Use FNDPossiblyVersioned() if this function
    //  is called in any active code paths (currently, it's
    //  only called in DEBUG code or in code paths that
    //  should be impossible

    if ( FNDVersion( pfucb->kdfCurr )
        && !FPIBDirty( pfucb->ppib ) )
    {
        BOOKMARK    bm;
        NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
        const VS vs = VsVERCheck( pfucb, pcsr, bm );
        fVisible = FVERPotThere( vs, !fVisible );
    }

    return fVisible;
}


//  ================================================================
VOID NDBulkDelete( CSR * pcsr, INT clines )
//  ================================================================
//
//  deletes clines lines from current position in page
//      - expected to be at the end of page
//
//-
{
    ASSERT_VALID( pcsr );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( pcsr->FDirty() );
    Assert( 0 <= clines );
    Assert( pcsr->ILine() >= 0 );
    Assert( pcsr->Cpage().Clines() == pcsr->ILine() + clines );

    //  delete lines from the end to avoid reorganization of tag array
    INT iline = pcsr->ILine() + clines - 1;
    for ( ; iline >= pcsr->ILine(); --iline )
    {
        Assert( pcsr->Cpage().Clines() - 1 == iline );
        pcsr->Cpage().Delete( iline );
    }
}


//  ================================================================
VOID NDInsert( FUCB *pfucb, CSR * pcsr, const KEYDATAFLAGS * pkdf, INT cbPrefix )
//  ================================================================
//
//  inserts node into page at current cursor position
//  cbPrefix bytes of *pkdf is prefix key part
//  no logging/versioning
//  used by split to perform split operation
//
//-
{
#ifdef DEBUG
    if( pfucbNil != pfucb )
    {
        ASSERT_VALID( pfucb );
        Assert( pfucb->ppib->Level() > 0 || !g_rgfmp[pfucb->ifmp].FLogOn() );
    }
    ASSERT_VALID( pcsr );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( pcsr->FDirty() );
    ASSERT_VALID( pkdf );
#endif

    DATA            rgdata[cdataKDF+1];
    INT             fFlagsLine;
    KEYDATAFLAGS    kdf                 = *pkdf;
    LE_KEYLEN       le_keylen;
    le_keylen.le_cbPrefix = (USHORT)cbPrefix;

    const INT       cdata               = CdataNDIPrefixAndKeydataflagsToDataflags(
                                                    &kdf,
                                                    rgdata,
                                                    &fFlagsLine,
                                                    &le_keylen );
    Assert( cdata <= cdataKDF + 1 );

    pcsr->Cpage().Insert( pcsr->ILine(),
                          rgdata,
                          cdata,
                          fFlagsLine );


#ifdef DEBUG
    if( FExpensiveDebugCodeEnabled( Debug_Node_Insert ) && pfucbNil != pfucb )
    {
        NDAssertNDInOrder( pfucb, pcsr );
    }
#endif
}


//  ================================================================
VOID NDInsert( FUCB *pfucb, CSR * pcsr, const KEYDATAFLAGS * pkdf )
//  ================================================================
//
//  inserts node into page at current cursor position
//  no logging/versioning
//  used by split to perform split operation
//
//-
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( pcsr );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( pcsr->FDirty() );
    ASSERT_VALID( pkdf );
    Assert( pfucb->ppib->Level() > 0 || !g_rgfmp[pfucb->ifmp].FLogOn() );

    DATA        rgdata[cdataKDF];
    INT         fFlagsLine;
    LE_KEYLEN   le_keylen;
    const INT   cdata           = CdataNDIKeydataflagsToDataflags(
                                            pkdf,
                                            rgdata,
                                            &fFlagsLine,
                                            &le_keylen );
    pcsr->Cpage().Insert( pcsr->ILine(),
                          rgdata,
                          cdata,
                          fFlagsLine );
    Assert( cdata <= cdataKDF );

    NDGet( pfucb, pcsr );
}


//  ================================================================
VOID NDReplaceForUpgrade(
    CPAGE * const pcpage,
    const INT iline,
    const DATA * const pdata,
    const KEYDATAFLAGS& kdfOld )
//  ================================================================
//
//  replaces data in page at current cursor position
//  no logging/versioning
//  used by upgrade to change record format
//  does not do a NDGet!
//
//-
{
    KEYDATAFLAGS    kdf = kdfOld;
    DATA            rgdata[cdataKDF];
    INT             fFlagsLine;
    LE_KEYLEN       le_keylen;

#ifdef DEBUG
    //  make sure the correct KDF was passed in
    KEYDATAFLAGS kdfDebug;
    NDIGetKeydataflags( *pcpage, iline, &kdfDebug );
    Assert( kdfDebug.key.prefix.Pv() == kdfOld.key.prefix.Pv() );
    Assert( kdfDebug.key.prefix.Cb() == kdfOld.key.prefix.Cb() );
    Assert( kdfDebug.key.suffix.Pv() == kdfOld.key.suffix.Pv() );
    Assert( kdfDebug.key.suffix.Cb() == kdfOld.key.suffix.Cb() );
    Assert( kdfDebug.data.Pv() == kdfOld.data.Pv() );
    Assert( kdfDebug.data.Cb() == kdfOld.data.Cb() );
#endif  //  DEBUG

    Assert( kdf.data.Cb() >= pdata->Cb() ); //  cannot grow the record as we have pointers to data on the page
    kdf.data = *pdata;

    const INT   cdata       = CdataNDIKeydataflagsToDataflags(
                                        &kdf,
                                        rgdata,
                                        &fFlagsLine,
                                        &le_keylen );
    Assert( cdata <= ( sizeof( rgdata ) / sizeof( rgdata[0]) ) );

    pcpage->Replace( iline, rgdata, cdata, fFlagsLine );
}


//  ================================================================
VOID NDReplace( CSR * pcsr, const DATA * pdata )
//  ================================================================
//
//  replaces data in page at current cursor position
//  no logging/versioning
//  used by split to perform split operation and by root page move
//  to replace catalog entries
//  does not do a NDGet!
//
//-
{
    NDIAssertCurrencyExists( pcsr );
    ASSERT_VALID( pcsr );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( pcsr->FDirty() );
    ASSERT_VALID( pdata );

    KEYDATAFLAGS    kdf;
    DATA            rgdata[cdataKDF];
    INT             fFlagsLine;
    LE_KEYLEN       le_keylen;

    //  get key and flags from page
    NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdf );
    kdf.data = *pdata;

    //  replace node with new data
    const INT       cdata           = CdataNDIKeydataflagsToDataflags(
                                                &kdf,
                                                rgdata,
                                                &fFlagsLine,
                                                &le_keylen );
    Assert( cdata <= cdataKDF );
    pcsr->Cpage().Replace( pcsr->ILine(), rgdata, cdata, fFlagsLine );
}


//  ================================================================
VOID NDDelete( CSR *pcsr )
//  ================================================================
//
//  deletes node from page at current cursor position
//  no logging/versioning
//
//-
{
    ASSERT_VALID( pcsr );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( pcsr->FDirty() );
    Assert( pcsr->ILine() < pcsr->Cpage().Clines() );

    pcsr->Cpage().Delete( pcsr->ILine() );
}

NOINLINE VOID NDIReportSeekOnEmptyPage( const FUCB * const pfucb, const CSR * const pcsr, const ERR err )
{
    WCHAR           rgwszDw[5][16];

    OSStrCbFormatW( rgwszDw[0], sizeof(rgwszDw[0]), L"%d", err );
    OSStrCbFormatW( rgwszDw[1], sizeof(rgwszDw[1]), L"%u", pfucb->u.pfcb->ObjidFDP() );
    OSStrCbFormatW( rgwszDw[2], sizeof(rgwszDw[2]), L"%u", pfucb->u.pfcb->PgnoFDP() );
    OSStrCbFormatW( rgwszDw[3], sizeof(rgwszDw[3]), L"%u", pcsr->Pgno() );
    OSStrCbFormatW( rgwszDw[4], sizeof(rgwszDw[4]), L"0x%I32x", pcsr->Cpage().FFlags() );

    const WCHAR *   rgwsz[] = { rgwszDw[0], rgwszDw[1], rgwszDw[2], rgwszDw[3], g_rgfmp[pfucb->u.pfcb->Ifmp()].WszDatabaseName(), rgwszDw[4] };

    UtilReportEvent(
            eventError,
            DATABASE_CORRUPTION_CATEGORY,
            BAD_PAGE_EMPTY_SEEK_ID,
            _countof( rgwsz ),
            rgwsz,
            0,
            NULL,
            PinstFromPfucb( pfucb ) );
}

//  ================================================================
ERR ErrNDSeek( FUCB * pfucb, CSR * pcsr, const BOOKMARK& bookmark )
//  ================================================================
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( pcsr );
    ASSERT_VALID( &bookmark );

    AssertSz( pcsr->Cpage().Clines() > 0, "Seeking on an empty page" );     //  can happen on lost flush corruption
    if ( pcsr->Cpage().Clines() <= 0 )
    {
        //  note: some upper code paths translate this error to one with better context data
        const ERR errRet = ErrERRCheck( JET_errBadEmptyPage );
        NDIReportSeekOnEmptyPage( pfucb, pcsr, errRet );
        return errRet;
    }

    const BOOL  fInternalPage   = !pcsr->Cpage().FLeafPage();
    ERR         err             = JET_errSuccess;

    if ( fInternalPage )
    {
        err = ErrNDISeekInternalPage( pfucb, pcsr, bookmark );
    }
    else
    {
        err = ErrNDISeekLeafPage( pfucb, pcsr, bookmark );
    }

    if ( err >= JET_errSuccess )
    {
        NDGet( pfucb, pcsr );
    }
    return err;
}


//  ================================================================
ERR ErrNDSeekInternal( FUCB * pfucb, CSR * pcsr, const BOOKMARK& bookmark )
//  ================================================================
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( pcsr );
    ASSERT_VALID( &bookmark );
    Assert( pcsr->Cpage().Clines() > 0 );
    Assert( !pcsr->Cpage().FLeafPage() );
    Assert( !bookmark.key.FNull() );

    INT compare = 0;
    const INT iline = IlineNDISeekGEQInternal( pcsr->Cpage(), bookmark, &compare );
    pcsr->SetILine( iline );
    NDGet( pfucb, pcsr );
    return ( ( compare == 0 ) ? JET_errSuccess : ErrERRCheck( wrnNDFoundGreater ) );
}


//  ================================================================
ERR ErrNDGet( FUCB * pfucb, const CSR * pcsr )
//  ================================================================
{
    if ( pcsr->ILine() < 0 ||
         pcsr->ILine() >= pcsr->Cpage().Clines() )
    {
        return ErrNDIReportBadLineCount( pfucb, ErrERRCheck( JET_errBadLineCount ), pcsr );
    }

    NDGet( pfucb, pcsr );
    return JET_errSuccess;
}

//  ================================================================
VOID NDGet( FUCB * pfucb, const CSR * pcsr )
//  ================================================================
{
    NDIAssertCurrencyExists( pfucb, pcsr );

    KEYDATAFLAGS keydataflags;
    NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &keydataflags );
    
    pfucb->kdfCurr = keydataflags;
    pfucb->fBookmarkPreviouslySaved = fFalse;
}


//  ================================================================
ERR ErrNDInsert(
        FUCB * const pfucb,
        CSR * const pcsr,
        const KEYDATAFLAGS * const pkdf,
        const DIRFLAG dirflag,
        const RCEID rceid,
        const VERPROXY * const pverproxy )
//  ================================================================
{
    NDIAssertCurrency( pfucb, pcsr );
    ASSERT_VALID( pkdf );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( NULL == pverproxy || proxyCreateIndex == pverproxy->proxy );

    ERR         err             = JET_errSuccess;
    const BOOL  fVersion        = !( dirflag & fDIRNoVersion ) && !g_rgfmp[pfucb->ifmp].FVersioningOff();
    const BOOL  fLogging        = !( dirflag & fDIRNoLog ) && g_rgfmp[pfucb->ifmp].FLogOn();

    KEYDATAFLAGS kdfT = *pkdf;

    Assert( !fLogging || pfucb->ppib->Level() > 0 );
    Assert( !fVersion || rceidNull != rceid || !fLogging );

    //  BUGFIX #151574: caller should have already ascertained that the node will
    //  fit on the page, but at that time, the caller may not have had the
    //  write-latch on the page and couldn't update the cbUncommittedFree, so
    //  now that we have the write-latch, we have re-compute to guarantee that
    //  the cbFree doesn't become less than the cbUncommittedFree
    const INT   cbReq           = CbNDNodeSizeCompressed( kdfT );
    if ( cbReq > pcsr->Cpage().CbPageFree( ) - pcsr->Cpage().CbUncommittedFree( ) )
    {
        //  now that we have the write-latch on the page we are guaranteed to
        //  be able to upgrade cbUncommittedFree on the page (this call to 
        //  FNDFreePageSpace() is now guaranteed to succeed and update the 
        //  cbUncommittedFree if necessary).
        const BOOL  fFreePageSpace  = FNDFreePageSpace( pfucb, pcsr, cbReq );
        AssertRTL( fFreePageSpace );

        //  now that we have have updated the page's free space, we _should_ be
        //  guaranteed to be able to fit the node/cbReq on the page, but re-check
        //  and bail out to ensure we don't corrupt the page.
        if ( cbReq > pcsr->Cpage().CbPageFree( ) - pcsr->Cpage().CbUncommittedFree( ) )
        {
            //  we used to Enforce() but this became exceptionally painful when 
            //  we hit this repeatedly during recovery (on multiple replicates). 
            //  we should never hit this b/c BT should always ensure there is 
            //  enough page space before doing this, and recovery shouldn't do 
            //  anything that BT didn't say was ok
            AssertRTL( cbReq <= pcsr->Cpage().CbPageFree() - pcsr->Cpage().CbUncommittedFree() );
            FireWall( "InsertNodeTooBig" );

            OSUHAEmitFailureTag( PinstFromPfucb( pfucb ), HaDbFailureTagCorruption, L"e21b39dc-57a5-48d3-9f8f-fe0330f255c2" );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
    }

    if ( pcsr->ILine() < 0 ||
         pcsr->ILine() > pcsr->Cpage().Clines() )
    {
        return ErrNDIReportBadLineCount( pfucb, ErrERRCheck( JET_errBadLineCount ), pcsr );
    }

    if ( fLogging )
    {
        //  log the operation, getting the lgpos
        LGPOS   lgpos;

        Call( ErrLGInsert( pfucb, pcsr, kdfT, rceid, dirflag, &lgpos, pverproxy, fMustDirtyCSR ) );
        pcsr->Cpage().SetLgposModify( lgpos );
    }
    else
    {
        pcsr->Dirty( );
    }

    //  insert operation can not fail after this point
    //  since we have already logged the operation
    if( fVersion )
    {
        kdfT.fFlags |= fNDVersion;
    }
    NDInsert( pfucb, pcsr, &kdfT );
    NDGet( pfucb, pcsr );

HandleError:
    Assert( JET_errSuccess == err || fLogging );
#ifdef DEBUG
    if( FExpensiveDebugCodeEnabled( Debug_Node_Insert ) )
    {
        NDAssertNDInOrder( pfucb, pcsr );
    }
#endif
    return err;
}


INLINE ERR ErrNDILogReplace(
    FUCB    * const pfucb,
    CSR     * const pcsr,
    const DATA&     data,
    const DIRFLAG   dirflag,
    const RCEID     rceid,
    BOOL *          pfEmptyDiff )
{
    ERR     err             = JET_errSuccess;
    void*   pvDiffBuffer    = NULL;
    DATA    dataDiff;
    BOOL    fOverflow       = fTrue;
    SIZE_T  cbDiff          = 0;

    *pfEmptyDiff = fFalse;

    AssertNDGet( pfucb, pcsr );     // logdiff needs access to kdfCurr

    Assert( data.Cb() <= g_rgfmp[ pfucb->ifmp ].CbPage() );
    if ( dirflag & ( fDIRLogColumnDiffs | fDIRLogChunkDiffs ) )
    {
        BFAlloc( bfasTemporary, &pvDiffBuffer );
        dataDiff.SetPv( pvDiffBuffer );
    }

    if ( dirflag & fDIRLogColumnDiffs )
    {
        Assert( data.Cb() >= REC::cbRecordMin );
        Assert( data.Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
        Assert( data.Cb() <= REC::CbRecordMost( pfucb ) );
        if ( data.Cb() > REC::CbRecordMost( pfucb ) )
        {
            FireWall( "NDILogReplaceDataRecTooBig8.2" ); // trying to update below if to more context sensitive CbRecordMost() call.
        }
        if ( data.Cb() < REC::cbRecordMin ||
            data.Cb() > REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) )
        {
            FireWall( "NDILogReplaceDataRecTooBig8.1" );
            Error( ErrERRCheck( JET_errInternalError ) );
        }

        if ( NULL != dataDiff.Pv() )
        {
            LGSetColumnDiffs(
                    pfucb,
                    data,
                    pfucb->kdfCurr.data,
                    (BYTE *)dataDiff.Pv(),
                    &fOverflow,
                    &cbDiff );
            Assert( cbDiff <= (SIZE_T)REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
            Assert( cbDiff <= (SIZE_T)REC::CbRecordMost( pfucb ) );
            Assert( !fOverflow || cbDiff == 0 );
            if ( cbDiff == 0 && !fOverflow )
            {
                *pfEmptyDiff = fTrue;
                Assert( err >= JET_errSuccess );
                goto HandleError;
            }
        }
    }
    else if ( dirflag & fDIRLogChunkDiffs )
    {
        const LONG cbLVChunkMost = CbOSEncryptAes256SizeNeeded( pfucb->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost() );
        Assert( data.Cb() <= cbLVChunkMost );
        if ( data.Cb() > cbLVChunkMost )
        {

            Error( ErrERRCheck( JET_errInternalError ) );
        }

        if ( NULL != dataDiff.Pv() )
        {
            LGSetLVDiffs(
                    pfucb,
                    data,
                    pfucb->kdfCurr.data,
                    (BYTE *)dataDiff.Pv(),
                    &fOverflow,
                    &cbDiff );
            Assert( cbDiff <= (SIZE_T)cbLVChunkMost );
            Assert( !fOverflow || cbDiff == 0 );
            if ( cbDiff == 0 && !fOverflow )
            {
                *pfEmptyDiff = fTrue;
                Assert( err >= JET_errSuccess );
                goto HandleError;
            }
        }
    }

    Assert( pvDiffBuffer || 0 == cbDiff );
    dataDiff.SetCb( cbDiff );

    //  log the operation, getting the lgpos
    LGPOS   lgpos;
    Call( ErrLGReplace( pfucb,
                        pcsr,
                        pfucb->kdfCurr.data,
                        data,
                        ( cbDiff > 0 ? &dataDiff : NULL ),
                        rceid,
                        dirflag,
                        &lgpos,
                        fMustDirtyCSR ) );
    pcsr->Cpage().SetLgposModify( lgpos );

HandleError:
    if ( pvDiffBuffer )
    {
        BFFree( pvDiffBuffer );
    }
    return err;
}

//  ================================================================
ERR ErrNDReplace(
        FUCB * const pfucb,
        CSR * const pcsr,
        const DATA * const pdata,
        const DIRFLAG dirflag,
        const RCEID rceid,
        RCE * const prceReplace )
//  ================================================================
//
//  UNDONE: we only need to replace the data, not the key
//          the page level should take the length of the key
//          and not replace the data
//
//-
{
    NDIAssertCurrencyExists( pfucb, pcsr ); //  we should have done a FlagReplace
    ASSERT_VALID( pdata );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );

    ERR         err         = JET_errSuccess;

    NDGet( pfucb, pcsr );
    const INT   cbDataOld   = pfucb->kdfCurr.data.Cb();
    const INT   cbReq       = pdata->Cb() - cbDataOld;
    const BOOL  fDirty      = !( dirflag & fDIRNoDirty );

    if ( cbReq > 0 && !FNDFreePageSpace( pfucb, pcsr, cbReq ) )
    {
        //  requested space not available in page
        //  check if same node has enough uncommitted freed space to be used
        Assert( fDirty );

        const ULONG     cbReserved  = CbNDReservedSizeOfNode( pfucb, pcsr );
        if ( (ULONG)cbReq > cbReserved + pcsr->Cpage().CbPageFree() - pcsr->Cpage().CbUncommittedFree() )
        {
            err = ErrERRCheck( errPMOutOfPageSpace );
            Assert( fDirty );
            return err;
        }
    }

    const BOOL      fVersion            = !( dirflag & fDIRNoVersion ) && !g_rgfmp[pfucb->ifmp].FVersioningOff();
    const BOOL      fLogging            = !( dirflag & fDIRNoLog ) && g_rgfmp[pfucb->ifmp].FLogOn();
    Assert( !fLogging || pfucb->ppib->Level() > 0 );
    Assert( !fVersion || rceidNull != rceid || !fLogging );
    Assert( ( prceNil == prceReplace ) == !fVersion || PinstFromIfmp( pfucb->ifmp )->FRecovering() );

    INT             cdata;
    INT             fFlagsLine;
    KEYDATAFLAGS    kdf;
    DATA            rgdata[cdataKDF+1];
    LE_KEYLEN       le_keylen;

    Assert( fDirty || pcsr->FDirty() );

    if ( fLogging )
    {
        BOOL fEmptyDiff = fFalse;

        //  we have to dirty
        Assert( fDirty );
        Call( ErrNDILogReplace(
                    pfucb,
                    pcsr,
                    *pdata,
                    dirflag,
                    rceid,
                    &fEmptyDiff ) );
        if ( fEmptyDiff )
        {
            Assert( cbReq == 0 );
            prceReplace->FlagEmptyDiff();
            NDGet( pfucb, pcsr );
            goto HandleError;
        }
    }
    else if ( fDirty )
    {
        pcsr->Dirty( );
    }

    // Do not modify page unless it is dirty
    Assert( pcsr->FDirty() );

    //  operation cannot fail from here on

    //  get key and flags from fucb
    kdf.key     = pfucb->bmCurr.key;
    kdf.data    = *pdata;
    kdf.fFlags  = pfucb->kdfCurr.fFlags | ( fVersion ? fNDVersion : 0 );

#ifdef DEBUG
    {
        AssertNDGet( pfucb, pcsr );

        KEYDATAFLAGS    kdfT;

        //  get key and flags from page. compare with bookmark
        NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdfT );
        Assert( CmpKey( kdf.key, kdfT.key ) == 0 );
    }
#endif  //  DEBUG

    //  replace node with new data
    le_keylen.le_cbPrefix = USHORT( pfucb->kdfCurr.key.prefix.Cb() );
    cdata = CdataNDIPrefixAndKeydataflagsToDataflags( &kdf, rgdata, &fFlagsLine, &le_keylen );
    Assert( cdata <= cdataKDF + 1 );

    if ( prceNil != prceReplace && cbReq > 0 )
    {
        Assert( fVersion || PinstFromIfmp( pfucb->ifmp )->FRecovering() );
        //  set uncommitted freed space in page for growing node
        VERSetCbAdjust( pcsr, prceReplace, pdata->Cb(), cbDataOld, fDoUpdatePage );
    }

    pcsr->Cpage().Replace( pcsr->ILine(), rgdata, cdata, fFlagsLine );

    if ( prceNil != prceReplace && cbReq < 0 )
    {
        Assert( fVersion || PinstFromIfmp( pfucb->ifmp )->FRecovering() );
        //  set uncommitted freed space for shrinking node
        VERSetCbAdjust( pcsr, prceReplace, pdata->Cb(), cbDataOld, fDoUpdatePage );
    }

    NDGet( pfucb, pcsr );

HandleError:
    Assert( JET_errSuccess == err || fLogging );
    return err;
}


//  ================================================================
ERR ErrNDFlagInsert(
        FUCB * const pfucb,
        CSR * const pcsr,
        const DIRFLAG dirflag,
        const RCEID rceid,
        const VERPROXY * const pverproxy )
//  ================================================================
//
//  this is a flag-undelete with the correct logging and version store stuff
//
//-
{
    NDIAssertCurrency( pfucb, pcsr );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( NULL == pverproxy || proxyCreateIndex == pverproxy->proxy );

    ERR         err             = JET_errSuccess;
    const BOOL  fVersion        = !( dirflag & fDIRNoVersion ) && !g_rgfmp[pfucb->ifmp].FVersioningOff();
    const BOOL  fLogging        = !( dirflag & fDIRNoLog ) && g_rgfmp[pfucb->ifmp].FLogOn();
    const BOOL  fDirty      = !( dirflag & fDIRNoDirty );
    Assert( !fLogging || pfucb->ppib->Level() > 0 );
    Assert( !fVersion || rceidNull != rceid || !fLogging );

    NDGet( pfucb, pcsr );

    Assert( fDirty || pcsr->FDirty() );

    if ( fLogging )
    {
        //  log the operation, getting the lgpos
        LGPOS   lgpos;

        // we have to dirty
        Assert (fDirty);

        Call( ErrLGFlagInsert( pfucb, pcsr, pfucb->kdfCurr, rceid, dirflag, &lgpos, pverproxy, fMustDirtyCSR ) );
        pcsr->Cpage().SetLgposModify( lgpos );
    }
    else
    {
        if ( fDirty )
        {
            pcsr->Dirty( );
        }
    }

    NDIResetFlag( pcsr, fNDDeleted );
    if ( fVersion )
    {
        NDISetFlag( pcsr, fNDVersion );
    }
    NDGet( pfucb, pcsr );

HandleError:
    Assert( JET_errSuccess == err || fLogging );
#ifdef DEBUG
    if( FExpensiveDebugCodeEnabled( Debug_Node_Insert ) )
    {
        NDAssertNDInOrder( pfucb, pcsr );
    }
#endif
    return err;
}


//  ================================================================
ERR ErrNDFlagInsertAndReplaceData(
    FUCB                * const pfucb,
    CSR                 * const pcsr,
    const KEYDATAFLAGS  * const pkdf,
    const DIRFLAG       dirflag,
    const RCEID         rceidInsert,
    const RCEID         rceidReplace,
    const RCE           * const prceReplace,
    const VERPROXY * const pverproxy )
//  ================================================================
{
    NDIAssertCurrency( pfucb, pcsr );
    Assert( pkdf->data.Cb() > 0 );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( NULL == pverproxy || proxyCreateIndex == pverproxy->proxy );

    ERR         err             = JET_errSuccess;
    const BOOL  fVersion        = !( dirflag & fDIRNoVersion ) && !g_rgfmp[pfucb->ifmp].FVersioningOff();
    const BOOL  fLogging        = !( dirflag & fDIRNoLog ) && g_rgfmp[pfucb->ifmp].FLogOn();
    Assert( !fLogging || pfucb->ppib->Level() > 0 );
    Assert( !fVersion || rceidNull != rceidInsert || !fLogging );
    Assert( !fVersion || rceidNull != rceidReplace || !fLogging );
    Assert( !fVersion || prceNil != prceReplace || !fLogging );

    NDGet( pfucb, pcsr );
    Assert( FNDDeleted( pfucb->kdfCurr ) );
    Assert( FKeysEqual( pkdf->key, pfucb->kdfCurr.key ) );

    const INT   cbDataOld   = pfucb->kdfCurr.data.Cb();
    const INT   cbReq       = pkdf->data.Cb() - cbDataOld;

    if ( cbReq > 0 && !FNDFreePageSpace ( pfucb, pcsr, cbReq ) )
    {
        //  requested space not available in page
        //  check if same node has enough uncommitted freed space to be used
        const ULONG     cbReserved  = CbNDReservedSizeOfNode( pfucb, pcsr );
        if ( (ULONG)cbReq > cbReserved + pcsr->Cpage().CbPageFree() - pcsr->Cpage().CbUncommittedFree() )
        {
            err = ErrERRCheck( errPMOutOfPageSpace );
            return err;
        }
    }

    KEYDATAFLAGS    kdf;
    INT             cdata;
    DATA            rgdata[cdataKDF+1];
    INT             fFlagsLine;
    LE_KEYLEN       le_keylen;


    Assert( !(dirflag & fDIRNoDirty ) );

    if ( fLogging )
    {
        //  log the operation, getting the lgpos
        LGPOS   lgpos;

        Call( ErrLGFlagInsertAndReplaceData( pfucb,
                                             pcsr,
                                             *pkdf,
                                             rceidInsert,
                                             rceidReplace,
                                             dirflag,
                                             &lgpos,
                                             pverproxy,
                                             fMustDirtyCSR ) );
        pcsr->Cpage().SetLgposModify( lgpos );
    }
    else
    {
        pcsr->Dirty( );
    }

    //  get key from fucb
    kdf.key         = pkdf->key;
    kdf.data        = pkdf->data;
    kdf.fFlags      = pfucb->kdfCurr.fFlags | ( fVersion ? fNDVersion : 0 );

    Assert( FKeysEqual( kdf.key, pfucb->kdfCurr.key ) );

    le_keylen.le_cbPrefix = USHORT( pfucb->kdfCurr.key.prefix.Cb() );

    //  replace node with new data
    cdata = CdataNDIPrefixAndKeydataflagsToDataflags(
                    &kdf,
                    rgdata,
                    &fFlagsLine,
                    &le_keylen );


    if ( prceNil != prceReplace && cbReq > 0 )
    {
        Assert( fVersion || PinstFromIfmp( pfucb->ifmp )->FRecovering() );
        //  set uncommitted freed space in page for growing node
        VERSetCbAdjust( pcsr, prceReplace, pkdf->data.Cb(), cbDataOld, fDoUpdatePage );
    }

    pcsr->Cpage().Replace( pcsr->ILine(), rgdata, cdata, fFlagsLine );

    if ( prceNil != prceReplace && cbReq < 0 )
    {
        Assert( fVersion || PinstFromIfmp( pfucb->ifmp )->FRecovering() );
        //  set uncommitted freed space for shrinking node
        VERSetCbAdjust( pcsr, prceReplace, pkdf->data.Cb(), cbDataOld, fDoUpdatePage );
    }

    NDIResetFlag( pcsr, fNDDeleted );

    NDGet( pfucb, pcsr );

HandleError:
    Assert( JET_errSuccess == err || fLogging );
#ifdef DEBUG
    if( FExpensiveDebugCodeEnabled( Debug_Node_Insert ) )
    {
        NDAssertNDInOrder( pfucb, pcsr );
    }
#endif
    return err;
}


//  ================================================================
template< typename TDelta >
ERR ErrNDDelta(
        FUCB            * const pfucb,
        CSR             * const pcsr,
        const INT       cbOffset,
        const TDelta    tDelta,
        TDelta          * const ptOldValue,
        const DIRFLAG   dirflag,
        const RCEID     rceid )
//  ================================================================
//
//  No VERPROXY because it is not used by concurrent create index
//
{
    NDIAssertCurrency( pfucb, pcsr );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );

    const BOOL  fVersion        = !( dirflag & fDIRNoVersion ) && !g_rgfmp[pfucb->ifmp].FVersioningOff();
    const BOOL  fLogging        = !( dirflag & fDIRNoLog ) && g_rgfmp[pfucb->ifmp].FLogOn();
    const BOOL  fDirty          = !( dirflag & fDIRNoDirty );
    Assert( !fLogging || pfucb->ppib->Level() > 0 );
    Assert( !fVersion || rceidNull != rceid || !fLogging );
    ERR                     err     = JET_errSuccess;

    KEYDATAFLAGS keydataflags;
    NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &keydataflags );
    Assert( cbOffset <= (INT)keydataflags.data.Cb() );
    Assert( cbOffset >= 0 );

    UnalignedLittleEndian< TDelta > * const pule_tDelta = (UnalignedLittleEndian< TDelta > *)( (BYTE *)keydataflags.data.Pv() + cbOffset );

    BOOKMARK        bookmark;
    NDIGetBookmark( pcsr->Cpage(), pcsr->ILine(), &bookmark, FFUCBUnique( pfucb ) );

    if ( ptOldValue )
    {
        //  Endian conversion
        *ptOldValue = *pule_tDelta;
    }

    Assert( fDirty || pcsr->FDirty() );

    if ( fLogging )
    {
        //  log the operation, getting the lgpos
        LGPOS   lgpos;

        // we have to dirty
        Assert (fDirty);

        Call( ErrLGDelta< TDelta >( pfucb, pcsr, bookmark, cbOffset, tDelta, rceid, dirflag, &lgpos, fMustDirtyCSR ) );
        pcsr->Cpage().SetLgposModify( lgpos );
    }
    else
    {
        if ( fDirty )
        {
            pcsr->Dirty( );
        }
    }

    //  we have a pointer to the data on the page so we can modify the data directly
    *pule_tDelta += tDelta;

    if ( fVersion )
    {
        NDISetFlag( pcsr, fNDVersion );
    }

HandleError:
    Assert( JET_errSuccess == err || fLogging );
    return err;
}


//  ================================================================
ERR ErrNDFlagDelete(
        FUCB * const pfucb,
        CSR * const pcsr,
        const DIRFLAG dirflag,
        const RCEID rceid,
        const VERPROXY * const pverproxy )
//  ================================================================
{
    NDIAssertCurrency( pfucb, pcsr );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( NULL == pverproxy || proxyCreateIndex == pverproxy->proxy );

    const BOOL  fVersion        = !( dirflag & fDIRNoVersion ) && !g_rgfmp[pfucb->ifmp].FVersioningOff();
    const BOOL  fLogging        = !( dirflag & fDIRNoLog ) && g_rgfmp[pfucb->ifmp].FLogOn();
    const BOOL  fDirty      = !( dirflag & fDIRNoDirty );
    const ULONG fFlags      = fNDDeleted | ( fVersion ? fNDVersion : 0 );

    Assert( !fLogging || pfucb->ppib->Level() > 0 );
    Assert( !fVersion || rceidNull != rceid || !fLogging );

    ERR         err             = JET_errSuccess;

    Assert( fDirty || pcsr->FDirty() );

    if ( fLogging )
    {
        //  log the operation, getting the lgpos
        LGPOS   lgpos;

        // we have to dirty
        Assert (fDirty);

        Call( ErrLGFlagDelete( pfucb, pcsr, rceid, dirflag, &lgpos, pverproxy, fMustDirtyCSR ) );
        pcsr->Cpage().SetLgposModify( lgpos );
    }
    else
    {
        if ( fDirty )
        {
            pcsr->Dirty( );
        }
    }

    NDISetFlag( pcsr, fFlags );

    //  track the count and size of nodes we are flag deleting as a way of measuring the version store
    //  cleanup debt we are incurring
    {
        KEYDATAFLAGS kdf;
        NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdf );

        TLS* const ptls = Ptls();
        ptls->threadstats.cNodesFlagDeleted++;
        ptls->threadstats.cbNodesFlagDeleted += kdf.key.Cb() + kdf.data.Cb();
    }

HandleError:
    Assert( JET_errSuccess == err || fLogging );
    return err;
}


//  ================================================================
VOID NDResetFlagDelete( CSR * pcsr )
//  ================================================================
//
//  this is called by VER to undo. the undo is already logged
//  so we don't need to log or version
//
//-
{
    Assert( pcsr->ILine() >= 0 );
    Assert( pcsr->ILine() < pcsr->Cpage().Clines( ) );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );

    NDIResetFlag( pcsr, fNDDeleted );
}


//  ================================================================
VOID NDDeferResetNodeVersion( CSR * pcsr )
//  ================================================================
//
//  we want to reset the bit but only flush the page if necessary
//  this will update the checksum
//
//-
{
    Assert( pcsr->ILine() >= 0 );
    Assert( pcsr->ILine() < pcsr->Cpage().Clines( ) );

    LATCH latchOld;
    if ( pcsr->ErrUpgradeToWARLatch( &latchOld ) == JET_errSuccess )
    {
        NDIResetFlag( pcsr, fNDVersion );
        BFDirty( pcsr->Cpage().PBFLatch(), bfdfUntidy, *TraceContextScope() );
        pcsr->DowngradeFromWARLatch( latchOld );
    }
}


//  ================================================================
VOID NDResetVersionInfo( CPAGE * const pcpage )
//  ================================================================
{
    if ( pcpage->FLeafPage() && !pcpage->FSpaceTree() )
    {
        pcpage->ResetAllFlags( fNDVersion );
        pcpage->SetCbUncommittedFree( 0 );
    }
}


//  ================================================================
bool FNDAnyNodeIsVersioned( const CPAGE& cpage )
//  ================================================================
{
    if ( cpage.FLeafPage() && !cpage.FSpaceTree() )
    {
        return cpage.CbUncommittedFree() || cpage.FAnyLineHasFlagSet( fNDVersion );
    }

    return false;
}

//  ================================================================
bool FNDAnyNodeIsCompressed( const CPAGE& cpage )
//  ================================================================
{
    return cpage.FAnyLineHasFlagSet( fNDCompressed );
}


//  ================================================================
ERR ErrNDFlagVersion( CSR * pcsr )
//  ================================================================
//
//  not logged
//
//-
{
    Assert( pcsr->ILine() >= 0 );
    Assert( pcsr->ILine() < pcsr->Cpage().Clines( ) );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( pcsr->FDirty() );

    NDISetFlag( pcsr, fNDVersion );
    return JET_errSuccess;
}


//  ================================================================
ERR ErrNDDelete( FUCB * pfucb, CSR * pcsr, const DIRFLAG dirflag )
//  ================================================================
//
//  delete is called by cleanup to delete *visible* nodes
//  that have been flagged as deleted.  This operation is logged
//  for redo only.
//
//-
{
#ifdef DEBUG
    NDIAssertCurrency( pfucb, pcsr );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    NDGet( pfucb, pcsr );
    Assert( FNDDeleted( pfucb->kdfCurr ) ||
            !FNDVersion( pfucb->kdfCurr ) );
#endif

    ERR         err         = JET_errSuccess;
    const BOOL  fLogging    = !( dirflag & fDIRNoLog ) && g_rgfmp[pfucb->ifmp].FLogOn();
    Assert( !g_rgfmp[pfucb->ifmp].FLogOn() || !PinstFromIfmp( pfucb->ifmp )->m_plog->FLogDisabled() );
    Assert( pfucb->ppib->Level() > 0 || !fLogging );

    Assert( !( dirflag & fDIRNoDirty ) );

    //  log the operation
    if ( fLogging )
    {
        LGPOS   lgpos;

        CallR( ErrLGDelete( pfucb, pcsr, &lgpos, fMustDirtyCSR ) );
        pcsr->Cpage().SetLgposModify( lgpos );
    }
    else
    {
        pcsr->Dirty( );
    }

    NDDelete( pcsr );

    return err;
}

//  ================================================================
//  Get persisted flag out of nodeRootField enum
LOCAL_BROKEN INLINE BYTE BNDIGetPersistedNrfFlag( _In_range_(noderfSpaceHeader, noderfIsamAutoInc) NodeRootField noderf )
//  ================================================================
{
    Assert( noderf > 0 );
    Assert( noderf < noderfMax );
    return ( 0x1 << ( noderf - 1 ) );
}

//  ================================================================
VOID NDGetPtrExternalHeader( _In_ const CPAGE& cpage, _Out_ LINE * pline, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderfRequested )
//  ================================================================
{
#ifdef DEBUG
    if ( !FNDIIsExternalHeaderUpgradable( cpage ) )
    {
        Assert( noderfRequested == noderfWhole );
    }
    else
    {
        Assert( cpage.FRootPage() && !cpage.FSpaceTree() );
        // Assert it's not noderfWhole or it's recovering but recovering check need INST...
        // Assert( noderfRequested != noderfWhole || 
    }
#endif //DEBUG

    Assert( noderfMax > noderfRequested );
    Assert( noderfWhole <= noderfRequested );

    cpage.GetPtrExternalHeader( pline );

    if ( noderfRequested == noderfWhole )
    {
        return;
    }

    BYTE fStoredFlag = 0;
    BYTE* pbCursor = (BYTE*)pline->pv;
    switch ( pline->cb )
    {
        case 0:
            Assert( fFalse ); // To test if this happens
            break; // flag 0 will be used
        case sizeof(SPACE_HEADER):
            fStoredFlag = BNDIGetPersistedNrfFlag( noderfSpaceHeader ); // make up a flag for below
            break;
        default:
            Assert( pline->cb > sizeof(SPACE_HEADER) );
            fStoredFlag = *pbCursor;
            ++pbCursor;
            break;
    }

    // If caller is requesting something doesn't exist in external header,
    // NULL pointer and size 0 are returned
    const BYTE fRequestFlag = BNDIGetPersistedNrfFlag( noderfRequested );

    if ( ( fStoredFlag & fRequestFlag ) != fRequestFlag )
    {
        pline->pv = NULL;
        pline->cb = 0;
        return;
    }

    // Assert because we are doing the cast in the for clause below.
    Assert( sizeof(NodeRootField) == sizeof(INT) );
    for( NodeRootField noderf = noderfSpaceHeader; noderf < noderfRequested; ++(INT&)noderf )
    {
        if ( fStoredFlag & BNDIGetPersistedNrfFlag( noderf ) )
        {
            pbCursor += g_rgcbExternalHeaderSize[noderf];
        }
    }
    AssertRTL( g_rgcbExternalHeaderSize[noderfRequested] <= pline->cb );
    AssertRTL( pbCursor >= (BYTE*)pline->pv );
    const BYTE* pbLineEnd = (BYTE*)pline->pv + pline->cb;
    OnNonRTM( pbLineEnd );
    AssertRTL( pbCursor + g_rgcbExternalHeaderSize[noderfRequested] <= pbLineEnd );
    pline->cb = g_rgcbExternalHeaderSize[noderfRequested];
    pline->pv = pbCursor;
}

//  ================================================================
VOID NDGetExternalHeader( _In_ KEYDATAFLAGS * const pkdf, _In_ const CSR * pcsr, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderfRequested )
//  ================================================================
{
    ASSERT_VALID( pcsr );

    LINE line;
    NDGetPtrExternalHeader( pcsr->Cpage(), &line, noderfRequested );
    pkdf->key.Nullify();
    pkdf->data.SetCb( line.cb );
    pkdf->data.SetPv( line.pv );
    pkdf->fFlags = 0;
}


//  ================================================================
VOID NDGetExternalHeader( _In_ FUCB* const pfucb, _In_ const CSR * pcsr, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderfRequested )
//  ================================================================
{
    ASSERT_VALID( pfucb );
    NDGetExternalHeader( &pfucb->kdfCurr, pcsr, noderfRequested );
}


//  ================================================================
VOID NDGetPrefix( FUCB * pfucb, const CSR * pcsr )
//  ================================================================
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( pcsr );

    LINE line;
    pcsr->Cpage().GetPtrExternalHeader( &line );

    pfucb->kdfCurr.Nullify();
    pfucb->kdfCurr.fFlags = 0;

    if ( !pcsr->Cpage().FRootPage() )
    {
        pfucb->kdfCurr.key.prefix.SetPv( line.pv );
        pfucb->kdfCurr.key.prefix.SetCb( line.cb );
    }
    else if ( pcsr->Cpage().FSpaceTree() )
    {
        if ( line.cb >= sizeof( SPLIT_BUFFER ) )
        {
            pfucb->kdfCurr.key.prefix.SetPv( (BYTE *)line.pv + sizeof( SPLIT_BUFFER ) );
            pfucb->kdfCurr.key.prefix.SetCb( line.cb - sizeof( SPLIT_BUFFER ) );
        }
        else
        {
            Assert( 0 == line.cb );
            Assert( FFUCBSpace( pfucb ) );

/*  Can't make this assertion because sometimes we are calling this code without having
    setup the SPLIT_BUFFER (eg. merge of the space tree itself)
            //  split buffer may be missing from page if upgrading from ESE97 and it couldn't
            //  fit on the page, in which case it should be hanging off the FCB
            Assert( NULL != pfucb->u.pfcb->Psplitbuf( FFUCBAvailExt( pfucb ) )
                || PinstFromPfucb( pfucb )->FRecovering() );
*/
            pfucb->kdfCurr.key.prefix.SetPv( line.pv );
            pfucb->kdfCurr.key.prefix.SetCb( 0 );
        }
    }
    else
    {
        pfucb->kdfCurr.key.prefix.SetPv( NULL );
        pfucb->kdfCurr.key.prefix.SetCb( 0 );
    }
    return;
}


//  ================================================================
ERR LOCAL ErrNDISetExternalHeader( _In_ FUCB* const pfucb, _In_ const IFMP ifmp, _In_ CSR * pcsr, _In_ const DATA * pdata, _In_ const DIRFLAG dirflag, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderfRequested )
//  ================================================================
{
    Assert( ifmp != ifmpNil );
    Assert( ( pfucb == pfucbNil ) || ( pfucb->ifmp == ifmp ) );

    if ( pfucb != pfucbNil )
    {
        NDIAssertCurrency( pfucb, pcsr );
    }
    else
    {
        NDIAssertCurrency( pcsr );
    }
    ASSERT_VALID( pdata );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );

    Assert( noderfRequested < noderfMax );

    ERR         err             = JET_errSuccess;
    const BOOL  fLogging        = ( !( dirflag & fDIRNoLog ) ) && g_rgfmp[ifmp].FLogOn();
    const BOOL  fDirty          = !( dirflag & fDIRNoDirty );
    DATA        newData;
    BYTE        pbBuffer[32];

    Assert( !fLogging || ( pfucb != pfucbNil ) );

#ifdef DEBUG
    if ( !FNDIIsExternalHeaderUpgradable( pcsr->Cpage() ) )
    {
        Assert( noderfRequested == noderfWhole );
    }
#endif // DEBUG

    if ( noderfRequested != noderfWhole )
    {
        EnforceSz( pdata->Cb() == (INT)g_rgcbExternalHeaderSize[noderfRequested], "InvalidExtHdrSizeUpdate" );
    }

    if ( !fLogging && fDirty )
    {
        pcsr->Dirty( );
    }

    LINE line;
    pcsr->Cpage().GetPtrExternalHeader( &line );

    // If caller intend to set the whole line
    // Or if it's legacy format, and we are not expanding it to new format
    if ( noderfRequested == noderfWhole ||
        noderfRequested == noderfSpaceHeader && ( line.cb == sizeof(SPACE_HEADER) || line.cb == 0 ) )
    {
        if ( fLogging )
        {
            //  log the operation, getting the lgpos
            //
            LGPOS   lgpos;
            Call( ErrLGSetExternalHeader( pfucb, pcsr, *pdata, &lgpos, fMustDirtyCSR ) );
            pcsr->Cpage().SetLgposModify( lgpos );
        }

        pcsr->Cpage().SetExternalHeader( pdata, 1, 0 ); //  external header has no flags -- yet
        return JET_errSuccess;
    }

    Assert( noderfRequested != noderfWhole );
    Assert( pcsr->Cpage().FRootPage() && !pcsr->Cpage().FSpaceTree() );

    BYTE* pb = (BYTE*)line.pv;
    BYTE fStoredFlag = 0;
    if ( line.cb == sizeof(SPACE_HEADER) )
    {
        // upgrading a solitary SPACE_HEADER to have root fields array in-front (presumably b/c we now want to store 2 or more fields).
        fStoredFlag = BNDIGetPersistedNrfFlag( noderfSpaceHeader );
    }
    else if ( line.cb == 0 )
    {
        fStoredFlag = 0;
    }
    else
    {
        fStoredFlag = *pb;
        ++pb;
    }

    // Reserve the first byte for field flags.
    //
    // pbBuffer is the whole extended external header. The first byte is used for flags, indicating which pieces of info is stored
    // in the extended external header. e.g.: 0x1 for SpaceHeader, 0x10 for AutoInc. 0x11 for both. 0x0100 for next coming piece...
    // See comments for the definition of NodeRootField for some more explanation.
    BYTE* pbCursorDst = pbBuffer + 1;
    BYTE* pbCursorSrc = pb;
    // Assert because we are doing the cast in the for clause below.
    Assert( sizeof(NodeRootField) == sizeof(INT) );
    for( NodeRootField noderf = noderfSpaceHeader; noderf < noderfRequested; ++(INT&)noderf )
    {
        if ( fStoredFlag & BNDIGetPersistedNrfFlag( noderf ) )
        {
            pbCursorSrc += g_rgcbExternalHeaderSize[noderf];
            AssertRTL( (BYTE*)line.pv + line.cb >= pbCursorSrc );
        }
    }

    const ULONG cbPrecedingStoredFields = (ULONG)(pbCursorSrc - pb);
    if ( cbPrecedingStoredFields > 0 )
    {
        Assert( _countof(pbBuffer) >= cbPrecedingStoredFields + 1 );
        if ( _countof(pbBuffer) < cbPrecedingStoredFields + 1 )
        {
            Error( ErrERRCheck( JET_errInvalidBufferSize ) );
        }
        UtilMemCpy( pbCursorDst, pb, cbPrecedingStoredFields );
        pbCursorDst += cbPrecedingStoredFields;
        AssertRTL( pbCursorDst - (BYTE*)pbBuffer < _countof(pbBuffer) );
    }

    AssertRTL( pdata->Cb() == (INT)g_rgcbExternalHeaderSize[noderfRequested] );
    Assert( _countof(pbBuffer) >= cbPrecedingStoredFields + g_rgcbExternalHeaderSize[noderfRequested] + 1 );
    if ( _countof(pbBuffer) < cbPrecedingStoredFields + g_rgcbExternalHeaderSize[noderfRequested] + 1 )
    {
        Error( ErrERRCheck( JET_errInvalidBufferSize ) );
    }
    UtilMemCpy( pbCursorDst, pdata->Pv(), g_rgcbExternalHeaderSize[noderfRequested] );
    const BYTE fRequestFlag = BNDIGetPersistedNrfFlag( noderfRequested );
    if ( fRequestFlag & fStoredFlag )
    {
        pbCursorSrc += g_rgcbExternalHeaderSize[noderfRequested];
    }
    pbCursorDst += g_rgcbExternalHeaderSize[noderfRequested];
    AssertRTL( pbCursorDst - (BYTE*)pbBuffer <= _countof(pbBuffer) );

    const SIZE_T cbPostcedingStoredFields = (BYTE*)line.pv + line.cb - pbCursorSrc;
    if ( cbPostcedingStoredFields > 0 )
    {
        UtilMemCpy( pbCursorDst, pbCursorSrc, cbPostcedingStoredFields );
        pbCursorDst += cbPostcedingStoredFields;
        AssertRTL( pbCursorDst - (BYTE*)pbBuffer <= _countof(pbBuffer) );
    }

    fStoredFlag |= fRequestFlag;
    *pbBuffer = fStoredFlag;

    newData.SetPv( pbBuffer );
    newData.SetCb( pbCursorDst - (BYTE*)pbBuffer );

    if ( fLogging )
    {
        //  log the operation, getting the lgpos
        //
        LGPOS lgpos;
        Call( ErrLGSetExternalHeader( pfucb, pcsr, newData, &lgpos, fMustDirtyCSR ) );
        pcsr->Cpage().SetLgposModify( lgpos );
    }

    pcsr->Cpage().SetExternalHeader( &newData, 1, 0 ); //  external header has no flags -- yet

HandleError:
    // WARNING: some callers depend on this never failing if fLogging is false.
    Assert( fLogging || ( err >= JET_errSuccess ) );
    return err;
}


//  ================================================================
ERR ErrNDSetExternalHeader( _In_ FUCB* const pfucb, _In_ CSR * pcsr, _In_ const DATA * pdata, _In_ const DIRFLAG dirflag, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderfRequested )
//  ================================================================
{
    return ErrNDISetExternalHeader( pfucb, pfucb->ifmp, pcsr, pdata, dirflag, noderfRequested );
}

//  ================================================================
LOCAL VOID NDISetExternalHeader( _In_ FUCB* const pfucb, _In_ const IFMP ifmp, _In_ CSR * pcsr, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderfRequested, _In_ const DATA * pdata )
//  ================================================================
{
    Assert( pcsr->FDirty() );
    Expected( noderfRequested == noderfSpaceHeader
            || noderfRequested == noderfWhole && pdata->Cb() == sizeof(SPLIT_BUFFER) );

    // If no logging, there shouldn't be strange error come out. OOM might be the only one.
    CallS( ErrNDISetExternalHeader( pfucb, ifmp, pcsr, pdata, fDIRNoLog | fDIRNoDirty, noderfRequested ) );
    return;
}


//  ================================================================
//  This function doesn't log data, so recovery will not be able to
//  replay this data if you use this API, and you probably
//  want ErrNDSetExternalHeader().
VOID NDSetExternalHeader( _In_ FUCB* const pfucb, _In_ CSR * pcsr, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderfRequested, _In_ const DATA * pdata )
//  ================================================================
{
    NDISetExternalHeader( pfucb, pfucb->ifmp, pcsr, noderfRequested, pdata );
}


//  ================================================================
//  This function doesn't log data, so recovery will not be able to
//  replay this data if you use this API, and you probably
//  want ErrNDSetExternalHeader().
VOID NDSetExternalHeader( _In_ const IFMP ifmp, _In_ CSR* const pcsr, _In_range_( 0, noderfMax - 1 ) const NodeRootField noderf, _In_ const DATA * pdata )
//  ================================================================
{
    NDISetExternalHeader( pfucbNil, ifmp, pcsr, noderf, pdata );
}


//  ================================================================
ERR ErrNDValidateSetExternalHeader( const CPAGE &cpage, _In_ const DATA * pdata )
//  ================================================================
{
    if ( cpage.FSpaceTree() || cpage.FRootPage() )
    {
        return JET_errSuccess;
    }

    const INT clines = cpage.Clines();
    for ( INT iline = 0; iline < clines; iline++ )
    {
        KEYDATAFLAGS kdfNode;
        NDIGetKeydataflags( cpage, iline, &kdfNode );
        if ( kdfNode.key.prefix.Cb() > pdata->Cb() )
        {
            return ErrERRCheck( JET_errLogOperationInconsistentWithDatabase );
        }
    }

    return JET_errSuccess;
}


///  ================================================================
LOCAL VOID NDScrubOneUsedPage(
        _In_ CSR * const pcsr,
        __in_ecount(cscrubOper) const SCRUBOPER * const rgscrubOper,
        const INT cscrubOper)
//  ================================================================
{
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( pcsr->FDirty( ) );
    Assert( pcsr->Cpage().FScrubbed() );
    Enforce( cscrubOper == pcsr->Cpage().Clines() );

    for( INT iline = 0; iline < cscrubOper; ++iline )
    {
        KEYDATAFLAGS kdf;
        NDIGetKeydataflags( pcsr->Cpage(), iline, &kdf );
        
        switch( rgscrubOper[iline] )
        {
            case scrubOperNone:
                break;
            case scrubOperScrubData:
                Assert( FNDDeleted( kdf ) );
                memset( kdf.data.Pv(), chSCRUBDBMaintDeletedDataFill, kdf.data.Cb() );
                pcsr->Cpage().ReplaceFlags( iline, kdf.fFlags & ~fNDVersion );
                break;
            case scrubOperScrubLVData:
                //  Bad interaction between scrubbing and compression
                //  Scrubbing compressed LV chunks causes the integrity check to fail
                //  when it inspects the LV. We'll sidestep that problem by overwriting LVs
                //  with a pattern that is recognizable by the compression code.
                //  We will use dummy compression regardless of the chunk being actually
                //  compressed or not. The reason for this is because when this function
                //  is called from the log-redo code, there is not context to figure out
                //  what the original size of the chunk is, so we can't determine whether
                //  it is compressed or not.
                CallS( ErrRECScrubLVChunk( kdf.data, chSCRUBDBMaintLVChunkFill ) );
                pcsr->Cpage().ReplaceFlags( iline, kdf.fFlags & ~fNDVersion );
                break;
            default:
                EnforceSz( fFalse, OSFormat( "UnknownScrubOper:%d", (INT)rgscrubOper[iline] ) );
                break;
        }
    }

    //  reorganize the page and zero all unused data
    pcsr->Cpage().OverwriteUnusedSpace( chSCRUBDBMaintUsedPageFreeFill );
}


//  ================================================================
ERR ErrNDScrubOneUsedPage(
        _In_ PIB * const ppib,
        const IFMP ifmp,
        _In_ CSR * const pcsr,
        __in_ecount(cscrubOper) const SCRUBOPER * const rgscrubOper,
        const INT cscrubOper,
        const DIRFLAG dirflag)
//  ================================================================
{
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( g_rgfmp[ifmp].FAttachedForRecovery() || BoolParam( PinstFromIfmp( ifmp ), JET_paramZeroDatabaseUnusedSpace ) );

    ERR         err         = JET_errSuccess;
    const BOOL  fLogging    = !( dirflag & fDIRNoLog ) && g_rgfmp[ifmp].FLogOn();
    PIBTraceContextScope tcScrub = ppib->InitTraceContextScope();
    tcScrub->iorReason.SetIort( iortScrubbing );

    if ( fLogging )
    {
        //  log the operation, getting the lgpos
        LGPOS   lgpos;
        Call( ErrLGScrub( ppib, ifmp, pcsr, fFalse, rgscrubOper, cscrubOper, &lgpos ) );
        pcsr->Cpage().SetLgposModify( lgpos );
    }
    else
    {
        pcsr->DirtyForScrub( );
    }

    NDScrubOneUsedPage( pcsr, rgscrubOper, cscrubOper );

HandleError:
    return err;
}


//  ================================================================
LOCAL VOID NDScrubOneUnusedPage( _In_ CSR * const pcsr )
//  ================================================================
{
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( pcsr->FDirty( ) );
    Assert( pcsr->Cpage().FScrubbed() );

    //  Delete all the data on the page
    const INT clines = pcsr->Cpage().Clines();
    for( INT iline = 0; iline < clines; ++iline )
    {
        pcsr->Cpage().Delete( 0 );
    }
    
    //  reorganize the page and zero all unused data
    pcsr->Cpage().OverwriteUnusedSpace( chSCRUBDBMaintUnusedPageFill );
}


//  ================================================================
ERR ErrNDScrubOneUnusedPage(
        _In_ PIB * const ppib,
        const IFMP ifmp,
        _In_ CSR * const pcsr,
        const DIRFLAG dirflag)
//  ================================================================
{
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( g_rgfmp[ifmp].FAttachedForRecovery() || BoolParam( PinstFromIfmp( ifmp ), JET_paramZeroDatabaseUnusedSpace ) );

    ERR         err         = JET_errSuccess;
    const BOOL  fLogging    = !( dirflag & fDIRNoLog ) && g_rgfmp[ifmp].FLogOn();
    PIBTraceContextScope tcScrub = ppib->InitTraceContextScope();
    tcScrub->iorReason.SetIort( iortScrubbing );

    if ( fLogging )
    {
        //  log the operation, getting the lgpos
        LGPOS   lgpos;
        Call( ErrLGScrub( ppib, ifmp, pcsr, fTrue, NULL, 0, &lgpos ) );
        pcsr->Cpage().SetLgposModify( lgpos );
    }
    else
    {
        pcsr->DirtyForScrub( );
    }

    NDScrubOneUnusedPage( pcsr );

HandleError:
    return err;
}


//  ================================================================
VOID NDSetPrefix( CSR * pcsr, const KEY& key )
//  ================================================================
{
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( pcsr->FDirty() );

    //  SOMEONE: if this is the source page of the split, I don't
    //  follow how we can guarantee this assert won't go off.
    //  Ultimately, it's probably okay because the cbUncommittedFree
    //  will eventually be set to the right value in BTISplitMoveNodes().
    Assert( pcsr->Cpage().CbPageFree( ) - pcsr->Cpage().CbUncommittedFree( ) >= key.Cb() );

    DATA            rgdata[cdataPrefix];
    INT             idata = 0;

    BYTE            rgexthdr[ max( sizeof(SPLIT_BUFFER), sizeof(SPACE_HEADER) ) ];

    if ( pcsr->Cpage().FRootPage() )
    {
        if ( key.FNull() )
            return;

        //  should currently be a dead code path because we never append prefix data to
        //  the SPLIT_BUFFER or SPACE_HEADER
        //  UNDONE: code below won't even work properly because it doesn't properly
        //  handle the case of the SPLIT_BUFFER being cached in the FCB
        Assert( fFalse );

        //  copy space header and reinsert
        const ULONG cbExtHdr    = ( pcsr->Cpage().FSpaceTree() ? sizeof(SPLIT_BUFFER) : sizeof(SPACE_HEADER) );
        LINE        line;

        pcsr->Cpage().GetPtrExternalHeader( &line );

        if ( 0 != line.cb )
        {
            //  don't currently support prefix data following SPLIT_BUFFER/SPACE_HEADER
            Assert( line.cb == cbExtHdr );

            UtilMemCpy( rgexthdr, line.pv, cbExtHdr );

            rgdata[idata].SetPv( rgexthdr );
            rgdata[idata].SetCb( cbExtHdr );
            ++idata;
        }
        else
        {
            //  SPLIT_BUFFER must be cached in the FCB
            Assert( pcsr->Cpage().FSpaceTree() );
        }
    }

    rgdata[idata] = key.prefix;
    ++idata;
    rgdata[idata] = key.suffix;
    ++idata;

    pcsr->Cpage().SetExternalHeader( rgdata, idata, 0 ); //  external header has no flags -- yet
}


//  ================================================================
VOID NDGrowCbPrefix( const FUCB *pfucb, CSR * pcsr, INT cbPrefixNew )
//  ================================================================
//
//  grows cbPrefix in current node to cbPrefixNew -- thereby shrinking node
//
//-
{
    NDIAssertCurrency( pfucb, pcsr );
    AssertNDGet( pfucb, pcsr );
    Assert( pfucb->kdfCurr.key.prefix.Cb() < cbPrefixNew );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( pcsr->FDirty() );

    KEYDATAFLAGS    kdf = pfucb->kdfCurr;

    //  set node as compressed
    Assert( cbPrefixNew > cbPrefixOverhead );
    NDISetCompressed( kdf );

    DATA        rgdata[cdataKDF+1];
    INT         fFlagsLine;
    LE_KEYLEN   le_keylen;
    le_keylen.le_cbPrefix = (USHORT)cbPrefixNew;

    //  replace node with new key
    const INT   cdata           = CdataNDIPrefixAndKeydataflagsToDataflags(
                                            &kdf,
                                            rgdata,
                                            &fFlagsLine,
                                            &le_keylen );
    Assert( cdata <= cdataKDF + 1 );
    pcsr->Cpage().Replace( pcsr->ILine(), rgdata, cdata, fFlagsLine );
}


//  ================================================================
VOID NDShrinkCbPrefix( FUCB *pfucb, CSR * pcsr, const DATA *pdataOldPrefix, INT cbPrefixNew )
//  ================================================================
//
//  shrinks cbPrefix in current node to cbPrefixNew -- thereby growing node
//  expensive operation -- perf needed
//
//-
{
    NDIAssertCurrency( pfucb, pcsr );
    AssertNDGet( pfucb, pcsr );
    Assert( pfucb->kdfCurr.key.prefix.Cb() > cbPrefixNew );
    Assert( pcsr->Cpage().FAssertWriteLatch( ) );
    Assert( pcsr->FDirty() );

    KEYDATAFLAGS    kdf = pfucb->kdfCurr;

    //  set flags in kdf to compressed
    //  point prefix to old prefix [which has been deleted]
    if ( cbPrefixNew > 0 )
    {
        Assert( cbPrefixNew > cbPrefixOverhead );
        NDISetCompressed( kdf );
    }
    else
    {
        NDIResetCompressed( kdf );
    }

    //  fix prefix to point to old prefix
    kdf.key.prefix.SetPv( pdataOldPrefix->Pv() );
    Assert( (INT)pdataOldPrefix->Cb() >= kdf.key.prefix.Cb() );
    Assert( kdf.key.prefix.Cb() == pfucb->kdfCurr.key.prefix.Cb() );

    BYTE *rgb;
//  BYTE    rgb[g_cbPageMax];
    BFAlloc( bfasTemporary, (VOID **)&rgb );

    kdf.key.suffix.SetPv( rgb );
    pfucb->kdfCurr.key.suffix.CopyInto( kdf.key.suffix );

    kdf.data.SetPv( rgb + kdf.key.suffix.Cb() );
    pfucb->kdfCurr.data.CopyInto( kdf.data );

    DATA        rgdata[cdataKDF+1];
    INT         fFlagsLine;
    LE_KEYLEN   le_keylen;
    le_keylen.le_cbPrefix = (USHORT)cbPrefixNew;

    //  replace node with new key
    const INT   cdata           = CdataNDIPrefixAndKeydataflagsToDataflags(
                                            &kdf,
                                            rgdata,
                                            &fFlagsLine,
                                            &le_keylen );
    Assert( cdata <= cdataKDF + 1 );
    pcsr->Cpage().Replace( pcsr->ILine(), rgdata, cdata, fFlagsLine );

    BFFree( rgb );
}


//  ================================================================
INT CbNDUncommittedFree( const FUCB * const pfucb, const CSR * const pcsr )
//  ================================================================
{
    ASSERT_VALID( pfucb );

    Assert( !pcsr->Cpage().FSpaceTree() );

    LONG    cbActualUncommitted     = 0;

    if ( pcsr->FPageRecentlyDirtied( pfucb->ifmp ) )
    {
        for ( INT iline = 0; iline < pcsr->Cpage().Clines( ); iline++ )
        {
            KEYDATAFLAGS keydataflags;
            NDIGetKeydataflags( pcsr->Cpage(), iline, &keydataflags );

            if ( FNDVersion( keydataflags ) )
            {
                BOOKMARK bookmark;
                NDIGetBookmark( pcsr->Cpage(), iline, &bookmark, FFUCBUnique( pfucb ) );

                cbActualUncommitted += CbVERGetNodeReserve( ppibNil, pfucb, bookmark, keydataflags.data.Cb() );
            }
        }
    }
    Assert( cbActualUncommitted >= 0 );
    Assert( cbActualUncommitted <= pcsr->Cpage().CbPageFree() );

    return cbActualUncommitted;
}


//  ****************************************************************
//  DEBUG ROUTINES
//  ****************************************************************

#ifdef DEBUG

//  ================================================================
INLINE VOID NDIAssertCurrency( const CSR * pcsr )
//  ================================================================
{
    ASSERT_VALID( pcsr );
    Assert( pcsr->FLatched() );
}


//  ================================================================
INLINE VOID NDIAssertCurrency( const FUCB * pfucb, const CSR * pcsr )
//  ================================================================
{
    ASSERT_VALID( pfucb );
    NDIAssertCurrency( pcsr );
}


//  ================================================================
INLINE VOID NDIAssertCurrencyExists( const FUCB * pfucb, const CSR * pcsr )
//  ================================================================
{
    ASSERT_VALID( pfucb );
    NDIAssertCurrencyExists( pcsr );
}


//  ================================================================
INLINE VOID NDIAssertCurrencyExists( const CSR * pcsr )
//  ================================================================
{
    NDIAssertCurrency( pcsr );
    Assert( pcsr->ILine() >= 0 );
    Assert( pcsr->ILine() < pcsr->Cpage().Clines( ) );
}


//  ================================================================
VOID NDAssertNDInOrder( const FUCB * pfucb, const CSR * pcsr )
//  ================================================================
//
//  assert nodes in page are in bookmark order
//
//-
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( pcsr );

    const INT   clines  = pcsr->Cpage().Clines( );

    //  we could optimize this loop by only fetching one node each time
    INT         iline   =   0;
    for ( ;iline < (clines - 1); iline++ )
    {
        BOOKMARK    bmLess;
        BOOKMARK    bmGreater;

        NDIGetBookmark( pcsr->Cpage(), iline, &bmLess, FFUCBUnique( pfucb ) );
        NDIGetBookmark( pcsr->Cpage(), iline+1, &bmGreater, FFUCBUnique( pfucb ) );

        if ( bmGreater.key.FNull() )
        {
            //  if key is null, then must be last node in internal page
            Assert( !pcsr->Cpage().FLeafPage() || FFUCBRepair( pfucb ) );
            Assert( iline + 1 == clines - 1 );
            continue;
        }

        Assert( !bmLess.key.FNull() );
        const INT cmp = pcsr->Cpage().FLeafPage() ?
                            CmpBM( bmLess, bmGreater ) :
                            CmpKey( bmLess.key, bmGreater.key );
        if (    cmp > 0
                || ( FFUCBUnique( pfucb ) && 0 == cmp ) )
        {
            AssertSz( fFalse, "NDAssertNDInOrder: node is out of order" );
        }
    }
}

void UtilCorruptShort( UnalignedLittleEndian<USHORT> * pule_us, const INT cbNodeSize, const JET_GRBIT grbitNodeSpecificCorruption )
{
    switch( grbitNodeSpecificCorruption )
    {
        case JET_bitTestHookCorruptSizeLargerThanNode:
            *pule_us += USHORT( cbNodeSize + 1 );
            break;

        case JET_bitTestHookCorruptSizeShortWrapSmall:
            {
                const USHORT us = *pule_us;
                *pule_us = us + 0x8000;
            }
            break;

        case JET_bitTestHookCorruptSizeShortWrapLarge:
            {
                const USHORT us = *pule_us;
                *pule_us = us + 0xF000;
            }
            break;

        default:
            break;
    }
}

void NDCorruptNodePrefixSize( const CPAGE& cpage, const INT iline, const JET_GRBIT grbitNodeCorruption, const USHORT usOffsetFixed )
{
    Assert( 0 == ( grbitNodeCorruption & ~JET_mskTestHookCorruptSpecific ) );
    LINE lineToCorrupt;
    Assert( cpage.Clines() > iline );
    // Must now use Err'd version to allow us to do bad things.
    cpage.ErrGetPtr( iline, &lineToCorrupt );
    KEYDATAFLAGS kdf;
    ErrNDIGetKeydataflags( cpage, iline, &kdf );
    Assert( FNDCompressed( kdf ) );
    UnalignedLittleEndian<USHORT> * pule_us = (UnalignedLittleEndian<USHORT>*)( lineToCorrupt.pv );
    LINE linePrefix;
    cpage.GetPtrExternalHeader( &linePrefix );
    if ( usOffsetFixed == 0 )
    {
        UtilCorruptShort( &( pule_us[0] ), linePrefix.cb, grbitNodeCorruption );
    }
    else
    {
        const USHORT us = pule_us[ 0 ];
        pule_us[ 0 ] = us + usOffsetFixed;
    }
}

void NDCorruptNodeSuffixSize( const CPAGE& cpage, const INT iline, const JET_GRBIT grbitNodeCorruption, const USHORT usOffsetFixed )
{
    Assert( 0 == ( grbitNodeCorruption & ~JET_mskTestHookCorruptSpecific ) );
    LINE lineToCorrupt;
    Assert( cpage.Clines() > iline );
    // Must now use Err'd version to allow us to do bad things.
    (void)cpage.ErrGetPtr( iline, &lineToCorrupt );
    KEYDATAFLAGS kdf;
    const bool fPreviouslySet = FNegTestSet( fCorruptingPageLogically );    //  this is b/c we may have corrupted things before this with pcorrupt->grbit & JET_bitTestHookCorruptNodePrefix
    (void)ErrNDIGetKeydataflags( cpage, iline, &kdf );
    if ( !fPreviouslySet )
    {
        FNegTestUnset( fCorruptingPageLogically );
    }
    UnalignedLittleEndian<USHORT> * pule_us = (UnalignedLittleEndian<USHORT>*)( lineToCorrupt.pv );
    if ( usOffsetFixed == 0 )
    {
        UtilCorruptShort( &( pule_us[ FNDCompressed( kdf ) ? 1 : 0 ] ), lineToCorrupt.cb, grbitNodeCorruption );
    }
    else
    {
        const USHORT us = *pule_us;
        pule_us[ FNDCompressed( kdf ) ? 1 : 0 ] = us + usOffsetFixed;
    }
}

//  This corrupts a random element that cpage.ErrCheckPage( ...CheckLineBoundedByTag ) will be 
//  able to catch.

BOOL FNDCorruptRandomNodeElement( CPAGE * const pcpage )
{
    if ( pcpage->FSpaceTree() )
    {
        //  Doesn't necessarily have nodes with normal prefixes and suffixes.
        return fFalse;
    }

    const INT clines = pcpage->Clines();
    const INT ilineSelected = clines > 1 ?
                                ( 1 + rand() % ( clines - 1 ) ) :
                                ( clines /* no usable tags / lines */ );
    if ( ilineSelected < pcpage->Clines() )
    {
        Assert( ilineSelected > 0 && ilineSelected < pcpage->Clines() );
        // ignore return, b/c page may already be corrupted before this.
        LINE line;
        (void)pcpage->ErrGetPtr( ilineSelected, &line );
        KEYDATAFLAGS kdf;
        (void)ErrNDIGetKeydataflags( *pcpage, ilineSelected, &kdf );

        const INT iselection = rand() % ( FNDCompressed( kdf ) ? 2 : 1 );
        switch( iselection )
        {
            case 0:
                NDCorruptNodeSuffixSize( *pcpage, ilineSelected, JET_bitTestHookCorruptSizeLargerThanNode );
                break;
            case 1:
                NDCorruptNodePrefixSize( *pcpage, ilineSelected, JET_bitTestHookCorruptSizeLargerThanNode );
                break;

        default:
            AssertSz( fFalse, "Bad code" );
        }
        //  Check this to ensure code that depends upon this, see the expected error it expects.
        Assert( JET_errDatabaseCorrupted == pcpage->ErrCheckPage( CPRINTFDBGOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) );
        return fTrue;
    }

    //  couldn't find anything to corrupt           
    return fFalse;
}

#endif      //  DEBUG

