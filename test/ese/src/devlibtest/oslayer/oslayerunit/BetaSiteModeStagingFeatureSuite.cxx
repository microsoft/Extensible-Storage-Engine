// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

#if defined(BUILD_ENV_IS_NT) || defined(BUILD_ENV_IS_WPHONE)
#include <esent_x.h>
#endif
#ifdef BUILD_ENV_IS_EX
#include <jet.h>
#endif

void boolset( BOOL * const rgf, BOOL fValue, ULONG cbArray )
{
    Assert( ( cbArray % sizeof(BOOL) ) == 0 );
    const ULONG c = cbArray / sizeof(BOOL);
    for( ULONG i = 0; i < c; i++ )
    {
        rgf[i] = fValue;
    }
}


CUnitTest( OslayerCanMaintainTwoInstsWithDifferentStaticFeaturesEnabled, 0, "" )
ERR OslayerCanMaintainTwoInstsWithDifferentStaticFeaturesEnabled::ErrTest()
{
    JET_ERR         err     = JET_errSuccess;
    COSLayerPreInit     oslayer;
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

    BOOL rgfStaticFeaturesInstOne[EseFeatureMax];
    BOOL rgfStaticFeaturesInstTwo[EseFeatureMax];


    boolset( rgfStaticFeaturesInstOne, fUninitBetaFeature, sizeof(rgfStaticFeaturesInstOne) );
    boolset( rgfStaticFeaturesInstTwo, fUninitBetaFeature, sizeof(rgfStaticFeaturesInstTwo) );



    #undef UlParam
    #define UlParam( x, y )             ((UtilSystemBetaSiteMode)JET_bitStageTestEnvAlphaMode)

    #undef RgfStaticFeatures
    #define RgfStaticFeatures( x )      rgfStaticFeaturesInstOne

    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );   

    #undef UlParam
    #define UlParam( x, y )             ((UtilSystemBetaSiteMode)JET_bitStageProdBetaMode)

    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );

    #undef RgfStaticFeatures
    #define RgfStaticFeatures( x )      rgfStaticFeaturesInstTwo
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    #undef RgfStaticFeatures
    #define RgfStaticFeatures( x )      rgfStaticFeaturesInstOne
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );

    #undef UlParam
    #define UlParam( x, y )             ((UtilSystemBetaSiteMode)JET_bitStageTestEnvAlphaMode)
    #undef RgfStaticFeatures
    #define RgfStaticFeatures( x )      rgfStaticFeaturesInstTwo
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );

    boolset( rgfStaticFeaturesInstTwo, fUninitBetaFeature, sizeof(rgfStaticFeaturesInstTwo) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );

    err = JET_errSuccess;

HandleError:

    if ( err )
    {
        wprintf( L"\tDone(FAILED, err=%d).\n", err);
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    return err;
}

CUnitTest( OslayerEnablesRightFeaturesstaticallyWithinFeatureArrayCycle, 0, "" )
ERR OslayerEnablesRightFeaturesstaticallyWithinFeatureArrayCycle::ErrTest()
{
    JET_ERR         err     = JET_errSuccess;
    COSLayerPreInit     oslayer;
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

    BOOL rgfStaticFeatures[EseFeatureMax];
    #undef RgfStaticFeatures
    #define RgfStaticFeatures( x )      rgfStaticFeatures


    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );




    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageTestEnvAlphaMode)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageProdBetaMode)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)0)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );



    
    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageProdBetaMode)
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageTestEnvAlphaMode)
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

    OSTestCall( ErrOSInit() );
    OSTerm(); 

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)0)
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );



    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)0)
#ifdef DEBUG
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );
#else
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

#endif


#ifdef DEBUG
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );
#else
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

#endif


    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageTestEnvAlphaMode)
#ifdef DEBUG
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );
#else
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );    
#endif


    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageProdBetaMode)
#ifdef DEBUG
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );
#else
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) ); 
#endif

    


    err = JET_errSuccess;

HandleError:

    if ( err )
    {
        wprintf( L"\tDone(FAILED, err=%d).\n", err);
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    return err;
}

CUnitTest( OslayerDisablesAllFeaturesUnderOnPrem, 0, "" )
ERR OslayerDisablesAllFeaturesUnderOnPrem::ErrTest()
{
    JET_ERR         err     = JET_errSuccess;
    COSLayerPreInit     oslayer;
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

    BOOL rgfStaticFeatures[EseFeatureMax];
    #undef RgfStaticFeatures
    #define RgfStaticFeatures( x )      rgfStaticFeatures


    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

#ifdef DEBUG
    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)0)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );
#else

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)0x0)
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
#endif

    err = JET_errSuccess;

HandleError:

    if ( err )
    {
        wprintf( L"\tDone(FAILED, err=%d).\n", err);
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    return err;
}

CUnitTest( OslayerEnablesRightFeaturesUnderTestBits, 0, "" )
ERR OslayerEnablesRightFeaturesUnderTestBits::ErrTest()
{
    JET_ERR         err     = JET_errSuccess;
    COSLayerPreInit     oslayer;
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }
    OSTestCall( ErrOSInit() );

    BOOL rgfStaticFeatures[EseFeatureMax];
    #undef RgfStaticFeatures
    #define RgfStaticFeatures( x )      rgfStaticFeatures


    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );


    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageTestEnvAlphaMode)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageTestEnvBetaMode)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageTestEnvBetaMode)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );

    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

#ifdef DEBUG
    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageSelfhostAlphaMode)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
#else
    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageSelfhostAlphaMode)
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
#endif

#ifdef RTM
    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageSelfhostBetaMode)
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
#else
    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageSelfhostBetaMode)
#ifdef DEBUG
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
#else
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
#endif
#endif

#ifdef DEBUG
    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)0)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
#endif

    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageProdAlphaMode)
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );

    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

#ifdef DEBUG
    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)0)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
#endif

    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageProdBetaMode)
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );

    err = JET_errSuccess;

HandleError:

    OSTerm();

    if ( err )
    {
        wprintf( L"\tDone(FAILED, err=%d).\n", err);
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    return err;
}

CUnitTest( OslayerEnablesExFeaturesWhenSet, 0, "" )
ERR OslayerEnablesExFeaturesWhenSet::ErrTest()
{
    JET_ERR         err     = JET_errSuccess;
    COSLayerPreInit     oslayer;
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

    BOOL rgfStaticFeatures[EseFeatureMax];
#undef RgfStaticFeatures
#define RgfStaticFeatures( x )      rgfStaticFeatures


    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageProdBetaMode)
    
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseRiskyFeatTest ) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)0x00000002 | JET_bitStageProdBetaMode)

    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseRiskyFeatTest ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageSelfhostAlphaMode)

    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseRiskyFeatTest ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)0x00000008 | JET_bitStageProdBetaMode)

    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseRiskyFeatTest ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)0x00000002 | JET_bitStageSelfhostAlphaMode)

    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseRiskyFeatTest ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)0x00000002 | (UtilSystemBetaSiteMode)0x00000020 | JET_bitStageSelfhostAlphaMode)
    
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseRiskyFeatTest ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)0x00000002 | (UtilSystemBetaSiteMode)0x00000020 | JET_bitStageProdBetaMode)
    
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseRiskyFeatTest ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );


    err = JET_errSuccess;

HandleError:

    if ( err )
    {
        wprintf( L"\tDone(FAILED, err=%d).\n", err);
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    return err;
}


