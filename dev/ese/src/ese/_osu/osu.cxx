// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"
#include "esestd.hxx"

#ifdef USE_WATSON_API
#include <NativeWatson.h>
#endif

//  globals

#ifdef DEBUG
BOOL g_fOSUInit = fFalse;
#endif


//  forward decls

void OSUTerm_();

extern BOOL g_fDisablePerfmon;
static VOID OSUTermProcessorLocalStorage();

//  allocate PLS (Processor Local Storage)

ULONG cPerfinstReserved = 0;
ULONG cPerfinstCommitted = 0;

static ERR ErrOSUInitProcessorLocalStorage()
{
    extern ULONG g_cpinstMax;                       //  set before init is called
    extern LONG g_cbPlsMemRequiredPerPerfInstance;  //  incremented in static constructors
    ULONG cPerfinstMaxT = 0;
    ULONG cPerfinstMinReqT = 0;

    if ( !g_fDisablePerfmon )
    {
        // The PLS perf counter buffer is used to store ESE-instance, Database and TableClasses performance counters,
        // so make sure we allocate space to cover the bigger of the three.

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

    //  initialize PLS

    if ( !FOSSyncConfigureProcessorLocalStorage( sizeof( PLS ) ) )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    //  FOSSyncPreinit counted the processors when the DLL was attached

    const size_t cProcs = (size_t)OSSyncGetProcessorCountMax();
    for ( size_t iProc = 0; iProc < cProcs; iProc++ )
    {
        PLS* const ppls = (PLS*)OSSyncGetProcessorLocalStorage( iProc );

        new( ppls ) PLS( (ULONG)iProc, g_fDisablePerfmon );
    }

    //  Make sure we have the bare minimum for global and default perf counters.

    if ( !g_fDisablePerfmon )
    {
        cPerfinstMinReqT = CPERFObjectsMinReq();
        if ( !PLS::FEnsurePerfCounterBuffer( cPerfinstMinReqT * g_cbPlsMemRequiredPerPerfInstance ) )
        {
            OSUTermProcessorLocalStorage();
            return ErrERRCheck( JET_errOutOfMemory );
        }
    }

    cPerfinstReserved = cPerfinstMaxT;
    cPerfinstCommitted = cPerfinstMinReqT;

    return JET_errSuccess;
}

//  free PLS (Processor Local Storage)

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
    OnDebug( const BOOL fFreePls = )FOSSyncConfigureProcessorLocalStorage( 0 );
    Assert( fFreePls );

    PLS::FreePerfCountersMemory();
}


//  Validates Globals are set to sane values.

ERR ErrOSUValidateGlobals()
{
    ERR err = JET_errSuccess;


    // Once initialized, make sure that cPerfinstMax does not shrink from underneath us
    // Otherwise we may end up overrunning the perf counter memory we allocated based on the
    // old cPerfinstMax

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

//  This routine loads traps, faults (injection), and overrides ...

//  Note: NO EQUIV TERM function, must not allocate data.

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
    // The format is "ID, err, ulProb, grbit" so ... "17396,-1011,20,1" is 20% failure for 
    // LID 17396 ... and the "1" at end indicates 20 is interprited as percentage.  See 
    // jethdr.w:JET_bitInjection for more details about last param.  -1011 is error to return.
    // Note this is fairly early, but not early enough to fault inject all things.
    // Multiple fault injections can be specified by a ';'.
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

    // The format is "ID, value, ulProb, grbit" so ... "41007,500,100,1" means that
    // LID 41007 will return a value of 500 100% of the time.
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
            g_rgwchTrapOSPath[0] = wsz[0]; // "atomically" update the trap.
        }
        delete[] wsz;
    }

    if ( ( wsz = GetDebugEnvValue( L"Atomic Override" ) ) != NULL )
    {
        extern DWORD g_cbAtomicOverride;
        g_cbAtomicOverride = _wtol( wsz );
        delete[] wsz;
    }

    //  set this regkey to silence some asserts which
    //  go off in some error code paths, because the
    //  test is explicitly trying to generate errors
    //
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
#endif  // DEBUG

    return JET_errSuccess;
}

//  sets the parameters for the CResource Manager to use

//  Note: NO EQUIV TERM function, must not allocate data.

ERR ErrOSUSetResourceManagerGlobals()
{
    ERR err = JET_errSuccess;

    //  configure our global page parameters
    //
    Assert( UlParam( JET_paramDatabasePageSize ) && FPowerOf2( UlParam( JET_paramDatabasePageSize ) ) );

    //  configure our global version bucket parameters
    //
    //  NOTE:  we also ensure that the version bucket is large enough to hold
    //         a record that fills an entire database page
    //  NOTE:  we also subtract the size of a CResourceSection from the size of
    //         the bucket to facilitate efficient use of resource memory
    //  NOTE:  we cannot allow JET_residVERBUCKET to exceed 64kb due to a bug
    //         in cresmgr
    //
    DWORD_PTR cbVerPage;
    cbVerPage = max( UlParam( JET_paramVerPageSize ), 2 * (ULONG_PTR)UlParam( JET_paramDatabasePageSize ) );
    cbVerPage = min( cbVerPage, 64 * 1024 );
    cbVerPage = cbVerPage - cbRESHeader;
    Call( ErrRESSetResourceParam( pinstNil, JET_residVERBUCKET, JET_resoperSize, cbVerPage ) );
    Call( ErrRESSetResourceParam( pinstNil, JET_residVERBUCKET, JET_resoperMinUse, UlParam( JET_paramGlobalMinVerPages ) ) );

    //  configure our global LRUK parameters
    //
    Call( RESLRUKHIST.ErrSetParam( JET_resoperMaxUse, UlParam( JET_paramLRUKHistoryMax ) ) );

    //  if we are running in a small configuration, bypass the resource manager
    //  and go directly to the heap for all resource allocations
    //
    for ( JET_RESID residT = JET_RESID( JET_residNull + 1 );
        residT < JET_residMax;
        residT = JET_RESID( residT + 1 ) )
    {
        if ( residT == JET_residPAGE )
        {
            //  I can't easily remove the JET_residPAGE b/c someone put the resid enums in 
            //  the jethdr.w / API. :(  But we don't need / use this one anymore, so we'll
            //  skip it ...
            continue;
        }
        CallS( ErrRESSetResourceParam( pinstNil, residT, JET_resoperAllocFromHeap, FJetConfigLowMemory() || FJetConfigMedMemory() ) );
    }

HandleError:

    return err;
}

//  sets globals / constants that the OS Layer consumes

//  Note: NO EQUIV TERM function, must not allocate data.


ERR ErrOSUSetOSLayerGlobals()
{
    //  init the settings for the OS Layer  from the JET System Parameters
    //

    OSPrepreinitSetUserTLSSize( sizeof( TLS ) );

    //  first reset the defaults in case it is needed

    COSLayerPreInit::SetDefaults();

    //  set up debug
    //
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

    //  init event log cache size
    //
    COSLayerPreInit::SetEventLogCache( (ULONG)UlParam( JET_paramEventLogCache ) );

    //  perfmon
    //
    if ( BoolParam( JET_paramDisablePerfmon ) )
    {
        COSLayerPreInit::DisablePerfmon();
    }
    else
    {
        COSLayerPreInit::EnablePerfmon();
    }

    //  special configuration parameters
    //
    if ( FJetConfigRunSilent() )
    {
        COSLayerPreInit::DisableTracing();
    }
    COSLayerPreInit::SetRestrictIdleActivity( FJetConfigLowPower() );

    //  set file settings
    //

    COSLayerPreInit::SetIOMaxOutstanding( (ULONG)UlParam( JET_paramOutstandingIOMax ) );
#ifdef DEBUG
    //  if default param, mix it up a bit ... though 256 isn't really mixing it up that much ... AND
    //  the last problem we had with this param was setting it up, NOT down, so adding that ...
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
#endif // DEBUG

    COSLayerPreInit::SetIOMaxOutstandingBackground( (ULONG)UlParam( JET_paramCheckpointIOMax ) );

    COSLayerPreInit::SetZeroExtend( (ULONG)UlParam( JET_paramMaxCoalesceWriteSize ) );

    COSLayerPreInit::SetProcessFriendlyName( SzParam( JET_paramProcessFriendlyName ) );

    extern COSLayerPreInit::PfnThreadWait g_pfnThreadWaitBegin;
    COSLayerPreInit::SetThreadWaitBeginNotification( g_pfnThreadWaitBegin );
    extern COSLayerPreInit::PfnThreadWait g_pfnThreadWaitEnd;
    COSLayerPreInit::SetThreadWaitEndNotification( g_pfnThreadWaitEnd );

    return JET_errSuccess;
}


//  sets globals / constants that the OSU Layer consumes

//  Note: NO EQUIV TERM function, must not allocate data.

ERR ErrOSUSetOSULayerGlobals()
{

    //  OSU config
    //
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

//  sets globals for OSU system 

//  Note: this func is idempotent (i.e. calling twice doesn't leak 
//  anything, just sets the globals again), and as such has no equivalent
//  "term" func.

ERR ErrOSUSetGlobals()
{
    //  detect if we are being invoked by our utilities
    //
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

//  Perfmon config storage

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

//  Initialize the perfmon storage

// Note: This was moved here, not because I could prove it had to be done 
// before ErrOSUInit/ErrOSInit(), but because I couldn't prove it did not
// need to be done before init.
ERR ErrOSUInitPerfmonStorage()
{
    ERR err = JET_errSuccess;

    //  Init the array for the ESE instance names.

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

    //  Init the array for the database names.

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

//  Terminate the perfmon storage

void OSUTermPerfmonStorage()
{
    //  free ESE instance names
    delete[] g_wszInstanceNames;
    delete[] g_wszInstanceNamesOut;
    g_wszInstanceNames = NULL;
    g_wszInstanceNamesOut = NULL;
    g_cchInstanceNames = 0;
    delete[] g_rgbInstanceAggregationIDs;
    g_rgbInstanceAggregationIDs = NULL;
    g_cInstances = 0;

    //  free DB names
    delete[] g_wszDatabaseNames;
    delete[] g_wszDatabaseNamesOut;
    g_wszDatabaseNames = NULL;
    g_wszDatabaseNamesOut = NULL;
    g_cchDatabaseNames = 0;
    delete[] g_rgbDatabaseAggregationIDs;
    g_rgbDatabaseAggregationIDs = NULL;
    g_cDatabases = 0;
}


//  init OSU subsystem

CInitTermLock   g_OSUInitControl;

const ERR ErrOSUInit()
{
    ERR err;

    CInitTermLock::ERR errInit = g_OSUInitControl.ErrInitBegin();
    if ( CInitTermLock::ERR::errSuccess == errInit )
    {
        Assert( g_fOSUInit );
        Assert( g_OSUInitControl.CConsumers() > 0 );

        //  checks that any global variables and allocations are inline 
        //  with global settings and policy (and other global variables)

        Call( ErrOSUValidateGlobals() );   //  note returns on error

        return JET_errSuccess;
    }
    Assert( CInitTermLock::ERR::errInitBegun == errInit );

#ifdef USE_WATSON_API
    //  register watson for error reporting.
    RegisterWatson();
#endif

    //  init global variables, cresmgr args, etc

    Call( ErrOSUSetGlobals() );

    //  init perfmon required storage

    Call( ErrOSUInitPerfmonStorage() );

    //  init PLS, perfmon (and other things) need this so do it first

    Call( ErrOSUInitProcessorLocalStorage() );

    //  init the OS subsystem

    Call( ErrOSInit() );

    //  initialize all OSU subsystems in dependency order

    Call( ErrOSUTimeInit() );
    Call( ErrOSUConfigInit() );
    Call( ErrOSUEventInit() );
    Call( ErrOSUSyncInit() );
    Call( ErrOSUFileInit() );
    Call( ErrOSUNormInit() );

    //  configure timeout deadlock detection
    OSSyncConfigDeadlockTimeoutDetection( !FNegTest( fDisableTimeoutDeadlockDetection ) );

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlOsuInitDone, NULL );

    //  early check that globals are good (so we don't get caught in the fast path out above)

    CallS( ErrOSUValidateGlobals() );

    //  complete the OSU initialization

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


//  terminate OSU subsystem

void OSUTerm_()
{
    //  Tracking should've been taken care of.

    Assert( !g_fOSUInit );

    //  terminate all OSU subsystems in reverse dependency order

    OSUNormTerm();
    OSUFileTerm();
    OSUSyncTerm();
    OSUEventTerm();
    OSUConfigTerm();
    OSUTimeTerm();

    //  term the OS subsystem

    OSTerm();

    //  terminate PLS last as perfmon uses it

    OSUTermProcessorLocalStorage();

    //  terminate the perfmon configuration storage

    OSUTermPerfmonStorage();

#ifdef USE_WATSON_API
    //  unregister watson for error reporting.
    UnregisterWatson();
#endif
}


//  terminate OSU subsystem (user level)

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

    //  terminate all OSU subsystems

    OSUTerm_();

    //  complete the OSU termination

    Assert( !g_fOSUInit );  // yes we just set it above, but if there is a concurrency issue it could be reset.

    g_OSUInitControl.TermFinish();

}


