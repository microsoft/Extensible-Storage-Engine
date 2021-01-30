// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "logstd.hxx"
#include "_logredomap.hxx"


RedoMapEntry::RedoMapEntry()
{
    lgpos = lgposMax;
    dbtimeBefore = dbtimeNil;
    dbtimePage = dbtimeNil;
    dbtimeAfter = dbtimeNil;
}

RedoMapEntry::RedoMapEntry(
            __in const LGPOS& lgposNew,
            __in const ERR errNew,
            __in const DBTIME dbtimeBeforeNew,
            __in const DBTIME dbtimePageNew,
            __in const DBTIME dbtimeAfterNew )
{
    lgpos = lgposNew;
    err = errNew;
    dbtimeBefore = dbtimeBeforeNew;
    dbtimePage = dbtimePageNew;
    dbtimeAfter = dbtimeAfterNew;
}



ERR CLogRedoMap::ErrToErr( __in const RedoMapTree::ERR err )
{
    switch( err )
    {
        case RedoMapTree::ERR::errSuccess:         return JET_errSuccess;
        case RedoMapTree::ERR::errOutOfMemory:     return ErrERRCheck( JET_errOutOfMemory );
        case RedoMapTree::ERR::errEntryNotFound:   return ErrERRCheck( JET_errRecordNotFound );
        case RedoMapTree::ERR::errDuplicateEntry:  return ErrERRCheck( JET_errKeyDuplicate );
        default:                                   Assert( fFalse );
    }

    return ErrERRCheck( errCodeInconsistency );
}

CLogRedoMap::CLogRedoMap()
{
    ClearLogRedoMap();
}

CLogRedoMap::~CLogRedoMap()
{
    AssertSz( NULL == m_rmt, "CLogRedoMap::TermLogRedoMap was not called!" );
}

VOID CLogRedoMap::ClearLogRedoMap()
{
    m_rmt = NULL;
    m_ifmp = ifmpNil;
}

ERR CLogRedoMap::ErrInitLogRedoMap( const IFMP ifmp )
{
    ERR err = JET_errSuccess;

    m_ifmp = ifmp;

    Alloc( m_rmt = new RedoMapTree() );

HandleError:
    if ( err < JET_errSuccess )
    {
        delete m_rmt;
        ClearLogRedoMap();
    }

    return err;
}

VOID CLogRedoMap::TermLogRedoMap()
{
    if ( m_rmt != NULL )
    {
        m_rmt->MakeEmpty();
        delete m_rmt;
    }

    ClearLogRedoMap();
}

BOOL CLogRedoMap::FLogRedoMapEnabled() const
{
    return ( m_rmt != NULL );
}

BOOL CLogRedoMap::FPgnoSet( __in const PGNO pgno ) const
{
    ERR err = JET_errSuccess;
    BOOL fSet = fFalse;

    Assert( pgno != pgnoNull );

    Alloc( m_rmt );

    err = ErrToErr( m_rmt->ErrFind( pgno ) );
    if ( err == JET_errSuccess )
    {
        fSet = fTrue;
    }
    else
    {
        Assert( err == JET_errRecordNotFound );
        err = JET_errSuccess;
    }

HandleError:
    CallSx( err, JET_errOutOfMemory );

#ifdef DEBUG
    if ( err < JET_errSuccess )
    {
        Assert( !fSet );
    }
#endif

    return fSet;
}

BOOL CLogRedoMap::FAnyPgnoSet() const
{
    ERR err = JET_errSuccess;
    BOOL fSet = fFalse;

    Alloc( m_rmt );
    fSet = !m_rmt->FEmpty();

HandleError:
    CallSx( err, JET_errOutOfMemory );

    return fSet;
}

ERR CLogRedoMap::ErrSetPgno( __in const PGNO pgno, __in const LGPOS& lgpos )
{
    return ErrSetPgno( pgno, lgpos, JET_errSuccess, dbtimeNil, dbtimeNil, dbtimeNil );
}

ERR CLogRedoMap::ErrSetPgno(
    __in const PGNO pgno,
    __in const LGPOS& lgpos,
    __in const ERR errNew,
    __in const DBTIME dbtimeBefore,
    __in const DBTIME dbtimePage,
    __in const DBTIME dbtimeAfter )
{
    ERR err = JET_errSuccess;
    RedoMapEntry rme( lgpos, errNew, dbtimeBefore, dbtimePage, dbtimeAfter );

    Alloc( m_rmt );

    err = ErrToErr( m_rmt->ErrInsert( pgno, rme ) );
    if ( err == JET_errSuccess )
    {
        OSTrace(
            JET_tracetagRecoveryValidation,
            OSFormat( "Log redo map [ifmp:%d] set - Pgno %I32u, LGPOS (%08I32X,%04hX,%04hX), err %d, dbtimeBefore 0x%I64x, dbtimePage 0x%I64x, dbtimeAfter 0x%I64x.",
            (int)m_ifmp, pgno,
            lgpos.lGeneration, lgpos.isec, lgpos.ib,
            errNew,
            dbtimeBefore, dbtimePage, dbtimeAfter ) );
    }
    else if ( err == JET_errKeyDuplicate )
    {
        err = JET_errSuccess;
    }
    Call( err );

HandleError:
    CallSx( err, JET_errOutOfMemory );

    return err;
}

VOID CLogRedoMap::ClearPgno(  __in PGNO pgnoStart, __in PGNO pgnoEnd )
{
    ERR err = JET_errSuccess;
    Assert( pgnoStart <= pgnoEnd );

    Alloc( m_rmt );

    PGNO pgnoCurrent = pgnoStart;
    PGNO pgnoFound = pgnoNull;
    while ( ( err = ErrToErr( m_rmt->ErrFindGreaterThanOrEqual( pgnoCurrent, &pgnoFound ) ) ) == JET_errSuccess )
    {
        Assert( pgnoFound >= pgnoCurrent );
        if ( pgnoFound <= pgnoEnd )
        {
            CallS( ErrToErr( m_rmt->ErrDelete( pgnoFound ) ) );
            OSTrace(
                JET_tracetagRecoveryValidation,
                OSFormat( "Log redo map [ifmp:%d] cleared - Pgno %I32u.",
                (int)m_ifmp, pgnoFound ) );
        }
        else
        {
            break;
        }

        pgnoCurrent = pgnoFound + 1;
    }

    Assert( ( err == JET_errSuccess ) || ( err == JET_errRecordNotFound ) );
    Assert( ( err == JET_errRecordNotFound ) || ( pgnoFound > pgnoEnd ) );
    err = JET_errSuccess;

HandleError:
    CallSx( err, JET_errOutOfMemory );
}

VOID CLogRedoMap::GetOldestLgposEntry( __out PGNO* const ppgno, __out RedoMapEntry* const prme, __out CPG* const pcpg ) const
{
    Assert( FAnyPgnoSet() );

    ERR err = JET_errSuccess;
    PGNO pgnoCurrent = 1;
    PGNO pgnoFound = pgnoNull;
    RedoMapEntry rmeFound;
    PGNO pgnoOldestLgpos = pgnoNull;
    RedoMapEntry rmeOldestLgpos;
    rmeOldestLgpos.lgpos = lgposMax;
    CPG cpg = 0;

    while ( ( err = ErrToErr( m_rmt->ErrFindGreaterThanOrEqual( pgnoCurrent, &pgnoFound, &rmeFound ) ) ) == JET_errSuccess )
    {
        cpg++;
        Assert( pgnoFound >= pgnoCurrent );
        const INT cmp = CmpLgpos( rmeFound.lgpos, rmeOldestLgpos.lgpos );
        if ( cmp < 0 )
        {
            pgnoOldestLgpos = pgnoFound;
            rmeOldestLgpos = rmeFound;
        }

        pgnoCurrent = pgnoFound + 1;
    }

    Assert( err == JET_errRecordNotFound );
    Assert( cpg > 0 );
    Assert( pgnoOldestLgpos != pgnoNull );
    Assert( CmpLgpos( rmeOldestLgpos.lgpos, lgposMin ) > 0 );
    Assert( CmpLgpos( rmeOldestLgpos.lgpos, lgposMax ) < 0 );

    *ppgno = pgnoOldestLgpos;
    *prme = rmeOldestLgpos;
    *pcpg = cpg;
}
