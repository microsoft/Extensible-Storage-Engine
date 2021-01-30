// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"
#include "esestd.hxx"

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

    if ( FOSLayerUp() && !pinstActual && haTag == HaDbFailureTagMemory )
    {
        INST*   pinstVictim     = NULL;
        size_t  ipinstVictim    = g_cpinstMax;
        __int64 ftInitVictim    = 0;
        
        for ( size_t ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
        {
            extern CRITPOOL< INST* > g_critpoolPinstAPI;
            CCriticalSection * const pcritInstCandidate = &g_critpoolPinstAPI.Crit( &g_rgpinst[ ipinst ] );
            const BOOL fAcquired = pcritInstCandidate->FTryEnter();
        
            INST * const pinstCandidate = g_rgpinst[ ipinst ];

            if ( !fAcquired )
            {
            }

            else if ( !pinstCandidate )
            {
            }
            
            else if (   !pinstCandidate->m_ftInit || pinstCandidate->m_ftInit < ftInitVictim ||
                        !pinstCandidate->m_wszInstanceName || !pinstCandidate->m_wszInstanceName[ 0 ] ||
                        !pinstCandidate->m_wszDisplayName || !pinstCandidate->m_wszDisplayName[ 0 ] )
                        
            {
            }
            
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

    if ( !pinstActual )
    {
        fEmit = fFalse;
    }

    if ( pinstActual && !UlParam( pinstActual, JET_paramEnableHaPublish ) )
    {
        fEmit = fFalse;
    }

    if (    pinstActual &&
            (   !pinstActual->m_wszInstanceName || !pinstActual->m_wszInstanceName[ 0 ] ||
                !pinstActual->m_wszDisplayName || !pinstActual->m_wszDisplayName[ 0 ] ) )
    {
        fEmit = fFalse;
    }

    if ( haTag == HaDbFailureTagNoOp )
    {
        fEmit = fFalse;
    }

    if ( !wszGuid || !wszGuid[ 0 ] )
    {
        fEmit = fFalse;
    }

    INT msgidOffset = (INT)haTag - (INT)HaDbFailureTagNoOp;
    if (    NOOP_FAILURE_TAG_ID + msgidOffset <= NOOP_FAILURE_TAG_ID ||
            NOOP_FAILURE_TAG_ID + msgidOffset > MAX_FAILURE_TAG_ID ||
            HA_NOOP_FAILURE_TAG_ID + msgidOffset <= HA_NOOP_FAILURE_TAG_ID ||
            HA_NOOP_FAILURE_TAG_ID + msgidOffset > HA_MAX_FAILURE_TAG_ID )
    {
        fEmit = fFalse;
    }

    if ( fEmit )
    {
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

    if ( pcritInstActual )
    {
        pcritInstActual->Leave();
    }
}
#endif

