// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "_bfconst.hxx"


template< class I >
class TCacheTelemetry
    :   public I
{
    public:

        void Miss(  _In_ const ICacheTelemetry::FileNumber  filenumber,
                    _In_ const ICacheTelemetry::BlockNumber blocknumber,
                    _In_ const BOOL                         fRead,
                    _In_ const BOOL                         fCacheIfPossible ) override;

        void Hit(   _In_ const ICacheTelemetry::FileNumber  filenumber,
                    _In_ const ICacheTelemetry::BlockNumber blocknumber,
                    _In_ const BOOL                         fRead,
                    _In_ const BOOL                         fCacheIfPossible ) override;

        void Update(    _In_ const ICacheTelemetry::FileNumber  filenumber, 
                        _In_ const ICacheTelemetry::BlockNumber blocknumber ) override;

        void Write( _In_ const ICacheTelemetry::FileNumber  filenumber, 
                    _In_ const ICacheTelemetry::BlockNumber blocknumber,
                    _In_ const BOOL                         fReplacementPolicy ) override;

        void Evict( _In_ const ICacheTelemetry::FileNumber  filenumber, 
                    _In_ const ICacheTelemetry::BlockNumber blocknumber,
                    _In_ const BOOL                         fReplacementPolicy ) override;

    private:

        BFRequestTraceFlags BfrtfReference( _In_ const BOOL fRead, _In_ const BOOL fCacheIfPossible );
};

template< class I >
void TCacheTelemetry<I>::Miss(  _In_ const ICacheTelemetry::FileNumber  filenumber, 
                                _In_ const ICacheTelemetry::BlockNumber blocknumber,
                                _In_ const BOOL                         fRead,
                                _In_ const BOOL                         fCacheIfPossible )
{
    if ( filenumber == filenumberInvalid )
    {
        return;
    }

    GetCurrUserTraceContext getutc;

    ETCacheCachePage(   TickOSTimeCurrent(),
                        (DWORD)filenumber,
                        (DWORD)blocknumber + 1,
                        0,
                        0,
                        100,
                        BfrtfReference( fRead, fCacheIfPossible ),
                        getutc->context.nClientType );
}

template< class I >
void TCacheTelemetry<I>::Hit(   _In_ const ICacheTelemetry::FileNumber  filenumber, 
                                _In_ const ICacheTelemetry::BlockNumber blocknumber,
                                _In_ const BOOL                         fRead,
                                _In_ const BOOL                         fCacheIfPossible )
{
    if ( filenumber == filenumberInvalid )
    {
        return;
    }

    GetCurrUserTraceContext getutc;

    ETCacheRequestPage( TickOSTimeCurrent(),
                        (DWORD)filenumber,
                        (DWORD)blocknumber + 1,
                        0,
                        0,
                        0,
                        0,
                        100,
                        BfrtfReference( fRead, fCacheIfPossible ),
                        getutc->context.nClientType );
}

template< class I >
void TCacheTelemetry<I>::Update(    _In_ const ICacheTelemetry::FileNumber  filenumber, 
                                    _In_ const ICacheTelemetry::BlockNumber blocknumber )
{
    if ( filenumber == filenumberInvalid )
    {
        return;
    }

    GetCurrUserTraceContext getutc;
    const TraceContext* petc = PetcTLSGetEngineContext();

    ETCacheDirtyPage(   TickOSTimeCurrent(),
                        (DWORD)filenumber,
                        (DWORD)blocknumber + 1,
                        0,
                        0,
                        0,
                        0,
                        getutc->context.dwUserID,
                        getutc->context.nOperationID,
                        getutc->context.nOperationType,
                        getutc->context.nClientType,
                        getutc->context.fFlags,
                        getutc->dwCorrelationID,
                        petc->iorReason.Iorp(),
                        petc->iorReason.Iors(),
                        petc->iorReason.Iort(),
                        petc->iorReason.Ioru(),
                        petc->iorReason.Iorf(),
                        petc->nParentObjectClass );
}

template< class I >
void TCacheTelemetry<I>::Write( _In_ const ICacheTelemetry::FileNumber  filenumber, 
                                _In_ const ICacheTelemetry::BlockNumber blocknumber,
                                _In_ const BOOL                         fReplacementPolicy )
{
    if ( filenumber == filenumberInvalid )
    {
        return;
    }

    GetCurrUserTraceContext getutc;
    const TraceContext* petc = PetcTLSGetEngineContext();

    ETCacheWritePage(   TickOSTimeCurrent(),
                        (DWORD)filenumber,
                        (DWORD)blocknumber + 1,
                        0,
                        0,
                        0,
                        getutc->context.dwUserID,
                        getutc->context.nOperationID,
                        getutc->context.nOperationType,
                        getutc->context.nClientType,
                        getutc->context.fFlags,
                        getutc->dwCorrelationID,
                        fReplacementPolicy ? iorpBFAvailPool : iorpBFDatabaseFlush,
                        petc->iorReason.Iors(),
                        petc->iorReason.Iort(),
                        petc->iorReason.Ioru(),
                        petc->iorReason.Iorf(),
                        petc->nParentObjectClass );
}

template< class I >
void TCacheTelemetry<I>::Evict( _In_ const ICacheTelemetry::FileNumber  filenumber,
                                _In_ const ICacheTelemetry::BlockNumber blocknumber,
                                _In_ const BOOL                         fReplacementPolicy )
{
    if ( filenumber == filenumberInvalid )
    {
        return;
    }

    ETCacheEvictPage(   TickOSTimeCurrent(),
                        (DWORD)filenumber,
                        (DWORD)blocknumber + 1,
                        fTrue,
                        JET_errSuccess, 
                        fReplacementPolicy ? bfefReasonAvailPool : bfefReasonPurgePage,
                        100 );
}

template< class I >
BFRequestTraceFlags TCacheTelemetry<I>::BfrtfReference( _In_ const BOOL fRead, _In_ const BOOL fCacheIfPossible )
{
    return (BFRequestTraceFlags)(   bfrtfUseHistory |
                                    ( fRead ? bfrtfNone : bfrtfNewPage ) |
                                    ( fCacheIfPossible ? bfrtfNone : bfrtfNoTouch ) );
}


class CCacheTelemetry : public TCacheTelemetry<ICacheTelemetry>
{
    public:

        CCacheTelemetry()
            : TCacheTelemetry<ICacheTelemetry>()
        {
        }

        virtual ~CCacheTelemetry() {}
};
