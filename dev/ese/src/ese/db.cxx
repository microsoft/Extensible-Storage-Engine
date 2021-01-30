// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_logredomap.hxx"
#include "errdata.hxx"


LOCAL ERR ErrDBICheck200And400( INST *const pinst, IFileSystemAPI *const pfsapi, __in PCWSTR wszDatabaseName )
{
    
#include <pshpack1.h>


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
        ULONG       ulChecksum;
        ULONG       ulDBTime;
        PGNO        pgnoFDP;
        SHORT       cbFree;
        SHORT       ibMic;
        SHORT       ctagMac;
        SHORT       itagFree;
        SHORT       cbUncommittedFreed;
        THREEBYTES  pgnoPrev;
        THREEBYTES  pgnoNext;
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
                        sizeof(PGHDR) -
                        sizeof(TAG) -
                        sizeof(BYTE) -
                        sizeof(PGTYP) -
                        sizeof(THREEBYTES) ];
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

    
    pb = (BYTE *)ppage + ibTag;
    if ( !FNDVisibleSons( *pb ) || CbNDKey( pb ) != 0 || FNDNullSon( *pb ) )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    
    INT cb;
    cb = (INT)( cbTag - ( PbNDData( pb ) - pb ) );
    if ( cb != sizeof(P_DATABASE_DATA) )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    
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


bool FDBIsLVChunkSizeCompatible(
    const ULONG cbPage,
    const ULONG ulDaeVersion,
    const ULONG ulDaeUpdateMajor )
{
    Assert( ulDAEVersionMax == ulDAEVersionNewLVChunk );
    Assert( ulDAEUpdateMajorMax >= ulDAEUpdateMajorNewLVChunk );

    Assert( 2*1024 == cbPage
            || 4*1024 == cbPage
            || 8*1024 == cbPage
            || 16*1024 == cbPage
            || 32*1024 == cbPage );


    Assert( ulDAEVersionMax == ulDaeVersion || FNegTest( fInvalidAPIUsage ) || FInEseutilPossibleUsageError() );

    if( FIsSmallPage( cbPage ) )
    {
        return true;
    }

    if( ulDaeVersion == ulDAEVersionNewLVChunk
        && ulDaeUpdateMajor < ulDAEUpdateMajorNewLVChunk )
    {
        return false;
    }

    return true;
}

JETUNITTEST( DB, FDBIsLVChunkSizeCompatible  )
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

LOCAL ERR ErrDBInitDatabase( PIB *ppib, IFMP ifmp, CPG cpgPrimary )
{
    ERR             err;
    OBJID           objidFDP;

    CallR( ErrDIRBeginTransaction( ppib, 40229, NO_GRBIT ) );

    Call( ErrSPCreate(
                ppib,
                ifmp,
                pgnoNull,
                pgnoSystemRoot,
                cpgPrimary,
                fSPMultipleExtent,
                (ULONG)CPAGE::fPagePrimary,
                &objidFDP ) );
    Assert( objidSystemRoot == objidFDP );

    Call( ErrDIRCommitTransaction( ppib, 0 ) );

    return err;

HandleError:
    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

    return err;
}


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


VOID DBReportTachmentEvent( const INST * const pinst, const IFMP ifmp, const MessageId msgidTachment, PCWSTR wszDatabaseName, const BOOL fCacheAlive = fFalse, const BOOL fDirtyCacheAlive = fFalse )
{
    FMP * pfmp = &g_rgfmp[ifmp];

    Assert( pinst );
    Assert( pfmp );
    Assert( msgidTachment );
    Assert( wszDatabaseName );

    if ( pfmp->FIsTempDB() )
    {
        return;
    }


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


    OSStrCbFormatW( wszSeconds, sizeof(wszSeconds), L"%I64d", usecTach / 1000000  );
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


CPG         cpgLegacyDatabaseDefaultSize    = 256;

const CPG   cpgDatabaseApiMinReserved       = 32;

CPG CpgDBDatabaseMinMin()
{
    const CPG   cpgDatabaseMinMin8K = 30;
    const CPG   cpgDatabaseMinMin4K = 30;
    const CPG   cpgDatabaseMinMin2K = 31;

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

CPG CpgDBInitialUserDatabaseSize( const INST * const pinst )
{
    const CPG cpgEseBaseSize = max( CpgDBDatabaseMinMin(), 30 );


    const CPG cpgUserDataMin = ( FDefaultParam( pinst, JET_paramDbExtensionSize ) &&
                                        ( (CPG)UlParam( pinst, JET_paramDbExtensionSize ) == cpgLegacyDatabaseDefaultSize ) ) ?
                                    (CPG)( cpgLegacyDatabaseDefaultSize - cpgDBReserved - cpgEseBaseSize ) :
                                    UlFunctionalMin( (CPG)UlParam( pinst, JET_paramDbExtensionSize ),
                                            (CPG)( cpgLegacyDatabaseDefaultSize - cpgDBReserved - cpgEseBaseSize ) );


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

ERR ErrDBGetDesiredVersion(
    _In_ const INST * const                 pinst,
    _In_ const DBFILEHDR_FIX * const        pdbfilehdr,
    _Out_ const FormatVersions ** const     ppfmtvers,
    _Out_ BOOL * const                      pfAllowPersistedFormat )
{
    ERR err = JET_errSuccess;

    JET_ENGINEFORMATVERSION efv = (JET_ENGINEFORMATVERSION)UlParam( pinst, JET_paramEngineFormatVersion );


    const BOOL fAllowPersistedFormat = ( efv & JET_efvAllowHigherPersistedFormat ) == JET_efvAllowHigherPersistedFormat;
    if ( pfAllowPersistedFormat )
    {
        *pfAllowPersistedFormat = fAllowPersistedFormat;
    }
    if ( fAllowPersistedFormat )
    {
        efv = efv & ~JET_efvAllowHigherPersistedFormat;
    }


    if ( efv == JET_efvUsePersistedFormat )
    {
    
        if ( pdbfilehdr )
        {
            const FormatVersions * pfmtvers = NULL;
            CallR( ErrDBFindHighestMatchingDbMajors( pdbfilehdr->Dbv(), &pfmtvers, pinst ) );
            if ( err >= JET_errSuccess )
            {
                Assert( pdbfilehdr->le_ulVersion == pfmtvers->dbv.ulDbMajorVersion &&
                        pdbfilehdr->le_ulDaeUpdateMajor == pfmtvers->dbv.ulDbUpdateMajor );
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
    

    if ( dbvEngineMax.ulDbMajorVersion < pdbfilehdr->le_ulVersion ||
        dbvEngineMax.ulDbMajorVersion == pdbfilehdr->le_ulVersion && dbvEngineMax.ulDbUpdateMajor < pdbfilehdr->le_ulDaeUpdateMajor )
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


    if ( dbvDesired.ulDbMajorVersion < pdbfilehdr->le_ulVersion ||
        dbvDesired.ulDbMajorVersion == pdbfilehdr->le_ulVersion && dbvDesired.ulDbUpdateMajor < pdbfilehdr->le_ulDaeUpdateMajor )
    {
        WCHAR wszDatabaseVersion[50];
        WCHAR wszParamVersion[50];
        WCHAR wszParamEfv[cchFormatEfvSetting];
        const WCHAR * rgszT[5] = { wszDbFullName, wszDatabaseVersion, wszParamVersion, wszParamEfv, L""  };
        
        OSStrCbFormatW( wszDatabaseVersion, _cbrg( wszDatabaseVersion ), L"%d.%d.%d", (ULONG)pdbfilehdr->le_ulVersion, (ULONG)pdbfilehdr->le_ulDaeUpdateMajor, (ULONG)pdbfilehdr->le_ulDaeUpdateMinor );
        OSStrCbFormatW( wszParamVersion, _cbrg( wszParamVersion ), L"%d.%d.%d", dbvDesired.ulDbMajorVersion, dbvDesired.ulDbUpdateMajor, dbvDesired.ulDbUpdateMinor );
        FormatEfvSetting( (JET_ENGINEFORMATVERSION)UlParam( pinst, JET_paramEngineFormatVersion ), wszParamEfv, sizeof(wszParamEfv) );

        if ( fAllowPersistedFormat )
        {
#ifdef ENABLE_MICROSOFT_MANAGED_DATACENTER_LEVEL_OPTICS

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

            UtilReportEvent( eventError,
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


    if ( pdbfilehdr->le_efvMaxBinAttachDiagnostic < JET_efvSetDbVersion &&
            ErrDBFormatFeatureEnabled_( JET_efvSetDbVersion, dbvDesired ) >= JET_errSuccess )
    {
        if ( pfDbNeedsUpdate )
        {
            *pfDbNeedsUpdate = fTrue;
        }
    }


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


    err = ErrDBGetDesiredVersion( pinst,
            ( ifmp == ifmpNil  || !FFMPIsTempDB( ifmp ) ) ? pdbfilehdr : NULL,
            &pfmtversDesired,
            &fAllowPersistedFormat );
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

void DBISetVersion(
        _In_    const INST* const               pinst,
        _In_z_  const WCHAR* const              wszDbFullName,
        _In_    const IFMP                      ifmp,
        _In_    const DbVersion&                dbvUpdate,
        _Inout_ DBFILEHDR_FIX * const           pdbfilehdr,
        _In_    const BOOL                      fDbNeedsUpdate,
        _In_    const BOOL                      fCreateDbUpgradeDoNotLog )
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
                    ( pinst->FRecovering() && pinst->m_plog->FRecoveringMode() == fRecoveringRedo ) ||
                    g_fRepair );
        Expected( fUpdateCheckT ||
                    ( pinst->FRecovering() && pinst->m_plog->FRecoveringMode() == fRecoveringRedo &&
                        ( CmpDbVer( pdbfilehdr->Dbv(), dbvUpdate ) < 0 ) ) ||
                    g_fRepair );
    }

    if ( pdbfilehdr->le_ulCreateVersion == 0 )
    {
        Assert( pdbfilehdr->le_ulCreateUpdate == 0 );
        Assert( pdbfilehdr->le_ulVersion == 0 );
        Assert( pdbfilehdr->le_ulDaeUpdateMajor == 0 );
        Assert( pdbfilehdr->le_ulDaeUpdateMinor == 0 );
        Assert( pdbfilehdr->le_efvMaxBinAttachDiagnostic == 0 );
    }
#endif

    BOOL fUpdatedHeader = fFalse;

    Expected( pdbfilehdr->le_efvMaxBinAttachDiagnostic < PfmtversEngineMax()->efv ||
                CmpDbVer( pdbfilehdr->Dbv(), dbvUpdate ) < 0 ||
                g_fRepair );

    if ( pdbfilehdr->le_efvMaxBinAttachDiagnostic < PfmtversEngineMax()->efv &&
            ErrDBFormatFeatureEnabled_( JET_efvSetDbVersion, dbvUpdate ) >= JET_errSuccess )
    {
        OSTrace( JET_tracetagUpgrade, OSFormat( "Updating DB.le_efvMaxBinAttachDiagnostic to %d from %d\n", PfmtversEngineMax()->efv, (ULONG)pdbfilehdr->le_efvMaxBinAttachDiagnostic  ) );
        pdbfilehdr->le_efvMaxBinAttachDiagnostic = PfmtversEngineMax()->efv;
        OnDebug( ifmp != ifmpNil ? g_rgfmp[ifmp].SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusUpdateSet ) : 3 );
        fUpdatedHeader = fTrue;
    }

    if ( CmpDbVer( pdbfilehdr->Dbv(), dbvUpdate ) > 0 )
    {
        CHAR szFireWall[140];
        OSStrCbFormatA( szFireWall, sizeof(szFireWall), "InvalidDbVersionDowngradeAttempt-%I32u.%I32u.%I32u->%I32u.%I32u.%I32u",
                        (ULONG)pdbfilehdr->le_ulVersion, (ULONG)pdbfilehdr->le_ulDaeUpdateMajor, (ULONG)pdbfilehdr->le_ulDaeUpdateMinor,
                        dbvUpdate.ulDbMajorVersion, dbvUpdate.ulDbUpdateMajor, dbvUpdate.ulDbUpdateMinor );
        FireWall( szFireWall );
    }

    if ( CmpDbVer( pdbfilehdr->Dbv(), dbvUpdate ) < 0 )
    {
        DbVersion dbvCurr = pdbfilehdr->Dbv();

        pdbfilehdr->le_ulVersion = dbvUpdate.ulDbMajorVersion;
        pdbfilehdr->le_ulDaeUpdateMajor = dbvUpdate.ulDbUpdateMajor;
        OnDebug( ifmp != ifmpNil ? g_rgfmp[ifmp].SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusUpdateSet ) : 3 );

        if ( ErrDBFormatFeatureEnabled_( JET_efvSetDbVersion, dbvUpdate ) >= JET_errSuccess )
        {
#ifdef DEBUG
            const FormatVersions * pfmtverExthdrAutoInc = NULL;
            CallS( ErrGetDesiredVersion( NULL, JET_efvExtHdrRootFieldAutoIncStorageReleased, &pfmtverExthdrAutoInc ) );
            const BOOL fRequireLoggingForDbvUpdate = CmpDbVer( dbvUpdate, pfmtverExthdrAutoInc->dbv ) >= 0;
            Assert( !fRequireLoggingForDbvUpdate || ifmp == ifmpNil || !g_rgfmp[ifmp].FLogOn() || pinst->FRecovering() || pinst->m_plog->ErrLGFormatFeatureEnabled( JET_efvSetDbVersion ) >= JET_errSuccess );
#endif

            pdbfilehdr->le_ulDaeUpdateMinor = dbvUpdate.ulDbUpdateMinor;
            OnDebug( ifmp != ifmpNil ? g_rgfmp[ifmp].SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusUpdateSet ) : 3 );
        }

        if ( pdbfilehdr->le_ulCreateVersion == 0 )
        {
            pdbfilehdr->le_ulCreateVersion = dbvUpdate.ulDbMajorVersion;
            pdbfilehdr->le_ulCreateUpdate = dbvUpdate.ulDbUpdateMajor;

            OSTrace( JET_tracetagUpgrade, OSFormat( "Stamp Initial DB Version: %d.%d.%d (%d) - %ws\n",
                        (ULONG)pdbfilehdr->le_ulVersion, (ULONG)pdbfilehdr->le_ulDaeUpdateMajor, (ULONG)pdbfilehdr->le_ulDaeUpdateMinor,
                        (ULONG)pdbfilehdr->le_efvMaxBinAttachDiagnostic,
                        wszDbFullName ) );
        }
        else if ( ifmp != ifmpNil && ifmp != 0 && !FFMPIsTempDB( ifmp ) )
        {
            OSTrace( JET_tracetagUpgrade, OSFormat( "Upgrade DB Version: %d.%d.%d (Cr %d.%d) from %d.%d.%d - %ws\n",
                        (ULONG)pdbfilehdr->le_ulVersion, (ULONG)pdbfilehdr->le_ulDaeUpdateMajor, (ULONG)pdbfilehdr->le_ulDaeUpdateMinor,
                        (ULONG)pdbfilehdr->le_ulCreateVersion, (ULONG)pdbfilehdr->le_ulCreateUpdate,
                        dbvCurr.ulDbMajorVersion, dbvCurr.ulDbUpdateMajor, dbvCurr.ulDbUpdateMinor,
                        wszDbFullName ) );

            if ( !fCreateDbUpgradeDoNotLog )
            {

                WCHAR wszDbvBefore[50];
                WCHAR wszDbvAfter[50];
                WCHAR wszEfvSetting[cchFormatEfvSetting];
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

    if ( pinst->m_plog->ErrLGFormatFeatureEnabled( JET_efvSetDbVersion ) >= JET_errSuccess )
    {
        LGPOS logposT;

        Expected( CmpDbVer( pfmp->Pdbfilehdr()->Dbv(), pfmtversDesired->dbv ) <= 0 );

        Call( ErrLGSetDbVersion( ppib, ifmp, pfmtversDesired->dbv, &logposT ) );

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
            const FormatVersions * pfmtverSetDbVer = NULL;
            CallS( ErrGetDesiredVersion( NULL, JET_efvSetDbVersion, &pfmtverSetDbVer ) );
            Assert( CmpDbVer( pfmp->Pdbfilehdr()->Dbv(), pfmtverSetDbVer->dbv ) < 0 );
            Assert( pinst->m_plog->ErrLGFormatFeatureEnabled( JET_efvSetDbVersion ) < JET_errSuccess );
            pfmp->SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusUpdateLogged );
        }
#endif
    }

HandleError:

    return err;
}

ERR ErrDBUpdateAndFlushVersion(
    _In_    const INST* const       pinst,
    _In_z_  const WCHAR * const     wszDbFullName,
    _In_    PIB * const             ppib,
    _In_    const IFMP              ifmp,
    _In_    const BOOL              fCreateDbUpgradeDoNotLog )
{
    ERR err = JET_errSuccess;
    FMP * const pfmp = &g_rgfmp[ifmp];
    
    Assert( ifmp != ifmpNil && ifmp != 0 );
    Assert( !pinst->FRecovering() );
    Assert( pinst->m_plog->FRecoveringMode() != fRecoveringUndo );

    DbVersion dbvBefore;
    BOOL fDbNeedsUpdate = fFalse;
    const FormatVersions * pfmtversDesired = NULL;

    {
    PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr();

    dbvBefore = pdbfilehdr->Dbv();


    Call( ErrDBIValidateUserVersions( pinst, wszDbFullName, ifmp, pdbfilehdr.get(), &pfmtversDesired, &fDbNeedsUpdate ) );
    }


    if ( CmpDbVer( dbvBefore, pfmtversDesired->dbv ) > 0 )
    {
        Assert( !fDbNeedsUpdate );

        OSTrace( JET_tracetagUpgrade, OSFormat( "Suppress DB Version Downgrade LR: %d.%d.%d to %d.%d.%d - %ws\n",
                    dbvBefore.ulDbMajorVersion, dbvBefore.ulDbUpdateMajor, dbvBefore.ulDbUpdateMinor,
                    pfmtversDesired->dbv.ulDbMajorVersion, pfmtversDesired->dbv.ulDbUpdateMajor, pfmtversDesired->dbv.ulDbUpdateMinor,
                    wszDbFullName ) );
        return JET_errSuccess;
    }
    Assert( fDbNeedsUpdate || CmpDbVer( dbvBefore, pfmtversDesired->dbv ) == 0 );

    if ( g_rgfmp[ ifmp ].FLogOn() )
    {

        Call( ErrDBLGVersionUpdate( pinst, ppib, ifmp, pfmtversDesired, fDbNeedsUpdate ) );
    }

    {
    PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
    

    OnDebug( if ( ( rand() % 5 ) == 0 ) UtilSleep( 20 ) );

    AssertTrack( CmpDbVer( dbvBefore, pdbfilehdr->Dbv() ) == 0, "UnexpectedDbVersionChange" );

    if ( fDbNeedsUpdate )
    {

        DBISetVersion( pinst, wszDbFullName, ifmp, pfmtversDesired->dbv, pdbfilehdr.get(), fDbNeedsUpdate, fCreateDbUpgradeDoNotLog );
    }

    }

    Assert( CmpDbVer( pfmp->Pdbfilehdr()->Dbv(), pfmtversDesired->dbv ) >= 0 );

    if ( fDbNeedsUpdate )
    {
        Assert( pfmp->Pfapi() );
        err = ErrUtilWriteAttachedDatabaseHeaders( pinst, pinst->m_pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() );
    }

    Assert( !pfmp->FHeaderUpdateInProgress() || err < JET_errSuccess );

    AssertTrack( CmpDbVer( pfmp->Pdbfilehdr()->Dbv(), dbvBefore ) >= 0, "InvalidDbVersionDowngradeUpdate" );

HandleError:
    return err;
}

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


    Alloc( pdbfilehdrTestUpdate = (DBFILEHDR*)PvOSMemoryPageAlloc( g_cbPage, NULL ) );
    memcpy( pdbfilehdrTestUpdate, pdbfilehdr.get(), g_cbPage );
    Assert( CmpDbVer( pdbfilehdr->Dbv(), pdbfilehdrTestUpdate->Dbv() ) == 0 );
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
                fAllowPersistedFormat ) );


    if ( CmpDbVer( pdbfilehdr->Dbv(), dbvSet ) < 0 )
    {

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
        Enforce( !fRedo );
    }
    }

    if ( fRedo )
    {
        Assert( pfmp->Pfapi() );
        err = ErrUtilWriteAttachedDatabaseHeaders( pinst, pinst->m_pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() );
    }

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
    memset( pdbfilehdr, 0, g_cbPage );

    pdbfilehdr->le_ulMagic = ulDAEMagic;

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
    _Out_opt_ CPG* const                                        pcpgDatabaseSizeMax,
    _Out_opt_ ULONG* const                                      ppctCachePriority,
    _Out_opt_ JET_GRBIT* const                                  pgrbitShrinkDatabaseOptions,
    _Out_opt_ LONG* const                                       pdtickShrinkDatabaseTimeQuota,
    _Out_opt_ CPG* const                                        pcpgShrinkDatabaseSizeLimit,
    _Out_opt_ BOOL* const                                       pfLeakReclaimerEnabled,
    _Out_opt_ LONG* const                                       pdtickLeakReclaimerTimeQuota )
{
    if ( ( rgsetdbparam == NULL ) && ( csetdbparam > 0 ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    Expected( ( csetdbparam > 0 ) || ( rgsetdbparam == NULL ) );


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


    for ( ULONG isetdbparam = 0; isetdbparam < csetdbparam; isetdbparam++ )
    {
        const ULONG dbparamid = rgsetdbparam[ isetdbparam ].dbparamid;
        const void* const pvParam = rgsetdbparam[ isetdbparam ].pvParam;
        const ULONG cbParam = rgsetdbparam[ isetdbparam ].cbParam;
        void* pvParamDest = NULL;
        ULONG cbParamDest = 0;

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

            default:
                Expected( ( dbparamid >= JET_dbparamDbSizeMaxPages )  && ( dbparamid < ( JET_dbparamDbSizeMaxPages + 1024 ) ) );
                return ErrERRCheck( JET_errInvalidDbparamId );
        }


        if ( pvParamDest == NULL )
        {
            continue;
        }

        if ( cbParam != cbParamDest )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }

        if ( ( pvParam == NULL ) && ( cbParam > 0 ) )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }

        Expected( ( cbParam > 0 ) || ( pvParam == NULL ) );


        if ( cbParam > 0 )
        {
            Assert( ( pvParamDest != NULL ) && ( pvParam != NULL ) );
            Assert( cbParam == cbParamDest );
            UtilMemCpy( pvParamDest, pvParam, cbParamDest );
        }
    }


    if ( ( ppctCachePriority != NULL ) &&
            FIsCachePriorityAssigned( *ppctCachePriority ) &&
            !FIsCachePriorityValid( *ppctCachePriority ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

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

    if ( ( pdtickShrinkDatabaseTimeQuota != NULL ) &&
            ( *pdtickShrinkDatabaseTimeQuota > ( 7 * 24 * 60 * 60 * 1000 ) ) &&
            ( *pdtickShrinkDatabaseTimeQuota != -1 ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( ( pdtickLeakReclaimerTimeQuota != NULL ) &&
            ( *pdtickLeakReclaimerTimeQuota > ( 7 * 24 * 60 * 60 * 1000 ) ) &&
            ( *pdtickLeakReclaimerTimeQuota != -1 ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( ( pcpgShrinkDatabaseSizeLimit != NULL ) && ( *pcpgShrinkDatabaseSizeLimit < 0 ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
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

    CallR( ErrDBParseDbParams( rgsetdbparam,
                                csetdbparam,
                                &cpgDatabaseSizeMax,
                                &pctCachePriority,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL ) );

    Assert( !( grbit & bitCreateDbImplicitly ) || PinstFromPpib( ppib )->m_plog->FRecovering() );

    Assert( cpgPrimary );
    if ( dbidGiven == dbidTemp )
    {
        Assert( cpgPrimary >= cpgMultipleExtentMin );
    }
    else if ( grbit & bitCreateDbImplicitly )
    {
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

    if ( plog->FRecovering() && dbidGiven < dbidMax && dbidGiven != dbidTemp )
    {
        ifmp = pinst->m_mpdbidifmp[ dbidGiven ];
        FMP::AssertVALIDIFMP( ifmp );

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

    FMP *pfmp;
    pfmp = &g_rgfmp[ ifmp ];

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlCreateBegin|sysosrtlContextFmp, pfmp, &ifmp, sizeof(ifmp) );

    pfmp->m_isdlCreate.InitSequence( isdltypeCreate, eCreateSeqMax );

    OnDebug( Ptls()->fNoExtendingDuringCreateDB = fTrue );

    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->SetCreatingDB();
    pfmp->RwlDetaching().LeaveAsWriter();

    pfmp->SetRuntimeDbParams( cpgDatabaseSizeMax, pctCachePriority, NO_GRBIT, -1, 0, fFalse, -1 );

    if ( dbidGiven != dbidTemp )
    {
        err = ErrUtilPathExists( pfsapi, wszDbFullName );
        if ( JET_errFileNotFound != err )
        {
            if ( JET_errSuccess == err )
            {
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

        const JET_ENGINEFORMATVERSION efv = (JET_ENGINEFORMATVERSION)UlParam( pinst, JET_paramEngineFormatVersion );
        Assert( efv >= EfvMinSupported() );
        if ( JET_efvUsePersistedFormat == efv )
        {
            AssertSz( FNegTest( fInvalidAPIUsage ), "Specified JET_efvUsePersistedFormat during create DB, must have a specific version or JET_efvUseEngineDefault." );
            Call( ErrERRCheck( JET_errInvalidOperation ) );
        }
    }


    if ( grbit & JET_bitDbRecoveryOff )
    {
        pfmp->SetVersioningOff();
    }
    else
    {
        FMP::EnterFMPPoolAsWriter();
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

    DBEnforce( ifmp, CmpLgpos( pfmp->LgposWaypoint(), lgposMin ) == 0 );

    pfmp->SetDbtimeLast( grbit & bitCreateDbImplicitly ? 0 : dbtimeStart );

    pfmp->SetObjidLast( 0 );


    {
    DBFILEHDR * pdbfilehdrInit = NULL;
    Alloc( pdbfilehdrInit = (DBFILEHDR_FIX * )PvOSMemoryPageAlloc( g_cbPage, NULL ) );

    InitDBDbfilehdr( pfmp, plog, grbit, psignDb, pdbfilehdrInit );

    OSTrace( JET_tracetagDatabases, OSFormat( "Create DB Version %d.%d (%d.%d) - %ws\n", (ULONG)pdbfilehdrInit->le_ulVersion, (ULONG)pdbfilehdrInit->le_ulDaeUpdateMajor, (ULONG)pdbfilehdrInit->le_ulCreateVersion, (ULONG)pdbfilehdrInit->le_ulCreateUpdate, wszDatabaseName ) );

    Assert( pdbfilehdrInit->le_objidLast == pfmp->ObjidLast() );
    Assert( pdbfilehdrInit->le_dbtimeDirtied == pfmp->DbtimeLast() );

    DBFILEHDR * pdbfilehdrPrev;
    err = pfmp->ErrSetPdbfilehdr( pdbfilehdrInit, &pdbfilehdrPrev );
    if ( err < JET_errSuccess )
    {
        Assert( !pdbfilehdrPrev );
        OSMemoryPageFree( pdbfilehdrPrev );
        goto HandleError;
    }
    Assert( pdbfilehdrPrev == NULL );
    }

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

    {
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
    }

    pfmp->m_isdlCreate.Trigger( eCreateLogged );

    OnDebug( pfmp->SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusHdrLoaded ) );

    if ( dbidGiven != dbidTemp )
    {
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

    Call( ErrIOOpenDatabase( pfsapi, ifmp, wszDbFullName, pinst->FRecovering() ? iofileDbRecovery : iofileDbAttached, fSparseEnabledFile ) );

    if ( dbidGiven == dbidTemp )
    {
        Call( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, wszDbFullName, pfmp, pfmp->Pfapi() ) );

        pfmp->m_isdlCreate.Trigger( eCreateHeaderInitialized );
    }
 
    if ( pfmp->FLogOn() )
    {
        FMP::EnterFMPPoolAsWriter();
        const LONG lgenMaxRequired = pfmp->Pdbfilehdr()->le_lGenMaxRequired;
        pfmp->SetWaypoint( lgenMaxRequired );
        FMP::LeaveFMPPoolAsWriter();
    }
    else
    {
        pfmp->ResetWaypoint();
    }

    {
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    err = ErrIONewSize( ifmp, *tcScope, cpgPrimary, 0, JET_bitNil );
    }

    if ( err < JET_errSuccess )
    {

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

    DBSetOpenDatabaseFlag( ppib, ifmp );
    fDatabaseOpen = fTrue;

    pfmp->m_isdlCreate.Trigger( eCreateDatabaseZeroed );

    if ( ( dbidGiven != dbidTemp ) &&
         plog->FRecovering() &&
         !( grbit & bitCreateDbImplicitly ) )
    {
        OnDebug( Ptls()->fNoExtendingDuringCreateDB = fFalse );

        *pifmp = ifmp;
        return JET_errSuccess;
    }

    Call( ErrDBInitDatabase( ppib, ifmp, cpgPrimary ) );

    pfmp->m_isdlCreate.Trigger( eCreateDatabaseSpaceInitialized );

    if ( dbidGiven != dbidTemp )
    {
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

    if( dbidGiven != dbidTemp && !pinst->FRecovering() )
    {
        Call( ErrCATCreateMSObjids( ppib, ifmp ) );
        Call( ErrCATPopulateMSObjids( ppib, ifmp ) );
        pfmp->SetFMaintainMSObjids();
    }

    if( dbidGiven != dbidTemp && !pinst->FRecovering() )
    {

        Call( ErrCATCreateMSLocales( ppib, ifmp ) );
        Assert( pfmp->PkvpsMSysLocales() );
        Assert( JET_errDatabaseCorrupted != ErrCATVerifyMSLocales( ppib, ifmp, fFalse  ) );



        pfmp->PdbfilehdrUpdateable()->le_qwSortVersion = g_qwUpgradedLocalesTable;

        Call( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ) );

        Assert( pfmp->Pdbfilehdr()->le_qwSortVersion == g_qwUpgradedLocalesTable );
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
                Expected( !FIODatabaseOpen( ifmp ) );
                if ( FIODatabaseOpen( ifmp ) )
                {
                    Assert( g_rgfmp[ifmp].Pfapi()->CioNonFlushed() == 0 );
                    IOCloseDatabase( ifmp );
                }
                
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

    OnDebug( Ptls()->fNoExtendingDuringCreateDB = fFalse );

    pfmp->m_isdlCreate.Trigger( eCreateDone );


    if ( !pfmp->FIsTempDB() )
    {

        DBReportTachmentEvent( pinst, ifmp, CREATE_DATABASE_DONE_ID, pfmp->WszDatabaseName(), fFalse );
    }
    else
    {
        pfmp->m_isdlCreate.TermSequence();
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
        PinstFromPpib( ppib )->SetInstanceUnavailable( err );
        if ( JET_errSuccess == ppib->ErrRollbackFailure() )
        {
            ppib->SetErrRollbackFailure( err );
        }

        FCB::PurgeDatabase( ifmp, fTrue  );
    }
    else
    {
        FCB::PurgeDatabase( ifmp, fFalse  );
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

        if ( ( grbit & bitCreateDbImplicitly ) )
        {
            pfmp->ResetVersioningOff();

            FMP::EnterFMPPoolAsWriter();
            pfmp->SetLogOn();
            FMP::LeaveFMPPoolAsWriter();
        }
    }

    {
    PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
    pdbfilehdr->le_dbtimeDirtied = pfmp->DbtimeLast();
    LGIGetDateTime( &pdbfilehdr->logtimeAttach );

    pfmp->m_isdlCreate.Trigger( eCreateFinishLogged );

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

    if ( pfmp->FIsTempDB() || !pinst->FRecovering() )
    {
        pdbfilehdr->le_dwMajorVersion = g_dwGlobalMajorVersion;
        pdbfilehdr->le_dwMinorVersion = g_dwGlobalMinorVersion;
        pdbfilehdr->le_dwBuildNumber = g_dwGlobalBuildNumber;
        pdbfilehdr->le_lSPNumber = g_lGlobalSPNumber;
        QWORD qwSortVersion;
        CallS( ErrNORMGetSortVersion( wszLocaleNameDefault, &qwSortVersion, NULL ) );
        pdbfilehdr->le_qwSortVersion = qwSortVersion;
    }
    Assert( pdbfilehdr->le_objidLast );
    }


    if ( !pfmp->FIsTempDB() )
    {
        Call( ErrUtilWriteAttachedDatabaseHeaders( pinst, pinst->m_pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ) );

        DBEnforce( ifmp, pfmp->LgposWaypoint().lGeneration <= pfmp->Pdbfilehdr()->le_lGenMaxRequired );
    }

    if ( pfmp->FLogOn() )
    {
        FMP::EnterFMPPoolAsWriter();
        pfmp->SetWaypoint( lgenCurrent );
        FMP::LeaveFMPPoolAsWriter();
    }

    if ( !pfmp->FIsTempDB() )
    {
        Call( ErrBFFlush( ifmp ) );
    }

    pfmp->m_isdlCreate.Trigger( eCreateDatabaseDirty );

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

    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->ResetCreatingDB();
    pfmp->ReleaseWriteLatch( ppib );
    pfmp->RwlDetaching().LeaveAsWriter();

    if ( !pfmp->FIsTempDB() && pinst->FRecovering() )
    {
        Call( ErrLGDbAttachedCallback( pinst, pfmp ) );
    }

    
    if ( pfmp->FIsTempDB() )
    {
        return JET_errSuccess;
    }

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
                Expected( !FIODatabaseOpen( ifmp ) );
                if ( FIODatabaseOpen( ifmp ) )
                {
                    Assert( g_rgfmp[ifmp].Pfapi()->CioNonFlushed() == 0 );
                    IOCloseDatabase( ifmp );
                }
                
                (VOID)ErrIODeleteDatabase( pinst->m_pfsapi, ifmp );
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


    Assert( pfmp->Pdbfilehdr()->le_ulVersion != 0 );
    Assert( pfmp->Pdbfilehdr()->le_ulDaeUpdateMajor != 0 );

    pfmp->m_isdlCreate.Trigger( eCreateDone );


    if ( !pfmp->FIsTempDB() )
    {

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

    AllocR( pdbfilehdr = (DBFILEHDR * )PvOSMemoryPageAlloc( g_cbPage, NULL ) );

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
        if ( JET_errReadVerifyFailure == err )
        {
            if (    ulDAEMagic == pdbfilehdr->le_ulMagic
                 && ulDAEVersion500 == pdbfilehdr->le_ulVersion )
            {

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

    Assert( ulDAEVersionMax >= ulDAEVersionESE97 );

    if ( ulDAEMagic != pdbfilehdr->le_ulMagic )
    {
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }
    else if ( pdbfilehdr->le_ulVersion >= ulDAEVersionESE97 )
    {
        err = JET_errSuccess;
    }
    else
    {
        Assert( pdbfilehdr->le_ulVersion < ulDAEVersionMax );
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }

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
                if ( 0 != pdbfilehdr->bkinfoFullCur.le_genLow )
                {
                    const WCHAR *rgszT[1];

                    rgszT[0] = pfmp->WszDatabaseName();

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
                    err = ErrERRCheck( JET_errDatabaseInconsistent );
                }
            }
            goto HandleError;
        }
    }

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


    if ( !fKeepBackupInfo )
    {
        if ( !g_rgfmp[ifmp].FLogOn()
            || memcmp( &pdbfilehdr->signLog, &plog->SignLog(), sizeof( SIGNATURE ) ) != 0 )
        {

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
        memset( &pdbfilehdr->bkinfoSnapshotCur, 0, sizeof( BKINFO ) );
    }

    if ( g_fRepair )
    {
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

        pdbfilehdr->le_lgposAttach = lgposAttach;

        pdbfilehdr->le_lgposLastReAttach = lgposMin;

        if ( 0 != memcmp( &pdbfilehdr->signLog, &plog->SignLog(), sizeof(SIGNATURE) ) )
        {
            pdbfilehdr->le_lGenPreRedoMinRequired = 0;
            pdbfilehdr->le_lGenPreRedoMinConsistent = 0;

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
        SIGGetSignature( &pdbfilehdr->signDb );
    }

    LGIGetDateTime( &pdbfilehdr->logtimeAttach );

    pdbfilehdr->le_lgposDetach = lgposMin;
    memset( &pdbfilehdr->logtimeDetach, 0, sizeof( LOGTIME ) );


    pdbfilehdr->le_dbid = g_rgfmp[ ifmp ].Dbid();
}

ERR ErrDBTryCreateSystemTable(
    __in PIB * const ppib,
    const IFMP ifmp,
    const CHAR * const szTableName,
    ERR (*pfnCreate)(PIB * const, const IFMP),
    const JET_GRBIT grbit )
{
    Assert( ppib );
    Assert( ifmpNil != ifmp );
    Assert( szTableName );
    Assert( pfnCreate );

    ERR         err             = JET_errSuccess;
    WCHAR * const wszDatabaseName   = g_rgfmp[ifmp].WszDatabaseName();
    IFMP        ifmpT;

    if( grbit & JET_bitDbReadOnly || g_fRepair )
    {
        return JET_errSuccess;
    }

    Call( ErrDBOpenDatabase( ppib, wszDatabaseName, &ifmpT, NO_GRBIT ) );
    Assert( ifmp == ifmpT );

    if( JET_wrnFileOpenReadOnly == err )
    {
        err = JET_errSuccess;
    }
    else
    {

        err = ErrCATSeekTable( ppib, ifmpT, szTableName, NULL, NULL );
        if( err < JET_errSuccess )
        {
            if( JET_errObjectNotFound == err )
            {
                err = (*pfnCreate)( ppib, ifmp );

#ifdef DEBUG
                if ( JET_errSuccess == err )
                {
                    const ERR errT = ErrCATSeekTable( ppib, ifmpT, szTableName, NULL, NULL );
                    AssertSz( JET_errSuccess == errT, "ErrDBTryCreateSystemTable didn't create the specified table" );
                }
#endif
            }
        }
    }

    CallS( ErrDBCloseDatabase( ppib, ifmpT, 0 ) );

    Call( err );

    return err;

HandleError:

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
            VERSignalCleanup( ppib );
        }
    }

    CallS( ErrDBCloseDatabase( ppib, ifmpT, 0 ) );
    Call( err );


    Call( ErrCATVerifyMSLocales( ppib, ifmp, fTrue  ) );
    
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

    Expected( 0 == ( grbit & JET_bitDbDeleteUnicodeIndexes ) );

    BOOL        fReadOnly;

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


    {
    PdbfilehdrReadWrite pdbfilehdr = g_rgfmp[ifmp].PdbfilehdrUpdateable();
    pdbfilehdr->le_dwMajorVersion = g_dwGlobalMajorVersion;
    pdbfilehdr->le_dwMinorVersion = g_dwGlobalMinorVersion;
    pdbfilehdr->le_dwBuildNumber = g_dwGlobalBuildNumber;
    pdbfilehdr->le_lSPNumber = g_lGlobalSPNumber;
    pdbfilehdr->ResetUpgradeDb();
    }


    Call( ErrCATVerifyMSLocales( ppib, ifmp, fTrue  ) );

#ifndef RTM
    AssertRTL( JET_errDatabaseCorrupted != ErrCATVerifyMSLocales( ppib, ifmp, fFalse  ) );
#endif

    if ( fIndexesDeleted )
    {
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

ERR ErrDBSetUserDbHeaderInfo(
    FMP * const         pfmp,
    ULONG               cbdbinfomisc,
    JET_DBINFOMISC7 *   pdbinfomisc )
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



LOCAL ERR ErrDBReadDBTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    const WCHAR * const wszFileName,
    __out_bcount( cbTrailer ) BYTE * const pbTrailer,
    const INT cbTrailer )
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

ERR ErrDBTryToZeroDBTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    const WCHAR * const wszDatabase)
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

    err = ErrDBReadAndCheckDBTrailer( pinst, pfsapi, wszDatabase, pbTrailer, g_cbPage );
    if( err < JET_errSuccess )
    {
        
        
        err = JET_errSuccess;
        goto HandleError;
    }

    Call( pfsapi->ErrFileOpen(
            wszDatabase,
            BoolParam( JET_paramEnableFileCache ) ? IFileAPI::fmfCached : IFileAPI::fmfNone,
            &pfapi ) );
            
    Call( pfapi->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );

    if( cbSize < QWORD ( cpgDBReserved + 1 )  * g_cbPage )
    {
        OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"e893b103-65f9-4833-9065-f484904fe609" );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    memset( pbTrailer, 0, g_cbPage );

    Call( pfapi->ErrIOWrite( *tcScope, cbSize - QWORD(g_cbPage), g_cbPage, pbTrailer, qosIONormal ) );
    CallS( err );

    Call( ErrUtilFlushFileBuffers( pfapi, iofrUtility ) );
    
HandleError:
    OSMemoryPageFree( pbTrailer );
    delete pfapi;
    return err;
}



ERR ErrDBReadAndCheckDBTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    const WCHAR * const wszFileName,
    __out_bcount( cbBuffer ) BYTE * const pbBuffer,
    const INT cbBuffer )
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
            Call( ErrERRCheck( JET_errBadPatchPage ) );
        }
    }

    CallS( err );
HandleError:
    return err;
}


ERR ErrDBCheckDBHeaderAndTrailer(
    const DBFILEHDR * const pdbfilehdr,
    const PATCHHDR * const ppatchhdr,
    const WCHAR * const wszDatabase,
    BOOL fSkipMinLogChecks )
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

BOOL FDBHeaderNeedsUpdating(const DBFILEHDR * const pdbfilehdr)
{
    if ( 0 == pdbfilehdr->bkinfoFullCur.le_genLow
        || pdbfilehdr->bkinfoFullCur.le_genHigh )
    {
        return fFalse;
    }
    return fTrue;
}


LOCAL ERR ErrDBIUpdateHeaderFromTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    DBFILEHDR   * const pdbfilehdr,
    const PATCHHDR * const ppatchhdr,
    const WCHAR * const wszDatabase,
    BOOL fSkipMinLogChecks )
{
    Assert( pfsapi );
    Assert( pdbfilehdr );
    Assert( ppatchhdr );
    Assert( wszDatabase );

    ERR             err         = JET_errSuccess;

    CallR( ErrDBCheckDBHeaderAndTrailer( pdbfilehdr, ppatchhdr, wszDatabase, fSkipMinLogChecks ) );

    Assert( FDBHeaderNeedsUpdating( pdbfilehdr ) || fSkipMinLogChecks );

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

    if ( ( lgenMinReqPatch > 0 ) &&
         ( lgenMinReqPatch < pdbfilehdr->le_lGenMinRequired ) )
    {
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

    pdbfilehdr->le_lGenMinConsistent = pdbfilehdr->le_lGenMinRequired;
    pdbfilehdr->le_lGenPreRedoMinConsistent = 0;
    pdbfilehdr->le_lGenPreRedoMinRequired = 0;

    Assert( !FDBHeaderNeedsUpdating( pdbfilehdr ) );

    CallR( ErrUtilWriteUnattachedDatabaseHeaders( pinst, pfsapi, wszDatabase, pdbfilehdr ) );

    CallS( err );

    return err;
}


ERR ErrDBUpdateHeaderFromTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    DBFILEHDR   * const pdbfilehdr,
    const WCHAR * const wszDatabase,
    BOOL        fSkipMinLogChecks )
{
    Assert( pfsapi );
    Assert( pdbfilehdr );
    Assert( wszDatabase );

    ERR             err         = JET_errSuccess;
    PATCHHDR *      ppatchhdr   = NULL;;

    Alloc( ppatchhdr = static_cast<PATCHHDR *>( PvOSMemoryPageAlloc( g_cbPage, NULL ) ) );

    if( attribDb != pdbfilehdr->le_attrib )
    {
        OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"0fcb4d48-e21a-4862-95d3-1b599eb63d51" );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if( !FDBHeaderNeedsUpdating( pdbfilehdr ) && !fSkipMinLogChecks )
    {
        CallS( err );

        Call( ErrDBTryToZeroDBTrailer( pinst,
                                       pfsapi,
                                       wszDatabase ) );

        CallS( err );
        
        goto HandleError;
    }

    Call( ErrDBReadAndCheckDBTrailer( pinst, pfsapi, wszDatabase, reinterpret_cast<BYTE*>( ppatchhdr ), g_cbPage ) );
    Call( ErrDBIUpdateHeaderFromTrailer( pinst, pfsapi, pdbfilehdr, ppatchhdr, wszDatabase, fSkipMinLogChecks ) );
    CallS( err );

    Call( ErrDBTryToZeroDBTrailer( pinst,
                                   pfsapi,
                                   wszDatabase ) );
    CallS( err );

HandleError:
    if ( err == JET_errBadPatchPage )
    {
        const WCHAR *rgszT[1];
        rgszT[0] = wszDatabase;

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


LOCAL ERR ErrDBIUpdateHeaderFromTrailer(
    const INST * const pinst,
    IFileSystemAPI * const pfsapi,
    const WCHAR * const wszDatabase,
    BOOL fSkipMinLogChecks )
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


ERR ErrDBUpdateHeaderFromTrailer( const WCHAR * const wszDatabase, BOOL fSkipMinLogChecks )
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
                                &dtickLeakReclaimerTimeQuota ) );

    memset( &logtimeOfGenWithAttach, 0, sizeof( logtimeOfGenWithAttach ) );

    COSTraceTrackErrors trackerrors( __FUNCTION__ );

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

    if ( ( grbit & JET_bitDbDeleteUnicodeIndexes ) && ( grbit & JET_bitDbReadOnly ) )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    pfsapi = pinst->m_pfsapi;

    err = ErrUtilPathComplete( pfsapi, wszDatabaseName, rgchDbFullName, fTrue );
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
                CallS( err );
        }
#endif
        pbackup->BKUnlockBackup();

        if ( err == JET_wrnDatabaseAttached &&
             g_rgfmp[ ifmp ].FAttachedForRecovery() )
        {
            #pragma push_macro( "Call" )
            #undef Call
            #define Call DO_NOT_USE_CALL_HERE_Use_CallJ_To_Detach_Like_Others

            pfmp = &g_rgfmp[ ifmp ];

            Assert( ( pfmp->PLogRedoMapZeroed() == NULL ) && ( pfmp->PLogRedoMapBadDbTime() == NULL ) );

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
            PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr();

            dbvBefore = pdbfilehdr->Dbv();

            CallJ( ErrDBIValidateUserVersions( pinst, wszDbFullName, ifmp, pdbfilehdr.get(), &pfmtversDesired, &fDbNeedsUpdate ), Detach );
            }
    
            LGPOS lgposReAttach;
            CallJ( ErrLGReAttachDB( ifmp, &lgposReAttach ), Detach );

            Assert(  fDbNeedsUpdate == ( CmpDbVer( dbvBefore, pfmtversDesired->dbv ) < 0 ) );
            if ( g_rgfmp[ ifmp ].FLogOn() && 
                 ( CmpDbVer( dbvBefore, pfmtversDesired->dbv ) <= 0 )
                   )
            {
                CallJ( ErrDBLGVersionUpdate( pinst, ppib, ifmp, pfmtversDesired, fDbNeedsUpdate ), Detach );
            }
            else
            {
                Enforce( !fDbNeedsUpdate || !g_rgfmp[ ifmp ].FLogOn() );
            }
        
            
            OnDebug( if ( ( rand() % 3 ) == 0 ) UtilSleep( 20 ) );
            Assert( CmpDbVer( dbvBefore, pfmp->Pdbfilehdr()->Dbv() ) == 0 );
            
            {
            PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();


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

            pdbfilehdr->le_lgposLastReAttach = lgposReAttach;
            LGIGetDateTime( &pdbfilehdr->logtimeLastReAttach );
            pfmp->m_isdlAttach.FixedData().sAttachData.lgposAttach = lgposReAttach;
            Expected( pfmp->m_isdlAttach.FActiveSequence() );

            memset( &pdbfilehdr->bkinfoFullCur, 0, sizeof( BKINFO ) );
            memset( &pdbfilehdr->bkinfoSnapshotCur, 0, sizeof( BKINFO ) );

            if ( fDbNeedsUpdate )
            {
            
                DBISetVersion( pinst, wszDbFullName, ifmp, pfmtversDesired->dbv, pdbfilehdr.get(), fDbNeedsUpdate, fFalse );
            }
            
            }

            Assert( CmpDbVer( pfmp->Pdbfilehdr()->Dbv(), pfmtversDesired->dbv ) >= 0 );


            CallJ( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ), Detach );

            Assert( !pfmp->FHeaderUpdateInProgress() );

            pfmp->Pfapi()->UpdateIFilePerfAPIEngineFileTypeId( iofileDbAttached, pfmp->Ifmp() );
            IOResetFmpIoLatencyStats( pfmp->Ifmp() );

#if DEBUG
            BYTE rgbSigZeroes[ sizeof( SIGNATURE ) ] = { 0 };
            Assert( 0 == memcmp( rgbSigZeroes, pfmp->Pdbfilehdr()->rgbReservedSignSLV, sizeof( rgbSigZeroes ) ) );
#endif

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
            OnDebug( PGNO pgnoLastBefore = pfmp->PgnoLast() );
            CallJ( ErrDBSetLastPage( ppib, ifmp ), Detach );
            Assert( pgnoLastBefore >= pfmp->PgnoLast() );
            if ( !BoolParam( JET_paramEnableViewCache ) )
            {
                OnDebug( pgnoLastBefore = pfmp->PgnoLast() );
                CallJ( ErrIONewSize( ifmp, *tcScope, pfmp->PgnoLast(), 0, JET_bitNil ), Detach );
                Assert( pgnoLastBefore == pfmp->PgnoLast() );
            }

            QWORD       cbLogicalSize       = OffsetOfPgno( pfmp->PgnoLast() + 1 );
            QWORD       cbNtfsSize          = 0;
            if ( pfmp->Pfapi()->ErrSize( &cbNtfsSize, IFileAPI::filesizeLogical ) >= JET_errSuccess )
            {
            AssertTrack( cbNtfsSize >= cbLogicalSize, "ReattachFileSizeTooSmall" );
            }
            }

            JET_ERR errLeakReclaimer =  JET_errSuccess;
            if ( pfmp->FLeakReclaimerEnabled() )
            {
                errLeakReclaimer = ErrSPReclaimSpaceLeaks( ppib, ifmp );
                pfmp->m_isdlAttach.Trigger( eAttachLeakReclaimerDone );
            }

            if ( ( errLeakReclaimer >= JET_errSuccess ) &&
                 pfmp->FShrinkDatabaseEofOnAttach() &&
                 !BoolParam( JET_paramEnableViewCache ) &&
                 pfmp->FEfvSupported( JET_efvShrinkEof ) )
            {
                const JET_ERR errT = ErrSHKShrinkDbFromEof( ppib, ifmp );

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

    pfmp = &g_rgfmp[ ifmp ];

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlAttachBegin|sysosrtlContextFmp, pfmp, &ifmp, sizeof(ifmp) );

    Assert( !plog->FRecovering() );
    Assert( !pfmp->FIsTempDB() );

    
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

    pbackup->BKUnlockBackup();

    if ( grbit & JET_bitDbRecoveryOff )
    {
        FMP::EnterFMPPoolAsWriter();
        pfmp->ResetLogOn();
        FMP::LeaveFMPPoolAsWriter();
    }
    else
    {
        FMP::EnterFMPPoolAsWriter();
        pfmp->SetLogOn();
        FMP::LeaveFMPPoolAsWriter();
    }

    Assert( !pfmp->FVersioningOff() );
    pfmp->ResetVersioningOff();

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


    Assert( UtilCmpFileName( pfmp->WszDatabaseName(), wszDbFullName ) == 0 );
    Assert( !( grbit & JET_bitDbReadOnly ) == !g_rgfmp[ifmp].FReadOnlyAttach() );
    Call( ErrDBReadHeaderCheckConsistency( pfsapi, pfmp ) );
    pfmp->TraceDbfilehdrInfo( tsidrEngineFmpDbHdr1st );

    Assert( ( pfmp->PLogRedoMapZeroed() == NULL ) && ( pfmp->PLogRedoMapBadDbTime() == NULL ) );

    if ( !plog->FLogDisabled()
        && 0 == memcmp( &pfmp->Pdbfilehdr()->signLog, &plog->SignLog(), sizeof(SIGNATURE) ) )
    {
#if 0
        if ( CmpLgpos( &pdbfilehdr->lgposAttach, &plog->m_lgposLogRec ) > 0
            || CmpLgpos( &pdbfilehdr->lgposConsistent, &plog->m_lgposLogRec ) > 0 )
        {
            FireWall( "ConsistentTimeMismatchOnAttach" );
            Call( ErrERRCheck( JET_errConsistentTimeMismatch ) );
        }
#endif
    }

    err = ErrDBICheckVersions( pinst, wszDbFullName, ifmp, pfmp->Pdbfilehdr().get(), PfmtversEngineMax()->dbv, pfmp->FReadOnlyAttach() );
    Assert( err != JET_errEngineFormatVersionSpecifiedTooLowForDatabaseVersion );
    Call( err );

    pfmp->m_isdlAttach.Trigger( eAttachReadDBHeader );

    if ( !pfmp->FReadOnlyAttach() )
    {
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

        DWORD tickSleep = (DWORD)UlConfigOverrideInjection( 38908, 0 );
        if ( tickSleep > 0 )
        {
            UtilSleep( tickSleep );
            (void)pinst->m_plog->ErrLGUpdateCheckpointFile( fFalse );
        }

        DBISetHeaderAfterAttach( pfmp->PdbfilehdrUpdateable().get(), lgposLogRec, &logtimeOfGenWithAttach, ifmp, fFalse );
        Assert( pfmp->Pdbfilehdr()->le_objidLast );

        if ( !pfmp->FIsTempDB() )
        {
            Call( pfmp->ErrCreateFlushMap( grbit ) );
        }

        Call( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, wszDbFullName, pfmp ) );

        if ( pfmp->FLogOn() )
        {
            FMP::EnterFMPPoolAsWriter();
            const LONG lgenMaxRequired = pfmp->Pdbfilehdr()->le_lGenMaxRequired;
            pfmp->SetWaypoint( lgenMaxRequired );
            FMP::LeaveFMPPoolAsWriter();
        }
        else
        {
            Assert( pinst->FComputeLogDisabled() );
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
            Call( pfmp->ErrCreateFlushMap( grbit ) );
        }
    }

    pfmp->m_isdlAttach.Trigger( eAttachToLogStream );

    Call( ErrIOOpenDatabase( pfsapi, ifmp, wszDbFullName, pinst->FRecovering() ? iofileDbRecovery : iofileDbAttached, fSparseEnabledFile ) );

    attachState = ATTACH_DB_OPENED;

    pfmp->m_isdlAttach.Trigger( eAttachIOOpenDatabase );


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
    QWORD       cbLogicalSize       = OffsetOfPgno( pfmp->PgnoLast() + 1 );
    QWORD       cbNtfsSize          = 0;
    if ( pfmp->Pfapi()->ErrSize( &cbNtfsSize, IFileAPI::filesizeLogical ) >= JET_errSuccess )
    {
        AssertTrack( g_fRepair || cbNtfsSize >= cbLogicalSize, "AttachFileSizeTooSmall" );
    }
    }

    BFPrereadPageRange( ifmp, 1, 16, bfprfDefault, ppib->BfpriPriority( ifmp ), *tcScope );

    if ( !pfmp->FReadOnlyAttach() && !g_fRepair )
    {
        CallJ( ErrDBUpdateAndFlushVersion(
                    pinst,
                    wszDbFullName,
                    ppib,
                    ifmp,
                    fFalse ), MoreAttachedThanDetached );

        JET_ERR errLeakReclaimer =  JET_errSuccess;
        if ( pfmp->FLeakReclaimerEnabled() )
        {
            errLeakReclaimer = ErrSPReclaimSpaceLeaks( ppib, ifmp );
            pfmp->m_isdlAttach.Trigger( eAttachLeakReclaimerDone );
        }

        if ( ( errLeakReclaimer >= JET_errSuccess ) &&
             pfmp->FShrinkDatabaseEofOnAttach() &&
             !BoolParam( JET_paramEnableViewCache ) &&
             pfmp->FEfvSupported( JET_efvShrinkEof ) )
        {
            const JET_ERR errT = ErrSHKShrinkDbFromEof( ppib, ifmp );

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

    if ( !g_fRepair )
    {
        CallJ( ErrDBTryCreateSystemTable( ppib, ifmp, szMSObjids, ErrCATCreateMSObjids, grbit ), Detach );
        CallJ( ErrCATPopulateMSObjids( ppib, ifmp ), Detach );
        g_rgfmp[ ifmp ].SetFMaintainMSObjids();
    }

    pfmp->m_isdlAttach.Trigger( eAttachCreateMSysObjids );

    Expected( !pinst->FRecovering() );
    Expected( !FFMPIsTempDB( ifmp ) );
    if( !FFMPIsTempDB( ifmp) &&
        !pinst->FRecovering() &&
        !g_fRepair &&
        !pfmp->FReadOnlyAttach()
        )
    {

        CallJ( ErrCATCreateOrUpgradeMSLocales( ppib, ifmp ), Detach );
        Assert( g_rgfmp[ ifmp ].PkvpsMSysLocales() );

        CallJ( ErrFaultInjection( 44400 ), Detach );


        if ( pfmp->Pdbfilehdr()->le_qwSortVersion != g_qwUpgradedLocalesTable )
        {
            pfmp->PdbfilehdrUpdateable()->le_qwSortVersion = g_qwUpgradedLocalesTable;
            CallJ( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ), Detach );
        }
    }

    pfmp->m_isdlAttach.Trigger( eAttachCreateMSysLocales );

    {
    PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr();


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

    const BOOL fIndexCheckingEnabled = ( !pfmp->FReadOnlyAttach() && BoolParam( pinst, JET_paramEnableIndexCleanup ) ) || ( UlParam( pinst, JET_paramEnableIndexChecking ) == JET_IndexCheckingOn );


    if ( !plog->FRecovering()
            && !g_fRepair
            && ( fIndexCheckingEnabled ) )
    {

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
        pfmp->m_isdlAttach.Trigger( eAttachDeleteUnicodeIndexes );
        pfmp->m_isdlAttach.Trigger( eAttachUpgradeUnicodeIndexes );
    }

    if( !FFMPIsTempDB( ifmp ) &&
        !pinst->FRecovering() &&
        !g_fRepair &&
        !pfmp->FReadOnlyAttach() )
    {
        BOOL fUpdateHeaders = fFalse;

    {
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
    }

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
        Assert( JET_errDatabaseCorrupted != ErrCATVerifyMSLocales( ppib, ifmp, fFalse  ) );
    }
#endif

    if ( !plog->FRecovering() && pfmp->FLogOn() && BoolParam( pinst, JET_paramDefragmentSequentialBTrees ) )
    {
        Assert( !g_fRepair );
        Assert( !pfmp->FReadOnlyAttach() );

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


    if ( !pfmp->FIsTempDB() )
    {

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


    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->ReleaseWriteLatch( ppib );
    pfmp->RwlDetaching().LeaveAsWriter();


Detach:

    ULONG rgulD[4] = { (ULONG)ifmp, (ULONG)err, PefLastThrow()->UlLine(), UlLineLastCall() };
    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlAttachFail|sysosrtlContextFmp, pfmp, rgulD, sizeof(rgulD) );
    pfmp->m_isdlAttach.TermSequence();

    Assert( err < JET_errSuccess );
    Assert( pfmp != NULL );
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
        FCB::PurgeDatabase( pfmp->Ifmp(), fFalse  );
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



    Assert( !pfmp->Pfapi() );


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

ERR ISAMAPI ErrIsamDetachDatabase( JET_SESID sesid, IFileSystemAPI* const pfsapiDB, const WCHAR *wszDatabaseName, const INT flags )
{
    Assert( sizeof(JET_SESID) == sizeof(PIB *) );
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


    IFileSystemAPI  *pfsapi         = ( NULL == pfsapiDB ) ? pinst->m_pfsapi : pfsapiDB;

    if ( NULL == wszDatabaseName || 0 == *wszDatabaseName )
    {
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


StartDetaching:

    Call( FMP::ErrWriteLatchByNameWsz( wszFullName, &ifmp, ppib ) );


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


    Assert( pfmp == NULL );
    pfmp = &g_rgfmp[ ifmp ];
    Assert( !pfmp->FIsTempDB() );

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDetachBegin|sysosrtlContextFmp, pfmp, &ifmp, sizeof(ifmp) );

    pfmp->m_isdlDetach.InitSequence( isdltypeDetach, eDetachSeqMax );

    pfmp->ReleaseWriteLatch( ppib );


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

    PagePatching::TermFmp( ifmp );

    pfmp->SetNoWaypointLatency();



    pbackup->BKUnlockBackup();
    fInCritBackup = fFalse;

    pfmp->m_isdlDetach.Trigger( eDetachSetDetaching );

    if ( !pfmp->FSkippedAttach()
        && !pfmp->FDeferredAttach() )
    {

        Call( PverFromIfmp( ifmp )->ErrVERRCEClean( ifmp ) );

    }

    pfmp->m_isdlDetach.Trigger( eDetachVersionStoreDone );

    pfmp->WaitForTasksToComplete();

    pfmp->m_isdlDetach.Trigger( eDetachStopSystemTasks );

    if ( !pfmp->FSkippedAttach()
        && !pfmp->FDeferredAttach() )
    {
        Call( PverFromIfmp( ifmp )->ErrVERRCEClean( ifmp ) );

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


        Call( PinstFromIfmp( ifmp )->m_plog->ErrLGUpdateWaypointIFMP( pfsapi, ifmp ) );


        Call( ErrBFFlush( ifmp ) );

        pfmp->m_isdlDetach.Trigger( eDetachBufferManagerFlushDone );

        BFPurge( ifmp );

        pfmp->m_isdlDetach.Trigger( eDetachBufferManagerPurgeDone );
    }
    else
    {
        pfmp->m_isdlDetach.Trigger( eDetachBufferManagerFlushDone );
        pfmp->m_isdlDetach.Trigger( eDetachBufferManagerPurgeDone );
    }


    pfmp->AssertRangeLockEmpty();


    Assert( UtilCmpFileName( wszFullName, pfmp->WszDatabaseName() ) == 0 );

    if ( !plog->FRecovering() )
    {
        plog->SetCheckpointEnabled( fFalse );
    }


    if ( pfmp->Pfapi() )
    {
        Call( ErrIOFlushDatabaseFileBuffers( ifmp, iofrFlushIfmpContext ) );
    }


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

    CATTermMSLocales( &(g_rgfmp[ifmp]) );
    Assert( NULL == pfmp->PkvpsMSysLocales() );

    if ( !plog->FRecovering() && ErrFaultInjection( 37004 ) < JET_errSuccess )
    {
        (VOID)plog->ErrLGUpdateCheckpointFile( fFalse );
        Call( ErrFaultInjection( 37004 ) );
    }

    Assert( pfmp->FAllowHeaderUpdate() || pfmp->FReadOnlyAttach() );
    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->ResetAllowHeaderUpdate();
    pfmp->RwlDetaching().LeaveAsWriter();

    if ( pfmp->Pfapi() )
    {
        AssertTrack( pfmp->Pfapi()->CioNonFlushed() == 0, "UnexpectedPendingFlushesDetach" );
    }

    CFlushMapForAttachedDb* const pfm = pfmp->PFlushMap();
    if ( !pfmp->FReadOnlyAttach() && ( pfm != NULL ) )
    {
        Call( pfm->ErrCleanFlushMap() );
    }



    
    {
    PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
    const BKINFO *              pbkInfoToCopy   = pdbfilehdr ? &(pdbfilehdr->bkinfoFullCur) : NULL;
    const DBFILEHDR::BKINFOTYPE     bkinfoType      = DBFILEHDR::backupNormal;

    if ( pbkInfoToCopy && pbkInfoToCopy->le_genLow != 0 && !pfmp->FReadOnlyAttach() )
    {
        Assert( pbkInfoToCopy->le_genHigh != 0 );

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
    }


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



        Assert( pfmp->LgposWaypoint().lGeneration <= pfmp->Pdbfilehdr()->le_lGenMaxRequired );
        pfmp->ResetWaypoint();

        pfmp->PdbfilehdrUpdateable()->le_dbtimeDirtied = pfmp->DbtimeLast();
        Assert( pfmp->Pdbfilehdr()->le_dbtimeDirtied != 0 );
        pfmp->PdbfilehdrUpdateable()->le_objidLast = pfmp->ObjidLast();
        Assert( pfmp->Pdbfilehdr()->le_objidLast != 0 );

        if ( plog->FRecovering() && plog->FRecoveringMode() == fRecoveringRedo &&
            pfmp->Pdbfilehdr()->Dbstate() == JET_dbstateDirtyAndPatchedShutdown )
        {
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


        pfmp->PdbfilehdrUpdateable()->le_dbtimeLastScrub    = pfmp->DbtimeLastScrub();
        pfmp->PdbfilehdrUpdateable()->logtimeScrub      = pfmp->LogtimeScrub();

        memset( &pfmp->PdbfilehdrUpdateable()->bkinfoSnapshotCur, 0, sizeof( BKINFO ) );

        Assert( pfmp->Pdbfilehdr()->le_objidLast );

        Call( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, wszDatabaseName, pfmp, pfmp->Pfapi() ) );

        Call( ErrIOFlushDatabaseFileBuffers( ifmp, iofrFlushIfmpContext ) );
        Assert( pfmp->Pfapi() == NULL || pfmp->Pfapi()->CioNonFlushed() == 0 );
    }

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


    pfmp->ResetFDontRegisterOLD2Tasks();
    pfmp->ResetFDontStartDBM();
    pfmp->ResetFDontStartOLD();
    pfmp->ResetFDontStartTrimTask();

    FMP::EnterFMPPoolAsWriter();
    pfmp->RwlDetaching().EnterAsWriter();
    Assert( pfmp->FDetachingDB() );

    DBResetFMP( pfmp, plog );


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


    if ( fDetachLogged )
    {
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

    Assert( pjdbid );
    *pjdbid = JET_dbidNil;

    Assert( sizeof(JET_SESID) == sizeof(PIB *) );

    CallR( ErrPIBCheck( ppib ) );

    CallR( ErrDBOpenDatabase( ppib, wszDatabaseName, &ifmp, grbit ) );

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

    }

    Assert( pfmp->Pfapi() );
    DBSetOpenDatabaseFlag( ppib, ifmp );


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

    if ( ( grbit & ~grbitValid ) != 0 )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    if ( ( ( grbit & JET_bitResizeDatabaseOnlyGrow ) != 0 ) && ( ( grbit & JET_bitResizeDatabaseOnlyShrink ) != 0 ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    Call( ErrSPGetLastPgno( ppib, ifmp, &pgnoLastCurrent ) );

    cpgCurrent = pgnoLastCurrent + cpgDBReserved;
    cpgActual = cpgCurrent;

    if ( cpgCurrent == (CPG)cpgTarget )
    {
        goto DoneWithResizing;
    }
    else if ( (CPG)cpgTarget < cpgCurrent )
    {
        if ( ( ( grbit & JET_bitResizeDatabaseOnlyGrow ) != 0 ) )
        {
            goto DoneWithResizing;
        }

        if ( ( ( GrbitParam( g_rgfmp[ ifmp ].Pinst(), JET_paramEnableShrinkDatabase ) & JET_bitShrinkDatabaseOn ) == 0 ) )
        {
            Error( ErrERRCheck( JET_errFeatureNotAvailable ) );
        }

        Call( ErrSPTrimRootAvail( ppib, ifmp, CPRINTFNULL::PcprintfInstance() ) );
        fTrimmed = fTrue;
    }
    else
    {
        if ( ( ( grbit & JET_bitResizeDatabaseOnlyShrink ) != 0 ) )
        {
            goto DoneWithResizing;
        }


        if ( BoolParam( JET_paramEnableViewCache ) )
        {
            Error( ErrERRCheck( JET_errFeatureNotAvailable ) );
        }

        CPG cpgExtend = (CPG)cpgTarget - cpgCurrent;
        Assert( cpgExtend > 0 );

        if ( ppib->Level() == 0 )
        {
            Call( ErrDIRBeginTransaction( ppib, 46920, NO_GRBIT ) );
            fTransactionStarted = fTrue;
        }
        
        cpgExtend = roundup( cpgExtend, cpgExtensionSize );
        Assert( cpgExtend > 0 );

        CPG cpgTotal = 0;
        do
        {
            Call( ErrSPExtendDB( ppib, ifmp, cpgExtensionSize, &pgnoAlloc, fFalse ) );

            cpgTotal += cpgExtensionSize;
        }
        while ( cpgTotal < cpgExtend );

    }

DoneWithResizing:
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

    
    pfmp->GetWriteLatch( ppib );

    Assert( FIODatabaseOpen( ifmp ) );
    if ( FLastOpen( ppib, ifmp ) )
    {

        pfucb = ppib->pfucbOfSession;
        while ( pfucb != pfucbNil
            && ( pfucb->ifmp != ifmp || !pfucb->u.pfcb->FPrimaryIndex() ) )
        {
            pfucb = pfucb->pfucbNextOfSession;
        }

        while ( pfucb != pfucbNil )
        {
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


    DBResetOpenDatabaseFlag( ppib, ifmp );
    if ( pfmp->FExclusiveBySession( ppib ) )
        pfmp->ResetExclusiveOpen( );

    if ( ppib->FSessionOLD() )
    {
        Assert( pfmp->FRunningOLD() );
        pfmp->ResetRunningOLD();
    }


    pfmp->ReleaseWriteLatch( ppib );


    return JET_errSuccess;
}


ERR ErrDBOpenDatabaseByIfmp( PIB *ppib, IFMP ifmp )
{
    ERR     err;


    CallR( FMP::ErrWriteLatchByIfmp( ifmp, ppib ) );


    FMP *pfmp = &g_rgfmp[ ifmp ];
    Assert( pfmp->FWriteLatchByMe(ppib) );
    Assert( pfmp->FAttached() );


    if ( pfmp->FExclusiveByAnotherSession( ppib )
        && !FPIBSessionLV( ppib )
        && !FPIBSessionSystemCleanup( ppib ) )
    {
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
    const FormatVersions * pfmtversMax = PfmtversEngineMax();

    Expected( efvFormatFeature <= pfmtversMax->efv );

    if ( efvFormatFeature <= pfmtversMax->efv &&
            0 == CmpDbVer( pfmtversMax->dbv, dbvCurrentFromFile ) )
    {
        OSTrace( JET_tracetagVersionAndStagingChecks, OSFormat( "DB format feature EFV %d check success (fast path).\n", efvFormatFeature ) );
        return JET_errSuccess;
    }


    const FormatVersions * pfmtversFormatFeature = NULL;
    CallS( ErrGetDesiredVersion( NULL , efvFormatFeature, &pfmtversFormatFeature ) );
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
