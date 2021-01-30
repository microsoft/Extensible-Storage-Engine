// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

PERSISTED
class TAGFLD_HEADER
{
    friend class TAGFIELDS_ITERATOR;
    friend class TAGFIELDS;
    friend class CTaggedColumnIter;

    private:
        TAGFLD_HEADER( const JET_COLTYP coltyp, const BOOL fSeparatedLV, const BOOL fNullVal, const BOOL fCompressedLV, const BOOL fEncryptedLV )
        {
            Assert( !( fSeparatedLV && fCompressedLV ) );
            m_bHeader = 0;

            if ( FRECLongValue( coltyp ) )
                m_bHeader |= BYTE( fLongValue );

            if ( fSeparatedLV )
            {
                Assert( FLongValue() );
                m_bHeader |= BYTE( fSeparated );
            }

            if ( fCompressedLV )
            {
                Assert( FLongValue() );
                m_bHeader |= BYTE( fCompressed );
            }

            if ( fEncryptedLV )
            {
                Assert( FLongValue() );
                m_bHeader |= BYTE( fEncrypted );
            }

            if ( fNullVal && !FIsSmallPage() )
            {
                m_bHeader |= BYTE( fNull );
            }
        }
        
    private:
        enum { fLongValue       = 0x01 };
        enum { fCompressed      = 0x02 };
        enum { fSeparated       = 0x04 };
        enum { fMultiValues     = 0x08 };
        enum { fTwoValues       = 0x10 };
        enum { fNull            = 0x20 };
        enum { fEncrypted       = 0x40 };
        enum { maskFlags        = 0x7f };

        BYTE        m_bHeader;

    public:
        BOOL        FNeedExtendedInfo() const           { return !FIsSmallPage() || m_bHeader; }
        BOOL        FSingleValue() const                { return ( m_bHeader & ( fMultiValues | fTwoValues | fNull ) ) == 0; }
        BOOL        FLongValue() const                  { return ( m_bHeader & fLongValue ); }
        BOOL        FSeparated() const                  { return ( m_bHeader & fSeparated ); }
        BOOL        FMultiValues() const                { return ( m_bHeader & fMultiValues ); }
        BOOL        FTwoValues() const                  { return ( m_bHeader & fTwoValues ); }
        BOOL        FNull( void ) const                 { return ( m_bHeader & fNull ); }
        BOOL        FColumnCanBeSeparated() const       { return ( m_bHeader & fLongValue ); }
        BOOL        FCompressed() const                 { return ( m_bHeader & fCompressed ); }
        BOOL        FEncrypted() const                  { return ( m_bHeader & fEncrypted ); }

        VOID        SetFLongValue()                     { m_bHeader |= BYTE( fLongValue ); }
        VOID        SetFSeparated()                     { m_bHeader |= BYTE( fSeparated ); }
        VOID        ResetFSeparated()                   { m_bHeader &= BYTE( ~fSeparated ); }
        VOID        SetFMultiValues()                   { m_bHeader |= BYTE( fMultiValues ); }
        VOID        ResetFMultiValues()                 { m_bHeader &= BYTE( ~fMultiValues ); }
        VOID        SetFTwoValues()                     { m_bHeader |= BYTE( fTwoValues ); }
        VOID        ResetFTwoValues()                   { m_bHeader &= BYTE( ~fTwoValues ); }
        VOID        SetFNull( void )                    { m_bHeader |= BYTE( fNull ); }
        VOID        ResetFNull( void )                  { m_bHeader &= BYTE( ~fNull ); }
        VOID        SetFCompressed()                    { m_bHeader |= BYTE( fCompressed ); }
        VOID        ResetFCompressed()                  { m_bHeader &= BYTE( ~fCompressed ); }
        VOID        SetFEncrypted()                     { m_bHeader |= BYTE( fEncrypted ); }
        VOID        ResetFEncrypted()                   { m_bHeader &= BYTE( ~fEncrypted ); }

        ERR         ErrRetrievalResult() const;
        ERR         ErrMultiValueRetrievalResult( const BOOL fCompressedLV, const BOOL fSeparatedLV ) const;
};


INLINE ERR TAGFLD_HEADER::ErrRetrievalResult() const
{
    Assert( !FMultiValues() );
    if ( FLongValue() )
    {
        if( FSeparated() )
        {
            Assert( !FCompressed() );
            Assert( !FEncrypted() );
            return ErrERRCheck( wrnRECSeparatedLV );
        }
        else if( FCompressed() )
        {
            return ErrERRCheck( wrnRECCompressed );
        }
        return ErrERRCheck( wrnRECIntrinsicLV );
    }
    else
    {
        Assert( !FCompressed() );
        Assert( !FEncrypted() );
#ifdef UNLIMITED_MULTIVALUES
#else
        Assert( !FSeparated() );
#endif
        return JET_errSuccess;
    }
}

INLINE ERR TAGFLD_HEADER::ErrMultiValueRetrievalResult(
    const BOOL  fCompressedLV,
    const BOOL  fSeparatedLV ) const
{
    Assert( FMultiValues() );
    if ( FLongValue() )
    {
        if( fSeparatedLV )
        {
            Assert( !fCompressedLV );
            return ErrERRCheck( wrnRECSeparatedLV );
        }
        else if( fCompressedLV )
        {
            return ErrERRCheck( wrnRECCompressed );
        }
        return ErrERRCheck( wrnRECIntrinsicLV );
    }
    else
    {
        Assert( !fSeparatedLV );
#ifdef UNLIMITED_MULTIVALUES
#else
        Assert( !fCompressedLV );
#endif
        return JET_errSuccess;
    }
}


    

PERSISTED
class TAGFLD
{
    friend class TAGFIELDS;
    friend class TAGFIELDS_ITERATOR;
    friend class CTaggedColumnIter;


    private:
        TAGFLD( const FID fid, const BOOL fDerivedColumn )
        {
            m_fid = fid;
            m_ib = 0;

            if ( fDerivedColumn )
                m_ib = WORD( m_ib | fDerived );
        }
        ~TAGFLD()   {}

    public:
        static VOID InitStaticMembersWithPageSize( ULONG cbPage );
        
    private:
        typedef UnalignedLittleEndian<FID>      TAGFID;
        typedef UnalignedLittleEndian<WORD>     TAGOFFSET;

        enum { fDerived             = 0x8000 };
        enum { fNullSmallPage       = 0x2000 };

        static WORD fExtendedInfo;
        static WORD maskIb;

    private:
        TAGFID      m_fid;
        TAGOFFSET   m_ib;

    private:
        FID         Fid() const                     { return m_fid; }
        ULONG       Ib() const                      { return ( m_ib & maskIb ); }
        BOOL        FDerived() const                { return ( m_ib & fDerived ); }
        BOOL        FExtendedInfo() const;
        BOOL        FNull( const TAGFIELDS* const ptagfields ) const;

        VOID        SetFDerived()                   { m_ib = WORD( m_ib | fDerived ); }
        VOID        SetFExtendedInfo()              { m_ib = WORD( m_ib | fExtendedInfo ); }
        VOID        ResetFExtendedInfo();
        VOID        SetFNull( const TAGFIELDS* const ptagfields );
        VOID        ResetFNull( const TAGFIELDS* const ptagfields );

        VOID        SetIb( const USHORT ib );
        VOID        DeltaIb( const SHORT delta );

        BOOL        FTemplateColumn( const TDB * const ptdb ) const;
        COLUMNID    Columnid( const TDB * const ptdb ) const;

        BOOL        FIsEqual( const FID fid, const BOOL fDerived ) const;
        BOOL        FIsLessThan( const FID fid, const BOOL fDerived ) const;
        BOOL        FIsLessThanOrEqual( const FID fid, const BOOL fDerived ) const;
        BOOL        FIsGreaterThan( const FID fid, const BOOL fDerived ) const;
        BOOL        FIsGreaterThanOrEqual( const FID fid, const BOOL fDerived ) const;

        BOOL        FIsEqual( const COLUMNID columnid, const TDB * const ptdb  ) const;
        BOOL        FIsLessThan( const COLUMNID columnid, const TDB * const ptdb  ) const;
        BOOL        FIsLessThanOrEqual( const COLUMNID columnid, const TDB * const ptdb  ) const;
        BOOL        FIsGreaterThan( const COLUMNID columnid, const TDB * const ptdb  ) const;
        BOOL        FIsGreaterThanOrEqual( const COLUMNID columnid, const TDB * const ptdb  ) const;

    private:
        static BOOL CmpTagfld(
                        const TAGFLD&   ptagfld1,
                        const TAGFLD&   ptagfld2 );
#ifdef DEBUG
        static BOOL CmpTagfld1(
                        const TAGFLD&   ptagfld1,
                        const TAGFLD&   ptagfld2 );
#endif
        static BOOL CmpTagfld2(
                        const TAGFLD&   ptagfld1,
                        const TAGFLD&   ptagfld2 );
#ifdef DEBUG
        static BOOL CmpTagfldSlow(
                        const TAGFLD&   ptagfld1,
                        const TAGFLD&   ptagfld2 );
        static VOID CmpTagfldTest();
#endif
};


INLINE VOID TAGFLD::InitStaticMembersWithPageSize( ULONG cbPage )
{
    const bool fSmallPage = FIsSmallPage( cbPage );
    fExtendedInfo   = fSmallPage ? 0x4000 : 0x0000;
    maskIb      = fSmallPage ? 0x1fff : 0x7fff;
}

INLINE BOOL TAGFLD::FExtendedInfo( ) const
{
#ifdef DEBUG
    if ( FIsSmallPage() )
    {
        Assert( !( ( m_ib & fExtendedInfo ) && ( m_ib & fNullSmallPage) ) );
    }
#endif
    return !fExtendedInfo || ( m_ib & fExtendedInfo );
}

INLINE VOID TAGFLD::ResetFExtendedInfo()
{
    m_ib &= WORD( ~fExtendedInfo );
}


INLINE VOID TAGFLD::SetIb( const USHORT ib )
{
    Assert( !( ib & ~maskIb ) );
    const USHORT    ibOld           = m_ib;
    const USHORT    ibNew           = ( ibOld & ~maskIb ) | ( ib & maskIb );
    m_ib = ibNew;
}

INLINE VOID TAGFLD::DeltaIb( const SHORT delta )
{
    Assert( 0 != delta );
    Assert( delta > 0 || (SHORT)Ib() + delta > 0 );
    const USHORT    ibT     = USHORT( Ib() + delta );
    Assert( ( delta > 0 && ibT > Ib() )
        || ( delta < 0 && ibT < Ib() ) );
    SetIb( ibT );
}

INLINE BOOL TAGFLD::FTemplateColumn( const TDB * const ptdb ) const
{
    return ( FDerived()
            || ptdb->FTemplateTable()
            || ( ptdb->FESE97DerivedTable() && Fid() < ptdb->FidTaggedFirst() ) );
}

INLINE COLUMNID TAGFLD::Columnid( const TDB * const ptdb ) const
{
    return ColumnidOfFid( Fid(), FTemplateColumn( ptdb ) );
}

INLINE BOOL TAGFLD::FIsEqual( const FID fid, const BOOL fDerived ) const
{
    Assert( FTaggedFid( fid ) );
    return ( Fid() == fid
        && ( ( FDerived() && fDerived ) || ( !FDerived() && !fDerived ) ) );
}
INLINE BOOL TAGFLD::FIsLessThan( const FID fid, const BOOL fDerived ) const
{
    Assert( FTaggedFid( fid ) );
    if ( FDerived() )
    {
        return ( fDerived ?
                    ( Fid() < fid ) :
                    fTrue );
    }
    else
    {
        return ( fDerived ?
                    fFalse :
                    ( Fid() < fid ) );
    }
}
INLINE BOOL TAGFLD::FIsLessThanOrEqual( const FID fid, const BOOL fDerived ) const
{
    Assert( FTaggedFid( fid ) );
    if ( FDerived() )
    {
        return ( fDerived ?
                    ( Fid() <= fid ) :
                    fTrue );
    }
    else
    {
        return ( fDerived ?
                    fFalse :
                    ( Fid() <= fid ) );
    }
}
INLINE BOOL TAGFLD::FIsGreaterThan( const FID fid, const BOOL fDerived ) const
{
    Assert( FTaggedFid( fid ) );
    if ( FDerived() )
    {
        return ( fDerived ?
                    ( Fid() > fid ) :
                    fFalse );
    }
    else
    {
        return ( fDerived ?
                    fTrue :
                    ( Fid() > fid ) );
    }
}
INLINE BOOL TAGFLD::FIsGreaterThanOrEqual( const FID fid, const BOOL fDerived ) const
{
    Assert( FTaggedFid( fid ) );
    if ( FDerived() )
    {
        return ( fDerived ?
                    ( Fid() >= fid ) :
                    fFalse );
    }
    else
    {
        return ( fDerived ?
                    fTrue :
                    ( Fid() >= fid ) );
    }
}


INLINE BOOL TAGFLD::FIsEqual( const COLUMNID columnid, const TDB * const ptdb  ) const
{
    return FIsEqual( FidOfColumnid( columnid ), FRECUseDerivedBit( columnid, ptdb ) );
}
INLINE BOOL TAGFLD::FIsLessThan( const COLUMNID columnid, const TDB * const ptdb  ) const
{
    return FIsLessThan( FidOfColumnid( columnid ), FRECUseDerivedBit( columnid, ptdb ) );
}
INLINE BOOL TAGFLD::FIsLessThanOrEqual( const COLUMNID columnid, const TDB * const ptdb  ) const
{
    return FIsLessThanOrEqual( FidOfColumnid( columnid ), FRECUseDerivedBit( columnid, ptdb ) );
}
INLINE BOOL TAGFLD::FIsGreaterThan( const COLUMNID columnid, const TDB * const ptdb  ) const
{
    return FIsGreaterThan( FidOfColumnid( columnid ), FRECUseDerivedBit( columnid, ptdb ) );
}
INLINE BOOL TAGFLD::FIsGreaterThanOrEqual( const COLUMNID columnid, const TDB * const ptdb  ) const
{
    return FIsGreaterThanOrEqual( FidOfColumnid( columnid ), FRECUseDerivedBit( columnid, ptdb ) );
}



class TWOVALUES
{
    friend class TAGFLD_ITERATOR_TWOVALUES;
    friend class TAGFIELDS;
    friend class CDualValuedTaggedColumnValueIter;

    private:
        TWOVALUES( BYTE * const pbTwoValues, const ULONG cbTwoValues );
        ~TWOVALUES()    {}

    private:
        typedef BYTE    TVLENGTH;
    
        TAGFLD_HEADER   * m_pheader;
        TVLENGTH        * m_pcbFirstValue;
        TVLENGTH        m_cbSecondValue;
        BYTE            * m_pbData;

    private:
        TAGFLD_HEADER   * Pheader()                             { return m_pheader; }
        BYTE            * PcbFirstValue()                       { return m_pcbFirstValue; }
        BYTE            CbFirstValue() const                    { return *m_pcbFirstValue; }
        BYTE            CbSecondValue() const                   { return m_cbSecondValue; }
        BYTE            * PbData()                              { return m_pbData; }
        const BYTE      * PbData() const                        { return m_pbData; }

        VOID            RetrieveInstance(
                            const ULONG         itagSequence,
                            DATA                * const pdataRetrieveBuffer );
        VOID            UpdateInstance(
                            const ULONG         itagSequence,
                            const DATA          * const pdataToSet,
                            const JET_COLTYP    coltyp );

        ERR             ErrCheckUnique(
                            _In_ const FIELD * const    pfield,
                            _In_ const DATA&            dataToSet,
                            _In_ const ULONG            itagSequence,
                            _In_ const NORM_LOCALE_VER* const   pnlv,
                            _In_ const BOOL             fNormalizedDataToSetIsTruncated );
};

INLINE TWOVALUES::TWOVALUES(
    BYTE            * const pbTwoValues,
    const ULONG     cbTwoValues )
{
    Assert( NULL != pbTwoValues );
    Assert( cbTwoValues >= sizeof(TAGFLD_HEADER) + sizeof(TVLENGTH) );

    m_pheader = (TAGFLD_HEADER *)pbTwoValues;
    Assert( Pheader()->FMultiValues() );
    Assert( Pheader()->FTwoValues() );
    Assert( !Pheader()->FColumnCanBeSeparated() );
    Assert( !Pheader()->FSeparated() );

    m_pcbFirstValue = pbTwoValues + 1;

    Assert( cbTwoValues >= sizeof(TAGFLD_HEADER) + sizeof(TVLENGTH) + *m_pcbFirstValue );

    const SIZE_T cbSecondValue = cbTwoValues - ( sizeof(TAGFLD_HEADER) + sizeof(TVLENGTH) + *m_pcbFirstValue );
    Assert( cbSecondValue <= 0xff );
    m_cbSecondValue = BYTE( cbSecondValue );

    m_pbData = pbTwoValues + sizeof(TAGFLD_HEADER) + sizeof(TVLENGTH);
}


INLINE VOID TWOVALUES::RetrieveInstance(
    const ULONG     itagSequence,
    DATA            * const pdataRetrieveBuffer )
{
    Assert( !Pheader()->FLongValue() );

    if ( 1 == itagSequence )
    {
        pdataRetrieveBuffer->SetPv( PbData() );
        pdataRetrieveBuffer->SetCb( CbFirstValue() );
    }
    else
    {
        Assert( 2 == itagSequence );
        pdataRetrieveBuffer->SetPv( PbData() + CbFirstValue() );
        pdataRetrieveBuffer->SetCb( CbSecondValue() );
    }
}




class MULTIVALUES
{
    friend class TAGFLD_ITERATOR_MULTIVALUES;
    friend class TAGFIELDS;
    friend class CMultiValuedTaggedColumnValueIter;

    private:
        MULTIVALUES( const BYTE * const pbMultiValues, const ULONG cbMultiValues );
        ~MULTIVALUES()  {}

    private:
        typedef UnalignedLittleEndian<USHORT>   MVOFFSET;

        enum { fSeparatedInstance   = 0x8000 };
        enum { maskFlags            = 0x8000 };
        enum { maskIb               = 0x7fff };

        TAGFLD_HEADER   * m_pheader;
        MVOFFSET        * m_rgmvoffs;
        ULONG           m_cbMultiValues;
        ULONG           m_cMultiValues;

    private:
        VOID            Refresh(
                            const BYTE      * pbMultiValues,
                            const ULONG     cbMultiValues );

        TAGFLD_HEADER   * Pheader()                                 { return m_pheader; }
        const TAGFLD_HEADER * Pheader() const                       { return m_pheader; }
        MVOFFSET        * Rgmvoffs()                                { return m_rgmvoffs; }
        const MVOFFSET      * Rgmvoffs() const                  { return m_rgmvoffs; }
        BYTE            * PbMultiValues()                           { return (BYTE *)m_rgmvoffs; }
        const BYTE      * PbMultiValues() const                     { return (BYTE *)m_rgmvoffs; }
        ULONG           CbMultiValues() const                       { return m_cbMultiValues; }
        ULONG           CMultiValues() const                        { Assert( m_cMultiValues > 1 ); return m_cMultiValues; }
        BYTE            * PbMax()                                   { return PbMultiValues() + CbMultiValues(); }

        ULONG           IbStartOfMVData() const;
        BYTE            * PbStartOfMVData()                         { return PbMultiValues() + IbStartOfMVData(); }
        ULONG           CbMVData() const                            { return CbMultiValues() - IbStartOfMVData(); }

        ULONG           Ib( const ULONG imv ) const                 { return ( m_rgmvoffs[imv] & maskIb ); }
        VOID            SetIb( const ULONG imv, const USHORT ib );
        VOID            DeltaIb( const ULONG imv, const SHORT delta );
        BYTE            * PbData( const ULONG imv );
        const BYTE      * PbData( const ULONG imv ) const;
        ULONG           CbData( const ULONG imv ) const;

        BOOL            FSeparatedInstance( const ULONG imv ) const { return ( m_rgmvoffs[imv] & fSeparatedInstance ); }
        VOID            SetFSeparatedInstance( const ULONG imv )    { m_rgmvoffs[imv] = USHORT( m_rgmvoffs[imv] | fSeparatedInstance ); }
        VOID            ResetFSeparatedInstance( const ULONG imv)   { m_rgmvoffs[imv] = USHORT( m_rgmvoffs[imv] & ~fSeparatedInstance ); }

        ERR             ErrRetrieveInstance(
                            const ULONG         itagSequence,
                            DATA                * const pdataRetrieveBuffer );
        VOID            AddInstance(
                            const DATA          * const pdataToSet,
                            const JET_COLTYP    coltyp,
                            const BOOL          fSeparatedLV );
        VOID            RemoveInstance(
                            const ULONG         itagSequence );
        VOID            UpdateInstance(
                            const ULONG         itagSequence,
                            const DATA          * const pdataToSet,
                            const JET_COLTYP    coltyp,
                            const BOOL          fSeparatedLV,
                            const BOOL          fCompressedLV );

        BOOL            FValidate(
                                  CPRINTF       * const pcprintf ) const;

        ERR             ErrCheckUnique(
                            _In_ const FIELD * const    pfield,
                            _In_ const DATA&            dataToSet,
                            _In_ const ULONG            itagSequence,
                            _In_ const NORM_LOCALE_VER* pnlv,
                            _In_ const BOOL             fNormalizedDataToSetIsTruncated );
};

INLINE MULTIVALUES::MULTIVALUES(
    const BYTE  * const pbMultiValues,
    const ULONG cbMultiValues )
{
    Assert( NULL != pbMultiValues );
    Assert( cbMultiValues >= sizeof(TAGFLD_HEADER) + sizeof(MVOFFSET) );

    m_pheader = (TAGFLD_HEADER *)pbMultiValues;
    Assert( Pheader()->FMultiValues() );
    Assert( !Pheader()->FTwoValues() );

#ifdef UNLIMITED_MULTIVALUES
#else
    Assert( !Pheader()->FSeparated() );
#endif
    
    m_rgmvoffs = (MVOFFSET *)( pbMultiValues + sizeof(TAGFLD_HEADER) );
    m_cbMultiValues = cbMultiValues - (ULONG) sizeof(TAGFLD_HEADER);

    Assert( Ib( 0 ) >= 2 * sizeof(MVOFFSET) );
    Assert( Ib( 0 ) <= CbMultiValues() );
    Assert( Ib( 0 ) % sizeof(MVOFFSET) == 0 );
    m_cMultiValues = Ib( 0 ) / sizeof(MVOFFSET);

    Assert( CMultiValues() > 1 );
}

INLINE VOID MULTIVALUES::Refresh(
    const BYTE  * const pbMultiValues,
    const ULONG cbMultiValues )
{
    Assert( NULL != pbMultiValues );
    Assert( cbMultiValues >= sizeof(TAGFLD_HEADER) + sizeof(MVOFFSET) );

    m_pheader = (TAGFLD_HEADER *)pbMultiValues;
    Assert( Pheader()->FMultiValues() );
    Assert( !Pheader()->FTwoValues() );

#ifdef UNLIMITED_MULTIVALUES
#else
    Assert( !Pheader()->FSeparated() );
#endif
    
    m_rgmvoffs = (MVOFFSET *)( pbMultiValues + sizeof(TAGFLD_HEADER) );
    
    Assert( m_cbMultiValues == cbMultiValues - sizeof(TAGFLD_HEADER) );

    Assert( Ib( 0 ) >= 2 * sizeof(MVOFFSET) );
    Assert( Ib( 0 ) <= CbMultiValues() );

#ifdef DEBUG
    const BYTE  * const pbMVData    = PbMultiValues() + Ib( 0 );
    Assert( ( pbMVData - PbMultiValues() ) % sizeof(MVOFFSET) == 0 );
    Assert( m_cMultiValues == ( pbMVData - PbMultiValues() ) / sizeof(MVOFFSET) );
#endif

    Assert( CMultiValues() > 1 );
}

INLINE ULONG MULTIVALUES::IbStartOfMVData() const
{
    Assert( CMultiValues() > 1 );
    Assert( Ib( 0 ) >= 2 * sizeof(MVOFFSET) );
    Assert( Ib( 0 ) <= CbMultiValues() );
    return Ib( 0 );
}


INLINE VOID MULTIVALUES::SetIb( const ULONG imv, const USHORT ib )
{
    Assert( imv < CMultiValues() );
    Assert( !( ib & maskFlags ) );
    const USHORT    usOffsetT   = m_rgmvoffs[imv];
    m_rgmvoffs[imv] = USHORT( ( usOffsetT & maskFlags ) | ( ib & maskIb ) );
}

INLINE VOID MULTIVALUES::DeltaIb( const ULONG imv, const SHORT delta )
{
    Assert( imv < CMultiValues() );
    Assert( 0 != delta );
    Assert( delta > 0 || (SHORT)Ib( imv ) + delta > 0 );
    const USHORT    ibT     = USHORT( Ib( imv ) + delta );
    Assert( ( delta > 0 && ibT > Ib( imv ) )
        || ( delta < 0 && ibT < Ib( imv ) ) );
    SetIb( imv, ibT );
}

INLINE BYTE * MULTIVALUES::PbData( const ULONG imv )
{
    Assert( CMultiValues() > 1 );
    Assert( imv < CMultiValues() );

    return PbMultiValues() + Ib( imv );
}

INLINE const BYTE * MULTIVALUES::PbData( const ULONG imv ) const
{
    Assert( CMultiValues() > 1 );
    Assert( imv < CMultiValues() );

    return PbMultiValues() + Ib( imv );
}

INLINE ULONG MULTIVALUES::CbData( const ULONG imv ) const
{
    Assert( CMultiValues() > 1 );
    Assert( imv < CMultiValues() );

    const ULONG     ibStart     = Ib( imv );
    const ULONG     ibEnd       = ( imv < CMultiValues() - 1 ?
                                        Ib( imv+1 ) :
                                        CbMultiValues() );
    Assert( ibEnd >= ibStart );
    return ( ibEnd - ibStart );
}


INLINE ERR MULTIVALUES::ErrRetrieveInstance(
    const ULONG     itagSequence,
    DATA            * const pdataRetrieveBuffer )
{
    const ULONG     imvRetrieve     = itagSequence - 1;

    pdataRetrieveBuffer->SetPv( PbData( imvRetrieve ) );
    pdataRetrieveBuffer->SetCb( CbData( imvRetrieve ) );

    return ( Pheader()->ErrMultiValueRetrievalResult(
                Pheader()->FCompressed() && ( 0 == imvRetrieve ),
                FSeparatedInstance( imvRetrieve ) ) );
}

    

class RECCHECKTABLE;

class TAGFIELDS
{
    friend class TAGFLD;
    friend class TAGFIELDS_ITERATOR;
    friend class CTaggedColumnIter;
    
    public:
        TAGFIELDS( const DATA& dataRec );
        virtual ~TAGFIELDS()    {}

    private:
        TAGFLD          * m_rgtagfld;
        ULONG           m_cbTaggedColumns;
        ULONG           m_cTaggedColumns;
        
    private:
        VOID            Refresh( const DATA& dataRec );
    
        TAGFLD          * Rgtagfld()                            { return m_rgtagfld; }
        BYTE            * PbTaggedColumns()                     { return (BYTE *)m_rgtagfld; }
        BYTE            * PbMax()                               { return PbTaggedColumns() + CbTaggedColumns(); }

        BYTE            * PbStartOfTaggedData()                 { return PbTaggedColumns() + IbStartOfTaggedData(); }

        TAGFLD          * Ptagfld( const ULONG itagfld );
        BYTE            * PbData( const ULONG itagfld );

        TAGFLD_HEADER   * Pheader( const ULONG itagfld );

        static const TAGFLD* PtagfldLowerBound( const TAGFLD* ptagfldStart, const TAGFLD* ptagfldMax, const TAGFLD& tagfldFind );

        VOID            InsertTagfld(
                            const ULONG         itagfldInsert,
                            TAGFLD              * const ptagfldInsert,
                            const DATA          * const pdataToInsert,
                            const JET_COLTYP    coltyp,
                            const BOOL          fSeparatedLV,
                            const BOOL          fCompressedLV,
                            const BOOL          fEncryptedLV );
        VOID            ResizeTagfld(
                            const ULONG         itagfldResize,
                            const INT           delta );
        VOID            ReplaceTagfldData(
                            const ULONG         itagfldReplace,
                            const DATA          * const pdataNew,
                            const JET_COLTYP    coltyp,
                            const BOOL          fSeparatedLV,
                            const BOOL          fCompressedLV,
                            const BOOL          fEncryptedLV );
        VOID            DeleteTagfld( const ULONG itagfldDelete );
        VOID            ConvertToTwoValues(
                            const ULONG         itagfld,
                            const DATA          * const pdataToSet,
                            const JET_COLTYP    coltyp );
        VOID            ConvertToMultiValues(
                            const ULONG         itagfld,
                            const DATA          * const pdataToSet,
                            const BOOL          fDataToSetIsSeparated );
        ULONG           CbConvertTwoValuesToSingleValue(
                            const ULONG         itagfld,
                            const ULONG         itagSequence );
        ULONG           CbDeleteMultiValue(
                            const ULONG         itagfld,
                            const ULONG         itagSequence );
        VOID            ConvertTwoValuesToMultiValues(
                            TWOVALUES           * const ptv,
                            const DATA          * const pdataToSet,
                            const JET_COLTYP    coltyp );

        ERR             ErrCheckUniqueMultiValues(
                            _In_ const FIELD * const    pfield,
                            _In_ const DATA&            dataToSet,
                            _In_ const ULONG            itagfld,
                            _In_ const ULONG            itagSequence,
                            _In_ const NORM_LOCALE_VER* const   pnlv,
                            _In_ const BOOL             fNormalizedDataToSetIsTruncated );
        ERR             ErrCheckUniqueNormalizedMultiValues(
                            _In_ const FIELD * const    pfield,
                            _In_ const DATA&            dataToSet,
                            _In_ const ULONG            itagfld,
                            _In_ const ULONG            itagSequence,
                            _In_ const NORM_LOCALE_VER* const   pnlv );


    protected:
        const TAGFLD    * Rgtagfld() const                      { return m_rgtagfld; }
        
        const BYTE      * PbTaggedColumns() const               { return (BYTE *)m_rgtagfld; }
        ULONG           CbTaggedColumns() const                 { return m_cbTaggedColumns; }
        ULONG           CTaggedColumns() const                  { return m_cTaggedColumns; }

        ULONG           IbStartOfTaggedData() const;
        const BYTE      * PbStartOfTaggedData() const           { return PbTaggedColumns() + IbStartOfTaggedData(); }
        ULONG           CbTaggedData() const                    { return CbTaggedColumns() - IbStartOfTaggedData(); }

        const TAGFLD    * Ptagfld( const ULONG itagfld ) const;
        INT             TagFldCmp( const TAGFLD * ptagfld1, const TAGFLD * ptagfld2 ) const;
        
        FID             Fid( const ULONG itagfld ) const        { Assert( itagfld < CTaggedColumns() ); return Ptagfld( itagfld )->Fid(); }
        const BYTE      * PbData( const ULONG itagfld ) const;
        ULONG           CbData( const ULONG itagfld ) const;

        const TAGFLD_HEADER * Pheader( const ULONG itagfld ) const;
        ULONG           ItagfldFind( const TAGFLD& tagfldFind ) const;

    public:
        VOID            Migrate(
                            BYTE                * const pbBuffer );
        ERR             ErrSetColumn(
                            FUCB                * const pfucb,
                            const FIELD         * const pfield,
                            const COLUMNID      columnid,
                            const ULONG         itagSequence,
                            const DATA          * const pdataToSet,
                            const JET_GRBIT     grbit );
        ERR             ErrRetrieveColumn(
                            FCB                 * const pfcb,
                            const COLUMNID      columnid,
                            const ULONG         itagSequence,
                            const DATA&         dataRec,
                            DATA                * const pdataRetrieveBuffer,
                            const JET_GRBIT     grbit );
        ULONG           UlColumnInstances(
                            FCB                 * const pfcb,
                            const COLUMNID      columnid,
                            const BOOL          fUseDerivedBit );
        ERR             ErrScan(
                            FUCB                * const pfucb,
                            const ULONG         itagSequence,
                            const DATA&         dataRec,
                            DATA                * const pdataField,
                            COLUMNID            * const pcolumnidRetrieved,
                            ULONG               * const pitagSequenceRetrieved,
                            BOOL                * const pfEncrypted,
                            const JET_GRBIT     grbit );

        VOID            ForceAllColumnsAsDerived();

        ERR             ErrAffectLongValuesInWorkBuf(
                            FUCB                * const pfucb,
                            const LVAFFECT      lvaffect,
                            const ULONG         cbThreshold );

        ERR             ErrDereferenceLongValuesInRecord(
                            FUCB                * const pfucb );
        VOID            CopyTaggedColumns(
                            FUCB                * const pfucbSrc,
                            FUCB                * const pfucbDest,
                            JET_COLUMNID        * const mpcolumnidcolumnidTagged );
        ERR             ErrUpdateSeparatedLongValuesAfterCopy(
                            FUCB                * const pfucbSrc,
                            FUCB                * const pfucbDest,
                            JET_COLUMNID        * const mpcolumnidcolumnidTagged,
                            STATUSINFO          * const pstatus );
        ERR             ErrCheckLongValues(
                            const KEYDATAFLAGS& kdf,
                            RECCHECKTABLE       * const precchecktable );
        BOOL            FIsValidTwoValues(
                            const ULONG         itagfld,
                                  CPRINTF       * const pcprintf ) const;
        BOOL            FIsValidMultiValues(
                            const ULONG         itagfld,
                                  CPRINTF       * const pcprintf ) const;
        BOOL            FValidate(
                                  CPRINTF       * const pcprintf ) const;
        VOID            AssertValid(
                            const TDB           * const ptdb ) const;
        VOID            AssertValidTagColumns(
                            const FUCB          * const pfucb ) const;

    public:
        static BOOL     FIsValidTagfields(
                            const LONG          cbPage,
                            const DATA          &dataRec,
                                  CPRINTF       * const pcprintf );
};


class TAGFLD_ITERATOR
{
    protected:
        TAGFLD_ITERATOR();
        virtual ~TAGFLD_ITERATOR();

    private:
        TAGFLD_ITERATOR( const TAGFLD_ITERATOR& );
        TAGFLD_ITERATOR& operator=( const TAGFLD_ITERATOR& );

    public:
        virtual VOID MoveBeforeFirst();
        virtual VOID MoveAfterLast();

        virtual ERR ErrMovePrev();
        virtual ERR ErrMoveNext();

        virtual INT Ctags() const;
        virtual ERR ErrSetItag( const INT itag );
        
    public:


        virtual INT Itag() const;
        virtual BOOL FSeparated() const;
        virtual BOOL FCompressed() const;
        virtual BOOL FEncrypted() const;
        virtual INT CbData() const;
        virtual const BYTE * PbData() const;
};


INLINE INT CmpFid(
            const FID fid1, const BOOL fDerived1,
            const FID fid2, const BOOL fDerived2 )
{
    Assert( FTaggedFid( fid1 ) );
    Assert( FTaggedFid( fid2 ) );

    INT cmp = 0;
    
    if( fDerived1 && !fDerived2 )
    {
        cmp = 1;
    }
    else if( !fDerived1 && fDerived2 )
    {
        cmp = -1;
    }
    else
    {
        if( fid1 == fid2 )
        {
            cmp = 0;
        }
        else if( fid1 < fid2 )
        {
            cmp = -1;
        }
        else
        {
            cmp = 1;
        }
    }

    return cmp;
}

    
class TAGFIELDS_ITERATOR
{
    public:
        TAGFIELDS_ITERATOR( const DATA& dataRec );
        ~TAGFIELDS_ITERATOR();

    private:
    
        TAGFIELDS_ITERATOR( const TAGFIELDS_ITERATOR& );
        TAGFIELDS_ITERATOR& operator=( const TAGFIELDS_ITERATOR& );

#ifdef DEBUG

    public:

        VOID AssertValid() const;
        
#endif

    public:


        INT CbTaggedColumns() const;
        INT CTaggedColumns() const;
        
    public:

        
        VOID MoveBeforeFirst();
        VOID MoveAfterLast();

        ERR ErrMovePrev();
        ERR ErrMoveNext();

    public:


        FID Fid() const;
        BOOL FNull() const;
        BOOL FLV() const;
        BOOL FDerived() const;
        
        COLUMNID    Columnid( const TDB * const ptdb ) const;
        BOOL        FTemplateColumn( const TDB * const ptdb ) const;

    public:

        
        TAGFLD_ITERATOR& TagfldIterator();
        const TAGFLD_ITERATOR& TagfldIterator() const;

    private:
        VOID CreateTagfldIterator_();
        
    private:
        const TAGFIELDS m_tagfields;

        const TAGFLD * const m_ptagfldMic;
        const TAGFLD * const m_ptagfldMax;

        TAGFLD_ITERATOR * const m_ptagflditerator;
        
        const TAGFLD * m_ptagfldCurr;
        
        BYTE    m_rgbTagfldIteratorBuf[64];
        
};


INLINE TAGFIELDS::TAGFIELDS( const DATA& dataRec )
{
    Assert( NULL != dataRec.Pv() );
    Assert( dataRec.Cb() >= REC::cbRecordMin );
    Assert( dataRec.Cb() <= REC::CbRecordMostWithGlobalPageSize() );

    const REC   * prec                      = (REC *)dataRec.Pv();
    const BYTE  * pbRecMax                  = (BYTE *)prec + dataRec.Cb();
    const BYTE  * pbStartOfTaggedColumns    = prec->PbTaggedData();

    Assert( pbStartOfTaggedColumns <= pbRecMax );
    m_rgtagfld = (TAGFLD *)pbStartOfTaggedColumns;
    m_cbTaggedColumns = ULONG( pbRecMax - pbStartOfTaggedColumns );

    if ( 0 == m_cbTaggedColumns )
    {
        m_cTaggedColumns = 0;
    }
    else
    {
        Assert( m_rgtagfld[0].Ib() >= sizeof(TAGFLD) );
        Assert( m_rgtagfld[0].Ib() <= m_cbTaggedColumns );
        Assert( m_rgtagfld[0].Ib() % sizeof(TAGFLD) == 0 );
        m_cTaggedColumns = m_rgtagfld[0].Ib() / sizeof(TAGFLD);
    }
}

INLINE VOID TAGFIELDS::Refresh( const DATA& dataRec )
{
    Assert( NULL != dataRec.Pv() );
    Assert( dataRec.Cb() >= REC::cbRecordMin );
    Assert( dataRec.Cb() <= REC::CbRecordMostWithGlobalPageSize() );

    Assert( CTaggedColumns() > 0 );
    Assert( CbTaggedColumns() > 0 );
    Assert( (INT)CbTaggedColumns() < dataRec.Cb() );

    const REC   * prec                      = (REC *)dataRec.Pv();
    const BYTE  * pbStartOfTaggedColumns    = prec->PbTaggedData();
    const BYTE  * pbRecMax                  = (BYTE *)prec + dataRec.Cb();

    m_rgtagfld          = (TAGFLD *)pbStartOfTaggedColumns;

    m_cbTaggedColumns   = ULONG( pbRecMax - pbStartOfTaggedColumns );

#ifdef DEBUG
    Assert( Ptagfld( 0 )->Ib() >= sizeof(TAGFLD) );
    Assert( Ptagfld( 0 )->Ib() <= CbTaggedColumns() );

    const BYTE  * const pbTaggedData        = PbTaggedColumns() + Ptagfld( 0 )->Ib();

    Assert( ( pbTaggedData - PbTaggedColumns() ) % sizeof(TAGFLD) == 0 );
    Assert( CTaggedColumns() == ( pbTaggedData - PbTaggedColumns() ) / sizeof(TAGFLD) );
#endif
}

INLINE VOID TAGFIELDS::Migrate( BYTE * const pbBuffer )
{
    UtilMemCpy( pbBuffer, PbTaggedColumns(), CbTaggedColumns() );
    m_rgtagfld = (TAGFLD *)pbBuffer;
}

INLINE TAGFLD * TAGFIELDS::Ptagfld( const ULONG itagfld )
{
    Assert( itagfld < CTaggedColumns() );
    return Rgtagfld() + itagfld;
}
INLINE const TAGFLD * TAGFIELDS::Ptagfld( const ULONG itagfld ) const
{
    Assert( itagfld < CTaggedColumns() );
    return Rgtagfld() + itagfld;
}

INLINE INT TAGFIELDS::TagFldCmp( const TAGFLD * const ptagfld1, const TAGFLD * const ptagfld2 ) const
{
    Assert( ptagfld1 != NULL );
    Assert( ptagfld2 != NULL );
    FID fid2 = ptagfld2->Fid();
    BOOL fDerived2 = ptagfld2->FDerived();
    return ptagfld1->FIsLessThan( fid2, fDerived2 )? -1: ptagfld1->FIsGreaterThan( fid2, fDerived2 )? 1: 0;
}

INLINE BYTE * TAGFIELDS::PbData( const ULONG itagfld )
{
    Assert( CTaggedColumns() > 0 );
    Assert( itagfld < CTaggedColumns() );

    return PbTaggedColumns() + Ptagfld( itagfld )->Ib();
}
INLINE const BYTE * TAGFIELDS::PbData( const ULONG itagfld ) const
{
    Assert( CTaggedColumns() > 0 );
    Assert( itagfld < CTaggedColumns() );

    return PbTaggedColumns() + Ptagfld( itagfld )->Ib();
}

INLINE ULONG TAGFIELDS::CbData( const ULONG itagfld ) const
{
    Assert( CTaggedColumns() > 0 );
    Assert( itagfld < CTaggedColumns() );

    const ULONG     ibStart     = Ptagfld( itagfld )->Ib();
    const ULONG     ibEnd       = ( itagfld < CTaggedColumns() - 1 ?
                                        Ptagfld( itagfld+1 )->Ib() :
                                        CbTaggedColumns() );
    Assert( ibEnd >= ibStart );
    return ( ibEnd - ibStart );
}

INLINE TAGFLD_HEADER * TAGFIELDS::Pheader( const ULONG itagfld )
{
    Assert( itagfld < CTaggedColumns() );
    return (TAGFLD_HEADER *)( Ptagfld( itagfld )->FExtendedInfo() ? PbData( itagfld ) : NULL );
}
INLINE const TAGFLD_HEADER * TAGFIELDS::Pheader( const ULONG itagfld ) const
{
    Assert( itagfld < CTaggedColumns() );
    return (TAGFLD_HEADER *)( Ptagfld( itagfld )->FExtendedInfo() ? PbData( itagfld ) : NULL );
}


INLINE BOOL TAGFLD::FNull( const TAGFIELDS* const ptagfields ) const
{
    if ( FIsSmallPage() || !ptagfields )
    {
        return m_ib & fNullSmallPage;
    }
    
    const TAGFLD_HEADER* const pHeader = ( const TAGFLD_HEADER* )( ptagfields->PbTaggedColumns() + Ib() );
    return pHeader->FNull();
}

INLINE VOID TAGFLD::SetFNull( const TAGFIELDS* const ptagfields )
{
    if ( FIsSmallPage() || !ptagfields )
    {
        m_ib |= WORD( fNullSmallPage );
    }
    else
    {
        TAGFLD_HEADER* const pHeader = ( TAGFLD_HEADER* )( ptagfields->PbTaggedColumns() + Ib() );
        pHeader->SetFNull();
    }
}

INLINE VOID TAGFLD::ResetFNull( const TAGFIELDS* const ptagfields )
{
    if ( FIsSmallPage() || !ptagfields )
    {
        m_ib &= WORD( ~fNullSmallPage );
    }
    else
    {
        TAGFLD_HEADER* const pHeader = ( TAGFLD_HEADER* )( ptagfields->PbTaggedColumns() + Ib() );
        pHeader->ResetFNull();
    }
}


#ifdef DEBUG


INLINE BOOL TAGFLD::CmpTagfldSlow(
    const TAGFLD& tagfld1,
    const TAGFLD& tagfld2 )
{
    if ( tagfld1.FDerived() )
    {
        if ( tagfld2.FDerived() )
        {
            return ( tagfld1.Fid() < tagfld2.Fid() );
        }
        else
        {
            return fTrue;
        }
    }
    else
    {
        if ( tagfld2.FDerived() )
        {
            return fFalse;
        }
        else
        {
            return ( tagfld1.Fid() < tagfld2.Fid() );
        }
    }
}


INLINE BOOL TAGFLD::CmpTagfld1(
    const TAGFLD& tagfld1,
    const TAGFLD& tagfld2 )
{
    const UINT cbitsPerByte = 8;

    Assert( sizeof( DWORD ) >= ( sizeof( tagfld1.m_ib ) + sizeof( FID ) ) );
    const DWORD dw1 = ( ( ~ ( tagfld1.m_ib & TAGFLD::fDerived ) ) << ( cbitsPerByte * sizeof( FID ) ) ) | tagfld1.Fid();
    const DWORD dw2 = ( ( ~ ( tagfld2.m_ib & TAGFLD::fDerived ) ) << ( cbitsPerByte * sizeof( FID ) ) ) | tagfld2.Fid();
    return dw1 < dw2;
}

#endif


INLINE BOOL TAGFLD::CmpTagfld2(
    const TAGFLD& tagfld1,
    const TAGFLD& tagfld2 )
{
    static_assert( 4 == sizeof( DWORD32 ),                                           "Needed for the algorithm." );
    static_assert( sizeof( TAGFLD ) == sizeof( DWORD32 ),                            "TAGFLD must be the same size as DWORD32" );
    static_assert( sizeof(tagfld1.m_fid) == sizeof(tagfld1.m_ib),                    "IB and FID must be the same size" );
    static_assert( 0 == offsetof(TAGFLD, m_fid),                                     "FID must be first in the structure." );
    static_assert( sizeof(tagfld1.m_fid) == offsetof(TAGFLD, m_ib),                  "IB must be directly after FID" );
    static_assert( sizeof(tagfld1.m_fid) + sizeof(tagfld1.m_ib) == sizeof( TAGFLD ), "TAGFLD must hold nothing other than FID and IB" );
    
    const UINT cbitsPerByte = 8;
    
    const DWORD32 dwFlipDerived = DWORD32( TAGFLD::fDerived ) << ( cbitsPerByte * sizeof( tagfld1.m_fid ) );
    
    const DWORD32 dwMask = dwFlipDerived | ( ( 1 << ( cbitsPerByte * sizeof( tagfld1.m_fid ) ) ) - 1 );

    const DWORD32 dw1 = ::FHostIsLittleEndian() ? ( Unaligned< DWORD32 >& ) tagfld1 : _rotl( ( const Unaligned< DWORD32 >& ) tagfld1, cbitsPerByte * sizeof( tagfld1.m_fid ) );
    const DWORD32 dw2 = ::FHostIsLittleEndian() ? ( Unaligned< DWORD32 >& ) tagfld2 : _rotl( ( const Unaligned< DWORD32 >& ) tagfld2, cbitsPerByte * sizeof( tagfld1.m_fid ) );

    Assert( dw1 == ( ( DWORD32( tagfld1.m_ib ) << ( cbitsPerByte * sizeof( tagfld1.m_fid ) ) ) | DWORD32( tagfld1.Fid() ) ) );
    Assert( dw2 == ( ( DWORD32( tagfld2.m_ib ) << ( cbitsPerByte * sizeof( tagfld1.m_fid ) ) ) | DWORD32( tagfld2.Fid() ) ) );

    return ( dwFlipDerived ^ ( dwMask & dw1 ) ) < ( dwFlipDerived ^ ( dwMask & dw2 ) );
}

INLINE BOOL TAGFLD::CmpTagfld(
    const TAGFLD& tagfld1,
    const TAGFLD& tagfld2 )
{
#ifdef DEBUG
    const BOOL fResult1 = CmpTagfld1( tagfld1, tagfld2 );
    const BOOL fResult2 = CmpTagfld2( tagfld1, tagfld2 );
    const BOOL fResultSlow = CmpTagfldSlow( tagfld1, tagfld2 );
    Assert( fResult1 == fResult2 );
    Assert( fResult2 == fResultSlow );
    return fResult1;
#else
    return CmpTagfld2( tagfld1, tagfld2 );
#endif
}

#ifdef DEBUG


INLINE VOID
TAGFLD::CmpTagfldTest()
{
    const BOOL rgfDerived[] = { fFalse, fTrue };
    const TAGOFFSET rgib[]  = { TAGOFFSET( 0 ), TAGOFFSET( maskIb / 2 ), TAGOFFSET( maskIb ) };
    const TAGFID rgfid[]    = { FID( 0 ), FID( 1111 ), FID( fidtypUnknown, (WORD)~0 ) };
    
    for ( INT iDerived1 = 0; iDerived1 < _countof( rgfDerived ); ++iDerived1 )
    {
        const BOOL fDerived1 = rgfDerived[ iDerived1 ];

        for ( INT iIb1 = 0; iIb1 < _countof( rgib ); ++iIb1 )
        {
            const TAGOFFSET ib1 = rgib[ iIb1 ];

            for ( INT ifid1 = 0; ifid1 < _countof( rgfid ); ++ifid1 )
            {
                const TAGFID fid1 = rgfid[ ifid1 ];

                {
                    for ( INT iDerived2 = 0; iDerived2 < _countof( rgfDerived ); ++iDerived2 )
                    {
                        const BOOL fDerived2 = rgfDerived[ iDerived2 ];

                        for ( INT iIb2 = 0; iIb2 < _countof( rgib ); ++iIb2 )
                        {
                            const TAGOFFSET ib2 = rgib[ iIb2 ];

                            for ( INT ifid2 = 0; ifid2 < _countof( rgfid ); ++ifid2 )
                            {
                                const TAGFID fid2 = rgfid[ ifid2 ];

                                TAGFLD  tagfld1( fid1, fDerived1 );
                                tagfld1.SetIb( ib1 );
                                TAGFLD  tagfld2( fid2, fDerived2 );
                                tagfld2.SetIb( ib2 );

                                (VOID) CmpTagfld( tagfld1, tagfld2 );
                            }
                        }
                    }
                }
            }
        }
    }
}

#endif

INLINE const TAGFLD*
TAGFIELDS::PtagfldLowerBound
    (
    const TAGFLD* const ptagfldStart,
    const TAGFLD* const ptagfldMax,
    const TAGFLD& tagfldFind
    )
{
    Assert( pNil != ptagfldStart );
    Assert( pNil != ptagfldMax );
    Assert( pNil != &tagfldFind );
    Assert( ptagfldMax >= ptagfldStart );

#ifdef DEBUG
    {
        static BOOL fTested = fFalse;

        if ( ! fTested )
        {
            TAGFLD::CmpTagfldTest();
            
            fTested = fTrue;
        }
    }
#endif
    
    SIZE_T  cfldSeparation = ptagfldMax - ptagfldStart;
    const TAGFLD* ptagfld = ptagfldStart;
    while ( cfldSeparation > 0 )
    {
        const SIZE_T cfldHalf = cfldSeparation / 2;
        const TAGFLD* const ptagfldMid = ptagfld + cfldHalf;
        if ( TAGFLD::CmpTagfld( *ptagfldMid, tagfldFind ) )
        {
            ptagfld = ptagfldMid + 1;
            cfldSeparation -= cfldHalf + 1;
        }
        else
        {
            cfldSeparation = cfldHalf;
        }
    }
    return ptagfld;
}

INLINE ULONG TAGFIELDS::ItagfldFind( const TAGFLD& tagfldFind ) const
{
    const TAGFLD    *       ptagfldStart    = Rgtagfld();
    const TAGFLD    *       ptagfldMax      = Rgtagfld() + CTaggedColumns();
    const TAGFLD    * const ptagfld         = PtagfldLowerBound(
                                                    ptagfldStart,
                                                    ptagfldMax,
                                                    tagfldFind
                                                    );
    Assert( (BYTE *)ptagfld >= (BYTE *)ptagfldStart );
    Assert( (BYTE *)ptagfld <= (BYTE *)ptagfldMax );
    Assert( ( (BYTE *)ptagfld - (BYTE *)ptagfldStart ) % sizeof(TAGFLD) == 0 );
    return ULONG( ptagfld - ptagfldStart );
}

INLINE ULONG TAGFIELDS::IbStartOfTaggedData() const
{
    Assert( CTaggedColumns() > 0 );
    Assert( Ptagfld( 0 )->Ib() >= sizeof(TAGFLD) );
    Assert( Ptagfld( 0 )->Ib() <= CbTaggedColumns() );
    return Ptagfld( 0 )->Ib();
}

INLINE VOID TAGFIELDS::ForceAllColumnsAsDerived()
{
    for ( ULONG itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
    {
        Assert( !Ptagfld( itagfld )->FDerived() );
        Ptagfld( itagfld )->SetFDerived();
    }
}

INLINE VOID TAGFIELDS::AssertValid( const TDB * const ptdb ) const
{
#ifdef DEBUG
    Assert( ptdbNil != ptdb );
    for ( ULONG itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
    {
        const FID   fid     = Ptagfld( itagfld )->Fid();
        Assert( FTaggedFid( fid ) );

        if ( Ptagfld( itagfld )->FDerived() )
        {
            if ( ptdb->FTemplateTable() )
            {
                ptdb->AssertValidTemplateTable();
                Assert( fid <= ptdb->FidTaggedLast() );
            }
            else
            {
                ptdb->AssertValidDerivedTable();
                Assert( fid <= ptdb->PfcbTemplateTable()->Ptdb()->FidTaggedLast() );
            }
        }
        else if ( !ptdb->FTemplateTable() )
        {
            Assert( fid <= ptdb->FidTaggedLast() );
        }
    }
    Assert( FValidate( CPRINTFNULL::PcprintfInstance() ) );
#endif
}

INLINE VOID TAGFIELDS::AssertValidTagColumns( const FUCB* const pfucb ) const
{
#ifdef DEBUG
    Assert( pfucbNil != pfucb );

    for ( ULONG itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
    {
        const TAGFLD* const ptagfld = Ptagfld( itagfld );
        Assert( FIsSmallPage() || ptagfld->FExtendedInfo() );
        
        if ( ptagfld->FExtendedInfo() )
        {
            const FID   fid = ptagfld->Fid();
            const TDB* ptdb = pfucb->u.pfcb->Ptdb();
            const COLUMNID columnid = ColumnidOfFid( fid, ptagfld->FDerived() || ptdb->FTemplateTable() );

            if ( FCOLUMNIDTemplateColumn( columnid ) && !ptdb->FTemplateTable() )
            {
                ptdb = ptdb->PfcbTemplateTable()->Ptdb();
            }

            const FIELD* const pfield = ptdb->PfieldTagged( columnid );
            const TAGFLD_HEADER* const pheader = Pheader( itagfld );
            Assert( pheader );
            Assert( !( pheader->FCompressed() && pheader->FSeparated() ) );
            Assert( JET_coltypNil == pfield->coltyp || !!FRECLongValue( pfield->coltyp ) == !!pheader->FLongValue() );
        }
    }
#endif
}


PERSISTED
class TAGFLD_OLD
{
    public:

        TAGFLD_OLD( const FID fid, const BOOL fLongValue, const BOOL fDerived );
        ~TAGFLD_OLD();

    private:
        union BITFIELDS
        {
            WORD        m_w;
            struct {
                WORD    m_cbData:12;
                WORD    reserved:1;
                WORD    m_fDerived:1;
                WORD    m_fNull:1;
                WORD    m_fLongValue:1;
            };
        };
        
        UnalignedLittleEndian< FID >    m_fid;
        UnalignedLittleEndian< WORD >   m_wBitFields;
        BYTE                            m_rgb[];


    public:
    
        FID Fid() const;
        ULONG CbData() const;
        BOOL FNull() const;
        BOOL FLongValue() const;
        BOOL FDerived() const;
        BYTE *Rgb();
        const BYTE *Rgb() const;

        VOID SetFid( const FID fid );
        VOID SetCbData( const ULONG cbData );
        VOID SetNull();
        VOID ResetNull();

        VOID UpgradeToESE98DerivedColumn();
        
        TAGFLD_OLD* PtagfldNext() const;
        BOOL FLastTaggedInstance( const BYTE *pbRecMax ) const;

        BOOL FTemplateColumn( const TDB * const ptdb ) const;
        COLUMNID Columnid( const TDB * const ptdb ) const;

        BOOL FIsEqual( const FID fid, const BOOL fDerived ) const;
        BOOL FIsLessThan( const FID fid, const BOOL fDerived ) const;
        BOOL FIsLessThanOrEqual( const FID fid, const BOOL fDerived ) const;
        BOOL FIsGreaterThan( const FID fid, const BOOL fDerived ) const;
        BOOL FIsGreaterThanOrEqual( const FID fid, const BOOL fDerived ) const;

        BOOL FIsEqual( const COLUMNID columnid, const TDB * const ptdb  ) const;
        BOOL FIsLessThan( const COLUMNID columnid, const TDB * const ptdb  ) const;
        BOOL FIsLessThanOrEqual( const COLUMNID columnid, const TDB * const ptdb  ) const;
        BOOL FIsGreaterThan( const COLUMNID columnid, const TDB * const ptdb  ) const;
        BOOL FIsGreaterThanOrEqual( const COLUMNID columnid, const TDB * const ptdb  ) const;

        VOID AssertValid( const TDB * const ptdb ) const;
};


INLINE TAGFLD_OLD::TAGFLD_OLD( const FID fid, const BOOL fLongValue, const BOOL fDerived ) :
    m_fid( fid )
{
    Assert( FTaggedFid( fid ) );
    
    if ( fLongValue || fDerived )
    {
        BITFIELDS btflds;
        btflds.m_w = 0;
        btflds.m_fLongValue = WORD( fLongValue ? fTrue : fFalse );
        btflds.m_fDerived = WORD( fDerived ? fTrue : fFalse );
        m_wBitFields = btflds.m_w;
    }
    else
        m_wBitFields = 0;
}

INLINE TAGFLD_OLD::~TAGFLD_OLD()
{
}

INLINE FID TAGFLD_OLD::Fid() const
{
    Assert( FTaggedFid( m_fid ) );
    return m_fid;
}

INLINE ULONG TAGFLD_OLD::CbData() const
{
    BITFIELDS btflds;
    btflds.m_w = m_wBitFields;
    Assert( btflds.m_cbData <= cbLVIntrinsicMost + 1 );
    return btflds.m_cbData;
}

INLINE BOOL TAGFLD_OLD::FNull() const
{
    BITFIELDS btflds;
    btflds.m_w = m_wBitFields;
    return btflds.m_fNull;
}

INLINE BOOL TAGFLD_OLD::FLongValue() const
{
    BITFIELDS btflds;
    btflds.m_w = m_wBitFields;
    return btflds.m_fLongValue;
}

INLINE BOOL TAGFLD_OLD::FDerived() const
{
    BITFIELDS btflds;
    btflds.m_w = ReverseBytesOnBE( m_wBitFields );
    return btflds.m_fDerived;
}

INLINE BYTE *TAGFLD_OLD::Rgb()
{
    return (BYTE *)m_rgb;
}

INLINE const BYTE *TAGFLD_OLD::Rgb() const
{
    return (BYTE *)m_rgb;
}

INLINE VOID TAGFLD_OLD::SetFid( const FID fid )
{
    Assert( FTaggedFid( m_fid ) );
    m_fid = fid;
}

INLINE VOID TAGFLD_OLD::SetCbData( const ULONG cbData )
{
    Assert( cbData <= cbLVIntrinsicMost );
    BITFIELDS btflds;
    btflds.m_w = m_wBitFields;
    btflds.m_cbData = (USHORT)cbData;
    m_wBitFields = btflds.m_w;
}

INLINE VOID TAGFLD_OLD::SetNull()
{
    BITFIELDS btflds;
    btflds.m_w = m_wBitFields;
    btflds.m_fNull = fTrue;
    m_wBitFields = btflds.m_w;
}

INLINE VOID TAGFLD_OLD::ResetNull()
{
    BITFIELDS btflds;
    btflds.m_w = m_wBitFields;
    btflds.m_fNull = fFalse;
    m_wBitFields = btflds.m_w;
}

INLINE VOID TAGFLD_OLD::UpgradeToESE98DerivedColumn()
{
    BITFIELDS btflds;
    btflds.m_w = m_wBitFields;
    btflds.m_fDerived = fTrue;
    m_wBitFields = btflds.m_w;
}

INLINE TAGFLD_OLD* TAGFLD_OLD::PtagfldNext() const
{
    return (TAGFLD_OLD*)( (BYTE *)( this + 1 ) + CbData() );
}
    
INLINE BOOL TAGFLD_OLD::FLastTaggedInstance( const BYTE *pbRecMax ) const
{
    BOOL    fLastInstance   = fTrue;
    TAGFLD_OLD  *ptagfldNext    = PtagfldNext();

    Assert( (BYTE *)ptagfldNext <= pbRecMax );
    if ( (BYTE *)ptagfldNext < pbRecMax )
    {
        if ( !FDerived() || ptagfldNext->FDerived() )
        {
            Assert( FDerived() || !ptagfldNext->FDerived() );
            Assert( ptagfldNext->Fid() >= Fid() );
            fLastInstance = ( ptagfldNext->Fid() > Fid() );
        }
    }
    
    return fLastInstance;
}

INLINE BOOL TAGFLD_OLD::FTemplateColumn( const TDB * const ptdb ) const
{
    return ( FDerived()
            || ptdb->FTemplateTable()
            || ( ptdb->FESE97DerivedTable() && Fid() < ptdb->FidTaggedFirst() ) );
}

INLINE COLUMNID TAGFLD_OLD::Columnid( const TDB * const ptdb ) const
{
    return ColumnidOfFid( Fid(), FTemplateColumn( ptdb ) );
}

INLINE BOOL TAGFLD_OLD::FIsEqual( const FID fid, const BOOL fDerived ) const
{
    Assert( FTaggedFid( fid ) );
    return ( Fid() == fid
        && ( ( FDerived() && fDerived ) || ( !FDerived() && !fDerived ) ) );
}
INLINE BOOL TAGFLD_OLD::FIsLessThan( const FID fid, const BOOL fDerived ) const
{
    Assert( FTaggedFid( fid ) );
    if ( FDerived() )
    {
        if ( fDerived )
        {
            return ( Fid() < fid );
        }
        else
        {
            return fTrue;
        }
    }
    else
    {
        if ( fDerived )
        {
            return fFalse;
        }
        else
        {
            return ( Fid() < fid );
        }
    }
}
INLINE BOOL TAGFLD_OLD::FIsLessThanOrEqual( const FID fid, const BOOL fDerived ) const
{
    Assert( FTaggedFid( fid ) );
    if ( FDerived() )
    {
        if ( fDerived )
        {
            return ( Fid() <= fid );
        }
        else
        {
            return fTrue;
        }
    }
    else
    {
        if ( fDerived )
        {
            return fFalse;
        }
        else
        {
            return ( Fid() <= fid );
        }
    }
}
INLINE BOOL TAGFLD_OLD::FIsGreaterThan( const FID fid, const BOOL fDerived ) const
{
    Assert( FTaggedFid( fid ) );
    if ( FDerived() )
    {
        if ( fDerived )
        {
            return ( Fid() > fid );
        }
        else
        {
            return fFalse;
        }
    }
    else
    {
        if ( fDerived )
        {
            return fTrue;
        }
        else
        {
            return ( Fid() > fid );
        }
    }
}
INLINE BOOL TAGFLD_OLD::FIsGreaterThanOrEqual( const FID fid, const BOOL fDerived ) const
{
    Assert( FTaggedFid( fid ) );
    if ( FDerived() )
    {
        if ( fDerived )
        {
            return ( Fid() >= fid );
        }
        else
        {
            return fFalse;
        }
    }
    else
    {
        if ( fDerived )
        {
            return fTrue;
        }
        else
        {
            return ( Fid() >= fid );
        }
    }
}

INLINE BOOL TAGFLD_OLD::FIsEqual( const COLUMNID columnid, const TDB * const ptdb  ) const
{
    return FIsEqual( FidOfColumnid( columnid ), FRECUseDerivedBit( columnid, ptdb ) );
}
INLINE BOOL TAGFLD_OLD::FIsLessThan( const COLUMNID columnid, const TDB * const ptdb  ) const
{
    return FIsLessThan( FidOfColumnid( columnid ), FRECUseDerivedBit( columnid, ptdb ) );
}
INLINE BOOL TAGFLD_OLD::FIsLessThanOrEqual( const COLUMNID columnid, const TDB * const ptdb  ) const
{
    return FIsLessThanOrEqual( FidOfColumnid( columnid ), FRECUseDerivedBit( columnid, ptdb ) );
}
INLINE BOOL TAGFLD_OLD::FIsGreaterThan( const COLUMNID columnid, const TDB * const ptdb  ) const
{
    return FIsGreaterThan( FidOfColumnid( columnid ), FRECUseDerivedBit( columnid, ptdb ) );
}
INLINE BOOL TAGFLD_OLD::FIsGreaterThanOrEqual( const COLUMNID columnid, const TDB * const ptdb  ) const
{
    return FIsGreaterThanOrEqual( FidOfColumnid( columnid ), FRECUseDerivedBit( columnid, ptdb ) );
}

INLINE VOID TAGFLD_OLD::AssertValid( const TDB * const ptdb ) const
{
#ifdef DEBUG
    Assert( FTaggedFid( Fid() ) );
    if ( FDerived() )
    {
        if ( ptdb->FTemplateTable() )
        {
            ptdb->AssertValidTemplateTable();
            Assert( Fid() <= ptdb->FidTaggedLast() );
        }
        else
        {
            ptdb->AssertValidDerivedTable();
            Assert( Fid() <= ptdb->PfcbTemplateTable()->Ptdb()->FidTaggedLast() );
        }
    }
    else if ( !ptdb->FTemplateTable() )
    {
        Assert( Fid() <= ptdb->FidTaggedLast() );
    }
#endif
}

