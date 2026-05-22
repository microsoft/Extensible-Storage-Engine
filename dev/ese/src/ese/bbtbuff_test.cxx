// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "bbtbuffwriter.hxx"

#ifndef ENABLE_JET_UNIT_TEST
#error This file should only be compiled with the unit tests!
#endif // !ENABLE_JET_UNIT_TEST

#define CHECK_CALL( expr ) CHECK( JET_errSuccess == ( expr ) )

class BBTBuffTrxLog_Test : public IBBTBuffTrxLog<BBTBuffTrxLog_Test>
{
public:
    ERR m_returnErr = JET_errSuccess;   // error to return from trx log for testing

    BBTBuffTrxLog_Test( BBTBuff* pbbtbuff ) : IBBTBuffTrxLog( pbbtbuff )
    {}

    ERR ErrLogInsert(
        const BBTBuffChangeContext& changeCtx,
        BBTBuffOpcode               opcode,
        const KEY&                  key,
        const DATA&                 data,
        SkipListNodeFlags           flags )
    {
        return m_returnErr;
    }

    ERR ErrLogDelete(
        const BBTBuffChangeContext& changeCtx,
        const SkipListNode*         pnodeDel )
    {
        return m_returnErr;
    }

    ERR ErrLogRangeDelete(
        DBTIME          dbtimeBefore,
        DBTIME          dbtimeCurr,
        SkipListLink    linkFirst,
        int             cNodes )
    {
        return m_returnErr;
    }

    ERR ErrLogMergeAndDel(
        DBTIME                  dbtimeBefore,
        DBTIME                  dbtimeCurr,
        const SkipListNode**    rgNodesMerged,
        SkipListLink*           rgLinksDel,
        int                     cNodesMerged,
        int                     cNodesDel,
        SkipListLink            linkIbMergeStart )
    {
        return m_returnErr;
    }
};

using BBTBuffWriter_Test = BBTBuffWriter<BBTBuffTrxLog_Test>;

class BBTBuffTestFixture
{
public:
    CSRHeapArray        m_rgcsr;
    BBTBuff             m_bbtBuff;
    BBTBuffTrxLog_Test  m_bbtBuffTrxLogger;
    BBTBuffWriter_Test  m_bbtBuffWriter;
    BBTBuffHeader*      m_pbbtHeader;

    BBTBuffTestFixture() :
        m_rgcsr( CSRHeapArray::MakeArray( BBTBuff::PBBTBuffFormatForPage( g_cbPage )->cpgInBBTBuff ) ),
        m_bbtBuffTrxLogger( &m_bbtBuff ),
        m_bbtBuffWriter( &m_bbtBuff, &m_bbtBuffTrxLogger )
    {
        m_rgcsr[ 0 ].LoadNewTestPage( g_cbPage, ifmpNil );
        m_rgcsr[ 0 ].Cpage().SetFlags( m_rgcsr[ 0 ].Cpage().FFlags() | CPAGE::fPageBBTBuffRoot | CPAGE::fPageBBTBuff );
        BBTBuff::InitBBTBuffRoot( NULL, &m_rgcsr[ 0 ] );

        for ( int i = 1; i < m_rgcsr.CItems(); i++ )
        {
            m_rgcsr[ i ].LoadNewTestPage( g_cbPage, ifmpNil, m_rgcsr[ 0 ].Pgno() + i );
            m_rgcsr[ i ].Cpage().SetFlags( m_rgcsr[ i ].Cpage().FFlags() | CPAGE::fPageBBTBuff );
            BBTBuff::InitBBTBuffNonRoot( &m_rgcsr[ i ] );
        }

        LINE line;
        BBTBuff::GetBBTBuffRoot( m_rgcsr[ 0 ], &line );
        m_pbbtHeader = BBTBuff::PBBTHeader( line );
        m_bbtBuff.Load( NULL, ifmpNil, &m_rgcsr[ 0 ], CSRHeapArray( m_rgcsr.Subarray( 1 ) ), m_pbbtHeader, latchWrite );
    }

    ERR ErrHotpointInsert( int iKeyStep, int cHotpoints, int level = -1 )
    {
        ERR             err;
        BigEndian<int>  be_key = iKeyStep;
        int             iData = 0;
        KEY             key;
        DATA            data;

        key.Nullify();
        key.suffix.SetPv( &be_key );
        key.suffix.SetCb( sizeof( be_key ) );
        data.Nullify();
        data.SetPv( &iData );
        data.SetCb( sizeof( iData ) );

        m_bbtBuffTrxLogger.m_dbtimeCoordinated = dbtimeStart + 10;
        CallR( m_bbtBuffWriter.ErrUpgradeToWriteMode() );

        int iKeyBound = ( cHotpoints + 1 ) * iKeyStep;
        do
        {
            m_bbtBuff.SetLevelSeed( level );    // override next node level for testing (-1 means no override)
            err = m_bbtBuffWriter.ErrInsert( bbtOpInsert, key, data, fSLNNone );
            be_key = be_key % iKeyBound;
            be_key += iKeyStep;
            iData += ( be_key == iKeyStep ? 1 : 0 ); // increment data on every wrap-around
        } while ( err == JET_errSuccess );

        return err;
    }

    ERR ErrRandomInsert( int count = INT_MAX )
    {
        ERR             err;
        BigEndian<int>  be_key = 0;
        int             iData = 0;
        KEY             key;
        DATA            data;

        key.Nullify();
        key.suffix.SetPv( &be_key );
        key.suffix.SetCb( sizeof( be_key ) );
        data.Nullify();
        data.SetPv( &iData );
        data.SetCb( sizeof( iData ) );

        m_bbtBuffTrxLogger.m_dbtimeCoordinated = dbtimeStart + 10;
        CallR( m_bbtBuffWriter.ErrUpgradeToWriteMode() );

        int cInserted = 0;
        do
        {
            be_key = rand();
            err = m_bbtBuffWriter.ErrInsert( bbtOpInsert, key, data, fSLNNone );
            iData += 10;
            cInserted++;
        } while ( err == JET_errSuccess && cInserted < count );

        return err;
    }

    ERR ErrDeleteKey( int ikey )
    {
        ERR             err;
        BigEndian<int>  be_key = ikey;
        KEY             key;
        key.Nullify();
        key.suffix.SetPv( &be_key );
        key.suffix.SetCb( sizeof( be_key ) );

        while ( JET_errSuccess == ( err = m_bbtBuffWriter.ErrDelete( key ) ) )
        {}

        return err == errBBTNodeNotFound ? JET_errSuccess : err;
    }

    template <typename TFunc>
    ERR ErrKeyForEachOldestToLatest( int ikey, TFunc func )
    {
        ERR             err;
        BigEndian<int>  be_key = ikey;
        KEY             key;
        key.Nullify();
        key.suffix.SetPv( &be_key );
        key.suffix.SetCb( sizeof( be_key ) );

        CallR( m_bbtBuff.ErrSeekGEQOldest( key ) );
        do
        {
            func( m_bbtBuff.PnodeCurr() );
        }
        while ( m_bbtBuff.PnodeCurr()->FDuplicateNext0() && JET_errSuccess == ( err = m_bbtBuff.ErrMoveNext( BBTBuff::sfAllowDuplicates ) ) );

        return err;
    }
};

//  ================================================================
JETUNITTEST( BBTBuff, DmlBasic )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture  tf;
    BigEndian<int>      be_key = 42;
    int                iData = 10;
    KEY                 key;
    DATA                data;

    key.Nullify();
    key.suffix.SetPv( &be_key );
    key.suffix.SetCb( sizeof( be_key ) );
    data.Nullify();
    data.SetPv( &iData );
    data.SetCb( sizeof( iData ) );

    tf.m_bbtBuffTrxLogger.m_dbtimeCoordinated = dbtimeStart + 10;

    for ( int i = 0; i < 10; i++ )
    {
        CHECK_CALL( tf.m_bbtBuffWriter.ErrInsert( bbtOpInsert, key, data, fSLNNone ) );
        be_key++;
        iData += 10;
    }

    be_key = 45;
    iData = 40;
    CHECK_CALL( tf.m_bbtBuff.ErrSeekLEQ( key ) );
    CHECK( 0 == CmpData( tf.m_bbtBuff.PnodeCurr()->Data(), data ) );

    CHECK_CALL( tf.m_bbtBuffWriter.ErrDelete( key ) );
    CHECK( wrnNDFoundLess == tf.m_bbtBuff.ErrSeekLEQ( key ) );

    be_key = 44;
    iData = 30;
    CHECK( 0 == tf.m_bbtBuff.PnodeCurr()->CmpKey(key ) );
    CHECK( 0 == CmpData( tf.m_bbtBuff.PnodeCurr()->Data(), data ) );
}

//  ================================================================
JETUNITTEST( BBTBuff, Append )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture  tf;
    BigEndian<int>      be_key = 0;
    int                 iData = 0;
    KEY                 key;
    DATA                data;

    key.Nullify();
    key.suffix.SetPv( &be_key );
    key.suffix.SetCb( sizeof( be_key ) );
    data.Nullify();
    data.SetPv( &iData );
    data.SetCb( sizeof( iData ) );

    tf.m_bbtBuffTrxLogger.m_dbtimeCoordinated = dbtimeStart + 10;
    CHECK_CALL( tf.m_bbtBuffWriter.ErrUpgradeToWriteMode() );

    ERR err = JET_errSuccess;
    do
    {
        err = tf.m_bbtBuffWriter.ErrInsert( bbtOpInsert, key, data, fSLNNone );
        CHECK( err == JET_errSuccess || err == errBBTBuffFull );
        be_key++;
        iData += 10;
    }
    while ( err == JET_errSuccess );

    be_key--;
    wprintf( L"%d nodes inserted. ", (int) be_key );
    CHECK( tf.m_pbbtHeader->le_cNodes == (int) be_key );
    CHECK( tf.m_pbbtHeader->le_ibMicFree->ToInt() + SkipListNode::Cb( 0, key.Cb(), data.Cb() ) > tf.m_bbtBuff.CbMax() );
}

//  ================================================================
JETUNITTEST( BBTBuff, HotpointInsert )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture tf;
    CHECK( errBBTBuffFull == tf.ErrHotpointInsert( 100, 16 ) );

    wprintf( L"%d nodes inserted. ", (INT) tf.m_pbbtHeader->le_cNodes );
    CHECK( tf.m_pbbtHeader->le_ibMicFree->ToInt() + SkipListNode::Cb( 0, sizeof( int ), sizeof( int ) ) > tf.m_bbtBuff.CbMax() );
}

JETUNITTEST( BBTBuff, InsertMaxNodeSize )
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture  tf;
    KEY                 key;
    DATA                data;
    unique_ptr<BYTE[]>  pkey( new BYTE[ cbKeyMostMost ] );
    unique_ptr<BYTE[]>  pdata( new BYTE[ g_cbPage ] );
    int                 cbMaxSize = tf.m_bbtBuff.CbMaxNodeSize() - SkipListNode::Cb( 0, 0, 0 );

    memset( pkey.get(), 0xdada, cbKeyMostMost);
    memset( pdata.get(), 0xd0d0, g_cbPage);

    key.Nullify();
    key.suffix.SetPv( pkey.get() );
    key.suffix.SetCb( cbKeyMostMost );
    data.Nullify();
    data.SetPv( pdata.get() );
    data.SetCb( cbMaxSize - cbKeyMostMost );

    tf.m_bbtBuffTrxLogger.m_dbtimeCoordinated = dbtimeStart + 10;
    CHECK_CALL( tf.m_bbtBuffWriter.ErrUpgradeToWriteMode() );
    CHECK_CALL( tf.m_bbtBuffWriter.ErrInsert( bbtOpInsert, key, data, fSLNNone ) );

    // Insert 1 byte more than max supported node.
    // Should enforce.
    bool fEnforce = false;
    data.SetCb( data.Cb() + 1 );

    __try
    {
        tf.m_bbtBuffWriter.ErrInsert( bbtOpInsert, key, data, fSLNNone );
    }
    __except ( JetTestEnforceSEHException::Filter( GetExceptionInformation() ) )
    {
        fEnforce = true;
        JetTestEnforceSEHException::Cleanup();
    }

    CHECK( fEnforce );
}

//  ================================================================
JETUNITTEST( BBTBuff, ScanForward )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    ERR err = JET_errSuccess;
    BBTBuffTestFixture tf;
    CHECK( errBBTBuffFull == tf.ErrRandomInsert() );

    int     nodes = tf.m_bbtBuff.CNodes();
    KEY     keyPrev;
    keyPrev.Nullify();

    // Scan all nodes including duplicates.
    CHECK_CALL( tf.m_bbtBuff.ErrMoveFirst( BBTBuff::sfAllowDuplicates ) );
    for ( int i = 0; i < nodes; i++ )
    {
        SkipListNode* pnode = tf.m_bbtBuff.PnodeCurr();
        CHECK( pnode->CmpKey( keyPrev ) >= 0 );
        keyPrev = pnode->Key();
        err = tf.m_bbtBuff.ErrMoveNext( BBTBuff::sfAllowDuplicates );
    }

    wprintf( L"Scanned %d nodes. ", nodes );
    CHECK( err == errBBTNodeNotFound );
}

//  ================================================================
JETUNITTEST( BBTBuff, ScanForwardUnique )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    ERR err = JET_errSuccess;
    BBTBuffTestFixture tf;
    CHECK( errBBTBuffFull == tf.ErrHotpointInsert( 100, 16 ) );

    int     iData = 0;
    KEY     keyPrev;
    DATA    data;
    keyPrev.Nullify();
    data.Nullify();
    data.SetPv( &iData );
    data.SetCb( sizeof( iData ) );

    // Scan unique nodes (latest versions).
    int nodes = 0;
    CHECK_CALL( tf.m_bbtBuff.ErrMoveFirst( BBTBuff::sfSkipDuplicates ) );
    do
    {
        SkipListNode* pnode = tf.m_bbtBuff.PnodeCurr();
        CHECK( pnode->CmpKey( keyPrev ) > 0 );
        CHECK( CmpData( data, pnode->Data() ) < 0 );
        keyPrev = pnode->Key();
        nodes++;
    }
    while( JET_errSuccess == ( err = tf.m_bbtBuff.ErrMoveNext( BBTBuff::sfSkipDuplicates ) ) );

    wprintf( L"Scanned %d unique nodes. ", nodes );
    CHECK( err == errBBTNodeNotFound );
}

//  ================================================================
JETUNITTEST( BBTBuff, ScanReverse )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    ERR err = JET_errSuccess;
    BBTBuffTestFixture tf;
    CHECK( errBBTBuffFull == tf.ErrRandomInsert() );

    int nodes = tf.m_bbtBuff.CNodes();
    KEY keyPrev;
    keyPrev.Nullify();

    CHECK_CALL( tf.m_bbtBuff.ErrMoveLast() );
    for ( int i = 0; i < nodes; i++ )
    {
        SkipListNode* pnode = tf.m_bbtBuff.PnodeCurr();
        CHECK( pnode->CmpKey( keyPrev ) <= 0 || keyPrev.FNull() );
        keyPrev = pnode->Key();
        err = tf.m_bbtBuff.ErrMovePrev( BBTBuff::sfAllowDuplicates );
    }

    wprintf( L"Scanned %d nodes. ", nodes );
    CHECK( err == errBBTNodeNotFound );
}

//  ================================================================
JETUNITTEST( BBTBuff, ScanReverseUnique )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    ERR err = JET_errSuccess;
    BBTBuffTestFixture tf;
    CHECK( errBBTBuffFull == tf.ErrHotpointInsert( 100, 16 ) );

    int     iData = 0;
    KEY     keyPrev;
    DATA    data;
    keyPrev.Nullify();
    data.Nullify();
    data.SetPv( &iData );
    data.SetCb( sizeof( iData ) );

    // Scan unique nodes (latest versions).
    int nodes = 0;
    CHECK_CALL( tf.m_bbtBuff.ErrMoveLast() );
    do
    {
        SkipListNode* pnode = tf.m_bbtBuff.PnodeCurr();
        CHECK( pnode->CmpKey( keyPrev ) < 0 || keyPrev.FNull() );
        CHECK( CmpData( data, pnode->Data() ) < 0 );
        keyPrev = pnode->Key();
        nodes++;
    } while ( JET_errSuccess == ( err = tf.m_bbtBuff.ErrMovePrev( BBTBuff::sfSkipDuplicates ) ) );

    wprintf( L"Scanned %d unique nodes. ", nodes );
    CHECK( err == errBBTNodeNotFound );
}

//  ================================================================
JETUNITTEST( BBTBuff, SeekModes )
//  ================================================================
{
    BBTBuffTestFixture tf;
    CHECK( errBBTBuffFull == tf.ErrHotpointInsert( 100, 16 ) );

    BigEndian<int>  be_key;
    int             iData = 0;
    KEY             key;
    DATA            data;
    key.Nullify();
    key.suffix.SetPv( &be_key );
    key.suffix.SetCb( sizeof( be_key ) );
    data.SetPv( &iData );
    data.SetCb( sizeof( iData ) );

    SkipListLink rgLinks[ MAX_LEVELS ];
    SkipListNode* rgpNodes[ MAX_LEVELS ];

    auto testSeek = [&]( int iKey, BBTBuff::SeekMode seekMode, BBTBuff::SeekPos seekPos, int iExpectedResult, bool fNodeFound )
    {
        int iSeekResult;
        memset( rgLinks, 0, sizeof( rgLinks ) );
        memset( rgpNodes, 0, sizeof( rgpNodes ) );
        be_key = iKey;

        CHECK_CALL( tf.m_bbtBuff.ErrSeek_( key, seekMode, seekPos, &iSeekResult, rgLinks, rgpNodes ) );
        CHECK( iSeekResult * iExpectedResult > 0 || iSeekResult == iExpectedResult );   // both have the same sign, or they are 0

        SkipListNode* pnodeSeeked = rgpNodes[ 0 ];
        CHECK( !!pnodeSeeked == fNodeFound );

        if ( pnodeSeeked == NULL )
        {
            CHECK_CALL( tf.m_bbtBuff.ErrMoveFirst( BBTBuff::sfAllowDuplicates ) );
            pnodeSeeked = tf.m_bbtBuff.PnodeCurr();
        }
        else if ( seekPos == BBTBuff::SeekPos::Prev )
        {
            // Returned node is prev to the node where the seek landed.
            CHECK( pnodeSeeked->FDuplicateNext0() );    // each key in the test has duplicates, so prev node should always be a duplicate
            CHECK_CALL( tf.m_bbtBuff.ErrSetCurrNodeFromLink( pnodeSeeked->LinkNext0() ) );
            pnodeSeeked = tf.m_bbtBuff.PnodeCurr();
        }

        int cmpActual = CmpKey( pnodeSeeked->Key(), key );
        CHECK( ( cmpActual * iExpectedResult ) > 0 || cmpActual == iExpectedResult );   // both have the same sign, or they are 0

        (void)tf.m_bbtBuff.ErrSetCurrNodeFromLink( rgLinks[ 0 ] );
        CHECK( tf.m_bbtBuff.PnodeCurr() == rgpNodes[ 0 ] );

        int i = 0;
        if ( fNodeFound )
        {
            int level = pnodeSeeked->Level();
            if ( seekPos == BBTBuff::SeekPos::Prev )
            {
                SkipListLink linkSeeked = rgpNodes[ 0 ]->LinkNext0();
                for ( i = 0; i <= level; i++ )
                {
                    // For SeekPos::Prev, returned nodes should be strictly the prev node to seeked node
                    // at current level.
                    CHECK( linkSeeked == rgpNodes[ i ]->RgLinksNext()[ i ] );
                    (void) tf.m_bbtBuff.ErrSetCurrNodeFromLink( rgLinks[ i ] );
                    CHECK( tf.m_bbtBuff.PnodeCurr() == rgpNodes[ i ] );
                }
            }
            else
            {
                for ( i = 1; i <= level; i++ )
                {
                    CHECK( pnodeSeeked == rgpNodes[ i ] );
                    CHECK( rgLinks[ i - 1 ] == rgLinks[ i ] );
                }

                CHECK( rgpNodes[ i ] != rgpNodes[ i - 1 ] );    // can't be the same node, levels are different
            }

            for ( i; i < MAX_LEVELS; i++ )
            {
                if ( rgpNodes[ i ] != NULL )
                {
                    // This node must be a prev/same node to lower level node
                    if ( rgpNodes[ i ] != rgpNodes[ i - 1 ] )
                    {
                        int cmp = CmpKey( rgpNodes[ i ]->Key(), rgpNodes[ i - 1 ]->Key() );
                        CHECK( cmp < 0 || ( cmp == 0 &&
                                          ( (int*) rgpNodes[ i ]->Data().Pv() ) <= ( (int*) rgpNodes[ i - 1 ]->Data().Pv() ) ) );
                    }

                    (void) tf.m_bbtBuff.ErrSetCurrNodeFromLink( rgLinks[ i ] );
                    CHECK( tf.m_bbtBuff.PnodeCurr() == rgpNodes[ i ] );

                    (void) tf.m_bbtBuff.ErrSetCurrNodeFromLink( rgLinks[ i - 1 ] );
                    CHECK( tf.m_bbtBuff.PnodeCurr() == rgpNodes[ i - 1 ] );
                }
                else
                {
                    break;
                }
            }
        }

        for ( i; i < MAX_LEVELS; i++ )
        {
            CHECK( rgpNodes[ i ] == NULL );
            CHECK( rgLinks[ i ] == NULL );
        }
    };

    // Seek before first key.
    testSeek( 50, BBTBuff::SeekMode::LT, BBTBuff::SeekPos::Curr, 1, false );
    testSeek( 50, BBTBuff::SeekMode::LEQ, BBTBuff::SeekPos::Curr, 1, false );
    testSeek( 50, BBTBuff::SeekMode::LT, BBTBuff::SeekPos::Prev, 1, false );
    testSeek( 50, BBTBuff::SeekMode::LEQ, BBTBuff::SeekPos::Prev, 1, false );

    // Seek to first key.
    testSeek( 100, BBTBuff::SeekMode::LT, BBTBuff::SeekPos::Curr, 0, false );
    testSeek( 100, BBTBuff::SeekMode::LEQ, BBTBuff::SeekPos::Curr, 0, true );
    testSeek( 100, BBTBuff::SeekMode::LT, BBTBuff::SeekPos::Prev, 0, false );
    testSeek( 100, BBTBuff::SeekMode::LEQ, BBTBuff::SeekPos::Prev, 0, true );   // will land on older duplicate version of first key

    // Seek before some key in the middle.
    testSeek( 950, BBTBuff::SeekMode::LT, BBTBuff::SeekPos::Curr, -1, true );
    testSeek( 950, BBTBuff::SeekMode::LEQ, BBTBuff::SeekPos::Curr, -1, true );
    testSeek( 950, BBTBuff::SeekMode::LT, BBTBuff::SeekPos::Prev, -1, true );
    testSeek( 950, BBTBuff::SeekMode::LEQ, BBTBuff::SeekPos::Prev, -1, true );

    // Seek to some key in the middle.
    testSeek( 1100, BBTBuff::SeekMode::LT, BBTBuff::SeekPos::Curr, -1, true );
    testSeek( 1100, BBTBuff::SeekMode::LEQ, BBTBuff::SeekPos::Curr, 0, true );
    testSeek( 1100, BBTBuff::SeekMode::LT, BBTBuff::SeekPos::Prev, -1, true );
    testSeek( 1100, BBTBuff::SeekMode::LEQ, BBTBuff::SeekPos::Prev, 0, true );

    // Seek after last.
    testSeek( 1800, BBTBuff::SeekMode::LT, BBTBuff::SeekPos::Curr, -1, true );
    testSeek( 1800, BBTBuff::SeekMode::LEQ, BBTBuff::SeekPos::Curr, -1, true );
    testSeek( 1800, BBTBuff::SeekMode::LT, BBTBuff::SeekPos::Curr, -1, true );
    testSeek( 1800, BBTBuff::SeekMode::LEQ, BBTBuff::SeekPos::Curr, -1, true );
}

//  ================================================================
JETUNITTEST( BBTBuff, SeekLEQ )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture tf;
    CHECK( errBBTBuffFull == tf.ErrHotpointInsert( 100, 16 ) );

    BigEndian<int>  be_key;
    int             iData = 0;
    KEY             key;
    DATA            data;
    key.Nullify();
    key.suffix.SetPv( &be_key );
    key.suffix.SetCb( sizeof( be_key ) );
    data.SetPv( &iData );
    data.SetCb( sizeof( iData ) );

    // Seek equal.
    be_key = 600;
    CHECK( JET_errSuccess == tf.m_bbtBuff.ErrSeekLEQ( key ) );
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) == 0 );
    CHECK( CmpData( data, tf.m_bbtBuff.PnodeCurr()->Data() ) < 0 ); // must seek to latest version
    CHECK_CALL( tf.m_bbtBuff.ErrMoveNext( BBTBuff::sfAllowDuplicates ) );
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) > 0 );               // latest node in the sequence means next node can't have the same key

    // Seek LT.
    be_key = 599;
    CHECK( wrnNDFoundLess == tf.m_bbtBuff.ErrSeekLEQ( key ) );
    be_key = 500;
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) == 0 );
    CHECK( CmpData( data, tf.m_bbtBuff.PnodeCurr()->Data() ) < 0 );
    CHECK_CALL( tf.m_bbtBuff.ErrMoveNext( BBTBuff::sfAllowDuplicates ) );
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) > 0 );

    // Seek to lowest key.
    be_key = 0;
    CHECK( wrnNDFoundGreater == tf.m_bbtBuff.ErrSeekLEQ( key ) );
    be_key = 100;
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) == 0 );
    CHECK( CmpData( data, tf.m_bbtBuff.PnodeCurr()->Data() ) < 0 );
    CHECK_CALL( tf.m_bbtBuff.ErrMoveNext( BBTBuff::sfAllowDuplicates ) );
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) > 0 );

    // Seek to highest key.
    be_key = INT_MAX;
    CHECK( wrnNDFoundLess == tf.m_bbtBuff.ErrSeekLEQ( key ) );
    be_key = 1700;
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) == 0 );
    CHECK( CmpData( data, tf.m_bbtBuff.PnodeCurr()->Data() ) < 0 );
    CHECK( errBBTNodeNotFound == tf.m_bbtBuff.ErrMoveNext( BBTBuff::sfAllowDuplicates ) );
}

//  ================================================================
JETUNITTEST( BBTBuff, SeekGEQOldest )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture tf;
    CHECK( errBBTBuffFull == tf.ErrHotpointInsert( 100, 16 ) );

    BigEndian<int>  be_key;
    int             iData = 0;
    KEY             key;
    DATA            data;
    key.Nullify();
    key.suffix.SetPv( &be_key );
    key.suffix.SetCb( sizeof( be_key ) );
    data.SetPv( &iData );
    data.SetCb( sizeof( iData ) );

    // Seek equal.
    be_key = 700;
    CHECK( JET_errSuccess == tf.m_bbtBuff.ErrSeekGEQOldest( key ) );
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) == 0 );
    CHECK( CmpData( data, tf.m_bbtBuff.PnodeCurr()->Data() ) == 0 );    // must seek to oldest version
    CHECK_CALL( tf.m_bbtBuff.ErrMovePrev( BBTBuff::sfAllowDuplicates ) );
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) < 0 );                   // oldest node in the sequence means prev node can't have the same key

    // Seek GT.
    be_key = 599;
    CHECK( wrnNDFoundGreater == tf.m_bbtBuff.ErrSeekGEQOldest( key ) );
    be_key = 600;
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) == 0 );
    CHECK( CmpData( data, tf.m_bbtBuff.PnodeCurr()->Data() ) == 0 );
    CHECK_CALL( tf.m_bbtBuff.ErrMovePrev( BBTBuff::sfAllowDuplicates ) );
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) < 0 );

    // Seek to lowest key.
    be_key = 0;
    CHECK( wrnNDFoundGreater == tf.m_bbtBuff.ErrSeekGEQOldest( key ) );
    be_key = 100;
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) == 0 );
    CHECK( CmpData( data, tf.m_bbtBuff.PnodeCurr()->Data() ) == 0 );
    CHECK( errBBTNodeNotFound == tf.m_bbtBuff.ErrMovePrev( BBTBuff::sfAllowDuplicates ) );

    // Seek to highest key.
    // Expected: this will return node not found. Will not seek to a smaller key
    // (unlike ErrSeekLEQ() which seeks to a GT key if no LEQ keys are found).
    be_key = INT_MAX;
    CHECK( errBBTNodeNotFound == tf.m_bbtBuff.ErrSeekGEQOldest( key ) );
}

//  ================================================================
JETUNITTEST( BBTBuff, Delete )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture tf;
    CHECK( errBBTBuffFull == tf.ErrHotpointInsert( 100, 16 ) );

    ERR             err;
    BigEndian<int>  be_key;
    int             iData = 0;
    int             cNodesBegin = tf.m_pbbtHeader->le_cNodes;
    int             cNodesDel = 0;
    KEY             key;
    key.Nullify();
    key.suffix.SetPv( &be_key );
    key.suffix.SetCb( sizeof( be_key ) );

    // Get latest version's data.
    be_key = 1400;
    CHECK_CALL( tf.m_bbtBuff.ErrSeekLEQ( key ) );
    iData = *( (int*) tf.m_bbtBuff.PnodeCurr()->Data().Pv() );
    CHECK( iData != 0 );

    // Delete all versions of a node (latest to oldest).
    for ( int i = iData; i > 0; i-- )
    {
        CHECK_CALL( tf.m_bbtBuffWriter.ErrDelete( key ) );
        CHECK_CALL( tf.m_bbtBuff.ErrSeekLEQ( key ) );
        int iDataCurr = *( (int*) tf.m_bbtBuff.PnodeCurr()->Data().Pv() );
        CHECK( iDataCurr == i - 1 );
        cNodesDel++;
    }

    // Last version.
    CHECK_CALL( tf.m_bbtBuffWriter.ErrDelete( key ) );
    CHECK( wrnNDFoundLess == tf.m_bbtBuff.ErrSeekLEQ( key ) );
    cNodesDel++;

    // Delete all versions of first node.
    be_key = 100;
    while ( JET_errSuccess == ( err = tf.m_bbtBuffWriter.ErrDelete( key ) ) )
    {
        cNodesDel++;
    }

    CHECK( err == errBBTNodeNotFound );
    CHECK_CALL( tf.m_bbtBuff.ErrMoveFirst( BBTBuff::sfSkipDuplicates ) );
    be_key = 200;
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) == 0 );

    // Delete all versions of last node.
    be_key = 1700;
    while ( JET_errSuccess == ( err = tf.m_bbtBuffWriter.ErrDelete( key ) ) )
    {
        cNodesDel++;
    }

    CHECK( err == errBBTNodeNotFound );
    CHECK_CALL( tf.m_bbtBuff.ErrMoveLast() );
    be_key = 1600;
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) == 0 );

    wprintf( L"%d nodes deleted. ", cNodesDel );
    CHECK( tf.m_pbbtHeader->le_cNodes == cNodesBegin - cNodesDel );
    CHECK_CALL( tf.m_bbtBuff.ErrValidate() );
}

//  ================================================================
JETUNITTEST( BBTBuff, RangeDelete )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture tf;
    CHECK( errBBTBuffFull == tf.ErrHotpointInsert( 100, 16 ) );

    BigEndian<int>  be_key;
    KEY             key;
    key.Nullify();
    key.suffix.SetPv( &be_key );
    key.suffix.SetCb( sizeof( be_key ) );

    // Get latest version's data.
    be_key = 1400;
    CHECK_CALL( tf.m_bbtBuff.ErrSeekGEQOldest( key ) );
    SkipListLink linkStart = tf.m_bbtBuff.GetLinkToCurrNode();
    int cNodesToDel = 1;
    while ( tf.m_bbtBuff.PnodeCurr()->FDuplicateNext0() )
    {
        CHECK_CALL( tf.m_bbtBuff.ErrMoveNext( BBTBuff::sfAllowDuplicates ) );
        cNodesToDel++;
    }

    int cNodes = tf.m_pbbtHeader->le_cNodes;
    CHECK_CALL( tf.m_bbtBuffWriter.ErrRangeDelete( linkStart, cNodesToDel ) );
    CHECK( cNodes - cNodesToDel == tf.m_pbbtHeader->le_cNodes );

    CHECK( wrnNDFoundLess == tf.m_bbtBuff.ErrSeekLEQ( key ) );
    be_key = 1300;
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) == 0 );
    be_key = 1500;
    tf.m_bbtBuff.ErrMoveNext( BBTBuff::sfAllowDuplicates );
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) == 0 );

    // RangeDelete from 1500 to end.
    linkStart = tf.m_bbtBuff.GetLinkToCurrNode();
    cNodesToDel = 1;
    while ( JET_errSuccess == tf.m_bbtBuff.ErrMoveNext( BBTBuff::sfAllowDuplicates ) )
    {
        cNodesToDel++;
    }

    cNodes = tf.m_pbbtHeader->le_cNodes;
    CHECK_CALL( tf.m_bbtBuffWriter.ErrRangeDelete( linkStart, cNodesToDel ) );
    CHECK( cNodes - cNodesToDel == tf.m_pbbtHeader->le_cNodes );

    be_key = 1300;
    CHECK_CALL( tf.m_bbtBuff.ErrMoveLast() );
    CHECK( tf.m_bbtBuff.PnodeCurr()->CmpKey( key ) == 0 );
    wprintf( L"%d nodes deleted. ", cNodesToDel );

    CHECK_CALL( tf.m_bbtBuff.ErrValidate() );
}

//  ================================================================
JETUNITTESTEX( BBTBuff, Reogranize, JetSimpleUnitTest::dwBufferManager )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture tf;
    CHECK( errBBTBuffFull == tf.ErrHotpointInsert( 100, 16 ) );

    BigEndian<int>  be_key;
    KEY             key;
    key.Nullify();
    key.suffix.SetPv( &be_key );
    key.suffix.SetCb( sizeof( be_key ) );

    // Delete all versions of a node (latest to oldest).
    int cNodesDel = tf.m_pbbtHeader->le_cNodes;
    CHECK_CALL( tf.ErrDeleteKey( 1400 ) );
    cNodesDel -= tf.m_pbbtHeader->le_cNodes;
    CHECK( cNodesDel > 0 );

    int ibMicFree = tf.m_pbbtHeader->le_ibMicFree->ToInt();
    CHECK_CALL( tf.m_bbtBuffWriter.ErrReorganize() );

    int ibMicFreeNew = tf.m_pbbtHeader->le_ibMicFree->ToInt();
    constexpr int cbSmallestNode = SkipListNode::Cb( 0, sizeof( int ), sizeof( int ) );

    // Reorganize() should atleast free up this much space, assuming every node was level0 (least overhead).
    CHECK( ibMicFreeNew + cNodesDel * cbSmallestNode <= ibMicFree );
    wprintf( L"Freed up %d bytes. ", ibMicFree - ibMicFreeNew );

    CHECK_CALL( tf.m_bbtBuff.ErrValidate() );
}


class MergeAndDelTestFixture : public JetTestFixture
{
public:
    BBTBuffTestFixture                  tf;
    BBTBuffTestFixture                  tfFrom;
    FixedHeapArray<SkipListLink>        rgLinksToDel;
    FixedHeapArray<const SkipListNode*> rgpNodesToMerge;
    int                                 cLinksToDel = 0;
    int                                 cNodesToMerge = 0;

    MergeAndDelTestFixture( JetUnitTestResult* presult ) : JetTestFixture( presult )
    {}

    void Prepare()
    {
        CHECK( errBBTBuffFull == tf.ErrHotpointInsert( 100, 16 ) );
        CHECK( errBBTBuffFull == tfFrom.ErrHotpointInsert( 100, 20 ) );

        BigEndian<int>  be_key;
        KEY             key;
        key.Nullify();
        key.suffix.SetPv( &be_key );
        key.suffix.SetCb( sizeof( be_key ) );

        // Make some room by marking some nodes for deletion.
        be_key = 1400;
        CHECK_CALL( tf.m_bbtBuff.ErrSeekLEQ( key ) );
        int cNodesPerKey = 1 + *( (int*) tf.m_bbtBuff.PnodeCurr()->Data().Pv() );

        rgLinksToDel = FixedHeapArray<SkipListLink>::MakeArray( cNodesPerKey * 3 );
        auto addLinkToDel = [this]( SkipListNode* pnode )
        {
            rgLinksToDel[ cLinksToDel++ ] = tf.m_bbtBuff.GetLinkToCurrNode();
        };

        CHECK_CALL( tf.ErrKeyForEachOldestToLatest( 1000, addLinkToDel ) );
        cLinksToDel--;  // deliberately leave 1 node from this key behind
        CHECK_CALL( tf.ErrKeyForEachOldestToLatest( 1300, addLinkToDel ) );
        cLinksToDel--;
        CHECK_CALL( tf.ErrKeyForEachOldestToLatest( 1500, addLinkToDel ) );
        cLinksToDel--;

        CHECK( cLinksToDel <= cNodesPerKey * 3 );

        // Get nodes to merge.
        rgpNodesToMerge = FixedHeapArray<const SkipListNode*>::MakeArray( cNodesPerKey * 5 );
        auto addNodesToMerge = [this]( SkipListNode* pnode )
        {
            rgpNodesToMerge[ cNodesToMerge++ ] = pnode;
        };

        // Gather more nodes than the amount that can fit.
        CHECK_CALL( tfFrom.ErrKeyForEachOldestToLatest( 100, addNodesToMerge ) );
        CHECK_CALL( tfFrom.ErrKeyForEachOldestToLatest( 500, addNodesToMerge ) );
        CHECK_CALL( tfFrom.ErrKeyForEachOldestToLatest( 1000, addNodesToMerge ) );
        CHECK_CALL( tfFrom.ErrKeyForEachOldestToLatest( 1500, addNodesToMerge ) );
        CHECK_CALL( tfFrom.ErrKeyForEachOldestToLatest( 1900, addNodesToMerge ) );
    }

    // This class derives from JetTestFixture just to make CHECK() macros work.
    // Setup_()/Teardown_() isn't used.
protected:
    bool SetUp_() { return false; }
    void TearDown_() {}
};

//  ================================================================
JETUNITTESTEX( BBTBuff, MergeAndDel, JetSimpleUnitTest::dwBufferManager )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    MergeAndDelTestFixture md( m_presult );
    md.Prepare();

    int cNodesBefore = md.tf.m_pbbtHeader->le_cNodes;
    int cNodesMerged = 0;
    int cNodesDeleted = 0;
    ERR err = md.tf.m_bbtBuffWriter.ErrMergeAndDelNodes(
                                        md.rgpNodesToMerge.PrgT(),
                                        md.rgLinksToDel.PrgT(),
                                        md.cNodesToMerge,
                                        md.cLinksToDel,
                                        &cNodesMerged,
                                        &cNodesDeleted,
                                        0 );

    wprintf( L"%d nodes merged, %d deleted. ", cNodesMerged, cNodesDeleted );
    CHECK( err == wrnBBTMergeTargetFull );
    CHECK( cNodesMerged <= md.cNodesToMerge );
    CHECK( cNodesDeleted == md.cLinksToDel );
    CHECK( cNodesBefore + cNodesMerged - cNodesDeleted == md.tf.m_pbbtHeader->le_cNodes );
    CHECK_CALL( md.tf.m_bbtBuff.ErrValidate() );
}

//  ================================================================
JETUNITTESTEX( BBTBuff, MergeAndDelNoSpace, JetSimpleUnitTest::dwBufferManager )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture tf;
    BBTBuffTestFixture tfFrom;
    CHECK( errBBTBuffFull == tf.ErrHotpointInsert( 100, 16, 0 ) );  // takes a long time because of n^2 runtime.
    CHECK( errBBTBuffFull == tfFrom.ErrHotpointInsert( 100, 20 ) );

    BigEndian<int>  be_key;
    KEY             key;
    key.Nullify();
    key.suffix.SetPv( &be_key );
    key.suffix.SetCb( sizeof( be_key ) );

    // Make some room by marking some nodes for deletion.
    int cLinksToDel = 10;
    unique_ptr<SkipListLink[]> rgLinksToDel( new SkipListLink[ cLinksToDel ] );

    CHECK_CALL( tf.m_bbtBuff.ErrMoveFirst( BBTBuff::sfSkipDuplicates ) );
    for ( int i = 0; i < cLinksToDel; i++ )
    {
        rgLinksToDel[ i ] = tf.m_bbtBuff.GetLinkToCurrNode();
        CHECK_CALL( tf.m_bbtBuff.ErrMoveNext( BBTBuff::sfSkipDuplicates ) );
    }

    // Get nodes to merge.
    int cNodesToMerge = 10;
    unique_ptr<const SkipListNode* []> rgpNodesToMerge( new const SkipListNode * [ cNodesToMerge ] );
    CHECK_CALL( tfFrom.m_bbtBuff.ErrMoveFirst( BBTBuff::sfSkipDuplicates ) );
    for ( int i = 0; i < cNodesToMerge; i++ )
    {
        rgLinksToDel[ i ] = tfFrom.m_bbtBuff.GetLinkToCurrNode();
        CHECK_CALL( tfFrom.m_bbtBuff.ErrMoveNext( BBTBuff::sfSkipDuplicates ) );
    }

    int cNodesBefore = tf.m_pbbtHeader->le_cNodes;
    int cNodesMerged = 0;
    int cNodesDeleted = 0;
    ERR err = tf.m_bbtBuffWriter.ErrMergeAndDelNodes(
        rgpNodesToMerge.get(),
        rgLinksToDel.get(),
        cNodesToMerge,
        cLinksToDel,
        &cNodesMerged,
        &cNodesDeleted,
        0 );

    CHECK( err == errBBTBuffFull );
    CHECK( cNodesMerged == 0 );
    CHECK( cNodesDeleted == 0 );
    CHECK( cNodesBefore + cNodesMerged - cNodesDeleted == tf.m_pbbtHeader->le_cNodes );
    CHECK_CALL( tf.m_bbtBuff.ErrValidate() );
}

//  ================================================================
JETUNITTEST( BBTBuff, Dump )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture tf;
    CHECK( JET_errSuccess == tf.ErrRandomInsert( 10 ) );

    std::string szDump = DumpBBTBuff( tf.m_bbtBuff, 0 );
    wprintf( L"\n%hs\n", szDump.c_str() );
}


////////////////////////////////////////////////////////////////////
// Negative tests. Tests with failed dml because of failed trx log.

//  ================================================================
JETUNITTEST( BBTBuff, FailedInsert )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture  tf;
    BigEndian<int>      be_key = 42;
    int                iData = 10;
    KEY                 key;
    DATA                data;

    key.Nullify();
    key.suffix.SetPv( &be_key );
    key.suffix.SetCb( sizeof( be_key ) );
    data.Nullify();
    data.SetPv( &iData );
    data.SetCb( sizeof( iData ) );

    CHECK_CALL( tf.ErrRandomInsert( 10 ) );

    DBTIME dbtimeInitial = tf.m_rgcsr[ 0 ].Dbtime();
    tf.m_bbtBuffTrxLogger.m_returnErr = ErrERRCheck( JET_errLogWriteFail );
    tf.m_bbtBuffTrxLogger.m_dbtimeCoordinated = dbtimeInitial + 10;
    CHECK( JET_errLogWriteFail == tf.m_bbtBuffWriter.ErrInsert( bbtOpInsert, key, data, fSLNNone ) );

    CHECK_CALL( tf.m_bbtBuff.ErrValidate() );
    CHECK( 10 == tf.m_pbbtHeader->le_cNodes );
    tf.m_rgcsr.ForEach( [this, dbtimeInitial]( const CSR& csr )
    {
        CHECK( csr.Dbtime() == dbtimeInitial );
    } );
}

//  ================================================================
JETUNITTEST( BBTBuff, FailedDelete )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture  tf;
    KEY                 key;
    DATA                data;

    CHECK_CALL( tf.ErrRandomInsert( 10 ) );
    CHECK_CALL( tf.m_bbtBuff.ErrMoveFirst( BBTBuff::sfSkipDuplicates ) );

    key = tf.m_bbtBuff.PnodeCurr()->Key();
    data = tf.m_bbtBuff.PnodeCurr()->Data();

    DBTIME dbtimeInitial = tf.m_rgcsr[ 0 ].Dbtime();
    tf.m_bbtBuffTrxLogger.m_returnErr = ErrERRCheck( JET_errLogWriteFail );
    tf.m_bbtBuffTrxLogger.m_dbtimeCoordinated = dbtimeInitial + 10;
    CHECK( JET_errLogWriteFail == tf.m_bbtBuffWriter.ErrDelete( key ) );

    CHECK_CALL( tf.m_bbtBuff.ErrValidate() );
    CHECK( 10 == tf.m_pbbtHeader->le_cNodes );
    tf.m_rgcsr.ForEach( [this, dbtimeInitial]( const CSR& csr )
    {
        CHECK( csr.Dbtime() == dbtimeInitial );
    } );
}

//  ================================================================
JETUNITTEST( BBTBuff, FailedRangeDelete )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    BBTBuffTestFixture  tf;

    CHECK_CALL( tf.ErrRandomInsert( 10 ) );
    CHECK_CALL( tf.m_bbtBuff.ErrMoveFirst( BBTBuff::sfSkipDuplicates ) );

    SkipListLink linkFirst = tf.m_bbtBuff.GetLinkToCurrNode();
    DBTIME dbtimeInitial = tf.m_rgcsr[ 0 ].Dbtime();
    tf.m_bbtBuffTrxLogger.m_returnErr = ErrERRCheck( JET_errLogWriteFail );
    tf.m_bbtBuffTrxLogger.m_dbtimeCoordinated = dbtimeInitial + 10;
    CHECK( JET_errLogWriteFail == tf.m_bbtBuffWriter.ErrRangeDelete( linkFirst, 5 ) );

    CHECK_CALL( tf.m_bbtBuff.ErrValidate() );
    CHECK( 10 == tf.m_pbbtHeader->le_cNodes );
    tf.m_rgcsr.ForEach( [this, dbtimeInitial]( const CSR& csr )
    {
        CHECK( csr.Dbtime() == dbtimeInitial );
    } );
}

//  ================================================================
JETUNITTESTEX( BBTBuff, FailedMergeAndDel, JetSimpleUnitTest::dwBufferManager )
//  ================================================================
{
    SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, 32768, NULL );

    MergeAndDelTestFixture md( m_presult );
    md.Prepare();

    DBTIME dbtimeInitial = md.tf.m_rgcsr[ 0 ].Dbtime();
    md.tf.m_bbtBuffTrxLogger.m_dbtimeCoordinated = dbtimeInitial + 10;
    md.tf.m_bbtBuffTrxLogger.m_returnErr = ErrERRCheck( JET_errLogWriteFail );

    int cNodesBefore = md.tf.m_pbbtHeader->le_cNodes;
    int cNodesMerged = 0;
    int cNodesDeleted = 0;
    ERR err = md.tf.m_bbtBuffWriter.ErrMergeAndDelNodes(
                                        md.rgpNodesToMerge.PrgT(),
                                        md.rgLinksToDel.PrgT(),
                                        md.cNodesToMerge,
                                        md.cLinksToDel,
                                        &cNodesMerged,
                                        &cNodesDeleted,
                                        0 );

    wprintf( L"%d nodes merged, %d deleted. ", cNodesMerged, cNodesDeleted );
    CHECK( err == JET_errLogWriteFail );
    CHECK( cNodesMerged == 0 );
    CHECK( cNodesDeleted == 0 );
    CHECK( cNodesBefore == md.tf.m_pbbtHeader->le_cNodes );
    CHECK_CALL( md.tf.m_bbtBuff.ErrValidate() );
    md.tf.m_rgcsr.ForEach( [this, dbtimeInitial]( const CSR& csr )
    {
        CHECK( csr.Dbtime() == dbtimeInitial );
    } );
}

