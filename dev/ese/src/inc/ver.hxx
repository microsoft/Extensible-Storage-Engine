// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.




class INST;
class VER;
INLINE VER *PverFromIfmp( IFMP ifmp );


#ifdef VERPERF
#endif



#ifdef DEBUG
BOOL FIsRCECleanup();
BOOL FInCritBucket( VER *pver );
#endif


enum NS
{
    nsNone,
    nsDatabase,
    nsVersionedUpdate,
    nsVersionedInsert,
    nsUncommittedVerInDB,
    nsCommittedVerInDB
};


enum VS
{
    vsNone,
    vsCommitted,
    vsUncommittedByCaller,
    vsUncommittedByOther
};

VS  VsVERCheck( const FUCB * pfucb, CSR * pcsr, const BOOKMARK& bookmark );
INLINE VS VsVERCheck( const FUCB * pfucb, const BOOKMARK& bookmark )
{
    return VsVERCheck( pfucb, Pcsr( const_cast<FUCB *>( pfucb ) ), bookmark );
}

INLINE INT TrxCmp( TRX trx1, TRX trx2 )
{
    LONG lTrx1 = (LONG)trx1;
    LONG lTrx2 = (LONG)trx2;

    if( trx1 == trx2 )
    {
        return 0;
    }
    else if( trxMax == trx2 )
    {
        return -1;
    }
    else if( trxMax == trx1 )
    {
        return 1;
    }
    else
    {
        return lTrx1 - lTrx2;
    }
}

INLINE INT RceidCmp( RCEID rceid1, RCEID rceid2 )
{
    const LONG lRceid1 = (LONG)rceid1;
    const LONG lRceid2 = (LONG)rceid2;

    if( rceid1 == rceid2 )
    {
        return 0;
    }
    else if( rceidNull== rceid1 )
    {
        return -1;
    }
    else if( rceidNull == rceid2 )
    {
        return 1;
    }
    else
    {
        return lRceid1 - lRceid2;
    }
}


typedef UINT OPER;

const OPER operMaskNull                 = 0x1000;
const OPER operMaskNullForMove          = 0x2000;
const OPER operMaskDDL                  = 0x0100;


const OPER operInsert                   = 0x0001;
const OPER operReplace                  = 0x0002;
const OPER operFlagDelete               = 0x0003;
const OPER operDelta                    = 0x0004;
const OPER operReadLock                 = 0x0005;
const OPER operWriteLock                = 0x0006;
const OPER operPreInsert                = 0x0080;
const OPER operAllocExt                 = 0x0007;
const OPER operDelta64                  = 0x0009;

const OPER operCreateTable              = 0x0100;
const OPER operDeleteTable              = 0x0300;
const OPER operAddColumn                = 0x0700;
const OPER operDeleteColumn             = 0x0900;
const OPER operCreateLV                 = 0x0b00;
const OPER operCreateIndex              = 0x0d00;
const OPER operDeleteIndex              = 0x0f00;

const OPER operRegisterCallback         = 0x0010;
const OPER operUnregisterCallback       = 0x0020;


INLINE BOOL FOperNull( const OPER oper )
{
    return ( oper & operMaskNull );
}


INLINE BOOL FOperDDL( const OPER oper )
{
    return ( oper & operMaskDDL ) && !FOperNull( oper );
}


INLINE BOOL FOperInHashTable( const OPER oper )
{
    return  !FOperNull( oper )
            && !FOperDDL( oper )
            && operAllocExt != oper
            && operCreateLV != oper;
}


INLINE BOOL FOperReplace( const OPER oper )
{
    return ( operReplace == oper );
}


INLINE BOOL FOperConcurrent( const OPER oper )
{
    return ( operDelta == oper
            || operDelta64 == oper
            || operWriteLock == oper
            || operPreInsert == oper
            || operReadLock == oper );
}


INLINE BOOL FOperAffectsSecondaryIndex( const OPER oper )
{
    return ( operInsert == oper || operFlagDelete == oper || operReplace == oper );
}


INLINE BOOL FUndoableLoggedOper( const OPER oper )
{
    return (    oper == operReplace
                || oper == operInsert
                || oper == operFlagDelete
                || oper == operDelta
                || oper == operDelta64 );
}


const UINT uiHashInvalid = UINT_MAX;


class RCE
{
#ifdef DEBUGGER_EXTENSION
    friend VOID EDBGVerHashSum( INST * pinstDebuggee, BOOL fVerbose );
#endif

    public:
        RCE (
            FCB *       pfcb,
            FUCB *      pfucb,
            UPDATEID    updateid,
            TRX         trxBegin0,
            TRX *       ptrxCommit0,
            LEVEL       level,
            USHORT      cbBookmarkKey,
            USHORT      cbBookmarkData,
            USHORT      cbData,
            OPER        oper,
            UINT        uiHash,
            BOOL        fProxy,
            RCEID       rceid
            );

        VOID    AssertValid ()  const;

    public:
        BOOL    FOperNull           ()  const;
        BOOL    FOperDDL            ()  const;
        BOOL    FUndoableLoggedOper ()  const;
        BOOL    FOperInHashTable    ()  const;
        BOOL    FOperReplace        ()  const;
        BOOL    FOperConcurrent     ()  const;
        BOOL    FOperAffectsSecondaryIndex  ()  const;
        BOOL    FActiveNotByMe      ( const PIB * ppib, const TRX trxSession ) const;

        BOOL    FRolledBack         ()  const;
        BOOL    FRolledBackEDBG     ()  const;
        BOOL    FMoved              ()  const;
        BOOL    FProxy              ()  const;
        BOOL    FEmptyDiff          ()  const;

        const   BYTE    *PbData         ()  const;
        const   BYTE    *PbBookmark     ()  const;
                INT     CbBookmarkKey   ()  const;
                INT     CbBookmarkData  ()  const;
                INT     CbData          ()  const;
                INT     CbRce           ()  const;
                INT     CbRceEDBG       ()  const;
        VOID    GetBookmark ( BOOKMARK * pbookmark ) const;
        VOID    CopyBookmark( const BOOKMARK& bookmark );
        ERR     ErrGetTaskForDelete( VOID ** ppvtask ) const;

        RCE     *PrceHashOverflow   ()  const;
        RCE     *PrceHashOverflowEDBG() const;
        RCE     *PrceNextOfNode     ()  const;
        RCE     *PrcePrevOfNode     ()  const;
        RCE     *PrceNextOfSession  ()  const;
        RCE     *PrcePrevOfSession  ()  const;
        BOOL    FFutureVersionsOfNode   ()  const;
        RCE     *PrceNextOfFCB      ()  const;
        RCE     *PrcePrevOfFCB      ()  const;
        RCE     *PrceUndoInfoNext ()    const;
        RCE     *PrceUndoInfoPrev ()    const;

        PGNO    PgnoUndoInfo        ()  const;

        RCEID   Rceid               ()  const;
        TRX     TrxBegin0           ()  const;

        LEVEL   Level       ()  const;
        IFMP    Ifmp        ()  const;
        PGNO    PgnoFDP     ()  const;
        OBJID   ObjidFDP    ()  const;
        TRX     TrxCommitted()  const;
        TRX     TrxCommittedEDBG()  const;
        BOOL    FFullyCommitted() const;

        UINT                UiHash      ()  const;
        CReaderWriterLock&  RwlChain    ();

        FUCB    *Pfucb      ()  const;
        FCB     *Pfcb       ()  const;
        OPER    Oper        ()  const;

        UPDATEID    Updateid()  const;

#ifdef DEBUGGER_EXTENSION
        VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif

    public:
        BYTE    *PbData             ();
        BYTE    *PbBookmark         ();

        RCE *&  PrceHashOverflow    ();

        VOID    ChangeOper          ( const OPER operNew );
        VOID    NullifyOper         ();
        ERR     ErrPrepareToDeallocate( TRX trxOldest );
        VOID    NullifyOperForMove  ();

        BOOL    FRCECorrectEDBG( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark );

        VOID    FlagRolledBack      ();
        VOID    FlagMoved           ();
        VOID    FlagEmptyDiff       ();

        VOID    SetLevel            ( LEVEL level );

        VOID    SetPrceHashOverflow ( RCE * prce );
        VOID    SetPrceNextOfNode   ( RCE * prce );
        VOID    SetPrcePrevOfNode   ( RCE * prce );
        VOID    SetPrcePrevOfSession( RCE * prce );
        VOID    SetPrceNextOfSession( RCE * prce );
        VOID    SetPrceNextOfFCB    ( RCE * prce );
        VOID    SetPrcePrevOfFCB    ( RCE * prce );

        VOID    SetTrxCommitted     ( const TRX trx );

        VOID    AddUndoInfo     ( PGNO pgno, RCE *prceNext, const BOOL fMove = fFalse );
        VOID    RemoveUndoInfo  ( const BOOL fMove = fFalse );

        VOID    SetCommittedByProxy( TRX trxCommit );

    private:
                ~RCE        ();
                RCE         ( const RCE& );
        RCE&    operator=   ( const RCE& );

#ifdef DEBUG
    private:
        BOOL    FAssertRwlHash_() const;
        BOOL    FAssertRwlHashAsWriter_() const;
        BOOL    FAssertCritFCBRCEList_() const;
        BOOL    FAssertRwlPIB_() const;
        BOOL    FAssertReadable_() const;
#endif

    private:


        const TRX           m_trxBegin0;

        TRX                 m_trxCommittedInactive;
        volatile TRX *      m_ptrxCommitted;

        const IFMP          m_ifmp;

        const PGNO          m_pgnoFDP;

        union
        {
            ULONG           m_ulFlags;
            struct
            {
                UINT        m_level:5;
                UINT        m_fRolledBack:1;
                UINT        m_fMoved:1;
                UINT        m_fProxy:1;
                UINT        m_fEmptyDiff:1;
            };
        };

        USHORT              m_oper;
        const USHORT        m_cbBookmarkKey;
        const USHORT        m_cbBookmarkData;
        const USHORT        m_cbData;

        const RCEID         m_rceid;
        const UPDATEID      m_updateid;

        FCB * const         m_pfcb;
        FUCB * const        m_pfucb;

        RCE *               m_prceNextOfSession;
        RCE *               m_prcePrevOfSession;
        RCE *               m_prceNextOfFCB;
        RCE *               m_prcePrevOfFCB;

        RCE *               m_prceNextOfNode;
        RCE *               m_prcePrevOfNode;

        RCE *               m_prceUndoInfoNext;
        RCE *               m_prceUndoInfoPrev;

        volatile PGNO       m_pgnoUndoInfo;

        const UINT          m_uiHash;
        RCE *               m_prceHashOverflow;


        BYTE                m_rgbData[0];
};

RCE * const prceNil     = 0;
#ifdef DEBUG
#ifdef _WIN64
RCE * const prceInvalid = (RCE *)0xFEADFEADFEADFEAD;
#else
RCE * const prceInvalid = (RCE *)(DWORD_PTR)0xFEADFEAD;
#endif
#endif


INLINE RCEID Rceid( const RCE * prce )
{
    if ( prceNil == prce )
    {
        return rceidNull;
    }
    else
    {
        return prce->Rceid();
    }
}


INLINE BOOL RCE::FOperNull() const
{
    return ::FOperNull( m_oper );
}


INLINE TRX RCE::TrxCommitted() const
{
    Assert( FIsRCECleanup() || FAssertReadable_() );


    TRX trx;

    while ( trxPrecommit == ( trx = *m_ptrxCommitted ) )
    {
        UtilSleep( 1 );
    }

    return trx;
}

INLINE TRX RCE::TrxCommittedEDBG() const
{
    return m_trxCommittedInactive;
}

INLINE BOOL RCE::FFullyCommitted() const
{
    return ( trxMax != m_trxCommittedInactive );
}

INLINE INT RCE::CbRce () const
{
    Assert( FIsRCECleanup() || FInCritBucket( PverFromIfmp( m_ifmp ) ) || FAssertReadable_() );
    return sizeof(RCE) + m_cbData + m_cbBookmarkKey + m_cbBookmarkData;
}
INLINE INT RCE::CbRceEDBG () const
{
    return sizeof(RCE) + m_cbData + m_cbBookmarkKey + m_cbBookmarkData;
}


INLINE INT RCE::CbData () const
{
    Assert( FInCritBucket( PverFromIfmp( m_ifmp ) ) || FAssertReadable_() );
    return m_cbData;
}


INLINE INT RCE::CbBookmarkKey() const
{
    Assert( FInCritBucket( PverFromIfmp( m_ifmp ) ) || FAssertReadable_() );
    return m_cbBookmarkKey;
}


INLINE INT RCE::CbBookmarkData() const
{
    Assert( FInCritBucket( PverFromIfmp( m_ifmp ) ) || FAssertReadable_() );
    return m_cbBookmarkData;
}


INLINE RCEID RCE::Rceid() const
{
    Assert( FAssertReadable_() );
    return m_rceid;
}


INLINE TRX RCE::TrxBegin0 () const
{
    Assert( FAssertReadable_() );
    return m_trxBegin0;
}


INLINE BOOL RCE::FOperDDL() const
{
    Assert( FAssertReadable_() );
    return ::FOperDDL( m_oper );
}


INLINE BOOL RCE::FOperInHashTable() const
{
    Assert( FAssertReadable_() );
    return ::FOperInHashTable( m_oper );
}


INLINE BOOL RCE::FUndoableLoggedOper() const
{
    Assert( FAssertReadable_() );
    return ::FUndoableLoggedOper( m_oper );
}


INLINE BOOL RCE::FOperReplace() const
{
    Assert( FAssertReadable_() );
    return ::FOperReplace( m_oper );
}


INLINE BOOL RCE::FOperConcurrent() const
{
    Assert( FAssertReadable_() );
    return ::FOperConcurrent( m_oper );
}


INLINE BOOL RCE::FOperAffectsSecondaryIndex() const
{
    Assert( FAssertReadable_() );
    return ::FOperAffectsSecondaryIndex( m_oper );
}

INLINE BYTE * RCE::PbData()
{
    Assert( FAssertReadable_() );
    AssertRTL( 0 == (LONG_PTR)(m_rgbData) % sizeof( LONG_PTR ) );
    return m_rgbData;
}


INLINE const BYTE * RCE::PbData() const
{
    Assert( FAssertReadable_() );
    AssertRTL( 0 == (LONG_PTR)(m_rgbData) % sizeof( LONG_PTR ) );
    return m_rgbData;
}


INLINE const BYTE * RCE::PbBookmark() const
{
    Assert( FAssertReadable_() );
    return m_rgbData + m_cbData;
}


INLINE VOID RCE::GetBookmark( BOOKMARK * pbookmark ) const
{
    Assert( FAssertReadable_() );
    pbookmark->key.prefix.Nullify();
    pbookmark->key.suffix.SetPv( const_cast<BYTE *>( PbBookmark() ) );
    pbookmark->key.suffix.SetCb( CbBookmarkKey() );
    pbookmark->data.SetPv( const_cast<BYTE *>( PbBookmark() ) + CbBookmarkKey() );
    pbookmark->data.SetCb( CbBookmarkData() );
    ASSERT_VALID( pbookmark );
}


INLINE VOID RCE::CopyBookmark( const BOOKMARK& bookmark )
{
    bookmark.key.CopyIntoBuffer( m_rgbData + m_cbData );
    UtilMemCpy( m_rgbData + m_cbData + bookmark.key.Cb(), bookmark.data.Pv(), bookmark.data.Cb() );
}


INLINE UINT RCE::UiHash() const
{
    Assert( FIsRCECleanup() || FAssertReadable_() );
    Assert( uiHashInvalid != m_uiHash );


    
    return m_uiHash;
}


INLINE IFMP RCE::Ifmp() const
{
    Assert( FIsRCECleanup() || FAssertReadable_() );
    return m_ifmp;
}


INLINE PGNO RCE::PgnoFDP() const
{
    Assert( FAssertReadable_() );
    return m_pgnoFDP;
}

INLINE OBJID RCE::ObjidFDP() const
{
    Assert( FAssertReadable_() );
    return Pfcb()->ObjidFDP();
}


INLINE FUCB *RCE::Pfucb() const
{
    Assert( FAssertReadable_() );
    Assert( !FFullyCommitted() );
    return m_pfucb;
}


INLINE UPDATEID RCE::Updateid() const
{
    Assert( FAssertReadable_() );
    Assert( !FFullyCommitted() );
    return m_updateid;
}


INLINE FCB *RCE::Pfcb() const
{
    Assert( FAssertReadable_() );
    return m_pfcb;
}


INLINE OPER RCE::Oper() const
{
    Assert( FIsRCECleanup() || FAssertReadable_() );
    return m_oper;
}


INLINE BOOL RCE::FActiveNotByMe( const PIB * ppib, const TRX trxSession ) const
{
    Assert( FAssertReadable_() );

    const TRX   trxCommitted    = TrxCommitted();

    Assert( trxSession != trxCommitted || PinstFromPpib( ppib )->FRecovering() );
    const BOOL  fActiveNotByMe  = ( trxMax == trxCommitted && m_pfucb->ppib != ppib )
                                    || ( trxMax != trxCommitted && TrxCmp( trxCommitted, trxSession ) > 0 );
    return fActiveNotByMe;
}


INLINE RCE *RCE::PrceHashOverflow() const
{
    Assert( FAssertRwlHash_() );
    return m_prceHashOverflow;
}


INLINE RCE *RCE::PrceHashOverflowEDBG() const
{
    return m_prceHashOverflow;
}


INLINE RCE *RCE::PrceNextOfNode() const
{
    Assert( FAssertRwlHash_() );
    return m_prceNextOfNode;
}


INLINE RCE *RCE::PrcePrevOfNode() const
{
    Assert( FAssertRwlHash_() );
    return m_prcePrevOfNode;
}


INLINE BOOL RCE::FFutureVersionsOfNode() const
{
    return prceNil != m_prceNextOfNode;
}


INLINE LEVEL RCE::Level() const
{
    Assert( PinstFromIfmp( Ifmp() )->FRecovering() || !FFullyCommitted() );
    return (LEVEL)m_level;
}


INLINE RCE *RCE::PrceNextOfSession() const
{
    return m_prceNextOfSession;
}


INLINE RCE *RCE::PrcePrevOfSession() const
{
    return m_prcePrevOfSession;
}


INLINE BOOL RCE::FRolledBack() const
{
    Assert( FIsRCECleanup() || FAssertReadable_() );
    return m_fRolledBack;
}
INLINE BOOL RCE::FRolledBackEDBG() const
{
    return m_fRolledBack;
}


INLINE BOOL RCE::FMoved() const
{
    return m_fMoved;
}


INLINE BOOL RCE::FProxy() const
{
    return m_fProxy;
}


INLINE BOOL RCE::FEmptyDiff() const
{
    return m_fEmptyDiff;
}


INLINE VOID RCE::FlagEmptyDiff()
{
    m_fEmptyDiff = fTrue;
}


INLINE RCE *RCE::PrceNextOfFCB() const
{
    Assert( PinstFromIfmp( Ifmp() )->FRecovering() || FAssertCritFCBRCEList_() );
    return m_prceNextOfFCB;
}


INLINE RCE *RCE::PrcePrevOfFCB() const
{
    Assert( PinstFromIfmp( Ifmp() )->FRecovering() || FAssertCritFCBRCEList_() );
    return m_prcePrevOfFCB;
}


INLINE VOID RCE::SetPrcePrevOfFCB( RCE * prce )
{
    Assert( PinstFromIfmp( Ifmp() )->FRecovering() || FAssertCritFCBRCEList_() );
    m_prcePrevOfFCB = prce;
}


INLINE VOID RCE::SetPrceNextOfFCB( RCE * prce )
{
    Assert( PinstFromIfmp( Ifmp() )->FRecovering() || FAssertCritFCBRCEList_() );
    m_prceNextOfFCB = prce;
}


INLINE RCE * RCE::PrceUndoInfoNext() const
{
    return m_prceUndoInfoNext;
}

INLINE RCE * RCE::PrceUndoInfoPrev() const
{
    return m_prceUndoInfoPrev;
}

INLINE PGNO RCE::PgnoUndoInfo() const
{
    return m_pgnoUndoInfo;
}


INLINE VOID RCE::AddUndoInfo( PGNO pgno, RCE *prceNext, const BOOL fMove )
{
    Assert( prceInvalid == m_prceUndoInfoNext );
    Assert( prceInvalid == m_prceUndoInfoPrev );
    Assert( prceNil == prceNext ||
            prceNil == prceNext->PrceUndoInfoPrev() );
    Assert( !fMove && pgnoNull == m_pgnoUndoInfo ||
            fMove && pgnoNull != m_pgnoUndoInfo );

    m_prceUndoInfoNext = prceNext;
    m_prceUndoInfoPrev = prceNil;

    if ( prceNext != prceNil )
    {
        Assert( prceNil == prceNext->m_prceUndoInfoPrev );
        prceNext->m_prceUndoInfoPrev = this;
    }

    m_pgnoUndoInfo = pgno;
}


INLINE VOID RCE::RemoveUndoInfo( const BOOL fMove )
{
    Assert( pgnoNull != m_pgnoUndoInfo );

    if ( prceNil != m_prceUndoInfoPrev )
    {
        Assert( this == m_prceUndoInfoPrev->m_prceUndoInfoNext );
        m_prceUndoInfoPrev->m_prceUndoInfoNext = m_prceUndoInfoNext;
    }

    if ( prceNil != m_prceUndoInfoNext )
    {
        Assert( this == m_prceUndoInfoNext->m_prceUndoInfoPrev );
        m_prceUndoInfoNext->m_prceUndoInfoPrev = m_prceUndoInfoPrev;
    }

#ifdef DEBUG
    m_prceUndoInfoPrev = prceInvalid;
    m_prceUndoInfoNext = prceInvalid;
#endif

    if ( !fMove )
    {
        m_pgnoUndoInfo = pgnoNull;
    }
}


INLINE VOID RCE::SetCommittedByProxy( TRX trxCommitted )
{
    Assert( TrxCommitted() == trxMax );

    m_prceNextOfSession = prceNil;
    m_prcePrevOfSession = prceNil;

    m_level = 0;

    Assert( m_ptrxCommitted == &m_trxCommittedInactive );
    m_trxCommittedInactive = trxCommitted;
}


INLINE VOID RCE::ChangeOper( const OPER operNew )
{
    Assert( operPreInsert == m_oper );
    Assert( USHORT( operNew ) == operNew );
    m_oper = USHORT( operNew );
}


struct VERREPLACE
{
    SHORT   cbMaxSize;
    SHORT   cbDelta;
    RCEID   rceidBeginReplace;
    BYTE    rgbBeforeImage[0];
};

const INT cbReplaceRCEOverhead = sizeof( VERREPLACE );


struct VEREXT
{
    PGNO    pgnoFDP;
    PGNO    pgnoChildFDP;
    PGNO    pgnoFirst;
    CPG     cpgSize;
};


template< typename TDelta > struct _VERDELTA;

template< typename TDelta >
struct VERDELTA_TRAITS
{
    static_assert( sizeof( TDelta ) == -1, "Specialize VERDELTA_TRAITS to map a VERDELTA to a specific verstore oper constant." );
    static const OPER oper = 0;
};

template<> struct VERDELTA_TRAITS< LONG >           { static const OPER oper = operDelta; };
template<> struct VERDELTA_TRAITS< LONGLONG >       { static const OPER oper = operDelta64; };

template< typename TDelta >
struct _VERDELTA
{
    TDelta          tDelta;
    USHORT          cbOffset;
    USHORT          fDeferredDelete:1;
    USHORT          fCallbackOnZero:1;
    USHORT          fDeleteOnZero:1;

    typedef VERDELTA_TRAITS< TDelta > TRAITS;
};

typedef _VERDELTA< LONG >       VERDELTA32;
typedef _VERDELTA< LONGLONG >   VERDELTA64;

struct VERADDCOLUMN
{
    JET_COLUMNID    columnid;
    BYTE            *pbOldDefaultRec;
};


struct VERCALLBACK
{
    JET_CALLBACK    pcallback;
    JET_CBTYP       cbtyp;
    VOID            *pvContext;
    CBDESC          *pcbdesc;
};


enum PROXYTYPE
{
    proxyRedo,
    proxyCreateIndex
};
struct VERPROXY
{
    union
    {
        RCEID   rceid;
        RCE     *prcePrimary;
    };

    PROXYTYPE   proxy:8;
    ULONG       level:8;
    ULONG       cbitReserved:16;
};



extern volatile LONG g_crefVERCreateIndexLock;


INLINE VOID VERBeginTransaction( PIB *ppib, const TRXID trxid )
{
    ASSERT_VALID( ppib );

    ppib->IncrementLevel( trxid );
    Assert( ppib->Level() < levelMax );

#ifdef NEVER
    LOG *plog = PinstFromPpib( ppib )->m_plog;
    if ( 1 == ppib->Level() && !( plog->FLogDisabled() || plog->FRecovering() ) && !( ppib->fReadOnly ) )
    {
        plog->GetLgposOfPbEntryWithLock( &ppib->lgposStart );
    }
#endif
}

VOID    VERCommitTransaction    ( PIB * const ppib );
ERR     ErrVERRollback          ( PIB *ppib );
ERR     ErrVERRollback          ( PIB *ppib, UPDATEID updateid );

BOOL    FVERActive          ( const IFMP ifmp, const PGNO pgnoFDP, const BOOKMARK& bm, const TRX trxSession );

BOOL    FVERActive          ( const FUCB * pfucb, const BOOKMARK& bm, const TRX trxSession );
INLINE BOOL FVERActive( const FUCB * pfucb, const BOOKMARK& bm )
{
    return FVERActive( pfucb, bm, TrxOldest( PinstFromPfucb( pfucb ) ) );
}
ERR     ErrVERAccessNode    ( FUCB * pfucb, const BOOKMARK& bookmark, NS * pns );
BOOL    FVERDeltaActiveNotByMe( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset );
BOOL    FVERWriteConflict   ( FUCB * pfucb, const BOOKMARK& bookmark, const OPER oper );

template< typename TDelta>
TDelta DeltaVERGetDelta ( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset );

template LONG DeltaVERGetDelta<LONG>( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset );
template LONGLONG DeltaVERGetDelta<LONGLONG>( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset );

BOOL    FVERCheckUncommittedFreedSpace(
    const FUCB      * pfucb,
    CSR             * const pcsr,
    const INT       cbReq,
    const BOOL      fPermitUpdateUncFree );
INT     CbVERGetNodeMax     ( const FUCB * pfucb, const BOOKMARK& bookmark );
BOOL FVERGetReplaceImage(
    const PIB       *ppib,
    const IFMP      ifmp,
    const PGNO      pgnoLVFDP,
    const BOOKMARK& bookmark,
    const RCEID     rceidFirst,
    const RCEID     rceidLast,
    const TRX       trxBegin0,
    const TRX       trxCommitted,
    const BOOL      fAfterImage,
    const BYTE      **ppb,
    ULONG           * const pcbActual
    );
INT     CbVERGetNodeReserve(
    const PIB * ppib,
    const FUCB * pfucb,
    const BOOKMARK& bookmark,
    INT cbCurrentData
    );
enum UPDATEPAGE { fDoUpdatePage, fDoNotUpdatePage };
VOID    VERSetCbAdjust(
    CSR         *pcsr,
    const RCE   *prce,
    INT         cbDataNew,
    INT         cbData,
    UPDATEPAGE  updatepage
    );

VOID VERNullifyFailedDMLRCE( RCE * prce );
VOID VERNullifyInactiveVersionsOnBM( const FUCB * pfucb, const BOOKMARK& bm );
VOID VERNullifyAllVersionsOnFCB( FCB * const pfcb );

VOID VERInsertRCEIntoLists( FUCB *pfucbNode, CSR *pcsr, RCE *prce, const VERPROXY *pverproxy = NULL );

INLINE TRX TrxVERISession ( const FUCB * const pfucb )
{
    const TRX trxSession = pfucb->ppib->trxBegin0;
    return trxSession;
}



INLINE BOOL FVERPotThere( const VS vs, const BOOL fDelete )
{
    return ( !fDelete || vsUncommittedByOther == vs );
}

#ifdef DEBUGGER_EXTENSION

UINT UiVERHash( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark, const UINT crcehead );

#endif


struct BUCKET;
BUCKET * const pbucketNil = 0;

class VER
    :   public CZeroInit
{
#ifdef DEBUGGER_EXTENSION
    friend VOID EDBGVerHashSum( INST * pinstDebuggee, BOOL fVerbose );
#endif

private:
    struct RCEHEAD
    {
        RCEHEAD() :
        rwl( CLockBasicInfo( CSyncBasicInfo( szRCEChain ), rankRCEChain, 0 ) ),
        prceChain( prceNil )
        {}

        CReaderWriterLock   rwl;
        RCE*                prceChain;
    };

    struct RCEHEADLEGACY
    {
        RCEHEADLEGACY() :
            crit( CLockBasicInfo( CSyncBasicInfo( szRCEChain ), rankRCEChain, 0 ) ),
            prceChain( prceNil )
        {
        }

        CCriticalSection    crit;
        RCE*                prceChain;
    };

public:
    VER( INST *pinst );
    ~VER();

private:
#pragma push_macro( "new" )
#undef new
    void* operator new[]( size_t );
    void operator delete[]( void* );
#pragma pop_macro( "new" )
public:

    enum { dtickRCECleanPeriod = 1000000 };

    INST                *m_pinst;

    volatile LONG       m_fVERCleanUpWait;
    TICK                m_tickLastRCEClean;
    CManualResetSignal  m_msigRCECleanPerformedRecently;
    CAutoResetSignal    m_asigRCECleanDone;
    CNestableCriticalSection    m_critRCEClean;

    CCriticalSection    m_critBucketGlobal;
    BUCKET              *m_pbucketGlobalHead;
    BUCKET              *m_pbucketGlobalTail;

    CResource           m_cresBucket;
    DWORD_PTR           m_cbBucket;

    PIB                 *m_ppibRCEClean;
    PIB                 *m_ppibRCECleanCallback;

    RCEID               m_rceidLast;

    PIB *               m_ppibTrxOldestLastLongRunningTransaction;
    DWORD_PTR           m_dwTrxContextLastLongRunningTransaction;
    TRX                 m_trxBegin0LastLongRunningTransaction;

    BOOL                m_fSyncronousTasks;
    RECTASKBATCHER      m_rectaskbatcher;

    BOOL                m_fAboveMaxTransactionSize;
public:

#ifdef VERPERF
    QWORD               qwStartTicks;

    INT                 m_cbucketCleaned;
    INT                 m_cbucketSeen;
    INT                 m_crceSeen;
    INT                 m_crceCleaned;
    INT                 m_crceFlagDelete;
    INT                 m_crceDeleteLV;

    CCriticalSection    m_critVERPerf;
#endif

public:
    size_t              m_crceheadHashTable;
    enum { cbrcehead = sizeof( VER::RCEHEAD ) };
    enum { cbrceheadLegacy = sizeof( VER::RCEHEADLEGACY ) };

private:
    BOOL                m_frceHashTableInited;
    RCEHEAD             m_rgrceheadHashTable[ 0 ];

private:

    VOID VERIReportDiscardedDeletes( const RCE * const prce );
    VOID VERIReportVersionStoreOOM( PIB * ppib, BOOL fMaxTrxSize, const BOOL fCleanupBlocked );



    INLINE size_t CbBUFree( const BUCKET * pbucket );
    INLINE BOOL FVERICleanWithoutIO();
    INLINE BOOL FVERICleanDiscardDeletes();
    INLINE ERR ErrVERIBUAllocBucket( const INT cbRCE, const UINT uiHash );
    INLINE BUCKET *PbucketVERIGetOldest( );
    BUCKET *PbucketVERIFreeAndGetNextOldestBucket( BUCKET * pbucket );

    ERR ErrVERIAllocateRCE( INT cbRCE, RCE ** pprce, const UINT uiHash );
    ERR ErrVERIMoveRCE( RCE * prce );
    ERR ErrVERICreateRCE(
            INT         cbNewRCE,
            FCB         *pfcb,
            FUCB        *pfucb,
            UPDATEID    updateid,
            const TRX   trxBegin0,
            TRX *       ptrxCommit0,
            const LEVEL level,
            INT         cbBookmarkKey,
            INT         cbBookmarkData,
            OPER        oper,
            UINT        uiHash,
            RCE         **pprce,
            const BOOL  fProxy = fFalse,
            RCEID       rceid = rceidNull
            );
    ERR ErrVERICreateDMLRCE(
            FUCB            * pfucb,
            UPDATEID        updateid,
            const BOOKMARK& bookmark,
            const UINT      uiHash,
            const OPER      oper,
            const LEVEL     level,
            const BOOL      fProxy,
            RCE             **pprce,
            RCEID           rceid
            );

    template< typename TDelta >
    ERR ErrVERICleanDeltaRCE( const RCE * const prce );

    ERR ErrVERIPossiblyDeleteLV( const RCE * const prce, ENTERCRITICALSECTION *pcritRCEChain );
    ERR ErrVERIPossiblyFinalize( const RCE * const prce, ENTERCRITICALSECTION *pcritRCEChain );

    static DWORD VERIRCECleanProc( VOID *pvThis );

public:
    static VER * VERAlloc( INST* pinst );
    static VOID VERFree( VER * pVer );
    ERR ErrVERInit( INST* pinst );
    VOID VERTerm( BOOL fNormal );
    INLINE ERR ErrVERModifyCommitted(
            FCB             *pfcb,
            const BOOKMARK& bookmark,
            const OPER      oper,
            const TRX       trxBegin0,
            const TRX       trxCommitted,
            RCE             **pprce
            );

    ERR ErrVERCheckTransactionSize( PIB * const ppib );
    ERR ErrVERModify(
            FUCB            * pfucb,
            const BOOKMARK& bookmark,
            const OPER      oper,
            RCE             **pprce,
            const VERPROXY  * const pverproxy
            );
    ERR ErrVERFlag( FUCB * pfucb, OPER oper, const VOID * pv, INT cb );
    ERR ErrVERStatus( );
    ERR ErrVERICleanOneRCE( RCE * const prce );
    ERR ErrVERRCEClean( const IFMP ifmp = g_ifmpMax );
    ERR ErrVERIRCEClean( const IFMP ifmp = g_ifmpMax );
    ERR ErrVERIDelete( PIB * ppib, const RCE * const prce );
    VOID VERSignalCleanup();

    RCEID RceidLast();
    RCEID RceidLastIncrement();

    VOID IncrementCAsyncCleanupDispatched();
    VOID IncrementCSyncCleanupDispatched();
    VOID IncrementCCleanupFailed();
    VOID IncrementCCleanupDiscarded( const RCE * const prce );

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif

    INLINE CReaderWriterLock& RwlRCEChain( UINT ui );
    INLINE RCE *GetChain( UINT ui ) const;
    INLINE RCE **PGetChain( UINT ui );
    INLINE VOID SetChain( UINT ui, RCE * );

#ifdef RTM
#else
public:
    ERR ErrInternalCheck();

protected:
    ERR ErrCheckRCEHashList( const RCE * const prce, const UINT uiHash ) const;
    ERR ErrCheckRCEChain( const RCE * const prce, const UINT uiHash ) const;
#endif
};

INLINE VER *PverFromIfmp( const IFMP ifmp ) { return g_rgfmp[ ifmp ].Pinst()->m_pver; }
INLINE VER *PverFromPpib( const PIB * const ppib ) { return ppib->m_pinst->m_pver; }
INLINE VOID VERSignalCleanup( const PIB * const ppib ) { PverFromPpib( ppib )->VERSignalCleanup(); }


INLINE RCEID VER::RceidLast()
{
    const RCEID rceid = m_rceidLast;
    Assert( rceid != rceidNull );
    return rceid;
}

INLINE RCEID VER::RceidLastIncrement( )
{
    const RCEID rceid = AtomicExchangeAdd( (LONG *)&m_rceidLast, 2 ) + 2;
    Assert(rceidNull != rceid);
    return rceid;
}




struct BUCKETHDR
{
    BUCKETHDR( VER* const pverIn, BYTE* const rgb )
        :   pver( pverIn ),
            pbucketPrev( NULL ),
            pbucketNext( NULL ),
            prceNextNew( (RCE *)rgb ),
            prceOldest( (RCE*)rgb ),
            pbLastDelete( rgb )
    {
    }

    VER         *pver;
    BUCKET      *pbucketPrev;
    BUCKET      *pbucketNext;
    RCE         *prceNextNew;
    RCE         *prceOldest;
    BYTE        *pbLastDelete;
};


struct BUCKET
{
    BUCKET( VER* const pver )
        :   hdr( pver, rgb )
    {
    }

#pragma push_macro( "new" )
#undef new
    private:
        void* operator new( size_t );
        void* operator new[]( size_t );
        void operator delete[]( void* );
    public:
        void* operator new( size_t cbAlloc, VER* pver )
        {
            return pver->m_cresBucket.PvRESAlloc_( SzNewFile(), UlNewLine() );
        }
        void operator delete( void* pv )
        {
            if ( pv )
            {
                BUCKET* pbucket = reinterpret_cast< BUCKET* >( pv );
                pbucket->hdr.pver->m_cresBucket.Free( pv );
            }
        }
#pragma pop_macro( "new" )

    BUCKETHDR       hdr;
    BYTE            rgb[ 0 ];
};

