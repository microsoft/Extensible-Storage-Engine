// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"


const TICK g_dtickOffsetToGuaranteeUpdate = 30 * 60 * 1000;

#define PrintStatsBasicPercentages( piostats )  PrintStatsBasicPercentages_( piostats, #piostats )
void PrintStatsBasicPercentages_( _In_ CIoStats * piostats, CHAR * szPiostatsName )
{
    const BOOL f = piostats->FStartUpdate_( TickOSTimeCurrent() + g_dtickOffsetToGuaranteeUpdate );
    Assert( f );
    wprintf( L"\t Results: %hs (%p)\n", szPiostatsName, piostats );
    wprintf( L"\t\t  Cio = %I64u \n", piostats->CioAccumulated() );
    wprintf( L"\t\t  Ave = %I64u \n", piostats->CusecAverage() );
    wprintf( L"\t\t   0%% = %I64u \n", piostats->CusecPercentile( 0 ) );
    wprintf( L"\t\t  10%% = %I64u \n", piostats->CusecPercentile( 10 ) );
    wprintf( L"\t\t  50%% = %I64u \n", piostats->CusecPercentile( 50 ) );
    wprintf( L"\t\t  90%% = %I64u \n", piostats->CusecPercentile( 90 ) );
    wprintf( L"\t\t  99%% = %I64u \n", piostats->CusecPercentile( 99 ) );
    wprintf( L"\t\t 100%% = %I64u \n", piostats->CusecPercentile( 100 ) );
    piostats->FinishUpdate( 1  );
}

#define OSTestSame( exp, act )      \
    {               \
    auto expT = (exp);      \
    auto actT = (act);      \
    if ( expT != actT )     \
        {           \
        wprintf( L"TestSame: %hs == %hs  FAIL  %I64u == %I64u\n", #exp, #act, expT, actT ); \
        OSTestCheck( expT == actT );    \
    }           \
}


CUnitTest( IoStatsBasicAcceptsAllValues, 0x0, "Tests that Io latency histograms can accept all values" );
ERR IoStatsBasicAcceptsAllValues::ErrTest()
{
    JET_ERR         err = JET_errSuccess;

    COSLayerPreInit     oslayer;
    CIoStats * piostatsDatacenter = NULL;
    CIoStats * piostatsClient = NULL;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }


    OSTestCall( CIoStats::ErrCreateStats( &piostatsDatacenter, fFalse ) );
    OSTestCall( CIoStats::ErrCreateStats( &piostatsClient, fTrue ) );

    wprintf( L"\tTesting IoStats functionality ...\n");

    piostatsDatacenter->Tare();

    for( ULONG i = 0; i < 20000000 + 100000; i++ )
    {
        piostatsDatacenter->AddIoSample_( i );
        piostatsClient->AddIoSample_( i );
    }

    PrintStatsBasicPercentages( piostatsClient );

    OSTestCheck( !piostatsClient->FStartUpdate() );

    PrintStatsBasicPercentages( piostatsDatacenter );

HandleError:

    delete piostatsClient;
    delete piostatsDatacenter;

    return err;
}

void MakeNormalizedSamples( _Inout_ CIoStats * piostats, _In_ INT cqwSamples, _In_ SAMPLE * rgqwSamples, _Out_writes_( cSamples ) SAMPLE * rgqwNormalizedSamples )
{
    ERR err;
    for ( LONG i = 0; i < cqwSamples; i++ )
    {
        piostats->Tare();
        piostats->AddIoSample_( rgqwSamples[ i ] );
        OSTestCheck( piostats->FStartUpdate_( TickOSTimeCurrent() + g_dtickOffsetToGuaranteeUpdate ) );
        rgqwNormalizedSamples[ i ] = piostats->CusecPercentile( 75 );
        piostats->FinishUpdate( 1  );
    }
    piostats->Tare();
    return;
HandleError:
    Assert( fFalse );
}

CUnitTest( IoStatsBasicAcceptsLimitedValues, 0x0, "Tests that Io latency histograms can accept limited values" );
ERR IoStatsBasicAcceptsLimitedValues::ErrTest()
{
    JET_ERR         err = JET_errSuccess;

    COSLayerPreInit     oslayer;
    CIoStats * piostatsDatacenter = NULL;
    CIoStats * piostatsClient = NULL;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }


    OSTestCall( CIoStats::ErrCreateStats( &piostatsDatacenter, fFalse ) );
    OSTestCall( CIoStats::ErrCreateStats( &piostatsClient, fTrue ) );

    wprintf( L"\tTesting IoStats specific values functionality ...\n");

    wprintf( L"\tTest samples, and normalized client / datacenter values...\n" );
    QWORD rgqwSamples [] = {
        0, 1, 2, 25, 49, 50, 51, 950, 999, 1000, 1001, 2499, 2500, 2501, 5000, 10000, 50000, 100000, 200000, 350000,
        1 * 1000000, 2 * 1000000, 5 * 1000000, 10 * 1000000, 20 * 1000000 - 1, 20 * 1000000, 20 * 1000000 + 1, 30 * 1000000, 60 * 1000000 - 1, 130 * 1000000 - 1
    };

    SAMPLE rgqwClientNormalizedSamples[ _countof( rgqwSamples ) ];
    MakeNormalizedSamples( piostatsClient, _countof( rgqwSamples ), rgqwSamples, rgqwClientNormalizedSamples );
    SAMPLE rgqwDatacenterNormalizedSamples[ _countof( rgqwSamples ) ];
    MakeNormalizedSamples( piostatsDatacenter, _countof( rgqwSamples ), rgqwSamples, rgqwDatacenterNormalizedSamples );

    for ( LONG iSampleOne = 0; iSampleOne < _countof( rgqwSamples ); iSampleOne++ )
    {
        for ( LONG iSampleTwo = -1; iSampleTwo < (LONG)_countof( rgqwSamples ); iSampleTwo++ )
        {
            if ( iSampleTwo == -1 )
            {
                wprintf( L"\t\tTesting single value: %I64u ... ", rgqwSamples[iSampleOne] );
            }
            else
            {
                wprintf( L"\t\tTesting two values: %I64u, %I64u ... ", rgqwSamples[iSampleOne], rgqwSamples[iSampleTwo] );
            }

            piostatsDatacenter->Tare();
            piostatsClient->Tare(); 

            piostatsDatacenter->AddIoSample_( rgqwSamples[iSampleOne] );
            piostatsClient->AddIoSample_( rgqwSamples[iSampleOne] );
            if ( iSampleTwo >= 0 )
            {
                piostatsDatacenter->AddIoSample_( rgqwSamples[iSampleTwo] );
                piostatsClient->AddIoSample_( rgqwSamples[iSampleTwo] );
            }


            SAMPLE qwLo = iSampleTwo >= 0 ? min( rgqwSamples[iSampleOne], rgqwSamples[iSampleTwo] ) : rgqwSamples[iSampleOne];
            SAMPLE qwHi = iSampleTwo >= 0 ? max( rgqwSamples[iSampleOne], rgqwSamples[iSampleTwo] ) : rgqwSamples[iSampleOne];

            OSTestCheck( piostatsClient->FStartUpdate_( TickOSTimeCurrent() + g_dtickOffsetToGuaranteeUpdate ) );
            if ( iSampleTwo == -1 )
            {
                OSTestSame( 1, piostatsClient->CioAccumulated() );
            }
            else
            {
                OSTestSame( 2, piostatsClient->CioAccumulated() );
            }
            SAMPLE qwNormLo = iSampleTwo >= 0 ? min( rgqwClientNormalizedSamples[ iSampleOne ], rgqwClientNormalizedSamples[ iSampleTwo ] ) : rgqwClientNormalizedSamples[ iSampleOne ];
            SAMPLE qwNormHi = iSampleTwo >= 0 ? max( rgqwClientNormalizedSamples[ iSampleOne ], rgqwClientNormalizedSamples[ iSampleTwo ] ) : rgqwClientNormalizedSamples[ iSampleOne ];
            OSTestCheck( ( ( qwLo + qwHi ) / 2 ) == piostatsClient->CusecAverage() );
            OSTestCheck( qwLo == piostatsClient->CusecPercentile( 0 ) );
            OSTestCheck( qwLo     == piostatsClient->CusecPercentile( 10 ) );
            OSTestCheck( qwNormLo == piostatsClient->CusecPercentile( 50 ) );
            OSTestCheck( qwNormHi == piostatsClient->CusecPercentile( 90 ) );
            OSTestCheck( qwNormHi == piostatsClient->CusecPercentile( 99 ) );
            OSTestCheck( qwHi == piostatsClient->CusecPercentile( 100 ) );
            piostatsClient->FinishUpdate( 1  );

            OSTestCheck( piostatsDatacenter->FStartUpdate_( TickOSTimeCurrent() + g_dtickOffsetToGuaranteeUpdate ) );
            if ( iSampleTwo == -1 )
            {
                OSTestCheck( 1 == piostatsDatacenter->CioAccumulated() );
            }
            else
            {
                OSTestCheck( 2 == piostatsDatacenter->CioAccumulated() );
            }
            qwNormLo = iSampleTwo >= 0 ? min( rgqwDatacenterNormalizedSamples[ iSampleOne ], rgqwDatacenterNormalizedSamples[ iSampleTwo ] ) : rgqwDatacenterNormalizedSamples[ iSampleOne ];
            qwNormHi = iSampleTwo >= 0 ? max( rgqwDatacenterNormalizedSamples[ iSampleOne ], rgqwDatacenterNormalizedSamples[ iSampleTwo ] ) : rgqwDatacenterNormalizedSamples[ iSampleOne ];
            OSTestCheck( ( ( qwLo + qwHi ) / 2 ) == piostatsDatacenter->CusecAverage() );
            OSTestCheck( qwLo     == piostatsDatacenter->CusecPercentile( 0 ) );
            OSTestCheck( qwLo     == piostatsDatacenter->CusecPercentile( 10 ) );
            OSTestCheck( qwNormLo == piostatsDatacenter->CusecPercentile( 50 ) );
            OSTestCheck( qwNormHi == piostatsDatacenter->CusecPercentile( 90 ) );
            OSTestCheck( qwNormHi == piostatsDatacenter->CusecPercentile( 99 ) );
            OSTestCheck( qwHi == piostatsDatacenter->CusecPercentile( 100 ) );
            piostatsDatacenter->FinishUpdate( 1  );

            wprintf( L"Done.\n" );
        }
    }

HandleError:

    delete piostatsClient;
    delete piostatsDatacenter;

    return err;
}

void VectorCopy( _In_ INT cqw, _In_ QWORD * rgqwSrc, _Out_writes_( cqw ) QWORD * rgqwDst )
{
    memcpy( rgqwDst, rgqwSrc, sizeof( rgqwSrc[0] ) * cqw );
}

void VectorMult( _In_ INT cqw,   QWORD * rgqw, QWORD qwMult )
{
    for ( LONG i = 0; i < cqw; i++ )
    {
        rgqw[ i ] *= qwMult;
    }
}

void AddAllSamples( _Inout_ CIoStats * piostats, _In_ const INT cqwSamples, _In_ const SAMPLE * const rgqwSamples )
{
    for ( LONG i = 0; i < cqwSamples; i++ )
    {
        piostats->AddIoSample_( rgqwSamples[ i ] );
    }
}
void TareAndAddAllSamples( _Inout_ CIoStats * piostats, _In_ const INT cqwSamples, _In_ SAMPLE * rgqwSamples )
{
    piostats->Tare();
    AddAllSamples( piostats, cqwSamples, rgqwSamples );
}

void PrintSamplesAndClientDatacenterNormalizedArrays( _In_ const INT cqwSamples, _In_ const SAMPLE * const rgqwSamples, _In_ const SAMPLE * const rgqwClientNormSamples, _In_ const SAMPLE * const rgqwDatacenterNormSamples )
{
    wprintf( L"\t Insert sample values:\n" );
    for ( LONG i = 0; i < cqwSamples; i++ )
    {
        wprintf( L"\t Sample[%d] = %I64u --> %I64u / %I64u\n", i, rgqwSamples[i], rgqwClientNormSamples[i], rgqwDatacenterNormSamples[i] );
    }
}

QWORD g_rgqwDistributionSeed [] = { 0, 10, 100, 100, 100, 100, 100, 250, 600, 900 };

JET_ERR TestDistributionOfTen( CIoStats * piostats, _In_ const INT cqwSamples, _In_ const SAMPLE * const rgqwDistribution, _In_ const SAMPLE * const rgqwNormSamples )
{
    ERR err = JET_errSuccess;

    OSTestCheck( piostats->FStartUpdate_( TickOSTimeCurrent() + g_dtickOffsetToGuaranteeUpdate ) );

    Assert( cqwSamples == 10 );
    OSTestCheck( piostats->CioAccumulated() == 10 );

    OSTestSame( rgqwDistribution[0], piostats->CusecPercentile( 0 ) );
    OSTestSame( rgqwNormSamples[0] , piostats->CusecPercentile( 10 ) );
    OSTestSame( rgqwNormSamples[5] , piostats->CusecPercentile( 50 ) );
    OSTestSame( rgqwNormSamples[8] , piostats->CusecPercentile( 90 ) );
    OSTestSame( rgqwNormSamples[9] , piostats->CusecPercentile( 99 ) );
    OSTestSame( rgqwDistribution[9], piostats->CusecPercentile( 100 ) );
    piostats->FinishUpdate( 1  );

HandleError:
    return err;
}

CUnitTest( IoStatsBasicTestInOrderDistributions, 0x0, "Tests that Io latency histograms can take a certain distribution of values." );
ERR IoStatsBasicTestInOrderDistributions::ErrTest()
{
    JET_ERR         err = JET_errSuccess;

    COSLayerPreInit     oslayer;
    CIoStats * piostatsDatacenter = NULL;
    CIoStats * piostatsClient = NULL;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }


    OSTestCall( CIoStats::ErrCreateStats( &piostatsClient, fFalse ) );
    OSTestCall( CIoStats::ErrCreateStats( &piostatsDatacenter, fTrue ) );

    for( ULONG iDecMult = 1; iDecMult < 9 ; iDecMult++ )
    {
        SAMPLE qwMult = 1;
        for( ULONG i = 0; i < iDecMult; i++ )
            qwMult = qwMult * 10;
            
        wprintf( L"\tTest sample distribution @ %I64u mult ...\n", qwMult );

        QWORD rgqwDistribution [ _countof( g_rgqwDistributionSeed ) ];
        VectorCopy( _countof( g_rgqwDistributionSeed ), g_rgqwDistributionSeed, rgqwDistribution );
        VectorMult( _countof( rgqwDistribution ), rgqwDistribution, qwMult );
        rgqwDistribution[0] = 3 * qwMult;
        Assert( rgqwDistribution[0] <= rgqwDistribution[1] );

        SAMPLE rgqwClientNormalizedSamples[ _countof( rgqwDistribution ) ];
        MakeNormalizedSamples( piostatsClient, _countof( rgqwDistribution ), rgqwDistribution, rgqwClientNormalizedSamples );
        SAMPLE rgqwDatacenterNormalizedSamples[ _countof( rgqwDistribution ) ];
        MakeNormalizedSamples( piostatsDatacenter, _countof( rgqwDistribution ), rgqwDistribution, rgqwDatacenterNormalizedSamples );

        PrintSamplesAndClientDatacenterNormalizedArrays( _countof( rgqwDistribution ), rgqwDistribution, rgqwClientNormalizedSamples, rgqwDatacenterNormalizedSamples );

        TareAndAddAllSamples( piostatsClient, _countof( rgqwDistribution ), rgqwDistribution );
        TareAndAddAllSamples( piostatsDatacenter, _countof( rgqwDistribution ), rgqwDistribution );

        PrintStatsBasicPercentages( piostatsClient );
        TestDistributionOfTen( piostatsClient, _countof( rgqwDistribution ), rgqwDistribution, rgqwClientNormalizedSamples );

        PrintStatsBasicPercentages( piostatsDatacenter );
        TestDistributionOfTen( piostatsDatacenter, _countof( rgqwDistribution ), rgqwDistribution, rgqwDatacenterNormalizedSamples );
    }

HandleError:

    delete piostatsClient;
    delete piostatsDatacenter;

    return err;
}


CUnitTest( IoStatsBasicTestKnownValuesAndPcts, 0x0, "Tests that Io latency histograms can take a few fixed sets of values and reports accurate percentages." );
ERR IoStatsBasicTestKnownValuesAndPcts::ErrTest()
{
    JET_ERR         err = JET_errSuccess;

    COSLayerPreInit     oslayer;
    CIoStats * piostatsDatacenter = NULL;
    CIoStats * piostatsClient = NULL;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }


    OSTestCall( CIoStats::ErrCreateStats( &piostatsClient, fFalse ) );
    OSTestCall( CIoStats::ErrCreateStats( &piostatsDatacenter, fTrue ) );

    wprintf( L"\tTesting IoStats specific values distribution ...\n" );

    wprintf( L"\tTest samples, and normalized client / datacenter values...\n" );
    QWORD rgqwDistribution [ _countof( g_rgqwDistributionSeed ) ];
    VectorCopy( _countof( g_rgqwDistributionSeed ), g_rgqwDistributionSeed, rgqwDistribution );

    SAMPLE rgqwClientNormalizedSamples[ _countof( rgqwDistribution ) ];
    MakeNormalizedSamples( piostatsClient, _countof( rgqwDistribution ), rgqwDistribution, rgqwClientNormalizedSamples );
    SAMPLE rgqwDatacenterNormalizedSamples[ _countof( rgqwDistribution ) ];
    MakeNormalizedSamples( piostatsDatacenter, _countof( rgqwDistribution ), rgqwDistribution, rgqwDatacenterNormalizedSamples );

    PrintSamplesAndClientDatacenterNormalizedArrays( _countof( rgqwDistribution ), rgqwDistribution, rgqwClientNormalizedSamples, rgqwDatacenterNormalizedSamples );

    TareAndAddAllSamples( piostatsClient, _countof( rgqwDistribution ), rgqwDistribution );
    TareAndAddAllSamples( piostatsDatacenter, _countof( rgqwDistribution ), rgqwDistribution );

    PrintStatsBasicPercentages( piostatsClient );
    PrintStatsBasicPercentages( piostatsDatacenter );

    PrintStatsBasicPercentages( piostatsClient );
    TestDistributionOfTen( piostatsClient, _countof( rgqwDistribution ), rgqwDistribution, rgqwClientNormalizedSamples );

    PrintStatsBasicPercentages( piostatsDatacenter );
    TestDistributionOfTen( piostatsDatacenter, _countof( rgqwDistribution ), rgqwDistribution, rgqwDatacenterNormalizedSamples );

HandleError:

    delete piostatsClient;
    delete piostatsDatacenter;

    return err;
}

