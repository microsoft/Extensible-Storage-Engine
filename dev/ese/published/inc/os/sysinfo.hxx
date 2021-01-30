// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_SYSINFO_HXX_INCLUDED
#define _OS_SYSINFO_HXX_INCLUDED




BOOL FSSEInstructionsAvailable();


BOOL FSSE2InstructionsAvailable();


BOOL FPopcntAvailable();


BOOL FAVXEnabled();




DWORD DwUtilSystemVersionMajor();


DWORD DwUtilSystemVersionMinor();


DWORD DwUtilSystemBuildNumber();


DWORD DwUtilSystemServicePackNumber();




const WCHAR* WszUtilProcessName();


const WCHAR* WszUtilProcessPath();


const WCHAR* WszUtilProcessFileName();


const WCHAR* WszUtilProcessFriendlyName();


const DWORD DwUtilProcessId();


const DWORD CUtilProcessProcessor();


const BOOL FUtilProcessAbort();


const BOOL FUtilProcessIsPackaged();



const VOID * PvUtilImageBaseAddress();


const WCHAR* WszUtilImageName();


const WCHAR* WszUtilImagePath();


const WCHAR* WszUtilImageVersionName();


DWORD DwUtilImageVersionMajor();


DWORD DwUtilImageVersionMinor();


DWORD DwUtilImageBuildNumberMajor();


DWORD DwUtilImageBuildNumberMinor();


const WCHAR* WszUtilImageBuildClass();



BOOL FOSSetupRunning();


void OSSysTraceStationId( const DWORD  tsidr );



enum UtilSystemBetaSiteMode;

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

#define fUninitBetaFeature                  (-3)



enum EseBetaFeatures
{
    EseTestFeatures,


#ifndef OS_LAYER_VIOLATIONS
    EseTestCase,
    EseTestCaseTwo,
    EseTestCaseThree,
    EseRiskyFeatTest,
#endif
    EseFeatureMax
};


inline BOOL FUtilSystemBetaFeatureEnabled_( const INST * const pinst, BOOL * const rgfStaticFeatures, const UtilSystemBetaSiteMode usbsmCurrent, const ULONG featureid, PCWSTR const wszFeatureName, const BOOL fTestStagingOnly = fFalse )
{
    extern INT usbsmPrimaryEnvironments;
    extern INT usbsmExFeatures;

    const INT usbsmStageCurrent = usbsmCurrent & usbsmPrimaryEnvironments;
    const INT usbsmFeatureCurrent = usbsmCurrent & usbsmExFeatures;

    Expected( usbsmStageCurrent == 0 || FPowerOf2( usbsmStageCurrent ) );

    Assert( g_rgbetaconfigs[featureid].fStaticFeature == fFeatureStatic || g_rgbetaconfigs[featureid].fStaticFeature == fFeatureDynamic );

    Assert( g_rgbetaconfigs[featureid].fStaticFeature != fFeatureStatic || rgfStaticFeatures != NULL || fTestStagingOnly );

    Enforce( featureid < EseFeatureMax );

    if ( rgfStaticFeatures )
    {
        OnDebug( const BOOL fInstCurrent = (BOOL)*(volatile LONG*)( &( rgfStaticFeatures[featureid] ) ) );
        Assert( fInstCurrent == fUninitBetaFeature || fInstCurrent == fFalse || fInstCurrent == fTrue );
    }

    if ( rgfStaticFeatures != NULL  && rgfStaticFeatures[featureid] != fUninitBetaFeature )
    {
        Assert( g_rgbetaconfigs[featureid].fStaticFeature == fFeatureStatic );
        Assert( rgfStaticFeatures[featureid] == fFalse || rgfStaticFeatures[ featureid ] == fTrue );
        return rgfStaticFeatures[featureid];
    }

    const BOOL fEnabledCurrently =
                    ( ( usbsmStageCurrent != 0 ) &&
                        ( usbsmStageCurrent == ( g_rgbetaconfigs[featureid].usbsm & usbsmStageCurrent ) ) )
                        ||
                    ( ( usbsmFeatureCurrent != 0 ) &&
                        ( g_rgbetaconfigs[featureid].usbsm & usbsmFeatureCurrent ) )
#ifdef DEBUG
                        ||
                    ( ( usbsmStageCurrent == 0 ) &&
                        ( 0x80 == ( g_rgbetaconfigs[featureid].usbsm & 0x80 ) ) )
#endif
                    ;

    if ( fTestStagingOnly )
    {
        return fEnabledCurrently;
    }

    Assert( g_rgbetaconfigs[featureid].fStaticFeature != fFeatureStatic || rgfStaticFeatures != NULL );

    Assert( fEnabledCurrently != fFeatureStatic && fEnabledCurrently != fFeatureDynamic && fEnabledCurrently != fUninitBetaFeature );
    Assert( g_rgbetaconfigs[featureid].fStaticFeature != fUninitBetaFeature );

    if ( g_rgbetaconfigs[featureid].fStaticFeature == fFeatureStatic )
    {
        Assert( fEnabledCurrently != fFeatureStatic && fEnabledCurrently != fFeatureDynamic && fEnabledCurrently != fUninitBetaFeature );
        const LONG winner = AtomicCompareExchange( (LONG*)&( rgfStaticFeatures[featureid] ), fUninitBetaFeature, (LONG)fEnabledCurrently );
        if ( winner != fUninitBetaFeature )
        {
            Assert( winner == fTrue || winner == fFalse );
            return winner;
        }
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


#define RgfStaticFeatures( pinst )          ( ( pinst == NULL ) ?  NULL : ( (BOOL*)(pinst->m_rgfStaticBetaFeatures) ) )
#define FUtilSystemBetaFeatureEnabled( pinst, featureid )       \
                FUtilSystemBetaFeatureEnabled_( pinst, RgfStaticFeatures( pinst ), (UtilSystemBetaSiteMode)UlParam( pinst, JET_paramStageFlighting ), featureid, L#featureid )


BOOL FUtilSystemRestrictIdleActivity();


ERR ErrUtilSystemSlConfiguration(
    _In_z_ const WCHAR *    pszValueName,
    _Out_   DWORD *         pdwValue );


ERR ErrUtilOsDowngradeWindowExpired( _Out_ BOOL * pfExpired );

#endif

