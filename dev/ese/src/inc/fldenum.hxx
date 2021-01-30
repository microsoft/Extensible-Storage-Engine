// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


class IColumnIter
{
    public:

        
        virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const = 0;


        virtual ERR ErrSetRecord( const DATA& dataRec ) = 0;

        
        virtual ERR ErrMoveBeforeFirst() = 0;
        virtual ERR ErrMoveNext() = 0;
        virtual ERR ErrSeek( const COLUMNID columnid ) = 0;


        virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const = 0;
        virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const = 0;
        virtual ERR ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const = 0;


        virtual ERR ErrGetColumnValue(  const size_t    iColumnValue,
                                        size_t* const   pcbColumnValue,
                                        void** const    ppvColumnValue,
                                        BOOL* const     pfColumnValueSeparated,
                                        BOOL* const     pfColumnValueCompressed ) const = 0;

        virtual const FIELD* const PField() const = 0;
};



class CFixedColumnIter
    :   public IColumnIter
{
    public:


        CFixedColumnIter();


        ERR ErrInit( FCB* const pfcb );

    public:

        
        virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const;


        virtual ERR ErrSetRecord( const DATA& dataRec );

        
        virtual ERR ErrMoveBeforeFirst();
        virtual ERR ErrMoveNext();
        virtual ERR ErrSeek( const COLUMNID columnid );


        virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const;
        virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;
        virtual ERR ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const;


        virtual ERR ErrGetColumnValue(  const size_t    iColumnValue,
                                        size_t* const   pcbColumnValue,
                                        void** const    ppvColumnValue,
                                        BOOL* const     pfColumnValueSeparated,
                                        BOOL* const     pfColumnValueCompressed ) const;

        virtual const FIELD* const PField() const
        {
            return &m_fieldCurr;
        }

    private:

        FCB*            m_pfcb;
        const REC*      m_prec;
        COLUMNID        m_columnidCurr;
        ERR             m_errCurr;
        FIELD           m_fieldCurr;
};

INLINE CFixedColumnIter::
CFixedColumnIter()
    :   m_pfcb( pfcbNil ),
        m_prec( NULL ),
        m_errCurr( errRECNoCurrentColumnValue )
{
}

INLINE ERR CFixedColumnIter::
ErrInit( FCB* const pfcb )
{
    ERR err = JET_errSuccess;

    if ( !pfcb )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    m_pfcb  = pfcb;
    m_prec  = NULL;

    Call( ErrMoveBeforeFirst() );

HandleError:
    return err;
}

INLINE ERR CFixedColumnIter::
ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const
{
    ERR err = JET_errSuccess;

    if ( !m_prec )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }
    
    *pcColumn = m_prec->FidFixedLastInRec() - fidFixedLeast + 1;

HandleError:
    return err;
}

INLINE ERR CFixedColumnIter::
ErrSetRecord( const DATA& dataRec )
{
    ERR err = JET_errSuccess;

    if ( !dataRec.Pv() )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    m_prec = (REC*)dataRec.Pv();

    Call( ErrMoveBeforeFirst() );

HandleError:
    return err;
}

INLINE ERR CFixedColumnIter::
ErrMoveBeforeFirst()
{
    m_columnidCurr  = fidFixedLeast - 1;
    m_errCurr       = errRECNoCurrentColumnValue;
    memset( &m_fieldCurr, 0, sizeof( m_fieldCurr ) );
    return JET_errSuccess;
}

INLINE ERR CFixedColumnIter::
ErrGetColumnId( COLUMNID* const pColumnId ) const
{
    ERR err = JET_errSuccess;

    Call( m_errCurr );

    *pColumnId = m_columnidCurr;

HandleError:
    return err;
}



class CVariableColumnIter
    :   public IColumnIter
{
    public:


        CVariableColumnIter();


        ERR ErrInit( FCB* const pfcb );

    public:

        
        virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const;


        virtual ERR ErrSetRecord( const DATA& dataRec );

        
        virtual ERR ErrMoveBeforeFirst();
        virtual ERR ErrMoveNext();
        virtual ERR ErrSeek( const COLUMNID columnid );


        virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const;
        virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;
        virtual ERR ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const;


        virtual ERR ErrGetColumnValue(  const size_t    iColumnValue,
                                        size_t* const   pcbColumnValue,
                                        void** const    ppvColumnValue,
                                        BOOL* const     pfColumnValueSeparated,
                                        BOOL* const     pfColumnValueCompressed ) const;

        virtual const FIELD* const PField() const
        {
            EnforceSz( fFalse, "FeatureNAVariableColumnIterPField" );
            return NULL;
        }

    private:

        FCB*            m_pfcb;
        const REC*      m_prec;
        COLUMNID        m_columnidCurr;
        ERR             m_errCurr;
};

INLINE CVariableColumnIter::
CVariableColumnIter()
    :   m_pfcb( pfcbNil ),
        m_prec( NULL ),
        m_errCurr( errRECNoCurrentColumnValue )
{
}

INLINE ERR CVariableColumnIter::
ErrInit( FCB* const pfcb )
{
    ERR err = JET_errSuccess;

    if ( !pfcb )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    m_pfcb  = pfcb;
    m_prec  = NULL;

    Call( ErrMoveBeforeFirst() );

HandleError:
    return err;
}

INLINE ERR CVariableColumnIter::
ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const
{
    ERR err = JET_errSuccess;

    if ( !m_prec )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }
    
    *pcColumn = m_prec->FidVarLastInRec() - fidVarLeast + 1;

HandleError:
    return err;
}

INLINE ERR CVariableColumnIter::
ErrSetRecord( const DATA& dataRec )
{
    ERR err = JET_errSuccess;

    if ( !dataRec.Pv() )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    m_prec = (REC*)dataRec.Pv();

    Call( ErrMoveBeforeFirst() );

HandleError:
    return err;
}

INLINE ERR CVariableColumnIter::
ErrMoveBeforeFirst()
{
    m_columnidCurr  = fidVarLeast - 1;
    m_errCurr       = errRECNoCurrentColumnValue;
    return JET_errSuccess;
}

INLINE ERR CVariableColumnIter::
ErrGetColumnId( COLUMNID* const pColumnId ) const
{
    ERR err = JET_errSuccess;

    Call( m_errCurr );

    *pColumnId = m_columnidCurr;

HandleError:
    return err;
}



class IColumnValueIter
{
    public:


        virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const = 0;
        virtual ERR ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const = 0;


        virtual ERR ErrGetColumnValue(  const size_t    iColumnValue,
                                        size_t* const   pcbColumnValue,
                                        void** const    ppvColumnValue,
                                        BOOL* const     pfColumnValueSeparated,
                                        BOOL* const     pfColumnValueCompressed ) const = 0;
        virtual size_t CbESE97Format() const = 0;
};



class CNullValuedTaggedColumnValueIter
    :   public IColumnValueIter
{
    public:


        CNullValuedTaggedColumnValueIter();


        ERR ErrInit();

    public:


        virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;
        virtual ERR ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const;


        virtual ERR ErrGetColumnValue(  const size_t    iColumnValue,
                                        size_t* const   pcbColumnValue,
                                        void** const    ppvColumnValue,
                                        BOOL* const     pfColumnValueSeparated,
                                        BOOL* const     pfColumnValueCompressed ) const;
        virtual size_t CbESE97Format() const;
};

INLINE CNullValuedTaggedColumnValueIter::
CNullValuedTaggedColumnValueIter()
{
}

INLINE ERR CNullValuedTaggedColumnValueIter::
ErrInit()
{
    return JET_errSuccess;
}

INLINE ERR CNullValuedTaggedColumnValueIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
{
    *pcColumnValue = 0;

    return JET_errSuccess;
}

INLINE ERR CNullValuedTaggedColumnValueIter::
ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const
{
    precsize->cbOverhead += sizeof(TAGFLD);
    precsize->cTaggedColumns++;
    return JET_errSuccess;
}

INLINE ERR CNullValuedTaggedColumnValueIter::
ErrGetColumnValue(  const size_t    iColumnValue,
                    size_t* const   pcbColumnValue,
                    void** const    ppvColumnValue,
                    BOOL* const     pfColumnValueSeparated,
                    BOOL* const     pfColumnValueCompressed ) const
{
    return ErrERRCheck( JET_errBadItagSequence );
}

INLINE size_t CNullValuedTaggedColumnValueIter::
CbESE97Format() const
{
    return sizeof(TAGFLD);
}



class CSingleValuedTaggedColumnValueIter
    :   public IColumnValueIter
{
    public:


        CSingleValuedTaggedColumnValueIter();


        ERR ErrInit( BYTE* const rgbData, size_t cbData, const BOOL fSeparatable, const BOOL fSeparated, const BOOL fCompressed, const BOOL fEncrypted );

    public:


        virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;
        virtual ERR ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const;


        virtual ERR ErrGetColumnValue(  const size_t    iColumnValue,
                                        size_t* const   pcbColumnValue,
                                        void** const    ppvColumnValue,
                                        BOOL* const     pfColumnValueSeparated,
                                        BOOL* const     pfColumnValueCompressed ) const;
        virtual size_t CbESE97Format() const;

    private:

        BYTE*           m_rgbData;
        size_t          m_cbData;
        union
        {
            BOOL        m_fFlags;
            struct
            {
                BOOL    m_fSeparatable:1;
                BOOL    m_fSeparated:1;
                BOOL    m_fCompressed:1;
                BOOL    m_fEncrypted:1;
            };
        };
};

INLINE CSingleValuedTaggedColumnValueIter::
CSingleValuedTaggedColumnValueIter()
    :   m_rgbData( NULL )
{
}

INLINE ERR CSingleValuedTaggedColumnValueIter::
ErrInit( BYTE* const rgbData, size_t cbData, const BOOL fSeparatable, const BOOL fSeparated, const BOOL fCompressed, const BOOL fEncrypted )
{
    ERR err = JET_errSuccess;

    if ( !rgbData )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    m_rgbData       = rgbData;
    m_cbData        = cbData;
    m_fFlags        = 0;

    Assert( !fSeparated || fSeparatable );
    m_fSeparatable  = !!fSeparatable;
    m_fSeparated    = !!fSeparated;
    m_fCompressed   = !!fCompressed;
    m_fEncrypted    = !!fEncrypted;

HandleError:
    return err;
}

INLINE ERR CSingleValuedTaggedColumnValueIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
{
    ERR err = JET_errSuccess;

    if ( !m_rgbData )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    *pcColumnValue = 1;

HandleError:
    return err;
}

INLINE size_t CSingleValuedTaggedColumnValueIter::
CbESE97Format() const
{
    size_t  cbESE97Format   = sizeof(TAGFLD);

    if ( m_fSeparatable )
    {
        cbESE97Format += sizeof(BYTE) + min( m_cbData, sizeof(_LID32) );
    }
    else
    {
        cbESE97Format += m_cbData;
    }

    return cbESE97Format;
}



class CDualValuedTaggedColumnValueIter
    :   public IColumnValueIter
{
    public:


        CDualValuedTaggedColumnValueIter();


        ERR ErrInit( BYTE* const rgbData, size_t cbData );

    public:


        virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;
        virtual ERR ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const;


        virtual ERR ErrGetColumnValue(  const size_t    iColumnValue,
                                        size_t* const   pcbColumnValue,
                                        void** const    ppvColumnValue,
                                        BOOL* const     pfColumnValueSeparated,
                                        BOOL* const     pfColumnValueCompressed ) const;
        virtual size_t CbESE97Format() const;

    private:

        TWOVALUES*      m_ptwovalues;

        BYTE            m_rgbTWOVALUES[ sizeof( TWOVALUES ) ];
};

INLINE CDualValuedTaggedColumnValueIter::
CDualValuedTaggedColumnValueIter()
    :   m_ptwovalues( NULL )
{
}

INLINE ERR CDualValuedTaggedColumnValueIter::
ErrInit( BYTE* const rgbData, size_t cbData )
{
    ERR err = JET_errSuccess;

    if ( !rgbData )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Assert( cbData <= ulMax );
    if ( cbData > ulMax )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    m_ptwovalues = new( m_rgbTWOVALUES ) TWOVALUES( rgbData, (ULONG) cbData );

HandleError:
    return err;
}

INLINE ERR CDualValuedTaggedColumnValueIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
{
    ERR err = JET_errSuccess;

    if ( !m_ptwovalues )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    *pcColumnValue = 2;

HandleError:
    return err;
}

INLINE size_t CDualValuedTaggedColumnValueIter::
CbESE97Format() const
{
    return ( sizeof(TAGFLD)
            + m_ptwovalues->CbFirstValue()
            + sizeof(TAGFLD)
            + m_ptwovalues->CbSecondValue() );
}



class CMultiValuedTaggedColumnValueIter
    :   public IColumnValueIter
{
    public:


        CMultiValuedTaggedColumnValueIter();


        ERR ErrInit( BYTE* const rgbData, size_t cbData, BOOL fCompressed  );

    public:


        virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;
        virtual ERR ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const;


        virtual ERR ErrGetColumnValue(  const size_t    iColumnValue,
                                        size_t* const   pcbColumnValue,
                                        void** const    ppvColumnValue,
                                        BOOL* const     pfColumnValueSeparated,
                                        BOOL* const     pfColumnValueCompressed ) const;
        virtual size_t CbESE97Format() const;

    private:

        BOOL            m_fCompressed;
        MULTIVALUES*    m_pmultivalues;

        BYTE            m_rgbMULTIVALUES[ sizeof( MULTIVALUES ) ];
};

INLINE CMultiValuedTaggedColumnValueIter::
CMultiValuedTaggedColumnValueIter()
    :   m_pmultivalues( NULL ),
        m_fCompressed( false )
{
}

INLINE ERR CMultiValuedTaggedColumnValueIter::
ErrInit( BYTE* const rgbData, size_t cbData, BOOL fCompressed )
{
    ERR err = JET_errSuccess;

    if ( !rgbData )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Assert( cbData <= ulMax );
    if ( cbData > ulMax )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    C_ASSERT( 0 == offsetof( CMultiValuedTaggedColumnValueIter, m_rgbMULTIVALUES ) % sizeof( void* ) );
    m_pmultivalues = new( m_rgbMULTIVALUES ) MULTIVALUES( rgbData, (ULONG) cbData );
    m_fCompressed = fCompressed;

HandleError:
    return err;
}

INLINE ERR CMultiValuedTaggedColumnValueIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
{
    ERR err = JET_errSuccess;

    if ( !m_pmultivalues )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    *pcColumnValue = m_pmultivalues->CMultiValues();

HandleError:
    return err;
}




#define SIZEOF_CVITER_MAX                                           \
    max(    max(    sizeof( CNullValuedTaggedColumnValueIter ),     \
                    sizeof( CSingleValuedTaggedColumnValueIter ) ), \
            max(    sizeof( CDualValuedTaggedColumnValueIter ),     \
                    sizeof( CMultiValuedTaggedColumnValueIter ) ) )

class CTaggedColumnIter
    :   public IColumnIter
{
    public:


        CTaggedColumnIter();


        ERR ErrInit( FCB* const pfcb );

    public:

        
        virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const;


        virtual ERR ErrSetRecord( const DATA& dataRec );

        
        virtual ERR ErrMoveBeforeFirst();
        virtual ERR ErrMoveNext();
        virtual ERR ErrSeek( const COLUMNID columnid );


        virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const;
        virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;
        virtual ERR ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const;


        virtual ERR ErrGetColumnValue(  const size_t    iColumnValue,
                                        size_t* const   pcbColumnValue,
                                        void** const    ppvColumnValue,
                                        BOOL* const     pfColumnValueSeparated,
                                        BOOL* const     pfColumnValueCompressed ) const;

        virtual const FIELD* const PField() const
        {
            EnforceSz( fFalse, "FeatureNATaggedColumnIterPField" );
            return NULL;
        }

        ERR ErrCalcCbESE97Format( size_t* const pcbESE97Format ) const;

    private:

        ERR ErrCreateCVIter( IColumnValueIter** const ppcviter );
        
    private:

        FCB*                m_pfcb;
        TAGFIELDS*          m_ptagfields;
        TAGFLD*             m_ptagfldCurr;
        ERR                 m_errCurr;
        IColumnValueIter*   m_pcviterCurr;

        BYTE                m_rgbTAGFIELDS[ sizeof( TAGFIELDS ) ];
        BYTE                m_rgbCVITER[ SIZEOF_CVITER_MAX ];
};

INLINE CTaggedColumnIter::
CTaggedColumnIter()
    :   m_pfcb( pfcbNil ),
        m_ptagfields( NULL ),
        m_errCurr( errRECNoCurrentColumnValue )
{
}

INLINE ERR CTaggedColumnIter::
ErrInit( FCB* const pfcb )
{
    ERR err = JET_errSuccess;

    if ( !pfcb )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    m_pfcb          = pfcb;
    m_ptagfields    = NULL;

    Call( ErrMoveBeforeFirst() );

HandleError:
    return err;
}

INLINE ERR CTaggedColumnIter::
ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const
{
    ERR err = JET_errSuccess;

    if ( !m_ptagfields )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }
    
    *pcColumn = m_ptagfields->CTaggedColumns();

HandleError:
    return err;
}

INLINE ERR CTaggedColumnIter::
ErrSetRecord( const DATA& dataRec )
{
    ERR err = JET_errSuccess;

    if ( !dataRec.Pv() )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    m_ptagfields = new( m_rgbTAGFIELDS ) TAGFIELDS( dataRec );

    Call( ErrMoveBeforeFirst() );

HandleError:
    return err;
}

INLINE ERR CTaggedColumnIter::
ErrMoveBeforeFirst()
{
    m_ptagfldCurr   = m_ptagfields ? m_ptagfields->Rgtagfld() - 1 : NULL;
    m_errCurr       = errRECNoCurrentColumnValue;
    return JET_errSuccess;
}

INLINE ERR CTaggedColumnIter::
ErrGetColumnId( COLUMNID* const pColumnId ) const
{
    ERR err = JET_errSuccess;

    Call( m_errCurr );

    *pColumnId = m_ptagfldCurr->Columnid( m_pfcb->Ptdb() );

HandleError:
    return err;
}

INLINE ERR CTaggedColumnIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
{
    ERR err = JET_errSuccess;

    Call( m_errCurr );

    Call( m_pcviterCurr->ErrGetColumnValueCount( pcColumnValue ) );

HandleError:
    return err;
}

INLINE ERR CTaggedColumnIter::
ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const
{
    ERR err = JET_errSuccess;

    Call( m_errCurr );

    Call( m_pcviterCurr->ErrGetColumnSize( pfucb, precsize, grbit ) );

HandleError:
    return err;
}

INLINE ERR CTaggedColumnIter::
ErrGetColumnValue(  const size_t    iColumnValue,
                    size_t* const   pcbColumnValue,
                    void** const    ppvColumnValue,
                    BOOL* const     pfColumnValueSeparated,
                    BOOL* const     pfColumnValueCompressed ) const
{
    ERR err = JET_errSuccess;

    Call( m_errCurr );

    Call( m_pcviterCurr->ErrGetColumnValue( iColumnValue,
                                            pcbColumnValue,
                                            ppvColumnValue,
                                            pfColumnValueSeparated,
                                            pfColumnValueCompressed ) );

HandleError:
    return err;
}

INLINE ERR CTaggedColumnIter::
ErrCalcCbESE97Format( size_t* const pcbESE97Format ) const
{
    ERR err = JET_errSuccess;

    Call( m_errCurr );

    *pcbESE97Format = m_pcviterCurr->CbESE97Format();

HandleError:
    return err;
}



class CUnionIter
    :   public IColumnIter
{
    public:


        CUnionIter();


        ERR ErrInit( IColumnIter* const pciterLHS, IColumnIter* const pciterRHS );

    public:

        
        virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const;


        virtual ERR ErrSetRecord( const DATA& dataRec );

        
        virtual ERR ErrMoveBeforeFirst();
        virtual ERR ErrMoveNext();
        virtual ERR ErrSeek( const COLUMNID columnid );


        virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const;
        virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;
        virtual ERR ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const;


        virtual ERR ErrGetColumnValue(  const size_t    iColumnValue,
                                        size_t* const   pcbColumnValue,
                                        void** const    ppvColumnValue,
                                        BOOL* const     pfColumnValueSeparated,
                                        BOOL* const     pfColumnValueCompressed ) const;

        virtual const FIELD* const PField() const
        {
            return m_pciterCurr->PField();
        }

    private:

        IColumnIter*    m_pciterLHS;
        IColumnIter*    m_pciterRHS;

        IColumnIter*    m_pciterCurr;
        BOOL            m_fRHSBeforeFirst;
};

INLINE CUnionIter::
CUnionIter()
    :   m_pciterLHS( NULL ),
        m_pciterRHS( NULL ),
        m_pciterCurr( NULL )
{
}

INLINE ERR CUnionIter::
ErrInit( IColumnIter* const pciterLHS, IColumnIter* const pciterRHS )
{
    ERR err = JET_errSuccess;

    if ( !pciterLHS || !pciterRHS )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    m_pciterLHS     = pciterLHS;
    m_pciterRHS     = pciterRHS;

    Call( ErrMoveBeforeFirst() );

HandleError:
    return err;
}

INLINE ERR CUnionIter::
ErrSetRecord( const DATA& dataRec )
{
    ERR err = JET_errSuccess;

    if ( !dataRec.Pv() )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Call( m_pciterLHS->ErrSetRecord( dataRec ) );
    Call( m_pciterRHS->ErrSetRecord( dataRec ) );

HandleError:
    return err;
}

INLINE ERR CUnionIter::
ErrGetColumnId( COLUMNID* const pColumnId ) const
{
    return m_pciterCurr->ErrGetColumnId( pColumnId );
}

INLINE ERR CUnionIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
{
    return m_pciterCurr->ErrGetColumnValueCount( pcColumnValue );
}

INLINE ERR CUnionIter::
ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const
{
    return m_pciterCurr->ErrGetColumnSize( pfucb, precsize, grbit );
}


INLINE ERR CUnionIter::
ErrGetColumnValue(  const size_t    iColumnValue,
                    size_t* const   pcbColumnValue,
                    void** const    ppvColumnValue,
                    BOOL* const     pfColumnValueSeparated,
                    BOOL* const     pfColumnValueCompressed ) const
{
    return m_pciterCurr->ErrGetColumnValue( iColumnValue,
                                            pcbColumnValue,
                                            ppvColumnValue,
                                            pfColumnValueSeparated,
                                            pfColumnValueCompressed );
}


