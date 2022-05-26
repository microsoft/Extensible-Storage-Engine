// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_logredomap.hxx"

//  ================================================================
//  DBEnforce
//  ================================================================

VOID ReportDBEnforceFailure( const WCHAR * wszContext, const CHAR* szMessage, const WCHAR * wszIssueSource )
{
    const WCHAR *   rgszT[3] = { wszIssueSource, WszUtilImageBuildClass(), wszContext };

    UtilReportEvent(
            eventError,
            GENERAL_CATEGORY,
            ENFORCE_DB_FAIL,
            3,
            rgszT );
}

extern VOID (*g_pfnReportEnforceFailure)(  const WCHAR * wszContext, const CHAR* szMessage, const WCHAR* wszIssueSource );

VOID __stdcall DBEnforceFail( const IFMP ifmp, const CHAR* szMessage, const CHAR* szFilename, LONG lLine )
{
    if ( DefaultReportEnforceFailure == g_pfnReportEnforceFailure )
    {
        g_pfnReportEnforceFailure = ReportDBEnforceFailure;
    }

    const WCHAR * const wszDbName = ( ifmp >= cfmpReserved && ifmp < g_ifmpMax ) ? g_rgfmp[ifmp].WszDatabaseName() : L"UnsetIfmpUnknownDatabase";
    EnforceContextFail( wszDbName, szMessage, szFilename, lLine );
}
    
//  ================================================================
//  DbfilehdrLock
//  ================================================================

DbfilehdrLock::DbfilehdrLock() :
    m_rwl(CLockBasicInfo(CSyncBasicInfo(szDbfilehdr), rankDbfilehdr, 0))
{
    m_ptlsWriter = NULL;
    m_cRecursion = 0;
    SetPdbfilehdr(NULL);
}

DbfilehdrLock::~DbfilehdrLock()
{
}

DBFILEHDR * DbfilehdrLock::SetPdbfilehdr(DBFILEHDR * const pdbfilehdr)
{
    EnterAsWriter();
    DBFILEHDR * const pdbfilehdrPrev = m_pdbfilehdr;
    m_pdbfilehdr = pdbfilehdr;
    LeaveAsWriter();
    return pdbfilehdrPrev;
}

DBFILEHDR * DbfilehdrLock::Pdbfilehdr()
{
    return m_pdbfilehdr;
}

const DBFILEHDR * DbfilehdrLock::Pdbfilehdr() const
{
    return m_pdbfilehdr;
}

void DbfilehdrLock::EnterAsReader()
{
    m_rwl.EnterAsReader();
}

void DbfilehdrLock::LeaveAsReader()
{
    m_rwl.LeaveAsReader();
}

void DbfilehdrLock::EnterAsWriter()
{
    TLS* ptls = Ptls();
    if ( m_ptlsWriter == ptls )
    {
        //  We don't think this happens anymore.  To be safe, allow recursion to proceed
        //  without blocking but report it.
        FireWall( "DbfilehdrLockReentrance" );
        m_cRecursion++;
    }
    else
    {
        m_rwl.EnterAsWriter();
        m_ptlsWriter = ptls;
        m_cRecursion = 1;
    }
}

void DbfilehdrLock::LeaveAsWriter()
{
    Assert( m_ptlsWriter == Ptls() );
    m_cRecursion--;
    if ( !m_cRecursion )
    {
        m_ptlsWriter = NULL;
        m_rwl.LeaveAsWriter();
    }
}

PdbfilehdrReadOnly DbfilehdrLock::GetROHeader()
{
    return PdbfilehdrReadOnly(this);
}

PdbfilehdrReadWrite DbfilehdrLock::GetRWHeader()
{
    return PdbfilehdrReadWrite(this);
}


//  ================================================================
//  PdbfilehdrLocked
//  ================================================================

PdbfilehdrLocked::PdbfilehdrLocked(DbfilehdrLock * const plock) :
    m_plock(plock)
{
}

PdbfilehdrLocked::PdbfilehdrLocked(PdbfilehdrLocked& rhs) :
    m_plock(rhs.m_plock)
{
    rhs.m_plock = NULL;
}

PdbfilehdrLocked::~PdbfilehdrLocked()
{
}
    
PdbfilehdrLocked::operator void const*() const
{
    return get();
}

const DBFILEHDR * PdbfilehdrLocked::operator->() const
{
    return get();
}

const DBFILEHDR * PdbfilehdrLocked::get() const
{
    return m_plock->Pdbfilehdr();
}

DbfilehdrLock * PdbfilehdrLocked::Plock()
{
    return m_plock;
}
    

//  ================================================================
//  PdbfilehdrReadOnly
//  ================================================================

PdbfilehdrReadOnly::PdbfilehdrReadOnly(DbfilehdrLock * const plock) :
    PdbfilehdrLocked(plock)
{
    Plock()->EnterAsReader();
}

PdbfilehdrReadOnly::PdbfilehdrReadOnly(PdbfilehdrReadOnly& rhs) :
    PdbfilehdrLocked(rhs)
{
}

PdbfilehdrReadOnly::~PdbfilehdrReadOnly()
{
    if ( Plock() )
    {
        Plock()->LeaveAsReader();
    }
}


//  ================================================================
//  PdbfilehdrReadWrite
//  ================================================================

PdbfilehdrReadWrite::PdbfilehdrReadWrite(DbfilehdrLock * const plock) :
    PdbfilehdrLocked(plock)
{
    Plock()->EnterAsWriter();
}

PdbfilehdrReadWrite::PdbfilehdrReadWrite(PdbfilehdrReadWrite& rhs) :
    PdbfilehdrLocked(rhs)
{
}

PdbfilehdrReadWrite::~PdbfilehdrReadWrite()
{
    if ( Plock() )
    {
        Plock()->LeaveAsWriter();
    }
}

DBFILEHDR * PdbfilehdrReadWrite::operator->()
{
    return get();
}

DBFILEHDR * PdbfilehdrReadWrite::get()
{
    return Plock()->Pdbfilehdr();
}


//  ================================================================
//  DbfilehdrLock tests
//  ================================================================

JETUNITTEST( DbfilehdrLock, ConstructorZeroesPdbfilehdr )
{
    DbfilehdrLock dbfilehdrlock;
    CHECK(NULL == dbfilehdrlock.SetPdbfilehdr(NULL));
}

JETUNITTEST( DbfilehdrLock, SetPdbfilehdr )
{
    DBFILEHDR dbfilehdr;
    DbfilehdrLock dbfilehdrlock;
    dbfilehdrlock.SetPdbfilehdr(&dbfilehdr);
    CHECK(&dbfilehdr == dbfilehdrlock.SetPdbfilehdr(NULL));
}

JETUNITTEST( DbfilehdrLock, GetReturnsHeader )
{
    DBFILEHDR dbfilehdr;
    DbfilehdrLock dbfilehdrlock;
    dbfilehdrlock.SetPdbfilehdr(&dbfilehdr);
    CHECK(&dbfilehdr == dbfilehdrlock.GetROHeader().get());
}

JETUNITTEST( DbfilehdrLock, ImplicitRO )
{
    const OBJID objid = 17;
    DBFILEHDR dbfilehdr;
    dbfilehdr.le_objidLast = objid;
    DbfilehdrLock dbfilehdrlock;
    dbfilehdrlock.SetPdbfilehdr(&dbfilehdr);

    // this calls operator-> on a temporary PdbfilehdrReadOnly
    CHECK(objid == dbfilehdrlock.GetROHeader()->le_objidLast);
}

JETUNITTEST( DbfilehdrLock, ExplicitRO )
{
    const OBJID objid = 19;
    DBFILEHDR dbfilehdr;
    dbfilehdr.le_objidLast = objid;
    DbfilehdrLock dbfilehdrlock;
    dbfilehdrlock.SetPdbfilehdr(&dbfilehdr);

    PdbfilehdrReadOnly pdbfilehdr = dbfilehdrlock.GetROHeader();
    CHECK(objid == pdbfilehdr->le_objidLast);
}

JETUNITTEST( DbfilehdrLock, ImplicitRW )
{
    const OBJID objid = 21;
    DBFILEHDR dbfilehdr;
    DbfilehdrLock dbfilehdrlock;
    dbfilehdrlock.SetPdbfilehdr(&dbfilehdr);

    // this calls operator-> on a temporary PdbfilehdrReadWrite
    dbfilehdrlock.GetRWHeader()->le_objidLast = objid;
    CHECK(objid == dbfilehdr.le_objidLast);
}

JETUNITTEST( DbfilehdrLock, ExplicitRW )
{
    const OBJID objid = 23;
    DBFILEHDR dbfilehdr;
    DbfilehdrLock dbfilehdrlock;
    dbfilehdrlock.SetPdbfilehdr(&dbfilehdr);

    PdbfilehdrReadWrite pdbfilehdr = dbfilehdrlock.GetRWHeader();
    pdbfilehdr->le_objidLast = objid;
    CHECK(objid == dbfilehdr.le_objidLast);
}

/*
JETUNITTEST( DbfilehdrLock, NestedRO )
{
    DBFILEHDR dbfilehdr;
    DbfilehdrLock dbfilehdrlock;
    dbfilehdrlock.SetPdbfilehdr(&dbfilehdr);

    // if the locking is broken this will hang
    PdbfilehdrReadOnly pdbfilehdr1 = dbfilehdrlock.GetROHeader();
    PdbfilehdrReadOnly pdbfilehdr2 = dbfilehdrlock.GetROHeader();
}

JETUNITTEST( DbfilehdrLock, ROandRW )
{
    DBFILEHDR dbfilehdr;
    DbfilehdrLock dbfilehdrlock;
    dbfilehdrlock.SetPdbfilehdr(&dbfilehdr);

    // if the locking is broken this will hang
    PdbfilehdrReadOnly pdbfilehdr1 = dbfilehdrlock.GetROHeader();
    PdbfilehdrReadWrite pdbfilehdr2 = dbfilehdrlock.GetRWHeader();
}
*/


//  ================================================================
FMP::FMP()
//  ================================================================
    :   CZeroInit( sizeof( FMP ) ),
        m_critLatch( CLockBasicInfo( CSyncBasicInfo( szFMP ), rankFMP, 0 ) ),
        m_critDbtime( CLockBasicInfo( CSyncBasicInfo( szDbtime ), rankDbtime, 0 ) ),
        m_gateWriteLatch( CSyncBasicInfo( _T( "FMP::m_gateWriteLatch" ) ) ),
        m_semRangeLock( CSyncBasicInfo( "FMP::m_semRangeLock" ) ),
        m_dbid( dbidMax ),
        m_rwlDetaching( CLockBasicInfo( CSyncBasicInfo( szFMPDetaching ), rankFMPDetaching, 0 ) ),
        m_rwlBFContext( CLockBasicInfo( CSyncBasicInfo( szBFFMPContext ), rankBFFMPContext, 0 ) ),
        m_sxwlRedoMaps( CLockBasicInfo( CSyncBasicInfo( szFMPRedoMaps ), rankFMPRedoMaps, 0 ) ),
        m_semIOSizeChange( CSyncBasicInfo( _T( "FMP::m_semIOSizeChange" ) ) ),
        m_cAsyncIOForViewCachePending( 0 ),
        m_semTrimmingDB( CSyncBasicInfo( _T( "FMP::m_semTrimmingDB" ) ) ),
        m_semDBM( CSyncBasicInfo( _T( "FMP::m_semDBM" ) ) ),
        m_isdlCreate( isdltypeCreate ),
        m_isdlAttach( isdltypeAttach ),
        m_isdlDetach( isdltypeDetach ),
        m_critOpenDbCheck( CLockBasicInfo( CSyncBasicInfo( szOpenDbCheck ), rankOpenDbCheck, 0 ) )

        // TRY NOT TO ADD INITIALIZATION HERE!  Only like critical section contructors or
        // something that might live in the g_rgfmp outside / longer than the normal attached 
        // to detach FMP lifecycle.  All regular state should be initialized in:
        //      ErrInitializeOneFmp().
        // Otherwise you can end up with dirty/stale FMP vars.
{
    m_dataHdrSig.Nullify();
    m_semIOSizeChange.Release();
    m_semRangeLock.Release();
    m_semTrimmingDB.Release();
    m_semDBM.Release();
}

//  ================================================================
FMP::~FMP()
//  ================================================================
{
    Assert( NULL == m_pdbmScan );
    Assert( NULL == m_pdbmFollower );
    Assert( m_cAsyncIOForViewCachePending == 0 );
    Assert( NULL == m_pLogRedoMapZeroed );
    Assert( NULL == m_pLogRedoMapBadDbtime );
    Assert( NULL == m_pLogRedoMapDbtimeRevert );
    Assert( NULL == m_pLogRedoMapDbtimeRevertIgnore );
}

/******************************************************************/
/*              FMP Routines                                      */
/******************************************************************/

INLINE IFMP FMP::IfmpMinInUse()
{
#ifdef DEBUG
    if ( !( ( ( s_ifmpMinInUse == g_ifmpMax ) && ( s_ifmpMacInUse == 0 ) ) || FMP::FAllocatedFmp( s_ifmpMinInUse ) ) )
    {
        EnterFMPPoolAsReader();
        Assert( ( ( s_ifmpMinInUse == g_ifmpMax ) && ( s_ifmpMacInUse == 0 ) ) || FMP::FAllocatedFmp( s_ifmpMinInUse ) );
        LeaveFMPPoolAsReader();
    }
#endif
    return s_ifmpMinInUse;
}
INLINE IFMP FMP::IfmpMacInUse()
{
#ifdef DEBUG
    if ( !( ( ( s_ifmpMinInUse == g_ifmpMax ) && ( s_ifmpMacInUse == 0 ) ) || FMP::FAllocatedFmp( s_ifmpMinInUse ) ) )
    {
        EnterFMPPoolAsReader();
        Assert( ( ( s_ifmpMinInUse == g_ifmpMax ) && ( s_ifmpMacInUse == 0 ) ) || FMP::FAllocatedFmp( s_ifmpMinInUse ) );
        LeaveFMPPoolAsReader();
    }
#endif
    return s_ifmpMacInUse;
}

//  Note: g_rgfmp is only _partially_ committed now.  Ifmp = 0 is always uncommitted, and anything from >= s_ifmpMacCommitted is 
//  also uncommitted and will return false from FMP::FAllocatedFmp().
FMP *   g_rgfmp       = NULL;     //  database file map
IFMP    g_ifmpMax     = 0;        //  note: must also check against FMP::FAllocatedFmp() or s_ifmpMacCommitted
IFMP    g_ifmpTrace = (IFMP)JET_dbidNil;

CReaderWriterLock FMP::rwlFMPPool( CLockBasicInfo( CSyncBasicInfo( szFMPGlobal ), rankFMPGlobal, 0 ) );

/*  The folowing functions only deal with the following fields in FMP
 *      rwlFMPPool & pfmp->szDatabaseName   - protect fmp id (szDatabaseName field)
 *      pfmp->critLatch - protect any fields except szDatabaseName.
 *      pfmp->fWriteLatch
 *      pfmp->gateWriteLatch
 *      pfmp->cPin - when getting fmp pointer for read, the cPin must > 0
 */

VOID FMP::GetWriteLatch( PIB *ppib )
{
    //  At least pinned (except dbidTemp which always sit
    //  in memory) or latched by the caller.

    Assert( ( m_cPin || FIsTempDB() ) || this->FWriteLatchByMe(ppib) );

    m_critLatch.Enter();

Start:
    Assert( m_critLatch.FOwner() );
    if ( !( this->FWriteLatchByOther(ppib) ) )
    {
        this->SetWriteLatch( ppib );
        m_critLatch.Leave();
    }
    else
    {
        m_gateWriteLatch.Wait( m_critLatch );
        m_critLatch.Enter();
        goto Start;
    }
}

VOID FMP::ReleaseWriteLatch( PIB *ppib )
{
    m_critLatch.Enter();

    /*  Free write latch.
     */
    this->ResetWriteLatch( ppib );
    if ( m_gateWriteLatch.CWait() > 0 )
    {
        //  we release only 1 waiter here, to ensure fairness.  When the
        //  first thread Y releases a single waiter here, that thread X 
        //  can go get the write latch and do it's work, and then when 
        //  thread X releases the write latch it will also release 1 more
        //  waiter, so then thread Z is able to get the write latch and 
        //  do it's thing, and upon releasing the write latch it also
        //  releases 1 more waiter, and so on and so on ...
        //  This can all go horribly wrong if a thread that is released
        //  decides to bail without signalling/releasing the next thread 
        //  in the chain.
        m_gateWriteLatch.Release( m_critLatch, 1 );
    }
    else
    {
        m_critLatch.Leave();
    }
}


//  ================================================================
ERR FMP::RegisterTask()
//  ================================================================
{
    AtomicIncrement( (LONG *)&m_ctasksActive );
    return JET_errSuccess;
}


//  ================================================================
ERR FMP::UnregisterTask()
//  ================================================================
{
    AtomicDecrement( (LONG *)&m_ctasksActive );
    return JET_errSuccess;
}


//  ================================================================
VOID FMP::WaitForTasksToComplete()
//  ================================================================
{
    //  very ugly, but hopefully we don't detach often
    while( 0 != m_ctasksActive )
    {
        UtilSleep( cmsecWaitGeneric );
    }
}

#ifdef DEBUG

BOOL FMP::FHeaderUpdatePending() const
{
    BOOL fRet = m_dbhus == dbhusUpdateLogged;

    // Asserts disabled due to:
#if 0
    //  Note: Don't think we need dbhusNoUpdate, as it's probably not called that early.
    Expected( fRet || m_dbhus == dbhusUpdateSet || m_dbhus == dbhusHdrLoaded || m_dbhus == dbhusUpdateFlushed ); // check all states accounted for
    Assert( fRet || m_dbhus == dbhusUpdateSet || m_dbhus == dbhusHdrLoaded || m_dbhus == dbhusUpdateFlushed || m_dbhus == dbhusNoUpdate ); // check all states accounted for
#endif // 0

    return fRet;
}

BOOL FMP::FHeaderUpdateInProgress() const
{
    BOOL fRet = m_dbhus == dbhusUpdateLogged || m_dbhus == dbhusUpdateSet;

    // Asserts disabled due to:
#if 0
    //  Note: Don't think we need dbhusNoUpdate, as it's probably not called that early.
    Expected( fRet || m_dbhus == dbhusHdrLoaded || m_dbhus == dbhusUpdateFlushed ); // check all states accounted for
    Assert( fRet || m_dbhus == dbhusHdrLoaded || m_dbhus == dbhusUpdateFlushed || m_dbhus == dbhusNoUpdate ); // check all states accounted for
#endif // 0

    return fRet;
}

VOID FMP::SetDbHeaderUpdateState( _In_ const DbHeaderUpdateState dbhusNewState )
{
    //  When we do an update to a page flush, we do a pattern of LatchPage, LogUpdate, SetLgposModify, 
    //  and then ReleaseLatch ... this ensures crash consistency as the log will be written out before
    //  the DB page update by virtual of the latch being held throughout.  The DB Header doesn't go 
    //  through the buffer manager, but we still want some new logged updates to basically be written 
    //  to log before we write/flush the DB header (such as the DbVersion updates).
    //
    //  If we did it wrong and wrote the hdr file update before logged operation we could end up in a 
    //  nasty crash where the logged oper was lost and hdr is updated.  This may be able to cause some
    //  copies to diverge on wha they thing the DB versions are.
    //

    Assert( dbhusNewState != dbhusNoUpdate );

    // Asserts disabled due to:
#if 0
    const BOOL fLoggingNeeded = FLogOn() &&
                                    //  "not in redo" ...
                                    !( m_pinst->FRecovering() && m_pinst->m_plog->FRecoveringMode() == fRecoveringRedo );

    switch( dbhusNewState )
    {
    case dbhusHdrLoaded:
        Assert( m_dbhus == dbhusNoUpdate );
        break;

    case dbhusUpdateLogged:
        //  Must start from never done an update or possibly previously completed an update cycle.
        Assert( m_dbhus == dbhusHdrLoaded ||        //  very first update cycle
                // Add this if we decide to log multiple things in one update cycle
                //m_dbhus == dbhusUpdateLogged ||
                m_dbhus == dbhusUpdateFlushed );    //  a post write update or subsequent update cycle (like from attach after recovery with keep attachements)
        break;

    case dbhusUpdateSet:
        Assert( m_dbhus == dbhusUpdateLogged ||     //  the basic / normal case, we set what we just logged
                m_dbhus == dbhusUpdateFlushed ||    //  a subsequent update cycle
                m_dbhus == dbhusUpdateSet  );       //  a 2nd sub-update within an update cycle (like redirtying a page)

        Assert( m_dbhus == dbhusUpdateLogged ||     //  the basic / normal case, we set what we just logged
                ( m_dbhus == dbhusUpdateFlushed && !fLoggingNeeded ) || //  a subsequent update cycle
                m_dbhus == dbhusUpdateSet  );       //  a 2nd sub-update within an update cycle (like redirtying a page)
        break;

    case dbhusUpdateFlushed:
        Assert( m_dbhus != dbhusUpdateLogged || !fLoggingNeeded ); // we should be in dbhusUpdateSet state if the the log is on.
        Assert( m_dbhus == dbhusHdrLoaded || m_dbhus == dbhusUpdateSet || m_dbhus == dbhusUpdateFlushed ||
                    ( m_dbhus == dbhusUpdateLogged && !fLoggingNeeded ) );  // 2nd way to check the same thing.
        Assert( m_dbhus == dbhusHdrLoaded || m_dbhus == dbhusUpdateSet || m_dbhus == dbhusUpdateFlushed );  // 2nd way to check the same thing.
        Assert( m_dbhus != dbhusNoUpdate );
        break;

    default:
        AssertSz( fFalse, "Unknown new state (%d), and dbhusNoUpdate not expected.", (DWORD)dbhusNewState );
    }
#else
    switch( dbhusNewState )
    {
    case dbhusHdrLoaded:
        Assert( m_dbhus == dbhusNoUpdate );
        break;

    case dbhusUpdateLogged:
    case dbhusUpdateSet:
    case dbhusUpdateFlushed:
        break;

    default:
        AssertSz( fFalse, "Unknown new state (%d), and dbhusNoUpdate not expected.", (DWORD)dbhusNewState );
    }
#endif // 0

    AtomicExchange( (LONG*)&m_dbhus, dbhusNewState );
}
#endif // DEBUG


ERR ErrDBFormatFeatureEnabled_( const JET_ENGINEFORMATVERSION efvFormatFeature, const DbVersion& dbvCurrentFromFile );

ERR FMP::ErrDBFormatFeatureEnabled( const DBFILEHDR* const pdbfilehdr, const JET_ENGINEFORMATVERSION efvFormatFeature )
{
    Assert( pdbfilehdr != NULL );
    Assert( ( m_dbfilehdrLock.m_pdbfilehdr == NULL ) || ( m_dbfilehdrLock.m_pdbfilehdr == pdbfilehdr ) );

    const DbVersion dbvHdr = pdbfilehdr->Dbv();
    const ERR err = ErrDBFormatFeatureEnabled_( efvFormatFeature, dbvHdr );
    if ( err == JET_errEngineFormatVersionParamTooLowForRequestedFeature )
    {
        if ( m_lesEventDbFeatureDisabled.FNeedToLog( efvFormatFeature ) )
        {
            /*  

            Windows reports this event is too noisy, and we do not need it in Exch Datacenter.  Occaionally useful for ESE development.

            WCHAR wszEfvSetting[cchFormatEfvSetting];
            WCHAR wszEfvDesired[60];
            WCHAR wszEfvCurrent[60];
            const WCHAR * rgszT[3] = { wszEfvSetting, wszEfvDesired, wszEfvCurrent };
            
            FormatEfvSetting( (JET_ENGINEFORMATVERSION)UlParam( m_pinst, JET_paramEngineFormatVersion ), wszEfvSetting, sizeof(wszEfvSetting) );
            OSStrCbFormatW( wszEfvDesired, _cbrg( wszEfvDesired ), L"%d (0x%x)", efvFormatFeature, efvFormatFeature );
            OSStrCbFormatW( wszEfvCurrent, _cbrg( wszEfvCurrent ), L"%d.%d.%d", dbvHdr.ulDbMajorVersion, dbvHdr.ulDbUpdateMajor, dbvHdr.ulDbUpdateMinor );
            
            UtilReportEvent(
                    eventWarning,
                    GENERAL_CATEGORY,
                    DATABASE_FORMAT_FEATURE_REFUSED_DUE_TO_ENGINE_FORMAT_VERSION_SETTING,
                    _countof( rgszT ),
                    rgszT,
                    0,
                    NULL,
                    m_pinst );
            */
        }
        
    }
    return err;
}

ERR FMP::ErrDBFormatFeatureEnabled( const JET_ENGINEFORMATVERSION efvFormatFeature )
{
    return ErrDBFormatFeatureEnabled( Pdbfilehdr().get(), efvFormatFeature );
}

ERR ErrFMFormatFeatureEnabled( const JET_ENGINEFORMATVERSION efvFormatFeature, const GenVersion& fmvCurrentFromFile )
{
    //  Fast path - check assuming all persisted versions are current!
    //
    const FormatVersions * pfmtversMax = PfmtversEngineMax();

    Expected( efvFormatFeature <= pfmtversMax->efv ); // any EFV / feature to check should be < than the engines last EFV in the table.
    if ( efvFormatFeature <= pfmtversMax->efv &&
            //  then _most likely_ we're on the current version (as setting EFVs back is usually temporary) ...
            0 == CmpFormatVer( &pfmtversMax->fmv, &fmvCurrentFromFile ) )
    {
        OSTrace( JET_tracetagVersionAndStagingChecks, OSFormat( "FM Feature requested EFV %d success (fast) ... { %d ... %d.%d.%d == %d.%d.%d }\n",
                    efvFormatFeature, pfmtversMax->efv,
                    pfmtversMax->fmv.ulVersionMajor, pfmtversMax->fmv.ulVersionUpdateMajor, pfmtversMax->fmv.ulVersionUpdateMinor,
                    fmvCurrentFromFile.ulVersionMajor, fmvCurrentFromFile.ulVersionUpdateMajor, fmvCurrentFromFile.ulVersionUpdateMinor ) );
        return JET_errSuccess;  //  yay! fast out ...
    }
    //  else the header version didn't match and/or EFV is lower than last EFV in table ... do slow method

    //  Slow path - search version table for EFV and check directly ...
    //

    const FormatVersions * pfmtversFeatureVersion = NULL;
    CallS( ErrGetDesiredVersion( NULL /* must be NULL to bypass staging */, efvFormatFeature, &pfmtversFeatureVersion ) );
    if ( pfmtversFeatureVersion )
    {
        if ( CmpFormatVer( &fmvCurrentFromFile, &pfmtversFeatureVersion->fmv ) >= 0 )
        {
            OSTrace( JET_tracetagVersionAndStagingChecks, OSFormat( "FM Feature requested EFV %d success ... { %d ... %d.%d.%d == %d.%d.%d }\n",
                        efvFormatFeature, pfmtversMax->efv,
                        pfmtversFeatureVersion->fmv.ulVersionMajor, pfmtversFeatureVersion->fmv.ulVersionUpdateMajor, pfmtversFeatureVersion->fmv.ulVersionUpdateMinor,
                        fmvCurrentFromFile.ulVersionMajor, fmvCurrentFromFile.ulVersionUpdateMajor, fmvCurrentFromFile.ulVersionUpdateMinor ) );
            return JET_errSuccess;
        }
        else
        {
            const ERR errRet = ErrERRCheck( JET_errEngineFormatVersionParamTooLowForRequestedFeature );
            //  Note no event, probably fine for FM.
            OSTrace( JET_tracetagVersionAndStagingChecks, OSFormat( "FM Feature requested EFV = %d { %d ... %d.%d.%d == %d.%d.%d } FAILED: %d.\n",
                        efvFormatFeature, pfmtversMax->efv,
                        pfmtversFeatureVersion->fmv.ulVersionMajor, pfmtversFeatureVersion->fmv.ulVersionUpdateMajor, pfmtversFeatureVersion->fmv.ulVersionUpdateMinor,
                        fmvCurrentFromFile.ulVersionMajor, fmvCurrentFromFile.ulVersionUpdateMajor, fmvCurrentFromFile.ulVersionUpdateMinor,
                        errRet ) );
            return errRet;
        }
    }

    AssertTrack( fFalse, "UnknownFmFormatFeatureDisabled" );

    return ErrERRCheck( errCodeInconsistency );
}


//  ================================================================
ERR FMP::ErrCreateFlushMap( const JET_GRBIT grbit )
//  ================================================================
{
    ERR err = JET_errSuccess;
    CFlushMapForAttachedDb* pflushmap = NULL;

    Assert( NULL == m_pflushmap );
    
    JET_ENGINEFORMATVERSION efvDesired = (JET_ENGINEFORMATVERSION)UlParam( m_pinst, JET_paramEngineFormatVersion );
    if ( efvDesired == JET_efvUsePersistedFormat )
    {
        // This is pre- JET_bitPersistedLostFlushDetectionEnabled, so it disables ...
        efvDesired = JET_efvExchange2016Rtm;
    }
    const FormatVersions * pfmtversDesired = NULL;
    err = ErrGetDesiredVersion( m_pinst, efvDesired, &pfmtversDesired );
    CallS( err );
    Call( err );
    
    const ULONG_PTR ulParam = UlParam( Pinst(), JET_paramPersistedLostFlushDetection );
    const BOOL fPersisted = ( ( ulParam & JET_bitPersistedLostFlushDetectionEnabled ) != 0 &&
                                ( ErrFMFormatFeatureEnabled( JET_efvPersistedLostFlushMapStagedToDebug, pfmtversDesired->fmv ) >= JET_errSuccess ) );
    const BOOL fCreateNew = ( ( ulParam & JET_bitPersistedLostFlushDetectionCreateNew ) != 0 ) || ( ( grbit & JET_bitDbOverwriteExisting ) != 0 );


    Alloc( pflushmap = new CFlushMapForAttachedDb() );
    pflushmap->SetDbFmp( this );
    pflushmap->SetPersisted( fPersisted );
    pflushmap->SetCreateNew( fCreateNew );
    Call( pflushmap->ErrInitFlushMap() );

    m_pflushmap = pflushmap;
    pflushmap = NULL;

HandleError:

    delete pflushmap;

    return err;
}

//  ================================================================
VOID FMP::DestroyFlushMap()
//  ================================================================
{
    delete m_pflushmap;
    m_pflushmap = NULL;
}

/*  ErrFMPWriteLatchByNameSz returns the ifmp of the database with the
 *  given name if return errSuccess, otherwise, it return DatabaseNotFound.
 */
ERR FMP::ErrWriteLatchByNameWsz( const WCHAR *wszFileName, IFMP *pifmp, PIB *ppib )
{
    IFMP    ifmp;
    BOOL    fFound = fFalse;

Start:
    EnterFMPPoolAsReader();
    for ( ifmp = IfmpMinInUse(); ifmp <= IfmpMacInUse(); ifmp++ )
    {
        FMP *pfmp = &g_rgfmp[ ifmp ];

        if ( pfmp->FInUse()
            && UtilCmpFileName( wszFileName, pfmp->WszDatabaseName() ) == 0 &&
            // there can be 2 FMP's with same name recovering
            pfmp->Pinst() == PinstFromPpib( ppib ) )
        {
            pfmp->CritLatch().Enter();
            if ( pfmp->FWriteLatchByOther(ppib) )
            {
                /*  found an entry and the entry is write latched.
                 */
                LeaveFMPPoolAsReader();
                pfmp->GateWriteLatch().Wait( pfmp->CritLatch() );
                goto Start;
            }

            /*  found an entry and the entry is not write latched.
             */
            *pifmp = ifmp;
            pfmp->SetWriteLatch( ppib );
            pfmp->CritLatch().Leave();
            fFound = fTrue;
            break;
        }
    }

    LeaveFMPPoolAsReader();

    return ( fFound ? JET_errSuccess : ErrERRCheck( JET_errDatabaseNotFound ) );
}

ERR FMP::ErrWriteLatchByIfmp( IFMP ifmp, PIB *ppib )
{
    FMP *pfmp = &g_rgfmp[ ifmp ];

Start:
    EnterFMPPoolAsReader();

    //  If no identity for this FMP, return.

    if ( !pfmp->FInUse() || PinstFromPpib( ppib ) != pfmp->Pinst() )
    {
        LeaveFMPPoolAsReader();
        return ErrERRCheck( JET_errDatabaseNotFound );
    }

    //  check if it is latchable.

    pfmp->CritLatch().Enter();
    if ( pfmp->FWriteLatchByOther(ppib) )
    {
        /*  found an entry and the entry is write latched.
         */
        LeaveFMPPoolAsReader();
        pfmp->GateWriteLatch().Wait( pfmp->CritLatch() );
        goto Start;
    }

    //  found an entry and the entry is not write latched.

    pfmp->SetWriteLatch( ppib );
    pfmp->CritLatch().Leave();

    LeaveFMPPoolAsReader();

    return JET_errSuccess;
}

//  tries to find an already attached database by the supplied full database path
ERR FMP::ErrResolveByNameWsz( const _In_z_ WCHAR *wszDatabaseName, _Out_writes_z_( cwchFullName ) WCHAR *wszFullName, const size_t cwchFullName )
{
    // we currently only support resolving a database by a full path.  we could have failed to compute the full path of
    // the database elsewhere due to part of that path being invalidated while the database was attached (e.g. delete
    // of a mount point or symbolic link, etc)
    ENTERREADERWRITERLOCK erwl( &rwlFMPPool, fTrue );
    for ( IFMP ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
    {
        FMP * pfmp = &g_rgfmp[ifmp];
        if ( pfmp->FInUse() && 0 == UtilCmpFileName( pfmp->WszDatabaseName(), wszDatabaseName ) )
        {
            return ErrOSStrCbCopyW( wszFullName, sizeof( WCHAR ) * cwchFullName, wszDatabaseName );
        }
    }
    return ErrERRCheck( JET_errDatabaseNotFound );
}

VOID FMP::AcquireIOSizeChangeLatch()
{
    SemIOSizeChange().Acquire();
    OnDebug( AssertSizesConsistent( fTrue ) );
}

VOID FMP::ReleaseIOSizeChangeLatch()
{
    OnDebug( AssertSizesConsistent( fTrue ) );
    SemIOSizeChange().Release();
}

VOID FMP::SetOwnedFileSize( const QWORD cb )
{
    OnDebug( AssertSafeToChangeOwnedSize() );
    (VOID)AtomicExchange( (__int64*)&m_cbOwnedFileSize, (__int64)cb );
    OnDebug( AssertSizesConsistent( fFalse ) );
}

#ifdef DEBUG
BOOL FMP::FIOSizeChangeLatchAvail()
{
    return ( SemIOSizeChange().CAvail() > 0 );
}

VOID FMP::AssertSafeToChangeOwnedSize()
{
#if !defined( ENABLE_JET_UNIT_TEST )
    // If don't own the DB root latch, it must be one of the known single-threaded / exclusive-DB access cases.
    const IOREASONUSER ioru = PetcTLSGetEngineContext()->iorReason.Ioru();
    if ( !FBFWriteLatched( Ifmp(), pgnoSystemRoot ) &&
         !FBFRDWLatched( Ifmp(), pgnoSystemRoot ) &&
         !FBFWARLatched( Ifmp(), pgnoSystemRoot ) )
    {
        Assert( ( FShrinkIsRunning() && ( ioru == opAttachDatabase ) ) ||
                ( FAttachingDB() && ( ioru == opAttachDatabase ) ) ||
                ( FCreatingDB() && ( ( ioru == opCreateDatabase ) || m_pinst->m_plog->FRecovering() ) ) ||
                // Utilities / JET APIs that implicitly have to create or attach DBs.
                ( ( FAttachingDB() || FCreatingDB() ) && ( ( ioru == opDBUtilities ) || ( ioru == opCompact ) || ( m_dbid == dbidTemp ) ) ) ||
                ( FAttachedForRecovery() && ( m_pinst->m_plog->FRecovering() || ( ioru == opAttachDatabase ) ) ) ||
                g_fRepair );
    }
#endif // !ENABLE_JET_UNIT_TEST && DEBUG
}

VOID FMP::AssertSizesConsistent( const BOOL fHasIoSizeLockDebugOnly )
{
    Assert( !fHasIoSizeLockDebugOnly || !FIOSizeChangeLatchAvail() );

    const QWORD cbOwnedFileSize = CbOwnedFileSize();  // AtomicRead( m_cbOwnedFileSize );
    const QWORD cbFsFileSizeAsyncTarget = CbFsFileSizeAsyncTarget();    // AtomicRead( m_cbFsFileSizeAsyncTarget );

    // Cases in which consistency can't be checked:
    //   1- Uninitialized state.
    //   2- Replaying logs without ExtendDB (older log format).
    //   3- Repair also has some corner cases.
    //   4- We are still in the initial required range when recovering a database. Its final
    //      size might be already smaller, but we could have rolled back the checkpoint and therefore
    //      could be replaying an old shrink log record.
    if ( ( ( cbOwnedFileSize == 0 ) && ( cbFsFileSizeAsyncTarget == 0 ) ) ||
         FOlderDemandExtendDb() ||
         g_fRepair ||
         FContainsDataFromFutureLogs() )
    {
        return;
    }

    QWORD cbFsFileSize = 0;
    if ( ( Pfapi() != NULL ) && ( Pfapi()->ErrSize( &cbFsFileSize, IFileAPI::filesizeLogical ) >= JET_errSuccess ) )
    {
        // Owned size can't be larger than physical size.
        Assert( cbOwnedFileSize <= cbFsFileSize );

        // We don't do async file shrinkage, so the target is always over the file system size.
        Assert( !fHasIoSizeLockDebugOnly || ( cbFsFileSize <= cbFsFileSizeAsyncTarget ) );
    }

    // Owned size can't be larger than the target physical size.
    Assert( cbOwnedFileSize <= cbFsFileSizeAsyncTarget );
}
#endif // DEBUG

VOID FMP::GetPeriodicTrimmingDBLatch()
{
    //  wait to own trimming the database

    Assert( g_fPeriodicTrimEnabled );
    SemPeriodicTrimmingDB().Acquire();
}

VOID FMP::ReleasePeriodicTrimmingDBLatch()
{
    //  release ownership of trimming the database

    Assert( g_fPeriodicTrimEnabled );
    SemPeriodicTrimmingDB().Release();
}

LOCAL VOID FMPIBuildRelocatedDbPath(
    IFileSystemAPI * const  pfsapi,
    const WCHAR * const     wszOldPath,
    const WCHAR * const     wszNewDir,
    __out_bcount(OSFSAPI_MAX_PATH * sizeof ( WCHAR )) PWSTR const           wszNewPath )
{
    WCHAR                   wszDirT[ IFileSystemAPI::cchPathMax ];
    WCHAR                   wszFileBaseT[ IFileSystemAPI::cchPathMax ];
    WCHAR                   wszFileExtT[ IFileSystemAPI::cchPathMax ];

    CallS( pfsapi->ErrPathParse(
                        wszOldPath,
                        wszDirT,
                        wszFileBaseT,
                        wszFileExtT ) );

    CallS( pfsapi->ErrPathBuild(
                        wszNewDir,
                        wszFileBaseT,
                        wszFileExtT,
                        wszNewPath ) );
}

ERR FMP::ErrStoreDbName(
    const INST * const      pinst,
    IFileSystemAPI * const  pfsapi,
    const WCHAR *           wszDbName,
    const BOOL              fValidatePaths )
{
    ERR                     err;
    const LOG * const       plog                = pinst->m_plog;
    const BOOL              fAlternateDbDir     = ( plog->FRecovering() && !FDefaultParam( pinst, JET_paramAlternateDatabaseRecoveryPath ) );
    WCHAR *                 wszRelocatedDb      = NULL;
    WCHAR                   wszDbFullName[IFileSystemAPI::cchPathMax];

    Assert( NULL != wszDbName );
    Assert( NULL != plog );

    if ( fAlternateDbDir )
    {
        FMPIBuildRelocatedDbPath(
                    pfsapi,
                    wszDbName,
                    SzParam( pinst, JET_paramAlternateDatabaseRecoveryPath ),
                    wszDbFullName );
        wszRelocatedDb = wszDbFullName;
    }

    else if ( fValidatePaths )
    {
        err = pfsapi->ErrPathComplete( wszDbName, wszDbFullName );
        CallR( err == JET_errInvalidPath ? ErrERRCheck( JET_errDatabaseInvalidPath ) : err );
        wszDbName = wszDbFullName;
    }

    Assert( NULL == wszRelocatedDb || fAlternateDbDir );

    const SIZE_T    cchRelocatedDb  = ( NULL != wszRelocatedDb ? LOSStrLengthW( wszRelocatedDb ) + 1 : 0 );
    const SIZE_T    cchDbName       = cchRelocatedDb + LOSStrLengthW( wszDbName ) + 1;
    const SIZE_T    cchAllocate     = cchRelocatedDb + cchDbName;
    SIZE_T          cbNext          = sizeof( WCHAR ) * cchAllocate;
    WCHAR *         pch             = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbNext ) );
    WCHAR *         pchNext         = pch;

    AllocR( pch );

    Assert( NULL == WszDatabaseName() );

    Assert( FWriterFMPPool() );
    SetWszDatabaseName( pch );

    Assert( NULL != wszRelocatedDb || !fAlternateDbDir );
    if ( NULL != wszRelocatedDb )
    {
        Assert( fAlternateDbDir );
        OSStrCbCopyW( pchNext, cbNext, wszRelocatedDb );
        pchNext += cchRelocatedDb;
        cbNext -= cchRelocatedDb * sizeof(WCHAR);
    }

    OSStrCbCopyW( pchNext, cbNext, wszDbName );
    pchNext += cchDbName;
    cbNext -= cchDbName * sizeof(WCHAR);

    return JET_errSuccess;
}

IFMP FMP::s_ifmpMinInUse = g_ifmpMax;
IFMP FMP::s_ifmpMacInUse = 0;
IFMP FMP::s_ifmpMacCommitted = 0;

// if a "free" FMP entry retains a database header signature (a valid DataHeaderSignature()),
// then there might still be some cached clean/untidy pages in BF manager for previously attached DB.
// we usually would like to exhaust "real" free FMP first before reusing them.
// 
// set fUseAny to treat all FMP entries in the same way
// 
INLINE BOOL IsFMPUsable( IFMP ifmp, BOOL fUseAny )
{
    if ( ifmp < cfmpReserved )
    {
        return fFalse;
    }
    const FMP* pfmp = &g_rgfmp[ ifmp ];
    return !pfmp->FInUse() && ( fUseAny || pfmp->DataHeaderSignature().FNull() );
}

/*
 *  NewAndWriteLatch
 *      IFMP *pifmp             returned fmp entry
 *      CHAR *szDatabaseName    used to check if an entry has the same database name and instance
 *      PIB ppib                used to set the latch
 *      INST *pinst             instance that request this latch
 *
 *  This function look for an entry for setting up the database of the given instance.
 *  During recovery, we may allocate one entry and fill the database name. In that case,
 *  the database name is null, and ppib is ppibSurrogate.
 */
ERR FMP::ErrNewAndWriteLatch(
    _Out_ IFMP                      *pifmp,
    _In_z_ const WCHAR              *wszDatabaseName,
    _In_ PIB                        *ppib,
    _In_ INST                       *pinst,
    _In_ IFileSystemAPI * const     pfsapi,
    _In_ DBID                       dbidGiven,
    _In_ const BOOL                 fForcePurgeCache,
    _In_ const BOOL                 fCheckForAttachInfoOverflow,
    _Out_opt_ BOOL * const          pfCacheAlive )
{
    ERR         err                     = JET_errSuccess;
    IFMP        ifmp;
    BOOL        fSkippedAttach          = fFalse;
    BOOL        fDeferredAttach         = fFalse;
    BOOL        fPurgeFMP               = fFalse;

    Assert( NULL != wszDatabaseName );

    if ( pfCacheAlive )
    {
        *pfCacheAlive = fFalse;
    }

    // if we are restoring in a different location that the usage collison check must be
    // done between existing entries in fmp array and the new destination name.
    if ( pinst->FRecovering() )
    {
        Assert( pinst->m_plog );
        LOG *plog = pinst->m_plog;

        // build the destination file name
        // SOFT_HARD: ??? we don't have the FMP yet ... we should use the Hard Recovery new flag in the
        // restore map
        if ( plog->FExternalRestore() || ( plog->IrstmapMac() > 0 && !plog->FHardRestore() ) )
        {
            if ( 0 > plog->IrstmapSearchNewName( wszDatabaseName ) )
            {
                fSkippedAttach = fTrue;
            }
        }
    }

    //  lock the IFMP pool

    EnterFMPPoolAsWriter();

    //  look for matching (DB file name) file map entry

    for ( ifmp = IfmpMinInUse(); ifmp <= IfmpMacInUse(); ifmp++ )
    {
        FMP *pfmp = &g_rgfmp[ ifmp ];

        if ( pfmp->FInUse() )
        {
            if ( 0 == UtilCmpFileName( pfmp->WszDatabaseName(), wszDatabaseName ) )
            {
                if ( pfmp->Pinst() == pinst )
                {
                    pfmp->CritLatch().Enter();

                    if ( pfmp->FAttached()
                        || pfmp->FWriteLatchByOther( ppib )
                        || pfmp->CPin() )
                    {
                        *pifmp = ifmp;
                        err = ErrERRCheck( JET_wrnDatabaseAttached );
                    }
                    else
                    {
                        pfmp->SetWriteLatch( ppib );
                        Assert( !( pfmp->FExclusiveOpen() ) );
                        *pifmp = ifmp;
                        CallS( err );
                    }

                    pfmp->CritLatch().Leave();
                }
                else
                {
                    // check if one of the 2 members just compared is in SkippedAttach mode
                    if ( fSkippedAttach
                        || pfmp->FSkippedAttach() )
                    {
                        continue;
                    }
                    else if ( pinst->FRecovering() )
                    {
                        fDeferredAttach = fTrue;
                        continue;
                    }

                    err = ErrERRCheck( JET_errDatabaseSharingViolation );
                }

                goto HandleError;
            }
        }
    }

    DBID dbid;

    //  Sort of a layer violation (as ATTACHINFO is not related to FMP at all, sort of a part
    //  of the logging aware code - which this is not), probably this should be out a layer?
    //  The other DBID mapping below makes it arguable, but I'd still argue this is more layer
    //  violation than not.
    if ( fCheckForAttachInfoOverflow && dbidGiven != dbidTemp )
    {
        // check if new database will overflow the attached database list
        ULONG cbAttachInfo = sizeof(ATTACHINFO) + sizeof(WCHAR) * (LOSStrLengthW( wszDatabaseName ) + 1);
        for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            if ( pinst->m_mpdbidifmp[ dbid ] >= g_ifmpMax )
                continue;
            FMP *pfmp = &g_rgfmp[ pinst->m_mpdbidifmp[ dbid ] ];
            if ( !pfmp->FLogOn() || pfmp->FSkippedAttach() || pfmp->FDeferredAttach() || !pfmp->FAttached() )
                continue;
            cbAttachInfo += sizeof(ATTACHINFO) + sizeof(WCHAR) * (LOSStrLengthW( pfmp->WszDatabaseName() ) + 1);
            if ( cbAttachInfo > cbAttach )
            {
                Error( ErrERRCheck( JET_errTooManyAttachedDatabases ) );
            }
        }
    }

    //  Allocate entry from pinst->m_mpdbidifmp

    if ( dbidGiven >= dbidMax )
    {
        for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            if ( pinst->m_mpdbidifmp[ dbid ] >= g_ifmpMax )
                break;
        }

        if ( dbid >= dbidMax )
        {
            Error( ErrERRCheck( JET_errTooManyAttachedDatabases ) );
        }

        dbidGiven = dbid;
    }

    ifmp = g_ifmpMax;
    BOOL fFoundMatchingFmp = fFalse;

    Assert( !FMP::FAllocatedFmp( ifmp ) );
    if ( dbidGiven != dbidTemp )
    {
        DBFILEHDR_FIX * pdbfilehdr = NULL;
        
        // do we have any fmp entry waiting to be resurrected?
        for ( ifmp = cfmpReserved; ifmp < g_ifmpMax && FMP::FAllocatedFmp( ifmp ); ifmp++ )
        {
            if ( !g_rgfmp[ifmp].DataHeaderSignature().FNull() )
            {
                DBFILEHDR_FIX* const pdbfilehdrsig = (DBFILEHDR_FIX*)g_rgfmp[ifmp].DataHeaderSignature().Pv();
                Assert( g_rgfmp[ifmp].DataHeaderSignature().Cb() >= sizeof( DBFILEHDR ) );
                AssertDatabaseHeaderConsistent( pdbfilehdrsig, g_rgfmp[ifmp].DataHeaderSignature().Cb(), g_cbPage );
                //  Checking not all zeros _except_ the checksum.
                C_ASSERT( offsetof( DBFILEHDR, le_ulMagic ) == sizeof( pdbfilehdr->le_ulChecksum ) );
                OnDebug( const BYTE* const pbhdrCheck = (BYTE*)pdbfilehdrsig );
                Assert( !FUtilZeroed( pbhdrCheck + offsetof( DBFILEHDR, le_ulMagic ), offsetof( DBFILEHDR, le_filetype ) - sizeof( pdbfilehdr->le_ulChecksum ) ) );

                // if we haven't allocated and loaded the signature yet, try that now ...
                if ( NULL == pdbfilehdr )
                {
                    pdbfilehdr = (DBFILEHDR_FIX*)PvOSMemoryPageAlloc( g_cbPage, NULL );
                    if ( pdbfilehdr )
                    {
                        memset( pdbfilehdr, 0, g_cbPage );
                        // read in db header
                        // note: read will fail from the newly created database path (when createdatabase() calls us)
                        const ERR errRH = ErrUtilReadShadowedHeader( pinst, pfsapi, wszDatabaseName, (BYTE*)pdbfilehdr, g_cbPage, -1, UtilReadHeaderFlags( urhfReadOnly | urhfNoEventLogging ) );
                        if ( errRH >= JET_errSuccess )
                        {
                            AssertDatabaseHeaderConsistent( pdbfilehdr, g_cbPage, g_cbPage );
                        }
                    }
                }
                if ( pdbfilehdr )
                {
                    if ( JET_filetypeDatabase == pdbfilehdr->le_filetype &&
                            0 == memcmp( pdbfilehdrsig, pdbfilehdr, offsetof( DBFILEHDR, rgbReserved ) ) )
                            // If we wanted to be really defensive we could add this clause, but an update that is non-logged (so 
                            // like lgposAttach doesn't update) and adds a new field seems very unlikely right?  Also now that
                            // we've added the update minor now, there is no excuse to not change that for the littlest non-logged 
                            // update, and so that will protect us against this potential as well.  But if we ever get scared:
                            //fTrue == FUtilZeroed( ((BYTE*)pdbfilehdr) + offsetof( DBFILEHDR, rgbReserved ), 0, g_cbPage - offsetof( DBFILEHDR, rgbReserved ) ) )
                    {
                        // fmp entry recognized
                        Assert( !g_rgfmp[ifmp].FInUse() );
                        OSMemoryPageFree( pdbfilehdr );
                        pdbfilehdr = NULL;
                        fFoundMatchingFmp = fTrue;
                        goto SelectedIfmp;
                    }
                }
            }
        } // end for each ifmp

        if ( pdbfilehdr )
        {
            OSMemoryPageFree( pdbfilehdr );
            pdbfilehdr = NULL;
        }
    }

    //  We did not find a match when searching for it, so now find a usable FMP (preferring
    //  ones with no unattached cache, and lower ifmps over higher ones)...
    ULONG cFmpsEvaluated = 0;
    ULONG cFmpsInUse = 0;
    ULONG cFmpsRedoMapNotEmpty = 0;
    ULONG cPreservedCaches = 0;
    for ( int iPreserveUnattachedCaches = 1; iPreserveUnattachedCaches >= 0; iPreserveUnattachedCaches-- )
    {
        const BOOL fPreserveUnattachedCaches = ( iPreserveUnattachedCaches != 0 );
        cFmpsEvaluated = 0;
        cFmpsInUse = 0;
        cFmpsRedoMapNotEmpty = 0;
        cPreservedCaches = 0;

        //  First we try to search something in the Min-Mac range to keep the IFMP range tight
        for ( ifmp = IfmpMinInUse(); ifmp <= IfmpMacInUse(); ifmp++ )
        {
            cFmpsEvaluated++;

            if ( g_rgfmp[ifmp].FInUse() )
            {
                cFmpsInUse++;
                continue;
            }

            if ( !g_rgfmp[ifmp].FRedoMapsEmpty() )
            {
                cFmpsRedoMapNotEmpty++;
                continue;
            }

            if ( fPreserveUnattachedCaches && !g_rgfmp[ifmp].DataHeaderSignature().FNull() )
            {
                cPreservedCaches++;
                continue;
            }

            goto SelectedIfmp;
        }

        //  Next we try to utilize FMPs on the lower side of Min/Mac range to keep ourselves from wasting g_rgfmp memory
        for ( ifmp = IfmpMinInUse() - 1; IfmpMacInUse() != 0 && ifmp >= cfmpReserved && ifmp < IfmpMinInUse() /* note using/catch UINT wrap-around */; ifmp-- )
        {
            Assert( NULL == g_rgfmp[ifmp].WszDatabaseName() ); // == Assert( !g_rgfmp[ifmp].FInUse() ); // that would be weird, as it's outside the min/mac range

            cFmpsEvaluated++;

            if ( !g_rgfmp[ifmp].FRedoMapsEmpty() )
            {
                cFmpsRedoMapNotEmpty++;
                continue;
            }

            if ( fPreserveUnattachedCaches && !g_rgfmp[ifmp].DataHeaderSignature().FNull() )
            {
                cPreservedCaches++;
                continue;
            }

            goto SelectedIfmp;
        }

        //  We might have previously grown, but then shrunk, so see if that is available.
        for ( ifmp = IfmpMacInUse() + 1; ifmp < g_ifmpMax && FMP::FAllocatedFmp( ifmp ); ifmp++ )
        {
            Assert( NULL == g_rgfmp[ifmp].WszDatabaseName() ); // == Assert( !g_rgfmp[ifmp].FInUse() ); // that would be weird, as it's outside the min/mac range

            cFmpsEvaluated++;

            if ( !g_rgfmp[ifmp].FRedoMapsEmpty() )
            {
                cFmpsRedoMapNotEmpty++;
                continue;
            }

            if ( fPreserveUnattachedCaches && !g_rgfmp[ifmp].DataHeaderSignature().FNull() )
            {
                cPreservedCaches++;
                continue;
            }

            goto SelectedIfmp;
        }

        Assert( cFmpsEvaluated == ( s_ifmpMacCommitted - cfmpReserved ) );
        Assert( cFmpsInUse <= cFmpsEvaluated );
        Assert( cFmpsRedoMapNotEmpty <= cFmpsEvaluated );
        Assert( cPreservedCaches <= cFmpsEvaluated );
        Assert( cPreservedCaches == 0 || fPreserveUnattachedCaches );
        Assert( ( cFmpsInUse + cFmpsRedoMapNotEmpty + cPreservedCaches ) == cFmpsEvaluated );

        if ( fPreserveUnattachedCaches && cPreservedCaches == 0 )
        {
            // We are trying to preserve caches, but we ended up not selecting any IFMP
            // for reasons other than preserving the IFMPs' caches, so it's pointless
            // to retry.
            break;
        }

#ifdef DEBUG
        const ULONG cMinimumPreservedCaches = ( ( rand() % 2 ) == 0 ) ? 2 : g_ifmpMax;
#else
        const ULONG cMinimumPreservedCaches = FJetConfigLowMemory() ? 2 :
                                                FJetConfigMedMemory() ? 5 :
                                                    g_ifmpMax;
#endif

        if ( fPreserveUnattachedCaches &&
                cPreservedCaches <= cMinimumPreservedCaches &&  // we don't have enough preserved caches
                cFmpsEvaluated < ( g_ifmpMax - 1 ) )            // we have room to grow the FMP pool
        {
            //  We found only preserved caches (or we would've bailed to SelectedIfmp by now), so we could
            //  go back through with (fPreserveUnattachedCaches == fFalse this time) and select one of them 
            //  and canabalize / evict it but we want to ensure we have a reasonable enough chance to save 
            //  the cache for another client coming back online, so we'll break out and grow the g_rgfmp
            //  instead ...
            break;
        }
    }

    //  Finally fall back to potentially actually growing the FMP array to new memory ...

    //  We fell off the end of the last loop, have we just hit the maximum DBs attached?
    if ( ifmp >= g_ifmpMax )
    {
        Assert( ifmp == g_ifmpMax );
        Assert( ifmp > cfmpReserved );
        Assert( ( ifmp - cfmpReserved ) == cFmpsEvaluated );
        Assert( cPreservedCaches == 0 );
        Assert( ( cFmpsInUse + cFmpsRedoMapNotEmpty ) == cFmpsEvaluated );
        Error( ErrERRCheck( JET_errTooManyAttachedDatabases ) );
    }

    //  We are here because we couldn't find an allocated IFMP or an allocated and no unattached-cache IFMP ...
    Assert( ifmp > IfmpMacInUse() );

    if ( !FAllocatedFmp( ifmp ) )
    {
        Call( ErrNewOneFmp( ifmp ) );
        Assert( IsFMPUsable( ifmp, fTrue ) );
    }

    //  Now fall through (and probably allocate a new FMP)

SelectedIfmp:

    Assert( ifmp < g_ifmpMax );
    Assert( FMP::FAllocatedFmp( ifmp ) );

    //  Make sure we have enough memory to accumulate perf counters for this IFMP.

    if ( !g_fDisablePerfmon && !PLS::FEnsurePerfCounterBuffer( ( ifmp + 1 ) * g_cbPlsMemRequiredPerPerfInstance ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    s_ifmpMinInUse = min( s_ifmpMinInUse, ifmp );
    s_ifmpMacInUse = max( s_ifmpMacInUse, ifmp );

    //  Partially AssertVALIDIFMP
    Assert( IfmpMinInUse() <= ifmp );
    Assert( IfmpMacInUse() >= ifmp );

    Call( ErrInitializeOneFmp(
        pinst,
        ppib,
        ifmp,
        dbidGiven,
        wszDatabaseName,
        pfsapi,
        fForcePurgeCache,
        fDeferredAttach ) );

    Assert( IfmpMinInUse() <= ifmp );
    Assert( IfmpMacInUse() >= ifmp );

    if ( fFoundMatchingFmp && !fForcePurgeCache && !BoolParam( JET_paramEnableViewCache ) )
    {
        // reactivate BFContext (if possible)
        Assert( !g_rgfmp[ifmp].DataHeaderSignature().FNull() );

        g_rgfmp[ifmp].FreeHeaderSignature();
        Assert( g_rgfmp[ifmp].DataHeaderSignature().FNull() );
        BFSetBFFMPContextAttached( ifmp );

        if ( pfCacheAlive )
        {
            *pfCacheAlive = fTrue;
        }
    }
    else if ( !g_rgfmp[ifmp].DataHeaderSignature().FNull() )
    {
        //  We selected a FMP with an unattached cache (purge the other DB).
        fPurgeFMP = fTrue;
        g_rgfmp[ifmp].FreeHeaderSignature();
        Assert( g_rgfmp[ifmp].DataHeaderSignature().FNull() );
    }

    if ( !fFoundMatchingFmp )
    {
        // We can't reuse an FMP with pending redo map entries because we would lose
        // track of unresolved entries, which could cause us to succeed in recovering
        // a database which could be potentially corrupted. However, this is already being
        // prevented above, as we always skip picking an FMP with a non-empty redo map.
        DBEnforceSz(
            ifmp,
            g_rgfmp[ifmp].FRedoMapsEmpty(),
            "ClobberNonEmptyRedoMap" );

        g_rgfmp[ifmp].FreeLogRedoMaps();
    }

    g_rgfmp[ifmp].TraceStationId( tsidrOpenInit );

    *pifmp = ifmp;
    err = JET_errSuccess;

HandleError:
    PERFSetDatabaseNames( pfsapi );
    LeaveFMPPoolAsWriter();

    // We may have reused a mismatched FMP and need to purge the old pages
    // The purge can't be done inside of critFMP, otherswise a deadlock
    // can occur
    if ( fPurgeFMP )
    {
        BFPurge( ifmp );
    }

    return err;
}

// Initializes a single FMP.
ERR FMP::ErrInitializeOneFmp(
    _In_ INST* const pinst,
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const DBID dbidGiven,
    _In_z_ const WCHAR* wszDatabaseName,
    _In_ IFileSystemAPI * const pfsapi,
    _In_ const BOOL fForcePurgeCache,
    _In_ const BOOL fDeferredAttach )
{
    ERR err = JET_errSuccess;
    FMP *const pfmp = &g_rgfmp[ ifmp ];

    Assert( FWriterFMPPool() );

    Assert( FAllocatedFmp( ifmp ) );

    //  if we can find one entry here, latch it.


    Assert( pfmp->m_pdbmScan == NULL );
    Assert( pfmp->PdbmFollower() == NULL );
    Assert( pfmp->PkvpsMSysLocales() == NULL );
    Assert( pfmp->PFlushMap() == NULL );

    Assert( !pfmp->FInUse() );
    Call( pfmp->ErrStoreDbName( pinst, pfsapi, wszDatabaseName, fFalse ) );

    pfmp->SetPinst( pinst );
    pfmp->SetDbid( dbidGiven );
    pinst->m_mpdbidifmp[ dbidGiven ] = ifmp;

    pfmp->CritLatch().Enter();

    pfmp->ResetFlags();

    if ( fDeferredAttach )
    {
        Assert( pinst->FRecovering() );
        pfmp->SetDeferredAttach( fFalse, fFalse );
        pfmp->SetLogOn();
    }

    //  set trxOldestTarget to trxMax to make these
    //  variables unusable until they get reset
    //  by Create/AttachDatabase
    pfmp->SetDbtimeOldestGuaranteed( 0 );
    pfmp->SetDbtimeOldestCandidate( 0 );
    pfmp->SetDbtimeOldestTarget( 0 );
    pfmp->SetTrxOldestCandidate( trxMax );
    pfmp->SetTrxOldestTarget( trxMax );
    pfmp->SetTrxNewestWhenDiscardsLastReported( trxMin );

    pfmp->SetWriteLatch( ppib );
    pfmp->SetLgposAttach( lgposMin );
    pfmp->SetLgposDetach( lgposMin );
    Assert( FWriterFMPPool() );
    pfmp->SetILgposWaypoint( lgposMin ); // effectively ResetWaypoint(), but we already have rwlFMPPool and we're in FMP.
    pfmp->SetLGenMaxCommittedAttachedDuringRecovery( 0 );
    pfmp->SetPfapi( NULL );
    pfmp->m_cbOwnedFileSize = 0; // not using the setter to avoid asserts.
    pfmp->AcquireIOSizeChangeLatch();
    pfmp->SetExtentPageCountCacheTableInfo( pgnoNull, objidNil );
    pfmp->SetPgnoSnapBackupMost( pgnoNull );
    pfmp->SetFsFileSizeAsyncTarget( 0 );
    pfmp->ReleaseIOSizeChangeLatch();
    pfmp->ResetScheduledPeriodicTrim();
    pfmp->ResetTrimSupported();
    pfmp->ResetContainsDataFromFutureLogs();
    pfmp->ResetOlderDemandExtendDb();
    OnDebug( pfmp->m_dbhus = dbhusNoUpdate );
    pfmp->ResetEventThrottles();
    pfmp->ResetPctCachePriorityFmp();
    pfmp->ResetPgnoShrinkTarget();
    pfmp->ResetShrinkIsRunning();
    pfmp->SetShrinkDatabaseOptions( NO_GRBIT );
    pfmp->SetShrinkDatabaseTimeQuota( -1 );
    pfmp->SetShrinkDatabaseSizeLimit( 0 );
    pfmp->SetLeakReclaimerEnabled( fFalse );
    pfmp->SetLeakReclaimerTimeQuota( -1 );
    pfmp->SetSelfAllocSpBufReservationEnabled( fFalse );
    pfmp->ResetLeakReclaimerIsRunning();
    pfmp->ResetPgnoMaxTracking();
    pfmp->ResetCpgAvail();

    pfmp->SetDbtimeBeginRBS( 0 );
    pfmp->ResetRBSOn();
    pfmp->ResetNeedUpdateDbtimeBeginRBS();

    Assert( pfmp->PdbmFollower() == NULL );
    Assert( pfmp->m_critOpenDbCheck.FNotOwner() );

    Assert( !pfmp->FExclusiveOpen() );

    pfmp->CritLatch().Leave();

HandleError:
    return err;
}

VOID FMP::ReleaseWriteLatchAndFree( PIB *ppib )
{
    Assert( FWriterFMPPool() );
    CritLatch().Enter();

    TraceStationId( tsidrCloseTerm );

    /*  Identity must exists. It was just newed, so
     *  if it is exclusively opened, it must be opened by me
     */
    Assert( !FExclusiveOpen() || PpibExclusiveOpen() == ppib );
    IFMP ifmp = PinstFromPpib( ppib )->m_mpdbidifmp[ Dbid() ];
    FMP::AssertVALIDIFMP( ifmp );

    OSMemoryHeapFree( WszDatabaseName() );
    SetWszDatabaseName( NULL );

    /*  Typically these should be disposed / free'd before now, but this catches
     *  the cases of many error paths which have to release and free the FMP.
     */
    m_isdlCreate.TermSequence();
    m_isdlAttach.TermSequence();
    m_isdlDetach.TermSequence();

    Assert( m_msRangeLock.FEmpty() );

    SetPinst( NULL );
    PinstFromPpib( ppib )->m_mpdbidifmp[ Dbid() ] = g_ifmpMax;
    SetDbid( dbidMax );
    Assert( !FExclusiveOpen() );
    ResetFlags();

    if ( IfmpMinInUse() == ifmp )
    {
        Assert( FMP::FAllocatedFmp( ifmp ) );

        IFMP ifmpT = IfmpMinInUse() + 1;
        for ( ; ifmpT <= IfmpMacInUse(); ifmpT++ )
        {
            if ( !IsFMPUsable( ifmpT, fFalse ) )
            {
                break;
            }
        }

        if ( ifmpT > IfmpMacInUse() )   //  no more ocupied FMPs
        {
            Assert( IfmpMacInUse() + 1 == ifmpT );
            s_ifmpMacInUse = 0;
            s_ifmpMinInUse = g_ifmpMax;
        }
        else
        {
            s_ifmpMinInUse = ifmpT;
        }
    }
    else if ( IfmpMacInUse() == ifmp )
    {
        Assert( FMP::FAllocatedFmp( ifmp ) );

        IFMP ifmpT = ifmp - 1;
        for ( ; ifmpT > IfmpMinInUse(); ifmpT-- )
        {
            if ( !IsFMPUsable( ifmpT, fFalse ) )
            {
                break;
            }
        }
        Assert( FMP::FAllocatedFmp( ifmpT ) );
        Assert( ifmpT < s_ifmpMacInUse ); // should not make it grow to release
        s_ifmpMacInUse = ifmpT;
    }

    /*  Free write latch.
     */
    ResetWriteLatch( ppib );
    if ( GateWriteLatch().CWait() > 0 )
    {
        //  When we release a write latch here, we can not just release 1
        //  waiter, because we may cause the next waiting thread to bail.
        //  So for this reason, we release all
        //  the waiters at once, so they can all figure out that the FMP
        //  is gone.  This is admittedly a delicate approach, as if we do
        //  something else that can cause this bail without signalling/releasing
        //  we will be in this predicament again.
        GateWriteLatch().ReleaseAll( CritLatch() );
    }
    else
    {
        CritLatch().Leave();
    }

    PERFSetDatabaseNames( ppib->m_pinst->m_pfsapi );
}


//  Returns whether the IFMP is allocated already, or we need to call ErrNewOneFmp() to 
//  use this IFMP.
//
//  Note: Even though ifmp == 0 is _not_ actually allocated, we still return true.  All
//  critical loops start (or should;) from cfmpReserved.
BOOL FMP::FAllocatedFmp( IFMP ifmp )
{
    Assert( ifmp <= g_ifmpMax );
    return ifmp < s_ifmpMacCommitted;
}

BOOL FMP::FAllocatedFmp( const FMP * const pfmp )
{
    if ( ( pfmp == NULL ) || ( g_rgfmp == NULL ) )
    {
        return fFalse;
    }
    
    //  warning: Do NOT use members of the pfmp->.  This may be used on not-yet-committed 
    //  FMP, and so pfmp is not deref-able.
    const IFMP ifmp = IFMP( pfmp - g_rgfmp );
    Assert( &( g_rgfmp[ ifmp ] ) == pfmp );
    return FMP::FAllocatedFmp( ifmp );
}

//  Allocates (via committing the pre-reserved memory @ at ErrFMPInit) and initializes 
//  that single FMP.
ERR FMP::ErrNewOneFmp( const IFMP ifmp )
{
    ERR err = JET_errSuccess;

    // Validate argument and runtime state / lock
    Assert( FWriterFMPPool() );
    Assert( g_rgfmp != NULL ); // g_rgfmp should be reserved / i.e. after ErrFMPInit()
    Assert( ifmp >= cfmpReserved /* currently == 1 */ );  // so we drop / don't accept ifmp = 0
    Assert( ifmp < g_ifmpMax ); // should not try to overgrow what we've reserved.

    // The below code always assumes we allocate one at a time, no skipping ifmp values.
    Assert( s_ifmpMacCommitted == ifmp );
    Assert( !FAllocatedFmp( ifmp ) );
    Assert( FMP::FAllocatedFmp( ifmp - 1 ) );   // since we allocate one at a time, previous one should be allocated.

    // Preallocate this to avoid having complicated rollback of the committed state.
    //

    CIoStats * piostatsDbRead = NULL;
    CIoStats * piostatsDbWrite = NULL;
    const SIZE_T crangeMax  = 1;
    const SIZE_T cbrangelock    = sizeof( RANGELOCK ) + crangeMax * sizeof( RANGE );
    RANGELOCK * prangelockPrealloc0 = NULL;
    RANGELOCK * prangelockPrealloc1 = NULL;

    Alloc( prangelockPrealloc0 = (RANGELOCK*)PvOSMemoryHeapAlloc( cbrangelock ) );
    Alloc( prangelockPrealloc1 = (RANGELOCK*)PvOSMemoryHeapAlloc( cbrangelock ) );
#ifdef ENABLE_MICROSOFT_MANAGED_DATACENTER_LEVEL_OPTICS
    const BOOL fDatacenterGranularStats = fTrue;
#else
    const BOOL fDatacenterGranularStats = fFalse;
#endif
    Call( CIoStats::ErrCreateStats( &piostatsDbRead, fDatacenterGranularStats ) );
    Call( CIoStats::ErrCreateStats( &piostatsDbWrite, fDatacenterGranularStats ) );

    // If the end of the previous ifmp and the end of this ifmp are in different pages ... then we 
    // need to allocate/commit a new page of memory to hold this FMP.
    //

    BYTE * pbBase = (BYTE*)( g_rgfmp + ( ( ifmp == 1 ) ? 0 : ifmp ) );
    BYTE * pbHigh = (BYTE*)( g_rgfmp + ifmp ) + sizeof(FMP);
    BYTE * pbHighPrev = pbBase - 1;

    const BOOL fInNewPage = roundup( (DWORD_PTR)pbHighPrev, OSMemoryPageCommitGranularity() )
                                != roundup( (DWORD_PTR)pbHigh, OSMemoryPageCommitGranularity() );

    Assert( OSMemoryPageCommitGranularity() >= 4096 ); // or next C_ASSERT is wrong.
    C_ASSERT( sizeof(FMP) < 4096 );
    C_ASSERT( sizeof(FMP) * cfmpReserved < 4096 );

    if ( fInNewPage )
    {
        size_t cbAlloc = roundup( pbHigh - pbBase, OSMemoryPageCommitGranularity() );
        Assert( cbAlloc );

        const BOOL fCommitted = FOSMemoryPageCommit( (void*)roundup( (DWORD_PTR)pbBase, OSMemoryPageCommitGranularity() ), cbAlloc );
        if ( !fCommitted )
        {
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }

        Assert( (DWORD_PTR)pbHigh < ( roundup( (DWORD_PTR)pbBase, OSMemoryPageCommitGranularity() ) + cbAlloc ) );
        OnDebug( AtomicExchangeAdd( (LONG*)( pbHigh - sizeof(LONG) ), 0 ) ); // double check we can force a deref
    }

    // Mark this IMFP/FMP allocated (and check asserts)
    //

    s_ifmpMacCommitted = ifmp + 1;

    Assert( FAllocatedFmp( ifmp ) );
    Assert( FAllocatedFmp( &( g_rgfmp[ ifmp ] ) ) );

    // Use placement-new to run the FMP constructor.
    //

    FMP* pfmpT = new(&g_rgfmp[ ifmp ]) FMP;

    // Ensure that placement-new didnt change the pointer
    // value (it does for array-placement-new).
    Assert( pfmpT == &g_rgfmp[ ifmp ] );

    // should be NULL after .ctor, places the pre-allocs into later
    Assert( g_rgfmp[ ifmp ].m_rgprangelock[ 0 ] == NULL );
    Assert( g_rgfmp[ ifmp ].m_rgprangelock[ 1 ] == NULL );

    //  Configure the rest of it
    //

    Assert( pfmpT->m_dbid == dbidMax );

#ifdef DEBUG
    g_rgfmp[ ifmp ].SetDatabaseSizeMax( 0xFFFFFFFF );
#endif  //  DEBUG

    //  initialize sentry range locks

    Assert( prangelockPrealloc0 != NULL );
    Assert( prangelockPrealloc1 != NULL );
    g_rgfmp[ ifmp ].m_rgprangelock[ 0 ] = prangelockPrealloc0;
    g_rgfmp[ ifmp ].m_rgprangelock[ 1 ] = prangelockPrealloc1;
    prangelockPrealloc0 = NULL;
    prangelockPrealloc1 = NULL;

    g_rgfmp[ ifmp ].m_rgprangelock[ 0 ]->crange       = 0;
    g_rgfmp[ ifmp ].m_rgprangelock[ 0 ]->crangeMax    = crangeMax;

    g_rgfmp[ ifmp ].m_rgprangelock[ 1 ]->crange       = 0;
    g_rgfmp[ ifmp ].m_rgprangelock[ 1 ]->crangeMax    = crangeMax;

    Assert( g_rgfmp[ ifmp ].m_msRangeLock.FEmpty() );

    g_rgfmp[ ifmp ].SetPiostats( piostatsDbRead, piostatsDbWrite );
    Assert( g_rgfmp[ ifmp ].Piostats( iotypeRead ) == piostatsDbRead );
    Assert( g_rgfmp[ ifmp ].Piostats( iotypeWrite ) == piostatsDbWrite );

    //  check alloc successful ...

    Assert( s_ifmpMacCommitted > ifmp );
    Assert( FAllocatedFmp( ifmp ) );
    Assert( FAllocatedFmp( &( g_rgfmp[ ifmp ] ) ) );

    return JET_errSuccess;

HandleError:
    Assert( err < JET_errSuccess );

    //  We failed _something_ insist we don't call this IFMP / slot allocated.
    Assert( s_ifmpMacCommitted == ifmp );
    Assert( !FAllocatedFmp( ifmp ) );

    OSMemoryHeapFree( prangelockPrealloc0 );
    OSMemoryHeapFree( prangelockPrealloc1 );
    delete piostatsDbRead;
    delete piostatsDbWrite;

    return err;
}

//  These three functions manage the reserved size, and the tricked out (off by 1/cfmpReserved) offset of the g_rgfmp.
LOCAL size_t CbFMPRgfmpShift()
{
    const size_t cb = roundup( cfmpReserved * sizeof( g_rgfmp[ 0 ] ), OSMemoryPageCommitGranularity() );
    Assert( ( cb % OSMemoryPageCommitGranularity() ) == 0 );
    return cb;
}

LOCAL ERR ErrFMPReserveShiftedRgfmp()
{
    ERR err = JET_errSuccess;

    const size_t cbFmpArrayPages = roundup( g_ifmpMax * sizeof( g_rgfmp[ 0 ] ), OSMemoryPageCommitGranularity() );
    //  allocating an "extra page", so we can shift off (and save the memory of) the reserved / invalid FMP = 0  ... 
    const size_t cbShiftedPages = CbFMPRgfmpShift();
    Assert( cfmpReserved * sizeof(FMP) < OSMemoryPageCommitGranularity() );

    BYTE * pbFmpBuffer = (BYTE*)PvOSMemoryPageReserve( cbFmpArrayPages + cbShiftedPages, NULL );
    AllocR( pbFmpBuffer );

    g_rgfmp = (FMP*)( &( pbFmpBuffer[ cbShiftedPages - ( cfmpReserved * sizeof(FMP) ) ] ) );
    Assert( (BYTE*)g_rgfmp >= pbFmpBuffer );

    //  Same calculation used below in ErrFMPUnreserveShiftedRgfmp / free ...
    Assert( ( ((BYTE*)g_rgfmp) + ( cfmpReserved * sizeof( g_rgfmp[ 0 ] ) ) - CbFMPRgfmpShift() ) == pbFmpBuffer );
    //  Required to make sure OSMemoryPageDecommit() call will be kosher ...    
    Assert( ( DWORD_PTR( &g_rgfmp[cfmpReserved] ) % OSMemoryPageCommitGranularity() ) == 0 );

    return JET_errSuccess;
}

LOCAL void FMPDecommitShiftedRgfmp( const IFMP cfmpMacCommitted )
{
    size_t cbFmpCommittedMax = roundup( ( (size_t)cfmpMacCommitted - cfmpReserved ) * sizeof(FMP), OSMemoryPageCommitGranularity() );
    Assert( ( DWORD_PTR( &g_rgfmp[cfmpReserved] ) % OSMemoryPageCommitGranularity() ) == 0 );
    OSMemoryPageDecommit( &g_rgfmp[cfmpReserved], cbFmpCommittedMax );
}
LOCAL void FMPUnreserveFreeShiftedRgfmp()
{
    BYTE * pbFree = ((BYTE*)g_rgfmp) + ( cfmpReserved * sizeof( g_rgfmp[ 0 ] ) ) - CbFMPRgfmpShift();
    OSMemoryPageFree( pbFree );
    g_rgfmp = NULL;
}

ERR FMP::ErrFMPInit( )
{
    ERR err;

    /* initialize the file map array */

    EnterFMPPoolAsWriter(); //  probably not actually necessary (but want to ensure non concurrency with FNewOneFmp, AND FMP::Term()).

    Assert( NULL == g_rgfmp );
    Assert( g_ifmpMax > 0 );

    Call( ErrFMPReserveShiftedRgfmp() );

    s_ifmpMinInUse = g_ifmpMax;
    s_ifmpMacInUse = 0;

    //  Since we don't use ifmp == 0, we pretend ifmp == 0 is committed (even though it is NOT)
    s_ifmpMacCommitted = cfmpReserved;

    LeaveFMPPoolAsWriter();

    return JET_errSuccess;

HandleError:
    Assert( g_rgfmp == NULL ); // no failure paths leave g_rgfmp allocated today.
    if ( g_rgfmp )
    {
        FMPUnreserveFreeShiftedRgfmp(); // for the future.
    }
    Assert( g_rgfmp == NULL );

    LeaveFMPPoolAsWriter();

    return err;
}


VOID FMP::Term( )
{

    EnterFMPPoolAsWriter(); //  probably not actually necessary (but want to ensure non concurrency with FNewOneFmp, AND FMP::Term()).

    Assert( g_rgfmp != NULL );

    for ( IFMP ifmp = cfmpReserved; ifmp < g_ifmpMax && FMP::FAllocatedFmp( ifmp ); ifmp++ )
    {
        FMP *pfmp = &g_rgfmp[ifmp];

        DBFILEHDR * const pdbfilehdr = pfmp->m_dbfilehdrLock.SetPdbfilehdr(NULL);

        OSMemoryHeapFree( pfmp->WszDatabaseName() );
        OSMemoryPageFree( pdbfilehdr );
        OSMemoryHeapFree( pfmp->Patchchk() );
        OSMemoryHeapFree( pfmp->WszPatchPath() );
        OSMemoryHeapFree( pfmp->m_rgprangelock[ 0 ] );
        OSMemoryHeapFree( pfmp->m_rgprangelock[ 1 ] );
        pfmp->FreeIostats();
        pfmp->FreeHeaderSignature();
        pfmp->FreeLogRedoMaps();

        // Must manually call the destructor, because we used placement-new.
        pfmp->~FMP();
    }


    /*  free FMP
    /**/

    if ( s_ifmpMacCommitted > cfmpReserved )
    {
        // OSMemoryPageDecommit is not actually needed, because the OSMemoryPageFree() from
        // inside FMPUnreserveFreeShiftedRgfmp will clean it up anyway.
        FMPDecommitShiftedRgfmp( s_ifmpMacCommitted );
        s_ifmpMinInUse = g_ifmpMax;
        s_ifmpMacInUse = 0;
        s_ifmpMacCommitted = 0;
    }

    FMPUnreserveFreeShiftedRgfmp(); // for the future.
    Assert( g_rgfmp == NULL );

    LeaveFMPPoolAsWriter();
}

VOID FMP::AssertRangeLockEmpty() const
{
    Assert( m_msRangeLock.FEmpty() );
}

ERR FMP::ErrIAddRangeLock( const PGNO pgnoStart, const PGNO pgnoEnd )
{
    ERR err = JET_errSuccess;
    RANGE range( pgnoStart, pgnoEnd );

    Assert( m_semRangeLock.CAvail() == 0 );

    //  get the pointers to the pointers to the current and new range locks so
    //  that we can manipulate them easily

    RANGELOCK** pprangelockCur  = &m_rgprangelock[ m_msRangeLock.GroupActive() ];
    RANGELOCK** pprangelockNew  = &m_rgprangelock[ m_msRangeLock.GroupInactive() ];
    const SIZE_T crangeNew      = (*pprangelockCur)->crange + 1;

    //  the new range lock doesn't have enough room to store all the ranges we need

    if ( (*pprangelockNew)->crangeMax < crangeNew )
    {
        //  double the size of the new range lock

        const LONG          crangeMaxT      = LNextPowerOf2( (LONG)crangeNew );
        const SIZE_T        crangeMax       = ( ( crangeMaxT >= 0 ) && ( (SIZE_T)crangeMaxT >= crangeNew ) ) ? (SIZE_T)crangeMaxT : crangeNew;
        const SIZE_T        cbrangelock     = sizeof( RANGELOCK ) + crangeMax * sizeof( RANGE );

        RANGELOCK * const   prangelock      = (RANGELOCK*)PvOSMemoryHeapAlloc( cbrangelock );
        Alloc( prangelock )

        prangelock->crangeMax = crangeMax;

        OSMemoryHeapFree( *pprangelockNew );

        *pprangelockNew = prangelock;
    }

    Assert( (*pprangelockNew)->crangeMax >= crangeNew );

    //  copy the state of the current range lock to the new range lock

    SIZE_T irange;
    for ( irange = 0; irange < (*pprangelockCur)->crange; irange++ )
    {
        (*pprangelockNew)->rgrange[ irange ] = (*pprangelockCur)->rgrange[ irange ];
    }

    //  append the new range to the new range lock

    (*pprangelockNew)->rgrange[ irange ] = range;

    //  set the number of ranges in the new range lock

    (*pprangelockNew)->crange = crangeNew;

    //  cause new writers to see the new range lock and wait until all writers
    //  that saw the old range lock are done writing

    m_msRangeLock.Partition();

HandleError:
    Assert( m_semRangeLock.CAvail() == 0 );
    return err;
}

VOID FMP::IRemoveRangeLock( const PGNO pgnoStart, const PGNO pgnoEnd )
{
    RANGE range( pgnoStart, pgnoEnd );

    Assert( m_semRangeLock.CAvail() == 0 );

    //  get the pointers to the pointers to the current and new range locks so
    //  that we can manipulate them easily

    RANGELOCK** pprangelockCur = &m_rgprangelock[ m_msRangeLock.GroupActive() ];
    RANGELOCK** pprangelockNew = &m_rgprangelock[ m_msRangeLock.GroupInactive() ];

    //  we should always have enough room to store all the ranges we need because
    //  we only add or remove one range at a time

    Assert( (*pprangelockNew)->crangeMax >= (*pprangelockCur)->crange - 1 );

    //  copy all ranges but the specified range to the new range lock
    //  it is possible to have more than one copy of a range so we only
    //  want to remove the first one we see

    bool fRemovedRange = false;
    SIZE_T irangeSrc;
    SIZE_T irangeDest;
    for ( irangeSrc = 0, irangeDest = 0; irangeSrc < (*pprangelockCur)->crange; irangeSrc++ )
    {
        if (    fRemovedRange ||
                !(*pprangelockCur)->rgrange[ irangeSrc ].FMatches( range ) )
        {
            (*pprangelockNew)->rgrange[ irangeDest ] = (*pprangelockCur)->rgrange[ irangeSrc ];
            irangeDest++;
        }
        else
        {
            fRemovedRange = true;
        }
    }

    //  we had better have found the range specified!!!!!

    Assert( fRemovedRange );
    Assert( irangeDest == irangeSrc - 1 );

    //  set the number of ranges in the new range lock

    Assert( (*pprangelockNew)->crangeMax >= irangeDest );
    (*pprangelockNew)->crange = irangeDest;

    //  cause new writers to see the new range lock and wait until all writers
    //  that saw the old range lock are done writing

    m_msRangeLock.Partition();

    Assert( m_semRangeLock.CAvail() == 0 );
}

//  waits until the given PGNO range can no longer be written to by the buffer
//  manager.  this is used to provide a coherent view of the underlying file
//  for this PGNO range for backup purposes

ERR FMP::ErrRangeLock( const PGNO pgnoStart, const PGNO pgnoEnd )
{
    ERR err = JET_errSuccess;

    //  prevent others from modifying the range lock while we are modifying the
    //  range lock

    m_semRangeLock.Acquire();

    Call( ErrIAddRangeLock( pgnoStart, pgnoEnd ) );

    //  allow others to modify the range lock now that it is set

HandleError:
    m_semRangeLock.Release();
    return err;
}

//  permits writes to resume to the given PGNO range locked using FMP::ErrRangeLock

VOID FMP::RangeUnlock( const PGNO pgnoStart, const PGNO pgnoEnd )
{
    //  prevent others from modifying the range lock while we are modifying the
    //  range lock

    m_semRangeLock.Acquire();

    IRemoveRangeLock( pgnoStart, pgnoEnd );

    //  allow others to modify the range lock now that it is set

    m_semRangeLock.Release();
}

//  enters the range lock if the current page does not participate in any range locks,
//  returns the active range lock if successful.  if fFalse is returned, pirangelock
//  contains either CMeteredSection::groupTooManyActiveErr or the current active group
//  it could not participate in, which means that pgno cannot be written at this time and the
//  caller does not need to LeaveRangeLock().  if fTrue is returned, that pgno may be
//  written.  if the caller decides to write that pgno, the caller must not call
//  LeaveRangeLock() until the write has completed.  if the caller decides not to write
//  that pgno, the caller must still LeaveRangeLock().

BOOL FMP::FEnterRangeLock( const PGNO pgno, CMeteredSection::Group* const pirangelock )
{
    *pirangelock = CMeteredSection::groupTooManyActiveErr;

    if( ErrFaultInjection( 39452 ) < JET_errSuccess )
    {
        return fFalse;
    }

    const CMeteredSection::Group irangelock = m_msRangeLock.GroupEnter();

    if ( irangelock == CMeteredSection::groupTooManyActiveErr )
    {
        return fFalse;
    }

    *pirangelock = irangelock;

    //  get a pointer to the specified range lock.  because the caller called
    //  EnterRangeLock() to get this range lock, it will be stable until they
    //  call LeaveRangeLock()

    const RANGELOCK* const prangelock = m_rgprangelock[ irangelock ];

    //  scan all ranges looking for this pgno

    for ( SIZE_T irange = 0; irange < prangelock->crange; irange++ )
    {
        //  the current range contains this pgno

        if ( prangelock->rgrange[ irange ].FContains( pgno ) )
        {
            //  this pgno is range locked

            m_msRangeLock.Leave( irangelock );
            return fFalse;
        }
    }

    //  we did not find a range that contains this pgno, so we successfully
    //  acquired the lock for this pgno

    return fTrue;
}

//  leaves the specified range lock, assert that pgno is allowed in the group

VOID FMP::LeaveRangeLock( const PGNO pgnoDebugOnly, const INT irangelock )
{
#ifdef DEBUG
    Assert( irangelock == 0 || irangelock == 1 );

    const RANGELOCK* const prangelock = m_rgprangelock[ irangelock ];

    for ( SIZE_T irange = 0; irange < prangelock->crange; irange++ )
    {
        Assert( !prangelock->rgrange[ irange ].FContains( pgnoDebugOnly ) );
    }
#endif  //  DEBUG

    m_msRangeLock.Leave( irangelock );
}

//  both inserts the range and locks it for further writes.

ERR FMP::ErrRangeLockAndEnter( const PGNO pgnoStart, const PGNO pgnoEnd, CMeteredSection::Group* const pirangelock )
{
    ERR err = JET_errSuccess;
    BOOL fRangeAdded = fFalse;
    BOOL fEnteredRange = fFalse;
    RANGE range( pgnoStart, pgnoEnd );

    //  prevent others from modifying the range lock while we are modifying the
    //  range lock.

    m_semRangeLock.Acquire();

    const RANGELOCK* const prangelock = m_rgprangelock[ m_msRangeLock.GroupActive() ];

    //  scan all ranges.

    for ( SIZE_T irange = 0; irange < prangelock->crange; irange++ )
    {
        //  the current range contains this page range

        if ( prangelock->rgrange[ irange ].FOverlaps( range ) )
        {
            //  this pgno is range locked

            Error( ErrERRCheck( JET_errTooManyActiveUsers ) );
        }
    }

    //  insert range.

    Call( ErrIAddRangeLock( pgnoStart, pgnoEnd ) );
    fRangeAdded = fTrue;

    //  add ourselves as consumers of the current group.

    *pirangelock = m_msRangeLock.GroupEnter();

    if ( *pirangelock == CMeteredSection::groupTooManyActiveErr )
    {
        Error( ErrERRCheck( JET_errTooManyActiveUsers ) );
    }
    fEnteredRange = fTrue;

HandleError:
    if ( fRangeAdded && ( err < JET_errSuccess ) )
    {
        Assert( !fEnteredRange );
        IRemoveRangeLock( pgnoStart, pgnoEnd );
        fRangeAdded = fFalse;
    }

    Assert( m_semRangeLock.CAvail() == 0 );
    m_semRangeLock.Release();

    Assert( ( ( err < JET_errSuccess ) && !fRangeAdded && !fEnteredRange ) ||
            ( ( err >= JET_errSuccess ) && fRangeAdded && fEnteredRange ) );
    
    return err;
}

VOID FMP::RangeUnlockAndLeave( const PGNO pgnoStart, const PGNO pgnoEnd, const CMeteredSection::Group irangelock )
{
#ifdef DEBUG
    RANGE range( pgnoStart, pgnoEnd );
    Assert( irangelock == 0 || irangelock == 1 );

    const RANGELOCK* const prangelock = m_rgprangelock[ irangelock ];
    BOOL fFoundRange = fFalse;

    for ( SIZE_T irange = 0; irange < prangelock->crange; irange++ )
    {
        if ( prangelock->rgrange[ irange ].FMatches( range ) )
        {
            Assert( !fFoundRange );
            fFoundRange = fTrue;
        }
        else
        {
            Assert( !prangelock->rgrange[ irange ].FOverlaps( range ) );
        }
    }

    Assert( fFoundRange );
#endif  //  DEBUG

    m_msRangeLock.Leave( irangelock );
    RangeUnlock( pgnoStart, pgnoEnd );
}

BOOL FMP::FPendingRedoMapEntries()
{
    BOOL fPendingRedoMapEntries = fFalse;

    FMP::EnterFMPPoolAsReader();

    for ( IFMP ifmp = cfmpReserved; ifmp < g_ifmpMax && FMP::FAllocatedFmp( ifmp ); ifmp++ )
    {
        if ( !g_rgfmp[ ifmp ].FRedoMapsEmpty() )
        {
            fPendingRedoMapEntries = fTrue;
            break;
        }
    }

    FMP::LeaveFMPPoolAsReader();

    return fPendingRedoMapEntries;
}

ERR FMP::ErrEnsureLogRedoMapsAllocated()
{
    m_sxwlRedoMaps.AcquireExclusiveLatch();

    Assert( ( m_pLogRedoMapZeroed == NULL ) == ( m_pLogRedoMapBadDbtime == NULL ) );
    Assert( ( m_pLogRedoMapDbtimeRevert == NULL ) == ( m_pLogRedoMapBadDbtime == NULL ) );
    Assert( ( m_pLogRedoMapDbtimeRevert == NULL ) == ( m_pLogRedoMapDbtimeRevertIgnore == NULL ) );

    Assert( m_pinst->FRecovering() && ( m_pinst->m_plog->FRecoveringMode() == fRecoveringRedo ) );

    ERR err = JET_errSuccess;

    if ( ( m_pLogRedoMapZeroed != NULL ) && ( m_pLogRedoMapBadDbtime != NULL ) && ( m_pLogRedoMapDbtimeRevert != NULL ) && ( m_pLogRedoMapDbtimeRevertIgnore != NULL ) )
    {
        m_sxwlRedoMaps.ReleaseExclusiveLatch();
        return JET_errSuccess;
    }

    m_sxwlRedoMaps.UpgradeExclusiveLatchToWriteLatch();

    if ( m_pLogRedoMapZeroed == NULL )
    {
        Alloc( m_pLogRedoMapZeroed = new CLogRedoMap() );
        Call( m_pLogRedoMapZeroed->ErrInitLogRedoMap( Ifmp() ) );
    }

    if ( m_pLogRedoMapBadDbtime == NULL )
    {
        Alloc( m_pLogRedoMapBadDbtime = new CLogRedoMap() );
        Call( m_pLogRedoMapBadDbtime->ErrInitLogRedoMap( Ifmp() ) );
    }

    if ( m_pLogRedoMapDbtimeRevert == NULL )
    {
        Alloc( m_pLogRedoMapDbtimeRevert = new CLogRedoMap() );
        Call( m_pLogRedoMapDbtimeRevert->ErrInitLogRedoMap( Ifmp() ) );
    }

    if ( m_pLogRedoMapDbtimeRevertIgnore == NULL )
    {
        Alloc( m_pLogRedoMapDbtimeRevertIgnore = new CLogRedoMap() );
        Call( m_pLogRedoMapDbtimeRevertIgnore->ErrInitLogRedoMap( Ifmp() ) );
    }

HandleError:
    Assert( m_sxwlRedoMaps.FOwnWriteLatch() );

    if ( err < JET_errSuccess )
    {
        FreeLogRedoMaps( fTrue /* fAllocCleanup */ );
    }

    m_sxwlRedoMaps.ReleaseWriteLatch();

    return err;
}

VOID FMP::FreeLogRedoMaps( const BOOL fAllocCleanup )
{
    if ( !fAllocCleanup )
    {
        m_sxwlRedoMaps.AcquireExclusiveLatch();
    }
    else
    {
        Assert( m_sxwlRedoMaps.FOwnWriteLatch() );
    }

    Assert( fAllocCleanup ||
            ( ( m_pLogRedoMapZeroed == NULL ) == ( m_pLogRedoMapBadDbtime == NULL ) &&
              ( m_pLogRedoMapBadDbtime == NULL ) == ( m_pLogRedoMapDbtimeRevert == NULL ) &&
              ( m_pLogRedoMapDbtimeRevert == NULL ) == ( m_pLogRedoMapDbtimeRevertIgnore == NULL ) ) );

    if ( ( m_pLogRedoMapZeroed == NULL ) && ( m_pLogRedoMapBadDbtime == NULL ) && ( m_pLogRedoMapDbtimeRevert == NULL ) && ( m_pLogRedoMapDbtimeRevertIgnore == NULL ) )
    {
        if ( !fAllocCleanup )
        {
            m_sxwlRedoMaps.ReleaseExclusiveLatch();
        }

        return;
    }

    if ( !fAllocCleanup )
    {
        m_sxwlRedoMaps.UpgradeExclusiveLatchToWriteLatch();
    }

    if ( m_pLogRedoMapZeroed != NULL )
    {
        m_pLogRedoMapZeroed->TermLogRedoMap();
        delete m_pLogRedoMapZeroed;
        m_pLogRedoMapZeroed = NULL;
    }

    if ( m_pLogRedoMapBadDbtime != NULL )
    {
        m_pLogRedoMapBadDbtime->TermLogRedoMap();
        delete m_pLogRedoMapBadDbtime;
        m_pLogRedoMapBadDbtime = NULL;
    }

    if ( m_pLogRedoMapDbtimeRevert != NULL )
    {
        m_pLogRedoMapDbtimeRevert->TermLogRedoMap();
        delete m_pLogRedoMapDbtimeRevert;
        m_pLogRedoMapDbtimeRevert = NULL;
    }

    if ( m_pLogRedoMapDbtimeRevertIgnore != NULL )
    {
        m_pLogRedoMapDbtimeRevertIgnore->TermLogRedoMap();
        delete m_pLogRedoMapDbtimeRevertIgnore;
        m_pLogRedoMapDbtimeRevertIgnore = NULL;
    }

    if ( !fAllocCleanup )
    {
        m_sxwlRedoMaps.ReleaseWriteLatch();
    }
    else
    {
        Assert( m_sxwlRedoMaps.FOwnWriteLatch() );
    }
}

BOOL FMP::FRedoMapsEmpty()
{
    m_sxwlRedoMaps.AcquireSharedLatch();

    Assert( ( m_pLogRedoMapZeroed == NULL ) == ( m_pLogRedoMapBadDbtime == NULL ) );
    Assert( ( m_pLogRedoMapDbtimeRevert == NULL ) == ( m_pLogRedoMapBadDbtime == NULL ) );
    Assert( ( m_pLogRedoMapDbtimeRevertIgnore == NULL ) == ( m_pLogRedoMapBadDbtime == NULL ) );    

    const BOOL fRedoMapsEmpty = ( ( m_pLogRedoMapZeroed == NULL ) || ( !m_pLogRedoMapZeroed->FAnyPgnoSet() ) ) &&
                                ( ( m_pLogRedoMapBadDbtime == NULL ) || ( !m_pLogRedoMapBadDbtime->FAnyPgnoSet() ) ) &&
                                ( ( m_pLogRedoMapDbtimeRevert == NULL ) || ( !m_pLogRedoMapDbtimeRevert->FAnyPgnoSet() ) ) &&
                                ( ( m_pLogRedoMapDbtimeRevertIgnore == NULL ) || ( !m_pLogRedoMapDbtimeRevertIgnore->FAnyPgnoSet() ) );

    m_sxwlRedoMaps.ReleaseSharedLatch();

    return fRedoMapsEmpty;
}

ULONG_PTR FMP::UlDiskId() const
{
    ULONG_PTR ulDiskId = 0;
    
    if ( Pfapi()->ErrDiskId( &ulDiskId ) < JET_errSuccess )
    {
        ulDiskId = 0;
    }

    Expected( ulDiskId != 0 );

    return ulDiskId;
}

BOOL FMP::FSeekPenalty() const
{
    return Pfapi()->FSeekPenalty();
}

PdbfilehdrReadOnly FMP::Pdbfilehdr() const
{
    return const_cast<FMP *>(this)->m_dbfilehdrLock.GetROHeader();
}

PdbfilehdrReadWrite FMP::PdbfilehdrUpdateable()
{
    return m_dbfilehdrLock.GetRWHeader();
}

ERR FMP::ErrSetPdbfilehdr( DBFILEHDR_FIX * pdbfilehdr, _Out_ DBFILEHDR ** ppdbfilehdr )
{
    ERR err = JET_errSuccess;

    *ppdbfilehdr = NULL;

    if ( NULL != pdbfilehdr && !FReadOnlyAttach() )
    {
        const INST *    pinst       = Pinst();
        DBID            dbid;

        Assert ( WszDatabaseName() );
        Assert ( !FSkippedAttach() );

        EnterFMPPoolAsReader();
        for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            if ( pinst->m_mpdbidifmp[ dbid ] >= g_ifmpMax || dbid == m_dbid )
                continue;

            FMP *pfmpT;

            pfmpT = &g_rgfmp[ pinst->m_mpdbidifmp[ dbid ] ];

            PdbfilehdrReadOnly pdbfilehdrCurrFmp = pfmpT->Pdbfilehdr();
            if ( pdbfilehdrCurrFmp
                    && !pfmpT->FReadOnlyAttach()
                    && 0 == memcmp( &( pdbfilehdr->signDb), &(pdbfilehdrCurrFmp->signDb), sizeof( SIGNATURE ) ) )
            {
                LeaveFMPPoolAsReader();
                Error( ErrERRCheck( JET_errDatabaseSignInUse ) );
            }
        }

        LeaveFMPPoolAsReader();
        CallS( err );
    }
    *ppdbfilehdr = m_dbfilehdrLock.SetPdbfilehdr( pdbfilehdr );
HandleError:
    return err;
}

VOID FMP::InitializeDbtimeOldest()
{
    // Setup variables needed for dbtime-oldest-guaranteed to work
    SetDbtimeOldestGuaranteed( DbtimeLast() );
    SetDbtimeOldestCandidate( DbtimeLast() );
    SetDbtimeOldestTarget( DbtimeLast() );
    SetTrxOldestCandidate( m_pinst->m_trxNewest );
    SetTrxOldestTarget( trxMax );
}

#ifdef PERFMON_SUPPORT
extern PERFInstanceDelayedTotal<QWORD, INST, fFalse, fFalse> lOldestTransaction;
#endif

VOID FMP::UpdateDbtimeOldest()
{
    const DBTIME    dbtimeLast  = DbtimeGet();  //  grab current dbtime before next trx begins
    TICK            tickOldestTransaction = TickOSTimeCurrent();

    Assert( FReaderFMPPool() );

    //  caller tries to prevent this function from being
    //  called if we're terminating, but there's no guarantee,
    //  because right after that check, another thread may initiate
    //  termination, so we can't perform the m_ppibGlobal assert
    //  below, but it's not a big deal, as the rest of the code will
    //  do the right thing even if we're terminating (it will simply
    //  detect there are no outstanding transactions)
    //  Assert( ppibNil != m_pinst->m_ppibGlobal );

    const size_t    cProcs      = (size_t)OSSyncGetProcessorCountMax();
    size_t iProc;
    for ( iProc = 0; iProc < cProcs; iProc++ )
    {
        INST::PLS* const ppls = m_pinst->Ppls( iProc );
        ppls->m_rwlPIBTrxOldest.EnterAsWriter();
    }
    TRX trxOldest   = trxMax;
    TRX trxNewest   = m_pinst->m_trxNewest;
    for ( iProc = 0; iProc < cProcs; iProc++ )
    {
        INST::PLS* const ppls = m_pinst->Ppls( iProc );
        PIB* const ppibTrxOldest = ppls->m_ilTrxOldest.PrevMost();
        if ( ppibTrxOldest && TrxCmp( ppibTrxOldest->trxBegin0, trxOldest ) < 0 )
        {
            trxOldest = ppibTrxOldest->trxBegin0;
            tickOldestTransaction = ppibTrxOldest->TickLevel0Begin();
        }
    }
    for ( iProc = 0; iProc < cProcs; iProc++ )
    {
        INST::PLS* const ppls = m_pinst->Ppls( iProc );
        ppls->m_rwlPIBTrxOldest.LeaveAsWriter();
    }

    //  Update perf counter
    PERFOpt( lOldestTransaction.Set( m_pinst, DtickDelta( tickOldestTransaction, TickOSTimeCurrent() ) ) );

    //  if we've reached the target trxOldest, then we
    //  can update the guaranteed dbtimeOldest
    const TRX       trxOldestCandidate  = TrxOldestCandidate();
    const TRX       trxOldestTarget     = TrxOldestTarget();

    Assert( trxMax == trxOldestTarget
        || TrxCmp( trxOldestCandidate, trxOldestTarget ) <= 0 );

    if ( trxMax == trxOldestTarget )
    {
        //  need to wait until the current candidate is
        //  older than the oldest transaction before
        //  we can establish a new target
        if ( TrxCmp( trxOldestCandidate, trxOldest ) < 0 )
        {
            SetDbtimeOldestTarget( dbtimeLast );
            SetTrxOldestTarget( trxNewest );
        }
    }
    else if ( TrxCmp( trxOldestTarget, trxOldest ) < 0 )
    {
        //  the target trxOldest has now been reached
        //  (ie. it is older than the oldest transaction,
        //  so the candidate dbtimeOldest is now the
        //  guaranteed dbtimeOldest
        //  UNDONE: need a better explanation than
        //  that to explain this complicated logic
        const DBTIME    dbtimeOldestGuaranteed  = DbtimeOldestGuaranteed();
        const DBTIME    dbtimeOldestCandidate   = DbtimeOldestCandidate();
        const DBTIME    dbtimeOldestTarget      = DbtimeOldestTarget();

        Assert( dbtimeOldestGuaranteed <= dbtimeOldestCandidate );
        Assert( dbtimeOldestCandidate <= dbtimeOldestTarget );
        Assert( dbtimeOldestTarget <= dbtimeLast );

        SetDbtimeOldestGuaranteed( dbtimeOldestCandidate );
        SetDbtimeOldestCandidate( dbtimeOldestTarget );
        SetDbtimeOldestTarget( dbtimeLast );
        SetTrxOldestCandidate( trxOldestTarget );
        SetTrxOldestTarget( trxMax );

        //  we know oldest transaction has advanced, so take this
        //  opportunity to signal version cleanup as well if it
        //  hasn't been signalled in a while
        //
        if ( TickOSTimeCurrent() - Pinst()->m_pver->m_tickLastRCEClean > VER::dtickRCECleanPeriod )
        {
            Pinst()->m_pver->VERSignalCleanup();
        }
    }
}

ERR FMP::ErrObjidLastIncrementAndGet( OBJID *pobjid )
{
    Assert( NULL != pobjid );

    if ( !FAtomicIncrementMax( &m_objidLast, pobjid, objidFDPMax ) )
    {
        const WCHAR *rgpszT[] = { WszDatabaseName() };
        UtilReportEvent(
                eventError,
                GENERAL_CATEGORY,
                OUT_OF_OBJID,
                _countof( rgpszT ),
                rgpszT,
                0,
                NULL,
                Pinst() );

        OSUHAPublishEvent(
            HaDbFailureTagAlertOnly, Pinst(), HA_GENERAL_CATEGORY,
            HaDbIoErrorNone, WszDatabaseName(), 0, 0,
            HA_OUT_OF_OBJID, _countof( rgpszT ), rgpszT );

        return ErrERRCheck( JET_errOutOfObjectIDs );
    }

    //  FAtomicIncrementMax() returns the objid before the increment
    (*pobjid)++;

    //  periodically notify user to defrag database if neccessary
    if ( *pobjid > objidMaxWarningThreshold )
    {
        const ULONG ulFrequency     = ( *pobjid > objidMaxPanicThreshold ?
                                                ulObjidMaxPanicFrequency :
                                                ulObjidMaxWarningFrequency );
        if ( *pobjid % ulFrequency == 0 )
        {
            const WCHAR *rgpszT[1] = { WszDatabaseName() };
            UtilReportEvent(
                    eventWarning,
                    GENERAL_CATEGORY,
                    ALMOST_OUT_OF_OBJID,
                    1,
                    rgpszT,
                    0,
                    NULL,
                    Pinst() );
        }
    }

    return JET_errSuccess;
}

VOID FMP::SetILgposWaypoint( const LGPOS &lgposWaypoint )
{

    // Must own the FMPPool to set a new waypoint ...
    Assert( FWriterFMPPool() ||
            ( FReaderFMPPool() && m_pinst->m_plog->FOwnerCheckpoint() ) );
    
    // Today we don't allow anything but end of a log or lgposMin ... this allows
    // us to play tricks w/ how we "atomically" read this in LgposWaypoint().
    Assert( ( lgposWaypoint.isec == lgposMax.isec && lgposWaypoint.ib == lgposMax.ib ) ||
            ( CmpLgpos( &lgposWaypoint, &lgposMin ) == 0 ) );
    // oh, and assert it was already in such a state ...
    Assert( ( m_lgposWaypoint.isec == lgposMax.isec && m_lgposWaypoint.ib == lgposMax.ib ) ||
            ( CmpLgpos( (LGPOS*)&m_lgposWaypoint, &lgposMin ) == 0 ) );

    // We should never reach lgposMax ... at least today.
    Assert( CmpLgpos( &lgposWaypoint, &lgposMax ) != 0 );

    if ( CmpLgpos( &lgposWaypoint, &lgposMin) != 0  )
    {
        const IFMP ifmp = Ifmp();

        // If we're not resetting our waypoing to "nil" ...
    
        // Like kitten climbing a tree, we can never go back down.  We can only establish 
        // a new waypoint that is as good as or higher up, than our current waypoint.
        DBEnforce( ifmp, CmpLgpos( &lgposWaypoint, (LGPOS*)&m_lgposWaypoint ) >= 0 );

        // We should never have gotten to a point where we are moving the in memory 
        // waypoint beyond what we have established as the waypoint in our header.
    {
        PdbfilehdrReadOnly pdbfilehdr = Pdbfilehdr();
        if ( pdbfilehdr )
        {
            DBEnforce( ifmp, pdbfilehdr->le_lGenMaxRequired >= lgposWaypoint.lGeneration );
        }
    }
    }

    // Note we're setting these explicitly simply so that we can be assured on
    // read we'll have an atomic read of lGeneration, which is the only aspect
    // of the waypoint we use today.
    m_lgposWaypoint.lGeneration = lgposWaypoint.lGeneration;
    if ( m_lgposWaypoint.lGeneration == 0 )
    {
        m_lgposWaypoint.isec = lgposMin.isec;
        m_lgposWaypoint.ib = lgposMin.ib;
    }
    else
    {
        m_lgposWaypoint.isec = lgposMax.isec;
        m_lgposWaypoint.ib = lgposMax.ib;
    }
}

LGPOS FMP::LgposWaypoint( ) const
{
    //  We are being way too cunning for our own good ... we're using the fact that
    //  we only need the lGeneration to infer the rest of the lgpos.
    LONG lGen = m_lgposWaypoint.lGeneration;
    LGPOS lgposActualWaypoint;
    if ( lGen == 0 )
    {
        lgposActualWaypoint = lgposMin;
    }
    else
    {
        lgposActualWaypoint = lgposMax;
        lgposActualWaypoint.lGeneration = lGen;
    }
    return lgposActualWaypoint;
}

//  ================================================================
ERR FMP::ErrStartDBMScan()
//  ================================================================
{
    ERR err = JET_errSuccess;
    const IFMP ifmp = Ifmp();

    SemDBM().Acquire();

    //  we should be using either active / normal scanning or following, but
    //  not both concurrently.
    Expected( m_pdbmFollower == NULL );

    if ( g_rgfmp[ ifmp ].FDontStartDBM() )
    {
        err = JET_errSuccess;
        goto HandleError;
    }

    if ( g_rgfmp[ ifmp ].FReadOnlyAttach() )
    {
        Call( ErrERRCheck( JET_errDatabaseFileReadOnly ) );
    }

    if ( NULL == m_pdbmScan )
    {
        Call( DBMScanFactory::ErrPdbmScanCreate( ifmp, &m_pdbmScan ) );
    }
    Call( m_pdbmScan->ErrStartDBMScan() );

HandleError:

    Assert( 0 == SemDBM().CAvail() );
    SemDBM().Release();

    // Starting a scan when one is already running returns an error.
    // In that case we don't want to stop the existing scan.
    if ( err < JET_errSuccess && JET_errDatabaseAlreadyRunningMaintenance != err )
    {
        StopDBMScan();
    }
    return err;
}

//  ================================================================
ERR FMP::ErrStartDBMScanSinglePass(
    JET_SESID sesid,
    const INT csecMax,
    const INT cmsecSleep,
    const JET_CALLBACK pfnCallback)
//  ================================================================
{
    ERR err = JET_errSuccess;
    const IFMP ifmp = Ifmp();

    SemDBM().Acquire();

    //  we should be using either active / normal scanning or following, but
    //  not both concurrently.
    Expected( m_pdbmFollower == NULL );

    if ( g_rgfmp[ ifmp ].FDontStartDBM() )
    {
        err = JET_errSuccess;
        goto HandleError;
    }

    if ( g_rgfmp[ ifmp ].FReadOnlyAttach() )
    {
        Call( ErrERRCheck( JET_errDatabaseFileReadOnly ) );
    }

    if ( NULL == m_pdbmScan )
    {
        Call( DBMScanFactory::ErrPdbmScanCreateSingleScan(
            sesid,
            ifmp,
            csecMax,
            cmsecSleep,
            pfnCallback,
            &m_pdbmScan ) );
    }
    Call( m_pdbmScan->ErrStartDBMScan() );

HandleError:

    Assert( 0 == SemDBM().CAvail() );
    SemDBM().Release();

    // Starting a scan when one is already running returns an error.
    // In that case we don't want to stop the existing scan.
    if (err < 0 && JET_errDatabaseAlreadyRunningMaintenance != err )
    {
        StopDBMScan();
    }
    return err;
}

//  ================================================================
VOID FMP::StopDBMScan()
//  ================================================================
{
    //  we should be using either active / normal scanning or following, but
    //  not both concurrently.
    Expected( m_pdbmFollower == NULL );

    if ( NULL != m_pdbmScan )
    {
        SemDBM().Acquire();

        if ( NULL != m_pdbmScan )
        {
            IDBMScan *  pdbmScan = m_pdbmScan;
            m_pdbmScan = NULL;

            pdbmScan->StopDBMScan();
            delete pdbmScan;
        }
        
        SemDBM().Release();
    }
}

//  ================================================================
BOOL FMP::FDBMScanOn()
//  ================================================================
{
    //  For now we don't have DBMScan / follower mode checked here as this really only
    //  deals with disabling DBM for things like trim.  Since everything is logged,
    //  and DBMScan in follower mode is sequentialized in the log stream, I'm not going
    //  to modify this.  Also switching DBM in follower mode off, is a more complex
    //  issue, which could involve a piece of the DB being missed for the whole pass
    //  as in follower mode we pick up whereever replay is at.
    return ( NULL != m_pdbmScan );
}

//  ================================================================
ERR FMP::ErrCreateDBMScanFollower()
//  ================================================================
{
    //  This is an Assert( not an Expected ), because the follower code requires we
    //  have a clean singular initialization, and because we wouldn't expect dueling
    //  threads to call this concurrently at the same time, should only be called by
    //  the JetInit() thread.
    Assert( m_pdbmFollower == NULL );

    if ( m_pdbmFollower == NULL )
    {
        ERR err = JET_errSuccess;
        AllocR( m_pdbmFollower = new CDBMScanFollower() );
        Assert( PdbmFollower() != NULL );
    }

    return JET_errSuccess;
}

//  ================================================================
VOID FMP::DestroyDBMScanFollower()
//  ================================================================
{
    Expected( m_pdbmFollower != NULL );
    if ( m_pdbmFollower )
    {
        Assert( m_pdbmFollower->FStopped() );
        delete m_pdbmFollower;
        m_pdbmFollower = NULL;
    }
}

//  ================================================================
CDBMScanFollower * FMP::PdbmFollower() const
//  ================================================================
{
    return m_pdbmFollower;
}

//  ================================================================
VOID FMP::IncrementAsyncIOForViewCache()
//  ================================================================
{
    Assert( BoolParam( JET_paramEnableViewCache ) );

    const LONG cAsyncIOForViewCachePending = AtomicIncrement( (LONG*)&m_cAsyncIOForViewCachePending );
    Assert( cAsyncIOForViewCachePending > 0 );

    //  detects resource leakage
    Assert( cAsyncIOForViewCachePending <= 30 /* due to small concurrency hole */ + (LONG)(UlParam( JET_paramOutstandingIOMax ) * UlParam(JET_paramMaxCoalesceReadSize) / g_cbPage));
}

//  ================================================================
VOID FMP::DecrementAsyncIOForViewCache()
//  ================================================================
{
    Assert( BoolParam( JET_paramEnableViewCache ) );

    const LONG cAsyncIOForViewCachePending = AtomicDecrement( (LONG*)&m_cAsyncIOForViewCachePending );
    Assert( cAsyncIOForViewCachePending >= 0 );

    //  detects resource leakage
    Assert( cAsyncIOForViewCachePending < 30 /* due to small concurrency hole */ + (LONG)(UlParam( JET_paramOutstandingIOMax ) * UlParam(JET_paramMaxCoalesceReadSize) / g_cbPage));
}

//  ================================================================
VOID FMP::WaitForAsyncIOForViewCache()
//  ================================================================
{
    if ( m_cAsyncIOForViewCachePending > 0 )
    {
        FireWall( "AsyncIoForViewCachePending" );
    }

    // wait for non-BF async IO's to complete (not returning this as an error because db will not need repair)

    const INT cmsecDelay = 100;
    INT cmsecTotal = 0;
    
    while ( m_cAsyncIOForViewCachePending > 0 )
    {
        UtilSleep( cmsecDelay );
        cmsecTotal += cmsecDelay;

        if ( cmsecTotal > cmsecDeadlock )
        {
            AssertSz( fFalse, "Asynchronous Read File I/O appears to be hung." );
        }
    }
}

//  ================================================================
ERR FMP::ErrPgnoLastFileSystem( PGNO* const ppgnoLast ) const
//  ================================================================
{
    ERR err = JET_errSuccess;
    QWORD cbSize = 0;

    Call( Pfapi()->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
    *ppgnoLast = (PGNO)( cbSize / g_cbPage );

    if ( *ppgnoLast <= cpgDBReserved )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    *ppgnoLast -= cpgDBReserved;

HandleError:
    return err;
}

//  ================================================================
ERR FMP::FPgnoInZeroedOrRevertedMaps( const PGNO pgno ) const
//  ================================================================
{
    return ( PLogRedoMapZeroed() && PLogRedoMapZeroed()->FPgnoSet( pgno ) ) || 
           ( PLogRedoMapDbtimeRevert() && PLogRedoMapDbtimeRevert()->FPgnoSet( pgno ) ) ||
           ( PLogRedoMapDbtimeRevert() && PLogRedoMapDbtimeRevertIgnore()->FPgnoSet( pgno ) );
}

//  ================================================================
VOID FMP::PauseOLD2Tasks()
//  ================================================================
{
    OLD2PauseTasks();
}

//  ================================================================
VOID FMP::UnpauseOLD2Tasks()
//  ================================================================
{
    OLD2UnpauseTasks();
}

//  ================================================================
BOOL FMP::FOLD2TasksPaused()
//  ================================================================
{
    return COLD2Tasks() == 0;
}

#ifdef ENABLE_JET_UNIT_TEST

// these are used by testing routines that need a mock FMP
FMP * FMP::PfmpCreateMockFMP()
{
    FMP * pfmp = new FMP();
    pfmp->m_wszDatabaseName = L"mockfmp.edb";
    DBFILEHDR * pdbfilehdr  = new DBFILEHDR();
    pfmp->m_dbfilehdrLock.SetPdbfilehdr(pdbfilehdr);
    return pfmp;
}

void FMP::FreeMockFMP(FMP * const pfmp)
{
    delete pfmp->m_dbfilehdrLock.SetPdbfilehdr(NULL);
    delete pfmp;
}

#endif // ENABLE_JET_UNIT_TEST

