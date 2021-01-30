// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef DEBUG
#endif

enum LATCH : BYTE
    {   latchNone,
        latchReadTouch,
        latchReadNoTouch,
        latchRIW,
        latchWrite,
        latchWAR,
};

enum PAGETRIM : BYTE
{
    pagetrimNormal,
    pagetrimTrimmed,
    pagetrimMax,
};

class CSR
{
    public:
        CSR( );
        ~CSR( );
        VOID operator=  ( CSR& );

    private:
        DBTIME      m_dbtimeSeen;
        PGNO        m_pgno;
        SHORT       m_iline;
        LATCH       m_latch;
        PAGETRIM    m_pagetrimState;
        CPAGE       m_cpage;

    public:
        DBTIME  Dbtime( )       const;
        VOID    SetDbtime( const DBTIME dbtime );
        VOID    RevertDbtime( const DBTIME dbtime, const ULONG fFlags );
        VOID    RestoreDbtime( const DBTIME dbtime );
        BOOL    FLatched( )     const;
        LATCH   Latch( )        const;
        PGNO    Pgno( )         const;
        INT     ILine( )        const;
        VOID    SetILine( const INT iline );
        VOID    IncrementILine();
        VOID    DecrementILine();
        CPAGE&  Cpage( );
        const CPAGE& Cpage( ) const;
        PAGETRIM    PagetrimState() const;
        VOID    SetPagetrimState( _In_ const PAGETRIM pagetrimState, _In_ const PGNO pgno );

#ifdef DEBUG
        BOOL FDirty()   const;
        VOID AssertValid    () const;
#endif

    public:
        ERR     ErrSwitchPage(
                PIB                 *ppib,
                const IFMP          ifmp,
                const PGNO          pgno,
                const BOOL          fTossImmediate = fFalse );
        ERR     ErrSwitchPageNoWait(
                PIB                 *ppib,
                const IFMP          ifmp,
                const PGNO          pgno );
        ERR     ErrGetPage(
                PIB                 *ppib,
                const IFMP          ifmp,
                const PGNO          pgno,
                const LATCH         latch,
                BFLatch* const      pbflHint = NULL,
                const BOOL fUninitPageOk = fFalse );
        ERR     ErrGetReadPage(
                PIB                 *ppib,
                const IFMP          ifmp,
                const PGNO          pgno,
                const BFLatchFlags  bflf,
                BFLatch* const      pbflHint = NULL );
        ERR     ErrGetRIWPage(
                PIB                 *ppib,
                const IFMP          ifmp,
                const PGNO          pgno );
        ERR     ErrGetRIWPage(
                PIB                 *ppib,
                const IFMP          ifmp,
                const PGNO          pgno,
                const BFLatchFlags  bflf );
        ERR     ErrLoadPage(
                PIB                 * const ppib,
                const IFMP          ifmp,
                const PGNO          pgno,
                const VOID          * const pvPage,
                const ULONG         cbPage,
                LATCH               latch );

        ERR     ErrGetNewPreInitPage(
                PIB                 *ppib,
                const IFMP          ifmp,
                const PGNO          pgno,
                const OBJID         objidFDP,
                const BOOL          fLogNewPage );
        ERR     ErrGetNewPreInitPageForRedo(
                PIB                 * const ppib,
                const IFMP          ifmp,
                const PGNO          pgno,
                const OBJID         objidFDP,
                const DBTIME        dbtimeOperToRedo );
        VOID    ConsumePreInitPage( const ULONG fPageFlags );
        VOID    FinalizePreInitPage( OnDebug( const BOOL fCheckPreInitPage = fFalse ) );
                
    private:
        VOID    OverrideDbtime_(
                const DBTIME        dbtime,
                const ULONG         fFlags );
        
        ERR     ErrSwitchPage_(
                PIB                 *ppib,
                const IFMP          ifmp,
                const PGNO          pgnoNext,
                const BFLatchFlags  bflf,
                const BOOL          fTossImmediate );
                
        ERR     ErrSwitchPageNoWait_(
                PIB                 *ppib,
                const IFMP          ifmp,
                const PGNO          pgno,
                const BFLatchFlags  bflf );
                
        ERR     ErrGetReadPage_(
                PIB                 *ppib,
                const IFMP          ifmp,
                const PGNO          pgno,
                const BFLatchFlags  bflf,
                BFLatch* const      pbflHint = NULL );
                
        ERR     ErrGetRIWPage_(
                PIB                 *ppib,
                const IFMP          ifmp,
                const PGNO          pgno,
                const BFLatchFlags  bflf,
                BFLatch* const      pbflHint = NULL );
    
        ERR     ErrGetNewPreInitPage_(
                PIB                 *ppib,
                const IFMP          ifmp,
                const PGNO          pgno,
                const OBJID         objidFDP,
                const DBTIME        dbtimeNoLog,
                const BOOL          fLogNewPage );


    public:
        ERR     ErrUpgradeFromReadLatch( );
        VOID    UpgradeFromRIWLatch( );
        ERR     ErrUpgrade();

        VOID    Downgrade( LATCH latch );

        ERR     ErrUpgradeToWARLatch( LATCH* platchOld );
        VOID    DowngradeFromWARLatch( LATCH latch );

        VOID    Dirty( const BFDirtyFlags bfdf = bfdfDirty );
        VOID    DirtyForScrub();
        VOID    CoordinatedDirty( const DBTIME dbtime, const BFDirtyFlags bfdf = bfdfDirty );
        BOOL    FPageRecentlyDirtied( const IFMP ifmp ) const;
        
        VOID    ReleasePage( BOOL fTossImmediate = fFalse );

        VOID    Reset( );

        VOID *  PvBufferForCrashDump()      { return m_cpage.PvBuffer(); }

#ifdef DEBUGGER_EXTENSION
        VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif

    private:
        CSR( const CSR& );

};

INLINE
DBTIME CSR::Dbtime( )       const
{
    ASSERT_VALID( this );

    return m_dbtimeSeen;
}

INLINE
BOOL CSR::FLatched( )       const
{
    ASSERT_VALID( this );
    
    return m_latch != latchNone;
}


INLINE
LATCH CSR::Latch( )     const
{
    ASSERT_VALID( this );
    
    return (LATCH)m_latch;
}

INLINE
PGNO CSR::Pgno( ) const
{
    ASSERT_VALID( this );
    
    return m_pgno;
}

INLINE INT CSR::ILine( ) const
{
    ASSERT_VALID( this );
    
    return m_iline;
}

INLINE VOID CSR::SetILine( const INT iline )
{
    ASSERT_VALID( this );
    
    m_iline = (SHORT)iline;
}

INLINE VOID CSR::IncrementILine()
{
    ASSERT_VALID( this );
    m_iline++;
}

INLINE VOID CSR::DecrementILine()
{
    ASSERT_VALID( this );
    m_iline--;
}

INLINE
CPAGE&  CSR::Cpage( )
{
    ASSERT_VALID( this );
    Assert( FLatched() );
    
    return m_cpage;
}

INLINE
const CPAGE&    CSR::Cpage( ) const
{
    ASSERT_VALID( this );
    Assert( FLatched() );
    
    return m_cpage;
}

INLINE
PAGETRIM CSR::PagetrimState() const
{
    ASSERT_VALID( this );
    return m_pagetrimState;
}

INLINE
VOID CSR::SetPagetrimState(
    _In_ const PAGETRIM pagetrimState,
    _In_ const PGNO pgno )
{
    ASSERT_VALID( this );

    m_pagetrimState = pagetrimState;
    m_pgno = pgno;
}

INLINE VOID CSR::Dirty( const BFDirtyFlags bfdf )
{
    ASSERT_VALID( this );
    
    m_cpage.Dirty( bfdf );

    Assert( m_cpage.Dbtime( ) > m_dbtimeSeen
        || ( PinstFromIfmp( m_cpage.Ifmp() )->m_plog->FRecoveringMode() == fRecoveringRedo
            && ( m_cpage.Dbtime( ) == m_dbtimeSeen
                || g_rgfmp[ m_cpage.Ifmp() ].FCreatingDB() ) )
        || g_fRepair );

    m_dbtimeSeen = m_cpage.Dbtime( );
    m_pagetrimState = pagetrimNormal;
}

INLINE VOID CSR::DirtyForScrub()
{
    ASSERT_VALID( this );
    
    m_cpage.DirtyForScrub();

    Assert( m_cpage.Dbtime( ) > m_dbtimeSeen
        || ( PinstFromIfmp( m_cpage.Ifmp() )->m_plog->FRecoveringMode() == fRecoveringRedo && m_cpage.Dbtime( ) == m_dbtimeSeen )
        || g_fRepair );

    m_dbtimeSeen = m_cpage.Dbtime( );
    m_pagetrimState = pagetrimNormal;
}

INLINE VOID CSR::CoordinatedDirty( const DBTIME dbtime, const BFDirtyFlags bfdf )
{
    ASSERT_VALID( this );
    
    m_cpage.CoordinatedDirty( dbtime, bfdf );

    Assert( m_cpage.Dbtime() >= m_dbtimeSeen || g_fRepair );

    m_dbtimeSeen = m_cpage.Dbtime( );
    m_pagetrimState = pagetrimNormal;
}

INLINE BOOL CSR::FPageRecentlyDirtied( const IFMP ifmp ) const
{
    ASSERT_VALID( this );

    const DBTIME    dbtimeOldest            = ( g_rgfmp[ifmp].DbtimeOldestGuaranteed() );
    const BOOL      fPageRecentlyDirtied    = ( Dbtime() >= dbtimeOldest );

    return fPageRecentlyDirtied;
}


INLINE
VOID CSR::SetDbtime( const DBTIME dbtime )
{
    ASSERT_VALID( this );
    Assert( FDirty() );
    
    Assert( Latch() == latchWrite );
    Assert( dbtime >= m_dbtimeSeen );

    m_cpage.SetDbtime( dbtime );
    m_dbtimeSeen = dbtime;

    Assert( dbtime == m_cpage.Dbtime() );
}

INLINE
VOID CSR::OverrideDbtime_( const DBTIME dbtime, const ULONG fFlags )
{
    ASSERT_VALID( this );
    Assert( FDirty() );
    
    Assert( Latch() == latchWrite );
    Assert( dbtime <= m_dbtimeSeen );

    m_cpage.RevertDbtime( dbtime, fFlags );
    m_dbtimeSeen = dbtime;

    Assert( dbtime == m_cpage.Dbtime() );
}

INLINE
VOID CSR::RevertDbtime( const DBTIME dbtime, const ULONG fFlags )
{
    Assert( PinstFromIfmp( m_cpage.Ifmp() )->m_plog->FRecoveringMode() != fRecoveringRedo );
    OverrideDbtime_( dbtime, fFlags );
}

INLINE
VOID CSR::RestoreDbtime( const DBTIME dbtime )
{
    Assert( m_cpage.FLoadedPage() );
    Assert( PinstFromIfmp( m_cpage.Ifmp() )->m_plog->FRecoveringMode() == fRecoveringRedo );
    OverrideDbtime_( dbtime, m_cpage.FFlags() );
}


#ifdef DEBUG

INLINE BOOL CSR::FDirty( ) const
{
    ASSERT_VALID( this );
    
    Assert( latchWrite == m_latch );
    Assert( m_cpage.Dbtime() == m_dbtimeSeen );

    return m_cpage.FAssertDirty();
}

INLINE VOID CSR::AssertValid( ) const
{
#ifdef DEBUG_CSR

    const BOOL  fPerformCheck   = ( latchNone != m_latch );
 
    if ( fPerformCheck )
    {
        Assert( latchReadTouch == m_latch
            || latchReadNoTouch == m_latch
            || latchRIW == m_latch
            || latchWrite == m_latch );
        Assert( m_cpage.PgnoThis() == m_pgno );
        Assert( m_cpage.Dbtime() == m_dbtimeSeen );
        Assert( m_iline >= 0 );
    }

#endif
}

#endif
    



INLINE ERR CSR::ErrSwitchPage(
    PIB                 *ppib,
    const IFMP          ifmp,
    const PGNO          pgnoNext,
    const BOOL          fTossImmediate )
{
    return ErrSwitchPage_( ppib, ifmp, pgnoNext, bflfDefault, fTossImmediate );
}

INLINE ERR CSR::ErrSwitchPage_(
    PIB                 *ppib,
    const IFMP          ifmp,
    const PGNO          pgnoNext,
    const BFLatchFlags  bflf,
    const BOOL          fTossImmediate )
{
    ERR         err = JET_errSuccess;
    CPAGE       cpageNext;

    ASSERT_VALID( this );
    Assert( m_latch == latchReadTouch ||
            m_latch == latchReadNoTouch ||
            m_latch == latchRIW );
    Assert( m_cpage.PgnoPrev() != pgnoNext );
    Assert( pgnoNext != pgnoNull );


    BFLatch* pbflHint;
    m_cpage.GetLatchHint( m_iline, &pbflHint );

    if ( latchRIW != m_latch )
    {
        const BFLatchFlags bflfT =  BFLatchFlags(   bflf |
                                                    (   latchReadTouch == m_latch ?
                                                            bflfNone :
                                                            bflfNoTouch ) );

        Call( cpageNext.ErrGetReadPage( ppib, ifmp, pgnoNext, bflfT, pbflHint ) );

        m_cpage.ReleaseReadLatch( fTossImmediate );
    }
    else
    {
        Assert( latchRIW == m_latch );

        Call( cpageNext.ErrGetRDWPage( ppib, ifmp, pgnoNext, bflf, pbflHint ) );

        m_cpage.ReleaseRDWLatch( fTossImmediate );
    }

    m_cpage = cpageNext;

    m_pgno = pgnoNext;
    Assert( m_latch == latchReadTouch || m_latch == latchReadNoTouch ||
            m_latch == latchRIW );
    m_dbtimeSeen = m_cpage.Dbtime( );
    m_pagetrimState = pagetrimNormal;

HandleError:
    if ( err == JET_errFileIOBeyondEOF )
    {
        OSTraceSuspendGC();
        const WCHAR* rgwsz[] =
        {
            OSFormatW( L"%d", err ),
            OSFormatW( L"%I32u", m_cpage.ObjidFDP() ),
            L"",
            g_rgfmp[ifmp].WszDatabaseName(),
            OSFormatW( L"%I32u", m_cpage.PgnoThis() ),
            OSFormatW( L"%I32u", pgnoNext ),
            L"",
            L"SwitchPage",
            L"1"
        };
        UtilReportEvent(
                eventError,
                DATABASE_CORRUPTION_CATEGORY,
                BAD_PAGE_LINKS_ID,
                _countof( rgwsz ),
                rgwsz,
                0,
                NULL,
                PinstFromIfmp( ifmp ) );
        OSTraceResumeGC();
    }

    return err;
}


INLINE ERR CSR::ErrSwitchPageNoWait(
    PIB                 *ppib,
    const IFMP          ifmp,
    const PGNO          pgnoNext )
{
    return ErrSwitchPageNoWait_( ppib, ifmp, pgnoNext, bflfDefault );
}

INLINE ERR CSR::ErrSwitchPageNoWait_(
    PIB                 *ppib,
    const IFMP          ifmp,
    const PGNO          pgno,
    const BFLatchFlags  bflf )
{
    ERR                 err = JET_errSuccess;
    CPAGE               cpage;
    
    ASSERT_VALID( this );
    Assert( latchReadTouch == m_latch || latchReadNoTouch == m_latch );
    Assert( m_cpage.PgnoPrev( ) == pgno );
    Assert( pgno != pgnoNull );

    const BFLatchFlags bflfT =  BFLatchFlags(   bflf |
                                                bflfNoWait |
                                                (   latchReadTouch == m_latch ?
                                                        bflfNone :
                                                        bflfNoTouch ) );

    Call( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfT ) );

    m_cpage.ReleaseReadLatch();
    m_cpage = cpage;
    
    m_pgno = pgno;
    Assert( latchReadTouch == m_latch || latchReadNoTouch == m_latch );
    m_dbtimeSeen = m_cpage.Dbtime( );
    m_pagetrimState = pagetrimNormal;

HandleError:
    if ( err == JET_errFileIOBeyondEOF )
    {
        OSTraceSuspendGC();
        const WCHAR* rgwsz[] =
        {
            OSFormatW( L"%d", err ),
            OSFormatW( L"%I32u", m_cpage.ObjidFDP() ),
            L"",
            g_rgfmp[ifmp].WszDatabaseName(),
            OSFormatW( L"%I32u", m_cpage.PgnoThis() ),
            OSFormatW( L"%I32u", pgno ),
            L"",
            L"SwitchPageNoWait",
            L"1"
        };
        UtilReportEvent(
                eventError,
                DATABASE_CORRUPTION_CATEGORY,
                BAD_PAGE_LINKS_ID,
                _countof( rgwsz ),
                rgwsz,
                0,
                NULL,
                PinstFromIfmp( ifmp ) );
        OSTraceResumeGC();
    }

    return err;
}


INLINE ERR CSR::ErrGetPage(
    PIB                 *ppib,
    const IFMP          ifmp,
    const PGNO          pgno,
    const LATCH         latch,
    BFLatch* const      pbflHint,
    const BOOL          fUninitPageOk )
{
    Assert( latchReadTouch == latch || latchReadNoTouch == latch || latchRIW == latch );
    BFLatchFlags bflfExtra = fUninitPageOk ? bflfUninitPageOk : bflfNone;

    if ( latchRIW != latch )
    {
        const BFLatchFlags bflf = ( latchReadTouch == latch ? bflfNone : bflfNoTouch );
        return ErrGetReadPage( ppib, ifmp, pgno, BFLatchFlags( bflf | bflfExtra ), pbflHint );
    }
    else
    {
        return ErrGetRIWPage_( ppib, ifmp, pgno, BFLatchFlags( bflfDefault | bflfExtra ), pbflHint );
    }
}


INLINE ERR CSR::ErrGetReadPage(
    PIB                 *ppib,
    const IFMP          ifmp,
    const PGNO          pgno,
    const BFLatchFlags  bflf,
    BFLatch* const      pbflHint )
{
    return ErrGetReadPage_( ppib, ifmp, pgno, bflf, pbflHint );
}
    
INLINE ERR CSR::ErrGetReadPage_(
    PIB                 *ppib,
    const IFMP          ifmp,
    const PGNO          pgno,
    const BFLatchFlags  bflf,
    BFLatch* const      pbflHint )
{
    ERR     err;

    ASSERT_VALID( this );
    Assert( m_latch == latchNone );
    
    CallR( m_cpage.ErrGetReadPage( ppib, ifmp, pgno, bflf, pbflHint ) );

    m_pgno = pgno;
    m_dbtimeSeen = m_cpage.Dbtime( );
    m_latch = LATCH( ( bflf & bflfNoTouch ) ? latchReadNoTouch : latchReadTouch );
    m_pagetrimState = pagetrimNormal;

    return err;
}


INLINE ERR CSR::ErrGetRIWPage(
    PIB                 *ppib,
    const IFMP          ifmp,
    const PGNO          pgno )
{
    return ErrGetRIWPage_( ppib, ifmp, pgno, bflfDefault, NULL );
}

INLINE ERR CSR::ErrGetRIWPage(
    PIB                 *ppib,
    const IFMP          ifmp,
    const PGNO          pgno,
    const BFLatchFlags  bflf )
{
    return ErrGetRIWPage_( ppib, ifmp, pgno, bflf, NULL );
}

INLINE ERR CSR::ErrGetRIWPage_(
    PIB                 *ppib,
    const IFMP          ifmp,
    const PGNO          pgno,
    const BFLatchFlags  bflf,
    BFLatch* const      pbflHint )
{
    ERR                 err;

    ASSERT_VALID( this );
    Assert( m_latch == latchNone );
    
    CallR( m_cpage.ErrGetRDWPage( ppib, ifmp, pgno, bflf, pbflHint ) );

    m_pgno = pgno;
    m_dbtimeSeen = m_cpage.Dbtime( );
    m_latch = latchRIW;
    m_pagetrimState = pagetrimNormal;

    return err;
}


INLINE ERR CSR::ErrLoadPage(
    PIB                 * const ppib,
    const IFMP          ifmp,
    const PGNO          pgno,
    const VOID          * const pvPage,
    const ULONG         cbPage,
    LATCH               latch )
{
    ERR                 err;

    ASSERT_VALID( this );
    Assert( m_latch == latchNone );

    CallR( m_cpage.ErrLoadPage( ppib, ifmp, pgno, pvPage, cbPage ) );

    m_pgno = pgno;
    m_dbtimeSeen = m_cpage.Dbtime( );
    m_latch = latch;
    m_pagetrimState = pagetrimNormal;

    Assert( m_dbtimeSeen == m_cpage.Dbtime() );

    return err;
}


INLINE ERR CSR::ErrGetNewPreInitPage(
    PIB                 *ppib,
    const IFMP          ifmp,
    const PGNO          pgno,
    const OBJID         objidFDP,
    const BOOL          fLogNewPage )
{
    return ErrGetNewPreInitPage_(
        ppib,
        ifmp,
        pgno,
        objidFDP,
        fLogNewPage ?
            1 :
            g_rgfmp[ifmp].DbtimeGet(),
        fLogNewPage );
}


INLINE ERR CSR::ErrGetNewPreInitPage_(
    PIB                 *ppib,
    const IFMP          ifmp,
    const PGNO          pgno,
    const OBJID         objidFDP,
    const DBTIME        dbtimeNoLog,
    const BOOL          fLogNewPage )
{
    ERR                 err;

    ASSERT_VALID( this );
    Assert( m_latch == latchNone );

    OnDebug( const DBTIME dbtimeCurrent = g_rgfmp[ifmp].DbtimeGet() );

    CallR( m_cpage.ErrGetNewPreInitPage(
                        ppib,
                        ifmp,
                        pgno,
                        objidFDP,
                        dbtimeNoLog,
                        fLogNewPage ) );

    Assert( ( PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() != fRecoveringRedo ) ||
            ( m_cpage.Dbtime() == dbtimeNoLog ) ||
            ( g_rgfmp[ ifmp ].FCreatingDB() && ( m_cpage.Dbtime() == ( dbtimeCurrent + 1 ) ) ) );
    Assert( ( PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() == fRecoveringRedo ) ||
            ( g_rgfmp[ifmp].DbtimeGet() > dbtimeCurrent ) );

    m_pgno = pgno;
    m_dbtimeSeen = m_cpage.Dbtime();
    m_latch = latchWrite;
    m_pagetrimState = pagetrimNormal;

    return err;
}


INLINE ERR CSR::ErrGetNewPreInitPageForRedo(
    PIB                 * const ppib,
    const IFMP          ifmp,
    const PGNO          pgno,
    const OBJID         objidFDP,
    const DBTIME        dbtimeOperToRedo )
{
    Assert( PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() == fRecoveringRedo );
    return ErrGetNewPreInitPage_(
        ppib,
        ifmp,
        pgno,
        objidFDP,
        dbtimeOperToRedo - 1,
        fFalse  );
}


INLINE VOID CSR::ConsumePreInitPage( const ULONG fPageFlags )
{
    m_cpage.ConsumePreInitPage( fPageFlags );
}


INLINE VOID CSR::FinalizePreInitPage( OnDebug( const BOOL fCheckPreInitPage ) )
{
    m_cpage.FinalizePreInitPage( OnDebug( fCheckPreInitPage ) );
}


INLINE ERR CSR::ErrUpgradeFromReadLatch( )
{
    ERR     err;

    ASSERT_VALID( this );
    Assert( m_latch == latchReadTouch || m_latch == latchReadNoTouch );

    err = m_cpage.ErrUpgradeReadLatchToWriteLatch( );

    if ( err >= 0 )
    {
        Assert( m_pgno != pgnoNull );
        Assert( m_dbtimeSeen == m_cpage.Dbtime( ) );
        Assert( pagetrimNormal == m_pagetrimState );

        m_latch = latchWrite;
    }
    else
    {
        m_latch = latchNone;
    }

    return err;
}
    

INLINE
ERR CSR::ErrUpgrade()
{
    ASSERT_VALID( this );
    Assert( m_latch == latchReadTouch || m_latch == latchReadNoTouch ||
            m_latch == latchRIW );

    if ( latchRIW != m_latch )
    {
        return ErrUpgradeFromReadLatch();
    }
    else
    {
        UpgradeFromRIWLatch();
        return JET_errSuccess;
    }
}

    
INLINE
VOID CSR::UpgradeFromRIWLatch( )
{
    ASSERT_VALID( this );
    Assert( latchRIW == m_latch );
    m_cpage.UpgradeRDWLatchToWriteLatch( );

    Assert( m_pgno != pgnoNull );
    Assert( m_dbtimeSeen == m_cpage.Dbtime( ) );
    m_latch = latchWrite;

    return;
}


INLINE
VOID    CSR::Downgrade( LATCH latch )
{
    ASSERT_VALID( this );

    Assert( latch == latchReadTouch || latch == latchReadNoTouch ||
            latchRIW == latch );
    Assert( m_dbtimeSeen == m_cpage.Dbtime( ) );

    if ( latchRIW == latch )
    {
        Assert( latchWrite == m_latch );
        m_cpage.DowngradeWriteLatchToRDWLatch();
    }
    else
    {
        Assert( latchRIW == m_latch || latchWrite == m_latch );
        if ( m_latch == latchWrite )
        {
            m_cpage.DowngradeWriteLatchToReadLatch();
        }
        else
        {
            m_cpage.DowngradeRDWLatchToReadLatch();
        }
    }

    Assert( m_dbtimeSeen == m_cpage.Dbtime( ) );
    m_latch = latch;
    return;
}

INLINE
ERR     CSR::ErrUpgradeToWARLatch( LATCH* platchOld )
{
    ERR err = JET_errSuccess;
    
    ASSERT_VALID( this );
    Assert( m_latch == latchReadTouch ||
            m_latch == latchReadNoTouch ||
            m_latch == latchRIW ||
            m_latch == latchWrite ||
            m_latch == latchWAR );

    *platchOld = LATCH( m_latch );

    if ( m_latch == latchRIW )
    {
        m_cpage.UpgradeRDWLatchToWARLatch();
    }
    else if ( m_latch == latchReadTouch || m_latch == latchReadNoTouch )
    {
        Call( m_cpage.ErrTryUpgradeReadLatchToWARLatch() );
    }

    m_latch = latchWAR;

HandleError:
    return err;
}

INLINE
VOID    CSR::DowngradeFromWARLatch( LATCH latch )
{
    ASSERT_VALID( this );
    Assert( m_latch == latchWAR );
    Assert( latch == latchReadTouch ||
            latch == latchReadNoTouch ||
            latch == latchRIW ||
            latch == latchWrite ||
            latch == latchWAR );

    if ( latch == latchRIW )
    {
        m_cpage.DowngradeWARLatchToRDWLatch();
    }
    else if ( latch == latchReadTouch || latch == latchReadNoTouch )
    {
        m_cpage.DowngradeWARLatchToReadLatch();
    }

    m_latch = latch;
}


INLINE
VOID    CSR::ReleasePage( BOOL fTossImmediate )
{
    ASSERT_VALID( this );
    if ( FLatched( ) )
    {
#ifdef DEBUG
        const DBTIME    dbtimePage  = m_cpage.Dbtime( );
#endif

        if ( m_latch == latchReadTouch || m_latch == latchReadNoTouch )
        {
            Assert( dbtimePage == m_dbtimeSeen );
            m_cpage.ReleaseReadLatch( fTossImmediate );
        }
        else if ( latchRIW == m_latch )
        {
            Assert( dbtimePage == m_dbtimeSeen );
            m_cpage.ReleaseRDWLatch( fTossImmediate );
        }
        else if ( latchWrite == m_latch )
        {
            Assert( dbtimePage == m_dbtimeSeen );
            m_cpage.ReleaseWriteLatch( fTossImmediate );
        }
        else
        {
            Assert( fFalse );
        }
    }

    m_latch = latchNone;
}

INLINE
CSR::CSR( ) :
    m_pagetrimState( pagetrimNormal ),
    m_cpage( )
{
    m_dbtimeSeen = dbtimeNil;
    m_latch = latchNone;
    m_pgno = pgnoNull;
}

INLINE VOID CSR::operator=( CSR& csr )
{
    ASSERT_VALID( this );
    ASSERT_VALID( &csr );
    Assert( &csr != this );

    if ( &csr != this )
    {
        m_pgno = csr.m_pgno;
        m_latch = csr.m_latch;
        m_dbtimeSeen = csr.m_dbtimeSeen;
        m_cpage = csr.m_cpage;

        csr.m_latch = latchNone;
        
        csr.Reset( );
    }
}


INLINE
CSR::~CSR()
{
    ASSERT_VALID( this );
    Assert( m_latch == latchNone );
}

INLINE
VOID CSR::Reset()
{
    ASSERT_VALID( this );
    Assert( m_latch == latchNone );

    m_pgno = pgnoNull;
    m_dbtimeSeen = dbtimeNil;
    m_pagetrimState = pagetrimNormal;
}
