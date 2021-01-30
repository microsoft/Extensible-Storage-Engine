// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSU_EVENT_HXX_INCLUDED
#define _OSU_EVENT_HXX_INCLUDED




class INST;

void UtilReportEvent(
    const EEventType    type,
    const CategoryId    catid,
    const MessageId     msgid,
    const DWORD         cString,
    const WCHAR *       rgpszString[],
    const DWORD         cbRawData = 0,
    void *              pvRawData = NULL,
    const INST *        pinst = NULL,
    const LONG          lEventLoggingLevel = 1 );
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
        Assert( dwContextKey != 0 );
        if ( rgdwContextKeys[ _countof(rgdwContextKeys) - 1 ] != 0 )
        {
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
                return fFalse;
            }
            ictx++;
        }
        return ictx < _countof(rgdwContextKeys) && !fLost;
    }
};



ERR ErrOSUEventInit();


void OSUEventTerm();


#endif

