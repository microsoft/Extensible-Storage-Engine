// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef LV_H_INCLUDED
#error: Already included
#endif
#define LV_H_INCLUDED

//  ****************************************************************
//  CONSTANTS
//  ****************************************************************

const ULONG ulLVOffsetFirst = 0;

//  LV preread can be performed with RetrieveColumn or PrereadIndexRanges.  
//  If a grbit is passed asking for only the first page, then just one page of LV is read.
//  However, if RetrievePrereadMany is passed then 32 pages of LV will be retrieved.
//  If neither grbit is passed then 8 pages of LV will be preread.
//
#define clidMostPreread         256
#define cpageLVPrereadDefault   16
#define cpageLVPrereadMany      32

const INT cbMinChiSquared = 128;        // The minmum number of bytes of data needed to run chi-squared with good results.
const INT cbChiSquaredSample = 1024;    // The maximum bytes to sample for running the chi-squared significance test.
const double dChiSquaredThreshold = 500.0;  // The minimum chi-squared value required to compress.


//  ****************************************************************
//  STRUCTURES
//  ****************************************************************

#include <pshpack1.h>

//  ================================================================
PERSISTED
struct LVKEY64
//  ================================================================
//
//  The nodes in the Long Value tree use a 64-bit longID and a 32-bit offset as
//  keys. We must have the lid first as this is all we store for LVROOTs.
//  The LVKEY_BUFFER::fLid64 flag distinguishes 64-bit LID based keys 
//  from the legacy 32-bit LID based keys. Also see FIsLid64()/FIsLid32().
//
//-
{
    UnalignedBigEndian< _LID64 >    lid;
    UnalignedBigEndian< ULONG > offset;
};

//  ================================================================
PERSISTED
struct LVKEY32
//  ================================================================
//  The older LV format required that the Long Value tree use a
//  32-bit longID and a 32-bit offset as keys.
//
//-
{
    UnalignedBigEndian< _LID32 >    lid;
    UnalignedBigEndian< ULONG > offset;
};

static_assert( sizeof( LVKEY64 ) == 12, "LVKEY64 must be 12 bytes !" );
static_assert( sizeof( LVKEY32 ) ==  8, "LVKEY32 must be 8 bytes !" );

#include <poppack.h>


//  ================================================================
PERSISTED
struct LVKEY_BUFFER
//  ================================================================
//  Helper struct for dealing with persisted LV keys.
//  The first byte is the high-byte of the lid (big-endian format).
//  This byte stores various flags used by the LV format.
// -
{
    union
    {
        union
        {
            struct
            {
                ULONG reserved      : 7;
                ULONG fLid64        : 1;
            };
            BYTE flags;
        };
        BYTE rgb[ sizeof( LVKEY64 ) ];
    };
};

static_assert( sizeof( LVKEY_BUFFER ) >= sizeof( LVKEY32 ), "LVKEY_BUFFER must be able to store LVKEY32" );
static_assert( sizeof( LVKEY_BUFFER ) >= sizeof( LVKEY64 ), "LVKEY_BUFFER must be able to store LVKEY64" );


//  ================================================================
INLINE bool FIsLVRootKey( const KEY& key )
//  ================================================================
{
    if ( key.Cb() == sizeof( LVKEY64 ) )
    {
#ifdef DEBUG
        // Check the highest bit of the key for the LID64 flag
        LVKEY_BUFFER lvkeyBuff;
        key.CopyIntoBuffer( &lvkeyBuff, sizeof( lvkeyBuff ) );
        Assert( lvkeyBuff.fLid64 || g_fRepair );
#endif
        return false;   //  it can only be a 64-bit chunk key
    }
    else if ( key.Cb() == sizeof( LVKEY32 ) )
    {
        // It can be a 64-bit LV root key, or a legacy 32-bit chunk key.
        // If the MSB of the first byte is set, it's a 64-bit root key.
        LVKEY_BUFFER lvkeyBuff;
        key.CopyIntoBuffer( &lvkeyBuff, sizeof( lvkeyBuff ) );
        return !!lvkeyBuff.fLid64;
    }
    else if ( key.Cb() == sizeof( _LID32 ) )
    {
#ifdef DEBUG
        // Check the highest bit of the key for the LID64 flag
        LVKEY_BUFFER lvkeyBuff;
        key.CopyIntoBuffer( &lvkeyBuff, sizeof( lvkeyBuff ) );
        Assert( !lvkeyBuff.fLid64 || g_fRepair );
#endif
        return true;
    }
    else
    {
        AssertTrack( false, "LvNodeKeySizeMismatch" );
        return false;
    }
}


//  ================================================================
INLINE bool FIsLVChunkKey( const KEY& key )
//  ================================================================
{
    if ( key.Cb() == sizeof( LVKEY64 ) )
    {
#ifdef DEBUG
        // Check the highest bit of the key for the LID64 flag
        LVKEY_BUFFER lvkeyBuff;
        key.CopyIntoBuffer( &lvkeyBuff, sizeof( lvkeyBuff ) );
        Assert( lvkeyBuff.fLid64 || g_fRepair );
#endif
        return true;    //  it can only be a 64-bit chunk key
    }
    else if ( key.Cb() == sizeof( LVKEY32 ) )
    {
        // It can be a 64-bit LV root key, or a legacy 32-bit chunk key.
        // If the MSB of the first byte is set, it's a 64-bit root key.
        LVKEY_BUFFER lvkeyBuff;
        key.CopyIntoBuffer( &lvkeyBuff, sizeof( lvkeyBuff ) );
        return !lvkeyBuff.fLid64;
    }
    else if ( key.Cb() == sizeof( _LID32 ) )
    {
#ifdef DEBUG
        // Check the highest bit of the key for the LID64 flag
        LVKEY_BUFFER lvkeyBuff;
        key.CopyIntoBuffer( &lvkeyBuff, sizeof( lvkeyBuff ) );
        Assert( !lvkeyBuff.fLid64 || g_fRepair );
#endif
        return false;
    }
    else
    {
        AssertTrack( false, "LvChunkKeySizeMismatch" );
        return false;
    }
}


//  ================================================================
INLINE void LVRootKeyFromLid( LVKEY_BUFFER* const plvkeyBuff, KEY* const pkey, LvId lid )
//  ================================================================
{
    Assert( plvkeyBuff );
    Assert( pkey );
    Assert( lid > lidMin );

    pkey->prefix.Nullify();
    pkey->suffix.SetPv( plvkeyBuff );
    if ( lid.FIsLid64() )
    {
        LVKEY64* const plvkey = (LVKEY64* const) plvkeyBuff;
        plvkey->lid = lid.ToLid64();    // converts to UnalignedBigEndian<>
        pkey->suffix.SetCb( sizeof( _LID64 ) );
    }
    else
    {
        Assert( lid < lid32Max );
        LVKEY32* const plvkey = (LVKEY32* const) plvkeyBuff;
        plvkey->lid = lid.ToLid32();    // converts to UnalignedBigEndian<>
        pkey->suffix.SetCb( sizeof( _LID32 ) );
    }
}


//  ================================================================
INLINE void LVKeyFromLidOffset( LVKEY_BUFFER* const plvkeyBuff, KEY* const pkey, LvId lid, ULONG ulOffset )
//  ================================================================
{
    Assert( plvkeyBuff );
    Assert( pkey );
    Assert( lid > lidMin );

    pkey->prefix.Nullify();
    pkey->suffix.SetPv( plvkeyBuff );
    if ( lid.FIsLid64() )
    {
        LVKEY64* const plvkey = (LVKEY64* const) plvkeyBuff;
        plvkey->lid = lid.ToLid64();    // converts to UnalignedBigEndian<>
        plvkey->offset = ulOffset;
        pkey->suffix.SetCb( sizeof( LVKEY64 ) );
    }
    else
    {
        Assert( lid < lid32Max );
        LVKEY32* const plvkey = (LVKEY32* const) plvkeyBuff;
        plvkey->lid = lid.ToLid32();    // converts to UnalignedBigEndian<>
        plvkey->offset = ulOffset;
        pkey->suffix.SetCb( sizeof( LVKEY32 ) );
    }
}


//  ================================================================
INLINE VOID OffsetFromKey( ULONG * pulOffset, const KEY& key )
//  ================================================================
{
    Assert( pulOffset );

    LVKEY_BUFFER lvkeyBuff;
    key.CopyIntoBuffer( &lvkeyBuff, sizeof( lvkeyBuff ) );

    if ( sizeof( LVKEY64 ) == key.Cb() )
    {
        LVKEY64* plvkey = (LVKEY64*) &lvkeyBuff;
        Assert( LvId::FIsLid64( plvkey->lid ) );
        *pulOffset = plvkey->offset;
    }
    else if ( sizeof( LVKEY32 ) == key.Cb() )
    {
        LVKEY32* plvkey = (LVKEY32*) &lvkeyBuff;
        Assert( LvId::FIsLid32( plvkey->lid ) );
        *pulOffset = plvkey->offset;
    }
    else
    {
        *pulOffset = 0;
        AssertTrack( sizeof( LVKEY64 ) == key.Cb() || sizeof( LVKEY32 ) == key.Cb(), "LvChunkKeySizeMismatch" );
    }
}


//  ================================================================
INLINE VOID LidFromKey( LvId* plid, const KEY& key )
//  ================================================================
{
    Assert( plid );

    LVKEY_BUFFER lvkeyBuff;
    key.CopyIntoBuffer( &lvkeyBuff, sizeof( lvkeyBuff ) );

    if ( sizeof( LVKEY64 ) == key.Cb() )
    {
        //  it can only be a 64-bit chunk key
        LVKEY64* plvkey = (LVKEY64*) &lvkeyBuff;
        *plid = (_LID64) plvkey->lid;
        Assert( plid->FIsLid64() );
    }
    else if ( sizeof( LVKEY32 ) == key.Cb() )
    {
        // It can be a 64-bit LV root key, or a legacy 32-bit chunk key.
        // If the MSB of the first byte is set, it's a 64-bit root key.
        if ( lvkeyBuff.fLid64 )
        {
            LVKEY64* plvkey = (LVKEY64*) &lvkeyBuff;
            *plid = (_LID64) plvkey->lid;
        }
        else
        {
            LVKEY32* plvkey = (LVKEY32*) &lvkeyBuff;
            *plid = (_LID32) plvkey->lid;
            Assert( plid->FIsLid32() );
        }
    }
    else if ( sizeof( _LID32 ) == key.Cb() )
    {
        LVKEY32* plvkey = (LVKEY32*) &lvkeyBuff;
        *plid = (_LID32) plvkey->lid;
        Assert( plid->FIsLid32() );
    }
    else
    {
        *plid = 0;
        AssertTrack( false, "LvNodeKeySizeMismatch" );
    }
}


//  ================================================================
INLINE VOID LidOffsetFromKey( LvId * plid, ULONG * pulOffset, const KEY& key )
//  ================================================================
{
    Assert( plid );
    Assert( pulOffset );

    LVKEY_BUFFER lvkeyBuff;
    key.CopyIntoBuffer( &lvkeyBuff, sizeof( lvkeyBuff ) );

    if ( sizeof( LVKEY64 ) == key.Cb() )
    {
        LVKEY64* plvkey = (LVKEY64*) &lvkeyBuff;
        *plid = (_LID64) plvkey->lid;
        *pulOffset = plvkey->offset;
        Assert( plid->FIsLid64() );
    }
    else if ( sizeof( LVKEY32 ) == key.Cb() )
    {
        LVKEY32* plvkey = (LVKEY32*) &lvkeyBuff;
        *plid = (_LID32) plvkey->lid;
        *pulOffset = plvkey->offset;
        Assert( plid->FIsLid32() );
    }
    else
    {
        *plid = 0;
        *pulOffset = 0;
        AssertTrack( sizeof( _LID64 ) == key.Cb() || sizeof( _LID32 ) == key.Cb(), "LvNodeKeySizeMismatch" );
    }
}


INLINE ULONG CbLVIntrinsicTableMost( const FUCB * pfucb )
{
    Assert( pfucb != pfucbNil );
    return (
        REC::CbRecordMost(pfucb)
        - sizeof(REC::cbRecordHeader)
        - sizeof(TAGFLD)
        - sizeof(TAGFLD_HEADER) );
}

//  use to report corrupted LVs
//
#define LVReportAndTrapCorruptedLV( pfucbLV, lid, wszGuid ) LVReportAndTrapCorruptedLV_( pfucbLV, lid, __FILE__, __LINE__, wszGuid )
VOID LVReportAndTrapCorruptedLV_( const FUCB * const pfucbLV, const LvId lid, const CHAR * const szFile, const LONG lLine, const WCHAR* const wszGuid );
#define LVReportAndTrapCorruptedLV2( pinst, wszDatabaseName, szTable, pgno, lid, wszGuid ) LVReportAndTrapCorruptedLV_( pinst, wszDatabaseName, szTable, pgno, lid, __FILE__, __LINE__, wszGuid )
VOID LVReportAndTrapCorruptedLV_( const INST * const pinst, PCWSTR wszDatabaseName, PCSTR szTable, const PGNO pgno, const LvId lid, const CHAR * const szFile, const LONG lLine, const WCHAR* const wszGuid );

ERR ErrLVPrereadLongValues(
    FUCB * const                        pfucb,
    __inout_ecount( clid ) LvId* const  plid,
    const ULONG                         clid,
    const ULONG                         cPageCacheMin,
    const ULONG                         cPageCacheMax,
    LONG * const                        pclidPrereadActual,
    ULONG * const                       pcPageCacheActual,
    const JET_GRBIT                     grbit );

ERR ErrLVPrereadLongValue(
    _In_ FUCB * const   pfucb,
    _In_ const LvId lid,
    _In_ const ULONG    ulOffset,
    _In_ const ULONG    cbData );

//  use to accumulate up stats for space dump
//
typedef struct
{
    BOOL                    fStarted;

    //  Data on the page we're processing
    PGNO                    pgnoLast;
    PGNO                    pgnoCurrent;

    //  Data on the current LV we're processing
    LvId                    lidCurrent;
    ULONG                   cbCurrent;
    ULONG                   cRefsCurrent;
    BOOL                    fEncrypted;

    PGNO                    pgnoLvStart;
    ULONG                   cbAccumLogical; // uncompressed
    ULONG                   cbAccumActual;  // compressed

    CPG                     cpgAccumMax;
    PGNO*                   rgpgnoAccum;
    CPG                     cpgAccum;

    INT                     cRunsAccessed;
    CPG                     cpgAccessed;

    BTREE_STATS_LV *        pLvData;
} EVAL_LV_PAGE_CTX;

ERR ErrAccumulateLvPageData(
    const PGNO pgno, const ULONG iLevel, const CPAGE * pcpage, void * pvCtx
    );

ERR ErrAccumulateLvNodeData(
    const CPAGE::PGHDR * const  ppghdr,
    INT                         itag,
    DWORD                       fNodeFlags,
    const KEYDATAFLAGS * const  pkdf,
    void *                      pvCtx
    );

INLINE BOOL FPartiallyDeletedLV( ULONG ulRefCount )
{
    return ( ulRefCount == ulMax );
}
