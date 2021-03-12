// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_logredomap.hxx"
#include "errdata.hxx"

//
// this function will return one of the followings:
//      JET_errDatabase200Format
//      JET_errDatabase400Format
//      JET_errDatabaseCorrupted
//

LOCAL ERR ErrDBICheck200And400( INST *const pinst, IFileSystemAPI *const pfsapi, __in PCWSTR wszDatabaseName )
{
    /* persistent database data, in database root node
    /**/
#include <pshpack1.h>

    //  Structures copied from JET200/JET400

    typedef ULONG SRID;

    typedef struct _database_data
    {
        ULONG   ulMagic;
        ULONG   ulVersion;
        ULONG   ulDBTime;
        USHORT  usFlags;
    } P_DATABASE_DATA;

    typedef BYTE    PGTYP;

    typedef struct _threebytes { BYTE b[3]; } THREEBYTES;

    typedef struct _pghdr
    {
        ULONG       ulChecksum;         //  checksum of page, always 1st byte
        ULONG       ulDBTime;           //  database time page dirtied
        PGNO        pgnoFDP;            //  pgno of FDP which owns this page
        SHORT       cbFree;             //  count free bytes
        SHORT       ibMic;              //  minimum used byte
        SHORT       ctagMac;            //  count tags used
        SHORT       itagFree;           //  itag of first free tag
        SHORT       cbUncommittedFreed; //  bytes freed from this page, but *possibly*
                                        //    uncommitted (this number will always be
                                        //    a subset of cbFree)
        THREEBYTES  pgnoPrev;           //  pgno of previous page
        THREEBYTES  pgnoNext;           //  pgno of next page
    } PGHDR;

    typedef struct _pgtrlr
    {
        PGTYP       pgtyp;
        THREEBYTES  pgnoThisPage;
    } PGTRLR;

    typedef struct _tag
    {
        SHORT       cb;
        SHORT       ib;
    } TAG;

    typedef struct _page
    {
        PGHDR       pghdr;
        TAG         rgtag[1];
        BYTE        rgbFiller[ cbPageOld -
                        sizeof(PGHDR) -         // pghdr
                        sizeof(TAG) -           // rgtag[1]
                        sizeof(BYTE) -          // rgbData[1]
                        sizeof(PGTYP) -         // pgtyp
                        sizeof(THREEBYTES) ];   // pgnoThisPage
        BYTE        rgbData[1];
        PGTYP       pgtyp;
        THREEBYTES  pgnoThisPage;
    } PAGE;

#include <poppack.h>

#define IbCbFromPtag( ibP, cbP, ptagP )                         \
        {   TAG *_ptagT = ptagP;                                \
            (ibP) = _ptagT->ib;                                 \
            (cbP) = _ptagT->cb;                                 \
        }

    //  node header bits
#define fNDVersion              0x80
#define fNDDeleted              0x40
#define fNDBackLink             0x20
#define fNDFDPPtr               0x10
#define fNDSon                  0x08
#define fNDVisibleSon           0x04
#define fNDFirstItem            0x02
#define fNDLastItem             0x01

#define FNDVisibleSons(b)       ( (b) & fNDVisibleSon )
#define FNDInvisibleSons(b)     ( !( (b) & fNDVisibleSon ) )
#define CbNDKey(pb)             ( *( (pb) + 1 ) )
#define FNDNullSon(b)           ( !( (b) & fNDSon ) )
#define PbNDSonTable(pb)            ( (pb) + 1 + 1 + *(pb + 1) )
#define PbNDBackLink(pb)                                            \
    ( PbNDSonTable(pb) + ( FNDNullSon( *(pb) ) ? 0 :                \
    ( ( ( *PbNDSonTable(pb) == 1 ) && FNDInvisibleSons( *(pb) ) ) ? \
    sizeof(PGNO) + 1 : *PbNDSonTable(pb) + 1 ) ) )
#define FNDBackLink(b)          ( (b) & fNDBackLink )
#define PbNDData(pb) ( PbNDBackLink(pb) + ( FNDBackLink( *(pb) ) ? sizeof(SRID) : 0 ) )

    ERR             err = JET_errSuccess;
    PAGE            *ppage;
    INT             ibTag;
    INT             cbTag;
    BYTE            *pb;
    ULONG           ulVersion;
    IFileAPI        *pfapi;
    TraceContextScope tcScope( iorpHeader );
    tcScope->SetDwEngineObjid( objidSystemRoot );

    CallR( CIOFilePerf::ErrFileOpen(
                            pfsapi,
                            pinst,
                            wszDatabaseName,
                            IFileAPI::fmfReadOnly,
                            iofileDbAttached,
                            qwLegacyFileID,
                            &pfapi ) );

    Alloc( ppage = (PAGE *)PvOSMemoryPageAlloc( cbPageOld, NULL ) );
    Call( pfapi->ErrIORead( *tcScope, QWORD( 0 ), cbPageOld, (BYTE* const)ppage, qosIONormal ) );

    IbCbFromPtag(ibTag, cbTag, &ppage->rgtag[0]);
    if ( ibTag < 0 ||
        cbTag < 0 ||
        ibTag < (BYTE*)&ppage->rgtag[1] - (BYTE*)ppage ||
        (BYTE*)ppage + ibTag + cbTag >= (BYTE*)(ppage + 1) )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    /*  at least FILES, OWNEXT, AVAILEXT
    /**/
    pb = (BYTE *)ppage + ibTag;
    if ( !FNDVisibleSons( *pb ) || CbNDKey( pb ) != 0 || FNDNullSon( *pb ) )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    /*  check data length
    /**/
    INT cb;
    cb = (INT)( cbTag - ( PbNDData( pb ) - pb ) );
    if ( cb != sizeof(P_DATABASE_DATA) )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    /*  check database version, for 200 ulVersion should be 1, for 400 ulVersion is 0x400.
    /**/
    ulVersion = ((P_DATABASE_DATA *)PbNDData(pb))->ulVersion;
    if ( ulDAEVersion200 == ulVersion )
        err = ErrERRCheck( JET_errDatabase200Format );
    else if ( ulDAEVersion400 == ulVersion )
        err = ErrERRCheck( JET_errDatabase400Format );
    else
        err = ErrERRCheck( JET_errDatabaseCorrupted );

HandleError:
    if ( ppage != NULL )
        OSMemoryPageFree( (VOID *)ppage );

    delete pfapi;
    return err;
}

#ifdef DEBUG
BOOL FInEseutilPossibleUsageError()
{
#ifdef ESENT
    return 0 == _wcsicmp( WszUtilProcessFileName(), L"esentutl.exe" );
#else
    return 0 == _wcsicmp( WszUtilProcessFileName(), L"eseutil.exe" );
#endif
}
#endif


//  ================================================================
bool FDBIsLVChunkSizeCompatible(
    const ULONG cbPage,
    const ULONG ulDaeVersion,
    const ULONG ulDaeUpdateMajor )
//  ================================================================
//
//  In version 0x620,0x11 of the database the long-value chunk size
//  changed, but only for large pages (16kb and 32kb pages). The
//  change is _not_ backwards compatible so we have to check for
//  the case where we are attaching a database that is older than
//  the update.
//
//-
{
    // when the major version changes this function won't be needed
    // any more.
    Assert( ulDAEVersionMax == ulDAEVersionNewLVChunk );
    Assert( ulDAEUpdateMajorMax >= ulDAEUpdateMajorNewLVChunk );

    Assert( 2*1024 == cbPage
            || 4*1024 == cbPage
            || 8*1024 == cbPage
            || 16*1024 == cbPage
            || 32*1024 == cbPage );

    // earlier checks should catch these conditions

    Assert( ulDAEVersionMax == ulDaeVersion || FNegTest( fInvalidAPIUsage ) || FInEseutilPossibleUsageError() );

    if( FIsSmallPage( cbPage ) )
    {
        // small pages are always compatible
        return true;
    }

    // this is a large page database. if it has the
    // old chunk size then it isn't compatible
    if( ulDaeVersion == ulDAEVersionNewLVChunk
        && ulDaeUpdateMajor < ulDAEUpdateMajorNewLVChunk )
    {
        return false;
    }

    return true;
}

//  ================================================================
JETUNITTEST( DB, FDBIsLVChunkSizeCompatible  )
//  ================================================================
{
    for( INT i = 4; i <= 32; i *= 2 )
    {
        CHECK( true == FDBIsLVChunkSizeCompatible( i*1024, ulDAEVersionMax, ulDAEUpdateMajorMax ) );
        CHECK( true == FDBIsLVChunkSizeCompatible( i*1024, ulDAEVersionMax, ulDAEUpdateMajorNewLVChunk ) );
    }

    CHECK( true == FDBIsLVChunkSizeCompatible( 4*1024, ulDAEVersionNewLVChunk, ulDAEUpdateMajorNewLVChunk-1 ) );
    CHECK( true == FDBIsLVChunkSizeCompatible( 8*1024, ulDAEVersionNewLVChunk, ulDAEUpdateMajorNewLVChunk-1 ) );
    CHECK( false == FDBIsLVChunkSizeCompatible( 16*1024, ulDAEVersionNewLVChunk, ulDAEUpdateMajorNewLVChunk-1 ) );
    CHECK( false == FDBIsLVChunkSizeCompatible( 32*1024, ulDAEVersionNewLVChunk, ulDAEUpdateMajorNewLVChunk-1 ) );
}

//+local
//  ErrDBInitDatabase
//  ========================================================================
//  ErrDBInitDatabase( PIB *ppib, IFMP ifmp, CPG cpgPrimary )
//
//  Initializes database structure.  Structure is customized for
//  system, temporary and user databases which are identified by
//  the ifmp.  Primary extent is set to cpgPrimary but no allocation
//  is performed.  The effect of this routine can be entirely
//  represented with page operations.
//
//  PARAMETERS  ppib            ppib of database creator
//              ifmp            ifmp of created database
//              cpgPrimary      number of pages to show in primary extent
//
//  RETURNS     JET_errSuccess or error returned from called routine.
//-
LOCAL ERR ErrDBInitDatabase( PIB *ppib, IFMP ifmp, CPG cpgPrimary )
{
    ERR             err;
    OBJID           objidFDP;
    CPG             cpgOEFDP;
    CPG             cpgAEFDP;

    CallR( ErrDIRBeginTransaction( ppib, 40229, NO_GRBIT ) );

    //  set up root page
    //
    Call( ErrSPCreate(
                ppib,
                ifmp,
                pgnoNull,
                pgnoSystemRoot,
                cpgPrimary,
                fSPMultipleExtent,
                (ULONG)CPAGE::fPagePrimary,
                &objidFDP,
                &cpgOEFDP,
                &cpgAEFDP ) );

    Assert( objidSystemRoot == objidFDP );

    CATSetExtentPageCounts(
        ppib,
        ifmp,
        objidSystemRoot,
        cpgOEFDP,
        cpgAEFDP );

    Call( ErrDIRCommitTransaction( ppib, 0 ) );

    return err;

HandleError:
    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

    return err;
}

//  we may want to keep track of last page of the database.

ERR ErrDBSetLastPage( PIB* const ppib, const IFMP ifmp )
{
    ERR     err;
    PGNO    pgnoLastOwned = 0;
    QWORD   cbFsFileSize  = 0;
    IFMP    ifmpT;
    BOOL    fDbOpened     = fFalse;

    ppib->SetFSetAttachDB();

    Assert( !PinstFromPpib( ppib )->m_plog->FRecovering() ||
                fRecoveringRedo != PinstFromPpib( ppib )->m_plog->FRecoveringMode() ||
                g_rgfmp[ ifmp ].m_fCreatingDB ||
                // Note: We CAN NOT allow ErrDBSetLastPage() during recovery 
                // of a patched database, because the database is not completely 
                // consistent yet.
                g_rgfmp[ ifmp ].Pdbfilehdr()->Dbstate() != JET_dbstateDirtyAndPatchedShutdown );

    Call( ErrDBOpenDatabase( ppib, g_rgfmp[ifmp].WszDatabaseName(), &ifmpT, NO_GRBIT ) );
    Assert( ifmpT == ifmp );
    fDbOpened = fTrue;

    Assert( ppib->FSetAttachDB() );
    Call( g_rgfmp[ifmp].Pfapi()->ErrSize( &cbFsFileSize, IFileAPI::filesizeLogical ) );
    Call( ErrBTGetLastPgno( ppib, ifmp, &pgnoLastOwned ) );
    g_rgfmp[ifmp].SetOwnedFileSize( CbFileSizeOfPgnoLast( pgnoLastOwned ) );
    g_rgfmp[ifmp].AcquireIOSizeChangeLatch();
    g_rgfmp[ifmp].SetFsFileSizeAsyncTarget( cbFsFileSize );
    g_rgfmp[ifmp].ReleaseIOSizeChangeLatch();

HandleError:
    if ( fDbOpened )
    {
        CallS( ErrDBCloseDatabase( ppib, ifmpT, NO_GRBIT ) );
    }

    ppib->ResetFSetAttachDB();
    return err;
}

//  Reports the basic event for Create, Attach, and Detach database.
//
//  fCacheAlive 
//      - on attach indicates if we hooked up with an already existing cache for this DB.
//      - on detach indicates if we managed to save / keep our cache for this DB.
//      - on create this flag means nothing.

VOID DBReportTachmentEvent( const INST * const pinst, const IFMP ifmp, const MessageId msgidTachment, PCWSTR wszDatabaseName, const BOOL fCacheAlive = fFalse, const BOOL fDirtyCacheAlive = fFalse )
{
    FMP * pfmp = &g_rgfmp[ifmp];

    Assert( pinst );
    Assert( pfmp );
    Assert( msgidTachment );
    Assert( wszDatabaseName );

    if ( pfmp->FIsTempDB() )
    {
        //  we don't log the temp DB attach/detach
        return;
    }

    // report our successful {create|attach|detach} and timings

    WCHAR wszSeconds[16];
    WCHAR wszFmpId[16];
    WCHAR wszKeepCacheAlive[24];
    WCHAR wszAddlFixedData[250];

    __int64 usecTach = 0;
    ULONG cbTimingResourceDataSequence = 0;
    WCHAR * wszTimingResourceDataSequence = NULL;
    CIsamSequenceDiagLog * pisdl = NULL;
    switch( msgidTachment )
    {
        case CREATE_DATABASE_DONE_ID:
            pisdl = &pfmp->m_isdlCreate;
            Assert( pisdl->EFinalStep() == eCreateDone );
            break;
        case ATTACH_DATABASE_DONE_ID:
            pisdl = &pfmp->m_isdlAttach;
            Assert( pisdl->EFinalStep() == eAttachDone );
            break;
        case DETACH_DATABASE_DONE_ID:
            pisdl = &pfmp->m_isdlDetach;
            Assert( pisdl->EFinalStep() == eDetachDone );
            break;
        default:
            AssertSz( fFalse, "Unknown msgid = %d for *tachment event!", msgidTachment );
    }
    if ( pisdl )
    {
        usecTach = pisdl->UsecTimer( eSequenceStart, pisdl->EFinalStep() );
        cbTimingResourceDataSequence = pisdl->CbSprintTimings();
        wszTimingResourceDataSequence = (WCHAR *)_alloca( cbTimingResourceDataSequence );
        pisdl->SprintTimings( wszTimingResourceDataSequence, cbTimingResourceDataSequence );
        pisdl->SprintFixedData( wszAddlFixedData, sizeof(wszAddlFixedData) );
        pisdl->TermSequence();
    }

    if ( msgidTachment == CREATE_DATABASE_DONE_ID || msgidTachment == ATTACH_DATABASE_DONE_ID )
    {
        INT ich = wcslen( wszAddlFixedData );
        OSStrCbFormatW( wszAddlFixedData + ich, _cbrg( wszAddlFixedData ) - ich * sizeof(WCHAR), L",\ndbv = " );
        ich = wcslen( wszAddlFixedData );
        FormatDbvEfvMapping( ifmp, wszAddlFixedData + ich, _cbrg( wszAddlFixedData ) - ich * sizeof(WCHAR) );
    }

    //  We might consider truncating the timing sequence data when usecTach < 100000 /* 100 ms */, or
    //  even skipping the whole event altogether.  Probably good to make this behavior based upon the
    //  event logging level.

    OSStrCbFormatW( wszSeconds, sizeof(wszSeconds), L"%I64d", usecTach / 1000000 /* convert to seconds */ );
    OSStrCbFormatW( wszFmpId, sizeof(wszFmpId), L"%d", (ULONG)ifmp );
    OSStrCbFormatW( wszKeepCacheAlive, sizeof(wszKeepCacheAlive), L"%d %d", fCacheAlive, fDirtyCacheAlive );

    const WCHAR * rgszT[6] = { wszFmpId, wszDatabaseName, wszSeconds, wszTimingResourceDataSequence, wszKeepCacheAlive, wszAddlFixedData };

    UtilReportEvent(
        eventInformation,
        GENERAL_CATEGORY,
        msgidTachment,
        _countof( rgszT ),
        rgszT,
        0,
        NULL,
        pinst );
}


// The original legacy value, removed from daeconst.hxx:
//const CPG cpgDatabaseMin              = 256;  //  File Size - cpgDBReserved (ie. true db min = cpgDatabaseMin+cpgDBReserved)
// we use this instead:
CPG         cpgLegacyDatabaseDefaultSize    = 256;

const CPG   cpgDatabaseApiMinReserved       = 32;   //  minimum for the current ESE schema (across page sizes).

//  This is the no lies minimal DB we can have with ESE's current base schema, and it
//  is not meant to be used except for Assert()s and in compact and recovery, because
//  at that minimum it would hold no user data ... so more of a jumping off point.
CPG CpgDBDatabaseMinMin()
{
    //  Because there is ~x4.5 KB of ESE catalog data in the base schema (and we don't
    //  seem to do a proper append split, or density on catalog is too low! :P), so we
    //  need extra pages for the Base ESE schema as you drop in page size to 4KB or less.
    const CPG   cpgDatabaseMinMin8K = 30;   //  True min is 29 pages, but wanted to match 32 pages w/ everything for 4KB and higher.
    const CPG   cpgDatabaseMinMin4K = 30;   //  This is one higher than 8KB b/c we need an extra page for shadow.
    const CPG   cpgDatabaseMinMin2K = 31;   //  This is one higher than 4KB b/c we need an extra extra page for shadow.

    CPG cpgRet = cpgDatabaseMinMin2K;
    if ( g_cbPage <= 2048 )
    {
        cpgRet = cpgDatabaseMinMin2K;
    }
    else if ( g_cbPage <= 4096 )
    {
        cpgRet = cpgDatabaseMinMin4K;
        Assert( cpgRet + cpgDBReserved <= cpgDatabaseApiMinReserved );
    }
    else
    {
        Assert( g_cbPage >= 8192 );
        cpgRet = cpgDatabaseMinMin8K;
        Assert( cpgRet + cpgDBReserved <= cpgDatabaseApiMinReserved );
    }

    Assert( cpgRet <= cpgDatabaseApiMinReserved );
    return cpgRet;
}

//  The initial size is dependent upon the ESE base schema + plus some rounding + the
//  user paramDbExtensionSize.
//
CPG CpgDBInitialUserDatabaseSize( const INST * const pinst )
{
    //  We establish the ESE base size off two things:
    //    A) CpgDBDatabaseMinMin() - this is the true min required, with ESE's min schema, 
    //          and no data pages reserved for the user.
    //    B) This 30 pages is just a very good number ... as it works out to 32 pages when 
    //          you include  DB header + shadow, so that the end of the schema is aligned
    //          to 128 KB (w/ 4 KB page size) or 256 KB (w/ 8 KB) etc.
    //  Having an aligned end of schema is good so that subsequent extent allocations will 
    //  be nicely aligned  for the first and second extensions of the DB.  The 3rd and later 
    //  extensions will be auto-fixed to align with the paramDbExtensionSize OR the average 
    //  space hints allocation size.
    //  Note: Currently CpgDBDatabaseMinMin is dominant/higher only for 2 KB page sized DBs.
    const CPG cpgEseBaseSize = max( CpgDBDatabaseMinMin(), 30 );

    //  We now use the user's DB extension size as the initial allocation of user data
    //  that will need to be inserted.

    const CPG cpgUserDataMin = ( FDefaultParam( pinst, JET_paramDbExtensionSize ) &&
                                        ( (CPG)UlParam( pinst, JET_paramDbExtensionSize ) == cpgLegacyDatabaseDefaultSize ) ) ?
                                    //  kind of cheat for default param to make it 256 "pages" w/ hdrs total, instead of 258.
                                    (CPG)( cpgLegacyDatabaseDefaultSize - cpgDBReserved - cpgEseBaseSize ) :
                                    UlFunctionalMin( (CPG)UlParam( pinst, JET_paramDbExtensionSize ),
                                            (CPG)( cpgLegacyDatabaseDefaultSize - cpgDBReserved - cpgEseBaseSize ) );

    //  Take the base ESE size and extend by the first desired user extension.

    return cpgEseBaseSize + cpgUserDataMin;
}

ERR ISAMAPI ErrIsamCreateDatabase(
    JET_SESID                   sesid,
    const WCHAR* const          wszDatabaseName,
    JET_DBID* const             pjifmp,
    const JET_SETDBPARAM* const rgsetdbparam,
    const ULONG                 csetdbparam,
    const JET_GRBIT             grbit )
{
    ERR         err;
    PIB         *ppib;
    IFMP        ifmp;

    COSTraceTrackErrors trackerrors( __FUNCTION__ );

    //  check parameters
    //
    Assert( sizeof(JET_SESID) == sizeof(PIB *) );
    ppib = (PIB *)sesid;
    CallR( ErrPIBCheck( ppib ) );

    const BOOL fSparseEnabledFile = !!( GrbitParam( PinstFromPpib( ppib ), JET_paramEnableShrinkDatabase ) & JET_bitShrinkDatabaseOn );

    OSTrace( JET_tracetagDatabases, OSFormat( "Session=[0x%p:0x%x] creating database %ws [sparse=%d]",
            ppib, ppib->trxBegin0, wszDatabaseName, fSparseEnabledFile ) );

    CallR( ErrDBCreateDatabase(
                ppib,
                NULL,
                wszDatabaseName,
                &ifmp,
                dbidMax,
                CpgDBInitialUserDatabaseSize( PinstFromPpib( ppib ) ),
                fSparseEnabledFile,
                NULL,
                rgsetdbparam,
                csetdbparam,
                grbit ) );

    *pjifmp = (JET_DBID)ifmp;

    return JET_errSuccess;
}

ERR ErrDBFormatFeatureEnabled_( const JET_ENGINEFORMATVERSION efvFormatFeature, const DbVersion& dbvCurrentFromFile );

//  Retrieves the FormatVersions structure representing the intentions of the client app
//  as specified by their JET_paramEngineFormatValues.  Translates special values (such as 
//  JET_efvUsePersistedFormat and JET_efvUseEngineDefault) to the closest discreet matching 
//  value.
//
ERR ErrDBGetDesiredVersion(
    _In_ const INST * const                 pinst,
    _In_ const DBFILEHDR_FIX * const        pdbfilehdr,
    _Out_ const FormatVersions ** const     ppfmtvers,
    _Out_ BOOL * const                      pfAllowPersistedFormat )
{
    ERR err = JET_errSuccess;

    JET_ENGINEFORMATVERSION efv = (JET_ENGINEFORMATVERSION)UlParam( pinst, JET_paramEngineFormatVersion );

    //  Compute and strip the soft compat bit

    const BOOL fAllowPersistedFormat = ( efv & JET_efvAllowHigherPersistedFormat ) == JET_efvAllowHigherPersistedFormat;
    if ( pfAllowPersistedFormat )
    {
        *pfAllowPersistedFormat = fAllowPersistedFormat;
    }
    if ( fAllowPersistedFormat )
    {
        efv = efv & ~JET_efvAllowHigherPersistedFormat;
    }

    //  Compute the desired DB version we want ...

    if ( efv == JET_efvUsePersistedFormat )
    {
        // Consider this subtle situation - If we have a new EFV with a DB UpdateMinor, but also a Log UpdateMajor change 
        // we will actually upgrade the DB for that, but NOT the log.  This is kind of correct actually, because the EFV 
        // param is not supposed to honored for just an UpdateMinor.  But when this is co-mingled with a Log UpdateMajor
        // this might catch people by surprise that we actually generate a set of files that have half the "updates" for
        // that particular EFV line in sysver.cxx!?  If we find this too confusing, we could as if FLogOn() and if so,
        // retrieve Call( ErrLGFindHighestMatchingLogMajors( LOG_STREAM.m_lgvHighestRecovered, &pfmtversLogDesired ) ) and
        // use the min of this EFV / pfmtversLogDesired and the ErrDBFindHighestMatchingDbMajors() pfmtvers retrieved below.
    
        if ( pdbfilehdr )
        {
            const FormatVersions * pfmtvers = NULL;
            CallR( ErrDBFindHighestMatchingDbMajors( pdbfilehdr->Dbv(), &pfmtvers, pinst ) );
            if ( err >= JET_errSuccess )
            {
                Assert( pdbfilehdr->le_ulVersion == pfmtvers->dbv.ulDbMajorVersion &&
                        pdbfilehdr->le_ulDaeUpdateMajor == pfmtvers->dbv.ulDbUpdateMajor ); // assert lookup did the job right
                efv = pfmtvers->efv;
            }

            CallR( ErrGetDesiredVersion( NULL, efv, ppfmtvers ) );
            Assert( (*ppfmtvers) != NULL );
            CallS( err );
            return err;
        }
        else
        {
            efv = JET_efvUseEngineDefault;
        }
    }

    CallR( ErrGetDesiredVersion( pinst, efv, ppfmtvers ) );
    Assert( (*ppfmtvers) != NULL );

    CallS( err );
    return err;
}

//  Validates the versions of the DBFILEHDR against both the DB maximum and the client apps intention
//  as expressed by dbvDesired (from their JET_paramEngineFormatValues setting).
//
//  ErrDBIValidateUserVersions() and ErrDBICheckVersion() are very similar to each other in that they 
//  both valdiate versions, but ErrDBICheckVersion() can be used in read only contexts by passing the
//  PfmtversEngineMax()->dbv to just check against what the engine understands.  For read-write contexts
//  the ErrDBIValidateUserVersions() must be used to check against JET_paramEnableFormatVersion and 
//  preserve the user specified version.
ERR ErrDBICheckVersions(
    _In_    const INST* const               pinst,
    _In_z_  const WCHAR* const              wszDbFullName,
    _In_    const IFMP                      ifmp,
    _In_    const DBFILEHDR_FIX * const     pdbfilehdr,
    _In_    const DbVersion&                dbvDesired,
    _In_    const BOOL                      fAllowPersistedFormat,
    _Out_   BOOL * const                    pfDbNeedsUpdate = NULL )
{
    ERR     err     = JET_errSuccess;

    DbVersion dbvEngineMax = PfmtversEngineMax()->dbv;
    
    //  First check DB is not greater than the engine itself (this would be a version we can't even hope to understand!).

    if ( dbvEngineMax.ulDbMajorVersion < pdbfilehdr->le_ulVersion ||
        dbvEngineMax.ulDbMajorVersion == pdbfilehdr->le_ulVersion && dbvEngineMax.ulDbUpdateMajor < pdbfilehdr->le_ulDaeUpdateMajor )
        //  Note: a too high le_ulDaeUpdateMinor is ok, as these are forward compatible changes.
    {
        WCHAR wszDatabaseVersion[50];
        WCHAR wszEngineVersion[50];
        const WCHAR * rgszT[3] = { wszDbFullName, wszDatabaseVersion, wszEngineVersion };
        
        OSStrCbFormatW( wszDatabaseVersion, _cbrg( wszDatabaseVersion ), L"%d.%d.%d", (ULONG)pdbfilehdr->le_ulVersion, (ULONG)pdbfilehdr->le_ulDaeUpdateMajor, (ULONG)pdbfilehdr->le_ulDaeUpdateMinor );
        OSStrCbFormatW( wszEngineVersion, _cbrg( wszEngineVersion ), L"%d.%d.%d", dbvEngineMax.ulDbMajorVersion, dbvEngineMax.ulDbUpdateMajor, dbvEngineMax.ulDbUpdateMinor );
        
        UtilReportEvent( eventError,
                GENERAL_CATEGORY,
                ATTACH_DATABASE_VERSION_TOO_HIGH_FOR_ENGINE_ID,
                _countof( rgszT ),
                rgszT,
                0,
                NULL,
                pinst );

        Assert( FNegTest( fInvalidAPIUsage ) || FInEseutilPossibleUsageError() );
        Call( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }

    //  Second check DB is not greater than the specified desired version (typically from the param).

    if ( dbvDesired.ulDbMajorVersion < pdbfilehdr->le_ulVersion ||
        dbvDesired.ulDbMajorVersion == pdbfilehdr->le_ulVersion && dbvDesired.ulDbUpdateMajor < pdbfilehdr->le_ulDaeUpdateMajor )
    {
        WCHAR wszDatabaseVersion[50];
        WCHAR wszParamVersion[50];
        WCHAR wszParamEfv[cchFormatEfvSetting];
        const WCHAR * rgszT[5] = { wszDbFullName, wszDatabaseVersion, wszParamVersion, wszParamEfv, L"" /* deprecated %8 */ };
        
        OSStrCbFormatW( wszDatabaseVersion, _cbrg( wszDatabaseVersion ), L"%d.%d.%d", (ULONG)pdbfilehdr->le_ulVersion, (ULONG)pdbfilehdr->le_ulDaeUpdateMajor, (ULONG)pdbfilehdr->le_ulDaeUpdateMinor );
        OSStrCbFormatW( wszParamVersion, _cbrg( wszParamVersion ), L"%d.%d.%d", dbvDesired.ulDbMajorVersion, dbvDesired.ulDbUpdateMajor, dbvDesired.ulDbUpdateMinor );
        FormatEfvSetting( (JET_ENGINEFORMATVERSION)UlParam( pinst, JET_paramEngineFormatVersion ), wszParamEfv, sizeof(wszParamEfv) );
        // deprecated: OSStrCbFormatW( wszAllowPersisted, _cbrg( wszAllowPersisted ), L"%d", fAllowPersistedFormat );

        if ( fAllowPersistedFormat )
        {
#ifdef ENABLE_MICROSOFT_MANAGED_DATACENTER_LEVEL_OPTICS
            //  Since the engine can understand this (got past first check AND the user 
            //  asked us to, we will let this through ...

            UtilReportEvent( eventWarning,
                    GENERAL_CATEGORY,
                    ATTACH_DATABASE_VERSION_TOO_HIGH_FOR_PARAM_ID,
                    _countof( rgszT ),
                    rgszT,
                    0,
                    NULL,
                    pinst );
#endif
        }
        else
        {
            //  User wants strict version compatibility, throw this one out.

            UtilReportEvent( eventError,    // NOTE: Same event, but here it is an error, above it is a warning!
                    GENERAL_CATEGORY,
                    ATTACH_DATABASE_VERSION_TOO_HIGH_FOR_PARAM_ID,
                    _countof( rgszT ),
                    rgszT,
                    0,
                    NULL,
                    pinst );
            Assert( FNegTest( fInvalidAPIUsage ) || FInEseutilPossibleUsageError() );
            Call( ErrERRCheck( JET_errEngineFormatVersionSpecifiedTooLowForDatabaseVersion ) );
        }
    }

    //  Calculate if the le_efvMaxBinAttachDiagnostic needs updating ...

    if ( pdbfilehdr->le_efvMaxBinAttachDiagnostic < JET_efvSetDbVersion &&
            //  This is not a normal usage of ErrDBFormatFeatureEnabled_() as typically code
            //  should be passing the current DB header format in as the 2nd arg.  Do not
            //  copy this example. ;)  We can't do that here b/c this is _whether to actual
            //  update the DB header iteself_, so the bootstrap has to check the desired / 
            //  param value instead.
            ErrDBFormatFeatureEnabled_( JET_efvSetDbVersion, dbvDesired ) >= JET_errSuccess )
    {
        if ( pfDbNeedsUpdate )
        {
            *pfDbNeedsUpdate = fTrue;
        }
    }

    //  Calculate if any of the main version numbers need updating ...

    if ( CmpDbVer( pdbfilehdr->Dbv(), dbvDesired ) < 0 )
    {
        if ( pfDbNeedsUpdate )
        {
            *pfDbNeedsUpdate = fTrue;
        }
    }

HandleError:

    if ( err < JET_errSuccess )
    {
        OSTrace( JET_tracetagUpgrade, OSFormat( "FAILED: ErrDBICheckVersions: %hs:%d", PefLastThrow()->SzFile(), PefLastThrow()->UlLine() ) );
    }

    return err;
}

//  Retrieves the desired DB versions structure and more importantly validates the versions of the 
//  DBFILEHDR against both the DB maximum and the client apps intention as expressed by their 
//  JET_paramEngineFormatValues setting.
//
//  ErrDBIValidateUserVersions() and ErrDBICheckVersion() are very similar to each other in that they 
//  both valdiate versions, but ErrDBICheckVersion() can be used in read only contexts by passing the
//  PfmtversEngineMax()->dbv to just check against what the engine understands.  For read-write contexts
//  the ErrDBIValidateUserVersions() must be used to allow us to save / perserve the user specified 
//  vesion in the case that JET_efvUsePersistedVersion is specified for the JET_paramEngineFormatVersion.
//
//  NOTE: for certain errors, such as JET_errEngineFormatVersionSpecifiedTooLowForDatabaseVersion or
//  JET_errInvalidDatabaseVersion the value of ppfmtversDesired can still be relied on.
//
ERR ErrDBIValidateUserVersions(
    _In_    const INST* const               pinst,
    _In_z_  const WCHAR* const              wszDbFullName,
    _In_    const IFMP                      ifmp,
    _In_    const DBFILEHDR_FIX * const     pdbfilehdr,
    _Out_   const FormatVersions ** const   ppfmtversDesired,
    _Out_   BOOL * const                    pfDbNeedsUpdate = NULL )
{
    ERR     err     = JET_errSuccess;

    BOOL fAllowPersistedFormat = fFalse;
    const FormatVersions * pfmtversDesired = NULL;

    //  Compute the desired DB version we want ...

    err = ErrDBGetDesiredVersion( pinst,
            // We could remove this silly ifmpNil thing?  AND FFMPIsTempDB thing?  If we fixedrepair to run through 
            // regular attach so it checks and updates (DBISetVersion()) through normal paths. Note: Not actually
            // sure it would be worth it.
            ( ifmp == ifmpNil /* from repair, pre-attach */ || !FFMPIsTempDB( ifmp ) ) ? pdbfilehdr : NULL,
            &pfmtversDesired,
            &fAllowPersistedFormat );
    //  these are used as signal errors from ErrDBICheckVersions()
    Assert( err != JET_errEngineFormatVersionSpecifiedTooLowForDatabaseVersion );
    Call( err );
    if ( ppfmtversDesired != NULL )
    {
        *ppfmtversDesired = pfmtversDesired;
    }

    if ( pfmtversDesired->efv < PfmtversEngineMax()->efv )
    {
        WCHAR wszEfvSetting[cchFormatEfvSetting];
        WCHAR wszEfvDesired[30];
        WCHAR wszEfvDefault[30];
        const WCHAR * rgszT[4] = { wszDbFullName, wszEfvSetting, wszEfvDesired, wszEfvDefault };

        FormatEfvSetting( (JET_ENGINEFORMATVERSION)UlParam( pinst, JET_paramEngineFormatVersion ), wszEfvSetting, sizeof(wszEfvSetting) );
        OSStrCbFormatW( wszEfvDesired, _cbrg( wszEfvDesired ), L"%d (0x%x)", pfmtversDesired->efv, pfmtversDesired->efv );
        OSStrCbFormatW( wszEfvDefault, _cbrg( wszEfvDefault ), L"%d (0x%x)", PfmtversEngineMax()->efv, PfmtversEngineMax()->efv );

        UtilReportEvent(
                eventInformation,
                GENERAL_CATEGORY,
                ATTACH_DATABASE_VERSION_UPGRADE_SKIPPED_ID,
                _countof( rgszT ),
                rgszT,
                0,
                NULL,
                pinst );
        OSTrace( JET_tracetagUpgrade, OSFormat( "Holding EFV at %ws due to param setting %ws, skipping any upgrades to latest EFV %ws.", wszEfvDesired, wszEfvSetting, wszEfvDefault ) );
    }

    Call( ErrDBICheckVersions( pinst, wszDbFullName, ifmp, pdbfilehdr, pfmtversDesired->dbv, fAllowPersistedFormat, pfDbNeedsUpdate ) );

HandleError:

    if ( err < JET_errSuccess )
    {
        OSTrace( JET_tracetagUpgrade, OSFormat( "FAILED: ErrDBIValidateUserVersions: %d", PefLastThrow()->UlLine() ) );
    }

    return err;
}

//  Sets the ver desired dbvUpdate value in the DBFILEHDR, ensuring that the DB version never
//  travels backwards.
void DBISetVersion(
        _In_    const INST* const               pinst,
        _In_z_  const WCHAR* const              wszDbFullName,
        _In_    const IFMP                      ifmp,
        _In_    const DbVersion&                dbvUpdate,
        _Inout_ DBFILEHDR_FIX * const           pdbfilehdr,
        _In_    const BOOL                      fDbNeedsUpdate,    // only for debugging
        _In_    const BOOL                      fCreateDbUpgradeDoNotLog )  // true to suppress logging on create DB path
{
    Assert( pinst != NULL );

    Assert( ifmp != ifmpNil && ifmp != 0 || g_fRepair );

    if ( ifmp != ifmpNil )
    {
        Assert( !g_rgfmp[ifmp].FReadOnlyAttach() );
    }

#ifdef DEBUG
    BOOL fUpdateCheckT = fFalse;
    const FormatVersions * pfmtversCheckT = NULL;
    const ERR errCheckT = ErrDBIValidateUserVersions( pinst, wszDbFullName, ifmp, pdbfilehdr, &pfmtversCheckT, &fUpdateCheckT );
    Assert( errCheckT >= JET_errSuccess );
    if ( errCheckT >= JET_errSuccess )
    {
        Expected( CmpDbVer( pfmtversCheckT->dbv, dbvUpdate ) == 0 ||
                    //  Redo follows what the log says, so may not check against the current version
                    ( pinst->FRecovering() && pinst->m_plog->FRecoveringMode() == fRecoveringRedo ) ||
                    //  Also repair just forces a version whether it's out of date or not as well.
                    g_fRepair );
        Expected( fUpdateCheckT ||
                    //  Redo forces its own versions ...
                    ( pinst->FRecovering() && pinst->m_plog->FRecoveringMode() == fRecoveringRedo &&
                        ( CmpDbVer( pdbfilehdr->Dbv(), dbvUpdate ) < 0 ) ) ||
                    g_fRepair );    // may be able to construct a case this goes off, but harmless.
    }

    if ( pdbfilehdr->le_ulCreateVersion == 0 )
    {
        Assert( pdbfilehdr->le_ulCreateUpdate == 0 );
        Assert( pdbfilehdr->le_ulVersion == 0 );
        Assert( pdbfilehdr->le_ulDaeUpdateMajor == 0 );
        Assert( pdbfilehdr->le_ulDaeUpdateMinor == 0 );
        Assert( pdbfilehdr->le_efvMaxBinAttachDiagnostic == 0 );
    }
#endif //   DEBUG

    BOOL fUpdatedHeader = fFalse;

    //  ErrDBICheckVersions & DBISetVersion are sort of paired, and if we are here, we would 
    //  expect one of the things this function knows how to update are out of date.
    Expected( pdbfilehdr->le_efvMaxBinAttachDiagnostic < PfmtversEngineMax()->efv ||
                CmpDbVer( pdbfilehdr->Dbv(), dbvUpdate ) < 0 ||
                g_fRepair );

    if ( pdbfilehdr->le_efvMaxBinAttachDiagnostic < PfmtversEngineMax()->efv &&
            //  This is not a normal usage of ErrDBFormatFeatureEnabled_() as typically code
            //  should be passing the current DB header format in as the 2nd arg.  Do not
            //  copy this example. ;)  We can't do that here b/c this is _whether to actual
            //  update the DB header iteself_, so the bootstrap has to check the desired / 
            //  param value instead.
            ErrDBFormatFeatureEnabled_( JET_efvSetDbVersion, dbvUpdate ) >= JET_errSuccess )
    {
        OSTrace( JET_tracetagUpgrade, OSFormat( "Updating DB.le_efvMaxBinAttachDiagnostic to %d from %d\n", PfmtversEngineMax()->efv, (ULONG)pdbfilehdr->le_efvMaxBinAttachDiagnostic  ) );
        pdbfilehdr->le_efvMaxBinAttachDiagnostic = PfmtversEngineMax()->efv;
        OnDebug( ifmp != ifmpNil ? g_rgfmp[ifmp].SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusUpdateSet ) : 3/*does nothing for repair passing ifmpNil*/ );
        fUpdatedHeader = fTrue;
    }

    //  We're trying to downgrade!  Danger, Will Robinson, Danger!
    //  We handle this, by skipping the update.
    if ( CmpDbVer( pdbfilehdr->Dbv(), dbvUpdate ) > 0 )
    {
        CHAR szFireWall[140]; // technically ~108 required.
        OSStrCbFormatA( szFireWall, sizeof(szFireWall), "InvalidDbVersionDowngradeAttempt-%I32u.%I32u.%I32u->%I32u.%I32u.%I32u",
                        (ULONG)pdbfilehdr->le_ulVersion, (ULONG)pdbfilehdr->le_ulDaeUpdateMajor, (ULONG)pdbfilehdr->le_ulDaeUpdateMinor, // stamped versions
                        dbvUpdate.ulDbMajorVersion, dbvUpdate.ulDbUpdateMajor, dbvUpdate.ulDbUpdateMinor );
        FireWall( szFireWall );
    }

    if ( CmpDbVer( pdbfilehdr->Dbv(), dbvUpdate ) < 0 )
    {
        DbVersion dbvCurr = pdbfilehdr->Dbv();

        //  Set version on db header
        pdbfilehdr->le_ulVersion = dbvUpdate.ulDbMajorVersion;
        pdbfilehdr->le_ulDaeUpdateMajor = dbvUpdate.ulDbUpdateMajor;
        OnDebug( ifmp != ifmpNil ? g_rgfmp[ifmp].SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusUpdateSet ) : 3/*does nothing for repair passing ifmpNil*/ );

        //  This is not a normal usage of ErrDBFormatFeatureEnabled_() as typically code
        //  should be passing the current DB header format in as the 2nd arg.  Do not
        //  copy this example. ;)  We can't do that here b/c this is _whether to actual
        //  update the DB header iteself_, so the bootstrap has to check the desired / 
        //  param value instead.
        if ( ErrDBFormatFeatureEnabled_( JET_efvSetDbVersion, dbvUpdate ) >= JET_errSuccess )
        {
#ifdef DEBUG
            //  The very next feature after JET_efvSetDbVersion, specifically the 
            //  JET_efvExtHdrRootFieldAutoIncStorageReleased feature has an UpdateMAJOR 
            //  change (not just an UpdateMINOR change like JET_efvSetDbVersion) ... DB 
            //  UpdateMajor changes are NOT allowed to be unlogged.  So if we've an update
            //  major change we need to expect logging.
            const FormatVersions * pfmtverExthdrAutoInc = NULL;
            CallS( ErrGetDesiredVersion( NULL, JET_efvExtHdrRootFieldAutoIncStorageReleased, &pfmtverExthdrAutoInc ) );
            const BOOL fRequireLoggingForDbvUpdate = CmpDbVer( dbvUpdate, pfmtverExthdrAutoInc->dbv ) >= 0;
            Assert( !fRequireLoggingForDbvUpdate || ifmp == ifmpNil || !g_rgfmp[ifmp].FLogOn() || pinst->FRecovering() || pinst->m_plog->ErrLGFormatFeatureEnabled( JET_efvSetDbVersion ) >= JET_errSuccess ); // to ensure we'll log this as well.
#endif

            pdbfilehdr->le_ulDaeUpdateMinor = dbvUpdate.ulDbUpdateMinor;
            OnDebug( ifmp != ifmpNil ? g_rgfmp[ifmp].SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusUpdateSet ) : 3/*does nothing for repair passing ifmpNil*/ );
        }

        //  Usually this version is not meant to be changed; 0 means it's never set, set it here
        if ( pdbfilehdr->le_ulCreateVersion == 0 )
        {
            pdbfilehdr->le_ulCreateVersion = dbvUpdate.ulDbMajorVersion;
            pdbfilehdr->le_ulCreateUpdate = dbvUpdate.ulDbUpdateMajor;

            OSTrace( JET_tracetagUpgrade, OSFormat( "Stamp Initial DB Version: %d.%d.%d (%d) - %ws\n",
                        (ULONG)pdbfilehdr->le_ulVersion, (ULONG)pdbfilehdr->le_ulDaeUpdateMajor, (ULONG)pdbfilehdr->le_ulDaeUpdateMinor, // stamped versions
                        (ULONG)pdbfilehdr->le_efvMaxBinAttachDiagnostic,
                        wszDbFullName ) );
        }
        else if ( ifmp != ifmpNil && ifmp != 0 && !FFMPIsTempDB( ifmp ) )
        {
            OSTrace( JET_tracetagUpgrade, OSFormat( "Upgrade DB Version: %d.%d.%d (Cr %d.%d) from %d.%d.%d - %ws\n",
                        (ULONG)pdbfilehdr->le_ulVersion, (ULONG)pdbfilehdr->le_ulDaeUpdateMajor, (ULONG)pdbfilehdr->le_ulDaeUpdateMinor, // new versions
                        (ULONG)pdbfilehdr->le_ulCreateVersion, (ULONG)pdbfilehdr->le_ulCreateUpdate,
                        dbvCurr.ulDbMajorVersion, dbvCurr.ulDbUpdateMajor, dbvCurr.ulDbUpdateMinor, // current
                        wszDbFullName ) );

            if ( !fCreateDbUpgradeDoNotLog )
            {

                // Perhaps move this after the header write, so we know we've successfully upgraded.  Low probability fail.
                WCHAR wszDbvBefore[50];
                WCHAR wszDbvAfter[50];
                WCHAR wszEfvSetting[cchFormatEfvSetting];
                //  Wish we had redo here.
                const WCHAR * rgszT[4] = { wszDbFullName, wszDbvBefore, wszDbvAfter, wszEfvSetting };
            
                OSStrCbFormatW( wszDbvBefore, _cbrg( wszDbvBefore ), L"%d.%d.%d", dbvCurr.ulDbMajorVersion, dbvCurr.ulDbUpdateMajor, dbvCurr.ulDbUpdateMinor );
                OSStrCbFormatW( wszDbvAfter, _cbrg( wszDbvAfter ), L"%d.%d.%d", (ULONG)pdbfilehdr->le_ulVersion, (ULONG)pdbfilehdr->le_ulDaeUpdateMajor, (ULONG)pdbfilehdr->le_ulDaeUpdateMinor );
                FormatEfvSetting( (JET_ENGINEFORMATVERSION)UlParam(pinst, JET_paramEngineFormatVersion), wszEfvSetting, sizeof(wszEfvSetting) );

                UtilReportEvent( eventInformation,
                        GENERAL_CATEGORY,
                        ATTACH_DATABASE_VERSION_UPGRADED_ID,
                        _countof( rgszT ),
                        rgszT,
                        0,
                        NULL,
                        pinst );
            }
        }

        fUpdatedHeader = fTrue;
    }

    Assert( fUpdatedHeader == fDbNeedsUpdate || pinst->FRecovering() || g_fRepair );
    Assert( fUpdatedHeader == fDbNeedsUpdate || g_fRepair );

    if ( fUpdatedHeader )
    {
        if ( ifmp != ifmpNil )
        {
            Assert( g_rgfmp[ifmp].FHeaderUpdateInProgress() );
        }
    }
}

//  Dual purpose, rolls the log if necessary to support logging of the DB versions (lrtypSetDbVer) and
//  logs the desired DB version (which will subsequently be set in the DB file's header).
//
ERR ErrDBLGVersionUpdate(
    _In_    const INST * const              pinst,
    _In_    PIB * const                     ppib,
    _In_    const IFMP                      ifmp,
    _In_    const FormatVersions * const    pfmtversDesired,
    _In_    const BOOL                      fDbNeedsUpdate )
{
    ERR err = JET_errSuccess;
    FMP * const pfmp = &g_rgfmp[ifmp];
    OnDebug( BOOL fPersistedLogVerHoldingBackLogUpgrade = fFalse );

    BOOL fLogVersionChanged = fFalse;

    Assert( g_rgfmp[ ifmp ].FLogOn() );
    Assert( pinst->m_plog->FRecoveringMode() != fRecoveringUndo );

    if ( pfmtversDesired->efv >= JET_efvSetDbVersion &&
        pinst->m_plog->ErrLGFormatFeatureEnabled( pfmtversDesired->efv ) < JET_errSuccess )
    {
        Call( ErrLGWaitForWrite( ppib, &lgposMax ) );
        if ( pinst->m_plog->ErrLGFormatFeatureEnabled( pfmtversDesired->efv ) < JET_errSuccess )
        {
            fLogVersionChanged = pinst->m_plog->FLGFileVersionUpdateNeeded( pfmtversDesired->lgv );

            //  Log rollover if there's log version change
            if ( fLogVersionChanged && pinst->m_plog->ErrLGFormatFeatureEnabled( pfmtversDesired->efv ) < JET_errSuccess )
            {
                OnDebug( const LONG lgenBefore = pinst->m_plog->PlgfilehdrForVerCtrl()->le_lGeneration );
                CHAR    szTrace[ 100 ];
                LGPOS   lgposRoll;
                OSStrCbFormatA( szTrace, sizeof( szTrace ), "Demand upgrade to lrtypSetDbVer (dbid:%d)", pfmp->Dbid() );
                Call( ErrLGForceLogRollover( ppib, szTrace, &lgposRoll ) );
                Call( ErrLGWaitForWrite( ppib, &lgposRoll ) );

                Assert( lgenBefore < pinst->m_plog->PlgfilehdrForVerCtrl()->le_lGeneration );
                Assert( lgenBefore <= pinst->m_plog->PlgfilehdrForVerCtrl()->le_lGeneration );
#ifdef DEBUG
                //  if still not upgraded and we're set to use persisted version, check to see if the persisted
                //  version is holding back the upgrade.
                if ( pinst->m_plog->ErrLGFormatFeatureEnabled( pfmtversDesired->efv ) < JET_errSuccess &&
                        UlParam( pinst, JET_paramEngineFormatVersion ) == JET_efvUsePersistedFormat )
                {
                    const LogVersion * plgvDesired = NULL;
                    CallS( pinst->m_plog->ErrLGGetPersistedLogVersion( pfmtversDesired->efv, &plgvDesired ) );
                    const FormatVersions * pfmtverRequiredForSetDbVersion = NULL;
                    CallS( ErrGetDesiredVersion( NULL, pfmtversDesired->efv, &pfmtverRequiredForSetDbVersion ) );
                    fPersistedLogVerHoldingBackLogUpgrade = CmpLgVer( *plgvDesired, pfmtverRequiredForSetDbVersion->lgv ) < 0;
                }
                if ( !fPersistedLogVerHoldingBackLogUpgrade )
                {
                    Assert( !pinst->m_plog->FLGFileVersionUpdateNeeded( pfmtversDesired->lgv ) );
                    Assert( pinst->m_plog->ErrLGFormatFeatureEnabled( pfmtversDesired->efv ) >= JET_errSuccess );
                }
#endif
            }
        }
    }

    //  We could do this instead
    //      if ( fDbNeedsUpdate && pinst->m_plog->ErrLGFormatFeatureEnabled( JET_efvSetDbVersion ) < JET_errSuccess )
    //  So one could avoid logging if !fDbNeedsUpdate (and indeed we later avoid re-setting
    //  the same value in the header and definitely want to avoid an extra header IO), BUT
    //  due to nothing really protecting HA inc-reseed from removing a log file that SetDbVer
    //  LR that hasn't played into this DB, but has to played into others, it can lead to an
    //  inconsistent view of the actual DB version.  And then a subsequent active who happened
    //  to replay the SetDbVer will not re-log it if we avoid relogging where the header isn't
    //  out of date.  What I don't like about this, is the passive DB will play forward with
    //  an inaccurate DB version, so instead we'll just always log the latest DB Ver on attach
    //  (you could think of it as being part of the attach LR) so that the inconsistency will
    //  be as temporary as possible, the next active will re-log the current DB ver and the
    //  passive the inc-reseeded the hdr update away will get a fresh one from the next active
    //  whether he has the current DB ver or a behind one.  Hope that made sense.
    //
    if ( pinst->m_plog->ErrLGFormatFeatureEnabled( JET_efvSetDbVersion ) >= JET_errSuccess )
    {
        LGPOS logposT;

        // It might seem like an Assert, but do-time, nor even redo of DBISetVersion will let
        // you take a DB backwards.  It is always one way.
        Expected( CmpDbVer( pfmp->Pdbfilehdr()->Dbv(), pfmtversDesired->dbv ) <= 0 );

        Call( ErrLGSetDbVersion( ppib, ifmp, pfmtversDesired->dbv, &logposT ) );

        //  And because of the spitting out of an unnecessary LR for the update, then we have 
        //  to smack our HeaderUpdateState() forward to realize it is ok to do this cycle in 
        //  this special case.
        if ( !fDbNeedsUpdate )
        {
            OnDebug( pfmp->SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusUpdateSet ) );
            OnDebug( pfmp->SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusUpdateFlushed ) );
        }
    }
    else
    {
#ifdef DEBUG
        if ( fDbNeedsUpdate )
        {
            //  Log and DB upgrade together for SetDbVersion, so if we can't log it, DB version should be 
            //  lower than this - which is legal.
            const FormatVersions * pfmtverSetDbVer = NULL;
            CallS( ErrGetDesiredVersion( NULL, JET_efvSetDbVersion, &pfmtverSetDbVer ) );
            // We're faking this ... because normally we would want to log, but since we're running
            // on pre SetDbVer LR, we can't log, so fake we logged.
            Assert( CmpDbVer( pfmp->Pdbfilehdr()->Dbv(), pfmtverSetDbVer->dbv ) < 0 );
            Assert( pinst->m_plog->ErrLGFormatFeatureEnabled( JET_efvSetDbVersion ) < JET_errSuccess );
            pfmp->SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusUpdateLogged );
        }
#endif
    }

HandleError:

    return err;
}

//  For given DB it retrieves the current version, checks the version is compatibile with the engine and
//  the client settings (JET_paramEngineFormatVersion) and then upgrades it (both logging and setting the
//  actual DBFILEHDR on the FMP), and finally writes the header to disk.
//
ERR ErrDBUpdateAndFlushVersion(
    _In_    const INST* const       pinst,
    _In_z_  const WCHAR * const     wszDbFullName,
    _In_    PIB * const             ppib,
    _In_    const IFMP              ifmp,
    _In_    const BOOL              fCreateDbUpgradeDoNotLog )  // true to suppress logging on create DB path
{
    ERR err = JET_errSuccess;
    FMP * const pfmp = &g_rgfmp[ifmp];
    
    Assert( ifmp != ifmpNil && ifmp != 0 ); // ?? FValidIfmp()?
    Assert( !pinst->FRecovering() );
    Assert( pinst->m_plog->FRecoveringMode() != fRecoveringUndo );

    DbVersion dbvBefore;
    BOOL fDbNeedsUpdate = fFalse;
    const FormatVersions * pfmtversDesired = NULL;

    {
    PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr();

    dbvBefore = pdbfilehdr->Dbv();

    //  Check if the header is compatible and/or needs updating.

    Call( ErrDBIValidateUserVersions( pinst, wszDbFullName, ifmp, pdbfilehdr.get(), &pfmtversDesired, &fDbNeedsUpdate ) );
    } // .dtor release dbfilehdr lock

    //  Do not even log (and of course update DB, but that's double protected lower) if the desired
    //  version is below the current pdbfilehdr version.  We do allow logging of equal DbVersions in 
    //  case a log replaying replica missed such an update due to Incremental Reseed or lossy failover.

    if ( CmpDbVer( dbvBefore, pfmtversDesired->dbv ) > 0 )
    {
        Assert( !fDbNeedsUpdate );

        OSTrace( JET_tracetagUpgrade, OSFormat( "Suppress DB Version Downgrade LR: %d.%d.%d to %d.%d.%d - %ws\n",
                    dbvBefore.ulDbMajorVersion, dbvBefore.ulDbUpdateMajor, dbvBefore.ulDbUpdateMinor, // current dbfilehdr
                    pfmtversDesired->dbv.ulDbMajorVersion, pfmtversDesired->dbv.ulDbUpdateMajor, pfmtversDesired->dbv.ulDbUpdateMinor,
                    wszDbFullName ) );
        return JET_errSuccess;
    }
    Assert( fDbNeedsUpdate || CmpDbVer( dbvBefore, pfmtversDesired->dbv ) == 0 );

    if ( g_rgfmp[ ifmp ].FLogOn() )
    {

        //  Note: takes care of both rolling log to upgrade/downgrade the log version as well as logging the header
        //  versions update (so must be called whether fDbNeedsUpdate is true or not).  Also we always log the
        //  SetDbVer on every attach to workaround inc reseed issues documented in this function.
        Call( ErrDBLGVersionUpdate( pinst, ppib, ifmp, pfmtversDesired, fDbNeedsUpdate ) );
    }

    {
    PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
    
    //  Post logging dbv should not have been updated by any other method ...

    OnDebug( if ( ( rand() % 5 ) == 0 ) UtilSleep( 20 ) );

    AssertTrack( CmpDbVer( dbvBefore, pdbfilehdr->Dbv() ) == 0, "UnexpectedDbVersionChange" );  // upgrade to Enforce at some point

    if ( fDbNeedsUpdate )
    {
        //  Update the headers DB version to the desired one.

        DBISetVersion( pinst, wszDbFullName, ifmp, pfmtversDesired->dbv, pdbfilehdr.get(), fDbNeedsUpdate, fCreateDbUpgradeDoNotLog );
    }

    } // .dtor release dbfilehdr lock

    Assert( CmpDbVer( pfmp->Pdbfilehdr()->Dbv(), pfmtversDesired->dbv ) >= 0 );

    if ( fDbNeedsUpdate )
    {
        Assert( pfmp->Pfapi() );
        err = ErrUtilWriteAttachedDatabaseHeaders( pinst, pinst->m_pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() );
    }

    //  Today, this shouldn't go off, because there is only one consumer of the header update 
    //  state. But if someone utilizes it from like checkpoint update or something, this will
    //  start going off and someone really should allow (and probably increase the header lock
    //  rank) the header lock be held over logging data, and definitely let the lock be passed
    //  into ErrUtilWriteAttachedDatabaseHeaders().
    Assert( !pfmp->FHeaderUpdateInProgress() || err < JET_errSuccess );

    //  We have somehow downgraded the DB version!  Bad.
    AssertTrack( CmpDbVer( pfmp->Pdbfilehdr()->Dbv(), dbvBefore ) >= 0, "InvalidDbVersionDowngradeUpdate" );

HandleError:
    return err;
}

//  This attempts to reapply the relevant DbVersion from a lrtypSetDbVer operation during recovery::redo.
//
ERR ErrDBRedoSetDbVersion(
        _In_    const INST* const               pinst,
        _In_    const IFMP                      ifmp,
        _In_    const DbVersion&                dbvSet )
    {
    ERR err = JET_errSuccess;
    DBFILEHDR * pdbfilehdrTestUpdate = NULL;

    OSTrace( JET_tracetagDatabases, OSFormat( "Evaluating SetDbVersion LR( %d.%d.%d )\n", dbvSet.ulDbMajorVersion, dbvSet.ulDbUpdateMajor, dbvSet.ulDbUpdateMinor ) );

    Assert( pinst->FRecovering() );
    Assert( pinst->m_plog->FRecoveringMode() == fRecoveringRedo );

    Assert( dbvSet.ulDbMajorVersion != 0 );
    
    FMP * pfmp = &g_rgfmp[ifmp];

    DbVersion dbvBefore;
    BOOL fRedo = fFalse;
    {
    PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();

    dbvBefore = pdbfilehdr->Dbv();

    //  First we check if updating with this SetDbVersion LR would push us past what we allow
    //  for user settings ... the easiest way is to create a false header as if the update was
    //  applied and check against that. :P

    Alloc( pdbfilehdrTestUpdate = (DBFILEHDR*)PvOSMemoryPageAlloc( g_cbPage, NULL ) );
    memcpy( pdbfilehdrTestUpdate, pdbfilehdr.get(), g_cbPage );
    Assert( CmpDbVer( pdbfilehdr->Dbv(), pdbfilehdrTestUpdate->Dbv() ) == 0 );
    //DBISetVersion( pinst, pfmp->WszDatabaseName(), ifmp, dbvSet, pdbfilehdrTestUpdate, fT );
    if ( CmpDbVer( pdbfilehdrTestUpdate->Dbv(), dbvSet ) < 0 )
    {
        pdbfilehdrTestUpdate->le_ulVersion = dbvSet.ulDbMajorVersion;
        pdbfilehdrTestUpdate->le_ulDaeUpdateMajor = dbvSet.ulDbUpdateMajor;
        pdbfilehdrTestUpdate->le_ulDaeUpdateMinor = dbvSet.ulDbUpdateMinor;
    }
    const FormatVersions * pfmtversCheckT = NULL;
    Call( ErrDBIValidateUserVersions( pinst, pfmp->WszDatabaseName(), ifmp, pdbfilehdrTestUpdate, &pfmtversCheckT ) );

    const BOOL fAllowPersistedFormat = ( UlParam( pinst, JET_paramEngineFormatVersion ) & JET_efvAllowHigherPersistedFormat ) == JET_efvAllowHigherPersistedFormat;

    Expected( CmpDbVer( pfmtversCheckT->dbv, dbvSet ) >= 0 ||
                FUpdateMinorMismatch( CmpDbVer( pfmtversCheckT->dbv, dbvSet ) ||
                fAllowPersistedFormat ) );  // otherwise we should've failed ErrDBIValidateUserVersions() check ...


    if ( CmpDbVer( pdbfilehdr->Dbv(), dbvSet ) < 0 )
    {
        //  Now evaluate if we need to update the actual DB header

        Call( ErrDBICheckVersions( pinst, pfmp->WszDatabaseName(), ifmp, pdbfilehdr.get(), dbvSet, fAllowPersistedFormat, &fRedo ) );

        if ( fRedo )
        {
            OSTrace( JET_tracetagUpgrade, OSFormat( "Replaying SetDbVersion LR( %d.%d.%d ) to upgrade DB (from %d.%d.%d)\n",
                        dbvSet.ulDbMajorVersion, dbvSet.ulDbUpdateMajor, dbvSet.ulDbUpdateMinor,
                        dbvBefore.ulDbMajorVersion, dbvBefore.ulDbUpdateMajor, dbvBefore.ulDbUpdateMinor ) );
            DBISetVersion( pinst, pfmp->WszDatabaseName(), ifmp, dbvSet, pdbfilehdr.get(), fRedo, fFalse );
        }
    }
    else
    {
        // DB file's DbVersion is higher than the DbVersion we're being asked to set, redo 
        // doesn't make sense - AND is potentially dangerous / a lie.
        Enforce( !fRedo );
    }
    } // PdbfilehdrReadWrite

    if ( fRedo )
    {
        Assert( pfmp->Pfapi() );
        err = ErrUtilWriteAttachedDatabaseHeaders( pinst, pinst->m_pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() );
    }

    // We have somehow downgraded the DB version!  Bad.
    AssertTrack( CmpDbVer( pfmp->Pdbfilehdr()->Dbv(), dbvBefore ) >= 0, "InvalidDbVersionDowngradeRedo" );

HandleError:
    OSMemoryPageFree( pdbfilehdrTestUpdate );
    return err;
}

void InitDBDbfilehdr(
    _In_ const FMP * const pfmp,
    _In_ const LOG * const plog,
    _In_ const ULONG grbit,
    _In_opt_ SIGNATURE * psignDb,
    _Out_ DBFILEHDR * const pdbfilehdr )
{
    // This memset() is needed (calling the struct CZeroInit .ctor would NOT be sufficient) to set the whole of the 
    // header past the actual struct size to zero, to ensure easy upgrade paths.
    memset( pdbfilehdr, 0, g_cbPage );

    // Other versions are set latter with ErrDBSetVersion
    pdbfilehdr->le_ulMagic = ulDAEMagic;

    //  Never changing point in time, the subsequent attach / the SetDbVer LRs will then upgrade.
    const ULONG ulDAEVersionMax_BeforeSetDbVerLr        = 0x00000620;
    const ULONG ulDAEUpdateMajorMax_BeforeSetDbVerLr    = 0x00000014;
    pdbfilehdr->le_ulVersion = ulDAEVersionMax_BeforeSetDbVerLr;
    pdbfilehdr->le_ulDaeUpdateMajor = ulDAEUpdateMajorMax_BeforeSetDbVerLr;
    pdbfilehdr->le_ulCreateVersion = ulDAEVersionMax_BeforeSetDbVerLr;
    pdbfilehdr->le_ulCreateUpdate = ulDAEUpdateMajorMax_BeforeSetDbVerLr;

    Assert( 0 == pdbfilehdr->le_dbtimeDirtied );
    Assert( 0 == pdbfilehdr->le_objidLast );
    Assert( attribDb == pdbfilehdr->le_attrib );

    Assert( 0 == pdbfilehdr->m_ulDbFlags );
    Assert( !pdbfilehdr->FShadowingDisabled() );
    if ( grbit & JET_bitDbShadowingOff )
    {
        pdbfilehdr->SetShadowingDisabled();
    }

    pdbfilehdr->le_dbid = pfmp->Dbid();
    if ( psignDb == NULL )
    {
        SIGGetSignature( &pdbfilehdr->signDb );
    }
    else
    {
        UtilMemCpy( &pdbfilehdr->signDb, psignDb, sizeof( SIGNATURE ) );
    }
    pdbfilehdr->SetDbstate( JET_dbstateJustCreated );
    Assert( 0 == pdbfilehdr->le_dbtimeDirtied );
    Assert( 0 == pdbfilehdr->le_objidLast );

    pdbfilehdr->le_cbPageSize = g_cbPage;

    if ( plog->FLogDisabled() || ( grbit & JET_bitDbRecoveryOff ) )
    {
        memset( &pdbfilehdr->signLog, 0, sizeof( SIGNATURE ) );
    }
    else
    {
        pdbfilehdr->signLog = plog->SignLog();
    }

    pdbfilehdr->le_filetype = JET_filetypeDatabase;

    pdbfilehdr->le_dbtimeDirtied = pfmp->DbtimeLast();

}

ERR ErrDBParseDbParams(
    _In_reads_opt_( csetdbparam )const JET_SETDBPARAM* const    rgsetdbparam,
    _In_ const ULONG                                            csetdbparam,
    _Out_opt_ CPG* const                                        pcpgDatabaseSizeMax,            // JET_dbparamDbSizeMaxPages
    _Out_opt_ ULONG* const                                      ppctCachePriority,              // JET_dbparamCachePriority
    _Out_opt_ JET_GRBIT* const                                  pgrbitShrinkDatabaseOptions,    // JET_dbparamShrinkDatabaseOptions
    _Out_opt_ LONG* const                                       pdtickShrinkDatabaseTimeQuota,  // JET_dbparamShrinkDatabaseTimeQuota
    _Out_opt_ CPG* const                                        pcpgShrinkDatabaseSizeLimit,    // JET_dbparamShrinkDatabaseSizeLimit
    _Out_opt_ BOOL* const                                       pfLeakReclaimerEnabled,         // JET_dbparamLeakReclaimerEnabled
    _Out_opt_ LONG* const                                       pdtickLeakReclaimerTimeQuota,   // JET_dbparamLeakReclaimerTimeQuota
    _Out_opt_ FEATURECONTROL* const                             pfcMaintainExtentPageCountCache // JET_dbparamMaintainExtentPageCountCache
    )
{
    ULONG ulRawMaintainExtentPageCountCache = 0;
    BOOL fFoundMaintainExtentPageCountCache = fFalse;

    if ( ( rgsetdbparam == NULL ) && ( csetdbparam > 0 ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    Expected( ( csetdbparam > 0 ) || ( rgsetdbparam == NULL ) );    // Why would you have a valid pointer and no elements to process?

    //
    // Set all Db parameter defaults.
    //

    if ( pcpgDatabaseSizeMax != NULL )
    {
        *pcpgDatabaseSizeMax = 0;
    }

    if ( ppctCachePriority != NULL )
    {
        *ppctCachePriority = g_pctCachePriorityUnassigned;
    }

    if ( pgrbitShrinkDatabaseOptions != NULL )
    {
#ifdef DEBUG
        *pgrbitShrinkDatabaseOptions = JET_bitShrinkDatabaseEofOnAttach;
#else
        *pgrbitShrinkDatabaseOptions = NO_GRBIT;
#endif
    }

    if ( pdtickShrinkDatabaseTimeQuota != NULL )
    {
        *pdtickShrinkDatabaseTimeQuota = -1;
    }

    if ( pcpgShrinkDatabaseSizeLimit != NULL )
    {
        *pcpgShrinkDatabaseSizeLimit = 0;
    }

    if ( pfLeakReclaimerEnabled != NULL )
    {
        *pfLeakReclaimerEnabled = fFalse;
    }

    if ( pdtickLeakReclaimerTimeQuota != NULL )
    {
        *pdtickLeakReclaimerTimeQuota = -1;
    }

    if ( pfcMaintainExtentPageCountCache )
    {
        *pfcMaintainExtentPageCountCache = FEATURECONTROL::fcNotSpecified;
    }

    //
    // Go through the array of DB parameters and collect all user inputs.
    //

    for ( ULONG isetdbparam = 0; isetdbparam < csetdbparam; isetdbparam++ )
    {
        const ULONG dbparamid = rgsetdbparam[ isetdbparam ].dbparamid;
        const void* const pvParam = rgsetdbparam[ isetdbparam ].pvParam;
        const ULONG cbParam = rgsetdbparam[ isetdbparam ].cbParam;
        void* pvParamDest = NULL;
        ULONG cbParamDest = 0;

        // Collect size and memory location of each user input.
        switch ( dbparamid )
        {
            case JET_dbparamDbSizeMaxPages:
                cbParamDest = sizeof( *pcpgDatabaseSizeMax );
                pvParamDest = pcpgDatabaseSizeMax;
                break;

            case JET_dbparamCachePriority:
                cbParamDest = sizeof( *ppctCachePriority );
                pvParamDest = ppctCachePriority;
                break;

            case JET_dbparamShrinkDatabaseOptions:
                cbParamDest = sizeof( *pgrbitShrinkDatabaseOptions );
                pvParamDest = pgrbitShrinkDatabaseOptions;
                break;

            case JET_dbparamShrinkDatabaseTimeQuota:
                cbParamDest = sizeof( *pdtickShrinkDatabaseTimeQuota );
                pvParamDest = pdtickShrinkDatabaseTimeQuota;
                break;

            case JET_dbparamShrinkDatabaseSizeLimit:
                cbParamDest = sizeof( *pcpgShrinkDatabaseSizeLimit );
                pvParamDest = pcpgShrinkDatabaseSizeLimit;
                break;

            case JET_dbparamLeakReclaimerEnabled:
                cbParamDest = sizeof( *pfLeakReclaimerEnabled );
                pvParamDest = pfLeakReclaimerEnabled;
                break;

            case JET_dbparamLeakReclaimerTimeQuota:
                cbParamDest = sizeof( *pdtickLeakReclaimerTimeQuota );
                pvParamDest = pdtickLeakReclaimerTimeQuota;
                break;

            case JET_dbparamMaintainExtentPageCountCache:
                fFoundMaintainExtentPageCountCache = fTrue;
                cbParamDest = sizeof( ulRawMaintainExtentPageCountCache );
                pvParamDest = &ulRawMaintainExtentPageCountCache;
                break;

            default:
                Expected( ( dbparamid >= JET_dbparamDbSizeMaxPages ) /* min value */ && ( dbparamid < ( JET_dbparamDbSizeMaxPages + 1024 ) ) ); // or they're passing a sysparam or sesparam?
                return ErrERRCheck( JET_errInvalidDbparamId );
        }

        //
        // Validates size and pointers.
        //

        // Caller is not expecting this parameter, do nothing.
        if ( pvParamDest == NULL )
        {
            continue;
        }

        // Size mismatches.
        if ( cbParam != cbParamDest )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }

        // Invalid pointer.
        if ( ( pvParam == NULL ) && ( cbParam > 0 ) )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }

        Expected( ( cbParam > 0 ) || ( pvParam == NULL ) ); // Why would you have a valid pointer and no bytes to copy?

        //
        // Finally, copy the data.
        //

        if ( cbParam > 0 )
        {
            Assert( ( pvParamDest != NULL ) && ( pvParam != NULL ) );
            Assert( cbParam == cbParamDest );
            UtilMemCpy( pvParamDest, pvParam, cbParamDest );
        }
    }

    //
    // Validate individual parameters.
    //

    // JET_dbparamCachePriority.
    if ( ( ppctCachePriority != NULL ) &&
            FIsCachePriorityAssigned( *ppctCachePriority ) &&
            !FIsCachePriorityValid( *ppctCachePriority ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    // JET_dbparamShrinkDatabaseOptions.
    if ( ( pgrbitShrinkDatabaseOptions != NULL ) &&
        ( ( *pgrbitShrinkDatabaseOptions &
          ~( JET_bitShrinkDatabaseEofOnAttach |
             JET_bitShrinkDatabaseFullCategorizationOnAttach |
             JET_bitShrinkDatabaseDontMoveRootsOnAttach |
             JET_bitShrinkDatabaseDontTruncateLeakedPagesOnAttach |
             JET_bitShrinkDatabaseDontTruncateIndeterminatePagesOnAttach ) ) != 0 ) )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    // JET_dbparamShrinkDatabaseTimeQuota.
    if ( ( pdtickShrinkDatabaseTimeQuota != NULL ) &&
            ( *pdtickShrinkDatabaseTimeQuota > ( 7 * 24 * 60 * 60 * 1000 ) ) &&
            ( *pdtickShrinkDatabaseTimeQuota != -1 ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    // JET_dbparamLeakReclaimerTimeQuota.
    if ( ( pdtickLeakReclaimerTimeQuota != NULL ) &&
            ( *pdtickLeakReclaimerTimeQuota > ( 7 * 24 * 60 * 60 * 1000 ) ) &&
            ( *pdtickLeakReclaimerTimeQuota != -1 ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    // JET_dbparamShrinkDatabaseSizeLimit.
    if ( ( pcpgShrinkDatabaseSizeLimit != NULL ) && ( *pcpgShrinkDatabaseSizeLimit < 0 ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    // JET_dbparamMaintainExtentPageCountCache
    if ( pfcMaintainExtentPageCountCache )
    {
        if ( fFoundMaintainExtentPageCountCache )
        {
            if ( ulRawMaintainExtentPageCountCache )
            {
                // Non-zero raw value means enable.
                *pfcMaintainExtentPageCountCache = FEATURECONTROL::fcEnableFromParam;
            }
            else
            {
                *pfcMaintainExtentPageCountCache = FEATURECONTROL::fcDisableFromParam;
            }
        }

        //
        // Allow registry based override of value. We differentiate between the override and the param
        // so when we try to create the table, if the efv doesn't support it but the param requested it,
        // we return an error, but if it's the override, we quietly do nothing.
        //
        WCHAR       wszBufExtentPageCountCache[ 2 ];
        if ( FOSConfigGet( wszDEBUGRoot, L"ExtentPageCountCacheCreateOverride", wszBufExtentPageCountCache, sizeof(wszBufExtentPageCountCache) ) )
        {
            switch ( wszBufExtentPageCountCache[0] )
            {
                case 0:
                    // No override specified.
                    break;

                case L'0':
                    if ( FEATURECONTROL::fcDisableFromParam != *pfcMaintainExtentPageCountCache)
                    {
                        // Force-disable the feature. Useful for tests or as an emergency control to
                        // turn off the feature regardless of client intent.
                        *pfcMaintainExtentPageCountCache = FEATURECONTROL::fcDisableFromOverride;
                    }
                    break;

                default:
                    if ( FEATURECONTROL::fcEnableFromParam != *pfcMaintainExtentPageCountCache)
                    {
                        // Force-enable the feature.
                        *pfcMaintainExtentPageCountCache = FEATURECONTROL::fcEnableFromOverride;
                    }
                    break;
            }

        }
    }

    return JET_errSuccess;
}

ERR ErrDBCreateDatabase(
    _In_ PIB                                                    *ppib,
    _In_ IFileSystemAPI                                         *pfsapiDest,
    _In_z_ const WCHAR                                          *wszDatabaseName,
    _Out_ IFMP                                                  *pifmp,
    _In_ DBID                                                   dbidGiven,
    _In_ CPG                                                    cpgPrimary,
    _In_ const BOOL                                             fSparseEnabledFile,
    _In_opt_ SIGNATURE                                          *psignDb,
    _In_reads_opt_( csetdbparam ) const JET_SETDBPARAM* const   rgsetdbparam,
    _In_ const ULONG                                            csetdbparam,
    _In_ const JET_GRBIT                                        grbit )
{
    ERR             err;
    WCHAR           rgwchDbFullName[IFileSystemAPI::cchPathMax];
    WCHAR           *wszDbFullName          = rgwchDbFullName;
    CPG             cpgDatabaseSizeMax      = 0;
    ULONG           pctCachePriority        = g_pctCachePriorityUnassigned;
    BOOL            fDatabaseOpen           = fFalse;
    BOOL            fNewDBCreated           = fFalse;
    BOOL            fLogged                 = fFalse;
    LGPOS           lgposLogRec;
    FEATURECONTROL  fcMaintainExtentPageCountCache;

    CallR( ErrDBParseDbParams( rgsetdbparam,
                               csetdbparam,
                               &cpgDatabaseSizeMax,
                               &pctCachePriority,
                               NULL,                 // JET_dbparamShrinkDatabaseOptions (not used here).
                               NULL,                 // JET_dbparamShrinkDatabaseTimeQuota (not used here).
                               NULL,                 // JET_dbparamShrinkDatabaseSizeLimit (not used here).
                               NULL,                 // JET_dbparamLeakReclaimerEnabled (not used here).
                               NULL,                 // JET_dbparamLeakReclaimerTimeQuota (not used here).
                               &fcMaintainExtentPageCountCache ) );

    Assert( !( grbit & bitCreateDbImplicitly ) || PinstFromPpib( ppib )->m_plog->FRecovering() );

    Assert( cpgPrimary );   //  caller should've protected.
    if ( dbidGiven == dbidTemp )
    {
        Assert( cpgPrimary >= cpgMultipleExtentMin );
    }
    else if ( grbit & bitCreateDbImplicitly )
    {
        //  Legacy create DB from recovery ...
        Assert( PinstFromPpib( ppib )->m_plog->FRecovering() );
        Assert( cpgPrimary >= cpgLegacyDatabaseDefaultSize );
    }
    else
    {
        Assert( cpgPrimary >= CpgDBDatabaseMinMin() );
    }

    CheckPIB( ppib );

    OSTrace(
        JET_tracetagDatabases,
        OSFormat(
            "Session=[0x%p:0x%x] attaching database %s [cpgMax=0x%x,pctCachePriority=%I32u,grbit=0x%x,sparse=%d]",
            ppib,
            ppib->trxBegin0,
            SzOSFormatStringW( wszDatabaseName ),
            cpgDatabaseSizeMax,
            pctCachePriority,
            grbit,
            fSparseEnabledFile ) );

    if ( ppib->Level() > 0 )
    {
        return ErrERRCheck( JET_errInTransaction );
    }

    if ( cpgPrimary > pgnoSysMax )
    {
        return ErrERRCheck( JET_errDatabaseInvalidPages );
    }

    if ( grbit & JET_bitDbVersioningOff )
    {
        if ( !( grbit & JET_bitDbRecoveryOff ) )
        {
            return ErrERRCheck( JET_errCannotDisableVersioning );
        }
    }

    INST            *pinst = PinstFromPpib( ppib );
    LOG             *plog = pinst->m_plog;
    IFMP            ifmp;
    IFileSystemAPI  *pfsapi;

    Assert( fRecoveringUndo != plog->FRecoveringMode() );
    Assert( !(grbit & bitCreateDbImplicitly) || plog->FRecoveringMode() );

    if ( NULL != pfsapiDest )
    {
        pfsapi = pfsapiDest;
    }
    else
    {
        pfsapi = pinst->m_pfsapi;
    }

    //  if recovering and ifmp is given latch the fmp first
    //
    if ( plog->FRecovering() && dbidGiven < dbidMax && dbidGiven != dbidTemp )
    {
        ifmp = pinst->m_mpdbidifmp[ dbidGiven ];
        FMP::AssertVALIDIFMP( ifmp );

        //  get corresponding ifmp
        //
        CallS( FMP::ErrNewAndWriteLatch(
            &ifmp,
            g_rgfmp[ifmp].WszDatabaseName(),
            ppib,
            pinst,
            pfsapi,
            dbidGiven,
            fTrue,
            fTrue,
            NULL ) );

        wszDbFullName = g_rgfmp[ifmp].WszDatabaseName();
    }
    else
    {
        if ( NULL == wszDatabaseName || 0 == *wszDatabaseName )
        {
            return ErrERRCheck( JET_errDatabaseInvalidPath );
        }

        if ( BoolParam( pinst, JET_paramCreatePathIfNotExist ) )
        {
            //  make sure database name has a file at the end of it (no '\')
            //
            Assert( !FOSSTRTrailingPathDelimiterW( const_cast< WCHAR * >( wszDatabaseName ) ) );
            err = ErrUtilCreatePathIfNotExist( pfsapi, wszDatabaseName, rgwchDbFullName, sizeof(rgwchDbFullName) );
        }
        else
        {
            err = pfsapi->ErrPathComplete( wszDatabaseName, rgwchDbFullName );
        }
        CallR( err == JET_errInvalidPath ? ErrERRCheck( JET_errDatabaseInvalidPath ) : err );

        Assert( wszDbFullName == rgwchDbFullName );
        Assert( dbidTemp == dbidGiven || dbidMax == dbidGiven );
        err = FMP::ErrNewAndWriteLatch(
                &ifmp,
                wszDbFullName,
                ppib,
                pinst,
                pfsapi,
                dbidGiven,
                fTrue,
                !(grbit & JET_bitDbRecoveryOff) && !plog->FLogDisabled(),
                NULL );

        if ( err != JET_errSuccess )
        {
            if ( JET_wrnDatabaseAttached == err )
            {
                err = ErrERRCheck( JET_errDatabaseDuplicate );
            }
            return err;
        }

        dbidGiven = g_rgfmp[ ifmp ].Dbid();
    }

    //  from this point we got a valid ifmp entry. Start the DB creation process.
    //
    FMP *pfmp;
    pfmp = &g_rgfmp[ ifmp ];

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlCreateBegin|sysosrtlContextFmp, pfmp, &ifmp, sizeof(ifmp) );

    pfmp->m_isdlCreate.InitSequence( isdltypeCreate, eCreateSeqMax );

    OnDebug( Ptls()->fNoExtendingDuringCreateDB = fTrue );

    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->SetCreatingDB();
    pfmp->RwlDetaching().LeaveAsWriter();

    pfmp->SetRuntimeDbParams( cpgDatabaseSizeMax, pctCachePriority, NO_GRBIT, -1, 0, fFalse, -1 );

    //  check if database file already exists
    //
    if ( dbidGiven != dbidTemp )
    {
        err = ErrUtilPathExists( pfsapi, wszDbFullName );
        if ( JET_errFileNotFound != err )
        {
            if ( JET_errSuccess == err ) // database file exists
            {
                // delete db with the same name
                //
                if( grbit & JET_bitDbOverwriteExisting )
                {
                    err = ErrIODeleteDatabase( pfsapi, ifmp );
                }
                else
                {
                    err = ErrERRCheck( JET_errDatabaseDuplicate );
                }
            }
            Call( err );
        }
    }
    else
    {
        ERR errDelFile;

        errDelFile = ErrIODeleteDatabase( pfsapi, ifmp );
        if ( JET_errSuccess != errDelFile
            && JET_errFileNotFound != errDelFile
            && JET_errInvalidPath != errDelFile )
        {
            Assert( FRFSFailureDetected( OSFileDelete ) );
            Call( errDelFile );
        }
    }

    if ( ( dbidGiven != dbidTemp ) && !plog->FRecovering() )
    {
        // We are about to really create DB, let's check if an available version param is set

        const JET_ENGINEFORMATVERSION efv = (JET_ENGINEFORMATVERSION)UlParam( pinst, JET_paramEngineFormatVersion );
        Assert( efv >= EfvMinSupported() );
        if ( JET_efvUsePersistedFormat == efv )
        {
            // This value means the version is not set, and we use
            // the version from DB header. In case of creating DB,
            // we don't know which version to use.
            AssertSz( FNegTest( fInvalidAPIUsage ), "Specified JET_efvUsePersistedFormat during create DB, must have a specific version or JET_efvUseEngineDefault." );
            Call( ErrERRCheck( JET_errInvalidOperation ) );
        }
    }

    //  create an empty database with header only.
    //

    if ( grbit & JET_bitDbRecoveryOff )
    {
        pfmp->SetVersioningOff();
    }
    else
    {
        FMP::EnterFMPPoolAsWriter();
        // implicit createDB was done partially with log off, we need to redo it
        // with log off also
        if ( grbit & bitCreateDbImplicitly )
        {
            pfmp->ResetLogOn();
            pfmp->SetVersioningOff();
        }
        else
        {
            pfmp->SetLogOn();
        }
        FMP::LeaveFMPPoolAsWriter();
    }

    // The FMP's waypoint must be min at this point, or we might be able to flush pages we
    // shouldn't flush.
    DBEnforce( ifmp, CmpLgpos( pfmp->LgposWaypoint(), lgposMin ) == 0 );

    pfmp->SetDbtimeLast( grbit & bitCreateDbImplicitly ? 0 : dbtimeStart );

    pfmp->SetObjidLast( 0 );

    // Create & initialize & set database header

    {
    DBFILEHDR * pdbfilehdrInit = NULL;
    Alloc( pdbfilehdrInit = (DBFILEHDR_FIX * )PvOSMemoryPageAlloc( g_cbPage, NULL ) );

    InitDBDbfilehdr( pfmp, plog, grbit, psignDb, pdbfilehdrInit );

    OSTrace( JET_tracetagDatabases, OSFormat( "Create DB Version %d.%d (%d.%d) - %ws\n", (ULONG)pdbfilehdrInit->le_ulVersion, (ULONG)pdbfilehdrInit->le_ulDaeUpdateMajor, (ULONG)pdbfilehdrInit->le_ulCreateVersion, (ULONG)pdbfilehdrInit->le_ulCreateUpdate, wszDatabaseName ) );

    Assert( pdbfilehdrInit->le_objidLast == pfmp->ObjidLast() );
    Assert( pdbfilehdrInit->le_dbtimeDirtied == pfmp->DbtimeLast() );

    //  Note conflicts in matching signature DBs on the same instance are thrown 
    //  here (so I'm not so sure if the if body is dead code).
    DBFILEHDR * pdbfilehdrPrev;
    err = pfmp->ErrSetPdbfilehdr( pdbfilehdrInit, &pdbfilehdrPrev );
    if ( err < JET_errSuccess )
    {
        //  I think this call is dead code
        Assert( !pdbfilehdrPrev );
        OSMemoryPageFree( pdbfilehdrPrev );
        goto HandleError;
    }
    Assert( pdbfilehdrPrev == NULL );   // think this should hold ... or aren't we leaking a DBFILEHDR?
    } // end pdbfilehdrInit / FMP.pdbfilehdr initialization

    pfmp->SetDbtimeBeginRBS( grbit & bitCreateDbImplicitly ? 0 : dbtimeStart );

    if ( pfmp->PRBS() )
    {
        Call( pfmp->PRBS()->ErrRBSRecordDbAttach( pfmp ) );
        if ( pfmp->FRBSOn() )
        {
            Assert( 0 == pfmp->PRBS()->GetDbtimeForFmp( pfmp ) );
            Call( pfmp->PRBS()->ErrSetDbtimeForFmp( pfmp, pfmp->DbtimeBeginRBS() ) );
        }
    }

    pfmp->m_isdlCreate.Trigger( eCreateInitVariableDone );

    Call( ErrLGCreateDB( ppib, ifmp, grbit, fSparseEnabledFile, &lgposLogRec ) );
    fLogged = fTrue;

    { // .ctor acquires PdbfilehdrReadWrite
    PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
    if ( ( dbidGiven != dbidTemp ) && plog->FRecovering() )
    {
        Assert( 0 == CmpLgpos( plog->LgposLGLogTipNoLock(), pfmp->LgposAttach() ) );
        pdbfilehdr->le_lgposAttach = pfmp->LgposAttach();
    }
    else
    {
        pdbfilehdr->le_lgposAttach = lgposLogRec;
    }
    pdbfilehdr->le_lGenMinRequired = pdbfilehdr->le_lgposAttach.le_lGeneration;
    pdbfilehdr->le_lGenMaxRequired = pdbfilehdr->le_lgposAttach.le_lGeneration;
    pdbfilehdr->le_lGenMaxCommitted = pdbfilehdr->le_lgposAttach.le_lGeneration;
    pdbfilehdr->le_lGenMinConsistent = pdbfilehdr->le_lgposAttach.le_lGeneration;
    pdbfilehdr->le_lGenPreRedoMinRequired = 0;
    pdbfilehdr->le_lGenPreRedoMinConsistent = 0;

    Expected( pfmp->m_isdlCreate.FActiveSequence() );
    pfmp->m_isdlCreate.FixedData().sCreateData.lgposCreate = pdbfilehdr->le_lgposAttach;
    } // .dtor releases PdbfilehdrReadWrite

    pfmp->m_isdlCreate.Trigger( eCreateLogged );

    //  "Header load" for create DB is logically just the init and first write.
    OnDebug( pfmp->SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusHdrLoaded ) );

    //  write database header (and implicitly create file) for non-temp db
    //
    if ( dbidGiven != dbidTemp )
    {
        //  create a flush map for this DB
        //
        Call( pfmp->ErrCreateFlushMap( grbit ) );


        Call( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, wszDbFullName, pfmp ) );

        pfmp->m_isdlCreate.Trigger( eCreateHeaderInitialized );
    }

    if ( (BOOL)UlConfigOverrideInjection( 41882, fFalse ) == fTrue )
    {
        CHAR    szTrace[64];
        OSStrCbFormatA( szTrace, sizeof( szTrace ), "CreateDB Test Hook 41882 (dbid:%d)", pfmp->Dbid() );
        Call( ErrLGForceLogRollover( ppib, szTrace ) );
    }

    fNewDBCreated = fTrue;

    // Open db file for non-temp db (create for temp db)
    // Note that a temp db uses iofileDbRecovery during recovery and iofileDBAttached otherwise,
    // even though the temp DB isn't actually recovering anything.
    Call( ErrIOOpenDatabase( pfsapi, ifmp, wszDbFullName, pinst->FRecovering() ? iofileDbRecovery : iofileDbAttached, fSparseEnabledFile ) );

    if ( dbidGiven == dbidTemp )
    {
        //  write database header for temp db
        //
        Call( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, wszDbFullName, pfmp, pfmp->Pfapi() ) );

        pfmp->m_isdlCreate.Trigger( eCreateHeaderInitialized );
    }
 
    if ( pfmp->FLogOn() )
    {
        //  WARNING: even though we set the waypoint, waypoint
        //  advancement will not consider this database until
        //  FMP::SetAttached() is called below
        //
        FMP::EnterFMPPoolAsWriter();
        const LONG lgenMaxRequired = pfmp->Pdbfilehdr()->le_lGenMaxRequired;
        pfmp->SetWaypoint( lgenMaxRequired );
        FMP::LeaveFMPPoolAsWriter();
    }
    else
    {
        pfmp->ResetWaypoint(); // no logging, means no waypoint ...
    }

    //  set proper database size.
    //
    {
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    err = ErrIONewSize( ifmp, *tcScope, cpgPrimary, 0, JET_bitNil );
    }

    if ( err < JET_errSuccess )
    {
        //  we have failed to do a "big" allocation
        //  drop down to small allocations and see if we can succeed

        const INT cbExtend  = g_cbPage;
        const CPG cpgExtend = cbExtend / g_cbPage;

        CPG cpgT            = 0;

        do
        {
            cpgT += cpgExtend;
            PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
            Call( ErrIONewSize( ifmp, *tcScope, cpgT, 0, JET_bitNil ) );
        } while( cpgT < cpgPrimary );

    }

    pfmp->SetOwnedFileSize( CbFileSizeOfCpg( cpgPrimary ) );

    //  initialize the database file.
    //
    DBSetOpenDatabaseFlag( ppib, ifmp );
    fDatabaseOpen = fTrue;

    pfmp->m_isdlCreate.Trigger( eCreateDatabaseZeroed );

    // for implicitly created DBs we must continue on.
    // for logged create DBs we bail out here, because the log has the log
    // records for:
    //   - ErrDBInitDatabase()
    //   - ErrCATCreate()
    //   - and finally one to trigger ErrDBCreateDBFinish().
    //
    if ( ( dbidGiven != dbidTemp ) &&
         plog->FRecovering() &&
         !( grbit & bitCreateDbImplicitly ) )
    {
        OnDebug( Ptls()->fNoExtendingDuringCreateDB = fFalse );

        *pifmp = ifmp;
        return JET_errSuccess;
    }

    //  setup and log root space
    //
    Call( ErrDBInitDatabase( ppib, ifmp, cpgPrimary ) );

    pfmp->m_isdlCreate.Trigger( eCreateDatabaseSpaceInitialized );

    if ( dbidGiven != dbidTemp )
    {
        //  create system tables
        //
        const BOOL fReplayCreateDbImplicitly =  plog->FRecovering() && ( grbit & bitCreateDbImplicitly );
        Call( ErrCATCreate( ppib, ifmp, fReplayCreateDbImplicitly ) );
    }

    pfmp->m_isdlCreate.Trigger( eCreateDatabaseCatalogInitialized );

    AssertDIRNoLatch( ppib );

    if ( !pinst->FRecovering() )
    {
        Call( ErrDBUpdateAndFlushVersion(
                    pinst,
                    wszDbFullName,
                    ppib,
                    ifmp,
                    fTrue ) );
    }

    Call( ErrDBCreateDBFinish( ppib, ifmp, grbit ) );

    // Create this *before* creating the other system tables (such as
    // MSysLocales) so that MSObjids contains entries for those tables.
    if( dbidGiven != dbidTemp && !pinst->FRecovering() )
    {
        Call( ErrCATCreateMSObjids( ppib, ifmp ) );
        Call( ErrCATPopulateMSObjids( ppib, ifmp ) );
        pfmp->SetFMaintainMSObjids();
    }

    //  create the MSysLocales table
    //
    if( dbidGiven != dbidTemp && !pinst->FRecovering() )
    {
        //  create the MSysLocales table (and init the facility)

        Call( ErrCATCreateMSLocales( ppib, ifmp ) );
        Assert( pfmp->PkvpsMSysLocales() );
        Assert( JET_errDatabaseCorrupted != ErrCATVerifyMSLocales( ppib, ifmp, fFalse /* fixup table */ ) );

        //  Note: We do not need to populate because there are no localized unicode indices
        //  in the initial database.  Very convenient.

        //  finally upgrade the DB header to indicate we're using the MSysLocales table

        pfmp->PdbfilehdrUpdateable()->le_qwSortVersion = g_qwUpgradedLocalesTable;

        Call( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ) );

        Assert( pfmp->Pdbfilehdr()->le_qwSortVersion == g_qwUpgradedLocalesTable );
    }

    // Turn off the switch saying we don't expect to extend the DB during create.  We might extend the DB
    // if we need to create the ExtentPageCountCache.
    OnDebug( Ptls()->fNoExtendingDuringCreateDB = fFalse );

    // Create the MSysExtentPageCountCache table, if necessary.  We do this after MSysLocales so that
    // MSysLocales has it's historic objid, which makes a number of existing tests continue to work.
    if( dbidGiven != dbidTemp && !pinst->FRecovering() )
    {
        PGNO  pgnoFDP;
        OBJID objidFDP;
        const WCHAR * rgwsz[] = { pfmp->WszDatabaseName(), NULL };

        MessageId evtId = PLAIN_TEXT_ID;                   // No-logging marker value.
        FEATURECONTROL fcT = FEATURECONTROL::fcNotSpecified; // Assume nothing to do.

        pgnoFDP = pgnoNull;
        objidFDP = objidNil;

        switch ( fcMaintainExtentPageCountCache )
        {
            case FEATURECONTROL::fcNotSpecified:
                // On DB create, since you didn't specify you wanted the table, we just won't make it.
                // We also don't log that.
                break;

            case FEATURECONTROL::fcDisableFromParam:
                evtId = EXTENT_PAGE_COUNT_CACHE_NOT_IN_USE_ID;
                rgwsz[1] = L"REQUEST_PARAM";
                break;

            case FEATURECONTROL::fcDisableFromOverride:
                evtId = EXTENT_PAGE_COUNT_CACHE_NOT_IN_USE_ID;
                rgwsz[1] = L"OVERRIDE_PARAM";
                break;

            case FEATURECONTROL::fcEnableFromParam:
                evtId = EXTENT_PAGE_COUNT_CACHE_IN_USE_ID;
                fcT = FEATURECONTROL::fcEnableFromParam;
                rgwsz[1] = L"REQUEST_PARAM";
                break;

            case FEATURECONTROL::fcEnableFromOverride:
                if ( pfmp->FEfvSupported( JET_efvExtentPageCountCache ) )
                {
                    // Go ahead, honor the override.
                    evtId = EXTENT_PAGE_COUNT_CACHE_IN_USE_ID;
                    fcT = FEATURECONTROL::fcEnableFromParam;
                    rgwsz[1] = L"OVERRIDE_PARAM";
                }
                else
                {
                    // Ignore the override if the efv doesn't support it.
                    evtId = EXTENT_PAGE_COUNT_CACHE_NOT_IN_USE_ID,
                    rgwsz[1] = L"UNSUPPORTED_OVERRIDE_PARAM";
                }
                break;

            default:
                AssertSz( fFalse, "Unexpected case in switch.");
                Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        if ( evtId != PLAIN_TEXT_ID )
        {
            UtilReportEvent(
                eventInformation,
                SPACE_MANAGER_CATEGORY,
                evtId,
                _countof( rgwsz ),
                rgwsz,
                0,
                NULL,
                pinst );
        }
        
        switch ( fcT )
        {
            case FEATURECONTROL::fcEnableFromParam:
                Assert( objidNil == objidFDP );
                Call( ErrCATCreateMSExtentPageCountCache( ppib, ifmp, &pgnoFDP, &objidFDP ) );
                Assert( objidNil != objidFDP );
                break;

            default:
                Assert( FEATURECONTROL::fcNotSpecified == fcT );
                break;
        }

        g_rgfmp[ ifmp ].SetExtentPageCountCacheTableInfo( pgnoFDP, objidFDP );
    }

    pfmp->m_isdlCreate.Trigger( eCreateLatentUpgradesDone );

    if ( 0 != ( grbit & JET_bitDbEnableBackgroundMaintenance ) )
    {
        err = pfmp->ErrStartDBMScan();
        if( err < JET_errSuccess )
        {
            const ERR errDetach = ErrIsamDetachDatabase( (JET_SESID) ppib, NULL, wszDatabaseName );
            if ( ( errDetach >= JET_errSuccess ) )
            {
                Expected( !FIODatabaseOpen( ifmp ) ); // added during FFB feature, b/c I(SOMEONE) didn't understand how on successful detach is there a DB Pfapi() left?
                if ( FIODatabaseOpen( ifmp ) )
                {
                    Assert( g_rgfmp[ifmp].Pfapi()->CioNonFlushed() == 0 );    // must hold - or leaking IOs.
                    IOCloseDatabase( ifmp );
                }

                //  best effort.
                //
                (VOID)ErrIODeleteDatabase( pfsapi, ifmp );
            }

            return err;
        }
    }

    if ( !g_fRepair &&
        !g_fEseutil &&
        g_fPeriodicTrimEnabled &&
        !pfmp->FIsTempDB() &&
        !pfmp->FReadOnlyAttach() )
    {
        CallS( ErrSPTrimDBTaskInit( ifmp ) );
    }

    pfmp->m_isdlCreate.Trigger( eCreateDone );

    //  we won't log the temp DB

    if ( !pfmp->FIsTempDB() )
    {
        // report our successful create database and timings

        DBReportTachmentEvent( pinst, ifmp, CREATE_DATABASE_DONE_ID, pfmp->WszDatabaseName(), fFalse );
    }
    else
    {
        pfmp->m_isdlCreate.TermSequence();  //  cleanup for temp DB as well.
    }

    *pifmp = ifmp;

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlCreateSucceed|sysosrtlContextFmp, pfmp, &ifmp, sizeof(ifmp) );

    return JET_errSuccess;

HandleError:

    ULONG rgul[4] = { (ULONG)ifmp, (ULONG)err, PefLastThrow()->UlLine(), UlLineLastCall() };
    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlAttachFail|sysosrtlContextFmp, pfmp, rgul, sizeof(rgul) );

    Assert( err < JET_errSuccess );
    Assert( pfmp != NULL );

    pfmp->m_isdlCreate.TermSequence();

    CATTermMSLocales( pfmp );

    //  purge the bad, half-created database
    //

    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->ResetCreatingDB();
    pfmp->SetDetachingDB();
    pfmp->RwlDetaching().LeaveAsWriter();

    BOOL fLeakFMP = fFalse;
    if ( !FFMPIsTempDB( ifmp ) )
    {
        const ERR errVERClean = PverFromIfmp( ifmp )->ErrVERRCEClean( ifmp ) ;
        fLeakFMP = ( errVERClean != JET_errSuccess );
    }

    if ( fLogged || fLeakFMP )
    {
        //  we have to take the instance offline if there is an error
        //  during database creation and the creation is logged or we could
        //  not clean version store entries for the database
        //
        PinstFromPpib( ppib )->SetInstanceUnavailable( err );
        if ( JET_errSuccess == ppib->ErrRollbackFailure() )
        {
            ppib->SetErrRollbackFailure( err );
        }

        // we need to clean the FCBs but because there might be RCEs
        // not expected (as a result of MSU done versioned), we
        // need to tell the FCB cleanup to ignore versions.
        // We can do this becase the instance is already unavailable
        //
        FCB::PurgeDatabase( ifmp, fTrue /* fTerminating */ );
    }
    else
    {
        FCB::PurgeDatabase( ifmp, fFalse /* fTerminating */ );
    }

    BFPurge( ifmp );

    g_rgfmp[ ifmp ].AssertRangeLockEmpty();

    if ( FIODatabaseOpen( ifmp ) )
    {
        (void)ErrIOFlushDatabaseFileBuffers( ifmp, iofrFlushIfmpContext );
        IOCloseDatabase( ifmp );
    }

    if ( fNewDBCreated )
    {
        (VOID)ErrIODeleteDatabase( pfsapi, ifmp );
        if ( fLogged )
        {
            Assert( UtilCmpFileName( wszDbFullName, pfmp->WszDatabaseName() ) == 0 );
        }
    }

    if ( fDatabaseOpen )
    {
        DBResetOpenDatabaseFlag( ppib, ifmp );
    }

    FMP::EnterFMPPoolAsWriter();
    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->ResetDetachingDB();
    if ( pfmp->Pdbfilehdr() )
    {
        pfmp->FreePdbfilehdr();
    }

    pfmp->DestroyFlushMap();

    // The latch can be released in ErrDBCreateDBFinish function.
    BOOL fWriteLatched = fFalse;
    pfmp->CritLatch().Enter();
    fWriteLatched = pfmp->FWriteLatchByMe( ppib );
    pfmp->CritLatch().Leave();
    if ( fWriteLatched )
    {
        if ( fLeakFMP )
        {
            pfmp->ReleaseWriteLatch( ppib );
        }
        else
        {
            pfmp->ReleaseWriteLatchAndFree( ppib );
        }
    }

    pfmp->RwlDetaching().LeaveAsWriter();
    FMP::LeaveFMPPoolAsWriter();

    OnDebug( Ptls()->fNoExtendingDuringCreateDB = fFalse );

    return err;
}

ERR
ErrDBCreateDBFinish(
    PIB *           ppib,
    IFMP            ifmp,
    const ULONG     grbit
    )
{
    ERR err;
    FMP *pfmp = &g_rgfmp[ ifmp ];
    INST *pinst = PinstFromPpib( ppib );
    LOG *plog = pinst->m_plog;
    LGPOS lgposLogRec;
    LONG lgenCurrent = 0;
    LOGTIME tmCurrent;
    
    memset( &tmCurrent, 0, sizeof( tmCurrent ) );

    CheckPIB( ppib );

    if ( ppib->Level() > 0 )
    {
        return ErrERRCheck( JET_errInTransaction );
    }

    Call( ErrLGCreateDBFinish( ppib, ifmp, &lgposLogRec ) );

    if ( pfmp->FLogOn() && BoolParam( pinst, JET_paramAggressiveLogRollover ) )
    {
        CHAR    szTrace[ 32 ];
        OSStrCbFormatA( szTrace, sizeof( szTrace ), "CreateDB (dbid:%d)", pfmp->Dbid() );
        Call( ErrLGForceLogRollover( ppib, szTrace ) );
    }

    if ( !plog->FLogDisabled() )
    {
        lgenCurrent = plog->LGGetCurrentFileGenWithLock( &tmCurrent );
    }

    pfmp->m_isdlCreate.Trigger( eCreateDatabaseInitialized );

    //  set database status to loggable if it is.
    //
    if ( grbit & JET_bitDbRecoveryOff )
    {
        Assert( !pfmp->FLogOn() );
        Assert( !pfmp->FReadOnlyAttach() );
        if ( !( grbit & JET_bitDbVersioningOff ) )
        {
            pfmp->ResetVersioningOff();
        }
    }
    else
    {
        Assert( !( grbit & JET_bitDbVersioningOff ) );

        // Now turn on logging for implicit createDB
        if ( ( grbit & bitCreateDbImplicitly ) )
        {
            pfmp->ResetVersioningOff();

            FMP::EnterFMPPoolAsWriter();
            pfmp->SetLogOn();
            FMP::LeaveFMPPoolAsWriter();
        }
    }

    { // .ctor acquires PdbfilehdrReadWrite
    PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
    pdbfilehdr->le_dbtimeDirtied = pfmp->DbtimeLast();
    LGIGetDateTime( &pdbfilehdr->logtimeAttach );

    pfmp->m_isdlCreate.Trigger( eCreateFinishLogged );

    //  set database state to be inconsistent from creating since
    //  the createdatabase op is logged.
    //
    pdbfilehdr->le_objidLast = pfmp->ObjidLast();

    Assert( pdbfilehdr->Dbstate() == JET_dbstateJustCreated );

    if ( !plog->FLogDisabled() && pfmp->FLogOn() )
    {
        pdbfilehdr->SetDbstate( JET_dbstateDirtyShutdown, pdbfilehdr->le_lgposAttach.le_lGeneration, lgenCurrent, &tmCurrent, !plog->FLogDisabled() );
    }
    else
    {
        pdbfilehdr->SetDbstate( JET_dbstateDirtyShutdown );
    }

    Expected( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) == 0 );
    if ( ( pfmp->ErrDBFormatFeatureEnabled( pdbfilehdr.get(), JET_efvLgposLastResize ) == JET_errSuccess ) &&
         ( CmpLgpos( pdbfilehdr->le_lgposAttach, pdbfilehdr->le_lgposLastResize ) > 0 ) )
    {
        pdbfilehdr->le_lgposLastResize = pdbfilehdr->le_lgposAttach;
    }

    //  set version info
    //
    if ( pfmp->FIsTempDB() || !pinst->FRecovering() )
    {
        pdbfilehdr->le_dwMajorVersion = g_dwGlobalMajorVersion;
        pdbfilehdr->le_dwMinorVersion = g_dwGlobalMinorVersion;
        pdbfilehdr->le_dwBuildNumber = g_dwGlobalBuildNumber;
        pdbfilehdr->le_lSPNumber = g_lGlobalSPNumber;
        //  we no longer track the default sort version in the header
        //
        //  during initial create we'll use the default sort version, and upgrade to 
        //  the MSysLocales table version / approach later.  This is necessary so
        //  that we don't alter the database generated by lrtypCreateDB, so that
        //  we can replay old logs and end up with the exact same database after
        //  CreateDB is done (nesc. b/c create DB is an unlogged operation).
        QWORD qwSortVersion;
        CallS( ErrNORMGetSortVersion( wszLocaleNameDefault, &qwSortVersion, NULL ) );
        pdbfilehdr->le_qwSortVersion = qwSortVersion;
    }
    Assert( pdbfilehdr->le_objidLast );
    } // .dtor releases PdbfilehdrReadWrite

    //  we do not update the header for the temp database

    if ( !pfmp->FIsTempDB() )
    {
        Call( ErrUtilWriteAttachedDatabaseHeaders( pinst, pinst->m_pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ) );

        DBEnforce( ifmp, pfmp->LgposWaypoint().lGeneration <= pfmp->Pdbfilehdr()->le_lGenMaxRequired );
    }

    if ( pfmp->FLogOn() )
    {
        //  WARNING: even though we set the waypoint, waypoint
        //  advancement will not consider this database until
        //  FMP::SetAttached() is called below
        //
        FMP::EnterFMPPoolAsWriter();
        pfmp->SetWaypoint( lgenCurrent );
        FMP::LeaveFMPPoolAsWriter();
    }

    if ( !pfmp->FIsTempDB() )
    {
        //  flush buffers
        //
        Call( ErrBFFlush( ifmp ) );
    }

    pfmp->m_isdlCreate.Trigger( eCreateDatabaseDirty );

    //  make the database attached
    //
    Assert( !( pfmp->FAttached() ) );
    FMP::EnterFMPPoolAsWriter();
    pfmp->SetAttached();
    FMP::LeaveFMPPoolAsWriter();

    pfmp->InitializeDbtimeOldest();

    Assert( !pfmp->FAllowHeaderUpdate() );
    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->SetAllowHeaderUpdate();
    pfmp->RwlDetaching().LeaveAsWriter();

    Call( ErrDBSetLastPage( ppib, ifmp ) );

    //  the database is created and attached.
    //  Make the fmp available for others to open etc.
    //  no need to wrl since it is OK for reader to mistaken it is being created
    //
    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->ResetCreatingDB();
    pfmp->ReleaseWriteLatch( ppib );
    pfmp->RwlDetaching().LeaveAsWriter();

    // Fire attached callback
    if ( !pfmp->FIsTempDB() && pinst->FRecovering() )
    {
        Call( ErrLGDbAttachedCallback( pinst, pfmp ) );
    }

    // Doing return JET_errSucces rather than Error( JET_errSuccess) below because the
    // caller is responsible for calling pfmp->m_isdlCreate.TermSequence() when it's
    // truly done.  It's too early here in the success cases we return on.
    
    // Don't need to do the following tasks if opening a temp DB.
    if ( pfmp->FIsTempDB() )
    {
        return JET_errSuccess;
    }

    // only recovery with non-implicit CreateDB needs to do the following tasks
    // here
    if ( !plog->FRecovering() ||
         grbit & bitCreateDbImplicitly )
    {
        return JET_errSuccess;
    }
        
    if ( 0 != ( grbit & JET_bitDbEnableBackgroundMaintenance ) )
    {
        err = pfmp->ErrStartDBMScan();
        if( err < JET_errSuccess )
        {
            const ERR errDetach = ErrIsamDetachDatabase( (JET_SESID) ppib, NULL, pfmp->WszDatabaseName() );
            if ( ( errDetach >= JET_errSuccess ) )
            {
                Expected( !FIODatabaseOpen( ifmp ) ); // added during FFB feature, b/c I(SOMEONE) didn't understand how on successful detach is there a DB Pfapi() left?
                if ( FIODatabaseOpen( ifmp ) )
                {
                    Assert( g_rgfmp[ifmp].Pfapi()->CioNonFlushed() == 0 );    // must hold - or leaking IOs.
                    IOCloseDatabase( ifmp );
                }
                
                //  best effort.
                //
                (VOID)ErrIODeleteDatabase( pinst->m_pfsapi, ifmp );
            }

            // Error handling in the preceding if statment already called m_isdlCreate.TermSequence()
            // (the only thing HandleError does).  Since I (SOMEONE) don't know if TermSequence is safely
            // idempotent, just "return err;" rather than "Error( err );"
            return err;
        }
    }

    if ( !g_fRepair &&
         !g_fEseutil &&
         g_fPeriodicTrimEnabled &&
         !pfmp->FIsTempDB() &&
         !pfmp->FReadOnlyAttach() )
    {
        CallS( ErrSPTrimDBTaskInit( ifmp ) );
    }

    //  validate the created "and attached" database ... 

    Assert( pfmp->Pdbfilehdr()->le_ulVersion != 0 );
    Assert( pfmp->Pdbfilehdr()->le_ulDaeUpdateMajor != 0 );

    pfmp->m_isdlCreate.Trigger( eCreateDone );

    //  we won't log the temp DB

    if ( !pfmp->FIsTempDB() )
    {
        // report our successful create database and timings

        DBReportTachmentEvent( pinst, ifmp, CREATE_DATABASE_DONE_ID, pfmp->WszDatabaseName(), fFalse );
    }

HandleError:

    pfmp->m_isdlCreate.TermSequence();

    return err;
}

ERR ErrDBReadHeaderCheckConsistency(
    IFileSystemAPI  * const pfsapi,
    FMP             * pfmp )
{
    ERR             err;
    DBFILEHDR       * pdbfilehdr;
    IFileAPI        * pfapi;

    //  bring in the database and check its header
    //
    AllocR( pdbfilehdr = (DBFILEHDR * )PvOSMemoryPageAlloc( g_cbPage, NULL ) );

    //  need to zero out header because we try to read it
    //  later even on failure
    memset( pdbfilehdr, 0, g_cbPage );

    Call( CIOFilePerf::ErrFileOpen(
                            pfsapi,
                            pfmp->Pinst(),
                            pfmp->WszDatabaseName(),
                            pfmp->FmfDbDefault(),
                            iofileDbAttached,
                            pfmp - g_rgfmp,
                            &pfapi ) );

    Assert( pfmp->FInUse() );

    err = ErrUtilReadShadowedHeader(    pfmp->Pinst(),
                                        pfsapi,
                                        pfapi,
                                        (BYTE*)pdbfilehdr,
                                        g_cbPage,
                                        OffsetOf( DBFILEHDR, le_cbPageSize ),
                                        pfmp->FReadOnlyAttach() ? urhfReadOnly : urhfNone );

    delete pfapi;
    pfapi = NULL;

    if ( err < JET_errSuccess )
    {
        //  600 use new checksum method, so read shadow will fail with JET_errReadVerifyFailure
        if ( JET_errReadVerifyFailure == err )
        {
            if (    ulDAEMagic == pdbfilehdr->le_ulMagic
                 && ulDAEVersion500 == pdbfilehdr->le_ulVersion )
            {
                //  500 has different way of doing checksum. Let's check the version directly.
                //  The magic number stays the same since 500

                err = ErrERRCheck( JET_errDatabase500Format );
            }

            else
            {
                err = ErrDBICheck200And400( pfmp->Pinst(), pfsapi, pfmp->WszDatabaseName() );

                if ( err != JET_errDatabase200Format && err != JET_errDatabase400Format )
                {
                    if ( pdbfilehdr->le_ulMagic == ulDAEMagic )
                        err = ErrERRCheck( JET_errInvalidDatabaseVersion );
                    else
                        err = ErrERRCheck( JET_errDatabaseCorrupted );
                }
            }
            OSUHAEmitFailureTag( pfmp->Pinst(), HaDbFailureTagCorruption, L"f2670764-592f-47a1-a5ce-2d706cc88b8a" );
        }

        goto HandleError;
    }

    //  the version and update numbers should always increase
    Assert( ulDAEVersionMax >= ulDAEVersionESE97 );

    //  do version check
    if ( ulDAEMagic != pdbfilehdr->le_ulMagic )
    {
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }
    else if ( pdbfilehdr->le_ulVersion >= ulDAEVersionESE97 )
    {
        //  if the database format needs to be upgraded we will do it after
        //  attaching
        err = JET_errSuccess;
    }
    else
    {
        Assert( pdbfilehdr->le_ulVersion < ulDAEVersionMax );
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }

    //  do pagesize check
    if ( ( 0 == pdbfilehdr->le_cbPageSize && g_cbPageDefault != g_cbPage )
            || ( 0 != pdbfilehdr->le_cbPageSize && pdbfilehdr->le_cbPageSize != (ULONG)g_cbPage ) )
        {
            OSUHAEmitFailureTag( pfmp->Pinst(), HaDbFailureTagConfiguration, L"4ee2aa4e-3940-403e-8475-73099917129d" );
            Call( ErrERRCheck( JET_errPageSizeMismatch ) );
        }

    DWORD cbSectorSize;
    Call( pfsapi->ErrFileAtomicWriteSize( pfmp->WszDatabaseName(), &cbSectorSize ) );
    Assert( FPowerOf2( cbSectorSize ) );
    if ( g_cbPage % cbSectorSize != 0 )
    {
        WCHAR szDiskSectorSize[16];
        OSStrCbFormatW( szDiskSectorSize, sizeof( szDiskSectorSize ), L"%d", cbSectorSize );
        WCHAR szDbPageSize[16];
        OSStrCbFormatW( szDbPageSize, sizeof( szDbPageSize ), L"%d", g_cbPage );
        const WCHAR *rgszT[3];

        rgszT[0] = pfmp->WszDatabaseName();
        rgszT[1] = szDiskSectorSize;
        rgszT[2] = szDbPageSize;

        //  The sector size is greater than database page size, may result in
        //  durability being lost
        UtilReportEvent(
                eventWarning,
                GENERAL_CATEGORY,
                DB_SECTOR_SIZE_MISMATCH,
                _countof( rgszT ),
                rgszT,
                0,
                NULL,
                pfmp->Pinst() );
    }

    //  check for LV chunk size incompatabilities

    if( !FDBIsLVChunkSizeCompatible( pdbfilehdr->le_cbPageSize, pdbfilehdr->le_ulVersion, pdbfilehdr->le_ulDaeUpdateMajor ) )
    {
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }

    if ( !g_fRepair )
    {
        if ( pdbfilehdr->Dbstate() != JET_dbstateCleanShutdown )
        {
            if ( pdbfilehdr->Dbstate() == JET_dbstateBeingConverted )
            {
                const WCHAR *rgszT[1];

                rgszT[0] = pfmp->WszDatabaseName();

                //  attempting to use a database which did not successfully
                //  complete conversion
                UtilReportEvent(
                        eventError,
                        CONVERSION_CATEGORY,
                        CONVERT_INCOMPLETE_ERROR_ID,
                        1,
                        rgszT,
                        0,
                        NULL,
                        pfmp->Pinst() );
                err = ErrERRCheck( JET_errDatabaseIncompleteUpgrade );
            }
            else if ( pdbfilehdr->Dbstate() == JET_dbstateIncrementalReseedInProgress )
            {
                OSUHAEmitFailureTag( pfmp->Pinst(), HaDbFailureTagCorruption, L"8d6a36f9-69ab-4348-9d69-52fdbcdea7d8" );
                err = ErrERRCheck( JET_errDatabaseIncompleteIncrementalReseed );
            }
            else if ( pdbfilehdr->Dbstate() == JET_dbstateRevertInProgress )
            {
                OSUHAEmitFailureTag( pfmp->Pinst(), HaDbFailureTagCorruption, L"55809f5e-fbba-4272-9029-ab9c7585c08d" );
                err = ErrERRCheck( JET_errDatabaseIncompleteRevert );
            }
            else
            {
                // we want to return a specific error if the database is from a backup set and not recovered
                if ( 0 != pdbfilehdr->bkinfoFullCur.le_genLow )
                {
                    const WCHAR *rgszT[1];

                    rgszT[0] = pfmp->WszDatabaseName();

                    //  attempting to use a database which did not successfully
                    //  complete conversion
                    UtilReportEvent(
                            eventError,
                            LOGGING_RECOVERY_CATEGORY,
                            ATTACH_TO_BACKUP_SET_DATABASE_ERROR_ID,
                            1,
                            rgszT,
                            0,
                            NULL,
                            pfmp->Pinst() );
                    
                    err = ErrERRCheck( JET_errSoftRecoveryOnBackupDatabase );
                }
                else
                {
                    // Note: We'll return this for both JET_dbstateDirtyShutdown and JET_dbstateDirtyAndPatchedShutdown ...
                    err = ErrERRCheck( JET_errDatabaseInconsistent );
                }
            }
            goto HandleError;
        }
    }

    // make sure the attached file is a database file
    if ( attribDb != pdbfilehdr->le_attrib
        || ( JET_filetypeUnknown != pdbfilehdr->le_filetype
            && JET_filetypeDatabase != pdbfilehdr->le_filetype
            && JET_filetypeTempDatabase != pdbfilehdr->le_filetype ) )
    {
        const WCHAR *rgszT[1];
        rgszT[0] = pfmp->WszDatabaseName();

        UtilReportEvent(
                eventError,
                DATABASE_CORRUPTION_CATEGORY,
                DATABASE_HEADER_ERROR_ID,
                1,
                rgszT,
                0,
                NULL,
                pfmp->Pinst() );
        
        OSUHAPublishEvent(  HaDbFailureTagCorruption,
                            pfmp->Pinst(),
                            HA_DATABASE_CORRUPTION_CATEGORY,
                            HaDbIoErrorNone, NULL, 0, 0,
                            HA_DATABASE_HEADER_ERROR_ID,
                            1,
                            rgszT );

        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    Assert( pfmp->Pdbfilehdr() == NULL );
    DBFILEHDR * pdbfilehdrPrev;
    Call( pfmp->ErrSetPdbfilehdr( pdbfilehdr, &pdbfilehdrPrev ) );
    Assert( NULL == pdbfilehdrPrev );
    pfmp->SetDbtimeLast( pdbfilehdr->le_dbtimeDirtied );
    
    if( pfmp->PRBS() )
    {
        Call( pfmp->PRBS()->ErrRBSRecordDbAttach( pfmp ) );
    }

    if ( pfmp->FRBSOn() )
    {
        DBTIME dbtimeSnapshot = pfmp->PRBS()->GetDbtimeForFmp( pfmp );
        if ( dbtimeSnapshot == 0 )
        {
            dbtimeSnapshot = pfmp->DbtimeGet();
            Call( pfmp->PRBS()->ErrSetDbtimeForFmp( pfmp, dbtimeSnapshot ) );
        }
        pfmp->SetDbtimeBeginRBS( dbtimeSnapshot );
    }

    Assert( pfmp->Pdbfilehdr()->le_dbtimeDirtied != 0 );
    pfmp->SetObjidLast( pdbfilehdr->le_objidLast );
    Assert( pfmp->Pdbfilehdr()->le_objidLast != 0 );

    OnDebug( pfmp->SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusHdrLoaded ) );

    // Set lGenMinConsistent back to lGenMinRequired because that's our effective initial checkpoint.
    // In the future, this can be delayed until a DB or flush map page gets dirtied.
    if ( !pfmp->FReadOnlyAttach() )
    {
        PdbfilehdrReadWrite pdbfilehdrUpdateable = pfmp->PdbfilehdrUpdateable();
        Expected( g_fRepair ||
                  ( ( pdbfilehdrUpdateable->Dbstate() == JET_dbstateCleanShutdown ) &&
                    ( pdbfilehdrUpdateable->le_lGenMinConsistent == 0 ) &&
                    ( pdbfilehdrUpdateable->le_lGenMinRequired == 0 ) &&
                    ( pdbfilehdrUpdateable->le_lGenPreRedoMinRequired == 0 ) &&
                    ( pdbfilehdrUpdateable->le_lGenPreRedoMinConsistent == 0 ) ) );
        pdbfilehdrUpdateable->le_lGenMinConsistent = pdbfilehdrUpdateable->le_lGenMinRequired;
    }

    OSTrace( JET_tracetagDatabases, OSFormat( "Loaded DB Version (%p): %d.%d.%d (%d) - %ws\n", pdbfilehdr, (ULONG)pdbfilehdr->le_ulVersion, (ULONG)pdbfilehdr->le_ulDaeUpdateMajor, (ULONG)pdbfilehdr->le_ulDaeUpdateMinor, (ULONG)pdbfilehdr->le_ulCreateVersion, pfmp->WszDatabaseName() ) );

    return JET_errSuccess;

HandleError:
    Assert( err < JET_errSuccess );
    Assert( NULL != pdbfilehdr );
    OSMemoryPageFree( (VOID *)pdbfilehdr );
    return err;
}

VOID DBISetHeaderAfterAttach(
    DBFILEHDR_FIX * const   pdbfilehdr,
    const LGPOS             lgposAttach,
    const LOGTIME * const   plogtimeOfGenWithAttach,
    const IFMP              ifmp,
    const BOOL              fKeepBackupInfo )
{
    LOG * const             plog    = PinstFromIfmp( ifmp )->m_plog;

    //  Update database file header.

    Assert( fRecoveringUndo != plog->FRecoveringMode() );
    if ( plog->FRecovering() )
    {
        pdbfilehdr->le_lGenRecovering = lgposAttach.lGeneration;
    }
    else
    {
        pdbfilehdr->le_lGenRecovering = 0;
    }

    if ( pdbfilehdr->Dbstate() != JET_dbstateDirtyShutdown )
    {
        Assert( ( pdbfilehdr->Dbstate() == JET_dbstateCleanShutdown ) ||
                ( g_fRepair && ( pdbfilehdr->Dbstate() == JET_dbstateIncrementalReseedInProgress || pdbfilehdr->Dbstate() == JET_dbstateRevertInProgress ) ) ||
                ( plog->FRecovering() && ( plog->FRecoveringMode() == fRecoveringRedo ) && ( pdbfilehdr->Dbstate() == JET_dbstateDirtyAndPatchedShutdown ) ) );

        const BOOL fPendingRedoMapEntries = !g_rgfmp[ ifmp ].FRedoMapsEmpty();
        Assert( !fPendingRedoMapEntries || ( pdbfilehdr->Dbstate() == JET_dbstateDirtyAndPatchedShutdown ) );

        if ( !fPendingRedoMapEntries )
        {
            if ( plog->FLogDisabled() )
            {
                Assert( pdbfilehdr->Dbstate() != JET_dbstateDirtyAndPatchedShutdown );
                pdbfilehdr->SetDbstate( JET_dbstateDirtyShutdown );
            }
            else
            {
                LOGTIME tmCreate;
                LONG lgenCurrent = plog->LGGetCurrentFileGenNoLock( &tmCreate );

                DBEnforce( ifmp, g_rgfmp[ ifmp ].LgposWaypoint().lGeneration <= pdbfilehdr->le_lGenMaxRequired );
                DBEnforce( ifmp, g_rgfmp[ ifmp ].LgposWaypoint().lGeneration <= lgenCurrent );


                if ( plog->FRecovering() )
                {
                    if ( JET_dbstateDirtyAndPatchedShutdown == pdbfilehdr->le_dbstate &&
                         lgposAttach.lGeneration >= pdbfilehdr->le_lGenMaxRequired )
                    {
                        OSTrace(
                            JET_tracetagDatabases,
                            OSFormat(
                                "Replayed attach of database=['%ws':0x%x] in DirtyAndPatched state with max-required gen of 0x%x at lgpos [%08x:%04x:%04x], moving to dirty state",
                                g_rgfmp[ifmp].WszDatabaseName(),
                                (ULONG)ifmp,
                                (LONG)pdbfilehdr->le_lGenMaxRequired,
                                lgposAttach.lGeneration,
                                lgposAttach.isec,
                                lgposAttach.ib ) );
                    }

                    if ( JET_dbstateDirtyAndPatchedShutdown != pdbfilehdr->le_dbstate ||
                         lgposAttach.lGeneration >= pdbfilehdr->le_lGenMaxRequired )
                    {
                        pdbfilehdr->SetDbstate( JET_dbstateDirtyShutdown, lgenCurrent, &tmCreate, !plog->FLogDisabled() );
                    }
                    // else, we maintain / leave the patched state at JET_dbstateDirtyAndPatchedShutdown.
                }
                else
                {
                    DBEnforce( ifmp, JET_dbstateDirtyAndPatchedShutdown != pdbfilehdr->le_dbstate ||
                             lgposAttach.lGeneration >= pdbfilehdr->le_lGenMaxRequired );
                    DBEnforce( ifmp, JET_dbstateIncrementalReseedInProgress != pdbfilehdr->le_dbstate );
                    DBEnforce( ifmp, JET_dbstateRevertInProgress != pdbfilehdr->le_dbstate );
                    Assert( NULL != plogtimeOfGenWithAttach );
                    pdbfilehdr->SetDbstate( JET_dbstateDirtyShutdown, lgposAttach.lGeneration, plogtimeOfGenWithAttach, !plog->FLogDisabled() );
                }
            }
        }
    }

    Assert( !g_rgfmp[ifmp].FLogOn() || !plog->FLogDisabled() );
    Assert( pdbfilehdr->le_dbtimeDirtied >= g_rgfmp[ifmp].DbtimeLast() );
    Assert( pdbfilehdr->le_objidLast >= g_rgfmp[ifmp].ObjidLast() );

    if ( pdbfilehdr->le_objidLast > objidMaxWarningThreshold )
    {
        const WCHAR *rgpszT[1] = { g_rgfmp[ifmp].WszDatabaseName() };
        UtilReportEvent(
                eventWarning,
                GENERAL_CATEGORY,
                ALMOST_OUT_OF_OBJID,
                1,
                rgpszT,
                0,
                NULL,
                PinstFromIfmp( ifmp ) );
    }

    //  reset bkinfo except in the recovering UNDO mode where
    //  we would like to keep the original backup information.

    if ( !fKeepBackupInfo )
    {
        if ( !g_rgfmp[ifmp].FLogOn()
            || memcmp( &pdbfilehdr->signLog, &plog->SignLog(), sizeof( SIGNATURE ) ) != 0 )
        {
            //  if no log or the log signaure is not the same as current log signature,
            //  then the bkinfoIncPrev, bfkinfoFullPrev, bkinfoCopyPrev and bkinfoDiffPrev
            //  are not meaningful.

            memset( &pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );
            memset( &pdbfilehdr->bkinfoFullPrev, 0, sizeof( BKINFO ) );
            memset( &pdbfilehdr->bkinfoCopyPrev, 0, sizeof( BKINFO ) );
            memset( &pdbfilehdr->bkinfoDiffPrev, 0, sizeof( BKINFO ) );
            pdbfilehdr->bkinfoTypeFullPrev = DBFILEHDR::backupNormal;
            pdbfilehdr->bkinfoTypeIncPrev = DBFILEHDR::backupNormal;
            pdbfilehdr->bkinfoTypeCopyPrev = DBFILEHDR::backupNormal;
            pdbfilehdr->bkinfoTypeDiffPrev = DBFILEHDR::backupNormal;
        }
        memset( &pdbfilehdr->bkinfoFullCur, 0, sizeof( BKINFO ) );
        // reset the snapshot data as well
        memset( &pdbfilehdr->bkinfoSnapshotCur, 0, sizeof( BKINFO ) );
    }

    if ( g_fRepair )
    {
        //  preserve the signature
    }
    else if ( plog->FRecovering() || g_rgfmp[ifmp].FLogOn() )
    {
        Assert( !plog->FLogDisabled() );
        Assert( 0 != CmpLgpos( lgposMin, g_rgfmp[ifmp].LgposAttach() ) );
        Assert( 0 == CmpLgpos( lgposMin, g_rgfmp[ifmp].LgposDetach() ) );

        Assert( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) == 0 ||
                CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposConsistent ) == 0 ||
                ( CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposAttach ) >= 0 &&
                  CmpLgpos( lgposAttach, pdbfilehdr->le_lgposAttach ) >= 0 ) );

        //  set new attachment time
        pdbfilehdr->le_lgposAttach = lgposAttach;

        // We have a regular attach, any re-attach is irrelevant now
        pdbfilehdr->le_lgposLastReAttach = lgposMin;

        //  Set global signature.
        if ( 0 != memcmp( &pdbfilehdr->signLog, &plog->SignLog(), sizeof(SIGNATURE) ) )
        {
            pdbfilehdr->le_lGenPreRedoMinRequired = 0;
            pdbfilehdr->le_lGenPreRedoMinConsistent = 0;

            //  must reset lgposConsistent for this log set
            //  keep that order (set lgposConsistent first
            //  then signLog) for the following two lines
            //  it is used by LGLoadAttachmentsFromFMP
            pdbfilehdr->le_lgposConsistent = lgposMin;
            pdbfilehdr->le_lgposLastResize = lgposMin;
            pdbfilehdr->signLog = plog->SignLog();
        }

        if ( ( g_rgfmp[ifmp].ErrDBFormatFeatureEnabled( pdbfilehdr, JET_efvLgposLastResize ) == JET_errSuccess ) &&
             ( CmpLgpos( lgposAttach, pdbfilehdr->le_lgposLastResize ) > 0 ) )
        {
            pdbfilehdr->le_lgposLastResize = lgposAttach;
        }
    }
    else
    {
        //  must regenerate signDb to disassociate it from the past
        SIGGetSignature( &pdbfilehdr->signDb );
    }

    LGIGetDateTime( &pdbfilehdr->logtimeAttach );

    //  reset detach time
    pdbfilehdr->le_lgposDetach = lgposMin;
    memset( &pdbfilehdr->logtimeDetach, 0, sizeof( LOGTIME ) );

    //  le_ulVersion and le_ulDaeUpdateMajor will be set by log record SetDbVersion after attach
    //
    //pdbfilehdr->le_ulVersion = ulDAEVersionMax;
    //pdbfilehdr->le_ulDaeUpdateMajor = ulDAEUpdateMajorMax;

    pdbfilehdr->le_dbid = g_rgfmp[ ifmp ].Dbid();
}

//  ================================================================
ERR ErrDBTryCreateSystemTable(
    __in PIB * const ppib,
    const IFMP ifmp,
    const CHAR * const szTableName,
    ERR (*pfnCreate)(PIB * const, const IFMP, PGNO *, OBJID *),
    const JET_GRBIT grbit,
    PGNO *ppgnoFDP = NULL,
    OBJID *pobjidFDP = NULL )
//  ================================================================
//
//  Used to add dynamically created system tables at attach time.
//  This function checks to see if the table is attached R/W and then
//  looks to see if the specified table exists. If the table doesn't
//  exist then the creation function is called.
//  
{
    Assert( ppib );
    Assert( ifmpNil != ifmp );
    Assert( szTableName );

    ERR         err             = JET_errSuccess;
    WCHAR * const wszDatabaseName   = g_rgfmp[ifmp].WszDatabaseName();
    IFMP        ifmpT;

    if( grbit & JET_bitDbReadOnly || g_fRepair )
    {
        //  no-one will be modifying the database so it doesn't matter if the table
        //  repair doesn't want these tables created
        return JET_errSuccess;
    }

    Call( ErrDBOpenDatabase( ppib, wszDatabaseName, &ifmpT, NO_GRBIT ) );
    Assert( ifmp == ifmpT );

    if( JET_wrnFileOpenReadOnly == err )
    {
        //  we have attached to a read-only file, but JET_bitDbReadOnly was not specified
        //  no-one will be modifying the database so it doesn't matter if the table exists or not
        err = JET_errSuccess;
        if ( NULL != ppgnoFDP )
        {
            *ppgnoFDP  = pgnoNull;
        }
        if (NULL != pobjidFDP )
        {
            *pobjidFDP = objidNil;
        }
    }
    else
    {
        //  look for the table. we used to simply create the table and look for
        //  JET_errTableDuplicate but that led to nullified RCE's filling the
        //  version store when a process did a lot of attaches

        err = ErrCATSeekTable( ppib, ifmpT, szTableName, ppgnoFDP, pobjidFDP );
        if( err < JET_errSuccess )
        {
            if( JET_errObjectNotFound == err )
            {
                if ( NULL != pfnCreate )
                {
                    err = (*pfnCreate)( ppib, ifmp, ppgnoFDP, pobjidFDP );

#ifdef DEBUG
                    if ( JET_errSuccess == err )
                    {
                        PGNO  pgno;
                        OBJID objid;
                        const ERR errT = ErrCATSeekTable( ppib, ifmpT, szTableName, &pgno, &objid );
                        AssertSz( JET_errSuccess == errT, "ErrDBTryCreateSystemTable didn't create the specified table" );
                        Assert( NULL == ppgnoFDP || pgno == *ppgnoFDP );
                        Assert( NULL == pobjidFDP || objid == *pobjidFDP );
                    }
#endif
                }
                else
                {
                    // No creation function, so caller was only checking if the table existed.
                    // It doesn't, so we signify that by returning success and setting the out params
                    // to appropriate NULLs.
                    err = JET_errSuccess;
                    if ( NULL != ppgnoFDP )
                    {
                        *ppgnoFDP  = pgnoNull;
                    }
                    if (NULL != pobjidFDP )
                    {
                        *pobjidFDP = objidNil;
                    }
                }
            }
        }
    }

    CallS( ErrDBCloseDatabase( ppib, ifmpT, 0 ) );

    Call( err );

    return err;

HandleError:    //  error case only

    return err;
}

INLINE ERR ErrDBDeleteUnicodeIndexes( PIB *ppib, const IFMP ifmp )
{
    ERR         err = JET_errSuccess;
    IFMP        ifmpT = 0;
    BOOL        fReadOnly = fFalse;
    BOOL        fIndexesUpdated = fFalse;
    BOOL        fIndexesDeleted = fFalse;

    Call( ErrDBOpenDatabase( ppib, g_rgfmp[ifmp].WszDatabaseName(), &ifmpT, NO_GRBIT ) );
    fReadOnly = ( JET_wrnFileOpenReadOnly == err );
    Assert( ifmp == ifmpT );


    if ( !fReadOnly )
    {
        err = ErrCATDeleteOrUpdateOutOfDateLocalizedIndexes(
                    ppib,
                    ifmpT,
                    catcifDeleteOutOfDateSecondaryIndices | catcifUpdateEmptyIndices | catcifForceDeleteIndices,
                    &fIndexesUpdated,
                    &fIndexesDeleted );

        if ( fIndexesDeleted )
        {
            //  signal version cleanup to reclaim space from
            //  the deleted indices (in case the user recreates
            //  the indices, the hope is that space requests
            //  will be serviced by space released from the
            //  deleted indices)
            //
            VERSignalCleanup( ppib );
        }
    }

    CallS( ErrDBCloseDatabase( ppib, ifmpT, 0 ) );
    Call( err );

    //  to ensure we don't do this again, we'll ask to fixup the table...

    Call( ErrCATVerifyMSLocales( ppib, ifmp, fTrue /* fixup table */ ) );
    
    return err;

HandleError:

    return err;
}

INLINE ERR ErrDBUpgradeForLocalisation( PIB *ppib, const IFMP ifmp, const JET_GRBIT grbit )
{
    ERR         err;
    INST        * const pinst = PinstFromPpib( ppib );
    IFMP        ifmpT;
    BOOL        fIndexesUpdated = fFalse;
    BOOL        fIndexesDeleted     = fFalse;
    const WCHAR *rgsz[9];

    //  judging from the call site, this grbit should never make it here!?!?
    Expected( 0 == ( grbit & JET_bitDbDeleteUnicodeIndexes ) );

    BOOL        fReadOnly;

    //  Write out header with build # = 0 so that any crash from now
    //  will have build # 0. This will cause upgrade again till it is done.
    WCHAR *         wszDatabaseName     = g_rgfmp[ifmp].WszDatabaseName();

    Assert( UlParam( pinst, JET_paramEnableIndexChecking ) == JET_IndexCheckingOn || BoolParam( pinst, JET_paramEnableIndexCleanup ) );

    Call( ErrDBOpenDatabase( ppib, wszDatabaseName, &ifmpT, NO_GRBIT ) );
    fReadOnly = ( JET_wrnFileOpenReadOnly == err || grbit & JET_bitDbReadOnly );

    Assert( ifmp == ifmpT );

    if ( 0 == g_rgfmp[ifmp].Pdbfilehdr()->le_dwMajorVersion )
    {
        WCHAR   rgszVerInfo[4][16];

        OSStrCbFormatW( rgszVerInfo[0], sizeof(rgszVerInfo[0]), L"%d", g_dwGlobalMajorVersion );
        OSStrCbFormatW( rgszVerInfo[1], sizeof(rgszVerInfo[1]), L"%d", g_dwGlobalMinorVersion );
        OSStrCbFormatW( rgszVerInfo[2], sizeof(rgszVerInfo[2]), L"%d", g_dwGlobalBuildNumber );
        OSStrCbFormatW( rgszVerInfo[3], sizeof(rgszVerInfo[3]), L"%d", g_lGlobalSPNumber );

        rgsz[0] = wszDatabaseName;
        rgsz[1] = rgszVerInfo[0];
        rgsz[2] = rgszVerInfo[1];
        rgsz[3] = rgszVerInfo[2];
        rgsz[4] = rgszVerInfo[3];

        UtilReportEvent(
                eventInformation,
                DATA_DEFINITION_CATEGORY,
                START_INDEX_CLEANUP_UNKNOWN_VERSION_ID,
                5,
                rgsz,
                0,
                NULL,
                PinstFromPpib( ppib ) );
    }
    else
    {
        WCHAR   rgszVerInfo[8][16];

        OSStrCbFormatW( rgszVerInfo[0], sizeof(rgszVerInfo[0]), L"%d", DWORD( g_rgfmp[ifmp].Pdbfilehdr()->le_dwMajorVersion ));
        OSStrCbFormatW( rgszVerInfo[1], sizeof(rgszVerInfo[1]), L"%d", DWORD( g_rgfmp[ifmp].Pdbfilehdr()->le_dwMinorVersion ));
        OSStrCbFormatW( rgszVerInfo[2], sizeof(rgszVerInfo[2]), L"%d", DWORD( g_rgfmp[ifmp].Pdbfilehdr()->le_dwBuildNumber ));
        OSStrCbFormatW( rgszVerInfo[3], sizeof(rgszVerInfo[3]), L"%d", DWORD( g_rgfmp[ifmp].Pdbfilehdr()->le_lSPNumber ));
        OSStrCbFormatW( rgszVerInfo[4], sizeof(rgszVerInfo[4]), L"%d", g_dwGlobalMajorVersion );
        OSStrCbFormatW( rgszVerInfo[5], sizeof(rgszVerInfo[5]), L"%d", g_dwGlobalMinorVersion );
        OSStrCbFormatW( rgszVerInfo[6], sizeof(rgszVerInfo[6]), L"%d", g_dwGlobalBuildNumber );
        OSStrCbFormatW( rgszVerInfo[7], sizeof(rgszVerInfo[7]), L"%d", g_lGlobalSPNumber );

        rgsz[0] = wszDatabaseName;
        rgsz[1] = rgszVerInfo[0];
        rgsz[2] = rgszVerInfo[1];
        rgsz[3] = rgszVerInfo[2];
        rgsz[4] = rgszVerInfo[3];
        rgsz[5] = rgszVerInfo[4];
        rgsz[6] = rgszVerInfo[5];
        rgsz[7] = rgszVerInfo[6];
        rgsz[8] = rgszVerInfo[7];

        UtilReportEvent(
                eventInformation,
                DATA_DEFINITION_CATEGORY,
                START_INDEX_CLEANUP_KNOWN_VERSION_ID,
                9,
                rgsz,
                0,
                NULL,
                PinstFromPpib( ppib ) );
    }

    if( JET_IndexCheckingOn == UlParam( pinst, JET_paramEnableIndexChecking )
         || BoolParam( pinst, JET_paramEnableIndexCleanup ) )
    {
        CATCheckIndicesFlags catcifFlags;
        if ( fReadOnly )
        {
            catcifFlags = catcifReadOnly;
        }
        else
        {
            catcifFlags = catcifUpdateEmptyIndices;
            if ( grbit & JET_bitDbDeleteCorruptIndexes )
            {
                catcifFlags |= catcifDeleteOutOfDateSecondaryIndices;
            }
        }

        err = ErrCATDeleteOrUpdateOutOfDateLocalizedIndexes(
                    ppib,
                    ifmpT,
                    catcifFlags,
                    &fIndexesUpdated,
                    &fIndexesDeleted );
    }
    else
    {
        //  don't do anything
        err = JET_errSuccess;
    }

    CallS( ErrDBCloseDatabase( ppib, ifmpT, 0 ) );

    Call( err );

    rgsz[0] = wszDatabaseName;
    
    UtilReportEvent(
            eventInformation,
            DATA_DEFINITION_CATEGORY,
            STOP_INDEX_CLEANUP_ID,
            1,
            rgsz,
            0,
            NULL,
            PinstFromPpib( ppib ) );

    //  Update the header with the new version info

    {
    PdbfilehdrReadWrite pdbfilehdr = g_rgfmp[ifmp].PdbfilehdrUpdateable();
    pdbfilehdr->le_dwMajorVersion = g_dwGlobalMajorVersion;
    pdbfilehdr->le_dwMinorVersion = g_dwGlobalMinorVersion;
    pdbfilehdr->le_dwBuildNumber = g_dwGlobalBuildNumber;
    pdbfilehdr->le_lSPNumber = g_lGlobalSPNumber;
    pdbfilehdr->ResetUpgradeDb();
    }

    //  Update the MSysLocales table with the new info for indices' locales

    Call( ErrCATVerifyMSLocales( ppib, ifmp, fTrue /* fixup table */ ) );

#ifndef RTM
    // check it worked 
    AssertRTL( JET_errDatabaseCorrupted != ErrCATVerifyMSLocales( ppib, ifmp, fFalse /* fixup table */ ) );
#endif

    if ( fIndexesDeleted )
    {
        //  signal version cleanup to reclaim space from
        //  the deleted indices (in case the user recreates
        //  the indices, the hope is that space requests
        //  will be serviced by space released from the
        //  deleted indices)
        //
        VERSignalCleanup( ppib );
        err = ErrERRCheck( JET_wrnCorruptIndexDeleted );
    }
    else
    {
        err = JET_errSuccess;
    }

    return err;

HandleError:

    return err;
}


LOCAL VOID DBReportPartiallyAttachedDb(
    const INST  *pinst,
    const WCHAR *wszDbName,
    const ULONG ulAttachStage,
    const ERR   err )
{
    WCHAR       wszErr[16];
    WCHAR       wszAttachStage[16];
    const WCHAR * rgszT[3]  = { wszDbName, wszAttachStage, wszErr };

    OSStrCbFormatW( wszAttachStage, sizeof( wszAttachStage), L"%d", ulAttachStage );
    OSStrCbFormatW( wszErr, sizeof(wszErr), L"%d", err );
    UtilReportEvent(
            eventError,
            DATABASE_CORRUPTION_CATEGORY,
            DB_PARTIALLY_ATTACHED_ID,
            3,
            rgszT,
            0,
            NULL,
            pinst );
}

LOCAL VOID DBReportPartiallyDetachedDb(
    const INST  *pinst,
    const WCHAR *szDbName,
    const ERR   err )
{
    WCHAR       szErr[16];
    const WCHAR * rgszT[2]  = { szDbName, szErr };

    OSStrCbFormatW( szErr, sizeof(szErr), L"%d", err );
    UtilReportEvent(
            eventError,
            DATABASE_CORRUPTION_CATEGORY,
            DB_PARTIALLY_DETACHED_ID,
            2,
            rgszT,
            0,
            NULL,
            pinst );
}

//  ================================================================
ERR ErrDBSetUserDbHeaderInfo(
    FMP * const         pfmp,
    ULONG               cbdbinfomisc,
    JET_DBINFOMISC7 *   pdbinfomisc )
//  ================================================================
{
    ERR err = JET_errSuccess;

    Assert( pfmp );
    Assert( pdbinfomisc );
    Assert( FMP::FWriterFMPPool() || FMP::FReaderFMPPool() );

    pfmp->RwlDetaching().EnterAsReader();

    if ( !pfmp->Pdbfilehdr() )
    {
        Error( ErrERRCheck( errNotFound ) );
    }

    UtilLoadDbinfomiscFromPdbfilehdr(
        pdbinfomisc,
        cbdbinfomisc,
        pfmp->Pdbfilehdr().get() );

HandleError:
    pfmp->RwlDetaching().LeaveAsReader();
    return err;
}



//  ================================================================
LOCAL ERR ErrDBReadDBTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    const WCHAR * const wszFileName,
    __out_bcount( cbTrailer ) BYTE * const pbTrailer,
    const INT cbTrailer )
//  ================================================================
//
// Reads the last page of the database into the buffer pointed to by pbTrailer
//
//-
{
    Assert( pfsapi );
    Assert( wszFileName );
    Assert( pbTrailer );
    Assert( g_cbPage == cbTrailer );
    
    ERR         err     = JET_errSuccess;
    IFileAPI *  pfapi   = NULL;
    QWORD       cbSize  = 0;
    TraceContextScope tcScope( iorpHeader );
    tcScope->SetDwEngineObjid( objidSystemRoot );

    if( cbTrailer < g_cbPage )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }
    
    Call( pfsapi->ErrFileOpen(
            wszFileName,
            (   IFileAPI::fmfReadOnly |
                ( BoolParam( JET_paramEnableFileCache ) ? IFileAPI::fmfCached : IFileAPI::fmfNone ) ),
            &pfapi ) );

    Call( pfapi->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );

    // there should be at least one page past the database header for us to read
    if( cbSize < QWORD ( cpgDBReserved + 1 )  * g_cbPage )
    {
        OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"d2482af5-0d49-4abb-ab54-13846e8c5034" );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    Call( pfapi->ErrIORead( *tcScope, cbSize - QWORD( cbTrailer ), cbTrailer, pbTrailer, qosIONormal ) );
    CallS( err );

HandleError:

    delete pfapi;
    return err;
}

//  ================================================================
ERR ErrDBTryToZeroDBTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    const WCHAR * const wszDatabase)
//  ================================================================
//
// Zeroes the last page of the database if the last page is actually a trailer.
//
//-
{
    Assert( pfsapi );
    Assert( wszDatabase );
    
    ERR         err     = JET_errSuccess;
    IFileAPI *  pfapi   = NULL;
    QWORD       cbSize  = 0;

    BYTE *      pbTrailer = NULL;;
    TraceContextScope tcScope( iorpHeader );
    tcScope->SetDwEngineObjid( objidSystemRoot );

    Alloc( pbTrailer = static_cast<BYTE *> ( PvOSMemoryPageAlloc( g_cbPage, NULL ) ) );

    // Check the trailer page to see if it really is a trailer page.
    err = ErrDBReadAndCheckDBTrailer( pinst, pfsapi, wszDatabase, pbTrailer, g_cbPage );
    if( err < JET_errSuccess )
    {
        
        // If something goes wrong here, it's ok because the zero'ing is just to suppress
        // unexpected checksum failures.
        
        err = JET_errSuccess;
        goto HandleError;
    }

    Call( pfsapi->ErrFileOpen(
            wszDatabase,
            BoolParam( JET_paramEnableFileCache ) ? IFileAPI::fmfCached : IFileAPI::fmfNone,
            &pfapi ) );
            
    Call( pfapi->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );

    // there should be at least one page past the database header for us to read
    if( cbSize < QWORD ( cpgDBReserved + 1 )  * g_cbPage )
    {
        OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"e893b103-65f9-4833-9065-f484904fe609" );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    // Zero the page
    memset( pbTrailer, 0, g_cbPage );

    Call( pfapi->ErrIOWrite( *tcScope, cbSize - QWORD(g_cbPage), g_cbPage, pbTrailer, qosIONormal ) );
    CallS( err );

    //  Ugh, no pinst!  Ugh ... skipping FFB suppression for now, but not that bad 
    //  for this one time JetInit after backup restore.
    Call( ErrUtilFlushFileBuffers( pfapi, iofrUtility ) );
    
HandleError:
    OSMemoryPageFree( pbTrailer );
    delete pfapi;
    return err;
}



//  ================================================================
ERR ErrDBReadAndCheckDBTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    const WCHAR * const wszFileName,
    __out_bcount( cbBuffer ) BYTE * const pbBuffer,
    const INT cbBuffer )
//  ================================================================
//
// Read the last page of the database into the buffer pointed to by 
// pbBuffer and checksum it.
//
//-
{
    Assert( pfsapi );
    Assert( wszFileName );
    Assert( pbBuffer );
    Assert( g_cbPage == cbBuffer );
    
    ERR         err         = JET_errSuccess;
    PAGECHECKSUM    checksumExpected    = 0;
    PAGECHECKSUM    checksumActual      = 0xffffffff;

    if( cbBuffer < g_cbPage )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    Call( ErrDBReadDBTrailer( pinst, pfsapi, wszFileName, pbBuffer, cbBuffer ) );

    ChecksumPage(
                pbBuffer,
                g_cbPage,
                databaseHeader,
                0,
                &checksumExpected,
                &checksumActual );

    if ( checksumExpected != checksumActual )
    {
        if ( !BoolParam( JET_paramDisableBlockVerification ) )
        {
            //  the page has a verification failure
            Call( ErrERRCheck( JET_errBadPatchPage ) );
        }
    }

    CallS( err );
HandleError:
    return err;
}


//  ================================================================
ERR ErrDBCheckDBHeaderAndTrailer(
    const DBFILEHDR * const pdbfilehdr,
    const PATCHHDR * const ppatchhdr,
    const WCHAR * const wszDatabase,
    BOOL fSkipMinLogChecks )
//  ================================================================
//
//  Compare the database header to the trailer page to see if they match
//
//-
{
    Assert( pdbfilehdr );
    Assert( ppatchhdr );
    Assert( wszDatabase );
    
    ERR err = JET_errSuccess;
    if ( memcmp( &pdbfilehdr->signDb, &ppatchhdr->signDb, sizeof( SIGNATURE ) ) != 0 ||
         memcmp( &pdbfilehdr->signLog, &ppatchhdr->signLog, sizeof( SIGNATURE ) ) != 0 ||
         ppatchhdr->bkinfo.le_genHigh == 0 ||
         ( !fSkipMinLogChecks &&
          CmpLgpos( &pdbfilehdr->bkinfoFullCur.le_lgposMark, &ppatchhdr->bkinfo.le_lgposMark ) != 0 ) )
    {
        const UINT csz = 1;
        const WCHAR *rgwszT[csz];

        rgwszT[0] = wszDatabase;
        UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                    DATABASE_PATCH_FILE_MISMATCH_ERROR_ID, csz, rgwszT, 0, NULL, pinstNil );
        CallR( ErrERRCheck( JET_errDatabasePatchFileMismatch ) );
    }
    CallS( err );
    return err;
}

//  ================================================================
BOOL FDBHeaderNeedsUpdating(const DBFILEHDR * const pdbfilehdr)
//  ================================================================
//
// See if the header has needs to be updated with information from
// the trailer.
//
//-
{
    if ( 0 == pdbfilehdr->bkinfoFullCur.le_genLow
        || pdbfilehdr->bkinfoFullCur.le_genHigh )
    {
        return fFalse;
    }
    return fTrue;
}


//  ================================================================
LOCAL ERR ErrDBIUpdateHeaderFromTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    DBFILEHDR   * const pdbfilehdr,
    const PATCHHDR * const ppatchhdr,
    const WCHAR * const wszDatabase,
    BOOL fSkipMinLogChecks )
//  ================================================================
//
// Updates the database header in pdbfilehdr with information from the
// trailer in ppatchhdr and writes the header to wszDatabase.
//
//-
{
    Assert( pfsapi );
    Assert( pdbfilehdr );
    Assert( ppatchhdr );
    Assert( wszDatabase );

    ERR             err         = JET_errSuccess;

    CallR( ErrDBCheckDBHeaderAndTrailer( pdbfilehdr, ppatchhdr, wszDatabase, fSkipMinLogChecks ) );

    Assert( FDBHeaderNeedsUpdating( pdbfilehdr ) || fSkipMinLogChecks );

    // All of these values are supposed to be unsigned, but they are sometimes treated
    // as signed or cast to signed variables in other parts of the code, which may lead
    // to problems if they are > lMax, i.e., if they turn out to be negative due to bugs.
    // This has happened in the past so the asserts below aim to catch such bugs as closely
    // as possible to their source.
    const LONG lgenLow = ppatchhdr->bkinfo.le_genLow;
    const LONG lgenHigh = ppatchhdr->bkinfo.le_genHigh;
    const LONG lgenMinReq = ppatchhdr->lgenMinReq;
    const LONG lgenMaxReq = ppatchhdr->lgenMaxReq;
    const LONG lgenMaxCommited = ppatchhdr->lgenMaxCommitted;

    Assert( lgenLow > 0 );
    Assert( lgenHigh > 0 );
    Assert( lgenMinReq >= 0 );
    Assert( lgenMaxReq >= 0 );
    Assert( lgenMaxCommited >= 0 );

    pdbfilehdr->bkinfoFullCur.le_genHigh = lgenHigh;

    if ( !fSkipMinLogChecks )
    {
        Assert( pdbfilehdr->le_lGenMinRequired == 0 || pdbfilehdr->le_lGenMinRequired >= lgenLow );
    }
    else
    {
        // HA restartable reseed passes this. We could have also detached/re-attached in the middle of
        // backup/reseed, so mark header as "dirty and patched" so we do not mark database as clean on
        // seeing a Detach/Term record until we are past the MaxRequired generation.
        // Restricting it to HA restartable reseed only because without aggressive log rollover, it may
        // take too long to get back to dirty state.

        // Copy relevant asserts from SetDbState. We cannot use SetDbState because it does not transition
        // from JET_dbstateIncrementalReseedInProgress and we do not have logtime of maxCommitted log, so
        // better to copy relevant asserts here rather than pollute the SetDbState state transition machine.
        Assert( 0 != lgenLow && lGenerationInvalid != lgenLow );
        Assert( 0 != lgenHigh && lGenerationInvalid != lgenHigh );
        Assert( ( pdbfilehdr->le_dbstate == JET_dbstateDirtyShutdown ) || ( pdbfilehdr->le_dbstate == JET_dbstateDirtyAndPatchedShutdown ) );

        pdbfilehdr->le_dbstate = JET_dbstateDirtyAndPatchedShutdown;
    }

    Expected( ( ( lgenMinReq == 0 ) && ( lgenMaxReq == 0 ) && ( lgenMaxCommited == 0 ) ) ||
              ( ( lgenMinReq > 0 ) && ( lgenMaxReq > 0 ) && ( lgenMaxCommited > 0 ) ) );
    Expected( !!ppatchhdr->logtimeGenMaxRequired.FIsSet() == !!ppatchhdr->logtimeGenMaxCommitted.FIsSet() );
    Assert( ( lgenMinReq == 0 ) || ( lgenMinReq >= lgenLow ) );
    Assert( ( lgenMaxReq == 0 ) || ( lgenMaxReq <= lgenHigh ) );
    Assert( ( lgenMaxCommited == 0 ) || ( lgenMaxCommited <= lgenHigh ) );
    Assert( pdbfilehdr->le_lGenMaxRequired <= lgenHigh );
    Assert( pdbfilehdr->le_lGenMaxCommitted <= lgenHigh );

    const LONG lgenMinReqPatch = lgenMinReq;
    const LONG lgenMaxReqPatch = ( lgenMaxReq > 0 ) ? lgenMaxReq : lgenHigh;
    const LONG lgenMaxComPatch = ( lgenMaxCommited > 0 ) ? lgenMaxCommited : lgenHigh;

    // The required range at the beginning of the backup may not be the same as at the end.
    if ( ( lgenMinReqPatch > 0 ) &&
         ( lgenMinReqPatch < pdbfilehdr->le_lGenMinRequired ) )
    {
        // We don't expect this code to hit for now. This can only happen if the backup was interrupted
        // and then restarted following a checkpoint deletion or incremental reseed, which only Exchange's
        // HA does. However, they also use JET_bitReplayIgnoreLogRecordsBeforeMinRequiredLog, which suppresses
        // replaying log files below the DB's min required. They also abort a full seed if the min required
        // goes backwards due to incremental reseed.
        FireWall( "BackupUpdHdrFromTrailerLoweredMinReq" );
        Assert( lgenMinReqPatch >= lgenLow );
        pdbfilehdr->le_lGenMinRequired = lgenMinReqPatch;
    }
    if ( lgenMaxReqPatch >= pdbfilehdr->le_lGenMaxRequired )
    {
        pdbfilehdr->logtimeGenMaxRequired = ppatchhdr->logtimeGenMaxRequired;
        pdbfilehdr->le_lGenMaxRequired = lgenMaxReqPatch;
    }
    if ( lgenMaxComPatch >= pdbfilehdr->le_lGenMaxCommitted )
    {
        pdbfilehdr->logtimeGenMaxCreate = ppatchhdr->logtimeGenMaxCommitted;
        pdbfilehdr->le_lGenMaxCommitted = lgenMaxComPatch;
    }

    // Reset these fields.
    pdbfilehdr->le_lGenMinConsistent = pdbfilehdr->le_lGenMinRequired;
    pdbfilehdr->le_lGenPreRedoMinConsistent = 0;
    pdbfilehdr->le_lGenPreRedoMinRequired = 0;

    Assert( !FDBHeaderNeedsUpdating( pdbfilehdr ) );

    CallR( ErrUtilWriteUnattachedDatabaseHeaders( pinst, pfsapi, wszDatabase, pdbfilehdr ) );

    CallS( err );

    return err;
}


//  ================================================================
ERR ErrDBUpdateHeaderFromTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    DBFILEHDR   * const pdbfilehdr,
    const WCHAR * const wszDatabase,
    BOOL        fSkipMinLogChecks )
//  ================================================================
//
// Reads the trailer from wszDatabase, updates the database header in pdbfilehdr
// with information from the trailer and writes the header to wszDatabase.
//
//-
{
    Assert( pfsapi );
    Assert( pdbfilehdr );
    Assert( wszDatabase );

    ERR             err         = JET_errSuccess;
    PATCHHDR *      ppatchhdr   = NULL;;

    Alloc( ppatchhdr = static_cast<PATCHHDR *>( PvOSMemoryPageAlloc( g_cbPage, NULL ) ) );

    // should be a db header
    if( attribDb != pdbfilehdr->le_attrib )
    {
        OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"0fcb4d48-e21a-4862-95d3-1b599eb63d51" );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if( !FDBHeaderNeedsUpdating( pdbfilehdr ) && !fSkipMinLogChecks )
    {
        CallS( err );

        // If we have a trailer page then we should try to zero it so as not to confuse any code that
        // is not expecting it to be a trailer page w.r.t. checksums.
        Call( ErrDBTryToZeroDBTrailer( pinst,
                                       pfsapi,
                                       wszDatabase ) );

        CallS( err );
        
        goto HandleError;
    }

    Call( ErrDBReadAndCheckDBTrailer( pinst, pfsapi, wszDatabase, reinterpret_cast<BYTE*>( ppatchhdr ), g_cbPage ) );
    Call( ErrDBIUpdateHeaderFromTrailer( pinst, pfsapi, pdbfilehdr, ppatchhdr, wszDatabase, fSkipMinLogChecks ) );
    CallS( err );

    // Try to zero the trailer page.
    Call( ErrDBTryToZeroDBTrailer( pinst,
                                   pfsapi,
                                   wszDatabase ) );
    CallS( err );

HandleError:
    if ( err == JET_errBadPatchPage )
    {
        const WCHAR *rgszT[1];
        rgszT[0] = wszDatabase;

        //  attempting to use a corrupt trailer page, probably an incomplete backup
        UtilReportEvent(
                eventError,
                LOGGING_RECOVERY_CATEGORY,
                TRAILER_PAGE_MISSING_OR_CORRUPT_ID,
                1,
                rgszT,
                0,
                NULL,
                pinst );

        OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"a03c9107-e57b-45f9-96f8-1bcee797c960" );
    }

    OSMemoryPageFree( ppatchhdr );
    return err;
}


//  ================================================================
LOCAL ERR ErrDBIUpdateHeaderFromTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    const WCHAR * const wszDatabase,
    BOOL fSkipMinLogChecks )
//  ================================================================
//
// Reads the trailer from wszDatabase, update the header with information
// from the trailer if needed.
//
//-
{
    Assert( pfsapi );
    Assert( wszDatabase );

    ERR         err         = JET_errSuccess;
    DBFILEHDR * pdbfilehdr  = NULL;

    Alloc( pdbfilehdr = static_cast<DBFILEHDR * >( PvOSMemoryPageAlloc( g_cbPage, NULL ) ) );

    err = ErrUtilReadShadowedHeader(
            pinst,
            pfsapi,
            wszDatabase,
            reinterpret_cast<BYTE *>( pdbfilehdr ),
            g_cbPage,
            OffsetOf( DBFILEHDR_FIX, le_cbPageSize ),
            urhfReadOnly );
    Assert( err != JET_errDatabasePatchFileMismatch );
    Call( err );

    Call( ErrDBUpdateHeaderFromTrailer( pinst, pfsapi, pdbfilehdr, wszDatabase, fSkipMinLogChecks ) );

    CallS( err );
HandleError:
    OSMemoryPageFree( pdbfilehdr );
    return err;
}


//  ================================================================
ERR ErrDBUpdateHeaderFromTrailer( const WCHAR * const wszDatabase, BOOL fSkipMinLogChecks )
//  ================================================================
//
// Reads the trailer from wszDatabase, update the header with information
// from the trailer if needed.
//
//-
{
    Assert( wszDatabase );

    ERR             err     = JET_errSuccess;
    IFileSystemAPI* pfsapi  = NULL;
    
    Call( ErrOSFSCreate( g_pfsconfigGlobal, &pfsapi ) );
    Call( ErrDBIUpdateHeaderFromTrailer( pinstNil, pfsapi, wszDatabase, fSkipMinLogChecks ) );

    CallS( err );
HandleError:
    delete pfsapi;
    return err;
}

ERR ISAMAPI ErrIsamAttachDatabase(
    JET_SESID                   sesid,
    __in PCWSTR                 wszDatabaseName,
    const BOOL                  fAllowTrimDBTask,
    const JET_SETDBPARAM* const rgsetdbparam,
    const ULONG                 csetdbparam,
    const JET_GRBIT             grbit )
{
    PIB                 *ppib;
    ERR                 err;
    IFMP                ifmp;
    WCHAR               rgchDbFullName[IFileSystemAPI::cchPathMax];
    WCHAR               *wszDbFullName                  = rgchDbFullName;
    CPG                 cpgDatabaseSizeMax              = 0;
    ULONG               pctCachePriority                = g_pctCachePriorityUnassigned;
    JET_GRBIT           grbitShrinkDatabaseOptions      = NO_GRBIT;
    LONG                dtickShrinkDatabaseTimeQuota    = -1;
    CPG                 cpgShrinkDatabaseSizeLimit      = 0;
    BOOL                fLeakReclaimerEnabled           = fFalse;
    LONG                dtickLeakReclaimerTimeQuota     = -1;
    LGPOS               lgposLogRec;
    LOGTIME             logtimeOfGenWithAttach;
    BOOL                fReadOnly;
    BOOL                fDeleteMSUTable                 = fFalse;
    BOOL                fCacheAlive                     = fFalse;
    BOOL                fDirtyCacheAlive                = fFalse;
    FMP                 *pfmp                           = NULL;
    IFileSystemAPI      *pfsapi;
    FEATURECONTROL      fcMaintainExtentPageCountCache;
    enum { ATTACH_NONE, ATTACH_LOGGED, ATTACH_DB_UPDATED, ATTACH_DB_OPENED, ATTACH_END }
        attachState;
    attachState = ATTACH_NONE;

    CallR( ErrDBParseDbParams( rgsetdbparam,
                               csetdbparam,
                               &cpgDatabaseSizeMax,
                               &pctCachePriority,
                               &grbitShrinkDatabaseOptions,
                               &dtickShrinkDatabaseTimeQuota,
                               &cpgShrinkDatabaseSizeLimit,
                               &fLeakReclaimerEnabled,
                               &dtickLeakReclaimerTimeQuota,
                               &fcMaintainExtentPageCountCache ) );

    memset( &logtimeOfGenWithAttach, 0, sizeof( logtimeOfGenWithAttach ) );

    COSTraceTrackErrors trackerrors( __FUNCTION__ );

    //  check parameters
    //
    Assert( sizeof(JET_SESID) == sizeof(PIB *) );
    ppib = (PIB *)sesid;

    CallR( ErrPIBCheck( ppib ) );

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = tceNone;
    tcScope->SetDwEngineObjid( objidSystemRoot );

    INST *pinst = PinstFromPpib( ppib );
    LOG *plog = pinst->m_plog;
    BACKUP_CONTEXT *pbackup = pinst->m_pbackup;
    const BOOL fSparseEnabledFile = !!( GrbitParam( pinst, JET_paramEnableShrinkDatabase ) & JET_bitShrinkDatabaseOn );

    OSTrace(
        JET_tracetagDatabases,
        OSFormat(
            "Session=[0x%p:0x%x] attaching database %s [cpgMax=0x%x,pctCachePriority=%I32u,grbit=0x%x,sparse=%d]",
            ppib,
            ppib->trxBegin0,
            SzOSFormatStringW( wszDatabaseName ),
            cpgDatabaseSizeMax,
            pctCachePriority,
            grbit,
            fSparseEnabledFile ) );

    if ( ppib->Level() > 0 )
        return ErrERRCheck( JET_errInTransaction );

    if ( grbit & JET_bitDbVersioningOff )
        return ErrERRCheck( JET_errCannotDisableVersioning );

    if ( NULL == wszDatabaseName || 0 == *wszDatabaseName )
        return ErrERRCheck( JET_errDatabaseInvalidPath );

    // We cannot delete indices and call this a read-only attach.
    if ( ( grbit & JET_bitDbDeleteUnicodeIndexes ) && ( grbit & JET_bitDbReadOnly ) )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    pfsapi = pinst->m_pfsapi;

    //  depend on ErrPathComplete to make same files same name
    //  thereby preventing same file to be multiply attached
    //
    err = ErrUtilPathComplete( pfsapi, wszDatabaseName, rgchDbFullName, fTrue );
    //  if ( JET_errFileNotFound == err )
    //      {
    //      must return FileNotFound to retain backward-compatibility
    //      err = ErrERRCheck( JET_errDatabaseNotFound );
    //      }
    CallR( JET_errInvalidPath == err ? ErrERRCheck( JET_errDatabaseInvalidPath ) : err );


    Assert( wszDbFullName == rgchDbFullName );
    CallR( ErrUtilPathReadOnly( pfsapi, wszDbFullName, &fReadOnly ) );
    if ( fReadOnly && !( grbit & JET_bitDbReadOnly ) )
    {
        err = ErrERRCheck( JET_errDatabaseFileReadOnly );
        return err;
    }

    pbackup->BKLockBackup();

    if ( pbackup->FBKBackupInProgress() )
    {
        BOOL fLocal = pbackup->FBKBackupInProgress( BACKUP_CONTEXT::backupLocalOnly );
        pbackup->BKUnlockBackup();
        if ( fLocal )
        {
            return ErrERRCheck( JET_errBackupInProgress );
        }
        else
        {
            return ErrERRCheck( JET_errSurrogateBackupInProgress );
        }
    }

    err = FMP::ErrNewAndWriteLatch(
            &ifmp,
            wszDbFullName,
            ppib,
            pinst,
            pfsapi,
            dbidMax,
            ( grbit & JET_bitDbPurgeCacheOnAttach ),
            !( grbit & JET_bitDbReadOnly ) && !plog->FLogDisabled(),
            &fCacheAlive );

    if ( err != JET_errSuccess )
    {
#ifdef DEBUG
        switch ( err )
        {
            case JET_wrnDatabaseAttached:
            case JET_errOutOfMemory:
            case JET_errDatabaseInUse:
            case JET_errTooManyAttachedDatabases:
            case JET_errDatabaseSharingViolation:
            case JET_errDatabaseInvalidPath:
                break;
            default:
                CallS( err );       //  force error to be reported in assert
        }
#endif
        pbackup->BKUnlockBackup();

        // database already attached by recovery, but needs more initialization
        // for active
        if ( err == JET_wrnDatabaseAttached &&
             g_rgfmp[ ifmp ].FAttachedForRecovery() )
        {
            #pragma push_macro( "Call" )
            #undef Call
            #define Call DO_NOT_USE_CALL_HERE_Use_CallJ_To_Detach_Like_Others

            pfmp = &g_rgfmp[ ifmp ];

            Assert( ( pfmp->PLogRedoMapZeroed() == NULL ) && ( pfmp->PLogRedoMapBadDbTime() == NULL ) && ( pfmp->PLogRedoMapDbtimeRevert() == NULL ) );

            Assert( !pinst->FRecovering() );
            Assert( !pfmp->FReadOnlyAttach() && !g_fRepair );

            fDirtyCacheAlive = fTrue;

            pfmp->m_isdlAttach.InitSequence( isdltypeAttach, eAttachSeqMax );
            pfmp->m_isdlAttach.Trigger( eAttachInitVariableDone );
            pfmp->m_isdlAttach.Trigger( eAttachFastReAttachAcquireFmp );

            pfmp->SetRuntimeDbParams( cpgDatabaseSizeMax,
                                      pctCachePriority,
                                      grbitShrinkDatabaseOptions,
                                      dtickShrinkDatabaseTimeQuota,
                                      cpgShrinkDatabaseSizeLimit,
                                      fLeakReclaimerEnabled,
                                      dtickLeakReclaimerTimeQuota );

            const FormatVersions * pfmtversDesired = NULL;
            BOOL fDbNeedsUpdate = fFalse;
            DbVersion dbvBefore;
            {
            PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr(); // .ctor acquires header lock

            dbvBefore = pdbfilehdr->Dbv();

            CallJ( ErrDBIValidateUserVersions( pinst, wszDbFullName, ifmp, pdbfilehdr.get(), &pfmtversDesired, &fDbNeedsUpdate ), Detach );
            } // .dtor releases header lock
    
            LGPOS lgposReAttach;
            CallJ( ErrLGReAttachDB( ifmp, &lgposReAttach ), Detach );

            Assert(  fDbNeedsUpdate == ( CmpDbVer( dbvBefore, pfmtversDesired->dbv ) < 0 ) );
            if ( g_rgfmp[ ifmp ].FLogOn() && 
                 // We also log equal DbVersions in case a log replaying replica missed such an
                 // update due to Incremental Reseed or lossy failover.
                 ( CmpDbVer( dbvBefore, pfmtversDesired->dbv ) <= 0 )
                 /* && !fRedoing - but always true */  )
            {
                CallJ( ErrDBLGVersionUpdate( pinst, ppib, ifmp, pfmtversDesired, fDbNeedsUpdate ), Detach );
            }
            else
            {
                // If we have a db-version update, no logging is the only reason we should not log the update.
                Enforce( !fDbNeedsUpdate || !g_rgfmp[ ifmp ].FLogOn() );
            }
        
            //  Post logging dbv should not have been updated by any other method ...
            
            OnDebug( if ( ( rand() % 3 ) == 0 ) UtilSleep( 20 ) );
            Assert( CmpDbVer( dbvBefore, pfmp->Pdbfilehdr()->Dbv() ) == 0 );
            
            {
            PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable(); // .ctor acquires header lock

            //  Bunch of stuff normally done by DBISetHeaderAfterAttach below

            pdbfilehdr->le_lGenRecovering = 0;

            if ( pdbfilehdr->Dbstate() != JET_dbstateDirtyShutdown )
            {
                FireWall( "NotDirtyOnReAttach" );
                pdbfilehdr->SetDbstate( JET_dbstateDirtyShutdown, lGenerationInvalid, lGenerationInvalid, NULL, fTrue );
            }

            if ( pdbfilehdr->le_objidLast > objidMaxWarningThreshold )
            {
                const WCHAR *rgpszT[1] = { pfmp->WszDatabaseName() };
                UtilReportEvent(
                        eventWarning,
                        GENERAL_CATEGORY,
                        ALMOST_OUT_OF_OBJID,
                        1,
                        rgpszT,
                        0,
                        NULL,
                        PinstFromIfmp( ifmp ) );
            }

            // Set re-attach time
            pdbfilehdr->le_lgposLastReAttach = lgposReAttach;
            LGIGetDateTime( &pdbfilehdr->logtimeLastReAttach );
            pfmp->m_isdlAttach.FixedData().sAttachData.lgposAttach = lgposReAttach;
            Expected( pfmp->m_isdlAttach.FActiveSequence() );

            //  reset bkinfo
            memset( &pdbfilehdr->bkinfoFullCur, 0, sizeof( BKINFO ) );
            // reset the snapshot data as well
            memset( &pdbfilehdr->bkinfoSnapshotCur, 0, sizeof( BKINFO ) );

            if ( fDbNeedsUpdate )
            {
                //  Update the headers DB version to the desired one.
            
                DBISetVersion( pinst, wszDbFullName, ifmp, pfmtversDesired->dbv, pdbfilehdr.get(), fDbNeedsUpdate, fFalse );
            }
            
            } // .dtor releases header lock

            Assert( CmpDbVer( pfmp->Pdbfilehdr()->Dbv(), pfmtversDesired->dbv ) >= 0 );

            //  Note: I caught the checkpoint running AND writing the DB header right here, I am not sure
            //  this was ever truly intended, but worth noting that the ordering of this code and relation
            //  to ErrUtilWriteAttachedDatabaseHeaders is not guaranteed!

            CallJ( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ), Detach );

            //  Today, this shouldn't go off, because there is only one consumer of the header update 
            //  state. But if someone utilizes it from like checkpoint update or something, this will
            //  start going off and someone really should allow (and probably increase the header lock
            //  rank) the header lock be held over logging data, and definitely let the lock be passed
            //  into ErrUtilWriteAttachedDatabaseHeaders().
            Assert( !pfmp->FHeaderUpdateInProgress() );

            //  Note: This synchronizes all outstanding IOs to FilePerfAPI completion (though actual IO 
            //  pfnIOCompletion may fire after).
            pfmp->Pfapi()->UpdateIFilePerfAPIEngineFileTypeId( iofileDbAttached, pfmp->Ifmp() );
            IOResetFmpIoLatencyStats( pfmp->Ifmp() );

#ifdef DEBUG
            BYTE rgbSigZeroes[ sizeof( SIGNATURE ) ] = { 0 };
            Assert( 0 == memcmp( rgbSigZeroes, pfmp->Pdbfilehdr()->rgbReservedSignSLV, sizeof( rgbSigZeroes ) ) );
#endif

            //  We have somehow downgraded the DB version!  Bad.
            AssertTrack( CmpDbVer( pfmp->Pdbfilehdr()->Dbv(), dbvBefore ) >= 0, "InvalidDbVersionDowngradeAttach" );
            
            OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextFmp, pfmp, &ifmp, sizeof(ifmp) );

            pfmp->InitializeDbtimeOldest();

            if ( BoolParam( pinst, JET_paramFlight_EnableReattachRaceBugFix ) )
            {
                Assert( !pfmp->FAllowHeaderUpdate() );
                pfmp->RwlDetaching().EnterAsWriter();
                pfmp->SetAllowHeaderUpdate();
                pfmp->RwlDetaching().LeaveAsWriter();
            }

            {
            //  set the last page of the database and resize it (we normally do it at the end of recovery,
            //  but not when we decide to keep the DB cache alive or were DirtyAndPatched when we hit a
            //  resize operation (like detach or attach), or we replayed a shrink LR in the required range).
            OnDebug( PGNO pgnoLastBefore = pfmp->PgnoLast() );
            CallJ( ErrDBSetLastPage( ppib, ifmp ), Detach );
            Assert( pgnoLastBefore >= pfmp->PgnoLast() );
            if ( !BoolParam( JET_paramEnableViewCache ) )
            {
                OnDebug( pgnoLastBefore = pfmp->PgnoLast() );
                CallJ( ErrIONewSize( ifmp, *tcScope, pfmp->PgnoLast(), 0, JET_bitNil ), Detach );
                Assert( pgnoLastBefore == pfmp->PgnoLast() );
            }

            //  ensure our database has the expected size.
            //
            QWORD       cbLogicalSize       = OffsetOfPgno( pfmp->PgnoLast() + 1 );
            QWORD       cbNtfsSize          = 0;
            if ( pfmp->Pfapi()->ErrSize( &cbNtfsSize, IFileAPI::filesizeLogical ) >= JET_errSuccess )
            {
            AssertTrack( cbNtfsSize >= cbLogicalSize, "ReattachFileSizeTooSmall" );
            }
            }

            {
                PGNO pgnoFDP;
                OBJID objidFDP;

                // Look up the ExtentPageCountCache table before we do anything that might affect
                // ExtentPageCountCache values. This sets up bookkeeping so any changes to space
                // made by Update and Shrink are correctly tracked.  Since we're not providing a
                // creation function to this call, it only looks up the table if it's there; it
                // doesn't create it and doesn't error if it's not there, just sets pgnoFDP and
                // objidFDP to nulls.
                CallJ( ErrDBTryCreateSystemTable(
                           ppib,
                           ifmp,
                           szMSExtentPageCountCache,
                           NULL,
                           NO_GRBIT,
                           &pgnoFDP,
                           &objidFDP ),
                       MoreAttachedThanDetached );
                g_rgfmp[ ifmp ].SetExtentPageCountCacheTableInfo( pgnoFDP, objidFDP );
            }

            // Trigger leak reclaimer.
            JET_ERR errLeakReclaimer =  JET_errSuccess;
            if ( pfmp->FLeakReclaimerEnabled() )
            {
                errLeakReclaimer = ErrSPReclaimSpaceLeaks( ppib, ifmp );
                pfmp->m_isdlAttach.Trigger( eAttachLeakReclaimerDone );
            }

            //  Trigger DB shrink.
            if ( ( errLeakReclaimer >= JET_errSuccess ) &&
                 pfmp->FShrinkDatabaseEofOnAttach() &&
                 !BoolParam( JET_paramEnableViewCache ) &&
                 pfmp->FEfvSupported( JET_efvShrinkEof ) )
            {
                const JET_ERR errT = ErrSHKShrinkDbFromEof( ppib, ifmp );

                // Fail the attach if we hit a corruption error, as safety measure to prevent the corruption from spreading.
                // Also, JET_errDatabaseInUse is used to signal that codepaths that require/assume exclusive access to the
                // database could not guarantee exclusivity, so fail the attach in that case for safety.
                const ErrData* perrdata = NULL;
                if ( ( errT < JET_errSuccess ) &&
                     ( errT != JET_errReadVerifyFailure ) &&
                     ( errT != JET_errPageNotInitialized ) &&
                     ( errT != JET_errReadLostFlushVerifyFailure ) &&
                     ( errT != JET_errDiskReadVerificationFailure ) &&
                     ( ( errT == JET_errDatabaseIdInUse ) ||
                       ( ( perrdata = PerrdataLookupErrValue( errT ) ) == NULL ) ||
                       ( perrdata->errorCategory == JET_errcatCorruption ) ) )
                {
                    AssertTrack( fFalse, OSFormat( "ShrinkFailAttachAlreadyAttached:%d", errT ) );
                    OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"b44c2c95-4e44-4947-babe-2255885d9eec" );
                    CallJ( errT, Detach );
                }

                pfmp->m_isdlAttach.Trigger( eAttachShrinkDone );
            }

            pfmp->ResetAttachedForRecovery();

            OSTrace( JET_tracetagDatabases, "Attach JET_wrnDatabaseAttached / KeepDirtyDbAttachAlive fast path; Skip to PostAttachTasks." );

            pfmp->m_isdlAttach.Trigger( eAttachFastBaseReAttachDone );

            #pragma pop_macro( "Call" )
            goto PostAttachTasks;
        }

        Assert( err < JET_errSuccess || ifmp == ifmpNil || ifmp == 0 || !g_rgfmp[ifmp].m_isdlAttach.FActiveSequence() );

        return err;
    }

    //  from this point we got a valid ifmp entry. Start the attaching DB process.
    //
    pfmp = &g_rgfmp[ ifmp ];

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlAttachBegin|sysosrtlContextFmp, pfmp, &ifmp, sizeof(ifmp) );

    Assert( !plog->FRecovering() );
    Assert( !pfmp->FIsTempDB() ); // Temp DBs are created, not [re]attached.

    
    pfmp->m_isdlAttach.InitSequence( isdltypeAttach, eAttachSeqMax );

    FMP::EnterFMPPoolAsWriter();
    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->SetAttachingDB( );
    pfmp->RwlDetaching().LeaveAsWriter();
    FMP::LeaveFMPPoolAsWriter();

    pfmp->SetRuntimeDbParams( cpgDatabaseSizeMax,
                              pctCachePriority,
                              grbitShrinkDatabaseOptions,
                              dtickShrinkDatabaseTimeQuota,
                              cpgShrinkDatabaseSizeLimit,
                              fLeakReclaimerEnabled,
                              dtickLeakReclaimerTimeQuota );

    //  backup thread will wait on attach completion from this point
    //  as the db is marked as attaching.
    //
    pbackup->BKUnlockBackup();

    //  set database loggable flags
    //
    if ( grbit & JET_bitDbRecoveryOff )
    {
        FMP::EnterFMPPoolAsWriter();
        pfmp->ResetLogOn();
        FMP::LeaveFMPPoolAsWriter();
    }
    else
    {
        //  set all databases loggable except Temp if not specified in grbit
        //
        FMP::EnterFMPPoolAsWriter();
        pfmp->SetLogOn();
        FMP::LeaveFMPPoolAsWriter();
    }

    // Can only turn versioning off for CreateDatabase().
    // UNDONE:  Is it useful to allow user to turn versioning off for AttachDatabase()?
    //
    Assert( !pfmp->FVersioningOff() );
    pfmp->ResetVersioningOff();

    //  set up FMP before logging
    //
    if ( grbit & JET_bitDbReadOnly )
    {
        FMP::EnterFMPPoolAsWriter();
        pfmp->ResetLogOn();
        FMP::LeaveFMPPoolAsWriter();
        
        pfmp->SetVersioningOff();
        pfmp->SetFDontRegisterOLD2Tasks();
        pfmp->SetReadOnlyAttach();
    }
    else
    {
        Assert( !pfmp->FDontRegisterOLD2Tasks() );
        pfmp->ResetReadOnlyAttach();
    }

    pfmp->m_isdlAttach.Trigger( eAttachInitVariableDone );

    //  Make sure the database is a good one

    Assert( UtilCmpFileName( pfmp->WszDatabaseName(), wszDbFullName ) == 0 );
    Assert( !( grbit & JET_bitDbReadOnly ) == !g_rgfmp[ifmp].FReadOnlyAttach() );
    Call( ErrDBReadHeaderCheckConsistency( pfsapi, pfmp ) );
    pfmp->TraceDbfilehdrInfo( tsidrEngineFmpDbHdr1st );

    Assert( ( pfmp->PLogRedoMapZeroed() == NULL ) && ( pfmp->PLogRedoMapBadDbTime() == NULL ) && ( pfmp->PLogRedoMapDbtimeRevert() == NULL ) );

    if ( !plog->FLogDisabled()
        && 0 == memcmp( &pfmp->Pdbfilehdr()->signLog, &plog->SignLog(), sizeof(SIGNATURE) ) )
    {
#if 0
        //  UNDONE: This logic detects if we are trying to detect a database that
        //  is too far in the future.  Re-enable this code when test scripts
        //  can properly handle this
        if ( CmpLgpos( &pdbfilehdr->lgposAttach, &plog->m_lgposLogRec ) > 0
            || CmpLgpos( &pdbfilehdr->lgposConsistent, &plog->m_lgposLogRec ) > 0 )
        {
            //  something is gravely wrong - the current lgposAttach
            //  and/or lgposConsistent are ahead of the current log position
            FireWall( "ConsistentTimeMismatchOnAttach" );
            Call( ErrERRCheck( JET_errConsistentTimeMismatch ) );
        }
#endif
    }

    err = ErrDBICheckVersions( pinst, wszDbFullName, ifmp, pfmp->Pdbfilehdr().get(), PfmtversEngineMax()->dbv, pfmp->FReadOnlyAttach() );
    Assert( err != JET_errEngineFormatVersionSpecifiedTooLowForDatabaseVersion ); // should be impossible.
    Call( err );

    pfmp->m_isdlAttach.Trigger( eAttachReadDBHeader );

    //  update header if upgrade needed
    if ( !pfmp->FReadOnlyAttach() )
    {
        //  log Attach
        Assert( pfmp == &g_rgfmp[ifmp] );
        Assert( UtilCmpFileName( wszDbFullName, pfmp->WszDatabaseName() ) == 0 );
        Assert( pfmp->CpgDatabaseSizeMax() == (UINT)cpgDatabaseSizeMax );
        Call( ErrLGAttachDB(
                ppib,
                ifmp,
                fSparseEnabledFile,
                &lgposLogRec ) );

        attachState = ATTACH_LOGGED;

        if ( 0 != lgposLogRec.lGeneration )
        {
            Assert( !plog->FRecovering() );

            //  HACK: we want the creation time of the generation with the
            //  AttachDB log record, and most of the time it should be the
            //  current log gen, but there are timing holes where it may
            //  not be (for instance, if the AttachDB log record happened
            //  to force a new log generation but it hasn't been created
            //  yet by the time we perform this check), so in such cases,
            //  just don't bother setting logtimeOfGenWithAttach (the only
            //  effect is that during redo, we won't be able to make the
            //  check that ensures that the timestamps match)
            //
            //  must grab m_critLGBuf because that's what 
            //  LOG::ErrLGUseNewLogFile() grabs when updating the contents
            //  of m_plgfilehdr
            //
            LOGTIME tmCreate;
            if ( lgposLogRec.lGeneration == plog->LGGetCurrentFileGenWithLock( &tmCreate ) )
            {
                logtimeOfGenWithAttach = tmCreate;
            }
        }

        pfmp->m_isdlAttach.FixedData().sAttachData.lgposAttach = lgposLogRec;
        Expected( pfmp->m_isdlAttach.FActiveSequence() );

        if ( pfmp->FLogOn() && BoolParam( pinst, JET_paramAggressiveLogRollover ) )
        {
            CHAR    szTrace[ 32 ];
            OSStrCbFormatA( szTrace, sizeof( szTrace ), "AttachDB (dbid:%d)", pfmp->Dbid() );
            Call( ErrLGForceLogRollover( ppib, szTrace ) );
        }

        // Below code to repro a bug due to a race condition
        DWORD tickSleep = (DWORD)UlConfigOverrideInjection( 38908, 0 );
        if ( tickSleep > 0 )
        {
            // Sleep for a bit to allow log rollover to complete and then
            // force checkpoint update while database cannot take updates
            UtilSleep( tickSleep );
            (void)pinst->m_plog->ErrLGUpdateCheckpointFile( fFalse );
        }

        //  Update database state to be dirty
        DBISetHeaderAfterAttach( pfmp->PdbfilehdrUpdateable().get(), lgposLogRec, &logtimeOfGenWithAttach, ifmp, fFalse ); // do not keep bkinfo
        Assert( pfmp->Pdbfilehdr()->le_objidLast );

        if ( !pfmp->FIsTempDB() )
        {
            //  create a flush map for this DB
            //
            Call( pfmp->ErrCreateFlushMap( grbit ) );
        }

        Call( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, wszDbFullName, pfmp ) );

        //  Once the header has been established for this attached database, we can set our waypoint.
        //
        //  a more logically-consistent place to do this is in
        //  DBISetHeaderAfterAttach() at the same place where
        //  lgposAttach is set, but we shouldn't do that because
        //  the waypoint should only be set after the header has
        //  been flushed (though technically, it doesn't matter
        //  in this particular case since we've just attached to
        //  the database so there's no danger of any other pages
        //  in the database being dirtied)
        //
        if ( pfmp->FLogOn() )
        {
            //  WARNING: even though we set the waypoint, waypoint
            //  advancement will not consider this database until
            //  FMP::SetAttached() is called below, so until that
            //  point, we cannot dirty any buffers for this
            //  database
            //
            FMP::EnterFMPPoolAsWriter();
            const LONG lgenMaxRequired = pfmp->Pdbfilehdr()->le_lGenMaxRequired;
            pfmp->SetWaypoint( lgenMaxRequired );
            FMP::LeaveFMPPoolAsWriter();
        }
        else
        {
            Assert( pinst->FComputeLogDisabled() );
            // note FMP::EnterFMPPoolAsWriter() is grabbed by ResetWaypoint()...
            pfmp->ResetWaypoint();
        }

        attachState = ATTACH_DB_UPDATED;
    }
    else
    {
        Assert( CmpLgpos( g_rgfmp[ ifmp ].LgposWaypoint(), lgposMin ) == 0 );
        Assert( pfmp->FReadOnlyAttach() );

        if ( !pfmp->FIsTempDB() )
        {
            //  create a flush map for this DB
            //
            Call( pfmp->ErrCreateFlushMap( grbit ) );
        }
    }

    pfmp->m_isdlAttach.Trigger( eAttachToLogStream );

    Call( ErrIOOpenDatabase( pfsapi, ifmp, wszDbFullName, pinst->FRecovering() ? iofileDbRecovery : iofileDbAttached, fSparseEnabledFile ) );

    //  if we fail after this, we must close the db
    attachState = ATTACH_DB_OPENED;

    pfmp->m_isdlAttach.Trigger( eAttachIOOpenDatabase );

    //  Make the database attached.

    Assert( !( pfmp->FAttached() ) );
    FMP::EnterFMPPoolAsWriter();
    pfmp->SetAttached();
    FMP::LeaveFMPPoolAsWriter();

    pfmp->InitializeDbtimeOldest();

    Assert( !pfmp->FAllowHeaderUpdate() );
    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->SetAllowHeaderUpdate();
    pfmp->RwlDetaching().LeaveAsWriter();

    #pragma push_macro( "Call" )
    #undef Call
    #define Call DO_NOT_USE_CALL_HERE_Use_CallJ_To_MoreAttachedThanDetached_Like_Others

    //  set the last page of the database and resize it (we normally do it at the end of recovery,
    //  but not when we decide to keep the DB cache alive or we replayed a shrink LR in the required range).
    OnDebug( PGNO pgnoLastBefore = pfmp->PgnoLast() );
    CallJ( ErrDBSetLastPage( ppib, ifmp ), MoreAttachedThanDetached );
    Assert( pgnoLastBefore >= pfmp->PgnoLast() );
    if ( !pfmp->FReadOnlyAttach() && !g_fRepair )
    {
        if ( !BoolParam( JET_paramEnableViewCache ) )
        {
            OnDebug( pgnoLastBefore = pfmp->PgnoLast() );
            CallJ( ErrIONewSize( ifmp, *tcScope, pfmp->PgnoLast(), 0, JET_bitNil ), MoreAttachedThanDetached );
            Assert( pgnoLastBefore == pfmp->PgnoLast() );
        }
    }

    {
    //  ensure our database has the expected size.
    //
    QWORD       cbLogicalSize       = OffsetOfPgno( pfmp->PgnoLast() + 1 );
    QWORD       cbNtfsSize          = 0;
    if ( pfmp->Pfapi()->ErrSize( &cbNtfsSize, IFileAPI::filesizeLogical ) >= JET_errSuccess )
    {
        AssertTrack( g_fRepair || cbNtfsSize >= cbLogicalSize, "AttachFileSizeTooSmall" );
    }
    }

    //  preread the first 16 pages of the database
    BFPrereadPageRange( ifmp, 1, 16, bfprfDefault, ppib->BfpriPriority( ifmp ), *tcScope );

    if ( !pfmp->FReadOnlyAttach() && !g_fRepair )
    {
        PGNO pgnoFDP;
        OBJID objidFDP;

        // Look up the ExtentPageCountCache table before we do anything that might affect ExtentPageCountCache values.
        // This sets up bookkeeping so any changes to space made by Update and Shrink are
        // correctly tracked.  Since we're not providing a creation function to this call,
        // it only looks up the table if it's there; it doesn't create it and doesn't error
        // if it's not there, just sets pgnoFDP and objidFDP to nulls.
        CallJ( ErrDBTryCreateSystemTable(
                   ppib,
                   ifmp,
                   szMSExtentPageCountCache,
                   NULL,
                   NO_GRBIT,
                   &pgnoFDP,
                   &objidFDP ),
               MoreAttachedThanDetached );
        g_rgfmp[ ifmp ].SetExtentPageCountCacheTableInfo( pgnoFDP, objidFDP );

        //  Upgrade the DB Version if necessary
        CallJ( ErrDBUpdateAndFlushVersion(
                    pinst,
                    wszDbFullName,
                    ppib,
                    ifmp,
                    fFalse ), MoreAttachedThanDetached );

        // Trigger leak reclaimer.
        JET_ERR errLeakReclaimer =  JET_errSuccess;
        if ( pfmp->FLeakReclaimerEnabled() )
        {
            errLeakReclaimer = ErrSPReclaimSpaceLeaks( ppib, ifmp );
            pfmp->m_isdlAttach.Trigger( eAttachLeakReclaimerDone );
        }

        //  Trigger DB shrink.
        if ( ( errLeakReclaimer >= JET_errSuccess ) &&
             pfmp->FShrinkDatabaseEofOnAttach() &&
             !BoolParam( JET_paramEnableViewCache ) &&
             pfmp->FEfvSupported( JET_efvShrinkEof ) )
        {
            const JET_ERR errT = ErrSHKShrinkDbFromEof( ppib, ifmp );

            // Fail the attach if we hit a corruption error, as safety measure to prevent the corruption from spreading.
            // Also, JET_errDatabaseInUse is used to signal that codepaths that require/assume exclusive access to the
            // database could not guarantee exclusivity, so fail the attach in that case for safety.
            const ErrData* perrdata = NULL;
            if ( ( errT < JET_errSuccess ) &&
                 ( errT != JET_errReadVerifyFailure ) &&
                 ( errT != JET_errPageNotInitialized ) &&
                 ( errT != JET_errReadLostFlushVerifyFailure ) &&
                 ( errT != JET_errDiskReadVerificationFailure ) &&
                 ( ( errT == JET_errDatabaseIdInUse ) ||
                   ( ( perrdata = PerrdataLookupErrValue( errT ) ) == NULL ) ||
                   ( perrdata->errorCategory == JET_errcatCorruption ) ) )
            {
                AssertTrack( fFalse, OSFormat( "ShrinkFailAttach:%d", errT ) );
                OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"e35a4768-a6c2-48a8-b9c9-b327f58ac806" );
                CallJ( errT, MoreAttachedThanDetached );
            }

            {
            //  ensure our database has the expected size.
            //
            QWORD       cbLogicalSize       = OffsetOfPgno( pfmp->PgnoLast() + 1 );
            QWORD       cbNtfsSize          = 0;
            if ( pfmp->Pfapi()->ErrSize( &cbNtfsSize, IFileAPI::filesizeLogical ) >= JET_errSuccess )
            {
                AssertTrack( g_fRepair || cbNtfsSize >= cbLogicalSize, "AttachFileSizeTooSmallAfterShrink" );
            }
            }

            pfmp->m_isdlAttach.Trigger( eAttachShrinkDone );
        }
    }

    //  Make the fmp available for others to open etc.
    FMP::EnterFMPPoolAsWriter();
    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->ResetAttachingDB();
    pfmp->ReleaseWriteLatch( ppib );
    pfmp->RwlDetaching().LeaveAsWriter();
    FMP::LeaveFMPPoolAsWriter();

    pfmp->m_isdlAttach.Trigger( eAttachSlowBaseAttachDone );

PostAttachTasks:

    #undef Call
    #define Call DO_NOT_USE_CALL_HERE_Use_CallJ_To_Detach_Like_Others

    err = JET_errSuccess;

    Expected( !pinst->FRecovering() ); // assert'ing this, b/c I can't find any evidence it can happen
    Expected( !FFMPIsTempDB( ifmp ) ); // same

    if ( !g_fRepair )
    {

        // Create this *before* creating the other system tables (such as
        // MSysLocales) so that MSObjids contains entries for those tables.
        CallJ( ErrDBTryCreateSystemTable( ppib, ifmp, szMSObjids, ErrCATCreateMSObjids, grbit ), Detach );
        CallJ( ErrCATPopulateMSObjids( ppib, ifmp ), Detach );
        g_rgfmp[ ifmp ].SetFMaintainMSObjids();
    }

    pfmp->m_isdlAttach.Trigger( eAttachCreateMSysObjids );

    if( !FFMPIsTempDB( ifmp)     && // just in case
        !pinst->FRecovering()    && // just in case
        !g_fRepair               &&
        !pfmp->FReadOnlyAttach()
        )
    {
        //  create the MSysLocales table if necessary (and init the facility)

        CallJ( ErrCATCreateOrUpgradeMSLocales( ppib, ifmp ), Detach );
        Assert( g_rgfmp[ ifmp ].PkvpsMSysLocales() );

        CallJ( ErrFaultInjection( 44400 ), Detach );

        //  update the header if necessary.

        if ( pfmp->Pdbfilehdr()->le_qwSortVersion != g_qwUpgradedLocalesTable )
        {
            pfmp->PdbfilehdrUpdateable()->le_qwSortVersion = g_qwUpgradedLocalesTable;
            CallJ( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ), Detach );
        }
    }

    pfmp->m_isdlAttach.Trigger( eAttachCreateMSysLocales );

    if( !FFMPIsTempDB( ifmp)     && // just in case
        !pinst->FRecovering()    && // just in case
        !g_fRepair               &&
        !pfmp->FReadOnlyAttach()
        )
    {
        PGNO  pgnoFDP;
        OBJID objidFDP;
        const WCHAR * rgwsz[] = { pfmp->WszDatabaseName(), NULL };

        // See if the table is there or not.
        CallJ( ErrDBTryCreateSystemTable(
                   ppib,
                   ifmp,
                   szMSExtentPageCountCache,
                   NULL,
                   NO_GRBIT,
                   &pgnoFDP,
                   &objidFDP ),
               Detach );

        // If one is NULL, both are NULL.
        Assert( ( pgnoFDP == pgnoNull ) == ( objidFDP == objidNil ) );

        MessageId evtId = PLAIN_TEXT_ID;                   // No-logging marker value.
        FEATURECONTROL fcT = FEATURECONTROL::fcNotSpecified; // Assume nothing to do.
        switch ( fcMaintainExtentPageCountCache )
        {
            case FEATURECONTROL::fcNotSpecified:
                // Caller didn't say anything about the cache, so we're going to look it up and use it if
                // it's there, but do nothing if it's not.
                if ( objidFDP != objidNil )
                {
                    evtId = EXTENT_PAGE_COUNT_CACHE_IN_USE_ID;
                    rgwsz[1] = L"EXISTING_STATE";
                }
                break;
                
            case FEATURECONTROL::fcDisableFromParam:
                evtId = EXTENT_PAGE_COUNT_CACHE_NOT_IN_USE_ID;
                if ( objidFDP != objidNil )
                {
                    fcT = FEATURECONTROL::fcDisableFromParam;
                    rgwsz[1] = L"REQUEST_PARAM";
                }
                else
                {
                    rgwsz[1] = L"REQUEST_PARAM_MATCHES_STATE";
                }
                break;
 
            case FEATURECONTROL::fcDisableFromOverride:
                evtId = EXTENT_PAGE_COUNT_CACHE_NOT_IN_USE_ID;
                if ( objidFDP != objidNil )
                {
                    fcT = FEATURECONTROL::fcDisableFromParam;
                    rgwsz[1] = L"OVERRIDE_PARAM";
                }
                else
                {
                    rgwsz[1] = L"OVERRIDE_PARAM_MATCHES_STATE";
                }
                break;

            case FEATURECONTROL::fcEnableFromParam:
                evtId = EXTENT_PAGE_COUNT_CACHE_IN_USE_ID;
                if ( objidFDP == objidNil )
                {
                    fcT = FEATURECONTROL::fcEnableFromParam;
                    rgwsz[1] = L"REQUEST_PARAM";
                }
                else
                {
                    rgwsz[1] = L"REQUEST_PARAM_MATCHES_STATE";
                }
                break;
                
            case FEATURECONTROL::fcEnableFromOverride:
                // We're only here if the dbparams were NotSpecified or DisableFromParam, but were
                // overridden by the registry.  We check the EFV before we try to create the table.
                // We ignore the override if the EFV doesn't support it (as opposed to trying
                // to create the table and generating an error).  A little confusing, but this
                // allows us to pass explicit EFV tests with the override set.
                if ( objidFDP != objidNil )
                {
                    Assert( pfmp->FEfvSupported( JET_efvExtentPageCountCache ) );
                    evtId = EXTENT_PAGE_COUNT_CACHE_IN_USE_ID;
                    rgwsz[1] = L"OVERRIDE_PARAM_MATCHES_STATE";
                }
                else if ( pfmp->FEfvSupported( JET_efvExtentPageCountCache ) )
                {
                    evtId = EXTENT_PAGE_COUNT_CACHE_IN_USE_ID;
                    fcT = FEATURECONTROL::fcEnableFromParam;
                    rgwsz[1] = L"OVERRIDE_PARAM";
                }
                else
                {
                    rgwsz[1] = L"OVERRIDE_PARAM_UNSUPPORTED";
                    evtId = EXTENT_PAGE_COUNT_CACHE_NOT_IN_USE_ID;
                }
                break;

            default:
                AssertSz( fFalse, "Unexpected case in switch.");
                CallJ( ErrERRCheck( JET_errInvalidParameter ), Detach );
        }

        if ( evtId != PLAIN_TEXT_ID )
        {
            UtilReportEvent(
                eventInformation,
                SPACE_MANAGER_CATEGORY,
                evtId,
                _countof( rgwsz ),
                rgwsz,
                0,
                NULL,
                pinst );
        }

        switch ( fcT )
        {
            case FEATURECONTROL::fcEnableFromParam:
                Assert( objidNil == objidFDP );
                CallJ(
                    ErrDBTryCreateSystemTable(
                        ppib,
                        ifmp,
                        szMSExtentPageCountCache,
                        ErrCATCreateMSExtentPageCountCache,
                        grbit,
                        &pgnoFDP,
                        &objidFDP
                        ),
                    Detach );
                Assert( objidNil != objidFDP );
                break;

            case FEATURECONTROL::fcDisableFromParam:
                Assert( objidNil != objidFDP );
                CallJ(
                    ErrCATDeleteMSExtentPageCountCache(
                        ppib,
                        ifmp,
                        EXTENT_CACHE_DELETE_REASON::FeatureOff
                        ),
                    Detach );
                pgnoFDP = pgnoNull;
                objidFDP = objidNil;
                break;

            default:
                Assert( FEATURECONTROL::fcNotSpecified == fcT );
                // Nothing.
                break;
        }

        g_rgfmp[ ifmp ].SetExtentPageCountCacheTableInfo( pgnoFDP, objidFDP );
    }

    //pfmp->m_isdlAttach.Trigger( eAttachProcessMSysExtentPageCountCache );

    {
    PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr();

    //  We will try to delete the (obsolete/deprecated) MSU if permitted.

    fDeleteMSUTable = ( !plog->FRecovering()
                        && !pfmp->FReadOnlyAttach()
                        && !g_fRepair );
    }

    if ( fDeleteMSUTable )
    {
        OSTrace( JET_tracetagUpgrade, OSFormat( "ErrCATDeleteMSU() upgrade\n" ) );
        CallJ( ErrCATDeleteMSU( ppib, ifmp ), Detach );
    }

    pfmp->m_isdlAttach.Trigger( eAttachDeleteMSysUnicodeFixup );

    BOOL fOsLocaleVersionOutOfDate = fFalse;

    // JET_paramEnableIndexCleanup was off by default pre-Win8 (despite being documented
    // as being on!). We're turning it on by default in Win8.
    //
    // When using JET_IndexCheckingDeferToOpenTable, we do not want to check MSysLocales to verify that we have out-of-date localized indices. While checking
    // MSysLocales is cheap enough, once it is out of date (remember we do not keep it up-to-date when we delete unicode indices), the cost to bring it back up-to-date
    // requires walking the whole catalog, which defeats the whole purpose of JET_IndexCheckingDeferToOpenTable.
    const BOOL fIndexCheckingEnabled = ( !pfmp->FReadOnlyAttach() && BoolParam( pinst, JET_paramEnableIndexCleanup ) ) || ( UlParam( pinst, JET_paramEnableIndexChecking ) == JET_IndexCheckingOn );

    //  if the user turned off index cleanup, then we won't even try to open the locales
    //  table and check for out-of-date locales ...

    if ( !plog->FRecovering()
            && !g_fRepair
            // this next clause is necessary ... 
            && ( fIndexCheckingEnabled ) )
    {
        //  Note: This code here adds a (at least 1) page read to the default attach 
        //  path to evaluate out of date indices.  It could be more than 1 page, but
        //  most likely only 1, as around 18 LCIDs+Versions fit on the first page of
        //  the MSysLocales table, assuming 4 KB pages.

        //  This used to be free as part of the early header reads, but was often 
        //  inaccurate and missed major / NLS version upgrades on non-English languages.
        CallJ( ErrCATCheckForOutOfDateLocales( ifmp, &fOsLocaleVersionOutOfDate ), Detach );
    }

    pfmp->m_isdlAttach.Trigger( eAttachCheckForOutOfDateLocales );

    if ( 0 != ( grbit & JET_bitDbDeleteUnicodeIndexes ) )
    {
        Assert( !pfmp->FReadOnlyAttach() );
        OSTrace( JET_tracetagUpgrade, OSFormat( "ErrDBDeleteUnicodeIndexes() upgrade\n" ) );
        CallJ( ErrDBDeleteUnicodeIndexes( ppib, ifmp ), Detach );

        pfmp->m_isdlAttach.Trigger( eAttachDeleteUnicodeIndexes );
        pfmp->m_isdlAttach.Trigger( eAttachUpgradeUnicodeIndexes );
    }
    else if ( fOsLocaleVersionOutOfDate && !pfmp->FReadOnlyAttach() )
    {
        pfmp->m_isdlAttach.Trigger( eAttachDeleteUnicodeIndexes );

        Assert( fIndexCheckingEnabled );

        OSTrace( JET_tracetagUpgrade, OSFormat( "ErrDBUpgradeForLocalisation() upgrade\n" ) );

        CallJ( ErrDBUpgradeForLocalisation( ppib, ifmp, grbit ), Detach );
        CallSx( err, JET_wrnCorruptIndexDeleted );

        pfmp->m_isdlAttach.Trigger( eAttachUpgradeUnicodeIndexes );
    }
    else
    {
        //  otherwise these two operations happen immediately
        pfmp->m_isdlAttach.Trigger( eAttachDeleteUnicodeIndexes );
        pfmp->m_isdlAttach.Trigger( eAttachUpgradeUnicodeIndexes );
    }

    //  We May need to update the headers now.
    if( !FFMPIsTempDB( ifmp ) &&   // just in case
        !pinst->FRecovering() &&            // just in case
        !g_fRepair &&
        !pfmp->FReadOnlyAttach() )
    {
        BOOL fUpdateHeaders = fFalse;

    { // .ctor acquires header lock at this point
        //  safe for updating if out of date ...
        PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
        if ( pdbfilehdr->le_dwMajorVersion != g_dwGlobalMajorVersion ||
            pdbfilehdr->le_dwMinorVersion != g_dwGlobalMinorVersion ||
            pdbfilehdr->le_dwBuildNumber != g_dwGlobalBuildNumber ||
            pdbfilehdr->le_lSPNumber != g_lGlobalSPNumber )
        {
            fUpdateHeaders = fTrue;
            pdbfilehdr->le_dwMajorVersion = g_dwGlobalMajorVersion;
            pdbfilehdr->le_dwMinorVersion = g_dwGlobalMinorVersion;
            pdbfilehdr->le_dwBuildNumber = g_dwGlobalBuildNumber;
            pdbfilehdr->le_lSPNumber = g_lGlobalSPNumber;
        }
    } // .dtor releases header lock at this point

        if ( fUpdateHeaders )
        {
            CallJ( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ), Detach );
        }
    }

    const ERR wrn = err;

    OSTrace( JET_tracetagDatabases, OSFormat( "Successfully attached database[0x%x]='%ws'", (ULONG)ifmp, wszDbFullName ) );

#ifdef DEBUG
    if ( !g_fRepair &&
            !pfmp->FReadOnlyAttach() )
    {
        Assert( JET_errDatabaseCorrupted != ErrCATVerifyMSLocales( ppib, ifmp, fFalse /* fixup table */ ) );
    }
#endif

    if ( !plog->FRecovering() && pfmp->FLogOn() && BoolParam( pinst, JET_paramDefragmentSequentialBTrees ) )
    {
        // FLogOn() should have protected us above.
        Assert( !g_fRepair );
        Assert( !pfmp->FReadOnlyAttach() );

        // re-register any OLD2 tasks which were running
        CallJ( ErrOLD2Resume( ppib, ifmp ), Detach );
    }

    if ( 0 != ( grbit & JET_bitDbEnableBackgroundMaintenance ) )
    {
        CallJ( pfmp->ErrStartDBMScan(), Detach );
    }

    if ( !g_fRepair &&
        !g_fEseutil &&
        g_fPeriodicTrimEnabled &&
        fAllowTrimDBTask &&
        !pfmp->FIsTempDB() &&
        !pfmp->FReadOnlyAttach() )
    {
        CallJ( ErrSPTrimDBTaskInit( ifmp ), Detach );
    }

    pfmp->m_isdlAttach.Trigger( eAttachDone );

    //  we won't log the temp DB

    if ( !pfmp->FIsTempDB() )
    {
        // report our successful attach and timings

        DBReportTachmentEvent( pinst, ifmp, ATTACH_DATABASE_DONE_ID, pfmp->WszDatabaseName(), fCacheAlive, fDirtyCacheAlive );
    }

    pfmp->m_isdlAttach.TermSequence();

    err = wrn;

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlAttachSucceed|sysosrtlContextFmp, pfmp, &ifmp, sizeof(ifmp) );

    return err;

    #pragma pop_macro( "Call" )

HandleError:

    ULONG rgul[4] = { (ULONG)ifmp, (ULONG)err, PefLastThrow()->UlLine(), UlLineLastCall() };
    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlAttachFail|sysosrtlContextFmp, pfmp, rgul, sizeof(rgul) );

    Assert( err < JET_errSuccess );
    Assert( pfmp != NULL );

    OSTrace(
        JET_tracetagDatabases,
        OSFormat(
            "Attach of database '%ws' failed with error %d (0x%x) at attach state %d",
            wszDbFullName,
            err,
            err,
            attachState ) );

    if ( attachState != ATTACH_NONE )
    {
        //  we have to take the instance offline if there is an error
        //  during attach and the attach is logged
        Assert( err < JET_errSuccess );
        pinst->SetInstanceUnavailable( err );
        ppib->SetErrRollbackFailure( err );
        DBReportPartiallyAttachedDb( pinst, wszDbFullName, attachState, err );
    }

    Assert( pfmp->CrefWriteLatch() == 1 );

    if ( FIODatabaseOpen( ifmp ) )
    {
        BFPurge( ifmp );
        (void)ErrIOFlushDatabaseFileBuffers( ifmp, iofrDefensiveCloseFlush );
        IOCloseDatabase( ifmp );
    }

    g_rgfmp[ ifmp ].AssertRangeLockEmpty();

    pfmp->m_isdlAttach.TermSequence();

    FMP::EnterFMPPoolAsWriter();
    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->ResetAttachingDB();
    pfmp->ResetAttached();
    pfmp->SetDetachingDB();
    pfmp->RwlDetaching().LeaveAsWriter();
    FMP::LeaveFMPPoolAsWriter();

    const ERR errVERClean = PverFromIfmp( ifmp )->ErrVERRCEClean( ifmp ) ;
    const BOOL fLeakFMP = ( errVERClean != JET_errSuccess );

    if ( fLeakFMP )
    {
        //  we have to take the instance offline if there is an error
        //  during database attachment and we could not clean version
        //  store entries for the database
        //
        PinstFromPpib( ppib )->SetInstanceUnavailable( err );
        ppib->SetErrRollbackFailure( err );

        FMP::EnterFMPPoolAsWriter();
        pfmp->RwlDetaching().EnterAsWriter();
        pfmp->ResetDetachingDB();
        pfmp->ReleaseWriteLatch( ppib );
        pfmp->RwlDetaching().LeaveAsWriter();
        FMP::LeaveFMPPoolAsWriter();
    }
    else
    {
        FMP::EnterFMPPoolAsWriter();
        pfmp->RwlDetaching().EnterAsWriter();
        DBResetFMP( pfmp, plog );
        pfmp->ReleaseWriteLatchAndFree( ppib );
        pfmp->RwlDetaching().LeaveAsWriter();
        FMP::LeaveFMPPoolAsWriter();
    }

    return err;

MoreAttachedThanDetached:

    //  I had issues with Call()s near end of attach process in that it dies with some sort of horrible instance 
    //  unavailable condition ... and ironically I found I could just Detach without even resetting attaching or
    //  _even_ releasing the write latch, but given the complicated stuff written in FMP::ReleaseWriteLatch(), I
    //  think it's safer to release it.  The test TestDowngradeCleanFrom102To101FailsAndCanReattach proves this 
    //  works.
    //  Let's leave it attaching (i.e., do not call pfmp->ResetAttachingDB()), so no one will use it through the detach.

    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->ReleaseWriteLatch( ppib );
    pfmp->RwlDetaching().LeaveAsWriter();

    //  fall through to detach

Detach:

    ULONG rgulD[4] = { (ULONG)ifmp, (ULONG)err, PefLastThrow()->UlLine(), UlLineLastCall() };
    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlAttachFail|sysosrtlContextFmp, pfmp, rgulD, sizeof(rgulD) );
    pfmp->m_isdlAttach.TermSequence();

    Assert( err < JET_errSuccess );
    Assert( pfmp != NULL );
    //  detach the database, ignoring any errors since an error already occurred
    (VOID)ErrIsamDetachDatabase( (JET_SESID) ppib, NULL, pfmp->WszDatabaseName() );
    return err;
}

ERR ErrDBUpgradeDatabase(
    JET_SESID   sesid,
    const WCHAR *wszDatabaseName,
    JET_GRBIT   grbit )
{
    return ErrERRCheck( JET_wrnNyi );
}

VOID DBResetFMP( FMP *pfmp, const LOG *plog )
{
    Assert( FMP::FWriterFMPPool() );

    if ( pfmp->FSkippedAttach() )
    {
        Assert( NULL != plog );
        pfmp->ResetSkippedAttach();
    }
    else if ( pfmp->FDeferredAttach() )
    {
        Assert( NULL != plog );
        Assert( plog->FRecovering() );
        pfmp->ResetDeferredAttach();
    }
    else
    {
        FCB::PurgeDatabase( pfmp->Ifmp(), fFalse /* fTerminating */ );
    }

    if ( pfmp->FRedoMapsEmpty() )
    {
        pfmp->FreeLogRedoMaps();
    }

    pfmp->DestroyFlushMap();

    pfmp->ResetAttached();
#ifdef DEBUG
    pfmp->SetDatabaseSizeMax( 0xffffffff );
#endif
    pfmp->ResetExclusiveOpen( );
    pfmp->ResetLogOn();
    pfmp->ResetReadOnlyAttach();
    pfmp->ResetVersioningOff();

    //  indicate this db entry is detached.

    OSMemoryHeapFree( pfmp->Patchchk() );
    pfmp->SetPatchchk( NULL );

    if ( pfmp->Pdbfilehdr() )
    {
        pfmp->FreePdbfilehdr();
    }

    if ( NULL != pfmp->Ppatchhdr() )
    {
        OSMemoryPageFree( pfmp->Ppatchhdr() );
        pfmp->SetPpatchhdr( NULL );
    }

    //  g_rgfmp[ifmp].szDatabaseName will be released within rwlFMPPool.
    //  other stream resources will be released within write latch.

    //  clean up fmp for future use

    Assert( !pfmp->Pfapi() );

    //  Free the FMP entry.

    pfmp->ResetDetachingDB();
    pfmp->ResetNoWaypointLatency();
    pfmp->ResetAttachedForRecovery();
}

LOCAL ERR ErrIsamDetachAllDatabase( JET_SESID sesid, const INT flags )
{
    ERR                 err         = JET_errSuccess;
    PIB                 *ppib       = PpibFromSesid( sesid );
    INST                *pinst      = PinstFromPpib( ppib );

    CallR( ErrPIBCheck( ppib ) );
    Assert ( 0 == ppib->Level() );

    FMP::EnterFMPPoolAsWriter();

    for ( DBID dbidDetach = dbidUserLeast; dbidDetach < dbidMax; dbidDetach++ )
    {
        IFMP ifmp = pinst->m_mpdbidifmp[ dbidDetach ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP *pfmp = &g_rgfmp[ ifmp ];
        if ( pfmp->FInUse() && pfmp->FAttached() )
        {
            FMP::LeaveFMPPoolAsWriter();

            FMP::AssertVALIDIFMP( pinst->m_mpdbidifmp[ dbidDetach ] );
            Assert ( NULL != g_rgfmp[ pinst->m_mpdbidifmp[ dbidDetach ] ].WszDatabaseName() );
            Call ( ErrIsamDetachDatabase( sesid, NULL, g_rgfmp[ pinst->m_mpdbidifmp[ dbidDetach ] ].WszDatabaseName(), flags ) );

            FMP::EnterFMPPoolAsWriter();
        }
    }

    FMP::LeaveFMPPoolAsWriter();

HandleError:
    return err;
}

//  szDatabaseName of NULL detaches all user databases.
//
ERR ISAMAPI ErrIsamDetachDatabase( JET_SESID sesid, IFileSystemAPI* const pfsapiDB, const WCHAR *wszDatabaseName, const INT flags )
{
    // check parameters
    //
    Assert( sizeof(JET_SESID) == sizeof(PIB *) );
    // there is no unknown flags set
    Assert( (flags & ~(0xf)) == 0 );

    ERR                 err         = JET_errSuccess;
    PIB                 *ppib       = PpibFromSesid( sesid );

    COSTraceTrackErrors trackerrors( __FUNCTION__ );

    CallR( ErrPIBCheck( ppib ) );

    if ( ppib->Level() > 0 )
    {
        return ErrERRCheck( JET_errInTransaction );
    }

    IFMP                ifmp            = ifmpNil;
    FMP                 *pfmp           = NULL;
    LGPOS               lgposLogRec;
    INST                *pinst          = PinstFromPpib( ppib );
    LOG                 *plog           = pinst->m_plog;
    BACKUP_CONTEXT      *pbackup        = pinst->m_pbackup;
    BOOL                fInCritBackup   = fFalse;
    BOOL                fDetachLogged   = fFalse;
    const size_t        cwchFullName    = IFileSystemAPI::cchPathMax;
    WCHAR               wszFullName[cwchFullName];

    if ( pinst->FInstanceUnavailable() )
    {
        return pinst->ErrInstanceUnavailableErrorCode();
    }
    Assert( JET_errSuccess == ppib->ErrRollbackFailure() );

    //  this should never be called on the temp database (e.g. we do not need to force the OS file-system)

    IFileSystemAPI  *pfsapi         = ( NULL == pfsapiDB ) ? pinst->m_pfsapi : pfsapiDB;

    if ( NULL == wszDatabaseName || 0 == *wszDatabaseName )
    {
        // this function will go through m_mpdbidifmp and call ErrIsamDetachDatabase for each one
        return ErrIsamDetachAllDatabase( sesid, flags );
    }

    OSTrace( JET_tracetagDatabases, OSFormat( "Session=[0x%p] detaching database[0x%x] %ws [flags=%d]",
                ppib, (ULONG)ifmp, wszDatabaseName, flags ) );

    err = ErrUtilPathComplete( pfsapi, wszDatabaseName, wszFullName, fTrue );
    if ( err == JET_errFileNotFound || err == JET_errInvalidPath )
    {
        CallR( FMP::ErrResolveByNameWsz( wszDatabaseName, wszFullName, cwchFullName ) );
    }
    CallR( err );

    if ( g_fPeriodicTrimEnabled )
    {
        SPTrimDBTaskStop( pinst, wszFullName );
    }

    pbackup->BKLockBackup();
    fInCritBackup = fTrue;

    if ( pbackup->FBKBackupInProgress() )
    {
        if ( pbackup->FBKBackupInProgress( BACKUP_CONTEXT::backupLocalOnly ) )
        {
            Call( ErrERRCheck( JET_errBackupInProgress ) );
        }
        else
        {
            Call( ErrERRCheck( JET_errSurrogateBackupInProgress ) );
        }
    }

    //  Use FMP::FDetachingDB() effectively like a critical section to make sure we don't turn
    //  tasks on/off concurrently by multiple threads racing in this path. We can't use the FMP
    //  latch returned by FMP::ErrWriteLatchByNameWsz() because it can't be held throughout the
    //  shutting down of the OLD/DBM/OLD2/Trim tasks, or we'll deadlock.

StartDetaching:

    Call( FMP::ErrWriteLatchByNameWsz( wszFullName, &ifmp, ppib ) );

    //  Retry until we have only one thread trying to detach the database.

    g_rgfmp[ifmp].RwlDetaching().EnterAsWriter();
    if ( g_rgfmp[ifmp].FDetachingDB() )
    {
        g_rgfmp[ifmp].RwlDetaching().LeaveAsWriter();
        g_rgfmp[ifmp].ReleaseWriteLatch( ppib );
        UtilSleep( 1 );
        goto StartDetaching;
    }
    g_rgfmp[ifmp].SetDetachingDB( );
    g_rgfmp[ifmp].RwlDetaching().LeaveAsWriter();

    //  From this point we got a valid ifmp entry. Start the detaching DB process.

    Assert( pfmp == NULL );
    pfmp = &g_rgfmp[ ifmp ];
    Assert( !pfmp->FIsTempDB() );

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDetachBegin|sysosrtlContextFmp, pfmp, &ifmp, sizeof(ifmp) );

    pfmp->m_isdlDetach.InitSequence( isdltypeDetach, eDetachSeqMax );

    pfmp->ReleaseWriteLatch( ppib );

    //  Terminate OLD / OLD2 here so that those sessions close the database

    OLDTermFmp( ifmp );

    if ( pfmp->PdbmFollower() )
    {
        pfmp->PdbmFollower()->DeRegisterFollower( &(g_rgfmp[ifmp]), CDBMScanFollower::dbmdrrStoppedScan );
        pfmp->DestroyDBMScanFollower();
        Assert( pfmp->PdbmFollower() == NULL );
    }
    pfmp->SetFDontStartDBM();
    pfmp->StopDBMScan();

    pfmp->SetFDontRegisterOLD2Tasks();
    OLD2TermFmp( ifmp );

    CallS( FMP::ErrWriteLatchByNameWsz( wszFullName, &ifmp, ppib ) );
    Assert( ifmp == pfmp->Ifmp() );

    Assert( !g_fPeriodicTrimEnabled || pfmp->FDontStartTrimTask() );
    Assert( !pfmp->FScheduledPeriodicTrim() );

    pfmp->m_isdlDetach.Trigger( eDetachQuiesceSystemTasks );

    Assert( pfmp->FDetachingDB( ) );

    if ( pfmp->CPin() )
    {
        Call( ErrERRCheck( JET_errDatabaseInUse ) );
    }

    if ( !pfmp->FAttached( ) )
    {
        Call( ErrERRCheck( JET_errDatabaseNotFound ) );
    }

    //  No new threads can come in and encouter corrupted pages, so now is the time
    //  to get rid of patch requests
    PagePatching::TermFmp( ifmp );

    pfmp->SetNoWaypointLatency();

    //  Enter a critical section to make sure no one, especially the
    //  checkpointer, looking for pdbfilehdr

    //  the version store will now process all tasks syncronously

    pbackup->BKUnlockBackup();
    fInCritBackup = fFalse;

    pfmp->m_isdlDetach.Trigger( eDetachSetDetaching );

    if ( !pfmp->FSkippedAttach()
        && !pfmp->FDeferredAttach() )
    {
        //  cleanup the version store to allow all tasks to be generated

        Call( PverFromIfmp( ifmp )->ErrVERRCEClean( ifmp ) );

        //  SOMEONE 06/09/99
        //
        //  CONSIDER: if there was an active user transaction and it commits
        //  after this call version store cleanup could generate tasks after
        //  the call to WaitForTasksToComplete which would cause an erroneous
        //  JET_errDatabaseInUse
        //
        //  The fix for this is to grab the version store cleanup critical section
        //  and to hold it over to calls to ErrVERRCEClean and WaitForTasksToComplete
    }

    pfmp->m_isdlDetach.Trigger( eDetachVersionStoreDone );

    //  Let all tasks active on this database complete
    //  From this point on, no additional tasks should be
    //  registered because:
    //      - OLD has terminated
    //      - the version store has been cleaned up
    //      - the database has been closed so no user actions can be performed
    pfmp->WaitForTasksToComplete();

    pfmp->m_isdlDetach.Trigger( eDetachStopSystemTasks );

    //  Clean up resources used by the ifmp.
    if ( !pfmp->FSkippedAttach()
        && !pfmp->FDeferredAttach() )
    {
        // Call RCE clean again, this time to clean versions on this db that
        // we may have missed.
        Call( PverFromIfmp( ifmp )->ErrVERRCEClean( ifmp ) );

        // All versions on this ifmp should be cleanable.
        if ( JET_wrnRemainingVersions == err )
        {
            Assert( fFalse );
            err = ErrERRCheck( JET_errDatabaseInUse );
            goto HandleError;
        }
    }

    pfmp->m_isdlDetach.Trigger( eDetachVersionStoreReallyDone );

    if ( FIODatabaseOpen( ifmp ) )
    {

        //  update gen required / waypoint for term
        //

        Call( PinstFromIfmp( ifmp )->m_plog->ErrLGUpdateWaypointIFMP( pfsapi, ifmp ) );

        //  flush all database buffers
        //

        Call( ErrBFFlush( ifmp ) );

        pfmp->m_isdlDetach.Trigger( eDetachBufferManagerFlushDone );

        //  purge all buffers for this ifmp
        //
        BFPurge( ifmp );

        pfmp->m_isdlDetach.Trigger( eDetachBufferManagerPurgeDone );
    }
    else
    {
        pfmp->m_isdlDetach.Trigger( eDetachBufferManagerFlushDone );
        pfmp->m_isdlDetach.Trigger( eDetachBufferManagerPurgeDone );
    }


    pfmp->AssertRangeLockEmpty();

    //  disable checkpoint update so we can detach DB safely

    Assert( UtilCmpFileName( wszFullName, pfmp->WszDatabaseName() ) == 0 );

    if ( !plog->FRecovering() )
    {
        plog->SetCheckpointEnabled( fFalse );
    }

    //  Now that we can forced the checkpoint to not write, we can FFB and be assured
    //  we have gotten to zero non-flushed IOs.

    if ( pfmp->Pfapi() )
    {
        Call( ErrIOFlushDatabaseFileBuffers( ifmp, iofrFlushIfmpContext ) );
    }

    //  log detach database

    Call( ErrLGDetachDB( ppib, ifmp, (BYTE)flags, &lgposLogRec ) );
    fDetachLogged = fTrue;
    pfmp->m_isdlDetach.FixedData().sDetachData.lgposDetach = lgposLogRec;

    if ( pfmp->FLogOn() && BoolParam( pinst, JET_paramAggressiveLogRollover ) )
    {
        CHAR    szTrace[ 32 ];
        OSStrCbFormatA( szTrace, sizeof( szTrace ), "DetachDB (dbid:%d)", pfmp->Dbid() );
        Call( ErrLGForceLogRollover( ppib, szTrace ) );
    }

    pfmp->m_isdlDetach.Trigger( eDetachLogged );

    // This cannot be undone (easily), so do this only after detach is logged and the we will not rollback the detach
    CATTermMSLocales( &(g_rgfmp[ifmp]) );
    Assert( NULL == pfmp->PkvpsMSysLocales() );

    if ( !plog->FRecovering() && ErrFaultInjection( 37004 ) < JET_errSuccess )
    {
        (VOID)plog->ErrLGUpdateCheckpointFile( fFalse );
        Call( ErrFaultInjection( 37004 ) );
    }

    // Now disallow header update by other threads (log writer or checkpoint advancement)
    // 1. For the log writer it is OK to generate a new log w/o updating the header as no log operations
    // for this db will be logged in new logs
    // 2. For the checkpoint: don't advance the checkpoint if db's header weren't update
    Assert( pfmp->FAllowHeaderUpdate() || pfmp->FReadOnlyAttach() );
    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->ResetAllowHeaderUpdate();
    pfmp->RwlDetaching().LeaveAsWriter();

    // Now, checkpoint maintenance cannot come in and update/flush the database header, assert that
    // there are no pending writes/flushes
    if ( pfmp->Pfapi() )
    {
        AssertTrack( pfmp->Pfapi()->CioNonFlushed() == 0, "UnexpectedPendingFlushesDetach" );
    }

    // Clean flush map.
    CFlushMapForAttachedDb* const pfm = pfmp->PFlushMap();
    if ( !pfmp->FReadOnlyAttach() && ( pfm != NULL ) )
    {
        Call( pfm->ErrCleanFlushMap() );
    }

    //  Update database file header. If we are detaching a bogus entry,
    //  then the db file should never be opened and pdbfilehdr will be Nil.


    /*  if attached before this detach.
     *  there should be no more operations on this database entry.
     *  detach it!!
     */
    { // .ctor acquires PdbfilehdrReadWrite
    PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
    const BKINFO *              pbkInfoToCopy   = pdbfilehdr ? &(pdbfilehdr->bkinfoFullCur) : NULL;
    const DBFILEHDR::BKINFOTYPE     bkinfoType      = DBFILEHDR::backupNormal;

    if ( pbkInfoToCopy && pbkInfoToCopy->le_genLow != 0 && !pfmp->FReadOnlyAttach() )
    {
        Assert( pbkInfoToCopy->le_genHigh != 0 );

        // don't update backup info on non-full backups (like Copy backups)
        if ( pbackup->FBKLogsTruncated() )
        {
            pdbfilehdr->bkinfoFullPrev = (*pbkInfoToCopy);
            pdbfilehdr->bkinfoTypeFullPrev = bkinfoType;
        }

        memset( &pdbfilehdr->bkinfoFullCur, 0, sizeof( BKINFO ) );
        memset( &pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );
        memset( &pdbfilehdr->bkinfoSnapshotCur, 0, sizeof( BKINFO ) );
        pdbfilehdr->bkinfoTypeIncPrev = DBFILEHDR::backupNormal;
    }
    } // .dtor releases PdbfilehdrReadWrite

    // delete the previous backup info on any hard recovery
    // this will prevent a incremental backup and log truncation problems
    // UNDONE: the above logic to copy bkinfoFullPrev is probably not needed
    // (we may consider this and delete it)

    if ( pfmp->FHardRecovery() )
    {
        memset( &pfmp->PdbfilehdrUpdateable()->bkinfoFullPrev, 0, sizeof( BKINFO ) );
        pfmp->PdbfilehdrUpdateable()->bkinfoTypeFullPrev = DBFILEHDR::backupNormal;
        pfmp->PdbfilehdrUpdateable()->bkinfoTypeIncPrev = DBFILEHDR::backupNormal;
    }

    if ( !pfmp->FReadOnlyAttach() && pfmp->Pdbfilehdr() )
    {

        Assert( !pfmp->FSkippedAttach() );
        Assert( !pfmp->FDeferredAttach() );

        //  If anything fail in this block, we simply occupy the FMP
        //  but bail out to the caller. The database in a state that it can
        //  not be used any more till next restore where FMP said it is
        //  detaching!

        // UNDONE: ask user to restart the engine.

        Assert( pfmp->LgposWaypoint().lGeneration <= pfmp->Pdbfilehdr()->le_lGenMaxRequired );
        pfmp->ResetWaypoint(); // reset waypoint

        pfmp->PdbfilehdrUpdateable()->le_dbtimeDirtied = pfmp->DbtimeLast();
        Assert( pfmp->Pdbfilehdr()->le_dbtimeDirtied != 0 );
        pfmp->PdbfilehdrUpdateable()->le_objidLast = pfmp->ObjidLast();
        Assert( pfmp->Pdbfilehdr()->le_objidLast != 0 );

        if ( plog->FRecovering() && plog->FRecoveringMode() == fRecoveringRedo &&
            pfmp->Pdbfilehdr()->Dbstate() == JET_dbstateDirtyAndPatchedShutdown )
        {
            //  For the JET_dbstateDirtyAndPatchedShutdown case we must maintain the same dbstate 
            //  and do not update the le_lGenMaxRequired because we are not done getting to a clean
            //  or consistent state yet.
        }
        else
        {
            Assert( pfmp->Pdbfilehdr()->Dbstate() != JET_dbstateDirtyAndPatchedShutdown );
            pfmp->PdbfilehdrUpdateable()->SetDbstate( JET_dbstateCleanShutdown );
        }

        if ( pfmp->FLogOn() )
        {
            Assert( !plog->FLogDisabled() );
            Assert( FSIGSignSet( &pfmp->Pdbfilehdr()->signLog ) );

            //  Set detachment time.

            if ( plog->FRecovering() && plog->FRecoveringMode() == fRecoveringRedo )
            {
                Assert( wszDatabaseName );
                Assert( 0 == CmpLgpos( plog->LgposLGLogTipNoLock(), pfmp->LgposDetach() ) );
                pfmp->PdbfilehdrUpdateable()->le_lgposDetach = pfmp->LgposDetach();
            }
            else
            {
                pfmp->PdbfilehdrUpdateable()->le_lgposDetach = lgposLogRec;
            }

            {
            PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
            pdbfilehdr->le_lgposConsistent = pdbfilehdr->le_lgposDetach;
            if ( pfmp->ErrDBFormatFeatureEnabled( pdbfilehdr.get(), JET_efvLgposLastResize ) == JET_errSuccess )
            {
                pdbfilehdr->le_lgposLastResize = pdbfilehdr->le_lgposConsistent;
            }
            }
        }

        {
        PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
        LGIGetDateTime( &pdbfilehdr->logtimeDetach );
        pdbfilehdr->logtimeConsistent = pdbfilehdr->logtimeDetach;
        }

        //  update the scrub information
        //  we wait until this point so we are sure all scrubbed pages have
        //  been written to disk

        pfmp->PdbfilehdrUpdateable()->le_dbtimeLastScrub    = pfmp->DbtimeLastScrub();
        pfmp->PdbfilehdrUpdateable()->logtimeScrub      = pfmp->LogtimeScrub();

        memset( &pfmp->PdbfilehdrUpdateable()->bkinfoSnapshotCur, 0, sizeof( BKINFO ) );

        Assert( pfmp->Pdbfilehdr()->le_objidLast );

        Call( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, wszDatabaseName, pfmp, pfmp->Pfapi() ) );

        Call( ErrIOFlushDatabaseFileBuffers( ifmp, iofrFlushIfmpContext ) );
        Assert( pfmp->Pfapi() == NULL || pfmp->Pfapi()->CioNonFlushed() == 0 );
    }

    //  snapshot the header info, retain the cache for this DB, in addition
    //  to its redo maps, if any,
    pfmp->SnapshotHeaderSignature();

    if ( pfm != NULL )
    {
        pfm->TermFlushMap();
    }

    pfmp->m_isdlDetach.Trigger( eDetachUpdateHeader );

    Assert( pfmp->Pfapi() == NULL || pfmp->Pfapi()->CioNonFlushed() == 0 );

    IOCloseDatabase( ifmp );

    pfmp->m_isdlDetach.Trigger( eDetachIOClose );

    if ( !plog->FRecovering() )
    {
        plog->SetCheckpointEnabled( fTrue );
    }

    //  Reset and free up FMP

    pfmp->ResetFDontRegisterOLD2Tasks();
    pfmp->ResetFDontStartDBM();
    pfmp->ResetFDontStartOLD();
    pfmp->ResetFDontStartTrimTask();

    FMP::EnterFMPPoolAsWriter();
    pfmp->RwlDetaching().EnterAsWriter();
    Assert( pfmp->FDetachingDB() );

    DBResetFMP( pfmp, plog );

    // report our successful detach and timings

    pfmp->m_isdlDetach.Trigger( eDetachDone );

    OSTrace( JET_tracetagDatabases, OSFormat( "Detached database[0x%x] %ws successfully",  (ULONG)ifmp, wszDatabaseName ) );
    DBReportTachmentEvent( pinst, ifmp, DETACH_DATABASE_DONE_ID, wszDatabaseName, fFalse );

    pfmp->ReleaseWriteLatchAndFree( ppib );
    pfmp->RwlDetaching().LeaveAsWriter();
    FMP::LeaveFMPPoolAsWriter();

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDetachSucceed|sysosrtlContextFmp, pfmp, &ifmp, sizeof(ifmp) );

    return JET_errSuccess;

HandleError:

    ULONG rgul[4] = { (ULONG)ifmp, (ULONG)err, PefLastThrow()->UlLine(), UlLineLastCall() };
    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDetachFail|sysosrtlContextFmp, pfmp, rgul, sizeof(rgul) );

    Assert( err < JET_errSuccess );

    if ( pfmp )
    {
        pfmp->m_isdlDetach.TermSequence();
    }

    //  do not reset detaching. We leave the database in detaching
    //  mode till next restore.

    if ( fDetachLogged )
    {
        //  if failure after detach logged, force shutdown to fix up database
        DBReportPartiallyDetachedDb( pinst, wszDatabaseName, err );
        ppib->SetErrRollbackFailure( err );
        pinst->SetInstanceUnavailable( err );

        Assert( NULL != pfmp );
        pfmp->ReleaseWriteLatch( ppib );
    }
    else if ( NULL != pfmp )
    {
        Assert( pfmp->FDetachingDB() );
        pfmp->ResetFDontRegisterOLD2Tasks();
        pfmp->ResetFDontStartDBM();
        pfmp->ResetFDontStartOLD();
        pfmp->ResetFDontStartTrimTask();

        pfmp->ResetNoWaypointLatency();

        pfmp->RwlDetaching().EnterAsWriter();
        Assert( pfmp->FDetachingDB() );
        pfmp->ResetDetachingDB( );
        pfmp->ReleaseWriteLatch( ppib );
        pfmp->RwlDetaching().LeaveAsWriter();
    }

    if ( fInCritBackup )
    {
        pbackup->BKUnlockBackup();
    }

    if ( !plog->FRecovering() )
    {
        plog->SetCheckpointEnabled( fTrue );
    }

    return err;
}



ERR ISAMAPI ErrIsamOpenDatabase(
    JET_SESID sesid,
    const WCHAR  *wszDatabaseName,
    const WCHAR  *wszConnect,
    JET_DBID *pjdbid,
    JET_GRBIT grbit )
{
    ERR     err;
    PIB * const ppib = (PIB *)sesid;
    IFMP    ifmp;
    INST    *pinst = PinstFromPpib( ppib );

    //  initialize return value
    Assert( pjdbid );
    *pjdbid = JET_dbidNil;

    //  check parameters
    //
    Assert( sizeof(JET_SESID) == sizeof(PIB *) );

    CallR( ErrPIBCheck( ppib ) );

    CallR( ErrDBOpenDatabase( ppib, wszDatabaseName, &ifmp, grbit ) );

    // we don't have any check to prevent JetOpenDatabase
    // using the temp database name.
    // we check now if we actualy opened the temp db
    if( !FUserIfmp( ifmp ) )
    {
        Assert( !g_rgfmp[ifmp].FAttachedForRecovery() );
        CallS ( ErrDBCloseDatabase( ppib, ifmp, NO_GRBIT ) );
        return ErrERRCheck( JET_errInvalidDatabase );
    }

    if ( pinst->FRecovering() )
    {
        Assert( g_rgfmp[ifmp].FAttachedForRecovery() );

        if ( g_rgfmp[ifmp].FContainsDataFromFutureLogs() )
        {
            AssertSz( fFalse, "JetOpenDatabase called too early" );
            CallS( ErrDBCloseDatabase( ppib, ifmp, NO_GRBIT ) );
            return ErrERRCheck( JET_errDatabaseNotReady );
        }

        if ( !g_rgfmp[ifmp].FRecoveryChecksDone() )
        {
            g_rgfmp[ifmp].CritOpenDbCheck().Enter();

            if ( !g_rgfmp[ifmp].FRecoveryChecksDone() )
            {
                // Do not check for JET_paramEnableIndexCleanup since a read-from-passive attach is always a read-only attach
                const BOOL fIndexCheckingEnabled = ( UlParam( pinst, JET_paramEnableIndexChecking ) == JET_IndexCheckingOn );
                if ( fIndexCheckingEnabled )
                {
                    BOOL fOsLocaleVersionOutOfDate = fFalse;
                    err = ErrCATCheckForOutOfDateLocales( ifmp, &fOsLocaleVersionOutOfDate );
                    if ( err < JET_errSuccess ||
                         fOsLocaleVersionOutOfDate )
                    {
                        if ( err < JET_errSuccess )
                        {
                            g_rgfmp[ifmp].CritOpenDbCheck().Leave();
                            CallS ( ErrDBCloseDatabase( ppib, ifmp, NO_GRBIT ) );
                            return err;
                        }
                        else
                        {
                            g_rgfmp[ifmp].CritOpenDbCheck().Leave();
                            Assert( fOsLocaleVersionOutOfDate );
                            CallS ( ErrDBCloseDatabase( ppib, ifmp, NO_GRBIT ) );
                            return ErrERRCheck( JET_errSecondaryIndexCorrupted );
                        }
                    }
                }
                g_rgfmp[ifmp].SetRecoveryChecksDone();
            }

            g_rgfmp[ifmp].CritOpenDbCheck().Leave();
        }
    }
    else if ( g_rgfmp[ifmp].FAttachedForRecovery() )
    {
        CallS ( ErrDBCloseDatabase( ppib, ifmp, NO_GRBIT ) );
        return ErrERRCheck( JET_errDatabaseAttachedForRecovery );
    }

    *pjdbid = (JET_DBID)ifmp;

    return JET_errSuccess;
}


ERR ErrDBOpenDatabase(
    PIB *ppib,
    __in PCWSTR wszDatabaseName,
    IFMP *pifmp,
    ULONG grbit )
{
    ERR                 err = JET_errSuccess;
    const size_t        cwchFullName = IFileSystemAPI::cchPathMax;
    WCHAR               rgwchFullName[cwchFullName];
    WCHAR               *wszFullName;
    WCHAR               *wszFileName;
    IFMP                ifmp = ifmpNil;
    INST                *pinst = PinstFromPpib( ppib );
    IFileSystemAPI      *pfsapi = NULL;
    const BOOL          fOpenForRecovery = !!( grbit & bitDbOpenForRecovery );

    if ( fOpenForRecovery )
    {
        Assert( NULL != wszDatabaseName );
        Assert( 0 != *wszDatabaseName );
        CallS( FMP::ErrWriteLatchByNameWsz( wszDatabaseName, &ifmp, ppib ) );
    }

    pfsapi = pinst->m_pfsapi;
    if ( fOpenForRecovery && ifmp != ifmpNil && !FFMPIsTempDB( ifmp ) )
    {
        wszFileName = wszFullName = g_rgfmp[ifmp].WszDatabaseName();
    }
    else
    {
        if ( NULL == wszDatabaseName || 0 == *wszDatabaseName )
        {
            return ErrERRCheck( JET_errDatabaseInvalidPath );
        }

        err = FMP::ErrResolveByNameWsz( wszDatabaseName, rgwchFullName, cwchFullName );
        if ( err == JET_errDatabaseNotFound )
        {
            err = ErrUtilPathComplete( pfsapi, wszDatabaseName, rgwchFullName, fFalse );
            Assert( JET_errFileNotFound != err );
        }
        CallR( err );

        wszFullName = rgwchFullName;
        wszFileName = wszFullName;
    }

    if ( !fOpenForRecovery )
    {
        Assert( rgwchFullName == wszFullName );
        CallR( FMP::ErrWriteLatchByNameWsz( wszFullName, &ifmp, ppib ) );
    }

    FMP *pfmp = &g_rgfmp[ ifmp ];

    if ( BoolParam( pinst, JET_paramOneDatabasePerSession )
        && !fOpenForRecovery
        && FUserIfmp( ifmp )
        && FSomeDatabaseOpen( ppib, ifmp ) )
    {
        Call( ErrERRCheck( JET_errOneDatabasePerSession ) );
    }

    //  during recovering, we could open an non-detached database
    //  to force to initialize the fmp entry.
    //  if database has been detached, then return error.
    //
    if ( !fOpenForRecovery && !pfmp->FAttached() )
    {
        Call( ErrERRCheck( JET_errDatabaseNotFound ) );
    }

    Assert( !pfmp->FSkippedAttach() );
    Assert( !pfmp->FDeferredAttach() );

    if ( pfmp->FReadOnlyAttach() && !( grbit & JET_bitDbReadOnly ) )
        err = ErrERRCheck( JET_wrnFileOpenReadOnly );

    if ( pfmp->FExclusiveByAnotherSession( ppib ) )
    {
        Call( ErrERRCheck( JET_errDatabaseLocked ) );
    }

    if ( grbit & JET_bitDbExclusive )
    {
        if( pfmp->CPin() > 0 )
        {
            Call( ErrERRCheck( JET_errDatabaseInUse ) );
        }
        pfmp->SetExclusiveOpen( ppib );

    }   //  if exclusive open

    Assert( pfmp->Pfapi() );
    DBSetOpenDatabaseFlag( ppib, ifmp );

    //  Allow others to open.

    pfmp->ReleaseWriteLatch( ppib );

    *pifmp = ifmp;
    return err;

HandleError:

    pfmp->ReleaseWriteLatch( ppib );
    return err;
}


ERR ISAMAPI ErrIsamCloseDatabase( JET_SESID sesid, JET_DBID ifmp, JET_GRBIT grbit )
{
    ERR     err;
    PIB     *ppib = (PIB *)sesid;

    //  check parameters
    //
    Assert( sizeof(JET_SESID) == sizeof(PIB *) );
    CallR( ErrPIBCheck( ppib ) );

    CallR( ErrDBCheckUserDbid( ifmp ) );

    CallR ( ErrDBCloseDatabase( ppib, ifmp, grbit ) );

    Assert( !BoolParam( PinstFromPpib( ppib ), JET_paramOneDatabasePerSession ) || !FSomeDatabaseOpen( ppib, ifmp ) );

    return JET_errSuccess;
}


ERR ISAMAPI ErrIsamSetMaxDatabaseSize(
    const JET_SESID     vsesid,
    const JET_DBID      vdbid,
    const ULONG         cpg,
    const JET_GRBIT     grbit )
{
    const IFMP          ifmp    = IFMP( vdbid );
    const ERR           err     = ErrDBCheckUserDbid( ifmp );
    CallSx( err, JET_errInvalidDatabaseId );

    if ( JET_errSuccess == err )
        g_rgfmp[ifmp].SetDatabaseSizeMax( cpg );

    return err;
}


ERR ISAMAPI ErrIsamGetMaxDatabaseSize(
    const JET_SESID     vsesid,
    const JET_DBID      vdbid,
    ULONG * const       pcpg,
    const JET_GRBIT     grbit )
{
    const IFMP          ifmp    = IFMP( vdbid );
    const ERR           err     = ErrDBCheckUserDbid( ifmp );
    CallSx( err, JET_errInvalidDatabaseId );

    if ( JET_errSuccess == err )
        *pcpg = g_rgfmp[ifmp].CpgDatabaseSizeMax();

    return err;
}


ERR ISAMAPI ErrIsamSetDatabaseSize( JET_SESID sesid, const WCHAR *wszDatabase, DWORD cpg, DWORD *pcpgReal )
{
    ERR             err         = JET_errSuccess;
    PIB*            ppib        = (PIB *)sesid;
    IFileSystemAPI* pfsapi      = PinstFromPpib( ppib )->m_pfsapi;
    DBFILEHDR_FIX*  pdbfilehdr  = NULL;
    IFileAPI*       pfapi       = NULL;
    QWORD           cbFileSize;

    if ( NULL == wszDatabase || 0 == *wszDatabase )
        return ErrERRCheck( JET_errDatabaseInvalidPath );

    if ( cpg < cpgDatabaseApiMinReserved )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    AllocR( pdbfilehdr = (DBFILEHDR_FIX*)PvOSMemoryPageAlloc( g_cbPage, NULL ) );

    //  this should never be called on the temp database (e.g. we will not need to force the OS file-system)
    Call( CIOFilePerf::ErrFileOpen( pfsapi,
                                    PinstFromPpib( ppib ),
                                    wszDatabase, 
                                    IFileAPI::fmfNone,
                                    iofileDbAttached,
                                    qwSetDbSizeFileID,
                                    &pfapi ) );
 
    Call( ErrUtilReadShadowedHeader(    PinstFromPpib( ppib ),
                                        pfsapi,
                                        pfapi,
                                        (BYTE *)pdbfilehdr,
                                        g_cbPage,
                                        OffsetOf( DBFILEHDR_FIX, le_cbPageSize ) ) );
 
    //  Disallow setting size on an inconsistent database

    if ( pdbfilehdr->Dbstate() == JET_dbstateIncrementalReseedInProgress )
    {
        Call( ErrERRCheck( JET_errDatabaseIncompleteIncrementalReseed ) );
    }
    else if ( pdbfilehdr->Dbstate() == JET_dbstateRevertInProgress )
    {
        Call( ErrERRCheck( JET_errDatabaseIncompleteRevert ) );
    }
    else if ( pdbfilehdr->Dbstate() != JET_dbstateCleanShutdown )
    {
        Call( ErrERRCheck( JET_errDatabaseInconsistent ) );
    }

    //  Set new database size only if it is larger than db size.

    Call( pfapi->ErrSize( &cbFileSize, IFileAPI::filesizeLogical ) );

    ULONG cpgNow;
    cpgNow = ULONG( ( cbFileSize / g_cbPage ) - cpgDBReserved );

    if ( cpgNow >= cpg )
    {
        *pcpgReal = cpgNow;
    }
    else
    {
        *pcpgReal = cpg;

        cbFileSize = g_cbPage * ( cpg + cpgDBReserved );
        
        PIBTraceContextScope tcSetSize = ppib->InitTraceContextScope();
        tcSetSize->iorReason.SetIorp( iorpDatabaseExtension );
        tcSetSize->SetDwEngineObjid( objidSystemRoot );

        Call( pfapi->ErrSetSize( *tcSetSize,
                                    cbFileSize,
                                    fTrue,
                                    QosSyncDefault( PinstFromPpib( ppib ) ) ) );

        //  No pinst, no ifmp, can't use any of the right functions.
        Call( ErrUtilFlushFileBuffers( pfapi, iofrDbResize ) );
    }

HandleError:
    OSMemoryPageFree( (void*)pdbfilehdr );
    if ( err < JET_errSuccess && pfapi != NULL )
    {
        Call( ErrUtilFlushFileBuffers( pfapi, iofrDefensiveErrorPath ) );
    }
    delete pfapi;
    return err;
}

ERR ISAMAPI ErrIsamResizeDatabase(
    __in JET_SESID      vsesid,
    __in JET_DBID       vdbid,
    __in ULONG  cpgTarget,
    _Out_opt_ ULONG *pcpgActual,
    __in JET_GRBIT      grbit )
{
    ERR     err                     = JET_errSuccess;
    PIB*    ppib                    = (PIB *)vsesid;
    const IFMP ifmp                 = (IFMP)vdbid;
    PGNO    pgnoAlloc               = pgnoNull;
    PGNO    pgnoLastCurrent         = pgnoNull;
    BOOL    fTransactionStarted     = fFalse;
    const CPG cpgExtensionSize      = (CPG)UlParam( PinstFromPpib( ppib ), JET_paramDbExtensionSize );
    const JET_GRBIT grbitValid      = ( JET_bitResizeDatabaseOnlyGrow | JET_bitResizeDatabaseOnlyShrink );
    CPG     cpgCurrent;
    CPG     cpgActual = 0;
    BOOL    fTrimmed = fFalse;

    if ( pcpgActual != NULL )
    {
        *pcpgActual = cpgActual;
    }

    // Invalid grbits.
    if ( ( grbit & ~grbitValid ) != 0 )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    // Invalid grbit combinations.
    if ( ( ( grbit & JET_bitResizeDatabaseOnlyGrow ) != 0 ) && ( ( grbit & JET_bitResizeDatabaseOnlyShrink ) != 0 ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    Call( ErrSPGetLastPgno( ppib, ifmp, &pgnoLastCurrent ) );

    cpgCurrent = pgnoLastCurrent + cpgDBReserved;
    cpgActual = cpgCurrent;

    if ( cpgCurrent == (CPG)cpgTarget )
    {
        // Do nothing
        goto DoneWithResizing;
    }
    else if ( (CPG)cpgTarget < cpgCurrent )
    {
        // We would shrink, but the client only wants growth.
        if ( ( ( grbit & JET_bitResizeDatabaseOnlyGrow ) != 0 ) )
        {
            goto DoneWithResizing;
        }

        // Fail if shrink is disabled.
        if ( ( ( GrbitParam( g_rgfmp[ ifmp ].Pinst(), JET_paramEnableShrinkDatabase ) & JET_bitShrinkDatabaseOn ) == 0 ) )
        {
            Error( ErrERRCheck( JET_errFeatureNotAvailable ) );
        }

        // Trim the database, regardless of whether the user specified JET_bitShrinkDatabaseRealtime
        // That parameter controls the in-line trimming, and we still want to support explicit
        // JetResizeDatabase() calls without setting that parameter.
        Call( ErrSPTrimRootAvail( ppib, ifmp, CPRINTFNULL::PcprintfInstance() ) );
        fTrimmed = fTrue;
    }
    else
    {
        // We would shrink, but the client only wants shrinkage.
        if ( ( ( grbit & JET_bitResizeDatabaseOnlyShrink ) != 0 ) )
        {
            goto DoneWithResizing;
        }

        // Grow the database...

        if ( BoolParam( JET_paramEnableViewCache ) )
        {
            Error( ErrERRCheck( JET_errFeatureNotAvailable ) );
        }

        CPG cpgExtend = (CPG)cpgTarget - cpgCurrent;
        Assert( cpgExtend > 0 );

        if ( ppib->Level() == 0 )
        {
            // Begin transaction as the space operations below
            // expect to run under a transaction.
            Call( ErrDIRBeginTransaction( ppib, 46920, NO_GRBIT ) );
            fTransactionStarted = fTrue;
        }
        
        // Same as above, we will round up by extension size.
        cpgExtend = roundup( cpgExtend, cpgExtensionSize );
        Assert( cpgExtend > 0 );

        // We will extend in extension size chunks so the OE is layed out in extension sizes
        // as well, which makes shrinking easier over time.
        CPG cpgTotal = 0;
        do
        {
            // Pass in fFalse for DBEXTENDTASK, so that the pcpgActual output parameter is
            // accurate.
            Call( ErrSPExtendDB( ppib, ifmp, cpgExtensionSize, &pgnoAlloc, fFalse ) );

            cpgTotal += cpgExtensionSize;
        }
        while ( cpgTotal < cpgExtend );

    }

DoneWithResizing:
    // Update the output parameter.
    QWORD cbActual = 0;

    if ( fTrimmed )
    {
        Call( g_rgfmp[ ifmp ].Pfapi()->ErrSize( &cbActual, IFileAPI::filesizeOnDisk ) );
    }
    else
    {
        Call( g_rgfmp[ ifmp ].Pfapi()->ErrSize( &cbActual, IFileAPI::filesizeLogical ) );
    }

    OSTraceFMP( ifmp, JET_tracetagSpaceInternal,
             OSFormat( "%hs: ifmp=%d had cpgActual=%d, now ErrSize is setting it to %d (cpgCurrent=%d; fTrimmed=%d).",
                       __FUNCTION__, ifmp, cpgActual, g_rgfmp[ ifmp ].CpgOfCb( cbActual ), cpgCurrent, fTrimmed ) );

    cpgActual = g_rgfmp[ ifmp ].CpgOfCb( cbActual );
    Assert( cpgActual > 0 );

    Assert( JET_errSuccess == err );

HandleError:
    if ( pcpgActual != NULL )
    {
        *pcpgActual = cpgActual;
    }

    if ( fTransactionStarted )
    {
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }

    return err;
}

ERR ErrDBCloseDatabase( PIB *ppib, IFMP ifmp, ULONG grbit )
{
    FMP     *pfmp = &g_rgfmp[ ifmp ];
    FUCB    *pfucb;
    FUCB    *pfucbNext;

    if ( !( FPIBUserOpenedDatabase( ppib, pfmp->Dbid() ) ) )
    {
        return ErrERRCheck( JET_errDatabaseNotFound );
    }

    /*  get a write latch on this fmp in order to change cPin.
     */
    pfmp->GetWriteLatch( ppib );

    Assert( FIODatabaseOpen( ifmp ) );
    if ( FLastOpen( ppib, ifmp ) )
    {
        //  close all open FUCBs on this database
        //

        //  get first table FUCB
        //
        pfucb = ppib->pfucbOfSession;
        while ( pfucb != pfucbNil
            && ( pfucb->ifmp != ifmp || !pfucb->u.pfcb->FPrimaryIndex() ) )
        {
            pfucb = pfucb->pfucbNextOfSession;
        }

        while ( pfucb != pfucbNil )
        {
            //  get next table FUCB
            //
            pfucbNext = pfucb->pfucbNextOfSession;
            while ( pfucbNext != pfucbNil
                && ( pfucbNext->ifmp != ifmp || !pfucbNext->u.pfcb->FPrimaryIndex() ) )
            {
                pfucbNext = pfucbNext->pfucbNextOfSession;
            }

            if ( !( FFUCBDeferClosed( pfucb ) ) )
            {
                CallS( pfucb->pvtfndef->pfnCloseTable( (JET_SESID)ppib, (JET_TABLEID) pfucb ) );
            }
            pfucb = pfucbNext;
        }
    }

    // if we opened it exclusively, we reset the flag

    DBResetOpenDatabaseFlag( ppib, ifmp );
    if ( pfmp->FExclusiveBySession( ppib ) )
        pfmp->ResetExclusiveOpen( );

    if ( ppib->FSessionOLD() )
    {
        Assert( pfmp->FRunningOLD() );
        pfmp->ResetRunningOLD();
    }

    // Release Write Latch

    pfmp->ReleaseWriteLatch( ppib );

    //  do not close file until file map space needed or database
    //  detached.

    return JET_errSuccess;
}


ERR ErrDBOpenDatabaseByIfmp( PIB *ppib, IFMP ifmp )
{
    ERR     err;

    //  Write latch the fmp since we are going to change cPin.

    CallR( FMP::ErrWriteLatchByIfmp( ifmp, ppib ) );

    //  The fmp we latch must be write latched by us and have
    //  a attached database.

    FMP *pfmp = &g_rgfmp[ ifmp ];
    Assert( pfmp->FWriteLatchByMe(ppib) );
    Assert( pfmp->FAttached() );


    // Allow LV create, RCE clean, and OLD sessions to bypass exclusive lock.
    if ( pfmp->FExclusiveByAnotherSession( ppib )
        && !FPIBSessionLV( ppib )
        && !FPIBSessionSystemCleanup( ppib ) )
    {
        //  It is opened by others already.
        err = ErrERRCheck( JET_errDatabaseLocked );
    }
    else
    {
        DBSetOpenDatabaseFlag( ppib, ifmp );

        if ( ppib->FSessionOLD() && !pfmp->FRunningOLD() )
        {
            Assert( !pfmp->FDontStartOLD() );
            pfmp->SetRunningOLD();
        }

        err = JET_errSuccess;
    }

    pfmp->ReleaseWriteLatch( ppib );

    return err;
}


//  ErrDBCloseAllDBs: Close all databases opened by this session
//
ERR ErrDBCloseAllDBs( PIB *ppib )
{
    DBID    dbid;
    ERR     err;
    INST    *pinst = PinstFromPpib( ppib );

    for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;
        while ( FPIBUserOpenedDatabase( ppib, dbid ) )
            CallR( ErrDBCloseDatabase( ppib, ifmp, 0 ) );
    }

    return JET_errSuccess;
}

ERR ErrDBFormatFeatureEnabled_( const JET_ENGINEFORMATVERSION efvFormatFeature, const DbVersion& dbvCurrentFromFile )
{
    //  Fast path - check assuming all persisted versions are current!
    //
    const FormatVersions * pfmtversMax = PfmtversEngineMax();

    Expected( efvFormatFeature <= pfmtversMax->efv ); // any EFV / feature to check should be < than the engines last EFV in the table.

    if ( efvFormatFeature <= pfmtversMax->efv &&
            //  then _most likely_ we're on the current version (as setting EFVs back is usually temporary) ...
            0 == CmpDbVer( pfmtversMax->dbv, dbvCurrentFromFile ) )
    {
        OSTrace( JET_tracetagVersionAndStagingChecks, OSFormat( "DB format feature EFV %d check success (fast path).\n", efvFormatFeature ) );
        return JET_errSuccess;  //  yay! fast out ...
    }
    //  else the header version didn't match (or the should be impossible case of EFV is 
    //  higher than last EFV in table) ... do slow method in that case.

    //  Slow path - search version table for EFV and check directly ...
    //

    const FormatVersions * pfmtversFormatFeature = NULL;
    CallS( ErrGetDesiredVersion( NULL /* must be NULL to bypass staging */, efvFormatFeature, &pfmtversFormatFeature ) );
    if ( pfmtversFormatFeature )
    {
        if ( CmpDbVer( dbvCurrentFromFile, pfmtversFormatFeature->dbv ) >= 0 )
        {
            OSTrace( JET_tracetagVersionAndStagingChecks, OSFormat( "DB format feature EFV %d check success (slow path).\n", efvFormatFeature ) );
            return JET_errSuccess;
        }
        else
        {
            OSTrace( JET_tracetagVersionAndStagingChecks, OSFormat( "DB format feature EFV %d check failed JET_errEngineFormatVersionParamTooLowForRequestedFeature (%d).\n", efvFormatFeature, JET_errEngineFormatVersionParamTooLowForRequestedFeature ) );
            return ErrERRCheck( JET_errEngineFormatVersionParamTooLowForRequestedFeature );
        }
    }

    AssertTrack( fFalse, "UnknownDbFormatFeatureDisabled" );

    return ErrERRCheck( errCodeInconsistency );
}
