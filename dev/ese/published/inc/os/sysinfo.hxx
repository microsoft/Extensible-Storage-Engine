// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_SYSINFO_HXX_INCLUDED
#define _OS_SYSINFO_HXX_INCLUDED


//
//  Hardware/CPU Capabilities
//

//  does the processor have SSE abilities. _mm_prefetch is a SSE instruction

BOOL FSSEInstructionsAvailable();

//  does the processor have SSE2 abilities

BOOL FSSE2InstructionsAvailable();

//  does the processor support POPCNT instruction

BOOL FPopcntAvailable();

//  do the processor and OS both support AVX

BOOL FAVXEnabled();


//
//  System Attributes
//

//  retrieves system major version

DWORD DwUtilSystemVersionMajor();

//  retrieves system minor version

DWORD DwUtilSystemVersionMinor();

//  retrieves system major build number

DWORD DwUtilSystemBuildNumber();

//  retrieves system service pack number

DWORD DwUtilSystemServicePackNumber();


//
//  Process Attributes
//

//  returns the current process' name

const WCHAR* WszUtilProcessName();

//  returns the current process' path

const WCHAR* WszUtilProcessPath();

//  returns the current process' file name

const WCHAR* WszUtilProcessFileName();

//  returns the current process' friendly name.

const WCHAR* WszUtilProcessFriendlyName();

//  returns the current process ID

const DWORD DwUtilProcessId();

//  returns the number of system processors on which the current process can execute

const DWORD CUtilProcessProcessor();

//  returns fTrue if the current process is terminating abnormally

const BOOL FUtilProcessAbort();

//  returns whether the current process is Packaged (Win8 only).

const BOOL FUtilProcessIsPackaged();

//
//  Image Attributes
//

//  retrieves image base address

const VOID * PvUtilImageBaseAddress();

//  retrieves image name

const WCHAR* WszUtilImageName();

//  retrieves image path

const WCHAR* WszUtilImagePath();

//  retrieves image version name

const WCHAR* WszUtilImageVersionName();

//  retrieves image major version

DWORD DwUtilImageVersionMajor();

//  retrieves image minor version

DWORD DwUtilImageVersionMinor();

//  retrieves image major build number

DWORD DwUtilImageBuildNumberMajor();

//  retrieves image minor build number

DWORD DwUtilImageBuildNumberMinor();

//  retrieves image build class

const WCHAR* WszUtilImageBuildClass();

//
//  Runtime State Attributes
//

//  are we currently running under OS setup/install?

BOOL FOSSetupRunning();

//
//  System / Process Level Station Identification
//

void OSSysTraceStationId( const DWORD /* TraceStationIdentificationReason */ tsidr );

//
//  The Badly Implemented Form of Beta Features
//

//  helpers for private feature staging checks (FUtilSystemBetaFeatureEnabled)

enum UtilSystemBetaSiteMode;    //  usbsm

// stored in .fStaticFeature (normally just use a bool, but want to make sure it can't
// get confused with the real values of true/false we pass back).
#define fFeatureDynamic     (-1)
#define fFeatureStatic      (-2)

typedef struct _UtilSystemBetaConfig
{
    LONG                    fSuppressInfoEvent;
    LONG                    fStaticFeature;
    ULONG                   ulFeatureId;
    UtilSystemBetaSiteMode  usbsm;
}
UtilSystemBetaConfig;
extern UtilSystemBetaConfig g_rgbetaconfigs[];
void UtilReportBetaFeatureInUse( const INST * const pinst, const UtilSystemBetaSiteMode usbsmCurrent, const ULONG featureid, PCWSTR const wszFeatureName );

// used to initialize client's (i.e. INST's or oslayerunit's) rgfStaticFeatures array
#define fUninitBetaFeature                  (-3)

//  list of features that can be privately staged

//    note: please keep this list dense and sorted - do not be afraid to remove entries.

enum EseBetaFeatures
{
    EseTestFeatures,

    // add before this line

    //  Unit testing of BetaFeatures feature.
#ifndef OS_LAYER_VIOLATIONS
    //  also update g_rgbetaconfigs in sysinfo.cxx!
    EseTestCase,
    EseTestCaseTwo,
    EseTestCaseThree,
    EseRiskyFeatTest,
#endif
    EseFeatureMax
};

//  determine if the specified feature is enabled in the current staging environment

inline BOOL FUtilSystemBetaFeatureEnabled_( const INST * const pinst, BOOL * const rgfStaticFeatures, const UtilSystemBetaSiteMode usbsmCurrent, const ULONG featureid, PCWSTR const wszFeatureName, const BOOL fTestStagingOnly = fFalse )
{
    extern INT usbsmPrimaryEnvironments;
    extern INT usbsmExFeatures;

    const INT usbsmStageCurrent = usbsmCurrent & usbsmPrimaryEnvironments;
    const INT usbsmFeatureCurrent = usbsmCurrent & usbsmExFeatures;

    //  While the g_rgbetaconfigs table may use flags to enable, the actual setting _should_ be
    //  a single bit - environment.
    Expected( usbsmStageCurrent == 0 || FPowerOf2( usbsmStageCurrent ) );

    Assert( g_rgbetaconfigs[featureid].fStaticFeature == fFeatureStatic || g_rgbetaconfigs[featureid].fStaticFeature == fFeatureDynamic );  // only values should be in use in here.

    //  We do not allow static features to pass a NULL for the static state, except for fTestStagingOnly.  And we should
    //  probably remove that.
    Assert( g_rgbetaconfigs[featureid].fStaticFeature != fFeatureStatic || rgfStaticFeatures != NULL || fTestStagingOnly );

    Enforce( featureid < EseFeatureMax );

    //  A previous (EseSetDbVersion) feature and a test feature (EseTestFeatures) are passing NULL rgfStaticFeatures b/c they 
    //  don't have an pinst.  Highly undesirable, but difficult to address at this point.
    if ( rgfStaticFeatures )
    {
        OnDebug( const BOOL fInstCurrent = (BOOL)*(volatile LONG*)( &( rgfStaticFeatures[featureid] ) ) );
        Assert( fInstCurrent == fUninitBetaFeature || fInstCurrent == fFalse || fInstCurrent == fTrue );    // only values should be in use in here.
    }

    if ( rgfStaticFeatures != NULL  && rgfStaticFeatures[featureid] != fUninitBetaFeature )
    {
        Assert( g_rgbetaconfigs[featureid].fStaticFeature == fFeatureStatic );  // should be configured a static feature, or rgfStaticFeatures[featureid] should not have been updated.
        Assert( rgfStaticFeatures[featureid] == fFalse || rgfStaticFeatures[ featureid ] == fTrue );    // should have rational answer
        return rgfStaticFeatures[featureid];
    }

    // All features are implicitly staged in debug first
    const BOOL fEnabledCurrently =
                    ( ( usbsmStageCurrent != 0 ) && //  no beta feature can be enabled if we're not in some kind of beta mode
                        ( usbsmStageCurrent == ( g_rgbetaconfigs[featureid].usbsm & usbsmStageCurrent ) ) )
                        ||
                    ( ( usbsmFeatureCurrent != 0 ) &&
                        ( g_rgbetaconfigs[featureid].usbsm & usbsmFeatureCurrent ) )
#ifdef DEBUG
                        ||
                    ( ( usbsmStageCurrent == 0 ) && //  no beta mode set, assume on if we're debug and the feature is on in all test envs.
                        ( 0x80/*usbsmTestEnvDebugMode*/ == ( g_rgbetaconfigs[featureid].usbsm & 0x80/*usbsmTestEnvDebugMode*/ ) ) )
#endif
                    ;

    if ( fTestStagingOnly )
    {
        //  this means this is a test check that has no material usage of the feature, and so should not trigger
        //  any of the below, setting in static or logging that feature is in use.
        return fEnabledCurrently;
    }

    Assert( g_rgbetaconfigs[featureid].fStaticFeature != fFeatureStatic || rgfStaticFeatures != NULL );

    Assert( fEnabledCurrently != fFeatureStatic && fEnabledCurrently != fFeatureDynamic && fEnabledCurrently != fUninitBetaFeature );
    Assert( g_rgbetaconfigs[featureid].fStaticFeature != fUninitBetaFeature ); // shouldn't be used in this variable.

    if ( g_rgbetaconfigs[featureid].fStaticFeature == fFeatureStatic )
    {
        Assert( fEnabledCurrently != fFeatureStatic && fEnabledCurrently != fFeatureDynamic && fEnabledCurrently != fUninitBetaFeature );
        const LONG winner = AtomicCompareExchange( (LONG*)&( rgfStaticFeatures[featureid] ), fUninitBetaFeature, (LONG)fEnabledCurrently );
        if ( winner != fUninitBetaFeature )
        {
            //  we didn't win, someone else has figure it out ... return thier answer now (as they would've done the event log)
            Assert( winner == fTrue || winner == fFalse );
            return winner;
        }
        // else we did win, but fall through so we log the first iteration
    }

    const BOOL fEnabled = g_rgbetaconfigs[featureid].fStaticFeature == fFeatureDynamic ?
                                fEnabledCurrently :
                                rgfStaticFeatures[featureid];

    Assert( fEnabled == fFalse || fEnabled == fTrue );

    if ( fEnabled && !g_rgbetaconfigs[featureid].fSuppressInfoEvent )
    {
        UtilReportBetaFeatureInUse( pinst, usbsmCurrent, featureid, wszFeatureName );
    }

    return fEnabled;
}

//  determine if the specified feature is enabled in the current staging environment

#define RgfStaticFeatures( pinst )          ( ( pinst == NULL ) ?  NULL : ( (BOOL*)(pinst->m_rgfStaticBetaFeatures) ) )
#define FUtilSystemBetaFeatureEnabled( pinst, featureid )       \
                FUtilSystemBetaFeatureEnabled_( pinst, RgfStaticFeatures( pinst ), (UtilSystemBetaSiteMode)UlParam( pinst, JET_paramStageFlighting ), featureid, L#featureid )

//  returns fTrue if idle activity should be avoided

BOOL FUtilSystemRestrictIdleActivity();

//  retrieves licensing policy configuration

ERR ErrUtilSystemSlConfiguration(
    _In_z_ const WCHAR *    pszValueName,
    _Out_   DWORD *         pdwValue );

//  Determines if the "Downgrade Window" has expired, and an OS update is no longer removable / rollbackable.

ERR ErrUtilOsDowngradeWindowExpired( _Out_ BOOL * pfExpired );

#endif  //  _OS_SYSINFO_HXX_INCLUDED

