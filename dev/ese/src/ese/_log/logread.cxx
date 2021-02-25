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

    //  perform unit test for checksum code, on LE only.

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

// Convert pb in log buffer to LGPOS *only* when we're
// actively reading the log file. Do *NOT* use when we're writing
// to the log file.

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

    // Compute how many sectors from the end of the log buffer we're at.
    // pbRead is the end of data in the log buffer.
    // This first case is for wrapping around in the log buffer.
    if ( pbAligned > pbRead )
        isec = ULONG( pbRead + m_cbLGBuf - pbAligned ) >> log2CbSec;
    else
        isec = ULONG( pbRead - pbAligned ) >> log2CbSec;

    // isecRead is the next sector to pull in from disk.
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
    // only used by new format
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

/* calculate the lgpos of the LR */
void LOG::PrintLgposReadLR ( VOID )
{
    LGPOS lgpos;
    m_pLogReadBuffer->GetLgposOfPbNext(&lgpos);
    (*CPRINTFSTDOUT::PcprintfInstance())( ">%08X,%04X,%04X", m_pLogStream->GetCurrentFileGen(),lgpos.isec, lgpos.ib);
}

// Resize the log buffers to requested size, hook up with LOG object

ERR LOG_READ_BUFFER::ErrLReaderInit( UINT csecBufSize )
{
    ERR err;

    Assert( !m_fReaderInited );

    Assert( csecBufSize > 0 );

    //  setup members

    m_fReadSectorBySector   = fFalse;

    m_ftFirstSegment = 0;
    m_hrtFirstSegment = 0;

    m_isecReadStart         = 0;
    m_csecReader            = 0;

    m_fIgnoreNextSectorReadFailure  = fTrue;

    m_wszReaderLogName[0]   = 0;

    //  allocate log buffers

    CallR( m_pLogBuffer->ErrLGInitLogBuffers( m_pinst, m_pLogStream, csecBufSize, fTrue ) );
    CallR( m_pLogBuffer->EnsureCommitted( m_pbLGBufMin, m_cbLGBuf ) );
    //  make "none" of log buffer used.

    m_pbWrite = m_pbLGBufMin;
    m_pbEntry = m_pbWrite;

    m_fReaderInited = fTrue;

    return JET_errSuccess;
}

// Resize log buffers to original size

ERR LOG_READ_BUFFER::ErrLReaderTerm()
{
    m_fReaderInited = fFalse;

    //  reset the buffer pointers

    m_pbWrite = m_pbLGBufMin;
    m_pbEntry = m_pbWrite;

    m_wszReaderLogName[0]   = 0;

    return JET_errSuccess;
}


//  this is like calling Term followed by Init without deallocating and reallocating the log buffers
//  it just resets the internal pointers to invalidate the log buffer data and force a fresh read from disk

VOID LOG_READ_BUFFER::LReaderReset()
{
#if DEBUG
    //
    // save the last log buffer for debugging purposes (the logging code
    // re-reads the same file currently, so try not to overwrite the
    // buffer in that case)
    //
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
            // if the number of sectors read is different, need a buffer of different size
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

            // just ignore allocation failure
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

    //  invalidate buffers for log-flush code

    m_pbWrite = m_pbLGBufMin;
    m_pbEntry = m_pbWrite;

    //  reset state

    m_ftFirstSegment = 0;
    m_hrtFirstSegment = 0;

    m_fReadSectorBySector   = fFalse;

    m_isecReadStart             = 0;
    m_csecReader                = 0;

    m_fIgnoreNextSectorReadFailure  = fTrue;
}

// Make sure that our buffer reflects the current log file.

ERR LOG_READ_BUFFER::ErrReaderEnsureLogFile()
{
    // ensure that we're going to be giving the user the right log file.

    if ( 0 == UtilCmpFileName( m_pLogStream->LogName(), m_wszReaderLogName ) )
    {
        // yup, got the right file already
    }
    else
    {
        // wrong file
        // setup so next ErrEnsureSector() will pull in from the right file.
        LReaderReset();

        OSStrCbCopyW( m_wszReaderLogName, sizeof( m_wszReaderLogName ), m_pLogStream->LogName() );
    }

    return JET_errSuccess;
}

// Ensure that the data from the log file starting at sector isec for csec
// sectors is in the log buffer (at a minimum). The addr of the data is
// returned in ppb.

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

    // Set this now to pbNil, in case we return an error.
    // We want to know if someone's using *ppb without checking for
    // an error first.
    *ppb = pbNil;

    // We only have it if we have the same log file that the user has open now
    if ( isec >= m_isecReadStart && isec + csec <= m_isecReadStart + m_csecReader )
    {
        // already have it in the buffer.
        *ppb = m_pbLGBufMin +
            ( isec - m_isecReadStart ) * m_pLogStream->CbSec();
        return JET_errSuccess;
    }

    // If log buffer isn't big enough to fit the sectors user
    // wants, we must enlarge
    if ( m_csecLGBuf < csec )
    {
        Call( m_pLogBuffer->ErrLGInitLogBuffers( m_pinst, m_pLogStream, csec, fTrue ) );
        m_pbWrite = m_pbLGBufMin;
        m_pbEntry = m_pbWrite;
    }

LReadSectorBySector:
    
    // read in the data
    if ( m_fReadSectorBySector )
    {
        if ( m_csecReader == 0 || m_isecReadStart > isec )
        {
            // If we've got 0 sectors stored in the log buffer,
            // the first sector in the buffer is the user requested one.
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
                    // We found a corrupted sector in the log file. This
                    // is unexpected and an error.
                    Call( ErrERRCheck( err ) );
                }
                m_fIgnoreNextSectorReadFailure = fFalse;
                // treat bad sector read as zero filled sector.
                // Hopefully the next sector will be a shadow sector.
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
        // user should never request more than what's in the log file
        Assert( csec <= csecLeftInFile );
        // We don't want to read more than the space we have in the log buffer
        // and not more than what's left in the log file.
        UINT    csecRead        = min( csecLeftInFile, m_csecLGBuf );
        // Of the user requested, we'd like to read more, if we have space
        // in the log buffer and if the file has that space.
        csecRead = max( csecRead, csec );

        // if we have an error doing a huge read, try going sector by
        // sector in the case of the corrupted sector that is causing
        // this to give an error.

        const DWORD cbReadMax   = (DWORD)UlParam( JET_paramMaxCoalesceReadSize );
        QWORD       ib          = QWORD( isec ) * m_pLogStream->CbSec();
        DWORD       cb          = csecRead * m_pLogStream->CbSec();
        BYTE        *pb         = m_pbLGBufMin;

        while ( cb )
        {

            //  decide how much to read

            const DWORD cbRead = min( cb, cbReadMax );

            err = ErrFaultInjection( 39546 );
            if ( err < 0 )
            {
                m_fReadSectorBySector = fTrue;
                goto LReadSectorBySector;
            }

            //  issue the read (fall back to sector-by-sector reads at the first sign of trouble)

            err = m_pLogStream->ErrLGReadSectorData( ib, cbRead, pb );
            if ( err < 0 )
            {
                m_fReadSectorBySector = fTrue;
                goto LReadSectorBySector;
            }

            //  update the read counters

            ib += cbRead;
            cb -= cbRead;
            pb += cbRead;

            //  verify that we don't exceed the log buffer

            Assert( ( cb > 0 && pb < m_pbLGBufMax ) || ( 0 == cb && pb <= m_pbLGBufMax ) );
        }

        // the first sector is what the user requested
        m_isecReadStart = isec;
        // this is how many sectors we have in the buffer
        m_csecReader = csecRead;
    }

    m_pbWrite = m_pbLGBufMin;
    m_pbEntry = m_pbWrite + m_csecReader * m_pLogStream->CbSec();
    if ( m_pbEntry == m_pbLGBufMax )
    {
        m_pbEntry = m_pbLGBufMin;
    }

    // if we get here, we just read in the stuff the user wanted
    Assert( isec >= m_isecReadStart );
    Assert( isec + csec <= m_isecReadStart + m_csecReader );
    *ppb = m_pbLGBufMin +
            ( isec - m_isecReadStart ) * m_pLogStream->CbSec();

HandleError:

    return err;
}

// If a client modified the log file on disk, we want our buffer to reflect that
// change. Copy a modified sector from a user buffer (perhaps the log buffer itself),
// into the log buffer.

VOID LOG_READ_BUFFER::ReaderSectorModified(
    const UINT isec,
    const BYTE* const pb
    )
{
    Assert( isec >= m_pLogStream->CSecHeader() );
    Assert( pbNil != pb );

    // If we have the sector in memory, update our in memory copy
    // so that we'll be consistent with what's on the disk.
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

    // only used by new format
    if ( plgposPbNextNext->isec != 0 )
    {
        m_pbNextNext = pb + m_pLogStream->CbSec() * ( plgposPbNextNext->isec - minSec ) + plgposPbNextNext->ib;
        Assert( m_pbNextNext >= m_pbLGBufMin && m_pbNextNext < m_pbLGBufMax );
    }

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    // only used by old format code
    if ( plgposChecksum->isec != 0 )
    {
        m_pbLastChecksum = pb + m_pLogStream->CbSec() * ( plgposChecksum->isec - minSec ) + plgposChecksum->ib;
        Assert( m_pbLastChecksum >= m_pbLGBufMin && m_pbLastChecksum < m_pbLGBufMax );

        m_lgposLastChecksum = *plgposChecksum;
    }
#endif

    return JET_errSuccess;
}


//  Implementation of LogPrereaderBase.

LogPrereaderBase::LogPrereaderBase() :
    m_dbidMaxUsed( 0 ),
    m_cpgGrowth( 0 ),
    m_rgArrayPagerefs( NULL )
{
}

LogPrereaderBase::~LogPrereaderBase()
{
    Expected( !FLGPEnabled() );
    LGPTerm();
}

//  This method initializes the log prereader by storing its configuration parameters
//  and trying to to allocate the array of CArray<PageRef>'s. If it fails to allocate,
//  that is fine as the prereading code will try and initialize it when it needs it.
//  It is NOT valid to call it when it has been already initialized.

VOID LogPrereaderBase::LGPInit( const DBID dbidMaxUsed, const CPG cpgGrowth )
{
    Assert( !FLGPEnabled() );
    Assert( dbidMaxUsed > 0 && cpgGrowth > 0 );
    Assert( m_rgArrayPagerefs == NULL || m_dbidMaxUsed > 0 );
    Assert( m_rgArrayPagerefs == NULL || m_cpgGrowth > 0 );
    m_dbidMaxUsed = dbidMaxUsed;
    m_cpgGrowth = cpgGrowth;
    m_rgArrayPagerefs = new CArray<PageRef>[ m_dbidMaxUsed ];
    Assert( m_rgArrayPagerefs == NULL || FLGPEnabled() );

    //  Set the default growth size.
    if ( m_rgArrayPagerefs != NULL )
    {
        for ( DBID dbid = dbidMin; dbid < m_dbidMaxUsed; dbid++ )
        {
            m_rgArrayPagerefs[ dbid ].SetCapacityGrowth( m_cpgGrowth );
        }
    }
}

//  This method frees all the CArray<PageRef>'s, as well as the array of those objects
//  as well.
//  It is valid to call it when the prereader is not initialized, in which
//  case, the function just returns silently.

VOID LogPrereaderBase::LGPTerm()
{
    if ( !FLGPEnabled() )
    {
        return;
    }

    //  Free all the arrays.
    for ( DBID dbid = dbidMin; dbid < m_dbidMaxUsed; dbid++ )
    {
        LGPDBDisable( dbid );
        Assert( !FLGPDBEnabled( dbid ) );
    }

    //  Free the array of arrays.
    delete[] m_rgArrayPagerefs;
    m_rgArrayPagerefs = NULL;
    Assert( !FLGPEnabled() );
}

//  This method allocates the CArray<PageRef> object for the specific dbid if it is not
//  already allocated or sets the array size to zero (i.e., remove all the PGNO's) if
//  the object is already allocated, so it is valid to call it when it has been already
//  initialized.
//  In the case where it needs to allocate, it will also try and initialize the array of
//  CArray<PageRef>'s if it's not already initialized.

VOID LogPrereaderBase::LGPDBEnable( const DBID dbid )
{
    Assert( m_dbidMaxUsed > 0 );
    Assert( dbid < m_dbidMaxUsed );
    
    //  If we are re-enabling, then just re-initialize the array.
    //  Otherwise, we need to allocate all the data structures.
    if ( FLGPDBEnabled( dbid ) )
    {
        const CArray<PageRef>::ERR errSetSize = m_rgArrayPagerefs[ dbid ].ErrSetSize( 0 );
        Assert( errSetSize == CArray<PageRef>::ERR::errSuccess );
        Assert( FLGPDBEnabled( dbid ) );
    }
    else
    {
        //  We may need to initialize the main array.
        if ( !FLGPEnabled() )
        {
            LGPInit( m_dbidMaxUsed, m_cpgGrowth );
        }

        //  Silently return.
        if ( !FLGPEnabled() )
        {
            return;
        }

        //  Set the initial capacity. It is not a problem if it fails, as we are going to
        //  try and allocate in the next log file.
        (VOID)m_rgArrayPagerefs[ dbid ].ErrSetCapacity( m_cpgGrowth );
    }

    Assert( CpgLGPIGetArrayPgnosSize( dbid ) == 0 );
}

//  This method frees the specific CArray<PageRef> object related to the specified dbid.
//  It is valid to call it if the specific CArray<PageRef> is not initialized yet, in which
//  case we will just return silently.

VOID LogPrereaderBase::LGPDBDisable( const DBID dbid )
{
    if ( !FLGPDBEnabled( dbid ) )
    {
        return;
    }

    const CArray<PageRef>::ERR errSetCapacity = m_rgArrayPagerefs[ dbid ].ErrSetCapacity( 0 );
    Assert( errSetCapacity == CArray<PageRef>::ERR::errSuccess );
    Assert( !FLGPDBEnabled( dbid ) );
}

INLINE BOOL LogPrereaderBase::FLGPEnabled() const
{
    return m_rgArrayPagerefs != NULL;
}

INLINE BOOL LogPrereaderBase::FLGPDBEnabled( const DBID dbid ) const
{
    if ( !FLGPEnabled() )
    {
        return fFalse;
    }

    return m_rgArrayPagerefs[ dbid ].Capacity() > 0;
}

ERR LogPrereaderBase::ErrLGPAddPgnoRef( const DBID dbid, const PGNO pgno, const LR* const plr )
{
    ERR err = JET_errSuccess;

    if ( !FLGPDBEnabled( dbid ) || pgno == pgnoNull )
    {
        goto HandleError;
    }

    //  Add even if it exists already. We'll sort and remove duplicates later.
    Call( ErrLGPISetEntry( dbid, CpgLGPIGetArrayPgnosSize( dbid ), pgno, IorsFromLr( plr ) ) );

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
            m_rgArrayPagerefs[ dbid ].SortAndRemoveDuplicates( LogPrereaderBase::ILGPICmpPagerefs );
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

    //  Find the page in our array.
    const size_t ipgMain = IpgLGPGetSorted( dbid, pgno );

    if ( ipgMain == CArray<PageRef>::iEntryNotFound )
    {
        ExpectedSz( fFalse, "We must have added this page first!" );
        Call( ErrERRCheck( JET_errInternalError ) );
    }

    Assert( PgnoLGPIGetEntry( dbid, ipgMain ) == pgno );

    //  Auxiliary variables.
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
        
        //  First time, we don't want to use bfprfCombinableOnly because we want to start
        //  a brand new I/O run in the TLS and issue whatever is pending to the heap.
        const BFPreReadFlags bfprfCombinable = ( ipg == ipgInitial ) ? bfprfDefault : bfprfCombinableOnly;

        const PGNO pgnoThisPass = PgnoLGPIGetEntry( dbid, (size_t)ipg );

        Expected( pgnoThisPass != pgnoNull );

        TraceContextScope tcScope;
        tcScope->iorReason.SetIors( IorsLGPIGetEntry( dbid, (size_t)ipg ) );

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

            //  If we are not on the first page, we may proceed depending on the error we've just hit.
            if ( ( err == errDiskTilt || err == errBFPageCached ) && ipg != ipgInitial )
            {
                err = JET_errSuccess;
            }
        }

        //  Have we hit the boundaries?
        if ( ipg == 0 )
        {
            fMayGoLeft = false;
        }
        if ( ipg == ( ipgMax - 1 ) )
        {
            fMayGoRight = false;
        }

        //  Next delta.
        if ( dipg > 0 )
        {
            if ( fMayGoLeft )
            {
                Assert( fOscillating );
                dipg = -( dipg + 1 );
            }
            else
            {
                //  If we can't go neither left nor right, we'll come out of the loop anyways.
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
                //  If we can't go neither right nor left, we'll come out of the loop anyways.
                dipg = -1;
                OnDebug( fOscillating = false );
            }
        }

        //  Next page to preread.
        ipg += dipg;
    } while ( ( err >= JET_errSuccess ) && ( fMayGoLeft || fMayGoRight ) );

HandleError:

    //  By design, we can't get errDiskTilt on the first attempt, so errDiskTilt should
    //  never get here.
    Assert( err != errDiskTilt );

    return err;
}

INLINE size_t LogPrereaderBase::CpgLGPIGetArrayPgnosSize( const DBID dbid ) const
{
    Assert( FLGPEnabled() );

    return m_rgArrayPagerefs[ dbid ].Size();
}

INLINE size_t LogPrereaderBase::IpgLGPGetSorted( const DBID dbid, const PGNO pgno ) const
{
    Assert( FLGPDBEnabled( dbid ) );

    return m_rgArrayPagerefs[ dbid ].SearchBinary( PageRef( pgno ), LogPrereaderBase::ILGPICmpPagerefs );
}

size_t LogPrereaderBase::IpgLGPIGetUnsorted( const DBID dbid, const PGNO pgno ) const
{
    Assert( FLGPDBEnabled( dbid ) );

    return m_rgArrayPagerefs[ dbid ].SearchLinear( PageRef( pgno ), LogPrereaderBase::ILGPICmpPagerefs );
}

ERR LogPrereaderBase::ErrLGPISetEntry( const DBID dbid, const size_t ipg, const PGNO pgno, const IOREASONSECONDARY iors )
{
    Assert( FLGPDBEnabled( dbid ) );

    ERR err;

    const CArray<PageRef>::ERR errT = m_rgArrayPagerefs[ dbid ].ErrSetEntry( ipg, PageRef( pgno, iors ) );

    switch ( errT )
    {
        case CArray<PageRef>::ERR::errSuccess:
            err = JET_errSuccess;
            break;
        
        case CArray<PageRef>::ERR::errOutOfMemory:
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
    return m_rgArrayPagerefs[ dbid ].Entry( ipg ).pgno;
}

IOREASONSECONDARY LogPrereaderBase::IorsLGPIGetEntry( const DBID dbid, const size_t ipg ) const
{
    Assert( FLGPDBEnabled( dbid ) );
    return m_rgArrayPagerefs[dbid].Entry( ipg ).iors;
}

INLINE INT __cdecl LogPrereaderBase::ILGPICmpPagerefs(  const LogPrereaderBase::PageRef*    ppageref1,
                                                        const LogPrereaderBase::PageRef*    ppageref2 )
{
    const PGNO pgno1 = ppageref1->pgno;
    const PGNO pgno2 = ppageref2->pgno;

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
            // empty page
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
                // shadow page
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


//  check the log, searching for the last log record and seeing whether or not it is a term record
//  this will patch any damange to the log as well (when it can)

// Implicit output parameters:
//      m_pbWrite
//      m_isecWrite
//      m_pbEntry
//      m_lgposLastRec

// At the end of this function, we should have done any necessary fix-ups
// to the log file. i.e. writing a shadow sector that didn't get written,
// writing over a corrupted regular data sector.

// Note that if any patchable corruption is encountered and pfPatchable is
// non-null, it is set to true and the function returns success
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

    //  we should not be probing for a torn-write unless we are dumping logs

    Assert( !pfIsPatchable || m_pLog->FDumpingLogs() );

    //  initialize variables

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
    //  assume the result will not be a cleanly shutdown log generation
    //     (e.g. we expect not to find an lrtypTerm or lrtypQuit record)

    *pfCloseNormally = fFalse;
    if ( plgposLastTerm != NULL )
    {
        *plgposLastTerm = lgposMin;
    }


    //  are we looking at edb.jtx/log?

{
    WCHAR wszT[IFileSystemAPI::cchPathMax], wszFNameT[IFileSystemAPI::cchPathMax];

    CallS( m_pinst->m_pfsapi->ErrPathParse( m_pLogStream->LogName(), wszT, wszFNameT, wszT ) );
    fJetLog = ( UtilCmpFileName( wszFNameT, SzParam( m_pinst, JET_paramBaseName ) ) == 0 );
}


    Assert( m_pLog->FRecovering() );
    Assert( m_pLog->FRecoveringMode() == fRecoveringRedo );

    //  allocate a LogReader class (coming from dump code) or share the existing one (coming from redo code)

    if ( m_fReaderInited )
    {
        fCreatedLogReader = fFalse;

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
        // If we switched from old format log to new format log in the middle of
        // replay, the segment size changed and need to update buffer for that
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
        //  make sure the log buffer is large enough for the whole file

        QWORD   cbSize;
        Call( m_pLogStream->ErrFileSize( &cbSize ) );

        Assert( m_pLogStream->CbSec() > 0 );
        Assert( ( cbSize % m_pLogStream->CbSec() ) == 0 );
        UINT    csecSize;
        csecSize = UINT( cbSize / m_pLogStream->CbSec() );
        Assert( csecSize > m_pLogStream->CSecHeader() );

        Assert( m_csecLGBuf >= csecSize );
#endif  //  DEBUG
    }
    else
    {
        fCreatedLogReader = fTrue;

        //  make sure the log buffer is large enough for the whole file

        QWORD   cbSize;
        Call( m_pLogStream->ErrFileSize( &cbSize ) );

        Assert( m_pLogStream->CbSec() > 0 );
        Assert( ( cbSize % m_pLogStream->CbSec() ) == 0 );
        UINT    csecSize;
        csecSize = UINT( cbSize / m_pLogStream->CbSec() );
        Assert( csecSize > m_pLogStream->CSecHeader() );

        //  initialize the log reader
        Call( ErrLReaderInit( csecSize ) );

        Assert( m_csecLGBuf >= csecSize );
    }
    Assert( m_cbLGBuf / m_csecLGBuf == m_pLogStream->CbSec() );

    //  re-open the log in read/write mode if requested by the caller

    Assert( m_pLogStream->FLGFileOpened() );
    if ( !fReadOnly )
    {
        Assert( NULL != m_pLogStream->LogName() );
        Assert( L'\0' != m_pLogStream->LogName()[ 0 ] );
    }

    //  make sure we have the right log file

    Call( ErrReaderEnsureLogFile() );
    m_fIgnoreNextSectorReadFailure = fFalse;

    // m_pLog->SetNOP( 0 );

    for ( lgposCurrent.isec = USHORT( m_pLogStream->CSecHeader() );
          lgposCurrent.isec < m_pLogStream->CSecLGFile() - 1;
          lgposCurrent.isec++ )
    {
        // try reading next sector
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
                    // is it shadow sector?
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
                        // empty segments should be in non-increasing lGen's
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
                    // invalid segment after empty segment cannot be fixed (even with
                    // shadow sector)
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
                    // empty segments should be in non-increasing lGen's
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
                    // invalid lgpos
                    lLine = __LINE__;
                    reason = eLCBadSegmentLgpos;
                    goto ReportCorruption;
                }

                if ( fEmptyPageSeen )
                {
                    // valid segment after empty page
                    lLine = __LINE__;
                    reason = eLCValidSegmentAfterEmpty;
                    goto ReportCorruption;
                }

                // segment is good
                if ( fUpdateAccSegChecksum )
                {
                    m_pLogStream->AccumulateSectorChecksum( pSegHdr->checksum );
                }
            }
        }

        if ( err < JET_errSuccess )
        {
            // can we fixup with shadow sector?
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
                // This is not the shadow sector we are looking for
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
                //  the caller wants to know if the log is patchable
                //  by returning TRUE, we imply that the log is damaged so we
                //  don't need to return an error as well

                *pfIsPatchable = fTrue;
                goto HandleError;
            }
            // SOFT_HARD: leave, at least for compatibility
            if ( m_pLog->FHardRestore() )
            {
                //  we are in hard-recovery mode
                LONG lgenLowRestore, lgenHighRestore;
                m_pLog->LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
                if ( lgposCurrent.lGeneration <= lgenHighRestore )
                {
                    fIsTornWrite = fTrue;
                    lLine = __LINE__;
                    reason = eLCCannotPatchHardRecovery;
                    goto ReportCorruption;
                }

                //  the current log generation is not part of the backup-set
                //  we can only patch edb.jtx/log -- all other generations
                //  require offline patching because generations after the bad
                //  one need to be deleted only report an error message when we
                //  cannot patch the log file
                if ( !fJetLog )
                {
                    fIsTornWrite = fTrue;
                    lLine = __LINE__;
                    reason = eLCCannotPatchHardRecovery;
                    goto ReportCorruption;
                }
            }

            //  send the event-log message
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

            // Segment is good
            if ( fUpdateAccSegChecksum )
            {
                pSegHdr = (LGSEGHDR*) pbSegment;
                m_pLogStream->AccumulateSectorChecksum( pSegHdr->checksum );
            }
        }

        pbLastValidSegment = pbSegment;
        lgposLastValidSegment = lgposCurrent;
    }

    // Is the last record in the log file a term etc - note that term record
    // is never fragmented because of the minimum fragment size
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

    //  prepare the event-log messages

    OSStrCbFormatW( szCorruption, sizeof( szCorruption ), L"isec %d reason %ws", lgposCurrent.isec, g_rgwszLogCorruptReason[ reason < eLCMax ? reason : 0 ] );

    //  this should only happen to edb.jtx/log...
    //  ... this can happen when doing bit-flip testing on logs
    Assert( FNegTest( fCorruptingLogFiles ) );

    // SOFT_HARD: leave, at least for compatibility
    if ( m_pLog->FHardRestore() )
    {

        //  we are in hard-recovery mode

        LONG lgenLowRestore, lgenHighRestore;
        m_pLog->LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
        if ( lgposCurrent.lGeneration <= lgenHighRestore )
        {
            //  report log file corruption to the event log

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

        //  report log file corruption to the event log

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
        //  report log file corruption to the event log

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
        //  we are in the dump code

        // SOFT_HARD: related with the check for it above
        Assert( !m_pLog->FHardRestore() );
        Assert( fCreatedLogReader );

        OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"3aa8328c-8b3d-45a1-93c5-4ea6bea70ad4" );
        Call( ErrERRCheck_( JET_errLogFileCorrupt, __FILE__, lLine ) );
    }

HandleError:

    m_lgposLastRec.ib           = 0;
    m_lgposLastRec.isec         = lgposLastValidSegment.isec + 1;
    m_lgposLastRec.lGeneration  = lgposLastValidSegment.lGeneration;

    //  handle errors and exit the function

    //  cleanup the LogReader class

    if ( fCreatedLogReader )
    {
        //  terminate the log reader
        errT = ErrLReaderTerm();

        //  fire off the error trap if necessary

        if ( errT != JET_errSuccess )
        {
            errT = ErrERRCheck( errT );
        }

        //  return the error code if no other error code yet exists

        if ( err == JET_errSuccess )
        {
            err = errT;
        }
    }

    if ( err >= 0 && !fReadOnly && fJetLog )
    {
        // seeding the write log buffer so we will know where to begin logging
        // on undo/do
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
        //  check the name of the file
        Assert( NULL != m_pLogStream->LogName() );
        Assert( L'\0' != m_pLogStream->LogName()[ 0 ] );

        //  open the file again
        errT = m_pLogStream->ErrLGOpenFile( NULL, fTrue );

        //  return the error code if no other error code yet exists
        if ( err == JET_errSuccess )
        {
            err = errT;
        }
    }

    return err;
}

/*
 *  Read first record pointed by plgposFirst.
 *  Initialize m_pbNext and m_pbNextNext.
 *  The first redo record must be within the good portion
 *  of the log file.
 */

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

    //  m_lgposLastRec should be set

    Assert( m_lgposLastRec.isec >= m_pLogStream->CSecHeader() &&
            ( m_lgposLastRec.isec < ( m_pLogStream->CSecLGFile() - 1 ) ||
              ( m_lgposLastRec.isec == ( m_pLogStream->CSecLGFile() - 1 ) && m_lgposLastRec.ib == 0 ) ) &&
            m_lgposLastRec.lGeneration == m_pLogStream->GetCurrentFileGen() );

    *ppbLR = pbNil;

    // make sure we've got the right log file loaded up
    Call( ErrReaderEnsureLogFile() );

    // Make sure that we do not ignore a sector read failure (will not overwrite
    // with shadow sector anymore)
    m_fIgnoreNextSectorReadFailure = fFalse;

    const USHORT csecRedo = min( max( ple_lgposRedo->le_isec, (USHORT)m_pLogStream->CSecHeader() ), m_lgposLastRec.isec );

    // read in start of log file
    Call( ErrReaderEnsureSector( csecRedo, 1, &pbSegment ) );

    m_pbNext = pbSegment;
    // Need to replay at the exact point that checkpoint says (attach-info
    // may be inaccurate for anywhere else)
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
        // empty log file, consumers expect at least one (LRCHECKSUM) record,
        // give them a NOP instead
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

//  Implicit input parameters
//      m_pbNext
//  Implicit output parameters
//      m_pbNext
//      m_pbNextNext

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

    //  m_lgposLastRec should be set

    Assert( m_lgposLastRec.isec >= m_pLogStream->CSecHeader() &&
            ( m_lgposLastRec.isec < ( m_pLogStream->CSecLGFile() - 1 ) ||
              ( m_lgposLastRec.isec == ( m_pLogStream->CSecLGFile() - 1 ) && m_lgposLastRec.ib == 0 ) ) &&
            m_lgposLastRec.lGeneration == m_pLogStream->GetCurrentFileGen() );

    if ( m_pbNextNext == pNil )
    {
        Call( errLGNoMoreRecords );
    }

    //  get the log position of the next log record (we will be replaying this next if its ok to replay it)

    GetLgposOfPbNextNext( &lgposPbNextNew );
    if ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) >= 0 )
    {
        Error( ErrERRCheck( errLGNoMoreRecords ) );
    }

    //  m_pbNext points to the log record we just replayed
    //  get that log position

    Assert( m_pbNext != pbNil );
    GetLgposOfPbNext( &lgposPbNext );

    //  make sure we don't exceed the end of the replay data

    //  since we already replayed the data at m_pbNext, we should be able to assert that m_pbNext is not
    //      past the end of usable data (if it is, we replayed unusable data -- e.g. GARBAGE)
    //
    //  this is a serious problem -- we replayed data that was not trusted
    //  the database may be really messed up at this point (I'm surprised the REDO code actually worked)
    //
    //  there is no way to get out of this; we'll have to trust UNDO to try and rectify things
    //      (unless the garbage we replayed happened to be a commit to level 0)
    //

    //  We somehow managed to replay garbage, and now, when we are moving past the garbage we just played, we are detecting it for the first time!?! Woops...

    EnforceSz( ( CmpLgpos( &lgposPbNext, &m_lgposLastRec ) < 0 ), "LogicalCorruptLogPbNextInconsistent" );

    //  m_pbNext should never suffer from wrap-around (it will always be in the first mapping of the buffers)

    Assert( m_pbNextNext >= m_pbLGBufMin && m_pbNextNext < m_pbLGBufMax );
    if ( m_pbNextNext < m_pbLGBufMin || m_pbNextNext >= m_pbLGBufMax )
    {
        Error( ErrERRCheck( JET_errLogFileCorrupt ) );
    }

    //  the new record should be at a greater log position than the old record
    //  it can be equal on the first iteration

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


//  Implicit input parameters
//      m_pbNext
//  Implicit output parameters
//      m_pbNext
//      m_pbNextNext

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

            //
            // Try not to recover much faster than the log generation time, as this can cause IO spikes due to
            // recovery
            //
            BOOL fContainsDataFromFutureLogs = fFalse;
            // No locking needed since this is recovery thread
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
                 !FLogtimeIsNull( &pSegHdr->logtimeSegmentStart ) ) // when agressively rolling over log before it is full, last empty segment does not have timestamp
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
                    // If we are replaying more than twice as fast as the generation rate, slow it down. 2x still allows
                    // recovery some room to catch up.
                    if ( dtickSegmentReplay < dtickSegmentGeneration / 2 )
                    {
                        // Sleep for upto 15.6ms (which is normally the minimum quanta for sleep.
                        // For a 1MB log file with 254 segments, that means a max delay of ~4secs.
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
                // beginning of fragment
                if ( pbCurrent + sizeof( LRFRAGMENTBEGIN ) > pbSegment + m_pLogStream->CbSec() )
                {
                    Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                }

                //
                // See details about UndoInfo record below.
                // Also, the UndoInfo record can be fragmented, if larger
                // than log segment size.
                //
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
                // continuation of fragment
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
                    // complete fragment, verify assembled record
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
                // skip nops
                while ( pbCurrent < pbSegment + m_pLogStream->CbSec() &&
                        *pbCurrent == lrtypNOP )
                {
                    // m_pLog->IncNOP();
                    pbCurrent++;
                }
                // nop should really go until end of segment
                Expected( pbCurrent == pbSegment + m_pLogStream->CbSec() );
                break;

            default:
                // regular record
                if ( fInFragment )
                {
                    //
                    // partial fragment would normally signify error except
                    // if occurring at the end of a segment and followed by
                    // an undo record at the very beginning of the next segment
                    // (that just means that we have a "torn" write from a
                    // dirty shutdown)
                    //
                    // There is a small window between the instance being marked
                    // in fRecoveringUndo state and the lrtypRecoveryUndo2
                    // record being written, BF could come in and write a
                    // lrtypUndoInfo record in that window, so that the
                    // lrtypRecoveryUndo2 record may not be the first record in
                    // undo phase (and hence not the first record after a torn
                    // write from a dirty shutdown).
                    //
                    // Redo to undo transition should probably be better
                    // synchronized with BF
                    //
                    if ( pbCurrent == pbSegment + sizeof( LGSEGHDR ) &&
                         ( *pbCurrent == lrtypRecoveryUndo ||
                           *pbCurrent == lrtypRecoveryUndo2 ||
                    //
                           *pbCurrent == lrtypUndoInfo ) )
                    {
                        // update m_pbNext so lgpos of record matches what it
                        // would have at logging time
                        m_pbNext = pbSegment;
                    }
                    else
                    {
                        Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                    }
                }
                if ( *pbCurrent >= lrtypMax )
                {
                    //  Did you maybe do something silly like add a non-pre-Ignored LR?
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
    //  did we get corruption?

    if ( err == JET_errLogFileCorrupt )
    {
        Assert( FNegTest( fCorruptingLogFiles ) );

        //  what recovery mode are we in?

        // SOFT_HARD: leave, the instance one should be good enough
        if ( m_pLog->FHardRestore() )
        {

            //  we are in hard-recovery mode

            LONG lgenLowRestore, lgenHighRestore;
            m_pLog->LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
            if ( m_pLogStream->GetCurrentFileGen() <= lgenHighRestore )
            {

                //  this generation is part of a backup set
                //  we must fail

                Assert( m_pLogStream->GetCurrentFileGen() >= lgenLowRestore );
                err = ErrERRCheck( JET_errLogCorruptDuringHardRestore );
            }
            else
            {

                //  the current log generation is not part of the backup-set
                //  we can patch the log and continue safely

                err = ErrERRCheck( JET_errLogCorruptDuringHardRecovery );
            }
        }
        else
        {

            //  we are in soft-recovery mode -- keep the original error JET_errLogFileCorrupt

        }
    }

    return err;
}


#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY

//  ================================================================
VOID LOG::LGIPrereadPage(
    const DBID      dbid,
    const PGNO      pgno,
    const OBJID     objid,
    BOOL *          pfPrereadIssued,
    BOOL * const        pfPrereadFailure,
    const BFPreReadFlags    bfprf )
//  ================================================================
{
    Assert( NULL != pfPrereadIssued );
    Assert( NULL != pfPrereadFailure );

    // If this page should not be preread, do not do it even if another LR asks for it to be preread
    //
    if ( m_plprereadSuppress->FLGPDBEnabled( dbid ) &&
         m_plprereadSuppress->IpgLGPGetSorted( dbid, pgno ) != CArray<PageRef>::iEntryNotFound )
    {
        return;
    }

    if ( ( pgnoNull != pgno ) && ( m_pPrereadWatermarks->CElements() < (UINT)UlParam( m_pinst, JET_paramPrereadIOMax ) ) )
    {
        const IFMP ifmp = m_pinst->m_mpdbidifmp[dbid];
        if ( ifmp < g_ifmpMax
            && FIODatabaseOpen( ifmp ) )
        {
            TraceContextScope tcScope( iortRecoveryRedo );
            tcScope->SetDwEngineObjid( objid );

            CPG cpgPreread = 0;
            const ERR err = m_plpreread->ErrLGPPrereadExtendedPageRange( dbid, pgno, &cpgPreread, bfprf );

            if ( ( err < JET_errSuccess ) && ( err != errBFPageCached ) && ( err != JET_errFileIOBeyondEOF ) )
            {
                *pfPrereadFailure = fTrue;
            }

            tcScope->SetDwEngineObjid( dwEngineObjidNone );

            //  This number is valid even if an error is returned because
            //  we may have preread some pages before we hit the failure.
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
                    //  Technically, the read is not completed yet, but the inaccuracy in the perf. counter will be irrelevant. 
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
#endif //  MINIMAL_FUNCTIONALITY

VOID
LOG::LGPrereadExecute(
    LGPOS lgposPbNext
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    ,LGPOS lgposLastChecksum
#endif
    )
{
#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY

    m_lgposPbNextPreread = lgposPbNext;
    // to prevent prereading from skipping the first log-record
    m_lgposPbNextNextPreread = lgposPbNext;
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    m_lgposLastChecksumPreread = lgposLastChecksum;
#endif

    // Do not preread if we are dumping the log
    if ( !m_fDumpingLogs )
    {
        // Preread is enabled for this log file.
        m_fPreread = fTrue;

        // Empty/free watermark queue.
        Assert( m_pPrereadWatermarks != NULL );
        while ( !m_pPrereadWatermarks->FEmpty() )
        {
            delete m_pPrereadWatermarks->RemovePrevMost( OffsetOf( LGPOSQueueNode, m_plgposNext ) );
        }
        
        //  Let's setup the log prereader. First, figure out which databases are attached.

        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            if ( FLGRICheckRedoConditionForDb( dbid, lgposPbNext ) )
            {
                //  Enabling an already-enabled database will wipe out the
                //  current list.
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

        //  Now, read the entire log to collect page references.

        ERR err = ErrLGIPrereadPages( fTrue );
        AssertSz( m_pPrereadWatermarks->FEmpty(), "Only collecting pgnos, not supposed to accumulate watermarks." );

        if ( err != JET_errSuccess )
        {
            LGPrereadTerm();
        }
        else
        {
            //  Finally, sort the pages in preparation for prereading.

            m_plpreread->LGPSortPages();
            m_plprereadSuppress->LGPSortPages();
            err = ErrLGIPrereadPages( fFalse );
        }
    }
#endif  //  MINIMAL_FUNCTIONALITY
}

//  Look forward in log records and ask buffer manager to preread pages.
//  fPgnosOnly indicates that we want to collect page number references only (first
//  phase of the pre-reading mechanism), as opoposed to actually issuing pre-reads (second
//  phase).

ERR LOG::ErrLGIPrereadExecute( const BOOL fPgnosOnly )
{
    ERR         err                     = JET_errSuccess;
    TraceContextScope tcScope( iortRecoveryRedo );

#ifdef MINIMAL_FUNCTIONALITY
#else //  !MINIMAL_FUNCTIONALITY
    const LR *  plr                     = pNil;
    BOOL        fPrereadFailure         = fFalse;
    BOOL        rgfPrereadIssued[dbidMax];
    const UINT  cRangesToPrereadMax = (UINT)UlParam( m_pinst, JET_paramPrereadIOMax );

    //  If we are only parsing/collecting page numbers, we want to go through the entire log file.
    //  On the other hand, we want to short-circuit in the case where pre-reading is disabled (i.e.
    //  JET_paramPrereadIOMax is zero).
    BOOL fBypassCpgCountCheck   = fPgnosOnly && cRangesToPrereadMax > 0;

    memset( rgfPrereadIssued, 0, sizeof(rgfPrereadIssued) );

    // Notice that we remove consecutive duplicates (and pgnoNull) from our preread
    // list, but that we count duplicates and null's to keep our bookkeeping simple.

    while ( ( m_pPrereadWatermarks->CElements() < cRangesToPrereadMax || fBypassCpgCountCheck ) &&
            !fPrereadFailure &&
            // Do not advance to next record if we have decided to exit the loop, otherwise we will skip that record
            // on the next iteration
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
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrpage->le_pgno, plr ) < JET_errSuccess )
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
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrsplit->le_pgno, plr ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrsplit->le_pgnoParent, plr ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrsplit->le_pgnoRight, plr ) != JET_errSuccess )
                    {
                        m_plpreread->LGPDBDisable( dbid );
                    }
                    if ( dbid < dbidMax &&
                         m_pinst->m_mpdbidifmp[ dbid ] < g_ifmpMax &&
                         g_rgfmp[ m_pinst->m_mpdbidifmp[ dbid ] ].FContainsDataFromFutureLogs() )
                    {
                        if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrsplit->le_pgnoNew, plr ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                        }
                    }
                    else
                    {
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrsplit->le_pgnoNew, plr );
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
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrmerge->le_pgno, plr ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrmerge->le_pgnoRight, plr ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrmerge->le_pgnoLeft, plr ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrmerge->le_pgnoParent, plr ) != JET_errSuccess )
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
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrpagemove->PgnoSource(), plr ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrpagemove->PgnoParent(), plr ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrpagemove->PgnoLeft(), plr ) != JET_errSuccess ||
                            m_plpreread->ErrLGPAddPgnoRef( dbid, plrpagemove->PgnoRight(), plr ) != JET_errSuccess )
                    {
                        m_plpreread->LGPDBDisable( dbid );
                    }
                    if ( dbid < dbidMax &&
                         m_pinst->m_mpdbidifmp[ dbid ] < g_ifmpMax &&
                         g_rgfmp[ m_pinst->m_mpdbidifmp[ dbid ] ].FContainsDataFromFutureLogs() )
                    {
                        if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrpagemove->PgnoDest(), plr ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                        }
                    }
                    else
                    {
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrpagemove->PgnoDest(), plr );
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
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrconvfdp->le_pgno, plr ) != JET_errSuccess )
                    {
                        m_plpreread->LGPDBDisable( dbid );
                    }

                    if ( dbid < dbidMax &&
                         m_pinst->m_mpdbidifmp[ dbid ] < g_ifmpMax &&
                         g_rgfmp[ m_pinst->m_mpdbidifmp[ dbid ] ].FContainsDataFromFutureLogs() )
                    {
                        if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrconvfdp->le_pgnoOE, plr ) != JET_errSuccess ||
                             m_plpreread->ErrLGPAddPgnoRef( dbid, plrconvfdp->le_pgnoAE, plr ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                        }
                    }
                    else
                    {
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrconvfdp->le_pgnoOE, plr );
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrconvfdp->le_pgnoAE, plr );
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
                        if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrcreatesefdp->le_pgno, plr ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                        }
                    }
                    else
                    {
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrcreatesefdp->le_pgno, plr );
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
                        if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrcreatemefdp->le_pgnoFDPParent, plr ) != JET_errSuccess ||
                             m_plpreread->ErrLGPAddPgnoRef( dbid, plrcreatemefdp->le_pgnoOE, plr ) != JET_errSuccess ||
                             m_plpreread->ErrLGPAddPgnoRef( dbid, plrcreatemefdp->le_pgnoAE, plr ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                        }
                    }
                    else
                    {
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrcreatemefdp->le_pgnoFDPParent, plr );
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrcreatemefdp->le_pgnoOE, plr );
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrcreatemefdp->le_pgnoAE, plr );
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

                // Always add to preread-list even if dbscan is not enabled as it can get enabled in
                // the middle of the log and we assert if we try to preread something we did not
                // first add to the list.
                if ( fPgnosOnly )
                {
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, pgno, plr ) != JET_errSuccess )
                    {
                        m_plpreread->LGPDBDisable( dbid );
                    }
                }
                else if ( FLGRICheckRedoScanCheck( &lrscancheck, fTrue /* fEvaluatePrereadLogic */ ) )
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
                    if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrpatch->Pgno(), plr ) < JET_errSuccess )
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
                        if ( m_plpreread->ErrLGPAddPgnoRef( dbid, plrnewpage->Pgno(), plr ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                        }
                    }
                    else
                    {
                        (void)m_plprereadSuppress->ErrLGPAddPgnoRef( dbid, plrnewpage->Pgno(), plr );
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
                // This is not logged for all free extent operations, only for those related to deleting a whole space tree.
                LREXTENTFREED lrextentfreed;
                lrextentfreed.InitExtentFreed( plr );
                const DBID dbid         = lrextentfreed.Dbid();
                const PGNO pgnofirst    = lrextentfreed.PgnoFirst();
                const CPG  cpgextent    = lrextentfreed.CpgExtent();

                // Only preread if RBS is enabled. 
                // We only enable during attach/create phase, so we don't need to worry about it getting enabled in between.
                if( m_pinst->m_mpdbidifmp[ dbid ] >= g_ifmpMax || 
                    !( g_rgfmp[ m_pinst->m_mpdbidifmp[ dbid ] ].FRBSOn() ) )
                {
                    break;
                }

                Assert( dbid > dbidTemp );
                Assert( dbid < dbidMax );

                if ( fPgnosOnly )
                {
                    // Add all pages in the extent freed to be preread 
                    for( INT i = 0; i < cpgextent; ++i )
                    {
                        if( m_plpreread->ErrLGPAddPgnoRef( dbid, pgnofirst + i, plr ) != JET_errSuccess )
                        {
                            m_plpreread->LGPDBDisable( dbid );
                            break;
                        }
                    }
                }
                else
                {
                    // Preread all pages in the extent. 
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

            //  When we hit a DetachDb or Term/RcvQuit in the logfile,
            //  we should technically stop prereading for the duration
            //  of the logfile  because there's no way to tell if future
            //  occurrences of the dbid in the logfile map to the same database.
            //  However, there's probably not much harm in prereading
            //  the wrong pages.  They'll eventually just get
            //  evicted.

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

        //  reached end of logfile, so disable preread for
        //  the rest of this logfile
        //
        m_fPreread = fFalse;
    }

#endif  //  MINIMAL_FUNCTIONALITY
    return err;
}

//  Setup state to look forward in log for page references
//  fPgnosOnly indicates that are going through the first phase of the pre-reading mechanism, which
//  only collects page references for further optimizations.

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

    // ================ Save normal log reading state ===================== //
    // These two will be used to retore m_pbNext and m_pbLastChecksum later
    m_pLogReadBuffer->GetLgposOfPbNext( &lgposPbNextSaved );
    m_pLogReadBuffer->GetLgposOfPbNextNext( &lgposPbNextNextSaved );
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    m_pLogReadBuffer->GetLgposOfLastChecksum( &lgposLastChecksumSaved );
#endif

    // ================ Restore to prereading state ======================= //
    // Return the LogReader to the state of the last preread. This will
    // ensure that the last CK that we were prereading in is available,
    // and that ErrLGIGetNextChecksumFF will be able to deal with it
    // sensibly.
    // state includes isec and csec from last call to ErrEnsureSector

    err = m_pLogReadBuffer->ErrReaderRestoreState(
            &m_lgposPbNextPreread,
            &m_lgposPbNextNextPreread
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
            ,&m_lgposLastChecksumPreread
#endif
            );

    if ( JET_errSuccess != err )
    {
        // non-fatal
        err = JET_errSuccess;
        m_fPreread = fFalse;
        LGPrereadTerm();
        goto LRestoreNormal;
    }

    // =========== Do actual prereading ================================== //

    err = ErrLGIPrereadExecute( fPgnosOnly );
    if ( JET_errSuccess != err )
    {
        // If any error occurs (including no more log records)
        // don't ever again try to preread this file again.
        // This is re-enabled in FirstRedoLogRec which will
        // open up the next log file. The exception is when we
        // are parsing the log for the first time to collect
        // the list of pages.
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

    // =========== Save prereading state ================================= //

    // we don't need to save the state if we are parsing the log for the first time.
    
    if ( !fPgnosOnly )
    {
        m_pLogReadBuffer->GetLgposOfPbNext( &m_lgposPbNextPreread );
        m_pLogReadBuffer->GetLgposOfPbNextNext( &m_lgposPbNextNextPreread );
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
        m_pLogReadBuffer->GetLgposOfLastChecksum( &m_lgposLastChecksumPreread );
#endif
    }

    // =========== Restore to normal log reading state =================== //
LRestoreNormal:
    // Need to restore LogReader to state on entry, so
    // that normal GetNextRec code will find the records it
    // expects. This is fatal if we can't restore the state to what
    // normal reading expects.
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




//+------------------------------------------------------------------------
//
//  CbLGSizeOfRec
//  =======================================================================
//
//  ERR CbLGSizeOfRec( plgrec )
//
//  Returns the length of a log record.
//
//  PARAMETER   plgrec  pointer to log record
//
//  RETURNS     size of log record in bytes
//
//-------------------------------------------------------------------------
typedef struct {
    SHORT   cb:15;
    SHORT   fDebugOnly:1;
} LRD;      /* log record descriptor */


const LRD mplrtyplrd[ ] = {
    {   /*  0   NOP      */         sizeof( LRTYP ),                0   },
    {   /*  1   Init     */         sizeof( LRINIT ),               0   },
    {   /*  2   Term     */         sizeof( LRTERMREC ),            0   },
    {   /*  3   MS       */         sizeof( LRMS ),                 0   },
    {   /*  4   Fill     */         sizeof( LRTYP ),                0   },

    {   /*  5   Begin    */         sizeof( LRBEGIN ),              0   },
    {   /*  6   Commit   */         sizeof( LRCOMMIT ),             0   },
    {   /*  7   Rollback */         sizeof( LRROLLBACK ),           0   },
    {   /*  8   Begin0   */         sizeof( LRBEGIN0 ),             0   },
    {   /*  9   Commit0  */         sizeof( LRCOMMIT0 ),            0   },
    {   /*  10  Refresh  */         sizeof( LRREFRESH ),            0   },
    {   /*  11  McrBegin */         sizeof( LRMACROBEGIN ),         0   },
    {   /*  12  McrCmmt  */         sizeof( LRMACROEND ),           0   },
    {   /*  13  McrAbort */         sizeof( LRMACROEND ),           0   },

    {   /*  14  CreateDB */         0,                              0   },
    {   /*  15  AttachDB */         0,                              0   },
    {   /*  16  DetachDB */         0,                              0   },

    {   /*  17  RcvrUndo */         sizeof( LRRECOVERYUNDO ),       0   },
    {   /*  18  RcvrQuit */         sizeof( LRTERMREC ),            0   },
    {   /*  19  FullBkUp */         0,                              0   },
    {   /*  20  IncBkUp  */         0,                              0   },

    {   /*  21  JetOp    */         sizeof( LRJETOP ),              1   },
    {   /*  22  Trace    */         0,                              1   },

    {   /*  23  ShutDown */         sizeof( LRSHUTDOWNMARK ),       0   },

    {   /*  24  Create ME FDP  */   sizeof( LRCREATEMEFDP ),        0   },
    {   /*  25  Create SE FDP  */   sizeof( LRCREATESEFDP ),        0   },
    {   /*  26  Convert FDP    */   sizeof( LRCONVERTFDP_OBSOLETE ),0   },

    {   /*  27  Split    */         0,                              0   },
    {   /*  28  Merge    */         0,                              0   },

    {   /*  29  Insert   */         0,                              0   },
    {   /*  30  FlagInsert */       0,                              0   },
{   /*  31  FlagInsertAndReplaceData */
                                    0,                              0   },

    {   /*  32  FDelete  */         sizeof( LRFLAGDELETE ),         0   },
    {   /*  33  Replace  */         0,                              0   },
    {   /*  34  ReplaceD */         0,                              0   },

    {   /*  35  Delete  */          sizeof( LRDELETE ),             0   },
    {   /*  36  UndoInfo */         0,                              0   },

    {   /*  37  Delta    */         0,                              0   },

    {   /*  38  SetExtHeader */     0,                              0   },
    {   /*  39  Undo     */         0,                              0   },

    {   /*  40  SLVPageAppend */    0,                              0   },
    {   /*  41  SLVSpace */         0,                              0   },
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    {   /*  42  Checksum */         sizeof( LRCHECKSUM ),           0   },
#else
    {   /*  42  Checksum (obsolete) */  0,                          0   },
#endif
    {   /*  43  SLVPageMove (obsolete) */       0,                  0   },
    {   /*  44  ForceDetachDB (obsolete) */ 0,                      0   },
    {   /*  45  ExtRestore */       0,                              0   },
    {   /*  46  Backup (obsolete) */ 0,                             0   },
    {   /*  47  UpgradeDB */        0,                              0   },
    {   /*  48  EmptyTree */        0,                              0   },
    {   /*  49  Init2 */            sizeof( LRINIT2 ),              0   },
    {   /*  50  Term2 */            sizeof( LRTERMREC2 ),           0   },
    {   /*  51  RecoveryUndo */     sizeof( LRRECOVERYUNDO2 ),      0   },
    {   /*  52  RecoveryQuit2 */    sizeof( LRTERMREC2 ),           0   },
    {   /*  53  BeginDT */          sizeof( LRBEGINDT ),            0   },
    {   /*  54  PrepCommit */       0,                              0   },
    {   /*  55  PrepRollback */     0,                              0   },
    {   /*  56  DbList */           0,                              0   },
    {   /*  57  ForceWriteLog */    sizeof( LRFORCEWRITELOG),       0   },
    {   /*  58  Convert FDP    */   sizeof( LRCONVERTFDP ),         0   },
    {   /*  59  ExtRestore    */    0,                              0   },
    {   /*  60  Backup */           0,                              0   },
    {   /*  61  ForceLogRollover */ 0,                              0   },
    {   /*  62  Split2 */           0,                              0   },
    {   /*  63  Merge2 */           0,                              0   },
    {   /*  64  Scrub */            0,                              0   },
    {   /*  65  PageMove */         0,                              0   },
    {   /*  66  PagePatchRequest */ sizeof( LRPAGEPATCHREQUEST),    0   },
    {   /*  67  MacroInfo */        0,                              0   },  // originally lrtypIgnored1
    {   /*  68  DBExtension */      0,                              0   },  // originally lrtypIgnored2
    {   /*  69  CommitCtx */        0,                              0   },  // originally lrtypIgnored3
    {   /*  70  ScanCheck */        0,                              0   },  // originally lrtypIgnored4
    {   /*  71  NOP2 */             0,                              0   },  // originally lrtypIgnored5
    {   /*  72  ReAttach */         0,                              0   },  // originally lrtypIgnored6
    {   /*  73  MacroInfo2 */       0,                              0   },  // originally lrtypIgnored7
    {   /*  74  FreeFDP */          0,                              0   },  // originally lrtypIgnored8
    {   /*  75  Ignored9 */         0,                              0   },
    {   /*  76  FragmentBegin */    0,                              0   },
    {   /*  77  Fragment */         0,                              0   },
    {   /*  78  ShrinkDB */         sizeof( LRSHRINKDB ),           0   },
    {   /*  79  CreateDBFinish */   sizeof( LRCREATEDBFINISH ),     0   },
    {   /*  80  RecoveryQuitKeepAttachments */  sizeof( LRTERMREC2 ), 0 },
    {   /*  81  DBTrim */           sizeof( LRTRIMDB ),             0   },
    {   /*  82  Ignored10 */        0,                              0   },
    {   /*  83  Ignored11 */        0,                              0   },
    {   /*  84  Ignored12 */        0,                              0   },
    {   /*  85  Ignored13 */        0,                              0   },
    {   /*  86  Ignored14 */        0,                              0   },
    {   /*  87  Ignored15 */        0,                              0   },
    {   /*  88  Ignored16 */        0,                              0   },
    {   /*  89  Ignored17 */        0,                              0   },
    {   /*  90  Ignored18 */        0,                              0   },
    {   /*  91  Ignored19 */        0,                              0   },
    {   /*  92  SetDbVersion */     sizeof( LRSETDBVERSION ),       0   },
    {   /*  93  Delta64  */         0,                              0   },
    {   /*  94  ShrinkDB2 */        sizeof( LRSHRINKDB ),           0   },
    {   /*  95  NewPage */          sizeof( LRNEWPAGE ),            0   },
    {   /*  96  PageMoveR */        sizeof( LRROOTPAGEMOVE ),       0   },
    {   /*  97  SigAttachDB */      sizeof( LRSIGNALATTACHDB ),     0   },
    {   /*  98  ScanCheck2 */       sizeof( LRSCANCHECK2 ),         0   },
    {   /*  99  ShrinkDB3 */        sizeof( LRSHRINKDB3 ),          0   },
    {   /*  100 ExtentFreed */      sizeof( LREXTENTFREED ),        0   },
};


#ifdef DEBUG
BOOL FLGDebugLogRec( LR *plr )
{
    return mplrtyplrd[plr->lrtyp].fDebugOnly;
}
#endif


//  For CbLGSizeOfRec() to work properly with FASTFLUSH, CbLGSizeOfRec()
//  must only use the fixed fields of log records to calculate their size.

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
                + ( plrcreatedb->FVersionInfo() ? sizeof(LRCREATEDB::VersionInfo) : 0 ) );  //  older format didn't have version info
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

    case lrtypMacroInfo:    // originally lrtypIgnored1
    case lrtypExtendDB:     // originally lrtypIgnored2
    case lrtypCommitCtx:    // originally lrtypIgnored3
    case lrtypScanCheck:    // originally lrtypIgnored4
    case lrtypNOP2:         // originally lrtypIgnored5
    case lrtypReAttach:     // originally lrtypIgnored6
    case lrtypMacroInfo2:   // originally lrtypIgnored7
    case lrtypFreeFDP:      // originally lrtypIgnored8
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

// mplrtypn
//
// Maps an lrtyp to the size of the fixed structure.
//

// Notice that we do not specify a size for this array. This is so we can
// catch developers that add a new lrtyp, but forget to modify this array
// by Assert( ( sizeof( mplrtypn ) / sizeof( mplrtypn[ 0 ] ) ) == lrtypMax )

const SHORT mplrtypn[ ] =
{
    sizeof( LRNOP ),
    sizeof( LRINIT ),
    sizeof( LRTERMREC ),
    sizeof( LRMS ),
    sizeof( LRTYP ),    // lrtypEnd
    sizeof( LRBEGIN ),
    sizeof( LRCOMMIT ),
    sizeof( LRROLLBACK ),
    sizeof( LRBEGIN0 ),
    sizeof( LRCOMMIT0 ),
    sizeof( LRREFRESH ),
    sizeof( LRMACROBEGIN ),
    sizeof( LRMACROEND ),
    sizeof( LRMACROEND ),   // lrtypMacroAbort
    sizeof( LRCREATEDB ),
    sizeof( LRATTACHDB ),
    sizeof( LRDETACHDB ),
    sizeof( LRRECOVERYUNDO ),
    sizeof( LRTERMREC ),    // lrtypRecoveryQuit
    sizeof( LRLOGRESTORE ), // lrtypFullBackup
    sizeof( LRLOGRESTORE ), // lrtypIncBackup
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
    sizeof( LRREPLACE ),    // lrtypReplaceD
    sizeof( LRDELETE ),
    sizeof( LRUNDOINFO ),
    sizeof( LRDELTA32 ),
    sizeof( LRSETEXTERNALHEADER ),
    sizeof( LRUNDO ),
    0, // was LRSLVPAGEAPPEND
    0, // was LRSLVSPACE
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    sizeof( LRCHECKSUM ),
#else
    0,
#endif
    0, // was LRSLVPAGEMOVE
    0,  // was LRFORCEDETACHDB
    sizeof( LREXTRESTORE ),
    sizeof( LRLOGBACKUP_OBSOLETE ),
    0,  // was UpgradeDB ...
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
    sizeof( LRLOGBACKUP ),  // lrtypBackup2
    sizeof( LRFORCELOGROLLOVER ),
    sizeof( LRSPLITNEW ),
    sizeof( LRMERGENEW ),
    sizeof( LRSCRUB ),
    sizeof( LRPAGEMOVE ),
    sizeof( LRPAGEPATCHREQUEST ),
    sizeof( LRIGNORED ),    // lrtypMacroInfo
    sizeof( LRIGNORED ),    // lrtypExtendDB
    sizeof( LRIGNORED ),    // lrtypCommitCtx
    sizeof( LRIGNORED ),    // lrtypScanCheck
    sizeof( LRIGNORED ),    // lrtypNOP2
    sizeof( LRIGNORED ),    // lrtypReAttach
    sizeof( LRIGNORED ),    // lrtypMacroInfo2
    sizeof( LRIGNORED ),    // lrtypFreeFDP
    sizeof( LRIGNORED ),    // lrtypIgnored9
    sizeof( LRFRAGMENTBEGIN ),
    sizeof( LRFRAGMENT ),
    sizeof( LRSHRINKDB ),
    sizeof( LRCREATEDBFINISH ),
    sizeof( LRTERMREC2 ),   // lrtypRecoveryQuitKeepAttachments
    sizeof( LRTRIMDB ),
    sizeof( LRIGNORED ),    // lrtypIgnored10
    sizeof( LRIGNORED ),    // lrtypIgnored11
    sizeof( LRIGNORED ),    // lrtypIgnored12
    sizeof( LRIGNORED ),    // lrtypIgnored13
    sizeof( LRIGNORED ),    // lrtypIgnored14
    sizeof( LRIGNORED ),    // lrtypIgnored15
    sizeof( LRIGNORED ),    // lrtypIgnored16
    sizeof( LRIGNORED ),    // lrtypIgnored17
    sizeof( LRIGNORED ),    // lrtypIgnored18
    sizeof( LRIGNORED ),    // lrtypIgnored19
    sizeof( LRSETDBVERSION ),   // lrtypSetDbVersion
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
    // Should only be passed valid lrtyp's.
    // If this fires, be sure to check the mlrtypn array above and lrtypMax
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
    // If this fires, someone added a new lrtyp (hence, increased lrtypMax),
    // but they didn't modify mplrtypn[] above, or they didn't set lrtypMax properly.
    C_ASSERT( _countof( mplrtypn ) == lrtypMax );
    // If this fires, someone added a new lrtyp (hence, increased lrtypMax),
    // but they didn't modify mplrtyplrd[] far above, or they didn't set lrtypMax properly.
    C_ASSERT( _countof( mplrtyplrd ) == lrtypMax );

    // Verify that fixed sizes in mplrtyplrd[] are the same as in mplrtypn[].
    for ( LRTYP iLrtyp = 0; iLrtyp < lrtypMax; iLrtyp++ )
    {
        if ( 0 != mplrtyplrd[ iLrtyp ].cb )
        {
            Assert( mplrtyplrd[ iLrtyp ].cb == mplrtypn[ iLrtyp ] );
        }
    }

}

#endif

//  Implementation of LogPrereader.

INLINE ERR LogPrereader::ErrLGPIPrereadPage( __in_range( dbidUserLeast, dbidMax - 1 ) const DBID dbid, const PGNO pgno, const BFPreReadFlags bfprf )
{
    Assert( FLGPDBEnabled( dbid ) );
    Assert( dbid >= dbidUserLeast && dbid < dbidMax );
    //  Redo is only using this for asking for Combinable, and no other real preread behavior (just tracing for 
    //  DBScan) ... perhaps this should be handled through TLS / TraceContexts so we don't pass bfprf all around
    //  making behavior changes hard to track down.
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


