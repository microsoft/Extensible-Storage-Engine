// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"


PERSISTED
const FormatVersions g_rgfmtversEngine[] = {



    { JET_efvExtHdrRootFieldAutoIncStorageStagedToDebug,{ 1568,20,0 },  { 8,4,3 },   { 0,0,0 } },

#ifdef DEBUG
    { JET_efvWindows10Rtm,                    { 1568,20,0 },  { 8,5,11 },  { 0,0,0 } },
#else
    { JET_efvWindows10Rtm,                    { 1568,20,0 },  { 8,4,3 },   { 0,0,0 } },
#endif


    { JET_efvPersistedLostFlushMapStagedToDebug,        { 1568,20,0 },  { 8,5,13 },  { 3,0,0 } },
    { JET_efvExchange2016Rtm,                 { 1568,20,0 },  { 8,5,14 },  { 3,0,0 } },
    { JET_efvWindows10v2Rtm,                  { 1568,20,0 },  { 8,5,14 },  { 3,0,0 } },
    { JET_efvSupportNlsInvariantLocale,                 { 1568,20,0 },  { 8,5,14 },  { 3,0,0 } },
    { JET_efvEncryptedColumns,                          { 1568,20,0 },  { 8,5,15 },  { 3,0,0 } },
    { JET_efvLoggingDeleteOfTableFcbs,                  { 1568,20,0 },  { 8,5,16 },  { 3,0,0 } },
    { JET_efvExchange2016Cu1Rtm  ,            { 1568,20,0 },  { 8,5,16 },  { 3,0,0 } },

    { JET_efvSetDbVersion        ,            { 1568,20,20 },  { 8,6,20 },  { 3,0,0 } },

    { JET_efvExtHdrRootFieldAutoIncStorageReleased,     { 1568,30,40 },  { 8,6,20 },  { 3,0,0 } },

    { JET_efvXpress9Compression  ,           { 1568,40,60 },  { 8,7,40 },  { 3,0,0} },

    { JET_efvUppercaseTextNormalization,      { 1568,50,80 },   { 8,7,40 },   { 3,0,0 } },
    { JET_efvDbscanHeaderHighestPageSeen,     { 1568,50,100 },  { 8,7,40 },   { 3,0,0 } },
    { JET_efvEscrow64,                        { 1568,60,120 },  { 8,20,60 },  { 3,0,0 } },
    { JET_efvSynchronousLVCleanup,            { 1568,60,140 },  { 8,20,60 },  { 3,0,0 } },
    { JET_efvLid64,                           { 1568,70,160 },  { 8,20,60 },  { 3,0,0 } },
    { JET_efvShrinkEof,                       { 1568,80,180 },  { 8,30,80 },  { 3,0,0 } },
    { JET_efvLogNewPage,                      { 1568,90,200 },  { 8,40,100 }, { 3,0,0 } },
    { JET_efvRootPageMove,                    { 1568,100,220 }, { 8,50,120 }, { 3,0,0 } },
    { JET_efvScanCheck2,                      { 1568,100,220 }, { 8,60,140 }, { 3,0,0 } },
    { JET_efvLgposLastResize,                 { 1568,110,240 }, { 8,60,140 }, { 3,0,0 } },
    { JET_efvShelvedPages,                    { 1568,120,260 }, { 8,60,140 }, { 3,0,0 } },
    { JET_efvShelvedPagesRevert,              { 1568,130,280 }, { 8,60,140 }, { 3,0,0 } },
    { JET_efvShelvedPages2,                   { 1568,140,300 }, { 8,60,140 }, { 3,0,0 } },
    { JET_efvLogtimeGenMaxRequired,           { 1568,150,320 }, { 8,60,140 }, { 3,0,0 } },
    { JET_efvVariableDbHdrSignatureSnapshot,  { 1568,160,340 }, { 8,60,140 }, { 3,0,0 } },
    { JET_efvLowerMinReqLogGenOnRedo,         { 1568,160,360 }, { 8,60,140 }, { 3,0,0 } },
    { JET_efvDbTimeShrink,                    { 1568,160,360 }, { 8,70,160 }, { 3,0,0 } },
    { JET_efvXpress10Compression,             { 1568,170,380 }, { 8,80,180 }, { 3,0,0 } },
    { JET_efvRevertSnapshot,                  { 1568,180,400 }, { 8,90,200 }, { 3,0,0 } },
    { JET_efvApplyRevertSnapshot,             { 1568,190,420 }, { 8,90,200 }, { 3,0,0 } },
};

const INT g_cfmtversEngine = _countof( g_rgfmtversEngine );
const INT g_ifmtversLastFeature = _countof(g_rgfmtversEngine) - 1;

FormatVersions g_fmtversEngineMax = g_rgfmtversEngine[g_ifmtversLastFeature];
FormatVersions g_fmtversEngineDefault = g_rgfmtversEngine[g_ifmtversLastFeature];

const FormatVersions * PfmtversEngineMin()
{
    return &( g_rgfmtversEngine[0] );
}

const FormatVersions * PfmtversEngineMax()
{
    return &( g_rgfmtversEngine[ _countof( g_rgfmtversEngine ) - 1] );
}

const FormatVersions * PfmtversEngineDefault()
{
    return &( g_rgfmtversEngine[ g_ifmtversLastFeature ] );
}

const INT g_ifmtversSafetyVersion = 3;
const FormatVersions * PfmtversEngineSafetyVersion()
{
#ifdef ESENT
    Assert( g_rgfmtversEngine[ g_ifmtversSafetyVersion ].efv == JET_efvExchange2016Rtm );
#else
    Assert( g_rgfmtversEngine[ g_ifmtversSafetyVersion ].efv == JET_efvExchange2016Rtm );
#endif
    return &( g_rgfmtversEngine[ g_ifmtversSafetyVersion ] );
}

ERR ErrGetDesiredVersion( _In_ const INST * const pinstStaging, _In_ JET_ENGINEFORMATVERSION efvDesired, _Out_ const FormatVersions ** const ppfmtversDesired, const BOOL fTestStagingOnly )
{
    Assert( ppfmtversDesired != NULL );
    *ppfmtversDesired = NULL;

    const BOOL fStripOptionalFlags = ( efvDesired & JET_efvAllowHigherPersistedFormat ) == JET_efvAllowHigherPersistedFormat;
    if ( fStripOptionalFlags )
    {
        efvDesired = efvDesired & ~JET_efvAllowHigherPersistedFormat;
    }

    Assert( efvDesired >= EfvMinSupported() );
    Assert( efvDesired != JET_efvUsePersistedFormat );

    Assert( EfvMaxSupported() ==  PfmtversEngineMax()->efv );

    if ( efvDesired == JET_efvUseEngineDefault )
    {
        Assert( g_fmtversEngineDefault.dbv.ulDbMajorVersion == g_rgfmtversEngine[ g_ifmtversLastFeature ].dbv.ulDbMajorVersion );
        Assert( g_fmtversEngineDefault.dbv.ulDbUpdateMajor == g_rgfmtversEngine[ g_ifmtversLastFeature ].dbv.ulDbUpdateMajor );
        *ppfmtversDesired = &g_fmtversEngineDefault;

        OSTrace( JET_tracetagUpgrade, OSFormat( "Upgrade returning %p (x.%d.z/a.%d.c) ",
                            *ppfmtversDesired, (*ppfmtversDesired)->dbv.ulDbUpdateMajor, (*ppfmtversDesired)->lgv.ulLGVersionUpdateMajor ) );

        return JET_errSuccess;
    }

    for ( ULONG i = 0; i < _countof(g_rgfmtversEngine); ++i )
    {
        if ( g_rgfmtversEngine[i].efv == efvDesired )
        {
            *ppfmtversDesired = &g_rgfmtversEngine[i];

            OSTrace( JET_tracetagUpgrade, OSFormat( "Upgrade returning %p (x.%d.z/a.%d.c) ",
                                *ppfmtversDesired, (*ppfmtversDesired)->dbv.ulDbUpdateMajor, (*ppfmtversDesired)->lgv.ulLGVersionUpdateMajor ) );

            return JET_errSuccess;
        }
    }
    Assert( *ppfmtversDesired == NULL );

    AssertSz( FNegTest( fInvalidAPIUsage ), "Intermediate EFV (%d) provided that was above min, and below max, but not in g_fmtvers table!", efvDesired );
    return ErrERRCheck( efvDesired < PfmtversEngineMax()->efv ? JET_errEngineFormatVersionNoLongerSupportedTooLow : JET_errEngineFormatVersionNotYetImplementedTooHigh );
}

BOOL FInEseutilPossibleUsageError();

ERR ErrDBFindHighestMatchingDbMajors( _In_ const DbVersion& dbvFromFileHeader, _Out_ const FormatVersions ** const ppfmtversMatching, _In_ const INST * const pinstStaging, const BOOL fDbMayBeTooHigh )
{
    Assert( ppfmtversMatching != NULL );
    *ppfmtversMatching = NULL;

    Assert( EfvMaxSupported() == PfmtversEngineMax()->efv );

    for ( INT i = g_ifmtversLastFeature; i >= 0; i-- )
    {
        const INT icmp = CmpDbVer( g_rgfmtversEngine[i].dbv, dbvFromFileHeader );
        Assert( abs( icmp ) >= 0 );

        if (
                    ( icmp == 0 || FUpdateMinorMismatch( icmp ) ) )
        {
            *ppfmtversMatching = &g_rgfmtversEngine[i];
            Assert( (*ppfmtversMatching)->efv != JET_efvUsePersistedFormat );
            Assert( (*ppfmtversMatching)->efv != JET_efvUseEngineDefault );
            break;
        }
    }

    if ( *ppfmtversMatching == NULL )
    {
        AssertSz( fDbMayBeTooHigh || FInEseutilPossibleUsageError(), "Asked for %d.%d.%d DB version, we don't have an entry in g_rgfmtversEngine for this!", dbvFromFileHeader.ulDbMajorVersion, dbvFromFileHeader.ulDbUpdateMajor, dbvFromFileHeader.ulDbUpdateMinor );
        return ErrERRCheck( JET_errInvalidDatabaseVersion );
    }
    return JET_errSuccess;
}

ERR ErrLGFindHighestMatchingLogMajors( _In_ const LogVersion& lgvFromLogHeader, _Out_ const FormatVersions ** const ppfmtversMatching )
{
    Assert( ppfmtversMatching != NULL );
    *ppfmtversMatching = NULL;

    Assert( EfvMaxSupported() == PfmtversEngineMax()->efv );

    for ( INT i = g_ifmtversLastFeature; i >= 0; i-- )
    {
        const INT icmp = CmpLgVer( g_rgfmtversEngine[i].lgv, lgvFromLogHeader );
        Assert( abs( icmp ) >= 0 );

        if ( icmp == 0 || FUpdateMinorMismatch( icmp ) )
        {
            *ppfmtversMatching = &g_rgfmtversEngine[i];
            Assert( (*ppfmtversMatching)->efv != JET_efvUsePersistedFormat );
            Assert( (*ppfmtversMatching)->efv != JET_efvUseEngineDefault );
            break;
        }
    }

    if ( *ppfmtversMatching == NULL )
    {
        AssertSz( fFalse, "Asked for %d.%d.%d log version, we don't have an entry in g_rgfmtversEngine for this!", lgvFromLogHeader.ulLGVersionMajor, lgvFromLogHeader.ulLGVersionUpdateMajor, lgvFromLogHeader.ulLGVersionUpdateMinor );
        return ErrERRCheck( JET_errBadLogVersion );
    }
    return JET_errSuccess;
}


void __cdecl OSStrCbAppendFormatW ( __inout_bcount(cbBuffer) PWSTR wszBuffer, size_t cbBuffer, __format_string PCWSTR cwszFormat, ... )
{
    va_list alist;
    va_start( alist, cwszFormat );
    const size_t cbUsed = wcslen( wszBuffer ) * sizeof(WCHAR);
    Assert( cbUsed < cbBuffer );
    Assert( ( cbUsed % sizeof(WCHAR) ) == 0 );
    HRESULT hr = StringCbVPrintfW( wszBuffer + ( cbUsed / sizeof(WCHAR) ), cbBuffer - cbUsed, cwszFormat, alist );
#ifdef DEBUG
    CallS( ErrFromStrsafeHr( hr ) );
#endif
    va_end( alist );
}


void FormatEfvSetting( const JET_ENGINEFORMATVERSION efvFullParam, _Out_writes_bytes_(cbEfvSetting) WCHAR * const wszEfvSetting, _In_range_( 240, 240  ) const ULONG cbEfvSetting )
{
    Assert( cbEfvSetting >= ( cchFormatEfvSetting * sizeof(WCHAR) ) );
    Assert( wszEfvSetting );

    wszEfvSetting[0];

    JET_ENGINEFORMATVERSION efvBaseValue = efvFullParam;
    const JET_ENGINEFORMATVERSION efvSignalBit = 0x40000000;
    const BOOL fJET_efvAllowHigherPersistedFormat = !!( ( efvBaseValue & JET_efvAllowHigherPersistedFormat ) == JET_efvAllowHigherPersistedFormat );
    if ( fJET_efvAllowHigherPersistedFormat )
    {
        efvBaseValue = ( efvBaseValue & ~JET_efvAllowHigherPersistedFormat ) | efvSignalBit;
    }
    
    const BOOL fJET_efvUseEngineDefault = !!( ( efvBaseValue & JET_efvUseEngineDefault ) == JET_efvUseEngineDefault );
    if( fJET_efvUseEngineDefault )
    {
        efvBaseValue = ( efvBaseValue & ~JET_efvUseEngineDefault) | efvSignalBit;
    }
    const BOOL fJET_efvUsePersistedFormat = !!( ( efvBaseValue & JET_efvUsePersistedFormat ) == JET_efvUsePersistedFormat );
    if ( fJET_efvUsePersistedFormat )
    {
        efvBaseValue = ( efvBaseValue & ~JET_efvUsePersistedFormat ) | efvSignalBit;
    }
    if ( fJET_efvAllowHigherPersistedFormat | fJET_efvUseEngineDefault | fJET_efvUsePersistedFormat )
    {
        efvBaseValue = efvBaseValue & ~efvSignalBit;
    }


    Assert( efvBaseValue < 0x00FFFFFF );

    OSStrCbFormatW( wszEfvSetting, cbEfvSetting, L"0x%X (", efvFullParam);
    BOOL fNeedOr = fFalse;
    if ( efvBaseValue != 0 )
    {
        OSStrCbAppendFormatW( wszEfvSetting, cbEfvSetting, L"%d", efvBaseValue );
        fNeedOr = fTrue;
    }
    if ( fJET_efvUseEngineDefault )
    {
        OSStrCbAppendFormatW( wszEfvSetting, cbEfvSetting, L"%hs%hs", fNeedOr ? " | " : "", "JET_efvUseEngineDefault" );
        fNeedOr = fTrue;
    }
    if ( fJET_efvUsePersistedFormat )
    {
        OSStrCbAppendFormatW( wszEfvSetting, cbEfvSetting, L"%hs%hs", fNeedOr ? " | " : "", "JET_efvUsePersistedFormat" );
        fNeedOr = fTrue;
    }
    if ( fJET_efvAllowHigherPersistedFormat )
    {
        OSStrCbAppendFormatW( wszEfvSetting, cbEfvSetting, L"%hs%hs", fNeedOr ? " | " : "", "JET_efvAllowHigherPersistedFormat" );
        fNeedOr = fTrue;
    }
    OSStrCbAppendW( wszEfvSetting, cbEfvSetting, L")" );
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( SysVerEngineFormatVersion, EngineFormatVersionFormatting )
{
    WCHAR wszTestBuffer[cchFormatEfvSetting];

    CHECK( 0 == wcscmp( L"0x0 ()", WszFormatEfvSetting( 0, wszTestBuffer, sizeof(wszTestBuffer) ) ) );

    CHECK( 0 == wcscmp( L"0x22D8 (8920)", WszFormatEfvSetting( 8920, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x22D8 (8920)", WszFormatEfvSetting( JET_efvExchange2016Cu1Rtm, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x410022D8 (8920 | JET_efvAllowHigherPersistedFormat)", WszFormatEfvSetting( 8920 | JET_efvAllowHigherPersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x40000001 (JET_efvUseEngineDefault)", WszFormatEfvSetting( JET_efvUseEngineDefault, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x40000002 (JET_efvUsePersistedFormat)", WszFormatEfvSetting( JET_efvUsePersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );

    CHECK( 0 == wcscmp( L"0x41000001 (JET_efvUseEngineDefault | JET_efvAllowHigherPersistedFormat)", WszFormatEfvSetting( JET_efvUseEngineDefault | JET_efvAllowHigherPersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x41000002 (JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat)", WszFormatEfvSetting( JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x40000003 (JET_efvUseEngineDefault | JET_efvUsePersistedFormat)", WszFormatEfvSetting( JET_efvUseEngineDefault | JET_efvUsePersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x41000003 (JET_efvUseEngineDefault | JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat)", WszFormatEfvSetting( JET_efvUsePersistedFormat | JET_efvUseEngineDefault | JET_efvAllowHigherPersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x41000103 (256 | JET_efvUseEngineDefault | JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat)", WszFormatEfvSetting( 0x100 | JET_efvUseEngineDefault | JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );

    CHECK( 0 == wcscmp( L"0x417F0103 (8323328 | JET_efvUseEngineDefault | JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat)", WszFormatEfvSetting( 0x7F0100 | JET_efvUseEngineDefault | JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
}

#endif

JET_ENGINEFORMATVERSION EfvBeBestMapping( const JET_ENGINEFORMATVERSION efvParamRaw,
                                          const BOOL         fLogged,
                                          const DbVersion&   dbvFromFileHeader,
                                          const LogVersion&  lgvFromFileHeader )
{
    JET_ENGINEFORMATVERSION efvParam            = efvParamRaw;
    const JET_ENGINEFORMATVERSION efvValue      = efvParamRaw & ~( JET_efvAllowHigherPersistedFormat );
    const FormatVersions * pfmtversMatchLow     = NULL;
    const FormatVersions * pfmtversMatchHigh    = NULL;
    const FormatVersions * pfmtversMatchLogLow  = NULL;
    const FormatVersions * pfmtversMatchLogHigh = NULL;

    for ( INT i = g_ifmtversLastFeature; i >= 0; i-- )
    {
        INT icmp = CmpDbVer( g_rgfmtversEngine[ i ].dbv, dbvFromFileHeader );
        if ( icmp == 0 )
        {
            if( pfmtversMatchHigh == NULL )
            {
                pfmtversMatchHigh = &g_rgfmtversEngine[ i ];
            }
            pfmtversMatchLow = &g_rgfmtversEngine[ i ];
        }

        icmp = CmpLgVer( g_rgfmtversEngine[ i ].lgv, lgvFromFileHeader );
        if ( icmp == 0 )
        {
            if ( pfmtversMatchLogHigh == NULL )
            {
                pfmtversMatchLogHigh = &g_rgfmtversEngine[ i ];
            }
            pfmtversMatchLogLow = &g_rgfmtversEngine[ i ];
        }
    }



    if ( fLogged && 
         pfmtversMatchLow != NULL && pfmtversMatchHigh != NULL && pfmtversMatchLogHigh != NULL &&
         pfmtversMatchLogHigh->efv >= pfmtversMatchLow->efv &&
         pfmtversMatchLogHigh->efv < pfmtversMatchHigh->efv )
    {
        pfmtversMatchHigh = pfmtversMatchLogHigh;
    }

    if ( pfmtversMatchHigh == NULL )
    {

        const INT icmpDbAboveTable = CmpDbVer( dbvFromFileHeader, g_fmtversEngineMax.dbv );

        AssertTrack( icmpDbAboveTable != 0, "BeBestMapShouldHaveMatchedDbv" );
        if ( icmpDbAboveTable < 0 )
        {
            efvParam = fLogged ? 20 : 30;
        }
        else
        {
            efvParam = fLogged ? 21 : 31;
        }
    }
    else if ( pfmtversMatchLogHigh == NULL )
    {

        const INT icmpLogAboveTable = CmpLgVer( lgvFromFileHeader, g_fmtversEngineMax.lgv );

        AssertTrack( icmpLogAboveTable != 0, "BeBestMapShouldHaveMatchedDbv" );
        if ( icmpLogAboveTable < 0 )
        {
            efvParam = fLogged ? 22 : 32;
        }
        else
        {
            efvParam = fLogged ? 27 : 37;
        }
    }
    else if ( pfmtversMatchLogLow->efv > pfmtversMatchHigh->efv )
    {
        efvParam = fLogged ? 16 : 36;
    }
    else if ( pfmtversMatchLogHigh->efv < pfmtversMatchLow->efv )
    {
        efvParam = fLogged ? 13 : 33;
    }
    else if ( pfmtversMatchHigh->efv == efvValue )
    {
        AssertRTL( efvParam == ( efvValue | ( efvParam & JET_efvAllowHigherPersistedFormat ) ) );
        AssertRTL( ( efvValue & JET_efvAllowHigherPersistedFormat ) != JET_efvAllowHigherPersistedFormat );
        efvParam = efvValue;
    }
    else if ( efvParam == JET_efvUseEngineDefault ||
              efvParam == JET_efvUsePersistedFormat )
    {
        efvParam = pfmtversMatchHigh->efv;
    }
    else if ( pfmtversMatchLow->efv <= efvValue &&
              efvValue <= pfmtversMatchHigh->efv )
    {
        Assert( efvValue != pfmtversMatchHigh->efv );
        efvParam = efvValue | JET_efvAllowHigherPersistedFormat;        
    }
    else
    {
        Assert( efvValue < pfmtversMatchLow->efv ||
                efvValue > pfmtversMatchHigh->efv );
        if ( efvValue < pfmtversMatchLow->efv )
        {
            efvParam = fLogged ? 14 : 34;
        }
        else
        {
            efvParam = fLogged ? 15 : 35;
        }
    }

    return efvParam;
}

void FormatDbvEfvMapping( const IFMP ifmp, _Out_writes_bytes_( cbSetting ) WCHAR * wszSetting, _In_ const ULONG cbSetting )
{
    const INST * const pinst                    = PinstFromIfmp( ifmp );
    const BOOL fLogged                          = pinst->m_plog && g_rgfmp[ ifmp ].FLogOn();

    const DbVersion dbvFromFileHeader           = g_rgfmp[ ifmp ].Pdbfilehdr()->Dbv();
    const LogVersion lgvFromFileHeader          = fLogged ? pinst->m_plog->LgvForReporting() : g_rgfmtversEngine[ g_ifmtversLastFeature ].lgv;

    
    const JET_ENGINEFORMATVERSION efvParam = EfvBeBestMapping( (JET_ENGINEFORMATVERSION)UlParam( pinst, JET_paramEngineFormatVersion ),
                                                               fLogged,
                                                               dbvFromFileHeader,
                                                               lgvFromFileHeader );

    
    OSStrCbFormatW( wszSetting, cbSetting, L"%d.%d.%d (%u)", dbvFromFileHeader.ulDbMajorVersion, dbvFromFileHeader.ulDbUpdateMajor, dbvFromFileHeader.ulDbUpdateMinor, (ULONG)efvParam );
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( SysVerEngineFormatVersion, CheckBestMappingWorksForAllExistingEfvs )
{
    wprintf( L"\n" );
    for( ULONG iefv = 0; iefv < _countof( g_rgfmtversEngine ); iefv++ )
    {
        const JET_ENGINEFORMATVERSION efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                                                    fTrue,
                                                                    g_rgfmtversEngine[ iefv ].dbv,
                                                                    g_rgfmtversEngine[ iefv ].lgv );
        wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
        if ( efvMapped & JET_efvAllowHigherPersistedFormat )
        {
            CHECK( ( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat ) == efvMapped );
        }
        else
        {
            CHECK( g_rgfmtversEngine[ iefv ].efv == efvMapped );
        }
    }
}

JETUNITTEST( SysVerEngineFormatVersion, CheckLowDbv )
{
    wprintf( L"\n" );
    JET_ENGINEFORMATVERSION efvMapped = 1;
    const ULONG iefv = 17;
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100  );

    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  g_rgfmtversEngine[ iefv - 1 ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 16 == efvMapped );
    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  g_rgfmtversEngine[ iefv - 1 ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 36 == efvMapped );
}

JETUNITTEST( SysVerEngineFormatVersion, CheckHighDbv )
{
    wprintf( L"\n" );
    JET_ENGINEFORMATVERSION efvMapped = 1;
    const ULONG iefv = 17;
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100  );

    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  g_rgfmtversEngine[ iefv + 1 ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 13 == efvMapped );
    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  g_rgfmtversEngine[ iefv + 1 ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 33 == efvMapped );
}

JETUNITTEST( SysVerEngineFormatVersion, CheckLowLog )
{
    wprintf( L"\n" );
    JET_ENGINEFORMATVERSION efvMapped = 1;
    const ULONG iefv = 17;
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100  );

    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv - 1 ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 13 == efvMapped );
    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv - 1 ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 33 == efvMapped );
}

JETUNITTEST( SysVerEngineFormatVersion, CheckHighLog )
{
    wprintf( L"\n" );
    JET_ENGINEFORMATVERSION efvMapped = 1;
    const ULONG iefv = 17;
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100  );

    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv + 1 ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 16 == efvMapped );
    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv + 1 ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 36 == efvMapped );
}

JETUNITTEST( SysVerEngineFormatVersion, CheckLowEfv )
{
    wprintf( L"\n" );
    JET_ENGINEFORMATVERSION efvMapped = 1;
    const ULONG iefv = 17;
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100  );

    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv - 1 | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 14 == efvMapped );
    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv - 1 | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 34 == efvMapped );
}

JETUNITTEST( SysVerEngineFormatVersion, CheckHighEfv )
{
    wprintf( L"\n" );
    JET_ENGINEFORMATVERSION efvMapped = 1;
    const ULONG iefv = 17;
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100  );

    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv + 1 | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 15 == efvMapped );
    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv + 1 | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 35 == efvMapped );
}

JETUNITTEST( SysVerEngineFormatVersion, CheckWayLowDbv )
{
    wprintf( L"\n" );
    JET_ENGINEFORMATVERSION efvMapped = 1;
    const ULONG iefv = 17;
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100  );

    DbVersion dbvWayLow = { 1568, 4, 0 };

    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  dbvWayLow,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 20 == efvMapped );
    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  dbvWayLow,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 30 == efvMapped );
}

JETUNITTEST( SysVerEngineFormatVersion, CheckWayHighDbv )
{
    wprintf( L"\n" );
    JET_ENGINEFORMATVERSION efvMapped = 1;
    const ULONG iefv = 17;
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100  );

    DbVersion dbvWayHigh = { 1568, 65000, 0 };

    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  dbvWayHigh,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 21 == efvMapped );
    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  dbvWayHigh,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 31 == efvMapped );
}

JETUNITTEST( SysVerEngineFormatVersion, CheckWayLowLog )
{
    wprintf( L"\n" );
    JET_ENGINEFORMATVERSION efvMapped = 1;
    const ULONG iefv = 17;
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100  );

    LogVersion lgvWayLow = { 6, 1, 0 };

    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  lgvWayLow );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 22 == efvMapped );
    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  lgvWayLow );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 32 == efvMapped );
}

JETUNITTEST( SysVerEngineFormatVersion, CheckWayHighLog )
{
    wprintf( L"\n" );
    JET_ENGINEFORMATVERSION efvMapped = 1;
    const ULONG iefv = 17;
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100  );

    LogVersion lgvWayHigh = { 9, 1, 0 };

    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  lgvWayHigh );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 27 == efvMapped );
    efvMapped = EfvBeBestMapping( g_rgfmtversEngine[ iefv ].efv | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  lgvWayHigh );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 37 == efvMapped );
}


JETUNITTEST( SysVerEngineFormatVersion, CheckWayLowEfv )
{
    wprintf( L"\n" );
    JET_ENGINEFORMATVERSION efvMapped = 1;
    const ULONG iefv = 17;
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100  );

    efvMapped = EfvBeBestMapping( 5000 | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 14 == efvMapped );
    efvMapped = EfvBeBestMapping( 5000 | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 34 == efvMapped );
}

JETUNITTEST( SysVerEngineFormatVersion, CheckWayHighEfv )
{
    wprintf( L"\n" );
    JET_ENGINEFORMATVERSION efvMapped = 1;
    const ULONG iefv = 17;
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100  );

    efvMapped = EfvBeBestMapping( 65000 | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 15 == efvMapped );
    efvMapped = EfvBeBestMapping( 65000 | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 35 == efvMapped );
}

#endif

