// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#if !defined( ESENT ) && !defined( MINIMAL_FUNCTIONALITY )
#define USE_HAPUBLISH_API
#endif

#if defined( USE_HAPUBLISH_API )
#include <guiddef.h>
#include <ExDbFailureItem.h>
#include "exdbmsg_ese.h"

#define Ese2HaId( id ) ( HADBFAILURE_EVENT_RANGE_START_ESE + ( id ) )

#define OSUHAPublishEvent( p0, p1, p2, p3, p4, p5, p6, p7, p8, p9 ) \
    OSUHAPublishEvent_( p0, p1, p2, p3, p4, p5, p6, p7, p8, p9 )

void OSUHAPublishEvent_(
    HaDbFailureTag      haTag,
    const INST*         pinst,
    DWORD               dwComponent,
    HaDbIoErrorCategory haCategory,
    const WCHAR*        wszFilename = NULL,
    unsigned _int64     qwOffset = 0,
    DWORD               cbSize = 0,
    DWORD               dwEventId = 0,
    DWORD               cParameter = 0,
    const WCHAR**       rgwszParameter = NULL );

void OSUHAPublishEventImpl(
    HaDbFailureTag      haTag,
    const WCHAR*        wszDbGuid,
    const WCHAR*        wszDbInstanceName,
    DWORD               dwComponent,
    HaDbIoErrorCategory haCategory,
    const WCHAR*        wszFilename,
    unsigned _int64     qwOffset,
    DWORD               cbSize,
    DWORD               dwEventId,
    DWORD               cParameter,
    const WCHAR**       rgwszParameter );

#define OSUHAEmitFailureTag( pinst, tag, wszGuid ) \
    OSUHAEmitFailureTag_( pinst, tag, wszGuid )
#define OSUHAEmitFailureTagEx( pinst, tag, wszGuid, wszAdditional ) \
        OSUHAEmitFailureTag_( pinst, tag, wszGuid, wszAdditional )

void OSUHAEmitFailureTag_(
    const INST* const       pinst,
    const HaDbFailureTag    haTag,
    const WCHAR* const      wszGuid,
    const WCHAR* const      wszAdditional = NULL );
#else
#define OSUHAPublishEvent( p0, p1, p2, p3, p4, p5, p6, p7, p8, p9 )
#define OSUHAEmitFailureTag( pinst, tag, wszGuid )
#define OSUHAEmitFailureTagEx( pinst, tag, wszGuid, wszAdditional )
#endif


bool FUtilHaPublishInit();

