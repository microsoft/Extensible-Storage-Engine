// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

//  ================================================================
TTMAP::TTMAP( PIB * const ppib ) :
//  ================================================================
    m_tableid( JET_tableidNil ),
    m_sesid( reinterpret_cast<JET_SESID>( ppib ) ),
    m_columnidKey( 0 ),
    m_columnidValue( 0 ),
    m_crecords( 0 )
{
}


//  ================================================================
TTMAP::~TTMAP()
//  ================================================================
{
    if( JET_tableidNil != m_tableid )
    {
        CallS( ErrDispCloseTable( m_sesid, m_tableid ) );
        m_tableid = JET_tableidNil;
    }
}


//  ================================================================
ERR TTMAP::ErrInit( INST * const )
//  ================================================================
{
    ERR err = JET_errSuccess;

    JET_COLUMNDEF   rgcolumndef[2] = {
            { sizeof( JET_COLUMNDEF ), 0, JET_coltypUnsignedLongLong, 0, 0, 0, 0, JET_cbKeyMost_OLD, JET_bitColumnTTKey },  //  Key
        { sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, 0, 0 },                               //  Value
    };
    JET_COLUMNID    rgcolumnid[sizeof(rgcolumndef)/sizeof(JET_COLUMNDEF)];

    CallR( ErrIsamOpenTempTable(
        m_sesid,
        rgcolumndef,
        sizeof(rgcolumndef)/sizeof(JET_COLUMNDEF),
        0,
        JET_bitTTIndexed | JET_bitTTUnique | JET_bitTTScrollable | JET_bitTTUpdatable,
        &m_tableid,
        rgcolumnid,
        JET_cbKeyMost_OLD,
        JET_cbKeyMost_OLD ) );

    m_columnidKey       = rgcolumnid[0];
    m_columnidValue     = rgcolumnid[1];

    return err;
}


//  ================================================================
ERR TTMAP::ErrInsertKeyValue_( const unsigned __int64 ui64Key, const ULONG ulValue )
//  ================================================================
{
    ERR err = JET_errSuccess;

    if( JET_tableidNil == m_tableid )
    {
        return ErrERRCheck( JET_errInternalError );
    }

    CallR( ErrDIRBeginTransaction( (PIB*) m_sesid, 38949, NO_GRBIT ) );
    Call( ErrDispPrepareUpdate( m_sesid, m_tableid, JET_prepInsert ) );

    Call( ErrDispSetColumn(
                m_sesid,
                m_tableid,
                m_columnidKey,
                (BYTE *)&ui64Key,
                sizeof( ui64Key ),
                0,
                NULL ) );

    Call( ErrDispSetColumn(
                m_sesid,
                m_tableid,
                m_columnidValue,
                (BYTE *)&ulValue,
                sizeof( ulValue ),
                0,
                NULL ) );

    Call( ErrDispUpdate( m_sesid, m_tableid, NULL, 0, NULL, NO_GRBIT ) );
    Call( ErrDIRCommitTransaction( (PIB*) m_sesid, NO_GRBIT ) );

    ++m_crecords;

    return err;

HandleError:
    (VOID)ErrDispPrepareUpdate( m_sesid, m_tableid, JET_prepCancel );
    CallS( ErrDIRRollback( (PIB*) m_sesid ) );
    return err;
}


//  ================================================================
ERR TTMAP::ErrRetrieveValue_( const unsigned __int64 ui64Key, ULONG * const pulValue ) const
//  ================================================================
{
    ERR err = JET_errSuccess;

    if( JET_tableidNil == m_tableid )
    {
        return ErrERRCheck( JET_errInternalError );
    }

    CallR( ErrDIRBeginTransaction( (PIB*) m_sesid, 55333, NO_GRBIT ) );
    CallS( ErrDispMakeKey( m_sesid, m_tableid, (BYTE *)&ui64Key, sizeof( ui64Key ), JET_bitNewKey ) );

    err = ErrDispSeek( m_sesid, m_tableid, JET_bitSeekEQ );

    if( err >= 0 && pulValue )
    {
        ULONG cbActual = 0;
        err = ErrDispRetrieveColumn(
                m_sesid,
                m_tableid,
                m_columnidValue,
                pulValue,
                sizeof( ULONG ),
                &cbActual,
                NO_GRBIT,
                NULL );
    }

    CallS( ErrDIRCommitTransaction( (PIB*) m_sesid, NO_GRBIT ) );
    return err;
}


//  ================================================================
ERR TTMAP::ErrIncrementValue( const unsigned __int64 ui64Key )
//  ================================================================
{
    ERR     err         = JET_errSuccess;
    BOOL    fUpdatePrepared = fFalse;

    CallR( ErrDIRBeginTransaction( (PIB*) m_sesid, 43045, NO_GRBIT ) );

    Call( ErrDispMakeKey( m_sesid, m_tableid, (BYTE *)&ui64Key, sizeof( ui64Key ), JET_bitNewKey ) );
    err = ErrDispSeek( m_sesid, m_tableid, JET_bitSeekEQ );
    if( JET_errRecordNotFound == err )
    {
        Call( ErrInsertKeyValue_( ui64Key, 1 ) );
    }
    else
    {
        ULONG   cbActual;
        ULONG   ulValue = 0;

        Call( err );
        Call( ErrDispPrepareUpdate( m_sesid, m_tableid, JET_prepReplace ) );
        fUpdatePrepared = fTrue;

        Call( ErrDispRetrieveColumn(
                m_sesid,
                m_tableid,
                m_columnidValue,
                &ulValue,
                sizeof( ulValue ),
                &cbActual,
                JET_bitRetrieveCopy,
                NULL ) );
        Assert( sizeof( ulValue ) == cbActual );

        ++ulValue;
        Call( ErrDispSetColumn(
                    m_sesid,
                    m_tableid,
                    m_columnidValue,
                    (BYTE *)&ulValue,
                    sizeof( ulValue ),
                    NO_GRBIT,
                    NULL ) );
        Call( ErrDispUpdate( m_sesid, m_tableid, NULL, 0, NULL, NO_GRBIT ) );
    }

    Call( ErrDIRCommitTransaction( (PIB*) m_sesid, NO_GRBIT ) );

    return err;

HandleError:
    if( fUpdatePrepared )
    {
        (VOID)ErrDispPrepareUpdate( m_sesid, m_tableid, JET_prepCancel );
    }
    CallS( ErrDIRRollback( (PIB*) m_sesid ) );
    return err;
}


//  ================================================================
ERR TTMAP::ErrSetValue( const unsigned __int64 ui64Key, const ULONG ulValue )
//  ================================================================
{
    ERR     err         = JET_errSuccess;
    BOOL    fUpdatePrepared = fFalse;

    if( JET_tableidNil == m_tableid )
    {
        return ErrERRCheck( JET_errInternalError );
    }

    CallR( ErrDIRBeginTransaction( (PIB*)m_sesid, 59429, NO_GRBIT ) );

    Call( ErrDispMakeKey( m_sesid, m_tableid, (BYTE *)&ui64Key, sizeof( ui64Key ), JET_bitNewKey ) );
    err = ErrDispSeek( m_sesid, m_tableid, JET_bitSeekEQ );
    if( JET_errRecordNotFound == err )
    {
        Call( ErrInsertKeyValue_( ui64Key, ulValue ) );
    }
    else
    {
        Call( err );
        Call( ErrDispPrepareUpdate( m_sesid, m_tableid, JET_prepReplace ) );
        fUpdatePrepared = fTrue;
        Call( ErrDispSetColumn(
                    m_sesid,
                    m_tableid,
                    m_columnidValue,
                    (BYTE *)&ulValue,
                    sizeof( ulValue ),
                    NO_GRBIT,
                    NULL ) );
        Call( ErrDispUpdate( m_sesid, m_tableid, NULL, 0, NULL, NO_GRBIT ) );
    }

    Call( ErrDIRCommitTransaction( (PIB*) m_sesid, NO_GRBIT ) );

    return err;

HandleError:
    if( fUpdatePrepared )
    {
        (VOID)ErrDispPrepareUpdate( m_sesid, m_tableid, JET_prepCancel );
    }
    CallS( ErrDIRRollback( (PIB*)m_sesid ) );
    return err;
}


//  ================================================================
ERR TTMAP::ErrGetValue( const unsigned __int64 ui64Key, ULONG * const pulValue ) const
//  ================================================================
{
    return ErrRetrieveValue_( ui64Key, pulValue );
}


//  ================================================================
ERR TTMAP::ErrGetCurrentKeyValue( unsigned __int64 * const pui64Key, ULONG * const pulValue ) const
//  ================================================================
{
    ERR err = JET_errSuccess;

    if( JET_tableidNil == m_tableid )
    {
        return ErrERRCheck( JET_errInternalError );
    }

    CallR( ErrDIRBeginTransaction( (PIB*) m_sesid, 34853, NO_GRBIT ) );

    ULONG cbActual;
    Call( ErrDispRetrieveColumn(
            m_sesid,
            m_tableid,
            m_columnidKey,
            pui64Key,
            sizeof( unsigned __int64 ),
            &cbActual,
            NO_GRBIT,
            NULL ) );

    Call( ErrDispRetrieveColumn(
            m_sesid,
            m_tableid,
            m_columnidValue,
            pulValue,
            sizeof( ULONG ),
            &cbActual,
            NO_GRBIT,
            NULL ) );

HandleError:
    CallS( ErrDIRCommitTransaction( (PIB*) m_sesid, NO_GRBIT ) );
    return err;
}


//  ================================================================
ERR TTMAP::ErrDeleteKey( const unsigned __int64 ui64Key )
//  ================================================================
{
    ERR err = JET_errSuccess;

    if( JET_tableidNil == m_tableid )
    {
        return ErrERRCheck( JET_errInternalError );
    }

    CallR( ErrDIRBeginTransaction( (PIB*) m_sesid, 51237, NO_GRBIT ) );
    CallS( ErrDispMakeKey( m_sesid, m_tableid, (BYTE *)&ui64Key, sizeof( ui64Key ), JET_bitNewKey ) );
    Call( ErrDispSeek( m_sesid, m_tableid, JET_bitSeekEQ ) );

    Call( ErrDispDelete( m_sesid, m_tableid ) );
    Call( ErrDIRCommitTransaction( (PIB*) m_sesid, NO_GRBIT ) );

    --m_crecords;

    return err;

HandleError:
    CallS( ErrDIRRollback( (PIB*) m_sesid ) );
    return err;
}


//  ================================================================
ERR TTMAP::ErrMoveFirst()
//  ================================================================
{
    if( JET_tableidNil == m_tableid )
    {
        return ErrERRCheck( JET_errInternalError );
    }

    FUCBSetSequential( (FUCB *)m_tableid );
    FUCBSetPrereadForward( (FUCB *)m_tableid, cpgPrereadSequential );
    return ErrDispMove( m_sesid, m_tableid, JET_MoveFirst, NO_GRBIT );
}


//  ================================================================
ERR TTMAP::ErrMoveNext()
//  ================================================================
{
    if( JET_tableidNil == m_tableid )
    {
        return ErrERRCheck( JET_errInternalError );
    }

    return ErrDispMove( m_sesid, m_tableid, JET_MoveNext, NO_GRBIT );
}


//  ================================================================
ERR TTMAP::ErrFEmpty( BOOL * const pfEmpty ) const
//  ================================================================
{
    if( JET_tableidNil == m_tableid )
    {
        return ErrERRCheck( JET_errInternalError );
    }

    ERR err = ErrDispMove( m_sesid, m_tableid, JET_MoveFirst, NO_GRBIT );

    *pfEmpty = fFalse;
    if( JET_errNoCurrentRecord == err )
    {
        *pfEmpty = fTrue;
        err = JET_errSuccess;
    }
    Call( err );

#ifdef DEBUG
    if( !*pfEmpty )
    {
        unsigned __int64 ui64Key;
        ULONG cbActual;
        const ERR errT = ErrDispRetrieveColumn(
            m_sesid,
            m_tableid,
            m_columnidKey,
            &ui64Key,
            sizeof( ui64Key ),
            &cbActual,
            NO_GRBIT,
            NULL );

        Assert( JET_errSuccess == errT );
    }
#endif  //  DEBUG

HandleError:
    return err;
}


//  ================================================================
TTARRAY::TTARRAY( const ULONG culEntries, const ULONG ulDefault ) :
//  ================================================================
    m_culEntries( culEntries ),
    m_ulDefault( ulDefault ),
    m_ppib( ppibNil ),
    m_pfucb( pfucbNil ),
    m_pgnoFirst( pgnoNull ),
    m_rgbitInit( NULL )
{
}


//  ================================================================
TTARRAY::~TTARRAY()
//  ================================================================
{
    //  delete our bit-map of init space if it exists

    if ( m_rgbitInit )
    {
        OSMemoryPageFree( m_rgbitInit );
        m_rgbitInit = NULL;
    }

    //  delete the dummy table if it exists

    if ( m_pfucb != pfucbNil )
    {
        CallS( ErrFILECloseTable( m_ppib, m_pfucb ) );
        m_pfucb = pfucbNil;
    }

    //  close our session

    if ( m_ppib != ppibNil )
    {
        PIBEndSession( m_ppib );
        m_ppib = ppibNil;
    }
}


DWORD TTARRAY::s_cTTArray = 0;

//  ================================================================
ERR TTARRAY::ErrInit( INST * const pinst )
//  ================================================================
{
    ERR                 err                     = JET_errSuccess;
    CHAR                szName[JET_cbNameMost+1];
    JET_TABLECREATE5_A  tablecreate;

    //  get a session

    Call( ErrPIBBeginSession( pinst, &m_ppib, procidNil, fFalse ) );

    //  create a dummy table large enough to contain all the elements of the array
    //  as well as the FDP

    DWORD cpg;
    cpg = sizeof( DWORD ) * m_culEntries / g_cbPage + 1;

    OSStrCbFormatA( szName, sizeof(szName), "TTArray%d", AtomicIncrement( (LONG*)&s_cTTArray ) );

    tablecreate.cbStruct            = sizeof( tablecreate );
    tablecreate.szTableName         = szName;
    tablecreate.szTemplateTableName = NULL;
    tablecreate.ulPages             = cpg + 16;
    tablecreate.ulDensity           = 100;
    tablecreate.rgcolumncreate      = NULL;
    tablecreate.cColumns            = 0;
    tablecreate.rgindexcreate       = NULL;
    tablecreate.cIndexes            = 0;
    tablecreate.szCallback          = NULL;
    tablecreate.cbtyp               = JET_cbtypNull;
    tablecreate.grbit               = NO_GRBIT;
    tablecreate.pSeqSpacehints      = NULL;
    tablecreate.pLVSpacehints       = NULL;
    tablecreate.cbSeparateLV        = 0;
    tablecreate.cbLVChunkMax        = 0;
    tablecreate.tableid             = JET_TABLEID( pfucbNil );
    tablecreate.cCreated            = 0;


    Call( ErrFILECreateTable( m_ppib, pinst->m_mpdbidifmp[ dbidTemp ], &tablecreate ) );
    m_pfucb = (FUCB*)( tablecreate.tableid );
    Assert( m_pfucb != pfucbNil );
    Assert( tablecreate.cCreated == 1 );

    m_pgnoFirst = m_pfucb->u.pfcb->PgnoFDP() + 16;

    //  issue a preread for up to the first 512 pages of the map file
    {
        TraceContextScope tcScope( iortSort );
        tcScope->nParentObjectClass = tceNone;
        tcScope->SetDwEngineObjid( ObjidFDP( m_pfucb ) );

        BFPrereadPageRange( m_pfucb->ifmp, m_pgnoFirst, min( 512, cpg ), bfprfDefault, m_ppib->BfpriPriority( m_pfucb->ifmp ), *tcScope );
    }

    //  allocate a bit-map to represent init space in the array.  the array will
    //  be pre-initialized to zero by the OS
    //
    //  NOTE:  the largest possible amount of memory allocated will be 512KB

    if ( !( m_rgbitInit = (ULONG*)PvOSMemoryPageAlloc( cpg / 8 + 1, NULL ) ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

HandleError:
    return err;
}


//  ================================================================
VOID TTARRAY::BeginRun( PIB* const ppib, RUN* const prun )
//  ================================================================
{
    //  nop
}

//  ================================================================
VOID TTARRAY::EndRun( PIB* const ppib, RUN* const prun )
//  ================================================================
{
    //  unlatch existing page if any

    if ( prun->pgno != pgnoNull )
    {
        if ( prun->fWriteLatch )
        {
            BFWriteUnlatch( &prun->bfl );
        }
        else
        {
            BFRDWUnlatch( &prun->bfl );
        }
        prun->pgno = pgnoNull;
    }
}

//  ================================================================
ERR TTARRAY::ErrSetValue( PIB * const ppib, const ULONG ulEntry, const ULONG ulValue, RUN* const prun )
//  ================================================================
{
    ERR err = JET_errSuccess;

    if( ulEntry >= m_culEntries )
    {
        return ErrERRCheck( JET_errRecordNotFound );
    }

    //  compute the pgno that this entry lives on

    const PGNO pgno = m_pgnoFirst + ulEntry / ( g_cbPage / sizeof( DWORD ) );

    //  we do not already have this page latched

    RUN runT;
    runT.pgno           = pgnoNull;
    runT.bfl.pv         = NULL;
    runT.bfl.dwContext  = NULL;
    runT.fWriteLatch    = fFalse;
    RUN * const  prunT = prun ? prun : &runT;
    PIBTraceContextScope tcRef = ppib->InitTraceContextScope( );
    tcRef->nParentObjectClass = tceNone;

    if ( prunT->pgno != pgno )
    {
        //  unlatch existing page if any

        if ( prunT->pgno != pgnoNull )
        {
            if ( prunT->fWriteLatch )
            {
                BFWriteUnlatch( &prunT->bfl );
            }
            else
            {
                BFRDWUnlatch( &prunT->bfl );
            }
            prunT->pgno         = pgnoNull;
            prunT->fWriteLatch  = fFalse;
        }

        //  latch the new page

        Call( ErrBFRDWLatchPage( &prunT->bfl, m_pfucb->ifmp, pgno, bflfDefault, ppib->BfpriPriority( m_pfucb->ifmp ), *tcRef ) );
        err = JET_errSuccess;   // clobber warnings.
        prunT->pgno         = pgno;
        prunT->fWriteLatch  = fFalse;
    }

    if( fFalse == prunT->fWriteLatch )
    {
        CallS( ErrBFUpgradeRDWLatchToWriteLatch( &prunT->bfl ) );
        prunT->fWriteLatch  = fTrue;
    }

    //  the page is not yet init

    if ( !FPageInit( pgno ) )
    {
        //  init the page

        DWORD iEntryMax;
        iEntryMax = g_cbPage / sizeof( DWORD );

        DWORD iEntry;
        for ( iEntry = 0; iEntry < iEntryMax; iEntry++ )
        {
            *( (ULONG*) prunT->bfl.pv + iEntry ) = m_ulDefault;
        }

        //  mark the page as init

        SetPageInit( pgno );
    }

    //  set the value on the page

    DWORD iEntry;
    iEntry = ulEntry % ( g_cbPage / sizeof( DWORD ) );

    *( (ULONG*) prunT->bfl.pv + iEntry ) = ulValue;

    BFDirty( &prunT->bfl, bfdfDirty, *tcRef );

HandleError:
    if ( runT.pgno != pgnoNull )
    {
        BFWriteUnlatch( &runT.bfl );
    }
    return err;
}


//  ================================================================
ERR TTARRAY::ErrGetValue( PIB * const ppib, const ULONG ulEntry, ULONG * const pulValue, RUN* const prun ) const
//  ================================================================
{
    ERR err = JET_errSuccess;

    if( ulEntry >= m_culEntries )
    {
        *pulValue = m_ulDefault;
        return JET_errSuccess;
    }

    //  compute the pgno that this entry lives on

    const PGNO pgno = m_pgnoFirst + ulEntry / ( g_cbPage / sizeof( DWORD ) );

    //  we do not already have this page latched

    RUN runT;
    runT.pgno           = pgnoNull;
    runT.bfl.pv         = NULL;
    runT.bfl.dwContext  = NULL;
    runT.fWriteLatch    = fFalse;
    RUN * const  prunT = prun ? prun : &runT;
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = tceNone;

    if ( prunT->pgno != pgno )
    {
        //  unlatch existing page if any

        if ( prunT->pgno != pgnoNull )
        {
            if ( prunT->fWriteLatch )
            {
                BFWriteUnlatch( &prunT->bfl );
            }
            else
            {
                BFRDWUnlatch( &prunT->bfl );
            }
            prunT->pgno         = pgnoNull;
            prunT->fWriteLatch  = fFalse;
        }

        //  latch the new page
        //  if we are not part of a run, just use a read-latch for speed and concurrency
        //  use we are part of a run use a RDW latch for consistency, but still allow reads

        if( prun )
        {
            Call( ErrBFRDWLatchPage( &prunT->bfl, m_pfucb->ifmp, pgno, bflfDefault, ppib->BfpriPriority( m_pfucb->ifmp ), *tcScope ) );
            err = JET_errSuccess;   // clobber warnings.
            prunT->fWriteLatch  = fFalse;
        }
        else
        {
            Assert( prunT == &runT );
            Call( ErrBFReadLatchPage( &prunT->bfl, m_pfucb->ifmp, pgno, bflfDefault, ppib->BfpriPriority( m_pfucb->ifmp ), *tcScope ) );
            err = JET_errSuccess;   // clobber warnings.
            prunT->fWriteLatch  = fFalse;
        }

        prunT->pgno = pgno;
    }

    //  this page is initialized

    if ( FPageInit( pgno ) )
    {
        //  retrieve the value from the page

        DWORD iEntry;
        iEntry = ulEntry % ( g_cbPage / sizeof( DWORD ) );

        *pulValue = *( (ULONG*) prunT->bfl.pv + iEntry );
    }

    //  this page is not initialized

    else
    {
        //  the answer is the default value

        *pulValue = m_ulDefault;
    }

HandleError:
    if ( runT.pgno != pgnoNull )
    {
        BFReadUnlatch( &runT.bfl );
    }
    return err;
}


//  ================================================================
ULONG TTARRAY::CEntries() const
//  ================================================================
{
    return m_culEntries;
}


//  ================================================================
BOOL TTARRAY::FPageInit( const PGNO pgno ) const
//  ================================================================
{
    const ULONG iPage   = pgno - m_pgnoFirst;
    const ULONG iul     = iPage / 32;
    const ULONG ibit    = iPage % 32;

    return ( m_rgbitInit[ iul ] & ( 1 << ibit ) ) != 0;
}


//  ================================================================
VOID TTARRAY::SetPageInit( const PGNO pgno )
//  ================================================================
{
    Assert( !FPageInit( pgno ) );

    const ULONG iPage   = pgno - m_pgnoFirst;
    const ULONG iul     = iPage / 32;
    const ULONG ibit    = iPage % 32;

    ULONG ulBIExpected;
    ULONG ulBI;
    ULONG ulAI;
    do  {
        ulBIExpected = m_rgbitInit[ iul ];
        ulAI = ulBIExpected | ( 1 << ibit );

        ulBI = AtomicCompareExchange( (LONG*)&m_rgbitInit[ iul ], ulBIExpected, ulAI );
    }
    while ( ulBI != ulBIExpected );

    Assert( FPageInit( pgno ) );
}


