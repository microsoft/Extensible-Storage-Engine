// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  This file gives the oslite.lib an indirection level for a few functions
//  the OS Layer expects to link against.  These functions are technically
//  layering violations that we do not have time to fix properly.

#include "osstd.hxx"


void UtilReportEvent(
    const EEventType    type,
    const CategoryId    catid,
    const MessageId     msgid,
    const DWORD         cString,
    const WCHAR *       rgpszString[],
    const DWORD         cbRawData,
    void *              pvRawData,
    const INST *        pinst,
    const LONG          lEventLoggingLevel )    //  1==JET_EventLoggingLevelMin
{
    return;
}

#if defined( USE_HAPUBLISH_API )
void OSUHAEmitFailureTag_(
    const INST* const       pinst,
    const HaDbFailureTag    haTag,
    const WCHAR* const      wszGuid,
    const WCHAR* const      wszAdditional ) {}
#endif






BOOL FINSTSomeInitialized(void)
{
    return fFalse;
}



#ifndef MINIMAL_FUNCTIONALITY
void JetErrorToString( JET_ERR err, const char **szError, const char **szErrorText )
{
    //*szError = szUnknownError;
    //*szErrorText = szUnknownError;
    AssertSz( false, "NYI" );
}
#endif



BOOL FOSRMPreinit(void)
{
    return fTrue;
}
ERR ErrOSRMInit(void)
{
    return JET_errSuccess;
}
void OSRMTerm(void)
{
    //  nop
}
void OSRMPostterm(void)
{
    //  nop
}








// Used by retail version of AssertFail to figure out whether to crash on firewall on retail builds
ULONG_PTR UlParam( const INST* const, const ULONG )
{
    return 0;
}

