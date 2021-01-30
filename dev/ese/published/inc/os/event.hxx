// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_EVENT_HXX_INCLUDED
#define _OS_EVENT_HXX_INCLUDED



typedef DWORD CategoryId;
typedef DWORD MessageId;

enum EEventType
{
    eventSuccess = 0,
    eventError = 1,
    eventWarning = 2,
    eventInformation = 4,
};

enum EEventFacility
{
    eventfacilityNone               = 0x00,
    eventfacilityReportOsEvent      = 0x01,
    eventfacilityOsDiagTracking     = 0x02,
    eventfacilityRingBufferCache    = 0x04,
    eventfacilityOsEventTrace       = 0x08,
    eventfacilityOsTrace            = 0x10,
};
DEFINE_ENUM_FLAG_OPERATORS_BASIC( EEventFacility );


void OSEventReportEvent(
        const WCHAR *       szEventSourceKey,
        const EEventFacility eventfacility,
        const EEventType    type,
        const CategoryId    catid,
        const MessageId     msgid,
        const DWORD         cString,
        const WCHAR *       rgpszString[],
        const DWORD         cDataSize = 0,
        void *              pvRawData = NULL );

#ifdef OS_LAYER_VIOLATIONS
#include "eventu.hxx"

#ifdef ESENT
#include "jetmsg.h"
#else
#include "jetmsgex.h"
#endif

#endif

#endif

