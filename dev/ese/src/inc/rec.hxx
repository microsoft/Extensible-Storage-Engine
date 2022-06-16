// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  If null bit is 1, then column is null.
//  Note that the fid passed in should already be converted to an
//  index.
#define FixedNullBit( ifid )    ( 1 << ( (ifid) % 8 ) )
INLINE BOOL FFixedNullBit( const BYTE *pbitNullity, const UINT ifid )
{
    return ( *pbitNullity & FixedNullBit( ifid ) );
}
INLINE VOID SetFixedNullBit( BYTE *pbitNullity, const UINT ifid )
{
    *pbitNullity |= FixedNullBit( ifid );   // Set to 1 (null)
}
INLINE VOID ResetFixedNullBit( BYTE *pbitNullity, const UINT ifid )
{
    *pbitNullity &= ~FixedNullBit( ifid );  // Set to 0 (non-null)
}

INLINE WORD IbVarOffset( const WORD ibVarOffs )
{
    // Highest bit is null bit, g_cbPage is limited to 32kiB
    return ibVarOffs & ( WORD(-1) >> 1 );
}
INLINE BOOL FVarNullBit( const WORD ibVarOffs )
{
    return ( ibVarOffs & 0x8000 );
}
INLINE VOID SetVarNullBit( WORD& ibVarOffs )
{
    ibVarOffs = WORD( ibVarOffs | 0x8000 ); // Set to 1 (null)
}
INLINE VOID SetVarNullBit( UnalignedLittleEndian< WORD >& ibVarOffs )
{
    ibVarOffs = WORD( ibVarOffs | 0x8000 ); // Set to 1 (null)
}
INLINE VOID ResetVarNullBit( WORD& ibVarOffs )
{
    ibVarOffs = WORD( ibVarOffs & 0x7fff ); // Set to 0 (non-null)
}
INLINE VOID ResetVarNullBit( UnalignedLittleEndian< WORD >& ibVarOffs )
{
    ibVarOffs = WORD( ibVarOffs & 0x7fff ); // Set to 0 (non-null)
}

// Used to flip highest bit of signed fields when transforming.
#define maskByteHighBit         ( BYTE( 1 ) << ( sizeof( BYTE ) * 8 - 1 ) )
#define maskWordHighBit         ( WORD( 1 ) << ( sizeof( WORD ) * 8 - 1 ) )
#define maskDWordHighBit        ( DWORD( 1 ) << ( sizeof( DWORD ) * 8 - 1 ) )
#define maskQWordHighBit        ( QWORD( 1 ) << ( sizeof( QWORD ) * 8 - 1 ) )
#define bFlipHighBit(b)         ((BYTE)((b) ^ maskByteHighBit))
#define wFlipHighBit(w)         ((WORD)((w) ^ maskWordHighBit))
#define dwFlipHighBit(dw)       ((DWORD)((dw) ^ maskDWordHighBit))
#define qwFlipHighBit(qw)       ((QWORD)((qw) ^ maskQWordHighBit))


typedef JET_RETRIEVEMULTIVALUECOUNT     TAGCOLINFO;


#include <pshpack1.h>


//  internal SetColumn() grbits
const JET_GRBIT     grbitSetColumnInternalFlagsMask         = 0xf0000000;
const JET_GRBIT     grbitSetColumnUseDerivedBit             = 0x80000000;
const JET_GRBIT     grbitSetColumnSeparated                 = 0x40000000;
const JET_GRBIT     grbitSetColumnCompressed                = 0x20000000;
const JET_GRBIT     grbitSetColumnEncrypted                 = 0x10000000;

//  internal RetrieveColumn() grbits
const JET_GRBIT     grbitRetrieveColumnInternalFlagsMask    = 0xf0000000;
const JET_GRBIT     grbitRetrieveColumnUseDerivedBit        = 0x80000000;
const JET_GRBIT     grbitRetrieveColumnDDLNotLocked         = 0x40000000;

//  LID Format:
//  Legacy LIDs were 31-bit integers, with MSB reserved for future use.
//  New LIDs are 64-bit integers, with the MSB (bit 63) used to distinguish
//  between a legacy 32-bit or a new 64-bit LID. Since the keys are laid
//  out in big endian format, inspecting the first byte will give us the MSB,
//  and will tell us whether the following bytes belong to a LID64 or a LID32.
//  The high byte is reserved for future use, leaving 56-bits for the actual lid.
//
//  lid64Flag is needed to disambiguate LIDs in two cases:
//  1. Identifying LV roots from a key. A key in an LV tree can be 4, 8 or 12 bytes.
//     A 4-byte key is always a legacy LV root key.
//     An 8-byte key is either a new format LV root (if lid64Flag is set), or a legacy LV chunk key.
//     A 12-byte key is always a new format LV chunk key.
//  2. The engine always works with 64-bit data types. Legacy format is identified at persistence time
//     by the absence of lid64Flag. It simplifies implementation and makes it easy to drop legacy
//     compatibility in the future when it isn't needed.

//  The following table lists all the possible ranges in the LID number space and their use:
//
//  Begin                   End                     Usage
//  -------------------     -------------------     -------------------------
//  0                       0                       lidMin (never used)
//  1                       0x7ffffeff              LID32
//  0x7fffff00              0x7fffffff              LID32 Reserved
//  0x80000000              0xffffffff              Invalid (can't be used, high bit identifies LID64)
//  0x00000001.00000000     0x7fffffff.ffffffff     Invalid (can't be used, high bit identifies LID64)
//  0x80000000.00000000     0x80000000.10000000     LID64 Reserved
//  0x80000000.10000001     0x80fffffe.ffffffff     LID64
//  0x80ffffff.00000000     0x80ffffff.ffffffff     LID64 Reserved
//  0x81000000.00000000     0xffffffff.ffffffff     Reserved (for future use)

typedef ULONG               _LID32;
typedef unsigned __int64    _LID64;

const _LID32 lidMin         = 0;
const _LID32 lid32Max       = 0x7fffffff;
const _LID64 lid64Flag      = 1ULL << 63;                       // = 0x8000000000000000
const _LID64 lid64Reserved  = lid64Flag | ( 0xffULL << 56 );    // = 0xff00000000000000, High byte is reserved for future use (bit-63 is already used)
const _LID64 lid64First     = lid64Flag | ( lid32Max + 2 );     // = 0x8000000010000001
const _LID64 lid64Max       = lid64Flag | ~lid64Reserved;       // = 0x80ffffffffffffff

// At this threshold we warn the user ( via an event and a failure item )
// that they are running out of lids, and return JET_errOutOfLongValueIDs
const _LID32 lid32MaxAllowed    = lid32Max - 256;                   // = 0x7fffff00 (= dwCounterMax for legacy reasons)
const _LID64 lid64MaxAllowed    = lid64Max - UINT32_MAX;            // = 0x80ffffff00000000

class LvId
{
    _LID64 m_lid;

public:
    LvId()                  { m_lid = lidMin; }
    LvId( const LvId& rhs ) { m_lid = rhs.m_lid; }

    LvId( const _LID64 _lid )
    {
        m_lid = _lid;
    }

    static LvId FromULE( const void* const pv, const INT cb )
    {
        if ( cb == sizeof( _LID64 ) )
        {
            return LvId( *( ( UnalignedLittleEndian<_LID64> * ) pv ) );
        }
        else if ( cb == sizeof( _LID32 ) )
        {
            return LvId( *( ( UnalignedLittleEndian<_LID32> * ) pv ) );
        }
        else
        {
            AssertTrack( g_fRepair, "BadLidSizeULE" );
            return LvId();
        }
    }

    static bool FIsLid64( const _LID64 lid )
    {
        bool fLid64 = !!( lid & lid64Flag );
        Assert( fLid64 || lid <= lid32Max || g_fRepair );   // repair can handle corrupted LIDs
        return fLid64;
    }

    static bool FIsLid32( const _LID64 lid )
    {
        return !FIsLid64( lid );
    }

    static bool FValid( const _LID64 lid )
    {
        if ( FIsLid64( lid ) )
        {
            return ( lid > lidMin && lid <= lid64Max );
        }
        else
        {
            return ( lid > lidMin && lid <= lid32Max );
        }
    }

    bool FIsLid64() const       { return FIsLid64( m_lid ); }
    bool FIsLid32() const       { return FIsLid32( m_lid ); }
    bool FValid() const         { return FValid( m_lid ); }
    
    // returned LID can be a LID64 or a LID32
    operator _LID64() const
    {
        bool fLid64 = !!( m_lid & lid64Flag );
        Assert( fLid64 || m_lid <= lid32Max || g_fRepair ); // repair can handle corrupted LIDs
        return m_lid;
    }

    _LID64 ToLid64() const
    {
        Assert( FIsLid64() );
        return m_lid;
    }

    _LID32 ToLid32() const
    {
        Assert( FIsLid32() );
        return (_LID32) m_lid;
    }

    INT Cb() const
    {
        if ( FIsLid64() )
        {
            return sizeof( _LID64 );
        }
        else
        {
            return sizeof( _LID32 );
        }
    }

    INT FLidObeysCurrFormat( const FUCB* const pfucb ) const
    {
        Assert( m_lid > lidMin );
        return ( Cb() == CbLidFromCurrFormat( pfucb ) );
    }

    static INT CbLidFromCurrFormat( const FUCB* const pfucb )
    {
        Assert( pfucb->u.pfcb->Ptdb() != NULL );
        return ( pfucb->u.pfcb->Ptdb()->FLid64() ? sizeof( _LID64 ) : sizeof( _LID32 ) );
    }

    static LvId LidMaxAllowedFromCurrFormat( const FUCB* const pfucb )
    {
        Assert( pfucb->u.pfcb->Ptdb() != NULL );
        return ( pfucb->u.pfcb->Ptdb()->FLid64() ? LvId( lid64MaxAllowed ) : LvId( lid32MaxAllowed ) );
    }
};


/*  long value root data format
/**/
PERSISTED
struct LVROOT
{
    UnalignedLittleEndian< ULONG >      ulReference;
    UnalignedLittleEndian< ULONG >      ulSize;
};

// Introduced with encryption feature, introduced in September, 2015
PERSISTED
typedef BYTE    LVROOTFLAGS;
const   LVROOTFLAGS fLVEncrypted    =   0x01;
INLINE BOOL FLVEncrypted( const LVROOTFLAGS fFlags ) { return ( fFlags & fLVEncrypted ); }

PERSISTED
struct LVROOT2
{
    UnalignedLittleEndian< ULONG >      ulReference;
    UnalignedLittleEndian< ULONG >      ulSize;
    LVROOTFLAGS                         fFlags;
};

//  check if Derived bit should be used in TAGFLD -- already know that this is
//  a template column in a derived table
INLINE BOOL FRECUseDerivedBitForTemplateColumnInDerivedTable(
    const COLUMNID  columnid,
    const TDB       * const ptdb )
{
    Assert( FCOLUMNIDTagged( columnid ) );
    Assert( FCOLUMNIDTemplateColumn( columnid ) );
    ptdb->AssertValidDerivedTable();

    //  HACK: treat derived columns in original-format derived table as
    //  non-derived, because they don't have the fDerived bit set in the TAGFLD
    return ( !ptdb->FESE97DerivedTable()
        || FidOfColumnid( columnid ) >= ptdb->FidTaggedFirst() );
}


//  check if Derived bit should be used in TAGFLD -- don't know yet if this is
//  a template column in a derived table
INLINE BOOL FRECUseDerivedBit( const COLUMNID columnid, const TDB * const ptdb )
{
    Assert( FCOLUMNIDTagged( columnid ) );
    return ( FCOLUMNIDTemplateColumn( columnid )
            && pfcbNil != ptdb->PfcbTemplateTable()
            && FRECUseDerivedBitForTemplateColumnInDerivedTable( columnid, ptdb ) );
}


PERSISTED
class REC
{
    public:
        REC();
        ~REC();

    public:
        typedef WORD RECOFFSET;     // offset relative to start of record
        typedef WORD VAROFFSET;     // offset relative to beginning of variable data

    private:
        PERSISTED
        struct RECHDR
        {
            UnalignedLittleEndian< BYTE >       fidFixedLastInRec;
            UnalignedLittleEndian< BYTE >       fidVarLastInRec;
            UnalignedLittleEndian< RECOFFSET >  ibEndOfFixedData;       // offset relative to start of record
        };

        RECHDR          m_rechdr;
        BYTE            m_rgbFixed[];

        // Fixed data is followed by:
        //    - fixed column null-bit array
        //    - variable offsets table
        //        - start of variable data has offset zero (ie. offsets are relative
        //          to the end of the variable offsets table)
        //        - each entry marks tne end of a variable column
        //        - the last entry coincides with the beginning of tagged data
        //    - variable data
        //    - tagged data


    public:
        static VOID SetMinimumRecord( DATA& data );
        FID         FidFixedLastInRec() const;
        VOID        SetFidFixedLastInRec( const FID fid );
        FID         FidVarLastInRec() const;
        VOID        SetFidVarLastInRec( const FID fid );

        ULONG       CFixedColumns() const;
        ULONG       CVarColumns() const;

        RECOFFSET   IbEndOfFixedData() const;
        VOID        SetIbEndOfFixedData( const RECOFFSET ib );
        BYTE        *PbFixedNullBitMap() const;
        ULONG       CbFixedNullBitMap() const;
        ULONG       CbFixedUserData() const;
        ULONG       CbFixedRecordOverhead() const;
        UnalignedLittleEndian<VAROFFSET>    *PibVarOffsets() const;
        VAROFFSET   IbVarOffsetStart( const FID fidVar ) const;
        VAROFFSET   IbVarOffsetEnd( const FID fidVar ) const;
        BYTE        *PbVarData() const;
        VAROFFSET   IbEndOfVarData() const;
        ULONG       CbVarUserData() const;
        ULONG       CbVarRecordOverhead() const;
        BYTE        *PbTaggedData() const;
        BOOL        FTaggedData( const SIZE_T cbRec ) const;

    public:
        enum { cbRecordHeader = sizeof(RECHDR) };
        enum { cbRecordMin = cbRecordHeader };
                                // 2 + 2 (for offset of end of fixed data) = 4

        //  Compute the largest record that can fit in a page. To be conservative we will assume
        //  that the record is prefix compressed. That means we have the overhead
        //  of cbPrefix and cbSuffix in the record.
        static INLINE LONG CbRecordMost( const LONG cbPage, const LONG cbKeyMost )
        {
            Assert( cbKeyMost >= 0 );
            Assert( cbKeyMost > 0 ); // Is cbKeyMost = 0 actually valid?
            Assert( cbKeyMost <= cbKeyMostMost );

            if ( !!FIsSmallPage() != FIsSmallPage( cbPage ) )
            {
                FireWall( "PageSizeProvidedDidNotMatchGlobPageSize" );
            }

            LONG cbRecordMost;
            if( FIsSmallPage() )
            {
                // for small page sizes we need to preserve backwards compatibility which means not increasing
                // the maximum record size. the old size calculation looked like this:
                //  cbRecordMost = CbNDNodeMost - cbKeyCount - pidb->CbKeyMost()
                // where cbKeyMost was always at least JET_cbKeyMostMin, but could be configured to be larger
                //
                // the problem is that the code above has a bug which shows up when the key size
                // is 254 bytes or greater. we want to fix that bug while keeping the maximum record size the
                // same for all records whose primary key is guaranteed to be less than 254 bytes
                cbRecordMost = CbNDNodeMost( cbPage ) - cbKeyCount - max( JET_cbKeyMostMin, ( cbKeyCount + cbKeyMost ) );
            }
            else
            {
                cbRecordMost = CbNDNodeMost( cbPage ) - cbKeyCount - cbKeyCount /* 2-bytes of prefix key overhead */ - cbKeyMost;
            }
            return cbRecordMost;
        }
        
        //  CbRecordMost without table or index defn should only be used for the purposes of error checking
        //  since it assumes a minimum cbKeyMost of 4.  Convenient when proper context not available.
        //
        static INLINE LONG CbRecordMostCHECK( const LONG cbPage )
        {
            return CbRecordMost( cbPage, sizeof(DBK) );
        }

        //  This is a deprecated global page size using function, needs to be removed complete ... and there are 
        //  ONLY "four" hits for it!  See:
        //      ErrRECIRetrieveFixedColumn() - but that function has 111 references.
        //      ErrRECIRetrieveVarColumn() - another 42 references.
        //      CbSORTCopyFixedVarColumns() - ugh, sorts.
        //      TAGFIELDS::TAGFIELDS() .ctor - Only takes dataRec as in arg.
        //      ErrRECISetIFixedColumn() - Shrink calls with pfucbNil
        //  First two takes a pfcb that we could promote to a ifmp and then to a CbPage(), BUT the pfcb sometimes 
        //  is past in as NULL when it wants to skip the DML latch!  So this is a hard g_cbPage ref to remove. First
        //  three take a TDB ... but I couldn't find a way to make a TDB into an IDB.
        static INLINE LONG CbRecordMostWithGlobalPageSize()
        {
            return CbRecordMost( g_cbPage, sizeof(DBK) );
        }

        static INLINE LONG CbRecordMost( const LONG cbPage, const IDB * const pidb )
        {
            // I(SOMEONE) think this pidbNil decision is a bit dicey, because IIRC during recovery we do not have 
            // such context - and so could mistakenly presume too small a cbKeyMost reserve, leading to too large
            // of CbRecordMost() value.
            const LONG cbKeyMost = ( pidbNil == pidb ) ? sizeof(DBK) : pidb->CbKeyMost();
            return CbRecordMost( cbPage, cbKeyMost );
        }

        static INLINE LONG CbRecordMost( const FCB * const pfcbTable )
        {
            Assert( pfcbTable->Ifmp() != 0 );
            Assert( pfcbTable->FTypeTable() || pfcbTable->FTypeTemporaryTable() || pfcbTable->FTypeSort() );

            // proper way to check IDB & pidb->CbKeyMost populated?
            // Did not hold: Assert( pfcbTable->FInitialized() ); 
            const IDB * const pidbT = pfcbTable->Pidb();

            return CbRecordMost( g_rgfmp[ pfcbTable->Ifmp() ].CbPage(), pidbT );
        }

        static INLINE LONG CbRecordMost( const FUCB * const pfucbTable )
        {
            return CbRecordMost( pfucbTable->u.pfcb );
        }

        static INLINE bool FValidLidRef( const DATA& data )
        {
            if ( data.Cb() == sizeof( _LID64 ) )
            {
                LvId lvid = LvId::FromULE( data.Pv(), data.Cb() );
                return lvid.FIsLid64();
            }
            else if ( data.Cb() == sizeof( _LID32 ) )
            {
                LvId lvid = LvId::FromULE( data.Pv(), data.Cb() );
                return lvid.FIsLid32();
            }
            else
            {
                return false;
            }
        }
};

#include <poppack.h>


INLINE REC::REC()
{
    Assert( sizeof(REC) == cbRecordMin );
    m_rechdr.fidFixedLastInRec = (BYTE)( FID( fidtypFixed, fidlimNone ) );
    m_rechdr.fidVarLastInRec = (BYTE)( FID( fidtypVar, fidlimNone ) );

    //
    // Because these are and have been persisted, they are magic and must not ever change.
    // Assert that they hold their magic, persisted values.
    //
    Assert( 0   == m_rechdr.fidFixedLastInRec );
    Assert( 127 == m_rechdr.fidVarLastInRec );
    
    m_rechdr.ibEndOfFixedData = cbRecordMin;
}

INLINE REC::~REC()
{
}

INLINE VOID REC::SetMinimumRecord( DATA& data )
{
    new( data.Pv() ) REC();
    data.SetCb( cbRecordMin );
}

INLINE FID REC::FidFixedLastInRec() const
{
    const FID   fidFixed = (FID)m_rechdr.fidFixedLastInRec;
    Assert( fidFixed.FFixedNone() || fidFixed.FFixed() );
    return fidFixed;
}
INLINE VOID REC::SetFidFixedLastInRec( const FID fid )
{
    Assert( fid.FFixedNone() || fid.FFixed() );
    m_rechdr.fidFixedLastInRec = (BYTE)fid;
}

INLINE FID REC::FidVarLastInRec() const
{
    const FID   fidVar = (FID)m_rechdr.fidVarLastInRec;
    Assert( fidVar.FVarNone() || fidVar.FVar() );
    return fidVar;
}
INLINE VOID REC::SetFidVarLastInRec( const FID fid )
{
    Assert( fid.FVarNone() || fid.FVar() );
    m_rechdr.fidVarLastInRec = (BYTE)fid;
}

INLINE ULONG REC::CFixedColumns() const
{
    return FidFixedLastInRec().CountOf( fidtypFixed );
}
INLINE ULONG REC::CVarColumns() const
{
    return FidVarLastInRec().CountOf( fidtypVar );
}

INLINE REC::RECOFFSET REC::IbEndOfFixedData() const
{
    return m_rechdr.ibEndOfFixedData;
}
INLINE VOID REC::SetIbEndOfFixedData( const RECOFFSET ib )
{
    Assert( ib < g_cbPage );  // very rough sanity check
    m_rechdr.ibEndOfFixedData = ib;
}

INLINE BYTE *REC::PbFixedNullBitMap() const
{
    Assert( CbFixedNullBitMap() < IbEndOfFixedData() );
    return ( (BYTE *)this ) + IbEndOfFixedData() - CbFixedNullBitMap();
}

INLINE ULONG REC::CbFixedNullBitMap() const
{
    return ( ( CFixedColumns() + 7 ) / 8 );
}

INLINE ULONG REC::CbFixedUserData() const
{
    return IbEndOfFixedData() - cbRecordHeader - CbFixedRecordOverhead();
}

INLINE ULONG REC::CbFixedRecordOverhead() const
{
    return CbFixedNullBitMap();
}

INLINE UnalignedLittleEndian<REC::VAROFFSET> *REC::PibVarOffsets() const
{
    Assert( FidVarLastInRec().FVarNone() ||  FidVarLastInRec().FVar() );
    return (UnalignedLittleEndian<VAROFFSET> *)( ( (BYTE *)this ) + IbEndOfFixedData() );
}

INLINE REC::VAROFFSET REC::IbVarOffsetStart( const FID fidVar ) const
{
    Assert( fidVar.FVar() );
    Assert( fidVar <= (FID)m_rechdr.fidVarLastInRec );

    //  The beginning of the desired column is equivalent to the
    //  the end of the previous column.
    return ( fidVar.FVarLeast() ?
                (REC::VAROFFSET)0 :
                IbVarOffset( PibVarOffsets()[ fidVar.IndexOf( fidtypVar ) - 1 ] ) );
}

INLINE REC::VAROFFSET REC::IbVarOffsetEnd( const FID fidVar ) const
{
    Assert( fidVar.FVar() );
    Assert( fidVar <= (FID)m_rechdr.fidVarLastInRec );
    return ( IbVarOffset( PibVarOffsets()[ fidVar.IndexOf( fidtypVar ) ] ) );
}

INLINE BYTE *REC::PbVarData() const
{
    const RECOFFSET     ibVarData           = RECOFFSET( IbEndOfFixedData()
                                                + ( CVarColumns() * sizeof(VAROFFSET) ) );
    Assert( ibVarData >= sizeof(RECHDR) );
    return ( (BYTE *)this ) + ibVarData;
}

INLINE REC::VAROFFSET REC::IbEndOfVarData() const
{
    if ( FidVarLastInRec().FVarNone() )
        return 0;       // no variable data

    return IbVarOffset( PibVarOffsets()[ FidVarLastInRec().IndexOf( fidtypVar ) ] );
}

INLINE ULONG REC::CbVarUserData() const
{
    return IbEndOfVarData();
}

INLINE ULONG REC::CbVarRecordOverhead() const
{
    return ( CVarColumns() * sizeof(VAROFFSET) );
}

INLINE BYTE *REC::PbTaggedData() const
{
    return PbVarData() + IbEndOfVarData();
}

INLINE BOOL REC::FTaggedData( const SIZE_T cbRec ) const
{
    const BYTE * const  pbTaggedData    = PbTaggedData();

    Assert( pbTaggedData > (BYTE *)this );
    Assert( SIZE_T( pbTaggedData - (BYTE *)this ) <= cbRec );

    return ( SIZE_T( pbTaggedData - (BYTE *)this ) < cbRec );
}


const ULONG cbLVIntrinsicMost       = 1023;

INLINE ULONG CbPreferredIntrinsicLVMost( const FCB * pfcbTable )
{
    Assert( pfcbTable );
    Assert( pfcbTable->Ptdb() != ptdbNil );

    ULONG cbUserPreferred = pfcbTable->Ptdb()->CbPreferredIntrinsicLV();

    //  0 means use the default ... 
    if ( cbUserPreferred == 0 )
    {
        //  Use the default heuristic
        return cbLVIntrinsicMost;
    }
    return cbUserPreferred;
}


const ULONG cbDefaultValueMost  = max( JET_cbColumnMost, JET_cbLVDefaultValueMost );

const ULONG rgitagBaseKey[JET_ccolKeyMost] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
const INT   iidxsegNoRollover = JET_ccolKeyMost + 1;
const INT   iidxsegComplete = 0;

class KeySequence
{
private:
    BOOL            m_fMultivalued;
    BOOL            m_fNestedTable;
    INT             m_iidxsegMac;
    INT             m_iidxsegMultiValuedMost;
    ULONG           m_rgitag[JET_ccolKeyMost];
    JET_COLUMNID    m_rgcolumnid[JET_ccolKeyMost];
    INT             m_rgfMultiValued[JET_ccolKeyMost];
    INT             m_fKeySequenceComplete;

    void Init( const FUCB * const pfucb, const IDB * const pidb )
    {
        FCB     *pfcb       = pfucb->u.pfcb;
        IDXSEG  rgidxseg[JET_ccolKeyMost];
        INT     iidxsegT;
        FIELD   field;

        m_fMultivalued = pidb->FMultivalued();
        m_fNestedTable = pidb->FNestedTable();
        m_iidxsegMac = pidb->Cidxseg();
        m_iidxsegMultiValuedMost = -1;
        memcpy( m_rgitag, rgitagBaseKey, sizeof(m_rgitag) );

        //  if index has one or more multi-valued columns,
        //  then set multi-valuedness of those column(s).
        //
        if ( m_fMultivalued )
        {
            //  retrieve each segment in key description
            //
            if ( pidb->FTemplateIndex() )
            {
                Assert( pfcb->FDerivedTable() || pfcb->FTemplateTable() );
                if ( pfcb->FDerivedTable() )
                {
                    //  switch to template table
                    //
                    pfcb = pfcb->Ptdb()->PfcbTemplateTable();
                    Assert( pfcbNil != pfcb );
                    Assert( pfcb->FTemplateTable() );
                }
                else
                {
                    Assert( pfcb->Ptdb()->PfcbTemplateTable() == pfcbNil );
                }
            }

            //  get columnids from index definition
            //
            pfcb->EnterDML();
            UtilMemCpy( rgidxseg, PidxsegIDBGetIdxSeg( pidb, pfcb->Ptdb() ), m_iidxsegMac * sizeof(IDXSEG) );
            pfcb->LeaveDML();

            //  set columnid properties for all columns,
            //  and reset multi-valuedness for all columns
            //
            for ( iidxsegT = 0; iidxsegT < m_iidxsegMac; iidxsegT++ )
            {
                m_rgcolumnid[iidxsegT] = rgidxseg[iidxsegT].Columnid();
                Assert( 1 == m_rgitag[iidxsegT] );
                m_rgfMultiValued[iidxsegT] = fFalse;
            }
#ifdef DEBUG
            for ( ; iidxsegT < JET_ccolKeyMost; iidxsegT++ )
            {
                m_rgcolumnid[iidxsegT] = 0;
                Assert( 1 == m_rgitag[iidxsegT] );
                m_rgfMultiValued[iidxsegT] = fFalse;
            }
#endif

            //  set multi-valuedness for index columns
            //
            for ( iidxsegT = 0; iidxsegT < m_iidxsegMac; iidxsegT++ )
            {
                const COLUMNID columnid = rgidxseg[iidxsegT].Columnid();

                Assert( columnid == m_rgcolumnid[iidxsegT] );
                Assert( fFalse == m_rgfMultiValued[iidxsegT] );

                if ( FCOLUMNIDTagged( columnid ) )
                {
                    // No need to check column access -- since the column belongs
                    // to an index, it MUST be available.  It can't be deleted, or
                    // even versioned deleted.
                    pfcb->EnterDML();
                    field = *( pfcb->Ptdb()->Pfield( columnid ) );
                    pfcb->LeaveDML();

                    if ( FFIELDMultivalued( field.ffield ) )
                    {
                        m_rgfMultiValued[iidxsegT] = fTrue;
                        m_iidxsegMultiValuedMost = iidxsegT;

                        //  if cross product index or nested table index, 
                        //  then recognize multi-valuedness of all multi-valued index columns.
                        //  Otherwise, recognize only the first.
                        //
                        if ( !pidb->FCrossProduct() && !pidb->FNestedTable() )
                        {
                            break;
                        }
                    }
                }
            }
        }

        Assert( m_iidxsegMultiValuedMost < pidb->Cidxseg() );
        Assert( m_iidxsegMultiValuedMost == -1 ||
                m_iidxsegMultiValuedMost >= 0 && m_iidxsegMultiValuedMost < JET_ccolKeyMost );
        Reset();
    }


public:
    void Reset( void )
    {
        INT iidxsegT;
        for ( iidxsegT = 0; iidxsegT <= m_iidxsegMultiValuedMost && iidxsegT < JET_ccolKeyMost; iidxsegT++ )
        {
            m_rgitag[iidxsegT] = 1;
        }
#ifdef DEBUG
        for ( ; iidxsegT < JET_ccolKeyMost; iidxsegT++ )
        {
            Assert( 1 == m_rgitag[iidxsegT] );
        }
#endif
        m_fKeySequenceComplete = fFalse;
    }

    KeySequence( const FUCB * const pfucb, const IDB * const pidb )
    {
        Init( pfucb, pidb );
        Reset();
    }

    KeySequence( const FUCB * const pfucb, const IDB * const pidb, ULONG itag )
    {
        INT         iidxsegT;

        Init( pfucb, pidb );
        Reset();

        //  set first multi-valued column itag sequence to given itag
        //
        if ( itag > 1 && m_iidxsegMultiValuedMost >= 0 )
        {
            Assert( m_iidxsegMultiValuedMost >= 0 );
            for( iidxsegT = 0; iidxsegT <= m_iidxsegMultiValuedMost && iidxsegT < JET_ccolKeyMost; iidxsegT++ )
            {
                if ( m_rgfMultiValued[ iidxsegT ] )
                {
                    m_rgitag[ iidxsegT ] = itag;
                    break;
                }
            }
            Assert( iidxsegT <= m_iidxsegMultiValuedMost );
            Assert( iidxsegT >= 0 && iidxsegT < JET_ccolKeyMost );
        }
    }

    ~KeySequence( void )
    {
        ;
    }


    //  returns fTrue if m_rgitag selects base key
    //
    BOOL FBaseKey( void )
    {
        return ( 0 == memcmp( m_rgitag, rgitagBaseKey, sizeof(m_rgitag) ) );
    }

    BOOL FSequenceComplete( void )
    {
        return m_fKeySequenceComplete;
    }

    void Increment( INT iidxsegRollover )
    {
        INT iidxsegT;

        //  if no specific column returned NULL with itagSequence > 1 then caller will pass -1
        //  In this case, increment least significant multi-valued column.
        //
        //  else rollover case: a multi-valued column returned NULL with an itagSequence > 1.
        //  Set that columns itagSequence back to 1 and increment next most significant multi-valued column.
        //
        if ( iidxsegNoRollover == iidxsegRollover )
        {
            //  nested table indexes roll all multi-value column itagSequences together
            //  while cross product indexes enumerate every combination.
            //
            if ( m_fNestedTable )
            {
                for ( iidxsegT = m_iidxsegMultiValuedMost; iidxsegT >= 0 ; iidxsegT-- )
                {
                    if ( m_rgfMultiValued[iidxsegT] )
                    {
                        m_rgitag[iidxsegT]++;
                    }
                }
            }
            else
            {
                if ( m_iidxsegMultiValuedMost >= 0 && m_iidxsegMultiValuedMost < JET_ccolKeyMost )
                {
                    m_rgitag[m_iidxsegMultiValuedMost]++;
                }
            }
        }
        else
        {
            //  should never increment beyond complete
            //
            Assert( !m_fKeySequenceComplete );

            if ( iidxsegComplete == iidxsegRollover )
            {
                m_fKeySequenceComplete = fTrue;
                return;
            }

            //  find next most signifcant multivalued column and increment it
            //
            Assert( iidxsegRollover > 0 );
            Assert( iidxsegRollover < JET_ccolKeyMost );
            iidxsegRollover = iidxsegRollover < JET_ccolKeyMost ? iidxsegRollover : JET_ccolKeyMost - 1;
            
            for ( iidxsegT = iidxsegRollover - 1; iidxsegT >= 0; iidxsegT-- )
            {
                if ( m_rgfMultiValued[iidxsegT] )
                {
                    m_rgitag[iidxsegT]++;
                    break;
                }
            }

            if ( iidxsegT < 0 )
            {
                m_fKeySequenceComplete = fTrue;
                return;
            }

            //  set rollover column, and all less signifant columns back to 1
            //
            for ( iidxsegT = iidxsegRollover; iidxsegT <= m_iidxsegMultiValuedMost && iidxsegT < JET_ccolKeyMost; iidxsegT++ )
            {
                m_rgitag[iidxsegT] = 1;
            }
#ifdef DEBUG
            for ( ; iidxsegT < JET_ccolKeyMost; iidxsegT++ )
            {
                Assert( m_rgitag[iidxsegT] == 1 );
            }
#endif
        }

        Assert( !m_fKeySequenceComplete );
        return;
    }

    ULONG *Rgitag( void )
    {
        return m_rgitag;
    }

    ULONG ItagFromColumnid( JET_COLUMNID columnidT )
    {
        INT     iidxsegT;

        //  if index not multivalued, itag must be 1
        //
        Assert( m_fMultivalued );
        Assert( m_iidxsegMac <= JET_ccolKeyMost );

        //  look for columnid in index
        //
        for ( iidxsegT = 0; iidxsegT < m_iidxsegMac; iidxsegT++ )
        {
            if ( columnidT == m_rgcolumnid[iidxsegT] )
            {
                break;
            }
        }

        AssertPREFIX( iidxsegT < m_iidxsegMac );
        AssertPREFIX( iidxsegT < _countof( m_rgcolumnid ) );
        Assert( m_rgcolumnid[iidxsegT] > 0 );
        Assert( m_rgitag[iidxsegT] >= 1 );

        return m_rgitag[iidxsegT];
    }
}; // class KeySequence

INLINE LvId LidOfSeparatedLV( const BYTE * const pbData, const INT cbData )
{
    return LvId::FromULE( pbData, cbData );
}
INLINE LvId LidOfSeparatedLV( const DATA& data )
{
    return LidOfSeparatedLV( ( BYTE* )data.Pv(), data.Cb() );
}

INLINE INT CbLVSetLidInRecord( BYTE * const pbData, const INT cbData, const LvId lid )
{
    if ( lid.FIsLid64() )
    {
        if ( cbData >= sizeof( _LID64 ) )
        {
            UnalignedLittleEndian< _LID64 > lidT = (_LID64) lid;
            UtilMemCpy( pbData, &lidT, sizeof( _LID64 ) );
            return sizeof( _LID64 );
        }
        else
        {
            FireWall( "NotEnoughSpaceForLid64" );
            memset( pbData, 0, cbData );    // set to invalid lid
            return cbData;
        }
    }
    else
    {
        Assert( lid.FIsLid32() );
        if ( cbData >= sizeof( _LID32 ) )
        {
            UnalignedLittleEndian< _LID32 > lidT = (_LID32) lid;
            UtilMemCpy( pbData, &lidT, sizeof( _LID32 ) );
            return sizeof( _LID32 );
        }
        else
        {
            FireWall( "NotEnoughSpaceForLid32" );
            memset( pbData, 0, cbData ); // set to invalid lid
            return cbData;
        }
    }
}

const ULONG fLVReference    = 0;
const ULONG fLVDereference  = 1;

const SHORT cbFLDBinaryChunk            = 8;                    // Break up binary data into chunks of this many bytes.
const SHORT cbFLDBinaryChunkNormalized  = cbFLDBinaryChunk+1;   // Length of one segment of normalised binary data.

ULONG UlChecksum( VOID *pv, ULONG cb );
ERR ErrRECSetCurrentIndex( FUCB *pfucb, const CHAR *szIndex, const INDEXID *pindexid );
ERR ErrRECIIllegalNulls( FUCB * const pfucb );

ERR     ErrLVInit   ( INST *pinst );
VOID    LVTerm      ( INST * pinst );

ERR ErrFILEOpenLVRoot( FUCB *pfucb, FUCB **ppfucbLV, BOOL fCreate );


#ifdef DEBUG
VOID LVCheckOneNodeWhileWalking(
    FUCB        *pfucbLV,
    LvId        *plidCurr,
    LvId        *plidPrev,
    ULONG       *pulSizeCurr,
    ULONG       *pulSizeSeen );
#endif  //  DEBUG

ERR ErrRECISetFixedColumn(
    FUCB            * const pfucb,
    TDB             * const ptdb,
    const COLUMNID  columnid,
    DATA            *pdataField );
ERR ErrRECISetFixedColumnInLoadedDataBuffer(
    DATA            * const pdataWorkBuf,
    TDB             * const ptdb,
    const COLUMNID  columnid,
    DATA            *pdataField );
ERR ErrRECISetVarColumn(
    FUCB            * const pfucb,
    TDB             * const ptdb,
    const COLUMNID  columnid,
    DATA            *pdataField );
ERR ErrRECISetTaggedColumn(
    FUCB            * const pfucb,
    TDB             * const ptdb,
    const COLUMNID  columnid,
    const ULONG     itagSequence,
    const DATA      * const pdataToSet,
    const BOOL      fUseDerivedBit,
    const JET_GRBIT grbit );
ERR ErrRECIFColumnSet(
    FUCB            * const pfucb,
    TDB             * const ptdb,
    const COLUMNID  columnid,
    const ULONG     itagSequence,
    RCE             *prcePrimary,
    const BOOL      fCheckForNullOnly,
    BOOL            *pfColumnSet );

INLINE ERR ErrRECSetColumn(
    FUCB            * const pfucb,
    COLUMNID        columnid,
    const ULONG     itagSequence,
    DATA            *pdataField,
    const JET_GRBIT grbit = NO_GRBIT )
{
    ERR             err;
    FCB             *pfcb           = pfucb->u.pfcb;

    Assert( FCOLUMNIDValid( columnid ) );

    Assert( pfcb != pfcbNil );
    if ( FCOLUMNIDTemplateColumn( columnid ) )
    {
        if ( !pfcb->FTemplateTable() )
        {
            // switch to template table
            pfcb->Ptdb()->AssertValidDerivedTable();
            pfcb = pfcb->Ptdb()->PfcbTemplateTable();
        }
        else
        {
            pfcb->Ptdb()->AssertValidTemplateTable();
        }
    }
    else
    {
        Assert( !pfcb->Ptdb()->FTemplateTable() );
    }
    Assert( ptdbNil != pfcb->Ptdb() );

    pfcb->EnterDML();
    if ( FCOLUMNIDTagged( columnid ) )
    {
        err = ErrRECISetTaggedColumn(
                    pfucb,
                    pfcb->Ptdb(),
                    columnid,
                    itagSequence,
                    pdataField,
                    FRECUseDerivedBit( columnid, pfucb->u.pfcb->Ptdb() ),
                    grbit );
    }
    else if ( FCOLUMNIDFixed( columnid ) )
    {
        err = ErrRECISetFixedColumn( pfucb, pfcb->Ptdb(), columnid, pdataField );
    }
    else
    {
        Assert( FCOLUMNIDVar( columnid ) );
        err = ErrRECISetVarColumn( pfucb, pfcb->Ptdb(), columnid, pdataField );
    }
    pfcb->LeaveDML();
    return err;
}

ERR ErrRECSetLongField(
    FUCB                *pfucb,
    const COLUMNID      columnid,
    const ULONG         itagSequence,
    DATA                *plineField,
    const CompressFlags compressFlags,
    const BOOL          fEncrypted = fFalse,
    JET_GRBIT           grbit = NO_GRBIT,
    const ULONG         ibLongValue = 0,
    const ULONG         ulMax = 0,
    ULONG               cbThreshold = 0 );  // cbThreshold = 0 will use the sizeof LID as threshold based on current format

ERR ErrRECRetrieveSLongField(
    FUCB            *pfucb,
    LvId            lid,
    BOOL            fAfterImage,
    ULONG           ibGraphic,
    const BOOL      fEncrypted,
    BYTE            *pb,
    ULONG           cbMax,
    ULONG           *pcbActual,
    JET_PFNREALLOC  pfnRealloc          = NULL,
    void*           pvReallocContext    = NULL,
    const RCE       * const prceBase    = prceNil
    );

ERR ErrRECRetrieveSLongFieldPrereadOnly(    FUCB *pfucb, LvId lid, const ULONG ulOffset, const ULONG cbMax, const BOOL fLazy, const JET_GRBIT grbit );


ERR ErrRECRetrieveSLongFieldRefCount(
    FUCB        *pfucb,
    LvId            lid,
    BYTE        *pb,
    ULONG       cbMax,
    ULONG       *pcbActual
    );

ERR ErrRECGetLVSize(
    FUCB * const    pfucb,
    const LvId      lid,
    const BOOL      fLogicalOnly,
    QWORD * const   pcbLVDataLogical,   // logical (i.e. uncompressed) size of the data
    QWORD * const   pcbLVDataPhysical,  // physical (i.e. compressed) size of the data
    QWORD * const   pcbLVOverhead );

ERR ErrRECDoesSLongFieldExist( FUCB * const pfucb, const LvId lid, BOOL* const pfExists );

ERR ErrRECFSetLV(
    FUCB        *pfucb,
    LvId        lid,
    BOOL        *pfSetLV );

INLINE VOID SetPbDataField(
    BYTE                ** ppbDataField,
    const DATA          * const pdataField,
    BYTE                * pbConverted,
    const JET_COLTYP    coltyp )
{
    Assert( pdataField );

    *ppbDataField = (BYTE*)pdataField->Pv();

    if ( pdataField->Cb() <= 8 && !FHostIsLittleEndian() )
    {
        switch ( coltyp )
        {
            case JET_coltypShort:
            case JET_coltypUnsignedShort:
                *(USHORT*)pbConverted = ReverseBytesOnBE( *(USHORT*)pdataField->Pv() );
                *ppbDataField = pbConverted;
                break;

            case JET_coltypLong:
            case JET_coltypUnsignedLong:
            case JET_coltypIEEESingle:
                *(ULONG*)pbConverted = ReverseBytesOnBE( *(ULONG*)pdataField->Pv() );
                *ppbDataField = pbConverted;
                break;

            case JET_coltypLongLong:
            case JET_coltypUnsignedLongLong:
            case JET_coltypCurrency:
            case JET_coltypIEEEDouble:
            case JET_coltypDateTime:
                *(QWORD*)pbConverted = ReverseBytesOnBE( *(QWORD*)pdataField->Pv() );
                 *ppbDataField = pbConverted;
                break;
        }
    }
}

INLINE VOID RECCopyData(
    BYTE                * const pbDest,
    const DATA          * const pdataSrc,
    const JET_COLTYP    coltyp )
{
    BYTE                rgb[8];
    BYTE                * pbSrc;

    Assert( JET_coltypNil != coltyp );
    SetPbDataField( &pbSrc, pdataSrc, rgb, coltyp );
    UtilMemCpy( pbDest, pbSrc, pdataSrc->Cb() );
}

ERR ErrRECICheckUniqueNormalizedTaggedRecordData(
    _In_ const FIELD *          pfield,
    _In_ const DATA&            dataFieldNorm,
    _In_ const DATA&            dataRecRaw,
    _In_ const NORM_LOCALE_VER* pnlv,
    _In_ const BOOL             fNormDataFieldTruncated );

INLINE ERR ErrRECICheckUnique(
    _In_ const FIELD * const    pfield,
    _In_ const DATA&            dataToSet,
    _In_ const DATA&            dataRec,
    _In_ const NORM_LOCALE_VER* pnlv,
    _In_ const BOOL             fNormalizedDataToSetIsTruncated )
{
    const BOOL                  fNormalize          = ( pfieldNil != pfield );
    Assert( !fNormalizedDataToSetIsTruncated || fNormalize );

    if ( fNormalize )
    {
        return ErrRECICheckUniqueNormalizedTaggedRecordData(
                    pfield,
                    dataToSet,
                    dataRec,
                    pnlv,
                    fNormalizedDataToSetIsTruncated );
    }
    else
    {
        return ( FDataEqual( dataToSet, dataRec ) ? ErrERRCheck( JET_errMultiValuedDuplicate ) : JET_errSuccess );
    }
}

enum LVOP
{
    lvopNull,
    lvopInsert,
    lvopReplace,
    lvopAppend,
    lvopInsertNull,
    lvopInsertZeroLength,
    lvopReplaceWithNull,
    lvopReplaceWithZeroLength,
    lvopInsertZeroedOut,
    lvopResize,
    lvopOverwriteRange,
    lvopOverwriteRangeAndResize
};

ERR ErrRECSeparateLV(
    _In_ FUCB               * const pfucb,
    _In_ const DATA         * const pdataField,
    _In_ const CompressFlags  compressFlags,
    _In_ const BOOL         fEncrypted,
    _Out_ LvId              * const plid,
    __in_opt FUCB           **ppfucb,
    __in_opt LVROOT2        *plvrootInit = NULL );

ERR ErrRECAffectSeparateLV( FUCB *pfucb, LvId *plid, ULONG fLVAffect );

ERR ErrRECAOIntrinsicLV(
    FUCB                *pfucb,
    const COLUMNID      columnid,
    const ULONG         itagSequence,
    const DATA          *pdataColumn,
    const DATA          *pdataNew,
    const ULONG         ibLongValue,
    const ULONG         ulMax,
    const LVOP          lvop,
    const CompressFlags compressFlags,
    const BOOL          fEncrypted );

ERR ErrRECDeleteLV_LegacyVersioned( FUCB *pfucbLV, const ULONG ulLVDeleteOffset, const DIRFLAG dirflag );
ERR ErrRECDeleteLV_SynchronousCleanup( FUCB *pfucbLV, const ULONG cChunksToDelete );
ERR ErrRECUpdateLVRefcount(
    FUCB        *pfucb,
    const LvId  lid,
    const ULONG ulRefcountOld,
    const ULONG ulRefcountNew );

enum LVAFFECT
{
    lvaffectSeparateAll,
    lvaffectReferenceAll,
};

// cbThreshold = 0 will use the size of LID as threshold based on current format
ERR ErrRECAffectLongFieldsInWorkBuf( FUCB *pfucb, LVAFFECT lvaffect, ULONG cbThreshold = 0);

ERR ErrRECDereferenceLongFieldsInRecord( FUCB *pfucb );

INLINE BOOL FRECLongValue( JET_COLTYP coltyp )
{
    return ( coltyp == JET_coltypLongText || coltyp == JET_coltypLongBinary );
}
INLINE BOOL FRECTextColumn( JET_COLTYP coltyp )
{
    return ( coltyp == JET_coltypText || coltyp == JET_coltypLongText );
}
INLINE BOOL FRECBinaryColumn( JET_COLTYP coltyp )
{
    return ( coltyp == JET_coltypBinary || coltyp == JET_coltypLongBinary );
}

//  field extraction
//
ERR ErrRECIGetRecord(
    FUCB    *pfucb,
    DATA    **ppdataRec,
    BOOL    fUseCopyBuffer );
ERR ErrRECIAccessColumn(
    FUCB            * pfucb,
    COLUMNID        columnid,
    FIELD           * pfieldFixed = pfieldNil,
    BOOL            * pfEncrypted = NULL );
ERR ErrRECIRetrieveFixedColumn(
    FCB             * const pfcb,
    const TDB       * ptdb,
    const COLUMNID  columnid,
    const DATA&     dataRec,
    DATA            * pdataField,
    const FIELD     * const pfieldFixed = pfieldNil );
ERR ErrRECIRetrieveVarColumn(
    FCB             * const pfcb,
    const TDB       * ptdb,
    const COLUMNID  columnid,
    const DATA&     dataRec,
    DATA            * pdataField );

ERR ErrRECIRetrieveTaggedColumn(
    FCB             * pfcb,
    const COLUMNID  columnid,
    const ULONG     itagSequence,
    const DATA&     dataRec,
    DATA            * const pdataRetrieveBuffer,
    const ULONG     grbit = NO_GRBIT );

ULONG UlRECICountTaggedColumnInstances(
    FCB             * pfcb,
    const COLUMNID  columnid,
    const DATA&     dataRec );

ERR ErrRECAdjustEscrowedColumn(
    FUCB *          pfucb,
    const COLUMNID  columnid,
    const FIELD&    fieldFixed,
    VOID *          pv,
    const ULONG     cb );

INLINE ERR ErrRECRetrieveNonTaggedColumn(
    FCB             *pfcb,
    const COLUMNID  columnid,
    const DATA&     dataRec,
    DATA            *pdataField,
    const FIELD     * const pfieldFixed )
{
    ERR             err;

    Assert( 0 != columnid );

    Assert( pfcbNil != pfcb );

    if ( FCOLUMNIDTemplateColumn( columnid ) )
    {
        if ( !pfcb->FTemplateTable() )
        {
            // switch to template table
            pfcb->Ptdb()->AssertValidDerivedTable();
            pfcb = pfcb->Ptdb()->PfcbTemplateTable();
        }
        else
        {
            pfcb->Ptdb()->AssertValidTemplateTable();
        }
    }
    else
    {
        Assert( !pfcb->Ptdb()->FTemplateTable() );
    }
    Assert( ptdbNil != pfcb->Ptdb() );

    if ( FCOLUMNIDFixed( columnid ) )
    {
        err = ErrRECIRetrieveFixedColumn(
                    pfcb,
                    pfcb->Ptdb(),
                    columnid,
                    dataRec,
                    pdataField,
                    pfieldFixed );
    }

    else
    {
        Assert( FCOLUMNIDVar( columnid ) );

        err = ErrRECIRetrieveVarColumn(
                    pfcb,
                    pfcb->Ptdb(),
                    columnid,
                    dataRec,
                    pdataField );
    }

    return err;
}


INLINE ERR ErrRECRetrieveTaggedColumn(
    FCB             *pfcb,
    const COLUMNID  columnid,
    const ULONG     itagSequence,
    const DATA&     dataRec,
    DATA            *pdataField,
    const JET_GRBIT grbit = NO_GRBIT )
{
    ERR             err;

    Assert( FCOLUMNIDTagged( columnid ) );
    Assert( !( grbit & grbitRetrieveColumnInternalFlagsMask ) );
    Assert( pfcbNil != pfcb );
    Assert( ptdbNil != pfcb->Ptdb() );

    if ( 0 != itagSequence )
    {
        err = ErrRECIRetrieveTaggedColumn(
                pfcb,
                columnid,
                itagSequence,
                dataRec,
                pdataField,
                grbit | grbitRetrieveColumnDDLNotLocked );
    }
    else
    {
        err = ErrERRCheck( JET_errBadItagSequence );
    }

    return err;
}


COLUMNID ColumnidRECFirstTaggedForScanOfDerivedTable( const TDB * const ptdb );
COLUMNID ColumnidRECNextTaggedForScanOfDerivedTable( const TDB * const ptdb, const COLUMNID columnid );
INLINE COLUMNID ColumnidRECFirstTaggedForScan( const TDB * const ptdb )
{
    if ( pfcbNil != ptdb->PfcbTemplateTable() )
    {
        return ColumnidRECFirstTaggedForScanOfDerivedTable( ptdb );
    }
    else
    {
        return ColumnidOfFid( ptdb->FidTaggedFirst(), ptdb->FTemplateTable() );
    }
}
INLINE COLUMNID ColumnidRECNextTaggedForScan(
    const TDB       * const ptdb,
    const COLUMNID  columnidCurr )
{
    if ( FCOLUMNIDTemplateColumn( columnidCurr ) && !ptdb->FTemplateTable() )
    {
        return ColumnidRECNextTaggedForScanOfDerivedTable( ptdb, columnidCurr );
    }
    else
        return ( columnidCurr + 1 );
}


// Possible return codes are:
//      JET_errSuccess if column not null
//      JET_wrnColumnNull if column is null
//      JET_errColumnNotFound if column not in record
INLINE ERR ErrRECIFixedColumnInRecord(
    const COLUMNID  columnid,
    FCB             * const pfcb,
    const DATA&     dataRec )
{
    ERR             err;

    Assert( FCOLUMNIDFixed( columnid ) );
    Assert( !dataRec.FNull() );
    Assert( dataRec.Cb() >= REC::cbRecordMin );
    Assert( dataRec.Cb() <= REC::CbRecordMost( pfcb ) );

    const REC   *prec = (REC *)dataRec.Pv();

    Assert( prec->FidFixedLastInRec().FFixedNone() || prec->FidFixedLastInRec().FFixed() );

    // RECIAccessColumn() should have already been called to verify FID.
#ifdef DEBUG
    Assert( pfcbNil != pfcb );
    pfcb->EnterDML();

    const TDB   *ptdb = pfcb->Ptdb();
    Assert( ptdbNil != ptdb );
    Assert( FidOfColumnid( columnid ) <= ptdb->FidFixedLast() );

    const FIELD *pfield = ptdb->PfieldFixed( columnid );
    Assert( pfieldNil != pfield );
    Assert( JET_coltypNil != pfield->coltyp );
    Assert( pfield->ibRecordOffset >= ibRECStartFixedColumns );
    Assert( pfield->ibRecordOffset < ptdb->IbEndFixedColumns() );
    Assert( pfield->ibRecordOffset < prec->IbEndOfFixedData()
        || FidOfColumnid( columnid ) > prec->FidFixedLastInRec() );

    pfcb->LeaveDML();
#endif

    if ( FidOfColumnid( columnid ) > prec->FidFixedLastInRec() )
    {
        err = ErrERRCheck( JET_errColumnNotFound );
    }
    else
    {
        Assert( prec->FidFixedLastInRec().FFixed() );

        const UINT  ifid            = FidOfColumnid( columnid ).IndexOf( fidtypFixed );
        const BYTE  * prgbitNullity = prec->PbFixedNullBitMap() + ifid/8;

        if ( FFixedNullBit( prgbitNullity, ifid ) )
            err = ErrERRCheck( JET_wrnColumnNull );
        else
            err = JET_errSuccess;
    }

    return err;
}

// Possible return codes are:
//      JET_errSuccess if column not null
//      JET_wrnColumnNull if column is null
//      JET_errColumnNotFound if column not in record
INLINE ERR ErrRECIVarColumnInRecord(
    const COLUMNID  columnid,
    FCB             * const pfcb,
    const DATA&     dataRec )
{
    ERR             err;

    Assert( FCOLUMNIDVar( columnid ) );
    Assert( !dataRec.FNull() );
    Assert( dataRec.Cb() >= REC::cbRecordMin );
    Assert( dataRec.Cb() <= REC::CbRecordMost( pfcb ) );

    const REC   *prec = (REC *)dataRec.Pv();

    Assert( prec->FidVarLastInRec().FVarNone() || prec->FidVarLastInRec().FVar() );

    // RECIAccessColumn() should have already been called to verify FID.
#ifdef DEBUG
    Assert( pfcbNil != pfcb );
    pfcb->EnterDML();

    const TDB   *ptdb = pfcb->Ptdb();
    Assert( ptdbNil != ptdb );
    Assert( FidOfColumnid( columnid ) <= ptdb->FidVarLast() );

    const FIELD *pfield = ptdb->PfieldVar( columnid );
    Assert( pfieldNil != pfield );
    Assert( JET_coltypNil != pfield->coltyp );

    pfcb->LeaveDML();
#endif

    if ( FidOfColumnid( columnid ) > prec->FidVarLastInRec() )
    {
        err = ErrERRCheck( JET_errColumnNotFound );
    }
    else
    {

        UnalignedLittleEndian<REC::VAROFFSET>   *pibVarOffs = prec->PibVarOffsets();

        Assert( prec->FidVarLastInRec().FVar() );
        Assert( prec->PbVarData() + IbVarOffset( pibVarOffs[prec->FidVarLastInRec().IndexOf( fidtypVar ) ] )
                <= (BYTE *)dataRec.Pv() + dataRec.Cb() );

        //  adjust fid to an index
        //
        const UINT  ifid    = FidOfColumnid( columnid ).IndexOf( fidtypVar );

        if ( FVarNullBit( pibVarOffs[ifid] ) )
        {
#ifdef DEBUG
            //  beginning of current column is end of previous column
            const WORD  ibVarOffset = ( FidOfColumnid( columnid ).FVarLeast() ?
                                            WORD( 0 ) :
                                            IbVarOffset( pibVarOffs[ifid-1] ) );
            Assert( IbVarOffset( pibVarOffs[ifid] ) - ibVarOffset == 0 );
#endif
            err = ErrERRCheck( JET_wrnColumnNull );
        }
        else
        {
            err = JET_errSuccess;
        }

    }

    return err;
}


ERR ErrRECIRetrieveSeparatedLongValue(
    FUCB        *pfucb,
    const DATA& dataField,
    BOOL        fAfterImage,    //  set to fFalse to get before-images of replaces we have done
    ULONG       ibLVOffset,
    const BOOL  fEncrypted,
    VOID        *pv,
    ULONG       cbMax,
    ULONG       *pcbActual,
    JET_GRBIT   grbit );

INLINE ERR ErrRECIRetrieveFixedDefaultValue(
    const TDB       *ptdb,
    const COLUMNID  columnid,
    DATA            *pdataField )
{
    Assert( FCOLUMNIDFixed( columnid ) );
    Assert( NULL != ptdb->PdataDefaultRecord() );
    return ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    ptdb,
                    columnid,
                    *ptdb->PdataDefaultRecord(),
                    pdataField );
}

INLINE ERR ErrRECIRetrieveVarDefaultValue(
    const TDB       *ptdb,
    const COLUMNID  columnid,
    DATA            *pdataField )
{
    Assert( FCOLUMNIDVar( columnid ) );
    Assert( NULL != ptdb->PdataDefaultRecord() );
    return ErrRECIRetrieveVarColumn(
                    pfcbNil,
                    ptdb,
                    columnid,
                    *ptdb->PdataDefaultRecord(),
                    pdataField );
}

INLINE ERR ErrRECIRetrieveTaggedDefaultValue(
    FCB             *pfcb,
    const COLUMNID  columnid,
    DATA            *pdataField )
{
    Assert( FCOLUMNIDTagged( columnid ) );
    Assert( NULL != pfcb->Ptdb()->PdataDefaultRecord() );
    return ErrRECIRetrieveTaggedColumn(
                    pfcb,
                    columnid,
                    1,              //  itagSequence
                    *pfcb->Ptdb()->PdataDefaultRecord(),
                    pdataField );
}

INLINE ERR ErrRECIRetrieveDefaultValue(
    FCB             *pfcb,
    const COLUMNID  columnid,
    DATA            *pdataField )
{
    if ( FCOLUMNIDTagged( columnid ) )
        return ErrRECIRetrieveTaggedDefaultValue( pfcb, columnid, pdataField );
    else if ( FCOLUMNIDFixed( columnid ) )
        return ErrRECIRetrieveFixedDefaultValue( pfcb->Ptdb(), columnid, pdataField );
    else
    {
        Assert( FCOLUMNIDVar( columnid ) );
        return ErrRECIRetrieveVarDefaultValue( pfcb->Ptdb(), columnid, pdataField );
    }
}


//  key extraction/normalization
//
VOID FLDNormalizeFixedSegment(
    const BYTE          *pbField,
    const ULONG         cbField,
    BYTE                *rgbSeg,
    INT                 *pcbSeg,
    const JET_COLTYP    coltyp,
    const BOOL          fDotNetGuid,
    BOOL                fDataPassedFromUser = fFalse );

ERR ErrFLDNormalizeTaggedData(
    _In_ const FIELD *          pfield,
    _In_ const DATA&            dataRaw,
    _Out_ DATA&                 dataNorm,
    _In_ const NORM_LOCALE_VER* const pnlv,
    _In_ const BOOL             fDotNetGuid,
    _Out_ BOOL *                pfDataTruncated);

INLINE void RECTextColumnPadding( __out_bcount(cbData) BYTE * pbData, const ULONG cbData, const ULONG cbDataField, const USHORT cp )
{
    Assert( cbData >= cbDataField );
    
    if ( usUniCodePage == cp )
    {
        BYTE *  pbFill      = (BYTE*)&wchRECFixedColAndKeyFill;
        ULONG   cbFill      = sizeof( wchRECFixedColAndKeyFill );
        ULONG   ibDataFill  = cbDataField;


        while ( ibDataFill < cbData )
        {
            pbData[ibDataFill] = pbFill[ ibDataFill % cbFill ];
            ibDataFill++;
        }
    }
    else
    {
        memset( pbData + cbDataField, chRECFixedColAndKeyFill, cbData - cbDataField );
    }
    
}

ERR ErrRECIRetrieveKey(
    FUCB        *pfucb,
    const IDB   * const pidb,
    DATA&       lineRec,
    KEY         *pkey,
    const ULONG *rgitag,
    const ULONG ichOffset,
    const BOOL  fRetrieveLVBeforeImg,
    RCE         *prce,
    ULONG       *piidxseg );

INLINE ERR ErrRECRetrieveKeyFromCopyBuffer(
    FUCB        *pfucb,
    const IDB   * const pidb,
    KEY         *pkey,
    const ULONG *rgitag,
    const ULONG ichOffset,
    RCE         *prce,
    ULONG       *piidxseg )
{
    Assert( !Pcsr( pfucb )->FLatched() );
    const ERR   err     = ErrRECIRetrieveKey(
                                pfucb,
                                pidb,
                                pfucb->dataWorkBuf,
                                pkey,
                                rgitag,
                                ichOffset,
                                fFalse,
                                prce,
                                piidxseg );
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}

INLINE ERR ErrRECRetrieveKeyFromRecord(
    FUCB        *pfucb,
    const IDB   * const pidb,
    KEY         *pkey,
    const ULONG *rgitag,
    const ULONG ichOffset,
    const BOOL  fRetrieveLVBeforeImg,
    ULONG       *piidxseg )
{
    Assert( !Pcsr( pfucb )->FLatched() );
    const ERR   err     = ErrRECIRetrieveKey(
                                pfucb,
                                pidb,
                                pfucb->kdfCurr.data,
                                pkey,
                                rgitag,
                                ichOffset,
                                fRetrieveLVBeforeImg,
                                prceNil,
                                piidxseg );
    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}

INLINE ERR ErrRECValidIndexKeyWarning( ERR err )
{
#ifdef DEBUG
    switch ( err )
    {
        case wrnFLDOutOfKeys:
        case wrnFLDOutOfTuples:
        case wrnFLDNotPresentInIndex:
        case wrnFLDNullKey:
        case wrnFLDNullFirstSeg:
        case wrnFLDNullSeg:
            err = JET_errSuccess;
            break;
    }
#endif

    return err;
}

VOID RECIRetrieveColumnFromKey(
    TDB * const             ptdb,
    IDB * const             pidb,
    const IDXSEG * const    pidxseg,
    const FIELD * const     pfield,
    const BYTE *&           pbKey,
    const BYTE * const      pbKeyMax,
    DATA * const            plineColumn,
    BOOL * const            pfNull );

ERR ErrRECIRetrieveColumnFromKey(
    TDB             * ptdb,
    IDB             * pidb,
    const KEY       * pkey,
    const COLUMNID  columnid,
    DATA            * plineColumn );

INLINE VOID RECReleaseKeySearchBuffer( FUCB *pfucb )
{
    // release key buffer if one was allocated
    if ( NULL != pfucb->dataSearchKey.Pv() )
    {
        if ( FFUCBUsingTableSearchKeyBuffer( pfucb ) )
        {
            FUCBResetUsingTableSearchKeyBuffer( pfucb );
        }
        else
        {
            RESKEY.Free( pfucb->dataSearchKey.Pv() );
        }
        pfucb->dataSearchKey.Nullify();
        pfucb->cColumnsInSearchKey = 0;
        KSReset( pfucb );
    }
}

INLINE VOID RECDeferMoveFirst( PIB *ppib, FUCB *pfucb )
{
    CheckPIB( ppib );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );
    Assert( !FFUCBUpdatePrepared( pfucb ) );
    AssertDIRMaybeNoLatch( ppib, pfucb );

    if ( pfucb->pfucbCurIndex != pfucbNil )
    {
        DIRDeferMoveFirst( pfucb->pfucbCurIndex );
    }
    DIRDeferMoveFirst( pfucb );
    return;
}

VOID RECAddMoveFilter(
    FUCB * const                pfucb,
    PFN_MOVE_FILTER const       pfnMoveFilter,
    MOVE_FILTER_CONTEXT * const pmoveFilterContext );
VOID RECRemoveMoveFilter(
    FUCB * const                    pfucb,
    PFN_MOVE_FILTER const           pfnMoveFilter,
    MOVE_FILTER_CONTEXT ** const    ppmoveFilterContextRemoved = NULL );
ERR ErrRECCheckMoveFilter( FUCB * const pfucb, MOVE_FILTER_CONTEXT* const pmoveFilterContext );
VOID RECRemoveCursorFilter( FUCB * const pfucb );

ERR ErrRECIAddToIndex(
    FUCB        *pfucb,
    FUCB        *pfucbIdx,
    const BOOKMARK  *pbmPrimary,
    DIRFLAG     dirflag,
    RCE         *prcePrimary = prceNil );
ERR ErrRECIDeleteFromIndex(
    FUCB        *pfucb,
    FUCB        *pfucbIdx,
    BOOKMARK    *pbmPrimary,
    RCE         *prcePrimary = prceNil );
ERR ErrRECIReplaceInIndex(
    FUCB        *pfucb,
    FUCB        *pfucbIdx,
    BOOKMARK    *pbmPrimary,
    RCE         *prcePrimary = prceNil );
ERR ErrRECIPopulateSecondaryIndex(
    FUCB            *pfucb,
    FUCB            *pfucbIdx,
    BOOKMARK        *pbmPrimary );


ERR ErrRECInsert( FUCB *pfucb, BOOKMARK * const pbmPrimary );

ERR ErrRECUpgradeReplaceNoLock( FUCB *pfucb );

ERR ErrRECCallback(
        PIB * const ppib,
        FUCB * const pfucb,
        const JET_CBTYP cbtyp,
        const ULONG ulId,
        void * const pvArg1,
        void * const pvArg2,
        const ULONG ulUnused,
        BOOL *pfCallbackCalled = NULL );

#if defined( DEBUG ) || !defined( RTM )
ERR ErrRECSessionWriteConflict( FUCB *pfucb );
#else
#define ErrRECSessionWriteConflict( pfucb ) ( JET_errSuccess )
#endif


INLINE VOID RECIAllocCopyBuffer( FUCB * const pfucb )
{
    if ( NULL == pfucb->pvWorkBuf )
    {
        BFAlloc( bfasIndeterminate, &pfucb->pvWorkBuf );
        Assert ( NULL != pfucb->pvWorkBuf );

        pfucb->dataWorkBuf.SetPv( (BYTE*)pfucb->pvWorkBuf );
    }
}

INLINE VOID RECIFreeCopyBuffer( FUCB * const pfucb )
{
    if ( NULL != pfucb->pvWorkBuf )
    {
        BFFree( pfucb->pvWorkBuf );
        pfucb->pvWorkBuf = NULL;
        pfucb->dataWorkBuf.SetPv( NULL );   //  verify that no one uses BF anymore
    }
}


INLINE BOOL FRECIFirstIndexColumnMultiValued( FUCB * const pfucb, const IDB * const pidb )
{
    Assert( pfucbNil != pfucb );
    Assert( pfcbNil != pfucb->u.pfcb );
    Assert( pidbNil != pidb );

    //  if index has one or more multivalued columns, return if first column is multivalued
    //
    if ( pidb->FMultivalued() )
    {
        FCB * const     pfcb        = pfucb->u.pfcb;
        TDB * const     ptdb        = pfcb->Ptdb();

        Assert( ptdbNil != ptdb );

        pfcb->EnterDML();

        const COLUMNID  columnid    = PidxsegIDBGetIdxSeg( pidb, ptdb )->Columnid();
        Assert( FCOLUMNIDValid( columnid ) );

        const FIELDFLAG ffield      = ptdb->Pfield( columnid )->ffield;

        pfcb->LeaveDML();

        return FFIELDMultivalued( ffield );
    }

    //  if index has no multivalued columns, then first column cannot be multivalued
    //
    return fFalse;
}

//  Scrub data
ERR ErrRECScrubLVChunk(
    DATA& data,
    const CHAR chScrub );

ERR ErrRECInitAutoIncSpace( _In_ FUCB* const pfucb, QWORD qwAutoInc );
ERR ErrRECRetrieveAndReserveAutoInc(
    _In_ FUCB* const                    pfucb,
    _Out_writes_bytes_( cbMax ) VOID*   pv,
    _In_ const ULONG                    cbMax );

ERR ErrRECCreateColumnReference(
    FUCB* const             pfucb,
    const JET_COLUMNID      columnid,
    const ULONG             itagSequence,
    const LvId              lid,
    ULONG* const            pcbReference,
    BYTE** const            prgbReference,
    const JET_PFNREALLOC    pfnRealloc = NULL,
    const void* const       pvReallocContext = NULL
    );
