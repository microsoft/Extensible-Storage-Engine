// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.



#include "osstd.hxx"
#include <evntprov.h>

NTOSFuncError( g_pfnEventWrite, g_mwszzEventingProviderLibs, EventWrite, oslfExpectedOnWin5x | oslfStrictFree );
NTOSFuncError( g_pfnEventWriteTransfer, g_mwszzEventingProviderLibs, EventWriteTransfer, oslfExpectedOnWin5x | oslfStrictFree );
NTOSFuncError( g_pfnEventRegister, g_mwszzEventingProviderLibs, EventRegister, oslfExpectedOnWin5x | oslfStrictFree );
NTOSFuncError( g_pfnEventUnregister, g_mwszzEventingProviderLibs, EventUnregister, oslfExpectedOnWin5x | oslfStrictFree );

#define EventWrite         g_pfnEventWrite
#define EventWriteTransfer g_pfnEventWriteTransfer
#define EventRegister      g_pfnEventRegister
#define EventUnregister    g_pfnEventUnregister

#include "Microsoft-ETW-ESE.h"



#include "_oseventtraceimpl.g.hxx"


#define PROVIDE_TRACETAG_ETW_IMPL 1
#include "_tracetagimpl.h"


typedef void (*PFNEVENTWRITE) (const MOF_FIELD *);

struct MANIFEST_GUID_REGISTRATION
{
    UINT carg;
    PFNEVENTWRITE pfn;
};


const MANIFEST_GUID_REGISTRATION g_rgOSEventTraceGUID[] =
{
        ESE_ETW_AUTOGEN_PFN_ARRAY

        ESE_ETW_TRACETAG_PFN_ARRAY
};

C_ASSERT( _countof( g_rgOSEventTraceGUID ) == etguidOsTraceBase - _etguidTraceBaseId + JET_tracetagMax );


void __cdecl OSEventTrace_( const ULONG etguid,
                            const size_t cData,
                            ... )
{
    MOF_FIELD               EventTraceData[ 32 ];

    const ULONG ietguid = etguid - _etguidTraceBaseId;


    size_t  iEventTraceData = 0;
    va_list arglist;

    va_start( arglist, cData );
    for ( iEventTraceData = 0; iEventTraceData < min( cData, _countof(EventTraceData) ); iEventTraceData++ )
    {
        EventTraceData[ iEventTraceData ].DataPtr   = (ULONG64)va_arg( arglist, void* );

        EventTraceData[ iEventTraceData ].Length    = 0;
        EventTraceData[ iEventTraceData ].DataType  = NULL;
    }
    va_end( arglist );

    AssertSz( g_rgOSEventTraceGUID[ ietguid ].carg == cData, "Caller must have passed in right number of parameters." );
    AssertSz( cData == iEventTraceData, "Parameters truncated. Feel free to increase the size of the EventTraceData array." );

    g_rgOSEventTraceGUID[ ietguid ].pfn(EventTraceData);
}


void OSEventTracePostterm()
{
}


BOOL FOSEventTracePreinit()
{

    return fTrue;
}



void OSEventTraceTerm()
{
#ifdef ESENT
    EventUnregisterMicrosoft_Windows_ESE();
#else
    EventUnregisterMicrosoft_Exchange_ESE();
#endif
}


ERR ErrOSEventTraceInit()
{
#ifdef ESENT
    EventRegisterMicrosoft_Windows_ESE();
#else
    EventRegisterMicrosoft_Exchange_ESE();
#endif

    (void)FOSEventTraceEnabled();

    return JET_errSuccess;
}


INLINE BOOL FOSEventTraceEnabled()
{
#ifdef ESENT
    return !g_fDisableTracingForced && Microsoft_Windows_ESE_Context.IsEnabled == EVENT_CONTROL_CODE_ENABLE_PROVIDER;
#else
    return !g_fDisableTracingForced && Microsoft_Exchange_ESE_Context.IsEnabled == EVENT_CONTROL_CODE_ENABLE_PROVIDER;
#endif
}

template< OSEventTraceGUID etguid >
INLINE BOOL FOSEventTraceEnabled()
{
    if ( g_fDisableTracingForced )
    {
        return fFalse;
    }

#ifdef ESENT
    MCGEN_TRACE_CONTEXT * p = &Microsoft_Windows_ESE_Context;
#else
    MCGEN_TRACE_CONTEXT * p = &Microsoft_Exchange_ESE_Context;
#endif

    if constexpr( etguid == _etguidSysStationId )
    {
        return MCGEN_ENABLE_CHECK( (*p), ESE_SysStationId_Trace );
    }
    else if constexpr( etguid == _etguidInstStationId )
    {
        return MCGEN_ENABLE_CHECK( (*p), ESE_InstStationId_Trace );
    }
    else if constexpr( etguid == _etguidFmpStationId )
    {
        return MCGEN_ENABLE_CHECK( (*p), ESE_FmpStationId_Trace );
    }
    else if constexpr( etguid == _etguidDiskStationId )
    {
        return MCGEN_ENABLE_CHECK( (*p), ESE_DiskStationId_Trace );
    }
    else if constexpr( etguid == _etguidFileStationId )
    {
        return MCGEN_ENABLE_CHECK( (*p), ESE_FileStationId_Trace );
    }
    else if constexpr( etguid == _etguidIsamDbfilehdrInfo )
    {
        return MCGEN_ENABLE_CHECK( (*p), ESE_IsamDbfilehdrInfo_Trace );
    }
    else if constexpr( etguid == _etguidCacheRequestPage )
    {
        return MCGEN_ENABLE_CHECK( (*p), ESE_CacheRequestPage_Trace );
    }
    else if constexpr( etguid == _etguidCacheMemoryUsage )
    {
        return MCGEN_ENABLE_CHECK( ( *p ), ESE_CacheMemoryUsage_Trace );
    }

}

