// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"
#include "esestd.hxx"

#ifndef ESENT
#include <NativeWatson.h>
#endif


#ifdef DEBUG
BOOL g_fOSUInit = fFalse;
#endif



void OSUTerm_();

extern BOOL g_fDisablePerfmon;
static VOID OSUTermProcessorLocalStorage();


ULONG cPerfinstReserved = 0;
ULONG cPerfinstCommitted = 0;

static ERR ErrOSUInitProcessorLocalStorage()
{
    extern ULONG g_cpinstMax;
    extern LONG g_cbPlsMemRequiredPerPerfInstance;
    ULONG cPerfinstMaxT = 0;
    ULONG cPerfinstMinReqT = 0;

    if ( !g_fDisablePerfmon )
    {

        if ( runInstModeNoSet == g_runInstMode )
        {
            const ULONG cpinstMaxT = (ULONG)UlParam( JET_paramMaxInstances );
            const ULONG ifmpMaxT = cpinstMaxT * dbidMax + cfmpReserved;
            cPerfinstMaxT = CPerfinstMaxToUse( cpinstMaxT, ifmpMaxT );
        }
        else
        {
            cPerfinstMaxT = cPerfinstMax;
        }

        const ULONG cbPerfCountersPerProc = cPerfinstMaxT * g_cbPlsMemRequiredPerPerfInstance;
        Assert( cbPerfCountersPerProc > 0 );
        if ( !PLS::FAllocatePerfCountersMemory( cbPerfCountersPerProc ) )
        {
            return ErrERRCheck( JET_errOutOfMemory );
        }
    }


    if ( !FOSSyncConfigureProcessorLocalStorage( sizeof( PLS ) ) )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }


    const size_t cProcs = (size_t)OSSyncGetProcessorCountMax();
    for ( size_t iProc = 0; iProc < cProcs; iProc++ )
    {
        PLS* const ppls = (PLS*)OSSyncGetProcessorLocalStorage( iProc );

        new( ppls ) PLS( (ULONG)iProc, g_fDisablePerfmon );
    }


    if ( !g_fDisablePerfmon )
    {
        cPerfinstMinReqT = CPERFObjectsMinReq();
        if ( !PLS::FEnsurePerfCounterBuffer( cPerfinstMinReqT * g_cbPlsMemRequiredPerPerfInstance ) )
        {
            OSUTermProcessorLocalStorage();
            OnDebug( const BOOL fFreePls = )FOSSyncConfigureProcessorLocalStorage( 0 );
            Assert( fFreePls );
            return ErrERRCheck( JET_errOutOfMemory );
        }
    }

    cPerfinstReserved = cPerfinstMaxT;
    cPerfinstCommitted = cPerfinstMinReqT;

    return JET_errSuccess;
}


static VOID OSUTermProcessorLocalStorage()
{
    cPerfinstReserved = 0;
    cPerfinstCommitted = 0;

    const size_t cProcs = (size_t)OSSyncGetProcessorCountMax();
    for ( size_t iProc = 0; iProc < cProcs; iProc++ )
    {
        PLS* const ppls = (PLS*)OSSyncGetProcessorLocalStorage( iProc );

        if ( ppls )
        {
            ppls->~PLS();
        }
    }

    PLS::FreePerfCountersMemory();
}



ERR ErrOSUValidateGlobals()
{
    ERR err = JET_errSuccess;



    if ( !g_fDisablePerfmon && ( ( cPerfinstMax > cPerfinstReserved ) || ( CPERFObjectsMinReq() > cPerfinstCommitted ) ) )
    {
        if ( !FNegTest( fInvalidAPIUsage ) )
        {
            FireWall( "OsuAlreadyInitialized" );
        }
        Error( ErrERRCheck( JET_errAlreadyInitialized ) );
    }

HandleError:
    return err;
}

#define DEBUG_ENV_VALUE_LEN         256



ERR ErrOSULoadOverloads()
{
#ifdef DEBUG
    WCHAR * wsz = NULL;
    if ( ( wsz = GetDebugEnvValue( L"Error Trap" ) ) != NULL )
    {
        (void)ErrERRSetErrTrap( _wtol( wsz ) );
        delete[] wsz;
        wsz = NULL;
    }

    if ( ( wsz = GetDebugEnvValue( L"Time Injection Disabled" ) ) != NULL )
    {
        if ( _wtol( wsz ) )
        {
            OSTimeSetTimeInjection( 0, 1, 0 );
        }
        delete[] wsz;
        wsz = NULL;
    }
#else
#ifndef RTM
    WCHAR wszBuf[ DEBUG_ENV_VALUE_LEN ];
    if ( FOSConfigGet( wszDEBUGRoot, L"Error Trap", wszBuf, sizeof(wszBuf) )
        && 0 != wszBuf[0] )
    {
        (void)ErrERRSetErrTrap( _wtol( wszBuf ) );
    }

    if ( FOSConfigGet( wszDEBUGRoot, L"Time Injection Disabled", wszBuf, sizeof(wszBuf) )
        && 0 != wszBuf[0] )
    {
        if ( _wtol( wszBuf ) )
        {
            OSTimeSetTimeInjection( 0, 1, 0 );
        }
    }
#endif
#endif

#ifdef DEBUG
    if ( ( wsz = GetDebugEnvValue( L"Fault Injection" ) ) != NULL )
    {
        ULONG ulID;
        ERR err;
        ULONG ulProb;
        DWORD grbit;
        WCHAR * wszNextToken;

        for ( WCHAR* wszT = wcstok_s( wsz, L";", &wszNextToken ); wszT != NULL; wszT = wcstok_s( NULL, L";", &wszNextToken ) )
        {
            if ( swscanf_s( wszT, L"%lu,%ld,%lu,%lu", &ulID, &err, &ulProb, &grbit ) == 4 )
            {
                (void)ErrEnableTestInjection( ulID, (ULONG_PTR)err, JET_TestInjectFault, ulProb, grbit );
            }
        }

        delete[] wsz;
    }

    if ( ( wsz = GetDebugEnvValue( L"Config Override Injection" ) ) != NULL )
    {
        ULONG ulID;
        QWORD ulValue;
        ULONG ulProb;
        DWORD grbit;
        WCHAR * wszNextToken;

        for ( WCHAR* wszT = wcstok_s( wsz, L";", &wszNextToken ); wszT != NULL; wszT = wcstok_s( NULL, L";", &wszNextToken ) )
        {
            if ( swscanf_s( wszT, L"%lu,%I64u,%lu,%lu", &ulID, &ulValue, &ulProb, &grbit ) == 4 )
            {
                (void)ErrEnableTestInjection( ulID, (JET_API_PTR)ulValue, JET_TestInjectConfigOverride, ulProb, grbit );
            }
        }

        delete[] wsz;
    }

    if ( ( wsz = GetDebugEnvValue( L"Test Injection Trap" ) ) != NULL )
    {
        extern ULONG g_ulIDTrap;
        g_ulIDTrap = _wtol( wsz );
        delete[] wsz;
    }

    if ( ( wsz = GetDebugEnvValue( L"Param Trap" ) ) != NULL )
    {
        extern ULONG g_paramidTrapParamSet;
        g_paramidTrapParamSet = _wtol( wsz );
        delete[] wsz;
    }

    if ( ( wsz = GetDebugEnvValue( L"OSPath Trap" ) ) != NULL )
    {
        extern WCHAR g_rgwchTrapOSPath[_MAX_PATH];
        if ( 0 <= ErrOSStrCbCopyW( g_rgwchTrapOSPath+1, sizeof(g_rgwchTrapOSPath)-sizeof(WCHAR), wsz+1 ) )
        {
            g_rgwchTrapOSPath[0] = wsz[0];
        }
        delete[] wsz;
    }

    if ( ( wsz = GetDebugEnvValue( L"Atomic Override" ) ) != NULL )
    {
        extern DWORD g_cbAtomicOverride;
        g_cbAtomicOverride = _wtol( wsz );
        delete[] wsz;
    }

    if ( ( wsz = GetDebugEnvValue( L"Negative Testing" ) ) != NULL )
    {
        (void)FNegTestSet( _wtol( wsz ) );
        delete[] wsz;
    }
#else
#ifndef RTM
    if ( FOSConfigGet( wszDEBUGRoot, L"Negative Testing", wszBuf, sizeof(wszBuf) )
        && 0 != wszBuf[0] )
    {
        (void)FNegTestSet( _wtol( wszBuf ) );
    }
#endif
#endif

    return JET_errSuccess;
}



ERR ErrOSUSetResourceManagerGlobals()
{
    ERR err = JET_errSuccess;

    Assert( UlParam( JET_paramDatabasePageSize ) && FPowerOf2( UlParam( JET_paramDatabasePageSize ) ) );

    DWORD_PTR cbVerPage;
    cbVerPage = max( UlParam( JET_paramVerPageSize ), 2 * (ULONG_PTR)UlParam( JET_paramDatabasePageSize ) );
    cbVerPage = min( cbVerPage, 64 * 1024 );
    cbVerPage = cbVerPage - cbRESHeader;
    Call( ErrRESSetResourceParam( pinstNil, JET_residVERBUCKET, JET_resoperSize, cbVerPage ) );
    Call( ErrRESSetResourceParam( pinstNil, JET_residVERBUCKET, JET_resoperMinUse, UlParam( JET_paramGlobalMinVerPages ) ) );

    Call( RESLRUKHIST.ErrSetParam( JET_resoperMaxUse, UlParam( JET_paramLRUKHistoryMax ) ) );

    for ( JET_RESID residT = JET_RESID( JET_residNull + 1 );
        residT < JET_residMax;
        residT = JET_RESID( residT + 1 ) )
    {
        if ( residT == JET_residPAGE )
        {
            continue;
        }
        CallS( ErrRESSetResourceParam( pinstNil, residT, JET_resoperAllocFromHeap, FJetConfigLowMemory() || FJetConfigMedMemory() ) );
    }

HandleError:

    return err;
}




ERR ErrOSUSetOSLayerGlobals()
{

    OSPrepreinitSetUserTLSSize( sizeof( TLS ) );


    COSLayerPreInit::SetDefaults();

    COSLayerPreInit::SetAssertAction( GrbitParam( JET_paramAssertAction ) );
    COSLayerPreInit::SetExceptionAction( GrbitParam( JET_paramExceptionAction ) );
    COSLayerPreInit::SetCatchExceptionsOnBackgroundThreads( GrbitParam( JET_paramExceptionAction ) != JET_ExceptionNone );
    if ( !FDefaultParam( JET_paramRFS2AllocsPermitted ) )
    {
        COSLayerPreInit::SetRFSAlloc( (ULONG)UlParam( JET_paramRFS2AllocsPermitted ) );
    }
    if ( !FDefaultParam( JET_paramRFS2IOsPermitted ) )
    {
        COSLayerPreInit::SetRFSIO( (ULONG)UlParam( JET_paramRFS2IOsPermitted ) );
    }

    COSLayerPreInit::SetEventLogCache( (ULONG)UlParam( JET_paramEventLogCache ) );

    if ( BoolParam( JET_paramDisablePerfmon ) )
    {
        COSLayerPreInit::DisablePerfmon();
    }
    else
    {
        COSLayerPreInit::EnablePerfmon();
    }

    if ( FJetConfigRunSilent() )
    {
        COSLayerPreInit::DisableTracing();
    }
    COSLayerPreInit::SetRestrictIdleActivity( FJetConfigLowPower() );


    COSLayerPreInit::SetIOMaxOutstanding( (ULONG)UlParam( JET_paramOutstandingIOMax ) );
#ifdef DEBUG
    if ( FDefaultParam( JET_paramOutstandingIOMax ) )
    {
        DWORD cioT = 0;
        switch( rand() % 5 )
        {
            case 0:     cioT = 324;     break;
            case 1:     cioT = 1024;    break;
            case 2:     cioT = 3072;    break;
            case 3:     cioT = 10000;   break;
            case 4:     cioT = 32764;   break;
        }
        COSLayerPreInit::SetIOMaxOutstanding( min( (ULONG)UlParam( JET_paramOutstandingIOMax ), cioT ) );
    }
#endif

    COSLayerPreInit::SetIOMaxOutstandingBackground( (ULONG)UlParam( JET_paramCheckpointIOMax ) );

    COSLayerPreInit::SetZeroExtend( (ULONG)UlParam( JET_paramMaxCoalesceWriteSize ) );

    COSLayerPreInit::SetProcessFriendlyName( SzParam( JET_paramProcessFriendlyName ) );

    extern COSLayerPreInit::PfnThreadWait g_pfnThreadWaitBegin;
    COSLayerPreInit::SetThreadWaitBeginNotification( g_pfnThreadWaitBegin );
    extern COSLayerPreInit::PfnThreadWait g_pfnThreadWaitEnd;
    COSLayerPreInit::SetThreadWaitEndNotification( g_pfnThreadWaitEnd );

    return JET_errSuccess;
}




ERR ErrOSUSetOSULayerGlobals()
{

    ULONG cbMaxWriteSize;
    cbMaxWriteSize = (ULONG)UlParam( JET_paramMaxCoalesceWriteSize );
    if ( cbMaxWriteSize < OSMemoryPageCommitGranularity() )
    {
        extern ULONG cbLogExtendPatternMin;
        cbMaxWriteSize -= cbMaxWriteSize % cbLogExtendPatternMin;
    }
    else if ( cbMaxWriteSize < OSMemoryPageReserveGranularity() )
    {
        cbMaxWriteSize -= cbMaxWriteSize % OSMemoryPageCommitGranularity();
    }
    else
    {
        cbMaxWriteSize -= cbMaxWriteSize % OSMemoryPageReserveGranularity();
    }

    Assert( (ULONG)UlParam( JET_paramMaxCoalesceWriteSize ) >= cbMaxWriteSize );

    cbLogExtendPattern = cbMaxWriteSize;

    return JET_errSuccess;
}



ERR ErrOSUSetGlobals()
{
    if ( !_wcsicmp( WszUtilProcessName(), L"ESEUTIL" ) || !_wcsicmp( WszUtilProcessName(), L"ESENTUTL" ) )
    {
        g_fEseutil = fTrue;
    }

    CallS( ErrOSULoadOverloads() );

    CallS( ErrOSUSetResourceManagerGlobals() );

    CallS( ErrOSUSetOSLayerGlobals() );

    CallS( ErrOSUSetOSULayerGlobals() );

    return JET_errSuccess;
}


extern INT g_cInstances;
extern WCHAR* g_wszInstanceNames;
extern WCHAR* g_wszInstanceNamesOut;
extern ULONG g_cchInstanceNames;
extern BYTE* g_rgbInstanceAggregationIDs;

extern INT g_cDatabases;
extern WCHAR* g_wszDatabaseNames;
extern WCHAR* g_wszDatabaseNamesOut;
extern ULONG g_cchDatabaseNames;
extern BYTE* g_rgbDatabaseAggregationIDs;


ERR ErrOSUInitPerfmonStorage()
{
    ERR err = JET_errSuccess;


    Assert( 0 == g_cchInstanceNames );
    Assert( NULL == g_wszInstanceNames );
    Assert( NULL == g_wszInstanceNamesOut );
    Assert( NULL == g_rgbInstanceAggregationIDs );


    ULONG cpinstMaxT = 0;
    ULONG ifmpMaxT = 0;
    if ( runInstModeNoSet == g_runInstMode )
    {
        cpinstMaxT = (ULONG)UlParam( JET_paramMaxInstances );
        ifmpMaxT = cpinstMaxT * dbidMax + cfmpReserved;
    }
    else
    {
        cpinstMaxT = g_cpinstMax;
        ifmpMaxT = g_ifmpMax;
    }
    const ULONG cinstPerfInstMax = cpinstMaxT + 1;

    if ( cinstPerfInstMax > ( cMaxInstances + 1 ) )
    {
        AssertTrack( fFalse, "InconsistentMaxIntCountPerfmon" );
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    if ( g_fDisablePerfmon )
    {
        g_cchInstanceNames = 0;
        g_wszInstanceNames = NULL;
        g_wszInstanceNamesOut = NULL;
        g_rgbInstanceAggregationIDs = NULL;
    }
    else
    {
        g_cchInstanceNames = cinstPerfInstMax * ( cchPerfmonInstanceNameMax + 1 ) + 1;
        if ( g_cchInstanceNames > ( ulMax / ( 2 * sizeof( WCHAR ) ) ) )
        {
            Assert( fFalse );
            g_cchInstanceNames = 0;
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }
        Alloc( g_wszInstanceNames = new WCHAR[ g_cchInstanceNames ] );
        Alloc( g_wszInstanceNamesOut = new WCHAR[ g_cchInstanceNames ] );
        Alloc( g_rgbInstanceAggregationIDs = new BYTE[ cinstPerfInstMax ] );
        memset( g_wszInstanceNames, 0, sizeof( WCHAR ) * g_cchInstanceNames );
        memset( g_wszInstanceNamesOut, 0, sizeof( WCHAR ) * g_cchInstanceNames );
        memset( g_rgbInstanceAggregationIDs, 0, cinstPerfInstMax );
    }
    g_cInstances = 0;


    Assert( 0 == g_cchDatabaseNames );
    Assert( NULL == g_wszDatabaseNames );
    Assert( NULL == g_wszDatabaseNamesOut );
    Assert( NULL == g_rgbDatabaseAggregationIDs );

    if ( ifmpMaxT > ( cMaxInstances * dbidMax + cfmpReserved + 1 ) )
    {
        AssertTrack( fFalse, "InconsistentMaxDbCountPerfmon" );
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    if ( g_fDisablePerfmon )
    {
        g_cchDatabaseNames = 0;
        g_wszDatabaseNames = NULL;
        g_wszDatabaseNamesOut = NULL;
        g_rgbDatabaseAggregationIDs = NULL;
    }
    else
    {
        g_cchDatabaseNames = ifmpMaxT * ( cchPerfmonInstanceNameMax + 1 ) + 1;
        if ( g_cchDatabaseNames > ( ulMax / ( 2 * sizeof( WCHAR ) ) ) )
        {
            Assert( fFalse );
            g_cchDatabaseNames = 0;
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }
        Alloc( g_wszDatabaseNames = new WCHAR[ g_cchDatabaseNames ] );
        Alloc( g_wszDatabaseNamesOut = new WCHAR[ g_cchDatabaseNames ] );
        Alloc( g_rgbDatabaseAggregationIDs = new BYTE[ ifmpMaxT ] );
        memset( g_wszDatabaseNames, 0, sizeof( WCHAR ) * g_cchDatabaseNames );
        memset( g_wszDatabaseNamesOut, 0, sizeof( WCHAR ) * g_cchDatabaseNames );
        memset( g_rgbDatabaseAggregationIDs, 0, ifmpMaxT );
    }
    g_cDatabases = 0;

HandleError:
    if ( err < JET_errSuccess )
    {
        g_cchInstanceNames = 0;
        delete[] g_wszInstanceNames;
        g_wszInstanceNames = NULL;
        delete[] g_wszInstanceNamesOut;
        g_wszInstanceNamesOut = NULL;
        delete[] g_rgbInstanceAggregationIDs;
        g_rgbInstanceAggregationIDs = NULL;

        g_cchDatabaseNames = 0;
        delete[] g_wszDatabaseNames;
        g_wszDatabaseNames = NULL;
        delete[] g_wszDatabaseNamesOut;
        g_wszDatabaseNamesOut = NULL;
        delete[] g_rgbDatabaseAggregationIDs;
        g_rgbDatabaseAggregationIDs = NULL;
    }

    return err;
}


void OSUTermPerfmonStorage()
{
    delete[] g_wszInstanceNames;
    delete[] g_wszInstanceNamesOut;
    g_wszInstanceNames = NULL;
    g_wszInstanceNamesOut = NULL;
    g_cchInstanceNames = 0;
    delete[] g_rgbInstanceAggregationIDs;
    g_rgbInstanceAggregationIDs = NULL;
    g_cInstances = 0;

    delete[] g_wszDatabaseNames;
    delete[] g_wszDatabaseNamesOut;
    g_wszDatabaseNames = NULL;
    g_wszDatabaseNamesOut = NULL;
    g_cchDatabaseNames = 0;
    delete[] g_rgbDatabaseAggregationIDs;
    g_rgbDatabaseAggregationIDs = NULL;
    g_cDatabases = 0;
}



CInitTermLock   g_OSUInitControl;

const ERR ErrOSUInit()
{
    ERR err;

    CInitTermLock::ERR errInit = g_OSUInitControl.ErrInitBegin();
    if ( CInitTermLock::ERR::errSuccess == errInit )
    {
        Assert( g_fOSUInit );
        Assert( g_OSUInitControl.CConsumers() > 0 );


        Call( ErrOSUValidateGlobals() );

        return JET_errSuccess;
    }
    Assert( CInitTermLock::ERR::errInitBegun == errInit );

#ifndef ESENT
    RegisterWatson();
#endif


    Call( ErrOSUSetGlobals() );


    Call( ErrOSUInitPerfmonStorage() );


    Call( ErrOSUInitProcessorLocalStorage() );


    Call( ErrOSInit() );


    Call( ErrOSUTimeInit() );
    Call( ErrOSUConfigInit() );
    Call( ErrOSUEventInit() );
    Call( ErrOSUSyncInit() );
    Call( ErrOSUFileInit() );
    Call( ErrOSUNormInit() );

    OSSyncConfigDeadlockTimeoutDetection( !FNegTest( fDisableTimeoutDeadlockDetection ) );

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlOsuInitDone, NULL );


    CallS( ErrOSUValidateGlobals() );


    OnDebug( g_fOSUInit = fTrue );

    g_OSUInitControl.InitFinish( CInitTermLock::fSuccessfulInit );

    return JET_errSuccess;

HandleError:

    Assert( err < JET_errSuccess );
    Assert( !g_fOSUInit );

    OSUTerm_();
    
    g_OSUInitControl.InitFinish( CInitTermLock::fFailedInit );

    return err;
}



void OSUTerm_()
{

    Assert( !g_fOSUInit );


    OSUNormTerm();
    OSUFileTerm();
    OSUSyncTerm();
    OSUEventTerm();
    OSUConfigTerm();
    OSUTimeTerm();


    OSTerm();


    OSUTermProcessorLocalStorage();


    OSUTermPerfmonStorage();

#ifndef ESENT
    UnregisterWatson();
#endif
}



void OSUTerm()
{
    Assert( g_fOSUInit );

    CInitTermLock::ERR errTerm = g_OSUInitControl.ErrTermBegin();
    if ( CInitTermLock::ERR::errSuccess == errTerm )
    {
        return;
    }
    Assert( CInitTermLock::ERR::errTermBegun == errTerm );

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlOsuTermStart, NULL );

    OnDebug( g_fOSUInit = fFalse );


    OSUTerm_();


    Assert( !g_fOSUInit );

    g_OSUInitControl.TermFinish();

}


