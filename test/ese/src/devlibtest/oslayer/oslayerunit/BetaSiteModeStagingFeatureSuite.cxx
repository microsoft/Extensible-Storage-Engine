// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

// needed for JET errors
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

    //  requires the inst-specific static array to be init'd

    boolset( rgfStaticFeaturesInstOne, fUninitBetaFeature, sizeof(rgfStaticFeaturesInstOne) );
    boolset( rgfStaticFeaturesInstTwo, fUninitBetaFeature, sizeof(rgfStaticFeaturesInstTwo) );

    //  We thunk out UlParam which is used in #define of FUtilSystemBetaFeatureEnabled().
    //#define UlParam( x, y )       ((UtilSystemBetaSiteMode)JET_bitStageTestEnvLocalMode)
    //OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );


    #undef UlParam
    #define UlParam( x, y )             ((UtilSystemBetaSiteMode)JET_bitStageTestEnvAlphaMode)

    #undef RgfStaticFeatures
    #define RgfStaticFeatures( x )      rgfStaticFeaturesInstOne

    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );   

    //  Change the prod beta stage mode and the answers should not change for the first/static feature
    #undef UlParam
    #define UlParam( x, y )             ((UtilSystemBetaSiteMode)JET_bitStageProdBetaMode)

    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );

    //  Now switch to InstTwo
    #undef RgfStaticFeatures
    #define RgfStaticFeatures( x )      rgfStaticFeaturesInstTwo
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    //  Now switch BACK to InstOne
    #undef RgfStaticFeatures
    #define RgfStaticFeatures( x )      rgfStaticFeaturesInstOne
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );

    //  Switch mode back, and back to inst two ... should still be off, as was original state.
    #undef UlParam
    #define UlParam( x, y )             ((UtilSystemBetaSiteMode)JET_bitStageTestEnvAlphaMode)
    #undef RgfStaticFeatures
    #define RgfStaticFeatures( x )      rgfStaticFeaturesInstTwo
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );

    //  Reset the inst array and two should be enabled.
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

    //  requires the inst-specific static array to be init'd

    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

    //  We thunk out UlParam which is used in #define of FUtilSystemBetaFeatureEnabled().
    //#define UlParam( x, y )       ((UtilSystemBetaSiteMode)JET_bitStageTestEnvLocalMode)
    //OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );


    //
    //      First test a "TestEnv" mode.
    //

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageTestEnvAlphaMode)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

    //  Now test that these answer the same immediately after (silly case)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

    //  Change the prod beta stage mode and the answers should not change for the first/static feature
    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageProdBetaMode)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

    //  Change to non-set usbm ....
    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)0)
    // All staged features are enabled on debug (IF NO stage is set, AND they are staged in test usbsmTestEnvDebugMode/0x80),
    //  this 2nd feature doesn't qualify b/c of the later (it is only staged to PROD in sysinfo.cxx).
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );


    //
    //      Second test a "Prod" mode
    //
    //  Note: Should give exact opposite answers as previous sections first checks.

    //  Verify we can reset the static features via (resetting static array)
    
    //OSTestCall( ErrOSInit() ); - used to use OS term
    //OSTerm(); 
    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageProdBetaMode)
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

    //  Change the to test env stage mode and the answers should not change for static feature, but should for 2nd/dynamic one.
    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageTestEnvAlphaMode)
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

    //  Verify OSInit/Term Cycle has no effect resetting (it used to reset EseTestCaseTwo pre-inst specific staging static features, emulated through rgfStaticFeatures)
    OSTestCall( ErrOSInit() );
    OSTerm(); 

    //  Change to non-set usbm ....
    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)0)
    // All staged features are enabled on debug (IF NO stage is set, AND they are staged in test usbsmTestEnvDebugMode/0x80),
    //  this 2nd feature doesn't qualify b/c of the later (it is only staged to PROD in sysinfo.cxx).
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );


    //
    //      Lastly we will test the non-set 0 / default staged in DEBUG mode
    //

    //  Verify we can reset the static features via (resetting static array)
    //OSTestCall( ErrOSInit() );
    //OSTerm(); 
    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)0)
#ifdef DEBUG
    // All staged features are enabled on debug (IF NO stage is set, AND they are staged in test usbsmTestEnvDebugMode/0x80),
    //  this 2nd feature doesn't qualify b/c of the later (it is only staged to PROD in sysinfo.cxx).
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );
#else
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

#endif

    //  Now test that these answer the same immediately after (silly case)

#ifdef DEBUG
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );
#else
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

#endif

    //  Now test set the TestEnv and prove the setttings don't change.

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageTestEnvAlphaMode)
#ifdef DEBUG
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );
#else
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );    
#endif

    //  Change the prod beta stage mode and the answers should not change for the first static feature, and should change for the 2nd / dynamic one

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

    //  requires the inst-specific static array to be init'd

    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

#ifdef DEBUG
    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)0)
    // All staged features are enabled on debug (IF NO stage is set, AND they are staged in test usbsmTestEnvDebugMode/0x80),
    //  this 2nd feature doesn't qualify b/c of the later, only staged to PROD.
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );
#else

    //  True on-prem / RTM production case: no JET_param value set (i.e. 0x0)
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

    //  requires the inst-specific static array to be init'd

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

    //  Since the EseTestCase is a static feature, reset it to ensure a good test ... 
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
    // All staged features are enabled on debug (IF NO stage is set)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
#endif

    //  Since the EseTestCase is a static feature, it must be reset ... 
    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageProdAlphaMode)
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );

    //  Since the EseTestCase is a static feature, it must be reset ... 
    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

#ifdef DEBUG
    #undef UlParam
    #define UlParam( x, y )     ((UtilSystemBetaSiteMode)0)
    // All staged features are enabled on debug (IF NO stage is set)
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
#endif

    //  Since the EseTestCase is a static feature, it must be reset ... 
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

    //  requires the inst-specific static array to be init'd

    boolset( rgfStaticFeatures, fUninitBetaFeature, sizeof(rgfStaticFeatures) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageProdBetaMode)
    
    //  Feature not under JET_bitStageProdBetaMode, so shouldn't be enabled.
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseRiskyFeatTest ) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)0x00000002/* usbsmExFeatRiskyFeatTest */ | JET_bitStageProdBetaMode)

    //  Feature enabled by explicit flag, not by JET_bitStageProdBetaMode.
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseRiskyFeatTest ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseTwo ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)JET_bitStageSelfhostAlphaMode)

    //  Feature enabled by JET_bitStageSelfhostAlphaMode mode, not by explicit bit.
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseRiskyFeatTest ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)0x00000008/* usbsmExFeatNegTest */ | JET_bitStageProdBetaMode)

    //  Feature not enabled by 2nd feature bit even with overlapping bit 0x01000000.
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseRiskyFeatTest ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)0x00000002/* usbsmExFeatRiskyFeatTest */ | JET_bitStageSelfhostAlphaMode)

    //  Feature enabled in both ways by explicit flag and by JET_bitStageSelfhostAlphaMode mode.
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseRiskyFeatTest ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)0x00000002/* usbsmExFeatRiskyFeatTest */ | (UtilSystemBetaSiteMode)0x00000020/* usbsmExFeatOtherFeatTest */ | JET_bitStageSelfhostAlphaMode)
    
    //  Two features enabled explicitly.
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseRiskyFeatTest ) );
    OSTestCheck( FUtilSystemBetaFeatureEnabled( NULL, EseTestCase ) );
    OSTestCheck( !FUtilSystemBetaFeatureEnabled( NULL, EseTestCaseThree ) );

#undef UlParam
#define UlParam( x, y )     ((UtilSystemBetaSiteMode)0x00000002/* usbsmExFeatRiskyFeatTest */ | (UtilSystemBetaSiteMode)0x00000020/* usbsmExFeatOtherFeatTest */ | JET_bitStageProdBetaMode)
    
    //  Two features enabled explicitly (and one by mode)
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


