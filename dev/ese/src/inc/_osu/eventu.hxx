// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSU_EVENT_HXX_INCLUDED
#define _OSU_EVENT_HXX_INCLUDED


//  Event Logging

//  reports error event in the context of a category and optionally
//  in the context of a IDEvent.  If IDEvent is 0, then an IDEvent
//  is chosen based on error code.  If IDEvent is !0, then the
//  appropriate event is reported.

class INST;

WCHAR WchReportInstState( const INST * const pinst );

class CBasicProcessInfo
{
    WCHAR m_wszStateInfoEx[16 + 2 + 4 + 20];  //  PID + Recovery/Undo/Do-time/Term mode + ApiOpCode + Bu.il.dNum.ber

public:

    CBasicProcessInfo( const INST * const pinst );

    const WCHAR * Wsz()
    {
        return m_wszStateInfoEx;
    }
};

void UtilReportEvent(
    const EEventType    type,
    const CategoryId    catid,
    const MessageId     msgid,
    const DWORD         cString,
    const WCHAR *       rgpszString[],
    const DWORD         cbRawData = 0,
    void *              pvRawData = NULL,
    const INST *        pinst = NULL,
    const LONG          lEventLoggingLevel = 1 );   //  1==JET_EventLoggingLevelMin
void UtilReportEventOfError(
    const CategoryId    catid,
    const MessageId     msgid,
    const ERR           err,
    const INST *        pinst );

class CLimitedEventSuppressor
{
private:
    DWORD       rgdwContextKeys[4];

    BOOL FAlreadyLogged( const DWORD dwContextKey ) const
    {
        C_ASSERT( _countof(rgdwContextKeys) == 4 );
        return rgdwContextKeys[0] == dwContextKey ||
                 rgdwContextKeys[1] == dwContextKey ||
                 rgdwContextKeys[2] == dwContextKey ||
                 rgdwContextKeys[3] == dwContextKey;
    }
    
public:
    void Reset()
    {
        memset( rgdwContextKeys, 0, _cbrg( rgdwContextKeys ) );
    }
    CLimitedEventSuppressor()
    {
        Reset();
    }
    BOOL FNeedToLog( const DWORD dwContextKey )
    {
        Assert( dwContextKey != 0 ); // or stuff will break down.
        if ( rgdwContextKeys[ _countof(rgdwContextKeys) - 1 ] != 0 )
        {
            //  Array is full, we can no longer suppress anyone, suppress all subsequent events.
            return fFalse;
        }

        if ( FAlreadyLogged( dwContextKey ) )
        {
            return fFalse;
        }
        
        ULONG ictx = 0;
        BOOL fLost = fTrue;
        while( ictx < _countof(rgdwContextKeys) &&
                ( 0 != ( fLost = AtomicCompareExchange( (LONG*)&(rgdwContextKeys[ictx]), (LONG)0, (LONG)dwContextKey ) ) ) )
        {
            if ( rgdwContextKeys[ictx] == dwContextKey )
            {
                //  We have lost a race with another thread ...
                return fFalse;
            }
            //  whew the update wasn't our context, try next slot
            ictx++;
        }
        return ictx < _countof(rgdwContextKeys) && !fLost;
    }
};


//  init event subsystem

ERR ErrOSUEventInit();

//  terminate event subsystem

void OSUEventTerm();


#endif  //  _OSU_EVENT_HXX_INCLUDED

