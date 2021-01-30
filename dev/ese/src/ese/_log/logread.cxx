// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "logstd.hxx"
#include "_dump.hxx"

PERFInstanceDelayedTotal<ULONG, INST, fFalse> cLGRecoveryThrottleIOSmoothing;
LONG RecoveryThrottleIOSmoothingCEFLPv( LONG iInstance, void *pvBuf )
{
    cLGRecoveryThrottleIOSmoothing.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<QWORD, INST, fFalse> cLGRecoveryThrottleIOSmoothingTime;
LONG RecoveryThrottleIOSmoothingTimeCEFLPv( LONG iInstance, void *pvBuf )
{
    cLGRecoveryThrottleIOSmoothingTime.PassTo( iInstance, pvBuf );
    return 0;
}

LOG_READ_BUFFER::LOG_READ_BUFFER( INST * pinst, LOG * pLog, ILogStream * pLogStream, LOG_BUFFER *pLogBuffer )
    : CZeroInit( sizeof( LOG_READ_BUFFER ) ),
      m_pLog( pLog ),
      m_pinst( pinst ),
      m_pLogStream( pLogStream ),
      m_pLogBuffer( pLogBuffer )
{
    m_wszReaderLogName[0] = 0;
    PERFOpt( cLGRecoveryThrottleIOSmoothing.Clear( pinst ) );
    PERFOpt( cLGRecoveryThrottleIOSmoothingTime.Clear( pinst ) );

#ifdef DEBUG
    for ( UINT i=0; i<NUM_SAVED_LOGS; i++ )
    {
        m_SavedLogData[i].m_pbSavedLogBuffer    = NULL;
        m_SavedLogData[i].m_wszSavedLogName[0]  = 0;
    }
#endif


    AssertRTL( LGChecksum::TestChecksumBytes() );
    AssertRTL( LRPAGEMOVE::FUnitTest() );
}

LOG_READ_BUFFER::~LOG_READ_BUFFER()
{
#ifdef DEBUG
    for ( UINT i=0; i<NUM_SAVED_LOGS; i++ )
    {
        if ( m_SavedLogData[i].m_pbSavedLogBuffer != NULL )
        {
            OSMemoryPageFree( m_SavedLogData[i].m_pbSavedLogBuffer );
            m_SavedLogData[i].m_pbSavedLogBuffer = NULL;
        }
    }
#endif

    OSMemoryHeapFree( m_pvAssembledLR[0] );
    m_pvAssembledLR[0] = NULL;
    m_cbAssembledLR[0] = 0;

    OSMemoryHeapFree( m_pvAssembledLR[1] );
    m_pvAssembledLR[1] = NULL;
    m_cbAssembledLR[1] = 0;
}


VOID LOG_READ_BUFFER::GetLgposDuringReading(
    const BYTE * const pb,
    LGPOS * const plgpos
    ) const
{
    Assert( pbNil != pb );
    Assert( pNil != plgpos );
    Assert( pb >= m_pbLGBufMin && pb < m_pbLGBufMax );

    const ULONG log2CbSec = m_pLogStream->Log2CbSec();

    const BYTE  * const pbAligned   = m_pLogStream->PbSecAligned( pb, m_pbLGBufMin );
    const INT   ib                  = ULONG( pb - pbAligned );
    UINT        isec;
    BYTE *      pbRead              = PbReaderGetEndOfData();
    UINT        isecRead            = IsecGetNextReadSector();

    if ( pbAligned > pbRead )
        isec = ULONG( pbRead + m_cbLGBuf - pbAligned ) >> log2CbSec;
    else
        isec = ULONG( pbRead - pbAligned ) >> log2CbSec;

    isec = isecRead - isec;

    plgpos->lGeneration   = m_pLogStream->GetCurrentFileGen();
    plgpos->isec          = (USHORT)isec;
    plgpos->ib            = (USHORT)ib;
}

VOID LOG_READ_BUFFER::GetLgposOfPbNext(LGPOS *plgpos) const
{
    GetLgposDuringReading( m_pbNext, plgpos );
}

VOID LOG_READ_BUFFER::GetLgposOfPbNextNext(LGPOS *plgpos) const
{
    if ( m_pbNextNext == NULL )
    {
        Assert ( FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) );
        *plgpos = lgposMin;
    }
    else
    {
        GetLgposDuringReading( m_pbNextNext, plgpos );
    }
}

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
VOID LOG_READ_BUFFER::GetLgposOfLastChecksum(LGPOS *plgpos) const
{
    *plgpos = m_lgposLastChecksum;
}
#endif


void LOG::PrintLgposReadLR ( VOID )
{
    LGPOS lgpos;
    m_pLogReadBuffer->GetLgposOfPbNext(&lgpos);
    (*CPRINTFSTDOUT::PcprintfInstance())( ">%08X,%04X,%04X", m_pLogStream->GetCurrentFileGen(),lgpos.isec, lgpos.ib);
}


ERR LOG_READ_BUFFER::ErrLReaderInit( UINT csecBufSize )
{
    ERR err;

    Assert( !m_fReaderInited );

    Assert( csecBufSize > 0 );


    m_fReadSectorBySector   = fFalse;

    m_ftFirstSegment = 0;
    m_hrtFirstSegment = 0;

    m_isecReadStart         = 0;
    m_csecReader            = 0;

    m_fIgnoreNextSectorReadFailure  = fTrue;

    m_wszReaderLogName[0]   = 0;


    CallR( m_pLogBuffer->ErrLGInitLogBuffers( m_pinst, m_pLogStream, csecBufSize, fTrue ) );
    CallR( m_pLogBuffer->EnsureCommitted( m_pbLGBufMin, m_cbLGBuf ) );

    m_pbWrite = m_pbLGBufMin;
    m_pbEntry = m_pbWrite;

    m_fReaderInited = fTrue;

    return JET_errSuccess;
}


ERR LOG_READ_BUFFER::ErrLReaderTerm()
{
    m_fReaderInited = fFalse;


    m_pbWrite = m_pbLGBufMin;
    m_pbEntry = m_pbWrite;

    m_wszReaderLogName[0]   = 0;

    return JET_errSuccess;
}



VOID LOG_READ_BUFFER::LReaderReset()
{
#if DEBUG
    if ( m_csecReader != 0 )
    {
        UINT i;
        BOOL fFound = fFalse;
        for ( i=0; i<NUM_SAVED_LOGS; i++ )
        {
            if ( m_SavedLogData[i].m_pbSavedLogBuffer != NULL &&
                 UtilCmpFileName( m_wszReaderLogName, m_SavedLogData[i].m_wszSavedLogName ) == 0 &&
                 m_SavedLogData[i].m_iSavedsecStart <= m_isecReadStart &&
                 ( m_SavedLogData[i].m_iSavedsecStart + m_SavedLogData[i].m_cSavedsec ) >= ( m_isecReadStart + m_csecReader ) )
            {
                fFound = fTrue;
                break;
            }
        }

        if ( !fFound )
        {
            if ( m_SavedLogData[m_cNextSavedLog].m_pbSavedLogBuffer != NULL &&
                 m_SavedLogData[m_cNextSavedLog].m_cSavedsec != m_csecReader )
            {
                OSMemoryPageFree( m_SavedLogData[m_cNextSavedLog].m_pbSavedLogBuffer );
                m_SavedLogData[m_cNextSavedLog].m_pbSavedLogBuffer = NULL;
            }

            if ( m_SavedLogData[m_cNextSavedLog].m_pbSavedLogBuffer == NULL )
            {
                m_SavedLogData[m_cNextSavedLog].m_pbSavedLogBuffer =
                    PvOSMemoryPageAlloc( m_csecReader * m_pLogStream->CbSec(), NULL );
            }

            if ( m_SavedLogData[m_cNextSavedLog].m_pbSavedLogBuffer != NULL )
            {
                memcpy( m_SavedLogData[m_cNextSavedLog].m_pbSavedLogBuffer,
                        m_pbLGBufMin,
                        m_csecReader * m_pLogStream->CbSec() );
                m_SavedLogData[m_cNextSavedLog].m_iSavedsecStart = m_isecReadStart;
                m_SavedLogData[m_cNextSavedLog].m_cSavedsec = m_csecReader;
                OSStrCbCopyW( m_SavedLogData[m_cNextSavedLog].m_wszSavedLogName,
                              sizeof(m_SavedLogData[m_cNextSavedLog].m_wszSavedLogName),
                              m_wszReaderLogName );
                m_cNextSavedLog = ( m_cNextSavedLog + 1 ) % NUM_SAVED_LOGS;
            }
        }
    }
#endif


    m_pbWrite = m_pbLGBufMin;
    m_pbEntry = m_pbWrite;


    m_ftFirstSegment = 0;
    m_hrtFirstSegment = 0;

    m_fReadSectorBySector   = fFalse;

    m_isecReadStart             = 0;
    m_csecReader                = 0;

    m_fIgnoreNextSectorReadFailure  = fTrue;
}


ERR LOG_READ_BUFFER::ErrReaderEnsureLogFile()
{

    if ( 0 == UtilCmpFileName( m_pLogStream->LogName(), m_wszReaderLogName ) )
    {
    }
    else
    {
        LReaderReset();

        OSStrCbCopyW( m_wszReaderLogName, sizeof( m_wszReaderLogName ), m_pLogStream->LogName() );
    }

    return JET_errSuccess;
}


ERR LOG_READ_BUFFER::ErrReaderEnsureSector(
    UINT    isec,
    UINT    csec,
    BYTE**  ppb
    )
{
    ERR err = JET_errSuccess;

    Assert( isec >= m_pLogStream->CSecHeader() );
    Assert( csec > 0 );
    Expected( m_isecReadStart == m_pLogStream->CSecHeader() || ( m_isecReadStart == 0 && isec == m_pLogStream->CSecHeader() ) );

    *ppb = pbNil;

    if ( isec >= m_isecReadStart && isec + csec <= m_isecReadStart + m_csecReader )
    {
        *ppb = m_pbLGBufMin +
            ( isec - m_isecReadStart ) * m_pLogStream->CbSec();
        return JET_errSuccess;
    }

    if ( m_csecLGBuf < csec )
    {
        Call( m_pLogBuffer->ErrLGInitLogBuffers( m_pinst, m_pLogStream, csec, fTrue ) );
        m_pbWrite = m_pbLGBufMin;
        m_pbEntry = m_pbWrite;
    }

LReadSectorBySector:
    
    if ( m_fReadSectorBySector )
    {
        if ( m_csecReader == 0 || m_isecReadStart > isec )
        {
            m_csecReader = 0;
            m_isecReadStart = isec;
        }

        for ( UINT ib = isec * m_pLogStream->CbSec();
            m_csecReader < ( ( isec - m_isecReadStart ) + csec );
            ib += m_pLogStream->CbSec(), m_csecReader++ )
        {
            err = m_pLogStream->ErrLGReadSectorData(
                    ib,
                    m_pLogStream->CbSec(),
                    m_pbLGBufMin + ( m_csecReader * m_pLogStream->CbSec() ) );
            if ( err < 0 )
            {
                if ( !m_fIgnoreNextSectorReadFailure )
                {
                    Call( ErrERRCheck( err ) );
                }
                m_fIgnoreNextSectorReadFailure = fFalse;
                memset( m_pbLGBufMin + m_csecReader * m_pLogStream->CbSec(),
                        FLGVersionZeroFilled( &m_pLogStream->GetCurrentFileHdr()->lgfilehdr ) ? 0xFF : 0,
                        m_pLogStream->CbSec() );
            }
        }
    }
    else
    {
        QWORD   qwSize;
        Call( m_pLogStream->ErrFileSize( &qwSize ) );
        UINT    csecLeftInFile  = UINT( ( qwSize - ( QWORD( isec ) * m_pLogStream->CbSec() ) ) / m_pLogStream->CbSec() );
        Assert( csec <= csecLeftInFile );
        UINT    csecRead        = min( csecLeftInFile, m_csecLGBuf );
        csecRead = max( csecRead, csec );


        const DWORD cbReadMax   = (DWORD)UlParam( JET_paramMaxCoalesceReadSize );
        QWORD       ib          = QWORD( isec ) * m_pLogStream->CbSec();
        DWORD       cb          = csecRead * m_pLogStream->CbSec();
        BYTE        *pb         = m_pbLGBufMin;

        while ( cb )
        {


            const DWORD cbRead = min( cb, cbReadMax );

            err = ErrFaultInjection( 39546 );
            if ( err < 0 )
            {
                m_fReadSectorBySector = fTrue;
                goto LReadSectorBySector;
            }


            err = m_pLogStream->ErrLGReadSectorData( ib, cbRead, pb );
            if ( err < 0 )
            {
                m_fReadSectorBySector = fTrue;
                goto LReadSectorBySector;
            }


            ib += cbRead;
            cb -= cbRead;
            pb += cbRead;


            Assert( ( cb > 0 && pb < m_pbLGBufMax ) || ( 0 == cb && pb <= m_pbLGBufMax ) );
        }

        m_isecReadStart = isec;
        m_csecReader = csecRead;
    }

    m_pbWrite = m_pbLGBufMin;
    m_pbEntry = m_pbWrite + m_csecReader * m_pLogStream->CbSec();
    if ( m_pbEntry == m_pbLGBufMax )
    {
        m_pbEntry = m_pbLGBufMin;
    }

    Assert( isec >= m_isecReadStart );
    Assert( isec + csec <= m_isecReadStart + m_csecReader );
    *ppb = m_pbLGBufMin +
            ( isec - m_isecReadStart ) * m_pLogStream->CbSec();

HandleError:

    return err;
}


VOID LOG_READ_BUFFER::ReaderSectorModified(
    const UINT isec,
    const BYTE* const pb
    )
{
    Assert( isec >= m_pLogStream->CSecHeader() );
    Assert( pbNil != pb );

    if ( isec >= m_isecReadStart && isec + 1 <= m_isecReadStart + m_csecReader )
    {
        BYTE* const pbDest = m_pbLGBufMin + ( isec - m_isecReadStart ) * m_pLogStream->CbSec();
        if ( pbDest != pb )
        {
            UtilMemCpy( pbDest, pb, m_pLogStream->CbSec() );
        }
    }
}

BYTE* LOG_READ_BUFFER::PbReaderGetEndOfData() const
{
    BYTE*   pb = m_pbLGBufMin + m_csecReader * m_pLogStream->CbSec();
    Assert( pb >= m_pbLGBufMin && pb <= m_pbLGBufMax );
    return pb;
}

UINT LOG_READ_BUFFER::IsecGetNextReadSector() const
{
    return m_isecReadStart + m_csecReader;
}

ERR
LOG_READ_BUFFER::ErrReaderRestoreState(
    const LGPOS* const plgposPbNext,
    const LGPOS* const plgposPbNextNext
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    ,const LGPOS* const plgposChecksum
#endif
    )
{
    ERR err;
    BYTE *pb;
    USHORT minSec, maxSec;

    minSec = plgposPbNext->isec;
    if ( plgposPbNextNext->isec != 0 )
    {
        minSec = min( minSec, plgposPbNextNext->isec );
    }
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    if ( plgposChecksum->isec != 0 )
    {
        minSec = min( minSec, plgposChecksum->isec );
    }
#endif
    maxSec = max( plgposPbNext->isec, plgposPbNextNext->isec );
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    maxSec = max( maxSec, plgposChecksum->isec );
#endif

    CallR( ErrReaderEnsureSector( minSec, maxSec - minSec + 1, &pb ) );

    m_pbNext = pb + m_pLogStream->CbSec() * ( plgposPbNext->isec - minSec ) + plgposPbNext->ib;
    Assert( m_pbNext >= m_pbLGBufMin && m_pbNext < m_pbLGBufMax );

    if ( plgposPbNextNext->isec != 0 )
    {
        m_pbNextNext = pb + m_pLogStream->CbSec() * ( plgposPbNextNext->isec - minSec ) + plgposPbNextNext->ib;
        Assert( m_pbNextNext >= m_pbLGBufMin && m_pbNextNext < m_pbLGBufMax );
    }

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    if ( plgposChecksum->isec != 0 )
    {
        m_pbLastChecksum = pb + m_pLogStream->CbSec() * ( plgposChecksum->isec - minSec ) + plgposChecksum->ib;
        Assert( m_pbLastChecksum >= m_pbLGBufMin && m_pbLastChecksum < m_pbLGBufMax );

        m_lgposLastChecksum = *plgposChecksum;
    }
#endif

    return JET_errSuccess;
}



LogPrereaderBase::LogPrereaderBase() :
    m_dbidMaxUsed( 0 ),
    m_cpgGrowth( 0 ),
    m_rgArrayPgnos( NULL )
{
}

LogPrereaderBase::~LogPrereaderBase()
{
    Expected( !FLGPEnabled() );
    LGPTerm();
}


VOID LogPrereaderBase::LGPInit( const DBID dbidMaxUsed, const CPG cpgGrowth )
{
    Assert( !FLGPEnabled() );
    Assert( dbidMaxUsed > 0 && cpgGrowth > 0 );
    Assert( m_rgArrayPgnos == NULL || m_dbidMaxUsed > 0 );
    Assert( m_rgArrayPgnos == NULL || m_cpgGrowth > 0 );
    m_dbidMaxUsed = dbidMaxUsed;
    m_cpgGrowth = cpgGrowth;
    m_rgArrayPgnos = new CArray<PGNO>[ m_dbidMaxUsed ];
    Assert( m_rgArrayPgnos == NULL || FLGPEnabled() );

    if ( m_rgArrayPgnos != NULL )
    {
        for ( DBID dbid = dbidMin; dbid < m_dbidMaxUsed; dbid++ )
        {
            m_rgArrayPgnos[ dbid ].SetCapacityGrowth( m_cpgGrowth );
        }
    }
}


VOID LogPrereaderBase::LGPTerm()
{
    if ( !FLGPEnabled() )
    {
        return;
    }

    for ( DBID dbid = dbidMin; dbid < m_dbidMaxUsed; dbid++ )
    {
        LGPDBDisable( dbid );
        Assert( !FLGPDBEnabled( dbid ) );
    }

    delete[] m_rgArrayPgnos;
    m_rgArrayPgnos = NULL;
    Assert( !FLGPEnabled() );
}


VOID LogPrereaderBase::LGPDBEnable( const DBID dbid )
{
    Assert( m_dbidMaxUsed > 0 );
    Assert( dbid < m_dbidMaxUsed );
    
    if ( FLGPDBEnabled( dbid ) )
    {
        const CArray<PGNO>::ERR errSetSize = m_rgArrayPgnos[ dbid ].ErrSetSize( 0 );
        Assert( errSetSize == CArray<PGNO>::ERR::errSuccess );
        Assert( FLGPDBEnabled( dbid ) );
    }
    else
    {
        if ( !FLGPEnabled() )
        {
            LGPInit( m_dbidMaxUsed, m_cpgGrowth );
        }

        if ( !FLGPEnabled() )
        {
            return;
        }

        (VOID)m_rgArrayPgnos[ dbid ].ErrSetCapacity( m_cpgGrowth );
    }

    Assert( CpgLGPIGetArrayPgnosSize( dbid ) == 0 );
}


VOID LogPrereaderBase::LGPDBDisable( const DBID dbid )
{
    if ( !FLGPDBEnabled( dbid ) )
    {
        return;
    }

    const CArray<PGNO>::ERR errSetCapacity = m_rgArrayPgnos[ dbid ].ErrSetCapacity( 0 );
    Assert( errSetCapacity == CArray<PGNO>::ERR::errSuccess );
    Assert( !FLGPDBEnabled( dbid ) );
}

INLINE BOOL LogPrereaderBase::FLGPEnabled() const
{
    return m_rgArrayPgnos != NULL;
}

INLINE BOOL LogPrereaderBase::FLGPDBEnabled( const DBID dbid ) const
{
    if ( !FLGPEnabled() )
    {
        return fFalse;
    }

    return m_rgArrayPgnos[ dbid ].Capacity() > 0;
}

ERR LogPrereaderBase::ErrLGPAddPgnoRef( const DBID dbid, const PGNO pgno )
{
    ERR err = JET_errSuccess;

    if ( !FLGPDBEnabled( dbid ) || pgno == pgnoNull )
    {
        goto HandleError;
    }

    Call( ErrLGPISetEntry( dbid, CpgLGPIGetArrayPgnosSize( dbid ), pgno ) );

HandleError:

    return err;
}

VOID LogPrereaderBase::LGPSortPages()
{
    if ( !FLGPEnabled() )
    {
        return;
    }

    for ( DBID dbid = dbidMin; dbid < m_dbidMaxUsed; dbid++ )
    {
        if ( FLGPDBEnabled( dbid ) )
        {
            m_rgArrayPgnos[ dbid ].SortAndRemoveDuplicates( LogPrereaderBase::ILGPICmpPgno );
        }
    }
}

ERR LogPrereaderBase::ErrLGPPrereadExtendedPageRange( const DBID dbid, const PGNO pgno, CPG* const pcpgPreread, const BFPreReadFlags bfprf )
{
    ERR err = JET_errSuccess;

    Assert( pcpgPreread != NULL );
    *pcpgPreread = 0;

    if ( !FLGPDBEnabled( dbid ) )
    {
        goto HandleError;
    }

    const size_t ipgMain = IpgLGPGetSorted( dbid, pgno );

    if ( ipgMain == CArray<PGNO>::iEntryNotFound )
    {
        ExpectedSz( fFalse, "We must have added this page first!" );
        Call( ErrERRCheck( JET_errInternalError ) );
    }

    Assert( PgnoLGPIGetEntry( dbid, ipgMain ) == pgno );

    const size_t cpg = CpgLGPIGetArrayPgnosSize( dbid );
    Assert( cpg > 0 );

    if ( ipgMain > ulMax || cpg > ulMax )
    {
        ExpectedSz( fFalse, "We can't have a database this big!" );
        Call( ErrERRCheck( JET_errInternalError ) );
    }

    const __int64 ipgInitial = (__int64)ipgMain;
    __int64 ipg = ipgInitial;
    __int64 dipg = 0;
    const __int64 ipgMax = (__int64)cpg;
    bool fMayGoRight = true;
    bool fMayGoLeft = true;
    OnDebug( bool fOscillating = true );

    do
    {
        Assert( ipg >= 0 && ipg < ipgMax );
        
        const BFPreReadFlags bfprfCombinable = ( ipg == ipgInitial ) ? bfprfDefault : bfprfCombinableOnly;

        const PGNO pgnoThisPass = PgnoLGPIGetEntry( dbid, (size_t)ipg );

        Expected( pgnoThisPass != pgnoNull );

        err = ErrLGPIPrereadPage( dbid, pgnoThisPass, BFPreReadFlags( bfprfNoIssue | bfprfCombinable | bfprf ) );

        if ( err >= JET_errSuccess )
        {
            (*pcpgPreread)++;
        }
        else
        {
            if ( ipg <= ipgInitial )
            {
                fMayGoLeft = false;
            }
            if ( ipg >= ipgInitial )
            {
                fMayGoRight = false;
            }

            if ( ( err == errDiskTilt || err == errBFPageCached ) && ipg != ipgInitial )
            {
                err = JET_errSuccess;
            }
        }

        if ( ipg == 0 )
        {
            fMayGoLeft = false;
        }
        if ( ipg == ( ipgMax - 1 ) )
        {
            fMayGoRight = false;
        }

        if ( dipg > 0 )
        {
            if ( fMayGoLeft )
            {
                Assert( fOscillating );
                dipg = -( dipg + 1 );
            }
            else
            {
                dipg = +1;
                OnDebug( fOscillating = false );
            }
        }
        else
        {
            if ( fMayGoRight )
            {
                Assert( fOscillating );
                dipg = -( dipg - 1 );
            }
            else
            {
                dipg = -1;
                OnDebug( fOscillating = false );
            }
        }

        ipg += dipg;
    } while ( ( err >= JET_errSuccess ) && ( fMayGoLeft || fMayGoRight ) );

HandleError:

    Assert( err != errDiskTilt );

    return err;
}

INLINE size_t LogPrereaderBase::CpgLGPIGetArrayPgnosSize( const DBID dbid ) const
{
    Assert( FLGPEnabled() );

    return m_rgArrayPgnos[ dbid ].Size();
}

INLINE size_t LogPrereaderBase::IpgLGPGetSorted( const DBID dbid, const PGNO pgno ) const
{
    Assert( FLGPDBEnabled( dbid ) );

    return m_rgArrayPgnos[ dbid ].SearchBinary( pgno, LogPrereaderBase::ILGPICmpPgno );
}

size_t LogPrereaderBase::IpgLGPIGetUnsorted( const DBID dbid, const PGNO pgno ) const
{
    Assert( FLGPDBEnabled( dbid ) );

    return m_rgArrayPgnos[ dbid ].SearchLinear( pgno, LogPrereaderBase::ILGPICmpPgno );
}

ERR LogPrereaderBase::ErrLGPISetEntry( const DBID dbid, const size_t ipg, const PGNO pgno )
{
    Assert( FLGPDBEnabled( dbid ) );

    ERR err;

    const CArray<PGNO>::ERR errT = m_rgArrayPgnos[ dbid ].ErrSetEntry( ipg, pgno );

    switch ( errT )
    {
        case CArray<PGNO>::ERR::errSuccess:
            err = JET_errSuccess;
            break;
        
        case CArray<PGNO>::ERR::errOutOfMemory:
            err = ErrERRCheck( JET_errOutOfMemory );
            break;
        
        default:
            err = ErrERRCheck( JET_errInternalError );
            AssertSz( fFalse, "CArray::ERR %d is not recognized", errT );
            break;
    }

    return err;
}

PGNO LogPrereaderBase::PgnoLGPIGetEntry( const DBID dbid, const size_t ipg ) const
{
    Assert( FLGPDBEnabled( dbid ) );
    return m_rgArrayPgnos[ dbid ].Entry( ipg );
}

INLINE INT __cdecl LogPrereaderBase::ILGPICmpPgno( const PGNO* ppgno1, const PGNO* ppgno2 )
{
    const PGNO pgno1 = *ppgno1;
    const PGNO pgno2 = *ppgno2;

    if ( pgno1 == pgno2 )
    {
        return 0;
    }
    else
    {
        return ( ( pgno1 < pgno2 ) ? -1 : +1 );
    }
}

PCWSTR g_rgwszLogCorruptReason[] =
{
    L"Unknown",
    L"EmptySegmentHigherLgen",
    L"CorruptAfterEmptySegment",
    L"BadSegmentLgpos",
    L"ValidSegmentAfterEmpty",
    L"IOError",
    L"SegmentCorrupt",
    L"CannotPatchHardRecovery",
    L"CorruptHeader"
};
C_ASSERT( _countof( g_rgwszLogCorruptReason ) == eLCMax );

LOCAL VOID LGIReportChecksumMismatch(
    const INST *    pinst,
    PCWSTR          wszLogName,
    const USHORT    isecLastValid,
    const USHORT    isecCurrent,
    const _int64    checksumExpected,
    const _int64    checksumActual,
    const ERR       err,
    const eLogCorruptReason reason )
{
    const WCHAR *   rgpszT[ 7 ];
    DWORD           irgpsz      = 0;
    WCHAR           szLastValid[ 8 ];
    WCHAR           szCurrent[ 8 ];
    WCHAR           szChecksumExpected[ 64 ];
    WCHAR           szChecksumActual[ 64 ];
    WCHAR           szError[ 32 ];

    OSStrCbFormatW( szLastValid, sizeof(szLastValid), L"%hu", isecLastValid );
    OSStrCbFormatW( szCurrent, sizeof(szCurrent), L"%hu", isecCurrent );
    OSStrCbFormatW( szChecksumExpected, sizeof(szChecksumExpected), L"%I64u (0x%I64x)", checksumExpected, checksumExpected );
    OSStrCbFormatW( szChecksumActual, sizeof(szChecksumActual), L"%I64u (0x%I64x)", checksumActual, checksumActual );
    OSStrCbFormatW( szError, sizeof(szError), L"%i (0x%08x)", err, err );
    
    rgpszT[ irgpsz++ ] = wszLogName;
    rgpszT[ irgpsz++ ] = g_rgwszLogCorruptReason[ reason < eLCMax ? reason : 0 ];
    rgpszT[ irgpsz++ ] = szLastValid;
    rgpszT[ irgpsz++ ] = szCurrent;
    rgpszT[ irgpsz++ ] = szChecksumExpected;
    rgpszT[ irgpsz++ ] = szChecksumActual;
    rgpszT[ irgpsz++ ] = szError;
    
    UtilReportEvent(
            eventError,
            LOGGING_RECOVERY_CATEGORY,
            LOG_CORRUPT_ID,
            irgpsz,
            rgpszT,
            0,
            NULL,
            pinst );

    OSUHAPublishEvent(
        HaDbFailureTagRecoveryRedoLogCorruption, pinst, HA_LOGGING_RECOVERY_CATEGORY,
        HaDbIoErrorNone, wszLogName, 0, 0,
        HA_LOG_CORRUPT_ID, irgpsz, rgpszT );
}


ERR
LOG_VERIFY_STATE::ErrVerifyHeader( INST * pinst, IFileAPI * pfapi, __deref_in_bcount(*pcb) BYTE ** ppb, DWORD *pcb )
{
    ERR err = JET_errSuccess;

    PAGECHECKSUM    checksumExpected    = 0;
    PAGECHECKSUM    checksumActual      = 0;

    if ( *pcb < sizeof( LGFILEHDR ) )
    {
        Error( ErrERRCheck( JET_errLogReadVerifyFailure ) );
    }
    const LGFILEHDR* const plgfilehdr = reinterpret_cast< const LGFILEHDR* >( *ppb );

    ChecksumPage(
                plgfilehdr,
                sizeof( LGFILEHDR ),
                logfileHeader,
                0,
                &checksumExpected,
                &checksumActual );

    if ( checksumExpected != checksumActual )
    {
        WCHAR szAbsPath[ IFileSystemAPI::cchPathMax ];
        szAbsPath[0] = 0;
        pfapi->ErrPath( szAbsPath );
        LGIReportChecksumMismatch(
                pinst,
                szAbsPath,
                0, 0,
                checksumExpected.rgChecksum[ 0 ],
                checksumActual.rgChecksum[ 0 ],
                JET_errSuccess,
                eLCCorruptHeader );

        Error( ErrERRCheck( JET_errLogReadVerifyFailure ) );
    }

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    if ( FIsOldLrckLogFormat( plgfilehdr->lgfilehdr.le_ulMajor ) )
    {
        *ppb += sizeof( LGFILEHDR );
        *pcb -= sizeof( LGFILEHDR );

        m_state = LogVerifyLRCKStart;
        m_lGeneration = plgfilehdr->lgfilehdr.le_lGeneration;
        m_cbSeen = sizeof( LGFILEHDR );
        m_ibLRCK = sizeof( LGFILEHDR );
        m_ibChecksumIncremental = sizeof( LGFILEHDR ) + sizeof( LRCHECKSUM );
    }
    else
#endif
    {
        m_state = LogVerifyLogSegments;
        m_cbSeg = plgfilehdr->lgfilehdr.le_cbSec;
        *ppb += plgfilehdr->lgfilehdr.le_csecHeader * m_cbSeg;
        *pcb -= plgfilehdr->lgfilehdr.le_csecHeader * m_cbSeg;
        m_iSeg = plgfilehdr->lgfilehdr.le_csecHeader;
    }

HandleError:
    return err;
}

ERR
LOG_VERIFY_STATE::ErrVerifyLogSegments(
    _Inout_updates_bytes_(cbBuffer) BYTE *  pb,
    DWORD   cbBuffer )
{
    ERR err = JET_errSuccess;
    PAGECHECKSUM checksumExpected;
    PAGECHECKSUM checksumActual;
    BOOL fCorrectableError;
    INT ibitCorrupted;

    while ( cbBuffer != 0 )
    {
        if ( cbBuffer < m_cbSeg )
        {
            Error( ErrERRCheck( JET_errLogReadVerifyFailure ) );
        }

        if ( FUtilZeroed( pb, m_cbSeg ) )
        {
        }
        else
        {
            ChecksumAndPossiblyFixPage(
                pb,
                m_cbSeg,
                logfilePage,
                m_iSeg,
                fTrue,
                &checksumExpected,
                &checksumActual,
                &fCorrectableError,
                &ibitCorrupted );
            if ( checksumExpected != checksumActual )
            {
                ChecksumAndPossiblyFixPage(
                    pb,
                    m_cbSeg,
                    logfilePage,
                    m_iSeg - 1,
                    fTrue,
                    &checksumExpected,
                    &checksumActual,
                    &fCorrectableError,
                    &ibitCorrupted );
                if ( checksumExpected != checksumActual )
                {
                    Error( ErrERRCheck( JET_errLogReadVerifyFailure ) );
                }
            }
        }

        pb += m_cbSeg;
        cbBuffer -= m_cbSeg;
        m_iSeg++;
    }

HandleError:
    return err;
}

ERR ErrLGReadAndVerifyFile(
    INST *              pinst,
    IFileAPI *          pfapi,
    DWORD               cbOffset,
    DWORD               cbFileSize,
    LOG_VERIFY_STATE *  pState,
    const TraceContext& tc,
    BYTE *              pb,
    DWORD               cbToRead )
{
    ERR err = JET_errSuccess;
    DWORD cbCurrentOffset = cbOffset;
    DWORD cbReadRemaining = cbToRead;
    DWORD cbBuffer = cbToRead;

    while ( cbReadRemaining )
    {
        DWORD cbRead = min( cbReadRemaining, (DWORD)UlParam( JET_paramMaxCoalesceReadSize ) );
        Call( pfapi->ErrIORead( tc, cbCurrentOffset, cbRead, pb + (cbCurrentOffset - cbOffset), QosSyncDefault( pinst ) ) );
        cbCurrentOffset += cbRead;
        cbReadRemaining -= cbRead;
    }

    if ( BoolParam( JET_paramDisableBlockVerification ) )
    {
        pState->m_state = LogVerifyDone;
        return JET_errSuccess;
    }

    do
    {
        switch ( pState->m_state )
        {
            case LogVerifyHeader:
                Call( pState->ErrVerifyHeader( pinst, pfapi, &pb, &cbBuffer ) );
                if ( pState->m_state != LogVerifyLogSegments )
                {
                    break;
                }
                __fallthrough;

            case LogVerifyLogSegments:
                err = pState->ErrVerifyLogSegments( pb, cbBuffer );
                goto HandleError;

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
            case LogVerifyLRCKStart:
                Call( pState->ErrVerifyLRCKStart( pb, cbBuffer ) );
                break;

            case LogVerifyLRCKContinue:
                Call( pState->ErrVerifyLRCKContinue( pb, cbBuffer ) );
                break;
#endif

            case LogVerifyDone:
                break;

            default:
                Assert( fFalse );
        }
    }
    while (
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
            pState->m_ibLRCK < pState->m_cbSeen + cbBuffer &&
            pState->m_state != LogVerifyLRCKContinue &&
#endif
            pState->m_state != LogVerifyDone );

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    if ( pState->m_state != LogVerifyDone )
    {
        if ( cbToRead + cbOffset == cbFileSize )
        {
            Error( ErrERRCheck( JET_errLogReadVerifyFailure ) );
        }

        pState->m_cbSeen += cbBuffer;
    }
#endif

HandleError:
    return err;
}

VOID
LOG_READ_BUFFER::ReportSegmentCorrection_( USHORT isec, ULONG cbOffset, INT ibitCorrupted )
{
    const WCHAR* rgpsz[4];
    DWORD irgpsz = 0;
    WCHAR szIsec[8];
    WCHAR szOffset[32];
    WCHAR szIbitCorrupted[16];

    OSStrCbFormatW( szIsec, sizeof(szIsec), L"%hu", isec );
    OSStrCbFormatW( szOffset, sizeof(szOffset), L"%u (0x%08x)", cbOffset, cbOffset );
    OSStrCbFormatW( szIbitCorrupted, sizeof(szIbitCorrupted), L"%d", ibitCorrupted );

    rgpsz[ irgpsz++ ] = m_pLogStream->LogName();
    rgpsz[ irgpsz++ ] = szIsec;
    rgpsz[ irgpsz++ ] = szOffset;
    rgpsz[ irgpsz++ ] = szIbitCorrupted;

    UtilReportEvent( eventWarning,
                     LOGGING_RECOVERY_CATEGORY,
                     LOG_SEGMENT_CHECKSUM_CORRECTED_ID,
                     irgpsz,
                     rgpsz,
                     0,
                     NULL,
                     m_pinst );
}





ERR
LOG_READ_BUFFER::ErrLGCheckReadLastLogRecordFF(
    BOOL                    *pfCloseNormally,
    const BOOL              fUpdateAccSegChecksum,
    const BOOL              fReadOnly,
    BOOL                    *pfIsPatchable,
    LGPOS                   *plgposLastTerm )
{
    ERR err = JET_errSuccess;
    ERR errT = JET_errSuccess;
    LGPOS lgposCurrent = lgposMin;
    BOOL fJetLog = fFalse;
    BOOL fCreatedLogReader = fFalse;
    BYTE *pbSegment = NULL;
    BOOL fEmptyPageSeen = fFalse;
    LONG lGenLastEmptyPage = m_pLogStream->GetCurrentFileGen();
    BYTE *pbLastValidSegment = NULL;
    LGPOS lgposLastValidSegment = lgposMin;
    LGSEGHDR *pSegHdr = NULL;
    BOOL fCorrectableError;
    INT ibitCorrupted;
    BOOL fFileOpenedForWrite = fFalse;
    PAGECHECKSUM checksumExpected, checksumExpectedT;
    PAGECHECKSUM checksumActual, checksumActualT;
    LONG lLine = 0;
    BOOL fIsTornWrite = fFalse;
    BYTE *pbLR;
    BOOL fGotQuit = fFalse;
    USHORT ibQuit = 0;
    eLogCorruptReason reason = eLCUnknown;

    Assert( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec == m_pLogStream->CbSec() );

    if ( fUpdateAccSegChecksum )
    {
        m_pLogStream->ResetAccumulatedSectorChecksum();
    }

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    if ( FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
    {
        return ErrLGCheckReadLastLogRecordFF_Legacy(
                pfCloseNormally,
                fReadOnly,
                pfIsPatchable,
                plgposLastTerm );
    }
#endif

    Assert( m_pLogStream->CbSec() == LOG_SEGMENT_SIZE );


    Assert( !pfIsPatchable || m_pLog->FDumpingLogs() );


    lgposCurrent.ib             = 0;
    lgposCurrent.isec           = USHORT( m_pLogStream->CSecHeader() );
    lgposCurrent.lGeneration    = m_pLogStream->GetCurrentFileGen();

    m_lgposLastRec = lgposCurrent;

    lgposLastValidSegment.ib            = 0;
    lgposLastValidSegment.isec          = (USHORT)(m_pLogStream->CSecHeader() - 1);
    lgposLastValidSegment.lGeneration   = m_pLogStream->GetCurrentFileGen();

    if ( pfIsPatchable )
    {
        *pfIsPatchable = fFalse;
    }

    *pfCloseNormally = fFalse;
    if ( plgposLastTerm != NULL )
    {
        *plgposLastTerm = lgposMin;
    }



{
    WCHAR wszT[IFileSystemAPI::cchPathMax], wszFNameT[IFileSystemAPI::cchPathMax];

    CallS( m_pinst->m_pfsapi->ErrPathParse( m_pLogStream->LogName(), wszT, wszFNameT, wszT ) );
    fJetLog = ( UtilCmpFileName( wszFNameT, SzParam( m_pinst, JET_paramBaseName ) ) == 0 );
}


    Assert( m_pLog->FRecovering() );
    Assert( m_pLog->FRecoveringMode() == fRecoveringRedo );


    if ( m_fReaderInited )
    {
        fCreatedLogReader = fFalse;

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
        if ( m_cbLGBuf / m_csecLGBuf != m_pLogStream->CbSec() )
        {
            Assert( m_pLogStream->FIsOldLrckVersionSeen() );
            QWORD   cbSize;
            Call( m_pLogStream->ErrFileSize( &cbSize ) );
            UINT    csecSize;
            csecSize = UINT( cbSize / m_pLogStream->CbSec() );
            Call( m_pLogBuffer->ErrLGInitLogBuffers( m_pinst, m_pLogStream, csecSize, fTrue ) );
        }
#endif

#ifdef DEBUG

        QWORD   cbSize;
        Call( m_pLogStream->ErrFileSize( &cbSize ) );

        Assert( m_pLogStream->CbSec() > 0 );
        Assert( ( cbSize % m_pLogStream->CbSec() ) == 0 );
        UINT    csecSize;
        csecSize = UINT( cbSize / m_pLogStream->CbSec() );
        Assert( csecSize > m_pLogStream->CSecHeader() );

        Assert( m_csecLGBuf >= csecSize );
#endif
    }
    else
    {
        fCreatedLogReader = fTrue;


        QWORD   cbSize;
        Call( m_pLogStream->ErrFileSize( &cbSize ) );

        Assert( m_pLogStream->CbSec() > 0 );
        Assert( ( cbSize % m_pLogStream->CbSec() ) == 0 );
        UINT    csecSize;
        csecSize = UINT( cbSize / m_pLogStream->CbSec() );
        Assert( csecSize > m_pLogStream->CSecHeader() );

        Call( ErrLReaderInit( csecSize ) );

        Assert( m_csecLGBuf >= csecSize );
    }
    Assert( m_cbLGBuf / m_csecLGBuf == m_pLogStream->CbSec() );


    Assert( m_pLogStream->FLGFileOpened() );
    if ( !fReadOnly )
    {
        Assert( NULL != m_pLogStream->LogName() );
        Assert( L'\0' != m_pLogStream->LogName()[ 0 ] );
    }


    Call( ErrReaderEnsureLogFile() );
    m_fIgnoreNextSectorReadFailure = fFalse;


    for ( lgposCurrent.isec = USHORT( m_pLogStream->CSecHeader() );
          lgposCurrent.isec < m_pLogStream->CSecLGFile() - 1;
          lgposCurrent.isec++ )
    {
        err = ErrReaderEnsureSector( lgposCurrent.isec, 1, &pbSegment );
        if ( err >= JET_errSuccess )
        {
            pSegHdr = (LGSEGHDR *)pbSegment;
            ChecksumAndPossiblyFixPage(
                    pbSegment,
                    m_pLogStream->CbSec(),
                    logfilePage,
                    lgposCurrent.isec,
                    fTrue,
                    &checksumExpected,
                    &checksumActual,
                    &fCorrectableError,
                    &ibitCorrupted );
            if ( checksumExpected != checksumActual )
            {
                if ( lgposCurrent.isec != m_pLogStream->CSecHeader() )
                {
                    ChecksumAndPossiblyFixPage(
                            pbSegment,
                            m_pLogStream->CbSec(),
                            logfilePage,
                            lgposCurrent.isec - 1,
                            fTrue,
                            &checksumExpectedT,
                            &checksumActualT,
                            &fCorrectableError,
                            &ibitCorrupted );
                    if ( checksumExpectedT == checksumActualT )
                    {
                        if ( fCorrectableError )
                        {
                            ReportSegmentCorrection_( lgposCurrent.isec - 1, lgposCurrent.isec * m_pLogStream->CbSec(), ibitCorrupted );
                        }
                        if ( pSegHdr->le_lgposSegment.le_lGeneration > lGenLastEmptyPage )
                        {
                            lLine = __LINE__;
                            reason = eLCEmptySegmentHigherLgen;
                            goto ReportCorruption;
                        }
                        lGenLastEmptyPage = pSegHdr->le_lgposSegment.le_lGeneration;
                        fEmptyPageSeen = fTrue;
                        continue;
                    }
                }

                if ( FLGVersionZeroFilled( &m_pLogStream->GetCurrentFileHdr()->lgfilehdr ) )
                {
                    if ( FUtilZeroed( pbSegment, m_pLogStream->CbSec() ) )
                    {
                        lGenLastEmptyPage = 0;
                        fEmptyPageSeen = fTrue;
                        continue;
                    }
                }

                if ( fEmptyPageSeen )
                {
                    lLine = __LINE__;
                    reason = eLCCorruptAfterEmptySegment;
                    goto ReportCorruption;
                }

                err = ErrERRCheck( JET_errLogFileCorrupt );
            }
            else
            {
                if ( fCorrectableError )
                {
                    ReportSegmentCorrection_( lgposCurrent.isec, lgposCurrent.isec * m_pLogStream->CbSec(), ibitCorrupted );
                }
                if ( CmpLgpos( &pSegHdr->le_lgposSegment, &lgposCurrent ) < 0 )
                {
                    if ( pSegHdr->le_lgposSegment.le_lGeneration > lGenLastEmptyPage )
                    {
                        lLine = __LINE__;
                        reason = eLCEmptySegmentHigherLgen;
                        goto ReportCorruption;
                    }
                    lGenLastEmptyPage = pSegHdr->le_lgposSegment.le_lGeneration;
                    fEmptyPageSeen = fTrue;
                    continue;
                }

                if ( CmpLgpos( &pSegHdr->le_lgposSegment, &lgposCurrent ) > 0 )
                {
                    lLine = __LINE__;
                    reason = eLCBadSegmentLgpos;
                    goto ReportCorruption;
                }

                if ( fEmptyPageSeen )
                {
                    lLine = __LINE__;
                    reason = eLCValidSegmentAfterEmpty;
                    goto ReportCorruption;
                }

                if ( fUpdateAccSegChecksum )
                {
                    m_pLogStream->AccumulateSectorChecksum( pSegHdr->checksum );
                }
            }
        }

        if ( err < JET_errSuccess )
        {
            err = ErrReaderEnsureSector( lgposCurrent.isec + 1, 1, &pbSegment );
            if ( JET_errSuccess != err )
            {
                Assert( ErrcatERRLeastSpecific( err ) != JET_errcatApi );
                Assert( ErrcatERRLeastSpecific( err ) != JET_errcatError );
                Assert( ErrcatERRLeastSpecific( err ) != JET_errcatUnknown );
                if ( ErrcatERRLeastSpecific( err ) == JET_errcatOperation )
                {
                    goto HandleError;
                }

                reason = eLCIOError;
                goto ReportCorruption;
            }
            pSegHdr = (LGSEGHDR *)pbSegment;

            ChecksumAndPossiblyFixPage(
                    pbSegment,
                    m_pLogStream->CbSec(),
                    logfilePage,
                    lgposCurrent.isec,
                    fTrue,
                    &checksumExpectedT,
                    &checksumActualT,
                    &fCorrectableError,
                    &ibitCorrupted );
            if ( checksumExpectedT != checksumActualT ||
                 CmpLgpos( &pSegHdr->le_lgposSegment, &lgposCurrent ) != 0 )
            {
                lLine = __LINE__;
                reason = eLCSegmentCorrupt;
                goto ReportCorruption;
            }

            if ( fCorrectableError )
            {
                ReportSegmentCorrection_( lgposCurrent.isec, ( lgposCurrent.isec + 1 ) * m_pLogStream->CbSec(), ibitCorrupted );
            }

            if ( pfIsPatchable )
            {

                *pfIsPatchable = fTrue;
                goto HandleError;
            }
            if ( m_pLog->FHardRestore() )
            {
                LONG lgenLowRestore, lgenHighRestore;
                m_pLog->LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
                if ( lgposCurrent.lGeneration <= lgenHighRestore )
                {
                    fIsTornWrite = fTrue;
                    lLine = __LINE__;
                    reason = eLCCannotPatchHardRecovery;
                    goto ReportCorruption;
                }

                if ( !fJetLog )
                {
                    fIsTornWrite = fTrue;
                    lLine = __LINE__;
                    reason = eLCCannotPatchHardRecovery;
                    goto ReportCorruption;
                }
            }

            const WCHAR *rgpsz[1] = { m_pLogStream->LogName() };
            UtilReportEvent(
                    eventWarning,
                    LOGGING_RECOVERY_CATEGORY,
                    LOG_USING_SHADOW_SECTOR_ID,
                    _countof( rgpsz ),
                    rgpsz,
                    0,
                    NULL,
                    m_pinst );

            if ( !fReadOnly )
            {
                if ( !fFileOpenedForWrite )
                {
                    Call( m_pLogStream->ErrLGOpenFile() );
                    fFileOpenedForWrite = fTrue;
                }

                Call( m_pLogStream->ErrLGWriteSectorData(
                        IOR( iorpPatchFix ),
                        lgposCurrent.lGeneration,
                        m_pLogStream->CbSec() * QWORD( lgposCurrent.isec ),
                        m_pLogStream->CbSec(),
                        pbSegment,
                        LOG_WRITE_ERROR_ID ) );
            }

            ReaderSectorModified( lgposCurrent.isec, pbSegment );
            Call( ErrReaderEnsureSector( lgposCurrent.isec, 1, &pbSegment ) );

            if ( fUpdateAccSegChecksum )
            {
                pSegHdr = (LGSEGHDR*) pbSegment;
                m_pLogStream->AccumulateSectorChecksum( pSegHdr->checksum );
            }
        }

        pbLastValidSegment = pbSegment;
        lgposLastValidSegment = lgposCurrent;
    }

    C_ASSERT( sizeof( LRTERMREC2 ) <= LOG_MIN_FRAGMENT_SIZE );
    if ( pbLastValidSegment != NULL )
    {
        pbSegment = pbLastValidSegment + m_pLogStream->CbSec();
        for ( pbLR = pbLastValidSegment + sizeof( LGSEGHDR );
              pbLR < pbSegment && *pbLR < lrtypMax;
              pbLR += CbLGSizeOfRec( (LR *)pbLR ) )
        {
            switch ( *pbLR )
            {
                case lrtypTerm:
                case lrtypTerm2:
                case lrtypRecoveryQuit:
                case lrtypRecoveryQuit2:
                case lrtypRecoveryQuitKeepAttachments:
                    fGotQuit = fTrue;
                    ibQuit = (USHORT)(pbLR - pbLastValidSegment);
                    if ( ibQuit == sizeof( LGSEGHDR ) )
                    {
                        ibQuit = 0;
                    }
                    break;

                case lrtypNOP:
                case lrtypNOP2:
                    break;

                default:
                    fGotQuit = fFalse;
                    break;
            }
        }
        if ( fGotQuit )
        {
            *pfCloseNormally = fTrue;
            if ( plgposLastTerm )
            {
                plgposLastTerm->ib          = ibQuit;
                plgposLastTerm->isec        = lgposLastValidSegment.isec;
                plgposLastTerm->lGeneration = lgposLastValidSegment.lGeneration;
            }
        }
    }

    goto HandleError;

ReportCorruption:

    OSDiagTrackCorruptLog( reason );

    LGIReportChecksumMismatch(
            m_pinst,
            m_pLogStream->LogName(),
            lgposLastValidSegment.isec,
            lgposCurrent.isec,
            checksumExpected.rgChecksum[ 0 ],
            checksumActual.rgChecksum[ 0 ],
            err,
            reason );
            
    WCHAR       szCorruption[64];
    const WCHAR*    rgpsz[3]        = { m_pLogStream->LogName(), L"", szCorruption };


    OSStrCbFormatW( szCorruption, sizeof( szCorruption ), L"isec %d reason %ws", lgposCurrent.isec, g_rgwszLogCorruptReason[ reason < eLCMax ? reason : 0 ] );

    Assert( FNegTest( fCorruptingLogFiles ) );

    if ( m_pLog->FHardRestore() )
    {


        LONG lgenLowRestore, lgenHighRestore;
        m_pLog->LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
        if ( lgposCurrent.lGeneration <= lgenHighRestore )
        {

            UtilReportEvent(    eventError,
                                LOGGING_RECOVERY_CATEGORY,
                                LOG_CORRUPTION_DURING_HARD_RESTORE_ID,
                                3,
                                rgpsz,
                                0,
                                NULL,
                                m_pinst );

            OSUHAPublishEvent(  HaDbFailureTagRecoveryRedoLogCorruption,
                                m_pinst,
                                HA_LOGGING_RECOVERY_CATEGORY,
                                HaDbIoErrorNone, NULL, 0, 0,
                                HA_LOG_CORRUPTION_DURING_HARD_RESTORE_ID,
                                3,
                                rgpsz );

            Call( ErrERRCheck_( fIsTornWrite ? JET_errLogTornWriteDuringHardRestore : JET_errLogCorruptDuringHardRestore, __FILE__, lLine ) );
        }


        UtilReportEvent(    eventError,
                            LOGGING_RECOVERY_CATEGORY,
                            LOG_CORRUPTION_DURING_HARD_RECOVERY_ID,
                            3,
                            rgpsz,
                            0,
                            NULL,
                            m_pinst );
        
        OSUHAPublishEvent(  HaDbFailureTagRecoveryRedoLogCorruption,
                            m_pinst,
                            HA_LOGGING_RECOVERY_CATEGORY,
                            HaDbIoErrorNone, NULL, 0, 0,
                            HA_LOG_CORRUPTION_DURING_HARD_RECOVERY_ID,
                            3,
                            rgpsz );

        Call( ErrERRCheck_( fIsTornWrite ? JET_errLogTornWriteDuringHardRecovery : JET_errLogCorruptDuringHardRecovery, __FILE__, lLine ) );
    }
    else if ( !fReadOnly )
    {

        UtilReportEvent(    eventError,
                            LOGGING_RECOVERY_CATEGORY,
                            LOG_CORRUPTION_DURING_SOFT_RECOVERY_ID,
                            3,
                            rgpsz,
                            0,
                            NULL,
                            m_pinst );

        OSUHAPublishEvent(  HaDbFailureTagRecoveryRedoLogCorruption,
                            m_pinst,
                            HA_LOGGING_RECOVERY_CATEGORY,
                            HaDbIoErrorNone, NULL, 0, 0,
                            HA_LOG_CORRUPTION_DURING_SOFT_RECOVERY_ID,
                            3,
                            rgpsz );

        Call( ErrERRCheck_( JET_errLogFileCorrupt, __FILE__, lLine ) );
    }
    else
    {

        Assert( !m_pLog->FHardRestore() );
        Assert( fCreatedLogReader );

        OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"3aa8328c-8b3d-45a1-93c5-4ea6bea70ad4" );
        Call( ErrERRCheck_( JET_errLogFileCorrupt, __FILE__, lLine ) );
    }

HandleError:

    m_lgposLastRec.ib           = 0;
    m_lgposLastRec.isec         = lgposLastValidSegment.isec + 1;
    m_lgposLastRec.lGeneration  = lgposLastValidSegment.lGeneration;



    if ( fCreatedLogReader )
    {
        errT = ErrLReaderTerm();


        if ( errT != JET_errSuccess )
        {
            errT = ErrERRCheck( errT );
        }


        if ( err == JET_errSuccess )
        {
            err = errT;
        }
    }

    if ( err >= 0 && !fReadOnly && fJetLog )
    {
        if ( m_lgposLastRec.isec < m_pLogStream->CSecLGFile() - 1 )
        {
            m_isecWrite                 = m_lgposLastRec.isec;
            m_pbEntry = m_pbWrite       = m_pbLGBufMin;

            m_lgposToWrite.isec         = USHORT( m_isecWrite );
            m_lgposToWrite.lGeneration  = m_pLogStream->GetCurrentFileGen();
            m_lgposToWrite.ib           = 0;

            m_lgposMaxWritePoint        = m_lgposToWrite;
        }
        else
        {
            m_isecWrite                 = m_pLogStream->CSecHeader();
            m_pbEntry = m_pbWrite       = m_pbLGBufMin;

            m_lgposToWrite.isec         = USHORT( m_isecWrite );
            m_lgposToWrite.lGeneration  = m_pLogStream->GetCurrentFileGen() + 1;
            m_lgposToWrite.ib           = 0;

            m_lgposMaxWritePoint        = m_lgposToWrite;
        }
    }

    if ( fFileOpenedForWrite )
    {
        Assert( NULL != m_pLogStream->LogName() );
        Assert( L'\0' != m_pLogStream->LogName()[ 0 ] );

        errT = m_pLogStream->ErrLGOpenFile( NULL, fTrue );

        if ( err == JET_errSuccess )
        {
            err = errT;
        }
    }

    return err;
}



ERR LOG_READ_BUFFER::ErrLGLocateFirstRedoLogRecFF(
    LE_LGPOS    *ple_lgposRedo,
    BYTE        **ppbLR )
{
    ERR         err             = JET_errSuccess;
    BYTE        *pbSegment      = pbNil;
    LGPOS       lgposPbNext;

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    if ( FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
    {
        return ErrLGLocateFirstRedoLogRecFF_Legacy( ple_lgposRedo, ppbLR );
    }
#endif


    Assert( m_lgposLastRec.isec >= m_pLogStream->CSecHeader() &&
            ( m_lgposLastRec.isec < ( m_pLogStream->CSecLGFile() - 1 ) ||
              ( m_lgposLastRec.isec == ( m_pLogStream->CSecLGFile() - 1 ) && m_lgposLastRec.ib == 0 ) ) &&
            m_lgposLastRec.lGeneration == m_pLogStream->GetCurrentFileGen() );

    *ppbLR = pbNil;

    Call( ErrReaderEnsureLogFile() );

    m_fIgnoreNextSectorReadFailure = fFalse;

    const USHORT csecRedo = min( max( ple_lgposRedo->le_isec, (USHORT)m_pLogStream->CSecHeader() ), m_lgposLastRec.isec );

    Call( ErrReaderEnsureSector( csecRedo, 1, &pbSegment ) );

    m_pbNext = pbSegment;
    if ( csecRedo == ple_lgposRedo->le_isec && csecRedo < m_lgposLastRec.isec )
    {
        Assert( ple_lgposRedo->le_ib < m_pLogStream->CbSec() );
        BYTE *pbRedo = pbSegment + ple_lgposRedo->le_ib;
        while ( m_pbNext < pbRedo )
        {
            if ( m_pbNext == pbSegment )
            {
                m_pbNext += sizeof( LGSEGHDR );
            }
            m_pbNext += CbLGSizeOfRec( (LR *)m_pbNext );
        }
    }

    m_pbNextNext = pNil;
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    m_pbLastChecksum = NULL;
    m_lgposLastChecksum = lgposMin;
#endif

    err = ErrLGIGetRecordAtPbNext( ppbLR, fFalse, ( csecRedo == ple_lgposRedo->le_isec ) );
    if ( err == errLGNoMoreRecords )
    {
        *ppbLR = (BYTE *)&m_lrNop;
        return JET_errSuccess;
    }
    Call( err );
    Assert( m_pbNextNext != pNil );

    GetLgposOfPbNext( &lgposPbNext );
    m_pLog->LGPrereadExecute( lgposPbNext
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
                    , lgposMin
#endif
                    );

    return err;

HandleError:

    m_pLog->LGPrereadTerm();

    return err;
}


ERR LOG_READ_BUFFER::ErrLGGetNextRecFF( BYTE **ppbLR, BOOL fPreread )
{
    ERR     err = JET_errSuccess;
    LGPOS   lgposPbNext;
    LGPOS   lgposPbNextNew;

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    if ( FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
    {
        return ErrLGGetNextRecFF_Legacy( ppbLR );
    }
#endif

    *ppbLR = pbNil;


    Assert( m_lgposLastRec.isec >= m_pLogStream->CSecHeader() &&
            ( m_lgposLastRec.isec < ( m_pLogStream->CSecLGFile() - 1 ) ||
              ( m_lgposLastRec.isec == ( m_pLogStream->CSecLGFile() - 1 ) && m_lgposLastRec.ib == 0 ) ) &&
            m_lgposLastRec.lGeneration == m_pLogStream->GetCurrentFileGen() );

    if ( m_pbNextNext == pNil )
    {
        Call( errLGNoMoreRecords );
    }


    GetLgposOfPbNextNext( &lgposPbNextNew );
    if ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) >= 0 )
    {
        Error( ErrERRCheck( errLGNoMoreRecords ) );
    }


    Assert( m_pbNext != pbNil );
    GetLgposOfPbNext( &lgposPbNext );




    EnforceSz( ( CmpLgpos( &lgposPbNext, &m_lgposLastRec ) < 0 ), "LogicalCorruptLogPbNextInconsistent" );


    Assert( m_pbNextNext >= m_pbLGBufMin && m_pbNextNext < m_pbLGBufMax );
    if ( m_pbNextNext < m_pbLGBufMin || m_pbNextNext >= m_pbLGBufMax )
    {
        Error( ErrERRCheck( JET_errLogFileCorrupt ) );
    }


    Assert( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) >= 0 );
    if ( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) < 0 )
    {
        Error( ErrERRCheck( JET_errLogFileCorrupt ) );
    }
    
    m_pbNext = m_pbNextNext;

    Call( ErrLGIGetRecordAtPbNext( ppbLR, fPreread ) );

HandleError:
    return err;
}



ERR LOG_READ_BUFFER::ErrLGIGetRecordAtPbNext( BYTE **ppbLR, BOOL fPreread, BOOL fSkipPartialLR )
{
    ERR err;
    LGPOS lgposCurrent;
    BYTE *pbSegment;
    BYTE *pbCurrent = m_pbNext;
    BOOL fInFragment = fFalse;
    LRFRAGMENTBEGIN *plrFragBegin;
    LRFRAGMENT *plrFrag;
    DWORD cbFragmentSeen = 0;
    DWORD cbTotalFragment = 0;
    DWORD iBuf = fPreread ? 1 : 0;

    forever
    {
        GetLgposDuringReading( pbCurrent, &lgposCurrent );
        if ( CmpLgpos( &lgposCurrent, &m_lgposLastRec ) >= 0 )
        {
            m_pbNextNext = pbCurrent;
            Error( ErrERRCheck( errLGNoMoreRecords ) );
        }
        Call( ErrReaderEnsureSector( lgposCurrent.isec, 1, &pbSegment ) );
        if ( pbCurrent == pbSegment )
        {
            LGSEGHDR *pSegHdr = (LGSEGHDR *)pbSegment;
            if ( m_pLog->FDumpingLogs() && m_pLog->FVerboseDump() )
            {
                DUMPPrintF( "Segment Checksum: 0x%016I64x, LGPOS (%08lX,%04hX,%04hX), logtimeStart ",
                        (XECHECKSUM)pSegHdr->checksum,
                        (INT)pSegHdr->le_lgposSegment.le_lGeneration,
                        (SHORT)pSegHdr->le_lgposSegment.le_isec,
                        (SHORT)pSegHdr->le_lgposSegment.le_ib );
                DUMPPrintLogTime( &pSegHdr->logtimeSegmentStart );
                DUMPPrintF("\n");
            }

            BOOL fContainsDataFromFutureLogs = fFalse;
            for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
            {
                const IFMP ifmp = m_pinst->m_mpdbidifmp[dbid];
                if ( ifmp < g_ifmpMax &&
                     g_rgfmp[ ifmp ].FContainsDataFromFutureLogs() &&
                     ((LGSEGHDR *)pbSegment)->le_lgposSegment.le_lGeneration + (LONG)UlParam( m_pinst, JET_paramCheckpointDepthMax)*2 >= g_rgfmp[ ifmp ].Pdbfilehdr()->le_lGenMaxRequired )
                {
                    fContainsDataFromFutureLogs = fTrue;
                    break;
                }
            }

            if ( !fPreread &&
                 !fContainsDataFromFutureLogs &&
                 m_pLog->FIODuringRecovery() &&
                 !m_pLog->FDumpingLogs() &&
                 UlParam( m_pinst, JET_paramReplayThrottlingLevel ) == JET_ReplayThrottlingSleep &&
                 !FLogtimeIsNull( &pSegHdr->logtimeSegmentStart ) )
            {
                const __int64 ftSegment = ConvertLogTimeToFileTime( &pSegHdr->logtimeSegmentStart );
                const HRT hrtCurrent = HrtHRTCount();
                if ( m_ftFirstSegment == 0 )
                {
                    m_ftFirstSegment = ftSegment;
                    m_hrtFirstSegment = hrtCurrent;
                }
                else
                {
                    const LONG dtickSegmentGeneration = (LONG)( ( ftSegment - m_ftFirstSegment ) / 10000 );
                    const LONG dtickSegmentReplay = (LONG)( ( 1000 * ( hrtCurrent - m_hrtFirstSegment ) ) / HrtHRTFreq() );
                    if ( dtickSegmentReplay < dtickSegmentGeneration / 2 )
                    {
                        const HRT hrtBefore = HrtHRTCount();
                        UtilSleep( min( dtickSegmentGeneration / 2 - dtickSegmentReplay, 15 ) );
                        m_pinst->m_isdlInit.AddThrottleTime( DblHRTElapsedTimeFromHrtStart( hrtBefore ) );
                        PERFOpt( cLGRecoveryThrottleIOSmoothingTime.Add( m_pinst, ( 1000 * ( HrtHRTCount() - hrtBefore ) ) / HrtHRTFreq() ) );
                        PERFOpt( cLGRecoveryThrottleIOSmoothing.Inc( m_pinst ) );
                    }
                }
            }

            pbCurrent += sizeof( LGSEGHDR );
        }

        switch ( *pbCurrent )
        {
            case lrtypFragmentBegin:
                if ( pbCurrent + sizeof( LRFRAGMENTBEGIN ) > pbSegment + m_pLogStream->CbSec() )
                {
                    Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                }

                if ( fInFragment &&
                     ( pbCurrent != pbSegment + sizeof( LGSEGHDR ) ||
                       ((LRFRAGMENTBEGIN *)pbCurrent)->le_rgbData[0] != lrtypUndoInfo ) )
                {
                    Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                }

                fInFragment = fTrue;
                fSkipPartialLR = fFalse;

                plrFragBegin = (LRFRAGMENTBEGIN *)pbCurrent;
                cbTotalFragment = plrFragBegin->le_cbTotalLRSize;
                if ( cbTotalFragment > m_cbAssembledLR[iBuf] )
                {
                    OSMemoryHeapFree( m_pvAssembledLR[iBuf] );
                    m_cbAssembledLR[iBuf] = 0;
                    Alloc( m_pvAssembledLR[iBuf] = (BYTE *)PvOSMemoryHeapAlloc( cbTotalFragment ) );
                    m_cbAssembledLR[iBuf] = cbTotalFragment;
                }

                if ( plrFragBegin->le_cbFragmentSize >= cbTotalFragment ||
                     plrFragBegin->le_cbFragmentSize > ( pbSegment + m_pLogStream->CbSec() - pbCurrent ) )
                {
                    Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                }

                UtilMemCpy( m_pvAssembledLR[iBuf],
                            plrFragBegin->le_rgbData,
                            plrFragBegin->le_cbFragmentSize );
                cbFragmentSeen = plrFragBegin->le_cbFragmentSize;
                pbCurrent += sizeof( LRFRAGMENTBEGIN ) + plrFragBegin->le_cbFragmentSize;
                break;

            case lrtypFragment:
                plrFrag = (LRFRAGMENT *)pbCurrent;
                if ( fSkipPartialLR )
                {
                    pbCurrent += sizeof( LRFRAGMENT ) + plrFrag->le_cbFragmentSize;
                    m_pbNext = pbCurrent;
                    break;
                }
                if ( !fInFragment ||
                     ( pbCurrent + sizeof( LRFRAGMENT ) > pbSegment + m_pLogStream->CbSec() ) )
                {
                    Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                }
                if ( plrFrag->le_cbFragmentSize > cbTotalFragment - cbFragmentSeen ||
                     plrFrag->le_cbFragmentSize > ( pbSegment + m_pLogStream->CbSec() - pbCurrent ) )
                {
                    Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                }
                UtilMemCpy( m_pvAssembledLR[iBuf] + cbFragmentSeen,
                            plrFrag->le_rgbData,
                            plrFrag->le_cbFragmentSize );
                cbFragmentSeen += plrFrag->le_cbFragmentSize;
                pbCurrent += sizeof( LRFRAGMENT ) + plrFrag->le_cbFragmentSize;
                if ( cbTotalFragment == cbFragmentSeen )
                {
                    if ( *m_pvAssembledLR[iBuf] >= lrtypMax ||
                         CbLGFixedSizeOfRec( (LR *)m_pvAssembledLR[iBuf] ) > cbTotalFragment ||
                         CbLGSizeOfRec( (LR *)m_pvAssembledLR[iBuf] ) != cbTotalFragment )
                    {
                        Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                    }
                    m_pbNextNext = pbCurrent;
                    *ppbLR = m_pvAssembledLR[iBuf];
                    return JET_errSuccess;
                }
                break;

            case lrtypNOP:
                while ( pbCurrent < pbSegment + m_pLogStream->CbSec() &&
                        *pbCurrent == lrtypNOP )
                {
                    pbCurrent++;
                }
                Expected( pbCurrent == pbSegment + m_pLogStream->CbSec() );
                break;

            default:
                if ( fInFragment )
                {
                    if ( pbCurrent == pbSegment + sizeof( LGSEGHDR ) &&
                         ( *pbCurrent == lrtypRecoveryUndo ||
                           *pbCurrent == lrtypRecoveryUndo2 ||
                           *pbCurrent == lrtypUndoInfo ) )
                    {
                        m_pbNext = pbSegment;
                    }
                    else
                    {
                        Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                    }
                }
                if ( *pbCurrent >= lrtypMax )
                {
                    AssertSz( FNegTest( fCorruptingLogFiles ), "Corrupt or missing LRTYP(%d) - did you maybe do something silly like add a non-pre-Ignored LR? ;)", (ULONG)*pbCurrent );
                    Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                }
                else if ( CbLGFixedSizeOfRec( (LR *)pbCurrent ) > (DWORD)( pbSegment + m_pLogStream->CbSec() - pbCurrent ) ||
                        CbLGSizeOfRec( (LR *)pbCurrent ) > (DWORD)( pbSegment + m_pLogStream->CbSec() - pbCurrent ) )
                {
                    AssertSz( FNegTest( fCorruptingLogFiles ), "Corrupt size of LR (%d,%d) past rest of segment (%d).",
                                    CbLGFixedSizeOfRec( (LR *)pbCurrent ),
                                    CbLGSizeOfRec( (LR *)pbCurrent ),
                                    (DWORD)( pbSegment + m_pLogStream->CbSec() - pbCurrent ) );
                    Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                }
                m_pbNextNext = pbCurrent + CbLGSizeOfRec( (LR *)pbCurrent );
                *ppbLR = pbCurrent;
                return JET_errSuccess;
        }
    }

HandleError:

    if ( err == JET_errLogFileCorrupt )
    {
        Assert( FNegTest( fCorruptingLogFiles ) );


        if ( m_pLog->FHardRestore() )
        {


            LONG lgenLowRestore, lgenHighRestore;
            m_pLog->LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
            if ( m_pLogStream->GetCurrentFileGen() <= lgenHighRestore )
            {


                Assert( m_pLogStream->GetCurrentFileGen() >= lgenLowRestore );
                err = ErrERRCheck( JET_errLogCorruptDuringHardRestore );
            }
            else
            {


                err = ErrERRCheck( JET_errLogCorruptDuringHardRecovery );
            }
        }
        else
        {


        }
    }

    return err;
}


#ifdef MINIMAL_FUNCTIONALITY
#else

VOID LOG::LGIPrereadPage(
    const DBID      dbid,
    const PGNO      pgno,
    const OBJID     objid,
    BOOL *          pfPrereadIssued,
    BOOL * const        pfPrereadFailure,
    const BFPreReadFlags    bfprf )
{
    Assert( NULL != pfPrereadIssued );
    Assert( NULL != pfPrereadFailure );

    if ( m_plprereadSuppress->FLGPDBEnabled( dbid ) &&
         m_plprereadSuppress->IpgLGPGetSorted( dbid, pgno ) != CArray<PGNO>::iEntryNotFound )
    {
        return;
    }

    if ( ( pgnoNull != pgno ) && ( m_pPrereadWatermarks->CElements() < (UINT)UlParam( m_pinst, JET_paramPrereadIOMax ) ) )
    {
        const IFMP ifmp = m_pinst->m_mpdbidifmp[dbid];
        if ( ifmp < g_ifmpMax
            && FIODatabaseOpen( ifmp ) )
        {
            TraceContextScope tcScope( iortRecovery );
            tcScope->SetDwEngineObjid( objid );

            CPG cpgPreread = 0;
            const ERR err = m_plpreread->ErrLGPPrereadExtendedPageRange( dbid, pgno, &cpgPreread, bfprf );

            if ( ( err < JET_errSuccess ) && ( err != errBFPageCached ) && ( err != JET_errFileIOBeyondEOF ) )
            {
                *pfPrereadFailure = fTrue;
            }

            tcScope->SetDwEngineObjid( dwEngineObjidNone );

            if ( cpgPreread > 0 )
            {
                *pfPrereadIssued = fTrue;

                LGPOS lgpos;
                m_pLogReadBuffer->GetLgposOfPbNext( &lgpos );
                LGPOSQueueNode* const plgposQueueNode = new LGPOSQueueNode( lgpos );

                if ( plgposQueueNode != NULL )
                {
                    OnDebug( const LGPOSQueueNode* const plgposQueueNodeTail = m_pPrereadWatermarks->Tail() );
                    Assert( ( plgposQueueNodeTail == NULL ) || ( CmpLgpos( plgposQueueNode->m_lgpos, plgposQueueNodeTail->m_lgpos ) >= 0 ) );

                    m_pPrereadWatermarks->InsertAsNextMost( plgposQueueNode, OffsetOf( LGPOSQueueNode, m_plgposNext ) );
                }

                if ( bfprf & bfprfDBScan )
                {
                    g_rgfmp[ifmp].PdbmFollower()->UpdateIoTracking( ifmp, cpgPreread );
                }
            }
            else
            {
                Assert( cpgPreread == 0 );
            }
        }
    }
}
#endif

VOID
LOG::LGPrereadExecute(
    LGPOS lgposPbNext
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    ,LGPOS lgposLastChecksum
#endif
    )
{
#ifdef MINIMAL_FUNCTIONALITY
#else

    m_lgposPbNextPreread = lgposPbNext;
    m_lgposPbNextNextPreread = lgposPbNext;
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    m_lgposLastChecksumPreread = lgposLastChecksum;
#endif

    if ( !m_fDumpingLogs )
    {
        m_fPreread = fTrue;

        Assert( m_pPrereadWatermarks != NULL );
        while ( !m_pPrereadWatermarks->FEmpty() )
        {
            delete m_pPrereadWatermarks->RemovePrevMost( OffsetOf( LGPOSQueueNode, m_plgposNext ) );
        }
        

        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            if ( FLGRICheckRedoConditionForDb( dbid, lgposPbNext ) )
            {
                m_plpreread->LGPDBEnable( dbid );
                m_plprereadSuppress->LGPDBEnable( dbid );

                m_fIODuringRecovery = fTrue;
            }
            else
            {
                m_plpreread->LGPDBDisable( dbid );
                m_plprereadSuppress->LGPDBDisable( dbid );
            }
        }


        ERR err = ErrLGIPrereadPages( fTrue );
        AssertSz( m_pPrereadWatermarks->FEmpty(), "Only collecting pgnos, not supposed to accumulate watermarks." );

        if ( err != JET_errSuccess )
        {
            LGPrereadTerm();
        }
        else
        {

            m_plpreread->LGPSortPages();
            m_plprereadSuppress->LGPSortPages();
            err = ErrLGIPrereadPages( fFalse );
        }
    }
#endif
}


ERR LOG::ErrLGIPrereadExecute( const BOOL fPgnosOnly )
{
    ERR         err                     = JET_errSuccess;
    TraceContextScope tcScope( iortRecovery );

#ifdef MINIMAL_FUNCTIONALITY
#else
    const LR *  plr                     = pNil;
    BOOL        fPrereadFailure         = fFalse;
    BOOL        rgfPrereadIssued[dbidMax];
    const UINT  cRangesToPrereadMax = (UINT)UlParam( m_pinst, JET_paramPrereadIOMax );

    BOOL fBypassCpgCountCheck   = fPgnosOnly && cRangesToPrereadMax > 0;

    memset( rgfPrereadIssued, 0, sizeof(rgfPrereadIssued) );


    while ( ( m_pPrereadWatermarks->CElements() < cRangesToPrereadMax || fBypassCpgCountCheck ) &&
            !fPrereadFailure &&
            JET_errSuccess == ( err = m_pLogReadBuffer->ErrLGGetNextRecFF( (BYTE **) &plr, fTrue ) ) )
    {
        switch( plr->lrtyp )
        {
            case lrtypInsert:
            case lrtypFlagInsert:
            case lrtypFlagInsertAndReplaceData:
            case lrtypReplace:
            case lrtypReplaceD:
            case lrtypFlagDelete:
            case lrtypDelete:
            case lrtypDelta:
            case lrtypDelta64:
            case lrtypSetExternalHeader:
            case lrtypScrub:
            case lrtypUndo:
            {
                const LRPAGE_ * const   plrpage     = (LRPAGE_ *)plr;
                const DBID              dbid        = plrpage->dbid;

                Assert( dbid > dbidTemp );
                Assert( dbid < dbidMax );

                if ( fPgnosOnly )
                {
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrpage->le_pgno ) < JET_errSuccess )
                    {
                        m_plpreread->LGPDBDisable( dbid );
                    }
                }
                else
                {
                    LGIPrereadPage(
                            dbid,
                            plrpage->le_pgno,
                            ((LRNODE_ *)plr)->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                }

                break;
            }

            case lrtypSplit:
            case lrtypSplit2:
            {
                const LRSPLIT_ * const  plrsplit    = (LRSPLIT_ *)plr;
                const DBID              dbid        = plrsplit->dbid;

                Assert( dbid > dbidTemp );
                Assert( dbid < dbidMax );

                if ( fPgnosOnly )
                {
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrsplit->le_pgno ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrsplit->le_pgnoParent ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrsplit->le_pgnoRight ) != JET_errSuccess )
                    {
                        m_plpreread->LGPDBDisable( dbid );
                    }
                    if ( dbid < dbidMax &&
                         m_pinst->m_mpdbidifmp[ dbid ] < g_ifmpMax &&
                         g_rgfmp[ m_pinst->m_mpdbidifmp[ dbid ] ].FContainsDataFromFutureLogs() )
                    {
                        if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrsplit->le_pgnoNew ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                        }
                    }
                    else
                    {
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrsplit->le_pgnoNew );
                    }
                }
                else
                {
                    LGIPrereadPage(
                            dbid,
                            plrsplit->le_pgno,
                            plrsplit->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrsplit->le_pgnoNew,
                            plrsplit->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrsplit->le_pgnoParent,
                            plrsplit->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrsplit->le_pgnoRight,
                            plrsplit->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                }

                break;
            }

            case lrtypMerge:
            case lrtypMerge2:
            {
                const LRMERGE_ * const  plrmerge    = (LRMERGE_ *)plr;
                const DBID              dbid        = plrmerge->dbid;

                Assert( dbid > dbidTemp );
                Assert( dbid < dbidMax );

                if ( fPgnosOnly )
                {
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrmerge->le_pgno ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrmerge->le_pgnoRight ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrmerge->le_pgnoLeft ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrmerge->le_pgnoParent ) != JET_errSuccess )
                    {
                        m_plpreread->LGPDBDisable( dbid );
                    }
                }
                else
                {
                    LGIPrereadPage(
                            dbid,
                            plrmerge->le_pgno,
                            plrmerge->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrmerge->le_pgnoRight,
                            plrmerge->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrmerge->le_pgnoLeft,
                            plrmerge->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrmerge->le_pgnoParent,
                            plrmerge->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                }

                break;
            }

            case lrtypPageMove:
            {
                const LRPAGEMOVE * const plrpagemove    = LRPAGEMOVE::PlrpagemoveFromLr( plr );
                const DBID              dbid            = plrpagemove->Dbid();

                Assert( dbid > dbidTemp );
                Assert( dbid < dbidMax );

                if ( fPgnosOnly )
                {
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrpagemove->PgnoSource() ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrpagemove->PgnoParent() ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrpagemove->PgnoLeft() ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrpagemove->PgnoRight() ) != JET_errSuccess )
                    {
                        m_plpreread->LGPDBDisable( dbid );
                    }
                    if ( dbid < dbidMax &&
                         m_pinst->m_mpdbidifmp[ dbid ] < g_ifmpMax &&
                         g_rgfmp[ m_pinst->m_mpdbidifmp[ dbid ] ].FContainsDataFromFutureLogs() )
                    {
                        if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrpagemove->PgnoDest() ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                        }
                    }
                    else
                    {
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrpagemove->PgnoDest() );
                    }
                }
                else
                {
                    LGIPrereadPage(
                            dbid,
                            plrpagemove->PgnoSource(),
                            plrpagemove->ObjidFDP(),
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrpagemove->PgnoDest(),
                            plrpagemove->ObjidFDP(),
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrpagemove->PgnoParent(),
                            plrpagemove->ObjidFDP(),
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrpagemove->PgnoLeft(),
                            plrpagemove->ObjidFDP(),
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrpagemove->PgnoRight(),
                            plrpagemove->ObjidFDP(),
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                }

                break;
            }
            
            case lrtypConvertFDP2:
            case lrtypConvertFDP:
            {
                const LRCONVERTFDP *    plrconvfdp  = (LRCONVERTFDP *)plr;
                const DBID              dbid        = plrconvfdp->dbid;

                Assert( dbid > dbidTemp );
                Assert( dbid < dbidMax );

                if ( fPgnosOnly )
                {
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrconvfdp->le_pgno ) != JET_errSuccess )
                    {
                        m_plpreread->LGPDBDisable( dbid );
                    }

                    if ( dbid < dbidMax &&
                         m_pinst->m_mpdbidifmp[ dbid ] < g_ifmpMax &&
                         g_rgfmp[ m_pinst->m_mpdbidifmp[ dbid ] ].FContainsDataFromFutureLogs() )
                    {
                        if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrconvfdp->le_pgnoOE ) != JET_errSuccess ||
                             m_plpreread->ErrLGPAddPgnoRef( dbid, plrconvfdp->le_pgnoAE ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                        }
                    }
                    else
                    {
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrconvfdp->le_pgnoOE );
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrconvfdp->le_pgnoAE );
                    }
                }
                else
                {
                    LGIPrereadPage(
                            dbid,
                            plrconvfdp->le_pgno,
                            plrconvfdp->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrconvfdp->le_pgnoOE,
                            plrconvfdp->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrconvfdp->le_pgnoAE,
                            plrconvfdp->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                }

                break;
            }

            case lrtypCreateSingleExtentFDP:
            {
                const LRCREATESEFDP*    plrcreatesefdp  = (LRCREATESEFDP *)plr;
                const DBID              dbid            = plrcreatesefdp->dbid;

                Assert( dbid > dbidTemp );
                Assert( dbid < dbidMax );

                if ( fPgnosOnly )
                {
                    if ( dbid < dbidMax &&
                         m_pinst->m_mpdbidifmp[ dbid ] < g_ifmpMax &&
                         g_rgfmp[ m_pinst->m_mpdbidifmp[ dbid ] ].FContainsDataFromFutureLogs() )
                    {
                        if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrcreatesefdp->le_pgno ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                        }
                    }
                    else
                    {
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrcreatesefdp->le_pgno );
                    }
                }
                else
                {
                    LGIPrereadPage(
                            dbid,
                            plrcreatesefdp->le_pgno,
                            plrcreatesefdp->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                }

                break;
            }

            case lrtypCreateMultipleExtentFDP:
            {
                const LRCREATEMEFDP*    plrcreatemefdp  = (LRCREATEMEFDP *)plr;
                const DBID              dbid            = plrcreatemefdp->dbid;

                Assert( dbid > dbidTemp );
                Assert( dbid < dbidMax );

                if ( fPgnosOnly )
                {
                    if ( dbid < dbidMax &&
                         m_pinst->m_mpdbidifmp[ dbid ] < g_ifmpMax &&
                         g_rgfmp[ m_pinst->m_mpdbidifmp[ dbid ] ].FContainsDataFromFutureLogs() )
                    {
                        if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrcreatemefdp->le_pgnoFDPParent ) != JET_errSuccess ||
                             m_plpreread->ErrLGPAddPgnoRef( dbid, plrcreatemefdp->le_pgnoOE ) != JET_errSuccess ||
                             m_plpreread->ErrLGPAddPgnoRef( dbid, plrcreatemefdp->le_pgnoAE ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                        }
                    }
                    else
                    {
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrcreatemefdp->le_pgnoFDPParent );
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrcreatemefdp->le_pgnoOE );
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrcreatemefdp->le_pgnoAE );
                    }
                }
                else
                {
                    LGIPrereadPage(
                            dbid,
                            plrcreatemefdp->le_pgnoFDPParent,
                            plrcreatemefdp->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrcreatemefdp->le_pgnoOE,
                            plrcreatemefdp->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                    LGIPrereadPage(
                            dbid,
                            plrcreatemefdp->le_pgnoAE,
                            plrcreatemefdp->le_objidFDP,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                }

                break;
            }

            case lrtypScanCheck:
            case lrtypScanCheck2:
            {
                LRSCANCHECK2 lrscancheck;
                lrscancheck.InitScanCheck( plr );

                const DBID dbid = lrscancheck.Dbid();
                const PGNO pgno = lrscancheck.Pgno();

                Assert( dbid > dbidTemp );
                Assert( dbid < dbidMax );

                if ( pgno == pgnoScanLastSentinel )
                {
                    break;
                }

                if ( fPgnosOnly )
                {
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, pgno ) != JET_errSuccess )
                    {
                        m_plpreread->LGPDBDisable( dbid );
                    }
                }
                else if ( FLGRICheckRedoScanCheck( &lrscancheck, fTrue  ) )
                {
                    const BOOL fDbScan = ( lrscancheck.BSource() == scsDbScan );
                    ExpectedSz( ( lrscancheck.BSource() == scsDbScan ) ||
                                ( lrscancheck.BSource() == scsDbShrink ),
                                "Select the appropriate preread flag for this new ScanCheck source." );
                    LGIPrereadPage(
                        dbid,
                        pgno,
                        objidNil,
                        rgfPrereadIssued + dbid,
                        &fPrereadFailure,
                        fDbScan ? bfprfDBScan : bfprfNone );
                }
                break;
            }

            case lrtypPagePatchRequest:
            {
                const LRPAGEPATCHREQUEST * const   plrpatch     = (LRPAGEPATCHREQUEST *)plr;
                const DBID              dbid        = plrpatch->Dbid();

                Assert( dbid > dbidTemp );
                Assert( dbid < dbidMax );

                if ( fPgnosOnly )
                {
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrpatch->Pgno() ) < JET_errSuccess )
                    {
                        m_plpreread->LGPDBDisable( dbid );
                    }
                }
                else
                {
                    LGIPrereadPage(
                            dbid,
                            plrpatch->Pgno(),
                            objidNil,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                }

                break;
            }

            case lrtypNewPage:
            {
                const LRNEWPAGE * const plrnewpage = (LRNEWPAGE *)plr;
                const DBID dbid = plrnewpage->Dbid();

                Assert( dbid > dbidTemp );
                Assert( dbid < dbidMax );

                if ( fPgnosOnly )
                {
                    if ( dbid < dbidMax &&
                         m_pinst->m_mpdbidifmp[ dbid ] < g_ifmpMax &&
                         g_rgfmp[ m_pinst->m_mpdbidifmp[ dbid ] ].FContainsDataFromFutureLogs() )
                    {
                        if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrnewpage->Pgno() ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                        }
                    }
                    else
                    {
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrnewpage->Pgno() );
                    }
                }
                else
                {
                    LGIPrereadPage(
                            dbid,
                            plrnewpage->Pgno(),
                            plrnewpage->Objid(),
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );
                }

                break;
            }

            case lrtypExtentFreed:
            {
                LREXTENTFREED lrextentfreed;
                lrextentfreed.InitExtentFreed( plr );
                const DBID dbid         = lrextentfreed.Dbid();
                const PGNO pgnofirst    = lrextentfreed.PgnoFirst();
                const CPG  cpgextent    = lrextentfreed.CpgExtent();

                if( m_pinst->m_mpdbidifmp[ dbid ] >= g_ifmpMax || 
                    !( g_rgfmp[ m_pinst->m_mpdbidifmp[ dbid ] ].FRBSOn() ) )
                {
                    break;
                }

                Assert( dbid > dbidTemp );
                Assert( dbid < dbidMax );

                if ( fPgnosOnly )
                {
                    for( INT i = 0; i < cpgextent; ++i )
                    {
                        if( m_plpreread->ErrLGPAddPgnoRef( dbid, pgnofirst + i ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                            break;
                        }
                    }
                }
                else
                {
                    for( INT i = 0; i < cpgextent; ++i )
                    {
                        LGIPrereadPage(
                            dbid,
                            pgnofirst + i,
                            objidNil,
                            rgfPrereadIssued + dbid,
                            &fPrereadFailure );

                        if( fPrereadFailure )
                        {
                            break;
                        }
                    }
                }
                break;
            }


            default:
                break;
        }

        ExpectedSz( !fBypassCpgCountCheck || m_pPrereadWatermarks->FEmpty(), "Should not accumulate any prereads." );
    }

    BOOL fPrereadIssued = fFalse;

    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        if ( rgfPrereadIssued[ dbid ] )
        {
            const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];

            Assert( !fPgnosOnly );
            Assert( ifmp < g_ifmpMax );
            Assert( FIODatabaseOpen( ifmp ) );
            Assert( Ptls()->cbfAsyncReadIOs > 0 );

            CallS( m_plpreread->ErrLGPIPrereadIssue( dbid ) );
            fPrereadIssued = fTrue;
        }
    }

    if ( fPrereadIssued )
    {
        Ptls()->cbfAsyncReadIOs = 0;
    }
    else
    {
        Assert( Ptls()->cbfAsyncReadIOs == 0 );
    }

    if ( !fPgnosOnly && ( errLGNoMoreRecords == err ) )
    {

        m_fPreread = fFalse;
    }

#endif
    return err;
}


ERR LOG::ErrLGIPrereadPages( const BOOL fPgnosOnly )
{
    ERR             err = JET_errSuccess;
    LGPOS           lgposPbNextSaved;
    LGPOS           lgposPbNextNextSaved;
    OnDebug( LGPOS  lgposPbNextT );
    OnDebug( LGPOS  lgposPbNextNextEndT );
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    LGPOS           lgposLastChecksumSaved = lgposMin;
#endif

    m_pLogReadBuffer->GetLgposOfPbNext( &lgposPbNextSaved );
    m_pLogReadBuffer->GetLgposOfPbNextNext( &lgposPbNextNextSaved );
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    m_pLogReadBuffer->GetLgposOfLastChecksum( &lgposLastChecksumSaved );
#endif


    err = m_pLogReadBuffer->ErrReaderRestoreState(
            &m_lgposPbNextPreread,
            &m_lgposPbNextNextPreread
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
            ,&m_lgposLastChecksumPreread
#endif
            );

    if ( JET_errSuccess != err )
    {
        err = JET_errSuccess;
        m_fPreread = fFalse;
        LGPrereadTerm();
        goto LRestoreNormal;
    }


    err = ErrLGIPrereadExecute( fPgnosOnly );
    if ( JET_errSuccess != err )
    {
        if ( !fPgnosOnly || err != errLGNoMoreRecords )
        {
            m_fPreread = fFalse;
        }

        err = JET_errSuccess;
    }
#ifdef DEBUG
    else
    {
        m_pLogReadBuffer->GetLgposOfPbNext( &lgposPbNextT );
        Assert( CmpLgpos( lgposPbNextT, lgposPbNextSaved ) >= 0 );
    }
#endif


    
    if ( !fPgnosOnly )
    {
        m_pLogReadBuffer->GetLgposOfPbNext( &m_lgposPbNextPreread );
        m_pLogReadBuffer->GetLgposOfPbNextNext( &m_lgposPbNextNextPreread );
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
        m_pLogReadBuffer->GetLgposOfLastChecksum( &m_lgposLastChecksumPreread );
#endif
    }

LRestoreNormal:
    err = m_pLogReadBuffer->ErrReaderRestoreState(
            &lgposPbNextSaved,
            &lgposPbNextNextSaved
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
            ,&lgposLastChecksumSaved
#endif
            );
    if ( JET_errSuccess != err )
    {
        m_fPreread = fFalse;
        LGPrereadTerm();
        return err;
    }

#ifdef DEBUG
    m_pLogReadBuffer->GetLgposOfPbNext( &lgposPbNextT );
    Assert( CmpLgpos( lgposPbNextSaved, lgposPbNextT ) == 0 );
    if ( !FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
    {
        m_pLogReadBuffer->GetLgposOfPbNextNext( &lgposPbNextNextEndT );
        Assert( CmpLgpos( lgposPbNextNextSaved, lgposPbNextNextEndT ) == 0 );
    }
#endif

    return err;
}




typedef struct {
    SHORT   cb:15;
    SHORT   fDebugOnly:1;
} LRD;      


const LRD mplrtyplrd[ ] = {
    {            sizeof( LRTYP ),                0   },
    {            sizeof( LRINIT ),               0   },
    {            sizeof( LRTERMREC ),            0   },
    {            sizeof( LRMS ),                 0   },
    {            sizeof( LRTYP ),                0   },

    {            sizeof( LRBEGIN ),              0   },
    {            sizeof( LRCOMMIT ),             0   },
    {            sizeof( LRROLLBACK ),           0   },
    {            sizeof( LRBEGIN0 ),             0   },
    {            sizeof( LRCOMMIT0 ),            0   },
    {            sizeof( LRREFRESH ),            0   },
    {            sizeof( LRMACROBEGIN ),         0   },
    {            sizeof( LRMACROEND ),           0   },
    {            sizeof( LRMACROEND ),           0   },

    {            0,                              0   },
    {            0,                              0   },
    {            0,                              0   },

    {            sizeof( LRRECOVERYUNDO ),       0   },
    {            sizeof( LRTERMREC ),            0   },
    {            0,                              0   },
    {            0,                              0   },

    {            sizeof( LRJETOP ),              1   },
    {            0,                              1   },

    {            sizeof( LRSHUTDOWNMARK ),       0   },

    {      sizeof( LRCREATEMEFDP ),        0   },
    {      sizeof( LRCREATESEFDP ),        0   },
    {      sizeof( LRCONVERTFDP_OBSOLETE ),0   },

    {            0,                              0   },
    {            0,                              0   },

    {            0,                              0   },
    {          0,                              0   },
{   
                                    0,                              0   },

    {            sizeof( LRFLAGDELETE ),         0   },
    {            0,                              0   },
    {            0,                              0   },

    {             sizeof( LRDELETE ),             0   },
    {            0,                              0   },

    {            0,                              0   },

    {        0,                              0   },
    {            0,                              0   },

    {       0,                              0   },
    {            0,                              0   },
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    {            sizeof( LRCHECKSUM ),           0   },
#else
    {     0,                          0   },
#endif
    {          0,                  0   },
    {    0,                      0   },
    {          0,                              0   },
    {    0,                             0   },
    {           0,                              0   },
    {           0,                              0   },
    {               sizeof( LRINIT2 ),              0   },
    {               sizeof( LRTERMREC2 ),           0   },
    {        sizeof( LRRECOVERYUNDO2 ),      0   },
    {       sizeof( LRTERMREC2 ),           0   },
    {             sizeof( LRBEGINDT ),            0   },
    {          0,                              0   },
    {        0,                              0   },
    {              0,                              0   },
    {       sizeof( LRFORCEWRITELOG),       0   },
    {      sizeof( LRCONVERTFDP ),         0   },
    {       0,                              0   },
    {              0,                              0   },
    {    0,                              0   },
    {              0,                              0   },
    {              0,                              0   },
    {               0,                              0   },
    {            0,                              0   },
    {    sizeof( LRPAGEPATCHREQUEST),    0   },
    {           0,                              0   },
    {         0,                              0   },
    {           0,                              0   },
    {           0,                              0   },
    {                0,                              0   },
    {            0,                              0   },
    {          0,                              0   },
    {             0,                              0   },
    {            0,                              0   },
    {       0,                              0   },
    {            0,                              0   },
    {            sizeof( LRSHRINKDB ),           0   },
    {      sizeof( LRCREATEDBFINISH ),     0   },
    {     sizeof( LRTERMREC2 ), 0 },
    {              sizeof( LRTRIMDB ),             0   },
    {           0,                              0   },
    {           0,                              0   },
    {           0,                              0   },
    {           0,                              0   },
    {           0,                              0   },
    {           0,                              0   },
    {           0,                              0   },
    {           0,                              0   },
    {           0,                              0   },
    {           0,                              0   },
    {        sizeof( LRSETDBVERSION ),       0   },
    {            0,                              0   },
    {           sizeof( LRSHRINKDB ),           0   },
    {             sizeof( LRNEWPAGE ),            0   },
    {           sizeof( LRROOTPAGEMOVE ),       0   },
    {         sizeof( LRSIGNALATTACHDB ),     0   },
    {          sizeof( LRSCANCHECK2 ),         0   },
    {           sizeof( LRSHRINKDB3 ),          0   },
    {         sizeof( LREXTENTFREED ),        0   },
};


#ifdef DEBUG
BOOL FLGDebugLogRec( LR *plr )
{
    return mplrtyplrd[plr->lrtyp].fDebugOnly;
}
#endif



UINT CbLGSizeOfRec( const LR *plr )
{
    UINT    cb;

    if ( plr->lrtyp >= lrtypMax )
    {
        AssertSz( fFalse, "plr->lrtyp = %d higher than lrtypMax = %d", (ULONG)plr->lrtyp, (ULONG)lrtypMax );
        return 0;
    }

    if ( ( cb = mplrtyplrd[plr->lrtyp].cb ) != 0 )
        return cb;

    switch ( plr->lrtyp )
    {
    case lrtypFullBackup:
    case lrtypIncBackup:
    {
        LRLOGRESTORE *plrlogrestore = (LRLOGRESTORE *) plr;
        return sizeof(LRLOGRESTORE) + plrlogrestore->le_cbPath;
    }
    case lrtypBackup:
    {
        LRLOGBACKUP_OBSOLETE *plrlogbackup = (LRLOGBACKUP_OBSOLETE *) plr;
        return sizeof(LRLOGBACKUP_OBSOLETE) + plrlogbackup->le_cbPath;
    }
    case lrtypBackup2:
    {
        LRLOGBACKUP *plrlogbackup = (LRLOGBACKUP *) plr;
        return sizeof(LRLOGBACKUP) + plrlogbackup->le_cbPath;
    }

    case lrtypCreateDB:
    {
        LRCREATEDB *plrcreatedb = (LRCREATEDB *)plr;
        Assert( plrcreatedb->CbPath() != 0 );
        return ( sizeof(LRCREATEDB)
                + plrcreatedb->CbPath()
                + ( plrcreatedb->FVersionInfo() ? sizeof(LRCREATEDB::VersionInfo) : 0 ) );
    }
    case lrtypAttachDB:
    {
        LRATTACHDB *plrattachdb = (LRATTACHDB *)plr;
        Assert( plrattachdb->CbPath() != 0 );
        return sizeof(LRATTACHDB) + plrattachdb->CbPath();
    }
    case lrtypDetachDB:
    {
        LRDETACHDB *plrdetachdb = (LRDETACHDB *)plr;
        Assert( plrdetachdb->CbPath() != 0 );
        return sizeof( LRDETACHDB ) + plrdetachdb->CbPath();
    }

    case lrtypSplit:
    {
        LRSPLITOLD *plrsplit = (LRSPLITOLD *) plr;
        return sizeof( LRSPLITOLD ) +
                plrsplit->le_cbKeyParent +
                plrsplit->le_cbPrefixSplitOld +
                plrsplit->le_cbPrefixSplitNew;
    }
    
    case lrtypSplit2:
    {
        LRSPLITNEW *plrsplit = (LRSPLITNEW *) plr;
        return sizeof( LRSPLITNEW ) +
                plrsplit->le_cbKeyParent +
                plrsplit->le_cbPrefixSplitOld +
                plrsplit->le_cbPrefixSplitNew +
                plrsplit->CbPageBeforeImage();
    }
    
    case lrtypMerge:
    {
        LRMERGEOLD *plrmerge = (LRMERGEOLD *) plr;
        return sizeof( LRMERGEOLD ) + plrmerge->le_cbKeyParentSep;
    }

    case lrtypMerge2:
    {
        LRMERGENEW *plrmerge = (LRMERGENEW *) plr;
        return sizeof( LRMERGENEW ) + plrmerge->le_cbKeyParentSep + plrmerge->CbPageBeforeImage();
    }

    case lrtypPageMove:
    {
        const LRPAGEMOVE * const plrpagemove = LRPAGEMOVE::PlrpagemoveFromLr( plr );
        return sizeof( LRPAGEMOVE ) + plrpagemove->CbTotal();
    }

    case lrtypEmptyTree:
    {
        LREMPTYTREE *plremptytree = (LREMPTYTREE *)plr;
        return sizeof(LREMPTYTREE) + plremptytree->CbEmptyPageList();
    }

    case lrtypDelta:
    {
        LRDELTA32   *plrdelta = (LRDELTA32 *) plr;
        return sizeof( LRDELTA32 ) +
               plrdelta->CbBookmarkKey() + plrdelta->CbBookmarkData();
    }

    case lrtypDelta64:
    {
        LRDELTA64   *plrdelta = (LRDELTA64 *) plr;
        return sizeof( LRDELTA64 ) +
            plrdelta->CbBookmarkKey() + plrdelta->CbBookmarkData();
    }

    case lrtypInsert:
    {
        LRINSERT *plrinsert = (LRINSERT *) plr;
        return  sizeof(LRINSERT) +
                plrinsert->CbSuffix() + plrinsert->CbPrefix() + plrinsert->CbData();
    }

    case lrtypFlagInsert:
    {
        LRFLAGINSERT    *plrflaginsert = (LRFLAGINSERT *) plr;
        return  sizeof(LRFLAGINSERT) +
                plrflaginsert->CbKey() + plrflaginsert->CbData();
    }

    case lrtypFlagInsertAndReplaceData:
    {
        LRFLAGINSERTANDREPLACEDATA *plrfiard = (LRFLAGINSERTANDREPLACEDATA *) plr;
        return sizeof(LRFLAGINSERTANDREPLACEDATA) + plrfiard->CbKey() + plrfiard->CbData();
    }

    case lrtypReplace:
    case lrtypReplaceD:
    {
        LRREPLACE *plrreplace = (LRREPLACE *) plr;
        return sizeof(LRREPLACE) + plrreplace->Cb();
    }

    case lrtypUndoInfo:
    {
        LRUNDOINFO *plrdbi = (LRUNDOINFO *) plr;
        return sizeof( LRUNDOINFO ) + plrdbi->le_cbData +
               plrdbi->CbBookmarkKey() +
               plrdbi->CbBookmarkData();
    }

    case lrtypUndo:
    {
        LRUNDO  *plrundo = (LRUNDO *) plr;

        return sizeof( LRUNDO ) +
               plrundo->CbBookmarkKey() +
               plrundo->CbBookmarkData();
    }

    case lrtypTrace:
    case lrtypForceLogRollover:
    {
        LRTRACE *plrtrace = (LRTRACE *) plr;
        return sizeof(LRTRACE) + plrtrace->le_cb;
    }

    case lrtypSetExternalHeader:
    {
        LRSETEXTERNALHEADER *plrsetextheader = (LRSETEXTERNALHEADER *) plr;
        return sizeof(LRSETEXTERNALHEADER) + plrsetextheader->CbData();
    }

    case lrtypScrub:
    {
        const LRSCRUB * const plrscrub = (LRSCRUB *) plr;
        return sizeof(LRSCRUB) + plrscrub->CbData();
    }

    case lrtypExtRestore:
    {
        LREXTRESTORE *plrextrestore = (LREXTRESTORE *) plr;
        return  sizeof(LREXTRESTORE) +  plrextrestore->CbInfo();
    }
    
    case lrtypExtRestore2:
    {
        LREXTRESTORE2 *plrextrestore = (LREXTRESTORE2 *) plr;
        return  sizeof(LREXTRESTORE2) + plrextrestore->CbInfo();
    }

    case lrtypPrepCommit:
    {
        const LRPREPCOMMIT  * const plrprepcommit   = (LRPREPCOMMIT *)plr;
        return sizeof(LRPREPCOMMIT) + plrprepcommit->le_cbData;
    }

    case lrtypMacroInfo:
    case lrtypExtendDB:
    case lrtypCommitCtx:
    case lrtypScanCheck:
    case lrtypNOP2:
    case lrtypReAttach:
    case lrtypMacroInfo2:
    case lrtypFreeFDP:
    case lrtypIgnored9:
    case lrtypIgnored10:
    case lrtypIgnored11:
    case lrtypIgnored12:
    case lrtypIgnored13:
    case lrtypIgnored14:
    case lrtypIgnored15:
    case lrtypIgnored16:
    case lrtypIgnored17:
    case lrtypIgnored18:
    case lrtypIgnored19:
    {
        const LRIGNORED * const plrignored = (LRIGNORED *)plr;
        return sizeof(LRIGNORED) + plrignored->Cb();
    }

    case lrtypFragmentBegin:
    {
        const LRFRAGMENTBEGIN * const plrFragmentBegin = (LRFRAGMENTBEGIN *)plr;
        return sizeof(LRFRAGMENTBEGIN) + plrFragmentBegin->le_cbFragmentSize;
    }

    case lrtypFragment:
    {
        const LRFRAGMENT * const plrFragment = (LRFRAGMENT *)plr;
        return sizeof(LRFRAGMENT) + plrFragment->le_cbFragmentSize;
    }

    default:
        Assert( fFalse );
        return 0;
    }
}



const SHORT mplrtypn[ ] =
{
    sizeof( LRNOP ),
    sizeof( LRINIT ),
    sizeof( LRTERMREC ),
    sizeof( LRMS ),
    sizeof( LRTYP ),
    sizeof( LRBEGIN ),
    sizeof( LRCOMMIT ),
    sizeof( LRROLLBACK ),
    sizeof( LRBEGIN0 ),
    sizeof( LRCOMMIT0 ),
    sizeof( LRREFRESH ),
    sizeof( LRMACROBEGIN ),
    sizeof( LRMACROEND ),
    sizeof( LRMACROEND ),
    sizeof( LRCREATEDB ),
    sizeof( LRATTACHDB ),
    sizeof( LRDETACHDB ),
    sizeof( LRRECOVERYUNDO ),
    sizeof( LRTERMREC ),
    sizeof( LRLOGRESTORE ),
    sizeof( LRLOGRESTORE ),
    sizeof( LRJETOP ),
    sizeof( LRTRACE ),
    sizeof( LRSHUTDOWNMARK ),
    sizeof( LRCREATEMEFDP ),
    sizeof( LRCREATESEFDP ),
    sizeof( LRCONVERTFDP_OBSOLETE ),
    sizeof( LRSPLITOLD ),
    sizeof( LRMERGEOLD ),
    sizeof( LRINSERT ),
    sizeof( LRFLAGINSERT ),
    sizeof( LRFLAGINSERTANDREPLACEDATA ),
    sizeof( LRFLAGDELETE ),
    sizeof( LRREPLACE ),
    sizeof( LRREPLACE ),
    sizeof( LRDELETE ),
    sizeof( LRUNDOINFO ),
    sizeof( LRDELTA32 ),
    sizeof( LRSETEXTERNALHEADER ),
    sizeof( LRUNDO ),
    0,
    0,
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    sizeof( LRCHECKSUM ),
#else
    0,
#endif
    0,
    0,
    sizeof( LREXTRESTORE ),
    sizeof( LRLOGBACKUP_OBSOLETE ),
    0,
    sizeof( LREMPTYTREE ),
    sizeof( LRINIT2 ),
    sizeof( LRTERMREC2 ),
    sizeof( LRRECOVERYUNDO2 ),
    sizeof( LRTERMREC2 ),
    sizeof( LRBEGINDT ),
    sizeof( LRPREPCOMMIT ),
    sizeof( LRPREPROLLBACK ),
    0,
    sizeof( LRFORCEWRITELOG ),
    sizeof( LRCONVERTFDP ),
    sizeof( LREXTRESTORE2 ),
    sizeof( LRLOGBACKUP ),
    sizeof( LRFORCELOGROLLOVER ),
    sizeof( LRSPLITNEW ),
    sizeof( LRMERGENEW ),
    sizeof( LRSCRUB ),
    sizeof( LRPAGEMOVE ),
    sizeof( LRPAGEPATCHREQUEST ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRFRAGMENTBEGIN ),
    sizeof( LRFRAGMENT ),
    sizeof( LRSHRINKDB ),
    sizeof( LRCREATEDBFINISH ),
    sizeof( LRTERMREC2 ),
    sizeof( LRTRIMDB ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRIGNORED ),
    sizeof( LRSETDBVERSION ),
    sizeof( LRDELTA64 ),
    sizeof( LRSHRINKDB ),
    sizeof( LRNEWPAGE ),
    sizeof( LRROOTPAGEMOVE ),
    sizeof( LRSIGNALATTACHDB ),
    sizeof( LRSCANCHECK2 ),
    sizeof( LRSHRINKDB3 ),
    sizeof( LREXTENTFREED ),
};

UINT CbLGFixedSizeOfRec( const LR * const plr )
{
    if ( plr->lrtyp >= lrtypMax )
    {
        AssertSz( fFalse, "plr->lrtyp = %d higher than lrtypMax = %d", (ULONG)plr->lrtyp, (ULONG)lrtypMax );
        return 0;
    }

    return mplrtypn[ plr->lrtyp ];
}

#ifdef DEBUG

VOID AssertLRSizesConsistent()
{
    C_ASSERT( _countof( mplrtypn ) == lrtypMax );
    C_ASSERT( _countof( mplrtyplrd ) == lrtypMax );

    for ( LRTYP iLrtyp = 0; iLrtyp < lrtypMax; iLrtyp++ )
    {
        if ( 0 != mplrtyplrd[ iLrtyp ].cb )
        {
            Assert( mplrtyplrd[ iLrtyp ].cb == mplrtypn[ iLrtyp ] );
        }
    }

}

#endif


INLINE ERR LogPrereader::ErrLGPIPrereadPage( __in_range( dbidUserLeast, dbidMax - 1 ) const DBID dbid, const PGNO pgno, const BFPreReadFlags bfprf )
{
    Assert( FLGPDBEnabled( dbid ) );
    Assert( dbid >= dbidUserLeast && dbid < dbidMax );
    Expected( ( ~( bfprfNoIssue | bfprfCombinableOnly | bfprfDBScan ) & bfprf ) == 0 );

    const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];

    return ErrBFPrereadPage( ifmp, pgno, bfprf, BfpriBFMake( PctFMPCachePriority( ifmp ), (BFTEMPOSFILEQOS)qosIODispatchImmediate ), TcCurr() );
}

ERR LogPrereader::ErrLGPIPrereadIssue( __in_range( dbidUserLeast, dbidMax - 1 ) const DBID dbid )
{
    Assert( dbid >= dbidUserLeast && dbid < dbidMax );
    const ERR err = g_rgfmp[ m_pinst->m_mpdbidifmp[ dbid ] ].Pfapi()->ErrIOIssue();

    return err;
}

LogPrereader::LogPrereader( INST* const pinst ) :
    LogPrereaderBase(),
    m_pinst( pinst )
{
    Assert( pinst != NULL );
}


