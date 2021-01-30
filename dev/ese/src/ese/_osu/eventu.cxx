// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"

#include "esestd.hxx"

#include <malloc.h>

WCHAR WchReportInstState( const INST * const pinst )
{
    switch( ( pinst == NULL ) ? -1 : pinst->m_perfstatusEvent )
    {
    case -1:                            return 'G';
    case 0:                             return 'P';
    case perfStatusRecoveryRedo:        return 'R';
    case perfStatusRecoveryUndo:        return 'U';
    case perfStatusRuntime:             return 'D';
    case perfStatusTerm:                return 'T';
    case perfStatusError:               Expected( fFalse ); return 'E';
    default:                            AssertSz( fFalse, "Unknown perfstatus / eventing state value = %d", pinst->m_perfstatusEvent );
    }
    return '-';
}

void UtilReportEvent(
    const EEventType    type,
    const CategoryId    catid,
    const MessageId     msgid,
    const DWORD         cString,
    const WCHAR*        rgpszString[],
    const DWORD         cbRawData,
    void*               pvRawData,
    const INST*         pinst,
    const LONG          lEventLoggingLevel )
{
    const UINT cAdditionalParam = 3;
    BOOL fSuppressReportOsEvent = fFalse;

    if ( pinst != pinstNil && (ULONG_PTR)lEventLoggingLevel > UlParam( pinst, JET_paramEventLoggingLevel ) )
    {
        fSuppressReportOsEvent = fTrue;
    }
    if ( pinst != pinstNil && eventInformation == type && BoolParam( pinst, JET_paramNoInformationEvent ) )
    {
        fSuppressReportOsEvent = fTrue;
    }

    const WCHAR** rgpsz = (const WCHAR**)_alloca( sizeof( const WCHAR* ) * ( cString + cAdditionalParam ) );


    Assert( catid < MAC_CATEGORY );
    Assert( DWORD( WORD( catid ) ) == catid );
    Assert( DWORD( WORD( cString + 2 ) ) == cString + 2 );


    rgpsz[ 0 ] = pinst != pinstNil ? SzParam( pinst, JET_paramEventSource ) : L"";
    if ( !rgpsz[ 0 ][ 0 ] )
    {
        rgpsz[ 0 ] = WszUtilProcessFriendlyName();
    }

    WCHAR wszStateInfoEx[16 + 2 + 4 + 20];
#ifdef ESENT
    OSStrCbFormatW( wszStateInfoEx, sizeof(wszStateInfoEx), L"%I32u,%wc,%I32u", DwUtilProcessId(), WchReportInstState( pinst ), OpTraceContext() );
#else
    OSStrCbFormatW( wszStateInfoEx, sizeof(wszStateInfoEx), L"%I32u,%wc,%I32u,%02I32u.%02I32u.%04I32u.%03I32u", DwUtilProcessId(), WchReportInstState( pinst ), OpTraceContext(),
        DwUtilImageVersionMajor(), DwUtilImageVersionMinor(), DwUtilImageBuildNumberMajor(), DwUtilImageBuildNumberMinor() );
#endif
    rgpsz[1] = wszStateInfoEx;

    WCHAR wszDisplayName[JET_cbFullNameMost + 3];
    
    if ( pinst && pinst->m_wszDisplayName )
    {
        OSStrCbFormatW( wszDisplayName, sizeof( wszDisplayName ), L"%s: ", pinst->m_wszDisplayName );
    }
    else if ( pinst && pinst->m_wszInstanceName )
    {
        OSStrCbFormatW( wszDisplayName, sizeof( wszDisplayName ), L"%s: ", pinst->m_wszInstanceName );
    }
    else
    {
        OSStrCbFormatW( wszDisplayName, sizeof( wszDisplayName ), L"" );
    }
    rgpsz[2] = wszDisplayName;


    AssertPREFIX( rgpszString != NULL || cString == 0 );

    if ( cString > 0 )
    {
        Assert( rgpszString != NULL );
        UtilMemCpy( &rgpsz[3], rgpszString, sizeof(const WCHAR*) * cString );
    }



    const WCHAR* wszEventSourceKey;
    wszEventSourceKey = pinst != pinstNil ? SzParam( pinst, JET_paramEventSourceKey ) : L"";
    if ( !wszEventSourceKey[ 0 ] )
    {
        wszEventSourceKey = WszUtilImageVersionName();
    }


    OSEventReportEvent(
            wszEventSourceKey,
            eventfacilityOsDiagTracking | eventfacilityRingBufferCache | eventfacilityOsEventTrace | eventfacilityOsTrace |
                ( fSuppressReportOsEvent ? eventfacilityNone : eventfacilityReportOsEvent ),
            type,
            catid,
            msgid,
            cString + cAdditionalParam,
            rgpsz,
            cbRawData,
            pvRawData );

}



void UtilReportEventOfError(
    const CategoryId    catid,
    const MessageId     msgid,
    const ERR           err,
    const INST *        pinst )
{
    WCHAR               szT[16];
    const WCHAR *       rgszT[1];

    OSStrCbFormatW( szT, sizeof( szT ), L"%d", err );
    rgszT[0] = szT;

    Assert( 0 != msgid );

    UtilReportEvent( eventError, catid, msgid, 1, rgszT, 0, NULL, pinst );
}



void OSUEventTerm()
{
}



ERR ErrOSUEventInit()
{

    return JET_errSuccess;
}

