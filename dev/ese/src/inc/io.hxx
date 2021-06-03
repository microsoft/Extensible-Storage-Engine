// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef _IO_H
#error IO.HXX already included
#endif
#define _IO_H

ERR ErrIOInit();
VOID IOTerm();

ERR ErrIOInit( INST *pinst );
ERR ErrIOTerm( INST *pinst, BOOL fNormal );
ERR ErrIOTermFMP( FMP *pfmp, LGPOS lgposShutdownMarkRec, BOOL fNormal );
void IOResetFMPFields( FMP * const pfmp, const LOG * const plog );


VOID ConvertFileTimeToLogTime( __int64 ft, LOGTIME* plt );
bool FLogtimeIsNull( const LOGTIME * const plt );
bool FLogtimeIsValid( const LOGTIME * const plt );
__int64 ConvertLogTimeToFileTime( const LOGTIME* plt );


//  Reserve first 2 pages of a database.
//
const CPG   cpgDBReserved = 2;

INLINE QWORD OffsetOfPgno( PGNO pgno )              { return QWORD( pgno - 1 + cpgDBReserved ) * g_cbPage; }
INLINE PGNO PgnoOfOffset( QWORD ib )                { return PGNO( ( ib / g_cbPage ) + 1 - cpgDBReserved ); }

INLINE QWORD CbFileSizeOfCpg( CPG cpg )             { return QWORD( cpg + cpgDBReserved ) * g_cbPage; }
INLINE QWORD CbFileSizeOfPgnoLast( PGNO pgnoLast )  { return CbFileSizeOfCpg( (CPG)pgnoLast ); }
INLINE PGNO FMP::PgnoLast() const                   { return PGNO( CpgOfCb( CbOwnedFileSize() ) - cpgDBReserved ); }

ERR ErrIOResizeUpdateDbHdrCount( const IFMP ifmp, const BOOL fExtend );
ERR ErrIOResizeUpdateDbHdrLgposLast( const IFMP ifmp, const LGPOS& lgposLastResize );
ERR ErrIONewSize( const IFMP ifmp, const TraceContext& tc, const CPG cpgNewSize, const CPG cpgAsyncExtension, const JET_GRBIT grbit );
ERR ErrIOArchiveShrunkPages( const IFMP ifmp, const TraceContext& tc, const PGNO pgnoFirst, const CPG cpgArchive );
ERR ErrIODeleteShrinkArchiveFiles( const IFMP ifmp );

ERR ErrIOTrimNormalizeOffsetsAndPgnos(
    _In_ const IFMP ifmp,
    _In_ const PGNO pgnoStartZeroes,
    _In_ const CPG cpgZeroLength,
    _Out_ QWORD* pibStartZeroes,
    _Out_ QWORD* pcbZeroLength,
    _Out_ PGNO* ppgnoStartZeroes,
    _Out_ CPG* pcpgZeroLength );

ERR ErrIOTrim(
    _In_ const IFMP ifmp,
    _In_ const QWORD ibStartZeroesAligned,
    _In_ const QWORD cbZeroLength );

ERR ErrIOUpdateCheckpoints( INST * pinst );

INLINE BOOL FIODatabaseOpen( IFMP ifmp )
{
    return BOOL( NULL != g_rgfmp[ ifmp ].Pfapi() );
}

ERR ErrIOOpenDatabase(
    _In_ IFileSystemAPI *const pfsapi,
    _In_ IFMP ifmp,
    _In_ PCWSTR wszDatabaseName,
    _In_ const IOFILE iofile,
    _In_ const BOOL fSparseEnabledFile );

VOID IOCloseDatabase( IFMP ifmp );

ERR ErrIODeleteDatabase( IFileSystemAPI *const pfsapi, IFMP ifmp );

//  Force previous writes to Database to disk (and update appropriate trackings)
ERR ErrIOFlushDatabaseFileBuffers( IFMP ifmp, const IOFLUSHREASON iofr );
#ifdef DEBUG
const static __int64 cioAllowLogRollHeaderUpdates = 4;  // note: Technically only need two, but gave myself two extra for insurance ...
BOOL FIOCheckUserDbNonFlushedIos( const INST * const pinst, const __int64 cioPerDbOutstandingLimit = 0, IFMP ifmpTargetedDB = ifmpNil );
#endif

ERR ErrIOReadDbPages( IFMP ifmp, IFileAPI *pfapi, BYTE *pbData, LONG pgnoStart, LONG pgnoEnd, BOOL fCheckPagesOffset, LONG pgnoMost, const TraceContext& tc, BOOL fExtensiveChecks );           // potential page number of the last page of the recoverying database, might be a patch page (in the form of DB header)

ERR ISAMAPI   ErrIsamGetInstanceInfo( ULONG *pcInstanceInfo, JET_INSTANCE_INFO_W ** paInstanceInfo, const CESESnapshotSession * pSnapshotSession );

ERR ISAMAPI ErrIsamOSSnapshotPrepare( JET_OSSNAPID * psnapId, const JET_GRBIT   grbit );
ERR ISAMAPI ErrIsamOSSnapshotPrepareInstance( JET_OSSNAPID snapId, INST * pinst, const JET_GRBIT    grbit );
ERR ISAMAPI ErrIsamOSSnapshotFreeze( const JET_OSSNAPID snapId, ULONG *pcInstanceInfo, JET_INSTANCE_INFO_W ** paInstanceInfo,const  JET_GRBIT grbit );
ERR ISAMAPI ErrIsamOSSnapshotThaw(  const JET_OSSNAPID snapId, const JET_GRBIT grbit );
ERR ISAMAPI ErrIsamOSSnapshotAbort( const JET_OSSNAPID snapId, const JET_GRBIT grbit );

ERR ISAMAPI ErrIsamOSSnapshotTruncateLog(   const JET_OSSNAPID snapId, INST * pinst, const  JET_GRBIT grbit );
ERR ISAMAPI ErrIsamOSSnapshotGetFreezeInfo( const JET_OSSNAPID          snapId,
                                            ULONG *         pcInstanceInfo,
                                            JET_INSTANCE_INFO_W **  paInstanceInfo,
                                            const JET_GRBIT         grbit );
ERR ISAMAPI ErrIsamOSSnapshotEnd(   const JET_OSSNAPID snapId, const    JET_GRBIT grbit );



//  analagous to THREADSTATS
//
struct THREADSTATSCOUNTERS
{
#ifdef MINIMAL_FUNCTIONALITY
    DWORD                   dwUnused;
#else
    PERFInstanceLiveTotal<>         cPageReferenced;    //  pages referenced
    PERFInstanceLiveTotal<>         cPageRead;          //  pages read from disk
    PERFInstanceLiveTotal<>         cPagePreread;       //  pages preread from disk
    PERFInstanceLiveTotal<>         cPageDirtied;       //  clean pages modified
    PERFInstanceLiveTotal<>         cPageRedirtied;     //  dirty pages modified
    PERFInstanceLiveTotal<>         cLogRecord;         //  log records generated
    PERFInstanceLiveTotal<QWORD>    cbLogRecord;        //  log record bytes generated
#endif
};

class UPDATETHREADSTATSCOUNTERS
{
#ifdef MINIMAL_FUNCTIONALITY
    public:
        UPDATETHREADSTATSCOUNTERS( INST * const pinst, THREADSTATSCOUNTERS * const ptscounters ) {}
        ~UPDATETHREADSTATSCOUNTERS()        {}

#else

    private:
        INST * const                m_pinst;
        THREADSTATSCOUNTERS *       m_ptscounters;
        JET_THREADSTATS             m_threadstatsInitial;

    public:
        UPDATETHREADSTATSCOUNTERS( INST * const pinst, THREADSTATSCOUNTERS * const ptscounters ) :
            m_pinst( pinst ),
            m_ptscounters( ptscounters )
        {
            Assert( NULL != m_pinst );
            Assert( NULL != m_ptscounters );

            //  snapshot initial stats
            //
            memcpy( &m_threadstatsInitial, &Ptls()->threadstats, sizeof( m_threadstatsInitial ) );
        }
        ~UPDATETHREADSTATSCOUNTERS()
        {
            //  counters automatically tabulated when the object is destructed (ie. goes out of scope)
            //
            TabulateCounters();
        }

        //  provide means of manually tabulating counters in case you want it to happen
        //  before the object is destructed
        //
        VOID TabulateCounters()
        {
            if ( NULL != m_ptscounters )
            {
                //  snapshot final stats
                //
                JET_THREADSTATS threadstatsFinal;
                memcpy( &threadstatsFinal, &Ptls()->threadstats, sizeof( threadstatsFinal ) );

                //  compare final stats with initial stats in order to update individual counters
                //
                PERFOpt( m_ptscounters->cPageReferenced.Add( m_pinst, threadstatsFinal.cPageReferenced - m_threadstatsInitial.cPageReferenced ) );
                PERFOpt( m_ptscounters->cPageRead.Add( m_pinst, threadstatsFinal.cPageRead - m_threadstatsInitial.cPageRead ) );
                PERFOpt( m_ptscounters->cPagePreread.Add( m_pinst, threadstatsFinal.cPagePreread - m_threadstatsInitial.cPagePreread ) );
                PERFOpt( m_ptscounters->cPageDirtied.Add( m_pinst, threadstatsFinal.cPageDirtied - m_threadstatsInitial.cPageDirtied ) );
                PERFOpt( m_ptscounters->cPageRedirtied.Add( m_pinst, threadstatsFinal.cPageRedirtied - m_threadstatsInitial.cPageRedirtied ) );
                PERFOpt( m_ptscounters->cLogRecord.Add( m_pinst, threadstatsFinal.cLogRecord - m_threadstatsInitial.cLogRecord ) );
                PERFOpt( m_ptscounters->cbLogRecord.Add( m_pinst, threadstatsFinal.cbLogRecord - m_threadstatsInitial.cbLogRecord ) );

                //  NULL out pointer to signify that counters have already been tabulated (in case
                //  you want to manually force tabulation of counters before the destructor is called
                //
                m_ptscounters = NULL;
            }
        }
#endif  //  MINIMAL_FUNCTIONALITY
};


//  These two functions will affect the qos base value throughout the entire engine.

INLINE OSFILEQOS QosSyncDefault( const INST * const pinst )
{
    OSFILEQOS qos = qosIODispatchImmediate;

    if ( ( pinst != pinstNil ) && ( JET_IOPriorityLow & UlParam( pinst, JET_paramIOPriority ) ) )
    {
        qos |= qosIOOSLowPriority;
    }

    /*
    Can't assume an INST b/c of INSTanceless APIs, such as this:
        Assertion Failure: pinst != pinstNil || ( 0 == INST::AllocatedInstances() ) || g_rgparam[ paramid ].FGlobal()
        Rel.828.0, File daedef.hxx, Line 4639, PID: 15944 (0x3e48), TID: 0x4064

        00000099'e06ae530 00007ffb'dd226b92 ESE!Param_+0x68 [f:\src\e16\esecallsign\sources\dev\ese\src\inc\daedef.hxx @ 4645]
        00000099'e06ae570 00007ffb'dd27da4b ESE!Param+0xc2 [f:\src\e16\esecallsign\sources\dev\ese\src\inc\daedef.hxx @ 4676]
        00000099'e06ae5a0 00007ffb'dd22883b ESE!PvParam+0x1b [f:\src\e16\esecallsign\sources\dev\ese\src\inc\daedef.hxx @ 4726]
        00000099'e06ae5d0 00007ffb'dd3ed6a7 ESE!QosSyncDefault+0x4b [f:\src\e16\esecallsign\sources\dev\ese\src\inc\io.hxx @ 197]
        00000099'e06ae610 00007ffb'dd3ee796 ESE!CFlushMap::ErrReadFmPage_+0x1c7 [f:\src\e16\esecallsign\sources\dev\ese\src\ese\flushmap.cxx @ 1354]
        00000099'e06ae6e0 00007ffb'dd3e9026 ESE!CFlushMap::ErrSyncReadFmPage_+0x26 [f:\src\e16\esecallsign\sources\dev\ese\src\ese\flushmap.cxx @ 1613]
        00000099'e06ae710 00007ffb'dd3ec992 ESE!CFlushMap::ErrAttachFlushMap_+0x7f6 [f:\src\e16\esecallsign\sources\dev\ese\src\ese\flushmap.cxx @ 384]
        00000099'e06aeb00 00007ffb'dd3ec237 ESE!CFlushMap::ErrInitFlushMap+0x532 [f:\src\e16\esecallsign\sources\dev\ese\src\ese\flushmap.cxx @ 2666]
        00000099'e06aeb90 00007ffb'dd42f114 ESE!CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime+0x2b7 [f:\src\e16\esecallsign\sources\dev\ese\src\ese\flushmap.cxx @ 3459]
        00000099'e06aee30 00007ffb'dd4965e1 ESE!ErrIsamRemoveLogfile+0xc84 [f:\src\e16\esecallsign\sources\dev\ese\src\ese\io.cxx @ 6235]
        00000099'e06aefe0 00007ffb'dd496730 ESE!JetRemoveLogfileExW+0x271 [f:\src\e16\esecallsign\sources\dev\ese\src\ese\jetapi.cxx @ 12826]
        00000099'e06af070 00007ff6'457872ae ESE!JetRemoveLogfileW+0x70 [f:\src\e16\esecallsign\sources\dev\ese\src\ese\jetapi.cxx @ 12858]
        00000099'e06af0d0 00007ff6'4576e566 eseunittest!TERMDIRTY::ErrTest+0x133e [f:\src\e16\esecallsign\sources\test\ese\src\blue\src\eseunittest\termdirty.cxx @ 550]
    */

    if ( pinst != pinstNil )
    {
         // Even though this is passed for both writes and reads, it doesn't harm or alter reads which will still be managed
         // by their main qosIODispatchBackground type arguments.
         Expected( ( UlParam( pinst, JET_paramFlight_NewQueueOptions ) & bitUseMetedQ ) == 0 ); // deprecated all-on SmoothIO replaces for per-session SmoothIO priorities.
         qos |= ( UlParam( pinst, JET_paramFlight_NewQueueOptions ) & bitUseMetedQ ) ? qosIODispatchWriteMeted : 0x0;
    }

    return qos;
}

//  Used only for ErrBFIPrereadPage and Backup _at the moment_.

inline OSFILEQOS QosAsyncReadDefault( INST * pinst )
{
    Assert( pinst != NULL && pinst != pinstNil );

    const ULONG grbitNewQueueOptions = (ULONG)UlParam( pinst, JET_paramFlight_NewQueueOptions );

    Expected( ( grbitNewQueueOptions & bitUseMetedQ ) == 0 ); // deprecated all-on SmoothIO replaces for per-session SmoothIO priorities.
    
    OSFILEQOS qos = ( grbitNewQueueOptions & bitUseMetedQ ) ? qosIODispatchBackground : qosIODispatchImmediate;

    if ( ( pinst != pinstNil ) && ( JET_IOPriorityLow & UlParam( pinst, JET_paramIOPriority ) ) )
    {
        qos |= qosIOOSLowPriority;
    }

    return qos;
}

//  For background read tasks (DbScan, OLDv1, and Scrub) we may use a different Dispatch Priority.

inline BFPriority BfpriBackgroundRead( const IFMP ifmp, const PIB * const ppib )
{
    Assert( ifmp >= cfmpReserved );
    Assert( FMP::FAllocatedFmp( ifmp ) );

    const OSFILEQOS qosIoPri = ( ( UlParam( PinstFromIfmp( ifmp ), JET_paramFlight_NewQueueOptions ) & bitUseMetedQEseTasks ) ?  qosIODispatchBackground : 0 );

    return ppib == ppibNil ?
                BfpriBFMake( PctFMPCachePriority( ifmp ), (BFTEMPOSFILEQOS)qosIoPri ) :
                ( ( qosIoPri == 0 ) ?
                    ppib->BfpriPriority( ifmp ) :
                    BfpriBFMake( ppib->PctCachePriorityPibDbid( g_rgfmp[ ifmp ].Dbid() ), (BFTEMPOSFILEQOS)qosIoPri ) );
}


#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY

//  Global and per-instance counters.

extern PERFInstanceDelayedTotal<HRT>        cIOTotalDhrts[iotypeMax][iofileMax];
extern PERFInstanceDelayedTotal<QWORD>      cIOTotalBytes[iotypeMax][iofileMax];
extern PERFInstanceDelayedTotal<>           cIOTotal[iotypeMax][iofileMax];
extern PERFInstanceDelayedTotal<>           cIOInHeap[iotypeMax][iofileMax];
extern PERFInstanceDelayedTotal<>           cIOAsyncPending[iotypeMax][iofileMax];

//  Per-DB counters.

extern PERFInstance<HRT>        cIOPerDBTotalDhrts[iotypeMax];
extern PERFInstance<QWORD>      cIOPerDBTotalBytes[iotypeMax];
extern PERFInstance<>           cIOPerDBTotal[iotypeMax];
extern PERFInstance<QWORD, fFalse>  cIOPerDBLatencyCount[iotypeMax][iocatMax];
extern PERFInstance<QWORD, fFalse>  cIOPerDBLatencyAve[iotypeMax][iocatMax];
extern PERFInstance<QWORD, fFalse>  cIOPerDBLatencyP50[iotypeMax][iocatMax];
extern PERFInstance<QWORD, fFalse>  cIOPerDBLatencyP90[iotypeMax][iocatMax];
extern PERFInstance<QWORD, fFalse>  cIOPerDBLatencyP99[iotypeMax][iocatMax];
extern PERFInstance<QWORD, fFalse>  cIOPerDBLatencyP100[iotypeMax][iocatMax];
extern PERFInstance<QWORD, fFalse>  cIODatabaseMetedQueueDepth[iofileMax];
extern PERFInstance<QWORD, fFalse>  cIODatabaseMetedOutstandingMax[iofileMax];
extern PERFInstance<QWORD, fFalse>  cIODatabaseAsyncReadPending[iofileMax];

//  FFB counters.

extern PERFInstanceDelayedTotal<HRT>        cFFBTotalDhrts;
extern PERFInstanceDelayedTotal<>           cFFBTotal;

#endif

class CIOThreadInfoTableKey
{
    public:

        CIOThreadInfoTableKey()
            : m_iocontext( NULL )
        {
        }

        CIOThreadInfoTableKey( DWORD_PTR iocontext )
            : m_iocontext( iocontext )
        {
        }

        CIOThreadInfoTableKey( const CIOThreadInfoTableKey &src )
        {
            m_iocontext = src.m_iocontext;
        }

        const CIOThreadInfoTableKey &operator =( const CIOThreadInfoTableKey &src )
        {
            m_iocontext = src.m_iocontext;

            return *this;
        }

    public:

        DWORD_PTR   m_iocontext;
};

class CIOThreadInfoTableEntry
{
    public:

        CIOThreadInfoTableEntry()
            : m_iocontext( NULL ), m_cDatabaseReadsPending( 0 )
        {
        }

        CIOThreadInfoTableEntry( DWORD_PTR iocontext, DWORD cDatabaseReadsPending )
            : m_iocontext( iocontext ), m_cDatabaseReadsPending( cDatabaseReadsPending )
        {
        }

        CIOThreadInfoTableEntry( const CIOThreadInfoTableEntry &src )
        {
            m_iocontext             = src.m_iocontext;
            m_cDatabaseReadsPending = src.m_cDatabaseReadsPending;
        }

        const CIOThreadInfoTableEntry &operator =( const CIOThreadInfoTableEntry &src )
        {
            m_iocontext             = src.m_iocontext;
            m_cDatabaseReadsPending = src.m_cDatabaseReadsPending;

            return *this;
        }

    public:

        DWORD_PTR   m_iocontext;
        DWORD       m_cDatabaseReadsPending;
};

typedef CDynamicHashTable< CIOThreadInfoTableKey, CIOThreadInfoTableEntry > CIOThreadInfoTable;

INLINE CIOThreadInfoTable::NativeCounter IOThreadInfoTableHashIoContext( DWORD_PTR iocontext )
{
    //  this is a TLS* and they are stored on 32 byte aligned boundaries
    return iocontext / 32;
}

inline CIOThreadInfoTable::NativeCounter CIOThreadInfoTable::CKeyEntry::Hash( const CIOThreadInfoTableKey &key )
{
    return IOThreadInfoTableHashIoContext( key.m_iocontext );
}

inline CIOThreadInfoTable::NativeCounter CIOThreadInfoTable::CKeyEntry::Hash() const
{
    return IOThreadInfoTableHashIoContext( m_entry.m_iocontext );
}

inline BOOL CIOThreadInfoTable::CKeyEntry::FEntryMatchesKey( const CIOThreadInfoTableKey &key ) const
{
    return m_entry.m_iocontext == key.m_iocontext;
}

inline void CIOThreadInfoTable::CKeyEntry::SetEntry( const CIOThreadInfoTableEntry &src )
{
    m_entry = src;
}

inline void CIOThreadInfoTable::CKeyEntry::GetEntry( CIOThreadInfoTableEntry * const pdst ) const
{
    *pdst = m_entry;
}

extern CIOThreadInfoTable g_iothreadinfotable;

class CIOFilePerf
    : public IFilePerfAPI
{
    public:
        CIOFilePerf( const INST * const pinst, const IOFILE iofile, const QWORD qwEngineFileId )
            : IFilePerfAPI( DWORD_PTR( iofile ), qwEngineFileId ), m_pinst( pinst )
        {
            Assert( pinstNil != pinst );
        }

        virtual ~CIOFilePerf()      {}

        virtual DWORD_PTR IncrementIOIssue(
            const DWORD     diskQueueDepth,
            const BOOL      fWrite )
        {
            DWORD_PTR       iocontext   = NULL;
            const IOTYPE    iotypeT     = ( fWrite ? iotypeWrite : iotypeRead );
            const IOFILE    iofileT     = IOFILE( m_dwEngineFileType );

            if ( iotypeT == iotypeRead && ( iofileDbAttached == iofileT || iofileDbRecovery == iofileT ))
            {
                TLS* const ptls = Ptls();

                CIOThreadInfoTable::CLock   lock;
                CIOThreadInfoTableKey       key( (DWORD_PTR)ptls );
                CIOThreadInfoTableEntry     entry;

                g_iothreadinfotable.WriteLockKey( key, &lock );
                CIOThreadInfoTable::ERR err = g_iothreadinfotable.ErrRetrieveEntry( &lock, &entry );
                const CIOThreadInfoTableEntry newEntry( key.m_iocontext, ( err == CIOThreadInfoTable::ERR::errSuccess ? entry.m_cDatabaseReadsPending : 0 ) + 1 );
                if ( err == CIOThreadInfoTable::ERR::errSuccess )
                {
                    err = g_iothreadinfotable.ErrReplaceEntry( &lock, newEntry );
                }
                else
                {
                    err = g_iothreadinfotable.ErrInsertEntry( &lock, newEntry );
                }
                iocontext = err == CIOThreadInfoTable::ERR::errSuccess ? key.m_iocontext : NULL;
                g_iothreadinfotable.WriteUnlockKey( &lock );

                ptls->threadstats.cDatabaseReads++;
                ptls->threadstats.cSumDatabaseReadQueueDepthImpact += iocontext ? newEntry.m_cDatabaseReadsPending : 0;
                ptls->threadstats.cSumDatabaseReadQueueDepth += diskQueueDepth;
            }

            return iocontext;
        }

        virtual VOID IncrementIOCompletion(
            const DWORD_PTR iocontext,
            const DWORD     dwDiskNumber,
            const OSFILEQOS qosIo,
            const HRT       dhrtIOElapsed,
            const DWORD     cbTransfer,
            const BOOL      fWrite )
        {
            const IOTYPE    iotypeT = ( fWrite ? iotypeWrite : iotypeRead );
            const IOFILE    iofileT = IOFILE( m_dwEngineFileType );

            if ( iotypeT == iotypeRead && ( iofileDbAttached == iofileT || iofileDbRecovery == iofileT ))
            {
                CIOThreadInfoTable::CLock   lock;
                CIOThreadInfoTableKey       key( iocontext );
                CIOThreadInfoTableEntry     entry;

                g_iothreadinfotable.WriteLockKey( key, &lock );
                CIOThreadInfoTable::ERR err = g_iothreadinfotable.ErrRetrieveEntry( &lock, &entry );
                const CIOThreadInfoTableEntry newEntry( iocontext, ( err == CIOThreadInfoTable::ERR::errSuccess ? entry.m_cDatabaseReadsPending : 0 ) - 1 );
                if ( err == CIOThreadInfoTable::ERR::errSuccess )
                {
                    Assert( newEntry.m_cDatabaseReadsPending >= 0 );
                    err = g_iothreadinfotable.ErrReplaceEntry( &lock, newEntry );
                    Assert( err == CIOThreadInfoTable::ERR::errSuccess );
                }
                g_iothreadinfotable.WriteUnlockKey( &lock );
            }

            //  only update these stats for a real transfer.  a zero byte transfer could be an error or
            //  a dummy completion.  don't pollute the stats with these
            if ( cbTransfer )
            {
                Assert( pinstNil != m_pinst );
                PERFOpt( cIOTotalDhrts[iotypeT][iofileT].Add( m_pinst, dhrtIOElapsed ) );
                PERFOpt( cIOTotalBytes[iotypeT][iofileT].Add( m_pinst, cbTransfer ) );
                PERFOpt( cIOTotal[iotypeT][iofileT].Inc( m_pinst ) );

                if ( iofileDbAttached == iofileT || iofileDbRecovery == iofileT )
                {
                    PERFOpt( cIOTotalDhrts[iotypeT][iofileDbTotal].Add( m_pinst, dhrtIOElapsed ) );
                    PERFOpt( cIOTotalBytes[iotypeT][iofileDbTotal].Add( m_pinst, cbTransfer ) );
                    PERFOpt( cIOTotal[iotypeT][iofileDbTotal].Inc( m_pinst ) );

                    QWORD qwEngineFileId = QwEngineFileId();
                    if ( qwEngineFileId >= g_ifmpMax )
                    {
                        Expected( ( qwEngineFileId == qwLegacyFileID ) ||
                            ( qwEngineFileId == qwSetDbSizeFileID ) ||
                            ( qwEngineFileId == qwDumpingFileID ) ||
                            ( qwEngineFileId == qwDefragFileID ) );
                        qwEngineFileId = 0;
                    }
                    const INT ifmp = (INT)qwEngineFileId;   // the user file ID is the IFMP for database-type files.
                    PERFOpt( cIOPerDBTotalDhrts[iotypeT].Add( ifmp, dhrtIOElapsed ) );
                    PERFOpt( cIOPerDBTotalBytes[iotypeT].Add( ifmp, cbTransfer ) );
                    PERFOpt( cIOPerDBTotal[iotypeT].Inc( ifmp ) );

                    // perhaps should have done a model (instead of cracking it from pinst and ifmp), just do a SetIoStatHisto() 
                    // on CIOFilePerf() and then consume it if set?
                    CIoStats * piolatstatDb;
                    IOCATEGORY iocat = iocatUnknown;
                    switch( iofileT )
                    {
                    case iofileDbAttached:
                        iocat = ( qosIo & qosUserPriorityMarkAsMaintenance ) ? iocatMaintenance : iocatTransactional;
                        break;
                    case iofileDbRecovery:
                        iocat = iocatMaintenance;
                        break;
                    default: AssertSz( fFalse, "Unknown iofile type = %d", iofileT );
                    }
                    Assert( ifmp == 0 || ( FMP::FAllocatedFmp( ifmp ) && ( piolatstatDb = g_rgfmp[ifmp].Piostats( iotypeT ) ) ) );
                    if ( ( ifmp != 0 ) && FMP::FAllocatedFmp( ifmp ) && ( ( piolatstatDb = g_rgfmp[ifmp].Piostats( iotypeT ) ) != NULL ) )
                    {
                        piolatstatDb->AddIoSample( dhrtIOElapsed );
                        if ( piolatstatDb->FStartUpdate() )
                        {
                            Assert( iocat < iocatMax );
                            Expected( iocat == iocatTransactional || iocat == iocatMaintenance ); // no "Unknown" cases at this time.

                            const QWORD cio = piolatstatDb->CioAccumulated();
                            PERFOpt( cIOPerDBLatencyCount[iotypeT][iocat].Set( ifmp, cio ) );
                            if ( cio >= CIoStats::cioMinValidSampleRate )
                            {
                                PERFOpt( cIOPerDBLatencyAve[iotypeT][iocat].Set( ifmp, piolatstatDb->CusecAverage() ) );
                                PERFOpt( cIOPerDBLatencyP50[iotypeT][iocat].Set( ifmp, piolatstatDb->CusecPercentile( 50 ) ) );
                                PERFOpt( cIOPerDBLatencyP90[iotypeT][iocat].Set( ifmp, piolatstatDb->CusecPercentile( 90 ) ) );
                                PERFOpt( cIOPerDBLatencyP99[iotypeT][iocat].Set( ifmp, piolatstatDb->CusecPercentile( 99 ) ) );
                                PERFOpt( cIOPerDBLatencyP100[iotypeT][iocat].Set( ifmp, piolatstatDb->CusecPercentile( 100 ) ) );
                            }
                            else
                            {
                                PERFOpt( cIOPerDBLatencyAve[iotypeT][iocat].Set( ifmp, 0 ) );
                                PERFOpt( cIOPerDBLatencyP50[iotypeT][iocat].Set( ifmp, 0 ) );
                                PERFOpt( cIOPerDBLatencyP90[iotypeT][iocat].Set( ifmp, 0 ) );
                                PERFOpt( cIOPerDBLatencyP99[iotypeT][iocat].Set( ifmp, 0 ) );
                                PERFOpt( cIOPerDBLatencyP100[iotypeT][iocat].Set( ifmp, 0 ) );
                            }

                            //  Note this clears the stats for the next sample, if we had the cioMinSampleRate worth IOs, otherwise
                            //  it continues to accumulate IOs.
                            piolatstatDb->FinishUpdate( dwDiskNumber );
                        }
                    }
                }
                else
                {
                    Assert( iofileDbTotal != iofileT );  // only virtual file type, constructed here for perf counters, not used.
                }
            }
        }

        virtual VOID IncrementIOInHeap( const BOOL fWrite )
        {
            PERFOptDeclare( const IOTYPE    iotypeT     = ( fWrite ? iotypeWrite : iotypeRead ) );
            const IOFILE    iofileT     = IOFILE( m_dwEngineFileType );

            Assert( pinstNil != m_pinst );
            PERFOpt( cIOInHeap[iotypeT][iofileT].Inc( m_pinst ) );
            if ( iofileDbAttached == iofileT || iofileDbRecovery == iofileT )
            {
                PERFOpt( cIOInHeap[iotypeT][iofileDbTotal].Inc( m_pinst ) );
            }
        }

        virtual VOID DecrementIOInHeap( const BOOL fWrite )
        {
            PERFOptDeclare( const IOTYPE    iotypeT     = ( fWrite ? iotypeWrite : iotypeRead ) );
            const IOFILE    iofileT     = IOFILE( m_dwEngineFileType );

            Assert( pinstNil != m_pinst );
            PERFOpt( cIOInHeap[iotypeT][iofileT].Dec( m_pinst ) );
            if ( iofileDbAttached == iofileT || iofileDbRecovery == iofileT )
            {
                PERFOpt( cIOInHeap[iotypeT][iofileDbTotal].Dec( m_pinst ) );
            }
        }

        virtual VOID IncrementIOAsyncPending( const BOOL fWrite )
        {
            PERFOptDeclare( const IOTYPE    iotypeT     = ( fWrite ? iotypeWrite : iotypeRead ) );
            const IOFILE    iofileT     = IOFILE( m_dwEngineFileType );

            Assert( pinstNil != m_pinst );
            PERFOpt( cIOAsyncPending[iotypeT][iofileT].Inc( m_pinst ) );
            if ( iofileDbAttached == iofileT || iofileDbRecovery == iofileT )
            {
                PERFOpt( cIOAsyncPending[iotypeT][iofileDbTotal].Inc( m_pinst ) );
            }
        }

        virtual VOID DecrementIOAsyncPending( const BOOL fWrite )
        {
            PERFOptDeclare( const IOTYPE    iotypeT     = ( fWrite ? iotypeWrite : iotypeRead ) );
            const IOFILE    iofileT     = IOFILE( m_dwEngineFileType );

            Assert( pinstNil != m_pinst );
            PERFOpt( cIOAsyncPending[iotypeT][iofileT].Dec( m_pinst ) );
            if ( iofileDbAttached == iofileT || iofileDbRecovery == iofileT )
            {
                PERFOpt( cIOAsyncPending[iotypeT][iofileDbTotal].Dec( m_pinst ) );
            }
        }

        virtual VOID IncrementFlushFileBuffer( const HRT dhrtElapsed )
        {
            // Do we want to maintain it per filetype?
            PERFOpt( cFFBTotalDhrts.Add( m_pinst, dhrtElapsed ) );
            PERFOpt( cFFBTotal.Inc( m_pinst ) );
        }

        virtual VOID SetCurrentQueueDepths( const LONG cioMetedReadQueue, const LONG cioAllowedMetedOps, const LONG cioAsyncRead )
        {
            const IOFILE    iofileT     = IOFILE( m_dwEngineFileType );
            //  Should we separate recovery and attached or transactional and maintenance ... remains to be seen.
            if ( iofileDbAttached == iofileT || iofileDbRecovery == iofileT )
            {
                Assert( cioAllowedMetedOps > 0 );
                PERFOpt( cIODatabaseMetedQueueDepth[iofileDbTotal].Set( m_pinst->m_iInstance, cioMetedReadQueue ) );
                // This is usually accurate, but occasionally drops to 0, which should not happen - should stay
                // at last value set.  Perhaps input is dropping to zero.
                PERFOpt( cIODatabaseMetedOutstandingMax[iofileDbTotal].Set( m_pinst->m_iInstance, cioAllowedMetedOps ) );
                PERFOpt( cIODatabaseAsyncReadPending[iofileDbTotal].Set( m_pinst->m_iInstance, cioAsyncRead ) );
            }
        }

        static ERR ErrFileOpen(
                        _In_ IFileSystemAPI * const         pfsapi,     // non-const due to volume cache m_critVolumePathCache / m_ilVolumePathCache
                        _In_ const INST * const             pinst,      // used only for perf counters ->m_iInstance
                        _In_z_ const WCHAR * const          wszPath,
                        _In_ const IFileAPI::FileModeFlags  fmf,
                        _In_ const IOFILE                   iofile,
                        _In_ const QWORD                    qwEngineFileId,
                        _Out_ IFileAPI ** const             ppfapi );

        static ERR ErrFileCreate(
                        _In_ IFileSystemAPI * const         pfsapi,     // non-const due to volume cache m_critVolumePathCache / m_ilVolumePathCache
                        _In_ const INST * const             pinst,      // used only for perf counters ->m_iInstance
                        _In_z_ const WCHAR * const          wszPath,
                        _In_ const IFileAPI::FileModeFlags  fmf,
                        _In_ const IOFILE                   iofile,
                        _In_ const QWORD                    qwEngineFileId,
                        _Out_ IFileAPI ** const             ppfapi );

    private:
        const INST * const  m_pinst;
};

inline void IOResetIoLatencyPerfCtrs_( const IFMP ifmp, const IOFILE iofile, const IOTYPE iotype )
{
    Assert( iofile <= iofileDbRecovery );
    Expected( iofile == iofileDbAttached || iofile == iofileDbRecovery );
    PERFOpt( cIOPerDBLatencyAve[iotype][iofile].Set( ifmp, 0 ) );
    PERFOpt( cIOPerDBLatencyP50[iotype][iofile].Set( ifmp, 0 ) );
    PERFOpt( cIOPerDBLatencyP90[iotype][iofile].Set( ifmp, 0 ) );
    PERFOpt( cIOPerDBLatencyP99[iotype][iofile].Set( ifmp, 0 ) );
    PERFOpt( cIOPerDBLatencyP100[iotype][iofile].Set( ifmp, 0 ) );
}

inline void IOResetFmpIoLatencyStats( const IFMP ifmp )
{
    Assert( ifmp != 0 && FMP::FAllocatedFmp( ifmp ) && g_rgfmp[ifmp].Piostats( iotypeWrite ) );
    if ( ifmp != 0 && FMP::FAllocatedFmp( ifmp ) && g_rgfmp[ifmp].Piostats( iotypeRead ) )
    {
        g_rgfmp[ifmp].Piostats( iotypeRead )->Tare();
    }
    if ( ifmp != 0 && FMP::FAllocatedFmp( ifmp ) && g_rgfmp[ifmp].Piostats( iotypeWrite ) )
    {
        g_rgfmp[ifmp].Piostats( iotypeWrite )->Tare();
    }

    IOResetIoLatencyPerfCtrs_( ifmp, iofileDbAttached, iotypeRead );
    IOResetIoLatencyPerfCtrs_( ifmp, iofileDbAttached, iotypeWrite );
    IOResetIoLatencyPerfCtrs_( ifmp, iofileDbRecovery, iotypeRead );
    IOResetIoLatencyPerfCtrs_( ifmp, iofileDbRecovery, iotypeWrite );
}

inline ERR CIOFilePerf::ErrFileOpen(
    _In_ IFileSystemAPI * const         pfsapi,
    _In_ const INST * const             pinst,
    _In_z_ const WCHAR * const          wszPath,
    _In_ const IFileAPI::FileModeFlags  fmf,
    _In_ const IOFILE                   iofile,
    _In_ const QWORD                    qwEngineFileId,
    _Out_ IFileAPI ** const             ppfapi )
{
    ERR                     err;
    IFileAPI *              pfapi       = NULL;
    IFilePerfAPI *          pfpapi      = NULL;

    Assert( pinstNil != pinst );

    //  create IFilePerfAPI object
    //
    Alloc( pfpapi = new CIOFilePerf( pinst, iofile, qwEngineFileId ) );

    //  create IFileAPI object
    //
    err = pfsapi->ErrFileOpen( wszPath, fmf, &pfapi );
    if ( err < JET_errSuccess )
    {
        Assert( pfapi == NULL );
        pfapi = NULL;
        delete pfpapi;
        goto HandleError;
    }

    //  register IFilePerfAPI object with IFileAPI object
    //
    pfapi->RegisterIFilePerfAPI( pfpapi );

HandleError:
    Assert( ( pfapi != NULL ) == ( err >= JET_errSuccess ) );
    *ppfapi = pfapi;
    return err;
}

inline ERR CIOFilePerf::ErrFileCreate(
    _In_ IFileSystemAPI * const         pfsapi,
    _In_ const INST * const             pinst,
    _In_z_ const WCHAR * const          wszPath,
    _In_ const IFileAPI::FileModeFlags  fmf,
    _In_ const IOFILE                   iofile,
    _In_ const QWORD                    qwEngineFileId,
    _Out_ IFileAPI ** const             ppfapi )
{
    ERR                     err;
    IFileAPI *              pfapi       = NULL;
    IFilePerfAPI *          pfpapi      = NULL;

    Assert( pinstNil != pinst );

    //  create IFilePerfAPI object
    //
    Alloc( pfpapi = new CIOFilePerf( pinst, iofile, qwEngineFileId ) );

    //  create IFileAPI object
    //
    err = pfsapi->ErrFileCreate( wszPath, fmf, &pfapi );
    if ( err < JET_errSuccess )
    {
        delete pfpapi;
        goto HandleError;
    }

    //  register IFilePerfAPI object with IFileAPI object
    //
    pfapi->RegisterIFilePerfAPI( pfpapi );
    *ppfapi = pfapi;

HandleError:
    return err;
}

ERR ErrBeginDatabaseIncReseedTracing( _In_ IFileSystemAPI * pfsapi, _In_ JET_PCWSTR wszDatabase, _Out_ CPRINTF ** ppcprintf );
VOID EndDatabaseIncReseedTracing( _Out_ CPRINTF ** ppcprintf );

void IRSCleanUpAllIrsResources( _In_ INST * const pinst );


