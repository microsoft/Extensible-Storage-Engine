// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

BOOL g_fDisablePerfmon;

void COSLayerPreInit::DisablePerfmon()
{
    g_fDisablePerfmon = fTrue;
}

#ifndef PERFMON_SUPPORT

void OSPerfmonPostterm()    {}
BOOL FOSPerfmonPreinit()    { return fTrue; }
void OSPerfmonTerm()        {}
ERR ErrOSPerfmonInit()      { return JET_errSuccess; }

UINT DwPERFCurPerfObj( void )
{
    AssertSz( fFalse,  "Needed for compile, but should not get executed" );
    return 0;
}


void COSLayerPreInit::EnablePerfmon()
{
    AssertSz( fFalse, "No perfmon support enabled / compiled in" );
}

#else

void COSLayerPreInit::EnablePerfmon()
{
    g_fDisablePerfmon = fFalse;
}

#include <wchar.h>
#include <winperf.h>
#include <aclapi.h>

//  Init/Term routines for performance monitoring

HANDLE  g_hPERFGDAMMF             = NULL;
PGDA    g_pgdaPERFGDA             = NULL;

HANDLE  g_hPERFInstanceMutex      = NULL;
HANDLE  g_hPERFIDAMMF             = NULL;
DWORD   g_cbPERFIDA               = 0;
PIDA    g_pidaPERFIDA             = NULL;
PIDA    g_pida                    = NULL;
HANDLE  g_hPERFGoEvent            = NULL;
HANDLE  g_hPERFReadyEvent         = NULL;
DWORD   g_cbMaxCounterBlockSize   = 0;
DWORD   g_cbInstanceSize          = 0;

HANDLE  g_hPERFEndDataThread      = NULL;
THREAD  g_threadPERFData          = NULL;

extern DWORD UtilPerfThread( DWORD_PTR dw );

//  Calculate space needed to store instance data.
//
//  Please, refer to eseperf.cxx for a detailed description of the data format.
//  in the following format:
//
//  NOTE: If an object has 0 instances, it only has cInstancesTotal
//      for its data. No PIDs are produced.

LOCAL INLINE DWORD CbPERFGetInstanceCountOffsetDataSize()
{
    return sizeof( ULONG64 );
}

LOCAL INLINE DWORD CbPERFGetInstanceCountDataSize()
{
    return sizeof( ULONG64 );
}

LOCAL INLINE const WCHAR* WszPERFGetProcessFileName()
{
    const WCHAR* wszProcessFileName = NULL;
    ProcInfoPIFPwszPf( &wszProcessFileName, NULL );

    return wszProcessFileName;
}

LOCAL INLINE DWORD CbPERFGetProcessFileNameDataSize()
{
    return QWORD_MULTIPLE( ( wcslen( WszPERFGetProcessFileName() ) + 1 ) * sizeof( WCHAR ) );
}

LOCAL INLINE DWORD CbPERFGetHeaderDataSize()
{
    return CbPERFGetInstanceCountOffsetDataSize() + CbPERFGetProcessFileNameDataSize();
}

LOCAL INLINE DWORD CbPERFGetAggregationIDArraySize( const DWORD cInstances )
{
    return QWORD_MULTIPLE( cInstances ); //  1 byte per ID.
}

LOCAL INLINE DWORD CbPERFGetPerformanceDataSize( const LONG* const rgcInstances )
{
    DWORD cInstancesTotal = 0;
    DWORD cbAggregationIDsTotal = 0;
    
    for ( DWORD dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++ )
    {
        const DWORD cInstances = rgcInstances[dwCurObj];
        cInstancesTotal += cInstances;
        cbAggregationIDsTotal += CbPERFGetAggregationIDArraySize( cInstances );
    }

    const DWORD cbHeader = CbPERFGetHeaderDataSize() * dwPERFNumObjects;
    const DWORD cbAggregationIDs = cbAggregationIDsTotal;
    const DWORD cbInstanceCount = CbPERFGetInstanceCountDataSize() * dwPERFNumObjects;
    const DWORD cbData = cInstancesTotal * g_cbInstanceSize;

    return cbHeader + cbAggregationIDs + cbInstanceCount + cbData;
}

LOCAL LONG g_cInits = 0;
LOCAL LONG g_cTerms = 0;
LOCAL LONG g_rgcInstancesMaxAtInit[4]; // == dwPERFNumObjects 
LOCAL ULONG g_opJetApiAtInit = 0x100;  // seeing a 0 here, may mean FOSDllUp() == fFalse, meaning called during DLL attach somehow.
#ifdef ENABLE_MICROSOFT_MANAGED_DATACENTER_LEVEL_OPTICS
LOCAL PVOID g_rgpfnStackAtInit[20]; // 160 bytes - 6 frames standard in ese.dll, 14 for the client.
#endif

void UtilPerfTerm(void)
{
    //  end the performance data thread

    if ( g_threadPERFData )
    {
        SetEvent( g_hPERFEndDataThread );
        UtilThreadEnd( g_threadPERFData );
        g_threadPERFData = NULL;
    }
    if ( g_hPERFEndDataThread )
    {
        CloseHandle( g_hPERFEndDataThread );
        g_hPERFEndDataThread = NULL;
    }

    //  terminate all counters/objects

    if ( g_cbInstanceSize )
    {
        for ( DWORD dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++ )
        {
            (void)rgpicfPERFICF[dwCurObj]( ICFTerm, NULL, NULL );
        }

        for ( DWORD dwCurCtr = 0; dwCurCtr < dwPERFNumCounters; dwCurCtr++ )
        {
            (void)rgpcefPERFCEF[dwCurCtr]( CEFTerm, NULL );
        }

        g_cbInstanceSize = 0;
        g_cbMaxCounterBlockSize = 0;
    }

    memset( g_rgcInstancesMaxAtInit, 0, sizeof( g_rgcInstancesMaxAtInit ) );
    g_opJetApiAtInit = 0x200;
    AtomicIncrement( &g_cTerms );
}

ERR ErrUtilPerfInit(void)
{
    ERR err;

    //  verify counter offsets and calculate instance size from template data

    const PERF_OBJECT_TYPE* ppotObjectSrc = (PPERF_OBJECT_TYPE)&PerfDataTemplateReadOnly;
    g_cbMaxCounterBlockSize = QWORD_MULTIPLE( sizeof( PERF_COUNTER_BLOCK ) );
    DWORD cbInstDefMax = 0;
    for ( DWORD dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++ )
    {
        const PERF_INSTANCE_DEFINITION* const ppidInstanceSrc = (PPERF_INSTANCE_DEFINITION)( (BYTE*)ppotObjectSrc + ppotObjectSrc->DefinitionLength );
        PERF_COUNTER_DEFINITION* ppcdCounterSrc = (PERF_COUNTER_DEFINITION*)( (BYTE*)ppotObjectSrc + ppotObjectSrc->HeaderLength );
        DWORD dwOffset = QWORD_MULTIPLE( sizeof( PERF_COUNTER_BLOCK ) );

        for ( DWORD dwCurCtr = 0; dwCurCtr < ppotObjectSrc->NumCounters; dwCurCtr++ )
        {
            AssertSzRTL( ppcdCounterSrc->CounterOffset == dwOffset,
                         "perfdata.pl miscalculated offsets (%d should equal %d).",
                         ppcdCounterSrc->CounterOffset, dwOffset );

            dwOffset += QWORD_MULTIPLE( ppcdCounterSrc->CounterSize );

            ppcdCounterSrc = (PERF_COUNTER_DEFINITION*)( (BYTE*)ppcdCounterSrc + ppcdCounterSrc->ByteLength );
        }

        cbInstDefMax = max( cbInstDefMax, ppidInstanceSrc->ByteLength );
        g_cbMaxCounterBlockSize = max( g_cbMaxCounterBlockSize, dwOffset );

        ppotObjectSrc = (PPERF_OBJECT_TYPE)( (BYTE*)ppotObjectSrc + ppotObjectSrc->TotalByteLength );
    }
    g_cbInstanceSize = cbInstDefMax + g_cbMaxCounterBlockSize;

    //  initialize all objects (perf. counter categories)

    LONG* rgcInstancesMax = NULL;
    Alloc( rgcInstancesMax = new LONG[dwPERFNumObjects] );

    Assert( dwPERFNumObjects == _countof( g_rgcInstancesMaxAtInit ) );
    for ( DWORD dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++ )
    {
        //  cache the maximum number of instances for all of them so we can compute
        //  the required buffer size afterwards

        rgcInstancesMax[dwCurObj] = rgpicfPERFICF[dwCurObj]( ICFInit, NULL, NULL );
        g_rgcInstancesMaxAtInit[dwCurObj] = rgcInstancesMax[dwCurObj];
    }
    g_opJetApiAtInit = (ULONG)OpTraceContext();
    AtomicIncrement( &g_cInits );
    OnMsftDatacenterOptics( RtlCaptureStackBackTrace( 1, _countof( g_rgpfnStackAtInit ), g_rgpfnStackAtInit, NULL ) );

    g_cbPERFIDA = roundup( sizeof( IDA ) + CbPERFGetPerformanceDataSize( rgcInstancesMax ), OSMemoryPageCommitGranularity() );

    delete[] rgcInstancesMax;
    rgcInstancesMax = NULL;

    //  initialize all perf. counters

    for ( DWORD dwCurCtr = 0; dwCurCtr < dwPERFNumCounters; dwCurCtr++ )
    {
        //  0 means success
        
        if ( rgpcefPERFCEF[dwCurCtr]( CEFInit, NULL ) != 0 )
        {
            //  terminate all the counters that were successfully initialized
            
            for ( dwCurCtr--; LONG( dwCurCtr ) >= 0; dwCurCtr-- )
            {
                rgpcefPERFCEF[dwCurCtr]( CEFTerm, NULL );
            }

            //  terminate all the categories

            for ( DWORD dwCurObj = dwPERFNumObjects-1; LONG( dwCurObj ) >= 0; dwCurObj-- )
            {
                (void)rgpicfPERFICF[dwCurObj]( ICFTerm, NULL, NULL );
            }

            Call( ErrERRCheck( JET_errPermissionDenied ) );
        }
    }

    //  create our performance data thread

    if ( !( g_hPERFEndDataThread = CreateEventW( NULL, FALSE, FALSE, NULL ) ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }
    Call( ErrUtilThreadCreate(  UtilPerfThread,
                                OSMemoryPageReserveGranularity(),
                                priorityTimeCritical,
                                &g_threadPERFData,
                                NULL ) );

    return JET_errSuccess;

HandleError:
    UtilPerfTerm();
    return err;
}

void UtilPerfThreadTerm(void)
{
    //  terminate all resources

    if ( g_hPERFReadyEvent )
    {
        SetHandleInformation( g_hPERFReadyEvent, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( g_hPERFReadyEvent );
        g_hPERFReadyEvent = NULL;
    }

    if ( g_hPERFGoEvent )
    {
        SetHandleInformation( g_hPERFGoEvent, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( g_hPERFGoEvent );
        g_hPERFGoEvent = NULL;
    }

    if ( g_pida )
    {
        VirtualFree( g_pida, 0, MEM_RELEASE );
        g_pida = NULL;
    }

    if ( g_pidaPERFIDA )
    {
        UnmapViewOfFile( g_pidaPERFIDA );
        g_pidaPERFIDA = NULL;
    }

    if ( g_hPERFIDAMMF )
    {
        SetHandleInformation( g_hPERFIDAMMF, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( g_hPERFIDAMMF );
        g_hPERFIDAMMF = NULL;
    }

    if ( g_hPERFInstanceMutex )
    {
        ReleaseMutex( g_hPERFInstanceMutex );
        SetHandleInformation( g_hPERFInstanceMutex, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( g_hPERFInstanceMutex );
        g_hPERFInstanceMutex = NULL;
    }

    if ( g_pgdaPERFGDA )
    {
        UnmapViewOfFile( g_pgdaPERFGDA );
        g_pgdaPERFGDA = NULL;
    }

    if ( g_hPERFGDAMMF )
    {
        SetHandleInformation( g_hPERFGDAMMF, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( g_hPERFGDAMMF );
        g_hPERFGDAMMF = NULL;
    }
}

//  make sure that the performance DLL will check this instance index
//
//  NOTE:  protect ourselves from corruption of the GDA
ERR ErrUtilPerfIEnsureCheckInstanceInstance(
    _In_ const DWORD iInstance )
{
    ERR err = JET_errSuccess;

    __try
    {
        size_t cAttempt     = 0;
        size_t cAttemptMax  = 1024;

        OSSYNC_FOREVER
        {
            ULONG ulBIExpected;
            ULONG ulAI;
            ULONG ulBI;

            ulBIExpected    = g_pgdaPERFGDA->iInstanceMax;
            ulAI            = max( iInstance + 1, ulBIExpected );
            ulBI            = AtomicCompareExchange(    (LONG*)&g_pgdaPERFGDA->iInstanceMax,
                                                        ulBIExpected,
                                                        ulAI );

            if ( ulBI == ulBIExpected || iInstance < ulBI )
            {
                break;
            }
            if ( ++cAttempt > cAttemptMax )
            {
                err = ErrERRCheck( JET_errPermissionDenied );
                break;
            }
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        err = ErrERRCheck( JET_errPermissionDenied );
    }

    return err;
}

ERR ErrUtilPerfThreadInit(void)
{
    ERR                         err         = JET_errSuccess;
    PSECURITY_DESCRIPTOR        pSD         = NULL;
    DWORD                       cbSD        = 0;
    SECURITY_ATTRIBUTES         sa          = { 0 };
    WCHAR                       wszT[ 256 ];

    NTOSFuncStd( pfnConvertStringSecurityDescriptorToSecurityDescriptorW, g_mwszzSecSddlLibs, ConvertStringSecurityDescriptorToSecurityDescriptorW, oslfExpectedOnWin5x | oslfStrictFree );

    Call( pfnConvertStringSecurityDescriptorToSecurityDescriptorW.ErrIsPresent() );

    if ( !pfnConvertStringSecurityDescriptorToSecurityDescriptorW(
                aclPERF,
                SDDL_REVISION_1,
                &pSD,
                &cbSD ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    sa.nLength = cbSD;
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = pSD;

    // open/create the shared global data area
    // retry in case object is closed between calls to create and open
    OSStrCbFormatW( wszT, sizeof( wszT ), L"Global\\GDA:  %ws", wszPERFVersion );
    Enforce( PERF_SIZEOF_GLOBAL_DATA >= sizeof( GDA ) );
    Expected( PERF_SIZEOF_GLOBAL_DATA == roundup( sizeof( GDA ), OSMemoryPageCommitGranularity() ) );
    Expected( ( PERF_SIZEOF_GLOBAL_DATA % OSMemoryPageCommitGranularity() ) == 0 );
    PERFCreateFileMapping( g_hPERFGDAMMF, sa, wszT, PERF_SIZEOF_GLOBAL_DATA );
    if ( !g_hPERFGDAMMF )
    {
#ifdef DEBUG
        DWORD dw = GetLastError();

        Unused( dw );
#endif
        Call( ErrERRCheck( JET_errPermissionDenied ) );
    }
    SetHandleInformation( g_hPERFGDAMMF, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
    if ( !( g_pgdaPERFGDA = PGDA( MapViewOfFile(  g_hPERFGDAMMF,
                                                FILE_MAP_WRITE,
                                                0,
                                                0,
                                                0 ) ) ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    //  find an instance number for which we can successfully gain ownership of
    //  the instance mutex

    DWORD iInstance;
    for ( iInstance = 0; iInstance < PERF_PERFINST_MAX; iInstance++ )
    {
        OSStrCbFormatW( wszT, sizeof( wszT ), L"Global\\Instance%d:  %ws", iInstance, wszPERFVersion );
        PERFCreateMutex( g_hPERFInstanceMutex, sa, wszT );
        if ( !g_hPERFInstanceMutex )
        {
            Call( ErrERRCheck( JET_errPermissionDenied ) );
        }

        DWORD errWin;
        errWin = WaitForSingleObject( g_hPERFInstanceMutex, 0 );

        if ( errWin == WAIT_OBJECT_0 || errWin == WAIT_ABANDONED )
        {
            //  if the file mapping already exists, the existing size may not be enough to accomomdate
            //  the currently needed size, if that is the case, skip to the next slot

            OSStrCbFormatW( wszT, sizeof( wszT ), L"Global\\IDA%d:  %ws", iInstance, wszPERFVersion );
            g_hPERFIDAMMF = CreateFileMappingW( INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE | SEC_COMMIT, 0, g_cbPERFIDA, wszT );
            const DWORD dwErrorFileMapping = GetLastError();
            if ( ( g_hPERFIDAMMF != NULL ) )
            {
                if ( dwErrorFileMapping == ERROR_ALREADY_EXISTS )
                {
                    SetHandleInformation( g_hPERFIDAMMF, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
                    CloseHandle( g_hPERFIDAMMF );
                    g_hPERFIDAMMF = NULL;
                }
                else
                {
                    SetHandleInformation( g_hPERFInstanceMutex, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
                    SetHandleInformation( g_hPERFIDAMMF, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
                    break;
                }
            }

            ReleaseMutex( g_hPERFInstanceMutex );
            SetHandleInformation( g_hPERFInstanceMutex, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        }

        CloseHandle( g_hPERFInstanceMutex );
        g_hPERFInstanceMutex = NULL;
    }

    if ( !g_hPERFInstanceMutex )
    {
        Call( ErrERRCheck( JET_errPermissionDenied ) );
    }

    //  open the shared instance data area

    if ( !( g_pidaPERFIDA = PIDA( MapViewOfFile(  g_hPERFIDAMMF,
                                                FILE_MAP_ALL_ACCESS,
                                                0,
                                                0,
                                                0 ) ) ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    //  allocate our temp instance data area

    if ( !( g_pida = PIDA( VirtualAlloc(  NULL,
                                        g_cbPERFIDA,
                                        MEM_RESERVE | MEM_COMMIT,
                                        PAGE_READWRITE ) ) ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    //  set total allocated size

    g_pida->cbIDA = g_cbPERFIDA;

    //  open/create the go event

    OSStrCbFormatW( wszT, sizeof( wszT ), L"Global\\Go%d:  %ws", iInstance, wszPERFVersion );
    PERFCreateEvent( g_hPERFGoEvent, sa, wszT );
    if ( !g_hPERFGoEvent )
    {
        Call( ErrERRCheck( JET_errPermissionDenied ) );
    }
    SetHandleInformation( g_hPERFGoEvent, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

    //  open/create the ready event

    // This banned API was fixed while looking at the Seattle Center's big fountain.
    OSStrCbFormatW( wszT, sizeof( wszT ), L"Global\\Ready%d:  %ws", iInstance, wszPERFVersion );
    PERFCreateEvent( g_hPERFReadyEvent, sa, wszT );
    if ( !g_hPERFReadyEvent )
    {
        Call( ErrERRCheck( JET_errPermissionDenied ) );
    }
    SetHandleInformation( g_hPERFReadyEvent, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

    Call( ErrUtilPerfIEnsureCheckInstanceInstance( iInstance ) );

    //  set our connect time
    //  add the current process ID, to reduce even further the possibility
    //  of collision across processes starting up at the same time

    do
    {
        g_pida->tickConnect = TickOSTimeCurrent() + DwUtilProcessId();
    }
    while ( g_pida->tickConnect == 0 );
    
HandleError:
    if ( pSD )
    {
        LocalFree( pSD );
    }
    if ( err < JET_errSuccess )
    {
        UtilPerfThreadTerm();
    }
    return err;
}


UINT DwPERFCurPerfObj( void )
{
    return Postls()->dwCurPerfObj;
}


//  Performance Data thread  */

DWORD UtilPerfThread( DWORD_PTR parm )
{
    if ( ErrUtilPerfThreadInit() < JET_errSuccess )
    {
        return 0;
    }

    //  Retrieve the server process' file name: the offset of the instance count and the process file name
    //  could be copied only once for the entire chunk of data but, instead, we'll choose to repeat them
    //  at the beginning of each counter object (i.e., category) so that we simplify the processing algorithm
    //  in eseperf/esentprf.
    
    const WCHAR* wszProcessFileName = WszPERFGetProcessFileName();
    const DWORD cbProcessFileName = CbPERFGetProcessFileNameDataSize();
    const DWORD ibInstanceCount = CbPERFGetHeaderDataSize();

    TICK tickLastThreshold = TickOSTimeCurrent();
    DWORD cCollectionsLastThreshold = 0;
    BOOL fHasValidData = fFalse;
    DWORD cExceptionRetry = 0;
    for ( ; ; )
    {
        //  TLS to keep track of what object we are currently gathering data for. This is used
        //  by PERFInstanceDelayedTotalWithClass::PassTo and PERFIntanceLiveTotalWithClass::PassTo to determine
        //  if iInstance refers to tableclass instance or Jet Instance.
        Postls()->dwCurPerfObj = PERF_OBJECT_INVALID;

        //  run at least through one collection cycle without waiting if
        //  we are just starting
        if ( fHasValidData )
        {
            //  set our ready event to indicate we are done collecting data and / or
            //  we are ready to collect more data

            SetEvent( g_hPERFReadyEvent );

            //  wait to either be killed or to be told to collect data

            const size_t    chWait              = 2;
            HANDLE          rghWait[ chWait ]   = { g_hPERFEndDataThread, g_hPERFGoEvent };
            if ( WaitForMultipleObjectsEx( chWait, rghWait, FALSE, INFINITE, FALSE ) == WAIT_OBJECT_0 )
            {
                break;
            }
            ResetEvent( g_hPERFGoEvent );
        }
        
        //  When wildcards are used to add counters to a given collector, CollectPerformanceData()
        //  (which triggers g_hPERFGoEvent) gets called approximately as many times as the number of
        //  matching counters. On a system with many processes/instances, this may be a problem
        //  and may cause clients to time out waiting for counters to be added, since the entire
        //  loop below will run potentially thousands of times. To solve that, we are limiting
        //  the number of times the collection loop is allowed to run per second.
        //  If the instance list needs refreshing, we will not enforce this optimization.

        bool fRefreshInstanceList = false;
        ProcInfoPIFPwszPf( NULL, &fRefreshInstanceList );

        if ( !fRefreshInstanceList && ( cCollectionsLastThreshold >= PERF_COLLECTION_FREQ_MAX ) )
        {
            const TICK tickNow = TickOSTimeCurrent();
            const TICK dtickSinceLastThreshold = DtickDelta( tickLastThreshold, tickNow );
            if ( ( dtickSinceLastThreshold > 0 ) && ( dtickSinceLastThreshold < 1000 ) )
            {
                continue;
            }
            else
            {
                tickLastThreshold = tickNow;
                cCollectionsLastThreshold = 0;
            }
        }
        cCollectionsLastThreshold++;

        //  collect instances for all objects

        for ( DWORD dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++ )
        {
            rglPERFNumInstances[dwCurObj] = rgpicfPERFICF[dwCurObj]( ICFData, &rgwszPERFInstanceList[dwCurObj], &rgpbPERFInstanceAggregationIDs[dwCurObj] );
        }

        const DWORD cbInstanceCount = CbPERFGetInstanceCountDataSize();
        const DWORD cbSpaceNeeded = CbPERFGetPerformanceDataSize( rglPERFNumInstances );
        g_pida->cbPerformanceData = cbSpaceNeeded;

        //  verify that we have sufficient store to collect our data

        Enforce( g_cbPERFIDA == g_pida->cbIDA );
        // This shadows the next enforce until this extra debug code is removed.
        EnforceSz( g_cbPERFIDA - sizeof( IDA ) >= cbSpaceNeeded, OSFormat( "PerfIdaBufferTooSmall:opApi:%I32ud\n", g_opJetApiAtInit ) );
        Enforce( g_cbPERFIDA - sizeof( IDA ) >= cbSpaceNeeded );

        //  get a pointer to our data block

        LPVOID pvBlock = g_pida->rgbPerformanceData;

        //  loop through all objects, filling our block with instance data

        DWORD dwCurCtr = 0;
        PPERF_OBJECT_TYPE ppotObjectSrc = (PPERF_OBJECT_TYPE)&PerfDataTemplateReadOnly;
        for ( DWORD dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++ )
        {
            Postls()->dwCurPerfObj = dwCurObj;

            //  first, write process file name.

            Enforce( CbPERFGetInstanceCountOffsetDataSize() == sizeof( ULONG64 ) );
            *( (ULONG64*)pvBlock ) = (ULONG64)ibInstanceCount;
            pvBlock = (ULONG64*)pvBlock + 1;
            memset( pvBlock, 0, cbProcessFileName );
            OSStrCbCopyW( (WCHAR*)pvBlock, cbProcessFileName, wszProcessFileName );
            pvBlock = (BYTE*)pvBlock + cbProcessFileName;

            //  write the number of instances for this object to the block

            Enforce( CbPERFGetInstanceCountDataSize() == sizeof( ULONG64 ) );
            const DWORD cInstances = (DWORD)rglPERFNumInstances[dwCurObj];
            *( (ULONG64*)pvBlock ) = (ULONG64)cInstances;
            pvBlock = (BYTE*)pvBlock + cbInstanceCount;

            //  write the aggregation IDs.

            const DWORD cbAggregationIDs = CbPERFGetAggregationIDArraySize( cInstances );
            memset( pvBlock, 0, cbAggregationIDs );
            UtilMemCpy( pvBlock, (void*)rgpbPERFInstanceAggregationIDs[dwCurObj], cInstances );
            pvBlock = (BYTE*)pvBlock + cbAggregationIDs;
            
            //  get current instance name list

            LPCWSTR lpwszInstName = rgwszPERFInstanceList[dwCurObj];

            //  loop through each instance

            const PERF_INSTANCE_DEFINITION* const ppidInstanceSrc = (PPERF_INSTANCE_DEFINITION)( (BYTE*)ppotObjectSrc + ppotObjectSrc->DefinitionLength );

            PPERF_INSTANCE_DEFINITION ppidInstanceDest = (PPERF_INSTANCE_DEFINITION)pvBlock;
            for ( DWORD dwCurInst = 0; dwCurInst < cInstances; dwCurInst++ )
            {
                //  initialize instance/counter block from template data

                UtilMemCpy( (void*)ppidInstanceDest, (void*)ppidInstanceSrc, ppidInstanceSrc->ByteLength );
                PPERF_COUNTER_BLOCK const ppcbCounterBlockDest = (PPERF_COUNTER_BLOCK)( (BYTE*)ppidInstanceDest + ppidInstanceDest->ByteLength );
                memset( (void*)ppcbCounterBlockDest, 0, g_cbMaxCounterBlockSize );
                ppcbCounterBlockDest->ByteLength = g_cbMaxCounterBlockSize;

                //  no unique instance ID

                ppidInstanceDest->UniqueID = PERF_NO_UNIQUE_ID;

                //  NOTE:  performance DLL sets object hierarchy information  */

                //  write instance name to buffer, avoiding overflow and illegal
                //  characters

                LPWSTR const wszName = (wchar_t*)( (BYTE*)ppidInstanceDest + ppidInstanceDest->NameOffset );
                DWORD cchName;
                const DWORD cchNameCapacity = ( ppidInstanceDest->ByteLength - ppidInstanceDest->NameOffset ) / sizeof( wchar_t );
                for ( cchName = 0; cchName < cchNameCapacity && lpwszInstName[cchName]; cchName++ )
                {
                    const wchar_t wch = lpwszInstName[cchName];
                    wszName[cchName] = PERFIsValidCharacter( wch ) ? wch : PERF_INVALID_CHAR_REPL;
                }
                Enforce( cchNameCapacity >= 1 );
                cchName = min( cchNameCapacity - 1, cchName );
                wszName[cchName] = L'\0';
                ppidInstanceDest->NameLength = (ULONG)( ( cchName + 1 ) * sizeof( wchar_t ) );
                lpwszInstName += ( wcslen( lpwszInstName ) + 1 );

                //  collect counter data for this instance

                const PERF_COUNTER_DEFINITION* ppcdCounterSrc = (PERF_COUNTER_DEFINITION*)( (BYTE*)ppotObjectSrc + ppotObjectSrc->HeaderLength );
                for ( DWORD dwCollectCtr = 0; dwCollectCtr < ppotObjectSrc->NumCounters; dwCollectCtr++ )
                {
                    rgpcefPERFCEF[dwCollectCtr + dwCurCtr]( dwCurInst, (void*)( (BYTE*)ppcbCounterBlockDest + ppcdCounterSrc->CounterOffset ) );
                    ppcdCounterSrc = (PERF_COUNTER_DEFINITION*)( (BYTE*)ppcdCounterSrc + ppcdCounterSrc->ByteLength );
                }
                ppidInstanceDest = (PPERF_INSTANCE_DEFINITION)( (BYTE*)ppidInstanceDest + g_cbInstanceSize );
            }
            dwCurCtr += ppotObjectSrc->NumCounters;
            ppotObjectSrc = (PPERF_OBJECT_TYPE)( (BYTE*)ppotObjectSrc + ppotObjectSrc->TotalByteLength );
            pvBlock = (void*)( (BYTE*)pvBlock + g_cbInstanceSize * cInstances );
        }

        Postls()->dwCurPerfObj = PERF_OBJECT_INVALID;

        //  copy our generated performance data into the IDA
        //
        //  NOTE:  protect ourselves from corruption of the IDA

        __try
        {
            memcpy( g_pidaPERFIDA, g_pida, sizeof( IDA ) + g_pida->cbPerformanceData );
            fHasValidData = fTrue;
            cExceptionRetry = 0;
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            AssertPREFIX( NULL != g_pida );
            AssertPREFIX( NULL != g_pidaPERFIDA );

            if ( ++cExceptionRetry >= PERF_COPY_RETRY )
            {
                break;
            }
            
            fHasValidData = fFalse;
            UtilSleep( PERF_TIMEOUT );
        }
    }

    UtilPerfThreadTerm();

    return 0;
}


//  post-terminate perfmon subsystem

void OSPerfmonPostterm()
{
    //  nop
}

//  pre-init perfmon subsystem

BOOL FOSPerfmonPreinit()
{
    //  nop

    return fTrue;
}


//  terminate perfmon subsystem

void OSPerfmonTerm()
{
    UtilPerfTerm();
}

//  init perfmon subsystem

ERR ErrOSPerfmonInit()
{
    ERR err = JET_errSuccess;

    //  start perfmon thread if perfmon has not been disabled

    if ( !g_fDisablePerfmon )
    {
        Call( ErrUtilPerfInit() );
    }

HandleError:
    return err;
}

#endif  //  MINIMAL_FUNCTIONALITY

