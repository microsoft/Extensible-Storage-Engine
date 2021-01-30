// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


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
    const LONG          lEventLoggingLevel )
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
}
void OSRMPostterm(void)
{
}








ULONG_PTR UlParam( const INST* const, const ULONG )
{
    return 0;
}

