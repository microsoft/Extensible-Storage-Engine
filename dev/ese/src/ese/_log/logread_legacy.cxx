// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "logstd.hxx"

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT

#if defined( DEBUG ) || !defined( RTM )



#define ENABLE_LOGPATCH_TRACE

#endif




#ifdef DEBUG
LOCAL BOOL g_fLRCKValidationTrap = fFalse;
#endif

BOOL LOG_READ_BUFFER::FValidLRCKRecord(
    const LRCHECKSUM * const plrck,
    const LGPOS * const plgpos
    )
{
    LGPOS   lgpos;
    QWORD   qwSize = 0;
    ULONG   cbRange = 0;
    ERR     err = JET_errSuccess;
    UINT    ib = 0;
    BOOL    fMultisector = fFalse;

    Assert( pNil != plrck );


    if ( lrtypChecksum != plrck->lrtyp )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }


    if ( plrck->le_cbBackwards > ( m_pLogStream->CbSec() - sizeof( LRCHECKSUM ) ) )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }


    if ( plrck->bUseShortChecksum == bShortChecksumOn )
    {

        if ( !FValidLRCKShortChecksum( plrck, plgpos->lGeneration ) )
        {
            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }


        fMultisector = fTrue;
    }
    else if ( plrck->bUseShortChecksum == bShortChecksumOff )
    {


        if ( plrck->le_ulShortChecksum != 0 )
        {
            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }


        fMultisector = fFalse;
    }
    else
    {


        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }


    const BYTE* const pbRegionStart = reinterpret_cast< const BYTE* >( plrck ) - plrck->le_cbBackwards;
    if ( m_pLogStream->PbSecAligned( const_cast< BYTE* >( pbRegionStart ), m_pbLGBufMin ) != pbRegionStart &&
        0 != plrck->le_cbBackwards )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }


    lgpos = *plgpos;
    m_pLogStream->AddLgpos( &lgpos, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );
    Call( m_pLogStream->ErrFileSize( &qwSize ) );
    Assert( ( qwSize % (QWORD)m_pLogStream->CbSec() ) == 0 );
    if ( ( ( (QWORD)lgpos.isec * (QWORD)m_pLogStream->CbSec() ) + (QWORD)lgpos.ib ) > qwSize )
    {
        AssertSz( !fMultisector, "We have a verified short checksum value and the forward pointer is still wrong!" );

        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }


    ib = ULONG( reinterpret_cast< const BYTE* >( plrck ) - m_pLogStream->PbSecAligned( reinterpret_cast< const BYTE* >( plrck ), m_pbLGBufMin ) );
    Assert( ib == plgpos->ib );


    if ( plrck->le_cbBackwards != 0 && plrck->le_cbBackwards != ib )
    {


        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }


    cbRange = plrck->le_cbBackwards + sizeof( LRCHECKSUM ) + plrck->le_cbForwards;
    if ( cbRange > m_pLogStream->CbSec() )
    {



        if ( !fMultisector )
        {
            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }


        if ( ( cbRange % m_pLogStream->CbSec() ) != 0 )
        {


            if ( plrck->le_cbBackwards != 0 )
            {


                Assert( !g_fLRCKValidationTrap );
                return fFalse;
            }
        }
    }



    cbRange = ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards;
    if ( cbRange > m_pLogStream->CbSec() )
    {



        if ( !fMultisector )
        {
            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }


        if ( ( cbRange % m_pLogStream->CbSec() ) != 0 )
        {


            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }
    }
    else
    {



        if ( fMultisector )
        {


            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }
    }



    if ( plrck->le_cbNext != 0 )
    {



        if ( cbRange < m_pLogStream->CbSec() || ( cbRange % m_pLogStream->CbSec() ) != 0 )
        {
            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }


        lgpos = *plgpos;
        m_pLogStream->AddLgpos( &lgpos, sizeof( LRCHECKSUM ) + plrck->le_cbNext );


        if ( lgpos.isec != plgpos->isec + ( cbRange / m_pLogStream->CbSec() ) )
        {


            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }


        
    }

    return fTrue;

HandleError:


    return fFalse;
}


BOOL LOG_READ_BUFFER::FValidLRCKShadow(
    const LRCHECKSUM * const plrck,
    const LGPOS * const plgpos,
    const LONG lGeneration
    )
{


    if ( !FValidLRCKRecord( plrck, plgpos ) )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }


    if ( plrck->bUseShortChecksum != bShortChecksumOff )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }


    if ( plrck->le_cbNext != 0 )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }


    if ( UlComputeShadowChecksum( UlComputeChecksum( plrck, (ULONG32)lGeneration ) ) != plrck->le_ulChecksum )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    return fTrue;
}



BOOL LOG_READ_BUFFER::FValidLRCKShadowWithoutCheckingCBNext(
    const LRCHECKSUM * const plrck,
    const LGPOS * const plgpos,
    const LONG lGeneration
    )
{


    if ( !FValidLRCKRecord( plrck, plgpos ) )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }


    if ( plrck->bUseShortChecksum != bShortChecksumOff )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }


    if ( UlComputeShadowChecksum( UlComputeChecksum( plrck, (ULONG32)lGeneration ) ) != plrck->le_ulChecksum )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    return fTrue;
}



BOOL LOG_READ_BUFFER::FValidLRCKShortChecksum(
    const LRCHECKSUM * const plrck,
    const LONG lGeneration
    )
{


    Assert( plrck->bUseShortChecksum == bShortChecksumOn );
    Assert( plrck->le_cbBackwards <= ( m_pLogStream->CbSec() - sizeof( LRCHECKSUM ) ) );


    if ( UlComputeShortChecksum( plrck, (ULONG32)lGeneration ) != plrck->le_ulShortChecksum )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    return fTrue;
}



BOOL LOG_READ_BUFFER::FValidLRCKRange(
    const LRCHECKSUM * const plrck,
    const LONG lGeneration
    )
{

    if ( UlComputeChecksum( plrck, (ULONG32)lGeneration ) != plrck->le_ulChecksum )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    return fTrue;

}



void LOG_READ_BUFFER::AssertValidLRCKRecord(
    const LRCHECKSUM * const plrck,
    const LGPOS * const plgpos )
{
#ifdef DEBUG
    const BOOL fValid = FValidLRCKRecord( plrck, plgpos );
    Assert( fValid );
#endif
}



void LOG_READ_BUFFER::AssertValidLRCKRange(
        const LRCHECKSUM * const plrck,
        const LONG lGeneration )
{
#ifdef DEBUG


    Assert( FValidLRCKRange( plrck, lGeneration ) );
#endif
}



void LOG_READ_BUFFER::AssertRTLValidLRCKRecord(
        const LRCHECKSUM * const plrck,
        const LGPOS * const plgpos )
{
    const BOOL fValid = FValidLRCKRecord( plrck, plgpos );
    AssertRTL( fValid );
}



void LOG_READ_BUFFER::AssertRTLValidLRCKRange(
        const LRCHECKSUM * const plrck,
        const LONG lGeneration )
{


    AssertRTL( FValidLRCKRange( plrck, lGeneration ) );
}



void LOG_READ_BUFFER::AssertRTLValidLRCKShadow(
        const LRCHECKSUM * const plrck,
        const LGPOS * const plgpos,
        const LONG lGeneration )
{
    const BOOL fValid = FValidLRCKShadow( plrck, plgpos, lGeneration );
    AssertRTL( fValid );
}



void LOG_READ_BUFFER::AssertInvalidLRCKRange(
        const LRCHECKSUM * const plrck,
        const LONG lGeneration )
{


    AssertRTL( !FValidLRCKRange( plrck, lGeneration ) );
}


#ifdef ENABLE_LOGPATCH_TRACE


INLINE BOOL FLGILogPatchDate( const WCHAR* wszPath, CPRINTFFILE **const ppcprintf )
{
    CPRINTFFILE *pcprintf = *ppcprintf;
    DATETIME    datetime;

    if ( !pcprintf )
    {


        pcprintf = new CPRINTFFILE( wszPath );
        if ( !pcprintf )
        {
            return fFalse;
        }
        *ppcprintf = pcprintf;


        (*pcprintf)( "====================================================================================\r\n" );
        (*pcprintf)( "ErrLGCheckReadLastLogRecordFF trace-log generated by Extensible Storage Engine (ESE)\r\n" );
        (*pcprintf)( "Do NOT delete this unless you know what you are doing...\r\n" );
        (*pcprintf)( "\r\n" );
    }

    UtilGetCurrentDateTime( &datetime );
    (*pcprintf)( "%02d:%02d:%02d %02d/%02d/%02d: ",
                datetime.hour,
                datetime.minute,
                datetime.second,
                datetime.month,
                datetime.day,
                datetime.year );

    return fTrue;
}

#endif


inline INT __cdecl _memcmpp( const void* pv, const void* pvPattern, size_t cbPattern, size_t cb )
{
    INT iCmp = 0;
    for ( size_t ib = 0; !iCmp && cbPattern && ib < cb; ib += cbPattern )
    {
        iCmp = memcmp( (char*)pv + ib, pvPattern, min( cbPattern, cb - ib ) );
    }
    return iCmp;
}


inline void* __cdecl _memcpyp( void* pv, const void* pvPattern, size_t cbPattern, size_t cb )
{
    for ( size_t ib = 0; cbPattern && ib < cb; ib += cbPattern )
    {
        memcpy( (char*)pv + ib, pvPattern, min( cbPattern, cb - ib ) );
    }
    return pv;
}


LOCAL VOID LGIReportChecksumMismatch(
    const INST *    pinst,
    PCWSTR          wszLogName,
    const QWORD     ibOffset,
    const ULONG     cbLength,
    const ULONG     ulChecksumExpected,
    const ULONG     ulChecksumActual,
    const ERR       err )
{
    const WCHAR *   rgpszT[ 6 ];
    DWORD           irgpsz      = 0;
    WCHAR           szOffset[ 64 ];
    WCHAR           szLength[ 64 ];
    WCHAR           szError[ 64 ];
    WCHAR           szChecksumExpected[ 64 ];
    WCHAR           szChecksumActual[ 64 ];

    OSStrCbFormatW( szOffset, sizeof(szOffset), L"%I64i (0x%016I64x)", ibOffset, ibOffset );
    OSStrCbFormatW( szLength, sizeof(szLength), L"%u (0x%08x)" , cbLength, cbLength );
    OSStrCbFormatW( szError, sizeof(szError), L"%i (0x%08x)", err, err );
    OSStrCbFormatW( szChecksumExpected, sizeof(szChecksumExpected), L"%u (0x%08x)", ulChecksumExpected, ulChecksumExpected );
    OSStrCbFormatW( szChecksumActual, sizeof(szChecksumActual), L"%u (0x%08x)", ulChecksumActual, ulChecksumActual );
    
    rgpszT[ irgpsz++ ] = wszLogName;
    rgpszT[ irgpsz++ ] = szOffset;
    rgpszT[ irgpsz++ ] = szLength;
    rgpszT[ irgpsz++ ] = szError;
    rgpszT[ irgpsz++ ] = szChecksumExpected;
    rgpszT[ irgpsz++ ] = szChecksumActual;
    
    bool fBadRec = 0 == ulChecksumExpected && 0 == ulChecksumActual;

    UtilReportEvent(
            eventError,
            LOGGING_RECOVERY_CATEGORY,
            fBadRec ? LOG_CHECKSUM_REC_CORRUPTION_ID : LOG_RANGE_CHECKSUM_MISMATCH_ID,
            irgpsz,
            rgpszT,
            0,
            NULL,
            pinst );

    OSUHAPublishEvent(
        HaDbFailureTagRecoveryRedoLogCorruption, pinst, HA_LOGGING_RECOVERY_CATEGORY,
        HaDbIoErrorNone, wszLogName, ibOffset, cbLength,
        fBadRec? HA_LOG_CHECKSUM_REC_CORRUPTION_ID : HA_LOG_RANGE_CHECKSUM_MISMATCH_ID, irgpsz, rgpszT );
}


ERR
LOG_VERIFY_STATE::ErrVerifyLRCKEnd()
{
    const ULONG ibEndOfForward = m_ibLRCK + sizeof( m_lrck ) + m_lrck.le_cbForwards;

    Assert( ibEndOfForward >= m_ibChecksumIncremental );
    if ( ibEndOfForward == m_ibChecksumIncremental )
    {
        if ( m_lrck.le_ulChecksum != m_ck.EndChecksum() )
        {
            return ErrERRCheck( JET_errLogReadVerifyFailure );
        }
        if ( 0 == m_lrck.le_cbNext )
        {
            m_state = LogVerifyDone;
        }
        else
        {
            m_ibLRCK += sizeof( LRCHECKSUM ) + m_lrck.le_cbNext;
            m_ibChecksumIncremental = m_ibLRCK + sizeof( LRCHECKSUM );
            m_state = LogVerifyLRCKStart;
        }
    }
    else
    {
        m_state = LogVerifyLRCKContinue;
    }
    return JET_errSuccess;
}

ERR
LOG_VERIFY_STATE::ErrVerifyLRCKContinue( __in_bcount(cb) BYTE *pb, DWORD cb )
{
    const ULONG ibEndOfForwards = m_ibLRCK + sizeof( m_lrck ) + m_lrck.le_cbForwards;
    Assert( ibEndOfForwards > m_ibChecksumIncremental );
    const ULONG cbMaxForwards = ibEndOfForwards - m_ibChecksumIncremental;
    const BYTE* const pbMax = pb + min( cb, cbMaxForwards );

    if ( ( pbMax < pb ) ||
        ( pbMax > pb + cb ) )
    {
        return ErrERRCheck( JET_errLogReadVerifyFailure );
    }
    m_ck.ChecksumBytes( pb, pbMax );
    m_ibChecksumIncremental += ULONG( pbMax - pb );

    return ErrVerifyLRCKEnd();
}


ERR
LOG_VERIFY_STATE::ErrVerifyLRCKStart( __in_bcount(cb) BYTE *pb, DWORD cb )
{
    ULONG32 ulChecksum = ulLRCKChecksumSeed;

    Assert( m_ibLRCK >= m_cbSeen );
    const LRCHECKSUM* const plrck = reinterpret_cast< const LRCHECKSUM* >( pb + ( m_ibLRCK - m_cbSeen ) );

    if ( ( reinterpret_cast< const BYTE* >( plrck ) < pb ) ||
         ( reinterpret_cast< const BYTE* >( plrck ) + sizeof( *plrck ) > cb + pb ) )
    {
        return ErrERRCheck( JET_errLogReadVerifyFailure );
    }

    if ( plrck->lrtyp != lrtypChecksum )
    {
        return ErrERRCheck( JET_errLogReadVerifyFailure );
    }

    m_lrck = *plrck;

    {
        const BYTE* const pbMin = reinterpret_cast< const BYTE* >( plrck ) - plrck->le_cbBackwards;
        if ( ( pbMin < pb ) ||
            ( pbMin >= cb + pb ) )
        {
            return ErrERRCheck( JET_errLogReadVerifyFailure );
        }
        const BYTE* const pbMax = reinterpret_cast< const BYTE* >( plrck );
        ulChecksum = LGChecksum::UlChecksumBytes( pbMin, pbMax, ulChecksum );
    }


    ulChecksum ^= plrck->le_cbBackwards;
    ulChecksum ^= plrck->le_cbForwards;
    ulChecksum ^= plrck->le_cbNext;

    ulChecksum ^= (ULONG32) plrck->bUseShortChecksum;

    ulChecksum ^= m_lGeneration;

    m_ck.BeginChecksum( ulChecksum );

    m_ibChecksumIncremental = m_ibLRCK + sizeof( *plrck );

    {
        const BYTE* const pbMin = reinterpret_cast< const BYTE* >( plrck ) + sizeof( *plrck );
        const BYTE* const pbMaxForwards = pbMin + plrck->le_cbForwards;
        const BYTE* const pbMaxBlock = pb + cb;
        const BYTE* const pbMax = min( pbMaxForwards, pbMaxBlock );

        if ( ( pbMin < pb ) ||
            ( pbMin > pb + cb ) )
        {
            return ErrERRCheck( JET_errLogReadVerifyFailure );
        }
        if ( ( pbMax < pb ) ||
            ( pbMax > pb + cb ) )
        {
            return ErrERRCheck( JET_errLogReadVerifyFailure );
        }
        if ( pbMax < pbMin )
        {
            return ErrERRCheck( JET_errLogReadVerifyFailure );
        }
        m_ck.ChecksumBytes( pbMin, pbMax );
        m_ibChecksumIncremental += ULONG( pbMax - pbMin );
    }

    return ErrVerifyLRCKEnd();
}




ERR LOG_READ_BUFFER::ErrLGCheckReadLastLogRecordFF_Legacy(
    BOOL                    *pfCloseNormally,
    const BOOL              fReadOnly,
    BOOL                    *pfIsPatchable,
    LGPOS                   *plgposLastTerm )
{
    ERR             err;
    LRCHECKSUM      *plrck;
    LGPOS           lgposCurrent;
    LGPOS           lgposLast;
    LGPOS           lgposEnd;
    LGPOS           lgposNext;
    BYTE            *pbEnsure;
    BYTE            *pbLastSector;
    BYTE            *pbLastChecksum;
    UINT            isecLastSector;
    ULONG           isecPatternFill;
    ULONG           csecPatternFill;
    BOOL            fGotQuit;
    LGPOS           lgposLastTerm;
    BOOL            fCreatedLogReader;
    BOOL            fLogToNextLogFile;
    BOOL            fDoScan;
    BOOL            fRecordOkRangeBad;
    LGPOS           lgposScan;
    BOOL            fJetLog;
    BOOL            fTookJump;
    BOOL            fSingleSectorTornWrite;
    BOOL            fSkipScanAndApplyPatch;
    ULONG           cbChecksum;
    ULONG           ulChecksumExpected;
    ULONG           ulChecksumActual;
#ifdef ENABLE_LOGPATCH_TRACE
    CPRINTFFILE     *pcprintfLogPatch = NULL;
    WCHAR           wszLogPatchPath[ IFileSystemAPI::cchPathMax ];
#endif


    Assert( m_pLogStream->CbSecVolume() != ~(ULONG)0 );
    Assert( m_pLogStream->CbSecVolume() % 512 == 0 );


    Assert( !pfIsPatchable || m_pLog->FDumpingLogs() );


    err                         = JET_errSuccess;
    plrck                       = pNil;
    lgposCurrent.ib             = 0;
    lgposCurrent.isec           = USHORT( m_pLogStream->CSecHeader() );
    lgposCurrent.lGeneration    = m_pLogStream->GetCurrentFileGen();
    lgposLast.ib                = 0;
    lgposLast.isec              = 0;
    lgposLast.lGeneration       = m_pLogStream->GetCurrentFileGen();
    lgposEnd.ib                 = 0;
    lgposEnd.isec               = 0;
    lgposEnd.lGeneration        = m_pLogStream->GetCurrentFileGen();
    pbEnsure                    = pbNil;
    pbLastSector                = pbNil;
    pbLastChecksum              = pbNil;
    isecLastSector              = m_pLogStream->CSecHeader();
    isecPatternFill             = 0;
    csecPatternFill             = 0;
    fDoScan                     = fFalse;
    fGotQuit                    = fFalse;
    lgposLastTerm               = lgposMin;
    fCreatedLogReader           = fFalse;
    fLogToNextLogFile           = fTrue;
    fRecordOkRangeBad           = fFalse;
    lgposScan.ib                = 0;
    lgposScan.isec              = 0;
    lgposScan.lGeneration       = m_pLogStream->GetCurrentFileGen();
    fTookJump                   = fFalse;
    fSingleSectorTornWrite      = fFalse;
    fSkipScanAndApplyPatch      = fFalse;
    cbChecksum                  = 0;
    ulChecksumExpected          = 0;
    ulChecksumActual            = 0;


#ifdef ENABLE_LOGPATCH_TRACE


{
    WCHAR   wszFolder[ IFileSystemAPI::cchPathMax ];
    WCHAR   wszT[ IFileSystemAPI::cchPathMax ];

    if ( m_pinst->m_pfsapi->ErrPathParse( m_pLogStream->LogName(), wszFolder, wszT, wszT ) < JET_errSuccess ||
         m_pinst->m_pfsapi->ErrPathBuild( wszFolder, L"LOGPATCH", L"TXT", wszLogPatchPath ) < JET_errSuccess )
    {
        OSStrCbCopyW( wszLogPatchPath, sizeof(wszLogPatchPath), L"LOGPATCH.TXT" );
    }
}
#endif

    if ( pfIsPatchable )
    {
        *pfIsPatchable = fFalse;
    }


    m_lgposLastRec = lgposCurrent;


{
        WCHAR wszT[IFileSystemAPI::cchPathMax], wszFNameT[IFileSystemAPI::cchPathMax];

        CallS( m_pinst->m_pfsapi->ErrPathParse( m_pLogStream->LogName(), wszT, wszFNameT, wszT ) );
        fJetLog = ( UtilCmpFileName( wszFNameT, SzParam( m_pinst, JET_paramBaseName ) ) == 0 );
}

    const BOOL  fReadOnlyLocal = fReadOnly || !fJetLog;
    
    Assert( m_pLog->FRecovering() );
    Assert( m_pLog->FRecoveringMode() == fRecoveringRedo );



    if ( m_wszReaderLogName[0] != 0 )
    {
        fCreatedLogReader = fFalse;

        LReaderReset();

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
    }


    Assert( pfCloseNormally );
    *pfCloseNormally = fFalse;


    Assert( m_pLogStream->FLGFileOpened() );
    if ( !fReadOnlyLocal )
    {
        Assert( NULL != m_pLogStream->LogName() );
        Assert( L'\0' != m_pLogStream->LogName()[ 0 ] );
        Call( m_pLogStream->ErrLGOpenFile() );
    }


    Call( ErrReaderEnsureLogFile() );


    pbLastSector = reinterpret_cast< BYTE* >( PvOSMemoryPageAlloc( m_pLogStream->CbSec(), NULL ) );
    if ( pbNil == pbLastSector )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }



    forever
    {


        Call( ErrReaderEnsureSector( lgposCurrent.isec, 1, &pbEnsure ) );
        plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposCurrent.ib );


        cbChecksum = m_pLogStream->CbSec();

        ulChecksumExpected = 0;
        ulChecksumActual = 0;


        if ( !FValidLRCKRecord( plrck, &lgposCurrent ) )
        {
            if ( lrtypChecksum == plrck->lrtyp
                && plrck->bUseShortChecksum == bShortChecksumOn )
            {
                if ( plrck->le_cbBackwards > ( m_pLogStream->CbSec() - sizeof( LRCHECKSUM ) ) )
                {
                    AssertSz( fFalse, "Unexpected invalid LRCK record - large le_cbBackwards = %#x!\n", (ULONG)plrck->le_cbBackwards );
                    Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                }

                const ULONG ulChecksumT     = UlComputeShortChecksum( plrck, ULONG32( lgposCurrent.lGeneration ) );

                if ( ulChecksumT != plrck->le_ulShortChecksum )
                {
                    ulChecksumActual = ulChecksumT;
                    ulChecksumExpected = plrck->le_ulShortChecksum;
                }
                else
                {
                }
            }

#ifdef ENABLE_LOGPATCH_TRACE
            if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
            {
                ULONG _ibdumpT_;

                (*pcprintfLogPatch)( "invalid LRCK record in logfile %ws\r\n", m_pLogStream->LogName() );

                (*pcprintfLogPatch)( "\r\n\tdumping state of ErrLGCheckReadLastLogRecordFF:\r\n" );
                (*pcprintfLogPatch)( "\t\terr                    = %d \r\n", err );
                (*pcprintfLogPatch)( "\t\tlgposCurrent           = {0x%x,0x%x,0x%x}\r\n", lgposCurrent.lGeneration, lgposCurrent.isec, lgposCurrent.ib );
                (*pcprintfLogPatch)( "\t\tlgposLast              = {0x%x,0x%x,0x%x}\r\n", lgposLast.lGeneration, lgposLast.isec, lgposLast.ib  );
                (*pcprintfLogPatch)( "\t\tlgposEnd               = {0x%x,0x%x,0x%x}\r\n", lgposEnd.lGeneration, lgposEnd.isec, lgposEnd.ib );
                (*pcprintfLogPatch)( "\t\tisecLastSector         = %d\r\n", isecLastSector );
                (*pcprintfLogPatch)( "\t\tisecPatternFill        = 0x%x\r\n", isecPatternFill );
                (*pcprintfLogPatch)( "\t\tcsecPatternFill        = 0x%x\r\n", csecPatternFill );
                (*pcprintfLogPatch)( "\t\tfGotQuit               = %s\r\n", ( fGotQuit ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tlgposLastTerm          = {0x%x,0x%x,0x%x}\r\n", lgposLastTerm.lGeneration, lgposLastTerm.isec, lgposLastTerm.ib );
                (*pcprintfLogPatch)( "\t\tfCreatedLogReader      = %s\r\n", ( fCreatedLogReader ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tfLogToNextLogFile      = %s\r\n", ( fLogToNextLogFile ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tfDoScan                = %s\r\n", ( fDoScan ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tfRecordOkRangeBad      = %s\r\n", ( fRecordOkRangeBad ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tlgposScan              = {0x%x,0x%x,0x%x}\r\n", lgposScan.lGeneration, lgposScan.isec, lgposScan.ib );
                (*pcprintfLogPatch)( "\t\tfJetLog                = %s\r\n", ( fJetLog ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tfTookJump              = %s\r\n", ( fTookJump ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tfSingleSectorTornWrite = %s\r\n", ( fSingleSectorTornWrite ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tfSkipScanAndApplyPatch = %s\r\n", ( fSkipScanAndApplyPatch ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tcbChecksum             = 0x%x\r\n", cbChecksum );

                (*pcprintfLogPatch)( "\r\n\tdumping partial state of LOG:\r\n" );
                (*pcprintfLogPatch)( "\t\tLOG::FRecovering()     = %s\r\n", ( m_pLog->FRecovering() ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tLOG::FRecoveringMode() = %d\r\n", m_pLog->FRecoveringMode() );
                (*pcprintfLogPatch)( "\t\tLOG::FHardRestore()    = %s\r\n", ( m_pLog->FHardRestore() ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tLOG::m_pLogStream->CSecLGFile()      = 0x%08x\r\n", m_pLogStream->CSecLGFile() );
                (*pcprintfLogPatch)( "\t\tLOG::m_csecLGBuf       = 0x%08x\r\n", m_csecLGBuf );
                (*pcprintfLogPatch)( "\t\tLOG::m_pLogStream->CSecHeader()      = %d\r\n", m_pLogStream->CSecHeader() );
                (*pcprintfLogPatch)( "\t\tLOG::m_cbSec           = %d\r\n", m_pLogStream->CbSec() );
                (*pcprintfLogPatch)( "\t\tLOG::m_cbSecVolume     = %d\r\n", m_pLogStream->CbSecVolume() );
                (*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMin      = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMin ) );
                (*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMax      = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMax ) );
                (*pcprintfLogPatch)( "\t\tLOG::m_pbWrite         = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbWrite ) );
                (*pcprintfLogPatch)( "\t\tLOG::m_pbEntry         = 0x%0*I64x: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n", sizeof( char* ) * 2, QWORD( m_pbEntry ), m_pbEntry[0], m_pbEntry[1], m_pbEntry[2], m_pbEntry[3], m_pbEntry[4], m_pbEntry[5], m_pbEntry[6], m_pbEntry[7] );

                (*pcprintfLogPatch)( "\r\n\tdumping data:\r\n" );

                (*pcprintfLogPatch)( "\t\tplrck (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( plrck ) );
                (*pcprintfLogPatch)( "\t\t  cbBackwards       = 0x%08x\r\n", (ULONG)plrck->le_cbBackwards );
                (*pcprintfLogPatch)( "\t\t  cbForwards        = 0x%08x\r\n", (ULONG)plrck->le_cbForwards );
                (*pcprintfLogPatch)( "\t\t  cbNext            = 0x%08x\r\n", (ULONG)plrck->le_cbNext );
                (*pcprintfLogPatch)( "\t\t  ulChecksum        = 0x%08x\r\n", (ULONG)plrck->le_ulChecksum );
                (*pcprintfLogPatch)( "\t\t  ulShortChecksum   = 0x%08x\r\n", (ULONG)plrck->le_ulShortChecksum );
                (*pcprintfLogPatch)( "\t\t  bUseShortChecksum = %s (0x%02x)\r\n",
                                    ( bShortChecksumOn == plrck->bUseShortChecksum ?
                                      "Yes" :
                                      ( bShortChecksumOff == plrck->bUseShortChecksum ?
                                        "No" : "???" ) ),
                                    BYTE( plrck->bUseShortChecksum ) );

                (*pcprintfLogPatch)( "\t\tpbEnsure (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbEnsure ) );

                _ibdumpT_ = 0;
                while ( _ibdumpT_ < m_pLogStream->CbSec() )
                {
                    (*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                                        _ibdumpT_,
                                        pbEnsure[_ibdumpT_+0],  pbEnsure[_ibdumpT_+1],  pbEnsure[_ibdumpT_+2],  pbEnsure[_ibdumpT_+3],
                                        pbEnsure[_ibdumpT_+4],  pbEnsure[_ibdumpT_+5],  pbEnsure[_ibdumpT_+6],  pbEnsure[_ibdumpT_+7],
                                        pbEnsure[_ibdumpT_+8],  pbEnsure[_ibdumpT_+9],  pbEnsure[_ibdumpT_+10], pbEnsure[_ibdumpT_+11],
                                        pbEnsure[_ibdumpT_+12], pbEnsure[_ibdumpT_+13], pbEnsure[_ibdumpT_+14], pbEnsure[_ibdumpT_+15] );
                    _ibdumpT_ += 16;
                    Assert( _ibdumpT_ <= m_pLogStream->CbSec() );
                }

                (*pcprintfLogPatch)( "\t\tpbLastSector (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbLastSector ) );

                _ibdumpT_ = 0;
                while ( _ibdumpT_ < m_pLogStream->CbSec() )
                {
                    (*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                                        _ibdumpT_,
                                        pbLastSector[_ibdumpT_+0],  pbLastSector[_ibdumpT_+1],  pbLastSector[_ibdumpT_+2],  pbLastSector[_ibdumpT_+3],
                                        pbLastSector[_ibdumpT_+4],  pbLastSector[_ibdumpT_+5],  pbLastSector[_ibdumpT_+6],  pbLastSector[_ibdumpT_+7],
                                        pbLastSector[_ibdumpT_+8],  pbLastSector[_ibdumpT_+9],  pbLastSector[_ibdumpT_+10], pbLastSector[_ibdumpT_+11],
                                        pbLastSector[_ibdumpT_+12], pbLastSector[_ibdumpT_+13], pbLastSector[_ibdumpT_+14], pbLastSector[_ibdumpT_+15] );
                    _ibdumpT_ += 16;
                    Assert( _ibdumpT_ <= m_pLogStream->CbSec() );
                }

                if ( pbLastChecksum )
                {
                    LRCHECKSUM *_plrckT_ = (LRCHECKSUM *)pbLastChecksum;

                    (*pcprintfLogPatch)( "\t\tpbLastChecksum (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( _plrckT_ ) );
                    (*pcprintfLogPatch)( "\t\t  cbBackwards       = 0x%08x\r\n", (ULONG)_plrckT_->le_cbBackwards );
                    (*pcprintfLogPatch)( "\t\t  cbForwards        = 0x%08x\r\n", (ULONG)_plrckT_->le_cbForwards );
                    (*pcprintfLogPatch)( "\t\t  cbNext            = 0x%08x\r\n", (ULONG)_plrckT_->le_cbNext );
                    (*pcprintfLogPatch)( "\t\t  ulChecksum        = 0x%08x\r\n", (ULONG)_plrckT_->le_ulChecksum );
                    (*pcprintfLogPatch)( "\t\t  ulShortChecksum   = 0x%08x\r\n", (ULONG)_plrckT_->le_ulShortChecksum );
                    (*pcprintfLogPatch)( "\t\t  bUseShortChecksum = %s (0x%02x)\r\n",
                                        ( bShortChecksumOn == _plrckT_->bUseShortChecksum ?
                                          "Yes" :
                                          ( bShortChecksumOff == _plrckT_->bUseShortChecksum ?
                                            "No" : "???" ) ),
                                        BYTE( _plrckT_->bUseShortChecksum ) );
                }
                else
                {
                    (*pcprintfLogPatch)( "\t\tpbLastChecksum (null)\r\n" );
                }

                (*pcprintfLogPatch)( "\r\n" );
            }
#endif


            fTookJump = fFalse;

RecoverWithShadow:



            Call( ErrReaderEnsureSector( lgposCurrent.isec + 1, 1, &pbEnsure ) );
            plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposCurrent.ib );


            if ( !FValidLRCKShadow( plrck, &lgposCurrent, m_pLogStream->GetCurrentFileGen() ) )
            {
#ifdef ENABLE_LOGPATCH_TRACE
                if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                {
                    (*pcprintfLogPatch)( "invalid shadow in sector 0x%x (%d)\r\n",
                                        lgposCurrent.isec + 1,
                                        lgposCurrent.isec + 1 );

                    (*pcprintfLogPatch)( "\t\tpbEnsure (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbEnsure ) );

                    ULONG _ibdumpT_ = 0;
                    while ( _ibdumpT_ < m_pLogStream->CbSec() )
                    {
                        (*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                                            _ibdumpT_,
                                            pbEnsure[_ibdumpT_+0],  pbEnsure[_ibdumpT_+1],  pbEnsure[_ibdumpT_+2],  pbEnsure[_ibdumpT_+3],
                                            pbEnsure[_ibdumpT_+4],  pbEnsure[_ibdumpT_+5],  pbEnsure[_ibdumpT_+6],  pbEnsure[_ibdumpT_+7],
                                            pbEnsure[_ibdumpT_+8],  pbEnsure[_ibdumpT_+9],  pbEnsure[_ibdumpT_+10], pbEnsure[_ibdumpT_+11],
                                            pbEnsure[_ibdumpT_+12], pbEnsure[_ibdumpT_+13], pbEnsure[_ibdumpT_+14], pbEnsure[_ibdumpT_+15] );
                        _ibdumpT_ += 16;
                        Assert( _ibdumpT_ <= m_pLogStream->CbSec() );
                    }
                }
#endif




                if ( !fTookJump && lgposCurrent.isec == lgposLast.isec + 1 )
                {


                    Assert( lgposLast.isec >= m_pLogStream->CSecHeader() && lgposLast.isec < ( m_pLogStream->CSecLGFile() - 1 ) );


                    Call( ErrReaderEnsureSector( lgposLast.isec, 2, &pbEnsure ) );


                    plrck = (LRCHECKSUM *)( pbEnsure + lgposLast.ib );


                    AssertValidLRCKRecord( plrck, &lgposLast );
                    AssertValidLRCKRange( plrck, lgposLast.lGeneration );
                    Assert( plrck->bUseShortChecksum == bShortChecksumOff );
                    Assert( plrck->le_cbNext != 0 );
                    Assert( lgposLast.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards == m_pLogStream->CbSec() );


                    plrck = (LRCHECKSUM *)( pbEnsure + m_pLogStream->CbSec() + lgposLast.ib );


                    if ( FValidLRCKShadowWithoutCheckingCBNext( plrck, &lgposLast, lgposLast.lGeneration ) )
                    {
#ifdef ENABLE_LOGPATCH_TRACE
                        if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                        {
                            (*pcprintfLogPatch)( "special case!\r\n" );
                            (*pcprintfLogPatch)( "\tlgposCurrent is NOT pointing to the next LRCHECKSUM record because we crashed before we could write it\r\n" );
                            (*pcprintfLogPatch)( "\tinstead, it is pointing to a shadow of the >>LAST<< LRCHECKSUM record (lgposLast)\r\n" );
                        }
#endif




                        fDoScan = fTrue;


                        lgposEnd.isec = lgposCurrent.isec;
                        lgposEnd.ib = 0;


                        lgposScan = lgposEnd;
                        m_pLogStream->AddLgpos( &lgposScan, m_pLogStream->CbSec() );


                        m_lgposLastRec = lgposEnd;



                        UtilMemCpy( pbLastSector, pbEnsure, m_pLogStream->CbSec() );


                        pbLastChecksum = pbLastSector + lgposLast.ib;
                        isecLastSector = lgposLast.isec;


                        fSingleSectorTornWrite = fTrue;


                        break;
                    }
                }



                fDoScan = fTrue;



                lgposEnd.isec = lgposCurrent.isec;
                lgposEnd.ib = 0;


                lgposScan = lgposEnd;


                m_lgposLastRec = lgposEnd;


                pbLastChecksum = pbLastSector + lgposCurrent.ib;



                plrck = reinterpret_cast< LRCHECKSUM* >( pbLastChecksum );
                memset( pbLastChecksum, 0, sizeof( LRCHECKSUM ) );
                plrck->lrtyp = lrtypChecksum;
                plrck->bUseShortChecksum = bShortChecksumOff;


                isecLastSector = lgposCurrent.isec;


                break;
            }

#ifdef ENABLE_LOGPATCH_TRACE
            if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
            {
                (*pcprintfLogPatch)( "shadow is OK -- we will patch with it (unless we are in read-only mode)\r\n" );

                (*pcprintfLogPatch)( "\r\n\t\tpbEnsure (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbEnsure ) );

                ULONG _ibdumpT_ = 0;
                while ( _ibdumpT_ < m_pLogStream->CbSec() )
                {
                    (*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                                        _ibdumpT_,
                                        pbEnsure[_ibdumpT_+0],  pbEnsure[_ibdumpT_+1],  pbEnsure[_ibdumpT_+2],  pbEnsure[_ibdumpT_+3],
                                        pbEnsure[_ibdumpT_+4],  pbEnsure[_ibdumpT_+5],  pbEnsure[_ibdumpT_+6],  pbEnsure[_ibdumpT_+7],
                                        pbEnsure[_ibdumpT_+8],  pbEnsure[_ibdumpT_+9],  pbEnsure[_ibdumpT_+10], pbEnsure[_ibdumpT_+11],
                                        pbEnsure[_ibdumpT_+12], pbEnsure[_ibdumpT_+13], pbEnsure[_ibdumpT_+14], pbEnsure[_ibdumpT_+15] );
                    _ibdumpT_ += 16;
                    Assert( _ibdumpT_ <= m_pLogStream->CbSec() );
                }
            }
#endif



            Assert( plrck->le_cbNext == 0 );
            Assert( ( plrck->le_cbBackwards + sizeof( LRCHECKSUM ) + plrck->le_cbForwards ) <= m_pLogStream->CbSec() );


            if ( fReadOnlyLocal )
            {
                Assert( !m_pLog->FHardRestore() );
                Assert( fCreatedLogReader );


                m_lgposLastRec = lgposCurrent;
                m_lgposLastRec.ib = 0;

                if ( pfIsPatchable )
                {


                    *pfIsPatchable = fTrue;
                    err = JET_errSuccess;
                    goto HandleError;
                }
                else
                {

                    const QWORD     ibOffset    = QWORD( lgposCurrent.isec ) * QWORD( m_pLogStream->CbSec() );
                    const DWORD     cbLength    = cbChecksum;

                    LGIReportChecksumMismatch(
                            m_pinst,
                            m_pLogStream->LogName(),
                            ibOffset,
                            cbLength,
                            ulChecksumExpected,
                            ulChecksumActual,
                            JET_errLogFileCorrupt );


                    Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                }
            }


            fDoScan = fFalse;


            const ULONG csz = 1;
            const WCHAR     *rgpsz[csz] = { m_pLogStream->LogName() };

            UtilReportEvent(    eventWarning,
                                LOGGING_RECOVERY_CATEGORY,
                                LOG_USING_SHADOW_SECTOR_ID,
                                csz,
                                rgpsz,
                                0,
                                NULL,
                                m_pinst );


            plrck->le_ulChecksum = UlComputeChecksum( plrck, m_pLogStream->GetCurrentFileGen() );


Assert( m_pLogStream->CbSec() == m_pLogStream->CbSecVolume() );
                Call( m_pLogStream->ErrLGWriteSectorData(
                        IOR( iorpPatchFix ),
                        m_lgposLastRec.lGeneration,
                        m_pLogStream->CbSec() * QWORD( lgposCurrent.isec ),
                        m_pLogStream->CbSec(),
                        pbEnsure,
                        LOG_WRITE_ERROR_ID ) );
                ReaderSectorModified( lgposCurrent.isec, pbEnsure );


            if ( plrck->le_cbForwards == 0 )
            {



                lgposEnd = lgposCurrent;
                m_pLogStream->AddLgpos( &lgposEnd, sizeof( LRCHECKSUM ) );
            }
            else
            {



                lgposLast = lgposCurrent;


                lgposEnd = lgposCurrent;
                Assert( plrck->le_cbBackwards == lgposCurrent.ib || 0 == plrck->le_cbBackwards );
                m_pLogStream->AddLgpos( &lgposEnd, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );
            }


            m_lgposLastRec = lgposEnd;



            UtilMemCpy( pbLastSector, pbEnsure, m_pLogStream->CbSec() );


            pbLastChecksum = pbLastSector + lgposCurrent.ib;
            isecLastSector = lgposCurrent.isec;


            break;
        }




        Call( ErrReaderEnsureSector( lgposCurrent.isec, ( ( lgposCurrent.ib +
            sizeof( LRCHECKSUM ) + plrck->le_cbForwards - 1 ) / m_pLogStream->CbSec() ) + 1,
            &pbEnsure ) );


        plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposCurrent.ib );


        cbChecksum = lgposCurrent.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards;

        ulChecksumExpected = plrck->le_ulChecksum;
        ulChecksumActual = UlComputeChecksum( plrck, ULONG32( m_pLogStream->GetCurrentFileGen() ) );

        if ( ulChecksumExpected != ulChecksumActual )
        {
            Assert( !g_fLRCKValidationTrap );

#ifdef ENABLE_LOGPATCH_TRACE
            if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
            {
                ULONG _ibdumpT_;

                (*pcprintfLogPatch)( "invalid LRCK range in logfile %ws\r\n", m_pLogStream->LogName() );

                (*pcprintfLogPatch)( "\r\n\tdumping state of ErrLGCheckReadLastLogRecordFF:\r\n" );
                (*pcprintfLogPatch)( "\t\terr                    = %d \r\n", err );
                (*pcprintfLogPatch)( "\t\tlgposCurrent           = {0x%x,0x%x,0x%x}\r\n", lgposCurrent.lGeneration, lgposCurrent.isec, lgposCurrent.ib );
                (*pcprintfLogPatch)( "\t\tlgposLast              = {0x%x,0x%x,0x%x}\r\n", lgposLast.lGeneration, lgposLast.isec, lgposLast.ib  );
                (*pcprintfLogPatch)( "\t\tlgposEnd               = {0x%x,0x%x,0x%x}\r\n", lgposEnd.lGeneration, lgposEnd.isec, lgposEnd.ib );
                (*pcprintfLogPatch)( "\t\tisecLastSector         = %d\r\n", isecLastSector );
                (*pcprintfLogPatch)( "\t\tisecPatternFill        = 0x%x\r\n", isecPatternFill );
                (*pcprintfLogPatch)( "\t\tcsecPatternFill        = 0x%x\r\n", csecPatternFill );
                (*pcprintfLogPatch)( "\t\tfGotQuit               = %s\r\n", ( fGotQuit ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tlgposLastTerm          = {0x%x,0x%x,0x%x}\r\n", lgposLastTerm.lGeneration, lgposLastTerm.isec, lgposLastTerm.ib );
                (*pcprintfLogPatch)( "\t\tfCreatedLogReader      = %s\r\n", ( fCreatedLogReader ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tfLogToNextLogFile      = %s\r\n", ( fLogToNextLogFile ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tfDoScan                = %s\r\n", ( fDoScan ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tfRecordOkRangeBad      = %s\r\n", ( fRecordOkRangeBad ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tlgposScan              = {0x%x,0x%x,0x%x}\r\n", lgposScan.lGeneration, lgposScan.isec, lgposScan.ib );
                (*pcprintfLogPatch)( "\t\tfJetLog                = %s\r\n", ( fJetLog ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tfTookJump              = %s\r\n", ( fTookJump ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tfSingleSectorTornWrite = %s\r\n", ( fSingleSectorTornWrite ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tfSkipScanAndApplyPatch = %s\r\n", ( fSkipScanAndApplyPatch ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tcbChecksum             = 0x%x\r\n", cbChecksum );

                (*pcprintfLogPatch)( "\r\n\tdumping partial state of LOG:\r\n" );
                (*pcprintfLogPatch)( "\t\tLOG::FRecovering()     = %s\r\n", ( m_pLog->FRecovering() ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tLOG::FRecoveringMode() = %d\r\n", m_pLog->FRecoveringMode() );
                (*pcprintfLogPatch)( "\t\tLOG::FHardRestore()    = %s\r\n", ( m_pLog->FHardRestore() ? "TRUE" : "FALSE" ) );
                (*pcprintfLogPatch)( "\t\tLOG::m_pLogStream->CSecLGFile()      = 0x%08x\r\n", m_pLogStream->CSecLGFile() );
                (*pcprintfLogPatch)( "\t\tLOG::m_csecLGBuf       = 0x%08x\r\n", m_csecLGBuf );
                (*pcprintfLogPatch)( "\t\tLOG::m_csecHeader      = %d\r\n", m_pLogStream->CSecHeader() );
                (*pcprintfLogPatch)( "\t\tLOG::m_cbSec           = %d\r\n", m_pLogStream->CbSec() );
                (*pcprintfLogPatch)( "\t\tLOG::m_cbSecVolume     = %d\r\n", m_pLogStream->CbSecVolume() );
                (*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMin      = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMin ) );
                (*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMax      = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMax ) );
                (*pcprintfLogPatch)( "\t\tLOG::m_pbWrite         = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbWrite ) );
                (*pcprintfLogPatch)( "\t\tLOG::m_pbEntry         = 0x%0*I64x: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n", sizeof( char* ) * 2, QWORD( m_pbEntry ), m_pbEntry[0], m_pbEntry[1], m_pbEntry[2], m_pbEntry[3], m_pbEntry[4], m_pbEntry[5], m_pbEntry[6], m_pbEntry[7] );

                (*pcprintfLogPatch)( "\r\n\tdumping data:\r\n" );

                (*pcprintfLogPatch)( "\t\tplrck (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( plrck ) );
                (*pcprintfLogPatch)( "\t\t  cbBackwards       = 0x%08x\r\n", (ULONG)plrck->le_cbBackwards );
                (*pcprintfLogPatch)( "\t\t  cbForwards        = 0x%08x\r\n", (ULONG)plrck->le_cbForwards );
                (*pcprintfLogPatch)( "\t\t  cbNext            = 0x%08x\r\n", (ULONG)plrck->le_cbNext );
                (*pcprintfLogPatch)( "\t\t  ulChecksum        = 0x%08x\r\n", (ULONG)plrck->le_ulChecksum );
                (*pcprintfLogPatch)( "\t\t  ulShortChecksum   = 0x%08x\r\n", (ULONG)plrck->le_ulShortChecksum );
                (*pcprintfLogPatch)( "\t\t  bUseShortChecksum = %s (0x%02x)\r\n",
                                    ( bShortChecksumOn == plrck->bUseShortChecksum ?
                                      "Yes" :
                                      ( bShortChecksumOff == plrck->bUseShortChecksum ?
                                        "No" : "???" ) ),
                                    BYTE( plrck->bUseShortChecksum ) );

                (*pcprintfLogPatch)( "\t\tpbEnsure (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbEnsure ) );

                _ibdumpT_ = 0;
                while ( _ibdumpT_ < m_pLogStream->CbSec() )
                {
                    (*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                                        _ibdumpT_,
                                        pbEnsure[_ibdumpT_+0],  pbEnsure[_ibdumpT_+1],  pbEnsure[_ibdumpT_+2],  pbEnsure[_ibdumpT_+3],
                                        pbEnsure[_ibdumpT_+4],  pbEnsure[_ibdumpT_+5],  pbEnsure[_ibdumpT_+6],  pbEnsure[_ibdumpT_+7],
                                        pbEnsure[_ibdumpT_+8],  pbEnsure[_ibdumpT_+9],  pbEnsure[_ibdumpT_+10], pbEnsure[_ibdumpT_+11],
                                        pbEnsure[_ibdumpT_+12], pbEnsure[_ibdumpT_+13], pbEnsure[_ibdumpT_+14], pbEnsure[_ibdumpT_+15] );
                    _ibdumpT_ += 16;
                    Assert( _ibdumpT_ <= m_pLogStream->CbSec() );
                }

                (*pcprintfLogPatch)( "\t\tpbLastSector (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbLastSector ) );

                _ibdumpT_ = 0;
                while ( _ibdumpT_ < m_pLogStream->CbSec() )
                {
                    (*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                                        _ibdumpT_,
                                        pbLastSector[_ibdumpT_+0],  pbLastSector[_ibdumpT_+1],  pbLastSector[_ibdumpT_+2],  pbLastSector[_ibdumpT_+3],
                                        pbLastSector[_ibdumpT_+4],  pbLastSector[_ibdumpT_+5],  pbLastSector[_ibdumpT_+6],  pbLastSector[_ibdumpT_+7],
                                        pbLastSector[_ibdumpT_+8],  pbLastSector[_ibdumpT_+9],  pbLastSector[_ibdumpT_+10], pbLastSector[_ibdumpT_+11],
                                        pbLastSector[_ibdumpT_+12], pbLastSector[_ibdumpT_+13], pbLastSector[_ibdumpT_+14], pbLastSector[_ibdumpT_+15] );
                    _ibdumpT_ += 16;
                    Assert( _ibdumpT_ <= m_pLogStream->CbSec() );
                }

                if ( pbLastChecksum )
                {
                    LRCHECKSUM *_plrckT_ = (LRCHECKSUM *)pbLastChecksum;

                    (*pcprintfLogPatch)( "\t\tpbLastChecksum (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( _plrckT_ ) );
                    (*pcprintfLogPatch)( "\t\t  cbBackwards       = 0x%08x\r\n", (ULONG)_plrckT_->le_cbBackwards );
                    (*pcprintfLogPatch)( "\t\t  cbForwards        = 0x%08x\r\n", (ULONG)_plrckT_->le_cbForwards );
                    (*pcprintfLogPatch)( "\t\t  cbNext            = 0x%08x\r\n", (ULONG)_plrckT_->le_cbNext );
                    (*pcprintfLogPatch)( "\t\t  ulChecksum        = 0x%08x\r\n", (ULONG)_plrckT_->le_ulChecksum );
                    (*pcprintfLogPatch)( "\t\t  ulShortChecksum   = 0x%08x\r\n", (ULONG)_plrckT_->le_ulShortChecksum );
                    (*pcprintfLogPatch)( "\t\t  bUseShortChecksum = %s (0x%02x)\r\n",
                                        ( bShortChecksumOn == _plrckT_->bUseShortChecksum ?
                                          "Yes" :
                                          ( bShortChecksumOff == _plrckT_->bUseShortChecksum ?
                                            "No" : "???" ) ),
                                        BYTE( _plrckT_->bUseShortChecksum ) );
                }
                else
                {
                    (*pcprintfLogPatch)( "\t\tpbLastChecksum (null)\r\n" );
                }

                (*pcprintfLogPatch)( "\r\n" );
            }
#endif



            if ( plrck->bUseShortChecksum == bShortChecksumOn )
            {






                fDoScan = fTrue;


                lgposLast = lgposCurrent;


                lgposEnd.isec = lgposCurrent.isec;
                lgposEnd.ib = 0;
                m_pLogStream->AddLgpos( &lgposEnd, m_pLogStream->CbSec() );


                m_lgposLastRec = lgposEnd;


                lgposScan = lgposCurrent;
                m_pLogStream->AddLgpos( &lgposScan, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );


                Assert( lgposScan.ib == 0 );


                fRecordOkRangeBad = fTrue;




                break;
            }



            Assert( plrck->bUseShortChecksum == bShortChecksumOff );



            Assert( lgposCurrent.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards <= m_pLogStream->CbSec() );


            if (    lgposLast.isec >= m_pLogStream->CSecHeader() &&
                    lgposLast.isec == lgposCurrent.isec - 1 &&
                    lgposLast.ib == lgposCurrent.ib )
            {


                Call( ErrReaderEnsureSector( lgposLast.isec, 2, &pbEnsure ) );
                plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposCurrent.ib );


                if ( FValidLRCKShadow(  reinterpret_cast< LRCHECKSUM* >( pbEnsure + m_pLogStream->CbSec() + lgposCurrent.ib ),
                                        &lgposCurrent, m_pLogStream->GetCurrentFileGen() ) )
                {
#ifdef ENABLE_LOGPATCH_TRACE
                    if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                    {
                        (*pcprintfLogPatch)( "special case!\r\n" );
                        (*pcprintfLogPatch)( "\twe crashed during a multi-sector flush which was replacing a single-sector shadowed flush\r\n" );
                        (*pcprintfLogPatch)( "\tthe crash occurred AFTER writing the first sector but BEFORE writing sectors 2-N\r\n" );

                        LRCHECKSUM *_plrckT_ = (LRCHECKSUM *)( pbEnsure + m_pLogStream->CbSec() + lgposCurrent.ib );

                        (*pcprintfLogPatch)( "\r\n\tplrck (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( _plrckT_ ) );
                        (*pcprintfLogPatch)( "\t  cbBackwards       = 0x%08x\r\n", (ULONG)_plrckT_->le_cbBackwards );
                        (*pcprintfLogPatch)( "\t  cbForwards        = 0x%08x\r\n", (ULONG)_plrckT_->le_cbForwards );
                        (*pcprintfLogPatch)( "\t  cbNext            = 0x%08x\r\n", (ULONG)_plrckT_->le_cbNext );
                        (*pcprintfLogPatch)( "\t  ulChecksum        = 0x%08x\r\n", (ULONG)_plrckT_->le_ulChecksum );
                        (*pcprintfLogPatch)( "\t  ulShortChecksum   = 0x%08x\r\n", (ULONG)_plrckT_->le_ulShortChecksum );
                        (*pcprintfLogPatch)( "\t  bUseShortChecksum = %s (0x%02x)\r\n",
                                            ( bShortChecksumOn == _plrckT_->bUseShortChecksum ?
                                              "Yes" :
                                              ( bShortChecksumOff == _plrckT_->bUseShortChecksum ?
                                                "No" : "???" ) ),
                                            BYTE( _plrckT_->bUseShortChecksum ) );
                        (*pcprintfLogPatch)( "\tpbEnsure (0x%0*I64x) -- 2 sectors\r\n", sizeof( char* ) * 2, QWORD( pbEnsure ) );

                        ULONG _ibdumpT_ = 0;
                        while ( _ibdumpT_ < 2 * m_pLogStream->CbSec() )
                        {
                            (*pcprintfLogPatch)( "\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                                                _ibdumpT_,
                                                pbEnsure[_ibdumpT_+0],  pbEnsure[_ibdumpT_+1],  pbEnsure[_ibdumpT_+2],  pbEnsure[_ibdumpT_+3],
                                                pbEnsure[_ibdumpT_+4],  pbEnsure[_ibdumpT_+5],  pbEnsure[_ibdumpT_+6],  pbEnsure[_ibdumpT_+7],
                                                pbEnsure[_ibdumpT_+8],  pbEnsure[_ibdumpT_+9],  pbEnsure[_ibdumpT_+10], pbEnsure[_ibdumpT_+11],
                                                pbEnsure[_ibdumpT_+12], pbEnsure[_ibdumpT_+13], pbEnsure[_ibdumpT_+14], pbEnsure[_ibdumpT_+15] );
                            _ibdumpT_ += 16;
                            Assert( _ibdumpT_ <= 2 * m_pLogStream->CbSec() );
                        }
                    }
#endif


                    fDoScan = fTrue;
                    fSkipScanAndApplyPatch = fTrue;


                    lgposScan.lGeneration = lgposLast.lGeneration;


                    lgposEnd.ib             = 0;
                    lgposEnd.isec           = lgposCurrent.isec;
                    lgposEnd.lGeneration    = lgposCurrent.lGeneration;


                    m_lgposLastRec = lgposEnd;


                    pbLastChecksum = pbLastSector + lgposCurrent.ib;
                    isecLastSector = lgposCurrent.isec;


                    Assert( rgbLogExtendPattern );
                    _memcpyp( pbLastSector, rgbLogExtendPattern, cbLogExtendPattern, m_pLogStream->CbSec() );


                    plrck = reinterpret_cast< LRCHECKSUM* >( pbLastChecksum );
                    memset( pbLastChecksum, 0, sizeof( LRCHECKSUM ) );
                    plrck->lrtyp = lrtypChecksum;
                    plrck->bUseShortChecksum = bShortChecksumOff;



                    break;
                }



            }



            fTookJump = fTrue;
            goto RecoverWithShadow;
        }



        if ( plrck->le_cbNext == 0 )
        {
            BOOL fIsMultiSector;



            fDoScan = fFalse;


            fIsMultiSector = ( plrck->bUseShortChecksum == bShortChecksumOn );


            if ( !fIsMultiSector && fJetLog )
            {


                Assert( m_pLogStream->PbSecAligned( reinterpret_cast< const BYTE* >( plrck ), m_pbLGBufMin ) == pbEnsure );
                const ULONG ulNormalChecksum = plrck->le_ulChecksum;


                Assert( ulNormalChecksum == UlComputeChecksum( plrck, (ULONG32)m_pLogStream->GetCurrentFileGen() ) );


                plrck->le_ulChecksum = UlComputeShadowChecksum( ulNormalChecksum );


                if ( !fReadOnlyLocal )
                {
Assert( m_pLogStream->CbSec() == m_pLogStream->CbSecVolume() );
                        Call( m_pLogStream->ErrLGWriteSectorData(
                                IOR( iorpPatchFix ),
                                m_lgposLastRec.lGeneration,
                                m_pLogStream->CbSec() * QWORD( lgposCurrent.isec + 1 ),
                                m_pLogStream->CbSec(),
                                pbEnsure,
                                LOG_WRITE_ERROR_ID ) );
                }


                ReaderSectorModified( lgposCurrent.isec + 1, pbEnsure );


                plrck->le_ulChecksum = ulNormalChecksum;
            }


            if ( plrck->le_cbForwards == 0 )
            {



                Assert( !fIsMultiSector );



                lgposEnd = lgposCurrent;
                m_pLogStream->AddLgpos( &lgposEnd, sizeof( LRCHECKSUM ) );



                UtilMemCpy( pbLastSector, pbEnsure, m_pLogStream->CbSec() );


                pbLastChecksum = pbLastSector + lgposCurrent.ib;
                isecLastSector = lgposCurrent.isec;
            }
            else
            {



                lgposLast = lgposCurrent;


                lgposEnd = lgposCurrent;
                Assert( plrck->le_cbBackwards == lgposCurrent.ib || 0 == plrck->le_cbBackwards );
                m_pLogStream->AddLgpos( &lgposEnd, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );


                if ( fIsMultiSector )
                {




                    if ( fJetLog )
                    {
                        fLogToNextLogFile = fTrue;
                    }
                }
                else
                {




                    UtilMemCpy( pbLastSector, pbEnsure, m_pLogStream->CbSec() );


                    pbLastChecksum = pbLastSector + lgposCurrent.ib;
                    isecLastSector = lgposCurrent.isec;
                }
            }


            m_lgposLastRec = lgposEnd;


            break;
        }




        lgposLast = lgposCurrent;


        Assert( lgposCurrent.ib == plrck->le_cbBackwards || 0 == plrck->le_cbBackwards );
        m_pLogStream->AddLgpos( &lgposCurrent, sizeof( LRCHECKSUM ) + plrck->le_cbNext );


        m_lgposLastRec = lgposCurrent;
        m_lgposLastRec.ib = 0;
    }



    if ( lgposEnd.isec == m_pLogStream->CSecHeader() && 0 == lgposEnd.ib )
    {



        Assert( !fSingleSectorTornWrite );


        Assert( fDoScan );


        Assert( CmpLgpos( &lgposCurrent, &lgposEnd ) == 0 );


        Assert( lgposLast.isec == 0 && lgposLast.ib == 0 );


        lgposLast = lgposCurrent;


        Assert( pbLastChecksum == pbLastSector );


        Assert( isecLastSector == lgposCurrent.isec );
    }
    else if ( lgposLast.isec == 0 )
    {



        lgposLast.isec = USHORT( m_pLogStream->CSecHeader() );
        Assert( lgposLast.ib == 0 );


        Assert( lgposEnd.isec > lgposLast.isec ||
                ( lgposEnd.isec == lgposLast.isec && lgposEnd.ib >= ( lgposLast.ib + sizeof( LRCHECKSUM ) ) ) );
    }


    Assert( CmpLgpos( &lgposLast, &lgposCurrent ) <= 0 );


    if ( fDoScan )
    {
#ifdef ENABLE_LOGPATCH_TRACE
        if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
        {
            ULONG _ibdumpT_;

            (*pcprintfLogPatch)( "scanning logfile %ws for a torn-write or for corruption\r\n", m_pLogStream->LogName() );

            (*pcprintfLogPatch)( "\r\n\tdumping state of ErrLGCheckReadLastLogRecordFF:\r\n" );
            (*pcprintfLogPatch)( "\t\terr                    = %d \r\n", err );
            (*pcprintfLogPatch)( "\t\tlgposCurrent           = {0x%x,0x%x,0x%x}\r\n", lgposCurrent.lGeneration, lgposCurrent.isec, lgposCurrent.ib );
            (*pcprintfLogPatch)( "\t\tlgposLast              = {0x%x,0x%x,0x%x}\r\n", lgposLast.lGeneration, lgposLast.isec, lgposLast.ib  );
            (*pcprintfLogPatch)( "\t\tlgposEnd               = {0x%x,0x%x,0x%x}\r\n", lgposEnd.lGeneration, lgposEnd.isec, lgposEnd.ib );
            (*pcprintfLogPatch)( "\t\tisecLastSector         = %d\r\n", isecLastSector );
            (*pcprintfLogPatch)( "\t\tisecPatternFill        = 0x%x\r\n", isecPatternFill );
            (*pcprintfLogPatch)( "\t\tcsecPatternFill        = 0x%x\r\n", csecPatternFill );
            (*pcprintfLogPatch)( "\t\tfGotQuit               = %s\r\n", ( fGotQuit ? "TRUE" : "FALSE" ) );
            (*pcprintfLogPatch)( "\t\tlgposLastTerm          = {0x%x,0x%x,0x%x}\r\n", lgposLastTerm.lGeneration, lgposLastTerm.isec, lgposLastTerm.ib );
            (*pcprintfLogPatch)( "\t\tfCreatedLogReader      = %s\r\n", ( fCreatedLogReader ? "TRUE" : "FALSE" ) );
            (*pcprintfLogPatch)( "\t\tfLogToNextLogFile      = %s\r\n", ( fLogToNextLogFile ? "TRUE" : "FALSE" ) );
            (*pcprintfLogPatch)( "\t\tfDoScan                = %s\r\n", ( fDoScan ? "TRUE" : "FALSE" ) );
            (*pcprintfLogPatch)( "\t\tfRecordOkRangeBad      = %s\r\n", ( fRecordOkRangeBad ? "TRUE" : "FALSE" ) );
            (*pcprintfLogPatch)( "\t\tlgposScan              = {0x%x,0x%x,0x%x}\r\n", lgposScan.lGeneration, lgposScan.isec, lgposScan.ib );
            (*pcprintfLogPatch)( "\t\tfJetLog                = %s\r\n", ( fJetLog ? "TRUE" : "FALSE" ) );
            (*pcprintfLogPatch)( "\t\tfTookJump              = %s\r\n", ( fTookJump ? "TRUE" : "FALSE" ) );
            (*pcprintfLogPatch)( "\t\tfSingleSectorTornWrite = %s\r\n", ( fSingleSectorTornWrite ? "TRUE" : "FALSE" ) );
            (*pcprintfLogPatch)( "\t\tfSkipScanAndApplyPatch = %s\r\n", ( fSkipScanAndApplyPatch ? "TRUE" : "FALSE" ) );
            (*pcprintfLogPatch)( "\t\tcbChecksum             = 0x%x\r\n", cbChecksum );

            (*pcprintfLogPatch)( "\r\n\tdumping partial state of LOG:\r\n" );
            (*pcprintfLogPatch)( "\t\tLOG::FRecovering()     = %s\r\n", ( m_pLog->FRecovering() ? "TRUE" : "FALSE" ) );
            (*pcprintfLogPatch)( "\t\tLOG::FRecoveringMode() = %d\r\n", m_pLog->FRecoveringMode() );
            (*pcprintfLogPatch)( "\t\tLOG::FHardRestore()    = %s\r\n", ( m_pLog->FHardRestore() ? "TRUE" : "FALSE" ) );
            (*pcprintfLogPatch)( "\t\tLOG::m_pLogStream->CSecLGFile()      = 0x%08x\r\n", m_pLogStream->CSecLGFile() );
            (*pcprintfLogPatch)( "\t\tLOG::m_csecLGBuf       = 0x%08x\r\n", m_csecLGBuf );
            (*pcprintfLogPatch)( "\t\tLOG::m_csecHeader      = %d\r\n", m_pLogStream->CSecHeader() );
            (*pcprintfLogPatch)( "\t\tLOG::m_cbSec           = %d\r\n", m_pLogStream->CbSec() );
            (*pcprintfLogPatch)( "\t\tLOG::m_cbSecVolume     = %d\r\n", m_pLogStream->CbSecVolume() );
            (*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMin      = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMin ) );
            (*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMax      = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMax ) );
            (*pcprintfLogPatch)( "\t\tLOG::m_pbWrite         = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbWrite ) );
            (*pcprintfLogPatch)( "\t\tLOG::m_pbEntry         = 0x%0*I64x: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n", sizeof( char* ) * 2, QWORD( m_pbEntry ), m_pbEntry[0], m_pbEntry[1], m_pbEntry[2], m_pbEntry[3], m_pbEntry[4], m_pbEntry[5], m_pbEntry[6], m_pbEntry[7] );

            (*pcprintfLogPatch)( "\r\n\tdumping data:\r\n" );

            (*pcprintfLogPatch)( "\t\tplrck (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( plrck ) );
            (*pcprintfLogPatch)( "\t\t  cbBackwards       = 0x%08x\r\n", (ULONG)plrck->le_cbBackwards );
            (*pcprintfLogPatch)( "\t\t  cbForwards        = 0x%08x\r\n", (ULONG)plrck->le_cbForwards );
            (*pcprintfLogPatch)( "\t\t  cbNext            = 0x%08x\r\n", (ULONG)plrck->le_cbNext );
            (*pcprintfLogPatch)( "\t\t  ulChecksum        = 0x%08x\r\n", (ULONG)plrck->le_ulChecksum );
            (*pcprintfLogPatch)( "\t\t  ulShortChecksum   = 0x%08x\r\n", (ULONG)plrck->le_ulShortChecksum );
            (*pcprintfLogPatch)( "\t\t  bUseShortChecksum = %s (0x%02x)\r\n",
                                ( bShortChecksumOn == plrck->bUseShortChecksum ?
                                  "Yes" :
                                  ( bShortChecksumOff == plrck->bUseShortChecksum ?
                                    "No" : "???" ) ),
                                BYTE( plrck->bUseShortChecksum ) );
            (*pcprintfLogPatch)( "\t\tpbEnsure (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbEnsure ) );

            _ibdumpT_ = 0;
            while ( _ibdumpT_ < m_pLogStream->CbSec() )
            {
                (*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                                    _ibdumpT_,
                                    pbEnsure[_ibdumpT_+0],  pbEnsure[_ibdumpT_+1],  pbEnsure[_ibdumpT_+2],  pbEnsure[_ibdumpT_+3],
                                    pbEnsure[_ibdumpT_+4],  pbEnsure[_ibdumpT_+5],  pbEnsure[_ibdumpT_+6],  pbEnsure[_ibdumpT_+7],
                                    pbEnsure[_ibdumpT_+8],  pbEnsure[_ibdumpT_+9],  pbEnsure[_ibdumpT_+10], pbEnsure[_ibdumpT_+11],
                                    pbEnsure[_ibdumpT_+12], pbEnsure[_ibdumpT_+13], pbEnsure[_ibdumpT_+14], pbEnsure[_ibdumpT_+15] );
                _ibdumpT_ += 16;
                Assert( _ibdumpT_ <= m_pLogStream->CbSec() );
            }

            (*pcprintfLogPatch)( "\t\tpbLastSector (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbLastSector ) );

            _ibdumpT_ = 0;
            while ( _ibdumpT_ < m_pLogStream->CbSec() )
            {
                (*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                                    _ibdumpT_,
                                    pbLastSector[_ibdumpT_+0],  pbLastSector[_ibdumpT_+1],  pbLastSector[_ibdumpT_+2],  pbLastSector[_ibdumpT_+3],
                                    pbLastSector[_ibdumpT_+4],  pbLastSector[_ibdumpT_+5],  pbLastSector[_ibdumpT_+6],  pbLastSector[_ibdumpT_+7],
                                    pbLastSector[_ibdumpT_+8],  pbLastSector[_ibdumpT_+9],  pbLastSector[_ibdumpT_+10], pbLastSector[_ibdumpT_+11],
                                    pbLastSector[_ibdumpT_+12], pbLastSector[_ibdumpT_+13], pbLastSector[_ibdumpT_+14], pbLastSector[_ibdumpT_+15] );
                _ibdumpT_ += 16;
                Assert( _ibdumpT_ <= m_pLogStream->CbSec() );
            }

            if ( pbLastChecksum )
            {
                LRCHECKSUM *_plrckT_ = (LRCHECKSUM *)pbLastChecksum;

                (*pcprintfLogPatch)( "\t\tpbLastChecksum (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( _plrckT_ ) );
                (*pcprintfLogPatch)( "\t\t  cbBackwards       = 0x%08x\r\n", (ULONG)_plrckT_->le_cbBackwards );
                (*pcprintfLogPatch)( "\t\t  cbForwards        = 0x%08x\r\n", (ULONG)_plrckT_->le_cbForwards );
                (*pcprintfLogPatch)( "\t\t  cbNext            = 0x%08x\r\n", (ULONG)_plrckT_->le_cbNext );
                (*pcprintfLogPatch)( "\t\t  ulChecksum        = 0x%08x\r\n", (ULONG)_plrckT_->le_ulChecksum );
                (*pcprintfLogPatch)( "\t\t  ulShortChecksum   = 0x%08x\r\n", (ULONG)_plrckT_->le_ulShortChecksum );
                (*pcprintfLogPatch)( "\t\t  bUseShortChecksum = %s (0x%02x)\r\n",
                                    ( bShortChecksumOn == _plrckT_->bUseShortChecksum ?
                                      "Yes" :
                                      ( bShortChecksumOff == _plrckT_->bUseShortChecksum ?
                                        "No" : "???" ) ),
                                    BYTE( _plrckT_->bUseShortChecksum ) );
            }
            else
            {
                (*pcprintfLogPatch)( "\t\tpbLastChecksum (null)\r\n" );
            }

            (*pcprintfLogPatch)( "\r\n" );
        }
#endif







        BYTE        *pbEnsureScan   = NULL;
        BYTE        *pbScan         = NULL;
        ULONG       csecScan        = 0;
        BOOL        fFoundPattern   = fTrue;
        BOOL        fIsTornWrite    = fTrue;
        WCHAR       szSector[30];
        WCHAR       szCorruption[30];
        const WCHAR*    rgpsz[3]        = { m_pLogStream->LogName(), szSector, szCorruption };

        const QWORD ibOffset        = QWORD( lgposCurrent.isec ) * QWORD( m_pLogStream->CbSec() );
        const DWORD cbLength        = cbChecksum;


        OSStrCbFormatW( szSector, sizeof( szSector ), L"%d:%d", lgposCurrent.isec, lgposCurrent.ib );
        szCorruption[0] = 0;


        if ( fSkipScanAndApplyPatch )
            goto ApplyPatch;


        Assert( lgposScan.isec > 0 );


        Assert( lgposScan.ib == 0 );


        Assert( CmpLgpos( &lgposEnd, &lgposScan ) <= 0 );


        if ( lgposScan.isec >= ( m_pLogStream->CSecLGFile() - 1 ) )
        {




            Assert( lgposScan.isec <= ( m_pLogStream->CSecLGFile() - 1 ) );



            goto SkipScan;
        }


        csecScan = m_pLogStream->CSecLGFile() - lgposScan.isec;
        Call( ErrReaderEnsureSector( lgposScan.isec, csecScan, &pbEnsureScan ) );


        Assert( cbLogExtendPattern >= m_pLogStream->CbSec() );
        pbScan = pbEnsureScan;
        while ( fFoundPattern && csecScan )
        {


            if ( _memcmpp( pbScan, rgbLogExtendPattern, cbLogExtendPattern, m_pLogStream->CbSec() ) != 0 )
            {

                fFoundPattern = fFalse;
                fIsTornWrite = fFalse;


                break;
            }


            pbScan += m_pLogStream->CbSec();
            csecScan--;
        }


        fIsTornWrite = fFoundPattern;

#ifdef ENABLE_LOGPATCH_TRACE
        if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
        {
            (*pcprintfLogPatch)( "pattern scan complete -- fFoundPattern = %s\r\n", ( fFoundPattern ? "TRUE" : "FALSE" ) );
        }
#endif


        if ( fFoundPattern && fRecordOkRangeBad )
        {
            BYTE        *pbScanT        = NULL;
            BYTE        *pbEnsureScanT  = NULL;
            ULONG       csecScanT       = 0;
            LRCHECKSUM  *plrckT         = NULL;

#ifdef ENABLE_LOGPATCH_TRACE
            if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
            {
                (*pcprintfLogPatch)( "special case!\r\n" );
                (*pcprintfLogPatch)( "\trecall that we had previously detected a bad LRCK range\r\n" );
                (*pcprintfLogPatch)( "\twe now suspect that this BAD-ness is the result of an incomplete I/O because fFoundPattern=TRUE\r\n" );
            }
#endif

            Assert( !fSingleSectorTornWrite );


            Call( ErrReaderEnsureSector( lgposCurrent.isec, 1, &pbEnsureScanT ) );
            plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsureScanT + lgposCurrent.ib );
            AssertValidLRCKRecord( plrck, &lgposCurrent );


            csecScanT = ( ( lgposCurrent.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards - 1 ) / m_pLogStream->CbSec() ) + 1;
            Assert( csecScanT > 1 );


            Call( ErrReaderEnsureSector( lgposCurrent.isec, csecScanT, &pbEnsureScanT ) );
            plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsureScanT + lgposCurrent.ib );
            AssertValidLRCKRecord( plrck, &lgposCurrent );
            AssertInvalidLRCKRange( plrck, lgposCurrent.lGeneration );

            if ( csecScanT == 2 )
            {


                plrckT = (LRCHECKSUM *)( (BYTE *)plrck + m_pLogStream->CbSec() );
                if ( FValidLRCKShadowWithoutCheckingCBNext( plrckT, &lgposCurrent, lgposCurrent.lGeneration ) )
                {


                    Assert( fJetLog );


                    if ( memcmp( pbEnsureScanT,
                                 pbEnsureScanT + m_pLogStream->CbSec(),
                                 (BYTE *)plrck - pbEnsureScanT ) == 0 &&
                         memcmp( (BYTE *)plrck + sizeof( LRCHECKSUM ),
                                 (BYTE *)plrckT + sizeof( LRCHECKSUM ),
                                 plrckT->le_cbForwards ) == 0 )
                    {


                        Assert( plrckT->le_cbForwards <= plrck->le_cbForwards );

                        Assert( (ULONG32)plrckT->le_cbBackwards == (ULONG32)plrck->le_cbBackwards );


                        Assert( fIsTornWrite );
                    }
                    else
                    {


                        AssertTrack( fFalse, "BadLogShadowSectorValidationLegacy" );


                        fIsTornWrite = fFalse;
                    }
                }
                else if ( _memcmpp( pbEnsureScanT + m_pLogStream->CbSec(), rgbLogExtendPattern, cbLogExtendPattern, m_pLogStream->CbSec() ) == 0 )
                {


                    Assert( fIsTornWrite );
                }
                else
                {


                    fIsTornWrite = fFalse;
                }
            }
            else
            {



                csecScanT--;
                pbScanT = pbEnsureScanT + m_pLogStream->CbSec();
                while ( csecScanT )
                {


                    if ( _memcmpp( pbScanT, rgbLogExtendPattern, cbLogExtendPattern, m_pLogStream->CbSec() ) == 0 )
                    {


                        Assert( fIsTornWrite );

                        break;
                    }

                    csecScanT--;
                    pbScanT += m_pLogStream->CbSec();
                }


                if ( !csecScanT )
                {


                    fIsTornWrite = fFalse;
                }
            }
        }

SkipScan:


        fSingleSectorTornWrite = fSingleSectorTornWrite && fIsTornWrite;

#ifdef ENABLE_LOGPATCH_TRACE
        if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
        {
            (*pcprintfLogPatch)( "torn-write detection complete\r\n" );
            (*pcprintfLogPatch)( "\tfIsTornWrite           = %s\r\n", ( fIsTornWrite ? "TRUE" : "FALSE" ) );
            (*pcprintfLogPatch)( "\tfSingleSectorTornWrite = %s\r\n", ( fSingleSectorTornWrite ? "TRUE" : "FALSE" ) );
        }
#endif

        if ( fIsTornWrite )
        {


            Assert( csecScan == 0 );


            Assert( fFoundPattern );

ApplyPatch:

            if ( m_pLog->FHardRestore() )
            {


                LONG lgenLowRestore, lgenHighRestore;
                m_pLog->LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
                if ( lgposScan.lGeneration <= lgenHighRestore )
                {
#ifdef ENABLE_LOGPATCH_TRACE
                    if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                    {
                        (*pcprintfLogPatch)( "failed to patch torn-write because we are dealing with a logfile in the backup set\r\n" );
                    }
#endif


                    LGIReportChecksumMismatch(
                            m_pinst,
                            m_pLogStream->LogName(),
                            ibOffset,
                            cbLength,
                            ulChecksumExpected,
                            ulChecksumActual,
                            JET_errLogTornWriteDuringHardRestore );


                    Assert( lgposScan.lGeneration >= lgenLowRestore );
                    UtilReportEvent(    eventError,
                                        LOGGING_RECOVERY_CATEGORY,
                                        LOG_TORN_WRITE_DURING_HARD_RESTORE_ID,
                                        2,
                                        rgpsz,
                                        0,
                                        NULL,
                                        m_pinst );

                    Error( ErrERRCheck( JET_errLogTornWriteDuringHardRestore ) );
                }



                if ( !fJetLog || fReadOnlyLocal )
                {
#ifdef ENABLE_LOGPATCH_TRACE
                    if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                    {
                        (*pcprintfLogPatch)( "failed to patch torn-write because %s\r\n",
                                            ( !fJetLog ? "fJetLog==FALSE" : "we are in read-only mode" ) );
                    }
#endif


                    LGIReportChecksumMismatch(
                            m_pinst,
                            m_pLogStream->LogName(),
                            ibOffset,
                            cbLength,
                            ulChecksumExpected,
                            ulChecksumActual,
                            JET_errLogTornWriteDuringHardRecovery );

                    UtilReportEvent(    eventError,
                                        LOGGING_RECOVERY_CATEGORY,
                                        LOG_TORN_WRITE_DURING_HARD_RECOVERY_ID,
                                        2,
                                        rgpsz,
                                        0,
                                        NULL,
                                        m_pinst );

                    Error( ErrERRCheck( JET_errLogTornWriteDuringHardRecovery ) );
                }

                CallS( err );
            }
            else if ( !fReadOnly )
            {



                CallS( err );
            }
            else
            {


                if ( pfIsPatchable )
                {


                    *pfIsPatchable = fTrue;
                    err = JET_errSuccess;
                    goto HandleError;
                }

#ifdef ENABLE_LOGPATCH_TRACE
                if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                {
                    (*pcprintfLogPatch)( "failed to patch torn-write because %s\r\n",
                                         fJetLog ? "we are in read-only mode" : "we are not in the last log file (e.g. edb.jtx)" );
                }
#endif


                LGIReportChecksumMismatch(
                        m_pinst,
                        m_pLogStream->LogName(),
                        ibOffset,
                        cbLength,
                        ulChecksumExpected,
                        ulChecksumActual,
                        JET_errLogFileCorrupt );


                Error( ErrERRCheck( JET_errLogFileCorrupt ) );
            }
        }
        else
        {


            if ( csecScan == 0 )
            {


                Assert( fRecordOkRangeBad );
                OSStrCbFormatW( szCorruption, sizeof( szCorruption ), L"%d", lgposCurrent.isec + 1 );
            }
            else
            {


                Assert( pbScan == m_pLogStream->PbSecAligned( pbScan, m_pbLGBufMin ) );
                Assert( pbEnsureScan == m_pLogStream->PbSecAligned( pbEnsureScan, m_pbLGBufMin ) );
                OSStrCbFormatW( szCorruption, sizeof( szCorruption ), L"%d", lgposScan.isec + (ULONG)( ( pbScan - pbEnsureScan ) / m_pLogStream->CbSec() ) );
            }


            if ( m_pLog->FHardRestore() )
            {

                LONG lgenLowRestore, lgenHighRestore;
                m_pLog->LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
                if ( lgposScan.lGeneration <= lgenHighRestore )
                {
#ifdef ENABLE_LOGPATCH_TRACE
                    if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                    {
                        (*pcprintfLogPatch)( "corruption detected during hard restore (this log is in the backup set)\r\n" );
                    }
#endif


                    LGIReportChecksumMismatch(
                            m_pinst,
                            m_pLogStream->LogName(),
                            ibOffset,
                            cbLength,
                            ulChecksumExpected,
                            ulChecksumActual,
                            JET_errLogCorruptDuringHardRestore );


                    Assert( lgposScan.lGeneration >= lgenLowRestore );
                    UtilReportEvent(    eventError,
                                        LOGGING_RECOVERY_CATEGORY,
                                        LOG_CORRUPTION_DURING_HARD_RESTORE_ID,
                                        3,
                                        rgpsz,
                                        0,
                                        NULL,
                                        m_pinst );

                    Error( ErrERRCheck( JET_errLogCorruptDuringHardRestore ) );
                }

#ifdef ENABLE_LOGPATCH_TRACE
                if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                {
                    (*pcprintfLogPatch)( "corruption detected during hard recovery\r\n" );
                }
#endif


                LGIReportChecksumMismatch(
                        m_pinst,
                        m_pLogStream->LogName(),
                        ibOffset,
                        cbLength,
                        ulChecksumExpected,
                        ulChecksumActual,
                        JET_errLogCorruptDuringHardRecovery );


                UtilReportEvent(    eventError,
                                    LOGGING_RECOVERY_CATEGORY,
                                    LOG_CORRUPTION_DURING_HARD_RECOVERY_ID,
                                    3,
                                    rgpsz,
                                    0,
                                    NULL,
                                    m_pinst );

                Error( ErrERRCheck( JET_errLogCorruptDuringHardRecovery ) );
            }
            else if ( !fReadOnly )
            {

#ifdef ENABLE_LOGPATCH_TRACE
                if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                {
                    (*pcprintfLogPatch)( "corruption detected during soft recovery\r\n" );
                }
#endif


                LGIReportChecksumMismatch(
                        m_pinst,
                        m_pLogStream->LogName(),
                        ibOffset,
                        cbLength,
                        ulChecksumExpected,
                        ulChecksumActual,
                        JET_errLogFileCorrupt );


                UtilReportEvent(    eventError,
                                    LOGGING_RECOVERY_CATEGORY,
                                    LOG_CORRUPTION_DURING_SOFT_RECOVERY_ID,
                                    3,
                                    rgpsz,
                                    0,
                                    NULL,
                                    m_pinst );

                Error( ErrERRCheck( JET_errLogFileCorrupt ) );
            }
            else
            {
#ifdef ENABLE_LOGPATCH_TRACE
                if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                {
                    (*pcprintfLogPatch)( "corruption detected\r\n" );
                }
#endif


                Assert( FNegTest( fCorruptingLogFiles ) || fCreatedLogReader );

                Assert( !m_pLog->FHardRestore() );


                LGIReportChecksumMismatch(
                        m_pinst,
                        m_pLogStream->LogName(),
                        ibOffset,
                        cbLength,
                        ulChecksumExpected,
                        ulChecksumActual,
                        JET_errLogFileCorrupt );


                Error( ErrERRCheck( JET_errLogFileCorrupt ) );
            }
        }
    }
    else
    {


        ULONG       isecScan;
        ULONG       csecScan        = 0;
        BYTE        *pbEnsureScan   = NULL;
        BYTE        *pbScan         = NULL;
        BOOL        fFoundPattern   = fTrue;
        WCHAR       szSector[30];
        WCHAR       szCorruption[30];
        const WCHAR*    rgpsz[3]        = { m_pLogStream->LogName(), szSector, szCorruption };


        OSStrCbCopyW( szSector, sizeof(szSector), L"END" );
        szCorruption[0] = 0;


        Assert( lgposEnd.isec <= ( m_pLogStream->CSecLGFile() - 1 ) );


        isecScan = lgposEnd.isec;
        if ( lgposEnd.ib )
        {
            isecScan++;
        }
        isecScan++;


        if ( isecScan < m_pLogStream->CSecLGFile() )
        {


            Call( ErrReaderEnsureSector( isecScan, m_pLogStream->CSecLGFile() - isecScan, &pbEnsureScan ) );


            Assert( cbLogExtendPattern >= m_pLogStream->CbSec() );
            pbScan = pbEnsureScan;
            csecScan = 0;
            while ( isecScan + csecScan < m_pLogStream->CSecLGFile() )
            {


                if ( _memcmpp( pbScan, rgbLogExtendPattern, cbLogExtendPattern, m_pLogStream->CbSec() ) != 0 )
                {


                    fFoundPattern = fFalse;
                    break;
                }


                pbScan += m_pLogStream->CbSec();
                csecScan++;
            }

            if ( !fFoundPattern )
            {


                OSStrCbFormatW( szCorruption, sizeof( szCorruption ), L"%d (0x%08X)", isecScan + csecScan, isecScan + csecScan );

                Assert( fJetLog || FNegTest( fCorruptingLogFiles ) );

                if ( m_pLog->FHardRestore() )
                {


                    LONG lgenLowRestore, lgenHighRestore;
                    m_pLog->LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
                    if ( lgposScan.lGeneration <= lgenHighRestore )
                    {
#ifdef ENABLE_LOGPATCH_TRACE
                        if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                        {
                            (*pcprintfLogPatch)( "corruption detected during hard restore (this log is in the backup set) -- data was found after the end of the log!\r\n" );
                        }
#endif


                        Assert( lgposScan.lGeneration >= lgenLowRestore );
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

                        Call( ErrERRCheck( JET_errLogCorruptDuringHardRestore ) );
                    }

#ifdef ENABLE_LOGPATCH_TRACE
                    if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                    {
                        (*pcprintfLogPatch)( "corruption detected during hard recovery -- data was found after the end of the log!\r\n" );
                    }
#endif


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

                    Call( ErrERRCheck( JET_errLogCorruptDuringHardRecovery ) );
                }
                else if ( !fReadOnlyLocal )
                {
#ifdef ENABLE_LOGPATCH_TRACE
                    if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                    {
                        (*pcprintfLogPatch)( "corruption detected during soft recovery -- data was found after the end of the log!\r\n" );
                    }
#endif


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

                    Call( ErrERRCheck( JET_errLogFileCorrupt ) );
                }
                else
                {
#ifdef ENABLE_LOGPATCH_TRACE
                    if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                    {
                        (*pcprintfLogPatch)( "corruption detected -- data was found after the end of the log!\r\n" );
                    }
#endif


                    Assert( !m_pLog->FHardRestore() );
                    Assert( fCreatedLogReader );

                    OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"3aa8328c-8b3d-45a1-93c5-4ea6bea70ad4" );
                    Call( ErrERRCheck( JET_errLogFileCorrupt ) );
                }
            }
        }
        else
        {


        }
    }



    lgposCurrent = lgposLast;
    m_pLogStream->AddLgpos( &lgposCurrent, sizeof( LRCHECKSUM ) );


    Call( ErrReaderEnsureSector( lgposLast.isec, 1, &pbEnsure ) );
    plrck = (LRCHECKSUM *)( pbEnsure + lgposLast.ib );
    AssertValidLRCKRecord( plrck, &lgposLast );


    lgposNext = lgposMin;
    if ( plrck->le_cbNext != 0 && !fSkipScanAndApplyPatch )
    {


        lgposNext = lgposLast;
        m_pLogStream->AddLgpos( &lgposNext, sizeof( LRCHECKSUM ) + plrck->le_cbNext );
        Assert( lgposNext.isec > lgposLast.isec );


        Call( ErrReaderEnsureSector( lgposNext.isec, 1, &pbEnsure ) );
        plrck = (LRCHECKSUM *)( pbEnsure + lgposNext.ib );


        if ( !FValidLRCKRecord( plrck, &lgposNext ) || plrck->le_cbBackwards == lgposNext.ib )
        {
            lgposNext.isec = 0;
        }
        else
        {


            Assert( plrck->le_cbBackwards == 0 );

        }
    }


    if ( CmpLgpos( &lgposNext, &lgposEnd ) >= 0 )
    {
        lgposNext.isec = 0;
    }

#ifdef ENABLE_LOGPATCH_TRACE
    if ( pcprintfLogPatch && FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
    {
        (*pcprintfLogPatch)( "scanning the log records in the last LRCK range\r\n" );
        (*pcprintfLogPatch)( "\tlgposCurrent = {0x%x,0x%x,0x%x}\r\n", lgposCurrent.lGeneration, lgposCurrent.isec, lgposCurrent.ib );
        (*pcprintfLogPatch)( "\tlgposEnd     = {0x%x,0x%x,0x%x}\r\n", lgposEnd.lGeneration, lgposEnd.isec, lgposEnd.ib );
        (*pcprintfLogPatch)( "\tlgposNext    = {0x%x,0x%x,0x%x}\r\n", lgposNext.lGeneration, lgposNext.isec, lgposNext.ib  );
    }
#endif



    forever
    {
        BYTE *  pb              = NULL;
        ULONG   cb              = 0;
        LGPOS   lgposEndOfRec   = { 0 };


RestartIterations:


        if ( CmpLgpos( &lgposCurrent, &lgposEnd ) >= 0 )
        {


            goto DoneRecordScan;
        }


        for ( size_t cIteration = 0; cIteration < 3; cIteration++ )
        {
            if ( cIteration == 0 )
            {
                cb = sizeof( LRTYP );
                Assert( sizeof( LRTYP ) == 1 );
            }
            else if ( cIteration == 1 )
            {
                
                if ( reinterpret_cast< const LR * >( pb )->lrtyp >= lrtypMax )
                {
                    AssertSzRTL( fFalse, "Log file is corrupted but not detected by our checksum." );
                    Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                }
                
                cb = CbLGFixedSizeOfRec( reinterpret_cast< const LR * >( pb ) );
            }
            else
            {
                Assert( cIteration == 2 );
                cb = CbLGSizeOfRec( reinterpret_cast< const LR * >( pb ) );
            }


            lgposEndOfRec = lgposCurrent;
            m_pLogStream->AddLgpos( &lgposEndOfRec, cb );


            if ( CmpLgpos( &lgposEndOfRec, &lgposEnd ) > 0 )
            {


                goto DoneRecordScan;
            }


            if ( lgposNext.isec )
            {


                Assert( lgposEndOfRec.isec <= lgposNext.isec );


                if ( lgposEndOfRec.isec == lgposNext.isec )
                {


                    lgposCurrent = lgposNext;


                    lgposNext.isec = 0;


                    goto RestartIterations;
                }
            }


            Assert( cb > 0 );
            Call( ErrReaderEnsureSector( lgposCurrent.isec, ( lgposCurrent.ib + cb - 1 ) / m_pLogStream->CbSec() + 1, &pbEnsure ) );


            pb = pbEnsure + lgposCurrent.ib;


            Assert( *pb >= 0 );
            Assert( *pb < lrtypMax );
        }



        Assert( *pb >= 0 );
        Assert( *pb < lrtypMax );
        Assert( cb == CbLGSizeOfRec( (LR *)pb ) );


        if ( *pb == lrtypTerm || *pb == lrtypTerm2 || *pb == lrtypRecoveryQuit || *pb == lrtypRecoveryQuit2 )
        {

            fGotQuit = fTrue;
            lgposLastTerm = lgposCurrent;
        }
        else if ( *pb == lrtypNOP )
        {
        }
        else if ( *pb == lrtypChecksum )
        {



            lgposLast = lgposCurrent;



            Assert( !fDoScan );


#ifdef DEBUG
            LGPOS lgposT = lgposLast;
            m_pLogStream->AddLgpos( &lgposT, sizeof( LRCHECKSUM ) );
            Assert( CmpLgpos( &lgposT, &lgposEnd ) == 0 );
#endif
        }
        else
        {


            fGotQuit = fFalse;
        }


        lgposCurrent = lgposEndOfRec;
    }

DoneRecordScan:

    if ( fRecordOkRangeBad )
    {


        Assert( !fSingleSectorTornWrite );
        Assert( fDoScan );


        Call( ErrReaderEnsureSector( lgposLast.isec, 1, &pbEnsure ) );
        plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposLast.ib );

        AssertValidLRCKRecord( plrck, &lgposLast );
        Assert( plrck->bUseShortChecksum == bShortChecksumOn );
        Assert( plrck->le_cbBackwards == ULONG( (BYTE *)plrck - m_pLogStream->PbSecAligned( (BYTE *)plrck, m_pbLGBufMin ) )
            || 0 == plrck->le_cbBackwards );
        const UINT cbBackwards      = ULONG( (BYTE *)plrck - m_pLogStream->PbSecAligned( (BYTE *)plrck, m_pbLGBufMin ) );


        const UINT ib = ( lgposCurrent.ib > 0 ) ? lgposCurrent.ib : m_pLogStream->CbSec();


        Assert( ib >= cbBackwards + sizeof( LRCHECKSUM ) );


        Assert( 0 == ( lgposLast.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards ) % m_pLogStream->CbSec() );
        const ULONG csecRange = ( lgposLast.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards ) / m_pLogStream->CbSec();
        if ( csecRange > 2 )
        {
            isecPatternFill = lgposLast.isec + 2;
            csecPatternFill = csecRange - 2;
        }
        else
        {



            Assert( 2 == csecRange );
        }


        plrck->le_cbForwards = ib - ( cbBackwards + sizeof( LRCHECKSUM ) );


        plrck->le_cbNext = 0;


        UtilMemCpy( pbLastSector, pbEnsure, m_pLogStream->CbSec() );


        pbLastChecksum = pbLastSector + cbBackwards;
        isecLastSector = lgposLast.isec;
    }



    Assert( pfCloseNormally );
    *pfCloseNormally = fGotQuit;
    if (plgposLastTerm)
    {
        *plgposLastTerm = ( fGotQuit ) ? lgposLastTerm : lgposMin;
    }


    if ( CmpLgpos( &lgposCurrent, &lgposEnd ) <= 0 )
    {
        m_lgposLastRec = lgposCurrent;
    }
    else
    {
        m_lgposLastRec = lgposEnd;
    }

#ifdef ENABLE_LOGPATCH_TRACE
    if ( pcprintfLogPatch && FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
    {
        (*pcprintfLogPatch)( "finished scanning the last LRCK range\r\n" );
        (*pcprintfLogPatch)( "\tfGotQuit            = %s\r\n", ( fGotQuit ? "TRUE" : "FALSE" ) );
        (*pcprintfLogPatch)( "\t\tlgposLastTerm     = {0x%x,0x%x,0x%x}\r\n", lgposLastTerm.lGeneration, lgposLastTerm.isec, lgposLastTerm.ib );
        (*pcprintfLogPatch)( "\tLOG::m_lgposLastRec = {0x%x,0x%x,0x%x}\r\n", m_lgposLastRec.lGeneration, m_lgposLastRec.isec, m_lgposLastRec.ib );
    }
#endif


    if ( fSingleSectorTornWrite )
    {


        Assert( ( m_lgposLastRec.isec == lgposLast.isec && CmpLgpos( &lgposLast, &m_lgposLastRec ) < 0 ) ||
                ( m_lgposLastRec.isec == lgposLast.isec + 1 && m_lgposLastRec.ib == 0 ) );


        Assert( pbLastChecksum != pbNil );
        plrck = (LRCHECKSUM *)pbLastChecksum;


        const UINT ibT = ( m_lgposLastRec.ib == 0 ) ? m_pLogStream->CbSec() : m_lgposLastRec.ib;
        Assert( ibT >= lgposLast.ib + sizeof( LRCHECKSUM ) );
        plrck->le_cbForwards = ibT - ( lgposLast.ib + sizeof( LRCHECKSUM ) );
        plrck->le_cbNext = 0;
    }

HandleError:



    if ( fCreatedLogReader )
    {
        ERR errT = JET_errSuccess;

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
    else
    {


        LReaderReset();
    }


    if ( err >= 0 && !fReadOnlyLocal && fJetLog )
    {


        CallS( err );


        if ( fLogToNextLogFile )
        {


            if ( fCreatedLogReader )
            {


                m_isecWrite                 = m_pLogStream->CSecHeader();


                m_pbWrite                   = m_pbLGBufMin;
                m_pbLastChecksum            = m_pbLGBufMin;


                m_pbEntry                   = m_pbLastChecksum + sizeof( LRCHECKSUM );


                Assert( pbNil != m_pbLGBufMin );
                memset( m_pbLGBufMin, 0, sizeof( LRCHECKSUM ) );
                plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );
                plrck->lrtyp = lrtypChecksum;


                m_lgposToWrite.isec         = USHORT( m_isecWrite );
                m_lgposToWrite.lGeneration  = m_pLogStream->GetCurrentFileGen() + 1;
                m_lgposToWrite.ib           = 0;


                m_lgposMaxWritePoint        = m_lgposToWrite;


                plrck->bUseShortChecksum = bShortChecksumOff;
                plrck->le_ulShortChecksum = 0;
                plrck->le_ulChecksum = UlComputeChecksum( plrck, m_lgposToWrite.lGeneration );


                AssertValidLRCKRecord( plrck, &m_lgposToWrite );
                AssertValidLRCKRange( plrck, m_lgposToWrite.lGeneration );

#ifdef ENABLE_LOGPATCH_TRACE
                if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                {
                    (*pcprintfLogPatch)( "forcing a new log generation because fLogToNextLogFile=TRUE\r\n" );
                }
#endif

            }
        }
        else if ( pbLastSector != pbNil )
        {
            BYTE    *pbEndOfData, *pbWrite;
            INT     isecWrite;



            Assert( pbNil != m_pbLGBufMin );
            UtilMemCpy( m_pbLGBufMin, pbLastSector, m_pLogStream->CbSec() );


            const UINT cbBackwards      = ULONG( pbLastChecksum - pbLastSector );


            m_pbLastChecksum            = m_pbLGBufMin + cbBackwards;


            plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );


            Assert( isecLastSector >= m_pLogStream->CSecHeader() && isecLastSector <= ( m_pLogStream->CSecLGFile() - 1 ) );

            m_isecWrite                 = isecLastSector;


            m_pbWrite                   = m_pbLGBufMin;


            m_pbEntry                   = m_pbLastChecksum + sizeof( LRCHECKSUM ) + plrck->le_cbForwards;


            m_lgposToWrite.isec         = USHORT( m_isecWrite );
            m_lgposToWrite.lGeneration  = m_pLogStream->GetCurrentFileGen();
            m_lgposToWrite.ib           = 0;
            m_pLogStream->AddLgpos( &m_lgposToWrite, cbBackwards + sizeof( LRCHECKSUM ) + plrck->le_cbForwards );


            m_lgposMaxWritePoint        = m_lgposToWrite;


            plrck->bUseShortChecksum = bShortChecksumOff;
            plrck->le_ulShortChecksum = 0;
            plrck->le_ulChecksum = UlComputeChecksum( plrck, m_pLogStream->GetCurrentFileGen() );


#ifdef DEBUG
            LGPOS lgposChecksum;
            lgposChecksum.ib            = (USHORT)cbBackwards;
            lgposChecksum.isec          = (USHORT)m_isecWrite;
            lgposChecksum.lGeneration   = m_pLogStream->GetCurrentFileGen();
            AssertValidLRCKRecord( (LRCHECKSUM *)m_pbLastChecksum, &lgposChecksum );
            AssertValidLRCKRange( (LRCHECKSUM *)m_pbLastChecksum, lgposChecksum.lGeneration );
#endif


            pbEndOfData                 = m_pbEntry;
            pbWrite                     = m_pbWrite;
            isecWrite                   = m_isecWrite;

#ifdef ENABLE_LOGPATCH_TRACE
            if ( pcprintfLogPatch && FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
            {
                (*pcprintfLogPatch)( "!!DANGER WILL ROBINSON!! we are about to PATCH the last LRCHECKSUM range\r\n" );

                (*pcprintfLogPatch)( "\tisecPatternFill           = 0x%x\r\n", isecPatternFill );
                (*pcprintfLogPatch)( "\tcsecPatternFill           = 0x%x\r\n", csecPatternFill );
                (*pcprintfLogPatch)( "\t\tLOG::m_pbWrite         = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbWrite ) );
                (*pcprintfLogPatch)( "\t\tLOG::m_pbEntry         = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbEntry ) );
                (*pcprintfLogPatch)( "\tLOG::m_lgposToWrite       = {0x%x,0x%x,0x%x}\r\n", m_lgposToWrite.lGeneration, m_lgposToWrite.isec, m_lgposToWrite.ib );
                (*pcprintfLogPatch)( "\tLOG::m_lgposMaxWritePoint = {0x%x,0x%x,0x%x}\r\n", m_lgposMaxWritePoint.lGeneration, m_lgposMaxWritePoint.isec, m_lgposMaxWritePoint.ib );
                (*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMin (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMin ) );

                ULONG _ibdumpT_ = 0;
                while ( _ibdumpT_ < m_pLogStream->CbSec() )
                {
                    (*pcprintfLogPatch)( "\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                                        _ibdumpT_,
                                        m_pbLGBufMin[_ibdumpT_+0],  m_pbLGBufMin[_ibdumpT_+1],  m_pbLGBufMin[_ibdumpT_+2],  m_pbLGBufMin[_ibdumpT_+3],
                                        m_pbLGBufMin[_ibdumpT_+4],  m_pbLGBufMin[_ibdumpT_+5],  m_pbLGBufMin[_ibdumpT_+6],  m_pbLGBufMin[_ibdumpT_+7],
                                        m_pbLGBufMin[_ibdumpT_+8],  m_pbLGBufMin[_ibdumpT_+9],  m_pbLGBufMin[_ibdumpT_+10], m_pbLGBufMin[_ibdumpT_+11],
                                        m_pbLGBufMin[_ibdumpT_+12], m_pbLGBufMin[_ibdumpT_+13], m_pbLGBufMin[_ibdumpT_+14], m_pbLGBufMin[_ibdumpT_+15] );
                    _ibdumpT_ += 16;
                    Assert( _ibdumpT_ <= m_pLogStream->CbSec() );
                }

                LRCHECKSUM *_plrckT_ = (LRCHECKSUM *)m_pbLastChecksum;

                (*pcprintfLogPatch)( "\tLOG::m_pbLastChecksum (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( _plrckT_ ) );
                (*pcprintfLogPatch)( "\t  cbBackwards       = 0x%08x\r\n", (ULONG)_plrckT_->le_cbBackwards );
                (*pcprintfLogPatch)( "\t  cbForwards        = 0x%08x\r\n", (ULONG)_plrckT_->le_cbForwards );
                (*pcprintfLogPatch)( "\t  cbNext            = 0x%08x\r\n", (ULONG)_plrckT_->le_cbNext );
                (*pcprintfLogPatch)( "\t  ulChecksum        = 0x%08x\r\n", (ULONG)_plrckT_->le_ulChecksum );
                (*pcprintfLogPatch)( "\t  ulShortChecksum   = 0x%08x\r\n", (ULONG)_plrckT_->le_ulShortChecksum );
                (*pcprintfLogPatch)( "\t  bUseShortChecksum = %s (0x%02x)\r\n",
                                    ( bShortChecksumOn == _plrckT_->bUseShortChecksum ?
                                      "Yes" :
                                      ( bShortChecksumOff == _plrckT_->bUseShortChecksum ?
                                        "No" : "???" ) ),
                                    BYTE( _plrckT_->bUseShortChecksum ) );
            }
#endif


Assert( m_pLogStream->CbSec() == m_pLogStream->CbSecVolume() );
                if ( 0 != csecPatternFill )
                {
                    ULONG cbPatternFillT;
                    ULONG cbT;


                    Assert( isecPatternFill >= m_pLogStream->CSecHeader() );
                    Assert( isecPatternFill < m_pLogStream->CSecLGFile() - 1 );
                    Assert( isecPatternFill + csecPatternFill <= m_pLogStream->CSecLGFile() - 1 );
                    Assert( fRecordOkRangeBad );


                    cbPatternFillT = 0;
                    while ( cbPatternFillT < csecPatternFill * m_pLogStream->CbSec() )
                    {


                        cbT = min( csecPatternFill * m_pLogStream->CbSec() - cbPatternFillT, cbLogExtendPattern );


                        Assert( isecPatternFill * m_pLogStream->CbSec() + cbPatternFillT + cbT <= ( m_pLogStream->CSecLGFile() - 1 ) * m_pLogStream->CbSec() );
                        err = m_pLogStream->ErrLGWriteSectorData(
                                IOR( iorpPatchFix, iorfFill ),
                                m_lgposLastRec.lGeneration,
                                isecPatternFill * m_pLogStream->CbSec() + cbPatternFillT,
                                cbT,
                                rgbLogExtendPattern,
                                LOG_FLUSH_WRITE_5_ERROR_ID );
                        if ( err < JET_errSuccess )
                        {
                            break;
                        }


                        cbPatternFillT += cbT;
                    }
                }

                if ( err >= JET_errSuccess )
                {
                    err = m_pLogStream->ErrLGWriteSectorData(
                            IOR( iorpPatchFix, iorfFill ),
                            m_lgposLastRec.lGeneration,
                            QWORD( isecWrite ) * m_pLogStream->CbSec(),
                            m_pLogStream->CbSec(),
                            pbWrite,
                            LOG_FLUSH_WRITE_3_ERROR_ID );
                }
                if ( err >= JET_errSuccess )
                {
                    plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );
                    plrck->le_ulChecksum = UlComputeShadowChecksum( plrck->le_ulChecksum );
                    err = m_pLogStream->ErrLGWriteSectorData(
                            IOR( iorpPatchFix, iorfShadow ),
                            m_lgposLastRec.lGeneration,
                            QWORD( isecWrite + 1 ) * m_pLogStream->CbSec(),
                            m_pLogStream->CbSec(),
                            pbWrite,
                            LOG_FLUSH_WRITE_3_ERROR_ID );
                }
                if ( err >= JET_errSuccess )
                {
                    m_pLogBuffer->GetLgpos( pbEndOfData, &m_lgposToWrite, m_pLogStream );
                }
        }
    }



    if ( !fReadOnlyLocal )
    {
        ERR errT;


        Assert( NULL != m_pLogStream->LogName() );
        Assert( L'\0' != m_pLogStream->LogName()[ 0 ] );


        errT = m_pLogStream->ErrLGOpenFile( NULL, fTrue );


        if ( errT != JET_errSuccess )
        {
            errT = ErrERRCheck( errT );
        }


        if ( err == JET_errSuccess )
        {
            err = errT;
        }
    }


    OSMemoryPageFree( pbLastSector );

#ifdef ENABLE_LOGPATCH_TRACE


    if ( pcprintfLogPatch )
    {
        delete pcprintfLogPatch;
    }
#endif


    return err;
}




ERR LOG_READ_BUFFER::ErrLGLocateFirstRedoLogRecFF_Legacy(
    LE_LGPOS    *ple_lgposRedo,
    BYTE        **ppbLR )
{
    ERR         err             = JET_errSuccess;
    LGPOS       lgposCurrent    = { 0, USHORT( m_pLogStream->CSecHeader() ), m_pLogStream->GetCurrentFileGen() };
    LRCHECKSUM  * plrck         = pNil;
    BYTE        * pbEnsure      = pbNil;
    BYTE        * pb            = pbNil;
    LGPOS       lgposPbNext;
    LGPOS       lgposLastChecksum;


    Assert( m_lgposLastRec.isec >= m_pLogStream->CSecHeader() &&
            ( m_lgposLastRec.isec < ( m_pLogStream->CSecLGFile() - 1 ) ||
              ( m_lgposLastRec.isec == ( m_pLogStream->CSecLGFile() - 1 ) && m_lgposLastRec.ib == 0 ) ) &&
            m_lgposLastRec.lGeneration == m_pLogStream->GetCurrentFileGen() );

    LGPOS lgposHack;
    lgposHack.ib            = sizeof( LRCHECKSUM );
    lgposHack.isec          = USHORT( m_pLogStream->CSecHeader() );
    lgposHack.lGeneration   = m_pLogStream->GetCurrentFileGen();
    if ( CmpLgpos( &m_lgposLastRec, &lgposHack ) < 0 )
    {
        Call( errLGNoMoreRecords );
    }

    *ppbLR = pbNil;
    m_pbNext = pbNil;
    m_pbLastChecksum = pbNil;
    m_lgposLastChecksum = lgposMin;

    Assert( ple_lgposRedo->le_isec == m_pLogStream->CSecHeader() );
    Assert( ple_lgposRedo->le_ib == 0 );

    m_lgposLastChecksum.lGeneration = m_pLogStream->GetCurrentFileGen();
    m_lgposLastChecksum.isec        = USHORT( m_pLogStream->CSecHeader() );
    m_lgposLastChecksum.ib          = 0;

    Call( ErrReaderEnsureLogFile() );

    m_fIgnoreNextSectorReadFailure = fFalse;

    Call( ErrReaderEnsureSector( lgposCurrent.isec, 1, &pbEnsure ) );
    m_pbLastChecksum = pbEnsure + lgposCurrent.ib;
    plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );

#ifdef DEBUG

    if ( !FValidLRCKRecord( plrck, &lgposCurrent ) )
    {
        AssertSz( fFalse, "Unexpected invalid LRCK record!" );

        Call( ErrERRCheck( JET_errLogFileCorrupt ) );
    }
#endif

    Call( ErrReaderEnsureSector( lgposCurrent.isec, ( ( lgposCurrent.ib +
        sizeof( LRCHECKSUM ) + plrck->le_cbForwards - 1 ) / m_pLogStream->CbSec() ) + 1,
        &pbEnsure ) );
    m_pbLastChecksum = pbEnsure + lgposCurrent.ib;
    plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );

#ifdef DEBUG

    if ( !FValidLRCKRange( plrck, m_pLogStream->GetCurrentFileGen() ) )
    {

        if ( plrck->bUseShortChecksum != bShortChecksumOn )
        {


            Call( ErrERRCheck( JET_errLogFileCorrupt ) );
        }


        Assert( ( m_lgposLastRec.isec == m_pLogStream->CSecHeader() && m_lgposLastRec.ib >= sizeof( LRCHECKSUM ) ) ||
                ( m_lgposLastRec.isec == m_pLogStream->CSecHeader() + 1 && m_lgposLastRec.ib == 0 ) );
    }
#endif


    Assert( sizeof( LRCHECKSUM ) == CbLGFixedSizeOfRec( reinterpret_cast< const LR* >( pbEnsure ) ) );
    Assert( sizeof( LRCHECKSUM ) == CbLGSizeOfRec( reinterpret_cast< const LR* >( pbEnsure ) ) );


    pb = pbEnsure;
    *ppbLR = m_pbNext = pb;


    CallS( err );

    GetLgposOfPbNext( &lgposPbNext );
    GetLgposOfLastChecksum( &lgposLastChecksum );

    m_pLog->LGPrereadExecute( lgposPbNext, lgposLastChecksum );

    return err;

HandleError:


    if ( err == JET_errLogFileCorrupt )
    {


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

    m_pLog->LGPrereadTerm();

    return err;
}



ERR LOG_READ_BUFFER::ErrLGIGetNextChecksumFF(
    LGPOS       *plgposNextRec,
    BYTE        **ppbLR )
{
    ERR             err = JET_errSuccess;
    LRCHECKSUM*&    plrck = reinterpret_cast< LRCHECKSUM*& >( m_pbLastChecksum );
    LGPOS           lgposNextChecksum;
    UINT            csec;
    BYTE            *pbEnsure;


    Assert( m_lgposLastRec.isec >= m_pLogStream->CSecHeader() );
    Assert( m_lgposLastRec.isec < ( m_pLogStream->CSecLGFile() - 1 ) ||
            ( m_lgposLastRec.isec == ( m_pLogStream->CSecLGFile() - 1 ) && m_lgposLastRec.ib == 0 ) );
    Assert( m_lgposLastRec.lGeneration == m_pLogStream->GetCurrentFileGen() );


    Assert( plgposNextRec->isec >= m_pLogStream->CSecHeader() );
    Assert( plgposNextRec->isec < ( m_pLogStream->CSecLGFile() - 1 ) );
    Assert( ppbLR != pNil );
#ifdef DEBUG
{
    LGPOS lgposPbNext;
    GetLgposOfPbNext( &lgposPbNext );
    Assert( CmpLgpos( &lgposPbNext, plgposNextRec ) == 0 );
}
#endif

    Assert( m_pbLastChecksum != pbNil );


    if ( plrck->le_cbNext == 0 )
    {



        Assert( CmpLgpos( &m_lgposLastRec, plgposNextRec ) >= 0 );
        m_lgposLastRec = *plgposNextRec;


        CallR( ErrERRCheck( JET_errLogFileCorrupt ) );
    }


    lgposNextChecksum = m_lgposLastChecksum;
    m_pLogStream->AddLgpos( &lgposNextChecksum, sizeof( LRCHECKSUM ) + plrck->le_cbNext );



    {
        LGPOS lgposEndOfNextChecksumRec = lgposNextChecksum;
        m_pLogStream->AddLgpos( &lgposEndOfNextChecksumRec, sizeof( LRCHECKSUM ) );


        if ( CmpLgpos( &lgposEndOfNextChecksumRec, &m_lgposLastRec ) > 0 )
        {


            CallR( ErrERRCheck( errLGNoMoreRecords ) );
        }
    }


    csec = lgposNextChecksum.isec + 1 - plgposNextRec->isec;
    Assert( csec > 0 );


    CallR( ErrReaderEnsureSector( plgposNextRec->isec, csec, &pbEnsure ) );


    m_pbLastChecksum = pbEnsure + ( ( csec - 1 ) * m_pLogStream->CbSec() ) + lgposNextChecksum.ib;
    m_lgposLastChecksum = lgposNextChecksum;


    m_pbNext = pbEnsure + plgposNextRec->ib;

#ifdef DEBUG

    if ( !FValidLRCKRecord( plrck, &m_lgposLastChecksum ) )
    {
        AssertSz( fFalse, "Unexpected invalid LRCK record!" );



        m_lgposLastRec = *plgposNextRec;


        CallR( ErrERRCheck( JET_errLogFileCorrupt ) );
    }
#endif



    m_pLogStream->AddLgpos( &lgposNextChecksum, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );
    Assert( ( lgposNextChecksum.isec == m_lgposLastChecksum.isec &&
              lgposNextChecksum.ib   >  m_lgposLastChecksum.ib ) ||
            ( lgposNextChecksum.isec >  m_lgposLastChecksum.isec &&
              lgposNextChecksum.ib   == 0 ) );


    csec = lgposNextChecksum.isec - m_lgposLastChecksum.isec;

    if ( csec > 0 )
    {


        CallR( ErrReaderEnsureSector( plgposNextRec->isec, csec, &pbEnsure ) );


        m_pbLastChecksum = pbEnsure + ( ( m_lgposLastChecksum.isec - plgposNextRec->isec ) * m_pLogStream->CbSec() ) + m_lgposLastChecksum.ib;


        m_pbNext = pbEnsure + plgposNextRec->ib;
    }

#ifdef DEBUG

    if ( !FValidLRCKRange( plrck, m_lgposLastChecksum.lGeneration ) )
    {



        m_lgposLastRec.ib = 0;
        m_lgposLastRec.isec = m_lgposLastChecksum.isec;
        m_lgposLastRec.lGeneration = m_pLogStream->GetCurrentFileGen();


        if ( plrck->bUseShortChecksum == bShortChecksumOn )
        {


            m_lgposLastRec.isec++;
            Assert( m_lgposLastRec.isec < ( m_pLogStream->CSecLGFile() - 1 ) );



            if ( plrck->le_cbBackwards != m_lgposLastChecksum.ib )
                goto NoBackwardRange;
        }


        CallR( ErrERRCheck( JET_errLogFileCorrupt ) );
    }
#endif


    if ( plrck->le_cbBackwards != m_lgposLastChecksum.ib )
    {


        if ( plrck->le_cbNext == 0 )
        {
            m_lgposLastRec = lgposNextChecksum;
        }

#ifdef DEBUG
NoBackwardRange:
#endif


        Assert( plrck->le_cbBackwards == 0 );



        m_pbNext = m_pbLastChecksum;
    }



    CallS( err );
    return err;
}


ERR LOG_READ_BUFFER::ErrLGGetNextRecFF_Legacy(
    BYTE **ppbLR )
{
    ERR         err;
    UINT        cbAvail, cbNeed, cIteration;
    LRCHECKSUM  *plrck;
    LGPOS       lgposPbNext, lgposPbNextNew, lgposChecksumEnd;
    BOOL        fDidRead;


    err         = JET_errSuccess;
    fDidRead    = fFalse;


    Assert( m_lgposLastRec.isec >= m_pLogStream->CSecHeader() &&
            ( m_lgposLastRec.isec < ( m_pLogStream->CSecLGFile() - 1 ) ||
              ( m_lgposLastRec.isec == ( m_pLogStream->CSecLGFile() - 1 ) && m_lgposLastRec.ib == 0 ) ) &&
            m_lgposLastRec.lGeneration == m_pLogStream->GetCurrentFileGen() );



    Assert( m_pbNext != pbNil );
    GetLgposOfPbNext( &lgposPbNextNew );


    lgposPbNext = lgposPbNextNew;



    AssertSz( ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) < 0 ),
              "We somehow managed to replay garbage, and now, when we are moving past the garbage we just played, we are detecting it for the first time!?! Woops..." );


    if ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) >= 0 )
    {


        Call( ErrERRCheck( errLGNoMoreRecords ) );
    }


    cbNeed = CbLGSizeOfRec( reinterpret_cast< const LR* >( m_pbNext ) );
    Assert( cbNeed > 0 );


    m_pbNext += cbNeed;


    Assert( m_pbNext > m_pbLGBufMin && m_pbNext < m_pbLGBufMax );
    if ( m_pbNext <= m_pbLGBufMin || m_pbNext >= m_pbLGBufMax )
    {
        Call( ErrERRCheck( JET_errLogFileCorrupt ) );
    }


    GetLgposOfPbNext( &lgposPbNextNew );


    Assert( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) > 0 );
    if ( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) <= 0 )
    {
        Call( ErrERRCheck( JET_errLogFileCorrupt ) );
    }
    

    Assert( m_pLogStream->CbOffsetLgpos( lgposPbNextNew, lgposPbNext ) == QWORD( cbNeed ) );
    if ( m_pLogStream->CbOffsetLgpos( lgposPbNextNew, lgposPbNext ) != QWORD( cbNeed ) )
    {
        Call( ErrERRCheck( JET_errLogFileCorrupt ) );
    }
    

    if ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) >= 0 )
    {
        Call( ErrERRCheck( errLGNoMoreRecords ) );
    }


    Assert( m_pbLastChecksum != pbNil );
    plrck = (LRCHECKSUM *)m_pbLastChecksum;


    lgposChecksumEnd = m_lgposLastChecksum;
    m_pLogStream->AddLgpos( &lgposChecksumEnd, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );


    Assert( CmpLgpos( &lgposChecksumEnd, &lgposPbNextNew ) >= 0 );
    cbAvail = (UINT)m_pLogStream->CbOffsetLgpos( lgposChecksumEnd, lgposPbNextNew );


    lgposPbNext = lgposPbNextNew;


    for ( cIteration = 0; cIteration < 3; cIteration++ )
    {


        if ( cIteration == 0 )
        {


            Assert( sizeof( LRTYP ) == 1 );
            cbNeed = sizeof( LRTYP );
        }
        else if ( cIteration == 1 )
        {


            cbNeed = CbLGFixedSizeOfRec( reinterpret_cast< const LR * >( m_pbNext ) );
        }
        else
        {


            Assert( cIteration == 2 );
            cbNeed = CbLGSizeOfRec( reinterpret_cast< const LR * >( m_pbNext ) );
        }


        lgposPbNextNew = lgposPbNext;
        m_pLogStream->AddLgpos( &lgposPbNextNew, cbNeed );


        if ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) > 0 )
        {
            Call( ErrERRCheck( errLGNoMoreRecords ) );
        }


        if ( cbAvail < cbNeed )
        {



            Assert( !fDidRead );


            err = ErrLGIGetNextChecksumFF( &lgposPbNext, ppbLR );
            fDidRead = fTrue;



            Assert( m_pbNext > m_pbLGBufMin && m_pbNext < m_pbLGBufMax );
            if ( m_pbNext <= m_pbLGBufMin || m_pbNext >= m_pbLGBufMax )
            {
                Call( ErrERRCheck( JET_errLogFileCorrupt ) );
            }


            GetLgposOfPbNext( &lgposPbNextNew );


            Assert( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) >= 0 );
            if ( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) < 0 )
            {
                Call( ErrERRCheck( JET_errLogFileCorrupt ) );
            }


            if ( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) > 0 )
            {



                CallS( err );


                goto HandleError;
            }


            if ( err == JET_errLogFileCorrupt )
            {



                Assert( CmpLgpos( &m_lgposLastRec, &lgposChecksumEnd ) == 0 ||
                        ( m_lgposLastRec.ib == lgposChecksumEnd.ib &&
                          m_lgposLastRec.isec == lgposChecksumEnd.isec + 1 &&
                          m_lgposLastRec.lGeneration == lgposChecksumEnd.lGeneration ) );


                m_pLogStream->AddLgpos( &lgposChecksumEnd, m_pLogStream->CbSec() );
                if ( CmpLgpos( &m_lgposLastRec, &lgposChecksumEnd ) == 0 )
                {



                    err = JET_errSuccess;
                    goto GotMoreData;
                }

                OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"d65b6db2-ef29-4371-a518-ea0d96b0b42c" );
                Call( err );
            }
            else if ( err != JET_errSuccess )
            {


                Assert( err < 0 );
                Call( err );
            }

GotMoreData:



            Assert( m_pbLastChecksum != pbNil );
            plrck = (LRCHECKSUM *)m_pbLastChecksum;


            if ( CmpLgpos( &lgposChecksumEnd, &m_lgposLastChecksum ) <= 0 )
            {
                lgposChecksumEnd = m_lgposLastChecksum;
                m_pLogStream->AddLgpos( &lgposChecksumEnd, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );
            }


            cbAvail = (UINT)m_pLogStream->CbOffsetLgpos( m_lgposLastChecksum, lgposPbNextNew );


            if ( cbAvail == 0 )
            {


                Assert( *m_pbNext == lrtypChecksum );


                Assert( cIteration == 0 );


                Assert( lgposPbNext.ib == 0 );
                Assert( m_pbNext == m_pLogStream->PbSecAligned( m_pbNext, m_pbLGBufMin ) );


                cbAvail = sizeof( LRCHECKSUM );
            }


            Assert( cbAvail >= cbNeed );
        }

    }


    Assert( cIteration == 3 );


    Assert( cbAvail >= CbLGSizeOfRec( reinterpret_cast< const LR * >( m_pbNext ) ) );


    CallS( err );

HandleError:


    *ppbLR = m_pbNext;


    if ( err == JET_errLogFileCorrupt )
    {


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


ULONG32 LOG_READ_BUFFER::UlComputeChecksum( const LRCHECKSUM* const plrck, const ULONG32 lGeneration )
{
    ULONG32 ulChecksum = ulLRCKChecksumSeed;
    const BYTE* pb = pbNil;
    Assert( NULL != plrck );
    const BYTE* pbEnd = pbNil;


    Assert( reinterpret_cast< const BYTE* >( plrck ) >= m_pbLGBufMin &&
        reinterpret_cast< const BYTE* >( plrck ) < m_pbLGBufMax );

    pb = reinterpret_cast< const BYTE* >( plrck ) - plrck->le_cbBackwards;
    pbEnd = reinterpret_cast< const BYTE* >( plrck );

    Assert( pb >= m_pbLGBufMin && pb < m_pbLGBufMax );
    Assert( pbEnd >= m_pbLGBufMin && pbEnd < m_pbLGBufMax );

    Assert( ( pbEnd - pb == 0 ) ? fTrue : m_pLogBuffer->FIsUsedSpace( pb, ULONG( pbEnd - pb ) ) );
    Assert( pbEnd >= pb );

    ulChecksum = LGChecksum::UlChecksumBytes( pb, pbEnd, ulChecksum );

    pb = reinterpret_cast< const BYTE* >( plrck ) + sizeof( *plrck );
    pbEnd = reinterpret_cast< const BYTE* >( plrck )
        + sizeof( *plrck ) + plrck->le_cbForwards;

    Assert( pb >= ( m_pbLGBufMin + sizeof( *plrck ) ) );
    Assert( pb <= m_pbLGBufMax );
    Assert( pbEnd > m_pbLGBufMin && pbEnd <= ( m_pbLGBufMax + m_cbLGBuf ) );

    Assert( ( pbEnd - pb == 0 ) ? fTrue
                                : m_pLogBuffer->FIsUsedSpace( ( pb == m_pbLGBufMax )    ? m_pbLGBufMin
                                                                        : pb,
                                                ULONG( pbEnd - pb ) ) );
    Assert( pbEnd >= pb );

    ulChecksum = LGChecksum::UlChecksumBytes( pb, pbEnd, ulChecksum );

    ulChecksum ^= plrck->le_cbBackwards;
    ulChecksum ^= plrck->le_cbForwards;
    ulChecksum ^= plrck->le_cbNext;

    Assert( plrck->bUseShortChecksum == bShortChecksumOn ||
            plrck->bUseShortChecksum == bShortChecksumOff );
    ulChecksum ^= (ULONG32)plrck->bUseShortChecksum;

    ulChecksum ^= lGeneration;

    return ulChecksum;
}


ULONG32 LOG_READ_BUFFER::UlComputeShadowChecksum( const ULONG32 ulOriginalChecksum )
{
    return ulOriginalChecksum ^ ulShadowSectorChecksum;
}


ULONG32 LOG_READ_BUFFER::UlComputeShortChecksum( const LRCHECKSUM* const plrck, const ULONG32 lGeneration )
{
    ULONG32 ulChecksum = ulLRCKShortChecksumSeed;
    const BYTE* pb = pbNil;
    Assert( NULL != plrck );
    const BYTE* pbEnd = pbNil;

    Assert( reinterpret_cast< const BYTE* >( plrck ) >= m_pbLGBufMin &&
        reinterpret_cast< const BYTE* >( plrck ) < m_pbLGBufMax );

    pb = reinterpret_cast< const BYTE* >( plrck ) - plrck->le_cbBackwards;
    Assert( m_pLogStream->PbSecAligned( const_cast< BYTE* >( reinterpret_cast< const BYTE* >( plrck ) ), m_pbLGBufMin ) == pb || 0 == plrck->le_cbBackwards );
    pbEnd = reinterpret_cast< const BYTE* >( plrck );

    Assert( pb >= m_pbLGBufMin && pb < m_pbLGBufMax );
    Assert( pbEnd >= m_pbLGBufMin && pbEnd < m_pbLGBufMax );

    Assert( pbEnd >= pb );

    ulChecksum = LGChecksum::UlChecksumBytes( pb, pbEnd, ulChecksum );

    pb = reinterpret_cast< const BYTE* >( plrck ) + sizeof( *plrck );
    pbEnd = m_pLogStream->PbSecAligned( const_cast< BYTE* >( reinterpret_cast< const BYTE* >( plrck ) ), m_pbLGBufMin ) + m_pLogStream->CbSec();

    Assert( pb >= ( m_pbLGBufMin + sizeof( *plrck ) ) );
    Assert( pb <= m_pbLGBufMax );
    Assert( pbEnd > m_pbLGBufMin && pbEnd <= m_pbLGBufMax );

    Assert( pbEnd >= pb );

    ulChecksum = LGChecksum::UlChecksumBytes( pb, pbEnd, ulChecksum );

    ulChecksum ^= plrck->le_cbBackwards;
    ulChecksum ^= plrck->le_cbForwards;
    ulChecksum ^= plrck->le_cbNext;

    Assert( plrck->bUseShortChecksum == bShortChecksumOn ||
            plrck->bUseShortChecksum == bShortChecksumOff );
    ulChecksum ^= (ULONG32)plrck->bUseShortChecksum;

    ulChecksum ^= lGeneration;

    return ulChecksum;
}

#endif

VOID
CHECKSUMINCREMENTAL::ChecksumBytes( const BYTE* const pbMin, const BYTE* const pbMax )
{
    const UINT      cbitsPerBytes = 8;
    const ULONG32   ulChecksum = LGChecksum::UlChecksumBytes( pbMin, pbMax, 0 );
    const ULONG32   ulChecksumAdjusted = _rotl( ulChecksum, m_cLeftRotate );
    m_cLeftRotate = ( ULONG( ( pbMax - pbMin ) % sizeof( ULONG32 ) ) * cbitsPerBytes + m_cLeftRotate ) % ( sizeof( ULONG32 ) * cbitsPerBytes );

    m_ulChecksum ^= ulChecksumAdjusted;
}

ULONG32 LGChecksum::UlChecksumBytes( const BYTE * const pbMin, const BYTE * const pbMax, const ULONG32 ulSeed )
{
    Assert( pbMin <= pbMax );

    if ( pbMin == pbMax )
        return ulSeed;

    const NATIVE_WORD cBitsInByte = 8;
    const INT cLoop = 16; 


    const NATIVE_WORD * pwMin = (NATIVE_WORD *)(DWORD_PTR) ( ( (NATIVE_WORD)pbMin / sizeof( NATIVE_WORD ) ) * sizeof( NATIVE_WORD ) );

    const NATIVE_WORD * const pwMax = (NATIVE_WORD *)(DWORD_PTR) ( ( ( (NATIVE_WORD)pbMax + sizeof( NATIVE_WORD ) - 1 ) / sizeof( NATIVE_WORD ) ) * sizeof( NATIVE_WORD ) );

    Assert( pwMin < pwMax );

    const NATIVE_WORD cbitsUnusedLastWord = ( sizeof( NATIVE_WORD ) - ( (NATIVE_WORD)pbMax % sizeof( NATIVE_WORD ) ) ) * cBitsInByte * ( (NATIVE_WORD)pwMax != (NATIVE_WORD)pbMax );
    Assert( cbitsUnusedLastWord < sizeof( NATIVE_WORD ) * cBitsInByte );
    const NATIVE_WORD wByteMaskLast = ~( (NATIVE_WORD)(~0) >> cbitsUnusedLastWord );

    const NATIVE_WORD cbitsUsedFirstWord = ( sizeof( NATIVE_WORD ) - ( (NATIVE_WORD)pbMin % sizeof( NATIVE_WORD ) ) ) * cBitsInByte;
    Assert( cbitsUsedFirstWord > 0 );

    const NATIVE_WORD wByteMaskFirst    = (NATIVE_WORD)(~0) >> ( cbitsUsedFirstWord - 1 ) >> 1;
    const NATIVE_WORD wFirst            = *(LittleEndian<NATIVE_WORD>*)pwMin;
    const NATIVE_WORD wMaskFirst        = wByteMaskFirst & wFirst;

    NATIVE_WORD wChecksum = 0;
    NATIVE_WORD wChecksumT = 0;

#pragma warning( suppress : 6305 )
    const ULONG cw = ULONG( pwMax - pwMin );
#pragma warning( suppress : 6305 )
    pwMin -= ( ( cLoop - ( cw % cLoop ) ) & ( cLoop - 1 ) );


    if( 8 == cLoop )
    {
        switch ( cw % cLoop )
        {
            while ( 1 )
            {
                case 0:
                    wChecksum  ^= pwMin[0];
                case 7:
                    wChecksumT ^= pwMin[1];
                case 6:
                    wChecksum  ^= pwMin[2];
                case 5:
                    wChecksumT ^= pwMin[3];
                case 4:
                    wChecksum  ^= pwMin[4];
                case 3:
                    wChecksumT ^= pwMin[5];
                case 2:
                    wChecksum  ^= pwMin[6];
                case 1:
                    wChecksumT ^= pwMin[7];
                    pwMin += cLoop;
                    if( pwMin >= pwMax )
                    {
                        goto EndLoop;
                    }
            }
        }
    }
    else if ( 16 == cLoop )
    {
        switch ( cw % cLoop )
        {
            while ( 1 )
            {
                case 0:
                    wChecksum  ^= pwMin[0];
                case 15:
                    wChecksumT ^= pwMin[1];
                case 14:
                    wChecksum  ^= pwMin[2];
                case 13:
                    wChecksumT ^= pwMin[3];
                case 12:
                    wChecksum  ^= pwMin[4];
                case 11:
                    wChecksumT ^= pwMin[5];
                case 10:
                    wChecksum  ^= pwMin[6];
                case 9:
                    wChecksumT ^= pwMin[7];
                case 8:
                    wChecksum  ^= pwMin[8];
                case 7:
                    wChecksumT ^= pwMin[9];
                case 6:
                    wChecksum  ^= pwMin[10];
                case 5:
                    wChecksumT ^= pwMin[11];
                case 4:
                    wChecksum  ^= pwMin[12];
                case 3:
                    wChecksumT ^= pwMin[13];
                case 2:
                    wChecksum  ^= pwMin[14];
                case 1:
                    wChecksumT ^= pwMin[15];
                    pwMin += cLoop;
                    if( pwMin >= pwMax )
                    {
                        goto EndLoop;
                    }
            }
        }
    }

EndLoop:
    wChecksum ^= wChecksumT;


    wChecksum = ReverseBytesOnBE( wChecksum );


    wChecksum ^= wMaskFirst;

    const NATIVE_WORD wLast         = *((LittleEndian<NATIVE_WORD>*)pwMax-1);
    const NATIVE_WORD wMaskLast     = wByteMaskLast & wLast;
    wChecksum ^= wMaskLast;

    ULONG32 ulChecksum;

    if( sizeof( ulChecksum ) != sizeof( wChecksum ) )
    {
        Assert( sizeof( NATIVE_WORD ) == sizeof( ULONG64 ) );
        const NATIVE_WORD wUpper = ( wChecksum >> ( sizeof( NATIVE_WORD ) * cBitsInByte / 2 ) );
        const NATIVE_WORD wLower = wChecksum & 0x00000000FFFFFFFF;
        Assert( wUpper == ( wUpper & 0x00000000FFFFFFFF ) );
        Assert( wLower == ( wLower & 0x00000000FFFFFFFF ) );

        wChecksum = wUpper ^ wLower;
        Assert( wChecksum == ( wChecksum & 0x00000000FFFFFFFF ) );
    }
    else
    {
        Assert( sizeof( NATIVE_WORD ) == sizeof( ULONG32 ) );
    }
    ulChecksum = ULONG32( wChecksum );

    ulChecksum = _rotl( ulChecksum, ULONG( cbitsUsedFirstWord ) );

    ulChecksum ^= ulSeed;

    return ulChecksum;
}


#ifndef RTM

ULONG32 LGChecksum::UlChecksumBytesNaive( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed )
{
    const INT cBitsInByte = 8;

    ULONG32 ulChecksum = ulSeed;

    INT cul = INT( pbMax - pbMin ) / 4;
    const Unaligned< ULONG32 >* pul             = (Unaligned< ULONG32 >*)pbMin;
    const Unaligned< ULONG32 >* const pulMax    = pul + cul;
    while( pul < pulMax )
    {
        ulChecksum ^= *pul++;
    }

    const BYTE * pb = (BYTE *)(pulMax);
    if ( FHostIsLittleEndian() )
    {
        INT ib = 0;
        while( pb < pbMax )
        {
            const BYTE b    = *pb;
            const ULONG32 w = b;
            ulChecksum ^= ( w << ( ib * cBitsInByte ) );
            ++ib;
            Assert( ib < 4 );
            ++pb;
        }
    }
    else
    {
        INT ib = 3;
        while( pb < pbMax )
        {
            const BYTE b    = *pb;
            const ULONG32 w = b;
            ulChecksum ^= ( w << ( ib * cBitsInByte ) );
            --ib;
            Assert( ib >= 0 );
            ++pb;
        }

        ulChecksum = ReverseBytes( ulChecksum );
    }

    return ulChecksum;
}


ULONG32 LGChecksum::UlChecksumBytesSlow( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed )
{
    const INT cBitsInByte = 8;

    ULONG32 ulChecksum = ulSeed;

    INT ib = 0;
    const BYTE * pb = pbMin;
    while( pb < pbMax )
    {
        const BYTE b    = *pb;
        const ULONG32 w = b;
        ulChecksum ^= ( w << ( ib * cBitsInByte ) );
        ib++;
        ib %= 4;
        ++pb;
    }

    return ulChecksum;
}


BOOL LGChecksum::TestChecksumBytesIteration( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed )
{
    const ULONG32 ulChecksum        = UlChecksumBytes( pbMin, pbMax, ulSeed );
    const ULONG32 ulChecksumSlow    = UlChecksumBytesSlow( pbMin, pbMax, ulSeed );
    const ULONG32 ulChecksumNaive   = UlChecksumBytesNaive( pbMin, pbMax, ulSeed );

    CHECKSUMINCREMENTAL ck;
    ck.BeginChecksum( ulSeed );
    ck.ChecksumBytes( pbMin, pbMax );
    const ULONG32 ulChecksumIncremental = ck.EndChecksum();

    const BYTE* const pbMid = pbMin + ( ( pbMax - pbMin ) + 2 - 1 ) / 2;
    ck.BeginChecksum( ulSeed );
    ck.ChecksumBytes( pbMin, pbMid );
    ck.ChecksumBytes( pbMid, pbMax );
    const ULONG32 ulChecksumIncremental2 = ck.EndChecksum();

    const BYTE* const pbMax1 = pbMin + ( ( pbMax - pbMin ) + 3 - 1 ) / 3;
    const BYTE* const pbMax2 = pbMin + ( 2 * ( pbMax - pbMin ) + 3 - 1 ) / 3;
    ck.BeginChecksum( ulSeed );
    ck.ChecksumBytes( pbMin, pbMax1 );
    ck.ChecksumBytes( pbMax1, pbMax2 );
    ck.ChecksumBytes( pbMax2, pbMax );
    const ULONG32 ulChecksumIncremental3 = ck.EndChecksum();

    return ( ulChecksum == ulChecksumSlow &&
             ulChecksum == ulChecksumNaive &&
             ulChecksum == ulChecksumIncremental &&
             ulChecksum == ulChecksumIncremental2 &&
             ulChecksum == ulChecksumIncremental3 );
}


LOCAL BYTE const rgbTestChecksum[] = {
    0x80, 0x23, 0x48, 0x04, 0x1B, 0x13, 0xA0, 0x03, 0x78, 0x55, 0x4C, 0x54, 0x88, 0x18, 0x4B, 0x63,
    0x98, 0x05, 0x4B, 0xC7, 0x36, 0xE5, 0x12, 0x00, 0xB1, 0x90, 0x1F, 0x02, 0x85, 0x05, 0xE5, 0x04,
    0x80, 0x55, 0x4C, 0x04, 0x00, 0x31, 0x60, 0x89, 0x01, 0x5E, 0x39, 0x66, 0x80, 0x55, 0x12, 0x98,
    0x18, 0x00, 0x00, 0x00, 0x48, 0xE5, 0x12, 0x00, 0x9C, 0x65, 0x1B, 0x04, 0x80, 0x55, 0x4C, 0x04,

    0x60, 0xE5, 0x12, 0x00, 0xDF, 0x7F, 0x1F, 0x04, 0x80, 0x55, 0x4C, 0x04, 0x00, 0x72, 0x91, 0x45,
    0x80, 0x23, 0x48, 0x04, 0x1B, 0x13, 0xA0, 0x03, 0x78, 0x55, 0x4C, 0x54, 0x88, 0x18, 0x4B, 0x63,
    0x98, 0x05, 0x4B, 0xC7, 0x36, 0xE5, 0x12, 0x00, 0xB1, 0x90, 0x1F, 0x02, 0x85, 0x05, 0xE5, 0x04,
    0x80, 0x55, 0x4C, 0x04, 0x00, 0x31, 0x60, 0x89, 0x01, 0x5E, 0x39, 0x66, 0x80, 0x55, 0x12, 0x98

};


BOOL LGChecksum::TestChecksumBytes()
{

    INT i;
    INT j;
    for( i = 0; i < sizeof( rgbTestChecksum ) - 1; ++i )
    {
        for( j = i + 1; j < sizeof( rgbTestChecksum ); ++j )
        {
            if ( !TestChecksumBytesIteration( rgbTestChecksum + i, rgbTestChecksum + j, j ) )
                return fFalse;
        }
    }
    return fTrue;
}


#endif

