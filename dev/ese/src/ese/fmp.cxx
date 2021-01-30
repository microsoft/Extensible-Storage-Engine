// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_logredomap.hxx"


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




FMP::FMP()
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

{
    m_dataHdrSig.Nullify();
    m_semIOSizeChange.Release();
    m_semRangeLock.Release();
    m_semTrimmingDB.Release();
    m_semDBM.Release();
}

FMP::~FMP()
{
    Assert( NULL == m_pdbmScan );
    Assert( NULL == m_pdbmFollower );
    Assert( m_cAsyncIOForViewCachePending == 0 );
    Assert( NULL == m_pLogRedoMapZeroed );
    Assert( NULL == m_pLogRedoMapBadDbtime );
}





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

FMP *   g_rgfmp       = NULL;
IFMP    g_ifmpMax     = 0;
IFMP    g_ifmpTrace = (IFMP)JET_dbidNil;

CReaderWriterLock FMP::rwlFMPPool( CLockBasicInfo( CSyncBasicInfo( szFMPGlobal ), rankFMPGlobal, 0 ) );



VOID FMP::GetWriteLatch( PIB *ppib )
{

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

    
    this->ResetWriteLatch( ppib );
    if ( m_gateWriteLatch.CWait() > 0 )
    {
        m_gateWriteLatch.Release( m_critLatch, 1 );
    }
    else
    {
        m_critLatch.Leave();
    }
}


ERR FMP::RegisterTask()
{
    AtomicIncrement( (LONG *)&m_ctasksActive );
    return JET_errSuccess;
}


ERR FMP::UnregisterTask()
{
    AtomicDecrement( (LONG *)&m_ctasksActive );
    return JET_errSuccess;
}


VOID FMP::WaitForTasksToComplete()
{
    while( 0 != m_ctasksActive )
    {
        UtilSleep( cmsecWaitGeneric );
    }
}

#ifdef DEBUG

BOOL FMP::FHeaderUpdatePending() const
{
    BOOL fRet = m_dbhus == dbhusUpdateLogged;

#if 0
    Expected( fRet || m_dbhus == dbhusUpdateSet || m_dbhus == dbhusHdrLoaded || m_dbhus == dbhusUpdateFlushed );
    Assert( fRet || m_dbhus == dbhusUpdateSet || m_dbhus == dbhusHdrLoaded || m_dbhus == dbhusUpdateFlushed || m_dbhus == dbhusNoUpdate );
#endif

    return fRet;
}

BOOL FMP::FHeaderUpdateInProgress() const
{
    BOOL fRet = m_dbhus == dbhusUpdateLogged || m_dbhus == dbhusUpdateSet;

#if 0
    Expected( fRet || m_dbhus == dbhusHdrLoaded || m_dbhus == dbhusUpdateFlushed );
    Assert( fRet || m_dbhus == dbhusHdrLoaded || m_dbhus == dbhusUpdateFlushed || m_dbhus == dbhusNoUpdate );
#endif

    return fRet;
}

VOID FMP::SetDbHeaderUpdateState( _In_ const DbHeaderUpdateState dbhusNewState )
{

    Assert( dbhusNewState != dbhusNoUpdate );

#if 0
    const BOOL fLoggingNeeded = FLogOn() &&
                                    !( m_pinst->FRecovering() && m_pinst->m_plog->FRecoveringMode() == fRecoveringRedo );

    switch( dbhusNewState )
    {
    case dbhusHdrLoaded:
        Assert( m_dbhus == dbhusNoUpdate );
        break;

    case dbhusUpdateLogged:
        Assert( m_dbhus == dbhusHdrLoaded ||
                m_dbhus == dbhusUpdateFlushed );
        break;

    case dbhusUpdateSet:
        Assert( m_dbhus == dbhusUpdateLogged ||
                m_dbhus == dbhusUpdateFlushed ||
                m_dbhus == dbhusUpdateSet  );

        Assert( m_dbhus == dbhusUpdateLogged ||
                ( m_dbhus == dbhusUpdateFlushed && !fLoggingNeeded ) ||
                m_dbhus == dbhusUpdateSet  );
        break;

    case dbhusUpdateFlushed:
        Assert( m_dbhus != dbhusUpdateLogged || !fLoggingNeeded );
        Assert( m_dbhus == dbhusHdrLoaded || m_dbhus == dbhusUpdateSet || m_dbhus == dbhusUpdateFlushed ||
                    ( m_dbhus == dbhusUpdateLogged && !fLoggingNeeded ) );
        Assert( m_dbhus == dbhusHdrLoaded || m_dbhus == dbhusUpdateSet || m_dbhus == dbhusUpdateFlushed );
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
#endif

    AtomicExchange( (LONG*)&m_dbhus, dbhusNewState );
}
#endif


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
    const FormatVersions * pfmtversMax = PfmtversEngineMax();

    Expected( efvFormatFeature <= pfmtversMax->efv );
    if ( efvFormatFeature <= pfmtversMax->efv &&
            0 == CmpFormatVer( &pfmtversMax->fmv, &fmvCurrentFromFile ) )
    {
        OSTrace( JET_tracetagVersionAndStagingChecks, OSFormat( "FM Feature requested EFV %d success (fast) ... { %d ... %d.%d.%d == %d.%d.%d }\n",
                    efvFormatFeature, pfmtversMax->efv,
                    pfmtversMax->fmv.ulVersionMajor, pfmtversMax->fmv.ulVersionUpdateMajor, pfmtversMax->fmv.ulVersionUpdateMinor,
                    fmvCurrentFromFile.ulVersionMajor, fmvCurrentFromFile.ulVersionUpdateMajor, fmvCurrentFromFile.ulVersionUpdateMinor ) );
        return JET_errSuccess;
    }


    const FormatVersions * pfmtversFeatureVersion = NULL;
    CallS( ErrGetDesiredVersion( NULL , efvFormatFeature, &pfmtversFeatureVersion ) );
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


ERR FMP::ErrCreateFlushMap( const JET_GRBIT grbit )
{
    ERR err = JET_errSuccess;
    CFlushMapForAttachedDb* pflushmap = NULL;

    Assert( NULL == m_pflushmap );
    
    JET_ENGINEFORMATVERSION efvDesired = (JET_ENGINEFORMATVERSION)UlParam( m_pinst, JET_paramEngineFormatVersion );
    if ( efvDesired == JET_efvUsePersistedFormat )
    {
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

VOID FMP::DestroyFlushMap()
{
    delete m_pflushmap;
    m_pflushmap = NULL;
}


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
            pfmp->Pinst() == PinstFromPpib( ppib ) )
        {
            pfmp->CritLatch().Enter();
            if ( pfmp->FWriteLatchByOther(ppib) )
            {
                
                LeaveFMPPoolAsReader();
                pfmp->GateWriteLatch().Wait( pfmp->CritLatch() );
                goto Start;
            }

            
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


    if ( !pfmp->FInUse() || PinstFromPpib( ppib ) != pfmp->Pinst() )
    {
        LeaveFMPPoolAsReader();
        return ErrERRCheck( JET_errDatabaseNotFound );
    }


    pfmp->CritLatch().Enter();
    if ( pfmp->FWriteLatchByOther(ppib) )
    {
        
        LeaveFMPPoolAsReader();
        pfmp->GateWriteLatch().Wait( pfmp->CritLatch() );
        goto Start;
    }


    pfmp->SetWriteLatch( ppib );
    pfmp->CritLatch().Leave();

    LeaveFMPPoolAsReader();

    return JET_errSuccess;
}

ERR FMP::ErrResolveByNameWsz( const _In_z_ WCHAR *wszDatabaseName, _Out_writes_z_( cwchFullName ) WCHAR *wszFullName, const size_t cwchFullName )
{
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
    const IOREASONUSER ioru = PetcTLSGetEngineContext()->iorReason.Ioru();
    if ( !FBFWriteLatched( Ifmp(), pgnoSystemRoot ) &&
         !FBFRDWLatched( Ifmp(), pgnoSystemRoot ) &&
         !FBFWARLatched( Ifmp(), pgnoSystemRoot ) )
    {
        Assert( ( FShrinkIsRunning() && ( ioru == opAttachDatabase ) ) ||
                ( FAttachingDB() && ( ioru == opAttachDatabase ) ) ||
                ( FCreatingDB() && ( ( ioru == opCreateDatabase ) || m_pinst->m_plog->FRecovering() ) ) ||
                ( ( FAttachingDB() || FCreatingDB() ) && ( ( ioru == opDBUtilities ) || ( ioru == opCompact ) || ( m_dbid == dbidTemp ) ) ) ||
                ( FAttachedForRecovery() && ( m_pinst->m_plog->FRecovering() || ( ioru == opAttachDatabase ) ) ) ||
                g_fRepair );
    }
#endif
}

VOID FMP::AssertSizesConsistent( const BOOL fHasIoSizeLockDebugOnly )
{
    Assert( !fHasIoSizeLockDebugOnly || !FIOSizeChangeLatchAvail() );

    const QWORD cbOwnedFileSize = CbOwnedFileSize();
    const QWORD cbFsFileSizeAsyncTarget = CbFsFileSizeAsyncTarget();

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
        Assert( cbOwnedFileSize <= cbFsFileSize );

        Assert( !fHasIoSizeLockDebugOnly || ( cbFsFileSize <= cbFsFileSizeAsyncTarget ) );
    }

    Assert( cbOwnedFileSize <= cbFsFileSizeAsyncTarget );
}
#endif

VOID FMP::GetPeriodicTrimmingDBLatch()
{

    Assert( g_fPeriodicTrimEnabled );
    SemPeriodicTrimmingDB().Acquire();
}

VOID FMP::ReleasePeriodicTrimmingDBLatch()
{

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

INLINE BOOL IsFMPUsable( IFMP ifmp, BOOL fUseAny )
{
    if ( ifmp < cfmpReserved )
    {
        return fFalse;
    }
    const FMP* pfmp = &g_rgfmp[ ifmp ];
    return !pfmp->FInUse() && ( fUseAny || pfmp->DataHeaderSignature().FNull() );
}


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

    if ( pinst->FRecovering() )
    {
        Assert( pinst->m_plog );
        LOG *plog = pinst->m_plog;

        if ( plog->FExternalRestore() || ( plog->IrstmapMac() > 0 && !plog->FHardRestore() ) )
        {
            if ( 0 > plog->IrstmapSearchNewName( wszDatabaseName ) )
            {
                fSkippedAttach = fTrue;
            }
        }
    }


    EnterFMPPoolAsWriter();


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

    if ( fCheckForAttachInfoOverflow && dbidGiven != dbidTemp )
    {
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
        
        for ( ifmp = cfmpReserved; ifmp < g_ifmpMax && FMP::FAllocatedFmp( ifmp ); ifmp++ )
        {
            if ( !g_rgfmp[ifmp].DataHeaderSignature().FNull() )
            {
                DBFILEHDR_FIX* const pdbfilehdrsig = (DBFILEHDR_FIX*)g_rgfmp[ifmp].DataHeaderSignature().Pv();
                Assert( g_rgfmp[ifmp].DataHeaderSignature().Cb() >= sizeof( DBFILEHDR ) );
                AssertDatabaseHeaderConsistent( pdbfilehdrsig, g_rgfmp[ifmp].DataHeaderSignature().Cb(), g_cbPage );
                C_ASSERT( offsetof( DBFILEHDR, le_ulMagic ) == sizeof( pdbfilehdr->le_ulChecksum ) );
                OnDebug( const BYTE* const pbhdrCheck = (BYTE*)pdbfilehdrsig );
                Assert( !FUtilZeroed( pbhdrCheck + offsetof( DBFILEHDR, le_ulMagic ), offsetof( DBFILEHDR, le_filetype ) - sizeof( pdbfilehdr->le_ulChecksum ) ) );

                if ( NULL == pdbfilehdr )
                {
                    pdbfilehdr = (DBFILEHDR_FIX*)PvOSMemoryPageAlloc( g_cbPage, NULL );
                    if ( pdbfilehdr )
                    {
                        memset( pdbfilehdr, 0, g_cbPage );
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
                    {
                        Assert( !g_rgfmp[ifmp].FInUse() );
                        OSMemoryPageFree( pdbfilehdr );
                        pdbfilehdr = NULL;
                        fFoundMatchingFmp = fTrue;
                        goto SelectedIfmp;
                    }
                }
            }
        }

        if ( pdbfilehdr )
        {
            OSMemoryPageFree( pdbfilehdr );
            pdbfilehdr = NULL;
        }
    }


    OnDebug( BOOL fDidCanabalizeUnattachedCachesPass = fFalse );
#pragma warning(suppress: 6293)
    for ( BOOL fPreserveUnattachedCaches = fTrue; fPreserveUnattachedCaches != 0xFFFFFFFF; fPreserveUnattachedCaches-- )
    {

        ULONG cFmpsEvaluated = 0;
        ULONG cPreservedCaches = 0;

        for ( ifmp = IfmpMinInUse(); ifmp <= IfmpMacInUse(); ifmp++ )
        {
            cFmpsEvaluated++;
            if ( g_rgfmp[ifmp].FInUse() )
                continue;
            if ( fPreserveUnattachedCaches && !g_rgfmp[ifmp].DataHeaderSignature().FNull() )
            {
                cPreservedCaches++;
                continue;
            }
            goto SelectedIfmp;
        }
        for ( ifmp = IfmpMinInUse() - 1; IfmpMacInUse() != 0 && ifmp >= cfmpReserved && ifmp < IfmpMinInUse() ; ifmp-- )
        {
            Assert( NULL == g_rgfmp[ifmp].WszDatabaseName() ); // == Assert( !g_rgfmp[ifmp].FInUse() );

            cFmpsEvaluated++;
            if ( fPreserveUnattachedCaches && !g_rgfmp[ifmp].DataHeaderSignature().FNull() )
            {
                cPreservedCaches++;
                continue;
            }
            goto SelectedIfmp;
        }
        for ( ifmp = IfmpMacInUse() + 1; ifmp < g_ifmpMax && FMP::FAllocatedFmp( ifmp ); ifmp++ )
        {
            Assert( NULL == g_rgfmp[ifmp].WszDatabaseName() ); // == Assert( !g_rgfmp[ifmp].FInUse() );

            cFmpsEvaluated++;
            if ( fPreserveUnattachedCaches && !g_rgfmp[ifmp].DataHeaderSignature().FNull() )
            {
                cPreservedCaches++;
                continue;
            }
            goto SelectedIfmp;
        }

        Assert( cFmpsEvaluated == ( s_ifmpMacCommitted - cfmpReserved ) );

#ifdef DEBUG
        const ULONG cMinimumPreservedCaches = ( ( rand() % 2 ) == 0 ) ? 2 : g_ifmpMax;
#else
        const ULONG cMinimumPreservedCaches = FJetConfigLowMemory() ? 2 :
                                                FJetConfigMedMemory() ? 5 :
                                                    g_ifmpMax;
#endif

        if ( fPreserveUnattachedCaches &&
                cPreservedCaches <= cMinimumPreservedCaches &&
                cFmpsEvaluated < ( g_ifmpMax - 1 ) )
        {
            break;
        }
        if ( !fPreserveUnattachedCaches )
        {
            OnDebug( fDidCanabalizeUnattachedCachesPass = fTrue );
        }
    }


    if ( ifmp >= g_ifmpMax )
    {
        ;
        Assert( fDidCanabalizeUnattachedCachesPass );
        err = ErrERRCheck( JET_errTooManyAttachedDatabases );
    }

    Assert( ifmp > IfmpMacInUse() );

    if ( !FAllocatedFmp( ifmp ) )
    {
        Call( ErrNewOneFmp( ifmp ) );
        Assert( IsFMPUsable( ifmp, fTrue ) );
    }


SelectedIfmp:

    Assert( ifmp < g_ifmpMax );
    Assert( FMP::FAllocatedFmp( ifmp ) );


    if ( !g_fDisablePerfmon && !PLS::FEnsurePerfCounterBuffer( ( ifmp + 1 ) * g_cbPlsMemRequiredPerPerfInstance ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    s_ifmpMinInUse = min( s_ifmpMinInUse, ifmp );
    s_ifmpMacInUse = max( s_ifmpMacInUse, ifmp );

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
        fPurgeFMP = fTrue;
        g_rgfmp[ifmp].FreeHeaderSignature();
        Assert( g_rgfmp[ifmp].DataHeaderSignature().FNull() );
    }

    if ( !fFoundMatchingFmp )
    {
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

    if ( fPurgeFMP )
    {
        BFPurge( ifmp );
    }

    return err;
}

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
    pfmp->SetILgposWaypoint( lgposMin );
    pfmp->SetLGenMaxCommittedAttachedDuringRecovery( 0 );
    pfmp->SetPfapi( NULL );
    pfmp->m_cbOwnedFileSize = 0;
    pfmp->AcquireIOSizeChangeLatch();
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
    pfmp->ResetLeakReclaimerIsRunning();
    pfmp->ResetPgnoMaxTracking();

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

    
    Assert( !FExclusiveOpen() || PpibExclusiveOpen() == ppib );
    IFMP ifmp = PinstFromPpib( ppib )->m_mpdbidifmp[ Dbid() ];
    FMP::AssertVALIDIFMP( ifmp );

    OSMemoryHeapFree( WszDatabaseName() );
    SetWszDatabaseName( NULL );

    
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

        if ( ifmpT > IfmpMacInUse() )
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
        Assert( ifmpT < s_ifmpMacInUse );
        s_ifmpMacInUse = ifmpT;
    }

    
    ResetWriteLatch( ppib );
    if ( GateWriteLatch().CWait() > 0 )
    {
        GateWriteLatch().ReleaseAll( CritLatch() );
    }
    else
    {
        CritLatch().Leave();
    }

    PERFSetDatabaseNames( ppib->m_pinst->m_pfsapi );
}


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
    
    const IFMP ifmp = IFMP( pfmp - g_rgfmp );
    Assert( &( g_rgfmp[ ifmp ] ) == pfmp );
    return FMP::FAllocatedFmp( ifmp );
}

ERR FMP::ErrNewOneFmp( const IFMP ifmp )
{
    ERR err = JET_errSuccess;

    Assert( FWriterFMPPool() );
    Assert( g_rgfmp != NULL );
    Assert( ifmp >= cfmpReserved  );
    Assert( ifmp < g_ifmpMax );

    Assert( s_ifmpMacCommitted == ifmp );
    Assert( !FAllocatedFmp( ifmp ) );
    Assert( FMP::FAllocatedFmp( ifmp - 1 ) );


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


    BYTE * pbBase = (BYTE*)( g_rgfmp + ( ( ifmp == 1 ) ? 0 : ifmp ) );
    BYTE * pbHigh = (BYTE*)( g_rgfmp + ifmp ) + sizeof(FMP);
    BYTE * pbHighPrev = pbBase - 1;

    const BOOL fInNewPage = roundup( (DWORD_PTR)pbHighPrev, OSMemoryPageCommitGranularity() )
                                != roundup( (DWORD_PTR)pbHigh, OSMemoryPageCommitGranularity() );

    Assert( OSMemoryPageCommitGranularity() >= 4096 );
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
        OnDebug( AtomicExchangeAdd( (LONG*)( pbHigh - sizeof(LONG) ), 0 ) );
    }


    s_ifmpMacCommitted = ifmp + 1;

    Assert( FAllocatedFmp( ifmp ) );
    Assert( FAllocatedFmp( &( g_rgfmp[ ifmp ] ) ) );


    FMP* pfmpT = new(&g_rgfmp[ ifmp ]) FMP;

    Assert( pfmpT == &g_rgfmp[ ifmp ] );

    Assert( g_rgfmp[ ifmp ].m_rgprangelock[ 0 ] == NULL );
    Assert( g_rgfmp[ ifmp ].m_rgprangelock[ 1 ] == NULL );


    Assert( pfmpT->m_dbid == dbidMax );

#ifdef DEBUG
    g_rgfmp[ ifmp ].SetDatabaseSizeMax( 0xFFFFFFFF );
#endif


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


    Assert( s_ifmpMacCommitted > ifmp );
    Assert( FAllocatedFmp( ifmp ) );
    Assert( FAllocatedFmp( &( g_rgfmp[ ifmp ] ) ) );

    return JET_errSuccess;

HandleError:
    Assert( err < JET_errSuccess );

    Assert( s_ifmpMacCommitted == ifmp );
    Assert( !FAllocatedFmp( ifmp ) );

    OSMemoryHeapFree( prangelockPrealloc0 );
    OSMemoryHeapFree( prangelockPrealloc1 );
    delete piostatsDbRead;
    delete piostatsDbWrite;

    return err;
}

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
    const size_t cbShiftedPages = CbFMPRgfmpShift();
    Assert( cfmpReserved * sizeof(FMP) < OSMemoryPageCommitGranularity() );

    BYTE * pbFmpBuffer = (BYTE*)PvOSMemoryPageReserve( cbFmpArrayPages + cbShiftedPages, NULL );
    AllocR( pbFmpBuffer );

    g_rgfmp = (FMP*)( &( pbFmpBuffer[ cbShiftedPages - ( cfmpReserved * sizeof(FMP) ) ] ) );
    Assert( (BYTE*)g_rgfmp >= pbFmpBuffer );

    Assert( ( ((BYTE*)g_rgfmp) + ( cfmpReserved * sizeof( g_rgfmp[ 0 ] ) ) - CbFMPRgfmpShift() ) == pbFmpBuffer );
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

    

    EnterFMPPoolAsWriter();

    Assert( NULL == g_rgfmp );
    Assert( g_ifmpMax > 0 );

    Call( ErrFMPReserveShiftedRgfmp() );

    s_ifmpMinInUse = g_ifmpMax;
    s_ifmpMacInUse = 0;

    s_ifmpMacCommitted = cfmpReserved;

    LeaveFMPPoolAsWriter();

    return JET_errSuccess;

HandleError:
    Assert( g_rgfmp == NULL );
    if ( g_rgfmp )
    {
        FMPUnreserveFreeShiftedRgfmp();
    }
    Assert( g_rgfmp == NULL );

    LeaveFMPPoolAsWriter();

    return err;
}


VOID FMP::Term( )
{

    EnterFMPPoolAsWriter();

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

        pfmp->~FMP();
    }


    

    if ( s_ifmpMacCommitted > cfmpReserved )
    {
        FMPDecommitShiftedRgfmp( s_ifmpMacCommitted );
        s_ifmpMinInUse = g_ifmpMax;
        s_ifmpMacInUse = 0;
        s_ifmpMacCommitted = 0;
    }

    FMPUnreserveFreeShiftedRgfmp();
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

    Assert( pgnoStart <= pgnoEnd );
    Assert( m_semRangeLock.CAvail() == 0 );


    RANGELOCK** pprangelockCur = &m_rgprangelock[ m_msRangeLock.GroupActive() ];
    RANGELOCK** pprangelockNew = &m_rgprangelock[ m_msRangeLock.GroupInactive() ];


    if ( (*pprangelockNew)->crangeMax < (*pprangelockCur)->crange + 1 )
    {

        SIZE_T              crangeMax   = 2 * (*pprangelockNew)->crangeMax;
        SIZE_T              cbrangelock = sizeof( RANGELOCK ) + crangeMax * sizeof( RANGE );

        RANGELOCK * const   prangelock  = (RANGELOCK*)PvOSMemoryHeapAlloc( cbrangelock + 0x80);

        if ( !prangelock )
        {
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }

        prangelock->crangeMax = crangeMax;

        OSMemoryHeapFree( *pprangelockNew );

        *pprangelockNew = prangelock;
    }


    SIZE_T irange;
    for ( irange = 0; irange < (*pprangelockCur)->crange; irange++ )
    {
        (*pprangelockNew)->rgrange[ irange ].pgnoStart  = (*pprangelockCur)->rgrange[ irange ].pgnoStart;
        (*pprangelockNew)->rgrange[ irange ].pgnoEnd    = (*pprangelockCur)->rgrange[ irange ].pgnoEnd;
    }


    (*pprangelockNew)->rgrange[ irange ].pgnoStart  = pgnoStart;
    (*pprangelockNew)->rgrange[ irange ].pgnoEnd    = pgnoEnd;


    (*pprangelockNew)->crange = (*pprangelockCur)->crange + 1;


    m_msRangeLock.Partition();

HandleError:
    Assert( m_semRangeLock.CAvail() == 0 );
    return err;
}

VOID FMP::IRemoveRangeLock( const PGNO pgnoStart, const PGNO pgnoEnd )
{
    Assert( m_semRangeLock.CAvail() == 0 );


    RANGELOCK** pprangelockCur = &m_rgprangelock[ m_msRangeLock.GroupActive() ];
    RANGELOCK** pprangelockNew = &m_rgprangelock[ m_msRangeLock.GroupInactive() ];


    Assert( (*pprangelockNew)->crangeMax >= (*pprangelockCur)->crange - 1 );


    bool fRemovedRange = false;
    SIZE_T irangeSrc;
    SIZE_T irangeDest;
    for ( irangeSrc = 0, irangeDest = 0; irangeSrc < (*pprangelockCur)->crange; irangeSrc++ )
    {
        if (    fRemovedRange ||
                (*pprangelockCur)->rgrange[ irangeSrc ].pgnoStart != pgnoStart ||
                (*pprangelockCur)->rgrange[ irangeSrc ].pgnoEnd != pgnoEnd )
        {
            (*pprangelockNew)->rgrange[ irangeDest ].pgnoStart  = (*pprangelockCur)->rgrange[ irangeSrc ].pgnoStart;
            (*pprangelockNew)->rgrange[ irangeDest ].pgnoEnd    = (*pprangelockCur)->rgrange[ irangeSrc ].pgnoEnd;

            irangeDest++;
        }
        else
        {
            fRemovedRange = true;
        }
    }


    Assert( fRemovedRange );
    Assert( irangeDest == irangeSrc - 1 );


    (*pprangelockNew)->crange = irangeDest;


    m_msRangeLock.Partition();

    Assert( m_semRangeLock.CAvail() == 0 );
}


ERR FMP::ErrRangeLock( const PGNO pgnoStart, const PGNO pgnoEnd )
{
    ERR err = JET_errSuccess;


    m_semRangeLock.Acquire();

    Call( ErrIAddRangeLock( pgnoStart, pgnoEnd ) );


HandleError:
    m_semRangeLock.Release();
    return err;
}


VOID FMP::RangeUnlock( const PGNO pgnoStart, const PGNO pgnoEnd )
{
    Assert( pgnoStart <= pgnoEnd );


    m_semRangeLock.Acquire();

    IRemoveRangeLock( pgnoStart, pgnoEnd );


    m_semRangeLock.Release();
}


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


    const RANGELOCK* const prangelock = m_rgprangelock[ irangelock ];


    for ( SIZE_T irange = 0; irange < prangelock->crange; irange++ )
    {

        if (    prangelock->rgrange[ irange ].pgnoStart <= pgno &&
                prangelock->rgrange[ irange ].pgnoEnd >= pgno )
        {

            m_msRangeLock.Leave( irangelock );
            return fFalse;
        }
    }


    return fTrue;
}


VOID FMP::LeaveRangeLock( const INT irangelock )
{
    m_msRangeLock.Leave( irangelock );
}


VOID FMP::LeaveRangeLock( const PGNO pgnoDebugOnly, const INT irangelock )
{
#ifdef DEBUG
    Assert( irangelock == 0 || irangelock == 1 );

    const RANGELOCK* const prangelock = m_rgprangelock[ irangelock ];

    for ( SIZE_T irange = 0; irange < prangelock->crange; irange++ )
    {
        Assert( pgnoDebugOnly < prangelock->rgrange[ irange ].pgnoStart ||
                pgnoDebugOnly > prangelock->rgrange[ irange ].pgnoEnd );
    }
#endif

    LeaveRangeLock( irangelock );
}


ERR FMP::ErrRangeLockAndEnter( const PGNO pgnoStart, const PGNO pgnoEnd, CMeteredSection::Group* const pirangelock )
{
    ERR err = JET_errSuccess;
    BOOL fRangeAdded = fFalse;
    BOOL fEnteredRange = fFalse;

    Assert( pgnoStart <= pgnoEnd );


    m_semRangeLock.Acquire();

    const RANGELOCK* const prangelock = m_rgprangelock[ m_msRangeLock.GroupActive() ];


    for ( SIZE_T irange = 0; irange < prangelock->crange; irange++ )
    {

        if (    prangelock->rgrange[ irange ].pgnoStart <= pgnoEnd &&
                prangelock->rgrange[ irange ].pgnoEnd >= pgnoStart )
        {

            Error( ErrERRCheck( JET_errTooManyActiveUsers ) );
        }
    }


    Call( ErrIAddRangeLock( pgnoStart, pgnoEnd ) );
    fRangeAdded = fTrue;


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
    LeaveRangeLock( irangelock );
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
    Assert( m_pinst->FRecovering() && ( m_pinst->m_plog->FRecoveringMode() == fRecoveringRedo ) );

    ERR err = JET_errSuccess;

    if ( ( m_pLogRedoMapZeroed != NULL ) && ( m_pLogRedoMapBadDbtime != NULL ) )
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

HandleError:
    Assert( m_sxwlRedoMaps.FOwnWriteLatch() );

    if ( err < JET_errSuccess )
    {
        FreeLogRedoMaps( fTrue  );
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

    Assert( fAllocCleanup || ( ( m_pLogRedoMapZeroed == NULL ) == ( m_pLogRedoMapBadDbtime == NULL ) ) );

    if ( ( m_pLogRedoMapZeroed == NULL ) && ( m_pLogRedoMapBadDbtime == NULL ) )
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

    const BOOL fRedoMapsEmpty = ( ( m_pLogRedoMapZeroed == NULL ) || ( !m_pLogRedoMapZeroed->FAnyPgnoSet() ) ) &&
                                 ( ( m_pLogRedoMapBadDbtime == NULL ) || ( !m_pLogRedoMapBadDbtime->FAnyPgnoSet() ) );

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

ERR FMP::ErrSetPdbfilehdr( DBFILEHDR_FIX * pdbfilehdr, __out DBFILEHDR ** ppdbfilehdr )
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
    const DBTIME    dbtimeLast  = DbtimeGet();
    TICK            tickOldestTransaction = TickOSTimeCurrent();

    Assert( FReaderFMPPool() );


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

    PERFOpt( lOldestTransaction.Set( m_pinst, DtickDelta( tickOldestTransaction, TickOSTimeCurrent() ) ) );

    const TRX       trxOldestCandidate  = TrxOldestCandidate();
    const TRX       trxOldestTarget     = TrxOldestTarget();

    Assert( trxMax == trxOldestTarget
        || TrxCmp( trxOldestCandidate, trxOldestTarget ) <= 0 );

    if ( trxMax == trxOldestTarget )
    {
        if ( TrxCmp( trxOldestCandidate, trxOldest ) < 0 )
        {
            SetDbtimeOldestTarget( dbtimeLast );
            SetTrxOldestTarget( trxNewest );
        }
    }
    else if ( TrxCmp( trxOldestTarget, trxOldest ) < 0 )
    {
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

    (*pobjid)++;

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

    Assert( FWriterFMPPool() ||
            ( FReaderFMPPool() && m_pinst->m_plog->FOwnerCheckpoint() ) );
    
    Assert( ( lgposWaypoint.isec == lgposMax.isec && lgposWaypoint.ib == lgposMax.ib ) ||
            ( CmpLgpos( &lgposWaypoint, &lgposMin ) == 0 ) );
    Assert( ( m_lgposWaypoint.isec == lgposMax.isec && m_lgposWaypoint.ib == lgposMax.ib ) ||
            ( CmpLgpos( (LGPOS*)&m_lgposWaypoint, &lgposMin ) == 0 ) );

    Assert( CmpLgpos( &lgposWaypoint, &lgposMax ) != 0 );

    if ( CmpLgpos( &lgposWaypoint, &lgposMin) != 0  )
    {
        const IFMP ifmp = Ifmp();

    
        DBEnforce( ifmp, CmpLgpos( &lgposWaypoint, (LGPOS*)&m_lgposWaypoint ) >= 0 );

    {
        PdbfilehdrReadOnly pdbfilehdr = Pdbfilehdr();
        if ( pdbfilehdr )
        {
            DBEnforce( ifmp, pdbfilehdr->le_lGenMaxRequired >= lgposWaypoint.lGeneration );
        }
    }
    }

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

ERR FMP::ErrStartDBMScan()
{
    ERR err = JET_errSuccess;
    const IFMP ifmp = Ifmp();

    SemDBM().Acquire();

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

    if ( err < JET_errSuccess && JET_errDatabaseAlreadyRunningMaintenance != err )
    {
        StopDBMScan();
    }
    return err;
}

ERR FMP::ErrStartDBMScanSinglePass(
    JET_SESID sesid,
    const INT csecMax,
    const INT cmsecSleep,
    const JET_CALLBACK pfnCallback)
{
    ERR err = JET_errSuccess;
    const IFMP ifmp = Ifmp();

    SemDBM().Acquire();

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

    if (err < 0 && JET_errDatabaseAlreadyRunningMaintenance != err )
    {
        StopDBMScan();
    }
    return err;
}

VOID FMP::StopDBMScan()
{
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

BOOL FMP::FDBMScanOn()
{
    return ( NULL != m_pdbmScan );
}

ERR FMP::ErrCreateDBMScanFollower()
{
    Assert( m_pdbmFollower == NULL );

    if ( m_pdbmFollower == NULL )
    {
        ERR err = JET_errSuccess;
        AllocR( m_pdbmFollower = new CDBMScanFollower() );
        Assert( PdbmFollower() != NULL );
    }

    return JET_errSuccess;
}

VOID FMP::DestroyDBMScanFollower()
{
    Expected( m_pdbmFollower != NULL );
    if ( m_pdbmFollower )
    {
        Assert( m_pdbmFollower->FStopped() );
        delete m_pdbmFollower;
        m_pdbmFollower = NULL;
    }
}

CDBMScanFollower * FMP::PdbmFollower() const
{
    return m_pdbmFollower;
}

VOID FMP::IncrementAsyncIOForViewCache()
{
    Assert( BoolParam( JET_paramEnableViewCache ) );

    const LONG cAsyncIOForViewCachePending = AtomicIncrement( (LONG*)&m_cAsyncIOForViewCachePending );
    Assert( cAsyncIOForViewCachePending > 0 );

    Assert( cAsyncIOForViewCachePending <= 30  + (LONG)(UlParam( JET_paramOutstandingIOMax ) * UlParam(JET_paramMaxCoalesceReadSize) / g_cbPage));
}

VOID FMP::DecrementAsyncIOForViewCache()
{
    Assert( BoolParam( JET_paramEnableViewCache ) );

    const LONG cAsyncIOForViewCachePending = AtomicDecrement( (LONG*)&m_cAsyncIOForViewCachePending );
    Assert( cAsyncIOForViewCachePending >= 0 );

    Assert( cAsyncIOForViewCachePending < 30  + (LONG)(UlParam( JET_paramOutstandingIOMax ) * UlParam(JET_paramMaxCoalesceReadSize) / g_cbPage));
}

VOID FMP::WaitForAsyncIOForViewCache()
{
    if ( m_cAsyncIOForViewCachePending > 0 )
    {
        FireWall( "AsyncIoForViewCachePending" );
    }


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

ERR FMP::ErrPgnoLastFileSystem( PGNO* const ppgnoLast ) const
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

#ifdef ENABLE_JET_UNIT_TEST

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

#endif

