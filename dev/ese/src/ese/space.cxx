// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.






#include "std.hxx"
#include "_space.hxx"

#ifdef PERFMON_SUPPORT

PERFInstanceLiveTotal<> cSPPagesTrimmed;
LONG LSPPagesTrimmedCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cSPPagesTrimmed.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotal<> cSPPagesNotTrimmedUnalignedPage;
LONG LSPPagesNotTrimmedUnalignedPageCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cSPPagesNotTrimmedUnalignedPage.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotal<> cSPDeletedTrees;
LONG LSPDeletedTreesCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cSPDeletedTrees.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotal<> cSPDeletedTreeFreedPages;
LONG LSPDeletedTreeFreedPagesCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cSPDeletedTreeFreedPages.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotal<> cSPDeletedTreeFreedExtents;
LONG LSPDeletedTreeFreedExtentsCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cSPDeletedTreeFreedExtents.PassTo( iInstance, pvBuf );
    return 0;
}

#endif


#include "_bt.hxx"


const CHAR * SzNameOfTable( const FUCB * const pfucb )
{
    if( pfucb->u.pfcb->FTypeTable() )
    {
        return pfucb->u.pfcb->Ptdb()->SzTableName();
    }
    else if( pfucb->u.pfcb->FTypeLV() )
    {
        return pfucb->u.pfcb->PfcbTable()->Ptdb()->SzTableName();
    }
    else if( pfucb->u.pfcb->FTypeSecondaryIndex() )
    {
        return pfucb->u.pfcb->PfcbTable()->Ptdb()->SzTableName();
    }
    return "";
}
const CHAR * SzSpaceTreeType( const FUCB * const pfucb )
{
    if ( pfucb->fAvailExt )
    {
        return " AE";
    }
    else if ( pfucb->fOwnExt )
    {
        return " OE";
    }
    return " !!";
}

#ifdef DEBUG

#endif

class CSPExtentInfo;

LOCAL ERR ErrSPIAddFreedExtent( FUCB *pfucb, FUCB *pfucbAE, const PGNO pgnoLast, const CPG cpgSize );

LOCAL ERR ErrSPISeekRootAE(
    __in FUCB* const pfucbAE,
    __in const PGNO pgno,
    __in const SpacePool sppAvailPool,
    __out CSPExtentInfo* const pspeiAE );

LOCAL ERR ErrSPIGetSparseInfoRange(
    _In_ FMP* const pfmp,
    _In_ const PGNO pgnoStart,
    _In_ const PGNO pgnoEnd,
    _Out_ CPG* pcpgSparse );

LOCAL ERR FSPIParentIsFs( FUCB * const pfucb );
LOCAL ERR ErrSPIGetFsSe(
    FUCB * const    pfucb,
    FUCB * const    pfucbAE,
    const CPG   cpgReq,
    const CPG   cpgMin,
    const ULONG fSPFlags,
    const BOOL fExact = fFalse,
    const BOOL fPermitAsyncExtension = fTrue,
    const BOOL fMayViolateMaxSize = fFalse );
LOCAL ERR ErrSPIGetSe(
    FUCB * const    pfucb,
    FUCB * const    pfucbAE,
    const CPG   cpgReq,
    const CPG   cpgMin,
    const ULONG fSPFlags,
    const SpacePool sppPool,
    const BOOL fMayViolateMaxSize = fFalse );
LOCAL ERR ErrSPIWasAlloc( FUCB *pfucb, PGNO pgnoFirst, CPG cpgSize );
LOCAL ERR ErrSPIValidFDP( PIB *ppib, IFMP ifmp, PGNO pgnoFDP );

LOCAL ERR ErrSPIReserveSPBufPages(
    FUCB* const pfucb,
    FUCB* const pfucbParent,
    const CPG cpgAddlReserveOE = 0,
    const CPG cpgAddlReserveAE = 0,
    const PGNO pgnoReplace = pgnoNull );
LOCAL ERR ErrSPIAddToAvailExt(
    __in    FUCB *      pfucbAE,
    __in    const PGNO  pgnoAELast,
    __in    const CPG   cpgAESize,
    __in    SpacePool   sppPool );


LOCAL ERR ErrSPIUnshelvePagesInRange( FUCB* const pfucbRoot, const PGNO pgnoFirst, const PGNO pgnoLast );


POSTIMERTASK g_posttSPITrimDBITask = NULL;
CSemaphore g_semSPTrimDBScheduleCancel( CSyncBasicInfo( "g_semSPTrimDBScheduleCancel" ) );
LOCAL VOID SPITrimDBITask( VOID*, VOID* );

ERR ErrSPInit()
{
    ERR err = JET_errSuccess;

    Assert( g_posttSPITrimDBITask == NULL );
    Call( ErrOSTimerTaskCreate( SPITrimDBITask, (void*)ErrSPInit, &g_posttSPITrimDBITask ) );

    Assert( g_semSPTrimDBScheduleCancel.CAvail() == 0 );
    g_semSPTrimDBScheduleCancel.Release();

HandleError:
    return err;
}

VOID SPTerm()
{
    if ( g_posttSPITrimDBITask )
    {
        OSTimerTaskCancelTask( g_posttSPITrimDBITask );
        OSTimerTaskDelete( g_posttSPITrimDBITask );
        g_posttSPITrimDBITask = NULL;
    }

    Assert( g_semSPTrimDBScheduleCancel.CAvail() <= 1 );
    g_semSPTrimDBScheduleCancel.Acquire();
    Assert( g_semSPTrimDBScheduleCancel.CAvail() == 0 );
}

INLINE VOID AssertSPIPfucbOnRoot( const FUCB * const pfucb )
{
#ifdef  DEBUG
    Assert( pfucb->pcsrRoot != pcsrNil );
    Assert( pfucb->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
    Assert( pfucb->pcsrRoot->Latch() == latchRIW
        || pfucb->pcsrRoot->Latch() == latchWrite );
    Assert( !FFUCBSpace( pfucb ) );
#endif
}

INLINE VOID AssertSPIPfucbOnRootOrNull( const FUCB * const pfucb )
{
#ifdef  DEBUG
    if ( pfucb != pfucbNil )
    {
        AssertSPIPfucbOnRoot( pfucb );
    }
#endif
}

INLINE VOID AssertSPIPfucbOnSpaceTreeRoot( FUCB *pfucb, CSR *pcsr )
{
#ifdef  DEBUG
    Assert( FFUCBSpace( pfucb ) );
    Assert( pcsr->FLatched() );
    Assert( pcsr->Pgno() == PgnoRoot( pfucb ) );
    Assert( pcsr->Cpage().FRootPage() );
    Assert( pcsr->Cpage().FSpaceTree() );
#endif
}

INLINE BOOL FSPValidPGNO( __in const PGNO pgno )
{
    return pgnoNull < pgno && pgnoSysMax > pgno;
}

INLINE BOOL FSPValidAllocPGNO( __in const PGNO pgno )
{
    return pgnoFDPMSO-1 < pgno && pgnoSysMax > pgno;
}


#ifdef DEBUG
PGNO    g_pgnoAllocTrap = 0;
INLINE VOID SPCheckPgnoAllocTrap( __in const PGNO pgnoAlloc, __in const CPG cpgAlloc )
{
    if ( g_pgnoAllocTrap == pgnoAlloc ||
            ( g_pgnoAllocTrap > pgnoAlloc && g_pgnoAllocTrap <= pgnoAlloc + cpgAlloc -1 ) )
    {
        AssertSz( fFalse, "Pgno Alloc Trap" );
    }
}
#else
INLINE VOID SPCheckPgnoAllocTrap( __in const PGNO pgnoAlloc, __in const CPG cpgAlloc )
{
    ;
}
#endif


#ifdef DEBUG
const PGNO pgnoBadNews = 0xFFFFFFFF;
const PGNO cpgBadNews = 0xFFFFFFFF;
#endif




#include <pshpack1.h>

PERSISTED
class SPEXTKEY {

    private:
        union {
            BYTE                mbe_pgnoLast[sizeof(PGNO)];
            struct
            {
                BYTE            bExtFlags;
                BYTE            mbe_pgnoLastAfterFlags[sizeof(PGNO)];
            };
            
        };

        BOOL _FNewSpaceKey() const
        {
            return fSPAvailExtReservedPool == ( bExtFlags & fSPAvailExtReservedPool );
        }

        ULONG _Cb( ) const
        {

            if ( _FNewSpaceKey() )
            {
                C_ASSERT( 0x5 == sizeof(SPEXTKEY) );
                return sizeof(SPEXTKEY);
            }
            else
            {
                C_ASSERT( 0x4 == sizeof(PGNO) );
                return sizeof(PGNO);
            }
        }

        PGNO _PgnoLast( void ) const
        {
            #ifdef DEBUG
            PGNO pgnoLast = pgnoBadNews;
            #else
            PGNO pgnoLast;
            #endif
            if ( _FNewSpaceKey() )
            {
                LongFromUnalignedKey( &pgnoLast, mbe_pgnoLastAfterFlags );
            }
            else
            {
                LongFromUnalignedKey( &pgnoLast, mbe_pgnoLast );
            }
            Assert( pgnoBadNews != pgnoLast );
            return pgnoLast;
        }

    public:

#ifdef DEBUG
        SPEXTKEY( )     { Invalidate(); }
        ~SPEXTKEY( )        { Invalidate(); }
        VOID Invalidate()   { memset( this, 0xFF, sizeof(*this) ); }
#else
        SPEXTKEY( )     { }
        ~SPEXTKEY( )        { }
#endif

    public:


        enum    E_SP_EXTENT_TYPE
                {
                    fSPExtentTypeUnknown    = 0x0,
                    fSPExtentTypeOE,
                    fSPExtentTypeAE,
                };


        enum    {   fSPAvailExtReservedPool = 0x80  };
        enum    {   fSPReservedZero         = 0x40  };
        enum    {   fSPReservedBitsMask     = 0x38  };
        enum    {   fSPPoolMask             = 0x07  };


        static_assert( ( 0xFF == ( (BYTE)fSPAvailExtReservedPool |
                                   (BYTE)fSPReservedZero         |
                                   (BYTE)fSPReservedBitsMask     |
                                   (BYTE)fSPPoolMask               ) ), "All bits are used." );

        static_assert( ( ( 0 == ( (BYTE)fSPAvailExtReservedPool & (BYTE)fSPReservedZero     ) ) &&
                         ( 0 == ( (BYTE)fSPAvailExtReservedPool & (BYTE)fSPReservedBitsMask ) ) &&
                         ( 0 == ( (BYTE)fSPAvailExtReservedPool & (BYTE)fSPPoolMask         ) ) &&
                         ( 0 == ( (BYTE)fSPReservedZero         & (BYTE)fSPReservedBitsMask ) ) &&
                         ( 0 == ( (BYTE)fSPReservedZero         & (BYTE)fSPPoolMask         ) ) &&
                         ( 0 == ( (BYTE)fSPReservedBitsMask     & (BYTE)fSPPoolMask         ) )    ) , "All bits are used only once." );

        static_assert( ( (BYTE)spp::MaxPool == ( (BYTE)fSPPoolMask & (BYTE)spp::MaxPool ) ),
                       "We are using enough bits to safely mask all spp:: values." );
    
        VOID Make(
            __in const E_SP_EXTENT_TYPE     eExtType,
            __in const PGNO                 pgnoLast,
            __in const SpacePool            sppAvailPool )
        {
            Assert( FSPIValidExplicitSpacePool( sppAvailPool ) );

            if ( eExtType == fSPExtentTypeOE )
            {
                Assert( spp::AvailExtLegacyGeneralPool == sppAvailPool );
                UnalignedKeyFromLong( mbe_pgnoLast, pgnoLast );
            }
            else if ( eExtType == fSPExtentTypeAE )
            {
                if ( spp::AvailExtLegacyGeneralPool != sppAvailPool )
                {
                    bExtFlags = ( ( BYTE ) fSPAvailExtReservedPool | ( BYTE )sppAvailPool );
                    UnalignedKeyFromLong( mbe_pgnoLastAfterFlags, pgnoLast );
                    Assert( _FNewSpaceKey() );
                }
                else
                {
                    UnalignedKeyFromLong( mbe_pgnoLast, pgnoLast );
                }
            }
            else
            {
                AssertSz( fFalse, "Unknown Ext type" );
            }
        }

        ULONG Cb( ) const           { ASSERT_VALID( this ); return _Cb(); }
        const VOID * Pv( ) const
        {
            ASSERT_VALID( this );
            Assert( (BYTE*)this == mbe_pgnoLast );

            C_ASSERT( OffsetOf( SPEXTKEY, mbe_pgnoLast ) == OffsetOf( SPEXTKEY, bExtFlags ) );
            Assert( OffsetOf( SPEXTKEY, mbe_pgnoLast ) == OffsetOf( SPEXTKEY, bExtFlags ) );
            C_ASSERT( OffsetOf( SPEXTKEY, mbe_pgnoLast ) != OffsetOf( SPEXTKEY, mbe_pgnoLastAfterFlags ) );
            Assert( OffsetOf( SPEXTKEY, mbe_pgnoLast ) != OffsetOf( SPEXTKEY, mbe_pgnoLastAfterFlags ) );

            return reinterpret_cast<const VOID *>(mbe_pgnoLast);
        }

        PGNO PgnoLast() const           { ASSERT_VALID( this ); Assert( pgnoNull != _PgnoLast() ); return _PgnoLast(); }
        BOOL FNewAvailFormat() const    { ASSERT_VALID( this ); return _FNewSpaceKey(); }
        SpacePool SppPool() const
        {
            SpacePool spp;
            ASSERT_VALID( this );
            
            if( _FNewSpaceKey() )
            {
                spp = (SpacePool) (bExtFlags & fSPPoolMask);
                Assert( FSPIValidExplicitSpacePool( spp ) );
                Assert( spp != spp::AvailExtLegacyGeneralPool );
            }
            else
            {
                spp = spp::AvailExtLegacyGeneralPool;
            }
            return spp;
        }

    public:

        enum    E_VALIDATE_TYPE
                {
                    fValidateData           = 0x01,
                    fValidateSearch         = 0x02
                };

        INLINE BOOL FValid( __in const E_SP_EXTENT_TYPE eExtType, __in const E_VALIDATE_TYPE fValidateType ) const
        {
            PGNO pgnoLast = _PgnoLast();

            Assert( pgnoBadNews != pgnoLast );
            if ( pgnoSysMax < pgnoLast )
            {
                return fFalse;
            }
            
            if ( fValidateData == fValidateType )
            {
                if ( eExtType == fSPExtentTypeOE &&
                    !FSPValidPGNO( pgnoLast ) )
                {
                    return fFalse;
                }

                if ( eExtType == fSPExtentTypeAE &&
                    !FSPValidAllocPGNO( pgnoLast ) )
                {
                    return fFalse;
                }
            }
            return fTrue;
        }
        INLINE BOOL FValid( __in const E_SP_EXTENT_TYPE eExtType, __in const ULONG cb ) const
        {
            return Cb() == cb && FValid( eExtType, fValidateData );
        }
    private:
        INLINE BOOL FValid( ) const
        {
            return FValid( fSPExtentTypeOE, fValidateData ) ||
                    FValid( fSPExtentTypeAE, fValidateData ) ||
                    FValid( fSPExtentTypeOE, fValidateSearch ) ||
                    FValid( fSPExtentTypeAE, fValidateSearch );
        }
    public:
#ifdef DEBUG
        VOID AssertValid() const
        {
            AssertSz( FValid( ), "Invalid Space Extent Key" );
        }
#endif

};

C_ASSERT( 0x5 == sizeof(SPEXTKEY ) );


PERSISTED
class SPEXTDATA {

    private:
        UnalignedLittleEndian< CPG >        le_cpgExtent;


    public:
        VOID Invalidate( )
        {
            #ifdef DEBUG
            le_cpgExtent = pgnoBadNews;
            #endif
        }
        SPEXTDATA( )
        {
            #ifdef DEBUG
            Invalidate();
            #endif
        }
        ~SPEXTDATA( )
        {
            #ifdef DEBUG
            Invalidate();
            #endif
        }

    public:
        SPEXTDATA( __in const CPG cpgExtent )
        {
            Set( cpgExtent );
            Assert( FValid( ) );
        }
        VOID Set( __in const CPG cpgExtent )
        {
            le_cpgExtent = cpgExtent;
            Assert( FValid( ) );
        }
        CPG         CpgExtent() const   { return le_cpgExtent; }
        ULONG       Cb() const          { C_ASSERT( 4 == sizeof(SPEXTDATA) ); return sizeof(SPEXTDATA); }
        const VOID* Pv() const          { return reinterpret_cast<const VOID*>(&le_cpgExtent); }

    public:
        BOOL FValid( ) const
        {
            return PGNO(le_cpgExtent) < pgnoSysMax;
        }
        BOOL FValid( ULONG cb ) const
        {
            return cb == Cb() && FValid( );
        }

};

C_ASSERT( 0x4 == sizeof(SPEXTDATA ) );

#include <poppack.h>

class CSPExtentInfo {

    private:

        SPEXTKEY::E_SP_EXTENT_TYPE  m_eSpExtType:8;
        FLAG32                      m_fFromTempDB:1;

        PGNO                        m_pgnoLast;
        CPG                         m_cpgExtent;
        SpacePool                   m_sppPool;
        FLAG32                      m_fNewAvailFormat:1;

        FLAG32                      m_fCorruptData:1;

        PGNO _PgnoFirst() const     { return m_pgnoLast - m_cpgExtent + 1; }
        BOOL _FOwnedExt() const     { return m_eSpExtType == SPEXTKEY::fSPExtentTypeOE; }
        BOOL _FAvailExt() const     { return m_eSpExtType == SPEXTKEY::fSPExtentTypeAE; }


        VOID _Set( __in const SPEXTKEY::E_SP_EXTENT_TYPE eSpExtType, __in const KEYDATAFLAGS& kdfCurr )
        {
            m_fCorruptData = fFalse;

            m_eSpExtType = eSpExtType;
            if ( SPEXTKEY::fSPExtentTypeUnknown == eSpExtType )
            {
                if( kdfCurr.key.Cb() == 0x4 )
                {
                    m_eSpExtType = SPEXTKEY::fSPExtentTypeOE;
                }
                else if ( kdfCurr.key.Cb() == 0x5 )
                {
                    m_eSpExtType = SPEXTKEY::fSPExtentTypeAE;
                }
            }

            if ( kdfCurr.key.Cb() == 0x5 )
            {
                Assert( m_eSpExtType == SPEXTKEY::fSPExtentTypeAE );
            }


            BYTE rgExtKey[sizeof(SPEXTKEY)];
            kdfCurr.key.CopyIntoBuffer( rgExtKey, sizeof( rgExtKey ) );
            const SPEXTKEY * const pSPExtKey = reinterpret_cast<const SPEXTKEY *>( rgExtKey );

            m_fCorruptData |= !pSPExtKey->FValid( m_eSpExtType, kdfCurr.key.Cb() );
            Assert( !m_fCorruptData );

            m_pgnoLast = pSPExtKey->PgnoLast();

            m_sppPool = pSPExtKey->SppPool();

            if ( pSPExtKey->FNewAvailFormat() )
            {
                Assert( m_eSpExtType == SPEXTKEY::fSPExtentTypeAE );
                Assert( _FAvailExt() );
                Assert( 0x5 == kdfCurr.key.Cb() );
                m_fNewAvailFormat = fTrue;
            }
            else
            {
                Assert( 0x4 == kdfCurr.key.Cb() );
                Assert( spp::AvailExtLegacyGeneralPool == m_sppPool );
                m_fNewAvailFormat = fFalse;
            }


            const SPEXTDATA * const pSPExtData = reinterpret_cast<const SPEXTDATA *>( kdfCurr.data.Pv() );

            m_fCorruptData |= !pSPExtData->FValid( kdfCurr.data.Cb() );
            Assert( !m_fCorruptData );

            m_cpgExtent = pSPExtData->CpgExtent();
            Assert( m_cpgExtent >= 0 );

        }

    public:

#ifdef DEBUG
        VOID Invalidate()
        {
            m_fCorruptData = fTrue;
            m_eSpExtType = SPEXTKEY::fSPExtentTypeUnknown;
            m_pgnoLast = pgnoBadNews;
            m_cpgExtent = cpgBadNews;
        }
        VOID AssertValid() const
        {
            Assert( SPEXTKEY::fSPExtentTypeUnknown != m_eSpExtType &&
                    pgnoBadNews != m_pgnoLast &&
                    cpgBadNews != m_cpgExtent );
            Assert( SPEXTKEY::fSPExtentTypeAE == m_eSpExtType ||
                    SPEXTKEY::fSPExtentTypeOE == m_eSpExtType );
            Assert( m_fCorruptData || ErrCheckCorrupted( fTrue ) >= JET_errSuccess );
        }
#endif
        CSPExtentInfo(  )   { Unset(); }
        ~CSPExtentInfo(  )  { Unset(); }

        CSPExtentInfo( const FUCB * pfucb )
        {
            Set( pfucb );
            ASSERT_VALID( this );
            Assert( cpgBadNews != PgnoLast() );
            Assert( cpgBadNews != CpgExtent() );
        }

        CSPExtentInfo( const KEYDATAFLAGS * pkdf )
        {
            _Set( SPEXTKEY::fSPExtentTypeUnknown, *pkdf );
            ASSERT_VALID( this );
            Assert( cpgBadNews != PgnoLast() );
            Assert( cpgBadNews != CpgExtent() );
        }

        VOID Set( __in const FUCB * pfucb )
        {
            m_fCorruptData = fFalse;

            Assert( FFUCBSpace( pfucb ) );
            Assert( Pcsr( pfucb )->FLatched() );

            m_eSpExtType = pfucb->fAvailExt ? SPEXTKEY::fSPExtentTypeAE : SPEXTKEY::fSPExtentTypeOE;
            m_fFromTempDB = FFMPIsTempDB( pfucb->ifmp );

            _Set( m_eSpExtType, pfucb->kdfCurr );

        }

        VOID Unset( )
        {
            m_eSpExtType = SPEXTKEY::fSPExtentTypeUnknown;
            #ifdef DEBUG
            Invalidate();
            #endif
        }

        BOOL FIsSet( ) const
        {
            return m_eSpExtType != SPEXTKEY::fSPExtentTypeUnknown;
        }

        BOOL FOwnedExt() const  { ASSERT_VALID( this ); return _FOwnedExt(); }
        BOOL FAvailExt() const  { ASSERT_VALID( this ); return _FAvailExt(); }
        
        BOOL FValidExtent( ) const      { ASSERT_VALID( this ); return ErrCheckCorrupted( fFalse ) >= JET_errSuccess; }
        PGNO PgnoLast() const           { ASSERT_VALID( this ); Assert( FValidExtent( ) ); return m_pgnoLast; }
        CPG  CpgExtent() const          { ASSERT_VALID( this ); Assert( FValidExtent( ) ); return m_cpgExtent; }
        PGNO PgnoFirst() const          { ASSERT_VALID( this ); Assert( FValidExtent( ) ); Assert( m_cpgExtent != 0 ); return _PgnoFirst(); }
        BOOL FContains( __in const PGNO pgnoIn ) const
        {
            ASSERT_VALID( this );
            #ifdef DEBUG
            BOOL fInExtent = ( ( pgnoIn >= _PgnoFirst() ) && ( pgnoIn <= m_pgnoLast ) );
            if ( 0x0 == m_cpgExtent )
            {
                Assert( !fInExtent );
            }
            else
            {
                Assert( FValidExtent( ) );
            }
            return fInExtent;
            #else
            return ( ( pgnoIn >= _PgnoFirst() ) && ( pgnoIn <= m_pgnoLast ) );
            #endif
        }

        BOOL FNewAvailFormat() const    { ASSERT_VALID( this ); Assert( FValidExtent( ) ); return m_fNewAvailFormat; }
        CPG  FEmptyExtent() const       { ASSERT_VALID( this ); return 0x0 == m_cpgExtent; }
        PGNO PgnoMarker() const     { ASSERT_VALID( this ); return m_pgnoLast; }
        SpacePool SppPool() const
        {
            ASSERT_VALID( this );

            Assert ( m_fNewAvailFormat || m_sppPool == spp::AvailExtLegacyGeneralPool );
            return m_sppPool;
        }


        enum    FValidationStrength { fStrong = 0x0, fZeroLengthOK = 0x1, fZeroFirstPgnoOK = 0x2 };
        INLINE static BOOL FValidExtent( __in const PGNO pgnoLast, __in const CPG cpg, __in const FValidationStrength fStrength = fStrong )
        {

            if( fStrength & fZeroLengthOK )
            {
                if( ( cpg < 0 ) ||
                    ( !( ( pgnoLast - cpg ) <= pgnoLast ) ) ||
                    ( (ULONG)cpg > pgnoLast ) ||
                    ( ( (INT)pgnoLast - cpg ) <= ( fStrength & fZeroFirstPgnoOK ? -1 : 0 ) ) ||
                    ( ( pgnoLast + 1 ) < (ULONG)cpg ) )
                {
                    return fFalse;
                }
                Assert( ( pgnoLast - cpg + 1  ) <= pgnoLast );
            }
            else
            {
                if( ( cpg <= 0 ) ||
                    ( !( ( pgnoLast - cpg ) < pgnoLast ) ) ||
                    ( (ULONG)cpg > pgnoLast ) ||
                    ( ( (INT)pgnoLast - cpg ) <= ( fStrength & fZeroFirstPgnoOK ? -1 : 0 ) ) ||
                    ( ( pgnoLast + 1 ) < (ULONG)cpg ) )
                {
                    return fFalse;
                }
                Assert( ( pgnoLast - cpg + 1  ) <= pgnoLast );
            }
            return fTrue;
        }

        ERR ErrCheckCorrupted( BOOL fSilentOperation = fFalse ) const
        {
            if( m_fCorruptData )
            {
                AssertSz( fSilentOperation, "Ext Node corrupted at TAG level" );
                goto HandleError;
            }

            if ( m_fNewAvailFormat && _FOwnedExt() )
            {
                AssertSz( fSilentOperation, "OE does not currently use the new format" );
                goto HandleError;
            }

            if ( ( spp::AvailExtLegacyGeneralPool != m_sppPool ) && _FOwnedExt() )
            {
                AssertSz( fSilentOperation, "OE does not currently support pools" );
                goto HandleError;
            }

            if ( !FSPIValidExplicitSpacePool( m_sppPool ) )
            {
                AssertSz( fSilentOperation, "Invalid pool" );
                goto HandleError;
            }

            if ( spp::ShelvedPool == m_sppPool && 1 != m_cpgExtent )
            {
                AssertSz( fSilentOperation, "Shelved pool must only contain single-page extents." );
                goto HandleError;
            }

            if( spp::ContinuousPool != m_sppPool || 0 != m_cpgExtent )
            {
                if( !FValidExtent( m_pgnoLast,
                                   m_cpgExtent,
                                   FValidationStrength( ( _FAvailExt() ? fZeroLengthOK : fStrong ) |
                                                        ( _FOwnedExt() ? fZeroFirstPgnoOK : fStrong ) ) ) )
                {
                    AssertSz( fSilentOperation, "Extent invalid range, could cause math overflow" );
                    goto HandleError;
                }
            }

            if( !FSPValidPGNO( _PgnoFirst() ) || !FSPValidPGNO( m_pgnoLast ) )
            {
                AssertSz( fSilentOperation, "Invalid pgno" );
                goto HandleError;
            }

            Assert( m_cpgExtent < 128 * 1024 / g_cbPage * 1024 * 1024 );

            return JET_errSuccess;

        HandleError:
            if ( _FOwnedExt()  )
            {
                return ErrERRCheck( JET_errSPOwnExtCorrupted );
            }
            else if ( _FAvailExt() )
            {
                return ErrERRCheck( JET_errSPAvailExtCorrupted );
            }

            AssertSz( fFalse, "Unknown extent type." );
            return ErrERRCheck( JET_errInternalError );
        }

};


class CSPExtentKeyBM {

    private:
        BOOKMARK                    m_bm;
        SPEXTKEY                    m_spextkey;
        SPEXTKEY::E_SP_EXTENT_TYPE  m_eSpExtType:8;
        BYTE                        m_fBookmarkSet:1;

    public:

        CSPExtentKeyBM()
        {
            m_fBookmarkSet = fFalse;
            #ifdef DEBUG
            m_eSpExtType = SPEXTKEY::fSPExtentTypeUnknown;
            m_spextkey.Invalidate();
            m_bm.Invalidate();
            #endif
        }

        CSPExtentKeyBM(
            __in const  SPEXTKEY::E_SP_EXTENT_TYPE  eExtType,
            __in const  PGNO                        pgno,
            __in const  SpacePool                   sppAvailPool = spp::AvailExtLegacyGeneralPool
            )
        {
            m_fBookmarkSet = fFalse;

            SPExtentMakeKeyBM( eExtType, pgno, sppAvailPool );

            Assert( m_spextkey.FValid( eExtType, SPEXTKEY::fValidateSearch  ) );
            Assert( m_spextkey.Pv() == m_bm.key.suffix.Pv() );
        }

        VOID SPExtentMakeKeyBM(
            __in const  SPEXTKEY::E_SP_EXTENT_TYPE  eExtType,
            __in const  PGNO                        pgno,
            __in const  SpacePool                   sppAvailPool = spp::AvailExtLegacyGeneralPool
            )
        {
            m_eSpExtType = eExtType;

            if( m_eSpExtType == SPEXTKEY::fSPExtentTypeAE )
            {
                Assert( FSPIValidExplicitSpacePool( sppAvailPool ) );
            }
            else
            {
                Assert( SPEXTKEY::fSPExtentTypeOE == eExtType );
                Assert( spp::AvailExtLegacyGeneralPool == sppAvailPool );
            }

            m_spextkey.Make( eExtType, pgno, sppAvailPool );
            Assert( m_spextkey.FValid( eExtType, SPEXTKEY::fValidateSearch ) );

            if ( !m_fBookmarkSet )
            {
                m_bm.key.prefix.Nullify();
                m_bm.key.suffix.SetPv( (VOID *) m_spextkey.Pv() );
                m_bm.key.suffix.SetCb( m_spextkey.Cb() );
                m_bm.data.Nullify();
                Assert( m_bm.key.suffix.Pv() == m_spextkey.Pv() );
                m_fBookmarkSet = fTrue;
            }

        }

        const BOOKMARK& GetBm( FUCB * pfucb ) const
        {
            Assert( FFUCBSpace( pfucb ) && FFUCBUnique( pfucb ) );
            Assert( m_spextkey.FValid( pfucb->fAvailExt ? SPEXTKEY::fSPExtentTypeAE : SPEXTKEY::fSPExtentTypeOE, SPEXTKEY::fValidateSearch ) );
            Assert( pgnoBadNews != m_spextkey.PgnoLast() );
            return m_bm;
        }

        const BOOKMARK* Pbm( FUCB * pfucb ) const
        {
            Assert( FFUCBSpace( pfucb ) && FFUCBUnique( pfucb ) );
            Assert( m_spextkey.FValid( pfucb->fAvailExt ? SPEXTKEY::fSPExtentTypeAE : SPEXTKEY::fSPExtentTypeOE, SPEXTKEY::fValidateSearch ) );
            return &m_bm;
        }

};

class CSPExtentNodeKDF {

    private:

        KEYDATAFLAGS                m_kdf;

        SPEXTDATA                   m_spextdata;
        SPEXTKEY                    m_spextkey;

        SPEXTKEY::E_SP_EXTENT_TYPE  m_eSpExtType:8;

        BYTE                        m_fShouldDeleteNode:1;

    public:

#ifdef DEBUG
        VOID AssertValid( ) const
        {
            Assert( pgnoBadNews != m_spextkey.PgnoLast() );
            Assert( pgnoBadNews != m_spextdata.CpgExtent() );
        }
#endif

        VOID Invalidate( void )
        {
            #ifdef DEBUG
            m_kdf.Nullify();
            m_spextkey.Invalidate();
            m_spextdata.Invalidate();
            #endif
        }
        BOOL FValid( void ) const
        {
            Assert( pgnoBadNews != m_spextkey.PgnoLast() );
            Assert( cpgBadNews != m_spextdata.CpgExtent() );
            if( m_spextkey.PgnoLast() > pgnoSysMax ||
                m_spextdata.CpgExtent() > pgnoSysMax ||
                pgnoNull == m_spextkey.PgnoLast() ||
                ( m_spextkey.SppPool() != spp::ContinuousPool &&
                    0x0 == m_spextdata.CpgExtent() )
                )
            {
                return fFalse;
            }
            return fTrue;
        }

    private:
        CSPExtentNodeKDF( )
        {
            Invalidate();
        }
    public:

        CSPExtentNodeKDF(
            __in const  SPEXTKEY::E_SP_EXTENT_TYPE  eExtType,
            __in const  PGNO                        pgnoLast,
            __in const  CPG                         cpgExtent,
            __in const  SpacePool                   sppAvailPool = spp::AvailExtLegacyGeneralPool )
        {
            Invalidate();

            m_eSpExtType = eExtType;

            m_spextkey.Make( m_eSpExtType, pgnoLast, sppAvailPool );

            m_spextdata.Set( cpgExtent );

            m_kdf.key.prefix.Nullify();
            m_kdf.key.suffix.SetCb( m_spextkey.Cb() );
            m_kdf.key.suffix.SetPv( (VOID*)m_spextkey.Pv() );
            m_kdf.data.SetCb( m_spextdata.Cb() );
            m_kdf.data.SetPv( (VOID*)m_spextdata.Pv() );
            m_kdf.fFlags = 0;

            m_fShouldDeleteNode = fFalse;

            ASSERT_VALID( this );
        }

        class CSPExtentInfo;

        ERR ErrConsumeSpace( __in const PGNO pgnoConsume, __in const CPG cpgConsume = 1 )
        {
            ASSERT_VALID( this );

            Assert( pgnoConsume == PgnoFirst() );
            Assert( pgnoConsume <= PgnoLast() );
            if ( cpgConsume < 0 )
            {
                AssertSz( fFalse, "Consume space has a negative cpgConsume." );
                return ErrERRCheck( JET_errInternalError );
            }
            else if ( cpgConsume != 1 )
            {
                Assert( pgnoConsume + cpgConsume - 1 <= PgnoLast() );
            }

            m_spextdata.Set( CpgExtent() - cpgConsume );

            Assert( m_spextkey.FValid( m_eSpExtType, SPEXTKEY::fValidateData ) );

            if ( m_spextkey.SppPool() != spp::ContinuousPool &&
                    CpgExtent() == 0 )
            {
                m_fShouldDeleteNode = fTrue;
            }

            if ( !m_fShouldDeleteNode && !FValid( ) )
            {
                AssertSz( fFalse, "Consume space has corrupted data." );
                return ErrERRCheck( JET_errInternalError );
            }

            ASSERT_VALID( this );
            return JET_errSuccess;
        }

        ERR ErrUnconsumeSpace( __in const CPG cpgConsume )
        {
            ASSERT_VALID( this );

            m_spextdata.Set( CpgExtent() + cpgConsume );

            if ( !FValid( ) )
            {
                AssertSz( fFalse, "Unconsume space has corrupted data." );
                return ErrERRCheck( JET_errInternalError );
            }

            ASSERT_VALID( this );
            return JET_errSuccess;
        }


        BOOL FDelete( void ) const
        {
            ASSERT_VALID( this );
            return m_fShouldDeleteNode;
        }

        PGNO PgnoLast( void ) const
        {
            ASSERT_VALID( this );
            Assert( m_kdf.key.Cb() == sizeof(PGNO) || m_kdf.key.Cb() == sizeof(SPEXTKEY) );
            return m_spextkey.PgnoLast();
        }
        PGNO PgnoFirst( void ) const
        {
            ASSERT_VALID( this );
            return PgnoLast() - CpgExtent() + 1;
        }

        CPG CpgExtent( void ) const
        {
            ASSERT_VALID( this );
            return m_spextdata.CpgExtent();
        }

        const KEYDATAFLAGS * Pkdf( void ) const
        {
            ASSERT_VALID( this );
            return &m_kdf;
        }

        const KEYDATAFLAGS& Kdf( void ) const
        {
            ASSERT_VALID( this );
            return m_kdf;
        }

        BOOKMARK GetBm( FUCB * pfucb ) const
        {
            BOOKMARK bm;
            Assert( pgnoBadNews != m_spextkey.PgnoLast() );
            Assert( FFUCBUnique( pfucb ) );
            NDGetBookmarkFromKDF( pfucb, Kdf(), &bm );

            ASSERT_VALID( this );
            return bm;
        }
        const KEY& GetKey( ) const
        {
            ASSERT_VALID( this );
            return Kdf().key;
        }
        DATA GetData( ) const
        {
            ASSERT_VALID( this );
            Assert( !m_fShouldDeleteNode );
            return Kdf().data;
        }

};


ERR ErrSPIExtentLastPgno( __in const FUCB * pfucb, __out PGNO * ppgnoLast )
{
    const CSPExtentInfo spext( pfucb );
    CallS( spext.ErrCheckCorrupted() );
    *ppgnoLast = spext.PgnoLast();
    return JET_errSuccess;
}

ERR ErrSPIExtentFirstPgno( __in const FUCB * pfucb, __out PGNO * ppgnoFirst )
{
    const CSPExtentInfo spext( pfucb );
    CallS( spext.ErrCheckCorrupted() );
    *ppgnoFirst = spext.PgnoFirst();
    return JET_errSuccess;
}

ERR ErrSPIExtentCpg( __in const FUCB * pfucb, __out CPG * pcpgSize )
{
    const CSPExtentInfo spext( pfucb );
    CallS( spext.ErrCheckCorrupted() );
    *pcpgSize = spext.CpgExtent();
    return JET_errSuccess;
}

ERR ErrSPIGetExtentInfo( __in const FUCB * pfucb, __out PGNO * ppgnoLast, __out CPG * pcpgSize, __out SpacePool * psppPool )
{
    const CSPExtentInfo spext( pfucb );
    CallS( spext.ErrCheckCorrupted() );
    *ppgnoLast = spext.PgnoLast();
    *pcpgSize = spext.CpgExtent();
    *psppPool = spext.SppPool();
    return JET_errSuccess;
}

ERR ErrSPIGetExtentInfo( __in const KEYDATAFLAGS * pkdf, __out PGNO * ppgnoLast, __out CPG * pcpgSize, __out SpacePool * psppPool )
{
    const CSPExtentInfo spext( pkdf );
    CallS( spext.ErrCheckCorrupted() );
    *ppgnoLast = spext.PgnoLast();
    *pcpgSize = spext.CpgExtent();
    *psppPool = spext.SppPool();
    return JET_errSuccess;
}


ERR ErrSPREPAIRValidateSpaceNode(
    __in const  KEYDATAFLAGS * pkdf,
    __out       PGNO *          ppgnoLast,
    __out       CPG *           pcpgExtent,
    __out       PCWSTR *        pwszPoolName )
{
    SpacePool spp;

    ERR err = ErrSPIREPAIRValidateSpaceNode(
        pkdf,
        ppgnoLast,
        pcpgExtent,
        &spp
        );

    if ( JET_errSuccess <= err )
    {
        *pwszPoolName = WszPoolName( spp );
    }

    return err;
}

ERR ErrSPIREPAIRValidateSpaceNode(
    __in const  KEYDATAFLAGS * pkdf,
    __out       PGNO *          ppgnoLast,
    __out       CPG *           pcpgExtent,
    __out       SpacePool *     psppPool )
{
    ERR err = JET_errSuccess;
    const CSPExtentInfo spext( pkdf );

    Assert( pkdf );
    Assert( ppgnoLast );
    Assert( pcpgExtent );

    Call( spext.ErrCheckCorrupted() );

    *ppgnoLast = spext.PgnoLast();
    *pcpgExtent = spext.CpgExtent();
    *psppPool = spext.SppPool();

HandleError:

    return err;
}

INLINE BOOL FSPIIsSmall( const FCB * const pfcb )
{
    #ifdef DEBUG
    Assert( pfcb->FSpaceInitialized() );
    if ( pfcb->PgnoOE() == pgnoNull )
    {
        Assert( pfcb->PgnoAE() == pgnoNull );
        Assert( FSPValidAllocPGNO( pfcb->PgnoFDP() ) );
        if ( !FFMPIsTempDB( pfcb->Ifmp() ) )
        {
            Assert( !FCATBaseSystemFDP( pfcb->PgnoFDP() ) );
        }
    }
    else
    {
        Assert( pfcb->PgnoAE() != pgnoNull );
        Assert( pfcb->PgnoOE() + 1 == pfcb->PgnoAE() );
    }
    #endif

    return pfcb->PgnoOE() == pgnoNull;
}

LOCAL VOID SPIUpgradeToWriteLatch( FUCB *pfucb )
{
    CSR     *pcsrT;

    if ( Pcsr( pfucb )->Latch() == latchNone )
    {
        Assert( pfucb->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );

        pcsrT = pfucb->pcsrRoot;
        Assert( latchRIW == pcsrT->Latch() || latchWrite == pcsrT->Latch()  );
    }
    else
    {
        Assert( Pcsr( pfucb ) == pfucb->pcsrRoot );
        Assert( Pcsr(pfucb)->Pgno() == PgnoFDP( pfucb ) );
        pcsrT = &pfucb->csr;
    }

    if ( pcsrT->Latch() != latchWrite )
    {
        Assert( pcsrT->Latch() == latchRIW );
        pcsrT->UpgradeFromRIWLatch();
    }
    else
    {
#ifdef DEBUG
        LINE    lineSpaceHeader;
        NDGetPtrExternalHeader( pcsrT->Cpage(), &lineSpaceHeader, noderfSpaceHeader );
        Assert( sizeof( SPACE_HEADER ) == lineSpaceHeader.cb );
        SPACE_HEADER * psph = (SPACE_HEADER*)lineSpaceHeader.pv;
        Assert( psph->FSingleExtent() );
        Assert( !psph->FMultipleExtent() );
        Assert( pcsrT->Cpage().FRootPage() );
        Assert( pcsrT->Cpage().FInvisibleSons() );
#endif
    }
}


ERR ErrSPIOpenAvailExt( PIB *ppib, FCB *pfcb, FUCB **ppfucbAE )
{
    ERR     err;
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = pfcb->TCE( fTrue );
    tcScope->SetDwEngineObjid( pfcb->ObjidFDP() );
    tcScope->iorReason.SetIort( iortSpace );

    CallR( ErrBTOpen( ppib, pfcb, ppfucbAE, fFalse ) );
    FUCBSetAvailExt( *ppfucbAE );
    FUCBSetIndex( *ppfucbAE );
    Assert( pfcb->FSpaceInitialized() );
    Assert( pfcb->PgnoAE() != pgnoNull );

    return err;
}


ERR ErrSPIOpenOwnExt( PIB *ppib, FCB *pfcb, FUCB **ppfucbOE )
{
    ERR err;
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = pfcb->TCE( fTrue );
    tcScope->SetDwEngineObjid( pfcb->ObjidFDP() );
    tcScope->iorReason.SetIort( iortSpace );

    CallR( ErrBTOpen( ppib, pfcb, ppfucbOE, fFalse ) );
    FUCBSetOwnExt( *ppfucbOE );
    FUCBSetIndex( *ppfucbOE );
    Assert( pfcb->FSpaceInitialized() );
    Assert( !FSPIIsSmall( pfcb ) );

    return err;
}


ERR ErrSPGetLastPgno( _Inout_ PIB * ppib, _In_ const IFMP ifmp, _Out_ PGNO * ppgno )
{
    ERR err;
    EXTENTINFO extinfo;

    Call( ErrSPGetLastExtent( ppib, ifmp, &extinfo ) );
    *ppgno = extinfo.PgnoLast();

HandleError:
    return err;
}


ERR ErrSPGetLastExtent( _Inout_ PIB * ppib, _In_ const IFMP ifmp, _Out_ EXTENTINFO * pextinfo )
{
    ERR     err;
    FUCB    *pfucb = pfucbNil;
    FUCB    *pfucbOE = pfucbNil;
    DIB     dib;

    CallR( ErrBTOpen( ppib, pgnoSystemRoot, ifmp, &pfucb, openNormal, fTrue ) );
    Assert( pfucbNil != pfucb );

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortSpace );

    if ( PinstFromPpib( ppib )->FRecovering() && !pfucb->u.pfcb->FSpaceInitialized() )
    {
        Call( ErrSPInitFCB( pfucb ) );

        pfucb->u.pfcb->Lock();
        pfucb->u.pfcb->ResetInitedForRecovery();
        pfucb->u.pfcb->Unlock();
    }
    Assert( pfucb->u.pfcb->FSpaceInitialized() );

    Call( ErrSPIOpenOwnExt( ppib, pfucb->u.pfcb, &pfucbOE ) );

    dib.dirflag = fDIRNull;
    dib.pos     = posLast;
    dib.pbm     = NULL;
    err = ErrBTDown( pfucbOE, &dib, latchReadTouch );

    if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
    {
        FireWall( "GetDbLastExtNoOwned" );
        Error( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
    }

    Call( err );

    {
    const CSPExtentInfo spLastOE( pfucbOE );
    Assert( spLastOE.FIsSet() && ( spLastOE.CpgExtent() > 0 ) );
    pextinfo->pgnoLastInExtent = spLastOE.PgnoLast();
    pextinfo->cpgExtent = spLastOE.CpgExtent();
    OSTraceFMP( ifmp, JET_tracetagSpaceInternal,
             OSFormat( "%hs: ifmp=%d now has pgnoLast=%lu, cpgExtent=%ld",
                       __FUNCTION__, ifmp, pextinfo->PgnoLast(), pextinfo->CpgExtent() ) );
    }

HandleError:
    if ( pfucbOE != pfucbNil )
    {
        BTClose( pfucbOE );
    }

    Assert ( pfucb != pfucbNil );
    BTClose( pfucb );

    return err;
}


C_ASSERT( sizeof(SPACE_HEADER) == 16 );

LOCAL VOID SPIInitFCB( FUCB *pfucb, const BOOL fDeferredInit )
{
    CSR             * pcsr  = ( fDeferredInit ? pfucb->pcsrRoot : Pcsr( pfucb ) );
    FCB             * pfcb  = pfucb->u.pfcb;

    Assert( pcsr->FLatched() );

    pfcb->Lock();

    if ( !pfcb->FSpaceInitialized() )
    {
        NDGetExternalHeader ( pfucb, pcsr, noderfSpaceHeader );
        Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
        const SPACE_HEADER * const psph = reinterpret_cast <const SPACE_HEADER * const> ( pfucb->kdfCurr.data.Pv() );

        if ( psph->FSingleExtent() )
        {
            pfcb->SetPgnoOE( pgnoNull );
            pfcb->SetPgnoAE( pgnoNull );
        }
        else
        {
            pfcb->SetPgnoOE( psph->PgnoOE() );
            pfcb->SetPgnoAE( psph->PgnoAE() );
            Assert( pfcb->PgnoAE() == pfcb->PgnoOE() + 1 );
        }

        if ( !fDeferredInit )
        {
            Assert( pfcb->FUnique() );
            if ( psph->FNonUnique() )
                pfcb->SetNonUnique();
        }

        pfcb->SetSpaceInitialized();

        #ifdef DEBUG
        if( psph->FSingleExtent() )
        {
            Assert( FSPIIsSmall( pfcb ) );
        }
        else
        {
            Assert( !FSPIIsSmall( pfcb ) );
        }
        #endif

    }

    pfcb->Unlock();

    return;
}


ERR ErrSPInitFCB( _Inout_ FUCB * const pfucb )
{
    ERR             err;
    FCB             *pfcb   = pfucb->u.pfcb;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->iorReason.SetIort( iortSpace );
    tcScope->SetDwEngineObjid( pfcb->ObjidFDP() );

    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !FFUCBSpace( pfucb ) );

    err = ErrBTIGotoRoot( pfucb, latchReadTouch );
    if ( err < 0 )
    {
        if ( g_fRepair )
        {
            pfcb->SetPgnoOE( pgnoNull );
            pfcb->SetPgnoAE( pgnoNull );

            Assert( objidNil == pfcb->ObjidFDP() );

            err = JET_errSuccess;
        }
    }
    else
    {

        Assert( objidNil == pfcb->ObjidFDP()
            || ( PinstFromIfmp( pfucb->ifmp )->FRecovering() && pfcb->ObjidFDP() == Pcsr( pfucb )->Cpage().ObjidFDP() ) );
        pfcb->SetObjidFDP( Pcsr( pfucb )->Cpage().ObjidFDP() );

        SPIInitFCB( pfucb, fFalse );

        BTUp( pfucb );
    }

    return err;
}


ERR ErrSPDeferredInitFCB( _Inout_ FUCB * const pfucb )
{
    ERR             err;
    FCB             * pfcb  = pfucb->u.pfcb;
    FUCB            * pfucbT    = pfucbNil;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortSpace );

    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !FFUCBSpace( pfucb ) );

    CallR( ErrBTIOpenAndGotoRoot(
                pfucb->ppib,
                pfcb->PgnoFDP(),
                pfucb->ifmp,
                &pfucbT ) );
    Assert( pfucbNil != pfucbT );
    Assert( pfucbT->u.pfcb == pfcb );
    Assert( pcsrNil != pfucbT->pcsrRoot );

    if ( !pfcb->FSpaceInitialized() )
    {
        SPIInitFCB( pfucbT, fTrue );
    }

    pfucbT->pcsrRoot->ReleasePage();
    pfucbT->pcsrRoot = pcsrNil;

    Assert( pfucbNil != pfucbT );
    BTClose( pfucbT );

    return JET_errSuccess;
}

INLINE const SPACE_HEADER * PsphSPIRootPage( FUCB* pfucb )
{
    const SPACE_HEADER  *psph;

    AssertSPIPfucbOnRoot( pfucb );

    NDGetExternalHeader( pfucb, pfucb->pcsrRoot, noderfSpaceHeader );
    Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
    psph = reinterpret_cast <const SPACE_HEADER *> ( pfucb->kdfCurr.data.Pv() );

    return psph;
}

PGNO PgnoSPIParentFDP( FUCB *pfucb )
{
    return PsphSPIRootPage( pfucb )->PgnoParent();
}

BOOL FSPIsRootSpaceTree( const FUCB * const pfucb )
{
    return ( ObjidFDP( pfucb ) == objidSystemRoot && FFUCBSpace( pfucb ) );
}

INLINE SPLIT_BUFFER *PspbufSPISpaceTreeRootPage( FUCB *pfucb, CSR *pcsr )
{
    SPLIT_BUFFER    *pspbuf;

    AssertSPIPfucbOnSpaceTreeRoot( pfucb, pcsr );

    NDGetExternalHeader( pfucb, pcsr, noderfWhole );
    Assert( sizeof( SPLIT_BUFFER ) == pfucb->kdfCurr.data.Cb() );
    pspbuf = reinterpret_cast <SPLIT_BUFFER *> ( pfucb->kdfCurr.data.Pv() );

    return pspbuf;
}

INLINE VOID SPITraceSplitBufferMsg( const FUCB * const pfucb, const CHAR * const szMsg )
{
    OSTraceFMP( pfucb->ifmp, JET_tracetagSpaceInternal, OSFormat(
                        "Database '%ws'[ifmp=0x%x]: %s SplitBuffer for a Btree (objidFDP %u, pgnoFDP %u).",
                        g_rgfmp[pfucb->ifmp].WszDatabaseName(),
                        pfucb->ifmp,
                        szMsg,
                        pfucb->u.pfcb->ObjidFDP(),
                        pfucb->u.pfcb->PgnoFDP() ) );
}

LOCAL ERR ErrSPIFixSpaceTreeRootPage( FUCB *pfucb, SPLIT_BUFFER **ppspbuf )
{
    ERR         err         = JET_errSuccess;
    const BOOL  fAvailExt   = FFUCBAvailExt( pfucb );

    AssertTrack( fFalse, "UnexpectedSpBufFixup" );

    AssertSPIPfucbOnSpaceTreeRoot( pfucb, Pcsr( pfucb ) );
    Assert( latchRIW == Pcsr( pfucb )->Latch() );

#ifdef DEBUG
    const BOOL  fNotEnoughPageSpace = ( Pcsr( pfucb )->Cpage().CbPageFree() < ( g_cbPage * 3 / 4 ) );
#else
    const BOOL  fNotEnoughPageSpace = ( Pcsr( pfucb )->Cpage().CbPageFree() < sizeof(SPLIT_BUFFER) );
#endif

    if ( fNotEnoughPageSpace )
    {
        if ( NULL == pfucb->u.pfcb->Psplitbuf( fAvailExt ) )
        {
            CallR( pfucb->u.pfcb->ErrEnableSplitbuf( fAvailExt ) );
            SPITraceSplitBufferMsg( pfucb, "Allocated" );
        }
        *ppspbuf = pfucb->u.pfcb->Psplitbuf( fAvailExt );
        Assert( NULL != *ppspbuf );
    }
    else
    {
        const BOOL      fSplitbufDangling   = ( NULL != pfucb->u.pfcb->Psplitbuf( fAvailExt ) );
        SPLIT_BUFFER    spbuf;
        DATA            data;
        Assert( 0 == pfucb->kdfCurr.data.Cb() );

        if ( fSplitbufDangling )
        {
            memcpy( &spbuf, pfucb->u.pfcb->Psplitbuf( fAvailExt ), sizeof(SPLIT_BUFFER) );
        }
        else
        {
            memset( &spbuf, 0, sizeof(SPLIT_BUFFER) );
        }

        data.SetPv( &spbuf );
        data.SetCb( sizeof(spbuf) );

        Pcsr( pfucb )->UpgradeFromRIWLatch();
        err = ErrNDSetExternalHeader(
                    pfucb,
                    &data,
                    ( pfucb->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    noderfWhole );
        Pcsr( pfucb )->Downgrade( latchRIW );
        CallR( err );

        if ( fSplitbufDangling )
        {
            pfucb->u.pfcb->DisableSplitbuf( fAvailExt );
            SPITraceSplitBufferMsg( pfucb, "Persisted" );
        }

        NDGetExternalHeader( pfucb, Pcsr( pfucb ), noderfWhole );

        *ppspbuf = reinterpret_cast <SPLIT_BUFFER *> ( pfucb->kdfCurr.data.Pv() );
    }

    return err;
}

INLINE ERR ErrSPIGetSPBuf( FUCB *pfucb, SPLIT_BUFFER **ppspbuf )
{
    ERR err;

    AssertSPIPfucbOnSpaceTreeRoot( pfucb, Pcsr( pfucb ) );

    NDGetExternalHeader( pfucb, Pcsr( pfucb ), noderfWhole );

    if ( sizeof( SPLIT_BUFFER ) != pfucb->kdfCurr.data.Cb() )
    {
        AssertTrack( fFalse, "PersistedSpBufMissing" );
        err = ErrSPIFixSpaceTreeRootPage( pfucb, ppspbuf );
    }
    else
    {
        Assert( NULL == pfucb->u.pfcb->Psplitbuf( FFUCBAvailExt( pfucb ) ) );
        *ppspbuf = reinterpret_cast <SPLIT_BUFFER *> ( pfucb->kdfCurr.data.Pv() );
        err = JET_errSuccess;
    }

    return err;
}


ERR ErrSPIGetSPBufUnlatched( __inout FUCB * pfucbSpace, __out_bcount(sizeof(SPLIT_BUFFER)) SPLIT_BUFFER * pspbuf )
{
    ERR err = JET_errSuccess;
    LINE line;

    Assert( pspbuf );
    Assert( pfucbSpace );
    Assert( !Pcsr( pfucbSpace )->FLatched() );

    CallR( ErrBTIGotoRoot( pfucbSpace, latchReadNoTouch ) );
    Assert( Pcsr( pfucbSpace )->FLatched() );

    NDGetPtrExternalHeader( Pcsr( pfucbSpace )->Cpage(), &line, noderfWhole );
    if (sizeof( SPLIT_BUFFER ) == line.cb)
    {
        UtilMemCpy( (void*)pspbuf, line.pv, sizeof( SPLIT_BUFFER ) );
    }
    else
    {
        memset( (void*)pspbuf, 0, sizeof(SPLIT_BUFFER) );
        Assert( 0 == pspbuf->CpgBuffer1() );
        Assert( 0 == pspbuf->CpgBuffer2() );
    }

    BTUp( pfucbSpace );

    return err;
}


LOCAL ERR ErrSPISPBufContains(
    _In_ PIB* const ppib,
    _In_ FCB* const pfcbFDP,
    _In_ const PGNO pgno,
    _In_ const BOOL fOwnExt,
    _Out_ BOOL* const pfContains )
{
    ERR err = JET_errSuccess;
    FUCB* pfucbSpace = pfucbNil;
    SPLIT_BUFFER* pspbuf;

    if ( fOwnExt )
    {
        Call( ErrSPIOpenOwnExt( ppib, pfcbFDP, &pfucbSpace ) );
    }
    else
    {
        Call( ErrSPIOpenAvailExt( ppib, pfcbFDP, &pfucbSpace ) );
    }
    Call( ErrBTIGotoRoot( pfucbSpace, latchRIW ) );
    Call( ErrSPIGetSPBuf( pfucbSpace, &pspbuf ) );
    *pfContains = ( ( pgno >= ( pspbuf->PgnoLastBuffer1() - pspbuf->CpgBuffer1() + 1 ) ) &&
                        ( pgno <= pspbuf->PgnoLastBuffer1() ) ) ||
                  ( ( pgno >= ( pspbuf->PgnoLastBuffer2() - pspbuf->CpgBuffer2() + 1 ) ) &&
                        ( pgno <= pspbuf->PgnoLastBuffer2() ) );

HandleError:
    if ( pfucbSpace != pfucbNil )
    {
        BTUp( pfucbSpace );
        BTClose( pfucbSpace );
        pfucbSpace = pfucbNil;
    }

    return err;
}


INLINE VOID SPIDirtyAndSetMaxDbtime( CSR *pcsr1, CSR *pcsr2, CSR *pcsr3 )
{
#ifdef DEBUG
    Assert( pcsr1->Latch() == latchWrite );
    Assert( pcsr2->Latch() == latchWrite );
    Assert( pcsr3->Latch() == latchWrite );

    const IFMP ifmp = pcsr1->Cpage().Ifmp();
    Assert( pcsr2->Cpage().Ifmp() == ifmp );
    Assert( pcsr3->Cpage().Ifmp() == ifmp );

    const BOOL fRedoImplicitCreateDb = ( PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() == fRecoveringRedo ) &&
                                  g_rgfmp[ifmp].FCreatingDB() &&
                                  !FFMPIsTempDB( ifmp );
    const DBTIME dbtimeMaxBefore = max( max( pcsr1->Dbtime(), pcsr2->Dbtime() ), pcsr3->Dbtime() );
    if ( fRedoImplicitCreateDb )
    {
        Expected( pcsr1->Dbtime() < dbtimeStart );
        Expected( pcsr2->Dbtime() < dbtimeStart );
        Expected( pcsr3->Dbtime() < dbtimeStart );
        Expected( pcsr1->Dbtime() != pcsr2->Dbtime() );
        Expected( pcsr2->Dbtime() != pcsr3->Dbtime() );
        Expected( pcsr1->Dbtime() != pcsr3->Dbtime() );
        Expected( dbtimeMaxBefore -
                min( min( pcsr1->Dbtime(), pcsr2->Dbtime() ), pcsr3->Dbtime() )
                == 2 );
    }
    else if ( !g_fRepair )
    {
        Expected( pcsr1->Dbtime() >= dbtimeStart );
        Expected( pcsr2->Dbtime() >= dbtimeStart );
        Expected( pcsr3->Dbtime() >= dbtimeStart );
    }
#endif

    pcsr1->Dirty();
    pcsr2->Dirty();
    pcsr3->Dirty();

    const DBTIME dbtimeMax = max( max( pcsr1->Dbtime(), pcsr2->Dbtime() ), pcsr3->Dbtime() );

#ifdef DEBUG
    if ( fRedoImplicitCreateDb )
    {
        Expected( dbtimeMax < dbtimeStart );
        Expected( ( dbtimeMax - dbtimeMaxBefore ) == 3 );
        Expected( pcsr1->Dbtime() != pcsr2->Dbtime() );
        Expected( pcsr2->Dbtime() != pcsr3->Dbtime() );
        Expected( pcsr1->Dbtime() != pcsr3->Dbtime() );
        Expected( dbtimeMax -
                min( min( pcsr1->Dbtime(), pcsr2->Dbtime() ), pcsr3->Dbtime() )
                == 2 );
    }
    else if ( !g_fRepair )
    {
        Expected( dbtimeMax >= dbtimeStart );
    }
#endif

    pcsr1->SetDbtime( dbtimeMax );
    pcsr2->SetDbtime( dbtimeMax );
    pcsr3->SetDbtime( dbtimeMax );
}


INLINE VOID SPIInitSplitBuffer( FUCB *pfucb, CSR *pcsr )
{
    DATA            data;
    SPLIT_BUFFER    spbuf;

    memset( &spbuf, 0, sizeof(SPLIT_BUFFER) );

    Assert( FBTIUpdatablePage( *pcsr ) );
    Assert( latchWrite == pcsr->Latch() );
    Assert( pcsr->FDirty() );
    Assert( pgnoNull != PgnoAE( pfucb ) || g_fRepair );
    Assert( pgnoNull != PgnoOE( pfucb ) || g_fRepair );
    Assert( ( FFUCBAvailExt( pfucb ) && pcsr->Pgno() == PgnoAE( pfucb ) )
        || ( FFUCBOwnExt( pfucb ) && pcsr->Pgno() == PgnoOE( pfucb ) )
        || g_fRepair );

    data.SetPv( (VOID *)&spbuf );
    data.SetCb( sizeof(spbuf) );
    NDSetExternalHeader( pfucb, pcsr, noderfWhole, &data );
}

VOID SPICreateExtentTree( FUCB *pfucb, CSR *pcsr, PGNO pgnoLast, CPG cpgExtent, BOOL fAvail )
{
    if ( !FBTIUpdatablePage( *pcsr ) )
    {
        return;
    }

    Assert( !FFUCBVersioned( pfucb ) );
    Assert( !FFUCBSpace( pfucb ) );
    Assert( pgnoNull != PgnoAE( pfucb ) || g_fRepair );
    Assert( pgnoNull != PgnoOE( pfucb ) || g_fRepair );
    Assert( latchWrite == pcsr->Latch() );
    Assert( pcsr->FDirty() );

    if ( fAvail )
    {
        FUCBSetAvailExt( pfucb );
    }
    else
    {
        FUCBSetOwnExt( pfucb );
    }
    Assert( pcsr->FDirty() );

    SPIInitSplitBuffer( pfucb, pcsr );

    Assert( 0 == pcsr->Cpage().Clines() );
    pcsr->SetILine( 0 );

    if ( cpgExtent != 0 )
    {

        const CSPExtentNodeKDF      spextnode( fAvail ?
                                                    SPEXTKEY::fSPExtentTypeAE :
                                                    SPEXTKEY::fSPExtentTypeOE,
                                                pgnoLast, cpgExtent );
        NDInsert( pfucb, pcsr, spextnode.Pkdf() );
    }
    else
    {
        Assert( FFUCBAvailExt( pfucb ) );
    }

    if ( fAvail )
    {
        FUCBResetAvailExt( pfucb );
    }
    else
    {
        FUCBResetOwnExt( pfucb );
    }

    return;
}


VOID SPIInitPgnoFDP( FUCB *pfucb, CSR *pcsr, const SPACE_HEADER& sph )
{
    if ( !FBTIUpdatablePage( *pcsr ) )
    {
        return;
    }

    Assert( latchWrite == pcsr->Latch() );
    Assert( pcsr->FDirty() );
    Assert( pcsr->Pgno() == PgnoFDP( pfucb ) || g_fRepair );

    DATA    data;

    data.SetPv( (VOID *)&sph );
    data.SetCb( sizeof(sph) );
    NDSetExternalHeader( pfucb, pcsr, noderfSpaceHeader, &data );
}


ERR ErrSPIImproveLastAlloc( FUCB * pfucb, const SPACE_HEADER * const psph, CPG cpgLast )
{
    ERR             err = JET_errSuccess;

    AssertSPIPfucbOnRoot( pfucb );

    if ( psph->FSingleExtent() )
    {
        return JET_errSuccess;
    }

    Assert( psph->FMultipleExtent() );

    SPACE_HEADER sphSet;
    memcpy( &sphSet, psph, sizeof(sphSet) );

    sphSet.SetCpgLast( cpgLast );

    DATA data;
    data.SetPv( &sphSet );
    data.SetCb( sizeof(sphSet) );

    if ( latchWrite != pfucb->pcsrRoot->Latch() )
    {
        Assert( pfucb->pcsrRoot->Latch() == latchRIW );
        SPIUpgradeToWriteLatch( pfucb );
    }
    Assert( pfucb->pcsrRoot->Latch() == latchWrite );

    CallR( ErrNDSetExternalHeader( pfucb, pfucb->pcsrRoot, &data,
                                    ( pfucb->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                                    noderfSpaceHeader ) );

    return err;
}


VOID SPIPerformCreateMultiple( FUCB *pfucb, CSR *pcsrFDP, CSR *pcsrOE, CSR *pcsrAE, PGNO pgnoPrimary, const SPACE_HEADER& sph )
{
    const CPG   cpgPrimary = sph.CpgPrimary();
    const PGNO  pgnoLast = pgnoPrimary;
    Assert( g_fRepair || pgnoLast == PgnoFDP( pfucb ) + cpgPrimary - 1 );

    Assert( FBTIUpdatablePage( *pcsrFDP ) );
    Assert( FBTIUpdatablePage( *pcsrOE ) );
    Assert( FBTIUpdatablePage( *pcsrAE ) );

    SPIInitPgnoFDP( pfucb, pcsrFDP, sph );

    SPICreateExtentTree( pfucb, pcsrOE, pgnoLast, cpgPrimary, fFalse );

    SPICreateExtentTree( pfucb, pcsrAE, pgnoLast, cpgPrimary - 1 - 1 - 1, fTrue );

    return;
}


ERR ErrSPCreateMultiple(
    FUCB        *pfucb,
    const PGNO  pgnoParent,
    const PGNO  pgnoFDP,
    const OBJID objidFDP,
    const PGNO  pgnoOE,
    const PGNO  pgnoAE,
    const PGNO  pgnoPrimary,
    const CPG   cpgPrimary,
    const BOOL  fUnique,
    const ULONG fPageFlags )
{
    ERR             err;
    CSR             csrOE;
    CSR             csrAE;
    CSR             csrFDP;
    SPACE_HEADER    sph;
    LGPOS           lgpos;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortSpace );

    Assert( objidFDP == ObjidFDP( pfucb ) || g_fRepair );
    Assert( objidFDP != objidNil );

    Assert( !( fPageFlags & CPAGE::fPageRoot ) );
    Assert( !( fPageFlags & CPAGE::fPageLeaf ) );
    Assert( !( fPageFlags & CPAGE::fPageParentOfLeaf ) );
    Assert( !( fPageFlags & CPAGE::fPageEmpty ) );
    Assert( !( fPageFlags & CPAGE::fPagePreInit ) );
    Assert( !( fPageFlags & CPAGE::fPageSpaceTree) );

    const BOOL fLogging = g_rgfmp[pfucb->ifmp].FLogOn();

    Call( csrOE.ErrGetNewPreInitPage( pfucb->ppib,
                                      pfucb->ifmp,
                                      pgnoOE,
                                      objidFDP,
                                      fLogging ) );
    csrOE.ConsumePreInitPage( ( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree ) & ~CPAGE::fPageRepair );
    Assert( csrOE.Latch() == latchWrite );

    Call( csrAE.ErrGetNewPreInitPage( pfucb->ppib,
                                      pfucb->ifmp,
                                      pgnoAE,
                                      objidFDP,
                                      fLogging ) );
    csrAE.ConsumePreInitPage( ( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree ) & ~CPAGE::fPageRepair );
    Assert( csrAE.Latch() == latchWrite );

    Call( csrFDP.ErrGetNewPreInitPage( pfucb->ppib,
                                       pfucb->ifmp,
                                       pgnoFDP,
                                       objidFDP,
                                       fLogging ) );
    csrFDP.ConsumePreInitPage( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf );
    Assert( csrFDP.Latch() == latchWrite );

    SPIDirtyAndSetMaxDbtime( &csrFDP, &csrOE, &csrAE );

    sph.SetCpgPrimary( cpgPrimary );
    sph.SetPgnoParent( pgnoParent );

    Assert( sph.FSingleExtent() );
    Assert( sph.FUnique() );

    sph.SetMultipleExtent();

    if ( !fUnique )
    {
        sph.SetNonUnique();
    }

    sph.SetPgnoOE( pgnoOE );

    Assert( ( PinstFromPfucb( pfucb )->m_plog->FRecoveringMode() != fRecoveringRedo ) ||
            ( !g_rgfmp[pfucb->ifmp].FLogOn() && g_rgfmp[pfucb->ifmp].FCreatingDB() ) );

    Call( ErrLGCreateMultipleExtentFDP( pfucb, &csrFDP, &sph, fPageFlags, &lgpos ) );
    csrFDP.Cpage().SetLgposModify( lgpos );
    csrOE.Cpage().SetLgposModify( lgpos );
    csrAE.Cpage().SetLgposModify( lgpos );

    csrFDP.FinalizePreInitPage();
    csrOE.FinalizePreInitPage();
    csrAE.FinalizePreInitPage();
    SPIPerformCreateMultiple( pfucb, &csrFDP, &csrOE, &csrAE, pgnoPrimary, sph );

HandleError:
    csrAE.ReleasePage();
    csrOE.ReleasePage();
    csrFDP.ReleasePage();
    return err;
}


INLINE ERR ErrSPICreateMultiple(
    FUCB        *pfucb,
    const PGNO  pgnoParent,
    const PGNO  pgnoFDP,
    const OBJID objidFDP,
    const CPG   cpgPrimary,
    const ULONG fPageFlags,
    const BOOL  fUnique )
{
    return ErrSPCreateMultiple(
            pfucb,
            pgnoParent,
            pgnoFDP,
            objidFDP,
            pgnoFDP + 1,
            pgnoFDP + 2,
            pgnoFDP + cpgPrimary - 1,
            cpgPrimary,
            fUnique,
            fPageFlags );
}

ERR ErrSPICreateSingle(
    FUCB            *pfucb,
    CSR             *pcsr,
    const PGNO      pgnoParent,
    const PGNO      pgnoFDP,
    const OBJID     objidFDP,
    CPG             cpgPrimary,
    const BOOL      fUnique,
    const ULONG     fPageFlags,
    const DBTIME    dbtime )
{
    ERR             err;
    SPACE_HEADER    sph;
    const BOOL      fRedo = ( PinstFromPfucb( pfucb )->m_plog->FRecoveringMode() == fRecoveringRedo );

    Assert( ( !fRedo && ( dbtime == dbtimeNil ) ) ||
            ( fRedo && ( dbtime != dbtimeNil ) ) ||
            ( fRedo && ( dbtime == dbtimeNil ) && g_rgfmp[pfucb->ifmp].FCreatingDB() && !g_rgfmp[pfucb->ifmp].FLogOn() ) );

    sph.SetPgnoParent( pgnoParent );
    sph.SetCpgPrimary( cpgPrimary );

    Assert( sph.FSingleExtent() );
    Assert( sph.FUnique() );

    Assert( !( fPageFlags & CPAGE::fPageRoot ) );
    Assert( !( fPageFlags & CPAGE::fPageLeaf ) );
    Assert( !( fPageFlags & CPAGE::fPageParentOfLeaf ) );
    Assert( !( fPageFlags & CPAGE::fPageEmpty ) );
    Assert( !( fPageFlags & CPAGE::fPagePreInit ) );
    Assert( !( fPageFlags & CPAGE::fPageSpaceTree) );

    if ( !fUnique )
        sph.SetNonUnique();

    Assert( cpgPrimary > 0 );
    if ( cpgPrimary > cpgSmallSpaceAvailMost )
    {
        sph.SetRgbitAvail( 0xffffffff );
    }
    else
    {
        sph.SetRgbitAvail( 0 );
        while ( --cpgPrimary > 0 )
        {
            sph.SetRgbitAvail( ( sph.RgbitAvail() << 1 ) + 1 );
        }
    }

    Assert( objidFDP == ObjidFDP( pfucb ) );
    Assert( objidFDP != 0 );

    const ULONG fInitPageFlags = fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf;
    if ( !fRedo || ( dbtime == dbtimeNil ) )
    {
        Call( pcsr->ErrGetNewPreInitPage(
            pfucb->ppib,
            pfucb->ifmp,
            pgnoFDP,
            objidFDP,
            !fRedo && g_rgfmp[pfucb->ifmp].FLogOn() ) );
        pcsr->ConsumePreInitPage( fInitPageFlags );

        if ( !fRedo )
        {
            LGPOS lgpos;
            Call( ErrLGCreateSingleExtentFDP( pfucb, pcsr, &sph, fPageFlags, &lgpos ) );
            pcsr->Cpage().SetLgposModify( lgpos );
        }
    }
    else
    {
        Call( pcsr->ErrGetNewPreInitPageForRedo(
                pfucb->ppib,
                pfucb->ifmp,
                pgnoFDP,
                objidFDP,
                dbtime ) );
        pcsr->ConsumePreInitPage( fInitPageFlags );
    }

    Assert( pcsr->FDirty() );
    SPIInitPgnoFDP( pfucb, pcsr, sph  );

    pcsr->FinalizePreInitPage();

    BTUp( pfucb );

HandleError:
    return err;
}


ERR ErrSPCreate(
    PIB             *ppib,
    const IFMP      ifmp,
    const PGNO      pgnoParent,
    const PGNO      pgnoFDP,
    const CPG       cpgPrimary,
    const ULONG     fSPFlags,
    const ULONG     fPageFlags,
    OBJID           *pobjidFDP )
{
    ERR             err;
    FUCB            *pfucb = pfucbNil;
    const BOOL      fUnique = !( fSPFlags & fSPNonUnique );

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortSpace );

    Assert( NULL != pobjidFDP );
    Assert( !( fPageFlags & CPAGE::fPageRoot ) );
    Assert( !( fPageFlags & CPAGE::fPageLeaf ) );
    Assert( !( fPageFlags & CPAGE::fPageParentOfLeaf ) );
    Assert( !( fPageFlags & CPAGE::fPageEmpty ) );
    Assert( !( fPageFlags & CPAGE::fPageSpaceTree ) );

    if ( pgnoFDP == pgnoSystemRoot &&
            pgnoParent == pgnoNull &&
            g_rgfmp[ifmp].FLogOn() &&
            !PinstFromIfmp( ifmp )->m_plog->FRecovering() )
    {
#ifdef DEBUG
        QWORD cbSize = 0;
        if ( g_rgfmp[ifmp].Pfapi()->ErrSize( &cbSize, IFileAPI::filesizeLogical ) >= JET_errSuccess )
        {
            Assert( ( cbSize / g_cbPage - cpgDBReserved ) == cpgPrimary );
        }
#endif
        Assert( cpgPrimary >= CpgDBDatabaseMinMin() ||
                FFMPIsTempDB( ifmp ) );

        LGPOS lgposExtend;
        Call( ErrLGExtendDatabase( PinstFromIfmp( ifmp )->m_plog, ifmp, cpgPrimary, &lgposExtend ) );
        Call( ErrIOResizeUpdateDbHdrCount( ifmp, fTrue  ) );
        if ( ( g_rgfmp[ ifmp ].ErrDBFormatFeatureEnabled( JET_efvLgposLastResize ) == JET_errSuccess ) )
        {
            Expected( fFalse );
            Call( ErrIOResizeUpdateDbHdrLgposLast( ifmp, lgposExtend ) );
        }
    }

    CallR( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb, openNew ) );

    tcScope->nParentObjectClass = TceFromFUCB( pfucb );

    FCB *pfcb   = pfucb->u.pfcb;
    Assert( pfcbNil != pfcb );

    if ( pgnoSystemRoot == pgnoFDP )
    {
        Assert( pfcb->FInitialized() );
        Assert( pfcb->FTypeDatabase() );
    }
    else
    {
        Assert( !pfcb->FInitialized() );
        Assert( pfcb->FTypeNull() );
    }

    Assert( pfcb->FUnique() );
    if ( !fUnique )
    {
        pfcb->Lock();
        pfcb->SetNonUnique();
        pfcb->Unlock();
    }


    if ( g_fRepair && pgnoSystemRoot == pgnoFDP )
    {
        *pobjidFDP = objidSystemRoot;
    }
    else
    {
        Call( g_rgfmp[ pfucb->ifmp ].ErrObjidLastIncrementAndGet( pobjidFDP ) );
    }
    Assert( pgnoSystemRoot != pgnoFDP || objidSystemRoot == *pobjidFDP );


    Assert( objidNil == pfcb->ObjidFDP() || g_fRepair );
    pfcb->SetObjidFDP( *pobjidFDP );
    tcScope->SetDwEngineObjid( *pobjidFDP );

    Assert( !pfcb->FSpaceInitialized() || g_fRepair );
    pfcb->Lock();
    pfcb->SetSpaceInitialized();
    pfcb->Unlock();

    if ( fSPFlags & fSPMultipleExtent )
    {
        Assert( PgnoFDP( pfucb ) == pgnoFDP );
        pfcb->SetPgnoOE( pgnoFDP + 1 );
        pfcb->SetPgnoAE( pgnoFDP + 2 );
        err = ErrSPICreateMultiple(
                    pfucb,
                    pgnoParent,
                    pgnoFDP,
                    *pobjidFDP,
                    cpgPrimary,
                    fPageFlags,
                    fUnique );
    }
    else
    {
        Assert( PgnoFDP( pfucb ) == pgnoFDP );
        pfcb->SetPgnoOE( pgnoNull );
        pfcb->SetPgnoAE( pgnoNull );
        err = ErrSPICreateSingle(
                    pfucb,
                    Pcsr( pfucb ),
                    pgnoParent,
                    pgnoFDP,
                    *pobjidFDP,
                    cpgPrimary,
                    fUnique,
                    fPageFlags,
                    dbtimeNil );
    }

    Assert( !FFUCBSpace( pfucb ) );
    Assert( !FFUCBVersioned( pfucb ) );

HandleError:
    Assert( ( pfucb != pfucbNil ) || ( err < JET_errSuccess ) );

    if ( pfucb != pfucbNil )
    {
        BTClose( pfucb );
    }

    return err;
}


VOID SPIConvertCalcExtents(
    const SPACE_HEADER& sph,
    const PGNO          pgnoFDP,
    EXTENTINFO          *rgext,
    INT                 *pcext )
{
    PGNO                pgno;
    UINT                rgbit       = 0x80000000;
    INT                 iextMac     = 0;

    if ( sph.CpgPrimary() - 1 > cpgSmallSpaceAvailMost )
    {
        pgno = pgnoFDP + sph.CpgPrimary() - 1;
        rgext[iextMac].pgnoLastInExtent = pgno;
        rgext[iextMac].cpgExtent = sph.CpgPrimary() - cpgSmallSpaceAvailMost - 1;
        Assert( rgext[iextMac].FValid() );
        pgno -= rgext[iextMac].CpgExtent();
        iextMac++;

        Assert( pgnoFDP + cpgSmallSpaceAvailMost == pgno );
        Assert( 0x80000000 == rgbit );
    }
    else
    {
        pgno = pgnoFDP + cpgSmallSpaceAvailMost;
        while ( 0 != rgbit && 0 == iextMac )
        {
            Assert( pgno > pgnoFDP );
            if ( rgbit & sph.RgbitAvail() )
            {
                Assert( pgno <= pgnoFDP + sph.CpgPrimary() - 1 );
                rgext[iextMac].pgnoLastInExtent = pgno;
                rgext[iextMac].cpgExtent = 1;
                Assert( rgext[iextMac].FValid() );
                iextMac++;
            }

            pgno--;
            rgbit >>= 1;
        }
    }

    Assert( ( 0 == iextMac && 0 == rgbit )
        || 1 == iextMac );

    for ( ; rgbit != 0; pgno--, rgbit >>= 1 )
    {
        Assert( pgno > pgnoFDP );
        if ( rgbit & sph.RgbitAvail() )
        {
            const INT   iextPrev    = iextMac - 1;
            Assert( rgext[iextPrev].CpgExtent() > 0 );
            Assert( (ULONG)rgext[iextPrev].CpgExtent() <= rgext[iextPrev].PgnoLast() );
            Assert( rgext[iextPrev].PgnoLast() <= pgnoFDP + sph.CpgPrimary() - 1 );

            const PGNO  pgnoFirst = rgext[iextPrev].PgnoLast() - rgext[iextPrev].CpgExtent() + 1;
            Assert( pgnoFirst > pgnoFDP );

            if ( pgnoFirst - 1 == pgno )
            {
                Assert( pgnoFirst - 1 > pgnoFDP );
                rgext[iextPrev].cpgExtent++;
                Assert( rgext[iextPrev].FValid() );
            }
            else
            {
                rgext[iextMac].pgnoLastInExtent = pgno;
                rgext[iextMac].cpgExtent = 1;
                Assert( rgext[iextMac].FValid() );
                iextMac++;
            }
        }
    }

    *pcext = iextMac;
}


LOCAL VOID SPIConvertUpdateOE( FUCB *pfucb, CSR *pcsrOE, const SPACE_HEADER& sph, PGNO pgnoSecondaryFirst, CPG cpgSecondary )
{
    if ( !FBTIUpdatablePage( *pcsrOE ) )
    {
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() );
        return;
    }

    Assert( pcsrOE->Latch() == latchWrite );
    Assert( !FFUCBOwnExt( pfucb ) );
    FUCBSetOwnExt( pfucb );

    SPIInitSplitBuffer( pfucb, pcsrOE );

    Assert( 0 == pcsrOE->Cpage().Clines() );
    Assert( cpgSecondary != 0 );
    pcsrOE->SetILine( 0 );

    const CSPExtentNodeKDF      spextSecondary( SPEXTKEY::fSPExtentTypeOE, pgnoSecondaryFirst + cpgSecondary - 1, cpgSecondary );
    NDInsert( pfucb, pcsrOE, spextSecondary.Pkdf() );

    const CSPExtentNodeKDF      spextPrimary( SPEXTKEY::fSPExtentTypeOE, PgnoFDP( pfucb ) + sph.CpgPrimary() - 1, sph.CpgPrimary() );
    const BOOKMARK          bm = spextPrimary.GetBm( pfucb );
    const ERR               err = ErrNDSeek( pfucb, pcsrOE, bm );
    AssertRTL( wrnNDFoundLess == err || wrnNDFoundGreater == err );
    if ( wrnNDFoundLess == err )
    {
        Assert( pcsrOE->Cpage().Clines() - 1 == pcsrOE->ILine() );
        pcsrOE->IncrementILine();
    }
    NDInsert( pfucb, pcsrOE, spextPrimary.Pkdf() );

    FUCBResetOwnExt( pfucb );
}


LOCAL VOID SPIConvertUpdateAE(
    FUCB            *pfucb,
    CSR             *pcsrAE,
    EXTENTINFO      *rgext,
    INT             iextMac,
    PGNO            pgnoSecondaryFirst,
    CPG             cpgSecondary )
{
    if ( !FBTIUpdatablePage( *pcsrAE ) )
    {
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() );
        return;
    }

    
    Assert( iextMac >= 0 );
    if ( iextMac < 0 )
    {
        return;
    }

    Assert( pcsrAE->Latch() == latchWrite );
    Assert( !FFUCBAvailExt( pfucb ) );
    FUCBSetAvailExt( pfucb );

    SPIInitSplitBuffer( pfucb, pcsrAE );

    Assert( cpgSecondary >= 2 );
    Assert( 0 == pcsrAE->Cpage().Clines() );
    pcsrAE->SetILine( 0 );

    CPG     cpgExtent = cpgSecondary - 1 - 1;
    PGNO    pgnoLast = pgnoSecondaryFirst + cpgSecondary - 1;
    if ( cpgExtent != 0 )
    {

        const CSPExtentNodeKDF      spavailextSecondary( SPEXTKEY::fSPExtentTypeAE, pgnoLast, cpgExtent );
        NDInsert( pfucb, pcsrAE, spavailextSecondary.Pkdf() );
    }

    Assert( latchWrite == pcsrAE->Latch() );

    for ( INT iext = iextMac - 1; iext >= 0; iext-- )
    {


        Assert( rgext[iext].CpgExtent() > 0 );

#ifdef DEBUG
        if ( iext > 0 )
        {
            Assert( rgext[iext].PgnoLast() < rgext[iext-1].PgnoLast() );
        }
#endif

        AssertTrack( rgext[iext].FValid(), "SPConvertInvalidExtInfo" );
        const CSPExtentNodeKDF      spavailextCurr( SPEXTKEY::fSPExtentTypeAE, rgext[iext].PgnoLast(), rgext[iext].CpgExtent() );

        if ( 0 < pcsrAE->Cpage().Clines() )
        {
            const BOOKMARK  bm = spavailextCurr.GetBm( pfucb );
            ERR         err;
            err = ErrNDSeek( pfucb, pcsrAE, bm );
            AssertRTL( wrnNDFoundLess == err || wrnNDFoundGreater == err );
            if ( wrnNDFoundLess == err )
                pcsrAE->IncrementILine();
        }

        NDInsert( pfucb, pcsrAE, spavailextCurr.Pkdf() );
    }

    FUCBResetAvailExt( pfucb );
    Assert( pcsrAE->Latch() == latchWrite );
}


INLINE VOID SPIConvertUpdateFDP( FUCB *pfucb, CSR *pcsrRoot, SPACE_HEADER *psph )
{
    if ( FBTIUpdatablePage( *pcsrRoot ) )
    {
        DATA    data;

        Assert( latchWrite == pcsrRoot->Latch() );
        Assert( pcsrRoot->FDirty() );

        data.SetPv( psph );
        data.SetCb( sizeof( *psph ) );
        NDSetExternalHeader( pfucb, pcsrRoot, noderfSpaceHeader, &data );
    }
    else
    {
        Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() );
    }
}


VOID SPIPerformConvert( FUCB            *pfucb,
                        CSR             *pcsrRoot,
                        CSR             *pcsrAE,
                        CSR             *pcsrOE,
                        SPACE_HEADER    *psph,
                        PGNO            pgnoSecondaryFirst,
                        CPG             cpgSecondary,
                        EXTENTINFO      *rgext,
                        INT             iextMac )
{
    SPIConvertUpdateOE( pfucb, pcsrOE, *psph, pgnoSecondaryFirst, cpgSecondary );
    SPIConvertUpdateAE( pfucb, pcsrAE, rgext, iextMac, pgnoSecondaryFirst, cpgSecondary );
    SPIConvertUpdateFDP( pfucb, pcsrRoot, psph );
}


VOID SPIConvertGetExtentinfo( FUCB          *pfucb,
                              CSR           *pcsrRoot,
                              SPACE_HEADER  *psph,
                              EXTENTINFO    *rgext,
                              INT           *piextMac )
{

    NDGetExternalHeader( pfucb, pcsrRoot, noderfSpaceHeader );
    Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );

    UtilMemCpy( psph, pfucb->kdfCurr.data.Pv(), sizeof(SPACE_HEADER) );

    SPIConvertCalcExtents( *psph, PgnoFDP( pfucb ), rgext, piextMac );
}


LOCAL ERR ErrSPIConvertToMultipleExtent( FUCB *pfucb, CPG cpgReq, CPG cpgMin, BOOL fUseSpaceHints = fTrue )
{
    ERR             err;
    SPACE_HEADER    sph;
    PGNO            pgnoSecondaryFirst  = pgnoNull;
    INT             iextMac             = 0;
    UINT            rgbitAvail;
    FUCB            *pfucbT;
    CSR             csrOE;
    CSR             csrAE;
    CSR             *pcsrRoot           = pfucb->pcsrRoot;
    DBTIME          dbtimeBefore        = dbtimeNil;
    ULONG           fFlagsBefore        = 0;
    LGPOS           lgpos               = lgposMin;
    EXTENTINFO      rgextT[(cpgSmallSpaceAvailMost+1)/2 + 1];

    Assert( NULL != pcsrRoot );
    Assert( latchRIW == pcsrRoot->Latch() );
    AssertSPIPfucbOnRoot( pfucb );
    Assert( !FFUCBSpace( pfucb ) );
    Assert( ObjidFDP( pfucb ) != objidNil );
    Assert( pfucb->u.pfcb->FSpaceInitialized() );

    const ULONG fPageFlags = pcsrRoot->Cpage().FFlags()
                                & ~CPAGE::fPageRepair
                                & ~CPAGE::fPageRoot
                                & ~CPAGE::fPageLeaf
                                & ~CPAGE::fPageParentOfLeaf;

    SPIConvertGetExtentinfo( pfucb, pcsrRoot, &sph, rgextT, &iextMac );
    Assert( sph.FSingleExtent() );

    OSTraceFMP( pfucb->ifmp, JET_tracetagSpaceInternal,
                OSFormat( "ConvertSMALLToMultiExt(%d.%d)-> ", pfucb->u.pfcb->PgnoFDP(), sph.CpgPrimary() ) );

    rgbitAvail = sph.RgbitAvail();

    CallR( ErrBTOpen( pfucb->ppib, pfucb->u.pfcb, &pfucbT, fFalse ) );

    cpgMin += cpgMultipleExtentConvert;
    cpgReq = max( cpgMin, cpgReq );

    if ( fUseSpaceHints )
    {
        CPG cpgNext = CpgSPIGetNextAlloc( pfucb->u.pfcb->Pfcbspacehints(), sph.CpgPrimary() );
        cpgReq = max( cpgNext, cpgReq );
    }

    Assert( sph.PgnoParent() != pgnoNull );
    Call( ErrSPGetExt( pfucbT, sph.PgnoParent(), &cpgReq, cpgMin, &pgnoSecondaryFirst ) );
    Assert( cpgReq >= cpgMin );
    Assert( pgnoSecondaryFirst != pgnoNull );

    Assert( pgnoSecondaryFirst != pgnoNull );
    sph.SetMultipleExtent();
    sph.SetPgnoOE( pgnoSecondaryFirst );

    Assert( !FFUCBVersioned( pfucbT ) );
    Assert( !FFUCBSpace( pfucbT ) );

    const BOOL fLogging = !pfucb->u.pfcb->FDontLogSpaceOps() && g_rgfmp[pfucb->ifmp].FLogOn();

    Call( csrOE.ErrGetNewPreInitPage( pfucbT->ppib,
                                      pfucbT->ifmp,
                                      sph.PgnoOE(),
                                      ObjidFDP( pfucbT ),
                                      fLogging ) );
    csrOE.ConsumePreInitPage( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree );
    Assert( latchWrite == csrOE.Latch() );

    Call( csrAE.ErrGetNewPreInitPage( pfucbT->ppib,
                                      pfucbT->ifmp,
                                      sph.PgnoAE(),
                                      ObjidFDP( pfucbT ),
                                      fLogging ) );
    csrAE.ConsumePreInitPage( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree );
    Assert( latchWrite == csrAE.Latch() );

    SPIUpgradeToWriteLatch( pfucb );

    dbtimeBefore = pfucb->pcsrRoot->Dbtime();
    fFlagsBefore = pfucb->pcsrRoot->Cpage().FFlags();
    Assert ( dbtimeNil != dbtimeBefore );

    SPIDirtyAndSetMaxDbtime( pfucb->pcsrRoot, &csrOE, &csrAE );

    if ( pfucb->pcsrRoot->Dbtime() > dbtimeBefore )
    {
        if ( !fLogging )
        {
            Assert( 0 == CmpLgpos( lgpos, lgposMin ) );
            err = JET_errSuccess;
        }
        else
        {
            err = ErrLGConvertFDP(
                        pfucb,
                        pcsrRoot,
                        &sph,
                        pgnoSecondaryFirst,
                        cpgReq,
                        dbtimeBefore,
                        rgbitAvail,
                        fPageFlags,
                        &lgpos );
        }
    }
    else
    {
        FireWall( "ConvertToMultiExtDbTimeTooLow" );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucb ), HaDbFailureTagCorruption, L"b4c2850e-04f0-4476-b967-2420fdcb6663" );
        err = ErrERRCheck( JET_errDbTimeCorrupted );
    }

    if ( err < JET_errSuccess )
    {
        pfucb->pcsrRoot->RevertDbtime( dbtimeBefore, fFlagsBefore );
    }
    Call ( err );

    pfucb->u.pfcb->SetPgnoOE( sph.PgnoOE() );
    pfucb->u.pfcb->SetPgnoAE( sph.PgnoAE() );
    Assert( pgnoNull != PgnoAE( pfucb ) );
    Assert( pgnoNull != PgnoOE( pfucb ) );

    pcsrRoot->Cpage().SetLgposModify( lgpos );
    csrOE.Cpage().SetLgposModify( lgpos );
    csrAE.Cpage().SetLgposModify( lgpos );

    SPIPerformConvert(
            pfucbT,
            pcsrRoot,
            &csrAE,
            &csrOE,
            &sph,
            pgnoSecondaryFirst,
            cpgReq,
            rgextT,
            iextMac );

    csrOE.FinalizePreInitPage();
    csrAE.FinalizePreInitPage();

    AssertSPIPfucbOnRoot( pfucb );

    Assert( pfucb->pcsrRoot->Latch() == latchWrite );

HandleError:
    Assert( err != JET_errKeyDuplicate );
    csrAE.ReleasePage();
    csrOE.ReleasePage();
    Assert( pfucbT != pfucbNil );
    BTClose( pfucbT );
    return err;
}


ERR ErrSPBurstSpaceTrees( FUCB *pfucb )
{
    ERR err = JET_errSuccess;

    Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
    pfucb->pcsrRoot = Pcsr( pfucb );

    Call( ErrSPIConvertToMultipleExtent(
            pfucb,
            0,
            0,
            fFalse ) );

HandleError:
    BTUp( pfucb );
    pfucb->pcsrRoot = pcsrNil;

    return err;
}


INLINE BOOL FSPIAllocateAllAvail(
    const CSPExtentInfo *   pcspaei,
    const CPG               cpgReq,
    const LONG              lPageFragment,
    const PGNO              pgnoFDP,
    const CPG               cpgDb,
    const BOOL              fSPFlags )
{
    BOOL fAllocateAllAvail  = fTrue;


    if ( pcspaei->CpgExtent() > cpgReq )
    {
        if ( BoolSetFlag( fSPFlags, fSPExactExtent ) )
        {
            fAllocateAllAvail = fFalse;
        }
        else if ( pgnoSystemRoot == pgnoFDP )
        {
            if ( BoolSetFlag( fSPFlags, fSPNewFDP ) && cpgDb < cpgSmallDbSpaceSize && pcspaei->PgnoFirst() < pgnoSmallDbSpaceStart )
            {
                fAllocateAllAvail = fFalse;
            }
            if ( pcspaei->CpgExtent() >= cpgReq + cpgSmallGrow )
            {
                fAllocateAllAvail = fFalse;
            }
        }
        else if ( cpgReq < lPageFragment || pcspaei->CpgExtent() >= cpgReq + lPageFragment )
        {
            fAllocateAllAvail = fFalse;
        }
    }

    return fAllocateAllAvail;
}



LOCAL INLINE ERR ErrSPIFindExt(
    __inout     FUCB * pfucb,
    __in const  PGNO pgno,
    __in const  SpacePool sppAvailPool,
    __in const  SPEXTKEY::E_SP_EXTENT_TYPE eExtType,
    __out       CSPExtentInfo * pspei )
{
    Assert( ( eExtType == SPEXTKEY::fSPExtentTypeOE ) || ( eExtType == SPEXTKEY::fSPExtentTypeAE ) );

    ERR err;
    DIB dib;

    OnDebug( const BOOL fOwnExt = eExtType == SPEXTKEY::fSPExtentTypeOE );
    Assert( fOwnExt ? pfucb->fOwnExt : pfucb->fAvailExt );
    Assert( ( sppAvailPool == spp::AvailExtLegacyGeneralPool ) || !fOwnExt );

    CSPExtentKeyBM spextkeyFind( eExtType, pgno, sppAvailPool );
    dib.pos = posDown;
    dib.pbm = spextkeyFind.Pbm( pfucb ) ;
    dib.dirflag = fDIRFavourNext;
    Call( ErrBTDown( pfucb, &dib, latchReadTouch ) );
    if ( wrnNDFoundLess == err )
    {
        Assert( Pcsr( pfucb )->Cpage().Clines() - 1 ==
                    Pcsr( pfucb )->ILine() );
        Call( ErrBTNext( pfucb, fDIRNull ) );
    }

    pspei->Set( pfucb );

    ASSERT_VALID( pspei );

    Call( pspei->ErrCheckCorrupted() );

    if ( pspei->SppPool() != sppAvailPool )
    {
        Error( ErrERRCheck( JET_errRecordNotFound ) );
    }

    err = JET_errSuccess;

HandleError:
    if ( err < JET_errSuccess )
    {
        pspei->Unset();
    }

    return err;
}


LOCAL ERR ErrSPIFindExtOE(
    __inout     FUCB * pfucbOE,
    __in const  PGNO pgnoFirst,
    __out       CSPExtentInfo * pcspoext
    )
{
    return ErrSPIFindExt( pfucbOE, pgnoFirst, spp::AvailExtLegacyGeneralPool, SPEXTKEY::fSPExtentTypeOE, pcspoext );
}


LOCAL ERR ErrSPIFindExtAE(
    __inout     FUCB * pfucbAE,
    __in const  PGNO pgnoFirst,
    __in const  SpacePool sppAvailPool,
    __out       CSPExtentInfo * pcspaext
    )
{
    Expected( ( sppAvailPool != spp::ShelvedPool ) || ( pfucbAE->u.pfcb->PgnoFDP() == pgnoSystemRoot ) );
    return ErrSPIFindExt( pfucbAE, pgnoFirst, sppAvailPool, SPEXTKEY::fSPExtentTypeAE, pcspaext );
}


LOCAL INLINE ERR ErrSPIFindExtOE(
    __inout     PIB *           ppib,
    __in        FCB *           pfcb,
    __in const  PGNO            pgnoFirst,
    __out       CSPExtentInfo * pcspoext )
{
    ERR         err = JET_errSuccess;
    FUCB        *pfucbOE;

    CallR( ErrSPIOpenOwnExt( ppib, pfcb, &pfucbOE ) );
    Assert( pfcb == pfucbOE->u.pfcb );

    Call( ErrSPIFindExtOE( pfucbOE, pgnoFirst, pcspoext ) );
    ASSERT_VALID( pcspoext );

HandleError:
    BTClose( pfucbOE );

    return err;
}


LOCAL INLINE ERR ErrSPIFindExtAE(
    __inout     PIB *           ppib,
    __in        FCB *           pfcb,
    __in const  PGNO            pgnoFirst,
    __in const  SpacePool       sppAvailPool,
    __out       CSPExtentInfo * pcspaext )
{
    ERR         err = JET_errSuccess;
    FUCB        *pfucbAE;

    CallR( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbAE ) );
    Assert( pfcb == pfucbAE->u.pfcb );

    Call( ErrSPIFindExtAE( pfucbAE, pgnoFirst, sppAvailPool, pcspaext ) );
    ASSERT_VALID( pcspaext );

HandleError:
    BTClose( pfucbAE );

    return err;
}

template< class T >
inline SpaceCategoryFlags& operator|=( SpaceCategoryFlags& spcatf, const T& t )
{
    spcatf = SpaceCategoryFlags( spcatf | (SpaceCategoryFlags)t );
    return spcatf;
}

BOOL FSPSpaceCatStrictlyLeaf( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfStrictlyLeaf ) != 0;
}

BOOL FSPSpaceCatStrictlyInternal( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfStrictlyInternal ) != 0;
}

BOOL FSPSpaceCatRoot( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfRoot ) != 0;
}

BOOL FSPSpaceCatSplitBuffer( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfSplitBuffer ) != 0;
}

BOOL FSPSpaceCatSmallSpace( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfSmallSpace ) != 0;
}

BOOL FSPSpaceCatSpaceOE( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfSpaceOE ) != 0;
}

BOOL FSPSpaceCatSpaceAE( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfSpaceAE ) != 0;
}

BOOL FSPSpaceCatAnySpaceTree( const SpaceCategoryFlags spcatf )
{
    return FSPSpaceCatSpaceOE( spcatf ) || FSPSpaceCatSpaceAE( spcatf );
}

BOOL FSPSpaceCatAvailable( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfAvailable ) != 0;
}

BOOL FSPSpaceCatIndeterminate( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfIndeterminate ) != 0;
}

BOOL FSPSpaceCatInconsistent( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfInconsistent ) != 0;
}

BOOL FSPSpaceCatLeaked( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfLeaked ) != 0;
}

BOOL FSPSpaceCatNotOwned( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfNotOwned ) != 0;
}

BOOL FSPSpaceCatNotOwnedEof( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfNotOwnedEof ) != 0;
}

BOOL FSPSpaceCatUnknown( const SpaceCategoryFlags spcatf )
{
    return ( spcatf & spcatfUnknown ) != 0;
}

BOOL FSPValidSpaceCategoryFlags( const SpaceCategoryFlags spcatf )
{
    if ( spcatf == spcatfNone )
    {
        return fFalse;
    }

    if ( FSPSpaceCatUnknown( spcatf ) && ( spcatf != spcatfUnknown ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatIndeterminate( spcatf ) && ( spcatf != spcatfIndeterminate ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatInconsistent( spcatf ) && ( spcatf != spcatfInconsistent ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatLeaked( spcatf ) && ( spcatf != spcatfLeaked ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatNotOwned( spcatf ) && ( spcatf != spcatfNotOwned ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatNotOwnedEof( spcatf ) && ( spcatf != spcatfNotOwnedEof ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatAvailable( spcatf ) &&
            ( FSPSpaceCatStrictlyLeaf( spcatf ) || FSPSpaceCatStrictlyInternal( spcatf ) || FSPSpaceCatRoot( spcatf ) ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatSpaceOE( spcatf ) && FSPSpaceCatSpaceAE( spcatf ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatSplitBuffer( spcatf ) &&
        ( !FSPSpaceCatAnySpaceTree( spcatf ) || !FSPSpaceCatAvailable( spcatf ) ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatAvailable( spcatf ) && !FSPSpaceCatSplitBuffer( spcatf ) && FSPSpaceCatAnySpaceTree( spcatf ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatAvailable( spcatf ) &&
            !FSPSpaceCatSplitBuffer( spcatf ) &&
            !FSPSpaceCatAnySpaceTree( spcatf ) &&
            !FSPSpaceCatSmallSpace( spcatf ) &&
            ( spcatf != spcatfAvailable ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatRoot( spcatf ) && ( FSPSpaceCatStrictlyLeaf( spcatf ) || FSPSpaceCatStrictlyInternal( spcatf ) ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatStrictlyLeaf( spcatf ) && ( FSPSpaceCatRoot( spcatf ) || FSPSpaceCatStrictlyInternal( spcatf ) ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatStrictlyInternal( spcatf ) && ( FSPSpaceCatRoot( spcatf ) || FSPSpaceCatStrictlyLeaf( spcatf ) ) )
    {
        return fFalse;
    }

    if ( FSPSpaceCatSmallSpace( spcatf ) && FSPSpaceCatAnySpaceTree( spcatf ) )
    {
        return fFalse;
    }

    return fTrue;
}

VOID SPFreeSpaceCatCtx( _Inout_ SpaceCatCtx** const ppSpCatCtx )
{
    Assert( ppSpCatCtx != NULL );
    SpaceCatCtx* const pSpCatCtx = *ppSpCatCtx;
    if ( pSpCatCtx == NULL )
    {
        return;
    }

    if ( pSpCatCtx->pfucbSpace != pfucbNil )
    {
        BTUp( pSpCatCtx->pfucbSpace );
        BTClose( pSpCatCtx->pfucbSpace );
        pSpCatCtx->pfucbSpace = pfucbNil;
    }

    Assert( ( pSpCatCtx->pfucb != pfucbNil ) || ( pSpCatCtx->pfucbParent == pfucbNil ) );
    CATFreeCursorsFromObjid( pSpCatCtx->pfucb, pSpCatCtx->pfucbParent );

    pSpCatCtx->pfucbParent = pfucbNil;
    pSpCatCtx->pfucb = pfucbNil;

    if ( pSpCatCtx->pbm != NULL )
    {
        delete pSpCatCtx->pbm;
        pSpCatCtx->pbm = NULL;
    }

    delete pSpCatCtx;

    *ppSpCatCtx = NULL;
}

ERR ErrSPIGetObjidFromPage(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _Out_ OBJID* const pobjid )
{
    CPAGE cpage;
    ERR err = cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfUninitPageOk );

    if ( err < JET_errSuccess )
    {
        *pobjid = objidNil;
    }
    else
    {
        *pobjid = cpage.ObjidFDP();
    }
    Call( err );
    cpage.ReleaseReadLatch();

HandleError:
    return err;
}

ERR ErrSPIGetSpaceCategoryObject(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _In_ const OBJID objid,
    _In_ const OBJID objidParent,
    _In_ const SYSOBJ sysobj,
    _Out_ SpaceCategoryFlags* const pspcatf,
    _Out_ BOOL* const pfOwned,
    _Out_ SpaceCatCtx** const ppSpCatCtx )
{
    ERR err = JET_errSuccess;
    BOOL fOwned = fFalse;
    PGNO pgnoFDPParent = pgnoNull;
    PGNO pgnoFDP = pgnoNull;
    FUCB* pfucbParent = pfucbNil;
    FUCB* pfucb = pfucbNil;
    FUCB* pfucbSpace = pfucbNil;
    FCB* pfcb = pfcbNil;
    SpaceCategoryFlags spcatf = spcatfNone;
    CPAGE cpage;
    BOOL fPageLatched = fFalse;
    KEYDATAFLAGS kdf;
    SpaceCatCtx* pSpCatCtx = NULL;
    BOOKMARK_COPY* pbm = NULL;

    Assert( objid != objidNil );
    Assert( objid != objidParent );
    Assert( ( sysobj == sysobjNil ) || ( sysobj == sysobjTable ) || ( sysobj == sysobjIndex ) || ( sysobj == sysobjLongValue ) );
    Assert( ( objidParent != objidNil ) != ( objid == objidSystemRoot ) );
    Assert( ( sysobj != sysobjNil ) != ( objid == objidSystemRoot ) );
    Assert( ( sysobj == sysobjTable ) == ( objidParent == objidSystemRoot ) );
    Assert( pspcatf != NULL );
    Assert( pfOwned != NULL );
    Assert( ppSpCatCtx != NULL );

    *pspcatf = spcatfUnknown;
    *pfOwned = fFalse;
    *ppSpCatCtx = NULL;

    Alloc( pSpCatCtx = new SpaceCatCtx );
    Alloc( pbm = new BOOKMARK_COPY );



    Call( ErrCATGetCursorsFromObjid(
            ppib,
            ifmp,
            objid,
            objidParent,
            sysobj,
            &pgnoFDP,
            &pgnoFDPParent,
            &pfucb,
            &pfucbParent ) );

    pfcb = pfucb->u.pfcb;

#ifdef DEBUG
    if ( pfucbParent != pfucbNil )
    {
        Assert( objid != objidSystemRoot );
        FCB* const pfcbParent = pfucbParent->u.pfcb;
        Assert( pfcbParent != pfcbNil );
        Assert( pfcbParent->FInitialized() );
        Assert( PgnoFDP( pfucbParent ) == pgnoFDPParent );
    }
    else
    {
        Assert( objid == objidSystemRoot );
    }
#endif

    Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
    pfucb->pcsrRoot = Pcsr( pfucb );


    if ( FSPIIsSmall( pfcb ) )
    {
        Expected( objid != objidSystemRoot );

        const SPACE_HEADER* const psph = PsphSPIRootPage( pfucb );
        Assert( psph->FSingleExtent() );

        const PGNO pgnoFirst = PgnoFDP( pfucb );
        const PGNO pgnoLast = pgnoFirst + psph->CpgPrimary() - 1;
        if ( ( pgno < pgnoFirst ) || ( pgno > pgnoLast ) )
        {
            spcatf = spcatfUnknown;
            goto HandleError;
        }

        fOwned = fTrue;
        spcatf |= spcatfSmallSpace;

        C_ASSERT( cpgSmallSpaceAvailMost <= ( 8 * sizeof( psph->RgbitAvail() ) ) );
        if (
             ( pgno > pgnoFirst ) &&
             (
               ( ( pgno - pgnoFirst ) > cpgSmallSpaceAvailMost ) ||

               ( psph->RgbitAvail() & ( (UINT)0x1 << ( pgno - pgnoFirst - 1 ) ) )
             )
           )
        {
            spcatf |= spcatfAvailable;
            goto HandleError;
        }

        if ( pgno == pgnoFDP )
        {
            spcatf |= spcatfRoot;
        }
    }
    else
    {
        CSPExtentInfo speiOE;
        err = ErrSPIFindExtOE( ppib, pfcb, pgno, &speiOE );
        if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
        {
            err = JET_errSuccess;
        }
        Call( err );

        if ( !speiOE.FIsSet() || !speiOE.FContains( pgno ) )
        {
            if ( objid == objidSystemRoot )
            {
                if ( speiOE.FIsSet() && !speiOE.FContains( pgno ) )
                {
                    spcatf = spcatfNotOwned;
                }
                else
                {
                    Assert( !speiOE.FIsSet() );

                    QWORD cbSize = 0;
                    Call( g_rgfmp[ ifmp ].Pfapi()->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
                    const PGNO pgnoBeyondEOF = PgnoOfOffset( cbSize );
                    if ( pgno >= pgnoBeyondEOF )
                    {
                        Error( ErrERRCheck( JET_errFileIOBeyondEOF ) );
                    }
                    else
                    {
                        spcatf = spcatfNotOwnedEof;
                    }
                }

                goto HandleError;
            }
            else
            {
                Assert( spcatf == spcatfNone );
            }
        }
        else
        {
            fOwned = fTrue;
        }

        for ( SpacePool sppAvailPool = spp::MinPool;
                sppAvailPool < spp::MaxPool;
                sppAvailPool++ )
        {
            if ( sppAvailPool == spp::ShelvedPool )
            {
                continue;
            }

            CSPExtentInfo speiAE;
            err = ErrSPIFindExtAE( ppib, pfcb, pgno, sppAvailPool, &speiAE );
            if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
            {
                err = JET_errSuccess;
            }
            Call( err );

            if ( speiAE.FIsSet() && speiAE.FContains( pgno ) )
            {
                if ( fOwned )
                {
                    spcatf |= spcatfAvailable;
                }
                else
                {
                    AssertTrack( fFalse, "SpaceCatAvailNotOwned" );
                    spcatf = spcatfInconsistent;
                }

                goto HandleError;
            }
        }

        if ( ( pgno == pgnoFDP ) || ( pgno == pfcb->PgnoOE() ) || ( pgno == pfcb->PgnoAE() ) )
        {
            spcatf |= spcatfRoot;
            spcatf |= ( pgno == pfcb->PgnoOE() ) ? spcatfSpaceOE :
                        ( ( pgno == pfcb->PgnoAE() ) ? spcatfSpaceAE : spcatfNone );
        }
    }

    BTUp( pfucb );
    pfucb->pcsrRoot = pcsrNil;

    Assert( ( fOwned && !FSPSpaceCatAvailable( spcatf ) ) ||
            ( !fOwned && !FSPIIsSmall( pfcb ) && ( objid != objidSystemRoot ) && ( spcatf == spcatfNone ) ) );


    Assert( !FSPSpaceCatSplitBuffer( spcatf ) );
    if ( !FSPSpaceCatRoot( spcatf ) && !FSPSpaceCatSmallSpace( spcatf ) )
    {
        BOOL fSPBufAEContains = fFalse;
        BOOL fSPBufOEContains = fFalse;
        Call( ErrSPISPBufContains( ppib, pfcb, pgno, fFalse, &fSPBufAEContains ) );
        if ( !fSPBufAEContains )
        {
            Call( ErrSPISPBufContains( ppib, pfcb, pgno, fTrue, &fSPBufOEContains ) );
        }
        if ( fSPBufAEContains || fSPBufOEContains )
        {
            Assert( !( fSPBufAEContains && fSPBufOEContains ) );
            spcatf |= ( spcatfSplitBuffer | spcatfAvailable | ( fSPBufAEContains ? spcatfSpaceAE : spcatfSpaceOE ) );
            goto HandleError;
        }
    }


    Assert( !FSPSpaceCatStrictlyInternal( spcatf ) && !FSPSpaceCatStrictlyLeaf( spcatf ) );
    err = cpage.ErrGetReadPage( ppib, ifmp, pgno, BFLatchFlags( bflfNoTouch | bflfUninitPageOk ) );
    if ( err < JET_errSuccess )
    {
        if ( err == JET_errPageNotInitialized )
        {
            err = JET_errSuccess;
            if ( FSPSpaceCatRoot( spcatf ) )
            {
                AssertTrack( false, "SpaceCatBlankRoot" );
                spcatf = spcatfInconsistent;
            }
            else
            {
                spcatf = spcatfUnknown;
            }
            goto HandleError;
        }
        else
        {
            Error( err );
        }
    }

    fPageLatched = fTrue;
    Assert( cpage.FPageIsInitialized() );

    if ( cpage.ObjidFDP() != objid )
    {
        spcatf = spcatfUnknown;
        goto HandleError;
    }

    if ( FSPSpaceCatRoot( spcatf ) && !cpage.FRootPage() )
    {
        AssertTrack( false, "SpaceCatIncRootFlag" );
        spcatf = spcatfInconsistent;
        goto HandleError;
    }

    if ( FSPSpaceCatAnySpaceTree( spcatf ) && !cpage.FSpaceTree() )
    {
        AssertTrack( false, "SpaceCatIncSpaceFlag" );
        spcatf = spcatfInconsistent;
        goto HandleError;
    }

    if ( cpage.FPreInitPage() && ( cpage.FRootPage() || cpage.FLeafPage() ) )
    {
        AssertTrack( false, "SpaceCatIncPreInitFlag" );
        spcatf = spcatfInconsistent;
        goto HandleError;
    }

    if ( FSPSpaceCatRoot( spcatf ) )
    {
        goto HandleError;
    }

    if ( cpage.FEmptyPage() || cpage.FPreInitPage() || ( cpage.Clines() <= 0 ) )
    {
        spcatf = spcatfUnknown;
        goto HandleError;
    }

#ifdef DEBUG
    const INT iline = ( ( cpage.Clines() - 1 ) * ( rand() % 5 ) ) / 4;
#else
    const INT iline = 0;
#endif
    Assert( ( iline >= 0 ) && ( iline < cpage.Clines() ) );
    NDIGetKeydataflags( cpage, iline, &kdf );

    const BOOL fLeafPage = cpage.FLeafPage();
    const BOOL fSpacePage = cpage.FSpaceTree();
    const BOOL fNonUniqueKeys = cpage.FNonUniqueKeys() && !fSpacePage;

    if ( fLeafPage && fNonUniqueKeys )
    {
        if ( kdf.key.FNull() || kdf.data.FNull() )
        {
            AssertTrack( false, "SpaceCatIncKdfNonUnique" );
            spcatf = spcatfInconsistent;
            goto HandleError;
        }

        Call( pbm->ErrCopyKeyData( kdf.key, kdf.data ) );
    }
    else
    {
        if ( fLeafPage && kdf.key.FNull() )
        {
            AssertTrack( false, "SpaceCatIncKdfUniqueLeaf" );
            spcatf = spcatfInconsistent;
            goto HandleError;
        }
        else if ( !fLeafPage && kdf.data.FNull() )
        {
            AssertTrack( false, "SpaceCatIncKdfUniqueInternal" );
            spcatf = spcatfInconsistent;
            goto HandleError;
        }

        Call( pbm->ErrCopyKey( kdf.key ) );
    }

    cpage.ReleaseReadLatch();
    fPageLatched = fFalse;

    if ( !fSpacePage )
    {
        if ( !fNonUniqueKeys != !!FFUCBUnique( pfucb ) )
        {
            AssertTrack( false, "SpaceCatIncUniqueFdp" );
            spcatf = spcatfInconsistent;
            goto HandleError;
        }

        err = ErrBTContainsPage( pfucb, *pbm, pgno, fLeafPage );
        if ( err >= JET_errSuccess )
        {
            spcatf |= ( fLeafPage ? spcatfStrictlyLeaf : spcatfStrictlyInternal );
            goto HandleError;
        }
        else if ( err == JET_errRecordNotFound )
        {
            err = JET_errSuccess;
        }
        Call( err );
    }
    else
    {
        Assert( pfucbSpace == pfucbNil );
        Call( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbSpace ) );
        if ( !fNonUniqueKeys != !!FFUCBUnique( pfucbSpace ) )
        {
            AssertTrack( false, "SpaceCatIncUniqueAe" );
            spcatf = spcatfInconsistent;
            goto HandleError;
        }
        err = ErrBTContainsPage( pfucbSpace, *pbm, pgno, fLeafPage );
        if ( err >= JET_errSuccess )
        {
            spcatf |= ( spcatfSpaceAE | ( fLeafPage ? spcatfStrictlyLeaf : spcatfStrictlyInternal ) );
            goto HandleError;
        }
        else if ( err == JET_errRecordNotFound )
        {
            err = JET_errSuccess;
        }
        Call( err );
        BTUp( pfucbSpace );
        BTClose( pfucbSpace );
        pfucbSpace = pfucbNil;

        Assert( pfucbSpace == pfucbNil );
        Call( ErrSPIOpenOwnExt( ppib, pfcb, &pfucbSpace ) );
        if ( !fNonUniqueKeys != !!FFUCBUnique( pfucbSpace ) )
        {
            AssertTrack( false, "SpaceCatIncUniqueOe" );
            spcatf = spcatfInconsistent;
            goto HandleError;
        }
        err = ErrBTContainsPage( pfucbSpace, *pbm, pgno, fLeafPage );
        if ( err >= JET_errSuccess )
        {
            spcatf |= ( spcatfSpaceOE | ( fLeafPage ? spcatfStrictlyLeaf : spcatfStrictlyInternal ) );
            goto HandleError;
        }
        else if ( err == JET_errRecordNotFound )
        {
            err = JET_errSuccess;
        }
        Call( err );
        BTUp( pfucbSpace );
        BTClose( pfucbSpace );
        pfucbSpace = pfucbNil;
    }


    spcatf = spcatfUnknown;

HandleError:
    if ( fPageLatched )
    {
        cpage.ReleaseReadLatch();
        fPageLatched = fFalse;
    }

    if ( pfucbSpace != pfucbNil )
    {
        BTUp( pfucbSpace );
    }

    if ( pfucb != pfucbNil )
    {
        BTUp( pfucb );
        pfucb->pcsrRoot = pcsrNil;
    }

    if ( pfucbParent != pfucbNil )
    {
        BTUp( pfucbParent );
    }

    if ( ( err >= JET_errSuccess ) &&
            FSPSpaceCatAnySpaceTree( spcatf ) &&
            ( pfucbSpace == pfucbNil ) )
    {
        if ( FSPSpaceCatSpaceOE( spcatf ) )
        {
            err = ErrSPIOpenOwnExt( ppib, pfcb, &pfucbSpace );
        }
        else
        {
            Assert( FSPSpaceCatSpaceAE( spcatf ) );
            err = ErrSPIOpenAvailExt( ppib, pfcb, &pfucbSpace );
        }
    }

    if ( ( err >= JET_errSuccess ) &&
            !FSPSpaceCatAnySpaceTree( spcatf ) &&
            ( pfucbSpace != pfucbNil ))
    {
        BTClose( pfucbSpace );
        pfucbSpace = NULL;
    }

    if ( ( err >= JET_errSuccess ) &&
            !FSPSpaceCatStrictlyInternal( spcatf ) &&
            !FSPSpaceCatStrictlyLeaf( spcatf ) &&
            ( pbm != NULL ) )
    {
        delete pbm;
        pbm = NULL;
    }

    if ( pSpCatCtx != NULL )
    {
        pSpCatCtx->pfucbParent = pfucbParent;
        pSpCatCtx->pfucb = pfucb;
        pSpCatCtx->pfucbSpace = pfucbSpace;
        pSpCatCtx->pbm = pbm;
    }
    else
    {
        Assert( pfucbParent == pfucbNil );
        Assert( pfucb == pfucbNil );
        Assert( pfucbSpace == pfucbNil );
        Assert( pbm == NULL );
    }

    if ( err >= JET_errSuccess )
    {
        *pspcatf = spcatf;
        *pfOwned = fOwned;
    }
    else
    {
        *pspcatf = spcatfUnknown;
        *pfOwned = fFalse;
        SPFreeSpaceCatCtx( &pSpCatCtx );
    }

    *ppSpCatCtx = pSpCatCtx;

    return err;
}

ERR ErrSPIGetSpaceCategoryObjectAndChildren(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _In_ const OBJID objid,
    _In_ const OBJID objidChildExclude,
    _Out_ OBJID* const pobjid,
    _Out_ SpaceCategoryFlags* const pspcatf,
    _Out_ SpaceCatCtx** const ppSpCatCtx )
{
    ERR err = JET_errSuccess;
    FUCB* pfucbCatalog = pfucbNil;
    OBJID objidChild = objidNil;
    SYSOBJ sysobjChild = sysobjNil;
    SpaceCatCtx* pSpCatCtxParent = NULL;

    BOOL fParentOwned = fFalse;
    Call( ErrSPIGetSpaceCategoryObject( ppib, ifmp, pgno, objid, objidSystemRoot, sysobjTable, pspcatf, &fParentOwned, ppSpCatCtx ) );
    if ( !FSPSpaceCatUnknown( *pspcatf ) )
    {
        *pobjid = objid;
        goto HandleError;
    }

    if ( !fParentOwned )
    {
        *pobjid = objidNil;
        goto HandleError;
    }

    pSpCatCtxParent = *ppSpCatCtx;
    *ppSpCatCtx = NULL;

    for ( err = ErrCATGetNextNonRootObject( ppib, ifmp, objid, &pfucbCatalog, &objidChild, &sysobjChild );
            ( err >= JET_errSuccess ) && ( objidChild != objidNil );
            err = ErrCATGetNextNonRootObject( ppib, ifmp, objid, &pfucbCatalog, &objidChild, &sysobjChild ) )
    {
        if ( objidChild == objidChildExclude )
        {
            continue;
        }

        BOOL fChildOwned = fFalse;
        Call( ErrSPIGetSpaceCategoryObject( ppib, ifmp, pgno, objidChild, objid, sysobjChild, pspcatf, &fChildOwned, ppSpCatCtx ) );
        if ( !FSPSpaceCatUnknown( *pspcatf ) )
        {
            *pobjid = objidChild;
            goto HandleError;
        }

        if ( fChildOwned )
        {
            *pobjid = objidChild;
            *pspcatf = spcatfLeaked;
            goto HandleError;
        }

        SPFreeSpaceCatCtx( ppSpCatCtx );
    }

    Call( err );

    *pobjid = objid;
    *pspcatf = spcatfLeaked;
    *ppSpCatCtx = pSpCatCtxParent;
    pSpCatCtxParent = NULL;

HandleError:
    if ( pfucbCatalog != pfucbNil )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }

    SPFreeSpaceCatCtx( &pSpCatCtxParent );

    if ( err < JET_errSuccess )
    {
        SPFreeSpaceCatCtx( ppSpCatCtx );
    }

    return err;
}

ERR ErrSPIGetSpaceCategoryNoHint(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _In_ const OBJID objidExclude,
    _Out_ OBJID* const pobjid,
    _Out_ SpaceCategoryFlags* const pspcatf,
    _Out_ SpaceCatCtx** const ppSpCatCtx )
{
    ERR err = JET_errSuccess;
    FUCB* pfucbCatalog = pfucbNil;
    OBJID objid = objidNil;

    for ( err = ErrCATGetNextRootObject( ppib, ifmp, &pfucbCatalog, &objid );
        ( err >= JET_errSuccess ) && ( objid != objidNil );
        err = ErrCATGetNextRootObject( ppib, ifmp, &pfucbCatalog, &objid ) )
    {
        if ( objid == objidExclude )
        {
            continue;
        }

        Call( ErrSPIGetSpaceCategoryObjectAndChildren( ppib, ifmp, pgno, objid, objidNil, pobjid, pspcatf, ppSpCatCtx ) );
        if ( !FSPSpaceCatUnknown( *pspcatf ) )
        {
            goto HandleError;
        }

        SPFreeSpaceCatCtx( ppSpCatCtx );
    }

    Call( err );

HandleError:
    if ( pfucbCatalog != pfucbNil )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }

    if ( err < JET_errSuccess )
    {
        SPFreeSpaceCatCtx( ppSpCatCtx );
    }

    return err;
}

ERR ErrSPGetSpaceCategory(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _In_ const OBJID objidHint,
    _In_ const BOOL fRunFullSpaceCat,
    _Out_ OBJID* const pobjid,
    _Out_ SpaceCategoryFlags* const pspcatf,
    _Out_ SpaceCatCtx** const ppSpCatCtx )
{
    Assert( pobjid != NULL );
    Assert( pspcatf != NULL );
    Assert( ppSpCatCtx != NULL );

    ERR err = JET_errSuccess;
    OBJID objid = objidNil;
    SpaceCategoryFlags spcatf = spcatfUnknown;
    OBJID objidHintParent = objidNil;
    OBJID objidHintPage = objidNil;
    OBJID objidHintPageParent = objidNil;
    OBJID objidHintExclude = objidNil;
    BOOL fSearchedRoot = fFalse;
    BOOL fOwned = fFalse;
    SpaceCatCtx* pSpCatCtxRoot = NULL;

    if ( ( objidHint != objidNil ) &&
            ( objidHint != objidSystemRoot ) )
    {
        SYSOBJ sysobjHint = sysobjNil;
        Call( ErrCATGetObjidMetadata( ppib, ifmp, objidHint, &objidHintParent, &sysobjHint ) );
        if ( sysobjHint != sysobjNil )
        {
            Assert( objidHintParent != objidNil );
            
            if ( objidHint != objidHintParent )
            {
                Call( ErrSPIGetSpaceCategoryObject( ppib,
                                                    ifmp,
                                                    pgno,
                                                    objidHint,
                                                    objidHintParent,
                                                    sysobjHint,
                                                    &spcatf,
                                                    &fOwned,
                                                    ppSpCatCtx ) );

                if ( !FSPSpaceCatUnknown( spcatf ) )
                {
                    objid = objidHint;
                    goto HandleError;
                }

                if ( fOwned )
                {
                    objid = objidHint;
                    spcatf = spcatfLeaked;
                    goto HandleError;
                }

                SPFreeSpaceCatCtx( ppSpCatCtx );
            }
        }
        else
        {
            objidHintParent = objidNil;
            err = JET_errSuccess;
        }
    }

    Assert( FSPSpaceCatUnknown( spcatf ) );

    if ( ( objidHintParent != objidNil ) &&
            ( objidHintParent != objidSystemRoot ) )
    {
        Call( ErrSPIGetSpaceCategoryObjectAndChildren( ppib,
                                                        ifmp,
                                                        pgno,
                                                        objidHintParent,
                                                        ( objidHint != objidHintParent ) ? objidHint : objidNil,
                                                        &objid,
                                                        &spcatf,
                                                        ppSpCatCtx ) );

        if ( !FSPSpaceCatUnknown( spcatf ) )
        {
            goto HandleError;
        }

        SPFreeSpaceCatCtx( ppSpCatCtx );

        objidHintExclude = objidHintParent;
    }

    Assert( FSPSpaceCatUnknown( spcatf ) );

    if ( objidHint == objidSystemRoot )
    {
        Call( ErrSPIGetSpaceCategoryObject( ppib, ifmp, pgno, objidSystemRoot, objidNil, sysobjNil, &spcatf, &fOwned, ppSpCatCtx ) );
        fSearchedRoot = fTrue;
        if ( !FSPSpaceCatUnknown( spcatf ) )
        {
            Assert( !FSPSpaceCatRoot( spcatf ) ||
                    ( pgno == pgnoSystemRoot ) ||
                    ( FSPSpaceCatSpaceOE( spcatf ) && ( pgno == ( pgnoSystemRoot + 1 ) ) ) ||
                    ( FSPSpaceCatSpaceAE( spcatf ) && ( pgno == ( pgnoSystemRoot + 2 ) ) ) );

            objid = objidSystemRoot;
            goto HandleError;
        }

        Assert( pSpCatCtxRoot == NULL );
        pSpCatCtxRoot = *ppSpCatCtx;
        *ppSpCatCtx = NULL;
    }

    Assert( FSPSpaceCatUnknown( spcatf ) );

    err = ErrSPIGetObjidFromPage( ppib, ifmp, pgno, &objidHintPage );
    if ( err >= JET_errSuccess )
    {
        if ( ( objidHintPage != objidNil ) &&
                ( objidHintPage != objidSystemRoot ) &&
                ( objidHintPage != objidHint ) &&
                ( objidHintPage != objidHintParent ) )
        {
            SYSOBJ sysobjHint = sysobjNil;
            Call( ErrCATGetObjidMetadata( ppib, ifmp, objidHintPage, &objidHintPageParent, &sysobjHint ) );
            if ( sysobjHint != sysobjNil )
            {
                Assert( objidHintPageParent != objidNil );
                
                if ( objidHintPage != objidHintPageParent )
                {
                    Call( ErrSPIGetSpaceCategoryObject( ppib,
                                                        ifmp,
                                                        pgno,
                                                        objidHintPage,
                                                        objidHintPageParent,
                                                        sysobjHint,
                                                        &spcatf,
                                                        &fOwned,
                                                        ppSpCatCtx ) );

                    if ( !FSPSpaceCatUnknown( spcatf ) )
                    {
                        objid = objidHintPage;
                        goto HandleError;
                    }

                    if ( fOwned )
                    {
                        objid = objidHintPage;
                        spcatf = spcatfLeaked;
                        goto HandleError;
                    }

                    SPFreeSpaceCatCtx( ppSpCatCtx );
                }
            }
            else
            {
                objidHintPageParent = objidNil;
                err = JET_errSuccess;
            }

            if ( ( objidHintPageParent != objidNil ) &&
                    ( objidHintPageParent != objidSystemRoot ) &&
                    ( objidHintPageParent != objidHint ) &&
                    ( objidHintPageParent != objidHintParent ) )
            {
                Call( ErrSPIGetSpaceCategoryObjectAndChildren( ppib,
                                                                ifmp,
                                                                pgno,
                                                                objidHintPageParent,
                                                                ( objidHintPage != objidHintPageParent ) ? objidHintPage : objidNil,
                                                                &objid,
                                                                &spcatf,
                                                                ppSpCatCtx ) );

                if ( !FSPSpaceCatUnknown( spcatf ) )
                {
                    goto HandleError;
                }

                SPFreeSpaceCatCtx( ppSpCatCtx );

                objidHintExclude = objidHintPageParent;
            }
        }
    }
    else if ( err == JET_errPageNotInitialized )
    {
        objidHintPage = objidNil;
        err = JET_errSuccess;
    }
    else
    {
        Assert( err < JET_errSuccess );
        Error( err );
    }

    Assert( FSPSpaceCatUnknown( spcatf ) );

    if ( !fSearchedRoot )
    {
        Call( ErrSPIGetSpaceCategoryObject( ppib, ifmp, pgno, objidSystemRoot, objidNil, sysobjNil, &spcatf, &fOwned, ppSpCatCtx ) );
        fSearchedRoot = fTrue;
        if ( !FSPSpaceCatUnknown( spcatf ) )
        {
            objid = objidSystemRoot;
            goto HandleError;
        }

        Assert( pSpCatCtxRoot == NULL );
        pSpCatCtxRoot = *ppSpCatCtx;
        *ppSpCatCtx = NULL;
    }

    Assert( FSPSpaceCatUnknown( spcatf ) );
    Assert( fSearchedRoot );

    if ( fRunFullSpaceCat )
    {
        Call( ErrSPIGetSpaceCategoryNoHint( ppib, ifmp, pgno, objidHintExclude, &objid, &spcatf, ppSpCatCtx ) );
        Assert( !FSPSpaceCatIndeterminate( spcatf ) );

        if ( FSPSpaceCatUnknown( spcatf ) )
        {
            objid = objidSystemRoot;
            spcatf = spcatfLeaked;

            Assert( pSpCatCtxRoot != NULL );
            *ppSpCatCtx = pSpCatCtxRoot;
            pSpCatCtxRoot = NULL;
        }
    }
    else
    {
        spcatf = spcatfIndeterminate;
    }

    Assert( FSPValidSpaceCategoryFlags( spcatf ) );
    Assert( !FSPSpaceCatUnknown( spcatf ) );

HandleError:

    SPFreeSpaceCatCtx( &pSpCatCtxRoot );

    if ( err >= JET_errSuccess )
    {
#ifdef DEBUG
        Assert( !FSPSpaceCatIndeterminate( spcatf ) || !fRunFullSpaceCat );
        Assert( ( objid != objidNil ) || FSPSpaceCatIndeterminate( spcatf ) );

        Assert( pgno != pgnoNull );
        if ( pgno <= ( pgnoSystemRoot + 2 ) )
        {
            Assert( objid == objidSystemRoot );
            Assert( ( pgno != pgnoSystemRoot ) ||
                    ( FSPSpaceCatRoot( spcatf ) && !FSPSpaceCatAnySpaceTree( spcatf ) ) );
            Assert( ( pgno != ( pgnoSystemRoot + 1 ) ) ||
                    ( FSPSpaceCatRoot( spcatf ) && FSPSpaceCatSpaceOE( spcatf ) ) );
            Assert( ( pgno != ( pgnoSystemRoot + 2 ) ) ||
                    ( FSPSpaceCatRoot( spcatf ) && FSPSpaceCatSpaceAE( spcatf ) ) );
        }
        if ( objid == objidSystemRoot )
        {
            Assert( FSPSpaceCatAnySpaceTree( spcatf ) ||
                    ( !FSPSpaceCatStrictlyLeaf( spcatf ) && !FSPSpaceCatStrictlyInternal( spcatf ) ) );
        }

        if ( pgno == pgnoFDPMSO )
        {
            Assert( objid == objidFDPMSO );
            Assert( FSPSpaceCatRoot( spcatf ) && !FSPSpaceCatAnySpaceTree( spcatf ) );
        }

        if ( pgno == pgnoFDPMSOShadow )
        {
            Assert( objid == objidFDPMSOShadow );
            Assert( FSPSpaceCatRoot( spcatf ) && !FSPSpaceCatAnySpaceTree( spcatf ) );
        }

        if ( pgno == pgnoFDPMSO_NameIndex )
        {
            Assert( FSPSpaceCatRoot( spcatf ) && !FSPSpaceCatAnySpaceTree( spcatf ) );
        }

        if ( pgno == pgnoFDPMSO_RootObjectIndex )
        {
            Assert( FSPSpaceCatRoot( spcatf ) && !FSPSpaceCatAnySpaceTree( spcatf ) );
        }

        const SpaceCatCtx* const pSpCatCtx = *ppSpCatCtx;
        if ( pSpCatCtx != NULL )
        {
            Assert( !FSPSpaceCatIndeterminate( spcatf ) );
            
            Assert( pSpCatCtx->pfucb != pfucbNil );
            Assert( pSpCatCtx->pfucb->u.pfcb->PgnoFDP() != pgnoNull );

            if ( pSpCatCtx->pfucbParent != pfucbNil )
            {
                Assert( objid != objidSystemRoot );

                Assert( pSpCatCtx->pfucbParent->u.pfcb != pSpCatCtx->pfucb->u.pfcb );

                if ( pSpCatCtx->pfucbSpace != pfucbNil )
                {
                    Assert( pSpCatCtx->pfucbParent->u.pfcb != pSpCatCtx->pfucbSpace->u.pfcb );
                }

                if ( pSpCatCtx->pfucbParent->u.pfcb->PgnoFDP() != pgnoSystemRoot )
                {
                    Assert( ( pSpCatCtx->pfucb == pSpCatCtx->pfucbParent->pfucbCurIndex ) ||
                            ( pSpCatCtx->pfucb == pSpCatCtx->pfucbParent->pfucbLV ) );
                }
            }
            else
            {
                Assert( objid == objidSystemRoot );
                Assert( pSpCatCtx->pfucb->u.pfcb->PgnoFDP() == pgnoSystemRoot );
            }

            if ( pSpCatCtx->pfucbSpace != pfucbNil )
            {
                Assert( FSPSpaceCatAnySpaceTree( spcatf ) );
                Assert( ( FSPSpaceCatSpaceOE( spcatf ) && FFUCBOwnExt( pSpCatCtx->pfucbSpace ) ) ||
                        ( FSPSpaceCatSpaceAE( spcatf ) && FFUCBAvailExt( pSpCatCtx->pfucbSpace ) ) );
                Assert( pSpCatCtx->pfucbSpace != pSpCatCtx->pfucb );
                Assert( pSpCatCtx->pfucbSpace->u.pfcb == pSpCatCtx->pfucb->u.pfcb );

                if ( FSPSpaceCatRoot( spcatf ) )
                {
                    Assert( ( FSPSpaceCatSpaceOE( spcatf ) && ( pSpCatCtx->pfucb->u.pfcb->PgnoOE() == pgno ) ) ||
                            ( FSPSpaceCatSpaceAE( spcatf ) && ( pSpCatCtx->pfucb->u.pfcb->PgnoAE() == pgno ) ) );
                }
            }
            else
            {
                Assert( !FSPSpaceCatAnySpaceTree( spcatf ) );

                if ( FSPSpaceCatRoot( spcatf ) )
                {
                    Assert( pSpCatCtx->pfucb->u.pfcb->PgnoFDP() == pgno );
                }
            }

            Assert( !FSPSpaceCatStrictlyInternal( spcatf ) && !FSPSpaceCatStrictlyLeaf( spcatf ) || ( pSpCatCtx->pbm != NULL )  );
        }
        else
        {
            Assert( FSPSpaceCatIndeterminate( spcatf ) );
        }
#endif

        if ( ( !FSPValidSpaceCategoryFlags( spcatf ) ||
                    FSPSpaceCatUnknown( spcatf ) ||
                    FSPSpaceCatInconsistent( spcatf ) ) )
        {
            FireWall( OSFormat( "InconsistentCategoryFlags:0x%I32x", spcatf ) );
        }

        *pobjid = objid;
        *pspcatf = spcatf;
        err = JET_errSuccess;
    }
    else
    {
        SPFreeSpaceCatCtx( ppSpCatCtx );
        *pobjid = objidNil;
        *pspcatf = spcatfUnknown;
    }

    return err;
}

ERR ErrSPGetSpaceCategory(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _In_ const OBJID objidHint,
    _In_ const BOOL fRunFullSpaceCat,
    _Out_ OBJID* const pobjid,
    _Out_ SpaceCategoryFlags* const pspcatf )
{
    SpaceCatCtx* pSpCatCtx = NULL;
    const ERR err = ErrSPGetSpaceCategory( ppib, ifmp, pgno, objidHint, fRunFullSpaceCat, pobjid, pspcatf, &pSpCatCtx );
    SPFreeSpaceCatCtx( &pSpCatCtx );
    return err;
}

ERR ErrSPGetSpaceCategoryRange(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgnoFirst,
    _In_ const PGNO pgnoLast,
    _In_ const BOOL fRunFullSpaceCat,
    _In_ const JET_SPCATCALLBACK pfnCallback,
    _In_opt_ VOID* const pvContext )
{
    ERR err = JET_errSuccess;
    BOOL fMSysObjidsReady = fFalse;

    if ( ( pgnoFirst < 1 ) || ( pgnoLast > pgnoSysMax ) || ( pgnoFirst > pgnoLast ) || ( pfnCallback == NULL ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Call( ErrCATCheckMSObjidsReady( ppib, ifmp, &fMSysObjidsReady ) );
    if ( !fMSysObjidsReady )
    {
        Error( ErrERRCheck( JET_errObjectNotFound ) );
    }

    const CPG cpgPrereadMax = LFunctionalMax( 1, (CPG)( UlParam( JET_paramMaxCoalesceReadSize ) / g_cbPage ) );
    PGNO pgnoPrereadWaypoint = pgnoFirst, pgnoPrereadNext = pgnoFirst;
    OBJID objidHint = objidNil;

    for ( PGNO pgnoCurrent = pgnoFirst; pgnoCurrent <= pgnoLast; pgnoCurrent++ )
    {
        if ( ( pgnoCurrent >= pgnoPrereadWaypoint ) && ( pgnoPrereadNext <= pgnoLast ) )
        {
            PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
            tcScope->iorReason.SetIort( iortDbShrink );
            const CPG cpgPrereadCurrent = LFunctionalMin( cpgPrereadMax, (CPG)( pgnoLast - pgnoPrereadNext + 1 ) );
            Assert( cpgPrereadCurrent >= 1 );
            BFPrereadPageRange( ifmp, pgnoPrereadNext, cpgPrereadCurrent, bfprfDefault, ppib->BfpriPriority( ifmp ), *tcScope );
            pgnoPrereadNext += cpgPrereadCurrent;
            pgnoPrereadWaypoint = pgnoPrereadNext - ( cpgPrereadCurrent / 2 );
        }

        OBJID objidCurrent = objidNil;
        SpaceCategoryFlags spcatfCurrent = spcatfNone;
        Call( ErrSPGetSpaceCategory(
                ppib,
                ifmp,
                pgnoCurrent,
                objidHint,
                fRunFullSpaceCat,
                &objidCurrent,
                &spcatfCurrent ) );
        objidHint = ( objidCurrent != objidNil ) ? objidCurrent : objidHint;
        Assert( FSPValidSpaceCategoryFlags( spcatfCurrent ) );
        Assert( !FSPSpaceCatUnknown( spcatfCurrent ) );

        pfnCallback( pgnoCurrent, objidCurrent, spcatfCurrent, pvContext );
    }

HandleError:
    return err;
}

ERR ErrSPICheckExtentIsAllocatable( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg )
{
    Assert( ifmp != ifmpNil );
    Assert( pgnoFirst != pgnoNull );
    Assert( cpg > 0 );

    if ( g_rgfmp[ ifmp ].FBeyondPgnoShrinkTarget( pgnoFirst, cpg ) )
    {
        AssertTrack( fFalse, "ShrinkExtentOverlap" );
        return ErrERRCheck( JET_errSPAvailExtCorrupted );
    }

    if ( ( pgnoFirst + cpg - 1 ) > g_rgfmp[ ifmp ].PgnoLast() )
    {
        AssertTrack( fFalse, "PageBeyondEofAlloc" );
        return ErrERRCheck( JET_errSPAvailExtCorrupted );
    }

    return JET_errSuccess;
}

ERR ErrSPISmallGetExt(
    FUCB        *pfucbParent,
    CPG         *pcpgReq,
    CPG         cpgMin,
    PGNO        *ppgnoFirst )
{
    FCB         * const pfcb    = pfucbParent->u.pfcb;
    ERR                 err     = errSPNoSpaceForYou;

    AssertSPIPfucbOnRoot( pfucbParent );
    Assert( pfcb->PgnoFDP() != pgnoSystemRoot );

    if ( cpgMin <= cpgSmallSpaceAvailMost )
    {
        SPACE_HEADER    sph;
        UINT            rgbitT;
        PGNO            pgnoAvail;
        DATA            data;
        INT             iT;

        NDGetExternalHeader( pfucbParent, pfucbParent->pcsrRoot, noderfSpaceHeader );
        Assert( pfucbParent->pcsrRoot->Latch() == latchRIW
                    || pfucbParent->pcsrRoot->Latch() == latchWrite );
        Assert( sizeof( SPACE_HEADER ) == pfucbParent->kdfCurr.data.Cb() );

        UtilMemCpy( &sph, pfucbParent->kdfCurr.data.Pv(), sizeof(sph) );

        const PGNO  pgnoFDP         = PgnoFDP( pfucbParent );
        const CPG   cpgSingleExtent = min( sph.CpgPrimary(), cpgSmallSpaceAvailMost+1 );
        Assert( cpgSingleExtent > 0 );
        Assert( cpgSingleExtent <= cpgSmallSpaceAvailMost+1 );

        Assert( cpgMin > 0 );
        Assert( cpgMin <= 32 );
        for ( rgbitT = 1, iT = 1; iT < cpgMin; iT++ )
        {
            rgbitT = (rgbitT<<1) + 1;
        }

        for( pgnoAvail = pgnoFDP + 1;
            pgnoAvail + cpgMin <= pgnoFDP + cpgSingleExtent;
            pgnoAvail++, rgbitT <<= 1 )
        {
            Assert( rgbitT != 0 );
            if ( ( rgbitT & sph.RgbitAvail() ) == rgbitT )
            {
                SPIUpgradeToWriteLatch( pfucbParent );

                sph.SetRgbitAvail( sph.RgbitAvail() ^ rgbitT );
                data.SetPv( &sph );
                data.SetCb( sizeof(sph) );
                Assert( errSPNoSpaceForYou == err );
                CallR( ErrNDSetExternalHeader(
                                pfucbParent,
                                pfucbParent->pcsrRoot,
                                &data,
                                ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                                noderfSpaceHeader ) );

                Assert( pgnoAvail != pgnoNull );

                Assert( !g_rgfmp[ pfucbParent->ifmp ].FBeyondPgnoShrinkTarget( pgnoAvail, cpgMin ) );

                Assert( ( pgnoAvail + cpgMin - 1 ) <= g_rgfmp[ pfucbParent->ifmp ].PgnoLast() );

                *ppgnoFirst = pgnoAvail;
                *pcpgReq = cpgMin;
                err = JET_errSuccess;
                goto HandleError;
            }
        }
    }

HandleError:
    return err;
}

ERR ErrSPIAEFindExt(
    __inout FUCB * const    pfucbAE,
    __in    const CPG       cpgMin,
    __in    const SpacePool sppAvailPool,
    __out   CSPExtentInfo * pcspaei )
{
    ERR         err = JET_errSuccess;
    DIB         dib;
    BOOL        fFoundNextAvailSE       = fFalse;
    FCB* const  pfcb = pfucbAE->u.pfcb;

    Assert( FFUCBSpace( pfucbAE ) );
    Assert( !FSPIIsSmall( pfcb ) );


    const PGNO  pgnoSeek = ( cpgMin >= cpageSEDefault ? pfcb->PgnoNextAvailSE() : pgnoNull );

    const CSPExtentKeyBM    spaeFindExt( SPEXTKEY::fSPExtentTypeAE, pgnoSeek, sppAvailPool );
    dib.pos = posDown;
    dib.pbm = spaeFindExt.Pbm( pfucbAE );


    dib.dirflag = fDIRFavourNext;

    if ( ( err = ErrBTDown( pfucbAE, &dib, latchReadTouch ) ) < 0 )
    {
        Assert( err != JET_errNoCurrentRecord );
        if ( JET_errRecordNotFound == err )
        {
            Error( ErrERRCheck( errSPNoSpaceForYou ) );
        }
        OSTraceFMP( pfucbAE->ifmp, JET_tracetagSpaceInternal,
                    OSFormat( "ErrSPGetExt could not down into available extent tree. [ifmp=0x%x]\n", pfucbAE->ifmp ) );
        Call( err );
    }


    do
    {
        pcspaei->Set( pfucbAE );

        if ( pcspaei->SppPool() != sppAvailPool )
        {
            Error( ErrERRCheck( errSPNoSpaceForYou ) );
        }

#ifdef DEPRECATED_ZERO_SIZED_AVAIL_EXTENT_CLEANUP
        if ( 0 == pcspaei->CpgExtent() )
        {
            Call( ErrBTFlagDelete(
                        pfucbAE,
                        fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
            Pcsr( pfucbAE )->Downgrade( latchReadTouch );
        }
#endif

        if ( pcspaei->CpgExtent() > 0 )
        {
            if ( g_rgfmp[ pfucbAE->ifmp ].FBeyondPgnoShrinkTarget( pcspaei->PgnoFirst(), pcspaei->CpgExtent() ) )
            {
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            if ( pcspaei->PgnoLast() > g_rgfmp[ pfucbAE->ifmp ].PgnoLast() )
            {
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            if ( !fFoundNextAvailSE
                && pcspaei->CpgExtent() >= cpageSEDefault )
            {
                pfcb->SetPgnoNextAvailSE( pcspaei->PgnoLast() - pcspaei->CpgExtent() + 1 );
                fFoundNextAvailSE = fTrue;
            }

            if ( pcspaei->CpgExtent() >= cpgMin )
            {
                if ( !fFoundNextAvailSE
                    && pfcb->PgnoNextAvailSE() <= pcspaei->PgnoLast() )
                {
                    pfcb->SetPgnoNextAvailSE( pcspaei->PgnoLast() + 1 );
                }
                err = JET_errSuccess;
                goto HandleError;
            }
        }

        err = ErrBTNext( pfucbAE, fDIRNull );
    }
    while ( err >= 0 );

    if ( err != JET_errNoCurrentRecord )
    {
        OSTraceFMP( pfucbAE->ifmp, JET_tracetagSpaceInternal,
                    OSFormat( "ErrSPGetExt could not scan available extent tree. [ifmp=0x%x]\n", pfucbAE->ifmp ) );
        Assert( err < 0 );
        goto HandleError;
    }

    if ( !fFoundNextAvailSE )
    {
        pfcb->SetPgnoNextAvailSE( pcspaei->PgnoLast() + 1 );
    }

    err = ErrERRCheck( errSPNoSpaceForYou );

HandleError:
    if ( err >= JET_errSuccess )
    {
        Assert( pcspaei->FIsSet() );
        const ERR errT = ErrSPICheckExtentIsAllocatable( pfucbAE->ifmp, pcspaei->PgnoFirst(), pcspaei->CpgExtent() );
        if ( errT < JET_errSuccess )
        {
            err = errT;
        }
    }

    Assert( err != JET_errRecordNotFound );
    Assert( err != JET_errNoCurrentRecord );

    if ( err < JET_errSuccess )
    {
        pcspaei->Unset();
        BTUp( pfucbAE );
    }
    else
    {
        Assert( Pcsr( pfucbAE )->FLatched() );
    }

    return err;
}

LOCAL ERR ErrSPIGetExt(
    FCB         *pfcbDstTracingOnly,
    FUCB        *pfucbSrc,
    CPG         *pcpgReq,
    CPG         cpgMin,
    PGNO        *ppgnoFirst,
    ULONG       fSPFlags = 0,
    UINT        fPageFlags = 0,
    OBJID       *pobjidFDP = NULL,
    const BOOL  fMayViolateMaxSize = fFalse )
{
    ERR         err;
    PIB         * const ppib            = pfucbSrc->ppib;
    FCB         * const pfcb            = pfucbSrc->u.pfcb;
    FUCB        *pfucbAE                = pfucbNil;
    const BOOL  fSelfReserving          = ( fSPFlags & fSPSelfReserveExtent ) != 0;

    CSPExtentInfo cspaei;

#ifdef DEBUG
    Assert( *pcpgReq > 0 || ( (fSPFlags & fSPNewFDP) && *pcpgReq == 0 ) );
    Assert( *pcpgReq >= cpgMin );
    Assert( !FFUCBSpace( pfucbSrc ) );
    AssertSPIPfucbOnRoot( pfucbSrc );
#endif

#ifdef SPACECHECK
    Assert( !( ErrSPIValidFDP( ppib, pfucbParent->ifmp, PgnoFDP( pfucbParent ) ) < 0 ) );
#endif

    if ( fSPFlags & fSPNewFDP )
    {
        Assert( !fSelfReserving );
        const CPG   cpgFDPMin = ( fSPFlags & fSPMultipleExtent ?
                                        cpgMultipleExtentMin :
                                        cpgSingleExtentMin );
        cpgMin = max( cpgMin, cpgFDPMin );
        *pcpgReq = max( *pcpgReq, cpgMin );

        Assert( NULL != pobjidFDP );
    }

    Assert( cpgMin > 0 );

    if ( !pfcb->FSpaceInitialized() )
    {
        SPIInitFCB( pfucbSrc, fTrue );
    }

    if ( FSPIIsSmall( pfcb ) )
    {
        Expected( !fSelfReserving );
        Assert( !fMayViolateMaxSize );
        err = ErrSPISmallGetExt( pfucbSrc, pcpgReq, cpgMin, ppgnoFirst );
        if ( JET_errSuccess <= err )
        {
            Expected( err == JET_errSuccess );
            goto NewFDP;
        }

        if( errSPNoSpaceForYou == err )
        {
            CallR( ErrSPIConvertToMultipleExtent( pfucbSrc, *pcpgReq, cpgMin ) );
        }

        Assert( FSPExpectedError( err ) );
        Call( err );
    }

    CallR( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbAE ) );
    Assert( pfcb == pfucbAE->u.pfcb );

    if ( pfcb->FUtilizeParentSpace() ||
        fSelfReserving ||
        FCATBaseSystemFDP( pfcb->PgnoFDP() ) )
    {
        const CPG cpgMinT = fSelfReserving ? ( cpgMin + 1 ) : cpgMin;

        err = ErrSPIAEFindExt( pfucbAE, cpgMinT, spp::AvailExtLegacyGeneralPool, &cspaei );
        Assert( err != JET_errRecordNotFound );
        Assert( err != JET_errNoCurrentRecord );

        if ( err >= JET_errSuccess )
        {
            Assert( cspaei.CpgExtent() >= cpgMinT );
            const CPG cpgMax = fSelfReserving ? ( cspaei.CpgExtent() - 1 ) : cspaei.CpgExtent();
            *pcpgReq = min( cpgMax, *pcpgReq );
        }
        else if ( err != errSPNoSpaceForYou )
        {
            Call( err );
        }

    }
    else
    {
        err = ErrERRCheck( errSPNoSpaceForYou );
    }

    if ( err == errSPNoSpaceForYou )
    {
        BTUp( pfucbAE );


        if ( fSelfReserving )
        {
            Error( err );
        }

        if ( !FSPIParentIsFs( pfucbSrc ) )
        {
            Call( ErrSPIGetSe(
                        pfucbSrc,
                        pfucbAE,
                        *pcpgReq,
                        cpgMin,
                        fSPFlags & ( fSPSplitting | fSPExactExtent ),
                        spp::AvailExtLegacyGeneralPool,
                        fMayViolateMaxSize ) );
        }
        else
        {
            Call( ErrSPIGetFsSe(
                        pfucbSrc,
                        pfucbAE,
                        *pcpgReq,
                        cpgMin,
                        fSPFlags & ( fSPSplitting | fSPExactExtent ),
                        fFalse,
                        fTrue,
                        fMayViolateMaxSize ) );
        }

        Assert( Pcsr( pfucbAE )->FLatched() );
        cspaei.Set( pfucbAE );
        Assert( cspaei.CpgExtent() > 0 );
        Assert( cspaei.CpgExtent() >= cpgMin );
    }

    err = cspaei.ErrCheckCorrupted();
    if ( err < JET_errSuccess )
    {
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"8aed90f2-6b6f-4bc4-902e-5ef1f328ebc3" );
    }
    Call( err );

    Assert( Pcsr( pfucbAE )->FLatched() );

    *ppgnoFirst = cspaei.PgnoFirst();

    const CPG cpgDb = (CPG)g_rgfmp[pfucbAE->ifmp].PgnoLast();

    if ( pfcb->PgnoFDP() == pgnoSystemRoot )
    {
        OSTraceFMP( pfucbSrc->ifmp, JET_tracetagSpaceInternal,
                        OSFormat( "found root space for alloc: %lu at %lu ... %d, %d\n",
                        cspaei.CpgExtent(), cspaei.PgnoFirst(),
                        (LONG)UlParam( PinstFromPpib( ppib ), JET_paramPageFragment ), cpgDb ) );
    }

    Assert( *pcpgReq >= cpgMin );
    Assert( cspaei.CpgExtent() >= cpgMin );
    Assert( !fSelfReserving || ( cspaei.CpgExtent() > *pcpgReq ) );
    if ( FSPIAllocateAllAvail( &cspaei, *pcpgReq, (LONG)UlParam( PinstFromPpib( ppib ), JET_paramPageFragment ), pfcb->PgnoFDP(), cpgDb, fSPFlags )
            && !fSelfReserving )
    {
        *pcpgReq = cspaei.CpgExtent();
        SPCheckPgnoAllocTrap( *ppgnoFirst, *pcpgReq );

        Assert( cspaei.CpgExtent() > 0 );

        Call( ErrBTFlagDelete(
                    pfucbAE,
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
    }
    else
    {
        Assert( cspaei.CpgExtent() > *pcpgReq );
        SPCheckPgnoAllocTrap( *ppgnoFirst, *pcpgReq );

        CSPExtentNodeKDF spAdjustedSize( SPEXTKEY::fSPExtentTypeAE, cspaei.PgnoLast(), cspaei.CpgExtent(), spp::AvailExtLegacyGeneralPool );

        OnDebug( const PGNO pgnoLastBefore = cspaei.PgnoLast() );
        Call( spAdjustedSize.ErrConsumeSpace( *ppgnoFirst, *pcpgReq ) );
        Assert( spAdjustedSize.CpgExtent() > 0 );
        Assert( pgnoLastBefore == cspaei.PgnoLast() );

        Call( ErrBTReplace(
                    pfucbAE,
                    spAdjustedSize.GetData( ),
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
        CallS( err );
        err = JET_errSuccess;
    }

    BTUp( pfucbAE );

    if ( ( pgnoSystemRoot == pfucbAE->u.pfcb->PgnoFDP() )
        && g_rgfmp[pfucbAE->u.pfcb->Ifmp()].FCacheAvail() )
    {
        g_rgfmp[pfucbAE->u.pfcb->Ifmp()].AdjustCpgAvail( -(*pcpgReq) );
    }

NewFDP:
    if ( fSPFlags & fSPNewFDP )
    {
        Assert( PgnoFDP( pfucbSrc ) != *ppgnoFirst );
        if ( fSPFlags & fSPMultipleExtent )
        {
            Assert( *pcpgReq >= cpgMultipleExtentMin );
        }
        else
        {
            Assert( *pcpgReq >= cpgSingleExtentMin );
        }

        Assert( pgnoSystemRoot != *ppgnoFirst );

        VEREXT  verext;
        verext.pgnoFDP = PgnoFDP( pfucbSrc );
        verext.pgnoChildFDP = *ppgnoFirst;
        verext.pgnoFirst = *ppgnoFirst;
        verext.cpgSize = *pcpgReq;

        if ( !( fSPFlags & fSPUnversionedExtent ) )
        {
            VER *pver = PverFromIfmp( pfucbSrc->ifmp );
            Call( pver->ErrVERFlag( pfucbSrc, operAllocExt, &verext, sizeof(verext) ) );
        }

        Assert( PgnoFDP( pfucbSrc ) != pgnoNull );
        Assert( *ppgnoFirst != pgnoSystemRoot );
        Call( ErrSPCreate(
                    ppib,
                    pfucbSrc->ifmp,
                    PgnoFDP( pfucbSrc ),
                    *ppgnoFirst,
                    *pcpgReq,
                    fSPFlags,
                    fPageFlags,
                    pobjidFDP ) );
        Assert( *pobjidFDP > objidSystemRoot );

        if ( fSPFlags & fSPMultipleExtent )
        {
            (*pcpgReq) -= cpgMultipleExtentMin;
        }
        else
        {
            (*pcpgReq) -= cpgSingleExtentMin;
        }

#ifdef DEBUG
        if ( 0 == ppib->Level() )
        {
            FMP     *pfmp   = &g_rgfmp[pfucbSrc->ifmp];
            Assert( fSPFlags & fSPUnversionedExtent );
            Assert( pfmp->FIsTempDB() || pfmp->FCreatingDB() );
        }
#endif
    }


    err = JET_errSuccess;

    CPG cpgExt = *pcpgReq + ( (fSPFlags & fSPNewFDP) ?
                                ( fSPFlags & fSPMultipleExtent ? cpgMultipleExtentMin : cpgSingleExtentMin ):
                                0
                            );
    const PGNO pgnoParentFDP = PgnoFDP( pfucbSrc );
    const PGNO pgnoFDP = pfcbDstTracingOnly->PgnoFDP();
    ETSpaceAllocExt( pfucbSrc->ifmp, pgnoFDP, *ppgnoFirst, cpgExt, pfcbDstTracingOnly->ObjidFDP(), pfcbDstTracingOnly->TCE() );
    OSTraceFMP( pfucbSrc->ifmp, JET_tracetagSpaceManagement,
                    OSFormat( "ErrSPIGetExt: %hs from %d.%lu for %lu at %lu for %lu pages (0x%x)\n",
                        fSPFlags & fSPNewFDP ? "NewExtFDP" : "AddlExt",
                        pfucbSrc->ifmp,
                        pgnoParentFDP,
                        pgnoFDP,
                        *ppgnoFirst,
                        cpgExt,
                        fSPFlags ) );

    Assert( cpgExt == ( (fSPFlags & fSPNewFDP) ?
                        *pcpgReq + ( (fSPFlags & fSPMultipleExtent) ? cpgMultipleExtentMin : cpgSingleExtentMin ) : *pcpgReq ) );

    if ( pgnoParentFDP == pgnoSystemRoot )
    {
        TLS* const ptls = Ptls();
        ptls->threadstats.cPageTableAllocated += cpgExt;
    }

HandleError:
    if ( pfucbAE != pfucbNil )
    {
        BTClose( pfucbAE );
    }

    return err;
}


ERR ErrSPGetExt(
    FUCB    *pfucb,
    PGNO    pgnoParentFDP,
    CPG     *pcpgReq,
    CPG     cpgMin,
    PGNO    *ppgnoFirst,
    ULONG   fSPFlags,
    UINT    fPageFlags,
    OBJID   *pobjidFDP )
{
    ERR     err;
    FUCB    *pfucbParent = pfucbNil;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortSpace );

    Assert( !g_rgfmp[ pfucb->ifmp ].FReadOnlyAttach() );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( 0 == ( fSPFlags & ~fMaskSPGetExt ) );

    CallR( ErrBTIOpenAndGotoRoot( pfucb->ppib, pgnoParentFDP, pfucb->ifmp, &pfucbParent ) );

    err = ErrSPIGetExt( pfucb->u.pfcb, pfucbParent, pcpgReq, cpgMin, ppgnoFirst, fSPFlags, fPageFlags, pobjidFDP );
    Assert( Pcsr( pfucbParent ) == pfucbParent->pcsrRoot );

    Assert( pfucbParent->pcsrRoot->Latch() == latchRIW
        || pfucbParent->pcsrRoot->Latch() == latchWrite );
    pfucbParent->pcsrRoot->ReleasePage();
    pfucbParent->pcsrRoot = pcsrNil;

    Assert( pfucbParent != pfucbNil );
    BTClose( pfucbParent );
    return err;
}


ERR ErrSPISPGetPage(
    __inout FUCB *          pfucb,
    __out   PGNO *          ppgnoAlloc )
{
    ERR         err = JET_errSuccess;
    const BOOL  fAvailExt   = FFUCBAvailExt( pfucb );

    Assert( FFUCBSpace( pfucb ) );
    Assert( ppgnoAlloc );
    Assert( pgnoNull == *ppgnoAlloc );
    AssertSPIPfucbOnSpaceTreeRoot( pfucb, pfucb->pcsrRoot );

    if ( NULL == pfucb->u.pfcb->Psplitbuf( fAvailExt ) )
    {
        CSR             *pcsrRoot   = pfucb->pcsrRoot;
        SPLIT_BUFFER    spbuf;
        DATA            data;

        UtilMemCpy( &spbuf, PspbufSPISpaceTreeRootPage( pfucb, pcsrRoot ), sizeof(SPLIT_BUFFER) );

        CallR( spbuf.ErrGetPage( pfucb->ifmp, ppgnoAlloc, fAvailExt ) );

        Assert( latchRIW == pcsrRoot->Latch() );
        pcsrRoot->UpgradeFromRIWLatch();

        data.SetPv( &spbuf );
        data.SetCb( sizeof(spbuf) );
        err = ErrNDSetExternalHeader(
                    pfucb,
                    pcsrRoot,
                    &data,
                    ( pfucb->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    noderfWhole );

        pcsrRoot->Downgrade( latchRIW );

    }
    else
    {
        SPITraceSplitBufferMsg( pfucb, "Retrieved page from" );
        err = pfucb->u.pfcb->Psplitbuf( fAvailExt )->ErrGetPage( pfucb->ifmp, ppgnoAlloc, fAvailExt );
    }

    Assert( ( err < JET_errSuccess ) || !g_rgfmp[ pfucb->ifmp ].FBeyondPgnoShrinkTarget( *ppgnoAlloc, 1 ) );
    Assert( ( err < JET_errSuccess ) || ( *ppgnoAlloc <= g_rgfmp[ pfucb->ifmp ].PgnoLast() ) );

    return err;
}

ERR ErrSPISmallGetPage(
    __inout FUCB *      pfucb,
    __in    PGNO        pgnoLast,
    __out   PGNO *      ppgnoAlloc )
{
    ERR             err = JET_errSuccess;
    SPACE_HEADER    sph;
    UINT            rgbitT;
    PGNO            pgnoAvail;
    DATA            data;
    const FCB * const   pfcb            = pfucb->u.pfcb;

    Assert( NULL != ppgnoAlloc );
    Assert( pgnoNull == *ppgnoAlloc );


    Assert( pfcb->FSpaceInitialized() );
    Assert( FSPIIsSmall( pfcb ) );

    AssertSPIPfucbOnRoot( pfucb );

    NDGetExternalHeader( pfucb, pfucb->pcsrRoot, noderfSpaceHeader );
    Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );

    UtilMemCpy( &sph, pfucb->kdfCurr.data.Pv(), sizeof(sph) );

    const PGNO  pgnoFDP         = PgnoFDP( pfucb );
    const CPG   cpgSingleExtent = min( sph.CpgPrimary(), cpgSmallSpaceAvailMost+1 );
    Assert( cpgSingleExtent > 0 );
    Assert( cpgSingleExtent <= cpgSmallSpaceAvailMost+1 );

    for( pgnoAvail = pgnoFDP + 1, rgbitT = 1;
        pgnoAvail <= pgnoFDP + cpgSingleExtent - 1;
        pgnoAvail++, rgbitT <<= 1 )
    {
        Assert( rgbitT != 0 );
        if ( rgbitT & sph.RgbitAvail() )
        {
            Assert( ( rgbitT & sph.RgbitAvail() ) == rgbitT );

            SPIUpgradeToWriteLatch( pfucb );

            sph.SetRgbitAvail( sph.RgbitAvail() ^ rgbitT );
            data.SetPv( &sph );
            data.SetCb( sizeof(sph) );
            CallR( ErrNDSetExternalHeader(
                        pfucb,
                        pfucb->pcsrRoot,
                        &data,
                        ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                        noderfSpaceHeader ) );

            Assert( !g_rgfmp[ pfucb->ifmp ].FBeyondPgnoShrinkTarget( pgnoAvail, 1 ) );

            Assert( pgnoAvail <= g_rgfmp[ pfucb->ifmp ].PgnoLast() );

            *ppgnoAlloc = pgnoAvail;
            SPCheckPgnoAllocTrap( *ppgnoAlloc );

            ETSpaceAllocPage( pfucb->ifmp, pgnoFDP, *ppgnoAlloc, pfucb->u.pfcb->ObjidFDP(), TceFromFUCB( pfucb ) );
            OSTraceFMP( pfucb->ifmp, JET_tracetagSpace,
                            OSFormat( "get page 1 at %lu from %d.%lu\n", *ppgnoAlloc, pfucb->ifmp, PgnoFDP( pfucb ) ) );

            return JET_errSuccess;
        }
    }

    return ErrERRCheck( errSPNoSpaceForYou );
}


enum SPFindFlags {
    fSPFindContinuousPage   = 0x1,
    fSPFindAnyGreaterPage   = 0x2
};

ERR ErrSPIAEFindPage(
    __inout FUCB * const        pfucbAE,
    __in    const SPFindFlags   fSPFindFlags,
    __in    const SpacePool     sppAvailPool,
    __in    const PGNO          pgnoLast,
    __out   CSPExtentInfo *     pspaeiAlloc,
    __out   CPG *               pcpgFindInsertionRegionMarker
    )
{
    ERR             err             = JET_errSuccess;
    FCB * const     pfcb            = pfucbAE->u.pfcb;
    DIB             dib;

    Assert( pfucbAE );
    Assert( pspaeiAlloc );
    Assert( FFUCBSpace( pfucbAE ) );
    Assert( !FSPIIsSmall( pfcb ) );

    pspaeiAlloc->Unset();
    if ( pcpgFindInsertionRegionMarker )
    {
        *pcpgFindInsertionRegionMarker = 0;
    }

    CSPExtentKeyBM  spaeFindPage( SPEXTKEY::fSPExtentTypeAE, pgnoLast, sppAvailPool );
    dib.pos = posDown;
    dib.pbm = spaeFindPage.Pbm( pfucbAE );


    spaeFindPage.SPExtentMakeKeyBM( SPEXTKEY::fSPExtentTypeAE, pgnoLast, sppAvailPool );

    dib.dirflag = fDIRNull;

    err = ErrBTDown( pfucbAE, &dib, latchReadTouch );

    switch ( err )
    {

        default:
            Assert( err < JET_errSuccess );
            Assert( err != JET_errNoCurrentRecord );
            OSTraceFMP( pfucbAE->ifmp, JET_tracetagSpaceInternal,
                        OSFormat( "ErrSPGetPage could not go down into available extent tree. [ifmp=0x%x]\n", pfucbAE->ifmp ) );
            Call( err );
            break;

        case JET_errRecordNotFound:

            Error( ErrERRCheck( errSPNoSpaceForYou ) );
            break;

        case JET_errSuccess:

            pspaeiAlloc->Set( pfucbAE );


            if ( !( fSPFindContinuousPage & fSPFindFlags ) ||
                    ( spp::ContinuousPool != sppAvailPool ) ||
                    ( !pspaeiAlloc->FEmptyExtent()  ) ||
                    ( pspaeiAlloc->SppPool() != sppAvailPool ) )
            {
                AssertSz( fFalse, "AvailExt corrupted." );
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"bc14a209-3a29-4f60-957b-3e847f0eefd0" );
                Call( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
            }

            Assert( sppAvailPool == spp::ContinuousPool );
            Assert( fSPFindContinuousPage & fSPFindFlags );

            {
            CSPExtentInfo spoePreviousExtSize;
            Call( ErrSPIFindExtOE( pfucbAE->ppib, pfcb, pgnoLast, &spoePreviousExtSize ) );
            if ( !spoePreviousExtSize.FContains( pgnoLast ) )
            {
                AssertSz( fFalse, "OwnExt corrupted." );
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"bfc12437-d215-4768-a6e9-3e534da76285" );
                Call( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
            }

            if ( NULL == pcpgFindInsertionRegionMarker )
            {
                spoePreviousExtSize.Unset();
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            Assert( pcpgFindInsertionRegionMarker );

            *pcpgFindInsertionRegionMarker = spoePreviousExtSize.CpgExtent();
            spoePreviousExtSize.Unset();
            pspaeiAlloc->Unset();
            err = JET_errSuccess;
            goto HandleError;
            }
            break;

        case wrnNDFoundLess:


            pspaeiAlloc->Set( pfucbAE );

            if ( pgnoNull != pgnoLast &&
                pspaeiAlloc->SppPool() == sppAvailPool &&
                ( fSPFindFlags & fSPFindAnyGreaterPage ) )
            {

                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }


        case wrnNDFoundGreater:

            pspaeiAlloc->Set( pfucbAE );


            if ( pspaeiAlloc->SppPool() != sppAvailPool )
            {
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            if ( pspaeiAlloc->FEmptyExtent() )
            {
                Assert( fSPFindContinuousPage & fSPFindFlags );
                Assert( spp::ContinuousPool == pspaeiAlloc->SppPool() );

#ifdef DEPRECATED_ZERO_SIZED_AVAIL_EXTENT_CLEANUP
                if ( pspaeiAlloc->Pool() != spp::ContinuousPool &&
                    0 == pspaeiAlloc->CpgExtent() )
                {
                    Call( ErrBTFlagDelete(
                                pfucbAE,
                                fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
                    BTUp( pfucbAE );
                    goto FindPage;
                }
#endif

                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            if ( ( fSPFindContinuousPage & fSPFindFlags ) &&
                    ( pspaeiAlloc->PgnoFirst() != ( pgnoLast + 1 ) ) )
            {
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            if ( g_rgfmp[ pfucbAE->ifmp ].FBeyondPgnoShrinkTarget( pspaeiAlloc->PgnoFirst(), pspaeiAlloc->CpgExtent() ) )
            {
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            if ( pspaeiAlloc->PgnoLast() > g_rgfmp[ pfucbAE->ifmp ].PgnoLast() )
            {
                Error( ErrERRCheck( errSPNoSpaceForYou ) );
            }

            break;
    }


    err = JET_errSuccess;

HandleError:
    if ( ( err >= JET_errSuccess ) && pspaeiAlloc->FIsSet() )
    {
        const ERR errT = ErrSPICheckExtentIsAllocatable( pfucbAE->ifmp, pspaeiAlloc->PgnoFirst(), pspaeiAlloc->CpgExtent() );
        if ( errT < JET_errSuccess )
        {
            err = errT;
        }
    }

    if ( err < JET_errSuccess )
    {

        pspaeiAlloc->Unset();

        BTUp( pfucbAE );

        Assert( !Pcsr( pfucbAE )->FLatched() );

    }
    else
    {

        Assert( Pcsr( pfucbAE )->FLatched() );

        if ( NULL == pcpgFindInsertionRegionMarker || 0 == *pcpgFindInsertionRegionMarker )
        {

            Assert( pspaeiAlloc->SppPool() == sppAvailPool );
            Assert( pspaeiAlloc->PgnoLast() && pspaeiAlloc->PgnoLast() != pgnoBadNews );
            Assert( !pspaeiAlloc->FEmptyExtent() );
        }
        else
        {

            Assert( *pcpgFindInsertionRegionMarker );
        }
    }

    return err;
}

ERR ErrSPIAEGetExtentAndPage(
    __inout FUCB * const        pfucb,
    __inout FUCB * const        pfucbAE,
    __in    const SpacePool     sppAvailPool,
    __in    const CPG           cpgRequest,
    __in    const ULONG         fSPFlags,
    __out   CSPExtentInfo *     pspaeiAlloc
    )
{
    ERR             err             = JET_errSuccess;

    Assert( cpgRequest );
    Assert( fSPFlags );
    Assert( !Pcsr( pfucbAE )->FLatched() );
    BTUp( pfucbAE );


    Assert( !FSPIParentIsFs( pfucb ) );
    if ( !FSPIParentIsFs( pfucb ) )
    {
        Call( ErrSPIGetSe( pfucb, pfucbAE, cpgRequest, cpgRequest, fSPFlags, sppAvailPool ) );
    }
    else
    {
        Call( ErrSPIGetFsSe( pfucb, pfucbAE, cpgRequest, cpgRequest, fSPFlags ) );
    }

    Assert( Pcsr( pfucbAE )->FLatched() );


    pspaeiAlloc->Set( pfucbAE );


    Assert( pspaeiAlloc->SppPool() == sppAvailPool );

HandleError:

    return err;
}

ERR ErrSPIAEGetContinuousPage(
    __inout FUCB * const        pfucb,
    __inout FUCB * const        pfucbAE,
    __in    const PGNO          pgnoLast,
    __in    const CPG           cpgReserve,
    __in    const BOOL          fHardReserve,
    __out   CSPExtentInfo *     pspaeiAlloc
    )
{
    ERR             err             = JET_errSuccess;
    FCB * const     pfcb            = pfucbAE->u.pfcb;
    CPG             cpgEscalatingRequest = 0;

    Assert( pfucb );
    Assert( pfucbAE );
    Assert( pspaeiAlloc );


    err = ErrSPIAEFindPage( pfucbAE, fSPFindContinuousPage, spp::ContinuousPool, pgnoLast, pspaeiAlloc, &cpgEscalatingRequest );

    if ( cpgEscalatingRequest )
    {

        CallS( err );
        Assert( Pcsr( pfucbAE )->FLatched() );

        cpgEscalatingRequest = CpgSPIGetNextAlloc( pfcb->Pfcbspacehints(), cpgEscalatingRequest );

        err = ErrBTFlagDelete(
                    pfucbAE,
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) );

        BTUp( pfucbAE );

        Assert( !Pcsr( pfucbAE )->FLatched() );

        Call( err );

        err = ErrERRCheck( errSPNoSpaceForYou );

    }
    else
    {
        cpgEscalatingRequest = 1;
    }

    if ( fHardReserve &&
            err >= JET_errSuccess &&
            pspaeiAlloc->CpgExtent() < cpgReserve )
    {
        pspaeiAlloc->Unset();
        BTUp( pfucbAE );
        Assert( !Pcsr( pfucbAE )->FLatched() );
        err = ErrERRCheck( errSPNoSpaceForYou );
    }

    if ( errSPNoSpaceForYou == err )
    {

        if ( cpgEscalatingRequest == 1 )
        {
            if ( cpgReserve )
            {
                cpgEscalatingRequest += cpgReserve;
            }
            else
            {
                cpgEscalatingRequest = CpgSPIGetNextAlloc( pfcb->Pfcbspacehints(), cpgEscalatingRequest );
            }
        }
        
        if ( fHardReserve &&
                cpgReserve != 0 &&
                cpgReserve != cpgEscalatingRequest )
        {
            const CPG cpgFullReserveRequest = ( cpgReserve + 1 );
            if ( ( cpgEscalatingRequest % cpgFullReserveRequest ) != 0 )
            {
                cpgEscalatingRequest = cpgFullReserveRequest;
            }
        }


        Call( ErrSPIAEGetExtentAndPage( pfucb, pfucbAE,
                            spp::ContinuousPool,
                            cpgEscalatingRequest,
                            fSPSplitting,
                            pspaeiAlloc ) );

    }

    CallS( err );
    Call( err );

    Assert( Pcsr( pfucbAE )->FLatched() );
    Assert( pspaeiAlloc->SppPool() == spp::ContinuousPool );
    Assert( pspaeiAlloc->PgnoFirst() && pspaeiAlloc->PgnoFirst() != pgnoBadNews );
    Assert( !pspaeiAlloc->FEmptyExtent() );

HandleError:

    Assert( JET_errSuccess == err || !Pcsr( pfucbAE )->FLatched() );
    Assert( err < JET_errSuccess || Pcsr( pfucbAE )->FLatched() );

    return err;
}


ERR ErrSPIAEGetAnyPage(
    __inout FUCB * const        pfucb,
    __inout FUCB * const        pfucbAE,
    __in    const PGNO          pgnoLastHint,
    __in    const BOOL          fSPAllocFlags,
    __out   CSPExtentInfo *     pspaeiAlloc
    )
{
    ERR             err             = JET_errSuccess;

    Assert( pfucb );
    Assert( pfucbAE );
    Assert( pspaeiAlloc );

    err = ErrSPIAEFindPage( pfucbAE, fSPFindContinuousPage, spp::ContinuousPool, pgnoLastHint, pspaeiAlloc, NULL );

    if ( errSPNoSpaceForYou == err )
    {
        err = ErrSPIAEFindPage( pfucbAE, fSPFindAnyGreaterPage, spp::AvailExtLegacyGeneralPool, pgnoLastHint, pspaeiAlloc, NULL );

        if ( errSPNoSpaceForYou == err )
        {
            err = ErrSPIAEFindPage( pfucbAE, fSPFindAnyGreaterPage, spp::AvailExtLegacyGeneralPool, pgnoNull, pspaeiAlloc, NULL );
        }
    }

    if ( errSPNoSpaceForYou == err )
    {

        Call( ErrSPIAEGetExtentAndPage( pfucb, pfucbAE,
                            spp::AvailExtLegacyGeneralPool,
                            1,
                            fSPAllocFlags | fSPSplitting | fSPOriginatingRequest,
                            pspaeiAlloc ) );

        Assert( Pcsr( pfucbAE )->FLatched() );
    }

    Call( err );

    Assert( Pcsr( pfucbAE )->FLatched() );
    Assert( pspaeiAlloc->PgnoFirst() && pspaeiAlloc->PgnoFirst() != pgnoBadNews );
    Assert( !pspaeiAlloc->FEmptyExtent() );

HandleError:

    Assert( JET_errSuccess == err || !Pcsr( pfucbAE )->FLatched() );
    Assert( err < JET_errSuccess || Pcsr( pfucbAE )->FLatched() );

    return err;
}


ERR ErrSPIAEGetPage(
    __inout FUCB *      pfucb,
    __in    PGNO        pgnoLast,
    __inout PGNO *      ppgnoAlloc,
    __in    const BOOL  fSPAllocFlags,
    __in    const CPG   cpgReserve
    )
{
    ERR             err             = JET_errSuccess;
    FCB * const     pfcb            = pfucb->u.pfcb;
    FUCB *          pfucbAE         = pfucbNil;
    CSPExtentInfo   cspaeiAlloc;

    Assert( 0 == ( fSPAllocFlags & ~( fSPContinuous | fSPUseActiveReserve | fSPExactExtent ) ) );

    AssertSPIPfucbOnRoot( pfucb );

    CallR( ErrSPIOpenAvailExt( pfucb->ppib, pfcb, &pfucbAE ) );
    Assert( pfcb == pfucbAE->u.pfcb );
    Assert( pfucb->ppib == pfucbAE->ppib );

    if ( fSPContinuous & fSPAllocFlags )
    {
        Call( ErrSPIAEGetContinuousPage( pfucb, pfucbAE, pgnoLast, cpgReserve, fSPAllocFlags & fSPUseActiveReserve, &cspaeiAlloc ) );
    }
    else
    {
        Call( ErrSPIAEGetAnyPage( pfucb, pfucbAE, pgnoLast, fSPAllocFlags & fSPExactExtent, &cspaeiAlloc ) );
    }

    Assert( err >= 0 );
    Assert( Pcsr( pfucbAE )->FLatched() );

    err = cspaeiAlloc.ErrCheckCorrupted();
    CallS( err );
    if ( err < JET_errSuccess )
    {
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"4fbc1735-8688-40ae-898f-40e86deb52c4" );
    }
    Call( err );
    Assert( cspaeiAlloc.PgnoFirst() && cspaeiAlloc.PgnoFirst() != pgnoBadNews );
    Assert( !cspaeiAlloc.FEmptyExtent() );
    Assert( cspaeiAlloc.CpgExtent() > 0 );


    if ( cspaeiAlloc.FContains( pgnoLast ) )
    {
        AssertSz( fFalse, "AvailExt corrupted." );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"ad23b08a-84d6-4d21-aefc-746560b0eceb" );
        Call( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
    }

    *ppgnoAlloc = cspaeiAlloc.PgnoFirst();
    SPCheckPgnoAllocTrap( *ppgnoAlloc );

    Assert( *ppgnoAlloc != pgnoLast );


    {
    CSPExtentNodeKDF        spAdjustedAvail( SPEXTKEY::fSPExtentTypeAE,
                                                cspaeiAlloc.PgnoLast(),
                                                cspaeiAlloc.CpgExtent(),
                                                cspaeiAlloc.SppPool() );

    Call( spAdjustedAvail.ErrConsumeSpace( cspaeiAlloc.PgnoFirst() ) );

    if ( spAdjustedAvail.FDelete() )
    {
        Call( ErrBTFlagDelete(
                    pfucbAE,
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
    }
    else
    {
        Assert( FKeysEqual( pfucbAE->kdfCurr.key, spAdjustedAvail.GetKey() ) );

        Call( ErrBTReplace(
                    pfucbAE,
                    spAdjustedAvail.GetData( ),
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
    }
    }

    BTUp( pfucbAE );
    err = JET_errSuccess;
    
    if ( ( pgnoSystemRoot == pfucbAE->u.pfcb->PgnoFDP() )
        && g_rgfmp[pfucbAE->u.pfcb->Ifmp()].FCacheAvail() )
    {
        g_rgfmp[pfucbAE->u.pfcb->Ifmp()].AdjustCpgAvail( -1 );
    }

HandleError:

    if ( pfucbAE != pfucbNil )
    {
        BTClose( pfucbAE );
    }

    return err;
}

CPG CpgSPIConsumeActiveSpaceRequestReserve( FUCB * const pfucb )
{
    Assert( !FFUCBSpace( pfucb ) );


    const CPG cpgAddlReserve = CpgDIRActiveSpaceRequestReserve( pfucb );
    DIRSetActiveSpaceRequestReserve( pfucb, cpgDIRReserveConsumed );
    Assert( CpgDIRActiveSpaceRequestReserve( pfucb ) == cpgDIRReserveConsumed );

    AssertSz( cpgAddlReserve != cpgDIRReserveConsumed, "The active space reserve should only be consumed one time." );

    return cpgAddlReserve;
}


ERR ErrSPGetPage(
    __inout FUCB *          pfucb,
    __in    const PGNO      pgnoLast,
    __in    const ULONG     fSPAllocFlags,
    __out   PGNO *          ppgnoAlloc
    )
{
    ERR         err = errCodeInconsistency;
    FCB         * const pfcb            = pfucb->u.pfcb;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortSpace );

    Assert( ppgnoAlloc != NULL );
    Assert( 0 == ( fSPAllocFlags & ~fMaskSPGetPage ) );

    CPG         cpgAddlReserve = 0;
    if ( fSPAllocFlags & fSPNewExtent )
    {
        cpgAddlReserve = 15;
    }

    if ( ( fSPAllocFlags & fSPContinuous ) && ( fSPAllocFlags & fSPUseActiveReserve ) )
    {
        cpgAddlReserve = CpgSPIConsumeActiveSpaceRequestReserve( pfucb );
    }

    if ( FFUCBSpace( pfucb ) )
    {
        err = ErrSPISPGetPage( pfucb, ppgnoAlloc );
    }
    else
    {
#ifdef SPACECHECK
        Assert( !( ErrSPIValidFDP( pfucb->ppib, pfucb->ifmp, PgnoFDP( pfucb ) ) < 0 ) );
        if ( pgnoNull != pgnoLast )
        {
            CallS( ErrSPIWasAlloc( pfucb, pgnoLast, (CPG)1 ) );
        }
#endif

        if ( !pfcb->FSpaceInitialized() )
        {
            SPIInitFCB( pfucb, fTrue );
        }

        if ( FSPIIsSmall( pfcb ) )
        {
            err = ErrSPISmallGetPage( pfucb, pgnoLast, ppgnoAlloc );
            if ( errSPNoSpaceForYou != err )
            {
                Assert( FSPExpectedError( err ) );
                goto HandleError;
            }

            Call( ErrSPIConvertToMultipleExtent( pfucb, 1, 1 ) );
        }

        Call( ErrSPIAEGetPage(
                    pfucb,
                    pgnoLast,
                    ppgnoAlloc,
                    fSPAllocFlags & ( fSPContinuous | fSPUseActiveReserve | fSPExactExtent ),
                    cpgAddlReserve ) );
    }

HandleError:

    if ( err < JET_errSuccess )
    {
        OSTraceFMP( pfucb->ifmp, JET_tracetagSpaceInternal,
                        OSFormat( "Failed to get page from %d.%lu\n", pfucb->ifmp, PgnoFDP( pfucb ) ) );
        if ( errSPNoSpaceForYou == err ||
                errCodeInconsistency == err )
        {
            AssertSz( fFalse, "Code malfunction." );
            err = ErrERRCheck( JET_errInternalError );
        }
    }
    else
    {
        Assert( FSPValidAllocPGNO( *ppgnoAlloc ) );

        const PGNO pgnoFDP = PgnoFDP( pfucb );
        ETSpaceAllocPage( pfucb->ifmp, pgnoFDP, *ppgnoAlloc, pfucb->u.pfcb->ObjidFDP(), TceFromFUCB( pfucb ) );
        OSTraceFMP( pfucb->ifmp, JET_tracetagSpace,
                        OSFormat( "get page 1 at %lu from %d.%lu\n", *ppgnoAlloc, pfucb->ifmp, PgnoFDP( pfucb ) ) );
    }

    return err;
}


LOCAL ERR ErrSPIFreeSEToParent(
    FUCB        *pfucb,
    FUCB        *pfucbOE,
    FUCB        *pfucbAE,
    FUCB* const pfucbParent,
    const PGNO  pgnoLast,
    const CPG   cpgSize )
{
    ERR         err;
    FCB         *pfcb = pfucb->u.pfcb;
    FCB         *pfcbParent;
    BOOL        fState;
    DIB         dib;
    FUCB        *pfucbParentLocal = pfucbParent;
    const CSPExtentKeyBM    CSPOEKey( SPEXTKEY::fSPExtentTypeOE, pgnoLast );

    Assert( pfcbNil != pfcb );
    AssertSPIPfucbOnRoot( pfucb );
    AssertSPIPfucbOnRootOrNull( pfucbParent );

    const PGNO  pgnoParentFDP = PgnoSPIParentFDP( pfucb );
    if ( pgnoParentFDP == pgnoNull )
    {
        Assert( pfucb->u.pfcb->PgnoFDP() == pgnoSystemRoot );
        return JET_errSuccess;
    }

    pfcbParent = FCB::PfcbFCBGet( pfucb->ifmp, pgnoParentFDP, &fState, fTrue, fTrue );
    Assert( pfcbParent != pfcbNil );
    Assert( fFCBStateInitialized == fState );
    Assert( !pfcb->FTypeNull() );

    if ( !pfcb->FInitedForRecovery() )
    {
        if ( pfcb->FTypeSecondaryIndex() || pfcb->FTypeLV() )
        {
            Assert( pfcbParent->FTypeTable() );
        }
        else
        {
            Assert( pfcbParent->FTypeDatabase() );
            Assert( !pfcbParent->FDeletePending() );
            Assert( !pfcbParent->FDeleteCommitted() );
        }
    }

    if ( pfucbAE != pfucbNil )
    {

        Call( ErrBTFlagDelete(
                    pfucbAE,
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
        BTUp( pfucbAE );
    }

    dib.pos = posDown;
    dib.pbm = CSPOEKey.Pbm( pfucbOE );
    dib.dirflag = fDIRNull;
    Call( ErrBTDown( pfucbOE, &dib, latchReadTouch ) );

    Call( ErrBTFlagDelete(
                pfucbOE,
                fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
    BTUp( pfucbOE );

    if ( pfucbParentLocal == pfucbNil )
    {
        Call( ErrBTIOpenAndGotoRoot( pfucb->ppib, pgnoParentFDP, pfucb->ifmp, &pfucbParentLocal ) );
    }
    Call( ErrSPFreeExt( pfucbParentLocal, pgnoLast - cpgSize + 1, cpgSize, "FreeToParent" ) );

    if ( pgnoParentFDP == pgnoSystemRoot )
    {
        TLS* const ptls = Ptls();
        ptls->threadstats.cPageTableReleased += cpgSize;
    }

HandleError:
    AssertSPIPfucbOnRoot( pfucb );
    AssertSPIPfucbOnRootOrNull( pfucbParent );

    if ( ( pfucbParentLocal != pfucbNil ) && ( pfucbParentLocal != pfucbParent ) )
    {
        Expected( pfucbParent == pfucbNil );
        AssertSPIPfucbOnRoot( pfucbParentLocal );
        pfucbParentLocal->pcsrRoot->ReleasePage();
        pfucbParentLocal->pcsrRoot = pcsrNil;
        BTClose( pfucbParentLocal );
        pfucbParentLocal = pfucbNil;
    }

    pfcbParent->Release();

    return err;
}


LOCAL_BROKEN VOID SPIReportLostPages(
    const IFMP  ifmp,
    const OBJID objidFDP,
    const PGNO  pgnoLast,
    const CPG   cpgLost )
{
    WCHAR       wszStartPagesLost[16];
    WCHAR       wszEndPagesLost[16];
    WCHAR       wszObjidFDP[16];
    const WCHAR *rgcwszT[4];

    OSStrCbFormatW( wszStartPagesLost, sizeof(wszStartPagesLost), L"%d", pgnoLast - cpgLost + 1 );
    OSStrCbFormatW( wszEndPagesLost, sizeof(wszEndPagesLost), L"%d", pgnoLast );
    OSStrCbFormatW( wszObjidFDP, sizeof(wszObjidFDP), L"%d", objidFDP );

    rgcwszT[0] = g_rgfmp[ifmp].WszDatabaseName();
    rgcwszT[1] = wszStartPagesLost;
    rgcwszT[2] = wszEndPagesLost;
    rgcwszT[3] = wszObjidFDP;

    UtilReportEvent(
            eventWarning,
            SPACE_MANAGER_CATEGORY,
            SPACE_LOST_ON_FREE_ID,
            4,
            rgcwszT,
            0,
            NULL,
            PinstFromIfmp( ifmp ) );
}

ERR ErrSPISPFreeExt( __inout FUCB * pfucb, __in const PGNO pgnoFirst, __in const CPG cpgSize )
{
    ERR         err         = JET_errSuccess;
    const BOOL  fAvailExt   = FFUCBAvailExt( pfucb );

    Assert( 1 == cpgSize );

    if ( NULL == pfucb->u.pfcb->Psplitbuf( fAvailExt ) )
    {
        CSR             *pcsrRoot   = pfucb->pcsrRoot;
        SPLIT_BUFFER    spbuf;
        DATA            data;

        AssertSPIPfucbOnSpaceTreeRoot( pfucb, pcsrRoot );

        UtilMemCpy( &spbuf, PspbufSPISpaceTreeRootPage( pfucb, pcsrRoot ), sizeof(SPLIT_BUFFER) );

        spbuf.ReturnPage( pgnoFirst );

        Assert( latchRIW == pcsrRoot->Latch() );
        pcsrRoot->UpgradeFromRIWLatch();

        data.SetPv( &spbuf );
        data.SetCb( sizeof(spbuf) );
        err = ErrNDSetExternalHeader(
                    pfucb,
                    pcsrRoot,
                    &data,
                    ( pfucb->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                    noderfWhole );

        pcsrRoot->Downgrade( latchRIW );
    }
    else
    {
        AssertSPIPfucbOnSpaceTreeRoot( pfucb, pfucb->pcsrRoot );
        pfucb->u.pfcb->Psplitbuf( fAvailExt )->ReturnPage( pgnoFirst );
        err = JET_errSuccess;
    }

    return err;
}

ERR ErrSPISmallFreeExt( __inout FUCB * pfucb, __in const PGNO pgnoFirst, __in const CPG cpgSize )
{
    ERR             err = JET_errSuccess;
    SPACE_HEADER    sph;
    UINT            rgbitT;
    INT             iT;
    DATA            data;

    AssertSPIPfucbOnRoot( pfucb );

    Assert( cpgSize <= cpgSmallSpaceAvailMost );
    Assert( FSPIIsSmall( pfucb->u.pfcb ) );

    Assert( pgnoFirst > PgnoFDP( pfucb ) );
    Assert( pgnoFirst - PgnoFDP( pfucb ) <= cpgSmallSpaceAvailMost );
    Assert( ( pgnoFirst + cpgSize - 1 - PgnoFDP( pfucb ) ) <= cpgSmallSpaceAvailMost );
    Assert( ( pgnoFirst + cpgSize - 1 ) <= g_rgfmp[pfucb->ifmp].PgnoLast() );

    if ( latchWrite != pfucb->pcsrRoot->Latch() )
    {
        SPIUpgradeToWriteLatch( pfucb );
    }

    NDGetExternalHeader( pfucb, pfucb->pcsrRoot, noderfSpaceHeader );
    Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );

    UtilMemCpy( &sph, pfucb->kdfCurr.data.Pv(), sizeof(sph) );

    for ( rgbitT = 1, iT = 1; iT < cpgSize; iT++ )
    {
        rgbitT = ( rgbitT << 1 ) + 1;
    }
    rgbitT <<= ( pgnoFirst - PgnoFDP( pfucb ) - 1 );
    sph.SetRgbitAvail( sph.RgbitAvail() | rgbitT );
    data.SetPv( &sph );
    data.SetCb( sizeof(sph) );
    Call( ErrNDSetExternalHeader(
                pfucb,
                pfucb->pcsrRoot,
                &data,
                ( pfucb->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                noderfSpaceHeader ) );
HandleError:

    return err;
}


ERR ErrSPIAERemoveInsertionRegion(
    __in    const FCB * const               pfcb,
    __inout FUCB * const                    pfucbAE,
    __in    const CSPExtentInfo * const     pcspoeContaining,
    __in    const PGNO                      pgnoFirstToFree,
    __inout CPG * const                     pcpgSizeToFree
    )
{
    ERR                 err = JET_errSuccess;
    DIB                 dibAE;
    CSPExtentKeyBM      cspextkeyAE;
    CSPExtentInfo       cspaei;

    PGNO                pgnoLast = pgnoFirstToFree + (*pcpgSizeToFree) - 1;

    Assert( !Pcsr( pfucbAE )->FLatched() );

    const SpacePool sppAvailContinuousPool = spp::ContinuousPool;

    cspextkeyAE.SPExtentMakeKeyBM( SPEXTKEY::fSPExtentTypeAE, pgnoFirstToFree - 1, sppAvailContinuousPool );
    Assert( cspextkeyAE.GetBm( pfucbAE ).data.Pv() == NULL );
    Assert( cspextkeyAE.GetBm( pfucbAE ).data.Cb() == 0 );
    dibAE.pos = posDown;
    dibAE.pbm = cspextkeyAE.Pbm( pfucbAE );
    dibAE.dirflag = fDIRFavourNext;

    err = ErrBTDown( pfucbAE, &dibAE, latchReadTouch );
    Assert( err != JET_errNoCurrentRecord );

    if ( JET_errRecordNotFound == err )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    if ( err < 0 )
    {
        OSTraceFMP( pfucbAE->ifmp, JET_tracetagSpaceInternal,
                    OSFormat( "ErrSPIAERemoveInsertionRegion could not go down into nonempty available extent tree. [ifmp=0x%x]\n", pfucbAE->ifmp ) );
    }
    Call( err );

    BOOL    fOnNextExtent   = fFalse;

    cspaei.Set( pfucbAE );

    ERR errT = cspaei.ErrCheckCorrupted();
    if( errT < JET_errSuccess )
    {
        CallS( errT );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"af6f9c2c-efa4-4c9e-bb1d-0cf605697457" );
        Call( errT );
    }

    if ( cspaei.SppPool() != sppAvailContinuousPool )
    {
        ExpectedSz( ( cspaei.SppPool() == spp::AvailExtLegacyGeneralPool ) ||
                    ( pfcb->PgnoFDP() == pgnoSystemRoot ), "We only have a next pool for the DB root today." );
        err = JET_errSuccess;
        goto HandleError;
    }

    if( !cspaei.FEmptyExtent() )
    {
        Assert( pgnoFirstToFree > cspaei.PgnoLast() ||
                pgnoLast < cspaei.PgnoFirst() );
    }

    if ( wrnNDFoundGreater == err )
    {
        Assert( cspaei.PgnoLast() > pgnoFirstToFree - 1 );

        fOnNextExtent = fTrue;
    }
    else
    {
        Assert( wrnNDFoundLess == err || JET_errSuccess == err );

        Assert( cspaei.PgnoLast() <= pgnoFirstToFree - 1 );

        cspaei.Unset();
        err = ErrBTNext( pfucbAE, fDIRNull );
        if ( err < 0 )
        {
            if ( JET_errNoCurrentRecord == err )
            {
                err = JET_errSuccess;
                goto HandleError;
            }
            Call( err );
            CallS( err );
        }
        else
        {
            cspaei.Set( pfucbAE );

            errT = cspaei.ErrCheckCorrupted();
            if( errT < JET_errSuccess )
            {
                CallS( errT );
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"30d82277-2e77-4742-af4d-1d1808205ba6" );
                Call( errT );
            }
            if ( cspaei.SppPool() != sppAvailContinuousPool )
            {
                ExpectedSz( ( cspaei.SppPool() == spp::AvailExtLegacyGeneralPool ) ||
                            ( pfcb->PgnoFDP() == pgnoSystemRoot ), "We only have a next pool for the DB root today." );
                err = JET_errSuccess;
                goto HandleError;
            }

            if( !cspaei.FEmptyExtent() )
            {
                Assert( pgnoFirstToFree > cspaei.PgnoLast() ||
                        pgnoLast < cspaei.PgnoFirst() );
            }
            fOnNextExtent = fTrue;
        }
    }

    Assert( cspaei.FValidExtent() );
    Assert( fOnNextExtent );
    Assert( cspaei.SppPool() == sppAvailContinuousPool );

    if ( !cspaei.FEmptyExtent() &&
            pgnoLast >= cspaei.PgnoFirst() )
    {
        AssertSz( fFalse, "AvailExt corrupted." );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"d02a83ff-28b7-4523-8dee-c862ca33cf7e" );
        Call( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
    }

    BOOL    fDeleteInsertionRegion = fFalse;
    if ( cspaei.FEmptyExtent() )
    {

        if ( pgnoLast == cspaei.PgnoMarker() &&
             cspaei.PgnoLast() <= pcspoeContaining->PgnoLast() )
        {
            fDeleteInsertionRegion = fTrue;
        }
    }
    else
    {

        Assert( cspaei.FValidExtent() );
        Assert( cspaei.CpgExtent() != 0 );
        if ( pgnoLast >= cspaei.PgnoFirst() )
        {
            AssertSz( fFalse, "AvailExt corrupted." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"ba12baa3-cd1e-4726-9e9b-94bf34d99a14" );
            Call( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }

        if ( pgnoLast == cspaei.PgnoLast() - cspaei.CpgExtent() &&
             cspaei.PgnoLast() <= pcspoeContaining->PgnoLast() )
        {
            fDeleteInsertionRegion = fTrue;
        }
        
    }

    if ( fDeleteInsertionRegion )
    {


        if ( !cspaei.FEmptyExtent() )
        {

            Assert( cspaei.PgnoLast() <= pcspoeContaining->PgnoLast() );

            *pcpgSizeToFree = *pcpgSizeToFree + cspaei.CpgExtent();

        }
        
        Call( ErrBTFlagDelete(
                    pfucbAE,
                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
        Assert( Pcsr( pfucbAE )->FLatched() );

        err = cspaei.FEmptyExtent() ? JET_errSuccess : wrnSPReservedPages;

    }
    else
    {
        err = JET_errSuccess;
    }

HandleError:

    BTUp( pfucbAE );

    return err;
}

LOCAL VOID SPIReportSpaceLeak( __in const FUCB* const pfucb, __in const ERR err, __in const PGNO pgnoFirst, __in const CPG cpg, __in_z const CHAR* const szTag )
{
    Assert( pfucb != NULL );
    Expected( err < JET_errSuccess );
    Assert( pgnoFirst != pgnoNull );
    Assert( cpg > 0 );

    const PGNO pgnoLast = pgnoFirst + cpg - 1;
    const OBJID objidFDP = pfucb->u.pfcb->ObjidFDP();
    const BOOL fDbRoot = ( pfucb->u.pfcb->PgnoFDP() == pgnoSystemRoot );

    OSTraceSuspendGC();
    const WCHAR* rgcwsz[] =
    {
        g_rgfmp[ pfucb->ifmp ].WszDatabaseName(),
        OSFormatW( L"%I32d", err ),
        OSFormatW( L"%I32u", objidFDP ),
        OSFormatW( L"%I32d", cpg ),
        OSFormatW( L"%I32u", pgnoFirst ),
        OSFormatW( L"%I32u", pgnoLast ),
        OSFormatW( L"%hs", szTag ),
    };
    UtilReportEvent(
        fDbRoot ? eventError : eventWarning,
        GENERAL_CATEGORY,
        fDbRoot ? DATABASE_LEAKED_ROOT_SPACE_ID : DATABASE_LEAKED_NON_ROOT_SPACE_ID,
        _countof( rgcwsz ),
        rgcwsz,
        0,
        NULL,
        g_rgfmp[ pfucb->ifmp ].Pinst() );
    OSTraceResumeGC();
}

LOCAL ERR ErrSPIAEFreeExt( __inout FUCB * pfucb, __in PGNO pgnoFirst, __in CPG cpgSize, __in FUCB * const pfucbParent = pfucbNil )
{
    ERR         err                 = errCodeInconsistency;
    PIB         * const ppib        = pfucb->ppib;
    FCB         * const pfcb        = pfucb->u.pfcb;
    PGNO        pgnoLast            = pgnoFirst + cpgSize - 1;
    BOOL        fCoalesced          = fFalse;
    CPG         cpgSizeAdj          = 0;

    Assert( !FSPIIsSmall( pfcb ) );
    Assert( cpgSize > 0 );

    CSPExtentInfo           cspoeContaining;

    CSPExtentInfo           cspaei;
    CSPExtentKeyBM          cspextkeyAE;
    DIB         dibAE;
 
    FUCB        *pfucbAE        = pfucbNil;
    FUCB        *pfucbOE        = pfucbNil;

    const SpacePool sppAvailGeneralPool = spp::AvailExtLegacyGeneralPool;

    #ifdef DEBUG
    BOOL    fWholeKitAndKabudle = fFalse;
    #endif

    AssertSPIPfucbOnRoot( pfucb );
    AssertSPIPfucbOnRootOrNull( pfucbParent );

    if ( ( pgnoLast > g_rgfmp[pfcb->Ifmp()].PgnoLast() ) && ( pfcb->PgnoFDP() == pgnoSystemRoot ) )
    {
        Assert( pfucbParent == pfucbNil );
        Assert( !g_rgfmp[pfcb->Ifmp()].FIsTempDB() );

        const PGNO pgnoFirstUnshelve = max( pgnoFirst, g_rgfmp[pfcb->Ifmp()].PgnoLast() + 1 );
        Call( ErrSPIUnshelvePagesInRange( pfucb, pgnoFirstUnshelve, pgnoLast ) );

        if ( pgnoFirst > g_rgfmp[pfcb->Ifmp()].PgnoLast() )
        {
            goto HandleError;
        }

        pgnoLast = min( pgnoLast, g_rgfmp[pfcb->Ifmp()].PgnoLast() );
        Assert( pgnoLast >= pgnoFirst );
        cpgSize = pgnoLast - pgnoFirst + 1;
    }

    cpgSizeAdj = cpgSize;

    Call( ErrSPIOpenOwnExt( ppib, pfcb, &pfucbOE ) );
    Assert( pfcb == pfucbOE->u.pfcb );

    Call( ErrSPIFindExtOE( pfucbOE, pgnoFirst, &cspoeContaining ) );

    if ( ( pgnoFirst > cspoeContaining.PgnoLast() ) || ( pgnoLast < cspoeContaining.PgnoFirst() ) )
    {


        err = JET_errSuccess;
        goto HandleError;
    }
    else if ( ( pgnoFirst < cspoeContaining.PgnoFirst() ) || ( pgnoLast > cspoeContaining.PgnoLast() ) )
    {

        FireWall( "CorruptedOeTree" );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbOE ), HaDbFailureTagCorruption, L"9322391a-63fb-4718-948e-89425a75ed2e" );
        Call( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
    }

    BTUp( pfucbOE );

    Call( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbAE ) );
    Assert( pfcb == pfucbAE->u.pfcb );

    if ( pgnoLast == cspoeContaining.PgnoLast()
        && cpgSizeAdj == cspoeContaining.CpgExtent() )
    {
        Call( ErrSPIReserveSPBufPages( pfucb, pfucbParent ) );

        OnDebug( fWholeKitAndKabudle = fTrue );
        goto InsertExtent;
    }

CoallesceWithNeighbors:

    Call( ErrSPIReserveSPBufPages( pfucb, pfucbParent ) );

    cspextkeyAE.SPExtentMakeKeyBM( SPEXTKEY::fSPExtentTypeAE, pgnoFirst - 1, sppAvailGeneralPool );
    Assert( cspextkeyAE.GetBm( pfucbAE ).data.Pv() == NULL );
    Assert( cspextkeyAE.GetBm( pfucbAE ).data.Cb() == 0 );
    dibAE.pos = posDown;
    dibAE.pbm = cspextkeyAE.Pbm( pfucbAE );
    dibAE.dirflag = fDIRFavourNext;

    err = ErrBTDown( pfucbAE, &dibAE, latchReadTouch );
    if ( JET_errRecordNotFound != err )
    {
        BOOL    fOnNextExtent   = fFalse;

        Assert( err != JET_errNoCurrentRecord );

        if ( err < 0 )
        {
            OSTraceFMP( pfucbAE->ifmp, JET_tracetagSpaceInternal,
                        OSFormat( "ErrSPFreeExt could not go down into nonempty available extent tree. [ifmp=0x%x]\n", pfucbAE->ifmp ) );
        }
        Call( err );

        cspaei.Set( pfucbAE );

        ERR errT = cspaei.ErrCheckCorrupted();
        if( errT < JET_errSuccess )
        {
            AssertSz( fFalse, "Corruption bad." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"2bc482de-56bd-44ce-ae3d-d51b7a0343df" );
            Call( errT );
        }
        Assert( cspaei.CpgExtent() >= 0 );

#ifdef DEPRECATED_ZERO_SIZED_AVAIL_EXTENT_CLEANUP
        if ( 0 == cspaei.CpgExtent() )
        {
            Call( ErrBTFlagDelete(
                        pfucbAE,
                        fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
            BTUp( pfucbAE );
            goto CoallesceWithNeighbors;
        }
#endif

        if ( cspaei.SppPool() == sppAvailGeneralPool )
        {
            Assert( pgnoFirst > cspaei.PgnoLast() ||
                    pgnoLast < cspaei.PgnoFirst() );

            if ( wrnNDFoundGreater == err )
            {
                Assert( cspaei.PgnoLast() > pgnoFirst - 1 );

                fOnNextExtent = fTrue;
            }
            else if ( wrnNDFoundLess == err )
            {
                Assert( cspaei.PgnoLast() < pgnoFirst - 1 );
            }
            else
            {
                Assert( cspaei.PgnoLast() == pgnoFirst - 1 );
                CallS( err );
            }

            if ( JET_errSuccess == err
                && pgnoFirst > cspoeContaining.PgnoFirst() )
            {
                Assert( cspaei.PgnoFirst() >= cspoeContaining.PgnoFirst() );
                #ifdef DEBUG
                CPG cpgAECurrencyCheck;
                CallS( ErrSPIExtentCpg( pfucbAE, &cpgAECurrencyCheck ) );
                Assert( cspaei.CpgExtent() == cpgAECurrencyCheck );
                #endif
                Assert( cspaei.PgnoLast() == pgnoFirst - 1 );

                cpgSizeAdj += cspaei.CpgExtent();
                pgnoFirst -= cspaei.CpgExtent();
                Assert( pgnoLast == pgnoFirst + cpgSizeAdj - 1 );
                Call( ErrBTFlagDelete(
                            pfucbAE,
                            fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );

                Assert( !fOnNextExtent );
                if ( pgnoLast == cspoeContaining.PgnoLast()
                    && cpgSizeAdj == cspoeContaining.CpgExtent() )
                {
                    OnDebug( fWholeKitAndKabudle = fTrue );
                    goto InsertExtent;
                }

                Pcsr( pfucbAE )->Downgrade( latchReadTouch );

                Assert( cspoeContaining.PgnoFirst() <= pgnoFirst );
                Assert( cspoeContaining.PgnoLast() >= pgnoLast );
            }

            if ( !fOnNextExtent )
            {
                err = ErrBTNext( pfucbAE, fDIRNull );
                if ( err < 0 )
                {
                    if ( JET_errNoCurrentRecord == err )
                    {
                        err = JET_errSuccess;
                    }
                    else
                    {
                        Call( err );
                    }
                }
                else
                {
                    fOnNextExtent = fTrue;
                }
            }

            if ( fOnNextExtent )
            {
                cspaei.Set( pfucbAE );

                if ( cspaei.SppPool() == sppAvailGeneralPool )
                {
                    Assert( cspaei.CpgExtent() != 0 );
                    if ( pgnoLast >= cspaei.PgnoFirst() )
                    {
                        AssertSz( fFalse, "AvailExt corrupted." );
                        OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"5bc71f68-0ef3-4fb6-9c36-20bdbb1f5a70" );
                        Call( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
                    }

                    if ( ( pgnoLast == ( cspaei.PgnoFirst() - 1 ) ) &&
                         ( cspaei.PgnoLast() <= cspoeContaining.PgnoLast() ) )
                    {
                        CSPExtentNodeKDF spAdjustedSize( SPEXTKEY::fSPExtentTypeAE, cspaei.PgnoLast(), cspaei.CpgExtent(), sppAvailGeneralPool );
                        spAdjustedSize.ErrUnconsumeSpace( cpgSizeAdj );
                        cpgSizeAdj += cspaei.CpgExtent();

                        Call( ErrBTReplace(
                                    pfucbAE,
                                    spAdjustedSize.GetData( ),
                                    fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
                        Assert( Pcsr( pfucbAE )->FLatched() );

                        pgnoLast = cspaei.PgnoLast();
                        fCoalesced = fTrue;

                        if ( cpgSizeAdj >= cpageSEDefault
                            && pgnoNull != pfcb->PgnoNextAvailSE()
                            && pgnoFirst < pfcb->PgnoNextAvailSE() )
                        {
                            pfcb->SetPgnoNextAvailSE( pgnoFirst );
                        }
                    }
                }
            }
        }
    }

    if ( !fCoalesced )
    {
InsertExtent:

        BTUp( pfucbAE );

        const CPG cpgSave = cpgSizeAdj;
        Call( ErrSPIAERemoveInsertionRegion( pfcb, pfucbAE, &cspoeContaining, pgnoFirst, &cpgSizeAdj ) );

        Assert( !fWholeKitAndKabudle || wrnSPReservedPages !=  err );

        if ( wrnSPReservedPages == err ||
                cpgSave != cpgSizeAdj )
        {
            Assert( wrnSPReservedPages == err && cpgSave != cpgSizeAdj );

            pgnoLast = pgnoFirst + cpgSizeAdj - 1;

            Assert( cspoeContaining.FContains( pgnoLast ) );
            Assert( cspoeContaining.FContains( pgnoFirst ) );
            
            goto CoallesceWithNeighbors;
        }

        BTUp( pfucbAE );

        Call( ErrSPIAddFreedExtent(
                    pfucb,
                    pfucbAE,
                    pgnoLast,
                    cpgSizeAdj ) );
    }
    Assert( Pcsr( pfucbAE )->FLatched() );

    if ( ( pgnoSystemRoot == pfucbAE->u.pfcb->PgnoFDP() )
        && g_rgfmp[pfucbAE->u.pfcb->Ifmp()].FCacheAvail() )
    {
        g_rgfmp[pfucbAE->u.pfcb->Ifmp()].AdjustCpgAvail( cpgSize );
    }

    Assert( pgnoLast != cspoeContaining.PgnoLast() || cpgSizeAdj <= cspoeContaining.CpgExtent() );
    if ( pgnoLast == cspoeContaining.PgnoLast() && cpgSizeAdj == cspoeContaining.CpgExtent() )
    {
        Assert( cpgSizeAdj > 0 );

#ifdef DEBUG
        Assert( Pcsr( pfucbAE )->FLatched() );
        cspaei.Set( pfucbAE );
        Assert( cspaei.PgnoLast() == pgnoLast );
        Assert( cspaei.CpgExtent() == cpgSizeAdj );
#endif

#ifdef DEBUG
        if ( ErrFaultInjection( 62250 ) >= JET_errSuccess )
#endif
        {
            Call( ErrSPIFreeSEToParent( pfucb, pfucbOE, pfucbAE, pfucbParent, cspoeContaining.PgnoLast(), cspoeContaining.CpgExtent() ) );
        }
    }

HandleError:

    Assert( FSPExpectedError( err ) );

    AssertSPIPfucbOnRoot( pfucb );
    AssertSPIPfucbOnRootOrNull( pfucbParent );

    if ( pfucbAE != pfucbNil )
    {
        BTClose( pfucbAE );
    }
    if ( pfucbOE != pfucbNil )
    {
        BTClose( pfucbOE );
    }

    Assert( err != JET_errKeyDuplicate );

    return err;
}

ERR ErrSPCaptureSnapshot( FUCB* const pfucb, const PGNO pgnoFirst, const CPG cpgSize )
{
    ERR err = JET_errSuccess;
    BOOL fEfvEnabled = ( g_rgfmp[pfucb->ifmp].FLogOn() && PinstFromPfucb( pfucb )->m_plog->ErrLGFormatFeatureEnabled( JET_efvRevertSnapshot ) >= JET_errSuccess );
    const CPG cpgPrereadMax = LFunctionalMax( 1, (CPG)( UlParam( JET_paramMaxCoalesceReadSize ) / g_cbPage ) );
    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortFreeExtSnapshot );

    for ( CPG cpgT = 0; cpgT < cpgSize; cpgT += cpgPrereadMax )
    {
        CPG cpgRead = LFunctionalMin( cpgSize - cpgT, cpgPrereadMax );

        if ( g_rgfmp[ pfucb->ifmp ].FRBSOn() ) 
        {
            BFPrereadPageRange( pfucb->ifmp, pgnoFirst + cpgT, cpgRead, bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->ifmp ), *tcScope );

            BFLatch bfl;

            for( int i = 0; i < cpgRead; ++i )
            {
                err = ErrBFWriteLatchPage( &bfl, pfucb->ifmp, pgnoFirst + cpgT + i, bflfUninitPageOk, pfucb->ppib->BfpriPriority( pfucb->ifmp ), *tcScope );

                if ( err == JET_errPageNotInitialized )
                {
                    BFMarkAsSuperCold( pfucb->ifmp, pgnoFirst + cpgT + i );
                    continue;
                }
                CallR( err );
                BFMarkAsSuperCold( &bfl );
                BFWriteUnlatch( &bfl );
            }
        }

        if ( fEfvEnabled )
        {
            CallR( ErrLGExtentFreed( PinstFromPfucb( pfucb )->m_plog, pfucb->ifmp, pgnoFirst + cpgT, cpgRead ) );
        }
    }

    return JET_errSuccess;
}

ERR ErrSPFreeExt( FUCB* const pfucb, const PGNO pgnoFirst, const CPG cpgSize, const CHAR* const szTag )
{
    ERR         err;
    FCB         * const pfcb    = pfucb->u.pfcb;
    BOOL        fRootLatched    = fFalse;

    PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortSpace );

    Assert( cpgSize > 0 );

#ifdef SPACECHECK
    CallS( ErrSPIValidFDP( ppib, pfucb->ifmp, PgnoFDP( pfucb ) ) );
#endif

    Call( ErrFaultInjection( 41610 ) );

    if ( FFUCBSpace( pfucb ) )
    {
        Call( ErrSPISPFreeExt( pfucb, pgnoFirst, cpgSize ) );
    }
    else
    {
        if ( pfucb->pcsrRoot == pcsrNil )
        {
            Assert( !Pcsr( pfucb )->FLatched() );
            Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
            pfucb->pcsrRoot = Pcsr( pfucb );
            fRootLatched = fTrue;
        }
        else
        {
            Assert( pfucb->pcsrRoot->Pgno() == PgnoRoot( pfucb ) );
            Assert( pfucb->pcsrRoot->Latch() == latchRIW
                || pfucb->pcsrRoot->Latch() == latchWrite );
        }

        if ( !pfcb->FSpaceInitialized() )
        {
            SPIInitFCB( pfucb, fTrue );
        }

#ifdef SPACECHECK
        CallS( ErrSPIWasAlloc( pfucb, pgnoFirst, cpgSize ) );
#endif

        Assert( pfucb->pcsrRoot != pcsrNil );


        if ( FSPIIsSmall( pfcb ) )
        {
            Call( ErrSPISmallFreeExt( pfucb, pgnoFirst, cpgSize ) );
        }
        else
        {
            Call( ErrSPIAEFreeExt( pfucb, pgnoFirst, cpgSize ) );
        }
    }

    CallS( err );
    
HandleError:

    if( err < JET_errSuccess )
    {
        OSTraceFMP( pfucb->ifmp, JET_tracetagSpaceInternal,
                        OSFormat( "Failed to free space %lu at %lu to FDP %d.%lu\n", cpgSize, pgnoFirst, pfucb->ifmp, PgnoFDP( pfucb ) ) );

        SPIReportSpaceLeak( pfucb, err, pgnoFirst, cpgSize, szTag );
    }
    else
    {
        const PGNO pgnoFDP = PgnoFDP( pfucb );
        if ( cpgSize > 1 )
        {
            ETSpaceFreeExt( pfucb->ifmp, pgnoFDP, pgnoFirst, cpgSize, pfucb->u.pfcb->ObjidFDP(), TceFromFUCB( pfucb ) );
            OSTraceFMP( pfucb->ifmp, JET_tracetagSpaceManagement,
                            OSFormat( "Free space %lu at %lu to FDP %d.%lu\n", cpgSize, pgnoFirst, pfucb->ifmp, PgnoFDP( pfucb ) ) );
        }
        else
        {
            ETSpaceFreePage( pfucb->ifmp, pgnoFDP, pgnoFirst, pfucb->u.pfcb->ObjidFDP(), TceFromFUCB( pfucb ) );
            OSTraceFMP( pfucb->ifmp, JET_tracetagSpaceManagement,
                            OSFormat( "Free page at %lu to FDP %d.%lu\n", pgnoFirst, pfucb->ifmp, PgnoFDP( pfucb ) ) );
        }
    }

    if ( fRootLatched )
    {
        Assert( Pcsr( pfucb ) == pfucb->pcsrRoot );
        Assert( pfucb->pcsrRoot->FLatched() );
        pfucb->pcsrRoot->ReleasePage();
        pfucb->pcsrRoot = pcsrNil;
    }

    Assert( err != JET_errKeyDuplicate );

    return err;
}

LOCAL ERR ErrErrBitmapToJetErr( const IBitmapAPI::ERR err )
{
    switch ( err )
    {
        case IBitmapAPI::ERR::errSuccess:
            return JET_errSuccess;

        case IBitmapAPI::ERR::errInvalidParameter:
            return ErrERRCheck( JET_errInvalidParameter );

        case IBitmapAPI::ERR::errOutOfMemory:
            return ErrERRCheck( JET_errOutOfMemory );
    }

    AssertSz( fFalse, "UnknownIBitmapAPI::ERR: %s", (int)err );
    return ErrERRCheck( JET_errInternalError );
}


ERR ErrSPTryCoalesceAndFreeAvailExt( FUCB* const pfucb, const PGNO pgnoInExtent, BOOL* const pfCoalesced )
{
    Assert( !FFUCBSpace( pfucb ) );

    ERR err = JET_errSuccess;
    PIB* const ppib = pfucb->ppib;
    FCB* const pfcb = pfucb->u.pfcb;
    FUCB* pfucbOE = pfucbNil;
    FUCB* pfucbAE = pfucbNil;
    CSPExtentInfo speiContaining;
    CSparseBitmap spbmAvail;

    Assert( !FSPIIsSmall( pfcb ) );
    Assert( pfcb->PgnoFDP() != pgnoSystemRoot );

    *pfCoalesced = fFalse;

    Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
    pfucb->pcsrRoot = Pcsr( pfucb );

    Call( ErrSPIOpenOwnExt( ppib, pfcb, &pfucbOE ) );
    Call( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbAE ) );

    err = ErrSPIFindExtOE( pfucbOE, pgnoInExtent, &speiContaining );
    if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
    {
        err = JET_errSuccess;
    }
    Call( err );
    if ( !speiContaining.FIsSet() || !speiContaining.FContains( pgnoInExtent ) )
    {
        goto HandleError;
    }
    BTUp( pfucbOE );

    const PGNO pgnoOeFirst = speiContaining.PgnoFirst();
    const PGNO pgnoOeLast = speiContaining.PgnoLast();
    const CPG cpgOwnExtent = speiContaining.CpgExtent();


    Call( ErrErrBitmapToJetErr( spbmAvail.ErrInitBitmap( cpgOwnExtent ) ) );

    for ( SpacePool sppAvailPool = spp::MinPool;
            sppAvailPool < spp::MaxPool;
            sppAvailPool++ )
    {
        if ( sppAvailPool == spp::ShelvedPool )
        {
            continue;
        }

        PGNO pgno = pgnoOeFirst;
        while ( pgno <= pgnoOeLast )
        {
            CSPExtentInfo speiAE;
            err = ErrSPIFindExtAE( pfucbAE, pgno, sppAvailPool, &speiAE );
            if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
            {
                err = JET_errSuccess;
            }
            Call( err );
            err = JET_errSuccess;

            Assert( !speiAE.FIsSet() || ( speiAE.PgnoLast() >= pgno ) );

            if ( speiAE.FIsSet() &&
                 ( speiAE.CpgExtent() > 0 ) &&
                 ( ( speiAE.PgnoFirst() < pgnoOeFirst ) ||
                   ( speiContaining.FContains( speiAE.PgnoFirst() ) && !speiContaining.FContains( speiAE.PgnoLast() ) ) ) )
            {
                AssertTrack( fFalse, "TryCoalesceAvailOverlapExt" );
                Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
            }

            if ( speiAE.FIsSet() &&
                 ( speiAE.SppPool() != spp::AvailExtLegacyGeneralPool ) &&
                 ( speiAE.SppPool() != spp::ContinuousPool ) )
            {
                FireWall( OSFormat( "TryCoalesceAvailUnkPool:0x%I32x", speiAE.SppPool() ) );
                goto HandleError;
            }

            if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() > pgnoOeLast ) )
            {
                break;
            }

            if ( speiAE.CpgExtent() > 0 )
            {
                for ( PGNO pgnoAvail = speiAE.PgnoFirst(); pgnoAvail <= speiAE.PgnoLast(); pgnoAvail++ )
                {
#ifdef DEBUG
                    BOOL fSet = fFalse;
                    if ( spbmAvail.ErrGet( pgnoAvail - pgnoOeFirst, &fSet ) == IBitmapAPI::ERR::errSuccess )
                    {
                        Assert( !fSet );
                    }
#endif

                    Call( ErrErrBitmapToJetErr( spbmAvail.ErrSet( pgnoAvail - pgnoOeFirst, fTrue ) ) );
                }
            }

            BTUp( pfucbAE );

            pgno = speiAE.PgnoLast() + 1;
        }
        BTUp( pfucbAE );
    }
    BTUp( pfucbAE );

    Expected( g_rgfmp[pfucb->ifmp].FShrinkIsRunning() );
    for ( PGNO ipgno = 0; ipgno < (PGNO)cpgOwnExtent; ipgno++ )
    {
        const size_t i = ( ( ipgno % 2 ) == 0 ) ? ( ipgno / 2 ) : ( (PGNO)cpgOwnExtent - 1 - ( ipgno / 2 ) );

        BOOL fSet = fFalse;
        Call( ErrErrBitmapToJetErr( spbmAvail.ErrGet( i, &fSet ) ) );
        if ( !fSet )
        {
            goto HandleError;
        }
    }

    for ( SpacePool sppAvailPool = spp::MinPool;
            sppAvailPool < spp::MaxPool;
            sppAvailPool++ )
    {
        if ( sppAvailPool == spp::ShelvedPool )
        {
            continue;
        }

        PGNO pgno = pgnoOeFirst;
        while ( pgno <= pgnoOeLast )
        {
            Call( ErrSPIReserveSPBufPages( pfucb, pfucbNil ) );

            CSPExtentInfo speiAE;
            err = ErrSPIFindExtAE( pfucbAE, pgno, sppAvailPool, &speiAE );
            if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
            {
                err = JET_errSuccess;
            }
            Call( err );
            err = JET_errSuccess;

            Assert( !speiAE.FIsSet() || ( speiAE.PgnoLast() >= pgno ) );
            Assert( !( speiAE.FIsSet() &&
                       ( speiAE.CpgExtent() > 0 ) &&
                       ( ( speiAE.PgnoFirst() < pgnoOeFirst ) ||
                         ( speiContaining.FContains( speiAE.PgnoFirst() ) && !speiContaining.FContains( speiAE.PgnoLast() ) ) ) ) );
            Assert( !( speiAE.FIsSet() &&
                       ( speiAE.SppPool() != spp::AvailExtLegacyGeneralPool ) &&
                       ( speiAE.SppPool() != spp::ContinuousPool ) ) );

            if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() > pgnoOeLast ) )
            {
                break;
            }

            pgno = speiAE.PgnoFirst();

            Call( ErrBTFlagDelete(
                        pfucbAE,
                        fDIRNoVersion | ( pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
            BTUp( pfucbAE );

            while ( pgno <= speiAE.PgnoLast() )
            {
                Call( ErrErrBitmapToJetErr( spbmAvail.ErrSet( pgno - pgnoOeFirst, fFalse ) ) );
                pgno++;
            }
        }
        BTUp( pfucbAE );
    }
    BTUp( pfucbAE );

    for ( PGNO pgno = pgnoOeFirst; pgno <= pgnoOeLast; pgno++ )
    {
        BOOL fSet = fFalse;
        Call( ErrErrBitmapToJetErr( spbmAvail.ErrGet( pgno - pgnoOeFirst, &fSet ) ) );
        if ( fSet )
        {
            AssertTrack( fFalse, "TryCoalesceAvailAeChanged" );
            Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }
    }

    Call( ErrSPIReserveSPBufPages( pfucb, pfucbNil ) );

    Call( ErrSPIFreeSEToParent(
            pfucb,
            pfucbOE,
            pfucbNil,
            pfucbNil,
            pgnoOeLast,
            cpgOwnExtent ) );

    *pfCoalesced = fTrue;

HandleError:
    if ( pfucbAE != pfucbNil )
    {
        BTUp( pfucbAE );
        BTClose( pfucbAE );
        pfucbAE = pfucbNil;
    }

    if ( pfucbOE != pfucbNil )
    {
        BTUp( pfucbOE );
        BTClose( pfucbOE );
        pfucbOE = pfucbNil;
    }

    BTUp( pfucb );
    pfucb->pcsrRoot = pcsrNil;

    return err;
}


ERR ErrSPShelvePage( PIB* const ppib, const IFMP  ifmp, const PGNO pgno )
{
    Assert( pgno != pgnoNull );

    ERR err = JET_errSuccess;
    FMP* const pfmp = g_rgfmp + ifmp;
    Assert( !pfmp->FIsTempDB() );
    Assert( pfmp->FBeyondPgnoShrinkTarget( pgno ) );
    Assert( pgno <= pfmp->PgnoLast() );

    FUCB* pfucbRoot = pfucbNil;
    FUCB* pfucbAE = pfucbNil;

    Call( ErrBTIOpenAndGotoRoot( ppib, pgnoSystemRoot, ifmp, &pfucbRoot ) );
    Call( ErrSPIOpenAvailExt( ppib, pfucbRoot->u.pfcb, &pfucbAE ) );

    Call( ErrSPIReserveSPBufPages( pfucbRoot, pfucbNil ) );

    Call( ErrSPIAddToAvailExt( pfucbAE, pgno, 1, spp::ShelvedPool ) );
    Assert( Pcsr( pfucbAE )->FLatched() );

HandleError:
    if ( pfucbAE != pfucbNil )
    {
        BTUp( pfucbAE );
        BTClose( pfucbAE );
        pfucbAE = pfucbNil;
    }

    if ( pfucbRoot != pfucbNil )
    {
        pfucbRoot->pcsrRoot->ReleasePage();
        pfucbRoot->pcsrRoot = pcsrNil;
        BTClose( pfucbRoot );
        pfucbRoot = pfucbNil;
    }

    return err;
}


ERR ErrSPUnshelveShelvedPagesBelowEof( PIB* const ppib, const IFMP ifmp )
{
    ERR err = JET_errSuccess;
    FMP* const pfmp = g_rgfmp + ifmp;
    FUCB* pfucbRoot = pfucbNil;
    BOOL fInTransaction = fFalse;
    Assert( !pfmp->FIsTempDB() );
    Expected( pfmp->FShrinkIsRunning() );

    Call( ErrDIRBeginTransaction( ppib, 46018, NO_GRBIT ) );
    fInTransaction = fTrue;

    Call( ErrBTIOpenAndGotoRoot( ppib, pgnoSystemRoot, ifmp, &pfucbRoot ) );

    Call( ErrSPIUnshelvePagesInRange( pfucbRoot, 1, pfmp->PgnoLast() ) );

HandleError:
    if ( pfucbRoot != pfucbNil )
    {
        pfucbRoot->pcsrRoot->ReleasePage();
        pfucbRoot->pcsrRoot = pcsrNil;
        BTClose( pfucbRoot );
        pfucbRoot = pfucbNil;
    }

    if ( fInTransaction )
    {
        if ( err >= JET_errSuccess )
        {
            err = ErrDIRCommitTransaction( ppib, NO_GRBIT );
            fInTransaction = ( err < JET_errSuccess );
        }

        if ( fInTransaction )
        {
            CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
            fInTransaction = fFalse;
        }
    }

    return err;
}


ERR ErrSPIUnshelvePagesInRange( FUCB* const pfucbRoot, const PGNO pgnoFirst, const PGNO pgnoLast )
{
    ERR err = JET_errSuccess;
    FUCB* pfucbAE = pfucbNil;

    AssertSPIPfucbOnRoot( pfucbRoot );
    Assert( ( pgnoFirst != pgnoNull ) && ( pgnoLast != pgnoNull) );
    Assert( pgnoFirst <= pgnoLast );

    FMP* const pfmp = g_rgfmp + pfucbRoot->ifmp;
    Assert( !pfmp->FIsTempDB() );

    const PGNO pgnoLastDbRootInitial = pfmp->PgnoLast();
    PGNO pgnoLastDbRootPrev = pgnoLastDbRootInitial;
    PGNO pgnoLastDbRoot = pgnoLastDbRootInitial;

    Expected( ( pgnoLast <= pgnoLastDbRootInitial ) || ( pgnoFirst > pgnoLastDbRootInitial ) );

    PGNO pgno = pgnoFirst;

    Call( ErrSPIOpenAvailExt( pfucbRoot->ppib, pfucbRoot->u.pfcb, &pfucbAE ) );
    while ( pgno <= pgnoLast )
    {
        pgnoLastDbRootPrev = pgnoLastDbRoot;
        Call( ErrSPIReserveSPBufPages( pfucbRoot, pfucbNil ) );
        pgnoLastDbRoot = pfmp->PgnoLast();
        Assert( pgnoLastDbRoot >= pgnoLastDbRootPrev );

        CSPExtentInfo speiAE;
        Call( ErrSPISeekRootAE( pfucbAE, pgno, spp::ShelvedPool, &speiAE ) );
        Assert( !speiAE.FIsSet() || ( speiAE.CpgExtent() == 1 ) );

        AssertTrack( !speiAE.FIsSet() ||
                     ( pgnoLastDbRoot == pgnoLastDbRootPrev ) ||
                     ( speiAE.PgnoLast() <= pgnoLastDbRootPrev ) ||
                     ( speiAE.PgnoFirst() > pgnoLastDbRoot ),
                     "UnshelvePagesNotUnshelvedByGrowth" );

        AssertTrack( ( pgnoFirst <= pgnoLastDbRootInitial ) ||
                     ( speiAE.FIsSet() && ( pgno == speiAE.PgnoFirst() ) ||
                     ( pgno <= pgnoLastDbRoot ) ),
                     "UnshelvePagesUnexpectedMissingShelves" );

        if ( !speiAE.FIsSet() || ( speiAE.PgnoFirst() < pgnoFirst ) || ( speiAE.PgnoLast() > pgnoLast ) )
        {
            break;
        }

        Call( ErrBTFlagDelete( pfucbAE, fDIRNoVersion | ( pfucbRoot->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
        BTUp( pfucbAE );

        pgno = speiAE.PgnoLast() + 1;
    }

HandleError:
    if ( pfucbAE != pfucbNil )
    {
        BTUp( pfucbAE );
        BTClose( pfucbAE );
        pfucbAE = pfucbNil;
    }

    return err;
}


class CPgnoFlagged
{
    public:
        CPgnoFlagged()
        {
            m_pgnoflagged = pgnoNull;
        }

        CPgnoFlagged( const PGNO pgno )
        {
            Assert( ( pgno & 0x80000000 ) == 0 );
            m_pgnoflagged = pgno;
        }

        CPgnoFlagged( const CPgnoFlagged& pgnoflagged )
        {
            m_pgnoflagged = pgnoflagged.m_pgnoflagged;
        }

        PGNO Pgno() const
        {
            return ( m_pgnoflagged & 0x7FFFFFFF );
        }

        BOOL FFlagged() const
        {
            return ( ( m_pgnoflagged & 0x80000000 ) != 0 );
        }

        void SetFlagged()
        {
            Expected( !FFlagged() );
            m_pgnoflagged = m_pgnoflagged | 0x80000000;
        }

    private:
        PGNO m_pgnoflagged;
};

static_assert( sizeof( CPgnoFlagged ) == sizeof( PGNO ), "SizeOfCPgnoFlaggedMustBeEqualToSizeOfPgno." );

LOCAL ERR ErrErrArrayPgnoToJetErr( const CArray<CPgnoFlagged>::ERR err )
{
    switch ( err )
    {
        case CArray<CPgnoFlagged>::ERR::errSuccess:
            return JET_errSuccess;

        case CArray<CPgnoFlagged>::ERR::errOutOfMemory:
            return ErrERRCheck( JET_errOutOfMemory );
    }

    AssertSz( fFalse, "UnknownCArray<CPgnoFlagged>::ERR: %s", (int)err );
    return ErrERRCheck( JET_errInternalError );
}

INT __cdecl PgnoShvdListCmpFunction( const CPgnoFlagged* ppgnoflagged1, const CPgnoFlagged* ppgnoflagged2 )
{
    if ( ppgnoflagged1->Pgno() > ppgnoflagged2->Pgno() )
    {
        return 1;
    }

    if ( ppgnoflagged1->Pgno() < ppgnoflagged2->Pgno() )
    {
        return -1;
    }

    return 0;
}

LOCAL ERR ErrSPILRProcessObjectSpaceOwnershipSetPgnos(
    _Inout_ CSparseBitmap* const pspbmOwned,
    _Inout_ CArray<CPgnoFlagged>* const parrShelved,
    _In_ const PGNO pgno,
    _In_ const PGNO pgnoLast,
    _In_ const BOOL fDbRoot,
    _In_ const BOOL fAvailSpace,
    _In_ const BOOL fMayAlreadyBeSet )
{
    ERR err = JET_errSuccess;

    Assert( !( fDbRoot && fMayAlreadyBeSet ) );
    Assert( !( !fDbRoot && ( !fAvailSpace != !fMayAlreadyBeSet ) ) );

    if ( pgno < 1 )
    {
        AssertTrack( fFalse, "LeakReclaimPgnoTooLow" );
        Error( ErrERRCheck( fAvailSpace ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
    }

    if ( pgno <= pgnoLast )
    {
        const size_t iBit = pgno - 1;
        BOOL fSet = fFalse;

        if ( !fMayAlreadyBeSet )
        {
            Call( ErrErrBitmapToJetErr( pspbmOwned->ErrGet( iBit, &fSet ) ) );
            if ( fSet )
            {
                AssertTrack( fFalse, "LeakReclaimPgnoDoubleBelowEof" );
                Error( ErrERRCheck( fAvailSpace ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
            }
        }

        if ( !fSet )
        {
            Call( ErrErrBitmapToJetErr( pspbmOwned->ErrSet( iBit, fTrue ) ) );
        }
    }
    else
    {
        const size_t iEntry = parrShelved->SearchBinary( (CPgnoFlagged)pgno, PgnoShvdListCmpFunction );
        if ( iEntry == CArray<CPgnoFlagged>::iEntryNotFound )
        {
            if ( fDbRoot && !fAvailSpace )
            {
                if ( ( parrShelved->Size() > 0 ) &&
                     ( ( parrShelved->Entry( parrShelved->Size() - 1 ).Pgno() > pgno ) ) )
                {
                    AssertTrack( fFalse, "LeakReclaimShelvedPagesOutOfOrder" );
                    Error( ErrERRCheck( fAvailSpace ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
                }
                
                if ( ( parrShelved->Size() + 1 ) > parrShelved->Capacity() )
                {
                    Call( ErrErrArrayPgnoToJetErr( parrShelved->ErrSetCapacity( 2 * ( parrShelved->Size() + 1 ) ) ) );
                }
                Call( ErrErrArrayPgnoToJetErr( parrShelved->ErrSetEntry( parrShelved->Size(), (CPgnoFlagged)pgno ) ) );
            }
            else
            {
                AssertTrack( fFalse, "LeakReclaimUntrackedPgnoBeyondEof" );
                Error( ErrERRCheck( fAvailSpace ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
            }
        }
        else
        {
            if ( ( fDbRoot && fAvailSpace ) || ( !fDbRoot && ( !fAvailSpace || fMayAlreadyBeSet ) ) )
            {
                CPgnoFlagged pgnoflaggedFound = parrShelved->Entry( iEntry );
                if ( !pgnoflaggedFound.FFlagged() )
                {
                    pgnoflaggedFound.SetFlagged();
                    Call( ErrErrArrayPgnoToJetErr( parrShelved->ErrSetEntry( iEntry, pgnoflaggedFound ) ) );
                }
                else if ( !fMayAlreadyBeSet )
                {
                    AssertTrack( fFalse, "LeakReclaimPgnoDoubleProcBeyondEof" );
                    Error( ErrERRCheck( fAvailSpace ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
                }
            }
            else
            {
                AssertTrack( fFalse, "LeakReclaimPgnoDoubleShelvedBeyondEof" );
                Error( ErrERRCheck( fAvailSpace ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
            }
        }
    }

HandleError:
    return err;
}

LOCAL ERR ErrSPILRProcessObjectSpaceOwnership(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const OBJID objid,
    _Inout_ CSparseBitmap* const pspbmOwned,
    _Inout_ CArray<CPgnoFlagged>* const parrShelved )
{
    ERR err = JET_errSuccess;
    PGNO pgnoFDP = pgnoNull;
    PGNO pgnoFDPParentUnused = pgnoNull;
    FCB* pfcb = pfcbNil;
    FUCB* pfucb = pfucbNil;
    FUCB* pfucbParentUnused = pfucbNil;
    BOOL fSmallSpace = fFalse;
    FUCB* pfucbSpace = pfucbNil;
    const BOOL fDbRoot = ( objid == objidSystemRoot );
    const OBJID objidParent = fDbRoot ? objidNil : objidSystemRoot;
    const SYSOBJ sysobj = fDbRoot ? sysobjNil : sysobjTable ;
    const PGNO pgnoLast = g_rgfmp[ifmp].PgnoLast();

    Call( ErrCATGetCursorsFromObjid(
        ppib,
        ifmp,
        objid,
        objidParent,
        sysobj,
        &pgnoFDP,
        &pgnoFDPParentUnused,
        &pfucb,
        &pfucbParentUnused ) );

    pfcb = pfucb->u.pfcb;
    fSmallSpace = FSPIIsSmall( pfcb );

    if ( fDbRoot )
    {

        Assert( !fSmallSpace );

        Call( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbSpace ) );
    }
    else
    {

        if ( !fSmallSpace )
        {
            Call( ErrSPIOpenOwnExt( ppib, pfcb, &pfucbSpace ) );
        }
        else
        {
            Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
            pfucb->pcsrRoot = Pcsr( pfucb );

            const SPACE_HEADER* const psph = PsphSPIRootPage( pfucb );
            Assert( psph->FSingleExtent() );

            const PGNO pgnoFirstSmallExt = PgnoFDP( pfucb );
            const PGNO pgnoLastSmallExt = pgnoFirstSmallExt + psph->CpgPrimary() - 1;
            for ( PGNO pgno = pgnoFirstSmallExt; pgno <= pgnoLastSmallExt; pgno++ )
            {
                Call( ErrSPILRProcessObjectSpaceOwnershipSetPgnos(
                            pspbmOwned,
                            parrShelved,
                            pgno,
                            pgnoLast,
                            fFalse,
                            fFalse,
                            fFalse ) );
            }

            BTUp( pfucb );
            pfucb->pcsrRoot = pcsrNil;
        }
    }

    Assert( !!fSmallSpace == ( pfucbSpace == pfucbNil ) );

    if ( fSmallSpace )
    {
        goto HandleError;
    }

    Assert( !!pfucbSpace->fOwnExt ^ !!pfucbSpace->fAvailExt );
    FUCBSetSequential( pfucbSpace );
    FUCBSetPrereadForward( pfucbSpace, cpgPrereadSequential );

    DIB dib;
    dib.pos = posFirst;
    dib.dirflag = fDIRNull;
    err = ErrBTDown( pfucbSpace, &dib, latchReadTouch );
    if ( err == JET_errRecordNotFound )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    Call( err );

    while ( err >= JET_errSuccess )
    {
        const CSPExtentInfo spext( pfucbSpace );
        Call( spext.ErrCheckCorrupted() );
        Assert( spext.FIsSet() );

        if ( spext.CpgExtent() > 0 )
        {
            const SpacePool spp = spext.SppPool();
            if ( !( ( pfucbSpace->fOwnExt && ( spp == spp::AvailExtLegacyGeneralPool ) ) ||
                    ( pfucbSpace->fAvailExt && ( spp == spp::AvailExtLegacyGeneralPool ) ) ||
                    ( pfucbSpace->fAvailExt && ( spp == spp::ContinuousPool ) ) ||
                    ( pfucbSpace->fAvailExt && ( spp == spp::ShelvedPool ) ) ||
                    ( !fDbRoot && ( spp == spp::ShelvedPool ) ) ) )
            {
                AssertTrack( fFalse, OSFormat( "LeakReclaimUnknownPool:%d:%d:%d", (int)fDbRoot, (int)pfucbSpace->fAvailExt, (int)spp ) );
                Error( ErrERRCheck( pfucbSpace->fAvailExt ? JET_errSPAvailExtCorrupted : JET_errSPOwnExtCorrupted ) );
            }

            if ( ( spp == spp::ShelvedPool ) && ( spext.PgnoFirst() <= pgnoLast ) && ( spext.PgnoLast() > pgnoLast ) )
            {
                AssertTrack( fFalse, "LeakReclaimOverlappingShelvedExt" );
                Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
            }

            if ( ( spp != spp::ShelvedPool ) || ( spext.PgnoFirst() > pgnoLast ) )
            {
                for ( PGNO pgno = spext.PgnoFirst(); pgno <= spext.PgnoLast(); pgno++ )
                {
                    const BOOL fAvailSpace = pfucbSpace->fAvailExt && ( spp != spp::ShelvedPool );
                    Call( ErrSPILRProcessObjectSpaceOwnershipSetPgnos(
                                pspbmOwned,
                                parrShelved,
                                pgno,
                                pgnoLast,
                                fDbRoot,
                                fAvailSpace,
                                fFalse ) );
                }
            }
        }

        err = ErrBTNext( pfucbSpace, dib.dirflag );
        if ( err == JET_errNoCurrentRecord )
        {
            err = JET_errSuccess;
            goto HandleError;
        }
    }
    Call( err );

    if ( !fSmallSpace )
    {
        BTUp( pfucbSpace );
        for ( int iSpaceTree = 1; iSpaceTree <= 2; iSpaceTree++ )
        {
            SPLIT_BUFFER* pspbuf = NULL;
            Call( ErrBTIGotoRoot( pfucbSpace, latchRIW ) );
            Call( ErrSPIGetSPBuf( pfucbSpace, &pspbuf ) );
            if ( pspbuf->CpgBuffer1() > 0 )
            {
                for ( PGNO pgno = pspbuf->PgnoFirstBuffer1(); pgno <= pspbuf->PgnoLastBuffer1(); pgno++ )
                {
                    Call( ErrSPILRProcessObjectSpaceOwnershipSetPgnos(
                                pspbmOwned,
                                parrShelved,
                                pgno,
                                pgnoLast,
                                fDbRoot,
                                fTrue,
                                !fDbRoot ) );
                }
            }
            if ( pspbuf->CpgBuffer2() > 0 )
            {
                for ( PGNO pgno = pspbuf->PgnoFirstBuffer2(); pgno <= pspbuf->PgnoLastBuffer2(); pgno++ )
                {
                    Call( ErrSPILRProcessObjectSpaceOwnershipSetPgnos(
                                pspbmOwned,
                                parrShelved,
                                pgno,
                                pgnoLast,
                                fDbRoot,
                                fTrue,
                                !fDbRoot ) );
                }
            }

            const BOOL fAvailTree = pfucbSpace->fAvailExt;
            BTUp( pfucbSpace );
            BTClose( pfucbSpace );
            pfucbSpace = pfucbNil;

            if ( iSpaceTree == 1 )
            {
                if ( fAvailTree )
                {
                    Call( ErrSPIOpenOwnExt( ppib, pfcb, &pfucbSpace ) );
                }
                else
                {
                    Call( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbSpace ) );
                }                
            }
        }
    }

    err = JET_errSuccess;

HandleError:
    if ( pfucb != pfucbNil )
    {
        BTUp( pfucb );
        pfucb->pcsrRoot = pcsrNil;
    }

    if ( pfucbSpace != pfucbNil )
    {
        BTUp( pfucbSpace );
        BTClose( pfucbSpace );
        pfucbSpace = pfucbNil;
    }

    CATFreeCursorsFromObjid( pfucb, pfucbParentUnused );
    pfucb = pfucbNil;
    pfucbParentUnused = pfucbNil;

    return err;
}

LOCAL ERR ErrSPILRProcessPotentiallyLeakedPage(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const PGNO pgno,
    _Inout_ CSparseBitmap* const pspbmOwned,
    _Out_ BOOL* const pfLeaked )
{
    ERR err = JET_errSuccess;
    OBJID objid = objidNil;
    SpaceCategoryFlags spcatf = spcatfNone;
    SpaceCatCtx* pSpCatCtx = NULL;
    BOOL fNotLeaked = fFalse;

    *pfLeaked = fFalse;

    Call( ErrErrBitmapToJetErr( pspbmOwned->ErrGet( pgno - 1, &fNotLeaked ) ) );
    if ( fNotLeaked )
    {
        goto HandleError;
    }

    Call( ErrSPGetSpaceCategory(
            ppib,
            ifmp,
            pgno,
            objidSystemRoot,
            fFalse,
            &objid,
            &spcatf,
            &pSpCatCtx ) );

    const BOOL fRootDb = ( ( pSpCatCtx != NULL ) && ( pSpCatCtx->pfucb != pfucbNil ) ) ?
                             ( PgnoFDP( pSpCatCtx->pfucb ) == pgnoSystemRoot ) :
                             fFalse ;
    *pfLeaked = FSPSpaceCatIndeterminate( spcatf );

    if ( !(
             *pfLeaked ||
             (
                 fRootDb &&
                 (
                     FSPSpaceCatAnySpaceTree( spcatf ) ||
                     ( !FSPSpaceCatAnySpaceTree( spcatf ) && FSPSpaceCatRoot( spcatf ) && ( pgno == pgnoSystemRoot ) )
                 )
             ) ||
             (
                 !fRootDb && FSPSpaceCatAnySpaceTree( spcatf )
             )
       ) )
    {
        AssertTrack( fFalse, OSFormat( "LeakReclaimUnexpectedCat:%d:%d", (int)fRootDb, (int)spcatf ) );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    SPFreeSpaceCatCtx( &pSpCatCtx );

    if ( !( *pfLeaked ) )
    {
        Call( ErrErrBitmapToJetErr( pspbmOwned->ErrSet( pgno - 1, fTrue ) ) );
    }

HandleError:
    SPFreeSpaceCatCtx( &pSpCatCtx );
    return err;
}

typedef enum
{
    lrdrNone,
    lrdrCompleted,
    lrdrFailed,
    lrdrTimeout,
    lrdrMSysObjIdsNotReady,
} LeakReclaimerDoneReason;

ERR ErrSPReclaimSpaceLeaks( PIB* const ppib, const IFMP ifmp )
{
    ERR err = JET_errSuccess;
    ERR errVerClean = JET_errSuccess;
    LeakReclaimerDoneReason lrdr = lrdrNone;

    FMP* const pfmp = g_rgfmp + ifmp;
    pfmp->SetLeakReclaimerIsRunning();

    const HRT hrtStarted = HrtHRTCount();
    const LONG dtickQuota = pfmp->DtickLeakReclaimerTimeQuota();

    BOOL fDbOpen = fFalse;
    BOOL fInTransaction = fFalse;
    PGNO pgnoFirstReclaimed = pgnoNull, pgnoLastReclaimed = pgnoNull;
    CPG cpgReclaimed = 0, cpgShelved = -1;
    PGNO pgnoLastInitial = pfmp->PgnoLast();
    PGNO pgnoMaxToProcess = pgnoNull;
    PGNO pgnoFirstShelved = pgnoNull, pgnoLastShelved = pgnoNull;
    FUCB* pfucbCatalog = pfucbNil;
    FUCB* pfucbRoot = pfucbNil;
    CSparseBitmap spbmOwned;
    CArray<CPgnoFlagged> arrShelved;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortSpace );

    Assert( !pfmp->FIsTempDB() );


    IFMP ifmpDummy;
    Call( ErrDBOpenDatabase( ppib, pfmp->WszDatabaseName(), &ifmpDummy, JET_bitDbExclusive ) );
    EnforceSz( pfmp->FExclusiveBySession( ppib ), "LeakReclaimDbNotExclusive" );
    Assert( ifmpDummy == ifmp );
    fDbOpen = fTrue;

    {
    BOOL fMSysObjidsReady = fFalse;
    Call( ErrCATCheckMSObjidsReady( ppib, ifmp, &fMSysObjidsReady ) );
    if ( !fMSysObjidsReady )
    {
        lrdr = lrdrMSysObjIdsNotReady;
        err = JET_errSuccess;
        goto HandleError;
    }
    }

    errVerClean = PverFromPpib( ppib )->ErrVERRCEClean( ifmp );
    if ( errVerClean != JET_errSuccess )
    {
        AssertTrack( fFalse, OSFormat( "LeakReclaimBeginVerCleanErr:%d", errVerClean ) );
        Error( ErrERRCheck( JET_errDatabaseInUse ) );
    }
    pfmp->WaitForTasksToComplete();

    if ( ( dtickQuota >= 0 ) && ( CmsecHRTFromHrtStart( hrtStarted ) >= (QWORD)dtickQuota ) )
    {
        lrdr = lrdrTimeout;
        err = JET_errSuccess;
        goto HandleError;
    }

    Call( ErrSPGetLastPgno( ppib, ifmp, &pgnoLastInitial ) );
    EnforceSz( pgnoLastInitial == pfmp->PgnoLast(), "LeakReclaimPgnoLastMismatch" );
    pgnoMaxToProcess = pgnoLastInitial;

    Call( ErrErrBitmapToJetErr( spbmOwned.ErrInitBitmap( pgnoLastInitial ) ) );



    Call( ErrSPILRProcessObjectSpaceOwnership( ppib, ifmp, objidSystemRoot, &spbmOwned, &arrShelved ) );
    cpgShelved = arrShelved.Size();
    if ( arrShelved.Size() > 0 )
    {
        pgnoFirstShelved = arrShelved.Entry( 0 ).Pgno();
        pgnoLastShelved = arrShelved.Entry( arrShelved.Size() - 1 ).Pgno();
        Assert( pgnoFirstShelved <= pgnoLastShelved );

        Assert( pgnoLastShelved > pgnoMaxToProcess );
        pgnoMaxToProcess = pgnoLastShelved;
    }

    if ( ( dtickQuota >= 0 ) && ( CmsecHRTFromHrtStart( hrtStarted ) >= (QWORD)dtickQuota ) )
    {
        lrdr = lrdrTimeout;
        err = JET_errSuccess;
        goto HandleError;
    }

    {
    OBJID objid = objidNil;
    for ( err = ErrCATGetNextRootObject( ppib, ifmp, &pfucbCatalog, &objid );
        ( err >= JET_errSuccess ) && ( objid != objidNil );
        err = ErrCATGetNextRootObject( ppib, ifmp, &pfucbCatalog, &objid ) )
    {
        Call( ErrSPILRProcessObjectSpaceOwnership( ppib, ifmp, objid, &spbmOwned, &arrShelved ) );

        if ( ( dtickQuota >= 0 ) && ( CmsecHRTFromHrtStart( hrtStarted ) >= (QWORD)dtickQuota ) )
        {
            lrdr = lrdrTimeout;
            err = JET_errSuccess;
            goto HandleError;
        }
    }
    Call( err );
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;
    }

    EnforceSz( cpgShelved == arrShelved.Size(), "LeakReclaimExtraShelvedPagesBefore" );
    EnforceSz( pgnoMaxToProcess < ulMax, "LeakReclaimPgnoMaxToProcessTooHigh" );


    {
    size_t iEntryShelvedFound = CArray<CPgnoFlagged>::iEntryNotFound;
    for ( PGNO pgnoCurrent = 1; pgnoCurrent <= pgnoMaxToProcess; )
    {
        PGNO pgnoFirstToReclaim = pgnoNull;
        while ( ( pgnoCurrent <= pgnoMaxToProcess ) && ( pgnoFirstToReclaim == pgnoNull ) )
        {
            BOOL fLeaked = fFalse;
            if ( pgnoCurrent <= pgnoLastInitial )
            {
                Assert( iEntryShelvedFound == CArray<CPgnoFlagged>::iEntryNotFound );
                Call( ErrSPILRProcessPotentiallyLeakedPage( ppib, ifmp, pgnoCurrent, &spbmOwned, &fLeaked ) );
            }
            else
            {
                const BOOL fFirstShelvedSeen = ( iEntryShelvedFound == CArray<CPgnoFlagged>::iEntryNotFound );
                iEntryShelvedFound = fFirstShelvedSeen ? 0 : ( iEntryShelvedFound + 1 );
                EnforceSz( arrShelved.Size() > 0, "LeakReclaimShelvedListEmpty" );
                EnforceSz( arrShelved.Size() > iEntryShelvedFound, "LeakReclaimShelvedListTooLow" );

                const CPgnoFlagged pgnoFlagged = arrShelved.Entry( iEntryShelvedFound );
                EnforceSz( ( pgnoFlagged.Pgno() >= pgnoCurrent ) || fFirstShelvedSeen, "LeakReclaimPgnoCurrentGoesBack" );
                pgnoCurrent = pgnoFlagged.Pgno();
                fLeaked = !pgnoFlagged.FFlagged();
            }

            if ( fLeaked )
            {
                pgnoFirstToReclaim = pgnoCurrent;
            }

            pgnoCurrent++;
        }

        EnforceSz( cpgShelved == arrShelved.Size(), "LeakReclaimExtraShelvedPagesFirst" );

        if ( pgnoFirstToReclaim == pgnoNull )
        {
            err = JET_errSuccess;
            goto HandleError;
        }

        PGNO pgnoMaxToReclaim = pgnoMaxToProcess;

        if ( pgnoFirstToReclaim <= pfmp->PgnoLast() )
        {
            Call( ErrDIRBeginTransaction( ppib, 37218, NO_GRBIT ) );
            fInTransaction = fTrue;

            Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucbRoot ) );

            CSPExtentInfo spext;
            err = ErrSPIFindExtOE( ppib, pfucbRoot->u.pfcb, pgnoFirstToReclaim, &spext );
            if ( ( err == JET_errNoCurrentRecord ) ||
                 ( err == JET_errRecordNotFound ) ||
                 ( ( err >= JET_errSuccess ) &&
                   ( !spext.FIsSet() || ( spext.PgnoFirst() > pgnoFirstToReclaim ) ) ) )
            {
                AssertTrack( fFalse, "LeakReclaimOeGap" );
                Error( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
            }
            Call( err );
            err = JET_errSuccess;

            pgnoMaxToReclaim = UlFunctionalMin( pgnoMaxToReclaim, spext.PgnoLast() );
            EnforceSz( pgnoMaxToReclaim >= pgnoFirstToReclaim, "LeakReclaimPgnoMaxToReclaimTooLow" );

            DIRClose( pfucbRoot );
            pfucbRoot = pfucbNil;
            Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
            fInTransaction = fFalse;
        }

        EnforceSz( pgnoCurrent == ( pgnoFirstToReclaim + 1 ), "LeakReclaimPgnoCurrentMisplacedFirst" );

        PGNO pgnoLastToReclaim = pgnoFirstToReclaim;
        while ( pgnoCurrent <= pgnoMaxToReclaim )
        {
            BOOL fLeaked = fFalse;
            if ( pgnoCurrent <= pgnoLastInitial )
            {
                Assert( pgnoFirstToReclaim <= pgnoLastInitial );
                Assert( iEntryShelvedFound == CArray<CPgnoFlagged>::iEntryNotFound );
                Call( ErrSPILRProcessPotentiallyLeakedPage( ppib, ifmp, pgnoCurrent, &spbmOwned, &fLeaked ) );
            }
            else
            {
                Assert( pgnoFirstToReclaim > pgnoLastInitial );
                Assert( iEntryShelvedFound != CArray<CPgnoFlagged>::iEntryNotFound );
                const size_t iEntryShelvedFoundT = ( iEntryShelvedFound == CArray<CPgnoFlagged>::iEntryNotFound ) ? 0 : ( iEntryShelvedFound + 1 );
                if ( arrShelved.Size() > iEntryShelvedFoundT )
                {
                    const CPgnoFlagged pgnoFlagged = arrShelved.Entry( iEntryShelvedFoundT );
                    fLeaked = ( ( pgnoFlagged.Pgno() == ( pgnoLastToReclaim + 1 ) ) && !pgnoFlagged.FFlagged() );
                    if ( fLeaked )
                    {
                        Assert( pgnoCurrent == pgnoFlagged.Pgno() );
                        iEntryShelvedFound = iEntryShelvedFoundT;
                    }
                }
                else
                {
                    fLeaked = fFalse;
                }
            }

            if ( fLeaked )
            {
                Assert( pgnoLastToReclaim == ( pgnoCurrent - 1 ) );
                pgnoLastToReclaim = pgnoCurrent;
                pgnoCurrent++;
            }
            else
            {
                break;
            }
        }


        EnforceSz( cpgShelved == arrShelved.Size(), "LeakReclaimExtraShelvedPagesLast" );
        EnforceSz( pgnoCurrent == ( pgnoLastToReclaim + 1 ), "LeakReclaimPgnoCurrentMisplacedLast" );
        EnforceSz( pgnoFirstToReclaim >= 1, "LeakReclaimPgnoFirstToReclaimTooLow" );
        EnforceSz( pgnoLastToReclaim >= 1, "LeakReclaimPgnoLastToReclaimTooLow" );
        EnforceSz( pgnoFirstToReclaim <= pgnoLastToReclaim, "LeakReclaimInvalidReclaimRange" );
        EnforceSz( pgnoLastToReclaim <= pgnoMaxToProcess, "LeakReclaimPgnoLastToReclaimTooHighMaxToProcess" );
        EnforceSz( pgnoLastToReclaim <= pgnoMaxToReclaim, "LeakReclaimPgnoLastToReclaimTooHighMaxToReclaim" );
        EnforceSz( ( ( pgnoFirstToReclaim <= pgnoLastInitial ) && ( pgnoLastToReclaim <= pgnoLastInitial ) ) ||
                   ( ( pgnoFirstToReclaim > pgnoLastInitial ) && ( pgnoLastToReclaim > pgnoLastInitial ) ),
                   "LeakReclaimRangeOverlapsInitialPgnoLast" );

        for ( PGNO pgnoToReclaim = pgnoFirstToReclaim; pgnoToReclaim <= pgnoLastToReclaim; pgnoToReclaim++ )
        {
            BOOL fNotLeaked = fTrue;
            if ( pgnoToReclaim <= pgnoLastInitial )
            {
                Call( ErrErrBitmapToJetErr( spbmOwned.ErrGet( pgnoToReclaim - 1, &fNotLeaked ) ) );
            }
            else
            {
                const size_t iEntry = arrShelved.SearchBinary( (CPgnoFlagged)pgnoToReclaim, PgnoShvdListCmpFunction );
                if ( iEntry == CArray<CPgnoFlagged>::iEntryNotFound )
                {
                    fNotLeaked = fTrue;
                }
                else
                {
                    const CPgnoFlagged pgnoflaggedFound = arrShelved.Entry( iEntry );
                    fNotLeaked = pgnoflaggedFound.FFlagged();
                }
            }

            EnforceSz( !fNotLeaked, "LeakReclaimRangeNotFullyLeaked" );
        }



        const CPG cpgToReclaim = pgnoLastToReclaim - pgnoFirstToReclaim + 1;

        Call( ErrDIRBeginTransaction( ppib, 51150, NO_GRBIT ) );
        fInTransaction = fTrue;
        Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucbRoot ) );
        const PGNO pgnoLastToReclaimBelowEof = UlFunctionalMin( pgnoLastToReclaim, pgnoLastInitial );
        if ( pgnoFirstToReclaim <= pgnoLastInitial )
        {
            Assert( pgnoLastToReclaimBelowEof >= pgnoFirstToReclaim );
            Call( ErrSPCaptureSnapshot( pfucbRoot, pgnoFirstToReclaim, pgnoLastToReclaimBelowEof - pgnoFirstToReclaim + 1 ) );
        }
        Call( ErrSPFreeExt( pfucbRoot, pgnoFirstToReclaim, cpgToReclaim, "LeakReclaimer" ) );
        cpgReclaimed += cpgToReclaim;
        if ( pgnoFirstReclaimed == pgnoNull )
        {
            pgnoFirstReclaimed = pgnoFirstToReclaim;
        }
        pgnoLastReclaimed = pgnoLastToReclaim;
        DIRClose( pfucbRoot );
        pfucbRoot = pfucbNil;
        Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
        fInTransaction = fFalse;

        for ( PGNO pgno = pgnoFirstToReclaim; pgno <= pgnoLastToReclaimBelowEof; pgno++ )
        {
            BFMarkAsSuperCold( ifmp, pgno );
        }

        if ( ( dtickQuota >= 0 ) && ( CmsecHRTFromHrtStart( hrtStarted ) >= (QWORD)dtickQuota ) )
        {
            lrdr = lrdrTimeout;
            err = JET_errSuccess;
            goto HandleError;
        }
    }
    }

    err = JET_errSuccess;

HandleError:


    if ( lrdr == lrdrNone )
    {
        lrdr = ( err >= JET_errSuccess ? lrdrCompleted : lrdrFailed );
    }



    if ( pfucbCatalog != pfucbNil )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }

    if ( pfucbRoot != pfucbNil )
    {
        DIRClose( pfucbRoot );
        pfucbRoot = pfucbNil;
    }

    if ( fInTransaction )
    {
        if ( err >= JET_errSuccess )
        {
            fInTransaction = ( ErrDIRCommitTransaction( ppib, NO_GRBIT ) < JET_errSuccess );
        }

        if ( fInTransaction )
        {
            CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
            fInTransaction = fFalse;
        }
    }

    errVerClean = PverFromPpib( ppib )->ErrVERRCEClean( ifmp );
    if ( errVerClean != JET_errSuccess )
    {
        AssertTrack( fFalse, OSFormat( "LeakReclaimEndVerCleanErr:%d", errVerClean ) );
        if ( errVerClean > JET_errSuccess )
        {
            errVerClean = ErrERRCheck( JET_errDatabaseInUse );
        }
    }
    else
    {
        pfmp->WaitForTasksToComplete();
        FCB::PurgeDatabase( ifmp, fFalse  );
    }

    Assert( errVerClean <= JET_errSuccess );
    if ( ( err >= JET_errSuccess ) && ( errVerClean < JET_errSuccess ) )
    {
        err = errVerClean;
    }

    if ( fDbOpen )
    {
        Assert( pfmp->FExclusiveBySession( ppib ) );
        CallS( ErrDBCloseDatabase( ppib, ifmp, NO_GRBIT ) );
        fDbOpen = fFalse;
    }

    pfmp->ResetLeakReclaimerIsRunning();



    OSTraceSuspendGC();
    const HRT dhrtElapsed = DhrtHRTElapsedFromHrtStart( hrtStarted );
    const double dblSecTotalElapsed = DblHRTSecondsElapsed( dhrtElapsed );
    const DWORD dwMinElapsed = (DWORD)( dblSecTotalElapsed / 60.0 );
    const double dblSecElapsed = dblSecTotalElapsed - (double)dwMinElapsed * 60.0;
    const WCHAR* rgwsz[] =
    {
        pfmp->WszDatabaseName(),
        OSFormatW( L"%d", (int)lrdr ),
        OSFormatW( L"%I32u", dwMinElapsed ), OSFormatW( L"%.2f", dblSecElapsed ),
        OSFormatW( L"%d", err ),
        OSFormatW( L"%I64u", pfmp->CbOfCpg( (CPG)pgnoLastInitial ) ), OSFormatW( L"%I32d", (CPG)pgnoLastInitial ),
        OSFormatW( L"%I64u", pfmp->CbOfCpg( (CPG)pfmp->PgnoLast() ) ), OSFormatW( L"%I32d", (CPG)pfmp->PgnoLast() ),
        OSFormatW( L"%I32d", cpgReclaimed ),
        ( pgnoFirstReclaimed != pgnoNull ) ? OSFormatW( L"%I32u", pgnoFirstReclaimed ) : L"-",
        ( pgnoLastReclaimed != pgnoNull ) ? OSFormatW( L"%I32u", pgnoLastReclaimed ) : L"-",
        ( cpgShelved != -1 ) ? OSFormatW( L"%I32d", cpgShelved ) : L"-",
        ( pgnoFirstShelved != pgnoNull ) ? OSFormatW( L"%I32u", pgnoFirstShelved ) : L"-",
        ( pgnoLastShelved != pgnoNull ) ? OSFormatW( L"%I32u", pgnoLastShelved ) : L"-",
    };
    UtilReportEvent(
        ( err < JET_errSuccess ) ? eventError : eventInformation,
        GENERAL_CATEGORY,
        ( err < JET_errSuccess ) ? DB_LEAK_RECLAIMER_FAILED_ID : DB_LEAK_RECLAIMER_SUCCEEDED_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        pfmp->Pinst() );
    OSTraceResumeGC();

    return err;
}


const ULONG cOEListEntriesInit  = 32;
const ULONG cOEListEntriesMax   = 127;

class OWNEXT_LIST
{
    public:
        OWNEXT_LIST( OWNEXT_LIST **ppOEListHead );
        ~OWNEXT_LIST();

    public:
        EXTENTINFO      *RgExtentInfo()         { return m_extentinfo; }
        ULONG           CEntries() const;
        OWNEXT_LIST     *POEListNext() const    { return m_pOEListNext; }
        VOID            AddExtentInfoEntry( const PGNO pgnoLast, const CPG cpgSize );

    private:
        EXTENTINFO      m_extentinfo[cOEListEntriesMax];
        ULONG           m_centries;
        OWNEXT_LIST     *m_pOEListNext;
};

INLINE OWNEXT_LIST::OWNEXT_LIST( OWNEXT_LIST **ppOEListHead )
{
    m_centries = 0;
    m_pOEListNext = *ppOEListHead;
    *ppOEListHead = this;
}

INLINE ULONG OWNEXT_LIST::CEntries() const
{
    Assert( m_centries <= cOEListEntriesMax );
    return m_centries;
}

INLINE VOID OWNEXT_LIST::AddExtentInfoEntry(
    const PGNO  pgnoLast,
    const CPG   cpgSize )
{
    Assert( m_centries < cOEListEntriesMax );
    m_extentinfo[m_centries].pgnoLastInExtent = pgnoLast;
    m_extentinfo[m_centries].cpgExtent = cpgSize;
    m_centries++;
}

INLINE ERR ErrSPIFreeOwnedExtentsInList(
    FUCB        *pfucbParent,
    EXTENTINFO  *rgextinfo,
    const ULONG cExtents )
{
    ERR         err;

    for ( size_t i = 0; i < cExtents; i++ )
    {
        const CPG   cpgSize = rgextinfo[i].CpgExtent();
        const PGNO  pgnoFirst = rgextinfo[i].PgnoLast() - cpgSize + 1;

        Assert( !FFUCBSpace( pfucbParent ) );
        CallR( ErrSPCaptureSnapshot( pfucbParent, pgnoFirst, cpgSize ) );
        CallR( ErrSPFreeExt( pfucbParent, pgnoFirst, cpgSize, "FreeFdpLarge" ) );
    }

    return JET_errSuccess;
}


LOCAL ERR ErrSPIFreeAllOwnedExtents( FUCB *pfucbParent, FCB *pfcb, const BOOL fPreservePrimaryExtent )
{
    ERR         err;
    FUCB        *pfucbOE;
    DIB         dib;
    PGNO        pgnoLastPrev;

    ULONG       cExtents    = 0;
    CPG         cpgOwned    = 0;

    Assert( pfcb != pfcbNil );

    CallR( ErrSPIOpenOwnExt( pfucbParent->ppib, pfcb, &pfucbOE ) );

    dib.pos = posFirst;
    dib.dirflag = fDIRNull;
    if ( ( err = ErrBTDown( pfucbOE, &dib, latchReadTouch ) ) < 0 )
    {
        BTClose( pfucbOE );
        return err;
    }
    Assert( wrnNDFoundLess != err );
    Assert( wrnNDFoundGreater != err );

    EXTENTINFO  extinfo[cOEListEntriesInit];
    OWNEXT_LIST *pOEList        = NULL;
    OWNEXT_LIST *pOEListCurr    = NULL;
    ULONG       cOEListEntries  = 0;



    pgnoLastPrev = 0;

    do
    {
        const CSPExtentInfo     spOwnedExt( pfucbOE );
        CallS( spOwnedExt.ErrCheckCorrupted() );

        if ( pgnoLastPrev >  ( spOwnedExt.PgnoFirst() - 1 ) )
        {
            AssertSzRTL( fFalse, "OwnExt nodes are not in monotonically-increasing key order." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbOE ), HaDbFailureTagCorruption, L"eb31e6ec-553f-4ac9-b2f6-2561ae325954" );
            Call( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
        }

        pgnoLastPrev = spOwnedExt.PgnoLast();

        cExtents++;
        cpgOwned += spOwnedExt.CpgExtent();

        if ( !fPreservePrimaryExtent
            || ( pfcb->PgnoFDP() != spOwnedExt.PgnoFirst() ) )
        {

            if ( cOEListEntries < cOEListEntriesInit )
            {
                extinfo[cOEListEntries].pgnoLastInExtent = spOwnedExt.PgnoLast();
                extinfo[cOEListEntries].cpgExtent = spOwnedExt.CpgExtent();
            }
            else
            {
                Assert( ( NULL == pOEListCurr && NULL == pOEList )
                    || ( NULL != pOEListCurr && NULL != pOEList ) );
                if ( NULL == pOEListCurr || pOEListCurr->CEntries() == cOEListEntriesMax )
                {
                    pOEListCurr = (OWNEXT_LIST *)PvOSMemoryHeapAlloc( sizeof( OWNEXT_LIST ) );
                    if ( NULL == pOEListCurr )
                    {
                        Assert( pfucbNil != pfucbOE );
                        BTClose( pfucbOE );
                        err = ErrERRCheck( JET_errOutOfMemory );
                        goto HandleError;
                    }
                    new( pOEListCurr ) OWNEXT_LIST( &pOEList );

                    Assert( pOEList == pOEListCurr );
                }

                pOEListCurr->AddExtentInfoEntry( spOwnedExt.PgnoLast(), spOwnedExt.CpgExtent() );
            }

            cOEListEntries++;
        }

        err = ErrBTNext( pfucbOE, fDIRNull );
    }
    while ( err >= 0 );

    OSTraceFMP( pfucbOE->ifmp, JET_tracetagSpaceInternal,
                OSFormat( "Free FDP with %08d owned pages and %08d owned extents [objid:0x%x,pgnoFDP:0x%x,ifmp=0x%x]\n",
                            cpgOwned, cExtents, pfcb->ObjidFDP(), pfcb->PgnoFDP(), pfucbOE->ifmp ) );


    Assert( pfucbNil != pfucbOE );
    BTClose( pfucbOE );


    if ( err != JET_errNoCurrentRecord )
    {
        Assert( err < 0 );
        goto HandleError;
    }

    Call( ErrSPIFreeOwnedExtentsInList(
            pfucbParent,
            extinfo,
            min( cOEListEntries, cOEListEntriesInit ) ) );

    for ( pOEListCurr = pOEList;
        pOEListCurr != NULL;
        pOEListCurr = pOEListCurr->POEListNext() )
    {
        Assert( cOEListEntries > cOEListEntriesInit );
        Call( ErrSPIFreeOwnedExtentsInList(
                pfucbParent,
                pOEListCurr->RgExtentInfo(),
                pOEListCurr->CEntries() ) );
    }

    PERFOpt( cSPDeletedTreeFreedPages.Add( PinstFromPfucb( pfucbParent ), cpgOwned ) );
    PERFOpt( cSPDeletedTreeFreedExtents.Add( PinstFromPfucb( pfucbParent ), cExtents ) );

HandleError:
    pOEListCurr = pOEList;
    while ( pOEListCurr != NULL )
    {
        OWNEXT_LIST *pOEListKill = pOEListCurr;

#ifdef DEBUG
        Assert( cOEListEntries > cOEListEntriesInit );
        Assert( cOEListEntries > pOEListCurr->CEntries() );
        cOEListEntries -= pOEListCurr->CEntries();
#endif

        pOEListCurr = pOEListCurr->POEListNext();

        OSMemoryHeapFree( pOEListKill );
    }
    Assert( cOEListEntries <= cOEListEntriesInit );

    return err;
}


LOCAL ERR ErrSPIReportAEsFreedWithFDP( PIB * const ppib, FCB * const pfcb )
{
    ERR         err;
    FUCB        *pfucbAE;
    DIB         dib;
    ULONG       cExtents    = 0;
    CPG         cpgFree     = 0;

    Assert( pfcb != pfcbNil );

    CallR( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbAE ) );

    dib.pos = posFirst;
    dib.dirflag = fDIRNull;
    err = ErrBTDown( pfucbAE, &dib, latchReadTouch );
    Assert( wrnNDFoundLess != err );
    Assert( wrnNDFoundGreater != err );
    if ( err == JET_errRecordNotFound )
    {
        OSTraceFMP( pfucbAE->ifmp, JET_tracetagSpaceInternal,
                        OSFormat( "Free FDP with 0 avail pages [objid:0x%x,pgnoFDP:0x%x,ifmp=0x%x]\n",
                                pfcb->ObjidFDP(), pfcb->PgnoFDP(), pfucbAE->ifmp ) );
        err = JET_errSuccess;
        goto HandleError;
    }

    do
    {
        Call( err );

        const CSPExtentInfo     spavailext( pfucbAE );

        cExtents++;
        cpgFree += spavailext.CpgExtent();

        err = ErrBTNext( pfucbAE, fDIRNull );
    }
    while ( JET_errNoCurrentRecord != err );

    err = JET_errSuccess;

    OSTraceFMP( pfucbAE->ifmp, JET_tracetagSpaceInternal,
                    OSFormat( "Free FDP with %08d avail pages and %08u avail extents [objid:0x%x,pgnoFDP:0x%x,ifmp=0x%x]\n",
                            cpgFree, cExtents, pfcb->ObjidFDP(), pfcb->PgnoFDP(), pfucbAE->ifmp ) );

HandleError:
    Assert( pfucbNil != pfucbAE );
    BTClose( pfucbAE );

    return err;
}


ERR ErrSPFreeFDP(
    PIB         *ppib,
    FCB         *pfcbFDPToFree,
    const PGNO  pgnoFDPParent,
    const BOOL  fPreservePrimaryExtent )
{
    ERR         err;
    const IFMP  ifmp            = pfcbFDPToFree->Ifmp();
    const PGNO  pgnoFDPFree     = pfcbFDPToFree->PgnoFDP();
    FUCB        *pfucbParent    = pfucbNil;
    FUCB        *pfucb          = pfucbNil;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = pfcbFDPToFree->TCE( fTrue );
    tcScope->SetDwEngineObjid( pfcbFDPToFree->ObjidFDP() );
    tcScope->iorReason.SetIort( iortSpace );

    BOOL    fBeginTrx   = fFalse;
    if ( ppib->Level() == 0 )
    {
        CallR( ErrDIRBeginTransaction( ppib, 47141, NO_GRBIT ) );
        fBeginTrx   = fTrue;
    }

    Assert( pgnoNull != pgnoFDPParent );
    Assert( pgnoNull != pgnoFDPFree );

    Assert( !FFMPIsTempDB( ifmp ) || pgnoSystemRoot == pgnoFDPParent );

    Call( ErrBTOpen( ppib, pgnoFDPParent, ifmp, &pfucbParent ) );
    Assert( pfucbNil != pfucbParent );
    Assert( pfucbParent->u.pfcb->FInitialized() );

#ifdef SPACECHECK
    CallS( ErrSPIValidFDP( ppib, pfucbParent->ifmp, pgnoFDPFree ) );
#endif

    OSTraceFMP( pfucbParent->ifmp, JET_tracetagSpaceManagement,
                    OSFormat( "free space FDP at %d.%lu\n", pfucbParent->ifmp, pgnoFDPFree ) );

    Call( ErrBTOpen( ppib, pfcbFDPToFree, &pfucb ) );
    Assert( pfucbNil != pfucb );
    Assert( pfucb->u.pfcb->FInitialized() );
    Assert( pfucb->u.pfcb->FDeleteCommitted() );
    FUCBSetIndex( pfucb );

    Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
    pfucb->pcsrRoot = Pcsr( pfucb );

#ifdef SPACECHECK
    CallS( ErrSPIWasAlloc( pfucb, pgnoFDPFree, 1 ) );
#endif

    Assert( pgnoFDPParent == PgnoSPIParentFDP( pfucb ) );
    Assert( pgnoFDPParent == PgnoFDP( pfucbParent ) );

    if ( !pfucb->u.pfcb->FSpaceInitialized() )
    {
        SPIInitFCB( pfucb, fTrue );
    }

    if ( FSPIIsSmall( pfucb->u.pfcb ) )
    {
        if ( !fPreservePrimaryExtent )
        {
            AssertSPIPfucbOnRoot( pfucb );

            NDGetExternalHeader( pfucb, pfucb->pcsrRoot, noderfSpaceHeader );
            Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
            const SPACE_HEADER * const psph = reinterpret_cast <SPACE_HEADER *> ( pfucb->kdfCurr.data.Pv() );

            ULONG cpgPrimary = psph->CpgPrimary();
            Assert( psph->CpgPrimary() != 0 );


            pfucb->pcsrRoot = pcsrNil;
            BTClose( pfucb );
            pfucb = pfucbNil;

            Assert( !FFUCBSpace( pfucbParent ) );
            Call( ErrSPCaptureSnapshot( pfucbParent, pgnoFDPFree, cpgPrimary ) );
            Call( ErrSPFreeExt( pfucbParent, pgnoFDPFree, cpgPrimary, "FreeFdpSmall" ) );
            PERFOpt( cSPDeletedTreeFreedPages.Add( PinstFromPfucb( pfucbParent ), cpgPrimary ) );
            PERFOpt( cSPDeletedTreeFreedExtents.Inc( PinstFromPfucb( pfucbParent ) ) );
        }
    }
    else
    {

        FCB *pfcb = pfucb->u.pfcb;
        pfucb->pcsrRoot = pcsrNil;
        BTClose( pfucb );
        pfucb = pfucbNil;

        if ( FOSTraceTagEnabled( JET_tracetagSpaceInternal ) &&
            FFMPIsTempDB( pfucbParent->ifmp ) )
        {
            Call( ErrSPIReportAEsFreedWithFDP( ppib, pfcb ) );
        }

        Call( ErrSPIFreeAllOwnedExtents( pfucbParent, pfcb, fPreservePrimaryExtent ) );
        Assert( !Pcsr( pfucbParent )->FLatched() );
    }
    PERFOpt( cSPDeletedTrees.Inc( PinstFromPfucb( pfucbParent ) ) );

HandleError:
    if ( pfucbNil != pfucb )
    {
        pfucb->pcsrRoot = pcsrNil;
        BTClose( pfucb );
    }

    if ( pfucbNil != pfucbParent )
    {
        BTClose( pfucbParent );
    }

    if ( fBeginTrx )
    {
        if ( err >= 0 )
        {
            err = ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush );
        }
        if ( err < 0 )
        {
            CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
        }
    }

#ifdef DEBUG
    if ( !FSPExpectedError( err ) )
    {
        CallS( err );
        AssertSz( fFalse, ( !FFMPIsTempDB( ifmp ) ?
                                "Space potentially lost permanently in user database." :
                                "Space potentially lost in temporary database." ) );
    }
#endif

    return err;
}

INLINE ERR ErrSPIAddExtent(
    __inout FUCB *pfucb,
    __in const CSPExtentNodeKDF * const pcspextnode )
{
    ERR         err;

    Assert( FFUCBSpace( pfucb ) );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( pcspextnode->CpgExtent() > 0 );

    Assert( pcspextnode->FValid() );

    const KEY   key     = pcspextnode->GetKey();
    const DATA  data    = pcspextnode->GetData();

    BTUp( pfucb );
    Call( ErrBTInsert(
                pfucb,
                key,
                data,
                fDIRNoVersion | ( pfucb->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
    Assert( Pcsr( pfucb )->FLatched() );

    Assert( pfucb->fOwnExt || pfucb->fAvailExt );
    OSTraceFMP( pfucb->ifmp, JET_tracetagSpaceManagement,
        OSFormat( "ErrSPIAddExtent: Add %lu + %d pages to 0x%x.%lu %hs.",
                pcspextnode->PgnoFirst(),
                pcspextnode->CpgExtent(),
                pfucb->ifmp,
                PgnoFDP( pfucb ),
                pfucb->fOwnExt ? "OE" : "AE" ) );

HandleError:
    Assert( errSPOutOfOwnExtCacheSpace != err );
    Assert( errSPOutOfAvailExtCacheSpace != err );
    return err;
}

LOCAL ERR ErrSPIAddToAvailExt(
    __in    FUCB *      pfucbAE,
    __in    const PGNO  pgnoAELast,
    __in    const CPG   cpgAESize,
    __in    SpacePool   sppPool )
{
    ERR         err;
    FCB         * const pfcb    = pfucbAE->u.pfcb;

    const CSPExtentNodeKDF  cspaei( SPEXTKEY::fSPExtentTypeAE, pgnoAELast, cpgAESize, sppPool );

    Assert( FFUCBAvailExt( pfucbAE ) );
    err = ErrSPIAddExtent( pfucbAE, &cspaei );
    if ( err < 0 )
    {
        if ( JET_errKeyDuplicate == err )
        {
            AssertSz( fFalse, "This is a bad thing, please show to ESE developers - likely explanation is the code around the ErrBTReplace() in ErrSPIAEFreeExt() is not taking care of insertion regions." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"917f5355-a040-4919-9503-81a9bdbcaa16" );
            err = ErrERRCheck( JET_errSPAvailExtCorrupted );
        }
    }

    else if ( ( sppPool != spp::ShelvedPool ) &&
              ( cspaei.CpgExtent() >= cpageSEDefault ) &&
              ( pgnoNull != pfcb->PgnoNextAvailSE() ) &&
              ( cspaei.PgnoFirst() < pfcb->PgnoNextAvailSE() ) )
    {
        pfcb->SetPgnoNextAvailSE( cspaei.PgnoFirst() );
    }

    return err;
}

LOCAL ERR ErrSPIAddToOwnExt(
    FUCB        *pfucb,
    const PGNO  pgnoOELast,
    CPG         cpgOESize,
    CPG         *pcpgCoalesced )
{
    ERR         err;
    FUCB        *pfucbOE;

    CallR( ErrSPIOpenOwnExt( pfucb->ppib, pfucb->u.pfcb, &pfucbOE ) );
    Assert( FFUCBOwnExt( pfucbOE ) );

    if ( NULL != pcpgCoalesced && FFMPIsTempDB( pfucb->ifmp ) )
    {
        DIB         dib;

        const CSPExtentKeyBM    spPgnoBeforeFirst( SPEXTKEY::fSPExtentTypeOE, pgnoOELast - cpgOESize );

        dib.pos     = posDown;
        dib.pbm     = spPgnoBeforeFirst.Pbm( pfucbOE );
        dib.dirflag = fDIRExact;
        err = ErrBTDown( pfucbOE, &dib, latchReadTouch );
        if ( JET_errRecordNotFound == err )
        {
            err = JET_errSuccess;
        }
        else if ( JET_errSuccess == err )
        {
            CSPExtentInfo       spextToCoallesce( pfucbOE );

            Assert( spextToCoallesce.PgnoLast() == pgnoOELast - cpgOESize );

            err = spextToCoallesce.ErrCheckCorrupted();
            if ( err < JET_errSuccess )
            {
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbOE ), HaDbFailureTagCorruption, L"4441dbd5-f6ff-4314-9315-efe96362f0a2" );
            }
            Call( err );
            
            Assert( spextToCoallesce.CpgExtent() > 0 );
            Call( ErrBTFlagDelete(
                        pfucbOE,
                        fDIRNoVersion | ( pfucbOE->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );

            Assert( NULL != pcpgCoalesced );
            *pcpgCoalesced = spextToCoallesce.CpgExtent();

            cpgOESize += spextToCoallesce.CpgExtent();
        }
        else
        {
            Call( err );
        }

        BTUp( pfucbOE );
    }

    if( pgnoSystemRoot == pfucb->u.pfcb->PgnoFDP() )
    {
        QWORD cbFsFileSize = 0;
        if ( g_rgfmp[pfucb->ifmp].Pfapi()->ErrSize( &cbFsFileSize, IFileAPI::filesizeLogical ) >= JET_errSuccess )
        {
            AssertTrack( CbFileSizeOfPgnoLast( pgnoOELast ) <= cbFsFileSize, "RootPgnoOeLastBeyondEof" );
        }
    }

    {
    const CSPExtentNodeKDF  cspextnode( SPEXTKEY::fSPExtentTypeOE, pgnoOELast, cpgOESize );

    Call( ErrSPIAddExtent( pfucbOE, &cspextnode ) );
    }

HandleError:
    Assert( errSPOutOfOwnExtCacheSpace != err );
    Assert( errSPOutOfAvailExtCacheSpace != err );
    pfucbOE->pcsrRoot = pcsrNil;
    BTClose( pfucbOE );
    return err;
}

LOCAL ERR ErrSPICoalesceAvailExt(
    FUCB        *pfucbAE,
    const PGNO  pgnoLast,
    const CPG   cpgSize,
    CPG         *pcpgCoalesce )
{
    ERR         err;
    DIB         dib;

    Assert( FFMPIsTempDB( pfucbAE->ifmp ) );

    *pcpgCoalesce = 0;

    CSPExtentKeyBM spavailextSeek( SPEXTKEY::fSPExtentTypeAE, pgnoLast - cpgSize, spp::AvailExtLegacyGeneralPool );

    dib.pos     = posDown;
    dib.pbm     = spavailextSeek.Pbm( pfucbAE );
    dib.dirflag = fDIRNull;
    err = ErrBTDown( pfucbAE, &dib, latchReadTouch );
    if ( JET_errRecordNotFound == err )
    {
        err = JET_errSuccess;
    }
    else if ( JET_errSuccess == err )
    {
        const CSPExtentInfo     spavailext( pfucbAE );
#ifdef DEBUG
        Assert( spavailext.PgnoLast() == pgnoLast - cpgSize );
#endif

        err = spavailext.ErrCheckCorrupted();
        if ( err < JET_errSuccess )
        {
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbAE ), HaDbFailureTagCorruption, L"9f63b812-97ed-46ce-b40d-3a05a844351b" );
        }
        Call( err );

        *pcpgCoalesce = spavailext.CpgExtent();

        err = ErrBTFlagDelete(
                    pfucbAE,
                    fDIRNoVersion | ( pfucbAE->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) );
    }

HandleError:

    BTUp( pfucbAE );

    return err;
}


LOCAL ERR ErrSPIAddSecondaryExtent(
    __in FUCB* const     pfucb,
    __in FUCB* const     pfucbAE,
    __in const PGNO      pgnoLast,
    __in CPG             cpgNewSpace,
    __in CPG             cpgAvailable,
    __in CArray<EXTENTINFO>* const parreiReleased,
    __in const SpacePool sppPool )
{
    ERR err;
    CPG cpgOECoalesced  = 0;
    const BOOL fRootDB = ( pfucb->u.pfcb->PgnoFDP() == pgnoSystemRoot );
    BOOL fAddedToOwnExt = fFalse;
    BOOL fAddedToAvailExt = fFalse;

    Assert( cpgNewSpace > 0 );
    Assert( cpgAvailable > 0 );

    Assert( ( cpgNewSpace == cpgAvailable ) ||
            ( fRootDB && ( cpgNewSpace > cpgAvailable ) ) );
    Assert( ( parreiReleased == NULL ) || ( parreiReleased->Size() == 0 ) ||
            ( fRootDB && ( cpgNewSpace > cpgAvailable ) ) );
    Assert( ( cpgNewSpace == cpgAvailable ) ||
            ( ( cpgNewSpace > cpgAvailable ) && !g_rgfmp[pfucb->ifmp].FIsTempDB() ) );

    Assert( sppPool != spp::ShelvedPool );

    Assert( !Pcsr( pfucbAE )->FLatched() );

    Call( ErrSPIAddToOwnExt(
                pfucb,
                pgnoLast,
                cpgNewSpace,
                &cpgOECoalesced ) );
    fAddedToOwnExt = fTrue;

    if ( fRootDB )
    {
        g_rgfmp[pfucb->ifmp].SetOwnedFileSize( CbFileSizeOfPgnoLast( pgnoLast ) );
    }

    if ( cpgOECoalesced > 0 )
    {
        Assert( g_rgfmp[pfucb->ifmp].Dbid() == dbidTemp );
        Expected( sppPool == spp::AvailExtLegacyGeneralPool );
        if ( sppPool == spp::AvailExtLegacyGeneralPool )
        {
            CPG cpgAECoalesced;

            Call( ErrSPICoalesceAvailExt( pfucbAE, pgnoLast, cpgAvailable, &cpgAECoalesced ) );

            Assert( cpgAECoalesced <= cpgOECoalesced );
            cpgAvailable += cpgAECoalesced;
            Assert( cpgAvailable > 0 );
        }
    }

    Assert( !Pcsr( pfucbAE )->FLatched() );

    if ( ( parreiReleased != NULL ) && ( parreiReleased->Size() > 0 ) )
    {
        Assert( !Pcsr( pfucbAE )->FLatched() );

        while ( parreiReleased->Size() > 0 )
        {
            const EXTENTINFO& extinfoReleased = parreiReleased->Entry( parreiReleased->Size() - 1 );
            Assert( extinfoReleased.FValid() && ( extinfoReleased.CpgExtent() > 0 ) );
            Call( ErrSPIAEFreeExt( pfucb, extinfoReleased.PgnoFirst(), extinfoReleased.CpgExtent(), pfucbNil ) );
            CallS( ( parreiReleased->ErrSetSize( parreiReleased->Size() - 1 ) == CArray<EXTENTINFO>::ERR::errSuccess ) ?
                                                                                 JET_errSuccess :
                                                                                 ErrERRCheck( JET_errOutOfMemory ) );
        }
        Assert( !Pcsr( pfucbAE )->FLatched() );

        Call( ErrSPIReserveSPBufPages( pfucb, pfucbNil ) );
        Assert( !Pcsr( pfucbAE )->FLatched() );
    }

    if ( fRootDB && ( cpgNewSpace > cpgAvailable ) )
    {
        Assert( !g_rgfmp[pfucb->ifmp].FIsTempDB() );
        Assert( !Pcsr( pfucbAE )->FLatched() );

        Call( ErrSPIUnshelvePagesInRange(
                pfucb,
                pgnoLast - cpgNewSpace + 1,
                pgnoLast - cpgAvailable ) );
        Assert( !Pcsr( pfucbAE )->FLatched() );

        Call( ErrSPIReserveSPBufPages( pfucb, pfucbNil ) );
        Assert( !Pcsr( pfucbAE )->FLatched() );
    }

    Call( ErrSPIAddToAvailExt( pfucbAE, pgnoLast, cpgAvailable, sppPool ) );
    fAddedToAvailExt = fTrue;
    Assert( Pcsr( pfucbAE )->FLatched() );

HandleError:
    Assert( ( err < JET_errSuccess ) || ( fAddedToOwnExt && fAddedToAvailExt ) );
    if ( fAddedToOwnExt && !fAddedToAvailExt )
    {
        SPIReportSpaceLeak( pfucb, err, pgnoLast - cpgAvailable + 1, cpgAvailable, "NewExt" );
    }

    return err;
}


INLINE ERR ErrSPICheckSmallFDP( FUCB *pfucb, BOOL *pfSmallFDP )
{
    ERR     err;
    FUCB    *pfucbOE    = pfucbNil;
    CPG     cpgOwned    = 0;
    DIB     dib;

    CallR( ErrSPIOpenOwnExt( pfucb->ppib, pfucb->u.pfcb, &pfucbOE ) );
    Assert( pfucbNil != pfucbOE );

    dib.pos = posFirst;
    dib.dirflag = fDIRNull;
    err = ErrBTDown( pfucbOE, &dib, latchReadTouch );
    Assert( err != JET_errNoCurrentRecord );
    Assert( err != JET_errRecordNotFound );

    *pfSmallFDP = fTrue;

    do
    {
        Call( err );

        const CSPExtentInfo     spextOwnedCurr( pfucbOE );

        cpgOwned += spextOwnedCurr.CpgExtent();
        if ( cpgOwned > cpgSmallFDP )
        {
            *pfSmallFDP = fFalse;
            break;
        }

        err = ErrBTNext( pfucbOE, fDIRNull );
    }
    while ( JET_errNoCurrentRecord != err );

    err = JET_errSuccess;

HandleError:
    Assert( pfucbNil != pfucbOE );
    BTClose( pfucbOE );
    return err;
}


LOCAL VOID SPReportMaxDbSizeExceeded( const IFMP ifmp, const CPG cpg )
{

    WCHAR       wszCurrentSizeMb[16];
    const WCHAR * rgcwszT[2]          = { g_rgfmp[ifmp].WszDatabaseName(), wszCurrentSizeMb };

    OSStrCbFormatW( wszCurrentSizeMb, sizeof(wszCurrentSizeMb), L"%d", (ULONG)(( (QWORD)cpg * (QWORD)( g_cbPage >> 10  ) ) >> 10 ) );

    UtilReportEvent(
            eventWarning,
            SPACE_MANAGER_CATEGORY,
            SPACE_MAX_DB_SIZE_REACHED_ID,
            2,
            rgcwszT,
            0,
            NULL,
            PinstFromIfmp( ifmp ) );
}

LOCAL ERR ErrSPINewSize(
    const TraceContext& tc,
    const IFMP  ifmp,
    const PGNO  pgnoLastCurr,
    const CPG   cpgReq,
    const CPG   cpgAsyncExtension )
{
    ERR err = JET_errSuccess;
    BOOL fUpdateLgposResizeHdr = fFalse;
    LGPOS lgposResize = lgposMin;

    OnDebug( g_rgfmp[ifmp].AssertSafeToChangeOwnedSize() );
    Assert( ( cpgReq <= 0 ) || !g_rgfmp[ifmp].FBeyondPgnoShrinkTarget( pgnoLastCurr + 1, cpgReq ) );
    Assert( ( cpgReq <= 0 ) || ( pgnoLastCurr <= g_rgfmp[ifmp].PgnoLast() ) );
    Expected( cpgAsyncExtension >= 0 );


    if ( cpgReq < 0 )
    {
        Call( ErrBFPreparePageRangeForExternalZeroing( ifmp, pgnoLastCurr + cpgReq + 1, -1 * cpgReq, tc ) );
    }


    if ( g_rgfmp[ifmp].FLogOn() )
    {
        LOG* const plog = PinstFromIfmp( ifmp )->m_plog;
        if ( cpgReq >= 0 )
        {
            Call( ErrLGExtendDatabase( plog, ifmp, pgnoLastCurr + cpgReq, &lgposResize ) );
        }
        else
        {
            if ( g_rgfmp[ifmp].FRBSOn() )
            {
                Call( g_rgfmp[ifmp].PRBS()->ErrFlushAll() );
            }

            Call( ErrLGShrinkDatabase( plog, ifmp, pgnoLastCurr + cpgReq, -1 * cpgReq, &lgposResize ) );
        }

        fUpdateLgposResizeHdr = ( g_rgfmp[ifmp].ErrDBFormatFeatureEnabled( JET_efvLgposLastResize ) == JET_errSuccess );
    }

    Call( ErrIOResizeUpdateDbHdrCount( ifmp, ( cpgReq >= 0 )  ) );


    Call( ErrIONewSize(
            ifmp,
            tc,
            pgnoLastCurr + cpgReq,
            cpgAsyncExtension,
            ( cpgReq >= 0 ) ? JET_bitResizeDatabaseOnlyGrow : JET_bitResizeDatabaseOnlyShrink ) );


    Call( ErrIOFlushDatabaseFileBuffers( ifmp, iofrDbResize ) );


    if ( fUpdateLgposResizeHdr )
    {
        Assert( CmpLgpos( lgposResize, lgposMin ) != 0 );
        Call( ErrIOResizeUpdateDbHdrLgposLast( ifmp, lgposResize ) );
    }

HandleError:
    OSTraceFMP(
        ifmp,
        JET_tracetagSpaceManagement,
        OSFormat(
            "Request to resize database=['%ws':0x%x] by %I64d bytes to %I64u bytes (and an additional %I64d bytes asynchronously) completed with error %d (0x%x)",
            g_rgfmp[ifmp].WszDatabaseName(),
            ifmp,
            (__int64)cpgReq * g_cbPage,
            QWORD( pgnoLastCurr + cpgReq ) * g_cbPage,
            (__int64)cpgAsyncExtension * g_cbPage,
            err,
            err ) );

    return err;
}

LOCAL ERR ErrSPIWriteZeroesDatabase(
    _In_ const IFMP ifmp,
    _In_ const QWORD ibOffsetStart,
    _In_ const QWORD cbZeroes,
    _In_ const TraceContext& tc )
{
    Assert( 0 != cbZeroes );

    ERR err = JET_errSuccess;
    DWORD_PTR dwRangeLockContext = NULL;
    const PGNO pgnoStart = PgnoOfOffset( ibOffsetStart );
    const PGNO pgnoEnd = PgnoOfOffset( ibOffsetStart + cbZeroes ) - 1;

    Assert( 0 == ( cbZeroes % ( g_cbPage ) ) );
    Assert( pgnoStart <= pgnoEnd );
    Assert( pgnoStart >= PgnoOfOffset( cpgDBReserved * g_cbPage ) );
    Assert( pgnoStart != pgnoNull );
    Assert( pgnoEnd != pgnoNull );
    Assert( ibOffsetStart == OffsetOfPgno( pgnoStart ) );
    Assert( ( ibOffsetStart + cbZeroes ) == OffsetOfPgno( pgnoEnd + 1 ) );

    FMP* const pfmp = &g_rgfmp[ ifmp ];
    const CPG cpg = ( pgnoEnd - pgnoStart ) + 1;
    TraceContextScope tcScope( iorpSPDatabaseInlineZero, iorsNone, iortSpace );

    OSTraceFMP(
        ifmp,
        JET_tracetagSpaceManagement,
        OSFormat(
            "%hs: Zeroing %I64d k at %#I64x (pages %lu through %lu).\n",
             __FUNCTION__, cbZeroes / 1024, ibOffsetStart, pgnoStart, pgnoEnd ) );


    CPG cpgZeroOptimal = CpgBFGetOptimalLockPageRangeSizeForExternalZeroing( ifmp );
    cpgZeroOptimal = LFunctionalMin( cpgZeroOptimal, pfmp->CpgOfCb( ::g_cbZero ) );
    PGNO pgnoZeroRangeThis = pgnoStart;
    while ( pgnoZeroRangeThis <= pgnoEnd )
    {
        const CPG cpgZeroRangeThis = LFunctionalMin( cpgZeroOptimal, (CPG)( pgnoEnd - pgnoZeroRangeThis + 1 ) );

        dwRangeLockContext = NULL;
        Call( ErrBFLockPageRangeForExternalZeroing( ifmp, pgnoZeroRangeThis, cpgZeroRangeThis, fTrue, *tcScope, &dwRangeLockContext ) );

        Call( pfmp->Pfapi()->ErrIOWrite(
            *tcScope,
            OffsetOfPgno( pgnoZeroRangeThis ),
            (DWORD)pfmp->CbOfCpg( cpgZeroRangeThis ),
            g_rgbZero,
            qosIONormal | ( ( UlParam( PinstFromIfmp( ifmp ), JET_paramFlight_NewQueueOptions ) & bitUseMetedQ ) ? qosIODispatchWriteMeted : 0x0 ) ) );

        BFPurgeLockedPageRangeForExternalZeroing( dwRangeLockContext, *tcScope );

        BFUnlockPageRangeForExternalZeroing( dwRangeLockContext, *tcScope );
        dwRangeLockContext = NULL;

        pgnoZeroRangeThis += cpgZeroRangeThis;
    }

HandleError:
    BFUnlockPageRangeForExternalZeroing( dwRangeLockContext, *tcScope );
    return err;
}

ERR ErrSPITrimUpdateDatabaseHeader( const IFMP ifmp )
{
    ERR err     = JET_errSuccess;
    FMP *pfmp   = &g_rgfmp[ifmp];

    Assert( pfmp->Pdbfilehdr() );

    if ( NULL != pfmp->Pdbfilehdr() )
    {
        BOOL fUpdateHeader = fTrue;

    {
        PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();

        pdbfilehdr->le_ulTrimCount++;

#ifndef DEBUG
        fUpdateHeader = ( pdbfilehdr->le_ulTrimCount <= 2 );
#endif
    }

        if ( fUpdateHeader )
        {
            Call( ErrUtilWriteAttachedDatabaseHeaders( PinstFromIfmp( ifmp ), PinstFromIfmp( ifmp )->m_pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ) );
        }
    }

HandleError:
    return err;
}

LOCAL ERR ErrSPIExtendDB(
    FUCB        *pfucbRoot,
    const CPG   cpgSEMin,
    CPG         *pcpgSEReq,
    PGNO        *ppgnoSELast,
    const BOOL  fPermitAsyncExtension,
    const BOOL  fMayViolateMaxSize,
    CArray<EXTENTINFO>* const parreiReleased,
    CPG         *pcpgSEAvail )
{
    ERR         err                 = JET_errSuccess;
    PGNO        pgnoSEMaxAdj        = pgnoNull;
    CPG         cpgSEMaxAdj         = 0;
    PGNO        pgnoSELast          = pgnoNull;
    PGNO        pgnoSELastAdj       = pgnoNull;
    CPG         cpgAdj              = 0;
    CPG         cpgSEReq            = *pcpgSEReq;
    CPG         cpgSEReqAdj         = 0;
    CPG         cpgSEMinAdj         = 0;
    FUCB        *pfucbOE            = pfucbNil;
    FUCB        *pfucbAE            = pfucbNil;
    CPG         cpgAsyncExtension   = 0;
    DIB         dib;

    CSPExtentInfo speiAEShelved;

    Assert( cpgSEMin > 0 );
    Assert( cpgSEReq > 0 );
    Assert( cpgSEReq >= cpgSEMin );
    Assert( parreiReleased != NULL );
    Assert( pgnoSystemRoot == pfucbRoot->u.pfcb->PgnoFDP() );
    Assert( !g_rgfmp[pfucbRoot->ifmp].FReadOnlyAttach() );

    AssertSPIPfucbOnRoot( pfucbRoot );

    if ( g_rgfmp[pfucbRoot->ifmp].FShrinkIsActive() )
    {
        if ( fMayViolateMaxSize )
        {
            g_rgfmp[pfucbRoot->ifmp].ResetPgnoShrinkTarget();
        }
        else
        {
            Error( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ) );
        }
    }

    Call( ErrSPIOpenOwnExt( pfucbRoot->ppib, pfucbRoot->u.pfcb, &pfucbOE ) );

    dib.pos = posLast;
    dib.dirflag = fDIRNull;

    Call( ErrBTDown( pfucbOE, &dib, latchReadTouch ) );
    CallS( ErrSPIExtentLastPgno( pfucbOE, &pgnoSELast ) );
    pgnoSELastAdj = pgnoSELast;
    BTUp( pfucbOE );

    pgnoSEMaxAdj = pgnoSysMax;
    if ( g_rgfmp[pfucbRoot->ifmp].CpgDatabaseSizeMax() > 0 )
    {
        pgnoSEMaxAdj = min( pgnoSEMaxAdj, (PGNO)g_rgfmp[pfucbRoot->ifmp].CpgDatabaseSizeMax() );
    }

    if ( pgnoSEMaxAdj >= pgnoSELast )
    {
        cpgSEMaxAdj = pgnoSEMaxAdj - pgnoSELast;
    }

    Call( ErrSPIOpenAvailExt( pfucbRoot->ppib, pfucbRoot->u.pfcb, &pfucbAE ) );
    Call( ErrSPISeekRootAE( pfucbAE, pgnoSELastAdj + 1, spp::ShelvedPool, &speiAEShelved ) );
    Assert( !speiAEShelved.FIsSet() || !g_rgfmp[pfucbRoot->ifmp].FIsTempDB() );
    while ( ( ( pgnoSELastAdj + cpgSEMin ) <= pgnoSysMax ) && speiAEShelved.FIsSet() )
    {
        Assert( speiAEShelved.PgnoFirst() > pgnoSELastAdj );
        const CPG cpgAvail = (CPG)( speiAEShelved.PgnoFirst() - pgnoSELastAdj - 1 );
        if ( cpgAvail >= cpgSEMin )
        {
            cpgSEMaxAdj = cpgAdj + cpgAvail;
            break;
        }

        if ( cpgAvail > 0 )
        {
            EXTENTINFO extinfoReleased;
            extinfoReleased.pgnoLastInExtent = pgnoSELastAdj + cpgAvail;
            extinfoReleased.cpgExtent = cpgAvail;
            Call( ( parreiReleased->ErrSetEntry( parreiReleased->Size(), extinfoReleased ) == CArray<EXTENTINFO>::ERR::errSuccess ) ?
                                                                                              JET_errSuccess :
                                                                                              ErrERRCheck( JET_errOutOfMemory ) );
        }

        pgnoSELastAdj = speiAEShelved.PgnoLast();
        cpgAdj = pgnoSELastAdj - pgnoSELast;

        speiAEShelved.Unset();
        err = ErrBTNext( pfucbAE, fDIRNull );
        if ( err >= JET_errSuccess )
        {
            speiAEShelved.Set( pfucbAE );
            if ( speiAEShelved.SppPool() != spp::ShelvedPool )
            {
                speiAEShelved.Unset();
            }
        }
        else if ( err == JET_errNoCurrentRecord )
        {
            err = JET_errSuccess;
        }
        Call( err );
    }
    BTUp( pfucbAE );
    BTClose( pfucbAE );
    pfucbAE = pfucbNil;

    if ( ( ( pgnoSELastAdj + cpgSEMin ) > pgnoSEMaxAdj ) &&
         ( !fMayViolateMaxSize || ( pgnoSEMaxAdj >= pgnoSysMax ) ) )
    {
        AssertTrack( !fMayViolateMaxSize, "ExtendDbMaxSizeBeyondPgnoSysMax" );
        SPReportMaxDbSizeExceeded( pfucbRoot->ifmp, (CPG)pgnoSELastAdj );
        Error( ErrERRCheck( JET_errOutOfDatabaseSpace ) );
    }

    AssertTrack( !speiAEShelved.FIsSet() ||
                 ( ( speiAEShelved.PgnoFirst() > pgnoSELastAdj ) &&
                   ( (CPG)( speiAEShelved.PgnoFirst() - pgnoSELastAdj - 1 ) >= cpgSEMin ) ), "ExtendDbUnprocessedShelvedExt" );
    
    Assert( cpgSEMaxAdj >= 0 );
    Assert( cpgAdj >= 0 );

    cpgSEMinAdj = cpgSEMin + cpgAdj;
    cpgSEReqAdj = max( cpgSEMinAdj, min( cpgSEReq, cpgSEMaxAdj ) );
    Assert( cpgSEMinAdj >= cpgSEMin );
    Assert( cpgSEReqAdj >= cpgSEMinAdj );

    if ( fPermitAsyncExtension && ( ( pgnoSELast + cpgSEReqAdj + cpgSEReq ) <= pgnoSEMaxAdj ) )
    {
        cpgAsyncExtension = cpgSEReq;
    }

    err = ErrSPINewSize( TcCurr(), pfucbRoot->ifmp, pgnoSELast, cpgSEReqAdj, cpgAsyncExtension );

    if ( err < JET_errSuccess )
    {
        err = ErrSPINewSize( TcCurr(), pfucbRoot->ifmp, pgnoSELast, cpgSEMinAdj, 0 );
        if( err < JET_errSuccess )
        {

            const CPG   cpgExtend   = 1;
            CPG         cpgT        = 0;

            do
            {
                cpgT += cpgExtend;
                Call( ErrSPINewSize( TcCurr(), pfucbRoot->ifmp, pgnoSELast, cpgT, 0 ) );
            }
            while ( cpgT < cpgSEMinAdj );
        }

        cpgSEReqAdj = cpgSEMinAdj;
    }

    pgnoSELastAdj = pgnoSELast + cpgSEReqAdj;
    *pcpgSEAvail = cpgSEReqAdj - cpgAdj;
    *pcpgSEReq = cpgSEReqAdj;
    *ppgnoSELast = pgnoSELastAdj;
    Assert( ( *ppgnoSELast > pgnoSELast ) && ( ( *ppgnoSELast <= pgnoSEMaxAdj ) || fMayViolateMaxSize ) );
    Assert( ( *pcpgSEAvail >= cpgSEMin ) && ( *pcpgSEAvail <= *pcpgSEReq ) );
    Assert( (CPG)( *ppgnoSELast - pgnoSELast ) == ( cpgAdj + *pcpgSEAvail ) );
    Assert( *pcpgSEReq == ( cpgAdj + *pcpgSEAvail ) );

HandleError:

    if ( pfucbNil != pfucbOE )
    {
        BTClose( pfucbOE );
    }

    if ( pfucbNil != pfucbAE )
    {
        BTClose( pfucbAE );
    }

    return err;
}

ERR ErrSPExtendDB(
    PIB *       ppib,
    const IFMP  ifmp,
    const CPG   cpgSEMin,
    PGNO        *ppgnoSELast,
    const BOOL  fPermitAsyncExtension )
{
    ERR         err;
    FUCB        *pfucbDbRoot        = pfucbNil;
    FUCB        *pfucbAE            = pfucbNil;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortSpace );
    tcScope->SetDwEngineObjid( objidSystemRoot );

    CallR( ErrBTIOpenAndGotoRoot( ppib, pgnoSystemRoot, ifmp, &pfucbDbRoot ) );
    tcScope->nParentObjectClass = TceFromFUCB( pfucbDbRoot );
    Assert( objidSystemRoot == ObjidFDP( pfucbDbRoot ) );

    Call( ErrSPIOpenAvailExt( pfucbDbRoot->ppib, pfucbDbRoot->u.pfcb, &pfucbAE ) );
    tcScope->nParentObjectClass = TceFromFUCB( pfucbAE );
    Assert( objidSystemRoot == ObjidFDP( pfucbAE ) );

    BTUp( pfucbAE );

    Assert( FSPIParentIsFs( pfucbDbRoot ) );
    Call( ErrSPIGetFsSe(
            pfucbDbRoot,
            pfucbAE,
            cpgSEMin,
            cpgSEMin,
            0,
            fTrue,
            fPermitAsyncExtension ) );

    Assert( Pcsr( pfucbAE )->FLatched() );

    BTUp( pfucbAE );

HandleError:
    
    if ( pfucbNil != pfucbAE )
    {
        BTClose( pfucbAE );
    }

    if ( pfucbNil != pfucbDbRoot )
    {
        BTClose( pfucbDbRoot );
    }

    return err;
}

LOCAL ERR ErrSPISeekRootOELast( __in FUCB* const pfucbOE, __out CSPExtentInfo* const pspeiOE )
{
    ERR err = JET_errSuccess;
    DIB dib;

    Assert( !Pcsr( pfucbOE )->FLatched() );
    Assert( FFUCBOwnExt( pfucbOE ) );
    Assert( pfucbOE->u.pfcb->PgnoFDP() == pgnoSystemRoot );

    dib.pos = posLast;
    dib.dirflag = fDIRNull;
    dib.pbm = NULL;
    err = ErrBTDown( pfucbOE, &dib, latchReadTouch );

    if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
    {
        FireWall( "SeekOeLastNoOwned" );
        Error( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
    }

    Call( err );

    pspeiOE->Set( pfucbOE );
    Call( pspeiOE->ErrCheckCorrupted() );

HandleError:
    if ( err < JET_errSuccess )
    {
        BTUp( pfucbOE );
    }

    return err;
}

LOCAL ERR ErrSPISeekRootAE(
    __in FUCB* const pfucbAE,
    __in const PGNO pgno,
    __in const SpacePool sppAvailPool,
    __out CSPExtentInfo* const pspeiAE )
{
    ERR err = JET_errSuccess;

    Assert( !Pcsr( pfucbAE )->FLatched() );
    Assert( FFUCBAvailExt( pfucbAE ) );
    Assert( pfucbAE->u.pfcb->PgnoFDP() == pgnoSystemRoot );

    err = ErrSPIFindExtAE( pfucbAE, pgno, sppAvailPool, pspeiAE );

    if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
    {
        err = JET_errSuccess;
        BTUp( pfucbAE );
        goto HandleError;
    }

    Call( err );

    pspeiAE->Set( pfucbAE );
    Call( pspeiAE->ErrCheckCorrupted() );

HandleError:
    if ( err < JET_errSuccess )
    {
        BTUp( pfucbAE );
    }

    return err;
}

LOCAL ERR ErrSPICheckOEAELastConsistency( __in const CSPExtentInfo& speiLastOE, __in const CSPExtentInfo& speiLastAE )
{
    ERR err = JET_errSuccess;

    Call( speiLastOE.ErrCheckCorrupted() );
    Call( speiLastAE.ErrCheckCorrupted() );

    if ( speiLastAE.PgnoLast() > speiLastOE.PgnoLast() )
    {
        AssertTrack( fFalse, "ShrinkConsistencyAePgnoLastBeyondOePgnoLast" );
        Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
    }

    if ( !speiLastOE.FContains( speiLastAE.PgnoFirst() ) && speiLastOE.FContains( speiLastAE.PgnoLast() ) )
    {
        AssertTrack( fFalse, "ShrinkConsistencyAeCrossesOeBoundary" );
        Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
    }

HandleError:
    return err;
}

ERR ErrSPShrinkTruncateLastExtent(
    _In_ PIB* ppib,
    _In_ const IFMP ifmp,
    _In_ CPRINTF* const pcprintfShrinkTraceRaw,
    _Inout_ HRT* const pdhrtExtMaint,
    _Inout_ HRT* const pdhrtFileTruncation,
    _Out_ PGNO* const ppgnoFirstFromLastExtentTruncated,
    _Out_ ShrinkDoneReason* const psdr )
{
    ERR err = JET_errSuccess;
    BOOL fInTransaction = fFalse;
    CSPExtentInfo speiLastOE, speiAE;
    CSPExtentInfo speiLastAfterOE, speiLastAfterAE;
    FMP* const pfmp = g_rgfmp + ifmp;
    FUCB* pfucbRoot = pfucbNil;
    FUCB* pfucbOE = pfucbNil;
    FUCB* pfucbAE = pfucbNil;
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope( );
    tcScope->iorReason.SetIort( iortDbShrink );
    tcScope->SetDwEngineObjid( objidSystemRoot );
    BOOL fExtMaint = fFalse;
    BOOL fFileTruncation = fFalse;
    HRT hrtExtMaintStart = 0;
    HRT hrtFileTruncationStart = 0;

    Assert( pfmp->FShrinkIsRunning() );
    Assert( pfmp->FExclusiveBySession( ppib ) );

    *psdr = sdrNone;
    *ppgnoFirstFromLastExtentTruncated = pgnoNull;

    Call( ErrDIRBeginTransaction( ppib, 48584, NO_GRBIT ) );
    fInTransaction = fTrue;

    Call( ErrBTIOpen( ppib, ifmp, pgnoSystemRoot, objidNil, openNormal, &pfucbRoot, fFalse ) );
    Call( ErrSPIOpenOwnExt( pfucbRoot->ppib, pfucbRoot->u.pfcb, &pfucbOE ) );
    Call( ErrSPIOpenAvailExt( pfucbRoot->ppib, pfucbRoot->u.pfcb, &pfucbAE ) );

    Call( ErrSPISeekRootOELast( pfucbOE, &speiLastOE ) );
    *ppgnoFirstFromLastExtentTruncated = speiLastOE.PgnoFirst();

    if ( speiLastOE.PgnoFirst() == pgnoSystemRoot )
    {
        *psdr = sdrNoLowAvailSpace;
        goto HandleError;
    }

    pfmp->SetPgnoShrinkTarget( speiLastOE.PgnoFirst() - 1 );

    Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast() + 1, spp::AvailExtLegacyGeneralPool, &speiAE ) );
    if ( speiAE.FIsSet() )
    {
        AssertTrack( fFalse, "ShrinkExtAeBeyondOe" );
        Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
    }

    if ( ( speiLastOE.PgnoLast() + (PGNO)cpgDBReserved ) <= (PGNO)pfmp->CpgShrinkDatabaseSizeLimit() )
    {
        *psdr = sdrReachedSizeLimit;
        goto HandleError;
    }

    Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast(), spp::AvailExtLegacyGeneralPool, &speiAE ) );
    if ( !speiAE.FIsSet() )
    {
        Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast(), spp::ShelvedPool, &speiAE ) );
        if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() != speiLastOE.PgnoLast() ) )
        {
            Assert( !speiAE.FIsSet() || ( speiAE.PgnoLast() > speiLastOE.PgnoLast() ) );
            Error( ErrERRCheck( JET_wrnShrinkNotPossible ) );
        }
    }

    Call( ErrSPICheckOEAELastConsistency( speiLastOE, speiAE ) );

    fExtMaint = fTrue;
    hrtExtMaintStart = HrtHRTCount();

    PGNO pgnoLastAEExpected = speiLastOE.PgnoLast();
    while ( fTrue )
    {
        if ( speiAE.CpgExtent() == 0 )
        {
            FireWall( "ShrinkExtZeroedAe" );
            *psdr = sdrUnexpected;
            goto HandleError;
        }

        Assert( speiAE.PgnoLast() == pgnoLastAEExpected );

        if ( speiAE.PgnoFirst() < speiLastOE.PgnoFirst() )
        {
            AssertTrack( fFalse, "ShrinkExtAeCrossesOeBoundary" );
            Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }

        if ( speiAE.PgnoFirst() == speiLastOE.PgnoFirst() )
        {
            break;
        }

        pgnoLastAEExpected = speiAE.PgnoFirst() - 1;

        BTUp( pfucbAE );
        Call( ErrSPISeekRootAE( pfucbAE, pgnoLastAEExpected, spp::AvailExtLegacyGeneralPool, &speiAE ) );
        if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() != pgnoLastAEExpected ) )
        {
            BTUp( pfucbAE );
            Call( ErrSPISeekRootAE( pfucbAE, pgnoLastAEExpected, spp::ShelvedPool, &speiAE ) );
            if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() != pgnoLastAEExpected ) )
            {
                Error( ErrERRCheck( JET_wrnShrinkNotPossible ) );
            }
        }
    }

    if ( !pfmp->FShrinkIsActive() )
    {
        AssertTrack( fFalse, "ShrinkExtShrinkInactiveBeforeTruncation" );
        Error( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ) );
    }

    Call( PverFromPpib( ppib )->ErrVERRCEClean( ifmp ) );
    if ( err != JET_errSuccess )
    {
        AssertTrack( fFalse, OSFormat( "ShrinkExtVerCleanWrn:%d", err ) );
        *psdr = sdrUnexpected;
        goto HandleError;
    }
    pfmp->WaitForTasksToComplete();

    PGNO pgnoLastFileSystem = pgnoNull;
    Call( pfmp->ErrPgnoLastFileSystem( &pgnoLastFileSystem ) );

    const CPG cpgShrunk = pgnoLastFileSystem - ( speiLastOE.PgnoFirst() - 1 );
    Assert( cpgShrunk > 0 );

    BTUp( pfucbAE );
    Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast(), spp::AvailExtLegacyGeneralPool, &speiAE ) );
    if ( !speiAE.FIsSet() )
    {
        Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast(), spp::ShelvedPool, &speiAE ) );
        if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() != speiLastOE.PgnoLast() ) )
        {
            AssertTrack( fFalse, "ShrinkExtNoLongerAvailableLast" );
            Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }
    }

    Call( ErrSPICheckOEAELastConsistency( speiLastOE, speiAE ) );

    ULONG cAeExtDeleted = 0, cAeExtShelved = 0;
    pgnoLastAEExpected = speiLastOE.PgnoLast();
    while ( fTrue )
    {
        if ( speiAE.CpgExtent() == 0 )
        {
            AssertTrack( fFalse, "ShrinkExtZeroedAeNew" );
            Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }

        Assert( speiAE.PgnoLast() == pgnoLastAEExpected );

        if ( speiAE.PgnoFirst() < speiLastOE.PgnoFirst() )
        {
            AssertTrack( fFalse, "ShrinkExtAeCrossesOeBoundaryNew" );
            Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }

        if ( speiAE.SppPool() == spp::AvailExtLegacyGeneralPool )
        {
            if ( !pfmp->FShrinkIsActive() )
            {
                AssertTrack( fFalse, "ShrinkExtShrinkInactiveBeforeAeDeletion" );
                Error( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ) );
            }

            Expected( !pfucbAE->u.pfcb->FDontLogSpaceOps() );
            Call( ErrBTFlagDelete(
                    pfucbAE,
                    fDIRNoVersion | ( pfucbAE->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) ) );
            Assert( latchWrite == Pcsr( pfucbAE )->Latch() );
            Pcsr( pfucbAE )->Downgrade( latchReadTouch );
            cAeExtDeleted++;

            Call( ErrFaultInjection( 49736 ) );
        }
        else
        {
            Assert( speiAE.SppPool() == spp::ShelvedPool );
            Call( ErrSPCaptureSnapshot( pfucbRoot, speiAE.PgnoFirst(), speiAE.CpgExtent() ) );
            cAeExtShelved++;
        }

        if ( speiAE.PgnoFirst() == speiLastOE.PgnoFirst() )
        {
            err = JET_errSuccess;
            break;
        }

        pgnoLastAEExpected = speiAE.PgnoFirst() - 1;

        BTUp( pfucbAE );
        Call( ErrSPISeekRootAE( pfucbAE, pgnoLastAEExpected, spp::AvailExtLegacyGeneralPool, &speiAE ) );
        if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() != pgnoLastAEExpected ) )
        {
            BTUp( pfucbAE );
            Call( ErrSPISeekRootAE( pfucbAE, pgnoLastAEExpected, spp::ShelvedPool, &speiAE ) );
            if ( !speiAE.FIsSet() || ( speiAE.PgnoLast() != pgnoLastAEExpected ) )
            {
                AssertTrack( fFalse, "ShrinkExtNoLongerAvailableNew" );
                Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
            }
        }
    }

    BTUp( pfucbAE );
    BTUp( pfucbOE );

    Call( ErrSPISeekRootOELast( pfucbOE, &speiLastAfterOE ) );
    if ( speiLastAfterOE.PgnoLast() != speiLastOE.PgnoLast() )
    {
        Assert( speiLastAfterOE.PgnoLast() > speiLastOE.PgnoLast() );
        FireWall( "ShrinkNewSpaceOwnedWhileShrinking" );
        *psdr = sdrUnexpected;
        goto HandleError;
    }

    Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast(), spp::AvailExtLegacyGeneralPool, &speiLastAfterAE ) );
    BTUp( pfucbAE );
    if ( speiLastAfterAE.FIsSet() )
    {
        Call( ErrSPICheckOEAELastConsistency( speiLastAfterOE, speiLastAfterAE ) );
        if ( speiLastAfterAE.PgnoLast() >= speiLastOE.PgnoFirst() )
        {
            FireWall( "ShrinkNewSpaceAvailWhileShrinking" );
            *psdr = sdrUnexpected;
            goto HandleError;
        }
    }

    if ( ( cAeExtDeleted > 0 ) && ( cAeExtShelved > 0 ) )
    {
        Call( ErrFaultInjection( 50114 ) );
    }

    {
    const QWORD cbOwnedFileSizeBefore = pfmp->CbOwnedFileSize();
    pfmp->SetOwnedFileSize( CbFileSizeOfPgnoLast( speiLastAfterOE.PgnoFirst() - 1 ) );

    if ( !pfmp->FShrinkIsActive() )
    {
        AssertTrack( fFalse, "ShrinkExtShrinkInactiveBeforeOeDeletion" );
        Error( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ) );
    }

    Expected( !pfucbOE->u.pfcb->FDontLogSpaceOps() );
    err = ErrBTFlagDelete(
            pfucbOE,
            fDIRNoVersion | ( pfucbOE->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ) );
    if ( err < JET_errSuccess )
    {
        pfmp->SetOwnedFileSize( cbOwnedFileSizeBefore );
    }
    Call( err );
    Assert( latchWrite == Pcsr( pfucbOE )->Latch() );
    BTUp( pfucbOE );
    (*pcprintfShrinkTraceRaw)( "ShrinkTruncate[%I32u:%I32u]\r\n", speiLastAfterOE.PgnoFirst(), speiLastAfterOE.PgnoLast() );

    if ( !pfmp->FShrinkIsActive() )
    {
        AssertTrack( fFalse, "ShrinkExtShrinkInactiveAfterTruncation" );
        Error( ErrERRCheck( errSPNoSpaceBelowShrinkTarget ) );
    }
    }

    Call( ErrSPISeekRootOELast( pfucbOE, &speiLastAfterOE ) );
    BTUp( pfucbOE );
    Assert( speiLastAfterOE.PgnoLast() == pfmp->PgnoLast() );

    Call( ErrSPISeekRootAE( pfucbAE, speiLastOE.PgnoLast(), spp::AvailExtLegacyGeneralPool, &speiLastAfterAE ) );
    BTUp( pfucbAE );

    if ( speiLastAfterOE.PgnoLast() >= speiLastOE.PgnoFirst() )
    {
        AssertTrack( fFalse, "ShrinkNewSpaceOwnedPostOeDeleteWhileShrinking" );
        Error( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
    }
    if ( speiLastAfterAE.FIsSet() )
    {
        Call( ErrSPICheckOEAELastConsistency( speiLastAfterOE, speiLastAfterAE ) );

        if ( speiLastAfterAE.PgnoLast() >= speiLastOE.PgnoFirst() )
        {
            AssertTrack( fFalse, "ShrinkNewSpaceAvailPostOeDeleteWhileShrinking" );
            Error( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
        }
    }

    if ( speiLastAfterOE.PgnoLast() > pgnoLastFileSystem )
    {
        AssertTrack( fFalse, "ShrinkExtOePgnoLastBeyondFsPgnoLast" );
        Error( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
    }

    BTClose( pfucbAE );
    pfucbAE = pfucbNil;
    BTClose( pfucbOE );
    pfucbOE = pfucbNil;
    BTClose( pfucbRoot );
    pfucbRoot = pfucbNil;

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTransaction = fFalse;

    *pdhrtExtMaint += DhrtHRTElapsedFromHrtStart( hrtExtMaintStart );
    fExtMaint = fFalse;

    fFileTruncation = fTrue;
    hrtFileTruncationStart = HrtHRTCount();

    {
    PIBTraceContextScope tcScopeT = ppib->InitTraceContextScope();
    tcScopeT->iorReason.SetIorp( iorpDatabaseShrink );

    Call( ErrFaultInjection( 40200 ) );

    Call( ErrSPINewSize( TcCurr(), ifmp, pgnoLastFileSystem, -1 * cpgShrunk, 0 ) );
    }

    pfmp->ResetPgnoMaxTracking( speiLastAfterOE.PgnoLast() );

    *pdhrtFileTruncation += DhrtHRTElapsedFromHrtStart( hrtFileTruncationStart );
    fFileTruncation = fFalse;

HandleError:
    Assert( !( fExtMaint && fFileTruncation ) );

    if ( fExtMaint )
    {
        *pdhrtExtMaint += DhrtHRTElapsedFromHrtStart( hrtExtMaintStart );
        fExtMaint = fFalse;
    }

    if ( fFileTruncation )
    {
        *pdhrtFileTruncation += DhrtHRTElapsedFromHrtStart( hrtFileTruncationStart );
        fFileTruncation = fFalse;
    }

    if ( pfucbAE != pfucbNil )
    {
        BTUp( pfucbAE );
        BTClose( pfucbAE );
        pfucbAE = pfucbNil;
    }

    if ( pfucbOE != pfucbNil )
    {
        BTUp( pfucbOE );
        BTClose( pfucbOE );
        pfucbOE = pfucbNil;
    }

    if ( pfucbRoot != pfucbNil )
    {
        BTClose( pfucbRoot );
        pfucbRoot = pfucbNil;
    }

    if ( fInTransaction )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
        fInTransaction = fFalse;
    }

    return err;
}

LOCAL CPG CpgSPICpgPrefFromCpgRequired( const CPG cpgRequired, const CPG cpgMinForSplit )
{
    CPG cpgPref = 0;

    if ( cpgMinForSplit <= cpgMaxRootPageSplit )
    {
        cpgPref = cpgPrefRootPageSplit;
    }
    else if ( cpgMinForSplit <= cpgMaxParentOfLeafRootSplit )
    {
        cpgPref = cpgPrefParentOfLeafRootSplit;
    }
    else
    {
        cpgPref = cpgPrefSpaceTreeSplit;
    }

    if ( cpgRequired > cpgPref )
    {
        cpgPref = LNextPowerOf2( cpgRequired );
    }

    #ifndef ENABLE_JET_UNIT_TEST 
    Expected( ( cpgPref <= 16 ) || ( UlConfigOverrideInjection( 46030, 0 ) != 0 ) );
    #endif

    return cpgPref;
}

#ifdef ENABLE_JET_UNIT_TEST
JETUNITTEST( SPACE, CpgSPICpgPrefFromCpgRequired )
{
    CHECK( 2 == CpgSPICpgPrefFromCpgRequired( 0, 0 ) );
    CHECK( 2 == CpgSPICpgPrefFromCpgRequired( 2, 0 ) );
    CHECK( 4 == CpgSPICpgPrefFromCpgRequired( 3, 0 ) );
    CHECK( 4 == CpgSPICpgPrefFromCpgRequired( 4, 0 ) );
    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 5, 0 ) );
    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 8, 0 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 9, 0 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 16, 0 ) );
    CHECK( 32 == CpgSPICpgPrefFromCpgRequired( 17, 0 ) );

    CHECK( 2 == CpgSPICpgPrefFromCpgRequired( 2, 2 ) );
    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 3, 3 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 4, 4 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 5, 5 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 16, 16 ) );
    CHECK( 32 == CpgSPICpgPrefFromCpgRequired( 17, 17 ) );

    CHECK( 4 == CpgSPICpgPrefFromCpgRequired( 3, 2 ) );
    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 4, 3 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 5, 4 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 6, 5 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 16, 15 ) );
    CHECK( 32 == CpgSPICpgPrefFromCpgRequired( 17, 16 ) );

    CHECK( 4 == CpgSPICpgPrefFromCpgRequired( 4, 2 ) );
    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 6, 3 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 8, 4 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 10, 5 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 16, 8 ) );
    CHECK( 32 == CpgSPICpgPrefFromCpgRequired( 18, 9 ) );

    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 5, 2 ) );
    CHECK( 8 == CpgSPICpgPrefFromCpgRequired( 7, 3 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 9, 4 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 11, 5 ) );
    CHECK( 16 == CpgSPICpgPrefFromCpgRequired( 15, 7 ) );
    CHECK( 32 == CpgSPICpgPrefFromCpgRequired( 17, 8 ) );
}
#endif

LOCAL ERR ErrSPIReserveSPBufPagesForSpaceTree(
    FUCB* const pfucb,
    FUCB* const pfucbSpace,
    FUCB* const pfucbParent,
    CArray<EXTENTINFO>* const parreiReleased = NULL,
    const CPG cpgAddlReserve = 0,
    const PGNO pgnoReplace = pgnoNull )
{
    ERR             err  = JET_errSuccess;
    ERR             wrn  = JET_errSuccess;
    SPLIT_BUFFER    *pspbuf = NULL;
    CPG             cpgMinForSplit = 0;
    FMP*            pfmp = g_rgfmp + pfucb->ifmp;

    OnDebug( ULONG crepeat = 0 );
    Assert( pfmp != NULL );
    Assert( ( pfucb != pfucbNil ) && !FFUCBSpace( pfucb ) );
    Assert( ( pfucbSpace != pfucbNil ) && FFUCBSpace( pfucbSpace ) );
    Assert( ( pfucbParent == pfucbNil ) || !FFUCBSpace( pfucbParent ) );
    Assert( !Pcsr( pfucbSpace )->FLatched() );
    Assert( pfucb->u.pfcb == pfucbSpace->u.pfcb );
    Assert( pfucb->u.pfcb->FSpaceInitialized() );
    Assert( !FSPIIsSmall( pfucb->u.pfcb ) );
    Assert( ( pgnoReplace == pgnoNull ) || pfmp->FShrinkIsRunning() );
    const BOOL fUpdatingDbRoot = ( pfucbParent == pfucbNil );
    const BOOL fMayViolateMaxSize = ( pgnoReplace == pgnoNull );
    const BOOL fAvailExt = FFUCBAvailExt( pfucbSpace );
    const BOOL fOwnExt = FFUCBOwnExt( pfucbSpace );
    Assert( !!fAvailExt ^ !!fOwnExt );

    rtlconst BOOL fForceRefillDbgOnly = fFalse;
    OnDebug( fForceRefillDbgOnly = ( ErrFaultInjection( 47594 ) < JET_errSuccess ) && !g_fRepair );

    rtlconst BOOL fForcePostAddToOwnExtDbgOnly = fFalse;
    rtlconst ERR errFaultAddToOe = JET_errSuccess;
#ifdef DEBUG
    errFaultAddToOe = fUpdatingDbRoot ? ErrFaultInjection( 60394 ) : ErrFaultInjection( 35818 );
    OnDebug( fForcePostAddToOwnExtDbgOnly = ( ( errFaultAddToOe == JET_errSuccess ) && ( rand() % 4 ) == 0 ) );
#endif
    forever
    {
        BOOL fSingleAndAvailableEnough = fFalse;
        SPLIT_BUFFER spbufBefore;

        Assert( crepeat < 3 );
        OnDebug( crepeat++ );

        AssertSPIPfucbOnRoot( pfucb );
        AssertSPIPfucbOnRootOrNull( pfucbParent );

        Call( ErrBTIGotoRoot( pfucbSpace, latchRIW ) );
        AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );
        Call( ErrSPIGetSPBuf( pfucbSpace, &pspbuf ) );
        UtilMemCpy( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) );

        if ( pfucbSpace->csr.Cpage().FLeafPage() )
        {
            fSingleAndAvailableEnough = pfucbSpace->csr.Cpage().CbPageFree() >= 80;

            Expected( fSingleAndAvailableEnough || ( ( pspbuf->CpgBuffer1() + pspbuf->CpgBuffer2() ) >= cpgMinForSplit ) );

            if ( pfucbSpace->csr.Cpage().CbPageFree() < 100 )
            {
                cpgMinForSplit = cpgMaxRootPageSplit;
            }
            else
            {
                cpgMinForSplit = 0;
            }
        }
        else if ( pfucbSpace->csr.Cpage().FParentOfLeaf() )
        {
            cpgMinForSplit = cpgMaxParentOfLeafRootSplit;
        }
        else
        {
            cpgMinForSplit = cpgMaxSpaceTreeSplit;
        }

        CPG cpgRequired;
        CPG cpgRequest;
        CPG cpgAvailable;
        CPG cpgNewSpace;
        BYTE ispbufReplace;
        BOOL fSpBuffersAvailableEnough;
        BOOL fSpBufRefilled;
        BOOL fAlreadyOwn;
        BOOL fAddedToOwnExt;
        PGNO pgnoLast;

        const ERR wrnT = wrn;

        forever
        {
            wrn = wrnT;
            cpgRequired = 0;
            cpgRequest = 0;
            cpgAvailable = 0;
            cpgNewSpace = 0;
            ispbufReplace = 0;
            fSpBuffersAvailableEnough = fFalse;
            fSpBufRefilled = fFalse;
            fAlreadyOwn = fFalse;
            fAddedToOwnExt = fFalse;
            pgnoLast = pgnoNull;

            cpgRequired = fOwnExt ? ( 2 * cpgMinForSplit ) : cpgMinForSplit;
            cpgRequired += ( cpgAddlReserve + (CPG)UlConfigOverrideInjection( 46030, 0 ) );

            OnDebug( fForceRefillDbgOnly = fForceRefillDbgOnly && ( cpgRequired > 0 ) );

            if ( pspbuf->PgnoLastBuffer1() > pfmp->PgnoLast() )
            {
                ispbufReplace = 1;
            }
            else if ( pspbuf->PgnoLastBuffer2() > pfmp->PgnoLast() )
            {
                ispbufReplace = 2;
            }

            if ( ( pgnoReplace != pgnoNull ) && ( ispbufReplace == 0 ) )
            {
                if ( pspbuf->FBuffer1ContainsPgno( pgnoReplace ) )
                {
                    Assert( !pspbuf->FBuffer2ContainsPgno( pgnoReplace ) );
                    ispbufReplace = 1;
                }
                else if ( pspbuf->FBuffer2ContainsPgno( pgnoReplace ) )
                {
                    ispbufReplace = 2;
                }
            }

            if ( pfmp->FShrinkIsActive() && ( ispbufReplace == 0 ) )
            {
                if ( pfmp->FBeyondPgnoShrinkTarget( pspbuf->PgnoFirstBuffer1(), pspbuf->CpgBuffer1() ) )
                {
                    ispbufReplace = 1;
                }
                else if ( pfmp->FBeyondPgnoShrinkTarget( pspbuf->PgnoFirstBuffer2(), pspbuf->CpgBuffer2() ) )
                {
                    ispbufReplace = 2;
                }
            }

            const CPG cpgSpBuffer1Available = ( pfmp->FBeyondPgnoShrinkTarget( pspbuf->PgnoFirstBuffer1(), pspbuf->CpgBuffer1() ) ||
                                                 ( pspbuf->PgnoLastBuffer1() > pfmp->PgnoLast() ) ) ? 0 : pspbuf->CpgBuffer1();
            const CPG cpgSpBuffer2Available = ( pfmp->FBeyondPgnoShrinkTarget( pspbuf->PgnoFirstBuffer2(), pspbuf->CpgBuffer2() ) ||
                                                 ( pspbuf->PgnoLastBuffer2() > pfmp->PgnoLast() ) ) ? 0 : pspbuf->CpgBuffer2();

            fSpBuffersAvailableEnough = ( ( cpgSpBuffer1Available + cpgSpBuffer2Available ) >= cpgMinForSplit );

            if ( ispbufReplace != 0 )
            {
                OnDebug( const PGNO pgnoLastSpBuf = ( ispbufReplace == 1 ) ? pspbuf->PgnoLastBuffer1() : pspbuf->PgnoLastBuffer2() );
                OnDebug( const PGNO cpgExt = ( ispbufReplace == 1 ) ? pspbuf->CpgBuffer1() : pspbuf->CpgBuffer2() );

                Expected( pfmp->FShrinkIsRunning() || ( pgnoLastSpBuf > pfmp->PgnoLast() ) );

                wrn = ErrERRCheck( wrnSPRequestSpBufRefill );

                const CPG cpgNewSpBuf = cpgRequired - ( ( ispbufReplace == 1 ) ? cpgSpBuffer2Available : cpgSpBuffer1Available );
                cpgRequired = max( cpgNewSpBuf, cpgMaxRootPageSplit );
            }

            if ( ( ( cpgSpBuffer1Available + cpgSpBuffer2Available ) >= cpgRequired ) &&
                    ( ispbufReplace == 0 ) &&
                    !fForceRefillDbgOnly )
            {
                fSpBufRefilled = fTrue;
                break;
            }

            Assert( cpgRequired > 0 );
            cpgRequest = CpgSPICpgPrefFromCpgRequired( cpgRequired, cpgMinForSplit );


            if ( pfmp->FShrinkIsRunning() && fUpdatingDbRoot )
            {

                BTUp( pfucbSpace );
                pspbuf = NULL;

                PGNO pgnoFirst = pgnoNull;
                cpgNewSpace = cpgRequest;
                err = ErrSPIGetExt(
                            pfucb->u.pfcb,
                            pfucb,
                            &cpgNewSpace,
                            cpgRequired,
                            &pgnoFirst,
                            fSPSelfReserveExtent );
                AssertSPIPfucbOnRoot( pfucb );

                if ( err >= JET_errSuccess )
                {
                    cpgAvailable = cpgNewSpace;
                    pgnoLast = pgnoFirst + cpgNewSpace - 1;
                    fAlreadyOwn = fTrue;
                }
                else if ( err == errSPNoSpaceForYou )
                {
                    err = JET_errSuccess;
                }
                Call( err );

                Call( ErrBTIGotoRoot( pfucbSpace, latchRIW ) );
                AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );
                Call( ErrSPIGetSPBuf( pfucbSpace, &pspbuf ) );
                Assert( 0 == memcmp( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) ) );
                UtilMemCpy( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) );
            }

            Assert( !!fAlreadyOwn ^ !!( pgnoLast == pgnoNull ) );

            if ( fForcePostAddToOwnExtDbgOnly ||
                 fAlreadyOwn || !fOwnExt ||
                 !fUpdatingDbRoot ||
                 fSpBuffersAvailableEnough || fSingleAndAvailableEnough )
            {
                break;
            }

            AssertTrack( pfmp->FShrinkIsActive(), "SpBufUnexpectedShrinkInactive" );
            if ( !pfmp->FShrinkIsActive() )
            {
                break;
            }

            pfmp->ResetPgnoShrinkTarget();
            Assert( !pfmp->FShrinkIsActive() );
        }

        if ( fSpBufRefilled )
        {
            break;
        }

        if ( !fAlreadyOwn )
        {
            if ( !fUpdatingDbRoot )
            {
                PGNO pgnoFirst = pgnoNull;
                cpgNewSpace = cpgRequest;
                err = ErrSPIGetExt(
                            pfucb->u.pfcb,
                            pfucbParent,
                            &cpgNewSpace,
                            cpgRequired,
                            &pgnoFirst,
                            0,
                            0,
                            NULL,
                            fMayViolateMaxSize );
                AssertSPIPfucbOnRoot( pfucb );
                AssertSPIPfucbOnRoot( pfucbParent );
                AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );
                Call( err );
                cpgAvailable = cpgNewSpace;

                pgnoLast = pgnoFirst + cpgNewSpace - 1;
            }
            else
            {
                Assert( pgnoSystemRoot == pfucb->u.pfcb->PgnoFDP() );

                cpgRequired = max( cpgRequired, cpgMaxSpaceTreeSplit );
                cpgRequest = CpgSPICpgPrefFromCpgRequired( cpgRequired, cpgMinForSplit );


                AssertTrack( NULL == pfucbSpace->u.pfcb->Psplitbuf( fAvailExt ), "NonNullRootAvailSplitBuff" );
                BTUp( pfucbSpace );
                pspbuf = NULL;

                cpgNewSpace = cpgRequest;
                Call( ErrSPIExtendDB(
                        pfucb,
                        cpgRequired,
                        &cpgNewSpace,
                        &pgnoLast,
                        fFalse,
                        fMayViolateMaxSize,
                        parreiReleased,
                        &cpgAvailable ) );

                Call( ErrBTIGotoRoot( pfucbSpace, latchRIW ) );
                AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );
                Call( ErrSPIGetSPBuf( pfucbSpace, &pspbuf ) );
                Assert( 0 == memcmp( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) ) );
                UtilMemCpy( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) );
            }
        }

        AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );
        Assert( cpgRequest >= cpgRequired );
        Assert( cpgAvailable >= cpgRequired );
        Assert( cpgNewSpace >= cpgAvailable );

        if ( !fAlreadyOwn &&
             ( !fOwnExt ||
               ( !fForcePostAddToOwnExtDbgOnly &&
                 ( fSpBuffersAvailableEnough || fSingleAndAvailableEnough ) ) ) )
        {
            BTUp( pfucbSpace );
            pspbuf = NULL;

            Call( errFaultAddToOe );
            Call( ErrSPIAddToOwnExt( pfucbSpace, pgnoLast, cpgNewSpace, NULL ) );
            fAddedToOwnExt = fTrue;

            if ( fUpdatingDbRoot )
            {
                pfmp->SetOwnedFileSize( CbFileSizeOfPgnoLast( pgnoLast ) );
            }

            wrn = ErrERRCheck( wrnSPRequestSpBufRefill );

            Call( ErrBTIGotoRoot( pfucbSpace, latchRIW ) );
            AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );
            Call( ErrSPIGetSPBuf( pfucbSpace, &pspbuf ) );
            Assert( fOwnExt || ( 0 == memcmp( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) ) ) );
            UtilMemCpy( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) );
        }

        AssertSPIPfucbOnSpaceTreeRoot( pfucbSpace, Pcsr( pfucbSpace ) );

        BYTE ispbuf = 0;
        if ( NULL == pfucbSpace->u.pfcb->Psplitbuf( fAvailExt ) )
        {

            pfucbSpace->csr.UpgradeFromRIWLatch();

            Assert( 0 == memcmp( &spbufBefore,
                                    PspbufSPISpaceTreeRootPage( pfucbSpace, Pcsr( pfucbSpace ) ),
                                    sizeof(SPLIT_BUFFER) ) );
            Assert( sizeof( SPLIT_BUFFER ) == pfucbSpace->kdfCurr.data.Cb() );
            UtilMemCpy( &spbufBefore, PspbufSPISpaceTreeRootPage( pfucbSpace, Pcsr( pfucbSpace ) ), sizeof(SPLIT_BUFFER) );

            SPLIT_BUFFER spbuf;
            DATA data;
            UtilMemCpy( &spbuf, &spbufBefore, sizeof(SPLIT_BUFFER) );
            ispbuf = spbuf.AddPages( pgnoLast, cpgAvailable, ispbufReplace );
            Assert( ( ispbufReplace == 0 ) || ( ispbuf == ispbufReplace ) );
            data.SetPv( &spbuf );
            data.SetCb( sizeof(spbuf) );
            Call( ErrNDSetExternalHeader(
                        pfucbSpace,
                        &data,
                        ( pfucbSpace->u.pfcb->FDontLogSpaceOps() ? fDIRNoLog : fDIRNull ),
                        noderfWhole ) );
        }
        else
        {

            Assert( pspbuf == pfucbSpace->u.pfcb->Psplitbuf( fAvailExt ) );
            Assert( 0 == memcmp( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) ) );
            UtilMemCpy( &spbufBefore, pspbuf, sizeof(SPLIT_BUFFER) );

            ispbuf = pspbuf->AddPages( pgnoLast, cpgAvailable, ispbufReplace );
            Assert( ( ispbufReplace == 0 ) || ( ispbuf == ispbufReplace ) );
            SPITraceSplitBufferMsg( pfucbSpace, "Added pages to" );
        }

        BTUp( pfucbSpace );
        pspbuf = NULL;

        EXTENTINFO extinfoReleased;

        Assert( ( ispbuf == 1 ) || ( ispbuf == 2 ) );
        extinfoReleased.pgnoLastInExtent = ( ispbuf == 1 ) ? spbufBefore.PgnoLastBuffer1() : spbufBefore.PgnoLastBuffer2();
        extinfoReleased.cpgExtent = ( ispbuf == 1 ) ? spbufBefore.CpgBuffer1() : spbufBefore.CpgBuffer2();
        Assert( extinfoReleased.FValid() );

        if ( extinfoReleased.CpgExtent() > 0 )
        {
            if ( parreiReleased != NULL )
            {
                Call( ( parreiReleased->ErrSetEntry( parreiReleased->Size(), extinfoReleased ) == CArray<EXTENTINFO>::ERR::errSuccess ) ?
                                                                                                  JET_errSuccess :
                                                                                                  ErrERRCheck( JET_errOutOfMemory ) );
            }
            else
            {
                FireWall( "SpBufDroppedReleased" );
            }
        }

        Assert( !fAddedToOwnExt || ( wrn == wrnSPRequestSpBufRefill ) );

        if ( !fAlreadyOwn && !fAddedToOwnExt )
        {
            Assert( fOwnExt );

            AssertTrack( fForcePostAddToOwnExtDbgOnly || ( !fSpBuffersAvailableEnough && !fUpdatingDbRoot ), "SpBufPostAddToOe" );

            if ( fUpdatingDbRoot )
            {
                pfmp->SetOwnedFileSize( CbFileSizeOfPgnoLast( pgnoLast ) );
            }

            Call( ErrSPIAddToOwnExt( pfucb, pgnoLast, cpgNewSpace, NULL ) );
            fAddedToOwnExt = fTrue;

            wrn = ErrERRCheck( wrnSPRequestSpBufRefill );
        }

        Assert( !fAddedToOwnExt || ( wrn == wrnSPRequestSpBufRefill ) );
        if ( !fOwnExt || !fAddedToOwnExt )
        {
            break;
        }

        OnDebug( fForceRefillDbgOnly = fFalse );
    }

    CallS( err );
    err = wrn;

HandleError:
    BTUp( pfucbSpace );
    pspbuf = NULL;

    AssertSPIPfucbOnRoot( pfucb );
    AssertSPIPfucbOnRootOrNull( pfucbParent );

    return err;
}

ERR ErrSPReserveSPBufPagesForSpaceTree( FUCB *pfucb, FUCB *pfucbSpace, FUCB *pfucbParent )
{
    ERR err = JET_errSuccess;
    PIBTraceContextScope tcScope = pfucbSpace->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucbSpace );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucbSpace ) );
    tcScope->iorReason.SetIort( iortSpace );

    Expected( g_fRepair );
    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( pcsrNil == pfucb->pcsrRoot );
    Assert( !Pcsr( pfucbParent )->FLatched() );
    Assert( pcsrNil == pfucbParent->pcsrRoot );

    Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
    pfucb->pcsrRoot = Pcsr( pfucb );

    Call( ErrBTIGotoRoot( pfucbParent, latchRIW ) );
    pfucbParent->pcsrRoot = Pcsr( pfucbParent );

    Call( ErrSPIReserveSPBufPagesForSpaceTree( pfucb, pfucbSpace, pfucbParent ) );

HandleError:
    BTUp( pfucbParent );
    pfucbParent->pcsrRoot = pcsrNil;

    BTUp( pfucb );
    pfucb->pcsrRoot = pcsrNil;

    return err;
}

ERR ErrSPReserveSPBufPages(
    FUCB* const pfucb,
    const BOOL fAddlReserveForPageMoveOE,
    const BOOL fAddlReserveForPageMoveAE )
{
    ERR err = JET_errSuccess;
    FUCB* pfucbOwningTree = pfucbNil;

    Call( ErrBTIOpenAndGotoRoot(
            pfucb->ppib,
            pfucb->u.pfcb->PgnoFDP(),
            pfucb->ifmp,
            &pfucbOwningTree ) );
    Assert( pfucbOwningTree->u.pfcb == pfucb->u.pfcb );

    Call( ErrSPIReserveSPBufPages(
            pfucbOwningTree,
            pfucbNil,
            fAddlReserveForPageMoveOE ? 1 : 0,
            fAddlReserveForPageMoveAE ? 1 : 0 ) );

HandleError:
    if ( pfucbOwningTree != pfucbNil )
    {
        pfucbOwningTree->pcsrRoot->ReleasePage();
        pfucbOwningTree->pcsrRoot = pcsrNil;
        BTClose( pfucbOwningTree );
        pfucbOwningTree = pfucbNil;
    }

    return err;
}

ERR ErrSPReplaceSPBuf(
    FUCB* const pfucb,
    FUCB* const pfucbParent,
    const PGNO pgnoReplace )
{
    ERR err = JET_errSuccess;

    Assert( pfucb != pfucbNil );

    Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
    pfucb->pcsrRoot = Pcsr( pfucb );

    if ( pfucbParent != pfucbNil )
    {
        Call( ErrBTIGotoRoot( pfucbParent, latchRIW ) );
        pfucbParent->pcsrRoot = Pcsr( pfucbParent );
    }

    Call( ErrSPIReserveSPBufPages(
            pfucb,
            pfucbParent,
            0,
            0,
            pgnoReplace ) );

HandleError:
    if ( ( pfucbParent != pfucbNil ) && ( pfucbParent->pcsrRoot != pcsrNil ) )
    {
        pfucbParent->pcsrRoot->ReleasePage();
        pfucbParent->pcsrRoot = pcsrNil;
    }

    if ( pfucb->pcsrRoot != pcsrNil )
    {
        pfucb->pcsrRoot->ReleasePage();
        pfucb->pcsrRoot = pcsrNil;
    }

    return err;
}

LOCAL ERR ErrSPIReserveSPBufPages(
    FUCB* const pfucb,
    FUCB* const pfucbParent,
    const CPG cpgAddlReserveOE,
    const CPG cpgAddlReserveAE,
    const PGNO pgnoReplace )
{
    ERR err = JET_errSuccess;
    FCB* const pfcb = pfucb->u.pfcb;
    FUCB* pfucbParentLocal = pfucbParent;
    FUCB* pfucbOE = pfucbNil;
    FUCB* pfucbAE = pfucbNil;
    BOOL fNeedRefill = fTrue;
    CArray<EXTENTINFO> arreiReleased( 10 );

    const PGNO pgnoParentFDP = PsphSPIRootPage( pfucb )->PgnoParent();
    const PGNO pgnoLastBefore = g_rgfmp[ pfucb->ifmp ].PgnoLast();

    Assert( ( pgnoParentFDP != pgnoNull ) || ( pfucbParent == pfucbNil ) );
    if ( ( pfucbParentLocal == pfucbNil ) && ( pgnoParentFDP != pgnoNull ) )
    {
        Call( ErrBTIOpenAndGotoRoot( pfucb->ppib, pgnoParentFDP, pfucb->ifmp, &pfucbParentLocal ) );
    }

    Call( ErrSPIOpenOwnExt( pfucb->ppib, pfcb, &pfucbOE ) );
    Call( ErrSPIOpenAvailExt( pfucb->ppib, pfcb, &pfucbAE ) );

    while ( fNeedRefill )
    {
        fNeedRefill = fFalse;

        Call( ErrSPIReserveSPBufPagesForSpaceTree(
                pfucb,
                pfucbOE,
                pfucbParentLocal,
                &arreiReleased,
                cpgAddlReserveOE,
                pgnoReplace ) );
        fNeedRefill = ( err == wrnSPRequestSpBufRefill );

        Call( ErrSPIReserveSPBufPagesForSpaceTree(
                pfucb,
                pfucbAE,
                pfucbParentLocal,
                &arreiReleased,
                cpgAddlReserveAE,
                pgnoReplace ) );
        fNeedRefill = fNeedRefill || ( err == wrnSPRequestSpBufRefill );

        if ( arreiReleased.Size() > 0 )
        {
            const EXTENTINFO& extinfoReleased = arreiReleased[ arreiReleased.Size() - 1 ];
            Assert( extinfoReleased.FValid() && ( extinfoReleased.CpgExtent() > 0 ) );
            Call( ErrSPIAEFreeExt( pfucb, extinfoReleased.PgnoFirst(), extinfoReleased.CpgExtent(), pfucbParentLocal ) );
            CallS( ( arreiReleased.ErrSetSize( arreiReleased.Size() - 1 ) == CArray<EXTENTINFO>::ERR::errSuccess ) ?
                                                                             JET_errSuccess :
                                                                             ErrERRCheck( JET_errOutOfMemory ) );
            fNeedRefill = fTrue;
        }
    }

    const PGNO pgnoLastAfter = g_rgfmp[ pfucb->ifmp ].PgnoLast();

    if ( !g_rgfmp[ pfucb->ifmp ].FIsTempDB() && ( pfcb->PgnoFDP() == pgnoSystemRoot ) && ( pgnoLastAfter > pgnoLastBefore ) )
    {
        Call( ErrSPIUnshelvePagesInRange( pfucb, pgnoLastBefore + 1, pgnoLastAfter ) );
        Call( ErrSPIReserveSPBufPages( pfucb, pfucbParentLocal, cpgAddlReserveOE, cpgAddlReserveAE, pgnoReplace ) );
    }

HandleError:
    if ( pfucbNil != pfucbOE )
    {
        BTClose( pfucbOE );
    }

    if ( pfucbNil != pfucbAE )
    {
        BTClose( pfucbAE );
    }

    if ( ( pfucbParentLocal != pfucbNil ) && ( pfucbParentLocal != pfucbParent ) )
    {
        Expected( pfucbParent == pfucbNil );
        AssertSPIPfucbOnRoot( pfucbParentLocal );
        pfucbParentLocal->pcsrRoot->ReleasePage();
        pfucbParentLocal->pcsrRoot = pcsrNil;
        BTClose( pfucbParentLocal );
        pfucbParentLocal = pfucbNil;
    }

    Assert( ( err < JET_errSuccess ) || ( arreiReleased.Size() == 0 ) );
    for ( size_t iext = 0; iext < arreiReleased.Size(); iext++ )
    {
        const EXTENTINFO& extinfoLeaked = arreiReleased[ iext ];
        SPIReportSpaceLeak( pfucb, err, extinfoLeaked.PgnoFirst(), (CPG)extinfoLeaked.CpgExtent(), "SpBuffer" );
    }

    return err;
}




LOCAL ERR ErrSPIAllocatePagesSlowlyIfSparse(
    _In_ IFMP ifmp,
    _In_ const PGNO pgnoFirstImportant,
    _In_ const PGNO pgnoLastImportant,
    _In_ const CPG cpgReq,
    _In_ const TraceContext& tc )
{
    ERR err = JET_errSuccess;

    Expected( cpgReq == (CPG)( pgnoLastImportant - pgnoFirstImportant + 1 ) );
    Enforce( pgnoFirstImportant <= pgnoLastImportant );

    FMP* const pfmp = &g_rgfmp[ ifmp ];

    const QWORD ibFirstRangeImportant = OffsetOfPgno( pgnoFirstImportant );
    const QWORD ibLastRangeImportant = OffsetOfPgno( pgnoLastImportant + 1 ) - 1;

    CArray<SparseFileSegment> arrsparseseg;

    Enforce( ibFirstRangeImportant >= OffsetOfPgno( pgnoSystemRoot + 3 ) );

    if ( !pfmp->FTrimSupported() )
    {
        goto HandleError;
    }

    Call( ErrIORetrieveSparseSegmentsInRegion(  pfmp->Pfapi(),
                                                ibFirstRangeImportant,
                                                ibLastRangeImportant,
                                                &arrsparseseg  ) );

    for ( size_t isparseseg = 0; isparseseg < arrsparseseg.Size(); isparseseg++ )
    {
        const SparseFileSegment& sparseseg = arrsparseseg[isparseseg];
        Enforce( sparseseg.ibFirst <= sparseseg.ibLast );

        Enforce( sparseseg.ibFirst >= OffsetOfPgno( pgnoSystemRoot + 3 ) );

        Enforce( sparseseg.ibFirst >= ibFirstRangeImportant );
        Enforce( sparseseg.ibLast <= ibLastRangeImportant );

        const QWORD cbToWrite = sparseseg.ibLast - sparseseg.ibFirst + 1;
        Enforce( ( cbToWrite % g_cbPage ) == 0 );

        Call( ErrSPIWriteZeroesDatabase( ifmp, sparseseg.ibFirst, cbToWrite, tc ) );
    }

HandleError:
    return err;
}


LOCAL ERR FSPIParentIsFs( FUCB * const pfucb )
{
    AssertSPIPfucbOnRoot( pfucb );
    Assert( pfucb->u.pfcb->FSpaceInitialized() );

    const SPACE_HEADER * const psph = PsphSPIRootPage( pfucb );
    const PGNO pgnoParentFDP = psph->PgnoParent();

    return pgnoNull == pgnoParentFDP;
}

LOCAL ERR ErrSPIGetSe(
    FUCB * const    pfucb,
    FUCB * const    pfucbAE,
    const CPG       cpgReq,
    const CPG       cpgMin,
    const ULONG     fSPFlags,
    const SpacePool sppPool,
    const BOOL      fMayViolateMaxSize )
{
    ERR             err;
    PIB             *ppib                   = pfucb->ppib;
    FUCB            *pfucbParent            = pfucbNil;
    CPG             cpgSEReq;
    CPG             cpgSEMin;
    PGNO            pgnoSELast;
    FMP             *pfmp                   = &g_rgfmp[ pfucb->ifmp ];
    const BOOL      fSplitting              = BoolSetFlag( fSPFlags, fSPSplitting );

    AssertSPIPfucbOnRoot( pfucb );
    Assert( !pfmp->FReadOnlyAttach() );
    Assert( pfucbNil != pfucbAE );
    Assert( !Pcsr( pfucbAE )->FLatched() );
    Assert( pfucb->u.pfcb->FSpaceInitialized() );
    Assert( !FSPIParentIsFs( pfucb ) );

    const SPACE_HEADER * const psph = PsphSPIRootPage( pfucb );
    const PGNO pgnoParentFDP = psph->PgnoParent();
    Assert( pgnoNull != pgnoParentFDP );


    PGNO    pgnoSEFirst = pgnoNull;
    BOOL    fSmallFDP   = fFalse;

    cpgSEMin = cpgMin;

    if ( fSPFlags & fSPOriginatingRequest )
    {

        if ( fSPFlags & fSPExactExtent )
        {
            cpgSEReq = cpgReq;
        }
        else
        {
            if ( psph->Fv1() &&
                    ( psph->CpgPrimary() < cpgSmallFDP ) )
            {
                const CPAGE&    cpage   = pfucb->pcsrRoot->Cpage();

                if ( cpage.FLeafPage()
                    || ( cpage.FParentOfLeaf() && cpage.Clines() < cpgSmallFDP ) )
                {
                    Call( ErrSPICheckSmallFDP( pfucb, &fSmallFDP ) );
                }
            }

            if ( fSmallFDP )
            {
                cpgSEReq = max( cpgReq, cpgSmallGrow );
            }
            else
            {
                const CPG   cpgSEReqDefault     = ( pfmp->FIsTempDB()
                                                    && ppib->FBatchIndexCreation() ?
                                                                cpageDbExtensionDefault :
                                                                cpageSEDefault );

                Assert( cpageDbExtensionDefault != cpgSEReqDefault
                    || pgnoSystemRoot == pgnoParentFDP );

                const CPG cpgNextLeast = psph->Fv1() ? ( psph->CpgPrimary() / cSecFrac ) : psph->CpgLastAlloc();
                cpgSEReq = max( cpgReq, max( cpgNextLeast, cpgSEReqDefault ) );
            }
        }

        Assert( psph->FMultipleExtent() );
        const CPG   cpgLastAlloc = psph->Fv1() ? psph->CpgPrimary() : psph->CpgLastAlloc();

        cpgSEReq = max( cpgSEReq, CpgSPIGetNextAlloc( pfucb->u.pfcb->Pfcbspacehints(), cpgLastAlloc ) );
        if ( cpgSEReq != cpgLastAlloc )
        {
            (void)ErrSPIImproveLastAlloc( pfucb, psph, cpgSEReq );
        }

        if ( fSPFlags & fSPExactExtent )
        {
            cpgSEMin = cpgSEReq;
        }
    }
    else
    {
        cpgSEReq = cpgReq;
    }

    Call( ErrBTIOpenAndGotoRoot( pfucb->ppib, pgnoParentFDP, pfucb->ifmp, &pfucbParent ) );

    Call( ErrSPIReserveSPBufPages( pfucb, pfucbParent ) );

    err = ErrSPIGetExt(
                pfucb->u.pfcb,
                pfucbParent,
                &cpgSEReq,
                cpgSEMin,
                &pgnoSEFirst,
                fSPFlags & ( fSplitting | fSPExactExtent ),
                0,
                NULL,
                fMayViolateMaxSize );
    AssertSPIPfucbOnRoot( pfucbParent );
    AssertSPIPfucbOnRoot( pfucb );
    Call( err );

    Assert( cpgSEReq >= cpgSEMin );
    pgnoSELast = pgnoSEFirst + cpgSEReq - 1;

    BTUp( pfucbAE );

    if ( pgnoSystemRoot == pgnoParentFDP )
    {
        if ( pfmp->Pdbfilehdr()->le_ulTrimCount > 0 )
        {
            OSTraceFMP( pfucb->ifmp, JET_tracetagSpaceInternal,
                        OSFormat( "Allocating Root Extent( %d - %d, %d )\n", pgnoSEFirst, pgnoSELast, cpgSEReq ) );

            PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
            Call( ErrSPIAllocatePagesSlowlyIfSparse( pfucb->ifmp, pgnoSEFirst, pgnoSELast, cpgSEReq, *tcScope ) );
        }
    }

    err = ErrSPIAddSecondaryExtent(
                    pfucb,
                    pfucbAE,
                    pgnoSELast,
                    cpgSEReq,
                    cpgSEReq,
                    NULL,
                    sppPool );
    Assert( errSPOutOfOwnExtCacheSpace != err );
    Assert( errSPOutOfAvailExtCacheSpace != err );
    Call( err );

    AssertSPIPfucbOnRoot( pfucb );
    Assert( Pcsr( pfucbAE )->FLatched() );
    Assert( cpgSEReq >= cpgSEMin );

HandleError:
    if ( pfucbNil != pfucbParent )
    {
        if ( pcsrNil != pfucbParent->pcsrRoot )
        {
            pfucbParent->pcsrRoot->ReleasePage();
            pfucbParent->pcsrRoot = pcsrNil;
        }

        Assert( !Pcsr( pfucbParent )->FLatched() );
        BTClose( pfucbParent );
    }

    return err;
}

LOCAL ERR ErrSPIGetFsSe(
    FUCB * const    pfucb,
    FUCB * const    pfucbAE,
    const CPG       cpgReq,
    const CPG       cpgMin,
    const ULONG     fSPFlags,
    const BOOL      fExact,
    const BOOL      fPermitAsyncExtension,
    const BOOL      fMayViolateMaxSize )
{
    ERR             err = JET_errSuccess;
    PIB             *ppib = pfucb->ppib;
    CPG             cpgSEReq = cpgReq;
    CPG             cpgSEMin = cpgMin;
    CPG             cpgSEAvail = 0;
    PGNO            pgnoSELast = pgnoNull;
    FMP             *pfmp = &g_rgfmp[ pfucb->ifmp ];
    PGNO            pgnoParentFDP = pgnoNull;
    CArray<EXTENTINFO> arreiReleased;

    AssertSPIPfucbOnRoot( pfucb );
    Assert( !pfmp->FReadOnlyAttach() );
    Assert( pfucbNil != pfucbAE );
    Assert( !Pcsr( pfucbAE )->FLatched() );
    Assert( pfucb->u.pfcb->FSpaceInitialized() );
    Assert( FSPIParentIsFs( pfucb ) );
    Assert( cpgMin > 0 );
    Assert( cpgReq >= cpgMin );

    const SPACE_HEADER * const psph = PsphSPIRootPage( pfucb );
    pgnoParentFDP = psph->PgnoParent();
    Assert( pgnoNull == pgnoParentFDP );


    const CPG cpgDbExtensionSize = (CPG)UlParam( PinstFromPpib( ppib ), JET_paramDbExtensionSize );

    PGNO pgnoPreLast = 0;

    if ( fExact )
    {
        Expected( cpgSEMin == cpgSEReq );
        Expected( 0 == cpgSEReq % cpgDbExtensionSize );
        cpgSEMin = cpgSEReq;
    }
    else
    {
        if ( pfmp->FIsTempDB() && !ppib->FBatchIndexCreation() )
        {
            cpgSEReq = max( cpgReq, cpageSEDefault );
        }
        else
        {
            cpgSEReq = max( cpgReq, cpgDbExtensionSize );
        }

        Assert( psph->Fv1() );

        cpgSEReq = max( cpgSEReq, psph->Fv1() ? ( psph->CpgPrimary() / cSecFrac ) : psph->CpgLastAlloc() );

        OSTraceFMP( pfucb->ifmp, JET_tracetagSpaceInternal, OSFormat( "GetFsSe[%d] - %d ? %d : %d,  %d -> %d",
                        (ULONG)pfucb->ifmp, psph->Fv1(), psph->Fv1() ? ( psph->CpgPrimary() / cSecFrac ) : -1,
                        psph->Fv1() ? -1 : psph->CpgLastAlloc(), cpgSEMin, cpgSEReq ) );

        if ( !pfmp->FIsTempDB() )
        {

            cpgSEReq = roundup( cpgSEReq, cpgDbExtensionSize );

            Assert( cpgSEMin >= cpgMin );
            Assert( cpgSEReq >= cpgReq );
            Assert( cpgSEReq >= cpgSEMin );


            Expected( FBFLatched( pfucb->ifmp, pgnoSystemRoot ) );
            pgnoPreLast = g_rgfmp[pfucb->ifmp].PgnoLast();
            const CPG cpgLogicalFileSize = pgnoPreLast + cpgDBReserved;

            Enforce( pgnoPreLast < 0x100000000 );

            Enforce( g_rgfmp[pfucb->ifmp].CbOwnedFileSize() / g_cbPage < 0x100000000 );

#ifdef DEBUG
            {
            PGNO pgnoPreLastCheck = 0;
            if ( ErrSPGetLastPgno( ppib, pfucb->ifmp, &pgnoPreLastCheck ) >= JET_errSuccess )
            {
                Assert( pgnoPreLastCheck == pgnoPreLast || g_fRepair );
            }

            const CPG cpgFullFileSize =  pfmp->CpgOfCb( g_rgfmp[pfucb->ifmp].CbOwnedFileSize() );
            const CPG cpgDbExt = (CPG)UlParam( PinstFromPfucb( pfucb ), JET_paramDbExtensionSize );

            OSTraceFMP( pfucb->ifmp, JET_tracetagSpaceInternal, OSFormat( "( %d + 2 [ + %d ] == %d )\n", pgnoPreLast, cpgDbExt, cpgFullFileSize ) );



            }
#endif


            const CPG cpgOverage = cpgLogicalFileSize % cpgDbExtensionSize;
            if ( cpgOverage )
            {

                const CPG cpgReduction = cpgSEReq - cpgOverage;
                if ( cpgReduction >= cpgSEMin )
                {
                    cpgSEReq = cpgReduction;
                }
                else
                {
                    Assert( cpgDbExtensionSize - cpgOverage > 0 );
                    cpgSEReq += ( cpgDbExtensionSize - cpgOverage );
                }
            }
        }
    }

    Assert( cpgSEMin >= cpgMin );
    Assert( cpgSEReq >= cpgSEMin );

    Assert( !Ptls()->fNoExtendingDuringCreateDB );

    Call( ErrSPIReserveSPBufPages( pfucb, pfucbNil ) );
    Assert( pgnoPreLast <= g_rgfmp[pfucb->ifmp].PgnoLast() || g_fRepair );

    OSTraceFMP( pfucb->ifmp, JET_tracetagSpaceInternal,
            OSFormat( "Pre-extend stats: %I64u bytes (pgnoLast %I32u) ... %d unaligned overage",
                g_rgfmp[pfucb->ifmp].CbOwnedFileSize(),
                g_rgfmp[pfucb->ifmp].PgnoLast(),
                g_rgfmp[pfucb->ifmp].CpgOfCb( g_rgfmp[pfucb->ifmp].CbOwnedFileSize() ) % cpgDbExtensionSize  ) );

    Assert( cpgSEMin >= cpgMin );
    Assert( cpgSEReq >= cpgSEMin );

    Call( ErrSPIExtendDB(
            pfucb,
            cpgSEMin,
            &cpgSEReq,
            &pgnoSELast,
            fPermitAsyncExtension,
            fMayViolateMaxSize,
            &arreiReleased,
            &cpgSEAvail ) );
    Assert( pgnoPreLast <= g_rgfmp[pfucb->ifmp].PgnoLast() || g_fRepair );


    BTUp( pfucbAE );
    err = ErrSPIAddSecondaryExtent(
            pfucb,
            pfucbAE,
            pgnoSELast,
            cpgSEReq,
            cpgSEAvail,
            &arreiReleased,
            spp::AvailExtLegacyGeneralPool );
    Assert( errSPOutOfOwnExtCacheSpace != err );
    Assert( errSPOutOfAvailExtCacheSpace != err );
    Call( err );
    Assert( pgnoPreLast < g_rgfmp[pfucb->ifmp].PgnoLast() || g_fRepair );
    Assert( arreiReleased.Size() == 0 );

    OSTraceFMP( pfucb->ifmp, JET_tracetagSpaceInternal,
            OSFormat( "Post-extend stats: %I64u bytes (pgnoLast %I32u) ... %d unaligned overage",
                g_rgfmp[pfucb->ifmp].CbOwnedFileSize(),
                g_rgfmp[pfucb->ifmp].PgnoLast(),
                g_rgfmp[pfucb->ifmp].CpgOfCb( g_rgfmp[pfucb->ifmp].CbOwnedFileSize() ) % cpgDbExtensionSize  ) );

#ifdef DEBUG
    Assert( FBFLatched( pfucb->ifmp, pgnoSystemRoot ) );

#endif

    if ( pfmp->FCacheAvail() )
    {
        pfmp->AdjustCpgAvail( cpgSEAvail );
    }

    AssertSPIPfucbOnRoot( pfucb );
    Assert( Pcsr( pfucbAE )->FLatched() );
    Assert( cpgSEAvail >= cpgSEMin );

HandleError:
    return err;
}



LOCAL ERR ErrSPIGetSparseInfoRange(
    _In_ FMP* const pfmp,
    _In_ const PGNO pgnoStart,
    _In_ const PGNO pgnoEnd,
    _Out_ CPG* pcpgSparse
    )
{
    ERR err = JET_errSuccess;
    CPG cpgSparseTotal = 0;
    CPG cpgAllocatedTotal = 0;
    const QWORD ibEndRange = OffsetOfPgno( pgnoEnd + 1 );
    CPG cpgRegionPrevious = 0;
    *pcpgSparse = 0;

    Assert( pgnoStart <= pgnoEnd );

    for ( PGNO pgnoQuery = pgnoStart; pgnoQuery < pgnoEnd + 1; pgnoQuery += cpgRegionPrevious )
    {
        const QWORD ibOffsetToQuery = OffsetOfPgno( pgnoQuery );
        QWORD ibStartAllocatedRegion;
        QWORD cbAllocated;

        Assert( ibOffsetToQuery < ibEndRange );

        cpgRegionPrevious = 0;

        Call( pfmp->Pfapi()->ErrRetrieveAllocatedRegion( ibOffsetToQuery, &ibStartAllocatedRegion, &cbAllocated ) );

        Assert( ibStartAllocatedRegion >= ibOffsetToQuery || 0 == ibStartAllocatedRegion );

        if ( ibStartAllocatedRegion > ibOffsetToQuery )
        {
            const QWORD ibEndSparse = min( ibEndRange, ibStartAllocatedRegion );
            const CPG cpgSparseThisQuery = pfmp->CpgOfCb( ibEndSparse - ibOffsetToQuery );

            cpgSparseTotal += cpgSparseThisQuery;
            cpgRegionPrevious += cpgSparseThisQuery;
        }
        else if ( 0 == ibStartAllocatedRegion )
        {
            const QWORD ibEndSparse = ibEndRange;
            const CPG cpgSparseThisQuery = pfmp->CpgOfCb( ibEndSparse - ibOffsetToQuery );

            cpgSparseTotal += cpgSparseThisQuery;
            cpgRegionPrevious += cpgSparseThisQuery;
        }

        const QWORD ibEndAllocated = min( ibEndRange, ibStartAllocatedRegion + cbAllocated );
        if ( ibEndAllocated > 0 && ibEndAllocated <= ibEndRange )
        {
            const QWORD ibStartAllocated = min( ibEndRange, ibStartAllocatedRegion );
            Assert( ibEndAllocated >= ibStartAllocated );
            const CPG cpgAllocatedThisQuery = pfmp->CpgOfCb( ibEndAllocated - ibStartAllocated );

            cpgAllocatedTotal += cpgAllocatedThisQuery;
            cpgRegionPrevious += cpgAllocatedThisQuery;
        }

        Assert( cpgRegionPrevious > 0 );

        Assert( pgnoQuery + cpgRegionPrevious <= pgnoEnd + 1 );
    }

    Assert( ( (CPG) ( pgnoEnd - pgnoStart + 1 ) ) == ( cpgAllocatedTotal + cpgSparseTotal ) );

    *pcpgSparse = cpgSparseTotal;

HandleError:
    return err;
}

LOCAL ERR ErrSPITrimRegion(
    _In_ const IFMP ifmp,
    _In_ PIB* const ppib,
    _In_ const PGNO pgnoLast,
    _In_ const CPG cpgRequestedToTrim,
    _Out_ CPG* pcpgSparseBeforeThisExtent,
    _Out_ CPG* pcpgSparseAfterThisExtent
    )
{
    ERR err;

    *pcpgSparseBeforeThisExtent = 0;
    *pcpgSparseAfterThisExtent = 0;

    const PGNO pgnoStartZeroes = pgnoLast - cpgRequestedToTrim + 1;
    PGNO pgnoStartZeroesAligned = 0;
    PGNO pgnoEndZeroesAligned = 0;
    CPG cpgZeroesAligned = 0;

    Assert( cpgRequestedToTrim > 0 );

    if ( ( UlParam(PinstFromIfmp ( ifmp ), JET_paramWaypointLatency ) > 0 ) ||
         g_rgfmp[ ifmp ].FDBMScanOn() )
    {

        OSTraceFMP( ifmp, JET_tracetagSpaceInternal,
                OSFormat( "Skipping Trim for ifmp %d because either LLR or DBScan is on (cpgRequested=%d).", ifmp, cpgRequestedToTrim ) );

        err = JET_errSuccess;
        goto HandleError;
    }

    QWORD ibStartZeroes = 0;
    QWORD cbZeroLength = 0;
    Call( ErrIOTrimNormalizeOffsetsAndPgnos(
        ifmp,
        pgnoStartZeroes,
        cpgRequestedToTrim,
        &ibStartZeroes,
        &cbZeroLength,
        &pgnoStartZeroesAligned,
        &cpgZeroesAligned ) );

    Assert( cpgZeroesAligned <= cpgRequestedToTrim );
    const CPG cpgNotZeroedDueToAlignment = cpgRequestedToTrim - cpgZeroesAligned;
    pgnoEndZeroesAligned = pgnoStartZeroesAligned + cpgZeroesAligned - 1;

    if ( cpgNotZeroedDueToAlignment > 0 )
    {
        PERFOpt( cSPPagesNotTrimmedUnalignedPage.Add( PinstFromIfmp( ifmp ), cpgNotZeroedDueToAlignment ) );
    }

    if ( cbZeroLength > 0 )
    {
        Assert( pgnoStartZeroesAligned > 0 );
        Assert( pgnoEndZeroesAligned >= pgnoStartZeroesAligned );

        const PGNO pgnoEndRegionAligned = pgnoStartZeroesAligned + cpgZeroesAligned - 1;
        Call( ErrSPIGetSparseInfoRange( &g_rgfmp[ ifmp ], pgnoStartZeroesAligned, pgnoEndRegionAligned, pcpgSparseBeforeThisExtent ) );
        *pcpgSparseAfterThisExtent = *pcpgSparseBeforeThisExtent;

        Assert( *pcpgSparseBeforeThisExtent <= cpgZeroesAligned );

        if ( *pcpgSparseBeforeThisExtent >= cpgZeroesAligned )
        {
            OSTraceFMP( ifmp, JET_tracetagSpaceInternal,
                        OSFormat( "Trimming ifmp %d for [start=%d,cpg=%d] is skipped because the region is already trimmed.",
                                  ifmp, pgnoStartZeroesAligned, cpgZeroesAligned ) );
        }
        else
        {
            TraceContextScope tcScope( iorpDatabaseTrim );
            if ( tcScope->nParentObjectClass == pocNone )
            {
                tcScope->nParentObjectClass = tceNone;
            }

            Call( ErrBFPreparePageRangeForExternalZeroing( ifmp, pgnoStartZeroesAligned, cpgZeroesAligned, *tcScope ) );

            if ( g_rgfmp[ ifmp ].FLogOn() )
            {
                Call( ErrLGTrimDatabase( PinstFromIfmp( ifmp )->m_plog, ifmp, ppib, pgnoStartZeroesAligned, cpgZeroesAligned ) );
            }

            PERFOpt( cSPPagesTrimmed.Add( PinstFromIfmp( ifmp ), cpgZeroesAligned ) );

            Call( ErrSPITrimUpdateDatabaseHeader( ifmp ) );

            err = ErrIOTrim( ifmp, ibStartZeroes, cbZeroLength );
            if( JET_errUnloadableOSFunctionality == err ||
                    JET_errFeatureNotAvailable == err )
            {
                *pcpgSparseAfterThisExtent = *pcpgSparseBeforeThisExtent;
                err = JET_errSuccess;
                goto HandleError;
            }
            Call( err );

            CallS( err );
    
            const ERR errT = ErrSPIGetSparseInfoRange( &g_rgfmp[ ifmp ], pgnoStartZeroesAligned, pgnoEndRegionAligned, pcpgSparseAfterThisExtent );

            Assert( errT >= JET_errSuccess );

            if ( errT >= JET_errSuccess )
            {

                Assert( *pcpgSparseAfterThisExtent >= 0 );
                Assert( *pcpgSparseAfterThisExtent >= *pcpgSparseBeforeThisExtent );
            }
        }
    }

HandleError:
    Assert( *pcpgSparseAfterThisExtent >= *pcpgSparseBeforeThisExtent );

    return err;
}


LOCAL ERR ErrSPIAddFreedExtent(
    FUCB        *pfucb,
    FUCB        *pfucbAE,
    const PGNO  pgnoLast,
    const CPG   cpgSize )
{
    ERR         err;
    const IFMP ifmp = pfucbAE->ifmp;

    AssertSPIPfucbOnRoot( pfucb );
    Assert( !Pcsr( pfucbAE )->FLatched() );

    const PGNO pgnoParentFDP = PsphSPIRootPage( pfucb )->PgnoParent();


    Call( ErrSPIAddToAvailExt( pfucbAE, pgnoLast, cpgSize, spp::AvailExtLegacyGeneralPool ) );
    Assert( Pcsr( pfucbAE )->FLatched() );

    if ( ( pgnoParentFDP == pgnoSystemRoot ) &&
         g_rgfmp[ ifmp ].FTrimSupported() &&
         ( ( GrbitParam( g_rgfmp[ ifmp ].Pinst(), JET_paramEnableShrinkDatabase ) & ( JET_bitShrinkDatabaseOn | JET_bitShrinkDatabaseRealtime ) ) ==
         ( JET_bitShrinkDatabaseOn | JET_bitShrinkDatabaseRealtime ) ) )
    {
        CPG cpgSparseBeforeThisExtent = 0;
        CPG cpgSparseAfterThisExtent = 0;
        (void) ErrSPITrimRegion( ifmp, pfucbAE->ppib, pgnoLast, cpgSize, &cpgSparseBeforeThisExtent, &cpgSparseAfterThisExtent );
    }

HandleError:
    AssertSPIPfucbOnRoot( pfucb );

    return err;
}


INLINE ERR ErrSPCheckInfoBuf( const ULONG cbBufSize, const ULONG fSPExtents )
{
    ULONG   cbUnchecked     = cbBufSize;

    if ( FSPOwnedExtent( fSPExtents ) )
    {
        if ( cbUnchecked < sizeof(CPG) )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }
        cbUnchecked -= sizeof(CPG);

        if ( FSPExtentList( fSPExtents ) )
        {
            if ( cbUnchecked < sizeof(EXTENTINFO) )
            {
                return ErrERRCheck( JET_errBufferTooSmall );
            }
            cbUnchecked -= sizeof(EXTENTINFO);
        }
    }

    if ( FSPAvailExtent( fSPExtents ) )
    {
        if ( cbUnchecked < sizeof(CPG) )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }
        cbUnchecked -= sizeof(CPG);

        if ( FSPExtentList( fSPExtents ) )
        {
            if ( cbUnchecked < sizeof(EXTENTINFO) )
            {
                return ErrERRCheck( JET_errBufferTooSmall );
            }
            cbUnchecked -= sizeof(EXTENTINFO);
        }
    }

    if ( FSPReservedExtent( fSPExtents ) )
    {
        if ( cbUnchecked < sizeof(CPG) )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }
        cbUnchecked -= sizeof(CPG);
    }

    if ( FSPShelvedExtent( fSPExtents ) )
    {
        if ( cbUnchecked < sizeof(CPG) )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }
        cbUnchecked -= sizeof(CPG);
    }

    return JET_errSuccess;
}

_When_( (return >= 0), _Post_satisfies_( *pcextMac <= *pcextMax ))
LOCAL ERR ErrSPIAddExtentInfo(
    _Inout_ BTREE_SPACE_EXTENT_INFO **  pprgext,
    _Inout_ ULONG *                                                         pcextMax,
    _Inout_ ULONG *                                                         pcextMac,
    _In_ const SpacePool                                                    sppPool,
    _In_ const PGNO                                                         pgnoLast,
    _In_ const CPG                                                          cpgExtent,
    _In_ const PGNO                                                         pgnoSpaceNode )
{
    ERR err = JET_errSuccess;

    Expected( cpgExtent || sppPool == spp::ContinuousPool );

    if ( *pcextMac >= *pcextMax )
    {
        Assert( *pcextMac == *pcextMax );
        BTREE_SPACE_EXTENT_INFO * prgextToFree = *pprgext;
        const ULONG cextNewSize = (*pcextMax) * 2;
        Alloc( *pprgext = new BTREE_SPACE_EXTENT_INFO[cextNewSize] );
        *pcextMax = cextNewSize;
        memset( *pprgext, 0, sizeof(BTREE_SPACE_EXTENT_INFO)*(*pcextMax) );
        C_ASSERT( sizeof(**pprgext) == 16 );
        memcpy( *pprgext, prgextToFree, *pcextMac * sizeof(**pprgext) );
        delete [] prgextToFree;
    }
    Assert( *pcextMac < *pcextMax );
    AssumePREFAST( *pcextMac < *pcextMax );

    (*pprgext)[*pcextMac].iPool = (ULONG)sppPool;
    (*pprgext)[*pcextMac].pgnoLast = pgnoLast;
    (*pprgext)[*pcextMac].cpgExtent = cpgExtent;
    (*pprgext)[*pcextMac].pgnoSpaceNode = pgnoSpaceNode;

    (*pcextMac)++;

HandleError:

    return err;
}

_Pre_satisfies_( *pcextMac <= *pcextMax )
_When_( (return >= 0), _Post_satisfies_( *pcextMac <= *pcextMax ))
LOCAL ERR ErrSPIExtGetExtentListInfo(
    _Inout_ FUCB *                      pfucb,
    _At_(*pprgext, _Out_writes_to_(*pcextMax, *pcextMac)) BTREE_SPACE_EXTENT_INFO **    pprgext,
    _Inout_ ULONG *                     pcextMax,
    _Inout_ ULONG *                     pcextMac )
{
    ERR         err;
    DIB         dib;

    ULONG       cRecords        = 0;
    ULONG       cRecordsDeleted = 0;

    FUCBSetSequential( pfucb );
    FUCBSetPrereadForward( pfucb, cpgPrereadSequential );

    dib.dirflag = fDIRNull;
    dib.pos = posFirst;
    Assert( FFUCBSpace( pfucb ) );
    err = ErrBTDown( pfucb, &dib, latchReadNoTouch );

    if ( err != JET_errRecordNotFound )
    {
        Call( err );

        forever
        {
            const CSPExtentInfo cspext( pfucb );

            ++cRecords;
            if( FNDDeleted( pfucb->kdfCurr ) )
            {
                ++cRecordsDeleted;
            }
            else
            {
                Assert( ( cspext.SppPool() != spp::ShelvedPool ) || FFUCBAvailExt( pfucb ) );
                Assert( ( cspext.SppPool() != spp::ShelvedPool ) || ( pfucb->u.pfcb->PgnoFDP() == pgnoSystemRoot ) );
                Expected( ( cspext.SppPool() != spp::ShelvedPool ) || ( cspext.CpgExtent() == 1 ) );
                Expected( FSPIValidExplicitSpacePool( cspext.SppPool() ) );
                Expected( cspext.CpgExtent() != 0 || cspext.SppPool() == spp::ContinuousPool );
                Call( ErrSPIAddExtentInfo( pprgext, pcextMax, pcextMac, (SpacePool)cspext.SppPool(), cspext.PgnoLast(), cspext.CpgExtent(), pfucb->csr.Pgno() ) );
            }

            err = ErrBTNext( pfucb, fDIRNull );
            if ( err < 0 )
            {
                if ( err == JET_errNoCurrentRecord )
                {
                    break;
                }
                Call( err );
            }
        }
    }

    Assert( err == JET_errNoCurrentRecord || err == JET_errRecordNotFound );
    err = JET_errSuccess;

HandleError:
    return err;
}


LOCAL ERR ErrSPIGetInfo(
    FUCB        *pfucb,
    CPG         *pcpgTotal,
    CPG         *pcpgReserved,
    CPG         *pcpgShelved,
    INT         *piext,
    INT         cext,
    EXTENTINFO  *rgext,
    INT         *pcextSentinelsRemaining,
    CPRINTF     * const pcprintf )
{
    ERR         err;
    DIB         dib;
    INT         iext            = *piext;
    const BOOL  fExtentList     = ( cext > 0 );

    PGNO        pgnoLastSeen    = pgnoNull;
    CPG         cpgSeen         = 0;
    ULONG       cRecords        = 0;
    ULONG       cRecordsDeleted = 0;

    Assert( !fExtentList || NULL != pcextSentinelsRemaining );

    *pcpgTotal = 0;
    if ( pcpgReserved )
    {
        *pcpgReserved = 0;
    }
    if ( pcpgShelved )
    {
        *pcpgShelved = 0;
    }

    FUCBSetSequential( pfucb );
    FUCBSetPrereadForward( pfucb, cpgPrereadSequential );

    dib.dirflag = fDIRNull;
    dib.pos = posFirst;
    Assert( FFUCBSpace( pfucb ) );
    err = ErrBTDown( pfucb, &dib, latchReadNoTouch );

    if ( err != JET_errRecordNotFound )
    {
        Call( err );

        forever
        {
            const CSPExtentInfo cspext( pfucb );

            if( pcprintf )
            {
                CPG cpgSparse = 0;

                if ( !cspext.FEmptyExtent() )
                {
                    (void) ErrSPIGetSparseInfoRange( &g_rgfmp[ pfucb->ifmp ], cspext.PgnoFirst(), cspext.PgnoLast(), &cpgSparse );
                }

                if( pgnoLastSeen != Pcsr( pfucb )->Pgno() )
                {
                    pgnoLastSeen = Pcsr( pfucb )->Pgno();

                    ++cpgSeen;
                }

                (*pcprintf)( "%30s: %s[%5d]:\t%6d-%6d (%3d) %s%s",
                                SzNameOfTable( pfucb ),
                                SzSpaceTreeType( pfucb ),
                                Pcsr( pfucb )->Pgno(),
                                cspext.FEmptyExtent() ? 0 : cspext.PgnoFirst(),
                                cspext.FEmptyExtent() ? cspext.PgnoMarker() : cspext.PgnoLast(),
                                cspext.CpgExtent(),
                                FNDDeleted( pfucb->kdfCurr ) ? " (DEL)" : "",
                                ( cspext.ErrCheckCorrupted( ) < JET_errSuccess ) ? " (COR)" : ""
                                );
                if ( cspext.FNewAvailFormat() )
                {
                    (*pcprintf)( "  Pool: %d %s",
                                    cspext.SppPool(),
                                    cspext.FNewAvailFormat() ? "(fNewAvailFormat)" : ""
                                    );
                }
                if ( cpgSparse > 0 )
                {
                    (*pcprintf)( " cpgSparse: %3d", cpgSparse );
                }
                (*pcprintf)( "\n" );

                ++cRecords;
                if( FNDDeleted( pfucb->kdfCurr ) )
                {
                    ++cRecordsDeleted;
                }
            }

            if ( cspext.SppPool() != spp::ShelvedPool )
            {
                *pcpgTotal += cspext.CpgExtent();
            }

            if ( pcpgReserved && cspext.FNewAvailFormat() &&
                ( cspext.SppPool() == spp::ContinuousPool ) )
            {
                *pcpgReserved += cspext.CpgExtent();
            }

            BOOL fSuppressExtent = fFalse;

            if ( pcpgShelved && cspext.FNewAvailFormat() &&
                ( cspext.SppPool() == spp::ShelvedPool ) )
            {
                if ( cspext.PgnoLast() > g_rgfmp[ pfucb->ifmp ].PgnoLast() )
                {
                    Assert( cspext.PgnoFirst() > g_rgfmp[ pfucb->ifmp ].PgnoLast() );
                    *pcpgShelved += cspext.CpgExtent();
                }
                else
                {
                    fSuppressExtent = fTrue;
                }
            }

            if ( fExtentList && !fSuppressExtent )
            {
                Assert( iext < cext );

                Assert( iext + *pcextSentinelsRemaining <= cext );
                if ( iext + *pcextSentinelsRemaining < cext )
                {
                    rgext[iext].pgnoLastInExtent = cspext.PgnoLast();
                    rgext[iext].cpgExtent = cspext.CpgExtent();
                    iext++;
                }
            }

            err = ErrBTNext( pfucb, fDIRNull );
            if ( err < 0 )
            {
                if ( err != JET_errNoCurrentRecord )
                    goto HandleError;
                break;
            }
        }
    }

    if ( fExtentList )
    {
        Assert( iext < cext );

        rgext[iext].pgnoLastInExtent = pgnoNull;
        rgext[iext].cpgExtent = 0;
        iext++;

        Assert( NULL != pcextSentinelsRemaining );
        Assert( *pcextSentinelsRemaining > 0 );
        (*pcextSentinelsRemaining)--;
    }

    *piext = iext;
    Assert( *piext + *pcextSentinelsRemaining <= cext );

    if( pcprintf )
    {
        (*pcprintf)( "%s:\t%d pages, %d nodes, %d deleted nodes\n",
                        SzNameOfTable( pfucb ),
                        cpgSeen,
                        cRecords,
                        cRecordsDeleted );
    }

    err = JET_errSuccess;

HandleError:

    return err;
}


VOID SPDumpSplitBufExtent(
    CPRINTF * const             pcprintf,
    __in const CHAR *           szTable,
    __in const CHAR *           szSpaceTree,
    __in const PGNO             pgnoSpaceFDP,
    __in const SPLIT_BUFFER *   pspbuf
    )
{
    if ( pcprintf )
    {
        if ( pspbuf->CpgBuffer1() )
        {
            (*pcprintf)( "%30s:  AE[%5d]:\t%6d-%6d (%3d)   Pool: (%s Split Buffer)\n",
                                        szTable,
                                        pgnoSpaceFDP,
                                        ( pspbuf->CpgBuffer1() <= 0 ) ? 0 : pspbuf->PgnoLastBuffer1() - pspbuf->CpgBuffer1() + 1,
                                        pspbuf->PgnoLastBuffer1(),
                                        pspbuf->CpgBuffer1(),
                                        szSpaceTree
                                        );
        }
        if ( pspbuf->CpgBuffer2() )
        {
            (*pcprintf)( "%30s:  AE[%5d]:\t%6d-%6d (%3d)   Pool: (%s Split Buffer)\n",
                                        szTable,
                                        pgnoSpaceFDP,
                                        ( pspbuf->CpgBuffer2() <= 0 ) ? 0 : pspbuf->PgnoLastBuffer2() - pspbuf->CpgBuffer2() + 1,
                                        pspbuf->PgnoLastBuffer2(),
                                        pspbuf->CpgBuffer2(),
                                        szSpaceTree
                                        );
        }
    }
}


ERR ErrSPGetDatabaseInfo(
    PIB         *ppib,
    const IFMP  ifmp,
    __out_bcount(cbMax) BYTE        *pbResult,
    const ULONG cbMax,
    const ULONG fSPExtents,
    bool fUseCachedResult,
    CPRINTF * const pcprintf )
{
    ERR             err = JET_errSuccess;
    CPG             *pcpgT = (CPG *)pbResult;
    ULONG           cbMaxReq = 0;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortSpace );

    if ( ( fSPExtents & ( fSPOwnedExtent | fSPAvailExtent | fSPShelvedExtent ) ) == 0 )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( ( fSPExtents & ~( fSPOwnedExtent | fSPAvailExtent | fSPShelvedExtent ) ) != 0 )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    Assert( FSPOwnedExtent( fSPExtents ) || FSPAvailExtent( fSPExtents ) || FSPShelvedExtent( fSPExtents ) );

    if ( FSPShelvedExtent( fSPExtents ) )
    {
        Assert( FSPAvailExtent( fSPExtents ) );
        if ( fUseCachedResult )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
    }

    if ( FSPOwnedExtent( fSPExtents ) )
    {
        cbMaxReq += sizeof( CPG );
    }
    if ( FSPAvailExtent( fSPExtents ) )
    {
        cbMaxReq += sizeof( CPG );
    }
    if ( FSPShelvedExtent( fSPExtents ) )
    {
        cbMaxReq += sizeof( CPG );
    }

    if ( cbMax < cbMaxReq )
    {
        AssertSz( fFalse, "Called without the necessary buffer allocated for extents." );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    
    CallR( ErrSPCheckInfoBuf( cbMax, fSPExtents ) );
    memset( pbResult, 0, cbMax );

    int ipg = 0;
    const int ipgOwned = FSPOwnedExtent( fSPExtents ) ? ipg++ : -1;
    const int ipgAvail = FSPAvailExtent( fSPExtents ) ? ipg++ : -1;
    const int ipgShelved = FSPShelvedExtent( fSPExtents ) ? ipg++ : -1;

    if ( !fUseCachedResult ||
         ( FSPAvailExtent( fSPExtents ) && !g_rgfmp[ifmp].FCacheAvail() ) )
    {
        Call( ErrSPGetInfo( ppib, ifmp, pfucbNil, pbResult, cbMax, fSPExtents, pcprintf ) );

        if ( FSPAvailExtent( fSPExtents ) )
        {
            Assert( ipgAvail >= 0 );
            g_rgfmp[ifmp].SetCpgAvail( *( pcpgT + ipgAvail ) );
        }
    }
    else
    {
        if ( ipgOwned >= 0 )
        {
            *( pcpgT + ipgOwned ) = g_rgfmp[ifmp].PgnoLast();
        }
        if ( ipgAvail >= 0 )
        {
            *( pcpgT + ipgAvail ) = g_rgfmp[ifmp].CpgAvail();
        }
        Assert( ipgShelved < 0 );
    }

HandleError:
    return err;
}


ERR ErrSPGetInfo(
    PIB         *ppib,
    const IFMP  ifmp,
    FUCB        *pfucb,
    __out_bcount(cbMax) BYTE        *pbResult,
    const ULONG cbMax,
    const ULONG fSPExtents,
    CPRINTF * const pcprintf )
{
    ERR         err;
    CPG         *pcpgOwnExtTotal;
    CPG         *pcpgAvailExtTotal;
    CPG         *pcpgReservedExtTotal;
    CPG         *pcpgShelvedExtTotal;
    EXTENTINFO  *rgext;
    FUCB        *pfucbT             = pfucbNil;
    INT         iext;
    SPLIT_BUFFER    spbufOnOE;
    SPLIT_BUFFER    spbufOnAE;
    ULONG cbMaxReq = 0;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortSpace );

    Assert( ( fSPExtents & fSPExtentList ) == 0 );

    if ( !( FSPOwnedExtent( fSPExtents ) || FSPAvailExtent( fSPExtents ) ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    if ( FSPReservedExtent( fSPExtents ) && !FSPAvailExtent( fSPExtents ) )
    {
        ExpectedSz( fFalse, "initially we won't support getting reserved w/o avail." );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    if ( FSPShelvedExtent( fSPExtents ) && !FSPAvailExtent( fSPExtents ) )
    {
        ExpectedSz( fFalse, "initially we won't support getting shelved w/o avail." );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    if ( FSPOwnedExtent( fSPExtents ) )
    {
        cbMaxReq += sizeof( CPG );
    }
    if ( FSPAvailExtent( fSPExtents ) )
    {
        cbMaxReq += sizeof( CPG );
    }
    if ( FSPReservedExtent( fSPExtents ) )
    {
        cbMaxReq += sizeof( CPG );
    }
    if ( FSPShelvedExtent( fSPExtents ) )
    {
        cbMaxReq += sizeof( CPG );
    }

    if ( cbMax < cbMaxReq )
    {

        AssertSz( fFalse, "Called without the necessary buffer allocated for extents." );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    CallR( ErrSPCheckInfoBuf( cbMax, fSPExtents ) );

    memset( pbResult, '\0', cbMax );

    memset( (void*)&spbufOnOE, 0, sizeof(spbufOnOE) );
    memset( (void*)&spbufOnAE, 0, sizeof(spbufOnAE) );

    CPG * pcpgT = (CPG *)pbResult;
    pcpgOwnExtTotal = NULL;
    pcpgAvailExtTotal = NULL;
    pcpgReservedExtTotal = NULL;
    pcpgShelvedExtTotal = NULL;
    if ( FSPOwnedExtent( fSPExtents ) )
    {
        pcpgOwnExtTotal = pcpgT;
        pcpgT++;
    }
    if ( FSPAvailExtent( fSPExtents ) )
    {
        pcpgAvailExtTotal = pcpgT;
        pcpgT++;
    }
    if ( FSPReservedExtent( fSPExtents ) )
    {
        pcpgReservedExtTotal = pcpgT;
        pcpgT++;
    }
    if ( FSPShelvedExtent( fSPExtents ) )
    {
        pcpgShelvedExtTotal = pcpgT;
        pcpgT++;
    }
    rgext = (EXTENTINFO *)pcpgT;

    if ( pcpgOwnExtTotal )
    {
        *pcpgOwnExtTotal = 0;
    }
    if ( pcpgAvailExtTotal )
    {
        *pcpgAvailExtTotal = 0;
    }
    if ( pcpgReservedExtTotal )
    {
        *pcpgReservedExtTotal = 0;
    }
    if ( pcpgShelvedExtTotal )
    {
        *pcpgShelvedExtTotal = 0;
    }

    const BOOL  fExtentList     = FSPExtentList( fSPExtents );
    const INT   cext            = fExtentList ? ( ( pbResult + cbMax ) - ( (BYTE *)rgext ) ) / sizeof(EXTENTINFO) : 0;
    INT         cextSentinelsRemaining;

    cextSentinelsRemaining = 0;
    if ( fExtentList )
    {
        cextSentinelsRemaining += FSPOwnedExtent( fSPExtents ) ? 1 : 0;
        cextSentinelsRemaining += FSPAvailExtent( fSPExtents ) ? 1 : 0;
        Assert( cext >= cextSentinelsRemaining );
    }

    if ( pfucbNil == pfucb )
    {
        err = ErrBTOpen( ppib, pgnoSystemRoot, ifmp, &pfucbT );
    }
    else
    {
        err = ErrBTOpen( ppib, pfucb->u.pfcb, &pfucbT );
    }
    CallR( err );
    Assert( pfucbNil != pfucbT );
    tcScope->nParentObjectClass = TceFromFUCB( pfucbT );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucbT ) );

    Call( ErrBTIGotoRoot( pfucbT, latchReadTouch ) );
    Assert( pcsrNil == pfucbT->pcsrRoot );
    pfucbT->pcsrRoot = Pcsr( pfucbT );

    if ( !pfucbT->u.pfcb->FSpaceInitialized() )
    {
        SPIInitFCB( pfucbT, fTrue );
        if( !FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            BFPrereadPageRange( pfucbT->ifmp, pfucbT->u.pfcb->PgnoOE(), 2, NULL, NULL, bfprfDefault, ppib->BfpriPriority( pfucbT->ifmp ), *tcScope );
        }
    }

    iext = 0;

    if ( FSPOwnedExtent( fSPExtents ) )
    {
        Assert( NULL != pcpgOwnExtTotal );

        if ( FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            SPACE_HEADER    *psph;

            if( pcprintf )
            {
                (*pcprintf)( "\n%s: OWNEXT: single extent\n", SzNameOfTable( pfucbT ) );
            }

            Assert( pfucbT->pcsrRoot != pcsrNil );
            Assert( pfucbT->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
            Assert( pfucbT->pcsrRoot->FLatched() );
            Assert( !FFUCBSpace( pfucbT ) );

            NDGetExternalHeader( pfucbT, pfucbT->pcsrRoot, noderfSpaceHeader );
            Assert( sizeof( SPACE_HEADER ) == pfucbT->kdfCurr.data.Cb() );
            psph = reinterpret_cast <SPACE_HEADER *> ( pfucbT->kdfCurr.data.Pv() );

            *pcpgOwnExtTotal = psph->CpgPrimary();
            if ( fExtentList )
            {
                Assert( iext + cextSentinelsRemaining <= cext );
                if ( iext + cextSentinelsRemaining < cext )
                {
                    rgext[iext].pgnoLastInExtent = PgnoFDP( pfucbT ) + psph->CpgPrimary() - 1;
                    rgext[iext].cpgExtent = psph->CpgPrimary();
                    iext++;
                }

                Assert( iext + cextSentinelsRemaining <= cext );
                rgext[iext].pgnoLastInExtent = pgnoNull;
                rgext[iext].cpgExtent = 0;
                iext++;

                Assert( cextSentinelsRemaining > 0 );
                cextSentinelsRemaining--;

                if ( iext == cext )
                {
                    Assert( !FSPAvailExtent( fSPExtents ) );
                    Assert( 0 == cextSentinelsRemaining );
                }
                else
                {
                    Assert( iext < cext );
                    Assert( iext + cextSentinelsRemaining <= cext );
                }
            }
        }

        else
        {
            FUCB    *pfucbOE = pfucbNil;

            Call( ErrSPIOpenOwnExt( ppib, pfucbT->u.pfcb, &pfucbOE ) );

            if( pcprintf )
            {
                (*pcprintf)( "\n%s: OWNEXT\n", SzNameOfTable( pfucbT ) );
            }

            err = ErrSPIGetInfo(
                pfucbOE,
                pcpgOwnExtTotal,
                NULL,
                NULL,
                &iext,
                cext,
                rgext,
                &cextSentinelsRemaining,
                pcprintf );
            Assert( pfucbOE != pfucbNil );
            BTClose( pfucbOE );
            Call( err );
        }
    }

    if ( FSPAvailExtent( fSPExtents ) )
    {
        Assert( NULL != pcpgAvailExtTotal );
        Assert( !fExtentList || 1 == cextSentinelsRemaining );

        if ( FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            SPACE_HEADER    *psph;

            if( pcprintf )
            {
                (*pcprintf)( "\n%s: AVAILEXT: single extent\n", SzNameOfTable( pfucbT ) );
            }

            Assert( pfucbT->pcsrRoot != pcsrNil );
            Assert( pfucbT->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
            Assert( pfucbT->pcsrRoot->FLatched() );
            Assert( !FFUCBSpace( pfucbT ) );

            NDGetExternalHeader( pfucbT, pfucbT->pcsrRoot, noderfSpaceHeader );
            Assert( sizeof( SPACE_HEADER ) == pfucbT->kdfCurr.data.Cb() );
            psph = reinterpret_cast <SPACE_HEADER *> ( pfucbT->kdfCurr.data.Pv() );

            *pcpgAvailExtTotal = 0;

            PGNO    pgnoT           = PgnoFDP( pfucbT ) + 1;
            CPG     cpgPrimarySeen  = 1;
            PGNO    pgnoPrevAvail   = pgnoNull;
            UINT    rgbitT;

            for ( rgbitT = 0x00000001;
                rgbitT != 0 && cpgPrimarySeen < psph->CpgPrimary();
                cpgPrimarySeen++, pgnoT++, rgbitT <<= 1 )
            {
                Assert( pgnoT <= PgnoFDP( pfucbT ) + cpgSmallSpaceAvailMost );

                if ( rgbitT & psph->RgbitAvail() )
                {
                    (*pcpgAvailExtTotal)++;

                    if ( fExtentList )
                    {
                        Assert( iext + cextSentinelsRemaining <= cext );
                        if ( pgnoT == pgnoPrevAvail + 1 )
                        {
                            Assert( iext > 0 );
                            const INT   iextPrev    = iext - 1;
                            Assert( pgnoNull != pgnoPrevAvail );
                            Assert( pgnoPrevAvail == rgext[iextPrev].PgnoLast() );
                            rgext[iextPrev].pgnoLastInExtent = pgnoT;
                            rgext[iextPrev].cpgExtent++;

                            Assert( rgext[iextPrev].PgnoLast() - rgext[iextPrev].CpgExtent()
                                    >= PgnoFDP( pfucbT ) );

                            pgnoPrevAvail = pgnoT;
                        }
                        else if ( iext + cextSentinelsRemaining < cext )
                        {
                            rgext[iext].pgnoLastInExtent = pgnoT;
                            rgext[iext].cpgExtent = 1;
                            iext++;

                            Assert( iext + cextSentinelsRemaining <= cext );

                            pgnoPrevAvail = pgnoT;
                        }
                    }
                }
            }


            if ( psph->CpgPrimary() > cpgSmallSpaceAvailMost + 1 )
            {
                Assert( cpgSmallSpaceAvailMost + 1 == cpgPrimarySeen );
                const CPG   cpgRemaining        = psph->CpgPrimary() - ( cpgSmallSpaceAvailMost + 1 );

                (*pcpgAvailExtTotal) += cpgRemaining;

                if ( fExtentList )
                {
                    Assert( iext + cextSentinelsRemaining <= cext );

                    const PGNO  pgnoLastOfRemaining = PgnoFDP( pfucbT ) + psph->CpgPrimary() - 1;
                    if ( pgnoLastOfRemaining - cpgRemaining == pgnoPrevAvail )
                    {
                        Assert( iext > 0 );
                        const INT   iextPrev    = iext - 1;
                        Assert( pgnoNull != pgnoPrevAvail );
                        Assert( pgnoPrevAvail == rgext[iextPrev].PgnoLast() );
                        rgext[iextPrev].pgnoLastInExtent = pgnoLastOfRemaining;
                        rgext[iextPrev].cpgExtent += cpgRemaining;

                        Assert( rgext[iextPrev].PgnoLast() - rgext[iextPrev].CpgExtent()
                                >= PgnoFDP( pfucbT ) );
                    }
                    else if ( iext + cextSentinelsRemaining < cext )
                    {
                        rgext[iext].pgnoLastInExtent = pgnoLastOfRemaining;
                        rgext[iext].cpgExtent = cpgRemaining;
                        iext++;

                        Assert( iext + cextSentinelsRemaining <= cext );
                    }
                }

            }

            if ( fExtentList )
            {
                Assert( iext < cext );
                rgext[iext].pgnoLastInExtent = pgnoNull;
                rgext[iext].cpgExtent = 0;
                iext++;

                Assert( cextSentinelsRemaining > 0 );
                cextSentinelsRemaining--;

                Assert( iext + cextSentinelsRemaining <= cext );
            }
        }

        else
        {
            FUCB    *pfucbAE = pfucbNil;

            Call( ErrSPIOpenAvailExt( ppib, pfucbT->u.pfcb, &pfucbAE ) );

            if( pcprintf )
            {
                (*pcprintf)( "\n%s: AVAILEXT\n", SzNameOfTable( pfucbT ) );
            }


            FUCB    *pfucbOE = pfucbNil;

            err = ErrSPIOpenOwnExt( ppib, pfucbT->u.pfcb, &pfucbOE );
            if ( err >= JET_errSuccess )
            {
                err = ErrSPIGetSPBufUnlatched( pfucbOE, &spbufOnOE );
            }

            if ( pfucbOE != pfucbNil )
            {
                BTClose( pfucbOE );
            }

            if ( err >= JET_errSuccess )
            {
                err = ErrSPIGetSPBufUnlatched( pfucbAE, &spbufOnAE );
            }

            SPDumpSplitBufExtent( pcprintf, SzNameOfTable( pfucbAE ), "OE", pfucbAE->u.pfcb->PgnoOE(), &spbufOnOE );

            SPDumpSplitBufExtent( pcprintf, SzNameOfTable( pfucbAE ), "AE", pfucbAE->u.pfcb->PgnoAE(), &spbufOnAE );

            if ( err >= JET_errSuccess )
            {
                err = ErrSPIGetInfo(
                    pfucbAE,
                    pcpgAvailExtTotal,
                    pcpgReservedExtTotal,
                    pcpgShelvedExtTotal,
                    &iext,
                    cext,
                    rgext,
                    &cextSentinelsRemaining,
                    pcprintf );


                const CPG cpgSplitBufferReserved = spbufOnOE.CpgBuffer1() +
                                                    spbufOnOE.CpgBuffer2() +
                                                    spbufOnAE.CpgBuffer1() +
                                                    spbufOnAE.CpgBuffer2();

                if ( FSPAvailExtent( fSPExtents ) )
                {
                    Assert( NULL != pcpgAvailExtTotal );
                    *pcpgAvailExtTotal += cpgSplitBufferReserved;
                }
                if ( FSPReservedExtent( fSPExtents ) )
                {
                    Assert( NULL != pcpgReservedExtTotal );
                    *pcpgReservedExtTotal += cpgSplitBufferReserved;
                }
            }

            Assert( pfucbAE != pfucbNil );
            BTClose( pfucbAE );
            Call( err );
        }

        Assert( !FSPOwnedExtent( fSPExtents )
            || ( *pcpgAvailExtTotal <= *pcpgOwnExtTotal ) );
        Assert( !FSPReservedExtent( fSPExtents )
            || ( *pcpgReservedExtTotal <= *pcpgAvailExtTotal ) );
    }

    Assert( 0 == cextSentinelsRemaining );


HandleError:

    Expected( pfucbNil != pfucbT );

    if ( pfucbT != pfucbNil )
    {
        pfucbT->pcsrRoot = pcsrNil;
        BTClose( pfucbT );
    }

    return err;
}


ERR ErrSPGetExtentInfo(
    _Inout_ PIB *                       ppib,
    _In_ const IFMP                     ifmp,
    _Inout_ FUCB *                      pfucb,
    _In_ const ULONG                    fSPExtents,
    _Out_ ULONG *                       pulExtentList,
    _Deref_post_cap_(*pulExtentList) BTREE_SPACE_EXTENT_INFO ** pprgExtentList )
{
    ERR                         err = JET_errSuccess;
    BTREE_SPACE_EXTENT_INFO *   prgext = NULL;
    FUCB *                      pfucbT              = pfucbNil;

    if ( !( FSPOwnedExtent( fSPExtents ) || FSPAvailExtent( fSPExtents ) ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    Assert( ( fSPExtents & ~( fSPOwnedExtent | fSPAvailExtent ) ) == 0 );

    if ( pfucbNil == pfucb )
    {
        err = ErrBTOpen( ppib, pgnoSystemRoot, ifmp, &pfucbT );
    }
    else
    {
        err = ErrBTOpen( ppib, pfucb->u.pfcb, &pfucbT );
    }
    CallR( err );
    Assert( pfucbNil != pfucbT );

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope( );
    tcScope->nParentObjectClass = TceFromFUCB( pfucbT );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucbT ) );
    tcScope->iorReason.SetIort( iortSpace );

    Call( ErrBTIGotoRoot( pfucbT, latchReadTouch ) );
    Assert( pcsrNil == pfucbT->pcsrRoot );
    pfucbT->pcsrRoot = Pcsr( pfucbT );

    if ( !pfucbT->u.pfcb->FSpaceInitialized() )
    {
        SPIInitFCB( pfucbT, fTrue );
        if( !FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            BFPrereadPageRange( pfucbT->ifmp, pfucbT->u.pfcb->PgnoOE(), 2, bfprfDefault, ppib->BfpriPriority( pfucbT->ifmp ), *tcScope );
        }
    }

    ULONG cextMax = 10;
    ULONG cextMac = 0;
    Alloc( prgext = new BTREE_SPACE_EXTENT_INFO[cextMax] );
    memset( prgext, 0, sizeof(BTREE_SPACE_EXTENT_INFO)*cextMax );

    if ( FSPOwnedExtent( fSPExtents ) )
    {
        Assert( !FSPAvailExtent( fSPExtents ) );

        if ( FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            SPACE_HEADER    *psph;

            Assert( pfucbT->pcsrRoot != pcsrNil );
            Assert( pfucbT->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
            Assert( pfucbT->pcsrRoot->FLatched() );
            Assert( !FFUCBSpace( pfucbT ) );

            NDGetExternalHeader( pfucbT, pfucbT->pcsrRoot, noderfSpaceHeader );
            Assert( sizeof( SPACE_HEADER ) == pfucbT->kdfCurr.data.Cb() );
            psph = reinterpret_cast <SPACE_HEADER *> ( pfucbT->kdfCurr.data.Pv() );

            Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::PrimaryExt, PgnoFDP( pfucb ) + psph->CpgPrimary() - 1, psph->CpgPrimary(), PgnoFDP( pfucb ) ) );
        }
        else
        {
            FUCB    *pfucbOE = pfucbNil;

            Call( ErrSPIOpenOwnExt( ppib, pfucbT->u.pfcb, &pfucbOE ) );

            Call( ErrSPIExtGetExtentListInfo( pfucbOE, &prgext, &cextMax, &cextMac ) );

            Assert( pfucbOE != pfucbNil );
            BTClose( pfucbOE );
            Call( err );
        }

        *pulExtentList = cextMac;
        *pprgExtentList = prgext;
    }

    if ( FSPAvailExtent( fSPExtents ) )
    {
        Assert( !FSPOwnedExtent( fSPExtents ) );

        SPLIT_BUFFER                spbufOnOE;
        SPLIT_BUFFER                spbufOnAE;

        memset( (void*)&spbufOnOE, 0, sizeof(spbufOnOE) );
        memset( (void*)&spbufOnAE, 0, sizeof(spbufOnAE) );
        
        if ( FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            SPACE_HEADER    *psph;

            Assert( pfucbT->pcsrRoot != pcsrNil );
            Assert( pfucbT->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
            Assert( pfucbT->pcsrRoot->FLatched() );
            Assert( !FFUCBSpace( pfucbT ) );

            NDGetExternalHeader( pfucbT, pfucbT->pcsrRoot, noderfSpaceHeader );
            Assert( sizeof( SPACE_HEADER ) == pfucbT->kdfCurr.data.Cb() );
            psph = reinterpret_cast <SPACE_HEADER *> ( pfucbT->kdfCurr.data.Pv() );

            PGNO    pgnoT           = PgnoFDP( pfucbT ) + 1;
            CPG     cpgPrimarySeen  = 1;
            UINT    rgbitT;

            CPG     cpgAccum        = 0;
            ULONG   pgnoLastAccum   = 0;

            for ( rgbitT = 0x00000001;
                rgbitT != 0 && cpgPrimarySeen < psph->CpgPrimary();
                cpgPrimarySeen++, pgnoT++, rgbitT <<= 1 )
            {
                Assert( pgnoT <= PgnoFDP( pfucbT ) + cpgSmallSpaceAvailMost );

                if ( rgbitT & psph->RgbitAvail() )
                {
                    Assert( pgnoNull != pgnoT );
                    Assert( cextMac == 0 || prgext[cextMac-1].pgnoLast - prgext[cextMac-1].cpgExtent >= PgnoFDP( pfucbT ) );
                    Assert( cpgAccum < cpgPrimarySeen );
                    Assert( pgnoLastAccum == 0 || pgnoLastAccum + 1 == pgnoT );
                    cpgAccum++;
                    pgnoLastAccum = pgnoT;
                }
                else
                {
                    if ( cpgAccum )
                    {
                        Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::PrimaryExt, pgnoLastAccum, cpgAccum, PgnoFDP( pfucb ) ) );
                    }
                    cpgAccum = 0;
                    pgnoLastAccum = 0;
                }
            }

            if ( psph->CpgPrimary() > cpgSmallSpaceAvailMost + 1 )
            {
                Assert( cpgSmallSpaceAvailMost + 1 == cpgPrimarySeen );
                const CPG   cpgRemaining        = psph->CpgPrimary() - ( cpgSmallSpaceAvailMost + 1 );
                const PGNO  pgnoLastOfRemaining = PgnoFDP( pfucbT ) + psph->CpgPrimary() - 1;
                if ( cextMac > 0 )
                {
                    Assert( pgnoLastAccum == prgext[cextMac-1].pgnoLast );
                    Assert( prgext[cextMac-1].pgnoLast - prgext[cextMac-1].cpgExtent >= PgnoFDP( pfucbT ) );
                }
                Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::PrimaryExt, pgnoLastOfRemaining, cpgRemaining, PgnoFDP( pfucb ) ) );
            }
            else if ( cpgAccum )
            {
                Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::PrimaryExt, pgnoLastAccum, cpgAccum, PgnoFDP( pfucb ) ) );
            }
        }
        else
        {
            FUCB    *pfucbAE = pfucbNil;

            Call( ErrSPIOpenAvailExt( ppib, pfucbT->u.pfcb, &pfucbAE ) );


            FUCB    *pfucbOE = pfucbNil;

            err = ErrSPIOpenOwnExt( ppib, pfucbT->u.pfcb, &pfucbOE );
            if ( err >= JET_errSuccess )
            {
                err = ErrSPIGetSPBufUnlatched( pfucbOE, &spbufOnOE );
            }

            if ( pfucbOE != pfucbNil )
            {
                BTClose( pfucbOE );
            }

            if ( err >= JET_errSuccess )
            {
                err = ErrSPIGetSPBufUnlatched( pfucbAE, &spbufOnAE );
            }


            if ( spbufOnOE.CpgBuffer1() )
            {
                Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::OwnedTreeAvail, spbufOnOE.PgnoLastBuffer1(), spbufOnOE.CpgBuffer1(), pfucbAE->u.pfcb->PgnoOE() ) );
            }
            if ( spbufOnOE.CpgBuffer2() )
            {
                Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::OwnedTreeAvail, spbufOnOE.PgnoLastBuffer2(), spbufOnOE.CpgBuffer2(), pfucbAE->u.pfcb->PgnoOE() ) );
            }
            if ( spbufOnAE.CpgBuffer1() )
            {
                Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::AvailTreeAvail, spbufOnAE.PgnoLastBuffer1(), spbufOnAE.CpgBuffer1(), pfucbAE->u.pfcb->PgnoAE() ) );
            }
            if ( spbufOnAE.CpgBuffer2() )
            {
                Call( ErrSPIAddExtentInfo( &prgext, &cextMax, &cextMac, spp::AvailTreeAvail, spbufOnAE.PgnoLastBuffer2(), spbufOnAE.CpgBuffer2(), pfucbAE->u.pfcb->PgnoAE() ) );
            }


            if ( err >= JET_errSuccess )
            {
                Call( ErrSPIExtGetExtentListInfo( pfucbAE, &prgext, &cextMax, &cextMac ) );
            }

            Assert( pfucbAE != pfucbNil );
            BTClose( pfucbAE );
            Call( err );

            for( ULONG iext = 0; iext < cextMac; iext++ )
            {
                BTREE_SPACE_EXTENT_INFO * pext = &prgext[iext];
                Assert( pext->iPool != (ULONG)spp::OwnedTreeAvail || iext <= 4 );
                Assert( pext->iPool != (ULONG)spp::AvailTreeAvail || iext <= 4 );
                if ( pext->iPool == (ULONG)spp::PrimaryExt )
                {
                    Assert( iext == 0 || prgext[iext-1].iPool != (ULONG)spp::AvailExtLegacyGeneralPool );
                    Assert( iext == 0 || prgext[iext-1].iPool != (ULONG)spp::ContinuousPool );
                    Assert( prgext[iext-1].iPool != (ULONG)spp::ShelvedPool );
                }
            }
        }

        *pulExtentList = cextMac;
        *pprgExtentList = prgext;
    }

HandleError:

    Expected( pfucbNil != pfucbT );

    if ( pfucbT != pfucbNil )
    {
        pfucbT->pcsrRoot = pcsrNil;
        BTClose( pfucbT );
    }

    return err;
}



LOCAL ERR ErrSPITrimOneAvailableExtent(
    _In_ FUCB* pfucb,
    _Out_ CPG* pcpgTotalAvail,
    _Out_ CPG* pcpgTotalAvailSparseBefore,
    _Out_ CPG* pcpgTotalAvailSparseAfter,
    _In_ CPRINTF* const pcprintf )
{
    ERR         err;
    DIB         dib;

    Assert( pcprintf );

    CPG         cpgAvailTotal   = 0;
    CPG         cpgAvailSparseBefore    = 0;
    CPG         cpgAvailSparseAfter     = 0;

    *pcpgTotalAvail = 0;
    *pcpgTotalAvailSparseBefore = 0;
    *pcpgTotalAvailSparseAfter = 0;

    FUCBSetSequential( pfucb );
    FUCBSetPrereadForward( pfucb, cpgPrereadSequential );

    dib.dirflag = fDIRNull;
    dib.pos = posFirst;
    Assert( FFUCBSpace( pfucb ) );
    err = ErrBTDown( pfucb, &dib, latchReadNoTouch );

    if ( err != JET_errRecordNotFound )
    {
        Call( err );

        forever
        {
            const CSPExtentInfo cspext( pfucb );

            Expected( ( cspext.SppPool() == spp::AvailExtLegacyGeneralPool ) || ( cspext.SppPool() == spp::ShelvedPool ) );

            if ( cspext.SppPool() != spp::ShelvedPool )
            {
                CPG cpgSparseBeforeThisExtent = 0;
                CPG cpgSparseAfterThisExtent = 0;
                QWORD ibStartZeroes = 0;
                QWORD cbZeroLength = 0;
                PGNO pgnoStartRegionAligned = 0;
                CPG cpgRegionAligned = 0;

                if ( !cspext.FEmptyExtent() )
                {
                    Call( ErrIOTrimNormalizeOffsetsAndPgnos(
                        pfucb->ifmp,
                        cspext.PgnoFirst(),
                        cspext.CpgExtent(),
                        &ibStartZeroes,
                        &cbZeroLength,
                        &pgnoStartRegionAligned,
                        &cpgRegionAligned ) );
                }

                if ( cpgRegionAligned != 0 )
                {
                    Assert( !cspext.FEmptyExtent() );

                    const PGNO pgnoEndRegionAligned = cspext.FEmptyExtent() ? 0 : pgnoStartRegionAligned + cpgRegionAligned - 1;

                    if ( !cspext.FEmptyExtent() )
                    {
                    }

                    (*pcprintf)( "%30s: %s[%5d]:\t%6d-%6d (%3d);\tA%6d-%6d (%3d) %s%s",
                                 SzNameOfTable( pfucb ),
                                 SzSpaceTreeType( pfucb ),
                                 Pcsr( pfucb )->Pgno(),
                                 cspext.FEmptyExtent() ? 0 : cspext.PgnoFirst(),
                                 cspext.FEmptyExtent() ? cspext.PgnoMarker() : cspext.PgnoLast(),
                                 cspext.CpgExtent(),
                                 pgnoStartRegionAligned,
                                 pgnoEndRegionAligned,
                                 cpgRegionAligned,
                                 FNDDeleted( pfucb->kdfCurr ) ? " (DEL)" : "",
                                 ( cspext.ErrCheckCorrupted( ) < JET_errSuccess ) ? " (COR)" : ""
                                 );

                    (*pcprintf)( " cpgSparseBefore: %3d", cpgSparseBeforeThisExtent );

                {
                    PIB pibFake;
                    pibFake.m_pinst = g_rgfmp[ pfucb->ifmp ].Pinst();
                    Call( ErrSPITrimRegion( pfucb->ifmp, &pibFake, pgnoEndRegionAligned, cpgRegionAligned, &cpgSparseBeforeThisExtent, &cpgSparseAfterThisExtent ) );

                    const CPG cpgSparseNewlyTrimmed = cpgSparseAfterThisExtent - cpgSparseBeforeThisExtent;
                    (*pcprintf)( " cpgSparseAfter: %3d cpgSparseNew: %3d", cpgSparseAfterThisExtent, cpgSparseNewlyTrimmed );

                    cpgAvailSparseBefore += cpgSparseBeforeThisExtent;
                    cpgAvailSparseAfter += cpgSparseAfterThisExtent;
                }

                    (*pcprintf)( "\n" );
                }

                cpgAvailTotal += cspext.CpgExtent();
            }

            err = ErrBTNext( pfucb, fDIRNull );
            if ( err < 0 )
            {
                if ( err != JET_errNoCurrentRecord )
                {
                    goto HandleError;
                }
                break;
            }
        }
    }

    *pcpgTotalAvail += cpgAvailTotal;
    *pcpgTotalAvailSparseBefore += cpgAvailSparseBefore;
    *pcpgTotalAvailSparseAfter += cpgAvailSparseAfter;

    if( pcprintf )
    {
        const CPG cpgSparseNewlyTrimmed = cpgAvailSparseAfter - cpgAvailSparseBefore;
        (*pcprintf)( "%s:\t%d trimmed (before), %d trimmed (after), %d pages newly trimmed.\n",
                     SzNameOfTable( pfucb ),
                     cpgAvailSparseBefore,
                     cpgAvailSparseAfter,
                     cpgSparseNewlyTrimmed );
    }

    err = JET_errSuccess;

HandleError:
    return err;
}

ERR ErrSPTrimRootAvail(
    _In_ PIB *ppib,
    _In_ const IFMP ifmp,
    _In_ CPRINTF * const pcprintf,
    _Out_opt_ CPG * const pcpgTrimmed )
{
    ERR err;
    CPG cpgAvailExtTotal = 0;
    CPG cpgAvailExtTotalSparseBefore = 0;
    CPG cpgAvailExtTotalSparseAfter = 0;
    FUCB *pfucbT = pfucbNil;
    FUCB *pfucbAE = pfucbNil;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortSpace );

    SPLIT_BUFFER    spbufOnAE;

    memset( (void*)&spbufOnAE, 0, sizeof(spbufOnAE) );

    Call( ErrBTIOpenAndGotoRoot( ppib, pgnoSystemRoot, ifmp, &pfucbT ) );
    AssertSPIPfucbOnRoot( pfucbT );

    if ( !pfucbT->u.pfcb->FSpaceInitialized() )
    {
        SPIInitFCB( pfucbT, fTrue );
        if( !FSPIIsSmall( pfucbT->u.pfcb ) )
        {
            BFPrereadPageRange( pfucbT->ifmp, pfucbT->u.pfcb->PgnoAE(), 2, bfprfDefault, ppib->BfpriPriority( pfucbT->ifmp ), *tcScope );
        }
    }

    AssertSz( !FSPIIsSmall( pfucbT->u.pfcb ), "Database root table should not be Small Space!" );

    Call( ErrSPIOpenAvailExt( ppib, pfucbT->u.pfcb, &pfucbAE ) );

    Call( ErrSPIGetSPBufUnlatched( pfucbAE, &spbufOnAE ) );

    Call( ErrSPITrimOneAvailableExtent(
        pfucbAE,
        &cpgAvailExtTotal,
        &cpgAvailExtTotalSparseBefore,
        &cpgAvailExtTotalSparseAfter,
        pcprintf ) );

    Assert( cpgAvailExtTotalSparseBefore <= cpgAvailExtTotal );
    Assert( cpgAvailExtTotalSparseBefore <= cpgAvailExtTotalSparseAfter );
    Assert( cpgAvailExtTotalSparseAfter <= cpgAvailExtTotal );

HandleError:
    if ( pfucbAE != pfucbNil )
    {
        BTClose( pfucbAE );
    }

    Assert( pfucbNil != pfucbT || err < JET_errSuccess );

    if ( pfucbT != pfucbNil )
    {
        pfucbT->pcsrRoot = pcsrNil;
        BTClose( pfucbT );
    }

    if ( pcpgTrimmed != NULL )
    {
        *pcpgTrimmed = 0;
        if ( ( err >= JET_errSuccess ) &&
            ( cpgAvailExtTotalSparseBefore > cpgAvailExtTotalSparseAfter ) )
        {
            *pcpgTrimmed = cpgAvailExtTotalSparseBefore - cpgAvailExtTotalSparseAfter;
        }
    }

    return err;
}


ERR ErrSPDummyUpdate( FUCB * pfucb )
{
    ERR             err;
    SPACE_HEADER    sph;
    DATA            data;

    Assert( !Pcsr( pfucb )->FLatched() );

    CallR( ErrBTIGotoRoot( pfucb, latchRIW ) );

    NDGetExternalHeader( pfucb, noderfSpaceHeader );
    Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );

    UtilMemCpy( &sph, pfucb->kdfCurr.data.Pv(), sizeof(sph) );

    Pcsr( pfucb )->UpgradeFromRIWLatch();

    data.SetPv( &sph );
    data.SetCb( sizeof(sph) );
    err = ErrNDSetExternalHeader( pfucb, &data, fDIRNull, noderfSpaceHeader );

    BTUp( pfucb );

    return err;
}


#ifdef SPACECHECK

LOCAL ERR ErrSPIValidFDP( PIB *ppib, IFMP ifmp, PGNO pgnoFDP )
{
    ERR         err;
    FUCB        *pfucb = pfucbNil;
    FUCB        *pfucbOE = pfucbNil;
    DIB         dib;

    Assert( pgnoFDP != pgnoNull );

    Call( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb ) );
    Assert( pfucbNil != pfucb );

    Assert( pfucb->u.pfcb->FInitialized() );

    if ( !FSPIIsSmall( pfucb->u.pfcb ) )
    {
        Call( ErrSPIOpenOwnExt( ppib, pfucb->u.pfcb, &pfucbOE ) );

        const CSPExtentKeyBM    spFindOwnedExtOfFDP( SPEXTKEY::fSPExtentTypeOE, pgnoFDP );
        dib.pos = posDown;
        dib.pbm = spFindOwnedExtOfFDP.Pbm( pfucbOE ) ;
        dib.dirflag = fDIRNull;
        Call( ErrBTDown( pfucbOE, &dib, latchReadTouch ) );
        if ( err == wrnNDFoundGreater )
        {
            Call( ErrBTGet( pfucbOE ) );
        }

        const CSPExtentInfo     spOwnedExtOfFDP( pfucbOE );

        Assert( spOwnedExtOfFDP.FValidExtent() );
        Assert( pgnoFDP == spOwnedExtOfFDP.PgnoFirst() );
    }

HandleError:
    if ( pfucbOE != pfucbNil )
        BTClose( pfucbOE );
    if ( pfucb != pfucbNil )
        BTClose( pfucb );
    return err;
}


LOCAL ERR ErrSPIWasAlloc(
    FUCB    *pfucb,
    PGNO    pgnoFirst,
    CPG     cpgSize )
{
    ERR     err;
    FUCB    *pfucbOE = pfucbNil;
    FUCB    *pfucbAE = pfucbNil;
    DIB     dib;
    CSPExtentKeyBM      spFindExt;
    CSPExtentInfo       spExtInfo;
    const PGNO          pgnoLast = pgnoFirst + cpgSize - 1;

    if ( FSPIIsSmall( pfucb->u.pfcb ) )
    {
        SPACE_HEADER    *psph;
        UINT            rgbitT;
        INT             iT;

        AssertSPIPfucbOnRoot( pfucb );

        NDGetExternalHeader( pfucb, pfucb->pcsrRoot, noderfSpaceHeader );
        Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
        psph = reinterpret_cast <SPACE_HEADER *> ( pfucb->kdfCurr.data.Pv() );
        for ( rgbitT = 1, iT = 1;
            iT < cpgSize && iT < cpgSmallSpaceAvailMost;
            iT++ )
            rgbitT = (rgbitT<<1) + 1;
        Assert( pgnoFirst - PgnoFDP( pfucb ) < cpgSmallSpaceAvailMost );
        if ( pgnoFirst != PgnoFDP( pfucb ) )
        {
            rgbitT <<= (pgnoFirst - PgnoFDP( pfucb ) - 1);
            Assert( ( psph->RgbitAvail() & rgbitT ) == 0 );
        }

        goto HandleError;
    }

    Call( ErrSPIOpenOwnExt( pfucb->ppib, pfucb->u.pfcb, &pfucbOE ) );

    spFindExt.SPExtentMakeKeyBM( SPEXTKEY::fSPExtentTypeOE, pgnoLast );
    dib.pos = posDown;
    dib.pbm = spFindExt.Pbm( pfucbOE );
    dib.dirflag = fDIRNull;
    Call( ErrBTDown( pfucbOE, &dib, latchReadNoTouch ) );
    if ( err == wrnNDFoundLess )
    {
        Assert( fFalse );
        Assert( Pcsr( pfucbOE )->Cpage().Clines() - 1 ==
                    Pcsr( pfucbOE )->ILine() );
        Assert( pgnoNull != Pcsr( pfucbOE )->Cpage().PgnoNext() );

        Call( ErrBTNext( pfucbOE, fDIRNull ) );
        err = ErrERRCheck( wrnNDFoundGreater );

        #ifdef DEBUG
        const CSPExtentInfo spExt( pfucbOE );
        Assert( spExt.PgnoLast() > pgnoLast );
        #endif
    }

    if ( err == wrnNDFoundGreater )
    {
        Call( ErrBTGet( pfucbOE ) );
    }

    spExtInfo.Set( pfucbOE );
    Assert( pgnoFirst >= spExtInfo.PgnoFirst() );
    BTUp( pfucbOE );


    EnforceSz( fFalse, "SpaceCheckNeedsFixingForPools" );

    spFindExt.SPExtentMakeKeyBM( SPEXTKEY::fSPExtentTypeAE, pgnoLast );
    dib.pos = posDown;
    dib.pbm = spFindExt.Pbm( pfucbOE );
    dib.dirflag = fDIRNull;
    Call( ErrSPIOpenAvailExt( pfucb->ppib, pfucb->u.pfcb, &pfucbAE ) );
    if ( ( err = ErrBTDown( pfucbAE, &dib, latchReadNoTouch ) ) < 0 )
    {
        if ( err == JET_errNoCurrentRecord )
        {
            err = JET_errSuccess;
            goto HandleError;
        }
        goto HandleError;
    }

    Assert( err != JET_errSuccess );

    if ( err == wrnNDFoundGreater )
    {
        Call( ErrBTNext( pfucbAE, fDIRNull ) );
        const CSPExtentInfo     spavailextNext( pfucbAE );
        Assert( spavailextNext.CpgExtent() != 0 );
        Assert( pgnoLast < spavailextNext.PgnoFirst() );
    }
    else
    {
        Assert( err == wrnNDFoundLess );
        Call( ErrBTPrev( pfucbAE, fDIRNull ) );
        const CSPExtentInfo     spavailextPrev( pfucbAE );

        Assert( pgnoFirst > spavailextPrev.PgnoLast() );
    }

HandleError:
    if ( pfucbOE != pfucbNil )
        BTClose( pfucbOE );
    if ( pfucbAE != pfucbNil )
        BTClose( pfucbAE );

    return JET_errSuccess;
}

#endif

const ULONG pctDoublingGrowth  = 200;

CPG CpgSPIGetNextAlloc(
    __in const FCB_SPACE_HINTS * const  pfcbsh,
    __in const CPG                      cpgPrevious
    )
{

    if ( pfcbsh->m_cpgMaxExtent != 0 &&
            pfcbsh->m_cpgMinExtent == pfcbsh->m_cpgMaxExtent )
    {
        return pfcbsh->m_cpgMaxExtent;
    }

    CPG cpgNextExtent = cpgPrevious;

    if ( pfcbsh->m_pctGrowth )
    {
        cpgNextExtent = max( cpgPrevious + 1, ( cpgPrevious * pfcbsh->m_pctGrowth ) / 100 );
        Assert( cpgNextExtent != cpgPrevious );
    }

    cpgNextExtent = (CPG)UlBound( (ULONG)cpgNextExtent,
                                (ULONG)( pfcbsh->m_cpgMinExtent ? pfcbsh->m_cpgMinExtent : cpgNextExtent ),
                                (ULONG)( pfcbsh->m_cpgMaxExtent ? pfcbsh->m_cpgMaxExtent : cpgNextExtent ) );

    Assert( cpgNextExtent >= 0 );
    return cpgNextExtent;
}

#ifdef SPACECHECK
void SPCheckPrintMaxNumAllocs( const LONG cbPageSize )
{
    JET_SPACEHINTS jsph = {
        sizeof(JET_SPACEHINTS),
        80,
        0 ,
        0x0, 0x0, 0x0, 0x0, 0x0,
        80,
        0 ,
        0,
        1 * 1024 * 1024 * 1024
    };
    FCB_SPACE_HINTS fcbshCheck = { 0 };

    ULONG iLastAllocs = 0;

    ULONG pct = 200;
    for( ULONG cpg = 1; cpg < (ULONG) ( 16 * 1024 * 1024 / cbPageSize ); cpg++ )
    {

        jsph.cbInitial = cpg * cbPageSize;
        for( ; pct >= 100; pct-- )
        {
            ULONG cpgNext = ( ( jsph.cbInitial / cbPageSize ) * ( pct ) ) / 100;
            if ( cpgNext == ( jsph.cbInitial / cbPageSize ) )
            {
                pct++;
                break;
            }

        }
        jsph.ulGrowth = pct;

        fcbshCheck.SetSpaceHints( &jsph, cbPageSize );

        CPG cpgLast = fcbshCheck.m_cpgInitial;
        ULONG iAlloc = 1;
        for( ; iAlloc < (1024 * 1024 * 1024 + 1) ; )
        {
            iAlloc++;
            cpgLast = CpgSPIGetNextAlloc( &fcbshCheck, cpgLast );
            if ( cpgLast >= fcbshCheck.m_cpgMaxExtent )
            {
                break;
            }
        }
        wprintf(L"\tCpgInit: %09d Min:%03d%% -> %d iAllocs\n", (ULONG)fcbshCheck.m_cpgInitial, (ULONG)fcbshCheck.m_pctGrowth, iAlloc );

        if( fcbshCheck.m_cpgInitial > 104 )
        {
            wprintf( L" reached the turning point, bailing.\n" );
            break;
        }
        iLastAllocs = iAlloc;
    }

}

#endif



#if ( ESENT || !DEBUG )
static const TICK               g_dtickTrimDBPeriod             = 1000 * 60 * 60 * 24;
#else
static const TICK               g_dtickTrimDBPeriod             = 1 * 1000;
#endif
    
static TICK             g_tickLastPeriodicTrimDB        = 0;
static BOOL             g_fSPTrimDBTaskScheduled        = fFalse;


LOCAL ERR ErrSPITrimDBITaskPerIFMP( const IFMP ifmp )
{
    ERR err = JET_errSuccess;
    PIB* ppib                   = ppibNil;
    WCHAR wsz[32];
    const WCHAR* rgcwsz[]       = { wsz };
    CPG cpgTrimmed              = 0;
#if DEBUG
    static BOOL s_fLoggedStartEvent = fFalse;
    static BOOL s_fLoggedStopNoActionEvent = fFalse;
#endif

    OSTraceFMP( ifmp, JET_tracetagSpaceInternal, __FUNCTION__ );

    OSTraceFMP( ifmp, JET_tracetagSpaceInternal,
                OSFormat( "Running trim task for ifmp %d.", ifmp ) );

    AssertSz( g_fPeriodicTrimEnabled, "Trim Task should never be run if the test hook wasn't enabled." );
    AssertSz( !g_fRepair, "Trim Task should never be run during Repair." );
    AssertSz( !g_rgfmp[ ifmp ].FReadOnlyAttach(), "Trim Task should not be run on a read-only database." );
    AssertSz( !FFMPIsTempDB( ifmp ), "Trim task should not be run on temp databases." );

    if ( PinstFromIfmp( ifmp )->FRecovering() )
    {
        OSTraceFMP( ifmp, JET_tracetagSpaceInternal,
                    OSFormat( "Periodic trim will not run an in-recovery database." ) );

        err = JET_errSuccess;
        goto HandleError;
    }

    const ULONG dbstate = g_rgfmp[ ifmp ].Pdbfilehdr()->Dbstate();
    if ( ( dbstate == JET_dbstateIncrementalReseedInProgress ) ||
        ( dbstate == JET_dbstateDirtyAndPatchedShutdown ) || 
        ( dbstate == JET_dbstateRevertInProgress ) )
    {
        AssertSz( fFalse, "Periodic trim should not take place during incremental reseed or revert." );
        OSTraceFMP( ifmp, JET_tracetagSpaceInternal,
                    OSFormat( "Periodic trim will not run for a database in inc reseed or revert or dirty and patched shutdown status." ) );

        err = JET_errSuccess;
        goto HandleError;
    }

#if DEBUG
    if ( !s_fLoggedStartEvent )
#endif
    {
        UtilReportEvent(
            eventInformation,
            GENERAL_CATEGORY,
            DB_TRIM_TASK_STARTED,
            0,
            NULL,
            0,
            NULL,
            PinstFromIfmp( ifmp ) );

        OnDebug( s_fLoggedStartEvent = fTrue );
    }

    if ( PinstFromIfmp( ifmp )->m_pbackup->FBKBackupInProgress() )
    {
        OSTraceFMP( ifmp, JET_tracetagSpaceInternal,
                    OSFormat( "Periodic trim will not run when there is backup in progress." ) );

        Error( ErrERRCheck( JET_errBackupInProgress ) );
    }

    Call( ErrPIBBeginSession( PinstFromIfmp( ifmp ), &ppib, procidNil, fFalse ) );

    if ( ( ( GrbitParam( g_rgfmp[ ifmp ].Pinst(), JET_paramEnableShrinkDatabase ) & JET_bitShrinkDatabaseRealtime ) != 0 ) &&
         g_rgfmp[ ifmp ].FTrimSupported() )
    {
        Call( ErrSPTrimRootAvail( ppib, ifmp, CPRINTFNULL::PcprintfInstance(), &cpgTrimmed ) );
    }

HandleError:

    OSTraceFMP( ifmp, JET_tracetagSpaceInternal,
                OSFormat( "Trim task for ifmp %d finished with result %d (0x%x).", ifmp, err, err ) );

    if ( ppibNil != ppib )
    {
        PIBEndSession( ppib );
    }

    if ( err < JET_errSuccess )
    {
        OSStrCbFormatW( wsz, sizeof(wsz), L"%d", err );

        UtilReportEvent(
            eventError,
            GENERAL_CATEGORY,
            DB_TRIM_TASK_FAILED,
            _countof(rgcwsz),
            rgcwsz,
            0,
            NULL,
            PinstFromIfmp( ifmp ) );
    }
    else
    {
        if ( cpgTrimmed > 0 )
        {
            OSStrCbFormatW( wsz, sizeof(wsz), L"%d", cpgTrimmed );

            UtilReportEvent(
                eventInformation,
                GENERAL_CATEGORY,
                DB_TRIM_TASK_SUCCEEDED,
                _countof(rgcwsz),
                rgcwsz,
                0,
                NULL,
                PinstFromIfmp( ifmp ) );
        }
        else
        {
#if DEBUG
            if ( !s_fLoggedStopNoActionEvent )
#endif
            {
                UtilReportEvent(
                    eventInformation,
                    GENERAL_CATEGORY,
                    DB_TRIM_TASK_NO_TRIM,
                    0,
                    NULL,
                    0,
                    NULL,
                    PinstFromIfmp( ifmp ) );

                OnDebug( s_fLoggedStopNoActionEvent = fTrue );
            }
        }
    }

    return err;
}

LOCAL VOID SPITrimDBITask( VOID*, VOID* )
{
    BOOL fReschedule = fFalse;

    Assert( g_fSPTrimDBTaskScheduled );

    OSTrace( JET_tracetagSpaceInternal, __FUNCTION__ );
    
    g_tickLastPeriodicTrimDB = TickOSTimeCurrent();

    if ( g_fRepair )
    {
        OSTrace( JET_tracetagSpaceInternal,
                    OSFormat( "We're in repair - JetDBUtilities is being executed. Aborting trim." ) );

        goto FinishTask;
    }

    FMP::EnterFMPPoolAsWriter();

    for ( IFMP ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
    {
        if ( FFMPIsTempDB( ifmp ) || !g_rgfmp[ifmp].FInUse() )
        {
            continue;
        }

        g_rgfmp[ ifmp ].GetPeriodicTrimmingDBLatch();

        FMP::LeaveFMPPoolAsWriter();

        if ( !g_rgfmp[ ifmp ].FScheduledPeriodicTrim() || g_rgfmp[ ifmp ].FDontStartTrimTask() )
        {
            OSTraceFMP( ifmp, JET_tracetagSpaceInternal,
                        OSFormat( "Periodic trimming is off for this IFMP. Skipping." ) );

            g_rgfmp[ ifmp ].ReleasePeriodicTrimmingDBLatch();

            FMP::EnterFMPPoolAsWriter();

            continue;
        }

        fReschedule = fTrue;

        (void)ErrSPITrimDBITaskPerIFMP( ifmp );
        g_rgfmp[ ifmp ].ReleasePeriodicTrimmingDBLatch();

        FMP::EnterFMPPoolAsWriter();
    }

    FMP::LeaveFMPPoolAsWriter();

FinishTask:
    if ( fReschedule )
    {
        OSTimerTaskScheduleTask( g_posttSPITrimDBITask, NULL, g_dtickTrimDBPeriod, g_dtickTrimDBPeriod );
    }
    else
    {
        g_semSPTrimDBScheduleCancel.Acquire();
        g_fSPTrimDBTaskScheduled = fFalse;
        g_semSPTrimDBScheduleCancel.Release();
    }

    Assert( !FMP::FWriterFMPPool() );
}

VOID SPTrimDBTaskStop( INST * pinst, const WCHAR * cwszDatabaseFullName )
{
    Assert( g_fPeriodicTrimEnabled );

    FMP::EnterFMPPoolAsWriter();
    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        const IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax ||
            !g_rgfmp[ifmp].FInUse() )
        {
            continue;
        }

        if ( NULL != cwszDatabaseFullName &&
            ( g_rgfmp[ifmp].Pinst() != pinst || UtilCmpFileName( cwszDatabaseFullName, g_rgfmp[ifmp].WszDatabaseName() ) != 0 ) )
        {
            continue;
        }

        FMP::LeaveFMPPoolAsWriter();

        g_rgfmp[ifmp].GetPeriodicTrimmingDBLatch();
        g_rgfmp[ifmp].SetFDontStartTrimTask();
        g_rgfmp[ifmp].ResetScheduledPeriodicTrim();
        g_rgfmp[ifmp].ReleasePeriodicTrimmingDBLatch();

        FMP::EnterFMPPoolAsWriter();
    }

    FMP::LeaveFMPPoolAsWriter();
}

ERR ErrSPTrimDBTaskInit( const IFMP ifmp )
{
    ERR err = JET_errSuccess;

    Assert( g_fPeriodicTrimEnabled );

    if ( g_rgfmp[ ifmp ].FReadOnlyAttach() )
    {
        AssertSz( fFalse, "Periodic trim should not be set up for read-only databases." );
        Error( ErrERRCheck( JET_errDatabaseFileReadOnly ) );
    }

    if ( g_fRepair || FFMPIsTempDB( ifmp ) )
    {
        AssertSz( fFalse, "Periodic trim should not be set up for a database in repair or for the temp db." );
        Error( ErrERRCheck( JET_errInvalidDatabaseId ) );
    }
    
    if ( ( GrbitParam( g_rgfmp[ ifmp ].Pinst(), JET_paramEnableShrinkDatabase ) & JET_bitShrinkDatabasePeriodically ) == 0 )
    {
        OSTraceFMP( ifmp, JET_tracetagSpaceInternal,
                    OSFormat( "Periodic trim is not enabled. Will not schedule the trim task." ) );

        err = JET_errSuccess;
        goto HandleError;
    }

    if ( BoolParam( JET_paramEnableViewCache ) )
    {
        OSTraceFMP( ifmp, JET_tracetagSpaceInternal,
                    OSFormat( "Periodic trimming cannot be enabled for view cache. Will not schedule the trimming task." ) );

        err = JET_errSuccess;
        goto HandleError;
    }

#ifdef MINIMAL_FUNCTIONALITY
#else

    g_rgfmp[ ifmp ].GetPeriodicTrimmingDBLatch();

    if ( g_rgfmp[ ifmp ].FDontStartTrimTask() )
    {
        OSTraceFMP( ifmp, JET_tracetagSpaceInternal,
                    OSFormat( "Periodic trimming is set so it should not start. Will not schedule the trim task." ) );

        err = JET_errSuccess;
        g_rgfmp[ ifmp ].ReleasePeriodicTrimmingDBLatch();

        goto HandleError;
    }

    g_rgfmp[ ifmp ].SetScheduledPeriodicTrim();

    g_rgfmp[ ifmp ].ReleasePeriodicTrimmingDBLatch();

    g_semSPTrimDBScheduleCancel.Acquire();
    Assert( g_semSPTrimDBScheduleCancel.CAvail() == 0 );
    if ( !g_fSPTrimDBTaskScheduled )
    {
        g_fSPTrimDBTaskScheduled = fTrue;
        OSTimerTaskScheduleTask( g_posttSPITrimDBITask, NULL, g_dtickTrimDBPeriod, g_dtickTrimDBPeriod );
    }

    Assert( g_semSPTrimDBScheduleCancel.CAvail() == 0 );
    g_semSPTrimDBScheduleCancel.Release();
#endif

HandleError:

    return err;
}

