// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef DEBUG
///#define DEBUG_CSR
#endif

//  page latches
//
enum LATCH : BYTE
    {   latchNone,
        latchReadTouch,
        latchReadNoTouch,
        latchRIW,
        latchWrite,
        latchWAR,
};

//  Page states.
enum PAGETRIM : BYTE
{
    pagetrimNormal,
    pagetrimTrimmed,
    pagetrimMax,
};

//  Currency Stack Register
//  stores physical currency of node
//
//  cpage and iLine are only members passed out as a reference
//      other classes [namely BT] may use and set iLine
//      but they can only use cpage [not overwrite it].
//      BT and node use the methods provided by CPAGe directly,
//      after getting the reference to CPAGE object from CSR
//      the CSR controls access/release and latching of Cpage objects.
//
class CSR
{
    public:
        //  constructor/destructor
        //
        CSR( );
        ~CSR( );
        VOID operator=  ( CSR& );  //  ** this resets the CSR being assigned from

    private:
        DBTIME      m_dbtimeSeen;       //  page time stamp
                                        //  set at GetPage only
        PGNO        m_pgno;             //  pgno of current page
        SHORT       m_iline;            //  current node itag
        LATCH       m_latch;            //  latch type
        PAGETRIM    m_pagetrimState;    //  Whether this page is trimmed and the operation should be ignored during Redo.
        CPAGE       m_cpage;            //  latched page

    public:
        //  read functions
        //
        DBTIME  Dbtime( )       const;
        VOID    SetDbtime( const DBTIME dbtime );
        VOID    RevertDbtime( const DBTIME dbtime, const ULONG fFlags );
        VOID    RestoreDbtime( const DBTIME dbtime, const BOOL fPageFDPDeleteBefore );
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

        //  page retrieve operations
        //
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
        VOID    CopyPage(
                const VOID*         pv,
                const ULONG         cb );

        // Pre-initialization functions:
        //
        //  - ErrGetNewPreInitPage(): produces a write-latched pre-initialized page to be
        //    consumed as a new page in operations that require so (splits, page moves, etc...).
        //    The cached page returned by this function is expected to be modified by the calling
        //    code and once the overall operation gets to a point in which it can't fail anymore,
        //    FinalizePreInitPage() must be called to signal the engine that the page is not
        //    supposed to be reverted to its pristine pre-initialized state once unlatched.
        //
        //  - ErrGetNewPreInitPageForRedo(): used to redo operations that require new pages. It also
        //    produces a write-latched page, but its dbtime is set to one unit behind dbtimeOperToRedo.
        //    Similarly to ErrGetNewPreInitPage(), FinalizePreInitPage() must also be called when the
        //    operation can't fail anymore, otherwise, the page will be returned to its pristine
        //    pre-initialized state (i.e., the state returned by ErrGetNewPreInitPageForRedo()) when the latch
        //    is released.
        //
        //  - ConsumePreInitPage(): it is supposed to be called to signal that the pre-initialized page produced
        //    by ErrGetNewPreInitPage() or ErrGetNewPreInitPageForRedo() is about to become a tree page. It is NOT
        //    supposed to be called if the overall operation clobbers the page with a cached or logged pre-image,
        //    as it the case for page moves (including root pages), because the clobbering itself is supposed to
        //    replace the page image with a consistent page. This function has the side effect of both resetting
        //    CPAGE::fPagePreInit and setting up an external header tag on the page.
        //
        //  - FinalizePreInitPage(): transitions the page from a semi-initialized state to a fully initialized
        //    one. It MUST be called once the operation that created the page with ErrGetNewPreInitPage() or
        //    ErrGetNewPreInitPageForRedo() is guaranteed to complete successfully. If the write latch is released
        //    without this function being called, the page will be reverted to its pristine pre-initialized state like
        //    the one produced by the ErrGetNewPreInitPage*() and left dirty in the cache. Note that this function
        //    is also where the new page operation is traced. It is traced here because this is the point in which we
        //    are guaranteed to have the page in a fully initialized state so the metadata that gets traced is consistent.
        //
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

        //  page dirty
        //
        VOID    Dirty( const BFDirtyFlags bfdf = bfdfDirty );
        VOID    DirtyForScrub();
        VOID    CoordinatedDirty( const DBTIME dbtime, const BFDirtyFlags bfdf = bfdfDirty );
        BOOL    FPageRecentlyDirtied( const IFMP ifmp ) const;
        
        //  page release operations
        //
        VOID    ReleasePage( BOOL fTossImmediate = fFalse );

        //  reset
        VOID    Reset( );

        //  for crash-dump gathering of referenced pages
        //
        const VOID *  PvBufferForCrashDump()    { return m_cpage.PvBuffer(); }

#ifdef DEBUGGER_EXTENSION
        VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif  //  DEBUGGER_EXTENSION

    private:
        CSR( const CSR& );  //  not defines

};

//  INLINE public functions
//
INLINE
DBTIME CSR::Dbtime( )       const
{
    ASSERT_VALID( this );

    //  used by BT to check if currency is valid
    //
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
    
    //  dirty page
    //  update m_dbtime
    //
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
    
    //  dirty page
    //  update m_dbtime
    //
    m_cpage.DirtyForScrub();

    Assert( m_cpage.Dbtime( ) > m_dbtimeSeen
        || ( PinstFromIfmp( m_cpage.Ifmp() )->m_plog->FRecoveringMode() == fRecoveringRedo && m_cpage.Dbtime( ) == m_dbtimeSeen )
        || g_fRepair );

    m_dbtimeSeen = m_cpage.Dbtime( );
    m_pagetrimState = pagetrimNormal;
}

//  called for multi-page operations to coordinate the dbtime of
//  all the pages involved in the operation with one dbtime
INLINE VOID CSR::CoordinatedDirty( const DBTIME dbtime, const BFDirtyFlags bfdf )
{
    ASSERT_VALID( this );
    
    //  dirty page
    //  update m_dbtime
    //
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
    Assert( PinstFromIfmp( m_cpage.Ifmp() )->m_plog->FRecoveringMode() != fRecoveringRedo );  // redo gets all preconditions done, and never has to revert.
    OverrideDbtime_( dbtime, fFlags );
}

INLINE
VOID CSR::RestoreDbtime( const DBTIME dbtime, const BOOL fPageFDPDeleteBefore )
{
    Assert( m_cpage.FLoadedPage() );
    Assert( PinstFromIfmp( m_cpage.Ifmp() )->m_plog->FRecoveringMode() == fRecoveringRedo );
    OverrideDbtime_( dbtime, m_cpage.FFlags() | ( fPageFDPDeleteBefore ? CPAGE::fPageFDPDelete : 0 ) );
}


#ifdef DEBUG

INLINE BOOL CSR::FDirty( ) const
{
    ASSERT_VALID( this );
    
    Assert( latchWrite == m_latch );
    Assert( m_cpage.Dbtime() == m_dbtimeSeen );

    return m_cpage.FAssertDirty();
}

//  ================================================================
INLINE VOID CSR::AssertValid( ) const
//  ================================================================
{
#ifdef DEBUG_CSR

    const BOOL  fPerformCheck   = ( latchNone != m_latch );
 
    if ( fPerformCheck )
    {
        ///ASSERT_VALID( &m_cpage );
        Assert( latchReadTouch == m_latch
            || latchReadNoTouch == m_latch
            || latchRIW == m_latch
            || latchWrite == m_latch );
        Assert( m_cpage.PgnoThis() == m_pgno );
        Assert( m_cpage.Dbtime() == m_dbtimeSeen );
        Assert( m_iline >= 0 );
    }

#endif  //  DEBUG_CSR
}

#endif  //  DEBUG
    

//  NOTE: that all the following methods are atomic. 
//  i.e., they do not change any of the members
//  unless the requested page is accessed successfully
//  This property is key in being able to access the previous page
//  with the same CSR
//
//  page retrieve operations
//

//  switches to pgnoNext with the same latch as current page
//

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

    //  get the latch hint from the previous page for use to go to the next page
    //
    //  NOTE:  this will return NULL if we are scanning because the previous page
    //  will be a leaf page and no leaf pages have hint context.  because of this,
    //  we can still ask for the hint even though the iline has nothing to do with
    //  the page pointer (as it does for internal pages)

    BFLatch* pbflHint;
    m_cpage.GetLatchHint( m_iline, &pbflHint );

    //  get given page with same latch type as current one and release the
    //  current page
    //
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

    //  copy next page into Current
    //
    m_cpage = cpageNext;

    //  set members
    //
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

    //  get given page without wait
    //
    const BFLatchFlags bflfT =  BFLatchFlags(   bflf |
                                                bflfNoWait |
                                                (   latchReadTouch == m_latch ?
                                                        bflfNone :
                                                        bflfNoTouch ) );

    Call( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfT ) );

    //  release current page
    //  and copy requested page into current
    //
    m_cpage.ReleaseReadLatch();
    m_cpage = cpage;
    
    //  set members
    //
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

    //  set members
    //
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

    //  set members
    //
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

    //  set members
    m_pgno = pgno;
    m_dbtimeSeen = m_cpage.Dbtime( );
    m_latch = latch;
    m_pagetrimState = pagetrimNormal;

    Assert( m_dbtimeSeen == m_cpage.Dbtime() );

    return err;
}


INLINE VOID CSR::CopyPage( const VOID* pvPage, const ULONG cbPage )
{
    ASSERT_VALID( this );
    Assert( m_latch == latchWrite );
    Assert( m_cpage.FAssertDirty() );

    m_cpage.CopyPage( pvPage, cbPage );

    //  set members
    Assert( m_pgno = m_cpage.PgnoThis() );  // pgno can't change
    m_dbtimeSeen = m_cpage.Dbtime();
    m_pagetrimState = pagetrimNormal;
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
            1 : // irrelevant because the page gets dirtied internally so this gets clobbered.
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

    //  set members
    //
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
        fFalse /* fLogNewPage */ );
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

    //  set members
    //
    if ( err >= 0 )
    {
        Assert( m_pgno != pgnoNull );
        Assert( m_dbtimeSeen == m_cpage.Dbtime( ) );
        Assert( pagetrimNormal == m_pagetrimState );

        m_latch = latchWrite;
    }
    else
    {
        //  lost read latch on page
        //
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

    //  set members
    //
    Assert( m_pgno != pgnoNull );
    Assert( m_dbtimeSeen == m_cpage.Dbtime( ) );
    m_latch = latchWrite;

    return;
}


//  downgrades latch from WriteLatch to given latch
//
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


//  page release operations
//
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

    //  do not reset dbtime or pgno
    //  they will be used later for refreshing currency
    //  
    m_latch = latchNone;
}

//  constructor
//
INLINE
CSR::CSR( ) :
    m_pagetrimState( pagetrimNormal ),
    m_cpage( )
{
    m_dbtimeSeen = dbtimeNil;
    m_latch = latchNone;
    m_pgno = pgnoNull;
}

//  copy constructor
//  resets CSR copied from
//
INLINE VOID CSR::operator=( CSR& csr )
{
    //  check for assignment to ourselves
    ASSERT_VALID( this );
    ASSERT_VALID( &csr );
    Assert( &csr != this );

    if ( &csr != this )
    {
        //  copy the data
        //
        m_pgno = csr.m_pgno;
        m_latch = csr.m_latch;
        m_dbtimeSeen = csr.m_dbtimeSeen;
        m_cpage = csr.m_cpage;              //  this destroys source c_page

        csr.m_latch = latchNone;
        
        //  reset source CSR
        //
        csr.Reset( );
    }
}


//  destructor
//
INLINE
CSR::~CSR()
{
    ASSERT_VALID( this );
    Assert( m_latch == latchNone );
}

//  reset 
//  used by BTUp
INLINE
VOID CSR::Reset()
{
    ASSERT_VALID( this );
    Assert( m_latch == latchNone );

    m_pgno = pgnoNull;
    m_dbtimeSeen = dbtimeNil;
    m_pagetrimState = pagetrimNormal;
}
