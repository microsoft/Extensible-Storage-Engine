// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_EVENT_TRACE_HXX_INCLUDED
#define _OS_EVENT_TRACE_HXX_INCLUDED


#include "oseventtrace.g.hxx"


void __cdecl OSEventTrace_( const ULONG etguid,
                            const size_t cData = 0,
                            ... );

INLINE BOOL FOSEventTraceEnabled();

template< OSEventTraceGUID etguid >
INLINE BOOL FOSEventTraceEnabled();

#define OSEventTrace if ( FOSEventTraceEnabled() ) OSEventTrace_


enum TraceStationIdentificationReason : BYTE
{
    tsidrInvalid = 0,

    tsidrPulseInfo = 1,

    tsidrOpenInit = 2,
    tsidrCloseTerm = 3,

    tsidrGenericMax = 16,

    tsidrFileRenameFile             = 17,
    tsidrFileChangeEngineFileType   = 18,

    tsidrEngineInstBeginRedo        = 65,
    tsidrEngineInstBeginUndo        = 66,
    tsidrEngineInstBeginDo          = 67,
    tsidrEngineFmpDbHdr1st          = 68,
};

class COSEventTraceIdCheck
{
private:
    BOOL                m_fTracedStationIdentification;
#ifdef DEBUG
    OSEventTraceGUID    m_etguidCheck;
#endif

public:
    COSEventTraceIdCheck()
    {
        m_fTracedStationIdentification = fFalse;
        OnDebug( m_etguidCheck = (OSEventTraceGUID)-1 );
    }

    template< OSEventTraceGUID etguid >
    INLINE BOOL FAnnounceTime( const TraceStationIdentificationReason tsidr )
    {
        Expected( etguid );
        Assert( m_etguidCheck == (OSEventTraceGUID)-1 || m_etguidCheck == etguid );
        OnDebug( m_etguidCheck = etguid );


        if ( tsidr != tsidrPulseInfo &&
                tsidr != tsidrOpenInit )
        {
            return fTrue;
        }


        const BOOL fTracingOn = FOSEventTraceEnabled< etguid >();



        if ( !fTracingOn && m_fTracedStationIdentification )
        {
            AtomicCompareExchange( (LONG*)&m_fTracedStationIdentification, (LONG)fTrue, (LONG)fFalse );
        }
        if ( fTracingOn && !m_fTracedStationIdentification )
        {
            BOOL fStationIDd = (BOOL) AtomicCompareExchange( (LONG*)&m_fTracedStationIdentification, (LONG)fFalse, (LONG)fTrue );
            if ( fStationIDd == fFalse )
            {
                return fTrue;
            }
        }

        return tsidr == tsidrOpenInit;
    }
    
};


template INLINE BOOL FOSEventTraceEnabled< _etguidCacheRequestPage >();
template INLINE BOOL FOSEventTraceEnabled< _etguidCacheMemoryUsage >();

template INLINE BOOL FOSEventTraceEnabled< _etguidInstStationId >();
template INLINE BOOL FOSEventTraceEnabled< _etguidDiskStationId >();
template INLINE BOOL FOSEventTraceEnabled< _etguidFileStationId >();
template INLINE BOOL FOSEventTraceEnabled< _etguidSysStationId >();
template INLINE BOOL FOSEventTraceEnabled< _etguidIsamDbfilehdrInfo >();
template INLINE BOOL FOSEventTraceEnabled< _etguidFmpStationId >();

template INLINE BOOL COSEventTraceIdCheck::FAnnounceTime< _etguidInstStationId >( const TraceStationIdentificationReason tsidr );
template INLINE BOOL COSEventTraceIdCheck::FAnnounceTime< _etguidDiskStationId >( const TraceStationIdentificationReason tsidr );
template INLINE BOOL COSEventTraceIdCheck::FAnnounceTime< _etguidFileStationId >( const TraceStationIdentificationReason tsidr );
template INLINE BOOL COSEventTraceIdCheck::FAnnounceTime< _etguidSysStationId >( const TraceStationIdentificationReason tsidr );
template INLINE BOOL COSEventTraceIdCheck::FAnnounceTime< _etguidIsamDbfilehdrInfo >( const TraceStationIdentificationReason tsidr );
template INLINE BOOL COSEventTraceIdCheck::FAnnounceTime< _etguidFmpStationId >( const TraceStationIdentificationReason tsidr );

#endif

