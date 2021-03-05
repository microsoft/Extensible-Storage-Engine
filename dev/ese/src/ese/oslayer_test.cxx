// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

ULONG IOSDiskIUrgentLevelFromQOS( _In_ const OSFILEQOS grbitQOS );
LONG CioOSDiskIFromUrgentQOS( _In_ const OSFILEQOS grbitQOS, _In_ const DWORD cioUrgentMaxMax );

JETUNITTEST( OSDISK, QosToUrgentLevelConversions )
{


    CHECK( qosIODispatchUrgentBackgroundMin == QosOSFileFromUrgentLevel( 1 ) );
    CHECK( qosIODispatchUrgentBackgroundMax == QosOSFileFromUrgentLevel( 127 ) );


    for( ULONG iUrgentLevel = 1; iUrgentLevel <= qosIODispatchUrgentBackgroundLevelMax; iUrgentLevel++ )
    {


        CHECK( iUrgentLevel == IOSDiskIUrgentLevelFromQOS( QosOSFileFromUrgentLevel( iUrgentLevel ) ) );
    }

    for( ULONG cioMaxMax = 4 ; cioMaxMax <= 1024 ; cioMaxMax++ )
    {

        CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 1 ), cioMaxMax ) >= 1 );


        CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( qosIODispatchUrgentBackgroundLevelMax ), cioMaxMax ) == (LONG)cioMaxMax );


        CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 1 ), cioMaxMax ) <
                    CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 / 4  ), cioMaxMax ) );
        CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 / 4 ), cioMaxMax ) <
                    CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 / 2  ), cioMaxMax ) );
        CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 / 2 ), cioMaxMax ) <
                    CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 * 3 / 4  ), cioMaxMax ) );
        if ( cioMaxMax >= 5 )
        {
            CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 * 3 / 4 ), cioMaxMax ) <
                        CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( 127 ), cioMaxMax ) );
        }

    }



    ULONG rgCioMaxMax [] = { 4, 8 , 9, 10, 16, 32, 64, 96, 127, 128, 129, 256, 386, 511, 512 , 513, 1024 };
    for( ULONG i = 0; i < _countof( rgCioMaxMax ); i++ )
    {
        for( ULONG iUrgentLevel = 1; iUrgentLevel <= qosIODispatchUrgentBackgroundLevelMax; iUrgentLevel++ )
        {

            if ( i > 1 )
            {

                CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( iUrgentLevel ), rgCioMaxMax[i-1] ) <=
                            CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( iUrgentLevel ), rgCioMaxMax[i] ) );
            }

            if ( iUrgentLevel > 1 )
            {

                CHECK( CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( iUrgentLevel - 1 ), rgCioMaxMax[i] ) <=
                            CioOSDiskIFromUrgentQOS( QosOSFileFromUrgentLevel( iUrgentLevel ), rgCioMaxMax[i] ) );
            }
        }
    }

    

}


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
#pragma warning( disable: 4101 4189 )

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
#endif
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


WCHAR WchLimitedChar( __in const WCHAR wchHint )
{
    #define __wide_toupper(c)      ( (((c) >= L'a') && ((c) <= L'z')) ? ((c) - L'a' + L'A') : (c) )

    WCHAR wch = 1;
    switch( rand() % 8 )
    {
    case 0:
    case 1:
        if ( wchHint != L'\0' )
        {
            wch = wchHint;
            break;
        }
    case 2:
        if ( wchHint != L'\0' )
        {
            wch = __wide_toupper( wchHint );
            break;
        }
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
        wch = L'\0';
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
        CHECK( g_rgbetaconfigs[featureid].ulFeatureId == featureid );
    }
}

extern INT usbsmPrimaryEnvironments;
JETUNITTEST( SYSINFO, BetaFeaturesShouldHaveOneStandardMode )
{
    for( ULONG featureid = 0; featureid < EseFeatureMax; featureid++ )
    {
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
                CHECK( rgusbsmExFeatureFlags[featureidCompare] != ( g_rgbetaconfigs[featureid].usbsm & usbsmExFeatures ) );
            }
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

    for( INT usbsmCheck = usbsmExFeatureMin; usbsmCheck < usbsmExFeatureMax; usbsmCheck += 0x01000000  )
    {
        Assert( ( usbsmCheck & ~usbsmExFeatures ) == 0 );

        BOOL fFoundFeature = fFalse;
        for( ULONG featureid = 0; featureid < EseFeatureMax; featureid++ )
        {
            if ( usbsmCheck == ( g_rgbetaconfigs[featureid].usbsm & usbsmExFeatures ) )
            {
                fFoundFeature = fTrue;
                break;
            }
        }
        CHECK( fFoundFeature
                    || usbsmCheck == 0x02000000 
                    || usbsmCheck == 0x03000000  );
    }
}
#endif

#pragma warning( pop )

