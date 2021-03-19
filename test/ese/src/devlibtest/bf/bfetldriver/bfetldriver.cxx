// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#include <tchar.h>
#include "os.hxx"

#ifdef BUILD_ENV_IS_NT
#include "esent_x.h"
#else
#include "jet.h"
#endif

#include "bfreqs.hxx"       // requisite thunked out types
#include "_bfconst.hxx"     // BF constants and simple types we need
#include "bfftl.hxx"        // BF trace data and IDs
#include "bfetldriver.hxx"  // header for this library
#include "bfftldriver.hxx"  // header for the FTL library
#include "EtwCollection.hxx"

#include <list>


//  Gets the size of a file. Returns -1 if any error occurs.

static __int64 CbGetFileSize( _In_ const WCHAR* const wszFilePath )
{
    __int64 cbFileSize = -1;
    DWORD cbFileSizeHigh = 0;

    HANDLE hFile = CreateFileW( wszFilePath, FILE_READ_ATTRIBUTES, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        goto HandleError;
    }

    const DWORD cbFileSizeLow = GetFileSize( hFile, &cbFileSizeHigh );
    if ( cbFileSizeLow == INVALID_FILE_SIZE )
    {
        goto HandleError;
    }

    cbFileSize = (__int64)( ( ( (__int64)cbFileSizeHigh ) << ( 8 * sizeof(cbFileSizeHigh) ) ) | cbFileSizeLow );

HandleError:

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }

    return cbFileSize;
}


//  Calculates the recommended buffer size for the ETL driver.

static ERR ErrGetOptimalETLDriverBufferSize( _In_ const WCHAR* const wszEtlFilePath, _Out_ DWORD* const pcEventsBufferedMax )
{
    const __int64 cbGetFileSize = CbGetFileSize( wszEtlFilePath );

    if ( cbGetFileSize < 0 )
    {
        return JET_errInternalError;
    }

    //  Estimate number of entries based on the size of the FTL struct.
    //  We'll shoot for 10% of the file size (always between 1MB and 1GB).

    *pcEventsBufferedMax = (DWORD)( max( min( ( 10 * cbGetFileSize ) / 100, 1 * 1024 * 1024 * 1024 ), 1 * 1024 * 1024 ) / sizeof (BFTRACE) );

    return JET_errSuccess;
}


//  Gets the BFRESMGR event from the raw ETW event.

static inline const EseBfResMgrEvent* GetEseBFResMgrEventFromEtwEvent( _In_ const EtwEvent* const pEtwEvent )
{
    const EventHeader* pEventHeader = (EventHeader*)( (BYTE*)pEtwEvent + pEtwEvent->ibExtraData );

    return (EseBfResMgrEvent*)pEventHeader;
}


//  Gets the IFMP summary to be stamped in the FTL file header for a given PID.

CFastTraceLog::BFFTLFilePostProcessHeader& BFETLContext::bfftlPostProcHdr( const DWORD dwPID ) const
{
    return rgbfftlPostProcHdr[ std::distance( pids->begin(), pids->find( dwPID ) ) ];
}


//  Gets the IFMP summary to be stamped in the FTL file header for the PID under collection.
//  Only valid if only one PID is being collected.

CFastTraceLog::BFFTLFilePostProcessHeader& BFETLContext::bfftlPostProcHdr() const
{
    Assert( pids->size() == 1 );
    return bfftlPostProcHdr( *( pids->begin() ) );
}


//  Initializes the BF trace driver's handle / context.

ERR ErrBFETLInit(
    _In_ const void* pvTraceProvider,
    _In_ const std::set<DWORD>& pids,
    _In_ DWORD cEventsBufferedMax,
    _In_ const DWORD grbit,
    _Out_ BFETLContext** const ppbfetlc )
{
    ERR err = JET_errSuccess;
    BFETLContext* pbfetlc = NULL;
    const BOOL fTestMode = ( grbit & fBFETLDriverTestMode ) != 0;
    const BOOL fCollectBfStats = ( grbit & fBFETLDriverCollectBFStats ) != 0;
    const size_t cPids = pids.size();

    //  Paramater validation.

    if ( !pvTraceProvider || !ppbfetlc || ( ( cEventsBufferedMax < 1 ) && fTestMode ) )
    {
        Error( JET_errInvalidParameter );
    }

    //  Block passing PID 0 with other PIDs. Also block no PIDs.

    if ( ( ( pids.count( 0 ) != 0 ) && ( cPids > 1 ) ) || ( cPids == 0 ) )
    {
        Error( JET_errInvalidParameter );
    }

    Alloc( pbfetlc = new BFETLContext() );
    memset( pbfetlc, 0, sizeof(*pbfetlc) );
    Alloc( pbfetlc->pids = new std::set<DWORD>( pids ) );
    Assert( pbfetlc->pids->size() == pids.size() );

    //  Get optimal size if none was passed.

    if ( cEventsBufferedMax == 0 )
    {
        Call( ErrGetOptimalETLDriverBufferSize( (WCHAR*)pvTraceProvider, &cEventsBufferedMax ) );
    }

    //  Initialize some elements explicitly.

    pbfetlc->pvTraceProvider = pvTraceProvider;
    pbfetlc->fTestMode = fTestMode;
    pbfetlc->fCollectBfStats = fCollectBfStats;
    pbfetlc->cEventsBufferedMax = cEventsBufferedMax;
    Alloc( pbfetlc->pListEtwEvent = new BFETLContext::ListEtwEvent() );
    Alloc( pbfetlc->rgbfftlPostProcHdr = new CFastTraceLog::BFFTLFilePostProcessHeader[ cPids ] );
    memset( pbfetlc->rgbfftlPostProcHdr, 0, cPids * sizeof( *( pbfetlc->rgbfftlPostProcHdr ) ) );

    if ( pbfetlc->fCollectBfStats )
    {
        Alloc( pbfetlc->pHashPidStats = new BFETLContext::HashPidStats() );
    }

    if ( !pbfetlc->fTestMode )
    {
        //  Real trace file.

        Alloc( pbfetlc->hETW = EtwOpenTraceFile( (WCHAR*)pbfetlc->pvTraceProvider ) );
    }
    else
    {
        pbfetlc->hETW = (HANDLE)pvTraceProvider;
    }

    Alloc( pbfetlc->hETW );

    *ppbfetlc = pbfetlc;

    return JET_errSuccess;

HandleError:

    BFETLTerm( pbfetlc );

    return err;
}


//  Same functionality as above, but it takes a single PID.

ERR ErrBFETLInit(
    _In_ const void* pvTraceProvider,
    _In_ const DWORD dwPID,
    _In_ DWORD cEventsBufferedMax,
    _In_ const DWORD grbit,
    _Out_ BFETLContext** const ppbfetlc )
{
    std::set<DWORD> pids;
    pids.insert( dwPID );

    return ErrBFETLInit( pvTraceProvider, pids, cEventsBufferedMax, grbit, ppbfetlc );
}


//  Shuts down and frees the BF trace drivers context.

void BFETLTerm( _In_ BFETLContext* const pbfetlc )
{
    //  Nothing to de-allocate.

    if ( !pbfetlc )
    {
        return;
    }

    //  Close ETW file.
    
    if ( pbfetlc->hETW )
    {
        if ( !pbfetlc->fTestMode )
        {
            EtwCloseTraceFile( pbfetlc->hETW );
        }

        pbfetlc->hETW = NULL;
    }

    //  Free cached elements.

    BFETLContext::ListEtwEvent* const pListEtwEvent = pbfetlc->pListEtwEvent;
    if ( pListEtwEvent )
    {
        for ( BFETLContext::ListEtwEventIter etwEventIter = pListEtwEvent->begin();
            etwEventIter != pListEtwEvent->end();
            etwEventIter++ )
        {
            Assert( *etwEventIter != NULL );

            if ( !pbfetlc->fTestMode )
            {
                EtwFreeEvent( *etwEventIter );
            }
        }
        
        delete pListEtwEvent;
        pbfetlc->pListEtwEvent = NULL;
    }

    //  Free PID stats.

    BFETLContext::HashPidStats* const pHashPidStats = pbfetlc->pHashPidStats;
    if ( pHashPidStats )
    {
        for ( BFETLContext::HashPidStatsIter pidStatsIter = pHashPidStats->begin();
            pidStatsIter != pHashPidStats->end();
            pidStatsIter++ )
        {
            BFTraceStats* const pBFTraceStats = pidStatsIter->second;
            Assert( pBFTraceStats != NULL );
            delete pBFTraceStats;
        }
        
        delete pHashPidStats;
        pbfetlc->pHashPidStats = NULL;
    }

    //  Free per-PID IFMP summary.

    delete[] pbfetlc->rgbfftlPostProcHdr;

    //  Free PID list.

    delete pbfetlc->pids;

    delete pbfetlc;
}


//  Gets the next trace record in the ETL trace file, returning JET_errInternalSuccess if we were
//  able to get the trace, errNotFound if we're done with the trace file, and an JET_errInternalError
//  in case of an unexpected error.

ERR ErrBFETLGetNext( _In_ BFETLContext* const pbfetlc, _Out_ BFTRACE* const pbftrace, _Out_ DWORD* const pdwPID )
{
    ERR err = JET_errSuccess;

    if ( !pbfetlc || !pbftrace )
    {
        Error( JET_errInvalidParameter );
    }

    const BOOL fCollectPid0 = ( pbfetlc->pids->count( 0 ) != 0 );

    //  Loop until there are no more traces or we've reached the desired number of buffered elements (+1 to be returned).

    BFETLContext::ListEtwEvent* const pListEtwEvent = pbfetlc->pListEtwEvent;
    EtwEvent* pEtwEvent = NULL;
    while ( ( pListEtwEvent->size() <= pbfetlc->cEventsBufferedMax ) && !pbfetlc->fEOF )
    {
        if ( pEtwEvent && !pbfetlc->fTestMode )
        {
            EtwFreeEvent( pEtwEvent );
        }

        pEtwEvent = NULL;
        
        if ( pbfetlc->fTestMode )
        {
            pEtwEvent = *( (EtwEvent**)pbfetlc->hETW );

            if ( pEtwEvent )
            {
                pbfetlc->hETW = (EtwEvent**)pbfetlc->hETW + 1;
            }
        }
        else
        {
            pEtwEvent = EtwGetNextEvent( pbfetlc->hETW );
        }

        //  Reached EOF.

        if ( !pEtwEvent )
        {
            pbfetlc->fEOF = fTrue;
            continue;
        }

        //  Determine if it's one of the traces of interest.

        if ( ( pEtwEvent->etwEvtType != etwevttEseResMgrInit ) &&
            ( pEtwEvent->etwEvtType != etwevttEseResMgrTerm ) &&
            ( pEtwEvent->etwEvtType != etwevttEseBfCachePage ) &&
            ( pEtwEvent->etwEvtType != etwevttEseBfRequestPage ) &&
            ( pEtwEvent->etwEvtType != etwevttEseBfDirtyPage ) &&
            ( pEtwEvent->etwEvtType != etwevttEseBfWritePage ) &&
            ( pEtwEvent->etwEvtType != etwevttEseBfSetLgposModify ) &&
            ( pEtwEvent->etwEvtType != etwevttEseBfEvictPage ) &&
            ( pEtwEvent->etwEvtType != etwevttEseBfMarkPageAsSuperCold ) )
        {
            continue;
        }

        const EventHeader* const pEventHeader = (EventHeader*)( (BYTE*)pEtwEvent + pEtwEvent->ibExtraData );

        //  Check if the non-ZERO PID is within the list. Also proceed with collection if we're collecting
        //  PID ZERO ("collect-all").

        if ( ( pbfetlc->pids->count( pEventHeader->ulProcessId ) == 0 ) && !fCollectPid0 )
        {
            continue;
        }

        const EseBfResMgrEvent* const pEseBfResMgrEvent = (EseBfResMgrEvent*)pEventHeader;

        const TICK tickCurrent = pEseBfResMgrEvent->tick;

        //  Initialize tracking ticks.

        if ( pListEtwEvent->empty() )
        {
            Assert( pbfetlc->tickMin == 0 );
            Assert( pbfetlc->tickMax == 0 );
            pbfetlc->tickMin = tickCurrent;
            pbfetlc->tickMax = tickCurrent;
        }

        //  The most common is for the new trace to go to the back on the list.

        if ( TickCmp( tickCurrent, pbfetlc->tickMax ) >= 0 )
        {
            pListEtwEvent->push_back( pEtwEvent );
            pEtwEvent = NULL;
            pbfetlc->tickMax = tickCurrent;
            continue;
        }

        pbfetlc->cTracesOutOfOrder++;

        //  Short-circuit the error case where we already returned traces and the buffer is too small.

        if ( TickCmp( tickCurrent, pbfetlc->tickMin ) < 0 )
        {
            if ( ( pbfetlc->cTracesProcessed == 0 ) || ( TickCmp( tickCurrent, pbfetlc->tickLast ) >= 0 ) )
            {
                pListEtwEvent->push_front( pEtwEvent );
                pEtwEvent = NULL;
                pbfetlc->tickMin = tickCurrent;
                continue;
            }
            else
            {
                if ( !pbfetlc->fTestMode )
                {
                    EtwFreeEvent( pEtwEvent );
                }
                
                Error( JET_errInternalError );
            }
        }

        //  We were out of luck. Proceed to expensive search, start from the back.

        Assert( pListEtwEvent->size() > 1 );    //  Covered by the conditions above.
        BOOL fFound = fFalse;
        for ( BFETLContext::ListEtwEventIter etwEventIter = ( --( pListEtwEvent->end() ) );
            true;
            etwEventIter-- )
        {
            const EseBfResMgrEvent* pEseBfResMgrEventIter = GetEseBFResMgrEventFromEtwEvent( *etwEventIter );
            
            if ( TickCmp( tickCurrent, pEseBfResMgrEventIter->tick ) >= 0 )
            {
                //  Insert at previous position, i.e., the next one in the iterator.

                pListEtwEvent->insert( ++etwEventIter, pEtwEvent );
                pEtwEvent = NULL;
                fFound = fTrue;
                break;
            }

            if ( etwEventIter == pListEtwEvent->begin() )
            {
                break;
            }
        }
            
            Assert( fFound );
    }

    if ( pEtwEvent && !pbfetlc->fTestMode )
    {
        EtwFreeEvent( pEtwEvent );
        pEtwEvent = NULL;
    }

    //  We may not have anything to return.

    if ( pListEtwEvent->empty() )
    {
        Error( errNotFound );
    }

    //  The front of the list is what we need to return.

    EtwEvent* const pEtwEventReturn = pListEtwEvent->front();
    const EseBfResMgrEvent* const pEseBfResMgrEvent = GetEseBFResMgrEventFromEtwEvent( pEtwEventReturn );
    const TICK tickCurrent = pEseBfResMgrEvent->tick;
    const EventHeader* const pEventHeader = (EventHeader*)pEseBfResMgrEvent;
    const DWORD dwPID = pEventHeader->ulProcessId;

    //  Can't fail from here.

    pbfetlc->cTracesProcessed++;

    //  Adjust last tick returned (assert always ascending).

    Assert( ( pbfetlc->cTracesProcessed == 1 ) || ( TickCmp( tickCurrent, pbfetlc->tickLast ) >= 0 ) );
    pbfetlc->tickLast = tickCurrent;

    memset( pbftrace, 0, sizeof(*pbftrace) );
    pbftrace->tick = tickCurrent;

    //  Accumulate per-PID stats.

    BFTraceStats bftstatsPIDDummy;
    BFTraceStats* pbftstatsPID = NULL;

    if ( pbfetlc->fCollectBfStats )
    {
        BFETLContext::HashPidStatsIter hashPidStatsIter = pbfetlc->pHashPidStats->find( dwPID );
        if ( hashPidStatsIter != pbfetlc->pHashPidStats->end() )
        {
            pbftstatsPID = hashPidStatsIter->second;
        }
        else
        {
            Alloc( pbftstatsPID = new BFTraceStats() );
            memset( pbftstatsPID, 0, sizeof(*pbftstatsPID) );
            pbfetlc->pHashPidStats->insert( BFETLContext::HashPidStats::value_type( dwPID, pbftstatsPID ) );
        }
    }
    else
    {
        pbftstatsPID = &bftstatsPIDDummy;
    }

    //  Process the trace.

    LONG ifmp = -1;
    ULONG pgno = 0;

    switch ( pEtwEventReturn->etwEvtType )
    {
        case etwevttEseResMgrInit:
        {
            const EseResMgrInit* const pEseResMgrInit = (EseResMgrInit*)pEseBfResMgrEvent;
            pbftrace->traceid = bftidSysResMgrInit;
            pbftrace->bfinit.K = pEseResMgrInit->K;
            pbftrace->bfinit.csecCorrelatedTouch = pEseResMgrInit->csecCorrelatedTouch;
            pbftrace->bfinit.csecTimeout = pEseResMgrInit->csecTimeout;
            pbftrace->bfinit.csecUncertainty = pEseResMgrInit->csecUncertainty;
            pbftrace->bfinit.dblHashLoadFactor = pEseResMgrInit->dblHashLoadFactor;
            pbftrace->bfinit.dblHashUniformity = pEseResMgrInit->dblHashUniformity;
            pbftrace->bfinit.dblSpeedSizeTradeoff = pEseResMgrInit->dblSpeedSizeTradeoff;

            pbfetlc->bftstats.cSysResMgrInit++;
            pbftstatsPID->cSysResMgrInit++;
        }
            break;

        case etwevttEseResMgrTerm:
        {
            //const EseResMgrTerm* const pEseResMgrTerm = (EseResMgrTerm*)pEseBfResMgrEvent;
            pbftrace->traceid = bftidSysResMgrTerm;

            pbfetlc->bftstats.cSysResMgrTerm++;
            pbftstatsPID->cSysResMgrTerm++;
        }
            break;

        case etwevttEseBfCachePage:
        {
            const EseBfCachePage* const pEseBfCachePage = (EseBfCachePage*)pEseBfResMgrEvent;
            pbftrace->traceid = bftidCache;
            Assert( pEseBfCachePage->ifmp <= UCHAR_MAX );
            pbftrace->bfcache.ifmp = (BYTE)pEseBfCachePage->ifmp;
            pbftrace->bfcache.pgno = pEseBfCachePage->pgno;
            Assert( pEseBfCachePage->bflt <= UCHAR_MAX );
            pbftrace->bfcache.bflt = (BYTE)pEseBfCachePage->bflt;
            pbftrace->bfcache.clientType = pEseBfCachePage->clientType;
            pbftrace->bfcache.pctPri = pEseBfCachePage->pctPriority;
            pbftrace->bfcache.fUseHistory = !!( pEseBfCachePage->bfrtf & bfrtfUseHistory );
            pbftrace->bfcache.fNewPage = !!( pEseBfCachePage->bfrtf & bfrtfNewPage );
            pbftrace->bfcache.fDBScan = !!( pEseBfCachePage->bfrtf & bfrtfDBScan );

            ifmp = pbftrace->bfcache.ifmp;
            pgno = pbftrace->bfcache.pgno;
            
            pbfetlc->bftstats.cCache++;
            pbftstatsPID->cCache++;
        }
            break;

        case etwevttEseBfRequestPage:
        {
            const EseBfRequestPage* const pEseBfRequestPage = (EseBfRequestPage*)pEseBfResMgrEvent;
            pbftrace->traceid = bftidTouch;
            Assert( pEseBfRequestPage->ifmp <= UCHAR_MAX );
            pbftrace->bftouch.ifmp = (BYTE)pEseBfRequestPage->ifmp;
            pbftrace->bftouch.pgno = pEseBfRequestPage->pgno;
            Assert( pEseBfRequestPage->bflt <= UCHAR_MAX );
            pbftrace->bftouch.bflt = (BYTE)pEseBfRequestPage->bflt;
            pbftrace->bftouch.clientType = pEseBfRequestPage->clientType;
            pbftrace->bftouch.pctPri = pEseBfRequestPage->pctPriority;
            pbftrace->bftouch.fUseHistory = !!( pEseBfRequestPage->bfrtf & bfrtfUseHistory );
            pbftrace->bftouch.fNewPage = !!( pEseBfRequestPage->bfrtf & bfrtfNewPage );
            pbftrace->bftouch.fNoTouch = !!( pEseBfRequestPage->bfrtf & bfrtfNoTouch );
            pbftrace->bftouch.fDBScan = !!( pEseBfRequestPage->bfrtf & bfrtfDBScan );

            ifmp = pbftrace->bftouch.ifmp;
            pgno = pbftrace->bftouch.pgno;

            pbfetlc->bftstats.cRequest++;
            pbftstatsPID->cRequest++;
        }
            break;

        case etwevttEseBfDirtyPage:
        {
            const EseBfDirtyPage* const pEseBfDirtyPage = (EseBfDirtyPage*)pEseBfResMgrEvent;
            pbftrace->traceid = bftidDirty;
            Assert( pEseBfDirtyPage->ifmp <= UCHAR_MAX );
            pbftrace->bfdirty.ifmp = (BYTE)pEseBfDirtyPage->ifmp;
            pbftrace->bfdirty.pgno = pEseBfDirtyPage->pgno;
            Assert( pEseBfDirtyPage->ulDirtyLevel <= UCHAR_MAX );
            pbftrace->bfdirty.bfdf = (BYTE)pEseBfDirtyPage->ulDirtyLevel;
            pbftrace->bfdirty.lgenModify = pEseBfDirtyPage->lgposModify.lGen;
            pbftrace->bfdirty.isecModify = pEseBfDirtyPage->lgposModify.isec;
            pbftrace->bfdirty.ibModify = pEseBfDirtyPage->lgposModify.ib;

            ifmp = pbftrace->bfdirty.ifmp;
            pgno = pbftrace->bfdirty.pgno;

            pbfetlc->bftstats.cDirty++;
            pbftstatsPID->cDirty++;
        }
            break;

        case etwevttEseBfWritePage:
        {
            const EseBfWritePage* const pEseBfWritePage = (EseBfWritePage*)pEseBfResMgrEvent;
            pbftrace->traceid = bftidWrite;
            Assert( pEseBfWritePage->ifmp <= UCHAR_MAX );
            pbftrace->bfwrite.ifmp = (BYTE)pEseBfWritePage->ifmp;
            pbftrace->bfwrite.pgno = pEseBfWritePage->pgno;
            Assert( pEseBfWritePage->ulDirtyLevel <= UCHAR_MAX );
            pbftrace->bfwrite.bfdf = (BYTE)pEseBfWritePage->ulDirtyLevel;
            pbftrace->bfwrite.iorp = pEseBfWritePage->tcevtBf.bIorp;

            ifmp = pbftrace->bfwrite.ifmp;
            pgno = pbftrace->bfwrite.pgno;

            pbfetlc->bftstats.cWrite++;
            pbftstatsPID->cWrite++;
        }
            break;

        case etwevttEseBfSetLgposModify:
        {
            const EseBfSetLgposModify* const pEseBfSetLgposModify = (EseBfSetLgposModify*)pEseBfResMgrEvent;
            pbftrace->traceid = bftidSetLgposModify;
            Assert( pEseBfSetLgposModify->ifmp <= UCHAR_MAX );
            pbftrace->bfsetlgposmodify.ifmp = (BYTE)pEseBfSetLgposModify->ifmp;
            pbftrace->bfsetlgposmodify.pgno = pEseBfSetLgposModify->pgno;
            pbftrace->bfsetlgposmodify.lgenModify = pEseBfSetLgposModify->lgposModify.lGen;
            pbftrace->bfsetlgposmodify.isecModify = pEseBfSetLgposModify->lgposModify.isec;
            pbftrace->bfsetlgposmodify.ibModify = pEseBfSetLgposModify->lgposModify.ib;

            ifmp = pbftrace->bfsetlgposmodify.ifmp;
            pgno = pbftrace->bfsetlgposmodify.pgno;

            pbfetlc->bftstats.cSetLgposModify++;
            pbftstatsPID->cSetLgposModify++;
        }
            break;

        case etwevttEseBfEvictPage:
        {
            const EseBfEvictPage* const pEseBfEvictPage = (EseBfEvictPage*)pEseBfResMgrEvent;
            pbftrace->traceid = bftidEvict;
            Assert( pEseBfEvictPage->ifmp <= UCHAR_MAX );
            pbftrace->bfevict.ifmp = (BYTE)pEseBfEvictPage->ifmp;
            pbftrace->bfevict.pgno = pEseBfEvictPage->pgno;
            pbftrace->bfevict.fCurrentVersion = pEseBfEvictPage->fCurrentVersion != 0;
            pbftrace->bfevict.errBF = pEseBfEvictPage->errBF;
            pbftrace->bfevict.bfef = pEseBfEvictPage->bfef;
            pbftrace->bfevict.pctPri = pEseBfEvictPage->pctPriority;

            ifmp = pbftrace->bfevict.ifmp;
            pgno = pbftrace->bfevict.pgno;

            pbfetlc->bftstats.cEvict++;
            pbftstatsPID->cEvict++;
        }
            break;

        case etwevttEseBfMarkPageAsSuperCold:
        {
            const EseBfMarkPageAsSuperCold* const pEseBfMarkPageAsSuperCold = (EseBfMarkPageAsSuperCold*)pEseBfResMgrEvent;
            pbftrace->traceid = bftidSuperCold;
            Assert( pEseBfMarkPageAsSuperCold->ifmp <= UCHAR_MAX );
            pbftrace->bfsupercold.ifmp = (BYTE)pEseBfMarkPageAsSuperCold->ifmp;
            pbftrace->bfsupercold.pgno = pEseBfMarkPageAsSuperCold->pgno;

            ifmp = pbftrace->bfsupercold.ifmp;
            pgno = pbftrace->bfsupercold.pgno;

            pbfetlc->bftstats.cSuperCold++;
            pbftstatsPID->cSuperCold++;
        }
            break;

        default:
            AssertSz( fFalse, "We should have filtered out unknown events before." );
    }

    //  Remove and free object.

    pListEtwEvent->pop_front();

    if ( !pbfetlc->fTestMode )
    {
        EtwFreeEvent( pEtwEventReturn );
    }

    //  Adjust IFMP array.

    Assert( ( ifmp >= 0 ) || ( pgno == 0 ) );
    if ( ifmp >= 0 )
    {
        const DWORD dwPIDT = fCollectPid0 ? 0 : dwPID;
        CFastTraceLog::BFFTLFilePostProcessHeader& bfftlPostProcHdr = pbfetlc->bfftlPostProcHdr( dwPIDT );
        
        Assert( ifmp < _countof( bfftlPostProcHdr.le_mpifmpcpg ) );
        Assert( bfftlPostProcHdr.le_cifmp < LONG_MAX );

        if ( ifmp >= (LONG)bfftlPostProcHdr.le_cifmp )
        {
            bfftlPostProcHdr.le_cifmp = (ULONG)( ifmp + 1 );
        }

        Assert( bfftlPostProcHdr.le_cifmp <= _countof( bfftlPostProcHdr.le_mpifmpcpg ) );

        if ( pgno > bfftlPostProcHdr.le_mpifmpcpg[ ifmp ] )
        {
            bfftlPostProcHdr.le_mpifmpcpg[ ifmp ] = pgno;
        }
    }

    //  Adjust tracking ticks.

    if ( pListEtwEvent->empty() )
    {
        pbfetlc->tickMin = 0;
        pbfetlc->tickMax = 0;
    }
    else
    {
        pbfetlc->tickMin = GetEseBFResMgrEventFromEtwEvent( pListEtwEvent->front() )->tick;
    }

    //  PID.

    if ( pdwPID != NULL )
    {
        *pdwPID = dwPID;
    }

HandleError:

    return err;
}


//  Overload of the function above that does not return a PID.

ERR ErrBFETLGetNext( _In_ BFETLContext* const pbfetlc, _Out_ BFTRACE* const pbftrace )
{
    return ErrBFETLGetNext( pbfetlc, pbftrace, NULL );
}


//  Gets BFTraceStats specific to a process ID.

const BFTraceStats* PbftsBFETLStatsByPID( _In_ const BFETLContext* const pbfetlc, __inout DWORD* const pdwPID )
{
    const DWORD iPIDSearch = *pdwPID;

    *pdwPID = 0;

    if ( !pbfetlc )
    {
        return NULL;
    }
    
    BFETLContext::HashPidStats* const pHashPidStats = pbfetlc->pHashPidStats;
    if ( !pHashPidStats || ( ( iPIDSearch + 1 ) > pHashPidStats->size() ) )
    {
        return NULL;
    }

    const BFTraceStats* pbfts = NULL;
    DWORD iPID = 0;

    for ( BFETLContext::HashPidStatsIter pidStatsIter = pHashPidStats->begin();
        pidStatsIter != pHashPidStats->end();
        pidStatsIter++, iPID++ )
    {
        if ( iPID == iPIDSearch )
        {
            *pdwPID = pidStatsIter->first;
            pbfts = pidStatsIter->second;
            break;
        }
    }

    Assert( *pdwPID != 0 ); 
    Assert( pbfts != NULL );    

    return pbfts;
}

class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
{
public:
    CFileSystemConfiguration()
    {
        m_dtickAccessDeniedRetryPeriod = 2000;
        m_cbMaxReadSize = 512 * 1024;
        m_cbMaxWriteSize = 384 * 1024;
    }
} g_fsconfigETL;

//  Converts an ETL file into an FTL post-processed file.

ERR ErrBFETLConvertToFTL(
    _In_ const WCHAR* const wszEtlFilePath,
    _In_ const WCHAR* const wszFtlFilePath,
    _In_ const std::set<DWORD>& pids )
{
    ERR err = JET_errSuccess;
    BFETLContext* pbfetlc = NULL;
    std::unordered_map<DWORD, CFastTraceLog*> pbfftls;
    __int64 cTracesConv = 0;
    IOREASON iorBFTraceFile( (IOREASONPRIMARY) 24 );    //  iorpOsLayerTracing = 24.
    BFTRACE bftrace;
    DWORD dwPID = 0;
    const BOOL fCollectPid0 = ( pids.count( 0 ) != 0 );
    const BOOL fCollectSinglePid = ( pids.size() == 1 );

    //  Initialize ETL.

    Call( ErrBFETLInit( wszEtlFilePath, pids, 0, fBFETLDriverCollectBFStats, &pbfetlc ) );
    Assert( !fCollectPid0 || fCollectSinglePid );

    //  Initialize FTL.

    for ( auto pidIter = pids.begin(); pidIter != pids.end(); pidIter++ )
    {
        CFastTraceLog* const pbfftl = new CFastTraceLog( NULL, &g_fsconfigETL );
        Alloc( pbfftl );
        const DWORD dwPID = *pidIter;
        pbfftls.insert( std::pair<DWORD, CFastTraceLog*>( dwPID, pbfftl ) );

        WCHAR wszFtlFileDrive[ _MAX_DRIVE + 1 ] = { L'\0' };
        WCHAR wszFtlFileDir[ _MAX_DIR + 1 ] = { L'\0' };
        WCHAR wszFtlFileName[ _MAX_FNAME + 1 ] = { L'\0' };
        WCHAR wszFtlFileExt[ _MAX_EXT + 1 ] = { L'\0' };
        WCHAR wszFtlFileNamePerPid[ _MAX_FNAME + 1 ] = { L'\0' };
        WCHAR wszFtlFilePathPerPid[ MAX_PATH + 1 ] = { L'\0' };

        if ( !fCollectSinglePid )
        {
            if ( _wsplitpath_s( wszFtlFilePath,
                    wszFtlFileDrive, _countof( wszFtlFileDrive ),
                    wszFtlFileDir, _countof( wszFtlFileDir ),
                    wszFtlFileName, _countof( wszFtlFileName ),
                    wszFtlFileExt, _countof( wszFtlFileExt ) ) )
            {
                Error( JET_errInternalError );
            }


            if ( swprintf_s( wszFtlFileNamePerPid, _countof( wszFtlFileNamePerPid ),
                    L"%ws.Pid%u", wszFtlFileName, dwPID ) < 6 )
            {
                Error( JET_errInternalError );
            }

            if ( _wmakepath_s( wszFtlFilePathPerPid, _countof( wszFtlFilePathPerPid ),
                    wszFtlFileDrive,
                    wszFtlFileDir,
                    wszFtlFileNamePerPid,
                    wszFtlFileExt ) )
            {
                Error( JET_errInternalError );
            }
        }

        const WCHAR* const wszFtlFilePathT = fCollectSinglePid ? wszFtlFilePath : wszFtlFilePathPerPid;

        Call( pbfftl->ErrFTLInitWriter( wszFtlFilePathT, &iorBFTraceFile, CFastTraceLog::ftlifNewlyCreated ) );
    }

    //  Go through all the events.

    printf( "\r\n" );
    while ( ( err = ErrBFETLGetNext( pbfetlc, &bftrace, &dwPID ) ) >= JET_errSuccess )
    {
        AssertSz( dwPID != 0, "Unexpected PID ZERO." );
        if ( fCollectPid0 )
        {
            dwPID = 0;
        }
        AssertSz( pids.count( dwPID ) != 0, "Unexpected PID %u.", dwPID );

        const BYTE* pbTrace = NULL;
        ULONG cbTrace = 0;

        switch ( bftrace.traceid )
        {
            case bftidSysResMgrInit:
                pbTrace = (BYTE*)&bftrace.bfinit;
                cbTrace = sizeof(bftrace.bfinit);
                break;

            case bftidSysResMgrTerm:
                pbTrace = (BYTE*)&bftrace.bfterm;
                cbTrace = sizeof(bftrace.bfterm);
                break;

            case bftidCache:
                pbTrace = (BYTE*)&bftrace.bfcache;
                cbTrace = sizeof(bftrace.bfcache);
                break;

            case bftidTouch:
            case bftidTouchLong:
                pbTrace = (BYTE*)&bftrace.bftouch;
                cbTrace = sizeof(bftrace.bftouch);
                break;

            case bftidDirty:
                pbTrace = (BYTE*)&bftrace.bfdirty;
                cbTrace = sizeof(bftrace.bfdirty);
                break;

            case bftidWrite:
                pbTrace = (BYTE*)&bftrace.bfwrite;
                cbTrace = sizeof(bftrace.bfwrite);
                break;

            case bftidSetLgposModify:
                pbTrace = (BYTE*)&bftrace.bfsetlgposmodify;
                cbTrace = sizeof(bftrace.bfsetlgposmodify);
                break;

            case bftidEvict:
                pbTrace = (BYTE*)&bftrace.bfevict;
                cbTrace = sizeof(bftrace.bfevict);
                break;

            case bftidSuperCold:
                pbTrace = (BYTE*)&bftrace.bfsupercold;
                cbTrace = sizeof(bftrace.bfsupercold);
                break;

            case bftidVerifyInfo:
                pbTrace = (BYTE*)&bftrace.bfverify;
                cbTrace = sizeof(bftrace.bfverify);
                break;

            default:
                AssertSz( fFalse, "Unknown trace type %hu.", bftrace.traceid );
                break;
        }

        CFastTraceLog* const pbfftl = pbfftls.find( dwPID )->second;
        Call( pbfftl->ErrFTLTrace( bftrace.traceid, pbTrace, cbTrace, bftrace.tick ) );

        if ( ( cTracesConv++ % 100000 ) == 0 )
        {
            printf( "." );
        }
    }

    if ( err == errNotFound )
    {
        err = JET_errSuccess;
    }

    Call( err );

    for ( auto pbfftlIter = pbfftls.begin(); pbfftlIter != pbfftls.end(); pbfftlIter++ )
    {
        CFastTraceLog* const pbfftl = pbfftlIter->second;
        Call( pbfftl->ErrFTLSetPostProcessHeader( &( pbfetlc->bfftlPostProcHdr( pbfftlIter->first ) ) ) );
    }

HandleError:

    printf( "\r\n" );
    printf( "Completed conversion (%I64d traces) with error code %d.\r\n", cTracesConv, err );

    if ( pbfetlc )
    {
        (void)ErrBFETLDumpStats( pbfetlc, fBFETLDriverCollectBFStats );
    }

    for ( auto pbfftlIter = pbfftls.begin(); pbfftlIter != pbfftls.end(); pbfftlIter++ )
    {
        CFastTraceLog* const pbfftl = pbfftlIter->second;
        pbfftl->FTLTerm();
        delete pbfftl;
    }

    BFETLTerm( pbfetlc );
    pbfetlc = NULL;

    return err;
}


//  Compares an ETL against an FTL file. Returns JET_errDatabaseCorrupted if they are not the same.

ERR ErrBFETLFTLCmp(
    _In_ const void* pvTraceProviderEtl,
    _In_ const void* pvTraceProviderFtl,
    _In_ const DWORD dwPID,
    _In_ const BOOL fTestMode,
    _Out_ __int64* const pcTracesProcessed )
{
    ERR err = JET_errSuccess;
    ERR errCmp = JET_errSuccess;
    BFETLContext* pbfetlc = NULL;
    BFFTLContext* pbfftlc = NULL;
    BFTRACE bftraceFromETL;
    BFTRACE bftraceFromFTL;
    DWORD cEventsBufferedMax = fTestMode ? 100 : 0;
    __int64 cTracesCmp = 0;

    //  Initialize ETL.

    Call( ErrBFETLInit(
        pvTraceProviderEtl,
        dwPID,
        cEventsBufferedMax,
        fTestMode ? fBFETLDriverTestMode : 0,
        &pbfetlc ) );

    //  Initialize FTL.

    Call( ErrBFFTLInit(
        pvTraceProviderFtl,
        fBFFTLDriverResMgrTraces | ( fTestMode ? fBFFTLDriverTestMode : 0 ),
        &pbfftlc ) );

    //  Go through all the events.

    printf( "\r\n" );
    while ( ( err = ErrBFETLGetNext( pbfetlc, &bftraceFromETL ) ) >= JET_errSuccess )
    {
        memset( &bftraceFromFTL, 0, sizeof(bftraceFromFTL) );
        const ERR errT = ErrBFFTLGetNext( pbfftlc, &bftraceFromFTL );

        if ( errT == errNotFound )
        {
            printf( "Failed to compare trace # %I64d: FTL reached EOF too early (ETL has more traces).\r\n", cTracesCmp );
            errCmp = JET_errDatabaseCorrupted;
            break;
        }

        Call( errT );

        if ( memcmp( &bftraceFromETL, &bftraceFromFTL, sizeof(BFTRACE) ) != 0 )
        {
            printf( "Failed to compare trace # %I64d: trace entries are different.\r\n", cTracesCmp );
            errCmp = JET_errDatabaseCorrupted;
            break;
        }

        if ( ( cTracesCmp++ % 100000 ) == 0 )
        {
            printf( "." );
        }
    }
    printf( "\r\n" );

    if ( err == errNotFound )
    {
        err = JET_errSuccess;

        const ERR errT = ErrBFFTLGetNext( pbfftlc, &bftraceFromFTL );
        if ( errT >= JET_errSuccess )
        {
            printf( "Failed to compare trace # %I64d: ETL reached EOF too early (FTL has more traces).\r\n", cTracesCmp );
            errCmp = JET_errDatabaseCorrupted;
        }
        else if ( errT != errNotFound )
        {
            Call( errT );
        }
    }

    Call( err );

    //  Compare post-processeed header.

    const ULONG cEtlIfmp = pbfetlc->bfftlPostProcHdr().le_cifmp;
    const ULONG cFtlIfmp = (ULONG)( pbfftlc->cIFMP );
    if ( cEtlIfmp != cFtlIfmp )
    {
        printf(  "Failed to compare post-processed header: IFMP count (ETL: %lu, FTL: %lu).\r\n", cEtlIfmp, cFtlIfmp );
        errCmp = JET_errDatabaseCorrupted;
    }

    for ( ULONG ifmp = 0; ifmp < min( cEtlIfmp, cFtlIfmp ); ifmp++ )
    {
        const ULONG pgnoEtlMax = pbfetlc->bfftlPostProcHdr().le_mpifmpcpg[ ifmp ];
        const ULONG pgnoFtlMax = (ULONG)( pbfftlc->rgpgnoMax[ ifmp ] );
        
        if ( pgnoEtlMax != pgnoFtlMax )
        {
            printf(
                "Failed to compare post-processed header: PGNO max for IFMP %lu (ETL: %lu, FTL: %lu).\r\n",
                ifmp,
                pgnoEtlMax,
                pgnoFtlMax );
            errCmp = JET_errDatabaseCorrupted;
            break;
        }
    }

HandleError:

    printf( "\r\n" );
    printf( "Completed comparison (%I64d traces) with error code %d, comparison code %d.\r\n", cTracesCmp, err, errCmp );

    if ( pbfftlc )
    {
        BFFTLTerm( pbfftlc );
        pbfftlc = NULL;
    }

    BFETLTerm( pbfetlc );
    pbfetlc = NULL;

    if ( pcTracesProcessed )
    {
        *pcTracesProcessed = cTracesCmp;
    }

    return ( errCmp < JET_errSuccess ) ? errCmp : err;
}


//  Dump stats related to the ETL file.

ERR ErrBFETLDumpStats( _In_ const BFETLContext* const pbfetlc, _In_ const DWORD grbit )
{
    //  Dump general stats.

    printf( "Total traces processed: %I64d\r\n", pbfetlc->cTracesProcessed );
    printf( "Traces out of order: %I64d\r\n", pbfetlc->cTracesOutOfOrder );
    printf( "IFMP/PGNO information (Ifmp,PgnoMax):\r\n" );
    for ( auto pidIter = pbfetlc->pids->begin(); pidIter != pbfetlc->pids->end(); pidIter++ )
    {
        const DWORD dwPID = *pidIter;
        CFastTraceLog::BFFTLFilePostProcessHeader& bfftlPostProcHdr = pbfetlc->bfftlPostProcHdr( dwPID );

        printf( "  PID %u:", dwPID );
        if ( bfftlPostProcHdr.le_cifmp > 0 )
        {
            for ( ULONG ifmp = 0; ifmp < bfftlPostProcHdr.le_cifmp; ifmp++ )
            {
                printf( " (%lu,%lu)", ifmp, (ULONG)( bfftlPostProcHdr.le_mpifmpcpg[ ifmp ] ) );
            }
        }
        else
        {
            printf( " no IFMPs reported" );
        }
        printf( "\r\n" );
    }

    //  Dump BF stats.

    if ( grbit & fBFETLDriverCollectBFStats )
    {
        printf( "BF/RESMGR trace stats:\r\n" );
        printf( "  Global:\r\n" );
        pbfetlc->bftstats.Print( 4 );

        const BFTraceStats* pbfts = NULL;
        for ( DWORD i = 0, dwPID = i;
            ( pbfts = PbftsBFETLStatsByPID( pbfetlc, &dwPID ) ) != NULL;
            dwPID = ++i )
        {
            printf( "  Process ID %u:\r\n", dwPID );
            pbfts->Print( 4 );
        }
        
        printf( "\r\n" );
    }

    return JET_errSuccess;
}

