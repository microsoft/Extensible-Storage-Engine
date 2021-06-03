// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "syncunittest.hxx"
#include "sync.hxx"

#ifndef OnRetail
#ifdef DEBUG
#define OnRetail( xxx )
#else
#define OnRetail( xxx )     xxx
#endif
#endif

//  Used to force an async partition, so the main thread doesn't hang.  Doesn't do anything.

void PartitionAsyncThunk( const DWORD_PTR dw )
{
}

//  Validated PartitionDone check

CMeteredSection::Group  g_groupPartitionExpected    = CMeteredSection::groupInvalidNil;
BOOL            g_fPartitionDone        = fTrue;

void PartitionDoneGroupCheck( const DWORD_PTR ptr )
{
    ERR err;
    CMeteredSection *       pmsTest = (CMeteredSection*)ptr;
    wprintf( L"\n\t\tPartitioningComplete!\n\n" );

    TestCheck( g_fPartitionDone == fFalse );    // should reset for each partition
    TestCheck( g_groupPartitionExpected == pmsTest->GroupActive() );
    TestCheck( 1 - pmsTest->GroupActive() == pmsTest->GroupInactive() );

    g_fPartitionDone = fTrue;
    return;

HandleError:

    //  should be false, but just in case, use this to err out the main thread...
    g_fPartitionDone = fFalse;
}

//  Test fixture(s).

//  --------------------------------------------------------------------------------------------------

class CMeteredSectionConcurrentBashTest : public UNITTEST
{
    private:
        static CMeteredSectionConcurrentBashTest s_instance;

    protected:
        CMeteredSectionConcurrentBashTest()
        {
        }

    public:
        ~CMeteredSectionConcurrentBashTest()
        {
        }

    public:
        const char * SzName() const     { return "CMeteredSectionConcurrentBashTest"; }
        const char * SzDescription() const  { return "Checks that everything stays consistent with concurrent Enter() / Leaves()"; }
        bool FRunUnderESE98() const     { return true; }
        bool FRunUnderESENT() const     { return true; }
        bool FRunUnderESE97() const     { return true; }

        ERR ErrTest();

};

CMeteredSectionConcurrentBashTest CMeteredSectionConcurrentBashTest::s_instance;

volatile INT g_cbashphase   = 0;
INT g_tickPhaseWait     = 10;   //  could use 0 for fast fast response.
INT g_cbasephaseDone    = 0x7fffffff;

typedef struct
{
    CMeteredSection *   pmsTest;

    HANDLE          hThread;
    DWORD           tid;

    INT         cbashphase;

    // these must be before rgcGroup
    QWORD           cUnderflowNegThree; //  one to grow on
    QWORD           cUnderflowNegTwo;
    QWORD           cUnderflowNegOne;
    QWORD           rgcGroup[2];

    QWORD           cSuccessfulEnters;
    QWORD           cFailedEnters;
    QWORD           cSuccessfulLeaves;
    QWORD           cSkippedLeaves;

} THREAD_BLOCK_MSCB;    // MeteredSectionConcurrentBash

THREAD_BLOCK_MSCB   g_rgtbMscb[3];

#define IthreadMscb( ptb )      ( ptbMscb - g_rgtbMscb )

//  Kinds of threads ... 

#define FEvenThread( ptb )      ( 0 == IthreadMscb( ptb ) )

DWORD WINAPI ConcurrentBashWorker( THREAD_BLOCK_MSCB * const ptbMscb )
{
    ERR err = JET_errSuccess;

    CMeteredSection * pmsTest = ptbMscb->pmsTest;   // short hand
    DWORD tickStart = GetTickCount();
    wprintf( L"\tConcurrentBashWorker thread [%d]%p started (@%d).\n", IthreadMscb( ptbMscb ), ptbMscb, tickStart );

    ptbMscb->cbashphase = 0;
    while( g_cbashphase == 0 )
    {
        // master thread says wait to coordinate beginning ...
    }

    BstfSetVerbosity( bvlPrintTests - 1 );

    ptbMscb->cbashphase = 1;
    while( g_cbashphase == 1 )
    {
        if ( ( rand() % ( FEvenThread( ptbMscb ) ? 2 : 4 ) ) != 0 )
        {
            CMeteredSection::Group groupEnter = pmsTest->GroupEnter();

            TestCheck( groupEnter >= -3 );  //  or bad things will happen
            ptbMscb->rgcGroup[groupEnter]++;

            if( groupEnter == CMeteredSection::groupTooManyActiveErr )
            {
                ptbMscb->cFailedEnters++;
                TestCheck( ptbMscb->cUnderflowNegTwo == ptbMscb->cFailedEnters );
            }
            else
            {
                ptbMscb->cSuccessfulEnters++;
            }
        }
        else
        {
            CMeteredSection::Group groupLeave = rand() % 2;
            if ( ptbMscb->rgcGroup[groupLeave] )
            {
                pmsTest->Leave( groupLeave );
                ptbMscb->rgcGroup[groupLeave]--;
                ptbMscb->cSuccessfulLeaves++;
            }
            else if ( ptbMscb->rgcGroup[!groupLeave] )
            {
                pmsTest->Leave( !groupLeave );
                ptbMscb->rgcGroup[!groupLeave]--;
                ptbMscb->cSuccessfulLeaves++;
            }
            else
            {
                ptbMscb->cSkippedLeaves++;
            }
        }
    }

    
    ptbMscb->cbashphase++;
    while( g_cbashphase == ptbMscb->cbashphase )
    {
        if ( ptbMscb->rgcGroup[0] == 0 && ptbMscb->rgcGroup[1] == 0 )
        {
            //  we've completed the phase, wait for main thread to break us out
            Sleep( g_tickPhaseWait );
            continue;
        }

        if ( ( rand() % ( FEvenThread( ptbMscb ) ? 2 : 4 ) ) == 0 )
        {
            CMeteredSection::Group groupEnter = pmsTest->GroupEnter();

            TestCheck( groupEnter >= -3 );  //  or bad things will happen
            ptbMscb->rgcGroup[groupEnter]++;

            if( groupEnter == CMeteredSection::groupTooManyActiveErr )
            {
                ptbMscb->cFailedEnters++;
                TestCheck( ptbMscb->cUnderflowNegTwo == ptbMscb->cFailedEnters );
            }
            else
            {
                ptbMscb->cSuccessfulEnters++;
            }
        }
        else
        {
            CMeteredSection::Group groupLeave = rand() % 2;
            if ( ptbMscb->rgcGroup[groupLeave] )
            {
                pmsTest->Leave( groupLeave );
                ptbMscb->rgcGroup[groupLeave]--;
                ptbMscb->cSuccessfulLeaves++;
            }
            else if ( ptbMscb->rgcGroup[!groupLeave] )
            {
                pmsTest->Leave( !groupLeave );
                ptbMscb->rgcGroup[!groupLeave]--;
                ptbMscb->cSuccessfulLeaves++;
            }
            else
            {
                ptbMscb->cSkippedLeaves++;
            }
        }
    }


    wprintf( L"\tConcurrentBashWorker thread [%d]%p finished (%d ms).\n", IthreadMscb( ptbMscb ), ptbMscb, GetTickCount() - tickStart );
    ptbMscb->cbashphase = g_cbasephaseDone;

HandleError:

    BstfSetVerbosity( bvlPrintTests );

    return err;
}

DWORD MsRand()
{
    const INT i = rand() % 5;
    switch( i )
    {
        case 0:     return 0;
        case 1:     return 1;
        case 2:     return 2;
        case 3:     return 10;
    }
    return rand() % 50;
}

void PartitionConcurrentBash( const DWORD_PTR ptr )
{
}

ERR CMeteredSectionConcurrentBashTest::ErrTest()
{
    ERR err = JET_errSuccess;
    const BOOL fOsSyncInit = FOSSyncPreinit();
    TestAssert( fOsSyncInit );

    CMeteredSection     msTest;

    TestCheck( msTest.FEmpty() );
    OnRetail( TestCheck( !msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 0 ) );

    g_cbashphase = 0;   //  use one if you want threads to start immediate / not coordinate

    DWORD tickStart = GetTickCount();
    memset( g_rgtbMscb, 0, sizeof(g_rgtbMscb) );    // should be all zeros anyway, but we depend upon it, so do it ...
    for( ULONG ithread = 0; ithread < _countof(g_rgtbMscb); ithread++ )
    {
        g_rgtbMscb[ithread].pmsTest = &msTest;
        g_rgtbMscb[ithread].hThread = HANDLE( CreateThread( NULL,
                                                0,
                                                LPTHREAD_START_ROUTINE( ConcurrentBashWorker ),
                                                (void*) &g_rgtbMscb[ithread],
                                                0,
                                                &g_rgtbMscb[ithread].tid ) );
        if ( !g_rgtbMscb[ithread].hThread )
        {
            TestCheckErr( JET_errOutOfMemory );
        }
    }

    Sleep( 10 );    // ensure threads are started ...
    wprintf( L"Threads started (%d ms)\n", GetTickCount() - tickStart );

    tickStart = GetTickCount();
    g_cbashphase = 1;

    C_ASSERT( _countof(g_rgtbMscb) == 3 );  //  too lazy
    while( g_rgtbMscb[0].cUnderflowNegTwo == 0 || g_rgtbMscb[0].cUnderflowNegTwo == 0 || g_rgtbMscb[0].cUnderflowNegTwo == 0 )
    {
        Sleep( g_tickPhaseWait );
    }
    wprintf( L"Finished phase 1 (%d ms)\n", GetTickCount() - tickStart );

    msTest.Partition( PartitionAsyncThunk, NULL );

    tickStart = GetTickCount();
    g_cbashphase = 2;

    C_ASSERT( _countof(g_rgtbMscb) == 3 );  //  still too lazy
    while( g_rgtbMscb[0].rgcGroup[0] != 0 || g_rgtbMscb[0].rgcGroup[1] != 0 || 
        g_rgtbMscb[1].rgcGroup[0] != 0 || g_rgtbMscb[1].rgcGroup[1] != 0 || 
        g_rgtbMscb[2].rgcGroup[0] != 0 || g_rgtbMscb[2].rgcGroup[1] != 0 )
    {
        Sleep( g_tickPhaseWait );
    }
    wprintf( L"Finished phase 2 (%d ms)\n", GetTickCount() - tickStart );

    g_cbashphase = g_cbasephaseDone;

    ULONG ithread = 0;
    while( g_rgtbMscb[ithread].cbashphase != g_cbasephaseDone || ithread++ < _countof(g_rgtbMscb) )
    {
        wprintf( L"\t\tGot quit of [%d] .. (%#x)\n", ithread, g_rgtbMscb[ithread].cbashphase );
        if ( ithread >= _countof(g_rgtbMscb) )
        {
            break;
        }
        Sleep( g_tickPhaseWait );
    }

    for( ULONG ithread = 0; ithread < _countof(g_rgtbMscb); ithread++ )
    {
        wprintf( L"\tg_rgtbMscb[%d]\n", ithread );

        TestCheck( g_rgtbMscb[ithread].cUnderflowNegThree == 0 );
        TestCheck( g_rgtbMscb[ithread].cUnderflowNegOne == 0 );

        wprintf( L"\t\t cUnderflowNegThree           = %d\n", g_rgtbMscb[ithread].cUnderflowNegThree );
        wprintf( L"\t\t rgcGroup[0]                  = %d\n", g_rgtbMscb[ithread].rgcGroup[0] );
        wprintf( L"\t\t rgcGroup[1]                  = %d\n", g_rgtbMscb[ithread].rgcGroup[1] );

        wprintf( L"\t\t cSuccessfulEnters            = %d\n", g_rgtbMscb[ithread].cSuccessfulEnters );
        wprintf( L"\t\t cFailedEnters                = %d\n", g_rgtbMscb[ithread].cFailedEnters );
        wprintf( L"\t\t cSuccessfulLeaves            = %d\n", g_rgtbMscb[ithread].cSuccessfulLeaves );
        wprintf( L"\t\t cSkippedLeaves               = %d\n", g_rgtbMscb[ithread].cSkippedLeaves );
    }


HandleError:

    OSSyncPostterm();

    return err;
}


//  --------------------------------------------------------------------------------------------------

class CMeteredSectionLimitTest : public UNITTEST
{
    private:
        static CMeteredSectionLimitTest s_instance;

    protected:
        CMeteredSectionLimitTest()
        {
        }

    public:
        ~CMeteredSectionLimitTest()
        {
        }

    public:
        const char * SzName() const     { return "CMeteredSectionLimitTest"; }
        const char * SzDescription() const  { return "Validates that we fail to enter when we reach a limit of active users."; }
        bool FRunUnderESE98() const     { return true; }
        bool FRunUnderESENT() const     { return true; }
        bool FRunUnderESE97() const     { return true; }

        ERR ErrTest();

};

CMeteredSectionLimitTest CMeteredSectionLimitTest::s_instance;

ERR CMeteredSectionLimitTest::ErrTest()
{
    ERR err = JET_errSuccess;
    const BOOL fOsSyncInit = FOSSyncPreinit();
    TestAssert( fOsSyncInit );

    CMeteredSection     msTest;

    TestCheck( msTest.FEmpty() );
    OnRetail( TestCheck( !msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 0 ) );

    g_groupPartitionExpected = -3;
    g_fPartitionDone = fTrue;

    ULONG iEnters = 0;

    CMeteredSection::Group groupFirst = msTest.GroupEnter();
    iEnters++;
    wprintf( L"\t\tgroupFirst = %d\n", groupFirst );

    TestCheck( groupFirst != CMeteredSection::groupTooManyActiveErr );
    OnRetail( TestCheck( msTest.CActiveUsers() == 1 ) );

    CMeteredSection::Group groupLast;
    while( groupFirst == ( groupLast = msTest.GroupEnter() ) )
    {
        iEnters++;
    }
    TestCheck( groupLast == CMeteredSection::groupTooManyActiveErr );
    TestCheck( iEnters == 0x7ffe );

    TestCheck( !msTest.FEmpty() );
    OnRetail( TestCheck( !msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0x7ffe ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 0 ) );

    OnRetail( TestCheck( CMeteredSection::groupTooManyActiveErr == msTest.GroupEnter() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0x7ffe ) );

    msTest.Partition( PartitionAsyncThunk, NULL );
    
    TestCheck( !msTest.FEmpty() );
    TestCheck( msTest.GroupActive() == 1 );
    TestCheck( msTest.GroupInactive() == 0 );
    OnRetail( TestCheck( msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 0x7ffe ) );

    CMeteredSection::Group groupSecond = msTest.GroupEnter();
    iEnters++;
    wprintf( L"\t\tgroupSecond = %d\n", groupSecond );

    TestCheck( groupSecond != CMeteredSection::groupTooManyActiveErr );
    TestCheck( groupFirst != groupSecond );

    while( groupSecond == ( groupLast = msTest.GroupEnter() ) )
    {
        iEnters++;
    }
    TestCheck( groupLast == CMeteredSection::groupTooManyActiveErr );
    TestCheck( iEnters == 2 * 0x7ffe );

    TestCheck( !msTest.FEmpty() );
    OnRetail( TestCheck( msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0x7ffe ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 0x7ffe ) );

    TestCheck( groupLast == CMeteredSection::groupTooManyActiveErr );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0x7ffe ) );

    BstfSetVerbosity( bvlPrintTests - 1 );
    while( iEnters > 2 )
    {
        msTest.Leave( iEnters % 2 );

        if ( iEnters > ( 2 * 0x7fff ) - 10 )
        {
            OnRetail( wprintf( L"\t\t\t\tLeaveResults[%#x] ... %#x / %#x ...\n", iEnters, msTest.CActiveUsers(), msTest.CQuiescingUsers() ) );
            //OnRetail( wprintf( L"\t\t\t\tLeaveResults[%#x] ... %#x / %#x ... %#x, %d, %d\n", iEnters, msTest.CActiveUsers(), msTest.CQuiescingUsers(),
            //      iEnters / 2 , iEnters % 2, iEnters % 2 == 0 ) );
        }

        OnRetail( TestCheck( (ULONG)msTest.CActiveUsers() == iEnters / 2 ) );
        OnRetail( TestCheck( (ULONG)msTest.CQuiescingUsers() == iEnters / 2 - ( iEnters % 2 == 0 ) ) );
        iEnters--;
    }
    BstfSetVerbosity( bvlPrintTests );

    TestCheck( g_fPartitionDone == fTrue );

    TestCheck( !msTest.FEmpty() );
    TestCheck( msTest.GroupActive() == 1 );
    TestCheck( msTest.GroupInactive() == 0 );
    OnRetail( TestCheck( msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0x1 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 0x1 ) );

    g_fPartitionDone = fFalse;
    g_groupPartitionExpected = 1;

    msTest.Leave( groupFirst ); //  partition complete

    TestCheck( !msTest.FEmpty() );
    TestCheck( msTest.GroupActive() == 1 );
    TestCheck( msTest.GroupInactive() == 0 );
    OnRetail( TestCheck( !msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0x1 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 0x0 ) );

    msTest.Leave( groupSecond );    //  active group empty

    TestCheck( msTest.FEmpty() );
    TestCheck( msTest.GroupActive() == 1 );
    TestCheck( msTest.GroupInactive() == 0 );
    OnRetail( TestCheck( !msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0x0 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 0x0 ) );

HandleError:

    OSSyncPostterm();

    return err;
}


//  --------------------------------------------------------------------------------------------------

class CMeteredSectionCaptureNonEmptyDtorAssert : public UNITTEST
{
    private:
        static CMeteredSectionCaptureNonEmptyDtorAssert s_instance;

    protected:
        CMeteredSectionCaptureNonEmptyDtorAssert()
        {
        }

    public:
        ~CMeteredSectionCaptureNonEmptyDtorAssert()
        {
        }

    public:
        const char * SzName() const     { return "CMeteredSectionCaptureNonEmptyDtorAssert"; }
        const char * SzDescription() const  { return "Checks that if someone deletes a ms without everyone leaving that we catch it / assert."; }
        bool FRunUnderESE98() const     { return true; }
        bool FRunUnderESENT() const     { return true; }
        bool FRunUnderESE97() const     { return true; }

        ERR ErrTest();

};

CMeteredSectionCaptureNonEmptyDtorAssert CMeteredSectionCaptureNonEmptyDtorAssert::s_instance;

#ifdef OnDebug
#error "Remove this defn"
#else
#ifdef DEBUG
#define OnDebug( xxx )      xxx
#else
#define OnDebug( xxx )      
#endif
#endif

ERR CMeteredSectionCaptureNonEmptyDtorAssert::ErrTest()
{
    ERR err = JET_errSuccess;
    const BOOL fOsSyncInit = FOSSyncPreinit();
    TestAssert( fOsSyncInit );

    CMeteredSection * pmsTest = NULL;

    // New metered section, and enter it ...

    pmsTest = new CMeteredSection;
    TestCheck( NULL != pmsTest );
    TestCheck( 0 == pmsTest->GroupEnter() );
    OnRetail( TestCheck( pmsTest->CActiveUsers() == 1 ) );
    OnRetail( TestCheck( !pmsTest->FQuiescing() ) );
    TestCheck( pmsTest->GroupActive() == 0 );
    TestCheck( pmsTest->GroupInactive() == 1 );

    // Check to see we assert on this bad situation
    
    g_fCaptureAssert = fTrue;
    delete pmsTest;
    //  Can only check this on debug, b/c we don't have asserts in retail ...
    OnDebug( TestCheck( 0 == strcmp( g_szCapturedAssert, "FEmpty() || g_fSyncProcessAbort" ) ) );
    g_fCaptureAssert = fFalse;
    g_szCapturedAssert = NULL;

    // Next try the same steps, but partition it, so the non-empty count is in the quiescing state ...

    pmsTest = new CMeteredSection;
    TestCheck( NULL != pmsTest );
    TestCheck( 0 == pmsTest->GroupEnter() );
    pmsTest->Partition( PartitionAsyncThunk, NULL );
    OnRetail( TestCheck( pmsTest->FQuiescing() ) );
    OnRetail( TestCheck( pmsTest->CQuiescingUsers() == 1 ) );
    OnRetail( TestCheck( pmsTest->CActiveUsers() == 0 ) );
    TestCheck( pmsTest->GroupActive() == 1 );
    TestCheck( pmsTest->GroupInactive() == 0 );
    g_fCaptureAssert = fTrue;
    delete pmsTest;
    OnDebug( TestCheck( 0 == strcmp( g_szCapturedAssert, "FEmpty() || g_fSyncProcessAbort" ) ) );
    g_fCaptureAssert = fFalse;
    g_szCapturedAssert = NULL;

HandleError:

    OSSyncPostterm();

    return err;
}


//  --------------------------------------------------------------------------------------------------

class CMeteredSectionBasicTest : public UNITTEST
{
    private:
        static CMeteredSectionBasicTest s_instance;

    protected:
        CMeteredSectionBasicTest()
        {
        }

    public:
        ~CMeteredSectionBasicTest()
        {
        }

    public:
        const char * SzName() const     { return "CMeteredSectionBasicTest"; }
        const char * SzDescription() const  { return "Validates basic functionality of the CSemaphore synchronization object."; }
        bool FRunUnderESE98() const     { return true; }
        bool FRunUnderESENT() const     { return true; }
        bool FRunUnderESE97() const     { return true; }

        ERR ErrTest();

};

CMeteredSectionBasicTest CMeteredSectionBasicTest::s_instance;

ERR CMeteredSectionBasicTest::ErrTest()
{
    ERR err = JET_errSuccess;
    const BOOL fOsSyncInit = FOSSyncPreinit();
    TestAssert( fOsSyncInit );

    wprintf( L"\tTesting basic CMeteredSection routines...\n" );

    //  -------------------------------------------------------------------------------------------
    //  Example code used to generate "Sample Code and State" documentation in sync.hxx
    // originally had a whole bunch of these, littered through it, but had to make all member vars
    // public to see them, so removed.
    //wprintf( L"Data[%d] msTest - %p ... 0x%x { %d, %d, %d, %d } ... %d / %d\n", __LINE__, &msTest, pmsTest->m_cw, pmsTest->m_cCurrent, pmsTest->m_groupCurrent, pmsTest->m_cQuiesced, pmsTest->m_groupQuiesced,pmsTest->ActiveGroup(), pmsTest->FQuiescing() );


    CMeteredSection     msTest;

    TestCheck( msTest.FEmpty() );
    TestCheck( msTest.GroupActive() == 0 );
    TestCheck( msTest.GroupInactive() == 1 );
    OnRetail( TestCheck( !msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 0 ) );

    const INT i = msTest.Enter();
    TestCheck( i == 0 );    //  first group after .ctor is 0, not that this is important ...
    TestCheck( i == msTest.Enter() );
    TestCheck( i == msTest.Enter() );

    TestCheck( !msTest.FEmpty() );
    TestCheck( msTest.GroupActive() == 0 );
    TestCheck( msTest.GroupInactive() == 1 );
    OnRetail( TestCheck( !msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 3 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 0 ) );

    //  by setting -3, it asserts we don't expect an immediate partition ...
    g_groupPartitionExpected = -3;
    msTest.Partition( PartitionDoneGroupCheck, (DWORD_PTR)&msTest );    //      - Begin

    TestCheck( !msTest.FEmpty() );
    TestCheck( msTest.GroupActive() == 1 );
    TestCheck( msTest.GroupInactive() == 0 );
    OnRetail( TestCheck( msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 3 ) );

    msTest.Leave( i );

    TestCheck( !msTest.FEmpty() );
    TestCheck( msTest.GroupActive() == 1 );
    TestCheck( msTest.GroupInactive() == 0 );
    OnRetail( TestCheck( msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 2 ) );

    const INT i2 = msTest.Enter();
    TestCheck( i2 == msTest.Enter() );
    TestCheck( i2 == msTest.Enter() );
    TestCheck( i2 == msTest.Enter() );
    TestCheck( i2 == msTest.Enter() );
    TestCheck( i2 == msTest.Enter() );

    TestCheck( !msTest.FEmpty() );
    TestCheck( msTest.GroupActive() == 1 );
    TestCheck( msTest.GroupInactive() == 0 );
    OnRetail( TestCheck( msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 6 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 2 ) );

    msTest.Leave( i );
    g_fPartitionDone = fFalse;
    g_groupPartitionExpected = 1;
    msTest.Leave( i );
    //  -> immediately triggers PartitionDoneGroupCheck / .Partition()  - End
    TestCheck( g_fPartitionDone == fTrue );

    TestCheck( !msTest.FEmpty() );
    TestCheck( msTest.GroupActive() == 1 );
    TestCheck( msTest.GroupInactive() == 0 );
    OnRetail( TestCheck( !msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 6 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 0 ) );

    msTest.Leave( i2 );

    TestCheck( !msTest.FEmpty() );
    TestCheck( msTest.GroupActive() == 1 );
    TestCheck( msTest.GroupInactive() == 0 );
    OnRetail( TestCheck( !msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 5 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 0 ) );

    msTest.Partition( PartitionDoneGroupCheck, (DWORD_PTR)&msTest );    //      - Begin2

    TestCheck( !msTest.FEmpty() );
    TestCheck( msTest.GroupActive() == 0 );
    TestCheck( msTest.GroupInactive() == 1 );
    OnRetail( TestCheck( msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 5 ) );

    msTest.Leave( i2 );
    msTest.Leave( i2 );
    msTest.Leave( i2 );
    msTest.Leave( i2 );

    g_fPartitionDone = fFalse;
    g_groupPartitionExpected = 0;   // goes back to group zero
    msTest.Leave( i2 );
    //  -> immediately triggers PartitionDoneGroupCheck / .Partition()  - End2

    TestCheck( msTest.FEmpty() );
    TestCheck( msTest.GroupActive() == 0 );
    TestCheck( msTest.GroupInactive() == 1 );
    OnRetail( TestCheck( !msTest.FQuiescing() ) );
    OnRetail( TestCheck( msTest.CActiveUsers() == 0 ) );
    OnRetail( TestCheck( msTest.CQuiescingUsers() == 0 ) );

    //  -------------------------------------------------------------------------------------------
    wprintf( L"\tCMeteredSectionBasicTest done!" );

HandleError:

    OSSyncPostterm();

    return err;
}



