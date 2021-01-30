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

ERR ErrIOFlushDatabaseFileBuffers( IFMP ifmp, const IOFLUSHREASON iofr );
#ifdef DEBUG
const static __int64 cioAllowLogRollHeaderUpdates = 4;
BOOL FIOCheckUserDbNonFlushedIos( const INST * const pinst, const __int64 cioPerDbOutstandingLimit = 0, IFMP ifmpTargetedDB = ifmpNil );
#endif

ERR ErrIOReadDbPages( IFMP ifmp, IFileAPI *pfapi, BYTE *pbData, LONG pgnoStart, LONG pgnoEnd, BOOL fCheckPagesOffset, LONG pgnoMost, const TraceContext& tc, BOOL fExtensiveChecks );

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



struct THREADSTATSCOUNTERS
{
#ifdef MINIMAL_FUNCTIONALITY
    DWORD                   dwUnused;
#else
    PERFInstanceLiveTotal<>         cPageReferenced;
    PERFInstanceLiveTotal<>         cPageRead;
    PERFInstanceLiveTotal<>         cPagePreread;
    PERFInstanceLiveTotal<>         cPageDirtied;
    PERFInstanceLiveTotal<>         cPageRedirtied;
    PERFInstanceLiveTotal<>         cLogRecord;
    PERFInstanceLiveTotal<QWORD>    cbLogRecord;
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

            memcpy( &m_threadstatsInitial, &Ptls()->threadstats, sizeof( m_threadstatsInitial ) );
        }
        ~UPDATETHREADSTATSCOUNTERS()
        {
            TabulateCounters();
        }

        VOID TabulateCounters()
        {
            if ( NULL != m_ptscounters )
            {
                JET_THREADSTATS threadstatsFinal;
                memcpy( &threadstatsFinal, &Ptls()->threadstats, sizeof( threadstatsFinal ) );

                PERFOpt( m_ptscounters->cPageReferenced.Add( m_pinst, threadstatsFinal.cPageReferenced - m_threadstatsInitial.cPageReferenced ) );
                PERFOpt( m_ptscounters->cPageRead.Add( m_pinst, threadstatsFinal.cPageRead - m_threadstatsInitial.cPageRead ) );
                PERFOpt( m_ptscounters->cPagePreread.Add( m_pinst, threadstatsFinal.cPagePreread - m_threadstatsInitial.cPagePreread ) );
                PERFOpt( m_ptscounters->cPageDirtied.Add( m_pinst, threadstatsFinal.cPageDirtied - m_threadstatsInitial.cPageDirtied ) );
                PERFOpt( m_ptscounters->cPageRedirtied.Add( m_pinst, threadstatsFinal.cPageRedirtied - m_threadstatsInitial.cPageRedirtied ) );
                PERFOpt( m_ptscounters->cLogRecord.Add( m_pinst, threadstatsFinal.cLogRecord - m_threadstatsInitial.cLogRecord ) );
                PERFOpt( m_ptscounters->cbLogRecord.Add( m_pinst, threadstatsFinal.cbLogRecord - m_threadstatsInitial.cbLogRecord ) );

                m_ptscounters = NULL;
            }
        }
#endif
};



INLINE OSFILEQOS QosSyncDefault( const INST * const pinst )
{
    OSFILEQOS qos = qosIODispatchImmediate;

    if ( ( pinst != pinstNil ) && ( JET_IOPriorityLow & UlParam( pinst, JET_paramIOPriority ) ) )
    {
        qos |= qosIOOSLowPriority;
    }

    

    if ( pinst != pinstNil )
    {
         Expected( ( UlParam( pinst, JET_paramFlight_NewQueueOptions ) & bitUseMetedQ ) == 0 );
         qos |= ( UlParam( pinst, JET_paramFlight_NewQueueOptions ) & bitUseMetedQ ) ? qosIODispatchWriteMeted : 0x0;
    }

    return qos;
}


inline OSFILEQOS QosAsyncReadDefault( INST * pinst )
{
    Assert( pinst != NULL && pinst != pinstNil );

    const ULONG grbitNewQueueOptions = (ULONG)UlParam( pinst, JET_paramFlight_NewQueueOptions );

    Expected( ( grbitNewQueueOptions & bitUseMetedQ ) == 0 );
    
    OSFILEQOS qos = ( grbitNewQueueOptions & bitUseMetedQ ) ? qosIODispatchBackground : qosIODispatchImmediate;

    if ( ( pinst != pinstNil ) && ( JET_IOPriorityLow & UlParam( pinst, JET_paramIOPriority ) ) )
    {
        qos |= qosIOOSLowPriority;
    }

    return qos;
}


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
#else


extern PERFInstanceDelayedTotal<HRT>        cIOTotalDhrts[iotypeMax][iofileMax];
extern PERFInstanceDelayedTotal<QWORD>      cIOTotalBytes[iotypeMax][iofileMax];
extern PERFInstanceDelayedTotal<>           cIOTotal[iotypeMax][iofileMax];
extern PERFInstanceDelayedTotal<>           cIOInHeap[iotypeMax][iofileMax];
extern PERFInstanceDelayedTotal<>           cIOAsyncPending[iotypeMax][iofileMax];


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
                    const INT ifmp = (INT)qwEngineFileId;
                    PERFOpt( cIOPerDBTotalDhrts[iotypeT].Add( ifmp, dhrtIOElapsed ) );
                    PERFOpt( cIOPerDBTotalBytes[iotypeT].Add( ifmp, cbTransfer ) );
                    PERFOpt( cIOPerDBTotal[iotypeT].Inc( ifmp ) );

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
                            Expected( iocat == iocatTransactional || iocat == iocatMaintenance );

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

                            piolatstatDb->FinishUpdate( dwDiskNumber );
                        }
                    }
                }
                else
                {
                    Assert( iofileDbTotal != iofileT );
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
            PERFOpt( cFFBTotalDhrts.Add( m_pinst, dhrtElapsed ) );
            PERFOpt( cFFBTotal.Inc( m_pinst ) );
        }

        virtual VOID SetCurrentQueueDepths( const LONG cioMetedReadQueue, const LONG cioAllowedMetedOps, const LONG cioAsyncRead )
        {
            const IOFILE    iofileT     = IOFILE( m_dwEngineFileType );
            if ( iofileDbAttached == iofileT || iofileDbRecovery == iofileT )
            {
                Assert( cioAllowedMetedOps > 0 );
                PERFOpt( cIODatabaseMetedQueueDepth[iofileDbTotal].Set( m_pinst->m_iInstance, cioMetedReadQueue ) );
                PERFOpt( cIODatabaseMetedOutstandingMax[iofileDbTotal].Set( m_pinst->m_iInstance, cioAllowedMetedOps ) );
                PERFOpt( cIODatabaseAsyncReadPending[iofileDbTotal].Set( m_pinst->m_iInstance, cioAsyncRead ) );
            }
        }

        static ERR ErrFileOpen(
                        _In_ IFileSystemAPI * const         pfsapi,
                        _In_ const INST * const             pinst,
                        _In_z_ const WCHAR * const          wszPath,
                        _In_ const IFileAPI::FileModeFlags  fmf,
                        _In_ const IOFILE                   iofile,
                        _In_ const QWORD                    qwEngineFileId,
                        _Out_ IFileAPI ** const             ppfapi );

        static ERR ErrFileCreate(
                        _In_ IFileSystemAPI * const         pfsapi,
                        _In_ const INST * const             pinst,
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

    Alloc( pfpapi = new CIOFilePerf( pinst, iofile, qwEngineFileId ) );

    err = pfsapi->ErrFileOpen( wszPath, fmf, &pfapi );
    if ( err < JET_errSuccess )
    {
        Assert( pfapi == NULL );
        pfapi = NULL;
        delete pfpapi;
        goto HandleError;
    }

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

    Alloc( pfpapi = new CIOFilePerf( pinst, iofile, qwEngineFileId ) );

    err = pfsapi->ErrFileCreate( wszPath, fmf, &pfapi );
    if ( err < JET_errSuccess )
    {
        delete pfpapi;
        goto HandleError;
    }

    pfapi->RegisterIFilePerfAPI( pfpapi );
    *ppfapi = pfapi;

HandleError:
    return err;
}

ERR ErrBeginDatabaseIncReseedTracing( _In_ IFileSystemAPI * pfsapi, _In_ JET_PCWSTR wszDatabase, _Out_ CPRINTF ** ppcprintf );
VOID EndDatabaseIncReseedTracing( _Out_ CPRINTF ** ppcprintf );

void IRSCleanUpAllIrsResources( _In_ INST * const pinst );


