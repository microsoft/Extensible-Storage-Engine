// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifndef ENABLE_JET_UNIT_TEST
#error This file should only be compiled with the unit tests!
#endif // ENABLE_JET_UNIT_TEST

class FmpTestFileSystemApi : public IFileSystemAPI
{
public:
    FmpTestFileSystemApi() {}

    virtual ~FmpTestFileSystemApi() {}

    ERR ErrDiskSpace(   const WCHAR* const  wszPath,
                        QWORD* const        pcbFreeForUser,
                        QWORD* const        pcbTotalForUser = NULL,
                        QWORD* const        pcbFreeOnDisk = NULL ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    //  returns the sector size for the specified path

    ERR ErrFileSectorSize(  const WCHAR* const  wszPath,
                            DWORD* const        pcbSize ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    //  returns the atomic write size for the specified path

    ERR ErrFileAtomicWriteSize( const WCHAR* const  wszPath,
                                DWORD* const        pcbSize ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    //  Error Conversion

    ERR ErrGetLastError( const DWORD error ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    //  Path Manipulation

    //  computes the root path of the specified path

    ERR ErrPathRoot(    const WCHAR* const                                          wszPath,
                        __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszAbsRootPath ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    //  computes the volume canonical path and disk ID of the specified root path

    void PathVolumeCanonicalAndDiskId(  const WCHAR* const                                  wszVolumePath,
                                        __out_ecount(cchVolumeCanonicalPath) WCHAR* const   wszVolumeCanonicalPath,
                                        _In_ const DWORD                                    cchVolumeCanonicalPath,
                                        __out_ecount(cchDiskId) WCHAR* const                wszDiskId,
                                        _In_ const DWORD                                    cchDiskId,
                                        _Out_ DWORD *                                       pdwDiskNumber ) override
    {
        AssertRTL( fFalse && "Not implemented" );
    }

    //  computes the absolute path of the specified path, returning
    //  JET_errInvalidPath if the path is not found.  the absolute path is
    //  returned in the specified buffer if provided

    ERR ErrPathComplete(    _In_z_ const WCHAR* const                           wszPath,
                            // UNDONE_BANAPI:
                            _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const   wszAbsPath  = NULL ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    //  breaks the given path into folder, filename, and extension
    //  components

    ERR ErrPathParse(   const WCHAR* const                                          wszPath,
                        // UNDONE_BANAPI:
                        __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFolder,
                        // UNDONE_BANAPI:
                        __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFileBase,
                        // UNDONE_BANAPI:
                        __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFileExt ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    const WCHAR * const WszPathFileName( _In_z_ const WCHAR * const wszOptionalFullPath ) const override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    //  composes a path from folder, filename, and extension components

    ERR ErrPathBuild(   __in_z const WCHAR* const                                   wszFolder,
                        __in_z const WCHAR* const                                   wszFileBase,
                        __in_z const WCHAR* const                                   wszFileExt,
                        // UNDONE_BANAPI:
                        __out_bcount_z(cbPath) WCHAR* const                         wszPath,
                        __in_range(cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW) ULONG  cbPath ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }


    //  appends a folder delimiter (in NT L"\") if it doesn't already have a
    //  folder delimiter at the end of the string.

    ERR ErrPathFolderNorm(  __inout_bcount(cbSize)  PWSTR const wszFolder,
                            DWORD                               cbSize ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    //  Returns whether the input path is absolute or relative.
    //  Note that a leading backslash (e.g. \windows\win.ini) is still relative.

    BOOL FPathIsRelative( _In_ PCWSTR wszPath ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    //  Returns whether the specified file/path exists.

    ERR ErrPathExists(  _In_ PCWSTR         wszPath,
                        _Out_opt_ BOOL *    pfIsDirectory ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    // Retrieves the default location that files should be stored.
    // The "." directory (current working dir) is not writeable by Windows Store apps.
    ERR ErrPathFolderDefault(   _Out_z_bytecap_(cbFolder) PWSTR const   wszFolder,
                                _In_ DWORD                              cbFolder,
                                _Out_ BOOL *                            pfCanProcessUseRelativePaths ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    // Gets the OS' temporary folder.
    ERR ErrGetTempFolder(   _Out_z_cap_(cchFolder) PWSTR const  wszFolder,
                            _In_ const DWORD                    cchFolder ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    // Gets a unique, random, temporary file name.
    ERR ErrGetTempFileName( _In_z_ PWSTR const                          wszFolder,
                            _In_z_ PWSTR const                          wszPrefix,
                            _Out_z_cap_(OSFSAPI_MAX_PATH) PWSTR const   wszFileName ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }


    //  Folder Control

    //  creates the specified folder.  if the folder already exists,
    //  JET_errFileAccessDenied will be returned

    ERR ErrFolderCreate( const WCHAR* const wszPath ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    //  removes the specified folder.  if the folder is not empty,
    //  JET_errFileAccessDenied will be returned.  if the folder does
    //  not exist, JET_errInvalidPath will be returned

    ERR ErrFolderRemove( const WCHAR* const wszPath ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }


    //  Folder Enumeration

    //  creates a File Find iterator that will traverse the absolute paths
    //  of all files and folders that match the specified path with
    //  wildcards.  if the iterator cannot be created, JET_errOutOfMemory
    //  will be returned

    ERR ErrFileFind(    const WCHAR* const      wszFind,
                        IFileFindAPI** const    ppffapi ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }


    //  File Control

    //  deletes the file specified.  if the file cannot be deleted,
    //  JET_errFileAccessDenied is returned

    ERR ErrFileDelete( const WCHAR* const wszPath ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    //  moves the file specified from a source path to a destination path, overwriting
    //  any file at the destination path if requested.  if the file cannot be moved,
    //  JET_errFileAccessDenied is returned

    ERR ErrFileMove(    const WCHAR* const  wszPathSource,
                        const WCHAR* const  wszPathDest,
                        const BOOL          fOverwriteExisting  = fFalse ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }


    //  copies the file specified from a source path to a destination path, overwriting
    //  any file at the destination path if requested.  if the file cannot be copied,
    //  JET_errFileAccessDenied is returned

    ERR ErrFileCopy(    const WCHAR* const  wszPathSource,
                        const WCHAR* const  wszPathDest,
                        const BOOL          fOverwriteExisting  = fFalse ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    //  creates the file specified and returns its handle.  if the file cannot
    //  be created, JET_errFileAccessDenied or JET_errDiskFull will be returned

    ERR ErrFileCreate(  _In_z_ const WCHAR* const       wszPath,
                        _In_ IFileAPI::FileModeFlags    fmf,
                        _Out_ IFileAPI** const          ppfapi ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }

    //  opens the file specified with the specified access privileges and returns
    //  its handle.  if the file cannot be opened, JET_errFileAccessDenied is returned

    ERR ErrFileOpen(    _In_z_ const WCHAR* const       wszPath,
                        _In_ IFileAPI::FileModeFlags    fmf,
                        _Out_ IFileAPI** const          ppfapi ) override
    {
        AssertRTL( fFalse && "Not implemented" );
        return JET_errSuccess;
    }
};


//  ================================================================
JETUNITTEST( FMP, NewAndWriteLatch )
//  ================================================================
{

    FmpTestFileSystemApi ftfsapi;
    BOOL fCacheAlive;

    // 1000 works, but slows down dramatically.
    //  const size_t cFmps = 1000;
    const size_t cFmps = 50;

    CHECKCALLS(JetSetSystemParameter( nullptr, JET_sesidNil, JET_paramMaxInstances, cFmps, nullptr ));
//  CHECKCALLS(JetSetSystemParameter( nullptr, JET_sesidNil, JET_paramMaxDatabasesPerInstance, 1, nullptr )); 

    IFMP rgifmp[ cFmps ] = { 0 };
    INST* rgpinst[ cFmps ] = { 0 };
    PIB* rgppib[ cFmps ] = { 0 };
    size_t i;
    CPG cpgAvail;
    
    // ErrFMPInit() hasn't been called yet.
    CHECK( g_ifmpMax == 0 );

    for ( i = 0; i < cFmps; ++i )
    {
        rgifmp[ i ] = ifmpNil;
        // CHECKCALLS( ErrNewInst( &rgpinst[ i ], NULL, NULL, NULL ) );
        wchar_t wszInstanceName[ 20 ];
        wchar_t wszDisplayName[ 20 ];
        swprintf_s( wszInstanceName, _countof(wszInstanceName), L"inst-%02Ix", i );
        swprintf_s( wszDisplayName, _countof(wszDisplayName), L"disp-%02Ix", i );
        CHECKCALLS( JetCreateInstance2W( (JET_INSTANCE*) &rgpinst[ i ], wszInstanceName, wszDisplayName, JET_bitNil ) );

        CHECKCALLS( JetSetSystemParameter( (JET_INSTANCE*) &rgpinst[i], JET_sesidNil, JET_paramRecovery, 0, "off" ) );
        CHECKCALLS( JetSetSystemParameter( (JET_INSTANCE*) &rgpinst[i], JET_sesidNil, JET_paramTempPath, 0, NULL));
        CHECKCALLS( JetSetSystemParameter( (JET_INSTANCE*) &rgpinst[i], JET_sesidNil, JET_paramMaxTemporaryTables, 0, NULL));

        CHECKCALLS( JetInit2( (JET_INSTANCE*) &rgpinst[ i ], JET_bitNil ) );

        CHECKCALLS( ErrPIBBeginSession( rgpinst[ i ], &rgppib[ i ], procidNil, fFalse ) );

        CHECK( !rgpinst[i]->FRecovering() );
    }

    // After ErrFMPInit, verify that Min/Mac in use are set correctly.
    // 7 dbs/instance plus one.
    CHECK( g_ifmpMax == cFmps * 7 + 1 );
    CHECK( g_ifmpMax == FMP::IfmpMinInUse() );
    CHECK( 0 == FMP::IfmpMacInUse() );

    for ( i = 0; i < cFmps; ++i )
    {
        wchar_t wszFakeFile[ 20 ];
        swprintf_s( wszFakeFile, _countof(wszFakeFile), L"fake-%02Ix.edb", i );

        CHECKCALLS( FMP::ErrNewAndWriteLatch(
            &rgifmp[ i ],
            wszFakeFile,
            rgppib[ i ],
            rgpinst[ i ],
            (IFileSystemAPI*)&ftfsapi,
            0, // dbid
            fFalse,
            fFalse,
            &fCacheAlive ) );
        CHECK( ifmpNil != rgifmp[ i ] );

        FMP* const pfmp = PfmpFromIfmp( rgifmp[ i ] );

        // Verify that IfmpMacInUse() increases.
        CHECK( i + 1 == FMP::IfmpMacInUse() );
        CHECK( 1 == FMP::IfmpMinInUse() );

        // Verify cache priority.
        pfmp->SetPctCachePriorityFmp( 2 * i );
        CHECK( ( 2 * i ) == pfmp->PctCachePriorityFmp() );

        // Verify shrink options.
        CHECK( !pfmp->FShrinkDatabaseEofOnAttach() );
        CHECK( !pfmp->FRunShrinkDatabaseFullCatOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontMoveRootsOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontTruncateLeakedPagesOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontTruncateIndeterminatePagesOnAttach() );
        pfmp->SetShrinkDatabaseOptions( JET_bitShrinkDatabaseEofOnAttach );
        CHECK( pfmp->FShrinkDatabaseEofOnAttach() );
        CHECK( !pfmp->FRunShrinkDatabaseFullCatOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontMoveRootsOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontTruncateLeakedPagesOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontTruncateIndeterminatePagesOnAttach() );
        pfmp->SetShrinkDatabaseOptions( JET_bitShrinkDatabaseFullCategorizationOnAttach );
        CHECK( !pfmp->FShrinkDatabaseEofOnAttach() );
        CHECK( pfmp->FRunShrinkDatabaseFullCatOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontMoveRootsOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontTruncateLeakedPagesOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontTruncateIndeterminatePagesOnAttach() );
        pfmp->SetShrinkDatabaseOptions( JET_bitShrinkDatabaseDontMoveRootsOnAttach );
        CHECK( !pfmp->FShrinkDatabaseEofOnAttach() );
        CHECK( !pfmp->FRunShrinkDatabaseFullCatOnAttach() );
        CHECK( pfmp->FShrinkDatabaseDontMoveRootsOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontTruncateLeakedPagesOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontTruncateIndeterminatePagesOnAttach() );
        pfmp->SetShrinkDatabaseOptions( JET_bitShrinkDatabaseDontTruncateLeakedPagesOnAttach );
        CHECK( !pfmp->FShrinkDatabaseEofOnAttach() );
        CHECK( !pfmp->FRunShrinkDatabaseFullCatOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontMoveRootsOnAttach() );
        CHECK( pfmp->FShrinkDatabaseDontTruncateLeakedPagesOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontTruncateIndeterminatePagesOnAttach() );
        pfmp->SetShrinkDatabaseOptions( JET_bitShrinkDatabaseDontTruncateIndeterminatePagesOnAttach );
        CHECK( !pfmp->FShrinkDatabaseEofOnAttach() );
        CHECK( !pfmp->FRunShrinkDatabaseFullCatOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontMoveRootsOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontTruncateLeakedPagesOnAttach() );
        CHECK( pfmp->FShrinkDatabaseDontTruncateIndeterminatePagesOnAttach() );
        pfmp->SetShrinkDatabaseOptions( NO_GRBIT );
        CHECK( !pfmp->FShrinkDatabaseEofOnAttach() );
        CHECK( !pfmp->FRunShrinkDatabaseFullCatOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontMoveRootsOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontTruncateLeakedPagesOnAttach() );
        CHECK( !pfmp->FShrinkDatabaseDontTruncateIndeterminatePagesOnAttach() );

        // Verify shrink time quota.
        CHECK( -1 == pfmp->DtickShrinkDatabaseTimeQuota() );
        pfmp->SetShrinkDatabaseTimeQuota( 1234 );
        CHECK( 1234 == pfmp->DtickShrinkDatabaseTimeQuota() );
        pfmp->SetShrinkDatabaseTimeQuota( -1 );
        CHECK( -1 == pfmp->DtickShrinkDatabaseTimeQuota() );

        // Verify shrink size limit.
        CHECK( 0 == pfmp->CpgShrinkDatabaseSizeLimit() );
        pfmp->SetShrinkDatabaseSizeLimit( 5678 );
        CHECK( 5678 == pfmp->CpgShrinkDatabaseSizeLimit() );
        pfmp->SetShrinkDatabaseSizeLimit( 0 );
        CHECK( 0 == pfmp->CpgShrinkDatabaseSizeLimit() );

        // Shrink running helpers.
        CHECK( !pfmp->FShrinkIsActive() );
        CHECK( !pfmp->FShrinkIsRunning() );
        CHECK( !pfmp->FPgnoShrinkTargetIsSet() );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 10 ) );
        pfmp->SetShrinkIsRunning();
        CHECK( !pfmp->FShrinkIsActive() );
        CHECK( pfmp->FShrinkIsRunning() );
        CHECK( !pfmp->FPgnoShrinkTargetIsSet() );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 10 ) );
        pfmp->ResetShrinkIsRunning();
        CHECK( !pfmp->FShrinkIsActive() );
        CHECK( !pfmp->FShrinkIsRunning() );
        CHECK( !pfmp->FPgnoShrinkTargetIsSet() );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 10 ) );
        pfmp->SetShrinkIsRunning();
        CHECK( !pfmp->FShrinkIsActive() );
        CHECK( !pfmp->FPgnoShrinkTargetIsSet() );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( pgnoNull ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( pgnoNull, 0 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( pgnoNull, 1 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( pgnoNull, 2 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 10 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 10, 0 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 10, 1 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 10, 2 ) );
        pfmp->SetPgnoShrinkTarget( 10 );
        CHECK( pfmp->FShrinkIsActive() );
        CHECK( pfmp->FPgnoShrinkTargetIsSet() );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( pgnoNull ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( pgnoNull, 0 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( pgnoNull, 1 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( pgnoNull, 2 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 9 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 9, 0 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 9, 1 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 9, 2 ) );
        CHECK( pfmp->FBeyondPgnoShrinkTarget( 9, 3 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 10 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 10, 0 ) );
        CHECK( !pfmp->FBeyondPgnoShrinkTarget( 10, 1 ) );
        CHECK( pfmp->FBeyondPgnoShrinkTarget( 10, 2 ) );
        CHECK( pfmp->FBeyondPgnoShrinkTarget( 11 ) );
        CHECK( pfmp->FBeyondPgnoShrinkTarget( 11, 0 ) );
        CHECK( pfmp->FBeyondPgnoShrinkTarget( 11, 1 ) );
        CHECK( pfmp->FBeyondPgnoShrinkTarget( 11, 2 ) );
        pfmp->ResetPgnoShrinkTarget();
        CHECK( !pfmp->FShrinkIsActive() );
        CHECK( !pfmp->FPgnoShrinkTargetIsSet() );
        pfmp->ResetShrinkIsRunning();
        CHECK( !pfmp->FShrinkIsActive() );
        CHECK( !pfmp->FPgnoShrinkTargetIsSet() );

        // Verify leak reclaimer options.
        CHECK( !pfmp->FLeakReclaimerEnabled() );
        pfmp->SetLeakReclaimerEnabled( fTrue );
        CHECK( pfmp->FLeakReclaimerEnabled() );
        pfmp->SetLeakReclaimerEnabled( fFalse );
        CHECK( !pfmp->FLeakReclaimerEnabled() );

        // Verify leak reclaimer time quota.
        CHECK( -1 == pfmp->DtickLeakReclaimerTimeQuota() );
        pfmp->SetLeakReclaimerTimeQuota( 1234 );
        CHECK( 1234 == pfmp->DtickLeakReclaimerTimeQuota() );
        pfmp->SetLeakReclaimerTimeQuota( -1 );
        CHECK( -1 == pfmp->DtickLeakReclaimerTimeQuota() );

        // Leak reclaimer running helpers.
        CHECK( !pfmp->FLeakReclaimerIsRunning() );
        pfmp->SetLeakReclaimerIsRunning();
        CHECK( pfmp->FLeakReclaimerIsRunning() );
        pfmp->ResetLeakReclaimerIsRunning();
        CHECK( !pfmp->FLeakReclaimerIsRunning() );

        // Leak report helpers.
        CHECK( pfmp->OjidLeakEstimation() == objidFDPOverMax );
        CHECK( pfmp->ErrStartRootSpaceLeakEstimation() == JET_errSuccess );
        CHECK( pfmp->OjidLeakEstimation() == objidNil );
        CHECK( pfmp->ErrStartRootSpaceLeakEstimation() == JET_errRootSpaceLeakEstimationAlreadyRunning );
        CHECK( pfmp->OjidLeakEstimation() == objidNil );
        pfmp->StopRootSpaceLeakEstimation();
        CHECK( pfmp->OjidLeakEstimation() == objidFDPOverMax );
        CHECK( pfmp->ErrStartRootSpaceLeakEstimation() == JET_errSuccess );
        CHECK( pfmp->OjidLeakEstimation() == objidNil );
        pfmp->StopRootSpaceLeakEstimation();
        CHECK( pfmp->OjidLeakEstimation() == objidFDPOverMax );
        CHECK( pfmp->ErrStartRootSpaceLeakEstimation() == JET_errSuccess );
        CHECK( pfmp->OjidLeakEstimation() == objidNil );
        pfmp->SetOjidLeakEstimation( 1 );
        CHECK( pfmp->OjidLeakEstimation() == 1 );
        pfmp->SetOjidLeakEstimation( 12 );
        CHECK( pfmp->OjidLeakEstimation() == 12 );
        pfmp->SetOjidLeakEstimation( 123 );
        CHECK( pfmp->OjidLeakEstimation() == 123 );
        pfmp->SetOjidLeakEstimation( objidFDPMax - 1 );
        CHECK( pfmp->OjidLeakEstimation() == ( objidFDPMax - 1 ) );
        CHECK( pfmp->ErrStartRootSpaceLeakEstimation() == JET_errRootSpaceLeakEstimationAlreadyRunning );
        CHECK( pfmp->OjidLeakEstimation() == ( objidFDPMax - 1 ) );
        pfmp->SetOjidLeakEstimation( objidFDPMax );
        CHECK( pfmp->OjidLeakEstimation() == objidFDPMax );
        pfmp->StopRootSpaceLeakEstimation();
        CHECK( pfmp->OjidLeakEstimation() == objidFDPOverMax );

        // Self-alloc split-buffer reservation.
        CHECK( !pfmp->FSelfAllocSpBufReservationEnabled() );
        pfmp->SetSelfAllocSpBufReservationEnabled( fTrue );
        CHECK( pfmp->FSelfAllocSpBufReservationEnabled() );
        pfmp->SetSelfAllocSpBufReservationEnabled( fFalse );
        CHECK( !pfmp->FSelfAllocSpBufReservationEnabled() );

        // Test the helpers that maintain the cached CpgAvail total.
        CHECK( !pfmp->FGetCpgAvail( &cpgAvail ) );
        pfmp->SetCpgAvail( 5 );
        CHECK( pfmp->FGetCpgAvail( &cpgAvail ) );
        CHECK( 5 == cpgAvail );
        pfmp->AdjustCpgAvail( 10 );
        CHECK( pfmp->FGetCpgAvail( &cpgAvail ) );
        CHECK( 15 == cpgAvail );
        pfmp->AdjustCpgAvail( -7 );
        CHECK( pfmp->FGetCpgAvail( &cpgAvail ) );
        CHECK( 8 == cpgAvail );
        pfmp->ResetCpgAvail();
        CHECK( !pfmp->FGetCpgAvail( &cpgAvail ) );
        pfmp->AdjustCpgAvail( 10 );
        CHECK( !pfmp->FGetCpgAvail( &cpgAvail ) );
        pfmp->SetCpgAvail( 3 );
        CHECK( pfmp->FGetCpgAvail( &cpgAvail ) );
        CHECK( 3 == cpgAvail );
        pfmp->SetCpgAvail( 3 ); // Setting the same value twice is allowed.
        CHECK( pfmp->FGetCpgAvail( &cpgAvail ) );
        CHECK( 3 == cpgAvail );
        pfmp->ResetCpgAvail();
        CHECK( !pfmp->FGetCpgAvail( &cpgAvail ) );
        
        // Verify PgnoMax tracking initial values.
        CHECK( 0 == pfmp->PgnoHighestWriteLatched() );
        CHECK( 0 == pfmp->PgnoDirtiedMax() );
        CHECK( 0 == pfmp->PgnoWriteLatchedNonScanMax() );
        CHECK( 0 == pfmp->PgnoLatchedScanMax() );
        CHECK( 0 == pfmp->PgnoPreReadScanMax() );
        CHECK( 0 == pfmp->PgnoScanMax() );

        // Verify PgnoMax tracking updating.
        pfmp->UpdatePgnoHighestWriteLatched( 10 );
        pfmp->UpdatePgnoHighestWriteLatched( 9 );
        CHECK( 10 == pfmp->PgnoHighestWriteLatched() );
        pfmp->UpdatePgnoDirtiedMax( 20 );
        pfmp->UpdatePgnoDirtiedMax( 9 );
        CHECK( 20 == pfmp->PgnoDirtiedMax() );
        pfmp->UpdatePgnoWriteLatchedNonScanMax( 30 );
        pfmp->UpdatePgnoWriteLatchedNonScanMax( 9 );
        CHECK( 30 == pfmp->PgnoWriteLatchedNonScanMax() );
        pfmp->UpdatePgnoLatchedScanMax( 40 );
        pfmp->UpdatePgnoLatchedScanMax( 9 );
        CHECK( 40 == pfmp->PgnoLatchedScanMax() );
        pfmp->UpdatePgnoPreReadScanMax( 50 );
        pfmp->UpdatePgnoPreReadScanMax( 9 );
        CHECK( 50 == pfmp->PgnoPreReadScanMax() );
        pfmp->UpdatePgnoScanMax( 60 );
        pfmp->UpdatePgnoScanMax( 9 );
        CHECK( 60 == pfmp->PgnoScanMax() );

        // Verify PgnoMax tracking resetting to a low value.
        pfmp->ResetPgnoMaxTracking( 1 );
        pfmp->ResetPgnoMaxTracking( 100 );
        CHECK( 1 == pfmp->PgnoHighestWriteLatched() );
        CHECK( 1 == pfmp->PgnoDirtiedMax() );
        CHECK( 1 == pfmp->PgnoWriteLatchedNonScanMax() );
        CHECK( 1 == pfmp->PgnoLatchedScanMax() );
        CHECK( 1 == pfmp->PgnoPreReadScanMax() );
        CHECK( 1 == pfmp->PgnoScanMax() );

        // Verify PgnoMax tracking resetting to 0.
        pfmp->ResetPgnoMaxTracking();
        CHECK( 0 == pfmp->PgnoHighestWriteLatched() );
        CHECK( 0 == pfmp->PgnoDirtiedMax() );
        CHECK( 0 == pfmp->PgnoWriteLatchedNonScanMax() );
        CHECK( 0 == pfmp->PgnoLatchedScanMax() );
        CHECK( 0 == pfmp->PgnoPreReadScanMax() );
        CHECK( 0 == pfmp->PgnoScanMax() );
    }

    // Free them in reverse. Have 'i' go forward, and iifmpToFree go backward.
    for ( i = 0; i < cFmps; ++i )
    {
        const INT iifmpToFree = cFmps - i - 1;
        FMP::EnterFMPPoolAsWriter();
        FMP* const pfmp = PfmpFromIfmp(rgifmp[iifmpToFree]);
        pfmp->ReleaseWriteLatchAndFree(rgppib[iifmpToFree]);
        FMP::LeaveFMPPoolAsWriter();
        rgifmp[ iifmpToFree ] = ifmpNil;

        // Verify that IfmpMacInUse() drops as we release FMPs.
        CHECK( iifmpToFree == (INT) FMP::IfmpMacInUse() );

        if ( 0 == iifmpToFree )
        {
            // When all of the FMPs have been freed, IfmpMinInUse() goes back to IfmpMacInUse.
            CHECK( g_ifmpMax == FMP::IfmpMinInUse() );
        }
        else
        {
            // There are still outstanding FMPs. So Min stays at 1.
            CHECK( 1 == FMP::IfmpMinInUse() );
        }
    }

    // 7 dbs/instance plus one.
    CHECK( g_ifmpMax == cFmps * 7 + 1 );
    CHECK( g_ifmpMax == FMP::IfmpMinInUse() );
    CHECK( 0 == FMP::IfmpMacInUse() );

    for ( i = 0; i < cFmps; ++i )
    {
        PIBEndSession( rgppib[ i ] );

        CHECKCALLS( JetTerm2( (JET_INSTANCE) rgpinst[ i ], JET_bitTermAbrupt ) );
    }

    return;
}

//  ================================================================
JETUNITTEST( FMP, RangeStructAssignment )
//  ================================================================
{
    FMP::RANGE range1( 2, 3 );
    FMP::RANGE range2( 5, 6 );
    CHECK( !range1.FMatches( range2 ) );
    CHECK( !range2.FMatches( range1 ) );

    range2 = range1;
    CHECK( range1.FMatches( range2 ) );
    CHECK( range2.FMatches( range1 ) );

    range2.Set( 8, 9 );
    CHECK( !range1.FMatches( range2 ) );
    CHECK( !range2.FMatches( range1 ) );

    range2.Set( 2, 3 );
    CHECK( range1.FMatches( range2 ) );
    CHECK( range2.FMatches( range1 ) );

    FMP::RANGE rangeNull;
    CHECK( !range1.FMatches( rangeNull ) );
    CHECK( !rangeNull.FMatches( range1 ) );

    range1 = rangeNull;
    CHECK( range1.FMatches( rangeNull ) );
    CHECK( rangeNull.FMatches( range1 ) );

    range1.Set( 8, 9 );
    CHECK( !range1.FMatches( rangeNull ) );
    CHECK( !rangeNull.FMatches( range1 ) );

    range1.Set( pgnoNull, pgnoNull );
    CHECK( range1.FMatches( rangeNull ) );
    CHECK( rangeNull.FMatches( range1 ) );
}

//  ================================================================
JETUNITTEST( FMP, RangeStructComparisons )
//  ================================================================
{
    for ( CPG cpg1 = 1; cpg1 <= 5; cpg1++ )
    {
        const PGNO pgnoStart1 = 100;
        const PGNO pgnoEnd1 = pgnoStart1 + cpg1 - 1;
        const FMP::RANGE range1( pgnoStart1, pgnoEnd1 );

        CHECK( pgnoStart1 <= pgnoEnd1 );

        int cLeft, cRight, cContains, cOverlaps, cMatches;

        // Test FContains().
        cLeft = cRight = cContains = 0;
        for ( CPG cpgFromStart = -4; cpgFromStart <= ( cpg1 + 4 - 1 ); cpgFromStart++ )
        {
            const PGNO pgno = pgnoStart1 + cpgFromStart;

            if ( pgno < pgnoStart1 )
            {
                // To the left.
                cLeft++;
                CHECK( !range1.FContains( pgno ) );
            }
            else if ( pgno > pgnoEnd1 )
            {
                // To the right.
                cRight++;
                CHECK( !range1.FContains( pgno ) );
            }
            else
            {
                // Contains.
                cContains++;
                CHECK( range1.FContains( pgno ) );
            }
        }
        CHECK( cLeft == 4 );
        CHECK( cRight == 4 );
        CHECK( cContains == cpg1 );

        // Test FOverlaps() and FMatches().
        for ( CPG cpg2 = 1; cpg2 <= cpg1; cpg2++ )
        {
            cLeft = cRight = cOverlaps = cMatches = 0;
            for ( CPG cpgEnd2FromStart1 = -4; cpgEnd2FromStart1 <= ( cpg1 + cpg2 + 4 - 2 ); cpgEnd2FromStart1++ )
            {
                const PGNO pgnoEnd2 = pgnoStart1 + cpgEnd2FromStart1;
                const PGNO pgnoStart2 = pgnoEnd2 - cpg2 + 1;
                const FMP::RANGE range2( pgnoStart2, pgnoEnd2 );

                if ( pgnoEnd2 < pgnoStart1 )
                {
                    // To the left.
                    cLeft++;
                    CHECK( !range1.FOverlaps( range2 ) );
                    CHECK( !range1.FMatches( range2 ) );
                    CHECK( !range2.FOverlaps( range1 ) );
                    CHECK( !range2.FMatches( range1 ) );
                }
                else if ( pgnoStart2 > pgnoEnd1 )
                {
                    // To the right.
                    cRight++;
                    CHECK( !range1.FOverlaps( range2 ) );
                    CHECK( !range1.FMatches( range2 ) );
                    CHECK( !range2.FOverlaps( range1 ) );
                    CHECK( !range2.FMatches( range1 ) );
                }
                else
                {
                    // Overlaps.
                    cOverlaps++;
                    CHECK( range1.FOverlaps( range2 ) );
                    CHECK( range2.FOverlaps( range1 ) );

                    if ( pgnoStart1 == pgnoStart2 && pgnoEnd1 == pgnoEnd2 )
                    {
                        // Matches.
                        cMatches++;
                        CHECK( range1.FMatches( range2 ) );
                        CHECK( range2.FMatches( range1 ) );
                    }
                    else
                    {
                        CHECK( !range1.FMatches( range2 ) );
                        CHECK( !range2.FMatches( range1 ) );
                    }
                }
            }
            CHECK( cLeft == 4 );
            CHECK( cRight == 4 );
            CHECK( cOverlaps == ( cpg1 + cpg2 - 1 ) );
            if ( cpg1 == cpg2 )
            {
                CHECK( cMatches == 1 );
            }
            else
            {
                CHECK( cMatches == 0 );
            }
        }
    }
}

