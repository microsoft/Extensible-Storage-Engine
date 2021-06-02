// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

//  Unfortunately, the OS Layer doesn't have access to the ESE internal unit test framework.
//      further it is inappropriate to expose this through the OS layer headers.  So we test
//      it here.
ULONG IOSDiskIUrgentLevelFromQOS( _In_ const OSFILEQOS grbitQOS );
LONG CioOSDiskIFromUrgentQOS( _In_ const OSFILEQOS grbitQOS, _In_ const DWORD cioUrgentMaxMax );

JETUNITTEST( OSDISK, QosToUrgentLevelConversions )
{

    //  Check the static min/base and max levels ...

    CHECK( qosIODispatchUrgentBackgroundMin == QosOSFileFromUrgentLevel( 1 ) );
    CHECK( qosIODispatchUrgentBackgroundMax == QosOSFileFromUrgentLevel( 127 ) );

    //  Check that the conversion routines convert all iUrgentLevels within the appropriate 
    //  range properly.

    for( ULONG iUrgentLevel = 1; iUrgentLevel <= qosIODispatchUrgentBackgroundLevelMax; iUrgentLevel++ )
    {

        //  Check that the conversion routines can convert all iUrgentLevels to QOS and back again.

        CHECK( iUrgentLevel == IOSDiskIUrgentLevelFromQOS( QosOSFileFromUrgentLevel( iUrgentLevel ) ) );
    }

    for( ULONG cioMaxMax = 4 /* min / 2 */; cioMaxMax <= 1024 /* max * 2 */; cioMaxMax++ )
    {
        //  Checks the minimal urgent level gives back 1 or greater, ensuring we make progress

        CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 1 ), cioMaxMax ) >= 1 );

        //  Checks the maximum urgent level gives back the cioMaxMax provided

        CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( qosIODispatchUrgentBackgroundLevelMax ), cioMaxMax ) == (LONG)cioMaxMax );

        //  Check there is a material difference between 
        //      the low level and the 1/4 way mark
        //      the 1/4 way mark and the 1/2 way mark
        //      the 1/2 way mark and the 3/4 way mark
        //      the 3/4 mark and the end

        CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 1 ), cioMaxMax ) <
                    CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 / 4  ), cioMaxMax ) );
        CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 / 4 ), cioMaxMax ) <
                    CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 / 2  ), cioMaxMax ) );
        CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 / 2 ), cioMaxMax ) <
                    CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 * 3 / 4  ), cioMaxMax ) );
        if ( cioMaxMax >= 5 )
        {
            //  Only holds at 5 or higher ...
            CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 * 3 / 4 ), cioMaxMax ) <
                        CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 ), cioMaxMax ) );
        }

    }

    //  Do a little more validation for certain key cioMaxMax values ...

    //      Iterate through interesting values of "background max max" ... even outside supported values.

    ULONG rgCioMaxMax [] = { 4, 8 /* min */, 9, 10, 16, 32, 64, 96, 127, 128, 129, 256, 386, 511, 512 /* max */, 513, 1024 };   //  must be ascending
    for( ULONG i = 0; i < _countof( rgCioMaxMax ); i++ )
    {
        for( ULONG iUrgentLevel = 1; iUrgentLevel <= qosIODispatchUrgentBackgroundLevelMax; iUrgentLevel++ )
        {

            if ( i > 1 )
            {
                //  Checks that increasing UrgentBackground IO quota increases (or maintains) the IO 
                //  aggressiveness for all urgent levels.

                CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( iUrgentLevel ), rgCioMaxMax[i-1] ) <=
                            CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( iUrgentLevel ), rgCioMaxMax[i] ) );
            }

            if ( iUrgentLevel > 1 )
            {
                //  Checks each urgent level gets progressively more (or stays equally) aggressive ...

                CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( iUrgentLevel - 1 ), rgCioMaxMax[i] ) <=
                            CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( iUrgentLevel ), rgCioMaxMax[i] ) );
            }
        }
    }

    ///* Printing out the IO outstanding vs. urgent level table ...
    for( ULONG iUrgentLevel = 1; iUrgentLevel <= qosIODispatchUrgentBackgroundLevelMax; iUrgentLevel++ )
    {
        //  Printing for some tables I wanted ...

        if ( iUrgentLevel == 1 )
        {
            wprintf( L"\n  This is table for X max outstanding background IOs ..."  );
            wprintf( L"\n  Note: That max background IO here, is half the actual JET_paramOutstandingIOMax setting (* = default param=%Id) value.", UlParam( JET_paramOutstandingIOMax ) );

            wprintf( L"\nMax Background: " );
            for( ULONG i = 0; i < _countof( rgCioMaxMax ); i++ )
            {
                if ( rgCioMaxMax[i] == 4 ||
                    rgCioMaxMax[i] == 8 ||  // 16 outstanding param, 8 background is default of small config.
                    rgCioMaxMax[i] == 16 ||
                    rgCioMaxMax[i] == 32 ||
                    rgCioMaxMax[i] == 64 ||
                    rgCioMaxMax[i] == 96 ||
                    rgCioMaxMax[i] == 128 ||
                    rgCioMaxMax[i] == 256 ||
                    rgCioMaxMax[i] == 512 || // 1024 outstanding, 512 background is default of legacy config (at the moment)
                    rgCioMaxMax[i] == 1024 )
                {
                    if ( rgCioMaxMax[i] == UlParam( JET_paramOutstandingIOMax ) / 2 )
                    {
                        wprintf( L"*%3d, ", rgCioMaxMax[i] );
                    }
                    else
                    {
                        wprintf( L"%4d, ", rgCioMaxMax[i] );
                    }
                }
            }
            wprintf( L"\n                 ----------------  outstanding IO  ------------------" );
        }
        
        wprintf( L"\nLevel-Cio, %3d, ", iUrgentLevel );
        for( ULONG i = 0; i < _countof( rgCioMaxMax ); i++ )
        {
            if ( rgCioMaxMax[i] == 4 ||
                rgCioMaxMax[i] == 8 ||  // 16 outstanding
                rgCioMaxMax[i] == 16 ||
                rgCioMaxMax[i] == 32 ||
                rgCioMaxMax[i] == 64 ||
                rgCioMaxMax[i] == 96 ||
                rgCioMaxMax[i] == 128 ||
                rgCioMaxMax[i] == 256 ||
                rgCioMaxMax[i] == 512 ||
                rgCioMaxMax[i] == 1024 )
            {
                wprintf( L"%4d, ", CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( iUrgentLevel ), rgCioMaxMax[i] ) );
            }
        }
    }
    //*/

}

//  Unfortunately, the OS Layer doesn't have access to the ESE internal unit test framework.
//      further it is inappropriate to expose this through the OS layer headers.  So we test
//      it here.

class PatrolDogSynchronizer
{
    private:
        CAutoResetSignal    m_asigNudgePatrolDog;
        BOOL volatile       m_fPutDownPatrolDog;
        DWORD volatile      m_cLoiters;
        THREAD              m_threadPatrolDog;
        LONG volatile       m_patrolDogState;
        void*               m_pvContext;

    public:
        PatrolDogSynchronizer();
        ~PatrolDogSynchronizer();
        ERR ErrInitPatrolDog( const PUTIL_THREAD_PROC pfnPatrolDog, const EThreadPriority priority, void* const pvContext );
        void TermPatrolDog();
        void EnterPerimeter();
        void LeavePerimeter();
        BOOL FCheckForLoiter( const TICK dtickTimeout );
        void* PvContext() const;
        SIZE_T CbSizeOf() const;
};

JETUNITTEST( PatrolDogSynchronizer, SizeOfEnforcement )
{
    PatrolDogSynchronizer pds;
    
    CHECK( sizeof( pds ) == pds.CbSizeOf() );
}

JETUNITTEST( PatrolDogSynchronizer, InvalidPfn )
{
    PatrolDogSynchronizer pds;

    CHECK( pds.ErrInitPatrolDog( NULL, priorityNormal, NULL ) == JET_errInvalidParameter );
}

LOCAL const DWORD g_dwContextTest = 0x12345678;

DWORD RetrievingPvContextThread( DWORD_PTR dwContext )
{
    const PatrolDogSynchronizer* const ppds = (PatrolDogSynchronizer*)dwContext;

    Enforce( *(DWORD*)( ppds->PvContext() ) == g_dwContextTest );
    
    return 0;
}

JETUNITTEST( PatrolDogSynchronizer, RetrievingPvContext )
{
    PatrolDogSynchronizer pds;

    CHECK( pds.ErrInitPatrolDog( RetrievingPvContextThread, priorityNormal, (void*)&g_dwContextTest ) == JET_errSuccess );
    pds.TermPatrolDog();
}

DWORD RetrievingDwContextThread( DWORD_PTR dwContext )
{
    const PatrolDogSynchronizer* const ppds = (PatrolDogSynchronizer*)dwContext;

    Enforce( (DWORD)(BOOL)( ppds->PvContext() == (void*)dwContext ) );
    
    return 0;
}

JETUNITTEST( PatrolDogSynchronizer, RetrievingDwContext )
{
    PatrolDogSynchronizer pds;

    CHECK( pds.ErrInitPatrolDog( RetrievingDwContextThread, priorityNormal, (void*)&pds ) == JET_errSuccess );
    pds.TermPatrolDog();
}

DWORD DummyThread( DWORD_PTR dwContext )
{
    return 0;
}

JETUNITTEST( PatrolDogSynchronizer, InitTwice )
{
    PatrolDogSynchronizer pds;

    CHECK( pds.ErrInitPatrolDog( DummyThread, priorityNormal, NULL ) == JET_errSuccess );
    CHECK( pds.ErrInitPatrolDog( DummyThread, priorityNormal, NULL ) == JET_errInvalidOperation );
    pds.TermPatrolDog();
    CHECK( pds.ErrInitPatrolDog( DummyThread, priorityNormal, NULL ) == JET_errSuccess );
    pds.TermPatrolDog();
}

JETUNITTEST( PatrolDogSynchronizer, TermTwice )
{
    PatrolDogSynchronizer pds;

    CHECK( pds.ErrInitPatrolDog( DummyThread, priorityNormal, NULL ) == JET_errSuccess );
    pds.TermPatrolDog();
    pds.TermPatrolDog();
}

LOCAL volatile DWORD g_cLoop = 0;

DWORD BasicEnterLeaveThread( DWORD_PTR dwContext )
{
    PatrolDogSynchronizer* const ppds = (PatrolDogSynchronizer*)dwContext;
    const DWORD dwTimeout = *(DWORD*)ppds->PvContext();

    while ( ppds->FCheckForLoiter( dwTimeout ) )
    {
        if ( ++g_cLoop == 0 )
        {
            g_cLoop++;
        }
    }
    
    return 0;
}

#pragma warning( push )
#pragma warning( disable: 4101 4189 ) // We don't care about unreferenced variables in Unit Tests.

JETUNITTEST( PatrolDogSynchronizer, NoEnter )
{
    PatrolDogSynchronizer pds;
    const DWORD dwTimeout = INT_MAX;

    g_cLoop = 0;
    CHECK( pds.ErrInitPatrolDog( BasicEnterLeaveThread, priorityNormal, (void*)&dwTimeout ) == JET_errSuccess );
    UtilSleep( 500 );
    pds.TermPatrolDog();
    CHECK( g_cLoop == 0 );
}

JETUNITTEST( PatrolDogSynchronizer, NoLeave )
{
    //  Asserts with a valid/good assert in DEBUG, but we still want to test the RETAIL behavior.
    
#ifndef DEBUG
    PatrolDogSynchronizer pds;
    const DWORD dwTimeout = INT_MAX;

    g_cLoop = 0;
    CHECK( pds.ErrInitPatrolDog( BasicEnterLeaveThread, priorityNormal, (void*)&dwTimeout ) == JET_errSuccess );
    UtilSleep( 500 );
    CHECK( g_cLoop == 0 );
    pds.EnterPerimeter();
    UtilSleep( 500 );
    pds.TermPatrolDog();
    CHECK( g_cLoop == 0 );
#endif  //  DEBUG
}

JETUNITTEST( PatrolDogSynchronizer, ProperTiming )
{
    PatrolDogSynchronizer pds;
    const DWORD dwTimeout = 100;

    g_cLoop = 0;
    CHECK( pds.ErrInitPatrolDog( BasicEnterLeaveThread, priorityNormal, (void*)&dwTimeout ) == JET_errSuccess );
    UtilSleep( 500 );
    CHECK( g_cLoop == 0 );
    
    pds.EnterPerimeter();
    UtilSleep( 1000 );
    pds.LeavePerimeter();
    
    DWORD cLoop1 = g_cLoop;
    CHECK( cLoop1 > 0 && cLoop1 <= 15 );
    
    UtilSleep( 500 );
    DWORD cLoop2 = g_cLoop;
    CHECK( ( cLoop2 >= cLoop1 ) && ( cLoop2 <= cLoop1 + 1 ) );
    g_cLoop = 0;
    
    pds.EnterPerimeter();
    UtilSleep( 1000 );
    
    cLoop1 = g_cLoop;
    CHECK( cLoop1 > 0 && cLoop1 <= 15 );
    g_cLoop = 0;

    pds.EnterPerimeter();
    UtilSleep( 1000 );
    
    cLoop1 = g_cLoop;
    CHECK( cLoop1 > 0 && cLoop1 <= 15 );
    g_cLoop = 0;

    pds.LeavePerimeter();
    g_cLoop = 0;
    UtilSleep( 1000 );

    pds.LeavePerimeter();

    cLoop1 = g_cLoop;
    CHECK( cLoop1 > 0 && cLoop1 <= 15 );
    
    UtilSleep( 500 );
    cLoop2 = g_cLoop;
    CHECK( ( cLoop2 >= cLoop1 ) && ( cLoop2 <= cLoop1 + 1 ) );

    pds.TermPatrolDog();
}


JETUNITTEST( NORM, NormCompareBasic )
{
    CHECK( 0 == NORMCompareLocaleName( L"en-US", L"en-US" ) );
    CHECK( 0 == NORMCompareLocaleName( L"en-US", L"eN-US" ) );
    CHECK( 0 == NORMCompareLocaleName( L"en-US", L"en-us" ) );

    CHECK( 0 != NORMCompareLocaleName( L"en-US", L"eN-UN" ) );
    CHECK( 0 != NORMCompareLocaleName( L"en-US", L"eN-Un" ) );

    CHECK( NORMCompareLocaleName( L"en-US", L"en-UA" ) > 0 );
    CHECK( NORMCompareLocaleName( L"en-US", L"en-UX" ) < 0 );

    CHECK( NORMCompareLocaleName( L"en-US", L"en-Ua" ) > 0 );
    CHECK( NORMCompareLocaleName( L"en-US", L"en-Ux" ) < 0 );

    CHECK( NORMCompareLocaleName( L"en-US", L"eN-USA" ) < 0 );
    CHECK( NORMCompareLocaleName( L"en-USA", L"eN-US" ) > 0 );

    CHECK( NORMCompareLocaleName( L"abc", L"cba" ) < 0 );
}

//  Helpers for the NORMCompareLocaleName() tests

WCHAR WchLimitedChar( __in const WCHAR wchHint )
{
    #define __wide_toupper(c)      ( (((c) >= L'a') && ((c) <= L'z')) ? ((c) - L'a' + L'A') : (c) )

    WCHAR wch = 1;  // should be invalid and assert locale compare later
    switch( rand() % 8 )
    {
    case 0: // 25% chance of same char
    case 1:
        if ( wchHint != L'\0' )
        {
            wch = wchHint;
            break;
        }
        // fall through
    case 2:
        if ( wchHint != L'\0' )
        {
            wch = __wide_toupper( wchHint );
            break;
        }
        // fall through
    case 3:
        wch = L'A' + rand() % 24;
        break;
    case 4:
        wch = L'a' + rand() % 24;
        break;
    case 5:
        wch = L'0' + rand() % 10;
        break;
    case 6:
        wch = ( ( rand() % 2 ) == 0 ) ? L'-' : L'_';
        break;
    case 7:
        wch = L'\0';    // NUL terminate.
        break;
    default:
        AssertSz( fFalse, "Something wrong" );
    }

    return wch;
}

VOID MakeLimitedCharStrings( __out_ecount(cch) WCHAR * wszStr1, __out_ecount(cch) WCHAR * wszStr2, __in const INT cch )
{
    for ( INT ich = 0; ich < cch - 1; ich++ )
    {
        wszStr1[ich] = WchLimitedChar( L'\0' );
        wszStr2[ich] = WchLimitedChar( wszStr1[ich] );
    }
    wszStr1[cch - 1] = L'\0';
    wszStr2[cch - 1] = L'\0';
}

// kind of sign equal ... consider 0 a separate "3rd sign" for comparison
BOOL FCmpSignEqual( __in const INT i1, __in const INT i2 )
{
    if ( i1 < 0 )
    {
        return i2 < 0;
    }
    if ( i1 > 0 )
    {
        return i2 > 0;
    }
    Assert( i1 == 0 );
    return i2 == 0;
}

JETUNITTEST( NORM, NormCompareAdvRandom )
{
    INT baseseed = TickOSTimeCurrent();
    wprintf( L"baseseed = %d\n", baseseed );
    for ( ULONG iter = 0; iter < 66000; iter++ )
    {
        srand( baseseed + iter );
        WCHAR wszStr1[6];
        WCHAR wszStr2[6];
        MakeLimitedCharStrings( wszStr1, wszStr2, _countof( wszStr1 ) );
        if ( !FCmpSignEqual( _wcsicmp( wszStr1, wszStr2 ), NORMCompareLocaleName( wszStr1, wszStr2 ) ) )
        {
            CHECK( FCmpSignEqual( _wcsicmp( wszStr1, wszStr2 ), NORMCompareLocaleName( wszStr1, wszStr2 ) ) );
            wprintf( L"ERROR DETAILS: seed = %d, icmp = %d != %d, strs = '%ws' / '%ws'\n",
                        baseseed + iter, _wcsicmp( wszStr1, wszStr2 ), NORMCompareLocaleName( wszStr1, wszStr2 ), wszStr1, wszStr2 );
        }
    }
}

UtilSystemBetaConfig    g_rgbetaconfigs [];

JETUNITTEST( SYSINFO, BetaFeaturesShouldHaveMatchingIndexAndFeatureIdValue )
{
    for( ULONG featureid = 0; featureid < EseFeatureMax; featureid++ )
    {
        //  Ensure in-order tracking to use fast lookup.
        CHECK( g_rgbetaconfigs[featureid].ulFeatureId == featureid );
    }
}

extern INT usbsmPrimaryEnvironments;
JETUNITTEST( SYSINFO, BetaFeaturesShouldHaveOneStandardMode )
{
    for( ULONG featureid = 0; featureid < EseFeatureMax; featureid++ )
    {
        //  At the very minimum it should be enabled for test.
        CHECK( ( g_rgbetaconfigs[featureid].usbsm & usbsmPrimaryEnvironments ) != 0 );
    }
}

extern INT usbsmExFeatures;
JETUNITTEST( SYSINFO, BetaFeaturesShouldNotReuseOtherFeaturesUsbsmExFeatEnum )
{
    INT rgusbsmExFeatureFlags[EseFeatureMax] = { 0 };

    for( ULONG featureid = 0; featureid < EseFeatureMax; featureid++ )
    {
        if ( g_rgbetaconfigs[featureid].usbsm & usbsmExFeatures )
        {
            for( ULONG featureidCompare = 0; featureidCompare < featureid; featureidCompare++ )
            {
                //  No one should re-use anyone else's usbsmExFeat* flag for their new feature, go get your own! ;-)
                CHECK( rgusbsmExFeatureFlags[featureidCompare] != ( g_rgbetaconfigs[featureid].usbsm & usbsmExFeatures ) );
            }
            //  Add this flag to set of seen usbsmExFeat* flags.
            rgusbsmExFeatureFlags[featureid] = ( g_rgbetaconfigs[featureid].usbsm & usbsmExFeatures );
        }
        else
        {
            Assert( rgusbsmExFeatureFlags[featureid] == 0 );
        }
    }
}

#ifdef DEBUG
extern INT usbsmExFeatureMin;
extern INT usbsmExFeatureMax;
JETUNITTEST( SYSINFO, BetaFeaturesExFeatEnumShouldBeListedInOneFeatureLine )
{
    INT rgusbsmExFeatureFlags[EseFeatureMax];

    for( INT usbsmCheck = usbsmExFeatureMin; usbsmCheck < usbsmExFeatureMax; usbsmCheck += 0x01000000 /* increment only top BYTE */ )
    {
        // if we did our increment right, we shouldn't have altered anything outside the usbsmExFeatureMask
        Assert( ( usbsmCheck & ~usbsmExFeatures ) == 0 );

        BOOL fFoundFeature = fFalse;
        for( ULONG featureid = 0; featureid < EseFeatureMax; featureid++ )
        {
            if ( usbsmCheck == ( g_rgbetaconfigs[featureid].usbsm & usbsmExFeatures ) )
            {
                //  Yeah we found a feature with it!
                fFoundFeature = fTrue;
                break;
            }
        }
        //  Every feature should be listed in a feature line ... well except for negative test
        //  cases and if we deprecate features out of order  Not quite sure how to do this best.
        CHECK( fFoundFeature
                    || usbsmCheck == 0x02000000 /* usbsmExFeatNegTest */
                    || usbsmCheck == 0x03000000 /* usbsmExFeatNegTestToo */ );
    }
}
#endif // DEBUG

#pragma warning( pop ) // Unreferenced variables in Unit Tests.

