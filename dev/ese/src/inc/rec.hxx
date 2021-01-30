// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define FixedNullBit( ifid )    ( 1 << ( (ifid) % 8 ) )
INLINE BOOL FFixedNullBit( const BYTE *pbitNullity, const UINT ifid )
{
    return ( *pbitNullity & FixedNullBit( ifid ) );
}
INLINE VOID SetFixedNullBit( BYTE *pbitNullity, const UINT ifid )
{
    *pbitNullity |= FixedNullBit( ifid );
}
INLINE VOID ResetFixedNullBit( BYTE *pbitNullity, const UINT ifid )
{
    *pbitNullity &= ~FixedNullBit( ifid );
}

INLINE WORD IbVarOffset( const WORD ibVarOffs )
{
    return ibVarOffs & ( WORD(-1) >> 1 );
}
INLINE BOOL FVarNullBit( const WORD ibVarOffs )
{
    return ( ibVarOffs & 0x8000 );
}
INLINE VOID SetVarNullBit( WORD& ibVarOffs )
{
    ibVarOffs = WORD( ibVarOffs | 0x8000 );
}
INLINE VOID SetVarNullBit( UnalignedLittleEndian< WORD >& ibVarOffs )
{
    ibVarOffs = WORD( ibVarOffs | 0x8000 );
}
INLINE VOID ResetVarNullBit( WORD& ibVarOffs )
{
    ibVarOffs = WORD( ibVarOffs & 0x7fff );
}
INLINE VOID ResetVarNullBit( UnalignedLittleEndian< WORD >& ibVarOffs )
{
    ibVarOffs = WORD( ibVarOffs & 0x7fff );
}

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


const JET_GRBIT     grbitSetColumnInternalFlagsMask         = 0xf0000000;
const JET_GRBIT     grbitSetColumnUseDerivedBit             = 0x80000000;
const JET_GRBIT     grbitSetColumnSeparated                 = 0x40000000;
const JET_GRBIT     grbitSetColumnCompressed                = 0x20000000;
const JET_GRBIT     grbitSetColumnEncrypted                 = 0x10000000;

const JET_GRBIT     grbitRetrieveColumnInternalFlagsMask    = 0xf0000000;
const JET_GRBIT     grbitRetrieveColumnUseDerivedBit        = 0x80000000;
const JET_GRBIT     grbitRetrieveColumnDDLNotLocked         = 0x40000000;



typedef ULONG               _LID32;
typedef unsigned __int64    _LID64;

const _LID32 lidMin         = 0;
const _LID32 lid32Max       = 0x7fffffff;
const _LID64 lid64Flag      = 1ULL << 63;
const _LID64 lid64Reserved  = lid64Flag | ( 0xffULL << 56 );
const _LID64 lid64First     = lid64Flag | ( lid32Max + 2 );
const _LID64 lid64Max       = lid64Flag | ~lid64Reserved;

const _LID32 lid32MaxAllowed    = lid32Max - 256;
const _LID64 lid64MaxAllowed    = lid64Max - UINT32_MAX;

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
        Assert( fLid64 || lid <= lid32Max || g_fRepair );
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
    
    operator _LID64() const
    {
        bool fLid64 = !!( m_lid & lid64Flag );
        Assert( fLid64 || m_lid <= lid32Max || g_fRepair );
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



PERSISTED
struct LVROOT
{
    UnalignedLittleEndian< ULONG >      ulReference;
    UnalignedLittleEndian< ULONG >      ulSize;
};

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

INLINE BOOL FRECUseDerivedBitForTemplateColumnInDerivedTable(
    const COLUMNID  columnid,
    const TDB       * const ptdb )
{
    Assert( FCOLUMNIDTagged( columnid ) );
    Assert( FCOLUMNIDTemplateColumn( columnid ) );
    ptdb->AssertValidDerivedTable();

    return ( !ptdb->FESE97DerivedTable()
        || FidOfColumnid( columnid ) >= ptdb->FidTaggedFirst() );
}


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
        typedef WORD RECOFFSET;
        typedef WORD VAROFFSET;

    private:
        PERSISTED
        struct RECHDR
        {
            UnalignedLittleEndian< BYTE >       fidFixedLastInRec;
            UnalignedLittleEndian< BYTE >       fidVarLastInRec;
            UnalignedLittleEndian< RECOFFSET >  ibEndOfFixedData;
        };

        RECHDR          m_rechdr;
        BYTE            m_rgbFixed[];



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

        static INLINE LONG CbRecordMost( const LONG cbPage, const LONG cbKeyMost )
        {
            Assert( cbKeyMost >= 0 );
            Assert( cbKeyMost > 0 );
            Assert( cbKeyMost <= cbKeyMostMost );

            if ( !!FIsSmallPage() != FIsSmallPage( cbPage ) )
            {
                FireWall( "PageSizeProvidedDidNotMatchGlobPageSize" );
            }

            LONG cbRecordMost;
            if( FIsSmallPage() )
            {
                cbRecordMost = CbNDNodeMost( cbPage ) - cbKeyCount - max( JET_cbKeyMostMin, ( cbKeyCount + cbKeyMost ) );
            }
            else
            {
                cbRecordMost = CbNDNodeMost( cbPage ) - cbKeyCount - cbKeyCount  - cbKeyMost;
            }
            return cbRecordMost;
        }
        
        static INLINE LONG CbRecordMostCHECK( const LONG cbPage )
        {
            return CbRecordMost( cbPage, sizeof(DBK) );
        }

        static INLINE LONG CbRecordMostWithGlobalPageSize()
        {
            return CbRecordMost( g_cbPage, sizeof(DBK) );
        }

        static INLINE LONG CbRecordMost( const LONG cbPage, const IDB * const pidb )
        {
            const LONG cbKeyMost = ( pidbNil == pidb ) ? sizeof(DBK) : pidb->CbKeyMost();
            return CbRecordMost( cbPage, cbKeyMost );
        }

        static INLINE LONG CbRecordMost( const FCB * const pfcbTable )
        {
            Assert( pfcbTable->Ifmp() != 0 );
            Assert( pfcbTable->FTypeTable() || pfcbTable->FTypeTemporaryTable() || pfcbTable->FTypeSort() );

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
    m_rechdr.fidFixedLastInRec = fidFixedLeast-1;
    m_rechdr.fidVarLastInRec = fidVarLeast-1;
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
    Assert( fidFixed >= fidFixedLeast-1 );
    Assert( fidFixed <= fidFixedMost );
    return fidFixed;
}
INLINE VOID REC::SetFidFixedLastInRec( const FID fid )
{
    Assert( fid >= fidFixedLeast-1 );
    Assert( fid <= fidFixedMost );
    m_rechdr.fidFixedLastInRec = (BYTE)fid;
}

INLINE FID REC::FidVarLastInRec() const
{
    const FID   fidVar = (FID)m_rechdr.fidVarLastInRec;
    Assert( fidVar >= fidVarLeast-1 );
    Assert( fidVar <= fidVarMost );
    return fidVar;
}
INLINE VOID REC::SetFidVarLastInRec( const FID fid )
{
    Assert( fid >= fidVarLeast-1 );
    Assert( fid <= fidVarMost );
    m_rechdr.fidVarLastInRec = (BYTE)fid;
}

INLINE ULONG REC::CFixedColumns() const
{
    Assert( FidFixedLastInRec() + 1 >= fidFixedLeast );
    return ( FidFixedLastInRec() + 1 - fidFixedLeast );
}
INLINE ULONG REC::CVarColumns() const
{
    Assert( FidVarLastInRec() + 1 >= fidVarLeast );
    return ( FidVarLastInRec() + 1 - fidVarLeast );
}

INLINE REC::RECOFFSET REC::IbEndOfFixedData() const
{
    return m_rechdr.ibEndOfFixedData;
}
INLINE VOID REC::SetIbEndOfFixedData( const RECOFFSET ib )
{
    Assert( ib < g_cbPage );
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
    Assert( FidVarLastInRec() >= fidVarLeast-1 );
    return (UnalignedLittleEndian<VAROFFSET> *)( ( (BYTE *)this ) + IbEndOfFixedData() );
}

INLINE REC::VAROFFSET REC::IbVarOffsetStart( const FID fidVar ) const
{
    Assert( fidVar >= fidVarLeast );
    Assert( fidVar <= (FID)m_rechdr.fidVarLastInRec );

    return ( fidVarLeast == fidVar ?
                (REC::VAROFFSET)0 :
                IbVarOffset( PibVarOffsets()[fidVar-fidVarLeast-1] ) );
}

INLINE REC::VAROFFSET REC::IbVarOffsetEnd( const FID fidVar ) const
{
    Assert( fidVar >= fidVarLeast );
    Assert( fidVar <= (FID)m_rechdr.fidVarLastInRec );
    return ( IbVarOffset( PibVarOffsets()[fidVar-fidVarLeast] ) );
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
    if ( FidVarLastInRec() == fidVarLeast-1 )
        return 0;

    return IbVarOffset( PibVarOffsets()[FidVarLastInRec()-fidVarLeast] );
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

    if ( cbUserPreferred == 0 )
    {
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

        if ( m_fMultivalued )
        {
            if ( pidb->FTemplateIndex() )
            {
                Assert( pfcb->FDerivedTable() || pfcb->FTemplateTable() );
                if ( pfcb->FDerivedTable() )
                {
                    pfcb = pfcb->Ptdb()->PfcbTemplateTable();
                    Assert( pfcbNil != pfcb );
                    Assert( pfcb->FTemplateTable() );
                }
                else
                {
                    Assert( pfcb->Ptdb()->PfcbTemplateTable() == pfcbNil );
                }
            }

            pfcb->EnterDML();
            UtilMemCpy( rgidxseg, PidxsegIDBGetIdxSeg( pidb, pfcb->Ptdb() ), m_iidxsegMac * sizeof(IDXSEG) );
            pfcb->LeaveDML();

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

            for ( iidxsegT = 0; iidxsegT < m_iidxsegMac; iidxsegT++ )
            {
                const COLUMNID columnid = rgidxseg[iidxsegT].Columnid();

                Assert( columnid == m_rgcolumnid[iidxsegT] );
                Assert( fFalse == m_rgfMultiValued[iidxsegT] );

                if ( FCOLUMNIDTagged( columnid ) )
                {
                    pfcb->EnterDML();
                    field = *( pfcb->Ptdb()->Pfield( columnid ) );
                    pfcb->LeaveDML();

                    if ( FFIELDMultivalued( field.ffield ) )
                    {
                        m_rgfMultiValued[iidxsegT] = fTrue;
                        m_iidxsegMultiValuedMost = iidxsegT;

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

        if ( iidxsegNoRollover == iidxsegRollover )
        {
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
            Assert( !m_fKeySequenceComplete );

            if ( iidxsegComplete == iidxsegRollover )
            {
                m_fKeySequenceComplete = fTrue;
                return;
            }

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

        Assert( m_fMultivalued );
        Assert( m_iidxsegMac <= JET_ccolKeyMost );

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
};

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
            memset( pbData, 0, cbData );
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
            memset( pbData, 0, cbData );
            return cbData;
        }
    }
}

const ULONG fLVReference    = 0;
const ULONG fLVDereference  = 1;

const SHORT cbFLDBinaryChunk            = 8;
const SHORT cbFLDBinaryChunkNormalized  = cbFLDBinaryChunk+1;

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
#endif

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
    ULONG               cbThreshold = 0 );

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
    QWORD * const   pcbLVDataLogical,
    QWORD * const   pcbLVDataPhysical,
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
    __in FUCB               * const pfucb,
    __in const DATA         * const pdataField,
    __in const CompressFlags  compressFlags,
    __in const BOOL         fEncrypted,
    __out LvId              * const plid,
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

    Assert( prec->FidFixedLastInRec() >= fidFixedLeast-1 );
    Assert( prec->FidFixedLastInRec() <= fidFixedMost );

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
        Assert( prec->FidFixedLastInRec() >= fidFixedLeast );

        const UINT  ifid            = FidOfColumnid( columnid ) - fidFixedLeast;
        const BYTE  * prgbitNullity = prec->PbFixedNullBitMap() + ifid/8;

        if ( FFixedNullBit( prgbitNullity, ifid ) )
            err = ErrERRCheck( JET_wrnColumnNull );
        else
            err = JET_errSuccess;
    }

    return err;
}

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

    Assert( prec->FidVarLastInRec() >= fidVarLeast-1 );
    Assert( prec->FidVarLastInRec() <= fidVarMost );

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

        Assert( prec->FidVarLastInRec() >= fidVarLeast );
        Assert( prec->PbVarData() + IbVarOffset( pibVarOffs[prec->FidVarLastInRec()-fidVarLeast] )
                <= (BYTE *)dataRec.Pv() + dataRec.Cb() );

        const UINT  ifid    = FidOfColumnid( columnid ) - fidVarLeast;

        if ( FVarNullBit( pibVarOffs[ifid] ) )
        {
#ifdef DEBUG
            const WORD  ibVarOffset = ( fidVarLeast == FidOfColumnid( columnid ) ?
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
    BOOL        fAfterImage,
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
                    1,
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
    AssertDIRNoLatch( ppib );

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

ERR ErrRECInsert( FUCB *pfucb, BOOKMARK * const pbmPrimary );

ERR ErrRECUpgradeReplaceNoLock( FUCB *pfucb );

ERR ErrRECCallback(
        PIB * const ppib,
        FUCB * const pfucb,
        const JET_CBTYP cbtyp,
        const ULONG ulId,
        void * const pvArg1,
        void * const pvArg2,
        const ULONG ulUnused );

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
        pfucb->dataWorkBuf.SetPv( NULL );
    }
}


INLINE BOOL FRECIFirstIndexColumnMultiValued( FUCB * const pfucb, const IDB * const pidb )
{
    Assert( pfucbNil != pfucb );
    Assert( pfcbNil != pfucb->u.pfcb );
    Assert( pidbNil != pidb );

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

    return fFalse;
}

ERR ErrRECScrubLVChunk(
    DATA& data,
    const CHAR chScrub );

ERR ErrRECInitAutoIncSpace( _In_ FUCB* const pfucb, QWORD qwAutoInc );

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
