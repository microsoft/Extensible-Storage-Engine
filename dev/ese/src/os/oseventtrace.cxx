// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


// ---------------------------------------------------------------------------
//
//  Instructions for adding new ETW tracing events:
//
//    Because there are 3 different lists being merged into the ETW events space the
//    lists are all auto-generated to enable easy correct modifications.
//    (For Regular ETW Event - all modifications are automated via gengen, see instructions in EseEtwEventsPregen.txt)
//    (For JET API events -  all modifications are completed automated through the build, just edit _jet.hxx)
//    (For JET_tracetag edits - are also completely automated through the build, just edit jethdr.w)
//
//  Generally speaking (about ETW events):
//
//      New Events, Templates, Tasks, Keywords, and localization strings will be added (from data
//      in EseEtwEventsPregen.txt, _jet.hxx, or jethdr - the three sources of truth) to the XML-based 
//      manifest file ese[nt]\src\_etw\Microsoft-ETW-ESE.mc.  When .mc is compiled header file will 
//      a Microsoft-ETW-ESE.h, will be generated containing macros for logging 
//      each event, which are called by ETW_Xxxx_Trace() which is called by the native ETXxxx()
//      wraper routine in EseEventTrace.g.hxx.
//
//  Specifics:
//
//      Following examples and code references are presuming a mythical / non-existing space.cxx trace that is 
//      called SpaceNewRoot that takes a pgno, a grbit (which I made 64-bit for fun), and a table name.
//
//      Code Gen Flow (for Regular ETW Event):
//
//      gengenetw.pl + EseEtwEventsPregen.txt, generates
//          -> Microsoft-ETW-ESE.mc     NOTE: Modifies and _re_-generates only 4 different tagged sections of file.
//          -> oseventtrace.g.hxx       Contains: _etguidSpaceNewRoot enum values (included within main oseventtrace.hxx).
//          -> EseEventTrace.g.hxx      Contains: Native ETSpaceNewRoot() trace functions (this is called from Engine)
//          -> EseEventTraceType.g.hxx  Contains: Packed struct of trace.
//          -> _oseventtraceimpl.g.hxx  Contains: ETW_SpaceNewRoot_Trace and g_rgOSEventTraceGUID entry.
//
//      Actual Flow (all ETW Events):
//
//      ETSpaceNewRoot( pgno, grbit, szTable );                                         EseEventTrace.g.hxx
//          EseSpaceNewRoot ettT = { pgno, grbit, szTable };                            EseEventTrace.g.hxx
//          OSEventTrace( _etguidSpaceNewRoot, 3, &pgno, &grbit, szTable );             EseEventTrace.g.hxx -> oseventtrace.hxx
//              if ( FOSEventTraceEnabled() )                                           oseventtrace.hxx
//                  OSEventTrace_( _etguidSpaceNewRoot, 3, ... /* varargs */ );         oseventtrace.hxx -> oseventtrace.cxx
//                      MOF_FIELD               EventTraceData[ 32 ];                           oseventtrace.cxx
//                      for ( i = 0; i < c; i++ )                                               oseventtrace.cxx
//                          EventTraceData[ i ].DataPtr = (ULONG64)va_arg( arglist, void* );    oseventtrace.cxx
//                      g_rgOSEventTraceGUID[ etguid ].pfn( EventTraceData )                    _oseventtraceimp.g.hxx (array started in oseventtrace.cxx)
//                          ETW_SpaceNewRoot_Trace( rgparg )                                    _oseventtraceimp.g.hxx 
//                              EventWriteESE_SpaceNewRoot_Trace(                               _oseventtraceimp.g.hxx -> obj\amd64\Microsoft-ETW-ESE.h (generated from .mc)
//                                                  *(DWORD*)rgparg[0].DataPtr,                         ...
//                                                  *(QWORD*)rgparg[1].DataPtr,                         ...
//                                                  (CHAR*)rgparg[2].DataPtr );                         ...
//                                  MCGEN_ENABLE_CHECK( Microsoft_Exchange_ESE_Context, ESE_SpaceNewRoot_Trace ) ?          obj\amd64\Microsoft-ETW-ESE.h
//                                      Template_dqz( Microsoft_Exchange_ESEHandle, &ESE_SpaceNewRoot_Trace, pgno, grbit, szTable ) :   ...
//                                      ERROR_SUCCESS;                                                                                  ...
//                                          #define ARGUMENT_COUNT_dqz 3                                                    obj\amd64\Microsoft-ETW-ESE.h
//                                          EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_dqz];                            obj\amd64\Microsoft-ETW-ESE.h
//                                          EventDataDescCreate(&EventData[0], &pgno, sizeof(const unsigned int));          obj\amd64\Microsoft-ETW-ESE.h
//                                          EventDataDescCreate(&EventData[1], &grbit, sizeof(const unsigned __int64));     obj\amd64\Microsoft-ETW-ESE.h
//                                          EventDataDescCreate(&EventData[2],                                              obj\amd64\Microsoft-ETW-ESE.h
//                                                  (szTable != NULL) ? szTable : "NULL",                                       ...
//                                                  (szTable != NULL) ? (ULONG)((strlen(szTable)+1) * sizeof(CHAR)) : ...);     ...
//                                          EventDataDescCreate(&EventData[2], &szTable, sizeof(const unsigned int) );      obj\amd64\Microsoft-ETW-ESE.h
//                                          return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_dqz, EventData);        obj\amd64\Microsoft-ETW-ESE.h
//
//          Obviously, we could use some collapsing of layers.
//

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

//  Wrappers for Trace EventWrite* macros. See autogenerated Microsoft-ETW-ESE.h for macros


// This included file is auto-generated, and has the implementation of all the ETW_* event functions for the auto-gen
// ETW events from EseEtwEventsPregen.txt
#include "_oseventtraceimpl.g.hxx"


// This included file is auto-generated, and has the implmentation of all the ETW_* event functions for the tracetags
#define PROVIDE_TRACETAG_ETW_IMPL 1
#include "_tracetagimpl.h"


typedef void (*PFNEVENTWRITE) (const MOF_FIELD *);

struct MANIFEST_GUID_REGISTRATION
{
    UINT carg;
    PFNEVENTWRITE pfn;
};

// add a structure for each ETW event 

const MANIFEST_GUID_REGISTRATION g_rgOSEventTraceGUID[] =
{
        // Auto-Generated ETW Traces (from EseEtwEventsPregen.txt)
        // this macro is implemented in the generated file _oseventtraceimpl.g.hxx (included above)
        ESE_ETW_AUTOGEN_PFN_ARRAY

        //Jet_tracetag GUIDS
        // this macro is implemented in the generated file _tracetagimpl.h (included above)
        ESE_ETW_TRACETAG_PFN_ARRAY
};

C_ASSERT( _countof( g_rgOSEventTraceGUID ) == etguidOsTraceBase - _etguidTraceBaseId + JET_tracetagMax );

//  Event Tracing API

void __cdecl OSEventTrace_( const ULONG etguid,
                            const size_t cData,
                            ... )
{
    MOF_FIELD               EventTraceData[ 32 ];

    //  convert the enum to an index into the trace defn array ...
    const ULONG ietguid = etguid - _etguidTraceBaseId;

    //  gather all the data for this event trace, if requested

    size_t  iEventTraceData = 0;
    va_list arglist;

    va_start( arglist, cData );
    for ( iEventTraceData = 0; iEventTraceData < min( cData, _countof(EventTraceData) ); iEventTraceData++ )
    {
        EventTraceData[ iEventTraceData ].DataPtr   = (ULONG64)va_arg( arglist, void* );

        //  Length and DataType are not used for anything
        //  the auto-generated templates in Microsoft-ETW-ESE.h already take care of this
        EventTraceData[ iEventTraceData ].Length    = 0;
        EventTraceData[ iEventTraceData ].DataType  = NULL;
    }
    va_end( arglist );

    AssertSz( g_rgOSEventTraceGUID[ ietguid ].carg == cData, "Caller must have passed in right number of parameters." );
    AssertSz( cData == iEventTraceData, "Parameters truncated. Feel free to increase the size of the EventTraceData array." );

    //  Call EventWrite<EventType>
    g_rgOSEventTraceGUID[ ietguid ].pfn(EventTraceData);
}

//  post-terminate event trace subsystem

void OSEventTracePostterm()
{
    //  nop
}

//  pre-init event trace subsystem

BOOL FOSEventTracePreinit()
{
    //  nop

    return fTrue;
}


//  terminate event trace subsystem

void OSEventTraceTerm()
{
    //  Clean up the provider before unloading
#ifdef ESENT
    EventUnregisterMicrosoft_Windows_ESE();
#else
    EventUnregisterMicrosoft_Exchange_ESE();
#endif
}

//  init event trace subsystem

ERR ErrOSEventTraceInit()
{
    //  Register the provider before logging any data
#ifdef ESENT
    EventRegisterMicrosoft_Windows_ESE();
#else
    EventRegisterMicrosoft_Exchange_ESE();
#endif

    // Needed to make everything compile.  Harmless.
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

    // Do NOT add a default return, since everything is compile time, if you hit an error 
    // that "FOSEventTraceEnabled<XXXX>" does not a return a value, it means you do not 
    // have a proper else if constexpr( eguid == XXX ) type check case above.
    // return !fThanks;
}

