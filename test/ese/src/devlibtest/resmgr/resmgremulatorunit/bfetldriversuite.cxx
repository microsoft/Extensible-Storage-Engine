// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "resmgremulatorunit.hxx"


class ETLDriverTest : public UNITTEST
{
    private:
        static ETLDriverTest s_instance;

    protected:
        ETLDriverTest() {}
    public:
        ~ETLDriverTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();

    private:
        const static size_t s_cbftrace = 11;
        BFTRACE m_rgBfTrace[s_cbftrace];

        const static size_t s_cetwevt = 14;
        EtwEvent* m_rgpEtwEvt[s_cetwevt];

    private:
        void SetupTestData_();
        void CleanupTestData_();
        ERR ErrInitWithInvalidParamsTest_();
        ERR ErrTermWithNullPointer_();
        ERR ErrGetNextWithInvalidParamsTest_();
        ERR ErrBasicInitTermTest_();
        ERR ErrBasicInitGetTermTest_();
        ERR ErrBasicInitTermMultiPidTest_();
        ERR ErrBasicInitGetTermMultiPidTest_();
        ERR ErrGetNextSmallBufferTest_();
        ERR ErrGetNextSuccessfulTest_( const DWORD cEventsBufferedMax, const INT iPID );
        ERR ErrGetNextMediumBufferTest_();
        ERR ErrGetNextLargeBufferTest_();
        ERR ErrGetNextHugeBufferTest_();
        ERR ErrGetNextAllPidsTest_();
        ERR ErrGetNextMultiPidsTest_();
        ERR ErrCmpEtlTooManyTest_();
        ERR ErrCmpFtlTooManyTest_();
        ERR ErrCmpTraceMismatchTest_();
        ERR ErrCmpSuccessEmptyTest_();
        ERR ErrCmpSuccessAllPidsTest_();
        ERR ErrCmpSuccessTest_();
        ERR ErrStatsByPidAllPidsTest_();
        ERR ErrStatsByPidOnePidTest_();
        ERR ErrStatsByPidInvalidParamsTest_();
        ERR ErrStatsByPidNoStatsTest_();
};

ETLDriverTest ETLDriverTest::s_instance;

const char * ETLDriverTest::SzName() const          { return "ETLDriverTest"; };
const char * ETLDriverTest::SzDescription() const   { return "Tests for the BFETL driver."; }
bool ETLDriverTest::FRunUnderESE98() const          { return true; }
bool ETLDriverTest::FRunUnderESENT() const          { return true; }
bool ETLDriverTest::FRunUnderESE97() const          { return true; }


ERR ETLDriverTest::ErrTest()
{
    ERR err = JET_errSuccess;
    
    TestCall( ErrInitWithInvalidParamsTest_() );
    TestCall( ErrTermWithNullPointer_() );
    TestCall( ErrGetNextWithInvalidParamsTest_() );
    TestCall( ErrBasicInitTermTest_() );
    TestCall( ErrBasicInitGetTermTest_() );
    TestCall( ErrBasicInitTermMultiPidTest_() );
    TestCall( ErrBasicInitGetTermMultiPidTest_() );
    TestCall( ErrGetNextSmallBufferTest_() );
    TestCall( ErrGetNextMediumBufferTest_() );
    TestCall( ErrGetNextLargeBufferTest_() );
    TestCall( ErrGetNextHugeBufferTest_() );
    TestCall( ErrGetNextAllPidsTest_() );
    TestCall( ErrGetNextMultiPidsTest_() );
    TestCall( ErrCmpEtlTooManyTest_() );
    TestCall( ErrCmpFtlTooManyTest_() );
    TestCall( ErrCmpTraceMismatchTest_() );
    TestCall( ErrCmpSuccessEmptyTest_() );
    TestCall( ErrCmpSuccessAllPidsTest_() );
    TestCall( ErrCmpSuccessTest_() );
    TestCall( ErrStatsByPidAllPidsTest_() );
    TestCall( ErrStatsByPidOnePidTest_() );
    TestCall( ErrStatsByPidInvalidParamsTest_() );
    TestCall( ErrStatsByPidNoStatsTest_() );

HandleError:
    return err;
}

void ETLDriverTest::SetupTestData_()
{

    memset( &m_rgBfTrace, 0, sizeof(m_rgBfTrace) );
    TestAssert( _countof( m_rgBfTrace ) == s_cbftrace );

    size_t cbftrace = 0;

    m_rgBfTrace[cbftrace].traceid = bftidEvict;
    m_rgBfTrace[cbftrace].tick = (TICK)0xFFFFFFFA;
    m_rgBfTrace[cbftrace].bfevict.ifmp = 1;
    m_rgBfTrace[cbftrace].bfevict.pgno = 123;
    m_rgBfTrace[cbftrace].bfevict.fCurrentVersion = fTrue;
    m_rgBfTrace[cbftrace].bfevict.errBF = -1019;
    m_rgBfTrace[cbftrace].bfevict.bfef = 0x89;
    m_rgBfTrace[cbftrace].bfevict.pctPri = 44;
    cbftrace++;

    m_rgBfTrace[cbftrace].traceid = bftidSysResMgrInit;
    m_rgBfTrace[cbftrace].tick = (TICK)0xFFFFFFFC;
    m_rgBfTrace[cbftrace].bfinit.K = 3;
    m_rgBfTrace[cbftrace].bfinit.csecCorrelatedTouch = 0.1234;
    m_rgBfTrace[cbftrace].bfinit.csecTimeout = 5678.0;
    m_rgBfTrace[cbftrace].bfinit.csecUncertainty = 432.1;
    m_rgBfTrace[cbftrace].bfinit.dblHashLoadFactor = 987.6;
    m_rgBfTrace[cbftrace].bfinit.dblHashUniformity = 123.4;
    m_rgBfTrace[cbftrace].bfinit.dblSpeedSizeTradeoff = 5.67;
    cbftrace++;

    m_rgBfTrace[cbftrace].traceid = bftidCache;
    m_rgBfTrace[cbftrace].tick = (TICK)0xFFFFFFFD;
    m_rgBfTrace[cbftrace].bfcache.ifmp = 1;
    m_rgBfTrace[cbftrace].bfcache.pgno = 234;
    m_rgBfTrace[cbftrace].bfcache.bflt = 0x56;
    m_rgBfTrace[cbftrace].bfcache.clientType = 0x78;
    m_rgBfTrace[cbftrace].bfcache.pctPri = 22;
    m_rgBfTrace[cbftrace].bfcache.fUseHistory = fTrue;
    m_rgBfTrace[cbftrace].bfcache.fNewPage = fFalse;
    m_rgBfTrace[cbftrace].bfcache.fDBScan = fFalse;
    cbftrace++;

    m_rgBfTrace[cbftrace].traceid = bftidTouch;
    m_rgBfTrace[cbftrace].tick = (TICK)0xFFFFFFFD;
    m_rgBfTrace[cbftrace].bftouch.ifmp = 2;
    m_rgBfTrace[cbftrace].bftouch.pgno = 567;
    m_rgBfTrace[cbftrace].bftouch.bflt = 0x65;
    m_rgBfTrace[cbftrace].bftouch.clientType = 0x87;
    m_rgBfTrace[cbftrace].bftouch.pctPri = 33;
    m_rgBfTrace[cbftrace].bftouch.fUseHistory = fFalse;
    m_rgBfTrace[cbftrace].bftouch.fNewPage = fTrue;
    m_rgBfTrace[cbftrace].bftouch.fNoTouch = fFalse;
    m_rgBfTrace[cbftrace].bftouch.fDBScan = fFalse;
    cbftrace++;

    m_rgBfTrace[cbftrace].traceid = bftidDirty;
    m_rgBfTrace[cbftrace].tick = (TICK)0xFFFFFFFD;
    m_rgBfTrace[cbftrace].bfdirty.ifmp = 2;
    m_rgBfTrace[cbftrace].bfdirty.pgno = 565;
    m_rgBfTrace[cbftrace].bfdirty.bfdf = bfdfDirty;
    m_rgBfTrace[cbftrace].bfdirty.lgenModify = 1;
    m_rgBfTrace[cbftrace].bfdirty.isecModify = 2;
    m_rgBfTrace[cbftrace].bfdirty.ibModify = 3;
    cbftrace++;

    m_rgBfTrace[cbftrace].traceid = bftidSetLgposModify;
    m_rgBfTrace[cbftrace].tick = (TICK)0xFFFFFFFD;
    m_rgBfTrace[cbftrace].bfsetlgposmodify.ifmp = 2;
    m_rgBfTrace[cbftrace].bfsetlgposmodify.pgno = 565;
    m_rgBfTrace[cbftrace].bfsetlgposmodify.lgenModify = 3;
    m_rgBfTrace[cbftrace].bfsetlgposmodify.isecModify = 2;
    m_rgBfTrace[cbftrace].bfsetlgposmodify.ibModify = 1;
    cbftrace++;

    m_rgBfTrace[cbftrace].traceid = bftidWrite;
    m_rgBfTrace[cbftrace].tick = (TICK)0xFFFFFFFD;
    m_rgBfTrace[cbftrace].bfwrite.ifmp = 2;
    m_rgBfTrace[cbftrace].bfwrite.pgno = 564;
    m_rgBfTrace[cbftrace].bfwrite.bfdf = bfdfFilthy;
    m_rgBfTrace[cbftrace].bfwrite.iorp = iorpBFAvailPool;
    cbftrace++;

    m_rgBfTrace[cbftrace].traceid = bftidSuperCold;
    m_rgBfTrace[cbftrace].tick = (TICK)0x00000005;
    m_rgBfTrace[cbftrace].bfevict.ifmp = 2;
    m_rgBfTrace[cbftrace].bfevict.pgno = 321;
    cbftrace++;

    m_rgBfTrace[cbftrace].traceid = bftidSysResMgrTerm;
    m_rgBfTrace[cbftrace].tick = (TICK)0x00000010;
    cbftrace++;

    m_rgBfTrace[cbftrace].traceid = bftidTouch;
    m_rgBfTrace[cbftrace].tick = (TICK)0x00000030;
    m_rgBfTrace[cbftrace].bftouch.ifmp = 3;
    m_rgBfTrace[cbftrace].bftouch.pgno = 555;
    m_rgBfTrace[cbftrace].bftouch.bflt = 0x56;
    m_rgBfTrace[cbftrace].bftouch.clientType = 0x78;
    m_rgBfTrace[cbftrace].bftouch.pctPri = 44;
    m_rgBfTrace[cbftrace].bftouch.fUseHistory = fTrue;
    m_rgBfTrace[cbftrace].bftouch.fNewPage = fTrue;
    m_rgBfTrace[cbftrace].bftouch.fNoTouch = fTrue;
    m_rgBfTrace[cbftrace].bftouch.fDBScan = fTrue;
    cbftrace++;

    m_rgBfTrace[cbftrace].traceid = bftidInvalid;
    cbftrace++;

    TestAssert( s_cbftrace == cbftrace );


    size_t cetwevt = 0;

    memset( &m_rgpEtwEvt, 0, sizeof(m_rgpEtwEvt) );
    TestAssert( _countof( m_rgpEtwEvt ) == s_cetwevt );
    const size_t ibExtraData = roundup( sizeof(EtwEvent), sizeof(void*) );
    const size_t cbExtraData = 100;
    const size_t cbEtwEvent = ibExtraData + cbExtraData;
    for ( INT i = 0; i < _countof( m_rgpEtwEvt ) - 1; i++ )
    {
        m_rgpEtwEvt[i] = (EtwEvent*)( new BYTE[cbEtwEvent] );
        memset( m_rgpEtwEvt[i], 0, cbEtwEvent );
        m_rgpEtwEvt[i]->timestamp = i;
        m_rgpEtwEvt[i]->ibExtraData = ibExtraData;

        EventHeader* pEventHeader = (EventHeader*)( (BYTE*)m_rgpEtwEvt[i] + m_rgpEtwEvt[i]->ibExtraData );
        pEventHeader->ulProcessId = 333;
        pEventHeader->ulThreadId = 999;
    }

    EseBfCachePage* pEseBfCachePage = (EseBfCachePage*)( (BYTE*)m_rgpEtwEvt[cetwevt] + m_rgpEtwEvt[cetwevt]->ibExtraData );
    m_rgpEtwEvt[cetwevt]->etwEvtType = etwevttEseBfCachePage;
    m_rgpEtwEvt[cetwevt]->cbExtraData = sizeof(EseBfCachePage);
    pEseBfCachePage->tick = (TICK)0xFFFFFFFD;
    pEseBfCachePage->ifmp = 1;
    pEseBfCachePage->pgno = 234;
    pEseBfCachePage->bflt = 0x56;
    pEseBfCachePage->clientType = 0x78;
    pEseBfCachePage->pctPriority = 22;
    pEseBfCachePage->bfrtf = BFRequestTraceFlags( bfrtfUseHistory );
    cetwevt++;

    EseTaskManagerRun* pEseTaskManagerRun = (EseTaskManagerRun*)( (BYTE*)m_rgpEtwEvt[cetwevt] + m_rgpEtwEvt[cetwevt]->ibExtraData );
    m_rgpEtwEvt[cetwevt]->etwEvtType = etwevttEseTaskManagerRun;
    m_rgpEtwEvt[cetwevt]->cbExtraData = sizeof(EseTaskManagerRun);
    cetwevt++;

    EseResMgrInit* pEseResMgrInit = (EseResMgrInit*)( (BYTE*)m_rgpEtwEvt[cetwevt] + m_rgpEtwEvt[cetwevt]->ibExtraData );
    m_rgpEtwEvt[cetwevt]->etwEvtType = etwevttEseResMgrInit;
    m_rgpEtwEvt[cetwevt]->cbExtraData = sizeof(EseResMgrInit);
    pEseResMgrInit->tick = (TICK)0xFFFFFFFC;
    pEseResMgrInit->K = 3;
    pEseResMgrInit->csecCorrelatedTouch = 0.1234;
    pEseResMgrInit->csecTimeout = 5678.0;
    pEseResMgrInit->csecUncertainty = 432.1;
    pEseResMgrInit->dblHashLoadFactor = 987.6;
    pEseResMgrInit->dblHashUniformity = 123.4;
    pEseResMgrInit->dblSpeedSizeTradeoff = 5.67;
    cetwevt++;

    pEseTaskManagerRun = (EseTaskManagerRun*)( (BYTE*)m_rgpEtwEvt[cetwevt] + m_rgpEtwEvt[cetwevt]->ibExtraData );
    m_rgpEtwEvt[cetwevt]->etwEvtType = etwevttEseTaskManagerRun;
    m_rgpEtwEvt[cetwevt]->cbExtraData = sizeof(EseTaskManagerRun);
    cetwevt++;

    EseBfEvictPage* pEseBfEvictPage = (EseBfEvictPage*)( (BYTE*)m_rgpEtwEvt[cetwevt] + m_rgpEtwEvt[cetwevt]->ibExtraData );
    m_rgpEtwEvt[cetwevt]->etwEvtType = etwevttEseBfEvictPage;
    m_rgpEtwEvt[cetwevt]->cbExtraData = sizeof(EseBfEvictPage);
    pEseBfEvictPage->tick = (TICK)0xFFFFFFFA;
    pEseBfEvictPage->ifmp = 1;
    pEseBfEvictPage->pgno = 123;
    pEseBfEvictPage->fCurrentVersion = fTrue;
    pEseBfEvictPage->errBF = -1019;
    pEseBfEvictPage->bfef = 0x89;
    pEseBfEvictPage->pctPriority = 44;
    cetwevt++;

    pEseTaskManagerRun = (EseTaskManagerRun*)( (BYTE*)m_rgpEtwEvt[cetwevt] + m_rgpEtwEvt[cetwevt]->ibExtraData );
    m_rgpEtwEvt[cetwevt]->etwEvtType = etwevttEseTaskManagerRun;
    m_rgpEtwEvt[cetwevt]->cbExtraData = sizeof(EseTaskManagerRun);
    cetwevt++;

    EseBfRequestPage* pEseBfRequestPage = (EseBfRequestPage*)( (BYTE*)m_rgpEtwEvt[cetwevt] + m_rgpEtwEvt[cetwevt]->ibExtraData );
    m_rgpEtwEvt[cetwevt]->etwEvtType = etwevttEseBfRequestPage;
    m_rgpEtwEvt[cetwevt]->cbExtraData = sizeof(EseBfRequestPage);
    pEseBfRequestPage->tick = (TICK)0xFFFFFFFD;
    pEseBfRequestPage->ifmp = 2;
    pEseBfRequestPage->pgno = 567;
    pEseBfRequestPage->bflt = 0x65;
    pEseBfRequestPage->clientType = 0x87;
    pEseBfRequestPage->pctPriority = 33;
    pEseBfRequestPage->bfrtf = BFRequestTraceFlags( bfrtfNewPage );
    cetwevt++;

    EseBfDirtyPage* pEseBfDirtyPage = (EseBfDirtyPage*)( (BYTE*)m_rgpEtwEvt[cetwevt] + m_rgpEtwEvt[cetwevt]->ibExtraData );
    m_rgpEtwEvt[cetwevt]->etwEvtType = etwevttEseBfDirtyPage;
    m_rgpEtwEvt[cetwevt]->cbExtraData = sizeof(EseBfDirtyPage);
    pEseBfDirtyPage->tick = (TICK)0xFFFFFFFD;
    pEseBfDirtyPage->ifmp = 2;
    pEseBfDirtyPage->pgno = 565;
    pEseBfDirtyPage->ulDirtyLevel = bfdfDirty;
    pEseBfDirtyPage->lgposModify.lGen = 1;
    pEseBfDirtyPage->lgposModify.isec = 2;
    pEseBfDirtyPage->lgposModify.ib = 3;
    cetwevt++;

    EseBfSetLgposModify* pEseBfSetLgposModify = (EseBfSetLgposModify*)( (BYTE*)m_rgpEtwEvt[cetwevt] + m_rgpEtwEvt[cetwevt]->ibExtraData );
    m_rgpEtwEvt[cetwevt]->etwEvtType = etwevttEseBfSetLgposModify;
    m_rgpEtwEvt[cetwevt]->cbExtraData = sizeof(EseBfSetLgposModify);
    pEseBfSetLgposModify->tick = (TICK)0xFFFFFFFD;
    pEseBfSetLgposModify->ifmp = 2;
    pEseBfSetLgposModify->pgno = 565;
    pEseBfSetLgposModify->lgposModify.lGen = 3;
    pEseBfSetLgposModify->lgposModify.isec = 2;
    pEseBfSetLgposModify->lgposModify.ib = 1;
    cetwevt++;

    EseBfWritePage* pEseBfWritePage = (EseBfWritePage*)( (BYTE*)m_rgpEtwEvt[cetwevt] + m_rgpEtwEvt[cetwevt]->ibExtraData );
    m_rgpEtwEvt[cetwevt]->etwEvtType = etwevttEseBfWritePage;
    m_rgpEtwEvt[cetwevt]->cbExtraData = sizeof(EseBfWritePage);
    pEseBfWritePage->tick = (TICK)0xFFFFFFFD;
    pEseBfWritePage->ifmp = 2;
    pEseBfWritePage->pgno = 564;
    pEseBfWritePage->ulDirtyLevel = bfdfFilthy;
    pEseBfWritePage->tcevtBf.bIorp = iorpBFAvailPool;
    cetwevt++;

    EseResMgrTerm* pEseResMgrTerm = (EseResMgrTerm*)( (BYTE*)m_rgpEtwEvt[cetwevt] + m_rgpEtwEvt[cetwevt]->ibExtraData );
    m_rgpEtwEvt[cetwevt]->etwEvtType = etwevttEseResMgrTerm;
    m_rgpEtwEvt[cetwevt]->cbExtraData = sizeof(EseResMgrTerm);
    pEseResMgrTerm->tick = (TICK)0x00000010;
    cetwevt++;

    EseBfMarkPageAsSuperCold* pEseBfMarkPageAsSuperCold = (EseBfMarkPageAsSuperCold*)( (BYTE*)m_rgpEtwEvt[cetwevt] + m_rgpEtwEvt[cetwevt]->ibExtraData );
    m_rgpEtwEvt[cetwevt]->etwEvtType = etwevttEseBfMarkPageAsSuperCold;
    m_rgpEtwEvt[cetwevt]->cbExtraData = sizeof(EseBfMarkPageAsSuperCold);
    pEseBfMarkPageAsSuperCold->tick = (TICK)0x00000005;
    pEseBfMarkPageAsSuperCold->ifmp = 2;
    pEseBfMarkPageAsSuperCold->pgno = 321;
    cetwevt++;

    pEseBfRequestPage = (EseBfRequestPage*)( (BYTE*)m_rgpEtwEvt[cetwevt] + m_rgpEtwEvt[cetwevt]->ibExtraData );
    ( (EventHeader*)pEseBfRequestPage )->ulProcessId = 334;
    m_rgpEtwEvt[cetwevt]->etwEvtType = etwevttEseBfRequestPage;
    m_rgpEtwEvt[cetwevt]->cbExtraData = sizeof(EseBfRequestPage);
    pEseBfRequestPage->tick = (TICK)0x00000030;
    pEseBfRequestPage->ifmp = 3;
    pEseBfRequestPage->pgno = 555;
    pEseBfRequestPage->bflt = 0x56;
    pEseBfRequestPage->clientType = 0x78;
    pEseBfRequestPage->pctPriority = 44;
    pEseBfRequestPage->bfrtf = BFRequestTraceFlags( bfrtfUseHistory | bfrtfNewPage | bfrtfNoTouch | bfrtfDBScan );
    cetwevt++;

    TestAssert( ( s_cetwevt - 1 ) == cetwevt );

    for ( INT i = 0; i < _countof( m_rgpEtwEvt ) - 1; i++ )
    {
        TestAssert( m_rgpEtwEvt[i]->cbExtraData <= cbExtraData );
    }
}

void ETLDriverTest::CleanupTestData_()
{
    for ( INT i = 0; i < _countof( m_rgpEtwEvt ); i++ )
    {
        if ( m_rgpEtwEvt[i] )
        {
            delete []( (BYTE*)m_rgpEtwEvt[i] );
            m_rgpEtwEvt[i] = NULL;
        }
    }
}

ERR ETLDriverTest::ErrInitWithInvalidParamsTest_()
{
    ERR err = JET_errSuccess;
    BFETLContext* bffetlc = NULL;
    EtwEvent* petwEvent = NULL;
    EtwEvent** ppetwEvent = &petwEvent;
    std::set<DWORD> pids;

    printf( "\t%s\r\n", __FUNCTION__ );

    printf( "NULL pvTraceProvider.\r\n" );
    TestCheck( ErrBFETLInit( NULL, 1234, 1, fBFETLDriverTestMode, &bffetlc ) == JET_errInvalidParameter );
    TestCheck( bffetlc == NULL );

    printf( "NULL BFETLContext.\r\n" );
    TestCheck( ErrBFETLInit( ppetwEvent, 1234, 1, fBFETLDriverTestMode, NULL ) == JET_errInvalidParameter );

    printf( "Invalid file name.\r\n" );
    TestCheck( ErrBFETLInit( L"AVeryUnlikelyToExistEtlFileName.etl", 1234, 1, 0, &bffetlc ) == JET_errOutOfMemory );
    TestCheck( bffetlc == NULL );

    printf( "Invalid cEventsBufferedMax.\r\n" );
    TestCheck( ErrBFETLInit( ppetwEvent, 1234, 0, fBFETLDriverTestMode, &bffetlc ) == JET_errInvalidParameter );
    TestCheck( bffetlc == NULL );

    printf( "Invalid PID set (empty).\r\n" );
    pids.clear();
    TestCheck( ErrBFETLInit( ppetwEvent, pids, 1, fBFETLDriverTestMode, &bffetlc ) == JET_errInvalidParameter );
    TestCheck( bffetlc == NULL );

    printf( "Invalid PID set (PID zero with other PIDs).\r\n" );
    pids.clear();
    pids.insert( 1234 );
    pids.insert( 0 );
    TestCheck( ErrBFETLInit( ppetwEvent, pids, 1, fBFETLDriverTestMode, &bffetlc ) == JET_errInvalidParameter );
    TestCheck( bffetlc == NULL );

HandleError:
    return err;
}

ERR ETLDriverTest::ErrTermWithNullPointer_()
{
    printf( "\t%s\r\n", __FUNCTION__ );
    
    printf( "NULL BFETLContext\r\n" );
    BFETLTerm( NULL );

    return JET_errSuccess;
}

ERR ETLDriverTest::ErrGetNextWithInvalidParamsTest_()
{
    ERR err = JET_errSuccess;
    BFETLContext* bffetlc = NULL;
    EtwEvent* petwEvent = NULL;
    EtwEvent** ppetwEvent = &petwEvent;
    BFTRACE bftrace;

    printf( "\t%s\r\n", __FUNCTION__ );

    TestCheckErr( ErrBFETLInit( ppetwEvent, 1234, 1, fBFETLDriverTestMode, &bffetlc ) );
    TestCheck( bffetlc != NULL );

    printf( "NULL BFETLContext.\r\n" );
    TestCheck( ErrBFETLGetNext( NULL, &bftrace ) == JET_errInvalidParameter );

    printf( "NULL BFTRACE.\r\n" );
    TestCheck( ErrBFETLGetNext( NULL, NULL ) == JET_errInvalidParameter );

HandleError:

    BFETLTerm( bffetlc );
    
    return err;
}

ERR ETLDriverTest::ErrBasicInitTermTest_()
{
    ERR err = JET_errSuccess;
    BFETLContext* bffetlc = NULL;
    EtwEvent* petwEvent = NULL;
    EtwEvent** ppetwEvent = &petwEvent;
    BFTRACE bftrace;

    printf( "\t%s\r\n", __FUNCTION__ );

    TestCheckErr( ErrBFETLInit( ppetwEvent, 1234, 1, fBFETLDriverTestMode, &bffetlc ) );
    TestCheck( bffetlc != NULL );
    TestCheck( bffetlc->cTracesOutOfOrder == 0 );
    TestCheck( bffetlc->cTracesProcessed == 0 );
    TestCheck( bffetlc->bftstats.cSysResMgrInit == 0 );
    TestCheck( bffetlc->bftstats.cSysResMgrTerm == 0 );
    TestCheck( bffetlc->bftstats.cCache == 0 );
    TestCheck( bffetlc->bftstats.cRequest == 0 );
    TestCheck( bffetlc->bftstats.cDirty == 0 );
    TestCheck( bffetlc->bftstats.cWrite == 0 );
    TestCheck( bffetlc->bftstats.cSetLgposModify == 0 );
    TestCheck( bffetlc->bftstats.cEvict == 0 );
    TestCheck( bffetlc->bftstats.cSuperCold == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr().le_cifmp == 0 );

HandleError:

    BFETLTerm( bffetlc );
    
    return err;
}

ERR ETLDriverTest::ErrBasicInitGetTermTest_()
{
    ERR err = JET_errSuccess;
    BFETLContext* bffetlc = NULL;
    EtwEvent* petwEvent = NULL;
    EtwEvent** ppetwEvent = &petwEvent;
    BFTRACE bftrace;

    printf( "\t%s\r\n", __FUNCTION__ );

    TestCheckErr( ErrBFETLInit( ppetwEvent, 1234, 1, fBFETLDriverTestMode, &bffetlc ) );
    
    TestCheck( ErrBFETLGetNext( bffetlc, &bftrace ) == errNotFound );
    TestCheck( bffetlc != NULL );
    TestCheck( bffetlc->cTracesOutOfOrder == 0 );
    TestCheck( bffetlc->cTracesProcessed == 0 );
    TestCheck( bffetlc->bftstats.cSysResMgrInit == 0 );
    TestCheck( bffetlc->bftstats.cSysResMgrTerm == 0 );
    TestCheck( bffetlc->bftstats.cCache == 0 );
    TestCheck( bffetlc->bftstats.cRequest == 0 );
    TestCheck( bffetlc->bftstats.cDirty == 0 );
    TestCheck( bffetlc->bftstats.cWrite == 0 );
    TestCheck( bffetlc->bftstats.cSetLgposModify == 0 );
    TestCheck( bffetlc->bftstats.cEvict == 0 );
    TestCheck( bffetlc->bftstats.cSuperCold == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr().le_cifmp == 0 );

HandleError:

    BFETLTerm( bffetlc );
    
    return err;
}

ERR ETLDriverTest::ErrBasicInitTermMultiPidTest_()
{
    ERR err = JET_errSuccess;
    BFETLContext* bffetlc = NULL;
    EtwEvent* petwEvent = NULL;
    EtwEvent** ppetwEvent = &petwEvent;
    BFTRACE bftrace;
    std::set<DWORD> pids;
    pids.insert( 1234 );
    pids.insert( 5678 );

    printf( "\t%s\r\n", __FUNCTION__ );

    TestCheckErr( ErrBFETLInit( ppetwEvent, pids, 1, fBFETLDriverTestMode, &bffetlc ) );
    TestCheck( bffetlc != NULL );
    TestCheck( bffetlc->cTracesOutOfOrder == 0 );
    TestCheck( bffetlc->cTracesProcessed == 0 );
    TestCheck( bffetlc->bftstats.cSysResMgrInit == 0 );
    TestCheck( bffetlc->bftstats.cSysResMgrTerm == 0 );
    TestCheck( bffetlc->bftstats.cCache == 0 );
    TestCheck( bffetlc->bftstats.cRequest == 0 );
    TestCheck( bffetlc->bftstats.cDirty == 0 );
    TestCheck( bffetlc->bftstats.cWrite == 0 );
    TestCheck( bffetlc->bftstats.cSetLgposModify == 0 );
    TestCheck( bffetlc->bftstats.cEvict == 0 );
    TestCheck( bffetlc->bftstats.cSuperCold == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr( 1234 ).le_cifmp == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr( 5678 ).le_cifmp == 0 );

HandleError:

    BFETLTerm( bffetlc );
    
    return err;
}

ERR ETLDriverTest::ErrBasicInitGetTermMultiPidTest_()
{
    ERR err = JET_errSuccess;
    BFETLContext* bffetlc = NULL;
    EtwEvent* petwEvent = NULL;
    EtwEvent** ppetwEvent = &petwEvent;
    BFTRACE bftrace;
    std::set<DWORD> pids;
    pids.insert( 1234 );
    pids.insert( 5678 );

    printf( "\t%s\r\n", __FUNCTION__ );

    TestCheckErr( ErrBFETLInit( ppetwEvent, pids, 1, fBFETLDriverTestMode, &bffetlc ) );
    
    TestCheck( ErrBFETLGetNext( bffetlc, &bftrace ) == errNotFound );
    TestCheck( bffetlc != NULL );
    TestCheck( bffetlc->cTracesOutOfOrder == 0 );
    TestCheck( bffetlc->cTracesProcessed == 0 );
    TestCheck( bffetlc->bftstats.cSysResMgrInit == 0 );
    TestCheck( bffetlc->bftstats.cSysResMgrTerm == 0 );
    TestCheck( bffetlc->bftstats.cCache == 0 );
    TestCheck( bffetlc->bftstats.cRequest == 0 );
    TestCheck( bffetlc->bftstats.cDirty == 0 );
    TestCheck( bffetlc->bftstats.cWrite == 0 );
    TestCheck( bffetlc->bftstats.cSetLgposModify == 0 );
    TestCheck( bffetlc->bftstats.cEvict == 0 );
    TestCheck( bffetlc->bftstats.cSuperCold == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr( 1234 ).le_cifmp == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr( 5678 ).le_cifmp == 0 );

HandleError:

    BFETLTerm( bffetlc );
    
    return err;
}

ERR ETLDriverTest::ErrGetNextSmallBufferTest_()
{
    ERR err = JET_errSuccess;
    BFETLContext* bffetlc = NULL;
    BFTRACE bftrace;

    printf( "\t%s\r\n", __FUNCTION__ );

    SetupTestData_();
    TestCheckErr( ErrBFETLInit( m_rgpEtwEvt, 333, 1, fBFETLDriverTestMode, &bffetlc ) );

    TestCheckErr( ErrBFETLGetNext( bffetlc, &bftrace ) );
    TestCheck( memcmp( &bftrace, &m_rgBfTrace[1], sizeof(bftrace) ) == 0 );
    TestCheck( bffetlc->bftstats.cSysResMgrInit == 1 );
    TestCheck( bffetlc->cTracesOutOfOrder == 1 );
    TestCheck( bffetlc->cTracesProcessed == 1 );
    TestCheck( bffetlc->bfftlPostProcHdr().le_cifmp == 0 );

    TestCheck( ErrBFETLGetNext( bffetlc, &bftrace ) == JET_errInternalError );
    TestCheck( bffetlc->cTracesOutOfOrder == 2 );
    TestCheck( bffetlc->cTracesProcessed == 1 );

HandleError:

    BFETLTerm( bffetlc );
    CleanupTestData_();

    return err;
}

ERR ETLDriverTest::ErrGetNextSuccessfulTest_( const DWORD cEventsBufferedMax, const INT iPID )
{
    ERR err = JET_errSuccess;
    BFETLContext* bffetlc = NULL;
    BFTRACE bftrace;
    DWORD dwPIDActual;
    const bool fMultiPid = ( iPID < 0 );
    DWORD dwPIDQuery = fMultiPid ? 333 : (DWORD)iPID;
    const __int64 cTracesOutOfOrder1 = ( cEventsBufferedMax < 6 ) ? 2 : 3;
    const __int64 cTracesOutOfOrder2 = ( cEventsBufferedMax < 5 ) ? 2 : 3;
    const __int64 cTracesOutOfOrder3 = 3;

    SetupTestData_();
    if ( !fMultiPid )
    {
        TestCheckErr( ErrBFETLInit( m_rgpEtwEvt, (DWORD)iPID, cEventsBufferedMax, fBFETLDriverTestMode, &bffetlc ) );
    }
    else
    {
        std::set<DWORD> pids;
        pids.insert( 333 );
        pids.insert( 334 );
        pids.insert( 335 );
        
        TestCheckErr( ErrBFETLInit( m_rgpEtwEvt, pids, cEventsBufferedMax, fBFETLDriverTestMode, &bffetlc ) );
    }

    size_t cbftrace = 0;

    TestCheck( bffetlc->bftstats.cEvict == 0 );
    dwPIDActual = 0;
    TestCheckErr( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) );
    TestCheck( dwPIDActual == 333 );
    TestCheck( memcmp( &bftrace, &m_rgBfTrace[cbftrace++], sizeof(bftrace) ) == 0 );
    TestCheck( bffetlc->bftstats.cEvict == 1 );
    TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder1 );
    TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_cifmp == 2 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[0] == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[1] == 123 );

    TestCheck( bffetlc->bftstats.cSysResMgrInit == 0 );
    dwPIDActual = 0;
    TestCheckErr( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) );
    TestCheck( dwPIDActual == 333 );
    TestCheck( memcmp( &bftrace, &m_rgBfTrace[cbftrace++], sizeof(bftrace) ) == 0 );
    TestCheck( bffetlc->bftstats.cSysResMgrInit == 1 );
    TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder1 );
    TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_cifmp == 2 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[0] == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[1] == 123 );

    TestCheck( bffetlc->bftstats.cCache == 0 );
    dwPIDActual = 0;
    TestCheckErr( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) );
    TestCheck( dwPIDActual == 333 );
    TestCheck( memcmp( &bftrace, &m_rgBfTrace[cbftrace++], sizeof(bftrace) ) == 0 );
    TestCheck( bffetlc->bftstats.cCache == 1 );
    TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder1 );
    TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_cifmp == 2 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[0] == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[1] == 234 );

    TestCheck( bffetlc->bftstats.cRequest == 0 );
    dwPIDActual = 0;
    TestCheckErr( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) );
    TestCheck( dwPIDActual == 333 );
    TestCheck( memcmp( &bftrace, &m_rgBfTrace[cbftrace++], sizeof(bftrace) ) == 0 );
    TestCheck( bffetlc->bftstats.cRequest == 1 );
    TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder2 );
    TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_cifmp == 3 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[0] == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[1] == 234 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[2] == 567 );

    TestCheck( bffetlc->bftstats.cDirty == 0 );
    dwPIDActual = 0;
    TestCheckErr( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) );
    TestCheck( dwPIDActual == 333 );
    TestCheck( memcmp( &bftrace, &m_rgBfTrace[cbftrace++], sizeof(bftrace) ) == 0 );
    TestCheck( bffetlc->bftstats.cDirty == 1 );
    TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder3 );
    TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_cifmp == 3 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[0] == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[1] == 234 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[2] == 567 );

    TestCheck( bffetlc->bftstats.cSetLgposModify == 0 );
    dwPIDActual = 0;
    TestCheckErr( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) );
    TestCheck( dwPIDActual == 333 );
    TestCheck( memcmp( &bftrace, &m_rgBfTrace[cbftrace++], sizeof(bftrace) ) == 0 );
    TestCheck( bffetlc->bftstats.cSetLgposModify == 1 );
    TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder3 );
    TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_cifmp == 3 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[0] == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[1] == 234 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[2] == 567 );

    TestCheck( bffetlc->bftstats.cWrite == 0 );
    dwPIDActual = 0;
    TestCheckErr( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) );
    TestCheck( dwPIDActual == 333 );
    TestCheck( memcmp( &bftrace, &m_rgBfTrace[cbftrace++], sizeof(bftrace) ) == 0 );
    TestCheck( bffetlc->bftstats.cWrite == 1 );
    TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder3 );
    TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_cifmp == 3 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[0] == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[1] == 234 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[2] == 567 );

    TestCheck( bffetlc->bftstats.cSuperCold == 0 );
    dwPIDActual = 0;
    TestCheckErr( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) );
    TestCheck( dwPIDActual == 333 );
    TestCheck( memcmp( &bftrace, &m_rgBfTrace[cbftrace++], sizeof(bftrace) ) == 0 );
    TestCheck( bffetlc->bftstats.cSuperCold == 1 );
    TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder3 );
    TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_cifmp == 3 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[0] == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[1] == 234 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[2] == 567 );

    TestCheck( bffetlc->bftstats.cSysResMgrTerm == 0 );
    dwPIDActual = 0;
    TestCheckErr( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) );
    TestCheck( dwPIDActual == 333 );
    TestCheck( memcmp( &bftrace, &m_rgBfTrace[cbftrace++], sizeof(bftrace) ) == 0 );
    TestCheck( bffetlc->bftstats.cSysResMgrTerm == 1 );
    TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder3 );
    TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_cifmp == 3 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[0] == 0 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[1] == 234 );
    TestCheck( bffetlc->bfftlPostProcHdr( dwPIDQuery ).le_mpifmpcpg[2] == 567 );

    if ( !fMultiPid )
    {
        if ( iPID == 0 )
        {
            TestCheck( bffetlc->bftstats.cRequest == 1 );
            dwPIDActual = 0;
            TestCheckErr( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) );
            TestCheck( dwPIDActual == 334 );
            TestCheck( memcmp( &bftrace, &m_rgBfTrace[cbftrace++], sizeof(bftrace) ) == 0 );
            TestCheck( bffetlc->bftstats.cRequest == 2 );
            TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder3 );
            TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
            TestCheck( bffetlc->bfftlPostProcHdr().le_cifmp == 4 );
            TestCheck( bffetlc->bfftlPostProcHdr().le_mpifmpcpg[0] == 0 );
            TestCheck( bffetlc->bfftlPostProcHdr().le_mpifmpcpg[1] == 234 );
            TestCheck( bffetlc->bfftlPostProcHdr().le_mpifmpcpg[2] == 567 );
            TestCheck( bffetlc->bfftlPostProcHdr().le_mpifmpcpg[3] == 555 );

            TestAssert( ( s_cbftrace - 1 ) == cbftrace );

            dwPIDActual = 0;
            TestCheck( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) == errNotFound );
            TestCheck( dwPIDActual == 0 );
            TestCheck( bffetlc->bftstats.cSysResMgrInit == 1 );
            TestCheck( bffetlc->bftstats.cSysResMgrTerm == 1 );
            TestCheck( bffetlc->bftstats.cCache == 1 );
            TestCheck( bffetlc->bftstats.cRequest == 2 );
            TestCheck( bffetlc->bftstats.cEvict == 1 );
            TestCheck( bffetlc->bftstats.cSuperCold == 1 );
            TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder3 );
            TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
            TestCheck( bffetlc->bfftlPostProcHdr().le_cifmp == 4 );
            TestCheck( bffetlc->bfftlPostProcHdr().le_mpifmpcpg[0] == 0 );
            TestCheck( bffetlc->bfftlPostProcHdr().le_mpifmpcpg[1] == 234 );
            TestCheck( bffetlc->bfftlPostProcHdr().le_mpifmpcpg[2] == 567 );
            TestCheck( bffetlc->bfftlPostProcHdr().le_mpifmpcpg[3] == 555 );
        }
        else
        {
            TestAssert( ( s_cbftrace - 2 ) == cbftrace );

            dwPIDActual = 0;
            TestCheck( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) == errNotFound );
            TestCheck( dwPIDActual == 0 );
            TestCheck( bffetlc->bftstats.cSysResMgrInit == 1 );
            TestCheck( bffetlc->bftstats.cSysResMgrTerm == 1 );
            TestCheck( bffetlc->bftstats.cCache == 1 );
            TestCheck( bffetlc->bftstats.cRequest == 1 );
            TestCheck( bffetlc->bftstats.cEvict == 1 );
            TestCheck( bffetlc->bftstats.cSuperCold == 1 );
            TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder3 );
            TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
            TestCheck( bffetlc->bfftlPostProcHdr().le_cifmp == 3 );
            TestCheck( bffetlc->bfftlPostProcHdr().le_mpifmpcpg[0] == 0 );
            TestCheck( bffetlc->bfftlPostProcHdr().le_mpifmpcpg[1] == 234 );
            TestCheck( bffetlc->bfftlPostProcHdr().le_mpifmpcpg[2] == 567 );
        }
    }
    else
    {
        TestCheck( bffetlc->bftstats.cRequest == 1 );
        dwPIDActual = 0;
        TestCheckErr( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) );
        TestCheck( dwPIDActual == 334 );
        TestCheck( memcmp( &bftrace, &m_rgBfTrace[cbftrace++], sizeof(bftrace) ) == 0 );
        TestCheck( bffetlc->bftstats.cRequest == 2 );
        TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder3 );
        TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
        TestCheck( bffetlc->bfftlPostProcHdr( 333 ).le_cifmp == 3 );
        TestCheck( bffetlc->bfftlPostProcHdr( 333 ).le_mpifmpcpg[0] == 0 );
        TestCheck( bffetlc->bfftlPostProcHdr( 333 ).le_mpifmpcpg[1] == 234 );
        TestCheck( bffetlc->bfftlPostProcHdr( 333 ).le_mpifmpcpg[2] == 567 );
        TestCheck( bffetlc->bfftlPostProcHdr( 334 ).le_cifmp == 4 );
        TestCheck( bffetlc->bfftlPostProcHdr( 334 ).le_mpifmpcpg[0] == 0 );
        TestCheck( bffetlc->bfftlPostProcHdr( 334 ).le_mpifmpcpg[1] == 0 );
        TestCheck( bffetlc->bfftlPostProcHdr( 334 ).le_mpifmpcpg[2] == 0 );
        TestCheck( bffetlc->bfftlPostProcHdr( 334 ).le_mpifmpcpg[3] == 555 );
        TestCheck( bffetlc->bfftlPostProcHdr( 335 ).le_cifmp == 0 );

        TestAssert( ( s_cbftrace - 1 ) == cbftrace );

        dwPIDActual = 0;
        TestCheck( ErrBFETLGetNext( bffetlc, &bftrace, &dwPIDActual ) == errNotFound );
        TestCheck( dwPIDActual == 0 );
        TestCheck( bffetlc->bftstats.cSysResMgrInit == 1 );
        TestCheck( bffetlc->bftstats.cSysResMgrTerm == 1 );
        TestCheck( bffetlc->bftstats.cCache == 1 );
        TestCheck( bffetlc->bftstats.cRequest == 2 );
        TestCheck( bffetlc->bftstats.cEvict == 1 );
        TestCheck( bffetlc->bftstats.cSuperCold == 1 );
        TestCheck( bffetlc->cTracesOutOfOrder == cTracesOutOfOrder3 );
        TestCheck( bffetlc->cTracesProcessed == (__int64)cbftrace );
        TestCheck( bffetlc->bfftlPostProcHdr( 333 ).le_cifmp == 3 );
        TestCheck( bffetlc->bfftlPostProcHdr( 333 ).le_mpifmpcpg[0] == 0 );
        TestCheck( bffetlc->bfftlPostProcHdr( 333 ).le_mpifmpcpg[1] == 234 );
        TestCheck( bffetlc->bfftlPostProcHdr( 333 ).le_mpifmpcpg[2] == 567 );
        TestCheck( bffetlc->bfftlPostProcHdr( 334 ).le_cifmp == 4 );
        TestCheck( bffetlc->bfftlPostProcHdr( 334 ).le_mpifmpcpg[0] == 0 );
        TestCheck( bffetlc->bfftlPostProcHdr( 334 ).le_mpifmpcpg[1] == 0 );
        TestCheck( bffetlc->bfftlPostProcHdr( 334 ).le_mpifmpcpg[2] == 0 );
        TestCheck( bffetlc->bfftlPostProcHdr( 334 ).le_mpifmpcpg[3] == 555 );
        TestCheck( bffetlc->bfftlPostProcHdr( 335 ).le_cifmp == 0 );
    }

HandleError:

    BFETLTerm( bffetlc );
    CleanupTestData_();

    return err;
}

ERR ETLDriverTest::ErrGetNextMediumBufferTest_()
{
    printf( "\t%s\r\n", __FUNCTION__ );

    return ErrGetNextSuccessfulTest_( 4, 333 );
}

ERR ETLDriverTest::ErrGetNextLargeBufferTest_()
{
    printf( "\t%s\r\n", __FUNCTION__ );

    return ErrGetNextSuccessfulTest_( 9, 333 );
}

ERR ETLDriverTest::ErrGetNextHugeBufferTest_()
{
    printf( "\t%s\r\n", __FUNCTION__ );

    return ErrGetNextSuccessfulTest_( 13, 333 );
}

ERR ETLDriverTest::ErrGetNextAllPidsTest_()
{
    printf( "\t%s\r\n", __FUNCTION__ );

    return ErrGetNextSuccessfulTest_( 5, 0 );
}

ERR ETLDriverTest::ErrGetNextMultiPidsTest_()
{
    printf( "\t%s\r\n", __FUNCTION__ );

    return ErrGetNextSuccessfulTest_( 5, -1 );
}

ERR ETLDriverTest::ErrCmpEtlTooManyTest_()
{
    ERR err = JET_errSuccess;
    __int64 cTracesProcessed = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    SetupTestData_();
    m_rgBfTrace[3].traceid = bftidInvalid;

    TestCheck( ErrBFETLFTLCmp( m_rgpEtwEvt, m_rgBfTrace, 0, fTrue, &cTracesProcessed ) == JET_errDatabaseCorrupted );
    TestCheck( cTracesProcessed == 3 );

HandleError:

    CleanupTestData_();

    return err;
}

ERR ETLDriverTest::ErrCmpFtlTooManyTest_()
{
    ERR err = JET_errSuccess;
    __int64 cTracesProcessed = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    SetupTestData_();
    delete []( (BYTE*)m_rgpEtwEvt[10] );
    m_rgpEtwEvt[10] = NULL;

    TestCheck( ErrBFETLFTLCmp( m_rgpEtwEvt, m_rgBfTrace, 0, fTrue, &cTracesProcessed ) == JET_errDatabaseCorrupted );
    TestCheck( cTracesProcessed == 7 );

HandleError:

    CleanupTestData_();

    return err;
}

ERR ETLDriverTest::ErrCmpTraceMismatchTest_()
{
    ERR err = JET_errSuccess;
    __int64 cTracesProcessed = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    SetupTestData_();

    m_rgBfTrace[2].tick++;
    TestCheck( ErrBFETLFTLCmp( m_rgpEtwEvt, m_rgBfTrace, 0, fTrue, &cTracesProcessed ) == JET_errDatabaseCorrupted );
    TestCheck( cTracesProcessed == 2 );

HandleError:

    CleanupTestData_();

    return err;
}

ERR ETLDriverTest::ErrCmpSuccessEmptyTest_()
{
    ERR err = JET_errSuccess;
    __int64 cTracesProcessed = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    SetupTestData_();
    m_rgBfTrace[0].traceid = bftidInvalid;
    delete []( (BYTE*)m_rgpEtwEvt[0] );
    m_rgpEtwEvt[0] = NULL;

    TestCheckErr( ErrBFETLFTLCmp( m_rgpEtwEvt, m_rgBfTrace, 0, fTrue, &cTracesProcessed ) );
    TestCheck( cTracesProcessed == 0 );

HandleError:

    CleanupTestData_();

    return err;
}

ERR ETLDriverTest::ErrCmpSuccessAllPidsTest_()
{
    ERR err = JET_errSuccess;
    __int64 cTracesProcessed = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    SetupTestData_();

    TestCheckErr( ErrBFETLFTLCmp( m_rgpEtwEvt, m_rgBfTrace, 0, fTrue, &cTracesProcessed ) );
    TestCheck( cTracesProcessed == (__int64)( s_cbftrace - 1 ) );

HandleError:

    CleanupTestData_();

    return err;
}

ERR ETLDriverTest::ErrCmpSuccessTest_()
{
    ERR err = JET_errSuccess;
    __int64 cTracesProcessed = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    SetupTestData_();
    m_rgBfTrace[s_cbftrace - 2].traceid = bftidInvalid;

    TestCheckErr( ErrBFETLFTLCmp( m_rgpEtwEvt, m_rgBfTrace, 333, fTrue, &cTracesProcessed ) );
    TestCheck( cTracesProcessed == (__int64)( s_cbftrace - 2 ) );

HandleError:

    CleanupTestData_();

    return err;
}

ERR ETLDriverTest::ErrStatsByPidAllPidsTest_()
{
    ERR err = JET_errSuccess;
    BFETLContext* bffetlc = NULL;
    BFTRACE bftrace;

    SetupTestData_();
    TestCheckErr( ErrBFETLInit( m_rgpEtwEvt, 0, s_cetwevt / 2, fBFETLDriverTestMode | fBFETLDriverCollectBFStats, &bffetlc ) );

    while ( ( err = ErrBFETLGetNext( bffetlc, &bftrace ) ) == JET_errSuccess );

    TestAssert( err == errNotFound );
    err = JET_errSuccess;

    const BFTraceStats* bfts = NULL;
    DWORD dwPID;

    dwPID = 0;
    bfts = PbftsBFETLStatsByPID( bffetlc, &dwPID );
    TestCheck( bfts != NULL );
    TestCheck( dwPID == 333 );
    TestCheck( bfts->cSysResMgrInit == 1 );
    TestCheck( bfts->cSysResMgrTerm == 1 );
    TestCheck( bfts->cCache == 1 );
    TestCheck( bfts->cRequest == 1 );
    TestCheck( bfts->cDirty == 1 );
    TestCheck( bfts->cWrite == 1 );
    TestCheck( bfts->cSetLgposModify == 1 );
    TestCheck( bfts->cEvict == 1 );
    TestCheck( bfts->cSuperCold == 1 );

    dwPID = 1;
    bfts = PbftsBFETLStatsByPID( bffetlc, &dwPID );
    TestCheck( bfts != NULL );
    TestCheck( dwPID == 334 );
    TestCheck( bfts->cSysResMgrInit == 0 );
    TestCheck( bfts->cSysResMgrTerm == 0 );
    TestCheck( bfts->cCache == 0 );
    TestCheck( bfts->cRequest == 1 );
    TestCheck( bfts->cEvict == 0 );
    TestCheck( bfts->cSuperCold == 0 );

    dwPID = 2;
    bfts = PbftsBFETLStatsByPID( bffetlc, &dwPID );
    TestCheck( bfts == NULL );
    TestCheck( dwPID == 0 );

HandleError:

    BFETLTerm( bffetlc );
    CleanupTestData_();

    return err;
}

ERR ETLDriverTest::ErrStatsByPidOnePidTest_()
{
    ERR err = JET_errSuccess;
    BFETLContext* bffetlc = NULL;
    BFTRACE bftrace;

    SetupTestData_();
    TestCheckErr( ErrBFETLInit( m_rgpEtwEvt, 333, s_cetwevt / 2, fBFETLDriverTestMode | fBFETLDriverCollectBFStats, &bffetlc ) );

    while ( ( err = ErrBFETLGetNext( bffetlc, &bftrace ) ) == JET_errSuccess );

    TestAssert( err == errNotFound );
    err = JET_errSuccess;

    const BFTraceStats* bfts = NULL;
    DWORD dwPID;

    dwPID = 0;
    bfts = PbftsBFETLStatsByPID( bffetlc, &dwPID );
    TestCheck( bfts != NULL );
    TestCheck( dwPID == 333 );
    TestCheck( bfts->cSysResMgrInit == 1 );
    TestCheck( bfts->cSysResMgrTerm == 1 );
    TestCheck( bfts->cCache == 1 );
    TestCheck( bfts->cRequest == 1 );
    TestCheck( bfts->cDirty == 1 );
    TestCheck( bfts->cWrite == 1 );
    TestCheck( bfts->cSetLgposModify == 1 );
    TestCheck( bfts->cEvict == 1 );
    TestCheck( bfts->cSuperCold == 1 );

    dwPID = 1;
    bfts = PbftsBFETLStatsByPID( bffetlc, &dwPID );
    TestCheck( bfts == NULL );
    TestCheck( dwPID == 0 );

HandleError:

    BFETLTerm( bffetlc );
    CleanupTestData_();

    return err;
}

ERR ETLDriverTest::ErrStatsByPidInvalidParamsTest_()
{
    ERR err = JET_errSuccess;
    BFETLContext* bffetlc = NULL;
    BFTRACE bftrace;

    SetupTestData_();
    TestCheckErr( ErrBFETLInit( m_rgpEtwEvt, 333, s_cetwevt / 2, fBFETLDriverTestMode | fBFETLDriverCollectBFStats, &bffetlc ) );

    while ( ( err = ErrBFETLGetNext( bffetlc, &bftrace ) ) == JET_errSuccess );

    TestAssert( err == errNotFound );
    err = JET_errSuccess;

    const BFTraceStats* bfts = NULL;
    DWORD dwPID;

    dwPID = 0;
    bfts = PbftsBFETLStatsByPID( NULL, &dwPID );
    TestCheck( bfts == NULL );
    TestCheck( dwPID == 0 );

HandleError:

    BFETLTerm( bffetlc );
    CleanupTestData_();

    return err;
}

ERR ETLDriverTest::ErrStatsByPidNoStatsTest_()
{
    ERR err = JET_errSuccess;
    BFETLContext* bffetlc = NULL;
    BFTRACE bftrace;

    SetupTestData_();
    TestCheckErr( ErrBFETLInit( m_rgpEtwEvt, 333, s_cetwevt / 2, fBFETLDriverTestMode, &bffetlc ) );

    while ( ( err = ErrBFETLGetNext( bffetlc, &bftrace ) ) == JET_errSuccess );

    TestAssert( err == errNotFound );
    err = JET_errSuccess;

    const BFTraceStats* bfts = NULL;
    DWORD dwPID;

    dwPID = 0;
    bfts = PbftsBFETLStatsByPID( bffetlc, &dwPID );
    TestCheck( bfts == NULL );
    TestCheck( dwPID == 0 );

HandleError:

    BFETLTerm( bffetlc );
    CleanupTestData_();

    return err;
}

