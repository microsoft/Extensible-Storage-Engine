// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"
#include "esestd.hxx"

// The current layer is high enough to understand INST internal.
// Extracing m_wszInstanceName and m_wszDisplayName was
// not possible down inside OS layer
#if defined( USE_HAPUBLISH_API )
void OSUHAPublishEvent_(
    HaDbFailureTag haTag,
    const INST* pinst,
    DWORD dwComponent,
    HaDbIoErrorCategory haCategory,
    const WCHAR* wszFilename,
    unsigned _int64 qwOffset,
    DWORD cbSize,
    DWORD dwEventId,
    DWORD cParameter,
    const WCHAR** rgwszParameter )
{
    // failure events need not be published if there is no instance
    if ( pinstNil != pinst && UlParam( pinst, JET_paramEnableHaPublish ) )
    {
        OSUHAPublishEventImpl(  haTag,
                                pinst->m_wszInstanceName,
                                pinst->m_wszDisplayName,
                                dwComponent,
                                haCategory,
                                wszFilename,
                                qwOffset,
                                cbSize,
                                dwEventId,
                                cParameter,
                                rgwszParameter );
    }
}

void OSUHAEmitFailureTag_(
    const INST* const       pinst,
    const HaDbFailureTag    haTag,
    const WCHAR* const      wszGuid,
    const WCHAR* const      wszAdditional )
{
    const INST*         pinstActual     = pinst;
    CCriticalSection *  pcritInstActual = NULL;
    BOOL                fEmit           = fTrue;
    const size_t        cwsz            = 4;
    const WCHAR*        rgwsz[ cwsz ]   = { 0 };
    size_t              iwsz            = 0;

    //  if the instance pointer is NULL and this is a Memory event then we will
    //  pick a victim instance that we feel is most likely to have tipped us over
    //  the edge
    //
    if ( FOSLayerUp() && !pinstActual && haTag == HaDbFailureTagMemory )
    {
        //  scan all instances to find the best victim
        //
        INST*   pinstVictim     = NULL;
        size_t  ipinstVictim    = g_cpinstMax;
        __int64 ftInitVictim    = 0;
        
        for ( size_t ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
        {
            extern CRITPOOL< INST* > g_critpoolPinstAPI;
            CCriticalSection * const pcritInstCandidate = &g_critpoolPinstAPI.Crit( &g_rgpinst[ ipinst ] );
            const BOOL fAcquired = pcritInstCandidate->FTryEnter();
        
            INST * const pinstCandidate = g_rgpinst[ ipinst ];

            //  if we couldn't acquire the lock, skip it
            //
            if ( !fAcquired )
            {
            }

            //  this entry doesn't contain an instance, skip it
            //
            else if ( !pinstCandidate )
            {
            }
            
            //  this instance doesn't better our current best victim, skip it
            //
            //  note our very sophisticated heuristic where the most recent
            //  instance to be initialized wins
            //
            else if (   !pinstCandidate->m_ftInit || pinstCandidate->m_ftInit < ftInitVictim ||
                        !pinstCandidate->m_wszInstanceName || !pinstCandidate->m_wszInstanceName[ 0 ] ||
                        !pinstCandidate->m_wszDisplayName || !pinstCandidate->m_wszDisplayName[ 0 ] )
                        
            {
            }
            
            //  this instance is our best victim so far, remember it
            //
            else
            {
                pinstVictim     = pinstCandidate;
                ipinstVictim    = ipinst;
                ftInitVictim    = pinstCandidate->m_ftInit;
            }

            if ( fAcquired )
            {
                pcritInstCandidate->Leave();
            }
        }

        //  try to reacquire our best victim.  if it is gone then mission accomplished already!  ;-)
        //
        if ( pinstVictim )
        {
            extern CRITPOOL< INST* > g_critpoolPinstAPI;
            pcritInstActual = &g_critpoolPinstAPI.Crit( &g_rgpinst[ ipinstVictim ] );
            const BOOL fAcquired = pcritInstActual->FTryEnter();
            
            pinstActual = g_rgpinst[ ipinstVictim ];

            if ( !fAcquired || pinstActual != pinstVictim || pinstActual->m_ftInit != ftInitVictim )
            {
                pinstActual = NULL;
                if ( fAcquired )
                {
                    pcritInstActual->Leave();
                }
                pcritInstActual = NULL;
            }
        }
    }

    //  if the instance pointer is NULL then do not emit an event
    //
    if ( !pinstActual )
    {
        fEmit = fFalse;
    }

    //  do not emit events or eventlogs if HA events aren't enabled
    //
    if ( pinstActual && !UlParam( pinstActual, JET_paramEnableHaPublish ) )
    {
        fEmit = fFalse;
    }

    //  do not emit events if the instance name or display name are empty
    //
    if (    pinstActual &&
            (   !pinstActual->m_wszInstanceName || !pinstActual->m_wszInstanceName[ 0 ] ||
                !pinstActual->m_wszDisplayName || !pinstActual->m_wszDisplayName[ 0 ] ) )
    {
        fEmit = fFalse;
    }

    //  do not emit "NoOp" events
    //
    if ( haTag == HaDbFailureTagNoOp )
    {
        fEmit = fFalse;
    }

    //  do not emit events without a valid GUID
    //
    if ( !wszGuid || !wszGuid[ 0 ] )
    {
        fEmit = fFalse;
    }

    //  if this is an unrecognized failure tag then do not emit the event
    //
    INT msgidOffset = (INT)haTag - (INT)HaDbFailureTagNoOp;
    if (    NOOP_FAILURE_TAG_ID + msgidOffset <= NOOP_FAILURE_TAG_ID ||
            NOOP_FAILURE_TAG_ID + msgidOffset > MAX_FAILURE_TAG_ID ||
            HA_NOOP_FAILURE_TAG_ID + msgidOffset <= HA_NOOP_FAILURE_TAG_ID ||
            HA_NOOP_FAILURE_TAG_ID + msgidOffset > HA_MAX_FAILURE_TAG_ID )
    {
        fEmit = fFalse;
    }

    //  we should emit this event
    //
    if ( fEmit )
    {
        //  log a generic event to the eventlog for this failure item so that we
        //  are sure to leave some evidence of why we emitted this failure item
        //
        rgwsz[ iwsz++ ] = pinstActual->m_wszDisplayName;
        rgwsz[ iwsz++ ] = pinstActual->m_wszInstanceName;
        rgwsz[ iwsz++ ] = wszGuid;
        rgwsz[ iwsz++ ] = wszAdditional;
        Assert( iwsz <= cwsz );
        
        UtilReportEvent(    eventError,
                            FAILURE_ITEM_CATEGORY,
                            NOOP_FAILURE_TAG_ID + msgidOffset,
                            iwsz,
                            rgwsz,
                            0,
                            NULL,
                            pinstActual);
        
        //  emit the failure item to HA, also with an eventlog entry
        //
        OSUHAPublishEventImpl(  haTag,
                                pinstActual->m_wszInstanceName,
                                pinstActual->m_wszDisplayName,
                                HA_FAILURE_ITEM_CATEGORY,
                                HaDbIoErrorNone,
                                NULL,
                                0,
                                0,
                                HA_NOOP_FAILURE_TAG_ID + msgidOffset,
                                iwsz,
                                rgwsz );
    }

    //  cleanup
    //
    if ( pcritInstActual )
    {
        pcritInstActual->Leave();
    }
}
#endif

