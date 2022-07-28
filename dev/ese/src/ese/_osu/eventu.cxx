// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"

#include "esestd.hxx"

#include <malloc.h>

WCHAR WchReportInstState( const INST * const pinst )
{
    switch( ( pinst == NULL ) ? -1 : pinst->m_perfstatusEvent )
    {
    case -1:                            return 'G';  //  Global
    case 0:                             return 'P';  //  Pre-recovery (sysinit or os init)
    case perfStatusRecoveryRedo:        return 'R';
    case perfStatusRecoveryUndo:        return 'U';
    case perfStatusRuntime:             return 'D';  //  Do-time
    case perfStatusTerm:                return 'T';
    case perfStatusError:               Expected( fFalse ); return 'E';  //  note we block perfStatusError at set time, so should not see.
    default:                            AssertSz( fFalse, "Unknown perfstatus / eventing state value = %d", pinst->m_perfstatusEvent );
    }
    return '-'; //  actually unknown!?
}

CBasicProcessInfo::CBasicProcessInfo( const INST * const pinst )
    {
#ifdef ESENT
    //  I (SOMEONE) believe that DwUtilImageVersionMajor() and DwUtilImageBuildNumberMajor() reports false numbers for various
    //  poorly thought out / invalid (*IMNHO*) reasons, would rather not report anything at all than bad / false numbers.
    OSStrCbFormatW( m_wszStateInfoEx, sizeof(m_wszStateInfoEx), L"%I32u,%wc,%I32u", DwUtilProcessId(), WchReportInstState( pinst ), OpTraceContext() );
#else
    //  Format of Bu.il.dNum.ber copied from FOSSysinfoPreinit() for Exchange.
    OSStrCbFormatW( m_wszStateInfoEx, sizeof(m_wszStateInfoEx), L"%I32u,%wc,%I32u,%02I32u.%02I32u.%04I32u.%03I32u", DwUtilProcessId(), WchReportInstState( pinst ), OpTraceContext(),
        DwUtilImageVersionMajor(), DwUtilImageVersionMinor(), DwUtilImageBuildNumberMajor(), DwUtilImageBuildNumberMinor() );
#endif
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

    //  validate IN args

    Assert( catid < MAC_CATEGORY );
    Assert( DWORD( WORD( catid ) ) == catid );
    Assert( DWORD( WORD( cString + 2 ) ) == cString + 2 );

    //  get event source and process ID strings

    rgpsz[ 0 ] = pinst != pinstNil ? SzParam( pinst, JET_paramEventSource ) : L"";
    if ( !rgpsz[ 0 ][ 0 ] )
    {
        rgpsz[ 0 ] = WszUtilProcessFriendlyName();
    }

    CBasicProcessInfo procinfo( pinst );
    rgpsz[1] = procinfo.Wsz();

    WCHAR wszDisplayName[JET_cbFullNameMost + 3]; // 3 = sizof(": ") + 1
    /*
    IF      m_wszDisplayName exists
    THEN    display m_wszDisplayName
    ELSE IF     m_wszInstanceName exists
         THEN   display m_wszInstanceName
    ELSE    display nothing
    */
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

    //  copy in remaining strings

    // size should be 0 if the pointer is NULL
    AssertPREFIX( rgpszString != NULL || cString == 0 );

    // avoid the call if nothing to be done
    if ( cString > 0 )
    {
        Assert( rgpszString != NULL );
        UtilMemCpy( &rgpsz[3], rgpszString, sizeof(const WCHAR*) * cString );
    }


    //  Get event source key in case we need to open the event source.

    const WCHAR* wszEventSourceKey;
    wszEventSourceKey = pinst != pinstNil ? SzParam( pinst, JET_paramEventSourceKey ) : L"";
    if ( !wszEventSourceKey[ 0 ] )
    {
        wszEventSourceKey = WszUtilImageVersionName();
    }

    //  the event log isn't open

    OSEventReportEvent(
            wszEventSourceKey,
            eventfacilityOsDiagTracking | eventfacilityRingBufferCache | eventfacilityOsEventTrace | eventfacilityOsTrace |
                //  the optional eventfacility ... 
                ( fSuppressReportOsEvent ? eventfacilityNone : eventfacilityReportOsEvent ),
            type,
            catid,
            msgid,
            cString + cAdditionalParam,
            rgpsz,
            cbRawData,
            pvRawData );

}


//  reports error event in the context of a category and optionally in the
//  context of a MessageId.  If the MessageId is 0, then a MessageId is chosen
//  based on the given error code.  If MessageId is !0, then the appropriate
//  event is reported

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
//  the following code is dead, as the
//  assert above affirms
//  if ( 0 == msgid && JET_errDiskFull == err )
//      {
//      msgid = DISK_FULL_ERROR_ID;
//      rgszT[0] = pinst->m_plog->m_szJetLog;
//      }

    UtilReportEvent( eventError, catid, msgid, 1, rgszT, 0, NULL, pinst );
}


//  terminate event subsystem

void OSUEventTerm()
{
    //  nop
}


//  init event subsystem

ERR ErrOSUEventInit()
{
    //  nop

    return JET_errSuccess;
}

