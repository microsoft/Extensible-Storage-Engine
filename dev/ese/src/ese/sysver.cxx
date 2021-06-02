// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"


//      Engine format version (EFV) table mapping
//
//  Rules for choosing version numbers:
//      - UpdateMinors should jump by 20 each time, to leave room for SP/QFE updates.
//      - UpdateMajors should jump by 10 each time, to leave room for SP/QFE updates.
//      - Major version can jump by 1 or any value you want.
//
//  Marked PERSISTED because while the whole table is not persisted, individual rows can be 
//  used to the values in the various ESE file headers.  Entries once added to the table should 
//  not have their values changed.  You should append to the table newer / larger versions.
//  A service pack / QFE version might be able to be inserted, but should not shift any other
//  values.
PERSISTED
const FormatVersions g_rgfmtversEngine[] = {

    //  EngineFormatVersion / EFV                       DbVersion       LogVersion  FlushMapVer.

//  Historically preserved (and removed) versions in case relevant or needed.
// { JET_efvExchange2000Rtm                            { 1568,0x09,- }, { 7,4,- },  { -,-,- } },  // snapshot of the versions used by Exchange 2000 RTM. Note: Minor - 3704 (not 4000)
// { JET_efvExchange2003Rtm                            { 1568,0x09,- }, { 7,5,- },  { -,-,- } },  // snapshot of the versions used by Exchange 2003 RTM. Note: Minor - 3704 (not 4000)

    { JET_efvExtHdrRootFieldAutoIncStorageStagedToDebug,{ 1568,20,0 },  { 8,4,3 },   { 0,0,0 } },

#ifdef DEBUG
    { JET_efvWindows10Rtm, /* 8020 */                   { 1568,20,0 },  { 8,5,11 },  { 0,0,0 } },
#else
    { JET_efvWindows10Rtm, /* 8020 */                   { 1568,20,0 },  { 8,4,3 },   { 0,0,0 } },
#endif


    // Conundrum:  I (SOMEONE) believe that 3,0,0 for the FlushMap version in one or both of these next
    // two lines (JET_efvPersistedLostFlushMapStagedToDebug & maybe JET_efvExchange2016Rtm) is inaccurate
    // and it should be maybe 2,0,0 ... but there is no point in fixing it at this point, because anyone
    // interested in going back this far, may also be interested in upgrading to intermediate versions 
    // that would have thesame bug, so we're going to leave our treatement of the FlushMap the same for
    // these bad versions.  SOMEONE makes a fair argument we perhaps should not support these versions back
    // this far ... as we did not fully implement it.  It may be worth trimming this table up a little,
    // but that will require a bunch of test fixes.
    { JET_efvPersistedLostFlushMapStagedToDebug,        { 1568,20,0 },  { 8,5,13 },  { 3,0,0 } },
    { JET_efvExchange2016Rtm, /* 8520 */                { 1568,20,0 },  { 8,5,14 },  { 3,0,0 } },  //  Final format version for Exchange 2016 RTM.  Actually released summer of 2015.
    { JET_efvWindows10v2Rtm, /* 8620 */                 { 1568,20,0 },  { 8,5,14 },  { 3,0,0 } },  //  Final format version for Windows 10 Version 1511 (Threshold 2).
    { JET_efvSupportNlsInvariantLocale,                 { 1568,20,0 },  { 8,5,14 },  { 3,0,0 } },
    { JET_efvEncryptedColumns,                          { 1568,20,0 },  { 8,5,15 },  { 3,0,0 } },
    { JET_efvLoggingDeleteOfTableFcbs,                  { 1568,20,0 },  { 8,5,16 },  { 3,0,0 } },
    { JET_efvExchange2016Cu1Rtm  /* 8920 */,            { 1568,20,0 },  { 8,5,16 },  { 3,0,0 } },

    { JET_efvSetDbVersion        /* 8940 */,            { 1568,20,20 },  { 8,6,20 },  { 3,0,0 } }, // [2016/03/31]

    { JET_efvExtHdrRootFieldAutoIncStorageReleased,     { 1568,30,40 },  { 8,6,20 },  { 3,0,0 } }, // [2016/03/31]

    { JET_efvXpress9Compression  /* 8980  */,           { 1568,40,60 },  { 8,7,40 },  { 3,0,0} }, // [2016/05/17]

    { JET_efvUppercaseTextNormalization, /* 9000 */     { 1568,50,80 },   { 8,7,40 },   { 3,0,0 } }, // [2016/05/21]
    { JET_efvDbscanHeaderHighestPageSeen, /* 9020 */    { 1568,50,100 },  { 8,7,40 },   { 3,0,0 } }, // [2016/07/11]
    { JET_efvEscrow64, /* 9040 */                       { 1568,60,120 },  { 8,20,60 },  { 3,0,0 } }, // [2017/01/10]
    { JET_efvSynchronousLVCleanup, /* 9060 */           { 1568,60,140 },  { 8,20,60 },  { 3,0,0 } }, // [2017/06/12]
    { JET_efvLid64, /* 9080 */                          { 1568,70,160 },  { 8,20,60 },  { 3,0,0 } }, // [2018/03/04]
    { JET_efvShrinkEof, /* 9100 */                      { 1568,80,180 },  { 8,30,80 },  { 3,0,0 } }, // [2018/05/03]
    { JET_efvLogNewPage, /* 9120 */                     { 1568,90,200 },  { 8,40,100 }, { 3,0,0 } }, // [2019/01/31]
    { JET_efvRootPageMove, /* 9140 */                   { 1568,100,220 }, { 8,50,120 }, { 3,0,0 } }, // [2019/02/13]
    { JET_efvScanCheck2, /* 9160 */                     { 1568,100,220 }, { 8,60,140 }, { 3,0,0 } }, // [2019/03/20]
    { JET_efvLgposLastResize, /* 9180 */                { 1568,110,240 }, { 8,60,140 }, { 3,0,0 } }, // [2019/06/13]
    { JET_efvShelvedPages, /* 9200 */                   { 1568,120,260 }, { 8,60,140 }, { 3,0,0 } }, // [2019/09/12]
    { JET_efvShelvedPagesRevert, /* 9220 */             { 1568,130,280 }, { 8,60,140 }, { 3,0,0 } }, // [2019/09/27]
    { JET_efvShelvedPages2, /* 9240 */                  { 1568,140,300 }, { 8,60,140 }, { 3,0,0 } }, // [2019/09/30]
    { JET_efvLogtimeGenMaxRequired, /* 9260 */          { 1568,150,320 }, { 8,60,140 }, { 3,0,0 } }, // [2019/10/04]
    { JET_efvVariableDbHdrSignatureSnapshot, /* 9280 */ { 1568,160,340 }, { 8,60,140 }, { 3,0,0 } }, // [2019/11/27]
    { JET_efvLowerMinReqLogGenOnRedo, /* 9300 */        { 1568,160,360 }, { 8,60,140 }, { 3,0,0 } }, // [2019/12/14]
    { JET_efvDbTimeShrink, /* 9320 */                   { 1568,160,360 }, { 8,70,160 }, { 3,0,0 } }, // [2019/12/18]
    { JET_efvXpress10Compression, /* 9340 */            { 1568,170,380 }, { 8,80,180 }, { 3,0,0 } }, // [2020/05/05]
    { JET_efvRevertSnapshot, /* 9360 */                 { 1568,180,400 }, { 8,90,200 }, { 3,0,0 } }, // [2020/09/23]
    { JET_efvApplyRevertSnapshot, /* 9380 */            { 1568,190,420 }, { 8,90,200 }, { 3,0,0 } }, // [2020/12/01]
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

//  Like a safety school, this is a fallback version that can be used for default values, but 
//  hopefully would not be relied on.  Sort of something not too far back, but shouldn't break
//  anything if we use it.
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

//  Note: pinstStaging will change the behavior and potentially (depending upon staging parameter) give back a reduced 
//  set of FormatVersions from before EFV-implementation.  This should be NULL if you're checking/retrieving an actual 
//  feature format value, but true for looking up the user param value.
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

    // Probably be more efficient to reverse the order right?  Maybe.
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
    //  Even though it may be within the range of the table, if there is no exact match we'll return JET_errEngineFormatVersionNoLongerSupportedTooLow,
    //  as it may have at one time been supported.
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

        if ( // we allow such minor upgrades to proceed (so allow only UpdateMinor mismatch to ... match).
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
        //  This hits if we replay from very old log files.
        AssertSz( fFalse, "Asked for %d.%d.%d log version, we don't have an entry in g_rgfmtversEngine for this!", lgvFromLogHeader.ulLGVersionMajor, lgvFromLogHeader.ulLGVersionUpdateMajor, lgvFromLogHeader.ulLGVersionUpdateMinor );
        return ErrERRCheck( JET_errBadLogVersion );
    }
    return JET_errSuccess;
}

//  create a formatted string at end of a given buffer

void __cdecl OSStrCbAppendFormatW ( __inout_bcount(cbBuffer) PWSTR wszBuffer, size_t cbBuffer, __format_string PCWSTR cwszFormat, ... )
{
    va_list alist;
    va_start( alist, cwszFormat );
    const size_t cbUsed = wcslen( wszBuffer ) * sizeof(WCHAR);
    Assert( cbUsed < cbBuffer );    // did someone pass in an unit buffer?
    Assert( ( cbUsed % sizeof(WCHAR) ) == 0 );
    HRESULT hr = StringCbVPrintfW( wszBuffer + ( cbUsed / sizeof(WCHAR) ), cbBuffer - cbUsed, cwszFormat, alist );
#ifdef DEBUG
    CallS( ErrFromStrsafeHr( hr ) );
#endif
    va_end( alist );
}


//  A nice formatting of the JET_paramEngineFormatValue setting ... the problem is we have an enum possibly combined with a
//  flag and so it is hard to translate / easily lookup if we just print it either as decimal or hex (because the EFV values 
//  are not in hex).
void FormatEfvSetting( const JET_ENGINEFORMATVERSION efvFullParam, _Out_writes_bytes_(cbEfvSetting) WCHAR * const wszEfvSetting, _In_range_( 240, 240 /* cchFormatEfvSetting * 2 */ ) const ULONG cbEfvSetting )
{
    Assert( cbEfvSetting >= ( cchFormatEfvSetting * sizeof(WCHAR) ) );
    Assert( wszEfvSetting );

    wszEfvSetting[0];

    //  Strip the flags one by one ...
    JET_ENGINEFORMATVERSION efvBaseValue = efvFullParam;
    //  Note we have a "flag indicator" 0x40000000 to indicate it's got a flag of some sort ... so when stripping flags have to
    //  add this back in case we get a value with two flags, e.g. JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat.
    //  See the values in jethdr.w for details.
    const JET_ENGINEFORMATVERSION efvSignalBit = 0x40000000;
    //  First strip the combinable flag
    const BOOL fJET_efvAllowHigherPersistedFormat = !!( ( efvBaseValue & JET_efvAllowHigherPersistedFormat ) == JET_efvAllowHigherPersistedFormat );
    if ( fJET_efvAllowHigherPersistedFormat )
    {
        efvBaseValue = ( efvBaseValue & ~JET_efvAllowHigherPersistedFormat ) | efvSignalBit;
    }
    
    //  First strip the "flagged" special enum EFV values ...
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
        //  Now finally strip the efvSignalBit signal bit.
        efvBaseValue = efvBaseValue & ~efvSignalBit;
    }

    //  Remainder in efvBaseValue should be an explicit EFV value such as 8940.

    Assert( efvBaseValue < 0x00FFFFFF );  //    This keeps the combinable flags and EFV values separate (adjust this mask if EFVs grow > 24 M, or more than 6 combinable flags are added).

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
    CHECK( 0 == wcscmp( L"0x22D8 (8920)", WszFormatEfvSetting( JET_efvExchange2016Cu1Rtm, wszTestBuffer, sizeof(wszTestBuffer) ) ) );   // shouldn't matter if we use constant name.
    CHECK( 0 == wcscmp( L"0x410022D8 (8920 | JET_efvAllowHigherPersistedFormat)", WszFormatEfvSetting( 8920 | JET_efvAllowHigherPersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x40000001 (JET_efvUseEngineDefault)", WszFormatEfvSetting( JET_efvUseEngineDefault, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x40000002 (JET_efvUsePersistedFormat)", WszFormatEfvSetting( JET_efvUsePersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );

    //      All the remaining cases are actually invalid param values, but I would want it to ensure it prints all the data out in case it getsinto production.
    CHECK( 0 == wcscmp( L"0x41000001 (JET_efvUseEngineDefault | JET_efvAllowHigherPersistedFormat)", WszFormatEfvSetting( JET_efvUseEngineDefault | JET_efvAllowHigherPersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x41000002 (JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat)", WszFormatEfvSetting( JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x40000003 (JET_efvUseEngineDefault | JET_efvUsePersistedFormat)", WszFormatEfvSetting( JET_efvUseEngineDefault | JET_efvUsePersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x41000003 (JET_efvUseEngineDefault | JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat)", WszFormatEfvSetting( JET_efvUsePersistedFormat | JET_efvUseEngineDefault | JET_efvAllowHigherPersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
    CHECK( 0 == wcscmp( L"0x41000103 (256 | JET_efvUseEngineDefault | JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat)", WszFormatEfvSetting( 0x100 | JET_efvUseEngineDefault | JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );

    //      Worst case all flags plus largest EFV value function will accept.
    CHECK( 0 == wcscmp( L"0x417F0103 (8323328 | JET_efvUseEngineDefault | JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat)", WszFormatEfvSetting( 0x7F0100 | JET_efvUseEngineDefault | JET_efvUsePersistedFormat | JET_efvAllowHigherPersistedFormat, wszTestBuffer, sizeof(wszTestBuffer) ) ) );
}

#endif // ENABLE_JET_UNIT_TEST

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
                pfmtversMatchHigh = &g_rgfmtversEngine[ i ];  // the first one we hit will be the highest.
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

    //  Ok, we have 3 versions to reconcile and report one number ...

    //  First we just factor in the log, to adjust the high match.

    if ( fLogged && 
         pfmtversMatchLow != NULL && pfmtversMatchHigh != NULL && pfmtversMatchLogHigh != NULL &&
         pfmtversMatchLogHigh->efv >= pfmtversMatchLow->efv &&
         pfmtversMatchLogHigh->efv < pfmtversMatchHigh->efv )
    {
        pfmtversMatchHigh = pfmtversMatchLogHigh;
    }

    if ( pfmtversMatchHigh == NULL )
    {
        //  We could not find any matching value to the DBFILEHDR's version!

        const INT icmpDbAboveTable = CmpDbVer( dbvFromFileHeader, g_fmtversEngineMax.dbv );

        AssertTrack( icmpDbAboveTable != 0, "BeBestMapShouldHaveMatchedDbv" );
        if ( icmpDbAboveTable < 0 )
        {
            //FireWall( "NoDbvFoundForDbfilehdrBelowTableMin" );
            efvParam = fLogged ? 20 : 30;  //  was = 10
        }
        else
        {
            //FireWall( "NoDbvFoundForDbfilehdrAboveTableMax" );
            efvParam = fLogged ? 21 : 31;  //  was = 10
        }
    }
    else if ( pfmtversMatchLogHigh == NULL )
    {
        //  We could not find any matching value to the LGFILEHDR's version!

        const INT icmpLogAboveTable = CmpLgVer( lgvFromFileHeader, g_fmtversEngineMax.lgv );

        AssertTrack( icmpLogAboveTable != 0, "BeBestMapShouldHaveMatchedDbv" );
        if ( icmpLogAboveTable < 0 )
        {
            //FireWall( "NoLgvFoundForLgfilehdrBelowTableMin" );
            efvParam = fLogged ? 22 : 32;  //  was = 11
        }
        else
        {
            //FireWall( "NoLgvFoundForLgfilehdrAboveTableMax" );
            efvParam = fLogged ? 27 : 37;  //  was = 11
        }
    }
    else if ( pfmtversMatchLogLow->efv > pfmtversMatchHigh->efv )
    {
        //  We found a LGFILEHDR version exceeding that of the possible DBFILEHDR's versions!
        //  Note: The way we have it above, a non-logged DB will come through here, so not really impossible.
        //FireWall( "LgvHigherThanPossibleDbv" );
        efvParam = fLogged ? 16 : 36; // was = 12; and based incorrectly on if ( pfmtversMatchLogHigh->efv > pfmtversMatchHigh->efv )...
    }
    else if ( pfmtversMatchLogHigh->efv < pfmtversMatchLow->efv )
    {
        //  We found a LGFILEHDR version lower that of the possible DBFILEHDR's versions!
        //FireWall( "LgvLowerThanPossibleDbv" );
        efvParam = fLogged ? 13 : 33;
    }
    else if ( pfmtversMatchHigh->efv == efvValue )
    {
        //  This is the best case, very strong match!
        //    (Exchange Store should be typically coming through here)
        //  Note: efvParam is already set correctly.
        AssertRTL( efvParam == ( efvValue | ( efvParam & JET_efvAllowHigherPersistedFormat ) ) );
        AssertRTL( ( efvValue & JET_efvAllowHigherPersistedFormat ) != JET_efvAllowHigherPersistedFormat );
        efvParam = efvValue;  //  with NO | JET_efvAllowHigherPersistedFormat
    }
    else if ( efvParam == JET_efvUseEngineDefault ||
              efvParam == JET_efvUsePersistedFormat )
    {
        //  These are virtual format specifiers, we'll use the highest value we read off files.
        efvParam = pfmtversMatchHigh->efv;
    }
    else if ( pfmtversMatchLow->efv <= efvValue &&
              efvValue <= pfmtversMatchHigh->efv )
    {
        //  The value we found is rationally set by this EFV.
        //  This happens in the first 2 entries in this range b/c there are no version differences.
        //    { JET_efvExchange2016Rtm, /* 8520 */                { 1568,20,0 },  { 8,5,14 },  { 3,0,0 } },  //  Final format version for Exchange 2016 RTM.  Actually released summer of 2015.
        //    { JET_efvWindows10v2Rtm, /* 8620 */                 { 1568,20,0 },  { 8,5,14 },  { 3,0,0 } },  //  Final format version for Windows 10 Version 1511 (Threshold 2).
        //    { JET_efvSupportNlsInvariantLocale /* 8880 */,      { 1568,20,0 },  { 8,5,14 },  { 3,0,0 } },
        Assert( efvValue != pfmtversMatchHigh->efv );  //  should be handled couple clauses above.
        efvParam = efvValue | JET_efvAllowHigherPersistedFormat;        
    }
    else
    {
        Assert( efvValue < pfmtversMatchLow->efv ||
                efvValue > pfmtversMatchHigh->efv );
        if ( efvValue < pfmtversMatchLow->efv )
        {
            //  Note: This could happen if Store / cluster downsizes the EFV it likes (post a DB upgrade).
            //FireWall( "EfvParamLowerThanPossibleDbv" );
            efvParam = fLogged ? 14 : 34;
        }
        else
        {
            //FireWall( "EfvParamHigherThanPossibleDbvMissedUpgrade" );
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

    //  Check our EFV setting value maps to something rational for the DBV and LGV version.
    
    const JET_ENGINEFORMATVERSION efvParam = EfvBeBestMapping( (JET_ENGINEFORMATVERSION)UlParam( pinst, JET_paramEngineFormatVersion ),
                                                               fLogged,
                                                               dbvFromFileHeader,
                                                               lgvFromFileHeader );

    //  Ok, now format everything ... 
    
    OSStrCbFormatW( wszSetting, cbSetting, L"%d.%d.%d (%u)", dbvFromFileHeader.ulDbMajorVersion, dbvFromFileHeader.ulDbUpdateMajor, dbvFromFileHeader.ulDbUpdateMinor, (ULONG)efvParam );
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( SysVerEngineFormatVersion, CheckBestMappingWorksForAllExistingEfvs )
{
    wprintf( L"\n" );
    for( ULONG iefv = 0; iefv < _countof( g_rgfmtversEngine ); iefv++ )
    {
        //  If we're on a given EFV and assume the DB and Log were upgraded appropriately (by passing
        //  those .dbv and .lgv values), the mapping shouldreturn the same EFV we gave it.
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
    const ULONG iefv = 17; // pick JET_efvShrinkEof because it has version changes below.
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100 /* JET_efvShrinkEof */ );  //  in case someone inserts entries above this entry

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
    const ULONG iefv = 17; // pick JET_efvShrinkEof because it has version changes above.
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100 /* JET_efvShrinkEof */ );  //  in case someone inserts entries above this entry

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
    const ULONG iefv = 17; // pick JET_efvShrinkEof because it has version changes below.
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100 /* JET_efvShrinkEof */ );  //  in case someone inserts entries above this entry

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
    const ULONG iefv = 17; // pick JET_efvShrinkEof because it has version changes above.
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100 /* JET_efvShrinkEof */ );  //  in case someone inserts entries above this entry

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
    const ULONG iefv = 17; // pick JET_efvShrinkEof because it has version changes below.
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100 /* JET_efvShrinkEof */ );  //  in case someone inserts entries above this entry

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
    const ULONG iefv = 17; // pick JET_efvShrinkEof because it has version changes above.
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100 /* JET_efvShrinkEof */ );  //  in case someone inserts entries above this entry

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
    const ULONG iefv = 17; // pick JET_efvShrinkEof because it has version changes below.
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100 /* JET_efvShrinkEof */ );  //  in case someone inserts entries above this entry

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
    const ULONG iefv = 17; // pick JET_efvShrinkEof because it has version changes above.
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100 /* JET_efvShrinkEof */ );  //  in case someone inserts entries above this entry

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
    const ULONG iefv = 17; // pick JET_efvShrinkEof because it has version changes below.
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100 /* JET_efvShrinkEof */ );  //  in case someone inserts entries above this entry

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
    const ULONG iefv = 17; // pick JET_efvShrinkEof because it has version changes above.
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100 /* JET_efvShrinkEof */ );  //  in case someone inserts entries above this entry

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
    const ULONG iefv = 17; // pick JET_efvShrinkEof because it has version changes below.
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100 /* JET_efvShrinkEof */ );  //  in case someone inserts entries above this entry

    efvMapped = EfvBeBestMapping( 5000 | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 14 == efvMapped ); // no difference between Low and WayLow
    efvMapped = EfvBeBestMapping( 5000 | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 34 == efvMapped ); // no difference between Low and WayLow
}

JETUNITTEST( SysVerEngineFormatVersion, CheckWayHighEfv )
{
    wprintf( L"\n" );
    JET_ENGINEFORMATVERSION efvMapped = 1;
    const ULONG iefv = 17; // pick JET_efvShrinkEof because it has version changes above.
    CHECK( g_rgfmtversEngine[ iefv ].efv == 9100 /* JET_efvShrinkEof */ );  //  in case someone inserts entries above this entry

    efvMapped = EfvBeBestMapping( 65000 | JET_efvAllowHigherPersistedFormat,
                                  fTrue,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 15 == efvMapped ); // no difference between High and WayHigh
    efvMapped = EfvBeBestMapping( 65000 | JET_efvAllowHigherPersistedFormat,
                                  fFalse,
                                  g_rgfmtversEngine[ iefv ].dbv,
                                  g_rgfmtversEngine[ iefv ].lgv );
    wprintf( L"   g_rgfmtversEngine[ %d ].efv = %d (0x%x) -(mapped)-> %d (0x%x)\n", iefv, g_rgfmtversEngine[ iefv ].efv, g_rgfmtversEngine[ iefv ].efv, efvMapped, efvMapped );
    CHECK( 35 == efvMapped ); // no difference between High and WayHigh
}

#endif // ENABLE_JET_UNIT_TEST

