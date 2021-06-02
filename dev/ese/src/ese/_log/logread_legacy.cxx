// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "logstd.hxx"

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT

#if defined( DEBUG ) || !defined( RTM )

//  all cases except !DEBUG && RTM

//  enabling this generates logpatch.txt
//  it contains trace information for ErrLGCheckReadLastLogRecordFF

#define ENABLE_LOGPATCH_TRACE

#endif  //  !DEBUG || RTM


//  XXX
//  If you modify this function, be sure to modify the class VERIFYCHECKSUM, especially
//  ErrVerifyLRCKStart.

//  verifies the supposedly valid LRCHECKSUM record
//  if there is a short checksum (bUseShortChecksum == bShortChecksumOn),
//      then we verify the whole sector with the short checksum value -- which in turn verifies the
//      bUseShortChecksum byte
//  if there was not a short checksum, the short checksum value is ignored
//  from this point, other heuristics are employed to see if the pointers in the LRCHECKSUM record look good
//      enough to use
//
//  THIS FUNCTION DOES NOT GUARANTEE ANYTHING ABOUT THE RANGE OF THE LRCHECKSUM RECORD!
//  while it MAY verify the first sector of the range using the short checksum, it does not guarantee the rest
//      of the data
//
//  NOTE: this assumes the entire sector containing the LRCHECKSUM record is in memory

#ifdef DEBUG
LOCAL BOOL g_fLRCKValidationTrap = fFalse;
#endif

BOOL LOG_READ_BUFFER::FValidLRCKRecord(
    // LRCHECKSUM to validate
    const LRCHECKSUM * const plrck,
    // LGPOS of this record
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

    //  verify the type as lrtypChecksum

    if ( lrtypChecksum != plrck->lrtyp )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    //  the backward range cannot exceed the length of a full sector (minus the size of the LRCHECKSUM record)

    if ( plrck->le_cbBackwards > ( m_pLogStream->CbSec() - sizeof( LRCHECKSUM ) ) )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    //  is the short checksum in use?

    if ( plrck->bUseShortChecksum == bShortChecksumOn )
    {
        //  verify the short checksum

        if ( !FValidLRCKShortChecksum( plrck, plgpos->lGeneration ) )
        {
            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }

        //  since there was a short checksum, this must be a multi-sector flush

        fMultisector = fTrue;
    }
    else if ( plrck->bUseShortChecksum == bShortChecksumOff )
    {

        //  this verifies that the bUseShortChecksum byte was set to a known value and stayed that way

        if ( plrck->le_ulShortChecksum != 0 )
        {
            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }

        //  since there was no short checksum, this must be a single-sector flush

        fMultisector = fFalse;
    }
    else
    {

        //  bUseShortChecksum is corrupt

        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    //  the backward pointer must land exactly on the start of the sector OR be 0

    const BYTE* const pbRegionStart = reinterpret_cast< const BYTE* >( plrck ) - plrck->le_cbBackwards;
    if ( m_pLogStream->PbSecAligned( const_cast< BYTE* >( pbRegionStart ), m_pbLGBufMin ) != pbRegionStart &&
        0 != plrck->le_cbBackwards )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    //  the forward range cannot exceed the length of the file

    lgpos = *plgpos;
    m_pLogStream->AddLgpos( &lgpos, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );
    Call( m_pLogStream->ErrFileSize( &qwSize ) );
    Assert( ( qwSize % (QWORD)m_pLogStream->CbSec() ) == 0 );
    if ( ( ( (QWORD)lgpos.isec * (QWORD)m_pLogStream->CbSec() ) + (QWORD)lgpos.ib ) > qwSize )
    {
        //  if this assert happens, it means that the short checksum value was tested and came out OK
        //  thus, the sector with data (in which plrck resides) must be OK
        //  however, the forward pointer in plrck stretched beyond the end of the file???
        //  this can happen 1 of 2 ways:
        //      we set it up wrong (most likely)
        //      it was mangled in such a way that it passed the checksum (practically impossible)
        AssertSz( !fMultisector, "We have a verified short checksum value and the forward pointer is still wrong!" );

        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    //  calculate the offset of the LRCHECKSUM record from the nearest sector boundary in memory

    ib = ULONG( reinterpret_cast< const BYTE* >( plrck ) - m_pLogStream->PbSecAligned( reinterpret_cast< const BYTE* >( plrck ), m_pbLGBufMin ) );
    Assert( ib == plgpos->ib );

    //  the backwards pointer in the LRCHECKSUM record must be either equal to ib or 0

    if ( plrck->le_cbBackwards != 0 && plrck->le_cbBackwards != ib )
    {

        //  the backward pointer is wrong

        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    //  calculate the range using the backward pointer instead of 'ib'
    //  NOTE: le_cbBackwards can be 0 here making this range calculation fall short of the real thing;
    //        having the range too short doesn't matter here because we only do tests if we see that
    //        the range is a multi-sector range; nothing happens if we decide that this is a single-sector
    //        range (e.g. being too short makes it appear < 1 sector)

    cbRange = plrck->le_cbBackwards + sizeof( LRCHECKSUM ) + plrck->le_cbForwards;
    if ( cbRange > m_pLogStream->CbSec() )
    {

        //  the range covers multiple sectors

        //  we should have a short checksum if this happens (causing fMultisector == fTrue)

        if ( !fMultisector )
        {
            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }

        //  also, multi-sector flushes must have a range that is sector-granular so that the forward
        //      pointer goes right up to the end of the last sector in the range

        if ( ( cbRange % m_pLogStream->CbSec() ) != 0 )
        {

            //  the only way we could get a range that is not sector granular is when the
            //      backward pointer is 0

            if ( plrck->le_cbBackwards != 0 )
            {

                //  woops -- the backward pointer is not what we expect

                Assert( !g_fLRCKValidationTrap );
                return fFalse;
            }
        }
    }


    //  re-test the range using the offset of the plrck pointer's alignment rather than the
    //      backward pointer because the backward pointer could be 0

    cbRange = ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards;
    if ( cbRange > m_pLogStream->CbSec() )
    {

        //  the range covers multiple sectors

        //  we should have a short checksum if this happens (causing fMultisector == fTrue)

        if ( !fMultisector )
        {
            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }

        //  also, multi-sector flushes must have a range that is sector-granular so that the forward
        //      pointer goes right up to the end of the last sector in the range

        if ( ( cbRange % m_pLogStream->CbSec() ) != 0 )
        {

            //  woops, the range is not granular

            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }
    }
    else
    {

        //  the range is 1 sector or less

        //  we should have no short checksum if this happens (causing fMultisector == fFalse)

        if ( fMultisector )
        {

            //  oops, we have a short checksum on a single-sector range

            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }
    }


    //  test the next pointer

    if ( plrck->le_cbNext != 0 )
    {

        //  there are other LRCHECKSUM records after this one

        //  the range must be atleast 1 sector long and must land on a sector-granular boundary

        if ( cbRange < m_pLogStream->CbSec() || ( cbRange % m_pLogStream->CbSec() ) != 0 )
        {
            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }

        //  calculate the position of the next LRCHECKSUM record

        lgpos = *plgpos;
        m_pLogStream->AddLgpos( &lgpos, sizeof( LRCHECKSUM ) + plrck->le_cbNext );

        //  see if the next pointer lands in the sector after the range of this LRCHECKSUM record
        //
        //  NOTE: this should imply that it does not reach the end of the file since earlier we
        //        made sure the range was less than the end of the file:
        //          next < range, range < EOF; therefore, next < EOF
        //
        //  NOTE: cbRange was previously set

        if ( lgpos.isec != plgpos->isec + ( cbRange / m_pLogStream->CbSec() ) )
        {

            //  it lands somewhere else (before or after)

            Assert( !g_fLRCKValidationTrap );
            return fFalse;
        }


        /*******
        //  see if it reaches beyond the end of the file

        if ( QWORD( lgpos.isec ) * m_pLogStream->CbSec() + lgpos.ib > ( qwSize - sizeof( LRCHECKSUM ) ) )
        {
            return fFalse;
            // the LRCHECKSUM is bad because it's next pointer points to
            // past the end of the log file.
            // Note that plrckNext->cbNext can be zero if it's the last LRCHECKSUM
            // in the log file.
        }
        *******/
    }

    return fTrue;

HandleError:

    //  this is the weird case where IFileAPI::ErrSize() returns an error??? should never happen...

    return fFalse;
}

// Checks if an LRCHECKSUM on a particular sector is a valid shadow sector.

BOOL LOG_READ_BUFFER::FValidLRCKShadow(
    const LRCHECKSUM * const plrck,
    const LGPOS * const plgpos,
    const LONG lGeneration
    )
{

    //  validate the LRCHECKSUM record

    if ( !FValidLRCKRecord( plrck, plgpos ) )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    //  we should not have a short checksum value

    if ( plrck->bUseShortChecksum != bShortChecksumOff )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    //  we should not have another LRCHECKSUM record after this one

    if ( plrck->le_cbNext != 0 )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    //  verify the range checksum

    if ( UlComputeShadowChecksum( UlComputeChecksum( plrck, (ULONG32)lGeneration ) ) != plrck->le_ulChecksum )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    return fTrue;
}


// Checks if an LRCHECKSUM on a particular sector is a valid shadow sector without looking at cbNext

BOOL LOG_READ_BUFFER::FValidLRCKShadowWithoutCheckingCBNext(
    const LRCHECKSUM * const plrck,
    const LGPOS * const plgpos,
    const LONG lGeneration
    )
{

    //  validate the LRCHECKSUM record

    if ( !FValidLRCKRecord( plrck, plgpos ) )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    //  we should not have a short checksum value

    if ( plrck->bUseShortChecksum != bShortChecksumOff )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    //  verify the range checksum

    if ( UlComputeShadowChecksum( UlComputeChecksum( plrck, (ULONG32)lGeneration ) ) != plrck->le_ulChecksum )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    return fTrue;
}


// Determines if an LRCHECKSUM on a sector has a valid short checksum.

BOOL LOG_READ_BUFFER::FValidLRCKShortChecksum(
    const LRCHECKSUM * const plrck,
    const LONG lGeneration
    )
{

    //  assume the caller has checked to see if we even have a short checksum

    Assert( plrck->bUseShortChecksum == bShortChecksumOn );
    Assert( plrck->le_cbBackwards <= ( m_pLogStream->CbSec() - sizeof( LRCHECKSUM ) ) );

    //  compute the checksum

    if ( UlComputeShortChecksum( plrck, (ULONG32)lGeneration ) != plrck->le_ulShortChecksum )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
    }

    return fTrue;
}


// Once we know the structure of an LRCHECKSUM record is valid, we need
// to verify the validity of the region of data covered by the LRCHECKSUM.

BOOL LOG_READ_BUFFER::FValidLRCKRange(
    const LRCHECKSUM * const plrck,
    const LONG lGeneration
    )
{

    // If the sector is a shadow sector, the checksum won't match
    // because a shadow sector has ulShadowSectorChecksum added to its
    // checksum.
    if ( UlComputeChecksum( plrck, (ULONG32)lGeneration ) != plrck->le_ulChecksum )
    {
        Assert( !g_fLRCKValidationTrap );
        return fFalse;
        // the LRCHECKSUM is bad because it's checksum doesn't match
    }

    return fTrue;

}


//  verify the LRCK record

void LOG_READ_BUFFER::AssertValidLRCKRecord(
    const LRCHECKSUM * const plrck,
    const LGPOS * const plgpos )
{
#ifdef DEBUG
    const BOOL fValid = FValidLRCKRecord( plrck, plgpos );
    Assert( fValid );
#endif  //  DEBUG
}


//  verify the current LRCK range

void LOG_READ_BUFFER::AssertValidLRCKRange(
        const LRCHECKSUM * const plrck,
        const LONG lGeneration )
{
#ifdef DEBUG

    //  we can directly assert this condition because it will never fail unless the checksum is actually bad

    Assert( FValidLRCKRange( plrck, lGeneration ) );
#endif  //  DEBUG
}


//  verify the current LRCK record in DBG or RTL

void LOG_READ_BUFFER::AssertRTLValidLRCKRecord(
        const LRCHECKSUM * const plrck,
        const LGPOS * const plgpos )
{
    const BOOL fValid = FValidLRCKRecord( plrck, plgpos );
    AssertRTL( fValid );
}


//  verify the current LRCK range in DBG or RTL

void LOG_READ_BUFFER::AssertRTLValidLRCKRange(
        const LRCHECKSUM * const plrck,
        const LONG lGeneration )
{

    //  we can directly assert this condition because it will never fail unless the checksum is actually bad

    AssertRTL( FValidLRCKRange( plrck, lGeneration ) );
}


//  verify the current LRCK shadow in DBG or RTL

void LOG_READ_BUFFER::AssertRTLValidLRCKShadow(
        const LRCHECKSUM * const plrck,
        const LGPOS * const plgpos,
        const LONG lGeneration )
{
    const BOOL fValid = FValidLRCKShadow( plrck, plgpos, lGeneration );
    AssertRTL( fValid );
}


//  verify that the current LRCK range is INVALID

void LOG_READ_BUFFER::AssertInvalidLRCKRange(
        const LRCHECKSUM * const plrck,
        const LONG lGeneration )
{

    //  we can directly assert this condition because it will never fail unless the checksum is actually bad

    AssertRTL( !FValidLRCKRange( plrck, lGeneration ) );
}


#ifdef ENABLE_LOGPATCH_TRACE

//  helper for writing to the log-patch text file in ErrLGCheckReadLastLogRecordFF

INLINE BOOL FLGILogPatchDate( const WCHAR* wszPath, CPRINTFFILE **const ppcprintf )
{
    CPRINTFFILE *pcprintf = *ppcprintf;
    DATETIME    datetime;

    if ( !pcprintf )
    {

        //  allocate a new trace file object

        pcprintf = new CPRINTFFILE( wszPath );
        if ( !pcprintf )
        {
            return fFalse;
        }
        *ppcprintf = pcprintf;

        //  start the trace file with a standard header

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

#endif  //  ENABLE_LOGPATCH_TRACE


//  this function is a version of memcmp() that attempts to match a repeating
//  pattern rather than a simple array.  that comperand size does not have to
//  be an integral multiple of the pattern size
//
//  NOTE:  if the pattern size is zero then the result will always be zero
//
inline INT __cdecl _memcmpp( const void* pv, const void* pvPattern, size_t cbPattern, size_t cb )
{
    INT iCmp = 0;
    for ( size_t ib = 0; !iCmp && cbPattern && ib < cb; ib += cbPattern )
    {
        iCmp = memcmp( (char*)pv + ib, pvPattern, min( cbPattern, cb - ib ) );
    }
    return iCmp;
}


//  this function is a version of memcpy() that attempts to fill a region of
//  memory with a repeating pattern.  the destination region size does not have
//  to be an integral multiple of the pattern size
//
//  NOTE:  if the pattern size is zero then the destination will be unchanged
//
inline void* __cdecl _memcpyp( void* pv, const void* pvPattern, size_t cbPattern, size_t cb )
{
    for ( size_t ib = 0; cbPattern && ib < cb; ib += cbPattern )
    {
        memcpy( (char*)pv + ib, pvPattern, min( cbPattern, cb - ib ) );
    }
    return pv;
}


//  UNDONE: we're currently using 0 for the expected/actual checksums
//  to denote that the checksum record itself is corrupt - this
//  is handled by reporting CHECKSUM_REC_CORRUPTION instead of RANGE_CHECKSUM_MISMATCH.
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
    // compute end of forward region
    const ULONG ibEndOfForward = m_ibLRCK + sizeof( m_lrck ) + m_lrck.le_cbForwards;

    Assert( ibEndOfForward >= m_ibChecksumIncremental );
    if ( ibEndOfForward == m_ibChecksumIncremental )
    {
        // done checksumming forward region & everything else
        if ( m_lrck.le_ulChecksum != m_ck.EndChecksum() )
        {
            return ErrERRCheck( JET_errLogReadVerifyFailure );
        }
        if ( 0 == m_lrck.le_cbNext )
        {
            // no next LRCK region, thus we've gone through entire log file and verified every byte
            m_state = LogVerifyDone;
        }
        else
        {
            // setup for next LRCHECKSUM
            // next LRCK could be in next block, or it could be in this block
            m_ibLRCK += sizeof( LRCHECKSUM ) + m_lrck.le_cbNext;
            m_ibChecksumIncremental = m_ibLRCK + sizeof( LRCHECKSUM );
            m_state = LogVerifyLRCKStart;
        }
    }
    else
    {
        // more data to checksum in next block, so setup continue state
        m_state = LogVerifyLRCKContinue;
    }
    return JET_errSuccess;
}

ERR
LOG_VERIFY_STATE::ErrVerifyLRCKContinue( __in_bcount(cb) BYTE *pb, DWORD cb )
{
    // if we checksummed from beginning of this block (which is m_ibChecksumIncremental) to
    // however far the forwards pointer goes
    const ULONG ibEndOfForwards = m_ibLRCK + sizeof( m_lrck ) + m_lrck.le_cbForwards;
    Assert( ibEndOfForwards > m_ibChecksumIncremental );
    const ULONG cbMaxForwards = ibEndOfForwards - m_ibChecksumIncremental;
    const BYTE* const pbMax = pb + min( cb, cbMaxForwards );

    if ( ( pbMax < pb ) ||
        ( pbMax > pb + cb ) )
    {
        // max out of buffer
        return ErrERRCheck( JET_errLogReadVerifyFailure );
    }
    m_ck.ChecksumBytes( pb, pbMax );
    m_ibChecksumIncremental += ULONG( pbMax - pb );

    return ErrVerifyLRCKEnd();
}

// Note: pb != plrck necessarily because an LRCK can be placed anywhere in a sector,
// thus anywhere on a data block.
// pb defined to be beginning of block that we have access to
// plrck defined to be beginning of LRCK in the block we have access to.
// plrck is within the block.
//
//  This function is based on UlComputeChecksum, so be sure these are in sync.

ERR
LOG_VERIFY_STATE::ErrVerifyLRCKStart( __in_bcount(cb) BYTE *pb, DWORD cb )
{
    ULONG32 ulChecksum = ulLRCKChecksumSeed;

    // distance from LRCK in this block to beginning of block
    Assert( m_ibLRCK >= m_cbSeen );
    const LRCHECKSUM* const plrck = reinterpret_cast< const LRCHECKSUM* >( pb + ( m_ibLRCK - m_cbSeen ) );

    // LRCK must be within the buffer
    if ( ( reinterpret_cast< const BYTE* >( plrck ) < pb ) ||
         ( reinterpret_cast< const BYTE* >( plrck ) + sizeof( *plrck ) > cb + pb ) )
    {
        return ErrERRCheck( JET_errLogReadVerifyFailure );
    }

    // LRCK's lrtyp must be correct
    if ( plrck->lrtyp != lrtypChecksum )
    {
        return ErrERRCheck( JET_errLogReadVerifyFailure );
    }

    // copy
    m_lrck = *plrck;

    // checksum backward region
    {
        const BYTE* const pbMin = reinterpret_cast< const BYTE* >( plrck ) - plrck->le_cbBackwards;
        if ( ( pbMin < pb ) ||
            ( pbMin >= cb + pb ) )
        {
            // le_cbBackwards was corrupt if it is outside of buffer
            return ErrERRCheck( JET_errLogReadVerifyFailure );
        }
        const BYTE* const pbMax = reinterpret_cast< const BYTE* >( plrck );
        ulChecksum = LGChecksum::UlChecksumBytes( pbMin, pbMax, ulChecksum );
    }

    // checksum various other attributes of LRCHECKSUM

    ulChecksum ^= plrck->le_cbBackwards;
    ulChecksum ^= plrck->le_cbForwards;
    ulChecksum ^= plrck->le_cbNext;

    ulChecksum ^= (ULONG32) plrck->bUseShortChecksum;

    ulChecksum ^= m_lGeneration;

    // prepare to start checksumming forward region which may extend into other blocks
    m_ck.BeginChecksum( ulChecksum );

    // end of bytes incrementally checksummed so far
    m_ibChecksumIncremental = m_ibLRCK + sizeof( *plrck );

    // checksum forward region
    {
        const BYTE* const pbMin = reinterpret_cast< const BYTE* >( plrck ) + sizeof( *plrck );
        const BYTE* const pbMaxForwards = pbMin + plrck->le_cbForwards;
        const BYTE* const pbMaxBlock = pb + cb;
        const BYTE* const pbMax = min( pbMaxForwards, pbMaxBlock );

        if ( ( pbMin < pb ) ||
            ( pbMin > pb + cb ) )
        {
            // min out of buffer
            return ErrERRCheck( JET_errLogReadVerifyFailure );
        }
        if ( ( pbMax < pb ) ||
            ( pbMax > pb + cb ) )
        {
            // max out of buffer
            return ErrERRCheck( JET_errLogReadVerifyFailure );
        }
        if ( pbMax < pbMin )
        {
            // psychotic pointers -- avoid access violation here
            return ErrERRCheck( JET_errLogReadVerifyFailure );
        }
        m_ck.ChecksumBytes( pbMin, pbMax );
        m_ibChecksumIncremental += ULONG( pbMax - pbMin );
    }

    return ErrVerifyLRCKEnd();
}

//  check the log, searching for the last log record and seeing whether or not it is a term record
//  this will patch any damange to the log as well (when it can)

// Implicit output parameters:
//      m_pbLastChecksum
//      m_pbWrite
//      m_isecWrite
//      m_pbEntry

// At the end of this function, we should have done any necessary fix-ups
// to the log file. i.e. writing a shadow sector that didn't get written,
// writing over a corrupted regular data sector.

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
#endif  //  ENABLE_LOGPATCH_TRACE
//
//  SEARCH-STRING: SecSizeMismatchFixMe
//
//  BOOL            fWriteOnSectorSizeMismatch;

    //  we must have an initialized volume sector size

    Assert( m_pLogStream->CbSecVolume() != ~(ULONG)0 );
    Assert( m_pLogStream->CbSecVolume() % 512 == 0 );

    //  we should not be probing for a torn-write unless we are dumping logs

    Assert( !pfIsPatchable || m_pLog->FDumpingLogs() );

    //  initialize variables

    err                         = JET_errSuccess;   //  assume success
    plrck                       = pNil;             //  LRCHECKSUM class pointer
    lgposCurrent.ib             = 0;                //  what we are looking at right now
    lgposCurrent.isec           = USHORT( m_pLogStream->CSecHeader() );
    lgposCurrent.lGeneration    = m_pLogStream->GetCurrentFileGen();
    lgposLast.ib                = 0;                //  the last LRCHECKSUM we saw
    lgposLast.isec              = 0;
    lgposLast.lGeneration       = m_pLogStream->GetCurrentFileGen();
    lgposEnd.ib                 = 0;                //  position after the end of logical data
    lgposEnd.isec               = 0;
    lgposEnd.lGeneration        = m_pLogStream->GetCurrentFileGen();
    pbEnsure                    = pbNil;            //  start of data we're looking at in the log buffer
    pbLastSector                = pbNil;            //  separate page with copy of last sector of log data
    pbLastChecksum              = pbNil;            //  offset of the LRCHECKSUM within pbLastSector
    isecLastSector              = m_pLogStream->CSecHeader();       //  sector offset within log file of data in pbLastSector
    isecPatternFill             = 0;                //  sector to start rewriting the logfile extend pattern
    csecPatternFill             = 0;                //  number of sectors of logfile extent pattern to rewrite
    fDoScan                     = fFalse;           //  perform a scan for corruption/torn-write
    fGotQuit                    = fFalse;           //  found lrtypQuit or lrtypTerm? ("clean-log" flag)
    lgposLastTerm               = lgposMin;         //  if fGotQuit, then this is the last Term
    fCreatedLogReader           = fFalse;           //  did we allocate a LogReader or are we sharing one?
    fLogToNextLogFile           = fTrue;            //  should we start a new gen. or continue the old one?
    fRecordOkRangeBad           = fFalse;           //  true when the LRCK recors is OK in itself, but its
                                                    //      range has a bad checksum value
    lgposScan.ib                = 0;                //  position from which to scan for corruption/torn-writes
    lgposScan.isec              = 0;
    lgposScan.lGeneration       = m_pLogStream->GetCurrentFileGen();
    fTookJump                   = fFalse;
    fSingleSectorTornWrite      = fFalse;           //  set when we have a single-sector torn-write
    fSkipScanAndApplyPatch      = fFalse;           //  see where this is set for details about it
    cbChecksum                  = 0;                //  number of bytes involved in the last bad checksum
    ulChecksumExpected          = 0;
    ulChecksumActual            = 0;

//
//  SEARCH-STRING: SecSizeMismatchFixMe
//
//  fWriteOnSectorSizeMismatch  = fFalse;           //  tried to write to the log using a different sector size

#ifdef ENABLE_LOGPATCH_TRACE

    //  compute the path for the trace log

{
    WCHAR   wszFolder[ IFileSystemAPI::cchPathMax ];
    WCHAR   wszT[ IFileSystemAPI::cchPathMax ];

    if ( m_pinst->m_pfsapi->ErrPathParse( m_pLogStream->LogName(), wszFolder, wszT, wszT ) < JET_errSuccess ||
         m_pinst->m_pfsapi->ErrPathBuild( wszFolder, L"LOGPATCH", L"TXT", wszLogPatchPath ) < JET_errSuccess )
    {
        OSStrCbCopyW( wszLogPatchPath, sizeof(wszLogPatchPath), L"LOGPATCH.TXT" );
    }
}
#endif  //  ENABLE_LOGPATCH_TRACE

    if ( pfIsPatchable )
    {
        *pfIsPatchable = fFalse;
    }

    //  mark the end of data for redo time

    m_lgposLastRec = lgposCurrent;

    //  are we looking at edb.jtx/log?

{
        WCHAR wszT[IFileSystemAPI::cchPathMax], wszFNameT[IFileSystemAPI::cchPathMax];

        CallS( m_pinst->m_pfsapi->ErrPathParse( m_pLogStream->LogName(), wszT, wszFNameT, wszT ) );
        fJetLog = ( UtilCmpFileName( wszFNameT, SzParam( m_pinst, JET_paramBaseName ) ) == 0 );
}

    // if we are not at edb.log, we do not need to open R/W 
    // even if the caller asked for
    //
    const BOOL  fReadOnlyLocal = fReadOnly || !fJetLog;
    
    Assert( m_pLog->FRecovering() );
    Assert( m_pLog->FRecoveringMode() == fRecoveringRedo );

    //  allocate a LogReader class (coming from dump code) or share the existing one (coming from redo code)


    if ( m_wszReaderLogName[0] != 0 )
    {
        fCreatedLogReader = fFalse;

        //  reset reader so we get fresh data from disk
        LReaderReset();

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

        //  get the size of the log in sectors

        QWORD   cbSize;
        Call( m_pLogStream->ErrFileSize( &cbSize ) );

        Assert( m_pLogStream->CbSec() > 0 );
        Assert( ( cbSize % m_pLogStream->CbSec() ) == 0 );
        UINT    csecSize;
        csecSize = UINT( cbSize / m_pLogStream->CbSec() );
        Assert( csecSize > m_pLogStream->CSecHeader() );

        //  initialize the log reader

        Call( ErrLReaderInit( csecSize ) );
    }

    //  assume the result will not be a cleanly shutdown log generation
    //     (e.g. we expect not to find an lrtypTerm or lrtypQuit record)

    Assert( pfCloseNormally );
    *pfCloseNormally = fFalse;

    //  re-open the log in read/write mode if requested by the caller

    Assert( m_pLogStream->FLGFileOpened() );
    if ( !fReadOnlyLocal )
    {
        Assert( NULL != m_pLogStream->LogName() );
        Assert( L'\0' != m_pLogStream->LogName()[ 0 ] );
        Call( m_pLogStream->ErrLGOpenFile() );
    }

    //  make sure we have the right log file

    Call( ErrReaderEnsureLogFile() );

    //  allocate memory for pbLastSector

    pbLastSector = reinterpret_cast< BYTE* >( PvOSMemoryPageAlloc( m_pLogStream->CbSec(), NULL ) );
    if ( pbNil == pbLastSector )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    // XXX
    // gross temp hack so that entire log buffer is considered in use,
    // so Assert()s don't fire off like crazy, especially in UlComputeChecksum().
    //
    //  ??? what is SOMEONE's comment (above) referring to ???

    //  loop forever while scanning this log generation

    forever
    {

        //  read the first sector and point to the LRCHECKSUM record

        Call( ErrReaderEnsureSector( lgposCurrent.isec, 1, &pbEnsure ) );
        plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposCurrent.ib );

        //  we are about to check data in the sector containing the LRCHECKSUM record
        //  we may checksum the whole sector, or we may just verify the LRCHECKSUM record itself through heuristics

        cbChecksum = m_pLogStream->CbSec();

        ulChecksumExpected = 0;
        ulChecksumActual = 0;

        //  check the validity of the LRCHECKSUM (does not verify the checksum value!)

        if ( !FValidLRCKRecord( plrck, &lgposCurrent ) )
        {
            //  see if this was a simple checksum mismatch, or something more complicated
            //
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
                    //  update checksum variables so that the
                    //  checksum mismatch will be correctly
                    //  reported in the eventlog
                    //
                    ulChecksumActual = ulChecksumT;
                    ulChecksumExpected = plrck->le_ulShortChecksum;
                }
                else
                {
                    //  short checksum was being used and it matches,
                    //  so there's something else wrong with the
                    //  LRCHECKSUM record
                    //
                    //  UNDONE: we're currently using 0 for the expected/actual checksums
                    //  to denote that the checksum record itself is corrupt - this
                    //  will probably be confusing when someone looks at the eventlog
                    //  message and sees 0 for the expected/actual checksums
                    //
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
                //(*pcprintfLogPatch)( "\t\tLOG::m_isecWrite       = 0x%08x\r\n", m_isecWrite );

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
#endif  //  ENABLE_LOGPATCH_TRACE

            //  remember that we got here by not having a valid record

            fTookJump = fFalse;

RecoverWithShadow:

            //  the LRCHECKSUM is invalid; revert to the shadow sector (if there is one)

            //  read the shadow sector and point to the LRCHECKSUM record

            Call( ErrReaderEnsureSector( lgposCurrent.isec + 1, 1, &pbEnsure ) );
            plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposCurrent.ib );

            //  check the validity of the shadowed LRCHECKSUM including the checksum value
            //      (we can verify the checksum value here because we know the data doesn't go
            //       past the end of the shadow sector -- thus, we don't need to bring in more data)

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
#endif  //  ENABLE_LOGPATCH_TRACE

                //  the shadowed LRCHECKSUM is also invalid

                //  one special case exists where we can still be ok in this situation:
                //      if the last LRCHECKSUM range (which was validated previously) covers 1 sector,
                //      and it was the last flush because we terminated abruptly, the disk would
                //      contain that last LRCHECKSUM record with a shadowed copy of it, followed by
                //      the log-extend pattern; this is a TORN-WRITE case, but the shadow is getting
                //      in the way and making it hard for us to see things clearly

                //  we must have an invalid LRCHECKSUM record and the last sector must be 1 away
                //      from the current sector

                if ( !fTookJump && lgposCurrent.isec == lgposLast.isec + 1 )
                {

                    //  make sure we have a valid lgposLast -- we should

                    Assert( lgposLast.isec >= m_pLogStream->CSecHeader() && lgposLast.isec < ( m_pLogStream->CSecLGFile() - 1 ) );

                    //  load data from the previous sector and the current sector

                    Call( ErrReaderEnsureSector( lgposLast.isec, 2, &pbEnsure ) );

                    //  set the LRCHECKSUM pointer

                    plrck = (LRCHECKSUM *)( pbEnsure + lgposLast.ib );

                    //  make sure things are as they should be

                    AssertValidLRCKRecord( plrck, &lgposLast );
                    AssertValidLRCKRange( plrck, lgposLast.lGeneration );
                    Assert( plrck->bUseShortChecksum == bShortChecksumOff );
                    Assert( plrck->le_cbNext != 0 );
                    Assert( lgposLast.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards == m_pLogStream->CbSec() );

                    //  move to the supposedly shadowed LRCHECKSUM record

                    plrck = (LRCHECKSUM *)( pbEnsure + m_pLogStream->CbSec() + lgposLast.ib );

                    //  see if this is in fact a valid shadow sector (use the special shadow validation)

                    if ( FValidLRCKShadowWithoutCheckingCBNext( plrck, &lgposLast, lgposLast.lGeneration ) )
                    {
#ifdef ENABLE_LOGPATCH_TRACE
                        if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                        {
                            (*pcprintfLogPatch)( "special case!\r\n" );
                            (*pcprintfLogPatch)( "\tlgposCurrent is NOT pointing to the next LRCHECKSUM record because we crashed before we could write it\r\n" );
                            (*pcprintfLogPatch)( "\tinstead, it is pointing to a shadow of the >>LAST<< LRCHECKSUM record (lgposLast)\r\n" );
                        }
#endif  //  ENABLE_LOGPATCH_TRACE

                        //  indeed we are looking at a shadow sector! this means we have torn-write
                        //  set things up for a special scan to make sure we do have the pattern everywhere

                        //  this should only happen in edb.jtx/log -- all other logs should be clean or corrupt
                        //      (they should never have a torn-write)

                        //  setup for a scan

                        fDoScan = fTrue;

                        //  mark the end of data as the start of the shadow sector

                        lgposEnd.isec = lgposCurrent.isec;
                        lgposEnd.ib = 0;

                        //  prepare a special scan which starts after the shadow sector

                        lgposScan = lgposEnd;
                        m_pLogStream->AddLgpos( &lgposScan, m_pLogStream->CbSec() );

                        //  mark the end of data for redo time

                        m_lgposLastRec = lgposEnd;

                        //  prepare pbLastSector with data for the next flush which will occur after we
                        //      exit the "forever" loop

                        //  copy the last LRCHECKSUM record

                        UtilMemCpy( pbLastSector, pbEnsure, m_pLogStream->CbSec() );

                        //  set the pointers to the copy of the sector with the LRCHECKSUM record

                        pbLastChecksum = pbLastSector + lgposLast.ib;
                        isecLastSector = lgposLast.isec;

                        //  remember that this has the potential to be a single-sector torn-write

                        fSingleSectorTornWrite = fTrue;

                        //  terminate the "forever" loop

                        break;
                    }
                }


                //  we need to do a scan to determine what kind of damage occurred

                fDoScan = fTrue;

                //  NOTE: since we haven't found a good LRCHECKSUM record,
                //      lgposLast remains pointing to the last good LRCHECKSUM record we found
                //      (unless we haven't seen even one, in which case lgposLast.isec == 0)

                //  set lgposEnd to indicate that the garbage to scan starts at the current sector, offset 0

                lgposEnd.isec = lgposCurrent.isec;
                lgposEnd.ib = 0;

                //  start the scan at lgposEnd

                lgposScan = lgposEnd;

                //  mark the end of data for redo time

                m_lgposLastRec = lgposEnd;

                //  prepare pbLastSector with data for the next flush which will occur after we
                //      exit the "forever" loop

                pbLastChecksum = pbLastSector + lgposCurrent.ib;

//
//  LEAVE THE OLD DATA THERE! DO NOT FILL THE SPACE WITH THE PATTERN!
//
//              // fill with known value, so if we ever use the filled data we'll know it (hopefully)
//
//              Assert( rgbLogExtendPattern );
//              _memcpyp( pbLastSector, rgbLogExtendPattern, cbLogExtendPattern, lgposCurrent.ib );

                //  prepare the new LRCHECKSUM record

                plrck = reinterpret_cast< LRCHECKSUM* >( pbLastChecksum );
                memset( pbLastChecksum, 0, sizeof( LRCHECKSUM ) );
                plrck->lrtyp = lrtypChecksum;
                plrck->bUseShortChecksum = bShortChecksumOff;

                //  NOTE: we leave the backward pointer set to 0 to cover the case where the head of
                //            a partial log record was written in the last sector, but the tail of that
                //            record was never written safely to this sector (we just filled the space
                //            where the tail was with log-extend pattern)
                //        the partial log record will be ignored when we scan record by record later on
                //            in this function

                isecLastSector = lgposCurrent.isec;

                //  terminate the "forever" loop

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
#endif  //  ENABLE_LOGPATCH_TRACE

            //  the shadowed LRCHECKSUM is valid and has a good checksum value

            //  this means we are definitely at the end of the log, and we can repair the corruption
            //      using the shadow sector (make repairs quietly except for an event-log message)
            //
            //  NOTE: it doesn't matter which recovery mode we are in or which log file we are dealing
            //        with -- shadow sectors can ALWAYS be patched!

            Assert( plrck->le_cbNext == 0 );
            Assert( ( plrck->le_cbBackwards + sizeof( LRCHECKSUM ) + plrck->le_cbForwards ) <= m_pLogStream->CbSec() );

            //  if we are in read-only mode, we cannot patch anything (dump code uses this mode)

            if ( fReadOnlyLocal )
            {
                // SOFT_HARD: leave, not important
                Assert( !m_pLog->FHardRestore() );
                Assert( fCreatedLogReader );

                //  mark the end of data for redo time

                m_lgposLastRec = lgposCurrent;
                m_lgposLastRec.ib = 0;

                if ( pfIsPatchable )
                {

                    //  the caller wants to know if the log is patchable
                    //  by returning TRUE, we imply that the log is damaged so we don't need to return
                    //      an error as well

                    *pfIsPatchable = fTrue;
                    err = JET_errSuccess;
                    goto HandleError;
                }
                else
                {
                    //  report log file corruption to the event log

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

                    //  return corruption

                    Error( ErrERRCheck( JET_errLogFileCorrupt ) );
                }
            }

            //  do not scan since we are going to repair the corruption

            fDoScan = fFalse;

            //  send the event-log message

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

            //  before patching the original LRCHECKSUM record with the shadowed LRCHECKSUM record,
            //      recalculate the checksum value since shadowed checksum values are different

            plrck->le_ulChecksum = UlComputeChecksum( plrck, m_pLogStream->GetCurrentFileGen() );

            //  patch the original LRCHECKSUM record

//
//  SEARCH-STRING: SecSizeMismatchFixMe
//
Assert( m_pLogStream->CbSec() == m_pLogStream->CbSecVolume() );
//          if ( m_pLogStream->CbSec() == m_pLogStream->CbSecVolume() )
//              {
                Call( m_pLogStream->ErrLGWriteSectorData(
                        IOR( iorpPatchFix ),
                        m_lgposLastRec.lGeneration,
                        m_pLogStream->CbSec() * QWORD( lgposCurrent.isec ),
                        m_pLogStream->CbSec(),
                        pbEnsure,
                        LOG_WRITE_ERROR_ID ) );
                ReaderSectorModified( lgposCurrent.isec, pbEnsure );
//              }
//          else
//              {
//              fWriteOnSectorSizeMismatch = fTrue;
//              }

            //  check for data after the LRCHECKSUM (at most, it will run to the end of the sector)

            if ( plrck->le_cbForwards == 0 )
            {

                //  no forward data was found
                //
                //  in this case, we want to search for the lrtypTerm and/or lrtypRecoveryQuit
                //      log records up to and including the current LRCHECKSUM record
                //      (e.g. the forward range of the last LRCHECKSUM and the backward
                //            range of this LRCHECKSUM)

                //  to make the search work, we must setup lgposLast and lgposEnd:
                //      we leave lgposLast (the last valid LRCHECKSUM we found) pointing at the
                //          previous LRCHECKSUM even though we just patched and made valid
                //          the current LRCHECKSUM at lgposCurrent
                //      we set lgposEnd (the point to stop searching) to the position
                //          immediately after this LRCHECKSUM since this LRCHECKSUM had no
                //          forward data

                lgposEnd = lgposCurrent;
                m_pLogStream->AddLgpos( &lgposEnd, sizeof( LRCHECKSUM ) );
            }
            else
            {

                //  forward data was found
                //
                //  in this case, we want to search for the lrtypTerm and/or lrtypRecoveryQuit
                //      log records within the backward data of the patched LRCHECKSUM

                //  set lgposLast (the last valid LRCHECKSUM record) to the current LRCHECKSUM

                lgposLast = lgposCurrent;

                //  set lgposEnd (the point after the last good data) to the first byte after
                //      the forward range of this LRCHECKSUM

                lgposEnd = lgposCurrent;
                Assert( plrck->le_cbBackwards == lgposCurrent.ib || 0 == plrck->le_cbBackwards );
                m_pLogStream->AddLgpos( &lgposEnd, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );
            }

            //  mark the end of data for redo time

            m_lgposLastRec = lgposEnd;

            //  prepare pbLastSector with data for the next flush which will occur after we
            //      exit the "forever" loop

            //  copy the patched shadow sector (contains the real LRCHECKSUM at this point)

            UtilMemCpy( pbLastSector, pbEnsure, m_pLogStream->CbSec() );

            //  set the pointers to the copy of the sector with the patched LRCHECKSUM

            pbLastChecksum = pbLastSector + lgposCurrent.ib;
            isecLastSector = lgposCurrent.isec;

            //  terminate the "forever" loop

            break;
        }


        //  at this point, the LRCHECKSUM record is definitely valid
        //  however, we still need to verify the checksum value over its range

        //  load the rest of the data in the range of this LRCHECKSUM

        Call( ErrReaderEnsureSector( lgposCurrent.isec, ( ( lgposCurrent.ib +
            sizeof( LRCHECKSUM ) + plrck->le_cbForwards - 1 ) / m_pLogStream->CbSec() ) + 1,
            &pbEnsure ) );

        //  adjust the pointer to the current LRCHECKSUM record
        //      (it may have moved when reading in new sectors in ErrEnsureSector )

        plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposCurrent.ib );

        //  we are about to check data in the range of the LRCHECKSUM record

        cbChecksum = lgposCurrent.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards;

        //  verify the checksum value
        //  (copied from FValidLRCKRange() because we need to
        //  preserve the checksum values for event-logging purposes)
        //
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
                //(*pcprintfLogPatch)( "\t\tLOG::m_isecWrite       = 0x%08x\r\n", m_isecWrite );

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
#endif  //  ENABLE_LOGPATCH_TRACE

            //  the checksum over the entire range of the LRCHECKSUM record is wrong

            //  do we have a short checksum?

            if ( plrck->bUseShortChecksum == bShortChecksumOn )
            {

                //  we have a short checksum, so we know that the first sector is valid and that the range
                //      of this LRCHECKSUM record is a multi-sector range

                //  we are either in the middle of the log file (most likely), or we are at the end of a
                //      generation whose last flush was a multisector flush (unlikely -- special case)

                //  in either case, this may really be the end of the log data!
                //      middle of log case: just say the power went out as we were flushing and the first few
                //        (cbNext != 0)     sectors went to disk (including the LRCHECKSUM), but the rest did not
                //                          make it; that would make it look like we are in the middle of the log
                //                          when we are really at the end of it
                //      end of log case:    we already know we were at the end of the log!
                //        (cbNext == 0)

                //  to see if this is the real end of the log, we must scan through data after the "forward"
                //      range of the current LRCHECKSUM record (lgposCurrent) all the way through to the end
                //      of the log file
                //
                //  if we find nothing but the log pattern, we can recover up this point and say that this is
                //      as far as we can safely go -- the last little bit of data is an incomplete fragment of
                //      our last flush during whatever caused us to crash, and as far as we are concerned,
                //      it is a casualty of the crash!
                //  if we do find something besides the log pattern, then we know there is some other log data
                //      out there, and we cannot safely say that we have restored as much information as possible

                //  set the flag to perform the corruption/torn-write scan

                fDoScan = fTrue;

                //  set lgposLast to the current LRCHECKSUM record

                lgposLast = lgposCurrent;

                //  set lgposEnd to indicate that the end of good data is the end of the current sector

                lgposEnd.isec = lgposCurrent.isec;
                lgposEnd.ib = 0;
                m_pLogStream->AddLgpos( &lgposEnd, m_pLogStream->CbSec() );

                //  mark the end of data for redo time

                m_lgposLastRec = lgposEnd;

                //  start the scan at the end of the data in the range of this LRCHECKSUM record

                lgposScan = lgposCurrent;
                m_pLogStream->AddLgpos( &lgposScan, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );

                //  this should be sector-aligned since it was a multi-sector flush

                Assert( lgposScan.ib == 0 );

                //  flag to defer adjusting the forward pointer until later
                //      (also indicates that the flush was a multi-sector range with a bad sector
                //       somewhere past the first sector)

                fRecordOkRangeBad = fTrue;

                //  NOTE: leave pbLastSector alone for now -- see the comment below to know when it gets updated

                //  NOTE: if the corruption is fixable,
                //            le_cbForwards will be properly adjusted after the fixup to include data from only
                //                the current sector
                //            the fixed up sector will be copied to pbLastSector
                //
                //            << other thing happen and we eventually get to the cleanup code >>
                //
                //            the cleanup code will take the last sector and put it onto the log buffers
                //                as a single sector marked for the start of the next flush

                //  terminate the "forever" loop

                break;
            }

            //  we do not have a short checksum

            //  make sure the bUseShortChecksum flag is definitely set to OFF

            Assert( plrck->bUseShortChecksum == bShortChecksumOff );

            //  SPECIAL CASE: the log may look like this
            //
            //   /---\  /---\ /---\  /---\
            //  |----|--|----|----|--|----|------------
            //  |???(LRCK)???|???(LRCK)???|  pattern
            //  |-------|----|-------|----|------------
            //          \--------/   \->NULL
            //
            //   last sector    shadow(old)
            //
            //
            //  NOTE: the cbNext ptr in "last sector" happens to point to the LRCK in the shadow sector
            //
            //  how do we get to this case?
            //      "last sector" was flushed and shadowed, and we crashed before the next flush could occur
            //  where are our ptrs?
            //      lgposLast is pointing to the checksum in "last sector"
            //      lgposCurrent is pointing to the checksum in "shadow" which has just failed the range checksum

            //  make sure the range of the current sector (SPECIAL CASE or not) is at most 1 sector long

            Assert( lgposCurrent.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards <= m_pLogStream->CbSec() );

            //  see if we are experiencing the SPECIAL CASE (lgposLast.ib will match lgposCurrent.ib)
            //  NOTE: lgposLast may not be set if we are on the first LRCK record

            if (    lgposLast.isec >= m_pLogStream->CSecHeader() &&
                    lgposLast.isec == lgposCurrent.isec - 1 &&
                    lgposLast.ib == lgposCurrent.ib )
            {

                //  read both "last sector" and "shadow" (right now, we only have "shadow")

                Call( ErrReaderEnsureSector( lgposLast.isec, 2, &pbEnsure ) );
                plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposCurrent.ib );

                //  see if the "shadow" sector is a real shadow

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
#endif  //  ENABLE_LOGPATCH_TRACE

                    //  enter the scan code, but skip the scan itself

                    fDoScan = fTrue;
                    fSkipScanAndApplyPatch = fTrue;

                    //  setup lgposScan's generation (the rest of it is not used)

                    lgposScan.lGeneration = lgposLast.lGeneration;

                    //  the end ptr is the start of the shadow sector

                    lgposEnd.ib             = 0;
                    lgposEnd.isec           = lgposCurrent.isec;
                    lgposEnd.lGeneration    = lgposCurrent.lGeneration;

                    //  mark the end of data for redo time

                    m_lgposLastRec = lgposEnd;

                    //  prepare the patch buffer ptrs

                    pbLastChecksum = pbLastSector + lgposCurrent.ib;
                    isecLastSector = lgposCurrent.isec;

                    //  fill with known value, so if we ever use the filled data we'll know it (hopefully)

                    Assert( rgbLogExtendPattern );
                    _memcpyp( pbLastSector, rgbLogExtendPattern, cbLogExtendPattern, m_pLogStream->CbSec() );

                    //  prepare the new LRCHECKSUM record

                    plrck = reinterpret_cast< LRCHECKSUM* >( pbLastChecksum );
                    memset( pbLastChecksum, 0, sizeof( LRCHECKSUM ) );
                    plrck->lrtyp = lrtypChecksum;
                    plrck->bUseShortChecksum = bShortChecksumOff;

                    //  NOTE: we leave the backward pointer set to 0 to cover the case where the head of
                    //            a partial log record was written in the last sector, but the tail of that
                    //            record was never written safely to this sector (we just filled the space
                    //            where the tail was with log-extend pattern)
                    //        the partial log record will be ignored when we scan record by record later on
                    //            in this function

                    //  terminate the "forever" loop

                    break;
                }

                //  "shadow" was not a shadow sector

                //  this case must be a standard case of corruption/torn-write and should run through the
                //      regular channels (continue on)

            }

            //  since the short checksum is off, the range is limited to the first sector only, and the
            //      data in that range is invalid
            //  it also means we should have a shadow sector making this the end of the log

            //  jump to the code that attempts to recover using the shadow sector

            fTookJump = fTrue;
            goto RecoverWithShadow;
        }

        //  at this point, the LRCHECKSUM record and its range are completely valid

        //  is this the last LRCHECKSUM record?

        if ( plrck->le_cbNext == 0 )
        {
            BOOL fIsMultiSector;

            //  this is the last LRCHECKSUM record in the log file

            //  since the LRCHECKSUM record is valid, we won't need a scan for any corruption

            fDoScan = fFalse;

            //  see if this LRCHECKSUM record is a multi-sector flush (it has a short checksum)
            //      (remember that logs can end in either single-sector or multi-sector flushes)

            fIsMultiSector = ( plrck->bUseShortChecksum == bShortChecksumOn );

            //  if this was a single-sector flush, we need to create a shadow sector now (even if one
            //      already exists); this ensures that if the flush at the end of this function goes
            //      bad, we can get back the data we lost
            //  NOTE: only do this for edb.jtx/log!

            if ( !fIsMultiSector && fJetLog )
            {

                //  this was a single-sector flush; create the shadow sector now

                Assert( m_pLogStream->PbSecAligned( reinterpret_cast< const BYTE* >( plrck ), m_pbLGBufMin ) == pbEnsure );
                const ULONG ulNormalChecksum = plrck->le_ulChecksum;

                //  the checksum value in the LRCHECKSUM record should match the re-calculated checksum

                Assert( ulNormalChecksum == UlComputeChecksum( plrck, (ULONG32)m_pLogStream->GetCurrentFileGen() ) );

                //  get the shadow checksum value

                plrck->le_ulChecksum = UlComputeShadowChecksum( ulNormalChecksum );

                //  write the shadow sector

                if ( !fReadOnlyLocal )
                {
//
//  SEARCH-STRING: SecSizeMismatchFixMe
//
Assert( m_pLogStream->CbSec() == m_pLogStream->CbSecVolume() );
//                  if ( m_pLogStream->CbSec() == m_pLogStream->CbSecVolume() )
//                      {
                        Call( m_pLogStream->ErrLGWriteSectorData(
                                IOR( iorpPatchFix ),
                                m_lgposLastRec.lGeneration,
                                m_pLogStream->CbSec() * QWORD( lgposCurrent.isec + 1 ),
                                m_pLogStream->CbSec(),
                                pbEnsure,
                                LOG_WRITE_ERROR_ID ) );
//                      }
//                  else
//                      {
//                      fWriteOnSectorSizeMismatch = fTrue;
//                      }
                }

                //  mark the sector as modified

                ReaderSectorModified( lgposCurrent.isec + 1, pbEnsure );

                //  fix the checksum value in the original LRCHECKSUM record

                plrck->le_ulChecksum = ulNormalChecksum;
            }

            //  does this LRCHECKSUM record have a "forward" range? (may be single- or multi- sector)

            if ( plrck->le_cbForwards == 0 )
            {

                //  no forward data was found

                //  this flush should have been a single-sector flush

                Assert( !fIsMultiSector );

                //  in this case, we want to search for the lrtypTerm and/or lrtypRecoveryQuit
                //      log records up to and including the current LRCHECKSUM record
                //      (e.g. the forward range of the last LRCHECKSUM and the backward
                //            range of this LRCHECKSUM)

                //  to make the search work, we must setup lgposLast and lgposEnd:
                //      we leave lgposLast (the last valid LRCHECKSUM we found) pointing at the
                //          previous LRCHECKSUM even though we know the current LRCHECKSUM is ok
                //          because we want to start scanning from the last LRCHECKSUM record
                //      we set lgposEnd (the point to stop searching) to the position
                //          immediately after this LRCHECKSUM since this LRCHECKSUM had no
                //          forward data

                lgposEnd = lgposCurrent;
                m_pLogStream->AddLgpos( &lgposEnd, sizeof( LRCHECKSUM ) );

                //  prepare pbLastSector with data for the next flush which will occur after we
                //      exit the "forever" loop

                //  copy the sector with the LRCHECKSUM record

                UtilMemCpy( pbLastSector, pbEnsure, m_pLogStream->CbSec() );

                //  set the pointers to the copy of the LRCHECKSUM record

                pbLastChecksum = pbLastSector + lgposCurrent.ib;
                isecLastSector = lgposCurrent.isec;
            }
            else
            {

                //  forward data was found
                //
                //  in this case, we want to search for the lrtypTerm and/or lrtypRecoveryQuit
                //      log records within the forward data of the LRCHECKSUM record

                //  set lgposLast (the last valid LRCHECKSUM record) to the current LRCHECKSUM record

                lgposLast = lgposCurrent;

                //  set lgposEnd (the point after the last good data) to the first byte after
                //      the forward range of this LRCHECKSUM

                lgposEnd = lgposCurrent;
                Assert( plrck->le_cbBackwards == lgposCurrent.ib || 0 == plrck->le_cbBackwards );
                m_pLogStream->AddLgpos( &lgposEnd, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );

                //  was this flush a multi-sector flush?

                if ( fIsMultiSector )
                {

                    //  this happens when the last flush is was multi-sector flush, which in turn happens when:
                    //      backup/restore request a new generation and we happen to be doing a multi-sector flush
                    //          because we are under high load
                    //      we are at the end of the log file, we need to flush N sectors where N is the exact
                    //          amount of sectors left in the log file, and there is no room for another
                    //          LRCHECKSUM record at the end of the (N-1)th sector
                    //
                    //  naturally, we want to continue using the log file (just like any other case) after the
                    //      last good data; however, we cannot because we would need to safely re-write the
                    //      LRCHECKSUM with an updated cbNext pointer
                    //  that is not SAFELY possible without a shadow sector to back us up in case we fail
                    //
                    //  but do we even really want to re-claim the empty space in the current generation? NO!
                    //      in the case of backup/restore, that generation is OVER and needs to stay that way
                    //          so that the database remains in the state we expect it to be in when we are
                    //          trying to restore it
                    //      in the end of log case, there is no space to reclaim anyway!
                    //
                    //  conclusion --> these cases, however rare they may be, are fine and SHOULD behave the way
                    //                 that they do

                    //  since we can't use this generation, set the flag to start a new one

                    //  NOTE: we can only start a new generation after edb.jtx./log!

                    if ( fJetLog )
                    {
                        fLogToNextLogFile = fTrue;
                    }
                }
                else
                {

                    //  the last flush was a single-sector flush

                    //  prepare pbLastSector with data for the next flush which will occur after we
                    //      exit the "forever" loop

                    //  copy the sector with the LRCHECKSUM record

                    UtilMemCpy( pbLastSector, pbEnsure, m_pLogStream->CbSec() );

                    //  set the pointers to the copy of the LRCHECKSUM record

                    pbLastChecksum = pbLastSector + lgposCurrent.ib;
                    isecLastSector = lgposCurrent.isec;
                }
            }

            //  mark the end of data for redo time

            m_lgposLastRec = lgposEnd;

            //  terminate the "forever" loop

            break;
        }


        //  move to the next LRCHECKSUM record

        //  set lgposLast (the last good LRCHECKSUM record) to the current LRCHECKSUM record

        lgposLast = lgposCurrent;

        //  set lgposCurrent to point to the supposed next LRCHECKSUM record (could be garbage)

        Assert( lgposCurrent.ib == plrck->le_cbBackwards || 0 == plrck->le_cbBackwards );
        m_pLogStream->AddLgpos( &lgposCurrent, sizeof( LRCHECKSUM ) + plrck->le_cbNext );

        //  mark the end of data for redo time

        m_lgposLastRec = lgposCurrent;
        m_lgposLastRec.ib = 0;
    }   //  bottom of "forever" loop

    //  we have now finished scanning the chain of LRCHECKSUM records

    //  did we find ANY records at all (including LRCHECKSUM records)?

    if ( lgposEnd.isec == m_pLogStream->CSecHeader() && 0 == lgposEnd.ib )
    {

        //  the beginning of the log file was also the end; this indicates corruption because
        //      every log file must start with an LRCHECKSUM record

        //  this should not be a single-sector torn-write

        Assert( !fSingleSectorTornWrite );

        //  we should already be prepared to scan for the cause of the corruption

        Assert( fDoScan );

        //  the end position should be set to the current position

        Assert( CmpLgpos( &lgposCurrent, &lgposEnd ) == 0 );

        //  the last position should not have been touched since it was initialized

        Assert( lgposLast.isec == 0 && lgposLast.ib == 0 );

        //  fix lgposLast to be lgposCurrent (we created an empty LRCHECKSUM record here)

        lgposLast = lgposCurrent;

        //  pbLastChecksum should be set to the start of the sector

        Assert( pbLastChecksum == pbLastSector );

        //  isecLastSector should be set to the current sector

        Assert( isecLastSector == lgposCurrent.isec );
    }
    else if ( lgposLast.isec == 0 )
    {

        //  the end of the log was NOT the beginning; HOWEVER, lgposLast was never set
        //
        //  this indicates that only 1 good LRCHECKSUM record exists and it is at the beginning of
        //      the log file; if we had found a second good LRCHECKSUM record, we would have set
        //      lgposLast to the location of the first good LRCHECKSUM record
        //
        //  we may or may not be setup to scan for corruption, so we are limited in the checking we can do
        //      (we might even be corruption free if there is only 1 LRCHECKSUM in the log!)

        //  adjust lgposLast for the record-to-record search for the lrtypTerm and/or lrtypRecoveryQuit records

        lgposLast.isec = USHORT( m_pLogStream->CSecHeader() );
        Assert( lgposLast.ib == 0 );

        //  double check that lgposEnd is atleast 1 full LRCHECKSUM record ahead of lgposLast

        Assert( lgposEnd.isec > lgposLast.isec ||
                ( lgposEnd.isec == lgposLast.isec && lgposEnd.ib >= ( lgposLast.ib + sizeof( LRCHECKSUM ) ) ) );
    }

    //  lgposCurrent, lgposLast, and lgposEnd should now be setup properly

    Assert( CmpLgpos( &lgposLast, &lgposCurrent ) <= 0 );   //  lgposLast <= lgposCurrent
    //  bad assert -- in the first failure case (invalid RECORD and invalid SHADOW),
    //      lgposCurrent will be greater than lgposEnd (or equal to it) because lgposCurrent will
    //      point directly to the location of the bad LRCHECKSUM record
    //Assert( CmpLgpos( &lgposCurrent, &lgposEnd ) <= 0 );  //  lgposCurrent <= lgposEnd

    //  scan for corruption

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
            //(*pcprintfLogPatch)( "\t\tLOG::m_isecWrite       = 0x%08x\r\n", m_isecWrite );

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
#endif  //  ENABLE_LOGPATCH_TRACE

        //  an inconsistency has occurred somewhere in the physical logging structures and/or log data

        //  NOTE: we can only patch torn writes in edb.jtx/log; all other patching must be done offline!

        //  we are either in the middle of the log file (most likely), or we are at the end of a
        //      generation whose last flush was a multisector flush (unlikely -- special case)

        //  in either case, this may really be the end of the log data!
        //      middle of log case: just say the power went out as we were flushing and the first few
        //        (cbNext != 0)     sectors went to disk (including the LRCHECKSUM), but the rest did not
        //                          make it; that would make it look like we are in the middle of the log
        //                          when we are really at the end of it
        //      end of log case:    we already know we were at the end of the log!
        //        (cbNext == 0)

        //  to see if this is the real end of the log, we must scan through data after the "forward" range
        //      of the current LRCHECKSUM record (lgposCurrent) all the way through to the end of the log
        //
        //  if we find nothing but the log pattern, we know this was the last data in the log
        //      (we can recover up this point and say that this is as far as we can safely go)
        //  if we do find something besides the log pattern, then we know there is some other log data
        //      out there, and we cannot safely say that this is the end of the log

        //  after examinig the different cases of corruption, it turns out that there are 2 cases where
        //      we actaully have a torn-write for sure; they are listed below:
        //
        //
        //  in the first case, we have an LRCHECKSUM record covering a range of 1 or more full sectors
        //      (the #'s represent a gap of sector ranging from none to infinity); the range is clean
        //      meaning that all checksums are correct and all data is valid; the next pointer of the
        //      LRCHECKSUM record points into the pattern which runs to the end of the log file
        //  this is the typical situation when we abruptly get a TerminateProcess() or suffer a power
        //      outage -- there is absolutely no chance to flush anything, and we had no idea it was
        //      coming in the first place
        //  NOTE: when the LRCHECKSUM record covers only 1 sector, that sector will have been shadowed
        //        immediately after it -- there is a special case to handle this situation
        //
        //     /---\  /--- # ---\
        //  --|----|--|--- # ----|---------------------------
        //    |...(LRCK).. # ....| pattern
        //  --|-----|----- # ----|---------------------------
        //          \----- # --------/
        //
        //
        //  in the second case, we have an LRCHECKSUM record covering a range of 2 or more full sectors
        //      (the #'s represent a gap of sectors ranging from 1 to infinity); the short checksum
        //      range is clean (it checks out OK and the data in the first sector is OK), but the long
        //      checksum range is dirty (some bits are wrong in one or more of the sectors after the
        //      first sector); notice that atleast one of the sectors exclusive to the long checksum
        //      range contains the pattern; the next pointer is aimed properly, but points at the
        //      the pattern instead of the next LRCHECKSUM record
        //  this is a more rare case; since we find the pattern within the flush, we assume that the
        //      disk never finished the IO request we made when we were flushing the data; we base
        //      this on the fact that there was no data after this, so this must be the end -- finding
        //      the pattern within the flush helps to confirm that this is an incomplete IO and not a
        //      case of real corruption
        //
        //     /---\  /--- # --------------\
        //  --|----|--|--- # --|-------|----|---------------------------
        //    |...(LRCK).. # ..|pattern|....| pattern
        //  --|-----|----- # --|-------|----|---------------------------
        //          \----- # -------------------/
        //
        //  NOTE: case #2 could occur when we crash in between I/Os while trying to overwrite the
        //        shadow sector; the disk will contain the NEW first sector, the OLD shadow sector,
        //        and the pattern; IF THE RANGE ONLY COVERS 2 SECTORS, WE WILL NOT SEE THE PATTERN
        //        IN THE RANGE -- ONLY THE SHADOW SECTOR WILL BE IN THE RANGE! so, we need to realize
        //        this case and check for the shadow-sector instead of the pattern within the range
        //  we should see 2 things in this case: the shadow sector should be a valid shadow sector, and
        //        the log records in the newly flushed first sector and the old shadow sector should
        //        match exactly (the shadow sector will have less than or equal to the number of
        //        log records in the new sector); the LRCHECKSUM records from each sector should not match
        //

        BYTE        *pbEnsureScan   = NULL;
        BYTE        *pbScan         = NULL;
        ULONG       csecScan        = 0;
        BOOL        fFoundPattern   = fTrue;    //  assume we will find a torn write
        BOOL        fIsTornWrite    = fTrue;    //  assume we will find a torn write
        WCHAR       szSector[30];
        WCHAR       szCorruption[30];
        const WCHAR*    rgpsz[3]        = { m_pLogStream->LogName(), szSector, szCorruption };

        const QWORD ibOffset        = QWORD( lgposCurrent.isec ) * QWORD( m_pLogStream->CbSec() );
        const DWORD cbLength        = cbChecksum;

        //  prepare the event-log messages

        OSStrCbFormatW( szSector, sizeof( szSector ), L"%d:%d", lgposCurrent.isec, lgposCurrent.ib );
        szCorruption[0] = 0;

        //  skip the scan if we are coming from a special-case above (see code for details)

        if ( fSkipScanAndApplyPatch )
            goto ApplyPatch;

        //  make sure lgposScan was set

        Assert( lgposScan.isec > 0 );

        //  we should be sector-aligned

        Assert( lgposScan.ib == 0 );

        //  we should be at or past the end of the good data

        Assert( CmpLgpos( &lgposEnd, &lgposScan ) <= 0 );

        //  determine if there is any data to scan through

        if ( lgposScan.isec >= ( m_pLogStream->CSecLGFile() - 1 ) )
        {

            //  we are at the file size limits of the log file meaning there is nothing left to scan

            //  since there isn't any data left to look through, we must be at the end of the log; that
            //      means that we definitely have a torn-write

            //  we should NEVER EVER point past the end of the log file

            Assert( lgposScan.isec <= ( m_pLogStream->CSecLGFile() - 1 ) );

            //  leave fFoundPattern set to fTrue

            //  skip the scanning loop

            goto SkipScan;
        }

        //  pull in the data we need to scan
        //  (including the extra sector we reserve for the final shadow sector)

        csecScan = m_pLogStream->CSecLGFile() - lgposScan.isec;
        Call( ErrReaderEnsureSector( lgposScan.isec, csecScan, &pbEnsureScan ) );

        //  scan sector by sector looking for the log-extend pattern

        Assert( cbLogExtendPattern >= m_pLogStream->CbSec() );
        pbScan = pbEnsureScan;
        while ( fFoundPattern && csecScan )
        {

            //  does the entire sector match the log-extend pattern?

            if ( _memcmpp( pbScan, rgbLogExtendPattern, cbLogExtendPattern, m_pLogStream->CbSec() ) != 0 )
            {
                //  no; this is not a torn write

                fFoundPattern = fFalse;
                fIsTornWrite = fFalse;

                //  stop scanning

                break;
            }

            //  advance the scan counters

            pbScan += m_pLogStream->CbSec();
            csecScan--;
        }

        //  assume that finding the pattern means we have a torn-write

        fIsTornWrite = fFoundPattern;

#ifdef ENABLE_LOGPATCH_TRACE
        if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
        {
            (*pcprintfLogPatch)( "pattern scan complete -- fFoundPattern = %s\r\n", ( fFoundPattern ? "TRUE" : "FALSE" ) );
        }
#endif  //  ENABLE_LOGPATCH_TRACE

        //  what about case #2 (see comments above) where we must also search for the pattern
        //      within the range of the LRCHECKSUM record itself? this is just to be sure that
        //      the reason the long checksum failed was in fact due to finding the pattern...
        //  NOTE: if the range is only 2 sectors and we crashed in the middle of trying to
        //        overwrite the previous shadow sector, the range will contain only the shadow
        //        sector (when the range is 3 or more sectors, it should contain atleast 1 sector
        //        with the pattern if it is a torn-write case)

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
#endif  //  ENABLE_LOGPATCH_TRACE

            Assert( !fSingleSectorTornWrite );

            //  read the sector with the LRCHECKSUM record

            Call( ErrReaderEnsureSector( lgposCurrent.isec, 1, &pbEnsureScanT ) );
            plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsureScanT + lgposCurrent.ib );
            AssertValidLRCKRecord( plrck, &lgposCurrent );

            //  calculate the number of sectors in its range

            csecScanT = ( ( lgposCurrent.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards - 1 ) / m_pLogStream->CbSec() ) + 1;
            Assert( csecScanT > 1 );

            //  read the entire range

            Call( ErrReaderEnsureSector( lgposCurrent.isec, csecScanT, &pbEnsureScanT ) );
            plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsureScanT + lgposCurrent.ib );
            AssertValidLRCKRecord( plrck, &lgposCurrent );
            AssertInvalidLRCKRange( plrck, lgposCurrent.lGeneration );

            if ( csecScanT == 2 )
            {

                //  this is the special case where we only have 2 sectors; the second sector could be
                //      an old shadow sector, it could be the pattern, or it could be corruption;
                //
                //  if it is an old shadow sector, we must have failed while trying to safely overwrite it
                //      using 2 I/Os (never made it to the second I/O) making this a torn-write case
                //  if we find the pattern, we must have been in the middle of a single I/O and crashed
                //      leaving "holes" of the pattern throughout the range of this LRCHECKSUM record
                //      making this a torn-write case
                //  if we find corruption, we cannot have a torn-write

                plrckT = (LRCHECKSUM *)( (BYTE *)plrck + m_pLogStream->CbSec() );
                if ( FValidLRCKShadowWithoutCheckingCBNext( plrckT, &lgposCurrent, lgposCurrent.lGeneration ) )
                {

                    //  this should only happen in edb.jtx/log -- all other logs should be clean or corrupt
                    //      (they should never have a torn-write)

                    Assert( fJetLog );

                    //  the log records in the shadow sector must match the log records in the new sector

                    if ( memcmp( pbEnsureScanT,
                                 pbEnsureScanT + m_pLogStream->CbSec(),
                                 (BYTE *)plrck - pbEnsureScanT ) == 0 &&
                         memcmp( (BYTE *)plrck + sizeof( LRCHECKSUM ),
                                 (BYTE *)plrckT + sizeof( LRCHECKSUM ),
                                 plrckT->le_cbForwards ) == 0 )
                    {

                        //  make sure the LRCHECKSUM pointers are correct

                        Assert( plrckT->le_cbForwards <= plrck->le_cbForwards );

                        // type cast explicitly for CISCO compiler
                        Assert( (ULONG32)plrckT->le_cbBackwards == (ULONG32)plrck->le_cbBackwards );

                        //  we are now sure that we have a torn-write

                        Assert( fIsTornWrite );
                    }
                    else
                    {

                        //  this is not a torn-write because the log records do not match? this should
                        //      never happen -- especially if we have determined that the shadow sector
                        //      is valid! it seems to suggest that the shadow-sector validation is bad

                        AssertTrack( fFalse, "BadLogShadowSectorValidationLegacy" );

                        //  this is not a torn-write

                        fIsTornWrite = fFalse;
                    }
                }
                else if ( _memcmpp( pbEnsureScanT + m_pLogStream->CbSec(), rgbLogExtendPattern, cbLogExtendPattern, m_pLogStream->CbSec() ) == 0 )
                {

                    //  the sector we are looking at is not a shadow-sector, but it does contain the
                    //      pattern meaning we issued 1 I/O but that I/O never completed; this is
                    //      a torn-write case

                    Assert( fIsTornWrite );
                }
                else
                {

                    //  the sector we are looking at is neither an old shadow or the pattern; it must
                    //      be corruption

                    fIsTornWrite = fFalse;
                }
            }
            else
            {

                //  we have more than 2 sectors to scan
                //  we can either find the pattern in 1 of the sectors making this case a torn-write, OR
                //      we won't find anything making this case corruption

                //  scan the sectors within the range of the LRCHECKSUM record
                //  if we find the log-extend pattern in just one sector, we know that the corruption
                //      was caused by that sector and not real corruption

                csecScanT--;
                pbScanT = pbEnsureScanT + m_pLogStream->CbSec();
                while ( csecScanT )
                {

                    //  see if we have atleast 1 sector with the log-extend pattern

                    if ( _memcmpp( pbScanT, rgbLogExtendPattern, cbLogExtendPattern, m_pLogStream->CbSec() ) == 0 )
                    {

                        //  we have one! this has to be a torn-write

                        Assert( fIsTornWrite );     //  this should be previously set

                        break;
                    }

                    csecScanT--;
                    pbScanT += m_pLogStream->CbSec();
                }

                //  did we find the pattern?

                if ( !csecScanT )
                {

                    //  no; this is not a torn-write

                    fIsTornWrite = fFalse;
                }
            }
        }

SkipScan:

        //  if a torn-write was detected, we may have a single-sector torn-write

        fSingleSectorTornWrite = fSingleSectorTornWrite && fIsTornWrite;

#ifdef ENABLE_LOGPATCH_TRACE
        if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
        {
            (*pcprintfLogPatch)( "torn-write detection complete\r\n" );
            (*pcprintfLogPatch)( "\tfIsTornWrite           = %s\r\n", ( fIsTornWrite ? "TRUE" : "FALSE" ) );
            (*pcprintfLogPatch)( "\tfSingleSectorTornWrite = %s\r\n", ( fSingleSectorTornWrite ? "TRUE" : "FALSE" ) );
        }
#endif  //  ENABLE_LOGPATCH_TRACE

        if ( fIsTornWrite )
        {

            //  we have a torn-write

            Assert( csecScan == 0 );

            //  we should have found the log-extend pattern

            Assert( fFoundPattern );

ApplyPatch:
            //  what recovery mode are we in?

            // SOFT_HARD: leave, at least for compatibility
            if ( m_pLog->FHardRestore() )
            {

                //  we are in hard-recovery mode

                LONG lgenLowRestore, lgenHighRestore;
                m_pLog->LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
                if ( lgposScan.lGeneration <= lgenHighRestore )
                {
#ifdef ENABLE_LOGPATCH_TRACE
                    if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                    {
                        (*pcprintfLogPatch)( "failed to patch torn-write because we are dealing with a logfile in the backup set\r\n" );
                    }
#endif  //  ENABLE_LOGPATCH_TRACE

                    //  report log file corruption to the event log

                    LGIReportChecksumMismatch(
                            m_pinst,
                            m_pLogStream->LogName(),
                            ibOffset,
                            cbLength,
                            ulChecksumExpected,
                            ulChecksumActual,
                            JET_errLogTornWriteDuringHardRestore );

                    //  this generation is part of a backup set -- we must fail

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

                //  the current log generation is not part of the backup-set
                //  we can only patch edb.jtx/log -- all other generations require offline patching
                //      because generations after the bad one need to be deleted

                //  only report an error message when we cannot patch the log file

                if ( !fJetLog || fReadOnlyLocal )
                {
#ifdef ENABLE_LOGPATCH_TRACE
                    if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                    {
                        (*pcprintfLogPatch)( "failed to patch torn-write because %s\r\n",
                                            ( !fJetLog ? "fJetLog==FALSE" : "we are in read-only mode" ) );
                    }
#endif  //  ENABLE_LOGPATCH_TRACE

                    //  report log file corruption to the event log

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

                //  we are in soft-recovery mode

                //UtilReportEvent(  eventWarning,
                //                  LOGGING_RECOVERY_CATEGORY,
                //                  LOG_TORN_WRITE_DURING_SOFT_RECOVERY_ID,
                //                  2,
                //                  rgpsz,
                //                  0,
                //                  NULL,
                //                  m_pinst );

                CallS( err );
            }
            else
            {

                //  we are in read-only mode

                if ( pfIsPatchable )
                {

                    //  the caller wants to know if the log is patchable
                    //  by returning TRUE, we imply that the log is damaged so we don't need to return
                    //      an error as well

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
#endif  //  ENABLE_LOGPATCH_TRACE

                //  report log file corruption to the event log

                LGIReportChecksumMismatch(
                        m_pinst,
                        m_pLogStream->LogName(),
                        ibOffset,
                        cbLength,
                        ulChecksumExpected,
                        ulChecksumActual,
                        JET_errLogFileCorrupt );

                //  return corruption

                Error( ErrERRCheck( JET_errLogFileCorrupt ) );
            }
        }
        else
        {

            //  we have corruption

            if ( csecScan == 0 )
            {

                //  we found the pattern outside the forward range, but not inside it
                //  thus, the corruption was inside the forward range

                Assert( fRecordOkRangeBad );
                OSStrCbFormatW( szCorruption, sizeof( szCorruption ), L"%d", lgposCurrent.isec + 1 );
            }
            else
            {

                //  we did not find the pattern
                //  the corruption must be after the forward range

                Assert( pbScan == m_pLogStream->PbSecAligned( pbScan, m_pbLGBufMin ) );
                Assert( pbEnsureScan == m_pLogStream->PbSecAligned( pbEnsureScan, m_pbLGBufMin ) );
                OSStrCbFormatW( szCorruption, sizeof( szCorruption ), L"%d", lgposScan.isec + (ULONG)( ( pbScan - pbEnsureScan ) / m_pLogStream->CbSec() ) );
            }

            //  what recovery mode are we in?

            // SOFT_HARD: leave, at least for compatibility
            if ( m_pLog->FHardRestore() )
            {
                //  we are in hard-recovery mode

                LONG lgenLowRestore, lgenHighRestore;
                m_pLog->LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
                if ( lgposScan.lGeneration <= lgenHighRestore )
                {
#ifdef ENABLE_LOGPATCH_TRACE
                    if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                    {
                        (*pcprintfLogPatch)( "corruption detected during hard restore (this log is in the backup set)\r\n" );
                    }
#endif  //  ENABLE_LOGPATCH_TRACE

                    //  report log file corruption to the event log

                    LGIReportChecksumMismatch(
                            m_pinst,
                            m_pLogStream->LogName(),
                            ibOffset,
                            cbLength,
                            ulChecksumExpected,
                            ulChecksumActual,
                            JET_errLogCorruptDuringHardRestore );

                    //  this generation is part of a backup set -- we must fail

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
#endif  //  ENABLE_LOGPATCH_TRACE

                //  report log file corruption to the event log

                LGIReportChecksumMismatch(
                        m_pinst,
                        m_pLogStream->LogName(),
                        ibOffset,
                        cbLength,
                        ulChecksumExpected,
                        ulChecksumActual,
                        JET_errLogCorruptDuringHardRecovery );

                //  the current log generation is not part of the backup-set

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
#endif  //  ENABLE_LOGPATCH_TRACE

                //  report log file corruption to the event log

                LGIReportChecksumMismatch(
                        m_pinst,
                        m_pLogStream->LogName(),
                        ibOffset,
                        cbLength,
                        ulChecksumExpected,
                        ulChecksumActual,
                        JET_errLogFileCorrupt );

                //  we are in soft-recovery mode

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
#endif  //  ENABLE_LOGPATCH_TRACE

                //  we are in the dump code

                //  fCreatedLogReader is set when we come from the dump code, it is not
                //  set in redo case, according to comments above ... its sort of 
                //  duplicate w/ the fOldRecoveringMode == fRecoveringNone check ...
                Assert( FNegTest( fCorruptingLogFiles ) || fCreatedLogReader );

                // SOFT_HARD: implied by the above checks on this flag
                Assert( !m_pLog->FHardRestore() );

                //  report log file corruption to the event log

                LGIReportChecksumMismatch(
                        m_pinst,
                        m_pLogStream->LogName(),
                        ibOffset,
                        cbLength,
                        ulChecksumExpected,
                        ulChecksumActual,
                        JET_errLogFileCorrupt );

                //  return corruption

                Error( ErrERRCheck( JET_errLogFileCorrupt ) );
            } // if FHardRestore(), elsif !fReadOnlyLocal, else ...
        } // if fIsTornWrite, else ...
    }
    else // if fDoScan, else ...
    {

        //  we did not need to scan for a torn-write or corruption
        //  however, we should scan the rest of the log to make sure we only see the pattern
        //  (if we find anything, we must assume that the log is corrupt [probably due to an I/O not making it out to disk])

        ULONG       isecScan;
        ULONG       csecScan        = 0;
        BYTE        *pbEnsureScan   = NULL;
        BYTE        *pbScan         = NULL;
        BOOL        fFoundPattern   = fTrue;    //  assume we will find a torn write
        WCHAR       szSector[30];
        WCHAR       szCorruption[30];
        const WCHAR*    rgpsz[3]        = { m_pLogStream->LogName(), szSector, szCorruption };

        //  prepare the event-log messages

        OSStrCbCopyW( szSector, sizeof(szSector), L"END" );
        szCorruption[0] = 0;

        //  we should NEVER EVER point past the end of the log file

        Assert( lgposEnd.isec <= ( m_pLogStream->CSecLGFile() - 1 ) );

        //  get the starting sector for the scan

        isecScan = lgposEnd.isec;
        if ( lgposEnd.ib )
        {
            isecScan++;                 //  round up to the next sector
        }
        isecScan++;                     //  skip the shadow sector

        //  determine if there is any data to scan through

        if ( isecScan < m_pLogStream->CSecLGFile() )
        {

            //  read in the data we need to scan
            //  (including the extra sector we reserve for the final shadow sector)

            Call( ErrReaderEnsureSector( isecScan, m_pLogStream->CSecLGFile() - isecScan, &pbEnsureScan ) );

            //  scan sector by sector looking for the log-extend pattern

            Assert( cbLogExtendPattern >= m_pLogStream->CbSec() );
            pbScan = pbEnsureScan;
            csecScan = 0;
            while ( isecScan + csecScan < m_pLogStream->CSecLGFile() )
            {

                //  does the entire sector match the log-extend pattern?

                if ( _memcmpp( pbScan, rgbLogExtendPattern, cbLogExtendPattern, m_pLogStream->CbSec() ) != 0 )
                {

                    //  no; this is not a torn write

                    fFoundPattern = fFalse;
                    break;
                }

                //  advance the scan counters

                pbScan += m_pLogStream->CbSec();
                csecScan++;
            }

            if ( !fFoundPattern )
            {

                //  we didn't find the pattern -- the log file must be corrupt!

                OSStrCbFormatW( szCorruption, sizeof( szCorruption ), L"%d (0x%08X)", isecScan + csecScan, isecScan + csecScan );

                //  this should only happen to edb.jtx/log...
                //  ... this can happen when doing bit-flip testing on logs
                Assert( fJetLog || FNegTest( fCorruptingLogFiles ) );

                // SOFT_HARD: leave, at least for compatibility
                if ( m_pLog->FHardRestore() )
                {

                    //  we are in hard-recovery mode

                    LONG lgenLowRestore, lgenHighRestore;
                    m_pLog->LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
                    if ( lgposScan.lGeneration <= lgenHighRestore )
                    {
#ifdef ENABLE_LOGPATCH_TRACE
                        if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                        {
                            (*pcprintfLogPatch)( "corruption detected during hard restore (this log is in the backup set) -- data was found after the end of the log!\r\n" );
                        }
#endif  //  ENABLE_LOGPATCH_TRACE

                        //  report log file corruption to the event log

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
#endif  //  ENABLE_LOGPATCH_TRACE

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

                    Call( ErrERRCheck( JET_errLogCorruptDuringHardRecovery ) );
                }
                else if ( !fReadOnlyLocal )
                {
#ifdef ENABLE_LOGPATCH_TRACE
                    if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                    {
                        (*pcprintfLogPatch)( "corruption detected during soft recovery -- data was found after the end of the log!\r\n" );
                    }
#endif  //  ENABLE_LOGPATCH_TRACE

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

                    Call( ErrERRCheck( JET_errLogFileCorrupt ) );
                }
                else
                {
#ifdef ENABLE_LOGPATCH_TRACE
                    if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                    {
                        (*pcprintfLogPatch)( "corruption detected -- data was found after the end of the log!\r\n" );
                    }
#endif  //  ENABLE_LOGPATCH_TRACE

                    //  we are in the dump code

                    // SOFT_HARD: related with the check for it above
                    Assert( !m_pLog->FHardRestore() );
                    Assert( fCreatedLogReader );

                    OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"3aa8328c-8b3d-45a1-93c5-4ea6bea70ad4" );
                    Call( ErrERRCheck( JET_errLogFileCorrupt ) );
                }
            }
        }
        else
        {

            //  there is nothing to scan because we are at the end of the log file

            //  nop
        }
    }

    //  the log should be completely valid or atleast fixable by now
    //      lgposLast is the last LRCHECKSUM record we verified and now trust
    //      lgposEnd is the absolute end of good data (this marks the area we are going to
    //          chop off and treat as garbage -- it include any corrupt data we don't trust)
    //
    //  start searching the last range of log records for markings of a graceful exit:
    //      lrtypTerm, lrtypRecoveryQuit log records are what we want
    //      lrtypNOP, lrtypChecksum records have no effect and are ignored
    //      all other log records indicate a non-graceful exit
    //
    //  NOTE: since we trust the data we are scanning (because we verified it with the checksum),
    //        we assume that every log record will be recognizable

    //  start at the first record after the last good LRCHECKSUM record

    lgposCurrent = lgposLast;
    m_pLogStream->AddLgpos( &lgposCurrent, sizeof( LRCHECKSUM ) );

    //  read in the LRCHECKSUM record

    Call( ErrReaderEnsureSector( lgposLast.isec, 1, &pbEnsure ) );
    plrck = (LRCHECKSUM *)( pbEnsure + lgposLast.ib );
    AssertValidLRCKRecord( plrck, &lgposLast );

    //  get the sector position of the next LRCHECKSUM record (if there is one)
    //
    //  we will use this in the record-to-record scan to determine when we have crossed over
    //      into the backward range of the next LRCHECKSUM record; more specifically, it will
    //      be important when the backward range of the next LRCHECKSUM record is zero even
    //      though there is space there (e.g. not covered by a checksum record)
    //  SPECIAL CASE: when fSkipScanAndApplyPatch is set, we need to bypass using lgposNext
    //                  even though it SHOULD work out (not point in testing unfriendly waters)

    lgposNext = lgposMin;
    if ( plrck->le_cbNext != 0 && !fSkipScanAndApplyPatch )
    {

        //  point to the next LRCHECKSUM record

        lgposNext = lgposLast;
        m_pLogStream->AddLgpos( &lgposNext, sizeof( LRCHECKSUM ) + plrck->le_cbNext );
        Assert( lgposNext.isec > lgposLast.isec );

        //  bring in the next LRCHECKSUM record

        Call( ErrReaderEnsureSector( lgposNext.isec, 1, &pbEnsure ) );
        plrck = (LRCHECKSUM *)( pbEnsure + lgposNext.ib );

        //  if the LRCHECKSUM is invalid or the backward range covers all data,
        //      then we won't need to use lgposNext

        if ( !FValidLRCKRecord( plrck, &lgposNext ) || plrck->le_cbBackwards == lgposNext.ib )
        {
            lgposNext.isec = 0;
        }
        else
        {

            //  there is no need to validate the range to verify the data in the LRCHECKSUM record's
            //      backward range; look at the cases where we could have a valid record and an invalid range:
            //          - multi-sector LRCHECKSUM with a short checksum
            //          - single-sector LRCHECKSUM with no short checksum
            //
            //      in the case of the short checksum, the validation of the LRCHECKSUM record's backward
            //          range is the short checksum itself
            //      in the case of not having a short checksum, we must be looking at:
            //          - a valid range
            //          - an invalid range that was fixed by patching with the shadow sector
            //          ***> there is no chance of a torn-write because that would mean the next LRCHECKSUM
            //               pointer would be aimed at the pattern, and FValidLRCKRecord would have failed
            //          ***> there is no chance of corruption because that would have already been detected
            //               and handled by dispatching an error

            Assert( plrck->le_cbBackwards == 0 );

//
//  NOT ANY MORE -- WE LEAVE THE OLD DATA THERE FOR DEBUGGING PURPOSES
//
//          //  the area before the LRCHECKSUM record should be the pattern
//
//          Assert( _memcmpp( pbEnsure, rgbLogExtendPattern, cbLogExtendPattern, lgposNext.ib ) == 0 );
        }
    }

    //  if the end of data is before this LRCHECKSUM, then we won't need to use lgposNext

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
#endif  //  ENABLE_LOGPATCH_TRACE


    //  loop forever

    forever
    {
        BYTE *  pb              = NULL;
        ULONG   cb              = 0;
        LGPOS   lgposEndOfRec   = { 0 };

        //  bring in an entire log record in three passes
        //      first, bring in the byte for the LRTYP
        //      second, bring in the FIXED portion of the record
        //      third, bring in the entire record

RestartIterations:

        //  have we passed the end of the good data area?

        if ( CmpLgpos( &lgposCurrent, &lgposEnd ) >= 0 )
        {

            //  yes; stop looping

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

            //  point to the end of the data we want

            lgposEndOfRec = lgposCurrent;
            m_pLogStream->AddLgpos( &lgposEndOfRec, cb );

            //  have we passed the end of the good data area?

            if ( CmpLgpos( &lgposEndOfRec, &lgposEnd ) > 0 )
            {

                //  yes; stop looping

                goto DoneRecordScan;
            }

            //  see if we have crossed over into the area before the next LRCHECKSUM record which is
            //      not covered by the backward range

            if ( lgposNext.isec )
            {

                //  we should always land exactly on the sector with the next LRCHECKSUM record
                //      (we can't shoot past it -- otherwise, we would have overwritten it)

                Assert( lgposEndOfRec.isec <= lgposNext.isec );

                //  see if we are in the same sector

                if ( lgposEndOfRec.isec == lgposNext.isec )
                {

                    //  we are pointing into the area before the next LRCHECKSUM record
                    //  this area is not be covered by the backward range, so we need to do a fixup
                    //      on lgposCurrent by moving it to the LRCHECKSUM record itself

                    lgposCurrent = lgposNext;

                    //  do not let this run again

                    lgposNext.isec = 0;

                    //  restart the iterations

                    goto RestartIterations;
                }
            }

            //  read the data we need for this iteration

            Assert( cb > 0 );
            Call( ErrReaderEnsureSector( lgposCurrent.isec, ( lgposCurrent.ib + cb - 1 ) / m_pLogStream->CbSec() + 1, &pbEnsure ) );

            //  set a pointer to it's record type information (1 byte)

            pb = pbEnsure + lgposCurrent.ib;

            //  make sure we recognize the log record

            Assert( *pb >= 0 );
            Assert( *pb < lrtypMax );
        }


        //  we now have a complete log record

        Assert( *pb >= 0 );
        Assert( *pb < lrtypMax );
        Assert( cb == CbLGSizeOfRec( (LR *)pb ) );

        //  check the type

        if ( *pb == lrtypTerm || *pb == lrtypTerm2 || *pb == lrtypRecoveryQuit || *pb == lrtypRecoveryQuit2 )
        {
            //  these types indicate a clean exit

            fGotQuit = fTrue;
            lgposLastTerm = lgposCurrent;
        }
        else if ( *pb == lrtypNOP )
        {
            //  NOPs are ignored and have no influence on the fGotQuit flag
        }
        else if ( *pb == lrtypChecksum )
        {

            //  LRCHECKSUMs are ignored and have no influence on the fGotQuit flag

            //  remember an LRCHECKSUM record as good if we see one

            lgposLast = lgposCurrent;

            //  this should only happen in the case where we have no corruption and no forward range
            //      in the current LRCHECKSUM

            //  we shouldn't have scanned for corruption

            Assert( !fDoScan );

            //  we should find this record right before we hit the end of data

#ifdef DEBUG
            LGPOS lgposT = lgposLast;
            m_pLogStream->AddLgpos( &lgposT, sizeof( LRCHECKSUM ) );
            Assert( CmpLgpos( &lgposT, &lgposEnd ) == 0 );
#endif
        }
        else
        {

            //  we cannot assume a clean exit since the last record we saw was not a Term or a Recovery-Quit

            fGotQuit = fFalse;
        }

        //  set the current log position to the next log record

        lgposCurrent = lgposEndOfRec;
    }

DoneRecordScan:

    if ( fRecordOkRangeBad )
    {

        //  we need to do a fixup in the case where we successfully verified the sector with the LRCHECKSUM
        //      record (using the short checksum value) but could not verify the other sectors in the range
        //      of the LRCHECKSUM record (using the long checksum)
        //
        //  the problem is that the "forward" pointer reaches past the sector boundary of the good sector
        //      (which is holding the LRCHECKSUM record), and, in doing so, it covers bad data
        //
        //  the fix is to shorten the "forward" pointer to include only the good data (up to lgposCurrent)
        //      and update the corrupt area in the log file with this new LRCHECKSUM record

        Assert( !fSingleSectorTornWrite );
        Assert( fDoScan );      //  we should have seen corruption or a torn-write

        //  load the sector with the last good LRCHECKSUM record

        Call( ErrReaderEnsureSector( lgposLast.isec, 1, &pbEnsure ) );
        plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposLast.ib );

        AssertValidLRCKRecord( plrck, &lgposLast );     //  it should be valid
        Assert( plrck->bUseShortChecksum == bShortChecksumOn ); //  it should be a multi-sector range
        Assert( plrck->le_cbBackwards == ULONG( (BYTE *)plrck - m_pLogStream->PbSecAligned( (BYTE *)plrck, m_pbLGBufMin ) )
            || 0 == plrck->le_cbBackwards );
        const UINT cbBackwards      = ULONG( (BYTE *)plrck - m_pLogStream->PbSecAligned( (BYTE *)plrck, m_pbLGBufMin ) );

        //  calculate the byte offset (this may have wrapped around to 0 when we were scanning record-to-record)

        const UINT ib = ( lgposCurrent.ib > 0 ) ? lgposCurrent.ib : m_pLogStream->CbSec();

        //  the current position should be at or past the point in the current sector immediately
        //  following the backward range and the LRCHECKSUM record itself
        //      (lgposCurrent should point at the first bad log record)

        Assert( ib >= cbBackwards + sizeof( LRCHECKSUM ) );

        //  remember that we must overwrite the part of the LRCHECKSUM range that we are "chopping off"

        Assert( 0 == ( lgposLast.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards ) % m_pLogStream->CbSec() );
        const ULONG csecRange = ( lgposLast.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards ) / m_pLogStream->CbSec();
        if ( csecRange > 2 )
        {
            isecPatternFill = lgposLast.isec + 2;   //  skip the sector we are patching and its shadow
            csecPatternFill = csecRange - 2;
        }
        else
        {

            //  the range should only encompass 2 sectors

            //  we don't need to rewrite the logfile pattern because those 2 sectors will be overwritten
            //  when we write the patched sector and its shadow

            Assert( 2 == csecRange );
        }

        //  adjust the "forward" pointer

        plrck->le_cbForwards = ib - ( cbBackwards + sizeof( LRCHECKSUM ) );

        //  there is no data after this LRCHECKSUM record

        plrck->le_cbNext = 0;

        //  copy the last sector into pbLastSector for the next flush

        UtilMemCpy( pbLastSector, pbEnsure, m_pLogStream->CbSec() );

        //  prepare pbLastChecksum and isecLastWrite for the next flush

        pbLastChecksum = pbLastSector + cbBackwards;
        isecLastSector = lgposLast.isec;
    }

    //  we have now completed scanning the last range of log records covered by the
    //      last good LRCHECKSUM record and we know if a graceful exit was previously made (fGotQuit)

    //  set the graceful exit flag for the user

    Assert( pfCloseNormally );
    *pfCloseNormally = fGotQuit;
    if (plgposLastTerm)
    {
        *plgposLastTerm = ( fGotQuit ) ? lgposLastTerm : lgposMin;
    }

    //  set m_lgposLastRec to the lesser of lgposCurrent or lgposEnd for REDO time
    //      (the REDO engine will not exceed this log position)
    //  in most cases, lgposCurrent will be less than lgposEnd and will point to the first BAD log record
    //  in the rare case where we don't have a single good LRCHECKSUM record, lgposCurrent will point past
    //      lgposEnd due to the way we initialized it before entering the search-loop

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
#endif  //  ENABLE_LOGPATCH_TRACE

    //  if this is a single-sector torn-write case, we need to patch the forward/next pointers

    if ( fSingleSectorTornWrite )
    {

        //  the first bad log record should be in the same sector as the checksum record
        //      OR should be aligned perfectly with the start of the next sector

        Assert( ( m_lgposLastRec.isec == lgposLast.isec && CmpLgpos( &lgposLast, &m_lgposLastRec ) < 0 ) ||
                ( m_lgposLastRec.isec == lgposLast.isec + 1 && m_lgposLastRec.ib == 0 ) );

        //  point to the LRCHECKSUM record within the copy of the last sector

        Assert( pbLastChecksum != pbNil );
        plrck = (LRCHECKSUM *)pbLastChecksum;

        //  reset the pointers

        const UINT ibT = ( m_lgposLastRec.ib == 0 ) ? m_pLogStream->CbSec() : m_lgposLastRec.ib;
        Assert( ibT >= lgposLast.ib + sizeof( LRCHECKSUM ) );
        plrck->le_cbForwards = ibT - ( lgposLast.ib + sizeof( LRCHECKSUM ) );
        plrck->le_cbNext = 0;
    }

HandleError:

    //  handle errors and exit the function

    //  cleanup the LogReader class

    if ( fCreatedLogReader )
    {
        ERR errT = JET_errSuccess;

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
    else
    {

        //  we never created the log reader, so we can't destroy it because other users may have pointers to it
        //  instead, we just invalidate its data pointers forcing it go to disk the next time someone does a read

        LReaderReset();
    }

    //  if things went well, we can patch the current log before returning
    //      (we can only do this if we are in read/write mode and the log is edb.jtx/log)

    if ( err >= 0 && !fReadOnlyLocal && fJetLog )
    {

        //  we are not expecting any warnings here

        CallS( err );

        //  do we need to make a new log file?

        if ( fLogToNextLogFile )
        {

            //  since the log creation is deferred, we must setup the buffers here and let the log
            //      flush thread pick up on them later; however, if we didn't create the log reader,
            //      we will lose the contents of our buffers later when the log reader is terminated
            //  thus, we shouldn't bother doing anything until we own the log reader ourselves

            if ( fCreatedLogReader )
            {

                //  the first sector to write is the first sector in the next log file

                m_isecWrite                 = m_pLogStream->CSecHeader();

                //  the write pointer and the last-checksum pointer both start at the beginning of
                //      the log buffer

                m_pbWrite                   = m_pbLGBufMin;
                m_pbLastChecksum            = m_pbLGBufMin;

                //  the entry pointer is the area in the log buffer immediately following the
                //      newly-created LRCHECKSUM record

                m_pbEntry                   = m_pbLastChecksum + sizeof( LRCHECKSUM );

                //  setup the beginning of the new log generation with a single LRCHECKSUM record;
                //      put it at the start of the log buffers

                Assert( pbNil != m_pbLGBufMin );
                memset( m_pbLGBufMin, 0, sizeof( LRCHECKSUM ) );
                plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );
                plrck->lrtyp = lrtypChecksum;

                //  the next flush should go all the way up to (but not including) the newly-created
                //      LRCHECKSUM record

                m_lgposToWrite.isec         = USHORT( m_isecWrite );
                m_lgposToWrite.lGeneration  = m_pLogStream->GetCurrentFileGen() + 1;
                m_lgposToWrite.ib           = 0;

                //  the maximum flush position is the next flush position

                m_lgposMaxWritePoint        = m_lgposToWrite;

                //  setup the rest of the LRCHECKSUM record

                plrck->bUseShortChecksum = bShortChecksumOff;
                plrck->le_ulShortChecksum = 0;
                plrck->le_ulChecksum = UlComputeChecksum( plrck, m_lgposToWrite.lGeneration );

                //  make sure the LRCHECKSUM record is valid

                AssertValidLRCKRecord( plrck, &m_lgposToWrite );
                AssertValidLRCKRange( plrck, m_lgposToWrite.lGeneration );

#ifdef ENABLE_LOGPATCH_TRACE
                if ( FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
                {
                    (*pcprintfLogPatch)( "forcing a new log generation because fLogToNextLogFile=TRUE\r\n" );
                }
#endif  //  ENABLE_LOGPATCH_TRACE

            }
        }
        else if ( pbLastSector != pbNil )
        {
            BYTE    *pbEndOfData, *pbWrite;
            INT     isecWrite;

            //  we need to patch the last sector of the current log

            //  transfer the copy of the last sector into the log buffers

            Assert( pbNil != m_pbLGBufMin );
            UtilMemCpy( m_pbLGBufMin, pbLastSector, m_pLogStream->CbSec() );

            //  calculate the distance of the backward pointer

            const UINT cbBackwards      = ULONG( pbLastChecksum - pbLastSector );

            //  set the last LRCHECKSUM record pointer to the LRECHECKSUM record within the log buffer

            m_pbLastChecksum            = m_pbLGBufMin + cbBackwards;

            //  get a pointer to the copy of the last LRCHECKSUM record

            plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );

            //  the first sector to write was previously determined

            Assert( isecLastSector >= m_pLogStream->CSecHeader() && isecLastSector <= ( m_pLogStream->CSecLGFile() - 1 ) );

            m_isecWrite                 = isecLastSector;

            //  the write pointer is the beginning of the area we just copied into the log buffers
            //      from pbLastSector (it will all need to be flushed again when the LRCHECKSUM record
            //      is updated with a new cbNext, checksum value, etc...)

            m_pbWrite                   = m_pbLGBufMin;

            //  set the entry pointer to the area after the forward range of the LRCHECKSUM record

            m_pbEntry                   = m_pbLastChecksum + sizeof( LRCHECKSUM ) + plrck->le_cbForwards;

            //  the next flush position is the area after the existing validated range of
            //      the LRCHECKSUM record

            m_lgposToWrite.isec         = USHORT( m_isecWrite );
            m_lgposToWrite.lGeneration  = m_pLogStream->GetCurrentFileGen();
            m_lgposToWrite.ib           = 0;
            m_pLogStream->AddLgpos( &m_lgposToWrite, cbBackwards + sizeof( LRCHECKSUM ) + plrck->le_cbForwards );

            //  the maximum flush position is the next flush position

            m_lgposMaxWritePoint        = m_lgposToWrite;

            //  setup the rest of the LRCHECKSUM record

            plrck->bUseShortChecksum = bShortChecksumOff;
            plrck->le_ulShortChecksum = 0;
            plrck->le_ulChecksum = UlComputeChecksum( plrck, m_pLogStream->GetCurrentFileGen() );

            //  make sure the fix to the log is good

#ifdef DEBUG
            LGPOS lgposChecksum;
            lgposChecksum.ib            = (USHORT)cbBackwards;
            lgposChecksum.isec          = (USHORT)m_isecWrite;
            lgposChecksum.lGeneration   = m_pLogStream->GetCurrentFileGen();
            AssertValidLRCKRecord( (LRCHECKSUM *)m_pbLastChecksum, &lgposChecksum );
            AssertValidLRCKRange( (LRCHECKSUM *)m_pbLastChecksum, lgposChecksum.lGeneration );
#endif

            //  capture the data pointers

            pbEndOfData                 = m_pbEntry;
            pbWrite                     = m_pbWrite;
            isecWrite                   = m_isecWrite;

#ifdef ENABLE_LOGPATCH_TRACE
            if ( pcprintfLogPatch && FLGILogPatchDate( wszLogPatchPath, &pcprintfLogPatch ) )
            {
                (*pcprintfLogPatch)( "!!DANGER WILL ROBINSON!! we are about to PATCH the last LRCHECKSUM range\r\n" );

                (*pcprintfLogPatch)( "\tisecPatternFill           = 0x%x\r\n", isecPatternFill );
                (*pcprintfLogPatch)( "\tcsecPatternFill           = 0x%x\r\n", csecPatternFill );
                //(*pcprintfLogPatch)( "\tLOG::m_isecWrite          = 0x%08x\r\n", m_isecWrite );
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
#endif  //  ENABLE_LOGPATCH_TRACE

            //  flush the fixup to the log right now

//
//  SEARCH-STRING: SecSizeMismatchFixMe
//
Assert( m_pLogStream->CbSec() == m_pLogStream->CbSecVolume() );
//          if ( m_pLogStream->CbSec() == m_pLogStream->CbSecVolume() )
//              {
                if ( 0 != csecPatternFill )
                {
                    ULONG cbPatternFillT;
                    ULONG cbT;

                    //  we are chopping off some partial data to patch a torn-write

                    Assert( isecPatternFill >= m_pLogStream->CSecHeader() );
                    Assert( isecPatternFill < m_pLogStream->CSecLGFile() - 1 );
                    Assert( isecPatternFill + csecPatternFill <= m_pLogStream->CSecLGFile() - 1 );
                    Assert( fRecordOkRangeBad );

                    //  rewrite the logfile pattern to "erase" the partial data

                    cbPatternFillT = 0;
                    while ( cbPatternFillT < csecPatternFill * m_pLogStream->CbSec() )
                    {

                        //  compute the size of the next write

                        cbT = min( csecPatternFill * m_pLogStream->CbSec() - cbPatternFillT, cbLogExtendPattern );

                        //  do the write

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

                        //  advance the counters

                        cbPatternFillT += cbT;
                    }
                }

                //  flush the sector and shadow it
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
                    // We modify the checksum by adding a special shadow sector checksum.
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
                    //m_fHaveShadow = fTrue;
                    m_pLogBuffer->GetLgpos( pbEndOfData, &m_lgposToWrite, m_pLogStream );
                }
//              }
//          else
//              {
//              fWriteOnSectorSizeMismatch = fTrue;
//              }
        }
    }

//
//  SEARCH-STRING: SecSizeMismatchFixMe
//
//  if ( err >= 0 && fWriteOnSectorSizeMismatch )
//      {
//
//      //  we tried to write to this log file, but our sector size was wrong
//
//      //  return a general sector-size mismatch error to indicate that the databases are
//      //      not necessarily consistent
//
//      err = ErrERRCheck( JET_errLogSectorSizeMismatch );
//      }

    //  re-open the log file in read-only mode

    if ( !fReadOnlyLocal )
    {
        ERR errT;

        //  check the name of the file

        Assert( NULL != m_pLogStream->LogName() );
        Assert( L'\0' != m_pLogStream->LogName()[ 0 ] );

        //  open the file again

        errT = m_pLogStream->ErrLGOpenFile( NULL, fTrue );

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

    //  free the memory holding the copy of the last sector

    OSMemoryPageFree( pbLastSector );

#ifdef ENABLE_LOGPATCH_TRACE

    //  close the output file

    if ( pcprintfLogPatch )
    {
        delete pcprintfLogPatch;
    }
#endif  //  ENABLE_LOGPATCH_TRACE

    //  return any error code we have

    return err;
}


/*
 *  Read first record pointed by plgposFirst.
 *  Initialize m_pbNext.
 *  The first redo record must be within the good portion
 *  of the log file.
 */

ERR LOG_READ_BUFFER::ErrLGLocateFirstRedoLogRecFF_Legacy(
    LE_LGPOS    *ple_lgposRedo,
    BYTE        **ppbLR )
{
    ERR         err             = JET_errSuccess;
    LGPOS       lgposCurrent    = { 0, USHORT( m_pLogStream->CSecHeader() ), m_pLogStream->GetCurrentFileGen() };
    LRCHECKSUM  * plrck         = pNil;
    BYTE        * pbEnsure      = pbNil;    // start of data we're looking at in buffer
    BYTE        * pb            = pbNil;
    LGPOS       lgposPbNext;
    LGPOS       lgposLastChecksum;

    //  m_lgposLastRec should be set

    Assert( m_lgposLastRec.isec >= m_pLogStream->CSecHeader() &&
            ( m_lgposLastRec.isec < ( m_pLogStream->CSecLGFile() - 1 ) ||
              ( m_lgposLastRec.isec == ( m_pLogStream->CSecLGFile() - 1 ) && m_lgposLastRec.ib == 0 ) ) &&
            m_lgposLastRec.lGeneration == m_pLogStream->GetCurrentFileGen() );

    //  see if we got corruption at the start of the log file
    //      if we have atleast sizeof( LRCHECKSUM ) bytes, we can read the first record which is an
    //      LRCHECKSUM record
    LGPOS lgposHack;
    lgposHack.ib            = sizeof( LRCHECKSUM );
    lgposHack.isec          = USHORT( m_pLogStream->CSecHeader() );
    lgposHack.lGeneration   = m_pLogStream->GetCurrentFileGen();
    if ( CmpLgpos( &m_lgposLastRec, &lgposHack ) < 0 )
    {
        //  lgposHack was past the stopping point
        Call( errLGNoMoreRecords );
    }

    *ppbLR = pbNil;
    m_pbNext = pbNil;
    m_pbLastChecksum = pbNil;
    m_lgposLastChecksum = lgposMin;

    // This is lazy version of ErrLGLocateFirstRedoLogRec() that will only
    // start from the beginning of the log file.
    Assert( ple_lgposRedo->le_isec == m_pLogStream->CSecHeader() );
    Assert( ple_lgposRedo->le_ib == 0 );

    m_lgposLastChecksum.lGeneration = m_pLogStream->GetCurrentFileGen();
    m_lgposLastChecksum.isec        = USHORT( m_pLogStream->CSecHeader() );
    m_lgposLastChecksum.ib          = 0;

    // make sure we've got the right log file loaded up
    Call( ErrReaderEnsureLogFile() );

    // Make sure that we do not ignore a sector read failure (will not overwrite
    // with shadow sector anymore)
    m_fIgnoreNextSectorReadFailure = fFalse;

    // read in start of log file
    Call( ErrReaderEnsureSector( lgposCurrent.isec, 1, &pbEnsure ) );
    m_pbLastChecksum = pbEnsure + lgposCurrent.ib;
    plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );

#ifdef DEBUG
    //  the LRCHECKSUM should always be valid because ErrLGCheckReadLastLogRecordFF will have patched
    //      any and all corruption
    //
    //  just in case...

    if ( !FValidLRCKRecord( plrck, &lgposCurrent ) )
    {
        AssertSz( fFalse, "Unexpected invalid LRCK record!" );

        //  start of log file was corrupted
        Call( ErrERRCheck( JET_errLogFileCorrupt ) );
    }
#endif

    // Pull in sectors that are part of the range
    Call( ErrReaderEnsureSector( lgposCurrent.isec, ( ( lgposCurrent.ib +
        sizeof( LRCHECKSUM ) + plrck->le_cbForwards - 1 ) / m_pLogStream->CbSec() ) + 1,
        &pbEnsure ) );
    m_pbLastChecksum = pbEnsure + lgposCurrent.ib;
    plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );

#ifdef DEBUG
    //  the LRCHECKSUM should always be valid because ErrLGCheckReadLastLogRecordFF will have patched
    //      any and all corruption
    //
    //  just in case...

    if ( !FValidLRCKRange( plrck, m_pLogStream->GetCurrentFileGen() ) )
    {
        //  in a multi-sector flush, it is possible for us to be pointing to an LRCHECKSUM record with
        //      a corrupt range yet still be able to use the data in the first sector because we
        //      verified it with the short checksum

        if ( plrck->bUseShortChecksum != bShortChecksumOn )
        {

            //  no short checksum -- return corruption

            Call( ErrERRCheck( JET_errLogFileCorrupt ) );
        }

        //  verify that the end of data marker is either already at the end of the current sector
        //      or it is a little before the end (e.g. it is at the start of the last log record in
        //      the sector and that log record stretches over into the next sector)

        Assert( ( m_lgposLastRec.isec == m_pLogStream->CSecHeader() && m_lgposLastRec.ib >= sizeof( LRCHECKSUM ) ) ||
                ( m_lgposLastRec.isec == m_pLogStream->CSecHeader() + 1 && m_lgposLastRec.ib == 0 ) );
    }
#endif  //  DEBUG

    // we've got a range in the buffer and it's valid, so let's return
    // the LRCK to the clients.

    // LRCK is start of the file, so it must be there
    Assert( sizeof( LRCHECKSUM ) == CbLGFixedSizeOfRec( reinterpret_cast< const LR* >( pbEnsure ) ) );
    Assert( sizeof( LRCHECKSUM ) == CbLGSizeOfRec( reinterpret_cast< const LR* >( pbEnsure ) ) );

    //  setup return variables

    pb = pbEnsure;
    *ppbLR = m_pbNext = pb;

    //  we should have success at this point

    CallS( err );

    // convert m_pbNext & m_pbLastChecksum to LGPOS for preread.
    // Database preread for this log file starts at the same point
    // as the normal log record processing.
    GetLgposOfPbNext( &lgposPbNext );
    GetLgposOfLastChecksum( &lgposLastChecksum );

    m_pLog->LGPrereadExecute( lgposPbNext, lgposLastChecksum );

    return err;

HandleError:

    //  did we get corruption?

    if ( err == JET_errLogFileCorrupt )
    {

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

    m_pLog->LGPrereadTerm();

    return err;
}

//  reads/verifies the next LRCHECKSUM record so we can get a complete log record that stretches across
//      two LRCHECKSUM ranges
//  reading the entire LRCHECKSUM range into memory ensures that we have the full log record
//      (the full log record can only use the backward range anyway)
//
//  lgposNextRec is the start of the log record which stretches across the LRCHECKSUM boundary

//  Implicit input parameters:
//      m_plread
//          To read in the next CK
//      m_pbLastChecksum
//          The current CK record on input, so we can find the next one
//      m_lgposLastChecksum
//          LGPOS of the current CK record on input, so we can compute location of next
//      m_pbNext
//          Points to earliest record to keep in buffer. Nothing before will
//          be in buffer after we return.

ERR LOG_READ_BUFFER::ErrLGIGetNextChecksumFF(
    // LGPOS of next record we want to return to caller.
    // LGPOS of m_pbNext.
    LGPOS       *plgposNextRec,
    // Points to start of next record we want to return to caller (UNUSED!!!).
    BYTE        **ppbLR )
{
    ERR             err = JET_errSuccess;
    // plrck shadows m_pbLastChecksum so we don't have to cast all over.
    LRCHECKSUM*&    plrck = reinterpret_cast< LRCHECKSUM*& >( m_pbLastChecksum );
    LGPOS           lgposNextChecksum;
    UINT            csec;
    BYTE            *pbEnsure;

    //  m_lgposLastRec should be set

    Assert( m_lgposLastRec.isec >= m_pLogStream->CSecHeader() );
    Assert( m_lgposLastRec.isec < ( m_pLogStream->CSecLGFile() - 1 ) ||
            ( m_lgposLastRec.isec == ( m_pLogStream->CSecLGFile() - 1 ) && m_lgposLastRec.ib == 0 ) );
    Assert( m_lgposLastRec.lGeneration == m_pLogStream->GetCurrentFileGen() );

    //  validate input params

    Assert( plgposNextRec->isec >= m_pLogStream->CSecHeader() );
    Assert( plgposNextRec->isec < ( m_pLogStream->CSecLGFile() - 1 ) );
    Assert( ppbLR != pNil );
#ifdef DEBUG
{
    //  check m_pbNext
    LGPOS lgposPbNext;
    GetLgposOfPbNext( &lgposPbNext );
    Assert( CmpLgpos( &lgposPbNext, plgposNextRec ) == 0 );
}
#endif

    Assert( m_pbLastChecksum != pbNil );

    //  is there another LRCHECKSUM record to read?

    if ( plrck->le_cbNext == 0 )
    {

        //  this is the last LRCHECKSUM record meaning we have a partial record whose starting half is
        //      hanging over the end of the current LRCHECKSUM and whose ending half is ???
        //  the only conclusion is that this must be corruption

        //  set the end of data marker to this log record (we should never move forwards)

        Assert( CmpLgpos( &m_lgposLastRec, plgposNextRec ) >= 0 );
        m_lgposLastRec = *plgposNextRec;

        //  return the error for corruption

        //  do not call OSUHAEmitFailureTag to emit a failure tag yet...  do this in ErrLGGetNextRecFF
        CallR( ErrERRCheck( JET_errLogFileCorrupt ) );
    }

    //  calculate the log position of the next LRCHECKSUM record

    lgposNextChecksum = m_lgposLastChecksum;
    m_pLogStream->AddLgpos( &lgposNextChecksum, sizeof( LRCHECKSUM ) + plrck->le_cbNext );

        //  is the next LRCHECKSUM record beyond the end of data pointer?

        //  we must be able to read up to and including the LRCHECKSUM record

    {
        LGPOS lgposEndOfNextChecksumRec = lgposNextChecksum;
        m_pLogStream->AddLgpos( &lgposEndOfNextChecksumRec, sizeof( LRCHECKSUM ) );

        //  will we pass the end of good data?

        if ( CmpLgpos( &lgposEndOfNextChecksumRec, &m_lgposLastRec ) > 0 )
        {

            //  we cannot safely read this LRCHECKSUM record

            CallR( ErrERRCheck( errLGNoMoreRecords ) );
        }
    }

    //  calculate the number of sectors from the log record we want to the next LRCHECKSUM record

    csec = lgposNextChecksum.isec + 1 - plgposNextRec->isec;
    Assert( csec > 0 );

    //  read from the first sector of the next log record to the first sector of the next LRCHECKSUM record

    CallR( ErrReaderEnsureSector( plgposNextRec->isec, csec, &pbEnsure ) );

    //  now that we have the next LRCHECKSUM record, setup our markers

    m_pbLastChecksum = pbEnsure + ( ( csec - 1 ) * m_pLogStream->CbSec() ) + lgposNextChecksum.ib;
    m_lgposLastChecksum = lgposNextChecksum;

    //  reset the pointer to the next log record (it may have moved due to ErrEnsureSector)

    m_pbNext = pbEnsure + plgposNextRec->ib;

#ifdef DEBUG
    //  validate the next LRCHECKSUM record
    //  NOTE: this should never fail because we pre-scan and patch each log file before this is called!
    //
    //  just in case...

    if ( !FValidLRCKRecord( plrck, &m_lgposLastChecksum ) )
    {
        AssertSz( fFalse, "Unexpected invalid LRCK record!" );

        //  the LRCHECKSUM record is invalid -- we must fail because we cannot read the entire log record

        //  set the end of data marker to the log position of the record we are trying to read in
        //  that way, we shouldn't even try to read it next time -- we certainly shouldn't trust it at all

        m_lgposLastRec = *plgposNextRec;

        //  return the standard corruption error (ErrLGGetNextRecFF will parse this into the right error code)

        CallR( ErrERRCheck( JET_errLogFileCorrupt ) );
    }
#endif

    //  now that we have the new LRCHECKSUM record and it looks good, read the entire range in and validate it

    //  move lgposNextChecksum to the end of the forward range of the LRCHECKSUM record
    //      (lgposNextChecksumEnd should either be farther along in the same sector, or on the boundary
    //       of a later sector)

    m_pLogStream->AddLgpos( &lgposNextChecksum, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );
    Assert( ( lgposNextChecksum.isec == m_lgposLastChecksum.isec &&
              lgposNextChecksum.ib   >  m_lgposLastChecksum.ib ) ||
            ( lgposNextChecksum.isec >  m_lgposLastChecksum.isec &&
              lgposNextChecksum.ib   == 0 ) );

    //  calculate the number of sectors we need for the log record and the new LRCHECKSUM range

    csec = lgposNextChecksum.isec - m_lgposLastChecksum.isec;

    if ( csec > 0 )
    {

        //  read from the first sector of the next log record to the end of the LRCHECKSUM range

        CallR( ErrReaderEnsureSector( plgposNextRec->isec, csec, &pbEnsure ) );

        //  now that we have the next LRCHECKSUM record, setup our markers

        m_pbLastChecksum = pbEnsure + ( ( m_lgposLastChecksum.isec - plgposNextRec->isec ) * m_pLogStream->CbSec() ) + m_lgposLastChecksum.ib;

        //  reset the pointer to the next log record (it may have moved due to ErrEnsureSector)

        m_pbNext = pbEnsure + plgposNextRec->ib;
    }

#ifdef DEBUG
    //  validate the LRCHECKSUM range
    //  NOTE: this should never fail because we pre-scan and patch each log file before this is called!
    //
    //  just in case...

    if ( !FValidLRCKRange( plrck, m_lgposLastChecksum.lGeneration ) )
    {

        //  the LRCHECKSUM range is invalid, but all is not lost!

        //  set the end of data marker to start at the sector with the LRCHECKSUM record

        m_lgposLastRec.ib = 0;
        m_lgposLastRec.isec = m_lgposLastChecksum.isec;
        m_lgposLastRec.lGeneration = m_pLogStream->GetCurrentFileGen();

        //  if we have a short checksum, that means that the sector with the LRCHECKSUM record is salvagable
        //      (but the rest are not)

        if ( plrck->bUseShortChecksum == bShortChecksumOn )
        {

            //  advance the end of data pointer past the salvagable sector

            m_lgposLastRec.isec++;
            Assert( m_lgposLastRec.isec < ( m_pLogStream->CSecLGFile() - 1 ) );

            //  since the first sector of the range is valid, we trust the pointers in the LRCHECKSUM record
            //  the backward pointer could be 0 if we had patched this sector in a previous crash
            //  the forward/next pointers may or may not point ahead to data (in this example they do)
            //
            //  so, we need to realize that even though we trust the data in the backward range which should
            //      be the rest of X, there may be no backward range to trust making Y the next record
            //  also, we need to realize that Y could stretch into the corruption we just found; however,
            //      that case will be handled after we recurse back into ErrLGGetNextRecFF and realize that
            //      log record Y does not end before m_lgposLastRec and is therefore useless as well
            //
            //              0  /---------------\
            //  ----|-------|--|------|--------|------
            //    (X|DADADA(LRCK)(YYYY| doodoo | ???
            //  ----|--------|--------|--------|------
            //               \--------------------/

            //  see if we do in fact need to handle the special "no backward range" case

            if ( plrck->le_cbBackwards != m_lgposLastChecksum.ib )
                goto NoBackwardRange;
        }

        //  return the standard corruption error (ErrLGGetNextRecFF will parse this into the right error code)

        CallR( ErrERRCheck( JET_errLogFileCorrupt ) );
    }
#endif  //  DEBUG

    //  what about when the backward range of the LRCHECKSUM doesn't cover the log record we want to read in?
    //
    //  this indicates that in a previous crash, we sensed that the LRCHECKSUM record or range was invalid, and
    //      we patched it by chopping it off (including the entire sector it was in)
    //
    //  a new LRCHECKSUM record was created with a backward range of 0 to indicate that the data in the
    //      area before the LRCHECKSUM record is not to be trusted; in fact, we fill it with a known pattern
    //
    //  thus, the log record that is hanging between the two LRCHECKSUM ranges is not usable
    //
    //  the next record we are able to replay is the LRCHECKSUM record we just read in
    //
    //  since we already recovered from the last crash (e.g. we made the special LRCHECKSUM record with no
    //      backward range), there SHOULD be a series of undo operations following the LRCHECKSUM record;
    //      (there is no concrete way to tell -- we could have crashed exactly after a commit to level 0)
    //      these will be replayed first, then any other operations will be replayed as well

    if ( plrck->le_cbBackwards != m_lgposLastChecksum.ib )
    {

        //  if this is the last LRCHECKSUM record, we set m_lgposLastRec to the end of the forward range

        if ( plrck->le_cbNext == 0 )
        {
            m_lgposLastRec = lgposNextChecksum;
        }

#ifdef DEBUG
NoBackwardRange:
#endif

        //  the backwards pointer should be 0

        Assert( plrck->le_cbBackwards == 0 );

//
//  THE PATTERN IS NO LONGER PUT HERE SO THAT WE HAVE THE OLD DATA IF WE NEED IT (DEBUGGING PURPOSES)
//
//      //  look for the known pattern
//
//      Assert( _memcmpp( m_pLogStream->PbSecAligned( m_pbLastChecksum, m_pbLGBufMin ), rgbLogExtendPattern, cbLogExtendPattern, m_lgposLastChecksum.ib ) == 0 );

        //  point to the LRCHECKSUM record as the next log record

        m_pbNext = m_pbLastChecksum;
    }

    //  we now have the next LRCHECKSUM range in memory and validated

    //  return success

    CallS( err );
    return err;
}

//  Implicit output parameters
//      m_pbNext
//  via ErrLGIReadMS
//      m_pbNext
//  via LGISearchLastSector
//      m_pbEntry       // not emulated
//      m_lgposLastRec  // emulated

ERR LOG_READ_BUFFER::ErrLGGetNextRecFF_Legacy(
    BYTE **ppbLR )
{
    ERR         err;
    UINT        cbAvail, cbNeed, cIteration;
    LRCHECKSUM  *plrck;
    LGPOS       lgposPbNext, lgposPbNextNew, lgposChecksumEnd;
    BOOL        fDidRead;

    //  initialize variables

    err         = JET_errSuccess;
    fDidRead    = fFalse;

    //  m_lgposLastRec should be set

    Assert( m_lgposLastRec.isec >= m_pLogStream->CSecHeader() &&
            ( m_lgposLastRec.isec < ( m_pLogStream->CSecLGFile() - 1 ) ||
              ( m_lgposLastRec.isec == ( m_pLogStream->CSecLGFile() - 1 ) && m_lgposLastRec.ib == 0 ) ) &&
            m_lgposLastRec.lGeneration == m_pLogStream->GetCurrentFileGen() );

    //  we should have a LogReader created for us

    //  m_pbNext points to the log record we just replayed
    //  get that log position

    Assert( m_pbNext != pbNil );
    GetLgposOfPbNext( &lgposPbNextNew );

    //  save this log position for future reference

    lgposPbNext = lgposPbNextNew;

    //  make sure we don't exceed the end of the replay data

    //  since we already replayed the data at m_pbNext, we should be able to assert that m_pbNext is not
    //      past the end of usable data (if it is, we replayed unusable data -- e.g. GARBAGE)

    AssertSz( ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) < 0 ),
              "We somehow managed to replay garbage, and now, when we are moving past the garbage we just played, we are detecting it for the first time!?! Woops..." );

    //  just in case

    if ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) >= 0 )
    {

        //  this is a serious problem -- we replayed data that was not trusted
        //  the database may be really messed up at this point (I'm surprised the REDO code actually worked)
        //
        //  there is no way to get out of this; we'll have to trust UNDO to try and rectify things
        //      (unless the garbage we replayed happened to be a commit to level 0)
        //
        //  the only thing we can do is stop the REDO code by returning that there are no more log records

        Call( ErrERRCheck( errLGNoMoreRecords ) );
    }

    //  get the size of both the fixed and variable length portions of the current log record

    cbNeed = CbLGSizeOfRec( reinterpret_cast< const LR* >( m_pbNext ) );
    Assert( cbNeed > 0 );

    //  advance m_pbNext past the current log record and on to the next log record

    m_pbNext += cbNeed;

    //  m_pbNext should never suffer from wrap-around (it will always be in the first mapping of the buffers)

    Assert( m_pbNext > m_pbLGBufMin && m_pbNext < m_pbLGBufMax );
    if ( m_pbNext <= m_pbLGBufMin || m_pbNext >= m_pbLGBufMax )
    {
        Call( ErrERRCheck( JET_errLogFileCorrupt ) );
    }

    //  get the log position of the next log record (we will be replaying this next if its ok to replay it)

    GetLgposOfPbNext( &lgposPbNextNew );

    //  the new record should be at a greater log position than the old record

    Assert( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) > 0 );
    if ( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) <= 0 )
    {
        Call( ErrERRCheck( JET_errLogFileCorrupt ) );
    }
    
    //  in fact, the difference in the log positions should be exactly equal to the the size of the old record

    Assert( m_pLogStream->CbOffsetLgpos( lgposPbNextNew, lgposPbNext ) == QWORD( cbNeed ) );
    if ( m_pLogStream->CbOffsetLgpos( lgposPbNextNew, lgposPbNext ) != QWORD( cbNeed ) )
    {
        Call( ErrERRCheck( JET_errLogFileCorrupt ) );
    }
    
    //  see if this was the last log record

    if ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) >= 0 )
    {
        Call( ErrERRCheck( errLGNoMoreRecords ) );
    }

    //  set the pointer to the current LRCHECKSUM record

    Assert( m_pbLastChecksum != pbNil );
    plrck = (LRCHECKSUM *)m_pbLastChecksum;

    //  calculate the end of the current LRCHECKSUM record

    lgposChecksumEnd = m_lgposLastChecksum;
    m_pLogStream->AddLgpos( &lgposChecksumEnd, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );

    //  calculate the available space in the current LRCHECKSUM range so we can see how much data we have
    //      in memory vs. how much data we need to bring in (from the next LRCHECKSUM record)

    Assert( CmpLgpos( &lgposChecksumEnd, &lgposPbNextNew ) >= 0 );
    cbAvail = (UINT)m_pLogStream->CbOffsetLgpos( lgposChecksumEnd, lgposPbNextNew );

    //  move lgposPbNext up to lgposPbNextNew

    lgposPbNext = lgposPbNextNew;

    //  we now have m_pbNext pointing to the next log record (some/all of it might be missing)
    //
    //  we need to bring in the entire log record (fixed and variable length portions) with regard to possible
    //      corruption, and return to the replay code so the record can be redone
    //
    //  do this in 3 iterations:
    //      first, read in the LRTYP
    //      second, using the LRTYP, read in the fixed portion
    //      third, using the fixed portion, read in the variable portion

    for ( cIteration = 0; cIteration < 3; cIteration++ )
    {

        //  decide how much to read for each iteration

        if ( cIteration == 0 )
        {

            //  stage 1 -- bring in the LRTYP (should be 1 byte)

            Assert( sizeof( LRTYP ) == 1 );
            cbNeed = sizeof( LRTYP );
        }
        else if ( cIteration == 1 )
        {

            //  stage 2 -- bring in the fixed portion using the LRTYP

            cbNeed = CbLGFixedSizeOfRec( reinterpret_cast< const LR * >( m_pbNext ) );
        }
        else
        {

            //  stage 3 -- bring in the dynamic portion using the fixed portion

            Assert( cIteration == 2 );
            cbNeed = CbLGSizeOfRec( reinterpret_cast< const LR * >( m_pbNext ) );
        }

        //  advance lgposPbNextNew to include the data we need

        lgposPbNextNew = lgposPbNext;
        m_pLogStream->AddLgpos( &lgposPbNextNew, cbNeed );

        //  make sure we don't exceed the end of the replay data

        if ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) > 0 )
        {
            Call( ErrERRCheck( errLGNoMoreRecords ) );
        }

        //  do we have the data we need?

        if ( cbAvail < cbNeed )
        {

            //  no; we need the next LRCHECKSUM range

            //  we should only be doing 1 read per iteration of this function

            Assert( !fDidRead );

            //  do the read

            err = ErrLGIGetNextChecksumFF( &lgposPbNext, ppbLR );
            fDidRead = fTrue;

            //  m_pbNext may have moved because the log record we wanted was partially covered by the next
            //      LRCHECKSUM's backward range and that LRCHECKSUM had no backward range
            //  we need to refresh our currency on m_pbNext to determine exactly what happened

            //  m_pbNext should never suffer from wrap-around (it will always be in the first mapping of the buffers)

            Assert( m_pbNext > m_pbLGBufMin && m_pbNext < m_pbLGBufMax );
            if ( m_pbNext <= m_pbLGBufMin || m_pbNext >= m_pbLGBufMax )
            {
                Call( ErrERRCheck( JET_errLogFileCorrupt ) );
            }

            //  get the new log position m_pbNext

            GetLgposOfPbNext( &lgposPbNextNew );

            //  m_pbNext should not have moved backwards

            Assert( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) >= 0 );
            if ( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) < 0 )
            {
                Call( ErrERRCheck( JET_errLogFileCorrupt ) );
            }

            //  did m_pbNext move?

            if ( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) > 0 )
            {

                //  yes, meaning that the LRCHECKSUM range we read in had no backward range, so we skipped
                //      to the next good record

                //  this should be a successful case

                CallS( err );

                //  return the log record

                goto HandleError;
            }

            //  did we get corruption?

            if ( err == JET_errLogFileCorrupt )
            {

                //  there was corruption in the next LRCHECKSUM range
                //  this should NEVER happen because ErrLGCheckReadLastLogRecordFF should have patched this up!

                //  m_lgposLastRec should point to the end of the last LRCHECKSUM's range or the end of the first
                //      sector in the new LRCHECKSUM we just read

                Assert( CmpLgpos( &m_lgposLastRec, &lgposChecksumEnd ) == 0 ||
                        ( m_lgposLastRec.ib == lgposChecksumEnd.ib &&
                          m_lgposLastRec.isec == lgposChecksumEnd.isec + 1 &&
                          m_lgposLastRec.lGeneration == lgposChecksumEnd.lGeneration ) );

                //  did we get any data anyway? we may have if we had a valid short checksum; m_lgposLastRec will be
                //      one full sector past lgposChecksumEnd if we did get more data

                m_pLogStream->AddLgpos( &lgposChecksumEnd, m_pLogStream->CbSec() );
                if ( CmpLgpos( &m_lgposLastRec, &lgposChecksumEnd ) == 0 )
                {

                    //  we got more data

                    //  it should be enough for the whole log record because we should have atleast gotten up to
                    //      and including the LRCHECKSUM (meaning the entire backward range) and the missing
                    //      portion of the log record must be in the backward range

                    err = JET_errSuccess;
                    goto GotMoreData;
                }

                //  we did not get any more data; handle the corruption error
                OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"d65b6db2-ef29-4371-a518-ea0d96b0b42c" );
                Call( err );
            }
            else if ( err != JET_errSuccess )
            {

                //  another error occurred -- it could be errLGNoMoreRecords (we do not expect warnings)

                Assert( err < 0 );
                Call( err );
            }

GotMoreData:
            //  we have enough data to continue

            //  since we read a new LRCHECKSUM range, we need to refresh some of our internal variables

            //  set the pointer to the next LRCHECKSUM record

            Assert( m_pbLastChecksum != pbNil );
            plrck = (LRCHECKSUM *)m_pbLastChecksum;

            //  if lgposChecksumEnd was not setup for the new LRCHECKSUM record, set it up now
            //      (it may have been set to the end of the first sector in the case of corruption)

            if ( CmpLgpos( &lgposChecksumEnd, &m_lgposLastChecksum ) <= 0 )
            {
                lgposChecksumEnd = m_lgposLastChecksum;
                m_pLogStream->AddLgpos( &lgposChecksumEnd, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );
            }

            //  recalculate the amount of available space as the number of bytes from the current
            //      log record to the start of the newly read LRCHECKSUM record

            cbAvail = (UINT)m_pLogStream->CbOffsetLgpos( m_lgposLastChecksum, lgposPbNextNew );

            //  if cbAvail == 0, then the next log record must be the new LRCHECKSUM record because
            //      we calculated cbAvail as the number of bytes BEFORE the LRCHECKSUM record
            //      (e.g. there is nothing before the LRCHECLSUM record to get)

            if ( cbAvail == 0 )
            {

                //  we must be looking at an LRCHECKSUM record

                Assert( *m_pbNext == lrtypChecksum );

                //  we should be on the first iteration because we shouldn't have had a partial
                //      LRCHECKSUM record hanging over the edge of a sector; it would have been
                //      sector aligned using lrtypNOP records

                Assert( cIteration == 0 );

                //  m_pbNext should be sector-aligned

                Assert( lgposPbNext.ib == 0 );
                Assert( m_pbNext == m_pLogStream->PbSecAligned( m_pbNext, m_pbLGBufMin ) );

                //  set cbAvail to be the size of the LRCHECKSUM record

                cbAvail = sizeof( LRCHECKSUM );
            }

            //  we should now have the data we need

            Assert( cbAvail >= cbNeed );
        }

        //  iteration complete -- loop back for next pass
    }

    //  we now have the log record in memory and validated

    Assert( cIteration == 3 );

    //  we should also have enough data to cover the entire log record

    Assert( cbAvail >= CbLGSizeOfRec( reinterpret_cast< const LR * >( m_pbNext ) ) );

    //  we should have no errors or warnings at this point

    CallS( err );

HandleError:

    //  setup the pointers in all cases (error or success)

    *ppbLR = m_pbNext;

    //  did we get corruption? we NEVER should because ErrLGCheckReadLastLogRecordFF patches it first!

    if ( err == JET_errLogFileCorrupt )
    {

        //  what recovery mode are we in?

        // SOFT_HARD: ???
        if ( m_pLog->FHardRestore() )
        {

            //  we are in hard-recovery mode

            LONG lgenLowRestore, lgenHighRestore;
            m_pLog->LGGetRestoreGens( &lgenLowRestore, &lgenHighRestore );
            if ( m_pLogStream->GetCurrentFileGen() <= lgenHighRestore )
            {

                //  this generation is part of a backup set

                Assert( m_pLogStream->GetCurrentFileGen() >= lgenLowRestore );
                err = ErrERRCheck( JET_errLogCorruptDuringHardRestore );
            }
            else
            {

                //  the current log generation is not part of the backup-set

                err = ErrERRCheck( JET_errLogCorruptDuringHardRecovery );
            }
        }
        else
        {

            //  we are in soft-recovery mode or in the dump code
            //  keep the original error JET_errLogFileCorrupt

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

    // Takes advantage of VM wrap-around

    Assert( reinterpret_cast< const BYTE* >( plrck ) >= m_pbLGBufMin &&
        reinterpret_cast< const BYTE* >( plrck ) < m_pbLGBufMax );

    pb = reinterpret_cast< const BYTE* >( plrck ) - plrck->le_cbBackwards;
    pbEnd = reinterpret_cast< const BYTE* >( plrck );

    Assert( pb >= m_pbLGBufMin && pb < m_pbLGBufMax );
    Assert( pbEnd >= m_pbLGBufMin && pbEnd < m_pbLGBufMax );

    Assert( ( pbEnd - pb == 0 ) ? fTrue : m_pLogBuffer->FIsUsedSpace( pb, ULONG( pbEnd - pb ) ) );
    Assert( pbEnd >= pb );

    //  checksum the "backward" range
    ulChecksum = LGChecksum::UlChecksumBytes( pb, pbEnd, ulChecksum );

    pb = reinterpret_cast< const BYTE* >( plrck ) + sizeof( *plrck );
    pbEnd = reinterpret_cast< const BYTE* >( plrck )
        + sizeof( *plrck ) + plrck->le_cbForwards;

    // Stay in region of the 2 mappings
    Assert( pb >= ( m_pbLGBufMin + sizeof( *plrck ) ) );
    Assert( pb <= m_pbLGBufMax );
    Assert( pbEnd > m_pbLGBufMin && pbEnd <= ( m_pbLGBufMax + m_cbLGBuf ) );

    Assert( ( pbEnd - pb == 0 ) ? fTrue
                                : m_pLogBuffer->FIsUsedSpace( ( pb == m_pbLGBufMax )    ? m_pbLGBufMin
                                                                        : pb,
                                                ULONG( pbEnd - pb ) ) );
    Assert( pbEnd >= pb );

    //  checksum the "forward" range
    ulChecksum = LGChecksum::UlChecksumBytes( pb, pbEnd, ulChecksum );

    //  checksum the backward/forward/next pointers
    ulChecksum ^= plrck->le_cbBackwards;
    ulChecksum ^= plrck->le_cbForwards;
    ulChecksum ^= plrck->le_cbNext;

    //  checksum the short checksum on/off byte
    Assert( plrck->bUseShortChecksum == bShortChecksumOn ||
            plrck->bUseShortChecksum == bShortChecksumOff );
    ulChecksum ^= (ULONG32)plrck->bUseShortChecksum;

    // The checksum includes the log generation number, so we can't
    // eat old log data from a previous generation and think that it is current data.
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
    // the region should start on a sector boundary, or plrck->cbBackwards can be zero
    Assert( m_pLogStream->PbSecAligned( const_cast< BYTE* >( reinterpret_cast< const BYTE* >( plrck ) ), m_pbLGBufMin ) == pb || 0 == plrck->le_cbBackwards );
    pbEnd = reinterpret_cast< const BYTE* >( plrck );

    Assert( pb >= m_pbLGBufMin && pb < m_pbLGBufMax );
    Assert( pbEnd >= m_pbLGBufMin && pbEnd < m_pbLGBufMax );

    // Can't call without m_critLGBuf
//  Assert( pbEnd - pb == 0 ? fTrue : FIsUsedSpace( pb, pbEnd - pb ) );
    Assert( pbEnd >= pb );

    //  checksum the "backward" region
    ulChecksum = LGChecksum::UlChecksumBytes( pb, pbEnd, ulChecksum );

    pb = reinterpret_cast< const BYTE* >( plrck ) + sizeof( *plrck );
    // end of short checksum region is the end of the sector.
    pbEnd = m_pLogStream->PbSecAligned( const_cast< BYTE* >( reinterpret_cast< const BYTE* >( plrck ) ), m_pbLGBufMin ) + m_pLogStream->CbSec();

    // Stay in region of the 2 mappings
    Assert( pb >= ( m_pbLGBufMin + sizeof( *plrck ) ) );
    Assert( pb <= m_pbLGBufMax );
    Assert( pbEnd > m_pbLGBufMin && pbEnd <= m_pbLGBufMax );

    // Can't call without m_critLGBuf
//  Assert( pbEnd - pb == 0 ? fTrue : FIsUsedSpace(
//      ( pb == _pbLGBufMax ) ? _pbLGBufMin : pb,
//      pbEnd - pb ) );
    Assert( pbEnd >= pb );

    //  checksum to the end of the sector (forward region and any garbage in there)
    ulChecksum = LGChecksum::UlChecksumBytes( pb, pbEnd, ulChecksum );

    //  checksum the backward/forward/next pointers
    ulChecksum ^= plrck->le_cbBackwards;
    ulChecksum ^= plrck->le_cbForwards;
    ulChecksum ^= plrck->le_cbNext;

    //  checksum the short checksum on/off byte
    Assert( plrck->bUseShortChecksum == bShortChecksumOn ||
            plrck->bUseShortChecksum == bShortChecksumOff );
    ulChecksum ^= (ULONG32)plrck->bUseShortChecksum;

    // The checksum includes the log generation number, so we can't
    // eat old log data from a previous generation and think that it is current data.
    ulChecksum ^= lGeneration;

    return ulChecksum;
}

#endif  // ENABLE_LOG_V7_RECOVERY_COMPAT

VOID
CHECKSUMINCREMENTAL::ChecksumBytes( const BYTE* const pbMin, const BYTE* const pbMax )
{
    const UINT      cbitsPerBytes = 8;
    const ULONG32   ulChecksum = LGChecksum::UlChecksumBytes( pbMin, pbMax, 0 );
    const ULONG32   ulChecksumAdjusted = _rotl( ulChecksum, m_cLeftRotate );
    m_cLeftRotate = ( ULONG( ( pbMax - pbMin ) % sizeof( ULONG32 ) ) * cbitsPerBytes + m_cLeftRotate ) % ( sizeof( ULONG32 ) * cbitsPerBytes );

    m_ulChecksum ^= ulChecksumAdjusted;
}

//  ================================================================
ULONG32 LGChecksum::UlChecksumBytes( const BYTE * const pbMin, const BYTE * const pbMax, const ULONG32 ulSeed )
//  ================================================================
//
//  The entire aligned NATIVE_WORD containing pbMin and pbMax must be accessible
//
//  For speed we do the checksum NATIVE_WORD aligned. UlChecksum would end up containing the
//  checksum for all the NATIVE_WORDs containing the bytes we want to checksum, possibly
//  including some unwanted bytes at the start and end. To strip out the unwanted bytes
//  we compute a mask to XOR in. The BYTES we want to keep are replaced in the mask by 0,
//  otherwise we use the original bytes.
//
//  At the end we rotate the checksum to take into account the alignment of the start byte.
//
{
//  Assert( FHostIsLittleEndian() );
    Assert( pbMin <= pbMax );

    //  special case
    if ( pbMin == pbMax )
        return ulSeed;

    const NATIVE_WORD cBitsInByte = 8;
    const INT cLoop = 16; /* 8 */

    //  HACK: cast to DWORD_PTR then to NATIVE_WORD* in order to permit compiling with /Wp64
    //

    //  round down to the start of the NATIVE_WORD containing the start pointer
    const NATIVE_WORD * pwMin = (NATIVE_WORD *)(DWORD_PTR) ( ( (NATIVE_WORD)pbMin / sizeof( NATIVE_WORD ) ) * sizeof( NATIVE_WORD ) );

    //  round up to the start of the next NATIVE_WORD that does not contain any of the end pointer
    const NATIVE_WORD * const pwMax = (NATIVE_WORD *)(DWORD_PTR) ( ( ( (NATIVE_WORD)pbMax + sizeof( NATIVE_WORD ) - 1 ) / sizeof( NATIVE_WORD ) ) * sizeof( NATIVE_WORD ) );

    Assert( pwMin < pwMax );

    //  calculate the number of bits in the last NATIVE_WORD that we do _not_ want
    const NATIVE_WORD cbitsUnusedLastWord = ( sizeof( NATIVE_WORD ) - ( (NATIVE_WORD)pbMax % sizeof( NATIVE_WORD ) ) ) * cBitsInByte * ( (NATIVE_WORD)pwMax != (NATIVE_WORD)pbMax );
    Assert( cbitsUnusedLastWord < sizeof( NATIVE_WORD ) * cBitsInByte );
    const NATIVE_WORD wByteMaskLast = ~( (NATIVE_WORD)(~0) >> cbitsUnusedLastWord );

    //  calculate the number of bits in the first NATIVE_WORD that we _do_ want
    const NATIVE_WORD cbitsUsedFirstWord = ( sizeof( NATIVE_WORD ) - ( (NATIVE_WORD)pbMin % sizeof( NATIVE_WORD ) ) ) * cBitsInByte;
    Assert( cbitsUsedFirstWord > 0 );

    //  strip out the unused bytes in the first NATIVE_WORD. do this first to avoid cache misses
    //  take ~0 to get 0xffff...
    //  right shift to get zeroes in the bytes we want to remove
    //  OR with the original NATIVE_WORD
    // right shifting by 32 is undefined, so do it in two steps
    const NATIVE_WORD wByteMaskFirst    = (NATIVE_WORD)(~0) >> ( cbitsUsedFirstWord - 1 ) >> 1;
    const NATIVE_WORD wFirst            = *(LittleEndian<NATIVE_WORD>*)pwMin;
    const NATIVE_WORD wMaskFirst        = wByteMaskFirst & wFirst;

    NATIVE_WORD wChecksum = 0;
    NATIVE_WORD wChecksumT = 0;

#pragma warning( suppress : 6305 )
    const ULONG cw = ULONG( pwMax - pwMin );
#pragma warning( suppress : 6305 )
    pwMin -= ( ( cLoop - ( cw % cLoop ) ) & ( cLoop - 1 ) );

    //  '^' can be calculated with either endian format. Convert
    //  the checksum result for BE at the end instead of convert for
    //  each word.

    if( 8 == cLoop )
    {
        switch ( cw % cLoop )
        {
            while ( 1 )
            {
                case 0: //  we can put this at the top because pdwMax != pdwMin
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
                case 0: //  we can put this at the top because pdwMax != pdwMin
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

    //  It is calculated in little endian form, convert to machine
    //  endian for other arithmatic operations.

    wChecksum = ReverseBytesOnBE( wChecksum );

    //  Take the first unaligned portion into account

    wChecksum ^= wMaskFirst;

    //  strip out the unused bytes in the last NATIVE_WORD. do this last to avoid cache misses
    //  take ~0 to get 0xffff..
    //  right shift to get zeroes in the bytes we want to keep
    //  invert (make the zeroes 0xff)
    //  OR with the original NATIVE_WORD
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

    //  we want the checksum we would have gotten if we had done this unaligned
    //  simply rotate the checksum by the appropriate number of bytes
    ulChecksum = _rotl( ulChecksum, ULONG( cbitsUsedFirstWord ) );

    //  now we have rotated, we can XOR in the seed
    ulChecksum ^= ulSeed;

    return ulChecksum;
}


#ifndef RTM

//  ================================================================
ULONG32 LGChecksum::UlChecksumBytesNaive( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed )
//  ================================================================
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


//  ================================================================
ULONG32 LGChecksum::UlChecksumBytesSlow( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed )
//  ================================================================
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


//  ================================================================
BOOL LGChecksum::TestChecksumBytesIteration( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed )
//  ================================================================
{
    const ULONG32 ulChecksum        = UlChecksumBytes( pbMin, pbMax, ulSeed );
    const ULONG32 ulChecksumSlow    = UlChecksumBytesSlow( pbMin, pbMax, ulSeed );
    const ULONG32 ulChecksumNaive   = UlChecksumBytesNaive( pbMin, pbMax, ulSeed );

    CHECKSUMINCREMENTAL ck;
    ck.BeginChecksum( ulSeed );
    ck.ChecksumBytes( pbMin, pbMax );
    const ULONG32 ulChecksumIncremental = ck.EndChecksum();

    const BYTE* const pbMid = pbMin + ( ( pbMax - pbMin ) + 2 - 1 ) / 2;    // round up to at least 1 byte if cb == 1
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
//           ulChecksum == ulChecksumSOMEONE &&
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

//  128 bytes total
};


//  ================================================================
BOOL LGChecksum::TestChecksumBytes()
//  ================================================================
{

    //  perform all possible iterations of checksumming
    //
    //  assume N is the number of bytes in the array
    //  the resulting number of iterations is approximately: ((n)(n+1)) / 2
    //
    //  for 128 bytes, there are about 16,500 iterations, each of decreasing length
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


#endif  //  !RTM

